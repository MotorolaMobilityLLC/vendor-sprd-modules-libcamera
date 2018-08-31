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

#ifndef _SENSOR_HI556_MIPI_RAW_H_
#define _SENSOR_HI556_MIPI_RAW_H_

#include <utils/Log.h>
#include "sensor.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"

#include "parameters/sensor_hi556_raw_param_main.c"

//#define FEATURE_OTP

#define VENDOR_NUM 1
#define SENSOR_NAME "hi556_mipi_raw"

#define I2C_SLAVE_ADDR 0x40 /* 8bit slave address*/

#define hi556_PID_ADDR 0x0F16
#define hi556_PID_VALUE 0x05
#define hi556_VER_ADDR 0x0F17
#define hi556_VER_VALUE 0x56

/* sensor parameters begin */

/* effective sensor output image size */
#define VIDEO_WIDTH 1280
#define VIDEO_HEIGHT 720
#define PREVIEW_WIDTH 1296
#define PREVIEW_HEIGHT 972
#define SNAPSHOT_WIDTH 2592
#define SNAPSHOT_HEIGHT 1944

/*Raw Trim parameters*/
#define VIDEO_TRIM_X 0
#define VIDEO_TRIM_Y 0
#define VIDEO_TRIM_W 1280
#define VIDEO_TRIM_H 720
#define PREVIEW_TRIM_X 0
#define PREVIEW_TRIM_Y 0
#define PREVIEW_TRIM_W 1296
#define PREVIEW_TRIM_H 972
#define SNAPSHOT_TRIM_X 0
#define SNAPSHOT_TRIM_Y 0
#define SNAPSHOT_TRIM_W 2592
#define SNAPSHOT_TRIM_H 1944

/*Mipi output*/
#define LANE_NUM 2
#define RAW_BITS 10

#define VIDEO_MIPI_PER_LANE_BPS 440    /* 2*Mipi clk */
#define PREVIEW_MIPI_PER_LANE_BPS 440  /* 2*Mipi clk */
#define SNAPSHOT_MIPI_PER_LANE_BPS 880 /* 2*Mipi clk */

/*line time unit: 1ns*/
#define VIDEO_LINE_TIME 16000
#define PREVIEW_LINE_TIME 16000
#define SNAPSHOT_LINE_TIME 16000

/* frame length*/
#define VIDEO_FRAME_LENGTH 1041
#define PREVIEW_FRAME_LENGTH 2083
#define SNAPSHOT_FRAME_LENGTH 2083

/* please ref your spec */
#define FRAME_OFFSET 6
#define SENSOR_MAX_GAIN 0x0100
#define SENSOR_BASE_GAIN 0x0010
#define SENSOR_MIN_SHUTTER 2

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

