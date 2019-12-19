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
 * V1.0
 */
 /*History
 *Date                  Modification                                 Reason
 *
 */

#ifndef _SENSOR_GC8034_MIPI_RAW_H_
#define _SENSOR_GC8034_MIPI_RAW_H_


#include <utils/Log.h>
#include "sensor.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"

//#include "parameters/sensor_gc8034_raw_param_main.c"

//#define FEATURE_OTP

#define VENDOR_NUM 1
#define SENSOR_NAME				"gc8034_mipi_raw"

#define I2C_SLAVE_ADDR			0x6e 		/* 8bit slave address*/

#define GC8034_PID_ADDR			0xf0
#define GC8034_PID_VALUE		0x80
#define GC8034_VER_ADDR			0xf1
#define GC8034_VER_VALUE		0x44

/* sensor parameters begin */

/* effective sensor output image size */
#define PREVIEW_WIDTH			1632	
#define PREVIEW_HEIGHT			1224
#define SNAPSHOT_WIDTH			3264	
#define SNAPSHOT_HEIGHT			2448

/*Raw Trim parameters*/
#define PREVIEW_TRIM_X			0
#define PREVIEW_TRIM_Y			0
#define PREVIEW_TRIM_W			1632
#define PREVIEW_TRIM_H			1224
#define SNAPSHOT_TRIM_X			0
#define SNAPSHOT_TRIM_Y			0
#define SNAPSHOT_TRIM_W			3264	
#define SNAPSHOT_TRIM_H			2448

/*Mipi output*/
#define LANE_NUM			2
#define RAW_BITS			10

#define PREVIEW_MIPI_PER_LANE_BPS	  672  /* 2*Mipi clk */
#define SNAPSHOT_MIPI_PER_LANE_BPS	  1171  /* 2*Mipi clk */

/*line time unit: 1ns*/
#define PREVIEW_LINE_TIME		  13350
#define SNAPSHOT_LINE_TIME		  14833

/* frame length*/
#define PREVIEW_FRAME_LENGTH		2500
#define SNAPSHOT_FRAME_LENGTH		2500

/* please ref your spec */
#define FRAME_OFFSET			16
#define SENSOR_MAX_GAIN			0x400
#define SENSOR_BASE_GAIN		0x40
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
/* sensor parameters end */

/* isp parameters, please don't change it*/
#define ISP_BASE_GAIN			0x80

/* please don't change it */
#define EX_MCLK				24

#undef GC8034_MIRROR_NORMAL
#undef GC8034_MIRROR_H
#undef GC8034_MIRROR_V
#undef GC8034_MIRROR_HV
/* If you use the otp function, keep the otp_drv -> gc8034_common_otp_drv.h consistent.*/
#define GC8034_MIRROR_NORMAL

#if defined(GC8034_MIRROR_NORMAL)
	#define GC8034_MIRROR	0xc0
	#define BINNING_STARTY  0x04
	#define BINNING_STARTX  0x05
	#define FULL_STARTY 	0x08
	#define FULL_STARTX 	0x09
#elif defined(GC8034_MIRROR_H)
	#define GC8034_MIRROR	0xc1
	#define BINNING_STARTY  0x04
	#define BINNING_STARTX  0x05
	#define FULL_STARTY 	0x08
	#define FULL_STARTX 	0x0b
#elif defined(GC8034_MIRROR_V)
	#define GC8034_MIRROR	0xc2
	#define BINNING_STARTY  0x04
	#define BINNING_STARTX  0x05
	#define FULL_STARTY 	0x08
	#define FULL_STARTX 	0x09
#elif defined(GC8034_MIRROR_HV)
	#define GC8034_MIRROR	0xc3
	#define BINNING_STARTY  0x04
	#define BINNING_STARTX  0x05
	#define FULL_STARTY 	0x08
	#define FULL_STARTX 	0x0b
