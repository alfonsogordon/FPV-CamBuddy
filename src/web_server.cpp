#include "web_server.h"
#include "camera_registry.h"
#include "web_content.h"
#include "config.h"

#include <WiFi.h>
#include <ArduinoJson.h>

class StringStream : public Stream {
public:
    int available() override { return 0; }
    int read() override { return -1; }
    int peek() override { return -1; }
    size_t write(uint8_t c) override { _buf += (char)c; return 1; }
    size_t write(const uint8_t *b, size_t n) override { for (size_t i=0;i<n;i++) _buf += (char)b[i]; return n; }
    const String &str() const { return _buf; }
private:
    String _buf;
};

void WebConfigServer::begin(ConfigManager &cfg, CameraRegistry *reg, Stream *dbg) {
    _cfg=&cfg; _reg=reg; _dbg=dbg;
    if (_running) return;

    // Fresh AP baseline: one mode change, one SoftAP start, no scans, DNS,
    // captive portal, mDNS or radio reset/disconnect sequence.
    WiFi.mode(WIFI_AP);
    delay(100);
    WiFi.setSleep(false);
    WiFi.setTxPower(WIFI_POWER_19_5dBm);

    const bool ok=WiFi.softAP(WIFI_AP_SSID,
                              strlen(WIFI_AP_PASSWORD) ? WIFI_AP_PASSWORD : nullptr,
                              WIFI_AP_CHANNEL, 0, 2);
    if (!ok) {
        if (_dbg) _dbg->println("[wifi] FPV CamBuddy SoftAP start FAILED");
        _running=false;
        return;
    }

    delay(100);
    if (_dbg) {
        _dbg->printf("[wifi] FPV CamBuddy AP ready: %s  IP=%s  ch=%u  tx=%d\n",
                     WIFI_AP_SSID, WiFi.softAPIP().toString().c_str(),
                     WIFI_AP_CHANNEL, (int)WiFi.getTxPower());
    }

    _server.on("/", HTTP_GET, [this](){ handleRoot(); });
    _server.on("/api/config", HTTP_GET, [this](){ handleGetConfig(); });
    _server.on("/api/config", HTTP_POST, [this](){ handlePostConfig(); });
    _server.on("/api/cameras", HTTP_GET, [this](){ handleGetCameras(); });
    _server.on("/api/cli", HTTP_POST, [this](){ handleCli(); });
    _server.onNotFound([this](){ _server.sendHeader("Location", "/", true); _server.send(302, "text/plain", ""); });
    _server.begin();
    _running=true;
}

void WebConfigServer::stop() {
    if (!_running) return;
    _server.stop();
    WiFi.softAPdisconnect(true);
    _running=false;
    if (_dbg) _dbg->println("[wifi] FPV CamBuddy AP stopped");
}

void WebConfigServer::update() { if (_running) _server.handleClient(); }
void WebConfigServer::handleRoot() { _server.send_P(200,"text/html",WEB_INDEX_HTML); }

void WebConfigServer::handleGetConfig() {
    const auto &c=_cfg->config();
    JsonDocument d;
    d["camera_type"]=c.cameraType;
    CameraEntry ce;
    d["caddx_ssid"]=(_reg && _reg->preferredEntry(2,ce)) ? ce.addr : "";
    d["disarm_delay"]=c.disarmStopDelayMs;
    d["stop_on_disarm"]=c.stopOnDisarm;
    d["aux_channel"]=c.auxChannel;
    d["aux_mode"]=c.auxMode;
    d["camera_match"]=c.cameraMatchMode;
    d["wake_guard"]=c.cameraWakeGuard;
    d["debug_ble"]=c.debugBle;
    d["low_power"]=c.lowPowerMode;
    d["wifi_ap_enabled"]=c.wifiApEnabled;
    d["wifi_ap_delay"]=c.wifiApStartDelaySec;
    d["osd1"]=c.osd1Tpl; d["osd2"]=c.osd2Tpl; d["osd3"]=c.osd3Tpl; d["osd4"]=c.osd4Tpl;
    d["bf45_compat"]=c.bf45Compat;
    d["pilot_en"]=c.pilotNameEnabled; d["pilot_tpl"]=c.pilotNameTpl;
    d["craft_en"]=c.craftNameEnabled; d["craft_tpl"]=c.craftNameTpl;
    d["fpv_state_mode"]=c.fpvStateMode;
    d["fpv_error"]=c.fpvErrorEnabled; d["fpv_err_text"]=c.fpvErrorText;
    d["fpv_ready"]=c.fpvReadyEnabled; d["fpv_ready_text"]=c.fpvReadyText;
    d["fpv_record"]=c.fpvRecordingEnabled; d["fpv_record_text"]=c.fpvRecordingText; d["fpv_flash"]=c.fpvRecFlash;
    d["fpv_low_batt"]=c.fpvLowBatteryEnabled; d["fpv_low_pct"]=c.fpvLowBatteryPct; d["fpv_low_rdyflash"]=c.fpvLowBatteryReadyFlash; d["fpv_low_rectext"]=c.fpvLowBatteryRecText; d["fpv_low_text"]=c.fpvLowBatteryText;
    d["fpv_rect_warn"]=c.fpvLowRecTimeEnabled; d["fpv_rect_min"]=c.fpvLowRecTimeMin; d["fpv_rect_ready"]=c.fpvLowRecReadyWarning; d["fpv_rect_record"]=c.fpvLowRecRecordingWarning; d["fpv_rect_text"]=c.fpvLowRecTimeText;
    d["fpv_hot_warn"]=c.fpvHotWarningEnabled; d["fpv_hot_ready"]=c.fpvHotReadyWarning; d["fpv_hot_record"]=c.fpvHotRecordingWarning; d["fpv_hot_text"]=c.fpvHotWarningText;
    d["fpv_prearm"]=c.fpvPreArmReminderEnabled;
    d["fpv_prearm_text"]=c.fpvPreArmReminderText;
    d["fpv_prearm_show"]=c.fpvPreArmReminderShowMs;
    d["fpv_prearm_interval"]=c.fpvPreArmReminderIntervalMs;
    String s; serializeJson(d,s); _server.send(200,"application/json",s);
}

