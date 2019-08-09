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

#ifndef _SENSOR_S5K4H7_MIPI_RAW_H_
#define _SENSOR_S5K4H7_MIPI_RAW_H_

#define LOG_TAG "s5k4h7_mipi_raw_2lane"
#include <utils/Log.h>
#include "sensor.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"


#define FEATURE_OTP

#ifdef FEATURE_OTP
#include "sensor_s5k4h7_qtech_otp.h"
static struct otp_info_t *s_s5k4h7_otp_info_ptr = &s_s5k4h7_qtech_otp_info;
#endif

#define VENDOR_NUM 1
#define SENSOR_NAME "s5k4h7_2lane"
#define MAJOR_I2C_SLAVE_ADDR 0x5A /* 8bit slave address*/
#define MINOR_I2C_SLAVE_ADDR 0x20

#define S5K4H7_PID_ADDR 0x0000
#define S5K4H7_PID_VALUE 0x48
#define S5K4H7_VER_ADDR 0x0001
#define S5K4H7_VER_VALUE 0x7B

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
#define LANE_NUM 2
#define RAW_BITS 10

#define VIDEO_MIPI_PER_LANE_BPS 900     /* 2*Mipi clk */
#define PREVIEW_MIPI_PER_LANE_BPS 900   /* 2*Mipi clk */
#define SNAPSHOT_MIPI_PER_LANE_BPS 1000 /* 2*Mipi clk */

/*line time unit: 1ns*/
#define VIDEO_LINE_TIME 18891
#define PREVIEW_LINE_TIME 18891
#define SNAPSHOT_LINE_TIME 16982

/* frame length*/
#define VIDEO_FRAME_LENGTH 1762
#define PREVIEW_FRAME_LENGTH 1762
#define SNAPSHOT_FRAME_LENGTH 2528

/* please ref your spec */
#define FRAME_OFFSET 8
#define SENSOR_MAX_GAIN 0x0200
#define SENSOR_BASE_GAIN 0x0020
#define SENSOR_MIN_SHUTTER 6

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
static const cmr_u32 sns_binning_fact[] = {1, 2, 1};
static const SENSOR_REG_T s5k4h7_init_setting[] = {
    // Sensor Information////////////////////////////
    // Sensor: s5k4h7
    // Date: 2018-01-11
    // Customer: SPRD_validation
    // MCLK: 24MHz
    // MIPI: 2 Lane
    // Pixel order: Gr 1st
    // BLC offset: 64code
    // Firmware Ver.: v1.0
    ////////////////////////////////////////////////
    {0x0100, 0x00}, {0x0B05, 0x01}, {0x3074, 0x06}, {0x3075, 0x2F},
    {0x308A, 0x20}, {0x308B, 0x08}, {0x308C, 0x0B}, {0x3081, 0x07},
    {0x307B, 0x85}, {0x307A, 0x0A}, {0x3079, 0x0A}, {0x306E, 0x71},
    {0x306F, 0x28}, {0x301F, 0x20}, {0x306B, 0x9A}, {0x3091, 0x1F},
    {0x30C4, 0x06}, {0x3200, 0x09}, {0x306A, 0x79}, {0x30B0, 0xFF},
    {0x306D, 0x08}, {0x3080, 0x00}, {0x3929, 0x3F}, {0x3084, 0x16},
    {0x3070, 0x0F}, {0x3B45, 0x01}, {0x30C2, 0x05}, {0x3069, 0x87},
    {0x3924, 0x7F}, {0x3925, 0xFD}, {0x3C08, 0xFF}, {0x3C09, 0xFF},
    {0x3C31, 0xFF}, {0x3C32, 0xFF}, {0x0A02, 0x14},

#ifdef FEATURE_OTP // enable lsc otp
//{0x021c, 0x0003},//otp LSC_en
#else
//{0x021c, 0x0001},//otp LSC_disable
#endif
};

