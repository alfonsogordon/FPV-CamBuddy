#include "msp_serial.h"
#include "config.h"
#include <cstring>
#include <cstdio>

void MSPSerial::begin(HardwareSerial &serial) {
    _serial = &serial;
}

// ─── update() — call from loop() ─────────────────────────────────────────────

void MSPSerial::update() {
    if (millis() - _lastPollMs >= 100) {
        _lastPollMs = millis();
        sendRequest(MSP_STATUS);
        if (_auxChannel > 0) sendRequest(MSP_RC);
    }
    while (_serial->available())
        feedByte(static_cast<uint8_t>(_serial->read()));
}

// ─── TX: request ─────────────────────────────────────────────────────────────

void MSPSerial::sendRequest(uint16_t cmd) {
    constexpr uint8_t FLAG    = 0x00;
    const     uint8_t cmdLo   = cmd & 0xFF;
    const     uint8_t cmdHi   = cmd >> 8;
    constexpr uint8_t sizeLo  = 0x00;
    constexpr uint8_t sizeHi  = 0x00;

    uint8_t crc = 0;
    crc = crc8DvbS2(crc, FLAG);
    crc = crc8DvbS2(crc, cmdLo);
    crc = crc8DvbS2(crc, cmdHi);
    crc = crc8DvbS2(crc, sizeLo);
    crc = crc8DvbS2(crc, sizeHi);

    _serial->write('$');
    _serial->write('X');
    _serial->write('<');
    _serial->write(FLAG);
    _serial->write(cmdLo);
    _serial->write(cmdHi);
    _serial->write(sizeLo);
    _serial->write(sizeHi);
    _serial->write(crc);
}

// ─── RX parser ───────────────────────────────────────────────────────────────

void MSPSerial::feedByte(uint8_t b) {
    switch (_rxState) {
    case RxState::IDLE:
        if (b == '$') _rxState = RxState::HDR_X;
        break;
    case RxState::HDR_X:
        _rxState = (b == 'X') ? RxState::HDR_DIR : RxState::IDLE;
        break;
    case RxState::HDR_DIR:
        _rxState = (b == '>') ? RxState::FLAG : RxState::IDLE;
        break;
    case RxState::FLAG:
        _rxCrc   = crc8DvbS2(0, b);
        _rxState = RxState::CMD_LO;
        break;
    case RxState::CMD_LO:
        _rxCmd   = b;
        _rxCrc   = crc8DvbS2(_rxCrc, b);
        _rxState = RxState::CMD_HI;
        break;
    case RxState::CMD_HI:
        _rxCmd  |= static_cast<uint16_t>(b) << 8;
        _rxCrc   = crc8DvbS2(_rxCrc, b);
        _rxState = RxState::SZ_LO;
        break;
    case RxState::SZ_LO:
        _rxSize  = b;
        _rxCrc   = crc8DvbS2(_rxCrc, b);
        _rxState = RxState::SZ_HI;
        break;
    case RxState::SZ_HI:
        _rxSize |= static_cast<uint16_t>(b) << 8;
        _rxCrc   = crc8DvbS2(_rxCrc, b);
        _rxPos   = 0;
        _rxState = (_rxSize == 0) ? RxState::CRC : RxState::PAYLOAD;
        break;
    case RxState::PAYLOAD:
        if (_rxPos < RX_BUF_SIZE) _rxBuf[_rxPos] = b;
        _rxCrc = crc8DvbS2(_rxCrc, b);
        if (++_rxPos >= _rxSize) _rxState = RxState::CRC;
        break;
    case RxState::CRC:
        if (b == _rxCrc) processResponse();
        _rxState = RxState::IDLE;
        break;
    }
}

void MSPSerial::processResponse() {
    switch (_rxCmd) {
        case MSP_STATUS: handleStatusResponse(); break;
        case MSP_RC:     handleRcResponse();     break;
    }
}

void MSPSerial::handleStatusResponse() {
    // MSP_STATUS payload layout:
    //   [0-1]  cycleTime       uint16
    //   [2-3]  i2cErrorCount   uint16
    //   [4-5]  sensorStatus    uint16
    //   [6-9]  flightModeFlags uint32  ← bit 0 = ARM box active
    if (_rxSize < 10) return;
    uint32_t flags = 0;
    memcpy(&flags, _rxBuf + 6, sizeof(flags));
    const bool armed = (flags & 0x01) != 0;
    if (armed) _hasArmedSinceBoot = true;
    if (armed != _armed) {
        _armed = armed;
        if (_armCb) _armCb(_armed);
    }
}

