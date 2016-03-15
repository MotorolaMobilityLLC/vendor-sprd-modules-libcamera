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

LOCAL uint32_t update_otp(void* param_ptr);
LOCAL uint32_t _ov2680_Truly_Identify_otp(void* param_ptr);
LOCAL uint32_t _ov2680_Sunny_Identify_otp(void* param_ptr);
LOCAL unsigned long _ov2680_GetResolutionTrimTab(unsigned long param);
LOCAL unsigned long _ov2680_PowerOn(unsigned long power_on);
LOCAL unsigned long _ov2680_Identify(unsigned long param);
LOCAL unsigned long _ov2680_BeforeSnapshot(unsigned long param);
LOCAL unsigned long _ov2680_after_snapshot(unsigned long param);
LOCAL unsigned long _ov2680_StreamOn(unsigned long param);
LOCAL unsigned long _ov2680_StreamOff(unsigned long param);
LOCAL unsigned long _ov2680_write_exposure(unsigned long param);
LOCAL unsigned long _ov2680_write_gain(unsigned long param);
LOCAL unsigned long _ov2680_write_af(unsigned long param);
LOCAL unsigned long _ov2680_ReadGain(unsigned long param);
LOCAL unsigned long _ov2680_SetEV(unsigned long param);
LOCAL unsigned long _ov2680_ExtFunc(unsigned long ctl_param);
LOCAL unsigned long _dw9174_SRCInit(unsigned long mode);
LOCAL unsigned long _ov2680_flash(unsigned long param);
LOCAL uint32_t _ov2680_com_Identify_otp(void* param_ptr);
LOCAL unsigned long _ov2680_cfg_otp(unsigned long  param);

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
	{ADDR_AND_LEN_OF_ARRAY(ov2680_1600X1200_mipi_raw), 1600, 1200, 24, SENSOR_IMAGE_FORMAT_RAW},
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
	//{0, 0, 800, 600, 518, 330, 644, {0, 0, 800, 600}},
	{0, 0, 1600, 1200, 258, 628, 1294, {0, 0, 1600, 1200}},
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

