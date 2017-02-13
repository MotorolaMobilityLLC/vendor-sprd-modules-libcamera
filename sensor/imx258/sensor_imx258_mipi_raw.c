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
#ifdef CONFIG_AF_VCM_DW9800W
//#include "../vcm/vcm_dw9800.h"
#else
#include "af_bu64297gwz.h"
#endif
#if defined(CONFIG_CAMERA_ISP_DIR_3)
#include "sensor_imx258_raw_param_v3.c"
#else
#include "imx258_param/sensor_imx258_raw_param_main.c"
#endif
#include "sensor_imx258_otp_truly.h"

#define DW9800_VCM_SLAVE_ADDR (0x18 >> 1)

#define SENSOR_NAME			"imx258_mipi_raw"
#define I2C_SLAVE_ADDR			0x34 //0x20    /* 16bit slave address*/


#define BINNING_FACTOR 			2
#define imx258_PID_ADDR			0x0016
#define imx258_PID_VALUE		0x02
#define imx258_VER_ADDR			0x0017
#define imx258_VER_VALUE		0x58


/* sensor parameters begin */
/* effective sensor output image size */
#define SNAPSHOT_WIDTH			4208 //5344
#define SNAPSHOT_HEIGHT			3120//4016
#define PREVIEW_WIDTH			4208//2672
#define PREVIEW_HEIGHT			3120//2008

/*Mipi output*/
#define LANE_NUM			4
#define RAW_BITS			10

#define SNAPSHOT_MIPI_PER_LANE_BPS	858
#define PREVIEW_MIPI_PER_LANE_BPS	660

/* please ref your spec */
#define FRAME_OFFSET			10
#define SENSOR_MAX_GAIN			0xF0
#define SENSOR_BASE_GAIN		0x20
#define SENSOR_MIN_SHUTTER		4

/* isp parameters, please don't change it*/
#if defined(CONFIG_CAMERA_ISP_VERSION_V3) || defined(CONFIG_CAMERA_ISP_VERSION_V4)
#define ISP_BASE_GAIN			0x80
#else
#define ISP_BASE_GAIN			0x10
#endif

/* please don't change it */
#define EX_MCLK				24

struct hdr_info_t {
	uint32_t capture_max_shutter;
	uint32_t capture_shutter;
	uint32_t capture_gain;
};

struct sensor_ev_info_t {
	uint16_t preview_shutter;
	float preview_gain;
};

static unsigned long imx258_access_val(SENSOR_HW_HANDLE handle, unsigned long param);
static uint32_t imx258_init_mode_fps_info(SENSOR_HW_HANDLE handle);

/*==============================================================================
 * Description:
 * global variable
 *============================================================================*/
static struct hdr_info_t s_hdr_info;
static uint32_t s_current_default_frame_length;
struct sensor_ev_info_t s_sensor_ev_info;

static SENSOR_IOCTL_FUNC_TAB_T s_imx258_ioctl_func_tab;
struct sensor_raw_info *s_imx258_mipi_raw_info_ptr = &s_imx258_mipi_raw_info;

static const SENSOR_REG_T imx258_init_setting[] = {
    //Address   value
    {0x0136,0x18},
    {0x0137,0x00},
    {0x3051,0x00},
    {0x6B11,0xCF},
    {0x7FF0,0x08},
    {0x7FF1,0x0F},
    {0x7FF2,0x08},
    {0x7FF3,0x1B},
    {0x7FF4,0x23},
    {0x7FF5,0x60},
    {0x7FF6,0x00},
    {0x7FF7,0x01},
    {0x7FF8,0x00},
    {0x7FF9,0x78},
    {0x7FFA,0x01},
    {0x7FFB,0x00},
    {0x7FFC,0x00},
    {0x7FFD,0x00},
    {0x7FFE,0x00},
    {0x7FFF,0x03},
    {0x7F76,0x03},
    {0x7F77,0xFE},
    {0x7FA8,0x03},
    {0x7FA9,0xFE},
    {0x7B24,0x81},
    {0x7B25,0x01},
    {0x6564,0x07},
    {0x6B0D,0x41},
    {0x653D,0x04},
    {0x6B05,0x8C},
    {0x6B06,0xF9},
    {0x6B08,0x65},
    {0x6B09,0xFC},
    {0x6B0A,0xCF},
    {0x6B0B,0xD2},
    {0x6700,0x0E},
    {0x6707,0x0E},
    {0x9104,0x00},
    {0x7421,0x1C},
    {0x7423,0xD7},
    {0x5F04,0x00},
    {0x5F05,0xED},

};

static const SENSOR_REG_T imx258_2096x1552_setting[] = {
    /*4Lane
    reg_B30
    2096x1552(4:3) 30fps
    H: 2096
    V: 1552
    Output format Setting
        Address value*/
        {0x0112,0x0A},
        {0x0113,0x0A},
        {0x0114,0x03},//Clock Setting
        {0x0301,0x05},
        {0x0303,0x02},
        {0x0305,0x04},
        {0x0306,0x00},
        {0x0307,0x6E},
        {0x0309,0x0A},
        {0x030B,0x01},
        {0x030D,0x02},
        {0x030E,0x00},
        {0x030F,0xD8},
        {0x0310,0x00},
        {0x0820,0x0A},
        {0x0821,0x50},
        {0x0822,0x00},
        {0x0823,0x00},//Line Length Setting
        {0x0342,0x14},
        {0x0343,0xE8},//Frame Length Setting
        {0x0340,0x06},
        {0x0341,0x6C},//ROI Setting
        {0x0344,0x00},
        {0x0345,0x00},
        {0x0346,0x00},
        {0x0347,0x00},
        {0x0348,0x10},
        {0x0349,0x6F},
        {0x034A,0x0C},
        {0x034B,0x2F},//Analog Image Size Setting
        {0x0381,0x01},
        {0x0383,0x01},
        {0x0385,0x01},
        {0x0387,0x01},
        {0x0900,0x01},
        {0x0901,0x12},//Digital Image Size Setting
        {0x0401,0x01},
        {0x0404,0x00},
        {0x0405,0x20},
        {0x0408,0x00},
        {0x0409,0x04},
        {0x040A,0x00},
        {0x040B,0x04},
        {0x040C,0x10},
        {0x040D,0x68},
        {0x040E,0x06},
        {0x040F,0x10},
        {0x3038,0x00},
        {0x303A,0x00},
        {0x303B,0x10},
        {0x300D,0x00},//Output Size Setting
        {0x034C,0x08},
        {0x034D,0x30},
        {0x034E,0x06},
        {0x034F,0x10},
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
        {0x7BCD,0x01},//Added Setting(AF)
        {0x94DC,0x20},//Added Setting(IQ)
        {0x94DD,0x20},
        {0x94DE,0x20},
        {0x95DC,0x20},
        {0x95DD,0x20},
        {0x95DE,0x20},
        {0x7FB0,0x00},
        {0x9010,0x3E},
        {0x9419,0x50},
        {0x941B,0x50},
        {0x9519,0x50},
        {0x951B,0x50},//Added Setting(mode)
        {0x3030,0x00},
        {0x3032,0x00},//0},//1},
        {0x0220,0x00},
        {0x4041,0x00},
};

