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

#ifndef _SENSOR_imx386_MIPI_RAW_H_
#define _SENSOR_imx386_MIPI_RAW_H_

#include <utils/Log.h>
#include "sensor.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"
#ifdef _SENSOR_RAW_SHARKL3_H_
#include "parameters_sharkl3/sensor_imx386_raw_param_main.c"
#else
#include "parameters/sensor_imx386_raw_param_main.c"
#endif
//#define FEATURE_OTP

#define VENDOR_NUM 1
#define SENSOR_NAME "imx386_mipi_raw"
#define I2C_SLAVE_ADDR 0x34 /* 8bit slave address*/

#define imx386_PID_ADDR 0x0016
#define imx386_PID_VALUE 0x03
#define imx386_VER_ADDR 0x0017
#define imx386_VER_VALUE 0x86

/* sensor parameters begin */

/* effective sensor output image size */
#define VIDEO_WIDTH 1920
#define VIDEO_HEIGHT 1080
#define PREVIEW_WIDTH 2016
#define PREVIEW_HEIGHT 1508 // 1504//1508
#define SNAPSHOT_WIDTH 4032
#define SNAPSHOT_HEIGHT 3016 // 3008///3016

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

#define VIDEO_MIPI_PER_LANE_BPS 808     /* 2*Mipi clk */
#define PREVIEW_MIPI_PER_LANE_BPS 344   /* 2*Mipi clk */
#define SNAPSHOT_MIPI_PER_LANE_BPS 1024 /* 2*Mipi clk */

/*line time unit: 1ns*/
#define VIDEO_LINE_TIME 6980
#define PREVIEW_LINE_TIME 16395
#define SNAPSHOT_LINE_TIME 10488

/* frame length*/
#define VIDEO_FRAME_LENGTH 1182
#define PREVIEW_FRAME_LENGTH 1772
#define SNAPSHOT_FRAME_LENGTH 3102

/* please ref your spec */
#define FRAME_OFFSET 20
#define SENSOR_MAX_GAIN 0x2c0 // 0x0200
#define SENSOR_BASE_GAIN 0x0020
#define SENSOR_MIN_SHUTTER 1

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

