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
#include "sensor.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"
//#include "sensor_s5k3p8sm_raw_param.c"
#include "isp_param_file_update.h"
#if defined(CONFIG_CAMERA_OIS_FUNC)
#include "OIS_main.h"
#else
#include "af_bu64297gwz.h"
#endif
#include "parameters/sensor_s5k3p8sm_raw_param_v3.c"

#define S5K3P8SM_I2C_ADDR_W 0x10
#define S5K3P8SM_I2C_ADDR_R 0x10
//#define S5K3P8SM_I2C_ADDR_W        0x2d
//#define S5K3P8SM_I2C_ADDR_R        0x2d
#define S5K3P8SM_RAW_PARAM_COM 0x0000

#ifndef RAW_INFO_END_ID
#define RAW_INFO_END_ID 0x71717567
#endif

#define s5k3p8sm_i2c_read_otp(addr) s5k3p8sm_i2c_read_otp_set(handle, addr)

static uint32_t g_module_id = 0;
static struct sensor_raw_info *s_s5k3p8sm_mipi_raw_info_ptr =
    &s_s5k3p8sm_mipi_raw_info;
static int s_capture_shutter = 0;
static int s_exposure_time = 0;
static uint32_t s_s5k3p8sm_sensor_close_flag = 0;

struct sensor_ev_info_t s_sensor_ev_info;

SENSOR_HW_HANDLE _s5k3p8sm_Create(void *privatedata);
void _s5k3p8sm_Destroy(SENSOR_HW_HANDLE handle);
static unsigned long _s5k3p8sm_GetResolutionTrimTab(SENSOR_HW_HANDLE handle,
                                                    unsigned long param);
static unsigned long _s5k3p8sm_Identify(SENSOR_HW_HANDLE handle,
                                        unsigned long param);
static uint32_t _s5k3p8sm_GetRawInof(SENSOR_HW_HANDLE handle);
static unsigned long _s5k3p8sm_StreamOn(SENSOR_HW_HANDLE handle,
                                        unsigned long param);
static unsigned long _s5k3p8sm_StreamOff(SENSOR_HW_HANDLE handle,
                                         unsigned long param);
static uint32_t _s5k3p8sm_com_Identify_otp(SENSOR_HW_HANDLE handle,
                                           void *param_ptr);
static unsigned long _s5k3p8sm_PowerOn(SENSOR_HW_HANDLE handle,
                                       unsigned long power_on);
static unsigned long _s5k3p8sm_write_exposure(SENSOR_HW_HANDLE handle,
                                              unsigned long param);
static unsigned long _s5k3p8sm_write_gain(SENSOR_HW_HANDLE handle,
                                          unsigned long param);
static unsigned long _s5k3p8sm_access_val(SENSOR_HW_HANDLE handle,
                                          unsigned long param);
static unsigned long s5k3p8sm_write_af(SENSOR_HW_HANDLE handle,
                                       unsigned long param);
static unsigned long _s5k3p8sm_GetExifInfo(SENSOR_HW_HANDLE handle,
                                           unsigned long param);
static unsigned long _s5k3p8sm_BeforeSnapshot(SENSOR_HW_HANDLE handle,
                                              unsigned long param);
static uint16_t _s5k3p8sm_get_shutter(SENSOR_HW_HANDLE handle);
static unsigned long _s5k3p8sm_ex_write_exposure(SENSOR_HW_HANDLE handle,
                                                 unsigned long param);

static const struct raw_param_info_tab s_s5k3p8sm_raw_param_tab[] = {
    {S5K3P8SM_RAW_PARAM_COM, NULL, _s5k3p8sm_com_Identify_otp, NULL},
    {RAW_INFO_END_ID, PNULL, PNULL, PNULL}};

static const SENSOR_REG_T s5k3p8sm_common_init[] = {
    {0x6028, 0x4000},
    {0x6010, 0x0001}, // Reset
    {0xffff, 0x000a}, // must add delay >3ms
    {0x6214, 0x7971},
    {0x6218, 0x7150},
    {0x6028, 0x2000}, // TP
    {0x602A, 0x3074},
    {0x6F12, 0x0000},
    {0x6F12, 0x0000},
    {0x6F12, 0x0000},
    {0x6F12, 0x0000},
    {0x6F12, 0x0000},
    {0x6F12, 0x0000},
    {0x6F12, 0x0449},
    {0x6F12, 0x0348},
    {0x6F12, 0x0860},
    {0x6F12, 0xCA8D},
    {0x6F12, 0x101A},
    {0x6F12, 0x8880},
    {0x6F12, 0x00F0},
    {0x6F12, 0x52B8},
    {0x6F12, 0x2000},
    {0x6F12, 0x31B8},
    {0x6F12, 0x2000},
    {0x6F12, 0x1E80},
    {0x6F12, 0x0000},
    {0x6F12, 0x0000},
    {0x6F12, 0x0000},
    {0x6F12, 0x0000},
    {0x6F12, 0x70B5},
    {0x6F12, 0x0646},
    {0x6F12, 0x2C48},
    {0x6F12, 0x0022},
    {0x6F12, 0x0168},
    {0x6F12, 0x0C0C},
    {0x6F12, 0x8DB2},
    {0x6F12, 0x2946},
    {0x6F12, 0x2046},
    {0x6F12, 0x00F0},
    {0x6F12, 0x61F8},
    {0x6F12, 0x3046},
    {0x6F12, 0x00F0},
    {0x6F12, 0x63F8},
    {0x6F12, 0x2748},
    {0x6F12, 0x284A},
    {0x6F12, 0x0188},
    {0x6F12, 0x1180},
    {0x6F12, 0x911C},
    {0x6F12, 0x4088},
    {0x6F12, 0x0880},
    {0x6F12, 0x2946},
    {0x6F12, 0x2046},
    {0x6F12, 0xBDE8},
    {0x6F12, 0x7040},
    {0x6F12, 0x0122},
    {0x6F12, 0x00F0},
    {0x6F12, 0x50B8},
    {0x6F12, 0x2DE9},
    {0x6F12, 0xF041},
    {0x6F12, 0x8046},
    {0x6F12, 0x1E48},
    {0x6F12, 0x1446},
    {0x6F12, 0x0F46},
    {0x6F12, 0x4068},
    {0x6F12, 0x0022},
    {0x6F12, 0x85B2},
    {0x6F12, 0x060C},
    {0x6F12, 0x2946},
    {0x6F12, 0x3046},
    {0x6F12, 0x00F0},
    {0x6F12, 0x42F8},
    {0x6F12, 0x1B48},
    {0x6F12, 0x408C},
    {0x6F12, 0x70B1},
    {0x6F12, 0x94F8},
    {0x6F12, 0x6B00},
    {0x6F12, 0x4021},
    {0x6F12, 0xB1FB},
    {0x6F12, 0xF0F0},
    {0x6F12, 0x1849},
    {0x6F12, 0x194A},
    {0x6F12, 0x098B},
    {0x6F12, 0x5288},
    {0x6F12, 0x891A},
    {0x6F12, 0x0901},
    {0x6F12, 0x91FB},
    {0x6F12, 0xF0F0},
    {0x6F12, 0x84B2},
    {0x6F12, 0x05E0},
    {0x6F12, 0x2246},
    {0x6F12, 0x3946},
    {0x6F12, 0x4046},
    {0x6F12, 0x00F0},
    {0x6F12, 0x35F8},
    {0x6F12, 0x0446},
    {0x6F12, 0x0122},
    {0x6F12, 0x2946},
    {0x6F12, 0x3046},
    {0x6F12, 0x00F0},
    {0x6F12, 0x25F8},
    {0x6F12, 0x2046},
    {0x6F12, 0xBDE8},
    {0x6F12, 0xF081},
    {0x6F12, 0x10B5},
    {0x6F12, 0x0022},
    {0x6F12, 0xAFF2},
    {0x6F12, 0x9B01},
    {0x6F12, 0x0C48},
    {0x6F12, 0x00F0},
    {0x6F12, 0x2AF8},
    {0x6F12, 0x054C},
    {0x6F12, 0x0022},
    {0x6F12, 0xAFF2},
    {0x6F12, 0x6F01},
    {0x6F12, 0x2060},
    {0x6F12, 0x0948},
    {0x6F12, 0x00F0},
    {0x6F12, 0x22F8},
    {0x6F12, 0x6060},
    {0x6F12, 0x10BD},
    {0x6F12, 0x0000},
    {0x6F12, 0x2000},
    {0x6F12, 0x31B0},
    {0x6F12, 0x2000},
    {0x6F12, 0x4A00},
    {0x6F12, 0x4000},
    {0x6F12, 0x950C},
    {0x6F12, 0x2000},
    {0x6F12, 0x2F10},
    {0x6F12, 0x2000},
    {0x6F12, 0x1B10},
    {0x6F12, 0x2000},
    {0x6F12, 0x2F40},
    {0x6F12, 0x0000},
    {0x6F12, 0x15E9},
    {0x6F12, 0x0000},
    {0x6F12, 0x9ECD},
    {0x6F12, 0x40F2},
    {0x6F12, 0xA37C},
    {0x6F12, 0xC0F2},
    {0x6F12, 0x000C},
    {0x6F12, 0x6047},
    {0x6F12, 0x41F2},
    {0x6F12, 0xE95C},
    {0x6F12, 0xC0F2},
    {0x6F12, 0x000C},
    {0x6F12, 0x6047},
    {0x6F12, 0x49F6},
    {0x6F12, 0xCD6C},
    {0x6F12, 0xC0F2},
    {0x6F12, 0x000C},
    {0x6F12, 0x6047},
    {0x6F12, 0x4DF6},
    {0x6F12, 0x1B0C},
    {0x6F12, 0xC0F2},
    {0x6F12, 0x000C},
    {0x6F12, 0x6047},
    {0x6F12, 0x3108},
    {0x6F12, 0x021C},
    {0x6F12, 0x0000},
    {0x6F12, 0x0005},
    {0x602A, 0x4A00},
    {0x6F12, 0x0088},
    {0x6F12, 0x0D70},
    {0x6028, 0x4000},
    {0x0202, 0x0006}, // 0x0100 },
    //{ 0x0204, 0x0140 },
    {0x6F12, 0x0D70},
    {0x0200, 0x0618},
    {0x021E, 0x0300},
    {0x021C, 0x0000},
    {0x6028, 0x2000},
    {0x602A, 0x162C},
    {0x6F12, 0x8080},
    {0x602A, 0x164C},
    {0x6F12, 0x5555},
    {0x6F12, 0x5500},
    {0x602A, 0x166C},
    {0x6F12, 0x4040},
    {0x6F12, 0x4040},
    {0x6028, 0x4000},
    {0x3606, 0x0103},
    {0xF496, 0x004D},
    {0xF470, 0x0008},
    {0xF43A, 0x0015},
    {0xF484, 0x0006},
    {0xF442, 0x44C6},
    {0xF408, 0xFFF7},
    {0xF494, 0x1010},
    {0xF4D4, 0x0038},
    {0xF4D6, 0x0039},
    {0xF4D8, 0x0034},
    {0xF4DA, 0x0035},
    {0xF4DC, 0x0038},
    {0xF4DE, 0x0039},
    {0xF47C, 0x001F},
    {0xF62E, 0x00C5},
    {0xF630, 0x00CD},
    {0xF632, 0x00DD},
    {0xF634, 0x00E5},
    {0xF636, 0x00F5},
    {0xF638, 0x00FD},
    {0xF63A, 0x010D},
    {0xF63C, 0x0115},
    {0xF63E, 0x0125},
    {0xF640, 0x012D},
    {0x3070, 0x0100},
    {0x3090, 0x0904},
    {0x31C0, 0x00C8},
    {0x6028, 0x2000},
    {0x602A, 0x18F0},
    {0x6F12, 0x0100},
    {0x602A, 0x18F6},
    {0x6F12, 0x002F},
    {0x6F12, 0x002F},
    {0x6F12, 0xF440},
};

