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
#ifndef _SENSOR_OV7251_MIPI_RAW_H_
#define _SENSOR_OV7251_MIPI_RAW_H_

#include <utils/Log.h>
#include "sensor.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"

#define SENSOR_NAME "ov7251_mipi_raw"
#define I2C_SLAVE_ADDR 0x66

#define VENDOR_NUM 1
#define ov7251_CHIP_ID_H_ADDR 0x300A
#define ov7251_CHIP_ID_H_VALUE 0x77
#define ov7251_CHIP_ID_L_ADDR 0x300B
#define ov7251_CHIP_ID_L_VALUE 0x50

/* sensor parameters begin */
/* effective sensor output image size */
#define SNAPSHOT_WIDTH 640
#define SNAPSHOT_HEIGHT 480

/* frame length*/
#define PREVIEW_FRAME_LENGTH 0xa1a

/* Mipi output */
#define LANE_NUM 1
#define RAW_BITS 10

/* please ref your spec */
#define FRAME_OFFSET 4
#define SENSOR_BASE_GAIN 0x10
#define ISP_BASE_GAIN			0x80
#define SENSOR_MIN_SHUTTER		8

/* please don't change it */
#define EX_MCLK 24

static struct sensor_module_info s_ov7251_module_info_tab[VENDOR_NUM] = {
    {
        .module_id = MODULE_STL3D_IRL_FRONT,
        .module_info = {
            .major_i2c_addr = 0xe0 >> 1,//0xe0 >> 1, //0xc0---main2
            .minor_i2c_addr = 0xc0 >> 1,//I2C_SLAVE_ADDR >> 1,

            .reg_addr_value_bits = SENSOR_I2C_REG_16BIT |
                                   SENSOR_I2C_VAL_8BIT |
                                   SENSOR_I2C_FREQ_100,

            .avdd_val = SENSOR_AVDD_2800MV,
            .iovdd_val = SENSOR_AVDD_1800MV,
            .dvdd_val = SENSOR_AVDD_1200MV,

            .image_pattern = SENSOR_IMAGE_PATTERN_RAWRGB_B,

            .preview_skip_num = 1,// TODO:
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
            .change_setting_skip_num = 1,// TODO:
            .horizontal_view_angle = 35,
            .vertical_view_angle = 35,
        },
    },
/*If there are multiple modules,please add here*/
};

