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

#ifndef _SENSOR_ov64b40_MIPI_RAW_H_
#define _SENSOR_ov64b40_MIPI_RAW_H_

#include <utils/Log.h>
#include "sensor.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"

//#include "parameters/parameters_sharkl5/sensor_ov64b40_raw_param_main.c"

//#define FEATURE_OTP

#define VENDOR_NUM 1
#define SENSOR_NAME "ov64b40_mipi_raw"

#define I2C_SLAVE_ADDR 0x20 /* 8bit slave address*/

#define ov64b40_PID_ADDR 0x300B
#define ov64b40_PID_VALUE 0x64
#define ov64b40_VER_ADDR 0x300C
#define ov64b40_VER_VALUE 0x42

/* sensor parameters begin */

/* effective sensor output image size */
#define VIDEO_WIDTH 1920
#define VIDEO_HEIGHT 1080
#define PREVIEW_WIDTH 4624
#define PREVIEW_HEIGHT 3472
#define SNAPSHOT_WIDTH 9248
#define SNAPSHOT_HEIGHT 6944

/*Raw Trim parameters*/
#define VIDEO_TRIM_X 0
#define VIDEO_TRIM_Y 0
#define VIDEO_TRIM_W 1920
#define VIDEO_TRIM_H 1080
#define PREVIEW_TRIM_X 0
#define PREVIEW_TRIM_Y 0
#define PREVIEW_TRIM_W 4624
#define PREVIEW_TRIM_H 3472
#define SNAPSHOT_TRIM_X 0
#define SNAPSHOT_TRIM_Y 0
#define SNAPSHOT_TRIM_W 9248
#define SNAPSHOT_TRIM_H 6944

/*Mipi output*/
#define LANE_NUM 4
#define RAW_BITS 10


#define VIDEO_MIPI_PER_LANE_BPS 2496    /* 2*Mipi clk */
#define PREVIEW_MIPI_PER_LANE_BPS 2496  /* 2*Mipi clk */
#define SNAPSHOT_MIPI_PER_LANE_BPS 2496 /* 2*Mipi clk */

/*line time unit: 1ns*/
#define VIDEO_LINE_TIME 2604
#define PREVIEW_LINE_TIME 8126
#define SNAPSHOT_LINE_TIME 11251 

/* frame length*/
#define VIDEO_FRAME_LENGTH 3200 //0x470
#define PREVIEW_FRAME_LENGTH 4102
#define SNAPSHOT_FRAME_LENGTH 8888

/* please ref your spec */
#define FRAME_OFFSET 31
#define SENSOR_MAX_GAIN 0x0F00
#define SENSOR_BASE_GAIN 0x0100
#define SENSOR_MIN_SHUTTER 16


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

static const SENSOR_REG_T ov64b40_init_setting[] = {};

static const SENSOR_REG_T ov64b40_preview_setting[] = {
 

#if 0
//{0x0100,0x01},
#ifdef IMAGE_NORMAL_MIRROR
    {0x3820, 0x00}, // 44
    {0x3674, 0x00}, // 04
    {0x3675, 0x16}, //
    {0x3727, 0x23}, //
    {0x3891, 0x0f}, //
    {0x4500, 0x25}, //
    {0x450f, 0x88}, //
    {0x5194, 0x00}, //
    {0x5195, 0x00}, //
    {0x5404, 0x00}, //
    {0x5405, 0x00}, //
#endif
#ifdef IMAGE_H_MIRROR
    {0x3820, 0x00}, // 44
    {0x3674, 0x04}, // 04
    {0x3675, 0x16}, //
    {0x3727, 0x23}, //
    {0x3891, 0x0f}, //
    {0x4500, 0x25}, //
    {0x450f, 0x88}, //
    {0x5194, 0x00}, //
    {0x5195, 0x00}, //
    {0x5404, 0x00}, //
    {0x5405, 0x00}, //
#endif
#ifdef IMAGE_V_MIRROR
    {0x3820, 0x44}, // 44
    {0x3674, 0x00}, // 04
    {0x3675, 0x03}, //
    {0x3727, 0x27}, //
    {0x3891, 0x4f}, //
    {0x4500, 0x25}, //
    {0x450f, 0x88}, //
    {0x5194, 0x06}, //
    {0x5195, 0xe7}, //
    {0x5404, 0x06}, //
    {0x5405, 0xe7}, //
#endif
#ifdef IMAGE_HV_MIRROR
    {0x3820, 0x44}, // 44
    {0x3674, 0x04}, // 04
    {0x3675, 0x03}, //
    {0x3727, 0x27}, //
    {0x3891, 0x4f}, //
    {0x4500, 0x25}, //
    {0x450f, 0x88}, //
    {0x5194, 0x06}, //
    {0x5195, 0xe7}, //
    {0x5404, 0x06}, //
    {0x5405, 0xe7}, //
#endif
#endif

};

