#include "multi_gopro_camera.h"
#include "config.h"
#include "dji_protocol.h"
#include <BLESecurity.h>
#include <cstring>

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
    for (const auto &s : _slots)
        if (!s.addr.empty() && s.addr == addr && (s.bleConnected || s.ready || s.found)) return true;
    return false;
}

int MultiGoProCamera::slotForClient(BLEClient *client) const {
    for (int i = 0; i < MAX_MULTI_GOPRO_SLOTS; ++i) if (_slots[i].client == client) return i;
    return -1;
}

int MultiGoProCamera::slotForNotify(BLERemoteCharacteristic *ch) const {
    for (int i = 0; i < MAX_MULTI_GOPRO_SLOTS; ++i) {
        const Slot &s = _slots[i];
        if (s.cmdNotify == ch || s.settingNotify == ch || s.queryNotify == ch) return i;
    }
    return -1;
}

void MultiGoProCamera::startScan(uint8_t slot) {
    if (!_scanEnabled) return;
    if (_scanning || slot >= MAX_MULTI_GOPRO_SLOTS || _slots[slot].ready || _slots[slot].bleConnected) return;
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
    if (self->_scanSlot < 0 || self->_scanSlot >= MAX_MULTI_GOPRO_SLOTS) return;
    Slot &s = self->_slots[self->_scanSlot];
    if (!s.found) {
        DBG_SERIAL.printf("[MULTI] No eligible GoPro found for slot %u\n", self->_scanSlot + 1);
        s.lastAttemptMs = millis();
    }
}

void MultiGoProCamera::onResult(BLEAdvertisedDevice device) {
    if (_scanSlot < 0 || _scanSlot >= MAX_MULTI_GOPRO_SLOTS) return;
    bool isGoPro = false;
    std::string mfr;
    if (device.haveManufacturerData()) mfr = device.getManufacturerData();
    if (device.haveServiceUUID() && device.isAdvertisingService(BLEUUID((uint16_t)GP_SERVICE_UUID))) isGoPro = true;
    if (!isGoPro && mfr.size() >= 2 && (uint8_t)mfr[0] == GP_MANUFACTURER_ID_HI && (uint8_t)mfr[1] == GP_MANUFACTURER_ID_LO) isGoPro = true;
    if (!isGoPro && device.haveName() && device.getName().find(GP_DEVICE_NAME_PREFIX) != std::string::npos) isGoPro = true;
    if (!isGoPro) return;
    const std::string addr = device.getAddress().toString();
    if (addressAlreadyUsed(addr)) return;

    if (_wakeGuard) {
        bool knownFamily = false, awake = false;
        if (mfr.size() >= 4 && (uint8_t)mfr[0] == GP_MANUFACTURER_ID_HI && (uint8_t)mfr[1] == GP_MANUFACTURER_ID_LO) {
            const uint8_t family = (uint8_t)mfr[2], state = (uint8_t)mfr[3];
            if (family == 0x02) { knownFamily = true; awake = (state == 0x05 || state == 0x01); }
            else if (family == 0x03) { knownFamily = true; awake = (state == 0x01 || state == 0x05); }
        }
        if (!knownFamily || !awake) return;
    }

    Slot &s = _slots[_scanSlot];
    s.addr = addr;
    s.name = device.haveName() ? device.getName() : "GoPro";
    s.addrType = device.getAddressType();
    s.found = true;
    DBG_SERIAL.printf("[MULTI] Slot %u found %s (%s) rssi=%d\n", _scanSlot + 1, s.name.c_str(), s.addr.c_str(), device.getRSSI());
    BLEDevice::getScan()->stop();
    _scanning = false;
}