LOCAL unsigned long _ov2680_set_video_mode(unsigned long param)
{
	SENSOR_REG_T_PTR sensor_reg_ptr;
	uint16_t         i = 0x00;
	uint32_t         mode;

	if (param >= SENSOR_VIDEO_MODE_MAX)
		return 0;

	if (SENSOR_SUCCESS != Sensor_GetMode_Ex(&mode, SENSOR_DEVICE2)) {
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
		Sensor_WriteReg_Ex(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value, SENSOR_DEVICE2);
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
	PNULL,//_ov2680_cfg_otp,
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
	"ov2680",		// name of sensor

	SENSOR_IMAGE_FORMAT_RAW,	// define in SENSOR_IMAGE_FORMAT_E enum,SENSOR_IMAGE_FORMAT_MAX
	// if set to SENSOR_IMAGE_FORMAT_MAX here, image format depent on SENSOR_REG_TAB_INFO_T

	SENSOR_IMAGE_PATTERN_RAWRGB_B,// pattern of input image form sensor;

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
};

LOCAL struct sensor_raw_info* Sensor_GetContext(void)
{
	return s_ov2680_mipi_raw_info_ptr;
}


LOCAL uint32_t Sensor_InitRawTuneInfo(void)
{
	uint32_t rtn=0x00;
	return rtn;
}

LOCAL unsigned long _ov2680_GetResolutionTrimTab(unsigned long param)
{
	SENSOR_PRINT("0x%lx", (unsigned long)s_ov2680_Resolution_Trim_Tab);
	return (unsigned long) s_ov2680_Resolution_Trim_Tab;
}

LOCAL unsigned long _ov2680_PowerOn(unsigned long power_on)
{
	SENSOR_AVDD_VAL_E dvdd_val = g_ov2680_mipi_raw_info.dvdd_val;
	SENSOR_AVDD_VAL_E avdd_val = g_ov2680_mipi_raw_info.avdd_val;
	SENSOR_AVDD_VAL_E iovdd_val = g_ov2680_mipi_raw_info.iovdd_val;
	BOOLEAN power_down = g_ov2680_mipi_raw_info.power_down_level;
	BOOLEAN reset_level = g_ov2680_mipi_raw_info.reset_pulse_level;

	if (SENSOR_TRUE == power_on) {
		Sensor_PowerDown_Ex(power_down, SENSOR_DEVICE2);
		Sensor_SetResetLevel_Ex(reset_level, SENSOR_DEVICE2);
		Sensor_SetMCLK_Ex(SENSOR_DEFALUT_MCLK, SENSOR_DEVICE2);
		usleep(1000);
		// Open power
		Sensor_SetAvddVoltage_Ex(avdd_val, SENSOR_DEVICE2);
		Sensor_SetIovddVoltage_Ex(iovdd_val, SENSOR_DEVICE2);
		usleep(1000);
		//_dw9174_SRCInit(2);
		Sensor_PowerDown_Ex(!power_down, SENSOR_DEVICE2);
		usleep(3*1000);  // > 8192*MCLK + 1ms
		// Reset sensor
		Sensor_SetResetLevel_Ex(!reset_level, SENSOR_DEVICE2);
		//usleep(20*1000);
	} else {
		Sensor_SetMCLK_Ex(SENSOR_DISABLE_MCLK, SENSOR_DEVICE2);
		usleep(1000); // >512 	mclk
		Sensor_PowerDown_Ex(power_down, SENSOR_DEVICE2);
		Sensor_SetResetLevel_Ex(reset_level, SENSOR_DEVICE2);
		usleep(500);
		Sensor_SetAvddVoltage_Ex(SENSOR_AVDD_CLOSED, SENSOR_DEVICE2);
		Sensor_SetIovddVoltage_Ex(SENSOR_AVDD_CLOSED, SENSOR_DEVICE2);
	}
	SENSOR_PRINT("SENSOR_ov2680: _ov2680_Power_On(1:on, 0:off): %d", power_on);
	return SENSOR_SUCCESS;
}

LOCAL unsigned long _ov2680_cfg_otp(unsigned long  param)
{
	uint32_t rtn=SENSOR_SUCCESS;
	struct raw_param_info_tab* tab_ptr = (struct raw_param_info_tab*)s_ov2680_raw_param_tab;
	uint32_t module_id=g_module_id;

	SENSOR_PRINT("SENSOR_ov2680: _ov2680_cfg_otp");

	if(PNULL!=tab_ptr[module_id].cfg_otp){
		tab_ptr[module_id].cfg_otp(0);
		}

	return rtn;
}

// index: index of otp group. (1, 2, 3)
// return: 0, group index is empty
// 1, group index has invalid data
// 2, group index has valid data
LOCAL uint32_t check_otp(int index)
{
	uint32_t flag = 0;
	uint32_t i = 0;
	uint32_t rg = 0;
	uint32_t bg = 0;

	if (index == 1)
	{
		// read otp --Bank 0
		Sensor_WriteReg_Ex(0x3d84, 0xc0, SENSOR_DEVICE2);
		Sensor_WriteReg_Ex(0x3d85, 0x00, SENSOR_DEVICE2);
		Sensor_WriteReg_Ex(0x3d86, 0x0f, SENSOR_DEVICE2);
		Sensor_WriteReg_Ex(0x3d81, 0x01, SENSOR_DEVICE2);
		usleep(5 * 1000);
		flag = Sensor_ReadReg_Ex(0x3d05, SENSOR_DEVICE2);
		rg = Sensor_ReadReg_Ex(0x3d07, SENSOR_DEVICE2);
		bg = Sensor_ReadReg_Ex(0x3d08, SENSOR_DEVICE2);
	}
	else if (index == 2)
	{
		// read otp --Bank 0
		Sensor_WriteReg_Ex(0x3d84, 0xc0, SENSOR_DEVICE2);
		Sensor_WriteReg_Ex(0x3d85, 0x00, SENSOR_DEVICE2);
		Sensor_WriteReg_Ex(0x3d86, 0x0f, SENSOR_DEVICE2);
		Sensor_WriteReg_Ex(0x3d81, 0x01, SENSOR_DEVICE2);
		usleep(5 * 1000);
		flag = Sensor_ReadReg_Ex(0x3d0e, SENSOR_DEVICE2);
		// read otp --Bank 1
		Sensor_WriteReg_Ex(0x3d84, 0xc0, SENSOR_DEVICE2);
		Sensor_WriteReg_Ex(0x3d85, 0x10, SENSOR_DEVICE2);
		Sensor_WriteReg_Ex(0x3d86, 0x1f, SENSOR_DEVICE2);
		Sensor_WriteReg_Ex(0x3d81, 0x01, SENSOR_DEVICE2);
		usleep(5 * 1000);
		rg = Sensor_ReadReg_Ex(0x3d00, SENSOR_DEVICE2);
		bg = Sensor_ReadReg_Ex(0x3d01, SENSOR_DEVICE2);
	}
	else if (index == 3)
	{
		// read otp --Bank 1
		Sensor_WriteReg_Ex(0x3d84, 0xc0, SENSOR_DEVICE2);
		Sensor_WriteReg_Ex(0x3d85, 0x10, SENSOR_DEVICE2);
		Sensor_WriteReg_Ex(0x3d86, 0x1f, SENSOR_DEVICE2);
		Sensor_WriteReg_Ex(0x3d81, 0x01, SENSOR_DEVICE2);
		usleep(5 * 1000);
		flag = Sensor_ReadReg_Ex(0x3d07, SENSOR_DEVICE2);
		rg = Sensor_ReadReg_Ex(0x3d09, SENSOR_DEVICE2);
		bg = Sensor_ReadReg_Ex(0x3d0a, SENSOR_DEVICE2);
	}
	SENSOR_PRINT("ov2680 check_otp: flag = 0x%d----index = %d---\n", flag, index);
	flag = flag & 0x80;
	// clear otp buffer
	for (i=0; i<16; i++) {
		Sensor_WriteReg_Ex(0x3d00 + i, 0x00, SENSOR_DEVICE2);
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
static int read_otp(int index, struct otp_struct *otp_ptr)
{
	int i = 0;
	int temp = 0;
	// read otp into buffer
	if (index == 1)
	{
		// read otp --Bank 0
		Sensor_WriteReg_Ex(0x3d84, 0xc0, SENSOR_DEVICE2);
		Sensor_WriteReg_Ex(0x3d85, 0x00, SENSOR_DEVICE2);
		Sensor_WriteReg_Ex(0x3d86, 0x0f, SENSOR_DEVICE2);
		Sensor_WriteReg_Ex(0x3d81, 0x01, SENSOR_DEVICE2);
		usleep(5 * 1000);
		(*otp_ptr).module_integrator_id = (Sensor_ReadReg_Ex(0x3d05, SENSOR_DEVICE2) & 0x7f);
		(*otp_ptr).lens_id = Sensor_ReadReg_Ex(0x3d06, SENSOR_DEVICE2);
		temp = Sensor_ReadReg_Ex(0x3d0b, SENSOR_DEVICE2);
		(*otp_ptr).rg_ratio = (Sensor_ReadReg_Ex(0x3d07, SENSOR_DEVICE2)<<2) + ((temp>>6) & 0x03);
		(*otp_ptr).bg_ratio = (Sensor_ReadReg_Ex(0x3d08, SENSOR_DEVICE2)<<2) + ((temp>>4) & 0x03);
		(*otp_ptr).light_rg = ((Sensor_ReadReg_Ex(0x3d0c, SENSOR_DEVICE2)<<2) + (temp>>2)) & 0x03;
		(*otp_ptr).light_bg = ((Sensor_ReadReg_Ex(0x3d0d, SENSOR_DEVICE2)<<2) + temp) & 0x03;

		(*otp_ptr).user_data[0] = Sensor_ReadReg_Ex(0x3d09, SENSOR_DEVICE2);
		(*otp_ptr).user_data[1] = Sensor_ReadReg_Ex(0x3d0a, SENSOR_DEVICE2);
	}
	else if (index == 2)
	{
		// read otp --Bank 0
		Sensor_WriteReg_Ex(0x3d84, 0xc0, SENSOR_DEVICE2);
		Sensor_WriteReg_Ex(0x3d85, 0x00, SENSOR_DEVICE2);
		Sensor_WriteReg_Ex(0x3d86, 0x0f, SENSOR_DEVICE2);
		Sensor_WriteReg_Ex(0x3d81, 0x01, SENSOR_DEVICE2);
		usleep(5 * 1000);
		(*otp_ptr).module_integrator_id = (Sensor_ReadReg_Ex(0x3d0e, SENSOR_DEVICE2) & 0x7f);
		(*otp_ptr).lens_id = Sensor_ReadReg_Ex(0x3d0f, SENSOR_DEVICE2);
		// read otp --Bank 1
		Sensor_WriteReg_Ex(0x3d84, 0xc0, SENSOR_DEVICE2);
		Sensor_WriteReg_Ex(0x3d85, 0x10, SENSOR_DEVICE2);
		Sensor_WriteReg_Ex(0x3d86, 0x1f, SENSOR_DEVICE2);
		Sensor_WriteReg_Ex(0x3d81, 0x01, SENSOR_DEVICE2);
		usleep(5 * 1000);
		temp = Sensor_ReadReg_Ex(0x3d04, SENSOR_DEVICE2);
		(*otp_ptr).rg_ratio = (Sensor_ReadReg_Ex(0x3d00, SENSOR_DEVICE2)<<2) + ((temp>>6) & 0x03);
		(*otp_ptr).bg_ratio = (Sensor_ReadReg_Ex(0x3d01, SENSOR_DEVICE2)<<2) + ((temp>>4) & 0x03);
		(*otp_ptr).light_rg = ((Sensor_ReadReg_Ex(0x3d05, SENSOR_DEVICE2)<<2) + (temp>>2)) & 0x03;
		(*otp_ptr).light_bg = ((Sensor_ReadReg_Ex(0x3d06, SENSOR_DEVICE2)<<2) + temp) & 0x03;
		(*otp_ptr).user_data[0] = Sensor_ReadReg_Ex(0x3d02, SENSOR_DEVICE2);
		(*otp_ptr).user_data[1] = Sensor_ReadReg_Ex(0x3d03, SENSOR_DEVICE2);
	}
	else if (index == 3)
	{
		// read otp --Bank 1
		Sensor_WriteReg_Ex(0x3d84, 0xc0, SENSOR_DEVICE2);
		Sensor_WriteReg_Ex(0x3d85, 0x10, SENSOR_DEVICE2);
		Sensor_WriteReg_Ex(0x3d86, 0x1f, SENSOR_DEVICE2);
		Sensor_WriteReg_Ex(0x3d81, 0x01, SENSOR_DEVICE2);
		usleep(5 * 1000);
		(*otp_ptr).module_integrator_id = (Sensor_ReadReg_Ex(0x3d07, SENSOR_DEVICE2) & 0x7f);
		(*otp_ptr).lens_id = Sensor_ReadReg_Ex(0x3d08, SENSOR_DEVICE2);
		temp = Sensor_ReadReg_Ex(0x3d0d, SENSOR_DEVICE2);
		(*otp_ptr).rg_ratio = (Sensor_ReadReg_Ex(0x3d09, SENSOR_DEVICE2)<<2) + ((temp>>6) & 0x03);
		(*otp_ptr).bg_ratio = (Sensor_ReadReg_Ex(0x3d0a, SENSOR_DEVICE2)<<2) + ((temp>>4) & 0x03);
		(*otp_ptr).light_rg = ((Sensor_ReadReg_Ex(0x3d0e, SENSOR_DEVICE2)<<2) + (temp>>2)) & 0x03;
		(*otp_ptr).light_bg = ((Sensor_ReadReg_Ex(0x3d0f, SENSOR_DEVICE2)<<2) + temp) & 0x03;
		(*otp_ptr).user_data[0] = Sensor_ReadReg_Ex(0x3d0b, SENSOR_DEVICE2);
		(*otp_ptr).user_data[1] = Sensor_ReadReg_Ex(0x3d0c ,SENSOR_DEVICE2);
	}

	// clear otp buffer
	for (i=0;i<16;i++) {
		Sensor_WriteReg_Ex(0x3d00 + i, 0x00, SENSOR_DEVICE2);
	}

	return 0;
}
// R_gain, sensor red gain of AWB, 0x400 =1
// G_gain, sensor green gain of AWB, 0x400 =1
// B_gain, sensor blue gain of AWB, 0x400 =1
// return 0;
static int update_awb_gain(int R_gain, int G_gain, int B_gain)
{
	if (R_gain>0x400) {
		Sensor_WriteReg_Ex(0x5186, R_gain>>8, SENSOR_DEVICE2);
		Sensor_WriteReg_Ex(0x5187, R_gain & 0x00ff, SENSOR_DEVICE2);
	}
	if (G_gain>0x400) {
		Sensor_WriteReg_Ex(0x5188, G_gain>>8, SENSOR_DEVICE2);
		Sensor_WriteReg_Ex(0x5189, G_gain & 0x00ff, SENSOR_DEVICE2);
	}
	if (B_gain>0x400) {
		Sensor_WriteReg_Ex(0x518a, B_gain>>8, SENSOR_DEVICE2);
		Sensor_WriteReg_Ex(0x518b, B_gain & 0x00ff, SENSOR_DEVICE2);
	}
	return 0;
}
// call this function after ov2680 initialization
// return: 0 update success
// 1, no OTP
LOCAL uint32_t update_otp (void* param_ptr)
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

	stream_value = Sensor_ReadReg_Ex(0x0100, SENSOR_DEVICE2);
	SENSOR_PRINT("ov2680 update_otp:stream_value = 0x%x\n", stream_value);
	if(1 != (stream_value & 0x01))
	{
		Sensor_WriteReg_Ex(0x0100, 0x01, SENSOR_DEVICE2);
		usleep(50 * 1000);
	}

	// R/G and B/G of current camera module is read out from sensor OTP
	// check first OTP with valid data
	for(i=1;i<=3;i++) {
		temp = check_otp(i);
		if (temp == 2) {
			otp_index = i;
			break;
		}
	}
	if (i>3) {
		// no valid wb OTP data
		return 1;
	}
	read_otp(otp_index, &current_otp);
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
	update_awb_gain(R_gain, G_gain, B_gain);

	if(1 != (stream_value & 0x01))
		Sensor_WriteReg_Ex(0x0100, stream_value, SENSOR_DEVICE2);

	SENSOR_PRINT("ov2680_otp: R_gain:0x%x, G_gain:0x%x, B_gain:0x%x------\n",R_gain, G_gain, B_gain);
	return 0;
}

LOCAL uint32_t ov2680_check_otp_module_id(void)
{
	struct otp_struct current_otp;
	int i = 0;
	int otp_index = 0;
	int temp = 0;
	uint16_t stream_value = 0;

	stream_value = Sensor_ReadReg_Ex(0x0100, SENSOR_DEVICE2);
	SENSOR_PRINT("ov2680_check_otp_module_id:stream_value = 0x%x\n", stream_value);
	if(1 != (stream_value & 0x01))
	{
		Sensor_WriteReg_Ex(0x0100, 0x01, SENSOR_DEVICE2);
		usleep(50 * 1000);
	}
	// R/G and B/G of current camera module is read out from sensor OTP
	// check first OTP with valid data

	for(i=1;i<=3;i++) {
		temp = check_otp(i);
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

	read_otp(otp_index, &current_otp);

	if(1 != (stream_value & 0x01))
		Sensor_WriteReg_Ex(0x0100, stream_value, SENSOR_DEVICE2);

	SENSOR_PRINT("read ov2680 otp  module_id = %x \n", current_otp.module_integrator_id);

	return current_otp.module_integrator_id;
}

LOCAL uint32_t _ov2680_Truly_Identify_otp(void* param_ptr)
{
	uint32_t rtn=SENSOR_FAIL;
	uint32_t param_id;

	SENSOR_PRINT("SENSOR_ov2680: _ov2680_Truly_Identify_otp");

	/*read param id from sensor omap*/
	param_id=ov2680_check_otp_module_id();;

	if(ov2680_RAW_PARAM_Truly==param_id){
		SENSOR_PRINT("SENSOR_ov2680: This is Truly module!!\n");
		RG_Ratio_Typical = 0x152;
		BG_Ratio_Typical = 0x137;
		rtn=SENSOR_SUCCESS;
	}

	return rtn;
}

LOCAL uint32_t _ov2680_Sunny_Identify_otp(void* param_ptr)
{
	uint32_t rtn=SENSOR_FAIL;
	uint32_t param_id;

	SENSOR_PRINT("SENSOR_ov2680: _ov2680_Sunny_Identify_otp");

	/*read param id from sensor omap*/
	param_id=ov2680_check_otp_module_id();

	if(ov2680_RAW_PARAM_Sunny==param_id){
		SENSOR_PRINT("SENSOR_ov2680: This is Sunny module!!\n");
		RG_Ratio_Typical = 386;
		BG_Ratio_Typical = 367;
		rtn=SENSOR_SUCCESS;
	}

	return rtn;
}

LOCAL uint32_t _ov2680_com_Identify_otp(void* param_ptr)
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

LOCAL uint32_t _ov2680_GetRawInof(void)
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
			if(SENSOR_SUCCESS==tab_ptr[i].identify_otp(0))
			{
				s_ov2680_mipi_raw_info_ptr = tab_ptr[i].info_ptr;
				SENSOR_PRINT("SENSOR_ov2680: ov2680_GetRawInof success");
				break;
			}
		}
	}

	return rtn;
}

