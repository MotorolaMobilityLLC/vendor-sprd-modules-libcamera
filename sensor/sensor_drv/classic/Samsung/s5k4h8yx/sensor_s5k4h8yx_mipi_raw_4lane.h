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

#ifndef _SENSOR_S5K4H8YX_MIPI_RAW_H_
#define _SENSOR_S5K4H8YX_MIPI_RAW_H_

#include <utils/Log.h>
#include "sensor.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"

//#define FEATURE_OTP

#define VENDOR_NUM 1
#define SENSOR_NAME "s5k4h8yx_mipi_raw"
#define I2C_SLAVE_ADDR 0x5A

#define s5k4h8yx_PID_ADDR 0x00
#define s5k4h8yx_PID_VALUE 0x4088
#define s5k4h8yx_VER_ADDR 0x00
#define s5k4h8yx_VER_VALUE 0x4088

/* sensor parameters begin */

/* effective sensor output image size */
#define VIDEO_WIDTH 1632
#define VIDEO_HEIGHT 1224
#define PREVIEW_WIDTH 1632
#define PREVIEW_HEIGHT 1224
#define SNAPSHOT_WIDTH 3264
#define SNAPSHOT_HEIGHT 2448

/*Raw Trim parameters*/
#define VIDEO_TRIM_X 0
#define VIDEO_TRIM_Y 0
#define VIDEO_TRIM_W VIDEO_WIDTH
#define VIDEO_TRIM_H VIDEO_HEIGHT
#define PREVIEW_TRIM_X 0
#define PREVIEW_TRIM_Y 0
#define PREVIEW_TRIM_W PREVIEW_WIDTH
#define PREVIEW_TRIM_H PREVIEW_HEIGHT
#define SNAPSHOT_TRIM_X 0
#define SNAPSHOT_TRIM_Y 0
#define SNAPSHOT_TRIM_W SNAPSHOT_WIDTH
#define SNAPSHOT_TRIM_H SNAPSHOT_HEIGHT

/*Mipi output*/
#define LANE_NUM 4
#define RAW_BITS 10

#define VIDEO_MIPI_PER_LANE_BPS 700    /* 2*Mipi clk */
#define PREVIEW_MIPI_PER_LANE_BPS 700  /* 2*Mipi clk */
#define SNAPSHOT_MIPI_PER_LANE_BPS 700 /* 2*Mipi clk */

/*line time unit: 1ns*/
#define VIDEO_LINE_TIME 13371
#define PREVIEW_LINE_TIME 13371
#define SNAPSHOT_LINE_TIME 13371

/* frame length*/
#define VIDEO_FRAME_LENGTH 1246
#define PREVIEW_FRAME_LENGTH 2492
#define SNAPSHOT_FRAME_LENGTH 2498

/* please ref your spec */
#define FRAME_OFFSET 8
#define SENSOR_MAX_GAIN 0x0200
#define SENSOR_BASE_GAIN 0x0020
#define SENSOR_MIN_SHUTTER 4

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

/*==============================================================================
 * Description:
 * register setting
 *============================================================================*/

