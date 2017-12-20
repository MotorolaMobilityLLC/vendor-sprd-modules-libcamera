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
#ifndef _SENSOR_IMX351_MIPI_RAW_H_
#define _SENSOR_IMX351_MIPI_RAW_H_

#define VENDOR_NUM 3

#include <utils/Log.h>
#include "sensor.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"

#include "parameters_sharkle/sensor_imx351_raw_param_main.c"

//#include "parameters/sensor_imx351_otp_truly.h"

#define SENSOR_NAME "imx351_mipi_raw"
#ifdef CAMERA_SENSOR_BACK_I2C_SWITCH
#define I2C_SLAVE_ADDR 0x20
#else
#define I2C_SLAVE_ADDR 0x34
#endif

#define BINNING_FACTOR 2
#define imx351_PID_ADDR 0x0016
#define imx351_PID_VALUE 0x03
#define imx351_VER_ADDR 0x0017
#define imx351_VER_VALUE 0x51

/* sensor parameters begin */
/* effective sensor output image size */
#if 0 // defined(CONFIG_CAMERA_SIZE_LIMIT_FOR_ANDROIDGO)
#define SNAPSHOT_WIDTH 2328  // 5344
#define SNAPSHOT_HEIGHT 1744 // 4016
#else
#define SNAPSHOT_WIDTH 4656  // 5344
#define SNAPSHOT_HEIGHT 3496 // 4016
#endif
#define PREVIEW_WIDTH 2328  // 2672
#define PREVIEW_HEIGHT 1744 // 2008

/*Mipi output*/
#define LANE_NUM 4
#define RAW_BITS 10

#define SNAPSHOT_MIPI_PER_LANE_BPS 1316
#define PREVIEW_MIPI_PER_LANE_BPS 374

/* please ref your spec */
#define FRAME_OFFSET 8
#define SENSOR_MAX_GAIN 0xF0
#define SENSOR_BASE_GAIN 0x20
#define SENSOR_MIN_SHUTTER 8

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

static struct sensor_ic_ops s_imx351_ops_tab;
struct sensor_raw_info *s_imx351_mipi_raw_info_ptr = &s_imx351_mipi_raw_info;

