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
 * V4.0
 */

#include <utils/Log.h>
#include "sensor.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"

#if defined(CONFIG_CAMERA_ISP_VERSION_V3) || defined(CONFIG_CAMERA_ISP_VERSION_V4)
#include "sensor_sp2509_raw_param_main.c"
#else
#include "sensor_sp2509_raw_param.c"
#endif

#define CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
#ifndef CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
//#include "af_dw9714.h"
#endif

#define SENSOR_NAME			"sp2509_mipi_raw"
#define I2C_SLAVE_ADDR			0x7a    /* 8bit slave address*/

#define sp2509_PID_ADDR			0x02
#define sp2509_PID_VALUE			0x25
#define sp2509_VER_ADDR			0x03
#define sp2509_VER_VALUE			0x09
 
/* sensor parameters begin */
/* effective sensor output image size */
#define SNAPSHOT_WIDTH			1600
#define SNAPSHOT_HEIGHT			1200
#define PREVIEW_WIDTH			1600
#define PREVIEW_HEIGHT		1200

/*Mipi output*/
#define LANE_NUM			1
#define RAW_BITS				10

#define SNAPSHOT_MIPI_PER_LANE_BPS	720
#define PREVIEW_MIPI_PER_LANE_BPS  720                                

/*line time unit: 0.1us*/
#define SNAPSHOT_LINE_TIME	26305 
#define PREVIEW_LINE_TIME		26305 // 947(*2)/36M(*2)

/* frame length*/
#define SNAPSHOT_FRAME_LENGTH		1267 //1234
#define PREVIEW_FRAME_LENGTH		1267 //1234

/* please ref your spec */
#define FRAME_OFFSET			0
#define SENSOR_MAX_GAIN			0x1ff //100 //15.5x //
#define SENSOR_BASE_GAIN		0x10
#define SENSOR_MIN_SHUTTER		1

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
#define SUPPORT_AUTO_FRAME_LENGTH	1 //0
/* sensor parameters end */

/* isp parameters, please don't change it*/
#if 1 //defined(CONFIG_CAMERA_ISP_VERSION_V3) || defined(CONFIG_CAMERA_ISP_VERSION_V4)
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
	uint16_t preview_framelength;
};

/*==============================================================================
 * Description:
 * global variable
 *============================================================================*/
static struct hdr_info_t s_hdr_info;
static uint32_t s_current_default_frame_length;
struct sensor_ev_info_t s_sensor_ev_info;
#define sp2509_RAW_PARAM_COM		0x0000

//#define FEATURE_OTP    /*OTP function switch*/

//#ifdef FEATURE_OTP
//#define MODULE_ID_NULL			0x0000
//#define sp2509_RAW_PARAM_COM		0x0000
//#define MODULE_ID_END			0xFFFF
//#define LSC_PARAM_QTY 240

/*struct otp_info_t {
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


#include "sensor_sp2509_yyy_otp.c"
*/
struct raw_param_info_tab s_sp2509_raw_param_tab[] = {
	{sp2509_RAW_PARAM_COM, &s_sp2509_mipi_raw_info, PNULL, PNULL},
	{RAW_INFO_END_ID, PNULL, PNULL, PNULL}
};

//#endif

static SENSOR_IOCTL_FUNC_TAB_T s_sp2509_ioctl_func_tab;
struct sensor_raw_info *s_sp2509_mipi_raw_info_ptr = &s_sp2509_mipi_raw_info;
static uint32_t s_sp2509_sensor_close_flag = 0;
static uint32_t s_current_frame_length=0;
static uint32_t s_current_default_line_time=0;

static const SENSOR_REG_T sp2509_init_setting[] = {
};
static const SENSOR_REG_T sp2509_snapshot_setting[] = {
{0xfd,0x00},
{0x2f,0x0c},       //2015.12.24
{0x34,0x00},
{0x35,0x21},
{0x30,0x1d},
{0x33,0x05},
{0xfd,0x01},
{0x44,0x00},
{0x2a,0x4c},
{0x2b,0x1e},      // ZH15.12.24
{0x2c,0x60},
{0x25,0x11},      //2015.12.24
{0x03,0x07},
{0x04,0x85},
{0x09,0x00},
{0x0a,0x02},      //2015.12.24
{0x06,0x2b},//0a},
{0x24,0xf0},
{0x01,0x01},
{0xfb,0x73},
{0xfd,0x01},
{0x16,0x04},
{0x1c,0x09},
{0x21,0x46},
{0x6c,0x00},
{0x6b,0x00},
{0x84,0x00},
{0x85,0x10},
{0x86,0x10},
{0x12,0x04},       // ZH15.12.24
{0x13,0x40},       //2015.12.24
{0x11,0x20},
{0x33,0x40},       //2015.12.24
{0xd0,0x03},
{0xd1,0x01},
{0xd2,0x00},       //2015.12.24
{0xd3,0x01},
{0xd4,0x20},       //2015.12.24
{0x51,0x14},       //2015.12.24
{0x52,0x10},       //2015.12.24
{0x55,0x30},
{0x58,0x10},
{0x71,0x10},
{0x6f,0x40},
{0x75,0x60},        //2015.12.24
{0x76,0x10},      // ZH15.12.24
{0x8a,0x22},
{0x8b,0x22},
{0x19,0x71},
{0x29,0x01},
{0xfd,0x01},
{0x9d,0xea},
{0xa0,0x29},//05 lxl151231
{0xa1,0x04},
{0xad,0x62},
{0xae,0x00},
{0xaf,0x85},//85 lxl151231
{0xb1,0x01},
//{0xac,0x01},//mipi_en
{0xfd,0x01},
{0xfc,0x10},
{0xfe,0x10},
{0xf9,0x00},
{0xfa,0x00},
{0xfd,0x01},
{0x8e,0x06},
{0x8f,0x40},
{0x90,0x04},
{0x91,0xb0},  
{0x45,0x01},
{0x46,0x00},
{0x47,0x6c},
{0x48,0x03},
{0x49,0x8b},
{0x4a,0x00},
{0x4b,0x07},
{0x4c,0x04},
{0x4d,0xb7},

};

static const SENSOR_REG_T sp2509_preview_setting[] = {
};