static const SENSOR_REG_T s5k4h8yx_init_setting[] = {
    /*4H8YX13-F1X9 setting*/
    {0x6028, 0x2000}, {0x602A, 0x1FD0}, {0x6F12, 0x0448}, {0x6F12, 0x0349},
    {0x6F12, 0x0160}, {0x6F12, 0xC26A}, {0x6F12, 0x511A}, {0x6F12, 0x8180},
    {0x6F12, 0x00F0}, {0x6F12, 0x60B8}, {0x6F12, 0x2000}, {0x6F12, 0x20E8},
    {0x6F12, 0x2000}, {0x6F12, 0x13A0}, {0x6F12, 0x0000}, {0x6F12, 0x0000},
    {0x6F12, 0x0000}, {0x6F12, 0x0000}, {0x6F12, 0x38B5}, {0x6F12, 0x0021},
    {0x6F12, 0x0446}, {0x6F12, 0x8DF8}, {0x6F12, 0x0010}, {0x6F12, 0x00F5},
    {0x6F12, 0xB470}, {0x6F12, 0x0122}, {0x6F12, 0x6946}, {0x6F12, 0x00F0},
    {0x6F12, 0x59F8}, {0x6F12, 0x9DF8}, {0x6F12, 0x0000}, {0x6F12, 0xFF28},
    {0x6F12, 0x05D0}, {0x6F12, 0x0020}, {0x6F12, 0x08B1}, {0x6F12, 0x04F2},
    {0x6F12, 0x6914}, {0x6F12, 0x2046}, {0x6F12, 0x38BD}, {0x6F12, 0x0120},
    {0x6F12, 0xF8E7}, {0x6F12, 0x10B5}, {0x6F12, 0x92B0}, {0x6F12, 0x0C46},
    {0x6F12, 0x4822}, {0x6F12, 0x6946}, {0x6F12, 0x00F0}, {0x6F12, 0x46F8},
    {0x6F12, 0x0020}, {0x6F12, 0x6946}, {0x6F12, 0x04EB}, {0x6F12, 0x4003},
    {0x6F12, 0x0A5C}, {0x6F12, 0x02F0}, {0x6F12, 0x0F02}, {0x6F12, 0x04F8},
    {0x6F12, 0x1020}, {0x6F12, 0x0A5C}, {0x6F12, 0x401C}, {0x6F12, 0x1209},
    {0x6F12, 0x5A70}, {0x6F12, 0x4828}, {0x6F12, 0xF2D3}, {0x6F12, 0x12B0},
    {0x6F12, 0x10BD}, {0x6F12, 0x2DE9}, {0x6F12, 0xF041}, {0x6F12, 0x164E},
    {0x6F12, 0x0F46}, {0x6F12, 0x06F1}, {0x6F12, 0x1105}, {0x6F12, 0xA236},
    {0x6F12, 0xB0B1}, {0x6F12, 0x1449}, {0x6F12, 0x1248}, {0x6F12, 0x0968},
    {0x6F12, 0x0078}, {0x6F12, 0xB1F8}, {0x6F12, 0x6A10}, {0x6F12, 0xC007},
    {0x6F12, 0x0ED0}, {0x6F12, 0x0846}, {0x6F12, 0xFFF7}, {0x6F12, 0xBEFF},
    {0x6F12, 0x84B2}, {0x6F12, 0x2946}, {0x6F12, 0x2046}, {0x6F12, 0xFFF7},
    {0x6F12, 0xD0FF}, {0x6F12, 0x4FF4}, {0x6F12, 0x9072}, {0x6F12, 0x3146},
    {0x6F12, 0x04F1}, {0x6F12, 0x4800}, {0x6F12, 0x00F0}, {0x6F12, 0x16F8},
    {0x6F12, 0x002F}, {0x6F12, 0x05D0}, {0x6F12, 0x3146}, {0x6F12, 0x2846},
    {0x6F12, 0xBDE8}, {0x6F12, 0xF041}, {0x6F12, 0x00F0}, {0x6F12, 0x13B8},
    {0x6F12, 0xBDE8}, {0x6F12, 0xF081}, {0x6F12, 0x0022}, {0x6F12, 0xAFF2},
    {0x6F12, 0x5501}, {0x6F12, 0x0348}, {0x6F12, 0x00F0}, {0x6F12, 0x10B8},
    {0x6F12, 0x2000}, {0x6F12, 0x0C40}, {0x6F12, 0x2000}, {0x6F12, 0x0560},
    {0x6F12, 0x0000}, {0x6F12, 0x152D}, {0x6F12, 0x48F6}, {0x6F12, 0x296C},
    {0x6F12, 0xC0F2}, {0x6F12, 0x000C}, {0x6F12, 0x6047}, {0x6F12, 0x41F2},
    {0x6F12, 0x950C}, {0x6F12, 0xC0F2}, {0x6F12, 0x000C}, {0x6F12, 0x6047},
    {0x6F12, 0x49F2}, {0x6F12, 0x514C}, {0x6F12, 0xC0F2}, {0x6F12, 0x000C},
    {0x6F12, 0x6047}, {0x6F12, 0x0000}, {0x6F12, 0x0000}, {0x6F12, 0x0000},
    {0x6F12, 0x0000}, {0x6F12, 0x0000}, {0x6F12, 0x4088}, {0x6F12, 0x0166},
    {0x6F12, 0x0000}, {0x6F12, 0x0002}, {0x5360, 0x0004}, {0x3078, 0x0059},
    {0x307C, 0x0025}, {0x36D0, 0x00DD}, {0x36D2, 0x0100}, {0x306A, 0x00EF},
};