void WebConfigServer::handlePostConfig() {
    if (!_server.hasArg("plain")) { _server.send(400,"text/plain","No body"); return; }
    JsonDocument d; if (deserializeJson(d,_server.arg("plain"))) { _server.send(400,"text/plain","JSON parse error"); return; }
    if(d["camera_type"].is<int>()) _cfg->setCameraType(d["camera_type"].as<uint8_t>());
    if(d["caddx_ssid"].is<const char*>()) _cfg->setCaddxSsid(d["caddx_ssid"].as<const char*>());
    if(d["caddx_pass"].is<const char*>()) _cfg->setCaddxPass(d["caddx_pass"].as<const char*>());
    if(d["disarm_delay"].is<int>()) _cfg->setDisarmDelay(d["disarm_delay"].as<uint32_t>());
    if(d["stop_on_disarm"].is<bool>()) _cfg->setStopOnDisarm(d["stop_on_disarm"].as<bool>());
    if(d["aux_channel"].is<int>()) _cfg->setAuxChannel(d["aux_channel"].as<uint8_t>());
    if(d["aux_mode"].is<int>()) _cfg->setAuxMode(d["aux_mode"].as<uint8_t>());
    if(d["camera_match"].is<int>()) _cfg->setCameraMatchMode(d["camera_match"].as<uint8_t>());
    if(d["wake_guard"].is<bool>()) _cfg->setCameraWakeGuard(d["wake_guard"].as<bool>());
    if(d["debug_ble"].is<bool>()) _cfg->setDebugBle(d["debug_ble"].as<bool>());
    if(d["low_power"].is<bool>()) _cfg->setLowPowerMode(d["low_power"].as<bool>());
    if(d["wifi_ap_enabled"].is<bool>()) _cfg->setWifiApEnabled(d["wifi_ap_enabled"].as<bool>());
    if(d["wifi_ap_delay"].is<int>()) _cfg->setWifiApStartDelay(d["wifi_ap_delay"].as<uint32_t>());
    if(d["osd1"].is<const char*>()) _cfg->setOsdTemplate(1,d["osd1"].as<const char*>());
    if(d["osd2"].is<const char*>()) _cfg->setOsdTemplate(2,d["osd2"].as<const char*>());
    if(d["osd3"].is<const char*>()) _cfg->setOsdTemplate(3,d["osd3"].as<const char*>());
    if(d["osd4"].is<const char*>()) _cfg->setOsdTemplate(4,d["osd4"].as<const char*>());
    if(d["bf45_compat"].is<bool>()) _cfg->setBf45Compat(d["bf45_compat"].as<bool>());
    if(d["pilot_en"].is<bool>()) _cfg->setPilotNameEnabled(d["pilot_en"].as<bool>());
    if(d["pilot_tpl"].is<const char*>()) _cfg->setPilotNameTemplate(d["pilot_tpl"].as<const char*>());
    if(d["craft_en"].is<bool>()) _cfg->setCraftNameEnabled(d["craft_en"].as<bool>());
    if(d["craft_tpl"].is<const char*>()) _cfg->setCraftNameTemplate(d["craft_tpl"].as<const char*>());
    if(d["fpv_state_mode"].is<int>()) _cfg->setFpvStateMode(d["fpv_state_mode"].as<uint8_t>());
    if(d["fpv_error"].is<bool>()) _cfg->setFpvErrorEnabled(d["fpv_error"].as<bool>());
    if(d["fpv_err_text"].is<const char*>()) _cfg->setFpvErrorText(d["fpv_err_text"].as<const char*>());
    if(d["fpv_ready"].is<bool>()) _cfg->setFpvReadyEnabled(d["fpv_ready"].as<bool>());
    if(d["fpv_ready_text"].is<const char*>()) _cfg->setFpvReadyText(d["fpv_ready_text"].as<const char*>());
    if(d["fpv_record"].is<bool>()) _cfg->setFpvRecordingEnabled(d["fpv_record"].as<bool>());
    if(d["fpv_record_text"].is<const char*>()) _cfg->setFpvRecordingText(d["fpv_record_text"].as<const char*>());
    if(d["fpv_flash"].is<bool>()) _cfg->setFpvRecFlash(d["fpv_flash"].as<bool>());
    if(d["fpv_low_batt"].is<bool>()) _cfg->setFpvLowBatteryEnabled(d["fpv_low_batt"].as<bool>());
    if(d["fpv_low_pct"].is<int>()) _cfg->setFpvLowBatteryPct(d["fpv_low_pct"].as<uint8_t>());
    if(d["fpv_low_rdyflash"].is<bool>()) _cfg->setFpvLowBatteryReadyFlash(d["fpv_low_rdyflash"].as<bool>());
    if(d["fpv_low_rectext"].is<bool>()) _cfg->setFpvLowBatteryRecText(d["fpv_low_rectext"].as<bool>());
    if(d["fpv_low_text"].is<const char*>()) _cfg->setFpvLowBatteryText(d["fpv_low_text"].as<const char*>());
    if(d["fpv_rect_warn"].is<bool>()) _cfg->setFpvLowRecTimeEnabled(d["fpv_rect_warn"].as<bool>());
    if(d["fpv_rect_min"].is<int>()) _cfg->setFpvLowRecTimeMin(d["fpv_rect_min"].as<uint16_t>());
    if(d["fpv_rect_ready"].is<bool>()) _cfg->setFpvLowRecReadyWarning(d["fpv_rect_ready"].as<bool>());
    if(d["fpv_rect_record"].is<bool>()) _cfg->setFpvLowRecRecordingWarning(d["fpv_rect_record"].as<bool>());
    if(d["fpv_rect_text"].is<const char*>()) _cfg->setFpvLowRecTimeText(d["fpv_rect_text"].as<const char*>());
    if(d["fpv_hot_warn"].is<bool>()) _cfg->setFpvHotWarningEnabled(d["fpv_hot_warn"].as<bool>());
    if(d["fpv_hot_ready"].is<bool>()) _cfg->setFpvHotReadyWarning(d["fpv_hot_ready"].as<bool>());
    if(d["fpv_hot_record"].is<bool>()) _cfg->setFpvHotRecordingWarning(d["fpv_hot_record"].as<bool>());
    if(d["fpv_hot_text"].is<const char*>()) _cfg->setFpvHotWarningText(d["fpv_hot_text"].as<const char*>());
    if(d["fpv_prearm"].is<bool>()) _cfg->setFpvPreArmReminderEnabled(d["fpv_prearm"].as<bool>());
    if(d["fpv_prearm_text"].is<const char*>()) _cfg->setFpvPreArmReminderText(d["fpv_prearm_text"].as<const char*>());
    if(d["fpv_prearm_show"].is<int>()) _cfg->setFpvPreArmReminderShowMs(d["fpv_prearm_show"].as<uint16_t>());
    if(d["fpv_prearm_interval"].is<int>()) _cfg->setFpvPreArmReminderIntervalMs(d["fpv_prearm_interval"].as<uint16_t>());
    _server.send(200,"text/plain","OK");
}

void WebConfigServer::handleGetCameras(){ const String s=_reg?_reg->toJson():"[]"; _server.send(200,"application/json",s); }
void WebConfigServer::handleWifiScan(){ _server.send(501,"application/json","[]"); }
void WebConfigServer::handleCli(){
    if(!_server.hasArg("plain")){_server.send(400,"text/plain","No command");return;}
    String cmd=_server.arg("plain"); cmd.trim(); if(!cmd.length()){_server.send(400,"text/plain","Empty command");return;}
    StringStream out; _cfg->processCommand(cmd.c_str(),out); _server.send(200,"text/plain",out.str());
}