void MSPSerial::handleRcResponse() {
    // MSP_RC payload: N × uint16 LE — Roll, Pitch, Yaw, Throttle, AUX1, AUX2, …
    // AUX channel N maps to RC index (3 + N), i.e. AUX1 → index 4.
    if (_auxChannel == 0) return;
    const uint8_t rcIdx = 3 + _auxChannel;  // AUX1=4, AUX2=5, …
    if (_rxSize < static_cast<uint16_t>(rcIdx + 1) * 2) return;
    uint16_t value = 0;
    memcpy(&value, _rxBuf + rcIdx * 2, sizeof(value));
    const bool high = (value > 1500);
    if (high != _auxHigh) {
        _auxHigh = high;
        if (_auxSwitchCb) _auxSwitchCb(high);
    }
}

void MSPSerial::setAuxChannel(uint8_t channel) {
    if (channel != _auxChannel) {
        _auxChannel = channel;
        _auxHigh    = false;  // reset edge state on channel change
    }
}

// ─── MSP v2 framing ──────────────────────────────────────────────────────────
// Wire format: '$'  'X'  dir  flag(1)  cmd(2 LE)  size(2 LE)  payload(size)  crc8(1)
// CRC8/DVB-S2 covers: flag + cmd[0] + cmd[1] + size[0] + size[1] + payload
void MSPSerial::sendFrame(uint16_t cmd, const uint8_t *payload, uint16_t length, char dir) {
    constexpr uint8_t FLAG = 0x00;

    const uint8_t cmdLo  = cmd    & 0xFF;
    const uint8_t cmdHi  = cmd    >> 8;
    const uint8_t sizeLo = length & 0xFF;
    const uint8_t sizeHi = length >> 8;

    // CRC over flag + cmd(2) + size(2) + payload
    uint8_t crc = 0;
    crc = crc8DvbS2(crc, FLAG);
    crc = crc8DvbS2(crc, cmdLo);
    crc = crc8DvbS2(crc, cmdHi);
    crc = crc8DvbS2(crc, sizeLo);
    crc = crc8DvbS2(crc, sizeHi);
    crc = crc8DvbS2Buf(crc, payload, length);

    // Write atomically — UART TX FIFO on ESP32 is 128 bytes, well above our 56-byte frame
    _serial->write('$');
    _serial->write('X');
    _serial->write(static_cast<uint8_t>(dir));
    _serial->write(FLAG);
    _serial->write(cmdLo);
    _serial->write(cmdHi);
    _serial->write(sizeLo);
    _serial->write(sizeHi);
    _serial->write(payload, length);
    _serial->write(crc);
}

void MSPSerial::sendCustomText(uint8_t textType, const char *text) {
    const uint8_t textLen = static_cast<uint8_t>(strlen(text));
    uint8_t buf[1 + 1 + 16];   // type(1) + length(1) + up to 16 chars
    buf[0] = textType;
    buf[1] = textLen;
    memcpy(buf + 2, text, textLen);
    sendFrame(MSP2_SET_TEXT, buf, 2 + textLen, '<');
}

// ─── OSD template engine ─────────────────────────────────────────────────────

