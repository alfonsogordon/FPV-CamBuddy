#pragma once
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEClient.h>
#include <BLERemoteService.h>
#include <BLERemoteCharacteristic.h>
#include <BLEAdvertisedDevice.h>
#include "camera.h"
#include "gopro_protocol.h"

// Experimental minimal dual-GoPro backend used by Multi Cam Sync.
// It deliberately focuses on the feature we need to prove first: maintain up
// to two independent encrypted Open GoPro BLE sessions and fan record/stop
// commands out to every connected camera. Rich telemetry remains a follow-up;
// the primary single-camera GoPro backend is left untouched when Multi Cam is
// disabled.
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

private:
    struct Slot {
        BLEClient *client = nullptr;
        BLERemoteCharacteristic *cmdWrite = nullptr;
        BLERemoteCharacteristic *cmdNotify = nullptr;
        std::string addr;
        std::string name;
        esp_ble_addr_type_t addrType = BLE_ADDR_TYPE_PUBLIC;
        bool found = false;
        bool bleConnected = false;
        bool ready = false;
        bool pendingHwInfo = false;
        uint32_t lastAttemptMs = 0;
        GpRxAssembler rx;
    };

    void onResult(BLEAdvertisedDevice device) override;
    void onConnect(BLEClient *client) override;
    void onDisconnect(BLEClient *client) override;

    void startScan(uint8_t slot);
    bool connectSlot(uint8_t slot);
    void sendHardwareInfo(uint8_t slot);
    bool sendShutter(uint8_t slot, bool on);
    void publishState();
    bool addressAlreadyUsed(const std::string &addr) const;
    int slotForClient(BLEClient *client) const;
    int slotForNotify(BLERemoteCharacteristic *ch) const;
    void handleCmdNotify(uint8_t slot, uint8_t *data, size_t len);

    static void scanDoneCallback(BLEScanResults results);
    static void cmdNotifyCallback(BLERemoteCharacteristic *ch,
                                  uint8_t *data, size_t len, bool isNotify);

    Slot _slots[2];
    int8_t _scanSlot = -1;
    bool _scanning = false;
    bool _recordingRequested = false;
    uint32_t _recordStartedMs = 0;
    CameraData _camera{};

    static MultiGoProCamera *_instance;
};
