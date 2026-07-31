#include "sensor/flow_meter.h"

#include <Arduino.h>

static FlowMeter *g_flow_instance = nullptr;

static void IRAM_ATTR flowIsr() {
  if (g_flow_instance != nullptr) {
    g_flow_instance->onPulse();
  }
}

FlowMeter::FlowMeter(uint8_t pin) : pin_(pin), pulses_(0) {}

void FlowMeter::begin() {
  pinMode(pin_, INPUT_PULLUP);
  g_flow_instance = this;
  attachInterrupt(digitalPinToInterrupt(pin_), flowIsr, RISING);
}

void FlowMeter::resetSession() { pulses_ = 0; }

uint32_t FlowMeter::pulses() const { return pulses_; }

uint16_t FlowMeter::volumeMl(uint16_t pulses_per_liter) const {
  if (pulses_per_liter == 0) {
    return 0;
  }
  const uint64_t ml = (static_cast<uint64_t>(pulses_) * 1000ULL) / pulses_per_liter;
  if (ml > 65535ULL) {
    return 65535;
  }
  return static_cast<uint16_t>(ml);
}

void FlowMeter::onPulse() { pulses_++; }
