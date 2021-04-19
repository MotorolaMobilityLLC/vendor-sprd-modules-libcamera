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

#ifndef _SENSOR_sp5508_MIPI_RAW_H_
#define _SENSOR_sp5508_MIPI_RAW_H_


#include <utils/Log.h>
#include "sensor.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"

//#include "parameters/sensor_sp5508_raw_param_main.c"

//#define FEATURE_OTP

#define VENDOR_NUM 1
#define SENSOR_NAME				"sp5508_mipi_raw"

#define I2C_SLAVE_ADDR			0x78 		/* 8bit slave address*/

#define sp5508_PID_ADDR				0x02
#define sp5508_PID_VALUE			0x55
#define sp5508_VER_ADDR				0x03
#define sp5508_VER_VALUE			0x08

/* sensor parameters begin */

/* effective sensor output image size */
#define PREVIEW_WIDTH			2592 //1600
#define PREVIEW_HEIGHT			1944 //1200
#define SNAPSHOT_WIDTH			2592 //1600 
#define SNAPSHOT_HEIGHT			1944 //1200

/*Raw Trim parameters*/
#define PREVIEW_TRIM_X			0
#define PREVIEW_TRIM_Y			0
#define PREVIEW_TRIM_W			PREVIEW_WIDTH
#define PREVIEW_TRIM_H			PREVIEW_HEIGHT
#define SNAPSHOT_TRIM_X			0
#define SNAPSHOT_TRIM_Y			0
#define SNAPSHOT_TRIM_W			SNAPSHOT_WIDTH
#define SNAPSHOT_TRIM_H			SNAPSHOT_HEIGHT

/*Mipi output*/
#define LANE_NUM			2
#define RAW_BITS			10

#define PREVIEW_MIPI_PER_LANE_BPS	  936 //720  /* 2*Mipi clk */
#define SNAPSHOT_MIPI_PER_LANE_BPS	  936 //720  /* 2*Mipi clk */

/*line time unit: 1ns*/
#define PREVIEW_LINE_TIME		  16688 //26300
#define SNAPSHOT_LINE_TIME		  16688 //26300

/* frame length*/
#define PREVIEW_FRAME_LENGTH		1994 //1234
#define SNAPSHOT_FRAME_LENGTH		1994 //1234

/* please ref your spec */
#define FRAME_OFFSET			10
#define SENSOR_MAX_GAIN			0xf8
#define SENSOR_BASE_GAIN		0x10
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

/*==============================================================================
 * Description:
 * register setting
 *============================================================================*/

static const SENSOR_REG_T sp5508_init_setting[] = {
/*			
	global setting		
	initial setting 	
	register	value	description
	fd	00	Page 0*/

	{0xfd, 0x00},
	{0x2e, 0x24},// PLL_CTRL
	{0x2f, 0x01},// 
	{0xfd, 0x01},// Page 1
	{0x03, 0x02},// Exposure RES
	{0x04, 0x57},// 
	{0x06, 0x01},// V blank (8LSB)
	{0x24, 0x10},// PGA gain control
	{0x39, 0x0d},// digital gain 1.625 
	{0x31, 0x00},// bypassdsp
	{0x3f, 0x01},//
	{0x01, 0x01},// 
	{0x11, 0x60},// 
	{0x33, 0xb0},// 
	{0x12, 0x03},// 
	{0x13, 0xd0},// 
	{0x45, 0x1e},// 
	{0x16, 0xea},// 
	{0x19, 0xf7},// 
	{0x1a, 0x5f},// 
	{0x1c, 0x0c},// 
	{0x1d, 0x06},// 
	{0x1e, 0x09},// 
	{0x20, 0x07},// 
	{0x2a, 0x0f},// 
	{0x2c, 0x10},// 
	{0x25, 0x0d},// 
	{0x26, 0x0d},// 
	{0x27, 0x08},// 
	{0x29, 0x01},// Pldo off
	{0x2d, 0x06},// 
	{0x55, 0x14},// Pixel timing
	{0x56, 0x00},// 
	{0x57, 0x17},// 
	{0x59, 0x00},// 
	{0x5a, 0x04},// 
	{0x50, 0x10},// 
	{0x53, 0x0e},// 
	{0x6b, 0x10},// 
	{0x5c, 0x20},// 
	{0x5d, 0x00},// 
	{0x5e, 0x06},// 
	{0x66, 0x38},// 
	{0x68, 0x30},// 
	{0x71, 0x3f},// 
	{0x72, 0x05},// 
	{0x73, 0x3e},// 
	{0x81, 0x22},// 
	{0x8a, 0x66},// 
	{0x8b, 0x66},// 
	{0xc0, 0x02},// Blc_rpc
	{0xc1, 0x02},// 
	{0xc2, 0x02},// 
	{0xc3, 0x02},// 
	{0xc4, 0x82},// 
	{0xc5, 0x82},// 
	{0xc6, 0x82},// 
	{0xc7, 0x82},// 
	{0xfb, 0x43},// Blc_offset
	{0xf0, 0x27},// 
	{0xf1, 0x27},// 
	{0xf2, 0x27},// 
	{0xf3, 0x27},// 
	{0xb1, 0xad},// 
	{0xb6, 0x42},// 
	{0xa1, 0x04},// 
	{0xa0, 0x00},// mipi buf en
	{0xfd, 0x02},// 
	{0x34, 0xc8},// 
	{0x60, 0x99},// 
	{0x93, 0x03},// 
	//{0x18, 0xe0},//	
	//{0x19, 0xa0},//	
	{0xfd, 0x04},// 
	{0x31, 0x4b},// 
	{0x32, 0x4b},// 
	{0xfd, 0x03},// Page 3
	{0xc0, 0x00},// 
	{0xfd, 0x01},// Page 1
	{0x8e, 0x0a},// mipi output size
	{0x8f, 0x20},// 
	{0x90, 0x07},// 
	{0x91, 0x98},// 
	{0xfd, 0x02},// Page 2
	{0xa0, 0x00},// Image vertical start
	{0xa1, 0x08},// 
	{0xa2, 0x07},// Image vertical end
	{0xa3, 0x98},// 
	{0xa4, 0x00},// Image horizontal start
	{0xa5, 0x08},// 
	{0xa6, 0x0a},// Image horizontal end
	{0xa7, 0x20},// 
	{0xfd, 0x01},// Page 1

};

