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
#include "sensor_gc2235_raw_param.c"

#define gc2235_I2C_ADDR_W        0x3c
#define gc2235_I2C_ADDR_R         0x3c

LOCAL uint32_t _gc2235_GetResolutionTrimTab(uint32_t param);
LOCAL uint32_t _gc2235_PowerOn(uint32_t power_on);
LOCAL uint32_t _gc2235_Identify(uint32_t param);
LOCAL uint32_t _gc2235_BeforeSnapshot(uint32_t param);
LOCAL uint32_t _gc2235_after_snapshot(uint32_t param);
LOCAL uint32_t _gc2235_StreamOn(uint32_t param);
LOCAL uint32_t _gc2235_StreamOff(uint32_t param);
LOCAL uint32_t _gc2235_write_exposure(uint32_t param);
LOCAL uint32_t _gc2235_write_gain(uint32_t param);
LOCAL uint32_t _gc2235_SetEV(uint32_t param);
LOCAL uint32_t _gc2235_ExtFunc(uint32_t ctl_param);

static uint32_t s_gc2235_gain = 0;

LOCAL const SENSOR_REG_T gc2235_com_raw[] =
{
	/////////////////////////////////////////////////////
	//////////////////////	 SYS   //////////////////////
	/////////////////////////////////////////////////////
	 {0xfe, 0x80},
	 {0xfe, 0x80},
	 {0xfe, 0x80},
	 {0xf2, 0x00}, //sync_pad_io_ebi
	 {0xf6, 0x00}, //up down
	 {0xfc, 0x06},
	 {0xf7, 0x15}, //pll enable
	 {0xf8, 0x83}, //Pll mode 2
	 {0xf9, 0xfe}, //[0] pll enable
	 {0xfa, 0x00}, //div
	 {0xfe, 0x00},

	/////////////////////////////////////////////////////
	////////////////   ANALOG & CISCTL	 ////////////////
	/////////////////////////////////////////////////////
	 {0x03, 0x0a},
	 {0x04, 0xec},
	 {0x05, 0x00},
	 {0x06, 0xd0},
	 {0x07, 0x00},
	 {0x08, 0x1a},
	 {0x0a, 0x02}, //row start
	 {0x0c, 0x00}, //0c //col start
	 {0x0d, 0x04}, //Window setting
	 {0x0e, 0xd0},
	 {0x0f, 0x06},
	 {0x10, 0x50},
	 {0x17, 0x15}, //14 //[0]mirror [1]flip
	 {0x18, 0x1e},
	 {0x19, 0x06},
	 {0x1a, 0x01},
	 {0x1b, 0x48},
	 {0x1e, 0x88},
	 {0x1f, 0x48}, //08//comv_r
	 {0x20, 0x03}, //07
	 {0x21, 0x6f}, //0f //rsg
	 {0x22, 0x80},
	 {0x23, 0xc1}, //c3
	 {0x24, 0x2f}, //16//PAD_drv
	 {0x26, 0x01}, //07
	 {0x27, 0x30},
	 {0x3f, 0x00},

	/////////////////////////////////////////////////////
	//////////////////////	 ISP   //////////////////////
	/////////////////////////////////////////////////////
	 {0x8b, 0xa0},
	 {0x8c, 0x02},
	 {0x90, 0x01},
	 {0x92, 0x02},
	 {0x94, 0x06},
	 {0x95, 0x04},
	 {0x96, 0xb0},
	 {0x97, 0x06},
	 {0x98, 0x40},

	/////////////////////////////////////////////////////
	//////////////////////	 BLK   //////////////////////
	/////////////////////////////////////////////////////
	 {0x40, 0x25},//27
	 {0x41, 0x04},//82
	 {0x5e, 0x20},//20
	 {0x5f, 0x20},
	 {0x60, 0x20},
	 {0x61, 0x20},
	 {0x62, 0x20},
	 {0x63, 0x20},
	 {0x64, 0x20},
	 {0x65, 0x20},
	 {0x66, 0x00},
	 {0x67, 0x00},
	 {0x68, 0x00},
	 {0x69, 0x00},


	/////////////////////////////////////////////////////
	//////////////////////	 GAIN	/////////////////////
	/////////////////////////////////////////////////////
	 {0xb2, 0x00},
	 {0xb3, 0x40},
	 {0xb4, 0x40},
	 {0xb5, 0x40},

	/////////////////////////////////////////////////////
	////////////////////   DARK SUN   ///////////////////
	/////////////////////////////////////////////////////
	 {0xb8, 0x0f},
	 {0xb9, 0x23},
	 {0xba, 0xff},
	 {0xbc, 0x00}, //dark sun_en
	 {0xbd, 0x00},
	 {0xbe, 0xff},
	 {0xbf, 0x09},

	/////////////////////////////////////////////////////
	//////////////////////	 MIPI	/////////////////////
	/////////////////////////////////////////////////////
	 {0xfe, 0x03},
	 {0x01, 0x00},
	 {0x02, 0x00},
	 {0x03, 0x00},
	 {0x06, 0x00},
	 {0x10, 0x00},
	 {0x15, 0x00},
	 {0xfe, 0x00},
	 {0xf2, 0x0f}, //sync_pad_io_ebi


};

LOCAL SENSOR_REG_TAB_INFO_T s_gc2235_resolution_Tab_RAW[] = {
	{ADDR_AND_LEN_OF_ARRAY(gc2235_com_raw), 0, 0, 24, SENSOR_IMAGE_FORMAT_RAW},
	{ADDR_AND_LEN_OF_ARRAY(gc2235_com_raw), 1600, 1200, 24, SENSOR_IMAGE_FORMAT_RAW},
	{ADDR_AND_LEN_OF_ARRAY(gc2235_com_raw), 1600, 1200, 24, SENSOR_IMAGE_FORMAT_RAW},

	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0}
};

LOCAL SENSOR_TRIM_T s_gc2235_Resolution_Trim_Tab[] = {
	{0, 0, 0, 0, 0, 0},
	{0, 0, 1600, 1200, 430, 480},//sysclk*10
	{0, 0, 1600, 1200, 430, 480},//sysclk*10
	{0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0}
};

