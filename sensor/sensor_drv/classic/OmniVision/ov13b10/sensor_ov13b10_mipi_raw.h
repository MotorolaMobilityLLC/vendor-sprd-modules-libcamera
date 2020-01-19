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

#ifndef _SENSOR_ov13b10_MIPI_RAW_H_
#define _SENSOR_ov13b10_MIPI_RAW_H_

#include <utils/Log.h>
#include "sensor.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"

//#include "parameters/parameters_sharkl5/sensor_ov13b10_raw_param_main.c"

#define VENDOR_NUM 1
#define SENSOR_NAME "ov13b10_mipi_raw"

#define MAJOR_I2C_SLAVE_ADDR 0x20
#define MINOR_I2C_SLAVE_ADDR 0x6C

#define ov13b10_PID_ADDR 0x300B
#define ov13b10_PID_VALUE 0x0d
#define ov13b10_VER_ADDR 0x300C
#define ov13b10_VER_VALUE 0x42

/* sensor parameters begin */

/* effective sensor output image size */
#define VIDEO_WIDTH 1280
#define VIDEO_HEIGHT 720
#define PREVIEW_WIDTH 2104
#define PREVIEW_HEIGHT 1560
#define SNAPSHOT_WIDTH 4208
#define SNAPSHOT_HEIGHT 3120

/*Raw Trim parameters*/
#define VIDEO_TRIM_X 0
#define VIDEO_TRIM_Y 0
#define VIDEO_TRIM_W 1280
#define VIDEO_TRIM_H 720
#define PREVIEW_TRIM_X 0
#define PREVIEW_TRIM_Y 0
#define PREVIEW_TRIM_W 2104
#define PREVIEW_TRIM_H 1560
#define SNAPSHOT_TRIM_X 0
#define SNAPSHOT_TRIM_Y 0
#define SNAPSHOT_TRIM_W 4208
#define SNAPSHOT_TRIM_H 3120

/*Mipi output*/
#define LANE_NUM 4
#define RAW_BITS 10

#define VIDEO_MIPI_PER_LANE_BPS 560
#define PREVIEW_MIPI_PER_LANE_BPS 560//1120
#define SNAPSHOT_MIPI_PER_LANE_BPS 1120

/*line time unit: 1ns*/
#define VIDEO_LINE_TIME 10500
#define PREVIEW_LINE_TIME 10500
#define SNAPSHOT_LINE_TIME 10500

/* frame length*/
#define VIDEO_FRAME_LENGTH 798
#define PREVIEW_FRAME_LENGTH 1598
#define SNAPSHOT_FRAME_LENGTH 3196

/* please ref your spec */
#define FRAME_OFFSET 8
#define SENSOR_MAX_GAIN 0x0F80
#define SENSOR_BASE_GAIN 0x0100
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

