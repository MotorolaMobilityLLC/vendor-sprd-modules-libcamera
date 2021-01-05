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

#ifndef _SENSOR_GC6153_MIPI_YUV_H_
#define _SENSOR_GC6153_MIPI_YUV_H_

#include <utils/Log.h>
#include "sensor.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"

//add for different target use differet sensor effect and sensor config. hong.zhang@itel.com, 2018-9-25
#if 0//def TARG_SENSCONFIG
#define _DEF_STR(name)            #name
#define DEF_STR(name)            _DEF_STR(name)
#define CONFIG_PATH(path)         path##gc6153config.h
#define SENSCONFIG_INCLUD(path)    DEF_STR(CONFIG_PATH(path))
#include SENSCONFIG_INCLUD(TARG_SENSCONFIG)
#endif
//add end
#define VENDOR_NUM 1
#define SENSOR_NAME				"gc6153_mipi_yuv"

#define I2C_SLAVE_ADDR			0x80   /* 8bit slave address*/

#define GC6153_PID_ADDR			0xf0
#define GC6153_PID_VALUE		0x61
#define GC6153_VER_ADDR			0xf1
#define GC6153_VER_VALUE		0x53
/* sensor parameters begin */

/* effective sensor output image size */
#define PREVIEW_WIDTH			320//240X320
#define PREVIEW_HEIGHT			240
#define SNAPSHOT_WIDTH			320 
#define SNAPSHOT_HEIGHT			240
//#define GC6153_MCLOCK_26
/*Raw Trim parameters*/
#define PREVIEW_TRIM_X			0
#define PREVIEW_TRIM_Y			0
#define PREVIEW_TRIM_W			320
#define PREVIEW_TRIM_H			240
#define SNAPSHOT_TRIM_X			0
#define SNAPSHOT_TRIM_Y			0
#define SNAPSHOT_TRIM_W			320
#define SNAPSHOT_TRIM_H			240

/*Mipi output*/
#define LANE_NUM				1
#define RAW_BITS				8

#define PREVIEW_MIPI_PER_LANE_BPS	  192  /* 2*Mipi clk */
#define SNAPSHOT_MIPI_PER_LANE_BPS	  192  /* 2*Mipi clk */

/*line time unit: 1ns*/
#ifdef GC6153_MCLOCK_26
#define PREVIEW_LINE_TIME		  92300
#define SNAPSHOT_LINE_TIME		  92300
#else
#define PREVIEW_LINE_TIME		  83300
#define SNAPSHOT_LINE_TIME		  83300
#endif
/* frame length*/
#define PREVIEW_FRAME_LENGTH		600
#define SNAPSHOT_FRAME_LENGTH		600

/* please ref your spec */
#define FRAME_OFFSET			6
#define SENSOR_MAX_GAIN			0x100
#define SENSOR_BASE_GAIN		0x10
#define SENSOR_MIN_SHUTTER		6

/* please ref your spec
 * 1 : average binning
 * 2 : sum-average binning
 * 4 : sum binning
 */
#define BINNING_FACTOR			0

/* please ref spec
 * 1: sensor auto caculate
 * 0: driver caculate
 */
/* sensor parameters end */

/* isp parameters, please don't change it*/
#define ISP_BASE_GAIN			0x80

/* please don't change it */
#ifdef GC6153_MCLOCK_26
#define EX_MCLK				26
#else
#define EX_MCLK				24
#endif

#define IMAGE_NORMAL 
//#define IMAGE_H_MIRROR 
//#define IMAGE_V_MIRROR 
//#define IMAGE_HV_MIRROR 
#ifdef IMAGE_NORMAL
#define MIRROR 0x7c
#define AD_NUM 0x05
#define COL_SWITCH 0x18
#endif

#ifdef IMAGE_H_MIRROR
#define MIRROR 0x69
#define AD_NUM 0x04
#define COL_SWITCH 0x19
#endif

#ifdef IMAGE_V_MIRROR
#define MIRROR 0x7e
#define AD_NUM 0x05
#define COL_SWITCH 0x18
#endif

#ifdef IMAGE_HV_MIRROR
#define MIRROR 0x6b
#define AD_NUM 0x04
#define COL_SWITCH 0x19
#endif


/*==============================================================================
 * Description:
 * register setting
 *============================================================================*/

