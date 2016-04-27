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

#if defined(CONFIG_CAMERA_ISP_VERSION_V3) || defined(CONFIG_CAMERA_ISP_VERSION_V4)
#include "sensor_imx132_raw_param_v3.c"
#else
#include "sensor_imx132_raw_param.c"
#endif


#define SENSOR_NAME			"imx132_mipi_raw"
#define I2C_SLAVE_ADDR			0x6c    /* 8bit slave address*/

#define imx132_PID_ADDR			0x0000
#define imx132_PID_VALUE			0x01
#define imx132_VER_ADDR			0x0001
#define imx132_VER_VALUE			0x32

/* sensor parameters begin */
/* effective sensor output image size */
#define SNAPSHOT_WIDTH			1920
#define SNAPSHOT_HEIGHT			1088
#define PREVIEW_WIDTH			1920
#define PREVIEW_HEIGHT			1088

/*Mipi output*/
#define LANE_NUM			2
#define RAW_BITS				10

#define SNAPSHOT_MIPI_PER_LANE_BPS	600
#define PREVIEW_MIPI_PER_LANE_BPS	600

/*line time unit: 0.1us*/
#define SNAPSHOT_LINE_TIME		18700
#define PREVIEW_LINE_TIME		18700

/* frame length*/
#define SNAPSHOT_FRAME_LENGTH		1780
#define PREVIEW_FRAME_LENGTH		1780

/* please ref your spec */
#define FRAME_OFFSET			5
#define SENSOR_MAX_GAIN			0xF0
#define SENSOR_BASE_GAIN		0x20
#define SENSOR_MIN_SHUTTER		4

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

struct hdr_info_t {
	uint32_t capture_max_shutter;
	uint32_t capture_shutter;
	uint32_t capture_gain;
};

struct sensor_ev_info_t {
	uint16_t preview_shutter;
	uint16_t preview_gain;
};

/*==============================================================================
 * Description:
 * global variable
 *============================================================================*/
static struct hdr_info_t s_hdr_info;
static uint32_t s_current_default_frame_length;
struct sensor_ev_info_t s_sensor_ev_info;

static SENSOR_IOCTL_FUNC_TAB_T s_imx132_ioctl_func_tab;
struct sensor_raw_info *s_imx132_mipi_raw_info_ptr = &s_imx132_mipi_raw_info;

static const SENSOR_REG_T imx132_init_setting[] = {
	{0x0100,0x00},
	{0x3087,0x53},
	{0x308B,0x5A},
	{0x3094,0x11},
	{0x309D,0xA4},
	{0x30AA,0x01},
	{0x30C6,0x00},
	{0x30C7,0x00},
	{0x3118,0x2F},
	{0x312A,0x00},
	{0x312B,0x0B},
	{0x312C,0x0B},
	{0x312D,0x13},
	{0x3032,0x40},
	{0x0305,0x02},
	{0x0307,0x32},
	{0x30A4,0x02},
	{0x303C,0x4B},
	{0x0340,0x06},
	{0x0341,0xF4},
	{0x0342,0x08},
	{0x0343,0xC8},
	{0x0344,0x00},
	{0x0345,0x1C},
	{0x0346,0x00},
	{0x0347,0x38},
	{0x0348,0x07},
	{0x0349,0x9B},
	{0x034A,0x04},
	{0x034B,0x6F},
	{0x034C,0x07},
	{0x034D,0x80},
	{0x034E,0x04},
	{0x034F,0x40},
	{0x0381,0x01},
	{0x0383,0x01},
	{0x0385,0x01},
	{0x0387,0x01},
	{0x303D,0x10},
	{0x303E,0x4A},
	{0x3040,0x08},
	{0x3041,0x97},
	{0x3048,0x00},
	{0x304C,0x2F},
	{0x304D,0x02},
	{0x3064,0x92},
	{0x306A,0x10},
	{0x309B,0x00},
	{0x309E,0x41},
	{0x30A0,0x10},
	{0x30A1,0x0B},
	{0x30B2,0x07},
	{0x30D5,0x00},
	{0x30D6,0x00},
	{0x30D7,0x00},
	{0x30D8,0x00},
	{0x30D9,0x00},
	{0x30DA,0x00},
	{0x30DB,0x00},
	{0x30DC,0x00},
	{0x30DD,0x00},
	{0x30DE,0x00},
	{0x3102,0x0C},
	{0x3103,0x33},
	{0x3104,0x30},
	{0x3105,0x00},
	{0x3106,0xCA},
	{0x3107,0x00},
	{0x3108,0x06},
	{0x3109,0x04},
	{0x310A,0x04},
	{0x315C,0x3D},
	{0x315D,0x3C},
	{0x316E,0x3E},
	{0x316F,0x3D},
	{0x3301,0x00},
	{0x3304,0x07},
	{0x3305,0x06},
	{0x3306,0x19},
	{0x3307,0x03},
	{0x3308,0x0F},
	{0x3309,0x07},
	{0x330A,0x0C},
	{0x330B,0x06},
	{0x330C,0x0B},
	{0x330D,0x07},
	{0x330E,0x03},
	{0x3318,0x63},
	{0x3322,0x09},
	{0x3342,0x00},
	{0x3348,0xE0},
};

