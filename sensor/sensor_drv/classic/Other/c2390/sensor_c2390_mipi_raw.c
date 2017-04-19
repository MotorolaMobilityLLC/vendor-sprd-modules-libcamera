/*
* Copyright (C) 2012 The Android Open Source Project8u7
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
 * V4.0
 */


#include <utils/Log.h>
#include "sensor.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"
//#include "sensor_c2390_otp.c"
//#include "sensor_c2390_denoise.c"
#if 0 //defined(CONFIG_CAMERA_ISP_VERSION_V3) || defined(CONFIG_CAMERA_ISP_VERSION_V4)
//#include "sensor_c2390_raw_param_v3.c"
#include "sensor_c2390_raw_param_main.c"

#else
#include "parameters/sensor_c2390_raw_param_main.c"
#endif

#define SENSOR_NAME			"c2390"
#define I2C_SLAVE_ADDR		0x6c    /* 8bit slave address*/

#define c2390_PID_ADDR			0x0000
#define c2390_PID_VALUE			0x02
#define c2390_VER_ADDR			0x0001
#define c2390_VER_VALUE			0x03
/* sensor parameters begin */
/* effective sensor output image size */
#define SNAPSHOT_WIDTH			1920 
#define SNAPSHOT_HEIGHT			1080
#define PREVIEW_WIDTH			1920
#define PREVIEW_HEIGHT			1080

/*Mipi output*/
#define LANE_NUM				2
#define RAW_BITS				10

#define SNAPSHOT_MIPI_PER_LANE_BPS	  400
#define PREVIEW_MIPI_PER_LANE_BPS	  400

/*line time unit: 0.1us*/
#define SNAPSHOT_LINE_TIME		301500//
#define PREVIEW_LINE_TIME		301500//

/* frame length*/
#define SNAPSHOT_FRAME_LENGTH		1104 //0x450
#define PREVIEW_FRAME_LENGTH		1104 //0x450

/* please ref your spec */
#define FRAME_OFFSET			4
#define SENSOR_MAX_GAIN			0xFFFF              //256 multiple
#define SENSOR_BASE_GAIN		0x10
#define SENSOR_MIN_SHUTTER		2
#if 0//def CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
#undef CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
#endif

/* please ref your spec
 * 1 : average binning
 * 2 : sum-average binning
 * 4 : sum binning
 */
#define BINNING_FACTOR			1

/* please ref spec
 * 1: sensor auto caculate
 * 0: driver caculate
 */
#define SUPPORT_AUTO_FRAME_LENGTH	0
/* sensor parameters end */

/* isp parameters, please don't change it*/
#if defined(CONFIG_CAMERA_ISP_VERSION_V3) || defined(CONFIG_CAMERA_ISP_VERSION_V4)
#define ISP_BASE_GAIN			0x80
#else
#define ISP_BASE_GAIN			0x10
#endif
/* please don't change it */
#define EX_MCLK				24

/*struct hdr_info_t {
	uint32_t capture_max_shutter;
	uint32_t capture_shutter;
	uint32_t capture_gain;
};
 
struct sensor_ev_info_t {
	uint16_t preview_shutter;
	uint16_t preview_gain;
};
*/
//#define DYNA_BLC_ENABLE
#ifdef DYNA_BLC_ENABLE
static uint32_t BLC_FLAG=0;
#define GAIN_THRESHOLD_HIGH 0x6c
#define GAIN_THRESHOLD_LOW 0x50  
#define DATA_THRESHOLD 16
#endif

/*==============================================================================
 * Description:
 * global variable
 *============================================================================*/
static struct hdr_info_t s_hdr_info;
static uint32_t s_current_default_frame_length;
struct sensor_ev_info_t s_sensor_ev_info;

//#define FEATURE_OTP    /*OTP function switch*/

#ifdef FEATURE_OTP
#define MODULE_ID_NULL			0x0000
#define MODULE_ID_C2390_DL   0x0031


#define MODULE_ID_END			0xFFFF
#define LSC_PARAM_QTY 240
static uint32_t g_module_id = 0;


//#include "sensor_c2390_lightarray_otp.c"
//#include "sensor_c2390_qtech_otp.c"

struct raw_param_info_tab s_c2390_raw_param_tab[] = {
	{MODULE_ID_C2390_DL, &s_c2390_mipi_raw_info, _c2390_Identify_otp, PNULL},
	//{MODULE_ID_OV8856_lightarray, &s_c2390_mipi_raw_info, c2390_lightarray_identify_otp, c2390_lightarray_update_otp},
	{MODULE_ID_END, PNULL, PNULL, PNULL}
};

#endif



#define CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
#if 0//ndef CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
//#include "af_dw9714.h"
//#include "sensor_c2390_otp.c"


#define DW9714_VCM_SLAVE_ADDR (0x18 >> 1)
/*==============================================================================
 * Description:
 * init vcm driver DW9714
 * you can change this function acording your spec if it's necessary
 * mode:
 * 1: Direct Mode
 * 2: Dual Level Control Mode
 * 3: Linear Slope Cntrol Mode
 *============================================================================*/
 
static uint32_t dw9714_init(uint32_t mode)
{
	uint8_t cmd_val[2] = { 0x00 };
	uint16_t slave_addr = 0;
	uint16_t cmd_len = 0;
	uint32_t ret_value = SENSOR_SUCCESS;

	slave_addr = DW9714_VCM_SLAVE_ADDR;
	SENSOR_PRINT("mode = %d\n", mode);
	switch (mode) {
	case 1:
		/* When you use direct mode after power on, you don't need register set. Because, DLC disable is default.*/
		break;
	case 2:
		/*Protection off */
		cmd_val[0] = 0xec;
		cmd_val[1] = 0xa3;
		cmd_len = 2;
		ret_value = Sensor_WriteI2C(slave_addr, (uint8_t *) & cmd_val[0], cmd_len);

		/*DLC and MCLK[1:0] setting */
		cmd_val[0] = 0xa1;
		/*for better performace, cmd_val[1][1:0] should adjust to matching with Tvib of your camera VCM*/
		cmd_val[1] = 0x0e;
		cmd_len = 2;
		ret_value = Sensor_WriteI2C(slave_addr, (uint8_t *) & cmd_val[0], cmd_len);

		/*T_SRC[4:0] setting */
		cmd_val[0] = 0xf2;
		/*for better performace, cmd_val[1][7:3] should be adjusted to matching with Tvib of your camera VCM*/
		cmd_val[1] = 0x90;
		cmd_len = 2;
		ret_value = Sensor_WriteI2C(slave_addr, (uint8_t *) & cmd_val[0], cmd_len);

		/*Protection on */
		cmd_val[0] = 0xdc;
		cmd_val[1] = 0x51;
		cmd_len = 2;
		ret_value = Sensor_WriteI2C(slave_addr, (uint8_t *) & cmd_val[0], cmd_len);
		break;
	case 3:
		/*Protection off */
		cmd_val[0] = 0xec;
		cmd_val[1] = 0xa3;
		cmd_len = 2;
		ret_value = Sensor_WriteI2C(slave_addr, (uint8_t *) & cmd_val[0], cmd_len);

		/*DLC and MCLK[1:0] setting */
		cmd_val[0] = 0xa1;
		cmd_val[1] = 0x05;
		cmd_len = 2;
		ret_value = Sensor_WriteI2C(slave_addr, (uint8_t *) & cmd_val[0], cmd_len);

		/*T_SRC[4:0] setting */
		cmd_val[0] = 0xf2;
		/*for better performace, cmd_val[1][7:3] should be adjusted to matching with the Tvib of your camera VCM*/
		cmd_val[1] = 0x00;
		cmd_len = 2;
		ret_value = Sensor_WriteI2C(slave_addr, (uint8_t *) & cmd_val[0], cmd_len);

		/*Protection on */
		cmd_val[0] = 0xdc;
		cmd_val[1] = 0x51;
		cmd_len = 2;
		ret_value = Sensor_WriteI2C(slave_addr, (uint8_t *) & cmd_val[0], cmd_len);
		break;
	}

	return ret_value;
}

