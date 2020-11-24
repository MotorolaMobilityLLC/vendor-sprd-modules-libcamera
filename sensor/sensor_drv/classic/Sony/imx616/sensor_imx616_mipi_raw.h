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

#ifndef _SENSOR_imx616_MIPI_RAW_H_
#define _SENSOR_imx616_MIPI_RAW_H_

#include <utils/Log.h>
#include "sensor.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"

//#define FEATURE_OTP

#define VENDOR_NUM 1
#define SENSOR_NAME "imx616_mipi_raw"
#define I2C_SLAVE_ADDR 0x34 /* 8bit slave address*/

#define imx616_PID_ADDR 0x0016
#define imx616_PID_VALUE 0x06
#define imx616_VER_ADDR 0x0017
#define imx616_VER_VALUE 0x16

/* sensor parameters begin */

/* effective sensor output image size */
#define VIDEO_WIDTH 1920
#define VIDEO_HEIGHT 1080
#define PREVIEW_WIDTH 3280 //3200
#define PREVIEW_HEIGHT 2464 //2432
#define SNAPSHOT_WIDTH 6560 //6528
#define SNAPSHOT_HEIGHT 4928 //4864

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

#define VIDEO_MIPI_PER_LANE_BPS 831     /* 2*Mipi clk */
#define PREVIEW_MIPI_PER_LANE_BPS 831   /* 2*Mipi clk */
#define SNAPSHOT_MIPI_PER_LANE_BPS 2090 /* 2*Mipi clk */

/*line time unit: 1ns*/
#define VIDEO_LINE_TIME 7130
#define PREVIEW_LINE_TIME 13000
#define SNAPSHOT_LINE_TIME 8320

/* frame length*/
#define VIDEO_FRAME_LENGTH 1166
#define PREVIEW_FRAME_LENGTH 2562
#define SNAPSHOT_FRAME_LENGTH 5005

