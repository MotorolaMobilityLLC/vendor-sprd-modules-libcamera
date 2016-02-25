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
#include "sensor_s5k3p3sm_raw_param.c"
#include "isp_param_file_update.h"
#include "af_bu64297gwz.h"

#define S5K3P3SM_I2C_ADDR_W        0x10
#define S5K3P3SM_I2C_ADDR_R        0x10
//#define S5K3P3SM_I2C_ADDR_W        0x2d
//#define S5K3P3SM_I2C_ADDR_R        0x2d
#define S5K3P3SM_RAW_PARAM_COM     0x0000

#ifndef RAW_INFO_END_ID
#define RAW_INFO_END_ID 0x71717567
#endif

static uint32_t g_module_id = 0;
static struct sensor_raw_info* s_s5k3p3sm_mipi_raw_info_ptr = NULL;
static int s_capture_shutter = 0;
static int s_exposure_time = 0;

static unsigned long _s5k3p3sm_GetResolutionTrimTab(unsigned long param);
static unsigned long _s5k3p3sm_Identify(unsigned long param);
static uint32_t _s5k3p3sm_GetRawInof(void);
static unsigned long _s5k3p3sm_StreamOn(unsigned long param);
static unsigned long _s5k3p3sm_StreamOff(unsigned long param);
static uint32_t _s5k3p3sm_com_Identify_otp(void* param_ptr);
static unsigned long _s5k3p3sm_PowerOn(unsigned long power_on);
static unsigned long _s5k3p3sm_write_exposure(unsigned long param);
static unsigned long _s5k3p3sm_write_gain(unsigned long param);
static unsigned long _s5k3p3sm_access_val(unsigned long param);
static unsigned long s5k3p3sm_write_af(unsigned long param);
static unsigned long _s5k3p3sm_GetExifInfo(unsigned long param);
static unsigned long _s5k3p3sm_BeforeSnapshot(unsigned long param);
static uint16_t _s5k3p3sm_get_shutter(void);
static unsigned long s5k3p3sm_cfg_otp(unsigned long param);
static unsigned long _s5k3p3sm_ex_write_exposure(unsigned long param);


static const struct raw_param_info_tab s_s5k3p3sm_raw_param_tab[] = {
	{S5K3P3SM_RAW_PARAM_COM, &s_s5k3p3sm_mipi_raw_info, _s5k3p3sm_com_Identify_otp, NULL},
	{RAW_INFO_END_ID, PNULL, PNULL, PNULL}
};