/* 640x480 RAW10 100fps */
static const SENSOR_REG_T ov7251_init_setting[] = {
    //{0x0103, 0x01}, {0xffff, 0xa0},
    {0x0100, 0x00},// software reset, stream off
    {0x3005, 0x00},// SC_REG5
    {0x3012, 0xc0}, {0x3013, 0xd2},// SC_MIPI_PHY
    {0x3014, 0x04},// SC_MIPI_SC_CTRL0
    {0x3016, 0x10}, {0x3017, 0x00}, {0x3018, 0x00}, {0x301a, 0x00}, {0x301b, 0x00}, {0x301c, 0x00},//SC_CLKRST
    {0x3023, 0x05},// SC_LOW_PWR_CTRL
    {0x3037, 0xf0},// SC_R37
    {0x3098, 0x04},// [4:0]pll2_pre_divider
    {0x3099, 0x28},// pll2_multiplier
    {0x309a, 0x05},// [3:0]pll2_sys_divider
    {0x309b, 0x04},// [3:0]pll2_adc_divider
    {0x30b0, 0x0a},// [3:0]pll1_pix_divider
    {0x30b1, 0x01},// [4:0]pll1_divider
    {0x30b3, 0x64},// pll1_multiplier
    {0x30b4, 0x03},// [2:0]pll1_pre_divider
    {0x30b5, 0x05},// [2:0]pll1_mipi_divider
    {0x3106, 0xda},// SB_SRB_CTRL
    {0x3500, 0x00}, {0x3501, 0x2d}, {0x3502, 0x80},// aec expo, 0x01f8
    {0x3503, 0x07},// agc, aec manual
    {0x3509, 0x10},// linear gain
    {0x350b, 0x10},// AEC AGC ADJ
    {0x3600, 0x1c}, {0x3602, 0x62}, {0x3620, 0xb7},// ANALOG REGISTERS
    {0x3622, 0x04}, {0x3626, 0x21}, {0x3627, 0x30},// ANALOG REGISTERS
    {0x3630, 0x44}, {0x3631, 0x35}, {0x3634, 0x60},// ANALOG REGISTERS
    {0x3636, 0x00},// ANA_CTRL_36
    {0x3662, 0x01},// ANA_CORE_2
    {0x3663, 0x70}, {0x3664, 0xf0},// ANALOG REGISTERS
    {0x3666, 0x0a},// ANA_CORE_6
    {0x3669, 0x1a}, {0x366a, 0x00}, {0x366b, 0x50},// ANALOG REGISTERS
    {0x3673, 0x01}, {0x3674, 0xef}, {0x3675, 0x03},// ANALOG REGISTERS
    {0x3705, 0xc1}, {0x3709, 0x40}, {0x373c, 0x08},// SENSOR CONTROL REGISTERS
    {0x3742, 0x00}, {0x3757, 0xb3}, {0x3788, 0x00},// SENSOR CONTROL REGISTERS
    {0x37a8, 0x01}, {0x37a9, 0xc0},// FIFO_CTRL0
    {0x3800, 0x00}, {0x3801, 0x04},// x addr start
    {0x3802, 0x00}, {0x3803, 0x04},// y addr start
    {0x3804, 0x02}, {0x3805, 0x8b},// x addr end
    {0x3806, 0x01}, {0x3807, 0xeb},// y addr end
    {0x3808, 0x02}, {0x3809, 0x80},// x output size
    {0x380a, 0x01}, {0x380b, 0xe0},// y output size
    {0x380c, 0x03}, {0x380d, 0xa0},// hts
    {0x380e, 0x02}, {0x380f, 0x0a},// vts
    {0x3810, 0x00}, {0x3811, 0x04},// isp x win
    {0x3812, 0x00}, {0x3813, 0x05},// isp y win
    {0x3814, 0x11},// x inc
    {0x3815, 0x11},// y inc
    {0x3820, 0x40}, {0x3821, 0x00},// format
    {0x382f, 0x0e},// TIMING_REG2F
    {0x3832, 0x00}, {0x3833, 0x05}, {0x3834, 0x00}, {0x3835, 0x0c},// TIMING_REG
    {0x3837, 0x00},// DIGITAL BINNING CTRL
    {0x3b80, 0x00}, {0x3b81, 0xa5}, {0x3b82, 0x10},// LED_PWM_REG
    {0x3b83, 0x00}, {0x3b84, 0x08}, {0x3b85, 0x00},// LED_PWM_REG
    {0x3b86, 0x01}, {0x3b87, 0x00}, {0x3b88, 0x00},// LED_PWM_REG
    {0x3b89, 0x00}, {0x3b8a, 0x00}, {0x3b8b, 0x05},// LED_PWM_REG
    {0x3b8c, 0x00}, {0x3b8d, 0x00}, {0x3b8e, 0x00},// LED_PWM_REG
    {0x3b8f, 0x1a}, {0x3b94, 0x05}, {0x3b95, 0xf2},// LED_PWM_REG
    {0x3b96, 0x40},// LED_PWM_REG
    {0x3c00, 0x89}, {0x3c01, 0x63}, {0x3c02, 0x01}, {0x3c03, 0x00},// LOWPWR
    {0x3c04, 0x00}, {0x3c05, 0x03}, {0x3c06, 0x00}, {0x3c07, 0x06},// LOWPWR
    {0x3c0c, 0x01}, {0x3c0d, 0xd0}, {0x3c0e, 0x02}, {0x3c0f, 0x0a},// LOWPWR
    {0x4001, 0x42},// blc ctrl
    {0x4004, 0x04},// blc num
    {0x4005, 0x00},// BLC MAN CTRL
    {0x404e, 0x01},// BLC AVG
    {0x4300, 0xff},// data max
    {0x4301, 0x00},// data min
    {0x4501, 0x48},// SC_REG1501
    {0x4600, 0x00}, {0x4601, 0x4e},// read start
    {0x4801, 0x0f}, {0x4806, 0x0f},// MIPI CTRL
    {0x4819, 0xaa},// HS_ZERO_MIN
    {0x4823, 0x3e},// CLK_TRAIL_MIN
    {0x4837, 0x19},// PCLK_PERIOD
    {0x4a0d, 0x00},
    {0x4a47, 0x7f}, {0x4a49, 0xf0}, {0x4a4b, 0x30},// LOWPWR CTRL REGISTERS
    {0x5000, 0x85}, {0x5001, 0x80},// isp ctrl
};

