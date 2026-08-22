#include "storage/irrigation_history.h"

#include <Preferences.h>
#include <string.h>

namespace {
Preferences histPrefs;
constexpr char kNs[] = "irrigation";
constexpr char kKey[] = "hist";
}  // namespace

IrrigationHistory g_history;

bool IrrigationHistory::begin() {
  if (!histPrefs.begin(kNs, false)) {
    return false;
  }
  uint8_t raw[1 + kMax * sizeof(IrrigationRecord)];
  const size_t n = histPrefs.getBytes(kKey, raw, sizeof(raw));
  if (n < 1) {
    return true;  // 无历史
  }
  count_ = raw[0] > kMax ? kMax : raw[0];
  if (count_ > 0) {
    memcpy(ring_, raw + 1, count_ * sizeof(IrrigationRecord));
  }
  return true;
}

void IrrigationHistory::add(const IrrigationRecord& r) {
  if (count_ < kMax) {
    ring_[count_++] = r;
  } else {
    memmove(ring_, ring_ + 1, (kMax - 1) * sizeof(IrrigationRecord));
    ring_[kMax - 1] = r;
  }
  uint8_t raw[1 + kMax * sizeof(IrrigationRecord)];
  raw[0] = count_;
  memcpy(raw + 1, ring_, count_ * sizeof(IrrigationRecord));
  histPrefs.putBytes(kKey, raw, 1 + count_ * sizeof(IrrigationRecord));
}

const IrrigationRecord* IrrigationHistory::get(uint8_t i) const {
  if (i >= count_) {
    return nullptr;
  }
  return &ring_[count_ - 1 - i];  // 0=最新
}
