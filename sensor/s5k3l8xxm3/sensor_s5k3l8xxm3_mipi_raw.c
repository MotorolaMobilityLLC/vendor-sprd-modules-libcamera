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
 * V6.0
 */
 /*History
 *Date                  Modification                                 Reason
 *
 */

#include <utils/Log.h>
#include "sensor.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"

//#define CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
#ifndef CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
#include "../vcm/vcm_ak7371.h"
#endif

#include "sensor_s5k3l8xxm3_raw_param_v3.c"

#define CAMERA_IMAGE_180

#define SENSOR_NAME			"s5k3l8xxm3_mipi_raw"
#define I2C_SLAVE_ADDR			0x5A    /* 8bit slave address*/

#define s5k3l8xxm3_PID_ADDR			0x0000
#define s5k3l8xxm3_PID_VALUE		0x30c8
#define s5k3l8xxm3_VER_ADDR			0x0004
#define s5k3l8xxm3_VER_VALUE		0x10ff

/* sensor parameters begin */
/* effective sensor output image size */
#define SNAPSHOT_WIDTH			4208
#define SNAPSHOT_HEIGHT			3120
#define PREVIEW_WIDTH			2104
#define PREVIEW_HEIGHT			1560

/*Raw Trim parameters*/
#define SNAPSHOT_TRIM_X			0
#define SNAPSHOT_TRIM_Y			0
#define SNAPSHOT_TRIM_W			4208
#define SNAPSHOT_TRIM_H			3120
#define PREVIEW_TRIM_X			0
#define PREVIEW_TRIM_Y			0
#define PREVIEW_TRIM_W			2104
#define PREVIEW_TRIM_H			1560

/*Mipi output*/
#define LANE_NUM				4
#define RAW_BITS				10

#define SNAPSHOT_MIPI_PER_LANE_BPS	1124  /* 2*Mipi clk */
#define PREVIEW_MIPI_PER_LANE_BPS	1124  /* 2*Mipi clk */

/*line time unit: 0.001us*/
#define SNAPSHOT_LINE_TIME		10256
#define PREVIEW_LINE_TIME		10256

/* frame length*/
#define SNAPSHOT_FRAME_LENGTH		3250
#define PREVIEW_FRAME_LENGTH		3250

/* please ref your spec */
#define FRAME_OFFSET			5
#define SENSOR_MAX_GAIN			0x200
#define SENSOR_BASE_GAIN		0x20
#define SENSOR_MIN_SHUTTER		5

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

/*delay 1 frame to write sensor gain*/
//#define GAIN_DELAY_1_FRAME

/* sensor parameters end */

/* isp parameters, please don't change it*/
#define ISP_BASE_GAIN			0x80

/* please don't change it */
#define EX_MCLK				24

struct sensor_ev_info_t {
	uint16_t preview_shutter;
	uint16_t preview_gain;	
	uint16_t preview_framelength;
};

/*==============================================================================
 * Description:
 * global variable
 *============================================================================*/
static uint32_t s_current_default_frame_length=PREVIEW_FRAME_LENGTH;
static struct sensor_ev_info_t s_sensor_ev_info={
	PREVIEW_FRAME_LENGTH-FRAME_OFFSET,
	SENSOR_BASE_GAIN,
	PREVIEW_FRAME_LENGTH
	};

//#define FEATURE_OTP    /*OTP function switch*/

#ifdef FEATURE_OTP
#include "sensor_s5k3l8xxm3_darling_otp.c"
#else
#include "s5k3l8xxm3_darling_otp.h"
#endif

static SENSOR_IOCTL_FUNC_TAB_T s_s5k3l8xxm3_ioctl_func_tab;
static struct sensor_raw_info *s_s5k3l8xxm3_mipi_raw_info_ptr = &s_s5k3l8xxm3_mipi_raw_info;