static const SENSOR_REG_T ov7251_640x480_raw10_30fps_setting[] = {
    {0x3800, 0x00}, {0x3801, 0x04},// x addr start
    {0x3802, 0x00}, {0x3803, 0x04},// y addr start
    {0x3804, 0x02}, {0x3805, 0x8b},// x addr end
    {0x3806, 0x01}, {0x3807, 0xeb},// y addr end
    {0x3808, 0x02}, {0x3809, 0x80},// x output size
    {0x380a, 0x01}, {0x380b, 0xe0},// y output size
    {0x380c, 0x03}, {0x380d, 0xa0},// hts
    {0x380e, 0x06}, {0x380f, 0xbc},// vts
    {0x3810, 0x00}, {0x3811, 0x05},// isp x win
    {0x3812, 0x00}, {0x3813, 0x05},// isp y win
    {0x3814, 0x11},// x inc
    {0x3815, 0x11},// y inc
    {0x3820, 0x40}, {0x3821, 0x00},// format
    {0x382f, 0x0e},// TIMING_REG2F
    {0x3832, 0x00}, {0x3833, 0x05}, {0x3834, 0x00}, {0x3835, 0x0c},// TIMING_REG
    {0x3837, 0x00},// DIGITAL BINNING CTRL
    //{0x5e00, 0x8c},// color bar
};

