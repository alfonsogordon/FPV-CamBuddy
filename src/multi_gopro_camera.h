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

// The old experimental backend stopped at an arbitrary six GoPros. C3/S3 BLE
// controller resources are the real boundary: with scanning active the useful
// controller ceiling is eight connections, and the configured host limit can
// be lower. Keep the array bounded for deterministic embedded memory usage, but
// do not impose an additional six-camera application limit.
static constexpr uint8_t MAX_MULTI_GOPRO_SLOTS = 8;

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

    uint8_t connectedCount() const;
    uint8_t recordingCount() const;

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

    Slot _slots[MAX_MULTI_GOPRO_SLOTS];
    int8_t _scanSlot = -1;
    bool _scanning = false;
    bool _recordingRequested = false;
    uint32_t _recordStartedMs = 0;
    CameraData _camera{};

    static MultiGoProCamera *_instance;
};

inline bool MultiGoProCamera::prepareSharedScanSlot() {
    for (uint8_t i = 0; i < MAX_MULTI_GOPRO_SLOTS; ++i) {
        Slot &s = _slots[i];
        if (!s.ready && !s.bleConnected && !s.found) {
            _scanSlot = static_cast<int8_t>(i);
            return true;
        }
    }
    _scanSlot = -1;
    return false;
}

inline bool MultiGoProCamera::acceptSharedAdvertisement(BLEAdvertisedDevice device) {
    if (_scanSlot < 0 && !prepareSharedScanSlot()) return false;
    Slot &s = _slots[_scanSlot];
    const bool had = s.found;

    // Wake Guard is useful for unknown sleeping GoPros because it prevents an
    // accidental BLE connection from waking them. A camera already present in
    // our registry is different: in Multi Cam mode it is one we have explicitly
    // used before, so after a range drop we must be allowed to reacquire it even
    // if its reconnect advertisement uses a sleep/remote-wake state byte.
    bool knownCamera = false;
    if (_registry) {
        const std::string addr = device.getAddress().toString();
        uint8_t number = 0;
        char label[CAMREG_LABEL_LEN] = {};
        knownCamera = _registry->identityForAddr(addr.c_str(), number, label, sizeof(label));
    }
    const bool wakeGuardWas = _wakeGuard;
    if (knownCamera) _wakeGuard = false;
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
    for (const auto &s : _slots) {
        if (!s.ready && (s.found || s.bleConnected || s.pendingHwInfo ||
                         s.pendingRegisterSettings || s.pendingRegisterStatus))
            return true;
    }
    return false;
}
