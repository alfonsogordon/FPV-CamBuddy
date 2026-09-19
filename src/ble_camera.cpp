#include "ble_camera.h"
#include "camera_registry.h"
#include "config.h"
#include "ble_debug.h"
#include <cstring>

BLECamera *BLECamera::_instance = nullptr;

// ─── Public ──────────────────────────────────────────────────────────────────

void BLECamera::begin() {
    _instance = this;
    BLEDevice::init("ESP32-DJI-Bridge");
    BLEDevice::setPower(_lowPowerMode ? ESP_PWR_LVL_N12 : ESP_PWR_LVL_P9);
    BLEDevice::setMTU(500);   // sets local preference; per-connection exchange done via _client->setMTU()
    startScan();
}

void BLECamera::update() {
    const uint32_t now = millis();

    // Nano DUML request ACKs are queued by the BLE callback and written here;
    // writing from inside the callback can re-enter the BLE stack and crash.
    if (_pendingNanoRespLen && _bleConnected && _writeChar) {
        const uint16_t n = _pendingNanoRespLen;
        _pendingNanoRespLen = 0;
        if (_debugBle) bleDebugDump(DBG_SERIAL, "TX", "Nano DUML ACK", _pendingNanoResp, n);
        _writeChar->writeValue(_pendingNanoResp, n, false);
        DBG_SERIAL.printf("[Nano] DUML response sent (%uB)\n", n);
        return;
    }

    if (_pendingNanoPairComplete && _bleConnected) {
        _pendingNanoPairComplete = false;
        _nanoPaired = true;
        _nanoPairApprovalNeeded = false;
        DBG_SERIAL.println("[Nano] Pairing complete — starting persistent session");

        // The camera remembers this identity after first approval, so only mark
        // it preferred once the Nano has actually accepted our app-level pair.
        if (_registry)
            _registry->onConnected(_targetName.c_str(), _targetAddr.c_str(),
                                   (uint8_t)_targetType, /*DJI=*/0);

        beginNanoRsdk();
        return;
    }

    // Hardware-verified Nano BLE sessions need this Mimo-style DUML keepalive
    // or the camera tears the link down after roughly 5–6 seconds.
    if (_isOsmoNano && _bleConnected && _nanoPaired &&
        (now - _nanoLastKeepaliveMs) >= 1000UL) {
        sendNanoKeepalive();
        _nanoLastKeepaliveMs = now;
    }

    // If the first SetPairingPIN write was lost, retry with the same identity.
    // Stop once the camera has answered with either ALREADY PAIRED or
    // APPROVAL REQUIRED; the latter waits for the tester to approve on-camera.
    if (_isOsmoNano && _bleConnected && !_nanoPaired && !_nanoPairApprovalNeeded) {
        const uint32_t age = now - _nanoPairStartMs;
        if ((age >= 2500UL && _nanoLastPairTxMs < _nanoPairStartMs + 2000UL) ||
            (age >= 5000UL && _nanoLastPairTxMs < _nanoPairStartMs + 4500UL)) {
            DBG_SERIAL.println("[Nano] No pairing reply yet — retrying SetPairingPIN");
            uint8_t payload[96];
            uint16_t o = 0;
            o += nano_pack_string(payload + o, sizeof(payload) - o,
                                  "284ae5b8d76b3375a04a6417ad71bea3");
            o += nano_pack_string(payload + o, sizeof(payload) - o, "cambuddy");
            sendNanoDuml(0x0702, 0x8092, 0x40, 0x07, 0x45, payload, o);
            _nanoLastPairTxMs = now;
        }
    }

    // Deferred R-SDK connect ACK: send from the main loop, not from the BLE
    // callback, to avoid calling writeValue() inside a BLE stack context.
    if (_pendingConnectAck && _bleConnected && !_djiConnected) {
        _pendingConnectAck = false;

        DJIConnectResponse resp{};
        resp.device_id = 0x00000001;
        resp.ret_code  = 0;

        if (!sendFrame(DJI_CMDSET_GENERAL, DJI_CMD_CONNECT, DJI_ACK,
                       reinterpret_cast<const uint8_t *>(&resp), sizeof(resp),
                       /*with_rsp=*/false, /*override_seq=*/(int32_t)_pendingAckSeq)) {
            DBG_SERIAL.println("[DJI] Failed to send connect ACK");
        } else {
            DBG_SERIAL.println("[DJI] Connection established — subscribing to status");
            if (sendStatusSubscription()) {
                _djiConnected = true;
                if (_isOsmoNano) {
                    _nanoLastStatusMs = 0;
                    _nanoLastStatusPollMs = now;
                    DBG_SERIAL.println("[Nano] R-SDK ready — ARM/disarm record control enabled");
                }
            }
        }
        return;
    }

    // Nano control/status stays on the hardware-proven BLE DUML path.
    // 0x02/0x80 normally pushes at ~10 Hz; if it goes quiet, poke the camera
    // with the documented 0x02/0x61 status poll and 0x02/0xA0 state query.
    if (_isOsmoNano && _djiConnected &&
        (now - _nanoLastStatusPollMs) >= 1500UL &&
        (_nanoLastStatusMs == 0 || (now - _nanoLastStatusMs) >= 2000UL)) {
        _nanoLastStatusPollMs = now;
        DBG_SERIAL.println("[Nano] Status feed quiet — polling DUML camera status");
        sendNanoDuml(0x0102, 0x8061, 0x00, 0x02, 0x61, nullptr, 0);
        delay(15);
        sendNanoDuml(0x0102, 0x80A0, 0x00, 0x02, 0xA0, nullptr, 0);
    }

    if (_djiConnected || _bleConnected || _scanning) return;

    if (_targetFound) {
        connectAndSetup();
        return;
    }

    if (millis() - _lastAttemptMs > BLE_RECONNECT_DELAY_MS)
        startScan();
}

// ─── Scan ────────────────────────────────────────────────────────────────────

