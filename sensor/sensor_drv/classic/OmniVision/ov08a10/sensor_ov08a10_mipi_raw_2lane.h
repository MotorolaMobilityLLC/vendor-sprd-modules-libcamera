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

#ifndef _SENSOR_ov08a10_MIPI_RAW_H_
#define _SENSOR_ov08a10_MIPI_RAW_H_

#define LOG_TAG "ov08a10_shine_2lane"

#include <utils/Log.h>
#include "sensor.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"

//#include "parameters/parameters_sharkl5/sensor_ov08a10_raw_param_main.c"

#define VENDOR_NUM 1
#define SENSOR_NAME "ov08a10_mipi_raw"

#define MAJOR_I2C_SLAVE_ADDR 0x6C
#define MINOR_I2C_SLAVE_ADDR 0x20

#define ov08a10_PID_ADDR 0x300B
#define ov08a10_PID_VALUE 0x08
#define ov08a10_VER_ADDR 0x300C
#define ov08a10_VER_VALUE 0x41

/* sensor parameters begin */

/* effective sensor output image size */
#define VIDEO_WIDTH 1920
#define VIDEO_HEIGHT 1080
#define PREVIEW_WIDTH 1632
#define PREVIEW_HEIGHT 1224
#define SNAPSHOT_WIDTH 3264
#define SNAPSHOT_HEIGHT 2448

/*Raw Trim parameters*/
#define VIDEO_TRIM_X 0
#define VIDEO_TRIM_Y 0
#define VIDEO_TRIM_W 1920
#define VIDEO_TRIM_H 1080
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

#define VIDEO_MIPI_PER_LANE_BPS 1524
#define PREVIEW_MIPI_PER_LANE_BPS 1524
#define SNAPSHOT_MIPI_PER_LANE_BPS 1524

/*line time unit: 1ns*/
#define VIDEO_LINE_TIME 11275
#define PREVIEW_LINE_TIME 11275
#define SNAPSHOT_LINE_TIME 11275

/* frame length*/
#define VIDEO_FRAME_LENGTH 2956
#define PREVIEW_FRAME_LENGTH 2956
#define SNAPSHOT_FRAME_LENGTH 5924

/* please ref your spec */
#define FRAME_OFFSET 12
#define SENSOR_MAX_GAIN 0xF80 // x15.5 A gain
#define SENSOR_BASE_GAIN 0x100
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

/*==============================================================================
 * Description:
 * register setting
 *============================================================================*/