LOCAL unsigned long _ov2680_GetMaxFrameLine(unsigned long index)
{
	uint32_t max_line=0x00;
	SENSOR_TRIM_T_PTR trim_ptr=s_ov2680_Resolution_Trim_Tab;

	max_line=trim_ptr[index].frame_line;

	return max_line;
}

LOCAL unsigned long _ov2680_Identify(unsigned long param)
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
	pid_value = Sensor_ReadReg_Ex(ov2680_PID_ADDR, SENSOR_DEVICE2);

	if (ov2680_PID_VALUE == pid_value) {
		ver_value = Sensor_ReadReg_Ex(ov2680_VER_ADDR, SENSOR_DEVICE2);
		SENSOR_PRINT("SENSOR_ov2680: Identify: PID = %x, VER = %x", pid_value, ver_value);
		if (ov2680_VER_VALUE == ver_value) {
			ret_value=_ov2680_GetRawInof();
			Sensor_InitRawTuneInfo();
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

LOCAL unsigned long _ov2680_write_exposure(unsigned long param)
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
	SENSOR_PRINT("SENSOR_ov2680: read reg :0x3820=%x\n", Sensor_ReadReg_Ex(0x3820, SENSOR_DEVICE2));
	SENSOR_PRINT("SENSOR_ov2680: read reg :0x3821=%x\n", Sensor_ReadReg_Ex(0x3821, SENSOR_DEVICE2));
	max_frame_len=_ov2680_GetMaxFrameLine(size_index);

	if(0x00!=max_frame_len)
	{
		frame_len = ((expsure_line+4)> max_frame_len) ? (expsure_line+4) : max_frame_len;

		if(0x00!=(0x01&frame_len))
		{
			frame_len+=0x01;
		}

		frame_len_cur = (Sensor_ReadReg_Ex(0x380e, SENSOR_DEVICE2)&0xff)<<8;
		frame_len_cur |= Sensor_ReadReg_Ex(0x380f, SENSOR_DEVICE2)&0xff;

		if(frame_len_cur != frame_len){
			value=(frame_len)&0xff;
			ret_value = Sensor_WriteReg_Ex(0x380f, value, SENSOR_DEVICE2);
			value=(frame_len>>0x08)&0xff;
			ret_value = Sensor_WriteReg_Ex(0x380e, value, SENSOR_DEVICE2);
		}
	}

	value=(expsure_line<<0x04)&0xff;
	ret_value = Sensor_WriteReg_Ex(0x3502, value, SENSOR_DEVICE2);
	value=(expsure_line>>0x04)&0xff;
	ret_value = Sensor_WriteReg_Ex(0x3501, value, SENSOR_DEVICE2);
	value=(expsure_line>>0x0c)&0x0f;
	ret_value = Sensor_WriteReg_Ex(0x3500, value, SENSOR_DEVICE2);

	return ret_value;
}

LOCAL unsigned long _ov2680_write_gain(unsigned long param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t value=0x00;
	uint32_t real_gain = 0;

	real_gain = ((param&0xf)+16)*(((param>>4)&0x01)+1)*(((param>>5)&0x01)+1)*(((param>>6)&0x01)+1)*(((param>>7)&0x01)+1);
	real_gain = real_gain*(((param>>8)&0x01)+1)*(((param>>9)&0x01)+1)*(((param>>10)&0x01)+1)*(((param>>11)&0x01)+1);

	SENSOR_PRINT("SENSOR_ov2680: real_gain:0x%x, param: 0x%x", real_gain, param);

	value = real_gain&0xff;
	ret_value = Sensor_WriteReg_Ex(0x350b, value, SENSOR_DEVICE2);/*0-7*/
	value = (real_gain>>0x08)&0x03;
	ret_value = Sensor_WriteReg_Ex(0x350a, value, SENSOR_DEVICE2);/*8*/

	return ret_value;
}

LOCAL unsigned long _ov2680_write_af(unsigned long param)
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
	ret_value = Sensor_WriteI2C_Ex(slave_addr,(uint8_t*)&cmd_val[0], cmd_len, SENSOR_DEVICE2);

	SENSOR_PRINT("SENSOR_ov2680: _write_af, ret =  %d, MSL:%x, LSL:%x\n", ret_value, cmd_val[0], cmd_val[1]);

	return ret_value;
}

