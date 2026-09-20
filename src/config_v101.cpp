#include "config_manager.h"
#include "camera_registry.h"
#include <cstring>
#include <cstdlib>

namespace {
constexpr const char *KEY_MULTI_CAM = "multi_cam";
constexpr const char *KEY_DYNAMIC_POWER = "dyn_power";
constexpr const char *KEY_ARMED_POWER = "arm_dbm";
constexpr const char *KEY_DISARMED_POWER = "dis_dbm";
constexpr const char *KEY_ARM_BOOST_POWER = "arm_b_dbm";
constexpr const char *KEY_ARM_BOOST_MS = "arm_b_ms";
constexpr const char *KEY_DISARM_BOOST_POWER = "dis_b_dbm";
constexpr const char *KEY_DISARM_BOOST_MS = "dis_b_ms";
constexpr const char *KEY_POWER_MULTI_ONLY = "pwr_multi";
constexpr const char *KEY_DISARM_DELAY_COMPAT = "disarm_delay";
constexpr uint32_t MAX_BOOST_MS = 60000;

int8_t clampPower(long v) {
    if (v < -12) v = -12;
    if (v > 9) v = 9;
    return static_cast<int8_t>(v);
}

uint32_t clampBoostMs(unsigned long v) {
    return v > MAX_BOOST_MS ? MAX_BOOST_MS : static_cast<uint32_t>(v);
}
}

void ConfigManager::loadV101Extras() {
    // A short-lived configurator default wrote 5 instead of 5000 ms. Treat
    // that exact value as the bad default and repair it once on V1.0.2 load.
    if (_cfg.disarmStopDelayMs == 5) {
        _cfg.disarmStopDelayMs = DEFAULT_DISARM_STOP_DELAY_MS;
        _prefs.putUInt(KEY_DISARM_DELAY_COMPAT, _cfg.disarmStopDelayMs);
    }
    _cfg.multiCamSync = _prefs.getBool(KEY_MULTI_CAM, DEFAULT_MULTI_CAM_SYNC);
    _cfg.dynamicTxPower = _prefs.getBool(KEY_DYNAMIC_POWER, DEFAULT_DYNAMIC_TX_POWER);
    _cfg.armedTxPowerDbm = clampPower(_prefs.getInt(KEY_ARMED_POWER, DEFAULT_ARMED_TX_POWER_DBM));
    _cfg.disarmedTxPowerDbm = clampPower(_prefs.getInt(KEY_DISARMED_POWER, DEFAULT_DISARMED_TX_POWER_DBM));
    _cfg.armBoostTxPowerDbm = clampPower(_prefs.getInt(KEY_ARM_BOOST_POWER, DEFAULT_ARM_BOOST_TX_POWER_DBM));
    _cfg.armBoostMs = clampBoostMs(_prefs.getULong(KEY_ARM_BOOST_MS, DEFAULT_ARM_BOOST_MS));
    _cfg.disarmBoostTxPowerDbm = clampPower(_prefs.getInt(KEY_DISARM_BOOST_POWER, DEFAULT_DISARM_BOOST_TX_POWER_DBM));
    _cfg.disarmBoostMs = clampBoostMs(_prefs.getULong(KEY_DISARM_BOOST_MS, DEFAULT_DISARM_BOOST_MS));
    _cfg.powerMultiOnly = _prefs.getBool(KEY_POWER_MULTI_ONLY, DEFAULT_POWER_MULTI_ONLY);
}