static const SENSOR_REG_T imx351_init_setting[] = {
    {0x0136, 0x18}, {0x0137, 0x00},
    {0x3C7D, 0x28}, {0x3C7E, 0x01},
    {0x3C7F, 0x0F}, {0x3140, 0x02},
    {0x3F7F, 0x01}, {0x4430, 0x05},
    {0x4431, 0xDC}, {0x4ED0, 0x01},
    {0x4ED1, 0x3E}, {0x4EDE, 0x01},
    {0x4EDF, 0x45}, {0x5222, 0x02},
    {0x5617, 0x0A}, {0x562B, 0x0A},
    {0x562D, 0x0C}, {0x56B7, 0x74},
    {0x6282, 0x82}, {0x6283, 0x80},
    {0x6286, 0x07}, {0x6287, 0xC0},
    {0x6288, 0x08}, {0x628A, 0x18},
    {0x628B, 0x80}, {0x628C, 0x20},
    {0x628E, 0x32}, {0x6290, 0x40},
    {0x6292, 0x0A}, {0x6296, 0x50},
    {0x629A, 0xF8}, {0x629B, 0x01},
    {0x629D, 0x03}, {0x629F, 0x04},
    {0x62B1, 0x06}, {0x62B5, 0x3C},
    {0x62B9, 0xC8}, {0x62BC, 0x02},
    {0x62BD, 0x70}, {0x62D0, 0x06},
    {0x62D4, 0x38}, {0x62D8, 0xB8},
    {0x62DB, 0x02}, {0x62DC, 0x40},
    {0x62DD, 0x03}, {0x637A, 0x11},
    {0x7BA0, 0x01}, {0x7BA9, 0x00},
    {0x7BAA, 0x01}, {0x7BAD, 0x00},
    {0x886B, 0x00}, {0x9002, 0x00},
    {0x9003, 0x00}, {0x9004, 0x09},
    {0x9006, 0x01}, {0x9200, 0x93},
    {0x9201, 0x85}, {0x9202, 0x93},
    {0x9203, 0x87}, {0x9204, 0x93},
    {0x9205, 0x8D}, {0x9206, 0x93},
    {0x9207, 0x8F}, {0x9208, 0x6A},
    {0x9209, 0x22}, {0x920A, 0x6A},
    {0x920B, 0x23}, {0x920C, 0x6A},
    {0x920D, 0x0F}, {0x920E, 0x71},
    {0x920F, 0x03}, {0x9210, 0x71},
    {0x9211, 0x0B}, {0x935D, 0x01},
    {0x9389, 0x05}, {0x938B, 0x05},
    {0x9391, 0x05}, {0x9393, 0x05},
    {0x9395, 0x82}, {0x9397, 0x78},
    {0x9399, 0x05}, {0x939B, 0x05},
    {0xA91F, 0x04}, {0xA921, 0x03},
    {0xA923, 0x02}, {0xA93D, 0x05},
    {0xA93F, 0x03}, {0xA941, 0x02},
    {0xA9AF, 0x04}, {0xA9B1, 0x03},
    {0xA9B3, 0x02}, {0xA9CD, 0x05},
    {0xA9CF, 0x03}, {0xA9D1, 0x02},
    {0xAA3F, 0x04}, {0xAA41, 0x03},
    {0xAA43, 0x02}, {0xAA5D, 0x05},
    {0xAA5F, 0x03}, {0xAA61, 0x02},
    {0xAACF, 0x04}, {0xAAD1, 0x03},
    {0xAAD3, 0x02}, {0xAAED, 0x05},
    {0xAAEF, 0x03}, {0xAAF1, 0x02},
    {0xAB87, 0x04}, {0xAB89, 0x03},
    {0xAB8B, 0x02}, {0xABA5, 0x05},
    {0xABA7, 0x03}, {0xABA9, 0x02},
    {0xABB7, 0x04}, {0xABB9, 0x03},
    {0xABBB, 0x02}, {0xABD5, 0x05},
    {0xABD7, 0x03}, {0xABD9, 0x02},
    {0xB388, 0x28}, {0xBC40, 0x03}, //  Image Quality adjustment setting
    {0x7B80, 0x00}, {0x7B81, 0x00},
    {0xA900, 0x20}, {0xA901, 0x20},
    {0xA902, 0x20}, {0xA903, 0x15},
    {0xA904, 0x15}, {0xA905, 0x15},
    {0xA906, 0x20}, {0xA907, 0x20},
    {0xA908, 0x20}, {0xA909, 0x15},
    {0xA90A, 0x15}, {0xA90B, 0x15},
    {0xA915, 0x3F}, {0xA916, 0x3F},
    {0xA917, 0x3F}, {0xA949, 0x03},
    {0xA94B, 0x03}, {0xA94D, 0x03},
    {0xA94F, 0x06}, {0xA951, 0x06},
    {0xA953, 0x06}, {0xA955, 0x03},
    {0xA957, 0x03}, {0xA959, 0x03},
    {0xA95B, 0x06}, {0xA95D, 0x06},
    {0xA95F, 0x06}, {0xA98B, 0x1F},
    {0xA98D, 0x1F}, {0xA98F, 0x1F},
    {0xAA21, 0x20}, {0xAA22, 0x20},
    {0xAA24, 0x15}, {0xAA25, 0x15},
    {0xAA26, 0x20}, {0xAA27, 0x20},
    {0xAA28, 0x20}, {0xAA29, 0x15},
    {0xAA2A, 0x15}, {0xAA2B, 0x15},
    {0xAA35, 0x3F}, {0xAA36, 0x3F},
    {0xAA37, 0x3F}, {0xAA6B, 0x03},
    {0xAA6D, 0x03}, {0xAA71, 0x06},
    {0xAA73, 0x06}, {0xAA75, 0x03},
    {0xAA77, 0x03}, {0xAA79, 0x03},
    {0xAA7B, 0x06}, {0xAA7D, 0x06},
    {0xAA7F, 0x06}, {0xAAAB, 0x1F},
    {0xAAAD, 0x1F}, {0xAAAF, 0x1F},
    {0xAAB0, 0x20}, {0xAAB1, 0x20},
    {0xAAB2, 0x20}, {0xAB53, 0x20},
    {0xAB54, 0x20}, {0xAB55, 0x20},
    {0xAB57, 0x40}, {0xAB59, 0x40},
    {0xAB5B, 0x40}, {0xAB63, 0x03},
    {0xAB65, 0x03}, {0xAB67, 0x03},
    {0x9A00, 0x0C}, {0x9A01, 0x0C},
    {0x9A06, 0x0C}, {0x9A18, 0x0C},
    {0x9A19, 0x0C}, {0xAA20, 0x3F},
    {0xAA23, 0x3F}, {0xAA32, 0x3F},
    {0xAA69, 0x3F}, {0xAA6F, 0x3F},
    {0xAAC2, 0x3F}, {0x8D1F, 0x00},
    {0x8D27, 0x00}, {0x9963, 0x64},
    {0x9964, 0x50}, {0xAC01, 0x0A},
    {0xAC03, 0x0A}, {0xAC05, 0x0A},
    {0xAC06, 0x01}, {0xAC07, 0xC0},
    {0xAC09, 0xC0}, {0xAC17, 0x0A},
    {0xAC19, 0x0A}, {0xAC1B, 0x0A},
    {0xAC1C, 0x01}, {0xAC1D, 0xC0},
    {0xAC1F, 0xC0}, {0xBCF1, 0x00},
};