static const SENSOR_REG_T s5k3l8xxm3_init_setting[] = {
	//{0x6028,0x4000},
	//{0x6214,0xFFFF},
	//{0x6216,0xFFFF},
	//{0x6218,0x0000},
	//{0x621A,0x0000},
	{0x6028,0x2000},
	{0x602A,0x2450},
	{0x6F12,0x0448},
	{0x6F12,0x0349},
	{0x6F12,0x0160},
	{0x6F12,0xC26A},
	{0x6F12,0x511A},
	{0x6F12,0x8180},
	{0x6F12,0x00F0},
	{0x6F12,0x48B8},
	{0x6F12,0x2000},
	{0x6F12,0x2588},
	{0x6F12,0x2000},
	{0x6F12,0x16C0},
	{0x6F12,0x0000},
	{0x6F12,0x0000},
	{0x6F12,0x0000},
	{0x6F12,0x0000},
	{0x6F12,0x10B5},
	{0x6F12,0x00F0},
	{0x6F12,0x5DF8},
	{0x6F12,0x2748},
	{0x6F12,0x4078},
	{0x6F12,0x0028},
	{0x6F12,0x0AD0},
	{0x6F12,0x00F0},
	{0x6F12,0x5CF8},
	{0x6F12,0x2549},
	{0x6F12,0xB1F8},
	{0x6F12,0x1403},
	{0x6F12,0x4200},
	{0x6F12,0x2448},
	{0x6F12,0x4282},
	{0x6F12,0x91F8},
	{0x6F12,0x9610},
	{0x6F12,0x4187},
	{0x6F12,0x10BD},
	{0x6F12,0x70B5},
	{0x6F12,0x0446},
	{0x6F12,0x2148},
	{0x6F12,0x0022},
	{0x6F12,0x4068},
	{0x6F12,0x86B2},
	{0x6F12,0x050C},
	{0x6F12,0x3146},
	{0x6F12,0x2846},
	{0x6F12,0x00F0},
	{0x6F12,0x4CF8},
	{0x6F12,0x2046},
	{0x6F12,0x00F0},
	{0x6F12,0x4EF8},
	{0x6F12,0x14F8},
	{0x6F12,0x680F},
	{0x6F12,0x6178},
	{0x6F12,0x40EA},
	{0x6F12,0x4100},
	{0x6F12,0x1749},
	{0x6F12,0xC886},
	{0x6F12,0x1848},
	{0x6F12,0x2278},
	{0x6F12,0x007C},
	{0x6F12,0x4240},
	{0x6F12,0x1348},
	{0x6F12,0xA230},
	{0x6F12,0x8378},
	{0x6F12,0x43EA},
	{0x6F12,0xC202},
	{0x6F12,0x0378},
	{0x6F12,0x4078},
	{0x6F12,0x9B00},
	{0x6F12,0x43EA},
	{0x6F12,0x4000},
	{0x6F12,0x0243},
	{0x6F12,0xD0B2},
	{0x6F12,0x0882},
	{0x6F12,0x3146},
	{0x6F12,0x2846},
	{0x6F12,0xBDE8},
	{0x6F12,0x7040},
	{0x6F12,0x0122},
	{0x6F12,0x00F0},
	{0x6F12,0x2AB8},
	{0x6F12,0x10B5},
	{0x6F12,0x0022},
	{0x6F12,0xAFF2},
	{0x6F12,0x8701},
	{0x6F12,0x0B48},
	{0x6F12,0x00F0},
	{0x6F12,0x2DF8},
	{0x6F12,0x084C},
	{0x6F12,0x0022},
	{0x6F12,0xAFF2},
	{0x6F12,0x6D01},
	{0x6F12,0x2060},
	{0x6F12,0x0848},
	{0x6F12,0x00F0},
	{0x6F12,0x25F8},
	{0x6F12,0x6060},
	{0x6F12,0x10BD},
	{0x6F12,0x0000},
	{0x6F12,0x2000},
	{0x6F12,0x0550},
	{0x6F12,0x2000},
	{0x6F12,0x0C60},
	{0x6F12,0x4000},
	{0x6F12,0xD000},
	{0x6F12,0x2000},
	{0x6F12,0x2580},
	{0x6F12,0x2000},
	{0x6F12,0x16F0},
	{0x6F12,0x0000},
	{0x6F12,0x2221},
	{0x6F12,0x0000},
	{0x6F12,0x2249},
	{0x6F12,0x42F2},
	{0x6F12,0x351C},
	{0x6F12,0xC0F2},
	{0x6F12,0x000C},
	{0x6F12,0x6047},
	{0x6F12,0x42F2},
	{0x6F12,0xE11C},
	{0x6F12,0xC0F2},
	{0x6F12,0x000C},
	{0x6F12,0x6047},
	{0x6F12,0x40F2},
	{0x6F12,0x077C},
	{0x6F12,0xC0F2},
	{0x6F12,0x000C},
	{0x6F12,0x6047},
	{0x6F12,0x42F2},
	{0x6F12,0x492C},
	{0x6F12,0xC0F2},
	{0x6F12,0x000C},
	{0x6F12,0x6047},
	{0x6F12,0x4BF2},
	{0x6F12,0x453C},
	{0x6F12,0xC0F2},
	{0x6F12,0x000C},
	{0x6F12,0x6047},
	{0x6F12,0x0000},
	{0x6F12,0x0000},
	{0x6F12,0x0000},
	{0x6F12,0x0000},
	{0x6F12,0x0000},
	{0x6F12,0x0000},
	{0x6F12,0x0000},
	{0x6F12,0x30C8},
	{0x6F12,0x0157},
	{0x6F12,0x0000},
	{0x6F12,0x0003},
	{0x3052,0x0000},//
	{0x3098,0x0364},//
};
static const SENSOR_REG_T s5k3l8xxm3_720p_setting[] = {
	//720p setting @120fps, 24Mclk,p566.4M,1124M/lane,4lanes
	{0x6028,0x2000},
	{0x602A,0x0F74},
	{0x6F12,0x0040},
	{0x6F12,0x0040},
	{0x6028,0x4000},
	{0x0344,0x00C0},
	{0x0346,0x01E8},
	{0x0348,0x0FBF},
	{0x034A,0x0A57},
	{0x034C,0x0500},
	{0x034E,0x02D0},
	{0x0900,0x0113},
	{0x0380,0x0001},
	{0x0382,0x0001},
	{0x0384,0x0001},
	{0x0386,0x0005},
	{0x0400,0x0001},
	{0x0404,0x0030},
	{0x0114,0x0300},
	{0x0110,0x0002},
	{0x0136,0x1800},
	{0x0304,0x0006},
	{0x0306,0x00B1},
	{0x0302,0x0001},
	{0x0300,0x0005},
	{0x030C,0x0006},
	{0x030E,0x0119},
	{0x030A,0x0001},
	{0x0308,0x0008},
	{0x0342,0x16B0},
	{0x0340,0x032C},
	{0x0202,0x0200},
	{0x0200,0x00C6},
	{0x0B04,0x0101},
	{0x0B08,0x0000},
	{0x0B00,0x0007},
	{0x316A,0x00A0},

};

static const SENSOR_REG_T s5k3l8xxm3_preview_setting[] = {
	//$MIPI[Width:2104,Height:1560,Format:Raw10,Lane:4,ErrorCheck:0,PolarityData:0,PolarityClock:0,Buffer:4,DataRate:1124,useEmbData:0]
	////$MV1[MCLK:24,Width:2104,Height:1560,Format:MIPI_Raw10,mipi_lane:4,mipi_datarate:1124,pvi_pclk_inverse:0]
	/*
	ExtClk :		24	MHz
	Vt_pix_clk :		566.4	MHz
	MIPI_output_speed :	1124	Mbps/lane
	Crop_Width :		4208	px
	Crop_Height :		3120	px
	Output_Width :		2104	px
	Output_Height : 	1560	px
	Frame rate :		30.01	fps
	Output format : 	Raw10
	*Pedestal :		64
	*Mapped BPC :		On
	*Dynamic BPC :		Off
	*Internal LSC : 	Off
	H-size :		5808	px
	H-blank :		3704	px
	V-size :		3250	line
	V-blank :		1690	line
	Lane :			4	lane
	First Pixel :		Gr	First	  */
	{0x6028,0x2000},
	{0x602A,0x0F74},
	{0x6F12,0x0040},
	{0x6F12,0x0040},
	{0x6028,0x4000},
	{0x0344,0x0008},
	{0x0346,0x0008},
	{0x0348,0x1077},
	{0x034A,0x0C37},
	{0x034C,0x0838},
	{0x034E,0x0618},
	{0x0900,0x0112},
	{0x0380,0x0001},
	{0x0382,0x0001},
	{0x0384,0x0001},
	{0x0386,0x0003},
	{0x0400,0x0001},
	{0x0404,0x0020},
	{0x0114,0x0300},
	{0x0110,0x0002},
	{0x0136,0x1800},
	{0x0304,0x0006},
	{0x0306,0x00B1},
	{0x0302,0x0001},
	{0x0300,0x0005},
	{0x030C,0x0006},
	{0x030E,0x0119},
	{0x030A,0x0001},
	{0x0308,0x0008},
	{0x0342,0x16B0},
	{0x0340,0x0CB2},
	{0x0202,0x0200},
	{0x0200,0x00C6},
	{0x0B04,0x0101},
	{0x0B08,0x0000},
	{0x0B00,0x0007},
	{0x316A,0x00A0},
};

