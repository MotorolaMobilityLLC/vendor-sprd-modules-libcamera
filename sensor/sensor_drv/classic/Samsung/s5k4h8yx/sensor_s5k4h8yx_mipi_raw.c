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
#include "cutils/properties.h"
#include <utils/Log.h>
#include <dlfcn.h>
#include "sensor.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"
//#if defined(CONFIG_CAMERA_ISP_VERSION_V3) ||
// defined(CONFIG_CAMERA_ISP_VERSION_V4)
//#include "parameters/sensor_s5k4h8yx_raw_param_v3.c"
//#else
//#endif
#include "isp_param_file_update.h"

#ifndef RAW_INFO_END_ID
#define RAW_INFO_END_ID 0x71717567
#endif

#define S5K4H8YX_I2C_ADDR_W 0x10
#define S5K4H8YX_I2C_ADDR_R 0x10
#define DW9807_VCM_SLAVE_ADDR (0x18 >> 1)
#define DW9807_EEPROM_SLAVE_ADDR (0xB0 >> 1)

//#define S5K4H8YX_2_LANES
#define S5K4H8YX_4_LANES

/**
 * Name of the sensor_raw_param as a string
 */
#define S5K4H8YX_RAW_PARAM_AS_STR "S5K4H8YXRP"

static int s_s5k4h8yx_gain = 0;
static int s_capture_shutter = 0;
static int s_capture_VTS = 0;
static int s_exposure_time = 0;
static uint32_t g_module_id = 0;
static uint32_t g_flash_mode_en = 0;
static struct sensor_raw_info *s_s5k4h8yx_mipi_raw_info_ptr = NULL;
static uint32_t s_set_gain;
static uint32_t s_set_exposure;
static uint32_t s_current_default_frame_length;
static uint32_t s_current_frame_length = 0;
static uint32_t s_current_default_line_time = 0;

struct sensor_ev_info_t s_sensor_ev_info;

static uint32_t s_s5k4h8yx_sensor_close_flag = 0;

SENSOR_HW_HANDLE _s5k4h8yx_Create(void *privatedata);
void _s5k4h8yx_Destroy(SENSOR_HW_HANDLE handle);
static unsigned long _s5k4h8yx_GetResolutionTrimTab(SENSOR_HW_HANDLE handle,
                                                    unsigned long param);
static unsigned long _s5k4h8yx_PowerOn(SENSOR_HW_HANDLE handle,
                                       unsigned long power_on);
static unsigned long _s5k4h8yx_Identify(SENSOR_HW_HANDLE handle,
                                        unsigned long param);
static uint32_t _s5k4h8yx_GetRawInof(SENSOR_HW_HANDLE handle);
static unsigned long _s5k4h8yx_BeforeSnapshot(SENSOR_HW_HANDLE handle,
                                              unsigned long param);
static unsigned long _s5k4h8yx_after_snapshot(SENSOR_HW_HANDLE handle,
                                              unsigned long param);
static unsigned long _s5k4h8yx_StreamOn(SENSOR_HW_HANDLE handle,
                                        unsigned long param);
static unsigned long _s5k4h8yx_StreamOff(SENSOR_HW_HANDLE handle,
                                         unsigned long param);
static unsigned long _s5k4h8yx_write_exp_dummy(SENSOR_HW_HANDLE handle,
                                               uint16_t expsure_line,
                                               uint16_t dummy_line,
                                               uint16_t size_index);
static unsigned long _s5k4h8yx_write_exposure(SENSOR_HW_HANDLE handle,
                                              unsigned long param);
static unsigned long _s5k4h8yx_ex_write_exposure(SENSOR_HW_HANDLE handle,
                                                 unsigned long param);
static unsigned long _s5k4h8yx_write_gain(SENSOR_HW_HANDLE handle,
                                          unsigned long param);
static unsigned long _s5k4h8yx_write_af(SENSOR_HW_HANDLE handle,
                                        unsigned long param);
static unsigned long _s5k4h8yx_flash(SENSOR_HW_HANDLE handle,
                                     unsigned long param);
static unsigned long _s5k4h8yx_GetExifInfo(SENSOR_HW_HANDLE handle,
                                           unsigned long param);
static unsigned long _s5k4h8yx_ExtFunc(SENSOR_HW_HANDLE handle,
                                       unsigned long ctl_param);
static uint16_t _s5k4h8yx_get_VTS(SENSOR_HW_HANDLE handle);
static uint32_t _s5k4h8yx_set_VTS(SENSOR_HW_HANDLE handle, uint16_t VTS);
static uint32_t _s5k4h8yx_ReadGain(SENSOR_HW_HANDLE handle, uint32_t *param);
static unsigned long _s5k4h8yx_set_video_mode(SENSOR_HW_HANDLE handle,
                                              unsigned long param);
static uint16_t _s5k4h8yx_get_shutter(SENSOR_HW_HANDLE handle);
static uint32_t _s5k4h8yx_set_shutter(SENSOR_HW_HANDLE handle,
                                      uint16_t shutter);
static unsigned long _s5k4h8yx_access_val(SENSOR_HW_HANDLE handle,
                                          unsigned long param);
static uint32_t _s5k4h8yx_read_otp(SENSOR_HW_HANDLE handle,
                                   unsigned long param);
static uint32_t _s5k4h8yx_write_otp(SENSOR_HW_HANDLE handle,
                                    unsigned long param);

//#define FEATURE_OTP    /*OTP function switch*/

#ifdef FEATURE_OTP
#define MODULE_ID_s5k4h8yx_ofilm 0x0007
#define MODULE_ID_s5k4h8yx_truly 0x0002
#define LSC_PARAM_QTY 240

struct otp_info_t {
    uint16_t flag;
    uint16_t module_id;
    uint16_t lens_id;
    uint16_t vcm_id;
    uint16_t vcm_driver_id;
    uint16_t year;
    uint16_t month;
    uint16_t day;
    uint16_t rg_ratio_current;
    uint16_t bg_ratio_current;
    uint16_t rg_ratio_typical;
    uint16_t bg_ratio_typical;
    uint16_t r_current;
    uint16_t g_current;
    uint16_t b_current;
    uint16_t r_typical;
    uint16_t g_typical;
    uint16_t b_typical;
    uint16_t vcm_dac_start;
    uint16_t vcm_dac_inifity;
    uint16_t vcm_dac_macro;
    uint16_t lsc_param[LSC_PARAM_QTY];
};

#include "sensor_s5k4h8yx_ofilm_otp.c"

static const struct raw_param_info_tab s_s5k4h8yx_raw_param_tab[] = {
    {MODULE_ID_s5k4h8yx_truly, NULL, s5k4h8yx_ofilm_identify_otp,
     s5k4h8yx_ofilm_update_otp},
    {RAW_INFO_END_ID, PNULL, PNULL, PNULL}};
#endif
////initial setting V24 F1X9
static const SENSOR_REG_T s5k4h8yx_common_init_new[] = {
    {0x6028, 0x2000}, {0x602A, 0x1FD0}, {0x6F12, 0x0448}, {0x6F12, 0x0349},
    {0x6F12, 0x0160}, {0x6F12, 0xC26A}, {0x6F12, 0x511A}, {0x6F12, 0x8180},
    {0x6F12, 0x00F0}, {0x6F12, 0x60B8}, {0x6F12, 0x2000}, {0x6F12, 0x20E8},
    {0x6F12, 0x2000}, {0x6F12, 0x13A0}, {0x6F12, 0x0000}, {0x6F12, 0x0000},
    {0x6F12, 0x0000}, {0x6F12, 0x0000}, {0x6F12, 0x38B5}, {0x6F12, 0x0021},
    {0x6F12, 0x0446}, {0x6F12, 0x8DF8}, {0x6F12, 0x0010}, {0x6F12, 0x00F5},
    {0x6F12, 0xB470}, {0x6F12, 0x0122}, {0x6F12, 0x6946}, {0x6F12, 0x00F0},
    {0x6F12, 0x59F8}, {0x6F12, 0x9DF8}, {0x6F12, 0x0000}, {0x6F12, 0xFF28},
    {0x6F12, 0x05D0}, {0x6F12, 0x0020}, {0x6F12, 0x08B1}, {0x6F12, 0x04F2},
    {0x6F12, 0x6914}, {0x6F12, 0x2046}, {0x6F12, 0x38BD}, {0x6F12, 0x0120},
    {0x6F12, 0xF8E7}, {0x6F12, 0x10B5}, {0x6F12, 0x92B0}, {0x6F12, 0x0C46},
    {0x6F12, 0x4822}, {0x6F12, 0x6946}, {0x6F12, 0x00F0}, {0x6F12, 0x46F8},
    {0x6F12, 0x0020}, {0x6F12, 0x6946}, {0x6F12, 0x04EB}, {0x6F12, 0x4003},
    {0x6F12, 0x0A5C}, {0x6F12, 0x02F0}, {0x6F12, 0x0F02}, {0x6F12, 0x04F8},
    {0x6F12, 0x1020}, {0x6F12, 0x0A5C}, {0x6F12, 0x401C}, {0x6F12, 0x1209},
    {0x6F12, 0x5A70}, {0x6F12, 0x4828}, {0x6F12, 0xF2D3}, {0x6F12, 0x12B0},
    {0x6F12, 0x10BD}, {0x6F12, 0x2DE9}, {0x6F12, 0xF041}, {0x6F12, 0x164E},
    {0x6F12, 0x0F46}, {0x6F12, 0x06F1}, {0x6F12, 0x1105}, {0x6F12, 0xA236},
    {0x6F12, 0xB0B1}, {0x6F12, 0x1449}, {0x6F12, 0x1248}, {0x6F12, 0x0968},
    {0x6F12, 0x0078}, {0x6F12, 0xB1F8}, {0x6F12, 0x6A10}, {0x6F12, 0xC007},
    {0x6F12, 0x0ED0}, {0x6F12, 0x0846}, {0x6F12, 0xFFF7}, {0x6F12, 0xBEFF},
    {0x6F12, 0x84B2}, {0x6F12, 0x2946}, {0x6F12, 0x2046}, {0x6F12, 0xFFF7},
    {0x6F12, 0xD0FF}, {0x6F12, 0x4FF4}, {0x6F12, 0x9072}, {0x6F12, 0x3146},
    {0x6F12, 0x04F1}, {0x6F12, 0x4800}, {0x6F12, 0x00F0}, {0x6F12, 0x16F8},
    {0x6F12, 0x002F}, {0x6F12, 0x05D0}, {0x6F12, 0x3146}, {0x6F12, 0x2846},
    {0x6F12, 0xBDE8}, {0x6F12, 0xF041}, {0x6F12, 0x00F0}, {0x6F12, 0x13B8},
    {0x6F12, 0xBDE8}, {0x6F12, 0xF081}, {0x6F12, 0x0022}, {0x6F12, 0xAFF2},
    {0x6F12, 0x5501}, {0x6F12, 0x0348}, {0x6F12, 0x00F0}, {0x6F12, 0x10B8},
    {0x6F12, 0x2000}, {0x6F12, 0x0C40}, {0x6F12, 0x2000}, {0x6F12, 0x0560},
    {0x6F12, 0x0000}, {0x6F12, 0x152D}, {0x6F12, 0x48F6}, {0x6F12, 0x296C},
    {0x6F12, 0xC0F2}, {0x6F12, 0x000C}, {0x6F12, 0x6047}, {0x6F12, 0x41F2},
    {0x6F12, 0x950C}, {0x6F12, 0xC0F2}, {0x6F12, 0x000C}, {0x6F12, 0x6047},
    {0x6F12, 0x49F2}, {0x6F12, 0x514C}, {0x6F12, 0xC0F2}, {0x6F12, 0x000C},
    {0x6F12, 0x6047}, {0x6F12, 0x0000}, {0x6F12, 0x0000}, {0x6F12, 0x0000},
    {0x6F12, 0x0000}, {0x6F12, 0x0000}, {0x6F12, 0x4088}, {0x6F12, 0x0166},
    {0x6F12, 0x0000}, {0x6F12, 0x0002}, {0x5360, 0x0004}, {0x3078, 0x0059},
    {0x307C, 0x0025}, {0x36D0, 0x00DD}, {0x36D2, 0x0100}, {0x306A, 0x00EF},

};

