#pragma once

#include <stdint.h>

struct IrrigationRecord {
  uint32_t ts;        // unix 时间（NTP；未同步为 0）
  uint8_t  zone;      // 0-9
  uint16_t volume_ml; // 实际浇水量
  uint8_t  trigger;   // IrrigateTrigger: 1=阈值 2=定时 3=手动
};

// 环形队列 50 条，NVS blob 持久化（重启不清零）
class IrrigationHistory {
 public:
  bool begin();                          // 从 NVS 加载
  void add(const IrrigationRecord& r);   // 追加并持久化
  uint8_t count() const { return count_; }
  const IrrigationRecord* get(uint8_t i) const;  // 0=最新

 private:
  static constexpr uint8_t kMax = 50;
  IrrigationRecord ring_[kMax];
  uint8_t count_ = 0;
};

extern IrrigationHistory g_history;