static const SENSOR_REG_T imx258_4208x3120_setting[] = {
    /*4Lane
    reg_A30
    Full (4:3) 30fps
    H: 4208
    V: 3120
    Output format Setting
        Address value*/
        {0x0112,0x0A},
        {0x0113,0x0A},
        {0x0114,0x03},//Clock Setting
        {0x0301,0x05},
        {0x0303,0x02},
        {0x0305,0x04},
        {0x0306,0x00},
        {0x0307,0xD8},
        {0x0309,0x0A},
        {0x030B,0x01},
        {0x030D,0x02},
        {0x030E,0x00},
        {0x030F,0xD8},
        {0x0310,0x00},
        {0x0820,0x14},
        {0x0821,0x40},
        {0x0822,0x00},
        {0x0823,0x00},//Line Length Setting
        {0x0342,0x14},
        {0x0343,0xE8},//Frame Length Setting
        {0x0340,0x0C},
        {0x0341,0x98},//ROI Setting
        {0x0344,0x00},
        {0x0345,0x00},
        {0x0346,0x00},
        {0x0347,0x00},
        {0x0348,0x10},
        {0x0349,0x6F},
        {0x034A,0x0C},
        {0x034B,0x2F},//Analog Image Size Setting
        {0x0381,0x01},
        {0x0383,0x01},
        {0x0385,0x01},
        {0x0387,0x01},
        {0x0900,0x00},
        {0x0901,0x11},//Digital Image Size Setting
        {0x0401,0x00},
        {0x0404,0x00},
        {0x0405,0x10},
        {0x0408,0x00},
        {0x0409,0x00},
        {0x040A,0x00},
        {0x040B,0x00},
        {0x040C,0x10},
        {0x040D,0x70},
        {0x040E,0x0C},
        {0x040F,0x30},
        {0x3038,0x00},
        {0x303A,0x00},
        {0x303B,0x10},
        {0x300D,0x00},//Output Size Setting
        {0x034C,0x10},
        {0x034D,0x70},
        {0x034E,0x0C},
        {0x034F,0x30},//Integration Time Setting
        {0x0202,0x0C},
        {0x0203,0x8E},//Gain Setting
        {0x0204,0x00},
        {0x0205,0x00},
        {0x020E,0x01},
        {0x020F,0x00},
        {0x0210,0x01},
        {0x0211,0x00},
        {0x0212,0x01},
        {0x0213,0x00},
        {0x0214,0x01},
        {0x0215,0x00},//Added Setting(AF)
        {0x7BCD,0x00},//Added Setting(IQ)
        {0x94DC,0x20},
        {0x94DD,0x20},
        {0x94DE,0x20},
        {0x95DC,0x20},
        {0x95DD,0x20},
        {0x95DE,0x20},
        {0x7FB0,0x00},
        {0x9010,0x3E},
        {0x9419,0x50},
        {0x941B,0x50},
        {0x9519,0x50},
        {0x951B,0x50},//Added Setting(mode)
 #ifdef PDAF_TYPE2
       {0x3030,0x01},
 #else
        {0x3030,0x00},//1},
#endif
        {0x3032,0x01},//0},//1},
        {0x0220,0x00},
        {0x4041,0x00},
 #ifdef PDAF_TYPE3
        {0x3052,0x00},//1},
        {0x7BCB,0x00},
        {0x7BC8,0x00},
        {0x7BC9,0x00},
#endif
};

static const SENSOR_REG_T imx258_1048x780_setting[] = {
    /*4Lane
    reg_E30
    HV1/4 (4:3) 120fps
    H: 1048
    V: 780
    Output format Setting
        Address value*/
        {0x0112,0x0A},
        {0x0113,0x0A},
        {0x0114,0x03},//Clock Setting
        {0x0301,0x05},
        {0x0303,0x02},
        {0x0305,0x04},
        {0x0306,0x00},
        {0x0307,0xD8},//6E},
        {0x0309,0x0A},
        {0x030B,0x01},
        {0x030D,0x02},
        {0x030E,0x00},
        {0x030F,0xD8},
        {0x0310,0x00},
        {0x0820,0x14},//0A},
        {0x0821,0x40},//50},
        {0x0822,0x00},
        {0x0823,0x00},//Clock Adjustment Setting
        {0x4648,0x7F},
        {0x7420,0x00},
        {0x7421,0x1C},
        {0x7422,0x00},
        {0x7423,0xD7},
        {0x9104,0x00},//Line Length Setting
        {0x0342,0x14},
        {0x0343,0xE8},//Frame Length Setting
        {0x0340,0x03},
        {0x0341,0x2C},//34},//ROI Setting
        {0x0344,0x00},
        {0x0345,0x00},
        {0x0346,0x00},
        {0x0347,0x00},
        {0x0348,0x10},
        {0x0349,0x6F},
        {0x034A,0x0C},
        {0x034B,0x2F},//Analog Image Size Setting
        {0x0381,0x01},
        {0x0383,0x01},
        {0x0385,0x01},
        {0x0387,0x01},
        {0x0900,0x01},
        {0x0901,0x14},//Digital Image Size Setting
        {0x0401,0x01},
        {0x0404,0x00},
        {0x0405,0x40},
        {0x0408,0x00},
        {0x0409,0x00},
        {0x040A,0x00},
        {0x040B,0x00},
        {0x040C,0x10},
        {0x040D,0x70},
        {0x040E,0x03},
        {0x040F,0x0C},
        {0x3038,0x00},
        {0x303A,0x00},
        {0x303B,0x10},
        {0x300D,0x00},//Output Size Setting
        {0x034C,0x04},
        {0x034D,0x18},
        {0x034E,0x03},
        {0x034F,0x0C},//Integration Time Setting
        {0x0202,0x03},
        {0x0203,0x22},//2A},//Gain Setting
        {0x0204,0x00},
        {0x0205,0x00},
        {0x020E,0x01},
        {0x020F,0x00},
        {0x0210,0x01},
        {0x0211,0x00},
        {0x0212,0x01},
        {0x0213,0x00},
        {0x0214,0x01},
        {0x0215,0x00},//Added Setting(AF)
        {0x7BCD,0x00},//Added Setting(IQ)
        {0x94DC,0x20},
        {0x94DD,0x20},
        {0x94DE,0x20},
        {0x95DC,0x20},
        {0x95DD,0x20},
        {0x95DE,0x20},
        {0x7FB0,0x00},
        {0x9010,0x3E},
        {0x9419,0x50},
        {0x941B,0x50},
        {0x9519,0x50},
        {0x951B,0x50},//Added Setting(mode)
        {0x3030,0x00},
        {0x3032,0x00},
        {0x0220,0x00},
        {0x4041,0x00},

};

