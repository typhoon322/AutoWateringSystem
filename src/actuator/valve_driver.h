#pragma once

#include <stdint.h>

class ValveDriver {
 public:
  void begin(uint8_t count, bool active_high);
  void open(uint8_t zone);
  void closeAll();
  bool isOpen(uint8_t zone) const;
  int8_t activeZone() const;

 private:
  void writeZone(uint8_t zone, bool on);

  uint8_t count_ = 0;
  bool active_high_ = true;
  int8_t active_ = -1;
};