static void resolveToken(const char *tok, const CameraData &data,
                         char *val, size_t valLen,
                         bool fpvErrorEnabled, const char *fpvErrorText,
                         bool fpvReadyEnabled, const char *fpvReadyText,
                         bool fpvRecordingEnabled, const char *fpvRecordingText,
                         bool fpvRecFlash, bool fpvLowBatteryEnabled,
                         uint8_t fpvLowBatteryPct, bool fpvLowBatteryReadyFlash,
                         bool fpvLowBatteryRecText, const char *fpvLowBatteryText,
                         const char *stateOverride) {
    static const char * const res_labels[] = {
        "480p", "720p", "1080", "1440", "2.7K", "4K", "4KW", "5.1K", "5.3K", "8K",
    };
    static const char * const fps_labels[] = {
        "24", "25", "30", "48", "50", "60", "90", "100", "120", "200", "240", "400",
    };
    static const char * const eis_labels[] = {
        "OFF", "RS", "HS", "RS+", "HB", "LOW", "HI", "BST", "ABS", "STD",
    };

    val[0] = '\0';

    const bool cameraError = !data.valid || data.temp_over != 0;
    const bool cameraRecording = data.valid && data.temp_over == 0 && data.recording;
    const bool cameraReady = data.valid && data.temp_over == 0 && !data.recording;
    const bool flashOn = (((millis() / 500UL) & 1U) == 0U);
    const unsigned pct = data.percent > 100 ? 100 : data.percent;
    const bool lowBatt = fpvLowBatteryEnabled && data.valid && data.has_battery && pct <= fpvLowBatteryPct;

    if (strcmp(tok, "state") == 0) {
        // In state-aware Craft Name mode the caller can force the already-
        // evaluated display state. This keeps {state} consistent with strict
        // readiness gating (e.g. low battery / low record time / media fault
        // must resolve to ERR rather than falling back to connected=RDY).
        if (stateOverride && stateOverride[0] != '\0') snprintf(val, valLen, "%s", stateOverride);
        else if (!data.valid || !data.has_recording || (data.has_temperature && data.temp_over >= 2)) snprintf(val, valLen, "ERR");
        else if (data.recording) snprintf(val, valLen, "REC");
        else snprintf(val, valLen, "RDY");
    } else if (strcmp(tok, "bat") == 0) {
        if (!data.valid || !data.has_battery) snprintf(val, valLen, "");
        else snprintf(val, valLen, "%u%%", pct);
    } else if (strcmp(tok, "batn") == 0) {
        if (!data.valid || !data.has_battery) snprintf(val, valLen, "");
        else snprintf(val, valLen, "%u", pct);
    } else if (strcmp(tok, "rect") == 0) {
        if (!data.valid || !data.has_remain_time) {
            snprintf(val, valLen, "");
        } else {
            uint32_t mins = (data.remain_time + 59UL) / 60UL;
            if (mins > 999UL) mins = 999UL;
            snprintf(val, valLen, "%lum", (unsigned long)mins);
        }
    } else if (strcmp(tok, "rec") == 0) {
        if (!data.valid) {
            snprintf(val, valLen, "NC");
        } else if (data.temp_over >= 2) {
            snprintf(val, valLen, "CAM:HOT");
        } else if (data.recording) {
            snprintf(val, valLen, "REC");
        } else {
            snprintf(val, valLen, "IDLE");
        }
    } else if (strcmp(tok, "batt") == 0) {
        if (data.valid && data.has_battery) snprintf(val, valLen, "B:%u", pct);
    } else if (strcmp(tok, "rectf") == 0) {
        if (data.valid && data.has_remain_time) {
            uint32_t mins = (data.remain_time + 59UL) / 60UL;
            if (mins > 999) mins = 999;
            snprintf(val, valLen, "T:%lum", (unsigned long)mins);
        }
    } else if (strcmp(tok, "recdur") == 0) {
        if (data.valid && data.temp_over < 2 && data.recording) {
            uint16_t mins = data.record_time / 60;
            uint8_t secs = data.record_time % 60;
            snprintf(val, valLen, "%3u:%02u", mins, secs);
        }
    } else if (strcmp(tok, "fpv") == 0) {
        // Legacy alias retained for old saved templates. New public configs use
        // state-aware per-state templates instead.
        if (cameraError) snprintf(val, valLen, "ERR");
        else if (cameraRecording) {
            if (!fpvRecFlash || flashOn) snprintf(val, valLen, "REC");
            else if (lowBatt && fpvLowBatteryRecText) snprintf(val, valLen, "%s", fpvLowBatteryText);
            else snprintf(val, valLen, "                ");
        } else {
            char tmp[17] = {};
            const bool hideBat = lowBatt && fpvLowBatteryReadyFlash && !flashOn;
            uint32_t mins = (data.remain_time + 59UL) / 60UL;
            if (mins > 999UL) mins = 999UL;
            if (hideBat) snprintf(tmp, sizeof(tmp), "RDY B:    T:%lum", (unsigned long)mins);
            else snprintf(tmp, sizeof(tmp), "RDY B:%u T:%lum", pct, (unsigned long)mins);
            snprintf(val, valLen, "%s", tmp);
        }
    } else if (strcmp(tok, "mode") == 0) {
        if (!data.valid) { snprintf(val, valLen, "---"); return; }
        // 3-char left-padded to preserve OSD column alignment
        switch (data.camera_mode) {
            case 0x00: snprintf(val, valLen, "SLO"); break;
            case 0x01: snprintf(val, valLen, "VID"); break;
            case 0x02: snprintf(val, valLen, "TL "); break;
            case 0x05: snprintf(val, valLen, "PHO"); break;
            case 0x0A: snprintf(val, valLen, "HYP"); break;
            default:   snprintf(val, valLen, "---"); break;
        }
    } else if (strcmp(tok, "res") == 0) {
        if (!data.valid) { snprintf(val, valLen, "---"); return; }
        const char *r = (data.resolution < sizeof(res_labels) / sizeof(res_labels[0]))
                      ? res_labels[data.resolution] : "---";
        snprintf(val, valLen, "%s", r);
    } else if (strcmp(tok, "fps") == 0) {
        if (!data.valid) { snprintf(val, valLen, "--"); return; }
        const char *f = (data.fps_idx < sizeof(fps_labels) / sizeof(fps_labels[0]))
                      ? fps_labels[data.fps_idx] : "--";
        snprintf(val, valLen, "%s", f);
    } else if (strcmp(tok, "eis") == 0) {
        if (!data.valid) { snprintf(val, valLen, "---"); return; }
        const char *e = (data.eis_mode < sizeof(eis_labels) / sizeof(eis_labels[0]))
                      ? eis_labels[data.eis_mode] : "---";
        snprintf(val, valLen, "%s", e);
    } else if (strcmp(tok, "rleft") == 0) {
        if (!data.valid || !data.has_remain_time) {
            snprintf(val, valLen, "");
        } else {
            uint32_t mins = data.remain_time / 60;
            if (mins >= 60) {
                uint32_t hrs = mins / 60;
                snprintf(val, valLen, "%luh%02lum",
                         (unsigned long)hrs, (unsigned long)(mins % 60));
            } else {
                snprintf(val, valLen, "%lum", (unsigned long)mins);
            }
        }
    } else if (strcmp(tok, "rcap") == 0) {
        if (!data.valid || data.remain_cap_mb == 0) {
            snprintf(val, valLen, "---");
        } else if (data.remain_cap_mb >= 1000) {
            uint32_t gb_int  = data.remain_cap_mb / 1000;
            uint32_t gb_frac = (data.remain_cap_mb % 1000) / 100;
            snprintf(val, valLen, "%lu.%luG",
                     (unsigned long)gb_int, (unsigned long)gb_frac);
        } else {
            snprintf(val, valLen, "%luM", (unsigned long)data.remain_cap_mb);
        }
    }
}

