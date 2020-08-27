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

#ifndef _SENSOR_ov02a10_MIPI_RAW_H_
#define _SENSOR_ov02a10_MIPI_RAW_H_

#include <utils/Log.h>
#include "sensor.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"

//#include "parameters/parameters_sharkl5/sensor_ov02a10_raw_param_main.c"

#define VENDOR_NUM 1
#define SENSOR_NAME "ov02a10_mipi_raw"

#define MAJOR_I2C_SLAVE_ADDR 0x7a
#define MINOR_I2C_SLAVE_ADDR 0x7a

#define ov02a10_PID_ADDR 0x02
#define ov02a10_PID_VALUE 0x25
#define ov02a10_VER_ADDR 0x03
#define ov02a10_VER_VALUE 0x09

/* sensor parameters begin */

/* effective sensor output image size */
#define VIDEO_WIDTH 1280
#define VIDEO_HEIGHT 720
#define PREVIEW_WIDTH 800
#define PREVIEW_HEIGHT 600
#define SNAPSHOT_WIDTH 1600
#define SNAPSHOT_HEIGHT 1200

/*Raw Trim parameters*/
#define VIDEO_TRIM_X 0
#define VIDEO_TRIM_Y 0
#define VIDEO_TRIM_W 1280
#define VIDEO_TRIM_H 720
#define PREVIEW_TRIM_X 0
#define PREVIEW_TRIM_Y 0
#define PREVIEW_TRIM_W 800
#define PREVIEW_TRIM_H 600
#define SNAPSHOT_TRIM_X 0
#define SNAPSHOT_TRIM_Y 0
#define SNAPSHOT_TRIM_W 1600
#define SNAPSHOT_TRIM_H 1200

/*Mipi output*/
#define LANE_NUM 1
#define RAW_BITS 10

#define VIDEO_MIPI_PER_LANE_BPS 1560
#define PREVIEW_MIPI_PER_LANE_BPS 1560
#define SNAPSHOT_MIPI_PER_LANE_BPS 1560

/*line time unit: 1ns*/
#define VIDEO_LINE_TIME 26720
#define PREVIEW_LINE_TIME 38270
#define SNAPSHOT_LINE_TIME 23950

/* frame length*/
#define VIDEO_FRAME_LENGTH 1248
#define PREVIEW_FRAME_LENGTH 620
#define SNAPSHOT_FRAME_LENGTH 1391

/* please ref your spec */
#define FRAME_OFFSET 1
#define SENSOR_MAX_GAIN 0xF8
#define SENSOR_BASE_GAIN 0x10
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
static const SENSOR_REG_T ov02a10_init_setting[] = {
    // global setting
    // initial setting
    // register	value	description
    /*mipi_clk =780 MHz
    resolution =1600x1200 W*H
    fps = 30
    line_time = 23.95us
    min_line = 4
    ob_value = 64
    base_gain = 1x
    ob_value @ max_gain  = 64@15.5x */
    {0xfd, 0x00},
    {0x2f, 0x29},
    {0x34, 0x00},
    {0x35, 0x21},
    {0x30, 0x15},
    {0x33, 0x01},
    {0xfd, 0x01},
    {0x44, 0x00},
    {0x2a, 0x4c},
    {0x2b, 0x1e},
    {0x2c, 0x60},
    {0x25, 0x11},
    {0x03, 0x01},
    {0x04, 0xae},
    {0x09, 0x00},
    {0x0a, 0x02},
    {0x06, 0xa6},
    {0x31, 0x00},
    {0x24, 0x40},
    {0x01, 0x01},
    {0xfb, 0x73},
    {0xfd, 0x01},
    {0x16, 0x04},
    {0x1c, 0x09},
    {0x21, 0x42},
    {0x12, 0x04},
    {0x13, 0x10},
    {0x11, 0x40},
    {0x33, 0x81},
    {0xd0, 0x00},
    {0xd1, 0x01},
    {0xd2, 0x00},
    {0x50, 0x10},
    {0x51, 0x23},
    {0x52, 0x20},
    {0x53, 0x10},
    {0x54, 0x02},
    {0x55, 0x20},
    {0x56, 0x02},
    {0x58, 0x48},
    {0x5d, 0x15},
    {0x5e, 0x05},
    {0x66, 0x66},
    {0x68, 0x68},
    {0x6b, 0x00},
    {0x6c, 0x00},
    {0x6f, 0x40},
    {0x70, 0x40},
    {0x71, 0x0a},
    {0x72, 0xf0},
    {0x73, 0x10},
    {0x75, 0x80},
    {0x76, 0x10},
    {0x84, 0x00},
    {0x85, 0x10},
    {0x86, 0x10},
    {0x87, 0x00},
    {0x8a, 0x22},
    {0x8b, 0x22},
    {0x19, 0xf1},
    {0x29, 0x01},
    {0xfd, 0x01},
    {0x9d, 0x16},
    {0xa0, 0x29},
    {0xa1, 0x04},
    {0xad, 0x62},
    {0xae, 0x00},
    {0xaf, 0x85},
    {0xb1, 0x01},
    {0x8e, 0x06},
    {0x8f, 0x40},
    {0x90, 0x04},
    {0x91, 0xb0},
    {0x45, 0x01},
    {0x46, 0x00},
    {0x47, 0x6c},
    {0x48, 0x03},
    {0x49, 0x8b},
    {0x4a, 0x00},
    {0x4b, 0x07},
    {0x4c, 0x04},
    {0x4d, 0xb7},
    {0xf0, 0x40},
    {0xf1, 0x40},
    {0xf2, 0x40},
    {0xf3, 0x40},
    //{0xac, 0x01},
    {0xfd, 0x01},
};