static const SENSOR_REG_T ov13b10_init_setting[] = {
//global setting		
//initial setting		
//register	value	description
{0x0103, 0x01},//	PLL
{0x0303, 0x01},//	
{0x0305, 0x46},//	
{0x0321, 0x00},//	
{0x0323, 0x04},//	
{0x0324, 0x01},//	
{0x0325, 0x50},//	
{0x0326, 0x81},//	
{0x0327, 0x04},//	
{0x3012, 0x07},//	system control
{0x3013, 0x32},//	
{0x3107, 0x23},//	SCCB control
{0x3501, 0x0c},//	AEC
{0x3502, 0x10},//	
{0x3504, 0x08},//	
{0x3508, 0x07},//	
{0x3509, 0xc0},//	
{0x3600, 0x16},//	Analog control
{0x3601, 0x54},//	
{0x3612, 0x4e},//	
{0x3620, 0x00},//	
{0x3621, 0x68},//	
{0x3622, 0x66},//	
{0x3623, 0x03},//	
{0x3662, 0x92},//	
{0x3666, 0xbb},//	
{0x3667, 0x44},//	
{0x366e, 0xff},//	
{0x366f, 0xf3},//	
{0x3675, 0x44},//	
{0x3676, 0x00},//	
{0x367f, 0xe9},//	
{0x3681, 0x32},//	
{0x3682, 0x1f},//	
{0x3683, 0x0b},//	
{0x3684, 0x0b},//	
{0x3704, 0x0f},//	Sensor top control
{0x3706, 0x40},//	
{0x3708, 0x3b},//	
{0x3709, 0x72},//	
{0x370b, 0xa2},//	
{0x3714, 0x24},//	
{0x371a, 0x3e},//	
{0x3725, 0x42},//	
{0x3739, 0x12},//	
{0x3767, 0x00},//	
{0x377a, 0x0d},//	
{0x3789, 0x18},//	
{0x3790, 0x40},//	
{0x3791, 0xa2},//	
{0x37c2, 0x04},//	
{0x37c3, 0xf1},//	
{0x37d9, 0x0c},//	
{0x37da, 0x02},//	
{0x37dc, 0x02},//	
{0x37e1, 0x04},//	
{0x37e2, 0x0a},//	
{0x3800, 0x00},//	X start
{0x3801, 0x00},//	
{0x3802, 0x00},//	Y start
{0x3803, 0x08},//	
{0x3804, 0x10},//	X end
{0x3805, 0x8f},//	
{0x3806, 0x0c},//	Y end
{0x3807, 0x47},//	
{0x3808, 0x10},//	X output size
{0x3809, 0x70},//	
{0x380a, 0x0c},//	Y output size
{0x380b, 0x30},//	
{0x380c, 0x04},//	HTS
{0x380d, 0x98},//	
{0x380e, 0x0c},//	VTS
{0x380f, 0x7c},//	
{0x3811, 0x0f},//	ISP X win
{0x3813, 0x08},//	ISP Y win
{0x3814, 0x01},//	X Inc odd
{0x3815, 0x01},//	X Inc even
{0x3816, 0x01},//	Y Inc odd
{0x3817, 0x01},//	Y Inc even
{0x381f, 0x08},//	VTS EXP diff
{0x3820, 0x88},//	fromat
{0x3821, 0x00},//	
{0x3822, 0x14},//	VTS auto dis
{0x382e, 0xe6},//	Expo offset
{0x3c80, 0x00},//	
{0x3c87, 0x01},//	
{0x3c8c, 0x19},//	
{0x3c8d, 0x1c},//	
{0x3ca0, 0x00},//	
{0x3ca1, 0x00},//	
{0x3ca2, 0x00},//	
{0x3ca3, 0x00},//	
{0x3ca4, 0x50},//	
{0x3ca5, 0x11},//	
{0x3ca6, 0x01},//	
{0x3ca7, 0x00},//	
{0x3ca8, 0x00},//	
{0x4008, 0x02},//	BLC control
{0x4009, 0x0f},//	
{0x400a, 0x01},//	
{0x400b, 0x19},//	
{0x4011, 0x21},//	
{0x4017, 0x08},//	
{0x4019, 0x04},//	
{0x401a, 0x58},//	
{0x4032, 0x1e},//	
{0x4050, 0x02},//	
{0x4051, 0x09},//	
{0x405e, 0x00},//	
{0x4066, 0x02},//	
{0x4501, 0x00},//	colum ADC sync and SYNC_FIFO
{0x4502, 0x10},//	
{0x4505, 0x00},//	
{0x4800, 0x64},//	MIPI
{0x481b, 0x3e},//	
{0x481f, 0x30},//	
{0x4825, 0x34},//	
{0x4837, 0x0e},//	
{0x484b, 0x01},//	
{0x4883, 0x02},//	
{0x5000, 0xff},//	DSP
{0x5001, 0x0f},//	
{0x5045, 0x20},//	
{0x5046, 0x20},//	
{0x5047, 0xa4},//	
{0x5048, 0x20},//	
{0x5049, 0xa4},//	

};

static const SENSOR_REG_T ov13b10_video_setting[] = {
//1280x720	
//119.35fps,line time 10.5us, 560Mbps/lane	
//register	value
{0x0305, 0x23},
{0x3501, 0x03},
{0x3502, 0x00},
{0x3662, 0x88},
{0x3714, 0x28},
{0x371a, 0x3e},
{0x3739, 0x10},
{0x37c2, 0x14},
{0x37d9, 0x06},
{0x3800, 0x03},
{0x3801, 0x30},
{0x3802, 0x03},
{0x3803, 0x48},
{0x3804, 0x0d},
{0x3805, 0x5f},
{0x3806, 0x09},
{0x3807, 0x07},
{0x3808, 0x05},
{0x3809, 0x00},
{0x380a, 0x02},
{0x380b, 0xd0},
{0x380c, 0x04},
{0x380d, 0x98},
{0x380e, 0x03},
{0x380f, 0x1e},
{0x3811, 0x0b},
{0x3813, 0x08},
{0x3814, 0x03},
{0x3815, 0x01},
{0x3816, 0x03},
{0x3817, 0x01},
{0x3820, 0x8b},
{0x3c38, 0x18},
{0x4008, 0x00},
{0x4009, 0x05},
{0x4050, 0x00},
{0x4051, 0x05},
{0x4501, 0x08},
{0x4505, 0x04},
{0x4837, 0x1d},
{0x5000, 0xfd},
{0x5001, 0x0d},
};

