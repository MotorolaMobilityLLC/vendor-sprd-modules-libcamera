/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * V2.0
 */

#include <utils/Log.h>
#include "sensor.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"
//#include "af_dw9718s.h"

#if defined(CONFIG_CAMERA_ISP_VERSION_V3) ||                                   \
    defined(CONFIG_CAMERA_ISP_VERSION_V4)
#include "parameters/sensor_s5k5e8yx_raw_param_main.c"
#else
#include "parameters/sensor_s5k5e8yx_raw_param.c"
#endif

#define SENSOR_NAME "s5k5e8yx_mipi_raw"
#define I2C_SLAVE_ADDR 0x30 /* 16bit slave address*/

#define s5k5e8yx_PID_ADDR 0x00
#define s5k5e8yx_PID_VALUE 0x5e
#define s5k5e8yx_VER_ADDR 0x01
#define s5k5e8yx_VER_VALUE 0x80

#define SNAPSHOT_WIDTH 2592
#define SNAPSHOT_HEIGHT 1944
#define PREVIEW_WIDTH 2592
#define PREVIEW_HEIGHT 1944

#define LANE_NUM 2
#define RAW_BITS 10

#define SNAPSHOT_MIPI_PER_LANE_BPS 836
#define PREVIEW_MIPI_PER_LANE_BPS 836

/*line time unit: 0.1us*/
#define SNAPSHOT_LINE_TIME 170
#define PREVIEW_LINE_TIME 170

/* frame length*/
#define SNAPSHOT_FRAME_LENGTH 0x07B0
#define PREVIEW_FRAME_LENGTH 0x07B0

#define FRAME_OFFSET 8
#define SENSOR_MAX_GAIN 0x200
#define SENSOR_BASE_GAIN 0x20
#define SENSOR_MIN_SHUTTER 4

/* please ref your spec
 *   1 : average binning
 *   2 : sum-average binning
 *   4 : sum binning
 *  */
#define BINNING_FACTOR 1

/* please ref spec
 *  * 1: sensor auto caculate
 *   * 0: driver caculate
 *    */
#define SUPPORT_AUTO_FRAME_LENGTH 0
/* sensor parameters end */

/* isp parameters, please don't change it*/
#if defined(CONFIG_CAMERA_ISP_VERSION_V3) ||                                   \
    defined(CONFIG_CAMERA_ISP_VERSION_V4)
#define ISP_BASE_GAIN 0x80
#else
#define ISP_BASE_GAIN 0x10
#endif

/* please don't change it */
#define EX_MCLK 24

/*==============================================================================
 * Description:
 * global variable
 *============================================================================*/
// static struct hdr_info_t s_hdr_info;
// static uint32_t s_current_default_frame_length;
// struct sensor_ev_info_t s_sensor_ev_info;

static SENSOR_IOCTL_FUNC_TAB_T s_s5k5e8yx_ioctl_func_tab;
struct sensor_raw_info *s_s5k5e8yx_mipi_raw_info_ptr =
    &s_s5k5e8yx_mipi_raw_info;