static const SENSOR_REG_T imx386_init_setting[] = {
    // Sensor Information////////////////////////////
    // Sensor	  : imx386
    // Date		  : 2017-04-06
    // Customer        : SPRD_validation
    // MCLK	          : 24MHz
    // MIPI            : 4 Lane
    // Pixel order 	  : B 1st
    // BLC offset	  : 64code
    // Firmware Ver.   : v1.0
    ////////////////////////////////////////////////
    /*Module common default setting*/

    {0x0100, 0x00},
    {0x0101, 0x00},

    {0x0136, 0x18},
    {0x3A7D, 0x00},
    {0x3A7E, 0x01},
    {0x0137, 0x00},
    {0x3A7F, 0x05},
    {0x3100, 0x00},
    {0x3101, 0x40},
    {0x3102, 0x00},
    {0x3103, 0x10},
    {0x3104, 0x01},
    {0x3105, 0xE8},
    {0x3106, 0x01},
    {0x3107, 0xF0},
    {0x3150, 0x04},
    {0x3151, 0x03},
    {0x3152, 0x02},
    {0x3153, 0x01},
    {0x5A86, 0x00},
    {0x5A87, 0x82},
    {0x5D1A, 0x00},
    {0x5D95, 0x02},
    {0x5E1B, 0x00},
    {0x5F5A, 0x00},
    {0x5F5B, 0x04},
    {0x682C, 0x31},
    {0x6831, 0x31},
    {0x6835, 0x0E},
    {0x6836, 0x31},
    {0x6838, 0x30},
    {0x683A, 0x06},
    {0x683B, 0x33},
    {0x683D, 0x30},
    {0x6842, 0x31},
    {0x6844, 0x31},
    {0x6847, 0x31},
    {0x6849, 0x31},
    {0x684D, 0x0E},
    {0x684E, 0x32},
    {0x6850, 0x31},
    {0x6852, 0x06},
    {0x6853, 0x33},
    {0x6855, 0x31},
    {0x685A, 0x32},
    {0x685C, 0x33},
    {0x685F, 0x31},
    {0x6861, 0x33},
    {0x6865, 0x0D},
    {0x6866, 0x33},
    {0x6868, 0x31},
    {0x686B, 0x34},
    {0x686D, 0x31},
    {0x6872, 0x32},
    {0x6877, 0x33},
    {0x7FF0, 0x01},
    {0x7FF4, 0x08},
    {0x7FF5, 0x3C},
    {0x7FFA, 0x01},
    {0x7FFD, 0x00},
    {0x831E, 0x00},
    {0x831F, 0x00},
    {0x9301, 0xBD},
    {0x9B94, 0x03},
    {0x9B95, 0x00},
    {0x9B96, 0x08},
    {0x9B97, 0x00},
    {0x9B98, 0x0A},
    {0x9B99, 0x00},
    {0x9BA7, 0x18},
    {0x9BA8, 0x18},
    {0x9D04, 0x08},
    {0x9D50, 0x8C},
    {0x9D51, 0x64},
    {0x9D52, 0x50},
    {0x9E31, 0x04},
    {0x9E32, 0x04},
    {0x9E33, 0x04},
    {0x9E34, 0x04},
    {0xA200, 0x00},
    {0xA201, 0x0A},
    {0xA202, 0x00},
    {0xA203, 0x0A},
    {0xA204, 0x00},
    {0xA205, 0x0A},
    {0xA206, 0x01},
    {0xA207, 0xC0},
    {0xA208, 0x00},
    {0xA209, 0xC0},
    {0xA20C, 0x00},
    {0xA20D, 0x0A},
    {0xA20E, 0x00},
    {0xA20F, 0x0A},
    {0xA210, 0x00},
    {0xA211, 0x0A},
    {0xA212, 0x01},
    {0xA213, 0xC0},
    {0xA214, 0x00},
    {0xA215, 0xC0},
    {0xA300, 0x00},
    {0xA301, 0x0A},
    {0xA302, 0x00},
    {0xA303, 0x0A},
    {0xA304, 0x00},
    {0xA305, 0x0A},
    {0xA306, 0x01},
    {0xA307, 0xC0},
    {0xA308, 0x00},
    {0xA309, 0xC0},
    {0xA30C, 0x00},
    {0xA30D, 0x0A},
    {0xA30E, 0x00},
    {0xA30F, 0x0A},
    {0xA310, 0x00},
    {0xA311, 0x0A},
    {0xA312, 0x01},
    {0xA313, 0xC0},
    {0xA314, 0x00},
    {0xA315, 0xC0},
    {0xBC19, 0x01},
    {0xBC1C, 0x0A},
    // Image Tuning Setting
    {0x3035, 0x01},
    {0x3051, 0x00},
    {0x7F47, 0x00},
    {0x7F78, 0x00},
    {0x7F89, 0x00},
    {0x7F93, 0x00},
    {0x7FB4, 0x00},
    {0x7FCC, 0x01},
    {0x9D02, 0x00},
    {0x9D44, 0x8C},
    {0x9D62, 0x8C},
    {0x9D63, 0x50},
    {0x9D64, 0x1B},
    {0x9E0D, 0x00},
    {0x9E0E, 0x00},
    {0x9E15, 0x0A},
    {0x9F02, 0x00},
    {0x9F03, 0x23},
    {0x9F4E, 0x00},
    {0x9F4F, 0x42},
    {0x9F54, 0x00},
    {0x9F55, 0x5A},
    {0x9F6E, 0x00},
    {0x9F6F, 0x10},
    {0x9F72, 0x00},
    {0x9F73, 0xC8},
    {0x9F74, 0x00},
    {0x9F75, 0x32},
    {0x9FD3, 0x00},
    {0x9FD4, 0x00},
    {0x9FD5, 0x00},
    {0x9FD6, 0x3C},
    {0x9FD7, 0x3C},
    {0x9FD8, 0x3C},
    {0x9FD9, 0x00},
    {0x9FDA, 0x00},
    {0x9FDB, 0x00},
    {0x9FDC, 0xFF},
    {0x9FDD, 0xFF},
    {0x9FDE, 0xFF},
    {0xA002, 0x00},
    {0xA003, 0x14},
    {0xA04E, 0x00},
    {0xA04F, 0x2D},
    {0xA054, 0x00},
    {0xA055, 0x40},
    {0xA06E, 0x00},
    {0xA06F, 0x10},
    {0xA072, 0x00},
    {0xA073, 0xC8},
    {0xA074, 0x00},
    {0xA075, 0x32},
    {0xA0CA, 0x04},
    {0xA0CB, 0x04},
    {0xA0CC, 0x04},
    {0xA0D3, 0x0A},
    {0xA0D4, 0x0A},
    {0xA0D5, 0x0A},
    {0xA0D6, 0x00},
    {0xA0D7, 0x00},
    {0xA0D8, 0x00},
    {0xA0D9, 0x18},
    {0xA0DA, 0x18},
    {0xA0DB, 0x18},
    {0xA0DC, 0x00},
    {0xA0DD, 0x00},
    {0xA0DE, 0x00},
    {0xBCB2, 0x01},

    {0x4041, 0x00},
};

