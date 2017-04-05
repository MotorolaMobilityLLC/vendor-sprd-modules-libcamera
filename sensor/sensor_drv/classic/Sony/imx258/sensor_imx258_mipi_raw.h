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

#if defined(CONFIG_CAMERA_ISP_DIR_3)
#include "parameters/sensor_imx258_raw_param_v3.c"
#include "vcm_lc898214.h"
#else
#include "parameters/sensor_imx258_raw_param_main.c"
#endif
#include "parameters/sensor_imx258_otp_truly.h"

#define SENSOR_NAME "imx258_mipi_raw"
#ifdef CAMERA_SENSOR_BACK_I2C_SWITCH
#define I2C_SLAVE_ADDR 0x20 // 0x34 // 0x20    /* 16bit slave address*/
#else
#define I2C_SLAVE_ADDR 0x34 // 0x20    /* 16bit slave address*/
#endif

#define BINNING_FACTOR 2
#define imx258_PID_ADDR 0x0016
#define imx258_PID_VALUE 0x02
#define imx258_VER_ADDR 0x0017
#define imx258_VER_VALUE 0x58

/* sensor parameters begin */
/* effective sensor output image size */
#define SNAPSHOT_WIDTH 4208  // 5344
#define SNAPSHOT_HEIGHT 3120 // 4016
#define PREVIEW_WIDTH 4208   // 2672
#define PREVIEW_HEIGHT 3120  // 2008

/*Mipi output*/
#define LANE_NUM 4
#define RAW_BITS 10

#define SNAPSHOT_MIPI_PER_LANE_BPS 858
#define PREVIEW_MIPI_PER_LANE_BPS 660

/* please ref your spec */
#define FRAME_OFFSET 10
#define SENSOR_MAX_GAIN 0xF0
#define SENSOR_BASE_GAIN 0x20
#define SENSOR_MIN_SHUTTER 4

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

static SENSOR_IOCTL_FUNC_TAB_T s_imx258_ioctl_func_tab;
struct sensor_raw_info *s_imx258_mipi_raw_info_ptr = &s_imx258_mipi_raw_info;

static const SENSOR_REG_T imx258_init_setting[] = {
    // Address   value
    {0x0136, 0x18}, {0x0137, 0x00}, {0x3051, 0x00}, {0x6B11, 0xCF},
    {0x7FF0, 0x08}, {0x7FF1, 0x0F}, {0x7FF2, 0x08}, {0x7FF3, 0x1B},
    {0x7FF4, 0x23}, {0x7FF5, 0x60}, {0x7FF6, 0x00}, {0x7FF7, 0x01},
    {0x7FF8, 0x00}, {0x7FF9, 0x78}, {0x7FFA, 0x01}, {0x7FFB, 0x00},
    {0x7FFC, 0x00}, {0x7FFD, 0x00}, {0x7FFE, 0x00}, {0x7FFF, 0x03},
    {0x7F76, 0x03}, {0x7F77, 0xFE}, {0x7FA8, 0x03}, {0x7FA9, 0xFE},
    {0x7B24, 0x81}, {0x7B25, 0x01}, {0x6564, 0x07}, {0x6B0D, 0x41},
    {0x653D, 0x04}, {0x6B05, 0x8C}, {0x6B06, 0xF9}, {0x6B08, 0x65},
    {0x6B09, 0xFC}, {0x6B0A, 0xCF}, {0x6B0B, 0xD2}, {0x6700, 0x0E},
    {0x6707, 0x0E}, {0x9104, 0x00}, {0x7421, 0x1C}, {0x7423, 0xD7},
    {0x5F04, 0x00}, {0x5F05, 0xED},

};