static const SENSOR_REG_T ov08a10_init_setting[] = {
    // global setting
    /*
    mclk = 160MHz
    mipi_clk = 762MHz
    resolution =3264x2448 W*H
    fps = 30
    line_time = 11.275us
    min_line = 8
    ob_value = 64
    base_gain = 1x
    ob_value @ max_gain  = 64@15.5x     xxx @ yyy
    bayer pattern = BGGR
    */
    {0x0103, 0x01},
    {0xffff, 0x01},
    {0x0100, 0x00},
    {0x0102, 0x01},
    {0x0304, 0x03},
    {0x0305, 0x20},
    {0x0306, 0x01},
    {0x0307, 0x00},
    {0x0323, 0x04},
    {0x0324, 0x01},
    {0x0325, 0x40},
    {0x4837, 0x0a},
    {0x3009, 0x06},
    {0x3012, 0x21},
    {0x301e, 0x98},
    {0x3026, 0x10},
    {0x3027, 0x08},
    {0x3106, 0x80},
    {0x3216, 0x01},
    {0x3217, 0x00},
    {0x3218, 0x00},
    {0x3219, 0x55},
    {0x3400, 0x00},
    {0x3408, 0x02},
    {0x340c, 0x20},
    {0x340d, 0x00},
    {0x3410, 0x00},
    {0x3412, 0x00},
    {0x3413, 0x00},
    {0x3414, 0x00},
    {0x3415, 0x00},
    {0x3501, 0x17},
    {0x3502, 0x04},
    {0x3504, 0x08},
    {0x3508, 0x04},
    {0x3509, 0x00},
    {0x353c, 0x04},
    {0x353d, 0x00},
    {0x3600, 0x20},
    {0x3608, 0x87},
    {0x3609, 0xe0},
    {0x360a, 0x66},
    {0x360c, 0x20},
    {0x361a, 0x80},
    {0x361b, 0xd0},
    {0x361c, 0x11},
    {0x361d, 0x63},
    {0x361e, 0x76},
    {0x3620, 0x50},
    {0x3621, 0x0a},
    {0x3622, 0x8a},
    {0x3625, 0x88},
    {0x3626, 0x49},
    {0x362a, 0x80},
    {0x3632, 0x00},
    {0x3633, 0x10},
    {0x3634, 0x10},
    {0x3635, 0x10},
    {0x3636, 0x0e},
    {0x3659, 0x11},
    {0x365a, 0x23},
    {0x365b, 0x38},
    {0x365c, 0x80},
    {0x3661, 0x0c},
    {0x3663, 0x40},
    {0x3665, 0x12},
    {0x3668, 0xf0},
    {0x3669, 0x0a},
    {0x366a, 0x10},
    {0x366b, 0x43},
    {0x366c, 0x02},
    {0x3674, 0x00},
    {0x3706, 0x1b},
    {0x3709, 0x25},
    {0x370b, 0x3f},
    {0x370c, 0x03},
    {0x3713, 0x02},
    {0x3714, 0x63},
    {0x3726, 0x20},
    {0x373b, 0x06},
    {0x373d, 0x0a},
    {0x3752, 0x00},
    {0x3753, 0x00},
    {0x3754, 0xee},
    {0x3767, 0x08},
    {0x3768, 0x0e},
    {0x3769, 0x02},
    {0x376a, 0x00},
    {0x376b, 0x00},
    {0x37d9, 0x08},
    {0x37dc, 0x00},
    {0x3800, 0x00},
    {0x3801, 0x00},
    {0x3802, 0x00},
    {0x3803, 0x08},
    {0x3804, 0x0c},
    {0x3805, 0xdf},
    {0x3806, 0x09},
    {0x3807, 0xa7},
    {0x3808, 0x0c},
    {0x3809, 0xc0},
    {0x380a, 0x09},
    {0x380b, 0x90},
    {0x380c, 0x03},
    {0x380d, 0x86},
    {0x380e, 0x17},
    {0x380f, 0x1a},
    {0x3810, 0x00},
    {0x3811, 0x11},
    {0x3812, 0x00},
    {0x3813, 0x08},
    {0x3814, 0x11},
    {0x3815, 0x11},
    {0x3820, 0x80},
    {0x3821, 0x04},
    {0x3823, 0x00},
    {0x3824, 0x00},
    {0x3825, 0x00},
    {0x3826, 0x00},
    {0x3827, 0x00},
    {0x382b, 0x08},
    {0x3834, 0xf4},
    {0x3835, 0x04},
    {0x3836, 0x14},
    {0x3837, 0x04},
    {0x3898, 0x00},
    {0x38a0, 0x02},
    {0x38a1, 0x02},
    {0x38a2, 0x02},
    {0x38a3, 0x04},
    {0x38c3, 0x00},
    {0x38c4, 0x00},
    {0x38c5, 0x00},
    {0x38c6, 0x00},
    {0x38c7, 0x00},
    {0x38c8, 0x00},
    {0x3d8c, 0x60},
    {0x3d8d, 0x30},
    {0x3f00, 0x8b},
    {0x4000, 0xf7},
    {0x4001, 0x60},
    {0x4002, 0x00},
    {0x4003, 0x40},
    {0x4008, 0x02},
    {0x4009, 0x11},
    {0x400a, 0x01},
    {0x400b, 0x00},
    {0x4020, 0x00},
    {0x4021, 0x00},
    {0x4022, 0x00},
    {0x4023, 0x00},
    {0x4024, 0x00},
    {0x4025, 0x00},
    {0x4026, 0x00},
    {0x4027, 0x00},
    {0x4030, 0x00},
    {0x4031, 0x00},
    {0x4032, 0x00},
    {0x4033, 0x00},
    {0x4034, 0x00},
    {0x4035, 0x00},
    {0x4036, 0x00},
    {0x4037, 0x00},
    {0x4040, 0x00},
    {0x4041, 0x07},
    {0x4201, 0x00},
    {0x4202, 0x00},
    {0x4204, 0x09},
    {0x4205, 0x00},
    {0x4300, 0xff},
    {0x4301, 0x00},
    {0x4302, 0x0f},
    {0x4500, 0x08},
    {0x4501, 0x00},
    {0x450b, 0x00},
    {0x4640, 0x01},
    {0x4641, 0x04},
    {0x4645, 0x03},
    {0x4800, 0x60},
    {0x4803, 0x18},
    {0x4809, 0x2b},
    {0x480e, 0x02},
    {0x4813, 0x90},
    {0x481b, 0x3c},
    {0x4847, 0x01},
    {0x4850, 0x5d},
    {0x4854, 0x0d},
    {0x4856, 0x7a},
    {0x4888, 0x90},
    {0x4901, 0x00},
    {0x4902, 0x00},
    {0x4904, 0x09},
    {0x4905, 0x00},
    {0x5000, 0x89},
    {0x5001, 0x5a},
    {0x5002, 0x51},
    {0x5005, 0xd0},
    {0x5007, 0xa0},
    {0x500a, 0x02},
    {0x500b, 0x02},
    {0x500c, 0x0a},
    {0x500d, 0x0a},
    {0x500e, 0x02},
    {0x500f, 0x06},
    {0x5010, 0x0a},
    {0x5011, 0x0e},
    {0x5013, 0x00},
    {0x5015, 0x00},
    {0x5017, 0x10},
    {0x5019, 0x00},
    {0x501b, 0xc0},
    {0x501d, 0xa0},
    {0x501e, 0x00},
    {0x501f, 0x40},
    {0x5058, 0x00},
    {0x5081, 0x00},
    {0x5180, 0x00},
    {0x5181, 0x3c},
    {0x5182, 0x01},
    {0x5183, 0xfc},
    {0x5200, 0x4f},
    {0x5203, 0x07},
    {0x5208, 0xff},
    {0x520a, 0x3f},
    {0x520b, 0xc0},
    {0x520c, 0x05},
    {0x520d, 0xc8},
    {0x520e, 0x3f},
    {0x520f, 0x0f},
    {0x5210, 0x0a},
    {0x5218, 0x02},
    {0x5219, 0x01},
    {0x521b, 0x02},
    {0x521c, 0x01},
    {0x58cb, 0x03},
};