// 4632x3480 30FPS v560M mipi1392M 4lane
// frame length	0x0E2A
// 1H time	91.9

// 4640x3488 30FPS Mclk24 v560M mipi1464Mbps/lane 4lane
// frame length	0x0E3B
// 1H time	91.4
static const SENSOR_REG_T s5k3p8sm_4640x3488_4lane_setting[] = {
    {0x6028, 0x4000}, {0x0136, 0x1800}, {0x0304, 0x0006}, {0x0306, 0x0069},
    {0x0302, 0x0001}, {0x0300, 0x0003}, {0x030C, 0x0004}, {0x030E, 0x007A},
    {0x030A, 0x0001}, {0x0308, 0x0008}, {0x0344, 0x0008}, {0x0346, 0x0008},
    {0x0348, 0x1227}, {0x034A, 0x0DA7}, {0x034C, 0x1220}, {0x034E, 0x0DA0},
    {0x0408, 0x0000}, {0x0900, 0x0011}, {0x0380, 0x0001}, {0x0382, 0x0001},
    {0x0384, 0x0001}, {0x0386, 0x0001}, {0x0400, 0x0000}, {0x0404, 0x0010},
    {0x0342, 0x1400}, {0x0340, 0x0E3B}, {0x0B0E, 0x0000}, {0x0216, 0x0000},
    {0x3604, 0x0002}, {0x3664, 0x0019}, {0x3004, 0x0003}, {0x3000, 0x0001},
    {0x317A, 0x0130}, {0x1006, 0x0002}, {0x6028, 0x2000}, {0x602A, 0x19E0},
    {0x6F12, 0x0001}, //type3
    //{0x6F12, 0x0000}, //type2
    {0x602A, 0x18F6}, {0x6F12, 0x002F}, {0x6F12, 0x002F},
    {0x31A4, 0x0102}, {0x0100, 0x0000}, // steam off
    //{ 0x0100, 0x0100 },//streaming on
};

// 2320x1744 30FPS Mclk24 v560M mipi732Mbps/lane 4lane
// frame length	0x3643
// 1H time	91.4
static const SENSOR_REG_T s5k3p8sm_2320x1744_4lane_setting[] = {
    {0x6028, 0x4000}, {0x0136, 0x1800}, {0x0304, 0x0006}, {0x0306, 0x0069},
    {0x0302, 0x0001}, {0x0300, 0x0003}, {0x030C, 0x0004}, {0x030E, 0x003D},
    {0x030A, 0x0001}, {0x0308, 0x0008}, {0x0344, 0x0008}, {0x0346, 0x0008},
    {0x0348, 0x1227}, {0x034A, 0x0DA7}, {0x034C, 0x0910}, {0x034E, 0x06D0},
    {0x0408, 0x0000}, {0x0900, 0x0112}, {0x0380, 0x0001}, {0x0382, 0x0001},
    {0x0384, 0x0001}, {0x0386, 0x0003}, {0x0400, 0x0001}, {0x0404, 0x0020},
    {0x0342, 0x1400}, {0x0340, 0x0E3B}, {0x0B0E, 0x0100}, {0x0216, 0x0000},
    {0x3604, 0x0002}, {0x3664, 0x0019}, {0x3004, 0x0003}, {0x3000, 0x0001},
    {0x317A, 0x00A0}, {0x1006, 0x0002}, {0x6028, 0x2000}, {0x602A, 0x19E0},
    {0x6F12, 0x0001}, {0x602A, 0x2F32}, {0x6F12, 0x0001}, {0x602A, 0x18F6},
    {0x6F12, 0x002F}, {0x6F12, 0x002F}, {0x6028, 0x4000}, {0x31A4, 0x0102},

    //{ 0x0100, 0x0100 },//streaming on
};

/* 2320 x 1748 30fps 720Mbps */
static const SENSOR_REG_T s5k3p8sm_2320x1748_4lane_setting[] = {
#if 1

#else
    {0x6028, 0x4000}, {0x0344, 0x0000}, {0x0346, 0x0000}, {0x0348, 0x090F},
    {0x034A, 0x06D3}, {0x034C, 0x0910}, {0x034E, 0x06D4}, {0x3002, 0x0001},
    {0x0136, 0x1800}, {0x0304, 0x0006}, {0x0306, 0x008C}, {0x0302, 0x0001},
    {0x0300, 0x0008}, {0x030C, 0x0004}, {0x030E, 0x0078}, {0x030A, 0x0001},
    {0x0308, 0x0008}, {0x3008, 0x0001}, {0x0800, 0x0000}, {0x0200, 0x0200},
    {0x0202, 0x0100}, {0x021C, 0x0200}, {0x021E, 0x0100}, {0x0342, 0x141C},
    {0x0340, 0x0708}, {0x3072, 0x03C0},
#endif
};

// 1920X1080 60FPS v560M mipi660M 4lane 2x2binning
// frame length	0x06E7
// 1H time	94.3
static const SENSOR_REG_T s5k3p8sm_1920x1080_4lane_setting[] = {
    {0x6028, 0x2000}, {0x602A, 0x2E26}, {0x6F12, 0x0103}, {0x6028, 0x4000},
    {0x3D7C, 0x0010}, {0x3D88, 0x0064}, {0x3D8A, 0x0068}, {0x0344, 0x018C},
    {0x0346, 0x029C}, {0x0348, 0x109B}, {0x034A, 0x0B0B}, {0x034C, 0x0780},
    {0x034E, 0x0438}, {0x0408, 0x0004}, {0x0900, 0x0122}, {0x0380, 0x0001},
    {0x0382, 0x0003}, {0x0384, 0x0001}, {0x0386, 0x0003}, {0x0400, 0x0001},
    {0x0404, 0x0010}, {0x0114, 0x0300}, {0x0110, 0x0002}, {0x0136, 0x1800},
    {0x0304, 0x0006}, {0x0306, 0x008C}, {0x0302, 0x0001}, {0x0300, 0x0004},
    {0x030C, 0x0004}, {0x030E, 0x0037}, {0x030A, 0x0001}, {0x0308, 0x0008},
    {0x0342, 0x14A2}, {0x0340, 0x06E7}, {0x0B0E, 0x0100}, {0x0216, 0x0000},
    // { 0x0100, 0x0000 }, //steam off
    // { 0x0100, 0x0100 },//steam on
};