static const SENSOR_REG_T s5k4h8yx_3264x2448_2lane_setting_new[] = {
    /*
    3264x2448_15fps_vt140M_2lane_mipi642M
    width   3264
    height  2448
    frame rate  14.97
    mipi_lane_num   2
    mipi_per_lane_bps   642
    line time(0.1us unit)   267.4285714
    frame length    2498
    Extclk  24M
    max gain    0x0200
    base gain   0x0020
    raw bits    raw10
    bayer patter    Gr first
    OB level    64
    offset  8
    min shutter 4
    address data    */
    {0x6028, 0x4000}, {0x602A, 0x6214}, {0x6F12, 0x7971}, {0x602A, 0x6218},
    {0x6F12, 0x7150}, {0x6028, 0x2000}, {0x602A, 0x0EC6}, {0x6F12, 0x0000},
    {0xF490, 0x0030}, {0xF47A, 0x0012}, {0xF428, 0x0200}, {0xF48E, 0x0010},
    {0xF45C, 0x0004}, {0x0B04, 0x0101}, {0x0B00, 0x0080}, {0x6028, 0x2000},
    {0x602A, 0x0C40}, {0x6F12, 0x0140}, {0x31FA, 0x0000}, {0x0200, 0x0800},
    {0x0202, 0x0465}, {0x31AA, 0x0004}, {0x1006, 0x0006}, {0x31FA, 0x0000},
    {0x0204, 0x0020}, {0x0344, 0x0008}, {0x0348, 0x0CC7}, {0x0346, 0x0008},
    {0x034A, 0x0997}, {0x034C, 0x0CC0}, {0x034E, 0x0990}, {0x0342, 0x0EA0},
    {0x0340, 0x09C2}, {0x0900, 0x0111}, {0x0380, 0x0001}, {0x0382, 0x0001},
    {0x0384, 0x0001}, {0x0386, 0x0001}, {0x0400, 0x0002}, {0x0404, 0x0010},
    {0x0114, 0x0130}, {0x0136, 0x1800}, {0x0300, 0x0005}, {0x0302, 0x0002},
    {0x0304, 0x0004}, {0x0306, 0x0075}, {0x030C, 0x0004}, {0x030E, 0x006B},
    {0x3008, 0x0000},

};
static const SENSOR_REG_T s5k4h8yx_3264x2448_4lane_setting_new[] = {
    /*3264x2448_30fps_vt280M_4lane_mipi700M
    width   3264
    height  2448
    frame rate  29.94
    mipi_lane_num   4
    mipi_per_lane_bps   700
    line time(0.1us unit)   133.71
    frame length    2498
    Extclk  24M
     max gain   0x0200
    base gain   0x0020
    raw bits    raw10
    bayer patter    Gr first
    OB level    64
    offset  8
    min shutter 4
    address data  */
    {0x6028, 0x4000}, {0x602A, 0x6214}, {0x6F12, 0x7971}, {0x602A, 0x6218},
    {0x6F12, 0x7150}, {0x6028, 0x2000}, {0x602A, 0x0EC6}, {0x6F12, 0x0000},
    {0xF490, 0x0030}, {0xF47A, 0x0012}, {0xF428, 0x0200}, {0xF48E, 0x0010},
    {0xF45C, 0x0004}, {0x0B04, 0x0101}, {0x0B00, 0x0080}, {0x6028, 0x2000},
    {0x602A, 0x0C40}, {0x6F12, 0x0140}, {0x0200, 0x0618}, {0x0202, 0x0904},
    {0x31AA, 0x0004}, {0x1006, 0x0006}, {0x31FA, 0x0000}, {0x0204, 0x0020},
    {0x0344, 0x0008}, {0x0348, 0x0CC7}, {0x0346, 0x0008}, {0x034A, 0x0997},
    {0x034C, 0x0CC0}, {0x034E, 0x0990}, {0x0342, 0x0EA0}, {0x0340, 0x09C2},
    {0x0900, 0x0111}, {0x0380, 0x0001}, {0x0382, 0x0001}, {0x0384, 0x0001},
    {0x0386, 0x0001}, {0x0400, 0x0002}, {0x0404, 0x0010}, {0x0114, 0x0330},
    {0x0136, 0x1800}, {0x0300, 0x0005}, {0x0302, 0x0001}, {0x0304, 0x0006},
    {0x0306, 0x00AF}, {0x030C, 0x0006}, {0x030E, 0x00AF}, {0x3008, 0x0000},

};
static const SENSOR_REG_T s5k4h8yx_common_init_new1[] = {
    /*for V22	FGX9
address	data*/
    {0x6028, 0x4000}, {0x602A, 0x6214}, {0x6F12, 0x7971},
    {0x602A, 0x6218}, {0x6F12, 0x7150}, {0x0806, 0x0001},
};
static const SENSOR_REG_T s5k4h8yx_3264x2448_2lane_setting_new1[] = {
    /*full size
3264x2448_15fps_vt140M_2lane_mipi642M
width	3264
height	2448
frame rate	29.94
mipi_lane_num	2
mipi_per_lane_bps	642
line time(0.1us unit)	267.4285714
frame length	2498
Extclk	24M
 max gain	0x0200
base gain	0x0020
raw bits	raw10
bayer patter 	Gr first
OB level	64
offset	8
min shutter	4
address	data*/
    {0x6028, 0x2000}, {0x602A, 0x0EC6}, {0x6F12, 0x0000}, {0xFCFC, 0x4000},
    {0xF490, 0x0030}, {0xF47A, 0x0012}, {0xF428, 0x0200}, {0xF48E, 0x0010},
    {0xF45C, 0x0004}, {0x0B04, 0x0101}, {0x0B00, 0x0080}, {0x6028, 0x2000},
    {0x602A, 0x0C40}, {0x6F12, 0x0140}, {0xFCFC, 0x4000}, {0x0200, 0x0618},
    {0x0202, 0x0465}, {0x31AA, 0x0004}, {0x1006, 0x0006}, {0x31FA, 0x0000},
    {0x0204, 0x0020}, {0x0344, 0x0008}, {0x0348, 0x0CC7}, {0x0346, 0x0008},
    {0x034A, 0x0997}, {0x034C, 0x0CC0}, {0x034E, 0x0990}, {0x0342, 0x0EA0},
    {0x0340, 0x09C2}, {0x0900, 0x0111}, {0x0380, 0x0001}, {0x0382, 0x0001},
    {0x0384, 0x0001}, {0x0386, 0x0001}, {0x0400, 0x0002}, {0x0404, 0x0010},
    {0x0114, 0x0130}, {0x0136, 0x1800}, {0x0300, 0x0005}, {0x0302, 0x0002},
    {0x0304, 0x0004}, {0x0306, 0x0075}, {0x030C, 0x0004}, {0x030E, 0x006B},
    {0x3008, 0x0000},
};
static const SENSOR_REG_T s5k4h8yx_3264x2448_4lane_setting_new1[] = {
    /*full size
3264x2448_30fps_vt280M_4lane_mipi700M
width	3264
height	2448
frame rate	29.94
mipi_lane_num	4
mipi_per_lane_bps	700
line time(0.1us unit)	133.7142857
frame length	2498
Extclk	24M
 max gain	0x0200
base gain	0x0020
raw bits	raw10
bayer patter 	Gr first
OB level	64
offset	8
min shutter	4
address	data*/
    {0x6028, 0x2000}, {0x602A, 0x0EC6}, {0x6F12, 0x0000},
    {0xFCFC, 0x4000}, {0xF490, 0x0030}, {0xF47A, 0x0012},
    {0xF428, 0x0200}, {0xF48E, 0x0010}, {0xF45C, 0x0004},
    {0x0B04, 0x0101}, {0x0B00, 0x0080}, {0x6028, 0x2000},
    {0x602A, 0x0C40}, {0x6F12, 0x0140}, {0xFCFC, 0x4000},
    {0x0200, 0x0618}, {0x0202, 0x0904}, {0x31AA, 0x0004},
    {0x1006, 0x0006}, {0x31FA, 0x0000}, {0x0204, 0x0020},
    {0x0344, 0x0008}, {0x0348, 0x0CC7}, {0x0346, 0x0008},
    {0x034A, 0x0997}, {0x034C, 0x0CC0}, {0x034E, 0x0990},
    {0x0342, 0x0EA0}, {0x0340, 0x09C2}, {0x0900, 0x0111},
    {0x0380, 0x0001}, {0x0382, 0x0001}, {0x0384, 0x0001},
    {0x0386, 0x0001}, {0x0400, 0x0002}, {0x0404, 0x0010},
    {0x0114, 0x0330}, {0x0136, 0x1800}, {0x0300, 0x0005},
    {0x0302, 0x0001}, {0x0304, 0x0006}, {0x0306, 0x00AF},
    {0x030C, 0x0006}, {0x030E, 0x00BE}, // AF},
    {0x3008, 0x0000},
};

static const SENSOR_REG_T s5k4h8yx_1632x1224_4lane_setting_new1[] = {
    /*binning size
1632x1224_60fps_vt280M_4lane_mipi700M
width	3264
height	2448
frame rate	60.02
mipi_lane_num	4
mipi_per_lane_bps	700
line time(0.1us unit)	133.7142857
frame length	1246
Extclk	24M
 max gain	0x0200
base gain	0x0020
raw bits	raw10
bayer patter 	Gr first
OB level	64
offset	8
min shutter	4
address	data*/
    {0x6028, 0x2000}, {0x602A, 0x0EC6}, {0x6F12, 0x0000}, {0xFCFC, 0x4000},
    {0xF490, 0x0030}, {0xF47A, 0x0012}, {0xF428, 0x0200}, {0xF48E, 0x0010},
    {0xF45C, 0x0004}, {0x0B04, 0x0101}, {0x0B00, 0x0080}, {0x6028, 0x2000},
    {0x602A, 0x0C40}, {0x6F12, 0x0140}, {0xFCFC, 0x4000}, {0x0200, 0x0618},
    {0x0202, 0x0904}, {0x31AA, 0x0004}, {0x1006, 0x0006}, {0x31FA, 0x0000},
    {0x0204, 0x0020}, {0x0344, 0x0008}, {0x0348, 0x0CC7}, {0x0346, 0x0008},
    {0x034A, 0x0997}, {0x034C, 0x0660}, {0x034E, 0x04C8}, {0x0342, 0x0EA0},
    {0x0340, 0x04DE}, {0x0900, 0x0212}, {0x0380, 0x0001}, {0x0382, 0x0001},
    {0x0384, 0x0001}, {0x0386, 0x0003}, {0x0400, 0x0002}, {0x0404, 0x0020},
    {0x0114, 0x0330}, {0x0136, 0x1800}, {0x0300, 0x0005}, {0x0302, 0x0001},
    {0x0304, 0x0006}, {0x0306, 0x00AF}, {0x030C, 0x0006}, {0x030E, 0x00AF},
    {0x3008, 0x0000},

};

////initial setting
static const SENSOR_REG_T s5k4h8yx_common_init[] = {
    {0x6028, 0x4000},
    {0x6010, 0x0001}, // Reset
    {0xffff, 0x000a}, // must add delay >3ms
    {0x602A, 0x6214},
    {0x6F12, 0x7971},
    {0x602A, 0x6218},
    {0x6F12, 0x7150},
    {0x6028, 0x2000},
    {0x602A, 0x0EC6},
    {0x6F12, 0x0000},
    {0xFCFC, 0x4000},
    {0xF490, 0x0030},
    {0xF47A, 0x0012},
    {0xF428, 0x0200},
    {0xF48E, 0x0010},
    {0xF45C, 0x0004},
    {0x0B04, 0x0101},
    {0x0B00, 0x0080}, // shading off;  if
                      // 0x0B00,0x0180
                      // //shading on
    {0x6028, 0x2000},
    {0x602A, 0x0C40},
    {0x6F12, 0x0140},
};

///////////res1//////////
/*full size setting, 3264x2448,30fps,vt280M,mipi700M,4lane
frame length    0x09C2
line time       133.7
*/
static const SENSOR_REG_T s5k4h8yx_3264x2448_4lane_setting[] = {
    {0x6028, 0x4000}, {0x0200, 0x0618}, {0x0202, 0x0904}, {0x31AA, 0x0004},
    {0x1006, 0x0006}, {0x31FA, 0x0000}, {0x0344, 0x0008}, {0x0348, 0x0CC7},
    {0x0346, 0x0008}, {0x034A, 0x0997}, {0x034C, 0x0CC0}, {0x034E, 0x0990},
    {0x0342, 0x0EA0}, {0x0340, 0x09C2}, {0x0900, 0x0111}, {0x0380, 0x0001},
    {0x0382, 0x0001}, {0x0384, 0x0001}, {0x0386, 0x0001}, {0x0400, 0x0002},
    {0x0404, 0x0010}, {0x0114, 0x0330}, {0x0136, 0x1800}, {0x0300, 0x0005},
    {0x0302, 0x0001}, {0x0304, 0x0006}, {0x0306, 0x00AF}, {0x030C, 0x0006},
    {0x030E, 0x00AF}, {0x3008, 0x0000},
};

///////////res2///////////////////
/*full size setting, 3264x2448,15fps,vt140M,mipi642M,2lane
frame length    0x09C2
line time       267.4
*/
static const SENSOR_REG_T s5k4h8yx_3264x2448_2lane_setting[] = {
    {0x6028, 0x4000}, {0x0200, 0x0618}, {0x0202, 0x0465}, {0x31AA, 0x0004},
    {0x1006, 0x0006}, {0x31FA, 0x0000}, {0x0344, 0x0008}, {0x0348, 0x0CC7},
    {0x0346, 0x0008}, {0x034A, 0x0997}, {0x034C, 0x0CC0}, {0x034E, 0x0990},
    {0x0342, 0x0EA0}, {0x0340, 0x09C2}, {0x0900, 0x0111}, {0x0380, 0x0001},
    {0x0382, 0x0001}, {0x0384, 0x0001}, {0x0386, 0x0001}, {0x0400, 0x0002},
    {0x0404, 0x0010}, {0x0114, 0x0130}, {0x0136, 0x1800}, {0x0300, 0x0005},
    {0x0302, 0x0002}, {0x0304, 0x0004}, {0x0306, 0x0075}, {0x030C, 0x0004},
    {0x030E, 0x006B}, {0x3008, 0x0000},
};