static const SENSOR_REG_T ov08a10_video_setting[] = {
    // register	value
    /*
    mclk =24MHz
    mipi_clk = 800MHz
    resolution =1920x1080 W*H
    fps = 60
    line_time = 11.275us
    min_line = 8
    ob_value = 64
    base_gain = 1x
    ob_value @ max_gain  =64@15.5x      xxx @ yyy
    bayer pattern = BGGR
    */
    {0x0100,0x00},
    {0x0304,0x02},
    {0x0305,0xfa},
    {0x4837,0x0a},
    {0x3012,0x21},
    {0x3501,0x0b},
    {0x3502,0x00},
    {0x3714,0x63},
    {0x3726,0x20},
    {0x373b,0x06},
    {0x373d,0x0a},
    {0x3752,0x00},
    {0x3753,0x00},
    {0x3754,0xee},
    {0x3767,0x08},
    {0x3768,0x0e},
    {0x3769,0x02},
    {0x376a,0x00},
    {0x376b,0x00},
    {0x37d9,0x08},
    {0x37dc,0x00},
    {0x3800,0x01},
    {0x3801,0xf0},
    {0x3802,0x02},
    {0x3803,0xa8},
    {0x3804,0x0a},
    {0x3805,0xef},
    {0x3806,0x07},
    {0x3807,0x07},
    {0x3808,0x07},
    {0x3809,0x80},
    {0x380a,0x04},
    {0x380b,0x38},
    {0x380c,0x03},
    {0x380d,0x86},
    {0x380e,0x0b},
    {0x380f,0x8c},
    {0x3811,0xc1},
    {0x3813,0x14},
    {0x3814,0x11},
    {0x3815,0x11},
    {0x3820,0x80},
    {0x3821,0x04},
    {0x4008,0x02},
    {0x4009,0x11},
    {0x4041,0x07},
    {0x4501,0x00},
    {0x4856,0x58},
    {0x5001,0x5a},
    //{0x0100,0x01},
};