static const SENSOR_REG_T sp2509_snapshot_setting1[] = {
{0xfd,0x00},
{0x2f,0x10},
{0x34,0x00},
{0x30,0x1d},
{0x33,0x05},
{0xfd,0x01},
{0x44,0x00},
{0x2a,0x5c},
{0x2c,0x60},
{0x25,0x06},
{0x03,0x01},
{0x04,0x71},
{0x09,0x00},
{0x0a,0x0a},
{0x06,0x0a},
{0x24,0x80},
{0x01,0x01},
{0xfb,0x63},
{0xfd,0x01},
{0x16,0x04},
{0x1c,0x09},
{0x21,0x66},
{0x6a,0x00},
{0x6b,0x00},
{0x84,0x00},
{0x85,0x10},
{0x86,0x10},
{0x13,0x60},
{0x11,0x20},
{0x33,0x60},
{0xd0,0x03},
{0xd1,0x01},
{0xd2,0x20},
{0xd3,0x01},
{0xd4,0xa0},
{0x58,0x10},
{0x75,0xa0},
{0x51,0x14},
{0x52,0x10},
{0x19,0x74},
{0xfd,0x01},
{0x9d,0x96},
{0xa1,0x04},
{0xb1,0x01},
{0xac,0x01},
};

static SENSOR_REG_TAB_INFO_T s_sp2509_resolution_tab_raw[SENSOR_MODE_MAX] = {
	{ADDR_AND_LEN_OF_ARRAY(sp2509_init_setting), 0, 0, EX_MCLK,
	 SENSOR_IMAGE_FORMAT_RAW},
	/*{ADDR_AND_LEN_OF_ARRAY(sp2509_preview_setting),
	 PREVIEW_WIDTH, PREVIEW_HEIGHT, EX_MCLK,
	 SENSOR_IMAGE_FORMAT_RAW},*/
	{ADDR_AND_LEN_OF_ARRAY(sp2509_snapshot_setting),
	 SNAPSHOT_WIDTH, SNAPSHOT_HEIGHT, EX_MCLK,
	 SENSOR_IMAGE_FORMAT_RAW},
};

static SENSOR_TRIM_T s_sp2509_resolution_trim_tab[SENSOR_MODE_MAX] = {
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	/*{0, 0, PREVIEW_WIDTH, PREVIEW_HEIGHT,
	 PREVIEW_LINE_TIME, PREVIEW_MIPI_PER_LANE_BPS, PREVIEW_FRAME_LENGTH,
	 {0, 0, PREVIEW_WIDTH, PREVIEW_HEIGHT}},*/
	{0, 0, SNAPSHOT_WIDTH, SNAPSHOT_HEIGHT,
	 SNAPSHOT_LINE_TIME, SNAPSHOT_MIPI_PER_LANE_BPS, SNAPSHOT_FRAME_LENGTH,
	 {0, 0, SNAPSHOT_WIDTH, SNAPSHOT_HEIGHT}},
};

static const SENSOR_REG_T s_sp2509_preview_size_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
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

static const SENSOR_REG_T s_sp2509_capture_size_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
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

