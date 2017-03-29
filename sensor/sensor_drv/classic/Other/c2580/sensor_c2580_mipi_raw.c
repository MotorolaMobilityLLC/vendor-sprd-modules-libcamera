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
#include <utils/Log.h>
#include "sensor.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"
//#include "sensor_c2580_raw_param.c"

#define c2580_I2C_ADDR_W 0x3C
#define c2580_I2C_ADDR_R 0x3C

#define c2580_ES_OFFSET 10

static int s_c2580_gain = 0;
static int s_c2580_gain_bak = 0;
static int s_c2580_shutter_bak = 0;
static int s_capture_shutter = 0;
static int s_capture_VTS = 0;
static int s_video_min_framerate = 0;
static int s_video_max_framerate = 0;

static unsigned long c2580_access_val(SENSOR_HW_HANDLE handle,
                                      unsigned long param);
LOCAL unsigned long _c2580_GetResolutionTrimTab(SENSOR_HW_HANDLE handle,
                                                uint32_t param);
LOCAL uint32_t _c2580_PowerOn(SENSOR_HW_HANDLE handle, uint32_t power_on);
LOCAL uint32_t _c2580_Identify(SENSOR_HW_HANDLE handle, uint32_t param);
LOCAL uint32_t _c2580_BeforeSnapshot(SENSOR_HW_HANDLE handle, uint32_t param);
LOCAL uint32_t _c2580_after_snapshot(SENSOR_HW_HANDLE handle, uint32_t param);
LOCAL uint32_t _c2580_StreamOn(SENSOR_HW_HANDLE handle, uint32_t param);
LOCAL uint32_t _c2580_StreamOff(SENSOR_HW_HANDLE handle, uint32_t param);
LOCAL uint32_t _c2580_write_exposure(SENSOR_HW_HANDLE handle, uint32_t param);
LOCAL uint32_t _c2580_write_gain(SENSOR_HW_HANDLE handle, uint32_t param);
LOCAL uint32_t _c2580_write_af(SENSOR_HW_HANDLE handle, uint32_t param);
LOCAL uint32_t _c2580_flash(SENSOR_HW_HANDLE handle, uint32_t param);
LOCAL uint32_t _c2580_ExtFunc(SENSOR_HW_HANDLE handle, uint32_t ctl_param);
LOCAL uint32_t _c2580_set_video_mode(SENSOR_HW_HANDLE handle, uint32_t param);
LOCAL int _c2580_get_shutter(SENSOR_HW_HANDLE handle);
LOCAL int _c2580_get_gain16(SENSOR_HW_HANDLE handle);
LOCAL int _c2580_get_vts(SENSOR_HW_HANDLE handle);
#if 0
LOCAL uint32_t _c2580_cfg_otp(SENSOR_HW_HANDLE handle,uint32_t  param);
LOCAL uint32_t _c2580_update_otp(void* param_ptr);
LOCAL uint32_t _c2580_Identify_otp(void* param_ptr);


LOCAL const struct raw_param_info_tab s_c2580_raw_param_tab[]={
	{0x01, &s_c2580_mipi_raw_info, _c2580_Identify_otp, _c2580_update_otp},
	{RAW_INFO_END_ID, PNULL, PNULL, PNULL}
};
#endif
static LOCAL SENSOR_IOCTL_FUNC_TAB_T s_c2580_ioctl_func_tab;
struct sensor_raw_info *s_c2580_mipi_raw_info_ptr = NULL;

static uint32_t g_c2580_module_id = 0;

static uint32_t g_flash_mode_en = 0;
LOCAL const SENSOR_REG_T c2580_common_init[] = {};

LOCAL const SENSOR_REG_T c2580_common_init1[] = {
    {0x0103, 0x01},
    {SENSOR_WRITE_DELAY, 0x0a},
    {0x0100, 0x00},
    // for group hold
    {0x3400, 0x00},
    {0x3404, 0x05},
    {0x3500, 0x10},
    {0xe000, 0x02}, // es H  0xE002
    {0xe001, 0x02},
    {0xe002, 0x02},
    {0xe003, 0x02}, // es L   0xE005
    {0xe004, 0x03},
    {0xe005, 0x68},
    {0xe006, 0x02}, // gain    0xE008
    {0xe007, 0x05},
    {0xe008, 0x00},
    {0xe009, 0x03}, // vts H  0xE00B
    {0xe00A, 0x40},
    {0xe00B, 0x02},
    {0xe00C, 0x03}, // vts L  0xE00E
    {0xe00D, 0x41},
    {0xe00E, 0x68},
    {0x3500, 0x00},

    {0x0101, 0x03}, // 1600*1200 setting
    {0x3009, 0x03}, //
    {0x300b, 0x03}, //
    //{0x3084,0x40},//;;80
    //{0x3089,0x10},//
    //{0x3c03,0x00},//
    {0x3180, 0xd0}, //
    {0x3181, 0x40}, //

    {0x3280, 0x06}, //
    {0x3281, 0x05}, //
    {0x3282, 0x93}, //
    {0x3283, 0xd5}, //
    {0x3287, 0x46}, //
    {0x3288, 0x5f}, //
    {0x3289, 0x30}, //
    {0x328A, 0x21}, //
    {0x328B, 0x44}, //
    {0x328C, 0x34}, //
    {0x328D, 0x55}, //
    {0x328E, 0x00}, //
    //{0x3089,0x18},//
    {0x308a, 0x00}, //
    {0x308b, 0x00}, //

    {0x3584, 0x00}, //
    {0x3209, 0x80},
    {0x320C, 0x00}, //
    {0x320E, 0x08}, //
    //{0x3209,0x80},//
    {0x3210, 0x11}, //
    {0x3211, 0x09}, //
    {0x3212, 0x1a}, //
    {0x3213, 0x15}, //
    {0x3214, 0x17}, //
    {0x3215, 0x09}, //
    {0x3216, 0x17}, //
    {0x3217, 0x06}, //
    {0x3218, 0x28}, //
    {0x3219, 0x12}, //
    {0x321A, 0x00}, //
    {0x321B, 0x04}, //
    {0x321C, 0x00}, //

    {0x3200, 0x03}, //
    {0x3201, 0x46}, //
    {0x3202, 0x00}, //
    {0x3203, 0x17}, //
    {0x3204, 0x01}, //
    {0x3205, 0x64}, //
    {0x3206, 0x00}, //
    {0x3207, 0xde}, //
    {0x3208, 0x83}, //

    {0x0303, 0x01}, //
    {0x0304, 0x00}, //
    {0x0305, 0x03}, //
    {0x0307, 0x59}, //
    {0x0309, 0x30}, //

    {0x0342, 0x0b}, //
    {0x0343, 0x5c}, //

    {0x3087, 0x90}, // ;aec/agc: off
    //{0x3084,0x40},//
    {0x3089, 0x18}, //
    //{0x308a,0x00},//
    //{0x308b,0x00},//
    {0x3c03, 0x01},

    {0x3d00, 0xad}, // ;;af; BPC ON
    {0x3d01, 0x17}, //; 01just for single white pixel//20150302 old 0x15
    {0x3d07, 0x48}, //; //20150302 old 0x40
    {0x3d08, 0x48}, //;//20150302 old 0x40

    {0x3805, 0x06}, // mipi
    {0x3806, 0x06},
    {0x3807, 0x06},
    {0x3808, 0x14},
    {0x3809, 0xc4},
    {0x380a, 0x6c},
    {0x380b, 0x8c},
    {0x380c, 0x21},
};
LOCAL const SENSOR_REG_T c2580_800_600_setting[] = {
    {0x0101, 0x03}, //
    {0x3009, 0x03}, //
    {0x300b, 0x03}, //
    {0x3805, 0x06}, //
    {0x3806, 0x04}, //
    {0x3807, 0x03}, //
    {0x3808, 0x14}, //
    {0x3809, 0x75}, //
    {0x380a, 0x6D}, //
    {0x380b, 0xCC}, //
    {0x380c, 0x21}, //
    {0x0340, 0x02}, //
    {0x0341, 0x67}, //
    {0x0342, 0x06}, //
    {0x0343, 0xb2}, //
    {0x034c, 0x03}, //
    {0x034d, 0x20}, //
    {0x034e, 0x02}, //
    {0x034f, 0x58}, //
    {0x0383, 0x03}, //
    {0x0387, 0x03}, //
    {0x3021, 0x28}, //
    {0x3022, 0x02}, //
    {0x3280, 0x00}, //
    {0x3281, 0x05}, //
    {0x3084, 0x40}, //;;80
    {0x3089, 0x10}, //
    {0x3c03, 0x00}, //
    {0x3180, 0xd0}, //
    {0x3181, 0x40}, //
    {0x3280, 0x04}, //
    {0x3281, 0x05}, //
    {0x3282, 0x73}, //
    {0x3283, 0xd1}, //
    {0x3287, 0x46}, //
    {0x3288, 0x5f}, //
    {0x3289, 0x30}, //
    {0x328A, 0x21}, //
    {0x328B, 0x44}, //
    {0x328C, 0x34}, //
    {0x328D, 0x55}, //
    {0x328E, 0x00}, //
    {0x308a, 0x00}, //
    {0x308b, 0x00}, //
    {0x320C, 0x00}, //
    {0x320E, 0x08}, //
    {0x3210, 0x22}, //
    {0x3211, 0x14}, //
    {0x3212, 0x40}, //
    {0x3213, 0x35}, //
    {0x3214, 0x30}, //
    {0x3215, 0x40}, //
    {0x3216, 0x2a}, //
    {0x3217, 0x50}, //
    {0x3218, 0x31}, //
    {0x3219, 0x12}, //
    {0x321A, 0x00}, //
    {0x321B, 0x04}, //
    {0x321C, 0x00}, //
    {0x0303, 0x01}, //
    {0x0304, 0x01}, //
    {0x0305, 0x03}, //
    {0x0307, 0x69}, //
    {0x0309, 0x10}, //
    {0x3087, 0x90}, // ;aec/agc: off
    {0x3d00, 0x8d}, // ;;af; BPC ON
    {0x3d01, 0x01}, //; 01just for single white pixel
    {0x3d07, 0x10}, //;
    {0x3d08, 0x10}, //;

    /*{0x0101,0x03},//
    {0x3009,0x02},//
    {0x300b,0x02},//
    {0x3805,0x06},//
    {0x3806,0x04},//
    {0x3807,0x03},//
    {0x3808,0x14},//
    {0x3809,0x75},//
    {0x380a,0x6D},//
    {0x380b,0xCC},//
    {0x380c,0x21},//
    {0x0340,0x02},//
    {0x0341,0x68},//
    {0x0342,0x06},//
    {0x0343,0xb2},//
    {0x034c,0x03},//
    {0x034d,0x20},//
    {0x034e,0x02},//
    {0x034f,0x58},//
    {0x0383,0x03},//
    {0x0387,0x03},//
    {0x3021,0x28},//
    {0x3022,0x02},//
    {0x3280,0x00},//
    {0x3281,0x05},//
    {0x3282,0x83},//
    {0x3283,0x12},//
    {0x3287,0x40},//
    {0x3288,0x57},//
    {0x3289,0x0c},//
    {0x328A,0x21},//  0x00->0x21 20141027
    {0x328B,0x44},//
    {0x328C,0x34},//
    {0x328D,0x55},//
    {0x328E,0x00},//
    {0x308a,0x00},//
    {0x308b,0x00},//
    {0x320C,0x00},//
    {0x320E,0x08},//
    {0x3210,0x22},//
    {0x3211,0x14},//
    {0x3212,0x28},//
    {0x3213,0x14},//
    {0x3214,0x14},//
    {0x3215,0x40},//
    {0x3216,0x1a},//
    {0x3217,0x50},//
    {0x3218,0x31},//
    {0x3219,0x12},//
    {0x321A,0x00},//
    {0x321B,0x04},//
    {0x321C,0x00},//
    {0x0303,0x01},//
    {0x0304,0x01},//
    {0x0305,0x03},//
    {0x0307,0x69},//
    {0x0309,0x10},//
    {0x3087,0xD0},// ;aec/agc: off  0x90->0xD0  gain, es
    {0x0202,0x02},//
    {0x0203,0x28},//
    {0x0205,0x30},//
    {0x3089,0x10},//
    {0x3c03,0x00},//
    {0x3180,0xd0},//
    {0x3181,0x40},//
    {0x3d00,0x8d},// ;;af; BPC ON
    {0x3d01,0x01},//; 01just for single white pixel
    {0x3d07,0x10},//;
    {0x3d08,0x10},//;*/
};

