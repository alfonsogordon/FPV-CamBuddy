#include "config_manager.h"
#include <cstring>
#include <cstdlib>

namespace {
constexpr const char *KEY_MULTI_CAM = "multi_cam";
}

void ConfigManager::loadV101Extras() {
    _cfg.multiCamSync = _prefs.getBool(KEY_MULTI_CAM, DEFAULT_MULTI_CAM_SYNC);
}

void ConfigManager::printV101Extras(Stream &out) {
    out.printf("[cfg] multi_cam       = %s\n", _cfg.multiCamSync ? "true" : "false");
}

void ConfigManager::setMultiCamSync(bool v) {
    _cfg.multiCamSync = v;
    _prefs.putBool(KEY_MULTI_CAM, v);
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
                const char *val = line + 14;
                while (*val == ' ') val++;
                setMultiCamSync(strtoul(val, nullptr, 10) != 0);
                _serial->printf("[cfg] multi_cam = %s (saved — reboot to apply)\n",
                                _cfg.multiCamSync ? "true" : "false");
            } else if (strcmp(line, "show") == 0) {
                processCommand(line, *_serial);
                printV101Extras(*_serial);
            } else if (strcmp(line, "help") == 0) {
                processCommand(line, *_serial);
                _serial->println("  set multi_cam <0|1>        - 1=connect/control up to two GoPros; 0=proven single-GoPro backend (reboot required)");
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
