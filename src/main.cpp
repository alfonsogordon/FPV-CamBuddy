#include <Arduino.h>
#include <BLEDevice.h>
#include "config.h"
#include "config_manager.h"
#include "camera_registry.h"
#include "camera.h"
#include "ble_camera.h"
#include "gopro_camera.h"
#include "multi_gopro_camera.h"
#include "multi_camera_coordinator.h"
#include "caddx_camera.h"
#include "sony_camera.h"
#include "blackmagic_camera.h"
#include "insta360_camera.h"
#include "msp_serial.h"
#include "dji_protocol.h"
#include "web_server.h"

static BLECamera        djiCamera;
static GoProCamera      goProCamera;
static MultiGoProCamera multiGoProCamera;
static CaddxCamera      caddxCamera;
static SonyCamera       sonyCamera;
static BlackmagicCamera blackmagicCamera;
static Insta360Camera   insta360Camera;
static MultiCameraCoordinator multiCameraCoordinator(multiGoProCamera, djiCamera, sonyCamera, blackmagicCamera, insta360Camera, caddxCamera);
static Camera          *activeCamera = nullptr;

static CameraRegistry   cameraRegistry;
static MSPSerial        mspSerial;
static ConfigManager    configManager;
static WebConfigServer  webServer;

static volatile bool hasCamera = false;
static CameraData    currentCamera{};
static uint32_t      lastBattMs = 0;

static bool     pendingStop = false;
static uint32_t disarmMs   = 0;

static bool benchSimActive = false;
static bool benchSimAuxHigh = false;

static uint32_t wifiDelayOriginMs  = 0;
static bool     cameraWasConnected = false;

static bool     forceAP        = false;
static uint32_t bootBtnPressMs = 0;
static uint32_t bootBtnLogSec  = 0;

static bool ledState = false;

enum class BlePowerPhase : uint8_t { IDLE, ARM_BOOST, ARMED, DISARM_BOOST };
static BlePowerPhase blePowerPhase = BlePowerPhase::IDLE;
static uint32_t blePowerPhaseSinceMs = 0;
static bool multiPowerLatched = false;
static int16_t lastAppliedBlePowerDbm = 127;

static esp_power_level_t blePowerForDbm(int8_t dbm) {
    if (dbm <= -12) return ESP_PWR_LVL_N12;
    if (dbm <= -9)  return ESP_PWR_LVL_N9;
    if (dbm <= -6)  return ESP_PWR_LVL_N6;
    if (dbm <= -3)  return ESP_PWR_LVL_N3;
    if (dbm <= 0)   return ESP_PWR_LVL_N0;
    if (dbm <= 3)   return ESP_PWR_LVL_P3;
    if (dbm <= 6)   return ESP_PWR_LVL_P6;
    return ESP_PWR_LVL_P9;
}

static const char *blePowerPhaseName(BlePowerPhase phase) {
    switch (phase) {
        case BlePowerPhase::IDLE: return "IDLE";
        case BlePowerPhase::ARM_BOOST: return "ARM-BOOST";
        case BlePowerPhase::ARMED: return "ARMED";
        case BlePowerPhase::DISARM_BOOST: return "DISARM-BOOST";
    }
    return "UNKNOWN";
}

static void applyBlePowerDbm(int8_t dbm, const char *reason) {
    if (lastAppliedBlePowerDbm == dbm) return;
    BLEDevice::setPower(blePowerForDbm(dbm));
    lastAppliedBlePowerDbm = dbm;
    DBG_SERIAL.printf("[power] BLE TX -> %d dBm (%s)\n", dbm, reason ? reason : "profile");
}

static bool blePowerProfileAllowed() {
    const auto &cfg = configManager.config();
    return cfg.dynamicTxPower && (!cfg.powerMultiOnly || multiPowerLatched);
}

static void restoreBaseBlePowerIfNeeded() {
    if (lastAppliedBlePowerDbm == 127) return;
    const auto &cfg = configManager.config();
    const int8_t baseDbm = cfg.lowPowerMode ? -12 : 9;
    applyBlePowerDbm(baseDbm, "normal Low Power setting");
    lastAppliedBlePowerDbm = 127;
}

static void setBlePowerPhase(BlePowerPhase phase, uint32_t now) {
    blePowerPhase = phase;
    blePowerPhaseSinceMs = now;
}

