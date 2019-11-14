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
#ifndef _SENSOR_hi846_wide_MIPI_RAW_H_
#define _SENSOR_hi846_wide_MIPI_RAW_H_

#include <utils/Log.h>
#include "sensor.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"

#define VENDOR_NUM 1
#define SENSOR_NAME "hi846_wide_mipi_raw"

#define I2C_SLAVE_ADDR 0x40 /* 8bit slave address*/

#define hi846_wide_PID_ADDR 0x0F17
#define hi846_wide_PID_VALUE 0x08
#define hi846_wide_VER_ADDR 0x0F16
#define hi846_wide_VER_VALUE 0x46

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
#define VIDEO_TRIM_W 1280
#define VIDEO_TRIM_H 720
#define PREVIEW_TRIM_X 0
#define PREVIEW_TRIM_Y 0
#define PREVIEW_TRIM_W 1632
#define PREVIEW_TRIM_H 1224
#define SNAPSHOT_TRIM_X 0
#define SNAPSHOT_TRIM_Y 0
#define SNAPSHOT_TRIM_W 3264
#define SNAPSHOT_TRIM_H 2448

/*Mipi output*/
#define LANE_NUM 2
#define RAW_BITS 10

#define VIDEO_MIPI_PER_LANE_BPS 720     /* 2*Mipi clk */
#define PREVIEW_MIPI_PER_LANE_BPS 720   /* 2*Mipi clk */
#define SNAPSHOT_MIPI_PER_LANE_BPS 1440 /* 2*Mipi clk */

/*line time unit: 1ns*/
#define VIDEO_LINE_TIME 13194
#define PREVIEW_LINE_TIME 13194
#define SNAPSHOT_LINE_TIME 13194

/* frame length*/
#define VIDEO_FRAME_LENGTH 842
#define PREVIEW_FRAME_LENGTH 2526
#define SNAPSHOT_FRAME_LENGTH 2526

/* please ref your spec */
#define FRAME_OFFSET 6
#define SENSOR_MAX_GAIN 0x100
#define SENSOR_BASE_GAIN 0x10
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