static const SENSOR_REG_T imx258_2096x1552_setting[] = {
    /*4Lane
    reg_B30
    2096x1552(4:3) 30fps
    H: 2096
    V: 1552
    Output format Setting
        Address value*/
    {0x0112, 0x0A},
    {0x0113, 0x0A},
    {0x0114, 0x03}, // Clock Setting
    {0x0301, 0x05},
    {0x0303, 0x02},
    {0x0305, 0x04},
    {0x0306, 0x00},
    {0x0307, 0x6E},
    {0x0309, 0x0A},
    {0x030B, 0x01},
    {0x030D, 0x02},
    {0x030E, 0x00},
    {0x030F, 0xD8},
    {0x0310, 0x00},
    {0x0820, 0x0A},
    {0x0821, 0x50},
    {0x0822, 0x00},
    {0x0823, 0x00}, // Line Length Setting
    {0x0342, 0x14},
    {0x0343, 0xE8}, // Frame Length Setting
    {0x0340, 0x06},
    {0x0341, 0x6C}, // ROI Setting
    {0x0344, 0x00},
    {0x0345, 0x00},
    {0x0346, 0x00},
    {0x0347, 0x00},
    {0x0348, 0x10},
    {0x0349, 0x6F},
    {0x034A, 0x0C},
    {0x034B, 0x2F}, // Analog Image Size Setting
    {0x0381, 0x01},
    {0x0383, 0x01},
    {0x0385, 0x01},
    {0x0387, 0x01},
    {0x0900, 0x01},
    {0x0901, 0x12}, // Digital Image Size
                    // Setting
    {0x0401, 0x01},
    {0x0404, 0x00},
    {0x0405, 0x20},
    {0x0408, 0x00},
    {0x0409, 0x04},
    {0x040A, 0x00},
    {0x040B, 0x04},
    {0x040C, 0x10},
    {0x040D, 0x68},
    {0x040E, 0x06},
    {0x040F, 0x10},
    {0x3038, 0x00},
    {0x303A, 0x00},
    {0x303B, 0x10},
    {0x300D, 0x00}, // Output Size Setting
    {0x034C, 0x08},
    {0x034D, 0x30},
    {0x034E, 0x06},
    {0x034F, 0x10},
#if 0
    {0x0202,0x06},
    {0x0203,0x62}, //Integration Time Setting
    {0x0204,0x00},
    {0x0205,0x00},//Gain Setting
    {0x020E,0x01},
    {0x020F,0x00},
    {0x0210,0x01},
    {0x0211,0x00},
    {0x0212,0x01},
    {0x0213,0x00},
    {0x0214,0x01},
    {0x0215,0x00},
#endif
    {0x7BCD, 0x01}, // Added Setting(AF)
    {0x94DC, 0x20}, // Added Setting(IQ)
    {0x94DD, 0x20},
    {0x94DE, 0x20},
    {0x95DC, 0x20},
    {0x95DD, 0x20},
    {0x95DE, 0x20},
    {0x7FB0, 0x00},
    {0x9010, 0x3E},
    {0x9419, 0x50},
    {0x941B, 0x50},
    {0x9519, 0x50},
    {0x951B, 0x50}, // Added Setting(mode)
    {0x3030, 0x00},
    {0x3032, 0x00}, // 0},//1},
    {0x0220, 0x00},
    {0x4041, 0x00},
};