static struct sensor_raw_info s_gc2235_raw_info={
	&s_gc2235_version_info,
	&s_gc2235_tune_info,
	&s_gc2235_fix_info
};


struct sensor_raw_info* s_gc2235_raw_info_ptr=&s_gc2235_raw_info;

LOCAL SENSOR_IOCTL_FUNC_TAB_T s_gc2235_ioctl_func_tab = {
	PNULL,
	_gc2235_PowerOn,
	PNULL,
	_gc2235_Identify,

	PNULL,// write register
	PNULL,// read  register
	PNULL,
	_gc2235_GetResolutionTrimTab,

	// External
	PNULL,
	PNULL,
	PNULL,

	PNULL, //_gc2235_set_brightness,
	PNULL, // _gc2235_set_contrast,
	PNULL,
	PNULL,//_gc2235_set_saturation,

	PNULL, //_gc2235_set_work_mode,
	PNULL, //_gc2235_set_image_effect,

	_gc2235_BeforeSnapshot,
	_gc2235_after_snapshot,
	PNULL,//_gc2235_flash,
	PNULL,
	_gc2235_write_exposure,
	PNULL,
	_gc2235_write_gain,
	PNULL,
	PNULL,
	PNULL,    //_gc2235_write_af,
	PNULL,
	PNULL, //_gc2235_set_awb,
	PNULL,
	PNULL,
	PNULL, //_gc2235_set_ev,
	PNULL,
	PNULL,
	PNULL,
	PNULL, //_gc2235_GetExifInfo,
	_gc2235_ExtFunc,
	PNULL, //_gc2235_set_anti_flicker,
	PNULL, //_gc2235_set_video_mode,
	PNULL, //pick_jpeg_stream
	PNULL,  //meter_mode
	PNULL, //get_status
	_gc2235_StreamOn,
	_gc2235_StreamOff
};


SENSOR_INFO_T g_gc2235_raw_info = {
	gc2235_I2C_ADDR_W,	// salve i2c write address
	gc2235_I2C_ADDR_R,	// salve i2c read address

	SENSOR_I2C_REG_8BIT | SENSOR_I2C_REG_8BIT,	// bit0: 0: i2c register value is 8 bit, 1: i2c register value is 16 bit
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
	{{0xf0, 0x22},		// supply two code to identify sensor.
	 {0xf1, 0x35}},		// for Example: index = 0-> Device id, index = 1 -> version id

	SENSOR_AVDD_2800MV,	// voltage of avdd

	1600,			// max width of source image
	1200,			// max height of source image
	"gc2235",		// name of sensor

	SENSOR_IMAGE_FORMAT_RAW,	// define in SENSOR_IMAGE_FORMAT_E enum,SENSOR_IMAGE_FORMAT_MAX
	// if set to SENSOR_IMAGE_FORMAT_MAX here, image format depent on SENSOR_REG_TAB_INFO_T

	SENSOR_IMAGE_PATTERN_RAWRGB_B,// pattern of input image form sensor;

	s_gc2235_resolution_Tab_RAW,	// point to resolution table information structure
	&s_gc2235_ioctl_func_tab,	// point to ioctl function table
	(uint32_t *)&s_gc2235_raw_info,		// information and table about Rawrgb sensor
	NULL,			//&g_gc2235_ext_info,                // extend information about sensor
	SENSOR_AVDD_1800MV,	// iovdd
	SENSOR_AVDD_1800MV,	// dvdd
	3,			// skip frame num before preview
	3,			// skip frame num before capture
	0,			// deci frame num during preview
	0,			// deci frame num during video preview

	0,
	0,
	0,
	0,
	0,
	{SENSOR_INTERFACE_TYPE_CCIR601, 8, 16, 1},
	PNULL,
	3,			// skip frame num while change setting
};

LOCAL struct sensor_raw_info* Sensor_GetContext(void)
{
	return s_gc2235_raw_info_ptr;
}

