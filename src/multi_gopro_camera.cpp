#include "multi_gopro_camera.h"
#include "config.h"
#include "dji_protocol.h"
#include <BLESecurity.h>

MultiGoProCamera *MultiGoProCamera::_instance = nullptr;

void MultiGoProCamera::begin() {
    _instance = this;
    BLEDevice::init("ESP32-GP-Multi");
    BLEDevice::setPower(_lowPowerMode ? ESP_PWR_LVL_N12 : ESP_PWR_LVL_P9);
    BLEDevice::setMTU(500);

    BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT);
    static BLESecurity security;
    security.setAuthenticationMode(ESP_LE_AUTH_REQ_SC_BOND);
    security.setCapability(ESP_IO_CAP_NONE);
    security.setKeySize(16);
    security.setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
    security.setRespEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);

    _camera.valid = false;
    _camera.has_recording = true;
    startScan(0);
}

uint8_t MultiGoProCamera::connectedCount() const {
    uint8_t n = 0;
    for (const auto &s : _slots) if (s.ready) ++n;
    return n;
}

bool MultiGoProCamera::addressAlreadyUsed(const std::string &addr) const {
    for (const auto &s : _slots) {
        if (!s.addr.empty() && s.addr == addr && (s.bleConnected || s.ready || s.found)) return true;
    }
    return false;
}

int MultiGoProCamera::slotForClient(BLEClient *client) const {
    for (int i = 0; i < 2; ++i) if (_slots[i].client == client) return i;
    return -1;
}

int MultiGoProCamera::slotForNotify(BLERemoteCharacteristic *ch) const {
    for (int i = 0; i < 2; ++i) if (_slots[i].cmdNotify == ch) return i;
    return -1;
}

void MultiGoProCamera::startScan(uint8_t slot) {
    if (_scanning || slot > 1 || _slots[slot].ready || _slots[slot].bleConnected) return;
    _scanSlot = static_cast<int8_t>(slot);
    _scanning = true;
    _slots[slot].found = false;
    _slots[slot].addr.clear();
    _slots[slot].name.clear();

    BLEScan *scan = BLEDevice::getScan();
    scan->setAdvertisedDeviceCallbacks(this, true);
    scan->clearResults();
    scan->setActiveScan(true);
    scan->setInterval(0x50);
    scan->setWindow(0x30);
    DBG_SERIAL.printf("[MULTI] Scanning for GoPro slot %u...\n", slot + 1);
    scan->start(BLE_SCAN_DURATION_SECS, scanDoneCallback, false);
}

void MultiGoProCamera::scanDoneCallback(BLEScanResults) {
    if (!_instance) return;
    MultiGoProCamera *self = _instance;
    self->_scanning = false;
    if (self->_scanSlot < 0 || self->_scanSlot > 1) return;
    Slot &s = self->_slots[self->_scanSlot];
    if (!s.found) {
        DBG_SERIAL.printf("[MULTI] No eligible GoPro found for slot %u\n", self->_scanSlot + 1);
        s.lastAttemptMs = millis();
    }
}

void MultiGoProCamera::onResult(BLEAdvertisedDevice device) {
    if (_scanSlot < 0 || _scanSlot > 1) return;

    bool isGoPro = false;
    std::string mfr;
    if (device.haveManufacturerData()) mfr = device.getManufacturerData();
    if (device.haveServiceUUID() && device.isAdvertisingService(BLEUUID((uint16_t)GP_SERVICE_UUID))) isGoPro = true;
    if (!isGoPro && mfr.size() >= 2 && (uint8_t)mfr[0] == GP_MANUFACTURER_ID_HI && (uint8_t)mfr[1] == GP_MANUFACTURER_ID_LO) isGoPro = true;
    if (!isGoPro && device.haveName() && device.getName().find(GP_DEVICE_NAME_PREFIX) != std::string::npos) isGoPro = true;
    if (!isGoPro) return;

    const std::string addr = device.getAddress().toString();
    if (addressAlreadyUsed(addr)) return;

    // Match the proven V1 wake guard for known modern GoPro advertisement
    // families. Explicit Multi Cam testing should never wake a sleeping camera.
    if (_wakeGuard) {
        bool knownFamily = false;
        bool awake = false;
        if (mfr.size() >= 4 && (uint8_t)mfr[0] == GP_MANUFACTURER_ID_HI && (uint8_t)mfr[1] == GP_MANUFACTURER_ID_LO) {
            const uint8_t family = (uint8_t)mfr[2];
            const uint8_t state = (uint8_t)mfr[3];
            if (family == 0x02) { knownFamily = true; awake = (state == 0x05 || state == 0x01); }
            else if (family == 0x03) { knownFamily = true; awake = (state == 0x01); }
        }
        if (!knownFamily || !awake) return;
    }

    Slot &s = _slots[_scanSlot];
    s.addr = addr;
    s.name = device.haveName() ? device.getName() : "GoPro";
    s.addrType = device.getAddressType();
    s.found = true;
    DBG_SERIAL.printf("[MULTI] Slot %u found %s (%s) rssi=%d\n",
                      _scanSlot + 1, s.name.c_str(), s.addr.c_str(), device.getRSSI());
    BLEDevice::getScan()->stop();
    _scanning = false;
}