///////////res3//////////
/*small size setting, 1632x1224,30fps,vt280M,mipi700M,4lane
frame length    0x04DE
line time       133.7
*/
static const SENSOR_REG_T s5k4h8yx_1632x1224_4lane_setting[] = {
    {0xFCFC, 0x4000}, {0x0200, 0x0618}, {0x0202, 0x0904}, {0x31AA, 0x0004},
    {0x1006, 0x0006}, {0x31FA, 0x0000}, {0x0204, 0x0020}, {0x020E, 0x0100},
    {0x0344, 0x0008}, {0x0348, 0x0CC7}, {0x0346, 0x0008}, {0x034A, 0x0997},
    {0x034C, 0x0660}, {0x034E, 0x04C8}, {0x0342, 0x0EA0}, {0x0340, 0x04DE},
    {0x0900, 0x0212}, {0x0380, 0x0001}, {0x0382, 0x0001}, {0x0384, 0x0001},
    {0x0386, 0x0003}, {0x0400, 0x0002}, {0x0404, 0x0020}, {0x0114, 0x0330},
    {0x0136, 0x1800}, {0x0300, 0x0005}, {0x0302, 0x0001}, {0x0304, 0x0006},
    {0x0306, 0x00AF}, {0x030C, 0x0006}, {0x030E, 0x00AF}, {0x3008, 0x0000},
};

static SENSOR_REG_TAB_INFO_T s_s5k4h8yx_resolution_Tab_RAW[] = {
#ifdef S5K4H8YX_4_LANES
    {ADDR_AND_LEN_OF_ARRAY(s5k4h8yx_common_init_new1), 0, 0, 24,
     SENSOR_IMAGE_FORMAT_RAW},
    {ADDR_AND_LEN_OF_ARRAY(s5k4h8yx_1632x1224_4lane_setting), 1632, 1224, 24,
     SENSOR_IMAGE_FORMAT_RAW},
    {ADDR_AND_LEN_OF_ARRAY(s5k4h8yx_3264x2448_4lane_setting_new1), 3264, 2448,
     24, SENSOR_IMAGE_FORMAT_RAW},
#else
//{ADDR_AND_LEN_OF_ARRAY(s5k4h8yx_common_init), 0, 0, 24,
// SENSOR_IMAGE_FORMAT_RAW},
//{ADDR_AND_LEN_OF_ARRAY(s5k4h8yx_3264x2448_4lane_setting), 3264, 2448, 24,
// SENSOR_IMAGE_FORMAT_RAW},
//{ADDR_AND_LEN_OF_ARRAY(s5k4h8yx_1632x1224_4lane_setting), 1632, 1224, 24,
// SENSOR_IMAGE_FORMAT_RAW},
#endif
    {PNULL, 0, 0, 0, 0, 0},
    {PNULL, 0, 0, 0, 0, 0},
    {PNULL, 0, 0, 0, 0, 0},
    {PNULL, 0, 0, 0, 0, 0},
    {PNULL, 0, 0, 0, 0, 0},
    {PNULL, 0, 0, 0, 0, 0},
    {PNULL, 0, 0, 0, 0, 0},
};

