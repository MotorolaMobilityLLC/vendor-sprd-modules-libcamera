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

//#include "sensor_ov13850r2a_raw_param_v3.c"
#include "param/sensor_ov13850r2a_raw_param_main.c"

#define OV13850R2A_I2C_ADDR_W         (0x36)
#define OV13850R2A_I2C_ADDR_R         (0x36)

#define OV13850R2A_RAW_PARAM_COM  0x0000

static int s_ov13850r2a_gain = 0;
static int s_capture_shutter = 0;
static int s_capture_VTS = 0;
static int s_video_min_framerate = 0;
static int s_video_max_framerate = 0;
static int s_exposure_time = 0;

LOCAL unsigned long _ov13850r2a_GetResolutionTrimTab(unsigned long param);
LOCAL unsigned long _ov13850r2a_PowerOn(SENSOR_HW_HANDLE handle, unsigned long power_on);
LOCAL unsigned long _ov13850r2a_Identify(SENSOR_HW_HANDLE handle, unsigned long param);
LOCAL unsigned long _ov13850r2a_BeforeSnapshot(SENSOR_HW_HANDLE handle, unsigned long param);
LOCAL unsigned long _ov13850r2a_after_snapshot(SENSOR_HW_HANDLE handle, unsigned long param);
LOCAL unsigned long _ov13850r2a_StreamOn(SENSOR_HW_HANDLE handle, unsigned long param);
LOCAL unsigned long _ov13850r2a_StreamOff(SENSOR_HW_HANDLE handle, unsigned long param);
LOCAL unsigned long _ov13850r2a_write_exposure(SENSOR_HW_HANDLE handle, unsigned long param);
LOCAL unsigned long _ov13850r2a_write_gain(SENSOR_HW_HANDLE handle, unsigned long param);
LOCAL unsigned long _ov13850r2a_write_af(SENSOR_HW_HANDLE handle, unsigned long param);
LOCAL unsigned long _ov13850r2a_flash(SENSOR_HW_HANDLE handle, unsigned long param);
LOCAL unsigned long _ov13850r2a_ExtFunc(SENSOR_HW_HANDLE handle, unsigned long ctl_param);
LOCAL unsigned long _dw9718s_SRCInit(SENSOR_HW_HANDLE handle, unsigned long mode);
LOCAL uint32_t _ov13850r2a_get_VTS(SENSOR_HW_HANDLE handle);
LOCAL uint32_t _ov13850r2a_set_VTS(SENSOR_HW_HANDLE handle, int VTS);
LOCAL unsigned long _ov13850r2a_ReadGain(SENSOR_HW_HANDLE handle, unsigned long param);
LOCAL unsigned long _ov13850r2a_set_video_mode(SENSOR_HW_HANDLE handle, unsigned long param);
LOCAL uint32_t _ov13850r2a_get_shutter(SENSOR_HW_HANDLE handle);
LOCAL uint32_t _ov13850r2a_com_Identify_otp(SENSOR_HW_HANDLE handle, void* param_ptr);
LOCAL unsigned long _ov13850r2a_cfg_otp(SENSOR_HW_HANDLE handle, unsigned long  param);
LOCAL uint32_t _ov13850r2a_read_otp_gain(SENSOR_HW_HANDLE handle, uint32_t *param);
LOCAL uint32_t _ov13850r2a_write_otp_gain(SENSOR_HW_HANDLE handle, uint32_t *param);
LOCAL unsigned long _ov13850r2a_access_val(SENSOR_HW_HANDLE handle, unsigned long param);
static unsigned long _ov13850r2a_GetExifInfo(SENSOR_HW_HANDLE handle, unsigned long param);

struct sensor_raw_info* s_ov13850_mipi_raw_info_ptr = NULL;
LOCAL const struct raw_param_info_tab s_ov13850_raw_param_tab[] = {
	{OV13850R2A_RAW_PARAM_COM, &s_ov13850r2a_mipi_raw_info, PNULL, PNULL},
};

struct sensor_raw_info* s_ov13850r2a_mipi_raw_info_ptr=NULL;


static uint32_t g_module_id = 0;

static uint32_t g_flash_mode_en = 0;
static uint32_t g_af_slewrate = 1;

LOCAL const SENSOR_REG_T ov13850r2a_common_init[] =
{
	{0x103,	0x1},
	{0x300,	0x0},
};