// 1920X1080 30FPS v560M mipi660M 4lane 2x2binning
// frame length	0x0DCE
// 1H time	94.3
static const SENSOR_REG_T s5k3p8sm_1920x1080_4lane_30fps_setting[] = {
    {0x6028, 0x2000}, {0x602A, 0x2E26}, {0x6F12, 0x0103}, {0x6028, 0x4000},
    {0x3D7C, 0x0010}, {0x3D88, 0x0064}, {0x3D8A, 0x0068}, {0x0344, 0x018C},
    {0x0346, 0x029C}, {0x0348, 0x109B}, {0x034A, 0x0B0B}, {0x034C, 0x0780},
    {0x034E, 0x0438}, {0x0408, 0x0004}, {0x0900, 0x0122}, {0x0380, 0x0001},
    {0x0382, 0x0003}, {0x0384, 0x0001}, {0x0386, 0x0003}, {0x0400, 0x0001},
    {0x0404, 0x0010}, {0x0114, 0x0300}, {0x0110, 0x0002}, {0x0136, 0x1800},
    {0x0304, 0x0006}, {0x0306, 0x008C}, {0x0302, 0x0001}, {0x0300, 0x0004},
    {0x030C, 0x0004}, {0x030E, 0x0037}, {0x030A, 0x0001}, {0x0308, 0x0008},
    {0x0342, 0x14A2}, {0x0340, 0x0DCE}, {0x0B0E, 0x0100}, {0x0216, 0x0000},
    //{0x0100,0x0100	}, //steam on
};

// 1280x720 120FPS v560M mipi660M 4lane 3x3binning
// frame length	0x038A
// 1H time	91.9

// 1280x720 120FPS Mclk24 v560M mipi1464Mbps/lane 4lane
// frame length	0x03C5
// 1H time	86.3
static const SENSOR_REG_T s5k3p8sm_1280x720_4lane_setting[] = {
    {0x6028, 0x4000}, {0x0136, 0x1800}, {0x0304, 0x0006}, {0x0306, 0x008C},
    {0x0302, 0x0001}, {0x0300, 0x0004}, {0x030C, 0x0004}, {0x030E, 0x007A},
    {0x030A, 0x0001}, {0x0308, 0x0008}, {0x0344, 0x0198}, {0x0346, 0x02A0},
    {0x0348, 0x1097}, {0x034A, 0x0B0F}, {0x034C, 0x0500}, {0x034E, 0x02D0},
    {0x0408, 0x0000}, {0x0900, 0x0113}, {0x0380, 0x0001}, {0x0382, 0x0001},
    {0x0384, 0x0001}, {0x0386, 0x0005}, {0x0400, 0x0001}, {0x0404, 0x0030},
    {0x0342, 0x12E0}, {0x0340, 0x03C5}, {0x0B0E, 0x0100}, {0x0216, 0x0000},
    {0x3604, 0x0001}, {0x3664, 0x0011}, {0x3004, 0x0004}, {0x3000, 0x0000},
    {0x317A, 0x0007}, {0x1006, 0x0003}, {0x6028, 0x2000}, {0x602A, 0x19E0},
    {0x6F12, 0x0001}, {0x602A, 0x18F6}, {0x6F12, 0x00AF}, {0x6F12, 0x00AF},
    {0x31A4, 0x0102}, {0x0100, 0x0000}, // steam off
    //{ 0x0100, 0x0100 },//streaming on
};

static SENSOR_REG_TAB_INFO_T s_s5k3p8sm_resolution_Tab_RAW[9] = {
    {ADDR_AND_LEN_OF_ARRAY(s5k3p8sm_common_init), 0, 0, 24,
     SENSOR_IMAGE_FORMAT_RAW},
    {ADDR_AND_LEN_OF_ARRAY(s5k3p8sm_1280x720_4lane_setting), 1280, 720, 24,
     SENSOR_IMAGE_FORMAT_RAW},
    {ADDR_AND_LEN_OF_ARRAY(s5k3p8sm_2320x1744_4lane_setting), 2320, 1744, 24,
     SENSOR_IMAGE_FORMAT_RAW},
    {ADDR_AND_LEN_OF_ARRAY(s5k3p8sm_4640x3488_4lane_setting), 4640, 3488, 24,
     SENSOR_IMAGE_FORMAT_RAW},
    //{ADDR_AND_LEN_OF_ARRAY(s5k3p8sm_1920x1080_4lane_30fps_setting), 1920,
    // 1080, 24, SENSOR_IMAGE_FORMAT_RAW},
    //{ADDR_AND_LEN_OF_ARRAY(s5k3p8sm_1920x1080_4lane_setting), 1920, 1080, 24,
    // SENSOR_IMAGE_FORMAT_RAW},
    //{ADDR_AND_LEN_OF_ARRAY(s5k3p8sm_2320x1748_4lane_setting), 2320, 1748, 24,
    // SENSOR_IMAGE_FORMAT_RAW},
    //{ADDR_AND_LEN_OF_ARRAY(s5k3p8sm_2304x1740_4lane_setting), 2304, 1740, 24,
    // SENSOR_IMAGE_FORMAT_RAW},
};

static SENSOR_TRIM_T s_s5k3p8sm_Resolution_Trim_Tab[SENSOR_MODE_MAX] = {
    {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
    {0, 0, 1280, 720, 9248, 1320, 965, {0, 0, 1280, 720}},
    {0, 0, 2320, 1744, 9140, 1320, 3643, {0, 0, 2320, 1744}},
    {0, 0, 4640, 3488, 9140, 2784, 3643, {0, 0, 4640, 3488}},
    //{0, 0, 1920, 1080, 9430, 1320, 3534, {0, 0, 1920, 1080}},
    //{0, 0, 1920, 1080, 9430, 1320, 1767, {0, 0, 1920, 1080}},
    //{0, 0, 2320, 1748, 18300, 1440, 1800, {0, 0, 2320, 1748}},
    //{0, 0, 2304, 1740, 9430, 1320, 0x0dc8, {0, 0, 2304, 1740}},
};

static const SENSOR_REG_T
    s_s5k3p8sm_2576x1932_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
        /*video mode 0: ?fps*/
        {{0xffff, 0xff}},
        /* video mode 1:?fps*/
        {{0xffff, 0xff}},
        /* video mode 2:?fps*/
        {{0xffff, 0xff}},
        /* video mode 3:?fps*/
        {{0xffff, 0xff}}};

static SENSOR_VIDEO_INFO_T s_s5k3p8sm_video_info[] = {
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
    {{{30, 30, 164, 100}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
     (SENSOR_REG_T **)s_s5k3p8sm_2576x1932_video_tab},
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL}};

static SENSOR_IOCTL_FUNC_TAB_T s_s5k3p8sm_ioctl_func_tab = {
    PNULL, _s5k3p8sm_PowerOn, PNULL, _s5k3p8sm_Identify,

    PNULL, // write register
    PNULL, // read  register
    PNULL, _s5k3p8sm_GetResolutionTrimTab,

    // External
    PNULL, PNULL, PNULL,

    PNULL, //_s5k3p8sm_set_brightness,
    PNULL, // _s5k3p8sm_set_contrast,
    PNULL,
    PNULL, //_s5k3p8sm_set_saturation,

    PNULL, //_s5k3p8sm_set_work_mode,
    PNULL, //_s5k3p8sm_set_image_effect,

    _s5k3p8sm_BeforeSnapshot,
    PNULL, //_s5k3p8sm_after_snapshot,
    PNULL, //_s5k3p8sm_flash,
    PNULL, // read_ae_value
    _s5k3p8sm_write_exposure,
    PNULL, // read_gain_value
    _s5k3p8sm_write_gain,
    PNULL, // read_gain_scale
    PNULL, // set_frame_rate
    PNULL, // s5k3p8sm_write_af, // s5k3p8sm_write_af,
    PNULL,
    PNULL, //_s5k3p8sm_set_awb,
    PNULL, PNULL,
    PNULL, //_s5k3p8sm_set_ev,
    PNULL, PNULL, PNULL, _s5k3p8sm_GetExifInfo,
    PNULL, //_s5k3p8sm_ExtFunc,
    PNULL, //_s5k3p8sm_set_anti_flicker,
    PNULL, //_s5k3p8sm_set_video_mode,
    PNULL, // pick_jpeg_stream
    PNULL, // meter_mode
    PNULL, // get_status
    _s5k3p8sm_StreamOn, _s5k3p8sm_StreamOff,
    _s5k3p8sm_access_val, // s5k3p8sm_cfg_otp,
    _s5k3p8sm_ex_write_exposure, PNULL};

static SENSOR_STATIC_INFO_T s_s5k3p8sm_static_info = {
    200, // f-number,focal ratio
    357, // focal_length;
    0,   // max_fps,max fps of sensor's all settings,it will be calculated from
         // sensor mode fps
    16 * 256,                 // max_adgain,AD-gain
    0,                        // ois_supported;
    SENSOR_PDAF_TYPE3_ENABLE, // pdaf_supported;
    //SENSOR_PDAF_TYPE2_ENABLE, // pdaf_supported;
    1,                        // exp_valid_frame_num;N+2-1
    64,                       // clamp_level,black level
    1,                        // adgain_valid_frame_num;N+1-1
};

