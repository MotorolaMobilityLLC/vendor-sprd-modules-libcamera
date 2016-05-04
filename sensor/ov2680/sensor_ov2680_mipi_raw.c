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
#if defined(CONFIG_CAMERA_ISP_VERSION_V3) || defined(CONFIG_CAMERA_ISP_VERSION_V4)
#include "sensor_ov2680_raw_param_v3.c"
#else
#include "sensor_ov2680_raw_param_v2.c"
#endif
#ifdef CONFIG_CAMERA_RE_FOCUS
#include "al3200.h"
#endif
#define ov2680_I2C_ADDR_W        (0x6c>>1)
#define ov2680_I2C_ADDR_R         (0x6c>>1)
#define RAW_INFO_END_ID 0x71717567

#define ov2680_MIN_FRAME_LEN_PRV  0x484
#define ov2680_MIN_FRAME_LEN_CAP  0x7B6
#define ov2680_RAW_PARAM_COM  0x0000
static uint32_t g_module_id = 0;

static uint32_t g_flash_mode_en = 0;
static uint32_t s_ov2680_gain = 0;
static int s_capture_VTS = 0;
static int s_capture_shutter = 0;

#define ov2680_RAW_PARAM_Truly     0x02
#define ov2680_RAW_PARAM_Sunny    0x01

static uint16_t RG_Ratio_Typical = 0x17d;
static uint16_t BG_Ratio_Typical = 0x164;

/*Revision: R2.52*/
struct otp_struct {
	int module_integrator_id;
	int lens_id;
	int rg_ratio;
	int bg_ratio;
	int user_data[2];
	int light_rg;
	int light_bg;
};

LOCAL uint32_t update_otp(SENSOR_HW_HANDLE handle, void* param_ptr);
LOCAL uint32_t _ov2680_Truly_Identify_otp(SENSOR_HW_HANDLE handle, void* param_ptr);
LOCAL uint32_t _ov2680_Sunny_Identify_otp(SENSOR_HW_HANDLE handle, void* param_ptr);
LOCAL unsigned long _ov2680_GetResolutionTrimTab(SENSOR_HW_HANDLE handle, unsigned long param);
LOCAL unsigned long _ov2680_PowerOn(SENSOR_HW_HANDLE handle, unsigned long power_on);
LOCAL unsigned long _ov2680_Identify(SENSOR_HW_HANDLE handle, unsigned long param);
LOCAL unsigned long _ov2680_BeforeSnapshot(SENSOR_HW_HANDLE handle, unsigned long param);
LOCAL unsigned long _ov2680_after_snapshot(SENSOR_HW_HANDLE handle, unsigned long param);
LOCAL unsigned long _ov2680_StreamOn(SENSOR_HW_HANDLE handle, unsigned long param);
LOCAL unsigned long _ov2680_StreamOff(SENSOR_HW_HANDLE handle, unsigned long param);
LOCAL unsigned long _ov2680_write_exposure(SENSOR_HW_HANDLE handle, unsigned long param);
LOCAL unsigned long _ov2680_write_gain(SENSOR_HW_HANDLE handle, unsigned long param);
LOCAL unsigned long _ov2680_write_af(SENSOR_HW_HANDLE handle, unsigned long param);
LOCAL unsigned long _ov2680_ReadGain(SENSOR_HW_HANDLE handle, unsigned long param);
LOCAL unsigned long _ov2680_SetEV(SENSOR_HW_HANDLE handle, unsigned long param);
LOCAL unsigned long _ov2680_ExtFunc(SENSOR_HW_HANDLE handle, unsigned long ctl_param);
LOCAL unsigned long _dw9174_SRCInit(SENSOR_HW_HANDLE handle, unsigned long mode);
LOCAL unsigned long _ov2680_flash(SENSOR_HW_HANDLE handle, unsigned long param);
LOCAL uint32_t _ov2680_com_Identify_otp(SENSOR_HW_HANDLE handle, void* param_ptr);
LOCAL unsigned long _ov2680_cfg_otp(SENSOR_HW_HANDLE handle, unsigned long  param);
LOCAL unsigned long _ov2680_access_val(SENSOR_HW_HANDLE handle, unsigned long param);

LOCAL const struct raw_param_info_tab s_ov2680_raw_param_tab[]={
	//{ov2680_RAW_PARAM_Sunny, &s_ov2680_mipi_raw_info, _ov2680_Sunny_Identify_otp, update_otp},
	//{ov2680_RAW_PARAM_Truly, &s_ov2680_mipi_raw_info, _ov2680_Truly_Identify_otp, update_otp},
	{ov2680_RAW_PARAM_COM, &s_ov2680_mipi_raw_info, _ov2680_com_Identify_otp, PNULL},
	{RAW_INFO_END_ID, PNULL, PNULL, PNULL}
};

struct sensor_raw_info* s_ov2680_mipi_raw_info_ptr=NULL;

//800x600
LOCAL const SENSOR_REG_T ov2680_com_mipi_raw[] = {
	{0x0103, 0x01},
	{0xffff, 0xa0},
	{0x3002, 0x00},
	{0x3016, 0x1c},
	{0x3018, 0x44},
	{0x3020, 0x00},
	{0x3080, 0x02},
	{0x3082, 0x37},
	{0x3084, 0x09},
	{0x3085, 0x04},
	{0x3086, 0x01},
	{0x3501, 0x26},
	{0x3502, 0x40},
	{0x3503, 0x03},
	{0x350b, 0x36},
	{0x3600, 0xb4},
	{0x3603, 0x39},
	{0x3604, 0x24},
	{0x3605, 0x00}, //bit3:   1: use external regulator  0: use internal regulator
	{0x3620, 0x26},
	{0x3621, 0x37},
	{0x3622, 0x04},
	{0x3628, 0x00},
	{0x3705, 0x3c},
	{0x370c, 0x50},
	{0x370d, 0xc0},
	{0x3718, 0x88},
	{0x3720, 0x00},
	{0x3721, 0x00},
	{0x3722, 0x00},
	{0x3723, 0x00},
	{0x3738, 0x00},
	{0x370a, 0x23},
	{0x3717, 0x58},
	{0x3781, 0x80},
	{0x3784, 0x0c},
	{0x3789, 0x60},
	{0x3800, 0x00},
	{0x3801, 0x00},
	{0x3802, 0x00},
	{0x3803, 0x00},
	{0x3804, 0x06},
	{0x3805, 0x4f},
	{0x3806, 0x04},
	{0x3807, 0xbf},
	{0x3808, 0x03},
	{0x3809, 0x20},
	{0x380a, 0x02},
	{0x380b, 0x58},
	{0x380c, 0x06},
	{0x380d, 0xac},
	{0x380e, 0x02},
	{0x380f, 0x84},
	{0x3810, 0x00},
	{0x3811, 0x04},
	{0x3812, 0x00},
	{0x3813, 0x04},
	{0x3814, 0x31},
	{0x3815, 0x31},
	{0x3819, 0x04},
#ifdef CONFIG_CAMERA_IMAGE_180
	{0x3820, 0xc2},
	{0x3821, 0x00},
#else
	{0x3820, 0xc6},
	{0x3821, 0x04},
#endif
	{0x4000, 0x81},
	{0x4001, 0x40},
	{0x4008, 0x00},
	{0x4009, 0x03},
	{0x4602, 0x02},
	{0x481b, 0x43},
	{0x481f, 0x36},
	{0x4825, 0x36},
	{0x4837, 0x30},
	{0x5002, 0x30},
	{0x5080, 0x00},
	{0x5081, 0x41},
	{0x0100, 0x00}

};


LOCAL const SENSOR_REG_T ov2680_640X480_mipi_raw[] = {
	{0x3086, 0x01},
	{0x3501, 0x26},
	{0x3502, 0x40},
	{0x3620, 0x26},
	{0x3621, 0x37},
	{0x3622, 0x04},
	{0x370a, 0x23},
	{0x370d, 0xc0},
	{0x3718, 0x88},
	{0x3721, 0x00},
	{0x3722, 0x00},
	{0x3723, 0x00},
	{0x3738, 0x00},
	{0x3800, 0x00},
	{0x3801, 0xa0},
	{0x3802, 0x00},
	{0x3803, 0x78},
	{0x3804, 0x05},
	{0x3805, 0xaf},
	{0x3806, 0x04},
	{0x3807, 0x47},
	{0x3808, 0x02},
	{0x3809, 0x80},
	{0x380a, 0x01},
	{0x380b, 0xe0},
	{0x380c, 0x06},
	{0x380d, 0xac},
	{0x380e, 0x02},
	{0x380f, 0x84},
	{0x3810, 0x00},
	{0x3811, 0x04},
	{0x3812, 0x00},
	{0x3813, 0x04},
	{0x3814, 0x31},
	{0x3815, 0x31},
#ifdef CONFIG_CAMERA_IMAGE_180
	{0x3820, 0xc2},
	{0x3821, 0x00},
#else
	{0x3820, 0xc6},
	{0x3821, 0x04},
#endif
	{0x4008, 0x00},
	{0x4009, 0x03},
	{0x4837, 0x30},
	{0x0100, 0x00}

};