static SENSOR_REG_TAB_INFO_T s_imx258_resolution_tab_raw[SENSOR_MODE_MAX] = {
	{ADDR_AND_LEN_OF_ARRAY(imx258_init_setting), 0, 0, EX_MCLK, SENSOR_IMAGE_FORMAT_RAW},
	{ADDR_AND_LEN_OF_ARRAY(imx258_1048x780_setting), 1048, 780, EX_MCLK, SENSOR_IMAGE_FORMAT_RAW},
//	{ADDR_AND_LEN_OF_ARRAY(imx258_2096x1552_setting), 2096, 1552, EX_MCLK, SENSOR_IMAGE_FORMAT_RAW},
	{ADDR_AND_LEN_OF_ARRAY(imx258_4208x3120_setting), 4208, 3120, EX_MCLK, SENSOR_IMAGE_FORMAT_RAW},
	//{ADDR_AND_LEN_OF_ARRAY(imx258_5344x4016_setting), 5344, 4016, EX_MCLK, SENSOR_IMAGE_FORMAT_RAW},
};

static SENSOR_TRIM_T s_imx258_resolution_trim_tab[SENSOR_MODE_MAX] = {
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 1048, 780, 10325*4 , 1296, 812, {0, 0, 1048, 780}},
	//{0, 0, 2096, 1552, 20650, 1296, 3224, {0, 0, 2096, 1552}},
	{0, 0, 4208, 3120, 10325, 1296, 3224, {0, 0, 4208, 3120}},
	//{0, 0, 5344, 4016, 10040, SNAPSHOT_MIPI_PER_LANE_BPS, 4140, {0, 0, 5344, 4016}},
};

static const SENSOR_REG_T s_imx258_preview_size_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
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

static const SENSOR_REG_T s_imx258_capture_size_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
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

static SENSOR_VIDEO_INFO_T s_imx258_video_info[SENSOR_MODE_MAX] = {
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{30, 30, 270, 90}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
	 (SENSOR_REG_T **) s_imx258_preview_size_video_tab},
	{{{2, 5, 338, 1000}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
	 (SENSOR_REG_T **) s_imx258_capture_size_video_tab},
};

/*==============================================================================
 * Description:
 * set video mode
 *
 *============================================================================*/