static void expandTemplate(const char *tpl, const CameraData &data,
                           char *out, size_t outLen,
                           bool fpvErrorEnabled, const char *fpvErrorText,
                           bool fpvReadyEnabled, const char *fpvReadyText,
                           bool fpvRecordingEnabled, const char *fpvRecordingText,
                           bool fpvRecFlash, bool fpvLowBatteryEnabled,
                           uint8_t fpvLowBatteryPct, bool fpvLowBatteryReadyFlash,
                           bool fpvLowBatteryRecText, const char *fpvLowBatteryText,
                           const char *stateOverride = nullptr) {
    size_t outPos = 0;
    while (*tpl && outPos < outLen - 1) {
        if (*tpl == '{') {
            const char *end = strchr(tpl + 1, '}');
            if (!end) { out[outPos++] = *tpl++; continue; }
            char tok[16] = {};
            size_t tLen = static_cast<size_t>(end - (tpl + 1));
            if (tLen < sizeof(tok)) {
                memcpy(tok, tpl + 1, tLen);
                tok[tLen] = '\0';
                char val[17] = {};
                resolveToken(tok, data, val, sizeof(val),
                             fpvErrorEnabled, fpvErrorText,
                             fpvReadyEnabled, fpvReadyText,
                             fpvRecordingEnabled, fpvRecordingText, fpvRecFlash,
                             fpvLowBatteryEnabled, fpvLowBatteryPct, fpvLowBatteryReadyFlash,
                             fpvLowBatteryRecText, fpvLowBatteryText, stateOverride);
                size_t vLen = strlen(val);
                size_t copy = (vLen < outLen - 1 - outPos) ? vLen : (outLen - 1 - outPos);
                memcpy(out + outPos, val, copy);
                outPos += copy;
            }
            tpl = end + 1;
        } else {
            out[outPos++] = *tpl++;
        }
    }
    out[outPos] = '\0';

    // Capability-dependent tokens may be blank. Collapse resulting whitespace
    // so a single template remains tidy across camera families with different
    // telemetry capabilities.
    size_t r = 0, w = 0;
    bool pendingSpace = false;
    while (out[r] == ' ') r++;
    for (; out[r] != '\0'; r++) {
        if (out[r] == ' ') {
            pendingSpace = (w > 0);
        } else {
            if (pendingSpace && w < outLen - 1) out[w++] = ' ';
            pendingSpace = false;
            if (w < outLen - 1) out[w++] = out[r];
        }
    }
    out[w] = '\0';
}

