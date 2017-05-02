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

#include <utils/Log.h>
#include "sensor.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"
//#include "af_dw9718s.h"
#if defined(CONFIG_DUAL_MODULE)
#include "parameters_dual/sensor_ov13855_raw_param_main.c"
#else
#include "parameters/sensor_ov13855_raw_param_main.c"
#endif

#define SENSOR_NAME "ov13855_mipi_raw"
#if defined(CONFIG_DUAL_MODULE)
#define I2C_SLAVE_ADDR 0x20 /* 16bit slave address*/
#else
#define I2C_SLAVE_ADDR 0x6c /* 16bit slave address*/
#endif

#define BINNING_FACTOR 1
#define ov13855_PID_ADDR 0x300A
#define ov13855_PID_VALUE 0x00
#define ov13855_VER_ADDR 0x300B
#define ov13855_VER_VALUE 0xD8
#define ov13855_VER_1_ADDR 0x300C
#define ov13855_VER_1_VALUE 0x55

/* sensor parameters begin */
/* effective sensor output image size */
#define SNAPSHOT_WIDTH 4224  // 5344
#define SNAPSHOT_HEIGHT 3136 // 4016
#define PREVIEW_WIDTH 2112   // 2672
#define PREVIEW_HEIGHT 1568

/*Mipi output*/
#define LANE_NUM 4
#define RAW_BITS 10

#define SNAPSHOT_MIPI_PER_LANE_BPS 1080
#define PREVIEW_MIPI_PER_LANE_BPS 540

/* please ref your spec */
#define FRAME_OFFSET 8 // 16 // 32
#define SENSOR_MAX_GAIN 0x7ff
#define SENSOR_BASE_GAIN 0x80
#define SENSOR_MIN_SHUTTER 4

/* isp parameters, please don't change it*/
#if defined(CONFIG_CAMERA_ISP_VERSION_V3) ||                                   \
    defined(CONFIG_CAMERA_ISP_VERSION_V4)
#define ISP_BASE_GAIN 0x80
#else
#define ISP_BASE_GAIN 0x10
#endif

/* please don't change it */
#define EX_MCLK 24

/*==============================================================================
 * Description:
 * global variable
 *============================================================================*/
// static struct hdr_info_t s_hdr_info;
// static uint32_t s_current_default_frame_length;
// struct sensor_ev_info_t s_sensor_ev_info;

static SENSOR_IOCTL_FUNC_TAB_T s_ov13855_ioctl_func_tab;
struct sensor_raw_info *s_ov13855_mipi_raw_info_ptr = &s_ov13855_mipi_raw_info;

