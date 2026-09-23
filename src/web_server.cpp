#include "web_server.h"
#include "camera_registry.h"
#include "web_content.h"
#include "config.h"
#include "diag_log.h"

#include <WiFi.h>
#include <ESPmDNS.h>
#include <BLEDevice.h>
#include <ArduinoJson.h>
#include <cstring>
#include <esp_wifi.h>
#include <esp_heap_caps.h>
#include <freertos/FreeRTOS.h>
#include <freertos/portmacro.h>

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

// ── AP radio diagnostics ─────────────────────────────────────────────────────
// Wi-Fi event callbacks run on the network task. Keep those callbacks tiny:
// copy compact event data into a fixed ring buffer, then persist/print from the
// normal loop in WebConfigServer::update().
enum RadioDiagType : uint8_t {
    RD_AP_START = 1,
    RD_AP_STOP,
    RD_STA_CONNECTED,
    RD_STA_DISCONNECTED,
    RD_IP_ASSIGNED,
    RD_MGMT_AUTH,
    RD_MGMT_ASSOC,
    RD_MGMT_REASSOC,
    RD_MGMT_DEAUTH,
    RD_MGMT_DISASSOC
};

struct RadioDiagEvent {
    uint32_t ms;
    uint8_t type;
    uint8_t mac[6];
    int8_t rssi;
    uint8_t channel;
    uint16_t a;
    uint16_t b;
    uint32_t extra;
};

static portMUX_TYPE s_radioDiagMux = portMUX_INITIALIZER_UNLOCKED;
static RadioDiagEvent s_radioDiagQ[32];
static volatile uint8_t s_radioDiagHead = 0;
static volatile uint8_t s_radioDiagTail = 0;
static volatile uint32_t s_radioDiagDropped = 0;
static volatile uint32_t s_probeCount = 0;
static volatile int8_t s_lastProbeRssi = -127;
static volatile int8_t s_bestProbeRssi = -127;
static uint8_t s_lastProbeMac[6] = {};
static uint8_t s_apMac[6] = {};
static bool s_wifiEventsInstalled = false;
static wifi_event_id_t s_wifiEventHandle = 0;
static bool s_promiscEnabled = false;

static void queueRadioDiag(uint8_t type, const uint8_t *mac = nullptr,
                           int8_t rssi = 0, uint8_t channel = 0,
                           uint16_t a = 0, uint16_t b = 0, uint32_t extra = 0) {
    RadioDiagEvent e{};
    e.ms = millis();
    e.type = type;
    if (mac) memcpy(e.mac, mac, 6);
    e.rssi = rssi;
    e.channel = channel;
    e.a = a;
    e.b = b;
    e.extra = extra;

    portENTER_CRITICAL(&s_radioDiagMux);
    const uint8_t next = (uint8_t)((s_radioDiagHead + 1U) % 32U);
    if (next == s_radioDiagTail) {
        s_radioDiagDropped++;
    } else {
        s_radioDiagQ[s_radioDiagHead] = e;
        s_radioDiagHead = next;
    }
    portEXIT_CRITICAL(&s_radioDiagMux);
}

static bool popRadioDiag(RadioDiagEvent &e) {
    bool ok = false;
    portENTER_CRITICAL(&s_radioDiagMux);
    if (s_radioDiagTail != s_radioDiagHead) {
        e = s_radioDiagQ[s_radioDiagTail];
        s_radioDiagTail = (uint8_t)((s_radioDiagTail + 1U) % 32U);
        ok = true;
    }
    portEXIT_CRITICAL(&s_radioDiagMux);
    return ok;
}

static const char *mgmtReasonName(uint16_t reason) {
    switch (reason) {
        case 1:  return "unspecified";
        case 2:  return "previous-auth-no-longer-valid";
        case 3:  return "station-leaving";
        case 4:  return "inactivity";
        case 5:  return "AP-unable-to-handle";
        case 6:  return "class2-frame-from-unauth";
        case 7:  return "class3-frame-from-unassoc";
        case 8:  return "station-leaving-BSS";
        case 15: return "4way-handshake-timeout";
        case 16: return "group-key-timeout";
        case 17: return "IE-mismatch";
        case 23: return "8021x-auth-failed";
        default: return "other";
    }
}

static void wifiPromiscRx(void *buf, wifi_promiscuous_pkt_type_t type) {
    if (!buf || type != WIFI_PKT_MGMT) return;
    const auto *pkt = reinterpret_cast<const wifi_promiscuous_pkt_t *>(buf);
    const uint8_t *f = pkt->payload;
    const uint16_t len = pkt->rx_ctrl.sig_len;
    if (len < 24) return;

    // For received management traffic addressed to our AP:
    // addr1=destination/BSSID, addr2=station source.
    if (memcmp(f + 4, s_apMac, 6) != 0) return;

    const uint8_t subtype = (uint8_t)((f[0] >> 4) & 0x0F);
    const uint8_t *src = f + 10;
    const int8_t rssi = pkt->rx_ctrl.rssi;
    const uint8_t channel = pkt->rx_ctrl.channel;
    auto le16 = [](const uint8_t *p) -> uint16_t {
        return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
    };

    switch (subtype) {
        case 0: // association request: capability, listen interval
            if (len >= 28) queueRadioDiag(RD_MGMT_ASSOC, src, rssi, channel, le16(f+24), le16(f+26));
            break;
        case 2: // reassociation request
            if (len >= 28) queueRadioDiag(RD_MGMT_REASSOC, src, rssi, channel, le16(f+24), le16(f+26));
            break;
        case 10: // disassociation: reason code
            if (len >= 26) queueRadioDiag(RD_MGMT_DISASSOC, src, rssi, channel, le16(f+24));
            break;
        case 11: // authentication: algorithm, sequence, status
            if (len >= 30) queueRadioDiag(RD_MGMT_AUTH, src, rssi, channel,
                                           le16(f+24), le16(f+26), le16(f+28));
            break;
        case 12: // deauthentication: reason code
            if (len >= 26) queueRadioDiag(RD_MGMT_DEAUTH, src, rssi, channel, le16(f+24));
            break;
        default:
            break;
    }
}

