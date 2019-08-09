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

#ifndef _SENSOR_GC5035_MIPI_RAW_H_
#define _SENSOR_GC5035_MIPI_RAW_H_


#include <utils/Log.h>
#include "sensor.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"

#define VENDOR_NUM 1
#define SENSOR_NAME			"gc5035"
#define I2C_SLAVE_ADDR			0x6e   /* 8bit slave address*/

#define GC5035_PID_ADDR			0xf0
#define GC5035_PID_VALUE		0x50
#define GC5035_VER_ADDR			0xf1
#define GC5035_VER_VALUE		0x35

/* sensor parameters begin */
/* effective sensor output image size */
#define PREVIEW_WIDTH			2592
#define PREVIEW_HEIGHT			1944
#define SNAPSHOT_WIDTH			2592 
#define SNAPSHOT_HEIGHT			1944

/*Raw Trim parameters*/
#define PREVIEW_TRIM_X			0
#define PREVIEW_TRIM_Y			0
#define PREVIEW_TRIM_W			2592
#define PREVIEW_TRIM_H			1944
#define SNAPSHOT_TRIM_X			0
#define SNAPSHOT_TRIM_Y			0
#define SNAPSHOT_TRIM_W			2592
#define SNAPSHOT_TRIM_H			1944

/*Mipi output*/
#define LANE_NUM			2
#define RAW_BITS			10

#define PREVIEW_MIPI_PER_LANE_BPS	  876  /* 2*Mipi clk */
#define SNAPSHOT_MIPI_PER_LANE_BPS	  876  /* 2*Mipi clk */

/*line time unit: 1ns*/
#define PREVIEW_LINE_TIME		  16667
#define SNAPSHOT_LINE_TIME		  16667

/* frame length*/
#define SNAPSHOT_FRAME_LENGTH		2008
#define PREVIEW_FRAME_LENGTH		2008

/* please ref your spec */
#define FRAME_OFFSET			16
#define SENSOR_MAX_GAIN			0x1000
#define SENSOR_BASE_GAIN		0x100
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

#undef GC5035_MIRROR_NORMAL
#undef GC5035_MIRROR_H
#undef GC5035_MIRROR_V
#undef GC5035_MIRROR_HV
#define GC5035_MIRROR_NORMAL

#if defined(GC5035_MIRROR_NORMAL)
	#define GC5035_MIRROR	 0x80
	#define GC5035_RSTDUMMY1 0x02
	#define GC5035_RSTDUMMY2 0x7c
#elif defined(GC5035_MIRROR_H)
	#define GC5035_MIRROR	 0x81
	#define GC5035_RSTDUMMY1 0x02
	#define GC5035_RSTDUMMY2 0x7c
#elif defined(GC5035_MIRROR_V)
	#define GC5035_MIRROR	 0x82
	#define GC5035_RSTDUMMY1 0x03
	#define GC5035_RSTDUMMY2 0xfc
#elif defined(GC5035_MIRROR_HV)
	#define GC5035_MIRROR	 0x83
	#define GC5035_RSTDUMMY1 0x03
	#define GC5035_RSTDUMMY2 0xfc
#else
	#define GC5035_MIRROR	 0x80
	#define GC5035_RSTDUMMY1 0x02
	#define GC5035_RSTDUMMY2 0x7c
#endif


/* Gain */
#define E_GAIN_INDEX     17
#define MAX_GAIN_INDEX   17

static cmr_u32 Dgain_ratio = 256;

/*sensor otp*/
#define DPC_FLAG_OFFSET 0x0068
#define DPC_TOTAL_NUMBER_OFFSET 0x0070
#define DPC_ERROR_NUMBER_OFFSET 0x0078
#define REG_INFO_FLAG_OFFSET 0x0880
#define REG_INFO_PAGE_OFFSET 0x0888
#define REG_INFO_ADDR_OFFSET 0x0890
#define REG_INFO_VALUE_OFFSET 0x0898
#define REG_INFO_SIZE 5

struct gc5035_otp {
	cmr_u8  dd_flag;
	cmr_u16 dd_num;
	cmr_u8  reg_flag;
	cmr_u8  reg_num;
	cmr_u8  regs[10][3];
};

static struct gc5035_otp gc5035_otp_info;

/*==============================================================================
 * Description:
 * register setting
 *============================================================================*/