static const SENSOR_REG_T ov08a10_preview_setting[] = {
    // register	value
    /*
    mclk = 24MHz
    mipi_clk = 800MHz
    resolution =1632x1224 W*H
    fps = 60
    line_time = 11.275us
    min_line = 8
    ob_value = 64
    base_gain = 1x
    ob_value @ max_gain  =64@15.5x      xxx @ yyy
    bayer pattern = BGGR
    */
    {0x0100,0x00},
    {0x0304,0x02},
    {0x0305,0xfa},
    {0x4837,0x0a},
    {0x3012,0x21},
    {0x3501,0x0b},
    {0x3502,0x6c},
    {0x3714,0x67},
    {0x3800,0x00},
    {0x3801,0x00},
    {0x3802,0x00},
    {0x3803,0x08},
    {0x3804,0x0c},
    {0x3805,0xdf},
    {0x3806,0x09},
    {0x3807,0xa7},
    {0x3808,0x06},
    {0x3809,0x60},
    {0x380a,0x04},
    {0x380b,0xc8},
    {0x380c,0x03},
    {0x380d,0x86},
    {0x380e,0x0b},
    {0x380f,0x8c},
    {0x3811,0x07},
    {0x3813,0x04},
    {0x3814,0x31},
    {0x3815,0x31},
    {0x3820,0x81},
    {0x3821,0x44},
    {0x4008,0x01},
    {0x4009,0x08},
    {0x4041,0x03},
    {0x4501,0x0c},
    {0x4856,0x58},
    {0x5001,0x4a},
    //{0x0100,0x01},
};


static const SENSOR_REG_T ov08a10_snapshot_setting[] = {
    // Full size  setting
    /*
    mclk = 24MHz
    mipi_clk = 762MHz
    resolution =3264x2448 W*H
    fps = 30
    line_time = 11.275us
    min_line = 8
    ob_value = 64
    base_gain = 1x
    ob_value @ max_gain  = 64@15.5x     xxx @ yyy
    bayer pattern = BGGR
    */
    {0x0100,0x00},
    {0x0304,0x02},
    {0x0305,0xfa},
    {0x4837,0x0a},
    {0x3012,0x21},
    {0x3501,0x17},
    {0x3502,0x04},
    {0x3714,0x63},
    {0x3800,0x00},
    {0x3801,0x00},
    {0x3802,0x00},
    {0x3803,0x08},
    {0x3804,0x0c},
    {0x3805,0xdf},
    {0x3806,0x09},
    {0x3807,0xa7},
    {0x3808,0x0c},
    {0x3809,0xc0},
    {0x380a,0x09},
    {0x380b,0x90},
    {0x380c,0x03},
    {0x380d,0x86},
    {0x380e,0x17},
    {0x380f,0x24},
    {0x3811,0x11},
    {0x3813,0x08},
    {0x3814,0x11},
    {0x3815,0x11},
    {0x3820,0x80},
    {0x3821,0x04},
    {0x4008,0x02},
    {0x4009,0x11},
    {0x4041,0x07},
    {0x4501,0x00},
    {0x4856,0x58},
    {0x5001,0x5a},
    //{0x0100,0x01},
};

static struct sensor_res_tab_info s_ov08a10_resolution_tab_raw[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .reg_tab =
         {{ADDR_AND_LEN_OF_ARRAY(ov08a10_init_setting), PNULL, 0, .width = 0,
           .height = 0, .xclk_to_sensor = EX_MCLK,
           .image_format = SENSOR_IMAGE_FORMAT_RAW},

          {ADDR_AND_LEN_OF_ARRAY(ov08a10_preview_setting), PNULL, 0,
           .width = PREVIEW_WIDTH, .height = PREVIEW_HEIGHT,
           .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW},

          {ADDR_AND_LEN_OF_ARRAY(ov08a10_video_setting), PNULL, 0,
           .width = VIDEO_WIDTH, .height = VIDEO_HEIGHT,
           .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW},

          {ADDR_AND_LEN_OF_ARRAY(ov08a10_snapshot_setting), PNULL, 0,
           .width = SNAPSHOT_WIDTH, .height = SNAPSHOT_HEIGHT,
           .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW}}}

    /*If there are multiple modules,please add here*/
};