static const SENSOR_REG_T ov64b40_snapshot_setting[] = {};

static const SENSOR_REG_T ov64b40_video_setting[] = {};

static struct sensor_res_tab_info s_ov64b40_resolution_tab_raw[VENDOR_NUM] = {
    {.module_id = MODULE_OPTICSZOOM_WIDE_BACK,
     .reg_tab =
         {{ADDR_AND_LEN_OF_ARRAY(ov64b40_init_setting), PNULL, 0, .width = 0,
           .height = 0, .xclk_to_sensor = EX_MCLK,
           .image_format = SENSOR_IMAGE_FORMAT_RAW},

          {ADDR_AND_LEN_OF_ARRAY(ov64b40_video_setting), PNULL, 0,
           .width = VIDEO_WIDTH, .height = VIDEO_HEIGHT,
           .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW},

          {ADDR_AND_LEN_OF_ARRAY(ov64b40_preview_setting), PNULL, 0,
           .width = PREVIEW_WIDTH, .height = PREVIEW_HEIGHT,
           .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW},

          {ADDR_AND_LEN_OF_ARRAY(ov64b40_snapshot_setting), PNULL, 0,
           .width = SNAPSHOT_WIDTH, .height = SNAPSHOT_HEIGHT,
           .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW}}}

    /*If there are multiple modules,please add here*/
};