/* please ref your spec */
#define FRAME_OFFSET 48
#define SENSOR_MAX_GAIN 0x2c0
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
static const SENSOR_REG_T imx616_init_setting[] = {
    /*	Stand-by OFF Sequence
                    Power ON
                    Input EXTCLK
                    XCLR OFF
                    External Clock Setting
                    Address value*/
    {0x0136, 0x18}, //
    {0x0137, 0x00}, //			Address value
    {0x3C7E, 0x01}, //
    {0x3C7F, 0x15}, //	Signaling mode setting
    {0x0111, 0x02}, //	Global Setting
    {0x380C, 0x00}, //
    {0x3C00, 0x01}, //
    {0x3C01, 0x00}, //
    {0x3C02, 0x00}, //
    {0x3C03, 0x03}, //
    {0x3C04, 0xFF}, //
    {0x3C05, 0x01}, //
    {0x3C06, 0x00}, //
    {0x3C07, 0x00}, //
    {0x3C08, 0x03}, //
    {0x3C09, 0xFF}, //
    {0x3C0A, 0x00}, //
    {0x3C0B, 0x00}, //
    {0x3C0C, 0x10}, //
    {0x3C0D, 0x10}, //
    {0x3C0E, 0x10}, //
    {0x3C0F, 0x10}, //
    {0x3C10, 0x10}, //
    {0x3C11, 0x20}, //
    {0x3C15, 0x00}, //
    {0x3C16, 0x00}, //
    {0x3C17, 0x01}, //
    {0x3C18, 0x00}, //
    {0x3C19, 0x01}, //
    {0x3C1A, 0x00}, //
    {0x3C1B, 0x01}, //
    {0x3C1C, 0x00}, //
    {0x3C1D, 0x01}, //
    {0x3C1E, 0x00}, //
    {0x3C1F, 0x00}, //
    {0x3F89, 0x01}, //
    {0x3F8F, 0x01}, //
    {0x53B9, 0x01}, //
    {0x62C4, 0x04}, //
    {0x658F, 0x07}, //
    {0x6590, 0x05}, //
    {0x6591, 0x07}, //
    {0x6592, 0x05}, //
    {0x6593, 0x07}, //
    {0x6594, 0x05}, //
    {0x6595, 0x07}, //
    {0x6596, 0x05}, //
    {0x6597, 0x05}, //
    {0x6598, 0x05}, //
    {0x6599, 0x05}, //
    {0x659A, 0x05}, //
    {0x659B, 0x05}, //
    {0x659C, 0x05}, //
    {0x659D, 0x05}, //
    {0x659E, 0x07}, //
    {0x659F, 0x05}, //
    {0x65A0, 0x07}, //
    {0x65A1, 0x05}, //
    {0x65A2, 0x07}, //
    {0x65A3, 0x05}, //
    {0x65A4, 0x07}, //
    {0x65A5, 0x05}, //
    {0x65A6, 0x05}, //
    {0x65A7, 0x05}, //
    {0x65A8, 0x05}, //
    {0x65A9, 0x05}, //
    {0x65AA, 0x05}, //
    {0x65AB, 0x05}, //
    {0x65AC, 0x05}, //
    {0x65AD, 0x07}, //
    {0x65AE, 0x07}, //
    {0x65AF, 0x07}, //
    {0x65B0, 0x05}, //
    {0x65B1, 0x05}, //
    {0x65B2, 0x05}, //
    {0x65B3, 0x05}, //
    {0x65B4, 0x07}, //
    {0x65B5, 0x07}, //
    {0x65B6, 0x07}, //
    {0x65B7, 0x07}, //
    {0x65B8, 0x05}, //
    {0x65B9, 0x05}, //
    {0x65BA, 0x05}, //
    {0x65BB, 0x05}, //
    {0x65BC, 0x05}, //
    {0x65BD, 0x05}, //
    {0x65BE, 0x05}, //
    {0x65BF, 0x05}, //
    {0x65C0, 0x05}, //
    {0x65C1, 0x05}, //
    {0x65C2, 0x05}, //
    {0x65C3, 0x05}, //
    {0x65C4, 0x05}, //
    {0x65C5, 0x05}, //
    {0x6E1C, 0x00}, //
    {0x6E1D, 0x00}, //
    {0x6E25, 0x00}, //
    {0x6E38, 0x03}, //
    {0x895C, 0x01}, //
    {0x895D, 0x00}, //
    {0x8966, 0x00}, //
    {0x8967, 0x4E}, //
    {0x896A, 0x00}, //
    {0x896B, 0x24}, //
    {0x896F, 0x34}, //
    {0x8976, 0x00}, //
    {0x8977, 0x00}, //
    {0x9004, 0x1F}, //
    {0x9200, 0xB7}, //
    {0x9201, 0x34}, //
    {0x9202, 0xB7}, //
    {0x9203, 0x36}, //
    {0x9204, 0xB7}, //
    {0x9205, 0x37}, //
    {0x9206, 0xB7}, //
    {0x9207, 0x38}, //
    {0x9208, 0xB7}, //
    {0x9209, 0x39}, //
    {0x920A, 0xB7}, //
    {0x920B, 0x3A}, //
    {0x920C, 0xB7}, //
    {0x920D, 0x3C}, //
    {0x920E, 0xB7}, //
    {0x920F, 0x3D}, //
    {0x9210, 0xB7}, //
    {0x9211, 0x3E}, //
    {0x9212, 0xB7}, //
    {0x9213, 0x3F}, //
    {0x9214, 0xF6}, //
    {0x9215, 0x13}, //
    {0x9216, 0xF6}, //
    {0x9217, 0x34}, //
    {0x9218, 0xF4}, //
    {0x9219, 0xA7}, //
    {0x921A, 0xF4}, //
    {0x921B, 0xAA}, //
    {0x921C, 0xF4}, //
    {0x921D, 0xAD}, //
    {0x921E, 0xF4}, //
    {0x921F, 0xB0}, //
    {0x9220, 0xF4}, //
    {0x9221, 0xB3}, //
    {0x9222, 0x85}, //
    {0x9223, 0x77}, //
    {0x9224, 0xC4}, //
    {0x9225, 0x4B}, //
    {0x9226, 0xC4}, //
    {0x9227, 0x4C}, //
    {0x9228, 0xC4}, //
    {0x9229, 0x4D}, //
    {0x922A, 0xF5}, //
    {0x922B, 0x5E}, //
    {0x922C, 0xF5}, //
    {0x922D, 0x5F}, //
    {0x922E, 0xF5}, //
    {0x922F, 0x64}, //
    {0x9230, 0xF5}, //
    {0x9231, 0x65}, //
    {0x9232, 0xF5}, //
    {0x9233, 0x6A}, //
    {0x9234, 0xF5}, //
    {0x9235, 0x6B}, //
    {0x9236, 0xF5}, //
    {0x9237, 0x70}, //
    {0x9238, 0xF5}, //
    {0x9239, 0x71}, //
    {0x923A, 0xF5}, //
    {0x923B, 0x76}, //
    {0x923C, 0xF5}, //
    {0x923D, 0x77}, //
    {0x9810, 0x14}, //
    {0x9814, 0x14}, //
    {0xC020, 0x00}, //
    {0xC026, 0x00}, //
    {0xC027, 0x00}, //
    {0xC448, 0x01}, //
    {0xC44F, 0x01}, //
    {0xC450, 0x00}, //
    {0xC451, 0x00}, //
    {0xC452, 0x01}, //
    {0xC455, 0x00}, //
    {0xE186, 0x36}, //
    {0xE206, 0x35}, //
    {0xE226, 0x33}, //
    {0xE266, 0x34}, //
    {0xE2A6, 0x31}, //
    {0xE2C6, 0x37}, //
    {0xE2E6, 0x32}, // Image Quality adjustment setting
    {0x88D6, 0x60}, //
    {0x9852, 0x00}, //
    {0xA569, 0x06}, //
    {0xA56A, 0x13}, //
    {0xA56B, 0x13}, //
    {0xA56C, 0x01}, //
    {0xA678, 0x00}, //
    {0xA679, 0x20}, //
    {0xA812, 0x00}, //
    {0xA813, 0x3F}, //
    {0xA814, 0x3F}, //
    {0xA830, 0x68}, //
    {0xA831, 0x56}, //
    {0xA832, 0x2B}, //
    {0xA833, 0x55}, //
    {0xA834, 0x55}, //
    {0xA835, 0x16}, //
    {0xA837, 0x51}, //
    {0xA838, 0x34}, //
    {0xA854, 0x4F}, //
    {0xA855, 0x48}, //
    {0xA856, 0x45}, //
    {0xA857, 0x02}, //
    {0xA85A, 0x23}, //
    {0xA85B, 0x16}, //
    {0xA85C, 0x12}, //
    {0xA85D, 0x02}, //
    {0xAA55, 0x00}, //
    {0xAA56, 0x01}, //
    {0xAA57, 0x30}, //
    {0xAA58, 0x01}, //
    {0xAA59, 0x30}, //
    {0xAC72, 0x01}, //
    {0xAC73, 0x26}, //
    {0xAC74, 0x01}, //
    {0xAC75, 0x26}, //
    {0xAC76, 0x00}, //
    {0xAC77, 0xC4}, //
    {0xAE09, 0xFF}, //
    {0xAE0A, 0xFF}, //
    {0xAE12, 0x58}, //
    {0xAE13, 0x58}, //
    {0xAE15, 0x10}, //
    {0xAE16, 0x10}, //
    {0xAF05, 0x48}, //
    {0xB069, 0x02}, //
    {0xEA4B, 0x00}, //
    {0xEA4C, 0x00}, //
    {0xEA4D, 0x00}, //
    {0xEA4E, 0x00}, //
};

