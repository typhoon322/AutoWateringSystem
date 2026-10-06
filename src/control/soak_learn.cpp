#include "control/soak_learn.h"

#include <Arduino.h>
#include <Preferences.h>
#include <string.h>

#include "storage/settings_store.h"

extern SettingsStore g_settings;
extern SystemConfig g_sys_config;
extern ZoneConfig g_zone_configs[MAX_ZONES];
extern ZoneStatus g_zone_status[MAX_ZONES];
extern SystemStatus g_sys_status;

namespace {
constexpr uint8_t kKeep = 12;
constexpr uint8_t kSamples = 40;
constexpr uint16_t kWindowSec = 180;
constexpr uint16_t kMaxSec = 3600;
constexpr uint32_t kMagic = 0x534F414Bu;  // SOAK
constexpr uint16_t kMinSpanSec = kWindowSec - (SAMPLE_INTERVAL_MS / 1000);

struct PersistZone {
  uint8_t count;
  uint8_t paused;
  uint8_t last_min;
  uint8_t reserved;
  uint16_t sec[kKeep];
};

struct Blob {
  uint32_t magic;
  PersistZone zone[MAX_ZONES];
};

struct Sample {
  uint16_t elapsed_s;
  uint8_t pct;
};

struct Watch {
  bool active = false;
  uint32_t t0_ms = 0;
  uint32_t last_ms = 0;
  uint8_t n = 0;
  Sample samples[kSamples]{};
};

Preferences prefs;
Blob blob{};
Watch watches[MAX_ZONES]{};
bool opened = false;

void saveBlob() {
  if (!opened) {
    return;
  }
  prefs.putBytes("obs", &blob, sizeof(blob));
}

void saveConfigs() {
  SystemContext sc = {&g_sys_config, g_zone_configs, g_zone_status, &g_sys_status};
  g_settings.save(sc);
}

int absDiff(int a, int b) {
  const int d = a - b;
  return d < 0 ? -d : d;
}

int recommend(const uint16_t *sec, uint8_t n) {
  if (n < 2) {
    return -1;
  }
  uint16_t tmp[kKeep];
  const uint8_t use = n > kKeep ? kKeep : n;
  memcpy(tmp, sec, use * sizeof(uint16_t));
  for (uint8_t i = 1; i < use; ++i) {
    const uint16_t v = tmp[i];
    int j = i;
    while (j > 0 && tmp[j - 1] > v) {
      tmp[j] = tmp[j - 1];
      --j;
    }
    tmp[j] = v;
  }
  const int index = ((static_cast<int>(use) * 3 + 3) / 4) - 1;
  int minutes = (static_cast<int>(tmp[index]) + 59) / 60;
  if (minutes < 5) {
    minutes = 5;
  }
  if (minutes > 60) {
    minutes = 60;
  }
  return minutes;
}

bool windowStable(const Watch &watch, uint16_t now_s) {
  if (watch.n < 4 || now_s < kWindowSec) {
    return false;
  }
  const uint16_t begin = now_s - kWindowSec;
  uint8_t lo = 255;
  uint8_t hi = 0;
  uint8_t count = 0;
  uint16_t first = 0;
  uint16_t last = 0;
  bool any = false;
  for (uint8_t i = 0; i < watch.n; ++i) {
    if (watch.samples[i].elapsed_s < begin) {
      continue;
    }
    if (!any) {
      first = watch.samples[i].elapsed_s;
      any = true;
    }
    last = watch.samples[i].elapsed_s;
    if (watch.samples[i].pct < lo) {
      lo = watch.samples[i].pct;
    }
    if (watch.samples[i].pct > hi) {
      hi = watch.samples[i].pct;
    }
    ++count;
  }
  if (!any || count < 4 || last < first || (last - first) < kMinSpanSec) {
    return false;
  }
  return static_cast<uint8_t>(hi - lo) <= 5;
}

void pushSample(Watch &watch, uint16_t elapsed_s, uint8_t pct) {
  if (watch.n >= kSamples) {
    memmove(watch.samples, watch.samples + 1, (kSamples - 1) * sizeof(Sample));
    watch.n = kSamples - 1;
  }
  watch.samples[watch.n++] = {elapsed_s, pct};
}

void finishWatch(uint8_t zone, uint16_t settle_s, ZoneConfig *configs) {
  Watch &watch = watches[zone];
  watch.active = false;
  PersistZone &row = blob.zone[zone];
  if (row.count >= kKeep) {
    memmove(row.sec, row.sec + 1, (kKeep - 1) * sizeof(uint16_t));
    row.count = kKeep - 1;
  }
  row.sec[row.count++] = settle_s;

  const int minutes = recommend(row.sec, row.count);
  const int previous = row.last_min >= 5 ? row.last_min : -1;
  const bool agree = minutes > 0 && previous > 0 && row.count >= 3 && absDiff(minutes, previous) < 2;
  if (minutes > 0) {
    row.last_min = static_cast<uint8_t>(minutes);
  }

  bool changed = false;
  uint8_t current = configs[zone].soak_min;
  if (current < 5 || current > 60) {
    current = DEFAULT_SOAK_MIN;
  }
  if (minutes > 0 && !row.paused && absDiff(minutes, current) >= 2) {
    configs[zone].soak_min = static_cast<uint8_t>(minutes);
    current = static_cast<uint8_t>(minutes);
    changed = true;
  }
  if (agree && minutes > 0 && absDiff(minutes, current) < 2) {
    row.paused = 1;
  }
  saveBlob();
  if (changed) {
    saveConfigs();
  }
  Serial.printf("soak %u %us -> %d min%s\n", static_cast<unsigned>(zone + 1), settle_s, minutes,
                row.paused ? " pause" : "");
}
}  // namespace