static const SENSOR_REG_T s5k3p3sm_common_init[] = {
{ 0x6028, 0x4000 },
{ 0x6010, 0x0001 },// Reset
{ 0xffff, 0x000a },//must add delay >3ms
{ 0x6214, 0x7971 },
{ 0x6218, 0x0100 },// Clock on
{ 0x3D7C, 0x1110 },
{ 0x3D88, 0x0064 },
{ 0x3D8A, 0x0068 },
{ 0xF408, 0x0048 },
{ 0xF40C, 0x0000 },
{ 0xF4AA, 0x0060 },
{ 0xF442, 0x0800 },
{ 0xF43E, 0x2020 },
{ 0xF440, 0x0000 },
{ 0xF4AC, 0x004B },
{ 0xF492, 0x0016 },
{ 0xF480, 0x0040 },
{ 0xF4A4, 0x0010 },
{ 0x3E58, 0x004B },
{ 0x3A38, 0x006C },
{ 0x3552, 0x00D0 },
{ 0x3CD6, 0x0100 },
{ 0x3CD8, 0x017F },
{ 0x3CDA, 0x1000 },
{ 0x3CDC, 0x104F },
{ 0x3CDE, 0x0180 },
{ 0x3CE0, 0x01FF },
{ 0x3CE2, 0x104F },
{ 0x3CE4, 0x104F },
{ 0x3CE6, 0x0200 },
{ 0x3CE8, 0x03FF },
{ 0x3CEA, 0x104F },
{ 0x3CEC, 0x1058 },
{ 0x3CEE, 0x0400 },
{ 0x3CF0, 0x07FF },
{ 0x3CF2, 0x1057 },
{ 0x3CF4, 0x1073 },
{ 0x3CF6, 0x0800 },
{ 0x3CF8, 0x1000 },
{ 0x3CFA, 0x1073 },
{ 0x3CFC, 0x10A2 },
{ 0x3D16, 0x0100 },
{ 0x3D18, 0x017F },
{ 0x3D1A, 0x1000 },
{ 0x3D1C, 0x104F },
{ 0x3D1E, 0x0180 },
{ 0x3D20, 0x01FF },
{ 0x3D22, 0x104F },
{ 0x3D24, 0x104F },
{ 0x3D26, 0x0200 },
{ 0x3D28, 0x03FF },
{ 0x3D2A, 0x104F },
{ 0x3D2C, 0x1058 },
{ 0x3D2E, 0x0400 },
{ 0x3D30, 0x07FF },
{ 0x3D32, 0x1057 },
{ 0x3D34, 0x1073 },
{ 0x3D36, 0x0800 },
{ 0x3D38, 0x1000 },
{ 0x3D3A, 0x1073 },
{ 0x3D3C, 0x10A2 },
{ 0x3002, 0x0001 },
{ 0x0216, 0x0101 },
{ 0x021B, 0x0100 },
//{ 0x0202, 0x0100 },
{ 0x0202, 0x06a0 },
{ 0x0200, 0x0200 },
{ 0x021E, 0x0100 },
{ 0x021C, 0x0200 },
{ 0x3072, 0x03C0 },//Global
{ 0x6028, 0x4000 },
{ 0x0B08, 0x0000 },
{ 0x3058, 0x0001 },
{ 0x0B0E, 0x0000 },
{ 0x316C, 0x0084 },
{ 0x316E, 0x1283 },
{ 0x3170, 0x0060 },
{ 0x3172, 0x0DDF },
{ 0x3D66, 0x0010 },
{ 0x3D68, 0x1004 },
{ 0x3D6A, 0x0404 },
{ 0x3D6C, 0x0704 },
{ 0x3D6E, 0x0B08 },
{ 0x3D70, 0x0708 },
{ 0x3D72, 0x0B08 },
{ 0x3D74, 0x0B08 },
{ 0x3D76, 0x0F00 },
{ 0x9920, 0x0104 },
{ 0x9928, 0x03CB },
{ 0x3D78, 0x396C },
{ 0x3D7A, 0x93C6 },//TnP
};

// 4632x3480 30FPS v560M mipi1392M 4lane
// frame length	0x0E2A
// 1H time	91.9
static const SENSOR_REG_T s5k3p3sm_4632x3480_4lane_setting[] = {
 { 0x6028, 0x2000 },
 { 0x602A, 0x2E26 },
 { 0x6F12, 0x0103 },
 { 0x6028, 0x4000 },
 { 0x3D7C, 0x1110 },
 { 0x3D88, 0x0064 },
 { 0x3D8A, 0x0068 },
 { 0x0344, 0x0000 },
 { 0x0346, 0x0008 },
 { 0x0348, 0x1227 },
 { 0x034A, 0x0D9F },
 { 0x034C, 0x1218 },
 { 0x034E, 0x0D98 },
 { 0x0408, 0x0008 },
 { 0x0900, 0x0011 },
 { 0x0380, 0x0001 },
 { 0x0382, 0x0001 },
 { 0x0384, 0x0001 },
 { 0x0386, 0x0001 },
 { 0x0400, 0x0000 },
 { 0x0404, 0x0010 },
 { 0x0114, 0x0300 },
 { 0x0110, 0x0002 },
 { 0x0136, 0x1800 },
 { 0x0304, 0x0006 },
 { 0x0306, 0x008C },
 { 0x0302, 0x0001 },
 { 0x0300, 0x0004 },
 { 0x030C, 0x0004 },
 { 0x030E, 0x0074 },
 { 0x030A, 0x0001 },
 { 0x0308, 0x0008 },
 { 0x0342, 0x141C },
 { 0x0340, 0x0E2A },
 { 0x0B0E, 0x0100 },
 { 0x0216, 0x0101 },
 { 0x0100, 0x0000 }, //steam off
 //{ 0x0100, 0x0100 }, //steam on
};