static const SENSOR_REG_T imx258_4208x3120_setting[] = {
    /*4Lane
    reg_A30
    Full (4:3) 30fps
    H: 4208
    V: 3120
    Output format Setting
        Address value*/
    {0x0112, 0x0A},
    {0x0113, 0x0A},
    {0x0114, 0x03}, // Clock Setting
    {0x0301, 0x05},
    {0x0303, 0x02},
    {0x0305, 0x04},
    {0x0306, 0x00},
    {0x0307, 0xD8},
    {0x0309, 0x0A},
    {0x030B, 0x01},
    {0x030D, 0x02},
    {0x030E, 0x00},
    {0x030F, 0xD8},
    {0x0310, 0x00},
    {0x0820, 0x14},
    {0x0821, 0x40},
    {0x0822, 0x00},
    {0x0823, 0x00}, // Line Length Setting
    {0x0342, 0x14},
    {0x0343, 0xE8}, // Frame Length Setting
    {0x0340, 0x0C},
    {0x0341, 0x98}, // ROI Setting
    {0x0344, 0x00},
    {0x0345, 0x00},
    {0x0346, 0x00},
    {0x0347, 0x00},
    {0x0348, 0x10},
    {0x0349, 0x6F},
    {0x034A, 0x0C},
    {0x034B, 0x2F}, // Analog Image Size Setting
    {0x0381, 0x01},
    {0x0383, 0x01},
    {0x0385, 0x01},
    {0x0387, 0x01},
    {0x0900, 0x00},
    {0x0901, 0x11}, // Digital Image Size
                    // Setting
    {0x0401, 0x00},
    {0x0404, 0x00},
    {0x0405, 0x10},
    {0x0408, 0x00},
    {0x0409, 0x00},
    {0x040A, 0x00},
    {0x040B, 0x00},
    {0x040C, 0x10},
    {0x040D, 0x70},
    {0x040E, 0x0C},
    {0x040F, 0x30},
    {0x3038, 0x00},
    {0x303A, 0x00},
    {0x303B, 0x10},
    {0x300D, 0x00}, // Output Size Setting
    {0x034C, 0x10},
    {0x034D, 0x70},
    {0x034E, 0x0C},
    {0x034F, 0x30}, // Integration Time Setting
    {0x0202, 0x0C},
    {0x0203, 0x8E}, // Gain Setting
    {0x0204, 0x00},
    {0x0205, 0x00},
    {0x020E, 0x01},
    {0x020F, 0x00},
    {0x0210, 0x01},
    {0x0211, 0x00},
    {0x0212, 0x01},
    {0x0213, 0x00},
    {0x0214, 0x01},
    {0x0215, 0x00}, // Added Setting(AF)
    {0x7BCD, 0x00}, // Added Setting(IQ)
    {0x94DC, 0x20},
    {0x94DD, 0x20},
    {0x94DE, 0x20},
    {0x95DC, 0x20},
    {0x95DD, 0x20},
    {0x95DE, 0x20},
    {0x7FB0, 0x00},
    {0x9010, 0x3E},
    {0x9419, 0x50},
    {0x941B, 0x50},
    {0x9519, 0x50},
    {0x951B, 0x50}, // Added Setting(mode)
#ifdef PDAF_TYPE2
    {0x3030, 0x01},
#else
    {0x3030, 0x00}, // 1},
#endif
    {0x3032, 0x01}, // 0},//1},
    {0x0220, 0x00},
    {0x4041, 0x00},
#ifdef PDAF_TYPE3
    {0x3052, 0x00}, // 1},
    {0x7BCB, 0x00},
    {0x7BC8, 0x00},
    {0x7BC9, 0x00},
#endif
};