static SENSOR_MODE_FPS_INFO_T s_s5k3p8sm_mode_fps_info = {
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

SENSOR_INFO_T g_s5k3p8sm_mipi_raw_info = {
    S5K3P8SM_I2C_ADDR_W, // salve i2c write address
    S5K3P8SM_I2C_ADDR_R, // salve i2c read address

    SENSOR_I2C_REG_16BIT | SENSOR_I2C_VAL_16BIT |
        SENSOR_I2C_FREQ_400, // bit0: 0: i2c register value is 8 bit, 1: i2c
                             // register value is 16 bit
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

    1,            // count of identify code
    {{0x0, 0x5e}, // supply two code to identify sensor.
     {0x1,
      0x20}}, // for Example: index = 0-> Device id, index = 1 -> version id

    //	SENSOR_AVDD_3000MV,	// voltage of avdd
    SENSOR_AVDD_2800MV, // voltage of avdd

    4640,                           // 4632,			// max width of source image
    3488,                           // 3480,			// max height of source image
    (cmr_s8 *) "s5k3p8sm_mipi_raw", // name of sensor

    SENSOR_IMAGE_FORMAT_RAW, // define in SENSOR_IMAGE_FORMAT_E
                             // enum,SENSOR_IMAGE_FORMAT_MAX
    // if set to SENSOR_IMAGE_FORMAT_MAX here, image format depent on
    // SENSOR_REG_TAB_INFO_T

    SENSOR_IMAGE_PATTERN_RAWRGB_GR, // SENSOR_IMAGE_PATTERN_RAWRGB_R,// pattern
                                    // of input image form sensor;

    s_s5k3p8sm_resolution_Tab_RAW, // point to resolution table information
                                   // structure
    &s_s5k3p8sm_ioctl_func_tab,    // point to ioctl function table
    &s_s5k3p8sm_mipi_raw_info_ptr, // information and table about Rawrgb sensor
    NULL, //&g_s5k3p8sm_ext_info,                // extend information about
    // sensor
    SENSOR_AVDD_1800MV, // iovdd
    SENSOR_AVDD_1000MV, // dvdd
    1,                  // skip frame num before preview
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
    {SENSOR_INTERFACE_TYPE_CSI2, 4, 10,
     0}, // csi, 4lanes, 10bits, 0:packed, 1: half-word
    s_s5k3p8sm_video_info,
    3,                              // skip frame num while change setting
    48,                             // horizontal view angle
    48,                             // vertical view angle
    (cmr_s8 *) "s5k3p8sm_truly_v1", // version info of sensor
};

static struct sensor_raw_info *Sensor_GetContext(SENSOR_HW_HANDLE handle) {
    return s_s5k3p8sm_mipi_raw_info_ptr;
}

#define param_update(x1, x2)                                                   \
    sprintf(name, "/data/s5k3p8sm_%s.bin", x1);                                \
    if (0 == access(name, R_OK)) {                                             \
        FILE *fp = NULL;                                                       \
        SENSOR_PRINT("param file %s exists", name);                            \
        if (NULL != (fp = fopen(name, "rb"))) {                                \
            fread((void *)x2, 1, sizeof(x2), fp);                              \
            fclose(fp);                                                        \
        } else {                                                               \
            SENSOR_PRINT("param open %s failure", name);                       \
        }                                                                      \
    }                                                                          \
    memset(name, 0, sizeof(name))

static unsigned long _s5k3p8sm_GetResolutionTrimTab(SENSOR_HW_HANDLE handle,
                                                    unsigned long param) {
    SENSOR_PRINT("0x%lx", (unsigned long)s_s5k3p8sm_Resolution_Trim_Tab);
    return (unsigned long)s_s5k3p8sm_Resolution_Trim_Tab;
}

static uint32_t _s5k3p8sm_init_mode_fps_info(SENSOR_HW_HANDLE handle) {
    uint32_t rtn = SENSOR_SUCCESS;
    SENSOR_PRINT("_s5k3p8sm_init_mode_fps_info:E");
    if (!s_s5k3p8sm_mode_fps_info.is_init) {
        uint32_t i, modn, tempfps = 0;
        SENSOR_PRINT("_s5k3p8sm_init_mode_fps_info:start init");
        for (i = 0; i < NUMBER_OF_ARRAY(s_s5k3p8sm_Resolution_Trim_Tab); i++) {
            // max fps should be multiple of 30,it calulated from line_time and
            // frame_line
            tempfps = s_s5k3p8sm_Resolution_Trim_Tab[i].line_time *
                      s_s5k3p8sm_Resolution_Trim_Tab[i].frame_line;
            if (0 != tempfps) {
                tempfps = 1000000000 / tempfps;
                modn = tempfps / 30;
                if (tempfps > modn * 30)
                    modn++;
                s_s5k3p8sm_mode_fps_info.sensor_mode_fps[i].max_fps = modn * 30;
                if (s_s5k3p8sm_mode_fps_info.sensor_mode_fps[i].max_fps > 30) {
                    s_s5k3p8sm_mode_fps_info.sensor_mode_fps[i].is_high_fps = 1;
                    s_s5k3p8sm_mode_fps_info.sensor_mode_fps[i]
                        .high_fps_skip_num =
                        s_s5k3p8sm_mode_fps_info.sensor_mode_fps[i].max_fps /
                        30;
                }
                if (s_s5k3p8sm_mode_fps_info.sensor_mode_fps[i].max_fps >
                    s_s5k3p8sm_static_info.max_fps) {
                    s_s5k3p8sm_static_info.max_fps =
                        s_s5k3p8sm_mode_fps_info.sensor_mode_fps[i].max_fps;
                }
            }
            SENSOR_PRINT("mode %d,tempfps %d,frame_len %d,line_time: %d ", i,
                         tempfps, s_s5k3p8sm_Resolution_Trim_Tab[i].frame_line,
                         s_s5k3p8sm_Resolution_Trim_Tab[i].line_time);
            SENSOR_PRINT("mode %d,max_fps: %d ", i,
                         s_s5k3p8sm_mode_fps_info.sensor_mode_fps[i].max_fps);
            SENSOR_PRINT(
                "is_high_fps: %d,highfps_skip_num %d",
                s_s5k3p8sm_mode_fps_info.sensor_mode_fps[i].is_high_fps,
                s_s5k3p8sm_mode_fps_info.sensor_mode_fps[i].high_fps_skip_num);
        }
        s_s5k3p8sm_mode_fps_info.is_init = 1;
    }
    SENSOR_PRINT("_s5k3p8sm_init_mode_fps_info:X");
    return rtn;
}

SENSOR_HW_HANDLE _s5k3p8sm_Create(void *privatedata) {
    SENSOR_PRINT("_s5k3p8sm_Create  IN");
    if (NULL == privatedata) {
        SENSOR_PRINT("_s5k3p8sm_Create invalied para");
        return NULL;
    }
    SENSOR_HW_HANDLE sensor_hw_handle =
        (SENSOR_HW_HANDLE)malloc(sizeof(SENSOR_HW_HANDLE));
    if (NULL == sensor_hw_handle) {
        SENSOR_PRINT("failed to create sensor_hw_handle");
        return NULL;
    }
    cmr_bzero(sensor_hw_handle, sizeof(SENSOR_HW_HANDLE));
    sensor_hw_handle->privatedata = privatedata;

    return sensor_hw_handle;
}

void _s5k3p8sm_Destroy(SENSOR_HW_HANDLE handle) {
    SENSOR_PRINT("_s5k3p8sm_Destroy  IN");
    if (NULL == handle) {
        SENSOR_PRINT("_s5k3p8sm_Destroy handle null");
    }
    free((void *)handle->privatedata);
    handle->privatedata = NULL;
}

static unsigned long _s5k3p8sm_Identify(SENSOR_HW_HANDLE handle,
                                        unsigned long param) {
#define S5K3P8SM_PID_VALUE 0x3108
#define S5K3P8SM_PID_ADDR 0x0000
#define S5K3P8SM_VER_VALUE 0xb001
#define S5K3P8SM_VER_ADDR 0x0002
    uint16_t pid_value = 0x00;
    uint16_t ver_value = 0x00;
    uint32_t ret_value = SENSOR_FAIL;
    uint16_t i = 0;
    uint16_t ret;

    SENSOR_PRINT_ERR("SENSOR_S5K3P8SM: mipi raw identify\n");

    pid_value = Sensor_ReadReg(S5K3P8SM_PID_ADDR);
    ver_value = Sensor_ReadReg(S5K3P8SM_VER_ADDR);

    if (S5K3P8SM_PID_VALUE == pid_value) {
        SENSOR_PRINT("SENSOR_S5K3P8SM: Identify: PID = %x, VER = %x", pid_value,
                     ver_value);
        if (S5K3P8SM_VER_VALUE == ver_value) {
            SENSOR_PRINT_ERR("SENSOR_S5K3P8SM: this is S5K3P8SM sensor !");
            ret_value = _s5k3p8sm_GetRawInof(handle);
            if (SENSOR_SUCCESS != ret_value) {
                SENSOR_PRINT_ERR(
                    "SENSOR_S5K3P8SM: the module is unknow error !");
            }
#if defined(CONFIG_CAMERA_OIS_FUNC)
            for (i = 0; i < 3; i++) {
                ret = ois_pre_open(handle);
                if (ret == 0)
                    break;
                usleep(20 * 1000);
            }
            if (ret == 0)
                OpenOIS(handle);
#else
// bu64297gwz_init(handle);
#endif
            _s5k3p8sm_init_mode_fps_info(handle);
        } else {
            SENSOR_PRINT_ERR(
                "SENSOR_S5K3P8SM: Identify this is hm%x%x sensor !", pid_value,
                ver_value);
            return ret_value;
        }
    } else {
        SENSOR_PRINT_ERR(
            "SENSOR_S5K3P8SM: identify fail,pid_value=%d, ver_value = %d",
            pid_value, ver_value);
    }

    return ret_value;
}

static uint32_t _s5k3p8sm_GetMaxFrameLine(uint32_t index) {
    uint32_t max_line = 0x00;
    SENSOR_TRIM_T_PTR trim_ptr = s_s5k3p8sm_Resolution_Trim_Tab;

    max_line = trim_ptr[index].frame_line;

    return max_line;
}

static unsigned long
_s5k3p8sm_write_exp_dummy(SENSOR_HW_HANDLE handle, uint16_t expsure_line,
                          uint16_t dummy_line, uint16_t size_index)

{
    uint32_t ret_value = SENSOR_SUCCESS;
    uint32_t frame_len_cur = 0x00;
    uint32_t frame_len = 0x00;
    uint32_t max_frame_len = 0x00;
    uint32_t linetime = 0;

    SENSOR_PRINT(
        "SENSOR_S5K3P8SM: write_exp_dummy line:%d, dummy:%d, size_index:%d",
        expsure_line, dummy_line, size_index);
    max_frame_len = _s5k3p8sm_GetMaxFrameLine(size_index);
    if (expsure_line < 3) {
        expsure_line = 3;
    }

    frame_len = expsure_line + dummy_line;
    frame_len = (frame_len > (uint32_t)(expsure_line + 5))
                    ? frame_len
                    : (uint32_t)(expsure_line + 5);
    frame_len = (frame_len > max_frame_len) ? frame_len : max_frame_len;
    if (0x00 != (0x01 & frame_len)) {
        frame_len += 0x01;
    }

    frame_len_cur = Sensor_ReadReg(0x0340);

    if (frame_len_cur != frame_len) {
        ret_value = Sensor_WriteReg(0x0340, frame_len);
    }

    ret_value = Sensor_WriteReg(0x202, expsure_line);

    /*if (frame_len_cur > frame_len) {
            ret_value = Sensor_WriteReg(0x0341, frame_len & 0xff);
            ret_value = Sensor_WriteReg(0x0340, (frame_len >> 0x08) & 0xff);
    }*/
    // ret_value = Sensor_WriteReg(0x104, 0x00);
    s_capture_shutter = expsure_line;
    linetime = s_s5k3p8sm_Resolution_Trim_Tab[size_index].line_time;
    Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_EXPOSURETIME, s_capture_shutter);
    s_exposure_time = s_capture_shutter * linetime / 1000;

    return ret_value;
}