void ConfigManager::printV101Extras(Stream &out) {
    out.printf("[cfg] multi_cam       = %s\n", _cfg.multiCamSync ? "true" : "false");
    out.printf("[cfg] dyn_power       = %s\n", _cfg.dynamicTxPower ? "true" : "false");
    out.printf("[cfg] arm_dbm         = %d dBm\n", _cfg.armedTxPowerDbm);
    out.printf("[cfg] dis_dbm         = %d dBm\n", _cfg.disarmedTxPowerDbm);
    out.printf("[cfg] arm_boost_dbm   = %d dBm\n", _cfg.armBoostTxPowerDbm);
    out.printf("[cfg] arm_boost_ms    = %lu ms\n", static_cast<unsigned long>(_cfg.armBoostMs));
    out.printf("[cfg] dis_boost_dbm   = %d dBm\n", _cfg.disarmBoostTxPowerDbm);
    out.printf("[cfg] dis_boost_ms    = %lu ms\n", static_cast<unsigned long>(_cfg.disarmBoostMs));
    out.printf("[cfg] power_multi_only = %s\n", _cfg.powerMultiOnly ? "true" : "false");
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

void ConfigManager::setArmBoostTxPowerDbm(int8_t dbm) {
    _cfg.armBoostTxPowerDbm = clampPower(dbm);
    _prefs.putInt(KEY_ARM_BOOST_POWER, _cfg.armBoostTxPowerDbm);
}

void ConfigManager::setArmBoostMs(uint32_t ms) {
    _cfg.armBoostMs = clampBoostMs(ms);
    _prefs.putULong(KEY_ARM_BOOST_MS, _cfg.armBoostMs);
}

void ConfigManager::setDisarmBoostTxPowerDbm(int8_t dbm) {
    _cfg.disarmBoostTxPowerDbm = clampPower(dbm);
    _prefs.putInt(KEY_DISARM_BOOST_POWER, _cfg.disarmBoostTxPowerDbm);
}

void ConfigManager::setDisarmBoostMs(uint32_t ms) {
    _cfg.disarmBoostMs = clampBoostMs(ms);
    _prefs.putULong(KEY_DISARM_BOOST_MS, _cfg.disarmBoostMs);
}

void ConfigManager::setPowerMultiOnly(bool v) {
    _cfg.powerMultiOnly = v;
    _prefs.putBool(KEY_POWER_MULTI_ONLY, v);
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
                _serial->printf("[cfg] multi_cam = %s (saved - reboot to apply)\n", _cfg.multiCamSync ? "true" : "false");
            } else if (strncmp(line, "set dyn_power ", 14) == 0) {
                const char *val = line + 14; while (*val == ' ') val++;
                setDynamicTxPower(strtoul(val, nullptr, 10) != 0);
                _serial->printf("[cfg] dyn_power = %s (saved)\n", _cfg.dynamicTxPower ? "true" : "false");
            } else if (strncmp(line, "set arm_dbm ", 12) == 0) {
                const char *val = line + 12; while (*val == ' ') val++;
                setArmedTxPowerDbm((int8_t)strtol(val, nullptr, 10));
                _serial->printf("[cfg] arm_dbm = %d dBm (saved)\n", _cfg.armedTxPowerDbm);
            } else if (strncmp(line, "set dis_dbm ", 12) == 0 || strncmp(line, "set idle_dbm ", 13) == 0) {
                const char *val = line + (line[4] == 'd' ? 12 : 13); while (*val == ' ') val++;
                setDisarmedTxPowerDbm((int8_t)strtol(val, nullptr, 10));
                _serial->printf("[cfg] dis_dbm = %d dBm (saved)\n", _cfg.disarmedTxPowerDbm);
            } else if (strncmp(line, "set arm_boost_dbm ", 18) == 0) {
                const char *val = line + 18; while (*val == ' ') val++;
                setArmBoostTxPowerDbm((int8_t)strtol(val, nullptr, 10));
                _serial->printf("[cfg] arm_boost_dbm = %d dBm (saved)\n", _cfg.armBoostTxPowerDbm);
            } else if (strncmp(line, "set arm_boost_ms ", 17) == 0) {
                const char *val = line + 17; while (*val == ' ') val++;
                setArmBoostMs(strtoul(val, nullptr, 10));
                _serial->printf("[cfg] arm_boost_ms = %lu ms (saved)\n", static_cast<unsigned long>(_cfg.armBoostMs));
            } else if (strncmp(line, "set dis_boost_dbm ", 18) == 0) {
                const char *val = line + 18; while (*val == ' ') val++;
                setDisarmBoostTxPowerDbm((int8_t)strtol(val, nullptr, 10));
                _serial->printf("[cfg] dis_boost_dbm = %d dBm (saved)\n", _cfg.disarmBoostTxPowerDbm);
            } else if (strncmp(line, "set dis_boost_ms ", 17) == 0) {
                const char *val = line + 17; while (*val == ' ') val++;
                setDisarmBoostMs(strtoul(val, nullptr, 10));
                _serial->printf("[cfg] dis_boost_ms = %lu ms (saved)\n", static_cast<unsigned long>(_cfg.disarmBoostMs));
            } else if (strncmp(line, "set power_multi_only ", 21) == 0) {
                const char *val = line + 21; while (*val == ' ') val++;
                setPowerMultiOnly(strtoul(val, nullptr, 10) != 0);
                _serial->printf("[cfg] power_multi_only = %s (saved)\n", _cfg.powerMultiOnly ? "true" : "false");
            } else if (strncmp(line, "cameras label ", 14) == 0) {
                const char *arg = line + 14;
                while (*arg == ' ') arg++;
                char *end = nullptr;
                const unsigned long rawIdx = strtoul(arg, &end, 10);
                while (end && *end == ' ') end++;
                if (!_registry || rawIdx >= _registry->count()) {
                    _serial->printf("[reg] No camera at index %lu\n", rawIdx);
                } else {
                    const char *labelText = (end && *end) ? end : "";
                    if (strcmp(labelText, "-") == 0) labelText = "";
                    _registry->setLabel((uint8_t)rawIdx, labelText);
                    _serial->printf("[reg] Camera %lu -> Cam %u label=\"%s\" (saved)\n",
                                    rawIdx,
                                    _registry->cameraNumber((uint8_t)rawIdx),
                                    _registry->label((uint8_t)rawIdx));
                }
            } else if (strcmp(line, "show") == 0) {
                processCommand(line, *_serial);
                printV101Extras(*_serial);
            } else if (strcmp(line, "help") == 0) {
                processCommand(line, *_serial);
                _serial->println("  set multi_cam <0|1>        - experimental Multi Cam mode; scans/controls all supported ready cameras (reboot required)");
                _serial->println("  set dyn_power <0|1>        - enable experimental BLE power phase control");
                _serial->println("  set dis_dbm <-12..9>       - idle/disarmed BLE power (legacy key; default +9 dBm)");
                _serial->println("  set idle_dbm <-12..9>      - alias for dis_dbm");
                _serial->println("  set arm_boost_dbm <-12..9> - BLE power immediately after arm");
                _serial->println("  set arm_boost_ms <0..60000> - arm boost duration; 0 disables boost");
                _serial->println("  set arm_dbm <-12..9>       - BLE power after arm boost (default -12 dBm)");
                _serial->println("  set dis_boost_dbm <-12..9> - BLE power immediately after disarm");
                _serial->println("  set dis_boost_ms <0..60000> - disarm boost duration; 0 disables boost");
                _serial->println("  set power_multi_only <0|1> - only activate profile after >1 cameras are detected; latches until reboot");
                _serial->println("  cameras label <idx> <name> - set a persistent 1-7 char OSD camera label; use '-' to clear");
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
