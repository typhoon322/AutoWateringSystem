#pragma once

#include <Arduino.h>

#include "types/zone_config.h"

class SerialCli {
 public:
  void begin(SystemContextEx *ctx);
  void poll();
  void printStatus() const;

 private:
  void dispatch(const char *line);
  void printHelp() const;

  SystemContextEx *ctx_ = nullptr;
  String line_buf_;
};