static void updateConfiguredBleTxPower(uint32_t now) {
    const auto &cfg = configManager.config();
    if (!blePowerProfileAllowed()) {
        restoreBaseBlePowerIfNeeded();
        return;
    }

    if (blePowerPhase == BlePowerPhase::ARM_BOOST &&
        (cfg.armBoostMs == 0 || now - blePowerPhaseSinceMs >= cfg.armBoostMs)) {
        setBlePowerPhase(BlePowerPhase::ARMED, now);
    } else if (blePowerPhase == BlePowerPhase::DISARM_BOOST &&
               (cfg.disarmBoostMs == 0 || now - blePowerPhaseSinceMs >= cfg.disarmBoostMs)) {
        setBlePowerPhase(BlePowerPhase::IDLE, now);
    }

    int8_t dbm = cfg.disarmedTxPowerDbm;
    switch (blePowerPhase) {
        case BlePowerPhase::IDLE: dbm = cfg.disarmedTxPowerDbm; break;
        case BlePowerPhase::ARM_BOOST: dbm = cfg.armBoostTxPowerDbm; break;
        case BlePowerPhase::ARMED: dbm = cfg.armedTxPowerDbm; break;
        case BlePowerPhase::DISARM_BOOST: dbm = cfg.disarmBoostTxPowerDbm; break;
    }
    applyBlePowerDbm(dbm, blePowerPhaseName(blePowerPhase));
}

static void observeCameraCountForPowerLatch(const CameraData &data) {
    if (multiPowerLatched || !configManager.config().powerMultiOnly) return;
    if (data.connected_cameras > 1) {
        multiPowerLatched = true;
        DBG_SERIAL.printf("[power] multiple cameras detected (%u) - power profile latched until reboot\n",
                          data.connected_cameras);
        updateConfiguredBleTxPower(millis());
    }
}

static void updateStatusLed(uint32_t now, bool camConnected, bool apRunning) {
    bool shouldBeOn;
    if (apRunning)
        shouldBeOn = (now % STATUS_LED_AP_PERIOD_MS) < STATUS_LED_AP_ON_MS;
    else if (camConnected)
        shouldBeOn = true;
    else
        shouldBeOn = (now % STATUS_LED_SCAN_PERIOD_MS) < STATUS_LED_SCAN_ON_MS;

    if (shouldBeOn == ledState) return;
    ledState = shouldBeOn;
#if STATUS_LED_ACTIVE_LOW
    digitalWrite(STATUS_LED_PIN, ledState ? LOW : HIGH);
#else
    digitalWrite(STATUS_LED_PIN, ledState ? HIGH : LOW);
#endif
}

static void onAuxSwitch(bool high) {
    const bool isGoPro = (configManager.config().cameraType == 1) && !configManager.config().multiCamSync;
    if (isGoPro && currentCamera.recording) {
        if (high) {
            DBG_SERIAL.println("[main] AUX high + GoPro recording -> Burst Slo-Mo");
            activeCamera->triggerBurstSloMo();
        } else {
            DBG_SERIAL.println("[main] AUX low + GoPro recording -> Standard");
            activeCamera->exitBurstSloMo();
        }
        return;
    }
    const uint8_t mode = high ? configManager.config().auxMode : DJI_MODE_VIDEO;
    DBG_SERIAL.printf("[main] AUX %s -> camera mode 0x%02X\n", high ? "high" : "low", mode);
    activeCamera->switchCameraMode(mode);
}

static void onCameraData(const CameraData &data) {
    currentCamera = data;
    observeCameraCountForPowerLatch(data);
    hasCamera = true;
}

static void onArmStateChange(bool armed) {
    const uint32_t now = millis();
    const auto &cfg = configManager.config();
    if (armed) {
        setBlePowerPhase(cfg.armBoostMs > 0 ? BlePowerPhase::ARM_BOOST : BlePowerPhase::ARMED, now);
        updateConfiguredBleTxPower(now);
        pendingStop = false;
        DBG_SERIAL.println("[main] FC armed - starting recording");
        activeCamera->startRecording();
    } else {
        setBlePowerPhase(cfg.disarmBoostMs > 0 ? BlePowerPhase::DISARM_BOOST : BlePowerPhase::IDLE, now);
        updateConfiguredBleTxPower(now);
        DBG_SERIAL.println("[main] FC disarmed");
        if (!cfg.stopOnDisarm) {
            DBG_SERIAL.println("[main] stop_on_disarm disabled - keeping recording");
        } else {
            const uint32_t delay = cfg.disarmStopDelayMs;
            if (delay == 0) {
                DBG_SERIAL.println("[main] stopping recording");
                activeCamera->stopRecording();
            } else {
                DBG_SERIAL.printf("[main] stopping recording in %u ms\n", delay);
                pendingStop = true;
                disarmMs = now;
            }
        }
    }
}

