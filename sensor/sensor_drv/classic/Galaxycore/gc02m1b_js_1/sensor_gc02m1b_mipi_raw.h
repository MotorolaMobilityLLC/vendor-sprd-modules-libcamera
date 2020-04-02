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

#ifndef _SENSOR_GC02M1B_MIPI_RAW_H_
#define _SENSOR_GC02M1B_MIPI_RAW_H_


#include <utils/Log.h>
#include "sensor.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"

//#include "parameters/sensor_gc02m1b_raw_param_main.c"
 

#define VENDOR_NUM           1
#define SENSOR_NAME          "gc02m1b_js_1"

#define I2C_SLAVE_ADDR       0x6e 	/*0x20*//* 8bit slave address*/

#define GC02M1B_PID_ADDR     0xf0
#define GC02M1B_PID_VALUE    0x02
#define GC02M1B_VER_ADDR     0xf1
#define GC02M1B_VER_VALUE    0xe0

/* sensor parameters begin */

/* effective sensor output image size */
#define PREVIEW_WIDTH        1600
#define PREVIEW_HEIGHT       1200
#define SNAPSHOT_WIDTH       1600
#define SNAPSHOT_HEIGHT      1200

/*Raw Trim parameters*/
#define PREVIEW_TRIM_X       0
#define PREVIEW_TRIM_Y       0
#define PREVIEW_TRIM_W       1600
#define PREVIEW_TRIM_H       1200
#define SNAPSHOT_TRIM_X      0
#define SNAPSHOT_TRIM_Y      0
#define SNAPSHOT_TRIM_W      1600
#define SNAPSHOT_TRIM_H      1200

/*Mipi output*/
#define LANE_NUM             1
#define RAW_BITS             10

#define PREVIEW_MIPI_PER_LANE_BPS      672 /* 2*Mipi clk */
#define SNAPSHOT_MIPI_PER_LANE_BPS     672 /* 2*Mipi clk */

/*line time unit: 1ns*/
#define PREVIEW_LINE_TIME       26095
#define SNAPSHOT_LINE_TIME      26095

/* frame length*/
#define PREVIEW_FRAME_LENGTH    1268
#define SNAPSHOT_FRAME_LENGTH   1268

/* please ref your spec */
#define FRAME_OFFSET			16
#define SENSOR_MAX_GAIN			0x3000 /*12x*/
#define SENSOR_BASE_GAIN		0x400
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

/* SENSOR MIRROR FLIP INFO */
#define GC02M1B_MIRROR_NORMAL    1
#define GC02M1B_MIRROR_H         0
#define GC02M1B_MIRROR_V         0
#define GC02M1B_MIRROR_HV        0

#if GC02M1B_MIRROR_NORMAL
#define GC02M1B_MIRROR	        0x80
#elif GC02M1B_MIRROR_H
#define GC02M1B_MIRROR	        0x81
#elif GC02M1B_MIRROR_V
#define GC02M1B_MIRROR	        0x82
#elif GC02M1B_MIRROR_HV
#define GC02M1B_MIRROR	        0x83
#else
#define GC02M1B_MIRROR	        0x80
#endif

/* SENSOR PRIVATE INFO FOR GAIN SETTING */
#define GC02M1B_SENSOR_GAIN_BASE             0x400
#define GC02M1B_SENSOR_GAIN_MAX              (12 * GC02M1B_SENSOR_GAIN_BASE)
#define GC02M1B_SENSOR_GAIN_MAX_VALID_INDEX  16
#define GC02M1B_SENSOR_GAIN_MAP_SIZE         16
#define GC02M1B_SENSOR_DGAIN_BASE            0x400

/*==============================================================================
 * Description:
 * register setting
 *============================================================================*/