static const SENSOR_REG_T ov7251_640x480_raw10_20fps_slave_setting[] = {
#if 1

/* 640x480 RAW10 100fps */
//static const SENSOR_REG_T ov7251_init_setting[] = {
    //{0x0103, 0x01}, {0xffff, 0xa0},
    {0x0100, 0x00},// software reset, stream off
    {0x3005, 0x00},// SC_REG5
    {0x3012, 0xc0}, {0x3013, 0xd2},// SC_MIPI_PHY
    {0x3014, 0x04},// SC_MIPI_SC_CTRL0
    {0x3016, 0x10}, {0x3017, 0x00}, {0x3018, 0x00}, {0x301a, 0x00}, {0x301b, 0x00}, {0x301c, 0x00},//SC_CLKRST
    {0x3023, 0x05},// SC_LOW_PWR_CTRL
    {0x3037, 0xf0},// SC_R37
    {0x3098, 0x04},// [4:0]pll2_pre_divider
    {0x3099, 0x28},// pll2_multiplier
    {0x309a, 0x05},// [3:0]pll2_sys_divider
    {0x309b, 0x04},// [3:0]pll2_adc_divider
    {0x30b0, 0x0a},// [3:0]pll1_pix_divider
    {0x30b1, 0x01},// [4:0]pll1_divider
    {0x30b3, 0x64},// pll1_multiplier
    {0x30b4, 0x03},// [2:0]pll1_pre_divider
    {0x30b5, 0x05},// [2:0]pll1_mipi_divider
    {0x3106, 0xda},// SB_SRB_CTRL
    {0x3500, 0x00}, {0x3501, 0x2d}, {0x3502, 0x80},// aec expo, 0x01f8
    {0x3503, 0x07},// agc, aec manual
    {0x3509, 0x10},// linear gain
    {0x350b, 0x10},// AEC AGC ADJ
    {0x3600, 0x1c}, {0x3602, 0x62}, {0x3620, 0xb7},// ANALOG REGISTERS
    {0x3622, 0x04}, {0x3626, 0x21}, {0x3627, 0x30},// ANALOG REGISTERS
    {0x3630, 0x44}, {0x3631, 0x35}, {0x3634, 0x60},// ANALOG REGISTERS
    {0x3636, 0x00},// ANA_CTRL_36
    {0x3662, 0x01},// ANA_CORE_2
    {0x3663, 0x70}, {0x3664, 0xf0},// ANALOG REGISTERS
    {0x3666, 0x0a},// ANA_CORE_6
    {0x3669, 0x1a}, {0x366a, 0x00}, {0x366b, 0x50},// ANALOG REGISTERS
    {0x3673, 0x01}, {0x3674, 0xef}, {0x3675, 0x03},// ANALOG REGISTERS
    {0x3705, 0xc1}, {0x3709, 0x40}, {0x373c, 0x08},// SENSOR CONTROL REGISTERS
    {0x3742, 0x00}, {0x3757, 0xb3}, {0x3788, 0x00},// SENSOR CONTROL REGISTERS
    {0x37a8, 0x01}, {0x37a9, 0xc0},// FIFO_CTRL0
    {0x3800, 0x00}, {0x3801, 0x04},// x addr start
    {0x3802, 0x00}, {0x3803, 0x04},// y addr start
    {0x3804, 0x02}, {0x3805, 0x8b},// x addr end
    {0x3806, 0x01}, {0x3807, 0xeb},// y addr end
    {0x3808, 0x02}, {0x3809, 0x80},// x output size
    {0x380a, 0x01}, {0x380b, 0xe0},// y output size
    {0x380c, 0x03}, {0x380d, 0xa0},// hts
    {0x380e, 0x02}, {0x380f, 0x0a},// vts
    {0x3810, 0x00}, {0x3811, 0x04},// isp x win
    {0x3812, 0x00}, {0x3813, 0x05},// isp y win
    {0x3814, 0x11},// x inc
    {0x3815, 0x11},// y inc
    {0x3820, 0x40}, {0x3821, 0x00},// format
    {0x382f, 0x0e},// TIMING_REG2F
    {0x3832, 0x00}, {0x3833, 0x05}, {0x3834, 0x00}, {0x3835, 0x0c},// TIMING_REG
    {0x3837, 0x00},// DIGITAL BINNING CTRL
    {0x3b80, 0x00}, {0x3b81, 0xa5}, {0x3b82, 0x10},// LED_PWM_REG
    {0x3b83, 0x00}, {0x3b84, 0x08}, {0x3b85, 0x00},// LED_PWM_REG
    {0x3b86, 0x01}, {0x3b87, 0x00}, {0x3b88, 0x00},// LED_PWM_REG
    {0x3b89, 0x00}, {0x3b8a, 0x00}, {0x3b8b, 0x05},// LED_PWM_REG
    {0x3b8c, 0x00}, {0x3b8d, 0x00}, {0x3b8e, 0x00},// LED_PWM_REG
    {0x3b8f, 0x1a}, {0x3b94, 0x05}, {0x3b95, 0xf2},// LED_PWM_REG
    {0x3b96, 0x40},// LED_PWM_REG
    {0x3c00, 0x89}, {0x3c01, 0x63}, {0x3c02, 0x01}, {0x3c03, 0x00},// LOWPWR
    {0x3c04, 0x00}, {0x3c05, 0x03}, {0x3c06, 0x00}, {0x3c07, 0x06},// LOWPWR
    {0x3c0c, 0x01}, {0x3c0d, 0xd0}, {0x3c0e, 0x02}, {0x3c0f, 0x0a},// LOWPWR
    {0x4001, 0x42},// blc ctrl
    {0x4004, 0x04},// blc num
    {0x4005, 0x00},// BLC MAN CTRL
    {0x404e, 0x01},// BLC AVG
    {0x4300, 0xff},// data max
    {0x4301, 0x00},// data min
    {0x4501, 0x48},// SC_REG1501
    {0x4600, 0x00}, {0x4601, 0x4e},// read start
    {0x4801, 0x0f}, {0x4806, 0x0f},// MIPI CTRL
    {0x4819, 0xaa},// HS_ZERO_MIN
    {0x4823, 0x3e},// CLK_TRAIL_MIN
    {0x4837, 0x19},// PCLK_PERIOD
    {0x4a0d, 0x00},
    {0x4a47, 0x7f}, {0x4a49, 0xf0}, {0x4a4b, 0x30},// LOWPWR CTRL REGISTERS
    {0x5000, 0x85}, {0x5001, 0x80},// isp ctrl
//};

#endif
    {0x3005, 0x00},
    {0x3666, 0x00},
    {0x3823, 0x30},
    {0x3824, 0x01},
    {0x3825, 0x07},
    {0x3826, 0x00},
    {0x3827, 0x00},
    {0x3800, 0x00}, {0x3801, 0x04},// x addr start
    {0x3802, 0x00}, {0x3803, 0x04},// y addr start
    {0x3804, 0x02}, {0x3805, 0x8b},// x addr end
    {0x3806, 0x01}, {0x3807, 0xeb},// y addr end
    {0x3808, 0x02}, {0x3809, 0x80},// x output size
    {0x380a, 0x01}, {0x380b, 0xe0},// y output size
    {0x380c, 0x03}, {0x380d, 0xa0},// hts
    {0x380e, 0x0a}, {0x380f, 0x1a},// vts
    {0x3810, 0x00}, {0x3811, 0x05},// isp x win
    {0x3812, 0x00}, {0x3813, 0x05},// isp y win
    {0x3814, 0x11},// x inc
    {0x3815, 0x11},// y inc
    {0x3820, 0x40}, {0x3821, 0x00},// format
    {0x382f, 0x0e},// TIMING_REG2F
    {0x3832, 0x00}, {0x3833, 0x05}, {0x3834, 0x00}, {0x3835, 0x0c},// TIMING_REG
    {0x3837, 0x00},// DIGITAL BINNING CTRL
};

