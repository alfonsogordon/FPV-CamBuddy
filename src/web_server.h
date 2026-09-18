#pragma once

#include <WebServer.h>
#include "config_manager.h"

class CameraRegistry;  // forward declaration

// WiFi AP + HTTP configuration server.
//
// Call begin() to start the AP and web server.
// Call update() every loop iteration while running.
// Call stop() when a camera connects.
class WebConfigServer {
public:
    void begin(ConfigManager &cfg, CameraRegistry *reg, Stream *dbg = nullptr);
    void stop();
    bool isRunning() const { return _running; }
    void update();

private:
    WebServer       _server{80};
    ConfigManager  *_cfg     = nullptr;
    CameraRegistry *_reg     = nullptr;
    Stream         *_dbg     = nullptr;
    bool            _running = false;
    int             _lastStations = -1;
    bool            _apLostLogged = false;
    uint32_t        _lastHealthMs = 0;
    uint32_t        _lastProbeLogMs = 0;
    uint32_t        _lastProbeCountLogged = 0;
    bool            _firstHttpLogged = false;
    uint16_t        _cliSetCount = 0;
    uint32_t        _cliSaveStartMs = 0;
    String          _cliKeys;

    void handleRoot();
    void handleGetConfig();
    void handlePostConfig();
    void handleGetCameras();
    void handleWifiScan();
    void handleCli();
    void handleDiag();
};
