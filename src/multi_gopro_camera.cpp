#include "multi_gopro_camera.h"
#include "config.h"
#include "dji_protocol.h"
#include <BLESecurity.h>
#include <cstring>
#include <new>

static esp_power_level_t selectedBlePower(bool advanced, int8_t dbm, bool low) {
    if (!advanced) return low ? ESP_PWR_LVL_N12 : ESP_PWR_LVL_P9;
#if defined(CONFIG_IDF_TARGET_ESP32C3)
    if (dbm <= -24) return ESP_PWR_LVL_N24;
    if (dbm <= -21) return ESP_PWR_LVL_N21;
    if (dbm <= -18) return ESP_PWR_LVL_N18;
    if (dbm <= -15) return ESP_PWR_LVL_N15;
#else
    // Classic ESP32 Arduino core bottoms out at -12 dBm.
    if (dbm < -12) dbm = -12;
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

MultiGoProCamera *MultiGoProCamera::_instance = nullptr;

MultiGoProCamera::Slot *MultiGoProCamera::ensureSlot(uint8_t slot) {
    if (slot >= MAX_MULTI_GOPRO_SLOTS) return nullptr;
    if (!_slots[slot]) {
        _slots[slot] = new (std::nothrow) Slot();
        if (!_slots[slot]) DBG_SERIAL.printf("[MULTI] Unable to allocate GoPro slot %u; memory limit reached\n", slot + 1);
    }
    return _slots[slot];
}

void MultiGoProCamera::begin() {
    _instance = this;
    BLEDevice::init("ESP32-GP-Multi");
    BLEDevice::setPower(selectedBlePower(_advancedPowerMode, _advancedPowerDbm, _lowPowerMode));
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
    for (const Slot *s : _slots) if (s && s->ready) ++n;
    return n;
}

uint8_t MultiGoProCamera::recordingCount() const {
    uint8_t n = 0;
    for (const Slot *s : _slots) if (s && s->ready && s->recordingKnown && s->recording) ++n;
    return n;
}

bool MultiGoProCamera::addressAlreadyUsed(const std::string &addr) const {
    for (const Slot *s : _slots)
        if (s && !s->addr.empty() && s->addr == addr && (s->bleConnected || s->ready || s->found)) return true;
    return false;
}

int MultiGoProCamera::slotForClient(BLEClient *client) const {
    for (int i = 0; i < MAX_MULTI_GOPRO_SLOTS; ++i) if (_slots[i] && _slots[i]->client == client) return i;
    return -1;
}

int MultiGoProCamera::slotForNotify(BLERemoteCharacteristic *ch) const {
    for (int i = 0; i < MAX_MULTI_GOPRO_SLOTS; ++i) {
        const Slot *s = _slots[i];
        if (s && (s->cmdNotify == ch || s->settingNotify == ch || s->queryNotify == ch)) return i;
    }
    return -1;
}

void MultiGoProCamera::startScan(uint8_t slot) {
    if (!_scanEnabled) return;
    if (_scanning || slot >= MAX_MULTI_GOPRO_SLOTS) return;
    Slot *sp = ensureSlot(slot);
    if (!sp || sp->ready || sp->bleConnected) return;
    _scanSlot = static_cast<int8_t>(slot);
    _scanning = true;
    sp->found = false;
    sp->addr.clear();
    sp->name.clear();
    BLEScan *scan = BLEDevice::getScan();
    scan->setAdvertisedDeviceCallbacks(this, true);
    scan->clearResults();
    scan->setActiveScan(true);
    scan->setInterval(0x50);
    scan->setWindow(0x30);
    DBG_SERIAL.printf("[MULTI] Scanning for GoPro slot %u/%u...\n", slot + 1, MAX_MULTI_GOPRO_SLOTS);
    scan->start(BLE_SCAN_DURATION_SECS, scanDoneCallback, false);
}

void MultiGoProCamera::scanDoneCallback(BLEScanResults) {
    if (!_instance) return;
    MultiGoProCamera *self = _instance;
    self->_scanning = false;
    if (self->_scanSlot < 0 || self->_scanSlot >= MAX_MULTI_GOPRO_SLOTS) return;
    Slot *sp = self->slotAt(static_cast<uint8_t>(self->_scanSlot));
    if (!sp) return;
    Slot &s = *sp;
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

    Slot *sp = ensureSlot(static_cast<uint8_t>(_scanSlot));
    if (!sp) return;
    Slot &s = *sp;
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
    Slot &s = *_slots[slot];
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

    s.cmdNotify->registerForNotify(cmdNotifyCallback);
    s.queryNotify->registerForNotify(queryNotifyCallback);
    s.settingNotify->registerForNotify(settingNotifyCallback);

    s.bleConnected = true;
    s.recordingKnown = false;
    s.pendingShutter = false;
    s.pendingHwInfo = true;
    s.pendingRegisterSettings = false;
    s.pendingRegisterStatus = false;
    s.telemetry = CameraData{};
    s.cameraNumber = 0;
    s.cameraLabel[0] = '\0';
    s.lastKeepAliveMs = millis();
    s.lastStatusPollMs = 0;
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
    Slot &s = *_slots[slot];
    DBG_SERIAL.printf("[MULTI] GoPro slot %d disconnected\n", slot + 1);
    s.cmdWrite = s.cmdNotify = s.settingWrite = s.settingNotify = s.queryWrite = s.queryNotify = nullptr;
    s.bleConnected = s.ready = s.pendingHwInfo = s.pendingRegisterSettings = s.pendingRegisterStatus = s.found = false;
    s.recording = false; s.recordingKnown = false; s.pendingShutter = false;
    s.telemetry = CameraData{}; s.cameraNumber = 0; s.cameraLabel[0] = '\0';
    s.addr.clear(); s.name.clear();
    s.cmdRx.reset(); s.settingRx.reset(); s.queryRx.reset();
    s.lastAttemptMs = millis(); s.lastKeepAliveMs = 0; s.lastStatusPollMs = 0;
    publishState();
}

void MultiGoProCamera::sendHardwareInfo(uint8_t slot) {
    Slot &s = *_slots[slot]; if (!s.cmdWrite || !s.bleConnected) return;
    const uint8_t buf[] = {0x01, GP_CMD_GET_HARDWARE_INFO};
    s.cmdWrite->writeValue(const_cast<uint8_t *>(buf), sizeof(buf), false);
    DBG_SERIAL.printf("[MULTI] Slot %u hardware-info query sent\n", slot + 1);
}

void MultiGoProCamera::sendRegisterSettings(uint8_t slot) {
    Slot &s = *_slots[slot]; if (!s.queryWrite || !s.bleConnected) return;
    const uint8_t ids[] = { GP_SETTING_RESOLUTION, GP_SETTING_FPS, GP_SETTING_HYPERSMOOTH };
    const uint8_t payloadLen = 1 + sizeof(ids);
    uint8_t buf[2 + sizeof(ids)];
    buf[0] = payloadLen & 0x1F; buf[1] = GP_QUERY_REGISTER_SETTING;
    memcpy(buf + 2, ids, sizeof(ids));
    s.queryWrite->writeValue(buf, 1 + payloadLen, false);
    DBG_SERIAL.printf("[MULTI] Slot %u register settings sent\n", slot + 1);
}

void MultiGoProCamera::sendRegisterStatus(uint8_t slot) {
    Slot &s = *_slots[slot]; if (!s.queryWrite || !s.bleConnected) return;
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
    Slot &s = *_slots[slot];
    if (!s.settingWrite || !s.bleConnected) return;
    uint8_t buf[] = {0x03, 91, 0x01, 66};
    s.settingWrite->writeValue(buf, sizeof(buf), false);
    DBG_SERIAL.printf("[MULTI] Slot %u keep-alive addr=%s\n", slot + 1, s.addr.c_str());
}

void MultiGoProCamera::sendStatusPoll(uint8_t slot) {
    Slot &s = *_slots[slot];
    if (!s.ready || !s.queryWrite || !s.bleConnected) return;
    const uint8_t ids[] = {
        GP_STATUS_BATTERY_PCT,
        GP_STATUS_REMAINING_TIME,
        GP_STATUS_OVERHEATING,
        GP_STATUS_ENCODING,
        GP_STATUS_LEGACY_RECORDING,
        GP_STATUS_ENC_DURATION
    };
    const uint8_t payloadLen = 1 + sizeof(ids);
    uint8_t buf[2 + sizeof(ids)];
    buf[0] = payloadLen & 0x1F;
    buf[1] = GP_QUERY_GET_STATUS;
    memcpy(buf + 2, ids, sizeof(ids));
    s.queryWrite->writeValue(buf, 1 + payloadLen, false);
}

void MultiGoProCamera::logSlotTelemetry(uint8_t slot) {
    if (slot >= MAX_MULTI_GOPRO_SLOTS) return;
    Slot &s = *_slots[slot];
    if (!s.ready) return;
    stampSlotIdentity(slot);
    const CameraData &d = s.telemetry;
    const int batt = d.has_battery ? (int)d.percent : -1;
    const long remain = d.has_remain_time ? (long)d.remain_time : -1L;
    const int rec = s.recordingKnown ? (s.recording ? 1 : 0) : -1;
    const int hot = d.has_temperature ? (d.temp_over ? 1 : 0) : -1;
    DBG_SERIAL.printf("[MULTI] LIVE slot=%u addr=%s bat=%d remain=%ld rec=%d hot=%d\n",
                      slot + 1, s.addr.c_str(), batt, remain, rec, hot);
}

void MultiGoProCamera::stampSlotIdentity(uint8_t slot) {
    if (slot >= MAX_MULTI_GOPRO_SLOTS || !_registry) return;
    Slot &s = *_slots[slot];
    uint8_t number = 0;
    char label[CAMREG_LABEL_LEN] = {};
    if (_registry->identityForAddr(s.addr.c_str(), number, label, sizeof(label))) {
        s.cameraNumber = number;
        strlcpy(s.cameraLabel, label, sizeof(s.cameraLabel));
    }
}

void MultiGoProCamera::parseSlotStatusTlv(uint8_t slot, const uint8_t *tlv, size_t len) {
    if (slot >= MAX_MULTI_GOPRO_SLOTS || !tlv) return;
    Slot &s = *_slots[slot];
    CameraData &d = s.telemetry;
    d.valid = true;

    size_t pos = 0;
    while (pos + 2 <= len) {
        const uint8_t id = tlv[pos++];
        const uint8_t vlen = tlv[pos++];
        if (pos + vlen > len) break;
        const uint8_t *v = tlv + pos;
        pos += vlen;

        switch (id) {
            case GP_STATUS_BATTERY_PCT:
                if (vlen >= 1) { d.percent = v[0]; d.has_battery = true; }
                break;
            case GP_STATUS_REMAINING_TIME:
                if (vlen >= 4) {
                    d.remain_time = ((uint32_t)v[0] << 24) | ((uint32_t)v[1] << 16) |
                                    ((uint32_t)v[2] << 8) | (uint32_t)v[3];
                    d.has_remain_time = true;
                }
                break;
            case GP_STATUS_OVERHEATING:
                if (vlen >= 1) { d.temp_over = v[0] ? 1 : 0; d.has_temperature = true; }
                break;
            case GP_STATUS_ENCODING:
            case GP_STATUS_LEGACY_RECORDING:
                if (vlen >= 1) {
                    s.recording = (v[0] != 0);
                    s.recordingKnown = true;
                    d.recording = s.recording;
                    d.has_recording = true;
                }
                break;
            case GP_STATUS_ENC_DURATION:
                if (vlen >= 4) {
                    const uint32_t secs = ((uint32_t)v[0] << 24) | ((uint32_t)v[1] << 16) |
                                          ((uint32_t)v[2] << 8) | (uint32_t)v[3];
                    d.record_time = secs > 65535UL ? 65535U : (uint16_t)secs;
                }
                break;
            default:
                break;
        }
    }
    publishState();
    logSlotTelemetry(slot);
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
    Slot &s = *_slots[slot];
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
            const bool requestedOn = s.pendingShutterOn;
            DBG_SERIAL.printf("[MULTI] Slot %u shutter %s (%s)\n", slot + 1,
                              status == 0 ? "OK" : "REJECTED", requestedOn ? "START" : "STOP");
            if (s.pendingShutter && status == 0) {
                s.recording = requestedOn;
                s.recordingKnown = true;
                s.telemetry.recording = requestedOn;
                s.telemetry.has_recording = true;
            }
            s.pendingShutter = false;
            publishState();
            logSlotTelemetry(slot);
        }
    }
    s.cmdRx.reset();
}