// 1920X1080 60FPS v560M mipi660M 4lane 2x2binning
// frame length	0x06E7
// 1H time	94.3
static const SENSOR_REG_T s5k3p3sm_1920x1080_4lane_setting[] = {
 { 0x6028, 0x2000 },
 { 0x602A, 0x2E26 },
 { 0x6F12, 0x0103 },
 { 0x6028, 0x4000 },
 { 0x3D7C, 0x0010 },
 { 0x3D88, 0x0064 },
 { 0x3D8A, 0x0068 },
 { 0x0344, 0x018C },
 { 0x0346, 0x029C },
 { 0x0348, 0x109B },
 { 0x034A, 0x0B0B },
 { 0x034C, 0x0780 },
 { 0x034E, 0x0438 },
 { 0x0408, 0x0004 },
 { 0x0900, 0x0122 },
 { 0x0380, 0x0001 },
 { 0x0382, 0x0003 },
 { 0x0384, 0x0001 },
 { 0x0386, 0x0003 },
 { 0x0400, 0x0001 },
 { 0x0404, 0x0010 },
 { 0x0114, 0x0300 },
 { 0x0110, 0x0002 },
 { 0x0136, 0x1800 },
 { 0x0304, 0x0006 },
 { 0x0306, 0x008C },
 { 0x0302, 0x0001 },
 { 0x0300, 0x0004 },
 { 0x030C, 0x0004 },
 { 0x030E, 0x0037 },
 { 0x030A, 0x0001 },
 { 0x0308, 0x0008 },
 { 0x0342, 0x14A2 },
 { 0x0340, 0x06E7 },
 { 0x0B0E, 0x0100 },
 { 0x0216, 0x0000 },
// { 0x0100, 0x0000 }, //steam off
// { 0x0100, 0x0100 },//steam on
};

// 1280x720 120FPS v560M mipi660M 4lane 3x3binning
// frame length	0x038A
// 1H time	91.9
static const SENSOR_REG_T s5k3p3sm_1280x720_4lane_setting[] = {
 { 0x6028, 0x2000 },
 { 0x602A, 0x2E26 },
 { 0x6F12, 0x0103 },
 { 0x6028, 0x4000 },
 { 0x3D7C, 0x0010 },
 { 0x3D88, 0x0064 },
 { 0x3D8A, 0x0068 },
 { 0x0344, 0x018C },
 { 0x0346, 0x029C },
 { 0x0348, 0x109A },
 { 0x034A, 0x0B0B },
 { 0x034C, 0x0500 },
 { 0x034E, 0x02D0 },
 { 0x0408, 0x0004 },
 { 0x0900, 0x0133 },
 { 0x0380, 0x0001 },
 { 0x0382, 0x0005 },
 { 0x0384, 0x0001 },
 { 0x0386, 0x0005 },
 { 0x0400, 0x0001 },
 { 0x0404, 0x0010 },
 { 0x0114, 0x0300 },
 { 0x0110, 0x0002 },
 { 0x0136, 0x1800 },
 { 0x0304, 0x0006 },
 { 0x0306, 0x008C },
 { 0x0302, 0x0001 },
 { 0x0300, 0x0004 },
 { 0x030C, 0x0004 },
 { 0x030E, 0x0037 },
 { 0x030A, 0x0001 },
 { 0x0308, 0x0008 },
 { 0x0342, 0x141C },
 { 0x0340, 0x038A },
 { 0x0B0E, 0x0100 },
 { 0x0216, 0x0000 },
 { 0x0100, 0x0000 }, //steam off
// { 0x0100, 0x0100 },//steam on
};

static SENSOR_REG_TAB_INFO_T s_s5k3p3sm_resolution_Tab_RAW[] = {
	{ADDR_AND_LEN_OF_ARRAY(s5k3p3sm_common_init), 0, 0, 24, SENSOR_IMAGE_FORMAT_RAW},
	//{ADDR_AND_LEN_OF_ARRAY(s5k3p3sm_2592x1944_2lane_setting), 2592, 1944, 24, SENSOR_IMAGE_FORMAT_RAW},
	{ADDR_AND_LEN_OF_ARRAY(s5k3p3sm_1920x1080_4lane_setting), 1920, 1080, 24, SENSOR_IMAGE_FORMAT_RAW},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
};

static SENSOR_TRIM_T s_s5k3p3sm_Resolution_Trim_Tab[] = {
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	//{0, 0, 2592, 1944, 267, 750, 1248, {0, 0, 1632, 1224}},
	{0, 0, 1920, 1080, 94, 1320, 1767, {0, 0, 1920, 1080}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}}
};