static SENSOR_VIDEO_INFO_T s_sp2509_video_info[SENSOR_MODE_MAX] = {
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	/*{{{30, 30, 270, 90}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
	 (SENSOR_REG_T **) s_sp2509_preview_size_video_tab},*/
	{{{2, 5, 338, 1000}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
	 (SENSOR_REG_T **) s_sp2509_capture_size_video_tab},
};

/*==============================================================================
 * Description:
 * set video mode
 *
 *============================================================================*/
static uint32_t sp2509_set_video_mode(SENSOR_HW_HANDLE handle,uint32_t param)
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

	if (PNULL == s_sp2509_video_info[mode].setting_ptr) {
		SENSOR_PRINT("fail.");
		return SENSOR_FAIL;
	}

	sensor_reg_ptr = (SENSOR_REG_T_PTR) & s_sp2509_video_info[mode].setting_ptr[param];
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
SENSOR_INFO_T g_sp2509_mipi_raw_info = {
	/* salve i2c write address */
	(I2C_SLAVE_ADDR >> 1),
	/* salve i2c read address */
	(I2C_SLAVE_ADDR >> 1),
	/*bit0: 0: i2c register value is 8 bit, 1: i2c register value is 16 bit */
	SENSOR_I2C_REG_8BIT | SENSOR_I2C_VAL_8BIT | SENSOR_I2C_FREQ_200,
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
	SENSOR_HIGH_LEVEL_PWDN,
	/* count of identify code */
	1,
	/* supply two code to identify sensor.
	 * for Example: index = 0-> Device id, index = 1 -> version id
	 * customer could ignore it.
	 */
	{{sp2509_PID_ADDR, sp2509_PID_VALUE}
	 ,
	 {sp2509_VER_ADDR, sp2509_VER_VALUE}
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
	SENSOR_IMAGE_PATTERN_RAWRGB_B,
	/* point to resolution table information structure */
	s_sp2509_resolution_tab_raw,
	/* point to ioctl function table */
	&s_sp2509_ioctl_func_tab,
	/* information and table about Rawrgb sensor */
	&s_sp2509_mipi_raw_info_ptr,
	/* extend information about sensor
	 * like &g_sp2509_ext_info
	 */
	NULL,
	/* voltage of iovdd */
	SENSOR_AVDD_1800MV,
	/* voltage of dvdd */
	SENSOR_AVDD_1800MV,
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
	"sp2509v1"
};

static SENSOR_STATIC_INFO_T s_sp2509_static_info = {
	240,	//f-number,focal ratio
	201,	//focal_length;
	0,	//max_fps,max fps of sensor's all settings,it will be calculated from sensor mode fps
	16*2,	//max_adgain,AD-gain
	0,	//ois_supported;
	0,	//pdaf_supported;
	1,	//exp_valid_frame_num;N+2-1
	0,	//clamp_level,black level
	1,	//adgain_valid_frame_num;N+1-1
};

static SENSOR_MODE_FPS_INFO_T s_sp2509_mode_fps_info = {
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

LOCAL uint32_t _sp2509_init_mode_fps_info(SENSOR_HW_HANDLE handle)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_PRINT("_sp2509_init_mode_fps_info:E");
	if(!s_sp2509_mode_fps_info.is_init) {
		uint32_t i,modn,tempfps = 0;
		SENSOR_PRINT("_sp2509_init_mode_fps_info:start init");
		for(i = 0;i < NUMBER_OF_ARRAY(s_sp2509_resolution_trim_tab); i++) {
			//max fps should be multiple of 30,it calulated from line_time and frame_line
			tempfps = s_sp2509_resolution_trim_tab[i].line_time*s_sp2509_resolution_trim_tab[i].frame_line;
			if(0 != tempfps) {
				tempfps = 1000000000/tempfps;
				modn = tempfps / 30;
				if(tempfps > modn*30)
					modn++;
				s_sp2509_mode_fps_info.sensor_mode_fps[i].max_fps = modn*30;
				if(s_sp2509_mode_fps_info.sensor_mode_fps[i].max_fps > 30) {
					s_sp2509_mode_fps_info.sensor_mode_fps[i].is_high_fps = 1;
					s_sp2509_mode_fps_info.sensor_mode_fps[i].high_fps_skip_num =
						s_sp2509_mode_fps_info.sensor_mode_fps[i].max_fps/30;
				}
				if(s_sp2509_mode_fps_info.sensor_mode_fps[i].max_fps >
						s_sp2509_static_info.max_fps) {
					s_sp2509_static_info.max_fps =
						s_sp2509_mode_fps_info.sensor_mode_fps[i].max_fps;
				}
			}
			SENSOR_PRINT("mode %d,tempfps %d,frame_len %d,line_time: %d ",i,tempfps,
					s_sp2509_resolution_trim_tab[i].frame_line,
					s_sp2509_resolution_trim_tab[i].line_time);
			SENSOR_PRINT("mode %d,max_fps: %d ",
					i,s_sp2509_mode_fps_info.sensor_mode_fps[i].max_fps);
			SENSOR_PRINT("is_high_fps: %d,highfps_skip_num %d",
					s_sp2509_mode_fps_info.sensor_mode_fps[i].is_high_fps,
					s_sp2509_mode_fps_info.sensor_mode_fps[i].high_fps_skip_num);
		}
		s_sp2509_mode_fps_info.is_init = 1;
	}
	SENSOR_PRINT("_sp2509_init_mode_fps_info:X");
	return rtn;
}

#if 1//defined(CONFIG_CAMERA_ISP_VERSION_V3) || defined(CONFIG_CAMERA_ISP_VERSION_V4)

#define param_update(x1,x2) sprintf(name,"/data/sp2509_%s.bin",x1);\
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

static uint32_t sp2509_InitRawTuneInfo(SENSOR_HW_HANDLE handle)
{
	uint32_t rtn=0x00;

	return rtn;
}
#endif

/*==============================================================================
 * Description:
 * get default frame length
 *
 *============================================================================*/
static uint32_t sp2509_get_default_frame_length(SENSOR_HW_HANDLE handle,uint32_t mode)
{
	return s_sp2509_resolution_trim_tab[mode].frame_line;
}

/*==============================================================================
 * Description:
 * write group-hold on to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void sp2509_group_hold_on(void)
{
	//SENSOR_PRINT("E");

	//Sensor_WriteReg(0xYYYY, 0xff);
}

/*==============================================================================
 * Description:
 * write group-hold off to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void sp2509_group_hold_off(void)
{
	//SENSOR_PRINT("E");

	//Sensor_WriteReg(0xYYYY, 0xff);
}


/*==============================================================================
 * Description:
 * read gain from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static uint16_t sp2509_read_gain(SENSOR_HW_HANDLE handle)
{
/*	uint16_t gain_a = 0;
	uint16_t gain_d = 0;

	//gain_h = Sensor_ReadReg(0xYYYY) & 0xff;
	  Sensor_WriteReg(0xfd, 0x01);
	gain_a = Sensor_ReadReg(0x24) & 0xff;
	Sensor_WriteReg(0xfd, 0x02);
	gain_d = Sensor_ReadReg(0x11) & 0xff;

	return (gain_a*gain_d);*/
	return s_sensor_ev_info.preview_gain;
}

/*==============================================================================
 * Description:
 * write gain to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void sp2509_write_gain(SENSOR_HW_HANDLE handle,float gain)
{
/*	if (SENSOR_MAX_GAIN < gain)
			gain = SENSOR_MAX_GAIN;

	Sensor_WriteReg(0xfd, 0x01);
	Sensor_WriteReg(0x24, gain & 0xff);
	Sensor_WriteReg(0x01, 0x01);*/
	//sp2509_group_hold_off(handle);
	float gain_a = gain;
	float gain_d= 0x80;

	if (SENSOR_MAX_GAIN < (uint32_t)gain) {
		gain_a = SENSOR_MAX_GAIN;
		gain_d = gain*0x80/gain_a;
		if ((uint16_t)gain_d > 2*0x80 - 1)
			gain_d = 2*0x80 -1;
	}
	SENSOR_PRINT("real_gain = 0x%x gain_a %x %x", (uint32_t)gain,(uint8_t)gain_a,(uint8_t)gain_d);
	Sensor_WriteReg(0xfd, 0x01);
	Sensor_WriteReg(0x24, (uint16_t)gain_a & 0xff);
	Sensor_WriteReg(0x37,((uint16_t)gain_a & 0x1ff) >> 0x08);
	Sensor_WriteReg(0x01, 0x01);
	Sensor_WriteReg(0xfd, 0x02);
	Sensor_WriteReg(0x10, (uint8_t)gain_d & 0xff);
	Sensor_WriteReg(0x11, (uint8_t)gain_d & 0xff);
	Sensor_WriteReg(0x13, (uint8_t)gain_d & 0xff);
	Sensor_WriteReg(0x14, (uint8_t)gain_d & 0xff);
}