static const SENSOR_REG_T s5k5e8yx_init_setting[] = {
    {0x0100, 0x00}, // stream off
    {0x3906, 0x7E},
    {0x3C01, 0x0F},
    {0x3C14, 0x00},
    {0x3235, 0x08},
    {0x3063, 0x2E},
    {0x307A, 0x10},
    {0x307B, 0x0E},
    {0x3079, 0x20},
    {0x3070, 0x05},
    {0x3067, 0x06},
    {0x3071, 0x62},
    {0x3203, 0x43},
    {0x3205, 0x43},
    {0x320b, 0x42},
    {0x323B, 0x02},
    {0x3007, 0x00},
    {0x3008, 0x14},
    {0x3020, 0x58},
    {0x300D, 0x34},
    {0x300E, 0x17},
    {0x3021, 0x02},
    {0x3010, 0x59},
    {0x3002, 0x01},
    {0x3005, 0x01},
    {0x3008, 0x04},
    {0x300F, 0x70},
    {0x3010, 0x69},
    {0x3017, 0x10},
    {0x3019, 0x19},
    {0x300C, 0x62},
    {0x3064, 0x10},
    {0x3C08, 0x0E},
    {0x3C09, 0x10},
    {0x3C31, 0x0D},
    {0x3C32, 0xAC},
    // Jessie 2592x1944,30fps,Mclk168M,mipi836M.2lane
    {0x0136, 0x18},
    {0x0137, 0x00},
    {0x0305, 0x06},
    {0x0306, 0x18},
    {0x0307, 0xA8},
    {0x0308, 0x34},
    {0x0309, 0x42},
    {0x3C1F, 0x00},
    {0x3C17, 0x00},
    {0x3C0B, 0x04},
    {0x3C1C, 0x47},
    {0x3C1D, 0x15},
    {0x3C14, 0x04},
    {0x3C16, 0x00},
    {0x0820, 0x03},
    {0x0821, 0x44},
    {0x0114, 0x01},
    {0x0344, 0x00},
    {0x0345, 0x08},
    {0x0346, 0x00},
    {0x0347, 0x08},
    {0x0348, 0x0A},
    {0x0349, 0x27},
    {0x034A, 0x07},
    {0x034B, 0x9F},
    {0x034C, 0x0A},
    {0x034D, 0x20},
    {0x034E, 0x07},
    {0x034F, 0x98},
    {0x0900, 0x00},
    {0x0901, 0x00},
    {0x0381, 0x01},
    {0x0383, 0x01},
    {0x0385, 0x01},
    {0x0387, 0x01},
    {0x0340, 0x07},
    {0x0341, 0xB0},
    {0x0342, 0x0B},
    {0x0343, 0x28},
    {0x0200, 0x00},
    {0x0201, 0x00},
    {0x0202, 0x03},
    {0x0203, 0xDE},
    {0x3303, 0x02},
    {0x3400, 0x01},
};

static const SENSOR_REG_T s5k5e8yx_snapshot_setting[] = {
    {0x0100, 0x00}, // stream off
    {0x3906, 0x7E},
    {0x3C01, 0x0F},
    {0x3C14, 0x00},
    {0x3235, 0x08},
    {0x3063, 0x2E},
    {0x307A, 0x10},
    {0x307B, 0x0E},
    {0x3079, 0x20},
    {0x3070, 0x05},
    {0x3067, 0x06},
    {0x3071, 0x62},
    {0x3203, 0x43},
    {0x3205, 0x43},
    {0x320b, 0x42},
    {0x323B, 0x02},
    {0x3007, 0x00},
    {0x3008, 0x14},
    {0x3020, 0x58},
    {0x300D, 0x34},
    {0x300E, 0x17},
    {0x3021, 0x02},
    {0x3010, 0x59},
    {0x3002, 0x01},
    {0x3005, 0x01},
    {0x3008, 0x04},
    {0x300F, 0x70},
    {0x3010, 0x69},
    {0x3017, 0x10},
    {0x3019, 0x19},
    {0x300C, 0x62},
    {0x3064, 0x10},
    {0x3C08, 0x0E},
    {0x3C09, 0x10},
    {0x3C31, 0x0D},
    {0x3C32, 0xAC},
    // Jessie 2592x1944,30fps,Mclk168M,mipi836M.2lane
    {0x0136, 0x18},
    {0x0137, 0x00},
    {0x0305, 0x06},
    {0x0306, 0x18},
    {0x0307, 0xA8},
    {0x0308, 0x34},
    {0x0309, 0x42},
    {0x3C1F, 0x00},
    {0x3C17, 0x00},
    {0x3C0B, 0x04},
    {0x3C1C, 0x47},
    {0x3C1D, 0x15},
    {0x3C14, 0x04},
    {0x3C16, 0x00},
    {0x0820, 0x03},
    {0x0821, 0x44},
    {0x0114, 0x01},
    {0x0344, 0x00},
    {0x0345, 0x08},
    {0x0346, 0x00},
    {0x0347, 0x08},
    {0x0348, 0x0A},
    {0x0349, 0x27},
    {0x034A, 0x07},
    {0x034B, 0x9F},
    {0x034C, 0x0A},
    {0x034D, 0x20},
    {0x034E, 0x07},
    {0x034F, 0x98},
    {0x0900, 0x00},
    {0x0901, 0x00},
    {0x0381, 0x01},
    {0x0383, 0x01},
    {0x0385, 0x01},
    {0x0387, 0x01},
    {0x0340, 0x07},
    {0x0341, 0xB0},
    {0x0342, 0x0B},
    {0x0343, 0x28},
    {0x0200, 0x00},
    {0x0201, 0x00},
    {0x0202, 0x03},
    {0x0203, 0xDE},
    {0x3303, 0x02},
    {0x3400, 0x01},
};