#else
	#define GC8034_MIRROR	0xc0
	#define BINNING_STARTY  0x04
	#define BINNING_STARTX  0x05
	#define FULL_STARTY 	0x08
	#define FULL_STARTX 	0x09
#endif

static cmr_u8 Binning_or_Fullsize = 1; /* 0:Binning,1:Fullsize */

struct gc_register {
	cmr_u8 addr;
	cmr_u8 value[2];
};

#define AG_INDEX 7
#define AGC_REG_NUM 14
struct agc_params_struct {
	cmr_u16	gain_level;
	struct gc_register	agc_register[AGC_REG_NUM];
};

static struct agc_params_struct GC8034_AGC_Param[AG_INDEX] = {
	{
		64, /* 1.00x */
		{	/* addr	binning	fullsize */
			{0xfe, {0x00, 0x00}},
			{0x20, {0x55, 0x54}},
			{0x33, {0x83, 0x82}},
			{0xfe, {0x01, 0x01}},
			{0xdf, {0x06, 0x06}},
			{0xe7, {0x18, 0x18}},
			{0xe8, {0x20, 0x20}},
			{0xe9, {0x16, 0x16}},
			{0xea, {0x17, 0x17}},
			{0xeb, {0x50, 0x50}},
			{0xec, {0x6c, 0x6c}},
			{0xed, {0x9b, 0x9b}},
			{0xee, {0xd8, 0xd8}},
			{0xfe, {0x00, 0x00}}
		}
	},
	{
		88, /* 1.38x */
		{
			{0xfe, {0x00, 0x00}},
			{0x20, {0x55, 0x54}},
			{0x33, {0x83, 0x82}},
			{0xfe, {0x01, 0x01}},
			{0xdf, {0x06, 0x06}},
			{0xe7, {0x18, 0x18}},
			{0xe8, {0x20, 0x20}},
			{0xe9, {0x16, 0x16}},
			{0xea, {0x17, 0x17}},
			{0xeb, {0x50, 0x50}},
			{0xec, {0x6c, 0x6c}},
			{0xed, {0x9b, 0x9b}},
			{0xee, {0xd8, 0xd8}},
			{0xfe, {0x00, 0x00}}
		}
	},
	{
		125, /* 1.95x */
		{
			{0xfe, {0x00, 0x00}},
			{0x20, {0x4e, 0x4e}},
			{0x33, {0x84, 0x83}},
			{0xfe, {0x01, 0x01}},
			{0xdf, {0x0c, 0x0c}},
			{0xe7, {0x2e, 0x2e}},
			{0xe8, {0x2d, 0x2d}},
			{0xe9, {0x15, 0x15}},
			{0xea, {0x19, 0x19}},
			{0xeb, {0x47, 0x47}},
			{0xec, {0x70, 0x70}},
			{0xed, {0x9f, 0x9f}},
			{0xee, {0xd8, 0xd8}},
			{0xfe, {0x00, 0x00}}
		}
	},
	{
		173, /* 2.70x */
		{
			{0xfe, {0x00, 0x00}},
			{0x20, {0x51, 0x51}},
			{0x33, {0x80, 0x80}},
			{0xfe, {0x01, 0x01}},
			{0xdf, {0x07, 0x07}},
			{0xe7, {0x28, 0x28}},
			{0xe8, {0x32, 0x32}},
			{0xe9, {0x22, 0x22}},
			{0xea, {0x20, 0x20}},
			{0xeb, {0x49, 0x49}},
			{0xec, {0x70, 0x70}},
			{0xed, {0x91, 0x91}},
			{0xee, {0xd9, 0xd9}},
			{0xfe, {0x00, 0x00}}
		}
	},
	{
		243, /* 3.80x */
		{
			{0xfe, {0x00, 0x00}},
			{0x20, {0x4d, 0x4c}},
			{0x33, {0x83, 0x82}},
			{0xfe, {0x01, 0x01}},
			{0xdf, {0x0f, 0x0f}},
			{0xe7, {0x3b, 0x3b}},
			{0xe8, {0x3b, 0x3b}},
			{0xe9, {0x1c, 0x1c}},
			{0xea, {0x1f, 0x1f}},
			{0xeb, {0x47, 0x47}},
			{0xec, {0x6f, 0x6f}},
			{0xed, {0x9b, 0x9b}},
			{0xee, {0xd3, 0xd3}},
			{0xfe, {0x00, 0x00}}
		}
	},
	{
		345, /* 5.40x */
		{
			{0xfe, {0x00, 0x00}},
			{0x20, {0x50, 0x4c}},
			{0x33, {0x83, 0x82}},
			{0xfe, {0x01, 0x01}},
			{0xdf, {0x08, 0x0f}},
			{0xe7, {0x35, 0x3b}},
			{0xe8, {0x46, 0x3b}},
			{0xe9, {0x1e, 0x1c}},
			{0xea, {0x22, 0x1f}},
			{0xeb, {0x4c, 0x47}},
			{0xec, {0x70, 0x6f}},
			{0xed, {0x9a, 0x9b}},
			{0xee, {0xd2, 0xd3}},
			{0xfe, {0x00, 0x00}}
		}
	},
	{
		490, /* 7.66x */
		{
			{0xfe, {0x00, 0x00}},
			{0x20, {0x52, 0x51}},
			{0x33, {0x80, 0x7f}},
			{0xfe, {0x01, 0x01}},
			{0xdf, {0x0c, 0x0c}},
			{0xe7, {0x35, 0x35}},
			{0xe8, {0x3a, 0x3a}},
			{0xe9, {0x2b, 0x2b}},
			{0xea, {0x2d, 0x2d}},
			{0xeb, {0x4c, 0x4c}},
			{0xec, {0x67, 0x67}},
			{0xed, {0x8d, 0x8d}},
			{0xee, {0xc0, 0xc0}},
			{0xfe, {0x00, 0x00}}
		}
	}
};