bool MultiGoProCamera::connectSlot(uint8_t slot) {
    if (slot >= MAX_MULTI_GOPRO_SLOTS) return false;
    Slot &s = _slots[slot];
    if (!s.found || s.addr.empty()) return false;
    if (!s.client) {
        s.client = BLEDevice::createClient();
        s.client->setClientCallbacks(this);
    }
    DBG_SERIAL.printf("[MULTI] Connecting GoPro slot %u -> %s\n", slot + 1, s.addr.c_str());
    if (!s.client->connect(BLEAddress(s.addr), s.addrType)) {
        DBG_SERIAL.printf("[MULTI] Slot %u connection failed\n", slot + 1);
        s.found = false; s.lastAttemptMs = millis();
        return false;
    }

    if (s.client->setMTU(500)) DBG_SERIAL.printf("[MULTI] Slot %u MTU=%u\n", slot + 1, s.client->getMTU());
    else DBG_SERIAL.printf("[MULTI] Slot %u MTU exchange failed; using default\n", slot + 1);

    BLERemoteService *svc = s.client->getService(BLEUUID((uint16_t)GP_SERVICE_UUID));
    if (!svc) { s.client->disconnect(); s.found = false; s.lastAttemptMs = millis(); return false; }

    s.cmdNotify = svc->getCharacteristic(BLEUUID(GP_CHAR_CMD_NOTIFY));
    s.queryNotify = svc->getCharacteristic(BLEUUID(GP_CHAR_QUERY_NOTIFY));
    s.cmdWrite = svc->getCharacteristic(BLEUUID(GP_CHAR_CMD_WRITE));
    s.settingWrite = svc->getCharacteristic(BLEUUID(GP_CHAR_SETTING_WRITE));
    s.settingNotify = svc->getCharacteristic(BLEUUID(GP_CHAR_SETTING_NOTIFY));
    s.queryWrite = svc->getCharacteristic(BLEUUID(GP_CHAR_QUERY_WRITE));

    const bool ok = s.cmdNotify && s.cmdNotify->canNotify() &&
                    s.queryNotify && s.queryNotify->canNotify() &&
                    s.cmdWrite && s.cmdWrite->canWrite() &&
                    s.settingWrite && s.settingWrite->canWrite() &&
                    s.settingNotify && s.settingNotify->canNotify() &&
                    s.queryWrite && s.queryWrite->canWrite();
    if (!ok) {
        DBG_SERIAL.printf("[MULTI] Slot %u missing required Open GoPro characteristic(s)\n", slot + 1);
        s.client->disconnect(); s.found = false; s.lastAttemptMs = millis();
        return false;
    }

    // Match the proven V1 subscription order exactly: command, query, setting.
    s.cmdNotify->registerForNotify(cmdNotifyCallback);
    s.queryNotify->registerForNotify(queryNotifyCallback);
    s.settingNotify->registerForNotify(settingNotifyCallback);

    s.bleConnected = true;
    s.pendingHwInfo = true;
    s.pendingRegisterSettings = false;
    s.pendingRegisterStatus = false;
    s.lastKeepAliveMs = millis();
    DBG_SERIAL.printf("[MULTI] Slot %u BLE transport ready; starting Open GoPro handshake\n", slot + 1);
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
    s.cmdWrite = s.cmdNotify = s.settingWrite = s.settingNotify = s.queryWrite = s.queryNotify = nullptr;
    s.bleConnected = s.ready = s.pendingHwInfo = s.pendingRegisterSettings = s.pendingRegisterStatus = s.found = false;
    s.addr.clear(); s.name.clear();
    s.cmdRx.reset(); s.settingRx.reset(); s.queryRx.reset();
    s.lastAttemptMs = millis(); s.lastKeepAliveMs = 0;
    publishState();
}

void MultiGoProCamera::sendHardwareInfo(uint8_t slot) {
    Slot &s = _slots[slot]; if (!s.cmdWrite || !s.bleConnected) return;
    const uint8_t buf[] = {0x01, GP_CMD_GET_HARDWARE_INFO};
    s.cmdWrite->writeValue(const_cast<uint8_t *>(buf), sizeof(buf), false);
    DBG_SERIAL.printf("[MULTI] Slot %u hardware-info query sent\n", slot + 1);
}

void MultiGoProCamera::sendRegisterSettings(uint8_t slot) {
    Slot &s = _slots[slot]; if (!s.queryWrite || !s.bleConnected) return;
    const uint8_t ids[] = { GP_SETTING_RESOLUTION, GP_SETTING_FPS, GP_SETTING_HYPERSMOOTH };
    const uint8_t payloadLen = 1 + sizeof(ids);
    uint8_t buf[2 + sizeof(ids)];
    buf[0] = payloadLen & 0x1F; buf[1] = GP_QUERY_REGISTER_SETTING;
    memcpy(buf + 2, ids, sizeof(ids));
    s.queryWrite->writeValue(buf, 1 + payloadLen, false);
    DBG_SERIAL.printf("[MULTI] Slot %u register settings sent\n", slot + 1);
}

void MultiGoProCamera::sendRegisterStatus(uint8_t slot) {
    Slot &s = _slots[slot]; if (!s.queryWrite || !s.bleConnected) return;
    const uint8_t ids[] = {
        GP_STATUS_OVERHEATING, GP_STATUS_ENCODING, GP_STATUS_LEGACY_RECORDING,
        GP_STATUS_ENC_DURATION, GP_STATUS_PRIMARY_STORAGE, GP_STATUS_REMAINING_TIME,
        GP_STATUS_LEGACY_MODE, GP_STATUS_SD_REMAINING, GP_STATUS_BATTERY_PCT,
        GP_STATUS_PRESET_GROUP
    };
    const uint8_t payloadLen = 1 + sizeof(ids);
    uint8_t buf[2 + sizeof(ids)];
    buf[0] = payloadLen & 0x1F; buf[1] = GP_QUERY_REGISTER_STATUS;
    memcpy(buf + 2, ids, sizeof(ids));
    s.queryWrite->writeValue(buf, 1 + payloadLen, false);
    DBG_SERIAL.printf("[MULTI] Slot %u register status sent\n", slot + 1);
}

