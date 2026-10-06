#pragma once

#include <stdint.h>

// outcome：0 完成，1 干转，2 超时，3 急停，4 其他故障
#pragma pack(push, 1)
struct IrrigationRecord {
  uint32_t ts;
  uint32_t seq;
  uint16_t volume_ml;
  uint8_t zone;
  uint8_t trigger;
  uint8_t outcome;
};
#pragma pack(pop)
static_assert(sizeof(IrrigationRecord) == 13, "history record size");

// 环形队列 50 条，NVS 持久化。更早的记录由手机在同步时留下。
class IrrigationHistory {
 public:
  bool begin();
  void add(const IrrigationRecord& r);
  void clear();  // 只清记录，序号继续往后走
  uint8_t count() const { return count_; }
  const IrrigationRecord* get(uint8_t i) const;  // 0=最新

 private:
  void persist();
  bool migrateOld();

  static constexpr uint8_t kMax = 50;
  IrrigationRecord ring_[kMax];
  uint8_t count_ = 0;
  uint32_t next_seq_ = 1;
};

extern IrrigationHistory g_history;