LOCAL const SENSOR_REG_T ov13850r2a_4lane_1056x594_setting[] =
{
	{0x0103,	0x1},
	{0x0300,	0x0},//0x01
	{0x0301,	0x0},
	{0x0302,	0x27},
	{0x0303,	0x2},
	{0x030a,	0x0},
	{0x300f,	0x11},
	{0x3010,	0x1},
	{0x3011,	0x76},
	{0x3012,	0x41},
	{0x3013,	0x12},
	{0x3014,	0x11},
	{0x301f,	0x3},
	{0x3106,	0x0},
	{0x3210,	0x47},
	{0x3500,	0x0},
	{0x3501,	0x29},
	{0x3502,	0xA0},
	{0x3506,	0x0},
	{0x3507,	0x2},
	{0x3508,	0x0},
	{0x350a,	0x0},
	{0x350b,	0x80},
	{0x350e,	0x0},
	{0x350f,	0x10},
	{0x351a,	0x0},
	{0x351b,	0x10},
	{0x351c,	0x0},
	{0x351d,	0x20},
	{0x351e,	0x0},
	{0x351f,	0x40},
	{0x3520,	0x0},
	{0x3521,	0x80},
	{0x3600,	0xc0},
	{0x3601,	0xfc},
	{0x3602,	0x2},
	{0x3603,	0x78},
	{0x3604,	0xb1},
	{0x3605,	0x95},
	{0x3606,	0x73},
	{0x3607,	0x7},
	{0x3609,	0x40},
	{0x360a,	0x30},
	{0x360b,	0x91},
	{0x360C,	0x9},
	{0x360f,	0x2},
	{0x3611,	0x10},
	{0x3612,	0x7f},
	{0x3613,	0x33},
	{0x3615,	0x0c},
	{0x3616,	0x0e},
	{0x3641,	0x2},
	{0x3660,	0x82},
	{0x3668,	0x54},
	{0x3669,	0x0},
	{0x366a,	0x3f},
	{0x3667,	0xa0},
	{0x3702,	0x5a},
	{0x3703,	0x44},
	{0x3704,	0x2c},
	{0x3705,	0x1},
	{0x3706,	0x15},
	{0x3707,	0x44},
	{0x3708,	0x3c},
	{0x3709,	0x1f},
	{0x370a,	0xa9},
	{0x370b,	0x3c},
	{0x3720,	0x55},
	{0x3722,	0x84},
	{0x3728,	0x40},
	{0x372a,	0x0},
	{0x372b,	0x2},
	{0x372e,	0x22},
	{0x372f,	0x88},
	{0x3730,	0x0},
	{0x3731,	0x0},
	{0x3732,	0x0},
	{0x3733,	0x0},
	{0x3710,	0x28},
	{0x3716,	0x3},
	{0x3718,	0x10},
	{0x3719,	0x0c},
	{0x371a,	0x8},
	{0x371c,	0xfc},
	{0x3748,	0x0},
	{0x3760,	0x13},
	{0x3761,	0x33},
	{0x3762,	0x86},
	{0x3763,	0x16},
	{0x3767,	0x24},
	{0x3768,	0x6},
	{0x3769,	0x45},
	{0x376c,	0x23},
	{0x376f,	0x80},
	{0x3773,	0x6},
	{0x3d84,	0x0},
	{0x3d85,	0x17},
	{0x3d8c,	0x73},
	{0x3d8d,	0xbf},
	{0x3800,	0x0},
	{0x3801,	0x0},
	{0x3802,	0x1},
	{0x3803,	0x78},
	{0x3804,	0x10},
	{0x3805,	0x9f},
	{0x3806,	0x0a},
	{0x3807,	0xcf},
	{0x3808,	0x4},
	{0x3809,	0x20},
	{0x380a,	0x2},
	{0x380b,	0x52},
	{0x380c,	0x12},
	{0x380d,	0xc0},
	{0x380e,	0x0d},
	{0x380f,	0x0},
	{0x3810,	0x0},
	{0x3811,	0x8},
	{0x3812,	0x0},
	{0x3813,	0x2},
	{0x3814,	0x31},
	{0x3815,	0x35},
	{0x3820,	0x2},
	{0x3821,	0x6},
	{0x3823,	0x0},
	{0x3826,	0x0},
	{0x3827,	0x2},
	{0x3834,	0x2},
	{0x3835,	0x1c},
	{0x3836,	0x8},
	{0x3837,	0x4},
	{0x4000,	0xf1},
	{0x4001,	0x0},
	{0x400b,	0x0c},
	{0x4011,	0x0},
	{0x401a,	0x0},
	{0x401b,	0x0},
	{0x401c,	0x0},
	{0x401d,	0x0},
	{0x4020,	0x0},
	{0x4021,	0xe4},
	{0x4022,	0x3},
	{0x4023,	0x3f},
	{0x4024,	0x4},
	{0x4025,	0x20},
	{0x4026,	0x4},
	{0x4027,	0x25},
	{0x4028,	0x0},
	{0x4029,	0x2},
	{0x402a,	0x2},
	{0x402b,	0x4},
	{0x402c,	0x6},
	{0x402d,	0x2},
	{0x402e,	0x8},
	{0x402f,	0x4},
	{0x403d,	0x2c},
	{0x403f,	0x7f},
	{0x4041,	0x7},
	{0x4500,	0x82},
	{0x4501,	0x3c},
	{0x458b,	0x0},
	{0x459c,	0x0},
	{0x459d,	0x0},
	{0x459e,	0x0},
	{0x4601,	0x40},
	{0x4602,	0x22},
	{0x4603,	0x1},
	{0x4837,	0xbc},
	{0x4d00,	0x4},
	{0x4d01,	0x42},
	{0x4d02,	0xd1},
	{0x4d03,	0x90},
	{0x4d04,	0x66},
	{0x4d05,	0x65},
	{0x4d0b,	0x0},
	{0x5000,	0x0e},
	{0x5001,	0x1},
	{0x5002,	0x7},
	{0x5013,	0x40},
	{0x501c,	0x0},
	{0x501d,	0x10},
	{0x510f,	0xfc},
	{0x5110,	0xf0},
	{0x5111,	0x10},
	{0x536d,	0x2},
	{0x536e,	0x67},
	{0x536f,	0x1},
	{0x5370,	0x4c},
	{0x5400,	0x0},
	{0x5400,	0x0},
	{0x5401,	0x51},
	{0x5402,	0x0},
	{0x5403,	0x0},
	{0x5404,	0x0},
	{0x5405,	0x20},
	{0x540c,	0x5},
	{0x5501,	0x0},
	{0x5b00,	0x0},
	{0x5b01,	0x0},
	{0x5b02,	0x1},
	{0x5b03,	0xff},
	{0x5b04,	0x2},
	{0x5b05,	0x6c},
	{0x5b09,	0x2},
	{0x5e00,	0x0},//0x80
	{0x5e10,	0x1c},
};

