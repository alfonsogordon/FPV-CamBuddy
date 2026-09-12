#pragma once
#include <Arduino.h>
#include "telemetry.h"

#define MSP_STATUS 101
#define MSP_RC 105
#define MSP2_SET_TEXT 0x3007
#define MSP_TEXT_PILOT_NAME 1
#define MSP_TEXT_CRAFT_NAME 2
#define MSP_TEXT_CUSTOM_1 7
#define MSP_TEXT_CUSTOM_2 8
#define MSP_TEXT_CUSTOM_3 9
#define MSP_TEXT_CUSTOM_4 10
#define MSP2_CAMERA_BATTERY 0x3001

struct __attribute__((packed)) MSP2CameraPayload {
    uint8_t percent;
    uint8_t camera_mode;
    uint8_t recording;
    uint8_t temp_over;
    uint8_t eis_mode;
    uint16_t record_time;
    uint32_t remain_cap_mb;
    uint32_t remain_time;
};
static_assert(sizeof(MSP2CameraPayload) == 15, "Camera payload size mismatch");

using ArmCallback = void (*)(bool armed);
using AuxSwitchCallback = void (*)(bool high);

class MSPSerial {
public:
    void begin(HardwareSerial &serial);
    void update();
    void sendCameraStatus(const CameraData &data);
    void sendCustomOSD1(const CameraData &data, const char *tpl);
    void sendCustomOSD2(const CameraData &data, const char *tpl);
    void sendCustomOSD3(const CameraData &data, const char *tpl);
    void sendCustomOSD4(const CameraData &data, const char *tpl);
    void sendPilotName(const CameraData &data, const char *tpl);
    void sendCraftName(const CameraData &data, const char *tpl);
    void setFpvDisplayOptions(bool stateMode,
                              bool errorEnabled, const char *errorTpl,
                              bool readyEnabled, const char *readyTpl,
                              bool recordingEnabled, const char *recordingTpl,
                              bool flashRec, bool lowBatteryEnabled,
                              uint8_t lowBatteryPct, bool lowBatteryReadyFlash,
                              bool lowBatteryRecText, const char *lowBatteryText,
                              bool lowRecEnabled, uint16_t lowRecMinutes,
                              bool lowRecReady, bool lowRecRecording, const char *lowRecText,
                              bool hotEnabled, bool hotReady, bool hotRecording, const char *hotText,
                              bool preArmEnabled, const char *preArmText, uint16_t preArmShowMs, uint16_t preArmIntervalMs) {
        _fpvStateMode = stateMode;
        _fpvErrorEnabled = errorEnabled;
        strlcpy(_fpvErrorText, errorTpl ? errorTpl : "", sizeof(_fpvErrorText));
        _fpvReadyEnabled = readyEnabled;
        strlcpy(_fpvReadyText, readyTpl ? readyTpl : "", sizeof(_fpvReadyText));
        _fpvRecordingEnabled = recordingEnabled;
        strlcpy(_fpvRecordingText, recordingTpl ? recordingTpl : "", sizeof(_fpvRecordingText));
        _fpvRecFlash = flashRec;
        _fpvLowBatteryEnabled = lowBatteryEnabled;
        _fpvLowBatteryPct = lowBatteryPct > 100 ? 100 : lowBatteryPct;
        _fpvLowBatteryReadyFlash = lowBatteryReadyFlash;
        _fpvLowBatteryRecText = lowBatteryRecText;
        strlcpy(_fpvLowBatteryText, lowBatteryText ? lowBatteryText : "", sizeof(_fpvLowBatteryText));
        _fpvLowRecEnabled = lowRecEnabled;
        _fpvLowRecMinutes = lowRecMinutes;
        _fpvLowRecReady = lowRecReady;
        _fpvLowRecRecording = lowRecRecording;
        strlcpy(_fpvLowRecText, lowRecText ? lowRecText : "", sizeof(_fpvLowRecText));
        _fpvHotEnabled = hotEnabled;
        _fpvHotReady = hotReady;
        _fpvHotRecording = hotRecording;
        strlcpy(_fpvHotText, hotText ? hotText : "", sizeof(_fpvHotText));
        _fpvPreArmEnabled = preArmEnabled;
        strlcpy(_fpvPreArmText, preArmText ? preArmText : "", sizeof(_fpvPreArmText));
        _fpvPreArmShowMs = preArmShowMs < 100 ? 100 : preArmShowMs;
        _fpvPreArmIntervalMs = preArmIntervalMs < _fpvPreArmShowMs ? _fpvPreArmShowMs : preArmIntervalMs;
    }
    bool isArmed() const { return _armed; }
    // USB bench simulation uses the same callback as a real MSP arm-state
    // transition. Always invoke it: on a bench there may be no FC response to
    // establish a previous state, and the explicit user action must reach the
    // camera handler even if the cached state already matches.
    void simulateArmState(bool armed) {
        if (armed) _hasArmedSinceBoot = true;
        _armed = armed;
        if (_armCb) _armCb(armed);
    }
    void simulateAuxSwitch(bool high) {
        if (_auxHigh == high) return;
        _auxHigh = high;
        if (_auxSwitchCb) _auxSwitchCb(high);
    }
    void setArmCallback(ArmCallback cb) { _armCb = cb; }
    void setAuxChannel(uint8_t channel);
    void setAuxSwitchCallback(AuxSwitchCallback cb) { _auxSwitchCb = cb; }

private:
    void sendFrame(uint16_t cmd, const uint8_t *payload, uint16_t length, char dir = '>');
    void sendRequest(uint16_t cmd);
    void handleByte(uint8_t b);
    void handleResponse(uint16_t cmd, const uint8_t *payload, uint16_t length);
    void sendCustomText(uint8_t textType, const char *text);
    void expandTemplate(const char *tpl, const CameraData &data, char *out, size_t outLen);
    void buildFpvStateText(const CameraData &data, char *out, size_t outLen);
    bool warningActive(const CameraData &data, bool recording, char warnings[][17], uint8_t &warningCount) const;