void BLECamera::startScan() {
    _targetFound   = false;
    _scanning      = true;
    _candidateAddr = "";
    _candidateName = "";
    _bestAddr      = "";
    _bestRssi      = -128;

    BLEScan *scan = BLEDevice::getScan();
    scan->setAdvertisedDeviceCallbacks(this, /*wantDuplicates=*/false);
    scan->setActiveScan(true);
    scan->setInterval(0x50);   // matches DJI SDK scan params
    scan->setWindow(0x30);

    std::string preferred = _registry ? _registry->preferredAddr() : "";
    if (preferred.empty())
        DBG_SERIAL.println("[BLE] Scanning for DJI Action camera...");
    else
        DBG_SERIAL.printf("[BLE] Scanning for DJI camera %s...\n", preferred.c_str());

    scan->start(BLE_SCAN_DURATION_SECS, scanDoneCallback, false);
}

void BLECamera::scanDoneCallback(BLEScanResults /*r*/) {
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
            DBG_SERIAL.println("[BLE] Scan done — no DJI camera found");
            _instance->_lastAttemptMs = millis();
        }
        return;
    }

    if (!_instance->_candidateAddr.empty() && _instance->_matchMode != CAM_MATCH_STRICT) {
        // Preferred camera wasn't seen — fall back to first DJI camera found
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
            DBG_SERIAL.println("[BLE] Scan done — no DJI camera found");
        _instance->_lastAttemptMs = millis();
    }
}

// onResult runs for every advertised device during scan.
// DJI cameras are identified by manufacturer-specific data bytes
// [0]=0xAA [1]=0x08 (manufacturer ID 0x08AA) and [4]=0xFA.
// Device name is used as a fallback if manufacturer data is absent.
//
// If the registry has a preferred address, stop scan early when it is found.
// Otherwise record the first found camera as a fallback and keep scanning
// in case the preferred device appears later in the window.
void BLECamera::onResult(BLEAdvertisedDevice device) {
    bool isDJI = false;
    bool isNano = false;

    if (device.haveManufacturerData()) {
        const std::string mfr = device.getManufacturerData();
        if (mfr.size() >= 3 &&
            (uint8_t)mfr[0] == 0xAA &&
            (uint8_t)mfr[1] == 0x08) {
            // Existing Action cameras use the 0xFA marker at byte 4.
            // Hardware-verified Osmo Nano advertisements use model byte 0x19.
            if ((mfr.size() >= 5 && (uint8_t)mfr[4] == 0xFA) ||
                (uint8_t)mfr[2] == 0x19) {
                isDJI = true;
                isNano = ((uint8_t)mfr[2] == 0x19);
            }
        }
    }

    if (device.haveName()) {
        const std::string devName = device.getName();
        if (!isDJI && devName.find(DJI_DEVICE_NAME_PREFIX) != std::string::npos)
            isDJI = true;
        if (devName.find("OsmoNano-") != std::string::npos) {
            isDJI = true;
            isNano = true;
        }
    }

    if (!isDJI) return;

    std::string addr = device.getAddress().toString();
    std::string name = device.haveName() ? device.getName() :
                       (isNano ? "DJI Osmo Nano" : "DJI Action");

    DBG_SERIAL.printf("[BLE] Found DJI camera: \"%s\"  addr=%s  rssi=%d%s\n",
                      name.c_str(), addr.c_str(), device.getRSSI(),
                      isNano ? "  [Osmo Nano]" : "");

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

    if (!preferred.empty() && addr == preferred) {
        // Preferred camera found — stop scan immediately
        BLEDevice::getScan()->stop();
        _targetAddr  = addr;
        _targetName  = name;
        _targetType  = device.getAddressType();
        _targetFound = true;
        _scanning    = false;
    } else if (preferred.empty() && !_targetFound) {
        // No preference — connect to first found (original behavior)
        BLEDevice::getScan()->stop();
        _targetAddr  = addr;
        _targetName  = name;
        _targetType  = device.getAddressType();
        _targetFound = true;
        _scanning    = false;
    }
}

// ─── Connect & characteristic discovery ──────────────────────────────────────

bool BLECamera::connectAndSetup() {
    _isOsmoNano = (_targetName.find("OsmoNano-") != std::string::npos ||
                   _targetName.find("Osmo Nano") != std::string::npos);
    DBG_SERIAL.printf("[BLE] Connecting to %s%s ...\n", _targetAddr.c_str(),
                      _isOsmoNano ? " (Osmo Nano)" : "");

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

    // Request MTU=500 immediately after connecting so DJI frames (up to ~56 bytes)
    // arrive in a single notification rather than truncated to 20 bytes.
    if (_client->setMTU(500)) {
        DBG_SERIAL.printf("[BLE] MTU exchange sent (want 500, got %u)\n", _client->getMTU());
    } else {
        DBG_SERIAL.println("[BLE] MTU exchange failed — staying at 23");
    }

    DBG_SERIAL.println("[BLE] BLE connected");

#if DEBUG_PRINT_SERVICES
    printServices();
#endif

    // Locate the DJI service
    BLERemoteService *svc = _client->getService(BLEUUID((uint16_t)DJI_SERVICE_UUID));
    if (!svc) {
        DBG_SERIAL.println("[BLE] DJI service 0xFFF0 not found");
        _client->disconnect();
        _targetFound   = false;
        _lastAttemptMs = millis();
        return false;
    }

    // Notify characteristic (0xFFF4) — camera → ESP32
    BLERemoteCharacteristic *notifyCh =
        svc->getCharacteristic(BLEUUID((uint16_t)DJI_NOTIFY_CHAR_UUID));
    if (!notifyCh || !notifyCh->canNotify()) {
        DBG_SERIAL.println("[BLE] Notify char 0xFFF4 not found or not notifiable");
        _client->disconnect();
        _targetFound   = false;
        _lastAttemptMs = millis();
        return false;
    }
    notifyCh->registerForNotify(notifyCallback);
    _notifyChar = notifyCh;
    DBG_SERIAL.printf("[BLE] Subscribed to 0xFFF4 notify (write=%d writeNR=%d)\n",
                      notifyCh->canWrite() ? 1 : 0,
                      notifyCh->canWriteNoResponse() ? 1 : 0);

    // Write characteristic — ESP32 → camera.
    // Osmo Nano exposes 0xFFF5 as Write-Without-Response, while some Action
    // bodies expose normal Write. Action 5 Pro may require the 0xFFF3 fallback.
    _writeNoResponseOnly = false;
    _writeChar = svc->getCharacteristic(BLEUUID((uint16_t)DJI_WRITE_CHAR_UUID));
    if (!_writeChar || (!_writeChar->canWrite() && !_writeChar->canWriteNoResponse())) {
        _writeChar = svc->getCharacteristic(BLEUUID((uint16_t)DJI_WRITE_CHAR_UUID_ALT));
    }
    if (!_writeChar || (!_writeChar->canWrite() && !_writeChar->canWriteNoResponse())) {
        DBG_SERIAL.println("[BLE] No writable command char found (tried 0xFFF5, 0xFFF3)");
        _client->disconnect();
        _targetFound   = false;
        _lastAttemptMs = millis();
        return false;
    }
    _writeNoResponseOnly = !_writeChar->canWrite() && _writeChar->canWriteNoResponse();
    DBG_SERIAL.printf("[BLE] Write char 0x%04X ready (%s notify=%d)\n",
                      _writeChar->getUUID().getNative()->uuid.uuid16,
                      _writeNoResponseOnly ? "write-no-response" : "write",
                      _writeChar->canNotify() ? 1 : 0);

    // Nano can send DUML/R-SDK notifications on both FFF4 and FFF5.
    if (_isOsmoNano && _writeChar->canNotify() && _writeChar != _notifyChar) {
        _writeChar->registerForNotify(notifyCallback);
        DBG_SERIAL.println("[Nano] Subscribed to 0xFFF5 notify too");
    }

    _bleConnected = true;

    // Existing Action cameras keep their proven R-SDK flow. Nano first needs
    // DJI's app-level DUML pairing/session sequence; only after that do we probe
    // the R-SDK status/record-control channel.
    if (_isOsmoNano) {
        startNanoPairing();
    } else {
        if (_registry)
            _registry->onConnected(_targetName.c_str(), _targetAddr.c_str(),
                                   (uint8_t)_targetType, /*DJI=*/0);
        sendConnectionRequest();
    }
    return true;
}