static const SENSOR_REG_T imx258_1040x768_setting[] = {
    /*4Lane
    reg_E30
    HV1/4 (4:3) 120fps
    H: 1048
    V: 780
    Output format Setting
        Address value*/
    {0x0112, 0x0A},
    {0x0113, 0x0A},
    {0x0114, 0x03}, // Clock Setting
    {0x0301, 0x05},
    {0x0303, 0x02},
    {0x0305, 0x04},
    {0x0306, 0x00},
    {0x0307, 0xD8}, // 6E},
    {0x0309, 0x0A},
    {0x030B, 0x01},
    {0x030D, 0x02},
    {0x030E, 0x00},
    {0x030F, 0xD8},
    {0x0310, 0x00},
    {0x0820, 0x14}, // 0A},
    {0x0821, 0x40}, // 50},
    {0x0822, 0x00},
    {0x0823, 0x00}, // Clock Adjustment Setting
    {0x4648, 0x7F},
    {0x7420, 0x00},
    {0x7421, 0x1C},
    {0x7422, 0x00},
    {0x7423, 0xD7},
    {0x9104, 0x00}, // Line Length Setting
    {0x0342, 0x14},
    {0x0343, 0xE8}, // Frame Length Setting
    {0x0340, 0x03},
    {0x0341, 0x2C}, // 34},//ROI Setting
    {0x0344, 0x00},
    {0x0345, 0x00},
    {0x0346, 0x00},
    {0x0347, 0x00},
    {0x0348, 0x10},
    {0x0349, 0x6F},
    {0x034A, 0x0C},
    {0x034B, 0x2F}, // Analog Image Size Setting
    {0x0381, 0x01},
    {0x0383, 0x01},
    {0x0385, 0x01},
    {0x0387, 0x01},
    {0x0900, 0x01},
    {0x0901, 0x14}, // Digital Image Size
                    // Setting
    {0x0401, 0x01},
    {0x0404, 0x00},
    {0x0405, 0x40},
    {0x0408, 0x00},
    {0x0409, 0x00},
    {0x040A, 0x00},
    {0x040B, 0x00},
    {0x040C, 0x10},
    {0x040D, 0x70},
    {0x040E, 0x03},
    {0x040F, 0x0C},
    {0x3038, 0x00},
    {0x303A, 0x00},
    {0x303B, 0x10},
    {0x300D, 0x00}, // Output Size Setting
    {0x034C, 0x04},
    {0x034D, 0x10},
    {0x034E, 0x03},
    {0x034F, 0x00}, // Integration Time Setting
    {0x0202, 0x03},
    {0x0203, 0x22}, // 2A},//Gain Setting
    {0x0204, 0x00},
    {0x0205, 0x00},
    {0x020E, 0x01},
    {0x020F, 0x00},
    {0x0210, 0x01},
    {0x0211, 0x00},
    {0x0212, 0x01},
    {0x0213, 0x00},
    {0x0214, 0x01},
    {0x0215, 0x00}, // Added Setting(AF)
    {0x7BCD, 0x00}, // Added Setting(IQ)
    {0x94DC, 0x20},
    {0x94DD, 0x20},
    {0x94DE, 0x20},
    {0x95DC, 0x20},
    {0x95DD, 0x20},
    {0x95DE, 0x20},
    {0x7FB0, 0x00},
    {0x9010, 0x3E},
    {0x9419, 0x50},
    {0x941B, 0x50},
    {0x9519, 0x50},
    {0x951B, 0x50}, // Added Setting(mode)
    {0x3030, 0x00},
    {0x3032, 0x00},
    {0x0220, 0x00},
    {0x4041, 0x00},
};