#define DD_WIDTH            3284
#define DD_HEIGHT           2464

struct gc8034_dd_t { 
	cmr_u16 x;
	cmr_u16 y;
	cmr_u16 t;
};

struct gc8034_otp_t {
	cmr_u8  dd_cnt;
	cmr_u8  dd_flag;
	struct gc8034_dd_t dd_param[160];
	cmr_u8  reg_flag;
	cmr_u8  reg_num;
	cmr_u8  reg_page[10];
	cmr_u8  reg_addr[10];
	cmr_u8  reg_value[10];
	cmr_u8  product_level;
};

static struct gc8034_otp_t gc8034_otp_info;

static cmr_u32 Dgain_ratio = 256;

/*==============================================================================
 * Description:
 * register setting
 *============================================================================*/

static const SENSOR_REG_T gc8034_init_setting[] = {
	/*SYS*/
	{0xf2,0x00},
	{0xf4,0x80},
	{0xf5,0x19},
	{0xf6,0x44},
	{0xf8,0x63},
	{0xfa,0x45},
	{0xf9,0x00},
	{0xf7,0x95},
	{0xfc,0x00},
	{0xfc,0x00},
	{0xfc,0xea},
	{0xfe,0x03},
	{0x03,0x9a},
	{0xfc,0xee},
	{0xfe,0x00},
	{0x88,0x03},
	           
	/*Cisctl&Analog*/
	{0xfe,0x00},
	{0x03,0x08},
	{0x04,0xc6},
	{0x05,0x02},
	{0x06,0x16},
	{0x07,0x00},
	{0x08,0x10},
	{0x0a,0x3a}, //row start
	{0x0b,0x00},
	{0x0c,0x04}, //col start
	{0x0d,0x09},
	{0x0e,0xa0}, //win_height 2464
	{0x0f,0x0c},
	{0x10,0xd4}, //win_width 3284
	{0x17,GC8034_MIRROR},
	{0x18,0x02},
	{0x19,0x17},
	{0x1e,0x50},
	{0x1f,0x80},
	{0x21,0x4c},
	{0x25,0x00},
	{0x28,0x4a},
	{0x2d,0x89},
	{0xca,0x02},
	{0xcb,0x00},
	{0xcc,0x39},
	{0xce,0xd0},
	{0xcf,0x93},
	{0xd0,0x1b},
	{0xd1,0xaa},
	{0xd8,0x40},
	{0xd9,0xff},
	{0xda,0x0e},
	{0xdb,0xb0},
	{0xdc,0x0e},
	{0xde,0x08},
	{0xe4,0xc6},
	{0xe5,0x08},
	{0xe6,0x10},
	{0xed,0x2a},
	{0xfe,0x02},
	{0x59,0x02},
	{0x5a,0x04},
	{0x5b,0x08},
	{0x5c,0x20},
	{0xfe,0x00},
	{0x1a,0x09},
	{0x1d,0x13},
	{0xfe,0x10},
	{0xfe,0x00},
	{0xfe,0x10},
	{0xfe,0x00},
           
	/*Gamma*/
	{0xfe,0x00},
	{0x20,0x55},
	{0x33,0x83},
	{0xfe,0x01},
	{0xdf,0x06},
	{0xe7,0x18},
	{0xe8,0x20},
	{0xe9,0x16},
	{0xea,0x17},
	{0xeb,0x50},
	{0xec,0x6c},
	{0xed,0x9b},
	{0xee,0xd8},
	          
	/*ISP*/
	{0xfe,0x00},
	{0x80,0x10},
	{0x84,0x01},
	{0x89,0x03},
	{0x8d,0x03},
	{0x8f,0x14},
	{0xad,0x30},
	{0x66,0x2c},
	{0xbc,0x49},
	{0xc2,0x7f},
	{0xc3,0xff},
	            
	/*Crop window*/
	{0x90,0x01},
	{0x92,BINNING_STARTY}, //crop y
	{0x94,BINNING_STARTX}, //crop x
	{0x95,0x04},
	{0x96,0xc8},
	{0x97,0x06},
	{0x98,0x60},
	
	/*Gain*/
	{0xb0,0x90},
	{0xb1,0x01},
	{0xb2,0x00},
	{0xb6,0x00},
	           
	/*BLK*/
	{0xfe,0x00},
	{0x40,0x22},
	{0x41,0x20},
	{0x42,0x02},	
	{0x43,0x08},
	{0x4e,0x0f},
	{0x4f,0xf0},
	{0x58,0x80},
	{0x59,0x80},
	{0x5a,0x80},
	{0x5b,0x80},
	{0x5c,0x00},
	{0x5d,0x00},
	{0x5e,0x00},
	{0x5f,0x00},
	{0x6b,0x01},
	{0x6c,0x00},
	{0x6d,0x0c},
		           
	/*WB offset*/
	{0xfe,0x01},
	{0xbf,0x40},
	           
	/*Dark Sun*/
	{0xfe,0x01},
	{0x68,0x77},
	          
	/*DPC*/
	{0xfe,0x01},
	{0x60,0x00},
	{0x61,0x10},
	{0x62,0x60},
	{0x63,0x30},
	{0x64,0x00},
	         
	/*LSC*/
	{0xfe,0x01},
	{0xa8,0x60},
	{0xa2,0xd1},
	{0xc8,0x57},
	{0xa1,0xb8},
	{0xa3,0x91},
	{0xc0,0x50},
	{0xd0,0x05},
	{0xd1,0xb2},
	{0xd2,0x1f},
	{0xd3,0x00},
	{0xd4,0x00},
	{0xd5,0x00},
	{0xd6,0x00},
	{0xd7,0x00},
	{0xd8,0x00},
	{0xd9,0x00},
	{0xa4,0x10},
	{0xa5,0x20},
	{0xa6,0x60},
	{0xa7,0x80},
	{0xab,0x18},
	{0xc7,0xc0},
	           
	/*ABB*/
	{0xfe,0x01},
	{0x20,0x02},
	{0x21,0x02},
	{0x23,0x42},
	          
	/*MIPI*/
	{0xfe,0x03},
	{0x01,0x07},
	{0x02,0x03},
	{0x04,0x80},
	{0x11,0x2b},
	{0x12,0xf8},
	{0x13,0x07},
	{0x15,0x10},
	{0x16,0x29},
	{0x17,0xff},
	{0x18,0x01},
	{0x19,0xaa},
	{0x1a,0x02},
	{0x21,0x05},
	{0x22,0x06},
	{0x23,0x16},
	{0x24,0x00},
	{0x25,0x12},
	{0x26,0x07},
	{0x29,0x07},
	{0x2a,0x08},
	{0x2b,0x07},
	{0xfe,0x00},
	{0x3f,0x00},
};