void BLECamera::onConnect(BLEClient * /*c*/) {
    DBG_SERIAL.println("[BLE] onConnect");
}

void BLECamera::onDisconnect(BLEClient * /*c*/) {
    DBG_SERIAL.println("[BLE] Disconnected — will rescan");
    _djiConnected  = false;
    _bleConnected  = false;
    _writeChar     = nullptr;
    _notifyChar    = nullptr;
    _writeNoResponseOnly = false;
    _isOsmoNano = false;
    _nanoPaired = false;
    _nanoPairApprovalNeeded = false;
    _nanoRsdkStarted = false;
    _nanoPairStartMs = 0;
    _nanoLastPairTxMs = 0;
    _nanoLastKeepaliveMs = 0;
    _nanoLastStatusMs = 0;
    _nanoLastStatusPollMs = 0;
    _pendingNanoPairComplete = false;
    _pendingNanoRespLen = 0;
    _targetFound   = false;
    _targetAddr    = "";
    _targetName    = "";
    _candidateAddr = "";
    _candidateName = "";
    _lastAttemptMs = millis();
}

// ─── Service listing (debug) ──────────────────────────────────────────────────

void BLECamera::printServices() {
    DBG_SERIAL.println("[BLE] ── Services ─────────────────────────────");
    auto *svcs = _client->getServices();
    if (!svcs) { DBG_SERIAL.println("[BLE] (none)"); return; }
    for (auto &[u, svc] : *svcs) {
        DBG_SERIAL.printf("[BLE] Svc  %s\n", svc->getUUID().toString().c_str());
        auto *chs = svc->getCharacteristics();
        if (!chs) continue;
        for (auto &[cu, ch] : *chs)
            DBG_SERIAL.printf("[BLE]   Chr %s  n=%d r=%d w=%d\n",
                              ch->getUUID().toString().c_str(),
                              ch->canNotify(), ch->canRead(), ch->canWrite());
    }
    DBG_SERIAL.println("[BLE] ──────────────────────────────────────────");
}

// ─── Osmo Nano DUML pairing/session ─────────────────────────────────────────

bool BLECamera::sendNanoDuml(uint16_t target, uint16_t id, uint8_t flags,
                             uint8_t cmdSet, uint8_t cmdId,
                             const uint8_t *payload, uint16_t payloadLen) {
    if (!_writeChar) return false;
    uint8_t frame[180];
    const uint16_t n = nano_duml_build(frame, sizeof(frame), target, id,
                                       flags, cmdSet, cmdId, payload, payloadLen);
    if (!n) return false;
    if (_debugBle) bleDebugDump(DBG_SERIAL, "TX", "Nano DUML", frame, n);
    _writeChar->writeValue(frame, n, false);
    DBG_SERIAL.printf("[Nano] DUML TX %02X/%02X flags=0x%02X id=0x%04X len=%u\n",
                      cmdSet, cmdId, flags, id, n);
    return true;
}

bool BLECamera::startNanoPairing() {
    if (!_notifyChar || !_writeChar) return false;

    _nanoPaired = false;
    _nanoPairApprovalNeeded = false;
    _nanoRsdkStarted = false;
    _pendingNanoPairComplete = false;
    _pendingNanoRespLen = 0;

    // Hardware-verified Mimo/Osmosis sequence:
    //   FFF4 <- [01 00]                    arm app-level pairing
    //   DUML 00/2B [04 00] -> target F0   wake/session preamble
    //   DUML 07/45 identifier + token     pair this app identity
    const uint8_t arm[2] = {0x01, 0x00};
    const bool armWithRsp = _notifyChar->canWrite();
    if (!_notifyChar->canWrite() && !_notifyChar->canWriteNoResponse()) {
        DBG_SERIAL.println("[Nano] FFF4 is not writable — cannot arm pairing");
        return false;
    }
    _notifyChar->writeValue((uint8_t *)arm, sizeof(arm),
                            armWithRsp ? true : false);
    DBG_SERIAL.printf("[Nano] Pairing arm [01 00] -> FFF4 (%s)\n",
                      armWithRsp ? "write-response" : "write-no-response");
    delay(80);

    const uint8_t wake[2] = {0x04, 0x00};
    sendNanoDuml(0xF002, 0x802B, 0x40, 0x00, 0x2B, wake, sizeof(wake));
    delay(120);

    uint8_t payload[96];
    uint16_t o = 0;
    o += nano_pack_string(payload + o, sizeof(payload) - o,
                          "284ae5b8d76b3375a04a6417ad71bea3");
    o += nano_pack_string(payload + o, sizeof(payload) - o, "cambuddy");
    const bool ok = sendNanoDuml(0x0702, 0x8092, 0x40, 0x07, 0x45, payload, o);
    _nanoPairStartMs = millis();
    _nanoLastPairTxMs = _nanoPairStartMs;
    DBG_SERIAL.println("[Nano] SetPairingPIN sent (token='cambuddy'); first use may require approval on camera");
    return ok;
}