static const SENSOR_REG_T s5k4h8yx_preview_setting[] = {
    /*
    2x2 binning size
    1632x1224_30fps_vt280M_4lane_mipi700M
    width	1632
    height	1224
    frame rate	30.01
    mipi_lane_num	4
    mipi_per_lane_bps	700
    line time(0.1us unit)	133.71
    frame length	2492
    Extclk	24M
    max gain	0x0200
    base gain	0x0020
    raw bits	raw10
    bayer patter	Gr first
    OB level	64
    offset	8
    min shutter 4
    */
    {0x6028, 0x4000}, {0x602A, 0x6214}, {0x6F12, 0x7971}, {0x602A, 0x6218},
    {0x6F12, 0x7150}, {0x6028, 0x2000}, {0x602A, 0x0EC6}, {0x6F12, 0x0000},
    {0xF490, 0x0030}, {0xF47A, 0x0012}, {0xF428, 0x0200}, {0xF48E, 0x0010},
    {0xF45C, 0x0004}, {0x0B04, 0x0101}, {0x0B00, 0x0080}, {0x6028, 0x2000},
    {0x602A, 0x0C40}, {0x6F12, 0x0140},

    {0x0200, 0x0618}, {0x0202, 0x0904}, {0x31AA, 0x0004}, {0x1006, 0x0006},
    {0x31FA, 0x0000}, {0x0204, 0x0020}, {0x0344, 0x0008}, {0x0348, 0x0CC7},
    {0x0346, 0x0008}, {0x034A, 0x0997}, {0x034C, 0x0660}, {0x034E, 0x04C8},
    {0x0342, 0x0EA0}, {0x0340, 0x09BC}, {0x0900, 0x0212}, {0x0380, 0x0001},
    {0x0382, 0x0001}, {0x0384, 0x0001}, {0x0386, 0x0003}, {0x0400, 0x0002},
    {0x0404, 0x0020}, {0x0114, 0x0330}, {0x0136, 0x1800}, {0x0300, 0x0005},
    {0x0302, 0x0001}, {0x0304, 0x0006}, {0x0306, 0x00AF}, {0x030C, 0x0006},
    {0x030E, 0x00AF}, {0x3008, 0x0000},
};

static const SENSOR_REG_T s5k4h8yx_snapshot_setting[] = {
    /*
    full size
    3264x2448_30fps_vt280M_4lane_mipi700M
    width	3264
    height	2448
    frame rate	29.94
    mipi_lane_num	4
    mipi_per_lane_bps	700
    line time(0.1us unit)	133.71
    frame length	2498
    Extclk	24M
    max gain	0x0200
    base gain	0x0020
    raw bits	raw10
    bayer patter	Gr first
    OB level	64
    offset	8
    min shutter 4*/
    {0x6028, 0x4000}, {0x602A, 0x6214}, {0x6F12, 0x7971}, {0x602A, 0x6218},
    {0x6F12, 0x7150}, {0x6028, 0x2000}, {0x602A, 0x0EC6}, {0x6F12, 0x0000},
    {0xF490, 0x0030}, {0xF47A, 0x0012}, {0xF428, 0x0200}, {0xF48E, 0x0010},
    {0xF45C, 0x0004}, {0x0B04, 0x0101}, {0x0B00, 0x0080}, {0x6028, 0x2000},
    {0x602A, 0x0C40}, {0x6F12, 0x0140},

    {0x0200, 0x0618}, {0x0202, 0x0904}, {0x31AA, 0x0004}, {0x1006, 0x0006},
    {0x31FA, 0x0000}, {0x0204, 0x0020}, {0x0344, 0x0008}, {0x0348, 0x0CC7},
    {0x0346, 0x0008}, {0x034A, 0x0997}, {0x034C, 0x0CC0}, {0x034E, 0x0990},
    {0x0342, 0x0EA0}, {0x0340, 0x09C2}, {0x0900, 0x0111}, {0x0380, 0x0001},
    {0x0382, 0x0001}, {0x0384, 0x0001}, {0x0386, 0x0001}, {0x0400, 0x0002},
    {0x0404, 0x0010}, {0x0114, 0x0330}, {0x0136, 0x1800}, {0x0300, 0x0005},
    {0x0302, 0x0001}, {0x0304, 0x0006}, {0x0306, 0x00AF}, {0x030C, 0x0006},
    {0x030E, 0x00AF}, {0x3008, 0x0000},

};

