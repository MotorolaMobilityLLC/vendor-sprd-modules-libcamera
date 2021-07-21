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
#define LOG_TAG "ov08d10_syp_2lane"

#ifndef _SENSOR_ov08d10_syp_MIPI_RAW_H_
#define _SENSOR_ov08d10_syp_MIPI_RAW_H_

#include <utils/Log.h>
#include "sensor.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"
#include "isp_com.h"

//#include "parameters/sensor_ov08d10_syp_raw_param_main.c"


#define VENDOR_NUM 1
#define SENSOR_NAME "ov08d10_syp_3"

#define I2C_SLAVE_ADDR 0x6c /* 8bit slave address*/

#define ov08d10_syp_PID_ADDR 0x01
#define ov08d10_syp_PID_VALUE 0x08
#define ov08d10_syp_VER_ADDR 0x02
#define ov08d10_syp_VER_VALUE 0x47

/* sensor parameters begin */

/* effective sensor output image size */
#define VIDEO_WIDTH 1280
#define VIDEO_HEIGHT 720
#define PREVIEW_WIDTH 1632
#define PREVIEW_HEIGHT 1224
#define SNAPSHOT_WIDTH 3264
#define SNAPSHOT_HEIGHT 2448

/*Raw Trim parameters*/
#define VIDEO_TRIM_X 0
#define VIDEO_TRIM_Y 0
#define VIDEO_TRIM_W 1280
#define VIDEO_TRIM_H 720
#define PREVIEW_TRIM_X 0
#define PREVIEW_TRIM_Y 0
#define PREVIEW_TRIM_W 1632
#define PREVIEW_TRIM_H 1224
#define SNAPSHOT_TRIM_X 0
#define SNAPSHOT_TRIM_Y 0
#define SNAPSHOT_TRIM_W 3264
#define SNAPSHOT_TRIM_H 2448

/*Mipi output*/
#define LANE_NUM 2
#define RAW_BITS 10

#define VIDEO_MIPI_PER_LANE_BPS 720     /* 2*Mipi clk */
#define PREVIEW_MIPI_PER_LANE_BPS 360   /* 2*Mipi clk */
#define SNAPSHOT_MIPI_PER_LANE_BPS 1440 /* 2*Mipi clk */

/*line time unit: ns*/
#define VIDEO_LINE_TIME 13278
#define PREVIEW_LINE_TIME 26556
#define SNAPSHOT_LINE_TIME 12778

/* frame length*/
#define VIDEO_FRAME_LENGTH 1252
#define PREVIEW_FRAME_LENGTH 1252
#define SNAPSHOT_FRAME_LENGTH 2608

/* please ref your spec */
#define FRAME_OFFSET 20
#define SENSOR_MAX_GAIN 0xf8 // x15.5 A gain
#define SENSOR_BASE_GAIN 0x10
#define SENSOR_MIN_SHUTTER 8

/* please ref your spec
 * 1 : average binning
 * 2 : sum-average binning
 * 4 : sum binning
 */
#define BINNING_FACTOR 1

/* please ref spec
 * 1: sensor auto caculate
 * 0: driver caculate
 */
/* sensor parameters end */

/* isp parameters, please don't change it*/
#define ISP_BASE_GAIN 0x80

/* please don't change it */
#define EX_MCLK 24

#define IMAGE_NORMAL_MIRROR

/*==============================================================================
 * Description:
 * register setting
 *============================================================================*/