static const SENSOR_REG_T gc8034_preview_setting[] = {
	/*SYS*/
	{0xf2,0x00},
	{0xf4,0x80},
	{0xf5,0x19},
	{0xf6,0x44},
	{0xf8,0x63},
	{0xfa,0x45},
	{0xf9,0x00},
	{0xf7,0x95},
	{0xfc,0x00},
	{0xfc,0x00},
	{0xfc,0xea},
	{0xfe,0x03},
	{0x03,0x9a},
	{0xfc,0xee},
	{0xfe,0x10},
	{0xfe,0x00},
	{0xfe,0x10},
	{0xfe,0x00},
	
	/*ISP*/
	{0xfe,0x00},
	{0x80,0x10},
	{0xad,0x30},
	{0x66,0x2c},
	{0xbc,0x49},

	/*Crop window*/
	{0x90,0x01},
	{0x92,BINNING_STARTY}, //crop y
	{0x94,BINNING_STARTX}, //crop x
	{0x95,0x04},
	{0x96,0xc8},
	{0x97,0x06},
	{0x98,0x60},

	/*MIPI*/
	{0xfe,0x03},
	{0x01,0x07},
	{0x02,0x03},
	{0x04,0x80},
	{0x11,0x2b},
	{0x12,0xf8},
	{0x13,0x07},
	{0x15,0x10},
	{0x16,0x29},
	{0x17,0xff},
	{0x18,0x01},
	{0x19,0xaa},
	{0x1a,0x02},
	{0x21,0x05},
	{0x22,0x06},
	{0x23,0x16},
	{0x24,0x00},
	{0x25,0x12},
	{0x26,0x07},
	{0x29,0x07},
	{0x2a,0x08},
	{0x2b,0x07},
	{0xfe,0x00},
	{0x3f,0x00},
};