static const SENSOR_REG_T gc5035_init_setting[] = {
	/*SYS*/   
	{0xfc,0x01},
	{0xf4,0x40},
	{0xf5,0xe9},
	{0xf6,0x14},
	{0xf8,0x49},
	{0xf9,0x82},
	{0xfa,0x00},
	{0xfc,0x81},
	{0xfe,0x00},
	{0x36,0x01},
	{0xd3,0x87},
	{0x36,0x00},
	{0x33,0x00},
	{0xfe,0x03},
	{0x01,0xe7},
	{0xf7,0x01},
	{0xfc,0x8f},
	{0xfc,0x8f},
	{0xfc,0x8e},
	{0xfe,0x00},
	{0xee,0x30},
	{0x87,0x18},
	{0xfe,0x01},
	{0x8c,0x90},
	{0xfe,0x00},
	
	/*Cisctl&Analog*/
	{0xfe,0x00},
	{0x05,0x02},
	{0x06,0xda},
	{0x9d,0x0c},
	{0x09,0x00},
	{0x0a,0x04},
	{0x0b,0x00},
	{0x0c,0x03},
	{0x0d,0x07},
	{0x0e,0xa8},
	{0x0f,0x0a},
	{0x10,0x30},
	{0x11,0x02},
	{0x17,GC5035_MIRROR},
	{0x19,0x05},
	{0xfe,0x02},
	{0x30,0x03},
	{0x31,0x03},
	{0xfe,0x00},
	{0xd9,0xc0},
	{0x1b,0x20},
	{0x21,0x48},
	{0x28,0x22},
	{0x29,0x58},
	{0x44,0x20},
	{0x4b,0x10},
	{0x4e,0x1a},
	{0x50,0x11},
	{0x52,0x33},
	{0x53,0x44},
	{0x55,0x10},
	{0x5b,0x11},
	{0xc5,0x02},
	{0x8c,0x1a},
	{0xfe,0x02},
	{0x33,0x05},
	{0x32,0x38},
	{0xfe,0x00},
	{0x91,0x80},
	{0x92,0x28},
	{0x93,0x20},
	{0x95,0xa0},
	{0x96,0xe0},
	{0xd5,0xfc},
	{0x97,0x28},
	{0x16,0x0c},
	{0x1a,0x1a},
	{0x1f,0x11},
	{0x20,0x10},
	{0x46,0xe3},
	{0x4a,0x04},
	{0x54,GC5035_RSTDUMMY1},
	{0x62,0x00},
	{0x72,0xcf},
	{0x73,0xc9},
	{0x7a,0x05},
	{0x7d,0xcc},
	{0x90,0x00},
	{0xce,0x98},
	{0xd0,0xb2},
	{0xd2,0x40},
	{0xe6,0xe0},
	{0xfe,0x02},
	{0x12,0x01},
	{0x13,0x01},
	{0x14,0x01},
	{0x15,0x02},
	{0x22,GC5035_RSTDUMMY2},
	{0xfe,0x00},
	{0xfc,0x88},
	{0xfe,0x10},
	{0xfe,0x00},	
	{0xfc,0x8e},
	{0xfe,0x00},
	{0xfe,0x00},
	{0xfe,0x00},
	{0xfc,0x88},
	{0xfe,0x10},
	{0xfe,0x00},
	{0xfc,0x8e},
	
	{0xfe,0x00},
	{0xb0,0x6e},
	{0xb1,0x01},
	{0xb2,0x00},
	{0xb3,0x00},
	{0xb4,0x00},
	{0xb6,0x00},
	/*ISP*/
	{0xfe,0x01},
	{0x53,0x00},
	{0x89,0x03},
	{0x60,0x40},
	{0xfe,0x01},
	{0x42,0x21},
	{0x49,0x03},
	{0x4a,0xff},
	{0x4b,0xc0},
	
	/*BLK*/ 				
	{0x55,0x00},
	{0xfe,0x01},
	{0x41,0x28},
	{0x4c,0x00},
	{0x4d,0x00},
	{0x4e,0x3c},
	{0x44,0x08},
	{0x48,0x01},
	
	/*gain*/				
	{0xfe,0x01},
	
	/*Crop window*/ 
	{0x91,0x00},
	{0x92,0x08},
	{0x93,0x00},
	{0x94,0x07},
	{0x95,0x07},
	{0x96,0x98},
	{0x97,0x0a},
	{0x98,0x20},
	{0x99,0x00},
	
	/*MIPI*/  
	{0xfe,0x03},
	{0x02,0x57},
	{0x03,0xb7},
	{0x15,0x14},
	{0x18,0x0f},
	{0x21,0x22},
	{0x22,0x06},
	{0x23,0x48},
	{0x24,0x12},
	{0x25,0x28},
	{0x26,0x08},
	{0x29,0x06},
	{0x2a,0x58},
	{0x2b,0x08},
	{0xfe,0x01},
	{0x8c,0x10},
	{0xfe,0x00},
	{0x3e,0x00},
};

static const SENSOR_REG_T gc5035_preview_setting[] = {

};

static const SENSOR_REG_T gc5035_snapshot_setting[] = {

};


static struct sensor_res_tab_info s_gc5035_resolution_tab_raw[VENDOR_NUM] = {
	{
	.module_id = MODULE_SUNNY,
	.reg_tab = {
		{ADDR_AND_LEN_OF_ARRAY(gc5035_init_setting), PNULL, 0,
		.width = 0, .height = 0,
		.xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW},
		
		{ADDR_AND_LEN_OF_ARRAY(gc5035_preview_setting), PNULL, 0,
		.width = PREVIEW_WIDTH, .height = PREVIEW_HEIGHT,
		.xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW},
	
		{ADDR_AND_LEN_OF_ARRAY(gc5035_snapshot_setting), PNULL, 0,
		.width = SNAPSHOT_WIDTH, .height = SNAPSHOT_HEIGHT,
		.xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW}
		}
	}
/*If there are multiple modules,please add here*/
};

