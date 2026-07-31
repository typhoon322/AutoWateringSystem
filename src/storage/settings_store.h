#pragma once

#include "types/zone_config.h"

class SettingsStore {
 public:
  bool begin();
  bool load(SystemContext &ctx);
  bool save(const SystemContext &ctx);
  void applyDefaults(SystemContext &ctx);

 private:
  static constexpr const char *kNamespace = "irrigation";
  bool opened_ = false;
};