static const SENSOR_REG_T imx616_video_setting[] = {
    /*	4Lane
            reg_Unisoc-C-3
            (16:9) 2x2 Adjacent Pixel Binning w/o PDAF 1080P, 120fps
            H: 1920
            V: 1080
            MIPI output setting
            Address value*/
    {0x0112, 0x0A}, //
    {0x0113, 0x0A}, //
    {0x0114, 0x03}, // Line Length PCK Setting
    {0x0342, 0x0E}, //
    {0x0343, 0xB8}, // Frame Length Lines Setting
    {0x0340, 0x04}, //
    {0x0341, 0x8E}, // ROI Setting
    {0x0344, 0x00}, //
    {0x0345, 0x00}, //
    {0x0346, 0x05}, //
    {0x0347, 0x68}, //
    {0x0348, 0x19}, //
    {0x0349, 0x9F}, //
    {0x034A, 0x0D}, //
    {0x034B, 0xD7}, // Mode Setting
    {0x0220, 0x62}, //
    {0x0222, 0x01}, //
    {0x0900, 0x01}, //
    {0x0901, 0x22}, //
    {0x0902, 0x08}, //
    {0x3140, 0x00}, //
    {0x3246, 0x81}, //
    {0x3247, 0x81}, // Digital Crop & Scaling
    {0x0401, 0x00}, //
    {0x0404, 0x00}, //
    {0x0405, 0x10}, //
    {0x0408, 0x02}, //
    {0x0409, 0xA8}, //
    {0x040A, 0x00}, //
    {0x040B, 0x00}, //
    {0x040C, 0x07}, //
    {0x040D, 0x80}, //
    {0x040E, 0x04}, //
    {0x040F, 0x38}, // Output Size Setting
    {0x034C, 0x07}, //
    {0x034D, 0x80}, //
    {0x034E, 0x04}, //
    {0x034F, 0x38}, // Clock Setting
    {0x0301, 0x05}, //
    {0x0303, 0x02}, //
    {0x0305, 0x04}, //
    {0x0306, 0x00}, //
    {0x0307, 0xDC}, //
    {0x030B, 0x02}, //
    {0x030D, 0x04}, //
    {0x030E, 0x01}, //
    {0x030F, 0x15}, //
    {0x0310, 0x01}, // Other Setting
    {0x3620, 0x00}, //
    {0x3621, 0x00}, //
    {0x3C12, 0x56}, //
    {0x3C13, 0x52}, //
    {0x3C14, 0x3E}, //
    {0x3F0C, 0x00}, //
    {0x3F14, 0x01}, //
    {0x3F80, 0x00}, //
    {0x3F81, 0xA0}, //
    {0x3F8C, 0x00}, //
    {0x3F8D, 0x00}, //
    {0x3FFC, 0x00}, //
    {0x3FFD, 0x1E}, //
    {0x3FFE, 0x00}, //
    {0x3FFF, 0xDC}, // Integration Setting
    {0x0202, 0x04}, //
    {0x0203, 0x5E}, //
    {0x0224, 0x01}, //
    {0x0225, 0xF4}, //
    {0x3FE0, 0x01}, //
    {0x3FE1, 0xF4}, // Gain Setting
    {0x0204, 0x00}, //
    {0x0205, 0x70}, //
    {0x0216, 0x00}, //
    {0x0217, 0x70}, //
    {0x0218, 0x01}, //
    {0x0219, 0x00}, //
    {0x020E, 0x01}, //
    {0x020F, 0x00}, //
    {0x0210, 0x01}, //
    {0x0211, 0x00}, //
    {0x0212, 0x01}, //
    {0x0213, 0x00}, //
    {0x0214, 0x01}, //
    {0x0215, 0x00}, //
    {0x3FE2, 0x00}, //
    {0x3FE3, 0x70}, //
    {0x3FE4, 0x01}, //
    {0x3FE5, 0x00}, // PDAF TYPE1 Setting
    {0x3E20, 0x01}, //
    {0x3E37, 0x00}, //

};

