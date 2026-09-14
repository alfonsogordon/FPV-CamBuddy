#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include "camera.h"
#include "caddx_protocol.h"

class CaddxCamera : public Camera {
public:
    void begin();
    void update();

    bool isConnected() const override { return _connected; }

    bool startRecording() override;
    bool stopRecording() override;
    bool switchCameraMode(uint8_t mode) override;

    // CameraRegistry stores the Caddx Wi-Fi SSID in the same identity field
    // used for BLE addresses. Expose it so Multi Cam can report the same
    // CONNECTED/OFFLINE state in the Cameras page for Wi-Fi cameras too.
    const String &liveAddress() const { return _ssid; }

private:
    enum class RecordVariant : uint8_t { kUnknown, kRecordCgi, kRecord2Cgi };

    void pollDeviceAttr();
    void pollAllInfo();
    void pollAllInfoSs(const String &body);
    void pollAllInfoLegacy(const String &body);
    void pollBattery();
    void pollSdState();

    int httpGet(const String &path, String &body);

    String baseUrl() const { return "http://" + _ip.toString() + CADDX_CGI_PATH; }

    uint8_t mapWorkModeToDji(long workMode) const;
    bool mapSsWorkModeToDji(const String &name, uint8_t &out) const;

    String   _ssid;
    String   _password;
    IPAddress _ip;

    bool _wifiWasConnected = false;
    bool _connected         = false;
    bool _attrProbed        = false;

    bool     _usingDefaultPass    = false;
    bool     _passWarningPrinted  = false;
    uint32_t _connectStartMs      = 0;

    RecordVariant _recordVariant = RecordVariant::kUnknown;
    bool          _isNewApp      = false;

    uint32_t _lastPollMs   = 0;
    uint8_t  _pollPhase    = 0;

    CameraData _camera{};
};