void benchSimArm(bool armed) {
    benchSimActive = true;
    DBG_SERIAL.printf("[sim] FC %s (USB bench only)\n", armed ? "ARM" : "DISARM");
    mspSerial.simulateArmState(armed);
}

void benchSimAux(bool high) {
    benchSimActive = true;
    benchSimAuxHigh = high;
    DBG_SERIAL.printf("[sim] AUX %s (USB bench only)\n", high ? "HIGH" : "LOW");
    mspSerial.simulateAuxSwitch(high);
}

void benchSimOff() {
    if (mspSerial.isArmed()) mspSerial.simulateArmState(false);
    if (benchSimAuxHigh) mspSerial.simulateAuxSwitch(false);
    benchSimAuxHigh = false;
    benchSimActive = false;
}

void benchSimStatus(Stream &out) {
    out.printf("[sim] active=%s armed=%s aux=%s camera_connected=%s recording=%s\n",
               benchSimActive ? "yes" : "no", mspSerial.isArmed() ? "yes" : "no",
               benchSimAuxHigh ? "high" : "low",
               (activeCamera && activeCamera->isConnected()) ? "yes" : "no",
               currentCamera.recording ? "yes" : "no");
}

void setup() {
    DBG_SERIAL.begin(DBG_BAUD);
    BF_SERIAL.begin(BF_BAUD, SERIAL_8N1, BF_RX_PIN, BF_TX_PIN);

    cameraRegistry.begin();

    configManager.begin(DBG_SERIAL);
    configManager.loadV101Extras();
    configManager.printV101Extras(DBG_SERIAL);
    configManager.setRegistry(&cameraRegistry);

    const uint8_t camType = configManager.config().cameraType;
    const char *camTypeName;
    if (configManager.config().multiCamSync) {
        activeCamera = &multiCameraCoordinator;
        camTypeName = "Multi Brand / Multi Cam";
    } else {
        switch (camType) {
            case 1:  activeCamera = &goProCamera;       camTypeName = "GoPro Single"; break;
            case 2:  activeCamera = &caddxCamera;       camTypeName = "Caddx Orca"; break;
            case 3:  activeCamera = &sonyCamera;        camTypeName = "Sony Alpha"; break;
            case 4:  activeCamera = &blackmagicCamera;  camTypeName = "Blackmagic"; break;
            case 5:  activeCamera = &insta360Camera;    camTypeName = "Insta360"; break;
            default: activeCamera = &djiCamera;         camTypeName = "DJI Action"; break;
        }
    }

    DBG_SERIAL.printf("[main] Camera type: %s\n", camTypeName);
    DBG_SERIAL.printf("[main] MSP output: TX=GPIO%d @ %u baud\n", BF_TX_PIN, BF_BAUD);
    if (configManager.config().wifiApEnabled) {
        DBG_SERIAL.printf("[main] WiFi AP '%s' starts in %u s if no camera connects\n",
                          WIFI_AP_SSID, configManager.config().wifiApStartDelaySec);
    } else {
        DBG_SERIAL.println("[main] WiFi AP auto-start disabled (BOOT-button force-AP still available)");
    }

    pinMode(WIFI_FORCE_AP_PIN, INPUT_PULLUP);
    pinMode(STATUS_LED_PIN, OUTPUT);
#if STATUS_LED_ACTIVE_LOW
    digitalWrite(STATUS_LED_PIN, HIGH);
#else
    digitalWrite(STATUS_LED_PIN, LOW);
#endif

    mspSerial.begin(BF_SERIAL);
    mspSerial.setArmCallback(onArmStateChange);
    mspSerial.setAuxSwitchCallback(onAuxSwitch);

    djiCamera.setRegistry(&cameraRegistry);
    goProCamera.setRegistry(&cameraRegistry);
    multiGoProCamera.setRegistry(&cameraRegistry);
    caddxCamera.setRegistry(&cameraRegistry);
    sonyCamera.setRegistry(&cameraRegistry);
    blackmagicCamera.setRegistry(&cameraRegistry);
    insta360Camera.setRegistry(&cameraRegistry);

    const uint8_t matchMode = configManager.config().cameraMatchMode;
    djiCamera.setMatchMode(matchMode);
    goProCamera.setMatchMode(matchMode);
    multiGoProCamera.setMatchMode(matchMode);
    sonyCamera.setMatchMode(matchMode);
    blackmagicCamera.setMatchMode(matchMode);
    insta360Camera.setMatchMode(matchMode);

    const bool wakeGuard = configManager.config().cameraWakeGuard;
    djiCamera.setWakeGuard(wakeGuard);
    goProCamera.setWakeGuard(wakeGuard);
    multiGoProCamera.setWakeGuard(wakeGuard);
    sonyCamera.setWakeGuard(wakeGuard);
    blackmagicCamera.setWakeGuard(wakeGuard);
    insta360Camera.setWakeGuard(wakeGuard);

    const bool debugBle = configManager.config().debugBle;
    djiCamera.setDebugBle(debugBle);
    goProCamera.setDebugBle(debugBle);
    multiGoProCamera.setDebugBle(debugBle);
    sonyCamera.setDebugBle(debugBle);
    blackmagicCamera.setDebugBle(debugBle);
    insta360Camera.setDebugBle(debugBle);

    const bool lowPowerMode = configManager.config().lowPowerMode;
    djiCamera.setLowPowerMode(lowPowerMode);
    goProCamera.setLowPowerMode(lowPowerMode);
    multiGoProCamera.setLowPowerMode(lowPowerMode);
    caddxCamera.setLowPowerMode(lowPowerMode);
    sonyCamera.setLowPowerMode(lowPowerMode);
    blackmagicCamera.setLowPowerMode(lowPowerMode);
    insta360Camera.setLowPowerMode(lowPowerMode);

    activeCamera->setCameraCallback(onCameraData);
    configManager.setCamera(activeCamera, &currentCamera);
    activeCamera->begin();
    setBlePowerPhase(BlePowerPhase::IDLE, millis());
    updateConfiguredBleTxPower(millis());

    wifiDelayOriginMs = millis();
}