static const SENSOR_REG_T imx616_preview_setting[] = {
    /*	4Lane
            reg_Unisoc-B
            (4:3) 2x2 Adjacent Pixel Binning w/ PDAF
            H: 3280
            V: 2464
            MIPI output setting
            Address value*/
    {0x0112, 0x0A}, //
    {0x0113, 0x0A}, //
    {0x0114, 0x03}, // Line Length PCK Setting
    {0x0342, 0x1A}, //
    {0x0343, 0xD4}, // Frame Length Lines Setting
    {0x0340, 0x0A}, //
    {0x0341, 0x02}, // ROI Setting
    {0x0344, 0x00}, //
    {0x0345, 0x00}, //
    {0x0346, 0x00}, //
    {0x0347, 0x00}, //
    {0x0348, 0x19}, //
    {0x0349, 0x9F}, //
    {0x034A, 0x13}, //
    {0x034B, 0x3F}, // Mode Setting
    {0x0220, 0x62}, //
    {0x0222, 0x01}, //
    {0x0900, 0x01}, //
    {0x0901, 0x22}, //
    {0x0902, 0x08}, //
    {0x3140, 0x00}, //
    {0x3246, 0x81}, //
    {0x3247, 0x81}, // Digital Crop & Scaling
    {0x0401, 0x00}, //
    {0x0404, 0x00}, //
    {0x0405, 0x10}, //
    {0x0408, 0x00}, //
    {0x0409, 0x00}, //
    {0x040A, 0x00}, //
    {0x040B, 0x00}, //
    {0x040C, 0x0C}, //
    {0x040D, 0x80}, //
    {0x040E, 0x09}, //
    {0x040F, 0x80}, // Output Size Setting 2432
    {0x034C, 0x0C}, //
    {0x034D, 0xd0},//3280//80}, // 0xc80 3200
    {0x034E, 0x09}, //
    {0x034F, 0xa0},//2464//80}, // Clock Setting
    {0x0301, 0x05}, //
    {0x0303, 0x02}, //
    {0x0305, 0x04}, //
    {0x0306, 0x00}, //
    {0x0307, 0xDC}, //
    {0x030B, 0x02}, //
    {0x030D, 0x04}, //
    {0x030E, 0x01}, //
    {0x030F, 0x15}, //
    {0x0310, 0x01}, // Other Setting
    {0x3620, 0x00}, //
    {0x3621, 0x00}, //
    {0x3C12, 0x14}, //
    {0x3C13, 0x5A}, //
    {0x3C14, 0x34}, //
    {0x3F0C, 0x01}, //
    {0x3F14, 0x00}, //
    {0x3F80, 0x00}, //
    {0x3F81, 0x32}, //
    {0x3F8C, 0x00}, //
    {0x3F8D, 0x00}, //
    {0x3FFC, 0x01}, //
    {0x3FFD, 0xA4}, //
    {0x3FFE, 0x00}, //
    {0x3FFF, 0x03}, // Integration Setting
    {0x0202, 0x09}, //
    {0x0203, 0xD2}, //
    {0x0224, 0x01}, //
    {0x0225, 0xF4}, //
    {0x3FE0, 0x01}, //
    {0x3FE1, 0xF4}, // Gain Setting
    {0x0204, 0x00}, //
    {0x0205, 0x70}, //
    {0x0216, 0x00}, //
    {0x0217, 0x70}, //
    {0x0218, 0x01}, //
    {0x0219, 0x00}, //
    {0x020E, 0x01}, //
    {0x020F, 0x00}, //
    {0x0210, 0x01}, //
    {0x0211, 0x00}, //
    {0x0212, 0x01}, //
    {0x0213, 0x00}, //
    {0x0214, 0x01}, //
    {0x0215, 0x00}, //
    {0x3FE2, 0x00}, //
    {0x3FE3, 0x70}, //
    {0x3FE4, 0x01}, //
    {0x3FE5, 0x00}, // PDAF TYPE1 Setting
    {0x3E20, 0x01}, //
    {0x3E37, 0x01}, //

};