static const SENSOR_REG_T ov13b10_preview_setting[] = {
 //   {0x0100, 0x00}, 
//	2104x1560	
//59.6fps,line time 10.5us, 560Mbps/lane	
//register	value
{0x0305, 0x23},
{0x3501, 0x06},
{0x3502, 0x10},
{0x3662, 0x88},
{0x3714, 0x28},
{0x371a, 0x3e},
{0x3739, 0x10},
{0x37c2, 0x14},
{0x37d9, 0x06},
{0x3800, 0x00},
{0x3801, 0x00},
{0x3802, 0x00},
{0x3803, 0x08},
{0x3804, 0x10},
{0x3805, 0x8f},
{0x3806, 0x0c},
{0x3807, 0x47},
{0x3808, 0x08},
{0x3809, 0x38},
{0x380a, 0x06},
{0x380b, 0x18},
{0x380c, 0x04},
{0x380d, 0x98},
{0x380e, 0x06},
{0x380f, 0x3e},
{0x3811, 0x07},
{0x3813, 0x04},
{0x3814, 0x03},
{0x3815, 0x01},
{0x3816, 0x03},
{0x3817, 0x01},
{0x3820, 0x8b},
{0x3c38, 0x18},
{0x4008, 0x00},
{0x4009, 0x05},
{0x4050, 0x00},
{0x4051, 0x05},
{0x4501, 0x08},
{0x4505, 0x04},
{0x4837, 0x1d},
{0x5000, 0xfd},
{0x5001, 0x0d},

	};

static const SENSOR_REG_T ov13b10_snapshot_setting[] = {
//    {0x0100, 0x00},
//Full size 4208x3120 setting 	
//29.8fps,line time 10.5us, 1120Mbps/lane	
//register	value
{0x0305, 0x46},
{0x3501, 0x0c},
{0x3502, 0x10},
{0x3662, 0x92},
{0x3714, 0x24},
{0x371a, 0x3e},
{0x3739, 0x12},
{0x37c2, 0x04},
{0x37d9, 0x0c},
{0x3800, 0x00},
{0x3801, 0x00},
{0x3802, 0x00},
{0x3803, 0x08},
{0x3804, 0x10},
{0x3805, 0x8f},
{0x3806, 0x0c},
{0x3807, 0x47},
{0x3808, 0x10},
{0x3809, 0x70},
{0x380a, 0x0c},
{0x380b, 0x30},
{0x380c, 0x04},
{0x380d, 0x98},
{0x380e, 0x0c},
{0x380f, 0x7c},
{0x3811, 0x0f},
{0x3813, 0x08},
{0x3814, 0x01},
{0x3815, 0x01},
{0x3816, 0x01},
{0x3817, 0x01},
{0x3820, 0x88},
{0x3c38, 0x19},
{0x4008, 0x02},
{0x4009, 0x0f},
{0x4050, 0x02},
{0x4051, 0x09},
{0x4501, 0x00},
{0x4505, 0x00},
{0x4837, 0x0e},
{0x5000, 0xff},
{0x5001, 0x0f},	
	};

static struct sensor_res_tab_info s_ov13b10_resolution_tab_raw[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .reg_tab =
         {{ADDR_AND_LEN_OF_ARRAY(ov13b10_init_setting), PNULL, 0, .width = 0,
           .height = 0, .xclk_to_sensor = EX_MCLK,
           .image_format = SENSOR_IMAGE_FORMAT_RAW},

          {ADDR_AND_LEN_OF_ARRAY(ov13b10_video_setting), PNULL, 0,
           .width = VIDEO_WIDTH, .height = VIDEO_HEIGHT,
           .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW},

          {ADDR_AND_LEN_OF_ARRAY(ov13b10_preview_setting), PNULL, 0,
           .width = PREVIEW_WIDTH, .height = PREVIEW_HEIGHT,
           .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW},

          {ADDR_AND_LEN_OF_ARRAY(ov13b10_snapshot_setting), PNULL, 0,
           .width = SNAPSHOT_WIDTH, .height = SNAPSHOT_HEIGHT,
           .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW}}}

    /*If there are multiple modules,please add here*/
};

