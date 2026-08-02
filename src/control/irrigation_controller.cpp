#include "control/irrigation_controller.h"

#include <Arduino.h>
#include <string.h>
#include <time.h>

#include "config.h"

IrrigationController::IrrigationController(ZoneManager *zones, PumpDriver *pump, ValveDriver *valves,
                                           FlowMeter *flow, SafetyMonitor *safety,
                                           SystemConfig *config, ZoneConfig *zone_configs,
                                           ZoneStatus *zone_status, SystemStatus *status)
    : zones_(zones),
      pump_(pump),
      valves_(valves),
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
  status_->valve_on = false;
  status_->active_zone = -1;
  status_->active_valve = -1;
  status_->daily_ml = 0;
  status_->queue_len = 0;
  status_->session_ml = 0;
  status_->trigger = IrrigateTrigger::None;
  last_yday_ = -1;
  if (valves_ != nullptr) {
    valves_->closeAll();
  }
  pump_->set(false);
}

void IrrigationController::transitionTo(IrrigationState state) { status_->state = state; }

void IrrigationController::syncActuatorStatus() {
  status_->pump_on = pump_->isOn();
  status_->valve_on = valves_ != nullptr && valves_->activeZone() >= 0;
  status_->active_valve = valves_ != nullptr ? valves_->activeZone() : -1;
}

const char *IrrigationController::stateText() const {
  switch (status_->state) {
    case IrrigationState::Idle:
      return "IDLE";
    case IrrigationState::Checking:
      return "CHECKING";
    case IrrigationState::Valving:
      return "VALVING";
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
  const uint8_t next = (queue_tail_ + 1) % MAX_ZONES;
  if (next == queue_head_) {
    return false;
  }
  queue_[queue_tail_] = {zone, volume_ml, trigger, true};
  queue_tail_ = next;
  status_->queue_len = (queue_tail_ >= queue_head_) ? (queue_tail_ - queue_head_)
                                                    : (MAX_ZONES - queue_head_ + queue_tail_);
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

void IrrigationController::startSession(uint8_t zone, uint16_t volume_ml, IrrigateTrigger trigger) {
  active_zone_ = zone;
  target_volume_ml_ = volume_ml > 0 ? volume_ml : zone_configs_[zone].volume_ml;
  status_->active_zone = static_cast<int8_t>(zone);
  status_->trigger = trigger;
  status_->session_ml = 0;

  if (valves_ != nullptr) {
    valves_->open(zone);
    valve_start_ms_ = millis();
    transitionTo(IrrigationState::Valving);
  } else {
    beginPumping();
  }
  syncActuatorStatus();
}

void IrrigationController::beginPumping() {
  pump_start_ms_ = millis();
  if (flow_ != nullptr) {
    flow_->resetSession();
  }
  pump_->set(true);
  transitionTo(IrrigationState::Pumping);
  syncActuatorStatus();
}

uint16_t IrrigationController::sessionVolumeMl() const {
  if (flow_ != nullptr) {
    return flow_->volumeMl(config_->pulses_per_liter);
  }
  const uint32_t elapsed_ms = millis() - pump_start_ms_;
  const uint32_t ml =
      (elapsed_ms * static_cast<uint32_t>(DEFAULT_PUMP_FLOW_ML_PER_SEC)) / 1000U;
  return ml > 65535U ? 65535U : static_cast<uint16_t>(ml);
}

void IrrigationController::finishSession(bool fault) {
  const uint16_t vol = sessionVolumeMl();
  if (vol > 0) {
    safety_->addDailyMl(vol);
  }
  pump_->set(false);
  if (valves_ != nullptr) {
    valves_->closeAll();
  }
  status_->session_ml = vol;
  status_->daily_ml = safety_->dailyMl();
  active_zone_ = 255;
  status_->active_zone = -1;
  syncActuatorStatus();

  if (fault) {
    transitionTo(IrrigationState::Fault);
  } else {
    transitionTo(IrrigationState::Done);
    done_until_ms_ = millis() + 2000;
  }
}

void IrrigationController::abortSession() {
  pump_->set(false);
  if (valves_ != nullptr) {
    valves_->closeAll();
  }
  active_zone_ = 255;
  status_->active_zone = -1;
  status_->session_ml = 0;
  syncActuatorStatus();
}

void IrrigationController::processQueue() {
  if (queue_head_ == queue_tail_) {
    return;
  }
  if (status_->state == IrrigationState::Pumping || status_->state == IrrigationState::Valving) {
    return;
  }
  if (status_->state == IrrigationState::Fault && safety_->isLocked()) {
    return;
  }
  IrrigateRequest req = queue_[queue_head_];
  queue_head_ = (queue_head_ + 1) % MAX_ZONES;
  status_->queue_len = (queue_tail_ >= queue_head_) ? (queue_tail_ - queue_head_)
                                                    : (MAX_ZONES - queue_head_ + queue_tail_);
  if (req.pending) {
    startSession(req.zone, req.volume_ml, req.trigger);
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
  if (status_->state == IrrigationState::Pumping) {
    finishSession(false);
  } else if (status_->state == IrrigationState::Valving) {
    abortSession();
  }
  safety_->unlock();
  status_->safety = SafetyState::Ok;
  transitionTo(IrrigationState::Idle);
  active_zone_ = 255;
  status_->active_zone = -1;
  queue_head_ = queue_tail_;
  status_->queue_len = 0;
  syncActuatorStatus();
}

void IrrigationController::emergencyStop() {
  if (pump_->isOn()) {
    const uint16_t vol = sessionVolumeMl();
    if (vol > 0) {
      safety_->addDailyMl(vol);
    }
  }
  abortSession();
  safety_->lock(SafetyState::Locked);
  transitionTo(IrrigationState::Fault);
  queue_head_ = queue_tail_;
  status_->queue_len = 0;
}

void IrrigationController::tick() {
  safety_->resetDailyIfNewDay();
  status_->daily_ml = safety_->dailyMl();
  status_->safety = safety_->state();
  syncActuatorStatus();

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

  if (status_->state == IrrigationState::Valving) {
    if (millis() - valve_start_ms_ >= DEFAULT_VALVE_SETTLE_MS) {
      beginPumping();
    }
    return;
  }

  if (status_->state == IrrigationState::Pumping) {
    const uint16_t vol = sessionVolumeMl();
    status_->session_ml = vol;

    if (safety_->checkTimeout(pump_start_ms_)) {
      safety_->lock(SafetyState::Timeout);
      finishSession(true);
      return;
    }
    if (flow_ != nullptr && safety_->checkDryRun(pump_start_ms_, flow_->pulses())) {
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
  if (status_->state == IrrigationState::Pumping || status_->state == IrrigationState::Valving) {
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