static SENSOR_TRIM_T s_s5k4h8yx_Resolution_Trim_Tab[] = {
    {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
#ifdef S5K4H8YX_2_LANES
    {0, 0, 3264, 2448, 26742, 700, 2498, {0, 0, 3264, 2448}},
//{0, 0, 1632, 1224, 26742, 700, 1246, {0, 0, 1632, 1224}},
#else
    {0, 0, 1632, 1224, 13371, 700, 2498, {0, 0, 1632, 1224}},
    {0, 0, 3264, 2448, 13371, 700, 2498, {0, 0, 3264, 2448}},
#endif
    {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
    {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
    {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
    {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
    {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
    {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}}};

static const SENSOR_REG_T
    s_s5k4h8yx_1632x1224_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
        /*video mode 0: ?fps*/
        {{0xffff, 0xff}},
        /* video mode 1:?fps*/
        {{0xffff, 0xff}},
        /* video mode 2:?fps*/
        {{0xffff, 0xff}},
        /* video mode 3:?fps*/
        {{0xffff, 0xff}}};

static const SENSOR_REG_T
    s_s5k4h8yx_1920x1080_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
        /*video mode 0: ?fps*/
        {{0xffff, 0xff}},
        /* video mode 1:?fps*/
        {{0xffff, 0xff}},
        /* video mode 2:?fps*/
        {{0xffff, 0xff}},
        /* video mode 3:?fps*/
        {{0xffff, 0xff}}};

static const SENSOR_REG_T
    s_s5k4h8yx_3264x2448_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
        /*video mode 0: ?fps*/
        {{0xffff, 0xff}},
        /* video mode 1:?fps*/
        {{0xffff, 0xff}},
        /* video mode 2:?fps*/
        {{0xffff, 0xff}},
        /* video mode 3:?fps*/
        {{0xffff, 0xff}}};

static SENSOR_VIDEO_INFO_T s_s5k4h8yx_video_info[] = {
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
    //{{{30, 30, 219, 100}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0,
    // 0}},(SENSOR_REG_T**)s_s5k4h8yx_1632x1224_video_tab},
    {{{15, 15, 131, 64}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
     (SENSOR_REG_T **)s_s5k4h8yx_3264x2448_video_tab},
    //{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL}};

static unsigned long _s5k4h8yx_set_video_mode(SENSOR_HW_HANDLE handle,
                                              unsigned long param) {
    SENSOR_REG_T_PTR sensor_reg_ptr;
    uint16_t i = 0;
    uint32_t mode = 0;

    if (param >= SENSOR_VIDEO_MODE_MAX)
        return 0;

    if (SENSOR_SUCCESS != Sensor_GetMode(&mode)) {
        SENSOR_LOGI("fail.");
        return SENSOR_FAIL;
    }

    if (PNULL == s_s5k4h8yx_video_info[mode].setting_ptr) {
        SENSOR_LOGI("fail.");
        return SENSOR_FAIL;
    }

    sensor_reg_ptr =
        (SENSOR_REG_T_PTR)&s_s5k4h8yx_video_info[mode].setting_ptr[param];
    if (PNULL == sensor_reg_ptr) {
        SENSOR_LOGI("fail.");
        return SENSOR_FAIL;
    }

    for (i = 0x00; (0xffff != sensor_reg_ptr[i].reg_addr) ||
                   (0xff != sensor_reg_ptr[i].reg_value);
         i++) {
        Sensor_WriteReg(sensor_reg_ptr[i].reg_addr,
                        sensor_reg_ptr[i].reg_value);
    }

    SENSOR_LOGI("%ld", param);
    return 0;
}

static SENSOR_IOCTL_FUNC_TAB_T s_s5k4h8yx_ioctl_func_tab = {
    //_s5k4h8yx_Create,
    //_s5k4h8yx_Destroy,
    PNULL, _s5k4h8yx_PowerOn, PNULL, _s5k4h8yx_Identify,

    PNULL, // write register
    PNULL, // read  register
    PNULL, _s5k4h8yx_GetResolutionTrimTab,

    // External
    PNULL, PNULL, PNULL,

    PNULL, //_s5k4h8yx_set_brightness,
    PNULL, // _s5k4h8yx_set_contrast,
    PNULL,
    PNULL, //_s5k4h8yx_set_saturation,

    PNULL, //_s5k4h8yx_set_work_mode,
    PNULL, //_s5k4h8yx_set_image_effect,

    _s5k4h8yx_BeforeSnapshot,
    PNULL, //_s5k4h8yx_after_snapshot,
    PNULL, //_s5k4h8yx_flash,
    PNULL,
    PNULL, //_s5k4h8yx_write_exposure,
    PNULL, _s5k4h8yx_write_gain, PNULL, PNULL,
    PNULL, //_s5k4h8yx_write_af,
    PNULL,
    PNULL, //_s5k4h8yx_set_awb,
    PNULL, PNULL,
    PNULL, //_s5k4h8yx_set_ev,
    PNULL, PNULL, PNULL,
    PNULL, //_s5k4h8yx_GetExifInfo,
    _s5k4h8yx_ExtFunc,
    PNULL, //_s5k4h8yx_set_anti_flicker,
    PNULL, //_s5k4h8yx_set_video_mode,
    PNULL, // pick_jpeg_stream
    PNULL, // meter_mode
    PNULL, // get_status
    _s5k4h8yx_StreamOn, _s5k4h8yx_StreamOff, _s5k4h8yx_access_val,
    _s5k4h8yx_ex_write_exposure, PNULL};

static SENSOR_STATIC_INFO_T s_s5k4h8yx_static_info = {
    200,      // f-number,focal ratio
    287,      // focal_length;
    0,        // max_fps,max fps of sensor's all settings
    16 * 256, // max_adgain,AD-gain
    0,        // ois_supported;
    0,        // pdaf_supported;
    1,        // exp_valid_frame_num;N+2-1
    64,       // clamp_level,black level
    1,        // adgain_valid_frame_num;N+1-1
};

static SENSOR_MODE_FPS_INFO_T s_s5k4h8yx_mode_fps_info = {
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

SENSOR_INFO_T g_s5k4h8yx_mipi_raw_info = {
    S5K4H8YX_I2C_ADDR_W, // salve i2c write address
    S5K4H8YX_I2C_ADDR_R, // salve i2c read address

    SENSOR_I2C_REG_16BIT | SENSOR_I2C_VAL_16BIT |
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
    5,                      // reset pulse width(ms)

    SENSOR_LOW_LEVEL_PWDN, // 1: high level valid; 0: low level valid

    1,           // count of identify code
    {{0x0, 0x2}, // supply two code to identify sensor.
     {0x1,
      0x19}}, // for Example: index = 0-> Device id, index = 1 -> version id

    SENSOR_AVDD_2800MV, // voltage of avdd

    3264,                           // max width of source image
    2448,                           // max height of source image
    (cmr_s8 *) "s5k4h8yx_mipi_raw", // name of sensor

    SENSOR_IMAGE_FORMAT_RAW, // define in SENSOR_IMAGE_FORMAT_E
                             // enum,SENSOR_IMAGE_FORMAT_MAX
    // if set to SENSOR_IMAGE_FORMAT_MAX here, image format depent on
    // SENSOR_REG_TAB_INFO_T

    SENSOR_IMAGE_PATTERN_RAWRGB_GB, // SENSOR_IMAGE_PATTERN_RAWRGB_R,// pattern
                                    // of input image form sensor;

    s_s5k4h8yx_resolution_Tab_RAW, // point to resolution table information
                                   // structure
    &s_s5k4h8yx_ioctl_func_tab,    // point to ioctl function table
    &s_s5k4h8yx_mipi_raw_info_ptr, // information and table about Rawrgb sensor
    NULL, //&g_s5k4h8yx_ext_info,                // extend information about
    // sensor
    SENSOR_AVDD_1800MV, // iovdd
    SENSOR_AVDD_1200MV, // dvdd
    0,                  // skip frame num before preview
    3,                  // skip frame num before capture
    6,                  // skip frame num for flash capture
    0,                  // skip frame num on mipi cap
    0,                  // deci frame num during preview
    0,                  // deci frame num during video preview

    0,
    0,
    0,
    0,
    0,
#if defined(S5K4H8YX_2_LANES)
    {SENSOR_INTERFACE_TYPE_CSI2, 2, 10, 0},
#elif defined(S5K4H8YX_4_LANES)
    {SENSOR_INTERFACE_TYPE_CSI2, 4, 10, 0},
#endif

    s_s5k4h8yx_video_info,
    1,                              // skip frame num while change setting
    48,                             // horizontal view angle
    48,                             // vertical view angle
    (cmr_s8 *) "s5k4h8yx_truly_v1", // version info of sensor
};

static struct sensor_raw_info *Sensor_GetContext(void) {
    return s_s5k4h8yx_mipi_raw_info_ptr;
}

#define param_update(x1, x2)                                                   \
    sprintf(name, "/data/s5k4h8yx_%s.bin", x1);                                \
    if (0 == access(name, R_OK)) {                                             \
        FILE *fp = NULL;                                                       \
        SENSOR_LOGI("param file %s exists", name);                             \
        if (NULL != (fp = fopen(name, "rb"))) {                                \
            fread((void *)x2, 1, sizeof(x2), fp);                              \
            fclose(fp);                                                        \
        } else {                                                               \
            SENSOR_LOGI("param open %s failure", name);                        \
        }                                                                      \
    }                                                                          \
    memset(name, 0, sizeof(name))

#if 0
static uint32_t Sensor_s5k4h8yx_InitRawTuneInfo(void)
{
	uint32_t rtn=0x00;

	isp_raw_para_update_from_file(&g_s5k4h8yx_mipi_raw_info,0);

	struct sensor_raw_info* raw_sensor_ptr=Sensor_GetContext();
	struct isp_mode_param* mode_common_ptr = raw_sensor_ptr->mode_ptr[0].addr;
	int i;
	char name[100] = {'\0'};

	for (i=0; i<mode_common_ptr->block_num; i++) {
		struct isp_block_header* header = &(mode_common_ptr->block_header[i]);
		uint8_t* data = (uint8_t*)mode_common_ptr + header->offset;
		switch (header->block_id)
		{
		case	ISP_BLK_PRE_WAVELET_V1: {
				/* modify block data */
				struct sensor_pwd_param* block = (struct sensor_pwd_param*)data;

				static struct sensor_pwd_level pwd_param[SENSOR_SMART_LEVEL_NUM] = {
#include "noise/pwd_param.h"
				};

				param_update("pwd_param",pwd_param);

				block->param_ptr = pwd_param;
			}
			break;
		case	ISP_BLK_BPC_V1: {
				/* modify block data */
				struct sensor_bpc_param_v1* block = (struct sensor_bpc_param_v1*)data;
				static struct sensor_bpc_level bpc_param[SENSOR_SMART_LEVEL_NUM] = {
#include "noise/bpc_param.h"
				};

				param_update("bpc_param",bpc_param);

				block->param_ptr = bpc_param;
			}
			break;

		case	ISP_BLK_BL_NR_V1: {
				/* modify block data */
				struct sensor_bdn_param* block = (struct sensor_bdn_param*)data;

				static struct sensor_bdn_level bdn_param[SENSOR_SMART_LEVEL_NUM] = {
#include "noise/bdn_param.h"
				};

				param_update("bdn_param",bdn_param);

				block->param_ptr = bdn_param;
			}
			break;

		case	ISP_BLK_GRGB_V1: {
				/* modify block data */
				struct sensor_grgb_v1_param* block = (struct sensor_grgb_v1_param*)data;
				static struct sensor_grgb_v1_level grgb_param[SENSOR_SMART_LEVEL_NUM] = {

#include "noise/grgb_param.h"
				};

				param_update("grgb_param",grgb_param);

				block->param_ptr = grgb_param;

			}
			break;

		case	ISP_BLK_NLM: {
				/* modify block data */
				struct sensor_nlm_param* block = (struct sensor_nlm_param*)data;

				static struct sensor_nlm_level nlm_param[32] = {
#include "noise/nlm_param.h"
				};

				param_update("nlm_param",nlm_param);

				static struct sensor_vst_level vst_param[32] = {
#include "noise/vst_param.h"
				};

				param_update("vst_param",vst_param);

				static struct sensor_ivst_level ivst_param[32] = {
#include "noise/ivst_param.h"
				};

				param_update("ivst_param",ivst_param);

				static struct sensor_flat_offset_level flat_offset_param[32] = {
#include "noise/flat_offset_param.h"
				};

				param_update("flat_offset_param",flat_offset_param);

				block->param_nlm_ptr = nlm_param;
				block->param_vst_ptr = vst_param;
				block->param_ivst_ptr = ivst_param;
				block->param_flat_offset_ptr = flat_offset_param;
			}
			break;

		case	ISP_BLK_CFA_V1: {
				/* modify block data */
				struct sensor_cfa_param_v1* block = (struct sensor_cfa_param_v1*)data;
				static struct sensor_cfae_level cfae_param[SENSOR_SMART_LEVEL_NUM] = {

#include "noise/cfae_param.h"
				};

				param_update("cfae_param",cfae_param);

				block->param_ptr = cfae_param;
			}
			break;

		case	ISP_BLK_RGB_PRECDN: {
				/* modify block data */
				struct sensor_rgb_precdn_param* block = (struct sensor_rgb_precdn_param*)data;

				static struct sensor_rgb_precdn_level precdn_param[SENSOR_SMART_LEVEL_NUM] = {
#include "noise/rgb_precdn_param.h"
				};

				param_update("rgb_precdn_param",precdn_param);

				block->param_ptr = precdn_param;
			}
			break;

		case	ISP_BLK_YUV_PRECDN: {
				/* modify block data */
				struct sensor_yuv_precdn_param* block = (struct sensor_yuv_precdn_param*)data;

				static struct sensor_yuv_precdn_level yuv_precdn_param[SENSOR_SMART_LEVEL_NUM] = {
#include "noise/yuv_precdn_param.h"
				};

				param_update("yuv_precdn_param",yuv_precdn_param);

				block->param_ptr = yuv_precdn_param;
			}
			break;

		case	ISP_BLK_PREF_V1: {
				/* modify block data */
				struct sensor_prfy_param* block = (struct sensor_prfy_param*)data;

				static struct sensor_prfy_level prfy_param[SENSOR_SMART_LEVEL_NUM] = {
#include "noise/prfy_param.h"
				};

				param_update("prfy_param",prfy_param);

				block->param_ptr = prfy_param;
			}
			break;

		case	ISP_BLK_UV_CDN: {
				/* modify block data */
				struct sensor_uv_cdn_param* block = (struct sensor_uv_cdn_param*)data;

				static struct sensor_uv_cdn_level uv_cdn_param[SENSOR_SMART_LEVEL_NUM] = {
#include "noise/yuv_cdn_param.h"
				};

				param_update("yuv_cdn_param",uv_cdn_param);

				block->param_ptr = uv_cdn_param;
			}
			break;

		case	ISP_BLK_EDGE_V1: {

				/* modify block data */
				struct sensor_ee_param* block = (struct sensor_ee_param*)data;

				static struct sensor_ee_level edge_param[SENSOR_SMART_LEVEL_NUM] = {
#include "noise/edge_param.h"
				};

				param_update("edge_param",edge_param);

				block->param_ptr = edge_param;
			}
			break;

		case	ISP_BLK_UV_POSTCDN: {
				/* modify block data */
				struct sensor_uv_postcdn_param* block = (struct sensor_uv_postcdn_param*)data;

				static struct sensor_uv_postcdn_level uv_postcdn_param[SENSOR_SMART_LEVEL_NUM] = {
#include "noise/yuv_postcdn_param.h"
				};

				param_update("yuv_postcdn_param",uv_postcdn_param);

				block->param_ptr = uv_postcdn_param;
			}
			break;

		case	ISP_BLK_IIRCNR_IIR: {
				/* modify block data */
				struct sensor_iircnr_param* block = (struct sensor_iircnr_param*)data;

				static struct sensor_iircnr_level iir_cnr_param[SENSOR_SMART_LEVEL_NUM] = {
#include "noise/iircnr_param.h"
				};

				param_update("iircnr_param",iir_cnr_param);

				block->param_ptr = iir_cnr_param;
			}
			break;

		case	ISP_BLK_IIRCNR_YRANDOM: {
				/* modify block data */
				struct sensor_iircnr_yrandom_param* block = (struct sensor_iircnr_yrandom_param*)data;
				static struct sensor_iircnr_yrandom_level iir_yrandom_param[SENSOR_SMART_LEVEL_NUM] = {
#include "noise/iir_yrandom_param.h"
				};

				param_update("iir_yrandom_param",iir_yrandom_param);

				block->param_ptr = iir_yrandom_param;
			}
			break;

		case  ISP_BLK_UVDIV_V1: {
				/* modify block data */
				struct sensor_cce_uvdiv_param_v1* block = (struct sensor_cce_uvdiv_param_v1*)data;

				static struct sensor_cce_uvdiv_level cce_uvdiv_param[SENSOR_SMART_LEVEL_NUM] = {
#include "noise/cce_uv_param.h"
				};

				param_update("cce_uv_param",cce_uvdiv_param);

				block->param_ptr = cce_uvdiv_param;
			}
			break;
		case ISP_BLK_YIQ_AFM:{
			/* modify block data */
			struct sensor_y_afm_param *block = (struct sensor_y_afm_param*)data;

			static struct sensor_y_afm_level y_afm_param[SENSOR_SMART_LEVEL_NUM] = {
#include "noise/y_afm_param.h"
				};

				param_update("y_afm_param",y_afm_param);

				block->param_ptr = y_afm_param;
			}
			break;

		default:
			break;
		}
	}


	return rtn;
}
#endif

static uint32_t _dw9807_SRCInit(SENSOR_HW_HANDLE handle, uint32_t mode) {
    uint8_t cmd_val[2] = {0x00};
    uint16_t slave_addr = 0;
    uint16_t cmd_len = 0;
    uint32_t ret_value = SENSOR_SUCCESS;
    SENSOR_LOGI("SENSOR_S5K4H8YX: %d", mode);

    slave_addr = DW9807_VCM_SLAVE_ADDR;
    switch (mode) {
    case 1:
        break;

    case 2: {
        cmd_len = 2;

        cmd_val[0] = 0x02;
        cmd_val[1] = 0x01;
        ret_value =
            Sensor_WriteI2C(slave_addr, (uint8_t *)&cmd_val[0], cmd_len);
        if (ret_value) {
            SENSOR_LOGI("SENSOR_S5K4H8YX: _dw9807_SRCInit 0 fail!");
        }

        cmd_val[0] = 0x02;
        cmd_val[1] = 0x00;
        ret_value =
            Sensor_WriteI2C(slave_addr, (uint8_t *)&cmd_val[0], cmd_len);
        if (ret_value) {
            SENSOR_LOGI("SENSOR_S5K4H8YX: _dw9807_SRCInit 1 fail!");
        }

        //		usleep(200);

        cmd_val[0] = 0x02;
        cmd_val[1] = 0x02;
        ret_value =
            Sensor_WriteI2C(slave_addr, (uint8_t *)&cmd_val[0], cmd_len);
        if (ret_value) {
            SENSOR_LOGI("SENSOR_S5K4H8YX: _dw9807_SRCInit 2 fail!");
        }

        cmd_val[0] = 0x06;
        cmd_val[1] = 0x61;
        ret_value =
            Sensor_WriteI2C(slave_addr, (uint8_t *)&cmd_val[0], cmd_len);
        if (ret_value) {
            SENSOR_LOGI("SENSOR_S5K4H8YX: _dw9807_SRCInit 3 fail!");
        }

        cmd_val[0] = 0x07;
        cmd_val[1] = 0x30;
        ret_value =
            Sensor_WriteI2C(slave_addr, (uint8_t *)&cmd_val[0], cmd_len);
        if (ret_value) {
            SENSOR_LOGI("SENSOR_S5K4H8YX: _dw9807_SRCInit 4 fail!");
        }

    } break;
    case 3:
        break;
    }

    return ret_value;
}

static unsigned long _s5k4h8yx_GetResolutionTrimTab(SENSOR_HW_HANDLE handle,
                                                    unsigned long param) {
    SENSOR_LOGI("0x%lx", (unsigned long)s_s5k4h8yx_Resolution_Trim_Tab);
    return (unsigned long)s_s5k4h8yx_Resolution_Trim_Tab;
}

SENSOR_HW_HANDLE _s5k4h8yx_Create(void *privatedata) {
    SENSOR_LOGI("_s5k3p3sm_Create  IN");
    if (NULL == privatedata) {
        SENSOR_LOGI("_s5k3p3sm_Create invalied para");
        return NULL;
    }
    SENSOR_HW_HANDLE sensor_hw_handle =
        (SENSOR_HW_HANDLE)malloc(sizeof(SENSOR_HW_HANDLE));
    if (NULL == sensor_hw_handle) {
        SENSOR_LOGI("failed to create sensor_hw_handle");
        return NULL;
    }
    cmr_bzero(sensor_hw_handle, sizeof(SENSOR_HW_HANDLE));
    sensor_hw_handle->privatedata = privatedata;

    return sensor_hw_handle;
}

void _s5k4h8yx_Destroy(SENSOR_HW_HANDLE handle) {
    SENSOR_LOGI("_s5k3p3sm_Destroy  IN");
    if (NULL == handle) {
        SENSOR_LOGI("_s5k3p3sm_Destroy handle null");
    }
    free((void *)handle->privatedata);
    handle->privatedata = NULL;
}

static unsigned long _s5k4h8yx_PowerOn(SENSOR_HW_HANDLE handle,
                                       unsigned long power_on) {
    SENSOR_AVDD_VAL_E dvdd_val = g_s5k4h8yx_mipi_raw_info.dvdd_val;
    SENSOR_AVDD_VAL_E avdd_val = g_s5k4h8yx_mipi_raw_info.avdd_val;
    SENSOR_AVDD_VAL_E iovdd_val = g_s5k4h8yx_mipi_raw_info.iovdd_val;
    BOOLEAN power_down = g_s5k4h8yx_mipi_raw_info.power_down_level;
    BOOLEAN reset_level = g_s5k4h8yx_mipi_raw_info.reset_pulse_level;

    uint8_t pid_value = 0x00;

    SENSOR_LOGI("In");
    if (SENSOR_TRUE == power_on) {
        Sensor_SetResetLevel(reset_level);
        Sensor_PowerDown(power_down);
        Sensor_SetVoltage(dvdd_val, avdd_val, iovdd_val);
        usleep(1);
        Sensor_PowerDown(!power_down);
        Sensor_SetResetLevel(!reset_level);
        usleep(1000);
        Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);
        usleep(1000);
    } else {
        Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
        Sensor_SetResetLevel(reset_level);
        Sensor_PowerDown(power_down);
        Sensor_SetVoltage(SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED,
                          SENSOR_AVDD_CLOSED);
    }

    SENSOR_LOGI("SENSOR_S5K4H8YX: _s5k4h8yx_Power_On(1:on, 0:off): %ld, "
                "reset_level %d, dvdd_val %d",
                power_on, reset_level, dvdd_val);
    return SENSOR_SUCCESS;
}

#ifdef FEATURE_OTP
/*==============================================================================
 * Description:
 * get  parameters from otp
 * please modify this function acording your spec
 *============================================================================*/
static int s5k4h8yx_get_otp_info(SENSOR_HW_HANDLE handle,
                                 struct otp_info_t *otp_info) {
    uint32_t ret = SENSOR_FAIL;
    uint32_t i = 0x00;

    // identify otp information
    for (i = 0; i < NUMBER_OF_ARRAY(s_s5k4h8yx_raw_param_tab); i++) {
        SENSOR_LOGI("identify module_id=0x%x",
                    s_s5k4h8yx_raw_param_tab[i].param_id);

        if (PNULL != s_s5k4h8yx_raw_param_tab[i].identify_otp) {
            // set default value;
            memset(otp_info, 0x00, sizeof(struct otp_info_t));

            if (SENSOR_SUCCESS ==
                s_s5k4h8yx_raw_param_tab[i].identify_otp(handle, otp_info)) {
                if (s_s5k4h8yx_raw_param_tab[i].param_id ==
                    otp_info->module_id) {
                    SENSOR_LOGI("identify otp sucess! module_id=0x%x",
                                s_s5k4h8yx_raw_param_tab[i].param_id);
                    ret = SENSOR_SUCCESS;
                    break;
                } else {
                    SENSOR_LOGI("identify module_id failed! table "
                                "module_id=0x%x, otp module_id=0x%x",
                                s_s5k4h8yx_raw_param_tab[i].param_id,
                                otp_info->module_id);
                }
            } else {
                SENSOR_LOGI("identify_otp failed!");
            }
        } else {
            SENSOR_LOGI("no identify_otp function!");
        }
    }

    if (SENSOR_SUCCESS == ret)
        return i;
    else
        return -1;
}

/*==============================================================================
 * Description:
 * apply otp parameters to sensor register
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t s5k4h8yx_apply_otp(SENSOR_HW_HANDLE handle,
                                   struct otp_info_t *otp_info, int id) {
    uint32_t ret = SENSOR_FAIL;
    // apply otp parameters
    SENSOR_LOGI("otp_table_id = %d", id);
    if (PNULL != s_s5k4h8yx_raw_param_tab[id].cfg_otp) {
        if (SENSOR_SUCCESS ==
            s_s5k4h8yx_raw_param_tab[id].cfg_otp(handle, otp_info)) {
            SENSOR_LOGI("apply otp parameters sucess! module_id=0x%x",
                        s_s5k4h8yx_raw_param_tab[id].param_id);
            ret = SENSOR_SUCCESS;
        } else {
            SENSOR_LOGI("update_otp failed!");
        }
    } else {
        SENSOR_LOGI("no update_otp function!");
    }

    return ret;
}

static uint32_t s5k4h8yx_cfg_otp(SENSOR_HW_HANDLE handle) {
    uint32_t ret = SENSOR_FAIL;
    struct otp_info_t otp_info = {0x00};
    int table_id = 0;

    table_id = s5k4h8yx_get_otp_info(handle, &otp_info);
    if (-1 != table_id) {
        ret = s5k4h8yx_apply_otp(handle, &otp_info, table_id);
    }
    // checking OTP apply result
    if (SENSOR_SUCCESS != ret) { // disable lsc
        // Sensor_WriteReg(0x0B00,0x0080);
    } else { // enable lsc
        // Sensor_WriteReg(0x0B00,0x0180);
        // s5k4h8yx_power_on(SENSOR_FALSE);
    }

    return ret;
}
#endif

static uint32_t _s5k4h8yx_init_otp(SENSOR_HW_HANDLE handle) {
    uint32_t ret = SENSOR_SUCCESS;
#ifdef FEATURE_OTP
    ret = s5k4h8yx_cfg_otp(handle);
#endif
    return ret;
}

static uint32_t _s5k4h8yx_GetMaxFrameLine(SENSOR_HW_HANDLE handle,
                                          uint32_t index) {
    uint32_t max_line = 0x00;
    SENSOR_TRIM_T_PTR trim_ptr = s_s5k4h8yx_Resolution_Trim_Tab;

    max_line = trim_ptr[index].frame_line;
    s_current_default_line_time = trim_ptr[index].line_time;

    return max_line;
}

static uint32_t _s5k4h8yx_init_mode_fps_info(SENSOR_HW_HANDLE handle) {
    uint32_t rtn = SENSOR_SUCCESS;
    SENSOR_LOGI("_s5k4h8yx_init_mode_fps_info:E");
    if (!s_s5k4h8yx_mode_fps_info.is_init) {
        uint32_t i, modn, tempfps = 0;
        SENSOR_LOGI("_s5k4h8yx_init_mode_fps_info:start init");
        for (i = 0; i < NUMBER_OF_ARRAY(s_s5k4h8yx_Resolution_Trim_Tab); i++) {
            // max fps should be multiple of 30,it calulated from line_time and
            // frame_line
            tempfps = s_s5k4h8yx_Resolution_Trim_Tab[i].line_time *
                      s_s5k4h8yx_Resolution_Trim_Tab[i].frame_line;
            if (0 != tempfps) {
                tempfps = 1000000000 / tempfps;
                modn = tempfps / 30;
                if (tempfps > modn * 30)
                    modn++;
                s_s5k4h8yx_mode_fps_info.sensor_mode_fps[i].max_fps = modn * 30;
                if (s_s5k4h8yx_mode_fps_info.sensor_mode_fps[i].max_fps > 30) {
                    s_s5k4h8yx_mode_fps_info.sensor_mode_fps[i].is_high_fps = 1;
                    s_s5k4h8yx_mode_fps_info.sensor_mode_fps[i]
                        .high_fps_skip_num =
                        s_s5k4h8yx_mode_fps_info.sensor_mode_fps[i].max_fps /
                        30;
                }
                if (s_s5k4h8yx_mode_fps_info.sensor_mode_fps[i].max_fps >
                    s_s5k4h8yx_static_info.max_fps) {
                    s_s5k4h8yx_static_info.max_fps =
                        s_s5k4h8yx_mode_fps_info.sensor_mode_fps[i].max_fps;
                }
            }
            SENSOR_LOGI("mode %d,tempfps %d,frame_len %d,line_time: %d ", i,
                        tempfps, s_s5k4h8yx_Resolution_Trim_Tab[i].frame_line,
                        s_s5k4h8yx_Resolution_Trim_Tab[i].line_time);
            SENSOR_LOGI("_s5k4h8yx:init_mode_fps_info:mode %d,max_fps: %d ", i,
                        s_s5k4h8yx_mode_fps_info.sensor_mode_fps[i].max_fps);
            SENSOR_LOGI(
                "_s5k4h8yx:init_mode_fps_info:is_high_fps: %d,highfps_skip_num "
                "%d",
                s_s5k4h8yx_mode_fps_info.sensor_mode_fps[i].is_high_fps,
                s_s5k4h8yx_mode_fps_info.sensor_mode_fps[i].high_fps_skip_num);
        }
        s_s5k4h8yx_mode_fps_info.is_init = 1;
    }
    SENSOR_LOGI("_s5k4h8yx_init_mode_fps_info:X");
    return rtn;
}

static uint32_t _s5k4h8yx_GetRawInof(SENSOR_HW_HANDLE handle) {
    uint32_t rtn = SENSOR_SUCCESS;
    void *handlelib;
    handlelib = dlopen("libcamsensortuning.so", RTLD_NOW);
    if (handlelib == NULL) {
        char const *err_str = dlerror();
        ALOGE("dlopen error%s", err_str ? err_str : "unknown");
    }

    /* Get the address of the struct hal_module_info. */
    const char *sym = S5K4H8YX_RAW_PARAM_AS_STR;

    s_s5k4h8yx_mipi_raw_info_ptr = (oem_module_t *)dlsym(handlelib, sym);
    if (s_s5k4h8yx_mipi_raw_info_ptr == NULL) {
        SENSOR_LOGI("load: couldn't find symbol %s", sym);
    } else {
        // s_s5k4h8yx_mipi_raw_info_ptr = tab_ptr[i].info_ptr;
        SENSOR_LOGI("SENSOR_S5K4H8YX: _s5k4h8yx_GetRawInof success");
    }

    return rtn;
}

static unsigned long _s5k4h8yx_Identify(SENSOR_HW_HANDLE handle,
                                        unsigned long param) {
#define S5K4H8YX_PID_VALUE 0x4088
#define S5K4H8YX_PID_ADDR 0x0000
#define S5K4H8YX_VER_VALUE 0xc001
#define S5K4H8YX_VER_ADDR 0x0002
    uint16_t pid_value = 0x00;
    uint16_t ver_value = 0x00;
    uint32_t ret_value = SENSOR_FAIL;

    SENSOR_LOGI("SENSOR_S5K4H8YX: mipi raw identify\n");

    //	while(1) {
    pid_value = Sensor_ReadReg(S5K4H8YX_PID_ADDR);
    ver_value = Sensor_ReadReg(S5K4H8YX_VER_ADDR);
    SENSOR_LOGI("SENSOR_S5K4H8YX: Identify: PID = %x, VER = %x", pid_value,
                ver_value);
    usleep(50 * 1000);
    //	}

    if (S5K4H8YX_PID_VALUE == pid_value) {
        if (S5K4H8YX_VER_VALUE == ver_value) {
            ret_value = SENSOR_SUCCESS;
            SENSOR_LOGI("SENSOR_S5K4H8YX: this is S5K4H8YX sensor !");
            ret_value = _s5k4h8yx_GetRawInof(handle);
            if (SENSOR_SUCCESS != ret_value) {
                SENSOR_LOGI("SENSOR_S5K4H8YX: the module is unknow error !");
            }
            //			_dw9807_SRCInit(2);
            //			Sensor_s5k4h8yx_InitRawTuneInfo();
            _s5k4h8yx_init_mode_fps_info(handle);
        } else {
            SENSOR_LOGI("SENSOR_S5K4H8YX: Identify this is hm%x%x sensor !",
                        pid_value, ver_value);
            return ret_value;
        }
    }

    return ret_value;
}

static unsigned long _s5k4h8yx_ex_write_exposure(SENSOR_HW_HANDLE handle,
                                                 unsigned long param) {
    uint32_t ret_value = SENSOR_SUCCESS;
    uint16_t exposure_line = 0x00;
    uint16_t dummy_line = 0x00;
    uint16_t size_index = 0x00;
    struct sensor_ex_exposure *ex = (struct sensor_ex_exposure *)param;

    if (!param) {
        SENSOR_LOGI("param is NULL !!!");
        return ret_value;
    }

    exposure_line = ex->exposure;
    dummy_line = ex->dummy;
    size_index = ex->size_index;

    ret_value = _s5k4h8yx_write_exp_dummy(handle, exposure_line, dummy_line,
                                          size_index);

    return ret_value;
}

static unsigned long _s5k4h8yx_write_exposure(SENSOR_HW_HANDLE handle,
                                              unsigned long param) {
    uint32_t ret_value = SENSOR_SUCCESS;
    uint32_t expsure_line = 0x00;
    uint32_t dummy_line = 0x00;
    uint32_t size_index = 0x00;

    expsure_line = param & 0xffff;
    dummy_line = (0 >> 0x10) & 0x0fff;
    size_index = (param >> 0x1c) & 0x0f;

    ret_value =
        _s5k4h8yx_write_exp_dummy(handle, expsure_line, dummy_line, size_index);

    return ret_value;
}

// uint32_t s_af_step=0x00;
static unsigned long _s5k4h8yx_write_exp_dummy(SENSOR_HW_HANDLE handle,
                                               uint16_t expsure_line,
                                               uint16_t dummy_line,
                                               uint16_t size_index) {
    uint32_t ret_value = SENSOR_SUCCESS;
    uint16_t frame_len_cur = 0x00;
    uint16_t frame_len = 0x00;
    uint16_t max_frame_len = 0x00;
    uint32_t linetime = 0;

    SENSOR_LOGI(
        "SENSOR_s5k3h2yx: write_exposure line:%d, dummy:%d, size_index:%d",
        expsure_line, dummy_line, size_index);
    max_frame_len = _s5k4h8yx_GetMaxFrameLine(handle, size_index);
    if (expsure_line < 2) {
        expsure_line = 2;
    }
    s_current_default_frame_length = max_frame_len;

    frame_len = expsure_line + dummy_line;
    frame_len = frame_len > (expsure_line + 8) ? frame_len : (expsure_line + 8);
    frame_len = (frame_len > max_frame_len) ? frame_len : max_frame_len;
    if (0x00 != (0x01 & frame_len)) {
        frame_len += 0x01;
    }

    frame_len_cur = Sensor_ReadReg(0x0340) & 0xffff;

    // ret_value = Sensor_WriteReg(0x104, 0x01); //group_hold_on

    if (frame_len_cur != frame_len) {
        Sensor_WriteReg(0x0340, frame_len & 0xffff);
    }
    s_current_frame_length = frame_len;

    Sensor_WriteReg(0x0202, expsure_line & 0xffff);
    s_capture_shutter = expsure_line;
    linetime = s_s5k4h8yx_Resolution_Trim_Tab[size_index].line_time;
    Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_EXPOSURETIME, s_capture_shutter);
    s_exposure_time = s_capture_shutter * linetime / 1000;
    s_sensor_ev_info.preview_shutter = s_capture_shutter;

/*if (frame_len_cur > frame_len) {
        ret_value = Sensor_WriteReg(0x0341, frame_len & 0xff);
        ret_value = Sensor_WriteReg(0x0340, (frame_len >> 0x08) & 0xff);
}*/
//	ret_value = Sensor_WriteReg(0x104, 0x00);

#if 0
	_s5k4h8yx_write_af(s_af_step);

	s_af_step+=50;

	if(1000==s_af_step)
		s_af_step=0;
#endif

    return ret_value;
}

static unsigned long _s5k4h8yx_write_gain(SENSOR_HW_HANDLE handle,
                                          unsigned long param) {
    uint32_t ret_value = SENSOR_SUCCESS;
    // uint32_t real_gain = 0;
    float real_gain = 0;
    float a_gain = 0;
    float d_gain = 0;

    real_gain = 1.0f * param * 0x0020 / 0x80;

    SENSOR_LOGI("_s5k4h8yx: real_gain:0x%x, param: %ul", (uint32_t)real_gain,
                param);
    s_sensor_ev_info.preview_gain = param;

    if ((uint32_t)real_gain <= 16 * 32) {
        a_gain = real_gain;
        d_gain = 256;
        // ret_value =
        // Sensor_WriteReg(0x204,(uint32_t)a_gain);//0x100);//a_gain);
    } else {
        a_gain = 16 * 32;
        d_gain = 256.0 * real_gain / a_gain;
        SENSOR_LOGI("_s5k4h8yx: real_gain:0x%x, a_gain: 0x%x, d_gain: 0x%x",
                    (uint32_t)real_gain, (uint32_t)a_gain, (uint32_t)d_gain);
        if ((uint32_t)d_gain > 256 * 256)
            d_gain = 256 * 256; // d_gain < 256x
    }

    ret_value = Sensor_WriteReg(0x204, (uint32_t)a_gain); // 0x100);//a_gain);
    ret_value = Sensor_WriteReg(0x20e, (uint32_t)d_gain);
    ret_value = Sensor_WriteReg(0x210, (uint32_t)d_gain);
    ret_value = Sensor_WriteReg(0x212, (uint32_t)d_gain);
    ret_value = Sensor_WriteReg(0x214, (uint32_t)d_gain);
    //         SENSOR_LOGI("_s5k3l2xx: real_gain a_gain:0x%x read_reg
    //         0x204:0x%x, 0x20e:0x%x, 0x202:0x%x, 0x1086:0x%x",
    //         (uint32_t)a_gain,Sensor_ReadReg(0x0204),Sensor_ReadReg(0x020e),Sensor_ReadReg(0x202),Sensor_ReadReg(0x1086));

    //	Sensor_WriteReg(0x0104, 0x00);

    return ret_value;
}

static unsigned long _s5k4h8yx_write_af(SENSOR_HW_HANDLE handle,
                                        unsigned long param) {
    uint32_t ret_value = SENSOR_SUCCESS;
    uint8_t cmd_val[2] = {0x00};
    uint16_t slave_addr = 0;
    uint16_t cmd_len = 0;

    SENSOR_LOGI("SENSOR_S5K4H8YX: _write_af %ld", param);
    slave_addr = DW9807_VCM_SLAVE_ADDR;

    cmd_val[0] = 0x03;
    cmd_val[1] = (param >> 8) & 0x03;
    cmd_len = 2;
    ret_value = Sensor_WriteI2C(slave_addr, (uint8_t *)&cmd_val[0], cmd_len);
    SENSOR_LOGI("SENSOR_S5K4H8YX: _write_af, ret =  %d, MSL:%x, LSL:%x\n",
                ret_value, cmd_val[0], cmd_val[1]);

    cmd_val[0] = 0x04;
    cmd_val[1] = param & 0xff;
    cmd_len = 2;
    ret_value = Sensor_WriteI2C(slave_addr, (uint8_t *)&cmd_val[0], cmd_len);

    SENSOR_LOGI("SENSOR_S5K4H8YX: _write_af, ret =  %d, MSL:%x, LSL:%x\n",
                ret_value, cmd_val[0], cmd_val[1]);
    return ret_value;
}

static unsigned long _s5k4h8yx_BeforeSnapshot(SENSOR_HW_HANDLE handle,
                                              unsigned long param) {
    uint8_t ret_l, ret_m, ret_h;
    uint32_t capture_exposure, preview_maxline;
    uint32_t capture_maxline, preview_exposure;
    uint32_t capture_mode = param & 0xffff;
    uint32_t preview_mode = (param >> 0x10) & 0xffff;
    uint32_t prv_linetime =
        s_s5k4h8yx_Resolution_Trim_Tab[preview_mode].line_time;
    uint32_t cap_linetime =
        s_s5k4h8yx_Resolution_Trim_Tab[capture_mode].line_time;
    uint32_t gain = 0;
    SENSOR_LOGI("SENSOR_S5K4H8YX: BeforeSnapshot mode: %ul", param);

    if (preview_mode == capture_mode) {
        SENSOR_LOGI("SENSOR_S5K4H8YX: prv mode equal to capmode");
        goto CFG_INFO;
    }

    preview_exposure = _s5k4h8yx_get_shutter(handle);
    preview_maxline = _s5k4h8yx_get_VTS(handle);

    Sensor_SetMode(capture_mode);
    Sensor_SetMode_WaitDone();
    // preview_exposure = s_sensor_ev_info.preview_shutter;
    gain = s_sensor_ev_info.preview_gain;

    if (prv_linetime == cap_linetime) {
        SENSOR_LOGI("SENSOR_S5K4H8YX: prvline equal to capline");
        goto CFG_INFO;
    }

    capture_maxline = _s5k4h8yx_get_VTS(handle);

    capture_exposure = preview_exposure * prv_linetime / cap_linetime;

    SENSOR_LOGI("BeforeSnapshot: capture_exposure 0x%x, capture_maxline "
                "0x%x,preview_maxline 0x%x, preview_exposure 0x%x",
                capture_exposure, capture_maxline, preview_maxline,
                preview_exposure);

    if (0 == capture_exposure) {
        capture_exposure = 1;
    }

    if (capture_exposure > (capture_maxline - 4)) {
        capture_maxline = capture_exposure + 4;
        //		capture_maxline = (capture_maxline+1)>>1<<1;
        //		ret_l = (unsigned char)(capture_maxline&0x0ff);
        //		ret_h = (unsigned char)((capture_maxline >> 8)&0xff);
        _s5k4h8yx_set_VTS(handle, capture_maxline);
    }

    /*
            ret_l = ((unsigned char)capture_exposure&0xf) << 4;
            ret_m = (unsigned char)((capture_exposure&0xfff) >> 4) & 0xff;
            ret_h = (unsigned char)(capture_exposure >> 12);
            */
    _s5k4h8yx_set_shutter(handle, capture_exposure);

CFG_INFO:
    // s_capture_shutter = _s5k4h8yx_get_shutter(handle);
    s_capture_VTS = _s5k4h8yx_get_VTS(handle);
    _s5k4h8yx_ReadGain(handle, &gain);
    Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_EXPOSURETIME, s_capture_shutter);
    Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_APERTUREVALUE, 20);
    Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_MAXAPERTUREVALUE, 20);
    Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_FNUMBER, 20);
    s_exposure_time = s_capture_shutter * cap_linetime / 1000;

    return SENSOR_SUCCESS;
}

