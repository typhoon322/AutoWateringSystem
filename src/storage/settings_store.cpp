#include "storage/settings_store.h"

#include <Preferences.h>
#include <string.h>

#include "config.h"

namespace {
Preferences prefs;

void copyString(char *dest, size_t destLen, const char *src) {
  if (destLen == 0) {
    return;
  }
  strncpy(dest, src != nullptr ? src : "", destLen - 1);
  dest[destLen - 1] = '\0';
}

char keyBuf[8];

const char *zoneKey(const char *prefix, uint8_t i) {
  snprintf(keyBuf, sizeof(keyBuf), "%s%u", prefix, i);
  return keyBuf;
}
}  // namespace

bool SettingsStore::begin() {
  opened_ = prefs.begin(kNamespace, false);
  return opened_;
}

void SettingsStore::applyDefaults(SystemContext &ctx) {
  if (ctx.config == nullptr || ctx.zones == nullptr) {
    return;
  }
  ctx.config->max_run_sec = DEFAULT_MAX_RUN_SEC;
  ctx.config->daily_limit_ml = DEFAULT_DAILY_LIMIT_ML;
  ctx.config->pulses_per_liter = DEFAULT_PULSES_PER_LITER;
  ctx.config->dry_run_sec = DEFAULT_DRY_RUN_SEC;
  ctx.config->zone_count = ACTIVE_ZONES;
  ctx.config->wifi_enabled = RADIO_WIFI_DEFAULT_ENABLED != 0;
  copyString(ctx.config->wifi_ssid, sizeof(ctx.config->wifi_ssid), WIFI_SSID);
  copyString(ctx.config->wifi_pass, sizeof(ctx.config->wifi_pass), WIFI_PASS);
  ctx.config->auto_window_enabled = true;
  ctx.config->auto_win_sh = 17;
  ctx.config->auto_win_sm = 0;
  ctx.config->auto_win_eh = 21;
  ctx.config->auto_win_em = 0;

  for (uint8_t i = 0; i < MAX_ZONES; ++i) {
    char name[16];
    snprintf(name, sizeof(name), "Zone%u", i);
    copyString(ctx.zones[i].name, sizeof(ctx.zones[i].name), name);
    ctx.zones[i].moisture_low = DEFAULT_MOISTURE_LOW;
    ctx.zones[i].moisture_high = DEFAULT_MOISTURE_HIGH;
    ctx.zones[i].volume_ml = DEFAULT_VOLUME_ML;
    ctx.zones[i].auto_enabled = false;
    ctx.zones[i].schedule_enabled = false;
    ctx.zones[i].schedule_hour = 8;
    ctx.zones[i].schedule_minute = 0;
    ctx.zones[i].cal_dry = DEFAULT_CAL_DRY;
    ctx.zones[i].cal_wet = DEFAULT_CAL_WET;
    ctx.zones[i].schedule_fired_today = false;
    ctx.zones[i].window_override = false;
    ctx.zones[i].win_sh = 17; ctx.zones[i].win_sm = 0;
    ctx.zones[i].win_eh = 21; ctx.zones[i].win_em = 0;
  }
}

