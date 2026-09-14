#pragma once
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEClient.h>
#include <BLERemoteService.h>
#include <BLERemoteCharacteristic.h>
#include <BLEAdvertisedDevice.h>
#include <BLESecurity.h>
#include <esp_gap_ble_api.h>
#include "camera.h"
#include "sony_protocol.h"

class SonyCamera : public Camera,
                    public BLEAdvertisedDeviceCallbacks,
                    public BLEClientCallbacks,
                    public BLESecurityCallbacks {
public:
    void begin();
    void update();

    bool isConnected() const override { return _sonyConnected; }

    bool startRecording() override;
    bool stopRecording() override;
    bool switchCameraMode(uint8_t mode) override;

    bool acceptSharedAdvertisement(BLEAdvertisedDevice device);
    bool sharedConnectionPending() const {
        return !_sonyConnected && (_targetFound || _bleConnected || _secured || _pendingDiscover);
    }
    const std::string &sharedAddress() const { return _targetAddr; }

private:
    void onResult(BLEAdvertisedDevice device) override;
    void onConnect(BLEClient *client) override;
    void onDisconnect(BLEClient *client) override;

    uint32_t onPassKeyRequest() override;
    void     onPassKeyNotify(uint32_t pass_key) override;
    bool     onSecurityRequest() override;
    bool     onConfirmPIN(uint32_t pin) override;
    void     onAuthenticationComplete(esp_ble_auth_cmpl_t cmpl) override;

    void startScan();
    bool connectAndSetup();
    bool discoverServiceAndSubscribe();
    void discoverBatteryService();

    void sendCommandByte(uint8_t cmd);
    void sendButtonPress(uint8_t downCmd, uint8_t upCmd);

    void handleNotification(uint8_t *data, size_t len);
    void handleBatteryNotification(uint8_t *data, size_t len);

    static void notifyCallback(BLERemoteCharacteristic *pChar,
                                uint8_t *data, size_t len, bool isNotify);
    static void batteryNotifyCallback(BLERemoteCharacteristic *pChar,
                                       uint8_t *data, size_t len, bool isNotify);
    static void scanDoneCallback(BLEScanResults results);

    BLEClient                *_client       = nullptr;
    BLERemoteCharacteristic  *_cmdChar      = nullptr;
    std::string               _targetAddr;
    std::string               _targetName;
    esp_ble_addr_type_t       _targetType   = BLE_ADDR_TYPE_PUBLIC;
    bool                      _targetFound  = false;
    bool                      _bleConnected = false;
    bool                      _secured      = false;
    bool                      _sonyConnected = false;
    bool                      _scanning     = false;
    uint32_t                  _lastAttemptMs = 0;

    std::string               _candidateAddr;
    std::string               _candidateName;
    esp_ble_addr_type_t       _candidateType = BLE_ADDR_TYPE_PUBLIC;

    std::string               _bestAddr;
    std::string               _bestName;
    esp_ble_addr_type_t       _bestType = BLE_ADDR_TYPE_PUBLIC;
    int8_t                    _bestRssi = -128;

    bool                      _pendingDiscover = false;

    CameraData      _camera{};

    static SonyCamera *_instance;
};

inline bool SonyCamera::acceptSharedAdvertisement(BLEAdvertisedDevice device) {
    if (_sonyConnected || _bleConnected || _targetFound) return false;
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