void MultiGoProCamera::handleSettingNotify(uint8_t slot, uint8_t *data, size_t len) {
    Slot &s = *_slots[slot];
    if (s.settingRx.feed(data, len)) s.settingRx.reset();
}

void MultiGoProCamera::handleQueryNotify(uint8_t slot, uint8_t *data, size_t len) {
    Slot &s = *_slots[slot];
    if (!s.queryRx.feed(data, len)) return;
    if (s.queryRx.expected >= 2) {
        const uint8_t query = s.queryRx.buf[0], status = s.queryRx.buf[1];
        if (query == GP_QUERY_REGISTER_SETTING) {
            DBG_SERIAL.printf("[MULTI] Slot %u setting registration status=0x%02X\n", slot + 1, status);
            s.pendingRegisterStatus = true;
        } else if (query == GP_QUERY_REGISTER_STATUS) {
            DBG_SERIAL.printf("[MULTI] Slot %u status registration status=0x%02X -> READY\n", slot + 1, status);
            s.ready = true;
            s.lastKeepAliveMs = millis();
            s.lastStatusPollMs = 0;
            if (_registry) {
                _registry->onConnected(s.name.c_str(), s.addr.c_str(), (uint8_t)s.addrType, 1);
                stampSlotIdentity(slot);
            }
            publishState();
            logSlotTelemetry(slot);
            syncSlotToDesiredState(slot);
        }

        if (s.queryRx.expected > 2 &&
            (query == GP_QUERY_REGISTER_STATUS || query == GP_QUERY_GET_STATUS ||
             query == GP_NOTIFY_STATUS_UPDATE || query == GP_NOTIFY_SETTING_UPDATE)) {
            parseSlotStatusTlv(slot, s.queryRx.buf + 2, s.queryRx.expected - 2);
        }
    }
    s.queryRx.reset();
}