LOCAL const SENSOR_REG_T ov2680_800X600_mipi_raw[] = {
	{0x0103, 0x01},
	{0x3002, 0x00},
	{0x3016, 0x1c},
	{0x3018, 0x44},
	{0x3020, 0x00},


	{0x3080, 0x02},
	{0x3082, 0x37},
	{0x3084, 0x09},
	{0x3085, 0x04},
	{0x3086, 0x01},

	{0x3501, 0x26},
	{0x3502, 0x40},
	{0x3503, 0x03},
	{0x350b, 0x36},
	{0x3600, 0xb4},
	{0x3603, 0x39},
	{0x3604, 0x24},
	{0x3605, 0x00},  //bit3:   1: use external regulator  0: use internal regulator
	{0x3620, 0x26},
	{0x3621, 0x37},
	{0x3622, 0x04},
	{0x3628, 0x00},
	{0x3705, 0x3c},
	{0x370c, 0x50},
	{0x370d, 0xc0},
	{0x3718, 0x88},
	{0x3720, 0x00},
	{0x3721, 0x00},
	{0x3722, 0x00},
	{0x3723, 0x00},
	{0x3738, 0x00},
	{0x370a, 0x23},
	{0x3717, 0x58},
	{0x3781, 0x80},
	{0x3784, 0x0c},
	{0x3789, 0x60},
	{0x3800, 0x00},
	{0x3801, 0x00},
	{0x3802, 0x00},
	{0x3803, 0x00},
	{0x3804, 0x06},
	{0x3805, 0x4f},
	{0x3806, 0x04},
	{0x3807, 0xbf},
	{0x3808, 0x03},
	{0x3809, 0x20},
	{0x380a, 0x02},
	{0x380b, 0x58},
	{0x380c, 0x06},//hts 1708 6ac  1710 6ae
	{0x380d, 0xae},
	{0x380e, 0x02}, //vts 644
	{0x380f, 0x84},
	{0x3810, 0x00},
	{0x3811, 0x04},
	{0x3812, 0x00},
	{0x3813, 0x04},
	{0x3814, 0x31},
	{0x3815, 0x31},
	{0x3819, 0x04},
#ifdef CONFIG_CAMERA_IMAGE_180
	{0x3820, 0xc2}, //bit[2] flip
	{0x3821, 0x00}, //bit[2] mirror
#else
	{0x3820, 0xc6}, //bit[2] flip
	{0x3821, 0x05}, //bit[2] mirror
#endif
	{0x4000, 0x81},
	{0x4001, 0x40},
	{0x4008, 0x00},
	{0x4009, 0x03},
	{0x4602, 0x02},
	{0x481f, 0x36},
	{0x4825, 0x36},
	{0x4837, 0x30},
	{0x5002, 0x30},
	{0x5080, 0x00},
	{0x5081, 0x41},
	{0x0100, 0x00}

};

LOCAL const SENSOR_REG_T ov2680_1600X1200_altek_mipi_raw[] = {
	{0x0103, 0x01},
	{0x3002, 0x00},
	{0x3016, 0x1c},
	{0x3018, 0x44},
	{0x3020, 0x00},
	{0x3080, 0x02},
	{0x3082, 0x37},
	{0x3084, 0x09},
	{0x3085, 0x04},
	{0x3086, 0x00},
	{0x3501, 0x4e},
	{0x3502, 0xe0},
	{0x3503, 0x03},
	{0x350b, 0x36},
	{0x3600, 0xb4},
	{0x3603, 0x35},
	{0x3604, 0x24},
	{0x3605, 0x00},
	{0x3620, 0x24},
	{0x3621, 0x34},
	{0x3622, 0x03},
	{0x3628, 0x00},
	{0x3701, 0x64},
	{0x3705, 0x3c},
	{0x370c, 0x50},
	{0x370d, 0xc0},
	{0x3718, 0x80},
	{0x3720, 0x00},
	{0x3721, 0x09},
	{0x3722, 0x06},
	{0x3723, 0x59},
	{0x3738, 0x99},
	{0x370a, 0x21},
	{0x3717, 0x58},
	{0x3781, 0x80},
	{0x3784, 0x0c},
	{0x3789, 0x60},
	{0x3800, 0x00},
	{0x3801, 0x00},
	{0x3802, 0x00},
	{0x3803, 0x00},
	{0x3804, 0x06},
	{0x3805, 0x4f},
	{0x3806, 0x04},
	{0x3807, 0xbf},
	{0x3808, 0x06},
	{0x3809, 0x40},
	{0x380a, 0x04},
	{0x380b, 0xb0},
	{0x380c, 0x06},
	{0x380d, 0xa4},
	{0x380e, 0x05},
	{0x380f, 0x0e},
	{0x3810, 0x00},
	{0x3811, 0x08},
	{0x3812, 0x00},
	{0x3813, 0x08},
	{0x3814, 0x11},
	{0x3815, 0x11},
	{0x3819, 0x04},
	{0x3820, 0xc0},
	{0x3821, 0x00},
	{0x4000, 0x81},
	{0x4001, 0x40},
	{0x4008, 0x02},
	{0x4009, 0x09},
	{0x4602, 0x02},
	{0x481f, 0x36},
	{0x4825, 0x36},
	{0x4837, 0x18},
	{0x5002, 0x30},
	{0x5080, 0x00},
	{0x5081, 0x41},
#if 0 // original setting , it clould output depth map
	{0x0100, 0x01},
	{0x0103, 0x01},
	{0x3002, 0x00},
	{0x3016, 0x1c},
	{0x3018, 0x44},
	{0x3020, 0x00},
	{0x3080, 0x02},
	{0x3082, 0x37},
	{0x3084, 0x09},
	{0x3085, 0x04},
	{0x3086, 0x01},
	{0x3501, 0x4e},
	{0x3502, 0xe0},
	{0x3503, 0x03},
	{0x350b, 0x36},
	{0x3600, 0xb4},
	{0x3603, 0x35},
	{0x3604, 0x24},
	{0x3605, 0x00},
	{0x3620, 0x24},
	{0x3621, 0x34},
	{0x3622, 0x03},
	{0x3628, 0x00},
	{0x3701, 0x64},
	{0x3705, 0x3c},
	{0x370c, 0x50},
	{0x370d, 0x00},
	{0x3718, 0x80},
	{0x3720, 0x00},
	{0x3721, 0x09},
	{0x3722, 0x0b},
	{0x3723, 0x48},
	{0x3738, 0x99},
	{0x370a, 0x21},
	{0x3717, 0x58},
	{0x3781, 0x80},
	{0x3784, 0x0c},
	{0x3789, 0x60},
	{0x3800, 0x00},
	{0x3801, 0x00},
	{0x3802, 0x00},
	{0x3803, 0x00},
	{0x3804, 0x06},
	{0x3805, 0x4f},
	{0x3806, 0x04},
	{0x3807, 0xbf},
	{0x3808, 0x06},
	{0x3809, 0x40},
	{0x380a, 0x04},
	{0x380b, 0xb0},
	{0x380c, 0x06},
	{0x380d, 0xa4},
	{0x380e, 0x05},
	{0x380f, 0x0e},
	{0x3810, 0x00},
	{0x3811, 0x08},
	{0x3812, 0x00},
	{0x3813, 0x08},
	{0x3814, 0x11},
	{0x3815, 0x11},
	{0x3819, 0x04},
	{0x3820, 0xc0},
	{0x3821, 0x00},
	{0x4000, 0x81},
	{0x4001, 0x40},
	{0x4008, 0x02},
	{0x4009, 0x09},
	{0x4602, 0x02},
	{0x481f, 0x36},
	{0x4825, 0x36},
	{0x4837, 0x30},
	{0x5002, 0x30},
	{0x5080, 0x00},
	{0x5081, 0x41},
#else // it is altek setting it can sync but d2 is not OK

	//{0x0100, 0x01},
	{0x5780, 0x3e},
	{0x5781, 0x0f},
	{0x5782, 0x04},
	{0x5783, 0x02},
	{0x5784, 0x01},
	{0x5785, 0x01},
	{0x5786, 0x00},
	{0x5787, 0x04},
	{0x5788, 0x02},
	{0x5789, 0x00},
	{0x578a, 0x01},
	{0x578b, 0x02},
	{0x578c, 0x03},
	{0x578d, 0x03},
	{0x578e, 0x08},
	{0x578f, 0x0c},
	{0x5790, 0x08},
	{0x5791, 0x04},
	{0x5792, 0x00},
	{0x5793, 0x00},
	{0x5794, 0x03},
#endif
#ifdef CONFIG_CAMERA_RE_FOCUS
	/*dual cam sync begin*/
	{0x3002, 0x00},
	{0x3823, 0x30},
	{0x3824, 0x00}, // cs
	{0x3825, 0x20},

	{0x3826, 0x00},
	{0x3827, 0x06},
	/*dual cam sync end*/
#endif

};


