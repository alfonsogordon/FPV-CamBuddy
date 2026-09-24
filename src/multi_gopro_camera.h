#pragma once
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEClient.h>
#include <BLERemoteService.h>
#include <BLERemoteCharacteristic.h>
#include <BLEAdvertisedDevice.h>
#include "camera.h"
#include "camera_registry.h"
#include "gopro_protocol.h"

// Application safety ceiling only. Slot state itself is allocated lazily as
// cameras are discovered, so unused cameras consume no full session state.
static constexpr uint8_t MAX_MULTI_GOPRO_SLOTS = 4;

class MultiGoProCamera : public Camera,
                         public BLEAdvertisedDeviceCallbacks,
                         public BLEClientCallbacks {
public:
    void begin() override;
    void update() override;
    bool isConnected() const override { return connectedCount() > 0; }
    bool startRecording() override;
    bool stopRecording() override;
    bool switchCameraMode(uint8_t mode) override;
    bool setDateTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second);

    uint8_t connectedCount() const;
    uint8_t recordingCount() const;
    uint8_t copySources(CameraData &out) const;

    bool prepareSharedScanSlot();
    bool acceptSharedAdvertisement(BLEAdvertisedDevice device);
    bool sharedConnectionPending() const;

private:
    struct Slot {
        BLEClient *client = nullptr;
        BLERemoteCharacteristic *cmdWrite = nullptr;
        BLERemoteCharacteristic *cmdNotify = nullptr;
        BLERemoteCharacteristic *settingWrite = nullptr;
        BLERemoteCharacteristic *settingNotify = nullptr;
        BLERemoteCharacteristic *queryWrite = nullptr;
        BLERemoteCharacteristic *queryNotify = nullptr;
        std::string addr;
        std::string name;
        esp_ble_addr_type_t addrType = BLE_ADDR_TYPE_PUBLIC;
        bool found = false;
        bool bleConnected = false;
        bool ready = false;
        bool pendingHwInfo = false;
        bool pendingRegisterSettings = false;
        bool pendingRegisterStatus = false;
        bool needsStopOnReconnect = false;
        bool recording = false;
        bool recordingKnown = false;
        bool pendingShutter = false;
        bool pendingShutterOn = false;
        uint32_t lastAttemptMs = 0;
        uint32_t lastKeepAliveMs = 0;
        uint32_t lastStatusPollMs = 0;
        uint8_t cameraNumber = 0;
        char cameraLabel[CAMREG_LABEL_LEN] = {};
        CameraData telemetry{};
        GpRxAssembler cmdRx;
        GpRxAssembler settingRx;
        GpRxAssembler queryRx;
    };

    void onResult(BLEAdvertisedDevice device) override;
    void onConnect(BLEClient *client) override;
    void onDisconnect(BLEClient *client) override;

    void startScan(uint8_t slot);
    bool connectSlot(uint8_t slot);
    void sendHardwareInfo(uint8_t slot);
    void sendRegisterSettings(uint8_t slot);
    void sendRegisterStatus(uint8_t slot);
    void sendKeepAlive(uint8_t slot);
    void sendStatusPoll(uint8_t slot);
    void logSlotTelemetry(uint8_t slot);
    bool sendShutter(uint8_t slot, bool on);
    void syncSlotToDesiredState(uint8_t slot);
    void stampSlotIdentity(uint8_t slot);
    void parseSlotStatusTlv(uint8_t slot, const uint8_t *tlv, size_t len);
    void publishState();
    bool addressAlreadyUsed(const std::string &addr) const;
    int slotForClient(BLEClient *client) const;
    int slotForNotify(BLERemoteCharacteristic *ch) const;
    void handleCmdNotify(uint8_t slot, uint8_t *data, size_t len);
    void handleSettingNotify(uint8_t slot, uint8_t *data, size_t len);
    void handleQueryNotify(uint8_t slot, uint8_t *data, size_t len);

    static void scanDoneCallback(BLEScanResults results);
    static void cmdNotifyCallback(BLERemoteCharacteristic *ch,
                                  uint8_t *data, size_t len, bool isNotify);
    static void settingNotifyCallback(BLERemoteCharacteristic *ch,
                                      uint8_t *data, size_t len, bool isNotify);
    static void queryNotifyCallback(BLERemoteCharacteristic *ch,
                                    uint8_t *data, size_t len, bool isNotify);

    Slot *_slots[MAX_MULTI_GOPRO_SLOTS]{};
    Slot *ensureSlot(uint8_t slot);
    Slot *slotAt(uint8_t slot) const { return slot < MAX_MULTI_GOPRO_SLOTS ? _slots[slot] : nullptr; }
    int8_t _scanSlot = -1;
    bool _scanning = false;
    bool _recordingRequested = false;
    uint32_t _recordStartedMs = 0;
    CameraData _camera{};

    static MultiGoProCamera *_instance;
};