/*==============================================================================
 * Description:
 * read frame length from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static uint16_t sp2509_read_frame_length(SENSOR_HW_HANDLE handle)
{
	uint16_t frame_len_h = 0;
	uint16_t frame_len_l = 0;
	
	Sensor_WriteReg(0xfd, 0x01);
	frame_len_h = Sensor_ReadReg(0x4e) & 0xff;
	frame_len_l = Sensor_ReadReg(0x4f) & 0xff;

	s_sensor_ev_info.preview_framelength =  ((frame_len_h << 8) | frame_len_l);
	return s_sensor_ev_info.preview_framelength;
}

/*==============================================================================
 * Description:
 * write frame length to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void sp2509_write_frame_length(SENSOR_HW_HANDLE handle,uint32_t frame_len)
{
	Sensor_WriteReg(0xfd, 0x01);
	Sensor_WriteReg(0x05, (frame_len >> 8) & 0xff);
	Sensor_WriteReg(0x06, frame_len & 0xff);
	Sensor_WriteReg(0x01, 0x01);
	//s_sensor_ev_info.preview_framelength = frame_len;
}

/*==============================================================================
 * Description:
 * read shutter from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t sp2509_read_shutter(SENSOR_HW_HANDLE handle)
{
	uint16_t shutter_h = 0;
	uint16_t shutter_l = 0;
  	Sensor_WriteReg(0xfd, 0x01);
	shutter_h = Sensor_ReadReg(0x03) & 0xff;
	shutter_l = Sensor_ReadReg(0x04) & 0xff;

	return (shutter_h << 8) | shutter_l;
}

/*==============================================================================
 * Description:
 * write shutter to sensor registers
 * please pay attention to the frame length
 * please modify this function acording your spec
 *============================================================================*/
static void sp2509_write_shutter(SENSOR_HW_HANDLE handle,uint32_t shutter)
{
	Sensor_WriteReg(0xfd, 0x01);
	Sensor_WriteReg(0x03, (shutter >> 8) & 0xff);
	Sensor_WriteReg(0x04, shutter & 0xff);
	Sensor_WriteReg(0x01, 0x01);
	//return shutter;
}

/*==============================================================================
 * Description:
 * write exposure to sensor registers and get current shutter
 * please pay attention to the frame length
 * please don't change this function if it's necessary
 *============================================================================*/
static uint16_t sp2509_update_exposure(SENSOR_HW_HANDLE handle,uint32_t shutter,uint32_t dummy_line)
{
	uint32_t dest_fr_len = 0;
	uint32_t cur_fr_len = 0;
	uint32_t fr_len = s_current_default_frame_length;

	//sp2509_group_hold_on();

	if (1 == SUPPORT_AUTO_FRAME_LENGTH)
		goto write_sensor_shutter;

	dummy_line = dummy_line > FRAME_OFFSET ? dummy_line : FRAME_OFFSET;
	dest_fr_len = ((shutter + dummy_line) > fr_len) ? (shutter +dummy_line) : fr_len;
	s_current_frame_length = dest_fr_len;

	cur_fr_len = sp2509_read_frame_length(handle);
	SENSOR_PRINT("current shutter = %d, fr_len = %d, dummy_line=%d cur_fr_len %d dest_fr_len %d", shutter, fr_len,dummy_line,cur_fr_len,dest_fr_len);

	if (shutter < SENSOR_MIN_SHUTTER)
		shutter = SENSOR_MIN_SHUTTER;

	if (dest_fr_len != cur_fr_len)
		sp2509_write_frame_length(handle,dummy_line);
	s_sensor_ev_info.preview_framelength = dest_fr_len;
write_sensor_shutter:
	/* write shutter to sensor registers */
	s_sensor_ev_info.preview_shutter=shutter;
	sp2509_write_shutter(handle,shutter);
	return shutter;
}

/*==============================================================================
 * Description:
 * sensor power on
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t sp2509_power_on(SENSOR_HW_HANDLE handle,uint32_t power_on)
{
	SENSOR_AVDD_VAL_E dvdd_val = g_sp2509_mipi_raw_info.dvdd_val;
	SENSOR_AVDD_VAL_E avdd_val = g_sp2509_mipi_raw_info.avdd_val;
	SENSOR_AVDD_VAL_E iovdd_val = g_sp2509_mipi_raw_info.iovdd_val;
	BOOLEAN power_down = g_sp2509_mipi_raw_info.power_down_level;
	BOOLEAN reset_level = g_sp2509_mipi_raw_info.reset_pulse_level;

	if (SENSOR_TRUE == power_on) {
		Sensor_PowerDown(power_down);
		Sensor_SetResetLevel(reset_level);
		Sensor_SetIovddVoltage(iovdd_val);
		Sensor_SetAvddVoltage(avdd_val);
		Sensor_SetDvddVoltage(dvdd_val);
		usleep(5 * 1000);
		Sensor_PowerDown(!power_down);
		usleep(5 * 1000);
		Sensor_SetResetLevel(!reset_level);
		Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);

		#ifndef CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
		Sensor_SetMonitorVoltage(SENSOR_AVDD_2800MV);
		//zzz_init(2);
		#endif

	} else {
    
		#ifndef CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
		//zzz_deinit(2);
		Sensor_SetMonitorVoltage(SENSOR_AVDD_CLOSED);
		#endif
		Sensor_SetResetLevel(reset_level);
		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		Sensor_PowerDown(power_down);
		Sensor_SetAvddVoltage(SENSOR_AVDD_CLOSED);
		Sensor_SetDvddVoltage(SENSOR_AVDD_CLOSED);
		Sensor_SetIovddVoltage(SENSOR_AVDD_CLOSED);
	}
	SENSOR_PRINT("(1:on, 0:off): %d", power_on);
	return SENSOR_SUCCESS;
}

//#ifdef FEATURE_OTP

/*==============================================================================
 * Description:
 * get  parameters from otp
 * please modify this function acording your spec
 *============================================================================*/