LOCAL const SENSOR_REG_T ov2680_1600X1200_mipi_raw[] = {
#if (SC_FPGA == 0)
	{0x3086, 0x00},
#else
	{0x3086, 0x02},
#endif
	{0x3501, 0x4e},
	{0x3502, 0xe0},
	{0x3620, 0x26},
	{0x3621, 0x37},
	{0x3622, 0x04},
	{0x370a, 0x21},
	{0x370d, 0xc0},
	{0x3718, 0x88},
	{0x3721, 0x00},
	{0x3722, 0x00},
	{0x3723, 0x00},
	{0x3738, 0x00},
	{0x3800, 0x00},
	{0x3801, 0x00},
	{0x3802, 0x00},
	{0x3803, 0x00},
	{0x3804, 0x06},
	{0x3805, 0x4f},
	{0x3806, 0x04},
	{0x3807, 0xbf},
	{0x3808, 0x06},
	{0x3809, 0x40},
	{0x380a, 0x04},
	{0x380b, 0xb0},
	{0x380c, 0x06},
	{0x380d, 0xa4},
#if (SC_FPGA == 0)
	{0x380e, 0x05},
	{0x380f, 0x0e},
#else
	{0x380e, 0x0a},
	{0x380f, 0x1c},
#endif
	{0x3811, 0x08},
	{0x3813, 0x08},
	{0x3814, 0x11},
	{0x3815, 0x11},
#ifdef CONFIG_CAMERA_IMAGE_180
	{0x3820, 0xc0},
	{0x3821, 0x00},
#else
	{0x3820, 0xc4},
	{0x3821, 0x04},
#endif
	{0x4008, 0x02},
	{0x4009, 0x09},
	{0x481b, 0x43},
	{0x4837, 0x18},

};

LOCAL SENSOR_REG_TAB_INFO_T s_ov2680_resolution_Tab_RAW[] = {
	{ADDR_AND_LEN_OF_ARRAY(ov2680_com_mipi_raw), 0, 0, 24, SENSOR_IMAGE_FORMAT_RAW},
	//{ADDR_AND_LEN_OF_ARRAY(ov2680_800X600_mipi_raw), 800, 600, 24, SENSOR_IMAGE_FORMAT_RAW},
#ifdef CONFIG_CAMERA_RE_FOCUS
	{ADDR_AND_LEN_OF_ARRAY(ov2680_1600X1200_altek_mipi_raw), 1600, 1200, 24, SENSOR_IMAGE_FORMAT_RAW},
#else
	{ADDR_AND_LEN_OF_ARRAY(ov2680_1600X1200_mipi_raw), 1600, 1200, 24, SENSOR_IMAGE_FORMAT_RAW},
#endif
	{PNULL, 0, 0, 0, 0, 0},
	//{PNULL, 0, 800, 600, 24, SENSOR_IMAGE_FORMAT_RAW},
	//{ADDR_AND_LEN_OF_ARRAY(ov2680_640x480_mipi_raw), 640, 480, 24, SENSOR_IMAGE_FORMAT_RAW},
	//{ADDR_AND_LEN_OF_ARRAY(ov2680_1600x1200_mipi_raw), 1600, 1200, 24, SENSOR_IMAGE_FORMAT_RAW},

	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0}
};

LOCAL SENSOR_TRIM_T s_ov2680_Resolution_Trim_Tab[] = {
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	//{0, 0, 800, 600, 51800, 330, 644, {0, 0, 800, 600}},
	{0, 0, 1600, 1200, 25758, 628, 1294, {0, 0, 1600, 1200}},//25757.6ns
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}}
};

LOCAL const SENSOR_REG_T s_ov2680_640X480_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
	/*video mode 0: ?fps*/
	{
		{0xffff, 0xff}
	},
	/* video mode 1:?fps*/
	{
		{0xffff, 0xff}
	},
	/* video mode 2:?fps*/
	{
		{0xffff, 0xff}
	},
	/* video mode 3:?fps*/
	{
		{0xffff, 0xff}
	}
};
LOCAL const SENSOR_REG_T s_ov2680_1600X1200_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
	/*video mode 0: ?fps*/
	{
		{0xffff, 0xff}
	},
	/* video mode 1:?fps*/
	{
		{0xffff, 0xff}
	},
	/* video mode 2:?fps*/
	{
		{0xffff, 0xff}
	},
	/* video mode 3:?fps*/
	{
		{0xffff, 0xff}
	}
};

LOCAL SENSOR_VIDEO_INFO_T s_ov2680_video_info[] = {
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	//{{{30, 30, 284, 90}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},(SENSOR_REG_T**)s_ov2680_640X480_video_tab},
	{{{25, 25, 258, 64}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},(SENSOR_REG_T**)s_ov2680_1600X1200_video_tab},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL}
};

