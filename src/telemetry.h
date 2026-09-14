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

struct CameraData {
    bool     valid        = false;
    bool     has_battery  = false;
    bool     has_recording = false;
    bool     has_temperature = false;
    bool     has_remain_time = false;
    bool     has_media_ready = false;
    bool     media_ready     = true;
    uint8_t  connected_cameras = 0;    // Multi Cam live connected count
    uint8_t  recording_cameras = 0;    // confirmed/acknowledged recording cameras when supplied
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
};