LOCAL const SENSOR_REG_T c2580_1600_1200_setting[] = {

    {0x0103, 0x01},
    // mDELAY{10},
    // Y order
    {0x3880, 0x00},

    // mirror,flip
    {0x3c00, 0x03},
    {0x0101, 0x03},

    // interface
    {0x3805, 0x06},
    {0x3806, 0x06},
    {0x3807, 0x06},
    {0x3808, 0x14},
    {0x3809, 0xC4},
    {0x380a, 0x6C},
    {0x380b, 0x8C},
    {0x380c, 0x21},
    {0x380e, 0x01},

    // analog
    {0x3200, 0x05},
    {0x3201, 0xe8},
    {0x3202, 0x06},
    {0x3203, 0x08},
    {0x3208, 0xc3},
    {0x3280, 0x07},
    {0x3281, 0x05},
    {0x3282, 0xb2},
    {0x3283, 0x12},
    {0x3284, 0x0D},
    {0x3285, 0x65},
    {0x3286, 0x7c},
    {0x3287, 0x67},
    {0x3288, 0x00},
    {0x3289, 0x00},
    {0x328A, 0x00},
    {0x328B, 0x44},
    {0x328C, 0x34},
    {0x328D, 0x55},
    {0x328E, 0x00},
    {0x308a, 0x00},
    {0x308b, 0x01},
    {0x320C, 0x0C},
    {0x320E, 0x08},
    {0x3210, 0x22},
    {0x3211, 0x0f},
    {0x3212, 0xa0},
    {0x3213, 0x30},
    {0x3214, 0x80},
    {0x3215, 0x20},
    {0x3216, 0x50},
    {0x3217, 0x21},
    {0x3218, 0x31},
    {0x3219, 0x12},
    {0x321A, 0x00},
    {0x321B, 0x02},
    {0x321C, 0x00},

    // blc
    {0x3109, 0x04},
    {0x310a, 0x42},
    {0x3180, 0xf0},
    {0x3108, 0x01},

    // timing
    {0x0304, 0x00},
    {0x0342, 0x06},
    {0x034b, 0xb7},
    {0x3000, 0x81},
    {0x3001, 0x44},
    {0x3009, 0x05},
    {0x300b, 0x04},

    // aec
    {0x3083, 0x5C},
    {0x3084, 0x3f},
    {0x3085, 0x54},
    {0x3086, 0x00},
    {0x3087, 0x00},
    {0x3088, 0xf8},
    {0x3089, 0x14},
    {0x3f08, 0x10},

    // lens
    {0x3C80, 0x70},
    {0x3C81, 0x70},
    {0x3C82, 0x70},
    {0x3C83, 0x71},
    {0x3C84, 0x0},
    {0x3C85, 0x0},
    {0x3C86, 0x0},
    {0x3C87, 0x0},
    {0x3C88, 0x81},
    {0x3C89, 0x6d},
    {0x3C8A, 0x6d},
    {0x3C8B, 0x70},
    {0x3C8C, 0x0},
    {0x3C8D, 0x1},
    {0x3C8E, 0x10},
    {0x3C8F, 0xff},
    {0x3C90, 0x28},
    {0x3C91, 0x20},
    {0x3C92, 0x20},
    {0x3C93, 0x22},
    {0x3C94, 0x50},
    {0x3C95, 0x47},
    {0x3C96, 0x47},
    {0x3C97, 0x49},
    {0x3C98, 0xaa},

    // awb
    {0x0210, 0x05},
    {0x0211, 0x75},
    {0x0212, 0x04},
    {0x0213, 0x00},
    {0x0214, 0x04},
    {0x0215, 0x30},
    {0x3f8c, 0x75},
    {0x3f80, 0x0},
    {0x3f81, 0x57},
    {0x3f82, 0x66},
    {0x3f83, 0x65},
    {0x3f84, 0x73},
    {0x3f85, 0x9f},
    {0x3f86, 0x28},
    {0x3f87, 0x07},
    {0x3f88, 0x6b},
    {0x3f89, 0x79},
    {0x3f8a, 0x49},
    {0x3f8b, 0x8},

    // dns
    {0x3d00, 0x8f},
    {0x3d01, 0x30},
    {0x3d02, 0xff},
    {0x3d03, 0x30},
    {0x3d04, 0xff},
    {0x3d05, 0x08},
    {0x3d06, 0x10},
    {0x3d07, 0x10},
    {0x3d08, 0xf0},
    {0x3d09, 0x50},
    {0x3d0a, 0x30},
    {0x3d0b, 0x00},
    {0x3d0c, 0x00},
    {0x3d0d, 0x20},
    {0x3d0e, 0x00},
    {0x3d0f, 0x50},
    {0x3d10, 0x00},
    {0x3d11, 0x10},
    {0x3d12, 0x00},
    {0x3d13, 0xf0},

    // edge
    {0x3d20, 0x03},
    {0x3d21, 0x10},
    {0x3d22, 0x40},

    // ccm
    {0x3e00, 0x9e},
    {0x3e01, 0x82},
    {0x3e02, 0x8b},
    {0x3e03, 0x84},
    {0x3e04, 0x8a},
    {0x3e05, 0xb1},

    // gamma
    {0x3d80, 0x13},
    {0x3d81, 0x1f},
    {0x3d82, 0x2f},
    {0x3d83, 0x4a},
    {0x3d84, 0x56},
    {0x3d85, 0x62},
    {0x3d86, 0x6d},
    {0x3d87, 0x76},
    {0x3d88, 0x7f},
    {0x3d89, 0x87},
    {0x3d8a, 0x96},
    {0x3d8b, 0xa5},
    {0x3d8c, 0xbf},
    {0x3d8d, 0xd5},
    {0x3d8e, 0xea},
    {0x3d8f, 0x15},

    // sde
    {0x3e80, 0x21},
    {0x3e83, 0x01},
    {0x3e84, 0x1f},
    {0x3e85, 0x1f},
    {0x3e88, 0x9f},
    {0x3e89, 0x00},
    {0x3e8b, 0x10},
    {0x3e8c, 0x40},
    {0x3e8d, 0x80},
    {0x3e8e, 0x30},

    // stream on
    //	{0x0100, 0x01},
    {0x0205, 0x01},

    /*{0x0101,0x03},//
    {0x3009,0x04},//0x04->0x03 20141028
    {0x300b,0x04},//0x04->0x03 20141028
    {0x0340,0x04},//
    {0x0341,0xc8},//
    {0x0342,0x06},//
    {0x0343,0xb2},//
    {0x034c,0x06},//
    {0x034d,0x40},//
    {0x034e,0x04},//
    {0x034f,0xb0},//
    {0x0383,0x01},//
    {0x0387,0x01},//
    {0x3021,0x00},//
    {0x3022,0x01},//
    {0x3280,0x00},//
    {0x3281,0x05},//
    {0x3282,0x83},//
    {0x3283,0x12},//
    {0x3287,0x40},//
    {0x3288,0x57},//
    {0x3289,0x0c},//
    {0x328A,0x21},//  0x00->0x21 20141027
    {0x328B,0x44},//
    {0x328C,0x34},//
    {0x328D,0x55},//
    {0x328E,0x00},//
    {0x308a,0x00},//
    {0x308b,0x00},//
    {0x320C,0x00},//
    {0x320E,0x08},//
    {0x3210,0x22},//
    {0x3211,0x14},//
    {0x3212,0x28},//
    {0x3213,0x14},//
    {0x3214,0x14},//
    {0x3215,0x40},//
    {0x3216,0x1a},//
    {0x3217,0x50},//
    {0x3218,0x31},//
    {0x3219,0x12},//
    {0x321A,0x00},//
    {0x321B,0x04},//
    {0x321C,0x00},//
    {0x0303,0x01},//
    {0x0304,0x01},//
    {0x0305,0x03},//
    {0x0307,0x69},//
    {0x0309,0x10},//
    {0x3087,0xD0},// ;aec/agc: off  0x90->0xD0
    //{0x0202,0x04},//
    //{0x0203,0x50},//
    //{0x0205,0x1f},//
    {0x3089,0x10},//
    {0x3c03,0x00},//
    {0x3180,0xd0},//
    {0x3181,0x40},//
    {0x3d00,0x8d},// ;;af; BPC ON
    {0x3d01,0x01},//; 01just for single white pixel
    {0x3d07,0x10},//;
    {0x3d08,0x10},//;*/
};

LOCAL SENSOR_REG_TAB_INFO_T s_c2580_resolution_Tab_RAW[] = {
    {ADDR_AND_LEN_OF_ARRAY(c2580_common_init), 0, 0, 24,
     SENSOR_IMAGE_FORMAT_RAW},
    //{ADDR_AND_LEN_OF_ARRAY(c2580_800_600_setting), 800, 600, 24,
    // SENSOR_IMAGE_FORMAT_RAW},
    {ADDR_AND_LEN_OF_ARRAY(c2580_1600_1200_setting), 1600, 1200, 24,
     SENSOR_IMAGE_FORMAT_RAW},
    {PNULL, 0, 0, 0, 0, 0},
    {PNULL, 0, 0, 0, 0, 0},
    {PNULL, 0, 0, 0, 0, 0},
    {PNULL, 0, 0, 0, 0, 0},
    {PNULL, 0, 0, 0, 0, 0},
    {PNULL, 0, 0, 0, 0, 0}, // cg
    {PNULL, 0, 0, 0, 0, 0}};