LOCAL const SENSOR_REG_T ov13850r2a_4208x3120_setting[] =
{

};

LOCAL const SENSOR_REG_T ov13850r2a_4224x3136_setting[] =
{
};

LOCAL SENSOR_REG_TAB_INFO_T s_ov13850r2a_resolution_Tab_RAW[] = {
	{ADDR_AND_LEN_OF_ARRAY(ov13850r2a_common_init), 0, 0, 24, SENSOR_IMAGE_FORMAT_RAW},
	//{ADDR_AND_LEN_OF_ARRAY(ov13850r2a_2112x1568_setting), 2112, 1568, 24, SENSOR_IMAGE_FORMAT_RAW},
	{ADDR_AND_LEN_OF_ARRAY(ov13850r2a_4lane_1056x594_setting), 1056, 594, 24, SENSOR_IMAGE_FORMAT_RAW},
	//{ADDR_AND_LEN_OF_ARRAY(ov13850r2a_4224x3136_setting), 4224, 3136, 24, SENSOR_IMAGE_FORMAT_RAW},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0}
};

LOCAL SENSOR_TRIM_T s_ov13850r2a_Resolution_Trim_Tab[] = {
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	//{0, 0,  2112, 1568, 200, 640, 1664, {0, 0,  2112, 1568}},  //vts
	{0, 0, 1056, 594, 100, 1200, 594, {0, 0, 1056, 594}},
	//{0, 0, 4224, 3136, 100, 1200, 3328, {0, 0, 4224, 3136}},

	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
    {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}}
};

LOCAL const SENSOR_REG_T s_ov13850r2a_2112x1568_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
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

LOCAL const SENSOR_REG_T  s_ov13850r2a_4208x3120_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
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

LOCAL SENSOR_VIDEO_INFO_T s_ov13850r2a_video_info[] = {
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	//{{{30, 30, 200, 100}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},(SENSOR_REG_T**)s_ov13850r2a_2112x1568_video_tab},
	{{{15, 15, 200, 64}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},(SENSOR_REG_T**)s_ov13850r2a_4208x3120_video_tab},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
        {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL}
};

LOCAL unsigned long _ov13850r2a_set_video_mode(SENSOR_HW_HANDLE handle, unsigned long param)
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

	if (PNULL == s_ov13850r2a_video_info[mode].setting_ptr) {
		SENSOR_PRINT("fail.");
		return SENSOR_FAIL;
	}

	sensor_reg_ptr = (SENSOR_REG_T_PTR)&s_ov13850r2a_video_info[mode].setting_ptr[param];
	if (PNULL == sensor_reg_ptr) {
		SENSOR_PRINT("fail.");
		return SENSOR_FAIL;
	}

	for (i=0x00; (0xffff!=sensor_reg_ptr[i].reg_addr)||(0xff!=sensor_reg_ptr[i].reg_value); i++) {
		Sensor_WriteReg(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
	}

	SENSOR_PRINT("0x%lx", param);
	return 0;
}