static const SENSOR_REG_T hi846_wide_init_setting[] = {
    // Sensor Information////////////////////////////
    // Sensor	  : Hi-846
    // Date		  : 2017-01-25
    // Customer        : SPRD_validation
    // MCLK	          : 24MHz
    // MIPI            : 2 Lane
    // Pixel order 	  : B 1st
    // BLC offset	  : 64code
    // Firmware Ver.   : v15
    ////////////////////////////////////////////////
    {0x0A00, 0x0000}, {0x2000, 0x100A},
    {0x2002, 0x00FF}, {0x2004, 0x0007},
    {0x2006, 0x3FFF}, {0x2008, 0x3FFF},
    {0x200A, 0xC216}, {0x200C, 0x1292},
    {0x200E, 0xC01A}, {0x2010, 0x403D},
    {0x2012, 0x000E}, {0x2014, 0x403E},
    {0x2016, 0x0B80}, {0x2018, 0x403F},
    {0x201A, 0x82AE}, {0x201C, 0x1292},
    {0x201E, 0xC00C}, {0x2020, 0x4130},
    {0x2022, 0x43E2}, {0x2024, 0x0180},
    {0x2026, 0x4130}, {0x2028, 0x7400},
    {0x202A, 0x5000}, {0x202C, 0x0253},
    {0x202E, 0x0AD1}, {0x2030, 0x2360},
    {0x2032, 0x0009}, {0x2034, 0x5020},
    {0x2036, 0x000B}, {0x2038, 0x0002},
    {0x203A, 0x0044}, {0x203C, 0x0016},
    {0x203E, 0x1792}, {0x2040, 0x7002},
    {0x2042, 0x154F}, {0x2044, 0x00D5},
    {0x2046, 0x000B}, {0x2048, 0x0019},
    {0x204A, 0x1698}, {0x204C, 0x000E},
    {0x204E, 0x099A}, {0x2050, 0x0058},
    {0x2052, 0x7000}, {0x2054, 0x1799},
    {0x2056, 0x0310}, {0x2058, 0x03C3},
    {0x205A, 0x004C}, {0x205C, 0x064A},
    {0x205E, 0x0001}, {0x2060, 0x0007},
    {0x2062, 0x0BC7}, {0x2064, 0x0055},
    {0x2066, 0x7000}, {0x2068, 0x1550},
    {0x206A, 0x158A}, {0x206C, 0x0004},
    {0x206E, 0x1488}, {0x2070, 0x7010},
    {0x2072, 0x1508}, {0x2074, 0x0004},
    {0x2076, 0x0016}, {0x2078, 0x03D5},
    {0x207A, 0x0055}, {0x207C, 0x08CA},
    {0x207E, 0x2019}, {0x2080, 0x0007},
    {0x2082, 0x7057}, {0x2084, 0x0FC7},
    {0x2086, 0x5041}, {0x2088, 0x12C8},
    {0x208A, 0x5060}, {0x208C, 0x5080},
    {0x208E, 0x2084}, {0x2090, 0x12C8},
    {0x2092, 0x7800}, {0x2094, 0x0802},
    {0x2096, 0x040F}, {0x2098, 0x1007},
    {0x209A, 0x0803}, {0x209C, 0x080B},
    {0x209E, 0x3803}, {0x20A0, 0x0807},
    {0x20A2, 0x0404}, {0x20A4, 0x0400},
    {0x20A6, 0xFFFF}, {0x20A8, 0xF0B2},
    {0x20AA, 0xFFEF}, {0x20AC, 0x0A84},
    {0x20AE, 0x1292}, {0x20B0, 0xC02E},
    {0x20B2, 0x4130}, {0x23FE, 0xC056},
    {0x3232, 0xFC0C}, {0x3236, 0xFC22},
    {0x3248, 0xFCA8}, {0x326A, 0x8302},
    {0x326C, 0x830A}, {0x326E, 0x0000},
    {0x32CA, 0xFC28}, {0x32CC, 0xC3BC},
    {0x32CE, 0xC34C}, {0x32D0, 0xC35A},
    {0x32D2, 0xC368}, {0x32D4, 0xC376},
    {0x32D6, 0xC3C2}, {0x32D8, 0xC3E6},
    {0x32DA, 0x0003}, {0x32DC, 0x0003},
    {0x32DE, 0x00C7}, {0x32E0, 0x0031},
    {0x32E2, 0x0031}, {0x32E4, 0x0031},
    {0x32E6, 0xFC28}, {0x32E8, 0xC3BC},
    {0x32EA, 0xC384}, {0x32EC, 0xC392},
    {0x32EE, 0xC3A0}, {0x32F0, 0xC3AE},
    {0x32F2, 0xC3C4}, {0x32F4, 0xC3E6},
    {0x32F6, 0x0003}, {0x32F8, 0x0003},
    {0x32FA, 0x00C7}, {0x32FC, 0x0031},
    {0x32FE, 0x0031}, {0x3300, 0x0031},
    {0x3302, 0x82CA}, {0x3304, 0xC164},
    {0x3306, 0x82E6}, {0x3308, 0xC19C},
    {0x330A, 0x001F}, {0x330C, 0x001A},
    {0x330E, 0x0034}, {0x3310, 0x0000},
    {0x3312, 0x0000}, {0x3314, 0xFC94},
    {0x3316, 0xC3D8}, {0x0A00, 0x0000},
    {0x0E04, 0x0012}, {0x002E, 0x1111},
    {0x0032, 0x1111}, {0x0022, 0x0008},
    {0x0026, 0x0040}, {0x0028, 0x0017},
    {0x002C, 0x09CF}, {0x005C, 0x2101},
    {0x0006, 0x09DE}, // frame_length_lines_h
    {0x0008, 0x0ED8}, // line_length_pck_h
    {0x000E, 0x0100}, // image_orient
    {0x000C, 0x0022}, {0x0A22, 0x0000},
    {0x0A24, 0x0000}, {0x0804, 0x0000},
    {0x0A12, 0x0CC0}, // x_output_size_h
    {0x0A14, 0x0990}, // y_output_size_h
    {0x0074, 0x09D8}, // Coarse integration time
    {0x0076, 0x0000}, // analog_gain_code_global_h
    {0x0046, 0x0002}, // group para off/per frame control off
    {0x051E, 0x0000}, {0x0200, 0x0400},
    {0x0A1A, 0x0C00}, // Pedestal enable
    {0x0A0C, 0x0010}, {0x0A1E, 0x0CCF},
    {0x0402, 0x0110}, {0x0404, 0x00F4},
    {0x0408, 0x0000}, {0x0410, 0x008D},
    {0x0412, 0x011A}, {0x0414, 0x864C},
#ifdef FEATURE_OTP    // enable lsc otp
    {0x021C, 0x0003}, // otp LSC_en
#else
    {0x021C, 0x0001}, // otp LSC_disable
#endif
    {0x0C00, 0x9150}, {0x0C06, 0x0021},
    {0x0C10, 0x0040}, // act_rr_offset_h
    {0x0C12, 0x0040}, // act_gr_offset_h
    {0x0C14, 0x0040}, // act_gb_offset_h
    {0x0C16, 0x0040}, // act_bb_offset_h
    {0x0A02, 0x0100},
#ifdef FEATURE_OTP    // enable lsc otp
    {0x0A04, 0x015A}, // Lens shading correction enable
#else
    {0x0A04, 0x014A}, // Lens shading correction disable
#endif
    {0x0418, 0x0000}, {0x012A, 0x03B4},
    {0x0120, 0x0046}, {0x0122, 0x0376},
    {0x0B02, 0xE04D}, {0x0B10, 0x6821},
    {0x0B12, 0x0120}, {0x0B14, 0x0001},
    {0x2008, 0x38FD}, {0x326E, 0x0000},
    {0x0900, 0x0320}, {0x0902, 0xC31A}, // tx_op_mode1/2
    {0x0914, 0xC109},                   // mipi_exit_seq, tlpx
    {0x0916, 0x061A},                   // r_mipi_value_clk_prepare
    {0x0918, 0x0306},                   // r_mipi_value_clk_pre
    {0x091A, 0x0B09},                   // r_mipi_value_data_zero
    {0x091C, 0x0C07},                   // r_mipi_value_clk_post
    {0x091E, 0x0A00},                   // r_mipi_value_exit
    {0x090C, 0x042A},                   // r_mipi_vblank_delay_h
    {0x090E, 0x006B},                   // r_mipi_hblank_short_delay_h
    {0x0954, 0x0089}, {0x0956, 0x0000},
    {0x0958, 0xCA00}, {0x095A, 0x9240},
    {0x0F08, 0x2F04}, {0x0F30, 0x001F},
    {0x0F36, 0x001F}, {0x0F04, 0x3A00},
    {0x0F32, 0x025A}, {0x0F38, 0x025A},
    {0x0F2A, 0x0024}, {0x006A, 0x0100},
    {0x004C, 0x0100},
};