void MultiGoProCamera::sendKeepAlive(uint8_t slot) {
    Slot &s = _slots[slot];
    if (!s.settingWrite || !s.bleConnected) return;
    // Same official Open GoPro keep-alive used by the proven V1 backend.
    uint8_t buf[] = {0x03, 91, 0x01, 66};
    s.settingWrite->writeValue(buf, sizeof(buf), false);
    DBG_SERIAL.printf("[MULTI] Slot %u keep-alive\n", slot + 1);
}

void MultiGoProCamera::cmdNotifyCallback(BLERemoteCharacteristic *ch, uint8_t *data, size_t len, bool) {
    if (!_instance) return; const int slot = _instance->slotForNotify(ch);
    if (slot >= 0) _instance->handleCmdNotify((uint8_t)slot, data, len);
}
void MultiGoProCamera::settingNotifyCallback(BLERemoteCharacteristic *ch, uint8_t *data, size_t len, bool) {
    if (!_instance) return; const int slot = _instance->slotForNotify(ch);
    if (slot >= 0) _instance->handleSettingNotify((uint8_t)slot, data, len);
}
void MultiGoProCamera::queryNotifyCallback(BLERemoteCharacteristic *ch, uint8_t *data, size_t len, bool) {
    if (!_instance) return; const int slot = _instance->slotForNotify(ch);
    if (slot >= 0) _instance->handleQueryNotify((uint8_t)slot, data, len);
}

void MultiGoProCamera::handleCmdNotify(uint8_t slot, uint8_t *data, size_t len) {
    Slot &s = _slots[slot];
    if (!s.cmdRx.feed(data, len)) return;
    if (s.cmdRx.expected >= 2) {
        const uint8_t cmd = s.cmdRx.buf[0], status = s.cmdRx.buf[1];
        if (cmd == GP_CMD_GET_HARDWARE_INFO) {
            if (status == 0) {
                DBG_SERIAL.printf("[MULTI] Slot %u hardware info OK\n", slot + 1);
                s.pendingRegisterSettings = true;
            } else {
                DBG_SERIAL.printf("[MULTI] Slot %u hardware info rejected 0x%02X; retrying\n", slot + 1, status);
                s.pendingHwInfo = true;
            }
        } else if (cmd == GP_CMD_SET_SHUTTER) {
            DBG_SERIAL.printf("[MULTI] Slot %u shutter %s\n", slot + 1, status == 0 ? "OK" : "REJECTED");
        }
    }
    s.cmdRx.reset();
}

void MultiGoProCamera::handleSettingNotify(uint8_t slot, uint8_t *data, size_t len) {
    Slot &s = _slots[slot];
    if (s.settingRx.feed(data, len)) s.settingRx.reset();
}

void MultiGoProCamera::handleQueryNotify(uint8_t slot, uint8_t *data, size_t len) {
    Slot &s = _slots[slot];
    if (!s.queryRx.feed(data, len)) return;
    if (s.queryRx.expected >= 2) {
        const uint8_t query = s.queryRx.buf[0], status = s.queryRx.buf[1];
        if (query == GP_QUERY_REGISTER_SETTING) {
            // V1 proceeds to status registration even if optional setting telemetry is rejected.
            DBG_SERIAL.printf("[MULTI] Slot %u setting registration status=0x%02X\n", slot + 1, status);
            s.pendingRegisterStatus = true;
        } else if (query == GP_QUERY_REGISTER_STATUS) {
            DBG_SERIAL.printf("[MULTI] Slot %u status registration status=0x%02X -> READY\n", slot + 1, status);
            s.ready = true;
            s.lastKeepAliveMs = millis();
            if (_registry) _registry->onConnected(s.name.c_str(), s.addr.c_str(), (uint8_t)s.addrType, 1);
            publishState();
            syncSlotToDesiredState(slot);
        }
    }
    s.queryRx.reset();
}