LOCAL unsigned long _ov2680_set_video_mode(SENSOR_HW_HANDLE handle, unsigned long param)
{
	SENSOR_REG_T_PTR sensor_reg_ptr;
	uint16_t         i = 0x00;
	uint32_t         mode;

	if (param >= SENSOR_VIDEO_MODE_MAX)
		return 0;

	if (SENSOR_SUCCESS != Sensor_GetMode(&mode)) {
		SENSOR_PRINT("fail.");
		return SENSOR_FAIL;
	}

	if (PNULL == s_ov2680_video_info[mode].setting_ptr) {
		SENSOR_PRINT("fail.");
		return SENSOR_FAIL;
	}

	sensor_reg_ptr = (SENSOR_REG_T_PTR)&s_ov2680_video_info[mode].setting_ptr[param];
	if (PNULL == sensor_reg_ptr) {
		SENSOR_PRINT("fail.");
		return SENSOR_FAIL;
	}

	for (i=0x00; (0xffff!=sensor_reg_ptr[i].reg_addr)||(0xff!=sensor_reg_ptr[i].reg_value); i++) {
		Sensor_WriteReg(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
	}

	SENSOR_PRINT("0x%02x", param);
	return 0;
}

LOCAL SENSOR_IOCTL_FUNC_TAB_T s_ov2680_ioctl_func_tab = {
	PNULL,
	_ov2680_PowerOn,
	PNULL,
	_ov2680_Identify,

	PNULL,// write register
	PNULL,// read  register
	PNULL,
	_ov2680_GetResolutionTrimTab,
	PNULL,
	PNULL,
	PNULL,

	PNULL, //_ov2680_set_brightness,
	PNULL, // _ov2680_set_contrast,
	PNULL,
	PNULL,//_ov2680_set_saturation,

	PNULL, //_ov2680_set_work_mode,
	PNULL, //_ov2680_set_image_effect,

	_ov2680_BeforeSnapshot,
	_ov2680_after_snapshot,
	PNULL,   //_ov2680_flash,
	PNULL,
	PNULL,//_ov2680_write_exposure,
	PNULL,
	PNULL,//_ov2680_write_gain,
	PNULL,
	PNULL,
	PNULL,//_ov2680_write_af,
	PNULL,
	PNULL, //_ov2680_set_awb,
	PNULL,
	PNULL,
	PNULL, //_ov2680_set_ev,
	PNULL,
	PNULL,
	PNULL,
	PNULL, //_ov2680_GetExifInfo,
	_ov2680_ExtFunc,
	PNULL, //_ov2680_set_anti_flicker,
	_ov2680_set_video_mode,
	PNULL, //pick_jpeg_stream
	PNULL,  //meter_mode
	PNULL, //get_status
	_ov2680_StreamOn,
	_ov2680_StreamOff,
	_ov2680_access_val,//_ov2680_access_val
	//_ov2680_ex_write_exposure,
};

static SENSOR_STATIC_INFO_T s_ov2680_static_info = {
	240,	//f-number,focal ratio
	200,	//focal_length;
	0,	//max_fps,max fps of sensor's all settings,it will be calculated from sensor mode fps
	8,	//max_adgain,AD-gain
	0,	//ois_supported;
	0,	//pdaf_supported;
	1,	//exp_valid_frame_num;N+2-1
	16,	//clamp_level,black level
	0,	//adgain_valid_frame_num;N+1-1
};

static SENSOR_MODE_FPS_INFO_T s_ov2680_mode_fps_info = {
	0,	//is_init;
	{{SENSOR_MODE_COMMON_INIT,0,1,0,0},
	{SENSOR_MODE_PREVIEW_ONE,0,1,0,0},
	{SENSOR_MODE_SNAPSHOT_ONE_FIRST,0,1,0,0},
	{SENSOR_MODE_SNAPSHOT_ONE_SECOND,0,1,0,0},
	{SENSOR_MODE_SNAPSHOT_ONE_THIRD,0,1,0,0},
	{SENSOR_MODE_PREVIEW_TWO,0,1,0,0},
	{SENSOR_MODE_SNAPSHOT_TWO_FIRST,0,1,0,0},
	{SENSOR_MODE_SNAPSHOT_TWO_SECOND,0,1,0,0},
	{SENSOR_MODE_SNAPSHOT_TWO_THIRD,0,1,0,0}}
};

SENSOR_INFO_T g_ov2680_mipi_raw_info = {
	ov2680_I2C_ADDR_W,	// salve i2c write address
	ov2680_I2C_ADDR_R,	// salve i2c read address

	SENSOR_I2C_REG_16BIT | SENSOR_I2C_REG_8BIT|SENSOR_I2C_FREQ_100,	// bit0: 0: i2c register value is 8 bit, 1: i2c register value is 16 bit
	// bit1: 0: i2c register addr  is 8 bit, 1: i2c register addr  is 16 bit
	// other bit: reseved
	SENSOR_HW_SIGNAL_PCLK_N | SENSOR_HW_SIGNAL_VSYNC_N | SENSOR_HW_SIGNAL_HSYNC_P,	// bit0: 0:negative; 1:positive -> polarily of pixel clock
	// bit2: 0:negative; 1:positive -> polarily of horizontal synchronization signal
	// bit4: 0:negative; 1:positive -> polarily of vertical synchronization signal
	// other bit: reseved

	// preview mode
	SENSOR_ENVIROMENT_NORMAL | SENSOR_ENVIROMENT_NIGHT,

	// image effect
	SENSOR_IMAGE_EFFECT_NORMAL |
	    SENSOR_IMAGE_EFFECT_BLACKWHITE |
	    SENSOR_IMAGE_EFFECT_RED |
	    SENSOR_IMAGE_EFFECT_GREEN |
	    SENSOR_IMAGE_EFFECT_BLUE |
	    SENSOR_IMAGE_EFFECT_YELLOW |
	    SENSOR_IMAGE_EFFECT_NEGATIVE | SENSOR_IMAGE_EFFECT_CANVAS,

	// while balance mode
	0,

	7,			// bit[0:7]: count of step in brightness, contrast, sharpness, saturation
	// bit[8:31] reseved

	SENSOR_LOW_PULSE_RESET,	// reset pulse level
	50,			// reset pulse width(ms)

	SENSOR_LOW_LEVEL_PWDN,	// 1: high level valid; 0: low level valid

	1,			// count of identify code
	{{0x0A, 0x26},		// supply two code to identify sensor.
	 {0x0B, 0x80}},		// for Example: index = 0-> Device id, index = 1 -> version id

	SENSOR_AVDD_2800MV,	// voltage of avdd

	1600,			// max width of source image
	1200,			// max height of source image
	"ov2680_mipi_raw",		// name of sensor

	SENSOR_IMAGE_FORMAT_RAW,	// define in SENSOR_IMAGE_FORMAT_E enum,SENSOR_IMAGE_FORMAT_MAX
	// if set to SENSOR_IMAGE_FORMAT_MAX here, image format depent on SENSOR_REG_TAB_INFO_T

	SENSOR_IMAGE_PATTERN_RAWRGB_R,// pattern of input image form sensor;

	s_ov2680_resolution_Tab_RAW,	// point to resolution table information structure
	&s_ov2680_ioctl_func_tab,	// point to ioctl function table
	&s_ov2680_mipi_raw_info_ptr,		// information and table about Rawrgb sensor
	NULL,			//&g_ov2680_ext_info,                // extend information about sensor
	SENSOR_AVDD_1800MV,	// iovdd
	SENSOR_AVDD_1500MV,	// dvdd
	1,			// skip frame num before preview
	0,			// skip frame num before capture
	0,			// deci frame num during preview
	0,			// deci frame num during video preview

	0,
	0,
	0,
	0,
	0,
	{SENSOR_INTERFACE_TYPE_CSI2, 1, 10, 0},
	s_ov2680_video_info,
	1,			// skip frame num while change setting
	48,			//horizontal_view_angle,need check
	48,			//vertical_view_angle,need check
	"ov2680v1"	//sensor version info
};

LOCAL struct sensor_raw_info* Sensor_GetContext(SENSOR_HW_HANDLE handle)
{
	return s_ov2680_mipi_raw_info_ptr;
}


LOCAL uint32_t Sensor_InitRawTuneInfo(SENSOR_HW_HANDLE handle)
{
	uint32_t rtn=0x00;
	return rtn;
}

LOCAL unsigned long _ov2680_GetResolutionTrimTab(SENSOR_HW_HANDLE handle, unsigned long param)
{
	SENSOR_PRINT("0x%lx", (unsigned long)s_ov2680_Resolution_Trim_Tab);
	return (unsigned long) s_ov2680_Resolution_Trim_Tab;
}

LOCAL uint32_t _ov2680_init_mode_fps_info(SENSOR_HW_HANDLE handle)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_PRINT("_ov2680_init_mode_fps_info:E");
	if(!s_ov2680_mode_fps_info.is_init) {
		uint32_t i,modn,tempfps = 0;
		SENSOR_PRINT("_ov2680_init_mode_fps_info:start init");
		for(i = 0;i < NUMBER_OF_ARRAY(s_ov2680_Resolution_Trim_Tab); i++) {
			//max fps should be multiple of 30,it calulated from line_time and frame_line
			tempfps = s_ov2680_Resolution_Trim_Tab[i].line_time*s_ov2680_Resolution_Trim_Tab[i].frame_line;
			if(0 != tempfps) {
				tempfps = 1000000000/tempfps;
				modn = tempfps / 30;
				if(tempfps > modn*30)
					modn++;
				s_ov2680_mode_fps_info.sensor_mode_fps[i].max_fps = modn*30;
				if(s_ov2680_mode_fps_info.sensor_mode_fps[i].max_fps > 30) {
					s_ov2680_mode_fps_info.sensor_mode_fps[i].is_high_fps = 1;
					s_ov2680_mode_fps_info.sensor_mode_fps[i].high_fps_skip_num =
						s_ov2680_mode_fps_info.sensor_mode_fps[i].max_fps/30;
				}
				if(s_ov2680_mode_fps_info.sensor_mode_fps[i].max_fps >
						s_ov2680_static_info.max_fps) {
					s_ov2680_static_info.max_fps =
						s_ov2680_mode_fps_info.sensor_mode_fps[i].max_fps;
				}
			}
			SENSOR_PRINT("mode %d,tempfps %d,frame_len %d,line_time: %d ",i,tempfps,
					s_ov2680_Resolution_Trim_Tab[i].frame_line,
					s_ov2680_Resolution_Trim_Tab[i].line_time);
			SENSOR_PRINT("mode %d,max_fps: %d ",
					i,s_ov2680_mode_fps_info.sensor_mode_fps[i].max_fps);
			SENSOR_PRINT("is_high_fps: %d,highfps_skip_num %d",
					s_ov2680_mode_fps_info.sensor_mode_fps[i].is_high_fps,
					s_ov2680_mode_fps_info.sensor_mode_fps[i].high_fps_skip_num);
		}
		s_ov2680_mode_fps_info.is_init = 1;
	}
	SENSOR_PRINT("_ov2680_init_mode_fps_info:X");
	return rtn;
}

LOCAL unsigned long _ov2680_PowerOn(SENSOR_HW_HANDLE handle, unsigned long power_on)
{
	SENSOR_AVDD_VAL_E dvdd_val = g_ov2680_mipi_raw_info.dvdd_val;
	SENSOR_AVDD_VAL_E avdd_val = g_ov2680_mipi_raw_info.avdd_val;
	SENSOR_AVDD_VAL_E iovdd_val = g_ov2680_mipi_raw_info.iovdd_val;
	BOOLEAN power_down = g_ov2680_mipi_raw_info.power_down_level;
	BOOLEAN reset_level = g_ov2680_mipi_raw_info.reset_pulse_level;

	if (SENSOR_TRUE == power_on) {
		Sensor_PowerDown(power_down);
		Sensor_SetResetLevel(reset_level);
		Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);
		usleep(1000);
		// Open power
		Sensor_SetAvddVoltage(avdd_val);
		Sensor_SetIovddVoltage(iovdd_val);
		usleep(1000);
		//_dw9174_SRCInit(2);
		Sensor_PowerDown(!power_down);
		usleep(3*1000);  // > 8192*MCLK + 1ms
		// Reset sensor
		Sensor_SetResetLevel(!reset_level);
		//usleep(20*1000);
	} else {
		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		usleep(1000); // >512 	mclk
		Sensor_PowerDown(power_down);
		Sensor_SetResetLevel(reset_level);
		usleep(500);
		Sensor_SetAvddVoltage(SENSOR_AVDD_CLOSED);
		Sensor_SetIovddVoltage(SENSOR_AVDD_CLOSED);
	}
	SENSOR_PRINT("SENSOR_ov2680: _ov2680_Power_On(1:on, 0:off): %d", power_on);
	return SENSOR_SUCCESS;
}

LOCAL unsigned long _ov2680_cfg_otp(SENSOR_HW_HANDLE handle, unsigned long  param)
{
	uint32_t rtn=SENSOR_SUCCESS;
	struct raw_param_info_tab* tab_ptr = (struct raw_param_info_tab*)s_ov2680_raw_param_tab;
	uint32_t module_id=g_module_id;

	SENSOR_PRINT("SENSOR_ov2680: _ov2680_cfg_otp");

	if(PNULL!=tab_ptr[module_id].cfg_otp){
		tab_ptr[module_id].cfg_otp(handle ,0);
		}

	return rtn;
}