static const SENSOR_REG_T ov08d10_syp_init_setting[] = {
    /*2lan init setting
    clk_in = 24MHz
    mipi_clk =360MHz
    resolution =1632*1224
    bayer pattern=BGGR*/
	{0xfd, 0x00},
	{0x20, 0x0e},
	{0x20, 0x0b},
	{0xfd, 0x00},
	{0x1d, 0x00},
	{0x1c, 0x19},
	{0x11, 0x2a},
	{0x14, 0x22},
	{0x1b, 0x78},
	{0x1e, 0x13},
	{0xb7, 0x02},
	{0xfd, 0x01},
	{0x1a, 0x0a},
	{0x1b, 0x08},
	{0x2a, 0x01},
	{0x2b, 0x9a},
	{0xfd, 0x01},
	{0x12, 0x00},
	{0x03, 0x08},
	{0x04, 0xd4},
	{0x06, 0x68},
	{0x07, 0x05},
	{0x21, 0x02},
	{0x24, 0x30},
	{0x33, 0x03},
	{0x31, 0x06},
	{0x33, 0x03},
	{0x01, 0x03},
	{0x19, 0x10},
	{0x42, 0x55},
	{0x43, 0x00},
	{0x47, 0x07},
	{0x48, 0x08},
	{0xb2, 0x3f},
	{0xb3, 0x5b},
	{0xbd, 0x08},
	{0xd2, 0x54},
	{0xd3, 0x0a},
	{0xd4, 0x08},
	{0xd5, 0x08},
	{0xd6, 0x06},
	{0xb1, 0x00},
	{0xb4, 0x00},
	{0xb7, 0x0a},
	{0xbc, 0x44},
	{0xbf, 0x48},
	{0xc3, 0x24},
	{0xc8, 0x03},
	{0xe1, 0x33},
	{0xe2, 0xbb},
	{0x51, 0x0c},
	{0x52, 0x0a},
	{0x57, 0x8c},
	{0x59, 0x09},
	{0x5a, 0x08},
	{0x5e, 0x10},
	{0x60, 0x02},
	{0x6d, 0x5c},
	{0x76, 0x16},
	{0x7c, 0x1a},
	{0x90, 0x28},
	{0x91, 0x16},
	{0x92, 0x1c},
	{0x93, 0x24},
	{0x95, 0x48},
	{0x9c, 0x06},
	{0xca, 0x0c},
	{0xce, 0x0d},
	{0xfd, 0x01},
	{0xc0, 0x00},
	{0xdd, 0x18},
	{0xde, 0x19},
	{0xdf, 0x32},
	{0xe0, 0x70},
	{0xfd, 0x01},
	{0xc2, 0x05},
	{0xd7, 0x88},
	{0xd8, 0x77},
	{0xd9, 0x00},
	{0xfd, 0x07},
	{0x00, 0xf8},
	{0x01, 0x2b},
	{0x05, 0x40},
	{0x08, 0x03},
	{0x09, 0x08},
	{0x28, 0x6f},
	{0x2a, 0x20},
	{0x2b, 0x05},
	{0x2c, 0x01},
	{0x50, 0x02},
	{0x51, 0x03},
	{0x5e, 0x00},
	{0x52, 0x00},
	{0x53, 0x7c},
	{0x54, 0x00},
	{0x55, 0x7c},
	{0x56, 0x00},
	{0x57, 0x7c},
	{0x58, 0x00},
	{0x59, 0x7c},
	{0xfd, 0x02},
	{0x9a, 0x30},
	{0xa8, 0x02},
	{0xfd, 0x02},
	{0xa9, 0x04},
	{0xaa, 0xd0},
	{0xab, 0x06},
	{0xac, 0x68},
	{0xa1, 0x04},
	{0xa2, 0x04},
	{0xa3, 0xc8},
	{0xa5, 0x04},
	{0xa6, 0x06},
	{0xa7, 0x60},
	{0xfd, 0x05},
	{0x06, 0x80},
	{0x18, 0x06},
	{0x19, 0x68},
	{0xfd, 0x00},
	{0x24, 0x01},
	{0xc0, 0x16},
	{0xc1, 0x08},
	{0xc2, 0x30},
	{0x8e, 0x06},
	{0x8f, 0x60},
	{0x90, 0x04},
	{0x91, 0xc8},
	{0x93, 0x0e},
	{0x94, 0x77},
	{0x95, 0x77},
	{0x96, 0x10},
	{0x98, 0x88},
	{0x9c, 0x1a},
	{0xfd, 0x05},
	{0x04, 0x40},
	{0x07, 0x99},
	{0x0D, 0x03},
	{0x0F, 0x03},
	{0x10, 0x00},
	{0x11, 0x00},
	{0x12, 0x0C},
	{0x13, 0xCF},
	{0x14, 0x00},
	{0x15, 0x00},
	{0xfd, 0x00},
	{0x20, 0x0f},
	{0xe7, 0x03},
	{0xe7, 0x00},
	{0xa0, 0x00},

#ifdef FEATURE_OTP // enable lsc otp
// otp LSC_en
#else
// otp LSC_disable
#endif
};