static const SENSOR_REG_T s5k4h7_preview_setting[] = {
    // Sensor Information////////////////////////////
    // Sensor: s5k4h7
    // Date: 2018-01-11
    // Customer: SPRD_validation
    // Image size: 1632x1224
    // MCLK/PCLK: 24MHz /285Mhz
    // MIPI speed(Mbps): 900Mbps x 2Lane
    // Frame Length: 1762
    // Line Length: 5384
    // line Time: 18891
    // Max Fps: 30.04fps
    // Pixel order: Green 1st (=GR)
    // X/Y-flip: X-flip??
    // BLC offset: 64code
    // Firmware Ver.: v1.0
    ////////////////////////////////////////////////
    {0x0100, 0x00}, {0x0136, 0x18}, {0x0137, 0x00}, {0x0305, 0x04},
    {0x0306, 0x00}, {0x0307, 0x5F}, {0x030D, 0x06}, {0x030E, 0x00},
    {0x030F, 0xE1}, {0x3C1F, 0x00}, {0x3C17, 0x00}, {0x3C1C, 0x04},
    {0x3C1D, 0x15}, {0x0301, 0x04}, {0x0820, 0x03}, {0x0821, 0x84},
    {0x0822, 0x00}, {0x0823, 0x00}, {0x0112, 0x0A}, {0x0113, 0x0A},
    {0x0114, 0x01}, {0x3906, 0x00}, {0x0344, 0x00}, {0x0345, 0x08},
    {0x0346, 0x00}, {0x0347, 0x08}, {0x0348, 0x0C}, {0x0349, 0xC7},
    {0x034A, 0x09}, {0x034B, 0x97}, {0x034C, 0x06}, {0x034D, 0x60},
    {0x034E, 0x04}, {0x034F, 0xC8}, {0x0900, 0x01}, {0x0901, 0x22},
    {0x0381, 0x01}, {0x0383, 0x01}, {0x0385, 0x01}, {0x0387, 0x03},
#ifdef FLIP
    {0x0101, 0x03},
#else
    {0x0101, 0x00},
#endif
    {0x0340, 0x06}, {0x0341, 0xE2}, {0x0342, 0x15}, {0x0343, 0x08},
    {0x0200, 0x14}, {0x0201, 0x78}, {0x0202, 0x00}, {0x0203, 0x02},
    {0x3400, 0x01},
};

static const SENSOR_REG_T s5k4h7_snapshot_setting[] = {
    // Sensor Information////////////////////////////
    // Sensor: s5k4h7
    // Date: 2018-01-11
    // Customer: SPRD_validation
    // Image size: 3264x2448
    // MCLK/PCLK: 24MHz /285Mhz
    // MIPI speed(Mbps): 1000Mbps x 2Lane
    // Frame Length: 2528
    // Line Length: 5384
    // line Time: 16982
    // Max Fps: 23.29fps
    // Pixel order: Green 1st (=GR)
    // X/Y-flip: X-flip??
    // BLC offset: 64code
    // Firmware Ver.: v1.0
    ////////////////////////////////////////////////
    {0x0100, 0x00}, {0x0136, 0x18}, {0x0137, 0x00}, {0x0305, 0x04},
    {0x0306, 0x00}, {0x0307, 0x5F}, {0x030D, 0x06}, {0x030E, 0x00},
    {0x030F, 0xFA}, {0x3C1F, 0x00}, {0x3C17, 0x00}, {0x3C1C, 0x04},
    {0x3C1D, 0x15}, {0x0301, 0x04}, {0x0820, 0x03}, {0x0821, 0xE8},
    {0x0822, 0x00}, {0x0823, 0x00}, {0x0112, 0x0A}, {0x0113, 0x0A},
    {0x0114, 0x01}, {0x3906, 0x00}, {0x0344, 0x00}, {0x0345, 0x08},
    {0x0346, 0x00}, {0x0347, 0x08}, {0x0348, 0x0C}, {0x0349, 0xC7},
    {0x034A, 0x09}, {0x034B, 0x97}, {0x034C, 0x0C}, {0x034D, 0xC0},
    {0x034E, 0x09}, {0x034F, 0x90}, {0x0900, 0x00}, {0x0901, 0x00},
    {0x0381, 0x01}, {0x0383, 0x01}, {0x0385, 0x01}, {0x0387, 0x01},
#ifdef FLIP
    {0x0101, 0x03},
#else
    {0x0101, 0x00},
#endif
    {0x0340, 0x09}, {0x0341, 0xE0}, {0x0342, 0x12}, {0x0343, 0xE8},
    {0x0200, 0x12}, {0x0201, 0x58}, {0x0202, 0x00}, {0x0203, 0x02},
    {0x3400, 0x01},
};