void BLECamera::queueNanoResponse(const NanoDumlFrame &f) {
    uint8_t payload[96];
    const uint8_t *p = f.payload;
    uint16_t plen = f.payloadLen;

    // Mimo supplies an APP identity blob when the camera asks 00/81.
    if (f.cmdSet == 0x00 && f.cmdId == 0x81) {
        memset(payload, 0, sizeof(payload));
        uint16_t o = 0;
        payload[o++] = 0x00;
        payload[o++] = 'A'; payload[o++] = 'P'; payload[o++] = 'P';
        o += 37;
        payload[o++] = 0x02;
        o += 8;
        payload[o++] = 0x02; payload[o++] = 0x08;
        o += 10;
        p = payload;
        plen = o;
    }

    // Inbound target is receiver/sender; swap the two address bytes for reply.
    const uint16_t replyTarget = (uint16_t)((f.target << 8) | (f.target >> 8));
    const uint16_t n = nano_duml_build(_pendingNanoResp, sizeof(_pendingNanoResp),
                                       replyTarget, f.id, 0xC0,
                                       f.cmdSet, f.cmdId, p, plen);
    if (!n) {
        DBG_SERIAL.printf("[Nano] Could not build response for %02X/%02X\n",
                          f.cmdSet, f.cmdId);
        return;
    }
    _pendingNanoRespLen = n;
    DBG_SERIAL.printf("[Nano] Queued response to inbound request %02X/%02X id=0x%04X\n",
                      f.cmdSet, f.cmdId, f.id);
}

static uint16_t nanoLe16(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}
static uint32_t nanoLe32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

void BLECamera::handleNanoDuml(const NanoDumlFrame &f) {
    DBG_SERIAL.printf("[Nano] DUML RX %02X/%02X flags=0x%02X id=0x%04X payload=%uB\n",
                      f.cmdSet, f.cmdId, f.flags, f.id, f.payloadLen);

    // Native Nano camera-status push (GetPushStateInfo), hardware-captured at ~10 Hz.
    // Layout is documented from real Mimo/Nano captures.
    if (f.cmdSet == 0x02 && f.cmdId == 0x80 && f.payload && f.payloadLen >= 58) {
        const uint32_t stateFlags = nanoLe32(f.payload + 0);
        const bool recording = (stateFlags & 0x00000080UL) != 0;
        const uint8_t mode = f.payload[57];

        _camera.valid = true;
        _camera.has_recording = true;
        _camera.recording = recording;
        _camera.camera_mode = mode;
        _camera.record_time = nanoLe16(f.payload + 29);

        const uint32_t freeMb = nanoLe32(f.payload + 9);
        _camera.remain_cap_mb = freeMb;
        const uint16_t remainSec = nanoLe16(f.payload + 17);
        _camera.remain_time = remainSec;
        _camera.has_remain_time = (mode != DJI_MODE_PHOTO);

        switch (f.payload[57]) {
            case DJI_MODE_VIDEO:
            case DJI_MODE_SLOW_MOTION:
            case DJI_MODE_TIMELAPSE:
            case DJI_MODE_PHOTO:
            case DJI_MODE_HYPERLAPSE:
                _activeProfile = f.payload[57];
                break;
            default:
                break;
        }

        _nanoLastStatusMs = millis();
        if (_cameraCb) _cameraCb(_camera);
        DBG_SERIAL.printf("[Nano] status rec=%s mode=0x%02X time=%us free=%luMB remain=%us\n",
                          recording ? "yes" : "no", mode,
                          (unsigned)_camera.record_time,
                          (unsigned long)freeMb, (unsigned)remainSec);
        return;
    }

    // Nano battery push: percent is byte 20. Temperature exists in the packet
    // but remains intentionally unavailable until its scale is hardware-verified.
    if (f.cmdSet == 0x0D && f.cmdId == 0x02 && f.payload && f.payloadLen >= 21) {
        const uint8_t pct = f.payload[20];
        if (pct <= 100) {
            _camera.percent = pct;
            _camera.has_battery = true;
            _camera.valid = true;
            if (_cameraCb) _cameraCb(_camera);
            DBG_SERIAL.printf("[Nano] battery=%u%%\n", pct);
        }
        return;
    }

    // 0x02/0xA0 state-query response carries elapsed recording seconds @6.
    // Use it only as a supplement to the authoritative 0x02/0x80 recording bit.
    if (f.cmdSet == 0x02 && f.cmdId == 0xA0 && f.payload && f.payloadLen >= 8) {
        _camera.record_time = nanoLe16(f.payload + 6);
        _nanoLastStatusMs = millis();
        if (_camera.valid && _cameraCb) _cameraCb(_camera);
        return;
    }

    // Record-control response. 00=accepted; state confirmation comes from 02/80.
    if (f.cmdSet == 0x02 && f.cmdId == 0x02 && f.flags == 0xC0) {
        const uint8_t ret = (f.payloadLen && f.payload) ? f.payload[0] : 0xFF;
        if (ret == 0)
            DBG_SERIAL.println("[Nano] Record command accepted — waiting for status bit");
        else
            DBG_SERIAL.printf("[Nano] Record command rejected: 0x%02X\n", ret);
        return;
    }

    // Camera-originated requests must be ACKed or the Nano drops the BLE link.
    if (f.flags == 0x40) {
        queueNanoResponse(f);
        if (f.cmdSet == 0x07 && f.cmdId == 0x46) {
            DBG_SERIAL.println("[Nano] Pairing APPROVED by camera (07/46 request)");
            _pendingNanoPairComplete = true;
        }
        return;
    }

    if (f.cmdSet == 0x07 && f.cmdId == 0x45) {
        const int status = (f.payloadLen >= 2) ? f.payload[1] : -1;
        if (status == 0x01) {
            DBG_SERIAL.println("[Nano] Pairing status: ALREADY PAIRED");
            _pendingNanoPairComplete = true;
        } else if (status == 0x02) {
            _nanoPairApprovalNeeded = true;
            DBG_SERIAL.println("[Nano] Pairing status: APPROVAL REQUIRED — approve FPV CamBuddy on the Nano screen");
        } else {
            DBG_SERIAL.printf("[Nano] Pairing status unknown: 0x%02X\n",
                              status < 0 ? 0xFF : status);
        }
    } else if (f.cmdSet == 0x07 && f.cmdId == 0x46) {
        DBG_SERIAL.println("[Nano] Pairing APPROVED (07/46)");
        _pendingNanoPairComplete = true;
    }
}