static const SENSOR_REG_T ov08d10_syp_preview_setting[] = {
	/*;----------------------------------------------
	; MCLK: 24Mhz
	; resolution: 1632x1224
	; Mipi : 2 lane
	; Mipi data rate: 360Mbps/Lane
	; SystemCLK     :18Mhz 
	; FPS	        : 30.07fps
	; HTS		:478(P1:0x37/38)
	; VTS		:1252(P1:0x34/0x35/0x36)
	; Tline 	:26.556us
	; mipi is gated clock mode
	; mirror/flip normal
	;---------------------------------------------*/
	{0xfd, 0x00},
	{0x1d, 0x00},
	{0x1c, 0x19},
	{0x14, 0x22},
	{0x1b, 0x78},
	{0xfd, 0x01},
	{0x1a, 0x0a},
	{0x1b, 0x08},
	{0x2a, 0x01},
	{0x2b, 0x9a},
	{0xfd, 0x01},
	{0x12, 0x00},
	{0x03, 0x08},
	{0x04, 0xd4},
	{0x06, 0x00},
	{0x24, 0x30},
	{0x33, 0x03},
	{0x31, 0x06},
	{0x33, 0x03},
	{0x01, 0x03},
	{0x7c, 0x1a},
	{0xfd, 0x07},
	{0x08, 0x03},
	{0x09, 0x08},
	{0x2c, 0x01},
	{0x50, 0x02},
	{0x51, 0x03},
	{0x5e, 0x00},
	{0xfd, 0x02},
	{0x9a, 0x30},
	{0xa9, 0x04},
	{0xaa, 0xd0},
	{0xab, 0x06},
	{0xac, 0x68},
	{0xa0, 0x00},
	{0xa1, 0x04},
	{0xa2, 0x04},
	{0xa3, 0xc8},
	{0xa4, 0x00},
	{0xa5, 0x04},
	{0xa6, 0x06},
	{0xa7, 0x60},
	{0xfd, 0x05},
	{0x06, 0x80},
	{0x18, 0x06},
	{0x19, 0x68},
	{0xfd, 0x00},
	{0x8e, 0x06},
	{0x8f, 0x60},
	{0x90, 0x04},
	{0x91, 0xc8},
	{0x93, 0x0e},
	{0x94, 0x77},
	{0x95, 0x77},
	{0x96, 0x10},
	{0x98, 0x88},
	{0x9c, 0x1a},
	{0xfd, 0x05},
	{0x04, 0x40},
	{0x07, 0x99},
	{0x0D, 0x03},
	{0x0F, 0x03},
	{0xfd, 0x01},
#ifdef IMAGE_NORMAL_MIRROR

#endif
#ifdef IMAGE_H_MIRROR

#endif
#ifdef IMAGE_V_MIRROR

#endif
#ifdef IMAGE_HV_MIRROR

#endif

};

static const SENSOR_REG_T ov08d10_syp_snapshot_setting[] = {
	/*;----------------------------------------------
	; MCLK: 24Mhz
	; resolution: 3264x2448
	; Mipi : 2 lane
	; Mipi data rate: 1440Mbps/Lane
	; SystemCLK     :36Mhz 
	; FPS	        : 30.01fps
	; HTS		:460(P1:0x37/38)
	; VTS		:2608(P1:0x34/0x35/0x36)
	; Tline 	:12.778us
	; mipi is gated clock mode
	; mirror/flip normal
	;---------------------------------------------*/
	{0xfd, 0x00},
	{0x1d, 0x13},
	{0x1c, 0x09},
	{0x14, 0x43},
	{0x1b, 0xf0},
	{0xfd, 0x01},
	{0x1a, 0x14},
	{0x1b, 0x10},
	{0x2a, 0x03},
	{0x2b, 0x34},
	{0xfd, 0x01},
	{0x03, 0x12},
	{0x04, 0x58},
	{0x06, 0xd0},
	{0x24, 0x30},
	{0x33, 0x03},
	{0x31, 0x00},
	{0x33, 0x03},
	{0x01, 0x01},
	{0x7c, 0x11},
	{0xfd, 0x07},
	{0x08, 0x06},
	{0x09, 0x11},
	{0x2c, 0x00},
	{0x50, 0x04},
	{0x51, 0x07},
	{0x5e, 0x40},
	{0xfd, 0x02},
	{0x9a, 0x30},
	{0xa9, 0x09},
	{0xaa, 0xa0},
	{0xab, 0x0c},
	{0xac, 0xd0},
	{0xa0, 0x00},
	{0xa1, 0x08},
	{0xa2, 0x09},
	{0xa3, 0x90},
	{0xa4, 0x00},
	{0xa5, 0x08},
	{0xa6, 0x0c},
	{0xa7, 0xc0},
	{0xfd, 0x05},
	{0x06, 0x00},
	{0x18, 0x00},
	{0x19, 0x00},
	{0xfd, 0x00},
	{0x8e, 0x0c},
	{0x8f, 0xc0},
	{0x90, 0x09},
	{0x91, 0x90},
	{0x93, 0x14},
	{0x94, 0xcc},
	{0x95, 0xba},
	{0x96, 0x16},
	{0x98, 0xee},
	{0x9c, 0x32},
	{0xfd, 0x05},
	{0x04, 0x40},
	{0x07, 0x00},
	{0x0D, 0x01},
	{0x0F, 0x01},
	{0xfd, 0x01},
#ifdef IMAGE_NORMAL_MIRROR

#endif
#ifdef IMAGE_H_MIRROR

#endif
#ifdef IMAGE_V_MIRROR

#endif
#ifdef IMAGE_HV_MIRROR

#endif

};