LOCAL SENSOR_IOCTL_FUNC_TAB_T s_ov13850r2a_ioctl_func_tab = {
	PNULL,
	_ov13850r2a_PowerOn,
	PNULL,
	_ov13850r2a_Identify,

	PNULL,			// write register
	PNULL,			// read  register
	PNULL,
	_ov13850r2a_GetResolutionTrimTab,

	// External
	PNULL,
	PNULL,
	PNULL,

	PNULL, //_ov13850r2a_set_brightness,
	PNULL, // _ov13850r2a_set_contrast,
	PNULL,
	PNULL,			//_ov13850r2a_set_saturation,

	PNULL, //_ov13850r2a_set_work_mode,
	PNULL, //_ov13850r2a_set_image_effect,

	_ov13850r2a_BeforeSnapshot,
	_ov13850r2a_after_snapshot,
	PNULL,//_ov13850r2a_flash,
	PNULL,
	PNULL,//_ov13850r2a_write_exposure,
	PNULL,
	PNULL,//_ov13850r2a_write_gain,
	PNULL,
	PNULL,
	PNULL,//_ov13850r2a_write_af,
	PNULL,
	PNULL, //_ov13850r2a_set_awb,
	PNULL,
	PNULL,
	PNULL, //_ov13850r2a_set_ev,
	PNULL,
	PNULL,
	PNULL,
	PNULL, //_ov13850r2a_GetExifInfo,
	PNULL,//_ov13850r2a_ExtFunc,
	PNULL, //_ov13850r2a_set_anti_flicker,
	_ov13850r2a_set_video_mode,
	PNULL, //pick_jpeg_stream
	PNULL,  //meter_mode
	PNULL, //get_status
	_ov13850r2a_StreamOn,
	_ov13850r2a_StreamOff,
	_ov13850r2a_access_val,
};

static SENSOR_STATIC_INFO_T s_ov13850r2a_static_info = {
	200,    //f-number,focal ratio
	357,    //focal_length;
	0,      //max_fps,max fps of sensor's all settings,it will be calculated from sensor mode fps
	16*256, //max_adgain,AD-gain
	0,      //ois_supported;
	0,      //pdaf_supported;
	1,      //exp_valid_frame_num;N+2-1
	64,     //clamp_level,black level
	1,      //adgain_valid_frame_num;N+1-1
};

SENSOR_INFO_T g_ov13850r2a_mipi_raw_info = {
	OV13850R2A_I2C_ADDR_W,	// salve i2c write address
	OV13850R2A_I2C_ADDR_R,	// salve i2c read address

	SENSOR_I2C_REG_16BIT | SENSOR_I2C_REG_8BIT | SENSOR_I2C_FREQ_400,	// bit0: 0: i2c register value is 8 bit, 1: i2c register value is 16 bit
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
	5,			// reset pulse width(ms)

	SENSOR_LOW_LEVEL_PWDN,	// 1: high level valid; 0: low level valid

	1,			// count of identify code
	{{0x300A, 0xD8},		// supply two code to identify sensor.
	 {0x300B, 0x50}},		// for Example: index = 0-> Device id, index = 1 -> version id

	SENSOR_AVDD_2800MV,	// voltage of avdd

	4224,			// max width of source image
	3136,			// max height of source image
	(cmr_s8 *)"ov13850r2a_mipi_raw",		// name of sensor

	SENSOR_IMAGE_FORMAT_RAW,	// define in SENSOR_IMAGE_FORMAT_E enum,SENSOR_IMAGE_FORMAT_MAX
	// if set to SENSOR_IMAGE_FORMAT_MAX here, image format depent on SENSOR_REG_TAB_INFO_T

	SENSOR_IMAGE_PATTERN_RAWRGB_B,// pattern of input image form sensor;

	s_ov13850r2a_resolution_Tab_RAW,	// point to resolution table information structure
	&s_ov13850r2a_ioctl_func_tab,	// point to ioctl function table
	&s_ov13850r2a_mipi_raw_info_ptr,		// information and table about Rawrgb sensor
	NULL,			//&g_ov13850r2a_ext_info,                // extend information about sensor
	SENSOR_AVDD_1800MV,	// iovdd
	SENSOR_AVDD_1200MV,	// dvdd
	0,			// skip frame num before preview
	1,			// skip frame num before capture
	0,			// deci frame num during preview
	0,			// deci frame num during video preview

	0,
	0,
	0,
	0,
	0,
	{SENSOR_INTERFACE_TYPE_CSI2, 4, 10, 0},
	s_ov13850r2a_video_info,
	3,			// skip frame num while change setting
	48,
	48,
	(cmr_s8 *)"ov13850r2a_v1",
};