static const SENSOR_REG_T imx351_2328x1744_setting[] = {
    /*       4Lane
       reg_2
       1/2Binning
       H: 2328
       V: 1744
       MIPI output setting
           Address value*/
    {0x0112, 0x0A}, //
    {0x0113, 0x0A}, //
    {0x0114, 0x03}, // Line Length PCK Setting
    {0x0342, 0x2F}, //
    {0x0343, 0x20}, // Frame Length Lines Setting
    {0x0340, 0x07}, //
    {0x0341, 0x14}, // ROI Setting
    {0x0344, 0x00}, //
    {0x0345, 0x00}, //
    {0x0346, 0x00}, //
    {0x0347, 0x04}, //
    {0x0348, 0x12}, //
    {0x0349, 0x2F}, //
    {0x034A, 0x0D}, //
    {0x034B, 0xA3}, // Mode Setting
    {0x0220, 0x00}, //
    {0x0221, 0x11}, //
    {0x0222, 0x01}, //
    {0x0381, 0x01}, //
    {0x0383, 0x01}, //
    {0x0385, 0x01}, //
    {0x0387, 0x01}, //
    {0x0900, 0x01}, //
    {0x0901, 0x22}, //
    {0x0902, 0x0A}, //
    {0x3243, 0x00}, //
    {0x3F4C, 0x01}, //
    {0x3F4D, 0x03}, //
    {0x4254, 0x7F}, // Digital Crop & Scaling
    {0x0401, 0x00}, //
    {0x0404, 0x00}, //
    {0x0405, 0x10}, //
    {0x0408, 0x00}, //
    {0x0409, 0x00}, //
    {0x040A, 0x00}, //
    {0x040B, 0x00}, //
    {0x040C, 0x09}, //
    {0x040D, 0x18}, //
    {0x040E, 0x06}, //
    {0x040F, 0xD0}, // Output Size Setting
    {0x034C, 0x09}, //
    {0x034D, 0x18}, //
    {0x034E, 0x06}, //
    {0x034F, 0xD0}, // Clock Setting
    {0x0301, 0x05}, //
    {0x0303, 0x02}, //
    {0x0305, 0x04}, //
    {0x0306, 0x01}, //
    {0x0307, 0x12}, //
    {0x030B, 0x04}, //
    {0x030D, 0x06}, //
    {0x030E, 0x01}, //
    {0x030F, 0x76}, //
    {0x0310, 0x01}, //
    {0x0820, 0x05}, //
    {0x0821, 0xD8}, //
    {0x0822, 0x00}, //
    {0x0823, 0x00}, //
    {0xBC41, 0x01}, // PDAF Setting
    {0x3E20, 0x01}, //
    {0x3E37, 0x00}, // 0x01},// enable
    {0x3E3B, 0x00}, // Other Setting
    {0x0106, 0x00}, //
    {0x0B00, 0x00}, //
    {0x3230, 0x00}, //
    {0x3C00, 0x6D}, //
    {0x3C01, 0x5B}, //
    {0x3C02, 0x77}, //
    {0x3C03, 0x66}, //
    {0x3C04, 0x00}, //
    {0x3C05, 0x84}, //
    {0x3C06, 0x14}, //
    {0x3C07, 0x00}, //
    {0x3C08, 0x01}, //
    {0x3F14, 0x01}, //
    {0x3F17, 0x00}, //
    {0x3F3C, 0x01}, //
    {0x3F78, 0x03}, //
    {0x3F79, 0xA4}, //
    {0x3F7C, 0x00}, //
    {0x3F7D, 0x00}, //
    {0x97C1, 0x04}, //
    {0x97C5, 0x0C}, // Integration Setting
    {0x0202, 0x07}, //
    {0x0203, 0x00}, //
    {0x0224, 0x01}, //
    {0x0225, 0xF4}, // Gain Setting
    {0x0204, 0x00}, //
    {0x0205, 0x00}, //
    {0x0216, 0x00}, //
    {0x0217, 0x00}, //
    {0x020E, 0x01}, //
    {0x020F, 0x00}, //
    {0x0218, 0x01}, //
    {0x0219, 0x00}, //

};