static const SENSOR_REG_T ov08d10_syp_video_setting[] = {
	/*;----------------------------------------------
	; MCLK: 24Mhz
	; resolution: 1280x720
	; Mipi : 2 lane
	; Mipi data rate: 720Mbps/Lane
	; SystemCLK     :36Mhz 
	; FPS	        : 60.15fps
	; HTS		:478(P1:0x37/38)
	; VTS		:1252(P1:0x34/0x35/0x36)
	; Tline 	:13.278us
	; mipi is gated clock mode
	; mirror/flip normal
	;---------------------------------------------*/
	{0xfd, 0x00},
	{0x1d, 0x00},
	{0x1c, 0x19},
	{0x14, 0x43},
	{0x1b, 0xf0},
	{0xfd, 0x01},
	{0x1a, 0x0a},
	{0x1b, 0x08},
	{0x2a, 0x01},
	{0x2b, 0x9a},
	{0xfd, 0x01},
	{0x12, 0x00},
	{0x03, 0x05},
	{0x04, 0xe2},
	{0x06, 0x00},
	{0x24, 0x30},
	{0x33, 0x03},
	{0x31, 0x06},
	{0x33, 0x03},
	{0x01, 0x03},
	{0x7c, 0x1a},
	{0xfd, 0x07},
	{0x08, 0x03},
	{0x09, 0x08},
	{0x2c, 0x01},
	{0x50, 0x02},
	{0x51, 0x03},
	{0x5e, 0x00},
	{0xfd, 0x02},
	{0x9a, 0x30},
	{0xa9, 0x04},
	{0xaa, 0xd0},
	{0xab, 0x06},
	{0xac, 0x68},
	{0xa0, 0x01},
	{0xa1, 0x00},
	{0xa2, 0x02},
	{0xa3, 0xD0},
	{0xa4, 0x00},
	{0xa5, 0xB4},
	{0xa6, 0x05},
	{0xa7, 0x00},
	{0xfd, 0x05},
	{0x06, 0x80},
	{0x18, 0x06},
	{0x19, 0x68},
	{0xfd, 0x00},
	{0x8e, 0x05},
	{0x8f, 0x00},
	{0x90, 0x02},
	{0x91, 0xd0},
	{0x93, 0x0e},
	{0x94, 0x77},
	{0x95, 0x77},
	{0x96, 0x10},
	{0x98, 0x88},
	{0x9c, 0x1a},
	{0xfd, 0x05},
	{0x04, 0x40},
	{0x07, 0x99},
	{0x0D, 0x03},
	{0x0F, 0x03},
	{0xfd, 0x01},
#ifdef IMAGE_NORMAL_MIRROR

#endif
#ifdef IMAGE_H_MIRROR

#endif
#ifdef IMAGE_V_MIRROR

#endif
#ifdef IMAGE_HV_MIRROR

#endif

};

