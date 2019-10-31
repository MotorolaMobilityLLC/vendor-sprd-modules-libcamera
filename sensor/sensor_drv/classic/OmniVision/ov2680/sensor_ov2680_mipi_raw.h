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
 * V2.0
 */
#ifndef _SENSOR_OV2680_MIPI_RAW_H_
#define _SENSOR_OV2680_MIPI_RAW_H_

#include <utils/Log.h>
#include "sensor.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"

#define SENSOR_NAME "ov2680_mipi_raw"
#define I2C_SLAVE_ADDR 0x6c /* 16bit slave address*/

#define VENDOR_NUM 1
#define BINNING_FACTOR 1
#define ov2680_PID_ADDR 0x300A
#define ov2680_PID_VALUE 0x26
#define ov2680_VER_ADDR 0x300B
#define ov2680_VER_VALUE 0x80

/* sensor parameters begin */
/* effective sensor output image size */
#define SNAPSHOT_WIDTH 1600
#define SNAPSHOT_HEIGHT 1200
#define PREVIEW_WIDTH 1600
#define PREVIEW_HEIGHT 1200

/* frame length*/
#define SNAPSHOT_FRAME_LENGTH 1294
#define PREVIEW_FRAME_LENGTH 1294

/*Mipi output*/
#define LANE_NUM 1
#define RAW_BITS 10

#define SNAPSHOT_MIPI_PER_LANE_BPS 628
#define PREVIEW_MIPI_PER_LANE_BPS 628

/* please ref your spec */
#define FRAME_OFFSET 4
#define SENSOR_MAX_GAIN 0x3ff
#define SENSOR_BASE_GAIN 0x10
#define SENSOR_MIN_SHUTTER 4

/* isp parameters, please don't change it*/
#define ISP_BASE_GAIN 0x80

/* please don't change it */
#define EX_MCLK 24
//#define CAMERA_IMAGE_180
/*==============================================================================
 * Description:
 * global variable
 *============================================================================*/
// static struct hdr_info_t s_hdr_info;
// static uint32_t s_current_default_frame_length;
// struct sensor_ev_info_t s_sensor_ev_info;

// 800x600
LOCAL const SENSOR_REG_T ov2680_com_mipi_raw[] = {
    {0x0103, 0x01},
    {0xffff, 0xa0},
    {0x3002, 0x00},
    {0x3016, 0x1c},
    {0x3018, 0x44},
    {0x3020, 0x00},
    {0x3080, 0x02},
    {0x3082, 0x37},
    {0x3084, 0x09},
    {0x3085, 0x04},
    {0x3086, 0x01},
    {0x3501, 0x26},
    {0x3502, 0x40},
    {0x3503, 0x03},
    {0x350b, 0x36},
    {0x3600, 0xb4},
    {0x3603, 0x39},
    {0x3604, 0x24},
    {0x3605, 0x00}, // bit3:   1: use external
                    // regulator  0: use
                    // internal regulator
    {0x3620, 0x26},
    {0x3621, 0x37},
    {0x3622, 0x04},
    {0x3628, 0x00},
    {0x3705, 0x3c},
    {0x370c, 0x50},
    {0x370d, 0xc0},
    {0x3718, 0x88},
    {0x3720, 0x00},
    {0x3721, 0x00},
    {0x3722, 0x00},
    {0x3723, 0x00},
    {0x3738, 0x00},
    {0x370a, 0x23},
    {0x3717, 0x58},
    {0x3781, 0x80},
    {0x3784, 0x0c},
    {0x3789, 0x60},
    {0x3800, 0x00},
    {0x3801, 0x00},
    {0x3802, 0x00},
    {0x3803, 0x00},
    {0x3804, 0x06},
    {0x3805, 0x4f},
    {0x3806, 0x04},
    {0x3807, 0xbf},
    {0x3808, 0x03},
    {0x3809, 0x20},
    {0x380a, 0x02},
    {0x380b, 0x58},
    {0x380c, 0x06},
    {0x380d, 0xac},
    {0x380e, 0x02},
    {0x380f, 0x84},
    {0x3810, 0x00},
    {0x3811, 0x04},
    {0x3812, 0x00},
    {0x3813, 0x04},
    {0x3814, 0x31},
    {0x3815, 0x31},
    {0x3819, 0x04},
    {0x3820, 0xc6},
    {0x3821, 0x04},
    {0x4000, 0x81},
    {0x4001, 0x40},
    {0x4008, 0x00},
    {0x4009, 0x03},
    {0x4602, 0x02},
    {0x481b, 0x43},
    {0x481f, 0x36},
    {0x4825, 0x36},
    {0x4837, 0x30},
    {0x5000, 0x00},
    {0x5002, 0x30},
    {0x5080, 0x00},
    {0x5081, 0x41},
    {0x0100, 0x00}

};