// index: index of otp group. (1, 2, 3)
// return: 0, group index is empty
// 1, group index has invalid data
// 2, group index has valid data
LOCAL uint32_t check_otp(SENSOR_HW_HANDLE handle, int index)
{
	uint32_t flag = 0;
	uint32_t i = 0;
	uint32_t rg = 0;
	uint32_t bg = 0;

	if (index == 1)
	{
		// read otp --Bank 0
		Sensor_WriteReg(0x3d84, 0xc0);
		Sensor_WriteReg(0x3d85, 0x00);
		Sensor_WriteReg(0x3d86, 0x0f);
		Sensor_WriteReg(0x3d81, 0x01);
		usleep(5 * 1000);
		flag = Sensor_ReadReg(0x3d05);
		rg = Sensor_ReadReg(0x3d07);
		bg = Sensor_ReadReg(0x3d08);
	}
	else if (index == 2)
	{
		// read otp --Bank 0
		Sensor_WriteReg(0x3d84, 0xc0);
		Sensor_WriteReg(0x3d85, 0x00);
		Sensor_WriteReg(0x3d86, 0x0f);
		Sensor_WriteReg(0x3d81, 0x01);
		usleep(5 * 1000);
		flag = Sensor_ReadReg(0x3d0e);
		// read otp --Bank 1
		Sensor_WriteReg(0x3d84, 0xc0);
		Sensor_WriteReg(0x3d85, 0x10);
		Sensor_WriteReg(0x3d86, 0x1f);
		Sensor_WriteReg(0x3d81, 0x01);
		usleep(5 * 1000);
		rg = Sensor_ReadReg(0x3d00);
		bg = Sensor_ReadReg(0x3d01);
	}
	else if (index == 3)
	{
		// read otp --Bank 1
		Sensor_WriteReg(0x3d84, 0xc0);
		Sensor_WriteReg(0x3d85, 0x10);
		Sensor_WriteReg(0x3d86, 0x1f);
		Sensor_WriteReg(0x3d81, 0x01);
		usleep(5 * 1000);
		flag = Sensor_ReadReg(0x3d07);
		rg = Sensor_ReadReg(0x3d09);
		bg = Sensor_ReadReg(0x3d0a);
	}
	SENSOR_PRINT("ov2680 check_otp: flag = 0x%d----index = %d---\n", flag, index);
	flag = flag & 0x80;
	// clear otp buffer
	for (i=0; i<16; i++) {
		Sensor_WriteReg(0x3d00 + i, 0x00);
	}
	SENSOR_PRINT("ov2680 check_otp: flag = 0x%d  rg = 0x%x, bg = 0x%x,-------\n", flag, rg, bg);
	if (flag) {
		return 1;
	}
	else
	{
		if (rg == 0 && bg == 0)
		{
			return 0;
		}
		else
		{
			return 2;
		}
	}
}
// index: index of otp group. (1, 2, 3)
// return: 0,
static int read_otp(SENSOR_HW_HANDLE handle, int index, struct otp_struct *otp_ptr)
{
	int i = 0;
	int temp = 0;
	// read otp into buffer
	if (index == 1)
	{
		// read otp --Bank 0
		Sensor_WriteReg(0x3d84, 0xc0);
		Sensor_WriteReg(0x3d85, 0x00);
		Sensor_WriteReg(0x3d86, 0x0f);
		Sensor_WriteReg(0x3d81, 0x01);
		usleep(5 * 1000);
		(*otp_ptr).module_integrator_id = (Sensor_ReadReg(0x3d05) & 0x7f);
		(*otp_ptr).lens_id = Sensor_ReadReg(0x3d06);
		temp = Sensor_ReadReg(0x3d0b);
		(*otp_ptr).rg_ratio = (Sensor_ReadReg(0x3d07)<<2) + ((temp>>6) & 0x03);
		(*otp_ptr).bg_ratio = (Sensor_ReadReg(0x3d08)<<2) + ((temp>>4) & 0x03);
		(*otp_ptr).light_rg = ((Sensor_ReadReg(0x3d0c)<<2) + (temp>>2)) & 0x03;
		(*otp_ptr).light_bg = ((Sensor_ReadReg(0x3d0d)<<2) + temp) & 0x03;

		(*otp_ptr).user_data[0] = Sensor_ReadReg(0x3d09);
		(*otp_ptr).user_data[1] = Sensor_ReadReg(0x3d0a);
	}
	else if (index == 2)
	{
		// read otp --Bank 0
		Sensor_WriteReg(0x3d84, 0xc0);
		Sensor_WriteReg(0x3d85, 0x00);
		Sensor_WriteReg(0x3d86, 0x0f);
		Sensor_WriteReg(0x3d81, 0x01);
		usleep(5 * 1000);
		(*otp_ptr).module_integrator_id = (Sensor_ReadReg(0x3d0e) & 0x7f);
		(*otp_ptr).lens_id = Sensor_ReadReg(0x3d0f);
		// read otp --Bank 1
		Sensor_WriteReg(0x3d84, 0xc0);
		Sensor_WriteReg(0x3d85, 0x10);
		Sensor_WriteReg(0x3d86, 0x1f);
		Sensor_WriteReg(0x3d81, 0x01);
		usleep(5 * 1000);
		temp = Sensor_ReadReg(0x3d04);
		(*otp_ptr).rg_ratio = (Sensor_ReadReg(0x3d00)<<2) + ((temp>>6) & 0x03);
		(*otp_ptr).bg_ratio = (Sensor_ReadReg(0x3d01)<<2) + ((temp>>4) & 0x03);
		(*otp_ptr).light_rg = ((Sensor_ReadReg(0x3d05)<<2) + (temp>>2)) & 0x03;
		(*otp_ptr).light_bg = ((Sensor_ReadReg(0x3d06)<<2) + temp) & 0x03;
		(*otp_ptr).user_data[0] = Sensor_ReadReg(0x3d02);
		(*otp_ptr).user_data[1] = Sensor_ReadReg(0x3d03);
	}
	else if (index == 3)
	{
		// read otp --Bank 1
		Sensor_WriteReg(0x3d84, 0xc0);
		Sensor_WriteReg(0x3d85, 0x10);
		Sensor_WriteReg(0x3d86, 0x1f);
		Sensor_WriteReg(0x3d81, 0x01);
		usleep(5 * 1000);
		(*otp_ptr).module_integrator_id = (Sensor_ReadReg(0x3d07) & 0x7f);
		(*otp_ptr).lens_id = Sensor_ReadReg(0x3d08);
		temp = Sensor_ReadReg(0x3d0d);
		(*otp_ptr).rg_ratio = (Sensor_ReadReg(0x3d09)<<2) + ((temp>>6) & 0x03);
		(*otp_ptr).bg_ratio = (Sensor_ReadReg(0x3d0a)<<2) + ((temp>>4) & 0x03);
		(*otp_ptr).light_rg = ((Sensor_ReadReg(0x3d0e)<<2) + (temp>>2)) & 0x03;
		(*otp_ptr).light_bg = ((Sensor_ReadReg(0x3d0f)<<2) + temp) & 0x03;
		(*otp_ptr).user_data[0] = Sensor_ReadReg(0x3d0b);
		(*otp_ptr).user_data[1] = Sensor_ReadReg(0x3d0c);
	}

	// clear otp buffer
	for (i=0;i<16;i++) {
		Sensor_WriteReg(0x3d00 + i, 0x00);
	}

	return 0;
}
// R_gain, sensor red gain of AWB, 0x400 =1
// G_gain, sensor green gain of AWB, 0x400 =1
// B_gain, sensor blue gain of AWB, 0x400 =1
// return 0;
static int update_awb_gain(SENSOR_HW_HANDLE handle, int R_gain, int G_gain, int B_gain)
{
	if (R_gain>0x400) {
		Sensor_WriteReg(0x5186, R_gain>>8);
		Sensor_WriteReg(0x5187, R_gain & 0x00ff);
	}
	if (G_gain>0x400) {
		Sensor_WriteReg(0x5188, G_gain>>8);
		Sensor_WriteReg(0x5189, G_gain & 0x00ff);
	}
	if (B_gain>0x400) {
		Sensor_WriteReg(0x518a, B_gain>>8);
		Sensor_WriteReg(0x518b, B_gain & 0x00ff);
	}
	return 0;
}
// call this function after ov2680 initialization
// return: 0 update success
// 1, no OTP
LOCAL uint32_t update_otp (SENSOR_HW_HANDLE handle, void* param_ptr)
{
	struct otp_struct current_otp;
	int i = 0;
	int otp_index = 0;
	int temp = 0;
	int R_gain, G_gain, B_gain, G_gain_R, G_gain_B;
	int rg = 0;
	int bg = 0;
	uint16_t stream_value = 0;
	uint16_t reg_value = 0;

	stream_value = Sensor_ReadReg(0x0100);
	SENSOR_PRINT("ov2680 update_otp:stream_value = 0x%x\n", stream_value);
	if(1 != (stream_value & 0x01))
	{
		Sensor_WriteReg(0x0100, 0x01);
		usleep(50 * 1000);
	}

	// R/G and B/G of current camera module is read out from sensor OTP
	// check first OTP with valid data
	for(i=1;i<=3;i++) {
		temp = check_otp(handle, i);
		if (temp == 2) {
			otp_index = i;
			break;
		}
	}
	if (i>3) {
		// no valid wb OTP data
		return 1;
	}
	read_otp(handle, otp_index, &current_otp);
	if(current_otp.light_rg==0) {
		// no light source information in OTP
		rg = current_otp.rg_ratio;
	}
	else {
		// light source information found in OTP
		rg = current_otp.rg_ratio * (current_otp.light_rg +512) / 1024;
	}
	if(current_otp.light_bg==0) {
		// no light source information in OTP
		bg = current_otp.bg_ratio;
	}
	else {
		// light source information found in OTP
		bg = current_otp.bg_ratio * (current_otp.light_bg +512) / 1024;
	}
	//calculate G gain
	//0x400 = 1x gain
	if(bg < BG_Ratio_Typical) {
		if (rg< RG_Ratio_Typical) {
			// current_otp.bg_ratio < BG_Ratio_typical &&
			// current_otp.rg_ratio < RG_Ratio_typical
			if((0 == bg) || (0 == rg)){
				SENSOR_PRINT("ov2680_otp: update otp failed!!, bg = %d, rg = %d\n", bg, rg);
				return 0;
			}
			G_gain = 0x400;
			B_gain = 0x400 * BG_Ratio_Typical / bg;
			R_gain = 0x400 * RG_Ratio_Typical / rg;
		}
		else {
			if(0 == bg){
				SENSOR_PRINT("ov2680_otp: update otp failed!!, bg = %d\n", bg);
				return 0;
			}
			// current_otp.bg_ratio < BG_Ratio_typical &&
			// current_otp.rg_ratio >= RG_Ratio_typical
			R_gain = 0x400;
			G_gain = 0x400 * rg / RG_Ratio_Typical;
			B_gain = G_gain * BG_Ratio_Typical /bg;
		}
	}
	else {
		if (rg < RG_Ratio_Typical) {
			// current_otp.bg_ratio >= BG_Ratio_typical &&
			// current_otp.rg_ratio < RG_Ratio_typical
			if(0 == rg){
				SENSOR_PRINT("ov2680_otp: update otp failed!!, rg = %d\n", rg);
				return 0;
			}
			B_gain = 0x400;
			G_gain = 0x400 * bg / BG_Ratio_Typical;
			R_gain = G_gain * RG_Ratio_Typical / rg;
		}
		else {
			// current_otp.bg_ratio >= BG_Ratio_typical &&
			// current_otp.rg_ratio >= RG_Ratio_typical
			G_gain_B = 0x400 * bg / BG_Ratio_Typical;
			G_gain_R = 0x400 * rg / RG_Ratio_Typical;
			if(G_gain_B > G_gain_R ) {
				if(0 == rg){
					SENSOR_PRINT("ov2680_otp: update otp failed!!, rg = %d\n", rg);
					return 0;
				}
				B_gain = 0x400;
				G_gain = G_gain_B;
				R_gain = G_gain * RG_Ratio_Typical /rg;
			}
			else {
				if(0 == bg){
					SENSOR_PRINT("ov2680_otp: update otp failed!!, bg = %d\n", bg);
					return 0;
				}
				R_gain = 0x400;
				G_gain = G_gain_R;
				B_gain = G_gain * BG_Ratio_Typical / bg;
			}
		}
	}
	update_awb_gain(handle, R_gain, G_gain, B_gain);

	if(1 != (stream_value & 0x01))
		Sensor_WriteReg(0x0100, stream_value);

	SENSOR_PRINT("ov2680_otp: R_gain:0x%x, G_gain:0x%x, B_gain:0x%x------\n",R_gain, G_gain, B_gain);
	return 0;
}