// ─── OSD send functions ───────────────────────────────────────────────────────

void MSPSerial::sendCustomOSD1(const CameraData &data, const char *tpl) { sendCustomOSD(MSP_TEXT_CUSTOM_1, data, tpl); }
void MSPSerial::sendCustomOSD2(const CameraData &data, const char *tpl) { sendCustomOSD(MSP_TEXT_CUSTOM_2, data, tpl); }
void MSPSerial::sendCustomOSD3(const CameraData &data, const char *tpl) { sendCustomOSD(MSP_TEXT_CUSTOM_3, data, tpl); }
void MSPSerial::sendCustomOSD4(const CameraData &data, const char *tpl) { sendCustomOSD(MSP_TEXT_CUSTOM_4, data, tpl); }

void MSPSerial::sendPilotName(const CameraData &data, const char *tpl) { sendCustomOSD(MSP_TEXT_PILOT_NAME, data, tpl); }
void MSPSerial::sendCraftName(const CameraData &data, const char *tpl) {
    if (!_fpvStateMode) {
        sendCustomOSD(MSP_TEXT_CRAFT_NAME, data, tpl);
        return;
    }

    constexpr const char *BLANK = "                ";
    const bool flashOn = (((millis() / 500UL) & 1U) == 0U);
    const bool valid = data.valid;
    const bool recording = valid && data.has_recording && data.recording;
    const bool criticalHot = valid && data.has_temperature && data.temp_over >= 2;
    const bool mediaNotReady = valid && data.has_media_ready && !data.media_ready;

    // Evaluate enabled warning thresholds before choosing the base state.
    // RDY now has a strict meaning: connected AND all enabled readiness checks
    // are healthy. Crossing any enabled threshold while not recording makes the
    // base state ERR, even though BLE remains connected.
    const unsigned pct = data.percent > 100 ? 100 : data.percent;
    const bool lowBatt = _fpvLowBatteryEnabled && valid && data.has_battery &&
                         pct <= _fpvLowBatteryPct;
    const bool lowRec = _fpvLowRecEnabled && valid && data.has_remain_time &&
                        data.remain_time <= ((uint32_t)_fpvLowRecMinutes * 60UL);
    const bool hot = _fpvHotEnabled && valid && data.has_temperature && data.temp_over != 0;
    const bool readinessWarning = !recording && (lowBatt || lowRec || hot);

    const char *stateTpl = nullptr;
    const char *forcedState = nullptr;
    if (!valid || !data.has_recording || criticalHot || mediaNotReady || readinessWarning) {
        forcedState = "ERR";
        if (_fpvErrorEnabled) stateTpl = _fpvErrorText;
    } else if (recording) {
        forcedState = "REC";
        if (_fpvRecordingEnabled) stateTpl = _fpvRecordingText;
    } else {
        forcedState = "RDY";
        if (_fpvReadyEnabled) stateTpl = _fpvReadyText;
    }

    if (!stateTpl) {
        sendCustomText(MSP_TEXT_CRAFT_NAME, BLANK);
        return;
    }

    // Build the active warning list. These warnings explain why a connected
    // camera is in ERR while idle, and occupy REC's alternate phase while
    // recording. Missing/unsupported telemetry never creates a false warning.
    const char *warnings[3] = {nullptr, nullptr, nullptr};
    uint8_t warningCount = 0;

    const bool warningContextRecording = recording;
    const bool warningContextReady = !recording;

    if (hot && ((warningContextRecording && _fpvHotRecording) ||
                (warningContextReady && _fpvHotReady))) {
        warnings[warningCount++] = _fpvHotText;
    }
    if (lowBatt && ((warningContextRecording && _fpvLowBatteryRecText) ||
                    (warningContextReady && _fpvLowBatteryReadyFlash))) {
        warnings[warningCount++] = _fpvLowBatteryText;
    }
    if (lowRec && ((warningContextRecording && _fpvLowRecRecording) ||
                   (warningContextReady && _fpvLowRecReady))) {
        warnings[warningCount++] = _fpvLowRecText;
    }

    // Phase A is the base state (RDY/REC/ERR). Phase B is the next active
    // warning, or (while recording with flash enabled) a true blank. If several
    // warnings are active, rotate one warning per second so none are hidden.
    const bool reminderDue = _fpvPreArmText[0] != '\0' &&
                             (!_fpvPreArmEnabled || !_hasArmedSinceBoot) && !recording &&
                             forcedState && strcmp(forcedState, "RDY") == 0 && warningCount == 0 &&
                             ((millis() % _fpvPreArmIntervalMs) < _fpvPreArmShowMs);
    if (reminderDue) {
        sendCustomText(MSP_TEXT_CRAFT_NAME, _fpvPreArmText);
        return;
    }

    const bool alternate = warningCount > 0 || (recording && _fpvRecFlash);
    if (alternate && !flashOn) {
        if (warningCount > 0) {
            const uint8_t idx = (uint8_t)((millis() / 1000UL) % warningCount);
            sendCustomText(MSP_TEXT_CRAFT_NAME, warnings[idx]);
        } else {
            sendCustomText(MSP_TEXT_CRAFT_NAME, BLANK);
        }
        return;
    }

    sendCustomOSD(MSP_TEXT_CRAFT_NAME, data, stateTpl, forcedState);
}