static const SENSOR_REG_T s_s5k3p3sm_2576x1932_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
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

static SENSOR_VIDEO_INFO_T s_s5k3p3sm_video_info[] = {
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{30, 30, 164, 100}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},(SENSOR_REG_T**)s_s5k3p3sm_2576x1932_video_tab},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL}
};

static SENSOR_IOCTL_FUNC_TAB_T s_s5k3p3sm_ioctl_func_tab = {
	PNULL,
	_s5k3p3sm_PowerOn,
	PNULL,
	_s5k3p3sm_Identify,

	PNULL,// write register
	PNULL,// read  register
	PNULL,
	_s5k3p3sm_GetResolutionTrimTab,

	// External
	PNULL,
	PNULL,
	PNULL,

	PNULL,//_s5k3p3sm_set_brightness,
	PNULL,// _s5k3p3sm_set_contrast,
	PNULL,
	PNULL,//_s5k3p3sm_set_saturation,

	PNULL,//_s5k3p3sm_set_work_mode,
	PNULL,//_s5k3p3sm_set_image_effect,

	_s5k3p3sm_BeforeSnapshot,
	PNULL,//_s5k3p3sm_after_snapshot,
	PNULL,//_s5k3p3sm_flash,
	PNULL,
	_s5k3p3sm_write_exposure,
	PNULL,
	_s5k3p3sm_write_gain,
	PNULL,
	PNULL,
	s5k3p3sm_write_af,
	PNULL,
	PNULL,//_s5k3p3sm_set_awb,
	PNULL,
	PNULL,
	PNULL,//_s5k3p3sm_set_ev,
	PNULL,
	PNULL,
	PNULL,
	_s5k3p3sm_GetExifInfo,
	PNULL,//_s5k3p3sm_ExtFunc,
	PNULL,//_s5k3p3sm_set_anti_flicker,
	PNULL,//_s5k3p3sm_set_video_mode,
	PNULL,//pick_jpeg_stream
	PNULL,//meter_mode
	PNULL,//get_status
	_s5k3p3sm_StreamOn,
	_s5k3p3sm_StreamOff,
	s5k3p3sm_cfg_otp,//	_s5k3p3sm_access_val,
	_s5k3p3sm_ex_write_exposure
};


SENSOR_INFO_T g_s5k3p3sm_mipi_raw_info = {
	S5K3P3SM_I2C_ADDR_W,	// salve i2c write address
	S5K3P3SM_I2C_ADDR_R,	// salve i2c read address

	SENSOR_I2C_REG_16BIT | SENSOR_I2C_VAL_16BIT | SENSOR_I2C_FREQ_400,	// bit0: 0: i2c register value is 8 bit, 1: i2c register value is 16 bit
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
	{{0x0, 0x5e},		// supply two code to identify sensor.
	 {0x1, 0x20}},		// for Example: index = 0-> Device id, index = 1 -> version id

//	SENSOR_AVDD_3000MV,	// voltage of avdd
	SENSOR_AVDD_2800MV,	// voltage of avdd

	4632,			// max width of source image
	3480,			// max height of source image
	(cmr_s8 *)"s5k3p3sm_mipi_raw",		// name of sensor

	SENSOR_IMAGE_FORMAT_RAW,	// define in SENSOR_IMAGE_FORMAT_E enum,SENSOR_IMAGE_FORMAT_MAX
	// if set to SENSOR_IMAGE_FORMAT_MAX here, image format depent on SENSOR_REG_TAB_INFO_T

	SENSOR_IMAGE_PATTERN_RAWRGB_GR,//SENSOR_IMAGE_PATTERN_RAWRGB_R,// pattern of input image form sensor;

	s_s5k3p3sm_resolution_Tab_RAW,	// point to resolution table information structure
	&s_s5k3p3sm_ioctl_func_tab,	// point to ioctl function table
	&s_s5k3p3sm_mipi_raw_info_ptr,		// information and table about Rawrgb sensor
	NULL,			//&g_s5k3p3sm_ext_info,                // extend information about sensor
	SENSOR_AVDD_1800MV,	// iovdd
	SENSOR_AVDD_1200MV,	// dvdd
	3,			// skip frame num before preview
	3,			// skip frame num before capture
	0,			// deci frame num during preview
	0,			// deci frame num during video preview

	0,
	0,
	0,
	0,
	0,
	{SENSOR_INTERFACE_TYPE_CSI2, 4, 10, 0}, // csi, 4lanes, 10bits, 0:packed, 1: half-word
	s_s5k3p3sm_video_info,
	3,			// skip frame num while change setting
	48,			// horizontal view angle
	48,			// vertical view angle
};