inline bool MultiGoProCamera::prepareSharedScanSlot() {
    for (uint8_t i = 0; i < MAX_MULTI_GOPRO_SLOTS; ++i) {
        Slot *s = _slots[i];
        if (!s || (!s->ready && !s->bleConnected && !s->found)) {
            _scanSlot = static_cast<int8_t>(i);
            return true;
        }
    }
    _scanSlot = -1;
    return false;
}

inline bool MultiGoProCamera::acceptSharedAdvertisement(BLEAdvertisedDevice device) {
    if (_scanSlot < 0 && !prepareSharedScanSlot()) return false;
    Slot *sp = ensureSlot(static_cast<uint8_t>(_scanSlot));
    if (!sp) return false;
    Slot &s = *sp;
    const bool had = s.found;

    // Keep Wake Guard active for saved cameras while idle so a sleeping/off
    // GoPro is not powered on merely because the board sees its advertisement.
    // During an active recording request, allow a known camera to bypass Wake
    // Guard so an in-flight range drop can still be reacquired.
    bool knownCamera = false;
    if (_registry) {
        const std::string addr = device.getAddress().toString();
        uint8_t number = 0;
        char label[CAMREG_LABEL_LEN] = {};
        knownCamera = _registry->identityForAddr(addr.c_str(), number, label, sizeof(label));
    }
    const bool wakeGuardWas = _wakeGuard;
    if (knownCamera && _recordingRequested) _wakeGuard = false;
    onResult(device);
    _wakeGuard = wakeGuardWas;

    return !had && s.found;
}

inline bool MultiGoProCamera::sharedConnectionPending() const {
    // Once shared discovery has claimed a GoPro, do not restart the BLE scan
    // until that slot has completed the Open GoPro handshake (READY) or the
    // connection attempt has failed and cleared its state. This is especially
    // important for camera 2: slot 1 can otherwise be found while slot 0 still
    // has deferred handshake/status work, causing the shared scan to restart
    // before slot 1 gets its GATT connection attempt.
    for (const Slot *s : _slots) {
        if (s && !s->ready && (s->found || s->bleConnected || s->pendingHwInfo ||
                              s->pendingRegisterSettings || s->pendingRegisterStatus))
            return true;
    }
    return false;
}

inline uint8_t MultiGoProCamera::copySources(CameraData &out) const {
    out.source_count = 0;
    for (const Slot *s : _slots) {
        if (!s || !s->ready || out.source_count >= CAM_OSD_SOURCE_MAX) continue;
        CameraSourceData &dst = out.sources[out.source_count++];
        dst.valid = s->telemetry.valid;
        dst.camera_number = s->cameraNumber;
        strlcpy(dst.camera_label, s->cameraLabel, sizeof(dst.camera_label));
        dst.has_battery = s->telemetry.has_battery;
        dst.percent = s->telemetry.percent;
        dst.has_recording = s->recordingKnown || s->telemetry.has_recording;
        dst.recording = s->recordingKnown ? s->recording : s->telemetry.recording;
        dst.has_temperature = s->telemetry.has_temperature;
        dst.temp_over = s->telemetry.temp_over;
        dst.has_remain_time = s->telemetry.has_remain_time;
        dst.remain_time = s->telemetry.remain_time;
        dst.remain_cap_mb = s->telemetry.remain_cap_mb;
        dst.record_time = s->telemetry.record_time;
        dst.camera_mode = s->telemetry.camera_mode;
        dst.eis_mode = s->telemetry.eis_mode;
        dst.resolution = s->telemetry.resolution;
        dst.fps_idx = s->telemetry.fps_idx;
    }
    return out.source_count;
}