static const SENSOR_REG_T imx616_snapshot_setting[] = {
    /*	4Lane
            reg_Unisoc-A
            (4:3) Full Resolution w/o PDAF, 24fps
            H: 6560
            V: 4928
            MIPI output setting
            {Address	value*/
    {0x0112, 0x0A}, //
    {0x0113, 0x0A}, //
    {0x0114, 0x03}, // Line Length PCK Setting
    {0x0342, 0x1C}, //
    {0x0343, 0x18}, // Frame Length Lines Setting
    {0x0340, 0x13}, //
    {0x0341, 0x8D}, // ROI Setting
    {0x0344, 0x00}, //
    {0x0345, 0x00}, //
    {0x0346, 0x00}, //
    {0x0347, 0x00}, //
    {0x0348, 0x19}, //
    {0x0349, 0x9F}, //
    {0x034A, 0x13}, //
    {0x034B, 0x3F}, // Mode Setting
    {0x0220, 0x62}, //
    {0x0222, 0x01}, //
    {0x0900, 0x00}, //
    {0x0901, 0x11}, //
    {0x0902, 0x0A}, //
    {0x3140, 0x00}, //
    {0x3246, 0x01}, //
    {0x3247, 0x01}, // Digital Crop & Scaling
    {0x0401, 0x00}, //
    {0x0404, 0x00}, //
    {0x0405, 0x10}, //
    {0x0408, 0x00}, //
    {0x0409, 0x00}, //
    {0x040A, 0x00}, //
    {0x040B, 0x00}, //
    {0x040C, 0x19}, //
    {0x040D, 0xA0}, //
    {0x040E, 0x13}, //
    {0x040F, 0x00}, // Output Size Setting
    {0x034C, 0x19}, //
    {0x034D, 0xa0},//80}, //
    {0x034E, 0x13}, //
    {0x034F, 0x40}, // Clock Setting
    {0x0301, 0x05}, //
    {0x0303, 0x02}, //
    {0x0305, 0x04}, //
    {0x0306, 0x01}, //
    {0x0307, 0x68}, //
    {0x030B, 0x01}, //
    {0x030D, 0x0C}, //
    {0x030E, 0x04}, //
    {0x030F, 0x15}, //
    {0x0310, 0x01}, // Other Setting
    {0x3620, 0x01}, //
    {0x3621, 0x00}, // QSC enable
    {0x3C12, 0x62}, //
    {0x3C13, 0x32}, //
    {0x3C14, 0x20}, //
    {0x3F0C, 0x00}, //
    {0x3F14, 0x01}, //
    {0x3F80, 0x00}, //
    {0x3F81, 0x64}, //
    {0x3F8C, 0x00}, //
    {0x3F8D, 0x00}, //
    {0x3FFC, 0x00}, //
    {0x3FFD, 0x14}, //
    {0x3FFE, 0x00}, //
    {0x3FFF, 0xA0}, // Integration Setting
    {0x0202, 0x13}, //
    {0x0203, 0x5D}, //
    {0x0224, 0x01}, //
    {0x0225, 0xF4}, //
    {0x3FE0, 0x01}, //
    {0x3FE1, 0xF4}, // Gain Setting
    {0x0204, 0x00}, //
    {0x0205, 0x70}, //
    {0x0216, 0x00}, //
    {0x0217, 0x70}, //
    {0x0218, 0x01}, //
    {0x0219, 0x00}, //
    {0x020E, 0x01}, //
    {0x020F, 0x00}, //
    {0x0210, 0x01}, //
    {0x0211, 0x00}, //
    {0x0212, 0x01}, //
    {0x0213, 0x00}, //
    {0x0214, 0x01}, //
    {0x0215, 0x00}, //
    {0x3FE2, 0x00}, //
    {0x3FE3, 0x70}, //
    {0x3FE4, 0x01}, //
    {0x3FE5, 0x00}, // PDAF TYPE1 Setting
    {0x3E20, 0x01}, //
    {0x3E37, 0x00}, //
};