static const SENSOR_REG_T hi846_wide_video_setting[] = {
    // Sensor Information////////////////////////////
    // Sensor	  : Hi-846
    // Date		  : 2017-01-25
    // Customer        : SPRD_validation
    // Image size	  : 1280x720
    // MCLK/PCLK	  : 24MHz /288Mhz
    // MIPI speed(Mbps): 720Mbps x 2Lane
    // Frame Length	  : 842
    // Line Length 	  : 3800
    // line Time       :13194
    // Max Fps 	  : 90.00fps
    // Firmware Ver.   : v15
    ////////////////////////////////////////////////
    {0x0A00, 0x0000},
    {0x002E, 0x3311},
    {0x0032, 0x3311},
    {0x0026, 0x0238},
    {0x002C, 0x07D7},
    {0x005C, 0x4202},
    {0x0006, 0x034A},
    {0x0008, 0x0ED8},
    {0x000C, 0x0122},
    {0x0A22, 0x0100},
    {0x0A24, 0x0000},
    {0x0804, 0x00B0},
    {0x0A12, 0x0500},
    {0x0A14, 0x02D0},
#ifdef FEATURE_OTP    // enable lsc otp
    {0x0A04, 0x017A}, // Lens shading correction enable
#else
    {0x0A04, 0x016A}, // Lens shading correction disable
#endif
    {0x0418, 0x0410},
    {0x0B02, 0xE04D},
    {0x0B10, 0x6C21},
    {0x0B12, 0x0120},
    {0x0B14, 0x0005},
    {0x2008, 0x38FD},
    {0x326E, 0x0000},
    //=============================================//
    //      MIPI 2lane 720Mbps
    //=============================================//
    {0x0900, 0x0300},
    {0x0902, 0x4319},
    {0x0914, 0xC109},
    {0x0916, 0x061A},
    {0x0918, 0x0407},
    {0x091A, 0x0A0B},
    {0x091C, 0x0E08},
    {0x091E, 0x0A00},
    {0x090C, 0x0427},
    {0x090E, 0x0145},
    {0x0954, 0x0089},
    {0x0956, 0x0000},
    {0x0958, 0xCA80},
    {0x095A, 0x9240},
    {0x0F2A, 0x4124},
    {0x004C, 0x0100},
};

