#include "storage/irrigation_history.h"

#include <Preferences.h>
#include <string.h>

namespace {
Preferences histPrefs;
constexpr char kNs[] = "irrigation";
constexpr char kKey[] = "hist2";
constexpr char kOldKey[] = "hist";
constexpr char kSeqKey[] = "hseq";

uint32_t readU32(const uint8_t *p) {
  return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
         (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
}

uint16_t readU16(const uint8_t *p) {
  return static_cast<uint16_t>(p[0] | (p[1] << 8));
}
}  // namespace

IrrigationHistory g_history;

bool IrrigationHistory::begin() {
  if (!histPrefs.begin(kNs, false)) {
    return false;
  }
  next_seq_ = histPrefs.getUInt(kSeqKey, 1);
  if (next_seq_ == 0) {
    next_seq_ = 1;
  }

  uint8_t raw[1 + kMax * sizeof(IrrigationRecord)];
  const size_t n = histPrefs.getBytes(kKey, raw, sizeof(raw));
  const uint8_t stored = n >= 1 ? raw[0] : 0;
  if (stored > 0 && stored <= kMax && n >= 1 + stored * sizeof(IrrigationRecord)) {
    count_ = stored;
    memcpy(ring_, raw + 1, count_ * sizeof(IrrigationRecord));
  } else if (!migrateOld()) {
    count_ = 0;
  }

  uint32_t max_seq = 0;
  for (uint8_t i = 0; i < count_; ++i) {
    if (ring_[i].seq > max_seq) {
      max_seq = ring_[i].seq;
    }
  }
  if (next_seq_ <= max_seq) {
    next_seq_ = max_seq + 1;
  }
  return true;
}

bool IrrigationHistory::migrateOld() {
  // 旧记录 8 字节：ts u32、zone u8、对齐填充、volume u16、trigger u8
  uint8_t raw[1 + kMax * 8];
  const size_t n = histPrefs.getBytes(kOldKey, raw, sizeof(raw));
  if (n < 1) {
    return false;
  }
  const uint8_t stored = raw[0] > kMax ? kMax : raw[0];
  if (stored == 0 || n < 1 + static_cast<size_t>(stored) * 8) {
    return false;
  }
  count_ = stored;
  for (uint8_t i = 0; i < stored; ++i) {
    const uint8_t *p = raw + 1 + static_cast<size_t>(i) * 8;
    IrrigationRecord rec{};
    rec.ts = readU32(p);
    rec.zone = p[4];
    rec.volume_ml = readU16(p + 6);
    rec.trigger = p[7];
    rec.outcome = 0;
    rec.seq = next_seq_++;
    ring_[i] = rec;
  }
  persist();
  return true;
}

void IrrigationHistory::add(const IrrigationRecord& r) {
  IrrigationRecord copy = r;
  copy.seq = next_seq_++;
  if (next_seq_ == 0) {
    next_seq_ = 1;
  }
  if (count_ < kMax) {
    ring_[count_++] = copy;
  } else {
    memmove(ring_, ring_ + 1, (kMax - 1) * sizeof(IrrigationRecord));
    ring_[kMax - 1] = copy;
  }
  persist();
}

void IrrigationHistory::persist() {
  uint8_t raw[1 + kMax * sizeof(IrrigationRecord)];
  raw[0] = count_;
  if (count_ > 0) {
    memcpy(raw + 1, ring_, count_ * sizeof(IrrigationRecord));
  }
  histPrefs.putBytes(kKey, raw, 1 + count_ * sizeof(IrrigationRecord));
  histPrefs.putUInt(kSeqKey, next_seq_);
}

const IrrigationRecord* IrrigationHistory::get(uint8_t i) const {
  if (i >= count_) {
    return nullptr;
  }
  return &ring_[count_ - 1 - i];
}