bool MultiGoProCamera::sendShutter(uint8_t slot, bool on) {
    Slot &s = *_slots[slot]; if (!s.ready || !s.cmdWrite) return false;
    const uint8_t buf[] = {0x03, GP_CMD_SET_SHUTTER, 0x01, (uint8_t)(on ? 0x01 : 0x00)};
    s.pendingShutter = true;
    s.pendingShutterOn = on;
    s.cmdWrite->writeValue(const_cast<uint8_t *>(buf), sizeof(buf), false);
    DBG_SERIAL.printf("[MULTI] Slot %u shutter -> %s\n", slot + 1, on ? "START" : "STOP");
    return true;
}

void MultiGoProCamera::syncSlotToDesiredState(uint8_t slot) {
    Slot &s = *_slots[slot]; if (!s.ready) return;
    if (_recordingRequested) {
        sendShutter(slot, true); s.needsStopOnReconnect = false;
    } else if (s.needsStopOnReconnect) {
        sendShutter(slot, false); s.needsStopOnReconnect = false;
    }
}

bool MultiGoProCamera::setDateTime(uint16_t year, uint8_t month, uint8_t day,
                                     uint8_t hour, uint8_t minute, uint8_t second) {
    bool any = false;
    for (uint8_t i = 0; i < MAX_MULTI_GOPRO_SLOTS; ++i) {
        Slot *s = _slots[i];
        if (!s || !s->ready || !s->cmdWrite) continue;
        const uint8_t buf[] = {
            0x09, GP_CMD_SET_DATE_TIME, 0x07,
            static_cast<uint8_t>((year >> 8) & 0xFF),
            static_cast<uint8_t>(year & 0xFF),
            month, day, hour, minute, second
        };
        s->cmdWrite->writeValue(const_cast<uint8_t *>(buf), sizeof(buf), false);
        DBG_SERIAL.printf("[GPS-TIME] Multi GoPro slot %u time sent: %04u-%02u-%02u %02u:%02u:%02u\n",
                          i + 1, year, month, day, hour, minute, second);
        any = true;
    }
    return any;
}