static void installWifiEventDiagnostics() {
    if (s_wifiEventsInstalled) return;
    s_wifiEventHandle = WiFi.onEvent([](arduino_event_id_t event, arduino_event_info_t info) {
        switch (event) {
            case ARDUINO_EVENT_WIFI_AP_START:
                queueRadioDiag(RD_AP_START);
                break;
            case ARDUINO_EVENT_WIFI_AP_STOP:
                queueRadioDiag(RD_AP_STOP);
                break;
            case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
                queueRadioDiag(RD_STA_CONNECTED, info.wifi_ap_staconnected.mac,
                               0, 0, info.wifi_ap_staconnected.aid);
                break;
            case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
                queueRadioDiag(RD_STA_DISCONNECTED, info.wifi_ap_stadisconnected.mac,
                               0, 0, info.wifi_ap_stadisconnected.aid);
                break;
            case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
                queueRadioDiag(RD_IP_ASSIGNED, nullptr, 0, 0, 0, 0,
                               info.wifi_ap_staipassigned.ip.addr);
                break;
            case ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED:
                portENTER_CRITICAL(&s_radioDiagMux);
                s_probeCount++;
                s_lastProbeRssi = info.wifi_ap_probereqrecved.rssi;
                if (s_lastProbeRssi > s_bestProbeRssi) s_bestProbeRssi = s_lastProbeRssi;
                memcpy(s_lastProbeMac, info.wifi_ap_probereqrecved.mac, 6);
                portEXIT_CRITICAL(&s_radioDiagMux);
                break;
            default:
                break;
        }
    });
    s_wifiEventsInstalled = true;
}

