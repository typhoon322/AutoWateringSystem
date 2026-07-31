#include "control/irrigation_controller.h"

#include <Arduino.h>
#include <string.h>
#include <time.h>

IrrigationController::IrrigationController(ZoneManager *zones, PumpDriver *pump, FlowMeter *flow,
                                           SafetyMonitor *safety, SystemConfig *config,
                                           ZoneConfig *zone_configs, ZoneStatus *zone_status,
                                           SystemStatus *status)
    : zones_(zones),
      pump_(pump),
      flow_(flow),
      safety_(safety),
      config_(config),
      zone_configs_(zone_configs),
      zone_status_(zone_status),
      status_(status) {
  memset(queue_, 0, sizeof(queue_));
}

void IrrigationController::begin() {
  status_->state = IrrigationState::Idle;
  status_->safety = SafetyState::Ok;
  status_->pump_on = false;
  status_->active_zone = -1;
  status_->daily_ml = 0;
  status_->queue_len = 0;
  status_->session_ml = 0;
  status_->trigger = IrrigateTrigger::None;
  last_yday_ = -1;
}

void IrrigationController::transitionTo(IrrigationState state) { status_->state = state; }

const char *IrrigationController::stateText() const {
  switch (status_->state) {
    case IrrigationState::Idle:
      return "IDLE";
    case IrrigationState::Checking:
      return "CHECKING";
    case IrrigationState::Pumping:
      return "PUMPING";
    case IrrigationState::Done:
      return "DONE";
    case IrrigationState::Fault:
      return "FAULT";
    default:
      return "UNKNOWN";
  }
}

bool IrrigationController::enqueue(uint8_t zone, uint16_t volume_ml, IrrigateTrigger trigger) {
  if (zone >= config_->zone_count) {
    return false;
  }
  const uint8_t next = (queue_tail_ + 1) % 4;
  if (next == queue_head_) {
    return false;
  }
  queue_[queue_tail_] = {zone, volume_ml, trigger, true};
  queue_tail_ = next;
  status_->queue_len = (queue_tail_ >= queue_head_) ? (queue_tail_ - queue_head_)
                                                    : (4 - queue_head_ + queue_tail_);
  return true;
}

bool IrrigationController::requestManual(uint8_t zone, uint16_t volume_ml) {
  return enqueue(zone, volume_ml, IrrigateTrigger::Manual);
}

bool IrrigationController::requestThreshold(uint8_t zone, uint16_t volume_ml) {
  return enqueue(zone, volume_ml, IrrigateTrigger::Threshold);
}

bool IrrigationController::requestSchedule(uint8_t zone, uint16_t volume_ml) {
  return enqueue(zone, volume_ml, IrrigateTrigger::Schedule);
}

void IrrigationController::resetScheduleFlags() {
  for (uint8_t i = 0; i < config_->zone_count; ++i) {
    zone_configs_[i].schedule_fired_today = false;
  }
}

void IrrigationController::startPumping(uint8_t zone, uint16_t volume_ml, IrrigateTrigger trigger) {
  active_zone_ = zone;
  target_volume_ml_ = volume_ml > 0 ? volume_ml : zone_configs_[zone].volume_ml;
  pump_start_ms_ = millis();
  flow_->resetSession();
  pump_->set(true);
  status_->pump_on = true;
  status_->active_zone = static_cast<int8_t>(zone);
  status_->trigger = trigger;
  status_->session_ml = 0;
  transitionTo(IrrigationState::Pumping);
}

void IrrigationController::finishSession(bool fault) {
  const uint16_t vol = flow_->volumeMl(config_->pulses_per_liter);
  if (vol > 0) {
    safety_->addDailyMl(vol);
  }
  pump_->set(false);
  status_->pump_on = false;
  status_->session_ml = vol;
  status_->daily_ml = safety_->dailyMl();
  active_zone_ = 255;
  status_->active_zone = -1;

  if (fault) {
    transitionTo(IrrigationState::Fault);
  } else {
    transitionTo(IrrigationState::Done);
    done_until_ms_ = millis() + 2000;
  }
}

