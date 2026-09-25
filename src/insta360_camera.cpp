#include "insta360_camera.h"
#include "camera_registry.h"
#include "config.h"
#include "ble_debug.h"
#include <cctype>

namespace {

struct PbField {
    uint32_t number = 0;
    uint8_t wire = 0;
    uint64_t varint = 0;
    const uint8_t *bytes = nullptr;
    size_t len = 0;
};

static bool pbReadVarint(const uint8_t *&p, const uint8_t *end, uint64_t &v) {
    v = 0;
    uint8_t shift = 0;
    while (p < end && shift < 64) {
        const uint8_t b = *p++;
        v |= (uint64_t)(b & 0x7F) << shift;
        if ((b & 0x80) == 0) return true;
        shift += 7;
    }
    return false;
}

static bool pbNext(const uint8_t *&p, const uint8_t *end, PbField &f) {
    if (p >= end) return false;
    uint64_t key = 0;
    if (!pbReadVarint(p, end, key) || key == 0) return false;
    f = PbField{};
    f.number = (uint32_t)(key >> 3);
    f.wire = (uint8_t)(key & 0x07);

    if (f.wire == 0) {
        return pbReadVarint(p, end, f.varint);
    }
    if (f.wire == 1) {
        if ((size_t)(end - p) < 8) return false;
        f.bytes = p; f.len = 8; p += 8; return true;
    }
    if (f.wire == 2) {
        uint64_t n = 0;
        if (!pbReadVarint(p, end, n) || n > (uint64_t)(end - p)) return false;
        f.bytes = p; f.len = (size_t)n; p += n; return true;
    }
    if (f.wire == 5) {
        if ((size_t)(end - p) < 4) return false;
        f.bytes = p; f.len = 4; p += 4; return true;
    }
    return false;
}

static bool pbFindVarint(const uint8_t *data, size_t len, uint32_t number, uint64_t &value) {
    const uint8_t *p = data, *end = data + len;
    PbField f;
    while (pbNext(p, end, f)) {
        if (f.number == number && f.wire == 0) { value = f.varint; return true; }
    }
    return false;
}

static bool pbFindBytes(const uint8_t *data, size_t len, uint32_t number,
                        const uint8_t *&value, size_t &valueLen) {
    const uint8_t *p = data, *end = data + len;
    PbField f;
    while (pbNext(p, end, f)) {
        if (f.number == number && f.wire == 2) {
            value = f.bytes; valueLen = f.len; return true;
        }
    }
    return false;
}

static uint8_t pbAppendVarintField(uint8_t *out, uint8_t pos, uint8_t max,
                                   uint32_t field, uint32_t value) {
    uint32_t key = field << 3;
    auto put = [&](uint32_t v) {
        while (v >= 0x80 && pos < max) { out[pos++] = (uint8_t)(v | 0x80); v >>= 7; }
        if (pos < max) out[pos++] = (uint8_t)v;
    };
    put(key); put(value);
    return pos;
}

static bool instaCaptureStateIsVideo(uint32_t s) {
    switch (s) {
        case 1:  // NORMAL_CAPTURE
        case 2:  // TIMELAPSE_CAPTURE
        case 7:  // BULLET_TIME_CAPTURE
        case 9:  // HDR_CAPTURE
        case 12: // INTERVAL_VIDEO_CAPTURE
        case 13: // TIMESHIFT_CAPTURE
        case 17: // SUPER_NORMAL_CAPTURE
        case 18: // LOOP_RECORDING_CAPTURE
        case 20: // FPV_RECORDING_CAPTURE
        case 21: // MOVIE_RECORDING_CAPTURE
        case 22: // SLOW_MOTION_CAPTURE
        case 23: // SELFIE_RECORDING_CAPTURE
        case 24: // PURE_RECORDING_CAPTURE
            return true;
        default:
            return false;
    }
}

static uint8_t instaCaptureStateToMode(uint32_t s) {
    switch (s) {
        case 2:
        case 12:
        case 13:
            return 0x02; // timelapse-style
        case 7:
        case 22:
            return 0x00; // slow motion
        default:
            return 0x01; // video/other supported recording modes
    }
}

} // namespace

Insta360Camera *Insta360Camera::_instance = nullptr;