static const SENSOR_REG_T ov13855_init_setting[] = {
    /*mipi 1080Mbps,4224,3136*/
    // Address   value
    {0x0103, 0x01}, {0x0300, 0x02}, {0x0301, 0x00}, {0x0302, 0x5a},
    {0x0303, 0x00}, {0x0304, 0x00}, {0x0305, 0x01}, {0x3022, 0x01},
    {0x3013, 0x32}, {0x3016, 0x72}, {0x301b, 0xF0}, {0x301f, 0xd0},
    {0x3106, 0x15}, {0x3107, 0x23}, {0x3500, 0x00}, {0x3501, 0x80},
    {0x3502, 0x00}, {0x3508, 0x02}, {0x3509, 0x00}, {0x350a, 0x00}, // 0x00
    {0x350e, 0x00}, {0x3510, 0x00}, {0x3511, 0x02}, {0x3512, 0x00},
    {0x3600, 0x2b}, {0x3601, 0x52}, {0x3602, 0x60}, {0x3612, 0x05},
    {0x3613, 0xa4}, {0x3620, 0x80}, {0x3621, 0x10}, {0x3622, 0x30},
    {0x3624, 0x1c}, {0x3640, 0x10}, {0x3641, 0x70}, {0x3661, 0x80},
    {0x3662, 0x12}, {0x3664, 0x73}, {0x3665, 0xa7}, {0x366e, 0xff},
    {0x366f, 0xf4}, {0x3674, 0x00}, {0x3679, 0x0c}, {0x367f, 0x01},
    {0x3680, 0x0c}, {0x3681, 0x60}, {0x3682, 0x17}, {0x3683, 0xa9},
    {0x3684, 0x9a}, {0x3709, 0x68}, {0x3714, 0x24}, {0x371a, 0x3e},
    {0x3737, 0x04}, {0x3738, 0xcc}, {0x3739, 0x12}, {0x373d, 0x26},
    {0x3764, 0x20}, {0x3765, 0x20}, {0x37a1, 0x36}, {0x37a8, 0x3b},
    {0x37ab, 0x31}, {0x37c2, 0x04}, {0x37c3, 0xf1}, {0x37c5, 0x00},
    {0x37d8, 0x03}, {0x37d9, 0x0c}, {0x37da, 0xc2}, {0x37dc, 0x02},
    {0x37e0, 0x00}, {0x37e1, 0x0a}, {0x37e2, 0x14}, {0x37e3, 0x04},
    {0x37e4, 0x2A}, {0x37e5, 0x03}, {0x37e6, 0x04}, {0x3800, 0x00},
    {0x3801, 0x00}, {0x3802, 0x00}, {0x3803, 0x08}, {0x3804, 0x10},
    {0x3805, 0x9f}, {0x3806, 0x0c}, {0x3807, 0x57}, {0x3808, 0x10},
    {0x3809, 0x80}, {0x380a, 0x0c}, {0x380b, 0x40}, {0x380c, 0x04},
    {0x380d, 0x62}, {0x380e, 0x0c}, {0x380f, 0x8e}, {0x3811, 0x10},
    {0x3813, 0x08}, {0x3814, 0x01}, {0x3815, 0x01}, {0x3816, 0x01},
#if defined(CONFIG_DUAL_MODULE)
    {0x3817, 0x01}, {0x3820, 0xb0}, {0x3821, 0x00}, {0x3822, 0xc2},
#else
    {0x3817, 0x01}, {0x3820, 0xa8}, {0x3821, 0x00}, {0x3822, 0xc2},
#endif
    {0x3823, 0x18}, {0x3826, 0x11}, {0x3827, 0x1c}, {0x3829, 0x03},
    {0x3832, 0x00}, {0x3c80, 0x00}, {0x3c87, 0x01}, {0x3c8c, 0x19},
    {0x3c8d, 0x1c}, {0x3c90, 0x00}, {0x3c91, 0x00}, {0x3c92, 0x00},
    {0x3c93, 0x00}, {0x3c94, 0x40}, {0x3c95, 0x54}, {0x3c96, 0x34},
    {0x3c97, 0x04}, {0x3c98, 0x00}, {0x3d8c, 0x73}, {0x3d8d, 0xc0},
    {0x3f00, 0x0b}, {0x3f03, 0x00}, {0x4001, 0xe0}, {0x4008, 0x00},
    {0x4009, 0x0f}, {0x4011, 0xf0}, {0x4017, 0x08}, {0x4050, 0x04},
    {0x4051, 0x0b}, {0x4052, 0x00}, {0x4053, 0x80}, {0x4054, 0x00},
    {0x4055, 0x80}, {0x4056, 0x00}, {0x4057, 0x80}, {0x4058, 0x00},
    {0x4059, 0x80}, {0x405e, 0x20}, {0x4500, 0x07}, {0x4503, 0x00},
    {0x450a, 0x04}, {0x4809, 0x04}, {0x480c, 0x12}, {0x481f, 0x30},
    {0x4833, 0x10}, {0x4837, 0x0e}, {0x4902, 0x01}, {0x4d00, 0x03},
    {0x4d01, 0xc9}, {0x4d02, 0xbc}, {0x4d03, 0xd7}, {0x4d04, 0xf0},
#ifndef CONFIG_CAMERA_PDAF
    {0x4d05, 0xa2}, {0x5000, 0xfd}, {0x5001, 0x05}, {0x5040, 0x39},
#else
    {0x4d05, 0xa2}, {0x5000, 0xff}, {0x5001, 0x07}, {0x5040, 0x39},
#endif
    {0x5041, 0x10}, {0x5042, 0x10}, {0x5043, 0x84}, {0x5044, 0x62},
    {0x5180, 0x00}, {0x5181, 0x10}, {0x5182, 0x02}, {0x5183, 0x0f},
    {0x5200, 0x1b}, {0x520b, 0x07}, {0x520c, 0x0f}, {0x5300, 0x04},
    {0x5301, 0x0C}, {0x5302, 0x0C}, {0x5303, 0x0f}, {0x5304, 0x00},
    {0x5305, 0x70}, {0x5306, 0x00}, {0x5307, 0x80}, {0x5308, 0x00},
    {0x5309, 0xa5}, {0x530a, 0x00}, {0x530b, 0xd3}, {0x530c, 0x00},
    {0x530d, 0xf0}, {0x530e, 0x01}, {0x530f, 0x10}, {0x5310, 0x01},
    {0x5311, 0x20}, {0x5312, 0x01}, {0x5313, 0x20}, {0x5314, 0x01},
    {0x5315, 0x20}, {0x5316, 0x08}, {0x5317, 0x08}, {0x5318, 0x10},
    {0x5319, 0x88}, {0x531a, 0x88}, {0x531b, 0xa9}, {0x531c, 0xaa},
    {0x531d, 0x0a}, {0x5405, 0x02}, {0x5406, 0x67}, {0x5407, 0x01},
    {0x5408, 0x4a}, {0x3503, 0x78},
    //   {0x0100,0x01},
};