LOCAL SENSOR_TRIM_T s_c2580_Resolution_Trim_Tab[] = {
    {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
    //{0, 0, 800, 600, 544, 315, 616, {0, 0, 800, 600}},
    //{0, 0, 1600, 1200, 544, 315, 1224, {0, 0, 1600, 1200}},
    {0, 0, 1600, 1200, 544, 534, 1224, {0, 0, 1600, 1200}},
    {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
    {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}}, // cg
    {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
    {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
    {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
    {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
    {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}}};

LOCAL const SENSOR_REG_T s_c2580_800x600_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
    /*video mode 0: ?fps*/
    {{0xffff, 0xff}},
    /* video mode 1:?fps*/
    {{0xffff, 0xff}},
    /* video mode 2:?fps*/
    {{0xffff, 0xff}},
    /* video mode 3:?fps*/
    {{0xffff, 0xff}}};

LOCAL const SENSOR_REG_T
    s_c2580_1600x1200_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
        /*video mode 0: ?fps*/
        {{0xffff, 0xff}},
        /* video mode 1:?fps*/
        {{0xffff, 0xff}},
        /* video mode 2:?fps*/
        {{0xffff, 0xff}},
        /* video mode 3:?fps*/
        {{0xffff, 0xff}}};

LOCAL SENSOR_VIDEO_INFO_T s_c2580_video_info[] = {
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
    //{{{30, 30, 544, 100}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0,
    // 0}},(SENSOR_REG_T**)s_c2580_800x600_video_tab},
    //{{{15, 15, 544, 100}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0,
    // 0}},(SENSOR_REG_T**)s_c2580_1600x1200_video_tab},
    {{{15, 15, 544, 100}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
     (SENSOR_REG_T **)s_c2580_1600x1200_video_tab},
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL}};

LOCAL uint32_t _c2580_set_video_mode(SENSOR_HW_HANDLE handle, uint32_t param) {
    SENSOR_REG_T_PTR sensor_reg_ptr;
    uint16_t i = 0x00;
    uint32_t mode;

    if (param >= SENSOR_VIDEO_MODE_MAX)
        return 0;

    if (SENSOR_SUCCESS != Sensor_GetMode(&mode)) {
        SENSOR_PRINT("fail.");
        return SENSOR_FAIL;
    }

    if (PNULL == s_c2580_video_info[mode].setting_ptr) {
        SENSOR_PRINT("fail. mode=%d", mode);
        return SENSOR_FAIL;
    }

    sensor_reg_ptr =
        (SENSOR_REG_T_PTR)&s_c2580_video_info[mode].setting_ptr[param];
    if (PNULL == sensor_reg_ptr) {
        SENSOR_PRINT("fail.");
        return SENSOR_FAIL;
    }

    for (i = 0x00; (0xffff != sensor_reg_ptr[i].reg_addr) ||
                   (0xff != sensor_reg_ptr[i].reg_value);
         i++) {
        Sensor_WriteReg(sensor_reg_ptr[i].reg_addr,
                        sensor_reg_ptr[i].reg_value);
    }

    SENSOR_PRINT("0x%02x", param);
    return 0;
}

SENSOR_INFO_T g_c2580_mipi_raw_info = {
    c2580_I2C_ADDR_W, // salve i2c write address
    c2580_I2C_ADDR_R, // salve i2c read address

    SENSOR_I2C_REG_16BIT | SENSOR_I2C_REG_8BIT |
        SENSOR_I2C_FREQ_400, // bit0: 0: i2c register value is 8 bit, 1: i2c
                             // register value is 16 bit
    // bit1: 0: i2c register addr  is 8 bit, 1: i2c register addr  is 16 bit
    // other bit: reseved
    SENSOR_HW_SIGNAL_PCLK_N | SENSOR_HW_SIGNAL_VSYNC_N |
        SENSOR_HW_SIGNAL_HSYNC_P, // bit0: 0:negative; 1:positive -> polarily of
                                  // pixel clock
    // bit2: 0:negative; 1:positive -> polarily of horizontal synchronization
    // signal
    // bit4: 0:negative; 1:positive -> polarily of vertical synchronization
    // signal
    // other bit: reseved

    // preview mode
    SENSOR_ENVIROMENT_NORMAL | SENSOR_ENVIROMENT_NIGHT,

    // image effect
    SENSOR_IMAGE_EFFECT_NORMAL | SENSOR_IMAGE_EFFECT_BLACKWHITE |
        SENSOR_IMAGE_EFFECT_RED | SENSOR_IMAGE_EFFECT_GREEN |
        SENSOR_IMAGE_EFFECT_BLUE | SENSOR_IMAGE_EFFECT_YELLOW |
        SENSOR_IMAGE_EFFECT_NEGATIVE | SENSOR_IMAGE_EFFECT_CANVAS,

    // while balance mode
    0,

    7, // bit[0:7]: count of step in brightness, contrast, sharpness, saturation
    // bit[8:31] reseved

    SENSOR_LOW_PULSE_RESET, // reset pulse level
    10,                     // reset pulse width(ms)

    SENSOR_LOW_LEVEL_PWDN, // 1: high level valid; 0: low level valid

    1,               // count of identify code
    {{0x0000, 0x02}, // supply two code to identify sensor.
     {0x0001,
      0x02}}, // for Example: index = 0-> Device id, index = 1 -> version id

    SENSOR_AVDD_2800MV, // voltage of avdd

    1600,               // max width of source image
    1200,               // max height of source image
    (cmr_s8 *) "c2580", // name of sensor

    SENSOR_IMAGE_FORMAT_YUV422, // define in SENSOR_IMAGE_FORMAT_E
                                // enum,SENSOR_IMAGE_FORMAT_MAX
    // if set to SENSOR_IMAGE_FORMAT_MAX here, image format depent on
    // SENSOR_REG_TAB_INFO_T

    SENSOR_IMAGE_PATTERN_YUV422_YUYV, // pattern of input image form sensor;

    s_c2580_resolution_Tab_RAW, // point to resolution table information
                                // structure
    &s_c2580_ioctl_func_tab,    // point to ioctl function table
    &s_c2580_mipi_raw_info_ptr, // information and table about Rawrgb sensor
    NULL, //&g_c2580_ext_info,                // extend information about sensor
    SENSOR_AVDD_1800MV, // iovdd
    SENSOR_AVDD_1500MV, // dvdd
    1,                  // skip frame num before preview
    1,                  // skip frame num before capture
    0,                  // deci frame num during preview
    0,                  // deci frame num during video preview
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    {SENSOR_INTERFACE_TYPE_CSI2, 1, 10, 0},
    s_c2580_video_info,
    3,  // skip frame num while change setting
    48, // horizontal view angle
    48, // vertical view angle
    (cmr_s8 *) "c2580",
};

LOCAL struct sensor_raw_info *Sensor_GetContext(void) {
    return s_c2580_mipi_raw_info_ptr;
}
LOCAL uint32_t _c2580_Identify_otp(void *param_ptr) {
    uint32_t rtn = SENSOR_SUCCESS;

    return rtn;
}

LOCAL uint32_t _c2580_update_otp(void *param_ptr) { return 0; }

static unsigned long _c2580_set_init(SENSOR_HW_HANDLE handle) {
    uint16_t i;
    SENSOR_REG_T *sensor_reg_ptr = (SENSOR_REG_T *)c2580_1600_1200_setting;
    usleep(100 * 1000);

    for (i = 0; (0xFF != sensor_reg_ptr[i].reg_addr) &&
                (0xFF != sensor_reg_ptr[i].reg_value);
         i++) {
        Sensor_WriteReg(sensor_reg_ptr[i].reg_addr,
                        sensor_reg_ptr[i].reg_value);
    }

    SENSOR_LOGI("X");

    return 0;
}

LOCAL uint32_t _c2580_StreamOn(SENSOR_HW_HANDLE handle, uint32_t param) {
    int temp;

    _c2580_set_init(handle);

    temp = Sensor_ReadReg(0x0100);

    temp = temp | 0x01;

    SENSOR_PRINT("SENSOR_c2580: StreamOn: 0x%x", temp);

    Sensor_WriteReg(0x0100, temp);
    // usleep(100*1000);

    return 0;
}
static unsigned long _c2580_get_brightness(SENSOR_HW_HANDLE handle,
                                           cmr_u32 *param) {
    uint16_t i = 0;

    // Sensor_WriteReg(0xfe,0x01);
    *param = Sensor_ReadReg(0x3f09);
    SENSOR_LOGI("SENSOR: get_brightness: lumma = 0x%x\n", *param);
    // Sensor_WriteReg(handle,0xfe,0x00);
    return 0;
}

static unsigned long c2580_access_val(SENSOR_HW_HANDLE handle,
                                      unsigned long param) {
    uint32_t rtn = SENSOR_SUCCESS;
    SENSOR_VAL_T *param_ptr = (SENSOR_VAL_T *)param;
    uint16_t tmp;

    SENSOR_LOGI("SENSOR_c2580: _c2580_access_val E param_ptr = %p", param_ptr);
    if (!param_ptr) {
        return rtn;
    }
    /*
    for(tmp = 0; tmp < 10; tmp ++){
            rtn = _c2580_get_brightness(handle, param_ptr->pval);
            usleep(1000*1000);
    }
    */
    SENSOR_LOGI("SENSOR_c2580: param_ptr->type=%x", param_ptr->type);
    switch (param_ptr->type) {
    case SENSOR_VAL_TYPE_GET_BV:
        rtn = _c2580_get_brightness(handle, param_ptr->pval);
        break;
    default:
        break;
    }

    SENSOR_LOGI("SENSOR_c2580: _c2580_access_val X");

    return rtn;
}

LOCAL uint32_t _c2580_StreamOff(SENSOR_HW_HANDLE handle, uint32_t param) {
    int temp;

    SENSOR_LOGI("Stop");

    temp = Sensor_ReadReg(0x0100);

    temp = temp & 0xfe;

    SENSOR_PRINT("SENSOR_c2580: StreamOff: 0x%x", temp);

    Sensor_WriteReg(0x0100, temp);
    usleep(150 * 1000);

    return 0;
}

int _c2580_get_gain16(SENSOR_HW_HANDLE handle) {
    int gain16;

    gain16 = Sensor_ReadReg(0x0205);

    return gain16;
}

int _c2580_set_gain16(SENSOR_HW_HANDLE handle, int gain16) {
    int temp;

    SENSOR_PRINT("SENSOR_c2580: write_gain:0x%x", gain16);

    temp = gain16 & 0xff;
    Sensor_WriteReg(0x0205, temp);

    return 0;
}
int _c2580_get_shutter(SENSOR_HW_HANDLE handle) {
    // read shutter, in number of line period
    int shutter;

    shutter = Sensor_ReadReg(0x0202);
    shutter = (shutter << 8) + Sensor_ReadReg(0x0203);

    return shutter;
}

int _c2580_set_shutter(SENSOR_HW_HANDLE handle, int shutter) {
    // write shutter, in number of line period
    int temp;

    SENSOR_PRINT("SENSOR_c2580: write_shutter:%d", shutter);

    temp = shutter >> 8;
    Sensor_WriteReg(0x0202, temp);

    temp = shutter & 0xff;
    Sensor_WriteReg(0x0203, temp);

    return 0;
}
LOCAL int _c2580_get_VTS(SENSOR_HW_HANDLE handle) {
    // read VTS from register settings
    int VTS;

    VTS = Sensor_ReadReg(0x0340); // total vertical size[15:8] high byte

    VTS = (VTS << 8) + Sensor_ReadReg(0x0341);

    return VTS;
}

LOCAL int _c2580_set_VTS(SENSOR_HW_HANDLE handle, int VTS) {
    // write VTS to registers
    int temp;

    SENSOR_PRINT("SENSOR_c2580: write_vts:%d", VTS);

    temp = VTS >> 8;
    Sensor_WriteReg(0x0340, temp);

    temp = VTS & 0xff;
    Sensor_WriteReg(0x0341, temp);

    return 0;
}

LOCAL int _c2580_grouphold_on(SENSOR_HW_HANDLE handle) {
    SENSOR_PRINT("");
    /*Sensor_WriteReg(0x3400,0x00);
    Sensor_WriteReg(0x3404,0x05);
    Sensor_WriteReg(0xe000,0x02);//es H  0xE002
    Sensor_WriteReg(0xe001,0x02);
    Sensor_WriteReg(0xe003,0x02);//es L   0xE005
    Sensor_WriteReg(0xe004,0x03);
    Sensor_WriteReg(0xe006,0x02);//gain    0xE008
    Sensor_WriteReg(0xe007,0x05);

    Sensor_WriteReg(0xe009,0x03);//vts H  0xE00B
    Sensor_WriteReg(0xe00A,0x40);*/
    Sensor_WriteReg(0xe00B, (_c2580_get_VTS(handle) >> 8) & 0xFF);
    /*Sensor_WriteReg(0xe00C,0x03);//vts L  0xE00E
    Sensor_WriteReg(0xe00D,0x41);*/
    Sensor_WriteReg(0xe00E, _c2580_get_VTS(handle) & 0xFF);
    return 0;
}
LOCAL int _c2580_grouphold_off(SENSOR_HW_HANDLE handle) {
    Sensor_WriteReg(0x340F, 0x20); // fast write
    SENSOR_PRINT("vts:0x%x, shutter:0x%x, gain: 0x%x", _c2580_get_VTS(handle),
                 _c2580_get_shutter(handle), _c2580_get_gain16(handle));

    return 0;
}

static void _calculate_hdr_exposure(SENSOR_HW_HANDLE handle, int capture_gain16,
                                    int capture_VTS, int capture_shutter) {
    //_c2580_grouphold_on();

    // write capture gain
    _c2580_set_gain16(handle, capture_gain16);

    // write capture shutter
    if (capture_shutter > (capture_VTS - c2580_ES_OFFSET)) {
        capture_VTS = capture_shutter + c2580_ES_OFFSET;
        _c2580_set_VTS(handle, capture_VTS);
    }
    _c2580_set_shutter(handle, capture_shutter);

    //_c2580_grouphold_off();
}
LOCAL uint32_t Sensor_c2580_InitRawTuneInfo(SENSOR_HW_HANDLE handle) {
    uint32_t rtn = 0x00;
#if 0
	struct sensor_raw_info* raw_sensor_ptr=Sensor_GetContext();
	struct sensor_raw_tune_info* sensor_ptr=raw_sensor_ptr->tune_ptr;
	struct sensor_raw_cali_info* cali_ptr=raw_sensor_ptr->cali_ptr;

	raw_sensor_ptr->version_info->version_id=0x00010000;
	raw_sensor_ptr->version_info->srtuct_size=sizeof(struct sensor_raw_info);

	//bypass
	sensor_ptr->version_id=0x00010000;
	sensor_ptr->blc_bypass=0x00;
	sensor_ptr->nlc_bypass=0x01;
	sensor_ptr->lnc_bypass=0x01;
	sensor_ptr->ae_bypass=0x00;
	sensor_ptr->awb_bypass=0x00;
	sensor_ptr->bpc_bypass=0x01;
	sensor_ptr->denoise_bypass=0x01;
	sensor_ptr->grgb_bypass=0x01;
	sensor_ptr->cmc_bypass=0x00;
	sensor_ptr->gamma_bypass=0x00;
	sensor_ptr->uvdiv_bypass=0x01;
	sensor_ptr->pref_bypass=0x01;
	sensor_ptr->bright_bypass=0x00;
	sensor_ptr->contrast_bypass=0x00;
	sensor_ptr->hist_bypass=0x01;
	sensor_ptr->auto_contrast_bypass=0x01;
	sensor_ptr->af_bypass=0x00;
	sensor_ptr->edge_bypass=0x00;
	sensor_ptr->fcs_bypass=0x00;
	sensor_ptr->css_bypass=0x00;
	sensor_ptr->saturation_bypass=0x00;
	sensor_ptr->hdr_bypass=0x01;
	sensor_ptr->glb_gain_bypass=0x01;
	sensor_ptr->chn_gain_bypass=0x01;

	//blc
	sensor_ptr->blc.mode=0x00;
	sensor_ptr->blc.offset[0].r=0x0f;
	sensor_ptr->blc.offset[0].gr=0x0f;
	sensor_ptr->blc.offset[0].gb=0x0f;
	sensor_ptr->blc.offset[0].b=0x0f;

	sensor_ptr->blc.offset[1].r=0x0f;
	sensor_ptr->blc.offset[1].gr=0x0f;
	sensor_ptr->blc.offset[1].gb=0x0f;
	sensor_ptr->blc.offset[1].b=0x0f;

	//nlc
	sensor_ptr->nlc.r_node[0]=0;
	sensor_ptr->nlc.r_node[1]=16;
	sensor_ptr->nlc.r_node[2]=32;
	sensor_ptr->nlc.r_node[3]=64;
	sensor_ptr->nlc.r_node[4]=96;
	sensor_ptr->nlc.r_node[5]=128;
	sensor_ptr->nlc.r_node[6]=160;
	sensor_ptr->nlc.r_node[7]=192;
	sensor_ptr->nlc.r_node[8]=224;
	sensor_ptr->nlc.r_node[9]=256;
	sensor_ptr->nlc.r_node[10]=288;
	sensor_ptr->nlc.r_node[11]=320;
	sensor_ptr->nlc.r_node[12]=384;
	sensor_ptr->nlc.r_node[13]=448;
	sensor_ptr->nlc.r_node[14]=512;
	sensor_ptr->nlc.r_node[15]=576;
	sensor_ptr->nlc.r_node[16]=640;
	sensor_ptr->nlc.r_node[17]=672;
	sensor_ptr->nlc.r_node[18]=704;
	sensor_ptr->nlc.r_node[19]=736;
	sensor_ptr->nlc.r_node[20]=768;
	sensor_ptr->nlc.r_node[21]=800;
	sensor_ptr->nlc.r_node[22]=832;
	sensor_ptr->nlc.r_node[23]=864;
	sensor_ptr->nlc.r_node[24]=896;
	sensor_ptr->nlc.r_node[25]=928;
	sensor_ptr->nlc.r_node[26]=960;
	sensor_ptr->nlc.r_node[27]=992;
	sensor_ptr->nlc.r_node[28]=1023;

	sensor_ptr->nlc.g_node[0]=0;
	sensor_ptr->nlc.g_node[1]=16;
	sensor_ptr->nlc.g_node[2]=32;
	sensor_ptr->nlc.g_node[3]=64;
	sensor_ptr->nlc.g_node[4]=96;
	sensor_ptr->nlc.g_node[5]=128;
	sensor_ptr->nlc.g_node[6]=160;
	sensor_ptr->nlc.g_node[7]=192;
	sensor_ptr->nlc.g_node[8]=224;
	sensor_ptr->nlc.g_node[9]=256;
	sensor_ptr->nlc.g_node[10]=288;
	sensor_ptr->nlc.g_node[11]=320;
	sensor_ptr->nlc.g_node[12]=384;
	sensor_ptr->nlc.g_node[13]=448;
	sensor_ptr->nlc.g_node[14]=512;
	sensor_ptr->nlc.g_node[15]=576;
	sensor_ptr->nlc.g_node[16]=640;
	sensor_ptr->nlc.g_node[17]=672;
	sensor_ptr->nlc.g_node[18]=704;
	sensor_ptr->nlc.g_node[19]=736;
	sensor_ptr->nlc.g_node[20]=768;
	sensor_ptr->nlc.g_node[21]=800;
	sensor_ptr->nlc.g_node[22]=832;
	sensor_ptr->nlc.g_node[23]=864;
	sensor_ptr->nlc.g_node[24]=896;
	sensor_ptr->nlc.g_node[25]=928;
	sensor_ptr->nlc.g_node[26]=960;
	sensor_ptr->nlc.g_node[27]=992;
	sensor_ptr->nlc.g_node[28]=1023;

	sensor_ptr->nlc.b_node[0]=0;
	sensor_ptr->nlc.b_node[1]=16;
	sensor_ptr->nlc.b_node[2]=32;
	sensor_ptr->nlc.b_node[3]=64;
	sensor_ptr->nlc.b_node[4]=96;
	sensor_ptr->nlc.b_node[5]=128;
	sensor_ptr->nlc.b_node[6]=160;
	sensor_ptr->nlc.b_node[7]=192;
	sensor_ptr->nlc.b_node[8]=224;
	sensor_ptr->nlc.b_node[9]=256;
	sensor_ptr->nlc.b_node[10]=288;
	sensor_ptr->nlc.b_node[11]=320;
	sensor_ptr->nlc.b_node[12]=384;
	sensor_ptr->nlc.b_node[13]=448;
	sensor_ptr->nlc.b_node[14]=512;
	sensor_ptr->nlc.b_node[15]=576;
	sensor_ptr->nlc.b_node[16]=640;
	sensor_ptr->nlc.b_node[17]=672;
	sensor_ptr->nlc.b_node[18]=704;
	sensor_ptr->nlc.b_node[19]=736;
	sensor_ptr->nlc.b_node[20]=768;
	sensor_ptr->nlc.b_node[21]=800;
	sensor_ptr->nlc.b_node[22]=832;
	sensor_ptr->nlc.b_node[23]=864;
	sensor_ptr->nlc.b_node[24]=896;
	sensor_ptr->nlc.b_node[25]=928;
	sensor_ptr->nlc.b_node[26]=960;
	sensor_ptr->nlc.b_node[27]=992;
	sensor_ptr->nlc.b_node[28]=1023;

	sensor_ptr->nlc.l_node[0]=0;
	sensor_ptr->nlc.l_node[1]=16;
	sensor_ptr->nlc.l_node[2]=32;
	sensor_ptr->nlc.l_node[3]=64;
	sensor_ptr->nlc.l_node[4]=96;
	sensor_ptr->nlc.l_node[5]=128;
	sensor_ptr->nlc.l_node[6]=160;
	sensor_ptr->nlc.l_node[7]=192;
	sensor_ptr->nlc.l_node[8]=224;
	sensor_ptr->nlc.l_node[9]=256;
	sensor_ptr->nlc.l_node[10]=288;
	sensor_ptr->nlc.l_node[11]=320;
	sensor_ptr->nlc.l_node[12]=384;
	sensor_ptr->nlc.l_node[13]=448;
	sensor_ptr->nlc.l_node[14]=512;
	sensor_ptr->nlc.l_node[15]=576;
	sensor_ptr->nlc.l_node[16]=640;
	sensor_ptr->nlc.l_node[17]=672;
	sensor_ptr->nlc.l_node[18]=704;
	sensor_ptr->nlc.l_node[19]=736;
	sensor_ptr->nlc.l_node[20]=768;
	sensor_ptr->nlc.l_node[21]=800;
	sensor_ptr->nlc.l_node[22]=832;
	sensor_ptr->nlc.l_node[23]=864;
	sensor_ptr->nlc.l_node[24]=896;
	sensor_ptr->nlc.l_node[25]=928;
	sensor_ptr->nlc.l_node[26]=960;
	sensor_ptr->nlc.l_node[27]=992;
	sensor_ptr->nlc.l_node[28]=1023;

	//ae
	sensor_ptr->ae.skip_frame=0x01;
	sensor_ptr->ae.normal_fix_fps=0;
	sensor_ptr->ae.night_fix_fps=0;
	sensor_ptr->ae.video_fps=0x1e;
	sensor_ptr->ae.target_lum=120;
	sensor_ptr->ae.target_zone=8;
	sensor_ptr->ae.quick_mode=1;
	sensor_ptr->ae.smart=0x00;// bit0: denoise bit1: edge bit2: startion
	sensor_ptr->ae.smart_rotio=255;
	sensor_ptr->ae.smart_mode=0; // 0: gain 1: lum
	sensor_ptr->ae.smart_base_gain=64;
	sensor_ptr->ae.smart_wave_min=0;
	sensor_ptr->ae.smart_wave_max=1023;
	sensor_ptr->ae.smart_pref_min=0;
	sensor_ptr->ae.smart_pref_max=255;
	sensor_ptr->ae.smart_denoise_min_index=0;
	sensor_ptr->ae.smart_denoise_max_index=254;
	sensor_ptr->ae.smart_edge_min_index=0;
	sensor_ptr->ae.smart_edge_max_index=6;
	sensor_ptr->ae.smart_sta_low_thr=40;
	sensor_ptr->ae.smart_sta_high_thr=120;
	sensor_ptr->ae.smart_sta_rotio=128;
	sensor_ptr->ae.ev[0]=0xd0;
	sensor_ptr->ae.ev[1]=0xe0;
	sensor_ptr->ae.ev[2]=0xf0;
	sensor_ptr->ae.ev[3]=0x00;
	sensor_ptr->ae.ev[4]=0x10;
	sensor_ptr->ae.ev[5]=0x20;
	sensor_ptr->ae.ev[6]=0x30;
	sensor_ptr->ae.ev[7]=0x00;
	sensor_ptr->ae.ev[8]=0x00;
	sensor_ptr->ae.ev[9]=0x00;
	sensor_ptr->ae.ev[10]=0x00;
	sensor_ptr->ae.ev[11]=0x00;
	sensor_ptr->ae.ev[12]=0x00;
	sensor_ptr->ae.ev[13]=0x00;
	sensor_ptr->ae.ev[14]=0x00;
	sensor_ptr->ae.ev[15]=0x00;

	//awb
	sensor_ptr->awb.win_start.x=0x00;
	sensor_ptr->awb.win_start.y=0x00;
	sensor_ptr->awb.win_size.w=40;
	sensor_ptr->awb.win_size.h=30;
	sensor_ptr->awb.quick_mode = 1;
	sensor_ptr->awb.r_gain[0]=0x6c0;
	sensor_ptr->awb.g_gain[0]=0x400;
	sensor_ptr->awb.b_gain[0]=0x600;
	sensor_ptr->awb.r_gain[1]=0x480;
	sensor_ptr->awb.g_gain[1]=0x400;
	sensor_ptr->awb.b_gain[1]=0xc00;
	sensor_ptr->awb.r_gain[2]=0x400;
	sensor_ptr->awb.g_gain[2]=0x400;
	sensor_ptr->awb.b_gain[2]=0x400;
	sensor_ptr->awb.r_gain[3]=0x3fc;
	sensor_ptr->awb.g_gain[3]=0x400;
	sensor_ptr->awb.b_gain[3]=0x400;
	sensor_ptr->awb.r_gain[4]=0x480;
	sensor_ptr->awb.g_gain[4]=0x400;
	sensor_ptr->awb.b_gain[4]=0x800;
	sensor_ptr->awb.r_gain[5]=0x700;
	sensor_ptr->awb.g_gain[5]=0x400;
	sensor_ptr->awb.b_gain[5]=0x500;
	sensor_ptr->awb.r_gain[6]=0xa00;
	sensor_ptr->awb.g_gain[6]=0x400;
	sensor_ptr->awb.b_gain[6]=0x4c0;
	sensor_ptr->awb.r_gain[7]=0x400;
	sensor_ptr->awb.g_gain[7]=0x400;
	sensor_ptr->awb.b_gain[7]=0x400;
	sensor_ptr->awb.r_gain[8]=0x400;
	sensor_ptr->awb.g_gain[8]=0x400;
	sensor_ptr->awb.b_gain[8]=0x400;
	sensor_ptr->awb.target_zone=0x10;

	/*awb win*/
	sensor_ptr->awb.win[0].x=135;
	sensor_ptr->awb.win[0].yt=232;
	sensor_ptr->awb.win[0].yb=219;

	sensor_ptr->awb.win[1].x=139;
	sensor_ptr->awb.win[1].yt=254;
	sensor_ptr->awb.win[1].yb=193;

	sensor_ptr->awb.win[2].x=145;
	sensor_ptr->awb.win[2].yt=259;
	sensor_ptr->awb.win[2].yb=170;

	sensor_ptr->awb.win[3].x=155;
	sensor_ptr->awb.win[3].yt=259;
	sensor_ptr->awb.win[3].yb=122;

	sensor_ptr->awb.win[4].x=162;
	sensor_ptr->awb.win[4].yt=256;
	sensor_ptr->awb.win[4].yb=112;

	sensor_ptr->awb.win[5].x=172;
	sensor_ptr->awb.win[5].yt=230;
	sensor_ptr->awb.win[5].yb=110;

	sensor_ptr->awb.win[6].x=180;
	sensor_ptr->awb.win[6].yt=195;
	sensor_ptr->awb.win[6].yb=114;

	sensor_ptr->awb.win[7].x=184;
	sensor_ptr->awb.win[7].yt=185;
	sensor_ptr->awb.win[7].yb=120;

	sensor_ptr->awb.win[8].x=190;
	sensor_ptr->awb.win[8].yt=179;
	sensor_ptr->awb.win[8].yb=128;

	sensor_ptr->awb.win[9].x=199;
	sensor_ptr->awb.win[9].yt=175;
	sensor_ptr->awb.win[9].yb=131;

	sensor_ptr->awb.win[10].x=205;
	sensor_ptr->awb.win[10].yt=172;
	sensor_ptr->awb.win[10].yb=129;

	sensor_ptr->awb.win[11].x=210;
	sensor_ptr->awb.win[11].yt=169;
	sensor_ptr->awb.win[11].yb=123;

	sensor_ptr->awb.win[12].x=215;
	sensor_ptr->awb.win[12].yt=166;
	sensor_ptr->awb.win[12].yb=112;

	sensor_ptr->awb.win[13].x=226;
	sensor_ptr->awb.win[13].yt=159;
	sensor_ptr->awb.win[13].yb=98;

	sensor_ptr->awb.win[14].x=234;
	sensor_ptr->awb.win[14].yt=153;
	sensor_ptr->awb.win[14].yb=92;

	sensor_ptr->awb.win[15].x=248;
	sensor_ptr->awb.win[15].yt=144;
	sensor_ptr->awb.win[15].yb=84;

	sensor_ptr->awb.win[16].x=265;
	sensor_ptr->awb.win[16].yt=133;
	sensor_ptr->awb.win[16].yb=81;

	sensor_ptr->awb.win[17].x=277;
	sensor_ptr->awb.win[17].yt=126;
	sensor_ptr->awb.win[17].yb=79;

	sensor_ptr->awb.win[18].x=291;
	sensor_ptr->awb.win[18].yt=119;
	sensor_ptr->awb.win[18].yb=80;

	sensor_ptr->awb.win[19].x=305;
	sensor_ptr->awb.win[19].yt=109;
	sensor_ptr->awb.win[19].yb=90;

	sensor_ptr->awb.gain_convert[0].r=0x100;
	sensor_ptr->awb.gain_convert[0].g=0x100;
	sensor_ptr->awb.gain_convert[0].b=0x100;

	sensor_ptr->awb.gain_convert[1].r=0x100;
	sensor_ptr->awb.gain_convert[1].g=0x100;
	sensor_ptr->awb.gain_convert[1].b=0x100;

	//c2580 awb param
	sensor_ptr->awb.t_func.a = 274;
	sensor_ptr->awb.t_func.b = -335;
	sensor_ptr->awb.t_func.shift = 10;

	sensor_ptr->awb.wp_count_range.min_proportion = 256 / 128;
	sensor_ptr->awb.wp_count_range.max_proportion = 256 / 4;

	sensor_ptr->awb.g_estimate.num = 4;
	sensor_ptr->awb.g_estimate.t_thr[0] = 2000;
	sensor_ptr->awb.g_estimate.g_thr[0][0] = 406;    //0.404
	sensor_ptr->awb.g_estimate.g_thr[0][1] = 419;    //0.414
	sensor_ptr->awb.g_estimate.w_thr[0][0] = 255;
	sensor_ptr->awb.g_estimate.w_thr[0][1] = 0;

	sensor_ptr->awb.g_estimate.t_thr[1] = 3000;
	sensor_ptr->awb.g_estimate.g_thr[1][0] = 406;    //0.404
	sensor_ptr->awb.g_estimate.g_thr[1][1] = 419;    //0.414
	sensor_ptr->awb.g_estimate.w_thr[1][0] = 255;
	sensor_ptr->awb.g_estimate.w_thr[1][1] = 0;

	sensor_ptr->awb.g_estimate.t_thr[2] = 6500;
	sensor_ptr->awb.g_estimate.g_thr[2][0] = 445;
	sensor_ptr->awb.g_estimate.g_thr[2][1] = 478;
	sensor_ptr->awb.g_estimate.w_thr[2][0] = 255;
	sensor_ptr->awb.g_estimate.w_thr[2][1] = 0;

	sensor_ptr->awb.g_estimate.t_thr[3] = 20000;
	sensor_ptr->awb.g_estimate.g_thr[3][0] = 407;
	sensor_ptr->awb.g_estimate.g_thr[3][1] = 414;
	sensor_ptr->awb.g_estimate.w_thr[3][0] = 255;
	sensor_ptr->awb.g_estimate.w_thr[3][1] = 0;

	sensor_ptr->awb.gain_adjust.num = 5;
	sensor_ptr->awb.gain_adjust.t_thr[0] = 1600;
	sensor_ptr->awb.gain_adjust.w_thr[0] = 192;
	sensor_ptr->awb.gain_adjust.t_thr[1] = 2200;
	sensor_ptr->awb.gain_adjust.w_thr[1] = 208;
	sensor_ptr->awb.gain_adjust.t_thr[2] = 3500;
	sensor_ptr->awb.gain_adjust.w_thr[2] = 256;
	sensor_ptr->awb.gain_adjust.t_thr[3] = 10000;
	sensor_ptr->awb.gain_adjust.w_thr[3] = 256;
	sensor_ptr->awb.gain_adjust.t_thr[4] = 12000;
	sensor_ptr->awb.gain_adjust.w_thr[4] = 128;

	sensor_ptr->awb.light.num = 7;
	sensor_ptr->awb.light.t_thr[0] = 2300;
	sensor_ptr->awb.light.w_thr[0] = 2;
	sensor_ptr->awb.light.t_thr[1] = 2850;
	sensor_ptr->awb.light.w_thr[1] = 4;
	sensor_ptr->awb.light.t_thr[2] = 4150;
	sensor_ptr->awb.light.w_thr[2] = 8;
	sensor_ptr->awb.light.t_thr[3] = 5500;
	sensor_ptr->awb.light.w_thr[3] = 160;
	sensor_ptr->awb.light.t_thr[4] = 6500;
	sensor_ptr->awb.light.w_thr[4] = 192;
	sensor_ptr->awb.light.t_thr[5] = 7500;
	sensor_ptr->awb.light.w_thr[5] = 96;
	sensor_ptr->awb.light.t_thr[6] = 8200;
	sensor_ptr->awb.light.w_thr[6] = 8;

	sensor_ptr->awb.steady_speed = 6;
	sensor_ptr->awb.debug_level = 2;
	sensor_ptr->awb.smart = 1;
	sensor_ptr->awb.alg_id = 0;
	sensor_ptr->awb.smart_index = 0;
	//bpc
	sensor_ptr->bpc.flat_thr=80;
	sensor_ptr->bpc.std_thr=20;
	sensor_ptr->bpc.texture_thr=2;

	// denoise
	sensor_ptr->denoise.write_back=0x00;
	sensor_ptr->denoise.r_thr=0x08;
	sensor_ptr->denoise.g_thr=0x08;
	sensor_ptr->denoise.b_thr=0x08;

	sensor_ptr->denoise.diswei[0]=255;
	sensor_ptr->denoise.diswei[1]=253;
	sensor_ptr->denoise.diswei[2]=251;
	sensor_ptr->denoise.diswei[3]=249;
	sensor_ptr->denoise.diswei[4]=247;
	sensor_ptr->denoise.diswei[5]=245;
	sensor_ptr->denoise.diswei[6]=243;
	sensor_ptr->denoise.diswei[7]=241;
	sensor_ptr->denoise.diswei[8]=239;
	sensor_ptr->denoise.diswei[9]=237;
	sensor_ptr->denoise.diswei[10]=235;
	sensor_ptr->denoise.diswei[11]=234;
	sensor_ptr->denoise.diswei[12]=232;
	sensor_ptr->denoise.diswei[13]=230;
	sensor_ptr->denoise.diswei[14]=228;
	sensor_ptr->denoise.diswei[15]=226;
	sensor_ptr->denoise.diswei[16]=225;
	sensor_ptr->denoise.diswei[17]=223;
	sensor_ptr->denoise.diswei[18]=221;

	sensor_ptr->denoise.ranwei[0]=255;
	sensor_ptr->denoise.ranwei[1]=252;
	sensor_ptr->denoise.ranwei[2]=243;
	sensor_ptr->denoise.ranwei[3]=230;
	sensor_ptr->denoise.ranwei[4]=213;
	sensor_ptr->denoise.ranwei[5]=193;
	sensor_ptr->denoise.ranwei[6]=170;
	sensor_ptr->denoise.ranwei[7]=147;
	sensor_ptr->denoise.ranwei[8]=125;
	sensor_ptr->denoise.ranwei[9]=103;
	sensor_ptr->denoise.ranwei[10]=83;
	sensor_ptr->denoise.ranwei[11]=66;
	sensor_ptr->denoise.ranwei[12]=51;
	sensor_ptr->denoise.ranwei[13]=38;
	sensor_ptr->denoise.ranwei[14]=28;
	sensor_ptr->denoise.ranwei[15]=20;
	sensor_ptr->denoise.ranwei[16]=14;
	sensor_ptr->denoise.ranwei[17]=10;
	sensor_ptr->denoise.ranwei[18]=6;
	sensor_ptr->denoise.ranwei[19]=4;
	sensor_ptr->denoise.ranwei[20]=2;
	sensor_ptr->denoise.ranwei[21]=1;
	sensor_ptr->denoise.ranwei[22]=0;
	sensor_ptr->denoise.ranwei[23]=0;
	sensor_ptr->denoise.ranwei[24]=0;
	sensor_ptr->denoise.ranwei[25]=0;
	sensor_ptr->denoise.ranwei[26]=0;
	sensor_ptr->denoise.ranwei[27]=0;
	sensor_ptr->denoise.ranwei[28]=0;
	sensor_ptr->denoise.ranwei[29]=0;
	sensor_ptr->denoise.ranwei[30]=0;

	//GrGb
	sensor_ptr->grgb.edge_thr=26;
	sensor_ptr->grgb.diff_thr=80;

	//cfa
	sensor_ptr->cfa.edge_thr=0x1a;
	sensor_ptr->cfa.diff_thr=0x00;

	//cmc
	sensor_ptr->cmc.matrix[0][0]=0x6f3;
	sensor_ptr->cmc.matrix[0][1]=0x3e0a;
	sensor_ptr->cmc.matrix[0][2]=0x3f03;
	sensor_ptr->cmc.matrix[0][3]=0x3ec0;
	sensor_ptr->cmc.matrix[0][4]=0x693;
	sensor_ptr->cmc.matrix[0][5]=0x3eae;
	sensor_ptr->cmc.matrix[0][6]=0x0d;
	sensor_ptr->cmc.matrix[0][7]=0x3c03;
	sensor_ptr->cmc.matrix[0][8]=0x7f0;

	//Gamma
	sensor_ptr->gamma.axis[0][0]=0;
	sensor_ptr->gamma.axis[0][1]=8;
	sensor_ptr->gamma.axis[0][2]=16;
	sensor_ptr->gamma.axis[0][3]=24;
	sensor_ptr->gamma.axis[0][4]=32;
	sensor_ptr->gamma.axis[0][5]=48;
	sensor_ptr->gamma.axis[0][6]=64;
	sensor_ptr->gamma.axis[0][7]=80;
	sensor_ptr->gamma.axis[0][8]=96;
	sensor_ptr->gamma.axis[0][9]=128;
	sensor_ptr->gamma.axis[0][10]=160;
	sensor_ptr->gamma.axis[0][11]=192;
	sensor_ptr->gamma.axis[0][12]=224;
	sensor_ptr->gamma.axis[0][13]=256;
	sensor_ptr->gamma.axis[0][14]=288;
	sensor_ptr->gamma.axis[0][15]=320;
	sensor_ptr->gamma.axis[0][16]=384;
	sensor_ptr->gamma.axis[0][17]=448;
	sensor_ptr->gamma.axis[0][18]=512;
	sensor_ptr->gamma.axis[0][19]=576;
	sensor_ptr->gamma.axis[0][20]=640;
	sensor_ptr->gamma.axis[0][21]=768;
	sensor_ptr->gamma.axis[0][22]=832;
	sensor_ptr->gamma.axis[0][23]=896;
	sensor_ptr->gamma.axis[0][24]=960;
	sensor_ptr->gamma.axis[0][25]=1023;

	sensor_ptr->gamma.axis[1][0]=0x00;
	sensor_ptr->gamma.axis[1][1]=0x05;
	sensor_ptr->gamma.axis[1][2]=0x09;
	sensor_ptr->gamma.axis[1][3]=0x0e;
	sensor_ptr->gamma.axis[1][4]=0x13;
	sensor_ptr->gamma.axis[1][5]=0x1f;
	sensor_ptr->gamma.axis[1][6]=0x2a;
	sensor_ptr->gamma.axis[1][7]=0x36;
	sensor_ptr->gamma.axis[1][8]=0x40;
	sensor_ptr->gamma.axis[1][9]=0x58;
	sensor_ptr->gamma.axis[1][10]=0x68;
	sensor_ptr->gamma.axis[1][11]=0x76;
	sensor_ptr->gamma.axis[1][12]=0x84;
	sensor_ptr->gamma.axis[1][13]=0x8f;
	sensor_ptr->gamma.axis[1][14]=0x98;
	sensor_ptr->gamma.axis[1][15]=0xa0;
	sensor_ptr->gamma.axis[1][16]=0xb0;
	sensor_ptr->gamma.axis[1][17]=0xbd;
	sensor_ptr->gamma.axis[1][18]=0xc6;
	sensor_ptr->gamma.axis[1][19]=0xcf;
	sensor_ptr->gamma.axis[1][20]=0xd8;
	sensor_ptr->gamma.axis[1][21]=0xe4;
	sensor_ptr->gamma.axis[1][22]=0xea;
	sensor_ptr->gamma.axis[1][23]=0xf0;
	sensor_ptr->gamma.axis[1][24]=0xf6;
	sensor_ptr->gamma.axis[1][25]=0xff;

	sensor_ptr->gamma.tab[0].axis[0][0]=0;
	sensor_ptr->gamma.tab[0].axis[0][1]=8;
	sensor_ptr->gamma.tab[0].axis[0][2]=16;
	sensor_ptr->gamma.tab[0].axis[0][3]=24;
	sensor_ptr->gamma.tab[0].axis[0][4]=32;
	sensor_ptr->gamma.tab[0].axis[0][5]=48;
	sensor_ptr->gamma.tab[0].axis[0][6]=64;
	sensor_ptr->gamma.tab[0].axis[0][7]=80;
	sensor_ptr->gamma.tab[0].axis[0][8]=96;
	sensor_ptr->gamma.tab[0].axis[0][9]=128;
	sensor_ptr->gamma.tab[0].axis[0][10]=160;
	sensor_ptr->gamma.tab[0].axis[0][11]=192;
	sensor_ptr->gamma.tab[0].axis[0][12]=224;
	sensor_ptr->gamma.tab[0].axis[0][13]=256;
	sensor_ptr->gamma.tab[0].axis[0][14]=288;
	sensor_ptr->gamma.tab[0].axis[0][15]=320;
	sensor_ptr->gamma.tab[0].axis[0][16]=384;
	sensor_ptr->gamma.tab[0].axis[0][17]=448;
	sensor_ptr->gamma.tab[0].axis[0][18]=512;
	sensor_ptr->gamma.tab[0].axis[0][19]=576;
	sensor_ptr->gamma.tab[0].axis[0][20]=640;
	sensor_ptr->gamma.tab[0].axis[0][21]=768;
	sensor_ptr->gamma.tab[0].axis[0][22]=832;
	sensor_ptr->gamma.tab[0].axis[0][23]=896;
	sensor_ptr->gamma.tab[0].axis[0][24]=960;
	sensor_ptr->gamma.tab[0].axis[0][25]=1023;

	sensor_ptr->gamma.tab[0].axis[1][0]=0x00;
	sensor_ptr->gamma.tab[0].axis[1][1]=0x05;
	sensor_ptr->gamma.tab[0].axis[1][2]=0x09;
	sensor_ptr->gamma.tab[0].axis[1][3]=0x0e;
	sensor_ptr->gamma.tab[0].axis[1][4]=0x13;
	sensor_ptr->gamma.tab[0].axis[1][5]=0x1f;
	sensor_ptr->gamma.tab[0].axis[1][6]=0x2a;
	sensor_ptr->gamma.tab[0].axis[1][7]=0x36;
	sensor_ptr->gamma.tab[0].axis[1][8]=0x40;
	sensor_ptr->gamma.tab[0].axis[1][9]=0x58;
	sensor_ptr->gamma.tab[0].axis[1][10]=0x68;
	sensor_ptr->gamma.tab[0].axis[1][11]=0x76;
	sensor_ptr->gamma.tab[0].axis[1][12]=0x84;
	sensor_ptr->gamma.tab[0].axis[1][13]=0x8f;
	sensor_ptr->gamma.tab[0].axis[1][14]=0x98;
	sensor_ptr->gamma.tab[0].axis[1][15]=0xa0;
	sensor_ptr->gamma.tab[0].axis[1][16]=0xb0;
	sensor_ptr->gamma.tab[0].axis[1][17]=0xbd;
	sensor_ptr->gamma.tab[0].axis[1][18]=0xc6;
	sensor_ptr->gamma.tab[0].axis[1][19]=0xcf;
	sensor_ptr->gamma.tab[0].axis[1][20]=0xd8;
	sensor_ptr->gamma.tab[0].axis[1][21]=0xe4;
	sensor_ptr->gamma.tab[0].axis[1][22]=0xea;
	sensor_ptr->gamma.tab[0].axis[1][23]=0xf0;
	sensor_ptr->gamma.tab[0].axis[1][24]=0xf6;
	sensor_ptr->gamma.tab[0].axis[1][25]=0xff;

	sensor_ptr->gamma.tab[1].axis[0][0]=0;
	sensor_ptr->gamma.tab[1].axis[0][1]=8;
	sensor_ptr->gamma.tab[1].axis[0][2]=16;
	sensor_ptr->gamma.tab[1].axis[0][3]=24;
	sensor_ptr->gamma.tab[1].axis[0][4]=32;
	sensor_ptr->gamma.tab[1].axis[0][5]=48;
	sensor_ptr->gamma.tab[1].axis[0][6]=64;
	sensor_ptr->gamma.tab[1].axis[0][7]=80;
	sensor_ptr->gamma.tab[1].axis[0][8]=96;
	sensor_ptr->gamma.tab[1].axis[0][9]=128;
	sensor_ptr->gamma.tab[1].axis[0][10]=160;
	sensor_ptr->gamma.tab[1].axis[0][11]=192;
	sensor_ptr->gamma.tab[1].axis[0][12]=224;
	sensor_ptr->gamma.tab[1].axis[0][13]=256;
	sensor_ptr->gamma.tab[1].axis[0][14]=288;
	sensor_ptr->gamma.tab[1].axis[0][15]=320;
	sensor_ptr->gamma.tab[1].axis[0][16]=384;
	sensor_ptr->gamma.tab[1].axis[0][17]=448;
	sensor_ptr->gamma.tab[1].axis[0][18]=512;
	sensor_ptr->gamma.tab[1].axis[0][19]=576;
	sensor_ptr->gamma.tab[1].axis[0][20]=640;
	sensor_ptr->gamma.tab[1].axis[0][21]=768;
	sensor_ptr->gamma.tab[1].axis[0][22]=832;
	sensor_ptr->gamma.tab[1].axis[0][23]=896;
	sensor_ptr->gamma.tab[1].axis[0][24]=960;
	sensor_ptr->gamma.tab[1].axis[0][25]=1023;

	sensor_ptr->gamma.tab[1].axis[1][0]=0x00;
	sensor_ptr->gamma.tab[1].axis[1][1]=0x05;
	sensor_ptr->gamma.tab[1].axis[1][2]=0x09;
	sensor_ptr->gamma.tab[1].axis[1][3]=0x0e;
	sensor_ptr->gamma.tab[1].axis[1][4]=0x13;
	sensor_ptr->gamma.tab[1].axis[1][5]=0x1f;
	sensor_ptr->gamma.tab[1].axis[1][6]=0x2a;
	sensor_ptr->gamma.tab[1].axis[1][7]=0x36;
	sensor_ptr->gamma.tab[1].axis[1][8]=0x40;
	sensor_ptr->gamma.tab[1].axis[1][9]=0x58;
	sensor_ptr->gamma.tab[1].axis[1][10]=0x68;
	sensor_ptr->gamma.tab[1].axis[1][11]=0x76;
	sensor_ptr->gamma.tab[1].axis[1][12]=0x84;
	sensor_ptr->gamma.tab[1].axis[1][13]=0x8f;
	sensor_ptr->gamma.tab[1].axis[1][14]=0x98;
	sensor_ptr->gamma.tab[1].axis[1][15]=0xa0;
	sensor_ptr->gamma.tab[1].axis[1][16]=0xb0;
	sensor_ptr->gamma.tab[1].axis[1][17]=0xbd;
	sensor_ptr->gamma.tab[1].axis[1][18]=0xc6;
	sensor_ptr->gamma.tab[1].axis[1][19]=0xcf;
	sensor_ptr->gamma.tab[1].axis[1][20]=0xd8;
	sensor_ptr->gamma.tab[1].axis[1][21]=0xe4;
	sensor_ptr->gamma.tab[1].axis[1][22]=0xea;
	sensor_ptr->gamma.tab[1].axis[1][23]=0xf0;
	sensor_ptr->gamma.tab[1].axis[1][24]=0xf6;
	sensor_ptr->gamma.tab[1].axis[1][25]=0xff;

	sensor_ptr->gamma.tab[2].axis[0][0]=0;
	sensor_ptr->gamma.tab[2].axis[0][1]=8;
	sensor_ptr->gamma.tab[2].axis[0][2]=16;
	sensor_ptr->gamma.tab[2].axis[0][3]=24;
	sensor_ptr->gamma.tab[2].axis[0][4]=32;
	sensor_ptr->gamma.tab[2].axis[0][5]=48;
	sensor_ptr->gamma.tab[2].axis[0][6]=64;
	sensor_ptr->gamma.tab[2].axis[0][7]=80;
	sensor_ptr->gamma.tab[2].axis[0][8]=96;
	sensor_ptr->gamma.tab[2].axis[0][9]=128;
	sensor_ptr->gamma.tab[2].axis[0][10]=160;
	sensor_ptr->gamma.tab[2].axis[0][11]=192;
	sensor_ptr->gamma.tab[2].axis[0][12]=224;
	sensor_ptr->gamma.tab[2].axis[0][13]=256;
	sensor_ptr->gamma.tab[2].axis[0][14]=288;
	sensor_ptr->gamma.tab[2].axis[0][15]=320;
	sensor_ptr->gamma.tab[2].axis[0][16]=384;
	sensor_ptr->gamma.tab[2].axis[0][17]=448;
	sensor_ptr->gamma.tab[2].axis[0][18]=512;
	sensor_ptr->gamma.tab[2].axis[0][19]=576;
	sensor_ptr->gamma.tab[2].axis[0][20]=640;
	sensor_ptr->gamma.tab[2].axis[0][21]=768;
	sensor_ptr->gamma.tab[2].axis[0][22]=832;
	sensor_ptr->gamma.tab[2].axis[0][23]=896;
	sensor_ptr->gamma.tab[2].axis[0][24]=960;
	sensor_ptr->gamma.tab[2].axis[0][25]=1023;

	sensor_ptr->gamma.tab[2].axis[1][0]=0x00;
	sensor_ptr->gamma.tab[2].axis[1][1]=0x05;
	sensor_ptr->gamma.tab[2].axis[1][2]=0x09;
	sensor_ptr->gamma.tab[2].axis[1][3]=0x0e;
	sensor_ptr->gamma.tab[2].axis[1][4]=0x13;
	sensor_ptr->gamma.tab[2].axis[1][5]=0x1f;
	sensor_ptr->gamma.tab[2].axis[1][6]=0x2a;
	sensor_ptr->gamma.tab[2].axis[1][7]=0x36;
	sensor_ptr->gamma.tab[2].axis[1][8]=0x40;
	sensor_ptr->gamma.tab[2].axis[1][9]=0x58;
	sensor_ptr->gamma.tab[2].axis[1][10]=0x68;
	sensor_ptr->gamma.tab[2].axis[1][11]=0x76;
	sensor_ptr->gamma.tab[2].axis[1][12]=0x84;
	sensor_ptr->gamma.tab[2].axis[1][13]=0x8f;
	sensor_ptr->gamma.tab[2].axis[1][14]=0x98;
	sensor_ptr->gamma.tab[2].axis[1][15]=0xa0;
	sensor_ptr->gamma.tab[2].axis[1][16]=0xb0;
	sensor_ptr->gamma.tab[2].axis[1][17]=0xbd;
	sensor_ptr->gamma.tab[2].axis[1][18]=0xc6;
	sensor_ptr->gamma.tab[2].axis[1][19]=0xcf;
	sensor_ptr->gamma.tab[2].axis[1][20]=0xd8;
	sensor_ptr->gamma.tab[2].axis[1][21]=0xe4;
	sensor_ptr->gamma.tab[2].axis[1][22]=0xea;
	sensor_ptr->gamma.tab[2].axis[1][23]=0xf0;
	sensor_ptr->gamma.tab[2].axis[1][24]=0xf6;
	sensor_ptr->gamma.tab[2].axis[1][25]=0xff;

	sensor_ptr->gamma.tab[3].axis[0][0]=0;
	sensor_ptr->gamma.tab[3].axis[0][1]=8;
	sensor_ptr->gamma.tab[3].axis[0][2]=16;
	sensor_ptr->gamma.tab[3].axis[0][3]=24;
	sensor_ptr->gamma.tab[3].axis[0][4]=32;
	sensor_ptr->gamma.tab[3].axis[0][5]=48;
	sensor_ptr->gamma.tab[3].axis[0][6]=64;
	sensor_ptr->gamma.tab[3].axis[0][7]=80;
	sensor_ptr->gamma.tab[3].axis[0][8]=96;
	sensor_ptr->gamma.tab[3].axis[0][9]=128;
	sensor_ptr->gamma.tab[3].axis[0][10]=160;
	sensor_ptr->gamma.tab[3].axis[0][11]=192;
	sensor_ptr->gamma.tab[3].axis[0][12]=224;
	sensor_ptr->gamma.tab[3].axis[0][13]=256;
	sensor_ptr->gamma.tab[3].axis[0][14]=288;
	sensor_ptr->gamma.tab[3].axis[0][15]=320;
	sensor_ptr->gamma.tab[3].axis[0][16]=384;
	sensor_ptr->gamma.tab[3].axis[0][17]=448;
	sensor_ptr->gamma.tab[3].axis[0][18]=512;
	sensor_ptr->gamma.tab[3].axis[0][19]=576;
	sensor_ptr->gamma.tab[3].axis[0][20]=640;
	sensor_ptr->gamma.tab[3].axis[0][21]=768;
	sensor_ptr->gamma.tab[3].axis[0][22]=832;
	sensor_ptr->gamma.tab[3].axis[0][23]=896;
	sensor_ptr->gamma.tab[3].axis[0][24]=960;
	sensor_ptr->gamma.tab[3].axis[0][25]=1023;

	sensor_ptr->gamma.tab[3].axis[1][0]=0x00;
	sensor_ptr->gamma.tab[3].axis[1][1]=0x05;
	sensor_ptr->gamma.tab[3].axis[1][2]=0x09;
	sensor_ptr->gamma.tab[3].axis[1][3]=0x0e;
	sensor_ptr->gamma.tab[3].axis[1][4]=0x13;
	sensor_ptr->gamma.tab[3].axis[1][5]=0x1f;
	sensor_ptr->gamma.tab[3].axis[1][6]=0x2a;
	sensor_ptr->gamma.tab[3].axis[1][7]=0x36;
	sensor_ptr->gamma.tab[3].axis[1][8]=0x40;
	sensor_ptr->gamma.tab[3].axis[1][9]=0x58;
	sensor_ptr->gamma.tab[3].axis[1][10]=0x68;
	sensor_ptr->gamma.tab[3].axis[1][11]=0x76;
	sensor_ptr->gamma.tab[3].axis[1][12]=0x84;
	sensor_ptr->gamma.tab[3].axis[1][13]=0x8f;
	sensor_ptr->gamma.tab[3].axis[1][14]=0x98;
	sensor_ptr->gamma.tab[3].axis[1][15]=0xa0;
	sensor_ptr->gamma.tab[3].axis[1][16]=0xb0;
	sensor_ptr->gamma.tab[3].axis[1][17]=0xbd;
	sensor_ptr->gamma.tab[3].axis[1][18]=0xc6;
	sensor_ptr->gamma.tab[3].axis[1][19]=0xcf;
	sensor_ptr->gamma.tab[3].axis[1][20]=0xd8;
	sensor_ptr->gamma.tab[3].axis[1][21]=0xe4;
	sensor_ptr->gamma.tab[3].axis[1][22]=0xea;
	sensor_ptr->gamma.tab[3].axis[1][23]=0xf0;
	sensor_ptr->gamma.tab[3].axis[1][24]=0xf6;
	sensor_ptr->gamma.tab[3].axis[1][25]=0xff;

	sensor_ptr->gamma.tab[4].axis[0][0]=0;
	sensor_ptr->gamma.tab[4].axis[0][1]=8;
	sensor_ptr->gamma.tab[4].axis[0][2]=16;
	sensor_ptr->gamma.tab[4].axis[0][3]=24;
	sensor_ptr->gamma.tab[4].axis[0][4]=32;
	sensor_ptr->gamma.tab[4].axis[0][5]=48;
	sensor_ptr->gamma.tab[4].axis[0][6]=64;
	sensor_ptr->gamma.tab[4].axis[0][7]=80;
	sensor_ptr->gamma.tab[4].axis[0][8]=96;
	sensor_ptr->gamma.tab[4].axis[0][9]=128;
	sensor_ptr->gamma.tab[4].axis[0][10]=160;
	sensor_ptr->gamma.tab[4].axis[0][11]=192;
	sensor_ptr->gamma.tab[4].axis[0][12]=224;
	sensor_ptr->gamma.tab[4].axis[0][13]=256;
	sensor_ptr->gamma.tab[4].axis[0][14]=288;
	sensor_ptr->gamma.tab[4].axis[0][15]=320;
	sensor_ptr->gamma.tab[4].axis[0][16]=384;
	sensor_ptr->gamma.tab[4].axis[0][17]=448;
	sensor_ptr->gamma.tab[4].axis[0][18]=512;
	sensor_ptr->gamma.tab[4].axis[0][19]=576;
	sensor_ptr->gamma.tab[4].axis[0][20]=640;
	sensor_ptr->gamma.tab[4].axis[0][21]=768;
	sensor_ptr->gamma.tab[4].axis[0][22]=832;
	sensor_ptr->gamma.tab[4].axis[0][23]=896;
	sensor_ptr->gamma.tab[4].axis[0][24]=960;
	sensor_ptr->gamma.tab[4].axis[0][25]=1023;

	sensor_ptr->gamma.tab[4].axis[1][0]=0x00;
	sensor_ptr->gamma.tab[4].axis[1][1]=0x05;
	sensor_ptr->gamma.tab[4].axis[1][2]=0x09;
	sensor_ptr->gamma.tab[4].axis[1][3]=0x0e;
	sensor_ptr->gamma.tab[4].axis[1][4]=0x13;
	sensor_ptr->gamma.tab[4].axis[1][5]=0x1f;
	sensor_ptr->gamma.tab[4].axis[1][6]=0x2a;
	sensor_ptr->gamma.tab[4].axis[1][7]=0x36;
	sensor_ptr->gamma.tab[4].axis[1][8]=0x40;
	sensor_ptr->gamma.tab[4].axis[1][9]=0x58;
	sensor_ptr->gamma.tab[4].axis[1][10]=0x68;
	sensor_ptr->gamma.tab[4].axis[1][11]=0x76;
	sensor_ptr->gamma.tab[4].axis[1][12]=0x84;
	sensor_ptr->gamma.tab[4].axis[1][13]=0x8f;
	sensor_ptr->gamma.tab[4].axis[1][14]=0x98;
	sensor_ptr->gamma.tab[4].axis[1][15]=0xa0;
	sensor_ptr->gamma.tab[4].axis[1][16]=0xb0;
	sensor_ptr->gamma.tab[4].axis[1][17]=0xbd;
	sensor_ptr->gamma.tab[4].axis[1][18]=0xc6;
	sensor_ptr->gamma.tab[4].axis[1][19]=0xcf;
	sensor_ptr->gamma.tab[4].axis[1][20]=0xd8;
	sensor_ptr->gamma.tab[4].axis[1][21]=0xe4;
	sensor_ptr->gamma.tab[4].axis[1][22]=0xea;
	sensor_ptr->gamma.tab[4].axis[1][23]=0xf0;
	sensor_ptr->gamma.tab[4].axis[1][24]=0xf6;
	sensor_ptr->gamma.tab[4].axis[1][25]=0xff;

	//uv div
	sensor_ptr->uv_div.thrd[0]=252;
	sensor_ptr->uv_div.thrd[1]=250;
	sensor_ptr->uv_div.thrd[2]=248;
	sensor_ptr->uv_div.thrd[3]=246;
	sensor_ptr->uv_div.thrd[4]=244;
	sensor_ptr->uv_div.thrd[5]=242;
	sensor_ptr->uv_div.thrd[6]=240;

	//pref
	sensor_ptr->pref.write_back=0x00;
	sensor_ptr->pref.y_thr=0x04;
	sensor_ptr->pref.u_thr=0x04;
	sensor_ptr->pref.v_thr=0x04;

	//bright
	sensor_ptr->bright.factor[0]=0xd0;
	sensor_ptr->bright.factor[1]=0xe0;
	sensor_ptr->bright.factor[2]=0xf0;
	sensor_ptr->bright.factor[3]=0x00;
	sensor_ptr->bright.factor[4]=0x10;
	sensor_ptr->bright.factor[5]=0x20;
	sensor_ptr->bright.factor[6]=0x30;
	sensor_ptr->bright.factor[7]=0x00;
	sensor_ptr->bright.factor[8]=0x00;
	sensor_ptr->bright.factor[9]=0x00;
	sensor_ptr->bright.factor[10]=0x00;
	sensor_ptr->bright.factor[11]=0x00;
	sensor_ptr->bright.factor[12]=0x00;
	sensor_ptr->bright.factor[13]=0x00;
	sensor_ptr->bright.factor[14]=0x00;
	sensor_ptr->bright.factor[15]=0x00;

	//contrast
	sensor_ptr->contrast.factor[0]=0x10;
	sensor_ptr->contrast.factor[1]=0x20;
	sensor_ptr->contrast.factor[2]=0x30;
	sensor_ptr->contrast.factor[3]=0x40;
	sensor_ptr->contrast.factor[4]=0x50;
	sensor_ptr->contrast.factor[5]=0x60;
	sensor_ptr->contrast.factor[6]=0x70;
	sensor_ptr->contrast.factor[7]=0x40;
	sensor_ptr->contrast.factor[8]=0x40;
	sensor_ptr->contrast.factor[9]=0x40;
	sensor_ptr->contrast.factor[10]=0x40;
	sensor_ptr->contrast.factor[11]=0x40;
	sensor_ptr->contrast.factor[12]=0x40;
	sensor_ptr->contrast.factor[13]=0x40;
	sensor_ptr->contrast.factor[14]=0x40;
	sensor_ptr->contrast.factor[15]=0x40;

	//hist
	sensor_ptr->hist.mode;
	sensor_ptr->hist.low_ratio;
	sensor_ptr->hist.high_ratio;

	//auto contrast
	sensor_ptr->auto_contrast.mode;

	//saturation
	sensor_ptr->saturation.factor[0]=0x28;
	sensor_ptr->saturation.factor[1]=0x30;
	sensor_ptr->saturation.factor[2]=0x38;
	sensor_ptr->saturation.factor[3]=0x40;
	sensor_ptr->saturation.factor[4]=0x48;
	sensor_ptr->saturation.factor[5]=0x50;
	sensor_ptr->saturation.factor[6]=0x58;
	sensor_ptr->saturation.factor[7]=0x40;
	sensor_ptr->saturation.factor[8]=0x40;
	sensor_ptr->saturation.factor[9]=0x40;
	sensor_ptr->saturation.factor[10]=0x40;
	sensor_ptr->saturation.factor[11]=0x40;
	sensor_ptr->saturation.factor[12]=0x40;
	sensor_ptr->saturation.factor[13]=0x40;
	sensor_ptr->saturation.factor[14]=0x40;
	sensor_ptr->saturation.factor[15]=0x40;

	//css
	sensor_ptr->css.lum_thr=255;
	sensor_ptr->css.chr_thr=2;
	sensor_ptr->css.low_thr[0]=3;
	sensor_ptr->css.low_thr[1]=4;
	sensor_ptr->css.low_thr[2]=5;
	sensor_ptr->css.low_thr[3]=6;
	sensor_ptr->css.low_thr[4]=7;
	sensor_ptr->css.low_thr[5]=8;
	sensor_ptr->css.low_thr[6]=9;
	sensor_ptr->css.low_sum_thr[0]=6;
	sensor_ptr->css.low_sum_thr[1]=8;
	sensor_ptr->css.low_sum_thr[2]=10;
	sensor_ptr->css.low_sum_thr[3]=12;
	sensor_ptr->css.low_sum_thr[4]=14;
	sensor_ptr->css.low_sum_thr[5]=16;
	sensor_ptr->css.low_sum_thr[6]=18;

	//af info
	sensor_ptr->af.max_step=0x3ff;
	sensor_ptr->af.min_step=0;
	sensor_ptr->af.max_tune_step=0;
	sensor_ptr->af.stab_period=120;
	sensor_ptr->af.alg_id=3;
	sensor_ptr->af.rough_count=12;
	sensor_ptr->af.af_rough_step[0]=320;
	sensor_ptr->af.af_rough_step[2]=384;
	sensor_ptr->af.af_rough_step[3]=448;
	sensor_ptr->af.af_rough_step[4]=512;
	sensor_ptr->af.af_rough_step[5]=576;
	sensor_ptr->af.af_rough_step[6]=640;
	sensor_ptr->af.af_rough_step[7]=704;
	sensor_ptr->af.af_rough_step[8]=768;
	sensor_ptr->af.af_rough_step[9]=832;
	sensor_ptr->af.af_rough_step[10]=896;
	sensor_ptr->af.af_rough_step[11]=960;
	sensor_ptr->af.af_rough_step[12]=1023;
	sensor_ptr->af.fine_count=4;

	//edge
	sensor_ptr->edge.info[0].detail_thr=0x00;
	sensor_ptr->edge.info[0].smooth_thr=0x30;
	sensor_ptr->edge.info[0].strength=0;
	sensor_ptr->edge.info[1].detail_thr=0x01;
	sensor_ptr->edge.info[1].smooth_thr=0x20;
	sensor_ptr->edge.info[1].strength=3;
	sensor_ptr->edge.info[2].detail_thr=0x2;
	sensor_ptr->edge.info[2].smooth_thr=0x10;
	sensor_ptr->edge.info[2].strength=5;
	sensor_ptr->edge.info[3].detail_thr=0x03;
	sensor_ptr->edge.info[3].smooth_thr=0x05;
	sensor_ptr->edge.info[3].strength=10;
	sensor_ptr->edge.info[4].detail_thr=0x06;
	sensor_ptr->edge.info[4].smooth_thr=0x05;
	sensor_ptr->edge.info[4].strength=20;
	sensor_ptr->edge.info[5].detail_thr=0x09;
	sensor_ptr->edge.info[5].smooth_thr=0x05;
	sensor_ptr->edge.info[5].strength=30;
	sensor_ptr->edge.info[6].detail_thr=0x0c;
	sensor_ptr->edge.info[6].smooth_thr=0x05;
	sensor_ptr->edge.info[6].strength=40;

	//emboss
	sensor_ptr->emboss.step=0x02;

	//global gain
	sensor_ptr->global.gain=0x40;

	//chn gain
	sensor_ptr->chn.r_gain=0x40;
	sensor_ptr->chn.g_gain=0x40;
	sensor_ptr->chn.b_gain=0x40;
	sensor_ptr->chn.r_offset=0x00;
	sensor_ptr->chn.r_offset=0x00;
	sensor_ptr->chn.r_offset=0x00;

	sensor_ptr->edge.info[0].detail_thr=0x00;
	sensor_ptr->edge.info[0].smooth_thr=0x30;
	sensor_ptr->edge.info[0].strength=0;
	sensor_ptr->edge.info[1].detail_thr=0x01;
	sensor_ptr->edge.info[1].smooth_thr=0x20;
	sensor_ptr->edge.info[1].strength=3;
	sensor_ptr->edge.info[2].detail_thr=0x2;
	sensor_ptr->edge.info[2].smooth_thr=0x10;
	sensor_ptr->edge.info[2].strength=5;
	sensor_ptr->edge.info[3].detail_thr=0x03;
	sensor_ptr->edge.info[3].smooth_thr=0x05;
	sensor_ptr->edge.info[3].strength=10;
	sensor_ptr->edge.info[4].detail_thr=0x06;
	sensor_ptr->edge.info[4].smooth_thr=0x05;
	sensor_ptr->edge.info[4].strength=20;
	sensor_ptr->edge.info[5].detail_thr=0x09;
	sensor_ptr->edge.info[5].smooth_thr=0x05;
	sensor_ptr->edge.info[5].strength=30;
	sensor_ptr->edge.info[6].detail_thr=0x0c;
	sensor_ptr->edge.info[6].smooth_thr=0x05;
	sensor_ptr->edge.info[6].strength=40;
	sensor_ptr->edge.info[7].detail_thr=0x0f;
	sensor_ptr->edge.info[7].smooth_thr=0x05;
	sensor_ptr->edge.info[7].strength=60;

	/*normal*/
	sensor_ptr->special_effect[0].matrix[0]=0x004d;
	sensor_ptr->special_effect[0].matrix[1]=0x0096;
	sensor_ptr->special_effect[0].matrix[2]=0x001d;
	sensor_ptr->special_effect[0].matrix[3]=0xffd5;
	sensor_ptr->special_effect[0].matrix[4]=0xffab;
	sensor_ptr->special_effect[0].matrix[5]=0x0080;
	sensor_ptr->special_effect[0].matrix[6]=0x0080;
	sensor_ptr->special_effect[0].matrix[7]=0xff95;
	sensor_ptr->special_effect[0].matrix[8]=0xffeb;
	sensor_ptr->special_effect[0].y_shift=0xff00;
	sensor_ptr->special_effect[0].u_shift=0x0000;
	sensor_ptr->special_effect[0].v_shift=0x0000;

	/*gray*/
	sensor_ptr->special_effect[1].matrix[0]=0x004d;
	sensor_ptr->special_effect[1].matrix[1]=0x0096;
	sensor_ptr->special_effect[1].matrix[2]=0x001d;
	sensor_ptr->special_effect[1].matrix[3]=0x0000;
	sensor_ptr->special_effect[1].matrix[4]=0x0000;
	sensor_ptr->special_effect[1].matrix[5]=0x0000;
	sensor_ptr->special_effect[1].matrix[6]=0x0000;
	sensor_ptr->special_effect[1].matrix[7]=0x0000;
	sensor_ptr->special_effect[1].matrix[8]=0x0000;
	sensor_ptr->special_effect[1].y_shift=0xff00;
	sensor_ptr->special_effect[1].u_shift=0x0000;
	sensor_ptr->special_effect[1].v_shift=0x0000;
	/*warm*/
	sensor_ptr->special_effect[2].matrix[0]=0x004d;
	sensor_ptr->special_effect[2].matrix[1]=0x0096;
	sensor_ptr->special_effect[2].matrix[2]=0x001d;
	sensor_ptr->special_effect[2].matrix[3]=0xffd5;
	sensor_ptr->special_effect[2].matrix[4]=0xffab;
	sensor_ptr->special_effect[2].matrix[5]=0x0080;
	sensor_ptr->special_effect[2].matrix[6]=0x0080;
	sensor_ptr->special_effect[2].matrix[7]=0xff95;
	sensor_ptr->special_effect[2].matrix[8]=0xffeb;
	sensor_ptr->special_effect[2].y_shift=0xff00;
	sensor_ptr->special_effect[2].u_shift=0xffd4;
	sensor_ptr->special_effect[2].v_shift=0x0080;
	/*green*/
	sensor_ptr->special_effect[3].matrix[0]=0x004d;
	sensor_ptr->special_effect[3].matrix[1]=0x0096;
	sensor_ptr->special_effect[3].matrix[2]=0x001d;
	sensor_ptr->special_effect[3].matrix[3]=0xffd5;
	sensor_ptr->special_effect[3].matrix[4]=0xffab;
	sensor_ptr->special_effect[3].matrix[5]=0x0080;
	sensor_ptr->special_effect[3].matrix[6]=0x0080;
	sensor_ptr->special_effect[3].matrix[7]=0xff95;
	sensor_ptr->special_effect[3].matrix[8]=0xffeb;
	sensor_ptr->special_effect[3].y_shift=0xff00;
	sensor_ptr->special_effect[3].u_shift=0xffd5;
	sensor_ptr->special_effect[3].v_shift=0xffca;
	/*cool*/
	sensor_ptr->special_effect[4].matrix[0]=0x004d;
	sensor_ptr->special_effect[4].matrix[1]=0x0096;
	sensor_ptr->special_effect[4].matrix[2]=0x001d;
	sensor_ptr->special_effect[4].matrix[3]=0xffd5;
	sensor_ptr->special_effect[4].matrix[4]=0xffab;
	sensor_ptr->special_effect[4].matrix[5]=0x0080;
	sensor_ptr->special_effect[4].matrix[6]=0x0080;
	sensor_ptr->special_effect[4].matrix[7]=0xff95;
	sensor_ptr->special_effect[4].matrix[8]=0xffeb;
	sensor_ptr->special_effect[4].y_shift=0xff00;
	sensor_ptr->special_effect[4].u_shift=0x0040;
	sensor_ptr->special_effect[4].v_shift=0x000a;
	/*orange*/
	sensor_ptr->special_effect[5].matrix[0]=0x004d;
	sensor_ptr->special_effect[5].matrix[1]=0x0096;
	sensor_ptr->special_effect[5].matrix[2]=0x001d;
	sensor_ptr->special_effect[5].matrix[3]=0xffd5;
	sensor_ptr->special_effect[5].matrix[4]=0xffab;
	sensor_ptr->special_effect[5].matrix[5]=0x0080;
	sensor_ptr->special_effect[5].matrix[6]=0x0080;
	sensor_ptr->special_effect[5].matrix[7]=0xff95;
	sensor_ptr->special_effect[5].matrix[8]=0xffeb;
	sensor_ptr->special_effect[5].y_shift=0xff00;
	sensor_ptr->special_effect[5].u_shift=0xff00;
	sensor_ptr->special_effect[5].v_shift=0x0028;
	/*negtive*/
	sensor_ptr->special_effect[6].matrix[0]=0xffb3;
	sensor_ptr->special_effect[6].matrix[1]=0xff6a;
	sensor_ptr->special_effect[6].matrix[2]=0xffe3;
	sensor_ptr->special_effect[6].matrix[3]=0x002b;
	sensor_ptr->special_effect[6].matrix[4]=0x0055;
	sensor_ptr->special_effect[6].matrix[5]=0xff80;
	sensor_ptr->special_effect[6].matrix[6]=0xff80;
	sensor_ptr->special_effect[6].matrix[7]=0x006b;
	sensor_ptr->special_effect[6].matrix[8]=0x0015;
	sensor_ptr->special_effect[6].y_shift=0x00ff;
	sensor_ptr->special_effect[6].u_shift=0x0000;
	sensor_ptr->special_effect[6].v_shift=0x0000;
	/*old*/
	sensor_ptr->special_effect[7].matrix[0]=0x004d;
	sensor_ptr->special_effect[7].matrix[1]=0x0096;
	sensor_ptr->special_effect[7].matrix[2]=0x001d;
	sensor_ptr->special_effect[7].matrix[3]=0x0000;
	sensor_ptr->special_effect[7].matrix[4]=0x0000;
	sensor_ptr->special_effect[7].matrix[5]=0x0000;
	sensor_ptr->special_effect[7].matrix[6]=0x0000;
	sensor_ptr->special_effect[7].matrix[7]=0x0000;
	sensor_ptr->special_effect[7].matrix[8]=0x0000;
	sensor_ptr->special_effect[7].y_shift=0xff00;
	sensor_ptr->special_effect[7].u_shift=0xffe2;
	sensor_ptr->special_effect[7].v_shift=0x0028;
#endif

    return rtn;
}

LOCAL unsigned long _c2580_GetResolutionTrimTab(SENSOR_HW_HANDLE handle,
                                                uint32_t param) {
    SENSOR_PRINT("%lu", (unsigned long)s_c2580_Resolution_Trim_Tab);
    return (unsigned long)s_c2580_Resolution_Trim_Tab;
}
LOCAL uint32_t _c2580_PowerOn(SENSOR_HW_HANDLE handle, uint32_t power_on) {
    SENSOR_AVDD_VAL_E dvdd_val = g_c2580_mipi_raw_info.dvdd_val;
    SENSOR_AVDD_VAL_E avdd_val = g_c2580_mipi_raw_info.avdd_val;
    SENSOR_AVDD_VAL_E iovdd_val = g_c2580_mipi_raw_info.iovdd_val;
    BOOLEAN power_down = g_c2580_mipi_raw_info.power_down_level;
    BOOLEAN reset_level = g_c2580_mipi_raw_info.reset_pulse_level;
    // uint32_t reset_width=g_c2580_yuv_info.reset_pulse_width;

    if (SENSOR_TRUE == power_on) {
        // set all power pin to disable status
        Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
        // Sensor_SetMonitorVoltage(SENSOR_AVDD_CLOSED);
        Sensor_SetVoltage(SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED,
                          SENSOR_AVDD_CLOSED);
        Sensor_SetResetLevel(reset_level);
        // Sensor_PowerDown(power_down);
        // usleep(20*1000);
        // step 0 power up DOVDD, the AVDD
        // Sensor_SetMonitorVoltage(SENSOR_AVDD_3300MV);
        Sensor_SetIovddVoltage(iovdd_val);
        usleep(2000);
        Sensor_SetAvddVoltage(avdd_val);
        // usleep(6000);
        // step 1 power up DVDD
        //	Sensor_SetDvddVoltage(dvdd_val);
        //	usleep(6000);
        // step 2 power down pin high
        // Sensor_PowerDown(!power_down);
        usleep(2000);
        // step 3 reset pin high
        Sensor_SetResetLevel(!reset_level);
        usleep(2 * 1000);
        // step 4 xvclk
        Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);
        usleep(4 * 1000);

    } else {
        // power off should start > 1ms after last SCCB
        usleep(4 * 1000);
        // step 1 reset and PWDN
        Sensor_SetResetLevel(reset_level);
        usleep(2000);
        //	Sensor_PowerDown(power_down);
        // usleep(2000);
        // step 2 dvdd
        // Sensor_SetDvddVoltage(SENSOR_AVDD_CLOSED);
        usleep(2000);
        // step 4 xvclk
        Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
        usleep(5000);
        // step 5 AVDD IOVDD
        Sensor_SetAvddVoltage(SENSOR_AVDD_CLOSED);
        usleep(2000);
        Sensor_SetIovddVoltage(SENSOR_AVDD_CLOSED);
        // Sensor_SetMonitorVoltage(SENSOR_AVDD_CLOSED);
    }
    SENSOR_PRINT("SENSOR_c2580: _c2580_Power_On(1:on, 0:off): %d", power_on);
    return SENSOR_SUCCESS;
}
#if 0
LOCAL uint32_t _c2580_cfg_otp(SENSOR_HW_HANDLE handle,uint32_t param)
{
	uint32_t rtn=SENSOR_SUCCESS;
	struct raw_param_info_tab* tab_ptr = (struct raw_param_info_tab*)s_c2580_raw_param_tab;
	uint32_t module_id=g_c2580_module_id;

	SENSOR_PRINT("_c2580_cfg_otp module_id:0x%x", module_id);

	/*be called in sensor thread, so not call Sensor_SetMode_WaitDone()*/
	usleep(10 * 1000);

	if (PNULL!=tab_ptr[module_id].cfg_otp) {
		tab_ptr[module_id].cfg_otp(0);
	}
	// do streamoff, and not sleep
	_c2580_StreamOff(handle);

	SENSOR_PRINT("_c2580_cfg_otp end");

	return rtn;
}

LOCAL uint32_t _c2580_GetRawInof(void)
{
	uint32_t rtn=SENSOR_SUCCESS;
	struct raw_param_info_tab* tab_ptr = (struct raw_param_info_tab*)s_c2580_raw_param_tab;
	uint32_t i=0x00;

	for(i=0x00; ; i++)
	{
		g_c2580_module_id = i;
		if(RAW_INFO_END_ID==tab_ptr[i].param_id){
			if(NULL==s_c2580_mipi_raw_info_ptr){
				SENSOR_PRINT(" c2580_GetRawInof no param error");
				rtn=SENSOR_FAIL;
			}
			SENSOR_PRINT("c2580_GetRawInof end");
			break;
		}
		else if(PNULL!=tab_ptr[i].identify_otp){
			if(SENSOR_SUCCESS==tab_ptr[i].identify_otp(0))
			{
				s_c2580_mipi_raw_info_ptr = tab_ptr[i].info_ptr;
				SENSOR_PRINT(" c2580_GetRawInof id:0x%x success", g_c2580_module_id);
				break;
			}
		}
	}

	return rtn;
}
#endif
LOCAL uint32_t _c2580_GetMaxFrameLine(SENSOR_HW_HANDLE handle, uint32_t index) {
    uint32_t max_line = 0x00;
    SENSOR_TRIM_T_PTR trim_ptr = s_c2580_Resolution_Trim_Tab;

    max_line = trim_ptr[index].frame_line;

    return max_line;
}

LOCAL uint32_t _c2580_Identify(SENSOR_HW_HANDLE handle, uint32_t param) {
#define c2580_PID_VALUE 0x02
#define c2580_PID_ADDR 0x0000
#define c2580_VER_VALUE 0x01
#define c2580_VER_ADDR 0x0001

    uint8_t pid_value = 0x00;
    uint8_t ver_value = 0x00;
    uint32_t ret_value = SENSOR_SUCCESS;

    SENSOR_PRINT_HIGH("SENSOR_c2580: mipi raw identify\n");

    pid_value = Sensor_ReadReg(c2580_PID_ADDR);
    if (c2580_PID_VALUE == pid_value) {
        ver_value = Sensor_ReadReg(c2580_VER_ADDR);
        SENSOR_PRINT("SENSOR_c2580: Identify: PID = %x, VER = %x", pid_value,
                     ver_value);
        if (c2580_VER_VALUE == ver_value) {
            SENSOR_PRINT_HIGH("SENSOR_c2580: this is c2580 sensor !");
#if 0
			//ret_value=_c2580_GetRawInof();
			if(SENSOR_SUCCESS != ret_value)
			{
				SENSOR_PRINT_ERR("SENSOR_c2580: the module is unknow error !");
			}
			//Sensor_c2580_InitRawTuneInfo();
#endif
        } else {
            SENSOR_PRINT_HIGH("SENSOR_c2580: Identify this is c%x%x sensor !",
                              pid_value, ver_value);
        }
    } else {
        SENSOR_PRINT_ERR("SENSOR_c2580: identify fail,pid_value=%d", pid_value);
    }

    return ret_value;
}

LOCAL uint32_t _c2580_write_exposure(SENSOR_HW_HANDLE handle, uint32_t param) {
    uint32_t ret_value = SENSOR_SUCCESS;
    uint16_t expsure_line = 0x00;
    uint16_t dummy_line = 0x00;
    uint16_t size_index = 0x00;
    uint16_t frame_len = 0x00;
    uint16_t frame_len_cur = 0x00;
    uint16_t max_frame_len = 0x00;

    expsure_line = param & 0xffff;
    dummy_line = (param >> 0x10) & 0x0fff;
    size_index = (param >> 0x1c) & 0x0f;

    SENSOR_PRINT(
        "SENSOR_c2580: write_exposure line:%d, dummy:%d, size_index:%d",
        expsure_line, dummy_line, size_index);

    // group hold on
    //_c2580_grouphold_on();

    max_frame_len = _c2580_GetMaxFrameLine(handle, size_index);

    if (0x00 != max_frame_len) {
        frame_len = ((expsure_line + c2580_ES_OFFSET) > max_frame_len)
                        ? (expsure_line + c2580_ES_OFFSET)
                        : max_frame_len;
        frame_len = (frame_len + 1) >> 1 << 1;

        frame_len_cur = _c2580_get_VTS(handle);

        SENSOR_PRINT("SENSOR_c2580: frame_len_cur:%d, frame_len:%d,",
                     frame_len_cur, frame_len);

        if (frame_len_cur != frame_len) {

            ret_value = _c2580_set_VTS(handle, frame_len);
        }
    }

    ret_value = _c2580_set_shutter(handle, expsure_line);

    return ret_value;
}

LOCAL uint32_t _c2580_write_gain(SENSOR_HW_HANDLE handle, uint32_t param) {
    uint32_t ret_value = SENSOR_SUCCESS;
    uint32_t isp_gain = 0x00;
    uint32_t sensor_gain = 0x00;
    SENSOR_PRINT("SENSOR_c2580: write_gain:0x%x", param);

    isp_gain = ((param & 0xf) + 16) * (((param >> 4) & 0x01) + 1) *
               (((param >> 5) & 0x01) + 1);
    isp_gain = isp_gain * (((param >> 6) & 0x01) + 1) *
               (((param >> 7) & 0x01) + 1) * (((param >> 8) & 0x01) + 1);

    // sensor_gain=(((isp_gain-16)/16)<<4)+isp_gain%16;
    sensor_gain = (((isp_gain - 16) / 16) << 4) + (isp_gain - 16) % 16;

    // SENSOR_PRINT("SENSOR_c2580: write_gain:0x%x, 0x%x", param,sensor_gain);

    ret_value = _c2580_set_gain16(handle, sensor_gain);

    // group hold off
    //_c2580_grouphold_off();
    return ret_value;
}

LOCAL uint32_t _c2580_write_af(SENSOR_HW_HANDLE handle, uint32_t param) {
    uint32_t ret_value = SENSOR_SUCCESS;

    return ret_value;
}

LOCAL uint32_t _c2580_BeforeSnapshot(SENSOR_HW_HANDLE handle, uint32_t param) {
    uint32_t capture_exposure, preview_maxline;
    uint32_t capture_maxline, preview_exposure;
    uint32_t capture_mode = param & 0xffff;
    uint32_t preview_mode = (param >> 0x10) & 0xffff;
    uint32_t prv_linetime = s_c2580_Resolution_Trim_Tab[preview_mode].line_time;
    uint32_t cap_linetime = s_c2580_Resolution_Trim_Tab[capture_mode].line_time;

    SENSOR_PRINT("SENSOR_c2580: BeforeSnapshot mode: 0x%08x", param);

    if (preview_mode == capture_mode) {
        SENSOR_PRINT("SENSOR_c2580: prv mode equal to capmode");
        goto CFG_INFO;
    }

    preview_exposure = _c2580_get_shutter(handle);

    preview_maxline = _c2580_get_VTS(handle);

    Sensor_SetMode(capture_mode);
    Sensor_SetMode_WaitDone();

    /*if (prv_linetime == cap_linetime) {
            SENSOR_PRINT("SENSOR_c2580: prvline equal to capline");
            goto CFG_INFO;
    }
    */
    capture_maxline = _c2580_get_VTS(handle);

    capture_exposure = preview_exposure * prv_linetime * 2 / cap_linetime;

    if (0 == capture_exposure) {
        capture_exposure = 1;
    }

    SENSOR_PRINT("SENSOR_c2580: preview_exposure:%d, capture_exposure:%d, "
                 "capture_maxline: %d",
                 preview_exposure, capture_exposure, capture_maxline);
    //_c2580_grouphold_on();
    if (capture_exposure > (capture_maxline - c2580_ES_OFFSET)) {
        capture_maxline = capture_exposure + c2580_ES_OFFSET;
        capture_maxline = (capture_maxline + 1) >> 1 << 1;
        _c2580_set_VTS(handle, capture_maxline);
    }

    _c2580_set_shutter(handle, capture_exposure);
//_c2580_grouphold_off();
CFG_INFO:
    s_capture_shutter = _c2580_get_shutter(handle);
    s_capture_VTS = _c2580_get_VTS(handle);
    s_c2580_gain = _c2580_get_gain16(handle);

    Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_EXPOSURETIME, s_capture_shutter);

    return SENSOR_SUCCESS;
}