static const SENSOR_REG_T sp5508_preview_setting[] = {

};

static const SENSOR_REG_T sp5508_snapshot_setting[] = {
/*	"Full size
	2592x1944"	
	30.05fps,line time 16.6880us, 936Mbps/lane	
	register	value*/
	{0xfd, 0x00},//
	{0x2e, 0x24},//
	{0x2f, 0x01},//
	{0x30, 0x03},//
	{0x33, 0x00},//
	{0xfd, 0x01},//
	{0x03, 0x02},//
	{0x04, 0x57},//
	{0x05, 0x00},//
	{0x06, 0x01},//
	{0x0e, 0x04},//
	{0x0f, 0x50},//
	{0x0d, 0x00},//
	{0x3f, 0x01},//
	{0x01, 0x01},//
	{0x31, 0x00},//
	{0x33, 0xb0},//
	{0x13, 0xd0},//
	{0x45, 0x1e},//
	{0x1f, 0x0a},//
	{0x2a, 0x0f},//
	{0x57, 0x17},//
	{0x71, 0x3f},//
	{0x72, 0x05},//
	{0x73, 0x3e},//
	{0xa1, 0x04},//
	{0xfd, 0x02},//
	{0x34, 0xc8},//
	{0xfd, 0x01},//
	{0x8e, 0x0a},//
	{0x8f, 0x20},//
	{0x90, 0x07},//
	{0x91, 0x98},//
	{0xfd, 0x02},//
	{0xa0, 0x00},//
	{0xa1, 0x08},//
	{0xa2, 0x07},//
	{0xa3, 0x98},//
	{0xa4, 0x00},//
	{0xa5, 0x08},//
	{0xa6, 0x0a},//
	{0xa7, 0x20},//

};


static struct sensor_res_tab_info s_sp5508_resolution_tab_raw[VENDOR_NUM] = {
	{
      .module_id = MODULE_SUNNY,
      .reg_tab = {
        {ADDR_AND_LEN_OF_ARRAY(sp5508_init_setting), PNULL, 0,
        .width = 0, .height = 0,
        .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW},
		
        {ADDR_AND_LEN_OF_ARRAY(sp5508_preview_setting), PNULL, 0,
        .width = PREVIEW_WIDTH, .height = PREVIEW_HEIGHT,
        .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW},

        {ADDR_AND_LEN_OF_ARRAY(sp5508_snapshot_setting), PNULL, 0,
        .width = SNAPSHOT_WIDTH, .height = SNAPSHOT_HEIGHT,
        .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW}
		}
	}

	/*If there are multiple modules,please add here*/
};