LOCAL struct sensor_raw_info* Sensor_GetContext(void)
{
	return s_ov13850r2a_mipi_raw_info_ptr;
}


LOCAL unsigned long _ov13850r2a_GetResolutionTrimTab(unsigned long param)
{
	SENSOR_PRINT("0x%lx",  (unsigned long)s_ov13850r2a_Resolution_Trim_Tab);
	return (unsigned long) s_ov13850r2a_Resolution_Trim_Tab;
}

LOCAL unsigned long _ov13850r2a_PowerOn(SENSOR_HW_HANDLE handle, unsigned long power_on)
{
	SENSOR_AVDD_VAL_E dvdd_val = g_ov13850r2a_mipi_raw_info.dvdd_val;
	SENSOR_AVDD_VAL_E avdd_val = g_ov13850r2a_mipi_raw_info.avdd_val;
	SENSOR_AVDD_VAL_E iovdd_val = g_ov13850r2a_mipi_raw_info.iovdd_val;
	BOOLEAN power_down = g_ov13850r2a_mipi_raw_info.power_down_level;
	BOOLEAN reset_level = g_ov13850r2a_mipi_raw_info.reset_pulse_level;

	if (SENSOR_TRUE == power_on) {
		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		Sensor_SetMonitorVoltage(SENSOR_AVDD_CLOSED);
		Sensor_SetVoltage(SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED);
		Sensor_Reset(reset_level);
		Sensor_PowerDown(power_down);
		usleep(2*1000);

		// Open power
		Sensor_SetMonitorVoltage(SENSOR_AVDD_2800MV);
		usleep(1000);
		Sensor_SetIovddVoltage(iovdd_val);
		usleep(1000);
		Sensor_SetAvddVoltage(avdd_val);
		usleep(1000);
		Sensor_SetDvddVoltage(dvdd_val);

		usleep(10*1000);
		Sensor_PowerDown(!power_down);
		usleep(1000);
		Sensor_Reset(!reset_level);
		usleep(2*1000);
		Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);
		usleep(2*1000);

	} else {
		usleep(1000);
		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		usleep(1000);
		Sensor_SetResetLevel(reset_level);
		Sensor_PowerDown(power_down);
		usleep(1000);
		Sensor_SetDvddVoltage(SENSOR_AVDD_CLOSED);
		usleep(1000);
		Sensor_SetIovddVoltage(SENSOR_AVDD_CLOSED);
		Sensor_SetAvddVoltage(SENSOR_AVDD_CLOSED);

		Sensor_SetMonitorVoltage(SENSOR_AVDD_CLOSED);
	}
	SENSOR_PRINT("SENSOR_OV13850R2A: _ov13850r2a_Power_On(1:on, 0:off): %ld", power_on);
	return SENSOR_SUCCESS;
}

LOCAL uint32_t _ov13850r2a_GetRawInfo(void)
{
	uint32_t rtn=SENSOR_SUCCESS;
	struct raw_param_info_tab* tab_ptr = (struct raw_param_info_tab*)s_ov13850_raw_param_tab;

	/*read param id from sensor omap*/
	s_ov13850r2a_mipi_raw_info_ptr = tab_ptr[0].info_ptr;
	SENSOR_PRINT("SENSOR_OV13850: ov13850_GetRawInof success");

	return rtn;
}

static uint32_t _ov13850r2a_get_static_info(SENSOR_HW_HANDLE handle, uint32_t *param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	struct sensor_ex_info *ex_info;
	uint32_t up = 0;
	uint32_t down = 0;

	ex_info = (struct sensor_ex_info*)param;
	ex_info->f_num = s_ov13850r2a_static_info.f_num;
	ex_info->focal_length = s_ov13850r2a_static_info.focal_length;
	ex_info->max_fps = s_ov13850r2a_static_info.max_fps;
	ex_info->max_adgain = s_ov13850r2a_static_info.max_adgain;
	ex_info->ois_supported = s_ov13850r2a_static_info.ois_supported;
	ex_info->pdaf_supported = s_ov13850r2a_static_info.pdaf_supported;
	ex_info->exp_valid_frame_num = s_ov13850r2a_static_info.exp_valid_frame_num;
	ex_info->clamp_level = s_ov13850r2a_static_info.clamp_level;
	ex_info->adgain_valid_frame_num = s_ov13850r2a_static_info.adgain_valid_frame_num;
	ex_info->preview_skip_num = g_ov13850r2a_mipi_raw_info.preview_skip_num;
	ex_info->capture_skip_num = g_ov13850r2a_mipi_raw_info.capture_skip_num;
	ex_info->name = g_ov13850r2a_mipi_raw_info.name;
	ex_info->sensor_version_info = g_ov13850r2a_mipi_raw_info.sensor_version_info;
	ex_info->pos_dis.up2hori = up;
	ex_info->pos_dis.hori2down = down;

	return rtn;
}