bool MultiGoProCamera::startRecording() {
    _recordingRequested = true; _recordStartedMs = millis(); bool any = false;
    for (uint8_t i = 0; i < MAX_MULTI_GOPRO_SLOTS; ++i) {
        Slot *s = _slots[i];
        if (!s) continue;
        s->needsStopOnReconnect = false;
        any = sendShutter(i, true) || any;
    }
    publishState(); return any;
}

bool MultiGoProCamera::stopRecording() {
    _recordingRequested = false; bool any = false;
    for (uint8_t i = 0; i < MAX_MULTI_GOPRO_SLOTS; ++i) {
        Slot *s = _slots[i];
        if (!s) continue;
        if (s->ready) { any = sendShutter(i, false) || any; s->needsStopOnReconnect = false; }
        else s->needsStopOnReconnect = true;
    }
    publishState(); return any;
}

bool MultiGoProCamera::switchCameraMode(uint8_t mode) {
    uint8_t group = GP_PRESET_VIDEO;
    if (mode == DJI_MODE_PHOTO) group = GP_PRESET_PHOTO;
    else if (mode == DJI_MODE_TIMELAPSE || mode == DJI_MODE_HYPERLAPSE) group = GP_PRESET_TIMELAPSE;
    bool any = false;
    for (uint8_t i = 0; i < MAX_MULTI_GOPRO_SLOTS; ++i) {
        Slot *sp = _slots[i];
        if (!sp) continue;
        Slot &s = *sp; if (!s.ready || !s.cmdWrite) continue;
        const uint8_t buf[] = {0x03, GP_CMD_LOAD_PRESET_GROUP, 0x01, group};
        s.cmdWrite->writeValue(const_cast<uint8_t *>(buf), sizeof(buf), false); any = true;
    }
    return any;
}

