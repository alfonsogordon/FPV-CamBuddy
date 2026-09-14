#pragma once
#include <Arduino.h>
#include "camera.h"
#include "ble_camera.h"
#include "multi_gopro_camera.h"
#include "sony_camera.h"
#include "blackmagic_camera.h"
#include "insta360_camera.h"
#include "caddx_camera.h"

class MultiCameraCoordinator : public Camera {
public:
    MultiCameraCoordinator(MultiGoProCamera &gopro,
                           BLECamera &dji,
                           SonyCamera &sony,
                           BlackmagicCamera &blackmagic,
                           Insta360Camera &insta360,
                           CaddxCamera &caddx);

    void begin() override;
    void update() override;
    bool isConnected() const override;
    bool startRecording() override;
    bool stopRecording() override;
    bool switchCameraMode(uint8_t mode) override;

    uint8_t connectedCount() const;

private:
    enum Family : uint8_t {
        GOPRO = 0, DJI = 1, SONY = 2, BLACKMAGIC = 3,
        INSTA360 = 4, CADDX = 5, FAMILY_COUNT = 6
    };
    static constexpr uint8_t BLE_FAMILY_COUNT = 5;
    // After the initial startup burst, rotate BLE ownership fairly quickly so
    // mixed-brand cameras are still discovered without long dead periods.
    static constexpr uint32_t SCAN_OWNER_WINDOW_MS = 4000;
    // Multi-GoPro is the primary V1.0.2 hardware-test path. Give GoPro one
    // uninterrupted startup window so a second camera can be found/handshaken
    // before short Auto Power Down timers (notably HERO11) put it back to sleep.
    static constexpr uint32_t INITIAL_GOPRO_PRIORITY_MS = 12000;

    MultiGoProCamera &_gopro;
    BLECamera &_dji;
    SonyCamera &_sony;
    BlackmagicCamera &_blackmagic;
    Insta360Camera &_insta360;
    CaddxCamera &_caddx;
    Camera *_all[FAMILY_COUNT];
    Camera *_ble[BLE_FAMILY_COUNT];

    CameraData _latest[FAMILY_COUNT]{};
    bool _seenData[FAMILY_COUNT]{};
    bool _wasConnected[FAMILY_COUNT]{};
    bool _recordingRequested = false;
    bool _stopRequested = false;
    uint32_t _recordStartedMs = 0;
    uint8_t _scanOwner = 0;
    uint32_t _scanOwnerSince = 0;
    uint32_t _startedMs = 0;
    CameraData _camera{};

    static MultiCameraCoordinator *_instance;
    static void cbGoPro(const CameraData &d);
    static void cbDji(const CameraData &d);
    static void cbSony(const CameraData &d);
    static void cbBlackmagic(const CameraData &d);
    static void cbInsta360(const CameraData &d);
    static void cbCaddx(const CameraData &d);

    void onSubData(uint8_t family, const CameraData &d);
    void publishState();
    void rotateScanOwner(bool force = false);
    bool familyWantsScan(uint8_t family) const;
    uint8_t familyConnectedCount(uint8_t family) const;
    void syncNewConnection(uint8_t family);
};