void BLECamera::sendNanoKeepalive() {
    const uint8_t keepalive[2] = {0x01, 0x01};
    sendNanoDuml(0xF002, 0x802B, 0x40, 0x00, 0x2B,
                 keepalive, sizeof(keepalive));
}

void BLECamera::beginNanoRsdk() {
    if (_nanoRsdkStarted) return;
    _nanoRsdkStarted = true;

    // Mimo wakes the post-pair session with 53/10 addressed to type 0x1C.
    const uint8_t wake5310[4] = {0, 0, 0, 0};
    sendNanoDuml(0x1C02, 0x8053, 0x40, 0x53, 0x10,
                 wake5310, sizeof(wake5310));
    delay(120);
    sendNanoKeepalive();
    _nanoLastKeepaliveMs = millis();

    // Nano's proven control plane is BLE DUML, not the 0xAA Action R-SDK
    // handshake. Mark it connected once app-level pairing succeeds so MSP
    // arm/disarm can drive the camera immediately.
    _djiConnected = true;
    _nanoLastStatusMs = 0;
    _nanoLastStatusPollMs = 0;
    DBG_SERIAL.println("[Nano] BLE DUML control ready — ARM/disarm record control enabled");

    // Kick status/state once; regular pushes should follow.
    delay(15);
    sendNanoDuml(0x0102, 0x8061, 0x00, 0x02, 0x61, nullptr, 0);
    delay(15);
    sendNanoDuml(0x0102, 0x80A0, 0x00, 0x02, 0xA0, nullptr, 0);
}

// ─── DJI frame send ───────────────────────────────────────────────────────────

bool BLECamera::sendFrame(uint8_t cmd_set, uint8_t cmd_id, uint8_t cmd_type,
                           const uint8_t *payload, uint16_t len,
                           bool with_rsp, int32_t override_seq) {
    if (!_writeChar) return false;
    uint8_t buf[DJI_MAX_FRAME];
    uint16_t seq = (override_seq >= 0) ? (uint16_t)override_seq : _seq++;
    uint16_t n = dji_build_frame(buf, sizeof(buf), cmd_set, cmd_id, cmd_type,
                                  seq, payload, len);
    if (n == 0) return false;
    if (_debugBle) bleDebugDump(DBG_SERIAL, "TX", "0xFFF5", buf, n);
    // Never request an ATT response from a write-no-response-only characteristic
    // (Osmo Nano 0xFFF5). Existing Action cameras keep their prior behaviour.
    _writeChar->writeValue(buf, n, _writeNoResponseOnly ? false : with_rsp);
    return true;
}

// Step 1 of DJI handshake — send controller identity to the camera.
// The camera responds via notify (CmdSet 0x00, CmdID 0x19, DJI_ACK).
bool BLECamera::sendConnectionRequest() {
    uint8_t mac[6] = {};
    esp_read_mac(mac, ESP_MAC_BT);

    DJIConnectRequest req{};
    // Use the lower 4 bytes of our BT MAC as device_id — stable across reboots,
    // unique per ESP32, no NVS needed.
    _deviceId        = (uint32_t)mac[2] << 24 | (uint32_t)mac[3] << 16 |
                       (uint32_t)mac[4] <<  8 | (uint32_t)mac[5];
    req.device_id    = _deviceId;
    req.mac_addr_len = 6;
    memcpy(req.mac_addr, mac, 6);
    req.fw_version  = 0x00;   // 0 = "no version" per DJI reference; non-zero triggers OTA update prompt
    req.verify_mode = 0;
    // Nano follows DJI's current R-SDK approval flow. A non-zero verification
    // value gives the camera a concrete first-use approval request; already
    // approved controllers still reconnect without user interaction.
    req.verify_data = _isOsmoNano ? (uint16_t)(esp_random() % 10000U) : 0;

    const uint8_t connectType = _isOsmoNano ? 0x02 : DJI_CMD;
    bool ok = sendFrame(DJI_CMDSET_GENERAL, DJI_CMD_CONNECT, connectType,
                        reinterpret_cast<const uint8_t *>(&req), sizeof(req),
                        /*with_rsp=*/true);
    if (ok) {
        if (_isOsmoNano)
            DBG_SERIAL.printf("[DJI] Osmo Nano R-SDK connection request sent (approval code=%u)\n", req.verify_data);
        else
            DBG_SERIAL.println("[DJI] Connection request sent");
    }
    return ok;
}

bool BLECamera::startRecording() {
    if (_isOsmoNano) {
        if (!_bleConnected || !_nanoPaired) return false;
        const uint8_t start = 0x01;
        const bool ok = sendNanoDuml(0x0102, 0x8202, 0x40, 0x02, 0x02, &start, 1);
        if (ok) DBG_SERIAL.println("[Nano] Record START sent via DUML 02/02");
        return ok;
    }
    DJIRecordControl ctrl{};
    ctrl.device_id = _deviceId;
    ctrl.action    = DJI_RECORD_START;
    bool ok = sendFrame(DJI_CMDSET_CAMERA, DJI_CMD_RECORD_CTRL, DJI_CMD,
                        reinterpret_cast<const uint8_t *>(&ctrl), sizeof(ctrl),
                        /*with_rsp=*/true);
    if (ok) DBG_SERIAL.println("[DJI] Record start sent");
    return ok;
}