/*static int sp2509_get_otp_info(struct otp_info_t *otp_info)
{
	uint32_t ret = SENSOR_FAIL;
	uint32_t i = 0x00;

	//identify otp information
	for (i = 0; i < NUMBER_OF_ARRAY(s_sp2509_raw_param_tab); i++) {
		SENSOR_PRINT("identify module_id=0x%x",s_sp2509_raw_param_tab[i].param_id);

		if(PNULL!=s_sp2509_raw_param_tab[i].identify_otp){
			//set default value;
			memset(otp_info, 0x00, sizeof(struct otp_info_t));

			if(SENSOR_SUCCESS==s_sp2509_raw_param_tab[i].identify_otp(otp_info)){
				if (s_sp2509_raw_param_tab[i].param_id== otp_info->module_id) {
					SENSOR_PRINT("identify otp sucess! module_id=0x%x",s_sp2509_raw_param_tab[i].param_id);
					ret = SENSOR_SUCCESS;
					break;
				}
				else{
					SENSOR_PRINT("identify module_id failed! table module_id=0x%x, otp module_id=0x%x",s_sp2509_raw_param_tab[i].param_id,otp_info->module_id);
				}
			}
			else{
				SENSOR_PRINT("identify_otp failed!");
			}
		}
		else{
			SENSOR_PRINT("no identify_otp function!");
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
/*static uint32_t sp2509_apply_otp(struct otp_info_t *otp_info, int id)
{
	uint32_t ret = SENSOR_FAIL;
	//apply otp parameters
	SENSOR_PRINT("otp_table_id = %d", id);
	if (PNULL != s_sp2509_raw_param_tab[id].cfg_otp) {

		if(SENSOR_SUCCESS==s_sp2509_raw_param_tab[id].cfg_otp(otp_info)){
			SENSOR_PRINT("apply otp parameters sucess! module_id=0x%x",s_sp2509_raw_param_tab[id].param_id);
			ret = SENSOR_SUCCESS;
		}
		else{
			SENSOR_PRINT("update_otp failed!");
		}
	}else{
		SENSOR_PRINT("no update_otp function!");
	}

	return ret;
}
*/
/*==============================================================================
 * Description:
 * cfg otp setting
 * please modify this function acording your spec
 *============================================================================*/
/*static uint32_t sp2509_cfg_otp(uint32_t param)
{
	uint32_t ret = SENSOR_FAIL;
	struct otp_info_t otp_info={0x00};
	int table_id = 0;

	table_id = sp2509_get_otp_info(&otp_info);
	if (-1 != table_id)
		ret = sp2509_apply_otp(&otp_info, table_id);

	//checking OTP apply result
	if (SENSOR_SUCCESS != ret) {//disable lsc
		Sensor_WriteReg(0xYYYY,0xYY);
	}
	else{//enable lsc
		Sensor_WriteReg(0xYYYY,0xYY);
	}

	return ret;
}
#endif
*/
/*==============================================================================
 * Description:
 * identify sensor id
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t sp2509_identify(SENSOR_HW_HANDLE handle,uint32_t param)
{
	uint16_t pid_value = 0x00;
	uint16_t ver_value = 0x00;
	uint32_t ret_value = SENSOR_FAIL;

	SENSOR_PRINT("mipi raw identify");

	pid_value = Sensor_ReadReg(sp2509_PID_ADDR);

	if (sp2509_PID_VALUE == pid_value) {
		ver_value = Sensor_ReadReg(sp2509_VER_ADDR);
		SENSOR_PRINT("Identify: PID = %x, VER = %x", pid_value, ver_value);
		if (sp2509_VER_VALUE == ver_value) {
			//#if defined(CONFIG_CAMERA_ISP_VERSION_V3) || defined(CONFIG_CAMERA_ISP_VERSION_V4)
			sp2509_InitRawTuneInfo(handle);
			//#endif
			_sp2509_init_mode_fps_info(handle);
			ret_value = SENSOR_SUCCESS;
			SENSOR_PRINT_HIGH("this is sp2509 sensor");
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
static unsigned long sp2509_get_resolution_trim_tab(SENSOR_HW_HANDLE handle,uint32_t param)
{
	return (unsigned long) s_sp2509_resolution_trim_tab;
}

/*==============================================================================
 * Description:
 * before snapshot
 * you can change this function if it's necessary
 *============================================================================*/
static uint32_t sp2509_before_snapshot(SENSOR_HW_HANDLE handle,uint32_t param)
{
	uint32_t cap_shutter = 0;
	uint32_t prv_shutter = 0;
	uint32_t gain = 0;
	uint32_t cap_gain = 0;
	uint32_t capture_mode = param & 0xffff;
	uint32_t preview_mode = (param >> 0x10) & 0xffff;

	uint32_t prv_linetime = s_sp2509_resolution_trim_tab[preview_mode].line_time;
	uint32_t cap_linetime = s_sp2509_resolution_trim_tab[capture_mode].line_time;

	s_current_default_frame_length = sp2509_get_default_frame_length(handle,capture_mode);
	SENSOR_PRINT("capture_mode = %d", capture_mode);

	if (preview_mode == capture_mode) {
		cap_shutter = s_sensor_ev_info.preview_shutter;
		cap_gain = s_sensor_ev_info.preview_gain;
		goto snapshot_info;
	}

	prv_shutter = s_sensor_ev_info.preview_shutter;	//sp2509_read_shutter();
	gain = s_sensor_ev_info.preview_gain;	//sp2509_read_gain();

	Sensor_SetMode(capture_mode);
	Sensor_SetMode_WaitDone();

	cap_shutter = prv_shutter * prv_linetime / cap_linetime ;//* BINNING_FACTOR;
/*
	while (gain >= (2 * SENSOR_BASE_GAIN)) {
		if (cap_shutter * 2 > s_current_default_frame_length)
			break;
		cap_shutter = cap_shutter * 2;
		gain = gain / 2;
	}
*/
	cap_shutter = sp2509_update_exposure(handle,cap_shutter,0);
	cap_gain = gain;
	sp2509_write_gain(handle,cap_gain);
	SENSOR_PRINT("preview_shutter = 0x%x, preview_gain = 0x%x",
		     s_sensor_ev_info.preview_shutter, s_sensor_ev_info.preview_gain);

	SENSOR_PRINT("capture_shutter = 0x%x, capture_gain = 0x%x", cap_shutter, cap_gain);
snapshot_info:
	s_hdr_info.capture_shutter = sp2509_read_shutter(handle);
	s_hdr_info.capture_gain = sp2509_read_gain(handle);
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
static uint32_t sp2509_write_exposure_dummy(SENSOR_HW_HANDLE handle, uint32_t exposure_line,
		uint32_t dummy_line, uint16_t size_index)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	/*uint16_t exposure_line = 0x00;
	uint16_t dummy_line = 0x00;
	uint16_t mode = 0x00;

	exposure_line = param & 0xffff;
	dummy_line = (param >> 0x10) & 0xfff; //for cits frame rate test
	mode = (param >> 0x1c) & 0x0f;*/

	s_current_default_frame_length = sp2509_get_default_frame_length(handle,size_index);

	s_sensor_ev_info.preview_shutter = sp2509_update_exposure(handle,exposure_line,dummy_line);
	
	return ret_value;
}