static const SENSOR_REG_T hi556_init_setting[] = {
    // Sensor Information////////////////////////////
    // Sensor	  : hi556
    // Date		  : 2017-02-20
    // Customer        : SPRD_validation
    // Image size	  : 2592x1944
    // MCLK/PCLK	  : 24MHz /176Mhz
    // MIPI speed(Mbps): 880Mbps x 2Lane
    // Frame Length	  : 2083
    // Line Length 	  : 2816
    // Max Fps 	  : 30.00fps
    // BLC offset	    : 64code
    // Firmware Ver.   : v1.0
    ////////////////////////////////////////////////
    {0x0a00, 0x0000}, // stream off
    {0x0e00, 0x0102}, {0x0e02, 0x0102}, {0x0e0c, 0x0100}, {0x2000, 0x7400},
    {0x2002, 0x001c}, {0x2004, 0x0242}, {0x2006, 0x0942}, {0x2008, 0x7007},
    {0x200a, 0x0fd9}, {0x200c, 0x0259}, {0x200e, 0x7008}, {0x2010, 0x160e},
    {0x2012, 0x0047}, {0x2014, 0x2118}, {0x2016, 0x0041}, {0x2018, 0x00d8},
    {0x201a, 0x0145}, {0x201c, 0x0006}, {0x201e, 0x0181}, {0x2020, 0x13cc},
    {0x2022, 0x2057}, {0x2024, 0x7001}, {0x2026, 0x0fca}, {0x2028, 0x00cb},
    {0x202a, 0x009f}, {0x202c, 0x7002}, {0x202e, 0x13cc}, {0x2030, 0x019b},
    {0x2032, 0x014d}, {0x2034, 0x2987}, {0x2036, 0x2766}, {0x2038, 0x0020},
    {0x203a, 0x2060}, {0x203c, 0x0e5d}, {0x203e, 0x181d}, {0x2040, 0x2066},
    {0x2042, 0x20c4}, {0x2044, 0x5000}, {0x2046, 0x0005}, {0x2048, 0x0000},
    {0x204a, 0x01db}, {0x204c, 0x025a}, {0x204e, 0x00c0}, {0x2050, 0x0005},
    {0x2052, 0x0006}, {0x2054, 0x0ad9}, {0x2056, 0x0259}, {0x2058, 0x0618},
    {0x205a, 0x0258}, {0x205c, 0x2266}, {0x205e, 0x20c8}, {0x2060, 0x2060},
    {0x2062, 0x707b}, {0x2064, 0x0fdd}, {0x2066, 0x81b8}, {0x2068, 0x5040},
    {0x206a, 0x0020}, {0x206c, 0x5060}, {0x206e, 0x3143}, {0x2070, 0x5081},
    {0x2072, 0x025c}, {0x2074, 0x7800}, {0x2076, 0x7400}, {0x2078, 0x001c},
    {0x207a, 0x0242}, {0x207c, 0x0942}, {0x207e, 0x0bd9}, {0x2080, 0x0259},
    {0x2082, 0x7008}, {0x2084, 0x160e}, {0x2086, 0x0047}, {0x2088, 0x2118},
    {0x208a, 0x0041}, {0x208c, 0x00d8}, {0x208e, 0x0145}, {0x2090, 0x0006},
    {0x2092, 0x0181}, {0x2094, 0x13cc}, {0x2096, 0x2057}, {0x2098, 0x7001},
    {0x209a, 0x0fca}, {0x209c, 0x00cb}, {0x209e, 0x009f}, {0x20a0, 0x7002},
    {0x20a2, 0x13cc}, {0x20a4, 0x019b}, {0x20a6, 0x014d}, {0x20a8, 0x2987},
    {0x20aa, 0x2766}, {0x20ac, 0x0020}, {0x20ae, 0x2060}, {0x20b0, 0x0e5d},
    {0x20b2, 0x181d}, {0x20b4, 0x2066}, {0x20b6, 0x20c4}, {0x20b8, 0x50a0},
    {0x20ba, 0x0005}, {0x20bc, 0x0000}, {0x20be, 0x01db}, {0x20c0, 0x025a},
    {0x20c2, 0x00c0}, {0x20c4, 0x0005}, {0x20c6, 0x0006}, {0x20c8, 0x0ad9},
    {0x20ca, 0x0259}, {0x20cc, 0x0618}, {0x20ce, 0x0258}, {0x20d0, 0x2266},
    {0x20d2, 0x20c8}, {0x20d4, 0x2060}, {0x20d6, 0x707b}, {0x20d8, 0x0fdd},
    {0x20da, 0x86b8}, {0x20dc, 0x50e0}, {0x20de, 0x0020}, {0x20e0, 0x5100},
    {0x20e2, 0x3143}, {0x20e4, 0x5121}, {0x20e6, 0x7800}, {0x20e8, 0x3140},
    {0x20ea, 0x01c4}, {0x20ec, 0x01c1}, {0x20ee, 0x01c0}, {0x20f0, 0x01c4},
    {0x20f2, 0x2700}, {0x20f4, 0x3d40}, {0x20f6, 0x7800}, {0x20f8, 0xffff},
    {0x27fe, 0xe000}, {0x3000, 0x60f8}, {0x3002, 0x187f}, {0x3004, 0x7060},
    {0x3006, 0x0114}, {0x3008, 0x60b0}, {0x300a, 0x1473}, {0x300c, 0x0013},
    {0x300e, 0x140f}, {0x3010, 0x0040}, {0x3012, 0x100f}, {0x3014, 0x60f8},
    {0x3016, 0x187f}, {0x3018, 0x7060}, {0x301a, 0x0114}, {0x301c, 0x60b0},
    {0x301e, 0x1473}, {0x3020, 0x0013}, {0x3022, 0x140f}, {0x3024, 0x0040},
    {0x3026, 0x000f},

    {0x0b00, 0x0000}, {0x0b02, 0x0045}, {0x0b04, 0xb405}, {0x0b06, 0xc403},
    {0x0b08, 0x0081}, {0x0b0a, 0x8252}, {0x0b0c, 0xf814}, {0x0b0e, 0xc618},
    {0x0b10, 0xa828}, {0x0b12, 0x004c}, {0x0b14, 0x4068}, {0x0b16, 0x0000},
    {0x0f30, 0x6e25}, // pll
    {0x0f32, 0x7067}, // pll
    {0x0954, 0x0009}, {0x0956, 0x1100}, {0x0958, 0xcc80}, {0x095a, 0x0000},
    {0x0c00, 0x1110}, {0x0c02, 0x0011}, {0x0c04, 0x0000}, {0x0c06, 0x0200},
    {0x0c10, 0x0040}, // OB
    {0x0c12, 0x0040}, // OB
    {0x0c14, 0x0040}, // OB
    {0x0c16, 0x0040}, // OB
    {0x0a10, 0x4000}, // pedestal_data
    {0x3068, 0xf800}, {0x306a, 0xf876}, {0x006c, 0x0000}, {0x005e, 0x0200},
    {0x000e, 0x0200}, {0x0e0a, 0x0001}, {0x004a, 0x0100}, {0x004c, 0x0000},
    {0x004e, 0x0100}, {0x000c, 0x0022}, {0x0008, 0x0b00}, // line length pck
                                                          // 2816
    {0x005a, 0x0202}, {0x0012, 0x000e}, {0x0018, 0x0a31}, {0x0022, 0x0008},
    {0x0028, 0x0017}, {0x0024, 0x0028}, {0x002a, 0x002d}, {0x0026, 0x0030},
    {0x002c, 0x07c7}, {0x002e, 0x1111}, {0x0030, 0x1111}, {0x0032, 0x1111},
    {0x0006, 0x0823}, // frame length lines min 0x07BC
    {0x0a22, 0x0000}, {0x0a12, 0x0a20}, // x output size 2592
    {0x0a14, 0x0798}, // y output size 1944
    {0x003e, 0x0000}, {0x0074, 0x0821}, // coarse integ time
    {0x0070, 0x0411}, {0x0002, 0x0000}, {0x0a02, 0x0100}, {0x0a24, 0x0100},
    {0x0076, 0x0000}, // analog gain 1x
    {0x0060, 0x0000}, {0x0062, 0x0530}, {0x0064, 0x0500}, {0x0066, 0x0530},
    {0x0068, 0x0500}, {0x0122, 0x0300}, {0x015a, 0xff08}, {0x0804, 0x0200},
    {0x005c, 0x0102}, {0x0a1a, 0x0800}, // DGain pedestal enable
};