static SENSOR_TRIM_T s_ov08a10_resolution_trim_tab[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .trim_info =
         {
             {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},

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

static SENSOR_REG_T ov08a10_shutter_reg[] = {
    {0x3501, 0x00}, {0x3502, 0x00}
};

static struct sensor_i2c_reg_tab ov08a10_shutter_tab = {
    .settings = ov08a10_shutter_reg, .size = ARRAY_SIZE(ov08a10_shutter_reg),
};

static SENSOR_REG_T ov08a10_again_reg[] = {
    {0x3508, 0x01}, {0x3509, 0x00},
};

static struct sensor_i2c_reg_tab ov08a10_again_tab = {
    .settings = ov08a10_again_reg, .size = ARRAY_SIZE(ov08a10_again_reg),
};

static SENSOR_REG_T ov08a10_dgain_reg[] = {
    {0x5100, 0x04},{0x5101, 0x00},{0x5102, 0x04},{0x5103, 0x00},
    {0x5104, 0x04},{0x5105, 0x04},{0x5106, 0x04},{0x5107, 0x00},
};

static struct sensor_i2c_reg_tab ov08a10_dgain_tab = {
    .settings = ov08a10_dgain_reg, .size = ARRAY_SIZE(ov08a10_dgain_reg),
};

static SENSOR_REG_T ov08a10_frame_length_reg[] = {
    {0x380e, 0x00}, {0x380f, 0x00},
};

static struct sensor_i2c_reg_tab ov08a10_frame_length_tab = {
    .settings = ov08a10_frame_length_reg,
    .size = ARRAY_SIZE(ov08a10_frame_length_reg),
};

static struct sensor_aec_i2c_tag ov08a10_aec_info = {
    .slave_addr = (MAJOR_I2C_SLAVE_ADDR >> 1),
    .addr_bits_type = SENSOR_I2C_REG_16BIT,
    .data_bits_type = SENSOR_I2C_VAL_8BIT,
    .shutter = &ov08a10_shutter_tab,
    .again = &ov08a10_again_tab,
    .dgain = &ov08a10_dgain_tab,
    .frame_length = &ov08a10_frame_length_tab,
};

static SENSOR_STATIC_INFO_T s_ov08a10_static_info[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .static_info = {.f_num = 200,
                     .focal_length = 354,
                     .max_fps = 60,
                     .max_adgain = 15.5,
                     .ois_supported = 0,
                     .pdaf_supported = 3,
                     .exp_valid_frame_num = 1,
                     .clamp_level = 64,
                     .adgain_valid_frame_num = 1,
                     .fov_info = {{5.120f, 3.840f}, 4.042f}}}
    /*If there are multiple modules,please add here*/
};

static SENSOR_MODE_FPS_INFO_T s_ov08a10_mode_fps_info[VENDOR_NUM] = {
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

static struct sensor_module_info s_ov08a10_module_info_tab[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .module_info = {.major_i2c_addr = MAJOR_I2C_SLAVE_ADDR >> 1,
                     .minor_i2c_addr = MINOR_I2C_SLAVE_ADDR >> 1,

                     .reg_addr_value_bits = SENSOR_I2C_REG_16BIT |
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
#if defined  _SENSOR_RAW_SHARKL5PRO_H_ || defined _SENSOR_RAW_SHARKL6_H_
                             .is_loose = 2,
#else
                             .is_loose = 0,
#endif
                         },
                     .change_setting_skip_num = 1,
                     .horizontal_view_angle = 65,
                     .vertical_view_angle = 60}}

    /*If there are multiple modules,please add here*/
};

static struct sensor_ic_ops s_ov08a10_ops_tab;
struct sensor_raw_info *s_ov08a10_mipi_raw_info_ptr =
    PNULL; //&s_ov08a10_mipi_raw_info;

/*==============================================================================
 * Description:
 * sensor all info
 * please modify this variable acording your spec
 *============================================================================*/
SENSOR_INFO_T g_ov08a10_mipi_raw_info = {
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
    .identify_code = {{.reg_addr = ov08a10_PID_ADDR,
                       .reg_value = ov08a10_PID_VALUE},
                       {.reg_addr = ov08a10_VER_ADDR,
                        .reg_value = ov08a10_VER_VALUE}},

    .source_width_max = SNAPSHOT_WIDTH,
    .source_height_max = SNAPSHOT_HEIGHT,
    .name = (cmr_s8 *)SENSOR_NAME,
    .image_format = SENSOR_IMAGE_FORMAT_RAW,

    .module_info_tab = s_ov08a10_module_info_tab,
    .module_info_tab_size = ARRAY_SIZE(s_ov08a10_module_info_tab),

    .resolution_tab_info_ptr = s_ov08a10_resolution_tab_raw,
    .sns_ops = &s_ov08a10_ops_tab,
    .raw_info_ptr = &s_ov08a10_mipi_raw_info_ptr,

    .video_tab_info_ptr = NULL,
    .sensor_version_info = (cmr_s8 *)"ov08a10_v1",
};

#endif