static const SENSOR_REG_T imx386_preview_setting[] = {
    // Sensor Information////////////////////////////
    // Sensor	  : imx386
    // Date		  : 2017-04-06
    // Customer        : SPRD_validation
    // Image size	  : 1632x1224
    // MCLK/PCLK	  : 24MHz /288Mhz
    // MIPI speed(Mbps): 360Mbps x 4Lane
    // Frame Length	  : 2492
    // Line Length 	  : 3800
    // line Time       :13194
    // Max Fps 	  : 30.00fps
    // Pixel order 	  : Green 1st (=GB)
    // X/Y-flip        : X-flip
    // BLC offset	    : 64code
    // Firmware Ver.   : v1.0
    ////////////////////////////////////////////////
    // Mode Setting
    {0x0112, 0x0A},
    {0x0113, 0x0A},
    // Clock Setting
    {0x0301, 0x06},
    {0x0303, 0x02},
    {0x0305, 0x02},
    {0x0306, 0x00},
    {0x0307, 0x1E},
    {0x0309, 0x0A},
    {0x030B, 0x01},
    {0x030D, 0x0C},
    {0x030E, 0x00},
    {0x030F, 0xAC},
    {0x0310, 0x01},
    // Output Size Setting
    {0x0342, 0x08},
    {0x0343, 0xD0},
    {0x0340, 0x06},
    {0x0341, 0xEC},
    {0x0344, 0x00},
    {0x0345, 0x00},
    {0x0346, 0x00},
    {0x0347, 0x00},
    {0x0348, 0x0F},
    {0x0349, 0xBF},
    {0x034A, 0x0B},
    {0x034B, 0xC7}, // 0xBF}, //0xC7
    {0x0385, 0x01},
    {0x0387, 0x01},
    {0x0900, 0x01},
    {0x0901, 0x22},
    {0x300D, 0x00},
    {0x302E, 0x00},
    {0x0401, 0x00},
    {0x0404, 0x00},
    {0x0405, 0x10},
    {0x040C, 0x07},
    {0x040D, 0xE0},
    {0x040E, 0x05},
    {0x040F, 0xE4},
    {0x034C, 0x07},
    {0x034D, 0xE0},
    {0x034E, 0x05},
    {0x034F, 0xE4},
    // Other Setting
    {0x0114, 0x03},
    {0x0408, 0x00},
    {0x0409, 0x00},
    {0x040A, 0x00},
    {0x040B, 0x00},
    {0x0902, 0x02},
    {0x3030, 0x00},
    {0x3031, 0x01},
    {0x3032, 0x00},
    {0x3047, 0x01},
    {0x3049, 0x01},
    {0x30E6, 0x00},
    {0x30E7, 0x00},
    {0x4E25, 0x80},
    {0x663A, 0x01},
    {0x9311, 0x3F},
    {0xA0CD, 0x0A},
    {0xA0CE, 0x0A},
    {0xA0CF, 0x0A},
    // Integration Time Setting
    {0x0202, 0x06},
    {0x0203, 0xE2},
    // Gain Setting
    {0x0204, 0x00},
    {0x0205, 0x00},
    {0x020E, 0x01},
    {0x020F, 0x00},
    {0x0210, 0x01},
    {0x0211, 0x00},
    {0x0212, 0x01},
    {0x0213, 0x00},
    {0x0214, 0x01},
    {0x0215, 0x00},

};