static struct sensor_raw_info* Sensor_GetContext(void)
{
	return s_s5k3p3sm_mipi_raw_info_ptr;
}

#define param_update(x1,x2) sprintf(name,"/data/s5k3p3sm_%s.bin",x1);\
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

static uint32_t Sensor_s5k3p3sm_InitRawTuneInfo(void)
{
	uint32_t rtn=0x00;
#if 0
	isp_raw_para_update_from_file(&g_s5k3p3sm_mipi_raw_info,1);

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

#endif

	return rtn;
}

static unsigned long _s5k3p3sm_GetResolutionTrimTab(unsigned long param)
{
	SENSOR_PRINT("0x%lx",  (unsigned long)s_s5k3p3sm_Resolution_Trim_Tab);
	return (unsigned long) s_s5k3p3sm_Resolution_Trim_Tab;
}



static unsigned long _s5k3p3sm_Identify(unsigned long param)
{
#define S5K3P3SM_PID_VALUE    0x3103
#define S5K3P3SM_PID_ADDR     0x0000
#define S5K3P3SM_VER_VALUE    0xc001
#define S5K3P3SM_VER_ADDR     0x0002
	uint16_t pid_value = 0x00;
	uint16_t ver_value = 0x00;
	uint32_t ret_value = SENSOR_FAIL;

	SENSOR_PRINT_ERR("SENSOR_S5K3P3SM: mipi raw identify\n");

	pid_value = Sensor_ReadReg(S5K3P3SM_PID_ADDR);
	ver_value = Sensor_ReadReg(S5K3P3SM_VER_ADDR);

	if (S5K3P3SM_PID_VALUE == pid_value) {
		SENSOR_PRINT("SENSOR_S5K3P3SM: Identify: PID = %x, VER = %x", pid_value, ver_value);
		if (S5K3P3SM_VER_VALUE == ver_value) {
			SENSOR_PRINT_ERR("SENSOR_S5K3P3SM: this is S5K3P3SM sensor !");
			ret_value=_s5k3p3sm_GetRawInof();
			if (SENSOR_SUCCESS != ret_value) {
				SENSOR_PRINT_ERR("SENSOR_S5K3P3SM: the module is unknow error !");
			}
			bu64297gwz_init();
			Sensor_s5k3p3sm_InitRawTuneInfo();
		} else {
			SENSOR_PRINT_ERR("SENSOR_S5K3P3SM: Identify this is hm%x%x sensor !", pid_value, ver_value);
			return ret_value;
		}
	} else {
		SENSOR_PRINT_ERR("SENSOR_S5K3P3SM: identify fail,pid_value=%d, ver_value = %d", pid_value, ver_value);
	}

	return ret_value;
}

static uint32_t _s5k3p3sm_GetMaxFrameLine(uint32_t index)
{
	uint32_t max_line=0x00;
	SENSOR_TRIM_T_PTR trim_ptr=s_s5k3p3sm_Resolution_Trim_Tab;

	max_line=trim_ptr[index].frame_line;

	return max_line;
}

static unsigned long _s5k3p3sm_write_exp_dummy(uint16_t expsure_line,
								uint16_t dummy_line, uint16_t size_index)