static unsigned long _s5k4h8yx_after_snapshot(SENSOR_HW_HANDLE handle,
                                              unsigned long param) {
    SENSOR_LOGI("SENSOR_s5k4h8yx: after_snapshot mode:%ld", param);
    Sensor_SetMode((uint32_t)param);
    return SENSOR_SUCCESS;
}

static unsigned long _s5k4h8yx_flash(SENSOR_HW_HANDLE handle,
                                     unsigned long param) {
    SENSOR_LOGI("Start:param=%ld", param);

    /* enable flash, disable in _s5k4h8yx_BeforeSnapshot */
    g_flash_mode_en = (uint32_t)param;
    Sensor_SetFlash((uint32_t)param);
    SENSOR_LOGI("end");
    return SENSOR_SUCCESS;
}

static unsigned long _s5k4h8yx_GetExifInfo(SENSOR_HW_HANDLE handle,
                                           unsigned long param) {
    LOCAL EXIF_SPEC_PIC_TAKING_COND_T sexif;
    sexif.ExposureTime.numerator = s_exposure_time;
    sexif.ExposureTime.denominator = 1000000;

    return (unsigned long)&sexif;
}

static unsigned long _s5k4h8yx_StreamOn(SENSOR_HW_HANDLE handle,
                                        unsigned long param) {
    SENSOR_LOGI("SENSOR_s5k4h8yx: StreamOn");

    Sensor_WriteReg(0x0100, 0x0103);
#if 1
    char value1[PROPERTY_VALUE_MAX];
    property_get("debug.camera.test.mode", value1, "0");
    if (!strcmp(value1, "1")) {
        SENSOR_LOGI("SENSOR_s5k4h8yx: enable test mode");
        Sensor_WriteReg(0x0600, 0x0002);
    }
#endif

    return 0;
}