static esp_power_level_t selectedBlePower(bool advanced, int8_t dbm, bool low) {
    if (!advanced) return low ? ESP_PWR_LVL_N12 : ESP_PWR_LVL_P9;
#if CONFIG_IDF_TARGET_ESP32C3
    if (dbm <= -24) return ESP_PWR_LVL_N24;
    if (dbm <= -21) return ESP_PWR_LVL_N21;
    if (dbm <= -18) return ESP_PWR_LVL_N18;
    if (dbm <= -15) return ESP_PWR_LVL_N15;
#else
    // Classic ESP32 has no levels below -12 dBm. The C3-only advanced values
    // are clamped here so the shared firmware still compiles safely for ESP32.
    if (dbm <= -12) return ESP_PWR_LVL_N12;
#endif
    if (dbm <= -12) return ESP_PWR_LVL_N12;
    if (dbm <= -9) return ESP_PWR_LVL_N9;
    if (dbm <= -6) return ESP_PWR_LVL_N6;
    if (dbm <= -3) return ESP_PWR_LVL_N3;
    if (dbm <= 0) return ESP_PWR_LVL_N0;
    if (dbm <= 3) return ESP_PWR_LVL_P3;
    if (dbm <= 6) return ESP_PWR_LVL_P6;
    return ESP_PWR_LVL_P9;
}

// ─── Public ──────────────────────────────────────────────────────────────────

void Insta360Camera::begin() {
    _instance = this;
    BLEDevice::init("FreeCLinker");
    BLEDevice::setPower(selectedBlePower(_advancedPowerMode, _advancedPowerDbm, _lowPowerMode));

    startScan();
}

void Insta360Camera::update() {
    const uint32_t now = millis();

    if (_instaConnected) {
        // Lost query responses must not permanently stop polling.
        if (_pendingCaptureStatusSeq != 0 && (now - _lastCapturePollMs) >= 3000UL) {
            if (_debugBle)
                DBG_SERIAL.printf("[Insta360][DBG] Capture-status query timeout seq=%lu\n",
                                  (unsigned long)_pendingCaptureStatusSeq);
            _pendingCaptureStatusSeq = 0;
        }
        if (_pendingOptionsSeq != 0 && (now - _lastOptionsPollMs) >= 8000UL) {
            if (_debugBle)
                DBG_SERIAL.printf("[Insta360][DBG] Telemetry-options query timeout seq=%lu\n",
                                  (unsigned long)_pendingOptionsSeq);
            _pendingOptionsSeq = 0;
        }

        // Keep capture state fresh enough for OSD/CLI without flooding the BLE link.
        if (_pendingCaptureStatusSeq == 0 && (now - _lastCapturePollMs) >= 1000UL) {
            requestCaptureStatus();
            _lastCapturePollMs = now;
        }
        // Battery/storage/remaining-time change far less often.
        if (_pendingOptionsSeq == 0 && (now - _lastOptionsPollMs) >= 5000UL) {
            requestTelemetryOptions();
            _lastOptionsPollMs = now;
        }
        return;
    }

    if (_bleConnected || _scanning) return;

    if (_targetFound) {
        connectAndSetup();
        return;
    }

    if (millis() - _lastAttemptMs > BLE_RECONNECT_DELAY_MS)
        startScan();
}

// ─── Scan ────────────────────────────────────────────────────────────────────

void Insta360Camera::startScan() {
    _targetFound   = false;
    _scanning      = true;
    _candidateAddr = "";
    _candidateName = "";
    _bestAddr      = "";
    _bestRssi      = -128;

    BLEScan *scan = BLEDevice::getScan();
    scan->setAdvertisedDeviceCallbacks(this, false);
    scan->setActiveScan(true);
    scan->setInterval(0x50);
    scan->setWindow(0x30);

    std::string preferred = _registry ? _registry->preferredAddr() : "";
    if (preferred.empty())
        DBG_SERIAL.println("[BLE] Scanning for Insta360 camera...");
    else
        DBG_SERIAL.printf("[BLE] Scanning for Insta360 camera %s...\n", preferred.c_str());

    scan->start(BLE_SCAN_DURATION_SECS, scanDoneCallback, false);
}

