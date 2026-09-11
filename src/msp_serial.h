#pragma once
#include <Arduino.h>
#include "telemetry.h"

// ─── MSP command IDs ─────────────────────────────────────────────────────────
// Legacy MSP v1 IDs (accepted in MSP v2 frames):
#define MSP_STATUS                  101   // poll FC arm / flight-mode state
#define MSP_RC                      105   // poll raw RC channel values (RPYT + AUX1…)

// Betaflight text field setter (msp_protocol_v2_betaflight.h: 0x3007).
// Direction '<' (command). Payload: textType(1) + textLen(1) + text bytes.
#define MSP2_SET_TEXT               0x3007

// Text type values for MSP2_SET_TEXT / MSP2_GET_TEXT
#define MSP_TEXT_PILOT_NAME         1   // Pilot Name field — present since BF4.5, unlike Custom Message 1-4
#define MSP_TEXT_CRAFT_NAME         2   // Craft Name field — present since BF4.5, unlike Custom Message 1-4
#define MSP_TEXT_CUSTOM_1           7   // "Custom Message 1" OSD field
#define MSP_TEXT_CUSTOM_2           8   // "Custom Message 2" OSD field
#define MSP_TEXT_CUSTOM_3           9   // "Custom Message 3" OSD field
#define MSP_TEXT_CUSTOM_4           10  // "Custom Message 4" OSD field

// Custom vendor command — carries DJI Action camera telemetry.
// Receivers that don't recognise 0x3001 silently ignore the frame.
#define MSP2_CAMERA_BATTERY         0x3001

// ─── MSP v2 payload structs ───────────────────────────────────────────────────
// Packed so memcpy onto the wire is safe regardless of compiler padding.

// MSP2_CAMERA_BATTERY (0x3001) payload — custom vendor message.
// Fields reflect what the DJI BLE status push actually provides.
struct __attribute__((packed)) MSP2CameraPayload {
    uint8_t  percent;        // battery 0–100 %
    uint8_t  camera_mode;    // 0x01=video, 0x05=photo, 0x0A=hyperlapse, …
    uint8_t  recording;      // 0=idle, 1=recording
    uint8_t  temp_over;      // 0=ok, 1=warn, 2=hot(can't record), 3=shutdown
    uint8_t  eis_mode;       // 0=off, 1=RS, 2=HS, 3=RS+, 4=HB
    uint16_t record_time;    // seconds currently recording
    uint32_t remain_cap_mb;  // SD card remaining (MB)
    uint32_t remain_time;    // recording seconds remaining
};
static_assert(sizeof(MSP2CameraPayload) == 15, "Camera payload size mismatch");

// Arm-state change notification — called on the main task (from update()).
using ArmCallback = void (*)(bool armed);

// AUX switch notification: fires on edge (low→high or high→low) of the
// configured AUX channel crossing the 1500 µs midpoint.
using AuxSwitchCallback = void (*)(bool high);

// ─────────────────────────────────────────────────────────────────────────────

class MSPSerial {
public:
    void begin(HardwareSerial &serial);

    // Call from loop() — polls FC for arm state and drains RX bytes.
    void update();

    // Build and send MSP2_CAMERA_BATTERY (0x3001) from a CameraData snapshot.
    void sendCameraStatus(const CameraData &data);

    // Expand `tpl` with telemetry tokens and push as Betaflight Custom Message N.
    // Tokens include {state}, {bat}, {batn}, {rect}, {rec}, {recdur},
    //         {mode}/{res}/{fps}/{eis}=settings, {rleft}=remaining record time, {rcap}=SD free space.
    void sendCustomOSD1(const CameraData &data, const char *tpl);
    void sendCustomOSD2(const CameraData &data, const char *tpl);
    void sendCustomOSD3(const CameraData &data, const char *tpl);
    void sendCustomOSD4(const CameraData &data, const char *tpl);

    // BF4.5 compatibility: Betaflight 4.5 has the Pilot Name and Craft Name
    // fields but not the Custom Message 1-4 fields (added in 4.6). Expand
    // `tpl` the same way as sendCustomOSD1-4 but write it into those fields
    // instead.
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
    void setArmCallback(ArmCallback cb) { _armCb = cb; }

    // AUX switch: channel=0 disables polling, 1=AUX1, 2=AUX2, …
    void setAuxChannel(uint8_t channel);
    void setAuxSwitchCallback(AuxSwitchCallback cb) { _auxSwitchCb = cb; }

private:
    // ── TX ────────────────────────────────────────────────────────────────
    // MSP v2 frame: $ X dir | flag(1) cmd(2LE) size(2LE) payload | crc8
    // dir='>' for push (sensor injection); dir='<' for commands (SET_TEXT etc.)
    void sendFrame(uint16_t cmd, const uint8_t *payload, uint16_t length, char dir = '>');
    // MSP v2 request frame: $ X < | flag(1) cmd(2LE) size(2LE) [empty] | crc8
    void sendRequest(uint16_t cmd);
    // MSP2_SET_TEXT helper: type(1) + text bytes
    void sendCustomText(uint8_t textType, const char *text);
    // Expand template and send as the given custom OSD slot
    void sendCustomOSD(uint8_t textType, const CameraData &data, const char *tpl, const char *stateOverride = nullptr);

    // ── RX parser ─────────────────────────────────────────────────────────
    void feedByte(uint8_t b);
    void processResponse();
    void handleStatusResponse();
    void handleRcResponse();

    enum class RxState : uint8_t {
        IDLE, HDR_X, HDR_DIR, FLAG, CMD_LO, CMD_HI, SZ_LO, SZ_HI, PAYLOAD, CRC
    };

    static constexpr uint8_t RX_BUF_SIZE = 32;
    RxState  _rxState = RxState::IDLE;
    uint8_t  _rxBuf[RX_BUF_SIZE]{};
    uint16_t _rxCmd   = 0;
    uint16_t _rxSize  = 0;
    uint16_t _rxPos   = 0;
    uint8_t  _rxCrc   = 0;

    // ── CRC ───────────────────────────────────────────────────────────────
    static uint8_t crc8DvbS2(uint8_t crc, uint8_t byte);
    static uint8_t crc8DvbS2Buf(uint8_t seed,
                                  const uint8_t *buf, uint16_t length);

    // ── State ─────────────────────────────────────────────────────────────
    HardwareSerial   *_serial       = nullptr;
    bool              _armed        = false;
    ArmCallback       _armCb        = nullptr;
    uint32_t          _lastPollMs   = 0;

    uint8_t           _auxChannel   = 0;      // 0=disabled, 1=AUX1, …
    bool              _auxHigh      = false;  // last known state
    AuxSwitchCallback _auxSwitchCb  = nullptr;

    // Options for state-aware BF4.5 Craft Name presentation.
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
    char _fpvPreArmText[32] = "CLEAN LENS";
    uint16_t _fpvPreArmShowMs = 1000;
    uint16_t _fpvPreArmIntervalMs = 3000;
    bool _hasArmedSinceBoot = false;
};
