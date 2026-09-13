#include "config_manager.h"
#include <cstring>
#include <cstdlib>

namespace {
constexpr const char *KEY_MULTI_CAM = "multi_cam";
constexpr const char *KEY_DYNAMIC_POWER = "dyn_power";
constexpr const char *KEY_ARMED_POWER = "arm_dbm";
constexpr const char *KEY_DISARMED_POWER = "dis_dbm";

int8_t clampPower(long v) {
    if (v < -12) v = -12;
    if (v > 9) v = 9;
    return static_cast<int8_t>(v);
}
}

void ConfigManager::loadV101Extras() {
    _cfg.multiCamSync = _prefs.getBool(KEY_MULTI_CAM, DEFAULT_MULTI_CAM_SYNC);
    _cfg.dynamicTxPower = _prefs.getBool(KEY_DYNAMIC_POWER, DEFAULT_DYNAMIC_TX_POWER);
    _cfg.armedTxPowerDbm = clampPower(_prefs.getInt(KEY_ARMED_POWER, DEFAULT_ARMED_TX_POWER_DBM));
    _cfg.disarmedTxPowerDbm = clampPower(_prefs.getInt(KEY_DISARMED_POWER, DEFAULT_DISARMED_TX_POWER_DBM));
}

void ConfigManager::printV101Extras(Stream &out) {
    out.printf("[cfg] multi_cam       = %s\n", _cfg.multiCamSync ? "true" : "false");
    out.printf("[cfg] dyn_power       = %s\n", _cfg.dynamicTxPower ? "true" : "false");
    out.printf("[cfg] arm_dbm         = %d dBm\n", _cfg.armedTxPowerDbm);
    out.printf("[cfg] dis_dbm         = %d dBm\n", _cfg.disarmedTxPowerDbm);
}

void ConfigManager::setMultiCamSync(bool v) {
    _cfg.multiCamSync = v;
    _prefs.putBool(KEY_MULTI_CAM, v);
}

void ConfigManager::setDynamicTxPower(bool v) {
    _cfg.dynamicTxPower = v;
    _prefs.putBool(KEY_DYNAMIC_POWER, v);
}

void ConfigManager::setArmedTxPowerDbm(int8_t dbm) {
    _cfg.armedTxPowerDbm = clampPower(dbm);
    _prefs.putInt(KEY_ARMED_POWER, _cfg.armedTxPowerDbm);
}

void ConfigManager::setDisarmedTxPowerDbm(int8_t dbm) {
    _cfg.disarmedTxPowerDbm = clampPower(dbm);
    _prefs.putInt(KEY_DISARMED_POWER, _cfg.disarmedTxPowerDbm);
}

void ConfigManager::updateV101() {
    if (!_serial) return;

    while (_serial->available()) {
        const char c = static_cast<char>(_serial->read());
        if (c == '\r') continue;
        if (c == '\n') {
  _buf[_len] = '\0';
  const char *line = _buf;
  while (*line == ' ') line++;

  if (strncmp(line, "set multi_cam ", 14) == 0) {
      const char *val = line + 14; while (*val == ' ') val++;
      setMultiCamSync(strtoul(val, nullptr, 10) != 0);
      _serial->printf("[cfg] multi_cam = %s (saved — reboot to apply)\n", _cfg.multiCamSync ? "true" : "false");
  } else if (strncmp(line, "set dyn_power ", 14) == 0) {
      const char *val = line + 14; while (*val == ' ') val++;
      setDynamicTxPower(strtoul(val, nullptr, 10) != 0);
      _serial->printf("[cfg] dyn_power = %s (saved — applies on next arm-state change)\n", _cfg.dynamicTxPower ? "true" : "false");
  } else if (strncmp(line, "set arm_dbm ", 12) == 0) {
      const char *val = line + 12; while (*val == ' ') val++;
      setArmedTxPowerDbm((int8_t)strtol(val, nullptr, 10));
      _serial->printf("[cfg] arm_dbm = %d dBm (saved)\n", _cfg.armedTxPowerDbm);
  } else if (strncmp(line, "set dis_dbm ", 12) == 0) {
      const char *val = line + 12; while (*val == ' ') val++;
      setDisarmedTxPowerDbm((int8_t)strtol(val, nullptr, 10));
      _serial->printf("[cfg] dis_dbm = %d dBm (saved)\n", _cfg.disarmedTxPowerDbm);
  } else if (strcmp(line, "show") == 0) {
      processCommand(line, *_serial);
      printV101Extras(*_serial);
  } else if (strcmp(line, "help") == 0) {
      processCommand(line, *_serial);
      _serial->println("  set multi_cam <0|1>        - experimental Multi Cam mode; scans/controls all supported ready cameras (reboot required)");
      _serial->println("  set dyn_power <0|1>        - use separate BLE TX powers for armed and disarmed states");
      _serial->println("  set arm_dbm <-12..9>       - BLE TX power while armed (default -12 dBm)");
      _serial->println("  set dis_dbm <-12..9>       - BLE TX power while disarmed (default +9 dBm)");
  } else if (strcmp(line, "reset") == 0) {
      processCommand(line, *_serial);
      loadV101Extras();
      printV101Extras(*_serial);
  } else {
      processCommand(line, *_serial);
  }
  _len = 0;
        } else if (_len < sizeof(_buf) - 1) {
  _buf[_len++] = c;
        }
    }
}