static SENSOR_TRIM_T s_ov13b10_resolution_trim_tab[VENDOR_NUM] = {
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

static SENSOR_REG_T ov13b10_shutter_reg[] = {
    {0x3500, 0x00}, {0x3501, 0x00}, {0x3502, 0x00},
};

static struct sensor_i2c_reg_tab ov13b10_shutter_tab = {
    .settings = ov13b10_shutter_reg, .size = ARRAY_SIZE(ov13b10_shutter_reg),
};

static SENSOR_REG_T ov13b10_again_reg[] = {
    {0x3208, 0x00}, {0x3508, 0x04}, {0x3509, 0x00},
    {0x3208, 0x10}, {0x3208, 0xa0},
};

static struct sensor_i2c_reg_tab ov13b10_again_tab = {
    .settings = ov13b10_again_reg, .size = ARRAY_SIZE(ov13b10_again_reg),
};

static SENSOR_REG_T ov13b10_dgain_reg[] = {

};

static struct sensor_i2c_reg_tab ov13b10_dgain_tab = {
    .settings = ov13b10_dgain_reg, .size = ARRAY_SIZE(ov13b10_dgain_reg),
};

static SENSOR_REG_T ov13b10_frame_length_reg[] = {
    {0x380e, 0x0e}, {0x380f, 0xe0},
};

static struct sensor_i2c_reg_tab ov13b10_frame_length_tab = {
    .settings = ov13b10_frame_length_reg,
    .size = ARRAY_SIZE(ov13b10_frame_length_reg),
};

static struct sensor_aec_i2c_tag ov13b10_aec_info = {
    .slave_addr = (MAJOR_I2C_SLAVE_ADDR >> 1),
    .addr_bits_type = SENSOR_I2C_REG_16BIT,
    .data_bits_type = SENSOR_I2C_VAL_8BIT,
    .shutter = &ov13b10_shutter_tab,
    .again = &ov13b10_again_tab,
    .dgain = &ov13b10_dgain_tab,
    .frame_length = &ov13b10_frame_length_tab,
};

static SENSOR_STATIC_INFO_T s_ov13b10_static_info[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .static_info = {.f_num = 200,
                     .focal_length = 354,
                     .max_fps = 30,
                     .max_adgain = 15.5,
                     .ois_supported = 0,
                     .pdaf_supported = 3,
                     .exp_valid_frame_num = 1,
                     .clamp_level = 64,
                     .adgain_valid_frame_num = 1,
                     .fov_info = {{5.120f, 3.840f}, 4.042f}}}
    /*If there are multiple modules,please add here*/
};

static SENSOR_MODE_FPS_INFO_T s_ov13b10_mode_fps_info[VENDOR_NUM] = {
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

static struct sensor_module_info s_ov13b10_module_info_tab[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .module_info = {.major_i2c_addr = 0x6c >> 1,
                     .minor_i2c_addr = 0x20 >> 1,

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

static struct sensor_ic_ops s_ov13b10_ops_tab;
struct sensor_raw_info *s_ov13b10_mipi_raw_info_ptr = PNULL; //&s_ov13b10_mipi_raw_info;

/*==============================================================================
 * Description:
 * sensor all info
 * please modify this variable acording your spec
 *============================================================================*/
SENSOR_INFO_T g_ov13b10_mipi_raw_info = {
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
    .identify_code = {{.reg_addr = ov13b10_PID_ADDR,
                       .reg_value = ov13b10_PID_VALUE},
                      {.reg_addr = ov13b10_VER_ADDR,
                       .reg_value = ov13b10_VER_VALUE}},

    .source_width_max = SNAPSHOT_WIDTH,
    .source_height_max = SNAPSHOT_HEIGHT,
    .name = (cmr_s8 *)SENSOR_NAME,
    .image_format = SENSOR_IMAGE_FORMAT_RAW,

    .module_info_tab = s_ov13b10_module_info_tab,
    .module_info_tab_size = ARRAY_SIZE(s_ov13b10_module_info_tab),

    .resolution_tab_info_ptr = s_ov13b10_resolution_tab_raw,
    .sns_ops = &s_ov13b10_ops_tab,
    .raw_info_ptr = &s_ov13b10_mipi_raw_info_ptr,

    .video_tab_info_ptr = NULL,
    .sensor_version_info = (cmr_s8 *)"ov13b10_v1",
};

#endif
