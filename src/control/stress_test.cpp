#include "control/stress_test.h"

#include <Arduino.h>

#include "config.h"
#include "control/irrigation_controller.h"
#include "types/zone_config.h"

StressTest g_stress;

void StressTest::begin(SystemContextEx *ctx) { ctx_ = ctx; }

void StressTest::setEnabled(bool on) {
  if (ctx_ == nullptr || ctx_->config == nullptr) {
    return;
  }
  enabled_ = on;
  if (on) {
    start_ms_ = millis();
    // 立即触发第一轮（last 设为过去一个间隔）
    last_trigger_ms_ =
        start_ms_ - static_cast<uint32_t>(ctx_->config->test_interval_min) * 60U * 1000U;
    round_ = 0;
    ok_ = 0;
    fail_ = 0;
    total_ml_ = 0;
    Serial.printf("[stress] test ON: %u h, interval %u min, %u ml/zone\n",
                  ctx_->config->test_duration_h, ctx_->config->test_interval_min,
                  ctx_->config->test_volume_ml);
  } else {
    Serial.printf("[stress] test OFF: rounds=%lu ok=%lu fail=%lu total=%lu ml\n",
                  static_cast<unsigned long>(round_), static_cast<unsigned long>(ok_),
                  static_cast<unsigned long>(fail_), static_cast<unsigned long>(total_ml_));
  }
}

void StressTest::tick() {
  if (!enabled_ || ctx_ == nullptr || ctx_->controller == nullptr || ctx_->config == nullptr) {
    return;
  }
  if (ctx_->status->purge_on) {
    return;  // 排气中暂停
  }
  const uint32_t now = millis();

  // 到时自动停
  const uint32_t duration_ms =
      static_cast<uint32_t>(ctx_->config->test_duration_h) * 3600U * 1000U;
  if (now - start_ms_ >= duration_ms) {
    Serial.printf("[stress] test finished by timer\n");
    setEnabled(false);
    return;
  }

  const uint32_t interval_ms =
      static_cast<uint32_t>(ctx_->config->test_interval_min) * 60U * 1000U;
  if (now - last_trigger_ms_ >= interval_ms) {
    last_trigger_ms_ = now;
    ++round_;
    for (uint8_t z = 0; z < ctx_->config->zone_count; ++z) {
      if (ctx_->controller->requestTest(z, ctx_->config->test_volume_ml)) {
        ++ok_;
        total_ml_ += ctx_->config->test_volume_ml;
      } else {
        ++fail_;
      }
    }
    Serial.printf("[stress] round %lu: %lu ok / %lu fail\n",
                  static_cast<unsigned long>(round_), static_cast<unsigned long>(ok_),
                  static_cast<unsigned long>(fail_));
  }
}