LOCAL const SENSOR_REG_T ov2680_640X480_mipi_raw[] = {
    {0x3086, 0x01}, {0x3501, 0x26}, {0x3502, 0x40},
    {0x3620, 0x26}, {0x3621, 0x37}, {0x3622, 0x04},
    {0x370a, 0x23}, {0x370d, 0xc0}, {0x3718, 0x88},
    {0x3721, 0x00}, {0x3722, 0x00}, {0x3723, 0x00},
    {0x3738, 0x00}, {0x3800, 0x00}, {0x3801, 0xa0},
    {0x3802, 0x00}, {0x3803, 0x78}, {0x3804, 0x05},
    {0x3805, 0xaf}, {0x3806, 0x04}, {0x3807, 0x47},
    {0x3808, 0x02}, {0x3809, 0x80}, {0x380a, 0x01},
    {0x380b, 0xe0}, {0x380c, 0x06}, {0x380d, 0xac},
    {0x380e, 0x02}, {0x380f, 0x84}, {0x3810, 0x00},
    {0x3811, 0x04}, {0x3812, 0x00}, {0x3813, 0x04},
    {0x3814, 0x31}, {0x3815, 0x31},
#ifdef CONFIG_CAMERA_IMAGE_180
    {0x3820, 0xc2}, {0x3821, 0x00},
#else
    {0x3820, 0xc6}, {0x3821, 0x04},
#endif
    {0x4008, 0x00}, {0x4009, 0x03}, {0x4837, 0x30},
    {0x0100, 0x00}

};

LOCAL const SENSOR_REG_T ov2680_800X600_mipi_raw[] = {
    {0x0103, 0x01},
    {0x3002, 0x00},
    {0x3016, 0x1c},
    {0x3018, 0x44},
    {0x3020, 0x00},

    {0x3080, 0x02},
    {0x3082, 0x37},
    {0x3084, 0x09},
    {0x3085, 0x04},
    {0x3086, 0x01},

    {0x3501, 0x26},
    {0x3502, 0x40},
    {0x3503, 0x03},
    {0x350b, 0x36},
    {0x3600, 0xb4},
    {0x3603, 0x39},
    {0x3604, 0x24},
    {0x3605, 0x00}, // bit3:   1: use external regulator  0: use
                    // internal regulator
    {0x3620, 0x26},
    {0x3621, 0x37},
    {0x3622, 0x04},
    {0x3628, 0x00},
    {0x3705, 0x3c},
    {0x370c, 0x50},
    {0x370d, 0xc0},
    {0x3718, 0x88},
    {0x3720, 0x00},
    {0x3721, 0x00},
    {0x3722, 0x00},
    {0x3723, 0x00},
    {0x3738, 0x00},
    {0x370a, 0x23},
    {0x3717, 0x58},
    {0x3781, 0x80},
    {0x3784, 0x0c},
    {0x3789, 0x60},
    {0x3800, 0x00},
    {0x3801, 0x00},
    {0x3802, 0x00},
    {0x3803, 0x00},
    {0x3804, 0x06},
    {0x3805, 0x4f},
    {0x3806, 0x04},
    {0x3807, 0xbf},
    {0x3808, 0x03},
    {0x3809, 0x20},
    {0x380a, 0x02},
    {0x380b, 0x58},
    {0x380c, 0x06}, // hts 1708 6ac  1710 6ae
    {0x380d, 0xae},
    {0x380e, 0x02}, // vts 644
    {0x380f, 0x84},
    {0x3810, 0x00},
    {0x3811, 0x04},
    {0x3812, 0x00},
    {0x3813, 0x04},
    {0x3814, 0x31},
    {0x3815, 0x31},
    {0x3819, 0x04},
#ifdef CONFIG_CAMERA_IMAGE_180
    {0x3820, 0xc2}, // bit[2] flip
    {0x3821, 0x00}, // bit[2] mirror
#else
    {0x3820, 0xc6}, // bit[2] flip
    {0x3821, 0x05}, // bit[2] mirror
#endif
    {0x4000, 0x81},
    {0x4001, 0x40},
    {0x4008, 0x00},
    {0x4009, 0x03},
    {0x4602, 0x02},
    {0x481f, 0x36},
    {0x4825, 0x36},
    {0x4837, 0x30},
    {0x5000, 0x00},
    {0x5002, 0x30},
    {0x5080, 0x00},
    {0x5081, 0x41},
    {0x0100, 0x00}

};