void soak_learn_begin() {
  memset(&blob, 0, sizeof(blob));
  opened = prefs.begin("soak", false);
  if (!opened) {
    return;
  }
  Blob loaded{};
  const size_t n = prefs.getBytes("obs", &loaded, sizeof(loaded));
  if (n == sizeof(loaded) && loaded.magic == kMagic) {
    blob = loaded;
    for (uint8_t i = 0; i < MAX_ZONES; ++i) {
      if (blob.zone[i].count > kKeep) {
        blob.zone[i].count = kKeep;
      }
    }
  } else {
    blob.magic = kMagic;
  }
}

void soak_learn_on_dose_started(uint8_t zone) {
  if (zone >= MAX_ZONES) {
    return;
  }
  watches[zone].active = false;
  watches[zone].n = 0;
}

void soak_learn_on_dose_finished(uint8_t zone) {
  if (zone >= MAX_ZONES || blob.zone[zone].paused) {
    return;
  }
  Watch &watch = watches[zone];
  watch.active = true;
  watch.t0_ms = millis();
  watch.last_ms = 0;
  watch.n = 0;
}

void soak_learn_restart(uint8_t zone) {
  if (zone >= MAX_ZONES) {
    return;
  }
  watches[zone].active = false;
  watches[zone].n = 0;
  memset(&blob.zone[zone], 0, sizeof(PersistZone));
  saveBlob();
  Serial.printf("soak %u restart\n", static_cast<unsigned>(zone + 1));
}

void soak_learn_tick(const ZoneStatus *zones, uint8_t count, ZoneConfig *configs) {
  if (zones == nullptr || configs == nullptr) {
    return;
  }
  const uint32_t now = millis();
  const uint8_t n = count > MAX_ZONES ? MAX_ZONES : count;
  for (uint8_t z = 0; z < n; ++z) {
    Watch &watch = watches[z];
    if (!watch.active || blob.zone[z].paused) {
      watch.active = false;
      continue;
    }
    const uint32_t elapsed_ms = now - watch.t0_ms;
    const uint16_t elapsed_s = elapsed_ms / 1000U > kMaxSec ? kMaxSec : static_cast<uint16_t>(elapsed_ms / 1000U);
    if (watch.last_ms != 0 && now - watch.last_ms < SAMPLE_INTERVAL_MS) {
      continue;
    }
    watch.last_ms = now;
    if (zones[z].sensor_valid) {
      pushSample(watch, elapsed_s, zones[z].moisture_pct);
    }
    const bool timed_out = elapsed_s >= kMaxSec;
    if (windowStable(watch, elapsed_s) || timed_out) {
      finishWatch(z, elapsed_s, configs);
    }
  }
}

uint8_t soak_learn_count(uint8_t zone) {
  if (zone >= MAX_ZONES) {
    return 0;
  }
  return blob.zone[zone].count;
}

bool soak_learn_paused(uint8_t zone) {
  if (zone >= MAX_ZONES) {
    return false;
  }
  return blob.zone[zone].paused != 0;
}

bool soak_learn_watching(uint8_t zone) {
  if (zone >= MAX_ZONES) {
    return false;
  }
  return watches[zone].active;
}