/*==============================================================================
 * Description:
 * write parameter to vcm driver IC DW9714
 * you can change this function acording your spec if it's necessary
 * uint16_t s_4bit: for better performace of linear slope control, s_4bit should be given a value corresponding to step period
 *============================================================================*/
static uint32_t dw9714_write_af(uint32_t param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint8_t cmd_val[2] = { 0x00 };
	uint16_t slave_addr = 0;
	uint16_t cmd_len = 0;
	uint16_t s_4bit = 0x09;

	SENSOR_PRINT("%d", param);

	slave_addr = DW9714_VCM_SLAVE_ADDR;
	cmd_val[0] = (param & 0xfff0) >> 4;
	cmd_val[1] = ((param & 0x0f) << 4) | s_4bit;
	cmd_len = 2;
	ret_value = Sensor_WriteI2C(slave_addr, (uint8_t *) & cmd_val[0], cmd_len);

	return ret_value;
}
#endif


static SENSOR_IOCTL_FUNC_TAB_T s_c2390_ioctl_func_tab;
struct sensor_raw_info *s_c2390_mipi_raw_info_ptr = &s_c2390_mipi_raw_info;

static const SENSOR_REG_T c2390_init_setting[] = {
{0x0103,0x01},
{SENSOR_WRITE_DELAY, 0x1a},  
{0x0003,0x00},
{0x0003,0x00},
{0x0003,0x00},
{0x0003,0x00},
{0x0003,0x00},
{0x0003,0x00},
{0x0003,0x00},
{0x0003,0x00},
{0x0003,0x00},
{0x0003,0x00},
{0x0003,0x00},
{0x0101,0x01},
{0x3000,0x80},
{0x3080,0x01},
{0x3081,0x14},
{0x3082,0x01},
{0x3083,0x4b},
{0x3087,0xd0},
{0x3089,0x10},
{0x3180,0x10},
{0x3182,0x30},
{0x3183,0x10},
{0x3184,0x20},
{0x3185,0xc0},
{0x3189,0x50},
{0x3c03,0x00},
{0x3f8c,0x00},
{0x320f,0x48},
{0x3023,0x00},
{0x3d00,0x33},
{0x3c9d,0x01},
{0x3f08,0x00},
{0x0309,0x2e},
{0x0303,0x01},
{0x0304,0x01},
{0x0307,0x56},
{0x3508,0x00},
{0x3509,0xcc},
{0x3292,0x28},
{0x350a,0x22},
{0x3209,0x05},
{0x0003,0x00},
{0x0003,0x00},
{0x0003,0x00},
{0x0003,0x00},
{0x3209,0x04},
{0x3108,0xcd},
{0x3109,0xbf},
{0x310a,0x82},
{0x310b,0x42},
{0x3112,0x43},
{0x3113,0x01},
{0x3114,0xc0},
{0x3115,0x40},
{0x3905,0x01},
{0x3980,0x01},
{0x3881,0x04},
{0x3882,0x11},
{0x328b,0x03},
{0x328c,0x00},
{0x3981,0x57},
{0x3180,0x10},
{0x3213,0x00},
{0x3205,0x40},
{0x3208,0x8d},
{0x3210,0x12},
{0x3211,0x40},
{0x3212,0x50},
{0x3215,0xc0},
{0x3216,0x70},
{0x3217,0x08},
{0x3218,0x20},
{0x321a,0x80},
{0x321b,0x00},
{0x321c,0x1a},
{0x321e,0x00},
{0x3223,0x20},
{0x3224,0x88},
{0x3225,0x00},
{0x3226,0x08},
{0x3227,0x00},
{0x3228,0x00},
{0x3229,0x08},
{0x322a,0x00},
{0x322b,0x44},
{0x308a,0x00},
{0x308b,0x00},
{0x3280,0x06},
{0x3281,0x30},
{0x3282,0x08},
{0x3283,0x51},
{0x3284,0x0d},
{0x3285,0x48},
{0x3286,0x3b},
{0x3287,0x07},
{0x3288,0x00},
{0x3289,0x00},
{0x328a,0x08},
{0x328d,0x01},
{0x328e,0x20},
{0x328f,0x0d},
{0x3290,0x10},
{0x3291,0x00},
{0x3292,0x28},
{0x3293,0x00},
{0x3216,0x50},
{0x3217,0x04},
{0x3205,0x20},
{0x3215,0x50},
{0x3223,0x10},
{0x3280,0x06},
{0x3282,0x0a},
{0x3283,0x50},
{0x308b,0x00},
{0x3184,0x20},
{0x3185,0xc0},
{0x3189,0x50},
{0x3280,0x86},
{0x0003,0x00},
{0x3280,0x06},
{0x0383,0x01},
{0x0387,0x01},
{0x0340,0x04},
{0x0341,0x50},
{0x0342,0x08},
{0x0343,0x1c},
{0x034c,0x07},
{0x034d,0x80},
{0x034e,0x04},
{0x034f,0x38},
{0x3b80,0x42},
{0x3b81,0x10},
{0x3b82,0x10},
{0x3b83,0x10},
{0x3b84,0x04},
{0x3b85,0x04},
{0x3b86,0x80},
{0x3b86,0x80},
{0x3021,0x11},
{0x3022,0x22},
{0x3209,0x04},
{0x3584,0x12},
{0x3805,0x05},
{0x3806,0x03},
{0x3807,0x03},
{0x3808,0x0c},
{0x3809,0x64},
{0x380a,0x5b},
{0x380b,0xe6},
{0x3500,0x10},
{0x308c,0x20},
{0x308d,0x31},
{0x3403,0x00},
{0x3407,0x01},
{0x3410,0x04},
{0x3414,0x01},
{0xe000,0x32},
{0xe001,0x85},
{0xe002,0x48},
{0xe030,0x32},
{0xe031,0x85},
{0xe032,0x48},
{0x3500,0x00},
{0x3a87,0x02},
{0x3a88,0x08},
{0x3a89,0x30},
{0x3a8a,0x01},
{0x3a8b,0x90},
{0x3a80,0x88},
{0x3a81,0x02},
{0x3009,0x09},
{0x300b,0x08},
{0x034b,0x47},
{0x0202,0x01},
{0x0203,0x4c},


///////////////////////////
{0x3500,0x10},
{0x3404,0x04},
{0xe000,0x02},
{0xe001,0x05},
{0xe002,0x00},
{0xe003,0x32},
{0xe004,0x8e},
{0xe005,0x20},
{0xe006,0x02},
{0xe007,0x16},
{0xe008,0x01},
{0xe009,0x02},
{0xe00a,0x17},
{0xe00b,0x00},
{0x3500,0x00},

//////////////////////////

};

static const SENSOR_REG_T c2390_preview_setting[] = {
	
};

static const SENSOR_REG_T c2390_snapshot_setting[] = {

};