static struct sensor_res_tab_info s_ov08d10_syp_resolution_tab_raw[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .reg_tab =
         {{ADDR_AND_LEN_OF_ARRAY(ov08d10_syp_init_setting), PNULL, 0, .width = 0,
           .height = 0, .xclk_to_sensor = EX_MCLK,
           .image_format = SENSOR_IMAGE_FORMAT_RAW},

          {ADDR_AND_LEN_OF_ARRAY(ov08d10_syp_video_setting), PNULL, 0,
           .width = VIDEO_WIDTH, .height = VIDEO_HEIGHT,
           .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW},

          {ADDR_AND_LEN_OF_ARRAY(ov08d10_syp_preview_setting), PNULL, 0,
           .width = PREVIEW_WIDTH, .height = PREVIEW_HEIGHT,
           .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW},

          {ADDR_AND_LEN_OF_ARRAY(ov08d10_syp_snapshot_setting), PNULL, 0,
           .width = SNAPSHOT_WIDTH, .height = SNAPSHOT_HEIGHT,
           .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW}}}

    /*If there are multiple modules,please add here*/
};

static SENSOR_TRIM_T s_ov08d10_syp_resolution_trim_tab[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .trim_info =
         {
             {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},

             {.trim_start_x = VIDEO_TRIM_X,
              .trim_start_y = VIDEO_TRIM_Y,
              .trim_width = VIDEO_TRIM_W,
              .trim_height = VIDEO_TRIM_H,
              .line_time = VIDEO_LINE_TIME,
              .bps_per_lane = VIDEO_MIPI_PER_LANE_BPS,
              .frame_line = VIDEO_FRAME_LENGTH,
              .scaler_trim = {.x = VIDEO_TRIM_X,
                              .y = VIDEO_TRIM_Y,
                              .w = VIDEO_TRIM_W,
                              .h = VIDEO_TRIM_H}},

             {.trim_start_x = PREVIEW_TRIM_X,
              .trim_start_y = PREVIEW_TRIM_Y,
              .trim_width = PREVIEW_TRIM_W,
              .trim_height = PREVIEW_TRIM_H,
              .line_time = PREVIEW_LINE_TIME,
              .bps_per_lane = PREVIEW_MIPI_PER_LANE_BPS,
              .frame_line = PREVIEW_FRAME_LENGTH,
              .scaler_trim = {.x = PREVIEW_TRIM_X,
                              .y = PREVIEW_TRIM_Y,
                              .w = PREVIEW_TRIM_W,
                              .h = PREVIEW_TRIM_H}},

             {.trim_start_x = SNAPSHOT_TRIM_X,
              .trim_start_y = SNAPSHOT_TRIM_Y,
              .trim_width = SNAPSHOT_TRIM_W,
              .trim_height = SNAPSHOT_TRIM_H,
              .line_time = SNAPSHOT_LINE_TIME,
              .bps_per_lane = SNAPSHOT_MIPI_PER_LANE_BPS,
              .frame_line = SNAPSHOT_FRAME_LENGTH,
              .scaler_trim = {.x = SNAPSHOT_TRIM_X,
                              .y = SNAPSHOT_TRIM_Y,
                              .w = SNAPSHOT_TRIM_W,
                              .h = SNAPSHOT_TRIM_H}},
         }}

    /*If there are multiple modules,please add here*/

};

static SENSOR_REG_T ov08d10_syp_shutter_reg[] = {
    {0x02, 0x00}, {0x03, 0x00}, {0x04, 0x00},
};

static struct sensor_i2c_reg_tab ov08d10_syp_shutter_tab = {
    .settings = ov08d10_syp_shutter_reg, .size = ARRAY_SIZE(ov08d10_syp_shutter_reg),
};

static SENSOR_REG_T ov08d10_syp_again_reg[] = {
    {0x24, 0x30},
};

static struct sensor_i2c_reg_tab ov08d10_syp_again_tab = {
    .settings = ov08d10_syp_again_reg, .size = ARRAY_SIZE(ov08d10_syp_again_reg),
};

static SENSOR_REG_T ov08d10_syp_dgain_reg[] = {
    {0x21, 0x02}, {0x22, 0x00}, 
};

static struct sensor_i2c_reg_tab ov08d10_syp_dgain_tab = {
    .settings = ov08d10_syp_dgain_reg, .size = ARRAY_SIZE(ov08d10_syp_dgain_reg),
};

static SENSOR_REG_T ov08d10_syp_frame_length_reg[] = {
    //{0x05, 0x00}, {0x06, 0x00},
};

static struct sensor_i2c_reg_tab ov08d10_syp_frame_length_tab = {
    .settings = ov08d10_syp_frame_length_reg,
    .size = ARRAY_SIZE(ov08d10_syp_frame_length_reg),
};

