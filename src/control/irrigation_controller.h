#pragma once

#include "config.h"
#include "actuator/pump_driver.h"
#include "actuator/valve_driver.h"
#include "control/zone_manager.h"
#include "safety/safety_monitor.h"
#include "sensor/flow_meter.h"
#include "types/zone_config.h"

struct IrrigateRequest {
  uint8_t zone;
  uint16_t volume_ml;
  IrrigateTrigger trigger;
  bool pending;
};

class IrrigationController {
 public:
  IrrigationController(ZoneManager *zones, PumpDriver *pump, ValveDriver *valves, FlowMeter *flow,
                       SafetyMonitor *safety, SystemConfig *config, ZoneConfig *zone_configs,
                       ZoneStatus *zone_status, SystemStatus *status);

  void begin();
  void tick();
  void stop();
  void emergencyStop();

  bool requestManual(uint8_t zone, uint16_t volume_ml);
  bool requestThreshold(uint8_t zone, uint16_t volume_ml);
  bool requestSchedule(uint8_t zone, uint16_t volume_ml);
  void resetScheduleFlags();

  const char *stateText() const;

 private:
  bool enqueue(uint8_t zone, uint16_t volume_ml, IrrigateTrigger trigger);
  void transitionTo(IrrigationState state);
  void startSession(uint8_t zone, uint16_t volume_ml, IrrigateTrigger trigger);
  void beginPumping();
  void finishSession(bool fault);
  void abortSession();
  void checkAutoTriggers();
  void processQueue();
  void syncActuatorStatus();
  uint16_t sessionVolumeMl() const;

  ZoneManager *zones_;
  PumpDriver *pump_;
  ValveDriver *valves_;
  FlowMeter *flow_;
  SafetyMonitor *safety_;
  SystemConfig *config_;
  ZoneConfig *zone_configs_;
  ZoneStatus *zone_status_;
  SystemStatus *status_;

  IrrigateRequest queue_[MAX_ZONES];
  uint8_t queue_head_ = 0;
  uint8_t queue_tail_ = 0;

  uint8_t active_zone_ = 255;
  uint16_t target_volume_ml_ = 0;
  uint32_t valve_start_ms_ = 0;
  uint32_t pump_start_ms_ = 0;
  uint32_t done_until_ms_ = 0;
  int last_yday_ = -1;
};