static const SENSOR_REG_T imx132_preview_setting[] = {
};

static const SENSOR_REG_T imx132_snapshot_setting[] = {
};

static SENSOR_REG_TAB_INFO_T s_imx132_resolution_tab_raw[] = {
	{ADDR_AND_LEN_OF_ARRAY(imx132_init_setting), 0, 0, EX_MCLK,
	 SENSOR_IMAGE_FORMAT_RAW},
	/*{ADDR_AND_LEN_OF_ARRAY(imx132_preview_setting),
	 PREVIEW_WIDTH, PREVIEW_HEIGHT, EX_MCLK,
	 SENSOR_IMAGE_FORMAT_RAW},*/
	{ADDR_AND_LEN_OF_ARRAY(imx132_snapshot_setting),
	 SNAPSHOT_WIDTH, SNAPSHOT_HEIGHT, EX_MCLK,
	 SENSOR_IMAGE_FORMAT_RAW},
};

static SENSOR_TRIM_T s_imx132_resolution_trim_tab[] = {
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	/*{0, 0, PREVIEW_WIDTH, PREVIEW_HEIGHT,
	 PREVIEW_LINE_TIME, PREVIEW_MIPI_PER_LANE_BPS, PREVIEW_FRAME_LENGTH,
	 {0, 0, PREVIEW_WIDTH, PREVIEW_HEIGHT}},*/
	{0, 0, SNAPSHOT_WIDTH, SNAPSHOT_HEIGHT,
	 SNAPSHOT_LINE_TIME, SNAPSHOT_MIPI_PER_LANE_BPS, SNAPSHOT_FRAME_LENGTH,
	 {0, 0, SNAPSHOT_WIDTH, SNAPSHOT_HEIGHT}},
};

static const SENSOR_REG_T s_imx132_preview_size_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
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

static const SENSOR_REG_T s_imx132_capture_size_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
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

static SENSOR_VIDEO_INFO_T s_imx132_video_info[SENSOR_MODE_MAX] = {
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{30, 30, 270, 90}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
	 (SENSOR_REG_T **) s_imx132_preview_size_video_tab},
	{{{2, 5, 338, 1000}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
	 (SENSOR_REG_T **) s_imx132_capture_size_video_tab},
};

/*==============================================================================
 * Description:
 * set video mode
 *
 *============================================================================*/
static uint32_t imx132_set_video_mode(SENSOR_HW_HANDLE handle, uint32_t param)
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

	if (PNULL == s_imx132_video_info[mode].setting_ptr) {
		SENSOR_PRINT("fail.");
		return SENSOR_FAIL;
	}

	sensor_reg_ptr = (SENSOR_REG_T_PTR) & s_imx132_video_info[mode].setting_ptr[param];
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

