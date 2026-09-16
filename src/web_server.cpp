#include "web_server.h"
#include "camera_registry.h"
#include "web_content.h"
#include "config.h"

#include <WiFi.h>
#include <ArduinoJson.h>

// Accumulates print() output into a String so CLI commands can be
// processed synchronously and their output returned as HTTP body text.
class StringStream : public Stream {
public:
    int    available() override { return 0; }
    int    read()      override { return -1; }
    int    peek()      override { return -1; }
    size_t write(uint8_t c) override { _buf += (char)c; return 1; }
    size_t write(const uint8_t *b, size_t n) override {
        for (size_t i = 0; i < n; i++) _buf += (char)b[i];
        return n;
    }
    const String &str() const { return _buf; }
private:
    String _buf;
};

// ── Public interface ──────────────────────────────────────────────────────────

void WebConfigServer::begin(ConfigManager &cfg, CameraRegistry *reg, Stream *dbg) {
    _cfg = &cfg;
    _reg = reg;
    _dbg = dbg;

    bool ok = WiFi.softAP(WIFI_AP_SSID, strlen(WIFI_AP_PASSWORD) ? WIFI_AP_PASSWORD : nullptr,
                          WIFI_AP_CHANNEL);

    if (_dbg) {
        if (ok) {
            _dbg->printf("[wifi] AP started — SSID: %s  IP: %s\n",
                         WIFI_AP_SSID, WiFi.softAPIP().toString().c_str());
        } else {
            _dbg->printf("[wifi] softAP() FAILED — mode=%d\n", (int)WiFi.getMode());
        }
    }

    // Wildcard DNS makes OS connectivity probes resolve to the C3. Combined
    // with the probe routes below this gives Android/iOS/Windows the normal
    // captive-portal prompt while keeping http://192.168.4.1/ as a fallback.
    _dns.start(53, "*", WiFi.softAPIP());

    _server.on("/",            HTTP_GET,  [this]() { handleRoot();        });
    _server.on("/generate_204", HTTP_GET, [this]() { handleCaptivePortal(); });
    _server.on("/gen_204",      HTTP_GET, [this]() { handleCaptivePortal(); });
    _server.on("/hotspot-detect.html", HTTP_GET, [this]() { handleCaptivePortal(); });
    _server.on("/library/test/success.html", HTTP_GET, [this]() { handleCaptivePortal(); });
    _server.on("/ncsi.txt", HTTP_GET, [this]() { handleCaptivePortal(); });
    _server.on("/connecttest.txt", HTTP_GET, [this]() { handleCaptivePortal(); });
    _server.on("/redirect", HTTP_GET, [this]() { handleCaptivePortal(); });
    _server.on("/api/config",  HTTP_GET,  [this]() { handleGetConfig();   });
    _server.on("/api/config",  HTTP_POST, [this]() { handlePostConfig();  });
    _server.on("/api/cameras", HTTP_GET,  [this]() { handleGetCameras();  });
    _server.on("/api/wifi_scan", HTTP_GET, [this]() { handleWifiScan();   });
    _server.on("/api/cli",     HTTP_POST, [this]() { handleCli();         });
    _server.onNotFound([this]() { handleCaptivePortal(); });

    _server.begin();
    _running = true;
}

void WebConfigServer::stop() {
    if (!_running) return;
    _dns.stop();
    _server.stop();
    WiFi.softAPdisconnect(true);
    _running = false;
    if (_dbg) _dbg->println("[wifi] AP stopped");
}

void WebConfigServer::update() {
    if (_running) {
        _dns.processNextRequest();
        _server.handleClient();
    }
}

// ── Route handlers ────────────────────────────────────────────────────────────

void WebConfigServer::handleRoot() {
    _server.send_P(200, "text/html", WEB_INDEX_HTML);
}

void WebConfigServer::handleCaptivePortal() {
    _server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    _server.sendHeader("Pragma", "no-cache");
    _server.sendHeader("Expires", "-1");
    _server.send_P(200, "text/html", WEB_INDEX_HTML);
}

