/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "Cam3Setting"

#include "cmr_common.h"
#include <cutils/properties.h>
#include <fcntl.h>
#include <media/hardware/MetadataBufferType.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <utils/Log.h>
#include <utils/String16.h>
//#include <androidfw/SprdIlog.h>
#include "SprdCamera3Setting.h"

#include "include/SprdCamera3.h"
//#include "SprdCameraHardwareConfig2.h"
#include "inc/SprdCamera3Config.h"
#include <dlfcn.h>
#include <map>
using namespace android;

namespace sprdcamera {

uint8_t SprdCamera3Setting::mSensorFocusEnable[] = {0, 0, 0, 0};

/**********************Macro Define**********************/
#ifdef CONFIG_CAMERA_FACE_DETECT
#ifdef CONFIG_COVERED_SENSOR
#define CAMERA3MAXFACE 11
#else
#define CAMERA3MAXFACE 10
#endif
#else
#define CAMERA3MAXFACE 0
#endif

typedef struct {
    float aperture;
    float filter_density;
    uint8_t optical_stabilization;
    int32_t lens_shading_map_size[2];
    float sensor_physical_size[2];
    int64_t exposure_time_range[2];
    int64_t frame_duration_range[2];
    uint8_t color_arrangement;
    int32_t white_level;
    int32_t black_level;
    int64_t flash_charge_duration;
    int32_t max_tone_map_curve_points;
    int32_t histogram_size;
    int32_t max_histogram_count;
    int32_t sharpness_map_size[2];
    int32_t max_sharpness_map_value;
    int64_t raw_min_duration[1];
    int32_t supported_preview_formats[4];
    int64_t processed_min_durations[1];
    int32_t available_fps_ranges[FPS_RANGE_COUNT];
    camera_metadata_rational exposureCompensationStep;
    int32_t exposureCompensationRange[2];
    int32_t available_processed_sizes[16];
    int32_t jpegThumbnailSizes[CAMERA_SETTINGS_THUMBNAILSIZE_ARRAYSIZE];
    int64_t FrameDurationRange[2];
    uint8_t availableFaceDetectModes[SPRD_MAX_AVAILABLE_FACE_DETECT_MODES];
    uint8_t availableVideoStabModes[1];
    uint8_t availEffectModes[9];
    uint8_t availSceneModes[18];
    uint8_t availAntibandingModes[4];
    uint8_t availableAfModes[6];
    uint8_t availableAmModes[4];
    uint8_t avail_awb_modes[9];
    uint8_t availableAeModes[5];
    uint8_t availableSlowMotion[3];
    int32_t max_output_streams[3];
    uint8_t availableBrightNess[7];
    uint8_t availableIso[7];
    uint8_t availableAutoHdr;
    uint8_t availableAiScene;
    uint8_t availableAuto3Dnr;
    uint8_t availDistortionCorrectionModes[1];
    uint8_t availLogoWatermark;
    uint8_t availTimeWatermark;
} camera3_common_t;

typedef struct {
    int32_t sensorRawW;
    int32_t sensorRawH;
    float fnumber;
    const uint8_t *availableAfModes;
    const uint8_t *sceneModeOverrides;
    const uint8_t *availableAeModes;
    int numAvailableAfModes;
    int numSceneModeOverrides;
    int numAvailableAeModes;
} camera3_config_t;

typedef struct _camera3_default_info {
    camera3_common_t common;
    camera3_config_t config[2];
} SprdCamera3DefaultInfo;

typedef struct _front_flash_type {
    const char *type_id;
    const char *type_name;
} front_flash_type;

static SprdCamera3DefaultInfo camera3_default_info;

static cam_dimension_t largest_picture_size[CAMERA_ID_COUNT];
static cmr_u16 sensor_max_width[CAMERA_ID_COUNT] = {2592, 2592, 2592, 2592};
static cmr_u16 sensor_max_height[CAMERA_ID_COUNT] = {1944, 1944, 1944, 1944};

// if cant get valid sensor fov info, use the default value
static drv_fov_info sensor_fov[CAMERA_ID_COUNT] = {
    {{3.50f, 2.625f}, 3.75f},
    {{3.50f, 2.625f}, 3.75f},
    {{3.50f, 2.625f}, 3.75f},
    {{3.50f, 2.625f}, 3.75f},
};

static front_flash_type front_flash[] = {
    {"2", "lcd"}, {"1", "led"}, {"2", "flash"}, {"1", "none"},
};

static bool isBlobOrRaw16(int j, int32_t *scaler_formats);


#if 0
const sensor_fov_tab_t back_sensor_fov_tab[] = {
    {"ov8825_mipi_raw", {4.614f, 3.444f}, 4.222f},
    {"ov5648_mipi_raw", {3.50f, 2.625f}, 3.75f},
    {"ov2680_mipi_raw", {3.50f, 2.625f}, 3.75f},
    {"ov5670_mipi_raw", {2.9457f, 2.214f}, 2.481f},
    {"hi544_mipi_raw", {3.50f, 2.625f}, 3.75f},
    {"sr352_mipi_yuv", {3.50f, 2.625f}, 3.75f},
    {"imx219_mipi_raw", {3.50f, 2.625f}, 3.75f},
    {"s5k4ec_mipi_yuv", {3.50f, 2.625f}, 3.75f},
    //{"ov8830_mipi_raw", {3.50f, 2.625f}, 3.75f},
    {"ov5640_mipi_yuv", {3.50f, 2.625f}, 3.75f},
    {"imx179_mipi_raw", {3.50f, 2.625f}, 3.75f},
    {"ov8865_mipi_raw", {3.50f, 2.625f}, 3.75f},
    {"ov13850_mipi_raw", {4.815f, 3.6783f}, 3.95f},
    {"s5k3m2xxm3_mipi_raw", {4.731f, 3.512f}, 4.544f},
    {"imx214_mipi_raw", {4.731f, 3.512f}, 4.544f},
    {"imx230_mipi_raw", {5.985f, 4.498f}, 4.75f},
    {"imx258_mipi_raw", {4.712f, 3.494f}, 3.698f},
    {"", {3.50f, 2.625f}, 3.75f},
};

const sensor_fov_tab_t front_sensor_fov_tab[] = {
    {"hi255_yuv", {3.50f, 2.625f}, 3.75f},
    {"GC2155_MIPI_yuv", {2.828f, 2.156f}, 2.6865f},
    {"GC0310_MIPI_yuv", {1.44f, 1.08f}, 1.476f},
    {"sr030pc50_yuv", {3.50f, 2.625f}, 3.75f},
    {"ov5648_mipi_raw", {3.6736f, 2.7384f}, 2.481f},
    {"s5k4h8yx_mipi_raw", {3.656f, 2.742f}, 3.01f},
    {"ov5675_mipi_raw", {2.903f, 2.177f}, 3.043},
    {"", {3.50f, 2.625f}, 3.75f},
};

const sensor_fov_tab_t third_sensor_fov_tab[] = {
    {"imx132_mipi_raw", {3.629f, 2.722f}, 3.486f},
    {"ov2680_mipi_raw", {2.84f, 2.15f}, 2.15f},
    {"", {3.50f, 2.625f}, 3.75f},
};
const sensor_fov_tab_t fourth_sensor_fov_tab[] = {
    {"imx132_mipi_raw", {3.629f, 2.722f}, 3.486f},
    {"ov2680_mipi_raw", {2.84f, 2.15f}, 2.15f},
    {"", {3.50f, 2.625f}, 3.75f},
};
#endif

const int32_t klens_shading_map_size[2] = {1, 1};
const int64_t kexposure_time_range[2] = {1000L, 30000000000L}; // 1 us - 30 sec
const int64_t kframe_duration_range[2] = {33331760L,
                                          30000000000L}; // ~1/30 s - 30 sec
const int32_t ksharpness_map_size[2] = {64, 64};
const int32_t ksupported_preview_formats[4] = {
    HAL_PIXEL_FORMAT_RAW16, HAL_PIXEL_FORMAT_BLOB, HAL_PIXEL_FORMAT_YV12,
    HAL_PIXEL_FORMAT_YCrCb_420_SP};

const int32_t kavailable_fps_ranges_back[] = {5,  15, 15, 15, 5,  20, 5,  24,
                                              24, 24, 5,  30, 20, 30, 30, 30};
const int32_t kavailable_fps_ranges_front[] = {5, 15, 15, 15, 5,  24, 24, 24,
                                               5, 30, 15, 30, 20, 30, 30, 30};

const int32_t kexposureCompensationRange[2] = {-16, 16};
const camera_metadata_rational kae_compensation_step = {1, 8};
// const int32_t kavailable_processed_sizes[16] = {/*must order from bigger to
// smaller*/
//	2592, 1944,
//	2048, 1536,
//	1920, HEIGHT_2M,
//	1600, 1200,
//	1280, 960,
//	1280, 720,
//	720, 480,
//	640, 480,
//};

const int32_t kjpegThumbnailSizes[CAMERA_SETTINGS_THUMBNAILSIZE_ARRAYSIZE] = {
    0, 0, 256, 144, 320, 240, 432, 288};
const int64_t kFrameDurationRange[2] = {
    33331760L, 500000000L}; //{33331760L,30000000000L}; // ~1/30 s - 1/5 sec
camera_metadata_rational kexposureCompensationStep = {1, 1};
const uint8_t availableFaceDetectModes[] = {
    ANDROID_STATISTICS_FACE_DETECT_MODE_OFF,
/* face detection is not supported now, will restore this macro
   when face detection is supported */
#ifdef CONFIG_CAMERA_FACE_DETECT
    ANDROID_STATISTICS_FACE_DETECT_MODE_SIMPLE
#endif
};
const uint8_t availableVstabModes[] = {
    ANDROID_CONTROL_VIDEO_STABILIZATION_MODE_OFF,
    // ANDROID_CONTROL_VIDEO_STABILIZATION_MODE_ON
};
const uint8_t avail_effect_mode[] = {
    ANDROID_CONTROL_EFFECT_MODE_OFF,      ANDROID_CONTROL_EFFECT_MODE_MONO,
    ANDROID_CONTROL_EFFECT_MODE_NEGATIVE, ANDROID_CONTROL_EFFECT_MODE_SOLARIZE,
    ANDROID_CONTROL_EFFECT_MODE_SEPIA,    ANDROID_CONTROL_EFFECT_MODE_AQUA};
const uint8_t avail_scene_modes[] = {
// ANDROID_CONTROL_SCENE_MODE_DISABLED,
#ifdef CONFIG_CAMERA_FACE_DETECT
    ANDROID_CONTROL_SCENE_MODE_FACE_PRIORITY,
#endif
    ANDROID_CONTROL_SCENE_MODE_NIGHT,
    ANDROID_CONTROL_SCENE_MODE_ACTION,
    ANDROID_CONTROL_SCENE_MODE_PORTRAIT,
    ANDROID_CONTROL_SCENE_MODE_LANDSCAPE,
#ifdef CONFIG_CAMERA_HDR_CAPTURE
    ANDROID_CONTROL_SCENE_MODE_HDR,
#endif
};

const uint8_t avail_antibanding_modes[] = {
    ANDROID_CONTROL_AE_ANTIBANDING_MODE_OFF,
    ANDROID_CONTROL_AE_ANTIBANDING_MODE_50HZ,
    ANDROID_CONTROL_AE_ANTIBANDING_MODE_60HZ,
    ANDROID_CONTROL_AE_ANTIBANDING_MODE_AUTO};

const uint8_t availableAfModes[] = {
    ANDROID_CONTROL_AF_MODE_OFF, ANDROID_CONTROL_AF_MODE_AUTO,
    ANDROID_CONTROL_AF_MODE_MACRO, ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE,
    ANDROID_CONTROL_AF_MODE_CONTINUOUS_VIDEO};

const uint8_t availableAfModesOfFront[] = {ANDROID_CONTROL_AF_MODE_OFF};

const uint8_t avail_awb_modes[] = {ANDROID_CONTROL_AWB_MODE_OFF,
                                   ANDROID_CONTROL_AWB_MODE_AUTO,
                                   ANDROID_CONTROL_AWB_MODE_INCANDESCENT,
                                   ANDROID_CONTROL_AWB_MODE_FLUORESCENT,
                                   ANDROID_CONTROL_AWB_MODE_DAYLIGHT,
                                   ANDROID_CONTROL_AWB_MODE_CLOUDY_DAYLIGHT};
const uint8_t availableAeModes[] = {ANDROID_CONTROL_AE_MODE_OFF,
                                    ANDROID_CONTROL_AE_MODE_ON,
                                    ANDROID_CONTROL_AE_MODE_ON_AUTO_FLASH,
                                    ANDROID_CONTROL_AE_MODE_ON_ALWAYS_FLASH};
enum {
    CAMERA_AE_FRAME_AVG = 0,
    CAMERA_AE_CENTER_WEIGHTED,
    CAMERA_AE_SPOT_METERING,
    CAMERA_AE_MODE_MAX
};
const uint8_t availableAmModes[] = {
    CAMERA_AE_FRAME_AVG, CAMERA_AE_CENTER_WEIGHTED, CAMERA_AE_SPOT_METERING,
    CAMERA_AE_MODE_MAX};

const uint8_t availDistortionCorrectionModes[] = {
    ANDROID_DISTORTION_CORRECTION_MODE_OFF};

const int32_t max_output_streams[3] = {0, 2, 1};
const uint8_t availableBrightNess[] = {0, 1, 2, 3, 4, 5, 6};
const uint8_t availableContrast[] = {0, 1, 2, 3, 4, 5, 6};
const uint8_t availableSaturation[] = {0, 1, 2, 3, 4, 5, 6};
#ifdef CONFIG_SUPPROT_AUTO_HDR
const uint8_t availableAutoHDR = 1;
#else
const uint8_t availableAutoHDR = 0;
#endif

#ifdef CONFIG_SUPPROT_AUTO_3DNR
const uint8_t availableAuto3DNR = 1;
#else
const uint8_t availableAuto3DNR = 0;
#endif

const uint8_t availableSlowMotion[] = {0, 1, 4};

#ifdef CONFIG_SUPPROT_AI_SCENE
const uint8_t availableAiScene = 1;
#else
const uint8_t availableAiScene = 0;
#endif

/* LOGO watermark not  support */
const uint8_t availLogoWatermark = 0;
/* Time watermark not support */
const uint8_t availTimeWatermark = 0;

enum {
    CAMERA_ISO_AUTO = 0,
    CAMERA_ISO_1600,
    CAMERA_ISO_800,
    CAMERA_ISO_400,
    CAMERA_ISO_200,
    CAMERA_ISO_100,
    CAMERA_ISO_MAX
};
const uint8_t availableIso[] = {CAMERA_ISO_AUTO, CAMERA_ISO_100,
                                CAMERA_ISO_200,  CAMERA_ISO_400,
                                CAMERA_ISO_800,  CAMERA_ISO_1600};

typedef struct cam_stream_info {
    cam_dimension_t stream_sizes_tbl;
    int64_t stream_min_duration;
    int64_t stream_stall_duration;
} cam_stream_info_t;

const cam_dimension_t default_sensor_max_sizes[CAMERA_ID_COUNT] = {
#if defined(CONFIG_CAMERA_SUPPORT_21M)
    {5312, 3984},
#elif defined(CONFIG_CAMERA_SUPPORT_16M)
    {4608, 3456},
#elif defined(CONFIG_CAMERA_SUPPORT_13M)
    {4160, 3120},
#elif defined(CONFIG_CAMERA_SUPPORT_8M)
    {3264, 2448},
#elif defined(CONFIG_CAMERA_SUPPORT_5M)
    {2592, 1944},
#elif defined(CONFIG_CAMERA_SUPPORT_3M)
    {2048, 1536},
#elif defined(CONFIG_CAMERA_SUPPORT_2M_1080P)
    {1920, HEIGHT_2M},
#elif defined(CONFIG_CAMERA_SUPPORT_2M)
    {1600, 1200},
#else
    {1600, 1200},
#endif

#if defined(CONFIG_FRONT_CAMERA_SUPPORT_21M)
    {5312, 3984},
#elif defined(CONFIG_FRONT_CAMERA_SUPPORT_16M)
    {4608, 3456},
#elif defined(CONFIG_FRONT_CAMERA_SUPPORT_13M)
    {4160, 3120},
#elif defined(CONFIG_FRONT_CAMERA_SUPPORT_8M)
    {3264, 2448},
#elif defined(CONFIG_FRONT_CAMERA_SUPPORT_5M)
    {2592, 1944},
#elif defined(CONFIG_FRONT_CAMERA_SUPPORT_3M)
    {2048, 1536},
#elif defined(CONFIG_FRONT_CAMERA_SUPPORT_2M_1080P)
    {1920, HEIGHT_2M},
#elif defined(CONFIG_FRONT_CAMERA_SUPPORT_2M)
    {1600, 1200},
#else
    {1600, 1200},
#endif

#if defined(CONFIG_BACK_EXT_CAMERA_SUPPORT_21M)
    {5312, 3984},
#elif defined(CONFIG_BACK_EXT_CAMERA_SUPPORT_16M)
    {4608, 3456},
#elif defined(CONFIG_BACK_EXT_CAMERA_SUPPORT_13M)
    {4160, 3120},
#elif defined(CONFIG_BACK_EXT_CAMERA_SUPPORT_8M)
    {3264, 2448},
#elif defined(CONFIG_BACK_EXT_CAMERA_SUPPORT_5M)
    {2592, 1944},
#elif defined(CONFIG_BACK_EXT_CAMERA_SUPPORT_3M)
    {2048, 1536},
#elif defined(CONFIG_BACK_EXT_CAMERA_SUPPORT_2M_1080P)
    {1920, HEIGHT_2M},
#elif defined(CONFIG_BACK_EXT_CAMERA_SUPPORT_2M)
    {1600, 1200},
#else
    {1600, 1200},
#endif

#if defined(CONFIG_FRONT_CAMERA_SUPPORT_21M)
    {5312, 3984},
#elif defined(CONFIG_FRONT_CAMERA_SUPPORT_16M)
    {4608, 3456},
#elif defined(CONFIG_FRONT_CAMERA_SUPPORT_13M)
    {4160, 3120},
#elif defined(CONFIG_FRONT_CAMERA_SUPPORT_8M)
    {3264, 2448},
#elif defined(CONFIG_FRONT_CAMERA_SUPPORT_5M)
    {2592, 1944},
#elif defined(CONFIG_FRONT_CAMERA_SUPPORT_3M)
    {2048, 1536},
#elif defined(CONFIG_FRONT_CAMERA_SUPPORT_2M_1080P)
    {1920, HEIGHT_2M},
#elif defined(CONFIG_FRONT_CAMERA_SUPPORT_2M)
    {1600, 1200},
#else
    {1600, 1200},
#endif
};

const cam_stream_info_t stream_info[] = {
    {{5312, 3984}, 41666666L, 41666666L},
    {{4608, 3456}, 33331760L, 33331760L},
    {{4160, 3120}, 33331760L, 33331760L},
    //{{3840, 2160}, 33331760L, 33331760L},
    {{3264, 2448}, 33331760L, 33331760L},
    {{3264, 1836}, 33331760L, 33331760L},
    {{2592, 1944}, 33331760L, 33331760L},
    {{2560, 1920}, 33331760L, 33331760L},
    //{{2560, 1440}, 33331760L, 33331760L},
    {{2048, 1536}, 33331760L, 33331760L},
    //{{1920, 1920}, 33331760L, 33331760L},
    {{1920, HEIGHT_2M}, 33331760L, 33331760L},
    {{1600, 1200}, 33331760L, 33331760L},
    //{{1440, 1080},33331760L,33331760L},
    {{1280, 960}, 33331760L, 33331760L},
    {{1280, 720}, 33331760L, 33331760L},
    {{960, 720}, 33331760L, 33331760L},
    {{864, 480}, 33331760L, 33331760L},
    {{720, 544}, 33331760L, 33331760L},
    {{720, 480}, 33331760L, 33331760L},
    {{640, 480}, 33331760L, 33331760L},
    {{640, 360}, 33331760L, 33331760L},
    {{624, 352}, 33331760L, 33331760L},
    {{480, 640}, 33331760L, 33331760L},
    {{480, 480}, 33331760L, 33331760L},
    {{352, 288}, 33331760L, 33331760L},
    {{320, 240}, 33331760L, 33331760L},
    {{288, 352}, 33331760L, 33331760L},
    {{240, 320}, 33331760L, 33331760L},
    {{176, 144}, 33331760L, 33331760L}};

const float kavailable_lens_info_aperture[] = {1.8, 2.0, 2.2, 2.6, 2.8, 3.0};

const int64_t kavailable_min_durations[1] = {
    33331760L,
};

const int32_t kmax_regions[3] = {
    1, 0, 1,
};

const int32_t kmax_front_regions[3] = {
    1, 0, 0,
};

const int32_t kavailable_test_pattern_modes[] = {
    ANDROID_SENSOR_TEST_PATTERN_MODE_OFF,
};

const uint8_t kavailable_aberration_modes[] = {
    ANDROID_COLOR_CORRECTION_ABERRATION_MODE_FAST,
    ANDROID_COLOR_CORRECTION_ABERRATION_MODE_HIGH_QUALITY};

const uint8_t kavailable_edge_modes[] = {ANDROID_EDGE_MODE_OFF,
                                         ANDROID_EDGE_MODE_FAST,
                                         ANDROID_EDGE_MODE_HIGH_QUALITY};

const int32_t ksensitivity_range[2] = {
    100, 3200,
};

const uint8_t kavailable_tone_map_modes[] = {ANDROID_TONEMAP_MODE_FAST,
                                             ANDROID_TONEMAP_MODE_HIGH_QUALITY};

const float kcolor_gains[] = {
    1.69f, 1.00f, 1.00f, 2.41f,
};
const float kfocus_range[] = {
    0.0f, 0.0f, 0.0f, 0.0f,
};

camera_metadata_rational_t kcolor_transform[] = {
    {9, 10}, {0, 1}, {0, 1}, {1, 5}, {1, 2}, {0, 1}, {0, 1}, {1, 10}, {7, 10}};

const int32_t kavailable_characteristics_keys[] = {
    ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL,
    ANDROID_LENS_INFO_MINIMUM_FOCUS_DISTANCE,
    ANDROID_LENS_INFO_HYPERFOCAL_DISTANCE,
    ANDROID_LENS_INFO_AVAILABLE_FOCAL_LENGTHS,
    ANDROID_LENS_INFO_AVAILABLE_APERTURES,
    ANDROID_LENS_INFO_AVAILABLE_FILTER_DENSITIES,
    ANDROID_LENS_INFO_AVAILABLE_OPTICAL_STABILIZATION,
    ANDROID_LENS_INFO_SHADING_MAP_SIZE,
    ANDROID_SENSOR_INFO_PHYSICAL_SIZE,
    ANDROID_SENSOR_INFO_EXPOSURE_TIME_RANGE,
    ANDROID_SENSOR_INFO_MAX_FRAME_DURATION,
    ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT,
    ANDROID_SENSOR_INFO_PIXEL_ARRAY_SIZE,
    ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE,
    ANDROID_SENSOR_INFO_WHITE_LEVEL,
    ANDROID_SENSOR_BLACK_LEVEL_PATTERN,
    ANDROID_SENSOR_ORIENTATION,
    ANDROID_FLASH_INFO_CHARGE_DURATION,
    ANDROID_TONEMAP_MAX_CURVE_POINTS,
    ANDROID_STATISTICS_INFO_MAX_FACE_COUNT,
    ANDROID_STATISTICS_INFO_HISTOGRAM_BUCKET_COUNT,
    ANDROID_STATISTICS_INFO_MAX_HISTOGRAM_COUNT,
    ANDROID_STATISTICS_INFO_SHARPNESS_MAP_SIZE,
    ANDROID_STATISTICS_INFO_MAX_SHARPNESS_MAP_VALUE,
    ANDROID_STATISTICS_INFO_AVAILABLE_FACE_DETECT_MODES,
    ANDROID_STATISTICS_INFO_AVAILABLE_LENS_SHADING_MAP_MODES,
    ANDROID_SHADING_AVAILABLE_MODES,
    ANDROID_SCALER_AVAILABLE_RAW_MIN_DURATIONS,
    ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS,
    ANDROID_SCALER_AVAILABLE_PROCESSED_MIN_DURATIONS,
    ANDROID_SCALER_AVAILABLE_MAX_DIGITAL_ZOOM,
    ANDROID_SCALER_AVAILABLE_JPEG_MIN_DURATIONS,
    ANDROID_SCALER_AVAILABLE_MIN_FRAME_DURATIONS,
    ANDROID_SCALER_AVAILABLE_STALL_DURATIONS,
    ANDROID_CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES,
    ANDROID_CONTROL_AE_COMPENSATION_STEP,
    ANDROID_CONTROL_AVAILABLE_VIDEO_STABILIZATION_MODES,
    ANDROID_CONTROL_MAX_REGIONS,
    ANDROID_CONTROL_AE_COMPENSATION_RANGE,
    ANDROID_CONTROL_AVAILABLE_EFFECTS,
    ANDROID_CONTROL_AVAILABLE_SCENE_MODES,
    ANDROID_CONTROL_AE_AVAILABLE_ANTIBANDING_MODES,
    ANDROID_CONTROL_AF_AVAILABLE_MODES,
    ANDROID_CONTROL_AWB_AVAILABLE_MODES,
    ANDROID_CONTROL_AE_AVAILABLE_MODES,
    ANDROID_SPRD_AVAILABLE_METERING_MODE,
    ANDROID_QUIRKS_USE_PARTIAL_RESULT,
    ANDROID_LENS_FACING,
    ANDROID_JPEG_AVAILABLE_THUMBNAIL_SIZES,
    ANDROID_JPEG_MAX_SIZE,
    ANDROID_FLASH_FIRING_POWER,
    ANDROID_FLASH_INFO_AVAILABLE,
    ANDROID_REQUEST_MAX_NUM_OUTPUT_STREAMS,
    ANDROID_REQUEST_AVAILABLE_CHARACTERISTICS_KEYS,
    ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS,
    ANDROID_REQUEST_AVAILABLE_RESULT_KEYS,
    ANDROID_REQUEST_AVAILABLE_CAPABILITIES,
    ANDROID_SPRD_AVAILABLE_BRIGHTNESS,
    ANDROID_SPRD_AVAILABLE_CONTRAST,
    ANDROID_SPRD_AVAILABLE_SATURATION,
    ANDROID_SPRD_AVAILABLE_ISO,
    ANDROID_SPRD_FLASH_MODE_SUPPORT,
    ANDROID_SPRD_PRV_REC_DIFFERENT_SIZE_SUPPORT,
    ANDROID_SPRD_VIDEO_SNAPSHOT_SUPPORT,
    ANDROID_NOISE_REDUCTION_AVAILABLE_NOISE_REDUCTION_MODES,
    ANDROID_SENSOR_AVAILABLE_TEST_PATTERN_MODES,
    ANDROID_COLOR_CORRECTION_AVAILABLE_ABERRATION_MODES,
    ANDROID_EDGE_AVAILABLE_EDGE_MODES,
    ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION,
    ANDROID_REQUEST_PARTIAL_RESULT_COUNT,
    ANDROID_REQUEST_PIPELINE_MAX_DEPTH,
    ANDROID_SCALER_CROPPING_TYPE,
    ANDROID_SENSOR_INFO_SENSITIVITY_RANGE,
    ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE,
    ANDROID_SENSOR_MAX_ANALOG_SENSITIVITY,
    ANDROID_SYNC_MAX_LATENCY,
    ANDROID_SPRD_AVAILABLE_AUTO_HDR,
    ANDROID_TONEMAP_AVAILABLE_TONE_MAP_MODES,
    ANDROID_SPRD_AVAILABLE_AI_SCENE,
    ANDROID_SPRD_AVAILABLE_AUTO_3DNR,
    ANDROID_SPRD_AVAILABLE_FACEAGEENABLE,
};

const int32_t kavailable_request_keys[] = {
    ANDROID_COLOR_CORRECTION_ABERRATION_MODE,
    ANDROID_CONTROL_AE_ANTIBANDING_MODE,
    ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION, ANDROID_CONTROL_AE_LOCK,
    ANDROID_CONTROL_AE_MODE, ANDROID_CONTROL_AE_TARGET_FPS_RANGE,
    ANDROID_CONTROL_AE_REGIONS, ANDROID_CONTROL_AF_MODE,
    ANDROID_CONTROL_AF_TRIGGER,
    // ANDROID_CONTROL_AF_REGIONS,
    ANDROID_CONTROL_AWB_LOCK, ANDROID_CONTROL_AWB_MODE,
    // ANDROID_CONTROL_AWB_REGIONS,
    ANDROID_CONTROL_CAPTURE_INTENT, ANDROID_CONTROL_EFFECT_MODE,
    ANDROID_CONTROL_MODE, ANDROID_CONTROL_SCENE_MODE, ANDROID_CONTROL_ENABLE_ZSL,
    ANDROID_CONTROL_VIDEO_STABILIZATION_MODE, ANDROID_FLASH_MODE,
    ANDROID_JPEG_GPS_COORDINATES, ANDROID_JPEG_GPS_PROCESSING_METHOD,
    ANDROID_JPEG_GPS_TIMESTAMP, ANDROID_JPEG_ORIENTATION, ANDROID_JPEG_QUALITY,
    ANDROID_JPEG_THUMBNAIL_QUALITY, ANDROID_JPEG_THUMBNAIL_SIZE,
    // ANDROID_LENS_FOCAL_LENGTH,
    // ANDROID_NOISE_REDUCTION_MODE,
    ANDROID_SCALER_CROP_REGION, ANDROID_STATISTICS_FACE_DETECT_MODE,
    ANDROID_STATISTICS_LENS_SHADING_MAP_MODE, ANDROID_LENS_APERTURE,
    ANDROID_LENS_FILTER_DENSITY, ANDROID_LENS_OPTICAL_STABILIZATION_MODE,
    ANDROID_LENS_FOCAL_LENGTH, ANDROID_SENSOR_FRAME_DURATION,
    ANDROID_SENSOR_EXPOSURE_TIME,
    // ANDROID_SENSOR_SENSITIVITY,
    // ANDROID_BLACK_LEVEL_LOCK,
    ANDROID_TONEMAP_MODE, ANDROID_COLOR_CORRECTION_GAINS,
    ANDROID_COLOR_CORRECTION_TRANSFORM, ANDROID_SHADING_MODE, ANDROID_EDGE_MODE,
    ANDROID_NOISE_REDUCTION_MODE};
const int32_t front_kavailable_request_keys[] = {
    ANDROID_COLOR_CORRECTION_ABERRATION_MODE,
    ANDROID_CONTROL_AE_ANTIBANDING_MODE,
    ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION, ANDROID_CONTROL_AE_LOCK,
    ANDROID_CONTROL_AE_MODE, ANDROID_CONTROL_AE_TARGET_FPS_RANGE,
    ANDROID_CONTROL_AE_REGIONS, ANDROID_CONTROL_AF_MODE,
    ANDROID_CONTROL_AF_TRIGGER,
    // ANDROID_CONTROL_AF_REGIONS,
    ANDROID_CONTROL_AWB_LOCK, ANDROID_CONTROL_AWB_MODE,
    // ANDROID_CONTROL_AWB_REGIONS,
    ANDROID_CONTROL_CAPTURE_INTENT, ANDROID_CONTROL_EFFECT_MODE,
    ANDROID_CONTROL_MODE, ANDROID_CONTROL_SCENE_MODE, ANDROID_CONTROL_ENABLE_ZSL,
    ANDROID_CONTROL_VIDEO_STABILIZATION_MODE, ANDROID_FLASH_MODE,
    ANDROID_JPEG_GPS_COORDINATES, ANDROID_JPEG_GPS_PROCESSING_METHOD,
    ANDROID_JPEG_GPS_TIMESTAMP, ANDROID_JPEG_ORIENTATION, ANDROID_JPEG_QUALITY,
    ANDROID_JPEG_THUMBNAIL_QUALITY, ANDROID_JPEG_THUMBNAIL_SIZE,
    // ANDROID_LENS_FOCAL_LENGTH,
    // ANDROID_NOISE_REDUCTION_MODE,
    ANDROID_SCALER_CROP_REGION, ANDROID_STATISTICS_FACE_DETECT_MODE,
    ANDROID_STATISTICS_LENS_SHADING_MAP_MODE, ANDROID_LENS_APERTURE,
    ANDROID_LENS_FILTER_DENSITY, ANDROID_LENS_OPTICAL_STABILIZATION_MODE,
    ANDROID_LENS_FOCAL_LENGTH, ANDROID_SENSOR_FRAME_DURATION,
    ANDROID_SENSOR_EXPOSURE_TIME,
    // ANDROID_SENSOR_SENSITIVITY,
    // ANDROID_BLACK_LEVEL_LOCK,
    ANDROID_TONEMAP_MODE, ANDROID_COLOR_CORRECTION_GAINS,
    ANDROID_COLOR_CORRECTION_TRANSFORM, ANDROID_SHADING_MODE, ANDROID_EDGE_MODE,
    ANDROID_NOISE_REDUCTION_MODE};

const int32_t kavailable_result_keys[] = {
    ANDROID_COLOR_CORRECTION_ABERRATION_MODE,
    ANDROID_CONTROL_AE_ANTIBANDING_MODE,
    ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION, ANDROID_CONTROL_AE_LOCK,
    ANDROID_CONTROL_AE_MODE, ANDROID_CONTROL_AF_MODE, ANDROID_CONTROL_AF_STATE,
    ANDROID_CONTROL_AWB_MODE, ANDROID_CONTROL_AWB_LOCK, ANDROID_CONTROL_MODE,
    ANDROID_CONTROL_ENABLE_ZSL, ANDROID_FLASH_MODE, ANDROID_JPEG_GPS_COORDINATES,
    ANDROID_JPEG_GPS_PROCESSING_METHOD, ANDROID_JPEG_GPS_TIMESTAMP,
    ANDROID_JPEG_ORIENTATION, ANDROID_JPEG_QUALITY,
    ANDROID_JPEG_THUMBNAIL_QUALITY, ANDROID_LENS_FOCAL_LENGTH,
    // ANDROID_NOISE_REDUCTION_MODE,
    ANDROID_REQUEST_PIPELINE_DEPTH, ANDROID_SCALER_CROP_REGION,
    ANDROID_SENSOR_TIMESTAMP, ANDROID_STATISTICS_FACE_DETECT_MODE,

    ANDROID_SENSOR_FRAME_DURATION, ANDROID_SENSOR_EXPOSURE_TIME,
    // ANDROID_BLACK_LEVEL_LOCK,
    ANDROID_EDGE_MODE, ANDROID_NOISE_REDUCTION_MODE};

const uint8_t kavailable_capabilities[] = {
    ANDROID_REQUEST_AVAILABLE_CAPABILITIES_BACKWARD_COMPATIBLE,
    // ANDROID_REQUEST_AVAILABLE_CAPABILITIES_MANUAL_SENSOR,
    // ANDROID_REQUEST_AVAILABLE_CAPABILITIES_MANUAL_POST_PROCESSING,
    // ANDROID_REQUEST_AVAILABLE_CAPABILITIES_RAW,
    ANDROID_REQUEST_AVAILABLE_CAPABILITIES_BURST_CAPTURE,
};

const uint8_t kavailable_noise_reduction_modes[] = {
    ANDROID_NOISE_REDUCTION_MODE_OFF, ANDROID_NOISE_REDUCTION_MODE_FAST,
    ANDROID_NOISE_REDUCTION_MODE_HIGH_QUALITY};
const uint8_t kavailable_lens_shading_map_modes[] = {
    ANDROID_STATISTICS_LENS_SHADING_MAP_MODE_OFF,
    ANDROID_STATISTICS_LENS_SHADING_MAP_MODE_ON};

const uint8_t kavailable_shading_modes[] = {ANDROID_SHADING_MODE_OFF,
                                            ANDROID_SHADING_MODE_FAST,
                                            ANDROID_SHADING_MODE_HIGH_QUALITY};

/**********************Static Members**********************/

SprdCameraParameters SprdCamera3Setting::mDefaultParameters;
camera_metadata_t *SprdCamera3Setting::mStaticMetadata[CAMERA_ID_COUNT];
CameraMetadata SprdCamera3Setting::mStaticInfo[CAMERA_ID_COUNT];

const int64_t USEC = 1000LL;
const int64_t MSEC = USEC * 1000LL;
const int64_t SEC = MSEC * 1000LL;

sprd_setting_info_t SprdCamera3Setting::s_setting[CAMERA_ID_COUNT];
int SprdCamera3Setting::mLogicalSensorNum = 0;
int SprdCamera3Setting::mPhysicalSensorNum = 0;
uint8_t SprdCamera3Setting::camera_identify_state[] = {1, 1, 1, 1, 1, 1};

enum cmr_flash_lcd_mode {
    FLASH_LCD_MODE_OFF = 0,
    FLASH_LCD_MODE_AUTO = 1,
    FLASH_LCD_MODE_ON = 2,
    FLASH_LCD_MODE_MAX
};
/**********************Function********************************/
int SprdCamera3Setting::parse_int(const char *str, int *data, char delim,
                                  char **endptr) {
    /* Find the first integer.*/
    char *end;
    int w = static_cast<int>(strtol(str, &end, 10));
    /*If a delimeter does not immediately follow, give up.*/
    if (*end != delim && *end != '\0') {
        ALOGE("Cannot find delimeter (%c) in str=%s", delim, str);
        return -1;
    }
    *data = w;
    if (endptr) {
        *endptr = end;
    }
    return 0;
}

int SprdCamera3Setting::parse_float(const char *str, float *data, char delim,
                                    char **endptr) {
    char *end;
    float w = strtof(str, &end);

    if (*end != delim && *end != '\0') {
        ALOGE("Cannot find delimeter (%c) in str=%s", delim, str);
        return -1;
    }
    *data = w;
    if (endptr) {
        *endptr = end;
    }
    return 0;
}

int SprdCamera3Setting::parse_string(char *str, char *data, char delim,
                                     char **endptr) {
    /* Find the first integer.*/
    unsigned int len = strlen(str);
    char *strstart = str;
    char *datastart = data;

    for (size_t i = 0; i < len; i++) {
        if (*strstart == delim)
            break;
        *datastart++ = *strstart++;
    }

    if (endptr) {
        *endptr = strstart;
    }
    *datastart = '\0';
    return 0;
}

void SprdCamera3Setting::parseStringInt(const char *sizesStr, int *fdStr) {
    if (sizesStr == 0 || fdStr == 0) {
        return;
    }

    char *sizeStartPtr = const_cast<char *>(sizesStr);
    int *fdStartStr = fdStr;

    while (true) {
        int success = parse_int(sizeStartPtr, fdStartStr, ',', &sizeStartPtr);
        if (success == -1 || (*sizeStartPtr != ',' && *sizeStartPtr != '\0')) {
            ALOGE("Picture sizes string \"%s\" contains invalid character.",
                  sizesStr);
            return;
        }
        if (*sizeStartPtr == '\0') {
            return;
        }
        sizeStartPtr++;
        fdStartStr++;
    }
}

void SprdCamera3Setting::SprdCamera3Setting::parseStringfloat(
    const char *sizesStr, float *fdStr) {
    if (sizesStr == 0 || fdStr == 0) {
        return;
    }

    char *sizeStartPtr = const_cast<char *>(sizesStr);
    float *fdStartStr = fdStr;

    while (true) {
        int success = parse_float(sizeStartPtr, fdStartStr, ',', &sizeStartPtr);
        if (success == -1 || (*sizeStartPtr != ',' && *sizeStartPtr != '\0')) {
            ALOGE("Picture sizes string \"%s\" contains invalid character.",
                  sizesStr);
            return;
        }

        if (*sizeStartPtr == '\0') {
            return;
        }
        sizeStartPtr++;
        fdStartStr++;
    }
}

int SprdCamera3Setting::getJpegStreamSize(int32_t cameraId, cmr_u16 width,
                                          cmr_u16 height) {
    char value[PROPERTY_VALUE_MAX];
    int32_t jpeg_stream_size = 0;

    if (width * height <= 1600 * 1200) {
        // 2M
        jpeg_stream_size = (1600 * 1200 * 3 / 2 + sizeof(camera3_jpeg_blob_t));
    } else if (width * height <= 2048 * 1536) {
        // 3M
        jpeg_stream_size = (2048 * 1536 * 3 / 2 + sizeof(camera3_jpeg_blob_t));
    } else if (width * height <= 2592 * 1944) {
        // 5M
        jpeg_stream_size = (2592 * 1944 * 3 / 2 + sizeof(camera3_jpeg_blob_t));
    } else if (width * height <= 3264 * 2448) {
        // 8M
        jpeg_stream_size = (3264 * 2448 * 3 / 2 + sizeof(camera3_jpeg_blob_t));
    } else if (width * height <= 4160 * 3120) {
        // 13M
        jpeg_stream_size = (4160 * 3120 * 3 / 2 + sizeof(camera3_jpeg_blob_t));
    } else if (width * height <= 4608 * 3456) {
        // 16M
        jpeg_stream_size = (4608 * 3456 * 3 / 2 + sizeof(camera3_jpeg_blob_t));
    } else if (width * height <= 5312 * 3984) {
        // 21M
        jpeg_stream_size = (5312 * 3984 * 3 / 2 + sizeof(camera3_jpeg_blob_t));
    } else {
        jpeg_stream_size = (5312 * 3984 * 3 / 2 + sizeof(camera3_jpeg_blob_t));
    }

    // enlarge buffer size for isp debug info for userdebug version
    property_get("ro.debuggable", value, "0");
    if (!strcmp(value, "1")) {
        jpeg_stream_size += 1024 * 1024;
    }

    CMR_LOGI("jpeg_stream_size = %d", jpeg_stream_size);

    return jpeg_stream_size;
}

int SprdCamera3Setting::setLargestSensorSize(int32_t cameraId, cmr_u16 width,
                                             cmr_u16 height) {
    sensor_max_width[cameraId] = width;
    sensor_max_height[cameraId] = height;

    HAL_LOGD("camera id = %d, max_width =%d, max_height = %d", cameraId,
             sensor_max_width[cameraId], sensor_max_height[cameraId]);

    return 0;
}

int SprdCamera3Setting::getLargestSensorSize(int32_t cameraId, cmr_u16 *width,
                                             cmr_u16 *height) {
    *width = sensor_max_width[cameraId];
    *height = sensor_max_height[cameraId];

    // just for camera developer debug
    char value[PROPERTY_VALUE_MAX];
    property_get("persist.vendor.cam.auto.detect.sensor", value, "on");
    if (!strcmp(value, "off")) {
        HAL_LOGI("turn off auto detect sensor, just for debug");
        *width = default_sensor_max_sizes[cameraId].width;
        *height = default_sensor_max_sizes[cameraId].height;
    }

    HAL_LOGD("camId=%d, max_width=%d, max_height=%d", cameraId, *width,
             *height);
    return 0;
}

int SprdCamera3Setting::getLargestPictureSize(int32_t cameraId, cmr_u16 *width,
                                              cmr_u16 *height) {
    *width = largest_picture_size[cameraId].width;
    *height = largest_picture_size[cameraId].height;
    HAL_LOGD("camId=%d, max_width=%d, max_height=%d", cameraId, *width,
             *height);
    return 0;
}

int SprdCamera3Setting::getSensorStaticInfo(int32_t cameraId) {
    struct phySensorInfo *phyPtr = NULL;
    int ret = 0;

    // just for camera developer debug
    char value[PROPERTY_VALUE_MAX];
    property_get("persist.vendor.cam.auto.detect.sensor", value, "on");
    if (!strcmp(value, "off")) {
        HAL_LOGI("turn off auto detect sensor, just for debug");
        return 0;
    }

    HAL_LOGI("E");

    phyPtr = sensorGetPhysicalSnsInfo(cameraId);

    if (phyPtr == NULL) {
        HAL_LOGE("open camera (%d) failed, can't get sensor info", cameraId);
        goto exit;
    }

    mSensorFocusEnable[cameraId] = phyPtr->focus_eb;

    // if sensor fov info is valid, use it; else use default value
    if (phyPtr->fov_info.physical_size[0] > 0 &&
        phyPtr->fov_info.physical_size[1] > 0 &&
        phyPtr->fov_info.focal_lengths > 0) {
        memcpy(&sensor_fov[cameraId], &phyPtr->fov_info,
               sizeof(phyPtr->fov_info));
    }

    if (phyPtr->source_width_max == 1920 && phyPtr->source_height_max == 1080) {
        setLargestSensorSize(cameraId, 1920, 1088);
    } else {
        setLargestSensorSize(cameraId, phyPtr->source_width_max,
                             phyPtr->source_height_max);
    }

    HAL_LOGI("camera id = %d, sensor_max_height = %d, sensor_max_width= %d",
             cameraId, phyPtr->source_height_max, phyPtr->source_width_max);

    HAL_LOGI("sensor sensorFocusEnable = %d, fov physical size (%f, "
             "%f), focal_lengths %f",
             mSensorFocusEnable[cameraId],
             sensor_fov[cameraId].physical_size[0],
             sensor_fov[cameraId].physical_size[1],
             sensor_fov[cameraId].focal_lengths);

exit:

    HAL_LOGI("X");
    return 0;
}

void SprdCamera3Setting::coordinate_struct_convert(int *rect_arr,
                                                   int arr_size) {
    int i = 0;
    int left = 0, top = 0, right = 0, bottom = 0;
    int width = 0, height = 0;
    int *rect_arr_copy = rect_arr;

    for (i = 0; i < arr_size / 4; i++) {
        left = rect_arr[i * 4];
        top = rect_arr[i * 4 + 1];
        right = rect_arr[i * 4 + 2];
        bottom = rect_arr[i * 4 + 3];
        width = (((right - left + 3) >> 2) << 2);
        height = (((bottom - top + 3) >> 2) << 2);
        rect_arr[i * 4 + 2] = width;
        rect_arr[i * 4 + 3] = height;
        HAL_LOGD("test:zone: left=%d,top=%d,right=%d,bottom=%d, w=%d, h=%d \n",
                 left, top, right, bottom, width, height);
    }

    for (i = 0; i < arr_size / 4; i++) {
        HAL_LOGD("test:zone:%d,%d,%d,%d.\n", rect_arr_copy[i * 4],
                 rect_arr_copy[i * 4 + 1], rect_arr_copy[i * 4 + 2],
                 rect_arr_copy[i * 4 + 3]);
    }
}

int SprdCamera3Setting::coordinate_convert(int *rect_arr, int arr_size,
                                           int angle, int is_mirror,
                                           struct img_size *preview_size,
                                           struct img_rect *preview_rect) {
    int i;
    int x1;
    int y1;
    int ret = 0;
    int new_width = preview_rect->width;
    int new_height = preview_rect->height;
    int point_x, point_y;

