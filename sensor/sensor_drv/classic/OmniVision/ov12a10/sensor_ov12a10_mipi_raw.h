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

#ifndef _SENSOR_ov12a10_MIPI_RAW_H_
#define _SENSOR_ov12a10_MIPI_RAW_H_

#include <utils/Log.h>
#include "sensor.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"

#include "parameters/parameters_sharkl5/sensor_ov12a10_raw_param_main.c"

#define VENDOR_NUM 1
#define SENSOR_NAME "ov12a10_mipi_raw"

#define MAJOR_I2C_SLAVE_ADDR 0x20
#define MINOR_I2C_SLAVE_ADDR 0x6C

#define ov12a10_PID_ADDR 0x300B
#define ov12a10_PID_VALUE 0x12
#define ov12a10_VER_ADDR 0x300C
#define ov12a10_VER_VALUE 0x41

/* sensor parameters begin */

/* effective sensor output image size */
#define VIDEO_WIDTH 1280
#define VIDEO_HEIGHT 720
#define PREVIEW_WIDTH 2048
#define PREVIEW_HEIGHT 1536
#define SNAPSHOT_WIDTH 4096
#define SNAPSHOT_HEIGHT 3072

/*Raw Trim parameters*/
#define VIDEO_TRIM_X 0
#define VIDEO_TRIM_Y 0
#define VIDEO_TRIM_W 1280
#define VIDEO_TRIM_H 720
#define PREVIEW_TRIM_X 0
#define PREVIEW_TRIM_Y 0
#define PREVIEW_TRIM_W 2048
#define PREVIEW_TRIM_H 1536
#define SNAPSHOT_TRIM_X 0
#define SNAPSHOT_TRIM_Y 0
#define SNAPSHOT_TRIM_W 4096
#define SNAPSHOT_TRIM_H 3072

/*Mipi output*/
#define LANE_NUM 4
#define RAW_BITS 10

#define VIDEO_MIPI_PER_LANE_BPS 560
#define PREVIEW_MIPI_PER_LANE_BPS 1120
#define SNAPSHOT_MIPI_PER_LANE_BPS 1120

/*line time unit: 1ns*/
#define VIDEO_LINE_TIME 9852
#define PREVIEW_LINE_TIME 9852
#define SNAPSHOT_LINE_TIME 9852

/* frame length*/
#define VIDEO_FRAME_LENGTH 844
#define PREVIEW_FRAME_LENGTH 3346
#define SNAPSHOT_FRAME_LENGTH 3346

/* please ref your spec */
#define FRAME_OFFSET 8
#define SENSOR_MAX_GAIN 0x07FF
#define SENSOR_BASE_GAIN 0x0080
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
//#define IMAGE_V_MIRROR

//#define IMAGE_V_MIRROR //IMAGE_NORMAL_MIRROR; IMAGE_H_MIRROR; IMAGE_V_MIRROR;
// IMAGE_HV_MIRROR