static const SENSOR_REG_T ov13855_2112x1568_setting[] = {
    /*4Lane
       binning (4:3) 29.96fps
           line time 10.38
		   bps 540Mbps/lan
       H: 2112
       V: 1568
       Output format Setting
           Address value*/
    //	{0x0100,0x00},
    {0x0303, 0x01}, {0x3500, 0x00}, {0x3501, 0x40}, {0x3502, 0x00},
    {0x3662, 0x10}, {0x3714, 0x28}, {0x3737, 0x08}, {0x3739, 0x20},
    {0x37c2, 0x14}, {0x37e3, 0x08}, {0x37e4, 0x34}, {0x37e6, 0x08},
    {0x3800, 0x00}, {0x3801, 0x00}, {0x3802, 0x00}, {0x3803, 0x08},
    {0x3804, 0x10}, {0x3805, 0x9f}, {0x3806, 0x0c}, {0x3807, 0x4f},
    {0x3808, 0x08}, {0x3809, 0x40}, {0x380a, 0x06}, {0x380b, 0x20},
    {0x380c, 0x04}, {0x380d, 0x62}, {0x380e, 0x0c}, {0x380f, 0x90},
    {0x3811, 0x08}, {0x3813, 0x02}, {0x3814, 0x03}, {0x3816, 0x03},
#if defined(CONFIG_DUAL_MODULE)
    {0x3820, 0xb3}, {0x3826, 0x04}, {0x3827, 0x90}, {0x3829, 0x07},
#else
    {0x3820, 0xab}, {0x3826, 0x04}, {0x3827, 0x90}, {0x3829, 0x07},
#endif
    {0x4009, 0x0d}, {0x4050, 0x04}, {0x4051, 0x0b}, {0x4837, 0x1c},
    {0x4902, 0x01},
    // {0x0100,0x01},
};

static const SENSOR_REG_T ov13855_4224x3136_30fps_setting[] = {
    /*4Lane
    Full (4:3) 29.95fps
        line time 10.38
		bps 1080Mbps/lan
    H: 4224
    V: 3136
    Output format Setting
        Address value*/
    //  {0x0100,0x00},
    {0x0303, 0x00}, {0x3500, 0x00}, {0x3501, 0x80}, {0x3502, 0x00},
    {0x3662, 0x12}, {0x3714, 0x24}, {0x3737, 0x04}, {0x3739, 0x12},
    {0x37c2, 0x04}, {0x37e3, 0x04}, {0x37e4, 0x26}, {0x37e6, 0x04},
    {0x3800, 0x00}, {0x3801, 0x00}, {0x3802, 0x00}, {0x3803, 0x08},
    {0x3804, 0x10}, {0x3805, 0x9f}, {0x3806, 0x0c}, {0x3807, 0x57},
    {0x3808, 0x10}, {0x3809, 0x80}, {0x380a, 0x0c}, {0x380b, 0x40},
    {0x380c, 0x04}, {0x380d, 0x62}, {0x380e, 0x0c}, {0x380f, 0x8e},
    {0x3811, 0x10}, {0x3813, 0x08}, {0x3814, 0x01}, {0x3816, 0x01},
#if defined(CONFIG_DUAL_MODULE)
    {0x3820, 0xb0}, {0x3826, 0x11}, {0x3827, 0x1c}, {0x3829, 0x03},
#else
    {0x3820, 0xa8}, {0x3826, 0x11}, {0x3827, 0x1c}, {0x3829, 0x03},
#endif
    {0x4009, 0x0f}, {0x4050, 0x04}, {0x4051, 0x0b}, {0x4837, 0x0e},
    {0x4902, 0x01},
    //    {0x0100,0x01},

};

