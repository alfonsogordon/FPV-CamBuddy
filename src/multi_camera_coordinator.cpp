#include "multi_camera_coordinator.h"
#include "camera_registry.h"
#include "config.h"
#include <cstring>

MultiCameraCoordinator *MultiCameraCoordinator::_instance = nullptr;

namespace {
uint8_t registryTypeForFamily(uint8_t family) {
    switch (family) {
        case 0: return 1; // GoPro
        case 1: return 0; // DJI
        case 2: return 3; // Sony
        case 3: return 4; // Blackmagic
        case 4: return 5; // Insta360
        case 5: return 2; // Caddx
        default: return 0;
    }
}
}

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
}

void MultiCameraCoordinator::begin() {
    _instance = this;
    _registry = _gopro.registry();

    _gopro.setCameraCallback(cbGoPro);
    _dji.setCameraCallback(cbDji);
    _sony.setCameraCallback(cbSony);
    _blackmagic.setCameraCallback(cbBlackmagic);
    _insta360.setCameraCallback(cbInsta360);
    _caddx.setCameraCallback(cbCaddx);

    // In Multi Cam mode no individual BLE backend owns the scanner. One shared
    // scan sees every advertisement and dispatches it through all family
    // matchers. Single-camera mode never instantiates this path and therefore
    // keeps the original stop-scanning-on-connect behaviour unchanged.
    _gopro.setScanEnabled(false);
    _dji.setScanEnabled(false);
    _sony.setScanEnabled(false);
    _blackmagic.setScanEnabled(false);
    _insta360.setScanEnabled(false);

    _gopro.begin();
    _dji.begin();
    _sony.begin();
    _blackmagic.begin();
    _insta360.begin();
    _caddx.begin();

    DBG_SERIAL.println("[MULTI] Shared BLE discovery active: GoPro/DJI/Sony/Blackmagic/Insta360");
    DBG_SERIAL.println("[MULTI] BLE discovery remains active while Multi Cam is enabled");
    startSharedScan();
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

bool MultiCameraCoordinator::anyBleFamilyWantsScan() const {
    for (uint8_t i = 0; i < BLE_FAMILY_COUNT; ++i)
        if (familyWantsScan(i)) return true;
    return false;
}

void MultiCameraCoordinator::startSharedScan() {
    if (_sharedScanning || !anyBleFamilyWantsScan()) return;

    // Pick the next free GoPro slot before advertisements start arriving.
    // Other camera families each have one connection slot in V1.0.2.
    if (familyWantsScan(GOPRO)) _gopro.prepareSharedScanSlot();

    BLEScan *scan = BLEDevice::getScan();
    scan->setAdvertisedDeviceCallbacks(this, true);
    scan->clearResults();
    scan->setActiveScan(true);
    scan->setInterval(0x50);
    scan->setWindow(0x30);
    _sharedScanning = true;
    DBG_SERIAL.println("[MULTI] Shared BLE scan -> all supported BLE camera families");
    scan->start(BLE_SCAN_DURATION_SECS, sharedScanDoneCallback, false);
}

void MultiCameraCoordinator::sharedScanDoneCallback(BLEScanResults) {
    if (!_instance) return;
    _instance->_sharedScanning = false;
    _instance->_sharedScanStoppedMs = millis();
}

void MultiCameraCoordinator::onResult(BLEAdvertisedDevice device) {
    bool accepted = false;
    uint8_t family = 0xFF;

    if (familyWantsScan(GOPRO) && _gopro.acceptSharedAdvertisement(device)) {
        accepted = true; family = GOPRO;
    } else if (familyWantsScan(DJI) && _dji.acceptSharedAdvertisement(device)) {
        accepted = true; family = DJI;
    } else if (familyWantsScan(SONY) && _sony.acceptSharedAdvertisement(device)) {
        accepted = true; family = SONY;
    } else if (familyWantsScan(BLACKMAGIC) && _blackmagic.acceptSharedAdvertisement(device)) {
        accepted = true; family = BLACKMAGIC;
    } else if (familyWantsScan(INSTA360) && _insta360.acceptSharedAdvertisement(device)) {
        accepted = true; family = INSTA360;
    }

    if (!accepted) return;

    // Connecting while an active scan owns the controller is needlessly
    // fragile on the C3. Stop only long enough for the selected backend to do
    // its GATT setup; update() restarts the all-family scan ~120 ms later.
    BLEDevice::getScan()->stop();
    _sharedScanning = false;
    _sharedScanStoppedMs = millis();
    DBG_SERIAL.printf("[MULTI] Shared discovery matched family %u; pausing scan for connection\n", family);
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

    if (!_sharedScanning && anyBleFamilyWantsScan() &&
        millis() - _sharedScanStoppedMs >= SHARED_SCAN_RESTART_MS) {
        startSharedScan();
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

        if (d.has_battery && (!haveBatt || d.percent < minBatt)) {
            minBatt = d.percent;
            haveBatt = true;
            out.battery_source_camera = d.battery_source_camera;
            strlcpy(out.battery_source_label, d.battery_source_label, sizeof(out.battery_source_label));
            if (out.battery_source_camera == 0 && _registry)
                _registry->identityForType(registryTypeForFamily(i), out.battery_source_camera,
                                           out.battery_source_label, sizeof(out.battery_source_label));
        }
        if (d.has_remain_time && (!haveRemain || d.remain_time < minRemain)) {
            minRemain = d.remain_time;
            haveRemain = true;
            out.remain_source_camera = d.remain_source_camera;
            strlcpy(out.remain_source_label, d.remain_source_label, sizeof(out.remain_source_label));
            if (out.remain_source_camera == 0 && _registry)
                _registry->identityForType(registryTypeForFamily(i), out.remain_source_camera,
                                           out.remain_source_label, sizeof(out.remain_source_label));
        }
        if (d.has_temperature && (!haveTemp || d.temp_over > hottest)) {
            hottest = d.temp_over;
            haveTemp = true;
            out.temp_source_camera = d.temp_source_camera;
            strlcpy(out.temp_source_label, d.temp_source_label, sizeof(out.temp_source_label));
            if (out.temp_source_camera == 0 && _registry)
                _registry->identityForType(registryTypeForFamily(i), out.temp_source_camera,
                                           out.temp_source_label, sizeof(out.temp_source_label));
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