static const SENSOR_REG_T gc8034_snapshot_setting[] = {
	/*SYS*/
	{0xf2,0x00},
	{0xf4,0x90},
	{0xf5,0x3d},
	{0xf6,0x44},
	{0xf8,0x59},
	{0xfa,0x3c},
	{0xf9,0x00},
	{0xf7,0x95},
	{0xfc,0x00},
	{0xfc,0x00},
	{0xfc,0xea},
	{0xfe,0x03},
	{0x03,0x9a},
	{0xfc,0xee},
	{0xfe,0x10},
	{0xfe,0x00},
	{0xfe,0x10},
	{0xfe,0x00},
	          
	/*ISP*/
	{0xfe,0x00},
	{0x80,0x13},
	{0xad,0x00},
	            
	/*Crop window*/
	{0x90,0x01},
	{0x92,FULL_STARTY}, //crop y
	{0x94,FULL_STARTX}, //crop x
	{0x95,0x09},
	{0x96,0x90},
	{0x97,0x0c},
	{0x98,0xc0},
	
	/*MIPI*/
	{0xfe,0x03},
	{0x01,0x07},
	{0x02,0x04},
	{0x04,0x80},
	{0x11,0x2b},
	{0x12,0xf0},
	{0x13,0x0f},
	{0x15,0x10},
	{0x16,0x29},
	{0x17,0xff},
	{0x18,0x01},
	{0x19,0xaa},
	{0x1a,0x02},
	{0x21,0x0c},
	{0x22,0x0e},
	{0x23,0x45},
	{0x24,0x01},
	{0x25,0x1c},
	{0x26,0x0b},
	{0x29,0x0e},
	{0x2a,0x1d},
	{0x2b,0x0b},
	{0xfe,0x00},
	{0x3f,0x00},
};