static struct sensor_aec_i2c_tag ov08d10_syp_aec_info = {
    .slave_addr = (I2C_SLAVE_ADDR >> 1),
    .addr_bits_type = SENSOR_I2C_REG_8BIT,
    .data_bits_type = SENSOR_I2C_VAL_8BIT,
    .shutter = &ov08d10_syp_shutter_tab,
    .again = &ov08d10_syp_again_tab,
    .dgain = &ov08d10_syp_dgain_tab,
    .frame_length = &ov08d10_syp_frame_length_tab,
};

static SENSOR_STATIC_INFO_T s_ov08d10_syp_static_info[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .static_info = {.f_num = 220,
                     .focal_length = 242,
                     .max_fps = 30,
                     .max_adgain = 16 * 2,
                     .ois_supported = 0,
                     .pdaf_supported = 0,
                     .exp_valid_frame_num = 1,
                     .clamp_level = 64,
                     .adgain_valid_frame_num = 0,
                     .fov_info = {{3.656f, 2.742f}, 2.560f}}}
    /*If there are multiple modules,please add here*/
};

static SENSOR_MODE_FPS_INFO_T s_ov08d10_syp_mode_fps_info[VENDOR_NUM] = {
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
       {SENSOR_MODE_SNAPSHOT_TWO_THIRD, 0, 1, 0, 0}}}}
    /*If there are multiple modules,please add here*/
};

static struct sensor_module_info s_ov08d10_syp_module_info_tab[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .module_info = {/*.major_i2c_addr = 0x20 >> 1,
                     .minor_i2c_addr = 0x6c >> 1,*/
                     .major_i2c_addr = I2C_SLAVE_ADDR >> 1,
                     .minor_i2c_addr = I2C_SLAVE_ADDR >> 1,
                     .reg_addr_value_bits = SENSOR_I2C_REG_8BIT |
                                            SENSOR_I2C_VAL_8BIT |
                                            SENSOR_I2C_FREQ_400,

                     .avdd_val = SENSOR_AVDD_2800MV,
                     .iovdd_val = SENSOR_AVDD_1800MV,
                     .dvdd_val = SENSOR_AVDD_1200MV,

                     .image_pattern = SENSOR_IMAGE_PATTERN_RAWRGB_B,

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

                     .sensor_interface =
                         {
                             .type = SENSOR_INTERFACE_TYPE_CSI2,
                             .bus_width = LANE_NUM,
                             .pixel_width = RAW_BITS,
                             .is_loose = 2,
                         },
                     .change_setting_skip_num = 1,
                     .horizontal_view_angle = 65,
                     .vertical_view_angle = 60}}

    /*If there are multiple modules,please add here*/
};

static struct sensor_ic_ops s_ov08d10_syp_ops_tab;
struct sensor_raw_info *s_ov08d10_syp_mipi_raw_info_ptr  = PNULL;
    //&s_ov08d10_syp_mipi_raw_info;

/*==============================================================================
 * Description:
 * sensor all info
 * please modify this variable acording your spec
 *============================================================================*/
SENSOR_INFO_T g_ov08d10_syp_mipi_raw_info = {
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
    .identify_code = {{.reg_addr = ov08d10_syp_PID_ADDR,
                       .reg_value = ov08d10_syp_PID_VALUE},
                      {.reg_addr = ov08d10_syp_VER_ADDR,
                       .reg_value = ov08d10_syp_VER_VALUE}},

    .source_width_max = SNAPSHOT_WIDTH,
    .source_height_max = SNAPSHOT_HEIGHT,
    .name = (cmr_s8 *)SENSOR_NAME,
    .image_format = SENSOR_IMAGE_FORMAT_RAW,

    .module_info_tab = s_ov08d10_syp_module_info_tab,
    .module_info_tab_size = ARRAY_SIZE(s_ov08d10_syp_module_info_tab),

    .resolution_tab_info_ptr = s_ov08d10_syp_resolution_tab_raw,
    .sns_ops = &s_ov08d10_syp_ops_tab,
    .raw_info_ptr = &s_ov08d10_syp_mipi_raw_info_ptr,

    .video_tab_info_ptr = NULL,
    .sensor_version_info = (cmr_s8 *)"ov08d10_syp_v1",
};

#endif