static const SENSOR_REG_T gc6153_init_setting[] = {
   	/*SYS*/
	{0xfe, 0xa0},
	{0xfe, 0xa0},
	{0xfe, 0xa0},
	{0xfa, 0x11},
	{0xfc, 0x00},
	{0xf6, 0x00},
	{0xfc, 0x12}, 

	/*ANALOG & CISCTL*/  
	{0xfe, 0x00},
	{0x01, 0x40}, 
	{0x02, 0x12}, 
	{0x0d, 0x40}, 
	{0x14, MIRROR}, 
	{0x16, AD_NUM}, 
	{0x17, COL_SWITCH}, 
	{0x1c, 0x31}, 
	{0x1d, 0xb9}, 
	{0x1f, 0x1a}, 
	{0x73, 0x20}, 
	{0x74, 0x71}, 
	{0x77, 0x22}, 
	{0x7a, 0x08}, 
	{0x11, 0x18}, 
	{0x13, 0x48}, 
	{0x12, 0xc8}, 
	{0x70, 0xc8}, 
	{0x7b, 0x18}, 
	{0x7d, 0x30}, 
	{0x7e, 0x02}, 

	{0xfe, 0x10}, 
	{0xfe, 0x00},
	{0xfe, 0x00},
	{0xfe, 0x00},
	{0xfe, 0x00},
	{0xfe, 0x00},
	{0xfe, 0x10},
	{0xfe, 0x00},

	{0x49, 0x61},
	{0x4a, 0x40},
	{0x4b, 0x58},

	/*ISP*/            
	{0xfe, 0x00},
	{0x39, 0x02}, 
	{0x3a, 0x80}, 
	{0x20, 0x7e}, 
	{0x26, 0x87}, 

	/*BLK*/         
	{0x33, 0x10}, 
	{0x37, 0x06}, 
	{0x2a, 0x21}, 

	/*GAIN*/
	{0x3f, 0x16}, 

	/*DNDD*/    
	{0x52, 0xa6},
	{0x53, 0x81},
	{0x54, 0x43},
	{0x56, 0x78},
	{0x57, 0xaa},
	{0x58, 0xff}, 

	/*ASDE*/           
	{0x5b, 0x60}, 
	{0x5c, 0x50}, 
	{0xab, 0x2a}, 
	{0xac, 0xb5},
					   
	/*INTPEE*/        
	{0x5e, 0x06}, 
	{0x5f, 0x06},
	{0x60, 0x44},
	{0x61, 0xff},
	{0x62, 0x69}, 
	{0x63, 0x13}, 

	/*CC*/           
	{0x65, 0x13}, 
	{0x66, 0x26},
	{0x67, 0x07},
	{0x68, 0xf5}, 
	{0x69, 0xea},
	{0x6a, 0x21},
	{0x6b, 0x21}, 
	{0x6c, 0xe4},
	{0x6d, 0xfb},
				
	/*YCP*/          
	{0x81, 0x3b}, 
	{0x82, 0x3b}, 
	{0x83, 0x4b},
	{0x84, 0x90},
	{0x86, 0xf0},
	{0x87, 0x1d},
	{0x88, 0x16},
	{0x8d, 0x74},
	{0x8e, 0x25},

	/*AEC*/
	{0x90, 0x36},//0x36 wangqiang20190418 changge for lightvule
	{0x92, 0x43},//0x43 wangqiang20190418 changge for lightvule
	{0x9d, 0x32},
	{0x9e, 0x81},
	{0x9f, 0xf4},
	{0xa0, 0xa0},
	{0xa1, 0x04},
	{0xa3, 0x2d},
	{0xa4, 0x01},

	/*AWB*/
	{0xb0, 0xc2},
	{0xb1, 0x1e},
	{0xb2, 0x10},
	{0xb3, 0x20},
	{0xb4, 0x2d},
	{0xb5, 0x1b}, 
	{0xb6, 0x2e},
	{0xb8, 0x13},
	{0xba, 0x60},
	{0xbb, 0x62},
	{0xbd, 0x78}, 
	{0xbe, 0x55},
	{0xbf, 0xa0}, 
	{0xc4, 0xe7},
	{0xc5, 0x15},
	{0xc6, 0x16},
	{0xc7, 0xeb}, 
	{0xc8, 0xe4},
	{0xc9, 0x16},
	{0xca, 0x16},
	{0xcb, 0xe9},
	{0x22, 0xf8}, 

	/*SPI*/          
	{0xfe, 0x02},
	{0x01, 0x00}, //SPI MASTER enable
	{0x02, 0x80}, //ddr_mode
	{0x20, 0xb6},  //frame_start -- not need
	{0x21, 0x80},  //line sync start
	{0x22, 0x9d},  //line sync end
	{0x23, 0xb6},  //frame end
	{0x25, 0xff},  //head sync code
	{0x26, 0x00},
	{0x27, 0x00},
	{0x04, 0x20},  //[4] master_outformat
	{0x0a, 0x00},  //Data ID, 0x00-YUV422, 0x01-RGB565
	{0x13, 0x10},
	{0x03, 0x20},  //one wire
	{0x24, 0x03},  //[1]sck_always [0]BT656
	{0x28, 0x03}, //clock_div_spi
	{0xfe, 0x00},
			  
	/*OUTPUT*/        
	{0xf2, 0x00}, 
	{0xf6, 0x00},
	{0xfe, 0x00},
};

static const SENSOR_REG_T gc6153_preview_setting[] = {
};

static const SENSOR_REG_T gc6153_snapshot_setting[] = {
};