static const SENSOR_REG_T imx386_snapshot_setting[] = {
    // Sensor Information////////////////////////////
    // Sensor	  : imx386
    // Date		  : 2017-04-06
    // Customer        : SPRD_validation
    // Image size	  : 3264x2448
    // MCLK/PCLK	  : 24MHz /288Mhz
    // MIPI speed(Mbps): 720Mbps x 4Lane
    // Frame Length	  : 2492
    // Line Length 	  : 3800
    // line Time       :13194
    // Max Fps 	  : 30.00fps
    // Pixel order 	  : Green 1st (=GB)
    // X/Y-flip        : X-flip
    // BLC offset	    : 64code
    // Firmware Ver.   : v1.0
    ////////////////////////////////////////////////
    // Mode Setting
    {0x0112, 0x0A},
    {0x0113, 0x0A},
    // Clock Setting
    {0x0301, 0x06},
    {0x0303, 0x02},
    {0x0305, 0x02},
    {0x0306, 0x00},
    {0x0307, 0x64},
    {0x0309, 0x0A},
    {0x030B, 0x01},
    {0x030D, 0x0C},
    {0x030E, 0x02},
    {0x030F, 0x00},
    {0x0310, 0x01},
    // Output Size Setting
    {0x0342, 0x10},
    {0x0343, 0xC8},
    {0x0340, 0x0C},
    {0x0341, 0x1E},
    {0x0344, 0x00},
    {0x0345, 0x00},
    {0x0346, 0x00},
    {0x0347, 0x00},
    {0x0348, 0x0F},
    {0x0349, 0xBF},
    {0x034A, 0x0B},
    {0x034B, 0xC7}, // 0xBF},	//0xC7
    {0x0385, 0x01},
    {0x0387, 0x01},
    {0x0900, 0x00},
    {0x0901, 0x11},
    {0x300D, 0x00},
    {0x302E, 0x00},
    {0x0401, 0x00},
    {0x0404, 0x00},
    {0x0405, 0x10},
    {0x040C, 0x0F},
    {0x040D, 0xC0},
    {0x040E, 0x0B},
    {0x040F, 0xC8},
    {0x034C, 0x0F},
    {0x034D, 0xC0},
    {0x034E, 0x0B},
    {0x034F, 0xC8},
    // Other Setting
    {0x0114, 0x03},
    {0x0408, 0x00},
    {0x0409, 0x00},
    {0x040A, 0x00},
    {0x040B, 0x00},
    {0x0902, 0x00},
    {0x3030, 0x00},
    {0x3031, 0x01},
    {0x3032, 0x00},
    {0x3047, 0x01},
    {0x3049, 0x01},
    {0x30E6, 0x02},
    {0x30E7, 0x59},
    {0x4E25, 0x80},
    {0x663A, 0x02},
    {0x9311, 0x00},
    {0xA0CD, 0x19},
    {0xA0CE, 0x19},
    {0xA0CF, 0x19},
    // Integration Time Setting
    {0x0202, 0x0C},
    {0x0203, 0x14},
    // Gain Setting
    {0x0204, 0x00},
    {0x0205, 0x00},
    {0x020E, 0x01},
    {0x020F, 0x00},
    {0x0210, 0x01},
    {0x0211, 0x00},
    {0x0212, 0x01},
    {0x0213, 0x00},
    {0x0214, 0x01},
    {0x0215, 0x00},
};