static const SENSOR_REG_T hi846_wide_preview_setting[] = {
    // Sensor Information////////////////////////////
    // Sensor	  : Hi-846
    // Date		  : 2017-01-25
    // Customer        : SPRD_validation
    // Image size	  : 1632x1224(BIN2)
    // MCLK/PCLK	  : 24MHz /288Mhz
    // MIPI speed(Mbps): 720Mbps x 2Lane
    // Frame Length	  : 2492
    // Line Length 	  : 3800
    // line Time       :13194
    // Max Fps 	  : 30.00fps
    // Firmware Ver.   : v15
    ////////////////////////////////////////////////
    {0x0A00, 0x0000},
    {0x002E, 0x3311},
    {0x0032, 0x3311},
    {0x0026, 0x0040},
    {0x002C, 0x09CF},
    {0x005C, 0x4202},
    {0x0006, 0x09DE},
    {0x0008, 0x0ED8},
    {0x000C, 0x0122},
    {0x0A22, 0x0100},
    {0x0A24, 0x0000},
    {0x0804, 0x0000},
    {0x0A12, 0x0660},
    {0x0A14, 0x04C8},
#ifdef FEATURE_OTP    // enable lsc otp
    {0x0A04, 0x017A}, // Lens shading correction enable
#else
    {0x0A04, 0x016A}, // Lens shading correction disable
#endif
    {0x0418, 0x0000},
    {0x0B02, 0xE04D},
    {0x0B10, 0x6C21},
    {0x0B12, 0x0120},
    {0x0B14, 0x0005},
    {0x2008, 0x38FD},
    {0x326E, 0x0000},
    //=============================================//
    //      MIPI 2lane 720Mbps
    //=============================================//
    {0x0900, 0x0300},
    {0x0902, 0x4319},
    {0x0914, 0xC109},
    {0x0916, 0x061A},
    {0x0918, 0x0407},
    {0x091A, 0x0A0B},
    {0x091C, 0x0E08},
    {0x091E, 0x0A00},
    {0x090C, 0x0427},
    {0x090E, 0x0069},
    {0x0954, 0x0089},
    {0x0956, 0x0000},
    {0x0958, 0xCA80},
    {0x095A, 0x9240},
    {0x0F2A, 0x4124},
    {0x004C, 0x0100},
};