static const SENSOR_REG_T imx351_4656x3496_setting[] = {
    /*   4Lane
       reg_1
       Full size
       H: 4656
       V: 3496
       MIPI output setting
           Address value*/
    {0x0112, 0x0A}, //
    {0x0113, 0x0A}, //
    {0x0114, 0x03}, // Line Length PCK Setting
    {0x0342, 0x17}, //
    {0x0343, 0x90}, // Frame Length Lines Setting
    {0x0340, 0x0D}, //
    {0x0341, 0xEC}, // ROI Setting
    {0x0344, 0x00}, //
    {0x0345, 0x00}, //
    {0x0346, 0x00}, //
    {0x0347, 0x00}, //
    {0x0348, 0x12}, //
    {0x0349, 0x2F}, //
    {0x034A, 0x0D}, //
    {0x034B, 0xA7}, // Mode Setting
    {0x0220, 0x00}, //
    {0x0221, 0x11}, //
    {0x0222, 0x01}, //
    {0x0381, 0x01}, //
    {0x0383, 0x01}, //
    {0x0385, 0x01}, //
    {0x0387, 0x01}, //
    {0x0900, 0x00}, //
    {0x0901, 0x11}, //
    {0x0902, 0x0A}, //
    {0x3243, 0x00}, //
    {0x3F4C, 0x01}, //
    {0x3F4D, 0x01}, //
    {0x4254, 0x7F}, // Digital Crop & Scaling
    {0x0401, 0x00}, //
    {0x0404, 0x00}, //
    {0x0405, 0x10}, //
    {0x0408, 0x00}, //
    {0x0409, 0x00}, //
    {0x040A, 0x00}, //
    {0x040B, 0x00}, //
    {0x040C, 0x12}, //
    {0x040D, 0x30}, //
    {0x040E, 0x0D}, //
    {0x040F, 0xA8}, // Output Size Setting
    {0x034C, 0x12}, //
    {0x034D, 0x30}, //
    {0x034E, 0x0D}, //
    {0x034F, 0xA8}, // Clock Setting
    {0x0301, 0x05}, //
    {0x0303, 0x02}, //
    {0x0305, 0x04}, //
    {0x0306, 0x01}, //
    {0x0307, 0x0D}, //
    {0x030B, 0x01}, //
    {0x030D, 0x06}, //
    {0x030E, 0x01}, //
    {0x030F, 0x49}, //
    {0x0310, 0x01}, //
    {0x0820, 0x14}, //
    {0x0821, 0x90}, //
    {0x0822, 0x00}, //
    {0x0823, 0x00}, //
    {0xBC41, 0x01}, // PDAF Setting
    {0x3E20, 0x01}, // enable
    {0x3E37, 0x00}, // 0x01},//
    {0x3E3B, 0x00}, // Other Setting
    {0x0106, 0x00}, //
    {0x0B00, 0x00}, //
    {0x3230, 0x00}, //
    {0x3C00, 0x5B}, //
    {0x3C01, 0x54}, //
    {0x3C02, 0x77}, //
    {0x3C03, 0x66}, //
    {0x3C04, 0x00}, //
    {0x3C05, 0xC8}, //
    {0x3C06, 0x14}, //
    {0x3C07, 0x00}, //
    {0x3C08, 0x01}, //
    {0x3F14, 0x01}, //
    {0x3F17, 0x00}, //
    {0x3F3C, 0x01}, //
    {0x3F78, 0x00}, //
    {0x3F79, 0xB4}, //
    {0x3F7C, 0x00}, //
    {0x3F7D, 0x00}, //
    {0x97C1, 0x00}, //
    {0x97C5, 0x14}, // Integration Setting
    {0x0202, 0x0D}, //
    {0x0203, 0xD8}, //
    {0x0224, 0x01}, //
    {0x0225, 0xF4}, // Gain Setting
    {0x0204, 0x00}, //
    {0x0205, 0x00}, //
    {0x0216, 0x00}, //
    {0x0217, 0x00}, //
    {0x020E, 0x01}, //
    {0x020F, 0x00}, //
    {0x0218, 0x01}, //
    {0x0219, 0x00}, //

};