static SENSOR_TRIM_T s_ov64b40_resolution_trim_tab[VENDOR_NUM] = {
    {.module_id = MODULE_OPTICSZOOM_WIDE_BACK,
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

static SENSOR_REG_T ov64b40_shutter_reg[] = {
    {0x3501, 0x3e}, {0x3502, 0x60},
};

static struct sensor_i2c_reg_tab ov64b40_shutter_tab = {
    .settings = ov64b40_shutter_reg, .size = ARRAY_SIZE(ov64b40_shutter_reg),
};

static SENSOR_REG_T ov64b40_again_reg[] = {
    {0x3208, 0x00}, {0x3508, 0x04}, {0x3509, 0x00},
    {0x3208, 0x10}, {0x3208, 0xa0},
};

static struct sensor_i2c_reg_tab ov64b40_again_tab = {
    .settings = ov64b40_again_reg, .size = ARRAY_SIZE(ov64b40_again_reg),
};

static SENSOR_REG_T ov64b40_dgain_reg[] = {

};

static struct sensor_i2c_reg_tab ov64b40_dgain_tab = {
    .settings = ov64b40_dgain_reg, .size = ARRAY_SIZE(ov64b40_dgain_reg),
};

static SENSOR_REG_T ov64b40_frame_length_reg[] = {
    {0x380e, 0x0e}, {0x380f, 0xe0},
};

static struct sensor_i2c_reg_tab ov64b40_frame_length_tab = {
    .settings = ov64b40_frame_length_reg,
    .size = ARRAY_SIZE(ov64b40_frame_length_reg),
};

static struct sensor_aec_i2c_tag ov64b40_aec_info = {
    .slave_addr = (I2C_SLAVE_ADDR >> 1),
    .addr_bits_type = SENSOR_I2C_REG_16BIT,
    .data_bits_type = SENSOR_I2C_VAL_8BIT,
    .shutter = &ov64b40_shutter_tab,
    .again = &ov64b40_again_tab,
    .dgain = &ov64b40_dgain_tab,
    .frame_length = &ov64b40_frame_length_tab,
};

static const cmr_u16 ov64b40_pd_is_right[] = {
    0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1,
    0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1,
};

static const cmr_u16 ov64b40_pd_row[] = {
    7,  7,  23, 23, 43, 43, 59, 59, 11, 11, 27, 27, 39, 39, 55, 55,
    11, 11, 27, 27, 39, 39, 55, 55, 7,  7,  23, 23, 43, 43, 59, 59};

static const cmr_u16 ov64b40_pd_col[] = {
    0,  4,  4,  8,  4,  8,  0,  4,  20, 16, 24, 20, 24, 20, 20, 16,
    36, 40, 32, 36, 32, 36, 36, 40, 56, 52, 52, 48, 52, 48, 56, 52};

static SENSOR_STATIC_INFO_T s_ov64b40_static_info[VENDOR_NUM] = {
    {.module_id = MODULE_OPTICSZOOM_WIDE_BACK,
     .static_info = {.f_num = 180,
                     .focal_length = 384,
                     .max_fps = 30,
                     .max_adgain = 15,
                     .ois_supported = 0,
#if 0 // def CONFIG_CAMERA_PDAF_TYPE
                     .pdaf_supported = CONFIG_CAMERA_PDAF_TYPE,
#else
                     .pdaf_supported = 0,
#endif
                     .exp_valid_frame_num = 1,
                     .clamp_level = 64,
                     .adgain_valid_frame_num = 1,
                     .fov_angle= 79.4,
                     .fov_info = {{5.223, 3.917}, 3.985f}}}
    /*If there are multiple modules,please add here*/
};

static SENSOR_MODE_FPS_INFO_T s_ov64b40_mode_fps_info[VENDOR_NUM] = {
    {.module_id = MODULE_OPTICSZOOM_WIDE_BACK,
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

static struct sensor_module_info s_ov64b40_module_info_tab[VENDOR_NUM] = {
    {.module_id = MODULE_OPTICSZOOM_WIDE_BACK,
     .module_info =
         {.major_i2c_addr = 0x44 >> 1,//0x20 >> 1,
          .minor_i2c_addr = 0x46 >> 1,//0x6c >> 1,

          .reg_addr_value_bits =
              SENSOR_I2C_REG_16BIT | SENSOR_I2C_VAL_8BIT | SENSOR_I2C_FREQ_400,

          .avdd_val = SENSOR_AVDD_2800MV,
          .iovdd_val = SENSOR_AVDD_1800MV,
          .dvdd_val = SENSOR_AVDD_1200MV,
#ifdef IMAGE_V_MIRROR
          .image_pattern = SENSOR_IMAGE_PATTERN_RAWRGB_R,
#else
          .image_pattern = SENSOR_IMAGE_PATTERN_RAWRGB_B,
#endif
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
                  #if 0 //def _SENSOR_RAW_SHARKL5PRO_H_,
                      .is_loose = 2,
                  #else
                      .is_loose = 0,
                  #endif
                  .is_cphy = 1,
 //                 .lane_switch_eb = 1,
 //                 .lane_seq =
 //                     0x210, // combo cphy swap: 0x210, only for ums512
              },
          .change_setting_skip_num = 1,
          .horizontal_view_angle = 65,
          .vertical_view_angle = 60}}

    /*If there are multiple modules,please add here*/
};

static struct sensor_ic_ops s_ov64b40_ops_tab;
struct sensor_raw_info *s_ov64b40_mipi_raw_info_ptr =
    PNULL; //&s_ov64b40_mipi_raw_info;

/*==============================================================================
 * Description:
 * sensor all info
 * please modify this variable acording your spec
 *============================================================================*/
SENSOR_INFO_T g_ov64b40_mipi_raw_info = {
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
    .power_down_level = SENSOR_LOW_PULSE_RESET,
    .identify_count = 1,
    .identify_code = {{.reg_addr = ov64b40_PID_ADDR,
                       .reg_value = ov64b40_PID_VALUE},
                      {.reg_addr = ov64b40_VER_ADDR,
                       .reg_value = ov64b40_VER_VALUE}},

    .source_width_max = SNAPSHOT_WIDTH,
    .source_height_max = SNAPSHOT_HEIGHT,
    .name = (cmr_s8 *)SENSOR_NAME,
    .image_format = SENSOR_IMAGE_FORMAT_RAW,

    .module_info_tab = s_ov64b40_module_info_tab,
    .module_info_tab_size = ARRAY_SIZE(s_ov64b40_module_info_tab),

    .resolution_tab_info_ptr = s_ov64b40_resolution_tab_raw,
    .sns_ops = &s_ov64b40_ops_tab,
    .raw_info_ptr = &s_ov64b40_mipi_raw_info_ptr,

    .video_tab_info_ptr = NULL,
    .sensor_version_info = (cmr_s8 *)"ov64b40_v1",
};

#endif