static const SENSOR_REG_T hi846_wide_snapshot_setting[] = {
    // Sensor Information////////////////////////////
    // Sensor	  : Hi-846
    // Date		  : 2017-01-25
    // Customer        : SPRD_validation
    // Image size	  : 3264x2448
    // MCLK/PCLK	  : 24MHz /288Mhz
    // MIPI speed(Mbps): 1440Mbps x 2Lane
    // Frame Length	  : 2492
    // Line Length 	  : 3800
    // line Time       :13194
    // Max Fps 	  : 30.00fps
    // Firmware Ver.   : v15
    ////////////////////////////////////////////////
    {0x0A00, 0x0000},
    {0x002E, 0x1111},
    {0x0032, 0x1111},
    {0x0026, 0x0040},
    {0x002C, 0x09CF},
    {0x005C, 0x2101},
    {0x0006, 0x09DE},
    {0x0008, 0x0ED8},
    {0x000C, 0x0022},
    {0x0A22, 0x0000},
    {0x0A24, 0x0000},
    {0x0804, 0x0000},
    {0x0A12, 0x0CC0},
    {0x0A14, 0x0990},
#ifdef FEATURE_OTP    // enable lsc otp
    {0x0A04, 0x015A}, // Lens shading correction enable
#else
    {0x0A04, 0x014A}, // Lens shading correction disable
#endif
    {0x0418, 0x0000},
    {0x0B02, 0xE04D},
    {0x0B10, 0x6821},
    {0x0B12, 0x0120},
    {0x0B14, 0x0001},
    {0x2008, 0x38FD},
    {0x326E, 0x0000},
    //=============================================//
    //      MIPI 2lane 1440Mbps
    //=============================================//
    {0x0900, 0x0320},
    {0x0902, 0xC31A},
    {0x0914, 0xC109},
    {0x0916, 0x061A},
    {0x0918, 0x0306},
    {0x091A, 0x0B09},
    {0x091C, 0x0C07},
    {0x091E, 0x0A00},
    {0x090C, 0x042A},
    {0x090E, 0x006B},
    {0x0954, 0x0089},
    {0x0956, 0x0000},
    {0x0958, 0xCA00},
    {0x095A, 0x9240},
    {0x0F2A, 0x0024},
    {0x004C, 0x0100},
};

static struct sensor_res_tab_info s_hi846_wide_resolution_tab_raw[VENDOR_NUM] =
    {
        {.module_id = MODULE_SPW_NONE_BACK,
         .reg_tab = {{ADDR_AND_LEN_OF_ARRAY(hi846_wide_init_setting), PNULL, 0,
                      .width = 0, .height = 0, .xclk_to_sensor = EX_MCLK,
                      .image_format = SENSOR_IMAGE_FORMAT_RAW},

                     {ADDR_AND_LEN_OF_ARRAY(hi846_wide_video_setting), PNULL, 0,
                      .width = VIDEO_WIDTH, .height = VIDEO_HEIGHT,
                      .xclk_to_sensor = EX_MCLK,
                      .image_format = SENSOR_IMAGE_FORMAT_RAW},

                     {ADDR_AND_LEN_OF_ARRAY(hi846_wide_preview_setting), PNULL,
                      0, .width = PREVIEW_WIDTH, .height = PREVIEW_HEIGHT,
                      .xclk_to_sensor = EX_MCLK,
                      .image_format = SENSOR_IMAGE_FORMAT_RAW},

                     {ADDR_AND_LEN_OF_ARRAY(hi846_wide_snapshot_setting), PNULL,
                      0, .width = SNAPSHOT_WIDTH, .height = SNAPSHOT_HEIGHT,
                      .xclk_to_sensor = EX_MCLK,
                      .image_format = SENSOR_IMAGE_FORMAT_RAW}}}

        /*If there are multiple modules,please add here*/
};

