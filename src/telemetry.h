#pragma once
#include <stdint.h>

// ─── Normalized camera settings codes ────────────────────────────────────────
// Both DJI and GoPro drivers map their native indices to these values so that
// msp_serial can use a single display table regardless of camera type.
// 0xFF means the camera has not reported that field.

#define CAM_RES_UNKNOWN  0xFF
#define CAM_RES_480P     0
#define CAM_RES_720P     1
#define CAM_RES_1080P    2
#define CAM_RES_1440P    3
#define CAM_RES_2_7K     4
#define CAM_RES_4K       5
#define CAM_RES_4K_WIDE  6
#define CAM_RES_5_1K     7
#define CAM_RES_5_3K     8
#define CAM_RES_8K       9

#define CAM_FPS_UNKNOWN  0xFF
#define CAM_FPS_24       0
#define CAM_FPS_25       1
#define CAM_FPS_30       2
#define CAM_FPS_48       3
#define CAM_FPS_50       4
#define CAM_FPS_60       5
#define CAM_FPS_90       6
#define CAM_FPS_100      7
#define CAM_FPS_120      8
#define CAM_FPS_200      9
#define CAM_FPS_240      10
#define CAM_FPS_400      11

#define CAM_EIS_UNKNOWN  0xFF
#define CAM_EIS_OFF      0
#define CAM_EIS_RS       1
#define CAM_EIS_HS       2
#define CAM_EIS_RS_PLUS  3
#define CAM_EIS_HB       4
#define CAM_EIS_LOW      5
#define CAM_EIS_HIGH     6
#define CAM_EIS_BOOST    7
#define CAM_EIS_AUTO     8
#define CAM_EIS_STD      9

// V1.0.2 camera labels are seven visible characters max. Keeping the source
// label directly in normalized telemetry lets the OSD attribute an aggregate
// warning to the physical camera which caused it.
#define CAM_WARNING_LABEL_LEN 8
#define CAM_OSD_SOURCE_MAX 8

// Per-camera telemetry carried alongside the normal aggregate CameraData.
// This is intentionally compact and only exists so the experimental OSD layer
// can target C1/C2/etc without changing the normal camera-control behaviour.
struct CameraSourceData {
    bool     valid = false;
    uint8_t  camera_number = 0;
    char     camera_label[CAM_WARNING_LABEL_LEN] = {};

    bool     has_battery = false;
    uint8_t  percent = 0;
    bool     has_recording = false;
    bool     recording = false;
    bool     has_temperature = false;
    uint8_t  temp_over = 0;
    bool     has_remain_time = false;
    uint32_t remain_time = 0;
    uint32_t remain_cap_mb = 0;
    uint16_t record_time = 0;
    uint8_t  camera_mode = 0;
    uint8_t  eis_mode = CAM_EIS_UNKNOWN;
    uint8_t  resolution = CAM_RES_UNKNOWN;
    uint8_t  fps_idx = CAM_FPS_UNKNOWN;
};

struct CameraData {
    bool     valid        = false;
    bool     has_battery  = false;
    bool     has_recording = false;
    bool     has_temperature = false;
    bool     has_remain_time = false;
    bool     has_media_ready = false;
    bool     media_ready     = true;
    bool     has_recording_count = false; // recording_cameras contains confirmed aggregate data
    uint8_t  connected_cameras = 0;
    uint8_t  recording_cameras = 0;
    uint8_t  percent      = 0;
    bool     recording    = false;
    uint8_t  camera_mode  = 0;
    uint8_t  eis_mode     = 0;
    uint8_t  temp_over    = 0;
    uint8_t  resolution   = CAM_RES_UNKNOWN;
    uint8_t  fps_idx      = CAM_FPS_UNKNOWN;
    uint16_t record_time  = 0;
    uint32_t remain_cap_mb = 0;
    uint32_t remain_time  = 0;

    // Source identity for aggregate warning-bearing values. camera==0 means
    // unknown/unattributed and preserves the legacy single-camera behaviour.
    uint8_t battery_source_camera = 0;
    char    battery_source_label[CAM_WARNING_LABEL_LEN] = {};
    uint8_t remain_source_camera = 0;
    char    remain_source_label[CAM_WARNING_LABEL_LEN] = {};
    uint8_t temp_source_camera = 0;
    char    temp_source_label[CAM_WARNING_LABEL_LEN] = {};

    // Experimental Multi Cam OSD source table. Standard single-camera paths
    // leave this empty and continue using the aggregate fields above.
    uint8_t source_count = 0;
    CameraSourceData sources[CAM_OSD_SOURCE_MAX]{};
};