LOCAL uint32_t ov2680_check_otp_module_id(SENSOR_HW_HANDLE handle)
{
	struct otp_struct current_otp;
	int i = 0;
	int otp_index = 0;
	int temp = 0;
	uint16_t stream_value = 0;

	stream_value = Sensor_ReadReg(0x0100);
	SENSOR_PRINT("ov2680_check_otp_module_id:stream_value = 0x%x\n", stream_value);
	if(1 != (stream_value & 0x01))
	{
		Sensor_WriteReg(0x0100, 0x01);
		usleep(50 * 1000);
	}
	// R/G and B/G of current camera module is read out from sensor OTP
	// check first OTP with valid data

	for(i=1;i<=3;i++) {
		temp = check_otp(handle, i);
		SENSOR_PRINT("ov2680_check_otp_module_id i=%d temp = %d \n",i,temp);
		if (temp == 2) {
			otp_index = i;
			break;
		}
	}
	if (i > 3) {
		// no valid wb OTP data
		SENSOR_PRINT("ov2680_check_otp_module_id no valid wb OTP data\n");
		return 1;
	}

	read_otp(handle, otp_index, &current_otp);

	if(1 != (stream_value & 0x01))
		Sensor_WriteReg(0x0100, stream_value);

	SENSOR_PRINT("read ov2680 otp  module_id = %x \n", current_otp.module_integrator_id);

	return current_otp.module_integrator_id;
}

LOCAL uint32_t _ov2680_Truly_Identify_otp(SENSOR_HW_HANDLE handle, void* param_ptr)
{
	uint32_t rtn=SENSOR_FAIL;
	uint32_t param_id;

	SENSOR_PRINT("SENSOR_ov2680: _ov2680_Truly_Identify_otp");

	/*read param id from sensor omap*/
	param_id=ov2680_check_otp_module_id(handle);;

	if(ov2680_RAW_PARAM_Truly==param_id){
		SENSOR_PRINT("SENSOR_ov2680: This is Truly module!!\n");
		RG_Ratio_Typical = 0x152;
		BG_Ratio_Typical = 0x137;
		rtn=SENSOR_SUCCESS;
	}

	return rtn;
}

LOCAL uint32_t _ov2680_Sunny_Identify_otp(SENSOR_HW_HANDLE handle, void* param_ptr)
{
	uint32_t rtn=SENSOR_FAIL;
	uint32_t param_id;

	SENSOR_PRINT("SENSOR_ov2680: _ov2680_Sunny_Identify_otp");

	/*read param id from sensor omap*/
	param_id=ov2680_check_otp_module_id(handle);

	if(ov2680_RAW_PARAM_Sunny==param_id){
		SENSOR_PRINT("SENSOR_ov2680: This is Sunny module!!\n");
		RG_Ratio_Typical = 386;
		BG_Ratio_Typical = 367;
		rtn=SENSOR_SUCCESS;
	}

	return rtn;
}

LOCAL uint32_t _ov2680_com_Identify_otp(SENSOR_HW_HANDLE handle, void* param_ptr)
{
	uint32_t rtn=SENSOR_FAIL;
	uint32_t param_id;

	SENSOR_PRINT("SENSOR_ov2680: _ov2680_com_Identify_otp");

	/*read param id from sensor omap*/
	param_id=ov2680_RAW_PARAM_COM;

	if(ov2680_RAW_PARAM_COM==param_id){
		rtn=SENSOR_SUCCESS;
	}

	return rtn;
}

LOCAL uint32_t _ov2680_GetRawInof(SENSOR_HW_HANDLE handle)
{
	uint32_t rtn=SENSOR_SUCCESS;
	struct raw_param_info_tab* tab_ptr = (struct raw_param_info_tab*)s_ov2680_raw_param_tab;
	uint32_t param_id;
	uint32_t i=0x00;

	/*read param id from sensor omap*/
	param_id=ov2680_RAW_PARAM_COM;

	for(i=0x00; ; i++)
	{
		g_module_id = i;
		if(RAW_INFO_END_ID==tab_ptr[i].param_id){
			if(NULL==s_ov2680_mipi_raw_info_ptr){
				SENSOR_PRINT("SENSOR_ov2680: ov5647_GetRawInof no param error");
				rtn=SENSOR_FAIL;
			}
			SENSOR_PRINT("SENSOR_ov2680: ov2680_GetRawInof end");
			break;
		}
		else if(PNULL!=tab_ptr[i].identify_otp){
			if(SENSOR_SUCCESS==tab_ptr[i].identify_otp(handle, 0))
			{
				s_ov2680_mipi_raw_info_ptr = tab_ptr[i].info_ptr;
				SENSOR_PRINT("SENSOR_ov2680: ov2680_GetRawInof success");
				break;
			}
		}
	}

	return rtn;
}

LOCAL unsigned long _ov2680_GetMaxFrameLine(SENSOR_HW_HANDLE handle, unsigned long index)
{
	uint32_t max_line=0x00;
	SENSOR_TRIM_T_PTR trim_ptr=s_ov2680_Resolution_Trim_Tab;

	max_line=trim_ptr[index].frame_line;

	return max_line;
}

LOCAL unsigned long _ov2680_Identify(SENSOR_HW_HANDLE handle, unsigned long param)
{
#define ov2680_PID_VALUE    0x26
#define ov2680_PID_ADDR     0x300a
#define ov2680_VER_VALUE    0x80
#define ov2680_VER_ADDR     0x300b

	uint8_t pid_value = 0x00;
	uint8_t ver_value = 0x00;
	uint32_t ret_value = SENSOR_FAIL;

	SENSOR_PRINT("SENSOR_ov2680: mipi raw identify\n");
	//Sensor_WriteReg(0x0103, 0x01);
	//usleep(100*1000);
	pid_value = Sensor_ReadReg(ov2680_PID_ADDR);

	if (ov2680_PID_VALUE == pid_value) {
		ver_value = Sensor_ReadReg(ov2680_VER_ADDR);
		SENSOR_PRINT("SENSOR_ov2680: Identify: PID = %x, VER = %x", pid_value, ver_value);
		if (ov2680_VER_VALUE == ver_value) {
			ret_value=_ov2680_GetRawInof(handle);
			Sensor_InitRawTuneInfo(handle);
			_ov2680_init_mode_fps_info(handle);
			ret_value = SENSOR_SUCCESS;
			SENSOR_PRINT("SENSOR_ov2680: this is ov2680 sensor !");
		} else {
			SENSOR_PRINT
			    ("SENSOR_ov2680: Identify this is OV%x%x sensor !", pid_value, ver_value);
		}
	} else {
		SENSOR_PRINT("SENSOR_ov2680: identify fail,pid_value=%d", pid_value);
	}

	return ret_value;
}