LOCAL unsigned long _ov13850r2a_access_val(SENSOR_HW_HANDLE handle, unsigned long param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_VAL_T* param_ptr = (SENSOR_VAL_T*)param;
	uint16_t tmp;

	SENSOR_PRINT("SENSOR_OV13850R2A: _ov13850r2a_access_val E");

	switch(param_ptr->type)
	{
		case SENSOR_VAL_TYPE_SHUTTER:
			*((uint32_t*)param_ptr->pval) = _ov13850r2a_get_shutter(handle);
			break;
		case SENSOR_VAL_TYPE_READ_OTP_GAIN:
			rtn = _ov13850r2a_read_otp_gain(handle, param_ptr->pval);
			break;
		case SENSOR_VAL_TYPE_GET_STATIC_INFO:
			rtn = _ov13850r2a_get_static_info(handle, param_ptr->pval);
			break;
		default:
			break;
	}

	SENSOR_PRINT("SENSOR_OV13850R2A: _ov13850r2a_access_val X");

	return rtn;
}

LOCAL unsigned long _ov13850r2a_GetMaxFrameLine(unsigned long index)
{
	uint32_t max_line=0x00;
	SENSOR_TRIM_T_PTR trim_ptr=s_ov13850r2a_Resolution_Trim_Tab;

	max_line=trim_ptr[index].frame_line;

	return max_line;
}

LOCAL unsigned long _ov13850r2a_Identify(SENSOR_HW_HANDLE handle, unsigned long param)
{
	#define OV13850R2A_PID_VALUE_H    0xD8
	#define OV13850R2A_PID_ADDR_H     0x300A
	#define OV13850R2A_PID_VALUE_L    0x50
	#define OV13850R2A_PID_ADDR_L     0x300B
	#define OV13850R2A_VER_VALUE      0xB2
	#define OV13850R2A_VER_ADDR       0x302A

	uint8_t pid_value_h = 0x00;
	uint8_t pid_value_l = 0x00;
	uint8_t ver_value = 0x00;
	uint32_t ret_value = SENSOR_FAIL;

	SENSOR_PRINT("SENSOR_OV13850R2A: mipi raw identify\n");

	pid_value_h = Sensor_ReadReg(OV13850R2A_PID_ADDR_H);
	pid_value_l = Sensor_ReadReg(OV13850R2A_PID_ADDR_L);
	if (OV13850R2A_PID_VALUE_H == pid_value_h && OV13850R2A_PID_VALUE_L == pid_value_l) {

		ver_value = Sensor_ReadReg(OV13850R2A_VER_ADDR);
		SENSOR_PRINT("SENSOR_OV13850R2A: Identify: PID = %x%x, VER = %x", pid_value_h, pid_value_l, ver_value);
		if (OV13850R2A_VER_VALUE == ver_value) {
			SENSOR_PRINT("SENSOR_OV13850R2A: this is ov13850r2a %x sensor !", ver_value);
			ret_value = _ov13850r2a_GetRawInfo();
			ret_value = SENSOR_SUCCESS;
			if(SENSOR_SUCCESS != ret_value)
			{
				SENSOR_PRINT_ERR("SENSOR_OV13850R2A: the module is unknow error !");
			}
			//Sensor_ov13850r2a_InitRawTuneInfo();
		} else {
			SENSOR_PRINT_HIGH("SENSOR_OV13850R2A: Identify this is OV%x%x %x sensor !", pid_value_h, pid_value_l, ver_value);
		}

	} else {
		SENSOR_PRINT_ERR("SENSOR_OV13850R2A: identify fail,pid_value_h=%x, pid_value_l=%x", pid_value_h, pid_value_l);
	}

	return ret_value;
}

LOCAL uint32_t ov13850_get_shutter(SENSOR_HW_HANDLE handle)
{
	// read shutter, in number of line period
	uint32_t shutter = 0;

	shutter = (Sensor_ReadReg(0x03500) & 0x0f);
	shutter = (shutter<<8) + Sensor_ReadReg(0x3501);
	shutter = (shutter<<4) + (Sensor_ReadReg(0x3502)>>4);

	return shutter;
}

LOCAL unsigned long  ov13850_set_shutter(SENSOR_HW_HANDLE handle, unsigned long shutter)
{
	// write shutter, in number of line period
	uint16_t temp = 0;

	shutter = shutter & 0xffff;
	temp = shutter & 0x0f;
	temp = temp<<4;
	Sensor_WriteReg(0x3502, temp);
	temp = shutter & 0xfff;
	temp = temp>>4;
	Sensor_WriteReg(0x3501, temp);
	temp = (shutter>>12) & 0xf;
	Sensor_WriteReg(0x3500, temp);

	return 0;
}