bool MultiGoProCamera::connectSlot(uint8_t slot) {
    if (slot > 1) return false;
    Slot &s = _slots[slot];
    if (!s.found || s.addr.empty()) return false;

    if (!s.client) {
        s.client = BLEDevice::createClient();
        s.client->setClientCallbacks(this);
    }

    DBG_SERIAL.printf("[MULTI] Connecting GoPro slot %u -> %s\n", slot + 1, s.addr.c_str());
    if (!s.client->connect(BLEAddress(s.addr), s.addrType)) {
        DBG_SERIAL.printf("[MULTI] Slot %u connection failed\n", slot + 1);
        s.found = false;
        s.lastAttemptMs = millis();
        return false;
    }

    s.client->setMTU(500);
    BLERemoteService *svc = s.client->getService(BLEUUID((uint16_t)GP_SERVICE_UUID));
    if (!svc) {
        DBG_SERIAL.printf("[MULTI] Slot %u GoPro service missing\n", slot + 1);
        s.client->disconnect();
        s.found = false;
        s.lastAttemptMs = millis();
        return false;
    }

    s.cmdWrite = svc->getCharacteristic(BLEUUID(GP_CHAR_CMD_WRITE));
    s.cmdNotify = svc->getCharacteristic(BLEUUID(GP_CHAR_CMD_NOTIFY));
    if (!s.cmdWrite || !s.cmdWrite->canWrite() || !s.cmdNotify || !s.cmdNotify->canNotify()) {
        DBG_SERIAL.printf("[MULTI] Slot %u required Open GoPro chars missing\n", slot + 1);
        s.client->disconnect();
        s.found = false;
        s.lastAttemptMs = millis();
        return false;
    }

    s.cmdNotify->registerForNotify(cmdNotifyCallback);
    s.bleConnected = true;
    s.pendingHwInfo = true;
    return true;
}

void MultiGoProCamera::onConnect(BLEClient *client) {
    const int slot = slotForClient(client);
    if (slot >= 0) DBG_SERIAL.printf("[MULTI] BLE connected slot %d\n", slot + 1);
}

void MultiGoProCamera::onDisconnect(BLEClient *client) {
    const int slot = slotForClient(client);
    if (slot < 0) return;
    Slot &s = _slots[slot];
    DBG_SERIAL.printf("[MULTI] GoPro slot %d disconnected\n", slot + 1);
    s.cmdWrite = nullptr;
    s.cmdNotify = nullptr;
    s.bleConnected = false;
    s.ready = false;
    s.pendingHwInfo = false;
    s.found = false;
    s.addr.clear();
    s.name.clear();
    s.rx.reset();
    s.lastAttemptMs = millis();
    publishState();
}

void MultiGoProCamera::sendHardwareInfo(uint8_t slot) {
    Slot &s = _slots[slot];
    if (!s.cmdWrite || !s.bleConnected) return;
    const uint8_t buf[] = {0x01, GP_CMD_GET_HARDWARE_INFO};
    s.cmdWrite->writeValue(const_cast<uint8_t *>(buf), sizeof(buf), false);
    DBG_SERIAL.printf("[MULTI] Slot %u hardware-info query sent\n", slot + 1);
}

void MultiGoProCamera::cmdNotifyCallback(BLERemoteCharacteristic *ch,
                                         uint8_t *data, size_t len, bool) {
    if (!_instance) return;
    const int slot = _instance->slotForNotify(ch);
    if (slot >= 0) _instance->handleCmdNotify((uint8_t)slot, data, len);
}

void MultiGoProCamera::handleCmdNotify(uint8_t slot, uint8_t *data, size_t len) {
    Slot &s = _slots[slot];
    if (!s.rx.feed(data, len)) return;
    if (s.rx.expected < 2) { s.rx.reset(); return; }

    const uint8_t cmd = s.rx.buf[0];
    const uint8_t status = s.rx.buf[1];
    if (cmd == GP_CMD_GET_HARDWARE_INFO) {
        if (status == 0) {
            s.ready = true;
            DBG_SERIAL.printf("[MULTI] GoPro slot %u READY (%s)\n", slot + 1, s.name.c_str());
            if (_registry) _registry->onConnected(s.name.c_str(), s.addr.c_str(), (uint8_t)s.addrType, 1);
            publishState();
            syncSlotToDesiredState(slot);
        } else {
            DBG_SERIAL.printf("[MULTI] Slot %u hardware info rejected 0x%02X; retrying\n", slot + 1, status);
            s.pendingHwInfo = true;
        }
    } else if (cmd == GP_CMD_SET_SHUTTER) {
        DBG_SERIAL.printf("[MULTI] Slot %u shutter %s\n", slot + 1, status == 0 ? "OK" : "REJECTED");
    }
    s.rx.reset();
}