bool BLECamera::stopRecording() {
    if (_isOsmoNano) {
        if (!_bleConnected || !_nanoPaired) return false;
        const uint8_t stop = 0x00;
        const bool ok = sendNanoDuml(0x0102, 0x8202, 0x40, 0x02, 0x02, &stop, 1);
        if (ok) DBG_SERIAL.println("[Nano] Record STOP sent via DUML 02/02");
        return ok;
    }
    DJIRecordControl ctrl{};
    ctrl.device_id = _deviceId;
    ctrl.action    = DJI_RECORD_STOP;
    bool ok = sendFrame(DJI_CMDSET_CAMERA, DJI_CMD_RECORD_CTRL, DJI_CMD,
                        reinterpret_cast<const uint8_t *>(&ctrl), sizeof(ctrl),
                        /*with_rsp=*/true);
    if (ok) DBG_SERIAL.println("[DJI] Record stop sent");
    return ok;
}

bool BLECamera::switchCameraMode(uint8_t mode) {
    if (_isOsmoNano) {
        if (!_bleConnected || !_nanoPaired) return false;
        // Sparse, hardware-captured Nano shooting-mode enum. Never sweep it.
        switch (mode) {
            case DJI_MODE_SLOW_MOTION:
            case DJI_MODE_VIDEO:
            case DJI_MODE_TIMELAPSE:
            case DJI_MODE_PHOTO:
            case DJI_MODE_HYPERLAPSE:
                break;
            default:
                DBG_SERIAL.printf("[Nano] Unsupported shooting mode 0x%02X\n", mode);
                return false;
        }
        const bool ok = sendNanoDuml(0x0102, 0x82E1, 0x40, 0x02, 0xE1, &mode, 1);
        if (ok) DBG_SERIAL.printf("[Nano] Shooting mode 0x%02X sent via DUML 02/E1\n", mode);
        return ok;
    }
    DJICameraModeSwitch cmd{};
    cmd.device_id = _deviceId;
    cmd.mode      = mode;
    bool ok = sendFrame(DJI_CMDSET_CAMERA, DJI_CMD_MODE_SWITCH, DJI_CMD,
                        reinterpret_cast<const uint8_t *>(&cmd), sizeof(cmd),
                        /*with_rsp=*/true);
    if (ok) DBG_SERIAL.printf("[DJI] Mode switch 0x%02X sent\n", mode);
    return ok;
}

// DJI Action 4/5 Pro/6 profile support uses only the hardware-proven R-SDK
// camera-mode switch. These cameras do not currently have a verified public
// command for loading arbitrary native saved presets, so profile IDs are
// deliberately limited to known camera modes instead of sending guessed DUML.
// IDs are stable CamBuddy values and can therefore be assigned to AUX LOW/MID/HIGH.
bool BLECamera::loadProfile(uint32_t profileId) {
    if (!_djiConnected || _isOsmoNano) return false;
    uint8_t mode;
    switch (profileId) {
        case DJI_MODE_VIDEO:       mode = DJI_MODE_VIDEO; break;
        case DJI_MODE_SLOW_MOTION: mode = DJI_MODE_SLOW_MOTION; break;
        case DJI_MODE_TIMELAPSE:   mode = DJI_MODE_TIMELAPSE; break;
        case DJI_MODE_PHOTO:       mode = DJI_MODE_PHOTO; break;
        case DJI_MODE_HYPERLAPSE:  mode = DJI_MODE_HYPERLAPSE; break;
        default:
            DBG_SERIAL.printf("[DJI-PROFILE] unsupported safe profile id=%lu\n", (unsigned long)profileId);
            return false;
    }
    const bool ok = switchCameraMode(mode);
    if (ok) _activeProfile = profileId;
    return ok;
}

bool BLECamera::queryProfiles() {
    if (!_djiConnected || _isOsmoNano) return false;
    DBG_SERIAL.println("[DJI-PRESET] id=1 name=Video title=0 number=0");
    DBG_SERIAL.println("[DJI-PRESET] id=0 name=Slow Motion title=0 number=0");
    DBG_SERIAL.println("[DJI-PRESET] id=2 name=Timelapse title=0 number=0");
    DBG_SERIAL.println("[DJI-PRESET] id=10 name=Hyperlapse title=0 number=0");
    DBG_SERIAL.println("[DJI-PRESET] id=5 name=Photo title=0 number=0");
    return true;
}

// Step 2 — subscribe to 2 Hz camera status push (battery, mode, temps, …).
bool BLECamera::sendStatusSubscription() {
    DJIStatusSubscription sub{};
    sub.push_mode = DJI_PUSH_PERIODIC_ON_CHANGE;
    sub.push_freq = 20;   // 20 × 0.1 Hz = 2 Hz (only value the camera accepts)

    bool ok = sendFrame(DJI_CMDSET_CAMERA, DJI_CMD_STATUS_SUB, DJI_CMD,
                        reinterpret_cast<const uint8_t *>(&sub), sizeof(sub));
    if (ok) DBG_SERIAL.println("[DJI] Status subscription sent (2 Hz)");
    return ok;
}

// ─── Incoming notify ─────────────────────────────────────────────────────────

void BLECamera::notifyCallback(BLERemoteCharacteristic * /*ch*/,
                                uint8_t *data, size_t len, bool /*isNotify*/) {
    if (_instance) _instance->handleNotification(data, static_cast<size_t>(len));
}