static const SENSOR_REG_T gc02m1b_init_setting[] = {
	/*system*/
	{0xfc, 0x01},
	{0xf4, 0x41},
	{0xf5, 0xc0},
	{0xf6, 0x44},
	{0xf8, 0x38},
	{0xf9, 0x82},
	{0xfa, 0x00},
	{0xfd, 0x80},
	{0xfc, 0x81},
	{0xfe, 0x03},
	{0x01, 0x0b},
	{0xf7, 0x01},
	{0xfc, 0x80},
	{0xfc, 0x80},
	{0xfc, 0x80},
	{0xfc, 0x8e},
	
	/*CISCTL*/
	{0xfe, 0x00},
	{0x87, 0x09},
	{0xee, 0x72},
	{0xfe, 0x01},
	{0x8c, 0x90},
	{0xfe, 0x00},
	{0x90, 0x00},
	{0x03, 0x04},
	{0x04, 0x7d},
	{0x41, 0x04},
	{0x42, 0xf4},
	{0x05, 0x04},
	{0x06, 0x48},
	{0x07, 0x00},
	{0x08, 0x18},
	{0x9d, 0x18},
	{0x09, 0x00},
	{0x0a, 0x02},
	{0x0d, 0x04},
	{0x0e, 0xbc},
	{0x17, GC02M1B_MIRROR},
	{0x19, 0x04},
	{0x24, 0x00},
	{0x56, 0x20},
	{0x5b, 0x00},
	{0x5e, 0x01},
	
	/*analog Register width*/
	{0x21, 0x3c},
	{0x44, 0x20},
	{0xcc, 0x01},
	
	/*analog mode*/
	{0x1a, 0x04},
	{0x1f, 0x11},
	{0x27, 0x30},
	{0x2b, 0x00},
	{0x33, 0x00},
	{0x53, 0x90},
	{0xe6, 0x50},
	
	/*analog voltage*/
	{0x39, 0x07},
	{0x43, 0x04},
	{0x46, 0x2a},
	{0x7c, 0xa0},
	{0xd0, 0xbe},
	{0xd1, 0x60},
	{0xd2, 0x40},
	{0xd3, 0xf3},
	{0xde, 0x1d},
	
	/*analog current*/
	{0xcd, 0x05},
	{0xce, 0x6f},
	
	/*CISCTL RESET*/
	{0xfc, 0x88},
	{0xfe, 0x10},
	{0xfe, 0x00},
	{0xfc, 0x8e},
	{0xfe, 0x00},
	{0xfe, 0x00},
	{0xfe, 0x00},
	{0xfe, 0x00},
	{0xfc, 0x88},
	{0xfe, 0x10},
	{0xfe, 0x00},
	{0xfc, 0x8e},
	{0xfe, 0x04},
	{0xe0, 0x01},
	{0xfe, 0x00},
	
	/*ISP*/
	{0xfe, 0x01},
	{0x53, 0x44},
	{0x87, 0x53},
	{0x89, 0x03},
	
	/*Gain*/
	{0xfe, 0x00},
	{0xb0, 0x74},
	{0xb1, 0x04},
	{0xb2, 0x00},
	{0xb6, 0x00},
	{0xfe, 0x04},
	{0xd8, 0x00},
	{0xc0, 0x40},
	{0xc0, 0x00},
	{0xc0, 0x00},
	{0xc0, 0x00},
	{0xc0, 0x60},
	{0xc0, 0x00},
	{0xc0, 0xc0},
	{0xc0, 0x2a},
	{0xc0, 0x80},
	{0xc0, 0x00},
	{0xc0, 0x00},
	{0xc0, 0x40},
	{0xc0, 0xa0},
	{0xc0, 0x00},
	{0xc0, 0x90},
	{0xc0, 0x19},
	{0xc0, 0xc0},
	{0xc0, 0x00},
	{0xc0, 0xD0},
	{0xc0, 0x2F},
	{0xc0, 0xe0},
	{0xc0, 0x00},
	{0xc0, 0x90},
	{0xc0, 0x39},
	{0xc0, 0x00},
	{0xc0, 0x01},
	{0xc0, 0x20},
	{0xc0, 0x04},
	{0xc0, 0x20},
	{0xc0, 0x01},
	{0xc0, 0xe0},
	{0xc0, 0x0f},
	{0xc0, 0x40},
	{0xc0, 0x01},
	{0xc0, 0xe0},
	{0xc0, 0x1a},
	{0xc0, 0x60},
	{0xc0, 0x01},
	{0xc0, 0x20},
	{0xc0, 0x25},
	{0xc0, 0x80},
	{0xc0, 0x01},
	{0xc0, 0xa0},
	{0xc0, 0x2c},
	{0xc0, 0xa0},
	{0xc0, 0x01},
	{0xc0, 0xe0},
	{0xc0, 0x32},
	{0xc0, 0xc0},
	{0xc0, 0x01},
	{0xc0, 0x20},
	{0xc0, 0x38},
	{0xc0, 0xe0},
	{0xc0, 0x01},
	{0xc0, 0x60},
	{0xc0, 0x3c},
	{0xc0, 0x00},
	{0xc0, 0x02},
	{0xc0, 0xa0},
	{0xc0, 0x40},
	{0xc0, 0x80},
	{0xc0, 0x02},
	{0xc0, 0x18},
	{0xc0, 0x5c},
	{0xfe, 0x00},
	{0x9f, 0x10},
	
	/*BLK*/
	{0xfe, 0x00},
	{0x26, 0x20},
	{0xfe, 0x01},
	{0x40, 0x22},
	{0x46, 0x7f},
	{0x49, 0x0f},
	{0x4a, 0xf0},
	{0xfe, 0x04},
	{0x14, 0x80},
	{0x15, 0x80},
	{0x16, 0x80},
	{0x17, 0x80},
	
	/*anti_blooming*/
	{0xfe, 0x01},
	{0x41, 0x20},
	{0x4c, 0x00},
	{0x4d, 0x0c},
	{0x44, 0x08},
	{0x48, 0x03},
	
	/*Window 1600X1200*/
	{0xfe, 0x01},
	{0x90, 0x01},
	{0x91, 0x00},
	{0x92, 0x06},
	{0x93, 0x00},
	{0x94, 0x06},
	{0x95, 0x04},
	{0x96, 0xb0},
	{0x97, 0x06},
	{0x98, 0x40},
	
	/*mipi*/
	{0xfe, 0x03},
	{0x01, 0x23},
	{0x03, 0xce},
	{0x04, 0x48},
	{0x15, 0x00},
	{0x21, 0x10},
	{0x22, 0x05},
	{0x23, 0x20},
	{0x25, 0x20},
	{0x26, 0x08},
	{0x29, 0x06},
	{0x2a, 0x0a},
	{0x2b, 0x08},
	
	/*out*/
	{0xfe, 0x01},
	{0x8c, 0x10},
	{0xfe, 0x00},
};