static SENSOR_TRIM_T s_sp5508_resolution_trim_tab[VENDOR_NUM] = {
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

static SENSOR_REG_T sp5508_shutter_reg[] = {
    {0xfd, 0x01}, 
	{0x03, 0x07}, 
	{0x04, 0x85}, 
	{0x01, 0x01}, 

};

static struct sensor_i2c_reg_tab sp5508_shutter_tab = {
    .settings = sp5508_shutter_reg, 
	.size = ARRAY_SIZE(sp5508_shutter_reg),
};

static SENSOR_REG_T sp5508_again_reg[] = {
   // {0xfd, 0x01}, 
	{0x24, 0xf0}, 
	{0x01, 0x01}, 
 
};

static struct sensor_i2c_reg_tab sp5508_again_tab = {
    .settings = sp5508_again_reg, 
	.size = ARRAY_SIZE(sp5508_again_reg),
};

static SENSOR_REG_T sp5508_dgain_reg[] = {
   
};

static struct sensor_i2c_reg_tab sp5508_dgain_tab = {
    .settings = sp5508_dgain_reg, 
	.size = ARRAY_SIZE(sp5508_dgain_reg),
};

static SENSOR_REG_T sp5508_frame_length_reg[] = {
    {0xfd, 0x01}, 
	{0x05, 0x07}, 
	{0x06, 0x85}, 
	{0x01, 0x01}, 

};

static struct sensor_i2c_reg_tab sp5508_frame_length_tab = {
    .settings = sp5508_frame_length_reg,
    .size = ARRAY_SIZE(sp5508_frame_length_reg),
};

static struct sensor_aec_i2c_tag sp5508_aec_info = {
    .slave_addr = (I2C_SLAVE_ADDR >> 1),
    .addr_bits_type = SENSOR_I2C_REG_8BIT,
    .data_bits_type = SENSOR_I2C_VAL_8BIT,
    .shutter = &sp5508_shutter_tab,
    .again = &sp5508_again_tab,
    .dgain = &sp5508_dgain_tab,
    .frame_length = &sp5508_frame_length_tab,
};


static SENSOR_STATIC_INFO_T s_sp5508_static_info[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .static_info = {
        .f_num = 200,
        .focal_length = 354,
        .max_fps = 0,
        .max_adgain = 15 * 2,
        .ois_supported = 0,
        .pdaf_supported = 0,
        .exp_valid_frame_num = 1,
        .clamp_level = 64,
        .adgain_valid_frame_num = 1,
        .fov_info = {{4.614f, 3.444f}, 4.222f}}
    }
    /*If there are multiple modules,please add here*/
};


static SENSOR_MODE_FPS_INFO_T s_sp5508_mode_fps_info[VENDOR_NUM] = {
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


static struct sensor_module_info s_sp5508_module_info_tab[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .module_info = {
         .major_i2c_addr = I2C_SLAVE_ADDR >> 1,
         .minor_i2c_addr = I2C_SLAVE_ADDR >> 1,

         .reg_addr_value_bits = SENSOR_I2C_REG_8BIT | SENSOR_I2C_VAL_8BIT |
                                SENSOR_I2C_FREQ_400,

         .avdd_val = SENSOR_AVDD_2800MV,
         .iovdd_val = SENSOR_AVDD_1800MV,
         .dvdd_val = SENSOR_AVDD_1200MV,

         .image_pattern = SENSOR_IMAGE_PATTERN_RAWRGB_GB,

         .preview_skip_num = 1,
         .capture_skip_num = 1,
         .flash_capture_skip_num = 6,
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

static struct sensor_ic_ops s_sp5508_ops_tab;
struct sensor_raw_info *s_sp5508_mipi_raw_info_ptr = PNULL;//&s_sp5508_mipi_raw_info;


/*==============================================================================
 * Description:
 * sensor all info
 * please modify this variable acording your spec
 *============================================================================*/
SENSOR_INFO_T g_sp5508_mipi_raw_info = {
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
        {{ .reg_addr = sp5508_PID_ADDR, .reg_value = sp5508_PID_VALUE},
         { .reg_addr = sp5508_VER_ADDR, .reg_value = sp5508_VER_VALUE}},

    .source_width_max = SNAPSHOT_WIDTH,
    .source_height_max = SNAPSHOT_HEIGHT,
    .name = (cmr_s8 *)SENSOR_NAME,
    .image_format = SENSOR_IMAGE_FORMAT_RAW,

    .module_info_tab = s_sp5508_module_info_tab,
    .module_info_tab_size = ARRAY_SIZE(s_sp5508_module_info_tab),

    .resolution_tab_info_ptr = s_sp5508_resolution_tab_raw,
    .sns_ops = &s_sp5508_ops_tab,
    .raw_info_ptr = &s_sp5508_mipi_raw_info_ptr,

    .video_tab_info_ptr = NULL,
    .sensor_version_info = (cmr_s8 *)"sp5508_v1",
};

#endif