LOCAL unsigned long _ov2680_write_exposure(SENSOR_HW_HANDLE handle, unsigned long param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t expsure_line=0x00;
	uint16_t dummy_line=0x00;
	uint16_t frame_len=0x00;
	uint16_t frame_len_cur=0x00;
	uint16_t max_frame_len=0x00;
	uint16_t size_index=0x00;
	uint16_t value=0x00;
	uint16_t value0=0x00;
	uint16_t value1=0x00;
	uint16_t value2=0x00;

	expsure_line=param&0xffff;
	dummy_line=(param>>0x10)&0xffff;
	size_index=(param>>0x1c)&0x0f;

	SENSOR_PRINT("SENSOR_ov2680: write_exposure line:%d, dummy:%d, size_index:%d\n", expsure_line, dummy_line, size_index);
	SENSOR_PRINT("SENSOR_ov2680: read reg :0x3820=%x\n", Sensor_ReadReg(0x3820));
	SENSOR_PRINT("SENSOR_ov2680: read reg :0x3821=%x\n", Sensor_ReadReg(0x3821));
	max_frame_len=_ov2680_GetMaxFrameLine(handle, size_index);

	if(0x00!=max_frame_len)
	{
		frame_len = ((expsure_line+dummy_line+4)> max_frame_len) ? (expsure_line+dummy_line+4) : max_frame_len;

		if(0x00!=(0x01&frame_len))
		{
			frame_len+=0x01;
		}

		frame_len_cur = (Sensor_ReadReg(0x380e)&0xff)<<8;
		frame_len_cur |= Sensor_ReadReg(0x380f)&0xff;

		if(frame_len_cur != frame_len){
			value=(frame_len)&0xff;
			ret_value = Sensor_WriteReg(0x380f, value);
			value=(frame_len>>0x08)&0xff;
			ret_value = Sensor_WriteReg(0x380e, value);
		}
	}

	value=(expsure_line<<0x04)&0xff;
	ret_value = Sensor_WriteReg(0x3502, value);
	value=(expsure_line>>0x04)&0xff;
	ret_value = Sensor_WriteReg(0x3501, value);
	value=(expsure_line>>0x0c)&0x0f;
	ret_value = Sensor_WriteReg(0x3500, value);

	return ret_value;
}

LOCAL unsigned long _ov2680_ex_write_exposure(unsigned long param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t exposure_line = 0x00;
	uint16_t dummy_line = 0x00;
	uint16_t size_index=0x00;
	struct sensor_ex_exposure  *ex = (struct sensor_ex_exposure*)param;
	float fps = 0.0;


	if (!param) {
		SENSOR_PRINT_ERR("param is NULL !!!");
		return ret_value;
	}
	exposure_line = ex->exposure;
	dummy_line = ex->dummy;
	size_index = ex->size_index;
	//if (s_exp == exposure_line && s_dummy == dummy_line)
	//	return 0;
	//s_exp = exposure_line;
	//s_dummy = dummy_line;
	//ret_value = _ov2680_write_exp_dummy(exposure_line, dummy_line, size_index);

	return ret_value;
}
LOCAL unsigned long _ov2680_write_gain(SENSOR_HW_HANDLE handle, unsigned long param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t value=0x00;
	uint32_t real_gain = 0;

	real_gain = ((param&0xf)+16)*(((param>>4)&0x01)+1)*(((param>>5)&0x01)+1)*(((param>>6)&0x01)+1)*(((param>>7)&0x01)+1);
	real_gain = real_gain*(((param>>8)&0x01)+1)*(((param>>9)&0x01)+1)*(((param>>10)&0x01)+1)*(((param>>11)&0x01)+1);

	SENSOR_PRINT("SENSOR_ov2680: real_gain:0x%x, param: 0x%x", real_gain, param);

	value = real_gain&0xff;
	ret_value = Sensor_WriteReg(0x350b, value);/*0-7*/
	value = (real_gain>>0x08)&0x03;
	ret_value = Sensor_WriteReg(0x350a, value);/*8*/

	return ret_value;
}

LOCAL unsigned long _ov2680_write_af(SENSOR_HW_HANDLE handle, unsigned long param)
{
#define DW9714_VCM_SLAVE_ADDR (0x18>>1)

	uint32_t ret_value = SENSOR_SUCCESS;
	uint8_t cmd_val[2] = {0x00};
	uint16_t  slave_addr = 0;
	uint16_t cmd_len = 0;

	SENSOR_PRINT("SENSOR_ov2680: _write_af %d", param);

	slave_addr = DW9714_VCM_SLAVE_ADDR;
	cmd_val[0] = (param&0xfff0)>>4;
	cmd_val[1] = ((param&0x0f)<<4)|0x09;
	cmd_len = 2;
	ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);

	SENSOR_PRINT("SENSOR_ov2680: _write_af, ret =  %d, MSL:%x, LSL:%x\n", ret_value, cmd_val[0], cmd_val[1]);

	return ret_value;
}

LOCAL unsigned long _ov2680_ReadGain(SENSOR_HW_HANDLE handle, unsigned long param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	uint16_t value=0x00;
	uint32_t gain = 0;

	value = Sensor_ReadReg(0x350b);/*0-7*/
	gain = value&0xff;
	value = Sensor_ReadReg(0x350a);/*8*/
	gain |= (value<<0x08)&0x300;

	s_ov2680_gain=(int)gain;

	SENSOR_PRINT("SENSOR_ov2680: _ov2680_ReadGain gain: 0x%x", s_ov2680_gain);

	return rtn;
}

LOCAL int _ov2680_set_gain16(SENSOR_HW_HANDLE handle, int gain16)
{
	// write gain, 16 = 1x
	int temp;
	gain16 = gain16 & 0x3ff;

	temp = gain16 & 0xff;
	Sensor_WriteReg(0x350b, temp);

	temp = gain16>>8;
	Sensor_WriteReg(0x350a, temp);
	SENSOR_PRINT("gain %d",gain16);

	return 0;
}

LOCAL int _ov2680_set_shutter(SENSOR_HW_HANDLE handle, int shutter)
{
	// write shutter, in number of line period
	int temp;

	shutter = shutter & 0xffff;

	temp = shutter & 0x0f;
	temp = temp<<4;
	Sensor_WriteReg(0x3502, temp);

	temp = shutter & 0xfff;
	temp = temp>>4;
	Sensor_WriteReg(0x3501, temp);

	temp = shutter>>12;
	Sensor_WriteReg(0x3500, temp);

	SENSOR_PRINT("shutter %d",shutter);

	return 0;
}

LOCAL void _calculate_hdr_exposure(SENSOR_HW_HANDLE handle, int capture_gain16,int capture_VTS, int capture_shutter)
{
	// write capture gain
	_ov2680_set_gain16(handle, capture_gain16);

	_ov2680_set_shutter(handle, capture_shutter);
}

LOCAL unsigned long _ov2680_SetEV(SENSOR_HW_HANDLE handle, unsigned long param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	uint16_t value=0x00;
	uint32_t gain = s_ov2680_gain;
	uint32_t ev = param;

	SENSOR_PRINT("_ov2680_SetEV param: 0x%x,0x%x,0x%x,0x%x", param, s_ov2680_gain,s_capture_VTS,s_capture_shutter);

	switch(ev) {
	case SENSOR_HDR_EV_LEVE_0:
		_calculate_hdr_exposure(handle, s_ov2680_gain/2,s_capture_VTS,s_capture_shutter/2);
		break;
	case SENSOR_HDR_EV_LEVE_1:
		_calculate_hdr_exposure(handle, s_ov2680_gain,s_capture_VTS,s_capture_shutter);
		break;
	case SENSOR_HDR_EV_LEVE_2:
		_calculate_hdr_exposure(handle, s_ov2680_gain,s_capture_VTS,s_capture_shutter*4);
		break;
	default:
		break;
	}
	return rtn;
}

LOCAL unsigned long _ov2680_ExtFunc(SENSOR_HW_HANDLE handle, unsigned long ctl_param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR) ctl_param;
	SENSOR_PRINT("cmd %d",ext_ptr->cmd);
	switch (ext_ptr->cmd) {
		case SENSOR_EXT_EV:
			rtn = _ov2680_SetEV(handle, ext_ptr->param);
			break;
		default:
			break;
	}

	return rtn;
}

int _ov2680_get_shutter(SENSOR_HW_HANDLE handle)
{
	// read shutter, in number of line period
	int shutter;

	shutter = (Sensor_ReadReg(0x03500) & 0x0f);
	shutter = (shutter<<8) + Sensor_ReadReg(0x3501);
	shutter = (shutter<<4) + (Sensor_ReadReg(0x3502)>>4);

	return shutter;
}

LOCAL int _ov2680_get_VTS(SENSOR_HW_HANDLE handle)
{
	// read VTS from register settings
	int VTS;

	VTS = Sensor_ReadReg(0x380e);//total vertical size[15:8] high byte

	VTS = (VTS<<8) + Sensor_ReadReg(0x380f);

	return VTS;
}