static SENSOR_REG_TAB_INFO_T s_c2390_resolution_tab_raw[] = {
	{ADDR_AND_LEN_OF_ARRAY(c2390_init_setting), 0, 0, EX_MCLK,
	 SENSOR_IMAGE_FORMAT_RAW},
	{ADDR_AND_LEN_OF_ARRAY(c2390_preview_setting),
	 PREVIEW_WIDTH, PREVIEW_HEIGHT, EX_MCLK,
	 SENSOR_IMAGE_FORMAT_RAW},
	{ADDR_AND_LEN_OF_ARRAY(c2390_snapshot_setting),
	 SNAPSHOT_WIDTH, SNAPSHOT_HEIGHT, EX_MCLK,
	 SENSOR_IMAGE_FORMAT_RAW},
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

static SENSOR_TRIM_T s_c2390_resolution_trim_tab[] = {
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, PREVIEW_WIDTH, PREVIEW_HEIGHT,
	 PREVIEW_LINE_TIME, PREVIEW_MIPI_PER_LANE_BPS, PREVIEW_FRAME_LENGTH,
	 {0, 0, PREVIEW_WIDTH, PREVIEW_HEIGHT}},
	{0, 0, SNAPSHOT_WIDTH, SNAPSHOT_HEIGHT,
	 SNAPSHOT_LINE_TIME, SNAPSHOT_MIPI_PER_LANE_BPS, SNAPSHOT_FRAME_LENGTH,
	 {0, 0, SNAPSHOT_WIDTH, SNAPSHOT_HEIGHT}},
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

static const SENSOR_REG_T s_c2390_preview_size_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
	/*video mode 0: ?fps */
	{
	 {0xffff, 0xff}
	 },
	/* video mode 1:?fps */
	{
	 {0xffff, 0xff}
	 },
	/* video mode 2:?fps */
	{
	 {0xffff, 0xff}
	 },
	/* video mode 3:?fps */
	{
	 {0xffff, 0xff}
	 }
};

static const SENSOR_REG_T s_c2390_capture_size_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
	/*video mode 0: ?fps */
	{
	 {0xffff, 0xff}
	 },
	/* video mode 1:?fps */
	{
	 {0xffff, 0xff}
	 },
	/* video mode 2:?fps */
	{
	 {0xffff, 0xff}
	 },
	/* video mode 3:?fps */
	{
	 {0xffff, 0xff}
	 }
};

static SENSOR_VIDEO_INFO_T s_c2390_video_info[SENSOR_MODE_MAX] = {
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{30, 30, 334, 90}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
	 (SENSOR_REG_T **) s_c2390_preview_size_video_tab},
	{{{15, 15, 334, 1000}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
	 (SENSOR_REG_T **) s_c2390_capture_size_video_tab},
};

/*==============================================================================
 * Description:
 * set video mode
 *
 *============================================================================*/