static const SENSOR_REG_T ov2680_1600X1200_mipi_raw[] = {
    {0x0103, 0x01}, {0x3002, 0x00}, {0x3016, 0x1c}, {0x3018, 0x44},
    {0x3020, 0x00}, {0x3080, 0x02}, {0x3082, 0x37}, {0x3084, 0x09},
    {0x3085, 0x04}, {0x3086, 0x00}, {0x3501, 0x4e}, {0x3502, 0xe0},
    {0x3503, 0x03}, {0x350b, 0x36}, {0x3600, 0xb4}, {0x3603, 0x35},
    {0x3604, 0x24}, {0x3605, 0x00}, {0x3620, 0x24}, {0x3621, 0x34},
    {0x3622, 0x03}, {0x3628, 0x00}, {0x3701, 0x64}, {0x3705, 0x3c},
    {0x370c, 0x50}, {0x370d, 0xc0}, {0x3718, 0x80}, {0x3720, 0x00},
    {0x3721, 0x09}, {0x3722, 0x06}, {0x3723, 0x59}, {0x3738, 0x99},
    {0x370a, 0x21}, {0x3717, 0x58}, {0x3781, 0x80}, {0x3784, 0x0c},
    {0x3789, 0x60}, {0x3800, 0x00}, {0x3801, 0x00}, {0x3802, 0x00},
    {0x3803, 0x00}, {0x3804, 0x06}, {0x3805, 0x4f}, {0x3806, 0x04},
    {0x3807, 0xbf}, {0x3808, 0x06}, {0x3809, 0x40}, {0x380a, 0x04},
    {0x380b, 0xb0},                 //
    {0x380c, 0x06}, {0x380d, 0xa4}, // hts 1700
    {0x380e, 0x05}, {0x380f, 0x0e}, // vts 1294
    {0x3810, 0x00}, {0x3811, 0x08}, {0x3812, 0x00}, {0x3813, 0x08},
    {0x3814, 0x11}, {0x3815, 0x11}, {0x3819, 0x04}, //
    {0x3820, 0xc0}, {0x3821, 0x00},                 // mirror flip
    {0x4000, 0x81}, {0x4001, 0x40}, {0x4008, 0x02}, {0x4009, 0x09},
    {0x4602, 0x02}, {0x481f, 0x36}, {0x4825, 0x36}, {0x4837, 0x18},
    {0x5002, 0x30}, {0x5080, 0x00}, {0x5081, 0x41}, {0x5780, 0x3e},
    {0x5781, 0x0f}, {0x5782, 0x04}, {0x5783, 0x02}, {0x5784, 0x01},
    {0x5785, 0x01}, {0x5786, 0x00}, {0x5787, 0x04}, {0x5788, 0x02},
    {0x5789, 0x00}, {0x578a, 0x01}, {0x578b, 0x02}, {0x578c, 0x03},
    {0x578d, 0x03}, {0x578e, 0x08}, {0x578f, 0x0c}, {0x5790, 0x08},
    {0x5791, 0x04}, {0x5792, 0x00}, {0x5793, 0x00}, {0x5794, 0x03},
};