static const SENSOR_REG_T hi556_preview_setting[] = {
    // Sensor Information////////////////////////////
    // Sensor	  : hi556
    // Date		  : 2017-02-20
    // Customer        : SPRD_validation
    // Image size	  : 1296x972
    // MCLK/PCLK	  : 24MHz /88Mhz
    // MIPI speed(Mbps): 440Mbps x 2Lane
    // Frame Length	  : 2083
    // Line Length 	  : 2816
    // Max Fps 	  : 30.00fps
    // BLC offset	    : 64code
    // Firmware Ver.   : v1.0
    ////////////////////////////////////////////////
    {0x0a00, 0x0000}, {0x0b0a, 0x8259}, {0x0f30, 0x6e25}, // pll
    {0x0f32, 0x7167}, // pll
    {0x004a, 0x0100}, {0x004c, 0x0000}, {0x004e, 0x0000}, // per-frame control
                                                          // off, on 0x0100
    {0x000c, 0x0122}, {0x0008, 0x0b00}, // line length pck 2816
    {0x005a, 0x0404}, {0x0012, 0x000c}, {0x0018, 0x0a33}, {0x0022, 0x0008},
    {0x0028, 0x0017}, {0x0024, 0x0022}, {0x002a, 0x002b}, {0x0026, 0x0030},
    {0x002c, 0x07c7}, {0x002e, 0x3311}, {0x0030, 0x3311}, {0x0032, 0x3311},
    {0x0006, 0x0823}, // frame length lines 2083
    {0x0a22, 0x0000}, {0x0a12, 0x0510}, // x output size 2592
    {0x0a14, 0x03cc}, // y output size 1944
    {0x003e, 0x0000}, {0x0804, 0x0200}, {0x0a04, 0x016a}, // isp_en
    {0x090e, 0x0010}, // mipi_vblank_delay
    {0x090c, 0x09c0}, // mipi_hblank_delay
    {0x0902, 0x4319}, // mipi_tx_op_mode1, mipi_tx_op_mode2
    {0x0914, 0xc106}, // mipi_exit_seq, tlpx
    {0x0916, 0x040e}, // tclk_prepare, tclk_zero
    {0x0918, 0x0304}, // tclk_pre, ths_prepare
    {0x091a, 0x0709}, // ths_zero, ths_trail
    {0x091c, 0x0e06}, // tclk_post, tclk_trail
    {0x091e, 0x0300}, // mipi_exit, null
};