static const SENSOR_REG_T imx616_snapshot_setting1[] = {
    /*	provided by shunyu
    4Lane
        reg_Unisoc-A
        (4:3) Full Resolution w/o PDAF, 24fps
        H: 6560
        V: 4928
        MIPI output setting
    {Address	value*/
    {0x0112, 0x0A},
    {0x0113, 0x0A},
    {0x0114, 0x03},
    {0x0342, 0x38},
    {0x0343, 0x30},
    {0x0340, 0x13},
    {0x0341, 0x7D},
    {0x0344, 0x00},
    {0x0345, 0x00},
    {0x0346, 0x00},
    {0x0347, 0x00},
    {0x0348, 0x19},
    {0x0349, 0x9F},
    {0x034A, 0x13},
    {0x034B, 0x3F},
    {0x0220, 0x62},
    {0x0222, 0x01},
    {0x0900, 0x00},
    {0x0901, 0x11},
    {0x0902, 0x0A},
    {0x3140, 0x00},
    {0x3246, 0x01},
    {0x3247, 0x01},
    {0x0401, 0x00},
    {0x0404, 0x00},
    {0x0405, 0x10},
    {0x0408, 0x00},
    {0x0409, 0x00},
    {0x040A, 0x00},
    {0x040B, 0x00},
    {0x040C, 0x19},
    {0x040D, 0xA0},
    {0x040E, 0x13},
    {0x040F, 0x40},//00}, // 0x40
    {0x034C, 0x19},
    {0x034D, 0xA0},//80}, // 0xA0
    {0x034E, 0x13},
    {0x034F, 0x40},
    {0x0301, 0x05},
    {0x0303, 0x02},
    {0x0305, 0x04},
    {0x0306, 0x01},
    {0x0307, 0x1A},
    {0x030B, 0x02},
    {0x030D, 0x04},
    {0x030E, 0x01},
    {0x030F, 0x0A},
    {0x0310, 0x01},
    {0x3620, 0x01},
    {0x3621, 0x00}, // QSC disable
    {0x3C12, 0x62},
    {0x3C13, 0x32},
    {0x3C14, 0x20},
    {0x3F0C, 0x00},
    {0x3F14, 0x01},
    {0x3F80, 0x00},
    {0x3F81, 0x64},
    {0x3F8C, 0x00},
    {0x3F8D, 0x00},
    {0x3FFC, 0x00},
    {0x3FFD, 0x14},
    {0x3FFE, 0x00},
    {0x3FFF, 0x50},
    {0x0202, 0x13},
    {0x0203, 0x4D},
    {0x0224, 0x01},
    {0x0225, 0xF4},
    {0x3FE0, 0x01},
    {0x3FE1, 0xF4},
    {0x0204, 0x00},
    {0x0205, 0x70},
    {0x0216, 0x00},
    {0x0217, 0x70},
    {0x0218, 0x01},
    {0x0219, 0x00},
    {0x020E, 0x01},
    {0x020F, 0x00},
    {0x0210, 0x01},
    {0x0211, 0x00},
    {0x0212, 0x01},
    {0x0213, 0x00},
    {0x0214, 0x01},
    {0x0215, 0x00},
    {0x3FE2, 0x00},
    {0x3FE3, 0x70},
    {0x3FE4, 0x01},
    {0x3FE5, 0x00},
    {0x3E20, 0x01},
    {0x3E37, 0x00},
};