    HAL_LOGD("coordinate_convert: preview_rect x=%d, y=%d, width=%d, height=%d",
             preview_rect->start_x, preview_rect->start_y, preview_rect->width,
             preview_rect->height);
    HAL_LOGD("coordinate_convert: arr_size=%d, angle=%d, is_mirror=%d \n",
             arr_size, angle, is_mirror);

    UNUSED(preview_size);
    for (i = 0; i < arr_size * 2; i++) {
        x1 = rect_arr[i * 2];
        y1 = rect_arr[i * 2 + 1];

        rect_arr[i * 2] = (1000 + x1) * new_width / 2000;
        rect_arr[i * 2 + 1] = (1000 + y1) * new_height / 2000;

        HAL_LOGD("coordinate_convert rect i=%d x=%d y=%d", i, rect_arr[i * 2],
                 rect_arr[i * 2 + 1]);
    }

    /*move to cap image coordinate*/
    point_x = preview_rect->start_x;
    point_y = preview_rect->start_y;
    for (i = 0; i < arr_size; i++) {

        HAL_LOGD("coordinate_convert %d: org: %d, %d, %d, %d.\n", i,
                 rect_arr[i * 4], rect_arr[i * 4 + 1], rect_arr[i * 4 + 2],
                 rect_arr[i * 4 + 3]);

        rect_arr[i * 4] += point_x;
        rect_arr[i * 4 + 1] += point_y;
        rect_arr[i * 4 + 2] += point_x;
        rect_arr[i * 4 + 3] += point_y;

        HAL_LOGD("coordinate_convert %d: final: %d, %d, %d, %d.\n", i,
                 rect_arr[i * 4], rect_arr[i * 4 + 1], rect_arr[i * 4 + 2],
                 rect_arr[i * 4 + 3]);
    }

    return ret;
}

void *SprdCamera3Setting::getCameraIdentifyState() {
    void *devPtr = NULL;
    devPtr = sensorGetIdentifyState();
    for (int m = 0; m < mPhysicalSensorNum; m++) {
        camera_identify_state[m] =
            ((struct camera_device_manager *)devPtr)->identify_state[m];
        HAL_LOGV("identify_state[%d]=%d", m, camera_identify_state[m]);
    }
    return devPtr;
}

int SprdCamera3Setting::getCameraInfo(int32_t cameraId,
                                      struct camera_info *cameraInfo) {
    struct phySensorInfo *phyPtr = NULL;

    if (cameraInfo == NULL) {
        HAL_LOGE("cameraInfo is NULL");
        return -1;
    }

    HAL_LOGI("cameraId=%d", cameraId);

    // TBD: for spreadtrum internal development use
    // add struct light camera info and three back camera phone info

    if (cameraId >= mPhysicalSensorNum) {
        HAL_LOGE("failed");
        return -1;
    }

    phyPtr = sensorGetPhysicalSnsInfo(cameraId);

    if (phyPtr->phyId == SENSOR_ID_INVALID) {
        cameraInfo->facing = -1;
        cameraInfo->orientation = -1;
        cameraInfo->resource_cost = -1;
    } else {
        cameraInfo->facing = phyPtr->face_type;
        cameraInfo->orientation = phyPtr->angle;
        cameraInfo->resource_cost = phyPtr->resource_cost;
    }
    // TBD: may be will add other variable in struct camera_info