bool MultiGoProCamera::sendShutter(uint8_t slot, bool on) {
    Slot &s = _slots[slot]; if (!s.ready || !s.cmdWrite) return false;
    const uint8_t buf[] = {0x03, GP_CMD_SET_SHUTTER, 0x01, (uint8_t)(on ? 0x01 : 0x00)};
    s.cmdWrite->writeValue(const_cast<uint8_t *>(buf), sizeof(buf), false);
    DBG_SERIAL.printf("[MULTI] Slot %u shutter -> %s\n", slot + 1, on ? "START" : "STOP");
    return true;
}

void MultiGoProCamera::syncSlotToDesiredState(uint8_t slot) {
    Slot &s = _slots[slot]; if (!s.ready) return;
    if (_recordingRequested) {
        sendShutter(slot, true); s.needsStopOnReconnect = false;
    } else if (s.needsStopOnReconnect) {
        sendShutter(slot, false); s.needsStopOnReconnect = false;
    }
}

bool MultiGoProCamera::startRecording() {
    _recordingRequested = true; _recordStartedMs = millis(); bool any = false;
    for (uint8_t i = 0; i < MAX_MULTI_GOPRO_SLOTS; ++i) { _slots[i].needsStopOnReconnect = false; any = sendShutter(i, true) || any; }
    publishState(); return any;
}

bool MultiGoProCamera::stopRecording() {
    _recordingRequested = false; bool any = false;
    for (uint8_t i = 0; i < MAX_MULTI_GOPRO_SLOTS; ++i) {
        if (_slots[i].ready) { any = sendShutter(i, false) || any; _slots[i].needsStopOnReconnect = false; }
        else _slots[i].needsStopOnReconnect = true;
    }
    publishState(); return any;
}

bool MultiGoProCamera::switchCameraMode(uint8_t mode) {
    uint8_t group = GP_PRESET_VIDEO;
    if (mode == DJI_MODE_PHOTO) group = GP_PRESET_PHOTO;
    else if (mode == DJI_MODE_TIMELAPSE || mode == DJI_MODE_HYPERLAPSE) group = GP_PRESET_TIMELAPSE;
    bool any = false;
    for (uint8_t i = 0; i < MAX_MULTI_GOPRO_SLOTS; ++i) {
        Slot &s = _slots[i]; if (!s.ready || !s.cmdWrite) continue;
        const uint8_t buf[] = {0x03, GP_CMD_LOAD_PRESET_GROUP, 0x01, group};
        s.cmdWrite->writeValue(const_cast<uint8_t *>(buf), sizeof(buf), false); any = true;
    }
    return any;
}

void MultiGoProCamera::publishState() {
    const uint8_t count = connectedCount();
    _camera.valid = count > 0; _camera.has_recording = true; _camera.connected_cameras = count;
    _camera.recording = _recordingRequested && count > 0;
    _camera.record_time = _camera.recording ? (uint16_t)((millis() - _recordStartedMs) / 1000UL) : 0;
    if (_cameraCb) _cameraCb(_camera);
}

void MultiGoProCamera::update() {
    // Mirror V1's deferred-write handshake per camera. Never write from a BLE notify callback.
    for (uint8_t i = 0; i < MAX_MULTI_GOPRO_SLOTS; ++i) {
        Slot &s = _slots[i];
        if (s.pendingHwInfo && s.bleConnected) { s.pendingHwInfo = false; sendHardwareInfo(i); return; }
        if (s.pendingRegisterSettings && s.bleConnected) { s.pendingRegisterSettings = false; sendRegisterSettings(i); return; }
        if (s.pendingRegisterStatus && s.bleConnected) { s.pendingRegisterStatus = false; sendRegisterStatus(i); return; }
    }

    const uint32_t now = millis();
    // Critical parity fix: each owned GoPro BLE session gets the official 10s keep-alive.
    for (uint8_t i = 0; i < MAX_MULTI_GOPRO_SLOTS; ++i) {
        Slot &s = _slots[i];
        if (s.ready && s.bleConnected && now - s.lastKeepAliveMs >= 10000UL) {
            s.lastKeepAliveMs = now;
            sendKeepAlive(i);
            return;
        }
    }

    for (uint8_t i = 0; i < MAX_MULTI_GOPRO_SLOTS; ++i) {
        Slot &s = _slots[i];
        if (s.found && !s.bleConnected && !s.ready) { connectSlot(i); return; }
    }
    if (_scanning) return;
    for (uint8_t i = 0; i < MAX_MULTI_GOPRO_SLOTS; ++i) {
        Slot &s = _slots[i];
        if (!s.ready && !s.bleConnected && !s.found && now - s.lastAttemptMs >= 750UL) { startScan(i); return; }
    }
    if (_camera.recording) {
        _camera.record_time = (uint16_t)((now - _recordStartedMs) / 1000UL);
        if (_cameraCb) _cameraCb(_camera);
    }
}