static struct sensor_res_tab_info s_imx616_resolution_tab_raw[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .reg_tab =
         {{ADDR_AND_LEN_OF_ARRAY(imx616_init_setting), PNULL, 0, .width = 0,
           .height = 0, .xclk_to_sensor = EX_MCLK,
           .image_format = SENSOR_IMAGE_FORMAT_RAW},

          {ADDR_AND_LEN_OF_ARRAY(imx616_video_setting), PNULL, 0,
           .width = VIDEO_WIDTH, .height = VIDEO_HEIGHT,
           .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW},

          {ADDR_AND_LEN_OF_ARRAY(imx616_preview_setting), PNULL, 0,
           .width = PREVIEW_WIDTH, .height = PREVIEW_HEIGHT,
           .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW},

          {ADDR_AND_LEN_OF_ARRAY(imx616_snapshot_setting), PNULL, 0,
           .width = SNAPSHOT_WIDTH, .height = SNAPSHOT_HEIGHT,
           .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW}}}

    /*If there are multiple modules,please add here*/
};

static SENSOR_TRIM_T s_imx616_resolution_trim_tab[VENDOR_NUM] = {
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

static SENSOR_REG_T imx616_shutter_reg[] = {
    {0x0202, 0x0000}, {0x0203, 0x0000},
};

static struct sensor_i2c_reg_tab imx616_shutter_tab = {
    .settings = imx616_shutter_reg, .size = ARRAY_SIZE(imx616_shutter_reg),
};

static SENSOR_REG_T imx616_again_reg[] = {
    {0x0104, 0x0001}, {0x0204, 0x0000}, {0x0205, 0x0000}, {0x0104, 0x0000},
};

static struct sensor_i2c_reg_tab imx616_again_tab = {
    .settings = imx616_again_reg, .size = ARRAY_SIZE(imx616_again_reg),
};

static SENSOR_REG_T imx616_dgain_reg[] = {

};

static struct sensor_i2c_reg_tab imx616_dgain_tab = {
    .settings = imx616_dgain_reg, .size = ARRAY_SIZE(imx616_dgain_reg),
};

static SENSOR_REG_T imx616_frame_length_reg[] = {
    {0x0340, 0x0000}, {0x0341, 0x0000},
};

static struct sensor_i2c_reg_tab imx616_frame_length_tab = {
    .settings = imx616_frame_length_reg,
    .size = ARRAY_SIZE(imx616_frame_length_reg),
};

static struct sensor_aec_i2c_tag imx616_aec_info = {
    .slave_addr = (I2C_SLAVE_ADDR >> 1),
    .addr_bits_type = SENSOR_I2C_REG_16BIT,
    .data_bits_type = SENSOR_I2C_VAL_8BIT,
    .shutter = &imx616_shutter_tab,
    .again = &imx616_again_tab,
    .dgain = &imx616_dgain_tab,
    .frame_length = &imx616_frame_length_tab,
};

static const cmr_u16 imx616_pd_is_right[] = {
    0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1,
    0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1,
};

static const cmr_u16 imx616_pd_row[] = {
    7,  7,  23, 23, 43, 43, 59, 59, 11, 11, 27, 27, 39, 39, 55, 55,
    11, 11, 27, 27, 39, 39, 55, 55, 7,  7,  23, 23, 43, 43, 59, 59};

static const cmr_u16 imx616_pd_col[] = {
    0,  4,  4,  8,  4,  8,  0,  4,  20, 16, 24, 20, 24, 20, 20, 16,
    36, 40, 32, 36, 32, 36, 36, 40, 56, 52, 52, 48, 52, 48, 56, 52};

static SENSOR_STATIC_INFO_T s_imx616_static_info[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .static_info = {.f_num = 200,
                     .focal_length = 354,
                     .max_fps = 30,
                     .max_adgain = 8,
                     .ois_supported = 0,
                     .pdaf_supported = 0,
                     .exp_valid_frame_num = 1,
                     .clamp_level = 64,
                     .adgain_valid_frame_num = 0,
                     .fov_info = {{4.614f, 3.444f}, 4.222f}}}
    /*If there are multiple modules,please add here*/
};