void loop() {
    configManager.updateV101();
    mspSerial.setAuxChannel(configManager.config().auxChannel);
    activeCamera->setDebugBle(configManager.config().debugBle);
    activeCamera->update();
    mspSerial.update();

    const uint32_t now          = millis();
    const bool     camConnected = activeCamera->isConnected();
    updateConfiguredBleTxPower(now);

    if (!forceAP) {
        if (digitalRead(WIFI_FORCE_AP_PIN) == LOW) {
            if (bootBtnPressMs == 0) {
                bootBtnPressMs = now;
                bootBtnLogSec  = 0;
                DBG_SERIAL.printf("[wifi] BOOT held - keep holding for %u s to force AP\n",
                                  WIFI_FORCE_AP_HOLD_MS / 1000);
            }
            uint32_t heldMs  = now - bootBtnPressMs;
            uint32_t heldSec = heldMs / 1000;
            if (heldSec != bootBtnLogSec && heldSec > 0) {
                bootBtnLogSec = heldSec;
                DBG_SERIAL.printf("[wifi] BOOT holding... %u/%u s\n",
                                  heldSec, WIFI_FORCE_AP_HOLD_MS / 1000);
            }
            if (heldMs >= WIFI_FORCE_AP_HOLD_MS) {
                forceAP        = true;
                bootBtnPressMs = 0;
                DBG_SERIAL.println("[wifi] Forcing WiFi AP mode until reboot");
                if (!webServer.isRunning())
                    webServer.begin(configManager, &cameraRegistry, &DBG_SERIAL);
            }
        } else {
            bootBtnPressMs = 0;
        }
    }

    if (camConnected && !cameraWasConnected) {
        if (webServer.isRunning() && !forceAP) {
            DBG_SERIAL.println("[wifi] Camera connected - stopping AP");
            webServer.stop();
        }
    }

    if (!camConnected && cameraWasConnected) {
        currentCamera = CameraData{};
        hasCamera     = true;

        if (!forceAP && configManager.config().wifiApEnabled) {
            DBG_SERIAL.printf("[wifi] Camera disconnected - AP starts in %u s\n",
                              configManager.config().wifiApStartDelaySec);
            wifiDelayOriginMs = now;
        }
    }

    cameraWasConnected = camConnected;

    if (!webServer.isRunning() &&
        !forceAP &&
        !camConnected &&
        configManager.config().wifiApEnabled &&
        (now - wifiDelayOriginMs) >= (configManager.config().wifiApStartDelaySec * 1000UL)) {
        webServer.begin(configManager, &cameraRegistry, &DBG_SERIAL);
    }

    webServer.update();

    if (pendingStop && (now - disarmMs) >= configManager.config().disarmStopDelayMs) {
        pendingStop = false;
        DBG_SERIAL.println("[main] stopping recording (delayed)");
        activeCamera->stopRecording();
    }

    const bool battKeepalive =
        MSP_BATTERY_KEEPALIVE_MS > 0 &&
        (now - lastBattMs) >= MSP_BATTERY_KEEPALIVE_MS;

    const auto &liveCfg = configManager.config();
    mspSerial.setFpvDisplayOptions(liveCfg.fpvStateMode,
                                   liveCfg.fpvErrorEnabled, liveCfg.fpvErrorText,
                                   liveCfg.fpvReadyEnabled, liveCfg.fpvReadyText,
                                   liveCfg.fpvRecordingEnabled, liveCfg.fpvRecordingText,
                                   liveCfg.fpvRecFlash, liveCfg.fpvLowBatteryEnabled,
                                   liveCfg.fpvLowBatteryPct, liveCfg.fpvLowBatteryReadyFlash,
                                   liveCfg.fpvLowBatteryRecText, liveCfg.fpvLowBatteryText,
                                   liveCfg.fpvLowRecTimeEnabled, liveCfg.fpvLowRecTimeMin,
                                   liveCfg.fpvLowRecReadyWarning, liveCfg.fpvLowRecRecordingWarning,
                                   liveCfg.fpvLowRecTimeText,
                                   liveCfg.fpvHotWarningEnabled, liveCfg.fpvHotReadyWarning,
                                   liveCfg.fpvHotRecordingWarning, liveCfg.fpvHotWarningText,
                                   liveCfg.fpvPreArmReminderEnabled, liveCfg.fpvPreArmReminderText,
                                   liveCfg.fpvPreArmReminderShowMs, liveCfg.fpvPreArmReminderIntervalMs);
    const bool craftFlashRefresh =
        liveCfg.bf45Compat && liveCfg.craftNameEnabled && liveCfg.fpvStateMode &&
        (now - lastBattMs) >= 500UL;

    if (hasCamera || battKeepalive || craftFlashRefresh) {
        hasCamera  = false;
        const auto &cfg = configManager.config();
        mspSerial.sendCameraStatus(currentCamera);
        if (cfg.bf45Compat) {
            if (cfg.pilotNameEnabled) mspSerial.sendPilotName(currentCamera, cfg.pilotNameTpl);
            if (cfg.craftNameEnabled) mspSerial.sendCraftName(currentCamera, cfg.craftNameTpl);
        } else {
            mspSerial.sendCustomOSD1(currentCamera, cfg.osd1Tpl);
            mspSerial.sendCustomOSD2(currentCamera, cfg.osd2Tpl);
            mspSerial.sendCustomOSD3(currentCamera, cfg.osd3Tpl);
            mspSerial.sendCustomOSD4(currentCamera, cfg.osd4Tpl);
        }
        lastBattMs = now;

        DBG_SERIAL.printf("[cam] bat=%u%%  mode=0x%02X  rec=%s  eis=%u  "
                          "time=%us  sd=%uMB  remain=%us  temp=%u\n",
                          currentCamera.percent, currentCamera.camera_mode,
                          currentCamera.recording ? "yes" : "no",
                          currentCamera.eis_mode, currentCamera.record_time,
                          currentCamera.remain_cap_mb, currentCamera.remain_time,
                          currentCamera.temp_over);
    }

    updateStatusLed(now, camConnected, webServer.isRunning());
    delay(10);
}