static const SENSOR_REG_T s5k5e8yx_preview_setting[] = {
    {0x0100, 0x00}, // stream off
    {0x3906, 0x7E},
    {0x3C01, 0x0F},
    {0x3C14, 0x00},
    {0x3235, 0x08},
    {0x3063, 0x2E},
    {0x307A, 0x10},
    {0x307B, 0x0E},
    {0x3079, 0x20},
    {0x3070, 0x05},
    {0x3067, 0x06},
    {0x3071, 0x62},
    {0x3203, 0x43},
    {0x3205, 0x43},
    {0x320b, 0x42},
    {0x323B, 0x02},
    {0x3007, 0x00},
    {0x3008, 0x14},
    {0x3020, 0x58},
    {0x300D, 0x34},
    {0x300E, 0x17},
    {0x3021, 0x02},
    {0x3010, 0x59},
    {0x3002, 0x01},
    {0x3005, 0x01},
    {0x3008, 0x04},
    {0x300F, 0x70},
    {0x3010, 0x69},
    {0x3017, 0x10},
    {0x3019, 0x19},
    {0x300C, 0x62},
    {0x3064, 0x10},
    {0x3C08, 0x0E},
    {0x3C09, 0x10},
    {0x3C31, 0x0D},
    {0x3C32, 0xAC},
    // Jessie 2592x1944,30fps,Mclk168M,mipi836M.2lane
    {0x0136, 0x18},
    {0x0137, 0x00},
    {0x0305, 0x06},
    {0x0306, 0x18},
    {0x0307, 0xA8},
    {0x0308, 0x34},
    {0x0309, 0x42},
    {0x3C1F, 0x00},
    {0x3C17, 0x00},
    {0x3C0B, 0x04},
    {0x3C1C, 0x47},
    {0x3C1D, 0x15},
    {0x3C14, 0x04},
    {0x3C16, 0x00},
    {0x0820, 0x03},
    {0x0821, 0x44},
    {0x0114, 0x01},
    {0x0344, 0x00},
    {0x0345, 0x08},
    {0x0346, 0x00},
    {0x0347, 0x08},
    {0x0348, 0x0A},
    {0x0349, 0x27},
    {0x034A, 0x07},
    {0x034B, 0x9F},
    {0x034C, 0x0A},
    {0x034D, 0x20},
    {0x034E, 0x07},
    {0x034F, 0x98},
    {0x0900, 0x00},
    {0x0901, 0x00},
    {0x0381, 0x01},
    {0x0383, 0x01},
    {0x0385, 0x01},
    {0x0387, 0x01},
    {0x0340, 0x07},
    {0x0341, 0xB0},
    {0x0342, 0x0B},
    {0x0343, 0x28},
    {0x0200, 0x00},
    {0x0201, 0x00},
    {0x0202, 0x03},
    {0x0203, 0xDE},
    {0x3303, 0x02},
    {0x3400, 0x01},
};

/*==============================================================================
 * Description:
 * sensor static info need by isp
 * please modify this variable acording your spec
 *============================================================================*/
static SENSOR_STATIC_INFO_T s_s5k5e8yx_static_info = {
    .f_num = 200,         // f-number,focal ratio
    .focal_length = 354,  // focal_length;
    .max_fps = 0,         // max_fps,max fps of sensor's all settings,it will be
                          // calculated from sensor mode fps
    .max_adgain = 15 * 2, // max_adgain,AD-gain
    .ois_supported = 0,   // ois_supported;
    .pdaf_supported = 0,  // pdaf_supported;
    .exp_valid_frame_num = 1,    // exp_valid_frame_num;N+2-1
    .clamp_level = 64,           // clamp_level,black level
    .adgain_valid_frame_num = 1, // adgain_valid_frame_num;N+1-1
};

/*==============================================================================
 * Description:
 * sensor fps info related to sensor mode need by isp
 * please modify this variable acording your spec
 *============================================================================*/