static const SENSOR_REG_T hi556_snapshot_setting[] = {
    // Sensor Information////////////////////////////
    // Sensor	  : hi556
    // Date		  : 2017-02-20
    // Customer        : SPRD_validation
    // Image size	  : 2592x1944
    // MCLK/PCLK	  : 24MHz /176Mhz
    // MIPI speed(Mbps): 880Mbps x 2Lane
    // Frame Length	  : 2083
    // Line Length 	  : 2816
    // Max Fps 	  : 30.00fps
    // BLC offset	    : 64code
    // Firmware Ver.   : v1.0
    ////////////////////////////////////////////////
    {0x0a00, 0x0000}, // stream off
    {0x0b0a, 0x8252}, {0x0f30, 0x6e25}, // pll
    {0x0f32, 0x7067}, // pll
    {0x004a, 0x0100}, {0x004c, 0x0000},
    {0x004e, 0x0000}, // per-frame control off, on 0x0100
    {0x000c, 0x0022}, {0x0008, 0x0b00}, // line length pck 2816
    {0x005a, 0x0202}, {0x0012, 0x000e},
    {0x0018, 0x0a31}, {0x0022, 0x0008},
    {0x0028, 0x0017}, {0x0024, 0x0028},
    {0x002a, 0x002d}, {0x0026, 0x0030},
    {0x002c, 0x07c7}, {0x002e, 0x1111},
    {0x0030, 0x1111}, {0x0032, 0x1111},
    {0x0006, 0x0823}, // frame length lines 2083
    {0x0a22, 0x0000}, {0x0a12, 0x0a20}, // x output size 2592
    {0x0a14, 0x0798}, // y output size 1944
    {0x003e, 0x0000}, {0x0804, 0x0200},
    {0x0a04, 0x014a}, {0x090c, 0x0fdc}, // mipi_vblank_delay
    {0x090e, 0x002d}, // mipi_hblank_delay
    {0x0902, 0x4319}, // mipi_tx_op_mode1, mipi_tx_op_mode2
    {0x0914, 0xc10a}, // mipi_exit_seq, tlpx
    {0x0916, 0x071f}, // tclk_prepare, tclk_zero
    {0x0918, 0x0408}, // tclk_pre, ths_prepare
    {0x091a, 0x0c0d}, // ths_zero, ths_trail
    {0x091c, 0x0f09}, // tclk_post, tclk_trail
    {0x091e, 0x0a00}, // mipi_exit, null
};