LOCAL const SENSOR_REG_T ov2680_1600X1200_mipi_raw_15fps[] = {
    /* 2680 Full 1600x1200 RAW 15fps 1lane */
    {0x0103, 0x01}, //
    {0xffff, 0x0a}, //
    {0x3002, 0x00}, //
    {0x3016, 0x1c}, //
    {0x3018, 0x44}, //
    {0x3020, 0x00}, //
    {0x3080, 0x02}, //
    {0x3082, 0x37}, //
    {0x3084, 0x09}, //
    {0x3085, 0x04}, //
    {0x3086, 0x01}, //
    {0x3501, 0x4e}, //
    {0x3502, 0xe0}, //
    {0x3503, 0x03}, //
    {0x350b, 0x36}, //
    {0x3600, 0xb4}, //
    {0x3603, 0x35}, //
    {0x3604, 0x24}, //
    {0x3605, 0x00}, //
    {0x3620, 0x24}, //
    {0x3621, 0x34}, //
    {0x3622, 0x03}, //
    {0x3628, 0x00}, //
    {0x3701, 0x64}, //
    {0x3705, 0x3c}, //
    {0x370c, 0x50}, //
    {0x370d, 0x00}, //
    {0x3718, 0x80}, //
    {0x3720, 0x00}, //
    {0x3721, 0x09}, //
    {0x3722, 0x0b}, //
    {0x3723, 0x48}, //
    {0x3738, 0x99}, //
    {0x370a, 0x21}, //
    {0x3717, 0x58}, //
    {0x3781, 0x80}, //
    {0x3784, 0x0c}, //
    {0x3789, 0x60}, //
    {0x3800, 0x00}, //
    {0x3801, 0x00}, //
    {0x3802, 0x00}, //
    {0x3803, 0x00}, //
    {0x3804, 0x06}, //
    {0x3805, 0x4f}, //
    {0x3806, 0x04}, //
    {0x3807, 0xbf}, //
    {0x3808, 0x06}, //
    {0x3809, 0x40}, //
    {0x380a, 0x04}, //
    {0x380b, 0xb0}, //
    {0x380c, 0x06}, //
    {0x380d, 0xe8}, // ;e8 ;e4 ;a4
    {0x380e, 0x04}, // 1244
    {0x380f, 0xdc}, // 1244
    {0x3810, 0x00}, //
    {0x3811, 0x08}, //
    {0x3812, 0x00}, //
    {0x3813, 0x08}, //
    {0x3814, 0x11}, //
    {0x3815, 0x11}, //
    {0x3819, 0x04}, //
#ifndef CAMERA_IMAGE_180
    {0x3820, 0xc0},
    {0x3821, 0x00},
#else
    {0x3820, 0xc4},
    {0x3821, 0x04},
#endif
    {0x4000, 0x81}, //
    {0x4001, 0x40}, //
    {0x4008, 0x02}, //
    {0x4009, 0x09}, //
    {0x4602, 0x02}, //
    {0x481f, 0x36}, //
    {0x4825, 0x36}, //
    {0x4837, 0x30}, //
                    //	{0x5000, 0x00},//
    {0x5002, 0x30}, //
    {0x5080, 0x00}, //
    {0x5081, 0x41}, //
    //{0x0100, 0x01},//
    {0x5780, 0x3e}, //
    {0x5781, 0x0f}, //
    {0x5782, 0x04}, //
    {0x5783, 0x02}, //
    {0x5784, 0x01}, //
    {0x5785, 0x01}, //
    {0x5786, 0x00}, //
    {0x5787, 0x04}, //
    {0x5788, 0x02}, //
    {0x5789, 0x00}, //
    {0x578a, 0x01}, //
    {0x578b, 0x02}, //
    {0x578c, 0x03}, //
    {0x578d, 0x03}, //
    {0x578e, 0x08}, //
    {0x578f, 0x0c}, //
    {0x5790, 0x08}, //
    {0x5791, 0x04}, //
    {0x5792, 0x00}, //
    {0x5793, 0x00}, //
    {0x5794, 0x03}, //

#if 0
    /*dual cam sync begin*/
    {0x3002, 0x00},
    {0x3823, 0x30},
    {0x3824, 0x00}, // cs
    {0x3825, 0x20},

    {0x3826, 0x00},
    {0x3827, 0x06},
/*dual cam sync end*/
#endif

};

static struct sensor_res_tab_info s_ov2680_resolution_tab_raw[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .reg_tab =
         {
             {ADDR_AND_LEN_OF_ARRAY(ov2680_com_mipi_raw), PNULL, 0, .width = 0,
              .height = 0, .xclk_to_sensor = EX_MCLK,
              .image_format = SENSOR_IMAGE_FORMAT_RAW},

             {ADDR_AND_LEN_OF_ARRAY(ov2680_1600X1200_mipi_raw), PNULL, 0,
              .width = 1600, .height = 1200, .xclk_to_sensor = 24,
              .image_format = SENSOR_IMAGE_FORMAT_RAW},
         }}
    /*If there are multiple modules,please add here*/
};

static SENSOR_TRIM_T s_ov2680_resolution_trim_tab[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .trim_info =
         {
             {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},

             {.trim_start_x = 0,
              .trim_start_y = 0,
              .trim_width = 1600,
              .trim_height = 1200,
              .line_time = 25760,
              .bps_per_lane = 628,
              .frame_line = 1294,
              .scaler_trim = {.x = 0, .y = 0, .w = 1600, .h = 1200}},

         }}

    /*If there are multiple modules,please add here*/
};