static SENSOR_TRIM_T s_gc5035_resolution_trim_tab[VENDOR_NUM] = {
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

static SENSOR_REG_T gc5035_shutter_reg[] = {
	{0xfe, 0x00}, 
	{0x04, 0xb8}, 
	{0x03, 0x0b},
};

static struct sensor_i2c_reg_tab gc5035_shutter_tab = {
	.settings = gc5035_shutter_reg, 
	.size = ARRAY_SIZE(gc5035_shutter_reg),
};

static SENSOR_REG_T gc5035_again_reg[] = {
	{0xfe, 0x00}, 
	{0xb6, 0x00}, 
	{0xb1, 0x01}, 
	{0xb2, 0x00}, 
};

static struct sensor_i2c_reg_tab gc5035_again_tab = {
	.settings = gc5035_again_reg, 
	.size = ARRAY_SIZE(gc5035_again_reg),
};

static SENSOR_REG_T gc5035_dgain_reg[] = {
   
};

static struct sensor_i2c_reg_tab gc5035_dgain_tab = {
	.settings = gc5035_dgain_reg, 
	.size = ARRAY_SIZE(gc5035_dgain_reg),
};

static SENSOR_REG_T gc5035_frame_length_reg[] = {
	{0xfe, 0x00}, 
	{0x41, 0x07},
	{0x42, 0xd8},
};

static struct sensor_i2c_reg_tab gc5035_frame_length_tab = {
	.settings = gc5035_frame_length_reg,
	.size = ARRAY_SIZE(gc5035_frame_length_reg),
};

static struct sensor_aec_i2c_tag gc5035_aec_info = {
	.slave_addr = (I2C_SLAVE_ADDR >> 1),
	.addr_bits_type = SENSOR_I2C_REG_8BIT,
	.data_bits_type = SENSOR_I2C_VAL_8BIT,
	.shutter = &gc5035_shutter_tab,
	.again = &gc5035_again_tab,
	.dgain = &gc5035_dgain_tab,
	.frame_length = &gc5035_frame_length_tab,
};
static SENSOR_STATIC_INFO_T s_gc5035_static_info[VENDOR_NUM] = {
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
		.fov_info = {{2.90304f, 2.17728f}, 2.3686f}}
	}
	/*If there are multiple modules,please add here*/
};


static SENSOR_MODE_FPS_INFO_T s_gc5035_mode_fps_info[VENDOR_NUM] = {
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


static struct sensor_module_info s_gc5035_module_info_tab[VENDOR_NUM] = {
	{.module_id = MODULE_SUNNY,
	.module_info = {
		.major_i2c_addr = I2C_SLAVE_ADDR >> 1,
		.minor_i2c_addr = I2C_SLAVE_ADDR >> 1,
	
		.reg_addr_value_bits = SENSOR_I2C_REG_8BIT | SENSOR_I2C_VAL_8BIT |
								SENSOR_I2C_FREQ_400,
	
		.avdd_val = SENSOR_AVDD_2800MV,
		.iovdd_val = SENSOR_AVDD_1800MV,
		.dvdd_val = SENSOR_AVDD_1200MV,
	
#if defined(GC5035_MIRROR_NORMAL)
		.image_pattern = SENSOR_IMAGE_PATTERN_RAWRGB_R,
#elif defined(GC5035_MIRROR_H)
		.image_pattern = SENSOR_IMAGE_PATTERN_RAWRGB_GR,
#elif defined(GC5035_MIRROR_V)
		.image_pattern = SENSOR_IMAGE_PATTERN_RAWRGB_GB,
#elif defined(GC5035_MIRROR_HV)
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

static struct sensor_ic_ops s_gc5035_ops_tab;
struct sensor_raw_info *s_gc5035_mipi_raw_info_ptr = PNULL;


/*==============================================================================
 * Description:
 * sensor all info
 * please modify this variable acording your spec
 *============================================================================*/
SENSOR_INFO_T g_gc5035_mipi_raw_info = {
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
		{{ .reg_addr = GC5035_PID_ADDR, .reg_value = GC5035_PID_VALUE},
		{ .reg_addr = GC5035_VER_ADDR, .reg_value = GC5035_VER_VALUE}},
	
	.source_width_max = SNAPSHOT_WIDTH,
	.source_height_max = SNAPSHOT_HEIGHT,
	.name = (cmr_s8 *)SENSOR_NAME,
	.image_format = SENSOR_IMAGE_FORMAT_RAW,
	
	.module_info_tab = s_gc5035_module_info_tab,
	.module_info_tab_size = ARRAY_SIZE(s_gc5035_module_info_tab),
	
	.resolution_tab_info_ptr = s_gc5035_resolution_tab_raw,
	.sns_ops = &s_gc5035_ops_tab,
	.raw_info_ptr = &s_gc5035_mipi_raw_info_ptr,
	
	.video_tab_info_ptr = NULL,
	.sensor_version_info = (cmr_s8 *)"gc5035_v1",
};

#endif