static unsigned long _s5k3p8sm_write_exposure(SENSOR_HW_HANDLE handle,
                                              unsigned long param) {
    uint32_t ret_value = SENSOR_SUCCESS;
    uint32_t expsure_line = 0x00;
    uint32_t dummy_line = 0x00;
    uint32_t size_index = 0x00;

    expsure_line = param & 0xffff;
    dummy_line = (param >> 0x10) & 0x0fff;
    size_index = (param >> 0x1c) & 0x0f;

    SENSOR_PRINT(
        "SENSOR_S5K3P8SM: write_exposure line:%d, dummy:%d, size_index:%d",
        expsure_line, dummy_line, size_index);

    ret_value =
        _s5k3p8sm_write_exp_dummy(handle, expsure_line, dummy_line, size_index);

    s_sensor_ev_info.preview_shutter = s_capture_shutter;

    return ret_value;
}

#define OTP_LSC_INFO_LEN 1658

static cmr_u8 s5k3p3_opt_lsc_data[OTP_LSC_INFO_LEN];
static struct sensor_otp_cust_info s5k3p3_otp_info;

static cmr_u8 s5k3p8sm_i2c_read_otp_set(SENSOR_HW_HANDLE handle, cmr_u16 addr) {
    return sensor_grc_read_i2c(0xA0 >> 1, addr, BITS_ADDR16_REG8);
}

static int s5k3p8sm_otp_init(SENSOR_HW_HANDLE handle) { return 0; }

static int s5k3p8sm_otp_read_data(SENSOR_HW_HANDLE handle) {
    cmr_u16 i = 0;
    cmr_u8 high_val = 0;
    cmr_u8 low_val = 0;
    cmr_u32 checksum = 0;
    struct sensor_single_otp_info *single_otp = NULL;

    single_otp = &s5k3p3_otp_info.single_otp;
    single_otp->program_flag = s5k3p8sm_i2c_read_otp(0x0000);
    SENSOR_PRINT("program_flag = %d", single_otp->program_flag);
    if (1 != single_otp->program_flag) {
        SENSOR_PRINT("failed to read otp or the otp is wrong data");
        return -1;
    }
    checksum += single_otp->program_flag;
    single_otp->module_info.year = s5k3p8sm_i2c_read_otp(0x0001);
    checksum += single_otp->module_info.year;
    single_otp->module_info.month = s5k3p8sm_i2c_read_otp(0x0002);
    checksum += single_otp->module_info.month;
    single_otp->module_info.day = s5k3p8sm_i2c_read_otp(0x0003);
    checksum += single_otp->module_info.day;
    single_otp->module_info.mid = s5k3p8sm_i2c_read_otp(0x0004);
    checksum += single_otp->module_info.mid;
    single_otp->module_info.lens_id = s5k3p8sm_i2c_read_otp(0x0005);
    checksum += single_otp->module_info.lens_id;
    single_otp->module_info.vcm_id = s5k3p8sm_i2c_read_otp(0x0006);
    checksum += single_otp->module_info.vcm_id;
    single_otp->module_info.driver_ic_id = s5k3p8sm_i2c_read_otp(0x0007);
    checksum += single_otp->module_info.driver_ic_id;

    high_val = s5k3p8sm_i2c_read_otp(0x0010);
    checksum += high_val;
    low_val = s5k3p8sm_i2c_read_otp(0x0011);
    checksum += low_val;
    single_otp->iso_awb_info.iso = (high_val << 8 | low_val);
    high_val = s5k3p8sm_i2c_read_otp(0x0012);
    checksum += high_val;
    low_val = s5k3p8sm_i2c_read_otp(0x0013);
    checksum += low_val;
    single_otp->iso_awb_info.gain_r = (high_val << 8 | low_val);
    high_val = s5k3p8sm_i2c_read_otp(0x0014);
    checksum += high_val;
    low_val = s5k3p8sm_i2c_read_otp(0x0015);
    checksum += low_val;
    single_otp->iso_awb_info.gain_g = (high_val << 8 | low_val);
    high_val = s5k3p8sm_i2c_read_otp(0x0016);
    checksum += high_val;
    low_val = s5k3p8sm_i2c_read_otp(0x0017);
    checksum += low_val;
    single_otp->iso_awb_info.gain_b = (high_val << 8 | low_val);

    for (i = 0; i < OTP_LSC_INFO_LEN; i++) {
        s5k3p3_opt_lsc_data[i] = s5k3p8sm_i2c_read_otp(0x0020 + i);
        checksum += s5k3p3_opt_lsc_data[i];
    }

    single_otp->lsc_info.lsc_data_addr = s5k3p3_opt_lsc_data;
    single_otp->lsc_info.lsc_data_size = sizeof(s5k3p3_opt_lsc_data);

    single_otp->af_info.flag = s5k3p8sm_i2c_read_otp(0x06A0);
    if (0 == single_otp->af_info.flag)
        SENSOR_PRINT("af otp is wrong");

    checksum = 0; //+= s5k3p3_otp_info.af_info.flag;
    /* cause checksum, skip af flag */
    low_val = s5k3p8sm_i2c_read_otp(0x06A1);
    checksum += low_val;
    high_val = s5k3p8sm_i2c_read_otp(0x06A2);
    checksum += high_val;
    single_otp->af_info.infinite_cali = (high_val << 8 | low_val);
    low_val = s5k3p8sm_i2c_read_otp(0x06A3);
    checksum += low_val;
    high_val = s5k3p8sm_i2c_read_otp(0x06A4);
    checksum += high_val;
    single_otp->af_info.macro_cali = (high_val << 8 | low_val);

    single_otp->checksum = s5k3p8sm_i2c_read_otp(0x06A5);
    SENSOR_PRINT("checksum = 0x%x s5k3p3_otp_info.checksum = 0x%x", checksum,
                 single_otp->checksum);

    if ((checksum % 0xff) != single_otp->checksum) {
        SENSOR_PRINT_ERR("checksum error!");
        single_otp->program_flag = 0;
        return -1;
    }

    return 0;
}

static unsigned long s5k3p8sm_otp_read(SENSOR_HW_HANDLE handle,
                                       SENSOR_VAL_T *param) {
    struct sensor_otp_cust_info *otp_info = NULL;
    SENSOR_PRINT("E");
    s5k3p8sm_otp_read_data(handle);
    otp_info = &s5k3p3_otp_info;

    if (1 != otp_info->single_otp.program_flag) {
        SENSOR_PRINT_ERR("otp error");
        param->pval = NULL;
        return -1;
    } else {
        param->pval = (void *)otp_info;
    }
    SENSOR_PRINT("param->pval = %p", param->pval);

    return 0;
}