static const SENSOR_REG_T s5k3l8xxm3_snapshot_setting[] = {
	// 3L8 ID : 03C8 read address 0000
	// 4208x3120_Mclk=24Mhz_4lane_30fps
	// Vt=566.4Mhz
	// MIPI speed 1124Mbps
	// line time :102
	// frame length :3250
	// Bayer pattern : GRBG

	{0x6028,0x2000},
	{0x602A,0x0F74},
	{0x6F12,0x0040},
	{0x6F12,0x0040},
	{0x6028,0x4000},
	/*{0x0344,0x0020},//4160 x3120
	{0x0346,0x0008},
	{0x0348,0x105F},
	{0x034A,0x0C37},
	{0x034C,0x1040},
	{0x034E,0x0C30},*/
	{0x0344,0x0008},//4208 x3120
	{0x0346,0x0008},
	{0x0348,0x1077},
	{0x034A,0x0C37},
	{0x034C,0x1070},
	{0x034E,0x0C30},
	{0x0900,0x0011},
	{0x0380,0x0001},
	{0x0382,0x0001},
	{0x0384,0x0001},
	{0x0386,0x0001},
	{0x0400,0x0000},
	{0x0404,0x0010},
	{0x0114,0x0300},
	{0x0110,0x0002},
	{0x0136,0x1800},
	{0x0304,0x0006},
	{0x0306,0x00B1},
	{0x0302,0x0001},
	{0x0300,0x0005},
	{0x030C,0x0006},
	{0x030E,0x0119},
	{0x030A,0x0001},
	{0x0308,0x0008},
	{0x0342,0x16B0},
	{0x0340,0x0CB2},//
	{0x0202,0x0200},
	{0x0200,0x00C6},
	{0x0B04,0x0101},
	{0x0B08,0x0000},//
	{0x0B00,0x0007},
	{0x316A,0x00A0},
};

static SENSOR_REG_TAB_INFO_T s_s5k3l8xxm3_resolution_tab_raw[SENSOR_MODE_MAX] = {
	{ADDR_AND_LEN_OF_ARRAY(s5k3l8xxm3_init_setting), 0, 0, EX_MCLK,
	 SENSOR_IMAGE_FORMAT_RAW},
	/*{ADDR_AND_LEN_OF_ARRAY(s5k3l8xxm3_preview_setting),
	 PREVIEW_WIDTH, PREVIEW_HEIGHT, EX_MCLK,
	 SENSOR_IMAGE_FORMAT_RAW},*/
	{ADDR_AND_LEN_OF_ARRAY(s5k3l8xxm3_snapshot_setting),
	 SNAPSHOT_WIDTH, SNAPSHOT_HEIGHT, EX_MCLK,
	 SENSOR_IMAGE_FORMAT_RAW},

};

static SENSOR_TRIM_T s_s5k3l8xxm3_resolution_trim_tab[SENSOR_MODE_MAX] = {
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	/*{PREVIEW_TRIM_X, PREVIEW_TRIM_Y, PREVIEW_TRIM_W, PREVIEW_TRIM_H,
	 PREVIEW_LINE_TIME, PREVIEW_MIPI_PER_LANE_BPS, PREVIEW_FRAME_LENGTH,
	 {0, 0, PREVIEW_TRIM_W, PREVIEW_TRIM_H}},*/
	{SNAPSHOT_TRIM_X, SNAPSHOT_TRIM_Y, SNAPSHOT_TRIM_W, SNAPSHOT_TRIM_H,
	 SNAPSHOT_LINE_TIME, SNAPSHOT_MIPI_PER_LANE_BPS, SNAPSHOT_FRAME_LENGTH,
	 {0, 0, SNAPSHOT_TRIM_W, SNAPSHOT_TRIM_H}},
};

/*==============================================================================
 * Description:
 * sensor all info
 * please modify this variable acording your spec
 *============================================================================*/
SENSOR_INFO_T g_s5k3l8xxm3_mipi_raw_info = {
	/* salve i2c write address */
	(I2C_SLAVE_ADDR >> 1),
	/* salve i2c read address */
	(I2C_SLAVE_ADDR >> 1),
	/*bit0: 0: i2c register value is 8 bit, 1: i2c register value is 16 bit */
	SENSOR_I2C_REG_16BIT | SENSOR_I2C_VAL_16BIT | SENSOR_I2C_FREQ_400,
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
	{{s5k3l8xxm3_PID_ADDR, s5k3l8xxm3_PID_VALUE}
	 ,
	 {s5k3l8xxm3_VER_ADDR, s5k3l8xxm3_VER_VALUE}
	 }
	,
	/* voltage of avdd */
	SENSOR_AVDD_2800MV,
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
	SENSOR_IMAGE_PATTERN_RAWRGB_GR,
	/* point to resolution table information structure */
	s_s5k3l8xxm3_resolution_tab_raw,
	/* point to ioctl function table */
	&s_s5k3l8xxm3_ioctl_func_tab,
	/* information and table about Rawrgb sensor */
	&s_s5k3l8xxm3_mipi_raw_info_ptr,
	/* extend information about sensor
	 * like &g_s5k3l8xxm3_ext_info
	 */
	NULL,
	/* voltage of iovdd */
	SENSOR_AVDD_1800MV,
	/* voltage of dvdd */
	SENSOR_AVDD_1200MV,
	/* skip frame num before preview */
	1,
	/* skip frame num before capture */
	1,
	/* deci frame num during preview */
	0,
	/* deci frame num during video preview */
	0,
	0,
	0,
	0,
	0,
	0,
	{SENSOR_INTERFACE_TYPE_CSI2, LANE_NUM, RAW_BITS, 0}
	,
	0,
	/* skip frame num while change setting */
	1,
	/* horizontal  view angle*/
	65,
	/* vertical view angle*/
	60,
	"s5k3l8xxm3_v1"
};