void Insta360Camera::scanDoneCallback(BLEScanResults /*r*/) {
    if (!_instance) return;
    _instance->_scanning = false;
    if (_instance->_targetFound) return;

    if (_instance->_matchMode == CAM_MATCH_BEST_SIGNAL) {
        if (!_instance->_bestAddr.empty()) {
            DBG_SERIAL.printf("[BLE] Best signal — connecting to %s (rssi=%d)\n",
                              _instance->_bestAddr.c_str(), _instance->_bestRssi);
            _instance->_targetAddr  = _instance->_bestAddr;
            _instance->_targetName  = _instance->_bestName;
            _instance->_targetType  = _instance->_bestType;
            _instance->_targetFound = true;
        } else {
            DBG_SERIAL.println("[BLE] Scan done — no Insta360 camera found");
            _instance->_lastAttemptMs = millis();
        }
        return;
    }

    if (!_instance->_candidateAddr.empty() && _instance->_matchMode != CAM_MATCH_STRICT) {
        DBG_SERIAL.printf("[BLE] Preferred not found — connecting to %s\n",
                          _instance->_candidateAddr.c_str());
        _instance->_targetAddr  = _instance->_candidateAddr;
        _instance->_targetName  = _instance->_candidateName;
        _instance->_targetType  = _instance->_candidateType;
        _instance->_targetFound = true;
    } else {
        if (_instance->_matchMode == CAM_MATCH_STRICT && !_instance->_candidateAddr.empty())
            DBG_SERIAL.println("[BLE] Preferred not found — strict mode, skipping other cameras");
        else
            DBG_SERIAL.println("[BLE] Scan done — no Insta360 camera found");
        _instance->_lastAttemptMs = millis();
    }
}

// No vendor scan filter (manufacturer ID, dedicated advertised service) for
// this family is documented/verified anywhere we could check, unlike DJI/
// GoPro/Sony/Blackmagic — this name check is a best-effort heuristic only.
// Cameras are user-nameable and product docs show the factory default is
// "<model> <serial-suffix>" (e.g. "X3 123456"), not a fixed "Insta360"
// prefix, so a miss here doesn't necessarily mean it isn't one.
static bool looksLikeInsta360Name(const std::string &name) {
    std::string lower = name;
    for (char &c : lower) c = (char)tolower((unsigned char)c);

    if (lower.find("insta360") != std::string::npos) return true;

    static const char *prefixes[] = {
        "x3", "x4", "x5", "one r", "one x", "go 2", "go 3", "go3s", "ace",
    };
    for (const char *p : prefixes)
        if (lower.rfind(p, 0) == 0) return true;

    return false;
}

// Identify candidates by the advertised primary service UUID 0xBE80 where
// present; fall back to the name heuristic above when it isn't (some BLE
// stacks omit a custom service from the advertisement payload — its
// 31-byte budget is easily eaten by the local name — so absence here isn't
// conclusive either).
void Insta360Camera::onResult(BLEAdvertisedDevice device) {
    bool isInsta = device.haveServiceUUID() &&
                   device.isAdvertisingService(BLEUUID(INSTA_SERVICE_UUID));

    if (!isInsta && device.haveName() && looksLikeInsta360Name(device.getName()))
        isInsta = true;

    if (!isInsta) return;

    std::string addr = device.getAddress().toString();
    std::string name = device.haveName() ? device.getName() : "Insta360 Camera";

    DBG_SERIAL.printf("[BLE] Found Insta360 camera: \"%s\"  addr=%s  rssi=%d\n",
                      name.c_str(), addr.c_str(), device.getRSSI());

    if (_matchMode == CAM_MATCH_BEST_SIGNAL) {
        int8_t rssi = device.getRSSI();
        if (_bestAddr.empty() || rssi > _bestRssi) {
            _bestAddr = addr;
            _bestName = name;
            _bestType = device.getAddressType();
            _bestRssi = rssi;
        }
        return;  // keep scanning the full window to find the strongest signal
    }

    // Keep first found as fallback
    if (_candidateAddr.empty()) {
        _candidateAddr = addr;
        _candidateName = name;
        _candidateType = device.getAddressType();
    }

    std::string preferred = _registry ? _registry->preferredAddr() : "";

    if ((!preferred.empty() && addr == preferred) ||
        (preferred.empty() && !_targetFound)) {
        BLEDevice::getScan()->stop();
        _targetAddr  = addr;
        _targetName  = name;
        _targetType  = device.getAddressType();
        _targetFound = true;
        _scanning    = false;
    }
}

// ─── Connect, service discovery, subscriptions ───────────────────────────────