{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint32_t frame_len_cur = 0x00;
	uint32_t frame_len = 0x00;
	uint32_t max_frame_len=0x00;
	uint32_t linetime = 0;


	SENSOR_PRINT("SENSOR_S5K3P3SM: write_exp_dummy line:%d, dummy:%d, size_index:%d", expsure_line, dummy_line, size_index);
	max_frame_len=_s5k3p3sm_GetMaxFrameLine(size_index);
	if (expsure_line < 3) {
		expsure_line = 3;
	}

	frame_len = expsure_line + dummy_line;
	frame_len = (frame_len > (uint32_t)(expsure_line + 8)) ? frame_len : (uint32_t)(expsure_line + 8);
	frame_len = (frame_len > max_frame_len) ? frame_len : max_frame_len;
	if (0x00!=(0x01&frame_len)) {
		frame_len+=0x01;
	}

	frame_len_cur = Sensor_ReadReg(0x0340);

	if(frame_len_cur != frame_len) {
		ret_value = Sensor_WriteReg(0x0340, frame_len);
	}

	ret_value = Sensor_WriteReg(0x202, expsure_line);


	/*if (frame_len_cur > frame_len) {
		ret_value = Sensor_WriteReg(0x0341, frame_len & 0xff);
		ret_value = Sensor_WriteReg(0x0340, (frame_len >> 0x08) & 0xff);
	}*/
	//ret_value = Sensor_WriteReg(0x104, 0x00);
	s_capture_shutter = expsure_line;
	linetime=s_s5k3p3sm_Resolution_Trim_Tab[size_index].line_time;
	Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_EXPOSURETIME, s_capture_shutter);
	s_exposure_time = s_capture_shutter * linetime / 10;

	return ret_value;
}
static unsigned long _s5k3p3sm_write_exposure(unsigned long param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint32_t expsure_line = 0x00;
	uint32_t dummy_line = 0x00;
	uint32_t size_index=0x00;


	expsure_line=param&0xffff;
	dummy_line=(param>>0x10)&0x0fff;
	size_index=(param>>0x1c)&0x0f;

	SENSOR_PRINT("SENSOR_S5K3P3SM: write_exposure line:%d, dummy:%d, size_index:%d", expsure_line, dummy_line, size_index);

	ret_value = _s5k3p3sm_write_exp_dummy(expsure_line, dummy_line, size_index);

	return ret_value;
}

static unsigned long s5k3p3sm_cfg_otp(unsigned long param)
{
	SENSOR_PRINT("E");
	return 0;
}

static unsigned long _s5k3p3sm_ex_write_exposure(unsigned long param)
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

	ret_value = _s5k3p3sm_write_exp_dummy(exposure_line, dummy_line, size_index);

	return ret_value;
}

static unsigned long _s5k3p3sm_write_gain(unsigned long param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint32_t real_gain = 0;
	uint32_t a_gain = 0;
	uint32_t d_gain = 0;


	real_gain = param >> 2; // / 128 * 32;

	SENSOR_PRINT("SENSOR_S5K3P3SM: real_gain:0x%x, param: 0x%lx", real_gain, param);


	if (real_gain <= 16*32) {
		a_gain = real_gain;
		d_gain = 256;
	} else {
		a_gain = 16*32;
		d_gain = real_gain>>1;
	}

	ret_value = Sensor_WriteReg(0x204, a_gain);
#if 0
	ret_value = Sensor_WriteReg(0x20e, d_gain);
	ret_value = Sensor_WriteReg(0x210, d_gain);
	ret_value = Sensor_WriteReg(0x212, d_gain);
	ret_value = Sensor_WriteReg(0x214, d_gain);
#endif
//	ret_value = Sensor_WriteReg(0x104, 0x00);
	SENSOR_PRINT("SENSOR_S5K3P3SM: a_gain:0x%x, d_gain: 0x%x", a_gain, d_gain);

	return ret_value;

}

static unsigned long s5k3p3sm_write_af(unsigned long param)
{
	bu64297gwz_write_af(param);
	return 0;
}


static unsigned long _s5k3p3sm_BeforeSnapshot(unsigned long param)
{
	uint8_t ret_l, ret_m, ret_h;
	uint32_t capture_exposure, preview_maxline;
	uint32_t capture_maxline, preview_exposure;
	uint32_t capture_mode = param & 0xffff;
	uint32_t preview_mode = (param >> 0x10 ) & 0xffff;
	uint32_t prv_linetime=s_s5k3p3sm_Resolution_Trim_Tab[preview_mode].line_time;
	uint32_t cap_linetime = s_s5k3p3sm_Resolution_Trim_Tab[capture_mode].line_time;

	SENSOR_PRINT("SENSOR_s5k3p3sm: BeforeSnapshot mode: 0x%08lx",param);

	if (preview_mode == capture_mode) {
		SENSOR_PRINT("SENSOR_s5k3p3sm: prv mode equal to capmode");
		goto CFG_INFO;
	}


	CFG_INFO:
	s_capture_shutter = _s5k3p3sm_get_shutter();
	Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_EXPOSURETIME, s_capture_shutter);
	s_exposure_time = s_capture_shutter * cap_linetime / 10;

	return SENSOR_SUCCESS;

}