static SENSOR_STATIC_INFO_T s_s5k3l8xxm3_static_info = {
	220,	//f-number,focal ratio
	346,	//focal_length;
	0,	//max_fps,max fps of sensor's all settings,it will be calculated from sensor mode fps
	16*256,	//max_adgain,AD-gain
	0,	//ois_supported;
	0,	//pdaf_supported;
	1,	//exp_valid_frame_num;N+2-1
	64,	//clamp_level,black level
	1,	//adgain_valid_frame_num;N+1-1
    };   


static SENSOR_MODE_FPS_INFO_T s_s5k3l8xxm3_mode_fps_info = {
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
 * get default frame length
 *
 *============================================================================*/
static uint32_t s5k3l8xxm3_get_default_frame_length(SENSOR_HW_HANDLE handle,uint32_t mode)
{
	return s_s5k3l8xxm3_resolution_trim_tab[mode].frame_line;
}

/*==============================================================================
 * Description:
 * write group-hold on to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void s5k3l8xxm3_group_hold_on(SENSOR_HW_HANDLE handle)
{
	SENSOR_PRINT("E");

	Sensor_WriteReg(0x0104, 0x0100);
}

/*==============================================================================
 * Description:
 * write group-hold off to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void s5k3l8xxm3_group_hold_off(SENSOR_HW_HANDLE handle)
{
	SENSOR_PRINT("E");

	Sensor_WriteReg(0x0104, 0x0000);
}


/*==============================================================================
 * Description:
 * read gain from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static uint16_t s5k3l8xxm3_read_gain(SENSOR_HW_HANDLE handle)
{
	return s_sensor_ev_info.preview_gain;
}

/*==============================================================================
 * Description:
 * write gain to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void s5k3l8xxm3_write_gain(SENSOR_HW_HANDLE handle,float gain)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t value=0x00;
	float real_gain = gain;
	float a_gain = 0;
	float d_gain = 0;

	/*if (SENSOR_MAX_GAIN < gain)
			gain = SENSOR_MAX_GAIN;
		
	Sensor_WriteReg(0x0204, gain);
	SENSOR_LOGI("_s5k3l8xxm3: real_gain:0x%x, param: 0x%x", (uint32_t)real_gain, param);
*/
	if ((uint32_t)real_gain <= 16*32) {
		a_gain = real_gain;
		d_gain = 256;
	//ret_value = Sensor_WriteReg(0x204,(uint32_t)a_gain);//0x100);//a_gain);
	} else {
		a_gain = 16*32;
		d_gain = 256.0*real_gain/a_gain;
		SENSOR_LOGI("_s5k3l8xxm3: real_gain:0x%x, a_gain: 0x%x, d_gain: 0x%x", (uint32_t)real_gain, (uint32_t)a_gain,(uint32_t)d_gain);
		if((uint32_t)d_gain>256*256)
			d_gain=256*256;  //d_gain < 256x
	}

	ret_value = Sensor_WriteReg(0x204,(uint32_t)a_gain);//0x100);//a_gain); 
	ret_value = Sensor_WriteReg(0x20e,(uint32_t) d_gain);
	ret_value = Sensor_WriteReg(0x210, (uint32_t)d_gain);
	ret_value = Sensor_WriteReg(0x212, (uint32_t)d_gain);
	ret_value = Sensor_WriteReg(0x214, (uint32_t)d_gain);

	//s5k3l8xxm3_group_hold_off(handle);

}

/*==============================================================================
 * Description:
 * read frame length from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static uint16_t s5k3l8xxm3_read_frame_length(SENSOR_HW_HANDLE handle)
{
	return s_sensor_ev_info.preview_framelength;
}

/*==============================================================================
 * Description:
 * write frame length to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void s5k3l8xxm3_write_frame_length(SENSOR_HW_HANDLE handle,uint32_t frame_len)
{
	Sensor_WriteReg(0x0340, frame_len );
}

/*==============================================================================
 * Description:
 * read shutter from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t s5k3l8xxm3_read_shutter(SENSOR_HW_HANDLE handle)
{
	return s_sensor_ev_info.preview_shutter;
}

/*==============================================================================
 * Description:
 * write shutter to sensor registers
 * please pay attention to the frame length
 * please modify this function acording your spec
 *============================================================================*/
static void s5k3l8xxm3_write_shutter(SENSOR_HW_HANDLE handle,uint32_t shutter)
{
	Sensor_WriteReg(0x0202, shutter);
}

/*==============================================================================
 * Description:
 * write exposure to sensor registers and get current shutter
 * please pay attention to the frame length
 * please don't change this function if it's necessary
 *============================================================================*/
static uint16_t s5k3l8xxm3_write_exposure_dummy(SENSOR_HW_HANDLE handle,uint32_t shutter,uint32_t dummy_line,uint16_t size_index)
{
	uint32_t dest_fr_len = 0;
	uint32_t cur_fr_len = 0;
	uint32_t fr_len = s_current_default_frame_length;

	//s5k3l8xxm3_group_hold_on(handle);

	if (1 == SUPPORT_AUTO_FRAME_LENGTH)
		goto write_sensor_shutter;

	dest_fr_len = ((shutter + dummy_line+FRAME_OFFSET) > fr_len) ? (shutter +dummy_line+ FRAME_OFFSET) : fr_len;

	cur_fr_len = s5k3l8xxm3_read_frame_length(handle);

	if (shutter < SENSOR_MIN_SHUTTER)
		shutter = SENSOR_MIN_SHUTTER;

	if (dest_fr_len != cur_fr_len)
		s5k3l8xxm3_write_frame_length(handle,dest_fr_len);
write_sensor_shutter:
	/* write shutter to sensor registers */
	s_sensor_ev_info.preview_shutter=shutter;
	s5k3l8xxm3_write_shutter(handle,shutter);

	#ifdef GAIN_DELAY_1_FRAME
	usleep(dest_fr_len*PREVIEW_LINE_TIME/10);
	#endif
	
	return SENSOR_SUCCESS;
}