bool Insta360Camera::connectAndSetup() {
    DBG_SERIAL.printf("[BLE] Connecting to Insta360 camera %s ...\n", _targetAddr.c_str());

    if (!_client) {
        _client = BLEDevice::createClient();
        _client->setClientCallbacks(this);
    }

    if (!_client->connect(BLEAddress(_targetAddr), _targetType)) {
        DBG_SERIAL.println("[BLE] Connection failed");
        _targetFound   = false;
        _lastAttemptMs = millis();
        return false;
    }

    _bleConnected = true;

    // Bump the ATT MTU so a command response (documented examples run past
    // 20 bytes) arrives as a single notification instead of needing manual
    // multi-packet reassembly.
    if (_client->setMTU(500))
        DBG_SERIAL.printf("[BLE] MTU exchange OK (got %u)\n", _client->getMTU());
    else
        DBG_SERIAL.println("[BLE] MTU exchange failed — staying at 23");

    BLERemoteService *svc = _client->getService(BLEUUID(INSTA_SERVICE_UUID));
    if (!svc) {
        DBG_SERIAL.println("[BLE] Insta360 camera-control service (0xBE80) not found");
        _client->disconnect();
        _bleConnected  = false;
        _targetFound   = false;
        _lastAttemptMs = millis();
        return false;
    }

    BLERemoteCharacteristic *notifyCh = svc->getCharacteristic(BLEUUID(INSTA_CHAR_NOTIFY));
    if (!notifyCh || !notifyCh->canNotify()) {
        DBG_SERIAL.println("[BLE] Notify char 0xBE82 not found or not notifiable");
        _client->disconnect();
        _bleConnected  = false;
        _targetFound   = false;
        _lastAttemptMs = millis();
        return false;
    }
    notifyCh->registerForNotify(notifyCallback);

    _writeChar = svc->getCharacteristic(BLEUUID(INSTA_CHAR_WRITE));
    if (!_writeChar || !_writeChar->canWrite()) {
        DBG_SERIAL.println("[BLE] Write char 0xBE81 not found or not writable");
        _client->disconnect();
        _bleConnected  = false;
        _targetFound   = false;
        _lastAttemptMs = millis();
        return false;
    }

    // No resolution/fps/EIS telemetry available — the camera's status
    // protobuf isn't decodable here (see insta360_protocol.h). Battery (if
    // the discovery below succeeds) fills in _camera.percent asynchronously.
    _camera = CameraData{};
    _camera.eis_mode = CAM_EIS_UNKNOWN;

    discoverBatteryService();

    _instaConnected = true;
    _lastCapturePollMs = millis() - 1000UL;
    _lastOptionsPollMs = millis() - 5000UL;
    _pendingCaptureStatusSeq = 0;
    _pendingOptionsSeq = 0;

    if (_registry)
        _registry->onConnected(_targetName.c_str(), _targetAddr.c_str(),
                               (uint8_t)_targetType, /*Insta360=*/5);

    DBG_SERIAL.println("[BLE] Insta360 camera ready — recording control active");
    return true;
}

// Standard Bluetooth SIG Battery Service — best-effort, since it isn't part
// of the documented 0xBE80 command channel. A missing service/characteristic
// just means no battery telemetry, not a connection failure.
void Insta360Camera::discoverBatteryService() {
    BLERemoteService *battSvc = _client->getService(BLEUUID((uint16_t)0x180F));
    if (!battSvc) {
        DBG_SERIAL.println("[BLE] Battery Service (0x180F) not available — no battery telemetry");
        return;
    }

    BLERemoteCharacteristic *battCh = battSvc->getCharacteristic(BLEUUID((uint16_t)0x2A19));
    if (!battCh || !battCh->canNotify()) {
        DBG_SERIAL.println("[BLE] Battery Level char (0x2A19) not found — no battery telemetry");
        return;
    }
    battCh->registerForNotify(batteryNotifyCallback);
    DBG_SERIAL.println("[BLE] Subscribed to 0x2A19 (battery notify)");

    if (battCh->canRead()) {
        std::string val = battCh->readValue();
        if (!val.empty())
            handleBatteryNotification((uint8_t *)val.data(), val.size());
    }
}

void Insta360Camera::onConnect(BLEClient * /*c*/) {
    DBG_SERIAL.println("[BLE] onConnect");
}