LOCAL unsigned long _ov2680_ReadGain(unsigned long param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	uint16_t value=0x00;
	uint32_t gain = 0;

	value = Sensor_ReadReg_Ex(0x350b, SENSOR_DEVICE2);/*0-7*/
	gain = value&0xff;
	value = Sensor_ReadReg_Ex(0x350a, SENSOR_DEVICE2);/*8*/
	gain |= (value<<0x08)&0x300;

	s_ov2680_gain=(int)gain;

	SENSOR_PRINT("SENSOR_ov2680: _ov2680_ReadGain gain: 0x%x", s_ov2680_gain);

	return rtn;
}

LOCAL int _ov2680_set_gain16(int gain16)
{
	// write gain, 16 = 1x
	int temp;
	gain16 = gain16 & 0x3ff;

	temp = gain16 & 0xff;
	Sensor_WriteReg_Ex(0x350b, temp, SENSOR_DEVICE2);

	temp = gain16>>8;
	Sensor_WriteReg_Ex(0x350a, temp, SENSOR_DEVICE2);
	SENSOR_PRINT("gain %d",gain16);

	return 0;
}

LOCAL int _ov2680_set_shutter(int shutter)
{
	// write shutter, in number of line period
	int temp;

	shutter = shutter & 0xffff;

	temp = shutter & 0x0f;
	temp = temp<<4;
	Sensor_WriteReg_Ex(0x3502, temp, SENSOR_DEVICE2);

	temp = shutter & 0xfff;
	temp = temp>>4;
	Sensor_WriteReg_Ex(0x3501, temp, SENSOR_DEVICE2);

	temp = shutter>>12;
	Sensor_WriteReg_Ex(0x3500, temp, SENSOR_DEVICE2);

	SENSOR_PRINT("shutter %d",shutter);

	return 0;
}

