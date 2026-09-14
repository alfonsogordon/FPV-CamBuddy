#pragma once
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEClient.h>
#include <BLERemoteService.h>
#include <BLERemoteCharacteristic.h>
#include <BLEAdvertisedDevice.h>
#include "camera.h"
#include "insta360_protocol.h"

class Insta360Camera : public Camera,
                        public BLEAdvertisedDeviceCallbacks,
                        public BLEClientCallbacks {
public:
    void begin();
    void update();

    bool isConnected() const override { return _instaConnected; }

    bool startRecording() override;
    bool stopRecording() override;
    bool switchCameraMode(uint8_t mode) override;

    bool acceptSharedAdvertisement(BLEAdvertisedDevice device);
    bool sharedConnectionPending() const {
        return !_instaConnected && (_targetFound || _bleConnected);
    }
    const std::string &sharedAddress() const { return _targetAddr; }

private:
    void onResult(BLEAdvertisedDevice device) override;
    void onConnect(BLEClient *client) override;
    void onDisconnect(BLEClient *client) override;

    void startScan();
    bool connectAndSetup();
    void discoverBatteryService();
    bool sendCommand(uint16_t cmd);

    void handleNotification(uint8_t *data, size_t len);
    void handleBatteryNotification(uint8_t *data, size_t len);

    static void notifyCallback(BLERemoteCharacteristic *pChar,
                                uint8_t *data, size_t len, bool isNotify);
    static void batteryNotifyCallback(BLERemoteCharacteristic *pChar,
                                       uint8_t *data, size_t len, bool isNotify);
    static void scanDoneCallback(BLEScanResults results);

    BLEClient               *_client    = nullptr;
    BLERemoteCharacteristic *_writeChar = nullptr;

    std::string          _targetAddr;
    std::string          _targetName;
    esp_ble_addr_type_t  _targetType     = BLE_ADDR_TYPE_PUBLIC;
    bool                 _targetFound    = false;
    bool                 _bleConnected   = false;
    bool                 _instaConnected = false;
    bool                 _scanning       = false;
    uint32_t             _lastAttemptMs  = 0;
    uint32_t             _seq            = INSTA_SEQ_START;

    std::string          _candidateAddr;
    std::string          _candidateName;
    esp_ble_addr_type_t  _candidateType = BLE_ADDR_TYPE_PUBLIC;

    std::string          _bestAddr;
    std::string          _bestName;
    esp_ble_addr_type_t  _bestType = BLE_ADDR_TYPE_PUBLIC;
    int8_t               _bestRssi = -128;

    bool _awaitingRecordAck   = false;
    bool _pendingRecordTarget = false;

    CameraData _camera{};

    static Insta360Camera *_instance;
};

inline bool Insta360Camera::acceptSharedAdvertisement(BLEAdvertisedDevice device) {
    if (_instaConnected || _bleConnected || _targetFound) return false;
    const bool hadCandidate = !_candidateAddr.empty() || !_bestAddr.empty();
    onResult(device);
    if (_targetFound) return true;
    if (!hadCandidate && !_candidateAddr.empty()) {
        _targetAddr = _candidateAddr;
        _targetName = _candidateName;
        _targetType = _candidateType;
        _targetFound = true;
        _scanning = false;
        return true;
    }
    if (!hadCandidate && !_bestAddr.empty()) {
        _targetAddr = _bestAddr;
        _targetName = _bestName;
        _targetType = _bestType;
        _targetFound = true;
        _scanning = false;
        return true;
    }
    return false;
}