/*==============================================================================
 * Description:
 * sensor power on
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t s5k3l8xxm3_power_on(SENSOR_HW_HANDLE handle,uint32_t power_on)
{
	SENSOR_AVDD_VAL_E dvdd_val = g_s5k3l8xxm3_mipi_raw_info.dvdd_val;
	SENSOR_AVDD_VAL_E avdd_val = g_s5k3l8xxm3_mipi_raw_info.avdd_val;
	SENSOR_AVDD_VAL_E iovdd_val = g_s5k3l8xxm3_mipi_raw_info.iovdd_val;
	BOOLEAN power_down = g_s5k3l8xxm3_mipi_raw_info.power_down_level;
	BOOLEAN reset_level = g_s5k3l8xxm3_mipi_raw_info.reset_pulse_level;

	if (SENSOR_TRUE == power_on) {
		Sensor_PowerDown(power_down);
		Sensor_SetResetLevel(reset_level);
		usleep(10 * 1000);
		Sensor_SetAvddVoltage(avdd_val);
		Sensor_SetDvddVoltage(dvdd_val);
		Sensor_SetIovddVoltage(iovdd_val);
		Sensor_PowerDown(!power_down);
		Sensor_SetResetLevel(!reset_level);
		usleep(10 * 1000);
		Sensor_SetMCLK(EX_MCLK);
		Sensor_SetMIPILevel(0);

		#ifndef CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
		Sensor_SetMonitorVoltage(SENSOR_AVDD_2800MV);
		usleep(5 * 1000);
		//zzz_init(2);
		#else
		Sensor_SetMonitorVoltage(SENSOR_AVDD_CLOSED);
		#endif

	} else {

		#ifndef CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
		//zzz_deinit(2);
		Sensor_SetMonitorVoltage(SENSOR_AVDD_CLOSED);
		#endif
		Sensor_SetMIPILevel(1);

		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		Sensor_SetResetLevel(reset_level);
		Sensor_PowerDown(power_down);		
		Sensor_SetDvddVoltage(SENSOR_AVDD_CLOSED);
		Sensor_SetAvddVoltage(SENSOR_AVDD_CLOSED);
		Sensor_SetIovddVoltage(SENSOR_AVDD_CLOSED);
	}
	SENSOR_PRINT("(1:on, 0:off): %d", power_on);
	return SENSOR_SUCCESS;
}

/*==============================================================================
 * Description:
 * calculate fps for every sensor mode according to frame_line and line_time
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t s5k3l8xxm3_init_mode_fps_info(SENSOR_HW_HANDLE handle)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_PRINT("s5k3l8xxm3_init_mode_fps_info:E");
	if(!s_s5k3l8xxm3_mode_fps_info.is_init) {
		uint32_t i,modn,tempfps = 0;
		SENSOR_PRINT("s5k3l8xxm3_init_mode_fps_info:start init");
		for(i = 0;i < NUMBER_OF_ARRAY(s_s5k3l8xxm3_resolution_trim_tab); i++) {
			//max fps should be multiple of 30,it calulated from line_time and frame_line
			tempfps = s_s5k3l8xxm3_resolution_trim_tab[i].line_time*s_s5k3l8xxm3_resolution_trim_tab[i].frame_line;
				if(0 != tempfps) {
					tempfps = 1000000000/tempfps;
				modn = tempfps / 30;
				if(tempfps > modn*30)
					modn++;
				s_s5k3l8xxm3_mode_fps_info.sensor_mode_fps[i].max_fps = modn*30;
				if(s_s5k3l8xxm3_mode_fps_info.sensor_mode_fps[i].max_fps > 30) {
					s_s5k3l8xxm3_mode_fps_info.sensor_mode_fps[i].is_high_fps = 1;
					s_s5k3l8xxm3_mode_fps_info.sensor_mode_fps[i].high_fps_skip_num =
						s_s5k3l8xxm3_mode_fps_info.sensor_mode_fps[i].max_fps/30;
				}
				if(s_s5k3l8xxm3_mode_fps_info.sensor_mode_fps[i].max_fps >
						s_s5k3l8xxm3_static_info.max_fps) {
					s_s5k3l8xxm3_static_info.max_fps =
						s_s5k3l8xxm3_mode_fps_info.sensor_mode_fps[i].max_fps;
				}
			}
			SENSOR_PRINT("mode %d,tempfps %d,frame_len %d,line_time: %d ",i,tempfps,
					s_s5k3l8xxm3_resolution_trim_tab[i].frame_line,
					s_s5k3l8xxm3_resolution_trim_tab[i].line_time);
			SENSOR_PRINT("mode %d,max_fps: %d ",
					i,s_s5k3l8xxm3_mode_fps_info.sensor_mode_fps[i].max_fps);
			SENSOR_PRINT("is_high_fps: %d,highfps_skip_num %d",
					s_s5k3l8xxm3_mode_fps_info.sensor_mode_fps[i].is_high_fps,
					s_s5k3l8xxm3_mode_fps_info.sensor_mode_fps[i].high_fps_skip_num);
		}
		s_s5k3l8xxm3_mode_fps_info.is_init = 1;
	}
	SENSOR_PRINT("s5k3l8xxm3_init_mode_fps_info:X");
	return rtn;
}


static const struct pd_pos_info s5k3l8xxm3_pd_pos_r[] = {
	{ 28, 35 },
	{ 44, 39 },
	{ 64, 39 },
	{ 80, 35 },
	{ 32, 47 },
	{ 48, 51 },
	{ 60, 51 },
	{ 76, 47 },
	{ 32, 71 },
	{ 48, 67 },
	{ 60, 67 },
	{ 76, 71 },
	{ 28, 83 },
	{ 44, 79 },
	{ 64, 79 },
	{ 80, 83 },
};

static const struct pd_pos_info s5k3l8xxm3_pd_pos_l[] = {
	{ 28, 31 },
	{ 44, 35 },
	{ 64, 35 },
	{ 80, 31 },
	{ 32, 51 },
	{ 48, 55 },
	{ 60, 55 },
	{ 76, 51 },
	{ 32, 67 },
	{ 48, 63 },
	{ 60, 63 },
	{ 76, 67 },
	{ 28, 87 },
	{ 44, 83 },
	{ 64, 83 },
	{ 80, 87 },
};

static uint32_t s5k3l8xxm3_get_pdaf_info(SENSOR_HW_HANDLE handle, uint32_t *param)
{
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
	pd_pos_r_size = NUMBER_OF_ARRAY(s5k3l8xxm3_pd_pos_r);
	pd_pos_l_size = NUMBER_OF_ARRAY(s5k3l8xxm3_pd_pos_l);
	if (pd_pos_r_size != pd_pos_l_size) {
		SENSOR_PRINT_ERR("s5k3l8xxm3_pd_pos_r size not match s5k3l8xxm3_pd_pos_l");
		return -1;
	}

	pdaf_info->pd_offset_x = 24;
	pdaf_info->pd_offset_y = 24;
	pdaf_info->pd_pitch_x = 64;
	pdaf_info->pd_pitch_y = 64;
	pdaf_info->pd_density_x = 16;
	pdaf_info->pd_density_y = 16;
	pdaf_info->pd_block_num_x = 65;
	pdaf_info->pd_block_num_y = 48;
	pdaf_info->pd_pos_size = pd_pos_r_size;
	pdaf_info->pd_pos_r = (struct pd_pos_info *)s5k3l8xxm3_pd_pos_r;
	pdaf_info->pd_pos_l = (struct pd_pos_info *)s5k3l8xxm3_pd_pos_l;

	return rtn;
}

static uint32_t s5k3l8xxm3_get_static_info(SENSOR_HW_HANDLE handle, uint32_t *param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	struct sensor_ex_info *ex_info;
	uint32_t up = 0;
	uint32_t down = 0;
	//make sure we have get max fps of all settings.
	if(!s_s5k3l8xxm3_mode_fps_info.is_init) {
		s5k3l8xxm3_init_mode_fps_info(handle);
	}
	ex_info = (struct sensor_ex_info*)param;
	ex_info->f_num = s_s5k3l8xxm3_static_info.f_num;
	ex_info->focal_length = s_s5k3l8xxm3_static_info.focal_length;
	ex_info->max_fps = s_s5k3l8xxm3_static_info.max_fps;
	ex_info->max_adgain = s_s5k3l8xxm3_static_info.max_adgain;
	ex_info->ois_supported = s_s5k3l8xxm3_static_info.ois_supported;
	ex_info->pdaf_supported = s_s5k3l8xxm3_static_info.pdaf_supported;
	ex_info->exp_valid_frame_num = s_s5k3l8xxm3_static_info.exp_valid_frame_num;
	ex_info->clamp_level = s_s5k3l8xxm3_static_info.clamp_level;
	ex_info->adgain_valid_frame_num = s_s5k3l8xxm3_static_info.adgain_valid_frame_num;
	ex_info->preview_skip_num = g_s5k3l8xxm3_mipi_raw_info.preview_skip_num;
	ex_info->capture_skip_num = g_s5k3l8xxm3_mipi_raw_info.capture_skip_num;
	ex_info->name = g_s5k3l8xxm3_mipi_raw_info.name;
	ex_info->sensor_version_info = g_s5k3l8xxm3_mipi_raw_info.sensor_version_info;
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


static uint32_t s5k3l8xxm3_get_fps_info(SENSOR_HW_HANDLE handle, uint32_t *param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_MODE_FPS_T *fps_info;
	//make sure have inited fps of every sensor mode.
	if(!s_s5k3l8xxm3_mode_fps_info.is_init) {
		s5k3l8xxm3_init_mode_fps_info(handle);
	}
	fps_info = (SENSOR_MODE_FPS_T*)param;
	uint32_t sensor_mode = fps_info->mode;
	fps_info->max_fps = s_s5k3l8xxm3_mode_fps_info.sensor_mode_fps[sensor_mode].max_fps;
	fps_info->min_fps = s_s5k3l8xxm3_mode_fps_info.sensor_mode_fps[sensor_mode].min_fps;
	fps_info->is_high_fps = s_s5k3l8xxm3_mode_fps_info.sensor_mode_fps[sensor_mode].is_high_fps;
	fps_info->high_fps_skip_num = s_s5k3l8xxm3_mode_fps_info.sensor_mode_fps[sensor_mode].high_fps_skip_num;
	SENSOR_PRINT("mode %d, max_fps: %d",fps_info->mode, fps_info->max_fps);
	SENSOR_PRINT("min_fps: %d", fps_info->min_fps);
	SENSOR_PRINT("is_high_fps: %d", fps_info->is_high_fps);
	SENSOR_PRINT("high_fps_skip_num: %d", fps_info->high_fps_skip_num);

	return rtn;
}

/*==============================================================================
 * Description:
 * cfg otp setting
 * please modify this function acording your spec
 *============================================================================*/