LOCAL unsigned long sp2509_write_exposure(SENSOR_HW_HANDLE handle, unsigned long param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint32_t expsure_line = 0x00;
	uint32_t dummy_line = 0x00;
	uint32_t size_index=0x00;


	struct sensor_ex_exposure  *ex = (struct sensor_ex_exposure*)param;

	if (!param) {
		SENSOR_PRINT_ERR("param is NULL !!!");
		return ret_value;
	}

	expsure_line = ex->exposure;
	dummy_line = ex->dummy;
	size_index = ex->size_index;


	ret_value = sp2509_write_exposure_dummy(handle, expsure_line, dummy_line, size_index);

	return ret_value;
}

LOCAL unsigned long sp2509_ex_write_exposure(SENSOR_HW_HANDLE handle, unsigned long param)
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
	SENSOR_PRINT("current mode = %d, exposure_line = %d, dummy_line=%d frame_length %d", size_index, exposure_line,dummy_line,s_current_default_frame_length);
	s_current_default_frame_length = sp2509_get_default_frame_length(handle,size_index);
	s_current_default_line_time = s_sp2509_resolution_trim_tab[size_index].line_time;

	ret_value = sp2509_write_exposure_dummy(handle, exposure_line, dummy_line, size_index);

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
static uint32_t sp2509_write_gain_value(SENSOR_HW_HANDLE handle,unsigned long param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	float real_gain = 0;

	//real_gain = isp_to_real_gain(handle,param);
	param = param < SENSOR_BASE_GAIN ? SENSOR_BASE_GAIN : param;

	real_gain = (float)1.0*param * SENSOR_BASE_GAIN / ISP_BASE_GAIN;

	SENSOR_PRINT("real_gain = 0x%x", (uint32_t)real_gain);

	s_sensor_ev_info.preview_gain = (uint32_t)real_gain;
	sp2509_write_gain(handle,(uint32_t)real_gain);

	return ret_value;
}

#ifndef CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
/*==============================================================================
 * Description:
 * write parameter to vcm
 * please add your VCM function to this function
 *============================================================================*/
/*static uint32_t sp2509_write_af(uint32_t param)
{
	return zzz_write_af(param);
}*/
#endif

/*==============================================================================
 * Description:
 * increase gain or shutter for hdr
 *
 *============================================================================*/
static void sp2509_increase_hdr_exposure(SENSOR_HW_HANDLE handle,uint8_t ev_multiplier)
{
	uint32_t shutter_multiply = s_hdr_info.capture_max_shutter / s_hdr_info.capture_shutter;
	uint32_t gain = 0;

	if (0 == shutter_multiply)
		shutter_multiply = 1;

	if (shutter_multiply >= ev_multiplier) {
		sp2509_update_exposure(handle,s_hdr_info.capture_shutter * ev_multiplier,0);
		sp2509_write_gain(handle,s_hdr_info.capture_gain);
	} else {
		gain = s_hdr_info.capture_gain * ev_multiplier / shutter_multiply;
		sp2509_update_exposure(handle,s_hdr_info.capture_shutter * shutter_multiply,0);
		sp2509_write_gain(handle,gain);
	}
}

/*==============================================================================
 * Description:
 * decrease gain or shutter for hdr
 *
 *============================================================================*/
static void sp2509_decrease_hdr_exposure(SENSOR_HW_HANDLE handle,uint8_t ev_divisor)
{
	uint16_t gain_multiply = 0;
	uint32_t shutter = 0;
	gain_multiply = s_hdr_info.capture_gain / SENSOR_BASE_GAIN;

	if (gain_multiply >= ev_divisor) {
		sp2509_update_exposure(handle,s_hdr_info.capture_shutter,0);
		sp2509_write_gain(handle,s_hdr_info.capture_gain / ev_divisor);

	} else {
		shutter = s_hdr_info.capture_shutter * gain_multiply / ev_divisor;
		sp2509_update_exposure(handle,shutter,0);
		sp2509_write_gain(handle,s_hdr_info.capture_gain / gain_multiply);
	}
}

/*==============================================================================
 * Description:
 * set hdr ev
 * you can change this function if it's necessary
 *============================================================================*/
static uint32_t sp2509_set_hdr_ev(SENSOR_HW_HANDLE handle,unsigned long param)
{
	uint32_t ret = SENSOR_SUCCESS;
	SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR) param;

	uint32_t ev = ext_ptr->param;
	uint8_t ev_divisor, ev_multiplier;

	switch (ev) {
	case SENSOR_HDR_EV_LEVE_0:
		ev_divisor = 2;
		sp2509_decrease_hdr_exposure(handle,ev_divisor);
		break;
	case SENSOR_HDR_EV_LEVE_1:
		ev_multiplier = 2;
		sp2509_increase_hdr_exposure(handle,ev_multiplier);
		break;
	case SENSOR_HDR_EV_LEVE_2:
		ev_multiplier = 1;
		sp2509_increase_hdr_exposure(handle,ev_multiplier);
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
static uint32_t sp2509_ext_func(SENSOR_HW_HANDLE handle,unsigned long param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR) param;

	SENSOR_PRINT("ext_ptr->cmd: %d", ext_ptr->cmd);
	switch (ext_ptr->cmd) {
	case SENSOR_EXT_EV:
		rtn = sp2509_set_hdr_ev(handle,param);
		break;
	default:
		break;
	}

	return rtn;
}
static unsigned long _sp2509_SetSlave_FrameSync(SENSOR_HW_HANDLE handle, unsigned long param)
{
	Sensor_WriteReg(0xfd, 0x00);
	Sensor_WriteReg(0x1f, 0x01);
	Sensor_WriteReg(0x40, 0x01);

	Sensor_WriteReg(0xfd, 0x01);
	Sensor_WriteReg(0x0b, 0x22);
	Sensor_WriteReg(0x01, 0x01);
	return 0;
}