static unsigned long s5k3p8sm_parse_otp(SENSOR_HW_HANDLE handle,
                                        SENSOR_VAL_T *param) {
    cmr_u8 *buff = NULL;
    struct sensor_single_otp_info *single_otp = NULL;
    cmr_u16 i = 0;
    cmr_u16 j = 0;
    cmr_u8 high_val = 0;
    cmr_u8 low_val = 0;
    cmr_u32 checksum = 0;

    SENSOR_PRINT("E");
    if (NULL == param->pval) {
        SENSOR_PRINT("s5k3p8sm_parse_otp is NULL data");
        return -1;
    }
    buff = param->pval;

    if (1 != buff[0]) {
        SENSOR_PRINT("s5k3p8sm_parse_otp is wrong data");
        param->pval = NULL;
        return -1;
    } else {
        single_otp = &s5k3p3_otp_info.single_otp;
        single_otp->program_flag = buff[i++];
        SENSOR_PRINT("program_flag = %d", single_otp->program_flag);

        checksum += single_otp->program_flag;
        single_otp->module_info.year = buff[i++];
        checksum += single_otp->module_info.year;
        single_otp->module_info.month = buff[i++];
        checksum += single_otp->module_info.month;
        single_otp->module_info.day = buff[i++];
        checksum += single_otp->module_info.day;
        single_otp->module_info.mid = buff[i++];
        checksum += single_otp->module_info.mid;
        single_otp->module_info.lens_id = buff[i++];
        checksum += single_otp->module_info.lens_id;
        single_otp->module_info.vcm_id = buff[i++];
        checksum += single_otp->module_info.vcm_id;
        single_otp->module_info.driver_ic_id = buff[i++];
        checksum += single_otp->module_info.driver_ic_id;

        high_val = buff[i++];
        checksum += high_val;
        low_val = buff[i++];
        checksum += low_val;
        single_otp->iso_awb_info.iso = (high_val << 8 | low_val);
        high_val = buff[i++];
        checksum += high_val;
        low_val = buff[i++];
        checksum += low_val;
        single_otp->iso_awb_info.gain_r = (high_val << 8 | low_val);
        high_val = buff[i++];
        checksum += high_val;
        low_val = buff[i++];
        checksum += low_val;
        single_otp->iso_awb_info.gain_g = (high_val << 8 | low_val);
        high_val = buff[i++];
        checksum += high_val;
        low_val = buff[i++];
        checksum += low_val;
        single_otp->iso_awb_info.gain_b = (high_val << 8 | low_val);

        for (j = 0; j < OTP_LSC_INFO_LEN; j++) {
            s5k3p3_opt_lsc_data[j] = buff[i++];
            checksum += s5k3p3_opt_lsc_data[j];
        }
        single_otp->lsc_info.lsc_data_addr = s5k3p3_opt_lsc_data;
        single_otp->lsc_info.lsc_data_size = sizeof(s5k3p3_opt_lsc_data);

        single_otp->af_info.flag = buff[i++];
        if (0 == single_otp->af_info.flag)
            SENSOR_PRINT("af otp is wrong");

        checksum = 0; //+= s5k3p3_otp_info.af_info.flag;
        /* cause checksum, skip af flag */
        low_val = buff[i++];
        checksum += low_val;
        high_val = buff[i++];
        checksum += high_val;
        single_otp->af_info.infinite_cali = (high_val << 8 | low_val);
        low_val = buff[i++];
        checksum += low_val;
        high_val = buff[i++];
        checksum += high_val;
        single_otp->af_info.macro_cali = (high_val << 8 | low_val);

        single_otp->checksum = buff[i++];
        SENSOR_PRINT(
            "checksum = 0x%x s5k3p3_otp_info.checksum = 0x%x,r=%d,g=%d,b=%d",
            checksum, single_otp->checksum, single_otp->iso_awb_info.gain_r,
            single_otp->iso_awb_info.gain_g, single_otp->iso_awb_info.gain_b);

        if ((checksum % 0xff) != single_otp->checksum) {
            SENSOR_PRINT_ERR("checksum error!");
            single_otp->program_flag = 0;
            param->pval = NULL;
            return -1;
        }
        param->pval = (void *)&s5k3p3_otp_info;
    }
    SENSOR_PRINT("param->pval = %p", param->pval);

    return 0;
}

static unsigned long _s5k3p8sm_ex_write_exposure(SENSOR_HW_HANDLE handle,
                                                 unsigned long param) {
    uint32_t ret_value = SENSOR_SUCCESS;
    uint16_t exposure_line = 0x00;
    uint16_t dummy_line = 0x00;
    uint16_t size_index = 0x00;
    struct sensor_ex_exposure *ex = (struct sensor_ex_exposure *)param;

    if (!param) {
        SENSOR_PRINT_ERR("param is NULL !!!");
        return ret_value;
    }

    exposure_line = ex->exposure;
    dummy_line = ex->dummy;
    size_index = ex->size_index;

    ret_value = _s5k3p8sm_write_exp_dummy(handle, exposure_line, dummy_line,
                                          size_index);

    s_sensor_ev_info.preview_shutter = s_capture_shutter;

    return ret_value;
}

static unsigned long _s5k3p8sm_update_gain(SENSOR_HW_HANDLE handle,
                                           unsigned long param) {
    uint32_t ret_value = SENSOR_SUCCESS;
    uint32_t real_gain = 0;
    uint32_t a_gain = 0;
    uint32_t d_gain = 0;

    real_gain = param >> 2; // / 128 * 32;

    SENSOR_PRINT("SENSOR_S5K3P8SM: real_gain:%d, param: %ld", real_gain, param);

    if (real_gain <= 16 * 32) {
        a_gain = real_gain;
        d_gain = 256;
    } else {
        a_gain = 16 * 32;
        d_gain = 256.0 * real_gain / a_gain;
        SENSOR_LOGI("_s5k3p8sm: real_gain:0x%x, a_gain: 0x%x, d_gain: 0x%x",
                    (uint32_t)real_gain, (uint32_t)a_gain, (uint32_t)d_gain);
        if ((uint32_t)d_gain > 256 * 256)
            d_gain = 256 * 256; // d_gain < 256x
    }

    ret_value = Sensor_WriteReg(0x204, a_gain);

    ret_value = Sensor_WriteReg(0x20e, d_gain);
    ret_value = Sensor_WriteReg(0x210, d_gain);
    ret_value = Sensor_WriteReg(0x212, d_gain);
    ret_value = Sensor_WriteReg(0x214, d_gain);

    //	ret_value = Sensor_WriteReg(0x104, 0x00);
    SENSOR_PRINT("SENSOR_S5K3P8SM: a_gain:0x%x, d_gain: 0x%x", a_gain, d_gain);

    return ret_value;
}

static unsigned long _s5k3p8sm_write_gain(SENSOR_HW_HANDLE handle,
                                          unsigned long param) {
    uint32_t ret_value = SENSOR_SUCCESS;
    SENSOR_PRINT("SENSOR_S5K3P8SM: param: %ld", param);
    _s5k3p8sm_update_gain(handle, param);
    s_sensor_ev_info.preview_gain = param;

    return ret_value;
}

static unsigned long s5k3p8sm_write_af(SENSOR_HW_HANDLE handle,
                                       unsigned long param) {
#if defined(CONFIG_CAMERA_OIS_FUNC)
    OIS_write_af(handle, param);
#else
    // bu64297gwz_write_af(handle, param);
    return 0;
#endif
    return 0;
}

static uint32_t _s5k3p8sm_ReadGain(SENSOR_HW_HANDLE handle, uint32_t param) {
    uint32_t rtn = SENSOR_SUCCESS;
    uint32_t again = 0;
    uint32_t dgain = 0;
    uint32_t gain = 0;

#if 1 // for MP tool //!??
    again = Sensor_ReadReg(0x0204);
    dgain = Sensor_ReadReg(0x0210);
    gain = again * dgain;

#endif
    // s_s5k3p8sm_gain=(int)gain;
    SENSOR_PRINT("SENSOR_s5k3p8sm: _s5k3p8sm_ReadGain gain: 0x%x", gain);
    return rtn;
}

static unsigned long _s5k3p8sm_BeforeSnapshot(SENSOR_HW_HANDLE handle,
                                              unsigned long param) {
    uint8_t ret_l, ret_m, ret_h;
    uint32_t capture_exposure, preview_maxline;
    uint32_t capture_maxline, preview_exposure;
    uint32_t capture_mode = param & 0xffff;
    uint32_t preview_mode = (param >> 0x10) & 0xffff;
    uint32_t prv_linetime =
        s_s5k3p8sm_Resolution_Trim_Tab[preview_mode].line_time;
    uint32_t cap_linetime =
        s_s5k3p8sm_Resolution_Trim_Tab[capture_mode].line_time;
    uint32_t frame_len = 0x00;
    uint32_t gain = 0;

    SENSOR_PRINT(
        "SENSOR_s5k3p8sm: BeforeSnapshot mode: 0x%08lx,capture_mode:%d", param,
        capture_mode);

    if (preview_mode == capture_mode) {
        SENSOR_PRINT("SENSOR_s5k3p8sm: prv mode equal to capmode");
        goto CFG_INFO;
    }

    Sensor_SetMode(capture_mode);
    Sensor_SetMode_WaitDone();

    preview_exposure = s_sensor_ev_info.preview_shutter;
    gain = s_sensor_ev_info.preview_gain;

    capture_exposure = preview_exposure * prv_linetime / cap_linetime;
    /*
            while(gain >= 0x40){
                    capture_exposure = capture_exposure * 2;
                    gain=gain / 2;
                    if(capture_exposure > frame_len*2)
                            break;
            }*/
    SENSOR_PRINT("SENSOR_s5k3p8sm: prev_exp=%d,cap_exp=%d,gain=%d",
                 preview_exposure, capture_exposure, gain);

    _s5k3p8sm_write_exp_dummy(handle, capture_exposure, 0, capture_mode);
    _s5k3p8sm_update_gain(handle, gain);

    Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_EXPOSURETIME, capture_exposure);
    Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_APERTUREVALUE, 20);
    Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_MAXAPERTUREVALUE, 20);
    Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_FNUMBER, 20);