static const SENSOR_REG_T s5k4h7_video_setting[] = {
    // Sensor Information////////////////////////////
    // Sensor: s5k4h7
    // Date: 2017-04-06
    // Customer: SPRD_validation
    // Image size: 1280x720
    // MCLK/PCLK: 24MHz /288Mhz
    // MIPI speed(Mbps): 360Mbps x 4Lane
    // Frame Length: 842
    // Line Length: 3800
    // line Time: 13194
    // Max Fps: 90.00fps
    // Pixel order: Green 1st (=GB)
    // X/Y-flip: X-flip
    // BLC offset: 64code
    // Firmware Ver.: v1.0
    ////////////////////////////////////////////////

};

static struct sensor_res_tab_info s_s5k4h7_resolution_tab_raw[VENDOR_NUM] = {
    {.module_id = MODULE_SPW_NONE_BACK,
     .reg_tab = {{ADDR_AND_LEN_OF_ARRAY(s5k4h7_init_setting), PNULL, 0,
                  .width = 0, .height = 0, .xclk_to_sensor = EX_MCLK,
                  .image_format = SENSOR_IMAGE_FORMAT_RAW},

                 /* {ADDR_AND_LEN_OF_ARRAY(s5k4h7_video_setting), PNULL, 0,
                     .width = VIDEO_WIDTH, .height = VIDEO_HEIGHT,
                     .xclk_to_sensor = EX_MCLK, .image_format =
                    SENSOR_IMAGE_FORMAT_RAW}, */

                 {ADDR_AND_LEN_OF_ARRAY(s5k4h7_preview_setting), PNULL, 0,
                  .width = PREVIEW_WIDTH, .height = PREVIEW_HEIGHT,
                  .xclk_to_sensor = EX_MCLK,
                  .image_format = SENSOR_IMAGE_FORMAT_RAW},

                 {ADDR_AND_LEN_OF_ARRAY(s5k4h7_snapshot_setting), PNULL, 0,
                  .width = SNAPSHOT_WIDTH, .height = SNAPSHOT_HEIGHT,
                  .xclk_to_sensor = EX_MCLK,
                  .image_format = SENSOR_IMAGE_FORMAT_RAW}}}

    /*If there are multiple modules,please add here*/
};