void MultiGoProCamera::publishState() {
    const uint8_t count = connectedCount();
    const uint8_t recCount = recordingCount();
    uint8_t knownCount = 0;
    for (const Slot *s : _slots) if (s && s->ready && s->recordingKnown) ++knownCount;

    CameraData out{};
    out.valid = count > 0;
    out.has_recording = true;
    out.connected_cameras = count;
    out.has_recording_count = count > 0 && knownCount == count;
    out.recording_cameras = recCount;
    if (out.has_recording_count)
        out.recording = recCount > 0;
    else
        out.recording = _recordingRequested && count > 0;
    out.record_time = out.recording ? (uint16_t)((millis() - _recordStartedMs) / 1000UL) : 0;

    bool haveBatt = false, haveRemain = false, haveTemp = false;
    for (uint8_t i = 0; i < MAX_MULTI_GOPRO_SLOTS; ++i) {
        Slot *sp = _slots[i];
        if (!sp) continue;
        Slot &s = *sp;
        if (!s.ready) continue;
        stampSlotIdentity(i);
        const CameraData &d = s.telemetry;

        if (d.has_battery && (!haveBatt || d.percent < out.percent)) {
            haveBatt = true;
            out.has_battery = true;
            out.percent = d.percent;
            out.battery_source_camera = s.cameraNumber;
            strlcpy(out.battery_source_label, s.cameraLabel, sizeof(out.battery_source_label));
        }
        if (d.has_remain_time && (!haveRemain || d.remain_time < out.remain_time)) {
            haveRemain = true;
            out.has_remain_time = true;
            out.remain_time = d.remain_time;
            out.remain_source_camera = s.cameraNumber;
            strlcpy(out.remain_source_label, s.cameraLabel, sizeof(out.remain_source_label));
        }
        if (d.has_temperature && (!haveTemp || d.temp_over > out.temp_over)) {
            haveTemp = true;
            out.has_temperature = true;
            out.temp_over = d.temp_over;
            out.temp_source_camera = s.cameraNumber;
            strlcpy(out.temp_source_label, s.cameraLabel, sizeof(out.temp_source_label));
        }
    }

    _camera = out;
    if (_cameraCb) _cameraCb(_camera);
}