CFG_INFO:
    s_capture_shutter = _s5k3p8sm_get_shutter(handle);
    Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_EXPOSURETIME, s_capture_shutter);
    Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_APERTUREVALUE, 20);
    Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_MAXAPERTUREVALUE, 20);
    Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_FNUMBER, 20);
    s_exposure_time = s_capture_shutter * cap_linetime / 1000;

    return SENSOR_SUCCESS;
}

static uint32_t _s5k3p8sm_GetRawInof(SENSOR_HW_HANDLE handle) {
    uint32_t rtn = SENSOR_SUCCESS;
    struct raw_param_info_tab *tab_ptr =
        (struct raw_param_info_tab *)s_s5k3p8sm_raw_param_tab;
    uint32_t param_id;
    uint32_t i = 0x00;

    /*read param id from sensor omap*/
    param_id = S5K3P8SM_RAW_PARAM_COM;

    for (i = 0x00;; i++) {
        g_module_id = i;
        if (RAW_INFO_END_ID == tab_ptr[i].param_id) {
            if (NULL == s_s5k3p8sm_mipi_raw_info_ptr) {
                SENSOR_PRINT(
                    "SENSOR_S5K3P8SM: _s5k3p8sm_GetRawInof no param error");
                rtn = SENSOR_FAIL;
            }
            SENSOR_PRINT("SENSOR_S5K3P8SM: _s5k3p8sm_GetRawInof end");
            break;
        } else if (PNULL != tab_ptr[i].identify_otp) {
            if (SENSOR_SUCCESS == tab_ptr[i].identify_otp(handle, 0)) {
                // s_s5k3p8sm_mipi_raw_info_ptr = tab_ptr[i].info_ptr;
                SENSOR_PRINT("SENSOR_S5K3P8SM: _s5k3p8sm_GetRawInof success");
                break;
            }
        }
    }

    return rtn;
}

static unsigned long _s5k3p8sm_GetExifInfo(SENSOR_HW_HANDLE handle,
                                           unsigned long param) {
    LOCAL EXIF_SPEC_PIC_TAKING_COND_T sexif;

    sexif.ExposureTime.numerator = s_exposure_time;
    sexif.ExposureTime.denominator = 1000000;

    return (unsigned long)&sexif;
}

static unsigned long _s5k3p8sm_StreamOn(SENSOR_HW_HANDLE handle,
                                        unsigned long param) {
    SENSOR_PRINT_ERR("SENSOR_S5K3P8SM: StreamOn");

    Sensor_WriteReg(0x0100, 0x0100);
#if 1
    char value1[PROPERTY_VALUE_MAX];
    property_get("debug.camera.test.mode", value1, "0");
    if (!strcmp(value1, "1")) {
        SENSOR_PRINT_ERR("SENSOR_s5k3p8sm: enable test mode");
        Sensor_WriteReg(0x0600, 0x0002);
    }
#endif

    SENSOR_PRINT_ERR("SENSOR_S5K3P8SM: StreamOn end");

    return 0;
}

static unsigned long _s5k3p8sm_StreamOff(SENSOR_HW_HANDLE handle,
                                         unsigned long param) {
    uint16_t value;

    SENSOR_LOGI("E");

    value = Sensor_ReadReg(0x0100);
    Sensor_WriteReg(0x0100, 0x0000);
    if (value == 0x0100)
            usleep(50 * 1000);

    return 0;
}

static uint32_t _s5k3p8sm_com_Identify_otp(SENSOR_HW_HANDLE handle,
                                           void *param_ptr) {
    uint32_t rtn = SENSOR_FAIL;
    uint32_t param_id;

    SENSOR_PRINT("SENSOR_S5K3P8SM: _s5k3p8sm_com_Identify_otp");

    /*read param id from sensor omap*/
    param_id = S5K3P8SM_RAW_PARAM_COM;

    if (S5K3P8SM_RAW_PARAM_COM == param_id) {
        rtn = SENSOR_SUCCESS;
    }

    return rtn;
}

static unsigned long _s5k3p8sm_PowerOn(SENSOR_HW_HANDLE handle,
                                       unsigned long power_on) {
    SENSOR_AVDD_VAL_E dvdd_val = g_s5k3p8sm_mipi_raw_info.dvdd_val;
    SENSOR_AVDD_VAL_E avdd_val = g_s5k3p8sm_mipi_raw_info.avdd_val;
    SENSOR_AVDD_VAL_E iovdd_val = g_s5k3p8sm_mipi_raw_info.iovdd_val;
    BOOLEAN power_down = g_s5k3p8sm_mipi_raw_info.power_down_level;
    BOOLEAN reset_level = g_s5k3p8sm_mipi_raw_info.reset_pulse_level;

    uint8_t pid_value = 0x00;

    if (SENSOR_TRUE == power_on) {
        Sensor_SetResetLevel(reset_level);
        Sensor_PowerDown(power_down);
        Sensor_SetVoltage(dvdd_val, avdd_val, iovdd_val);
        usleep(1);
        Sensor_SetResetLevel(!reset_level);
        Sensor_PowerDown(!power_down);
        usleep(20);
        //_dw9807_SRCInit(2);
        Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);
        usleep(1000);
    } else {
        Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
        Sensor_SetResetLevel(reset_level);
        Sensor_PowerDown(power_down);
        Sensor_SetVoltage(SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED,
                          SENSOR_AVDD_CLOSED);
    }

    SENSOR_PRINT_ERR("SENSOR_S5K3P8SM: _s5k3p8sm_PowerOn(1:on, 0:off): %ld, "
                     "reset_level %d, dvdd_val %d",
                     power_on, reset_level, dvdd_val);
    return SENSOR_SUCCESS;
}

static uint32_t _s5k3p8sm_write_otp_gain(SENSOR_HW_HANDLE handle,
                                         uint32_t *param) {
    uint32_t ret_value = SENSOR_SUCCESS;
    uint16_t value = 0x00;
    float a_gain = 0;
    float d_gain = 0;
    float real_gain = 0.0f;
    SENSOR_PRINT("SENSOR_s5k4h8yx: write_gain:0x%x\n", *param);

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

    SENSOR_PRINT("SENSOR_s5k3p8sm: write_gain:0x%x\n", *param);
    /*
            //ret_value = Sensor_WriteReg(0x104, 0x01);
            value = (*param)>>0x08;
            ret_value = Sensor_WriteReg(0x204, value);
            value = (*param)&0xff;
            ret_value = Sensor_WriteReg(0x205, value);
            //ret_value = Sensor_WriteReg(0x104, 0x00);
    */
    return ret_value;
}

static uint32_t _s5k3p8sm_read_otp_gain(SENSOR_HW_HANDLE handle,
                                        uint32_t *param) {
    uint32_t rtn = SENSOR_SUCCESS;
    uint16_t gain_a = 0;
    uint16_t gain_d = 0;
#if 1 // for MP tool //!??
    gain_a = Sensor_ReadReg(0x0204) & 0xffff;
    gain_d = Sensor_ReadReg(0x0210) & 0xffff;
    *param = gain_a * gain_d;
#else
    *param = s_set_gain;
#endif
    SENSOR_PRINT("SENSOR_s5k3p8sm: gain: %d", *param);

    return rtn;
}

static uint16_t _s5k3p8sm_get_shutter(SENSOR_HW_HANDLE handle) {
    // read shutter, in number of line period
    uint16_t shutter;
#if 1 // MP tool //!??
    shutter = Sensor_ReadReg(0x0202) & 0xffff;
    // shutter_l = Sensor_ReadReg(0x0203) & 0xff;

    return shutter; //(shutter_h << 8) | shutter_l;
#else
    return s_set_exposure;
#endif
}