static struct sensor_reg_tag ov2680_shutter_reg[] = {
    {0x3502, 0}, {0x3501, 0}, {0x3500, 0},
};

static struct sensor_i2c_reg_tab ov2680_shutter_tab = {
    .settings = ov2680_shutter_reg, .size = ARRAY_SIZE(ov2680_shutter_reg),
};

static struct sensor_reg_tag ov2680_again_reg[] = {
    {0x3208, 0x01}, {0x350b, 0x00}, {0x350a, 0x00},
};

static struct sensor_i2c_reg_tab ov2680_again_tab = {
    .settings = ov2680_again_reg, .size = ARRAY_SIZE(ov2680_again_reg),
};

static struct sensor_reg_tag ov2680_dgain_reg[] = {
    {0x5004, 0}, {0x5005, 0}, {0x5006, 0},    {0x5007, 0},
    {0x5008, 0}, {0x5009, 0}, {0x3208, 0x11}, {0x3208, 0xA1},
};

struct sensor_i2c_reg_tab ov2680_dgain_tab = {
    .settings = ov2680_dgain_reg, .size = ARRAY_SIZE(ov2680_dgain_reg),
};

static struct sensor_reg_tag ov2680_frame_length_reg[] = {
    {0x380e, 0}, {0x380f, 0},
};

static struct sensor_i2c_reg_tab ov2680_frame_length_tab = {
    .settings = ov2680_frame_length_reg,
    .size = ARRAY_SIZE(ov2680_frame_length_reg),
};

static struct sensor_aec_i2c_tag ov2680_aec_info = {
    .slave_addr = (I2C_SLAVE_ADDR >> 1),
    .addr_bits_type = SENSOR_I2C_REG_16BIT,
    .data_bits_type = SENSOR_I2C_VAL_8BIT,
    .shutter = &ov2680_shutter_tab,
    .again = &ov2680_again_tab,
    .dgain = &ov2680_dgain_tab,
    .frame_length = &ov2680_frame_length_tab,
};

static SENSOR_MODE_FPS_INFO_T s_ov2680_mode_fps_info[VENDOR_NUM] = {
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

static SENSOR_STATIC_INFO_T s_ov2680_static_info[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .static_info = {.f_num = 220,
                     .focal_length = 227,
                     .max_fps = 0,
                     .max_adgain = 8 * 4,
                     .ois_supported = 0,
                     .pdaf_supported = 0,
                     .exp_valid_frame_num = 1,
                     .clamp_level = 4,
                     .adgain_valid_frame_num = 1,
                     .fov_info = {{4.614f, 3.444f}, 4.222f}}}
    /*If there are multiple modules,please add here*/
};

static struct sensor_module_info s_ov2680_module_info_tab[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .module_info = {.major_i2c_addr = I2C_SLAVE_ADDR >> 1,
                     .minor_i2c_addr = 0x6c >> 1,

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
                             .pixel_width = 10,
                             #ifdef _SENSOR_RAW_SHARKL5PRO_H_,
                                 .is_loose = 2,
                             #else
                                 .is_loose = 0,
                             #endif
                         },
                     .change_setting_skip_num = 1,
                     .horizontal_view_angle = 35,
                     .vertical_view_angle = 35}}

    /*If there are multiple modules,please add here*/
};

static struct sensor_ic_ops s_ov2680_ops_tab;

struct sensor_raw_info *s_ov2680_mipi_raw_info_ptr = PNULL;

SENSOR_INFO_T g_ov2680_mipi_raw_info = {
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
        {
            {.reg_addr = ov2680_PID_ADDR, .reg_value = ov2680_PID_VALUE},
            {.reg_addr = ov2680_VER_ADDR, .reg_value = ov2680_VER_VALUE},
        },

    .source_width_max = SNAPSHOT_WIDTH,
    .source_height_max = SNAPSHOT_HEIGHT,
    .name = (cmr_s8 *)SENSOR_NAME,
    .image_format = SENSOR_IMAGE_FORMAT_RAW,

    .module_info_tab = s_ov2680_module_info_tab,
    .module_info_tab_size = ARRAY_SIZE(s_ov2680_module_info_tab),

    .resolution_tab_info_ptr = s_ov2680_resolution_tab_raw,
    .sns_ops = &s_ov2680_ops_tab,
    .raw_info_ptr = &s_ov2680_mipi_raw_info_ptr,

    .video_tab_info_ptr = NULL,
    .sensor_version_info = (cmr_s8 *)"ov2680v1",
};
#endif