LOCAL uint32_t Sensor_InitRawTuneInfo(void)
{
	uint32_t rtn=0x00;
	struct sensor_raw_info* raw_sensor_ptr=Sensor_GetContext();
	struct sensor_raw_tune_info* sensor_ptr=raw_sensor_ptr->tune_ptr;

	raw_sensor_ptr->version_info->version_id=0x00000000;
	raw_sensor_ptr->version_info->srtuct_size=sizeof(struct sensor_raw_info);

	//bypass
	sensor_ptr->version_id=0x00000000;
	sensor_ptr->blc_bypass=0x00;
	sensor_ptr->nlc_bypass=0x01;
	sensor_ptr->lnc_bypass=0x00;
	sensor_ptr->ae_bypass=0x00;
	sensor_ptr->awb_bypass=0x00;
	sensor_ptr->bpc_bypass=0x00;
	sensor_ptr->denoise_bypass=0x00;
	sensor_ptr->grgb_bypass=0x01;
	sensor_ptr->cmc_bypass=0x00;
	sensor_ptr->gamma_bypass=0x00;
	sensor_ptr->uvdiv_bypass=0x01;
	sensor_ptr->pref_bypass=0x00;
	sensor_ptr->bright_bypass=0x00;
	sensor_ptr->contrast_bypass=0x00;
	sensor_ptr->hist_bypass=0x01;
	sensor_ptr->auto_contrast_bypass=0x01;
	sensor_ptr->af_bypass=0x00;
	sensor_ptr->edge_bypass=0x01;
	sensor_ptr->fcs_bypass=0x00;
	sensor_ptr->css_bypass=0x01;
	sensor_ptr->saturation_bypass=0x01;
	sensor_ptr->hdr_bypass=0x01;
	sensor_ptr->glb_gain_bypass=0x01;
	sensor_ptr->chn_gain_bypass=0x01;

	//blc
	sensor_ptr->blc.mode=0x00;
	sensor_ptr->blc.offset[0].r=0x0f;
	sensor_ptr->blc.offset[0].gr=0x0f;
	sensor_ptr->blc.offset[0].gb=0x0f;
	sensor_ptr->blc.offset[0].b=0x0f;
	//nlc
	sensor_ptr->nlc.r_node[0]=0;
	sensor_ptr->nlc.r_node[1]=16;
	sensor_ptr->nlc.r_node[2]=32;
	sensor_ptr->nlc.r_node[3]=64;
	sensor_ptr->nlc.r_node[4]=96;
	sensor_ptr->nlc.r_node[5]=128;
	sensor_ptr->nlc.r_node[6]=160;
	sensor_ptr->nlc.r_node[7]=192;
	sensor_ptr->nlc.r_node[8]=224;
	sensor_ptr->nlc.r_node[9]=256;
	sensor_ptr->nlc.r_node[10]=288;
	sensor_ptr->nlc.r_node[11]=320;
	sensor_ptr->nlc.r_node[12]=384;
	sensor_ptr->nlc.r_node[13]=448;
	sensor_ptr->nlc.r_node[14]=512;
	sensor_ptr->nlc.r_node[15]=576;
	sensor_ptr->nlc.r_node[16]=640;
	sensor_ptr->nlc.r_node[17]=672;
	sensor_ptr->nlc.r_node[18]=704;
	sensor_ptr->nlc.r_node[19]=736;
	sensor_ptr->nlc.r_node[20]=768;
	sensor_ptr->nlc.r_node[21]=800;
	sensor_ptr->nlc.r_node[22]=832;
	sensor_ptr->nlc.r_node[23]=864;
	sensor_ptr->nlc.r_node[24]=896;
	sensor_ptr->nlc.r_node[25]=928;
	sensor_ptr->nlc.r_node[26]=960;
	sensor_ptr->nlc.r_node[27]=992;
	sensor_ptr->nlc.r_node[28]=1023;

	sensor_ptr->nlc.g_node[0]=0;
	sensor_ptr->nlc.g_node[1]=16;
	sensor_ptr->nlc.g_node[2]=32;
	sensor_ptr->nlc.g_node[3]=64;
	sensor_ptr->nlc.g_node[4]=96;
	sensor_ptr->nlc.g_node[5]=128;
	sensor_ptr->nlc.g_node[6]=160;
	sensor_ptr->nlc.g_node[7]=192;
	sensor_ptr->nlc.g_node[8]=224;
	sensor_ptr->nlc.g_node[9]=256;
	sensor_ptr->nlc.g_node[10]=288;
	sensor_ptr->nlc.g_node[11]=320;
	sensor_ptr->nlc.g_node[12]=384;
	sensor_ptr->nlc.g_node[13]=448;
	sensor_ptr->nlc.g_node[14]=512;
	sensor_ptr->nlc.g_node[15]=576;
	sensor_ptr->nlc.g_node[16]=640;
	sensor_ptr->nlc.g_node[17]=672;
	sensor_ptr->nlc.g_node[18]=704;
	sensor_ptr->nlc.g_node[19]=736;
	sensor_ptr->nlc.g_node[20]=768;
	sensor_ptr->nlc.g_node[21]=800;
	sensor_ptr->nlc.g_node[22]=832;
	sensor_ptr->nlc.g_node[23]=864;
	sensor_ptr->nlc.g_node[24]=896;
	sensor_ptr->nlc.g_node[25]=928;
	sensor_ptr->nlc.g_node[26]=960;
	sensor_ptr->nlc.g_node[27]=992;
	sensor_ptr->nlc.g_node[28]=1023;

	sensor_ptr->nlc.b_node[0]=0;
	sensor_ptr->nlc.b_node[1]=16;
	sensor_ptr->nlc.b_node[2]=32;
	sensor_ptr->nlc.b_node[3]=64;
	sensor_ptr->nlc.b_node[4]=96;
	sensor_ptr->nlc.b_node[5]=128;
	sensor_ptr->nlc.b_node[6]=160;
	sensor_ptr->nlc.b_node[7]=192;
	sensor_ptr->nlc.b_node[8]=224;
	sensor_ptr->nlc.b_node[9]=256;
	sensor_ptr->nlc.b_node[10]=288;
	sensor_ptr->nlc.b_node[11]=320;
	sensor_ptr->nlc.b_node[12]=384;
	sensor_ptr->nlc.b_node[13]=448;
	sensor_ptr->nlc.b_node[14]=512;
	sensor_ptr->nlc.b_node[15]=576;
	sensor_ptr->nlc.b_node[16]=640;
	sensor_ptr->nlc.b_node[17]=672;
	sensor_ptr->nlc.b_node[18]=704;
	sensor_ptr->nlc.b_node[19]=736;
	sensor_ptr->nlc.b_node[20]=768;
	sensor_ptr->nlc.b_node[21]=800;
	sensor_ptr->nlc.b_node[22]=832;
	sensor_ptr->nlc.b_node[23]=864;
	sensor_ptr->nlc.b_node[24]=896;
	sensor_ptr->nlc.b_node[25]=928;
	sensor_ptr->nlc.b_node[26]=960;
	sensor_ptr->nlc.b_node[27]=992;
	sensor_ptr->nlc.b_node[28]=1023;

	sensor_ptr->nlc.l_node[0]=0;
	sensor_ptr->nlc.l_node[1]=16;
	sensor_ptr->nlc.l_node[2]=32;
	sensor_ptr->nlc.l_node[3]=64;
	sensor_ptr->nlc.l_node[4]=96;
	sensor_ptr->nlc.l_node[5]=128;
	sensor_ptr->nlc.l_node[6]=160;
	sensor_ptr->nlc.l_node[7]=192;
	sensor_ptr->nlc.l_node[8]=224;
	sensor_ptr->nlc.l_node[9]=256;
	sensor_ptr->nlc.l_node[10]=288;
	sensor_ptr->nlc.l_node[11]=320;
	sensor_ptr->nlc.l_node[12]=384;
	sensor_ptr->nlc.l_node[13]=448;
	sensor_ptr->nlc.l_node[14]=512;
	sensor_ptr->nlc.l_node[15]=576;
	sensor_ptr->nlc.l_node[16]=640;
	sensor_ptr->nlc.l_node[17]=672;
	sensor_ptr->nlc.l_node[18]=704;
	sensor_ptr->nlc.l_node[19]=736;
	sensor_ptr->nlc.l_node[20]=768;
	sensor_ptr->nlc.l_node[21]=800;
	sensor_ptr->nlc.l_node[22]=832;
	sensor_ptr->nlc.l_node[23]=864;
	sensor_ptr->nlc.l_node[24]=896;
	sensor_ptr->nlc.l_node[25]=928;
	sensor_ptr->nlc.l_node[26]=960;
	sensor_ptr->nlc.l_node[27]=992;
	sensor_ptr->nlc.l_node[28]=1023;

	//ae
	sensor_ptr->ae.skip_frame=0x01;
	sensor_ptr->ae.normal_fix_fps=0x1e;
	sensor_ptr->ae.night_fix_fps=0x1e;
	sensor_ptr->ae.target_lum=60;
	sensor_ptr->ae.target_zone=8;
	sensor_ptr->ae.quick_mode=1;
	sensor_ptr->ae.smart=0;
	sensor_ptr->ae.smart_rotio=255;
	sensor_ptr->ae.ev[0]=0xe8;
	sensor_ptr->ae.ev[1]=0xf0;
	sensor_ptr->ae.ev[2]=0xf8;
	sensor_ptr->ae.ev[3]=0x00;
	sensor_ptr->ae.ev[4]=0x08;
	sensor_ptr->ae.ev[5]=0x10;
	sensor_ptr->ae.ev[6]=0x18;
	sensor_ptr->ae.ev[7]=0x00;
	sensor_ptr->ae.ev[8]=0x00;
	sensor_ptr->ae.ev[9]=0x00;
	sensor_ptr->ae.ev[10]=0x00;
	sensor_ptr->ae.ev[11]=0x00;
	sensor_ptr->ae.ev[12]=0x00;
	sensor_ptr->ae.ev[13]=0x00;
	sensor_ptr->ae.ev[14]=0x00;
	sensor_ptr->ae.ev[15]=0x00;

	//awb
	sensor_ptr->awb.win_start.x=0x00;
	sensor_ptr->awb.win_start.y=0x00;
	sensor_ptr->awb.win_size.w=40;
	sensor_ptr->awb.win_size.h=30;
	sensor_ptr->awb.r_gain[0]=0x1b0;
	sensor_ptr->awb.g_gain[0]=0xff;
	sensor_ptr->awb.b_gain[0]=0x180;
	sensor_ptr->awb.r_gain[1]=0x120;
	sensor_ptr->awb.g_gain[1]=0xff;
	sensor_ptr->awb.b_gain[1]=0x300;
	sensor_ptr->awb.r_gain[2]=0xff;
	sensor_ptr->awb.g_gain[2]=0xff;
	sensor_ptr->awb.b_gain[2]=0xff;
	sensor_ptr->awb.r_gain[3]=0xff;
	sensor_ptr->awb.g_gain[3]=0xff;
	sensor_ptr->awb.b_gain[3]=0xff;
	sensor_ptr->awb.r_gain[4]=0x120;
	sensor_ptr->awb.g_gain[4]=0xff;
	sensor_ptr->awb.b_gain[4]=0x200;
	sensor_ptr->awb.r_gain[5]=0x1c0;
	sensor_ptr->awb.g_gain[5]=0xff;
	sensor_ptr->awb.b_gain[5]=0x140;
	sensor_ptr->awb.r_gain[6]=0x280;
	sensor_ptr->awb.g_gain[6]=0xff;
	sensor_ptr->awb.b_gain[6]=0x130;
	sensor_ptr->awb.r_gain[7]=0xff;
	sensor_ptr->awb.g_gain[7]=0xff;
	sensor_ptr->awb.b_gain[7]=0xff;
	sensor_ptr->awb.r_gain[8]=0xff;
	sensor_ptr->awb.g_gain[8]=0xff;
	sensor_ptr->awb.b_gain[8]=0xff;
	sensor_ptr->awb.target_zone=0x40;

	/*awb cali*/
	sensor_ptr->awb.quick_mode=1;

	/*awb win*/
	sensor_ptr->awb.win[0].x=132;
	sensor_ptr->awb.win[1].x=133;
	sensor_ptr->awb.win[2].x=139;
	sensor_ptr->awb.win[3].x=158;
	sensor_ptr->awb.win[4].x=171;
	sensor_ptr->awb.win[5].x=177;
	sensor_ptr->awb.win[6].x=182;
	sensor_ptr->awb.win[7].x=188;
	sensor_ptr->awb.win[8].x=192;
	sensor_ptr->awb.win[9].x=197;
	sensor_ptr->awb.win[10].x=203;
	sensor_ptr->awb.win[11].x=209;
	sensor_ptr->awb.win[12].x=214;
	sensor_ptr->awb.win[13].x=220;
	sensor_ptr->awb.win[14].x=233;
	sensor_ptr->awb.win[15].x=251;
	sensor_ptr->awb.win[16].x=269;
	sensor_ptr->awb.win[17].x=285;
	sensor_ptr->awb.win[18].x=299;
	sensor_ptr->awb.win[19].x=312;

	sensor_ptr->awb.win[0].yt=234;
	sensor_ptr->awb.win[1].yt=257;
	sensor_ptr->awb.win[2].yt=264;
	sensor_ptr->awb.win[3].yt=266;
	sensor_ptr->awb.win[4].yt=260;
	sensor_ptr->awb.win[5].yt=237;
	sensor_ptr->awb.win[6].yt=211;
	sensor_ptr->awb.win[7].yt=184;
	sensor_ptr->awb.win[8].yt=177;
	sensor_ptr->awb.win[9].yt=175;
	sensor_ptr->awb.win[10].yt=177;
	sensor_ptr->awb.win[11].yt=176;
	sensor_ptr->awb.win[12].yt=171;
	sensor_ptr->awb.win[13].yt=163;
	sensor_ptr->awb.win[14].yt=151;
	sensor_ptr->awb.win[15].yt=145;
	sensor_ptr->awb.win[16].yt=141;
	sensor_ptr->awb.win[17].yt=134;
	sensor_ptr->awb.win[18].yt=127;
	sensor_ptr->awb.win[19].yt=113;

	sensor_ptr->awb.win[0].yb=221;
	sensor_ptr->awb.win[1].yb=193;
	sensor_ptr->awb.win[2].yb=169;
	sensor_ptr->awb.win[3].yb=130;
	sensor_ptr->awb.win[4].yb=117;
	sensor_ptr->awb.win[5].yb=115;
	sensor_ptr->awb.win[6].yb=116;
	sensor_ptr->awb.win[7].yb=122;
	sensor_ptr->awb.win[8].yb=128;
	sensor_ptr->awb.win[9].yb=135;
	sensor_ptr->awb.win[10].yb=137;
	sensor_ptr->awb.win[11].yb=135;
	sensor_ptr->awb.win[12].yb=126;
	sensor_ptr->awb.win[13].yb=112;
	sensor_ptr->awb.win[14].yb=99;
	sensor_ptr->awb.win[15].yb=84;
	sensor_ptr->awb.win[16].yb=78;
	sensor_ptr->awb.win[17].yb=76;
	sensor_ptr->awb.win[18].yb=77;
	sensor_ptr->awb.win[19].yb=92;

	//bpc
	sensor_ptr->bpc.flat_thr=80;
	sensor_ptr->bpc.std_thr=20;
	sensor_ptr->bpc.texture_thr=2;

	// denoise
	sensor_ptr->denoise.write_back=0x02;
	sensor_ptr->denoise.r_thr=0x08;
	sensor_ptr->denoise.g_thr=0x08;
	sensor_ptr->denoise.b_thr=0x08;

	sensor_ptr->denoise.diswei[0]=255;
	sensor_ptr->denoise.diswei[1]=253;
	sensor_ptr->denoise.diswei[2]=251;
	sensor_ptr->denoise.diswei[3]=249;
	sensor_ptr->denoise.diswei[4]=247;
	sensor_ptr->denoise.diswei[5]=245;
	sensor_ptr->denoise.diswei[6]=243;
	sensor_ptr->denoise.diswei[7]=241;
	sensor_ptr->denoise.diswei[8]=239;
	sensor_ptr->denoise.diswei[9]=237;
	sensor_ptr->denoise.diswei[10]=235;
	sensor_ptr->denoise.diswei[11]=234;
	sensor_ptr->denoise.diswei[12]=232;
	sensor_ptr->denoise.diswei[13]=230;
	sensor_ptr->denoise.diswei[14]=228;
	sensor_ptr->denoise.diswei[15]=226;
	sensor_ptr->denoise.diswei[16]=225;
	sensor_ptr->denoise.diswei[17]=223;
	sensor_ptr->denoise.diswei[18]=221;

	sensor_ptr->denoise.ranwei[0]=255;
	sensor_ptr->denoise.ranwei[1]=252;
	sensor_ptr->denoise.ranwei[2]=243;
	sensor_ptr->denoise.ranwei[3]=230;
	sensor_ptr->denoise.ranwei[4]=213;
	sensor_ptr->denoise.ranwei[5]=193;
	sensor_ptr->denoise.ranwei[6]=170;
	sensor_ptr->denoise.ranwei[7]=147;
	sensor_ptr->denoise.ranwei[8]=125;
	sensor_ptr->denoise.ranwei[9]=103;
	sensor_ptr->denoise.ranwei[10]=83;
	sensor_ptr->denoise.ranwei[11]=66;
	sensor_ptr->denoise.ranwei[12]=51;
	sensor_ptr->denoise.ranwei[13]=38;
	sensor_ptr->denoise.ranwei[14]=28;
	sensor_ptr->denoise.ranwei[15]=20;
	sensor_ptr->denoise.ranwei[16]=14;
	sensor_ptr->denoise.ranwei[17]=10;
	sensor_ptr->denoise.ranwei[18]=6;
	sensor_ptr->denoise.ranwei[19]=4;
	sensor_ptr->denoise.ranwei[20]=2;
	sensor_ptr->denoise.ranwei[21]=1;
	sensor_ptr->denoise.ranwei[22]=0;
	sensor_ptr->denoise.ranwei[23]=0;
	sensor_ptr->denoise.ranwei[24]=0;
	sensor_ptr->denoise.ranwei[25]=0;
	sensor_ptr->denoise.ranwei[26]=0;
	sensor_ptr->denoise.ranwei[27]=0;
	sensor_ptr->denoise.ranwei[28]=0;
	sensor_ptr->denoise.ranwei[29]=0;
	sensor_ptr->denoise.ranwei[30]=0;

	//GrGb
	sensor_ptr->grgb.edge_thr=26;
	sensor_ptr->grgb.diff_thr=80;
	//cfa
	sensor_ptr->cfa.edge_thr=0x1a;
	sensor_ptr->cfa.diff_thr=0x00;
	//cmc
	sensor_ptr->cmc.matrix[0][0]=0x771;
	sensor_ptr->cmc.matrix[0][1]=0x3cd8;
	sensor_ptr->cmc.matrix[0][2]=0x3fb8;
	sensor_ptr->cmc.matrix[0][3]=0x3dcb;
	sensor_ptr->cmc.matrix[0][4]=0x5ed;
	sensor_ptr->cmc.matrix[0][5]=0x48;
	sensor_ptr->cmc.matrix[0][6]=0x3f60;
	sensor_ptr->cmc.matrix[0][7]=0x3cde;
	sensor_ptr->cmc.matrix[0][8]=0x7c3;
	//Gamma
	sensor_ptr->gamma.axis[0][0]=0;
	sensor_ptr->gamma.axis[0][1]=8;
	sensor_ptr->gamma.axis[0][2]=16;
	sensor_ptr->gamma.axis[0][3]=24;
	sensor_ptr->gamma.axis[0][4]=32;
	sensor_ptr->gamma.axis[0][5]=48;
	sensor_ptr->gamma.axis[0][6]=64;
	sensor_ptr->gamma.axis[0][7]=80;
	sensor_ptr->gamma.axis[0][8]=96;
	sensor_ptr->gamma.axis[0][9]=128;
	sensor_ptr->gamma.axis[0][10]=160;
	sensor_ptr->gamma.axis[0][11]=192;
	sensor_ptr->gamma.axis[0][12]=224;
	sensor_ptr->gamma.axis[0][13]=256;
	sensor_ptr->gamma.axis[0][14]=288;
	sensor_ptr->gamma.axis[0][15]=320;
	sensor_ptr->gamma.axis[0][16]=384;
	sensor_ptr->gamma.axis[0][17]=448;
	sensor_ptr->gamma.axis[0][18]=512;
	sensor_ptr->gamma.axis[0][19]=576;
	sensor_ptr->gamma.axis[0][20]=640;
	sensor_ptr->gamma.axis[0][21]=768;
	sensor_ptr->gamma.axis[0][22]=832;
	sensor_ptr->gamma.axis[0][23]=896;
	sensor_ptr->gamma.axis[0][24]=960;
	sensor_ptr->gamma.axis[0][25]=1023;

	sensor_ptr->gamma.axis[1][0]=0x00;
	sensor_ptr->gamma.axis[1][1]=0x05;
	sensor_ptr->gamma.axis[1][2]=0x09;
	sensor_ptr->gamma.axis[1][3]=0x0e;
	sensor_ptr->gamma.axis[1][4]=0x13;
	sensor_ptr->gamma.axis[1][5]=0x1f;
	sensor_ptr->gamma.axis[1][6]=0x2a;
	sensor_ptr->gamma.axis[1][7]=0x36;
	sensor_ptr->gamma.axis[1][8]=0x40;
	sensor_ptr->gamma.axis[1][9]=0x58;
	sensor_ptr->gamma.axis[1][10]=0x68;
	sensor_ptr->gamma.axis[1][11]=0x76;
	sensor_ptr->gamma.axis[1][12]=0x84;
	sensor_ptr->gamma.axis[1][13]=0x8f;
	sensor_ptr->gamma.axis[1][14]=0x98;
	sensor_ptr->gamma.axis[1][15]=0xa0;
	sensor_ptr->gamma.axis[1][16]=0xb0;
	sensor_ptr->gamma.axis[1][17]=0xbd;
	sensor_ptr->gamma.axis[1][18]=0xc6;
	sensor_ptr->gamma.axis[1][19]=0xcf;
	sensor_ptr->gamma.axis[1][20]=0xd8;
	sensor_ptr->gamma.axis[1][21]=0xe4;
	sensor_ptr->gamma.axis[1][22]=0xea;
	sensor_ptr->gamma.axis[1][23]=0xf0;
	sensor_ptr->gamma.axis[1][24]=0xf6;
	sensor_ptr->gamma.axis[1][25]=0xff;

	//uv div
	sensor_ptr->uv_div.thrd[0]=252;
	sensor_ptr->uv_div.thrd[1]=250;
	sensor_ptr->uv_div.thrd[2]=248;
	sensor_ptr->uv_div.thrd[3]=246;
	sensor_ptr->uv_div.thrd[4]=244;
	sensor_ptr->uv_div.thrd[5]=242;
	sensor_ptr->uv_div.thrd[6]=240;

	//pref
	sensor_ptr->pref.write_back=0x00;
	sensor_ptr->pref.y_thr=0x04;
	sensor_ptr->pref.u_thr=0x04;
	sensor_ptr->pref.v_thr=0x04;
	//bright
	sensor_ptr->bright.factor[0]=0xd0;
	sensor_ptr->bright.factor[1]=0xe0;
	sensor_ptr->bright.factor[2]=0xf0;
	sensor_ptr->bright.factor[3]=0x00;
	sensor_ptr->bright.factor[4]=0x10;
	sensor_ptr->bright.factor[5]=0x20;
	sensor_ptr->bright.factor[6]=0x30;
	sensor_ptr->bright.factor[7]=0x00;
	sensor_ptr->bright.factor[8]=0x00;
	sensor_ptr->bright.factor[9]=0x00;
	sensor_ptr->bright.factor[10]=0x00;
	sensor_ptr->bright.factor[11]=0x00;
	sensor_ptr->bright.factor[12]=0x00;
	sensor_ptr->bright.factor[13]=0x00;
	sensor_ptr->bright.factor[14]=0x00;
	sensor_ptr->bright.factor[15]=0x00;
	//contrast
	sensor_ptr->contrast.factor[0]=0x10;
	sensor_ptr->contrast.factor[1]=0x20;
	sensor_ptr->contrast.factor[2]=0x30;
	sensor_ptr->contrast.factor[3]=0x40;
	sensor_ptr->contrast.factor[4]=0x50;
	sensor_ptr->contrast.factor[5]=0x60;
	sensor_ptr->contrast.factor[6]=0x70;
	sensor_ptr->contrast.factor[7]=0x40;
	sensor_ptr->contrast.factor[8]=0x40;
	sensor_ptr->contrast.factor[9]=0x40;
	sensor_ptr->contrast.factor[10]=0x40;
	sensor_ptr->contrast.factor[11]=0x40;
	sensor_ptr->contrast.factor[12]=0x40;
	sensor_ptr->contrast.factor[13]=0x40;
	sensor_ptr->contrast.factor[14]=0x40;
	sensor_ptr->contrast.factor[15]=0x40;
	//hist
	sensor_ptr->hist.mode;
	sensor_ptr->hist.low_ratio;
	sensor_ptr->hist.high_ratio;
	//auto contrast
	sensor_ptr->auto_contrast.mode;
	//saturation
	sensor_ptr->saturation.factor[0]=0x40;
	sensor_ptr->saturation.factor[1]=0x40;
	sensor_ptr->saturation.factor[2]=0x40;
	sensor_ptr->saturation.factor[3]=0x40;
	sensor_ptr->saturation.factor[4]=0x40;
	sensor_ptr->saturation.factor[5]=0x40;
	sensor_ptr->saturation.factor[6]=0x40;
	sensor_ptr->saturation.factor[7]=0x40;
	sensor_ptr->saturation.factor[8]=0x40;
	sensor_ptr->saturation.factor[9]=0x40;
	sensor_ptr->saturation.factor[10]=0x40;
	sensor_ptr->saturation.factor[11]=0x40;
	sensor_ptr->saturation.factor[12]=0x40;
	sensor_ptr->saturation.factor[13]=0x40;
	sensor_ptr->saturation.factor[14]=0x40;
	sensor_ptr->saturation.factor[15]=0x40;

	//af info
	sensor_ptr->af.max_step=1024;
	sensor_ptr->af.stab_period=10;

	//edge
	sensor_ptr->edge.info[0].detail_thr=0x03;
	sensor_ptr->edge.info[0].smooth_thr=0x05;
	sensor_ptr->edge.info[0].strength=10;
	sensor_ptr->edge.info[1].detail_thr=0x03;
	sensor_ptr->edge.info[1].smooth_thr=0x05;
	sensor_ptr->edge.info[1].strength=10;
	sensor_ptr->edge.info[2].detail_thr=0x03;
	sensor_ptr->edge.info[2].smooth_thr=0x05;
	sensor_ptr->edge.info[2].strength=10;
	sensor_ptr->edge.info[3].detail_thr=0x03;
	sensor_ptr->edge.info[3].smooth_thr=0x05;
	sensor_ptr->edge.info[3].strength=10;
	sensor_ptr->edge.info[4].detail_thr=0x03;
	sensor_ptr->edge.info[4].smooth_thr=0x05;
	sensor_ptr->edge.info[4].strength=10;
	sensor_ptr->edge.info[5].detail_thr=0x03;
	sensor_ptr->edge.info[5].smooth_thr=0x05;
	sensor_ptr->edge.info[5].strength=10;

	//emboss
	sensor_ptr->emboss.step=0x00;
	//global gain
	sensor_ptr->global.gain=0x40;
	//chn gain
	sensor_ptr->chn.r_gain=0x40;
	sensor_ptr->chn.g_gain=0x40;
	sensor_ptr->chn.b_gain=0x40;
	sensor_ptr->chn.r_offset=0x00;
	sensor_ptr->chn.r_offset=0x00;
	sensor_ptr->chn.r_offset=0x00;

	return rtn;
}