void WebConfigServer::handleGetConfig() {
    const auto &c = _cfg->config();
    JsonDocument doc;
    doc["camera_type"]    = c.cameraType;
    CameraEntry caddxEntry;
    doc["caddx_ssid"] = (_reg && _reg->preferredEntry(/*Caddx=*/2, caddxEntry))
                       ? caddxEntry.addr : "";
    doc["disarm_delay"]   = c.disarmStopDelayMs;
    doc["stop_on_disarm"] = c.stopOnDisarm;
    doc["aux_channel"]    = c.auxChannel;
    doc["aux_mode"]       = c.auxMode;
    doc["camera_match"]   = c.cameraMatchMode;
    doc["wake_guard"]     = c.cameraWakeGuard;
    doc["low_power"]      = c.lowPowerMode;
    doc["wifi_ap_enabled"]  = c.wifiApEnabled;
    doc["wifi_ap_delay"]    = c.wifiApStartDelaySec;
    doc["osd1"]           = c.osd1Tpl;
    doc["osd2"]           = c.osd2Tpl;
    doc["osd3"]           = c.osd3Tpl;
    doc["osd4"]           = c.osd4Tpl;
    doc["bf45_compat"]    = c.bf45Compat;
    doc["pilot_en"]       = c.pilotNameEnabled;
    doc["pilot_tpl"]      = c.pilotNameTpl;
    doc["craft_en"]       = c.craftNameEnabled;
    doc["craft_tpl"]      = c.craftNameTpl;
    doc["fpv_state_mode"]  = c.fpvStateMode;
    doc["fpv_error"]       = c.fpvErrorEnabled;
    doc["fpv_err_text"]    = c.fpvErrorText;
    doc["fpv_ready"]       = c.fpvReadyEnabled;
    doc["fpv_ready_text"]  = c.fpvReadyText;
    doc["fpv_record"]      = c.fpvRecordingEnabled;
    doc["fpv_record_text"] = c.fpvRecordingText;
    doc["fpv_flash"]       = c.fpvRecFlash;
    doc["fpv_low_batt"]    = c.fpvLowBatteryEnabled;
    doc["fpv_low_pct"]     = c.fpvLowBatteryPct;
    doc["fpv_low_rdyflash"] = c.fpvLowBatteryReadyFlash;
    doc["fpv_low_rectext"] = c.fpvLowBatteryRecText;
    doc["fpv_low_text"]    = c.fpvLowBatteryText;
    doc["fpv_rect_warn"]   = c.fpvLowRecTimeEnabled;
    doc["fpv_rect_min"]    = c.fpvLowRecTimeMin;
    doc["fpv_rect_ready"]  = c.fpvLowRecReadyWarning;
    doc["fpv_rect_record"] = c.fpvLowRecRecordingWarning;
    doc["fpv_rect_text"]   = c.fpvLowRecTimeText;
    doc["fpv_hot_warn"]    = c.fpvHotWarningEnabled;
    doc["fpv_hot_ready"]   = c.fpvHotReadyWarning;
    doc["fpv_hot_record"]  = c.fpvHotRecordingWarning;
    doc["fpv_hot_text"]    = c.fpvHotWarningText;
    String json;
    serializeJson(doc, json);
    _server.send(200, "application/json", json);
}

