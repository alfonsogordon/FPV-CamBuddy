#include "multi_camera_coordinator.h"
#include "config.h"

MultiCameraCoordinator *MultiCameraCoordinator::_instance = nullptr;

MultiCameraCoordinator::MultiCameraCoordinator(MultiGoProCamera &gopro,
                                               BLECamera &dji,
                                               SonyCamera &sony,
                                               BlackmagicCamera &blackmagic,
                                               Insta360Camera &insta360,
                                               CaddxCamera &caddx)
    : _gopro(gopro), _dji(dji), _sony(sony), _blackmagic(blackmagic),
      _insta360(insta360), _caddx(caddx) {
    _all[GOPRO] = &_gopro;
    _all[DJI] = &_dji;
    _all[SONY] = &_sony;
    _all[BLACKMAGIC] = &_blackmagic;
    _all[INSTA360] = &_insta360;
    _all[CADDX] = &_caddx;
    _ble[0] = &_gopro;
    _ble[1] = &_dji;
    _ble[2] = &_sony;
    _ble[3] = &_blackmagic;
    _ble[4] = &_insta360;
}

void MultiCameraCoordinator::begin() {
    _instance = this;

    _gopro.setCameraCallback(cbGoPro);
    _dji.setCameraCallback(cbDji);
    _sony.setCameraCallback(cbSony);
    _blackmagic.setCameraCallback(cbBlackmagic);
    _insta360.setCameraCallback(cbInsta360);
    _caddx.setCameraCallback(cbCaddx);

    for (uint8_t i = 0; i < BLE_FAMILY_COUNT; ++i) _ble[i]->setScanEnabled(false);
    _scanOwner = 0;
    _ble[_scanOwner]->setScanEnabled(true);
    _scanOwnerSince = millis();

    _gopro.begin();
    _dji.begin();
    _sony.begin();
    _blackmagic.begin();
    _insta360.begin();
    _caddx.begin();

    DBG_SERIAL.println("[MULTI] Multi-brand coordinator active: GoPro/DJI/Sony/Blackmagic/Insta360/Caddx");
    publishState();
}

uint8_t MultiCameraCoordinator::familyConnectedCount(uint8_t family) const {
    if (family == GOPRO) return _gopro.connectedCount();
    return _all[family]->isConnected() ? 1 : 0;
}

uint8_t MultiCameraCoordinator::connectedCount() const {
    uint16_t n = 0;
    for (uint8_t i = 0; i < FAMILY_COUNT; ++i) n += familyConnectedCount(i);
    return n > 255 ? 255 : static_cast<uint8_t>(n);
}

bool MultiCameraCoordinator::isConnected() const {
    return connectedCount() > 0;
}

bool MultiCameraCoordinator::familyWantsScan(uint8_t family) const {
    if (family >= BLE_FAMILY_COUNT) return false;
    if (family == GOPRO) return _gopro.connectedCount() < MAX_MULTI_GOPRO_SLOTS;
    return !_all[family]->isConnected();
}

void MultiCameraCoordinator::rotateScanOwner(bool force) {
    const uint32_t now = millis();
    if (!force && now - _scanOwnerSince < SCAN_OWNER_WINDOW_MS) return;

    for (uint8_t i = 0; i < BLE_FAMILY_COUNT; ++i) _ble[i]->setScanEnabled(false);

    for (uint8_t step = 1; step <= BLE_FAMILY_COUNT; ++step) {
        const uint8_t candidate = (_scanOwner + step) % BLE_FAMILY_COUNT;
        if (familyWantsScan(candidate)) {
            _scanOwner = candidate;
            _ble[_scanOwner]->setScanEnabled(true);
            _scanOwnerSince = now;
            DBG_SERIAL.printf("[MULTI] BLE scan owner -> family %u\n", _scanOwner);
            return;
        }
    }

    _scanOwner = GOPRO;
    _ble[_scanOwner]->setScanEnabled(true);
    _scanOwnerSince = now;
}

void MultiCameraCoordinator::syncNewConnection(uint8_t family) {
    if (_recordingRequested) {
        _all[family]->startRecording();
    } else if (_stopRequested) {
        _all[family]->stopRecording();
    }
}

void MultiCameraCoordinator::update() {
    for (uint8_t i = 0; i < FAMILY_COUNT; ++i) _all[i]->update();

    bool connectionChanged = false;
    for (uint8_t i = 0; i < FAMILY_COUNT; ++i) {
        const bool nowConnected = _all[i]->isConnected();
        if (nowConnected && !_wasConnected[i]) {
            DBG_SERIAL.printf("[MULTI] family %u joined; reconciling requested record state\n", i);
            syncNewConnection(i);
            connectionChanged = true;
        } else if (!nowConnected && _wasConnected[i]) {
            DBG_SERIAL.printf("[MULTI] family %u left; republishing aggregate state\n", i);
            connectionChanged = true;
        }
        _wasConnected[i] = nowConnected;
    }
    if (connectionChanged) publishState();

    if (_scanOwner != GOPRO && _all[_scanOwner]->isConnected() &&
        millis() - _scanOwnerSince > 500) {
        rotateScanOwner(true);
    } else {
        rotateScanOwner(false);
    }

    if (_camera.recording) {
        _camera.record_time = static_cast<uint16_t>((millis() - _recordStartedMs) / 1000UL);
        if (_cameraCb) _cameraCb(_camera);
    }
}

