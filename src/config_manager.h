#pragma once

#include <Preferences.h>
#include <Stream.h>
#include <stdint.h>

class CameraRegistry;  // forward declaration
class Camera;           // forward declaration
struct CameraData;      // forward declaration

class ConfigManager {
public:
    static constexpr uint8_t OSD_TPL_LEN = 32;

    struct Config {
        uint32_t disarmStopDelayMs;  // delay between FC disarm and stopping recording
        bool     stopOnDisarm;       // false = never stop recording on disarm
        uint8_t  auxChannel;         // AUX channel for camera mode switch (0=disabled, 1=AUX1, …)
        uint8_t  auxMode;            // camera mode when AUX is high (0x00=slow motion)
        uint8_t  cameraType;         // 0=DJI, 1=GoPro, 2=Caddx, 3=Sony, 4=Blackmagic, 5=Insta360
        uint8_t  cameraMatchMode;    // CAM_MATCH_* — see camera.h
        bool     cameraWakeGuard;    // true = don't connect to a sleeping/powered-down GoPro (see camera.h setWakeGuard)
        bool     debugBle;           // true = log raw BLE TX/RX packets to the serial console
        bool     lowPowerMode;       // true = minimum BLE/Wi-Fi TX power, to reduce interference with the RC receiver (see camera.h setLowPowerMode)
        uint32_t wifiApStartDelaySec; // seconds after boot/disconnect before the config-portal AP auto-starts
        bool     wifiApEnabled;      // false = never auto-start the AP (BOOT-button force-AP still works — see main.cpp)
        // OSD custom message templates — tokens: {bat} {rec} {mode} {res} {fps} {eis} {rleft} {rcap}
        char osd1Tpl[OSD_TPL_LEN];
        char osd2Tpl[OSD_TPL_LEN];
        char osd3Tpl[OSD_TPL_LEN];
        char osd4Tpl[OSD_TPL_LEN];
        // Legacy Betaflight uses Pilot Name and Craft Name instead of Custom
        // Message 1-4. The configurator selects exactly one method at a time.
        bool     bf45Compat;
        bool     pilotNameEnabled;
        char     pilotNameTpl[OSD_TPL_LEN];
        bool     craftNameEnabled;
        char     craftNameTpl[OSD_TPL_LEN];
        // Shared state/warning behaviour used by every active OSD destination.
        bool     fpvStateMode;
        bool     fpvErrorEnabled;
        char     fpvErrorText[OSD_TPL_LEN];
        bool     fpvReadyEnabled;
        char     fpvReadyText[OSD_TPL_LEN];
        bool     fpvRecordingEnabled;
        char     fpvRecordingText[OSD_TPL_LEN];
        bool     fpvRecFlash;
        bool     fpvLowBatteryEnabled;
        uint8_t  fpvLowBatteryPct;
        bool     fpvLowBatteryReadyFlash;
        bool     fpvLowBatteryRecText;
        char     fpvLowBatteryText[OSD_TPL_LEN];
        bool     fpvLowRecTimeEnabled;
        uint16_t fpvLowRecTimeMin;
        bool     fpvLowRecReadyWarning;
        bool     fpvLowRecRecordingWarning;
        char     fpvLowRecTimeText[OSD_TPL_LEN];
        bool     fpvHotWarningEnabled;
        bool     fpvHotReadyWarning;
        bool     fpvHotRecordingWarning;
        char     fpvHotWarningText[OSD_TPL_LEN];
        bool     fpvPreArmReminderEnabled;
        char     fpvPreArmReminderText[OSD_TPL_LEN];
        uint16_t fpvPreArmReminderShowMs;
        uint16_t fpvPreArmReminderIntervalMs;
    };

    static constexpr uint32_t DEFAULT_DISARM_STOP_DELAY_MS = 5000;
    static constexpr bool     DEFAULT_STOP_ON_DISARM       = true;
    static constexpr uint8_t  DEFAULT_AUX_CHANNEL          = 0;
    static constexpr uint8_t  DEFAULT_AUX_MODE             = 0x00;
    static constexpr uint8_t  DEFAULT_CAMERA_TYPE          = 1;
    static constexpr uint8_t  DEFAULT_CAMERA_MATCH_MODE    = 0;
    static constexpr bool     DEFAULT_CAMERA_WAKE_GUARD    = true;
    static constexpr bool     DEFAULT_DEBUG_BLE            = false;
    static constexpr bool     DEFAULT_LOW_POWER_MODE       = true;
    static constexpr uint32_t DEFAULT_WIFI_AP_START_DELAY_SEC = 30;
    static constexpr bool     DEFAULT_WIFI_AP_ENABLED      = false;

    // Stage-3/V1 OSD defaults mirror a fresh configurator session: the complete
    // OSD feature is effectively OFF until the user enables OSD Templates and
    // presses Apply. Current-BF is the default method; legacy Pilot/Craft has a
    // useful Pilot default ready for when that method is selected.
    static constexpr const char *DEFAULT_OSD1_TPL = "";
    static constexpr const char *DEFAULT_OSD2_TPL = "";
    static constexpr const char *DEFAULT_OSD3_TPL = "";
    static constexpr const char *DEFAULT_OSD4_TPL = "";
    static constexpr bool     DEFAULT_BF45_COMPAT        = false;
    static constexpr bool     DEFAULT_PILOT_NAME_ENABLED = true;
    static constexpr const char *DEFAULT_PILOT_NAME_TPL  = "{stateonly} {batt} {rectf}";
    static constexpr bool     DEFAULT_CRAFT_NAME_ENABLED = false;
    static constexpr const char *DEFAULT_CRAFT_NAME_TPL  = "";