static const SENSOR_REG_T imx351_1280x720_setting[] = {
    /*    4Lane
        reg_3
        720p@90fps
        H: 1280
        V: 720
        MIPI output setting
            Address value*/
    {0x0112, 0x0A}, //
    {0x0113, 0x0A}, //
    {0x0114, 0x03}, // Line Length PCK Setting
    {0x0342, 0x11}, //
    {0x0343, 0x9C}, // Frame Length Lines Setting
    {0x0340, 0x07}, //
    {0x0341, 0x18}, // ROI Setting
    {0x0344, 0x00}, //
    {0x0345, 0x00}, //
    {0x0346, 0x00}, //
    {0x0347, 0x00}, //
    {0x0348, 0x12}, //
    {0x0349, 0x2F}, //
    {0x034A, 0x0D}, //
    {0x034B, 0x9F}, // Mode Setting
    {0x0220, 0x00}, //
    {0x0221, 0x11}, //
    {0x0222, 0x01}, //
    {0x0381, 0x01}, //
    {0x0383, 0x01}, //
    {0x0385, 0x01}, //
    {0x0387, 0x01}, //
    {0x0900, 0x01}, //
    {0x0901, 0x22}, //
    {0x0902, 0x0A}, //
    {0x3243, 0x00}, //
    {0x3F4C, 0x01}, //
    {0x3F4D, 0x01}, //
    {0x4254, 0x7F}, // Digital Crop & Scaling
    {0x0401, 0x02}, //
    {0x0404, 0x00}, //
    {0x0405, 0x1D}, //
    {0x0408, 0x00}, //
    {0x0409, 0x04}, //
    {0x040A, 0x00}, //
    {0x040B, 0xDA}, //
    {0x040C, 0x09}, //
    {0x040D, 0x10}, //
    {0x040E, 0x05}, //
    {0x040F, 0x1A}, // Output Size Setting
    {0x034C, 0x05}, //
    {0x034D, 0x00}, //
    {0x034E, 0x02}, //
    {0x034F, 0xD0}, // Clock Setting
    {0x0301, 0x05}, //
    {0x0303, 0x02}, //
    {0x0305, 0x04}, //
    {0x0306, 0x01}, //
    {0x0307, 0x34}, //
    {0x030B, 0x02}, //
    {0x030D, 0x06}, //
    {0x030E, 0x01}, //
    {0x030F, 0x7B}, //
    {0x0310, 0x01}, //
    {0x0820, 0x0B}, //
    {0x0821, 0xD8}, //
    {0x0822, 0x00}, //
    {0x0823, 0x00}, //
    {0xBC41, 0x01}, // PDAF Setting
    {0x3E20, 0x01}, //
    {0x3E37, 0x00}, //
    {0x3E3B, 0x00}, // Other Setting
    {0x0106, 0x00}, //
    {0x0B00, 0x00}, //
    {0x3230, 0x00}, //
    {0x3C00, 0x5B}, //
    {0x3C01, 0x54}, //
    {0x3C02, 0x77}, //
    {0x3C03, 0x66}, //
    {0x3C04, 0x00}, //
    {0x3C05, 0x9A}, //
    {0x3C06, 0x14}, //
    {0x3C07, 0x00}, //
    {0x3C08, 0x01}, //
    {0x3F14, 0x01}, //
    {0x3F17, 0x00}, //
    {0x3F3C, 0x01}, //
    {0x3F78, 0x02}, //
    {0x3F79, 0xAC}, //
    {0x3F7C, 0x00}, //
    {0x3F7D, 0x00}, //
    {0x97C1, 0x04}, //
    {0x97C5, 0x0C}, // Integration Setting
    {0x0202, 0x07}, //
    {0x0203, 0x04}, //
    {0x0224, 0x01}, //
    {0x0225, 0xF4}, // Gain Setting
    {0x0204, 0x00}, //
    {0x0205, 0x00}, //
    {0x0216, 0x00}, //
    {0x0217, 0x00}, //
    {0x020E, 0x01}, //
    {0x020F, 0x00}, //
    {0x0218, 0x01}, //
    {0x0219, 0x00}, //

};