LOCAL uint32_t _gc2235_GetResolutionTrimTab(uint32_t param)
{
	SENSOR_LOGI("0x%x", (uint32_t)s_gc2235_Resolution_Trim_Tab);
	return (uint32_t) s_gc2235_Resolution_Trim_Tab;
}

LOCAL uint32_t _gc2235_PowerOn(uint32_t power_on)
{
	SENSOR_AVDD_VAL_E dvdd_val = g_gc2235_raw_info.dvdd_val;
	SENSOR_AVDD_VAL_E avdd_val = g_gc2235_raw_info.avdd_val;
	SENSOR_AVDD_VAL_E iovdd_val = g_gc2235_raw_info.iovdd_val;
	BOOLEAN power_down = g_gc2235_raw_info.power_down_level;
	BOOLEAN reset_level = g_gc2235_raw_info.reset_pulse_level;

	if (SENSOR_TRUE == power_on) {

		Sensor_PowerDown(!power_down);
		// Open power
		Sensor_SetMonitorVoltage(SENSOR_AVDD_2800MV);
		Sensor_SetVoltage(dvdd_val, avdd_val, iovdd_val);
		usleep(20*1000);
		//_dw9174_SRCInit(2);
		Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);
		usleep(10*1000);
		Sensor_PowerDown(power_down);
		// Reset sensor
		Sensor_Reset(reset_level);

	} else {

		Sensor_PowerDown(power_down);
		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		Sensor_SetVoltage(SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED);
		Sensor_SetMonitorVoltage(SENSOR_AVDD_CLOSED);


	}
	SENSOR_LOGI("SENSOR_GC2235: _gc2235_Power_On(1:on, 0:off): %d  ", power_on);
	return SENSOR_SUCCESS;
}