static const SENSOR_REG_T gc02m1b_preview_setting[] = {
    {0xfe, 0x00},
};

static const SENSOR_REG_T gc02m1b_snapshot_setting[] = {
    {0xfe, 0x00},
};


static struct sensor_res_tab_info s_gc02m1b_resolution_tab_raw[VENDOR_NUM] = {
	{
      .module_id = MODULE_SUNNY,
      .reg_tab = {
        {ADDR_AND_LEN_OF_ARRAY(gc02m1b_init_setting), PNULL, 0,
        .width = 0, .height = 0,
        .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW},
        /*	
        {ADDR_AND_LEN_OF_ARRAY(gc02m1b_preview_setting), PNULL, 0,
        .width = PREVIEW_WIDTH, .height = PREVIEW_HEIGHT,
        .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW},
        */
        {ADDR_AND_LEN_OF_ARRAY(gc02m1b_snapshot_setting), PNULL, 0,
        .width = SNAPSHOT_WIDTH, .height = SNAPSHOT_HEIGHT,
        .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW}
		}
	}

/*If there are multiple modules,please add here*/
};

static SENSOR_TRIM_T s_gc02m1b_resolution_trim_tab[VENDOR_NUM] = {
{
     .module_id = MODULE_SUNNY,
     .trim_info = {
       {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	   /*
	   {.trim_start_x = PREVIEW_TRIM_X, .trim_start_y = PREVIEW_TRIM_Y,
        .trim_width = PREVIEW_TRIM_W,   .trim_height = PREVIEW_TRIM_H,
        .line_time = PREVIEW_LINE_TIME, .bps_per_lane = PREVIEW_MIPI_PER_LANE_BPS,
        .frame_line = PREVIEW_FRAME_LENGTH,
        .scaler_trim = {.x = PREVIEW_TRIM_X, .y = PREVIEW_TRIM_Y, .w = PREVIEW_TRIM_W, .h = PREVIEW_TRIM_H}},
       */
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

static SENSOR_REG_T gc02m1b_shutter_reg[] = {
    {0xfe, 0x00},
	{0x03, 0x04},
	{0x04, 0x7d},
};

static struct sensor_i2c_reg_tab gc02m1b_shutter_tab = {
    .settings = gc02m1b_shutter_reg, 
	.size = ARRAY_SIZE(gc02m1b_shutter_reg),
};

static SENSOR_REG_T gc02m1b_again_reg[] = {
    {0xfe, 0x00},
    {0xb6, 0x00},
	{0xb1, 0x00},
	{0xb2, 0x00},
};

static struct sensor_i2c_reg_tab gc02m1b_again_tab = {
    .settings = gc02m1b_again_reg, 
	.size = ARRAY_SIZE(gc02m1b_again_reg),
};

static SENSOR_REG_T gc02m1b_dgain_reg[] = {

};

static struct sensor_i2c_reg_tab gc02m1b_dgain_tab = {
    .settings = gc02m1b_dgain_reg, 
	.size = ARRAY_SIZE(gc02m1b_dgain_reg),
};

static SENSOR_REG_T gc02m1b_frame_length_reg[] = {
    {0xfe, 0x00},
    {0x41, 0x04},
	{0x42, 0xf4},
};

static struct sensor_i2c_reg_tab gc02m1b_frame_length_tab = {
    .settings = gc02m1b_frame_length_reg,
    .size = ARRAY_SIZE(gc02m1b_frame_length_reg),
};

static struct sensor_aec_i2c_tag gc02m1b_aec_info = {
    .slave_addr = (I2C_SLAVE_ADDR >> 1),
    .addr_bits_type = SENSOR_I2C_REG_8BIT,
    .data_bits_type = SENSOR_I2C_VAL_8BIT,
    .shutter = &gc02m1b_shutter_tab,
    .again = &gc02m1b_again_tab,
    .dgain = &gc02m1b_dgain_tab,
    .frame_length = &gc02m1b_frame_length_tab,
};


static SENSOR_STATIC_INFO_T s_gc02m1b_static_info[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .static_info = {
        .f_num = 200,
        .focal_length = 354,
        .max_fps = 30,
        .max_adgain = 8,
        .ois_supported = 0,
		.pdaf_supported = 0,
        .exp_valid_frame_num = 1,
        .clamp_level = 64,
        .adgain_valid_frame_num = 1,
        .fov_info = {{4.614f, 3.444f}, 4.222f}}
    }
    /*If there are multiple modules,please add here*/
};

static SENSOR_MODE_FPS_INFO_T s_gc02m1b_mode_fps_info[VENDOR_NUM] = {
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


static struct sensor_module_info s_gc02m1b_module_info_tab[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .module_info = {
         .major_i2c_addr = I2C_SLAVE_ADDR >> 1,
         .minor_i2c_addr = I2C_SLAVE_ADDR >> 1,

         .reg_addr_value_bits = SENSOR_I2C_REG_8BIT | SENSOR_I2C_VAL_8BIT |
                                SENSOR_I2C_FREQ_400,

         .avdd_val = SENSOR_AVDD_2800MV,
         .iovdd_val = SENSOR_AVDD_1800MV,
         .dvdd_val = SENSOR_AVDD_1800MV,

         .image_pattern = SENSOR_IMAGE_PATTERN_RAWRGB_R,

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

static struct sensor_ic_ops s_gc02m1b_ops_tab;
struct sensor_raw_info *s_gc02m1b_mipi_raw_info_ptr = PNULL;


/*==============================================================================
 * Description:
 * sensor all info
 * please modify this variable acording your spec
 *============================================================================*/
SENSOR_INFO_T g_gc02m1b_mipi_raw_info = {
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
        {{ .reg_addr = GC02M1B_PID_ADDR, .reg_value = GC02M1B_PID_VALUE},
         { .reg_addr = GC02M1B_VER_ADDR, .reg_value = GC02M1B_VER_VALUE}},

    .source_width_max = SNAPSHOT_WIDTH,
    .source_height_max = SNAPSHOT_HEIGHT,
    .name = (cmr_s8 *)SENSOR_NAME,
    .image_format = SENSOR_IMAGE_FORMAT_RAW,

    .module_info_tab = s_gc02m1b_module_info_tab,
    .module_info_tab_size = ARRAY_SIZE(s_gc02m1b_module_info_tab),

    .resolution_tab_info_ptr = s_gc02m1b_resolution_tab_raw,
    .sns_ops = &s_gc02m1b_ops_tab,
    .raw_info_ptr = &s_gc02m1b_mipi_raw_info_ptr,

    .video_tab_info_ptr = NULL,
    .sensor_version_info = (cmr_s8 *)"gc02m1b_js_1",
};

#endif