static const SENSOR_REG_T ov13855_4224x3136_15fps_setting[] = {
    /*4Lane
    Full (4:3) 14.97fps
        line time 20.22us
        bps 540Mbps/lan
    H: 4224
    V: 3136
    Output format Setting
        Address value*/
    //    {0x0100,0x00},
    {0x0303, 0x01}, {0x3500, 0x00}, {0x3501, 0x80}, {0x3502, 0x00},
    {0x3662, 0x12}, {0x3714, 0x24}, {0x3737, 0x04}, {0x3739, 0x12},
    {0x37c2, 0x04}, {0x37e3, 0x04}, {0x37e4, 0x26}, {0x37e6, 0x04},
    {0x3800, 0x00}, {0x3801, 0x00}, {0x3802, 0x00}, {0x3803, 0x08},
    {0x3804, 0x10}, {0x3805, 0x9f}, {0x3806, 0x0c}, {0x3807, 0x57},
    {0x3808, 0x10}, {0x3809, 0x80}, {0x380a, 0x0c}, {0x380b, 0x40},
    {0x380c, 0x08}, {0x380d, 0xc4}, {0x380e, 0x0c}, {0x380f, 0x8e},
    {0x3811, 0x10}, {0x3813, 0x08}, {0x3814, 0x01}, {0x3816, 0x01},
#if defined(CONFIG_DUAL_MODULE)
    {0x3820, 0xb0}, {0x3826, 0x11}, {0x3827, 0x1c}, {0x3829, 0x03},
#else
    {0x3820, 0xa8}, {0x3826, 0x11}, {0x3827, 0x1c}, {0x3829, 0x03},
#endif
    {0x4009, 0x0f}, {0x4050, 0x04}, {0x4051, 0x0b}, {0x4837, 0x1c},
    {0x4902, 0x01},
    // {0x0100,0x01},
};

static const SENSOR_REG_T ov13855_1024x768_setting[] = {
    /*4Lane
    HV1/4 (4:3) 119.72fps
		line time 10.38us
        bps 270Mbps/lan
    H: 1024
    V: 768
    Output format Setting
        Address value*/
    //    {0x0100,0x00},
    {0x0303, 0x03}, {0x3500, 0x00}, {0x3501, 0x20}, {0x3502, 0x00},
    {0x3662, 0x08}, {0x3714, 0x30}, {0x3737, 0x08}, {0x3739, 0x20},
    {0x37c2, 0x2c}, {0x37d9, 0x06}, {0x37e3, 0x08}, {0x37e4, 0x34},
    {0x37e6, 0x08}, {0x3800, 0x00}, {0x3801, 0x40}, {0x3802, 0x00},
    {0x3803, 0x40}, {0x3804, 0x10}, {0x3805, 0x5f}, {0x3806, 0x0c},
    {0x3807, 0x5f}, {0x3808, 0x04}, {0x3809, 0x00}, {0x380a, 0x03},
    {0x380b, 0x00}, {0x380c, 0x04}, {0x380d, 0x62}, {0x380e, 0x03},
    {0x380f, 0x24}, {0x3811, 0x04}, {0x3813, 0x04}, {0x3814, 0x07},
#if defined(CONFIG_DUAL_MODULE)
    {0x3816, 0x07}, {0x3820, 0xb4}, {0x3826, 0x04}, {0x3827, 0x48},
#else
    {0x3816, 0x07}, {0x3820, 0xac}, {0x3826, 0x04}, {0x3827, 0x48},
#endif
    {0x3829, 0x03}, {0x4009, 0x05}, {0x4050, 0x02}, {0x4051, 0x05},
    {0x4837, 0x38}, {0x4902, 0x02},
    // {0x0100,0x01},
};