static unsigned long _s5k4h8yx_StreamOff(SENSOR_HW_HANDLE handle,
                                         unsigned long param) {

    uint16_t value;

    SENSOR_LOGI("E");

    value = Sensor_ReadReg(0x0100);
    Sensor_WriteReg(0x0100, 0x0003);
    if (value == 0x0103)
            usleep(50 * 1000);

    return 0;
}

static uint16_t _s5k4h8yx_get_shutter(SENSOR_HW_HANDLE handle) {
    // read shutter, in number of line period
    uint16_t shutter = 0;
#if 1 // MP tool //!??
    shutter = Sensor_ReadReg(0x0202);

    return shutter;
#else
    return s_set_exposure;
#endif
}

static uint32_t _s5k4h8yx_set_shutter(SENSOR_HW_HANDLE handle,
                                      uint16_t shutter) {
    // Sensor_WriteReg(0x104, 0x01);
    // write shutter, in number of line period
    Sensor_WriteReg(0x0202, shutter);
    // Sensor_WriteReg(0x104, 0x00);

    return 0;
}

static uint32_t _s5k4h8yx_get_gain16(SENSOR_HW_HANDLE handle) {
    // read gain, 16 = 1x
    uint32_t gain16;

    gain16 =
        (256 * 16) / (256 - Sensor_ReadReg(0x0157)); // a_gain= 256/(256-x);

    return gain16;
}

static uint16_t _s5k4h8yx_set_gain16(SENSOR_HW_HANDLE handle, uint32_t gain16) {
    // write gain, 16 = 1x
    uint16_t temp;
    gain16 = gain16 & 0x3ff;

    if (gain16 < 16)
        gain16 = 16;
    if (gain16 > 170)
        gain16 = 170;

    temp = (256 * (gain16 - 16)) / gain16;
    Sensor_WriteReg(0x0157, temp & 0xff);

    return 0;
}

// uint32_t s_af_step=0x00;
static unsigned long _s5k4h8yx_write_exposure_ev(SENSOR_HW_HANDLE handle,
                                                 unsigned long param) {
    uint32_t ret_value = SENSOR_SUCCESS;
    uint16_t expsure_line = 0x00;
    uint16_t dummy_line = 0x00;
    uint16_t frame_len_cur = 0x00;
    uint16_t frame_len = 0x00;
    uint16_t size_index = 0x00;
    uint16_t max_frame_len = 0x00;

    expsure_line = param;
    s_set_exposure = expsure_line;
    SENSOR_LOGI("HDR_TEST: write_exposure_ev line:%d, dummy:%d, size_index:%d",
                expsure_line, dummy_line, size_index);
    max_frame_len = 2480;
    if (expsure_line < 2) {
        expsure_line = 2;
    }

    frame_len = 2480;
    frame_len = frame_len > (expsure_line + 8) ? frame_len : (expsure_line + 8);
    if (0x00 != (0x01 & frame_len)) {
        frame_len += 0x01;
    }

    frame_len_cur = (Sensor_ReadReg(0x0340)) & 0xffff;
    // frame_len_cur |= (Sensor_ReadReg(0x0340)<<0x08)&0xff00;

    // ret_value = Sensor_WriteReg(0x104, 0x01);
    if (frame_len_cur != frame_len) {
        ret_value = Sensor_WriteReg(0x0340, frame_len & 0xffff);
        // ret_value = Sensor_WriteReg(0x0340, (frame_len >> 0x08) & 0xff);
    }

    ret_value = Sensor_WriteReg(0x202, expsure_line & 0xffff);
    // ret_value = Sensor_WriteReg(0x202, (expsure_line >> 0x08) & 0xff);
    // ret_value = Sensor_WriteReg(0x104, 0x00);
    return ret_value;
}
static void _calculate_hdr_exposure(SENSOR_HW_HANDLE handle, int capture_gain16,
                                    int capture_VTS, int capture_shutter) {
    // write capture gain
    //_s5k4h8yx_set_gain16(capture_gain16);

    // write capture shutter
    /*if (capture_shutter > (capture_VTS - 4)) {
            capture_VTS = capture_shutter + 4;
            OV5640_set_VTS(capture_VTS);
    }*/
    //_s5k4h8yx_set_shutter(capture_shutter);
    _s5k4h8yx_write_exposure_ev(handle, capture_shutter);
}