void IrrigationController::processQueue() {
  if (queue_head_ == queue_tail_) {
    return;
  }
  if (status_->state == IrrigationState::Pumping) {
    return;
  }
  if (status_->state == IrrigationState::Fault && safety_->isLocked()) {
    return;
  }
  IrrigateRequest req = queue_[queue_head_];
  queue_head_ = (queue_head_ + 1) % 4;
  status_->queue_len = (queue_tail_ >= queue_head_) ? (queue_tail_ - queue_head_)
                                                    : (4 - queue_head_ + queue_tail_);
  if (req.pending) {
    startPumping(req.zone, req.volume_ml, req.trigger);
  }
}

void IrrigationController::checkAutoTriggers() {
  if (!safety_->canAutoIrrigate(safety_->dailyMl())) {
    return;
  }

  time_t now;
  time(&now);
  struct tm ti;
  localtime_r(&now, &ti);

  if (last_yday_ >= 0 && ti.tm_yday != last_yday_) {
    resetScheduleFlags();
  }
  last_yday_ = ti.tm_yday;

  for (uint8_t z = 0; z < config_->zone_count; ++z) {
    ZoneConfig &zc = zone_configs_[z];

    if (zc.schedule_enabled && !zc.schedule_fired_today &&
        static_cast<uint8_t>(ti.tm_hour) == zc.schedule_hour &&
        static_cast<uint8_t>(ti.tm_min) == zc.schedule_minute) {
      if (enqueue(z, zc.volume_ml, IrrigateTrigger::Schedule)) {
        zc.schedule_fired_today = true;
      }
    }

    if (zc.auto_enabled && zones_->needsWater(z)) {
      enqueue(z, zc.volume_ml, IrrigateTrigger::Threshold);
    }
  }
}

void IrrigationController::stop() {
  if (pump_->isOn()) {
    finishSession(false);
  }
  safety_->unlock();
  transitionTo(IrrigationState::Idle);
  queue_head_ = queue_tail_;
  status_->queue_len = 0;
}

void IrrigationController::emergencyStop() {
  if (pump_->isOn()) {
    const uint16_t vol = flow_->volumeMl(config_->pulses_per_liter);
    if (vol > 0) {
      safety_->addDailyMl(vol);
    }
  }
  pump_->set(false);
  status_->pump_on = false;
  active_zone_ = 255;
  status_->active_zone = -1;
  safety_->lock(SafetyState::Locked);
  transitionTo(IrrigationState::Fault);
  queue_head_ = queue_tail_;
  status_->queue_len = 0;
}

void IrrigationController::tick() {
  safety_->resetDailyIfNewDay();
  status_->daily_ml = safety_->dailyMl();
  status_->safety = safety_->state();

  if (status_->state == IrrigationState::Done) {
    if (millis() >= done_until_ms_) {
      transitionTo(IrrigationState::Idle);
    }
    processQueue();
    return;
  }

  if (status_->state == IrrigationState::Fault) {
    if (!safety_->isLocked()) {
      transitionTo(IrrigationState::Idle);
    }
    return;
  }

  if (status_->state == IrrigationState::Pumping) {
    const uint16_t vol = flow_->volumeMl(config_->pulses_per_liter);
    status_->session_ml = vol;

    if (safety_->checkTimeout(pump_start_ms_)) {
      safety_->lock(SafetyState::Timeout);
      finishSession(true);
      return;
    }
    if (safety_->checkDryRun(pump_start_ms_, flow_->pulses())) {
      safety_->lock(SafetyState::DryRun);
      finishSession(true);
      return;
    }

    if (status_->trigger == IrrigateTrigger::Threshold && active_zone_ < config_->zone_count) {
      if (zone_status_[active_zone_].sensor_valid &&
          zone_status_[active_zone_].moisture_pct >= zone_configs_[active_zone_].moisture_high) {
        finishSession(false);
        return;
      }
    }

    if (vol >= target_volume_ml_) {
      finishSession(false);
      return;
    }
    return;
  }

  processQueue();
  if (status_->state == IrrigationState::Pumping) {
    return;
  }

  if (config_->daily_limit_ml > 0 && safety_->dailyMl() >= config_->daily_limit_ml) {
    status_->safety = SafetyState::DailyLimit;
    return;
  }

  transitionTo(IrrigationState::Checking);
  checkAutoTriggers();
  transitionTo(IrrigationState::Idle);
  processQueue();
}