void Insta360Camera::onDisconnect(BLEClient * /*c*/) {
    DBG_SERIAL.println("[BLE] Insta360 camera disconnected — will rescan");
    _instaConnected      = false;
    _bleConnected        = false;
    _writeChar           = nullptr;
    _targetFound         = false;
    _targetAddr          = "";
    _targetName          = "";
    _candidateAddr       = "";
    _candidateName       = "";
    _awaitingRecordAck   = false;
    _pendingRecordSeq    = 0;
    _pendingCaptureStatusSeq = 0;
    _pendingOptionsSeq   = 0;
    _lastAttemptMs       = millis();
}

// ─── Commands ─────────────────────────────────────────────────────────────────

bool Insta360Camera::sendCommand(uint16_t cmd, const uint8_t *payload,
                                 uint16_t payloadLen, uint32_t *seqOut) {
    if (!_writeChar) return false;
    uint8_t pkt[INSTA_MAX_PACKET];
    const uint32_t seq = _seq++ & 0x00FFFFFFUL;
    const uint16_t n = instaBuildPacket(cmd, seq, payload, payloadLen, pkt, sizeof(pkt));
    if (!n) return false;
    if (seqOut) *seqOut = seq;
    if (_debugBle) {
        DBG_SERIAL.printf("[Insta360][DBG] TX cmd=%u seq=%lu payload=%uB total=%uB\n",
                          (unsigned)cmd, (unsigned long)seq,
                          (unsigned)payloadLen, (unsigned)n);
        bleDebugDump(DBG_SERIAL, "TX", "0xBE81", pkt, n);
    }
    _writeChar->writeValue(pkt, n, true);
    return true;
}

bool Insta360Camera::requestCaptureStatus() {
    uint32_t seq = 0;
    if (!sendCommand(INSTA_CMD_GET_CAPTURE_STATUS, nullptr, 0, &seq)) return false;
    _pendingCaptureStatusSeq = seq;
    if (_debugBle)
        DBG_SERIAL.printf("[Insta360][DBG] Capture-status query seq=%lu\n", (unsigned long)seq);
    return true;
}

bool Insta360Camera::requestTelemetryOptions() {
    // GetOptions.option_types (field 1, repeated enum):
    // remaining capture time, battery, storage, video sub-mode, camera type, temperature.
    const uint32_t opts[] = {
        INSTA_OPT_REMAINING_CAPTURE_TIME,
        INSTA_OPT_BATTERY_STATUS,
        INSTA_OPT_STORAGE_STATE,
        INSTA_OPT_VIDEO_SUB_MODE,
        INSTA_OPT_CAMERA_TYPE,
        INSTA_OPT_TEMP_VALUE
    };
    uint8_t pb[32];
    uint8_t n = 0;
    for (uint32_t v : opts) n = pbAppendVarintField(pb, n, sizeof(pb), 1, v);

    uint32_t seq = 0;
    if (!sendCommand(INSTA_CMD_GET_OPTIONS, pb, n, &seq)) return false;
    _pendingOptionsSeq = seq;
    if (_debugBle)
        DBG_SERIAL.printf("[Insta360][DBG] Telemetry-options query seq=%lu fields=%u\n",
                          (unsigned long)seq, (unsigned)(sizeof(opts) / sizeof(opts[0])));
    return true;
}

// The camera only tells us whether the command it just received succeeded
// (a 200-OK ack) — there's no separate, decodable "current recording state"
// push to fall back on (see insta360_protocol.h), so state is derived from
// the command we sent, gated on that ack.
bool Insta360Camera::startRecording() {
    if (!_writeChar) return false;
    if (_camera.valid && _camera.recording) return true;
    uint32_t seq = 0;
    if (!sendCommand(INSTA_CMD_START_VIDEO, nullptr, 0, &seq)) return false;
    _awaitingRecordAck   = true;
    _pendingRecordTarget = true;
    _pendingRecordSeq    = seq;
    DBG_SERIAL.println("[Insta360] Start video sent");
    return true;
}

bool Insta360Camera::stopRecording() {
    if (!_writeChar) return false;
    if (_camera.valid && !_camera.recording) return true;
    uint32_t seq = 0;
    if (!sendCommand(INSTA_CMD_STOP_VIDEO, nullptr, 0, &seq)) return false;
    _awaitingRecordAck   = true;
    _pendingRecordTarget = false;
    _pendingRecordSeq    = seq;
    DBG_SERIAL.println("[Insta360] Stop video sent");
    return true;
}