static uint32_t _s5k3p3sm_GetRawInof(void)
{
	uint32_t rtn=SENSOR_SUCCESS;
	struct raw_param_info_tab* tab_ptr = (struct raw_param_info_tab*)s_s5k3p3sm_raw_param_tab;
	uint32_t param_id;
	uint32_t i=0x00;

	/*read param id from sensor omap*/
	param_id=S5K3P3SM_RAW_PARAM_COM;

	for (i=0x00; ; i++) {
		g_module_id = i;
		if (RAW_INFO_END_ID==tab_ptr[i].param_id) {
			if (NULL==s_s5k3p3sm_mipi_raw_info_ptr) {
				SENSOR_PRINT("SENSOR_S5K3P3SM: _s5k3p3sm_GetRawInof no param error");
				rtn=SENSOR_FAIL;
			}
			SENSOR_PRINT("SENSOR_S5K3P3SM: _s5k3p3sm_GetRawInof end");
			break;
		}
		else if (PNULL!=tab_ptr[i].identify_otp) {
			if (SENSOR_SUCCESS==tab_ptr[i].identify_otp(0)) {
				s_s5k3p3sm_mipi_raw_info_ptr = tab_ptr[i].info_ptr;
				SENSOR_PRINT("SENSOR_S5K3P3SM: _s5k3p3sm_GetRawInof success");
				break;
			}
		}
	}

	return rtn;
}

static unsigned long _s5k3p3sm_GetExifInfo(unsigned long param)
{
	LOCAL EXIF_SPEC_PIC_TAKING_COND_T sexif;

	sexif.ExposureTime.numerator = s_exposure_time;
	sexif.ExposureTime.denominator = 1000000;

	return (unsigned long) & sexif;
}


static unsigned long _s5k3p3sm_StreamOn(unsigned long param)
{
	SENSOR_PRINT_ERR("SENSOR_S5K3P3SM: StreamOn");

	Sensor_WriteReg(0x0100, 0x0100);

	SENSOR_PRINT_ERR("SENSOR_S5K3P3SM: StreamOn end");

	return 0;
}

static unsigned long _s5k3p3sm_StreamOff(unsigned long param)
{
	SENSOR_PRINT_ERR("SENSOR_S5K3P3SM: StreamOff");

	Sensor_WriteReg(0x0100, 0x0000);
	usleep(10*1000);

	return 0;
}

static uint32_t _s5k3p3sm_com_Identify_otp(void* param_ptr)
{
	uint32_t rtn=SENSOR_FAIL;
	uint32_t param_id;

	SENSOR_PRINT("SENSOR_S5K3P3SM: _s5k3p3sm_com_Identify_otp");

	/*read param id from sensor omap*/
	param_id=S5K3P3SM_RAW_PARAM_COM;

	if(S5K3P3SM_RAW_PARAM_COM==param_id){
		rtn=SENSOR_SUCCESS;
	}

	return rtn;
}

static unsigned long _s5k3p3sm_PowerOn(unsigned long power_on)
{
	SENSOR_AVDD_VAL_E dvdd_val = g_s5k3p3sm_mipi_raw_info.dvdd_val;
	SENSOR_AVDD_VAL_E avdd_val = g_s5k3p3sm_mipi_raw_info.avdd_val;
	SENSOR_AVDD_VAL_E iovdd_val = g_s5k3p3sm_mipi_raw_info.iovdd_val;
	BOOLEAN power_down = g_s5k3p3sm_mipi_raw_info.power_down_level;
	BOOLEAN reset_level = g_s5k3p3sm_mipi_raw_info.reset_pulse_level;

	uint8_t pid_value = 0x00;

	if (SENSOR_TRUE == power_on) {
		Sensor_SetResetLevel(reset_level);
		Sensor_PowerDown(power_down);
		Sensor_SetVoltage(dvdd_val, avdd_val, iovdd_val);
		Sensor_SetMonitorVoltage(SENSOR_AVDD_3000MV);
		usleep(50*1000);

		Sensor_SetResetLevel(!reset_level);
		Sensor_PowerDown(!power_down);
		usleep(50*1000);
		//_dw9807_SRCInit(2);
		Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);
		usleep(50*1000);
	} else {
		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		Sensor_SetResetLevel(reset_level);
		Sensor_PowerDown(power_down);
		Sensor_SetVoltage(SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED);
		Sensor_SetMonitorVoltage(SENSOR_AVDD_CLOSED);
	}

	SENSOR_PRINT_ERR("SENSOR_S5K3P3SM: _s5k3p3sm_PowerOn(1:on, 0:off): %ld, reset_level %d, dvdd_val %d", power_on, reset_level, dvdd_val);
	return SENSOR_SUCCESS;
}