bool MultiCameraCoordinator::startRecording() {
    _recordingRequested = true;
    _stopRequested = false;
    _recordStartedMs = millis();
    bool any = false;
    for (uint8_t i = 0; i < FAMILY_COUNT; ++i) any = _all[i]->startRecording() || any;
    publishState();
    return any;
}

bool MultiCameraCoordinator::stopRecording() {
    _recordingRequested = false;
    _stopRequested = true;
    bool any = false;
    for (uint8_t i = 0; i < FAMILY_COUNT; ++i) any = _all[i]->stopRecording() || any;
    publishState();
    return any;
}

bool MultiCameraCoordinator::switchCameraMode(uint8_t mode) {
    bool any = false;
    for (uint8_t i = 0; i < FAMILY_COUNT; ++i) any = _all[i]->switchCameraMode(mode) || any;
    return any;
}

void MultiCameraCoordinator::onSubData(uint8_t family, const CameraData &d) {
    if (family >= FAMILY_COUNT) return;
    _latest[family] = d;
    _seenData[family] = true;
    publishState();
}

void MultiCameraCoordinator::publishState() {
    CameraData out{};
    const uint8_t count = connectedCount();
    out.valid = count > 0;
    out.connected_cameras = count;
    out.has_recording = true;

    bool haveBatt = false, haveRemain = false, haveTemp = false, haveMedia = false;
    uint8_t minBatt = 100, hottest = 0;
    uint32_t minRemain = 0;
    uint16_t recordingCount = 0;
    uint16_t recordingKnownCount = 0;

    for (uint8_t i = 0; i < FAMILY_COUNT; ++i) {
        if (!_seenData[i] || !_all[i]->isConnected()) continue;
        const CameraData &d = _latest[i];
        const uint8_t familyCount = familyConnectedCount(i);

        if (i == GOPRO) {
            if (d.has_recording_count) {
                recordingKnownCount += familyCount;
                recordingCount += d.recording_cameras > familyCount ? familyCount : d.recording_cameras;
            }
        } else if (d.has_recording) {
            recordingKnownCount += familyCount;
            if (d.recording) recordingCount += familyCount;
        }

        if (d.has_battery) {
            if (!haveBatt || d.percent < minBatt) minBatt = d.percent;
            haveBatt = true;
        }
        if (d.has_remain_time) {
            if (!haveRemain || d.remain_time < minRemain) minRemain = d.remain_time;
            haveRemain = true;
        }
        if (d.has_temperature) {
            if (!haveTemp || d.temp_over > hottest) hottest = d.temp_over;
            haveTemp = true;
        }
        if (d.has_media_ready) {
            out.media_ready = haveMedia ? (out.media_ready && d.media_ready) : d.media_ready;
            haveMedia = true;
        }
        if (out.camera_mode == 0 && d.camera_mode != 0) out.camera_mode = d.camera_mode;
        if (out.resolution == CAM_RES_UNKNOWN && d.resolution != CAM_RES_UNKNOWN) out.resolution = d.resolution;
        if (out.fps_idx == CAM_FPS_UNKNOWN && d.fps_idx != CAM_FPS_UNKNOWN) out.fps_idx = d.fps_idx;
        if (out.eis_mode == CAM_EIS_UNKNOWN && d.eis_mode != CAM_EIS_UNKNOWN) out.eis_mode = d.eis_mode;
    }

    out.has_recording_count = count > 0 && recordingKnownCount >= count;
    out.recording_cameras = recordingCount > 255 ? 255 : static_cast<uint8_t>(recordingCount);
    if (out.has_recording_count)
        out.recording = out.recording_cameras > 0;
    else
        out.recording = _recordingRequested && count > 0;
    out.record_time = out.recording
        ? static_cast<uint16_t>((millis() - _recordStartedMs) / 1000UL) : 0;

    out.has_battery = haveBatt;
    if (haveBatt) out.percent = minBatt;
    out.has_remain_time = haveRemain;
    if (haveRemain) out.remain_time = minRemain;
    out.has_temperature = haveTemp;
    if (haveTemp) out.temp_over = hottest;
    out.has_media_ready = haveMedia;

    _camera = out;
    if (_cameraCb) _cameraCb(_camera);
}

void MultiCameraCoordinator::cbGoPro(const CameraData &d)      { if (_instance) _instance->onSubData(GOPRO, d); }
void MultiCameraCoordinator::cbDji(const CameraData &d)        { if (_instance) _instance->onSubData(DJI, d); }
void MultiCameraCoordinator::cbSony(const CameraData &d)       { if (_instance) _instance->onSubData(SONY, d); }
void MultiCameraCoordinator::cbBlackmagic(const CameraData &d) { if (_instance) _instance->onSubData(BLACKMAGIC, d); }
void MultiCameraCoordinator::cbInsta360(const CameraData &d)   { if (_instance) _instance->onSubData(INSTA360, d); }
void MultiCameraCoordinator::cbCaddx(const CameraData &d)      { if (_instance) _instance->onSubData(CADDX, d); }