static uint32_t imx258_set_video_mode(SENSOR_HW_HANDLE handle, uint32_t param)
{
	SENSOR_REG_T_PTR sensor_reg_ptr;
	uint16_t i = 0x00;
	uint32_t mode;

	if (param >= SENSOR_VIDEO_MODE_MAX)
		return 0;

	if (SENSOR_SUCCESS != Sensor_GetMode(&mode)) {
		SENSOR_LOGI("fail.");
		return SENSOR_FAIL;
	}

	if (PNULL == s_imx258_video_info[mode].setting_ptr) {
		SENSOR_LOGI("fail.");
		return SENSOR_FAIL;
	}

	sensor_reg_ptr = (SENSOR_REG_T_PTR) & s_imx258_video_info[mode].setting_ptr[param];
	if (PNULL == sensor_reg_ptr) {
		SENSOR_LOGI("fail.");
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
 * sensor static info need by isp
 * please modify this variable acording your spec
 *============================================================================*/
static SENSOR_STATIC_INFO_T s_imx258_static_info = {
	220,	//f-number,focal ratio
	462,	//focal_length;
	0,	//max_fps,max fps of sensor's all settings,it will be calculated from sensor mode fps
	16*16,	//max_adgain,AD-gain
	0,	//ois_supported;
	0,	//pdaf_supported;
	1,	//exp_valid_frame_num;N+2-1
	64,	//clamp_level,black level
	1,	//adgain_valid_frame_num;N+1-1
};

/*==============================================================================
 * Description:
 * sensor fps info related to sensor mode need by isp
 * please modify this variable acording your spec
 *============================================================================*/
static SENSOR_MODE_FPS_INFO_T s_imx258_mode_fps_info = {
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
 * sensor all info
 * please modify this variable acording your spec
 *============================================================================*/
SENSOR_INFO_T g_imx258_mipi_raw_info = {
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
	{{imx258_PID_ADDR, imx258_PID_VALUE}
	 ,
	 {imx258_VER_ADDR, imx258_VER_VALUE}
	 }
	,
	/* voltage of avdd */
	SENSOR_AVDD_2800MV,
	/* max width of source image */
	SNAPSHOT_WIDTH,
	/* max height of source image */
	SNAPSHOT_HEIGHT,
	/* name of sensor */
	(cmr_s8 *)SENSOR_NAME,
	/* define in SENSOR_IMAGE_FORMAT_E enum,SENSOR_IMAGE_FORMAT_MAX
	 * if set to SENSOR_IMAGE_FORMAT_MAX here,
	 * image format depent on SENSOR_REG_TAB_INFO_T
	 */
	SENSOR_IMAGE_FORMAT_RAW,
	/*  pattern of input image form sensor */
	SENSOR_IMAGE_PATTERN_RAWRGB_R,
	/* point to resolution table information structure */
	s_imx258_resolution_tab_raw,
	/* point to ioctl function table */
	&s_imx258_ioctl_func_tab,
	/* information and table about Rawrgb sensor */
	&s_imx258_mipi_raw_info_ptr,
	/* extend information about sensor
	 * like &g_imx132_ext_info
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
	0,//threshold_eb
	0,//threshold_mode
	0,//threshold_start
	0,//threshold_end
	0,//i2c_dev_handler
	{SENSOR_INTERFACE_TYPE_CSI2, LANE_NUM, RAW_BITS, 0},//sensor_interface
	0,//video_tab_info_ptr
	/* skip frame num while change setting */
	1,
	/* horizontal  view angle*/
	35,
	/* vertical view angle*/
	35,
	(cmr_s8 *)"imx258v1",//sensor version info
};

/*==============================================================================
 * Description:
 * get default frame length
 *
 *============================================================================*/
static uint32_t imx258_get_default_frame_length(SENSOR_HW_HANDLE handle, uint32_t mode)
{
	return s_imx258_resolution_trim_tab[mode].frame_line;
}

/*==============================================================================
 * Description:
 * write group-hold on to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void imx258_group_hold_on(SENSOR_HW_HANDLE handle)
{
	//Sensor_WriteReg(0x0104, 0x01);
}

/*==============================================================================
 * Description:
 * write group-hold off to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void imx258_group_hold_off(SENSOR_HW_HANDLE handle)
{
	//Sensor_WriteReg(0x0104, 0x00);
}


/*==============================================================================
 * Description:
 * read gain from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static uint16_t imx258_read_gain(SENSOR_HW_HANDLE handle)
{
	uint16_t gain_a= 0;
	uint16_t gain_d= 0;

	gain_a = Sensor_ReadReg(0x0205);
	gain_d = Sensor_ReadReg(0x0210);

	return gain_a*gain_d;
}

/*==============================================================================
 * Description:
 * write gain to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void imx258_write_gain(SENSOR_HW_HANDLE handle, float gain)
{
	uint32_t sensor_again = 0;
	uint32_t sensor_dgain = 0;
	float temp_gain;

	gain = gain/32.0;

	temp_gain = gain;
	if (temp_gain < 1.0)
		temp_gain = 1.0;
	else if (temp_gain > 16.0)
		temp_gain = 16.0;
	sensor_again = (uint16_t)(512.0 - 512.0 / temp_gain);
	Sensor_WriteReg(0x0204, (sensor_again>>8)& 0xFF);
	Sensor_WriteReg(0x0205, sensor_again & 0xFF);

	temp_gain = gain/16;
	if (temp_gain >16.0)
		temp_gain = 16.0;
	else if (temp_gain < 1.0)
		temp_gain = 1.0;
	sensor_dgain = (uint16_t)(256 * temp_gain);
	Sensor_WriteReg(0x020e, (sensor_dgain>>8)& 0xFF);
	Sensor_WriteReg(0x020f, sensor_dgain & 0xFF);
	Sensor_WriteReg(0x0210, (sensor_dgain>>8)& 0xFF);
	Sensor_WriteReg(0x0211, sensor_dgain & 0xFF);
	Sensor_WriteReg(0x0212, (sensor_dgain>>8)& 0xFF);
	Sensor_WriteReg(0x0213, sensor_dgain & 0xFF);
	Sensor_WriteReg(0x0214, (sensor_dgain>>8)& 0xFF);
	Sensor_WriteReg(0x0215, sensor_dgain & 0xFF);

	SENSOR_LOGI("realgain=%f,again=%d,dgain=%f", gain, sensor_again, temp_gain);

	//imx258_group_hold_off(handle);

}

/*==============================================================================
 * Description:
 * read frame length from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static uint16_t imx258_read_frame_length(SENSOR_HW_HANDLE handle)
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
static void imx258_write_frame_length(SENSOR_HW_HANDLE handle, uint32_t frame_len)
{
	Sensor_WriteReg(0x0340, (frame_len >> 8) & 0xff);
	Sensor_WriteReg(0x0341, frame_len & 0xff);
}

/*==============================================================================
 * Description:
 * read shutter from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t imx258_read_shutter(SENSOR_HW_HANDLE handle)
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
static void imx258_write_shutter(SENSOR_HW_HANDLE handle, uint32_t shutter)
{
	Sensor_WriteReg(0x0202, (shutter >> 8) & 0xff);
	Sensor_WriteReg(0x0203, shutter & 0xff);
}

/*==============================================================================
 * Description:
 * write exposure to sensor registers and get current shutter
 * please pay attention to the frame length
 * please don't change this function if it's necessary
 *============================================================================*/
static uint16_t imx258_update_exposure(SENSOR_HW_HANDLE handle, uint32_t shutter,uint32_t dummy_line)
{
	uint32_t dest_fr_len = 0;
	uint32_t cur_fr_len = 0;
	uint32_t fr_len = s_current_default_frame_length;
	int32_t offset = 0;

//	imx258_group_hold_on(handle);

	if (dummy_line > FRAME_OFFSET)
		offset = dummy_line;
	else
		offset = FRAME_OFFSET;
	dest_fr_len = ((shutter + offset) > fr_len) ? (shutter + offset) : fr_len;

	cur_fr_len = imx258_read_frame_length(handle);

	if (shutter < SENSOR_MIN_SHUTTER)
		shutter = SENSOR_MIN_SHUTTER;

	if (dest_fr_len != cur_fr_len)
		imx258_write_frame_length(handle, dest_fr_len);
write_sensor_shutter:
	/* write shutter to sensor registers */
	imx258_write_shutter(handle, shutter);
	return shutter;
}


/*==============================================================================
 * Description:
 * sensor power on
 * please modify this function acording your spec
 *============================================================================*/
static unsigned long imx258_power_on(SENSOR_HW_HANDLE handle, unsigned long power_on)
{
	SENSOR_AVDD_VAL_E dvdd_val = g_imx258_mipi_raw_info.dvdd_val;
	SENSOR_AVDD_VAL_E avdd_val = g_imx258_mipi_raw_info.avdd_val;
	SENSOR_AVDD_VAL_E iovdd_val = g_imx258_mipi_raw_info.iovdd_val;
	BOOLEAN power_down = g_imx258_mipi_raw_info.power_down_level;
	BOOLEAN reset_level = g_imx258_mipi_raw_info.reset_pulse_level;

	if (SENSOR_TRUE == power_on) {
		Sensor_PowerDown(power_down);
		Sensor_SetResetLevel(reset_level);
		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		Sensor_SetVoltage(SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED);
		usleep(10 * 1000);
		Sensor_SetVoltage(dvdd_val, avdd_val, iovdd_val);
		usleep(10 * 1000);
		Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);
		usleep(10 * 1000);
		Sensor_PowerDown(!power_down);
		Sensor_SetResetLevel(!reset_level);
		Sensor_SetMonitorVoltage(SENSOR_AVDD_2800MV);
	} else {
		Sensor_Reset(reset_level);
		Sensor_PowerDown(power_down);
		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		Sensor_SetVoltage(SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED);
		Sensor_SetMonitorVoltage(SENSOR_AVDD_CLOSED);
	}
	SENSOR_LOGI("(1:on, 0:off): %ld", power_on);
	return SENSOR_SUCCESS;
}

/*==============================================================================
 * Description:
 * calculate fps for every sensor mode according to frame_line and line_time
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t imx258_init_mode_fps_info(SENSOR_HW_HANDLE handle)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_LOGI("imx258_init_mode_fps_info:E");
	if(!s_imx258_mode_fps_info.is_init) {
		uint32_t i,modn,tempfps = 0;
		SENSOR_LOGI("imx258_init_mode_fps_info:start init");
		for(i = 0;i < NUMBER_OF_ARRAY(s_imx258_resolution_trim_tab); i++) {
			//max fps should be multiple of 30,it calulated from line_time and frame_line
			tempfps = s_imx258_resolution_trim_tab[i].line_time*s_imx258_resolution_trim_tab[i].frame_line;
				if(0 != tempfps) {
					tempfps = 1000000000/tempfps;
				modn = tempfps / 30;
				if(tempfps > modn*30)
					modn++;
				s_imx258_mode_fps_info.sensor_mode_fps[i].max_fps = modn*30;
				if(s_imx258_mode_fps_info.sensor_mode_fps[i].max_fps > 30) {
					s_imx258_mode_fps_info.sensor_mode_fps[i].is_high_fps = 1;
					s_imx258_mode_fps_info.sensor_mode_fps[i].high_fps_skip_num =
						s_imx258_mode_fps_info.sensor_mode_fps[i].max_fps/30;
				}
				if(s_imx258_mode_fps_info.sensor_mode_fps[i].max_fps >
						s_imx258_static_info.max_fps) {
					s_imx258_static_info.max_fps =
						s_imx258_mode_fps_info.sensor_mode_fps[i].max_fps;
				}
			}
			SENSOR_LOGI("mode %d,tempfps %d,frame_len %d,line_time: %d ",i,tempfps,
					s_imx258_resolution_trim_tab[i].frame_line,
					s_imx258_resolution_trim_tab[i].line_time);
			SENSOR_LOGI("mode %d,max_fps: %d ",
					i,s_imx258_mode_fps_info.sensor_mode_fps[i].max_fps);
			SENSOR_LOGI("is_high_fps: %d,highfps_skip_num %d",
					s_imx258_mode_fps_info.sensor_mode_fps[i].is_high_fps,
					s_imx258_mode_fps_info.sensor_mode_fps[i].high_fps_skip_num);
		}
		s_imx258_mode_fps_info.is_init = 1;
	}
	SENSOR_LOGI("imx258_init_mode_fps_info:X");
	return rtn;
}

/*==============================================================================
 * Description:
 * identify sensor id
 * please modify this function acording your spec
 *============================================================================*/
static unsigned long imx258_identify(SENSOR_HW_HANDLE handle, unsigned long param)
{
	uint8_t pid_value = 0x00;
	uint8_t ver_value = 0x00;
	uint32_t ret_value = SENSOR_FAIL;
	uint8_t i = 0x00;
	UNUSED(param);

	SENSOR_LOGI("imx258 mipi raw identify");

	pid_value = Sensor_ReadReg(imx258_PID_ADDR);
	if (imx258_PID_VALUE == pid_value) {
		ver_value = Sensor_ReadReg(imx258_VER_ADDR);
		SENSOR_LOGI("Identify: PID = %x, VER = %x", pid_value, ver_value);
		if (imx258_VER_VALUE == ver_value) {
			ret_value = SENSOR_SUCCESS;
			SENSOR_LOGI("this is imx258 sensor");
#ifdef CONFIG_AF_VCM_DW9800W
			vcm_dw9800_init(handle);
#else
			bu64297gwz_init(handle);
#endif
			imx258_init_mode_fps_info(handle);
		} else {
			SENSOR_LOGI("Identify this is %x%x sensor", pid_value, ver_value);
		}
	} else {
		SENSOR_LOGI("identify fail, pid_value = %x", pid_value);
	}

	return ret_value;
}

/*==============================================================================
 * Description:
 * get resolution trim
 *
 *============================================================================*/
static unsigned long imx258_get_resolution_trim_tab(SENSOR_HW_HANDLE handle, unsigned long param)
{
	UNUSED(param);
	return (unsigned long) s_imx258_resolution_trim_tab;
}

/*==============================================================================
 * Description:
 * before snapshot
 * you can change this function if it's necessary
 *============================================================================*/
static unsigned long imx258_before_snapshot(SENSOR_HW_HANDLE handle, unsigned long param)
{
	uint32_t cap_shutter = 0;
	uint32_t prv_shutter = 0;
	float gain = 0;
	float cap_gain = 0;
	uint32_t capture_mode = param & 0xffff;
	uint32_t preview_mode = (param >> 0x10) & 0xffff;

	uint32_t prv_linetime = s_imx258_resolution_trim_tab[preview_mode].line_time;
	uint32_t cap_linetime = s_imx258_resolution_trim_tab[capture_mode].line_time;

	s_current_default_frame_length = imx258_get_default_frame_length(handle, capture_mode);
	SENSOR_LOGI("capture_mode = %d,preview_mode=%d\n", capture_mode, preview_mode);

	if (preview_mode == capture_mode) {
		cap_shutter = s_sensor_ev_info.preview_shutter;
		cap_gain = s_sensor_ev_info.preview_gain;
		goto snapshot_info;
	}

	prv_shutter = s_sensor_ev_info.preview_shutter;	//imx132_read_shutter();
	gain = s_sensor_ev_info.preview_gain;	//imx132_read_gain();

	Sensor_SetMode(capture_mode);
	Sensor_SetMode_WaitDone();

	cap_shutter = prv_shutter * prv_linetime / cap_linetime;// * BINNING_FACTOR;

/*	while (gain >= (2 * SENSOR_BASE_GAIN)) {
		if (cap_shutter * 2 > s_current_default_frame_length)
			break;
		cap_shutter = cap_shutter * 2;
		gain = gain / 2;
	}
*/
	cap_shutter = imx258_update_exposure(handle, cap_shutter,0);
	cap_gain = gain;
	imx258_write_gain(handle, cap_gain);
	SENSOR_LOGI("preview_shutter = %d, preview_gain = %f",
		     s_sensor_ev_info.preview_shutter, s_sensor_ev_info.preview_gain);

	SENSOR_LOGI("capture_shutter = %d, capture_gain = %f", cap_shutter, cap_gain);
snapshot_info:
	s_hdr_info.capture_shutter = cap_shutter; //imx132_read_shutter();
	s_hdr_info.capture_gain = cap_gain; //imx132_read_gain();
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
static unsigned long imx258_write_exposure(SENSOR_HW_HANDLE handle, unsigned long param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t exposure_line = 0x00;
	uint16_t dummy_line = 0x00;
	uint16_t mode = 0x00;

	exposure_line = param & 0xffff;
	dummy_line = (param >> 0x10) & 0xfff; /*for cits frame rate test*/
	mode = (param >> 0x1c) & 0x0f;

	SENSOR_LOGI("current mode = %d, exposure_line = %d, dummy_line=%d", mode, exposure_line,dummy_line);
	s_current_default_frame_length = imx258_get_default_frame_length(handle, mode);

	s_sensor_ev_info.preview_shutter = imx258_update_exposure(handle, exposure_line,dummy_line);

	return ret_value;
}

static unsigned long imx258_ex_write_exposure(SENSOR_HW_HANDLE handle, unsigned long param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t exposure_line = 0x00;
	uint16_t dummy_line = 0x00;
	uint16_t mode = 0x00;
	struct sensor_ex_exposure  *ex = (struct sensor_ex_exposure*)param;

	if (!param) {
		SENSOR_LOGI("param is NULL !!!");
		return ret_value;
	}

	exposure_line = ex->exposure;
	dummy_line = ex->dummy;
	mode = ex->size_index;

	SENSOR_LOGI("current mode = %d, exposure_line = %d, dummy_line=%d", mode, exposure_line,dummy_line);
	s_current_default_frame_length = imx258_get_default_frame_length(handle, mode);

	s_sensor_ev_info.preview_shutter = imx258_update_exposure(handle, exposure_line,dummy_line);

	return ret_value;
}

/*==============================================================================
 * Description:
 * get the parameter from isp to real gain
 * you mustn't change the funcion !
 *============================================================================*/
static uint32_t isp_to_real_gain(SENSOR_HW_HANDLE handle, uint32_t param)
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
static unsigned long imx258_write_gain_value(SENSOR_HW_HANDLE handle, unsigned long param)
{
	unsigned long ret_value = SENSOR_SUCCESS;
	float real_gain = 0;

	real_gain = (float)param * SENSOR_BASE_GAIN / ISP_BASE_GAIN*1.0;

	SENSOR_LOGI("real_gain = %f", real_gain);

	s_sensor_ev_info.preview_gain = real_gain;
	imx258_write_gain(handle, real_gain);

	return ret_value;
}

/*==============================================================================
 * Description:
 * increase gain or shutter for hdr
 *
 *============================================================================*/
static void imx258_increase_hdr_exposure(SENSOR_HW_HANDLE handle, uint8_t ev_multiplier)
{
	uint32_t shutter_multiply = s_hdr_info.capture_max_shutter / s_hdr_info.capture_shutter;
	uint32_t gain = 0;

	if (0 == shutter_multiply)
		shutter_multiply = 1;

	if (shutter_multiply >= ev_multiplier) {
		imx258_update_exposure(handle, s_hdr_info.capture_shutter * ev_multiplier,0);
		imx258_write_gain(handle, s_hdr_info.capture_gain);
	} else {
		gain = s_hdr_info.capture_gain * ev_multiplier / shutter_multiply;
		imx258_update_exposure(handle, s_hdr_info.capture_shutter * shutter_multiply,0);
		imx258_write_gain(handle, gain);
	}
}

/*==============================================================================
 * Description:
 * decrease gain or shutter for hdr
 *
 *============================================================================*/
static void imx258_decrease_hdr_exposure(SENSOR_HW_HANDLE handle, uint8_t ev_divisor)
{
	uint16_t gain_multiply = 0;
	uint32_t shutter = 0;
	gain_multiply = s_hdr_info.capture_gain / SENSOR_BASE_GAIN;

	if (gain_multiply >= ev_divisor) {
		imx258_update_exposure(handle, s_hdr_info.capture_shutter,0);
		imx258_write_gain(handle, s_hdr_info.capture_gain / ev_divisor);

	} else {
		shutter = s_hdr_info.capture_shutter * gain_multiply / ev_divisor;
		imx258_update_exposure(handle, shutter,0);
		imx258_write_gain(handle, s_hdr_info.capture_gain / gain_multiply);
	}
}

/*==============================================================================
 * Description:
 * set hdr ev
 * you can change this function if it's necessary
 *============================================================================*/
static uint32_t imx258_set_hdr_ev(SENSOR_HW_HANDLE handle, unsigned long param)
{
	uint32_t ret = SENSOR_SUCCESS;
	SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR) param;

	uint32_t ev = ext_ptr->param;
	uint8_t ev_divisor, ev_multiplier;

	switch (ev) {
	case SENSOR_HDR_EV_LEVE_0:
		ev_divisor = 2;
		imx258_decrease_hdr_exposure(handle, ev_divisor);
		break;
	case SENSOR_HDR_EV_LEVE_1:
		ev_multiplier = 2;
		imx258_increase_hdr_exposure(handle, ev_multiplier);
		break;
	case SENSOR_HDR_EV_LEVE_2:
		ev_multiplier = 1;
		imx258_increase_hdr_exposure(handle, ev_multiplier);
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
static unsigned long imx258_ext_func(SENSOR_HW_HANDLE handle, unsigned long param)
{
	unsigned long rtn = SENSOR_SUCCESS;
	SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR) param;

	SENSOR_LOGI("ext_ptr->cmd: %d", ext_ptr->cmd);
	switch (ext_ptr->cmd) {
	case SENSOR_EXT_EV:
		rtn = imx258_set_hdr_ev(handle, param);
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
static unsigned long imx258_stream_on(SENSOR_HW_HANDLE handle, unsigned long param)
{
	SENSOR_LOGI("E");
	UNUSED(param);
	Sensor_WriteReg(0x0100, 0x01);
	/*delay*/
	//usleep(10 * 1000);

	return 0;
}

/*==============================================================================
 * Description:
 * mipi stream off
 * please modify this function acording your spec
 *============================================================================*/
static unsigned long imx258_stream_off(SENSOR_HW_HANDLE handle, unsigned long param)
{
	SENSOR_LOGI("E");
	UNUSED(param);
	Sensor_WriteReg(0x0100, 0x00);
	/*delay*/
	usleep(100 * 1000);

	return 0;
}

static unsigned long imx258_write_af(SENSOR_HW_HANDLE handle, unsigned long param)
{
#ifdef CONFIG_AF_VCM_DW9800W
	return vcm_dw9800_set_position(handle, param);
#else
	return bu64297gwz_write_af(handle, param);
#endif
}

static uint32_t imx258_get_static_info(SENSOR_HW_HANDLE handle, uint32_t *param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	struct sensor_ex_info *ex_info;
	uint32_t up = 0;
	uint32_t down = 0;
	//make sure we have get max fps of all settings.
	if(!s_imx258_mode_fps_info.is_init) {
		imx258_init_mode_fps_info(handle);
	}
	ex_info = (struct sensor_ex_info*)param;
	ex_info->f_num = s_imx258_static_info.f_num;
	ex_info->focal_length = s_imx258_static_info.focal_length;
	ex_info->max_fps = s_imx258_static_info.max_fps;
	ex_info->max_adgain = s_imx258_static_info.max_adgain;
	ex_info->ois_supported = s_imx258_static_info.ois_supported;
	ex_info->pdaf_supported = s_imx258_static_info.pdaf_supported;
	ex_info->exp_valid_frame_num = s_imx258_static_info.exp_valid_frame_num;
	ex_info->clamp_level = s_imx258_static_info.clamp_level;
	ex_info->adgain_valid_frame_num = s_imx258_static_info.adgain_valid_frame_num;
	ex_info->preview_skip_num = g_imx258_mipi_raw_info.preview_skip_num;
	ex_info->capture_skip_num = g_imx258_mipi_raw_info.capture_skip_num;
	ex_info->name = g_imx258_mipi_raw_info.name;
	ex_info->sensor_version_info = g_imx258_mipi_raw_info.sensor_version_info;
#ifdef CONFIG_AF_VCM_DW9800W
	vcm_dw9800_get_pose_dis(handle, &up, &down);
#else
	bu64297gwz_get_pose_dis(handle, &up, &down);
#endif
	ex_info->pos_dis.up2hori = up;
	ex_info->pos_dis.hori2down = down;
	SENSOR_LOGI("SENSOR_imx258: f_num: %d", ex_info->f_num);
	SENSOR_LOGI("SENSOR_imx258: max_fps: %d", ex_info->max_fps);
	SENSOR_LOGI("SENSOR_imx258: max_adgain: %d", ex_info->max_adgain);
	SENSOR_LOGI("SENSOR_imx258: ois_supported: %d", ex_info->ois_supported);
	SENSOR_LOGI("SENSOR_imx258: pdaf_supported: %d", ex_info->pdaf_supported);
	SENSOR_LOGI("SENSOR_imx258: exp_valid_frame_num: %d", ex_info->exp_valid_frame_num);
	SENSOR_LOGI("SENSOR_imx258: clam_level: %d", ex_info->clamp_level);
	SENSOR_LOGI("SENSOR_imx258: adgain_valid_frame_num: %d", ex_info->adgain_valid_frame_num);
	SENSOR_LOGI("SENSOR_imx258: sensor name is: %s", ex_info->name);
	SENSOR_LOGI("SENSOR_imx258: sensor version info is: %s", ex_info->sensor_version_info);

	return rtn;
}


static uint32_t imx258_get_fps_info(SENSOR_HW_HANDLE handle, uint32_t *param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_MODE_FPS_T *fps_info;
	//make sure have inited fps of every sensor mode.
	if(!s_imx258_mode_fps_info.is_init) {
		imx258_init_mode_fps_info(handle);
	}
	fps_info = (SENSOR_MODE_FPS_T*)param;
	uint32_t sensor_mode = fps_info->mode;
	fps_info->max_fps = s_imx258_mode_fps_info.sensor_mode_fps[sensor_mode].max_fps;
	fps_info->min_fps = s_imx258_mode_fps_info.sensor_mode_fps[sensor_mode].min_fps;
	fps_info->is_high_fps = s_imx258_mode_fps_info.sensor_mode_fps[sensor_mode].is_high_fps;
	fps_info->high_fps_skip_num = s_imx258_mode_fps_info.sensor_mode_fps[sensor_mode].high_fps_skip_num;
	SENSOR_LOGI("SENSOR_imx258: mode %d, max_fps: %d",fps_info->mode, fps_info->max_fps);
	SENSOR_LOGI("SENSOR_imx258: min_fps: %d", fps_info->min_fps);
	SENSOR_LOGI("SENSOR_imx258: is_high_fps: %d", fps_info->is_high_fps);
	SENSOR_LOGI("SENSOR_imx258: high_fps_skip_num: %d", fps_info->high_fps_skip_num);

	return rtn;
}

static unsigned long imx258_access_val(SENSOR_HW_HANDLE handle, unsigned long param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_VAL_T* param_ptr = (SENSOR_VAL_T*)param;
	uint16_t tmp;

	SENSOR_LOGI("SENSOR_imx258: _imx258_access_val E param_ptr = %p", param_ptr);
	if(!param_ptr){
		return rtn;
	}

	SENSOR_LOGI("SENSOR_imx258: param_ptr->type=%x", param_ptr->type);
	switch(param_ptr->type)
	{
		case SENSOR_VAL_TYPE_INIT_OTP:
			//imx258_otp_init(handle);
			break;
		case SENSOR_VAL_TYPE_SHUTTER:
			//*((uint32_t*)param_ptr->pval) = imx258_get_shutter();
			break;
		case SENSOR_VAL_TYPE_READ_VCM:
			//rtn = imx258_read_vcm(handle, param_ptr->pval);
			break;
		case SENSOR_VAL_TYPE_WRITE_VCM:
			//rtn = imx258_write_vcm(handle, param_ptr->pval);
			break;
		case SENSOR_VAL_TYPE_READ_OTP:
			//imx258_otp_read(handle,param_ptr);
			break;
		case SENSOR_VAL_TYPE_PARSE_OTP:
			//imx258_parse_otp(handle, param_ptr);
			break;
		case SENSOR_VAL_TYPE_WRITE_OTP:
			//rtn = _hi544_write_otp(handle, (uint32_t)param_ptr->pval);
			break;
		case SENSOR_VAL_TYPE_GET_RELOADINFO:
			{
//				struct isp_calibration_info **p= (struct isp_calibration_info **)param_ptr->pval;
//				*p=&calibration_info;
			}
			break;
		case SENSOR_VAL_TYPE_GET_AFPOSITION:
			//*(uint32_t*)param_ptr->pval = 0;//cur_af_pos;
			break;
		case SENSOR_VAL_TYPE_WRITE_OTP_GAIN:
			//rtn = imx258_write_otp_gain(handle, param_ptr->pval);
			break;
		case SENSOR_VAL_TYPE_READ_OTP_GAIN:
			//rtn = imx258_read_otp_gain(handle, param_ptr->pval);
			break;
		case SENSOR_VAL_TYPE_GET_STATIC_INFO:
			rtn = imx258_get_static_info(handle, param_ptr->pval);
			break;
		case SENSOR_VAL_TYPE_GET_FPS_INFO:
			rtn = imx258_get_fps_info(handle, param_ptr->pval);
			break;
		default:
			break;
	}

	SENSOR_LOGI("SENSOR_imx258: _imx258_access_val X");

	return rtn;
}

uint32_t dw9800_set_pos(SENSOR_HW_HANDLE handle, uint16_t pos){
	uint8_t cmd_val[2] ={0};

	// set pos
	cmd_val[0] = 0x03;
	cmd_val[1] = (pos>>8)&0x03;
	Sensor_WriteI2C(DW9800_VCM_SLAVE_ADDR,(uint8_t*)&cmd_val[0],2);

	cmd_val[0] = 0x04;
	cmd_val[1] = pos&0xff;
	Sensor_WriteI2C(DW9800_VCM_SLAVE_ADDR,(uint8_t*)&cmd_val[0],2);
	SENSOR_LOGI("set_position:0x%x",pos);
	return 0;
}

uint32_t dw900_get_otp(SENSOR_HW_HANDLE handle,uint16_t *inf,uint16_t *macro){
	uint8_t bTransfer[2] = {0,0};

	// get otp
	bTransfer[0] = 0x00;
	bTransfer[1] = 0x00;
	Sensor_WriteI2C(DW9800_VCM_SLAVE_ADDR, &bTransfer[0], 2);
	usleep(1000);
	//Macro
	bTransfer[0] = 0x01;
	bTransfer[1] = 0x0c;
	Sensor_ReadI2C(DW9800_VCM_SLAVE_ADDR, &bTransfer[0], 2);
	*macro = bTransfer[0];
	bTransfer[0] = 0x01;
	bTransfer[1] = 0x0d;
	Sensor_ReadI2C(DW9800_VCM_SLAVE_ADDR, &bTransfer[0], 2);
	*macro += bTransfer[0]<<8;
	//Infi
	bTransfer[0] = 0x01;
	bTransfer[1] = 0x10;
	Sensor_ReadI2C(DW9800_VCM_SLAVE_ADDR, &bTransfer[0], 2);
	*inf = bTransfer[0];
	bTransfer[0] = 0x01;
	bTransfer[1] = 0x11;
	Sensor_ReadI2C(DW9800_VCM_SLAVE_ADDR, &bTransfer[0], 2);
	*inf += bTransfer[0]<<8;
	return 0;
}

uint32_t dw9800_get_motor_pos(SENSOR_HW_HANDLE handle,uint16_t *pos){
	uint8_t cmd_val[2];
	// read
	SENSOR_LOGI("handle:0x%x",handle);
	cmd_val[0] = 0x03;
	Sensor_ReadI2C(DW9800_VCM_SLAVE_ADDR,(uint8_t*)&cmd_val[0],1);
	*pos = (cmd_val[0] & 0x03)<<8;
	usleep(200);
	cmd_val[0] = 0x04;
	Sensor_ReadI2C(DW9800_VCM_SLAVE_ADDR,(uint8_t*)&cmd_val[0],1);
	*pos += cmd_val[0];
	SENSOR_LOGI("get_position:0x%x",*pos);
	return 0;
}

uint32_t dw9800_set_motor_bestmode(SENSOR_HW_HANDLE handle){

	uint8_t ctrl,mode,freq;
	uint8_t pos1,pos2;
	uint8_t cmd_val[2];

	//set
	cmd_val[0] = 0x02;
	cmd_val[1] = 0x2;
	Sensor_WriteI2C(DW9800_VCM_SLAVE_ADDR,(uint8_t*)&cmd_val[0],2);
	usleep(200);
	cmd_val[0] = 0x06;
	cmd_val[1] = 0x80;//61;
	Sensor_WriteI2C(DW9800_VCM_SLAVE_ADDR,(uint8_t*)&cmd_val[0],2);
	usleep(200);
	cmd_val[0] = 0x07;
	cmd_val[1] = 0x75;//38;
	Sensor_WriteI2C(DW9800_VCM_SLAVE_ADDR,(uint8_t*)&cmd_val[0],2);
	usleep(200*1000);

	CMR_LOGI("VCM ctrl mode freq pos 2nd,%d %d %d %d",ctrl,mode,freq,(pos1<<8)+pos2);
	return 0;
}

uint32_t dw9800_get_test_vcm_mode(SENSOR_HW_HANDLE handle){

	uint8_t ctrl,mode,freq;
	uint8_t pos1,pos2;
	uint8_t cmd_val[2];

	FILE* fp = NULL;
	fp = fopen("/data/misc/cameraserver/cur_vcm_info.txt","wb");
	// read
	cmd_val[0] = 0x02;
	cmd_val[1] = 0x02;
	Sensor_ReadI2C(DW9800_VCM_SLAVE_ADDR,(uint8_t*)&cmd_val[0],1);
	ctrl = cmd_val[0];
	usleep(200);
	cmd_val[0] = 0x06;
	cmd_val[1] = 0x06;
	Sensor_ReadI2C(DW9800_VCM_SLAVE_ADDR,(uint8_t*)&cmd_val[0],1);
	mode = cmd_val[0];
	usleep(200);
	cmd_val[0] = 0x07;
	cmd_val[1] = 0x07;
	Sensor_ReadI2C(DW9800_VCM_SLAVE_ADDR,(uint8_t*)&cmd_val[0],1);
	freq = cmd_val[0];

	// read
	cmd_val[0] = 0x03;
	cmd_val[1] = 0x03;
	Sensor_ReadI2C(DW9800_VCM_SLAVE_ADDR,(uint8_t*)&cmd_val[0],1);
	pos1 = cmd_val[0];
	usleep(200);
	cmd_val[0] = 0x04;
	cmd_val[1] = 0x04;
	Sensor_ReadI2C(DW9800_VCM_SLAVE_ADDR,(uint8_t*)&cmd_val[0],1);
	pos2 = cmd_val[0];

	fprintf(fp,"VCM ctrl mode freq pos ,%d %d %d %d",ctrl,mode,freq,(pos1<<8)+pos2);
	fclose(fp);
	return 0;
}

uint32_t dw9800_set_test_vcm_mode(SENSOR_HW_HANDLE handle,char* vcm_mode){

	uint8_t ctrl,mode,freq;
	uint8_t pos1,pos2;
	uint8_t cmd_val[2];
	char* p1=vcm_mode;

	while( *p1!='~'  && *p1!='\0' )
		p1++;
	*p1++ = '\0';
	ctrl = atoi(vcm_mode);
	vcm_mode = p1;
	while( *p1!='~'  && *p1!='\0' )
		p1++;
	*p1++ = '\0';
	mode = atoi(vcm_mode);
	vcm_mode = p1;
	while( *p1!='~'  && *p1!='\0' )
		p1++;
	*p1++ = '\0';
	freq = atoi(vcm_mode);
	CMR_LOGI("VCM ctrl mode freq pos 1nd,%d %d %d",ctrl,mode,freq);
	//set
	cmd_val[0] = 0x02;
	cmd_val[1] = ctrl;
	Sensor_WriteI2C(DW9800_VCM_SLAVE_ADDR,(uint8_t*)&cmd_val[0],2);
	usleep(200);
	cmd_val[0] = 0x06;
	cmd_val[1] = mode;
	Sensor_WriteI2C(DW9800_VCM_SLAVE_ADDR,(uint8_t*)&cmd_val[0],2);
	usleep(200);
	cmd_val[0] = 0x07;
	cmd_val[1] = freq;
	Sensor_WriteI2C(DW9800_VCM_SLAVE_ADDR,(uint8_t*)&cmd_val[0],2);
	usleep(200*1000);
	// read
	cmd_val[0] = 0x02;
	cmd_val[1] = 0x02;
	Sensor_ReadI2C(DW9800_VCM_SLAVE_ADDR,(uint8_t*)&cmd_val[0],1);
	ctrl = cmd_val[0];
	usleep(200);
	cmd_val[0] = 0x06;
	cmd_val[1] = 0x06;
	Sensor_ReadI2C(DW9800_VCM_SLAVE_ADDR,(uint8_t*)&cmd_val[0],1);
	mode = cmd_val[0];
	usleep(200);
	cmd_val[0] = 0x07;
	cmd_val[1] = 0x07;
	Sensor_ReadI2C(DW9800_VCM_SLAVE_ADDR,(uint8_t*)&cmd_val[0],1);
	freq = cmd_val[0];
	CMR_LOGI("VCM ctrl mode freq pos 2nd,%d %d %d",ctrl,mode,freq);
	return 0;
}

/*==============================================================================
 * Description:
 * all ioctl functoins
 * you can add functions reference SENSOR_IOCTL_FUNC_TAB_T from sensor_drv_u.h
 *
 * add ioctl functions like this:
 * .power = imx132_power_on,
 *============================================================================*/
static SENSOR_IOCTL_FUNC_TAB_T s_imx258_ioctl_func_tab = {
	.power = imx258_power_on,
	.identify = imx258_identify,
	.get_trim = imx258_get_resolution_trim_tab,
	.before_snapshort = imx258_before_snapshot,
	.ex_write_exp = imx258_ex_write_exposure,
	.write_gain_value = imx258_write_gain_value,
	//.set_focus = imx258_ext_func,
	//.set_video_mode = imx132_set_video_mode,
	.stream_on = imx258_stream_on,
	.stream_off = imx258_stream_off,
	.af_enable = imx258_write_af,
	//.group_hold_on = imx132_group_hold_on,
	//.group_hold_of = imx132_group_hold_off,
	.cfg_otp = imx258_access_val,
//	.set_motor_bestmode = dw9800_set_motor_bestmode,// set vcm best mode and avoid damping
#if defined(CONFIG_CAMERA_ISP_DIR_2_1)
	.set_pos = dw9800_set_pos,// set vcm pos
	.get_otp = PNULL,
	.get_motor_pos = dw9800_get_motor_pos,// get vcm pos in register
	.set_motor_bestmode = dw9800_set_motor_bestmode,// set vcm best mode and avoid damping
	.get_test_vcm_mode = dw9800_get_test_vcm_mode,// test whether vcm mode valid in register
	.set_test_vcm_mode = dw9800_set_test_vcm_mode,// set vcm mode and test best mode for damping
	//.set_shutter_gain_delay_info = imx258_set_shutter_gain_delay_info,
#endif
};