static SENSOR_MODE_FPS_INFO_T s_imx616_mode_fps_info[VENDOR_NUM] = {
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

static struct sensor_module_info s_imx616_module_info_tab[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .module_info = {.major_i2c_addr = 0x34 >> 1,
                     .minor_i2c_addr = 0x20 >> 1,

                     .reg_addr_value_bits = SENSOR_I2C_REG_16BIT |
                                            SENSOR_I2C_VAL_8BIT |
                                            SENSOR_I2C_FREQ_400,

                     .avdd_val = SENSOR_AVDD_2800MV,
                     .iovdd_val = SENSOR_AVDD_1800MV,
                     .dvdd_val = SENSOR_AVDD_1000MV,

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

static struct sensor_ic_ops s_imx616_ops_tab;
struct sensor_raw_info *s_imx616_mipi_raw_info_ptr =
    PNULL; //&s_imx616_mipi_raw_info;

/*==============================================================================
 * Description:
 * sensor all info
 * please modify this variable acording your spec
 *============================================================================*/
SENSOR_INFO_T g_imx616_mipi_raw_info = {
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
    .identify_code = {{.reg_addr = imx616_PID_ADDR,
                       .reg_value = imx616_PID_VALUE},
                      {.reg_addr = imx616_VER_ADDR,
                       .reg_value = imx616_VER_VALUE}},

    .source_width_max = SNAPSHOT_WIDTH,
    .source_height_max = SNAPSHOT_HEIGHT,
    .name = (cmr_s8 *)SENSOR_NAME,
    .image_format = SENSOR_IMAGE_FORMAT_RAW,

    .module_info_tab = s_imx616_module_info_tab,
    .module_info_tab_size = ARRAY_SIZE(s_imx616_module_info_tab),

    .resolution_tab_info_ptr = s_imx616_resolution_tab_raw,
    .sns_ops = &s_imx616_ops_tab,
    .raw_info_ptr = &s_imx616_mipi_raw_info_ptr,

    .video_tab_info_ptr = NULL,
    .sensor_version_info = (cmr_s8 *)"imx616_v1",
};

#endif