static const SENSOR_REG_T imx258_1280x720_setting[] = {
    /*4Lane
    reg_J30
    HV1/3 (16:9) 120fps
    H: 1280
    V: 720
    Output format Setting*/
    {0x0112, 0x0A},
    {0x0113, 0x0A},
    {0x0114, 0x03}, // Clock Setting
    {0x0301, 0x05},
    {0x0303, 0x02},
    {0x0305, 0x04},
    {0x0306, 0x00},
    {0x0307, 0xA0},
    {0x0309, 0x0A},
    {0x030B, 0x01},
    {0x030D, 0x02},
    {0x030E, 0x00},
    {0x030F, 0xD8},
    {0x0310, 0x00},
    {0x0820, 0x0F},
    {0x0821, 0x00},
    {0x0822, 0x00},
    {0x0823, 0x00}, // Clock Adjustment Setting
    {0x4648, 0x7F}, //
    {0x7420, 0x00}, //
    {0x7421, 0x1C},
    {0x7422, 0x00}, //
    {0x7423, 0xD7},
    {0x9104, 0x00}, // Line Length Setting
    {0x0342, 0x14},
    {0x0343, 0xE8}, // Frame Length Setting
    {0x0340, 0x03},
    {0x0341, 0x1C}, // ROI Setting
    {0x0344, 0x00},
    {0x0345, 0x00},
    {0x0346, 0x01},
    {0x0347, 0xB0},
    {0x0348, 0x10},
    {0x0349, 0x6F},
    {0x034A, 0x0A},
    {0x034B, 0x7F}, // Analog Image Size Setting
    {0x0381, 0x01},
    {0x0383, 0x01},
    {0x0385, 0x03},
    {0x0387, 0x03},
    {0x0900, 0x01},
    {0x0901, 0x12}, // Digital Image Size
                    // Setting
    {0x0401, 0x01},
    {0x0404, 0x00},
    {0x0405, 0x30},
    {0x0408, 0x00},
    {0x0409, 0xB2},
    {0x040A, 0x00},
    {0x040B, 0x10},
    {0x040C, 0x0F},
    {0x040D, 0x0C},
    {0x040E, 0x02},
    {0x040F, 0xD0},
    {0x3038, 0x00},
    {0x303A, 0x00},
    {0x303B, 0x10},
    {0x300D, 0x01}, // Output Size Setting
    {0x034C, 0x05},
    {0x034D, 0x00},
    {0x034E, 0x02},
    {0x034F, 0xD0}, // Integration Time Setting
    {0x0202, 0x03},
    {0x0203, 0x12}, // Gain Setting
    {0x0204, 0x00},
    {0x0205, 0x00},
    {0x020E, 0x01},
    {0x020F, 0x00},
    {0x0210, 0x01},
    {0x0211, 0x00},
    {0x0212, 0x01},
    {0x0213, 0x00},
    {0x0214, 0x01},
    {0x0215, 0x00}, // Added Setting(AF)
    {0x7BCD, 0x01}, // Added Setting(IQ)
    {0x94DC, 0x20},
    {0x94DD, 0x20},
    {0x94DE, 0x20},
    {0x95DC, 0x20},
    {0x95DD, 0x20},
    {0x95DE, 0x20},
    {0x7FB0, 0x00},
    {0x9010, 0x3E},
    {0x9419, 0x50},
    {0x941B, 0x50},
    {0x9519, 0x50},
    {0x951B, 0x50}, // Added Setting(mode)
    {0x3030, 0x00},
    {0x3032, 0x00},
    {0x0220, 0x00},
    {0x4041, 0x00},
};

/*==============================================================================
 * Description:
 * sensor static info need by isp
 * please modify this variable acording your spec
 *============================================================================*/
static SENSOR_STATIC_INFO_T s_imx258_static_info = {
    .f_num = 220,        // f-number,focal ratio
    .focal_length = 462, // focal_length;
    .max_fps = 0,        // max_fps,max fps of sensor's all settings,it will be
                         // calculated from sensor mode fps
    .max_adgain = 16 * 16,       // max_adgain,AD-gain
    .ois_supported = 0,          // ois_supported;
    .pdaf_supported = 0,         // pdaf_supported;
    .exp_valid_frame_num = 1,    // exp_valid_frame_num;N+2-1
    .clamp_level = 64,           // clamp_level,black level
    .adgain_valid_frame_num = 1, // adgain_valid_frame_num;N+1-1
};

/*==============================================================================
 * Description:
 * sensor fps info related to sensor mode need by isp
 * please modify this variable acording your spec
 *============================================================================*/