static struct sensor_res_tab_info s_gc8034_resolution_tab_raw[VENDOR_NUM] = {
	{
      .module_id = MODULE_SUNNY,
      .reg_tab = {
        {ADDR_AND_LEN_OF_ARRAY(gc8034_init_setting), PNULL, 0,
        .width = 0, .height = 0,
        .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW},

		
        {ADDR_AND_LEN_OF_ARRAY(gc8034_preview_setting), PNULL, 0,
        .width = PREVIEW_WIDTH, .height = PREVIEW_HEIGHT,
        .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW},
        
        {ADDR_AND_LEN_OF_ARRAY(gc8034_snapshot_setting), PNULL, 0,
        .width = SNAPSHOT_WIDTH, .height = SNAPSHOT_HEIGHT,
        .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW}
		}
	}

	/*If there are multiple modules,please add here*/
};

static SENSOR_TRIM_T s_gc8034_resolution_trim_tab[VENDOR_NUM] = {
	{
     .module_id = MODULE_SUNNY,
     .trim_info = {
       {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	   
	   
	   {.trim_start_x = PREVIEW_TRIM_X, .trim_start_y = PREVIEW_TRIM_Y,
        .trim_width = PREVIEW_TRIM_W,   .trim_height = PREVIEW_TRIM_H,
        .line_time = PREVIEW_LINE_TIME, .bps_per_lane = PREVIEW_MIPI_PER_LANE_BPS,
        .frame_line = PREVIEW_FRAME_LENGTH,
        .scaler_trim = {.x = PREVIEW_TRIM_X, .y = PREVIEW_TRIM_Y, .w = PREVIEW_TRIM_W, .h = PREVIEW_TRIM_H}},
       
	   {
        .trim_start_x = SNAPSHOT_TRIM_X, .trim_start_y = SNAPSHOT_TRIM_Y,
        .trim_width = SNAPSHOT_TRIM_W,   .trim_height = SNAPSHOT_TRIM_H,
        .line_time = SNAPSHOT_LINE_TIME, .bps_per_lane = SNAPSHOT_MIPI_PER_LANE_BPS,
        .frame_line = SNAPSHOT_FRAME_LENGTH,
        .scaler_trim = {.x = SNAPSHOT_TRIM_X, .y = SNAPSHOT_TRIM_Y, .w = SNAPSHOT_TRIM_W, .h = SNAPSHOT_TRIM_H}},
       }
	}

    /*If there are multiple modules,please add here*/

};

static SENSOR_REG_T gc8034_shutter_reg[] = {
    {0xfe, 0x00}, 
	{0x04, 0xc6}, 
	{0x03, 0x08},
};

static struct sensor_i2c_reg_tab gc8034_shutter_tab = {
    .settings = gc8034_shutter_reg, 
	.size = ARRAY_SIZE(gc8034_shutter_reg),
};

static SENSOR_REG_T gc8034_again_reg[] = {
    {0xfe, 0x00}, 
	{0x20, 0x55},
	{0x33, 0x83},
	{0xfe, 0x01},
	{0xdf, 0x06},
	{0xe7, 0x18},
	{0xe8, 0x20},
	{0xe9, 0x16},
	{0xea, 0x17},
	{0xeb, 0x50},
	{0xec, 0x6c},
	{0xed, 0x9b},
	{0xee, 0xd8},
	{0xfe, 0x00},
	{0xb6, 0x00}, 
	{0xb1, 0x01},
    {0xb2, 0x00}, 
};

static struct sensor_i2c_reg_tab gc8034_again_tab = {
    .settings = gc8034_again_reg, 
	.size = ARRAY_SIZE(gc8034_again_reg),
};

static SENSOR_REG_T gc8034_dgain_reg[] = {
   
};

static struct sensor_i2c_reg_tab gc8034_dgain_tab = {
    .settings = gc8034_dgain_reg, 
	.size = ARRAY_SIZE(gc8034_dgain_reg),
};

static SENSOR_REG_T gc8034_frame_length_reg[] = {
	{0xfe, 0x00}, 
    {0x07, 0x00}, 
    {0x08, 0x10}, 
};

static struct sensor_i2c_reg_tab gc8034_frame_length_tab = {
    .settings = gc8034_frame_length_reg,
    .size = ARRAY_SIZE(gc8034_frame_length_reg),
};

static struct sensor_aec_i2c_tag gc8034_aec_info = {
    .slave_addr = (I2C_SLAVE_ADDR >> 1),
    .addr_bits_type = SENSOR_I2C_REG_8BIT,
    .data_bits_type = SENSOR_I2C_VAL_8BIT,
    .shutter = &gc8034_shutter_tab,
    .again = &gc8034_again_tab,
    .dgain = &gc8034_dgain_tab,
    .frame_length = &gc8034_frame_length_tab,
};


static SENSOR_STATIC_INFO_T s_gc8034_static_info[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .static_info = {
        .f_num = 200,
        .focal_length = 354,
        .max_fps = 30,
        .max_adgain = 16,
        .ois_supported = 0,
        .pdaf_supported = 0,
        .exp_valid_frame_num = 1,
        .clamp_level = 64,
        .adgain_valid_frame_num = 1,
        .fov_info = {{4.614f, 3.444f}, 4.222f}}
    }
    /*If there are multiple modules,please add here*/
};


