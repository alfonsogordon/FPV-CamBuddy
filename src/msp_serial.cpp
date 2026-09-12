#include "msp_serial.h"
#include "config.h"
#include <cstring>
#include <cstdio>

namespace {

constexpr uint8_t TEXT_LIMIT = 16;

uint8_t destinationForTextType(uint8_t textType) {
    switch (textType) {
        case MSP_TEXT_PILOT_NAME: return 1;
        case MSP_TEXT_CRAFT_NAME: return 2;
        case MSP_TEXT_CUSTOM_1:   return 1;
        case MSP_TEXT_CUSTOM_2:   return 2;
        case MSP_TEXT_CUSTOM_3:   return 3;
        case MSP_TEXT_CUSTOM_4:   return 4;
        default:                  return 0;
    }
}

uint8_t prefixedTarget(const char *stored, const char **cleanText) {
    if (cleanText) *cleanText = stored ? stored : "";
    if (!stored) return 1;
    if (stored[0] == '@' && stored[1] >= '1' && stored[1] <= '4' && stored[2] == ':') {
        if (cleanText) *cleanText = stored + 3;
        return static_cast<uint8_t>(stored[1] - '0');
    }
    return 1;
}

bool startsWith(const char *s, const char *prefix) {
    return s && prefix && strncmp(s, prefix, strlen(prefix)) == 0;
}

bool templateHasStatus(const char *tpl) {
    return tpl && (strstr(tpl, "{state}") || strstr(tpl, "{stateonly}"));
}

void blankRecToken(char *text) {
    if (!text) return;
    for (char *p = strstr(text, "REC"); p; p = strstr(p + 3, "REC")) {
        p[0] = ' ';
        p[1] = ' ';
        p[2] = ' ';
    }
}

const char *resolutionLabel(uint8_t v) {
    static const char * const labels[] = {
        "480p", "720p", "1080", "1440", "2.7K", "4K", "4KW", "5.1K", "5.3K", "8K"
    };
    return v < sizeof(labels) / sizeof(labels[0]) ? labels[v] : "---";
}

const char *fpsLabel(uint8_t v) {
    static const char * const labels[] = {
        "24", "25", "30", "48", "50", "60", "90", "100", "120", "200", "240", "400"
    };
    return v < sizeof(labels) / sizeof(labels[0]) ? labels[v] : "--";
}

const char *eisLabel(uint8_t v) {
    static const char * const labels[] = {
        "OFF", "RS", "HS", "RS+", "HB", "LOW", "HI", "BST", "ABS", "STD"
    };
    return v < sizeof(labels) / sizeof(labels[0]) ? labels[v] : "---";
}

const char *modeLabel(uint8_t v) {
    switch (v) {
        case 0x00: return "SLOMO";
        case 0x01: return "VIDEO";
        case 0x02: return "TIMELAPSE";
        case 0x05: return "PHOTO";
        case 0x0A: return "HYPER";
        default:   return "---";
    }
}

void resolveToken(const char *tok, const CameraData &data, const char *state,
                  char *val, size_t valLen) {
    val[0] = '\0';
    const unsigned pct = data.percent > 100 ? 100 : data.percent;

    if (strcmp(tok, "state") == 0 || strcmp(tok, "stateonly") == 0) {
        snprintf(val, valLen, "%s", state ? state : "ERR");
    } else if (strcmp(tok, "batt") == 0) {
        if (data.valid && data.has_battery) snprintf(val, valLen, "B:%u", pct);
    } else if (strcmp(tok, "bat") == 0) {
        if (data.valid && data.has_battery) snprintf(val, valLen, "%u%%", pct);
    } else if (strcmp(tok, "batn") == 0) {
        if (data.valid && data.has_battery) snprintf(val, valLen, "%u", pct);
    } else if (strcmp(tok, "recdur") == 0) {
        if (data.valid && data.has_recording && data.recording) {
            const unsigned mins = data.record_time / 60U;
            const unsigned secs = data.record_time % 60U;
            snprintf(val, valLen, "%02u:%02u", mins, secs);
        } else {
            snprintf(val, valLen, "00:00");
        }
    } else if (strcmp(tok, "mode") == 0) {
        snprintf(val, valLen, "%s", data.valid ? modeLabel(data.camera_mode) : "---");
    } else if (strcmp(tok, "res") == 0) {
        snprintf(val, valLen, "%s", data.valid ? resolutionLabel(data.resolution) : "---");
    } else if (strcmp(tok, "fps") == 0) {
        snprintf(val, valLen, "%s", data.valid ? fpsLabel(data.fps_idx) : "--");
    } else if (strcmp(tok, "eis") == 0) {
        snprintf(val, valLen, "%s", data.valid ? eisLabel(data.eis_mode) : "---");
    } else if (strcmp(tok, "rectf") == 0 || strcmp(tok, "rect") == 0) {
        if (data.valid && data.has_remain_time) {
            uint32_t mins = (data.remain_time + 59UL) / 60UL;
            if (mins > 999UL) mins = 999UL;
            if (strcmp(tok, "rectf") == 0) snprintf(val, valLen, "T:%lum", (unsigned long)mins);
            else snprintf(val, valLen, "%lum", (unsigned long)mins);
        }
    } else if (strcmp(tok, "rleft") == 0) {
        if (data.valid && data.has_remain_time) {
            const uint32_t mins = data.remain_time / 60UL;
            if (mins >= 60UL) snprintf(val, valLen, "%luh%02lum", (unsigned long)(mins / 60UL), (unsigned long)(mins % 60UL));
            else snprintf(val, valLen, "%lum", (unsigned long)mins);
        }
    } else if (strcmp(tok, "rcap") == 0) {
        if (!data.valid || data.remain_cap_mb == 0) {
            snprintf(val, valLen, "---");
        } else if (data.remain_cap_mb >= 1000UL) {
            const uint32_t gb = (data.remain_cap_mb + 999UL) / 1000UL;
            snprintf(val, valLen, "%luGB", (unsigned long)gb);
        } else {
            snprintf(val, valLen, "%luMB", (unsigned long)data.remain_cap_mb);
        }
    } else if (strcmp(tok, "rec") == 0 || strcmp(tok, "fpv") == 0) {
        snprintf(val, valLen, "%s", state ? state : "ERR");
    }
}

void expandTemplate(const char *tpl, const CameraData &data, const char *state,
                    char *out, size_t outLen) {
    if (!out || outLen == 0) return;
    out[0] = '\0';
    if (!tpl) return;

    size_t outPos = 0;
    while (*tpl && outPos < outLen - 1) {
        if (*tpl == '{') {
            const char *end = strchr(tpl + 1, '}');
            if (!end) {
                out[outPos++] = *tpl++;
                continue;
            }
            char tok[16] = {};
            const size_t tLen = static_cast<size_t>(end - (tpl + 1));
            if (tLen < sizeof(tok)) {
                memcpy(tok, tpl + 1, tLen);
                tok[tLen] = '\0';
                char val[20] = {};
                resolveToken(tok, data, state, val, sizeof(val));
                const size_t vLen = strlen(val);
                const size_t room = outLen - 1 - outPos;
                const size_t copy = vLen < room ? vLen : room;
                memcpy(out + outPos, val, copy);
                outPos += copy;
            }
            tpl = end + 1;
        } else {
            out[outPos++] = *tpl++;
        }
    }
    out[outPos] = '\0';

    // Collapse whitespace from unavailable telemetry tokens. REC flashing is
    // applied after this step so its three columns remain reserved and the
    // rest of the OSD line never shifts left/right during the flash cycle.
    size_t r = 0, w = 0;
    bool pendingSpace = false;
    while (out[r] == ' ') ++r;
    for (; out[r] != '\0'; ++r) {
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

} // namespace

void MSPSerial::begin(HardwareSerial &serial) {
    _serial = &serial;
}

void MSPSerial::update() {
    if (millis() - _lastPollMs >= 100) {
        _lastPollMs = millis();
        sendRequest(MSP_STATUS);
        if (_auxChannel > 0) sendRequest(MSP_RC);
    }
    while (_serial->available()) feedByte(static_cast<uint8_t>(_serial->read()));
}

void MSPSerial::sendRequest(uint16_t cmd) {
    constexpr uint8_t FLAG = 0x00;
    const uint8_t cmdLo = cmd & 0xFF;
    const uint8_t cmdHi = cmd >> 8;
    constexpr uint8_t sizeLo = 0x00;
    constexpr uint8_t sizeHi = 0x00;
    uint8_t crc = 0;
    crc = crc8DvbS2(crc, FLAG);
    crc = crc8DvbS2(crc, cmdLo);
    crc = crc8DvbS2(crc, cmdHi);
    crc = crc8DvbS2(crc, sizeLo);
    crc = crc8DvbS2(crc, sizeHi);
    _serial->write('$'); _serial->write('X'); _serial->write('<');
    _serial->write(FLAG); _serial->write(cmdLo); _serial->write(cmdHi);
    _serial->write(sizeLo); _serial->write(sizeHi); _serial->write(crc);
}

void MSPSerial::feedByte(uint8_t b) {
    switch (_rxState) {
        case RxState::IDLE:    if (b == '$') _rxState = RxState::HDR_X; break;
        case RxState::HDR_X:   _rxState = (b == 'X') ? RxState::HDR_DIR : RxState::IDLE; break;
        case RxState::HDR_DIR: _rxState = (b == '>') ? RxState::FLAG : RxState::IDLE; break;
        case RxState::FLAG:    _rxCrc = crc8DvbS2(0, b); _rxState = RxState::CMD_LO; break;
        case RxState::CMD_LO:  _rxCmd = b; _rxCrc = crc8DvbS2(_rxCrc, b); _rxState = RxState::CMD_HI; break;
        case RxState::CMD_HI:  _rxCmd |= static_cast<uint16_t>(b) << 8; _rxCrc = crc8DvbS2(_rxCrc, b); _rxState = RxState::SZ_LO; break;
        case RxState::SZ_LO:   _rxSize = b; _rxCrc = crc8DvbS2(_rxCrc, b); _rxState = RxState::SZ_HI; break;
        case RxState::SZ_HI:
            _rxSize |= static_cast<uint16_t>(b) << 8;
            _rxCrc = crc8DvbS2(_rxCrc, b);
            _rxPos = 0;
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
        case MSP_RC: handleRcResponse(); break;
    }
}

void MSPSerial::handleStatusResponse() {
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
    if (_auxChannel == 0) return;
    const uint8_t rcIdx = 3 + _auxChannel;
    if (_rxSize < static_cast<uint16_t>(rcIdx + 1) * 2) return;
    uint16_t value = 0;
    memcpy(&value, _rxBuf + rcIdx * 2, sizeof(value));
    const bool high = value > 1500;
    if (high != _auxHigh) {
        _auxHigh = high;
        if (_auxSwitchCb) _auxSwitchCb(high);
    }
}

void MSPSerial::setAuxChannel(uint8_t channel) {
    if (channel != _auxChannel) {
        _auxChannel = channel;
        _auxHigh = false;
    }
}

void MSPSerial::sendFrame(uint16_t cmd, const uint8_t *payload, uint16_t length, char dir) {
    constexpr uint8_t FLAG = 0x00;
    const uint8_t cmdLo = cmd & 0xFF;
    const uint8_t cmdHi = cmd >> 8;
    const uint8_t sizeLo = length & 0xFF;
    const uint8_t sizeHi = length >> 8;
    uint8_t crc = 0;
    crc = crc8DvbS2(crc, FLAG);
    crc = crc8DvbS2(crc, cmdLo);
    crc = crc8DvbS2(crc, cmdHi);
    crc = crc8DvbS2(crc, sizeLo);
    crc = crc8DvbS2(crc, sizeHi);
    crc = crc8DvbS2Buf(crc, payload, length);
    _serial->write('$'); _serial->write('X'); _serial->write(static_cast<uint8_t>(dir));
    _serial->write(FLAG); _serial->write(cmdLo); _serial->write(cmdHi);
    _serial->write(sizeLo); _serial->write(sizeHi);
    _serial->write(payload, length); _serial->write(crc);
}

void MSPSerial::sendCustomText(uint8_t textType, const char *text) {
    const size_t rawLen = text ? strlen(text) : 0;
    const uint8_t textLen = static_cast<uint8_t>(rawLen > TEXT_LIMIT ? TEXT_LIMIT : rawLen);
    uint8_t buf[2 + TEXT_LIMIT] = {};
    buf[0] = textType;
    buf[1] = textLen;
    if (textLen) memcpy(buf + 2, text, textLen);
    sendFrame(MSP2_SET_TEXT, buf, 2 + textLen, '<');
}

void MSPSerial::sendCustomOSD1(const CameraData &data, const char *tpl) { sendCustomOSD(MSP_TEXT_CUSTOM_1, data, tpl); }
void MSPSerial::sendCustomOSD2(const CameraData &data, const char *tpl) { sendCustomOSD(MSP_TEXT_CUSTOM_2, data, tpl); }
void MSPSerial::sendCustomOSD3(const CameraData &data, const char *tpl) { sendCustomOSD(MSP_TEXT_CUSTOM_3, data, tpl); }
void MSPSerial::sendCustomOSD4(const CameraData &data, const char *tpl) { sendCustomOSD(MSP_TEXT_CUSTOM_4, data, tpl); }
void MSPSerial::sendPilotName(const CameraData &data, const char *tpl) { sendCustomOSD(MSP_TEXT_PILOT_NAME, data, tpl); }
void MSPSerial::sendCraftName(const CameraData &data, const char *tpl) { sendCustomOSD(MSP_TEXT_CRAFT_NAME, data, tpl); }

void MSPSerial::sendCustomOSDWarnings(uint8_t target, const CameraData &data, const char *tpl) {
    uint8_t textType = MSP_TEXT_CUSTOM_1;
    if (target == 2) textType = MSP_TEXT_CUSTOM_2;
    else if (target == 3) textType = MSP_TEXT_CUSTOM_3;
    else if (target == 4) textType = MSP_TEXT_CUSTOM_4;
    sendCustomOSD(textType, data, tpl);
}

void MSPSerial::sendCustomOSD(uint8_t textType, const CameraData &data, const char *tpl, const char *stateOverride) {
    (void)stateOverride;
    const uint8_t destination = destinationForTextType(textType);

    const bool criticalHot = data.valid && data.has_temperature && data.temp_over >= 2;
    const bool mediaNotReady = data.valid && data.has_media_ready && !data.media_ready;
    const bool cameraError = !data.valid || !data.has_recording || criticalHot || mediaNotReady;
    const bool cameraRecording = data.valid && data.has_recording && data.recording;
    const bool recOnlyWhenArmed = startsWith(_fpvRecordingText, "@ARM:");

    const char *state = "RDY";
    if (cameraError) state = "ERR";
    else if (cameraRecording) state = "REC";

    const unsigned pct = data.percent > 100 ? 100 : data.percent;
    const bool lowBatt = _fpvLowBatteryEnabled && data.valid && data.has_battery && pct <= _fpvLowBatteryPct;
    const bool lowRec = _fpvLowRecEnabled && data.valid && data.has_remain_time &&
                        data.remain_time <= static_cast<uint32_t>(_fpvLowRecMinutes) * 60UL;
    const bool hot = _fpvHotEnabled && data.valid && data.has_temperature && data.temp_over != 0;

    const char *warnings[3] = {nullptr, nullptr, nullptr};
    uint8_t warningCount = 0;
    if (lowBatt && _fpvLowBatteryText[0]) warnings[warningCount++] = _fpvLowBatteryText;
    if (lowRec && _fpvLowRecText[0]) warnings[warningCount++] = _fpvLowRecText;
    if (hot && _fpvHotText[0]) warnings[warningCount++] = _fpvHotText;

    const bool warningHere = warningCount > 0 && destination == _fpvWarningTarget;
    const bool warningPhase = ((millis() / 1000UL) & 1U) != 0U;
    if (warningHere && warningPhase) {
        const uint8_t idx = static_cast<uint8_t>((millis() / 2000UL) % warningCount);
        sendCustomText(textType, warnings[idx]);
        return;
    }

    const char *preArmText = _fpvPreArmText;
    const uint8_t preArmTarget = prefixedTarget(_fpvPreArmText, &preArmText);
    const bool reminderAllowed = preArmText && preArmText[0] && destination == preArmTarget &&
                                 strcmp(state, "RDY") == 0 &&
                                 (!_fpvPreArmEnabled || !_hasArmedSinceBoot) &&
                                 !warningHere;
    const uint16_t interval = _fpvPreArmIntervalMs < 100 ? 3000 : _fpvPreArmIntervalMs;
    const uint16_t showMs = _fpvPreArmShowMs < 100 ? 100 : _fpvPreArmShowMs;
    if (reminderAllowed && (millis() % interval) < showMs) {
        sendCustomText(textType, preArmText);
        return;
    }

    char text[TEXT_LIMIT + 1] = {};
    const bool recOnlyTakeover = recOnlyWhenArmed && _armed && cameraRecording &&
                                 !cameraError && templateHasStatus(tpl);
    if (recOnlyTakeover) {
        // Locked V1 behaviour: only a Status-containing destination is
        // temporarily replaced by REC while armed + recording. All other
        // destinations remain untouched; the configured template returns
        // immediately when recording stops or the FC disarms.
        snprintf(text, sizeof(text), "REC");
    } else {
        expandTemplate(tpl ? tpl : "", data, state, text, sizeof(text));
    }

    // Flash only the REC token, not the whole line. Replacing it after template
    // expansion preserves its exact three-character width so adjacent values do
    // not move during the 1 Hz flash cycle (matching the web Preview).
    if (strcmp(state, "REC") == 0 && _fpvRecFlash && ((millis() / 500UL) & 1U)) {
        blankRecToken(text);
    }

    sendCustomText(textType, text);
}

void MSPSerial::sendCameraStatus(const CameraData &data) {
    MSP2CameraPayload p{};
    p.percent = data.percent;
    p.camera_mode = data.camera_mode;
    p.recording = data.recording ? 1 : 0;
    p.temp_over = data.temp_over;
    p.eis_mode = data.eis_mode;
    p.record_time = data.record_time;
    p.remain_cap_mb = data.remain_cap_mb;
    p.remain_time = data.remain_time;
    sendFrame(MSP2_CAMERA_BATTERY, reinterpret_cast<const uint8_t *>(&p), sizeof(p));
}

uint8_t MSPSerial::crc8DvbS2(uint8_t crc, uint8_t byte) {
    crc ^= byte;
    for (int i = 0; i < 8; ++i) crc = (crc & 0x80) ? (crc << 1) ^ 0xD5 : (crc << 1);
    return crc;
}

uint8_t MSPSerial::crc8DvbS2Buf(uint8_t crc, const uint8_t *buf, uint16_t length) {
    for (uint16_t i = 0; i < length; ++i) crc = crc8DvbS2(crc, buf[i]);
    return crc;
}