static SENSOR_STATIC_INFO_T s_imx132_static_info = {
   200,    //f-number,focal ratio
   357,    //focal_length;
   0,  //max_fps,max fps of sensor's all settings,it will be calculated from sensor mode fps
   16, //max_adgain,AD-gain
   0,  //ois_supported;
   0,  //pdaf_supported;
   1,  //exp_valid_frame_num;N+2-1
   64, //clamp_level,black level
   1,  //adgain_valid_frame_num;N+1-1
};

static SENSOR_MODE_FPS_INFO_T s_imx132_mode_fps_info = {
   0,  //is_init;
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

static uint32_t imx132_init_mode_fps_info(SENSOR_HW_HANDLE handle)
{
   uint32_t rtn = SENSOR_SUCCESS;
   SENSOR_PRINT("imx132_init_mode_fps_info:E");
   if(!s_imx132_mode_fps_info.is_init) {
       uint32_t i,modn,tempfps = 0;
   SENSOR_PRINT("imx132_init_mode_fps_info:start init");
   for(i = 0;i < NUMBER_OF_ARRAY(s_imx132_resolution_trim_tab); i++) {
    //max fps should be multiple of 30,it calulated from line_time and frame_line
   tempfps = s_imx132_resolution_trim_tab[i].line_time * s_imx132_resolution_trim_tab[i].frame_line;
   if(0 != tempfps) {
   tempfps = 1000000000/tempfps;
   modn = tempfps / 30;
   if(tempfps > modn*30)
     modn++;
   s_imx132_mode_fps_info.sensor_mode_fps[i].max_fps = modn*30;
   if(s_imx132_mode_fps_info.sensor_mode_fps[i].max_fps > 30) {
   s_imx132_mode_fps_info.sensor_mode_fps[i].is_high_fps = 1;
   s_imx132_mode_fps_info.sensor_mode_fps[i].high_fps_skip_num =
   s_imx132_mode_fps_info.sensor_mode_fps[i].max_fps/30;
   }
   if(s_imx132_mode_fps_info.sensor_mode_fps[i].max_fps >
   s_imx132_static_info.max_fps) {
   s_imx132_static_info.max_fps = s_imx132_mode_fps_info.sensor_mode_fps[i].max_fps;
	}

   }
    SENSOR_PRINT("mode %d,tempfps %d,frame_len %d,line_time: %d ",i,tempfps,
    s_imx132_resolution_trim_tab[i].frame_line,
    s_imx132_resolution_trim_tab[i].line_time);
    SENSOR_PRINT("mode %d,max_fps: %d ",i,s_imx132_mode_fps_info.sensor_mode_fps[i].max_fps);
	SENSOR_PRINT("is_high_fps: %d,highfps_skip_num %d",
			s_imx132_mode_fps_info.sensor_mode_fps[i].is_high_fps,
          s_imx132_mode_fps_info.sensor_mode_fps[i].high_fps_skip_num);
     }
      s_imx132_mode_fps_info.is_init = 1;
   }
   SENSOR_PRINT("_imx132_init_mode_fps_info:X");
   return rtn;
}
/*==============================================================================
 * Description:
 * sensor all info
 * please modify this variable acording your spec
 *============================================================================*/
SENSOR_INFO_T g_imx132_mipi_raw_info = {
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
	{{imx132_PID_ADDR, imx132_PID_VALUE}
	 ,
	 {imx132_VER_ADDR, imx132_VER_VALUE}
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
	SENSOR_IMAGE_PATTERN_RAWRGB_R,
	/* point to resolution table information structure */
	s_imx132_resolution_tab_raw,
	/* point to ioctl function table */
	&s_imx132_ioctl_func_tab,
	/* information and table about Rawrgb sensor */
	&s_imx132_mipi_raw_info_ptr,
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
	35,
	/* vertical view angle*/
	35
};



/*==============================================================================
 * Description:
 * get default frame length
 *
 *============================================================================*/
static uint32_t imx132_get_default_frame_length(SENSOR_HW_HANDLE handle, uint32_t mode)
{
	return s_imx132_resolution_trim_tab[mode].frame_line;
}

/*==============================================================================
 * Description:
 * write group-hold on to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void imx132_group_hold_on(SENSOR_HW_HANDLE handle)
{
	SENSOR_PRINT("E");

	//Sensor_WriteReg(0x0104, 0x01);
}

/*==============================================================================
 * Description:
 * write group-hold off to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void imx132_group_hold_off(SENSOR_HW_HANDLE handle)
{
	SENSOR_PRINT("E");

	//Sensor_WriteReg(0x0104, 0x00);
}


/*==============================================================================
 * Description:
 * read gain from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static uint16_t imx132_read_gain(SENSOR_HW_HANDLE handle)
{
	uint16_t gain_l = 0;

	gain_l = Sensor_ReadReg(0x0205);

	return gain_l;
}

/*==============================================================================
 * Description:
 * write gain to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void imx132_write_gain(SENSOR_HW_HANDLE handle, uint32_t gain)
{
	uint32_t sensor_again = 0;

	sensor_again=256-(256*SENSOR_BASE_GAIN/gain);
	sensor_again=sensor_again&0xFF;

	if (SENSOR_MAX_GAIN < sensor_again)
			sensor_again = SENSOR_MAX_GAIN;
	SENSOR_PRINT("sensor_again=0x%x",sensor_again);
	Sensor_WriteReg(0x0205, sensor_again);

	imx132_group_hold_off(handle);

}

/*==============================================================================
 * Description:
 * read frame length from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static uint16_t imx132_read_frame_length(SENSOR_HW_HANDLE handle)
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
static void imx132_write_frame_length(SENSOR_HW_HANDLE handle, uint32_t frame_len)
{
	Sensor_WriteReg(0x0340, (frame_len >> 8) & 0xff);
	Sensor_WriteReg(0x0341, frame_len & 0xff);
}

/*==============================================================================
 * Description:
 * read shutter from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t imx132_read_shutter(SENSOR_HW_HANDLE handle)
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
static void imx132_write_shutter(SENSOR_HW_HANDLE handle, uint32_t shutter)
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
static uint16_t imx132_update_exposure(SENSOR_HW_HANDLE handle, uint32_t shutter,uint32_t dummy_line)
{
	uint32_t dest_fr_len = 0;
	uint32_t cur_fr_len = 0;
	uint32_t fr_len = s_current_default_frame_length;

	imx132_group_hold_on(handle);

	if (1 == SUPPORT_AUTO_FRAME_LENGTH)
		goto write_sensor_shutter;

	dest_fr_len = ((shutter + dummy_line+FRAME_OFFSET) > fr_len) ? (shutter +dummy_line+ FRAME_OFFSET) : fr_len;

	cur_fr_len = imx132_read_frame_length(handle);

	if (shutter < SENSOR_MIN_SHUTTER)
		shutter = SENSOR_MIN_SHUTTER;

	if (dest_fr_len != cur_fr_len)
		imx132_write_frame_length(handle, dest_fr_len);
write_sensor_shutter:
	/* write shutter to sensor registers */
	imx132_write_shutter(handle, shutter);
	return shutter;
}

/*==============================================================================
 * Description:
 * sensor power on
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t imx132_power_on(SENSOR_HW_HANDLE handle, uint32_t power_on)
{
	SENSOR_AVDD_VAL_E dvdd_val = g_imx132_mipi_raw_info.dvdd_val;
	SENSOR_AVDD_VAL_E avdd_val = g_imx132_mipi_raw_info.avdd_val;
	SENSOR_AVDD_VAL_E iovdd_val = g_imx132_mipi_raw_info.iovdd_val;
	BOOLEAN power_down = g_imx132_mipi_raw_info.power_down_level;
	BOOLEAN reset_level = g_imx132_mipi_raw_info.reset_pulse_level;

	if (SENSOR_TRUE == power_on) {
		Sensor_PowerDown(power_down);
		Sensor_SetResetLevel(reset_level);
		usleep(2* 1000);
		Sensor_SetVoltage(dvdd_val, avdd_val, iovdd_val);
		usleep(2 * 1000);
		Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);
		usleep(2 * 1000);
		//Sensor_SetMIPILevel(1);
		Sensor_PowerDown(!power_down);
		Sensor_SetResetLevel(!reset_level);

	} else {

		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		Sensor_SetVoltage(SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED);
		Sensor_Reset(reset_level);
		Sensor_PowerDown(power_down);
	}
	SENSOR_PRINT("(1:on, 0:off): %d", power_on);
	return SENSOR_SUCCESS;
}


/*==============================================================================
 * Description:
 * identify sensor id
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t imx132_identify(SENSOR_HW_HANDLE handle, uint32_t param)
{
	uint8_t pid_value = 0x00;
	uint8_t ver_value = 0x00;
	uint32_t ret_value = SENSOR_FAIL;

	SENSOR_PRINT("mipi raw identify");

	pid_value = Sensor_ReadReg(imx132_PID_ADDR);
	ver_value = Sensor_ReadReg(imx132_VER_ADDR);

	if (imx132_PID_VALUE == pid_value) {
		ver_value = Sensor_ReadReg(imx132_VER_ADDR);
		SENSOR_PRINT("Identify: PID = %x, VER = %x", pid_value, ver_value);
		if (imx132_VER_VALUE == ver_value) {
			ret_value = SENSOR_SUCCESS;
			SENSOR_PRINT_HIGH("this is imx132 sensor");
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
static unsigned long imx132_get_resolution_trim_tab(SENSOR_HW_HANDLE handle, uint32_t param)
{
	return (unsigned long) s_imx132_resolution_trim_tab;
}

/*==============================================================================
 * Description:
 * before snapshot
 * you can change this function if it's necessary
 *============================================================================*/
static uint32_t imx132_before_snapshot(SENSOR_HW_HANDLE handle, uint32_t param)
{
	uint32_t cap_shutter = 0;
	uint32_t prv_shutter = 0;
	uint32_t gain = 0;
	uint32_t cap_gain = 0;
	uint32_t capture_mode = param & 0xffff;
	uint32_t preview_mode = (param >> 0x10) & 0xffff;

	uint32_t prv_linetime = s_imx132_resolution_trim_tab[preview_mode].line_time;
	uint32_t cap_linetime = s_imx132_resolution_trim_tab[capture_mode].line_time;

	s_current_default_frame_length = imx132_get_default_frame_length(handle, capture_mode);
	SENSOR_PRINT("capture_mode = %d", capture_mode);

	if (preview_mode == capture_mode) {
		cap_shutter = s_sensor_ev_info.preview_shutter;
		cap_gain = s_sensor_ev_info.preview_gain;
		goto snapshot_info;
	}

	prv_shutter = s_sensor_ev_info.preview_shutter;	//imx132_read_shutter();
	gain = s_sensor_ev_info.preview_gain;	//imx132_read_gain();

	Sensor_SetMode(capture_mode);
	Sensor_SetMode_WaitDone();

	cap_shutter = prv_shutter * prv_linetime / cap_linetime * BINNING_FACTOR;

	while (gain >= (2 * SENSOR_BASE_GAIN)) {
		if (cap_shutter * 2 > s_current_default_frame_length)
			break;
		cap_shutter = cap_shutter * 2;
		gain = gain / 2;
	}

	cap_shutter = imx132_update_exposure(handle, cap_shutter,0);
	cap_gain = gain;
	imx132_write_gain(handle, cap_gain);
	SENSOR_PRINT("preview_shutter = 0x%x, preview_gain = 0x%x",
		     s_sensor_ev_info.preview_shutter, s_sensor_ev_info.preview_gain);

	SENSOR_PRINT("capture_shutter = 0x%x, capture_gain = 0x%x", cap_shutter, cap_gain);
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
static uint32_t imx132_write_exposure(SENSOR_HW_HANDLE handle, uint32_t param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t exposure_line = 0x00;
	uint16_t dummy_line = 0x00;
	uint16_t mode = 0x00;

	exposure_line = param & 0xffff;
	dummy_line = (param >> 0x10) & 0xfff; /*for cits frame rate test*/
	mode = (param >> 0x1c) & 0x0f;

	SENSOR_PRINT("current mode = %d, exposure_line = %d, dummy_line=%d", mode, exposure_line,dummy_line);
	s_current_default_frame_length = imx132_get_default_frame_length(handle, mode);

	s_sensor_ev_info.preview_shutter = imx132_update_exposure(handle, exposure_line,dummy_line);

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
static uint32_t imx132_write_gain_value(SENSOR_HW_HANDLE handle, uint32_t param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint32_t real_gain = 0;

	real_gain = isp_to_real_gain(handle, param);

	real_gain = real_gain * SENSOR_BASE_GAIN / ISP_BASE_GAIN;

	SENSOR_PRINT("real_gain = 0x%x", real_gain);

	s_sensor_ev_info.preview_gain = real_gain;
	imx132_write_gain(handle, real_gain);

	return ret_value;
}

/*==============================================================================
 * Description:
 * increase gain or shutter for hdr
 *
 *============================================================================*/
static void imx132_increase_hdr_exposure(SENSOR_HW_HANDLE handle, uint8_t ev_multiplier)
{
	uint32_t shutter_multiply = s_hdr_info.capture_max_shutter / s_hdr_info.capture_shutter;
	uint32_t gain = 0;

	if (0 == shutter_multiply)
		shutter_multiply = 1;

	if (shutter_multiply >= ev_multiplier) {
		imx132_update_exposure(handle, s_hdr_info.capture_shutter * ev_multiplier,0);
		imx132_write_gain(handle, s_hdr_info.capture_gain);
	} else {
		gain = s_hdr_info.capture_gain * ev_multiplier / shutter_multiply;
		imx132_update_exposure(handle, s_hdr_info.capture_shutter * shutter_multiply,0);
		imx132_write_gain(handle, gain);
	}
}

/*==============================================================================
 * Description:
 * decrease gain or shutter for hdr
 *
 *============================================================================*/
static void imx132_decrease_hdr_exposure(SENSOR_HW_HANDLE handle, uint8_t ev_divisor)
{
	uint16_t gain_multiply = 0;
	uint32_t shutter = 0;
	gain_multiply = s_hdr_info.capture_gain / SENSOR_BASE_GAIN;

	if (gain_multiply >= ev_divisor) {
		imx132_update_exposure(handle, s_hdr_info.capture_shutter,0);
		imx132_write_gain(handle, s_hdr_info.capture_gain / ev_divisor);

	} else {
		shutter = s_hdr_info.capture_shutter * gain_multiply / ev_divisor;
		imx132_update_exposure(handle, shutter,0);
		imx132_write_gain(handle, s_hdr_info.capture_gain / gain_multiply);
	}
}

/*==============================================================================
 * Description:
 * set hdr ev
 * you can change this function if it's necessary
 *============================================================================*/
static uint32_t imx132_set_hdr_ev(SENSOR_HW_HANDLE handle, unsigned long param)
{
	uint32_t ret = SENSOR_SUCCESS;
	SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR) param;

	uint32_t ev = ext_ptr->param;
	uint8_t ev_divisor, ev_multiplier;

	switch (ev) {
	case SENSOR_HDR_EV_LEVE_0:
		ev_divisor = 2;
		imx132_decrease_hdr_exposure(handle, ev_divisor);
		break;
	case SENSOR_HDR_EV_LEVE_1:
		ev_multiplier = 2;
		imx132_increase_hdr_exposure(handle, ev_multiplier);
		break;
	case SENSOR_HDR_EV_LEVE_2:
		ev_multiplier = 1;
		imx132_increase_hdr_exposure(handle, ev_multiplier);
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
static uint32_t imx132_ext_func(SENSOR_HW_HANDLE handle, unsigned long param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR) param;

	SENSOR_PRINT("ext_ptr->cmd: %d", ext_ptr->cmd);
	switch (ext_ptr->cmd) {
	case SENSOR_EXT_EV:
		rtn = imx132_set_hdr_ev(handle, param);
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
static uint32_t imx132_stream_on(SENSOR_HW_HANDLE handle, uint32_t param)
{
	SENSOR_PRINT("E");

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
static uint32_t imx132_stream_off(SENSOR_HW_HANDLE handle, uint32_t param)
{
	SENSOR_PRINT("E");

	Sensor_WriteReg(0x0100, 0x00);
	/*delay*/
	usleep(10 * 1000);

	return 0;
}

static uint32_t imx132_get_static_info(SENSOR_HW_HANDLE handle, uint32_t *param)
{
   uint32_t rtn = SENSOR_SUCCESS;
   struct sensor_ex_info *ex_info;
   //make sure we have get max fps of all settings.
   if(!s_imx132_mode_fps_info.is_init) {
      imx132_init_mode_fps_info(handle);
   }
   ex_info = (struct sensor_ex_info*)param;
   ex_info->f_num = s_imx132_static_info.f_num;
   ex_info->focal_length = s_imx132_static_info.focal_length;
   ex_info->max_fps = s_imx132_static_info.max_fps;
   ex_info->max_adgain = s_imx132_static_info.max_adgain;
   ex_info->ois_supported = s_imx132_static_info.ois_supported;
   ex_info->pdaf_supported = s_imx132_static_info.pdaf_supported;
   ex_info->exp_valid_frame_num = s_imx132_static_info.exp_valid_frame_num;
   ex_info->clamp_level = s_imx132_static_info.clamp_level;
   ex_info->adgain_valid_frame_num = s_imx132_static_info.adgain_valid_frame_num;
   ex_info->preview_skip_num = g_imx132_mipi_raw_info.preview_skip_num;
   ex_info->capture_skip_num = g_imx132_mipi_raw_info.capture_skip_num;
   ex_info->name = g_imx132_mipi_raw_info.name;
   ex_info->sensor_version_info = g_imx132_mipi_raw_info.sensor_version_info;
   SENSOR_PRINT("SENSOR_imx132: f_num: %d", ex_info->f_num);
   SENSOR_PRINT("SENSOR_imx132: max_fps: %d", ex_info->max_fps);
   SENSOR_PRINT("SENSOR_imx132: max_adgain: %d", ex_info->max_adgain);
   SENSOR_PRINT("SENSOR_imx132: ois_supported: %d", ex_info->ois_supported);
   SENSOR_PRINT("SENSOR_imx132: pdaf_supported: %d", ex_info->pdaf_supported);
   SENSOR_PRINT("SENSOR_imx132: exp_valid_frame_num: %d", ex_info->exp_valid_frame_num);
   SENSOR_PRINT("SENSOR_imx132: clam_level: %d", ex_info->clamp_level);
   SENSOR_PRINT("SENSOR_imx132: adgain_valid_frame_num: %d", ex_info->adgain_valid_frame_num);
   SENSOR_PRINT("SENSOR_imx132: sensor name is: %s", ex_info->name);
   SENSOR_PRINT("SENSOR_imx132: sensor version info is: %s", ex_info->sensor_version_info);

   return rtn;
}

static uint32_t imx132_get_fps_info(SENSOR_HW_HANDLE handle, uint32_t *param)
{
   uint32_t rtn = SENSOR_SUCCESS;
   SENSOR_MODE_FPS_T *fps_info;
   //make sure have inited fps of every sensor mode.
   if(!s_imx132_mode_fps_info.is_init) {
    imx132_init_mode_fps_info(handle);
   }
   fps_info = (SENSOR_MODE_FPS_T*)param;
   uint32_t sensor_mode = fps_info->mode;
   fps_info->max_fps = s_imx132_mode_fps_info.sensor_mode_fps[sensor_mode].max_fps;
   fps_info->min_fps = s_imx132_mode_fps_info.sensor_mode_fps[sensor_mode].min_fps;
   fps_info->is_high_fps = s_imx132_mode_fps_info.sensor_mode_fps[sensor_mode].is_high_fps;
   fps_info->high_fps_skip_num = s_imx132_mode_fps_info.sensor_mode_fps[sensor_mode].high_fps_skip_num;
   SENSOR_PRINT("SENSOR_imx132: mode %d, max_fps: %d",fps_info->mode, fps_info->max_fps);
   SENSOR_PRINT("SENSOR_imx132: min_fps: %d", fps_info->min_fps);
   SENSOR_PRINT("SENSOR_imx132: is_high_fps: %d", fps_info->is_high_fps);
   SENSOR_PRINT("SENSOR_imx132: high_fps_skip_num: %d", fps_info->high_fps_skip_num);
   return rtn;
}

static unsigned long imx132_access_val(SENSOR_HW_HANDLE handle, unsigned long param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_VAL_T* param_ptr = (SENSOR_VAL_T*)param;
	uint16_t tmp;

	SENSOR_PRINT("SENSOR_imx132: _imx132_access_val E");
	if(!param_ptr){
		return rtn;
	}

	SENSOR_PRINT("SENSOR_imx132: param_ptr->type=%x", param_ptr->type);
	switch(param_ptr->type)
	{
		case SENSOR_VAL_TYPE_SHUTTER:
			//*((uint32_t*)param_ptr->pval) = imx132_get_shutter();
			break;
		case SENSOR_VAL_TYPE_READ_VCM:
			//rtn = _imx132_read_vcm(param_ptr->pval);
			break;
		case SENSOR_VAL_TYPE_WRITE_VCM:
			//rtn = _imx132_write_vcm(param_ptr->pval);
			break;
		case SENSOR_VAL_TYPE_READ_OTP:
			//((SENSOR_OTP_PARAM_T*)param_ptr->pval)->len = 0;
			//rtn=SENSOR_FAIL;
			//rtn = _hi544_read_otp((uint32_t)param_ptr->pval);
			break;
		case SENSOR_VAL_TYPE_WRITE_OTP:
			//rtn = _hi544_write_otp((uint32_t)param_ptr->pval);
			break;
		case SENSOR_VAL_TYPE_GET_RELOADINFO:
			{
			//struct isp_calibration_info **p= (struct isp_calibration_info **)param_ptr->pval;
			//*p=&calibration_info;
			}
			break;
		case SENSOR_VAL_TYPE_GET_AFPOSITION:
			*(uint32_t*)param_ptr->pval = 0;//cur_af_pos;
			break;
		case SENSOR_VAL_TYPE_WRITE_OTP_GAIN:
			//rtn = imx132_write_otp_gain(param_ptr->pval);
			break;
		case SENSOR_VAL_TYPE_READ_OTP_GAIN:
			//rtn = imx132_read_otp_gain(param_ptr->pval);
			break;
		case SENSOR_VAL_TYPE_GET_STATIC_INFO:
		    rtn = imx132_get_static_info(handle, param_ptr->pval);
		    break;
		case SENSOR_VAL_TYPE_GET_FPS_INFO:
		    rtn = imx132_get_fps_info(handle, param_ptr->pval);
		    break;
		default:
			break;
	}

	SENSOR_PRINT("SENSOR_imx132: _imx132_access_val X");

	return rtn;

}
/*==============================================================================
 * Description:
 * all ioctl functoins
 * you can add functions reference SENSOR_IOCTL_FUNC_TAB_T from sensor_drv_u.h
 *
 * add ioctl functions like this:
 * .power = imx132_power_on,
 *============================================================================*/
static SENSOR_IOCTL_FUNC_TAB_T s_imx132_ioctl_func_tab = {
	.power = imx132_power_on,
	.identify = imx132_identify,
	.get_trim = imx132_get_resolution_trim_tab,
	.before_snapshort = imx132_before_snapshot,
	.write_ae_value = imx132_write_exposure,
	.write_gain_value = imx132_write_gain_value,
	.set_focus = imx132_ext_func,
	//.set_video_mode = imx132_set_video_mode,
	.stream_on = imx132_stream_on,
	.stream_off = imx132_stream_off,

	//.group_hold_on = imx132_group_hold_on,
	//.group_hold_of = imx132_group_hold_off,
	.cfg_otp = imx132_access_val,
};