static const SENSOR_REG_T s5k4h8yx_video_setting[] = {
    /*
    2x2 binning size
    1632x1224_60fps_vt280M_4lane_mipi700M
    width	1632
    height	1224
    frame rate	60.02
    mipi_lane_num	4
    mipi_per_lane_bps	700
    line time(0.1us unit)	133.71
    frame length	1246
    Extclk	24M
    max gain	0x0200
    base gain	0x0020
    raw bits	raw10
    bayer patter	Gr first
    OB level	64
    offset	8
    min shutter 4
    */
    {0x6028, 0x4000}, {0x602A, 0x6214}, {0x6F12, 0x7971}, {0x602A, 0x6218},
    {0x6F12, 0x7150}, {0x6028, 0x2000}, {0x602A, 0x0EC6}, {0x6F12, 0x0000},
    {0xF490, 0x0030}, {0xF47A, 0x0012}, {0xF428, 0x0200}, {0xF48E, 0x0010},
    {0xF45C, 0x0004}, {0x0B04, 0x0101}, {0x0B00, 0x0080}, {0x6028, 0x2000},
    {0x602A, 0x0C40}, {0x6F12, 0x0140},

    {0x0200, 0x0618}, {0x0202, 0x0904}, {0x31AA, 0x0004}, {0x1006, 0x0006},
    {0x31FA, 0x0000}, {0x0204, 0x0020}, {0x0344, 0x0008}, {0x0348, 0x0CC7},
    {0x0346, 0x0008}, {0x034A, 0x0997}, {0x034C, 0x0660}, {0x034E, 0x04C8},
    {0x0342, 0x0EA0}, {0x0340, 0x04DE}, {0x0900, 0x0212}, {0x0380, 0x0001},
    {0x0382, 0x0001}, {0x0384, 0x0001}, {0x0386, 0x0003}, {0x0400, 0x0002},
    {0x0404, 0x0020}, {0x0114, 0x0330}, {0x0136, 0x1800}, {0x0300, 0x0005},
    {0x0302, 0x0001}, {0x0304, 0x0006}, {0x0306, 0x00AF}, {0x030C, 0x0006},
    {0x030E, 0x00AF}, {0x3008, 0x0000},
};

static struct sensor_res_tab_info s_s5k4h8yx_resolution_tab_raw[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .reg_tab =
         {{ADDR_AND_LEN_OF_ARRAY(s5k4h8yx_init_setting), PNULL, 0, .width = 0,
           .height = 0, .xclk_to_sensor = EX_MCLK,
           .image_format = SENSOR_IMAGE_FORMAT_RAW},

          {ADDR_AND_LEN_OF_ARRAY(s5k4h8yx_video_setting), PNULL, 0,
           .width = VIDEO_WIDTH, .height = VIDEO_HEIGHT,
           .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW},

          {ADDR_AND_LEN_OF_ARRAY(s5k4h8yx_preview_setting), PNULL, 0,
           .width = PREVIEW_WIDTH, .height = PREVIEW_HEIGHT,
           .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW},

          {ADDR_AND_LEN_OF_ARRAY(s5k4h8yx_snapshot_setting), PNULL, 0,
           .width = SNAPSHOT_WIDTH, .height = SNAPSHOT_HEIGHT,
           .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW}}}

    /*If there are multiple modules,please add here*/
};