bool Insta360Camera::switchCameraMode(uint8_t /*mode*/) {
    DBG_SERIAL.println("[Insta360] switchCameraMode not supported — protocol only exposes "
                       "separate start-recording-in-mode-X commands, not a mode switch");
    return false;
}

// ─── Notify callbacks (static trampolines) ───────────────────────────────────

void Insta360Camera::notifyCallback(BLERemoteCharacteristic * /*ch*/,
                                     uint8_t *data, size_t len, bool /*isNotify*/) {
    if (_instance) _instance->handleNotification(data, len);
}

void Insta360Camera::batteryNotifyCallback(BLERemoteCharacteristic * /*ch*/,
                                            uint8_t *data, size_t len, bool /*isNotify*/) {
    if (_instance) _instance->handleBatteryNotification(data, len);
}

// Only the fixed header is parsed (see insta360_protocol.h) — a bare 7-byte
// Keep-Alive-shaped ack carries no response code and is just logged; a full
// Phone Command response's code at [7:9] resolves whichever recording
// command is currently in flight. The protobuf body (if any) is never
// decoded — no available schema.
void Insta360Camera::handleNotification(uint8_t *data, size_t len) {
    if (_debugBle) bleDebugDump(DBG_SERIAL, "RX", "0xBE82", data, len);
    if (!data || len < 7) return;

    const bool isKeepAlive = len >= 7 && data[4] == INSTA_MSGTYPE_KEEPALIVE &&
                             data[5] == 0 && data[6] == 0;
    const bool isCommand = len >= 16 && data[4] == INSTA_MSGTYPE_COMMAND &&
                           data[5] == 0 && data[6] == 0;

    if (isKeepAlive) {
        if (_debugBle) DBG_SERIAL.println("[Insta360][DBG] RX keepalive/short ack");
        return;
    }
    if (!isCommand) {
        if (_debugBle)
            DBG_SERIAL.printf("[Insta360][DBG] RX unknown envelope type=0x%02X len=%u\n",
                              data[4], (unsigned)len);
        return;
    }

    const uint16_t code = (uint16_t)data[7] | ((uint16_t)data[8] << 8);
    const uint32_t seq = (uint32_t)data[10] |
                         ((uint32_t)data[11] << 8) |
                         ((uint32_t)data[12] << 16);
    const uint8_t *body = data + 16;
    const size_t bodyLen = len - 16;

    if (_debugBle) {
        DBG_SERIAL.printf("[Insta360][DBG] RX code=%u seq=%lu payload=%uB%s\n",
                          (unsigned)code, (unsigned long)seq, (unsigned)bodyLen,
                          code == INSTA_RESP_OK ? " OK" : "");
    }

    // Async notifications are identified by response code, not by our request seq.
    if (code == INSTA_NOTIFY_CAPTURE_STATUS) {
        handleCaptureStatusPayload(body, bodyLen, false);
        return;
    }
    if (code == INSTA_NOTIFY_STORAGE_UPDATE) {
        handleStorageUpdatePayload(body, bodyLen);
        return;
    }
    if (code == INSTA_NOTIFY_STORAGE_FULL) {
        _camera.has_media_ready = true;
        _camera.media_ready = false;
        _camera.has_remain_time = true;
        _camera.remain_time = 0;
        _camera.valid = true;
        DBG_SERIAL.println("[Insta360] Storage full");
        if (_cameraCb) _cameraCb(_camera);
        return;
    }
    if (code == INSTA_NOTIFY_CAPTURE_STOPPED) {
        _camera.recording = false;
        _camera.has_recording = true;
        _camera.record_time = 0;
        _camera.valid = true;
        DBG_SERIAL.println("[Insta360] Capture stopped notification");
        if (_cameraCb) _cameraCb(_camera);
        return;
    }
    if (code == INSTA_NOTIFY_BATTERY_LOW) {
        // The low-battery notification does not carry a trustworthy percentage.
        // Keep percent unknown unless BatteryStatus/0x2A19 supplies it.
        if (_debugBle) DBG_SERIAL.println("[Insta360][DBG] Battery-low notification");
        return;
    }

    if (code != INSTA_RESP_OK) {
        if (_awaitingRecordAck && seq == _pendingRecordSeq) {
            _awaitingRecordAck = false;
            _pendingRecordSeq = 0;
            DBG_SERIAL.printf("[Insta360] Recording command failed (response=%u seq=%lu)\n",
                              (unsigned)code, (unsigned long)seq);
        } else if (_debugBle) {
            DBG_SERIAL.printf("[Insta360][DBG] Non-OK response code=%u seq=%lu\n",
                              (unsigned)code, (unsigned long)seq);
        }
        if (seq == _pendingCaptureStatusSeq) _pendingCaptureStatusSeq = 0;
        if (seq == _pendingOptionsSeq) _pendingOptionsSeq = 0;
        return;
    }

    // Sequence-correlated 200 responses. This avoids a telemetry response
    // accidentally satisfying a record command that is also in flight.
    if (_awaitingRecordAck && seq == _pendingRecordSeq) {
        _awaitingRecordAck = false;
        _pendingRecordSeq = 0;
        _camera.recording = _pendingRecordTarget;
        _camera.has_recording = true;
        _camera.valid = true;
        DBG_SERIAL.printf("[Insta360] Recording %s\n",
                          _camera.recording ? "started" : "stopped");
        if (_cameraCb) _cameraCb(_camera);
        return;
    }

    if (seq == _pendingCaptureStatusSeq) {
        _pendingCaptureStatusSeq = 0;
        handleCaptureStatusPayload(body, bodyLen, true);
        return;
    }

    if (seq == _pendingOptionsSeq) {
        _pendingOptionsSeq = 0;
        handleOptionsPayload(body, bodyLen);
        return;
    }

    if (_debugBle)
        DBG_SERIAL.printf("[Insta360][DBG] Unmatched OK response seq=%lu payload=%uB\n",
                          (unsigned long)seq, (unsigned)bodyLen);
}