static const SENSOR_REG_T ov12a10_init_setting[] = {
    {0x0103, 0x01}, {0x0100, 0x00}, {0x0300, 0x04}, {0x0301, 0x01},
    {0x0302, 0x25}, {0x0303, 0x00}, {0x0304, 0x00}, {0x0305, 0x01},
    {0x0307, 0x01}, {0x030e, 0x02}, {0x0313, 0x05}, {0x3002, 0x21},
    {0x3012, 0x40}, {0x3013, 0x72}, {0x3016, 0x72}, {0x301b, 0xd0},
    {0x301d, 0xf0}, {0x301f, 0xd0}, {0x3021, 0x23}, {0x3022, 0x01},
    {0x3106, 0x15}, {0x3107, 0x23}, {0x3500, 0x00}, {0x3501, 0xd0},
    {0x3502, 0x00}, {0x3505, 0x83}, {0x3508, 0x02}, {0x3509, 0x00},
    {0x3600, 0x43}, {0x3611, 0x8a}, {0x3613, 0x97}, {0x3620, 0x80},
    {0x3624, 0x2c}, {0x3625, 0xa0}, {0x3626, 0x00}, {0x3631, 0x00},
    {0x3632, 0x01}, {0x3641, 0x80}, {0x3642, 0x12}, {0x3644, 0x78},
    {0x3645, 0xa7}, {0x364e, 0x44}, {0x364f, 0x44}, {0x3650, 0x11},
    {0x3654, 0x00}, {0x3657, 0x31}, {0x3659, 0x0c}, {0x365f, 0x07},
    {0x3661, 0x17}, {0x3662, 0x17}, {0x3663, 0x17}, {0x3664, 0x17},
    {0x3666, 0x08}, {0x366b, 0x20}, {0x366c, 0xa4}, {0x366d, 0x20},
    {0x366e, 0xa4}, {0x3680, 0x00}, {0x3714, 0x24}, {0x371a, 0x3e},
    {0x3737, 0x04}, {0x3739, 0x12}, {0x3765, 0x20}, {0x3767, 0x00},
    {0x37a1, 0x3e}, {0x37a8, 0x4d}, {0x37ab, 0x2c}, {0x37c2, 0x04},
    {0x37d8, 0x03}, {0x37d9, 0x0c}, {0x37e0, 0x00}, {0x37e1, 0x0a},
    {0x37e2, 0x14}, {0x37e3, 0x04}, {0x37e4, 0x2a}, {0x37e5, 0x03},
    {0x37e6, 0x04}, {0x3800, 0x00}, {0x3801, 0x00}, {0x3802, 0x00},
    {0x3803, 0x00}, {0x3804, 0x10}, {0x3805, 0x0f}, {0x3806, 0x0c},
    {0x3807, 0x0f}, {0x3808, 0x10}, {0x3809, 0x00}, {0x380a, 0x0c},
    {0x380b, 0x00}, {0x380c, 0x04}, {0x380d, 0x28}, {0x380e, 0x0d},
    {0x380f, 0x12}, {0x3811, 0x0a}, {0x3813, 0x08}, {0x3814, 0x01},
    {0x3815, 0x01}, {0x3816, 0x01}, {0x3817, 0x01}, {0x3820, 0xa8},
    {0x3821, 0x00}, {0x3822, 0x91}, {0x3823, 0x18}, {0x3826, 0x00},
    {0x3827, 0x00}, {0x3829, 0x03}, {0x3832, 0x08}, {0x3833, 0x30},
    {0x3c80, 0x00}, {0x3c87, 0x01}, {0x3c8c, 0x1a}, {0x3c8d, 0x68},
    {0x3c97, 0x02}, {0x3cc0, 0x40}, {0x3cc1, 0x54}, {0x3cc2, 0x34},
    {0x3cc3, 0x04}, {0x3cc4, 0x00}, {0x3cc5, 0x00}, {0x3cc6, 0x00},
    {0x3cc7, 0x00}, {0x3cc8, 0x00}, {0x3cc9, 0x00}, {0x3d8c, 0x73},
    {0x3d8d, 0xc0}, {0x4001, 0x2b}, {0x4008, 0x02}, {0x4009, 0x0f},
    {0x4011, 0xff}, {0x4013, 0x08}, {0x4014, 0x08}, {0x4015, 0x08},
    {0x4017, 0x08}, {0x401a, 0x58}, {0x4050, 0x04}, {0x4051, 0x0b},
    {0x405e, 0x20}, {0x4501, 0x00}, {0x4503, 0x00}, {0x450a, 0x04},
    {0x4601, 0x30}, {0x4800, 0x00}, {0x481f, 0x30}, {0x4837, 0x0d},
    {0x483c, 0x0f}, {0x484b, 0x01}, {0x4d00, 0x05}, {0x4d01, 0x19},
    {0x4d02, 0xfd}, {0x4d03, 0xd1}, {0x4d04, 0xff}, {0x4d05, 0xff},
    {0x5000, 0x09}, {0x5001, 0x42}, {0x5002, 0x01}, {0x5005, 0x00},
    {0x5081, 0x04}, {0x5180, 0x00}, {0x5181, 0x10}, {0x5182, 0x02},
    {0x5183, 0x0f}, {0x5185, 0x6c}, {0x5200, 0x03}, {0x520b, 0x07},
    {0x520c, 0x0f}};

static const SENSOR_REG_T ov12a10_video_setting[] = {
    {0x0100, 0x00}, {0x3642, 0x10}, {0x3666, 0x00}, {0x366b, 0xa4},
    {0x366c, 0x20}, {0x366d, 0xa4}, {0x366e, 0x20}, {0x3714, 0x28},
    {0x371a, 0x3e}, {0x3737, 0x08}, {0x3739, 0x20}, {0x37c2, 0x14},
    {0x37d9, 0x0c}, {0x37e3, 0x08}, {0x37e4, 0x36}, {0x37e6, 0x08},
    {0x3800, 0x00}, {0x3801, 0x00}, {0x3802, 0x03}, {0x3803, 0x28},
    {0x3804, 0x0f}, {0x3805, 0xff}, {0x3806, 0x08}, {0x3807, 0xe7},
    {0x3808, 0x05}, {0x3809, 0x00}, {0x380a, 0x02}, {0x380b, 0xd0},
    {0x380e, 0x03}, {0x380f, 0x4c}, {0x3810, 0x01}, {0x3811, 0x86},
    {0x3813, 0x08}, {0x3814, 0x03}, {0x3816, 0x03}, {0x3820, 0xab},
    {0x4009, 0x0d}, {0x4050, 0x04}, {0x4051, 0x0b}, {0x4501, 0x00},
    {0x5002, 0x01}, {0x3501, 0x34}, {0x3502, 0x40}};