static SENSOR_TRIM_T s_s5k4h8yx_resolution_trim_tab[VENDOR_NUM] = {
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

static SENSOR_REG_T s5k4h8yx_shutter_reg[] = {
    {0x0202, 0x0000},
};

static struct sensor_i2c_reg_tab s5k4h8yx_shutter_tab = {
    .settings = s5k4h8yx_shutter_reg, .size = ARRAY_SIZE(s5k4h8yx_shutter_reg),
};

static SENSOR_REG_T s5k4h8yx_again_reg[] = {
    {0x0204, 0x0000},
};

static struct sensor_i2c_reg_tab s5k4h8yx_again_tab = {
    .settings = s5k4h8yx_again_reg, .size = ARRAY_SIZE(s5k4h8yx_again_reg),
};

static SENSOR_REG_T s5k4h8yx_dgain_reg[] = {

};

static struct sensor_i2c_reg_tab s5k4h8yx_dgain_tab = {
    .settings = s5k4h8yx_dgain_reg, .size = ARRAY_SIZE(s5k4h8yx_dgain_reg),
};

static SENSOR_REG_T s5k4h8yx_frame_length_reg[] = {
    {0x0340, 0x0000},
};

static struct sensor_i2c_reg_tab s5k4h8yx_frame_length_tab = {
    .settings = s5k4h8yx_frame_length_reg,
    .size = ARRAY_SIZE(s5k4h8yx_frame_length_reg),
};

static struct sensor_aec_i2c_tag s5k4h8yx_aec_info = {
    .slave_addr = (I2C_SLAVE_ADDR >> 1),
    .addr_bits_type = SENSOR_I2C_REG_16BIT,
    .data_bits_type = SENSOR_I2C_VAL_16BIT,
    .shutter = &s5k4h8yx_shutter_tab,
    .again = &s5k4h8yx_again_tab,
    .dgain = &s5k4h8yx_dgain_tab,
    .frame_length = &s5k4h8yx_frame_length_tab,
};

static SENSOR_STATIC_INFO_T s_s5k4h8yx_static_info[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .static_info = {.f_num = 200,
                     .focal_length = 354,
                     .max_fps = 0,
                     .max_adgain = 15 * 2,
                     .ois_supported = 0,
                     .pdaf_supported = 0,
                     .exp_valid_frame_num = 1,
                     .clamp_level = 64,
                     .adgain_valid_frame_num = 1,
                     .fov_info = {{4.614f, 3.444f}, 4.222f}}}
    /*If there are multiple modules,please add here*/
};

static SENSOR_MODE_FPS_INFO_T s_s5k4h8yx_mode_fps_info[VENDOR_NUM] = {
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

static struct sensor_module_info s_s5k4h8yx_module_info_tab[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .module_info = {.major_i2c_addr = I2C_SLAVE_ADDR >> 1,
                     .minor_i2c_addr = I2C_SLAVE_ADDR >> 1,

                     .reg_addr_value_bits = SENSOR_I2C_REG_16BIT |
                                            SENSOR_I2C_VAL_16BIT |
                                            SENSOR_I2C_FREQ_400,

                     .avdd_val = SENSOR_AVDD_2800MV,
                     .iovdd_val = SENSOR_AVDD_1800MV,
                     .dvdd_val = SENSOR_AVDD_1200MV,

                     .image_pattern = SENSOR_IMAGE_PATTERN_RAWRGB_GR,

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
                             .is_loose = 0,
                         },
                     .change_setting_skip_num = 1,
                     .horizontal_view_angle = 65,
                     .vertical_view_angle = 60}}

    /*If there are multiple modules,please add here*/
};

static struct sensor_ic_ops s_s5k4h8yx_ops_tab;
struct sensor_raw_info *s_s5k4h8yx_mipi_raw_info_ptr = PNULL;

/*==============================================================================
 * Description:
 * sensor all info
 * please modify this variable acording your spec
 *============================================================================*/
SENSOR_INFO_T g_s5k4h8yx_mipi_raw_info = {
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
    .identify_code = {{.reg_addr = s5k4h8yx_PID_ADDR,
                       .reg_value = s5k4h8yx_PID_VALUE},
                      {.reg_addr = s5k4h8yx_VER_ADDR,
                       .reg_value = s5k4h8yx_VER_VALUE}},

    .source_width_max = SNAPSHOT_WIDTH,
    .source_height_max = SNAPSHOT_HEIGHT,
    .name = (cmr_s8 *)SENSOR_NAME,
    .image_format = SENSOR_IMAGE_FORMAT_RAW,

    .module_info_tab = s_s5k4h8yx_module_info_tab,
    .module_info_tab_size = ARRAY_SIZE(s_s5k4h8yx_module_info_tab),

    .resolution_tab_info_ptr = s_s5k4h8yx_resolution_tab_raw,
    .sns_ops = &s_s5k4h8yx_ops_tab,
    .raw_info_ptr = &s_s5k4h8yx_mipi_raw_info_ptr,

    .video_tab_info_ptr = NULL,
    .sensor_version_info = (cmr_s8 *)"s5k4h8yx_v1",
};

#endif