static SENSOR_MODE_FPS_INFO_T s_imx258_mode_fps_info = {
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
LOCAL SENSOR_REG_TAB_INFO_T s_imx258_resolution_tab_raw[] = {
    {ADDR_AND_LEN_OF_ARRAY(imx258_init_setting), 0, 0, EX_MCLK, SENSOR_IMAGE_FORMAT_RAW},
    /*	{ ADDR_AND_LEN_OF_ARRAY(imx258_1040x768_setting),1040,768,EX_MCLK,SENSOR_IMAGE_FORMAT_RAW },*/
    {ADDR_AND_LEN_OF_ARRAY(imx258_1280x720_setting), 1280, 720, EX_MCLK, SENSOR_IMAGE_FORMAT_RAW},
    {ADDR_AND_LEN_OF_ARRAY(imx258_2096x1552_setting), 2096, 1552, EX_MCLK,SENSOR_IMAGE_FORMAT_RAW},
    {ADDR_AND_LEN_OF_ARRAY(imx258_4208x3120_setting), 4208, 3120, EX_MCLK,SENSOR_IMAGE_FORMAT_RAW},
    {PNULL, 0, 0, 0, 0, 0},
    {PNULL, 0, 0, 0, 0, 0},
    {PNULL, 0, 0, 0, 0, 0},
    {PNULL, 0, 0, 0, 0, 0},
    {PNULL, 0, 0, 0, 0, 0},
    {PNULL, 0, 0, 0, 0, 0},
    {PNULL, 0, 0, 0, 0, 0},
    {PNULL, 0, 0, 0, 0, 0},
    {PNULL, 0, 0, 0, 0, 0},
};

LOCAL SENSOR_TRIM_T s_imx258_resolution_trim_tab[] = {
    {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
    /*	{0,0, 1040, 768,10325,1296, 812, { 0,0,1040,768}},*/
    {0, 0, 1280, 720, 13939, 960, 796, {0, 0, 1280, 720}},
    {0, 0, 2096, 1552, 20300, 1296, 1644, {0, 0, 2096, 1552}},
    {0, 0, 4208, 3120, 10215, 1296, 3224, {0, 0, 4208, 3120}},
    {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
    {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
    {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
    {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
    {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
    {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
    {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
    {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
    {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
};

LOCAL const SENSOR_REG_T
    s_imx258_preview_size_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
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
    s_imx258_capture_size_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
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

LOCAL SENSOR_VIDEO_INFO_T s_imx258_video_info[] = {
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
        .setting_ptr = (struct sensor_reg_tag **)s_imx258_preview_size_video_tab,
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
        .setting_ptr = (struct sensor_reg_tag **)s_imx258_capture_size_video_tab,
    }};

SENSOR_INFO_T g_imx258_mipi_raw_info = {
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
    .identify_code = {{
                          .reg_addr = imx258_PID_ADDR,
                          .reg_value = imx258_PID_VALUE,
                      },
                      {
                          .reg_addr = imx258_VER_ADDR,
                          .reg_value = imx258_VER_VALUE,
                      }},
    .avdd_val = SENSOR_AVDD_2800MV,
    .iovdd_val = SENSOR_AVDD_1800MV,
    .dvdd_val = SENSOR_AVDD_1200MV,
    .source_width_max = SNAPSHOT_WIDTH,   /* max width of source image */
    .source_height_max = SNAPSHOT_HEIGHT, /* max height of source image */
    .name = (cmr_s8 *)SENSOR_NAME,
    .image_format = SENSOR_IMAGE_FORMAT_RAW,
#if defined(CONFIG_CAMERA_ISP_DIR_3)
#ifndef CAMERA_SENSOR_BACK_I2C_SWITCH
    .image_pattern = SENSOR_IMAGE_PATTERN_RAWRGB_B,
#else
    .image_pattern = SENSOR_IMAGE_PATTERN_RAWRGB_R,
#endif
#else
    .image_pattern = SENSOR_IMAGE_PATTERN_RAWRGB_R,
#endif
    .resolution_tab_info_ptr = s_imx258_resolution_tab_raw,
    .ioctl_func_tab_ptr = &s_imx258_ioctl_func_tab,
    .raw_info_ptr = &s_imx258_mipi_raw_info_ptr,
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
            .bus_width = 4,    /*lane number or bit-width*/
            .pixel_width = 10, /*bits per pixel*/
            .is_loose = 0,     /*0 packet, 1 half word per pixel*/
        },

    .video_tab_info_ptr = NULL,
    .change_setting_skip_num = 1,
    .horizontal_view_angle =
        35, // fov=78.4/2  min_focal_length=phical_dimension/(2*tan(fov/2))
    .vertical_view_angle = 35,
    .sensor_version_info = (cmr_s8 *)"imx258v1",
    //	.pixel_size = 1120, //1.12um phical_dimension=pixel_size*max_width or
    // max_height
};