static void formatMac(char *out, size_t n, const uint8_t mac[6]) {
    snprintf(out, n, "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

static void logRadioSnapshot(const char *tag) {
    wifi_mode_t mode = WIFI_MODE_NULL;
    uint8_t primary = 0;
    wifi_second_chan_t second = WIFI_SECOND_CHAN_NONE;
    int8_t txQuarterDbm = 0;
    wifi_ps_type_t ps = WIFI_PS_NONE;
    wifi_country_t country{};
    wifi_config_t apCfg{};
    wifi_sta_list_t staList{};

    const esp_err_t modeRc = esp_wifi_get_mode(&mode);
    const esp_err_t chRc = esp_wifi_get_channel(&primary, &second);
    const esp_err_t txRc = esp_wifi_get_max_tx_power(&txQuarterDbm);
    const esp_err_t psRc = esp_wifi_get_ps(&ps);
    const esp_err_t countryRc = esp_wifi_get_country(&country);
    const esp_err_t cfgRc = esp_wifi_get_config(WIFI_IF_AP, &apCfg);
    const esp_err_t staRc = esp_wifi_ap_get_sta_list(&staList);

    diagLog("%s RADIO mode=%d(rc=%d) ch=%u/sec=%d(rc=%d) tx_qdbm=%d=%.1fdBm(rc=%d) ps=%d(rc=%d) country=%.2s(rc=%d)",
            tag, (int)mode, (int)modeRc, (unsigned)primary, (int)second, (int)chRc,
            (int)txQuarterDbm, ((float)txQuarterDbm)/4.0f, (int)txRc,
            (int)ps, (int)psRc, country.cc, (int)countryRc);

    char apMacStr[20];
    formatMac(apMacStr, sizeof(apMacStr), s_apMac);
    diagLog("%s AP mac=%s ip=%s ssid='%.*s' ssid_len=%u auth=%d hidden=%u max_conn=%u beacon=%u cfg_rc=%d sta_count=%u sta_rc=%d",
            tag, apMacStr, WiFi.softAPIP().toString().c_str(),
            cfgRc == ESP_OK ? (int)apCfg.ap.ssid_len : 0,
            cfgRc == ESP_OK ? (const char *)apCfg.ap.ssid : "",
            cfgRc == ESP_OK ? (unsigned)apCfg.ap.ssid_len : 0,
            cfgRc == ESP_OK ? (int)apCfg.ap.authmode : -1,
            cfgRc == ESP_OK ? (unsigned)apCfg.ap.ssid_hidden : 0,
            cfgRc == ESP_OK ? (unsigned)apCfg.ap.max_connection : 0,
            cfgRc == ESP_OK ? (unsigned)apCfg.ap.beacon_interval : 0,
            (int)cfgRc,
            staRc == ESP_OK ? (unsigned)staList.num : 0,
            (int)staRc);

    diagLog("%s MEM heap=%u min8=%u largest8=%u stations_api=%u tx_enum=%d",
            tag,
            (unsigned)ESP.getFreeHeap(),
            (unsigned)heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT),
            (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT),
            (unsigned)WiFi.softAPgetStationNum(),
            (int)WiFi.getTxPower());

    if (staRc == ESP_OK) {
        for (int i = 0; i < staList.num && i < 4; ++i) {
            char staMac[20];
            formatMac(staMac, sizeof(staMac), staList.sta[i].mac);
            diagLog("%s STA[%d] mac=%s rssi=%d phy11b=%u 11g=%u 11n=%u lr=%u",
                    tag, i, staMac, (int)staList.sta[i].rssi,
                    (unsigned)staList.sta[i].phy_11b,
                    (unsigned)staList.sta[i].phy_11g,
                    (unsigned)staList.sta[i].phy_11n,
                    (unsigned)staList.sta[i].phy_lr);
        }
    }
}

static void enablePromiscDiagnostics() {
    if (s_promiscEnabled) return;
    if (esp_wifi_get_mac(WIFI_IF_AP, s_apMac) != ESP_OK) memset(s_apMac, 0, sizeof(s_apMac));

    wifi_promiscuous_filter_t filter{};
    filter.filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT;
    const esp_err_t filterRc = esp_wifi_set_promiscuous_filter(&filter);
    const esp_err_t cbRc = esp_wifi_set_promiscuous_rx_cb(wifiPromiscRx);
    const esp_err_t enRc = esp_wifi_set_promiscuous(true);
    s_promiscEnabled = (enRc == ESP_OK);
    diagLog("AP sniffer mgmt-only filter_rc=%d cb_rc=%d enable_rc=%d enabled=%d",
            (int)filterRc, (int)cbRc, (int)enRc, s_promiscEnabled ? 1 : 0);
}

static void disablePromiscDiagnostics() {
    if (!s_promiscEnabled) return;
    esp_wifi_set_promiscuous(false);
    s_promiscEnabled = false;
}

void WebConfigServer::begin(ConfigManager &cfg, CameraRegistry *reg, Stream *dbg) {
    _cfg = &cfg;
    _reg = reg;
    _dbg = dbg;
    if (_running) return;

    // Enter a dedicated configuration mode. Once the AP is requested we no
    // longer need a live camera connection. The C3's shared 2.4 GHz radio gets
    // a deliberate quiet period before SoftAP starts so the first phone
    // association cannot race BLE/STA shutdown.
    if (_dbg) _dbg->println("[wifi] Entering dedicated configuration mode");
    installWifiEventDiagnostics();

    portENTER_CRITICAL(&s_radioDiagMux);
    s_probeCount = 0;
    s_lastProbeRssi = -127;
    s_bestProbeRssi = -127;
    memset(s_lastProbeMac, 0, sizeof(s_lastProbeMac));
    portEXIT_CRITICAL(&s_radioDiagMux);

    diagLog("AP begin camera_type=%u heap=%u mode_before=%d stations_before=%u",
            (unsigned)cfg.config().cameraType, (unsigned)ESP.getFreeHeap(),
            (int)WiFi.getMode(), (unsigned)WiFi.softAPgetStationNum());

    if (cfg.config().cameraType == 2) {
        // Caddx is the Wi-Fi camera backend. Drop its STA connection without
        // erasing saved credentials; ConfigManager/registry retains them.
        WiFi.setAutoReconnect(false);
        WiFi.disconnect(false, false);
        diagLog("AP radio: Caddx STA disconnect requested mode=%d", (int)WiFi.getMode());
        delay(300);
    } else {
        // All other current camera backends use BLE. Stop scanning and release
        // the BLE controller before handing the shared radio to Wi-Fi.
        BLEDevice::getScan()->stop();
        BLEDevice::deinit(true);
        diagLog("AP radio: BLE scan stopped + deinit complete");
        delay(500);
    }

    // Start from a genuinely cold Wi-Fi state. The previous 100 ms transition
    // was short enough for the AP to advertise and then fall over on the first
    // association on some ESP32-C3 boards.
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
    diagLog("AP radio: WiFi OFF mode=%d", (int)WiFi.getMode());
    delay(500);

    WiFi.mode(WIFI_AP);
    WiFi.setSleep(false);
    diagLog("AP radio: WIFI_AP mode requested mode=%d", (int)WiFi.getMode());
    delay(500);

    const IPAddress apIp(192, 168, 4, 1);
    const IPAddress apGw(192, 168, 4, 1);
    const IPAddress apMask(255, 255, 255, 0);
    if (!WiFi.softAPConfig(apIp, apGw, apMask)) {
        if (_dbg) _dbg->println("[wifi] FPV CamBuddy softAPConfig failed");
        diagLog("AP FAIL softAPConfig mode=%d heap=%u", (int)WiFi.getMode(), (unsigned)ESP.getFreeHeap());
        WiFi.mode(WIFI_OFF);
        return;
    }

    const bool ok = WiFi.softAP(WIFI_AP_SSID,
                                strlen(WIFI_AP_PASSWORD) ? WIFI_AP_PASSWORD : nullptr,
                                WIFI_AP_CHANNEL,
                                0,
                                4);
    if (!ok) {
        if (_dbg) _dbg->println("[wifi] FPV CamBuddy SoftAP start FAILED");
        diagLog("AP FAIL softAP() mode=%d heap=%u", (int)WiFi.getMode(), (unsigned)ESP.getFreeHeap());
        WiFi.mode(WIFI_OFF);
        return;
    }

    // Apply AP transmit power only after the Wi-Fi driver and AP are active.
    // This setting is intentionally independent from camera low-power mode.
    // V1.0.2 AP reliability test: use 8.5 dBm instead of maximum C3 TX power.
    const bool txOk = WiFi.setTxPower(WIFI_POWER_8_5dBm);
    diagLog("AP softAP started tx_request=8.5dBm tx_set_ok=%d tx_enum=%d mode=%d",
            txOk ? 1 : 0, (int)WiFi.getTxPower(), (int)WiFi.getMode());

    // Capture incoming 802.11 management traffic addressed to this AP. This is
    // diagnostic-only and lets us distinguish probe/auth/association failures
    // from DHCP/HTTP failures, including client-sent deauth/disassoc reason codes.
    enablePromiscDiagnostics();

    // Do not accept HTTP clients until the AP interface has a valid address and
    // has had time to settle. This specifically protects the first association.
    const uint32_t readyStart = millis();
    while ((WiFi.softAPIP()[0] == 0) && (millis() - readyStart < 2500UL)) {
        delay(50);
    }
    if (WiFi.softAPIP()[0] == 0) {
        if (_dbg) _dbg->println("[wifi] FPV CamBuddy AP failed to obtain IP");
        diagLog("AP FAIL no IP mode=%d stations=%u", (int)WiFi.getMode(), (unsigned)WiFi.softAPgetStationNum());
        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_OFF);
        return;
    }
    delay(500);

    _server.on("/", HTTP_GET, [this](){ handleRoot(); });
    _server.on("/api/config", HTTP_GET, [this](){ handleGetConfig(); });
    _server.on("/api/config", HTTP_POST, [this](){ handlePostConfig(); });
    _server.on("/api/cameras", HTTP_GET, [this](){ handleGetCameras(); });
    _server.on("/api/cli", HTTP_POST, [this](){ handleCli(); });
    _server.on("/api/diag", HTTP_GET, [this](){ handleDiag(); });
    _server.onNotFound([this](){
        _server.sendHeader("Location", "/", true);
        _server.send(302, "text/plain", "");
    });
    _server.begin();

    // Friendly local name for the field configurator. mDNS is optional:
    // the fixed AP address remains available even on clients that do not
    // resolve .local names.
    const bool mdnsOk = MDNS.begin(WIFI_AP_HOSTNAME);
    if (mdnsOk) {
        MDNS.addService("http", "tcp", 80);
        diagLog("AP mDNS ready host=%s.local", WIFI_AP_HOSTNAME);
    } else {
        diagLog("AP mDNS start failed host=%s.local; IP fallback remains available", WIFI_AP_HOSTNAME);
    }

    _running = true;
    _lastStations = (int)WiFi.softAPgetStationNum();
    _apLostLogged = false;
    _lastHealthMs = millis();
    _lastProbeLogMs = millis();
    _lastProbeCountLogged = 0;
    _firstHttpLogged = false;
    _cliSetCount = 0;
    _cliSaveStartMs = 0;
    _cliKeys = "";
    diagLog("AP READY ip=%s stations=%d heap=%u", WiFi.softAPIP().toString().c_str(), _lastStations, (unsigned)ESP.getFreeHeap());
    logRadioSnapshot("READY");

    if (_dbg) {
        _dbg->printf("[wifi] FPV CamBuddy AP ready: %s  IP=%s  mode=AP-only  ch=%u  tx=%d\n",
                     WIFI_AP_SSID,
                     WiFi.softAPIP().toString().c_str(),
                     WIFI_AP_CHANNEL,
                     (int)WiFi.getTxPower());
    }
}