    return 0;
}

int SprdCamera3Setting::getCameraIPInited() {
    void *handle = dlopen("libinterface.so", RTLD_LAZY);
    char *error = NULL;
    if(!handle) {
        HAL_LOGD("CameraIP open failed");
        return -1;
    }
    CAMIP_INTERFACE_INIT camip_init = (CAMIP_INTERFACE_INIT)dlsym(handle, "interface_init");
    if(!camip_init) {
        HAL_LOGD("dlysm func failed");
        dlclose(handle);
        return -1;
    }
    camip_init(&error);
    dlclose(handle);
    return 0;
}

int SprdCamera3Setting::getNumberOfCameras() {
    int num = 0;

    mPhysicalSensorNum = sensorGetPhysicalSnsNum();

    if (mPhysicalSensorNum) {
        mLogicalSensorNum = sensorGetLogicalSnsNum();
    }

    num = mPhysicalSensorNum;

    LOGI("getNumberOfCameras:%d", num);

    return num;
}

bool SprdCamera3Setting::isFaceBeautyOn(SPRD_DEF_Tag sprddefInfo) {
    for (int i = 0; i < SPRD_FACE_BEAUTY_PARAM_NUM; i++) {
        if (sprddefInfo.perfect_skin_level[i] != 0)
            return true;
    }
    return false;
}

int SprdCamera3Setting::setDefaultParaInfo(int32_t cameraId) {
    // camera3_default_info.common.aperture = 2.8f;
    camera3_default_info.common.filter_density = 0.0f;
    camera3_default_info.common.optical_stabilization = 0;

    memcpy(camera3_default_info.common.lens_shading_map_size,
           klens_shading_map_size, sizeof(klens_shading_map_size));

    memcpy(camera3_default_info.common.sensor_physical_size,
           sensor_fov[cameraId].physical_size,
           sizeof(sensor_fov[cameraId].physical_size));
    HAL_LOGI("Camera %d, physical_size %f, %f", cameraId,
             camera3_default_info.common.sensor_physical_size[0],
             camera3_default_info.common.sensor_physical_size[1]);

    // memcpy(camera3_default_info.common.sensor_physical_size,ksensor_physical_size,sizeof(ksensor_physical_size));
    memcpy(camera3_default_info.common.exposure_time_range,
           kexposure_time_range, sizeof(kexposure_time_range));
    memcpy(camera3_default_info.common.frame_duration_range,
           kframe_duration_range, sizeof(kframe_duration_range));
    camera3_default_info.common.color_arrangement = 0;
    camera3_default_info.common.white_level = 4000;
    camera3_default_info.common.black_level = 1000;
    camera3_default_info.common.flash_charge_duration = 0;
    camera3_default_info.common.max_tone_map_curve_points =
        SPRD_MAX_TONE_CURVE_POINT;
    camera3_default_info.common.histogram_size = 64;
    camera3_default_info.common.max_histogram_count = 1000;
    memcpy(camera3_default_info.common.sharpness_map_size, ksharpness_map_size,
           sizeof(ksharpness_map_size));
    camera3_default_info.common.max_sharpness_map_value = 1000;
    camera3_default_info.common.raw_min_duration[0] =
        camera3_default_info.common.frame_duration_range[0];
    memcpy(camera3_default_info.common.supported_preview_formats,
           ksupported_preview_formats, sizeof(ksupported_preview_formats));
    camera3_default_info.common.processed_min_durations[0] =
        camera3_default_info.common.frame_duration_range[0];
    if (cameraId == 0)
        memcpy(camera3_default_info.common.available_fps_ranges,
               kavailable_fps_ranges_back, sizeof(kavailable_fps_ranges_back));
    else
        memcpy(camera3_default_info.common.available_fps_ranges,
               kavailable_fps_ranges_front,
               sizeof(kavailable_fps_ranges_front));
    memcpy(&camera3_default_info.common.exposureCompensationStep,
           &kexposureCompensationStep, sizeof(kexposureCompensationStep));
    memcpy(camera3_default_info.common.exposureCompensationRange,
           kexposureCompensationRange, sizeof(kexposureCompensationRange));
    // memcpy(camera3_default_info.common.available_processed_sizes,
    // kavailable_processed_sizes, sizeof(kavailable_processed_sizes));
    memcpy(camera3_default_info.common.jpegThumbnailSizes, kjpegThumbnailSizes,
           sizeof(kjpegThumbnailSizes));
    memcpy(camera3_default_info.common.FrameDurationRange, kFrameDurationRange,
           sizeof(kFrameDurationRange));

    memcpy(camera3_default_info.common.availableFaceDetectModes,
           availableFaceDetectModes, sizeof(availableFaceDetectModes));
    memcpy(camera3_default_info.common.availableVideoStabModes,
           availableVstabModes, sizeof(availableVstabModes));
    memcpy(camera3_default_info.common.availEffectModes, avail_effect_mode,
           sizeof(avail_effect_mode));
    memcpy(camera3_default_info.common.availSceneModes, avail_scene_modes,
           sizeof(avail_scene_modes));
    memcpy(camera3_default_info.common.availAntibandingModes,
           avail_antibanding_modes, sizeof(avail_antibanding_modes));
    memset(camera3_default_info.common.availableAfModes, 0, 6);

    if (mSensorFocusEnable[cameraId]) {
        memcpy(camera3_default_info.common.availableAfModes, availableAfModes,
               sizeof(availableAfModes));
    } else {
        memcpy(camera3_default_info.common.availableAfModes,
               availableAfModesOfFront, sizeof(availableAfModesOfFront));
    }

    memcpy(camera3_default_info.common.availableSlowMotion, availableSlowMotion,
           sizeof(availableSlowMotion));
    memcpy(camera3_default_info.common.avail_awb_modes, avail_awb_modes,
           sizeof(avail_awb_modes));
    memcpy(camera3_default_info.common.availableAeModes, availableAeModes,
           sizeof(availableAeModes));
    memcpy(camera3_default_info.common.availableAmModes, availableAmModes,
           sizeof(availableAmModes));
    memcpy(camera3_default_info.common.max_output_streams, max_output_streams,
           sizeof(max_output_streams));
    memcpy(camera3_default_info.common.availableBrightNess, availableBrightNess,
           sizeof(availableBrightNess));
    memcpy(camera3_default_info.common.availableIso, availableIso,
           sizeof(availableIso));
    memcpy(camera3_default_info.common.availableFaceDetectModes,
           availableFaceDetectModes, sizeof(availableFaceDetectModes));
    camera3_default_info.common.availableAutoHdr = availableAutoHDR;
    camera3_default_info.common.availLogoWatermark = availLogoWatermark;
    camera3_default_info.common.availTimeWatermark = availTimeWatermark;

    camera3_default_info.common.availableAiScene = availableAiScene;

    camera3_default_info.common.availableAuto3Dnr = availableAuto3DNR;
    memcpy(camera3_default_info.common.availDistortionCorrectionModes,
           availDistortionCorrectionModes,
           sizeof(availDistortionCorrectionModes));

    return 0;
}

bool SprdCamera3Setting::getLcdSize(uint32_t *width, uint32_t *height) {
    char const *const device_template[] = {"/dev/graphics/fb%u", "/dev/fb%u",
                                           NULL};

    int fd = -1;
    int i = 0;
    char name[64];

    if (NULL == width || NULL == height)
        return false;

    while ((fd == -1) && device_template[i]) {
        snprintf(name, 64, device_template[i], 0);
        fd = open(name, O_RDONLY, 0);
        i++;
    }

    LOGI("getLcdSize dev is %s", name);

    if (fd < 0) {
        LOGE("getLcdSize fail to open fb");
        return false;
    }
    close(fd);
    return true;
}

int SprdCamera3Setting::initDefaultParameters(int32_t cameraId) {
    uint32_t lcd_w = 0, lcd_h = 0;
    int ret = NO_ERROR;

    setDefaultParaInfo(cameraId);

    return ret;
}

static bool isBlobOrRaw16(int j , int32_t *scaler_formats){
    bool ispixel = ((scaler_formats[j] == HAL_PIXEL_FORMAT_BLOB) ||
                    (scaler_formats[j] == HAL_PIXEL_FORMAT_RAW16));
    return ispixel;
}

/*
    for sprd camera feature init
*/
void SprdCamera3Setting::initCameraIpFeature(int32_t cameraId) {
    Vector<uint8_t> available_cam_features;
    char prop[PROPERTY_VALUE_MAX] = {
        0,
    };
    uint32_t dualPropSupport = 0;
    bool hasRealCameraUnuseful = false;
    bool hasFrontCameraUnuseful = false;

    for (int i = 0; i < mPhysicalSensorNum; i++) {
        if (camera_identify_state[1] == IDENTIFY_STATUS_NOT_PRESENT) {
            hasFrontCameraUnuseful = true;
        }
        if (i != 1 && camera_identify_state[i] == IDENTIFY_STATUS_NOT_PRESENT) {
            hasRealCameraUnuseful = true;
        }
        HAL_LOGV("camera_identify_state[%d]=%d, hasRealCameraUnuseful=%d, "
                 "hasFrontCameraUnuseful=%d",
                 i, camera_identify_state[i], hasRealCameraUnuseful,
                 hasFrontCameraUnuseful);
    }

    // 0 facebeauty version
    property_get("persist.vendor.cam.facebeauty.corp", prop, "1");

    // 1 back blur or bokeh version
    available_cam_features.add(atoi(prop));
    property_get("persist.vendor.cam.ba.blur.version", prop, "0");
    if (hasRealCameraUnuseful == false && atoi(prop) == 6){
#ifdef CONFIG_BOKEH_HDR_SUPPORT
        available_cam_features.add(9);
#else
        available_cam_features.add(6);
#endif
    } else {
        available_cam_features.add(0);
    }

    // 2 front blur version
    property_get("persist.vendor.cam.fr.blur.version", prop, "0");
    available_cam_features.add(0);

    // 3 blur cover id
    property_get("persist.vendor.cam.blur.cov.id", prop, "3");
    available_cam_features.add(atoi(prop));

    // 4 front flash type
    for (int i = 0; i < (int)ARRAY_SIZE(front_flash); i++) {
        if (!strcmp(FRONT_CAMERA_FLASH_TYPE, front_flash[i].type_name)) {
            available_cam_features.add(atoi(front_flash[i].type_id));
            break;
        }
    }

    // 5 wide and tele enable
    property_get("persist.vendor.cam.wt.enable", prop, "0");
    if (!dualPropSupport) {
        available_cam_features.add(0);
    } else {
        available_cam_features.add(atoi(prop));
    }

    // 6 auto tracking enable
    property_get("persist.vendor.cam.auto.tracking.enable", prop, "0");
    if (cameraId == 0) {
        available_cam_features.add(atoi(prop));
    } else {
        available_cam_features.add(0);
    }

    // 7 back ultra wide enable
    if (hasRealCameraUnuseful == true) {
        available_cam_features.add(0);
    } else if (sensorGetPhyId4Role(SENSOR_ROLE_MULTICAM_SUPERWIDE,
                                   SNS_FACE_BACK) != SENSOR_ID_INVALID) {
        available_cam_features.add(
            resetFeatureStatus("persist.vendor.cam.ip.warp",
                               "persist.vendor.cam.ultra.wide.enable"));
    } else {
        available_cam_features.add(0);
    }

    // 8 bokeh gdepth enable
#ifdef CONFIG_SUPPORT_GDEPTH
    available_cam_features.add(1);
#else
    available_cam_features.add(0);
#endif

    // 9 back portrait mode
#ifdef CONFIG_PORTRAIT_SUPPORT
    if (hasRealCameraUnuseful == true) {
        available_cam_features.add(0);
    } else {
        available_cam_features.add(
            resetFeatureStatus("persist.vendor.cam.ip.daul.portrait",
                               "persist.vendor.cam.ba.portrait.enable"));
    }
#else
    if (hasRealCameraUnuseful == true) {
        available_cam_features.add(0);
    } else {
        available_cam_features.add(
            resetFeatureStatus("persist.vendor.cam.ip.single.portrait",
                               "persist.vendor.cam.ba.portrait.enable"));
    }
#endif

    // 10 front portrait mode
    available_cam_features.add(
        resetFeatureStatus("persist.vendor.cam.ip.single.portrait",
                           "persist.vendor.cam.fr.portrait.enable"));

    // 11 montion photo enable
#ifdef CONFIG_CAMERA_MOTION_PHONE
    available_cam_features.add(1);
#else
    available_cam_features.add(0);
#endif

    // 12 default quarter size
#ifdef CONFIG_DEFAULT_CAPTURE_SIZE_8M
    available_cam_features.add(1);
#else
    available_cam_features.add(0);
#endif

    // 13 multi camera superwide & wide & tele
    if (hasRealCameraUnuseful == true) {
        available_cam_features.add(0);
    } else {
        available_cam_features.add(
            resetFeatureStatus("persist.vendor.cam.ip.OpticsZoom",
                               "persist.vendor.cam.multi.camera.enable"));
    }

    // 14 camera back high resolution definition mode
    property_get("persist.vendor.cam.back.high.resolution.mode", prop, "0");
    if (camera_identify_state[0] == IDENTIFY_STATUS_NOT_PRESENT) {
        available_cam_features.add(0);
    } else {
        available_cam_features.add(atoi(prop));
    }

    // 15 camera hdr_zsl
    property_get("persist.vendor.cam.hdr.zsl", prop, "0");
    available_cam_features.add(atoi(prop));

    // 16 MMI opticszoom calibration mode: 1-SW+W, 2-W+T, 3-SW+W+T
    property_get("persist.vendor.cam.opticszoom.cali.mode", prop, "0");
    available_cam_features.add(atoi(prop));

    // 17 camera infrared
    property_get("persist.vendor.cam.infrared.enable", prop, "0");
    if (hasRealCameraUnuseful == true) {
        available_cam_features.add(0);
    } else {
        available_cam_features.add(atoi(prop));
    }

    // 18 camera macrophoto
    property_get("persist.vendor.cam.macrophoto.enable", prop, "0");
    if (hasRealCameraUnuseful == true) {
        available_cam_features.add(0);
    } else {
        available_cam_features.add(atoi(prop));
    }

    // 19 camera macrovideo
    property_get("persist.vendor.cam.macrovideo.enable", prop, "0");
    if (hasRealCameraUnuseful == true) {
        available_cam_features.add(0);
    } else {
        available_cam_features.add(atoi(prop));
    }

    // 20 camera front high resolution definition mode
    property_get("persist.vendor.cam.front.high.resolution.mode", prop, "0");
    available_cam_features.add(!!atoi(prop));

    // 21 stl3denable
    if (sensorGetLogicaInfo4multiCameraId(SPRD_3D_FACE_ID))
        available_cam_features.add(1);
    else
        available_cam_features.add(0);

    // 22 video face beauty
    available_cam_features.add(
        resetFeatureStatus("persist.vendor.cam.ip.video.beauty",
                           "persist.vendor.cam.video.face.beauty.enable"));

    // 23 fov fusion
    if (hasRealCameraUnuseful == true) {
        available_cam_features.add(0);
    } else {
        available_cam_features.add(
            resetFeatureStatus("persist.vendor.cam.ip.wtfusion.pro",
                               "persist.vendor.cam.fov.fusion.enable"));
    }

    // 24 nightshot pro
    available_cam_features.add(resetFeatureStatus(
        "persist.vendor.cam.ip.night", "persist.vendor.cam.night.pro.enable"));

    // 25 camera lightportrait_ba
#ifdef CONFIG_PORTRAIT_SUPPORT
    if (hasRealCameraUnuseful == true) {
        available_cam_features.add(0);
    } else {
        available_cam_features.add(
            resetFeatureStatus("persist.vendor.cam.ip.light.dual.portrait",
                           "persist.vendor.cam.lightportrait.ba.enable"));
    }
#else
    if (hasRealCameraUnuseful == true) {
        available_cam_features.add(0);
    } else {
        available_cam_features.add(
            resetFeatureStatus("persist.vendor.cam.ip.light.single.portrait",
                           "persist.vendor.cam.lightportrait.ba.enable"));
    }
#endif

    // 26 camera lightportrait_fr
    available_cam_features.add(
        resetFeatureStatus("persist.vendor.cam.ip.light.single.portrait",
                           "persist.vendor.cam.lightportrait.fr.enable"));

      // 27 FDR
    available_cam_features.add(resetFeatureStatus("persist.vendor.cam.ip.fdr",
                                      "persist.vendor.cam.fdr.enable"));

    // 28 dual view video
    available_cam_features.add(0);

#ifdef CONFIG_PORTRAIT_SCENE_SUPPORT
    // 29 portrait scene mode
    HAL_LOGD("pbrb_enable=%d", atoi(prop));
    property_get("persist.vendor.cam.wechat.portrait.scene.enable", prop, "2");
    if (hasRealCameraUnuseful == true) {
        available_cam_features.add(0);
    } else {
        if (atoi(prop) == 0) {
            property_set("persist.vendor.cam.wechat.portrait.scene.enable", "1");
            property_set("persist.vendor.cam.ip.wechat.back.replace", "0");
        }
        available_cam_features.add(
            resetFeatureStatus("persist.vendor.cam.ip.portrait.back.replace",
                               "persist.vendor.cam.portrait.scene.enable"));
    }
#else
    available_cam_features.add(0);
#endif

    // 30 higher micro photo
    if (hasRealCameraUnuseful == true) {
        available_cam_features.add(0);
    } else {
        available_cam_features.add(
            resetFeatureStatus("persist.vendor.cam.ip.macro.photo",
                               "persist.vendor.cam.higher.macrophoto.enable"));
    }

    // 31 eis_pro video
    available_cam_features.add(
        resetFeatureStatus("persist.vendor.cam.ip.eis.pro",
                           "persist.vendor.cam.dv.ba.eispro.enable"));

    // 32 portrait mutex lpt not support in pike2
    available_cam_features.add(0);

    // 33 video face detect
    property_get("persist.vendor.cam.video.fd.enable", prop, "0");
    available_cam_features.add(atoi(prop));

    memcpy(s_setting[cameraId].sprddefInfo.sprd_cam_feature_list,
           &(available_cam_features[0]),
           available_cam_features.size() * sizeof(uint8_t));
    s_setting[cameraId].sprddefInfo.sprd_cam_feature_list_size =
        available_cam_features.size();
    memcpy(s_setting[cameraId].lensInfo.distortion_correction_modes,
           camera3_default_info.common.availDistortionCorrectionModes,
           sizeof(camera3_default_info.common.availDistortionCorrectionModes));

    ALOGV("available_cam_features=%d", available_cam_features.size());
    getCameraIPInited();
    property_set("persist.vendor.cam.ip.switch.on", "0");
    HAL_LOGI("available_cam_features = %d",
             s_setting[cameraId].sprddefInfo.sprd_cam_feature_list_size);

}

int SprdCamera3Setting::initStaticParameters(int32_t cameraId) {
    int ret = NO_ERROR;
    SprdCamera3DefaultInfo *default_info = &camera3_default_info;
    int i = 0;
    char value[PROPERTY_VALUE_MAX];
    memset(&(s_setting[cameraId]), 0, sizeof(sprd_setting_info_t));

    s_setting[cameraId].supported_hardware_level =
        ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LIMITED;

    // color
    memcpy(s_setting[cameraId].colorInfo.available_aberration_modes,
           kavailable_aberration_modes, sizeof(kavailable_aberration_modes));

    // edge
    memcpy(s_setting[cameraId].edgeInfo.available_edge_modes,
           kavailable_edge_modes, sizeof(kavailable_edge_modes));

    // lens_info
    if (!mSensorFocusEnable[cameraId]) {
        s_setting[cameraId].lens_InfoInfo.mini_focus_distance = 0.0f;
    } else {
        s_setting[cameraId].lens_InfoInfo.mini_focus_distance =
            cameraId ? 0.0f : 1023.0f;
    }

    s_setting[cameraId].lens_InfoInfo.hyperfocal_distance = 2.0f;

    s_setting[cameraId].lens_InfoInfo.available_focal_lengths =
        sensor_fov[cameraId].focal_lengths;

    s_setting[cameraId].lens_InfoInfo.available_apertures =
        default_info->common.aperture;
    s_setting[cameraId].lens_InfoInfo.filter_density =
        default_info->common.filter_density;
    s_setting[cameraId].lens_InfoInfo.optical_stabilization =
        default_info->common.optical_stabilization;
    memcpy(s_setting[cameraId].lens_InfoInfo.shading_map_size,
           camera3_default_info.common.lens_shading_map_size,
           sizeof(klens_shading_map_size));
    s_setting[cameraId].lens_InfoInfo.focus_distance_calibration =
        ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_UNCALIBRATED;

    // sensor
    struct camera_info cameraInfo;
    memset(&cameraInfo, 0, sizeof(cameraInfo));
    getCameraInfo(cameraId, &cameraInfo);
    s_setting[cameraId].sensorInfo.orientation = cameraInfo.orientation;

    memcpy(s_setting[cameraId].sensorInfo.available_test_pattern_modes,
           kavailable_test_pattern_modes,
           sizeof(kavailable_test_pattern_modes));
    s_setting[cameraId].sensorInfo.max_analog_sensitivity = 800;

    // flash_info
    s_setting[cameraId].flash_InfoInfo.charge_duration =
        default_info->common.flash_charge_duration;

    // tonemap
    s_setting[cameraId].toneInfo.max_curve_points =
        default_info->common.max_tone_map_curve_points;
    memcpy(s_setting[cameraId].toneInfo.available_tone_map_modes,
           kavailable_tone_map_modes, sizeof(kavailable_tone_map_modes));

    // statistics_info
    s_setting[cameraId].statis_InfoInfo.max_face_count = CAMERA3MAXFACE;
    s_setting[cameraId].statis_InfoInfo.histogram_bucket_count =
        default_info->common.histogram_size;
    s_setting[cameraId].statis_InfoInfo.max_histogram_count =
        default_info->common.max_histogram_count;
    memcpy(s_setting[cameraId].statis_InfoInfo.sharpness_map_size,
           camera3_default_info.common.sharpness_map_size,
           sizeof(s_setting[cameraId].statis_InfoInfo.sharpness_map_size));
    s_setting[cameraId].statis_InfoInfo.max_sharpness_map_size =
        default_info->common.max_sharpness_map_value;
    memcpy(s_setting[cameraId].statis_InfoInfo.available_face_detect_modes,
           camera3_default_info.common.availableFaceDetectModes,
           sizeof(camera3_default_info.common.availableFaceDetectModes));

    // scaler
    s_setting[cameraId].scalerInfo.raw_min_duration[0] =
        default_info->common.raw_min_duration[0];

    int32_t scaler_formats[] = {HAL_PIXEL_FORMAT_YCbCr_420_888,
                                HAL_PIXEL_FORMAT_BLOB,
                                HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED};

    size_t scaler_formats_count = sizeof(scaler_formats) / sizeof(int32_t);
    size_t stream_sizes_tbl_cnt = sizeof(stream_info) / sizeof(cam_stream_info);

    cmr_u16 largest_sensor_w = 0;
    cmr_u16 largest_sensor_h = 0;
#ifdef CONFIG_CAMERA_AUTO_DETECT_SENSOR
    largest_sensor_w = sensor_max_width[cameraId];
    largest_sensor_h = sensor_max_height[cameraId];
#else
    largest_sensor_w = default_sensor_max_sizes[cameraId].width;
    largest_sensor_h = default_sensor_max_sizes[cameraId].height;
#endif
    HAL_LOGD("cameraId=%d, largest_sensor_w=%d, largest_sensor_h=%d", cameraId,
             largest_sensor_w, largest_sensor_h);

    /* Add input/output stream configurations for each scaler formats*/
    Vector<int32_t> available_stream_configs;
    for (size_t j = 0; j < scaler_formats_count; j++) {
        for (size_t i = 0; i < stream_sizes_tbl_cnt; i++) {
            if ((stream_info[i].stream_sizes_tbl.width <= largest_sensor_w &&
                 stream_info[i].stream_sizes_tbl.height <= largest_sensor_h) ||
                (stream_info[i].stream_sizes_tbl.width == 480 &&
                 stream_info[i].stream_sizes_tbl.height == 640)) {

                available_stream_configs.add(scaler_formats[j]);
                available_stream_configs.add(
                    stream_info[i].stream_sizes_tbl.width);
                available_stream_configs.add(
                    stream_info[i].stream_sizes_tbl.height);
                available_stream_configs.add(
                    ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT);
                if ((scaler_formats[j] ==
                         HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED ||
                     scaler_formats[j] == HAL_PIXEL_FORMAT_YCBCR_420_888) &&
                    stream_info[i].stream_sizes_tbl.height == 1088) {
                    available_stream_configs.add(scaler_formats[j]);
                    available_stream_configs.add(
                        stream_info[i].stream_sizes_tbl.width);
                    available_stream_configs.add(1080);
                    available_stream_configs.add(
                        ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT);
                }
                /* keep largest */
                if (stream_info[i].stream_sizes_tbl.width >=
                        largest_picture_size[cameraId].width &&
                    stream_info[i].stream_sizes_tbl.height >=
                        largest_picture_size[cameraId].height)
                    largest_picture_size[cameraId] =
                        stream_info[i].stream_sizes_tbl;
            }
        }
    }
    memcpy(s_setting[cameraId].scalerInfo.available_stream_configurations,
           &(available_stream_configs[0]),
           available_stream_configs.size() * sizeof(int32_t));

    /* android.scaler.availableMinFrameDurations */
    Vector<int64_t> available_min_durations;
    for (size_t j = 0; j < scaler_formats_count; j++) {
        for (size_t i = 0; i < stream_sizes_tbl_cnt; i++) {
            if ((stream_info[i].stream_sizes_tbl.width <= largest_sensor_w &&
                 stream_info[i].stream_sizes_tbl.height <= largest_sensor_h) ||
                (stream_info[i].stream_sizes_tbl.width == 480 &&
                 stream_info[i].stream_sizes_tbl.height == 640)) {
                available_min_durations.add(scaler_formats[j]);
                available_min_durations.add(
                    stream_info[i].stream_sizes_tbl.width);
                available_min_durations.add(
                    stream_info[i].stream_sizes_tbl.height);

                if (scaler_formats[j] == HAL_PIXEL_FORMAT_YCbCr_420_888) {
                    if (stream_info[i].stream_sizes_tbl.width ==
                            largest_picture_size[cameraId].width){
                        if ((stream_info[i].stream_sizes_tbl.width == 4160) ||
                             (stream_info[i].stream_sizes_tbl.width == 4608)) {
                            HAL_LOGD("YUV %d*%d output in ~100ms"
                                     "offline so change min frame duration",
                                     stream_info[i].stream_sizes_tbl.width,
                                     stream_info[i].stream_sizes_tbl.height);
                            available_min_durations.add(100000000L);
                        } else if (stream_info[i].stream_sizes_tbl.width == 3264) {
                            HAL_LOGD("YUV 3264*2448 output in ~66ms"
                                     "offline so change min frame duration");
                            available_min_durations.add(66666670L);
                        } else if (stream_info[i].stream_sizes_tbl.width == 2592) {
                            HAL_LOGD("YUV 2592*1944 output in ~66ms"
                                     "offline so change min frame duration");
                            available_min_durations.add(66666670L);
                        } else {
                            available_min_durations.add(
                            stream_info[i].stream_min_duration);
                        }
                    }else{
                        available_min_durations.add(
                        stream_info[i].stream_min_duration);
                    }
                } else{
                    available_min_durations.add(stream_info[i].stream_min_duration);
                }

                if ((scaler_formats[j] ==
                         HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED ||
                     scaler_formats[j] == HAL_PIXEL_FORMAT_YCBCR_420_888) &&
                    stream_info[i].stream_sizes_tbl.height == 1088) {
                    available_min_durations.add(scaler_formats[j]);
                    available_min_durations.add(
                        stream_info[i].stream_sizes_tbl.width);
                    available_min_durations.add(1080);
                    available_min_durations.add(
                        stream_info[i].stream_min_duration);
                }
            }
        }
    }
    memcpy(s_setting[cameraId].scalerInfo.min_frame_durations,
           &(available_min_durations[0]),
           available_min_durations.size() * sizeof(int64_t));

    /*available stall durations*/
    Vector<int64_t> available_stall_durations;
    for (size_t j = 0; j < scaler_formats_count; j++) {
        for (size_t i = 0; i < stream_sizes_tbl_cnt; i++) {
            if ((stream_info[i].stream_sizes_tbl.width <= largest_sensor_w &&
                 stream_info[i].stream_sizes_tbl.height <= largest_sensor_h) ||
                (stream_info[i].stream_sizes_tbl.width == 480 &&
                 stream_info[i].stream_sizes_tbl.height == 640)) {
                if (isBlobOrRaw16(j,scaler_formats)) {
                    available_stall_durations.add(scaler_formats[j]);
                    available_stall_durations.add(
                        stream_info[i].stream_sizes_tbl.width);
                    available_stall_durations.add(
                        stream_info[i].stream_sizes_tbl.height);
                    available_stall_durations.add(
                        stream_info[i].stream_stall_duration);
                } else {
                    available_stall_durations.add(scaler_formats[j]);
                    available_stall_durations.add(
                        stream_info[i].stream_sizes_tbl.width);
                    available_stall_durations.add(
                        stream_info[i].stream_sizes_tbl.height);
                    available_stall_durations.add(0);
                }
            }
        }
    }
    memcpy(s_setting[cameraId].scalerInfo.stall_durations,
           &(available_stall_durations[0]),
           available_stall_durations.size() * sizeof(int64_t));

    HAL_LOGI("id=%d format=%d", cameraId,
             s_setting[cameraId].scalerInfo.available_stream_configurations[0]);
    memcpy(s_setting[cameraId].scalerInfo.processed_min_durations,
           kavailable_min_durations, sizeof(kavailable_min_durations));
    s_setting[cameraId].scalerInfo.max_digital_zoom = MAX_DIGITAL_ZOOM_RATIO;
    memcpy(s_setting[cameraId].scalerInfo.jpeg_min_durations,
           camera3_default_info.common.FrameDurationRange,
           sizeof(camera3_default_info.common.FrameDurationRange));
    s_setting[cameraId].scalerInfo.cropping_type =
        ANDROID_SCALER_CROPPING_TYPE_FREEFORM;

    // sensor_info
    memcpy(s_setting[cameraId].sensor_InfoInfo.physical_size,
           camera3_default_info.common.sensor_physical_size,
           sizeof(camera3_default_info.common.sensor_physical_size));
    memcpy(s_setting[cameraId].sensor_InfoInfo.exposupre_time_range,
           camera3_default_info.common.exposure_time_range,
           sizeof(camera3_default_info.common.exposure_time_range));
    s_setting[cameraId].sensor_InfoInfo.max_frame_duration =
        camera3_default_info.common.frame_duration_range[1];
    s_setting[cameraId].sensor_InfoInfo.color_filter_arrangement =
        default_info->common.color_arrangement;

    s_setting[cameraId].sensor_InfoInfo.pixer_array_size[0] =
        largest_picture_size[cameraId].width;
    s_setting[cameraId].sensor_InfoInfo.pixer_array_size[1] =
        largest_picture_size[cameraId].height;
    s_setting[cameraId].sensor_InfoInfo.active_array_size[0] = 0;
    s_setting[cameraId].sensor_InfoInfo.active_array_size[1] = 0;
    s_setting[cameraId].sensor_InfoInfo.active_array_size[2] =
        largest_picture_size[cameraId].width;
    s_setting[cameraId].sensor_InfoInfo.active_array_size[3] =
        largest_picture_size[cameraId].height;

    s_setting[cameraId].sensor_InfoInfo.white_level =
        default_info->common.white_level;
    for (size_t i = 0; i < 4; i++)
        s_setting[cameraId].sensorInfo.black_level_pattern[i] =
            default_info->common.black_level;
    memcpy(s_setting[cameraId].sensor_InfoInfo.sensitivity_range,
           ksensitivity_range, sizeof(ksensitivity_range));
    s_setting[cameraId].sensor_InfoInfo.timestamp_source =
        ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE_UNKNOWN;

    // control
    if (cameraId == 0)
        memcpy(s_setting[cameraId].controlInfo.ae_available_fps_ranges,
               kavailable_fps_ranges_back, sizeof(kavailable_fps_ranges_back));
    else
        memcpy(s_setting[cameraId].controlInfo.ae_available_fps_ranges,
               kavailable_fps_ranges_front,
               sizeof(kavailable_fps_ranges_front));
    s_setting[cameraId].controlInfo.ae_compensation_step.numerator = 1;
    s_setting[cameraId].controlInfo.ae_compensation_step.denominator = 2;
    memcpy(s_setting[cameraId].controlInfo.available_video_stab_modes,
           camera3_default_info.common.availableVideoStabModes,
           sizeof(camera3_default_info.common.availableVideoStabModes));
    if (mSensorFocusEnable[cameraId]) {
        memcpy(s_setting[cameraId].controlInfo.max_regions, kmax_regions,
               sizeof(kmax_regions));
    } else {
        memcpy(s_setting[cameraId].controlInfo.max_regions, kmax_front_regions,
               sizeof(kmax_regions));
    }

    s_setting[cameraId].controlInfo.ae_compensation_range[0] =
        kexposureCompensationRange[0];
    s_setting[cameraId].controlInfo.ae_compensation_range[1] =
        kexposureCompensationRange[1];
    s_setting[cameraId].controlInfo.ae_compensation_step.numerator =
        kae_compensation_step.numerator;
    s_setting[cameraId].controlInfo.ae_compensation_step.denominator =
        kae_compensation_step.denominator;

    {
        // s_setting[cameraId].controlInfo.available_effects[0] =
        // ANDROID_CONTROL_EFFECT_MODE_OFF;
        memcpy(s_setting[cameraId].controlInfo.available_effects,
               camera3_default_info.common.availEffectModes,
               sizeof(camera3_default_info.common.availEffectModes));
    }
    {
        // s_setting[cameraId].controlInfo.available_scene_modes[0] =
        // ANDROID_CONTROL_SCENE_MODE_NIGHT;
        memcpy(s_setting[cameraId].controlInfo.available_scene_modes,
               camera3_default_info.common.availSceneModes,
               sizeof(camera3_default_info.common.availSceneModes));
    }
    memcpy(s_setting[cameraId].controlInfo.ae_available_abtibanding_modes,
           camera3_default_info.common.availAntibandingModes,
           sizeof(camera3_default_info.common.availAntibandingModes));
    memcpy(s_setting[cameraId].controlInfo.af_available_modes,
           camera3_default_info.common.availableAfModes,
           sizeof(camera3_default_info.common.availableAfModes));
    memcpy(s_setting[cameraId].controlInfo.awb_available_modes,
           camera3_default_info.common.avail_awb_modes,
           sizeof(camera3_default_info.common.avail_awb_modes));
    memcpy(s_setting[cameraId].controlInfo.am_available_modes,
           camera3_default_info.common.availableAmModes,
           sizeof(camera3_default_info.common.availableAmModes));
    if (cameraId <= 1)
        memcpy(s_setting[cameraId].sprddefInfo.availabe_slow_motion,
               camera3_default_info.common.availableSlowMotion,
               sizeof(camera3_default_info.common.availableSlowMotion));
    // quirks
    s_setting[cameraId].quirksInfo.use_parital_result = 1;

    // lens
    if (cameraInfo.facing == -1) {
        s_setting[cameraId].lensInfo.facing = 0xff;
    } else {
        std::map<int,int> map_tag = {{CAMERA_FACING_BACK,ANDROID_LENS_FACING_BACK},
                                      {CAMERA_FACING_FRONT,ANDROID_LENS_FACING_FRONT},
                                       {CAMERA_FACING_EXTERNAL,ANDROID_LENS_FACING_EXTERNAL}};
        s_setting[cameraId].lensInfo.facing = map_tag[cameraInfo.facing];
    }

    // lens shading
    s_setting[cameraId].shadingInfo.factor_count = SPRD_SHADING_FACTOR_NUM;
    for (i = 0; i < SPRD_SHADING_FACTOR_NUM; i++) {
        s_setting[cameraId].shadingInfo.gain_factor[i] = 1.0f;
    }

    // jpeg
    int32_t jpeg_stream_size;
    jpeg_stream_size =
        getJpegStreamSize(cameraId, largest_picture_size[cameraId].width,
                          largest_picture_size[cameraId].height);
    memcpy(s_setting[cameraId].jpgInfo.available_thumbnail_sizes,
           camera3_default_info.common.jpegThumbnailSizes,
           sizeof(camera3_default_info.common.jpegThumbnailSizes));
    s_setting[cameraId].jpgInfo.max_size = jpeg_stream_size;

    // flash
    s_setting[cameraId].flashInfo.firing_power = 10;

    // flash_info
    // s_setting[cameraId].flash_InfoInfo.available = (cameraId == 0) ? 1 : 0;
    if (cameraId == 0) {
        s_setting[cameraId].flash_InfoInfo.available = 1;
    } else if (cameraId == 1) {
        if (!strcmp(FRONT_CAMERA_FLASH_TYPE, "none") ||
            !strcmp(FRONT_CAMERA_FLASH_TYPE, "lcd"))
            s_setting[cameraId].flash_InfoInfo.available = 0;
        else
            s_setting[cameraId].flash_InfoInfo.available = 1;
    }

    if (s_setting[cameraId].flash_InfoInfo.available) {
        memcpy(s_setting[cameraId].controlInfo.ae_available_modes,
               camera3_default_info.common.availableAeModes,
               sizeof(camera3_default_info.common.availableAeModes));
    } else {
        for (i = ANDROID_CONTROL_AE_MODE_OFF;
             i < ANDROID_CONTROL_AE_MODE_ON_AUTO_FLASH; i++) {
            s_setting[cameraId].controlInfo.ae_available_modes[i] = i;
        }
    }

    // request
    memcpy(s_setting[cameraId].requestInfo.max_num_output_streams,
           camera3_default_info.common.max_output_streams,
           sizeof(camera3_default_info.common.max_output_streams));
    memcpy(s_setting[cameraId].requestInfo.available_characteristics_keys,
           kavailable_characteristics_keys,
           sizeof(kavailable_characteristics_keys));

    memcpy(s_setting[cameraId].requestInfo.available_request_keys,
           kavailable_request_keys, sizeof(kavailable_request_keys));
    if (mSensorFocusEnable[cameraId]) {
        int length = ARRAY_SIZE(kavailable_request_keys);
        int maxIndex =
            ARRAY_SIZE(s_setting[cameraId].requestInfo.available_request_keys);
        if (length < maxIndex) {
            s_setting[cameraId].requestInfo.available_request_keys[length] =
                ANDROID_CONTROL_AF_REGIONS;
        } else {
            HAL_LOGE("camera id %d available_request_keys out of size",
                     cameraId);
        }
    }

    memcpy(s_setting[cameraId].requestInfo.available_result_keys,
           kavailable_result_keys, sizeof(kavailable_result_keys));
    memcpy(s_setting[cameraId].requestInfo.available_capabilites,
           kavailable_capabilities, sizeof(kavailable_capabilities));
    s_setting[cameraId].requestInfo.partial_result_count = 1;
    s_setting[cameraId].requestInfo.pipeline_max_depth = 8;

    // noise
    memcpy(
        s_setting[cameraId].noiseInfo.reduction_available_noise_reduction_modes,
        kavailable_noise_reduction_modes,
        sizeof(kavailable_noise_reduction_modes));
    // Shading Mode(init static parameter)
    memcpy(s_setting[cameraId].shadingInfo.available_lens_shading_map_modes,
           kavailable_lens_shading_map_modes,
           sizeof(kavailable_lens_shading_map_modes));
    memcpy(s_setting[cameraId].shadingInfo.available_shading_modes,
           kavailable_shading_modes, sizeof(kavailable_shading_modes));

    // sync
    s_setting[cameraId].syncInfo.max_latency =
        4; // ANDROID_SYNC_MAX_LATENCY_UNKNOWN;

    // sprd
    memcpy(s_setting[cameraId].sprddefInfo.availabe_brightness,
           camera3_default_info.common.availableBrightNess,
           sizeof(camera3_default_info.common.availableBrightNess));
    memcpy(s_setting[cameraId].sprddefInfo.availabe_contrast, availableContrast,
           sizeof(availableContrast));
    memcpy(s_setting[cameraId].sprddefInfo.availabe_saturation,
           availableSaturation, sizeof(availableSaturation));
    memcpy(s_setting[cameraId].sprddefInfo.availabe_iso,
           camera3_default_info.common.availableIso,
           sizeof(camera3_default_info.common.availableIso));

    memcpy(s_setting[cameraId].statis_InfoInfo.available_face_detect_modes,
           camera3_default_info.common.availableFaceDetectModes,
           sizeof(camera3_default_info.common.availableFaceDetectModes));
    s_setting[cameraId].sprddefInfo.flash_mode_support = 1;
    s_setting[cameraId].sprddefInfo.prev_rec_size_diff_support = 0;
    s_setting[cameraId].sprddefInfo.availabe_auto_hdr =
        camera3_default_info.common.availableAutoHdr;
    s_setting[cameraId].sprddefInfo.availabe_auto_3dnr =
        camera3_default_info.common.availableAuto3Dnr;
    s_setting[cameraId].sprddefInfo.available_logo_watermark =
        camera3_default_info.common.availLogoWatermark;
    s_setting[cameraId].sprddefInfo.available_time_watermark =
        camera3_default_info.common.availTimeWatermark;

    s_setting[cameraId].sprddefInfo.rec_snap_support =
        ANDROID_SPRD_VIDEO_SNAPSHOT_SUPPORT_ON;
    s_setting[cameraId].sprddefInfo.availabe_smile_enable = 1;
#ifdef CONFIG_SPRD_FD_LIB_VERSION_2
    s_setting[cameraId].sprddefInfo.availabe_gender_race_age_enable = 7;
#else
    s_setting[cameraId].sprddefInfo.availabe_gender_race_age_enable = 0;
#endif
    s_setting[cameraId].sprddefInfo.availabe_antiband_auto_supported = 1;

#ifdef CONFIG_CAMERA_RT_REFOCUS
    s_setting[cameraId].sprddefInfo.is_support_refocus = 1;
#else
    s_setting[cameraId].sprddefInfo.is_support_refocus = 0;
#endif

    // max preview size supported for hardware limitation
    s_setting[cameraId].sprddefInfo.max_preview_size[0] =
        MAX_PREVIEW_SIZE_WIDTH;
    s_setting[cameraId].sprddefInfo.max_preview_size[1] =
        MAX_PREVIEW_SIZE_HEIGHT;

    // default 0, will be update
    s_setting[cameraId].sprddefInfo.is_takepicture_with_flash = 0;

    s_setting[cameraId].sprddefInfo.sprd_available_flash_level = 0;
#ifdef CONFIG_AVAILABLE_FLASH_LEVEL
    if (cameraId == 1) {
        s_setting[cameraId].sprddefInfo.sprd_available_flash_level =
            CONFIG_AVAILABLE_FLASH_LEVEL;
    }
#endif
    s_setting[cameraId].sprddefInfo.sprd_is_hdr_scene = 0;
    HAL_LOGI("cameraId:%d, availableSprdFlashLevel:%d, availableAutohdr %d ",
             cameraId,
             s_setting[cameraId].sprddefInfo.sprd_available_flash_level,
             s_setting[cameraId].sprddefInfo.availabe_auto_hdr);
    s_setting[cameraId].sprddefInfo.availabe_ai_scene =
        camera3_default_info.common.availableAiScene;
    s_setting[cameraId].sprddefInfo.sprd_ai_scene_type_current =
        HAL_AI_SCENE_DEFAULT;
    HAL_LOGI(
        "cameraId:%d, availabe_ai_scene:%d,  sprd_ai_scene_type_current:%d",
        cameraId, s_setting[cameraId].sprddefInfo.availabe_ai_scene,
        s_setting[cameraId].sprddefInfo.sprd_ai_scene_type_current);

    return ret;
}

int SprdCamera3Setting::initStaticMetadata(
    int32_t cameraId, camera_metadata_t **static_metadata) {
    int rc = 0;
    int array_size;
    int i = 0;
    CameraMetadata staticInfo;
    SprdCamera3DefaultInfo *default_info = &camera3_default_info;

#define FILL_CAM_INFO(Array, Start, Num, Flag)                                 \
    for (array_size = Start; array_size < Num; array_size++) {                 \
        if (Array[array_size] == 0)                                            \
            break;                                                             \
    }                                                                          \
    staticInfo.update(Flag, Array, array_size);

#define FILL_CAM_INFO_ARRAY(Array, Start, Num, Flag)                           \
    for (array_size = Start; array_size < Num; array_size++) {                 \
        if (Array[array_size * 4] == 0)                                        \
            break;                                                             \
    }                                                                          \
    staticInfo.update(Flag, Array, array_size * 4);

    /* android.info: hardware level */
    staticInfo.update(ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL,
                      &(s_setting[cameraId].supported_hardware_level), 1);

    /*COLOR*/
    FILL_CAM_INFO(s_setting[cameraId].colorInfo.available_aberration_modes, 1,
                  3, ANDROID_COLOR_CORRECTION_AVAILABLE_ABERRATION_MODES)

    /*EDGE*/
    staticInfo.update(
        ANDROID_EDGE_AVAILABLE_EDGE_MODES,
        s_setting[cameraId].edgeInfo.available_edge_modes,
        ARRAY_SIZE(s_setting[cameraId].edgeInfo.available_edge_modes));

    /* LENS_INFO*/
    staticInfo.update(ANDROID_LENS_INFO_MINIMUM_FOCUS_DISTANCE,
                      &(s_setting[cameraId].lens_InfoInfo.mini_focus_distance),
                      1);
    staticInfo.update(ANDROID_LENS_INFO_HYPERFOCAL_DISTANCE,
                      &(s_setting[cameraId].lens_InfoInfo.hyperfocal_distance),
                      1);
    staticInfo.update(
        ANDROID_LENS_INFO_AVAILABLE_FOCAL_LENGTHS,
        &(s_setting[cameraId].lens_InfoInfo.available_focal_lengths), 1);
    staticInfo.update(ANDROID_LENS_INFO_AVAILABLE_APERTURES,
                      kavailable_lens_info_aperture,
                      ARRAY_SIZE(kavailable_lens_info_aperture));
    staticInfo.update(ANDROID_LENS_INFO_AVAILABLE_FILTER_DENSITIES,
                      &(s_setting[cameraId].lens_InfoInfo.filter_density), 1);
    staticInfo.update(
        ANDROID_LENS_INFO_AVAILABLE_OPTICAL_STABILIZATION,
        &(s_setting[cameraId].lens_InfoInfo.optical_stabilization), 1);
    staticInfo.update(
        ANDROID_LENS_INFO_SHADING_MAP_SIZE,
        s_setting[cameraId].lens_InfoInfo.shading_map_size,
        ARRAY_SIZE(s_setting[cameraId].lens_InfoInfo.shading_map_size));
    staticInfo.update(
        ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION,
        &(s_setting[cameraId].lens_InfoInfo.focus_distance_calibration), 1);

    /*SENSOR_INFO*/
    staticInfo.update(
        ANDROID_SENSOR_INFO_PHYSICAL_SIZE,
        s_setting[cameraId].sensor_InfoInfo.physical_size,
        ARRAY_SIZE(s_setting[cameraId].sensor_InfoInfo.physical_size));
    staticInfo.update(
        ANDROID_SENSOR_INFO_EXPOSURE_TIME_RANGE,
        s_setting[cameraId].sensor_InfoInfo.exposupre_time_range,
        ARRAY_SIZE(s_setting[cameraId].sensor_InfoInfo.exposupre_time_range));
    staticInfo.update(ANDROID_SENSOR_INFO_MAX_FRAME_DURATION,
                      &(s_setting[cameraId].sensor_InfoInfo.max_frame_duration),
                      1);
    staticInfo.update(
        ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT,
        &(s_setting[cameraId].sensor_InfoInfo.color_filter_arrangement), 1);
    staticInfo.update(
        ANDROID_SENSOR_INFO_PIXEL_ARRAY_SIZE,
        s_setting[cameraId].sensor_InfoInfo.pixer_array_size,
        ARRAY_SIZE(s_setting[cameraId].sensor_InfoInfo.pixer_array_size));
    staticInfo.update(
        ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE,
        s_setting[cameraId].sensor_InfoInfo.active_array_size,
        ARRAY_SIZE(s_setting[cameraId].sensor_InfoInfo.active_array_size));
    staticInfo.update(ANDROID_SENSOR_INFO_WHITE_LEVEL,
                      &(s_setting[cameraId].sensor_InfoInfo.white_level), 1);
    staticInfo.update(
        ANDROID_SENSOR_INFO_SENSITIVITY_RANGE,
        s_setting[cameraId].sensor_InfoInfo.sensitivity_range,
        ARRAY_SIZE(s_setting[cameraId].sensor_InfoInfo.sensitivity_range));
    staticInfo.update(ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE,
                      &(s_setting[cameraId].sensor_InfoInfo.timestamp_source),
                      1);

    /*SENSOR*/
    staticInfo.update(ANDROID_SENSOR_BLACK_LEVEL_PATTERN,
                      s_setting[cameraId].sensorInfo.black_level_pattern, 4);
    staticInfo.update(ANDROID_SENSOR_ORIENTATION,
                      &(s_setting[cameraId].sensorInfo.orientation), 1);
    staticInfo.update(
        ANDROID_SENSOR_AVAILABLE_TEST_PATTERN_MODES,
        s_setting[cameraId].sensorInfo.available_test_pattern_modes,
        ARRAY_SIZE(
            s_setting[cameraId].sensorInfo.available_test_pattern_modes));
    staticInfo.update(ANDROID_SENSOR_MAX_ANALOG_SENSITIVITY,
                      &(s_setting[cameraId].sensorInfo.max_analog_sensitivity),
                      1);

    /*FLASH_INFO*/
    staticInfo.update(ANDROID_FLASH_INFO_CHARGE_DURATION,
                      &(s_setting[cameraId].flash_InfoInfo.charge_duration), 1);

    /*TONEMAP*/
    staticInfo.update(ANDROID_TONEMAP_MAX_CURVE_POINTS,
                      &(s_setting[cameraId].toneInfo.max_curve_points), 1);
    FILL_CAM_INFO(s_setting[cameraId].toneInfo.available_tone_map_modes, 1, 3,
                  ANDROID_TONEMAP_AVAILABLE_TONE_MAP_MODES)

    /*STATISTICS_INFO*/
    staticInfo.update(ANDROID_STATISTICS_INFO_MAX_FACE_COUNT,
                      &(s_setting[cameraId].statis_InfoInfo.max_face_count), 1);
    staticInfo.update(
        ANDROID_STATISTICS_INFO_HISTOGRAM_BUCKET_COUNT,
        &(s_setting[cameraId].statis_InfoInfo.histogram_bucket_count), 1);
    staticInfo.update(
        ANDROID_STATISTICS_INFO_MAX_HISTOGRAM_COUNT,
        &(s_setting[cameraId].statis_InfoInfo.max_histogram_count), 1);
    staticInfo.update(
        ANDROID_STATISTICS_INFO_SHARPNESS_MAP_SIZE,
        s_setting[cameraId].statis_InfoInfo.sharpness_map_size,
        ARRAY_SIZE(s_setting[cameraId].statis_InfoInfo.sharpness_map_size));
    staticInfo.update(
        ANDROID_STATISTICS_INFO_MAX_SHARPNESS_MAP_VALUE,
        &(s_setting[cameraId].statis_InfoInfo.max_sharpness_map_size), 1);
    staticInfo.update(
        ANDROID_STATISTICS_INFO_AVAILABLE_FACE_DETECT_MODES,
        s_setting[cameraId].statis_InfoInfo.available_face_detect_modes,
        ARRAY_SIZE(
            s_setting[cameraId].statis_InfoInfo.available_face_detect_modes));

    /*SCALER*/
    staticInfo.update(
        ANDROID_SCALER_AVAILABLE_RAW_MIN_DURATIONS,
        s_setting[cameraId].scalerInfo.raw_min_duration,
        ARRAY_SIZE(s_setting[cameraId].scalerInfo.raw_min_duration));
    FILL_CAM_INFO_ARRAY(
        s_setting[cameraId].scalerInfo.available_stream_configurations, 0,
        CAMERA_SETTINGS_CONFIG_ARRAYSIZE,
        ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS)
    HAL_LOGI("format=%d size=%d",
             s_setting[cameraId].scalerInfo.available_stream_configurations[0],
             array_size);
    staticInfo.update(
        ANDROID_SCALER_AVAILABLE_PROCESSED_MIN_DURATIONS,
        s_setting[cameraId].scalerInfo.processed_min_durations,
        ARRAY_SIZE(s_setting[cameraId].scalerInfo.processed_min_durations));
    staticInfo.update(ANDROID_SCALER_AVAILABLE_MAX_DIGITAL_ZOOM,
                      &(s_setting[cameraId].scalerInfo.max_digital_zoom), 1);
    staticInfo.update(
        ANDROID_SCALER_AVAILABLE_JPEG_MIN_DURATIONS,
        s_setting[cameraId].scalerInfo.jpeg_min_durations,
        ARRAY_SIZE(s_setting[cameraId].scalerInfo.jpeg_min_durations));
    FILL_CAM_INFO_ARRAY(s_setting[cameraId].scalerInfo.min_frame_durations, 0,
                        CAMERA_SETTINGS_CONFIG_ARRAYSIZE,
                        ANDROID_SCALER_AVAILABLE_MIN_FRAME_DURATIONS)
    FILL_CAM_INFO_ARRAY(s_setting[cameraId].scalerInfo.stall_durations, 0,
                        CAMERA_SETTINGS_CONFIG_ARRAYSIZE,
                        ANDROID_SCALER_AVAILABLE_STALL_DURATIONS)
    staticInfo.update(ANDROID_SCALER_CROPPING_TYPE,
                      &(s_setting[cameraId].scalerInfo.cropping_type), 1);

    /*CONTROL*/
    /*staticInfo.update(ANDROID_CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES,
                    s_setting[cameraId].controlInfo.ae_available_fps_ranges,
                    ARRAY_SIZE(s_setting[cameraId].controlInfo.ae_available_fps_ranges)
       );*/
    FILL_CAM_INFO(s_setting[cameraId].controlInfo.ae_available_fps_ranges, 2,
                  FPS_RANGE_COUNT,
                  ANDROID_CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES)
    staticInfo.update(ANDROID_CONTROL_AE_COMPENSATION_STEP,
                      &(s_setting[cameraId].controlInfo.ae_compensation_step),
                      1);
    staticInfo.update(
        ANDROID_CONTROL_AVAILABLE_VIDEO_STABILIZATION_MODES,
        s_setting[cameraId].controlInfo.available_video_stab_modes,
        ARRAY_SIZE(s_setting[cameraId].controlInfo.available_video_stab_modes));
    staticInfo.update(ANDROID_CONTROL_MAX_REGIONS,
                      s_setting[cameraId].controlInfo.max_regions,
                      ARRAY_SIZE(s_setting[cameraId].controlInfo.max_regions));
    staticInfo.update(
        ANDROID_CONTROL_AE_COMPENSATION_RANGE,
        s_setting[cameraId].controlInfo.ae_compensation_range,
        ARRAY_SIZE(s_setting[cameraId].controlInfo.ae_compensation_range));

    FILL_CAM_INFO(s_setting[cameraId].controlInfo.available_effects, 1, 9,
                  ANDROID_CONTROL_AVAILABLE_EFFECTS)
    FILL_CAM_INFO(s_setting[cameraId].controlInfo.available_scene_modes, 1, 18,
                  ANDROID_CONTROL_AVAILABLE_SCENE_MODES)
    FILL_CAM_INFO(
        s_setting[cameraId].controlInfo.ae_available_abtibanding_modes, 1, 4,
        ANDROID_CONTROL_AE_AVAILABLE_ANTIBANDING_MODES)
    FILL_CAM_INFO(s_setting[cameraId].controlInfo.af_available_modes, 1, 6,
                  ANDROID_CONTROL_AF_AVAILABLE_MODES)
    FILL_CAM_INFO(s_setting[cameraId].controlInfo.awb_available_modes, 1, 9,
                  ANDROID_CONTROL_AWB_AVAILABLE_MODES)
    FILL_CAM_INFO(s_setting[cameraId].controlInfo.ae_available_modes, 1, 5,
                  ANDROID_CONTROL_AE_AVAILABLE_MODES)

    FILL_CAM_INFO(s_setting[cameraId].controlInfo.am_available_modes, 1, 4,
                  ANDROID_SPRD_AVAILABLE_METERING_MODE)

    /*Quirk*/
    staticInfo.update(ANDROID_QUIRKS_USE_PARTIAL_RESULT,
                      &(s_setting[cameraId].quirksInfo.use_parital_result), 1);

    /*LENS*/
    staticInfo.update(ANDROID_LENS_FACING,
                      &(s_setting[cameraId].lensInfo.facing), 1);

    /*JPEG*/
    staticInfo.update(
        ANDROID_JPEG_AVAILABLE_THUMBNAIL_SIZES,
        s_setting[cameraId].jpgInfo.available_thumbnail_sizes,
        ARRAY_SIZE(s_setting[cameraId].jpgInfo.available_thumbnail_sizes));
    staticInfo.update(ANDROID_JPEG_MAX_SIZE,
                      &(s_setting[cameraId].jpgInfo.max_size), 1);

    /*FLASH*/
    staticInfo.update(ANDROID_FLASH_FIRING_POWER,
                      &(s_setting[cameraId].flashInfo.firing_power), 1);

    /*FLASH_INFO*/
    staticInfo.update(ANDROID_FLASH_INFO_AVAILABLE,
                      &(s_setting[cameraId].flash_InfoInfo.available), 1);

    /*REQUEST*/
    staticInfo.update(
        ANDROID_REQUEST_MAX_NUM_OUTPUT_STREAMS,
        s_setting[cameraId].requestInfo.max_num_output_streams,
        ARRAY_SIZE(s_setting[cameraId].requestInfo.max_num_output_streams));
    FILL_CAM_INFO(
        s_setting[cameraId].requestInfo.available_characteristics_keys, 0, 100,
        ANDROID_REQUEST_AVAILABLE_CHARACTERISTICS_KEYS)
    FILL_CAM_INFO(s_setting[cameraId].requestInfo.available_request_keys, 0, 50,
                  ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS)
    FILL_CAM_INFO(s_setting[cameraId].requestInfo.available_result_keys, 0, 50,
                  ANDROID_REQUEST_AVAILABLE_RESULT_KEYS)
    FILL_CAM_INFO(s_setting[cameraId].requestInfo.available_capabilites, 1, 5,
                  ANDROID_REQUEST_AVAILABLE_CAPABILITIES)
    staticInfo.update(ANDROID_REQUEST_PARTIAL_RESULT_COUNT,
                      &(s_setting[cameraId].requestInfo.partial_result_count),
                      1);
    staticInfo.update(ANDROID_REQUEST_PIPELINE_MAX_DEPTH,
                      &(s_setting[cameraId].requestInfo.pipeline_max_depth), 1);

    /*LENS SHADING*/
    staticInfo.update(ANDROID_STATISTICS_LENS_SHADING_CORRECTION_MAP,
                      &(s_setting[cameraId].shadingInfo.factor_count), 1);
    staticInfo.update(ANDROID_STATISTICS_LENS_SHADING_MAP,
                      s_setting[cameraId].shadingInfo.gain_factor,
                      SPRD_SHADING_FACTOR_NUM);

    /*NOISE*/
    staticInfo.update(
        ANDROID_NOISE_REDUCTION_AVAILABLE_NOISE_REDUCTION_MODES,
        s_setting[cameraId].noiseInfo.reduction_available_noise_reduction_modes,
        ARRAY_SIZE(s_setting[cameraId]
                       .noiseInfo.reduction_available_noise_reduction_modes));

    staticInfo.update(
        ANDROID_STATISTICS_INFO_AVAILABLE_LENS_SHADING_MAP_MODES,
        s_setting[cameraId].shadingInfo.available_lens_shading_map_modes,
        ARRAY_SIZE(
            s_setting[cameraId].shadingInfo.available_lens_shading_map_modes));
    staticInfo.update(
        ANDROID_SHADING_AVAILABLE_MODES,
        s_setting[cameraId].shadingInfo.available_shading_modes,
        ARRAY_SIZE(s_setting[cameraId].shadingInfo.available_shading_modes));
    /*SYNC*/
    staticInfo.update(ANDROID_SYNC_MAX_LATENCY,
                      &(s_setting[cameraId].syncInfo.max_latency), 1);

    /*SPRD*/
    staticInfo.update(
        ANDROID_SPRD_AVAILABLE_BRIGHTNESS,
        s_setting[cameraId].sprddefInfo.availabe_brightness,
        ARRAY_SIZE(s_setting[cameraId].sprddefInfo.availabe_brightness));
    staticInfo.update(
        ANDROID_SPRD_AVAILABLE_CONTRAST,
        s_setting[cameraId].sprddefInfo.availabe_contrast,
        ARRAY_SIZE(s_setting[cameraId].sprddefInfo.availabe_contrast));
    staticInfo.update(
        ANDROID_SPRD_AVAILABLE_SATURATION,
        s_setting[cameraId].sprddefInfo.availabe_saturation,
        ARRAY_SIZE(s_setting[cameraId].sprddefInfo.availabe_saturation));
    staticInfo.update(ANDROID_SPRD_AVAILABLE_SMILEENABLE,
                      &(s_setting[cameraId].sprddefInfo.availabe_smile_enable),
                      1);
    staticInfo.update(
        ANDROID_SPRD_AVAILABLE_FACEAGEENABLE,
        &(s_setting[cameraId].sprddefInfo.availabe_gender_race_age_enable), 1);
    staticInfo.update(
        ANDROID_SPRD_AVAILABLE_ANTIBAND_AUTOSUPPORTED,
        &(s_setting[cameraId].sprddefInfo.availabe_antiband_auto_supported), 1);
    if (cameraId == 0 || cameraId == 1 || cameraId == 2) {
        staticInfo.update(
            ANDROID_SPRD_AVAILABLE_ISO,
            s_setting[cameraId].sprddefInfo.availabe_iso,
            ARRAY_SIZE(s_setting[cameraId].sprddefInfo.availabe_iso));
    }
    staticInfo.update(ANDROID_SPRD_FLASH_MODE_SUPPORT,
                      &(s_setting[cameraId].sprddefInfo.flash_mode_support), 1);
    staticInfo.update(
        ANDROID_SPRD_PRV_REC_DIFFERENT_SIZE_SUPPORT,
        &(s_setting[cameraId].sprddefInfo.prev_rec_size_diff_support), 1);
    staticInfo.update(
        ANDROID_SPRD_AVAILABLE_SLOW_MOTION,
        s_setting[cameraId].sprddefInfo.availabe_slow_motion,
        ARRAY_SIZE(s_setting[cameraId].sprddefInfo.availabe_slow_motion));
    staticInfo.update(ANDROID_SPRD_VIDEO_SNAPSHOT_SUPPORT,
                      &(s_setting[cameraId].sprddefInfo.rec_snap_support), 1);
    staticInfo.update(ANDROID_SPRD_IS_SUPPORT_REFOCUS,
                      &(s_setting[cameraId].sprddefInfo.is_support_refocus), 1);
    staticInfo.update(
        ANDROID_SPRD_3DCALIBRATION_CAPTURE_SIZE,
        (int32_t *)s_setting[cameraId].sprddefInfo.sprd_3dcalibration_cap_size,
        ARRAY_SIZE(
            s_setting[cameraId].sprddefInfo.sprd_3dcalibration_cap_size));
    // max preview size supported for hardware limitation
    staticInfo.update(ANDROID_SPRD_MAX_PREVIEW_SIZE,
                      s_setting[cameraId].sprddefInfo.max_preview_size, 2);
    // takepicture with flash or not
    staticInfo.update(
        ANDROID_SPRD_IS_TAKEPICTURE_WITH_FLASH,
        &(s_setting[cameraId].sprddefInfo.is_takepicture_with_flash), 1);

    staticInfo.update(
        ANDROID_SPRD_AVAILABLE_FLASH_LEVEL,
        &(s_setting[cameraId].sprddefInfo.sprd_available_flash_level), 1);

    staticInfo.update(ANDROID_SPRD_IS_HDR_SCENE,
                      &(s_setting[cameraId].sprddefInfo.sprd_is_hdr_scene), 1);

    staticInfo.update(ANDROID_SPRD_AVAILABLE_AUTO_HDR,
                      &(s_setting[cameraId].sprddefInfo.availabe_auto_hdr), 1);

    staticInfo.update(
        ANDROID_SPRD_AVAILABLE_LOGOWATERMARK,
        &(s_setting[cameraId].sprddefInfo.available_logo_watermark), 1);
    staticInfo.update(
        ANDROID_SPRD_AVAILABLE_TIMEWATERMARK,
        &(s_setting[cameraId].sprddefInfo.available_time_watermark), 1);

    staticInfo.update(
        ANDROID_SPRD_AI_SCENE_TYPE_CURRENT,
        &(s_setting[cameraId].sprddefInfo.sprd_ai_scene_type_current), 1);

    staticInfo.update(ANDROID_SPRD_AVAILABLE_AUTO_3DNR,
                      &(s_setting[cameraId].sprddefInfo.availabe_auto_3dnr), 1);
    HAL_LOGI("%s availabe_auto_3dnr =%d,", __FUNCTION__,
             s_setting[cameraId].sprddefInfo.availabe_auto_3dnr);

    staticInfo.update(ANDROID_SPRD_AVAILABLE_AI_SCENE,
                      &(s_setting[cameraId].sprddefInfo.availabe_ai_scene), 1);
    // FILL_CAM_INFO_ARRAY(s_setting[cameraId].sprddefInfo.sprd_cam_feature_list,
    //                  0, CAMERA_SETTINGS_CONFIG_ARRAYSIZE,
    //                  ANDROID_SPRD_CAM_FEATURE_LIST)
    staticInfo.update(
        ANDROID_SPRD_CAM_FEATURE_LIST,
        s_setting[cameraId].sprddefInfo.sprd_cam_feature_list,
        s_setting[cameraId].sprddefInfo.sprd_cam_feature_list_size);
    staticInfo.update(
        ANDROID_DISTORTION_CORRECTION_AVAILABLE_MODES,
        s_setting[cameraId].lensInfo.distortion_correction_modes,
        ARRAY_SIZE(s_setting[cameraId].lensInfo.distortion_correction_modes));

    *static_metadata = staticInfo.release();
#undef FILL_CAM_INFO
#undef FILL_CAM_INFO_ARRAY
    return rc;
}

int SprdCamera3Setting::getStaticMetadata(int32_t cameraId,
                                          camera_metadata_t **static_metadata) {
    int ret = 0;

    if (NULL == mStaticMetadata[cameraId]) {
        initStaticParameters(cameraId);
        //for sprd camera features
        initCameraIpFeature(cameraId);
        initStaticMetadata(cameraId, &mStaticMetadata[cameraId]);
        mStaticInfo[cameraId] = mStaticMetadata[cameraId];
    }
    *static_metadata = mStaticMetadata[cameraId];

    return ret;
}

SprdCamera3Setting::SprdCamera3Setting(int cameraId) {
    mCameraId = cameraId;

    for (size_t i = 0; i < CAMERA3_TEMPLATE_COUNT; i++)
        mDefaultMetadata[i] = NULL;
}

SprdCamera3Setting::~SprdCamera3Setting() {
    // reset paramters/tag to default value
    initStaticParameters(mCameraId);

    for (size_t i = 0; i < CAMERA3_TEMPLATE_COUNT; i++) {
        if (mDefaultMetadata[i])
            free_camera_metadata(mDefaultMetadata[i]);
    }
    if (mStaticMetadata[mCameraId]) {
        free(mStaticMetadata[mCameraId]);
        mStaticMetadata[mCameraId] = NULL;
        HAL_LOGI("%s id=%d", __FUNCTION__, mCameraId);
    }
    HAL_LOGI("%s X", __FUNCTION__);
}

int SprdCamera3Setting::getDefaultParameters(SprdCameraParameters &params) {
    Mutex::Autolock l(&mLock);

    params = mDefaultParameters;
    return 0;
}

int SprdCamera3Setting::constructDefaultMetadata(int type,
                                                 camera_metadata_t **metadata) {
    size_t i = 0;
    if (mDefaultMetadata[type] != NULL) {
        return 0;
    }
    CameraMetadata requestInfo;
    CameraMetadata characteristicsInfo = mStaticInfo[mCameraId];

    if (type != CAMERA3_TEMPLATE_MANUAL) {
        uint8_t mode = ANDROID_CONTROL_MODE_AUTO;
        requestInfo.update(ANDROID_CONTROL_MODE, &mode, 1);
    }

    int32_t maxRegionsAe = 0;
    int32_t maxRegionsAwb = 0;
    int32_t maxRegionsAf = 0;
    if (characteristicsInfo.exists(ANDROID_CONTROL_MAX_REGIONS)) {
        maxRegionsAe =
            characteristicsInfo.find(ANDROID_CONTROL_MAX_REGIONS).data.i32[0];
        maxRegionsAwb =
            characteristicsInfo.find(ANDROID_CONTROL_MAX_REGIONS).data.i32[1];
        maxRegionsAf =
            characteristicsInfo.find(ANDROID_CONTROL_MAX_REGIONS).data.i32[2];
    }
    HAL_LOGD("maxRegionsAe = %d, maxRegionsAwb = %d, maxRegionsAf = %d",
             maxRegionsAe, maxRegionsAwb, maxRegionsAf);

    { // check af mode
        bool hasFocuser = false;
        if (characteristicsInfo.exists(
                ANDROID_LENS_INFO_MINIMUM_FOCUS_DISTANCE)) {
            float lensFocusDistance =
                characteristicsInfo.find(
                                       ANDROID_LENS_INFO_MINIMUM_FOCUS_DISTANCE)
                    .data.f[0];
            hasFocuser = (lensFocusDistance > 0);
        } else {
            hasFocuser = true;
        }

        if (hasFocuser == true) {
            uint8_t targetAfMode = ANDROID_CONTROL_AF_MODE_AUTO;
            if (type == CAMERA3_TEMPLATE_PREVIEW ||
                type == CAMERA3_TEMPLATE_STILL_CAPTURE ||
                type == CAMERA3_TEMPLATE_ZERO_SHUTTER_LAG) {
                if (characteristicsInfo.exists(
                        ANDROID_CONTROL_AF_AVAILABLE_MODES)) {
                    size_t count = characteristicsInfo
                                       .find(ANDROID_CONTROL_AF_AVAILABLE_MODES)
                                       .count;
                    for (i = 0; i < count; i++) {
                        if (characteristicsInfo
                                .find(ANDROID_CONTROL_AF_AVAILABLE_MODES)
                                .data.u8[i] ==
                            ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE) {
                            targetAfMode =
                                ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE;
                            break;
                        }
                    }
                }
            } else if (type == CAMERA3_TEMPLATE_VIDEO_RECORD ||
                       type == CAMERA3_TEMPLATE_VIDEO_SNAPSHOT) {
                if (characteristicsInfo.exists(
                        ANDROID_CONTROL_AF_AVAILABLE_MODES)) {
                    size_t count = characteristicsInfo
                                       .find(ANDROID_CONTROL_AF_AVAILABLE_MODES)
                                       .count;
                    for (i = 0; i < count; i++) {
                        if (characteristicsInfo
                                .find(ANDROID_CONTROL_AF_AVAILABLE_MODES)
                                .data.u8[i] ==
                            ANDROID_CONTROL_AF_MODE_CONTINUOUS_VIDEO) {
                            targetAfMode =
                                ANDROID_CONTROL_AF_MODE_CONTINUOUS_VIDEO;
                            break;
                        }
                    }
                }
            } else if (type == CAMERA3_TEMPLATE_MANUAL) {
                targetAfMode = ANDROID_CONTROL_AF_MODE_OFF;
            }
            requestInfo.update(ANDROID_CONTROL_AF_MODE, &targetAfMode, 1);

            if (characteristicsInfo.exists(
                    ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS)) {
                size_t count = characteristicsInfo
                                   .find(ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS)
                                   .count;
                for (i = 0; i < count; i++) {
                    if (characteristicsInfo
                            .find(ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS)
                            .data.i32[i] == ANDROID_LENS_FOCUS_DISTANCE) {
                        float lensFocusDistance = 0.0f;
                        requestInfo.update(ANDROID_LENS_FOCUS_DISTANCE,
                                           &lensFocusDistance, 1);
                        break;
                    }
                }
            }
        } else {
            uint8_t targetAfMode = ANDROID_CONTROL_AF_MODE_OFF;
            requestInfo.update(ANDROID_CONTROL_AF_MODE, &targetAfMode, 1);
        }
    }

#ifndef MINICAMERA
    { // check fps range
        if (characteristicsInfo.exists(
                ANDROID_CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES)) {
            size_t count =
                characteristicsInfo
                    .find(ANDROID_CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES)
                    .count;
            int32_t aeTargetFpsRange[2];

            if (count < 2) {
                HAL_LOGE(
                    "Unable to find valid fps range in camera characteristics");
                return -1;
            }
            aeTargetFpsRange[0] = 5;
            aeTargetFpsRange[1] = 30;
            for (i = 0; i < count; i += 2) {
                if (aeTargetFpsRange[0] ==
                        characteristicsInfo
                            .find(
                                ANDROID_CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES)
                            .data.i32[i] &&
                    aeTargetFpsRange[1] ==
                        characteristicsInfo
                            .find(
                                ANDROID_CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES)
                            .data.i32[i + 1])
                    break;
            }
            if (i >= count) {
                aeTargetFpsRange[0] =
                    characteristicsInfo
                        .find(ANDROID_CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES)
                        .data.i32[0];
                aeTargetFpsRange[1] =
                    characteristicsInfo
                        .find(ANDROID_CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES)
                        .data.i32[1];
            }

            if (type != CAMERA3_TEMPLATE_MANUAL &&
                type != CAMERA3_TEMPLATE_STILL_CAPTURE) {
                if (!(aeTargetFpsRange[0] == 5 && aeTargetFpsRange[1] == 30)) {
                    for (i = 0; i < count; i += 2) {
                        aeTargetFpsRange[0] =
                            characteristicsInfo
                                .find(
                                    ANDROID_CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES)
                                .data.i32[i];
                        aeTargetFpsRange[1] =
                            characteristicsInfo
                                .find(
                                    ANDROID_CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES)
                                .data.i32[i + 1];

                        if (aeTargetFpsRange[1] >= 20)
                            break;
                    }
                    if (i >= count) {
                        aeTargetFpsRange[0] = 5;
                        aeTargetFpsRange[1] = 30;
                    }
                }

                if (type == CAMERA3_TEMPLATE_VIDEO_RECORD &&
                    characteristicsInfo
                            .find(ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL)
                            .data.u8[0] !=
                        ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LEGACY) {
                    for (i = 0; i < count; i += 2) {
                        aeTargetFpsRange[0] =
                            characteristicsInfo
                                .find(
                                    ANDROID_CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES)
                                .data.i32[i];
                        aeTargetFpsRange[1] =
                            characteristicsInfo
                                .find(
                                    ANDROID_CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES)
                                .data.i32[i + 1];

                        if (aeTargetFpsRange[1] >= 20 &&
                            aeTargetFpsRange[0] == aeTargetFpsRange[1])
                            break;
                    }
                    if (i >= count) {
                        aeTargetFpsRange[0] = 30;
                        aeTargetFpsRange[1] = 30;
                    }
                }
            }

            requestInfo.update(ANDROID_CONTROL_AE_TARGET_FPS_RANGE,
                               aeTargetFpsRange, ARRAY_SIZE(aeTargetFpsRange));
        } else {
            HAL_LOGE(
                "Unable to find vailable fps range in camera characteristics");
            return -1;
        }
    }

    { // check anti banding mode
        if (type != CAMERA3_TEMPLATE_MANUAL) {
            if (characteristicsInfo.exists(
                    ANDROID_CONTROL_AE_AVAILABLE_ANTIBANDING_MODES)) {
                size_t count =
                    characteristicsInfo
                        .find(ANDROID_CONTROL_AE_AVAILABLE_ANTIBANDING_MODES)
                        .count;
                uint8_t aeAntiMode = ANDROID_CONTROL_AE_ANTIBANDING_MODE_OFF;

                for (i = 0; i < count; i++) {
                    uint8_t aeAvailableAntiMode =
                        characteristicsInfo
                            .find(
                                ANDROID_CONTROL_AE_AVAILABLE_ANTIBANDING_MODES)
                            .data.u8[i];
                    if (aeAvailableAntiMode ==
                        ANDROID_CONTROL_AE_ANTIBANDING_MODE_AUTO) {
                        aeAntiMode = ANDROID_CONTROL_AE_ANTIBANDING_MODE_AUTO;
                    } else if (aeAvailableAntiMode ==
                                   ANDROID_CONTROL_AE_ANTIBANDING_MODE_50HZ &&
                               aeAntiMode !=
                                   ANDROID_CONTROL_AE_ANTIBANDING_MODE_AUTO) {
                        aeAntiMode = ANDROID_CONTROL_AE_ANTIBANDING_MODE_50HZ;
                    } else if (aeAvailableAntiMode ==
                                   ANDROID_CONTROL_AE_ANTIBANDING_MODE_60HZ &&
                               aeAntiMode !=
                                   ANDROID_CONTROL_AE_ANTIBANDING_MODE_AUTO &&
                               aeAntiMode !=
                                   ANDROID_CONTROL_AE_ANTIBANDING_MODE_50HZ) {
                        aeAntiMode = ANDROID_CONTROL_AE_ANTIBANDING_MODE_60HZ;
                    }
                }

                requestInfo.update(ANDROID_CONTROL_AE_ANTIBANDING_MODE,
                                   &aeAntiMode, 1);
            } else {
                HAL_LOGE("Unable to find vailable antibanding mode in camera "
                         "characteristics");
                return -1;
            }
        }
    }
#endif

    if (type == CAMERA3_TEMPLATE_MANUAL) {
        uint8_t mode = ANDROID_CONTROL_MODE_OFF;
        requestInfo.update(ANDROID_CONTROL_MODE, &mode, 1);

        uint8_t aeMode = ANDROID_CONTROL_AE_MODE_OFF;
        requestInfo.update(ANDROID_CONTROL_AE_MODE, &aeMode, 1);

        uint8_t awbMode = ANDROID_CONTROL_AWB_MODE_OFF;
        requestInfo.update(ANDROID_CONTROL_AWB_MODE, &awbMode, 1);
    } else {
        uint8_t aeMode = ANDROID_CONTROL_AE_MODE_ON;
        requestInfo.update(ANDROID_CONTROL_AE_MODE, &aeMode, 1);

        int32_t aeExposureCom = 0;
        requestInfo.update(ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION,
                           &aeExposureCom, 1);

        uint8_t aeLock = false;
        requestInfo.update(ANDROID_CONTROL_AE_LOCK, &aeLock, 1);

        uint8_t aePreTrigger = ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER_IDLE;
        requestInfo.update(ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER, &aePreTrigger,
                           1);

        uint8_t afTrigger = ANDROID_CONTROL_AF_TRIGGER_IDLE;
        requestInfo.update(ANDROID_CONTROL_AF_TRIGGER, &afTrigger, 1);

        uint8_t awbMode = ANDROID_CONTROL_AWB_MODE_AUTO;
        requestInfo.update(ANDROID_CONTROL_AWB_MODE, &awbMode, 1);

        uint8_t awbLock = false;
        requestInfo.update(ANDROID_CONTROL_AWB_LOCK, &awbLock, 1);

        if (maxRegionsAe > 0) {
            int32_t ae_regions[5] = {0, 0, 0, 0, 0};
            requestInfo.update(ANDROID_CONTROL_AE_REGIONS, ae_regions,
                               ARRAY_SIZE(ae_regions));
        }

        if (maxRegionsAwb > 0) {
            int32_t awb_regions[5] = {0, 0, 0, 0, 0};
            requestInfo.update(ANDROID_CONTROL_AWB_REGIONS, awb_regions,
                               ARRAY_SIZE(awb_regions));
        }

        if (maxRegionsAf > 0) {
            int32_t af_regions[5] = {0, 0, 0, 0, 0};
            requestInfo.update(ANDROID_CONTROL_AF_REGIONS, af_regions,
                               ARRAY_SIZE(af_regions));
        }
    }

    // sensor setting
    {
        bool lens_aperture_request = false;
        bool lens_aperture_capabilities = false;
        if (characteristicsInfo.exists(
                ANDROID_REQUEST_AVAILABLE_CHARACTERISTICS_KEYS)) {
            size_t count =
                characteristicsInfo
                    .find(ANDROID_REQUEST_AVAILABLE_CHARACTERISTICS_KEYS)
                    .count;
            for (i = 0; i < count; i++) {
                if (characteristicsInfo
                        .find(ANDROID_REQUEST_AVAILABLE_CHARACTERISTICS_KEYS)
                        .data.i32[i] == ANDROID_LENS_INFO_AVAILABLE_APERTURES) {
                    lens_aperture_capabilities = true;
                    break;
                }
            }
        }
        if (characteristicsInfo.exists(
                ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS)) {
            size_t count =
                characteristicsInfo.find(ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS)
                    .count;
            for (i = 0; i < count; i++) {
                if (characteristicsInfo
                        .find(ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS)
                        .data.i32[i] == ANDROID_LENS_APERTURE) {
                    lens_aperture_request = true;
                    break;
                }
            }
        }

        if (lens_aperture_request != lens_aperture_capabilities) {
            HAL_LOGE("Lens aperture must be present in request if available "
                     "apretures are present in metadata");
            return -1;
        }

        if (lens_aperture_capabilities == true) {
            float lensAperture = 0.0f;
            size_t count =
                characteristicsInfo.find(ANDROID_LENS_INFO_AVAILABLE_APERTURES)
                    .count;
            // if(count > 1) {
            lensAperture =
                characteristicsInfo.find(ANDROID_LENS_INFO_AVAILABLE_APERTURES)
                    .data.f[0];
            requestInfo.update(ANDROID_LENS_APERTURE, &lensAperture, 1);
            //}
        }
    }

    {
        bool lens_filter_request = false;
        bool lens_filter_capabilities = false;
        if (characteristicsInfo.exists(
                ANDROID_REQUEST_AVAILABLE_CHARACTERISTICS_KEYS)) {
            size_t count =
                characteristicsInfo
                    .find(ANDROID_REQUEST_AVAILABLE_CHARACTERISTICS_KEYS)
                    .count;
            for (i = 0; i < count; i++) {
                if (characteristicsInfo
                        .find(ANDROID_REQUEST_AVAILABLE_CHARACTERISTICS_KEYS)
                        .data.i32[i] ==
                    ANDROID_LENS_INFO_AVAILABLE_FILTER_DENSITIES) {
                    lens_filter_capabilities = true;
                    break;
                }
            }
        }
        if (characteristicsInfo.exists(
                ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS)) {
            size_t count =
                characteristicsInfo.find(ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS)
                    .count;
            for (i = 0; i < count; i++) {
                if (characteristicsInfo
                        .find(ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS)
                        .data.i32[i] == ANDROID_LENS_FILTER_DENSITY) {
                    lens_filter_request = true;
                    break;
                }
            }
        }
        if (lens_filter_request != lens_filter_capabilities) {
            HAL_LOGE("Lens filter density must be present in request if "
                     "available filter densities are present in metadata");
            return -1;
        }
        if (lens_filter_capabilities == true) {
            float lensFilterDensities = 0.0f;
            size_t count =
                characteristicsInfo
                    .find(ANDROID_LENS_INFO_AVAILABLE_FILTER_DENSITIES)
                    .count;
            // if(count > 1) {
            lensFilterDensities =
                characteristicsInfo
                    .find(ANDROID_LENS_INFO_AVAILABLE_FILTER_DENSITIES)
                    .data.f[0];
            requestInfo.update(ANDROID_LENS_FILTER_DENSITY,
                               &lensFilterDensities, 1);
            //}
        }
    }

    {
        float lensFocalLen = 0.0f;
        if (characteristicsInfo.exists(
                ANDROID_LENS_INFO_AVAILABLE_FOCAL_LENGTHS)) {
            size_t count = characteristicsInfo
                               .find(ANDROID_LENS_INFO_AVAILABLE_FOCAL_LENGTHS)
                               .count;
            // if(count > 1) {
            lensFocalLen = characteristicsInfo
                               .find(ANDROID_LENS_INFO_AVAILABLE_FOCAL_LENGTHS)
                               .data.f[0];
            requestInfo.update(ANDROID_LENS_FOCAL_LENGTH, &lensFocalLen, 1);
            //}
        }
    }

    {
        bool lens_optical_request = false;
        bool lens_optical_capabilities = false;
        if (characteristicsInfo.exists(
                ANDROID_REQUEST_AVAILABLE_CHARACTERISTICS_KEYS)) {
            size_t count =
                characteristicsInfo
                    .find(ANDROID_REQUEST_AVAILABLE_CHARACTERISTICS_KEYS)
                    .count;
            for (i = 0; i < count; i++) {
                if (characteristicsInfo
                        .find(ANDROID_REQUEST_AVAILABLE_CHARACTERISTICS_KEYS)
                        .data.i32[i] ==
                    ANDROID_LENS_INFO_AVAILABLE_OPTICAL_STABILIZATION) {
                    lens_optical_capabilities = true;
                    break;
                }
            }
        }
        if (characteristicsInfo.exists(
                ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS)) {
            size_t count =
                characteristicsInfo.find(ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS)
                    .count;
            for (i = 0; i < count; i++) {
                if (characteristicsInfo
                        .find(ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS)
                        .data.i32[i] ==
                    ANDROID_LENS_OPTICAL_STABILIZATION_MODE) {
                    lens_optical_request = true;
                    break;
                }
            }
        }
        if (lens_optical_request != lens_optical_capabilities) {
            HAL_LOGE("Lens optical stabilization must be present in request if "
                     "available optical stabilization are present in metadata");
            return -1;
        }
        if (lens_optical_capabilities == true) {
            uint8_t lensOptStab = ANDROID_LENS_OPTICAL_STABILIZATION_MODE_OFF;
            size_t count =
                characteristicsInfo
                    .find(ANDROID_LENS_INFO_AVAILABLE_OPTICAL_STABILIZATION)
                    .count;
            // if(count > 1) {
            lensOptStab =
                characteristicsInfo
                    .find(ANDROID_LENS_INFO_AVAILABLE_OPTICAL_STABILIZATION)
                    .data.u8[0];
            requestInfo.update(ANDROID_LENS_OPTICAL_STABILIZATION_MODE,
                               &lensOptStab, 1);
            //}
        }
    }

    if (characteristicsInfo.exists(ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS)) {
        size_t count =
            characteristicsInfo.find(ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS)
                .count;
        for (i = 0; i < count; i++) {
            if (characteristicsInfo.find(ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS)
                    .data.i32[i] == ANDROID_BLACK_LEVEL_LOCK) {
                uint8_t blackLevelLock = ANDROID_BLACK_LEVEL_LOCK_OFF;
                requestInfo.update(ANDROID_BLACK_LEVEL_LOCK, &blackLevelLock,
                                   1);
            } else if (characteristicsInfo
                           .find(ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS)
                           .data.i32[i] == ANDROID_SENSOR_FRAME_DURATION) {
                int64_t sensorframeDura = NSEC_PER_33MSEC;
                requestInfo.update(ANDROID_SENSOR_FRAME_DURATION,
                                   &sensorframeDura, 1);
            } else if (characteristicsInfo
                           .find(ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS)
                           .data.i32[i] == ANDROID_SENSOR_EXPOSURE_TIME) {
                int64_t sensorExpTime = 10 * MSEC;
                requestInfo.update(ANDROID_SENSOR_EXPOSURE_TIME, &sensorExpTime,
                                   1);
            } else if (characteristicsInfo
                           .find(ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS)
                           .data.i32[i] == ANDROID_SENSOR_SENSITIVITY) {
                int32_t sensorSen = 100;
                requestInfo.update(ANDROID_SENSOR_SENSITIVITY, &sensorSen, 1);
            }
        }
    }

    uint8_t fd_mode = ANDROID_STATISTICS_FACE_DETECT_MODE_OFF;
    requestInfo.update(ANDROID_STATISTICS_FACE_DETECT_MODE, &fd_mode, 1);

    uint8_t flash_mode = ANDROID_FLASH_MODE_OFF;
    requestInfo.update(ANDROID_FLASH_MODE, &flash_mode, 1);

    {
        /*Shading Mode(construct default Metadata)*/
        bool lens_shading_map_mode_request = false;
        bool lens_shading_map_mode_capabilities = false;

        if (characteristicsInfo.exists(
                ANDROID_REQUEST_AVAILABLE_CHARACTERISTICS_KEYS)) {
            size_t count =
                characteristicsInfo
                    .find(ANDROID_REQUEST_AVAILABLE_CHARACTERISTICS_KEYS)
                    .count;
            for (i = 0; i < count; i++) {
                if (characteristicsInfo
                        .find(ANDROID_REQUEST_AVAILABLE_CHARACTERISTICS_KEYS)
                        .data.i32[i] ==
                    ANDROID_STATISTICS_INFO_AVAILABLE_LENS_SHADING_MAP_MODES) {
                    lens_shading_map_mode_capabilities = true;
                    break;
                }
            }
        }

        if (characteristicsInfo.exists(
                ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS)) {
            size_t count =
                characteristicsInfo.find(ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS)
                    .count;
            for (i = 0; i < count; i++) {
                if (characteristicsInfo
                        .find(ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS)
                        .data.i32[i] ==
                    ANDROID_STATISTICS_LENS_SHADING_MAP_MODE) {
                    lens_shading_map_mode_request = true;
                    break;
                }
            }
        }

        if (lens_shading_map_mode_capabilities &&
            lens_shading_map_mode_request) {
            uint8_t lenShadingMapMode =
                ANDROID_STATISTICS_LENS_SHADING_MAP_MODE_OFF;
            requestInfo.update(ANDROID_STATISTICS_LENS_SHADING_MAP_MODE,
                               &lenShadingMapMode, 1);
        } else {
            HAL_LOGE("lens shading map must be present in request if "
                     "available lens shading map are present in metadata");
            return -1;
        }

        bool shading_mode_request = false;
        bool shading_mode_capabilities = false;
        if (characteristicsInfo.exists(
                ANDROID_REQUEST_AVAILABLE_CHARACTERISTICS_KEYS)) {
            size_t count =
                characteristicsInfo
                    .find(ANDROID_REQUEST_AVAILABLE_CHARACTERISTICS_KEYS)
                    .count;
            for (i = 0; i < count; i++) {
                if (characteristicsInfo
                        .find(ANDROID_REQUEST_AVAILABLE_CHARACTERISTICS_KEYS)
                        .data.i32[i] == ANDROID_SHADING_AVAILABLE_MODES) {
                    shading_mode_capabilities = true;
                    break;
                }
            }
        }

        if (characteristicsInfo.exists(
                ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS)) {
            size_t count =
                characteristicsInfo.find(ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS)
                    .count;
            for (i = 0; i < count; i++) {
                if (characteristicsInfo
                        .find(ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS)
                        .data.i32[i] == ANDROID_SHADING_MODE) {
                    shading_mode_request = true;
                    break;
                }
            }
        }

        if (!shading_mode_capabilities || !shading_mode_request) {
            HAL_LOGE("shading mode must be present in request if "
                     "available shading mode are present in metadata");
            return -1;
        }
        /*Shading Mode(construct default Metadata)*/
    }

    if (type == CAMERA3_TEMPLATE_STILL_CAPTURE) {
        if (characteristicsInfo.exists(
                ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS)) {
            size_t count =
                characteristicsInfo.find(ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS)
                    .count;
            for (i = 0; i < count; i++) {
                if (characteristicsInfo
                        .find(ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS)
                        .data.i32[i] == ANDROID_COLOR_CORRECTION_MODE) {
                    uint8_t colorCorrectMode =
                        ANDROID_COLOR_CORRECTION_MODE_TRANSFORM_MATRIX;
                    requestInfo.update(ANDROID_COLOR_CORRECTION_MODE,
                                       &colorCorrectMode, 1);
                    break;
                }
            }
        }

        {
            bool edge_mode_request = false;
            bool edge_mode_capabilities = false;
            if (characteristicsInfo.exists(
                    ANDROID_REQUEST_AVAILABLE_CHARACTERISTICS_KEYS)) {
                size_t count =
                    characteristicsInfo
                        .find(ANDROID_REQUEST_AVAILABLE_CHARACTERISTICS_KEYS)
                        .count;
                for (i = 0; i < count; i++) {
                    if (characteristicsInfo
                            .find(
                                ANDROID_REQUEST_AVAILABLE_CHARACTERISTICS_KEYS)
                            .data.i32[i] == ANDROID_EDGE_AVAILABLE_EDGE_MODES) {
                        edge_mode_capabilities = true;
                        break;
                    }
                }
            }
            if (characteristicsInfo.exists(
                    ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS)) {
                size_t count = characteristicsInfo
                                   .find(ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS)
                                   .count;
                for (i = 0; i < count; i++) {
                    if (characteristicsInfo
                            .find(ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS)
                            .data.i32[i] == ANDROID_EDGE_MODE) {
                        edge_mode_request = true;
                        break;
                    }
                }
            }
            if (edge_mode_request != edge_mode_capabilities) {
                HAL_LOGE("edge mode must be present in request if available "
                         "edge mode are present in metadata");
                return -1;
            }
            if (edge_mode_request == true) {
                uint8_t edgeMode = ANDROID_EDGE_MODE_OFF;
                size_t count =
                    characteristicsInfo.find(ANDROID_EDGE_AVAILABLE_EDGE_MODES)
                        .count;
                for (i = 0; i < count; i++) {
                    uint8_t availableEdgeMode =
                        characteristicsInfo
                            .find(ANDROID_EDGE_AVAILABLE_EDGE_MODES)
                            .data.u8[i];
                    if (availableEdgeMode == ANDROID_EDGE_MODE_HIGH_QUALITY) {
                        edgeMode = ANDROID_EDGE_MODE_HIGH_QUALITY;
                    } else if (edgeMode != ANDROID_EDGE_MODE_HIGH_QUALITY &&
                               availableEdgeMode == ANDROID_EDGE_MODE_FAST) {
                        edgeMode = ANDROID_EDGE_MODE_FAST;
                    } else if (edgeMode != ANDROID_EDGE_MODE_HIGH_QUALITY &&
                               edgeMode != ANDROID_EDGE_MODE_FAST &&
                               availableEdgeMode == ANDROID_EDGE_MODE_OFF) {
                        edgeMode = ANDROID_EDGE_MODE_OFF;
                    }
                }
                requestInfo.update(ANDROID_EDGE_MODE, &edgeMode, 1);
            }
        }

        {
            bool color_correction_aberration_mode_request = false;
            bool color_correction_aberration_mode_capabilities = false;
            if (characteristicsInfo.exists(
                    ANDROID_REQUEST_AVAILABLE_CHARACTERISTICS_KEYS)) {
                size_t count =
                    characteristicsInfo
                        .find(ANDROID_REQUEST_AVAILABLE_CHARACTERISTICS_KEYS)
                        .count;
                for (i = 0; i < count; i++) {
                    if (characteristicsInfo
                            .find(
                                ANDROID_REQUEST_AVAILABLE_CHARACTERISTICS_KEYS)
                            .data.i32[i] ==
                        ANDROID_COLOR_CORRECTION_AVAILABLE_ABERRATION_MODES) {
                        color_correction_aberration_mode_capabilities = true;
                        break;
                    }
                }
            }
            if (characteristicsInfo.exists(
                    ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS)) {
                size_t count = characteristicsInfo
                                   .find(ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS)
                                   .count;
                for (i = 0; i < count; i++) {
                    if (characteristicsInfo
                            .find(ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS)
                            .data.i32[i] ==
                        ANDROID_COLOR_CORRECTION_ABERRATION_MODE) {
                        color_correction_aberration_mode_request = true;
                        break;
                    }
                }
            }
            if (color_correction_aberration_mode_capabilities !=
                color_correction_aberration_mode_request) {
                HAL_LOGE("color correction aberration mode must be present in "
                         "request if available color correction aberration "
                         "mode are present in metadata");
                return -1;
            }
            if (color_correction_aberration_mode_request == true) {
                uint8_t colorCorrectionAberrationMode =
                    ANDROID_COLOR_CORRECTION_ABERRATION_MODE_OFF;
                size_t count =
                    characteristicsInfo
                        .find(
                            ANDROID_COLOR_CORRECTION_AVAILABLE_ABERRATION_MODES)
                        .count;
                for (i = 0; i < count; i++) {
                    uint8_t availableColorCorrectionAberrationMode =
                        characteristicsInfo
                            .find(
                                ANDROID_COLOR_CORRECTION_AVAILABLE_ABERRATION_MODES)
                            .data.u8[i];
                    if (availableColorCorrectionAberrationMode ==
                        ANDROID_COLOR_CORRECTION_ABERRATION_MODE_HIGH_QUALITY) {
                        colorCorrectionAberrationMode =
                            ANDROID_COLOR_CORRECTION_ABERRATION_MODE_HIGH_QUALITY;
                        break;
                    }
                }
                requestInfo.update(ANDROID_COLOR_CORRECTION_ABERRATION_MODE,
                                   &colorCorrectionAberrationMode, 1);
            }
        }

        {
            bool noise_reduction_request = false;
            bool noise_reduction_capabilities = false;
            if (characteristicsInfo.exists(
                    ANDROID_REQUEST_AVAILABLE_CHARACTERISTICS_KEYS)) {
                size_t count =
                    characteristicsInfo
                        .find(ANDROID_REQUEST_AVAILABLE_CHARACTERISTICS_KEYS)
                        .count;
                for (i = 0; i < count; i++) {
                    if (characteristicsInfo
                            .find(
                                ANDROID_REQUEST_AVAILABLE_CHARACTERISTICS_KEYS)
                            .data.i32[i] ==
                        ANDROID_NOISE_REDUCTION_AVAILABLE_NOISE_REDUCTION_MODES) {
                        noise_reduction_capabilities = true;
                        break;
                    }
                }
            }
            if (characteristicsInfo.exists(
                    ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS)) {
                size_t count = characteristicsInfo
                                   .find(ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS)
                                   .count;
                for (i = 0; i < count; i++) {
                    if (characteristicsInfo
                            .find(ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS)
                            .data.i32[i] == ANDROID_NOISE_REDUCTION_MODE) {
                        noise_reduction_request = true;
                        break;
                    }
                }
            }
            if (noise_reduction_request != noise_reduction_capabilities) {
                HAL_LOGE("noise reduction must be present in request if "
                         "available noise reduction are present in metadata");
                return -1;
            }
            if (noise_reduction_capabilities == true) {
                uint8_t noiseReductionMode = ANDROID_NOISE_REDUCTION_MODE_OFF;
                size_t count =
                    characteristicsInfo
                        .find(
                            ANDROID_NOISE_REDUCTION_AVAILABLE_NOISE_REDUCTION_MODES)
                        .count;
                for (i = 0; i < count; i++) {
                    uint8_t availableNoiseReductionMode =
                        characteristicsInfo
                            .find(
                                ANDROID_NOISE_REDUCTION_AVAILABLE_NOISE_REDUCTION_MODES)
                            .data.u8[i];
                    if (availableNoiseReductionMode ==
                        ANDROID_NOISE_REDUCTION_MODE_HIGH_QUALITY) {
                        noiseReductionMode =
                            ANDROID_NOISE_REDUCTION_MODE_HIGH_QUALITY;
                    } else if (noiseReductionMode !=
                                   ANDROID_NOISE_REDUCTION_MODE_HIGH_QUALITY &&
                               availableNoiseReductionMode ==
                                   ANDROID_NOISE_REDUCTION_MODE_FAST) {
                        noiseReductionMode = ANDROID_NOISE_REDUCTION_MODE_FAST;
                    } else if (noiseReductionMode !=
                                   ANDROID_NOISE_REDUCTION_MODE_HIGH_QUALITY &&
                               noiseReductionMode !=
                                   ANDROID_NOISE_REDUCTION_MODE_FAST &&
                               availableNoiseReductionMode ==
                                   ANDROID_NOISE_REDUCTION_MODE_OFF) {
                        noiseReductionMode = ANDROID_EDGE_MODE_OFF;
                    }
                }
                requestInfo.update(ANDROID_NOISE_REDUCTION_MODE,
                                   &noiseReductionMode, 1);
            }
        }

        {
            bool tone_map_request = false;
            bool tone_map_capabilities = false;
            if (characteristicsInfo.exists(
                    ANDROID_REQUEST_AVAILABLE_CHARACTERISTICS_KEYS)) {
                size_t count =
                    characteristicsInfo
                        .find(ANDROID_REQUEST_AVAILABLE_CHARACTERISTICS_KEYS)
                        .count;
                for (i = 0; i < count; i++) {
                    if (characteristicsInfo
                            .find(
                                ANDROID_REQUEST_AVAILABLE_CHARACTERISTICS_KEYS)
                            .data.i32[i] ==
                        ANDROID_TONEMAP_AVAILABLE_TONE_MAP_MODES) {
                        tone_map_capabilities = true;
                        break;
                    }
                }
            }
            if (characteristicsInfo.exists(
                    ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS)) {
                size_t count = characteristicsInfo
                                   .find(ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS)
                                   .count;
                for (i = 0; i < count; i++) {
                    if (characteristicsInfo
                            .find(ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS)
                            .data.i32[i] == ANDROID_TONEMAP_MODE) {
                        tone_map_request = true;
                        break;
                    }
                }
            }

            if (tone_map_request != tone_map_capabilities) {
                HAL_LOGE("Tonemap mode must be present in request if available "
                         "tonemap mode are present in metadata");
                return -1;
            }
            if (tone_map_capabilities == true) {
                uint8_t toneMapMode = ANDROID_TONEMAP_MODE_FAST;
                size_t count =
                    characteristicsInfo
                        .find(ANDROID_TONEMAP_AVAILABLE_TONE_MAP_MODES)
                        .count;
                for (i = 0; i < count; i++) {
                    uint8_t availableToneMapMode =
                        characteristicsInfo
                            .find(ANDROID_TONEMAP_AVAILABLE_TONE_MAP_MODES)
                            .data.u8[i];
                    if (availableToneMapMode ==
                        ANDROID_TONEMAP_MODE_HIGH_QUALITY) {
                        toneMapMode = ANDROID_TONEMAP_MODE_HIGH_QUALITY;
                        break;
                    } else {
                        toneMapMode = ANDROID_TONEMAP_MODE_FAST;
                    }
                }
                requestInfo.update(ANDROID_TONEMAP_MODE, &toneMapMode, 1);
            }
        }

        {
            bool support_cap_raw = false;
            if (characteristicsInfo.exists(
                    ANDROID_REQUEST_AVAILABLE_CAPABILITIES)) {
                size_t count = characteristicsInfo
                                   .find(ANDROID_REQUEST_AVAILABLE_CAPABILITIES)
                                   .count;
                for (i = 0; i < count; i++) {
                    if (characteristicsInfo
                            .find(ANDROID_REQUEST_AVAILABLE_CAPABILITIES)
                            .data.u8[i] ==
                        ANDROID_REQUEST_AVAILABLE_CAPABILITIES_RAW) {
                        support_cap_raw = true;
                        break;
                    }
                }
            }

            if (characteristicsInfo.exists(
                    ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS)) {
                size_t count = characteristicsInfo
                                   .find(ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS)
                                   .count;
                for (i = 0; i < count; i++) {
                    if (characteristicsInfo
                            .find(ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS)
                            .data.i32[i] ==
                        ANDROID_STATISTICS_LENS_SHADING_MAP_MODE) {
                        if (support_cap_raw == true) {
                            uint8_t lenShadingMapMode =
                                ANDROID_STATISTICS_LENS_SHADING_MAP_MODE_ON;
                            requestInfo.update(
                                ANDROID_STATISTICS_LENS_SHADING_MAP_MODE,
                                &lenShadingMapMode, 1);
                        }
                        break;
                    }
                }
                for (i = 0; i < count; i++) {
                    if (characteristicsInfo
                            .find(ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS)
                            .data.i32[i] == ANDROID_SHADING_MODE) {
                        uint8_t shadingMode = ANDROID_SHADING_MODE_HIGH_QUALITY;
                        requestInfo.update(ANDROID_SHADING_MODE, &shadingMode,
                                           1);
                        break;
                    }
                }
            }
        }
    } else {
        if (characteristicsInfo.exists(
                ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS)) {
            size_t count =
                characteristicsInfo.find(ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS)
                    .count;
            for (i = 0; i < count; i++) {
                if (characteristicsInfo
                        .find(ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS)
                        .data.i32[i] == ANDROID_EDGE_MODE) {
                    if (type == CAMERA3_TEMPLATE_PREVIEW ||
                        type == CAMERA3_TEMPLATE_VIDEO_RECORD) {
                        uint8_t edgeMode = ANDROID_EDGE_MODE_FAST;
                        requestInfo.update(ANDROID_EDGE_MODE, &edgeMode, 1);
                    } else {
                        uint8_t edgeMode = ANDROID_EDGE_MODE_OFF;
                        requestInfo.update(ANDROID_EDGE_MODE, &edgeMode, 1);
                    }
                } else if (characteristicsInfo
                               .find(ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS)
                               .data.i32[i] == ANDROID_NOISE_REDUCTION_MODE) {
                    if (type == CAMERA3_TEMPLATE_PREVIEW ||
                        type == CAMERA3_TEMPLATE_VIDEO_RECORD) {
                        uint8_t noiseReducMode =
                            ANDROID_NOISE_REDUCTION_MODE_FAST;
                        requestInfo.update(ANDROID_NOISE_REDUCTION_MODE,
                                           &noiseReducMode, 1);
                    } else {
                        uint8_t noiseReducMode =
                            ANDROID_NOISE_REDUCTION_MODE_OFF;
                        requestInfo.update(ANDROID_NOISE_REDUCTION_MODE,
                                           &noiseReducMode, 1);
                    }
                } else if (characteristicsInfo
                               .find(ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS)
                               .data.i32[i] == ANDROID_TONEMAP_MODE) {
                    uint8_t toneMapMode = ANDROID_TONEMAP_MODE_FAST;
                    requestInfo.update(ANDROID_TONEMAP_MODE, &toneMapMode, 1);
                } else if (characteristicsInfo
                               .find(ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS)
                               .data.i32[i] ==
                           ANDROID_STATISTICS_LENS_SHADING_MAP_MODE) {
                    uint8_t shadingMapMode =
                        ANDROID_STATISTICS_LENS_SHADING_MAP_MODE_OFF;
                    requestInfo.update(ANDROID_STATISTICS_LENS_SHADING_MAP_MODE,
                                       &shadingMapMode, 1);
                } else if (characteristicsInfo
                               .find(ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS)
                               .data.i32[i] == ANDROID_SHADING_MODE) {
                    // Shading Mode  [ANDROID_SHADING_MODE] for not still
                    // capture
                    if (type == CAMERA3_TEMPLATE_PREVIEW ||
                        type == CAMERA3_TEMPLATE_VIDEO_RECORD) {
                        uint8_t shadingMode = ANDROID_SHADING_MODE_FAST;
                        requestInfo.update(ANDROID_SHADING_MODE, &shadingMode,
                                           1);
                    } else {
                        uint8_t shadingMode = ANDROID_SHADING_MODE_OFF;
                        requestInfo.update(ANDROID_SHADING_MODE, &shadingMode,
                                           1);
                    }
                } else if (characteristicsInfo
                               .find(ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS)
                               .data.i32[i] ==
                           ANDROID_COLOR_CORRECTION_ABERRATION_MODE) {
                    uint8_t colorCorrectionAberrationMode =
                        ANDROID_COLOR_CORRECTION_ABERRATION_MODE_FAST;
                    requestInfo.update(ANDROID_COLOR_CORRECTION_ABERRATION_MODE,
                                       &colorCorrectionAberrationMode, 1);
                }
            }
        }
    }

    uint8_t enableZSL = 0;
    if (type == CAMERA3_TEMPLATE_STILL_CAPTURE) {
        enableZSL = 1;
        requestInfo.update(ANDROID_CONTROL_ENABLE_ZSL, &enableZSL, 1);
    } else {
        requestInfo.update(ANDROID_CONTROL_ENABLE_ZSL, &enableZSL, 1);
    }


    uint8_t captureIntent = type;
    requestInfo.update(ANDROID_CONTROL_CAPTURE_INTENT, &captureIntent, 1);

    int32_t cropRegion[4] = {0, 0, 0, 0};
    requestInfo.update(ANDROID_SCALER_CROP_REGION, cropRegion,
                       ARRAY_SIZE(cropRegion));

    // SPRD
    uint8_t burstCapCnt = 0;
    requestInfo.update(ANDROID_SPRD_BURST_CAP_CNT, &burstCapCnt, 1);

    uint8_t captureMode = 1;
    requestInfo.update(ANDROID_SPRD_CAPTURE_MODE, &captureMode, 1);

    uint8_t brightness = 3;
    requestInfo.update(ANDROID_SPRD_BRIGHTNESS, &brightness, 1);

    uint8_t contrast = 3;
    requestInfo.update(ANDROID_SPRD_CONTRAST, &contrast, 1);

    uint8_t saturation = 3;
    requestInfo.update(ANDROID_SPRD_SATURATION, &saturation, 1);

    uint8_t slowMotion = 1;
    requestInfo.update(ANDROID_SPRD_SLOW_MOTION, &slowMotion, 1);

    uint8_t iso = 0;
    requestInfo.update(ANDROID_SPRD_ISO, &iso, 1);

    uint8_t amMode = 0;
    requestInfo.update(ANDROID_SPRD_METERING_MODE, &amMode, 1);

    uint8_t sprdZslEnabled = 0;
    requestInfo.update(ANDROID_SPRD_ZSL_ENABLED, &sprdZslEnabled, 1);

    uint8_t sprdEisEnabled = 0;
    requestInfo.update(ANDROID_SPRD_EIS_ENABLED, &sprdEisEnabled, 1);

    uint8_t sprd3dnrEnabled = 0;
    requestInfo.update(ANDROID_SPRD_3DNR_ENABLED, &sprd3dnrEnabled, 1);

    int32_t sprdAppmodeId = -1;
    requestInfo.update(ANDROID_SPRD_APP_MODE_ID, &sprdAppmodeId, 1);

    uint8_t sprdFilterType = 0;
    requestInfo.update(ANDROID_SPRD_FILTER_TYPE, &sprdFilterType, 1);
    uint8_t isTakePictureWithFlash = 0;
    requestInfo.update(ANDROID_SPRD_IS_TAKEPICTURE_WITH_FLASH,
                       &isTakePictureWithFlash, 1);

    uint8_t sprdAutoHdrEnabled = 0;
    requestInfo.update(ANDROID_SPRD_AUTO_HDR_ENABLED, &sprdAutoHdrEnabled, 1);

    uint8_t sprdIsHdrScene = 0;
    requestInfo.update(ANDROID_SPRD_IS_HDR_SCENE, &sprdIsHdrScene, 1);
    uint8_t sprdWaterMarkEnabled = 0;
    requestInfo.update(ANDROID_SPRD_LOGOWATERMARK_ENABLED,
                       &sprdWaterMarkEnabled, 1);
    requestInfo.update(ANDROID_SPRD_TIMEWATERMARK_ENABLED,
                       &sprdWaterMarkEnabled, 1);

    if (!strcmp(FRONT_CAMERA_FLASH_TYPE, "lcd")) {
        uint8_t sprdFlashLcdMode = FLASH_LCD_MODE_OFF;
        requestInfo.update(ANDROID_SPRD_FLASH_LCD_MODE, &sprdFlashLcdMode, 1);
    }

    if (mCameraId == 0) {
        requestInfo.update(ANDROID_SPRD_VCM_STEP,
                           &(s_setting[mCameraId].vcmInfo.vcm_step), 1);
        requestInfo.update(ANDROID_SPRD_OTP_DATA,
                           s_setting[mCameraId].otpInfo.otp_data,
                           SPRD_DUAL_OTP_SIZE);
        requestInfo.update(ANDROID_SPRD_DUAL_OTP_FLAG,
                           &(s_setting[mCameraId].otpInfo.dual_otp_flag), 1);
    }

    mDefaultMetadata[type] = requestInfo.release();
    *metadata = mDefaultMetadata[type];

    return 0;
}

int SprdCamera3Setting::popAndroidParaTag() {
    List<camera_metadata_tag_t>::iterator tag;
    int ret;

    if (mParaChangedTagQueue.size() == 0)
        return -1;

    tag = mParaChangedTagQueue.begin()++;
    ret = static_cast<int>(*tag);
    mParaChangedTagQueue.erase(tag);
    return ret;
}
int SprdCamera3Setting::popSprdParaTag() {
    List<sprd_camera_metadata_tag_t>::iterator tag;
    int ret;

    if (mSprdParaChangedTagQueue.size() == 0)
        return -1;

    tag = mSprdParaChangedTagQueue.begin()++;
    ret = static_cast<int>(*tag);
    mSprdParaChangedTagQueue.erase(tag);
    return ret;
}

void SprdCamera3Setting::pushAndroidParaTag(camera_metadata_tag_t tag) {
    mParaChangedTagQueue.push_back(tag);
}
void SprdCamera3Setting::pushAndroidParaTag(sprd_camera_metadata_tag_t tag) {
    mSprdParaChangedTagQueue.push_back(tag);
}

void SprdCamera3Setting::releaseAndroidParaTag() {
    List<camera_metadata_tag_t>::iterator round;
    HAL_LOGD("para changed.size : %d", mParaChangedTagQueue.size());

    while (mParaChangedTagQueue.size() > 0) {
        round = mParaChangedTagQueue.begin()++;
        mParaChangedTagQueue.erase(round);
    }
}

int SprdCamera3Setting::updateWorkParameters(
    const CameraMetadata &frame_settings) {
    int rc = 0;
    uint32_t tagCnt;
    uint8_t valueU8 = 0;
    float valueFloat = 0.0f;
    int32_t valueI32 = 0;
    int64_t valueI64 = 0;
    int32_t is_capture = 0;

    uint8_t is_raw_capture = 0;
    char value[PROPERTY_VALUE_MAX];
    struct camera_info cameraInfo;
    memset(&cameraInfo, 0, sizeof(cameraInfo));
    getCameraInfo(mCameraId, &cameraInfo);

    property_get("persist.vendor.cam.raw.mode", value, "jpeg");
    if (!strcmp(value, "raw")) {
        is_raw_capture = 1;
    }

    Mutex::Autolock l(mLock);

#define GET_VALUE_IF_DIF(x, y, tag)                                            \
    if (((x) != (y)) || ((x) == (y) && (x) == 0)) {                            \
        if ((x) != (y))                                                        \
            rc++;                                                              \
        (x) = (y);                                                             \
        pushAndroidParaTag(tag);                                               \
    }

    tagCnt = frame_settings.entryCount();
    HAL_LOGV("cnt %d", tagCnt);

    if (tagCnt == 0) {
        return rc;
    }
    if (frame_settings.exists(ANDROID_SPRD_APP_MODE_ID)) {
        s_setting[mCameraId].sprddefInfo.sprd_appmode_id =
            frame_settings.find(ANDROID_SPRD_APP_MODE_ID).data.i32[0];
        pushAndroidParaTag(ANDROID_SPRD_APP_MODE_ID);
        HAL_LOGV("sprd app mode id is %d",
                 s_setting[mCameraId].sprddefInfo.sprd_appmode_id);
    }
    if (frame_settings.exists(ANDROID_CONTROL_AE_ANTIBANDING_MODE)) {
        valueU8 =
            frame_settings.find(ANDROID_CONTROL_AE_ANTIBANDING_MODE).data.u8[0];
        GET_VALUE_IF_DIF(s_setting[mCameraId].controlInfo.antibanding_mode,
                         valueU8, ANDROID_CONTROL_AE_ANTIBANDING_MODE)
        HAL_LOGV("ANDROID_CONTROL_AE_ANTIBANDING_MODE %d", valueU8);
    }

    if (frame_settings.exists(ANDROID_CONTROL_ENABLE_ZSL)) {
        valueU8 = frame_settings.find(ANDROID_CONTROL_ENABLE_ZSL).data.u8[0];
        s_setting[mCameraId].controlInfo.enable_zsl = valueU8;
        HAL_LOGD("Enable Zsl = %d", valueU8);
    }

    // CONTROL
    if (frame_settings.exists(ANDROID_CONTROL_CAPTURE_INTENT)) {
        valueU8 =
            frame_settings.find(ANDROID_CONTROL_CAPTURE_INTENT).data.u8[0];
        GET_VALUE_IF_DIF(s_setting[mCameraId].controlInfo.capture_intent,
                         valueU8, ANDROID_CONTROL_CAPTURE_INTENT)
        if (ANDROID_CONTROL_CAPTURE_INTENT_STILL_CAPTURE ==
                s_setting[mCameraId].controlInfo.capture_intent ||
            ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_SNAPSHOT ==
                s_setting[mCameraId].controlInfo.capture_intent) {
            is_capture = 1;
        }
    }

    // LENS
    if (frame_settings.exists(ANDROID_LENS_FOCAL_LENGTH)) {
        valueFloat = frame_settings.find(ANDROID_LENS_FOCAL_LENGTH).data.f[0];
        GET_VALUE_IF_DIF(s_setting[mCameraId].lensInfo.focal_length, valueFloat,
                         ANDROID_LENS_FOCAL_LENGTH)
        HAL_LOGV("lens focal len is %f", valueFloat);
    }

    // REQUEST
    /*if (frame_settings.exists(ANDROID_REQUEST_ID)) {
            valueI32 = frame_settings.find(ANDROID_REQUEST_ID).data.i32[0];
            GET_VALUE_IF_DIF(s_setting[mCameraId].requestInfo.id, valueI32,
    ANDROID_REQUEST_ID)
            HAL_LOGD("android request id %d", valueI32);
    }*/

    if (frame_settings.exists(ANDROID_REQUEST_TYPE)) {
        valueU8 = frame_settings.find(ANDROID_REQUEST_TYPE).data.u8[0];
        GET_VALUE_IF_DIF(s_setting[mCameraId].requestInfo.type, valueU8,
                         ANDROID_REQUEST_TYPE)
        HAL_LOGD("req type %d", valueU8);
    }

    // black level lock
    if (frame_settings.exists(ANDROID_BLACK_LEVEL_LOCK)) {
        valueU8 = frame_settings.find(ANDROID_BLACK_LEVEL_LOCK).data.u8[0];
        GET_VALUE_IF_DIF(s_setting[mCameraId].black_level_lock, valueU8,
                         ANDROID_BLACK_LEVEL_LOCK)
        HAL_LOGV("android black level lock %d", valueU8);
    }

    // edge mode
    if (frame_settings.exists(ANDROID_EDGE_MODE)) {
        valueU8 = frame_settings.find(ANDROID_EDGE_MODE).data.u8[0];
        GET_VALUE_IF_DIF(s_setting[mCameraId].edgeInfo.mode, valueU8,
                         ANDROID_EDGE_MODE)
        HAL_LOGV("android edge mode %d", valueU8);
    }

    // noise reduction mode
    if (frame_settings.exists(ANDROID_NOISE_REDUCTION_MODE)) {
        valueU8 = frame_settings.find(ANDROID_NOISE_REDUCTION_MODE).data.u8[0];
        GET_VALUE_IF_DIF(s_setting[mCameraId].noiseInfo.reduction_mode, valueU8,
                         ANDROID_NOISE_REDUCTION_MODE)
        HAL_LOGV("android noise reduction mode %d", valueU8);
    }

    // statistics lens shading map mode
    if (frame_settings.exists(ANDROID_STATISTICS_LENS_SHADING_MAP_MODE)) {
        valueU8 = frame_settings.find(ANDROID_STATISTICS_LENS_SHADING_MAP_MODE)
                      .data.u8[0];
        GET_VALUE_IF_DIF(
            s_setting[mCameraId].statisticsInfo.lens_shading_map_mode, valueU8,
            ANDROID_STATISTICS_LENS_SHADING_MAP_MODE)
        HAL_LOGV("android statistics lens shading map mode  %d", valueU8);
    }
    // shading mode
    if (frame_settings.exists(ANDROID_SHADING_MODE)) {
        valueU8 = frame_settings.find(ANDROID_SHADING_MODE).data.u8[0];
        GET_VALUE_IF_DIF(s_setting[mCameraId].shadingInfo.mode, valueU8,
                         ANDROID_SHADING_MODE)
        HAL_LOGV("android shading mode %d", valueU8);
    }

    // TONE CURVE
    if (frame_settings.exists(ANDROID_TONEMAP_CURVE_BLUE)) {
        memset(s_setting[mCameraId].toneInfo.curve_blue, 0,
               SPRD_MAX_TONE_CURVE_POINT * sizeof(float));
        memcpy(s_setting[mCameraId].toneInfo.curve_blue,
               frame_settings.find(ANDROID_TONEMAP_CURVE_BLUE).data.f,
               frame_settings.find(ANDROID_TONEMAP_CURVE_BLUE).count *
                   sizeof(float));

        memset(s_setting[mCameraId].toneInfo.curve_green, 0,
               SPRD_MAX_TONE_CURVE_POINT * sizeof(float));
        memcpy(s_setting[mCameraId].toneInfo.curve_green,
               frame_settings.find(ANDROID_TONEMAP_CURVE_GREEN).data.f,
               frame_settings.find(ANDROID_TONEMAP_CURVE_GREEN).count *
                   sizeof(float));

        memset(s_setting[mCameraId].toneInfo.curve_red, 0,
               SPRD_MAX_TONE_CURVE_POINT * sizeof(float));
        memcpy(s_setting[mCameraId].toneInfo.curve_red,
               frame_settings.find(ANDROID_TONEMAP_CURVE_RED).data.f,
               frame_settings.find(ANDROID_TONEMAP_CURVE_RED).count *
                   sizeof(float));
        /*		HAL_LOGD("android tone cure point, count = %d");*/
    }

    if (frame_settings.exists(ANDROID_TONEMAP_MODE)) {
        valueU8 = frame_settings.find(ANDROID_TONEMAP_MODE).data.u8[0];
        GET_VALUE_IF_DIF(s_setting[mCameraId].toneInfo.mode, valueU8,
                         ANDROID_TONEMAP_MODE)
        HAL_LOGV("android tonemap mode %d", valueU8);
    }

    // SENSOR
    if (frame_settings.exists(ANDROID_SENSOR_EXPOSURE_TIME)) {
        valueI64 =
            frame_settings.find(ANDROID_SENSOR_EXPOSURE_TIME).data.i64[0];
        GET_VALUE_IF_DIF(s_setting[mCameraId].sensorInfo.exposure_time,
                         valueI64, ANDROID_SENSOR_EXPOSURE_TIME)
        HAL_LOGV("sensor exposure_time is update = %lld", valueI64);
    }

    if (frame_settings.exists(ANDROID_SENSOR_FRAME_DURATION)) {
        valueI64 =
            frame_settings.find(ANDROID_SENSOR_FRAME_DURATION).data.i64[0];
        GET_VALUE_IF_DIF(s_setting[mCameraId].sensorInfo.frame_duration,
                         valueI64, ANDROID_SENSOR_FRAME_DURATION)
        HAL_LOGV("frame duration %lld", valueI64);
    }

    if (frame_settings.exists(ANDROID_SENSOR_SENSITIVITY)) {
        valueI32 = frame_settings.find(ANDROID_SENSOR_SENSITIVITY).data.i32[0];
        GET_VALUE_IF_DIF(s_setting[mCameraId].sensorInfo.sensitivity, valueI32,
                         ANDROID_SENSOR_SENSITIVITY)
        HAL_LOGV("sensitivity is %d", valueI32);
    }

    if (frame_settings.exists(ANDROID_SPRD_SENSOR_ORIENTATION)) {
        s_setting[mCameraId].sprddefInfo.sensor_orientation =
            frame_settings.find(ANDROID_SPRD_SENSOR_ORIENTATION).data.u8[0];
        pushAndroidParaTag(ANDROID_SPRD_SENSOR_ORIENTATION);
        HAL_LOGV("orien %d",
                 s_setting[mCameraId].sprddefInfo.sensor_orientation);
    }

    if (frame_settings.exists(ANDROID_SPRD_DEVICE_ORIENTATION)) {
        s_setting[mCameraId].sprddefInfo.device_orietation =
            frame_settings.find(ANDROID_SPRD_DEVICE_ORIENTATION).data.i32[0];
        pushAndroidParaTag(ANDROID_SPRD_DEVICE_ORIENTATION);
        HAL_LOGD("device_orietation %d",
                 s_setting[mCameraId].sprddefInfo.device_orietation);
    }

    if (frame_settings.exists(ANDROID_SPRD_SENSOR_ROTATION)) {
        int32_t rotation =
            frame_settings.find(ANDROID_SPRD_SENSOR_ROTATION).data.i32[0];

        if (-1 == rotation)
            rotation = 0;
        s_setting[mCameraId].sprddefInfo.sensor_rotation = rotation;
        pushAndroidParaTag(ANDROID_SPRD_SENSOR_ROTATION);
        HAL_LOGV("rot %d", s_setting[mCameraId].sprddefInfo.sensor_rotation);
    }

    if (frame_settings.exists(ANDROID_SPRD_UCAM_SKIN_LEVEL)) {
        int32_t perfectskinlevel[SPRD_FACE_BEAUTY_PARAM_NUM];
        if (is_raw_capture == 1) {
            memset(perfectskinlevel, 0,
                   sizeof(int32_t) * SPRD_FACE_BEAUTY_PARAM_NUM);
        } else {
            for (size_t i = 0;
                 i < (frame_settings.find(ANDROID_SPRD_UCAM_SKIN_LEVEL).count);
                 i++) {
                perfectskinlevel[i] =
                    frame_settings.find(ANDROID_SPRD_UCAM_SKIN_LEVEL)
                        .data.i32[i];
                HAL_LOGV("face beauty level %d : %d", i, perfectskinlevel[i]);
            }
        }
        memcpy(s_setting[mCameraId].sprddefInfo.perfect_skin_level,
               perfectskinlevel, sizeof(int32_t) * SPRD_FACE_BEAUTY_PARAM_NUM);
        pushAndroidParaTag(ANDROID_SPRD_UCAM_SKIN_LEVEL);
    }

    if (frame_settings.exists(ANDROID_SPRD_EIS_ENABLED)) {
        int32_t sprd_eis_enabled =
            frame_settings.find(ANDROID_SPRD_EIS_ENABLED).data.u8[0];

        s_setting[mCameraId].sprddefInfo.sprd_eis_enabled = sprd_eis_enabled;
        pushAndroidParaTag(ANDROID_SPRD_EIS_ENABLED);
    }

    if (frame_settings.exists(ANDROID_SPRD_CONTROL_FRONT_CAMERA_MIRROR)) {
        uint8_t flip_on =
            frame_settings.find(ANDROID_SPRD_CONTROL_FRONT_CAMERA_MIRROR)
                .data.u8[0];
        s_setting[mCameraId].sprddefInfo.flip_on = flip_on;
        pushAndroidParaTag(ANDROID_SPRD_CONTROL_FRONT_CAMERA_MIRROR);
        HAL_LOGV("flip_on_level %d", s_setting[mCameraId].sprddefInfo.flip_on);
    }
    if (frame_settings.exists(ANDROID_SPRD_FILTER_TYPE)) {
        s_setting[mCameraId].sprddefInfo.sprd_filter_type =
            frame_settings.find(ANDROID_SPRD_FILTER_TYPE).data.i32[0];
        pushAndroidParaTag(ANDROID_SPRD_FILTER_TYPE);
    }
    // JPEG
    if (frame_settings.exists(ANDROID_JPEG_QUALITY) && is_capture) {
        valueU8 = frame_settings.find(ANDROID_JPEG_QUALITY).data.u8[0];
        GET_VALUE_IF_DIF(s_setting[mCameraId].jpgInfo.quality, valueU8,
                         ANDROID_JPEG_QUALITY)
        HAL_LOGV("jpeg quality is %d", valueU8);
    }

    if (frame_settings.exists(ANDROID_JPEG_THUMBNAIL_QUALITY) && is_capture) {
        valueU8 =
            frame_settings.find(ANDROID_JPEG_THUMBNAIL_QUALITY).data.u8[0];
        GET_VALUE_IF_DIF(s_setting[mCameraId].jpgInfo.thumbnail_quality,
                         valueU8, ANDROID_JPEG_THUMBNAIL_QUALITY)
        HAL_LOGV("thumnail quality %d",
                 s_setting[mCameraId].jpgInfo.thumbnail_quality);
    }

    if (frame_settings.exists(ANDROID_JPEG_THUMBNAIL_SIZE) && is_capture) {
        int32_t thumW =
            frame_settings.find(ANDROID_JPEG_THUMBNAIL_SIZE).data.i32[0];
        int32_t thumH =
            frame_settings.find(ANDROID_JPEG_THUMBNAIL_SIZE).data.i32[1];
        if (s_setting[mCameraId].jpgInfo.thumbnail_size[0] != thumW ||
            s_setting[mCameraId].jpgInfo.thumbnail_size[1] != thumH) {
            s_setting[mCameraId].jpgInfo.thumbnail_size[0] = thumW;
            s_setting[mCameraId].jpgInfo.thumbnail_size[1] = thumH;
            pushAndroidParaTag(ANDROID_JPEG_THUMBNAIL_SIZE);
        }
        HAL_LOGV("jpeg thumnail size is %dx%d", thumW, thumH);
    }
    if (frame_settings.exists(ANDROID_JPEG_ORIENTATION) && is_capture) {
        int32_t jpeg_orientation =
            frame_settings.find(ANDROID_JPEG_ORIENTATION).data.i32[0];
        HAL_LOGV("jpeg_orientation %d ", jpeg_orientation);
        s_setting[mCameraId].jpgInfo.orientation_original = jpeg_orientation;
        if (jpeg_orientation == -1) {
            HAL_LOGV("rot not specified or invalid, set to 0");
            jpeg_orientation = 0;
        } else if (jpeg_orientation % 90) {
            HAL_LOGV("rot %d is not a multiple of 90 degrees!  set to zero.",
                     jpeg_orientation);
            jpeg_orientation = 0;
        } else {
            // normalize to [0 - 270] degrees
            jpeg_orientation %= 360;
            if (jpeg_orientation < 0)
                jpeg_orientation += 360;
        }

        s_setting[mCameraId].jpgInfo.orientation = jpeg_orientation;
        pushAndroidParaTag(ANDROID_JPEG_ORIENTATION);
    }

    if (frame_settings.exists(ANDROID_JPEG_GPS_COORDINATES) && is_capture) {
        for (size_t i = 0;
             i < frame_settings.find(ANDROID_JPEG_GPS_COORDINATES).count; i++) {
            s_setting[mCameraId].jpgInfo.gps_coordinates[i] =
                frame_settings.find(ANDROID_JPEG_GPS_COORDINATES).data.d[i];
            HAL_LOGD("GPS coordinates %lf",
                     s_setting[mCameraId].jpgInfo.gps_coordinates[i]);
        }
        pushAndroidParaTag(ANDROID_JPEG_GPS_COORDINATES);
    }

    if (frame_settings.exists(ANDROID_JPEG_GPS_PROCESSING_METHOD) &&
        is_capture) {
        for (size_t i = 0;
             i < frame_settings.find(ANDROID_JPEG_GPS_PROCESSING_METHOD).count;
             i++) {
            s_setting[mCameraId].jpgInfo.gps_processing_method[i] =
                frame_settings.find(ANDROID_JPEG_GPS_PROCESSING_METHOD)
                    .data.u8[i];
        }
        s_setting[mCameraId].jpgInfo.gps_processing_method
            [frame_settings.find(ANDROID_JPEG_GPS_PROCESSING_METHOD).count] = 0;
        HAL_LOGD("GPS processin method %s",
                 s_setting[mCameraId].jpgInfo.gps_processing_method);
        pushAndroidParaTag(ANDROID_JPEG_GPS_PROCESSING_METHOD);
    }

    if (frame_settings.exists(ANDROID_JPEG_GPS_TIMESTAMP) && is_capture) {
        s_setting[mCameraId].jpgInfo.gps_timestamp =
            frame_settings.find(ANDROID_JPEG_GPS_TIMESTAMP).data.i64[0];
        HAL_LOGD("GPS timestamp %lld",
                 s_setting[mCameraId].jpgInfo.gps_timestamp);
        pushAndroidParaTag(ANDROID_JPEG_GPS_TIMESTAMP);
    }

    if (frame_settings.exists(ANDROID_FLASH_MODE)) {
        valueU8 = frame_settings.find(ANDROID_FLASH_MODE).data.u8[0];
        GET_VALUE_IF_DIF(s_setting[mCameraId].flashInfo.mode, valueU8,
                         ANDROID_FLASH_MODE)
    }

    if (frame_settings.exists(ANDROID_CONTROL_AE_MODE)) {
        valueU8 = frame_settings.find(ANDROID_CONTROL_AE_MODE).data.u8[0];
        HAL_LOGV("ae mode %d", s_setting[mCameraId].controlInfo.ae_mode);
        if (s_setting[mCameraId].flash_InfoInfo.available == 0 &&
            valueU8 > ANDROID_CONTROL_AE_MODE_ON) {
            valueU8 = ANDROID_CONTROL_AE_MODE_ON;
        }
        GET_VALUE_IF_DIF(s_setting[mCameraId].controlInfo.ae_mode, valueU8,
                         ANDROID_CONTROL_AE_MODE)
    }

    if (frame_settings.exists(ANDROID_CONTROL_AE_LOCK)) {
        valueU8 = frame_settings.find(ANDROID_CONTROL_AE_LOCK).data.u8[0];
        GET_VALUE_IF_DIF(s_setting[mCameraId].controlInfo.ae_lock, valueU8,
                         ANDROID_CONTROL_AE_LOCK)
    }

    if (frame_settings.exists(ANDROID_CONTROL_SCENE_MODE)) {
        valueU8 = frame_settings.find(ANDROID_CONTROL_SCENE_MODE).data.u8[0];
        GET_VALUE_IF_DIF(s_setting[mCameraId].controlInfo.scene_mode, valueU8,
                         ANDROID_CONTROL_SCENE_MODE)
    }

    if (frame_settings.exists(ANDROID_CONTROL_EFFECT_MODE)) {
        valueU8 = frame_settings.find(ANDROID_CONTROL_EFFECT_MODE).data.u8[0];
        GET_VALUE_IF_DIF(s_setting[mCameraId].controlInfo.effect_mode, valueU8,
                         ANDROID_CONTROL_EFFECT_MODE)
        HAL_LOGV("effect %d", valueU8);
    }

    if (frame_settings.exists(ANDROID_CONTROL_AWB_MODE)) {
        valueU8 = frame_settings.find(ANDROID_CONTROL_AWB_MODE).data.u8[0];
        GET_VALUE_IF_DIF(s_setting[mCameraId].controlInfo.awb_mode, valueU8,
                         ANDROID_CONTROL_AWB_MODE)
        HAL_LOGV("awb %d", valueU8);
    }

    if (frame_settings.exists(ANDROID_CONTROL_AWB_LOCK)) {
        valueU8 = frame_settings.find(ANDROID_CONTROL_AWB_LOCK).data.u8[0];
        GET_VALUE_IF_DIF(s_setting[mCameraId].controlInfo.awb_lock, valueU8,
                         ANDROID_CONTROL_AWB_LOCK)
        HAL_LOGV("awb_lock %d", valueU8);
    }

    if (frame_settings.exists(ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION)) {
        if (s_setting[mCameraId].controlInfo.ae_exposure_compensation !=
            frame_settings.find(ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION)
                .data.i32[0]) {
            s_setting[mCameraId].controlInfo.ae_manual_trigger = 1;
            s_setting[mCameraId].controlInfo.ae_comp_change = true;
            s_setting[mCameraId].controlInfo.ae_comp_effect_frames_cnt =
                EV_EFFECT_FRAME_NUM;
        }
        s_setting[mCameraId].controlInfo.ae_exposure_compensation =
            frame_settings.find(ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION)
                .data.i32[0];
        pushAndroidParaTag(ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION);
    }

    if (frame_settings.exists(ANDROID_CONTROL_AE_ANTIBANDING_MODE)) {
        valueU8 =
            frame_settings.find(ANDROID_CONTROL_AE_ANTIBANDING_MODE).data.u8[0];
        GET_VALUE_IF_DIF(s_setting[mCameraId].controlInfo.ae_abtibanding_mode,
                         valueU8, ANDROID_CONTROL_AE_ANTIBANDING_MODE)
        HAL_LOGV("ANDROID_CONTROL_AE_ANTIBANDING_MODE %d", valueU8);
    }
    // SPRD
    if (frame_settings.exists(ANDROID_SPRD_CAPTURE_MODE)) {
        valueU8 = frame_settings.find(ANDROID_SPRD_CAPTURE_MODE).data.u8[0];
        GET_VALUE_IF_DIF(s_setting[mCameraId].sprddefInfo.capture_mode, valueU8,
                         ANDROID_SPRD_CAPTURE_MODE)
    }
    if (frame_settings.exists(ANDROID_SPRD_BURST_CAP_CNT)) {
        valueU8 = frame_settings.find(ANDROID_SPRD_BURST_CAP_CNT).data.u8[0];
        GET_VALUE_IF_DIF(s_setting[mCameraId].sprddefInfo.burst_cap_cnt,
                         valueU8, ANDROID_SPRD_BURST_CAP_CNT)
    }

    if (frame_settings.exists(ANDROID_SPRD_BRIGHTNESS)) {
        s_setting[mCameraId].sprddefInfo.brightness =
            frame_settings.find(ANDROID_SPRD_BRIGHTNESS).data.u8[0];
        pushAndroidParaTag(ANDROID_SPRD_BRIGHTNESS);
        HAL_LOGV("brightness is %d",
                 s_setting[mCameraId].sprddefInfo.brightness);
    }
    if (frame_settings.exists(ANDROID_CONTROL_MODE)) {
        s_setting[mCameraId].controlInfo.mode =
            frame_settings.find(ANDROID_CONTROL_MODE).data.u8[0];
        // pushAndroidParaTag(ANDROID_SPRD_BRIGHTNESS);
        HAL_LOGV("ANDROID_CONTROL_MODE is %d",
                 s_setting[mCameraId].controlInfo.mode);
    }
    if (frame_settings.exists(ANDROID_SPRD_CONTRAST)) {
        s_setting[mCameraId].sprddefInfo.contrast =
            frame_settings.find(ANDROID_SPRD_CONTRAST).data.u8[0];
        valueU8 = frame_settings.find(ANDROID_SPRD_CONTRAST).data.u8[0];
        pushAndroidParaTag(ANDROID_SPRD_CONTRAST);
        HAL_LOGV("contrast is %d", valueU8);
    }
    if (frame_settings.exists(ANDROID_SPRD_SATURATION)) {
        s_setting[mCameraId].sprddefInfo.saturation =
            frame_settings.find(ANDROID_SPRD_SATURATION).data.u8[0];
        valueU8 = frame_settings.find(ANDROID_SPRD_SATURATION).data.u8[0];
        pushAndroidParaTag(ANDROID_SPRD_SATURATION);
        HAL_LOGV("saturation is %d", valueU8);
    }
    if (frame_settings.exists(ANDROID_SPRD_ISO)) {
        s_setting[mCameraId].sprddefInfo.iso =
            frame_settings.find(ANDROID_SPRD_ISO).data.u8[0];
        valueU8 = frame_settings.find(ANDROID_SPRD_ISO).data.u8[0];
        pushAndroidParaTag(ANDROID_SPRD_ISO);
    }

    if (frame_settings.exists(ANDROID_SPRD_SLOW_MOTION)) {
        s_setting[mCameraId].sprddefInfo.slowmotion =
            frame_settings.find(ANDROID_SPRD_SLOW_MOTION).data.u8[0];
        pushAndroidParaTag(ANDROID_SPRD_SLOW_MOTION);
        HAL_LOGV("slowmotion %d", s_setting[mCameraId].sprddefInfo.slowmotion);
    }

    if (frame_settings.exists(ANDROID_SPRD_METERING_MODE)) {
        s_setting[mCameraId].sprddefInfo.am_mode =
            frame_settings.find(ANDROID_SPRD_METERING_MODE).data.u8[0];
        pushAndroidParaTag(ANDROID_SPRD_METERING_MODE);
    }

    int updateAE = false;
    if (frame_settings.exists(ANDROID_CONTROL_AE_REGIONS)) {
        int32_t crop_area[5] = {0};
        int32_t ae_area[5] = {0};
        size_t i = 0, cnt = 0;
        int ret = 0;

        if (frame_settings.find(ANDROID_CONTROL_AE_REGIONS).count == 5) {
            for (i = 0; i < 5; i++)
                ae_area[i] =
                    frame_settings.find(ANDROID_CONTROL_AE_REGIONS).data.i32[i];

            if (frame_settings.exists(ANDROID_SCALER_CROP_REGION)) {
                cnt = frame_settings.find(ANDROID_SCALER_CROP_REGION).count;
                if (cnt > 5) {
                    cnt = 5;
                }
                for (i = 0; i < cnt; i++)
                    crop_area[i] =
                        frame_settings.find(ANDROID_SCALER_CROP_REGION)
                            .data.i32[i];

                crop_area[2] = crop_area[2] + crop_area[0];
                crop_area[3] = crop_area[3] + crop_area[1];
            }

            ret = checkROIValid(ae_area, crop_area);
            if (ret) {
                HAL_LOGE("ae region invalid");
            } else {
                uint8_t af_trigger = 0;
                if (frame_settings.exists(ANDROID_CONTROL_AF_TRIGGER)) {
                    af_trigger = frame_settings.find(ANDROID_CONTROL_AF_TRIGGER)
                                     .data.u8[0];
                }

                if (af_trigger == ANDROID_CONTROL_AF_TRIGGER_CANCEL) {
                    HAL_LOGD("cancel auto focus dont set ae region");
                } else {
                    updateAE = true;
                    for (size_t i = 0;
                         i <
                         frame_settings.find(ANDROID_CONTROL_AE_REGIONS).count;
                         i++)
                        s_setting[mCameraId].controlInfo.ae_regions[i] =
                            ae_area[i];

                    pushAndroidParaTag(ANDROID_CONTROL_AE_REGIONS);
                }
            }
        }
    }

    if (frame_settings.exists(ANDROID_SPRD_AF_MODE_MACRO_FIXED)) {
        valueU8 =
            frame_settings.find(ANDROID_SPRD_AF_MODE_MACRO_FIXED).data.u8[0];
        GET_VALUE_IF_DIF(s_setting[mCameraId].sprddefInfo.is_macro_fixed,
                         valueU8, ANDROID_SPRD_AF_MODE_MACRO_FIXED)
        HAL_LOGD("is_macro_fixed is %d", valueU8);
    }

    /**add for 3d calibration update metadata begin*/
    if (frame_settings.exists(ANDROID_SPRD_3DCALIBRATION_ENABLED)) {
        valueU8 =
            frame_settings.find(ANDROID_SPRD_3DCALIBRATION_ENABLED).data.u8[0];
        GET_VALUE_IF_DIF(
            s_setting[mCameraId].sprddefInfo.sprd_3dcalibration_enabled,
            valueU8, ANDROID_SPRD_3DCALIBRATION_ENABLED)
        HAL_LOGD("sprd_3dcalibration_enabled %d",
                 s_setting[mCameraId].sprddefInfo.sprd_3dcalibration_enabled);
    } else {
        char prop[PROPERTY_VALUE_MAX];
        int val = 0;
        property_get("persist.vendor.cam.hal.3d", prop, "0");
        val = atoi(prop);
        if (1 == val) {
            s_setting[mCameraId].sprddefInfo.sprd_3dcalibration_enabled = val;
            pushAndroidParaTag(ANDROID_SPRD_3DCALIBRATION_ENABLED);
            HAL_LOGD(
                "force set sprd_3dcalibration_enabled %d",
                s_setting[mCameraId].sprddefInfo.sprd_3dcalibration_enabled);
        } else {
            s_setting[mCameraId].sprddefInfo.sprd_3dcalibration_enabled = 0;
        }
    }
    /**add for 3d calibration update metadata end*/

    if (frame_settings.exists(ANDROID_SPRD_ZSL_ENABLED)) {
        if (is_raw_capture == 1) {
            s_setting[mCameraId].sprddefInfo.sprd_zsl_enabled = 0;
        } else {
            s_setting[mCameraId].sprddefInfo.sprd_zsl_enabled =
                frame_settings.find(ANDROID_SPRD_ZSL_ENABLED).data.u8[0];
            /**add for 3d calibration force set sprd zsl enable begin*/
            if (s_setting[mCameraId].sprddefInfo.sprd_3dcalibration_enabled)
                s_setting[mCameraId].sprddefInfo.sprd_zsl_enabled =
                    s_setting[mCameraId].sprddefInfo.sprd_3dcalibration_enabled;
            /**add for 3d calibration force set sprd zsl enable end*/
        }
        pushAndroidParaTag(ANDROID_SPRD_ZSL_ENABLED);
    }

    int32_t cropRegionUpdate = false;
    if (frame_settings.exists(ANDROID_SCALER_CROP_REGION)) {
        int32_t x = frame_settings.find(ANDROID_SCALER_CROP_REGION).data.i32[0];
        int32_t y = frame_settings.find(ANDROID_SCALER_CROP_REGION).data.i32[1];
        int32_t w = frame_settings.find(ANDROID_SCALER_CROP_REGION).data.i32[2];
        int32_t h = frame_settings.find(ANDROID_SCALER_CROP_REGION).data.i32[3];
        if (s_setting[mCameraId].scalerInfo.crop_region[0] != x ||
            s_setting[mCameraId].scalerInfo.crop_region[1] != y ||
            s_setting[mCameraId].scalerInfo.crop_region[2] != w ||
            s_setting[mCameraId].scalerInfo.crop_region[3] != h) {
            s_setting[mCameraId].scalerInfo.crop_region[0] = x;
            s_setting[mCameraId].scalerInfo.crop_region[1] = y;
            s_setting[mCameraId].scalerInfo.crop_region[2] = w;
            s_setting[mCameraId].scalerInfo.crop_region[3] = h;
            // pushAndroidParaTag(ANDROID_SCALER_CROP_REGION);
            cropRegionUpdate = true;
        } else if (s_setting[mCameraId].video_size.width != 0 &&
                   s_setting[mCameraId].video_size.height != 0 &&
                   (s_setting[mCameraId].preview_size.width !=
                        s_setting[mCameraId].video_size.width ||
                    s_setting[mCameraId].preview_size.height !=
                        s_setting[mCameraId].video_size.height)) {
            cropRegionUpdate = true;
        }
        if (cropRegionUpdate == true)
            pushAndroidParaTag(ANDROID_SCALER_CROP_REGION);
    }

    // AE
    if (frame_settings.exists(ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER)) {
        valueU8 = frame_settings.find(ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER)
                      .data.u8[0];
        s_setting[mCameraId].controlInfo.ae_precap_trigger = valueU8;
        if (valueU8 == 1) {
            pushAndroidParaTag(ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER);
            HAL_LOGD("AE precap trigger status = %d", valueU8);
        }
    }
    if (frame_settings.exists(ANDROID_CONTROL_AE_PRECAPTURE_ID)) {
        s_setting[mCameraId].controlInfo.ae_precapture_id =
            frame_settings.find(ANDROID_CONTROL_AE_PRECAPTURE_ID).data.i32[0];
        pushAndroidParaTag(ANDROID_CONTROL_AE_PRECAPTURE_ID);
        HAL_LOGD("AE precap trigger id = %d",
                 s_setting[mCameraId].controlInfo.ae_precapture_id);
    }
    if (frame_settings.exists(ANDROID_CONTROL_AE_TARGET_FPS_RANGE)) {
        // if((s_setting[mCameraId].controlInfo.ae_target_fps_range[1]
        // !=frame_settings.find(ANDROID_CONTROL_AE_TARGET_FPS_RANGE).data.i32[1]))
        // {
        HAL_LOGV("AE target fps min %d, max %d",
                 s_setting[mCameraId].controlInfo.ae_target_fps_range[0],
                 s_setting[mCameraId].controlInfo.ae_target_fps_range[1]);
        pushAndroidParaTag(ANDROID_CONTROL_AE_TARGET_FPS_RANGE);
        //}
        s_setting[mCameraId].controlInfo.ae_target_fps_range[0] =
            frame_settings.find(ANDROID_CONTROL_AE_TARGET_FPS_RANGE)
                .data.i32[0];
        s_setting[mCameraId].controlInfo.ae_target_fps_range[1] =
            frame_settings.find(ANDROID_CONTROL_AE_TARGET_FPS_RANGE)
                .data.i32[1];
        HAL_LOGV("AE target fps min %d, max %d",
                 s_setting[mCameraId].controlInfo.ae_target_fps_range[0],
                 s_setting[mCameraId].controlInfo.ae_target_fps_range[1]);
    }

    // Before start af, need set af region
    if (mSensorFocusEnable[mCameraId]) {
        if (frame_settings.exists(ANDROID_CONTROL_AF_REGIONS)) {
            int32_t crop_area[5] = {0};
            int32_t af_area[5] = {0};
            size_t i = 0, cnt = 0;
            uint8_t af_triger = 0;
            int ret = 0;

            if (frame_settings.find(ANDROID_CONTROL_AF_REGIONS).count == 5) {
                for (i = 0; i < 5; i++)
                    af_area[i] = frame_settings.find(ANDROID_CONTROL_AF_REGIONS)
                                     .data.i32[i];

                if (frame_settings.exists(ANDROID_SCALER_CROP_REGION)) {
                    cnt = frame_settings.find(ANDROID_SCALER_CROP_REGION).count;
                    if (cnt > 5) {
                        cnt = 5;
                    }
                    for (i = 0; i < cnt; i++)
                        crop_area[i] =
                            frame_settings.find(ANDROID_SCALER_CROP_REGION)
                                .data.i32[i];

                    crop_area[2] = crop_area[2] + crop_area[0];
                    crop_area[3] = crop_area[3] + crop_area[1];
                }

                ret = checkROIValid(af_area, crop_area);
                if (ret) {
                    HAL_LOGE("af region invalid");
                } else {
                    af_area[2] = af_area[2] - af_area[0];
                    af_area[3] = af_area[3] - af_area[1];

                    if (frame_settings.exists(ANDROID_CONTROL_AF_TRIGGER)) {
                        af_triger =
                            frame_settings.find(ANDROID_CONTROL_AF_TRIGGER)
                                .data.u8[0];
                    }

                    if (af_triger == ANDROID_CONTROL_AF_TRIGGER_CANCEL) {
                        HAL_LOGD("cancel auto focus dont set af region");
                    } else {
                        for (i = 0; i < 5; i++)
                            s_setting[mCameraId].controlInfo.af_regions[i] =
                                af_area[i];
                    }
                }
            }
        }
    }

    // AF_CONTROL
    if (frame_settings.exists(ANDROID_CONTROL_AF_TRIGGER)) {
        valueU8 = frame_settings.find(ANDROID_CONTROL_AF_TRIGGER).data.u8[0];
        s_setting[mCameraId].controlInfo.af_trigger = valueU8;
        pushAndroidParaTag(ANDROID_CONTROL_AF_TRIGGER);
    }
    if (frame_settings.exists(ANDROID_CONTROL_AF_TRIGGER_ID)) {
        s_setting[mCameraId].controlInfo.af_trigger_Id =
            frame_settings.find(ANDROID_CONTROL_AF_TRIGGER_ID).data.i32[0];
        pushAndroidParaTag(ANDROID_CONTROL_AF_TRIGGER_ID);
    }
    if (frame_settings.exists(ANDROID_CONTROL_AF_MODE)) {
        valueU8 = frame_settings.find(ANDROID_CONTROL_AF_MODE).data.u8[0];
        GET_VALUE_IF_DIF(s_setting[mCameraId].controlInfo.af_mode, valueU8,
                         ANDROID_CONTROL_AF_MODE)
    }
    if (frame_settings.exists(ANDROID_CONTROL_AF_STATE)) {
        valueU8 = frame_settings.find(ANDROID_CONTROL_AF_STATE).data.u8[0];
        GET_VALUE_IF_DIF(s_setting[mCameraId].controlInfo.af_state, valueU8,
                         ANDROID_CONTROL_AF_STATE)
    }

    if (frame_settings.exists(ANDROID_STATISTICS_FACE_DETECT_MODE)) {
        valueU8 =
            frame_settings.find(ANDROID_STATISTICS_FACE_DETECT_MODE).data.u8[0];
        s_setting[mCameraId].statisticsInfo.face_detect_mode = valueU8;
        mFaceDetectModeSet = valueU8;
        pushAndroidParaTag(ANDROID_STATISTICS_FACE_DETECT_MODE);
        HAL_LOGV("fd mode %d", valueU8);
    }
    if (frame_settings.exists(ANDROID_SPRD_CONTROL_REFOCUS_ENABLE)) {
        valueU8 =
            frame_settings.find(ANDROID_SPRD_CONTROL_REFOCUS_ENABLE).data.u8[0];
        s_setting[mCameraId].sprddefInfo.refocus_enable = valueU8;
        pushAndroidParaTag(ANDROID_SPRD_CONTROL_REFOCUS_ENABLE);
        HAL_LOGD("camera id %d, refocus mode %d", mCameraId, valueU8);
    }
    if (frame_settings.exists(ANDROID_SPRD_SET_TOUCH_INFO)) {
        s_setting[mCameraId].sprddefInfo.touchxy[0] =
            frame_settings.find(ANDROID_SPRD_SET_TOUCH_INFO).data.i32[0];
        s_setting[mCameraId].sprddefInfo.touchxy[1] =
            frame_settings.find(ANDROID_SPRD_SET_TOUCH_INFO).data.i32[1];
        pushAndroidParaTag(ANDROID_SPRD_SET_TOUCH_INFO);
        HAL_LOGD("touch info %d %d",
                 s_setting[mCameraId].sprddefInfo.touchxy[0],
                 s_setting[mCameraId].sprddefInfo.touchxy[1]);
    }
    if (frame_settings.exists(ANDROID_SPRD_PIPVIV_ENABLED)) {
        s_setting[mCameraId].sprddefInfo.sprd_pipviv_enabled =
            frame_settings.find(ANDROID_SPRD_PIPVIV_ENABLED).data.u8[0];
        pushAndroidParaTag(ANDROID_SPRD_PIPVIV_ENABLED);
        HAL_LOGD("sprd pipviv enabled is %d",
                 s_setting[mCameraId].sprddefInfo.sprd_pipviv_enabled);
    }
    if (frame_settings.exists(ANDROID_SPRD_3DNR_ENABLED)) {
        s_setting[mCameraId].sprddefInfo.sprd_3dnr_enabled =
            frame_settings.find(ANDROID_SPRD_3DNR_ENABLED).data.u8[0];
        pushAndroidParaTag(ANDROID_SPRD_3DNR_ENABLED);
        HAL_LOGV("sprd 3dnr enabled is %d",
                 s_setting[mCameraId].sprddefInfo.sprd_3dnr_enabled);
    }
    if (frame_settings.exists(ANDROID_SPRD_AUTO_HDR_ENABLED)) {
        s_setting[mCameraId].sprddefInfo.sprd_auto_hdr_enable =
            frame_settings.find(ANDROID_SPRD_AUTO_HDR_ENABLED).data.u8[0];
        pushAndroidParaTag(ANDROID_SPRD_AUTO_HDR_ENABLED);
        HAL_LOGV("sprd auto hdr enabled is %d",
                 s_setting[mCameraId].sprddefInfo.sprd_auto_hdr_enable);
    }
    if (frame_settings.exists(ANDROID_SPRD_FLASH_LCD_MODE)) {
        if (s_setting[mCameraId].flash_InfoInfo.available == 0 &&
            !strcmp(FRONT_CAMERA_FLASH_TYPE, "lcd") &&
            cameraInfo.facing == CAMERA_FACING_FRONT) {
            s_setting[mCameraId].sprddefInfo.sprd_flash_lcd_mode =
                frame_settings.find(ANDROID_SPRD_FLASH_LCD_MODE).data.u8[0];
            pushAndroidParaTag(ANDROID_SPRD_FLASH_LCD_MODE);
            HAL_LOGV("flash lcd mode %d",
                     s_setting[mCameraId].sprddefInfo.sprd_flash_lcd_mode);
        }
    }

    if (frame_settings.exists(ANDROID_SPRD_LOGOWATERMARK_ENABLED)) {
        valueU8 =
            frame_settings.find(ANDROID_SPRD_LOGOWATERMARK_ENABLED).data.u8[0];
        GET_VALUE_IF_DIF(
            s_setting[mCameraId].sprddefInfo.sprd_is_logo_watermark, valueU8,
            ANDROID_SPRD_LOGOWATERMARK_ENABLED);
    }

    if (frame_settings.exists(ANDROID_SPRD_TIMEWATERMARK_ENABLED)) {
        valueU8 =
            frame_settings.find(ANDROID_SPRD_TIMEWATERMARK_ENABLED).data.u8[0];
        GET_VALUE_IF_DIF(
            s_setting[mCameraId].sprddefInfo.sprd_is_time_watermark, valueU8,
            ANDROID_SPRD_TIMEWATERMARK_ENABLED);
    }

    HAL_LOGD(
        "isFaceBeautyOn=%d, eis=%d, flash_mode=%d, ae_lock=%d, "
        "scene_mode=%d, cap_mode=%d, cap_cnt=%d, iso=%d, jpeg orien=%d, "
        "zsl=%d, 3dcali=%d, crop %d %d %d %d cropRegionUpdate=%d, "
        "am_mode=%d, updateAE=%d, ae_regions: %d %d %d %d %d, "
        "af_trigger=%d, af_mode=%d, af_state=%d, af_region: %d %d %d %d %d",
        isFaceBeautyOn(s_setting[mCameraId].sprddefInfo),
        s_setting[mCameraId].sprddefInfo.sprd_eis_enabled,
        s_setting[mCameraId].flashInfo.mode,
        s_setting[mCameraId].controlInfo.ae_lock,
        s_setting[mCameraId].controlInfo.scene_mode,
        s_setting[mCameraId].sprddefInfo.capture_mode,
        s_setting[mCameraId].sprddefInfo.burst_cap_cnt,
        s_setting[mCameraId].sprddefInfo.iso,
        s_setting[mCameraId].jpgInfo.orientation_original,
        s_setting[mCameraId].sprddefInfo.sprd_zsl_enabled,
        s_setting[mCameraId].sprddefInfo.sprd_3dcalibration_enabled,
        s_setting[mCameraId].scalerInfo.crop_region[0],
        s_setting[mCameraId].scalerInfo.crop_region[1],
        s_setting[mCameraId].scalerInfo.crop_region[2],
        s_setting[mCameraId].scalerInfo.crop_region[3], cropRegionUpdate,
        s_setting[mCameraId].sprddefInfo.am_mode, updateAE,
        s_setting[mCameraId].controlInfo.ae_regions[0],
        s_setting[mCameraId].controlInfo.ae_regions[1],
        s_setting[mCameraId].controlInfo.ae_regions[2],
        s_setting[mCameraId].controlInfo.ae_regions[3],
        s_setting[mCameraId].controlInfo.ae_regions[4],
        s_setting[mCameraId].controlInfo.af_trigger,
        s_setting[mCameraId].controlInfo.af_mode,
        s_setting[mCameraId].controlInfo.af_state,
        s_setting[mCameraId].controlInfo.af_regions[0],
        s_setting[mCameraId].controlInfo.af_regions[1],
        s_setting[mCameraId].controlInfo.af_regions[2],
        s_setting[mCameraId].controlInfo.af_regions[3],
        s_setting[mCameraId].controlInfo.af_regions[4]);

#undef GET_VALUE_IF_DIF
    return rc;
}

int SprdCamera3Setting::checkROIValid(int32_t *roi_area, int32_t *crop_area) {
    int ret = NO_ERROR;

    if (roi_area == NULL || crop_area == NULL) {
        HAL_LOGE("roi_area=%p,crop_area=%p", roi_area, crop_area);
        goto exit;
    }

    if (crop_area[0] == 0 && crop_area[1] == 0 && crop_area[2] == 0 &&
        crop_area[3] == 0) {
        HAL_LOGV("crop area is zero, dont check roi_area");
        goto exit;
    }

    if (roi_area[2] < crop_area[0] || roi_area[0] > crop_area[2] ||
        roi_area[3] < crop_area[1] || roi_area[1] > crop_area[3]) {
        HAL_LOGE(
            "roi_area invalid, roi_area: %d %d %d %d, crop_area: %d %d %d %d",
            roi_area[0], roi_area[1], roi_area[2], roi_area[3], crop_area[0],
            crop_area[1], crop_area[2], crop_area[3]);
        ret = -1;
        goto exit;
    }

    if (roi_area[0] <= crop_area[0])
        roi_area[0] = crop_area[0];
    if (roi_area[1] <= crop_area[1])
        roi_area[1] = crop_area[1];
    if (roi_area[2] >= crop_area[2])
        roi_area[2] = crop_area[2];
    if (roi_area[3] >= crop_area[3])
        roi_area[3] = crop_area[3];

exit:
    return ret;
}

void SprdCamera3Setting::convertToRegions(int32_t *rect, int32_t *region,
                                          int weight) {
    region[0] = rect[0];
    region[1] = rect[1];
    region[2] = rect[2];
    region[3] = rect[3];
    if (weight > -1) {
        region[4] = weight;
    }
}

camera_metadata_t *SprdCamera3Setting::translateLocalToFwMetadata() {
    CameraMetadata camMetadata;
    camera_metadata_t *resultMetadata;
    int area[5];
    Mutex::Autolock l(mLock);
    int32_t maxRegionsAe = 0;
    int32_t maxRegionsAwb = 0;
    int32_t maxRegionsAf = 0;
    maxRegionsAe = s_setting[mCameraId].controlInfo.max_regions[0];
    maxRegionsAwb = s_setting[mCameraId].controlInfo.max_regions[1];
    maxRegionsAf = s_setting[mCameraId].controlInfo.max_regions[2];

    // HAL_LOGD("timestamp = %lld, request_id = %d, frame_count = %d, mCameraId
    // = %d",s_setting[mCameraId].sensorInfo.timestamp,
    //			s_setting[mCameraId].requestInfo.id,
    // s_setting[mCameraId].requestInfo.frame_count, mCameraId);
    camMetadata.update(ANDROID_CONTROL_ENABLE_ZSL,
                       &(s_setting[mCameraId].controlInfo.enable_zsl), 1);

    camMetadata.update(ANDROID_SENSOR_TIMESTAMP,
                       &(s_setting[mCameraId].resultInfo.timestamp), 1);
    camMetadata.update(ANDROID_SENSOR_TEST_PATTERN_MODE,
                       &(s_setting[mCameraId].sensorInfo.test_pattern_mode), 1);
    camMetadata.update(ANDROID_SENSOR_ROLLING_SHUTTER_SKEW,
                       &(s_setting[mCameraId].sensorInfo.rollingShutterSkew),
                       1);
    camMetadata.update(ANDROID_REQUEST_ID,
                       &(s_setting[mCameraId].requestInfo.id), 1);
    camMetadata.update(ANDROID_REQUEST_FRAME_COUNT,
                       &(s_setting[mCameraId].requestInfo.frame_count), 1);
    camMetadata.update(ANDROID_SHADING_MODE,
                       &(s_setting[mCameraId].shadingInfo.mode), 1);

    if (s_setting[mCameraId].resultInfo.af_trigger !=
        ANDROID_CONTROL_AF_TRIGGER_IDLE)
        s_setting[mCameraId].resultInfo.af_state =
            s_setting[mCameraId].controlInfo.af_state;

    if (s_setting[mCameraId].resultInfo.ae_precap_trigger !=
            ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER_IDLE ||
        s_setting[mCameraId].resultInfo.ae_manual_trigger) {
        s_setting[mCameraId].resultInfo.ae_state =
            s_setting[mCameraId].controlInfo.ae_state;
    } else if ((s_setting[mCameraId].controlInfo.ae_state ==
                ANDROID_CONTROL_AE_STATE_CONVERGED) &&
               (s_setting[mCameraId].resultInfo.ae_precap_trigger ==
                ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER_IDLE)) {
        s_setting[mCameraId].resultInfo.ae_state =
            s_setting[mCameraId].controlInfo.ae_state;
    }

    // HAL_LOGD("af_state = %d, af_mode = %d, af_trigger_Id = %d, mCameraId =
    // %d",s_setting[mCameraId].controlInfo.af_state,
    //			s_setting[mCameraId].controlInfo.af_mode,
    // s_setting[mCameraId].controlInfo.af_trigger_Id, mCameraId);
    camMetadata.update(ANDROID_CONTROL_AF_STATE,
                       &(s_setting[mCameraId].resultInfo.af_state), 1);
    camMetadata.update(ANDROID_CONTROL_AF_MODE,
                       &(s_setting[mCameraId].controlInfo.af_mode), 1);
    camMetadata.update(ANDROID_CONTROL_AF_TRIGGER_ID,
                       &(s_setting[mCameraId].controlInfo.af_trigger_Id), 1);
    camMetadata.update(ANDROID_CONTROL_SCENE_MODE,
                       &(s_setting[mCameraId].controlInfo.scene_mode), 1);
    camMetadata.update(ANDROID_CONTROL_MODE,
                       &(s_setting[mCameraId].controlInfo.mode), 1);
    camMetadata.update(ANDROID_CONTROL_AE_ANTIBANDING_MODE,
                       &(s_setting[mCameraId].controlInfo.ae_abtibanding_mode),
                       1);
    camMetadata.update(
        ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION,
        &(s_setting[mCameraId].controlInfo.ae_exposure_compensation), 1);
    camMetadata.update(ANDROID_CONTROL_AE_MODE,
                       &(s_setting[mCameraId].controlInfo.ae_mode), 1);
    camMetadata.update(
        ANDROID_CONTROL_AE_TARGET_FPS_RANGE,
        s_setting[mCameraId].controlInfo.ae_target_fps_range,
        ARRAY_SIZE(s_setting[mCameraId].controlInfo.ae_target_fps_range));
    camMetadata.update(ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER,
                       &(s_setting[mCameraId].resultInfo.ae_precap_trigger), 1);
    /*for (int i = 0; i < 5; i++)
            area[i] = s_setting[mCameraId].controlInfo.af_regions[i];
    area[2] += area[0];
    area[3] += area[1];
    camMetadata.update(ANDROID_CONTROL_AF_REGIONS, area, ARRAY_SIZE(area));*/
    camMetadata.update(ANDROID_CONTROL_AF_TRIGGER,
                       &(s_setting[mCameraId].resultInfo.af_trigger), 1);
    camMetadata.update(ANDROID_CONTROL_AWB_LOCK,
                       &(s_setting[mCameraId].controlInfo.awb_lock), 1);
    camMetadata.update(ANDROID_SCALER_CROP_REGION,
                       s_setting[mCameraId].scalerInfo.crop_region,
                       ARRAY_SIZE(s_setting[mCameraId].scalerInfo.crop_region));
    // HAL_LOGD(" CROP_REGIONS :%d %d %d
    // %d",s_setting[mCameraId].scalerInfo.crop_region[0],s_setting[mCameraId].scalerInfo.crop_region[1],
    //			s_setting[mCameraId].scalerInfo.crop_region[2],s_setting[mCameraId].scalerInfo.crop_region[3]);

    {
        if (mSensorFocusEnable[mCameraId]) {
            if (maxRegionsAf > 0) {
                area[0] = s_setting[mCameraId]
                              .metaInfo.af_regions[0]; // for cts testAfRegions
                area[1] = s_setting[mCameraId].metaInfo.af_regions[1];
                area[2] = s_setting[mCameraId].metaInfo.af_regions[2];
                area[3] = s_setting[mCameraId].metaInfo.af_regions[3];
                area[4] = s_setting[mCameraId].metaInfo.af_regions[4];
                area[2] += area[0];
                area[3] += area[1];
                HAL_LOGV("AF_REGIONS, area %d %d %d %d %d", area[0], area[1],
                         area[2], area[3], area[4]);

                camMetadata.update(ANDROID_CONTROL_AF_REGIONS, area, 5);
            }
        }

        /* AeRegions */
        if (maxRegionsAe > 0) {
            area[0] = s_setting[mCameraId]
                          .metaInfo.ae_regions[0]; // for cts testAeRegions
            area[1] = s_setting[mCameraId].metaInfo.ae_regions[1];
            area[2] = s_setting[mCameraId].metaInfo.ae_regions[2];
            area[3] = s_setting[mCameraId].metaInfo.ae_regions[3];
            area[4] = s_setting[mCameraId].metaInfo.ae_regions[4];
            HAL_LOGV("AE_REGIONS, area %d %d %d %d %d", area[0], area[1],
                     area[2], area[3], area[4]);
            camMetadata.update(ANDROID_CONTROL_AE_REGIONS, area, 5);
        }
    }
    /*CMR_LOGI("%d %d %d %d %d", s_setting[mCameraId].controlInfo.ae_regions[0],
            s_setting[mCameraId].controlInfo.ae_regions[1],
            s_setting[mCameraId].controlInfo.ae_regions[2],
            s_setting[mCameraId].controlInfo.ae_regions[3],
            s_setting[mCameraId].controlInfo.ae_regions[4]);
    camMetadata.update(ANDROID_CONTROL_AWB_REGIONS, area, 5);*/
    camMetadata.update(ANDROID_CONTROL_CAPTURE_INTENT,
                       &(s_setting[mCameraId].controlInfo.capture_intent), 1);
    camMetadata.update(ANDROID_CONTROL_EFFECT_MODE,
                       &(s_setting[mCameraId].controlInfo.effect_mode), 1);
    camMetadata.update(
        ANDROID_CONTROL_VIDEO_STABILIZATION_MODE,
        &(s_setting[mCameraId].controlInfo.video_stabilization_mode), 1);
    camMetadata.update(ANDROID_HOT_PIXEL_MODE,
                       &(s_setting[mCameraId].hotpixerInfo.mode), 1);
    camMetadata.update(ANDROID_LENS_STATE,
                       &(s_setting[mCameraId].lensInfo.state), 1);
    camMetadata.update(ANDROID_LENS_FILTER_DENSITY,
                       &(s_setting[mCameraId].lensInfo.filter_density), 1);
    camMetadata.update(ANDROID_LENS_FOCUS_DISTANCE,
                       &(s_setting[mCameraId].lensInfo.focus_distance), 1);
    camMetadata.update(
        ANDROID_LENS_OPTICAL_STABILIZATION_MODE,
        &(s_setting[mCameraId].lensInfo.optical_stabilization_mode), 1);
    camMetadata.update(ANDROID_LENS_FOCUS_RANGE,
                       (s_setting[mCameraId].lensInfo.focus_range),
                       ARRAY_SIZE(s_setting[mCameraId].lensInfo.focus_range));

    if ((s_setting[mCameraId].metaInfo.flash_mode))
        s_setting[mCameraId].flashInfo.state = ANDROID_FLASH_STATE_FIRED;
    else {
        // s_setting[mCameraId].flashInfo.state = mCameraId == 0 ?
        // ANDROID_FLASH_STATE_READY : ANDROID_FLASH_STATE_UNAVAILABLE;
        if (mCameraId == 0) {
            s_setting[mCameraId].flashInfo.state = ANDROID_FLASH_STATE_READY;
        } else if (mCameraId == 1) {
            if (!strcmp(FRONT_CAMERA_FLASH_TYPE, "none") ||
                !strcmp(FRONT_CAMERA_FLASH_TYPE, "lcd"))
                s_setting[mCameraId].flashInfo.state =
                    ANDROID_FLASH_STATE_UNAVAILABLE;
            else
                s_setting[mCameraId].flashInfo.state =
                    ANDROID_FLASH_STATE_READY;
        } else {
            s_setting[mCameraId].flashInfo.state =
                ANDROID_FLASH_STATE_UNAVAILABLE;
        }
    }
    camMetadata.update(ANDROID_FLASH_STATE,
                       &(s_setting[mCameraId].flashInfo.state), 1);
    camMetadata.update(ANDROID_FLASH_MODE,
                       &(s_setting[mCameraId].metaInfo.flash_mode), 1);
    camMetadata.update(ANDROID_EDGE_MODE, &(s_setting[mCameraId].edgeInfo.mode),
                       1);
    camMetadata.update(ANDROID_REQUEST_PIPELINE_DEPTH,
                       &(s_setting[mCameraId].requestInfo.pipeline_depth), 1);
    if (s_setting[mCameraId].controlInfo.ae_state ==
            ANDROID_CONTROL_AE_STATE_LOCKED &&
        s_setting[mCameraId].controlInfo.ae_comp_effect_frames_cnt != 0 &&
        s_setting[mCameraId].controlInfo.ae_lock) {
        s_setting[mCameraId].resultInfo.ae_state =
            ANDROID_CONTROL_AE_STATE_SEARCHING;
        s_setting[mCameraId].controlInfo.ae_comp_effect_frames_cnt--;
    }
    camMetadata.update(ANDROID_CONTROL_AE_STATE,
                       &(s_setting[mCameraId].resultInfo.ae_state), 1);
    HAL_LOGV("result ae_state=%d", s_setting[mCameraId].resultInfo.ae_state);
    // HAL_LOGD("ae sta=%d precap id=%d",
    // s_setting[mCameraId].controlInfo.ae_state,
    //			s_setting[mCameraId].controlInfo.ae_precapture_id);
    camMetadata.update(ANDROID_CONTROL_AE_PRECAPTURE_ID,
                       &(s_setting[mCameraId].controlInfo.ae_precapture_id), 1);
    // Update ANDROID_SPRD_AE_INFO
    camMetadata.update(ANDROID_SPRD_AE_INFO,
                       s_setting[mCameraId].sprddefInfo.ae_info,
                       AE_CB_MAX_INDEX - 1);
    camMetadata.update(ANDROID_CONTROL_AE_LOCK,
                       &(s_setting[mCameraId].controlInfo.ae_lock), 1);

    // s_setting[mCameraId].controlInfo.awb_mode =
    // ANDROID_CONTROL_AWB_MODE_AUTO;
    camMetadata.update(ANDROID_CONTROL_AWB_MODE,
                       &(s_setting[mCameraId].controlInfo.awb_mode), 1);
    // s_setting[mCameraId].controlInfo.awb_state =
    // ANDROID_CONTROL_AWB_STATE_INACTIVE;
    camMetadata.update(ANDROID_CONTROL_AWB_STATE,
                       &(s_setting[mCameraId].controlInfo.awb_state), 1);
    // Update ANDROID_SPRD_DEVICE_ORIENTATION
    camMetadata.update(ANDROID_SPRD_DEVICE_ORIENTATION,
                       &(s_setting[mCameraId].sprddefInfo.device_orietation),
                       1);
    uint8_t hardware_level_limited = 0;
    if (s_setting[mCameraId].supported_hardware_level ==
        ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LIMITED)
        hardware_level_limited = 1;
    if ((s_setting[mCameraId].jpgInfo.orientation_original == 90 ||
         s_setting[mCameraId].jpgInfo.orientation_original == 270) &&
        (hardware_level_limited)) {
        int32_t rotated_thumbnail_size[2];
        rotated_thumbnail_size[0] =
            s_setting[mCameraId].jpgInfo.thumbnail_size[1];
        rotated_thumbnail_size[1] =
            s_setting[mCameraId].jpgInfo.thumbnail_size[0];
        camMetadata.update(ANDROID_JPEG_THUMBNAIL_SIZE, rotated_thumbnail_size,
                           2);
    } else {
        camMetadata.update(ANDROID_JPEG_THUMBNAIL_SIZE,
                           s_setting[mCameraId].jpgInfo.thumbnail_size, 2);
    }
    camMetadata.update(ANDROID_JPEG_ORIENTATION,
                       &(s_setting[mCameraId].jpgInfo.orientation_original), 1);
    camMetadata.update(ANDROID_JPEG_QUALITY,
                       &(s_setting[mCameraId].jpgInfo.quality), 1);
    camMetadata.update(ANDROID_JPEG_THUMBNAIL_QUALITY,
                       &(s_setting[mCameraId].jpgInfo.thumbnail_quality), 1);
    camMetadata.update(ANDROID_JPEG_GPS_COORDINATES,
                       s_setting[mCameraId].jpgInfo.gps_coordinates, 3);
    camMetadata.update(ANDROID_JPEG_GPS_PROCESSING_METHOD,
                       s_setting[mCameraId].jpgInfo.gps_processing_method, 36);
    camMetadata.update(ANDROID_JPEG_GPS_TIMESTAMP,
                       &(s_setting[mCameraId].jpgInfo.gps_timestamp), 1);
    camMetadata.update(ANDROID_LENS_FOCAL_LENGTH,
                       &(s_setting[mCameraId].lensInfo.focal_length), 1);
    camMetadata.update(ANDROID_LENS_APERTURE,
                       &(s_setting[mCameraId].lensInfo.aperture), 1);
    camMetadata.update(ANDROID_COLOR_CORRECTION_GAINS,
                       s_setting[mCameraId].colorInfo.gains,
                       ARRAY_SIZE(s_setting[mCameraId].colorInfo.gains));
    camMetadata.update(ANDROID_COLOR_CORRECTION_MODE,
                       &(s_setting[mCameraId].colorInfo.correction_mode), 1);
    camMetadata.update(ANDROID_COLOR_CORRECTION_ABERRATION_MODE,
                       &(s_setting[mCameraId].colorInfo.aberration_mode), 1);
    camMetadata.update(ANDROID_COLOR_CORRECTION_TRANSFORM,
                       s_setting[mCameraId].colorInfo.transform,
                       ARRAY_SIZE(s_setting[mCameraId].colorInfo.transform));
    camMetadata.update(ANDROID_TONEMAP_MODE,
                       &(s_setting[mCameraId].toneInfo.mode), 1);
    camMetadata.update(ANDROID_BLACK_LEVEL_LOCK,
                       &(s_setting[mCameraId].black_level_lock), 1);
    camMetadata.update(ANDROID_NOISE_REDUCTION_MODE,
                       &(s_setting[mCameraId].noiseInfo.reduction_mode), 1);
    camMetadata.update(
        ANDROID_STATISTICS_HOT_PIXEL_MAP_MODE,
        &(s_setting[mCameraId].statisticsInfo.hot_pixel_map_mode), 1);
    camMetadata.update(ANDROID_STATISTICS_SCENE_FLICKER,
                       &(s_setting[mCameraId].statisticsInfo.scene_flicker), 1);
    camMetadata.update(
        ANDROID_STATISTICS_LENS_SHADING_MAP_MODE,
        &(s_setting[mCameraId].statisticsInfo.lens_shading_map_mode), 1);
    // s_setting[mCameraId].statisticsInfo.face_detect_mode =
    // ANDROID_STATISTICS_FACE_DETECT_MODE_OFF;
    camMetadata.update(ANDROID_SPRD_EIS_ENABLED,
                       &(s_setting[mCameraId].sprddefInfo.sprd_eis_enabled), 1);
    camMetadata.update(ANDROID_STATISTICS_HISTOGRAM,
                       s_setting[mCameraId].hist_report, CAMERA_ISP_HIST_ITEMS);
    if (ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_RECORD ==
            s_setting[mCameraId].controlInfo.capture_intent &&
        s_setting[mCameraId].sprddefInfo.sprd_eis_enabled) {
        int32_t eisCrop[4];
        eisCrop[0] = s_setting[mCameraId].eiscrop_Info.crop[0];
        eisCrop[1] = s_setting[mCameraId].eiscrop_Info.crop[1];
        eisCrop[2] = s_setting[mCameraId].eiscrop_Info.crop[2];
        eisCrop[3] = s_setting[mCameraId].eiscrop_Info.crop[3];
        HAL_LOGV("eis crop:[%d, %d, %d, %d]", eisCrop[0], eisCrop[1],
                 eisCrop[2], eisCrop[3]);
        camMetadata.update(ANDROID_SPRD_EIS_CROP, eisCrop, 4);
    }
    camMetadata.update(ANDROID_STATISTICS_FACE_DETECT_MODE,
                       &(s_setting[mCameraId].statisticsInfo.face_detect_mode),
                       1);
    if (ANDROID_STATISTICS_FACE_DETECT_MODE_OFF !=
            (s_setting[mCameraId].statisticsInfo.face_detect_mode) ||
        (isFaceBeautyOn(s_setting[mCameraId].sprddefInfo))) {
#define MAX_ROI 10
        FACE_Tag *faceDetectionInfo =
            (FACE_Tag *)&(s_setting[mCameraId].faceInfo);
        uint8_t numFaces = faceDetectionInfo->face_num;
        int32_t faceIds[MAX_ROI];
        uint8_t faceScores[MAX_ROI];
        int32_t faceRectangles[MAX_ROI * 4];
        int32_t faceLandmarks[MAX_ROI * 6];
        uint8_t dataSize = 1;
        int j = 0;
        for (int i = 0; i < numFaces; i++) {
            faceIds[i] = faceDetectionInfo->face[i].id;
            faceScores[i] = faceDetectionInfo->face[i].score;
            convertToRegions(faceDetectionInfo->face[i].rect,
                             faceRectangles + j, -1);
            j += 4;
        }
        if (numFaces <= 0) {
            memset(faceIds, 0, sizeof(int32_t) * MAX_ROI);
            memset(faceScores, 0, sizeof(uint8_t) * MAX_ROI);
            memset(faceRectangles, 0, sizeof(int32_t) * MAX_ROI * 4);
            memset(faceLandmarks, 0, sizeof(int32_t) * MAX_ROI * 6);
        }
        if (numFaces >= 1) {
            dataSize = numFaces;
        }
        camMetadata.update(ANDROID_STATISTICS_FACE_IDS, faceIds, dataSize);
        camMetadata.update(ANDROID_STATISTICS_FACE_SCORES, faceScores,
                           dataSize);
        camMetadata.update(ANDROID_STATISTICS_FACE_RECTANGLES, faceRectangles,
                           dataSize * 4);
        camMetadata.update(ANDROID_SPRD_FACE_INFO,
                           (uint8_t *)&(s_setting[mCameraId].orifaceInfo),
                           sizeof(FACE_Tag));

        if (camMetadata.exists(ANDROID_STATISTICS_FACE_RECTANGLES)) {

            int32_t g_face_info1 =
                camMetadata.find(ANDROID_STATISTICS_FACE_RECTANGLES)
                    .data.i32[0];
            int32_t g_face_info2 =
                camMetadata.find(ANDROID_STATISTICS_FACE_RECTANGLES)
                    .data.i32[1];
            int32_t g_face_info3 =
                camMetadata.find(ANDROID_STATISTICS_FACE_RECTANGLES)
                    .data.i32[2];
            int32_t g_face_info4 =
                camMetadata.find(ANDROID_STATISTICS_FACE_RECTANGLES)
                    .data.i32[3];
            HAL_LOGV("id%d:face sx %d sy %d ex %d ey %d", mCameraId,
                     g_face_info1, g_face_info2, g_face_info3, g_face_info4);
        }
        // hangcheng note:have to remove this memset,
        // due to it would face crop can not show when face detect lib cost time
        // large than frame rate time.
        // memset remove to SprdCamera3OEMIf::receivePreviewFDFrame function
        // memset(faceDetectionInfo,0,sizeof(FACE_Tag));
    }

    // perfect_level
    camMetadata.update(ANDROID_SPRD_UCAM_SKIN_LEVEL,
                       s_setting[mCameraId].sprddefInfo.perfect_skin_level,
                       SPRD_FACE_BEAUTY_PARAM_NUM);
    // sensor
    if (s_setting[mCameraId].controlInfo.ae_target_fps_range[1])
        s_setting[mCameraId].sensorInfo.frame_duration =
            (int64_t)(NSEC_PER_SEC /
                      s_setting[mCameraId].controlInfo.ae_target_fps_range[1]);
    camMetadata.update(ANDROID_SENSOR_FRAME_DURATION,
                       &(s_setting[mCameraId].sensorInfo.frame_duration), 1);

    // s_setting[mCameraId].sensorInfo.exposure_time =
    // (int64_t)(NSEC_PER_SEC/s_setting[mCameraId].controlInfo.ae_target_fps_range[1]);
    HAL_LOGV("sensorInfo.exposure_time is upload = %lld",
             s_setting[mCameraId].sensorInfo.exposure_time);
    camMetadata.update(ANDROID_SENSOR_EXPOSURE_TIME,
                       &(s_setting[mCameraId].sensorInfo.exposure_time), 1);
    camMetadata.update(ANDROID_SENSOR_SENSITIVITY,
                       &(s_setting[mCameraId].sensorInfo.sensitivity), 1);
    camMetadata.update(ANDROID_STATISTICS_LENS_SHADING_CORRECTION_MAP,
                       &(s_setting[mCameraId].shadingInfo.factor_count), 1);
    camMetadata.update(ANDROID_STATISTICS_LENS_SHADING_MAP,
                       s_setting[mCameraId].shadingInfo.gain_factor,
                       SPRD_SHADING_FACTOR_NUM);
    camMetadata.update(ANDROID_TONEMAP_CURVE_BLUE,
                       s_setting[mCameraId].toneInfo.curve_blue,
                       SPRD_MAX_TONE_CURVE_POINT);
    camMetadata.update(ANDROID_TONEMAP_CURVE_GREEN,
                       s_setting[mCameraId].toneInfo.curve_green,
                       SPRD_MAX_TONE_CURVE_POINT);
    camMetadata.update(ANDROID_TONEMAP_CURVE_RED,
                       s_setting[mCameraId].toneInfo.curve_red,
                       SPRD_MAX_TONE_CURVE_POINT);

    if (mCameraId == 0) { // && s_setting[mCameraId].otpInfo.otp_data){ //
                          // "s_setting[mCameraId].otpInfo.otp_data" always
                          // "true"
        camMetadata.update(ANDROID_SPRD_OTP_DATA,
                           s_setting[mCameraId].otpInfo.otp_data,
                           SPRD_DUAL_OTP_SIZE);
        camMetadata.update(ANDROID_SPRD_DUAL_OTP_FLAG,
                           &(s_setting[mCameraId].otpInfo.dual_otp_flag), 1);
    }
    if (mCameraId == 0) {
        camMetadata.update(ANDROID_SPRD_VCM_STEP,
                           &(s_setting[mCameraId].vcmInfo.vcm_step), 1);
    }

    camMetadata.update(
        ANDROID_SPRD_IS_TAKEPICTURE_WITH_FLASH,
        &(s_setting[mCameraId].sprddefInfo.is_takepicture_with_flash), 1);

    HAL_LOGD("auto hdr scene report %d",
             s_setting[mCameraId].sprddefInfo.sprd_is_hdr_scene);
    camMetadata.update(ANDROID_SPRD_IS_HDR_SCENE,
                       &(s_setting[mCameraId].sprddefInfo.sprd_is_hdr_scene),
                       1);

    resultMetadata = camMetadata.release();
    return resultMetadata;
}

int SprdCamera3Setting::setPreviewSize(cam_dimension_t size) {
    s_setting[mCameraId].preview_size = size;

    return 0;
}

int SprdCamera3Setting::getPreviewSize(cam_dimension_t *size) {
    if (size) {
        *size = s_setting[mCameraId].preview_size;
    }
    return 0;
}

int SprdCamera3Setting::setPictureSize(cam_dimension_t size) {
    s_setting[mCameraId].picture_size = size;
    return 0;
}

int SprdCamera3Setting::getPictureSize(cam_dimension_t *size) {
    if (size) {
        *size = s_setting[mCameraId].picture_size;
    }
    return 0;
}

int SprdCamera3Setting::setVideoSize(cam_dimension_t size) {
    s_setting[mCameraId].video_size = size;
    return 0;
}

int SprdCamera3Setting::getVideoSize(cam_dimension_t *size) {
    if (size) {
        *size = s_setting[mCameraId].video_size;
    }
    return 0;
}

const char *
SprdCamera3Setting::getVendorSectionName(/*get base type*/
                                         const vendor_tag_query_ops_t *v,
                                         uint32_t tag) {
    uint32_t tag_section = (tag >> 16) - VENDOR_SECTION;

    UNUSED(v);
    if (tag_section >= ANDROID_VENDOR_SECTION_COUNT) {
        return NULL;
    }
    return cam_hal_metadata_section_names[tag_section];
}

int SprdCamera3Setting::getVendorTagType(const vendor_tag_query_ops_t *v,
                                         uint32_t tag) {
    uint32_t tag_section = (tag >> 16) - VENDOR_SECTION;
    uint32_t tag_index = tag & 0xFFFF;

    UNUSED(v);
    if (tag_section >= ANDROID_VENDOR_SECTION_COUNT ||
        tag >= (uint32_t)(cam_hal_metadata_section_bounds[tag_section][1])) {
        return -1;
    }
    return cam_tag_info[tag_section][tag_index].tag_type;
}

const char *
SprdCamera3Setting::getVendorTagName(const vendor_tag_query_ops_t *v,
                                     uint32_t tag) {
    uint32_t tag_section = (tag >> 16) - VENDOR_SECTION;
    uint32_t tag_index = tag & 0xFFFF;

    UNUSED(v);
    if (tag_section >= ANDROID_VENDOR_SECTION_COUNT ||
        tag >= (uint32_t)(cam_hal_metadata_section_bounds[tag_section][1])) {
        return NULL;
    }
    return cam_tag_info[tag_section][tag_index].tag_name;
}

int SprdCamera3Setting::getVendorTagCnt(const vendor_tag_query_ops_t *v) {
    HAL_LOGV("dont' handle");
    UNUSED(v);
    return 0;
}

void SprdCamera3Setting::getVendorTags(const vendor_tag_query_ops_t *v,
                                       uint32_t *tag_array) {
    HAL_LOGV("dont' handle");
    UNUSED(v);
    UNUSED(tag_array);
}

int SprdCamera3Setting::get_tag_count(const vendor_tag_ops_t *ops) {
    UNUSED(ops);
    return (VENDOR_SECTION_END - VENDOR_SECTION_START);
}

void SprdCamera3Setting::get_all_tags(const vendor_tag_ops_t *ops,
                                      uint32_t *tag_array) {
    uint32_t *tag_array_tmp = tag_array;

    UNUSED(ops);
    for (int i = VENDOR_SECTION_START; i < VENDOR_SECTION_END; i++) {
        *tag_array_tmp = i;
        tag_array_tmp++;
    }
}

const char *SprdCamera3Setting::get_section_name(const vendor_tag_ops_t *ops,
                                                 uint32_t tag) {
    uint32_t tag_section = (tag >> 16) - VENDOR_SECTION;

    UNUSED(ops);

    if (tag_section >= ANDROID_VENDOR_SECTION_COUNT) {
        return NULL;
    }
    return cam_hal_metadata_section_names[tag_section];
}

const char *SprdCamera3Setting::get_tag_name(const vendor_tag_ops_t *ops,
                                             uint32_t tag) {
    uint32_t tag_section = (tag >> 16) - VENDOR_SECTION;
    uint32_t tag_index = tag & 0xFFFF;

    UNUSED(ops);
    if (tag_section >= ANDROID_VENDOR_SECTION_COUNT ||
        tag >= (uint32_t)(cam_hal_metadata_section_bounds[tag_section][1])) {
        return NULL;
    }
    return cam_tag_info[tag_section][tag_index].tag_name;
}

int SprdCamera3Setting::get_tag_type(const vendor_tag_ops_t *ops,
                                     uint32_t tag) {
    uint32_t tag_section = (tag >> 16) - VENDOR_SECTION;
    uint32_t tag_index = tag & 0xFFFF;

    UNUSED(ops);
    if (tag_section >=
        ANDROID_VENDOR_SECTION_COUNT) { // modify for coverity 124179
        HAL_LOGE("####get_tag_type: hal %d, tag_section=%x, >= "
                 "ANDROID_VENDOR_SECTION_COUNT\n",
                 __LINE__, tag_section);
        return -1;
    } else if (tag >=
               (uint32_t)(cam_hal_metadata_section_bounds[tag_section][1])) {
        return -1;
    }
    return cam_tag_info[tag_section][tag_index].tag_type;
}

int SprdCamera3Setting::setCOLORTag(COLOR_Tag colorInfo) {
    s_setting[mCameraId].colorInfo = colorInfo;
    return 0;
}

int SprdCamera3Setting::getCOLORTag(COLOR_Tag *colorInfo) {
    *colorInfo = s_setting[mCameraId].colorInfo;
    return 0;
}

int SprdCamera3Setting::setLENSTag(LENS_Tag lensInfo) {
    s_setting[mCameraId].lensInfo = lensInfo;
    return 0;
}

int SprdCamera3Setting::getLENSTag(LENS_Tag *lensInfo) {
    *lensInfo = s_setting[mCameraId].lensInfo;
    return 0;
}

int SprdCamera3Setting::setJPEGTag(JPEG_Tag jpgInfo) {
    s_setting[mCameraId].jpgInfo = jpgInfo;
    return 0;
}

int SprdCamera3Setting::getJPEGTag(JPEG_Tag *jpgInfo) {
    *jpgInfo = s_setting[mCameraId].jpgInfo;
    return 0;
}

int SprdCamera3Setting::setLENSINFOTag(LENS_INFO_Tag lens_InfoInfo) {
    s_setting[mCameraId].lens_InfoInfo = lens_InfoInfo;
    return 0;
}

int SprdCamera3Setting::getLENSINFOTag(LENS_INFO_Tag *lens_InfoInfo) {
    *lens_InfoInfo = s_setting[mCameraId].lens_InfoInfo;
    return 0;
}

int SprdCamera3Setting::setSENSORINFOTag(SENSOR_INFO_Tag sensor_InfoInfo) {
    s_setting[mCameraId].sensor_InfoInfo = sensor_InfoInfo;
    return 0;
}

int SprdCamera3Setting::getSENSORINFOTag(SENSOR_INFO_Tag *sensor_InfoInfo) {
    *sensor_InfoInfo = s_setting[mCameraId].sensor_InfoInfo;
    return 0;
}

int SprdCamera3Setting::setFLASHTag(FLASH_Tag flashInfo) {
    s_setting[mCameraId].flashInfo = flashInfo;
    return 0;
}
int SprdCamera3Setting::getFLASHTag(FLASH_Tag *flashInfo) {
    *flashInfo = s_setting[mCameraId].flashInfo;
    return 0;
}

int SprdCamera3Setting::setOTPTag(OTP_Tag *otpInfo) {
    s_setting[mCameraId].otpInfo = *otpInfo;
    return 0;
}
int SprdCamera3Setting::getOTPTag(OTP_Tag *otpInfo) {
    *otpInfo = s_setting[mCameraId].otpInfo;
    return 0;
}

int SprdCamera3Setting::setVCMTag(VCM_Tag vcmInfo) {
    s_setting[mCameraId].vcmInfo = vcmInfo;
    return 0;
}
int SprdCamera3Setting::getVCMTag(VCM_Tag *vcmInfo) {
    *vcmInfo = s_setting[mCameraId].vcmInfo;
    return 0;
}

int SprdCamera3Setting::androidAeModeToDrvAeMode(uint8_t androidAeMode,
                                                 int8_t *convertDrvMode) {
    int ret = 0;

    HAL_LOGD("aeMode %d", androidAeMode);
    switch (androidAeMode) {
    case ANDROID_CONTROL_AE_MODE_OFF:
        *convertDrvMode = -1;
        ret = -1;
        break;

    case ANDROID_CONTROL_AE_MODE_ON:
        *convertDrvMode = CAMERA_FLASH_MODE_OFF;
        break;

    case ANDROID_CONTROL_AE_MODE_ON_AUTO_FLASH:
        *convertDrvMode = CAMERA_FLASH_MODE_AUTO;
        break;

    case ANDROID_CONTROL_AE_MODE_ON_ALWAYS_FLASH:
        *convertDrvMode = CAMERA_FLASH_MODE_ON;
        break;

    default:
        *convertDrvMode = CAMERA_FLASH_MODE_OFF;
        break;
    }
    return ret;
}

int SprdCamera3Setting::androidFlashModeToDrvFlashMode(uint8_t androidFlashMode,
                                                       int8_t *convertDrvMode) {
    int ret = 0;

    HAL_LOGD("flashMode %d", androidFlashMode);

    switch (androidFlashMode) {
    case ANDROID_FLASH_MODE_TORCH:
        *convertDrvMode = CAMERA_FLASH_MODE_TORCH;
        break;

    case ANDROID_FLASH_MODE_OFF:
        *convertDrvMode = CAMERA_FLASH_MODE_OFF;
        break;

    case ANDROID_FLASH_MODE_SINGLE:
        *convertDrvMode = CAMERA_FLASH_MODE_ON;
        break;

    default:
        break;
    }
    return ret;
}

int SprdCamera3Setting::androidAwbModeToDrvAwbMode(uint8_t androidAwbMode,
                                                   int8_t *convertDrvMode) {
    int ret = 0;

    HAL_LOGD("awbMode %d", androidAwbMode);

    switch (androidAwbMode) {
    case ANDROID_CONTROL_AWB_MODE_OFF:
        *convertDrvMode = CAMERA_WB_MAX;
        break;

    case ANDROID_CONTROL_AWB_MODE_AUTO:
        *convertDrvMode = CAMERA_WB_AUTO;
        break;

    case ANDROID_CONTROL_AWB_MODE_INCANDESCENT:
        *convertDrvMode = CAMERA_WB_INCANDESCENT;
        break;

    case ANDROID_CONTROL_AWB_MODE_FLUORESCENT:
        *convertDrvMode = CAMERA_WB_FLUORESCENT;
        break;

    case ANDROID_CONTROL_AWB_MODE_DAYLIGHT:
        *convertDrvMode = CAMERA_WB_DAYLIGHT;
        break;

    case ANDROID_CONTROL_AWB_MODE_CLOUDY_DAYLIGHT:
        *convertDrvMode = CAMERA_WB_CLOUDY_DAYLIGHT;
        break;
    default:
        *convertDrvMode = CAMERA_WB_AUTO;
        break;
    }
    return ret;
}

int SprdCamera3Setting::androidAntibandingModeToDrvAntibandingMode(
    uint8_t androidAntibandingMode, int8_t *convertAntibandingMode) {
    int ret = 0;

    HAL_LOGD("awbMode %d", androidAntibandingMode);
    switch (androidAntibandingMode) {
    case ANDROID_CONTROL_AE_ANTIBANDING_MODE_50HZ:
        *convertAntibandingMode = 0;
        break;
    case ANDROID_CONTROL_AE_ANTIBANDING_MODE_60HZ:
        *convertAntibandingMode = 1;
        break;

    case ANDROID_CONTROL_AE_ANTIBANDING_MODE_AUTO:
        *convertAntibandingMode = 3;
        break;

    default:
        *convertAntibandingMode = 0;
        break;
    }
    return ret;
}

int SprdCamera3Setting::androidAfModeToDrvAfMode(uint8_t androidAfMode,
                                                 int8_t *convertDrvMode) {
    int ret = 0;

    HAL_LOGD("afMode %d", androidAfMode);

    switch (androidAfMode) {
    case ANDROID_CONTROL_AF_MODE_AUTO:
        *convertDrvMode = CAMERA_FOCUS_MODE_AUTO;
        break;

    case ANDROID_CONTROL_AF_MODE_MACRO:
        if (s_setting[mCameraId].sprddefInfo.is_macro_fixed == 0)
            *convertDrvMode = CAMERA_FOCUS_MODE_MACRO;
        else
            *convertDrvMode = CAMERA_FOCUS_MODE_MACRO_FIXED;
        break;

    case ANDROID_CONTROL_AF_MODE_EDOF:
    case ANDROID_CONTROL_AF_MODE_OFF:
        *convertDrvMode = CAMERA_FOCUS_MODE_INFINITY;
        break;

    case ANDROID_CONTROL_AF_MODE_CONTINUOUS_VIDEO:
        *convertDrvMode = CAMERA_FOCUS_MODE_CAF_VIDEO;
        break;

    case ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE:
        *convertDrvMode = CAMERA_FOCUS_MODE_CAF;
        break;

    default:
        *convertDrvMode = CAMERA_FOCUS_MODE_INFINITY;
        break;
    }

    if (!mSensorFocusEnable[mCameraId]) {
        *convertDrvMode = CAMERA_FOCUS_MODE_INFINITY;
    }

    return ret;
}

int SprdCamera3Setting::androidIsoToDrvMode(uint8_t androidIso,
                                            int8_t *convertDrvMode) {
    int ret = 0;

    HAL_LOGD("iso %d", androidIso);
    switch (androidIso) {
    case CAMERA_ISO_AUTO:
        *convertDrvMode = 0;
        break;
    case CAMERA_ISO_100:
        *convertDrvMode = 1;
        break;
    case CAMERA_ISO_200:
        *convertDrvMode = 2;
        break;
    case CAMERA_ISO_400:
        *convertDrvMode = 3;
        break;
    case CAMERA_ISO_800:
        *convertDrvMode = 4;
        break;
    case CAMERA_ISO_1600:
        *convertDrvMode = 5;
        break;
    default:
        *convertDrvMode = 0;
        break;
    }
    return ret;
}
int SprdCamera3Setting::androidSceneModeToDrvMode(uint8_t androidScreneMode,
                                                  int8_t *convertDrvMode) {
    int ret = 0;

    HAL_LOGD("scene %d", androidScreneMode);
    switch (androidScreneMode) {
    case ANDROID_CONTROL_SCENE_MODE_DISABLED:
        *convertDrvMode = CAMERA_SCENE_MODE_AUTO;
        break;

    case ANDROID_CONTROL_SCENE_MODE_ACTION:
        *convertDrvMode = CAMERA_SCENE_MODE_ACTION;
        break;

    case ANDROID_CONTROL_SCENE_MODE_NIGHT:
        *convertDrvMode = CAMERA_SCENE_MODE_NIGHT;
        break;

    case ANDROID_CONTROL_SCENE_MODE_PORTRAIT:
        *convertDrvMode = CAMERA_SCENE_MODE_PORTRAIT;
        break;

    case ANDROID_CONTROL_SCENE_MODE_LANDSCAPE:
        *convertDrvMode = CAMERA_SCENE_MODE_LANDSCAPE;
        break;

    case ANDROID_CONTROL_SCENE_MODE_HDR:
        *convertDrvMode = CAMERA_SCENE_MODE_HDR;
        break;

    default:
        *convertDrvMode = CAMERA_SCENE_MODE_AUTO;
        break;
    }
    return ret;
}

int SprdCamera3Setting::androidEffectModeToDrvMode(uint8_t androidEffectMode,
                                                   int8_t *convertDrvMode) {
    int ret = 0;

    HAL_LOGD("effect %d", androidEffectMode);
    switch (androidEffectMode) {
    case ANDROID_CONTROL_EFFECT_MODE_OFF:
        *convertDrvMode = CAMERA_EFFECT_NONE;
        break;

    case ANDROID_CONTROL_EFFECT_MODE_MONO:
        *convertDrvMode = CAMERA_EFFECT_MONO;
        break;

    case ANDROID_CONTROL_EFFECT_MODE_NEGATIVE:
        *convertDrvMode = CAMERA_EFFECT_NEGATIVE;
        break;

    case ANDROID_CONTROL_EFFECT_MODE_SEPIA:
        *convertDrvMode = CAMERA_EFFECT_SEPIA;
        break;
    case ANDROID_CONTROL_EFFECT_MODE_SOLARIZE:
        *convertDrvMode = CAMERA_EFFECT_YELLOW; // old
        break;
    case ANDROID_CONTROL_EFFECT_MODE_AQUA:
        *convertDrvMode = CAMERA_EFFECT_BLUE; // clod color
        break;
    default:
        *convertDrvMode = CAMERA_EFFECT_NONE;
        break;
    }
    return ret;
}

int SprdCamera3Setting::flashLcdModeToDrvFlashMode(uint8_t flashLcdMode,
                                                   int8_t *convertDrvMode) {
    int ret = 0;

    HAL_LOGD("flash lcd mode %d", flashLcdMode);
    switch (flashLcdMode) {
    case FLASH_LCD_MODE_OFF:
        *convertDrvMode = CAMERA_FLASH_MODE_OFF;
        break;

    case FLASH_LCD_MODE_AUTO:
        *convertDrvMode = CAMERA_FLASH_MODE_AUTO;
        break;

    case FLASH_LCD_MODE_ON:
        *convertDrvMode = CAMERA_FLASH_MODE_ON;
        break;

    default:
        *convertDrvMode = CAMERA_FLASH_MODE_OFF;
        break;
    }
    return ret;
}

int SprdCamera3Setting::setFLASHINFOTag(FLASH_INFO_Tag flash_InfoInfo) {
    s_setting[mCameraId].flash_InfoInfo = flash_InfoInfo;
    return 0;
}

int SprdCamera3Setting::getFLASHINFOTag(FLASH_INFO_Tag *flash_InfoInfo) {
    *flash_InfoInfo = s_setting[mCameraId].flash_InfoInfo;
    return 0;
}

int SprdCamera3Setting::setTONEMAPTag(TONEMAP_Tag *toneInfo) {
    s_setting[mCameraId].toneInfo = *toneInfo;
    return 0;
}

int SprdCamera3Setting::getTONEMAPTag(TONEMAP_Tag *toneInfo) {
    *toneInfo = s_setting[mCameraId].toneInfo;
    return 0;
}

int SprdCamera3Setting::setSTATISTICSINFOTag(
    STATISTICS_INFO_Tag statis_InfoInfo) {
    s_setting[mCameraId].statis_InfoInfo = statis_InfoInfo;
    return 0;
}

int SprdCamera3Setting::getSTATISTICSINFOTag(
    STATISTICS_INFO_Tag *statis_InfoInfo) {
    *statis_InfoInfo = s_setting[mCameraId].statis_InfoInfo;
    return 0;
}

int SprdCamera3Setting::setSCALERTag(SCALER_Tag *scalerInfo) {
    Mutex::Autolock l(mLock);
    s_setting[mCameraId].scalerInfo = *scalerInfo;
    return 0;
}

int SprdCamera3Setting::getSCALERTag(SCALER_Tag *scalerInfo) {
    Mutex::Autolock l(mLock);
    *scalerInfo = s_setting[mCameraId].scalerInfo;
    return 0;
}

int SprdCamera3Setting::setCONTROLTag(CONTROL_Tag *controlInfo) {
    Mutex::Autolock l(mLock);
    s_setting[mCameraId].controlInfo = *controlInfo;
    return 0;
}

int SprdCamera3Setting::setResultTag(CONTROL_Tag *resultInfo) {
    Mutex::Autolock l(mLock);
    s_setting[mCameraId].resultInfo = *resultInfo;
    return 0;
}

int SprdCamera3Setting::getResultTag(CONTROL_Tag *resultInfo) {
    Mutex::Autolock l(mLock);
    *resultInfo = s_setting[mCameraId].resultInfo;
    return 0;
}

int SprdCamera3Setting::setAeCONTROLTag(CONTROL_Tag *controlInfo) {
    Mutex::Autolock l(mLock);
    s_setting[mCameraId].controlInfo.ae_state = controlInfo->ae_state;
    return 0;
}

int SprdCamera3Setting::setAfCONTROLTag(CONTROL_Tag *controlInfo) {
    Mutex::Autolock l(mLock);
    s_setting[mCameraId].controlInfo.af_state = controlInfo->af_state;
    return 0;
}

int SprdCamera3Setting::setAwbCONTROLTag(CONTROL_Tag *controlInfo) {
    Mutex::Autolock l(mLock);
    s_setting[mCameraId].controlInfo.awb_state = controlInfo->awb_state;
    return 0;
}

int SprdCamera3Setting::getCONTROLTag(CONTROL_Tag *controlInfo) {
    Mutex::Autolock l(mLock);
    *controlInfo = s_setting[mCameraId].controlInfo;
    return 0;
}

int SprdCamera3Setting::setEDGETag(EDGE_Tag edgeInfo) {
    s_setting[mCameraId].edgeInfo = edgeInfo;
    return 0;
}
int SprdCamera3Setting::getEDGETag(EDGE_Tag *edgeInfo) {
    *edgeInfo = s_setting[mCameraId].edgeInfo;
    return 0;
}

int SprdCamera3Setting::setQUIRKSTag(QUIRKS_Tag quirksInfo) {
    s_setting[mCameraId].quirksInfo = quirksInfo;
    return 0;
}

int SprdCamera3Setting::getQUIRKSTag(QUIRKS_Tag *quirksInfo) {
    *quirksInfo = s_setting[mCameraId].quirksInfo;
    return 0;
}

int SprdCamera3Setting::setREQUESTTag(REQUEST_Tag *requestInfo) {
    s_setting[mCameraId].requestInfo = *requestInfo;
    return 0;
}

int SprdCamera3Setting::getREQUESTTag(REQUEST_Tag *requestInfo) {
    *requestInfo = s_setting[mCameraId].requestInfo;
    return 0;
}

int SprdCamera3Setting::setSPRDDEFTag(SPRD_DEF_Tag sprddefInfo) {
    s_setting[mCameraId].sprddefInfo = sprddefInfo;
    return 0;
}

int SprdCamera3Setting::getSPRDDEFTag(SPRD_DEF_Tag *sprddefInfo) {
    *sprddefInfo = s_setting[mCameraId].sprddefInfo;
    return 0;
}

int SprdCamera3Setting::setGEOMETRICTag(GEOMETRIC_Tag geometricInfo) {
    s_setting[mCameraId].geometricInfo = geometricInfo;
    return 0;
}

int SprdCamera3Setting::getGEOMETRICTag(GEOMETRIC_Tag *geometricInfo) {
    *geometricInfo = s_setting[mCameraId].geometricInfo;
    return 0;
}

int SprdCamera3Setting::setHOTPIXERTag(HOT_PIXEL_Tag hotpixerInfo) {
    s_setting[mCameraId].hotpixerInfo = hotpixerInfo;
    return 0;
}

int SprdCamera3Setting::getHOTPIXERTag(HOT_PIXEL_Tag *hotpixerInfo) {
    *hotpixerInfo = s_setting[mCameraId].hotpixerInfo;
    return 0;
}

int SprdCamera3Setting::setNOISETag(NOISE_Tag noiseInfo) {
    s_setting[mCameraId].noiseInfo = noiseInfo;
    return 0;
}

int SprdCamera3Setting::getNOISETag(NOISE_Tag *noiseInfo) {
    *noiseInfo = s_setting[mCameraId].noiseInfo;
    return 0;
}

int SprdCamera3Setting::setSENSORTag(SENSOR_Tag sensorInfo) {
    s_setting[mCameraId].sensorInfo = sensorInfo;
    return 0;
}

int SprdCamera3Setting::setExposureTimeTag(int64_t exposureTime) {
    Mutex::Autolock l(mLock);
    s_setting[mCameraId].sensorInfo.exposure_time = exposureTime;
    return 0;
}

int SprdCamera3Setting::getSENSORTag(SENSOR_Tag *sensorInfo) {
    *sensorInfo = s_setting[mCameraId].sensorInfo;
    return 0;
}

int SprdCamera3Setting::setSHADINGTag(SHADING_Tag shadingInfo) {
    s_setting[mCameraId].shadingInfo = shadingInfo;
    return 0;
}

int SprdCamera3Setting::getSHADINGTag(SHADING_Tag *shadingInfo) {
    *shadingInfo = s_setting[mCameraId].shadingInfo;
    return 0;
}

int SprdCamera3Setting::setSTATISTICSTag(STATISTICS_Tag statisticsInfo) {
    s_setting[mCameraId].statisticsInfo = statisticsInfo;
    return 0;
}

int SprdCamera3Setting::getSTATISTICSTag(STATISTICS_Tag *statisticsInfo) {
    *statisticsInfo = s_setting[mCameraId].statisticsInfo;
    return 0;
}

int SprdCamera3Setting::setLEDTag(LED_Tag ledInfo) {
    s_setting[mCameraId].ledInfo = ledInfo;
    return 0;
}

int SprdCamera3Setting::getLEDTag(LED_Tag *ledInfo) {
    *ledInfo = s_setting[mCameraId].ledInfo;
    return 0;
}
int SprdCamera3Setting::setFACETag(FACE_Tag *faceInfo) {
    s_setting[mCameraId].faceInfo = *faceInfo;
    return 0;
}

int SprdCamera3Setting::setORIFACETag(FACE_Tag *faceInfo) {
    s_setting[mCameraId].orifaceInfo = *faceInfo;
    return 0;
}

int SprdCamera3Setting::getFACETag(FACE_Tag *faceInfo) {
    *faceInfo = s_setting[mCameraId].faceInfo;
    return 0;
}

int SprdCamera3Setting::setEISCROPTag(EIS_CROP_Tag eiscrop_Info) {
    s_setting[mCameraId].eiscrop_Info = eiscrop_Info;
    return 0;
}

int SprdCamera3Setting::getEISCROPTag(EIS_CROP_Tag *eiscrop_Info) {
    *eiscrop_Info = s_setting[mCameraId].eiscrop_Info;
    return 0;
}

int SprdCamera3Setting::setMETAInfo(meta_info_t metaInfo) {
    s_setting[mCameraId].metaInfo = metaInfo;
    return 0;
}
int SprdCamera3Setting::getMETAInfo(meta_info_t *metaInfo) {
    *metaInfo = s_setting[mCameraId].metaInfo;
    return 0;
}

int SprdCamera3Setting::setHISTOGRAMTag(int32_t *hist_report) {
    memcpy(s_setting[mCameraId].hist_report, hist_report,
           sizeof(cmr_u32) * CAMERA_ISP_HIST_ITEMS);
    return 0;
}

int SprdCamera3Setting::resetFeatureStatus(const char *fea_ip,
                                           const char *fea_eb) {
    char ip_feature[PROPERTY_VALUE_MAX];
    char feature_switch[PROPERTY_VALUE_MAX];
    char prop[PROPERTY_VALUE_MAX];
    int rc = 0;
    property_get("persist.vendor.cam.ip.switch.on", feature_switch, "0");

    property_get(fea_ip, ip_feature, "2");
    if (atoi(ip_feature) == 0) {
        property_set(fea_eb, "0");
    } else if (atoi(ip_feature) == 1) {
        property_set(fea_eb, "1");
    }

    property_get(fea_eb, prop, "2");
    if ((atoi(feature_switch) == 1) && (atoi(prop) == 0)) {
        rc = 1;
        if (atoi(prop) != 2)
            property_set(fea_ip, "1");
    } else if (atoi(prop) == 1) {
        rc = 1;
        if (atoi(prop) == 0) {
            property_set(fea_ip, "0");
        } else if (atoi(prop) == 1) {
            property_set(fea_ip, "1");
        }
    } else {
        rc = 0;
        if (atoi(prop) == 0) {
            property_set(fea_ip, "0");
        } else if (atoi(prop) == 1) {
            property_set(fea_ip, "1");
        }
    }

    return rc;
}
} // namespace sprdcamera