static uint32_t _s5k3p3sm_write_otp_gain(uint32_t *param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t value = 0x00;

	SENSOR_PRINT("SENSOR_s5k3p3sm: write_gain:0x%x\n", *param);

	//ret_value = Sensor_WriteReg(0x104, 0x01);
	value = (*param)>>0x08;
	ret_value = Sensor_WriteReg(0x204, value);
	value = (*param)&0xff;
	ret_value = Sensor_WriteReg(0x205, value);
	ret_value = Sensor_WriteReg(0x104, 0x00);

	return ret_value;
}

static uint32_t _s5k3p3sm_read_otp_gain(uint32_t *param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	uint16_t gain_h = 0;
	uint16_t gain_l = 0;
	#if 1 // for MP tool //!??
	gain_h = Sensor_ReadReg(0x0204) & 0xff;
	gain_l = Sensor_ReadReg(0x0205) & 0xff;
	*param = ((gain_h << 8) | gain_l);
	#else
	*param = s_set_gain;
	#endif
	SENSOR_PRINT("SENSOR_s5k3p3sm: gain: %d", *param);

	return rtn;
}

static uint16_t _s5k3p3sm_get_shutter(void)
{
	// read shutter, in number of line period
	uint16_t shutter_h = 0;
	uint16_t shutter_l = 0;
#if 1  // MP tool //!??
	shutter_h = Sensor_ReadReg(0x0202) & 0xff;
	shutter_l = Sensor_ReadReg(0x0203) & 0xff;

	return (shutter_h << 8) | shutter_l;
#else
	return s_set_exposure;
#endif
}

static unsigned long _s5k3p3sm_access_val(unsigned long param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_VAL_T* param_ptr = (SENSOR_VAL_T*)param;
	uint16_t tmp;

	SENSOR_PRINT("SENSOR_s5k3p3sm: _s5k3p3sm_access_val E");
	if(!param_ptr){
		return rtn;
	}

	SENSOR_PRINT("SENSOR_s5k3p3sm: param_ptr->type=%x", param_ptr->type);
	switch(param_ptr->type)
	{
		case SENSOR_VAL_TYPE_SHUTTER:
			*((uint32_t*)param_ptr->pval) = _s5k3p3sm_get_shutter();
			break;
		case SENSOR_VAL_TYPE_READ_VCM:
			//rtn = _s5k3p3sm_read_vcm(param_ptr->pval);
			break;
		case SENSOR_VAL_TYPE_WRITE_VCM:
			//rtn = _s5k3p3sm_write_vcm(param_ptr->pval);
			break;
		case SENSOR_VAL_TYPE_READ_OTP:
			((SENSOR_OTP_PARAM_T*)param_ptr->pval)->len = 0;
			rtn=SENSOR_FAIL;
			//rtn = _hi544_read_otp((uint32_t)param_ptr->pval);
			break;
		case SENSOR_VAL_TYPE_WRITE_OTP:
			//rtn = _hi544_write_otp((uint32_t)param_ptr->pval);
			break;
		case SENSOR_VAL_TYPE_GET_RELOADINFO:
			{
//				struct isp_calibration_info **p= (struct isp_calibration_info **)param_ptr->pval;
//				*p=&calibration_info;
			}
			break;
		case SENSOR_VAL_TYPE_GET_AFPOSITION:
			*(uint32_t*)param_ptr->pval = 0;//cur_af_pos;
			break;
		case SENSOR_VAL_TYPE_WRITE_OTP_GAIN:
			rtn = _s5k3p3sm_write_otp_gain(param_ptr->pval);
			break;
		case SENSOR_VAL_TYPE_READ_OTP_GAIN:
			rtn = _s5k3p3sm_read_otp_gain(param_ptr->pval);
			break;
		default:
			break;
	}

	SENSOR_PRINT("SENSOR_s5k3p3sm: _s5k3p3sm_access_val X");

	return rtn;
}