/*==============================================================================
 * Description:
 * mipi stream on
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t sp2509_stream_on(SENSOR_HW_HANDLE handle,uint32_t param)
{
	SENSOR_PRINT("E");
	//_sp2509_SetSlave_FrameSync(handle,param);
	Sensor_WriteReg(0xfd, 0x01);
	Sensor_WriteReg(0xac, 0x01);
	/*delay*/
	//usleep(10 * 1000);
	
	return 0;
}

/*==============================================================================
 * Description:
 * mipi stream off
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t sp2509_stream_off(SENSOR_HW_HANDLE handle,uint32_t param)
{
	SENSOR_PRINT("E");
	unsigned char value;
	unsigned int sleep_time = 0, frame_time;

	value = Sensor_ReadReg(0xac);
	if (value == 0x01) {
		Sensor_WriteReg(0xfd,0x01);
		Sensor_WriteReg(0xac,0x00);
		if (!s_sp2509_sensor_close_flag) {
			usleep(50 * 1000);
		}
	} else {
		Sensor_WriteReg(0xfd,0x01);
		Sensor_WriteReg(0xac,0x00);
	}

	s_sp2509_sensor_close_flag = 0;
	SENSOR_LOGI("X sleep_time=%dus", sleep_time);
	return 0;
} 

/*==============================================================================
 * Description:
 * calculate fps for every sensor mode according to frame_line and line_time
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t sp2509_init_mode_fps_info(SENSOR_HW_HANDLE handle)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_LOGI("sp2509_init_mode_fps_info:E");
	if(!s_sp2509_mode_fps_info.is_init) {
		uint32_t i,modn,tempfps = 0;
		SENSOR_LOGI("sp2509_init_mode_fps_info:start init");
		for(i = 0;i < NUMBER_OF_ARRAY(s_sp2509_resolution_trim_tab); i++) {
			//max fps should be multiple of 30,it calulated from line_time and frame_line
			tempfps = s_sp2509_resolution_trim_tab[i].line_time*s_sp2509_resolution_trim_tab[i].frame_line;
				if(0 != tempfps) {
					tempfps = 1000000000/tempfps;
				modn = tempfps / 30;
				if(tempfps > modn*30)
					modn++;
				s_sp2509_mode_fps_info.sensor_mode_fps[i].max_fps = modn*30;
				if(s_sp2509_mode_fps_info.sensor_mode_fps[i].max_fps > 30) {
					s_sp2509_mode_fps_info.sensor_mode_fps[i].is_high_fps = 1;
					s_sp2509_mode_fps_info.sensor_mode_fps[i].high_fps_skip_num =
						s_sp2509_mode_fps_info.sensor_mode_fps[i].max_fps/30;
				}
				if(s_sp2509_mode_fps_info.sensor_mode_fps[i].max_fps >
						s_sp2509_static_info.max_fps) {
					s_sp2509_static_info.max_fps =
						s_sp2509_mode_fps_info.sensor_mode_fps[i].max_fps;
				}
			}
			SENSOR_LOGI("mode %d,tempfps %d,frame_len %d,line_time: %d ",i,tempfps,
					s_sp2509_resolution_trim_tab[i].frame_line,
					s_sp2509_resolution_trim_tab[i].line_time);
			SENSOR_LOGI("mode %d,max_fps: %d ",
					i,s_sp2509_mode_fps_info.sensor_mode_fps[i].max_fps);
			SENSOR_LOGI("is_high_fps: %d,highfps_skip_num %d",
					s_sp2509_mode_fps_info.sensor_mode_fps[i].is_high_fps,
					s_sp2509_mode_fps_info.sensor_mode_fps[i].high_fps_skip_num);
		}
		s_sp2509_mode_fps_info.is_init = 1;
	}
	SENSOR_LOGI("sp2509_init_mode_fps_info:X");
	return rtn;
}

static uint32_t sp2509_get_static_info(SENSOR_HW_HANDLE handle, uint32_t *param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	struct sensor_ex_info *ex_info;
	uint32_t up = 0;
	uint32_t down = 0;
	//make sure we have get max fps of all settings.
	if(!s_sp2509_mode_fps_info.is_init) {
		sp2509_init_mode_fps_info(handle);
	}
	ex_info = (struct sensor_ex_info*)param;
	ex_info->f_num = s_sp2509_static_info.f_num;
	ex_info->focal_length = s_sp2509_static_info.focal_length;
	ex_info->max_fps = s_sp2509_static_info.max_fps;
	ex_info->max_adgain = s_sp2509_static_info.max_adgain;
	ex_info->ois_supported = s_sp2509_static_info.ois_supported;
	ex_info->pdaf_supported = s_sp2509_static_info.pdaf_supported;
	ex_info->exp_valid_frame_num = s_sp2509_static_info.exp_valid_frame_num;
	ex_info->clamp_level = s_sp2509_static_info.clamp_level;
	ex_info->adgain_valid_frame_num = s_sp2509_static_info.adgain_valid_frame_num;
	ex_info->preview_skip_num = g_sp2509_mipi_raw_info.preview_skip_num;
	ex_info->capture_skip_num = g_sp2509_mipi_raw_info.capture_skip_num;
	ex_info->name = g_sp2509_mipi_raw_info.name;
	ex_info->sensor_version_info = g_sp2509_mipi_raw_info.sensor_version_info;
	//vcm_dw9800_get_pose_dis(handle, &up, &down);
	ex_info->pos_dis.up2hori = up;
	ex_info->pos_dis.hori2down = down;
	SENSOR_LOGI("SENSOR_sp2509: f_num: %d", ex_info->f_num);
	SENSOR_LOGI("SENSOR_sp2509: max_fps: %d", ex_info->max_fps);
	SENSOR_LOGI("SENSOR_sp2509: max_adgain: %d", ex_info->max_adgain);
	SENSOR_LOGI("SENSOR_sp2509: ois_supported: %d", ex_info->ois_supported);
	SENSOR_LOGI("SENSOR_sp2509: pdaf_supported: %d", ex_info->pdaf_supported);
	SENSOR_LOGI("SENSOR_sp2509: exp_valid_frame_num: %d", ex_info->exp_valid_frame_num);
	SENSOR_LOGI("SENSOR_sp2509: clam_level: %d", ex_info->clamp_level);
	SENSOR_LOGI("SENSOR_sp2509: adgain_valid_frame_num: %d", ex_info->adgain_valid_frame_num);
	SENSOR_LOGI("SENSOR_sp2509: sensor name is: %s", ex_info->name);
	SENSOR_LOGI("SENSOR_sp2509: sensor version info is: %s", ex_info->sensor_version_info);

	return rtn;
}


static uint32_t sp2509_get_fps_info(SENSOR_HW_HANDLE handle, uint32_t *param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_MODE_FPS_T *fps_info;
	//make sure have inited fps of every sensor mode.
	if(!s_sp2509_mode_fps_info.is_init) {
		sp2509_init_mode_fps_info(handle);
	}
	fps_info = (SENSOR_MODE_FPS_T*)param;
	uint32_t sensor_mode = fps_info->mode;
	fps_info->max_fps = s_sp2509_mode_fps_info.sensor_mode_fps[sensor_mode].max_fps;
	fps_info->min_fps = s_sp2509_mode_fps_info.sensor_mode_fps[sensor_mode].min_fps;
	fps_info->is_high_fps = s_sp2509_mode_fps_info.sensor_mode_fps[sensor_mode].is_high_fps;
	fps_info->high_fps_skip_num = s_sp2509_mode_fps_info.sensor_mode_fps[sensor_mode].high_fps_skip_num;
	SENSOR_LOGI("SENSOR_sp2509: mode %d, max_fps: %d",fps_info->mode, fps_info->max_fps);
	SENSOR_LOGI("SENSOR_sp2509: min_fps: %d", fps_info->min_fps);
	SENSOR_LOGI("SENSOR_sp2509: is_high_fps: %d", fps_info->is_high_fps);
	SENSOR_LOGI("SENSOR_sp2509: high_fps_skip_num: %d", fps_info->high_fps_skip_num);

	return rtn;
}

static uint32_t sp2509_set_sensor_close_flag(SENSOR_HW_HANDLE handle)
{
	uint32_t rtn = SENSOR_SUCCESS;

	s_sp2509_sensor_close_flag = 1;

	return rtn;
}

static unsigned long sp2509_access_val(SENSOR_HW_HANDLE handle, unsigned long param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_VAL_T* param_ptr = (SENSOR_VAL_T*)param;
	uint16_t tmp;

	SENSOR_LOGI("SENSOR_sp2509: _sp2509_access_val E param_ptr = %p", param_ptr);
	if(!param_ptr){
		return rtn;
	}

	SENSOR_LOGI("SENSOR_sp2509: param_ptr->type=%x", param_ptr->type);
	switch(param_ptr->type)
	{
		case SENSOR_VAL_TYPE_INIT_OTP:
			//rtn = sp2509_otp_init(handle);
			break;
		case SENSOR_VAL_TYPE_SHUTTER:
			*((uint32_t*)param_ptr->pval) = s_sensor_ev_info.preview_shutter;
			break;
		case SENSOR_VAL_TYPE_READ_VCM:
			//rtn = sp2509_read_vcm(handle, param_ptr->pval);
			break;
		case SENSOR_VAL_TYPE_WRITE_VCM:
			//rtn = sp2509_write_vcm(handle, param_ptr->pval);
			break;
		case SENSOR_VAL_TYPE_READ_OTP:
			//rtn = sp2509_otp_read(handle,param_ptr);
			break;
		case SENSOR_VAL_TYPE_PARSE_OTP:
			//rtn = sp2509_parse_otp(handle, param_ptr);
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
			//rtn = sp2509_write_otp_gain(handle, param_ptr->pval);
			break;
		case SENSOR_VAL_TYPE_READ_OTP_GAIN:
			*((uint32_t*)param_ptr->pval)  = s_sensor_ev_info.preview_gain;
			break;
		case SENSOR_VAL_TYPE_GET_STATIC_INFO:
			rtn = sp2509_get_static_info(handle, param_ptr->pval);
			break;
		case SENSOR_VAL_TYPE_GET_FPS_INFO:
			rtn = sp2509_get_fps_info(handle, param_ptr->pval);
			break;
		case SENSOR_VAL_TYPE_SET_SENSOR_CLOSE_FLAG:
			rtn = sp2509_set_sensor_close_flag(handle);
			break;
		default:
			break;
	}

	SENSOR_LOGI("SENSOR_sp2509: _sp2509_access_val X");

	return rtn;
}

/*==============================================================================
 * Description:
 * all ioctl functoins
 * you can add functions reference SENSOR_IOCTL_FUNC_TAB_T from sensor_drv_u.h
 *
 * add ioctl functions like this:
 * .power = sp2509_power_on,
 *============================================================================*/
static SENSOR_IOCTL_FUNC_TAB_T s_sp2509_ioctl_func_tab = {
	.power = sp2509_power_on,
	.identify = sp2509_identify,
	.get_trim = sp2509_get_resolution_trim_tab,
	.before_snapshort = sp2509_before_snapshot,
	//.write_ae_value = sp2509_write_exposure,
	.write_gain_value = sp2509_write_gain_value,
	#ifndef CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
	//.af_enable = sp2509_write_af,
	#endif
	//.set_focus = sp2509_ext_func,
	//.set_video_mode = sp2509_set_video_mode,
	.stream_on = sp2509_stream_on,
	.stream_off = sp2509_stream_off,
	//#ifdef FEATURE_OTP
	//.cfg_otp=sp2509_cfg_otp,
	//#endif	
	.cfg_otp = sp2509_access_val,
	.ex_write_exp = sp2509_ex_write_exposure,

	//.group_hold_on = sp2509_group_hold_on,
	//.group_hold_of = sp2509_group_hold_off,
};