static struct sensor_reg_tag imx351_shutter_reg[] = {
    {0x0202, 0}, {0x0203, 0},
};

static struct sensor_i2c_reg_tab imx351_shutter_tab = {
    .settings = imx351_shutter_reg, .size = ARRAY_SIZE(imx351_shutter_reg),
};

static struct sensor_reg_tag imx351_again_reg[] = {
    {0x0204, 0}, {0x0205, 0},
};

static struct sensor_i2c_reg_tab imx351_again_tab = {
    .settings = imx351_again_reg, .size = ARRAY_SIZE(imx351_again_reg),
};

static struct sensor_reg_tag imx351_dgain_reg[] = {
    {0x020e, 0x00}, {0x020f, 0x00}, {0x0210, 0x00}, {0x0211, 0x00},
    {0x0212, 0x00}, {0x0213, 0x00}, {0x0214, 0x00}, {0x0215, 0x00},
};

struct sensor_i2c_reg_tab imx351_dgain_tab = {
    .settings = imx351_dgain_reg, .size = ARRAY_SIZE(imx351_dgain_reg),
};

static struct sensor_reg_tag imx351_frame_length_reg[] = {
    {0x0340, 0}, {0x0341, 0},
};

static struct sensor_i2c_reg_tab imx351_frame_length_tab = {
    .settings = imx351_frame_length_reg,
    .size = ARRAY_SIZE(imx351_frame_length_reg),
};

static struct sensor_aec_i2c_tag imx351_aec_info = {
    .slave_addr = (I2C_SLAVE_ADDR >> 1),
    .addr_bits_type = SENSOR_I2C_REG_16BIT,
    .data_bits_type = SENSOR_I2C_VAL_8BIT,
    .shutter = &imx351_shutter_tab,
    .again = &imx351_again_tab,
    .dgain = &imx351_dgain_tab,
    .frame_length = &imx351_frame_length_tab,
};

static SENSOR_STATIC_INFO_T s_imx351_static_info[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .static_info = {.f_num = 200,
                     .focal_length = 352,
                     .max_fps = 0,
                     .max_adgain = 16 * 16,
                     .ois_supported = 0,
                     .pdaf_supported = 0,
                     .exp_valid_frame_num = 1,
                     .clamp_level = 64,
                     .adgain_valid_frame_num = 1,
                     .fov_info = {{4.656f, 3.496f}, 3.698f}}},
};

/*==============================================================================
 * Description:
 * sensor fps info related to sensor mode need by isp
 * please modify this variable acording your spec
 *============================================================================*/
static SENSOR_MODE_FPS_INFO_T s_imx351_mode_fps_info[VENDOR_NUM] = {
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
       {SENSOR_MODE_SNAPSHOT_TWO_THIRD, 0, 1, 0, 0}}}},
};

/*==============================================================================
 * Description:
 * sensor all info
 * please modify this variable acording your spec
 *============================================================================*/
static struct sensor_res_tab_info s_imx351_resolution_tab_raw[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .reg_tab =
         {{ADDR_AND_LEN_OF_ARRAY(imx351_init_setting), PNULL, 0, .width = 0,
           .height = 0, .xclk_to_sensor = EX_MCLK,
           .image_format = SENSOR_IMAGE_FORMAT_RAW},
          /*{
             ADDR_AND_LEN_OF_ARRAY(imx351_1040x768_setting),1040,768,EX_MCLK,SENSOR_IMAGE_FORMAT_RAW
             },*/
          {ADDR_AND_LEN_OF_ARRAY(imx351_1280x720_setting), PNULL, 0,
           .width = 1280, .height = 720, .xclk_to_sensor = EX_MCLK,
           .image_format = SENSOR_IMAGE_FORMAT_RAW},
          {ADDR_AND_LEN_OF_ARRAY(imx351_2328x1744_setting), PNULL, 0,
           .width = 2328, .height = 1744, .xclk_to_sensor = EX_MCLK,
           .image_format = SENSOR_IMAGE_FORMAT_RAW},
          {ADDR_AND_LEN_OF_ARRAY(imx351_4656x3496_setting), PNULL, 0,
           .width = 4656, .height = 3496, .xclk_to_sensor = EX_MCLK,
           .image_format = SENSOR_IMAGE_FORMAT_RAW}}},
};