static SENSOR_MODE_FPS_INFO_T s_s5k5e8yx_mode_fps_info = {
    0, // is_init;
    {{SENSOR_MODE_COMMON_INIT, 0, 1, 0, 0},
     {SENSOR_MODE_PREVIEW_ONE, 0, 1, 0, 0},
     {SENSOR_MODE_SNAPSHOT_ONE_FIRST, 0, 1, 0, 0},
     {SENSOR_MODE_SNAPSHOT_ONE_SECOND, 0, 1, 0, 0},
     {SENSOR_MODE_SNAPSHOT_ONE_THIRD, 0, 1, 0, 0},
     {SENSOR_MODE_PREVIEW_TWO, 0, 1, 0, 0},
     {SENSOR_MODE_SNAPSHOT_TWO_FIRST, 0, 1, 0, 0},
     {SENSOR_MODE_SNAPSHOT_TWO_SECOND, 0, 1, 0, 0},
     {SENSOR_MODE_SNAPSHOT_TWO_THIRD, 0, 1, 0, 0}}};

/*==============================================================================
 * Description:
 * sensor all info
 * please modify this variable acording your spec
 *============================================================================*/

LOCAL SENSOR_REG_TAB_INFO_T s_s5k5e8yx_resolution_tab_raw[] = {
    {ADDR_AND_LEN_OF_ARRAY(s5k5e8yx_init_setting), 0, 0, 24,
     SENSOR_IMAGE_FORMAT_RAW},
    {ADDR_AND_LEN_OF_ARRAY(s5k5e8yx_preview_setting), 2592, 1944, 24,
     SENSOR_IMAGE_FORMAT_RAW},
    {ADDR_AND_LEN_OF_ARRAY(s5k5e8yx_snapshot_setting), 2592, 1944, 24,
     SENSOR_IMAGE_FORMAT_RAW},
    {PNULL, 0, 0, 0, 0, 0},
    {PNULL, 0, 0, 0, 0, 0},
    {PNULL, 0, 0, 0, 0, 0},
    {PNULL, 0, 0, 0, 0, 0},
    {PNULL, 0, 0, 0, 0, 0},
};