static unsigned long s5k3l8xxm3_access_val(SENSOR_HW_HANDLE handle,unsigned long param)
{
	uint32_t ret = SENSOR_FAIL;
    SENSOR_VAL_T* param_ptr = (SENSOR_VAL_T*)param;
	
	if(!param_ptr){
		#ifdef FEATURE_OTP
		if(PNULL!=s_s5k3l8xxm3_raw_param_tab_ptr->cfg_otp){
			ret = s_s5k3l8xxm3_raw_param_tab_ptr->cfg_otp(handle,s_s5k3l8xxm3_otp_info_ptr);
			//checking OTP apply result
			if (SENSOR_SUCCESS != ret) {
				SENSOR_PRINT("apply otp failed");
			}
		} 
		else {
			SENSOR_PRINT("no update otp function!");
		}
		#endif
		return ret;
	}
	
	SENSOR_PRINT("sensor s5k3l8xxm3: param_ptr->type=%x", param_ptr->type);
	
	switch(param_ptr->type)
	{
		case SENSOR_VAL_TYPE_INIT_OTP:
			ret =s5k3l8xxm3_otp_init(handle);
			break;
		case SENSOR_VAL_TYPE_READ_OTP:
			ret =s5k3l8xxm3_otp_read(handle,param_ptr);
			break;
		case SENSOR_VAL_TYPE_READ_DUAL_OTP:
			#ifdef S5K3L8XXM3_DUAL_OTP
			ret = s5k3l8xxm3_dual_otp_read(handle, param_ptr);
			#endif
			break;
		case SENSOR_VAL_TYPE_PARSE_OTP:
			ret = s5k3l8xxm3_parse_otp(handle, param_ptr);
			break;
		case SENSOR_VAL_TYPE_PARSE_DUAL_OTP:
			#ifdef S5K3L8XXM3_DUAL_OTP
			ret = s5k3l8xxm3_parse_dual_otp(handle, param_ptr);
			#endif
			break;
		case SENSOR_VAL_TYPE_WRITE_OTP:
			//rtn = _s5k3l8xxm3_write_otp(handle, (uint32_t)param_ptr->pval);
			break;
		case SENSOR_VAL_TYPE_SHUTTER:
			*((uint32_t*)param_ptr->pval) = s5k3l8xxm3_read_shutter(handle);
			break;
		case SENSOR_VAL_TYPE_READ_OTP_GAIN:
			*((uint32_t*)param_ptr->pval) = s5k3l8xxm3_read_gain(handle);
			break;
		case SENSOR_VAL_TYPE_GET_STATIC_INFO:
			ret = s5k3l8xxm3_get_static_info(handle, param_ptr->pval);
			break;
		case SENSOR_VAL_TYPE_GET_FPS_INFO:
			ret = s5k3l8xxm3_get_fps_info(handle, param_ptr->pval);
			break;
		case SENSOR_VAL_TYPE_GET_PDAF_INFO:
			ret = s5k3l8xxm3_get_pdaf_info(handle, param_ptr->pval);
			break;
		default:
			break;
	}
    ret = SENSOR_SUCCESS;

	return ret;
}