static SENSOR_TRIM_T s_hi846_wide_resolution_trim_tab[VENDOR_NUM] = {
    {.module_id = MODULE_SPW_NONE_BACK,
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

static SENSOR_REG_T hi846_wide_shutter_reg[] = {
    {0x0074, 0x0000},
};

static struct sensor_i2c_reg_tab hi846_wide_shutter_tab = {
    .settings = hi846_wide_shutter_reg,
    .size = ARRAY_SIZE(hi846_wide_shutter_reg),
};

static SENSOR_REG_T hi846_wide_again_reg[] = {
    {0x0076, 0x0000},
};

static struct sensor_i2c_reg_tab hi846_wide_again_tab = {
    .settings = hi846_wide_again_reg, .size = ARRAY_SIZE(hi846_wide_again_reg),
};

static SENSOR_REG_T hi846_wide_dgain_reg[] = {

};

static struct sensor_i2c_reg_tab hi846_wide_dgain_tab = {
    .settings = hi846_wide_dgain_reg, .size = ARRAY_SIZE(hi846_wide_dgain_reg),
};

static SENSOR_REG_T hi846_wide_frame_length_reg[] = {
    {0x0006, 0x0000},
};

static struct sensor_i2c_reg_tab hi846_wide_frame_length_tab = {
    .settings = hi846_wide_frame_length_reg,
    .size = ARRAY_SIZE(hi846_wide_frame_length_reg),
};

static struct sensor_aec_i2c_tag hi846_wide_aec_info = {
    .slave_addr = (I2C_SLAVE_ADDR >> 1),
    .addr_bits_type = SENSOR_I2C_REG_16BIT,
    .data_bits_type = SENSOR_I2C_VAL_16BIT,
    .shutter = &hi846_wide_shutter_tab,
    .again = &hi846_wide_again_tab,
    .dgain = &hi846_wide_dgain_tab,
    .frame_length = &hi846_wide_frame_length_tab,
};

static SENSOR_STATIC_INFO_T s_hi846_wide_static_info[VENDOR_NUM] = {
    {.module_id = MODULE_SPW_NONE_BACK,
     .static_info = {.f_num = 200,
                     .focal_length = 354,
                     .max_fps = 30,
                     .max_adgain = 15 * 2,
                     .ois_supported = 0,
                     .pdaf_supported = 0,
                     .exp_valid_frame_num = 1,
                     .clamp_level = 64,
                     .adgain_valid_frame_num = 1,
                     .fov_info = {{4.614f, 3.444f}, 4.222f}}}
    /*If there are multiple modules,please add here*/
};

static SENSOR_MODE_FPS_INFO_T s_hi846_wide_mode_fps_info[VENDOR_NUM] = {
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

static struct sensor_module_info s_hi846_wide_module_info_tab[VENDOR_NUM] = {
    {.module_id = MODULE_SPW_NONE_BACK,
     .module_info = {.major_i2c_addr = I2C_SLAVE_ADDR >> 1,
                     .minor_i2c_addr = I2C_SLAVE_ADDR >> 1,

                     .reg_addr_value_bits = SENSOR_I2C_REG_16BIT |
                                            SENSOR_I2C_VAL_16BIT |
                                            SENSOR_I2C_FREQ_400,

                     .avdd_val = SENSOR_AVDD_2800MV,
                     .iovdd_val = SENSOR_AVDD_1800MV,
                     .dvdd_val = SENSOR_AVDD_1200MV,

                     .image_pattern = SENSOR_IMAGE_PATTERN_RAWRGB_GB,

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

static struct sensor_ic_ops s_hi846_wide_ops_tab;
struct sensor_raw_info *s_hi846_wide_mipi_raw_info_ptr = PNULL;

SENSOR_INFO_T g_hi846_wide_mipi_raw_info = {
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
    .identify_code = {{.reg_addr = hi846_wide_PID_ADDR,
                       .reg_value = hi846_wide_PID_VALUE},
                      {.reg_addr = hi846_wide_VER_ADDR,
                       .reg_value = hi846_wide_VER_VALUE}},

    .source_width_max = SNAPSHOT_WIDTH,
    .source_height_max = SNAPSHOT_HEIGHT,
    .name = (cmr_s8 *)SENSOR_NAME,
    .image_format = SENSOR_IMAGE_FORMAT_RAW,

    .module_info_tab = s_hi846_wide_module_info_tab,
    .module_info_tab_size = ARRAY_SIZE(s_hi846_wide_module_info_tab),

    .resolution_tab_info_ptr = s_hi846_wide_resolution_tab_raw,
    .sns_ops = &s_hi846_wide_ops_tab,
    .raw_info_ptr = &s_hi846_wide_mipi_raw_info_ptr,

    .video_tab_info_ptr = NULL,
    .sensor_version_info = (cmr_s8 *)"hi846_wide_v1",
};

#endif
