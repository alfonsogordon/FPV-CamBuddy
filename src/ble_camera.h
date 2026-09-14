#pragma once
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEClient.h>
#include <BLERemoteService.h>
#include <BLERemoteCharacteristic.h>
#include <BLEAdvertisedDevice.h>
#include <esp_gap_ble_api.h>
#include <esp_mac.h>
#include "camera.h"
#include "dji_protocol.h"

class BLECamera : public Camera,
                  public BLEAdvertisedDeviceCallbacks,
                  public BLEClientCallbacks {
public:
    void begin() override;
    void update() override;

    bool isConnected() const override { return _djiConnected; }

    bool startRecording() override;
    bool stopRecording() override;
    bool switchCameraMode(uint8_t mode) override;

    bool acceptSharedAdvertisement(BLEAdvertisedDevice device);
    bool sharedConnectionPending() const {
        return !_djiConnected && (_targetFound || _bleConnected || _pendingConnectAck);
    }
    const std::string &sharedAddress() const { return _targetAddr; }

private:
    void onResult(BLEAdvertisedDevice device) override;
    void onConnect(BLEClient *client) override;
    void onDisconnect(BLEClient *client) override;

    void startScan();
    bool connectAndSetup();
    bool sendConnectionRequest();
    bool sendStatusSubscription();

    bool sendFrame(uint8_t cmd_set, uint8_t cmd_id, uint8_t cmd_type,
                   const uint8_t *payload, uint16_t len,
                   bool with_rsp = false, int32_t override_seq = -1);

    void handleNotification(uint8_t *data, size_t length);
    void dispatchFrame(uint8_t cmd_type, uint8_t cmd_set, uint8_t cmd_id,
                       uint16_t seq, const uint8_t *payload, uint16_t payload_len);

    void handleConnectResponse(const uint8_t *payload, uint16_t len);
    void handleConnectCommand(uint16_t camSeq, const uint8_t *payload, uint16_t len);
    void handleCameraStatus(const uint8_t *payload, uint16_t len);
    void handleNewCameraStatus(const uint8_t *payload, uint16_t len);
    void handleRecordAck(const uint8_t *payload, uint16_t len);
    void handleModeSwitchAck(const uint8_t *payload, uint16_t len);

    void printServices();

    static void notifyCallback(BLERemoteCharacteristic *pChar,
                                uint8_t *pData, size_t length, bool isNotify);
    static void scanDoneCallback(BLEScanResults results);

    BLEClient                *_client       = nullptr;
    BLERemoteCharacteristic  *_writeChar    = nullptr;
    std::string               _targetAddr;
    std::string               _targetName;
    esp_ble_addr_type_t       _targetType   = BLE_ADDR_TYPE_PUBLIC;
    bool                      _targetFound  = false;
    bool                      _bleConnected = false;
    bool                      _djiConnected = false;
    bool                      _scanning     = false;
    uint16_t                  _seq          = 0;
    uint32_t                  _lastAttemptMs = 0;

    std::string               _candidateAddr;
    std::string               _candidateName;
    esp_ble_addr_type_t       _candidateType = BLE_ADDR_TYPE_PUBLIC;

    std::string               _bestAddr;
    std::string               _bestName;
    esp_ble_addr_type_t       _bestType = BLE_ADDR_TYPE_PUBLIC;
    int8_t                    _bestRssi = -128;

    bool                      _pendingConnectAck = false;
    uint16_t                  _pendingAckSeq     = 0;

    uint32_t        _deviceId  = 0;
    CameraData      _camera{};

    static BLECamera *_instance;
};

inline bool BLECamera::acceptSharedAdvertisement(BLEAdvertisedDevice device) {
    if (_djiConnected || _bleConnected || _targetFound) return false;
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