static const SENSOR_REG_T hi556_video_setting[] = {
    // Sensor Information////////////////////////////
    // Sensor	  : hi556
    // Date		  : 2017-02-20
    // Customer        : SPRD_validation
    // Image size	  : 1280x720
    // MCLK/PCLK	  : 24MHz /88Mhz
    // MIPI speed(Mbps): 440Mbps x 2Lane
    // Frame Length	  : 1041
    // Line Length 	  : 2816
    // Max Fps 	  : 60.04fps
    // BLC offset	    : 64code
    // Firmware Ver.   : v1.0
    ////////////////////////////////////////////////
    {0x0a00, 0x0000}, {0x0b0a, 0x8252}, {0x0f30, 0x6e25}, // pll
    {0x0f32, 0x7167}, // pll
    {0x004a, 0x0100}, {0x004c, 0x0000}, {0x004e, 0x0000}, // per-frame control
                                                          // off, on 0x0100
    {0x000c, 0x0022}, {0x0008, 0x0b00}, // line length pck 2816
    {0x005a, 0x0204}, {0x0012, 0x001c}, {0x0018, 0x0a23}, {0x0022, 0x0008},
    {0x0028, 0x0017}, {0x0024, 0x0122}, {0x002a, 0x0127}, {0x0026, 0x012c},
    {0x002c, 0x06cb}, {0x002e, 0x1111}, {0x0030, 0x1111}, {0x0032, 0x3311},
    {0x0006, 0x0411}, // frame length lines 1041
    {0x0a22, 0x0000}, {0x0a12, 0x0500}, // x output size 2592
    {0x0a14, 0x02d0}, // y output size 1944
    {0x003e, 0x0000}, {0x0804, 0x0200}, {0x0a04, 0x016a}, // isp_en
    {0x090e, 0x0010}, // mipi_vblank_delay
    {0x090c, 0x09c0}, // mipi_hblank_delay
    {0x0902, 0x4319}, // mipi_tx_op_mode1, mipi_tx_op_mode2
    {0x0914, 0xc106}, // mipi_exit_seq, tlpx
    {0x0916, 0x040e}, // tclk_prepare, tclk_zero
    {0x0918, 0x0304}, // tclk_pre, ths_prepare
    {0x091c, 0x0e06}, // ths_zero, ths_trail
    {0x091a, 0x0709}, // tclk_post, tclk_trail
    {0x091e, 0x0300}, // mipi_exit, null
};

static struct sensor_res_tab_info s_hi556_resolution_tab_raw[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .reg_tab = {{ADDR_AND_LEN_OF_ARRAY(hi556_init_setting), PNULL, 0,
                  .width = 0, .height = 0, .xclk_to_sensor = EX_MCLK,
                  .image_format = SENSOR_IMAGE_FORMAT_RAW},

                 /*{ADDR_AND_LEN_OF_ARRAY(hi556_video_setting), PNULL, 0,
         .width = VIDEO_WIDTH, .height = VIDEO_HEIGHT,
         .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW},

         {ADDR_AND_LEN_OF_ARRAY(hi556_preview_setting), PNULL, 0,
         .width = PREVIEW_WIDTH, .height = PREVIEW_HEIGHT,
         .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW},*/

                 {ADDR_AND_LEN_OF_ARRAY(hi556_snapshot_setting), PNULL, 0,
                  .width = SNAPSHOT_WIDTH, .height = SNAPSHOT_HEIGHT,
                  .xclk_to_sensor = EX_MCLK,
                  .image_format = SENSOR_IMAGE_FORMAT_RAW}}}
    /*If there are multiple modules,please add here*/
};