bool MultiGoProCamera::sendShutter(uint8_t slot, bool on) {
    Slot &s = _slots[slot];
    if (!s.ready || !s.cmdWrite) return false;
    const uint8_t buf[] = {0x03, GP_CMD_SET_SHUTTER, 0x01, (uint8_t)(on ? 0x01 : 0x00)};
    s.cmdWrite->writeValue(const_cast<uint8_t *>(buf), sizeof(buf), false);
    DBG_SERIAL.printf("[MULTI] Slot %u shutter -> %s\n", slot + 1, on ? "START" : "STOP");
    return true;
}

void MultiGoProCamera::syncSlotToDesiredState(uint8_t slot) {
    Slot &s = _slots[slot];
    if (!s.ready) return;
    if (_recordingRequested) {
        // Camera returned while the quad still wants recording: catch it up.
        sendShutter(slot, true);
        s.needsStopOnReconnect = false;
        DBG_SERIAL.printf("[MULTI] Slot %u rejoined during recording -> START replayed\n", slot + 1);
    } else if (s.needsStopOnReconnect) {
        // Camera missed STOP while out of range. As soon as it comes back into
        // BLE range (for example after landing next to a chest-mounted camera),
        // stop it automatically instead of leaving it recording indefinitely.
        sendShutter(slot, false);
        s.needsStopOnReconnect = false;
        DBG_SERIAL.printf("[MULTI] Slot %u rejoined after missed stop -> STOP replayed\n", slot + 1);
    }
}

bool MultiGoProCamera::startRecording() {
    _recordingRequested = true;
    _recordStartedMs = millis();
    bool any = false;
    for (uint8_t i = 0; i < 2; ++i) {
        _slots[i].needsStopOnReconnect = false;
        any = sendShutter(i, true) || any;
    }
    DBG_SERIAL.printf("[MULTI] START fan-out -> %u connected camera(s)\n", connectedCount());
    publishState();
    return any;
}

bool MultiGoProCamera::stopRecording() {
    _recordingRequested = false;
    bool any = false;
    for (uint8_t i = 0; i < 2; ++i) {
        if (_slots[i].ready) {
            any = sendShutter(i, false) || any;
            _slots[i].needsStopOnReconnect = false;
        } else {
            // If this slot was away when STOP happened, remember the missed
            // command and replay it after that same logical slot reconnects.
            _slots[i].needsStopOnReconnect = true;
        }
    }
    DBG_SERIAL.printf("[MULTI] STOP fan-out -> %u connected camera(s); missing slots queued for STOP on reconnect\n", connectedCount());
    publishState();
    return any;
}

bool MultiGoProCamera::switchCameraMode(uint8_t mode) {
    uint8_t group = GP_PRESET_VIDEO;
    if (mode == DJI_MODE_PHOTO) group = GP_PRESET_PHOTO;
    else if (mode == DJI_MODE_TIMELAPSE || mode == DJI_MODE_HYPERLAPSE) group = GP_PRESET_TIMELAPSE;
    bool any = false;
    for (uint8_t i = 0; i < 2; ++i) {
        Slot &s = _slots[i];
        if (!s.ready || !s.cmdWrite) continue;
        const uint8_t buf[] = {0x03, GP_CMD_LOAD_PRESET_GROUP, 0x01, group};
        s.cmdWrite->writeValue(const_cast<uint8_t *>(buf), sizeof(buf), false);
        any = true;
    }
    return any;
}

void MultiGoProCamera::publishState() {
    const uint8_t count = connectedCount();
    _camera.valid = count > 0;
    _camera.has_recording = true;
    _camera.recording = _recordingRequested && count > 0;
    _camera.record_time = _camera.recording ? (uint16_t)((millis() - _recordStartedMs) / 1000UL) : 0;
    if (_cameraCb) _cameraCb(_camera);
}

void MultiGoProCamera::update() {
    for (uint8_t i = 0; i < 2; ++i) {
        Slot &s = _slots[i];
        if (s.pendingHwInfo && s.bleConnected) {
            s.pendingHwInfo = false;
            sendHardwareInfo(i);
            return;
        }
    }

    // Connect a camera found by the current scan before starting another scan.
    for (uint8_t i = 0; i < 2; ++i) {
        Slot &s = _slots[i];
        if (s.found && !s.bleConnected && !s.ready) {
            connectSlot(i);
            return;
        }
    }

    if (_scanning) return;

    // Fill slot 0 first, then look for a distinct second GoPro. Missing slots
    // are retried independently so one camera dropping out never blocks the other.
    for (uint8_t i = 0; i < 2; ++i) {
        Slot &s = _slots[i];
        if (!s.ready && !s.bleConnected && !s.found && millis() - s.lastAttemptMs >= 750UL) {
            startScan(i);
            return;
        }
    }

    if (_camera.recording) {
        _camera.record_time = (uint16_t)((millis() - _recordStartedMs) / 1000UL);
        if (_cameraCb) _cameraCb(_camera);
    }
}