#define AK7371_VCM_SLAVE_ADDR 0x18>>1 
static uint32_t _ak7371_init(SENSOR_HW_HANDLE handle, uint32_t mode)
{
	uint8_t cmd_val[2] = {0x00};
	uint16_t slave_addr = 0;
	uint16_t cmd_len = 0;
	uint32_t ret_value = SENSOR_SUCCESS;
	SENSOR_LOGI("SENSOR_S5K3L8XX: vcm mode init %d",mode);

	slave_addr = AK7371_VCM_SLAVE_ADDR;
	switch (mode) {
	case 1:
		break;

	case 2:
	{
		cmd_len = 2;
		cmd_val[0] = 0x02;
		cmd_val[1] = 0x00;
		ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);
		if(ret_value){
			SENSOR_LOGI("SENSOR_S5K3L8XX: _ak7371_init  fail!");
		}

		usleep(200);
		
	}
		break;

	}

	return ret_value;
}
static unsigned int m_vcm_pos=0;
uint32_t _vcm_ak7371_set_position(SENSOR_HW_HANDLE handle, uint32_t pos)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint8_t cmd_val[2] = {0x00};
	uint16_t  slave_addr = 0;
	uint16_t cmd_len = 0;
	uint32_t time_out = 0;

	if ((int32_t)pos < 0)
		pos = 0;
	else if ((int32_t)pos > 0x3FF)
		pos = 0x3FF;
	m_vcm_pos=pos&0x3FF;

	slave_addr = AK7371_VCM_SLAVE_ADDR;
	cmd_len = 2;
	cmd_val[0] = 0x00;
	cmd_val[1] = (pos&0x3ff)>>2;
	ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);
	cmd_val[0] = 0x01;
	cmd_val[1] = (pos&0x03)<<6;
	ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);
	return ret_value;
}

/*==============================================================================
 * Description:
 * identify sensor id
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t s5k3l8xxm3_identify(SENSOR_HW_HANDLE handle,uint32_t param)
{
	uint16_t pid_value = 0x00;
	uint16_t ver_value = 0x00;
	uint32_t ret_value = SENSOR_FAIL;

	SENSOR_PRINT("mipi raw identify");

	pid_value = Sensor_ReadReg(s5k3l8xxm3_PID_ADDR);

	if (s5k3l8xxm3_PID_VALUE == pid_value) {
		ver_value = Sensor_ReadReg(s5k3l8xxm3_VER_ADDR);
		SENSOR_PRINT("Identify: PID = %x, VER = %x", pid_value, ver_value);
		if (s5k3l8xxm3_VER_VALUE == ver_value) {
			SENSOR_PRINT_HIGH("this is s5k3l8xxm3 sensor");
			//_ak7371_init(handle, 2);
			vcm_ak7371_init(handle,2);
			#ifdef FEATURE_OTP
			/*if read otp info failed or module id mismatched ,identify failed ,return SENSOR_FAIL ,exit identify*/
			if(PNULL!=s_s5k3l8xxm3_raw_param_tab_ptr->identify_otp){
				SENSOR_PRINT("identify module_id=0x%x",s_s5k3l8xxm3_raw_param_tab_ptr->param_id);
				//set default value
				memset(s_s5k3l8xxm3_otp_info_ptr, 0x00, sizeof(struct otp_info_t));
				ret_value = s_s5k3l8xxm3_raw_param_tab_ptr->identify_otp(handle,s_s5k3l8xxm3_otp_info_ptr);
				if(SENSOR_SUCCESS == ret_value ){
					SENSOR_PRINT("identify otp sucess! module_id=0x%x, module_name=%s",s_s5k3l8xxm3_raw_param_tab_ptr->param_id,MODULE_NAME);
				} else{
					SENSOR_PRINT("identify otp fail! exit identify");
					return ret_value;
				}
			} else{
			SENSOR_PRINT("no identify_otp function!");
			}

			#endif
			s5k3l8xxm3_init_mode_fps_info(handle);
			ret_value = SENSOR_SUCCESS;
			
		} else {
			SENSOR_PRINT_HIGH("Identify this is %x%x sensor", pid_value, ver_value);
		}
	} else {
		SENSOR_PRINT_HIGH("sensor identify fail, pid_value = %x", pid_value);
	}

	return ret_value;
}

/*==============================================================================
 * Description:
 * get resolution trim
 *
 *============================================================================*/
static unsigned long s5k3l8xxm3_get_resolution_trim_tab(SENSOR_HW_HANDLE handle,uint32_t param)
{
	return (unsigned long) s_s5k3l8xxm3_resolution_trim_tab;
}

/*==============================================================================
 * Description:
 * before snapshot
 * you can change this function if it's necessary
 *============================================================================*/