static SENSOR_MODE_FPS_INFO_T s_gc8034_mode_fps_info[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
       {.is_init = 0,
         {{SENSOR_MODE_COMMON_INIT, 0, 1, 0, 0},
         {SENSOR_MODE_PREVIEW_ONE, 0, 1, 0, 0},
         {SENSOR_MODE_SNAPSHOT_ONE_FIRST, 0, 1, 0, 0},
         {SENSOR_MODE_SNAPSHOT_ONE_SECOND, 0, 1, 0, 0},
         {SENSOR_MODE_SNAPSHOT_ONE_THIRD, 0, 1, 0, 0},
         {SENSOR_MODE_PREVIEW_TWO, 0, 1, 0, 0},
         {SENSOR_MODE_SNAPSHOT_TWO_FIRST, 0, 1, 0, 0},
         {SENSOR_MODE_SNAPSHOT_TWO_SECOND, 0, 1, 0, 0},
         {SENSOR_MODE_SNAPSHOT_TWO_THIRD, 0, 1, 0, 0}}}
    }
    /*If there are multiple modules,please add here*/
};


static struct sensor_module_info s_gc8034_module_info_tab[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .module_info = {
         .major_i2c_addr = I2C_SLAVE_ADDR >> 1,
         .minor_i2c_addr = I2C_SLAVE_ADDR >> 1,

         .reg_addr_value_bits = SENSOR_I2C_REG_8BIT | SENSOR_I2C_VAL_8BIT |
                                SENSOR_I2C_FREQ_400,

         .avdd_val = SENSOR_AVDD_2800MV,
         .iovdd_val = SENSOR_AVDD_1800MV,
         .dvdd_val = SENSOR_AVDD_1200MV,

#if defined(GC8034_MIRROR_NORMAL)
		 .image_pattern = SENSOR_IMAGE_PATTERN_RAWRGB_R,
#elif defined(GC8034_MIRROR_H)
		 .image_pattern = SENSOR_IMAGE_PATTERN_RAWRGB_GR,
#elif defined(GC8034_MIRROR_V)
		 .image_pattern = SENSOR_IMAGE_PATTERN_RAWRGB_GB,
#elif defined(GC8034_MIRROR_HV)
		 .image_pattern = SENSOR_IMAGE_PATTERN_RAWRGB_B,
#else 
		 .image_pattern = SENSOR_IMAGE_PATTERN_RAWRGB_R,
#endif

         .preview_skip_num = 1,
         .capture_skip_num = 1,
         .flash_capture_skip_num = 3,
         .mipi_cap_skip_num = 0,
         .preview_deci_num = 0,
         .video_preview_deci_num = 0,

         .threshold_eb = 0,
         .threshold_mode = 0,
         .threshold_start = 0,
         .threshold_end = 0,

         .sensor_interface = {
              .type = SENSOR_INTERFACE_TYPE_CSI2,
              .bus_width = LANE_NUM,
              .pixel_width = RAW_BITS,
              .is_loose = 0,
          },
         .change_setting_skip_num = 1,
         .horizontal_view_angle = 65,
         .vertical_view_angle = 60
      }
    }