LOCAL uint32_t _ov13850_get_VTS(SENSOR_HW_HANDLE handle)
{
	// read VTS from register settings
	uint32_t VTS;

	VTS = Sensor_ReadReg(0x380e);//total vertical size[15:8] high byte
	VTS = ((VTS & 0x7f)<<8) + Sensor_ReadReg(0x380f);

	return VTS;
}

LOCAL unsigned long ov13850_set_VTS(SENSOR_HW_HANDLE handle, unsigned long VTS)
{
	// write VTS to registers
	uint16_t temp = 0;

	temp = VTS & 0xff;
	Sensor_WriteReg(0x380f, temp);
	temp = (VTS>>8) & 0x7f;
	Sensor_WriteReg(0x380e, temp);

	return 0;
}

//sensor gain
LOCAL uint16_t ov13850_get_sensor_gain16(SENSOR_HW_HANDLE handle)
{
	// read sensor gain, 16 = 1x
	uint16_t gain16 = 0;

	gain16 = Sensor_ReadReg(0x350a) & 0x03;
	gain16 = (gain16<<8) + Sensor_ReadReg( 0x350b);
	gain16 = ((gain16 >> 4) + 1)*((gain16 & 0x0f) + 16);

	return gain16;
}

LOCAL uint16_t ov13850_set_sensor_gain16(SENSOR_HW_HANDLE handle, int gain16)
{
	// write sensor gain, 16 = 1x
	int gain_reg = 0;
	uint16_t temp = 0;
	int i = 0;

	if(gain16 > 0x7ff)
	{
		gain16 = 0x7ff;
	}

	for(i=0; i<5; i++) {
		if (gain16>31) {
			gain16 = gain16/2;
			gain_reg = gain_reg | (0x10<<i);
		}
		else
			break;
	}
	gain_reg = gain_reg | (gain16 - 16);
	temp = gain_reg & 0xff;
	Sensor_WriteReg(0x350b, temp);
	temp = gain_reg>>8;
	Sensor_WriteReg(0x350a, temp);
	return 0;
}

LOCAL unsigned long _ov13850r2a_write_exposure(SENSOR_HW_HANDLE handle, unsigned long param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t expsure_line=0x00;
	uint16_t dummy_line=0x00;
	uint16_t size_index=0x00;
	uint16_t frame_len=0x00;
	uint16_t frame_len_cur=0x00;
	uint16_t max_frame_len=0x00;
	uint16_t value=0x00;
	uint16_t value0=0x00;
	uint16_t value1=0x00;
	uint16_t value2=0x00;
	uint32_t linetime = 0;

	expsure_line=param&0xffff;
	dummy_line=(param>>0x10)&0x0fff;
	size_index=(param>>0x1c)&0x0f;

	SENSOR_PRINT("SENSOR_OV13850R2A: write_exposure line:%d, dummy:%d, size_index:%d", expsure_line, dummy_line, size_index);

	max_frame_len=_ov13850r2a_GetMaxFrameLine(size_index);
	if(0x00!=max_frame_len)
	{
		frame_len = ((expsure_line+8)> max_frame_len) ? (expsure_line+8) : max_frame_len;

		frame_len_cur = _ov13850_get_VTS(handle);

		SENSOR_PRINT("SENSOR_OV13850R2A: frame_len: %d,   frame_len_cur:%d\n", frame_len, frame_len_cur);

		if(frame_len_cur != frame_len){
			ov13850_set_VTS(handle, frame_len);
		}
	}
	ov13850_set_shutter(handle, expsure_line);

	s_capture_shutter = expsure_line;
	linetime=s_ov13850r2a_Resolution_Trim_Tab[size_index].line_time;
	s_exposure_time = s_capture_shutter * linetime / 10;
	Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_EXPOSURETIME, s_capture_shutter);

	return ret_value;
}

LOCAL unsigned long _ov13850r2a_write_gain(SENSOR_HW_HANDLE handle, unsigned long param)
{
	uint32_t ret_value = SENSOR_SUCCESS;

	return ret_value;
}

LOCAL unsigned long _ov13850r2a_write_af(SENSOR_HW_HANDLE handle, unsigned long param)
{
	uint32_t ret_value = SENSOR_SUCCESS;

	return ret_value;
}

LOCAL unsigned long _ov13850r2a_BeforeSnapshot(SENSOR_HW_HANDLE handle, unsigned long param)
{

	return SENSOR_SUCCESS;
}