void WebConfigServer::handlePostConfig() {
    if (!_server.hasArg("plain")) {
        _server.send(400, "text/plain", "No body");
        return;
    }
    JsonDocument doc;
    if (deserializeJson(doc, _server.arg("plain"))) {
        _server.send(400, "text/plain", "JSON parse error");
        return;
    }

    if (doc["camera_type"].is<int>())     _cfg->setCameraType(doc["camera_type"].as<uint8_t>());
    if (doc["caddx_ssid"].is<const char *>()) _cfg->setCaddxSsid(doc["caddx_ssid"].as<const char *>());
    if (doc["caddx_pass"].is<const char *>()) _cfg->setCaddxPass(doc["caddx_pass"].as<const char *>());
    if (doc["disarm_delay"].is<int>())    _cfg->setDisarmDelay(doc["disarm_delay"].as<uint32_t>());
    if (doc["stop_on_disarm"].is<bool>()) _cfg->setStopOnDisarm(doc["stop_on_disarm"].as<bool>());
    if (doc["aux_channel"].is<int>())     _cfg->setAuxChannel(doc["aux_channel"].as<uint8_t>());
    if (doc["aux_mode"].is<int>())        _cfg->setAuxMode(doc["aux_mode"].as<uint8_t>());
    if (doc["camera_match"].is<int>())    _cfg->setCameraMatchMode(doc["camera_match"].as<uint8_t>());
    if (doc["wake_guard"].is<bool>())     _cfg->setCameraWakeGuard(doc["wake_guard"].as<bool>());
    if (doc["low_power"].is<bool>())      _cfg->setLowPowerMode(doc["low_power"].as<bool>());
    if (doc["wifi_ap_enabled"].is<bool>()) _cfg->setWifiApEnabled(doc["wifi_ap_enabled"].as<bool>());
    if (doc["wifi_ap_delay"].is<int>())    _cfg->setWifiApStartDelay(doc["wifi_ap_delay"].as<uint32_t>());
    if (doc["osd1"].is<const char *>())   _cfg->setOsdTemplate(1, doc["osd1"].as<const char *>());
    if (doc["osd2"].is<const char *>())   _cfg->setOsdTemplate(2, doc["osd2"].as<const char *>());
    if (doc["osd3"].is<const char *>())   _cfg->setOsdTemplate(3, doc["osd3"].as<const char *>());
    if (doc["osd4"].is<const char *>())   _cfg->setOsdTemplate(4, doc["osd4"].as<const char *>());
    if (doc["bf45_compat"].is<bool>())    _cfg->setBf45Compat(doc["bf45_compat"].as<bool>());
    if (doc["pilot_en"].is<bool>())       _cfg->setPilotNameEnabled(doc["pilot_en"].as<bool>());
    if (doc["pilot_tpl"].is<const char *>()) _cfg->setPilotNameTemplate(doc["pilot_tpl"].as<const char *>());
    if (doc["craft_en"].is<bool>())       _cfg->setCraftNameEnabled(doc["craft_en"].as<bool>());
    if (doc["craft_tpl"].is<const char *>()) _cfg->setCraftNameTemplate(doc["craft_tpl"].as<const char *>());
    if (doc["fpv_state_mode"].is<bool>()) _cfg->setFpvStateMode(doc["fpv_state_mode"].as<bool>());
    if (doc["fpv_error"].is<bool>())       _cfg->setFpvErrorEnabled(doc["fpv_error"].as<bool>());
    if (doc["fpv_err_text"].is<const char *>()) _cfg->setFpvErrorText(doc["fpv_err_text"].as<const char *>());
    if (doc["fpv_ready"].is<bool>())       _cfg->setFpvReadyEnabled(doc["fpv_ready"].as<bool>());
    if (doc["fpv_ready_text"].is<const char *>()) _cfg->setFpvReadyText(doc["fpv_ready_text"].as<const char *>());
    if (doc["fpv_record"].is<bool>())      _cfg->setFpvRecordingEnabled(doc["fpv_record"].as<bool>());
    if (doc["fpv_record_text"].is<const char *>()) _cfg->setFpvRecordingText(doc["fpv_record_text"].as<const char *>());
    if (doc["fpv_flash"].is<bool>())       _cfg->setFpvRecFlash(doc["fpv_flash"].as<bool>());
    if (doc["fpv_low_batt"].is<bool>())    _cfg->setFpvLowBatteryEnabled(doc["fpv_low_batt"].as<bool>());
    if (doc["fpv_low_pct"].is<unsigned int>()) _cfg->setFpvLowBatteryPct((uint8_t)doc["fpv_low_pct"].as<unsigned int>());
    if (doc["fpv_low_rdyflash"].is<bool>()) _cfg->setFpvLowBatteryReadyFlash(doc["fpv_low_rdyflash"].as<bool>());
    if (doc["fpv_low_rectext"].is<bool>()) _cfg->setFpvLowBatteryRecText(doc["fpv_low_rectext"].as<bool>());
    if (doc["fpv_low_text"].is<const char *>()) _cfg->setFpvLowBatteryText(doc["fpv_low_text"].as<const char *>());
    if (doc["fpv_rect_warn"].is<bool>()) _cfg->setFpvLowRecTimeEnabled(doc["fpv_rect_warn"].as<bool>());
    if (doc["fpv_rect_min"].is<unsigned int>()) _cfg->setFpvLowRecTimeMin((uint16_t)doc["fpv_rect_min"].as<unsigned int>());
    if (doc["fpv_rect_ready"].is<bool>()) _cfg->setFpvLowRecReadyWarning(doc["fpv_rect_ready"].as<bool>());
    if (doc["fpv_rect_record"].is<bool>()) _cfg->setFpvLowRecRecordingWarning(doc["fpv_rect_record"].as<bool>());
    if (doc["fpv_rect_text"].is<const char *>()) _cfg->setFpvLowRecTimeText(doc["fpv_rect_text"].as<const char *>());
    if (doc["fpv_hot_warn"].is<bool>()) _cfg->setFpvHotWarningEnabled(doc["fpv_hot_warn"].as<bool>());
    if (doc["fpv_hot_ready"].is<bool>()) _cfg->setFpvHotReadyWarning(doc["fpv_hot_ready"].as<bool>());
    if (doc["fpv_hot_record"].is<bool>()) _cfg->setFpvHotRecordingWarning(doc["fpv_hot_record"].as<bool>());
    if (doc["fpv_hot_text"].is<const char *>()) _cfg->setFpvHotWarningText(doc["fpv_hot_text"].as<const char *>());

    handleGetConfig();
}

void WebConfigServer::handleGetCameras() {
    String json = _reg ? _reg->toJson() : "[]";
    _server.send(200, "application/json", json);
}

// Blocking scan (a few seconds) — acceptable for a manual "Scan" click.
// WIFI_MODE_APSTA keeps this server's own AP alive across the scan.
void WebConfigServer::handleWifiScan() {
    if (WiFi.getMode() != WIFI_MODE_APSTA) {
        WiFi.mode(WIFI_MODE_APSTA);
        delay(100);
    }
    int n = WiFi.scanNetworks();
    if (n < 0) {
        delay(200);
        n = WiFi.scanNetworks();
    }
    if (n < 0) n = 0;
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();
    for (int i = 0; i < n; i++) {
        JsonObject o = arr.add<JsonObject>();
        o["ssid"] = WiFi.SSID(i);
        o["rssi"] = WiFi.RSSI(i);
        o["open"] = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN);
    }
    WiFi.scanDelete();
    String json;
    serializeJson(doc, json);
    _server.send(200, "application/json", json);
}

void WebConfigServer::handleCli() {
    if (!_server.hasArg("plain")) {
        _server.send(400, "text/plain", "No body");
        return;
    }
    StringStream out;
    _cfg->processCommand(_server.arg("plain").c_str(), out);
    _server.send(200, "text/plain", out.str());
}