LOCAL unsigned long _ov2680_PreBeforeSnapshot(SENSOR_HW_HANDLE handle, unsigned long param)
{
	uint8_t ret_l, ret_m, ret_h;
	uint32_t capture_exposure, preview_maxline;
	uint32_t capture_maxline, preview_exposure;
	uint32_t capture_mode = param & 0xffff;
	uint32_t preview_mode = (param >> 0x10 ) & 0xffff;
	uint32_t prv_linetime = s_ov2680_Resolution_Trim_Tab[preview_mode].line_time;
	uint32_t cap_linetime = s_ov2680_Resolution_Trim_Tab[capture_mode].line_time;

	SENSOR_PRINT("SENSOR_ov2680: BeforeSnapshot mode: 0x%08x",param);

	if (preview_mode == capture_mode) {
		SENSOR_PRINT("SENSOR_ov2680: prv mode equal to capmode");
		goto CFG_INFO;
	}

	ret_h = (uint8_t) Sensor_ReadReg(0x3500);
	ret_m = (uint8_t) Sensor_ReadReg(0x3501);
	ret_l = (uint8_t) Sensor_ReadReg(0x3502);
	preview_exposure = (ret_h << 12) + (ret_m << 4) + (ret_l >> 4);

	ret_h = (uint8_t) Sensor_ReadReg(0x380e);
	ret_l = (uint8_t) Sensor_ReadReg(0x380f);
	preview_maxline = (ret_h << 8) + ret_l;

	Sensor_SetMode(capture_mode);
	Sensor_SetMode_WaitDone();

	if (prv_linetime == cap_linetime) {
		SENSOR_PRINT("SENSOR_ov2680: prvline equal to capline");
		goto CFG_INFO;
	}

	ret_h = (uint8_t) Sensor_ReadReg(0x380e);
	ret_l = (uint8_t) Sensor_ReadReg(0x380f);
	capture_maxline = (ret_h << 8) + ret_l;

	capture_exposure = preview_exposure * prv_linetime/cap_linetime;

	if(0 == capture_exposure){
		capture_exposure = 1;
	}

	if(capture_exposure > (capture_maxline - 4)){
		capture_maxline = capture_exposure + 4;
		capture_maxline = (capture_maxline+1)>>1<<1;
		ret_l = (unsigned char)(capture_maxline&0x0ff);
		ret_h = (unsigned char)((capture_maxline >> 8)&0xff);
		Sensor_WriteReg(0x380e, ret_h);
		Sensor_WriteReg(0x380f, ret_l);
	}

	ret_l = ((unsigned char)capture_exposure&0xf) << 4;
	ret_m = (unsigned char)((capture_exposure&0xfff) >> 4) & 0xff;
	ret_h = (unsigned char)(capture_exposure >> 12);

	Sensor_WriteReg(0x3502, ret_l);
	Sensor_WriteReg(0x3501, ret_m);
	Sensor_WriteReg(0x3500, ret_h);

	CFG_INFO:
	s_capture_shutter = _ov2680_get_shutter(handle);
	s_capture_VTS = _ov2680_get_VTS(handle);
	_ov2680_ReadGain(handle, capture_mode);
	Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_EXPOSURETIME, s_capture_shutter);

	return SENSOR_SUCCESS;
}

LOCAL unsigned long _ov2680_BeforeSnapshot(SENSOR_HW_HANDLE handle, unsigned long param)
{
	uint32_t cap_mode = (param>>CAP_MODE_BITS);
	uint32_t rtn = SENSOR_SUCCESS;

	SENSOR_PRINT("%d,%d.",cap_mode,param);

	rtn = _ov2680_PreBeforeSnapshot(handle, param);

	return rtn;
}

LOCAL unsigned long _ov2680_after_snapshot(SENSOR_HW_HANDLE handle, unsigned long param)
{
	SENSOR_PRINT("SENSOR_ov2680: after_snapshot mode:%ld", param);
	Sensor_SetMode((uint32_t)param);

	return SENSOR_SUCCESS;
}

LOCAL unsigned long _ov2680_StreamOn(SENSOR_HW_HANDLE handle, unsigned long param)
{
	SENSOR_PRINT("SENSOR_ov2680: StreamOn");

#ifdef CONFIG_CAMERA_RE_FOCUS
	al3200_mini_ctrl(param);
	al3200_mini_ctrl(1);
#endif
	usleep(100 * 1000);
	Sensor_WriteReg(0x0100, 0x01);

	return 0;
}

LOCAL unsigned long _ov2680_StreamOff(SENSOR_HW_HANDLE handle, unsigned long param)
{
	SENSOR_PRINT("SENSOR_ov2680: StreamOff");

	Sensor_WriteReg(0x0100, 0x00);
	usleep(100*1000);

	return 0;
}

LOCAL unsigned long _dw9174_SRCInit(SENSOR_HW_HANDLE handle, unsigned long mode)
{
	uint8_t cmd_val[6] = {0x00};
	uint16_t  slave_addr = 0;
	uint16_t cmd_len = 0;
	uint32_t ret_value = SENSOR_SUCCESS;

	slave_addr = DW9714_VCM_SLAVE_ADDR;

	switch (mode) {
		case 1:
		break;

		case 2:
		{
			cmd_val[0] = 0xec;
			cmd_val[1] = 0xa3;
			cmd_val[2] = 0xf2;
			cmd_val[3] = 0x00;
			cmd_val[4] = 0xdc;
			cmd_val[5] = 0x51;
			cmd_len = 6;
			Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);
		}
		break;

		case 3:
		break;

	}

	return ret_value;
}

LOCAL uint32_t _ov2680_get_static_info(SENSOR_HW_HANDLE handle, uint32_t *param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	struct sensor_ex_info *ex_info;
	//make sure we have get max fps of all settings.
	if(!s_ov2680_mode_fps_info.is_init) {
		_ov2680_init_mode_fps_info(handle);
	}
	ex_info = (struct sensor_ex_info*)param;
	ex_info->f_num = s_ov2680_static_info.f_num;
	ex_info->focal_length = s_ov2680_static_info.focal_length;
	ex_info->max_fps = s_ov2680_static_info.max_fps;
	ex_info->max_adgain = s_ov2680_static_info.max_adgain;
	ex_info->ois_supported = s_ov2680_static_info.ois_supported;
	ex_info->pdaf_supported = s_ov2680_static_info.pdaf_supported;
	ex_info->exp_valid_frame_num = s_ov2680_static_info.exp_valid_frame_num;
	ex_info->clamp_level = s_ov2680_static_info.clamp_level;
	ex_info->adgain_valid_frame_num = s_ov2680_static_info.adgain_valid_frame_num;
	ex_info->preview_skip_num = g_ov2680_mipi_raw_info.preview_skip_num;
	ex_info->capture_skip_num = g_ov2680_mipi_raw_info.capture_skip_num;
	ex_info->name = g_ov2680_mipi_raw_info.name;
	ex_info->sensor_version_info = g_ov2680_mipi_raw_info.sensor_version_info;
	SENSOR_PRINT("SENSOR_ov2680: f_num: %d", ex_info->f_num);
	SENSOR_PRINT("SENSOR_ov2680: max_fps: %d", ex_info->max_fps);
	SENSOR_PRINT("SENSOR_ov2680: max_adgain: %d", ex_info->max_adgain);
	SENSOR_PRINT("SENSOR_ov2680: ois_supported: %d", ex_info->ois_supported);
	SENSOR_PRINT("SENSOR_ov2680: pdaf_supported: %d", ex_info->pdaf_supported);
	SENSOR_PRINT("SENSOR_ov2680: exp_valid_frame_num: %d", ex_info->exp_valid_frame_num);
	SENSOR_PRINT("SENSOR_ov2680: clam_level: %d", ex_info->clamp_level);
	SENSOR_PRINT("SENSOR_ov2680: adgain_valid_frame_num: %d", ex_info->adgain_valid_frame_num);
	SENSOR_PRINT("SENSOR_ov2680: sensor name is: %s", ex_info->name);
	SENSOR_PRINT("SENSOR_ov2680: sensor version info is: %s", ex_info->sensor_version_info);

	return rtn;
}


LOCAL uint32_t _ov2680_get_fps_info(SENSOR_HW_HANDLE handle, uint32_t *param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_MODE_FPS_T *fps_info;
	//make sure have inited fps of every sensor mode.
	if(!s_ov2680_mode_fps_info.is_init) {
		_ov2680_init_mode_fps_info(handle);
	}
	fps_info = (SENSOR_MODE_FPS_T*)param;
	uint32_t sensor_mode = fps_info->mode;
	fps_info->max_fps = s_ov2680_mode_fps_info.sensor_mode_fps[sensor_mode].max_fps;
	fps_info->min_fps = s_ov2680_mode_fps_info.sensor_mode_fps[sensor_mode].min_fps;
	fps_info->is_high_fps = s_ov2680_mode_fps_info.sensor_mode_fps[sensor_mode].is_high_fps;
	fps_info->high_fps_skip_num = s_ov2680_mode_fps_info.sensor_mode_fps[sensor_mode].high_fps_skip_num;
	SENSOR_PRINT("SENSOR_ov2680: mode %d, max_fps: %d",fps_info->mode, fps_info->max_fps);
	SENSOR_PRINT("SENSOR_ov2680: min_fps: %d", fps_info->min_fps);
	SENSOR_PRINT("SENSOR_ov2680: is_high_fps: %d", fps_info->is_high_fps);
	SENSOR_PRINT("SENSOR_ov2680: high_fps_skip_num: %d", fps_info->high_fps_skip_num);

	return rtn;
}

LOCAL unsigned long _ov2680_access_val(SENSOR_HW_HANDLE handle, unsigned long param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_VAL_T* param_ptr = (SENSOR_VAL_T*)param;
	uint16_t tmp;

	SENSOR_PRINT("SENSOR_ov2680: _ov2680_access_val E param_ptr = %p", param_ptr);
	if(!param_ptr){
		return rtn;
	}

	SENSOR_PRINT("SENSOR_ov2680: param_ptr->type=%x", param_ptr->type);
	switch(param_ptr->type)
	{
		case SENSOR_VAL_TYPE_GET_STATIC_INFO:
			rtn = _ov2680_get_static_info(handle, param_ptr->pval);
			break;
		case SENSOR_VAL_TYPE_GET_FPS_INFO:
			rtn = _ov2680_get_fps_info(handle, param_ptr->pval);
			break;
		default:
			break;
	}

	SENSOR_PRINT("SENSOR_ov2680: _ov2680_access_val X");

	return rtn;
}