static SENSOR_STATIC_INFO_T s_ov7251_static_info[VENDOR_NUM] = {
    {
        .module_id = MODULE_STL3D_IRL_FRONT,
        .static_info = {
            .f_num = 200,
            .focal_length = 354,
            .max_fps = 0,
            .max_adgain = 15 * 2,
            .ois_supported = 0,
            .pdaf_supported = 0,
            .exp_valid_frame_num = 1,
            .clamp_level = 64,
            .adgain_valid_frame_num = 1,
            .fov_info = {{4.614f, 3.444f}, 4.222f},
        },
    },
    /*If there are multiple modules,please add here*/
};

static SENSOR_MODE_FPS_INFO_T s_ov7251_mode_fps_info[VENDOR_NUM] = {
    {
        .module_id = MODULE_STL3D_IRL_FRONT,
        .fps_info = {
            .is_init = 0,
            .sensor_mode_fps = {
                {SENSOR_MODE_COMMON_INIT, 0, 1, 0, 0},
                {SENSOR_MODE_PREVIEW_ONE, 0, 1, 0, 0},
                {SENSOR_MODE_SNAPSHOT_ONE_FIRST, 0, 1, 0, 0},
                {SENSOR_MODE_SNAPSHOT_ONE_SECOND, 0, 1, 0, 0},
                {SENSOR_MODE_SNAPSHOT_ONE_THIRD, 0, 1, 0, 0},
                {SENSOR_MODE_PREVIEW_TWO, 0, 1, 0, 0},
                {SENSOR_MODE_SNAPSHOT_TWO_FIRST, 0, 1, 0, 0},
                {SENSOR_MODE_SNAPSHOT_TWO_SECOND, 0, 1, 0, 0},
                {SENSOR_MODE_SNAPSHOT_TWO_THIRD, 0, 1, 0, 0},
            },
        },
    },
    /*If there are multiple modules,please add here*/
};