LOCAL void _calculate_hdr_exposure(int capture_gain16,int capture_VTS, int capture_shutter)
{
	// write capture gain
	_ov2680_set_gain16(capture_gain16);

	_ov2680_set_shutter(capture_shutter);
}

LOCAL unsigned long _ov2680_SetEV(unsigned long param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	uint16_t value=0x00;
	uint32_t gain = s_ov2680_gain;
	uint32_t ev = param;

	SENSOR_PRINT("_ov2680_SetEV param: 0x%x,0x%x,0x%x,0x%x", param, s_ov2680_gain,s_capture_VTS,s_capture_shutter);

	switch(ev) {
	case SENSOR_HDR_EV_LEVE_0:
		_calculate_hdr_exposure(s_ov2680_gain/2,s_capture_VTS,s_capture_shutter/2);
		break;
	case SENSOR_HDR_EV_LEVE_1:
		_calculate_hdr_exposure(s_ov2680_gain,s_capture_VTS,s_capture_shutter);
		break;
	case SENSOR_HDR_EV_LEVE_2:
		_calculate_hdr_exposure(s_ov2680_gain,s_capture_VTS,s_capture_shutter*4);
		break;
	default:
		break;
	}
	return rtn;
}

LOCAL unsigned long _ov2680_ExtFunc(unsigned long ctl_param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR) ctl_param;
	SENSOR_PRINT("cmd %d",ext_ptr->cmd);
	switch (ext_ptr->cmd) {
		case SENSOR_EXT_EV:
			rtn = _ov2680_SetEV(ext_ptr->param);
			break;
		default:
			break;
	}

	return rtn;
}