// Each BLE notification arrives as ≤20 bytes (ATT MTU 23 = 20 data bytes if no MTU exchange).
// Full DJI frames can be 27–51+ bytes. We dispatch whatever we receive:
//   • If the notification holds the complete frame (len >= frame_len): full CRC parse.
//   • If truncated (len < frame_len): header-only dispatch — enough for the 4-step handshake
//     because the critical fields (CmdSet, CmdID, cmd_type, seq, ret_code) all land within
//     the first 20 bytes of every known frame type.
void BLECamera::handleNotification(uint8_t *data, size_t length) {
    if (length == 0) return;

    if (_debugBle) bleDebugDump(DBG_SERIAL, "RX", "DJI BLE", data, length);

    if (_isOsmoNano && data[0] == 0x55) {
        NanoDumlFrame f;
        if (nano_duml_parse(data, (uint16_t)length, f)) {
            handleNanoDuml(f);
        } else {
            DBG_SERIAL.printf("[Nano] Unparsed DUML notification (%uB)\n", (unsigned)length);
        }
        return;
    }

    // R-SDK frames use the existing 0xAA transport.
    if (data[0] != DJI_SOF) return;
    if (length < 14) return;   // need at least SOF…CmdID

    uint16_t frame_len = (uint16_t)data[1] | (((uint16_t)data[2] & 0x03) << 8);
    uint8_t  cmd_type  = data[3];
    uint16_t seq       = (uint16_t)data[8] | ((uint16_t)data[9] << 8);
    uint8_t  cmd_set   = data[12];
    uint8_t  cmd_id    = data[13];

    // Complete frame — verify both CRCs before dispatching.
    if (frame_len >= DJI_OVERHEAD && length >= frame_len) {
        const uint8_t *payload;
        uint16_t payload_len;
        if (!dji_parse_frame(data, frame_len, &cmd_type, &cmd_set, &cmd_id,
                              &payload, &payload_len)) {
            DBG_SERIAL.printf("[DJI] CRC error: cs=0x%02X id=0x%02X\n", cmd_set, cmd_id);
            return;
        }
        dispatchFrame(cmd_type, cmd_set, cmd_id, seq, payload, payload_len);
        return;
    }

    // Partial frame (MTU limited): dispatch header + whatever payload bytes arrived.
    // All critical fields for the 4-step handshake are within the first 20 bytes.
    const uint8_t *partial     = (length > 14) ? data + 14 : nullptr;
    uint16_t       partial_len = (length > 14) ? (uint16_t)(length - 14) : 0;
    dispatchFrame(cmd_type, cmd_set, cmd_id, seq, partial, partial_len);
}

void BLECamera::dispatchFrame(uint8_t cmd_type, uint8_t cmd_set, uint8_t cmd_id,
                               uint16_t seq, const uint8_t *payload, uint16_t payload_len) {
    if (cmd_set == DJI_CMDSET_GENERAL && cmd_id == DJI_CMD_CONNECT) {
        if (DJI_IS_ACK(cmd_type))
            handleConnectResponse(payload, payload_len);
        else
            handleConnectCommand(seq, payload, payload_len);
    } else if (cmd_set == DJI_CMDSET_CAMERA && cmd_id == DJI_CMD_STATUS_PUSH) {
        handleCameraStatus(payload, payload_len);
    } else if (cmd_set == DJI_CMDSET_CAMERA && cmd_id == DJI_CMD_NEW_STATUS_PUSH) {
        handleNewCameraStatus(payload, payload_len);
    } else if (cmd_set == DJI_CMDSET_CAMERA && cmd_id == DJI_CMD_RECORD_CTRL
               && DJI_IS_ACK(cmd_type)) {
        handleRecordAck(payload, payload_len);
    } else if (cmd_set == DJI_CMDSET_CAMERA && cmd_id == DJI_CMD_MODE_SWITCH
               && DJI_IS_ACK(cmd_type)) {
        handleModeSwitchAck(payload, payload_len);
    } else {
        DBG_SERIAL.printf("[DJI] Unhandled: cs=0x%02X id=0x%02X type=0x%02X\n",
                          cmd_set, cmd_id, cmd_type);
    }
}

// ─── DJI frame handlers ───────────────────────────────────────────────────────

// Step 4 of handshake: camera ACKed our connect request.
// Don't subscribe yet — the camera will next send its own hello (step 5),
// which we ACK (step 6) and THEN subscribe to status (step 7).
void BLECamera::handleConnectResponse(const uint8_t *payload, uint16_t len) {
    // ret_code is at DJIConnectResponse byte 4 (after the 4-byte device_id).
    // We always get at least 6 bytes of payload even with 20-byte MTU notifications.
    if (len < 5) {
        DBG_SERIAL.println("[DJI] ACK payload too short — ignoring");
        return;
    }
    uint8_t ret_code = payload[4];
    if (ret_code != 0) {
        DBG_SERIAL.printf("[DJI] Connection rejected: ret_code=%u\n", ret_code);
        _client->disconnect();
        return;
    }
    DBG_SERIAL.println("[DJI] Connect ACK received — waiting for camera hello");
}

// Step 5 of handshake: camera sends its own connect request (cmd_type 0x02 = CMD).
// We must ACK with the same seq number, then subscribe to status.
// NOTE: called from inside the BLE notify callback — defer the BLE write to update().
void BLECamera::handleConnectCommand(uint16_t camSeq, const uint8_t *payload,
                                      uint16_t len) {
    if (_djiConnected || _pendingConnectAck) return;   // already handled

    // The camera's hello carries its own device_id (bytes 0-3, LE) — per DJI's
    // published protocol_data_segment.md this identifies the model (e.g. 0xFF33
    // Action 4, 0xFF44 Action 5 Pro, 0xFF55 Action 6, 0xFF66 Osmo 360).
    // Log unknown device IDs rather than guessing model compatibility.
    if (len >= 4) {
        uint32_t camDeviceId = (uint32_t)payload[0] | ((uint32_t)payload[1] << 8) |
                               ((uint32_t)payload[2] << 16) | ((uint32_t)payload[3] << 24);
        _cameraDeviceId = camDeviceId;
        DBG_SERIAL.printf("[DJI] Camera hello (seq=0x%04X) device_id=0x%08X\n",
                          camSeq, camDeviceId);
    } else {
        DBG_SERIAL.printf("[DJI] Camera hello (seq=0x%04X)\n", camSeq);
    }

    // Current R-SDK cameras, including Osmo Nano, put approval result in
    // verify_mode @26 + verify_data @27 (u16 LE). verify_mode=2 / data=0 means
    // approved. A non-zero data value is rejection; don't ACK it as connected.
    if (len >= 29) {
        const uint8_t verifyMode = payload[26];
        const uint16_t verifyData = (uint16_t)payload[27] | ((uint16_t)payload[28] << 8);
        DBG_SERIAL.printf("[DJI] Camera verify mode=%u data=%u\n", verifyMode, verifyData);
        if (verifyMode == 2 && verifyData != 0) {
            DBG_SERIAL.println("[DJI] Camera rejected controller approval");
            if (_client) _client->disconnect();
            return;
        }
        if (_isOsmoNano && verifyMode == 2 && verifyData == 0)
            DBG_SERIAL.println("[DJI] Osmo Nano controller approved");
    }

    DBG_SERIAL.printf("[DJI] Queuing connection ACK seq=0x%04X\n", camSeq);
    _pendingAckSeq     = camSeq;
    _pendingConnectAck = true;
}