LOCAL uint32_t _gc2235_Identify(uint32_t param)
{
#define gc2235_PID_VALUE    0x22
#define gc2235_PID_ADDR     0xf0
#define gc2235_VER_VALUE    0x35
#define gc2235_VER_ADDR     0xf1

	uint8_t pid_value = 0x00;
	uint8_t ver_value = 0x00;
	uint32_t ret_value = SENSOR_FAIL;

	SENSOR_LOGI("SENSOR_GC2235:  raw identify \n");

	pid_value = Sensor_ReadReg(gc2235_PID_ADDR);

	if (gc2235_PID_VALUE == pid_value) {
		ver_value = Sensor_ReadReg(gc2235_VER_ADDR);
		SENSOR_LOGI("SENSOR_GC2235: Identify: PID = %x, VER = %x", pid_value, ver_value);
		if (gc2235_VER_VALUE == ver_value) {
			Sensor_InitRawTuneInfo();
			ret_value = SENSOR_SUCCESS;
			SENSOR_LOGI("SENSOR_GC2235: this is gc2235 sensor !");
		} else {
			SENSOR_LOGI
			    ("SENSOR_GC2235: Identify this is OV%x%x sensor !", pid_value, ver_value);
		}
	} else {
		SENSOR_LOGI("SENSOR_GC2235: identify fail,pid_value=%x", pid_value);
	}

	return ret_value;
}