void Insta360Camera::handleCaptureStatusPayload(const uint8_t *data, size_t len, bool wrapped) {
    if (!data || !len) return;

    const uint8_t *msg = data;
    size_t msgLen = len;

    if (wrapped) {
        // GetCurrentCaptureStatusResp.status = field 1, length-delimited.
        if (!pbFindBytes(data, len, 1, msg, msgLen)) {
            if (_debugBle)
                DBG_SERIAL.printf("[Insta360][DBG] Capture status response had no field-1 status (%uB)\n",
                                  (unsigned)len);
            return;
        }
    }

    uint64_t state = 0, captureTime = 0;
    const bool haveState = pbFindVarint(msg, msgLen, 1, state);
    const bool haveTime = pbFindVarint(msg, msgLen, 2, captureTime);
    if (!haveState && !haveTime) return;

    if (haveState) {
        _camera.recording = instaCaptureStateIsVideo((uint32_t)state);
        _camera.has_recording = true;
        if (state != 0)
            _camera.camera_mode = instaCaptureStateToMode((uint32_t)state);
    }
    if (haveTime)
        _camera.record_time = (uint16_t)(captureTime > 65535ULL ? 65535ULL : captureTime);

    _camera.valid = true;
    if (_debugBle)
        DBG_SERIAL.printf("[Insta360][DBG] Capture status state=%lu recording=%d time=%lus\n",
                          (unsigned long)state, _camera.recording ? 1 : 0,
                          (unsigned long)captureTime);
    if (_cameraCb) _cameraCb(_camera);
}