static SENSOR_TRIM_T s_hi556_resolution_trim_tab[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .trim_info =
         {
             {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},

             /*{.trim_start_x = VIDEO_TRIM_X, .trim_start_y = VIDEO_TRIM_Y,
          .trim_width = VIDEO_TRIM_W,   .trim_height = VIDEO_TRIM_H,
          .line_time = VIDEO_LINE_TIME, .bps_per_lane = VIDEO_MIPI_PER_LANE_BPS,
          .frame_line = VIDEO_FRAME_LENGTH,
          .scaler_trim = {.x = VIDEO_TRIM_X, .y = VIDEO_TRIM_Y, .w =
          VIDEO_TRIM_W, .h = VIDEO_TRIM_H}},

             {.trim_start_x = PREVIEW_TRIM_X, .trim_start_y = PREVIEW_TRIM_Y,
          .trim_width = PREVIEW_TRIM_W,   .trim_height = PREVIEW_TRIM_H,
          .line_time = PREVIEW_LINE_TIME, .bps_per_lane =
          PREVIEW_MIPI_PER_LANE_BPS,
          .frame_line = PREVIEW_FRAME_LENGTH,
          .scaler_trim = {.x = PREVIEW_TRIM_X, .y = PREVIEW_TRIM_Y, .w =
          PREVIEW_TRIM_W, .h = PREVIEW_TRIM_H}},*/

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

static SENSOR_REG_T hi556_shutter_reg[] = {{0x0074, 0x0821}};

static struct sensor_i2c_reg_tab hi556_shutter_tab = {
    .settings = hi556_shutter_reg, .size = ARRAY_SIZE(hi556_shutter_reg),
};

static SENSOR_REG_T hi556_again_reg[] = {{0x0076, 0x0000}};

static struct sensor_i2c_reg_tab hi556_again_tab = {
    .settings = hi556_again_reg, .size = ARRAY_SIZE(hi556_again_reg),
};

static struct sensor_reg_tag hi556_dgain_reg[] = {

};

struct sensor_i2c_reg_tab hi556_dgain_tab = {
    .settings = hi556_dgain_reg, .size = ARRAY_SIZE(hi556_dgain_reg),
};

static SENSOR_REG_T hi556_frame_length_reg[] = {{0x0006, 0x0823}};

static struct sensor_i2c_reg_tab hi556_frame_length_tab = {
    .settings = hi556_frame_length_reg,
    .size = ARRAY_SIZE(hi556_frame_length_reg),
};

static struct sensor_aec_i2c_tag hi556_aec_info = {
    .slave_addr = (I2C_SLAVE_ADDR >> 1),
    .addr_bits_type = SENSOR_I2C_REG_16BIT,
    .data_bits_type = SENSOR_I2C_VAL_16BIT,
    .shutter = &hi556_shutter_tab,
    .again = &hi556_again_tab,
    .dgain = &hi556_dgain_tab,
    .frame_length = &hi556_frame_length_tab,
};

static SENSOR_STATIC_INFO_T s_hi556_static_info[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .static_info = {.f_num = 240,
                     .focal_length = 239,
                     .max_fps = 30,
                     .max_adgain = 8,
                     .ois_supported = 0,
                     .pdaf_supported = 0,
                     .exp_valid_frame_num = 1,
                     .clamp_level = 64,
                     .adgain_valid_frame_num = 1,
                     .fov_info = {{4.614f, 3.444f}, 4.222f}}}
    /*If there are multiple modules,please add here*/
};

static SENSOR_MODE_FPS_INFO_T s_hi556_mode_fps_info[VENDOR_NUM] = {
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

static struct sensor_module_info s_hi556_module_info_tab[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .module_info = {.major_i2c_addr = I2C_SLAVE_ADDR >> 1,
                     .minor_i2c_addr = I2C_SLAVE_ADDR >> 1,

                     .reg_addr_value_bits = SENSOR_I2C_REG_16BIT |
                                            SENSOR_I2C_VAL_16BIT |
                                            SENSOR_I2C_FREQ_400,

                     .avdd_val = SENSOR_AVDD_2800MV,
                     .iovdd_val = SENSOR_AVDD_1800MV,
                     .dvdd_val = SENSOR_AVDD_1200MV,

                     .image_pattern = SENSOR_IMAGE_PATTERN_RAWRGB_GR,

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

static struct sensor_ic_ops s_hi556_ops_tab;
struct sensor_raw_info *s_hi556_mipi_raw_info_ptr = &s_hi556_mipi_raw_info;

/*==============================================================================
 * Description:
 * sensor all info
 * please modify this variable acording your spec
 *============================================================================*/
SENSOR_INFO_T g_hi556_mipi_raw_info = {
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
    .identify_code = {{.reg_addr = hi556_PID_ADDR,
                       .reg_value = hi556_PID_VALUE},
                      {.reg_addr = hi556_VER_ADDR,
                       .reg_value = hi556_VER_VALUE}},

    .source_width_max = SNAPSHOT_WIDTH,
    .source_height_max = SNAPSHOT_HEIGHT,
    .name = (cmr_s8 *)SENSOR_NAME,
    .image_format = SENSOR_IMAGE_FORMAT_RAW,

    .module_info_tab = s_hi556_module_info_tab,
    .module_info_tab_size = ARRAY_SIZE(s_hi556_module_info_tab),

    .resolution_tab_info_ptr = s_hi556_resolution_tab_raw,
    .sns_ops = &s_hi556_ops_tab,
    .raw_info_ptr = &s_hi556_mipi_raw_info_ptr,

    .video_tab_info_ptr = NULL,
    .sensor_version_info = (cmr_s8 *)"hi556_v1",
};

#endif