static uint32_t c2390_set_video_mode(SENSOR_HW_HANDLE handle,uint32_t param)
{
	SENSOR_REG_T_PTR sensor_reg_ptr;
	uint16_t i = 0x00;
	uint32_t mode;

	if (param >= SENSOR_VIDEO_MODE_MAX)
		return 0;

	if (SENSOR_SUCCESS != Sensor_GetMode(&mode)) {
		SENSOR_PRINT("fail.");
		return SENSOR_FAIL;
	}

	if (PNULL == s_c2390_video_info[mode].setting_ptr) {
		SENSOR_PRINT("fail.");
		return SENSOR_FAIL;
	}

	sensor_reg_ptr = (SENSOR_REG_T_PTR) & s_c2390_video_info[mode].setting_ptr[param];
	if (PNULL == sensor_reg_ptr) {
		SENSOR_PRINT("fail.");
		return SENSOR_FAIL;
	}

	for (i = 0x00; (0xffff != sensor_reg_ptr[i].reg_addr)
	     || (0xff != sensor_reg_ptr[i].reg_value); i++) {
		Sensor_WriteReg(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
	}

	return 0;
}

/*==============================================================================
 * Description:
 * sensor all info
 * please modify this variable acording your spec
 *============================================================================*/
SENSOR_INFO_T g_c2390_mipi_raw_info = {
	/* salve i2c write address */
	(I2C_SLAVE_ADDR >> 1),
	/* salve i2c read address */
	(I2C_SLAVE_ADDR >> 1),
	/*bit0: 0: i2c register value is 8 bit, 1: i2c register value is 16 bit */
	SENSOR_I2C_REG_16BIT | SENSOR_I2C_VAL_8BIT | SENSOR_I2C_FREQ_400,
	/* bit2: 0:negative; 1:positive -> polarily of horizontal synchronization signal
	 * bit4: 0:negative; 1:positive -> polarily of vertical synchronization signal
	 * other bit: reseved
	 */
	SENSOR_HW_SIGNAL_PCLK_P | SENSOR_HW_SIGNAL_VSYNC_P | SENSOR_HW_SIGNAL_HSYNC_P,
	/* preview mode */
	SENSOR_ENVIROMENT_NORMAL | SENSOR_ENVIROMENT_NIGHT,
	/* image effect */
	SENSOR_IMAGE_EFFECT_NORMAL |
	    SENSOR_IMAGE_EFFECT_BLACKWHITE |
	    SENSOR_IMAGE_EFFECT_RED |
	    SENSOR_IMAGE_EFFECT_GREEN | SENSOR_IMAGE_EFFECT_BLUE | SENSOR_IMAGE_EFFECT_YELLOW |
	    SENSOR_IMAGE_EFFECT_NEGATIVE | SENSOR_IMAGE_EFFECT_CANVAS,

	/* while balance mode */
	0,
	/* bit[0:7]: count of step in brightness, contrast, sharpness, saturation
	 * bit[8:31] reseved
	 */
	7,
	/* reset pulse level */
	SENSOR_LOW_PULSE_RESET,
	/* reset pulse width(ms) */
	50,
	/* 1: high level valid; 0: low level valid */
	SENSOR_LOW_LEVEL_PWDN,
	/* count of identify code */
	1,
	/* supply two code to identify sensor.
	 * for Example: index = 0-> Device id, index = 1 -> version id
	 * customer could ignore it.
	 */
	{{c2390_PID_ADDR, c2390_PID_VALUE}
	 ,
	 {c2390_VER_ADDR, c2390_VER_VALUE}
	 }
	,
	/* voltage of avdd */
	SENSOR_AVDD_3300MV, 
	/* max width of source image */
	SNAPSHOT_WIDTH,
	/* max height of source image */
	SNAPSHOT_HEIGHT,
	/* name of sensor */
	SENSOR_NAME,
	/* define in SENSOR_IMAGE_FORMAT_E enum,SENSOR_IMAGE_FORMAT_MAX
	 * if set to SENSOR_IMAGE_FORMAT_MAX here,
	 * image format depent on SENSOR_REG_TAB_INFO_T
	 */
	SENSOR_IMAGE_FORMAT_RAW,
	/*  pattern of input image form sensor */
	SENSOR_IMAGE_PATTERN_RAWRGB_B,
	/* point to resolution table information structure */
	s_c2390_resolution_tab_raw,
	/* point to ioctl function table */
	&s_c2390_ioctl_func_tab,
	/* information and table about Rawrgb sensor */
	&s_c2390_mipi_raw_info_ptr,
	/* extend information about sensor
	 * like &g_c2390_ext_info
	 */
	NULL,
	/* voltage of iovdd */
	SENSOR_AVDD_1800MV,
	/* voltage of dvdd */
	SENSOR_AVDD_1500MV,
	/* skip frame num before preview <3 zsl function will work!!! */   
	2,
	/* skip frame num before capture */
	3,0,0,
	/* deci frame num during preview */
	0,
	/* deci frame num during video preview */
	0,
	0,
	0,
	0,
	0,
	0,
	{SENSOR_INTERFACE_TYPE_CSI2, LANE_NUM, RAW_BITS, 0},
	0,
	/* skip frame num while change setting */
	2,
	/* horizontal  view angle*/
	65,
	/* vertical view angle*/
	60,
	(cmr_s8 *)"c2390_v1",
};

static SENSOR_STATIC_INFO_T s_c2390_static_info = {
	220,	//f-number,focal ratio
	346,	//focal_length;
	0,	//max_fps,max fps of sensor's all settings,it will be calculated from sensor mode fps
	8, //max_adgain,AD-gain
	0,	//ois_supported;
	0,	//pdaf_supported;
	1,	//exp_valid_frame_num;N+2-1
	0,	//clamp_level,black level
	1,	//adgain_valid_frame_num;N+1-1
 };


static SENSOR_MODE_FPS_INFO_T s_c2390_mode_fps_info = {
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
/*==============================================================================
 * Description:
 * calculate fps for every sensor mode according to frame_line and line_time
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t c2390_init_mode_fps_info(SENSOR_HW_HANDLE handle)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_PRINT("c2390_init_mode_fps_info:E");
	if(!s_c2390_mode_fps_info.is_init) {
		uint32_t i,modn,tempfps = 0;
		SENSOR_PRINT("c2390_init_mode_fps_info:start init");
		for(i = 0;i < NUMBER_OF_ARRAY(s_c2390_resolution_trim_tab); i++) {
			//max fps should be multiple of 30,it calulated from line_time and frame_line
			tempfps = s_c2390_resolution_trim_tab[i].line_time*s_c2390_resolution_trim_tab[i].frame_line;
				if(0 != tempfps) {
					tempfps = 1000000000/tempfps;
				modn = tempfps / 30;
				if(tempfps > modn*30)
					modn++;
				s_c2390_mode_fps_info.sensor_mode_fps[i].max_fps = modn*30;
				if(s_c2390_mode_fps_info.sensor_mode_fps[i].max_fps > 30) {
					s_c2390_mode_fps_info.sensor_mode_fps[i].is_high_fps = 1;
					s_c2390_mode_fps_info.sensor_mode_fps[i].high_fps_skip_num =
						s_c2390_mode_fps_info.sensor_mode_fps[i].max_fps/30;
				}
				if(s_c2390_mode_fps_info.sensor_mode_fps[i].max_fps >
						s_c2390_static_info.max_fps) {
					s_c2390_static_info.max_fps =
						s_c2390_mode_fps_info.sensor_mode_fps[i].max_fps;
				}
			}
			SENSOR_PRINT("mode %d,tempfps %d,frame_len %d,line_time: %d ",i,tempfps,
					s_c2390_resolution_trim_tab[i].frame_line,
					s_c2390_resolution_trim_tab[i].line_time);
			SENSOR_PRINT("mode %d,max_fps: %d ",
					i,s_c2390_mode_fps_info.sensor_mode_fps[i].max_fps);
			SENSOR_PRINT("is_high_fps: %d,highfps_skip_num %d",
					s_c2390_mode_fps_info.sensor_mode_fps[i].is_high_fps,
					s_c2390_mode_fps_info.sensor_mode_fps[i].high_fps_skip_num);
		}
		s_c2390_mode_fps_info.is_init = 1;
	}
	SENSOR_PRINT("c2390_init_mode_fps_info:X");
	return rtn;
}

static uint32_t c2390_get_static_info(SENSOR_HW_HANDLE handle, uint32_t *param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	struct sensor_ex_info *ex_info;
	uint32_t up = 0;
	uint32_t down = 0;
	//make sure we have get max fps of all settings.
	if(!s_c2390_mode_fps_info.is_init) {
		c2390_init_mode_fps_info(handle);
	}
	ex_info = (struct sensor_ex_info*)param;
	ex_info->f_num = s_c2390_static_info.f_num;
	ex_info->focal_length = s_c2390_static_info.focal_length;
	ex_info->max_fps = s_c2390_static_info.max_fps;
	ex_info->max_adgain = s_c2390_static_info.max_adgain;
	ex_info->ois_supported = s_c2390_static_info.ois_supported;
	ex_info->pdaf_supported = s_c2390_static_info.pdaf_supported;
	ex_info->exp_valid_frame_num = s_c2390_static_info.exp_valid_frame_num;
	ex_info->clamp_level = s_c2390_static_info.clamp_level;
	ex_info->adgain_valid_frame_num = s_c2390_static_info.adgain_valid_frame_num;
	ex_info->preview_skip_num = g_c2390_mipi_raw_info.preview_skip_num;
	ex_info->capture_skip_num = g_c2390_mipi_raw_info.capture_skip_num;
	ex_info->name = g_c2390_mipi_raw_info.name;
	ex_info->sensor_version_info = g_c2390_mipi_raw_info.sensor_version_info;
	//vcm_ak7371_get_pose_dis(handle, &up, &down);
	ex_info->pos_dis.up2hori = up;
	ex_info->pos_dis.hori2down = down;
	SENSOR_PRINT("f_num: %d", ex_info->f_num);
	SENSOR_PRINT("max_fps: %d", ex_info->max_fps);
	SENSOR_PRINT("max_adgain: %d", ex_info->max_adgain);
	SENSOR_PRINT("ois_supported: %d", ex_info->ois_supported);
	SENSOR_PRINT("pdaf_supported: %d", ex_info->pdaf_supported);
	SENSOR_PRINT("exp_valid_frame_num: %d", ex_info->exp_valid_frame_num);
	SENSOR_PRINT("clam_level: %d", ex_info->clamp_level);
	SENSOR_PRINT("adgain_valid_frame_num: %d", ex_info->adgain_valid_frame_num);
	SENSOR_PRINT("sensor name is: %s", ex_info->name);
	SENSOR_PRINT("sensor version info is: %s", ex_info->sensor_version_info);

	return rtn;
}


static uint32_t c2390_get_fps_info(SENSOR_HW_HANDLE handle, uint32_t *param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_MODE_FPS_T *fps_info;
	//make sure have inited fps of every sensor mode.
	if(!s_c2390_mode_fps_info.is_init) {
		c2390_init_mode_fps_info(handle);
	}
	fps_info = (SENSOR_MODE_FPS_T*)param;
	uint32_t sensor_mode = fps_info->mode;
	fps_info->max_fps = s_c2390_mode_fps_info.sensor_mode_fps[sensor_mode].max_fps;
	fps_info->min_fps = s_c2390_mode_fps_info.sensor_mode_fps[sensor_mode].min_fps;
	fps_info->is_high_fps = s_c2390_mode_fps_info.sensor_mode_fps[sensor_mode].is_high_fps;
	fps_info->high_fps_skip_num = s_c2390_mode_fps_info.sensor_mode_fps[sensor_mode].high_fps_skip_num;
	SENSOR_PRINT("mode %d, max_fps: %d",fps_info->mode, fps_info->max_fps);
	SENSOR_PRINT("min_fps: %d", fps_info->min_fps);
	SENSOR_PRINT("is_high_fps: %d", fps_info->is_high_fps);
	SENSOR_PRINT("high_fps_skip_num: %d", fps_info->high_fps_skip_num);

	return rtn;
}


#if 0//defined(CONFIG_CAMERA_ISP_VERSION_V3) || defined(CONFIG_CAMERA_ISP_VERSION_V4)



#define param_update(x1,x2) sprintf(name,"/data/c2390_%s.bin",x1);\
				if(0==access(name,R_OK))\
				{\
					FILE* fp = NULL;\
					SENSOR_PRINT("param file %s exists",name);\
					if( NULL!=(fp=fopen(name,"rb")) ){\
						fread((void*)x2,1,sizeof(x2),fp);\
						fclose(fp);\
					}else{\
						SENSOR_PRINT("param open %s failure",name);\
					}\
				}\
				memset(name,0,sizeof(name))


static uint32_t c2390_InitRawTuneInfo(void)
{
	uint32_t rtn=0x00;

	isp_raw_para_update_from_file(&g_c2390_mipi_raw_info,0);

	struct sensor_raw_info* raw_sensor_ptr=s_c2390_mipi_raw_info_ptr;
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
					#include "NR/pwd_param.h"
				};

				param_update("pwd_param",pwd_param);

				block->param_ptr = pwd_param;
			}
			break;

		case	ISP_BLK_BPC_V1: {
				/* modify block data */
				struct sensor_bpc_param_v1* block = (struct sensor_bpc_param_v1*)data;

				static struct sensor_bpc_level bpc_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "NR/bpc_param.h"
				};

				param_update("bpc_param",bpc_param);

				block->param_ptr = bpc_param;
			}
			break;

		case	ISP_BLK_BL_NR_V1: {
				/* modify block data */
				struct sensor_bdn_param* block = (struct sensor_bdn_param*)data;

				static struct sensor_bdn_level bdn_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "NR/bdn_param.h"
				};

				param_update("bdn_param",bdn_param);

				block->param_ptr = bdn_param;
			}
			break;

		case	ISP_BLK_GRGB_V1: {
				/* modify block data */
				struct sensor_grgb_v1_param* block = (struct sensor_grgb_v1_param*)data;
				static struct sensor_grgb_v1_level grgb_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "NR/grgb_param.h"
				};

				param_update("grgb_param",grgb_param);

				block->param_ptr = grgb_param;

			}
			break;

		case	ISP_BLK_NLM: {
				/* modify block data */
				struct sensor_nlm_param* block = (struct sensor_nlm_param*)data;

				static struct sensor_nlm_level nlm_param[32] = {
					#include "NR/nlm_param.h"
				};

				param_update("nlm_param",nlm_param);

				static struct sensor_vst_level vst_param[32] = {
					#include "NR/vst_param.h"
				};

				param_update("vst_param",vst_param);

				static struct sensor_ivst_level ivst_param[32] = {
					#include "NR/ivst_param.h"
				};

				param_update("ivst_param",ivst_param);

				static struct sensor_flat_offset_level flat_offset_param[32] = {
					#include "NR/flat_offset_param.h"
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
					#include "NR/cfae_param.h"
				};

				param_update("cfae_param",cfae_param);

				block->param_ptr = cfae_param;
			}
			break;

		case	ISP_BLK_RGB_PRECDN: {
				/* modify block data */
				struct sensor_rgb_precdn_param* block = (struct sensor_rgb_precdn_param*)data;

				static struct sensor_rgb_precdn_level precdn_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "NR/rgb_precdn_param.h"
				};

				param_update("rgb_precdn_param",precdn_param);

				block->param_ptr = precdn_param;
			}
			break;

		case	ISP_BLK_YUV_PRECDN: {
				/* modify block data */
				struct sensor_yuv_precdn_param* block = (struct sensor_yuv_precdn_param*)data;

				static struct sensor_yuv_precdn_level yuv_precdn_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "NR/yuv_precdn_param.h"
				};

				param_update("yuv_precdn_param",yuv_precdn_param);

				block->param_ptr = yuv_precdn_param;
			}
			break;

		case	ISP_BLK_PREF_V1: {
				/* modify block data */
				struct sensor_prfy_param* block = (struct sensor_prfy_param*)data;

				static struct sensor_prfy_level prfy_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "NR/prfy_param.h"
				};

				param_update("prfy_param",prfy_param);

				block->param_ptr = prfy_param;
			}
			break;

		case	ISP_BLK_UV_CDN: {
				/* modify block data */
				struct sensor_uv_cdn_param* block = (struct sensor_uv_cdn_param*)data;

				static struct sensor_uv_cdn_level uv_cdn_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "NR/yuv_cdn_param.h"
				};

				param_update("yuv_cdn_param",uv_cdn_param);

				block->param_ptr = uv_cdn_param;
			}
			break;

		case	ISP_BLK_EDGE_V1: {
				/* modify block data */
				struct sensor_ee_param* block = (struct sensor_ee_param*)data;

				static struct sensor_ee_level edge_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "NR/edge_param.h"
				};

				param_update("edge_param",edge_param);

				block->param_ptr = edge_param;
			}
			break;

		case	ISP_BLK_UV_POSTCDN: {
				/* modify block data */
				struct sensor_uv_postcdn_param* block = (struct sensor_uv_postcdn_param*)data;

				static struct sensor_uv_postcdn_level uv_postcdn_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "NR/yuv_postcdn_param.h"
				};

				param_update("yuv_postcdn_param",uv_postcdn_param);

				block->param_ptr = uv_postcdn_param;
			}
			break;

		case	ISP_BLK_IIRCNR_IIR: {
				/* modify block data */
				struct sensor_iircnr_param* block = (struct sensor_iircnr_param*)data;

				static struct sensor_iircnr_level iir_cnr_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "NR/iircnr_param.h"
				};

				param_update("iircnr_param",iir_cnr_param);

				block->param_ptr = iir_cnr_param;
			}
			break;

		case	ISP_BLK_IIRCNR_YRANDOM: {
				/* modify block data */
				struct sensor_iircnr_yrandom_param* block = (struct sensor_iircnr_yrandom_param*)data;
				static struct sensor_iircnr_yrandom_level iir_yrandom_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "NR/iir_yrandom_param.h"
				};

				param_update("iir_yrandom_param",iir_yrandom_param);

				block->param_ptr = iir_yrandom_param;
			}
			break;

		case  ISP_BLK_UVDIV_V1: {
				/* modify block data */
				struct sensor_cce_uvdiv_param_v1* block = (struct sensor_cce_uvdiv_param_v1*)data;

				static struct sensor_cce_uvdiv_level cce_uvdiv_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "NR/cce_uv_param.h"
				};

				param_update("cce_uv_param",cce_uvdiv_param);

				block->param_ptr = cce_uvdiv_param;
			}
			break;
		case ISP_BLK_YIQ_AFM:{
			/* modify block data */
			struct sensor_y_afm_param *block = (struct sensor_y_afm_param*)data;

			static struct sensor_y_afm_level y_afm_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "NR/y_afm_param.h"
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

/*==============================================================================
 * Description:
 * get default frame length
 *
 *============================================================================*/
static uint32_t c2390_get_default_frame_length(uint32_t mode)
{
	return s_c2390_resolution_trim_tab[mode].frame_line;
}

/*==============================================================================
 * Description:
 * write group-hold on to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void c2390_group_hold_on(void)
{
	SENSOR_PRINT("E");

}

/*==============================================================================
 * Description:
 * write group-hold off to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void c2390_group_hold_off(void)
{
	SENSOR_PRINT("E");

}


/*==============================================================================
 * Description:
 * read gain from sensor registers
 * please modify this function acording your spec
 *============================================================================*/

static uint16_t c2390_read_gain(SENSOR_HW_HANDLE handle)
{
	uint16_t gain_h = 0;
	uint16_t gain_l = 0;

	gain_h = 0;//Sensor_ReadReg(0x0205) & 0xff;
	gain_l = Sensor_ReadReg(0x0205) & 0xff;

	return ((gain_h << 8) | gain_l);
}





/*==============================================================================
 * Description:
 * write gain to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void c2390_write_gain(SENSOR_HW_HANDLE handle,uint32_t gain)
{
	uint32_t sensor_gain=0x00;
	uint32_t reg0216=0x00;
	uint32_t reg0217=0x00;
	
	SENSOR_PRINT_HIGH("zcf 12-15 c2390_write_gain:gain = %x\n",gain);
	if ( gain > SENSOR_MAX_GAIN)
			gain = SENSOR_MAX_GAIN;
	
	if ( gain < SENSOR_BASE_GAIN)
		{
			Sensor_WriteReg(0xe002,0x00);
			Sensor_WriteReg(0xe005,0x20);
			Sensor_WriteReg(0xe008,0x01);
			Sensor_WriteReg(0xe00b,0x00);
			Sensor_WriteReg(0x340f,0x10);
		}
	else if ( gain <= 0x100) // <16x
		{
			sensor_gain=(((gain-16)/16)<<4)+gain%16;
			Sensor_WriteReg(0xe002,sensor_gain);
			Sensor_WriteReg(0xe005,0x20);
			Sensor_WriteReg(0xe008,0x01);
			Sensor_WriteReg(0xe00b,0x00);
			Sensor_WriteReg(0x340f,0x10);
		}
	else if ( gain <= 0x200) // <32x
		{
			sensor_gain=(((gain/2-16)/16)<<4)+(gain/2)%16;
			Sensor_WriteReg(0xe002,sensor_gain);
			Sensor_WriteReg(0xe005,0x10); // night mode x2
			Sensor_WriteReg(0xe008,0x01);
			Sensor_WriteReg(0xe00b,0x00);
			Sensor_WriteReg(0x340f,0x10);
		}
	else  					// >= 32x
		{
			reg0216=(gain/SENSOR_BASE_GAIN)>>5;
			reg0217=(gain%(SENSOR_BASE_GAIN*32))/2;
			Sensor_WriteReg(0xe002,0xf0);
			Sensor_WriteReg(0xe005,0x10); // night mode x2
			Sensor_WriteReg(0xe008,reg0216);
			Sensor_WriteReg(0xe00b,reg0217);
			Sensor_WriteReg(0x340f,0x10);
		}
		
	SENSOR_PRINT_HIGH("c2390_write_gain:sensor_gain = %d\n",sensor_gain);


	//c2390_group_hold_off();

}



/*==============================================================================
 * Description:
 * read frame length from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static uint16_t c2390_read_frame_length(SENSOR_HW_HANDLE handle)
{
	uint16_t frame_len_h = 0;
	uint16_t frame_len_l = 0;

	frame_len_h = Sensor_ReadReg(0x0340) & 0xff;
	frame_len_l = Sensor_ReadReg(0x0341) & 0xff;

	return ((frame_len_h << 8) | frame_len_l);
}

/*==============================================================================
 * Description:
 * write frame length to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void c2390_write_frame_length(SENSOR_HW_HANDLE handle,uint32_t frame_len)
{
	Sensor_WriteReg(0x0340, (frame_len >> 8) & 0xff);
	Sensor_WriteReg(0x0341, frame_len & 0xff);
}

/*==============================================================================
 * Description:
 * read shutter from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t c2390_read_shutter(SENSOR_HW_HANDLE handle)
{
	uint16_t shutter_h = 0;
	uint16_t shutter_l = 0;

	shutter_h = Sensor_ReadReg(0x0202) & 0xff;
	shutter_l = Sensor_ReadReg(0x0203) & 0xff;

	return (shutter_h << 8) | shutter_l;
}

/*==============================================================================
 * Description:
 * write shutter to sensor registers
 * please pay attention to the frame length
 * please modify this function acording your spec
 *============================================================================*/
static void c2390_write_shutter(SENSOR_HW_HANDLE handle,uint32_t shutter)
{
	Sensor_WriteReg(0x0202, (shutter >> 8) & 0xff);
	Sensor_WriteReg(0x0203, shutter & 0xff);
	SENSOR_PRINT("(c2390_write_shutter %d", shutter);
}

/*==============================================================================
 * Description:
 * write exposure to sensor registers and get current shutter
 * please pay attention to the frame length
 * please don't change this function if it's necessary
 *============================================================================*/
static uint16_t c2390_update_exposure(SENSOR_HW_HANDLE handle,uint32_t shutter,uint32_t dummy_line)
{
	uint32_t dest_fr_len = 0;
	uint32_t cur_fr_len = 0;
	uint32_t fr_len = s_current_default_frame_length;

	//c2390_group_hold_on();

	if (1 == SUPPORT_AUTO_FRAME_LENGTH)
		goto write_sensor_shutter;

	dummy_line = dummy_line > FRAME_OFFSET ? dummy_line : FRAME_OFFSET;
	dest_fr_len = ((shutter + dummy_line) > fr_len) ? (shutter +dummy_line) : fr_len;

	cur_fr_len = c2390_read_frame_length(handle);

	if (shutter < SENSOR_MIN_SHUTTER)
		shutter = SENSOR_MIN_SHUTTER;

	if (dest_fr_len != cur_fr_len)
		c2390_write_frame_length(handle,dest_fr_len);
write_sensor_shutter:
	/* write shutter to sensor registers */
	c2390_write_shutter(handle,shutter);
	return shutter;
}

/*==============================================================================
 * Description:
 * sensor power on
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t c2390_power_on(SENSOR_HW_HANDLE handle,uint32_t power_on)
{
	SENSOR_AVDD_VAL_E dvdd_val = g_c2390_mipi_raw_info.dvdd_val;
	SENSOR_AVDD_VAL_E avdd_val = g_c2390_mipi_raw_info.avdd_val;
	SENSOR_AVDD_VAL_E iovdd_val = g_c2390_mipi_raw_info.iovdd_val;
	BOOLEAN power_down = g_c2390_mipi_raw_info.power_down_level;
	BOOLEAN reset_level = g_c2390_mipi_raw_info.reset_pulse_level;

	if (SENSOR_TRUE == power_on) {
		Sensor_PowerDown(!power_down);
		Sensor_SetResetLevel(reset_level);
		usleep(1 * 1000);
		Sensor_SetIovddVoltage(iovdd_val);
		usleep(1 * 1000);
		Sensor_SetAvddVoltage(avdd_val);
		Sensor_SetDvddVoltage(dvdd_val);
		//Sensor_SetIovddVoltage(iovdd_val);
		Sensor_SetMonitorVoltage(SENSOR_AVDD_3300MV);
		//usleep(10 * 1000);
		//Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);
		//Sensor_SetResetLevel(!reset_level);
		usleep(1 * 1000);
		//Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);
		usleep(1 * 1000);
		Sensor_PowerDown(power_down);
		Sensor_SetResetLevel(!reset_level);
		usleep(1 * 1000);
		Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);
		usleep(1 * 1000);
		#ifndef CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
		SENSOR_PRINT("dw9714_init:hehe\n");
		//Sensor_SetMonitorVoltage(SENSOR_AVDD_3300MV);
		//usleep(5 * 1000);
		dw9714_init(3);
		#endif

	} else {
			
		#ifndef CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
	//	dw9714_deinit(2);
		#endif
		#ifdef DYNA_BLC_ENABLE
			BLC_FLAG=0;
			#endif
		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		usleep(1 * 1000);
		Sensor_SetResetLevel(reset_level);
		Sensor_PowerDown(!power_down);
		usleep(1 * 1000);		
		Sensor_SetMonitorVoltage(SENSOR_AVDD_CLOSED);
		Sensor_SetAvddVoltage(SENSOR_AVDD_CLOSED);
		Sensor_SetDvddVoltage(SENSOR_AVDD_CLOSED);
		usleep(1 * 1000);
		Sensor_SetIovddVoltage(SENSOR_AVDD_CLOSED);
		//Sensor_SetResetLevel(reset_level);
		//Sensor_PowerDown(power_down);
	}
	SENSOR_PRINT("(1:on, 0:off): %d", power_on);
	return SENSOR_SUCCESS;
}

#ifdef FEATURE_OTP

/*==============================================================================
 * Description:
 * cfg otp setting
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t c2390_cfg_otp(SENSOR_HW_HANDLE handle,uint32_t param)
{
	UNUSED(param);
    cmr_uint rtn=SENSOR_SUCCESS;
    struct raw_param_info_tab* tab_ptr = (struct raw_param_info_tab*)s_c2390_raw_param_tab;
    uint32_t module_id = g_module_id;

    SENSOR_PRINT("SENSOR_OV5670:cfg_otp E");
    if(!param){
        if(PNULL!=tab_ptr[module_id].identify_otp){
            tab_ptr[module_id].identify_otp(0);
        }
    }
    return rtn;
	
		
}
#endif

/*==============================================================================
 * Description:
 * identify sensor id
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t c2390_identify(SENSOR_HW_HANDLE handle,uint32_t param)
{
	uint8_t pid_value = 0x05;
	uint8_t ver_value = 0x01;
	uint32_t ret_value = SENSOR_FAIL;

	SENSOR_PRINT("mipi raw identify");

	pid_value = Sensor_ReadReg(c2390_PID_ADDR);

	if (c2390_PID_VALUE == pid_value) {
		ver_value = Sensor_ReadReg(c2390_VER_ADDR);
		SENSOR_PRINT("Identify: PID = %x, VER = %x", pid_value, ver_value);
		if (c2390_VER_VALUE == ver_value) {
			#if 0 //defined(CONFIG_CAMERA_ISP_VERSION_V3) || defined(CONFIG_CAMERA_ISP_VERSION_V4)
			c2390_InitRawTuneInfo();
			#endif
			ret_value = SENSOR_SUCCESS;
			SENSOR_PRINT_HIGH("this is c2390 sensor");
		} else {
			SENSOR_PRINT_HIGH("Identify this is %x%x sensor", pid_value, ver_value);
		}
	} else {
		SENSOR_PRINT_HIGH("identify fail, pid_value = %x", pid_value);
	}

	return ret_value;
}

/*==============================================================================
 * Description:
 * get resolution trim
 *
 *============================================================================*/
static unsigned long c2390_get_resolution_trim_tab(uint32_t param)
{
	return (unsigned long) s_c2390_resolution_trim_tab;
}

/*==============================================================================
 * Description:
 * before snapshot
 * you can change this function if it's necessary
 *============================================================================*/
static uint32_t c2390_before_snapshot(SENSOR_HW_HANDLE handle,uint32_t param)
{
	uint32_t cap_shutter = 0;
	uint32_t prv_shutter = 0;
	uint32_t gain = 0;
	uint32_t cap_gain = 0;
	uint32_t capture_mode = param & 0xffff;
	uint32_t preview_mode = (param >> 0x10) & 0xffff;

	uint32_t prv_linetime = s_c2390_resolution_trim_tab[preview_mode].line_time;
	uint32_t cap_linetime = s_c2390_resolution_trim_tab[capture_mode].line_time;

	s_current_default_frame_length = c2390_get_default_frame_length(capture_mode);
	SENSOR_PRINT("capture_mode = %d", capture_mode);

	if (preview_mode == capture_mode) {
		cap_shutter = s_sensor_ev_info.preview_shutter;
		cap_gain = s_sensor_ev_info.preview_gain;
		goto snapshot_info;
	}

	prv_shutter = s_sensor_ev_info.preview_shutter;	//c2390_read_shutter();
	gain = s_sensor_ev_info.preview_gain;	//c2390_read_gain();

	Sensor_SetMode(capture_mode);
	Sensor_SetMode_WaitDone();

	cap_shutter = prv_shutter * prv_linetime / cap_linetime;// * BINNING_FACTOR;
/*
	while (gain >= (2 * SENSOR_BASE_GAIN)) {
		if (cap_shutter * 2 > s_current_default_frame_length)
			break;
		cap_shutter = cap_shutter * 2;
		gain = gain / 2;
	}
*/
	cap_shutter = c2390_update_exposure(handle,cap_shutter,0);
	cap_gain = gain;
	c2390_write_gain(handle,cap_gain);
	SENSOR_PRINT("preview_shutter = 0x%x, preview_gain = 0x%x",
		     s_sensor_ev_info.preview_shutter, s_sensor_ev_info.preview_gain);

	SENSOR_PRINT("capture_shutter = 0x%x, capture_gain = 0x%x", cap_shutter, cap_gain);
snapshot_info:
	s_hdr_info.capture_shutter = cap_shutter; //c2390_read_shutter();
	s_hdr_info.capture_gain = cap_gain; //c2390_read_gain();
	/* limit HDR capture min fps to 10;
	 * MaxFrameTime = 1000000*0.1us;
	 */ 
	s_hdr_info.capture_max_shutter = 1000000 / cap_linetime;

	Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_EXPOSURETIME, cap_shutter);

	return SENSOR_SUCCESS;
}

