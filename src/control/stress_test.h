#pragma once

#include <stdint.h>

struct SystemContextEx;

// 稳定性测试：设定时长内，每 interval 分钟把全部盆依次入队浇水（volume_ml）
// 验证连续运行下泵/阀/流量计/历史记录的稳定性；记录保留最近 50 条 + 运行统计。
class StressTest {
 public:
  void begin(SystemContextEx *ctx);
  void tick();  // 主循环周期调用（每 ~100ms）
  void setEnabled(bool on);
  bool enabled() const { return enabled_; }

  uint32_t roundCount() const { return round_; }
  uint32_t okCount() const { return ok_; }
  uint32_t failCount() const { return fail_; }
  uint32_t totalMl() const { return total_ml_; }
  uint32_t startMs() const { return start_ms_; }
  uint32_t lastTriggerMs() const { return last_trigger_ms_; }

 private:
  SystemContextEx *ctx_ = nullptr;
  bool enabled_ = false;
  uint32_t start_ms_ = 0;
  uint32_t last_trigger_ms_ = 0;
  uint32_t round_ = 0;
  uint32_t ok_ = 0;
  uint32_t fail_ = 0;
  uint32_t total_ml_ = 0;
};

extern StressTest g_stress;