static uint32_t _s5k4h8yx_SetEV(SENSOR_HW_HANDLE handle, unsigned long param) {
    uint32_t rtn = SENSOR_SUCCESS;
    SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR)param;

    uint16_t value = 0x00;
    uint32_t gain = s_s5k4h8yx_gain;
    uint32_t ev = ext_ptr->param;

    uint16_t expsure_line = 0x00;
    uint16_t line_time = 0x00;
    uint32_t exp_delay_time = 0;

    line_time = 134;

    SENSOR_LOGI("SENSOR: _s5k4h8yx_SetEV param: 0x%x s_capture_shutter %d",
                ext_ptr->param, s_capture_shutter);

    switch (ev) {
    case SENSOR_HDR_EV_LEVE_0:
        _calculate_hdr_exposure(handle, s_s5k4h8yx_gain, s_capture_VTS,
                                s_capture_shutter / 4);
        break;
    case SENSOR_HDR_EV_LEVE_1:
        _calculate_hdr_exposure(handle, s_s5k4h8yx_gain, s_capture_VTS,
                                s_capture_shutter);
        break;
    case SENSOR_HDR_EV_LEVE_2:
        _calculate_hdr_exposure(handle, s_s5k4h8yx_gain, s_capture_VTS,
                                s_capture_shutter * 2);
        break;
    default:
        break;
    }

    expsure_line = s_capture_shutter;

    exp_delay_time = (expsure_line * line_time) / 10;
    exp_delay_time += 10000;

    if (50000 > exp_delay_time) {
        exp_delay_time = 50000;
    }

    usleep(exp_delay_time);

    return rtn;
}

static unsigned long _s5k4h8yx_ExtFunc(SENSOR_HW_HANDLE handle,
                                       unsigned long ctl_param) {
    uint32_t rtn = SENSOR_SUCCESS;
    SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR)ctl_param;
    SENSOR_LOGI("0x%x", ext_ptr->cmd);

    switch (ext_ptr->cmd) {
    case SENSOR_EXT_FUNC_INIT:
        break;
    case SENSOR_EXT_FOCUS_START:
        break;
    case SENSOR_EXT_EXPOSURE_START:
        break;
    case SENSOR_EXT_EV:
        rtn = _s5k4h8yx_SetEV(handle, ctl_param);
        break;
    default:
        break;
    }
    return rtn;
}

static uint16_t _s5k4h8yx_get_VTS(SENSOR_HW_HANDLE handle) {
    // read VTS from register settings
    uint16_t VTS;

    VTS = Sensor_ReadReg(0x0160); // total vertical size[15:8] high byte
    // VTS = (VTS<<8) + Sensor_ReadReg(0x0161);

    return VTS;
}

static uint32_t _s5k4h8yx_set_VTS(SENSOR_HW_HANDLE handle, uint16_t VTS) {
    // write VTS to registers
    Sensor_WriteReg(0x0160, (VTS & 0xffff));
    // Sensor_WriteReg(0x0160, ((VTS>>8)& 0xff));

    return 0;
}

static uint32_t _s5k4h8yx_ReadGain(SENSOR_HW_HANDLE handle, uint32_t *param) {
    uint32_t rtn = SENSOR_SUCCESS;
    uint32_t gain = 0;
    uint16_t a_gain = 0;
    uint16_t d_gain = 0;
    a_gain = Sensor_ReadReg(0x0204);
    d_gain = Sensor_ReadReg(0x0210);
    gain = a_gain * d_gain;

    // gain = Sensor_ReadReg(0x0204);
    // gain = gain<<8|Sensor_ReadReg(0x205);/*8*/

    s_s5k4h8yx_gain = (int)gain;
    *param = gain;

    SENSOR_LOGI("SENSOR_s5k4h8yx: _s5k4h8yx_ReadGain gain: 0x%x",
                s_s5k4h8yx_gain);

    return rtn;
}

static uint32_t _s5k4h8yx_write_otp_gain(SENSOR_HW_HANDLE handle,
                                         uint32_t *param) {
    uint32_t ret_value = SENSOR_SUCCESS;
    uint16_t value = 0x00;
    float a_gain = 0;
    float d_gain = 0;
    float real_gain = 0.0f;
    SENSOR_LOGI("SENSOR_s5k4h8yx: write_gain:0x%x\n", *param);

    // ret_value = Sensor_WriteReg(0x104, 0x01);
    if ((*param / 4) > 0x200) {
        a_gain = 16 * 32;
        d_gain = (*param / 4.0) * 256 / a_gain;
        if (d_gain > 256 * 256)
            d_gain = 256 * 256;
    } else {
        a_gain = *param / 4;
        d_gain = 256;
    }
    ret_value = Sensor_WriteReg(0x204, (uint32_t)a_gain); // 0x100);//a_gain);
    ret_value = Sensor_WriteReg(0x20e, (uint32_t)d_gain);
    ret_value = Sensor_WriteReg(0x210, (uint32_t)d_gain);
    ret_value = Sensor_WriteReg(0x212, (uint32_t)d_gain);
    ret_value = Sensor_WriteReg(0x214, (uint32_t)d_gain);

    //          SENSOR_LOGI("_s5k3l2xx: real_gain a_gain:0x%x read_reg
    //          0x204:0x%x, 0x20e:0x%x, 0x202:0x%x, 0x1088:0x%x",
    //          (uint32_t)a_gain,Sensor_ReadReg(0x0204),Sensor_ReadReg(0x020e),Sensor_ReadReg(0x202),Sensor_ReadReg(0x1086));
    /*
            SENSOR_LOGI("SENSOR_s5k4h8yx: write_gain:0x%x\n", *param);

            //ret_value = Sensor_WriteReg(0x104, 0x01);
            value = (*param)>>0x08;
            ret_value = Sensor_WriteReg(0x204, value);
            value = (*param)&0xff;
            ret_value = Sensor_WriteReg(0x205, value);
            ret_value = Sensor_WriteReg(0x104, 0x00);
    */
    return ret_value;
}

static uint32_t _s5k4h8yx_read_otp_gain(SENSOR_HW_HANDLE handle,
                                        uint32_t *param) {
    uint32_t rtn = SENSOR_SUCCESS;
    uint32_t gain = 0;
    uint16_t a_gain = 0;
    uint16_t d_gain = 0;
    a_gain = Sensor_ReadReg(0x0204);
    d_gain = Sensor_ReadReg(0x0210);
    *param = a_gain * d_gain;
#if 1 // for MP tool //!??
      /*	gain_h = Sensor_ReadReg(0x0204) & 0xff;
              gain_l = Sensor_ReadReg(0x0205) & 0xff;
              *param = ((gain_h << 8) | gain_l);
              gain_h = Sensor_ReadReg(0x020e) & 0xff;
              gain_l = Sensor_ReadReg(0x020f) & 0xff;
              *param *= (uint32_t)((gain_h << 8) | gain_l);
              *param >>= 8;*/
#else
    uint8_t cmd_val[3] = {0x00};
    uint16_t cmd_len;

    cmd_len = 2;
    cmd_val[0] = 0x02;
    cmd_val[1] = 0x04;
    rtn = Sensor_ReadI2C(S5K4H8YX_I2C_ADDR_W, (uint8_t *)&cmd_val[0], cmd_len);
    if (SENSOR_SUCCESS == rtn) {
        gain_h = (cmd_val[0]) & 0xff;
    }
    cmd_val[0] = 0x02;
    cmd_val[1] = 0x05;
    rtn = Sensor_ReadI2C(S5K4H8YX_I2C_ADDR_W, (uint8_t *)&cmd_val[0], cmd_len);
    if (SENSOR_SUCCESS == rtn) {
        gain_l = (cmd_val[0]) & 0xff;
    }
    *param = ((gain_h << 8) | gain_l);
//*param = s_set_gain;
#endif
    SENSOR_LOGI("SENSOR_s5k4h8yx: gain: %d", *param);

    return rtn;
}

static uint32_t _s5k4h8yx_write_vcm(SENSOR_HW_HANDLE handle, uint32_t *param) {
    uint32_t ret_value = SENSOR_SUCCESS;
    uint8_t cmd_val[2] = {0x00};
    uint16_t slave_addr = 0;
    uint16_t cmd_len = 0;

    slave_addr = DW9807_VCM_SLAVE_ADDR;

    cmd_len = 2;
    cmd_val[0] = ((*param) >> 16) & 0xff;
    cmd_val[1] = (*param) & 0xff;
    ret_value = Sensor_WriteI2C(slave_addr, (uint8_t *)&cmd_val[0], cmd_len);

    SENSOR_LOGI("SENSOR_s5k4h8yx: _write_vcm, ret =  %d, MSL:%x, LSL:%x\n",
                ret_value, cmd_val[0], cmd_val[1]);

    return ret_value;
}

static uint32_t _s5k4h8yx_read_vcm(SENSOR_HW_HANDLE handle, uint32_t *param) {
    uint32_t ret_value = SENSOR_SUCCESS;
    uint8_t cmd_val[2] = {0x00};
    uint16_t slave_addr = 0;
    uint16_t cmd_len = 0;

    slave_addr = DW9807_VCM_SLAVE_ADDR;

    cmd_len = 1;
    cmd_val[0] = ((*param) >> 16) & 0xff;
    // ret_value = (uint32_t)Sensor_ReadI2C(slave_addr,(cmr_u8*)&cmd_val[0],
    // cmd_len);
    if (SENSOR_SUCCESS == ret_value)
        *param |= cmd_val[0];

    SENSOR_LOGI("SENSOR_s5k4h8yx: _read_vcm, ret =  %d, MSL:%x, LSL:%x\n",
                ret_value, cmd_val[0], cmd_val[1]);

    return ret_value;
}

LOCAL uint32_t _s5k4h8yx_erase_otp(SENSOR_HW_HANDLE handle,
                                   unsigned long param) {
    uint32_t rtn = SENSOR_SUCCESS;
    uint8_t cmd_val[3] = {0x00};
    uint16_t cmd_len;

    SENSOR_LOGI("SENSOR_s5k4h8yx: _s5k4h8yx_erase_otp E");

    // EEPROM Erase
    cmd_len = 2;
    cmd_val[0] = 0x81;
    cmd_val[1] = 0xEE;
    Sensor_WriteI2C(DW9807_EEPROM_SLAVE_ADDR, (uint8_t *)&cmd_val[0], cmd_len);
    usleep(100 * 1000);

    SENSOR_LOGI("SENSOR_s5k4h8yx: _s5k4h8yx_erase_otp X");

    return rtn;
}

LOCAL uint32_t _s5k4h8yx_write_otp(SENSOR_HW_HANDLE handle,
                                   unsigned long param) {
    uint32_t rtn = SENSOR_SUCCESS;
    struct _sensor_otp_param_tag *param_ptr =
        (struct _sensor_otp_param_tag *)param;
    uint32_t start_addr = param_ptr->start_addr;
    uint32_t len = param_ptr->len;
    uint8_t *buff = param_ptr->buff;

    uint8_t cmd_val[3] = {0x00};
    uint16_t cmd_len;
    uint32_t i;

    SENSOR_LOGI("SENSOR_s5k4h8yx: _s5k4h8yx_write_otp E");

    // EEPROM Write
    cmd_len = 3;
    for (i = 0; i < len; i++) {
        cmd_val[0] = ((start_addr + i) >> 8) & 0xff;
        cmd_val[1] = (start_addr + i) & 0xff;
        cmd_val[2] = buff[i];
        Sensor_WriteI2C(DW9807_EEPROM_SLAVE_ADDR, (uint8_t *)&cmd_val[0],
                        cmd_len);
    }

    SENSOR_LOGI("SENSOR_s5k4h8yx: _s5k4h8yx_write_otp X");

    return rtn;
}