/*==============================================================================
 * Description:
 * get the shutter from isp
 * please don't change this function unless it's necessary
 *============================================================================*/
static uint32_t c2390_write_exposure(SENSOR_HW_HANDLE handle,uint32_t param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t exposure_line = 0x00;
	uint16_t dummy_line = 0x00;
	uint16_t mode = 0x00;

	exposure_line = param & 0xffff;
	dummy_line = (param >> 0x10) & 0xfff; /*for cits frame rate test*/
	mode = (param >> 0x1c) & 0x0f;

	SENSOR_PRINT("current mode = %d, exposure_line = %d, dummy_line=%d", mode, exposure_line,dummy_line);
	s_current_default_frame_length = c2390_get_default_frame_length(mode);

	s_sensor_ev_info.preview_shutter = c2390_update_exposure(handle,exposure_line,dummy_line);

	return ret_value;
}

static uint32_t c2390_write_exposure_ex(SENSOR_HW_HANDLE handle,unsigned long param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t exposure_line = 0x00;
	uint16_t dummy_line = 0x00;
	uint16_t mode = 0x00;
	struct sensor_ex_exposure *ex = (struct sensor_ex_exposure*)param;

	if (!param) {
		SENSOR_PRINT_ERR("param is NULL!!!");
		return ret_value;
	}
	exposure_line = ex->exposure;
	dummy_line = ex->dummy;
	mode = ex->size_index;

	SENSOR_PRINT("current mode = %d, exposure_line = %d, dummy_line=%d", mode, exposure_line,dummy_line);
	s_current_default_frame_length = c2390_get_default_frame_length(mode);

	s_sensor_ev_info.preview_shutter = c2390_update_exposure(handle,exposure_line,dummy_line);

	return ret_value;
}
/*==============================================================================
 * Description:
 * get the parameter from isp to real gain
 * you mustn't change the funcion !
 *============================================================================*/