LOCAL unsigned long _ov13850r2a_after_snapshot(SENSOR_HW_HANDLE handle, unsigned long param)
{
	SENSOR_PRINT("SENSOR_OV13850R2A: after_snapshot mode:%ld", param);
	Sensor_SetMode((uint32_t)param);

	return SENSOR_SUCCESS;
}

static unsigned long _ov13850r2a_GetExifInfo(SENSOR_HW_HANDLE handle, unsigned long param)
{
	LOCAL EXIF_SPEC_PIC_TAKING_COND_T sexif;

	sexif.ExposureTime.numerator = s_exposure_time;
	sexif.ExposureTime.denominator = 1000000;

	return (unsigned long) & sexif;
}


LOCAL unsigned long _ov13850r2a_flash(SENSOR_HW_HANDLE handle, unsigned long param)
{
	SENSOR_PRINT("SENSOR_OV13850R2A: param=%d", param);

	/* enable flash, disable in _ov13850r2a_BeforeSnapshot */
	g_flash_mode_en = param;
	Sensor_SetFlash(param);
	SENSOR_PRINT("end");
	return SENSOR_SUCCESS;
}

LOCAL unsigned long _ov13850r2a_StreamOn(SENSOR_HW_HANDLE handle, unsigned long param)
{
	SENSOR_PRINT("SENSOR_OV13850R2A: StreamOn");

	Sensor_WriteReg(0x0100, 0x01);

	return 0;
}

LOCAL unsigned long _ov13850r2a_StreamOff(SENSOR_HW_HANDLE handle, unsigned long param)
{
	SENSOR_PRINT("SENSOR_OV13850R2A: StreamOff");

	Sensor_WriteReg(0x0100, 0x00);
	usleep(150*1000);

	return 0;
}

LOCAL uint32_t _ov13850r2a_get_shutter(SENSOR_HW_HANDLE handle)
{
	int shutter;

	shutter = (Sensor_ReadReg(0x03500) & 0x0f);
	shutter = (shutter<<8) + Sensor_ReadReg(0x3501);
	shutter = (shutter<<4) + (Sensor_ReadReg(0x3502)>>4);

	return shutter;
}

LOCAL uint32_t _ov13850r2a_set_shutter(SENSOR_HW_HANDLE handle, int shutter)
{
	return 0;
}

LOCAL int _ov13850r2a_get_gain16(SENSOR_HW_HANDLE handle)
{
	// read gain, 16 = 1x
	int gain16;

	gain16 = Sensor_ReadReg(0x350a) & 0x03;
	gain16 = (gain16<<8) + Sensor_ReadReg(0x350b);

	return gain16;
}

LOCAL int _ov13850r2a_set_gain16(SENSOR_HW_HANDLE handle, int gain16)
{
	// write gain, 16 = 1x
	int temp;
	gain16 = gain16 & 0x3ff;

	temp = gain16 & 0xff;
	Sensor_WriteReg(0x350b, temp);

	temp = gain16>>8;
	Sensor_WriteReg(0x350a, temp);

	return 0;
}

static void _calculate_hdr_exposure(SENSOR_HW_HANDLE handle, int capture_gain16,int capture_VTS, int capture_shutter)
{
	return;
}

static unsigned long _ov13850r2a_SetEV(SENSOR_HW_HANDLE handle, unsigned long param)
{
	return 0;
}
LOCAL unsigned long _ov13850r2a_ExtFunc(SENSOR_HW_HANDLE handle, unsigned long ctl_param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr =
	    (SENSOR_EXT_FUN_PARAM_T_PTR) ctl_param;
	SENSOR_PRINT("0x%x", ext_ptr->cmd);

	switch (ext_ptr->cmd) {
	case SENSOR_EXT_FUNC_INIT:
		break;
	case SENSOR_EXT_FOCUS_START:
		break;
	case SENSOR_EXT_EXPOSURE_START:
		break;
	case SENSOR_EXT_EV:
		rtn = _ov13850r2a_SetEV(handle, ctl_param);
		break;
	default:
		break;
	}
	return rtn;
}
LOCAL uint32_t _ov13850r2a_get_VTS(SENSOR_HW_HANDLE handle)
{
	// read VTS from register settings
	int VTS;

	VTS = Sensor_ReadReg(0x380e);//total vertical size[15:8] high byte

	VTS = (VTS<<8) + Sensor_ReadReg(0x380f);

	return VTS;
}

LOCAL uint32_t _ov13850r2a_set_VTS(SENSOR_HW_HANDLE handle, int VTS)
{
	return 0;
}

LOCAL unsigned long _ov13850r2a_ReadGain(SENSOR_HW_HANDLE handle, unsigned long param)
{
	return 0;
}


LOCAL uint32_t _ov13850r2a_read_otp_gain(SENSOR_HW_HANDLE handle, uint32_t *param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	return rtn;
}
