#pragma once
#include <Arduino.h>
#include "telemetry.h"

#define MSP_STATUS 101
#define MSP_RC 105
#define MSP_RAW_GPS 106
#define MSP_RTC 247
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
using ProfileSwitchCallback = void (*)(uint8_t position); // 0=low,1=mid,2=high
using RecordSwitchCallback = void (*)(bool high);

struct MspRtcDateTime {
    uint16_t year = 0;
    uint8_t month = 0, day = 0, hour = 0, minute = 0, second = 0;
    uint16_t millis = 0;
    bool valid = false;
};
using RtcCallback = void (*)(const MspRtcDateTime &dt);

class MSPSerial {
public:
    void begin(HardwareSerial &serial);
    void update();
    void sendCameraStatus(const CameraData &data);
    void sendCustomOSD1(const CameraData &data, const char *tpl);
    void sendCustomOSD2(const CameraData &data, const char *tpl);
    void sendCustomOSD3(const CameraData &data, const char *tpl);
    void sendCustomOSD4(const CameraData &data, const char *tpl);
    void sendCustomOSDWarnings(uint8_t target, const CameraData &data, const char *tpl);
    uint8_t warningTarget() const { return _fpvWarningTarget; }
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
        _fpvWarningTarget = 1;
        const char *cleanLowBatteryText = lowBatteryText ? lowBatteryText : "";
        if (cleanLowBatteryText[0] == '@' && cleanLowBatteryText[1] >= '1' && cleanLowBatteryText[1] <= '4' && cleanLowBatteryText[2] == ':') {
            _fpvWarningTarget = static_cast<uint8_t>(cleanLowBatteryText[1] - '0');
            cleanLowBatteryText += 3;
        }
        strlcpy(_fpvLowBatteryText, cleanLowBatteryText, sizeof(_fpvLowBatteryText));
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
    // USB bench simulation uses the same state/callback path as MSP responses.
    // RAM-only: reboot clears it and it can never arm a real flight controller.
    void simulateArmState(bool armed) {
        if (armed) _hasArmedSinceBoot = true;
        if (_armed == armed) return;
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
    void setProfileAuxChannel(uint8_t channel);
    void setProfileSwitchCallback(ProfileSwitchCallback cb) { _profileSwitchCb = cb; }
    void setRecordAuxChannel(uint8_t channel);
    void setRecordSwitchCallback(RecordSwitchCallback cb) { _recordSwitchCb = cb; }
    void setRtcCallback(RtcCallback cb) { _rtcCb = cb; }
    const MspRtcDateTime &rtc() const { return _rtc; }
    bool gpsTimeReady() const { return _gpsFix && _gpsSatellites > 0 && _rtc.valid; }
    bool gpsFix() const { return _gpsFix; }
    uint8_t gpsSatellites() const { return _gpsSatellites; }
    void showTransientMessage(uint8_t target, const char *text, uint16_t durationMs);

private:
    void sendFrame(uint16_t cmd, const uint8_t *payload, uint16_t length, char dir = '>');
    void sendRequest(uint16_t cmd);
    void sendCustomText(uint8_t textType, const char *text);
    void sendCustomOSD(uint8_t textType, const CameraData &data, const char *tpl, const char *stateOverride = nullptr);
    void feedByte(uint8_t b);
    void processResponse();
    void handleStatusResponse();
    void handleRcResponse();
    void handleGpsResponse();
    void handleRtcResponse();
    enum class RxState : uint8_t { IDLE, HDR_X, HDR_DIR, FLAG, CMD_LO, CMD_HI, SZ_LO, SZ_HI, PAYLOAD, CRC };
    static constexpr uint8_t RX_BUF_SIZE = 32;
    RxState _rxState = RxState::IDLE;
    uint8_t _rxBuf[RX_BUF_SIZE]{};
    uint16_t _rxCmd = 0;
    uint16_t _rxSize = 0;
    uint16_t _rxPos = 0;
    uint8_t _rxCrc = 0;
    static uint8_t crc8DvbS2(uint8_t crc, uint8_t byte);
    static uint8_t crc8DvbS2Buf(uint8_t seed, const uint8_t *buf, uint16_t length);
    HardwareSerial *_serial = nullptr;
    bool _armed = false;
    ArmCallback _armCb = nullptr;
    uint32_t _lastPollMs = 0;
    uint8_t _auxChannel = 0;
    bool _auxHigh = false;
    AuxSwitchCallback _auxSwitchCb = nullptr;
    uint8_t _profileAuxChannel = 0;
    uint8_t _recordAuxChannel = 0;
    bool _recordAuxHigh = false;
    RecordSwitchCallback _recordSwitchCb = nullptr;
    uint8_t _profilePosition = 0xFF;
    ProfileSwitchCallback _profileSwitchCb = nullptr;
    bool _fpvStateMode = true;
    bool _fpvErrorEnabled = true;
    char _fpvErrorText[32] = "{state}";
    bool _fpvReadyEnabled = true;
    char _fpvReadyText[32] = "{state} B:{batn} T:{rect}";
    bool _fpvRecordingEnabled = true;
    char _fpvRecordingText[32] = "{state}";
    bool _fpvRecFlash = true;
    bool _fpvLowBatteryEnabled = true;
    uint8_t _fpvLowBatteryPct = 10;
    bool _fpvLowBatteryReadyFlash = true;
    bool _fpvLowBatteryRecText = true;
    char _fpvLowBatteryText[32] = "BATT LOW";
    uint8_t _fpvWarningTarget = 1;
    bool _fpvLowRecEnabled = true;
    uint16_t _fpvLowRecMinutes = 5;
    bool _fpvLowRecReady = true;
    bool _fpvLowRecRecording = true;
    char _fpvLowRecText[32] = "REC LOW";
    bool _fpvHotEnabled = true;
    bool _fpvHotReady = true;
    bool _fpvHotRecording = true;
    char _fpvHotText[32] = "CAM HOT";
    bool _fpvPreArmEnabled = true;
    char _fpvPreArmText[32] = "";
    uint16_t _fpvPreArmShowMs = 1000;
    uint16_t _fpvPreArmIntervalMs = 3000;
    char _transientText[32] = "";
    uint8_t _transientTarget = 1;
    uint32_t _transientUntilMs = 0;
    bool _hasArmedSinceBoot = false;
    MspRtcDateTime _rtc{};
    RtcCallback _rtcCb = nullptr;
    uint32_t _lastRtcPollMs = 0;
    bool _gpsFix = false;
    uint8_t _gpsSatellites = 0;
    uint32_t _lastGpsPollMs = 0;
};