static const SENSOR_REG_T ov13855_1280x720_setting[] = {
    /*4Lane
    HV1/4 (4:3) 90fps
        line time 10.38
        bps 540Mpbs/lane
    H: 1280
    V: 720
    Output format Setting
        Address value*/
    //   {0x0100,0x00},
    {0x0303, 0x01}, {0x3500, 0x00}, {0x3501, 0x40}, {0x3502, 0x00},
    {0x3662, 0x10}, {0x3714, 0x28}, {0x3737, 0x08}, {0x3739, 0x20},
    {0x37c2, 0x14}, {0x37e3, 0x08}, {0x37e4, 0x34}, {0x37e6, 0x08},
    {0x3800, 0x03}, {0x3801, 0x30}, {0x3802, 0x03}, {0x3803, 0x50},
    {0x3804, 0x0d}, {0x3805, 0x6f}, {0x3806, 0x09}, {0x3807, 0x0f},
    {0x3808, 0x05}, {0x3809, 0x00}, {0x380a, 0x02}, {0x380b, 0xd0},
    {0x380c, 0x04}, {0x380d, 0x62}, {0x380e, 0x04}, {0x380f, 0x2d},
    {0x3811, 0x08}, {0x3813, 0x08}, {0x3814, 0x03}, {0x3816, 0x03},
#if defined(CONFIG_DUAL_MODULE)
    {0x3820, 0xb3}, {0x3826, 0x04}, {0x3827, 0x90}, {0x3829, 0x07},
#else
    {0x3820, 0xab}, {0x3826, 0x04}, {0x3827, 0x90}, {0x3829, 0x07},
#endif
    {0x4009, 0x0d}, {0x4050, 0x04}, {0x4051, 0x0b}, {0x4837, 0x1c},
    {0x4902, 0x01},
    //    {0x0100,0x01},
};

/*==============================================================================
 * Description:
 * sensor static info need by isp
 * please modify this variable acording your spec
 *============================================================================*/
static SENSOR_STATIC_INFO_T s_ov13855_static_info = {
    .f_num = 200,         // f-number,focal ratio
    .focal_length = 354,  // focal_length;
    .max_fps = 0,         // max_fps,max fps of sensor's all settings,it will be
                          // calculated from sensor mode fps
    .max_adgain = 15 * 2, // max_adgain,AD-gain
    .ois_supported = 0,   // ois_supported;
    .pdaf_supported = 0,  // pdaf_supported;
    .exp_valid_frame_num = 1,    // exp_valid_frame_num;N+2-1
    .clamp_level = 64,           // clamp_level,black level
    .adgain_valid_frame_num = 1, // adgain_valid_frame_num;N+1-1
};

/*==============================================================================
 * Description:
 * sensor fps info related to sensor mode need by isp
 * please modify this variable acording your spec
 *============================================================================*/
static SENSOR_MODE_FPS_INFO_T s_ov13855_mode_fps_info = {
    0, // is_init;
    {{SENSOR_MODE_COMMON_INIT, 0, 1, 0, 0},
     {SENSOR_MODE_PREVIEW_ONE, 0, 1, 0, 0},
     {SENSOR_MODE_SNAPSHOT_ONE_FIRST, 0, 1, 0, 0},
     {SENSOR_MODE_SNAPSHOT_ONE_SECOND, 0, 1, 0, 0},
     {SENSOR_MODE_SNAPSHOT_ONE_THIRD, 0, 1, 0, 0},
     {SENSOR_MODE_PREVIEW_TWO, 0, 1, 0, 0},
     {SENSOR_MODE_SNAPSHOT_TWO_FIRST, 0, 1, 0, 0},
     {SENSOR_MODE_SNAPSHOT_TWO_SECOND, 0, 1, 0, 0},
     {SENSOR_MODE_SNAPSHOT_TWO_THIRD, 0, 1, 0, 0}}};

/*==============================================================================
 * Description:
 * sensor all info
 * please modify this variable acording your spec
 *============================================================================*/