static const SENSOR_REG_T imx386_video_setting[] = {
    // Sensor Information////////////////////////////
    // Sensor	  : imx386
    // Date		  : 2017-04-06
    // Customer        : SPRD_validation
    // Image size	  : 1280x720
    // MCLK/PCLK	  : 24MHz /288Mhz
    // MIPI speed(Mbps): 360Mbps x 4Lane
    // Frame Length	  :  842
    // Line Length 	  : 3800
    // line Time       :13194
    // Max Fps 	  : 90.00fps
    // Pixel order 	  : Green 1st (=GB)
    // X/Y-flip        : X-flip
    // BLC offset	    : 64code
    // Firmware Ver.   : v1.0
    ////////////////////////////////////////////////
    // Mode Setting
    {0x0112, 0x0A},
    {0x0113, 0x0A},
    // Clock Setting
    {0x0301, 0x06},
    {0x0303, 0x02},
    {0x0305, 0x02},
    {0x0306, 0x00},
    {0x0307, 0x50},
    {0x0309, 0x0A},
    {0x030B, 0x01},
    {0x030D, 0x0C},
    {0x030E, 0x01},
    {0x030F, 0x94},
    {0x0310, 0x01},
    // Output Size Setting
    {0x0342, 0x08},
    {0x0343, 0xD0},
    {0x0340, 0x04},
    {0x0341, 0x9E},
    {0x0344, 0x00},
    {0x0345, 0x00},
    {0x0346, 0x01},
    {0x0347, 0x7C},
    {0x0348, 0x0F},
    {0x0349, 0xBF},
    {0x034A, 0x0A},
    {0x034B, 0x4B},
    {0x0385, 0x01},
    {0x0387, 0x01},
    {0x0900, 0x01},
    {0x0901, 0x22},
    {0x300D, 0x00},
    {0x302E, 0x00},
    {0x0401, 0x00},
    {0x0404, 0x00},
    {0x0405, 0x10},
    {0x040C, 0x07},
    {0x040D, 0x80},
    {0x040E, 0x04},
    {0x040F, 0x38},
    {0x034C, 0x07},
    {0x034D, 0x80},
    {0x034E, 0x04},
    {0x034F, 0x38},
    // Other Setting
    {0x0114, 0x03},
    {0x0408, 0x00},
    {0x0409, 0x30},
    {0x040A, 0x00},
    {0x040B, 0x18},
    {0x0902, 0x02},
    {0x3030, 0x00},
    {0x3031, 0x01},
    {0x3032, 0x00},
    {0x3047, 0x01},
    {0x3049, 0x00},
    {0x30E6, 0x00},
    {0x30E7, 0x00},
    {0x4E25, 0x80},
    {0x663A, 0x01},
    {0x9311, 0x3F},
    {0xA0CD, 0x0A},
    {0xA0CE, 0x0A},
    {0xA0CF, 0x0A},
    // Integration Time Setting
    {0x0202, 0x04},
    {0x0203, 0x94},
    // Gain Setting
    {0x0204, 0x00},
    {0x0205, 0x00},
    {0x020E, 0x01},
    {0x020F, 0x00},
    {0x0210, 0x01},
    {0x0211, 0x00},
    {0x0212, 0x01},
    {0x0213, 0x00},
    {0x0214, 0x01},
    {0x0215, 0x00},
};

static struct sensor_res_tab_info s_imx386_resolution_tab_raw[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .reg_tab =
         {{ADDR_AND_LEN_OF_ARRAY(imx386_init_setting), PNULL, 0, .width = 0,
           .height = 0, .xclk_to_sensor = EX_MCLK,
           .image_format = SENSOR_IMAGE_FORMAT_RAW},

          {ADDR_AND_LEN_OF_ARRAY(imx386_video_setting), PNULL, 0,
           .width = VIDEO_WIDTH, .height = VIDEO_HEIGHT,
           .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW},

          {ADDR_AND_LEN_OF_ARRAY(imx386_preview_setting), PNULL, 0,
           .width = PREVIEW_WIDTH, .height = PREVIEW_HEIGHT,
           .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW},

          {ADDR_AND_LEN_OF_ARRAY(imx386_snapshot_setting), PNULL, 0,
           .width = SNAPSHOT_WIDTH, .height = SNAPSHOT_HEIGHT,
           .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW}}}

    /*If there are multiple modules,please add here*/
};