LOCAL uint32_t _gc2235_write_exposure(uint32_t param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t expsure_line=0x00;
	uint16_t dummy_line=0x00;
	uint16_t value=0x00;
	uint16_t value0=0x00;
	uint16_t value1=0x00;
	uint16_t value2=0x00;

	expsure_line=param&0xffff;
	dummy_line=(param>>0x10)&0xffff;


	if (!expsure_line) expsure_line = 1; /* avoid 0 */

#ifdef GC2235_DRIVER_TRACE
	SENSORDB("GC2235_Write_Shutter iShutter = %d \n",iShutter);
#endif
	if(expsure_line < 1) expsure_line = 1;
	if(expsure_line > 8192) expsure_line = 8192;//2    ^ 13
	//Update Shutter
	ret_value = Sensor_WriteReg(0x04, (expsure_line) & 0xFF);
	ret_value = Sensor_WriteReg(0x03, (expsure_line >> 8) & 0x1F);
	return ret_value;

}

LOCAL uint32_t _gc2235_write_gain(uint32_t param)
{
	uint16_t iReg,temp;

	iReg = (uint16_t)param;
	if(256> iReg)
	{
	//analogic gain
	Sensor_WriteReg(0xb0, 0x40); // global gain
	Sensor_WriteReg(0xb1, iReg);//only digital gain 12.13
	}
	else
	{
	//analogic gain
	temp = 64*iReg/256;
	Sensor_WriteReg(0xb0, temp); // global gain
	Sensor_WriteReg(0xb1, 0xff);//only digital gain 12.13
	}
	return param;

}