static uint32_t _s5k3p8sm_get_static_info(SENSOR_HW_HANDLE handle,
                                          uint32_t *param) {
    uint32_t rtn = SENSOR_SUCCESS;
    struct sensor_ex_info *ex_info;
    uint32_t up = 0;
    uint32_t down = 0;
    // make sure we have get max fps of all settings.
    if (!s_s5k3p8sm_mode_fps_info.is_init) {
        _s5k3p8sm_init_mode_fps_info(handle);
    }
    ex_info = (struct sensor_ex_info *)param;
    ex_info->f_num = s_s5k3p8sm_static_info.f_num;
    ex_info->focal_length = s_s5k3p8sm_static_info.focal_length;
    ex_info->max_fps = s_s5k3p8sm_static_info.max_fps;
    ex_info->max_adgain = s_s5k3p8sm_static_info.max_adgain;
    ex_info->ois_supported = s_s5k3p8sm_static_info.ois_supported;
    ex_info->pdaf_supported = s_s5k3p8sm_static_info.pdaf_supported;
    ex_info->exp_valid_frame_num = s_s5k3p8sm_static_info.exp_valid_frame_num;
    ex_info->clamp_level = s_s5k3p8sm_static_info.clamp_level;
    ex_info->adgain_valid_frame_num =
        s_s5k3p8sm_static_info.adgain_valid_frame_num;
    ex_info->preview_skip_num = g_s5k3p8sm_mipi_raw_info.preview_skip_num;
    ex_info->capture_skip_num = g_s5k3p8sm_mipi_raw_info.capture_skip_num;
    ex_info->name = (cmr_s8 *)g_s5k3p8sm_mipi_raw_info.name;
    ex_info->sensor_version_info =
        (cmr_s8 *)g_s5k3p8sm_mipi_raw_info.sensor_version_info;
#if defined(CONFIG_CAMERA_OIS_FUNC)
    Ois_get_pose_dis(handle, &up, &down);
#else
// bu64297gwz_get_pose_dis(handle, &up, &down);
#endif
    ex_info->pos_dis.up2hori = up;
    ex_info->pos_dis.hori2down = down;
    SENSOR_PRINT("SENSOR_s5k3p8sm: f_num: %d", ex_info->f_num);
    SENSOR_PRINT("SENSOR_s5k3p8sm: max_fps: %d", ex_info->max_fps);
    SENSOR_PRINT("SENSOR_s5k3p8sm: max_adgain: %d", ex_info->max_adgain);
    SENSOR_PRINT("SENSOR_s5k3p8sm: ois_supported: %d", ex_info->ois_supported);
    SENSOR_PRINT("SENSOR_s5k3p8sm: pdaf_supported: %d",
                 ex_info->pdaf_supported);
    SENSOR_PRINT("SENSOR_s5k3p8sm: exp_valid_frame_num: %d",
                 ex_info->exp_valid_frame_num);
    SENSOR_PRINT("SENSOR_s5k3p8sm: clam_level: %d", ex_info->clamp_level);
    SENSOR_PRINT("SENSOR_s5k3p8sm: adgain_valid_frame_num: %d",
                 ex_info->adgain_valid_frame_num);
    SENSOR_PRINT("SENSOR_s5k3p8sm: sensor name is: %s", ex_info->name);
    SENSOR_PRINT("SENSOR_s5k3p8sm: sensor version info is: %s",
                 ex_info->sensor_version_info);
    SENSOR_PRINT("SENSOR_s5k3p8sm: up2h %d h2down %d", ex_info->pos_dis.up2hori,
                 ex_info->pos_dis.hori2down);

    return rtn;
}

static uint32_t _s5k3p8sm_get_fps_info(SENSOR_HW_HANDLE handle,
                                       uint32_t *param) {
    uint32_t rtn = SENSOR_SUCCESS;
    SENSOR_MODE_FPS_T *fps_info;
    // make sure have inited fps of every sensor mode.
    if (!s_s5k3p8sm_mode_fps_info.is_init) {
        _s5k3p8sm_init_mode_fps_info(handle);
    }
    fps_info = (SENSOR_MODE_FPS_T *)param;
    uint32_t sensor_mode = fps_info->mode;
    fps_info->max_fps =
        s_s5k3p8sm_mode_fps_info.sensor_mode_fps[sensor_mode].max_fps;
    fps_info->min_fps =
        s_s5k3p8sm_mode_fps_info.sensor_mode_fps[sensor_mode].min_fps;
    fps_info->is_high_fps =
        s_s5k3p8sm_mode_fps_info.sensor_mode_fps[sensor_mode].is_high_fps;
    fps_info->high_fps_skip_num =
        s_s5k3p8sm_mode_fps_info.sensor_mode_fps[sensor_mode].high_fps_skip_num;
    SENSOR_PRINT("SENSOR_s5k3p8sm: mode %d, max_fps: %d", fps_info->mode,
                 fps_info->max_fps);
    SENSOR_PRINT("SENSOR_s5k3p8sm: min_fps: %d", fps_info->min_fps);
    SENSOR_PRINT("SENSOR_s5k3p8sm: is_high_fps: %d", fps_info->is_high_fps);
    SENSOR_PRINT("SENSOR_s5k3p8sm: high_fps_skip_num: %d",
                 fps_info->high_fps_skip_num);

    return rtn;
}

static const struct pd_pos_info _s5k3p8sm_pd_pos_l[] = {
    {20, 23}, {36, 27}, {56, 27}, {72, 23}, {24, 43}, {40, 47},
    {52, 47}, {68, 43}, {24, 59}, {40, 55}, {52, 55}, {68, 59},
    {20, 79}, {36, 75}, {56, 75}, {72, 79},
};

static const struct pd_pos_info _s5k3p8sm_pd_pos_r[] = {
    {20, 27}, {36, 31}, {56, 31}, {72, 27}, {24, 39}, {40, 43},
    {52, 43}, {68, 39}, {24, 63}, {40, 59}, {52, 59}, {68, 63},
    {20, 75}, {36, 71}, {56, 71}, {72, 75},

};

static uint32_t _s5k3p8sm_get_pdaf_info(SENSOR_HW_HANDLE handle,
                                        uint32_t *param) {
    uint32_t rtn = SENSOR_SUCCESS;
    struct sensor_pdaf_info *pdaf_info = NULL;
    cmr_u16 pd_pos_r_size = 0;
    cmr_u16 pd_pos_l_size = 0;

    /*TODO*/
    if (param == NULL) {
        SENSOR_PRINT_ERR("null input");
        return -1;
    }
    pdaf_info = (struct sensor_pdaf_info *)param;
    pd_pos_r_size = NUMBER_OF_ARRAY(_s5k3p8sm_pd_pos_r);
    pd_pos_l_size = NUMBER_OF_ARRAY(_s5k3p8sm_pd_pos_l);
    if (pd_pos_r_size != pd_pos_l_size) {
        SENSOR_PRINT_ERR(
            "s5k3l8xxm3_pd_pos_r size not match s5k3l8xxm3_pd_pos_l");
        return -1;
    }

    pdaf_info->pd_offset_x = 16;
    pdaf_info->pd_offset_y = 16;
    pdaf_info->pd_pitch_x = 64;
    pdaf_info->pd_pitch_y = 64;
    pdaf_info->pd_density_x = 16;
    pdaf_info->pd_density_y = 16;
    pdaf_info->pd_block_num_x = 72;
    pdaf_info->pd_block_num_y = 54;
    pdaf_info->pd_pos_size = pd_pos_r_size;
    pdaf_info->pd_pos_r = (struct pd_pos_info *)_s5k3p8sm_pd_pos_r;
    pdaf_info->pd_pos_l = (struct pd_pos_info *)_s5k3p8sm_pd_pos_l;
    pdaf_info->vendor_type = SENSOR_VENDOR_S5K3P8SM;
    pdaf_info->type2_info.data_type = 0x30;
    pdaf_info->type2_info.data_format = DATA_RAW10;
    pdaf_info->type2_info.width = 72 * 2;
    pdaf_info->type2_info.height = 54 * 16;
    pdaf_info->type2_info.pd_size = pdaf_info->type2_info.width * pdaf_info->type2_info.height * 10 / 8;

    return rtn;
}

static uint32_t _s5k3p8sm_set_sensor_close_flag(SENSOR_HW_HANDLE handle) {
    uint32_t rtn = SENSOR_SUCCESS;

    s_s5k3p8sm_sensor_close_flag = 1;

    return rtn;
}

static unsigned long _s5k3p8sm_access_val(SENSOR_HW_HANDLE handle,
                                          unsigned long param) {
    uint32_t rtn = SENSOR_SUCCESS;
    SENSOR_VAL_T *param_ptr = (SENSOR_VAL_T *)param;
    uint16_t tmp;

    SENSOR_PRINT("SENSOR_s5k3p8sm: _s5k3p8sm_access_val E param_ptr = %p",
                 param_ptr);
    if (!param_ptr) {
        return rtn;
    }

    SENSOR_PRINT("SENSOR_s5k3p8sm: param_ptr->type=%x", param_ptr->type);
    switch (param_ptr->type) {
    case SENSOR_VAL_TYPE_INIT_OTP:
        s5k3p8sm_otp_init(handle);
        break;
    case SENSOR_VAL_TYPE_SHUTTER:
        *((uint32_t *)param_ptr->pval) = _s5k3p8sm_get_shutter(handle);
        break;
    case SENSOR_VAL_TYPE_READ_VCM:
        // rtn = _s5k3p8sm_read_vcm(param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_WRITE_VCM:
        // rtn = _s5k3p8sm_write_vcm(param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_READ_OTP:
        s5k3p8sm_otp_read(handle, param_ptr);
        break;
    case SENSOR_VAL_TYPE_PARSE_OTP:
        s5k3p8sm_parse_otp(handle, param_ptr);
        break;
    case SENSOR_VAL_TYPE_WRITE_OTP:
        // rtn = _hi544_write_otp((uint32_t)param_ptr->pval);
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
        rtn = _s5k3p8sm_write_otp_gain(handle, param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_READ_OTP_GAIN:
        rtn = _s5k3p8sm_read_otp_gain(handle, param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_GET_STATIC_INFO:
        rtn = _s5k3p8sm_get_static_info(handle, param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_GET_FPS_INFO:
        rtn = _s5k3p8sm_get_fps_info(handle, param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_GET_PDAF_INFO:
        rtn = _s5k3p8sm_get_pdaf_info(handle, param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_SET_SENSOR_CLOSE_FLAG:
        rtn = _s5k3p8sm_set_sensor_close_flag(handle);
        break;
    default:
        break;
    }

    SENSOR_PRINT("SENSOR_s5k3p8sm: _s5k3p8sm_access_val X");

    return rtn;
}