static struct sensor_res_tab_info s_gc6153_resolution_tab_raw[VENDOR_NUM] = {
	{
      .module_id = MODULE_SUNNY,
      .reg_tab = {
        {ADDR_AND_LEN_OF_ARRAY(gc6153_init_setting), PNULL, 0,
        .width = 0, .height = 0,
        .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW},
		
        {ADDR_AND_LEN_OF_ARRAY(gc6153_preview_setting), PNULL, 0,
        .width = PREVIEW_WIDTH, .height = PREVIEW_HEIGHT,
        .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW},

        {ADDR_AND_LEN_OF_ARRAY(gc6153_snapshot_setting), PNULL, 0,
        .width = SNAPSHOT_WIDTH, .height = SNAPSHOT_HEIGHT,
        .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW}
		}
	}
/*If there are multiple modules,please add here*/
};

static SENSOR_TRIM_T s_gc6153_resolution_trim_tab[VENDOR_NUM] = {
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

static SENSOR_REG_T gc6153_shutter_reg[] = {
};

static struct sensor_i2c_reg_tab gc6153_shutter_tab = {
    .settings = gc6153_shutter_reg, 
	.size = ARRAY_SIZE(gc6153_shutter_reg),
};

static SENSOR_REG_T gc6153_again_reg[] = {
};

static struct sensor_i2c_reg_tab gc6153_again_tab = {
    .settings = gc6153_again_reg, 
	.size = ARRAY_SIZE(gc6153_again_reg),
};

static SENSOR_REG_T gc6153_dgain_reg[] = {
   
};

static struct sensor_i2c_reg_tab gc6153_dgain_tab = {
    .settings = gc6153_dgain_reg, 
	.size = ARRAY_SIZE(gc6153_dgain_reg),
};

static SENSOR_REG_T gc6153_frame_length_reg[] = {
};

static struct sensor_i2c_reg_tab gc6153_frame_length_tab = {
    .settings = gc6153_frame_length_reg,
    .size = ARRAY_SIZE(gc6153_frame_length_reg),
};

static struct sensor_aec_i2c_tag gc6153_aec_info = {
    .slave_addr = (I2C_SLAVE_ADDR >> 1),
    .addr_bits_type = SENSOR_I2C_REG_8BIT,
    .data_bits_type = SENSOR_I2C_VAL_8BIT,
    .shutter = &gc6153_shutter_tab,
    .again = &gc6153_again_tab,
    .dgain = &gc6153_dgain_tab,
    .frame_length = &gc6153_frame_length_tab,
};


static SENSOR_STATIC_INFO_T s_gc6153_static_info[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .static_info = {
        .f_num = 200,
        .focal_length = 354,
        .max_fps = 30,
        .max_adgain = 4,
        .ois_supported = 0,
        .pdaf_supported = 0,
        .exp_valid_frame_num = 1,
        .clamp_level = 0,
        .adgain_valid_frame_num = 1,
        .fov_info = {{4.614f, 3.444f}, 4.222f}}
    }
    /*If there are multiple modules,please add here*/
};

static SENSOR_MODE_FPS_INFO_T s_gc6153_mode_fps_info[VENDOR_NUM] = {
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


static struct sensor_module_info s_gc6153_module_info_tab[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .module_info = {
         .major_i2c_addr = I2C_SLAVE_ADDR >> 1,
         .minor_i2c_addr = I2C_SLAVE_ADDR >> 1,

         .reg_addr_value_bits = SENSOR_I2C_REG_8BIT | SENSOR_I2C_VAL_8BIT |
                                SENSOR_I2C_FREQ_400,

         .avdd_val = SENSOR_AVDD_2800MV,
         .iovdd_val = SENSOR_AVDD_1800MV,
         .dvdd_val = SENSOR_AVDD_1800MV,

         .image_pattern = SENSOR_IMAGE_PATTERN_YUV422_YUYV,

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

static struct sensor_ic_ops s_gc6153_ops_tab;
struct sensor_raw_info *s_gc6153_mipi_raw_info_ptr = NULL;


/*==============================================================================
 * Description:
 * sensor all info
 * please modify this variable acording your spec
 *============================================================================*/
SENSOR_INFO_T g_gc6153_mipi_yuv_info = {
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
    .reset_pulse_width = 100,
    .power_down_level = SENSOR_HIGH_LEVEL_PWDN,
    .identify_count = 1,
    .identify_code =
        {{ .reg_addr = GC6153_PID_ADDR, .reg_value = GC6153_PID_VALUE},
         { .reg_addr = GC6153_VER_ADDR, .reg_value = GC6153_VER_VALUE}},

    .source_width_max = SNAPSHOT_WIDTH,
    .source_height_max = SNAPSHOT_HEIGHT,
    .name = (cmr_s8 *)SENSOR_NAME,
    .image_format = SENSOR_IMAGE_FORMAT_YUV422,

    .module_info_tab = s_gc6153_module_info_tab,
    .module_info_tab_size = ARRAY_SIZE(s_gc6153_module_info_tab),

    .resolution_tab_info_ptr = s_gc6153_resolution_tab_raw,
    .sns_ops = &s_gc6153_ops_tab,
    .raw_info_ptr = &s_gc6153_mipi_raw_info_ptr,

    .video_tab_info_ptr = NULL,
    .sensor_version_info = (cmr_s8 *)"gc6153_v1",
};

#endif