void MultiGoProCamera::update() {
    for (uint8_t i = 0; i < MAX_MULTI_GOPRO_SLOTS; ++i) {
        Slot *sp = _slots[i];
        if (!sp) continue;
        Slot &s = *sp;
        if (s.pendingHwInfo && s.bleConnected) { s.pendingHwInfo = false; sendHardwareInfo(i); return; }
        if (s.pendingRegisterSettings && s.bleConnected) { s.pendingRegisterSettings = false; sendRegisterSettings(i); return; }
        if (s.pendingRegisterStatus && s.bleConnected) { s.pendingRegisterStatus = false; sendRegisterStatus(i); return; }
    }

    const uint32_t now = millis();
    for (uint8_t i = 0; i < MAX_MULTI_GOPRO_SLOTS; ++i) {
        Slot *sp = _slots[i];
        if (!sp) continue;
        Slot &s = *sp;
        if (s.ready && s.bleConnected && now - s.lastKeepAliveMs >= 10000UL) {
            s.lastKeepAliveMs = now;
            sendKeepAlive(i);
            return;
        }
    }

    // Poll each READY camera independently. Some GoPros do not push every
    // registered status value, which previously left only one camera with
    // useful telemetry in the Cameras page / warning-source logic.
    for (uint8_t i = 0; i < MAX_MULTI_GOPRO_SLOTS; ++i) {
        Slot *sp = _slots[i];
        if (!sp) continue;
        Slot &s = *sp;
        if (s.ready && s.bleConnected && (s.lastStatusPollMs == 0 || now - s.lastStatusPollMs >= 2000UL)) {
            s.lastStatusPollMs = now;
            sendStatusPoll(i);
            return;
        }
    }

    for (uint8_t i = 0; i < MAX_MULTI_GOPRO_SLOTS; ++i) {
        Slot *sp = _slots[i];
        if (!sp) continue;
        Slot &s = *sp;
        if (s.found && !s.bleConnected && !s.ready) { connectSlot(i); return; }
    }
    if (_scanning) return;
    for (uint8_t i = 0; i < MAX_MULTI_GOPRO_SLOTS; ++i) {
        Slot *sp = _slots[i];
        if (!sp) continue;
        Slot &s = *sp;
        if (!s.ready && !s.bleConnected && !s.found && now - s.lastAttemptMs >= 750UL) { startScan(i); return; }
    }
    if (_camera.recording) {
        _camera.record_time = (uint16_t)((now - _recordStartedMs) / 1000UL);
        if (_cameraCb) _cameraCb(_camera);
    }
}