static uint32_t isp_to_real_gain(uint32_t param)
{
	uint32_t real_gain = 0;

	
#if defined(CONFIG_CAMERA_ISP_VERSION_V3) || defined(CONFIG_CAMERA_ISP_VERSION_V4)
	real_gain=param;
#else
	real_gain = ((param & 0xf) + 16) * (((param >> 4) & 0x01) + 1);
	real_gain = real_gain * (((param >> 5) & 0x01) + 1) * (((param >> 6) & 0x01) + 1);
	real_gain = real_gain * (((param >> 7) & 0x01) + 1) * (((param >> 8) & 0x01) + 1);
	real_gain = real_gain * (((param >> 9) & 0x01) + 1) * (((param >> 10) & 0x01) + 1);
	real_gain = real_gain * (((param >> 11) & 0x01) + 1);
#endif

	return real_gain;
}

/*==============================================================================
 * Description:
 * write gain value to sensor
 * you can change this function if it's necessary
 *============================================================================*/
static uint32_t c2390_write_gain_value(SENSOR_HW_HANDLE handle,uint32_t param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint32_t real_gain = 0;

	real_gain = isp_to_real_gain(param);

	real_gain = real_gain * SENSOR_BASE_GAIN / ISP_BASE_GAIN;

	SENSOR_PRINT("real_gain = 0x%x", real_gain);

	s_sensor_ev_info.preview_gain = real_gain;
	c2390_write_gain(handle,real_gain);

	return ret_value;
}