void Insta360Camera::handleOptionsPayload(const uint8_t *data, size_t len) {
    if (!data || !len) return;

    // GetOptionsResp.value = field 2, length-delimited Options.
    const uint8_t *opts = nullptr;
    size_t optsLen = 0;
    if (!pbFindBytes(data, len, 2, opts, optsLen)) {
        if (_debugBle)
            DBG_SERIAL.printf("[Insta360][DBG] GetOptions response had no value message (%uB)\n",
                              (unsigned)len);
        return;
    }

    bool changed = false;
    const uint8_t *p = opts, *end = opts + optsLen;
    PbField f;
    while (pbNext(p, end, f)) {
        if (f.number == 9 && f.wire == 0) { // remaining_capture_time, seconds
            _camera.remain_time = (uint32_t)(f.varint > 0xFFFFFFFFULL ? 0xFFFFFFFFULL : f.varint);
            _camera.has_remain_time = true;
            changed = true;
            if (_debugBle)
                DBG_SERIAL.printf("[Insta360][DBG] remaining_capture_time=%lus\n",
                                  (unsigned long)_camera.remain_time);
        } else if (f.number == 11 && f.wire == 2) { // BatteryStatus
            uint64_t level = 0;
            if (pbFindVarint(f.bytes, f.len, 2, level) && level <= 100) {
                _camera.percent = (uint8_t)level;
                _camera.has_battery = true;
                changed = true;
                if (_debugBle)
                    DBG_SERIAL.printf("[Insta360][DBG] battery_level=%u%%\n",
                                      (unsigned)_camera.percent);
            }
        } else if (f.number == 20 && f.wire == 2) { // StorageState
            uint64_t state = 0, freeBytes = 0;
            const bool haveState = pbFindVarint(f.bytes, f.len, 1, state);
            const bool haveFree = pbFindVarint(f.bytes, f.len, 2, freeBytes);
            if (haveState) {
                _camera.has_media_ready = true;
                _camera.media_ready = (state == 0); // STOR_CS_PASS
                changed = true;
            }
            if (haveFree) {
                const uint64_t freeMb = freeBytes / (1024ULL * 1024ULL);
                _camera.remain_cap_mb = (uint32_t)(freeMb > 0xFFFFFFFFULL ? 0xFFFFFFFFULL : freeMb);
                changed = true;
            }
            if (_debugBle)
                DBG_SERIAL.printf("[Insta360][DBG] storage state=%lu free=%lluB (%luMB)\n",
                                  (unsigned long)state,
                                  (unsigned long long)freeBytes,
                                  (unsigned long)_camera.remain_cap_mb);
        } else if (f.number == 41 && f.wire == 0) { // video_sub_mode
            if (_debugBle)
                DBG_SERIAL.printf("[Insta360][DBG] video_sub_mode=%lu\n",
                                  (unsigned long)f.varint);
        } else if (f.number == 48 && f.wire == 2) { // camera_type string
            if (_debugBle) {
                String s;
                for (size_t i = 0; i < f.len && i < 48; ++i) {
                    const char ch = (char)f.bytes[i];
                    s += (ch >= 32 && ch <= 126) ? ch : '.';
                }
                DBG_SERIAL.printf("[Insta360][DBG] camera_type='%s'\n", s.c_str());
            }
        } else if (f.number == 77 && f.wire == 0) { // temp_value, int32
            // Keep temperature unavailable in CameraData: it only has an
            // over-temperature severity field, and no verified threshold maps
            // this raw value to WARN/HOT/SHUTDOWN across Insta360 models.
            if (_debugBle)
                DBG_SERIAL.printf("[Insta360][DBG] temp_value(raw)=%ld (not mapped to overheat state)\n",
                                  (long)(int32_t)(uint32_t)f.varint);
        } else if (_debugBle) {
            DBG_SERIAL.printf("[Insta360][DBG] Options unknown/unused field=%lu wire=%u len=%u\n",
                              (unsigned long)f.number, (unsigned)f.wire,
                              (unsigned)f.len);
        }
    }

    if (changed) {
        _camera.valid = true;
        if (_cameraCb) _cameraCb(_camera);
    }
}

void Insta360Camera::handleStorageUpdatePayload(const uint8_t *data, size_t len) {
    if (!data || !len) return;
    uint64_t state = 0;
    if (!pbFindVarint(data, len, 1, state)) return;

    _camera.has_media_ready = true;
    _camera.media_ready = (state == 0); // STOR_CS_PASS
    _camera.valid = true;
    DBG_SERIAL.printf("[Insta360] Storage state: %s (code=%lu)\n",
                      _camera.media_ready ? "ready" : "not ready",
                      (unsigned long)state);
    if (_cameraCb) _cameraCb(_camera);
}

// Battery Level (0x2A19) — standard BLE SIG format: a single byte, 0-100.
void Insta360Camera::handleBatteryNotification(uint8_t *data, size_t len) {
    if (_debugBle) bleDebugDump(DBG_SERIAL, "RX", "0x2A19", data, len);
    if (len < 1 || data[0] > 100) return;

    _camera.percent = data[0];
    _camera.has_battery = true;
    _camera.valid   = true;
    DBG_SERIAL.printf("[Insta360] Battery: %u%%\n", _camera.percent);
    if (_cameraCb) _cameraCb(_camera);
}