void WebConfigServer::stop() {
    if (!_running) return;

    _server.stop();
    MDNS.end();
    logRadioSnapshot("STOP-BEFORE");
    disablePromiscDiagnostics();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);

    _running = false;
    if (_dbg) _dbg->println("[wifi] FPV CamBuddy AP stopped; reboot required for camera mode");
}

void WebConfigServer::update() {
    if (!_running) return;
    _server.handleClient();

    // Drain network-task events here so Preferences/NVS writes never happen
    // inside the Wi-Fi callback itself.
    RadioDiagEvent e;
    while (popRadioDiag(e)) {
        char mac[20];
        formatMac(mac, sizeof(mac), e.mac);
        switch (e.type) {
            case RD_AP_START:
                diagLog("WIFI-EVENT AP_START evt_ms=%lu", (unsigned long)e.ms);
                break;
            case RD_AP_STOP:
                diagLog("WIFI-EVENT AP_STOP evt_ms=%lu", (unsigned long)e.ms);
                break;
            case RD_STA_CONNECTED:
                diagLog("WIFI-EVENT STA_CONNECTED mac=%s aid=%u evt_ms=%lu stations=%u",
                        mac, (unsigned)e.a, (unsigned long)e.ms,
                        (unsigned)WiFi.softAPgetStationNum());
                logRadioSnapshot("STA-CONNECT");
                break;
            case RD_STA_DISCONNECTED:
                diagLog("WIFI-EVENT STA_DISCONNECTED mac=%s aid=%u evt_ms=%lu stations=%u reason=not-exposed-by-IDF4.4-AP-event",
                        mac, (unsigned)e.a, (unsigned long)e.ms,
                        (unsigned)WiFi.softAPgetStationNum());
                logRadioSnapshot("STA-DISCONNECT");
                break;
            case RD_IP_ASSIGNED: {
                const uint8_t *ip = reinterpret_cast<const uint8_t *>(&e.extra);
                diagLog("WIFI-EVENT DHCP_IP_ASSIGNED ip=%u.%u.%u.%u evt_ms=%lu stations=%u",
                        ip[0], ip[1], ip[2], ip[3], (unsigned long)e.ms,
                        (unsigned)WiFi.softAPgetStationNum());
                break;
            }
            case RD_MGMT_AUTH:
                diagLog("80211 RX AUTH mac=%s rssi=%d ch=%u algorithm=%u sequence=%u status=%u evt_ms=%lu",
                        mac, (int)e.rssi, (unsigned)e.channel,
                        (unsigned)e.a, (unsigned)e.b, (unsigned)e.extra,
                        (unsigned long)e.ms);
                break;
            case RD_MGMT_ASSOC:
                diagLog("80211 RX ASSOC_REQ mac=%s rssi=%d ch=%u capability=0x%04X listen=%u evt_ms=%lu",
                        mac, (int)e.rssi, (unsigned)e.channel,
                        (unsigned)e.a, (unsigned)e.b, (unsigned long)e.ms);
                break;
            case RD_MGMT_REASSOC:
                diagLog("80211 RX REASSOC_REQ mac=%s rssi=%d ch=%u capability=0x%04X listen=%u evt_ms=%lu",
                        mac, (int)e.rssi, (unsigned)e.channel,
                        (unsigned)e.a, (unsigned)e.b, (unsigned long)e.ms);
                break;
            case RD_MGMT_DEAUTH:
                diagLog("80211 RX DEAUTH mac=%s rssi=%d ch=%u reason=%u(%s) evt_ms=%lu",
                        mac, (int)e.rssi, (unsigned)e.channel,
                        (unsigned)e.a, mgmtReasonName(e.a), (unsigned long)e.ms);
                break;
            case RD_MGMT_DISASSOC:
                diagLog("80211 RX DISASSOC mac=%s rssi=%d ch=%u reason=%u(%s) evt_ms=%lu",
                        mac, (int)e.rssi, (unsigned)e.channel,
                        (unsigned)e.a, mgmtReasonName(e.a), (unsigned long)e.ms);
                break;
            default:
                break;
        }
    }

    const wifi_mode_t mode = WiFi.getMode();
    const bool apUp = (mode & WIFI_MODE_AP) != 0;
    if (!apUp) {
        if (!_apLostLogged) {
            _apLostLogged = true;
            diagLog("AP LOST while server running mode=%d heap=%u dropped_events=%lu",
                    (int)mode, (unsigned)ESP.getFreeHeap(),
                    (unsigned long)s_radioDiagDropped);
        }
        return;
    }
    _apLostLogged = false;

    const int stations = (int)WiFi.softAPgetStationNum();
    if (stations != _lastStations) {
        diagLog("AP stations %d -> %d ip=%s heap=%u", _lastStations, stations,
                WiFi.softAPIP().toString().c_str(), (unsigned)ESP.getFreeHeap());
        _lastStations = stations;
    }

    const uint32_t now = millis();

    // Probe summary tells us whether the phone can at least hear/reach the AP
    // even when authentication/association never completes.
    uint32_t probeCount;
    int8_t lastProbeRssi;
    int8_t bestProbeRssi;
    uint8_t lastProbeMac[6];
    portENTER_CRITICAL(&s_radioDiagMux);
    probeCount = s_probeCount;
    lastProbeRssi = s_lastProbeRssi;
    bestProbeRssi = s_bestProbeRssi;
    memcpy(lastProbeMac, s_lastProbeMac, 6);
    portEXIT_CRITICAL(&s_radioDiagMux);

    if (probeCount != _lastProbeCountLogged && (now - _lastProbeLogMs) >= 5000UL) {
        char pmac[20];
        formatMac(pmac, sizeof(pmac), lastProbeMac);
        diagLog("PROBES total=%lu delta=%lu last_mac=%s last_rssi=%d best_rssi=%d stations=%u",
                (unsigned long)probeCount,
                (unsigned long)(probeCount - _lastProbeCountLogged),
                pmac, (int)lastProbeRssi, (int)bestProbeRssi,
                (unsigned)stations);
        _lastProbeCountLogged = probeCount;
        _lastProbeLogMs = now;
    }

    // Sparse health heartbeat: every 10 s for the first minute, then every 30 s.
    const uint32_t healthInterval = now < 60000UL ? 10000UL : 30000UL;
    if ((now - _lastHealthMs) >= healthInterval) {
        uint8_t ch = 0;
        wifi_second_chan_t second = WIFI_SECOND_CHAN_NONE;
        int8_t tx = 0;
        const esp_err_t chRc = esp_wifi_get_channel(&ch, &second);
        const esp_err_t txRc = esp_wifi_get_max_tx_power(&tx);
        diagLog("HEALTH mode=%d ch=%u(rc=%d) tx_qdbm=%d=%.1fdBm(rc=%d) stations=%u probes=%lu dropped=%lu heap=%u min8=%u largest8=%u",
                (int)mode, (unsigned)ch, (int)chRc,
                (int)tx, ((float)tx)/4.0f, (int)txRc,
                (unsigned)stations, (unsigned long)probeCount,
                (unsigned long)s_radioDiagDropped,
                (unsigned)ESP.getFreeHeap(),
                (unsigned)heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT),
                (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
        _lastHealthMs = now;
    }
}

void WebConfigServer::handleDiag() {
    String s = diagRead();
    if (!s.length()) s = "(diagnostic log empty)\n";
    _server.send(200, "text/plain", s);
}

void WebConfigServer::handleRoot() {
    const IPAddress remote = _server.client().remoteIP();
    if (!_firstHttpLogged) {
        _firstHttpLogged = true;
        diagLog("HTTP FIRST_REQUEST path=/ remote=%s stations=%u heap=%u",
                remote.toString().c_str(), (unsigned)WiFi.softAPgetStationNum(),
                (unsigned)ESP.getFreeHeap());
    } else {
        diagLog("HTTP GET / remote=%s", remote.toString().c_str());
    }
    _server.send_P(200, "text/html", WEB_INDEX_HTML);
}

void WebConfigServer::handleGetConfig() {
    diagLog("HTTP GET /api/config remote=%s", _server.client().remoteIP().toString().c_str());
    const auto &c = _cfg->config();
    JsonDocument d;
    d["camera_type"] = c.cameraType;
    CameraEntry ce;
    d["caddx_ssid"] = (_reg && _reg->preferredEntry(2, ce)) ? ce.addr : "";
    d["disarm_delay"] = c.disarmStopDelayMs;
    d["stop_on_disarm"] = c.stopOnDisarm;
    d["aux_channel"] = c.auxChannel;
    d["aux_mode"] = c.auxMode;
    d["profile_aux"] = c.profileAuxChannel;
    d["record_aux"] = c.recordAuxChannel;
    d["profile_low"] = c.profileLow;
    d["profile_mid"] = c.profileMid;
    d["profile_high"] = c.profileHigh;
    d["profile_osd"] = c.profileOsdEnabled;
    d["profile_osd_dest"] = c.profileOsdTarget;
    d["camera_match"] = c.cameraMatchMode;
    d["wake_guard"] = c.cameraWakeGuard;
    d["debug_ble"] = c.debugBle;
    d["low_power"] = c.lowPowerMode;
    d["advanced_power"] = c.advancedPowerMode;
    d["advanced_dbm"] = c.advancedPowerDbm;
    d["wifi_ap_enabled"] = c.wifiApEnabled;
    d["wifi_ap_delay"] = c.wifiApStartDelaySec;
    d["gopro_gps_time"] = c.goproGpsTimeSync;
    d["gopro_tz_mode"] = c.goproTimezoneMode;
    d["gopro_tz_offset"] = c.goproTimezoneOffsetMin;
    d["osd1"] = c.osd1Tpl;
    d["osd2"] = c.osd2Tpl;
    d["osd3"] = c.osd3Tpl;
    d["osd4"] = c.osd4Tpl;
    d["bf45_compat"] = c.bf45Compat;
    d["pilot_en"] = c.pilotNameEnabled;
    d["pilot_tpl"] = c.pilotNameTpl;
    d["craft_en"] = c.craftNameEnabled;
    d["craft_tpl"] = c.craftNameTpl;
    d["fpv_state_mode"] = c.fpvStateMode;
    d["fpv_error"] = c.fpvErrorEnabled;
    d["fpv_err_text"] = c.fpvErrorText;
    d["fpv_ready"] = c.fpvReadyEnabled;
    d["fpv_ready_text"] = c.fpvReadyText;
    d["fpv_record"] = c.fpvRecordingEnabled;
    d["fpv_record_text"] = c.fpvRecordingText;
    d["fpv_flash"] = c.fpvRecFlash;
    d["fpv_low_batt"] = c.fpvLowBatteryEnabled;
    d["fpv_low_pct"] = c.fpvLowBatteryPct;
    d["fpv_low_rdyflash"] = c.fpvLowBatteryReadyFlash;
    d["fpv_low_rectext"] = c.fpvLowBatteryRecText;
    d["fpv_low_text"] = c.fpvLowBatteryText;
    d["fpv_rect_warn"] = c.fpvLowRecTimeEnabled;
    d["fpv_rect_min"] = c.fpvLowRecTimeMin;
    d["fpv_rect_ready"] = c.fpvLowRecReadyWarning;
    d["fpv_rect_record"] = c.fpvLowRecRecordingWarning;
    d["fpv_rect_text"] = c.fpvLowRecTimeText;
    d["fpv_hot_warn"] = c.fpvHotWarningEnabled;
    d["fpv_hot_ready"] = c.fpvHotReadyWarning;
    d["fpv_hot_record"] = c.fpvHotRecordingWarning;
    d["fpv_hot_text"] = c.fpvHotWarningText;
    d["fpv_prearm"] = c.fpvPreArmReminderEnabled;
    d["fpv_prearm_text"] = c.fpvPreArmReminderText;
    d["fpv_prearm_show"] = c.fpvPreArmReminderShowMs;
    d["fpv_prearm_interval"] = c.fpvPreArmReminderIntervalMs;

    String s;
    serializeJson(d, s);
    _server.send(200, "application/json", s);
}

void WebConfigServer::handlePostConfig() {
    diagLog("HTTP POST /api/config remote=%s bytes=%u",
            _server.client().remoteIP().toString().c_str(),
            _server.hasArg("plain") ? (unsigned)_server.arg("plain").length() : 0U);
    if (!_server.hasArg("plain")) {
        _server.send(400, "text/plain", "No body");
        return;
    }

    JsonDocument d;
    if (deserializeJson(d, _server.arg("plain"))) {
        _server.send(400, "text/plain", "JSON parse error");
        return;
    }

    if (d["camera_type"].is<int>()) _cfg->setCameraType(d["camera_type"].as<uint8_t>());
    if (d["caddx_ssid"].is<const char*>()) _cfg->setCaddxSsid(d["caddx_ssid"].as<const char*>());
    if (d["caddx_pass"].is<const char*>()) _cfg->setCaddxPass(d["caddx_pass"].as<const char*>());
    if (d["disarm_delay"].is<int>()) _cfg->setDisarmDelay(d["disarm_delay"].as<uint32_t>());
    if (d["stop_on_disarm"].is<bool>()) _cfg->setStopOnDisarm(d["stop_on_disarm"].as<bool>());
    if (d["aux_channel"].is<int>()) _cfg->setAuxChannel(d["aux_channel"].as<uint8_t>());
    if (d["aux_mode"].is<int>()) _cfg->setAuxMode(d["aux_mode"].as<uint8_t>());
    if (d["profile_aux"].is<int>()) _cfg->setProfileAuxChannel(d["profile_aux"].as<uint8_t>());
    if (d["record_aux"].is<int>()) _cfg->setRecordAuxChannel(d["record_aux"].as<uint8_t>());
    if (d["profile_low"].is<uint32_t>()) _cfg->setProfileId(0, d["profile_low"].as<uint32_t>());
    if (d["profile_mid"].is<uint32_t>()) _cfg->setProfileId(1, d["profile_mid"].as<uint32_t>());
    if (d["profile_high"].is<uint32_t>()) _cfg->setProfileId(2, d["profile_high"].as<uint32_t>());
    if (d["profile_osd"].is<bool>()) _cfg->setProfileOsdEnabled(d["profile_osd"].as<bool>());
    if (d["profile_osd_dest"].is<int>()) _cfg->setProfileOsdTarget(d["profile_osd_dest"].as<uint8_t>());
    if (d["camera_match"].is<int>()) _cfg->setCameraMatchMode(d["camera_match"].as<uint8_t>());
    if (d["wake_guard"].is<bool>()) _cfg->setCameraWakeGuard(d["wake_guard"].as<bool>());
    if (d["debug_ble"].is<bool>()) _cfg->setDebugBle(d["debug_ble"].as<bool>());
    if (d["low_power"].is<bool>()) _cfg->setLowPowerMode(d["low_power"].as<bool>());
    if (d["advanced_power"].is<bool>()) _cfg->setAdvancedPowerMode(d["advanced_power"].as<bool>());
    if (d["advanced_dbm"].is<int>()) _cfg->setAdvancedPowerDbm(d["advanced_dbm"].as<int8_t>());
    if (d["wifi_ap_enabled"].is<bool>()) _cfg->setWifiApEnabled(d["wifi_ap_enabled"].as<bool>());
    if (d["wifi_ap_delay"].is<int>()) _cfg->setWifiApStartDelay(d["wifi_ap_delay"].as<uint32_t>());
    if (d["osd1"].is<const char*>()) _cfg->setOsdTemplate(1, d["osd1"].as<const char*>());
    if (d["osd2"].is<const char*>()) _cfg->setOsdTemplate(2, d["osd2"].as<const char*>());
    if (d["osd3"].is<const char*>()) _cfg->setOsdTemplate(3, d["osd3"].as<const char*>());
    if (d["osd4"].is<const char*>()) _cfg->setOsdTemplate(4, d["osd4"].as<const char*>());
    if (d["bf45_compat"].is<bool>()) _cfg->setBf45Compat(d["bf45_compat"].as<bool>());
    if (d["pilot_en"].is<bool>()) _cfg->setPilotNameEnabled(d["pilot_en"].as<bool>());
    if (d["pilot_tpl"].is<const char*>()) _cfg->setPilotNameTemplate(d["pilot_tpl"].as<const char*>());
    if (d["craft_en"].is<bool>()) _cfg->setCraftNameEnabled(d["craft_en"].as<bool>());
    if (d["craft_tpl"].is<const char*>()) _cfg->setCraftNameTemplate(d["craft_tpl"].as<const char*>());
    if (d["fpv_state_mode"].is<int>()) _cfg->setFpvStateMode(d["fpv_state_mode"].as<uint8_t>());
    if (d["fpv_error"].is<bool>()) _cfg->setFpvErrorEnabled(d["fpv_error"].as<bool>());
    if (d["fpv_err_text"].is<const char*>()) _cfg->setFpvErrorText(d["fpv_err_text"].as<const char*>());
    if (d["fpv_ready"].is<bool>()) _cfg->setFpvReadyEnabled(d["fpv_ready"].as<bool>());
    if (d["fpv_ready_text"].is<const char*>()) _cfg->setFpvReadyText(d["fpv_ready_text"].as<const char*>());
    if (d["fpv_record"].is<bool>()) _cfg->setFpvRecordingEnabled(d["fpv_record"].as<bool>());
    if (d["fpv_record_text"].is<const char*>()) _cfg->setFpvRecordingText(d["fpv_record_text"].as<const char*>());
    if (d["fpv_flash"].is<bool>()) _cfg->setFpvRecFlash(d["fpv_flash"].as<bool>());
    if (d["fpv_low_batt"].is<bool>()) _cfg->setFpvLowBatteryEnabled(d["fpv_low_batt"].as<bool>());
    if (d["fpv_low_pct"].is<int>()) _cfg->setFpvLowBatteryPct(d["fpv_low_pct"].as<uint8_t>());
    if (d["fpv_low_rdyflash"].is<bool>()) _cfg->setFpvLowBatteryReadyFlash(d["fpv_low_rdyflash"].as<bool>());
    if (d["fpv_low_rectext"].is<bool>()) _cfg->setFpvLowBatteryRecText(d["fpv_low_rectext"].as<bool>());
    if (d["fpv_low_text"].is<const char*>()) _cfg->setFpvLowBatteryText(d["fpv_low_text"].as<const char*>());
    if (d["fpv_rect_warn"].is<bool>()) _cfg->setFpvLowRecTimeEnabled(d["fpv_rect_warn"].as<bool>());
    if (d["fpv_rect_min"].is<int>()) _cfg->setFpvLowRecTimeMin(d["fpv_rect_min"].as<uint16_t>());
    if (d["fpv_rect_ready"].is<bool>()) _cfg->setFpvLowRecReadyWarning(d["fpv_rect_ready"].as<bool>());
    if (d["fpv_rect_record"].is<bool>()) _cfg->setFpvLowRecRecordingWarning(d["fpv_rect_record"].as<bool>());
    if (d["fpv_rect_text"].is<const char*>()) _cfg->setFpvLowRecTimeText(d["fpv_rect_text"].as<const char*>());
    if (d["fpv_hot_warn"].is<bool>()) _cfg->setFpvHotWarningEnabled(d["fpv_hot_warn"].as<bool>());
    if (d["fpv_hot_ready"].is<bool>()) _cfg->setFpvHotReadyWarning(d["fpv_hot_ready"].as<bool>());
    if (d["fpv_hot_record"].is<bool>()) _cfg->setFpvHotRecordingWarning(d["fpv_hot_record"].as<bool>());
    if (d["fpv_hot_text"].is<const char*>()) _cfg->setFpvHotWarningText(d["fpv_hot_text"].as<const char*>());
    if (d["fpv_prearm"].is<bool>()) _cfg->setFpvPreArmReminderEnabled(d["fpv_prearm"].as<bool>());
    if (d["fpv_prearm_text"].is<const char*>()) _cfg->setFpvPreArmReminderText(d["fpv_prearm_text"].as<const char*>());
    if (d["fpv_prearm_show"].is<int>()) _cfg->setFpvPreArmReminderShowMs(d["fpv_prearm_show"].as<uint16_t>());
    if (d["fpv_prearm_interval"].is<int>()) _cfg->setFpvPreArmReminderIntervalMs(d["fpv_prearm_interval"].as<uint16_t>());

    // Setters above write Preferences/NVS. Now throw away the in-RAM copy and
    // reload from storage before checking anything. A 200 response therefore
    // proves the values that will actually be loaded after a reboot.
    _cfg->reloadFromStorage();
    const auto &c = _cfg->config();

    String mismatch;
    auto fail = [&](const char *key) {
        if (mismatch.length()) mismatch += ", ";
        mismatch += key;
    };
    auto checkInt = [&](const char *key, uint32_t actual) {
        if (d[key].is<int>() && d[key].as<uint32_t>() != actual) fail(key);
    };
    auto checkBool = [&](const char *key, bool actual) {
        if (d[key].is<bool>() && d[key].as<bool>() != actual) fail(key);
    };
    auto checkStr = [&](const char *key, const char *actual) {
        if (d[key].is<const char*>() && strcmp(d[key].as<const char*>(), actual) != 0) fail(key);
    };

    checkInt("camera_type", c.cameraType);
    checkInt("disarm_delay", c.disarmStopDelayMs);
    checkBool("stop_on_disarm", c.stopOnDisarm);
    checkInt("aux_channel", c.auxChannel);
    checkInt("aux_mode", c.auxMode);
    checkInt("camera_match", c.cameraMatchMode);
    checkBool("wake_guard", c.cameraWakeGuard);
    checkBool("debug_ble", c.debugBle);
    checkBool("low_power", c.lowPowerMode);
    checkBool("advanced_power", c.advancedPowerMode);
    checkInt("advanced_dbm", c.advancedPowerDbm);
    checkBool("wifi_ap_enabled", c.wifiApEnabled);
    checkInt("wifi_ap_delay", c.wifiApStartDelaySec);
    checkStr("osd1", c.osd1Tpl);
    checkStr("osd2", c.osd2Tpl);
    checkStr("osd3", c.osd3Tpl);
    checkStr("osd4", c.osd4Tpl);
    checkBool("bf45_compat", c.bf45Compat);
    checkBool("pilot_en", c.pilotNameEnabled);
    checkStr("pilot_tpl", c.pilotNameTpl);
    checkBool("craft_en", c.craftNameEnabled);
    checkStr("craft_tpl", c.craftNameTpl);
    if (d["fpv_state_mode"].is<int>() && (d["fpv_state_mode"].as<uint8_t>() != (uint8_t)c.fpvStateMode)) fail("fpv_state_mode");
    checkBool("fpv_error", c.fpvErrorEnabled);
    checkStr("fpv_err_text", c.fpvErrorText);
    checkBool("fpv_ready", c.fpvReadyEnabled);
    checkStr("fpv_ready_text", c.fpvReadyText);
    checkBool("fpv_record", c.fpvRecordingEnabled);
    checkStr("fpv_record_text", c.fpvRecordingText);
    checkBool("fpv_flash", c.fpvRecFlash);
    checkBool("fpv_low_batt", c.fpvLowBatteryEnabled);
    checkInt("fpv_low_pct", c.fpvLowBatteryPct);
    checkBool("fpv_low_rdyflash", c.fpvLowBatteryReadyFlash);
    checkBool("fpv_low_rectext", c.fpvLowBatteryRecText);
    checkStr("fpv_low_text", c.fpvLowBatteryText);
    checkBool("fpv_rect_warn", c.fpvLowRecTimeEnabled);
    checkInt("fpv_rect_min", c.fpvLowRecTimeMin);
    checkBool("fpv_rect_ready", c.fpvLowRecReadyWarning);
    checkBool("fpv_rect_record", c.fpvLowRecRecordingWarning);
    checkStr("fpv_rect_text", c.fpvLowRecTimeText);
    checkBool("fpv_hot_warn", c.fpvHotWarningEnabled);
    checkBool("fpv_hot_ready", c.fpvHotReadyWarning);
    checkBool("fpv_hot_record", c.fpvHotRecordingWarning);
    checkStr("fpv_hot_text", c.fpvHotWarningText);
    checkBool("fpv_prearm", c.fpvPreArmReminderEnabled);
    checkStr("fpv_prearm_text", c.fpvPreArmReminderText);
    checkInt("fpv_prearm_show", c.fpvPreArmReminderShowMs);
    checkInt("fpv_prearm_interval", c.fpvPreArmReminderIntervalMs);

    CameraEntry persistedCaddx;
    const bool haveCaddx = _reg && _reg->preferredEntry(2, persistedCaddx);
    if (d["caddx_ssid"].is<const char*>()) {
        const char *wanted = d["caddx_ssid"].as<const char*>();
        if ((wanted[0] != '\0') && (!haveCaddx || strcmp(wanted, persistedCaddx.addr) != 0)) fail("caddx_ssid");
    }
    if (d["caddx_pass"].is<const char*>()) {
        const char *wanted = d["caddx_pass"].as<const char*>();
        if ((wanted[0] != '\0') && (!haveCaddx || strcmp(wanted, persistedCaddx.pass) != 0)) fail("caddx_pass");
    }

    if (mismatch.length()) {
        diagLog("SAVE /api/config VERIFY_FAIL keys=%s", mismatch.c_str());
        if (_dbg) _dbg->printf("[cfg] AP NVS verification FAILED: %s\n", mismatch.c_str());
        _server.send(500, "text/plain", String("Save verification failed after NVS reload: ") + mismatch);
        return;
    }

    diagLog("SAVE /api/config VERIFIED after NVS reload");
    if (_dbg) _dbg->println("[cfg] AP save verified by reloading persisted NVS state");
    _server.send(200, "application/json", "{\"ok\":true,\"verified\":true,\"source\":\"nvs\"}");
}

void WebConfigServer::handleGetCameras() {
    const String s = _reg ? _reg->toJson() : "[]";
    _server.send(200, "application/json", s);
}

void WebConfigServer::handleWifiScan() {
    _server.send(501, "application/json", "[]");
}

void WebConfigServer::handleCli() {
    if (!_server.hasArg("plain")) {
        _server.send(400, "text/plain", "No command");
        return;
    }
    String cmd = _server.arg("plain");
    cmd.trim();
    if (!cmd.length()) {
        _server.send(400, "text/plain", "Empty command");
        return;
    }

    if (cmd.startsWith("set ")) {
        if (_cliSetCount == 0) {
            _cliSaveStartMs = millis();
            _cliKeys = "";
            diagLog("SAVE CLI sequence START remote=%s",
                    _server.client().remoteIP().toString().c_str());
        }
        _cliSetCount++;
        const int keyStart = 4;
        int keyEnd = cmd.indexOf(' ', keyStart);
        if (keyEnd < 0) keyEnd = cmd.length();
        const String key = cmd.substring(keyStart, keyEnd);
        if (_cliKeys.length() < 600) {
            if (_cliKeys.length()) _cliKeys += ",";
            _cliKeys += key;
        }
    }

    StringStream out;
    _cfg->processCommand(cmd.c_str(), out);
    const String response = out.str();

    if (response.indexOf("error") >= 0 || response.indexOf("invalid") >= 0 ||
        response.indexOf("unknown") >= 0 || response.indexOf("FAILED") >= 0) {
        diagLog("SAVE/CLI ERROR cmd='%s' response='%s'", cmd.c_str(), response.c_str());
    }

    if (cmd == "show" && _cliSetCount > 0) {
        diagLog("SAVE CLI sequence END commands=%u duration_ms=%lu keys=%s",
                (unsigned)_cliSetCount,
                (unsigned long)(millis() - _cliSaveStartMs),
                _cliKeys.c_str());
        _cliSetCount = 0;
        _cliSaveStartMs = 0;
        _cliKeys = "";
    }

    _server.send(200, "text/plain", response);
}