LOCAL uint32_t _c2580_after_snapshot(SENSOR_HW_HANDLE handle, uint32_t param) {
    SENSOR_PRINT("SENSOR_c2580: after_snapshot mode:%d", param);
    Sensor_SetMode(param);
    return SENSOR_SUCCESS;
}

LOCAL uint32_t _c2580_flash(SENSOR_HW_HANDLE handle, uint32_t param) {
    SENSOR_PRINT("SENSOR_c2580: param=%d", param);

    /* enable flash, disable in _c2580_BeforeSnapshot */
    g_flash_mode_en = param;
    Sensor_SetFlash(param);
    SENSOR_PRINT_HIGH("end");
    return SENSOR_SUCCESS;
}

static uint32_t _c2580_SetEV(SENSOR_HW_HANDLE handle, uint32_t param) {
    uint32_t rtn = SENSOR_SUCCESS;
    SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR)param;

    uint32_t ev = ext_ptr->param;

    SENSOR_PRINT("SENSOR_c2580: _c2580_SetEV param: 0x%x", ext_ptr->param);

    switch (ev) {
    case SENSOR_HDR_EV_LEVE_0:
        _calculate_hdr_exposure(handle, s_c2580_gain, s_capture_VTS,
                                s_capture_shutter / 2);
        break;
    case SENSOR_HDR_EV_LEVE_1:
        _calculate_hdr_exposure(handle, s_c2580_gain, s_capture_VTS,
                                s_capture_shutter);
        break;
    case SENSOR_HDR_EV_LEVE_2:
        _calculate_hdr_exposure(handle, s_c2580_gain, s_capture_VTS,
                                s_capture_shutter * 2);
        break;
    default:
        break;
    }
    return rtn;
}
/*
static unsigned long _c2580_set_brightness(SENSOR_HW_HANDLE handle,unsigned long
level)
{
        uint16_t i;
        SENSOR_REG_T* sensor_reg_ptr =
(SENSOR_REG_T*)c2580_MIPI_brightness_tab[level];

        if (level>6)
                return 0;

        for (i = 0; (0xFF != sensor_reg_ptr[i].reg_addr) && (0xFF !=
sensor_reg_ptr[i].reg_value); i++) {
                c2580_MIPI_WriteReg(handle,sensor_reg_ptr[i].reg_addr,
sensor_reg_ptr[i].reg_value);
        }

        return 0;
}
*/
LOCAL uint32_t _c2580_saveLoad_exposure(SENSOR_HW_HANDLE handle,
                                        uint32_t param) {
    uint32_t rtn = SENSOR_SUCCESS;
    SENSOR_EXT_FUN_PARAM_T_PTR sl_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR)param;

    uint32_t sl_param = sl_ptr->param;
    if (sl_param) {
        //		usleep(180*1000);     /*wait for effect after init
        // stable(AWB)*/
        /*load exposure params to sensor*/
        SENSOR_PRINT_HIGH(
            "_c2580_saveLoad_exposure load shutter 0x%x gain 0x%x",
            s_c2580_shutter_bak, s_c2580_gain_bak);
        //_c2580_grouphold_on();
        _c2580_set_gain16(handle, s_c2580_gain_bak);
        _c2580_set_shutter(handle, s_c2580_shutter_bak);
        //_c2580_grouphold_off();
    } else {
        /*ave exposure params from sensor*/
        s_c2580_shutter_bak = _c2580_get_shutter(handle);
        s_c2580_gain_bak = _c2580_get_gain16(handle);
        SENSOR_PRINT_HIGH(
            "_c2580_saveLoad_exposure save shutter 0x%x gain 0x%x",
            s_c2580_shutter_bak, s_c2580_gain_bak);
    }
    return rtn;
}