static const SENSOR_REG_T ov02a10_video_setting[] = {
    // register	value
    /*mipi_clk = 780MHz
    resolution =1280x720 W*H
    fps = 30
    line_time = 26.72us
    min_line = 4
    ob_value = 64
    base_gain = 1x
    ob_value @ max_gain  = 64@15.5x*/
    {0xfd, 0x00},
    {0x33, 0x01},
    {0xfd, 0x01},
    {0x03, 0x04},
    {0x04, 0x62},
    {0x06, 0x15},
    {0x24, 0x80},
    {0x30, 0x00},
    {0x31, 0x00},
    {0x33, 0x40},
    {0x66, 0xa6},
    {0x68, 0xa8},
    {0x71, 0x10},
    {0x73, 0x80},
    {0x75, 0xb9},
    {0x76, 0x80},
    {0x8E, 0x05},
    {0x8F, 0x00},
    {0x90, 0x02},
    {0x91, 0xd0},
    {0x9d, 0x16},
    {0x45, 0x01},
    {0x46, 0x00},
    {0x47, 0xc0},
    {0x48, 0x03},
    {0x49, 0x3f},
    {0x4A, 0x00},
    {0x4B, 0xfe},
    {0x4C, 0x03},
    {0x4D, 0xcd},
    {0xd6, 0x80},
    {0xd7, 0x00},
    {0xc9, 0x80},
    {0xca, 0x00},
    {0xd8, 0x00},
    {0xd9, 0x04},
    {0xda, 0xc0},
    {0xdb, 0x00},
    {0xdc, 0x60},
    {0xdd, 0x03},
    {0xde, 0x40},
    {0xdf, 0x00},
    {0xea, 0x20},
    {0xeb, 0x01},
};

static const SENSOR_REG_T ov02a10_preview_setting[] = {
    // register	value
    /*mclk =78 MHz
    mipi_clk =780 MHz
    resolution =800x600 W*H
    fps = 42.15
    line_time = 38.27us
    min_line = 4
    ob_value = 64
    base_gain = 1x
    ob_value @ max_gain  = 64@15.5x  */
    {0xfd, 0x00},
    {0x33, 0x00},
    {0xfd, 0x01},
    {0x03, 0x02},
    {0x04, 0x0a},
    {0x06, 0x04},
    {0x24, 0xf0},
    {0x30, 0x01},
    {0x31, 0x04},
    {0x33, 0x40},
    {0x66, 0xa6},
    {0x68, 0xa8},
    {0x71, 0x10},
    {0x73, 0x80},
    {0x75, 0xb9},
    {0x76, 0x80},
    {0x8E, 0x03},
    {0x8F, 0x20},
    {0x90, 0x02},
    {0x91, 0x58},
    {0x9d, 0x6a},
    {0x45, 0x01},
    {0x46, 0x00},
    {0x47, 0x6c},
    {0x48, 0x03},
    {0x49, 0x8b},
    {0x4A, 0x00},
    {0x4B, 0x01},
    {0x4C, 0x01},
    {0x4D, 0x2d},
    {0xd6, 0x50},
    {0xd7, 0x00},
    {0xc9, 0x20},
    {0xca, 0x00},
    {0xd8, 0x00},
    {0xd9, 0x04},
    {0xda, 0x50},
    {0xdb, 0x00},
    {0xdc, 0x30},
    {0xdd, 0x02},
    {0xde, 0x50},
    {0xdf, 0x00},
    {0xea, 0x30},
    {0xeb, 0x02},
};