    static constexpr bool     DEFAULT_FPV_STATE_MODE          = true;
    static constexpr bool     DEFAULT_FPV_ERROR_ENABLED       = true;
    static constexpr const char *DEFAULT_FPV_ERROR_TEXT       = "{state}";
    static constexpr bool     DEFAULT_FPV_READY_ENABLED       = true;
    static constexpr const char *DEFAULT_FPV_READY_TEXT       = "{state}";
    static constexpr bool     DEFAULT_FPV_RECORDING_ENABLED   = true;
    static constexpr const char *DEFAULT_FPV_RECORDING_TEXT   = "{state}"; // REC-only OFF
    static constexpr bool     DEFAULT_FPV_REC_FLASH           = true;

    // Warnings are OFF until explicitly enabled in the configurator.
    static constexpr bool     DEFAULT_FPV_LOW_BATTERY_ENABLED = false;
    static constexpr uint8_t  DEFAULT_FPV_LOW_BATTERY_PCT     = 10;
    static constexpr bool     DEFAULT_FPV_LOW_BAT_READY_FLASH = true;
    static constexpr bool     DEFAULT_FPV_LOW_BAT_REC_TEXT    = true;
    static constexpr const char *DEFAULT_FPV_LOW_BATTERY_TEXT = "BATT LOW";
    static constexpr bool     DEFAULT_FPV_LOW_REC_ENABLED      = false;
    static constexpr uint16_t DEFAULT_FPV_LOW_REC_MIN          = 5;
    static constexpr bool     DEFAULT_FPV_LOW_REC_READY        = true;
    static constexpr bool     DEFAULT_FPV_LOW_REC_RECORDING    = true;
    static constexpr const char *DEFAULT_FPV_LOW_REC_TEXT      = "REC LOW";
    static constexpr bool     DEFAULT_FPV_HOT_ENABLED          = false;
    static constexpr bool     DEFAULT_FPV_HOT_READY            = true;
    static constexpr bool     DEFAULT_FPV_HOT_RECORDING        = true;
    static constexpr const char *DEFAULT_FPV_HOT_TEXT          = "CAM HOT";

    // Temporary Message master is represented by a non-empty reminder text;
    // empty text means OFF. This bool is the "Only before first arm" behaviour.
    static constexpr bool     DEFAULT_FPV_PREARM_ENABLED       = true;
    static constexpr const char *DEFAULT_FPV_PREARM_TEXT       = "";
    static constexpr uint16_t DEFAULT_FPV_PREARM_SHOW_MS       = 1000;
    static constexpr uint16_t DEFAULT_FPV_PREARM_INTERVAL_MS   = 3000;

    void begin(Stream &serial);
    void update();
    void setRegistry(CameraRegistry *reg) { _registry = reg; }
    void setCamera(Camera *cam, const CameraData *data) { _camera = cam; _cameraData = data; }

    const Config &config() const { return _cfg; }

    void processCommand(const char *line, Stream &out);

    void setCameraType(uint8_t v);
    void setDisarmDelay(uint32_t ms);
    void setStopOnDisarm(bool v);
    void setAuxChannel(uint8_t ch);
    void setAuxMode(uint8_t mode);
    void setCameraMatchMode(uint8_t v);
    void setCameraWakeGuard(bool v);
    void setDebugBle(bool v);
    void setLowPowerMode(bool v);
    void setWifiApStartDelay(uint32_t sec);
    void setWifiApEnabled(bool v);
    void setOsdTemplate(uint8_t n, const char *tpl);
    void setBf45Compat(bool v);
    void setPilotNameEnabled(bool v);
    void setPilotNameTemplate(const char *tpl);
    void setCraftNameEnabled(bool v);
    void setCraftNameTemplate(const char *tpl);
    void setFpvStateMode(bool v);
    void setFpvErrorEnabled(bool v);
    void setFpvErrorText(const char *text);
    void setFpvReadyEnabled(bool v);
    void setFpvReadyText(const char *text);
    void setFpvRecordingEnabled(bool v);
    void setFpvRecordingText(const char *text);
    void setFpvRecFlash(bool v);
    void setFpvLowBatteryEnabled(bool v);
    void setFpvLowBatteryPct(uint8_t pct);
    void setFpvLowBatteryReadyFlash(bool v);
    void setFpvLowBatteryRecText(bool v);
    void setFpvLowBatteryText(const char *text);
    void setFpvLowRecTimeEnabled(bool v);
    void setFpvLowRecTimeMin(uint16_t mins);
    void setFpvLowRecReadyWarning(bool v);
    void setFpvLowRecRecordingWarning(bool v);
    void setFpvLowRecTimeText(const char *text);
    void setFpvHotWarningEnabled(bool v);
    void setFpvHotReadyWarning(bool v);
    void setFpvHotRecordingWarning(bool v);
    void setFpvHotWarningText(const char *text);
    void setFpvPreArmReminderEnabled(bool v);
    void setFpvPreArmReminderText(const char *text);
    void setFpvPreArmReminderShowMs(uint16_t ms);
    void setFpvPreArmReminderIntervalMs(uint16_t ms);

    void setCaddxSsid(const char *ssid);
    void setCaddxPass(const char *pass);

    void setCameraRegistry(CameraRegistry *r) { _registry = r; }
    void setCameraPtr(Camera *c) { _camera = c; }
    void setCameraDataPtr(const CameraData *d) { _cameraData = d; }

private:
    void load();
    void printAll(Stream &out) const;
    void handleSet(const char *rest, Stream &out);

    Preferences _prefs;
    Config _cfg{};
    Stream *_serial = nullptr;
    CameraRegistry *_registry = nullptr;
    Camera *_camera = nullptr;
    const CameraData *_cameraData = nullptr;
};