LOCAL uint32_t _c2580_ExtFunc(SENSOR_HW_HANDLE handle, uint32_t ctl_param) {
    uint32_t rtn = SENSOR_SUCCESS;
    SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR)ctl_param;
    SENSOR_PRINT_HIGH("0x%x", ext_ptr->cmd);

    switch (ext_ptr->cmd) {
    case SENSOR_EXT_FUNC_INIT:
        break;
    case SENSOR_EXT_FOCUS_START:
        break;
    case SENSOR_EXT_EXPOSURE_START:
        break;
    case SENSOR_EXT_EV:
        rtn = _c2580_SetEV(handle, ctl_param);
        break;

    case SENSOR_EXT_EXPOSURE_SL:
        rtn = _c2580_saveLoad_exposure(handle, ctl_param);
        break;

    default:
        break;
    }
    return rtn;
}

static LOCAL SENSOR_IOCTL_FUNC_TAB_T s_c2580_ioctl_func_tab = {
    .power = _c2580_PowerOn,
    .identify = _c2580_Identify,
    .get_trim = _c2580_GetResolutionTrimTab,
    .stream_on = _c2580_StreamOn,
    .stream_off = _c2580_StreamOff,
    .read_aec_info = c2580_access_val,
    .cfg_otp = c2580_access_val,
};