static const SENSOR_REG_T ov02a10_snapshot_setting[] = {
    // Full size  setting
    // register	value
    /*mipi_clk =780 MHz
    resolution =1600x1200 W*H
    fps = 30
    line_time = 23.95us
    min_line = 4
    ob_value = 64
    base_gain = 1x
    ob_value @ max_gain  = 64@15.5x */
    {0xfd, 0x00},
    {0x33, 0x01},
    {0xfd, 0x01},
    {0x03, 0x04},
    {0x04, 0xe4},
    {0x06, 0xa6},
    {0x24, 0x40},
    {0x30, 0x00},
    {0x31, 0x00},
    {0x33, 0x81},
    {0x66, 0x66},
    {0x68, 0x68},
    {0x71, 0x0a},
    {0x73, 0x10},
    {0x75, 0x80},
    {0x76, 0x10},
    {0x8E, 0x06},
    {0x8F, 0x40},
    {0x90, 0x04},
    {0x91, 0xb0},
    {0x9d, 0x16},
    {0x45, 0x01},
    {0x46, 0x00},
    {0x47, 0x6c},
    {0x48, 0x03},
    {0x49, 0x8b},
    {0x4A, 0x00},
    {0x4B, 0x07},
    {0x4C, 0x04},
    {0x4D, 0xb7},
    {0xd6, 0x80},
    {0xd7, 0x00},
    {0xc9, 0x80},
    {0xca, 0x00},
    {0xd8, 0x00},
    {0xd9, 0x04},
    {0xda, 0xc0},
    {0xdb, 0x00},
    {0xdc, 0x60},
    {0xdd, 0x03},
    {0xde, 0x40},
    {0xdf, 0x00},
    {0xea, 0x20},
    {0xeb, 0x01},
};

static struct sensor_res_tab_info s_ov02a10_resolution_tab_raw[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .reg_tab =
         {{ADDR_AND_LEN_OF_ARRAY(ov02a10_init_setting), PNULL, 0, .width = 0,
           .height = 0, .xclk_to_sensor = EX_MCLK,
           .image_format = SENSOR_IMAGE_FORMAT_RAW},

           {ADDR_AND_LEN_OF_ARRAY(ov02a10_preview_setting), PNULL, 0,
           .width = PREVIEW_WIDTH, .height = PREVIEW_HEIGHT,
           .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW},

          {ADDR_AND_LEN_OF_ARRAY(ov02a10_video_setting), PNULL, 0,
           .width = VIDEO_WIDTH, .height = VIDEO_HEIGHT,
           .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW},

          {ADDR_AND_LEN_OF_ARRAY(ov02a10_snapshot_setting), PNULL, 0,
           .width = SNAPSHOT_WIDTH, .height = SNAPSHOT_HEIGHT,
           .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW}}}

    /*If there are multiple modules,please add here*/
};