#if 0//ndef CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
/*==============================================================================
 * Description:
 * write parameter to vcm
 * please add your VCM function to this function
 *============================================================================*/
static uint32_t c2390_write_af(SENSOR_HW_HANDLE handle,uint32_t param)
{
	return 0;//dw9714_write_af(param);
}
#endif

/*==============================================================================
 * Description:
 * increase gain or shutter for hdr
 *
 *============================================================================*/
static void c2390_increase_hdr_exposure(SENSOR_HW_HANDLE handle,uint8_t ev_multiplier)
{
	uint32_t shutter_multiply = s_hdr_info.capture_max_shutter / s_hdr_info.capture_shutter;
	uint32_t gain = 0;

	if (0 == shutter_multiply)
		shutter_multiply = 1;

	if (shutter_multiply >= ev_multiplier) {
		c2390_update_exposure(handle,s_hdr_info.capture_shutter * ev_multiplier,0);
		c2390_write_gain(handle,s_hdr_info.capture_gain);
	} else {
		gain = s_hdr_info.capture_gain * ev_multiplier / shutter_multiply;
		c2390_update_exposure(handle,s_hdr_info.capture_shutter * shutter_multiply,0);
		c2390_write_gain(handle,gain);
	}
}

/*==============================================================================
 * Description:
 * decrease gain or shutter for hdr
 *
 *============================================================================*/