static uint32_t s5k3l8xxm3_before_snapshot(SENSOR_HW_HANDLE handle,uint32_t param)
{
	uint32_t cap_shutter = 0;
	uint32_t prv_shutter = 0;
	uint32_t gain = 0;
	uint32_t cap_gain = 0;
	uint32_t capture_mode = param & 0xffff;
	uint32_t preview_mode = (param >> 0x10) & 0xffff;

	uint32_t prv_linetime = s_s5k3l8xxm3_resolution_trim_tab[preview_mode].line_time;
	uint32_t cap_linetime = s_s5k3l8xxm3_resolution_trim_tab[capture_mode].line_time;

	s_current_default_frame_length = s5k3l8xxm3_get_default_frame_length(handle,capture_mode);
	SENSOR_PRINT("capture_mode = %d", capture_mode);

	if (preview_mode == capture_mode) {
		cap_shutter = s_sensor_ev_info.preview_shutter;
		cap_gain = s_sensor_ev_info.preview_gain;
		goto snapshot_info;
	}

	prv_shutter = s_sensor_ev_info.preview_shutter;	//s5k3l8xxm3_read_shutter();
	gain = s_sensor_ev_info.preview_gain;	//s5k3l8xxm3_read_gain();

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
	cap_shutter = s5k3l8xxm3_write_exposure_dummy(handle,cap_shutter,0,0);
	cap_gain = gain;
	s5k3l8xxm3_write_gain(handle,cap_gain);
	SENSOR_PRINT("preview_shutter = 0x%x, preview_gain = 0x%x",
		     s_sensor_ev_info.preview_shutter, s_sensor_ev_info.preview_gain);

	SENSOR_PRINT("capture_shutter = 0x%x, capture_gain = 0x%x", cap_shutter, cap_gain);
snapshot_info:
	
	Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_EXPOSURETIME, cap_shutter);

	return SENSOR_SUCCESS;
}

/*==============================================================================
 * Description:
 * get the shutter from isp
 * please don't change this function unless it's necessary
 *============================================================================*/
static unsigned long s5k3l8xxm3_write_exposure(SENSOR_HW_HANDLE handle,unsigned long param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t exposure_line = 0x00;
	uint16_t dummy_line = 0x00;
	uint16_t size_index=0x00;
	struct sensor_ex_exposure  *ex = (struct sensor_ex_exposure*)param;
	
	if (!param) {
		SENSOR_PRINT_ERR("param is NULL !!!");
		return ret_value;
	}

	exposure_line = ex->exposure;
	dummy_line = ex->dummy;
	size_index = ex->size_index;

	SENSOR_PRINT("size_index=%d, exposure_line = %d, dummy_line=%d",size_index,exposure_line,dummy_line);
	s_current_default_frame_length = s5k3l8xxm3_get_default_frame_length(handle,size_index);

	ret_value = s5k3l8xxm3_write_exposure_dummy(handle, exposure_line, dummy_line, size_index);

	return ret_value;
}

/*==============================================================================
 * Description:
 * get the parameter from isp to real gain
 * you mustn't change the funcion !
 *============================================================================*/
static uint32_t isp_to_real_gain(SENSOR_HW_HANDLE handle,uint32_t param)
{
	uint32_t real_gain = 0;

	
	real_gain=param;

	return real_gain;
}

/*==============================================================================
 * Description:
 * write gain value to sensor
 * you can change this function if it's necessary
 *============================================================================*/
static uint32_t s5k3l8xxm3_write_gain_value(SENSOR_HW_HANDLE handle,unsigned long param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	float real_gain = 0;

	//real_gain = isp_to_real_gain(handle,param);
	param = param < SENSOR_BASE_GAIN ? SENSOR_BASE_GAIN : param;

	real_gain = (float)param * SENSOR_BASE_GAIN / ISP_BASE_GAIN*1.0;

	SENSOR_PRINT("real_gain = %f", real_gain);

	s_sensor_ev_info.preview_gain = real_gain;
	s5k3l8xxm3_write_gain(handle,real_gain);

	return ret_value;
}

#ifndef CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
/*==============================================================================
 * Description:
 * write parameter to vcm
 * please add your VCM function to this function
 *============================================================================*/
static uint32_t s5k3l8xxm3_write_af(SENSOR_HW_HANDLE handle,uint32_t param)
{
	return vcm_ak7371_set_position(handle,param);
}
#endif

/*==============================================================================
 * Description:
 * write parameter to frame sync
 * please add your frame sync function to this function
 *============================================================================*/

static unsigned long _s5k3l8xxm3_SetMaster_FrameSync(SENSOR_HW_HANDLE handle, unsigned long param)
{
	Sensor_WriteReg(0x30c2, 0x0100);
	//Sensor_WriteReg(0x30c3, 0x0100);
	Sensor_WriteReg(0x30c4, 0x0100);
	return 0;
}

/*==============================================================================
 * Description:
 * mipi stream on
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t s5k3l8xxm3_stream_on(SENSOR_HW_HANDLE handle,uint32_t param)
{
	SENSOR_PRINT("E");
	//_s5k3l8xxm3_SetMaster_FrameSync(handle,param);

	Sensor_WriteReg(0x0100, 0x0100);
	/*delay*/
	//usleep(10 * 1000);

	return 0;
}

/*==============================================================================
 * Description:
 * mipi stream off
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t s5k3l8xxm3_stream_off(SENSOR_HW_HANDLE handle,uint32_t param)
{
	SENSOR_PRINT("E");

	Sensor_WriteReg(0x0100, 0x0000);
	/*delay*/
	usleep(50 * 1000);

	return 0;
}

/*==============================================================================
 * Description:
 * all ioctl functoins
 * you can add functions reference SENSOR_IOCTL_FUNC_TAB_T from sensor_drv_u.h
 *
 * add ioctl functions like this:
 * .power = s5k3l8xxm3_power_on,
 *============================================================================*/
static SENSOR_IOCTL_FUNC_TAB_T s_s5k3l8xxm3_ioctl_func_tab = {
	.power = s5k3l8xxm3_power_on,
	.identify = s5k3l8xxm3_identify,
	.get_trim = s5k3l8xxm3_get_resolution_trim_tab,
	.before_snapshort = s5k3l8xxm3_before_snapshot,
	.ex_write_exp = s5k3l8xxm3_write_exposure,
	.write_gain_value = s5k3l8xxm3_write_gain_value,
	#ifndef CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
	.af_enable = s5k3l8xxm3_write_af,
	#endif
	.stream_on = s5k3l8xxm3_stream_on,
	.stream_off = s5k3l8xxm3_stream_off,
	.cfg_otp=s5k3l8xxm3_access_val,	
	//.af_enable =_s5k3l8xxm3_write_af,
	//.group_hold_on = s5k3l8xxm3_group_hold_on,
	//.group_hold_of = s5k3l8xxm3_group_hold_off,
};