void BLECamera::handleRecordAck(const uint8_t *payload, uint16_t len) {
    uint8_t ret = (len > 0) ? payload[0] : 0xFF;
    if (ret == 0)
        DBG_SERIAL.println("[DJI] Record command OK");
    else
        DBG_SERIAL.printf("[DJI] Record command rejected: ret=0x%02X\n", ret);
}

void BLECamera::handleModeSwitchAck(const uint8_t *payload, uint16_t len) {
    uint8_t ret = (len > 0) ? payload[0] : 0xFF;
    if (ret == 0)
        DBG_SERIAL.println("[DJI] Mode switch OK");
    else
        DBG_SERIAL.printf("[DJI] Mode switch rejected: ret=0x%02X\n", ret);
}

void BLECamera::handleCameraStatus(const uint8_t *payload, uint16_t len) {
    if (_isOsmoNano) _nanoLastStatusMs = millis();
    // Current DJI R-SDK bodies share the first seven bytes (mode/status/res/fps/
    // EIS/record-time). Nano support keeps that useful subset even if a future
    // firmware returns a shorter status struct than the 38-byte Action layout.
    if (len < 7) {
        DBG_SERIAL.printf("[DJI] Status push too short: %uB\n", len);
        return;
    }

    const uint8_t mode = payload[0];
    const uint8_t status = payload[1];
    const uint8_t videoResolution = payload[2];
    const uint8_t fpsCode = payload[3];
    const uint8_t eisMode = payload[4];
    const uint16_t recordTime = (uint16_t)payload[5] | ((uint16_t)payload[6] << 8);

    _camera.camera_mode = mode;
    _activeProfile = mode;
    _camera.recording = (status == 0x03 || status == 0x05);
    _camera.has_recording = true;
    _camera.record_time = recordTime;
    _camera.eis_mode = eisMode;

    if (len >= sizeof(DJICameraStatus)) {
        const auto *s = reinterpret_cast<const DJICameraStatus *>(payload);
        _camera.percent = s->bat_percent;
        _camera.has_battery = true;
        _camera.has_temperature = true;
        _camera.has_remain_time = true;
        _camera.temp_over = s->temp_over;
        _camera.remain_cap_mb = s->remain_capacity;
        _camera.remain_time = s->remain_time;
    } else {
        // Osmosis' hardware-tested R-SDK parser treats the last byte as battery
        // on shorter camera-status variants. Surface it when it is plausible,
        // while leaving unsupported temperature/storage fields invalid.
        const uint8_t tail = payload[len - 1];
        if (tail <= 100) {
            _camera.percent = tail;
            _camera.has_battery = true;
        }
        _camera.has_temperature = false;
        _camera.has_remain_time = false;
        _camera.temp_over = 0;
        _camera.remain_cap_mb = 0;
        _camera.remain_time = 0;
        DBG_SERIAL.printf("[DJI] Short status variant %uB (Nano-compatible basic decode)\n", len);
    }

    // video_resolution is a sparse code, not a sequential index.
    switch (videoResolution) {
        case 10:  _camera.resolution = CAM_RES_1080P;   break;
        case 66:  _camera.resolution = CAM_RES_1080P;   break;
        case 16:  _camera.resolution = CAM_RES_4K;      break;
        case 109: _camera.resolution = CAM_RES_4K;      break;
        case 103: _camera.resolution = CAM_RES_4K_WIDE; break;
        case 45:  _camera.resolution = CAM_RES_2_7K;    break;
        case 67:  _camera.resolution = CAM_RES_2_7K;    break;
        case 95:  _camera.resolution = CAM_RES_2_7K;    break;
        default:  _camera.resolution = CAM_RES_UNKNOWN; break;
    }

    switch (fpsCode) {
        case 1:  _camera.fps_idx = CAM_FPS_24;  break;
        case 2:  _camera.fps_idx = CAM_FPS_25;  break;
        case 3:  _camera.fps_idx = CAM_FPS_30;  break;
        case 4:  _camera.fps_idx = CAM_FPS_48;  break;
        case 5:  _camera.fps_idx = CAM_FPS_50;  break;
        case 6:  _camera.fps_idx = CAM_FPS_60;  break;
        case 7:  _camera.fps_idx = CAM_FPS_120; break;
        case 8:  _camera.fps_idx = CAM_FPS_240; break;
        case 10: _camera.fps_idx = CAM_FPS_100; break;
        case 19: _camera.fps_idx = CAM_FPS_200; break;
        default: _camera.fps_idx = CAM_FPS_UNKNOWN; break;
    }

    _camera.valid = true;
    if (_cameraCb) _cameraCb(_camera);

    DBG_SERIAL.printf("[DJI] bat=%s%u%%  mode=0x%02X  rec=%s  eis=%u  time=%us  status_len=%u\n",
                      _camera.has_battery ? "" : "?",
                      _camera.has_battery ? _camera.percent : 0,
                      mode, _camera.recording ? "yes" : "no",
                      eisMode, recordTime, len);
}

// 0x1D/0x06: newer cameras (Action 5 Pro, etc.) push mode name + params as ASCII.
// The frame carries two TLV-ish sections at fixed offsets (46 bytes total).
void BLECamera::handleNewCameraStatus(const uint8_t *payload, uint16_t len) {
    if (len < 46) return;

    char mode_name[21] = {};
    char mode_param[21] = {};
    uint8_t name_len  = payload[1];
    uint8_t param_len = payload[24];
    if (name_len  > 20) name_len  = 20;
    if (param_len > 20) param_len = 20;
    memcpy(mode_name,  payload + 2,  name_len);
    memcpy(mode_param, payload + 25, param_len);

    DBG_SERIAL.printf("[DJI] New status: mode=\"%s\"  param=\"%s\"\n",
                      mode_name, mode_param);
}