static void c2390_decrease_hdr_exposure(SENSOR_HW_HANDLE handle,uint8_t ev_divisor)
{
	uint16_t gain_multiply = 0;
	uint32_t shutter = 0;
	gain_multiply = s_hdr_info.capture_gain / SENSOR_BASE_GAIN;

	if (gain_multiply >= ev_divisor) {
		c2390_update_exposure(handle,s_hdr_info.capture_shutter,0);
		c2390_write_gain(handle,s_hdr_info.capture_gain / ev_divisor);

	} else {
		shutter = s_hdr_info.capture_shutter * gain_multiply / ev_divisor;
		c2390_update_exposure(handle,shutter,0);
		c2390_write_gain(handle,s_hdr_info.capture_gain / gain_multiply);
	}
}

/*==============================================================================
 * Description:
 * set hdr ev
 * you can change this function if it's necessary
 *============================================================================*/
static uint32_t c2390_set_hdr_ev(SENSOR_HW_HANDLE handle,unsigned long param)
{
	uint32_t ret = SENSOR_SUCCESS;
	SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR) param;

	uint32_t ev = ext_ptr->param;
	uint8_t ev_divisor, ev_multiplier;

	switch (ev) {
	case SENSOR_HDR_EV_LEVE_0:
		ev_divisor = 2;
		c2390_decrease_hdr_exposure(handle,ev_divisor);
		break;
	case SENSOR_HDR_EV_LEVE_1:
		ev_multiplier = 2;
		c2390_increase_hdr_exposure(handle,ev_multiplier);
		break;
	case SENSOR_HDR_EV_LEVE_2:
		ev_multiplier = 1;
		c2390_increase_hdr_exposure(handle,ev_multiplier);
		break;
	default:
		break;
	}
	return ret;
}

/*==============================================================================
 * Description:
 * extra functoin
 * you can add functions reference SENSOR_EXT_FUNC_CMD_E which from sensor_drv_u.h
 *============================================================================*/
static uint32_t c2390_ext_func(SENSOR_HW_HANDLE handle,unsigned long param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR) param;

	SENSOR_PRINT("ext_ptr->cmd: %d", ext_ptr->cmd);
	switch (ext_ptr->cmd) {
	case SENSOR_EXT_EV:
		rtn = c2390_set_hdr_ev(handle,param);
		break;
	default:
		break;
	}

	return rtn;
}

/*==============================================================================
 * Description:
 * mipi stream on
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t c2390_stream_on(SENSOR_HW_HANDLE handle,uint32_t param)
{
	SENSOR_PRINT("E");
	usleep(60*1000); 
	SENSOR_PRINT("SENSOR_c2390: StreamOn");	 
	Sensor_WriteReg(0x0003,0x00);
	Sensor_WriteReg(0x0003,0x00);
	Sensor_WriteReg(0x0309,0x56);
	Sensor_WriteReg(0x0003,0x00);
	Sensor_WriteReg(0x0003,0x00);
	Sensor_WriteReg(0x0100,0x01);
	usleep(30*1000); 
	Sensor_WriteReg(0x0003,0x00);
	Sensor_WriteReg(0x0003,0x00);
	Sensor_WriteReg(0x0003,0x00);
	/*delay*/
	usleep(20 * 1000);

	return 0;
}

/*==============================================================================
 * Description:
 * mipi stream off
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t c2390_stream_off(SENSOR_HW_HANDLE handle,uint32_t param)
{
	SENSOR_PRINT("E");
	SENSOR_PRINT("SENSOR_c2390: StreamOff");
	
	/*delay*/
	
	Sensor_WriteReg(0x0309,0x2e); 
	Sensor_WriteReg(0x0003,0x00); 
	Sensor_WriteReg(0x0003,0x00); 
	Sensor_WriteReg(0x0003,0x00); 
	Sensor_WriteReg(0x0100,0x00); 
	Sensor_WriteReg(0x0003,0x00); 
	Sensor_WriteReg(0x0003,0x00); 
	usleep(100*1000); //100*1000 capture fail 

	return 0;
}


/*==============================================================================
 * Description:
 * cfg otp setting
 * please modify this function acording your spec
 *============================================================================*/
static unsigned long c2390_access_val(SENSOR_HW_HANDLE handle,unsigned long param)
{
	uint32_t ret = SENSOR_FAIL;
    SENSOR_VAL_T* param_ptr = (SENSOR_VAL_T*)param;
	
	if(!param_ptr){
		#ifdef FEATURE_OTP
		if(PNULL!=s_c2390_raw_param_tab_ptr->cfg_otp){
			ret = s_c2390_raw_param_tab_ptr->cfg_otp(s_c2390_otp_info_ptr);
			//checking OTP apply result
			if (SENSOR_SUCCESS != ret) {
				SENSOR_PRINT("apply otp failed");
			}
		} else {
			SENSOR_PRINT("no update otp function!");
	}
	#endif
		return ret;
	}
	
	SENSOR_PRINT("sensor c2390: param_ptr->type=%x", param_ptr->type);
	
	switch(param_ptr->type)
	{
		case SENSOR_VAL_TYPE_SHUTTER:
			*((uint32_t*)param_ptr->pval) = c2390_read_shutter(handle);
			break;
		case SENSOR_VAL_TYPE_READ_OTP_GAIN:
			*((uint32_t*)param_ptr->pval) = c2390_read_gain(handle);
			break;
		case SENSOR_VAL_TYPE_GET_STATIC_INFO:
			ret = c2390_get_static_info(handle, param_ptr->pval);
			break;
		case SENSOR_VAL_TYPE_GET_FPS_INFO:
			ret = c2390_get_fps_info(handle, param_ptr->pval);
			break;
		default:
			break;
	}
    ret = SENSOR_SUCCESS;

	return ret;
}

/*==============================================================================
 * Description:
 * all ioctl functoins
 * you can add functions reference SENSOR_IOCTL_FUNC_TAB_T from sensor_drv_u.h
 *
 * add ioctl functions like this:
 * .power = c2390_power_on,
 *============================================================================*/
static SENSOR_IOCTL_FUNC_TAB_T s_c2390_ioctl_func_tab = {
	.power = c2390_power_on,
	.identify = c2390_identify,
	.get_trim = c2390_get_resolution_trim_tab,
	.before_snapshort = c2390_before_snapshot,
	.ex_write_exp = c2390_write_exposure_ex,
	.write_gain_value = c2390_write_gain_value,
	#if 0//ndef CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
	.af_enable = c2390_write_af,
	#endif
	.set_focus = c2390_ext_func,
	//.set_video_mode = c2390_set_video_mode,
	.stream_on = c2390_stream_on,
	.stream_off = c2390_stream_off,
	#if 1//def FEATURE_OTP
	.cfg_otp=c2390_access_val,
	#endif	

	//.group_hold_on = c2390_group_hold_on,
	//.group_hold_of = c2390_group_hold_off,
};