LOCAL uint32_t _s5k4h8yx_read_otp(SENSOR_HW_HANDLE handle,
                                  unsigned long param) {
#if 0
	uint32_t rtn                  = SENSOR_SUCCESS;
	struct _sensor_otp_param_tag* param_ptr = (struct _sensor_otp_param_tag *)param;
	uint32_t start_addr           = 0;
	uint32_t len                  = 0;
	uint8_t *buff                 = NULL;
	uint16_t tmp;
	uint8_t cmd_val[3]            = {0x00};
	uint16_t cmd_len;
	uint32_t i;

	if (NULL == param_ptr) {
		return -1;
	}

	SENSOR_LOGI("SENSOR_s5k4h8yx: _s5k4h8yx_read_otp E");

	cmd_len = 2;
	cmd_val[0] = 0x00;
	cmd_val[1] = 0x00;
	rtn = Sensor_WriteI2C(DW9807_EEPROM_SLAVE_ADDR,(uint8_t*)&cmd_val[0], cmd_len);
	usleep(1000);

	SENSOR_LOGI("write i2c rtn %d type %d", rtn, param_ptr->type);

	buff                 = param_ptr->buff;
	if (SENSOR_OTP_PARAM_NORMAL == param_ptr->type) {
		start_addr           = param_ptr->start_addr;
		len                  = 8192;
	} else if (SENSOR_OTP_PARAM_CHECKSUM == param_ptr->type) {
		start_addr           = 0x12FC;
		len                  = 4;
	} else if (SENSOR_OTP_PARAM_READBYTE == param_ptr->type) {
		start_addr           = param_ptr->start_addr;
		len                  = param_ptr->len;
	} else if (SENSOR_OTP_PARAM_FW_VERSION == param_ptr->type) {
		start_addr           = 0x30;
		len                  = 11;
	}

	// EEPROM READ
	for(i = 0;i < len; i++) {
		cmd_val[0] = ((start_addr + i) >> 8) & 0xff;
		cmd_val[1] = (start_addr + i) & 0xff;
		rtn = Sensor_ReadI2C(DW9807_EEPROM_SLAVE_ADDR,(uint8_t*)&cmd_val[0], cmd_len);
		if (SENSOR_SUCCESS == rtn) {
			buff[i] = (cmd_val[0]) & 0xff;
		}
		//SENSOR_LOGI("rtn %d value %d addr 0x%x", rtn, buff[i], start_addr + i);
	}
	param_ptr->len = len;

	if (NULL != param_ptr->awb.data_ptr && SENSOR_OTP_PARAM_NORMAL == param_ptr->type) {
		uint32_t awb_src_addr = 0x900;
		uint32_t lsc_src_addr = 0xA00;
		uint32_t real_size = 0;
		rtn = s5k4h8_awb_package_convert((void *)(buff + awb_src_addr), param_ptr->awb.data_ptr,
			param_ptr->awb.size, &real_size);
		param_ptr->awb.size = real_size;
		SENSOR_LOGI("SENSOR_s5k4h8yx: awb real_size %d", real_size);
		rtn = s5k4h8_lsc_package_convert((void *)(buff + lsc_src_addr), param_ptr->lsc.data_ptr,
			param_ptr->lsc.size, &real_size);
		param_ptr->lsc.size = real_size;
		SENSOR_LOGI("SENSOR_s5k4h8yx: lsc real_size %d", real_size);
	}
	SENSOR_LOGI("SENSOR_s5k4h8yx: _s5k4h8yx_read_otp X");
	return rtn;
#else
    return 0;
#endif
}

LOCAL uint32_t _s5k4h8yx_parse_otp(SENSOR_HW_HANDLE handle,
                                   unsigned long param) {
    uint32_t rtn = SENSOR_SUCCESS;
    struct _sensor_otp_param_tag *param_ptr =
        (struct _sensor_otp_param_tag *)param;
    uint8_t *buff = NULL;
    uint32_t awb_src_addr = 0x900;
    uint32_t lsc_src_addr = 0xA00;
    uint32_t real_size = 0;

    if (NULL == param_ptr) {
        return -1;
    }
#if 0
	SENSOR_LOGI("SENSOR_s5k4h8yx: _s5k4h8yx_parse_otp E");

	buff                 = param_ptr->buff;

	if (NULL != param_ptr->awb.data_ptr) {
		rtn = s5k4h8_awb_package_convert((void *)(buff + awb_src_addr), param_ptr->awb.data_ptr,
			param_ptr->awb.size, &real_size);
		param_ptr->awb.size = real_size;
		SENSOR_LOGI("SENSOR_s5k4h8yx: awb real_size %d", real_size);
	}

	if (NULL != param_ptr->lsc.data_ptr) {
		rtn = s5k4h8_lsc_package_convert((void *)(buff + lsc_src_addr), param_ptr->lsc.data_ptr,
			param_ptr->lsc.size, &real_size);
		param_ptr->lsc.size = real_size;
		SENSOR_LOGI("SENSOR_s5k4h8yx: lsc real_size %d", real_size);
	}

	SENSOR_LOGI("SENSOR_s5k4h8yx: _s5k4h8yx_parse_otp X");
#endif
    return rtn;
}

static uint32_t _s5k4h8yx_get_golden_data(SENSOR_HW_HANDLE handle,
                                          unsigned long param) {
    uint32_t rtn = SENSOR_SUCCESS;
    struct _sensor_otp_param_tag *param_ptr =
        (struct _sensor_otp_param_tag *)param;
#if 0
	param_ptr->golden.data_ptr = (void*)s5k4h8_golden_data;
	param_ptr->golden.size = sizeof(s5k4h8_golden_data);

	SENSOR_LOGI("SENSOR_s5k4h8yx: golden: %d", param_ptr->golden.size);
#endif

    return rtn;
}

static uint32_t _s5k4h8yx_get_fps_info(SENSOR_HW_HANDLE handle,
                                       uint32_t *param) {
    uint32_t rtn = SENSOR_SUCCESS;
    SENSOR_MODE_FPS_T *fps_info;
    // make sure have inited fps of every sensor mode.
    if (!s_s5k4h8yx_mode_fps_info.is_init) {
        _s5k4h8yx_init_mode_fps_info(handle);
    }
    fps_info = (SENSOR_MODE_FPS_T *)param;
    uint32_t sensor_mode = fps_info->mode;
    fps_info->max_fps =
        s_s5k4h8yx_mode_fps_info.sensor_mode_fps[sensor_mode].max_fps;
    fps_info->min_fps =
        s_s5k4h8yx_mode_fps_info.sensor_mode_fps[sensor_mode].min_fps;
    fps_info->is_high_fps =
        s_s5k4h8yx_mode_fps_info.sensor_mode_fps[sensor_mode].is_high_fps;
    fps_info->high_fps_skip_num =
        s_s5k4h8yx_mode_fps_info.sensor_mode_fps[sensor_mode].high_fps_skip_num;
    SENSOR_LOGI("SENSOR_s5k4h8yx: mode %d, max_fps: %d", fps_info->mode,
                fps_info->max_fps);
    SENSOR_LOGI("SENSOR_s5k4h8yx: min_fps: %d", fps_info->min_fps);
    SENSOR_LOGI("SENSOR_s5k4h8yx: is_high_fps: %d", fps_info->is_high_fps);
    SENSOR_LOGI("SENSOR_s5k4h8yx: high_fps_skip_num: %d",
                fps_info->high_fps_skip_num);

    return rtn;
}

static uint32_t _s5k4h8yx_get_static_info(SENSOR_HW_HANDLE handle,
                                          uint32_t *param) {
    uint32_t rtn = SENSOR_SUCCESS;
    struct sensor_ex_info *ex_info;
    ex_info = (struct sensor_ex_info *)param;
    ex_info->f_num = s_s5k4h8yx_static_info.f_num;
    ex_info->focal_length = s_s5k4h8yx_static_info.focal_length;
    ex_info->max_fps = s_s5k4h8yx_static_info.max_fps;
    ex_info->max_adgain = s_s5k4h8yx_static_info.max_adgain;
    ex_info->ois_supported = s_s5k4h8yx_static_info.ois_supported;
    ex_info->pdaf_supported = s_s5k4h8yx_static_info.pdaf_supported;
    ex_info->exp_valid_frame_num = s_s5k4h8yx_static_info.exp_valid_frame_num;
    ex_info->clamp_level = s_s5k4h8yx_static_info.clamp_level;
    ex_info->adgain_valid_frame_num =
        s_s5k4h8yx_static_info.adgain_valid_frame_num;
    ex_info->preview_skip_num = g_s5k4h8yx_mipi_raw_info.preview_skip_num;
    ex_info->capture_skip_num = g_s5k4h8yx_mipi_raw_info.capture_skip_num;
    ex_info->name = (cmr_s8 *)g_s5k4h8yx_mipi_raw_info.name;
    ex_info->sensor_version_info =
        (cmr_s8 *)g_s5k4h8yx_mipi_raw_info.sensor_version_info;
    SENSOR_LOGI("SENSOR_s5k4h8yx: f_num: %d", ex_info->f_num);
    SENSOR_LOGI("SENSOR_s5k4h8yx: focal_length: %d", ex_info->focal_length);
    SENSOR_LOGI("SENSOR_s5k4h8yx: max_fps: %d", ex_info->max_fps);
    SENSOR_LOGI("SENSOR_s5k4h8yx: max_adgain: %d", ex_info->max_adgain);
    SENSOR_LOGI("SENSOR_s5k4h8yx: ois_supported: %d", ex_info->ois_supported);
    SENSOR_LOGI("SENSOR_s5k4h8yx: pdaf_supported: %d", ex_info->pdaf_supported);
    SENSOR_LOGI("SENSOR_s5k4h8yx: exp_valid_frame_num: %d",
                ex_info->exp_valid_frame_num);
    SENSOR_LOGI("SENSOR_s5k4h8yx: clam_level: %d", ex_info->clamp_level);
    SENSOR_LOGI("SENSOR_s5k4h8yx: adgain_valid_frame_num: %d",
                ex_info->adgain_valid_frame_num);
    SENSOR_LOGI("SENSOR_s5k4h8yx: sensor name is: %s", ex_info->name);
    SENSOR_LOGI("SENSOR_s5k4h8yx: sensor version info is: %s",
                ex_info->sensor_version_info);

    return rtn;
}

static uint32_t _s5k4h8yx_set_sensor_close_flag(SENSOR_HW_HANDLE handle) {
    uint32_t rtn = SENSOR_SUCCESS;

    s_s5k4h8yx_sensor_close_flag = 1;

    return rtn;
}

static unsigned long _s5k4h8yx_access_val(SENSOR_HW_HANDLE handle,
                                          unsigned long param) {
    uint32_t rtn = SENSOR_SUCCESS;
    SENSOR_VAL_T *param_ptr = (SENSOR_VAL_T *)param;
    uint16_t tmp;

    SENSOR_LOGI("SENSOR_s5k4h8yx: _s5k4h8yx_access_val E");
    if (!param_ptr) {
        return rtn;
    }

    SENSOR_LOGI("SENSOR_s5k4h8yx: param_ptr->type=%x", param_ptr->type);
    switch (param_ptr->type) {
    case SENSOR_VAL_TYPE_INIT_OTP:
        // rtn = _s5k4h8yx_init_otp(handle);
        break;
    case SENSOR_VAL_TYPE_SHUTTER:
        *((uint32_t *)param_ptr->pval) = _s5k4h8yx_get_shutter(handle);
        break;
    case SENSOR_VAL_TYPE_READ_VCM:
        rtn = _s5k4h8yx_read_vcm(handle, param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_WRITE_VCM:
        rtn = _s5k4h8yx_write_vcm(handle, param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_READ_OTP:
        // rtn = _s5k4h8yx_read_otp(handle, (unsigned long)param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_PARSE_OTP:
        // rtn = _s5k4h8yx_parse_otp(handle, (unsigned long)param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_WRITE_OTP:
        // rtn = _s5k4h8yx_write_otp(handle, (unsigned long)param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_ERASE_OTP:
        // rtn = _s5k4h8yx_erase_otp(handle, (unsigned long)param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_GET_RELOADINFO: {
        //				struct isp_calibration_info **p= (struct
        // isp_calibration_info **)param_ptr->pval;
        //				*p=&calibration_info;
    } break;
    case SENSOR_VAL_TYPE_GET_AFPOSITION:
        *(uint32_t *)param_ptr->pval = 0; // cur_af_pos;
        break;
    case SENSOR_VAL_TYPE_WRITE_OTP_GAIN:
        rtn = _s5k4h8yx_write_otp_gain(handle, param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_READ_OTP_GAIN:
        rtn = _s5k4h8yx_read_otp_gain(handle, param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_GET_GOLDEN_DATA:
        rtn = _s5k4h8yx_get_golden_data(handle, (unsigned long)param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_GET_GOLDEN_LSC_DATA:
        //*(uint32_t*)param_ptr->pval = s5k4h8_lsc_golden_data;
        break;
    case SENSOR_VAL_TYPE_GET_STATIC_INFO:
        rtn = _s5k4h8yx_get_static_info(handle, param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_GET_FPS_INFO:
        rtn = _s5k4h8yx_get_fps_info(handle, param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_SET_SENSOR_CLOSE_FLAG:
        rtn = _s5k4h8yx_set_sensor_close_flag(handle);
        break;
    default:
        break;
    }

    SENSOR_LOGI("SENSOR_s5k4h8yx: _s5k4h8yx_access_val X");

    return rtn;
}