static SENSOR_TRIM_T s_imx351_resolution_trim_tab[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .trim_info = {{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
                   /*	{0,0, 1040, 768,10325,1296, 812, { 0,0,1040,768}},*/
                   {0, 0, 1280, 720, 6098, 758, 1816, {0, 0, 1280, 720}},
                   {0, 0, 2328, 1744, 18364, 374, 1812, {0, 0, 2328, 1744}},
                   {0, 0, 4656, 3496, 9343, 1316, 3564, {0, 0, 4656, 3496}}}},
};

static const SENSOR_REG_T
    s_imx351_preview_size_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
        /*video mode 0: ?fps */
        {{0xffff, 0xff}},
        /* video mode 1:?fps */
        {{0xffff, 0xff}},
        /* video mode 2:?fps */
        {{0xffff, 0xff}},
        /* video mode 3:?fps */
        {{0xffff, 0xff}}};

static const SENSOR_REG_T
    s_imx351_capture_size_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
        /*video mode 0: ?fps */
        {{0xffff, 0xff}},
        /* video mode 1:?fps */
        {{0xffff, 0xff}},
        /* video mode 2:?fps */
        {{0xffff, 0xff}},
        /* video mode 3:?fps */
        {{0xffff, 0xff}}};

static SENSOR_VIDEO_INFO_T s_imx351_video_info[] = {
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},

    {.ae_info =
         {{.min_frate = 30, .max_frate = 30, .line_time = 270, .gain = 90},
          {0, 0, 0, 0},
          {0, 0, 0, 0},
          {0, 0, 0, 0}},
     .setting_ptr = (struct sensor_reg_tag **)s_imx351_preview_size_video_tab},

    {.ae_info =
         {{.min_frate = 2, .max_frate = 5, .line_time = 338, .gain = 1000},
          {0, 0, 0, 0},
          {0, 0, 0, 0},
          {0, 0, 0, 0}},
     .setting_ptr = (struct sensor_reg_tag **)s_imx351_capture_size_video_tab},
};

static struct sensor_module_info s_imx351_module_info_tab[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .module_info = {.major_i2c_addr = 0x34 >> 1,
                     .minor_i2c_addr = 0x10 >> 1,
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

                     .sensor_interface =
                         {
                             .type = SENSOR_INTERFACE_TYPE_CSI2,
                             .bus_width = 4,
                             .pixel_width = 10,
                             .is_loose = 0,
                         },

                     .change_setting_skip_num = 1,
                     .horizontal_view_angle = 35,
                     .vertical_view_angle = 35}},
};

SENSOR_INFO_T g_imx351_mipi_raw_info = {
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
    .identify_code = {{.reg_addr = imx351_PID_ADDR,
                       .reg_value = imx351_PID_VALUE},
                      {.reg_addr = imx351_VER_ADDR,
                       .reg_value = imx351_VER_VALUE}},

    .source_width_max = SNAPSHOT_WIDTH,
    .source_height_max = SNAPSHOT_HEIGHT,
    .name = (cmr_s8 *)SENSOR_NAME,
    .image_format = SENSOR_IMAGE_FORMAT_RAW,

    .resolution_tab_info_ptr = s_imx351_resolution_tab_raw,
    .sns_ops = &s_imx351_ops_tab,
    .raw_info_ptr = &s_imx351_mipi_raw_info_ptr,
    .module_info_tab = s_imx351_module_info_tab,
    .module_info_tab_size = ARRAY_SIZE(s_imx351_module_info_tab),
    .video_tab_info_ptr = s_imx351_video_info,
    .ext_info_ptr = NULL,

    .sensor_version_info = (cmr_s8 *)"imx351v1"};

#endif