int _ov2680_get_shutter(void)
{
	// read shutter, in number of line period
	int shutter;

	shutter = (Sensor_ReadReg_Ex(0x03500, SENSOR_DEVICE2) & 0x0f);
	shutter = (shutter<<8) + Sensor_ReadReg_Ex(0x3501, SENSOR_DEVICE2);
	shutter = (shutter<<4) + (Sensor_ReadReg_Ex(0x3502, SENSOR_DEVICE2)>>4);

	return shutter;
}

LOCAL int _ov2680_get_VTS(void)
{
	// read VTS from register settings
	int VTS;

	VTS = Sensor_ReadReg_Ex(0x380e, SENSOR_DEVICE2);//total vertical size[15:8] high byte

	VTS = (VTS<<8) + Sensor_ReadReg_Ex(0x380f, SENSOR_DEVICE2);

	return VTS;
}

LOCAL unsigned long _ov2680_PreBeforeSnapshot(unsigned long param)
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

	ret_h = (uint8_t) Sensor_ReadReg_Ex(0x3500, SENSOR_DEVICE2);
	ret_m = (uint8_t) Sensor_ReadReg_Ex(0x3501, SENSOR_DEVICE2);
	ret_l = (uint8_t) Sensor_ReadReg_Ex(0x3502, SENSOR_DEVICE2);
	preview_exposure = (ret_h << 12) + (ret_m << 4) + (ret_l >> 4);

	ret_h = (uint8_t) Sensor_ReadReg_Ex(0x380e, SENSOR_DEVICE2);
	ret_l = (uint8_t) Sensor_ReadReg_Ex(0x380f, SENSOR_DEVICE2);
	preview_maxline = (ret_h << 8) + ret_l;

	Sensor_SetMode_Ex(capture_mode, SENSOR_DEVICE2);
	Sensor_SetMode_WaitDone_Ex(SENSOR_DEVICE2);

	if (prv_linetime == cap_linetime) {
		SENSOR_PRINT("SENSOR_ov2680: prvline equal to capline");
		goto CFG_INFO;
	}

	ret_h = (uint8_t) Sensor_ReadReg_Ex(0x380e, SENSOR_DEVICE2);
	ret_l = (uint8_t) Sensor_ReadReg_Ex(0x380f, SENSOR_DEVICE2);
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
		Sensor_WriteReg_Ex(0x380e, ret_h, SENSOR_DEVICE2);
		Sensor_WriteReg_Ex(0x380f, ret_l, SENSOR_DEVICE2);
	}

	ret_l = ((unsigned char)capture_exposure&0xf) << 4;
	ret_m = (unsigned char)((capture_exposure&0xfff) >> 4) & 0xff;
	ret_h = (unsigned char)(capture_exposure >> 12);

	Sensor_WriteReg_Ex(0x3502, ret_l, SENSOR_DEVICE2);
	Sensor_WriteReg_Ex(0x3501, ret_m, SENSOR_DEVICE2);
	Sensor_WriteReg_Ex(0x3500, ret_h, SENSOR_DEVICE2);

	CFG_INFO:
	s_capture_shutter = _ov2680_get_shutter();
	s_capture_VTS = _ov2680_get_VTS();
	_ov2680_ReadGain(capture_mode);
	Sensor_SetSensorExifInfo_Ex(SENSOR_EXIF_CTRL_EXPOSURETIME, s_capture_shutter, SENSOR_DEVICE2);

	return SENSOR_SUCCESS;
}