static SENSOR_TRIM_T s_s5k4h7_resolution_trim_tab[VENDOR_NUM] = {
    {.module_id = MODULE_SPW_NONE_BACK,
     .trim_info =
         {
             {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},

             /* {.trim_start_x = VIDEO_TRIM_X,
                 .trim_start_y = VIDEO_TRIM_Y,
                 .trim_width = VIDEO_TRIM_W,
                 .trim_height = VIDEO_TRIM_H,
                 .line_time = VIDEO_LINE_TIME,
                 .bps_per_lane = VIDEO_MIPI_PER_LANE_BPS,
                 .frame_line = VIDEO_FRAME_LENGTH,
                 .scaler_trim = {.x = VIDEO_TRIM_X,
                                 .y = VIDEO_TRIM_Y,
                                 .w = VIDEO_TRIM_W,
                                 .h = VIDEO_TRIM_H}}, */

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

static SENSOR_REG_T s5k4h7_shutter_reg[] = {
    {0x0202, 0x00}, {0x0203, 0x10},
};

static struct sensor_i2c_reg_tab s5k4h7_shutter_tab = {
    .settings = s5k4h7_shutter_reg, .size = ARRAY_SIZE(s5k4h7_shutter_reg),
};

static SENSOR_REG_T s5k4h7_again_reg[] = {
    {0x0204, 0x00}, {0x0205, 0x20},
};

static struct sensor_i2c_reg_tab s5k4h7_again_tab = {
    .settings = s5k4h7_again_reg, .size = ARRAY_SIZE(s5k4h7_again_reg),
};

static SENSOR_REG_T s5k4h7_dgain_reg[] = {

};

static struct sensor_i2c_reg_tab s5k4h7_dgain_tab = {
    .settings = s5k4h7_dgain_reg, .size = ARRAY_SIZE(s5k4h7_dgain_reg),
};

static SENSOR_REG_T s5k4h7_frame_length_reg[] = {
    {0x0340, 0x06}, {0x0341, 0xe2},
};

static struct sensor_i2c_reg_tab s5k4h7_frame_length_tab = {
    .settings = s5k4h7_frame_length_reg,
    .size = ARRAY_SIZE(s5k4h7_frame_length_reg),
};

static struct sensor_aec_i2c_tag s5k4h7_aec_info = {
    .slave_addr = (MAJOR_I2C_SLAVE_ADDR >> 1),
    .addr_bits_type = SENSOR_I2C_REG_16BIT,
    .data_bits_type = SENSOR_I2C_VAL_8BIT,
    .shutter = &s5k4h7_shutter_tab,
    .again = &s5k4h7_again_tab,
    .dgain = &s5k4h7_dgain_tab,
    .frame_length = &s5k4h7_frame_length_tab,
};

static const cmr_u16 s5k4h7_pd_is_right[] = {
    0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1,
    0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1,
};

static const cmr_u16 s5k4h7_pd_row[] = {
    7,  7,  23, 23, 43, 43, 59, 59, 11, 11, 27, 27, 39, 39, 55, 55,
    11, 11, 27, 27, 39, 39, 55, 55, 7,  7,  23, 23, 43, 43, 59, 59};

static const cmr_u16 s5k4h7_pd_col[] = {
    0,  4,  4,  8,  4,  8,  0,  4,  20, 16, 24, 20, 24, 20, 20, 16,
    36, 40, 32, 36, 32, 36, 36, 40, 56, 52, 52, 48, 52, 48, 56, 52};

static SENSOR_STATIC_INFO_T s_s5k4h7_static_info[VENDOR_NUM] = {
    {.module_id = MODULE_SPW_NONE_BACK,
     .static_info = {.f_num = 200,
                     .focal_length = 354,
                     .max_fps = 30,
                     .max_adgain = 8,
                     .ois_supported = 0,
#ifdef CONFIG_CAMERA_PDAF_TYPE
                     .pdaf_supported = CONFIG_CAMERA_PDAF_TYPE,
#else
                     .pdaf_supported = 0,
#endif
                     .exp_valid_frame_num = 1,
                     .clamp_level = 64,
                     .adgain_valid_frame_num = 1,
                     .fov_info = {{4.614f, 3.444f}, 4.222f}}}
    /*If there are multiple modules,please add here*/
};

static SENSOR_MODE_FPS_INFO_T s_s5k4h7_mode_fps_info[VENDOR_NUM] = {
    {.module_id = MODULE_SPW_NONE_BACK,
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

static struct sensor_module_info s_s5k4h7_module_info_tab[VENDOR_NUM] = {
    {.module_id = MODULE_SPW_NONE_BACK,
     .module_info = {.major_i2c_addr = MAJOR_I2C_SLAVE_ADDR >> 1,
                     .minor_i2c_addr = MINOR_I2C_SLAVE_ADDR >> 1,

                     .reg_addr_value_bits = SENSOR_I2C_REG_16BIT |
                                            SENSOR_I2C_VAL_8BIT |
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

static struct sensor_ic_ops s_s5k4h7_ops_tab;
struct sensor_raw_info *s_s5k4h7_mipi_raw_info_ptr = PNULL;

/*==============================================================================
 * Description:
 * sensor all info
 * please modify this variable acording your spec
 *============================================================================*/
SENSOR_INFO_T g_s5k4h7_mipi_raw_info = {
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
    .identify_code = {{.reg_addr = S5K4H7_PID_ADDR,
                       .reg_value = S5K4H7_PID_VALUE},
                      {.reg_addr = S5K4H7_VER_ADDR,
                       .reg_value = S5K4H7_VER_VALUE}},

    .source_width_max = SNAPSHOT_WIDTH,
    .source_height_max = SNAPSHOT_HEIGHT,
    .name = (cmr_s8 *)SENSOR_NAME,
    .image_format = SENSOR_IMAGE_FORMAT_RAW,

    .module_info_tab = s_s5k4h7_module_info_tab,
    .module_info_tab_size = ARRAY_SIZE(s_s5k4h7_module_info_tab),

    .resolution_tab_info_ptr = s_s5k4h7_resolution_tab_raw,
    .sns_ops = &s_s5k4h7_ops_tab,
    .raw_info_ptr = &s_s5k4h7_mipi_raw_info_ptr,

    .video_tab_info_ptr = NULL,
    .sensor_version_info = (cmr_s8 *)"s5k4h7_v1",
};

#endif