static const SENSOR_REG_T ov12a10_preview_setting[] = {
    {0x0100, 0x00}, {0x3642, 0x10}, {0x3666, 0x08}, {0x366b, 0x20},
    {0x366c, 0xa4}, {0x366d, 0x20}, {0x366e, 0xa4}, {0x3714, 0x28},
    {0x371a, 0x3e}, {0x3737, 0x08}, {0x3739, 0x20}, {0x37c2, 0x14},
    {0x37d9, 0x0c}, {0x37e3, 0x08}, {0x37e4, 0x36}, {0x37e6, 0x08},
    {0x3800, 0x00}, {0x3801, 0x00}, {0x3802, 0x00}, {0x3803, 0x00},
    {0x3804, 0x10}, {0x3805, 0x1f}, {0x3806, 0x0c}, {0x3807, 0x0f},
    {0x3808, 0x08}, {0x3809, 0x00}, {0x380a, 0x06}, {0x380b, 0x00},
    {0x380e, 0x0d}, {0x380f, 0x12}, {0x3810, 0x00}, {0x3811, 0x06},
    {0x3813, 0x04}, {0x3814, 0x03}, {0x3816, 0x03}, {0x3820, 0xab},
    {0x4009, 0x0d}, {0x4050, 0x04}, {0x4051, 0x0b}, {0x4501, 0x3c},//00},
    {0x5002, 0x01}, {0x3501, 0x69}, {0x3502, 0x20}};

static const SENSOR_REG_T ov12a10_snapshot_setting[] = {
    {0x0100, 0x00}, {0x3642, 0x12}, {0x3666, 0x08}, {0x366b, 0x20},
    {0x366c, 0xa4}, {0x366d, 0x20}, {0x366e, 0xa4}, {0x3714, 0x24},
    {0x371a, 0x3e}, {0x3737, 0x04}, {0x3739, 0x12}, {0x37c2, 0x04},
    {0x37d9, 0x0c}, {0x37e3, 0x04}, {0x37e4, 0x2a}, {0x37e6, 0x04},
    {0x3800, 0x00}, {0x3801, 0x00}, {0x3802, 0x00}, {0x3803, 0x00},
    {0x3804, 0x10}, {0x3805, 0x0f}, {0x3806, 0x0c}, {0x3807, 0x0f},
    {0x3808, 0x10}, {0x3809, 0x00}, {0x380a, 0x0c}, {0x380b, 0x00},
    {0x380e, 0x0d}, {0x380f, 0x12}, {0x3810, 0x00}, {0x3811, 0x0a},
    {0x3813, 0x08}, {0x3814, 0x01}, {0x3816, 0x01}, {0x3820, 0xa8},
    {0x4009, 0x0f}, {0x4050, 0x04}, {0x4051, 0x0b}, {0x4501, 0x00},
    {0x5002, 0x45}, {0x3501, 0xd0}, {0x3502, 0xa0}};

static struct sensor_res_tab_info s_ov12a10_resolution_tab_raw[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .reg_tab =
         {{ADDR_AND_LEN_OF_ARRAY(ov12a10_init_setting), PNULL, 0, .width = 0,
           .height = 0, .xclk_to_sensor = EX_MCLK,
           .image_format = SENSOR_IMAGE_FORMAT_RAW},

          {ADDR_AND_LEN_OF_ARRAY(ov12a10_video_setting), PNULL, 0,
           .width = VIDEO_WIDTH, .height = VIDEO_HEIGHT,
           .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW},

          {ADDR_AND_LEN_OF_ARRAY(ov12a10_preview_setting), PNULL, 0,
           .width = PREVIEW_WIDTH, .height = PREVIEW_HEIGHT,
           .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW},

          {ADDR_AND_LEN_OF_ARRAY(ov12a10_snapshot_setting), PNULL, 0,
           .width = SNAPSHOT_WIDTH, .height = SNAPSHOT_HEIGHT,
           .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW}}}

    /*If there are multiple modules,please add here*/
};