LOCAL unsigned long _ov2680_BeforeSnapshot(unsigned long param)
{
	uint32_t cap_mode = (param>>CAP_MODE_BITS);
	uint32_t rtn = SENSOR_SUCCESS;

	SENSOR_PRINT("%d,%d.",cap_mode,param);

	rtn = _ov2680_PreBeforeSnapshot(param);

	return rtn;
}

LOCAL unsigned long _ov2680_after_snapshot(unsigned long param)
{
	SENSOR_PRINT("SENSOR_ov2680: after_snapshot mode:%ld", param);
	Sensor_SetMode_Ex((uint32_t)param, SENSOR_DEVICE2);

	return SENSOR_SUCCESS;
}

LOCAL unsigned long _ov2680_StreamOn(unsigned long param)
{
	SENSOR_PRINT("SENSOR_ov2680: StreamOn");

	Sensor_WriteReg_Ex(0x0100, 0x01, SENSOR_DEVICE2);

	return 0;
}

LOCAL unsigned long _ov2680_StreamOff(unsigned long param)
{
	SENSOR_PRINT("SENSOR_ov2680: StreamOff");

	Sensor_WriteReg_Ex(0x0100, 0x00, SENSOR_DEVICE2);
	usleep(100*1000);

	return 0;
}

LOCAL unsigned long _dw9174_SRCInit(unsigned long mode)
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
			Sensor_WriteI2C_Ex(slave_addr,(uint8_t*)&cmd_val[0], cmd_len, SENSOR_DEVICE2);
		}
		break;

		case 3:
		break;

	}

	return ret_value;
}