    HardwareSerial *_serial = nullptr;
    ArmCallback _armCb = nullptr;
    AuxSwitchCallback _auxSwitchCb = nullptr;
    bool _armed = false;
    bool _hasArmedSinceBoot = false;
    uint8_t _auxChannel = 0;
    bool _auxHigh = false;
    uint32_t _lastStatusReqMs = 0;
    uint32_t _lastRcReqMs = 0;

    enum ParseState { IDLE, HEADER_M, HEADER_DIR, SIZE, CMD, PAYLOAD, CHECKSUM };
    ParseState _state = IDLE;
    uint8_t _size = 0, _cmd = 0, _offset = 0, _checksum = 0;
    uint8_t _payload[64];

    bool _fpvStateMode = false;
    bool _fpvErrorEnabled = true;
    char _fpvErrorText[32] = "{state} {batt} {rectf}";
    bool _fpvReadyEnabled = true;
    char _fpvReadyText[32] = "{state} {batt} {rectf}";
    bool _fpvRecordingEnabled = true;
    char _fpvRecordingText[32] = "{state}";
    bool _fpvRecFlash = true;
    bool _fpvLowBatteryEnabled = true;
    uint8_t _fpvLowBatteryPct = 10;
    bool _fpvLowBatteryReadyFlash = true;
    bool _fpvLowBatteryRecText = true;
    char _fpvLowBatteryText[17] = "BATT LOW";
    bool _fpvLowRecEnabled = true;
    uint16_t _fpvLowRecMinutes = 5;
    bool _fpvLowRecReady = true;
    bool _fpvLowRecRecording = true;
    char _fpvLowRecText[17] = "REC LOW";
    bool _fpvHotEnabled = true;
    bool _fpvHotReady = true;
    bool _fpvHotRecording = true;
    char _fpvHotText[17] = "CAM HOT";
    bool _fpvPreArmEnabled = false;
    char _fpvPreArmText[32] = "";
    uint16_t _fpvPreArmShowMs = 1000;
    uint16_t _fpvPreArmIntervalMs = 3000;
};