LOCAL SENSOR_REG_TAB_INFO_T s_ov13855_resolution_tab_raw[] = {
    {ADDR_AND_LEN_OF_ARRAY(ov13855_init_setting),0, 0, 24, SENSOR_IMAGE_FORMAT_RAW},
    {ADDR_AND_LEN_OF_ARRAY(ov13855_1280x720_setting),1280,720, 24, SENSOR_IMAGE_FORMAT_RAW},
    /*{ADDR_AND_LEN_OF_ARRAY(ov13855_1024x768_setting),1024,768,24,SENSOR_IMAGE_FORMAT_RAW},*/
    {ADDR_AND_LEN_OF_ARRAY(ov13855_2112x1568_setting),2112,1568,24,SENSOR_IMAGE_FORMAT_RAW},
    {ADDR_AND_LEN_OF_ARRAY(ov13855_4224x3136_30fps_setting),4224,3136,24,SENSOR_IMAGE_FORMAT_RAW },
    {PNULL,0, 0,0,0,0 },
    {PNULL,0, 0, 0, 0, 0},
    {PNULL, 0,0,0, 0, 0},
    {PNULL,0,0,0, 0, 0},
    {PNULL,0,0,0,0,0 },
    {PNULL,0, 0, 0, 0, 0},
    {PNULL, 0,0,0, 0, 0},
    {PNULL,0,0,0, 0, 0},
    {PNULL,0,0,0,0,0 },
};

LOCAL SENSOR_TRIM_T s_ov13855_resolution_trim_tab[] = {
                {0,0, 0, 0, 0,0, 0, {0, 0, 0, 0 }},
                {0, 0,1280,720,10380,540,1069, {0, 0, 1280,720 }},
                /*{0,0,1024, 768,10380, 270,804,{ 0, 0, 1024, 768 }}, */
                { 0, 0, 2112,1568,10380,540,3216, { 0,0, 2112, 1568}},
                { 0, 0, 4224,3136,10380,1080,3214, { 0,0,4224,3136}},
                {0,0,0, 0, 0, 0,0,{ 0, 0,  0, 0}},
                {0, 0,0, 0, 0, 0,0,{ 0, 0, 0, 0 }},
                {0,0,0,0, 0, 0, 0, { 0, 0, 0,  0}},
                {0, 0, 0,0,0, 0,0, { 0, 0, 0,  0}},
                {0, 0, 0, 0,0, 0, 0,{0, 0, 0,  0}},
                {0,0,0, 0, 0, 0,0,{ 0, 0,  0, 0}},
                {0, 0,0, 0, 0, 0,0,{ 0, 0, 0, 0 }},
                {0,0,0,0, 0, 0, 0, { 0, 0, 0,  0}},
                {0, 0, 0,0,0, 0,0, { 0, 0, 0,  0}},
};

LOCAL const SENSOR_REG_T
    s_ov13855_preview_size_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
        {/*video mode 0: ?fps */
         {
             .reg_addr = 0xffff, .reg_value = 0xff,
         }},
        {
            /* video mode 1:?fps */
            {
                .reg_addr = 0xffff, .reg_value = 0xff,
            },
        },
        {
            /* video mode 2:?fps */
            {
                .reg_addr = 0xffff, .reg_value = 0xff,
            },
        },
        {
            /* video mode 3:?fps */
            {
                .reg_addr = 0xffff, .reg_value = 0xff,
            },
        },
};
LOCAL const SENSOR_REG_T
    s_ov13855_capture_size_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
        {
            /*video mode 0: ?fps */
            {
                .reg_addr = 0xffff, .reg_value = 0xff,
            },
        },
        {
            /* video mode 1:?fps */
            {
                .reg_addr = 0xffff, .reg_value = 0xff,
            },
        },
        {
            /* video mode 2:?fps */
            {
                .reg_addr = 0xffff, .reg_value = 0xff,
            },
        },
        {/* video mode 3:?fps */
         {
             .reg_addr = 0xffff, .reg_value = 0xff,
         }}};