void MSPSerial::sendCustomOSD(uint8_t textType, const CameraData &data, const char *tpl, const char *stateOverride) {
    char text[17] = {};
    expandTemplate(tpl, data, text, sizeof(text),
                   _fpvErrorEnabled, _fpvErrorText,
                   _fpvReadyEnabled, _fpvReadyText,
                   _fpvRecordingEnabled, _fpvRecordingText, _fpvRecFlash,
                   _fpvLowBatteryEnabled, _fpvLowBatteryPct, _fpvLowBatteryReadyFlash,
                   _fpvLowBatteryRecText, _fpvLowBatteryText, stateOverride);
    sendCustomText(textType, text);
}

void MSPSerial::sendCameraStatus(const CameraData &data) {
    MSP2CameraPayload p{};
    p.percent       = data.percent;
    p.camera_mode   = data.camera_mode;
    p.recording     = data.recording ? 1 : 0;
    p.temp_over     = data.temp_over;
    p.eis_mode      = data.eis_mode;
    p.record_time   = data.record_time;
    p.remain_cap_mb = data.remain_cap_mb;
    p.remain_time   = data.remain_time;

    sendFrame(MSP2_CAMERA_BATTERY,
              reinterpret_cast<const uint8_t *>(&p),
              sizeof(p));
}

// ─── CRC8/DVB-S2 (polynomial 0xD5) ──────────────────────────────────────────

uint8_t MSPSerial::crc8DvbS2(uint8_t crc, uint8_t byte) {
    crc ^= byte;
    for (int i = 0; i < 8; i++) {
        crc = (crc & 0x80) ? (crc << 1) ^ 0xD5 : (crc << 1);
    }
    return crc;
}

uint8_t MSPSerial::crc8DvbS2Buf(uint8_t crc,
                                  const uint8_t *buf, uint16_t length) {
    for (uint16_t i = 0; i < length; i++) {
        crc = crc8DvbS2(crc, buf[i]);
    }
    return crc;
}