bool SettingsStore::load(SystemContext &ctx) {
  if (!opened_ || ctx.config == nullptr || ctx.zones == nullptr) {
    return false;
  }

  const uint32_t magic = prefs.getUInt("magic", 0);
  if (magic != NVS_MAGIC) {
    applyDefaults(ctx);
    return false;
  }

  ctx.config->max_run_sec = prefs.getUShort("maxRun", DEFAULT_MAX_RUN_SEC);
  ctx.config->daily_limit_ml = prefs.getUInt("dailyLim", DEFAULT_DAILY_LIMIT_ML);
  ctx.config->pulses_per_liter = prefs.getUShort("ppl", DEFAULT_PULSES_PER_LITER);
  ctx.config->dry_run_sec = prefs.getUChar("dryRun", DEFAULT_DRY_RUN_SEC);
  ctx.config->zone_count = prefs.getUChar("zCnt", ACTIVE_ZONES);
  if (ctx.config->zone_count > MAX_ZONES) {
    ctx.config->zone_count = MAX_ZONES;
  }
  ctx.config->wifi_enabled = prefs.getUChar("wifiEn", RADIO_WIFI_DEFAULT_ENABLED) != 0;
  ctx.config->auto_window_enabled = prefs.getUChar("aWinEn", 1) != 0;
  ctx.config->auto_win_sh = prefs.getUChar("aWinSH", 17);
  ctx.config->auto_win_sm = prefs.getUChar("aWinSM", 0);
  ctx.config->auto_win_eh = prefs.getUChar("aWinEH", 21);
  ctx.config->auto_win_em = prefs.getUChar("aWinEM", 0);

  String ssid = prefs.getString("wifiSSID", "");
  String pass = prefs.getString("wifiPass", "");
  if (ssid.length() == 0) {
    copyString(ctx.config->wifi_ssid, sizeof(ctx.config->wifi_ssid), WIFI_SSID);
    copyString(ctx.config->wifi_pass, sizeof(ctx.config->wifi_pass), WIFI_PASS);
  } else {
    copyString(ctx.config->wifi_ssid, sizeof(ctx.config->wifi_ssid), ssid.c_str());
    copyString(ctx.config->wifi_pass, sizeof(ctx.config->wifi_pass), pass.c_str());
  }

  for (uint8_t i = 0; i < ctx.config->zone_count; ++i) {
    String name = prefs.getString(zoneKey("zN", i), "");
    if (name.length() == 0) {
      char def[16];
      snprintf(def, sizeof(def), "Zone%u", i);
      copyString(ctx.zones[i].name, sizeof(ctx.zones[i].name), def);
    } else {
      copyString(ctx.zones[i].name, sizeof(ctx.zones[i].name), name.c_str());
    }
    ctx.zones[i].moisture_low = prefs.getUChar(zoneKey("zML", i), DEFAULT_MOISTURE_LOW);
    ctx.zones[i].moisture_high = prefs.getUChar(zoneKey("zMH", i), DEFAULT_MOISTURE_HIGH);
    ctx.zones[i].volume_ml = prefs.getUShort(zoneKey("zVol", i), DEFAULT_VOLUME_ML);
    ctx.zones[i].auto_enabled = prefs.getUChar(zoneKey("zAuto", i), 1) != 0;
    ctx.zones[i].schedule_enabled = prefs.getUChar(zoneKey("zSchE", i), 0) != 0;
    ctx.zones[i].schedule_hour = prefs.getUChar(zoneKey("zSchH", i), 8);
    ctx.zones[i].schedule_minute = prefs.getUChar(zoneKey("zSchM", i), 0);
    ctx.zones[i].cal_dry = prefs.getUShort(zoneKey("zDry", i), DEFAULT_CAL_DRY);
    ctx.zones[i].cal_wet = prefs.getUShort(zoneKey("zWet", i), DEFAULT_CAL_WET);
    ctx.zones[i].schedule_fired_today = false;
    ctx.zones[i].window_override = prefs.getUChar(zoneKey("zWinE", i), 0) != 0;
    ctx.zones[i].win_sh = prefs.getUChar(zoneKey("zWinSH", i), 17);
    ctx.zones[i].win_sm = prefs.getUChar(zoneKey("zWinSM", i), 0);
    ctx.zones[i].win_eh = prefs.getUChar(zoneKey("zWinEH", i), 21);
    ctx.zones[i].win_em = prefs.getUChar(zoneKey("zWinEM", i), 0);
  }

  return true;
}

bool SettingsStore::save(const SystemContext &ctx) {
  if (!opened_ || ctx.config == nullptr || ctx.zones == nullptr) {
    return false;
  }

  prefs.putUInt("magic", NVS_MAGIC);
  prefs.putUShort("maxRun", ctx.config->max_run_sec);
  prefs.putUInt("dailyLim", ctx.config->daily_limit_ml);
  prefs.putUShort("ppl", ctx.config->pulses_per_liter);
  prefs.putUChar("dryRun", ctx.config->dry_run_sec);
  prefs.putUChar("zCnt", ctx.config->zone_count);
  prefs.putUChar("wifiEn", ctx.config->wifi_enabled ? 1 : 0);
  prefs.putUChar("aWinEn", ctx.config->auto_window_enabled ? 1 : 0);
  prefs.putUChar("aWinSH", ctx.config->auto_win_sh);
  prefs.putUChar("aWinSM", ctx.config->auto_win_sm);
  prefs.putUChar("aWinEH", ctx.config->auto_win_eh);
  prefs.putUChar("aWinEM", ctx.config->auto_win_em);
  prefs.putString("wifiSSID", ctx.config->wifi_ssid);
  prefs.putString("wifiPass", ctx.config->wifi_pass);

  for (uint8_t i = 0; i < ctx.config->zone_count; ++i) {
    prefs.putString(zoneKey("zN", i), ctx.zones[i].name);
    prefs.putUChar(zoneKey("zML", i), ctx.zones[i].moisture_low);
    prefs.putUChar(zoneKey("zMH", i), ctx.zones[i].moisture_high);
    prefs.putUShort(zoneKey("zVol", i), ctx.zones[i].volume_ml);
    prefs.putUChar(zoneKey("zAuto", i), ctx.zones[i].auto_enabled ? 1 : 0);
    prefs.putUChar(zoneKey("zSchE", i), ctx.zones[i].schedule_enabled ? 1 : 0);
    prefs.putUChar(zoneKey("zSchH", i), ctx.zones[i].schedule_hour);
    prefs.putUChar(zoneKey("zSchM", i), ctx.zones[i].schedule_minute);
    prefs.putUShort(zoneKey("zDry", i), ctx.zones[i].cal_dry);
    prefs.putUShort(zoneKey("zWet", i), ctx.zones[i].cal_wet);
    prefs.putUChar(zoneKey("zWinE", i), ctx.zones[i].window_override ? 1 : 0);
    prefs.putUChar(zoneKey("zWinSH", i), ctx.zones[i].win_sh);
    prefs.putUChar(zoneKey("zWinSM", i), ctx.zones[i].win_sm);
    prefs.putUChar(zoneKey("zWinEH", i), ctx.zones[i].win_eh);
    prefs.putUChar(zoneKey("zWinEM", i), ctx.zones[i].win_em);
  }

  return true;
}