LOCAL uint32_t _gc2235_SetEV(uint32_t param)
{
	uint32_t rtn = SENSOR_SUCCESS;
#if 0
	SENSOR_EXT_FUN_T_PTR ext_ptr = (SENSOR_EXT_FUN_T_PTR) param;
	uint16_t value=0x00;
	uint32_t gain = s_gc2235_gain;
	uint32_t ev = ext_ptr->param;

	SENSOR_LOGI("SENSOR: _gc2235_SetEV param: 0x%x", ev);

	gain=(gain*ext_ptr->param)>>0x06;

	value = gain&0xff;
	Sensor_WriteReg(0x350b, value);/*0-7*/
	value = (gain>>0x08)&0x03;
	Sensor_WriteReg(0x350a, value);/*8*/
#endif
	return rtn;
}

LOCAL uint32_t _gc2235_ExtFunc(uint32_t ctl_param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR) ctl_param;

	switch (ext_ptr->cmd) {
		//case SENSOR_EXT_EV:
		case 10:
			rtn = _gc2235_SetEV(ctl_param);
			break;
		default:
			break;
	}

	return rtn;
}

LOCAL uint32_t _gc2235_BeforeSnapshot(uint32_t param)
{

#if 0
	uint8_t ret_l, ret_m, ret_h;
	uint32_t capture_exposure, preview_maxline;
	uint32_t capture_maxline, preview_exposure;
	uint32_t gain = 0, value = 0;
	uint32_t prv_linetime=s_gc2235_Resolution_Trim_Tab[SENSOR_MODE_PREVIEW_ONE].line_time;
	uint32_t cap_linetime = s_gc2235_Resolution_Trim_Tab[param].line_time;

	SENSOR_LOGI("SENSOR_GC2235: BeforeSnapshot moe: %d",param);

	if (SENSOR_MODE_PREVIEW_ONE >= param){
		_gc2235_ReadGain(0x00);
		SENSOR_LOGI("SENSOR_GC2235: prvmode equal to capmode");
		return SENSOR_SUCCESS;
	}

	ret_h = (uint8_t) Sensor_ReadReg(0x3500);
	ret_m = (uint8_t) Sensor_ReadReg(0x3501);
	ret_l = (uint8_t) Sensor_ReadReg(0x3502);
	preview_exposure = ((ret_h&0x0f) << 12) + (ret_m << 4) + ((ret_l >> 4)&0x0f);

	ret_h = (uint8_t) Sensor_ReadReg(0x380e);
	ret_l = (uint8_t) Sensor_ReadReg(0x380f);
	preview_maxline = (ret_h << 8) + ret_l;

	_gc2235_ReadGain(&gain);
	Sensor_SetMode(param);

	if (prv_linetime == cap_linetime) {
		SENSOR_LOGI("SENSOR_GC2235: prvline equal to capline");
		return SENSOR_SUCCESS;
	}

	ret_h = (uint8_t) Sensor_ReadReg(0x380e);
	ret_l = (uint8_t) Sensor_ReadReg(0x380f);
	capture_maxline = (ret_h << 8) + ret_l;
	capture_exposure = preview_exposure *prv_linetime  / cap_linetime ;

	if (0 == capture_exposure) {
		capture_exposure = 1;
	}

	capture_exposure = capture_exposure * 2;
	gain=gain / 2;
	gain=gain * 320/300;

	ret_l = (unsigned char)((capture_exposure&0x0f) << 4);
	ret_m = (unsigned char)((capture_exposure&0xfff) >> 4);
	ret_h = (unsigned char)(capture_exposure >> 12)&0x0f;

	Sensor_WriteReg(0x3502, ret_l);
	Sensor_WriteReg(0x3501, ret_m);
	Sensor_WriteReg(0x3500, ret_h);

	value = gain&0xff;
	Sensor_WriteReg(0x350b, value);/*0-7*/
	value = (gain>>0x08)&0x03;
	Sensor_WriteReg(0x350a, value);/*8-9*/
	s_gc2235_gain = gain;

	Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_EXPOSURETIME, capture_exposure);
#endif
	return SENSOR_SUCCESS;
}

LOCAL uint32_t _gc2235_after_snapshot(uint32_t param)
{
	SENSOR_LOGI("SENSOR_GC2235: after_snapshot mode:%d", param);
	Sensor_SetMode(param);

	return SENSOR_SUCCESS;
}

LOCAL uint32_t _gc2235_StreamOn(uint32_t param)
{
	SENSOR_LOGI("SENSOR_GC2235: StreamOn");

	//Sensor_WriteReg(0x0100, 0x01);

	return 0;
}

LOCAL uint32_t _gc2235_StreamOff(uint32_t param)
{
	SENSOR_LOGI("SENSOR_GC2235: StreamOff");

	//Sensor_WriteReg(0x0100, 0x00);

	return 0;
}