LOCAL SENSOR_TRIM_T s_s5k5e8yx_resolution_trim_tab[] = {
    {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
    {0, 0, 2592, 1944, 17000, 836, 1968, {0, 0, 2592, 1944}},
    {0, 0, 2592, 1944, 17000, 836, 1968, {0, 0, 2592, 1944}},
    {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
    {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
    {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
    {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
    {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
};

LOCAL const SENSOR_REG_T
    s_s5k5e8yx_preview_size_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
        {/*video mode 0: ?fps */
         {
             .reg_addr = 0xffff, .reg_value = 0xff,
         }},
        {
            /* video mode 1:?fps */
            {
                .reg_addr = 0xffff, .reg_value = 0xff,
            },
        },
        {
            /* video mode 2:?fps */
            {
                .reg_addr = 0xffff, .reg_value = 0xff,
            },
        },
        {
            /* video mode 3:?fps */
            {
                .reg_addr = 0xffff, .reg_value = 0xff,
            },
        },
};
LOCAL const SENSOR_REG_T
    s_s5k5e8yx_capture_size_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
        {
            /*video mode 0: ?fps */
            {
                .reg_addr = 0xffff, .reg_value = 0xff,
            },
        },
        {
            /* video mode 1:?fps */
            {
                .reg_addr = 0xffff, .reg_value = 0xff,
            },
        },
        {
            /* video mode 2:?fps */
            {
                .reg_addr = 0xffff, .reg_value = 0xff,
            },
        },
        {/* video mode 3:?fps */
         {
             .reg_addr = 0xffff, .reg_value = 0xff,
         }}};

LOCAL SENSOR_VIDEO_INFO_T s_s5k5e8yx_video_info[] = {
    {
        .ae_info =
            {
                {
                    .min_frate = 0, .max_frate = 0, .line_time = 0, .gain = 0,
                },
            },
        .setting_ptr = NULL,
    },
    {
        .ae_info =
            {
                {
                    .min_frate = 30,
                    .max_frate = 30,
                    .line_time = 270,
                    .gain = 90,
                },
            },
        .setting_ptr = s_s5k5e8yx_preview_size_video_tab,
    },
    {
        .ae_info =
            {
                {
                    .min_frate = 2,
                    .max_frate = 5,
                    .line_time = 338,
                    .gain = 1000,
                },
            },
        .setting_ptr = s_s5k5e8yx_capture_size_video_tab,
    }};

SENSOR_INFO_T g_s5k5e8yx_mipi_raw_info = {
    .salve_i2c_addr_w = I2C_SLAVE_ADDR >> 1,
    .salve_i2c_addr_r = I2C_SLAVE_ADDR >> 1,
    .reg_addr_value_bits =
        SENSOR_I2C_REG_16BIT | SENSOR_I2C_VAL_8BIT | SENSOR_I2C_FREQ_400,
    .hw_signal_polarity = SENSOR_HW_SIGNAL_PCLK_P | SENSOR_HW_SIGNAL_VSYNC_P |
                          SENSOR_HW_SIGNAL_HSYNC_P,
    .environment_mode = SENSOR_ENVIROMENT_NORMAL | SENSOR_ENVIROMENT_NIGHT,
    .image_effect = SENSOR_IMAGE_EFFECT_NORMAL |
                    SENSOR_IMAGE_EFFECT_BLACKWHITE | SENSOR_IMAGE_EFFECT_RED |
                    SENSOR_IMAGE_EFFECT_GREEN | SENSOR_IMAGE_EFFECT_BLUE |
                    SENSOR_IMAGE_EFFECT_YELLOW | SENSOR_IMAGE_EFFECT_NEGATIVE |
                    SENSOR_IMAGE_EFFECT_CANVAS,

    /* bit[0:7]: count of step in brightness, contrast, sharpness, saturation
    * bit[8:31] reseved
    */
    .wb_mode = 0,
    .step_count = 7,
    .reset_pulse_level =
        SENSOR_LOW_PULSE_RESET, /*here should be care when bring up*/
    .reset_pulse_width = 50,
    .power_down_level =
        SENSOR_LOW_LEVEL_PWDN, /*here should be care when bring up*/
    .identify_count = 1,
    .identify_code =
        {
            {
                .reg_addr = s5k5e8yx_PID_ADDR, .reg_value = s5k5e8yx_PID_VALUE,
            },
            {
                .reg_addr = s5k5e8yx_VER_ADDR, .reg_value = s5k5e8yx_VER_VALUE,
            },
        },
    .avdd_val = SENSOR_AVDD_2800MV,
    .iovdd_val = SENSOR_AVDD_1800MV,
    .dvdd_val = SENSOR_AVDD_1200MV,
    .source_width_max = SNAPSHOT_WIDTH,   /* max width of source image */
    .source_height_max = SNAPSHOT_HEIGHT, /* max height of source image */
    .name = (cmr_s8 *)SENSOR_NAME,
    .image_format = SENSOR_IMAGE_FORMAT_RAW,
    .image_pattern = SENSOR_IMAGE_PATTERN_RAWRGB_GR,

    .resolution_tab_info_ptr = s_s5k5e8yx_resolution_tab_raw,
    .ioctl_func_tab_ptr = &s_s5k5e8yx_ioctl_func_tab,
    .raw_info_ptr = &s_s5k5e8yx_mipi_raw_info_ptr,
    .ext_info_ptr = NULL,

    .preview_skip_num = 1,
    .capture_skip_num = 1,
    .flash_capture_skip_num = 6,
    .mipi_cap_skip_num = 0,
    .preview_deci_num = 0,
    .video_preview_deci_num = 0,

    .threshold_eb = 0,
    .threshold_mode = 0,
    .threshold_start = 0,
    .threshold_end = 0,
    .i2c_dev_handler = 0,
    .sensor_interface =
        {
            .type = SENSOR_INTERFACE_TYPE_CSI2,
            .bus_width = LANE_NUM,   /*lane number or bit-width*/
            .pixel_width = RAW_BITS, /*bits per pixel*/
            .is_loose = 0,           /*0 packet, 1 half word per pixel*/
        },

    .video_tab_info_ptr = NULL,
    .change_setting_skip_num = 1,
    .horizontal_view_angle = 65,
    .vertical_view_angle = 60,
    .sensor_version_info = (cmr_s8 *)"s5k5e8yxv1",
};