/*If there are multiple modules,please add here*/
};

static struct sensor_ic_ops s_gc8034_ops_tab;
struct sensor_raw_info *s_gc8034_mipi_raw_info_ptr = PNULL;


/*==============================================================================
 * Description:
 * sensor all info
 * please modify this variable acording your spec
 *============================================================================*/
SENSOR_INFO_T g_gc8034_mipi_raw_info = {
    .hw_signal_polarity = SENSOR_HW_SIGNAL_PCLK_P | SENSOR_HW_SIGNAL_VSYNC_P |
                          SENSOR_HW_SIGNAL_HSYNC_P,
    .environment_mode = SENSOR_ENVIROMENT_NORMAL | SENSOR_ENVIROMENT_NIGHT,
    .image_effect = SENSOR_IMAGE_EFFECT_NORMAL |
                    SENSOR_IMAGE_EFFECT_BLACKWHITE | SENSOR_IMAGE_EFFECT_RED |
                    SENSOR_IMAGE_EFFECT_GREEN | SENSOR_IMAGE_EFFECT_BLUE |
                    SENSOR_IMAGE_EFFECT_YELLOW | SENSOR_IMAGE_EFFECT_NEGATIVE |
                    SENSOR_IMAGE_EFFECT_CANVAS,

    .wb_mode = 0,
    .step_count = 7,
    .reset_pulse_level = SENSOR_LOW_PULSE_RESET,
    .reset_pulse_width = 50,
    .power_down_level = SENSOR_LOW_LEVEL_PWDN,
    .identify_count = 1,
    .identify_code =
        {{ .reg_addr = GC8034_PID_ADDR, .reg_value = GC8034_PID_VALUE},
         { .reg_addr = GC8034_VER_ADDR, .reg_value = GC8034_VER_VALUE}},

    .source_width_max = SNAPSHOT_WIDTH,
    .source_height_max = SNAPSHOT_HEIGHT,
    .name = (cmr_s8 *)SENSOR_NAME,
    .image_format = SENSOR_IMAGE_FORMAT_RAW,

    .module_info_tab = s_gc8034_module_info_tab,
    .module_info_tab_size = ARRAY_SIZE(s_gc8034_module_info_tab),

    .resolution_tab_info_ptr = s_gc8034_resolution_tab_raw,
    .sns_ops = &s_gc8034_ops_tab,
    .raw_info_ptr = &s_gc8034_mipi_raw_info_ptr,

    .video_tab_info_ptr = NULL,
    .sensor_version_info = (cmr_s8 *)"gc8034_v1",
};

#endif