static SENSOR_TRIM_T s_ov02a10_resolution_trim_tab[VENDOR_NUM] = {
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

static SENSOR_REG_T ov02a10_shutter_reg[] = {
    {0xfd, 0x01}, {0x03, 0x01}, {0x04, 0x68}, {0x01, 0x01},
};

static struct sensor_i2c_reg_tab ov02a10_shutter_tab = {
    .settings = ov02a10_shutter_reg, .size = ARRAY_SIZE(ov02a10_shutter_reg),
};

static SENSOR_REG_T ov02a10_again_reg[] = {
    {0xfd, 0x01}, {0x24, 0x20}, {0x01, 0x01},
};

static struct sensor_i2c_reg_tab ov02a10_again_tab = {
    .settings = ov02a10_again_reg, .size = ARRAY_SIZE(ov02a10_again_reg),
};

static SENSOR_REG_T ov02a10_dgain_reg[] = {

};

static struct sensor_i2c_reg_tab ov02a10_dgain_tab = {
    .settings = ov02a10_dgain_reg, .size = ARRAY_SIZE(ov02a10_dgain_reg),
};

static SENSOR_REG_T ov02a10_frame_length_reg[] = {
    {0xfd, 0x01}, {0x0e, 0x04}, {0x0f, 0xc8},{0x01, 0x01},
};

static struct sensor_i2c_reg_tab ov02a10_frame_length_tab = {
    .settings = ov02a10_frame_length_reg,
    .size = ARRAY_SIZE(ov02a10_frame_length_reg),
};

static struct sensor_aec_i2c_tag ov02a10_aec_info = {
    .slave_addr = (MAJOR_I2C_SLAVE_ADDR >> 1),
    .addr_bits_type = SENSOR_I2C_REG_8BIT,
    .data_bits_type = SENSOR_I2C_VAL_8BIT,
    .shutter = &ov02a10_shutter_tab,
    .again = &ov02a10_again_tab,
    .dgain = &ov02a10_dgain_tab,
    .frame_length = &ov02a10_frame_length_tab,
};

static SENSOR_STATIC_INFO_T s_ov02a10_static_info[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .static_info = {.f_num = 200,
                     .focal_length = 354,
                     .max_fps = 42,
                     .max_adgain = 15.5,
                     .ois_supported = 0,
                     .pdaf_supported = 0,
                     .exp_valid_frame_num = 1,
                     .clamp_level = 64,
                     .adgain_valid_frame_num = 1,
                     .fov_info = {{5.120f, 3.840f}, 4.042f}}}
    /*If there are multiple modules,please add here*/
};

static SENSOR_MODE_FPS_INFO_T s_ov02a10_mode_fps_info[VENDOR_NUM] = {
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

static struct sensor_module_info s_ov02a10_module_info_tab[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .module_info = {.major_i2c_addr = MAJOR_I2C_SLAVE_ADDR >> 1,
                     .minor_i2c_addr = MINOR_I2C_SLAVE_ADDR >> 1,

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
                             .is_loose = 0,
                         },
                     .change_setting_skip_num = 1,
                     .horizontal_view_angle = 65,
                     .vertical_view_angle = 60}}

    /*If there are multiple modules,please add here*/
};

static struct sensor_ic_ops s_ov02a10_ops_tab;
struct sensor_raw_info *s_ov02a10_mipi_raw_info_ptr =
    PNULL; //&s_ov02a10_mipi_raw_info;

/*==============================================================================
 * Description:
 * sensor all info
 * please modify this variable acording your spec
 *============================================================================*/
SENSOR_INFO_T g_ov02a10_mipi_raw_info = {
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
    .reset_pulse_level = SENSOR_HIGH_PULSE_RESET,
    .reset_pulse_width = 50,
    .power_down_level = SENSOR_LOW_LEVEL_PWDN,
    .identify_count = 1,
    .identify_code = {{.reg_addr = ov02a10_PID_ADDR,
                       .reg_value = ov02a10_PID_VALUE},
                      {.reg_addr = ov02a10_VER_ADDR,
                       .reg_value = ov02a10_VER_VALUE}},

    .source_width_max = SNAPSHOT_WIDTH,
    .source_height_max = SNAPSHOT_HEIGHT,
    .name = (cmr_s8 *)SENSOR_NAME,
    .image_format = SENSOR_IMAGE_FORMAT_RAW,

    .module_info_tab = s_ov02a10_module_info_tab,
    .module_info_tab_size = ARRAY_SIZE(s_ov02a10_module_info_tab),

    .resolution_tab_info_ptr = s_ov02a10_resolution_tab_raw,
    .sns_ops = &s_ov02a10_ops_tab,
    .raw_info_ptr = &s_ov02a10_mipi_raw_info_ptr,

    .video_tab_info_ptr = NULL,
    .sensor_version_info = (cmr_s8 *)"ov02a10_v1",
};

#endif