static SENSOR_TRIM_T s_imx386_resolution_trim_tab[VENDOR_NUM] = {
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

static SENSOR_REG_T imx386_shutter_reg[] = {
    {0x0202, 0x0000}, {0x0203, 0x0000},
};

static struct sensor_i2c_reg_tab imx386_shutter_tab = {
    .settings = imx386_shutter_reg, .size = ARRAY_SIZE(imx386_shutter_reg),
};

static SENSOR_REG_T imx386_again_reg[] = {
    {0x0104, 0x0001}, {0x0204, 0x0000}, {0x0205, 0x0000}, {0x0104, 0x0000},
};

static struct sensor_i2c_reg_tab imx386_again_tab = {
    .settings = imx386_again_reg, .size = ARRAY_SIZE(imx386_again_reg),
};

static SENSOR_REG_T imx386_dgain_reg[] = {

};

static struct sensor_i2c_reg_tab imx386_dgain_tab = {
    .settings = imx386_dgain_reg, .size = ARRAY_SIZE(imx386_dgain_reg),
};

static SENSOR_REG_T imx386_frame_length_reg[] = {
    {0x0340, 0x0000}, {0x0341, 0x0000},
};

static struct sensor_i2c_reg_tab imx386_frame_length_tab = {
    .settings = imx386_frame_length_reg,
    .size = ARRAY_SIZE(imx386_frame_length_reg),
};

static struct sensor_aec_i2c_tag imx386_aec_info = {
    .slave_addr = (I2C_SLAVE_ADDR >> 1),
    .addr_bits_type = SENSOR_I2C_REG_16BIT,
    .data_bits_type = SENSOR_I2C_VAL_16BIT,
    .shutter = &imx386_shutter_tab,
    .again = &imx386_again_tab,
    .dgain = &imx386_dgain_tab,
    .frame_length = &imx386_frame_length_tab,
};

static const cmr_u16 imx386_pd_is_right[] = {
    0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1,
    0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1,
};

static const cmr_u16 imx386_pd_row[] = {
    7,  7,  23, 23, 43, 43, 59, 59, 11, 11, 27, 27, 39, 39, 55, 55,
    11, 11, 27, 27, 39, 39, 55, 55, 7,  7,  23, 23, 43, 43, 59, 59};

static const cmr_u16 imx386_pd_col[] = {
    0,  4,  4,  8,  4,  8,  0,  4,  20, 16, 24, 20, 24, 20, 20, 16,
    36, 40, 32, 36, 32, 36, 36, 40, 56, 52, 52, 48, 52, 48, 56, 52};

static SENSOR_STATIC_INFO_T s_imx386_static_info[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
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
                     .adgain_valid_frame_num = 0,
                     .fov_info = {{4.614f, 3.444f}, 4.222f}}}
    /*If there are multiple modules,please add here*/
};

static SENSOR_MODE_FPS_INFO_T s_imx386_mode_fps_info[VENDOR_NUM] = {
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

static struct sensor_module_info s_imx386_module_info_tab[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .module_info = {.major_i2c_addr = I2C_SLAVE_ADDR >> 1,
                     .minor_i2c_addr = I2C_SLAVE_ADDR >> 1,

                     .reg_addr_value_bits = SENSOR_I2C_REG_16BIT |
                                            SENSOR_I2C_VAL_8BIT |
                                            SENSOR_I2C_FREQ_400,

                     .avdd_val = SENSOR_AVDD_2800MV,
                     .iovdd_val = SENSOR_AVDD_1800MV,
                     .dvdd_val = SENSOR_AVDD_1200MV,

                     .image_pattern = SENSOR_IMAGE_PATTERN_RAWRGB_R,

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

static struct sensor_ic_ops s_imx386_ops_tab;
struct sensor_raw_info *s_imx386_mipi_raw_info_ptr = &s_imx386_mipi_raw_info;

/*==============================================================================
 * Description:
 * sensor all info
 * please modify this variable acording your spec
 *============================================================================*/
SENSOR_INFO_T g_imx386_mipi_raw_info = {
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
    .identify_code = {{.reg_addr = imx386_PID_ADDR,
                       .reg_value = imx386_PID_VALUE},
                      {.reg_addr = imx386_VER_ADDR,
                       .reg_value = imx386_VER_VALUE}},

    .source_width_max = SNAPSHOT_WIDTH,
    .source_height_max = SNAPSHOT_HEIGHT,
    .name = (cmr_s8 *)SENSOR_NAME,
    .image_format = SENSOR_IMAGE_FORMAT_RAW,

    .module_info_tab = s_imx386_module_info_tab,
    .module_info_tab_size = ARRAY_SIZE(s_imx386_module_info_tab),

    .resolution_tab_info_ptr = s_imx386_resolution_tab_raw,
    .sns_ops = &s_imx386_ops_tab,
    .raw_info_ptr = &s_imx386_mipi_raw_info_ptr,

    .video_tab_info_ptr = NULL,
    .sensor_version_info = (cmr_s8 *)"imx386_v1",
};

#endif