static SENSOR_TRIM_T s_ov12a10_resolution_trim_tab[VENDOR_NUM] = {
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

static SENSOR_REG_T ov12a10_shutter_reg[] = {
    {0x3500, 0x00}, {0x3501, 0x00}, {0x3502, 0x00},
};

static struct sensor_i2c_reg_tab ov12a10_shutter_tab = {
    .settings = ov12a10_shutter_reg, .size = ARRAY_SIZE(ov12a10_shutter_reg),
};

static SENSOR_REG_T ov12a10_again_reg[] = {
    {0x3208, 0x00}, {0x3508, 0x04}, {0x3509, 0x00},
    {0x3208, 0x10}, {0x3208, 0xa0},
};

static struct sensor_i2c_reg_tab ov12a10_again_tab = {
    .settings = ov12a10_again_reg, .size = ARRAY_SIZE(ov12a10_again_reg),
};

static SENSOR_REG_T ov12a10_dgain_reg[] = {

};

static struct sensor_i2c_reg_tab ov12a10_dgain_tab = {
    .settings = ov12a10_dgain_reg, .size = ARRAY_SIZE(ov12a10_dgain_reg),
};

static SENSOR_REG_T ov12a10_frame_length_reg[] = {
    {0x380e, 0x0e}, {0x380f, 0xe0},
};

static struct sensor_i2c_reg_tab ov12a10_frame_length_tab = {
    .settings = ov12a10_frame_length_reg,
    .size = ARRAY_SIZE(ov12a10_frame_length_reg),
};

static struct sensor_aec_i2c_tag ov12a10_aec_info = {
    .slave_addr = (MAJOR_I2C_SLAVE_ADDR >> 1),
    .addr_bits_type = SENSOR_I2C_REG_16BIT,
    .data_bits_type = SENSOR_I2C_VAL_8BIT,
    .shutter = &ov12a10_shutter_tab,
    .again = &ov12a10_again_tab,
    .dgain = &ov12a10_dgain_tab,
    .frame_length = &ov12a10_frame_length_tab,
};

static SENSOR_STATIC_INFO_T s_ov12a10_static_info[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .static_info = {.f_num = 200,
                     .focal_length = 354,
                     .max_fps = 30,
                     .max_adgain = 16,
                     .ois_supported = 0,
                     .pdaf_supported = 3,
                     .exp_valid_frame_num = 1,
                     .clamp_level = 64,
                     .adgain_valid_frame_num = 1,
                     .fov_info = {{5.120f, 3.840f}, 4.042f}}}
    /*If there are multiple modules,please add here*/
};

static SENSOR_MODE_FPS_INFO_T s_ov12a10_mode_fps_info[VENDOR_NUM] = {
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

static struct sensor_module_info s_ov12a10_module_info_tab[VENDOR_NUM] = {
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
                             .is_loose = 0,
                         },
                     .change_setting_skip_num = 1,
                     .horizontal_view_angle = 65,
                     .vertical_view_angle = 60}}

    /*If there are multiple modules,please add here*/
};

static struct sensor_ic_ops s_ov12a10_ops_tab;
struct sensor_raw_info *s_ov12a10_mipi_raw_info_ptr = &s_ov12a10_mipi_raw_info;

/*==============================================================================
 * Description:
 * sensor all info
 * please modify this variable acording your spec
 *============================================================================*/
SENSOR_INFO_T g_ov12a10_mipi_raw_info = {
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
    .identify_code = {{.reg_addr = ov12a10_PID_ADDR,
                       .reg_value = ov12a10_PID_VALUE},
                      {.reg_addr = ov12a10_VER_ADDR,
                       .reg_value = ov12a10_VER_VALUE}},

    .source_width_max = SNAPSHOT_WIDTH,
    .source_height_max = SNAPSHOT_HEIGHT,
    .name = (cmr_s8 *)SENSOR_NAME,
    .image_format = SENSOR_IMAGE_FORMAT_RAW,

    .module_info_tab = s_ov12a10_module_info_tab,
    .module_info_tab_size = ARRAY_SIZE(s_ov12a10_module_info_tab),

    .resolution_tab_info_ptr = s_ov12a10_resolution_tab_raw,
    .sns_ops = &s_ov12a10_ops_tab,
    .raw_info_ptr = &s_ov12a10_mipi_raw_info_ptr,

    .video_tab_info_ptr = NULL,
    .sensor_version_info = (cmr_s8 *)"ov12a10_v1",
};

#endif