static struct sensor_res_tab_info s_ov7251_resolution_tab_raw[VENDOR_NUM] = {
    {
        .module_id = MODULE_STL3D_IRL_FRONT,
        .reg_tab = {
            {
                ADDR_AND_LEN_OF_ARRAY(ov7251_init_setting),
                PNULL,
                0,
                .width = 0,
                .height = 0,
                .xclk_to_sensor = EX_MCLK,
                .image_format = SENSOR_IMAGE_FORMAT_RAW,
            },
            {
                ADDR_AND_LEN_OF_ARRAY(ov7251_640x480_raw10_20fps_slave_setting),
                PNULL,
                0,
                .width = 640,
                .height = 480,
                .xclk_to_sensor = EX_MCLK,
                .image_format = SENSOR_IMAGE_FORMAT_RAW,
            },
        },
    },
/*If there are multiple modules,please add here*/
};

static SENSOR_TRIM_T s_ov7251_resolution_trim_tab[VENDOR_NUM] = {
    {
        .module_id = MODULE_STL3D_IRL_FRONT,
        .trim_info = {
            {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
            {
                .trim_start_x = 0,
                .trim_start_y = 0,
                .trim_width = 640,
                .trim_height = 480,
                .line_time = 19334,
                .bps_per_lane = 540,
                .frame_line = PREVIEW_FRAME_LENGTH,
                .scaler_trim = {
                    .x = 0,
                    .y = 0,
                    .w = 640,
                    .h = 480,
                },
            },
        },
    },
    /*If there are multiple modules,please add here*/
};

static struct sensor_ic_ops s_ov7251_ops_tab;
static struct sensor_raw_info *s_ov7251_mipi_raw_info_ptr = PNULL;

SENSOR_INFO_T g_ov7251_mipi_raw_info = {
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
    .reset_pulse_level = SENSOR_HIGH_PULSE_RESET,//SENSOR_LOW_PULSE_RESET,
    .reset_pulse_width = 50,
    .power_down_level = SENSOR_LOW_LEVEL_PWDN,
    .identify_count = 1,
    .identify_code = {
        {
            .reg_addr = ov7251_CHIP_ID_H_ADDR,
            .reg_value = ov7251_CHIP_ID_H_VALUE,
        },
        {
            .reg_addr = ov7251_CHIP_ID_L_ADDR,
            .reg_value = ov7251_CHIP_ID_L_VALUE,
        },
    },

    .source_width_max = SNAPSHOT_WIDTH,
    .source_height_max = SNAPSHOT_HEIGHT,
    .name = (cmr_s8 *)SENSOR_NAME,
    .image_format = SENSOR_IMAGE_FORMAT_RAW,

    .module_info_tab = s_ov7251_module_info_tab,
    .module_info_tab_size = ARRAY_SIZE(s_ov7251_module_info_tab),

    .resolution_tab_info_ptr = s_ov7251_resolution_tab_raw,
    .sns_ops = &s_ov7251_ops_tab,
    .raw_info_ptr = &s_ov7251_mipi_raw_info_ptr,

    .video_tab_info_ptr = NULL,
    .sensor_version_info = (cmr_s8 *)"ov7251v1",
};
#endif