LOCAL SENSOR_VIDEO_INFO_T s_ov13855_video_info[] = {
    {
        .ae_info =
            {
                {
                    .min_frate = 0, .max_frate = 0, .line_time = 0, .gain = 0,
                },
            },
        .setting_ptr = NULL,
    },
    {
        .ae_info =
            {
                {
                    .min_frate = 30,
                    .max_frate = 30,
                    .line_time = 270,
                    .gain = 90,
                },
            },
        .setting_ptr = s_ov13855_preview_size_video_tab,
    },
    {
        .ae_info =
            {
                {
                    .min_frate = 2,
                    .max_frate = 5,
                    .line_time = 338,
                    .gain = 1000,
                },
            },
        .setting_ptr = s_ov13855_capture_size_video_tab,
    }};

SENSOR_INFO_T g_ov13855_mipi_raw_info = {
    .salve_i2c_addr_w = I2C_SLAVE_ADDR >> 1,
    .salve_i2c_addr_r = I2C_SLAVE_ADDR >> 1,
    .reg_addr_value_bits =
        SENSOR_I2C_REG_16BIT | SENSOR_I2C_VAL_8BIT | SENSOR_I2C_FREQ_400,
    .hw_signal_polarity = SENSOR_HW_SIGNAL_PCLK_P | SENSOR_HW_SIGNAL_VSYNC_P |
                          SENSOR_HW_SIGNAL_HSYNC_P,
    .environment_mode = SENSOR_ENVIROMENT_NORMAL | SENSOR_ENVIROMENT_NIGHT,
    .image_effect = SENSOR_IMAGE_EFFECT_NORMAL |
                    SENSOR_IMAGE_EFFECT_BLACKWHITE | SENSOR_IMAGE_EFFECT_RED |
                    SENSOR_IMAGE_EFFECT_GREEN | SENSOR_IMAGE_EFFECT_BLUE |
                    SENSOR_IMAGE_EFFECT_YELLOW | SENSOR_IMAGE_EFFECT_NEGATIVE |
                    SENSOR_IMAGE_EFFECT_CANVAS,

    /* bit[0:7]: count of step in brightness, contrast, sharpness, saturation
    * bit[8:31] reseved
    */
    .wb_mode = 0,
    .step_count = 7,
    .reset_pulse_level =
        SENSOR_LOW_PULSE_RESET, /*here should be care when bring up*/
    .reset_pulse_width = 50,
    .power_down_level =
        SENSOR_LOW_LEVEL_PWDN, /*here should be care when bring up*/
    .identify_count = 1,
    .identify_code =
        {{
             .reg_addr = ov13855_PID_ADDR, .reg_value = ov13855_PID_VALUE,
         },
         {
             .reg_addr = ov13855_VER_ADDR, .reg_value = ov13855_VER_VALUE,
         },
         {
             .reg_addr = ov13855_VER_1_ADDR, .reg_value = ov13855_VER_1_VALUE,
         }

        },
    .avdd_val = SENSOR_AVDD_2800MV,
    .iovdd_val = SENSOR_AVDD_1800MV,
    .dvdd_val = SENSOR_AVDD_1200MV,
    .source_width_max = SNAPSHOT_WIDTH,   /* max width of source image */
    .source_height_max = SNAPSHOT_HEIGHT, /* max height of source image */
    .name = (cmr_s8 *)SENSOR_NAME,
    .image_format = SENSOR_IMAGE_FORMAT_RAW,
    .image_pattern = SENSOR_IMAGE_PATTERN_RAWRGB_B,

    .resolution_tab_info_ptr = s_ov13855_resolution_tab_raw,
    .ioctl_func_tab_ptr = &s_ov13855_ioctl_func_tab,
    .raw_info_ptr = &s_ov13855_mipi_raw_info_ptr,
    .ext_info_ptr = NULL,

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
    .i2c_dev_handler = 0,
    .sensor_interface =
        {
            .type = SENSOR_INTERFACE_TYPE_CSI2,
            .bus_width = 4,    /*lane number or bit-width*/
            .pixel_width = 10, /*bits per pixel*/
            .is_loose = 0,     /*0 packet, 1 half word per pixel*/
        },

    .video_tab_info_ptr = NULL,
    .change_setting_skip_num = 1,
    .horizontal_view_angle = 35,
    .vertical_view_angle = 35,
    .sensor_version_info = (cmr_s8 *)"ov13855v1",
};
