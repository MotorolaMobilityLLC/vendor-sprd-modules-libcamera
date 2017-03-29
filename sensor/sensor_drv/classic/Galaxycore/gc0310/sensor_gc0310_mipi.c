/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <utils/Log.h>
#include "sensor.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"

#define GC0310_MIPI_I2C_ADDR_W 0x21
#define GC0310_MIPI_I2C_ADDR_R 0x21
#define SENSOR_GAIN_SCALE 16

static SENSOR_IOCTL_FUNC_TAB_T s_GC0310_MIPI_ioctl_func_tab;

typedef enum { FLICKER_50HZ = 0, FLICKER_60HZ, FLICKER_MAX } FLICKER_E;

SENSOR_REG_T GC0310_MIPI_YUV_COMMON1[] = {};

SENSOR_REG_T GC0310_MIPI_YUV_COMMON2[] = {
    {0xfe, 0xf0},
    {0xfe, 0xf0},
    {0xfe, 0x00},
    {0xfc, 0x16},
    {0xfc, 0x16},
    {0xf2, 0x07},
    {0xf3, 0xa1},
    {0xf5, 0x07},
    {0xf7, 0x88},
    {0xf8, 0x00},
    {0xf9, 0x4f},
    /* mipi */
    {0xfa, 0x74},
    {0xfc, 0xce},
    {0xfd, 0x00},

    {0x00, 0x2f},
    {0x01, 0x0f},
    {0x02, 0x04},
    {0x03, 0x02},
    {0x04, 0x12},
    {0x09, 0x00},
    {0x0a, 0x00},
    {0x0b, 0x00},
    {0x0c, 0x04},
    {0x0d, 0x01},
    {0x0e, 0xe8},
    {0x0f, 0x02},
    {0x10, 0x88},
    {0x16, 0x00},
    {0x17, 0x14},
    {0x18, 0x1a},
    {0x19, 0x14},
    {0x1b, 0x48},
    {0x1c, 0x6c},
    {0x1e, 0x6b},
    {0x1f, 0x28},
    {0x20, 0x8b}, // 0x89
    {0x21, 0x49},
    {0x22, 0xd0},
    {0x23, 0x04},
    {0x24, 0xff},
    {0x34, 0x20},
    // BLK//
    {0x26, 0x23},
    {0x28, 0xff},
    {0x29, 0x00},
    {0x32, 0x04},
    {0x33, 0x10},
    {0x37, 0x20},
    {0x38, 0x10},
    {0x47, 0x80},
    {0x4e, 0x66},
    {0xa8, 0x02},
    {0xa9, 0x80},
    // ISP reg//
    {0x40, 0xff},
    {0x41, 0x21},
    {0x42, 0xcf},
    {0x44, 0x02},
    {0x45, 0xa0},
    {0x46, 0x02},
    {0x4a, 0x11},
    {0x4b, 0x01},
    {0x4c, 0x20},
    {0x4d, 0x05},
    {0x4f, 0x01},
    {0x50, 0x01},
    {0x55, 0x01},
    {0x56, 0xe0},
    {0x57, 0x02},
    {0x58, 0x80},
    // GAIN//
    {0x70, 0x70},
    {0x5a, 0x84},
    {0x5b, 0xc9},
    {0x5c, 0xed},
    {0x77, 0x74},
    {0x78, 0x40},
    {0x79, 0x5f},
    // DNDD//
    {0x82, 0x14},
    {0x83, 0x0b},
    {0x89, 0xf0},
    // EEINTP//
    {0x8f, 0xaa},
    {0x90, 0x8c},
    {0x91, 0x90},
    {0x92, 0x03},
    {0x93, 0x03},
    {0x94, 0x05},
    {0x95, 0x65},
    {0x96, 0xf0},
    // ASDE//
    {0xfe, 0x00},
    {0x9a, 0x20},
    {0x9b, 0x80},
    {0x9c, 0x40},
    {0x9d, 0x80},
    {0xa1, 0x30},
    {0xa2, 0x32},
    {0xa4, 0x80},
    {0xa5, 0x28},
    {0xaa, 0x30},
    {0xac, 0x22},
    // GAMMA//
    {0xfe, 0x00}, // big gamma
    {0xbf, 0x08},
    {0xc0, 0x16},
    {0xc1, 0x28},
    {0xc2, 0x41},
    {0xc3, 0x5a},
    {0xc4, 0x6c},
    {0xc5, 0x7a},
    {0xc6, 0x96},
    {0xc7, 0xac},
    {0xc8, 0xbc},
    {0xc9, 0xc9},
    {0xca, 0xd3},
    {0xcb, 0xdd},
    {0xcc, 0xe5},
    {0xcd, 0xf1},
    {0xce, 0xfa},
    {0xcf, 0xff},

    // YCP//
    {0xd0, 0x40},
    {0xd1, 0x34},
    {0xd2, 0x34},
    {0xd3, 0x40},
    {0xd6, 0xf2},
    {0xd7, 0x1b},
    {0xd8, 0x18},
    {0xdd, 0x03},
    // AEC//
    {0xfe, 0x01},
    {0x05, 0x30},
    {0x06, 0x75},
    {0x07, 0x40},
    {0x08, 0xb0},
    {0x0a, 0xc5},
    {0x0b, 0x11},
    {0x0c, 0x00},
    {0x12, 0x52},
    {0x13, 0x38},
    {0x18, 0x95},
    {0x19, 0x96},
    {0x1f, 0x20},
    {0x20, 0xc0}, // 80

    {0x3e, 0x40},
    {0x3f, 0x57},
    {0x40, 0x7d},
    {0x03, 0x60},

    {0x44, 0x02},
    // AWB//
    {0x1c, 0x91},
    {0x21, 0x15},
    {0x50, 0x80},
    {0x56, 0x04},
    {0x59, 0x08},
    {0x5b, 0x02},
    {0x61, 0x8d},
    {0x62, 0xa7},
    {0x63, 0xd0},
    {0x65, 0x06},
    {0x66, 0x06},
    {0x67, 0x84},
    {0x69, 0x08},
    {0x6a, 0x25}, // 50
    {0x6b, 0x01},
    {0x6c, 0x00},
    {0x6d, 0x02},
    {0x6e, 0xf0},
    {0x6f, 0x80},
    {0x76, 0x80},

    {0x78, 0xaf},
    {0x79, 0x75},
    {0x7a, 0x40},
    {0x7b, 0x50},
    {0x7c, 0x0c},

    {0xa4, 0xb9},
    {0xa5, 0xa0},
    {0x90, 0xc9},
    {0x91, 0xbe},

    {0xa6, 0xb8},
    {0xa7, 0x95},
    {0x92, 0xe6},
    {0x93, 0xca},

    {0xa9, 0xbc},
    {0xaa, 0x95},
    {0x95, 0x23},
    {0x96, 0xe7},

    {0xab, 0x9d},
    {0xac, 0x80},
    {0x97, 0x43},
    {0x98, 0x24},

    {0xae, 0xb7},
    {0xaf, 0x9e},
    {0x9a, 0x43},
    {0x9b, 0x24},

    {0xb0, 0xc8},
    {0xb1, 0x97},
    {0x9c, 0xc4},
    {0x9d, 0x44},

    {0xb3, 0xb7},
    {0xb4, 0x7f},
    {0x9f, 0xc7},
    {0xa0, 0xc8},

    {0xb5, 0x00},
    {0xb6, 0x00},
    {0xa1, 0x00},
    {0xa2, 0x00},

    {0x86, 0x60},
    {0x87, 0x08},
    {0x88, 0x00},
    {0x89, 0x00},
    {0x8b, 0xde},
    {0x8c, 0x80},
    {0x8d, 0x00},
    {0x8e, 0x00},

    {0x94, 0x55},
    {0x99, 0xa6},
    {0x9e, 0xaa},
    {0xa3, 0x0a},
    {0x8a, 0x0a},
    {0xa8, 0x55},
    {0xad, 0x55},
    {0xb2, 0x55},
    {0xb7, 0x05},
    {0x8f, 0x05},

    {0xb8, 0xcc},
    {0xb9, 0x9a},
    // CC//
    {0xfe, 0x01},
    {0xd0, 0x38},
    {0xd1, 0x00},
    {0xd2, 0x02}, // 0a
    {0xd3, 0x04},
    {0xd4, 0x38},
    {0xd5, 0x12}, // f0
    {0xd6, 0x30},
    {0xd7, 0x00},
    {0xd8, 0x0a},
    {0xd9, 0x16},
    {0xda, 0x39},
    {0xdb, 0xf8},
    // LSC//
    {0xfe, 0x01},
    {0xc1, 0x3c},
    {0xc2, 0x50},
    {0xc3, 0x00},
    {0xc4, 0x40},
    {0xc5, 0x30},
    {0xc6, 0x30},
    {0xc7, 0x10},
    {0xc8, 0x00},
    {0xc9, 0x00},
    {0xdc, 0x20},
    {0xdd, 0x10},
    {0xdf, 0x00},
    {0xde, 0x00},
    // Histogram//
    {0x01, 0x10},
    {0x0b, 0x31},
    {0x0e, 0x50},
    {0x0f, 0x0f},
    {0x10, 0x6e},
    {0x12, 0xa0},
    {0x15, 0x60},
    {0x16, 0x60},
    {0x17, 0xe0},
    // Measure Window//
    {0xcc, 0x0c},
    {0xcd, 0x10},
    {0xce, 0xa0},
    {0xcf, 0xe6},
    // dark sun//
    {0x45, 0xf7},
    {0x46, 0xff},
    {0x47, 0x15},
    {0x48, 0x03},
    {0x4f, 0x60},
    // banding//
    {0xfe, 0x00},
    {0x05, 0x01},
    {0x06, 0xb6}, // HB
    {0x07, 0x00},
    {0x08, 0x2a}, // VB
    {0xfe, 0x01},
    {0x25, 0x00}, // step
    {0x26, 0x6a},
    {0x27, 0x02}, // 20fps
    {0x28, 0x12},
    {0x29, 0x03}, // 12.5fps
    {0x2a, 0x50},
    {0x2b, 0x04}, // 7.14fps
    {0x2c, 0xf8},
    {0x2d, 0x07}, // 5.55fps
    {0x2e, 0x74},
    {0x3c, 0x20},
    {0xfe, 0x03},
    {0x01, 0x00},
    {0x02, 0x00},
    {0x10, 0x00},
    {0x15, 0x00},
    {0x17, 0x00},
    {0x04, 0x10},
    {0x05, 0x00},
    {0x40, 0x00},
    {0x52, 0x02},
    {0x53, 0x00},
    {0x54, 0x20},
    {0x55, 0x20},
    // dark sun//
    {0x5a, 0x00},
    {0x5b, 0x00},
    {0x5c, 0x02},
    {0x5d, 0xe0},
    {0x5a, 0x03},
    // banding//
    {0x60, 0xb6},
    {0x61, 0x80},
    {0x62, 0x9d}, // HB
    {0x63, 0xb6},
    {0x64, 0x05}, // VB
    {0x65, 0xff},
    {0x66, 0x00}, // step
    {0x67, 0x00},
    {0xfe, 0x00},
};

SENSOR_REG_T GC0310_MIPI_YUV_COMMON[] = {
    {0xfe, 0xf0},
    {0xfe, 0xf0},
    {0xfe, 0x00},
    {0xfc, 0x0e},
    {0xfc, 0x0e},
    {0xf2, 0x80},
    {0xf3, 0x00},
    {0xf7, 0x1b},
    {0xf8, 0x04},
    {0xf9, 0x8e},
    {0xfa, 0x11},
    /* mipi */
    {0xfe, 0x03},
    {0x40, 0x08},
    {0x42, 0x00},
    {0x43, 0x00},
    {0x01, 0x03},
    {0x10, 0x84},

    {0x01, 0x03},
    {0x02, 0x11},
    {0x03, 0x94},
    {0x04, 0x01},
    {0x05, 0x00},
    {0x06, 0x80},
    {0x11, 0x1e},
    {0x12, 0x00},
    {0x13, 0x05},
    {0x15, 0x10},
    {0x21, 0x10},
    {0x22, 0x01},
    {0x23, 0x10},
    {0x24, 0x02},
    {0x25, 0x10},
    {0x26, 0x03},
    {0x29, 0x02},
    {0x2a, 0x0a},
    {0x2b, 0x04},
    {0xfe, 0x00},

    {0x00, 0x2f},
    {0x01, 0x0f},
    {0x02, 0x04},
    {0x03, 0x03},
    {0x04, 0x50},
    {0x09, 0x00},
    {0x0a, 0x00},
    {0x0b, 0x00},
    {0x0c, 0x04},
    {0x0d, 0x01},
    {0x0e, 0xe8},
    {0x0f, 0x02},
    {0x10, 0x88},
    {0x16, 0x00},
    {0x17, 0x14},
    {0x18, 0x1a},
    {0x19, 0x14},
    {0x1b, 0x48},
    {0x1e, 0x6b},
    {0x1f, 0x28},
    {0x20, 0x8b}, // 0x89
    {0x21, 0x49},
    {0x22, 0xb0},
    {0x23, 0x04},
    {0x24, 0x16},
    {0x34, 0x20},
    // BLK//
    {0x26, 0x23},
    {0x28, 0xff},
    {0x29, 0x00},
    {0x33, 0x10},
    {0x37, 0x20},
    {0x38, 0x10},
    {0x47, 0x80},
    {0x4e, 0x66},
    {0xa8, 0x02},
    {0xa9, 0x80},
    // ISP reg//
    {0x40, 0xff},
    {0x41, 0x21},
    {0x42, 0xcf},
    {0x44, 0x02},
    {0x45, 0xa0},
    {0x46, 0x02},
    {0x4a, 0x11},
    {0x4b, 0x01},
    {0x4c, 0x20},
    {0x4d, 0x05},
    {0x4f, 0x01},
    {0x50, 0x01},
    {0x55, 0x01},
    {0x56, 0xe0},
    {0x57, 0x02},
    {0x58, 0x80},
    // GAIN//
    {0x70, 0x70},
    {0x5a, 0x84},
    {0x5b, 0xc9},
    {0x5c, 0xed},
    {0x77, 0x74},
    {0x78, 0x40},
    {0x79, 0x5f},
    // DNDD//
    {0x82, 0x14},
    {0x83, 0x0b},
    {0x89, 0xf0},
    // EEINTP//
    {0x8f, 0xaa},
    {0x90, 0x8c},
    {0x91, 0x90},
    {0x92, 0x03},
    {0x93, 0x03},
    {0x94, 0x05},
    {0x95, 0x65},
    {0x96, 0xf0},
    // ASDE//
    {0xfe, 0x00},
    {0x9a, 0x20},
    {0x9b, 0x80},
    {0x9c, 0x40},
    {0x9d, 0x80},
    {0xa1, 0x30},
    {0xa2, 0x32},
    {0xa4, 0x30},
    {0xa5, 0x30},
    {0xaa, 0x10},
    {0xac, 0x22},
    // GAMMA//
    {0xfe, 0x00}, // big gamma
    {0xbf, 0x08},
    {0xc0, 0x1d},
    {0xc1, 0x34},
    {0xc2, 0x4b},
    {0xc3, 0x60},
    {0xc4, 0x73},
    {0xc5, 0x85},
    {0xc6, 0x9f},
    {0xc7, 0xb5},
    {0xc8, 0xc7},
    {0xc9, 0xd5},
    {0xca, 0xe0},
    {0xcb, 0xe7},
    {0xcc, 0xec},
    {0xcd, 0xf4},
    {0xce, 0xfa},
    {0xcf, 0xff},

    // YCP//
    {0xd0, 0x40},
    {0xd1, 0x28},
    {0xd2, 0x28},
    {0xd3, 0x40},
    {0xd6, 0xf2},
    {0xd7, 0x1b},
    {0xd8, 0x18},
    {0xdd, 0x03},
    // AEC//
    {0xfe, 0x01},
    {0x05, 0x30},
    {0x06, 0x75},
    {0x07, 0x40},
    {0x08, 0xb0},
    {0x0a, 0xc5},
    {0x0b, 0x11},
    {0x0c, 0x00},
    {0x12, 0x52},
    {0x13, 0x38},
    {0x18, 0x95},
    {0x19, 0x96},
    {0x1f, 0x20},
    {0x20, 0xc0}, // 80

    {0x3e, 0x40},
    {0x3f, 0x57},
    {0x40, 0x7d},
    {0x03, 0x60},

    {0x44, 0x03},
    // AWB//
    {0x1c, 0x91},
    {0x21, 0x15},
    {0x50, 0x80},
    {0x56, 0x04},
    {0x59, 0x08},
    {0x5b, 0x02},
    {0x61, 0x8d},
    {0x62, 0xa7},
    {0x63, 0xd0},
    {0x65, 0x06},
    {0x66, 0x06},
    {0x67, 0x84},
    {0x69, 0x08},
    {0x6a, 0x25}, // 50
    {0x6b, 0x01},
    {0x6c, 0x00},
    {0x6d, 0x02},
    {0x6e, 0xf0},
    {0x6f, 0x80},
    {0x76, 0x80},

    {0x78, 0xaf},
    {0x79, 0x75},
    {0x7a, 0x40},
    {0x7b, 0x50},
    {0x7c, 0x0c},

    {0xa4, 0xb9},
    {0xa5, 0xa0},
    {0x90, 0xc9},
    {0x91, 0xbe},

    {0xa6, 0xb8},
    {0xa7, 0x95},
    {0x92, 0xe6},
    {0x93, 0xca},

    {0xa9, 0xb6},
    {0xaa, 0x89},
    {0x95, 0x23},
    {0x96, 0xe7},

    {0xab, 0x9d},
    {0xac, 0x80},
    {0x97, 0x43},
    {0x98, 0x24},

    {0xae, 0xb7},
    {0xaf, 0x9e},
    {0x9a, 0x43},
    {0x9b, 0x24},

    {0xb0, 0xc8},
    {0xb1, 0x97},
    {0x9c, 0xc4},
    {0x9d, 0x44},

    {0xb3, 0xb7},
    {0xb4, 0x7f},
    {0x9f, 0xc7},
    {0xa0, 0xc8},

    {0xb5, 0x00},
    {0xb6, 0x00},
    {0xa1, 0x00},
    {0xa2, 0x00},

    {0x86, 0x60},
    {0x87, 0x08},
    {0x88, 0x00},
    {0x89, 0x00},
    {0x8b, 0xde},
    {0x8c, 0x80},
    {0x8d, 0x00},
    {0x8e, 0x00},

    {0x94, 0x55},
    {0x99, 0xa6},
    {0x9e, 0xaa},
    {0xa3, 0x0a},
    {0x8a, 0x0a},
    {0xa8, 0x55},
    {0xad, 0x55},
    {0xb2, 0x55},
    {0xb7, 0x05},
    {0x8f, 0x05},

    {0xb8, 0xcc},
    {0xb9, 0x9a},
    // CC//
    {0xfe, 0x01},
    {0xd0, 0x38},
    {0xd1, 0x00},
    {0xd2, 0x06}, // 0a
    {0xd3, 0xf8},
    {0xd4, 0x3c},
    {0xd5, 0xf0}, // f0
    {0xd6, 0x30},
    {0xd7, 0x00},
    {0xd8, 0x0a},
    {0xd9, 0x16},
    {0xda, 0x39},
    {0xdb, 0xf8},
    // LSC//
    {0xfe, 0x01},
    {0xc1, 0x3c},
    {0xc2, 0x50},
    {0xc3, 0x00},
    {0xc4, 0x40},
    {0xc5, 0x30},
    {0xc6, 0x30},
    {0xc7, 0x10},
    {0xc8, 0x00},
    {0xc9, 0x00},
    {0xdc, 0x20},
    {0xdd, 0x10},
    {0xdf, 0x00},
    {0xde, 0x00},
    // Histogram//
    {0x01, 0x10},
    {0x0b, 0x31},
    {0x0e, 0x50},
    {0x0f, 0x0f},
    {0x10, 0x6e},
    {0x12, 0xa0},
    {0x15, 0x60},
    {0x16, 0x60},
    {0x17, 0xe0},
    // Measure Window//
    {0xcc, 0x0c},
    {0xcd, 0x10},
    {0xce, 0xa0},
    {0xcf, 0xe6},
    // dark sun//
    {0x45, 0xf7},
    {0x46, 0xff},
    {0x47, 0x15},
    {0x48, 0x03},
    {0x4f, 0x60},
    // banding//
    {0xfe, 0x00},
    {0x05, 0x02},
    {0x06, 0xd1}, // HB
    {0x07, 0x00},
    {0x08, 0x22}, // VB
    {0xfe, 0x01},
    {0x25, 0x00}, // step
    {0x26, 0x6a},
    {0x27, 0x02}, // 20fps
    {0x28, 0x12},
    {0x29, 0x03}, // 12.5fps
    {0x2a, 0x50},
    {0x2b, 0x05}, // 7.14fps
    {0x2c, 0xcc},
    {0x2d, 0x07}, // 5.55fps
    {0x2e, 0x74},
    {0x3c, 0x20},
    {0xfe, 0x00},
    {0x7a, 0x80},
    {0x7b, 0x80},
    {0x7c, 0x86},
};

static SENSOR_REG_TAB_INFO_T s_GC0310_MIPI_resolution_Tab_YUV[] = {
    // COMMON INIT
    {ADDR_AND_LEN_OF_ARRAY(GC0310_MIPI_YUV_COMMON1), 0, 0, 24,
     SENSOR_IMAGE_FORMAT_YUV422},

    // YUV422 PREVIEW 1
    {ADDR_AND_LEN_OF_ARRAY(GC0310_MIPI_YUV_COMMON1), 640, 480, 24,
     SENSOR_IMAGE_FORMAT_YUV422},
    {PNULL, 0, 0, 0, 0, 0},
    {PNULL, 0, 0, 0, 0, 0},
    {PNULL, 0, 0, 0, 0, 0},

    // YUV422 PREVIEW 2
    {PNULL, 0, 0, 0, 0, 0},
    {PNULL, 0, 0, 0, 0, 0},
    {PNULL, 0, 0, 0, 0, 0},
    {PNULL, 0, 0, 0, 0, 0}};

static SENSOR_TRIM_T s_Gc0310_Resolution_Trim_Tab[] = {
    {0, 0, 640, 480, 0, 0, 0, {0, 0, 640, 480}},

    {0, 0, 640, 480, 0, 240, 0x03b8, {0, 0, 640, 480}},
    {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
    {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
    {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},

    {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
    {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
    {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
    {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}}};

LOCAL SENSOR_VIDEO_INFO_T s_GC0310_video_info[] = {
    {{{0, 30, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
    {{{0, 15, 0, 0}, {0, 30, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL}};

SENSOR_INFO_T g_GC0310_MIPI_yuv_info = {
    .salve_i2c_addr_w = GC0310_MIPI_I2C_ADDR_W, // salve i2c write address
    .salve_i2c_addr_r = GC0310_MIPI_I2C_ADDR_R, // salve i2c read address

    .reg_addr_value_bits = SENSOR_I2C_REG_8BIT | SENSOR_I2C_VAL_8BIT |
                           SENSOR_I2C_FREQ_100, // bit0: 0: i2c register value
                                                // is 8 bit, 1: i2c register
                                                // value is 16 bit
    // bit2: 0: i2c register addr  is 8 bit, 1: i2c register addr  is 16 bit
    // other bit: reseved
    .hw_signal_polarity = SENSOR_HW_SIGNAL_PCLK_P | SENSOR_HW_SIGNAL_VSYNC_N |
                          SENSOR_HW_SIGNAL_HSYNC_P, // bit0: 0:negative;
                                                    // 1:positive -> polarily of
                                                    // pixel clock
    // bit2: 0:negative; 1:positive -> polarily of horizontal synchronization
    // signal
    // bit4: 0:negative; 1:positive -> polarily of vertical synchronization
    // signal
    // other bit: reseved

    // preview mode
    .environment_mode = SENSOR_ENVIROMENT_NORMAL | SENSOR_ENVIROMENT_NIGHT |
                        SENSOR_ENVIROMENT_SUNNY,

    // image effect
    .image_effect = SENSOR_IMAGE_EFFECT_NORMAL |
                    SENSOR_IMAGE_EFFECT_BLACKWHITE | SENSOR_IMAGE_EFFECT_RED |
                    SENSOR_IMAGE_EFFECT_GREEN | SENSOR_IMAGE_EFFECT_BLUE |
                    SENSOR_IMAGE_EFFECT_YELLOW | SENSOR_IMAGE_EFFECT_NEGATIVE |
                    SENSOR_IMAGE_EFFECT_CANVAS,

    // while balance mode
    .wb_mode = 0,

    .step_count = 7, // bit[0:7]: count of step in brightness, contrast,
                     // sharpness, saturation
                     // bit[8:31] reseved

    .reset_pulse_level = SENSOR_LOW_PULSE_RESET, // reset pulse level
    .reset_pulse_width = 100,                    // reset pulse width(ms)

    .power_down_level =
        SENSOR_HIGH_LEVEL_PWDN, // 1: high level valid; 0: low level valid

    .identify_count = 2, // count of identify code
    .identify_code =
        {
            {
                .reg_addr = 0xf0, .reg_value = 0xa3,
            },
            {
                .reg_addr = 0xf1, .reg_value = 0x10,
            },
        },

    .avdd_val = SENSOR_AVDD_2800MV,  // voltage of avdd
    .iovdd_val = SENSOR_AVDD_1800MV, // dvdd
    .dvdd_val = SENSOR_AVDD_1800MV,  // dvdd

    .source_width_max = 640,         // max width of source image
    .source_height_max = 480,        // max height of source image
    .name = (cmr_s8 *)"GC0310_MIPI", // name of sensor

    .image_format =
        SENSOR_IMAGE_FORMAT_YUV422, // define in SENSOR_IMAGE_FORMAT_E enum,
    // if set to SENSOR_IMAGE_FORMAT_MAX here, image format depent on
    // SENSOR_REG_TAB_INFO_T
    .image_pattern =
        SENSOR_IMAGE_PATTERN_YUV422_YUYV, // pattern of input image form sensor;

    .resolution_tab_info_ptr =
        s_GC0310_MIPI_resolution_Tab_YUV, // point to resolution table
                                          // information structure
    .ioctl_func_tab_ptr =
        &s_GC0310_MIPI_ioctl_func_tab, // point to ioctl function table
    .ext_info_ptr = PNULL,             // extend information about sensor
    .preview_skip_num = 2,             // skip frame num before preview
    .capture_skip_num = 1,             // skip frame num before capture
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
            .bus_width = 1,   /*lane number or bit-width*/
            .pixel_width = 8, /*bits per pixel*/
            .is_loose = 1,    /*0 packet, 1 half word per pixel*/
        },

    .video_tab_info_ptr = NULL,
    .change_setting_skip_num = 3,
    .horizontal_view_angle = 48,
    .vertical_view_angle = 48,
    .sensor_version_info = (cmr_s8 *)"gc0310_v1",

};

static void GC0310_MIPI_WriteReg(SENSOR_HW_HANDLE handle, uint8_t subaddr,
                                 uint8_t data) {
    Sensor_WriteReg(subaddr, data);
    //	Sensor_WriteReg_8bits(subaddr, data);
}

static uint8_t GC0310_MIPI_ReadReg(SENSOR_HW_HANDLE handle, uint8_t subaddr) {
    uint8_t value;
    value = Sensor_ReadReg(subaddr);
    return value;
}

static unsigned long GC0310_GetResolutionTrimTab(SENSOR_HW_HANDLE handle,
                                                 uint8_t param) {
    UNUSED(param);
    return (unsigned long)s_Gc0310_Resolution_Trim_Tab;
}

static uint32_t GC0310_MIPI_PowerOn(SENSOR_HW_HANDLE handle,
                                    uint32_t power_on) {
    SENSOR_AVDD_VAL_E dvdd_val = g_GC0310_MIPI_yuv_info.dvdd_val;
    SENSOR_AVDD_VAL_E avdd_val = g_GC0310_MIPI_yuv_info.avdd_val;
    SENSOR_AVDD_VAL_E iovdd_val = g_GC0310_MIPI_yuv_info.iovdd_val;
    BOOLEAN power_down = g_GC0310_MIPI_yuv_info.power_down_level;
    BOOLEAN reset_level = g_GC0310_MIPI_yuv_info.reset_pulse_level;

    if (SENSOR_TRUE == power_on) {
        Sensor_PowerDown(!power_down);
        usleep(1000);
        Sensor_SetIovddVoltage(iovdd_val);
        usleep(2000);
        Sensor_SetAvddVoltage(avdd_val);
        usleep(1000);
        Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);
        usleep(1000);
        Sensor_PowerDown(power_down);
        usleep(1000);
        Sensor_PowerDown(!power_down);
    } else {
        Sensor_PowerDown(power_down);
        usleep(1000);
        Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
        usleep(1000);
        Sensor_SetAvddVoltage(SENSOR_AVDD_CLOSED);
        usleep(2000);
        Sensor_SetIovddVoltage(SENSOR_AVDD_CLOSED);
        usleep(1000);
    }
    return SENSOR_SUCCESS;
}

static unsigned long GC0310_MIPI_Identify(SENSOR_HW_HANDLE handle,
                                          unsigned long param) {
#define GC0310_MIPI_PID_ADDR1 0xf0
#define GC0310_MIPI_PID_ADDR2 0xf1
#define GC0310_MIPI_SENSOR_ID 0xa310

    uint16_t sensor_id = 0;
    uint8_t pid_value = 0;
    uint8_t ver_value = 0;
    int i;
    // set_brightness(handle,1);

    for (i = 0; i < 3; i++) {
        sensor_id = GC0310_MIPI_ReadReg(handle, GC0310_MIPI_PID_ADDR1) << 8;
        sensor_id |= GC0310_MIPI_ReadReg(handle, GC0310_MIPI_PID_ADDR2);
        SENSOR_PRINT("%s sensor_id gc0310 is %x\n", __func__, sensor_id);
        if (sensor_id == GC0310_MIPI_SENSOR_ID) {
            SENSOR_PRINT("sensor is GC0310_MIPI\n");
            return SENSOR_SUCCESS;
        }
    }

    return SENSOR_FAIL;
}

static unsigned long get_brightness(SENSOR_HW_HANDLE handle, cmr_u32 *param) {
    uint16_t i = 0;

    GC0310_MIPI_WriteReg(handle, 0xfe, 0x01);
    *param = GC0310_MIPI_ReadReg(handle, 0x14);
    SENSOR_LOGI("SENSOR: get_brightness: lumma = 0x%x\n", *param);
    GC0310_MIPI_WriteReg(handle, 0xfe, 0x00);
    return 0;
}

static unsigned long GC0310_access_val(SENSOR_HW_HANDLE handle,
                                       unsigned long param) {
    uint32_t rtn = SENSOR_SUCCESS;
    SENSOR_VAL_T *param_ptr = (SENSOR_VAL_T *)param;
    uint16_t tmp;

    SENSOR_LOGI("SENSOR_GC0310: _gc0310_access_val E param_ptr = %p",
                param_ptr);
    if (!param_ptr) {
        return rtn;
    }

    SENSOR_LOGI("SENSOR_GC0310: param_ptr->type=%x", param_ptr->type);
    switch (param_ptr->type) {
    case SENSOR_VAL_TYPE_GET_BV:
        rtn = get_brightness(handle, param_ptr->pval);
        break;
    default:
        break;
    }

    SENSOR_LOGI("SENSOR_GC0310: _gc0310_access_val X");

    return rtn;
}

SENSOR_REG_T GC0310_MIPI_brightness_tab[][2] = {
    {{0xd5, 0xd0}, {0xff, 0xff}}, {{0xd5, 0xe0}, {0xff, 0xff}},
    {{0xd5, 0xf0}, {0xff, 0xff}}, {{0xd5, 0x00}, {0xff, 0xff}},
    {{0xd5, 0x20}, {0xff, 0xff}}, {{0xd5, 0x30}, {0xff, 0xff}},
    {{0xd5, 0x40}, {0xff, 0xff}},
};

static unsigned long set_brightness(SENSOR_HW_HANDLE handle,
                                    unsigned long level) {
    uint16_t i;
    SENSOR_REG_T *sensor_reg_ptr =
        (SENSOR_REG_T *)GC0310_MIPI_brightness_tab[level];

    if (level > 6)
        return 0;

    for (i = 0; (0xFF != sensor_reg_ptr[i].reg_addr) &&
                (0xFF != sensor_reg_ptr[i].reg_value);
         i++) {
        GC0310_MIPI_WriteReg(handle, sensor_reg_ptr[i].reg_addr,
                             sensor_reg_ptr[i].reg_value);
    }

    return 0;
}
static unsigned long set_init(SENSOR_HW_HANDLE handle) {
    uint16_t i;
    SENSOR_REG_T *sensor_reg_ptr = (SENSOR_REG_T *)GC0310_MIPI_YUV_COMMON2;
    // usleep(100*1000);

    SENSOR_LOGI("gx ");
    for (i = 0; (0xFF != sensor_reg_ptr[i].reg_addr) &&
                (0xFF != sensor_reg_ptr[i].reg_value);
         i++) {
        GC0310_MIPI_WriteReg(handle, sensor_reg_ptr[i].reg_addr,
                             sensor_reg_ptr[i].reg_value);
        // usleep(100000);
        // usleep(1000);
    }
    SENSOR_LOGI("X");

    // get_brightness(handle);
    // SENSOR_LOGI("X");

    return 0;
}

LOCAL const SENSOR_REG_T gc0310_video_mode_tab[][18] = {
    /* normal preview mode*/
    {{0xff, 0xff}},
    /* video mode: 15fps */
    {{0xfe, 0x00},
     {0x05, 0x02},
     {0x06, 0xd1},
     {0x07, 0x00},
     {0x08, 0x22},
     {0xfe, 0x01},
     {0x25, 0x00},
     {0x26, 0x6a},
     {0x27, 0x02},
     {0x28, 0x7c},
     {0x29, 0x02},
     {0x2a, 0x7c},
     {0x2b, 0x02},
     {0x2c, 0x7c},
     {0x2d, 0x02},
     {0x2e, 0x7c},
     {0xfe, 0x00},
     {0xff, 0xff}}};

static unsigned long set_GC0310_video_mode(SENSOR_HW_HANDLE handle,
                                           unsigned long mode) {
    SENSOR_REG_T_PTR sensor_reg_ptr =
        (SENSOR_REG_T_PTR)gc0310_video_mode_tab[mode];
    uint16_t i = 0x00;

    SENSOR_LOGI("SENSOR: set_video_mode: mode = %lu\n", mode);
    if (mode > 1 || mode == 0)
        return 0;

    for (i = 0x00; (0xff != sensor_reg_ptr[i].reg_addr) ||
                   (0xff != sensor_reg_ptr[i].reg_value);
         i++) {
        GC0310_MIPI_WriteReg(handle, sensor_reg_ptr[i].reg_addr,
                             sensor_reg_ptr[i].reg_value);
    }

    return 0;
}

SENSOR_REG_T GC0310_MIPI_ev_tab[][4] = {
    {{0xfe, 0x01}, {0x13, 0x18}, {0xfe, 0x00}, {0xff, 0xff}},
    {{0xfe, 0x01}, {0x13, 0x20}, {0xfe, 0x00}, {0xff, 0xff}},
    {{0xfe, 0x01}, {0x13, 0x28}, {0xfe, 0x00}, {0xff, 0xff}},
    {{0xfe, 0x01}, {0x13, 0x38}, {0xfe, 0x00}, {0xff, 0xff}},
    {{0xfe, 0x01}, {0x13, 0x40}, {0xfe, 0x00}, {0xff, 0xff}},
    {{0xfe, 0x01}, {0x13, 0x48}, {0xfe, 0x00}, {0xff, 0xff}},
    {{0xfe, 0x01}, {0x13, 0x50}, {0xfe, 0x00}, {0xff, 0xff}},
};

static unsigned long set_GC0310_MIPI_ev(SENSOR_HW_HANDLE handle,
                                        unsigned long level) {
    uint16_t i;
    SENSOR_REG_T *sensor_reg_ptr = (SENSOR_REG_T *)GC0310_MIPI_ev_tab[level];

    if (level > 6)
        return 0;

    for (i = 0; (0xFF != sensor_reg_ptr[i].reg_addr) ||
                (0xFF != sensor_reg_ptr[i].reg_value);
         i++) {
        GC0310_MIPI_WriteReg(handle, sensor_reg_ptr[i].reg_addr,
                             sensor_reg_ptr[i].reg_value);
    }
    SENSOR_LOGI("set ev");

    return 0;
}

static unsigned long set_GC0310_MIPI_anti_flicker(SENSOR_HW_HANDLE handle,
                                                  unsigned long param) {
    switch (param) {
    case FLICKER_50HZ:
        GC0310_MIPI_WriteReg(handle, 0xfe, 0x00);
        GC0310_MIPI_WriteReg(handle, 0x05, 0x02); // hb
        GC0310_MIPI_WriteReg(handle, 0x06, 0xd1);
        GC0310_MIPI_WriteReg(handle, 0x07, 0x00); // vb
        GC0310_MIPI_WriteReg(handle, 0x08, 0x22);
        GC0310_MIPI_WriteReg(handle, 0xfe, 0x01);
        GC0310_MIPI_WriteReg(handle, 0x25, 0x00); // step
        GC0310_MIPI_WriteReg(handle, 0x26, 0x6a);
        GC0310_MIPI_WriteReg(handle, 0x27, 0x02); // level1
        GC0310_MIPI_WriteReg(handle, 0x28, 0x12);
        GC0310_MIPI_WriteReg(handle, 0x29, 0x03); // level2
        GC0310_MIPI_WriteReg(handle, 0x2a, 0x50);
        GC0310_MIPI_WriteReg(handle, 0x2b, 0x05); // 6e8//level3 640
        GC0310_MIPI_WriteReg(handle, 0x2c, 0xcc);
        GC0310_MIPI_WriteReg(handle, 0x2d, 0x07); // level4
        GC0310_MIPI_WriteReg(handle, 0x2e, 0x74);
        GC0310_MIPI_WriteReg(handle, 0xfe, 0x00);
        break;

    case FLICKER_60HZ:
        GC0310_MIPI_WriteReg(handle, 0xfe, 0x00);
        GC0310_MIPI_WriteReg(handle, 0x05, 0x02);
        GC0310_MIPI_WriteReg(handle, 0x06, 0x60);
        GC0310_MIPI_WriteReg(handle, 0x07, 0x00);
        GC0310_MIPI_WriteReg(handle, 0x08, 0x58);
        GC0310_MIPI_WriteReg(handle, 0xfe, 0x01);
        GC0310_MIPI_WriteReg(handle, 0x25, 0x00); // anti-flicker step [11:8]
        GC0310_MIPI_WriteReg(handle, 0x26, 0x60); // anti-flicker step [7:0]

        GC0310_MIPI_WriteReg(handle, 0x27, 0x02); // exp level 0  14.28fps
        GC0310_MIPI_WriteReg(handle, 0x28, 0x40);
        GC0310_MIPI_WriteReg(handle, 0x29, 0x03); // exp level 1  12.50fps
        GC0310_MIPI_WriteReg(handle, 0x2a, 0x60);
        GC0310_MIPI_WriteReg(handle, 0x2b, 0x06); // exp level 2  6.67fps
        GC0310_MIPI_WriteReg(handle, 0x2c, 0x00);
        GC0310_MIPI_WriteReg(handle, 0x2d, 0x08); // exp level 3  5.55fps
        GC0310_MIPI_WriteReg(handle, 0x2e, 0x40);
        GC0310_MIPI_WriteReg(handle, 0xfe, 0x00);
        break;

    default:
        break;
    }

    return 0;
}

SENSOR_REG_T GC0310_MIPI_awb_tab[][6] = {
    // Auto
    {{0x77, 0x57}, // r gain
     {0x78, 0x4d}, // g gain
     {0x79, 0x45}, // b gain
     {0x42, 0xcf}, // awb_enable[1]
     {0xff, 0xff}},
    // INCANDESCENCE:
    {{0x42, 0xcd}, // Disable AWB
     {0x77, 0x4c},
     {0x78, 0x40},
     {0x79, 0xbb}, // 7b
     {0xff, 0xff}},
    // U30
    {{0x42, 0xcd}, // Disable AWB
     {0x77, 0x40},
     {0x78, 0x54},
     {0x79, 0x70},
     {0xff, 0xff}},
    // CWF
    {{0x42, 0xcd}, // Disable AWB
     {0x77, 0x40},
     {0x78, 0x54},
     {0x79, 0x70},
     {0xff, 0xff}},
    // FLUORESCENT
    {{0x42, 0xcd}, // Disable AWB
     {0x77, 0x60}, // 40
     {0x78, 0x48},
     {0x79, 0x88}, // 50
     {0xff, 0xff}},
    // SUN
    {{0x42, 0xcd}, // Disable AWB
     {0x77, 0x80}, // 50
     {0x78, 0x40},
     {0x79, 0x48}, // 40
     {0xff, 0xff}},
    // CLOUD
    {{0x42, 0xcd}, // Disable AWB
     {0x77, 0xb0},
     {0x78, 0x40}, // 42
     {0x79, 0x40},
     {0xff, 0xff}},
};

static unsigned long set_GC0310_MIPI_awb(SENSOR_HW_HANDLE handle,
                                         unsigned long mode) {
    uint8_t awb_en_value;
    uint16_t i;

    SENSOR_REG_T *sensor_reg_ptr = (SENSOR_REG_T *)GC0310_MIPI_awb_tab[mode];

    if (mode > 6)
        return 0;

    for (i = 0; (0xFF != sensor_reg_ptr[i].reg_addr) ||
                (0xFF != sensor_reg_ptr[i].reg_value);
         i++) {
        GC0310_MIPI_WriteReg(handle, sensor_reg_ptr[i].reg_addr,
                             sensor_reg_ptr[i].reg_value);
    }

    return 0;
}

SENSOR_REG_T GC0310_MIPI_contrast_tab[][2] = {
    {
        {0xd3, 0x28}, {0xff, 0xff},
    },

    {
        {0xd3, 0x30}, {0xff, 0xff},
    },

    {
        {0xd3, 0x34}, {0xff, 0xff},
    },

    {
        {0xd3, 0x3c}, {0xff, 0xff},
    },

    {
        {0xd3, 0x44}, {0xff, 0xff},
    },

    {
        {0xd3, 0x48}, {0xff, 0xff},
    },

    {
        {0xd3, 0x50}, {0xff, 0xff},
    },
};

static unsigned long set_contrast(SENSOR_HW_HANDLE handle,
                                  unsigned long level) {
    uint16_t i;
    SENSOR_REG_T *sensor_reg_ptr;
    sensor_reg_ptr = (SENSOR_REG_T *)GC0310_MIPI_contrast_tab[level];

    if (level > 6)
        return 0;

    for (i = 0; (0xFF != sensor_reg_ptr[i].reg_addr) &&
                (0xFF != sensor_reg_ptr[i].reg_value);
         i++) {
        GC0310_MIPI_WriteReg(handle, sensor_reg_ptr[i].reg_addr,
                             sensor_reg_ptr[i].reg_value);
    }

    return 0;
}

SENSOR_REG_T GC0310_MIPI_saturation_tab[][3] = {
    {
        {0xd1, 0x10}, {0xd2, 0x10}, {0xff, 0xff},
    },

    {
        {0xd1, 0x18}, {0xd2, 0x18}, {0xff, 0xff},
    },

    {
        {0xd1, 0x20}, {0xd2, 0x20}, {0xff, 0xff},
    },

    {
        {0xd1, 0x28}, {0xd2, 0x28}, {0xff, 0xff},
    },

    {
        {0xd1, 0x40}, {0xd2, 0x40}, {0xff, 0xff},
    },

    {
        {0xd1, 0x48}, {0xd2, 0x48}, {0xff, 0xff},
    },

    {
        {0xd1, 0x50}, {0xd2, 0x50}, {0xff, 0xff},
    },
};

static unsigned long set_saturation(SENSOR_HW_HANDLE handle,
                                    unsigned long level) {
    uint16_t i;
    SENSOR_REG_T *sensor_reg_ptr;
    sensor_reg_ptr = (SENSOR_REG_T *)GC0310_MIPI_saturation_tab[level];

    if (level > 6)
        return 0;

    for (i = 0; (0xFF != sensor_reg_ptr[i].reg_addr) &&
                (0xFF != sensor_reg_ptr[i].reg_value);
         i++) {
        GC0310_MIPI_WriteReg(handle, sensor_reg_ptr[i].reg_addr,
                             sensor_reg_ptr[i].reg_value);
    }

    return 0;
}

static unsigned long set_preview_mode(SENSOR_HW_HANDLE handle,
                                      unsigned long preview_mode) {
    SENSOR_LOGI("set_preview_mode: preview_mode = %ld\n", preview_mode);

    set_GC0310_MIPI_anti_flicker(handle, 0);
    switch (preview_mode) {
    case DCAMERA_ENVIRONMENT_NORMAL:
        // YCP_saturation
        // GC0310_MIPI_WriteReg(handle,0xd1 , 0x34);
        // GC0310_MIPI_WriteReg(handle,0xd2 , 0x34);
        GC0310_MIPI_WriteReg(handle, 0xfe, 0x01);
        GC0310_MIPI_WriteReg(handle, 0x3c, 0x20);
        GC0310_MIPI_WriteReg(handle, 0xfe, 0x00);
        SENSOR_LOGI("set_preview_mode: DCAMERA_ENVIRONMENT_NORMAL\n");
        break;

    case 1: // DCAMERA_ENVIRONMENT_NIGHT:
        // YCP_saturation
        // GC0310_MIPI_WriteReg(handle,0xd1 , 0x28);
        // GC0310_MIPI_WriteReg(handle,0xd2 , 0x28);

        GC0310_MIPI_WriteReg(handle, 0xfe, 0x01);
        GC0310_MIPI_WriteReg(handle, 0x3c, 0x30);
        GC0310_MIPI_WriteReg(handle, 0xfe, 0x00);
        SENSOR_LOGI("set_preview_mode: DCAMERA_ENVIRONMENT_NIGHT\n");
        break;

    case 3: // SENSOR_ENVIROMENT_PORTRAIT:
        // YCP_saturation
        GC0310_MIPI_WriteReg(handle, 0xd1, 0x34);
        GC0310_MIPI_WriteReg(handle, 0xd2, 0x34);
        GC0310_MIPI_WriteReg(handle, 0xfe, 0x01);
        GC0310_MIPI_WriteReg(handle, 0x3c, 0x20);
        GC0310_MIPI_WriteReg(handle, 0xfe, 0x00);
        SENSOR_LOGI("set_preview_mode: SENSOR_ENVIROMENT_PORTRAIT\n");
        break;

    case 4: // SENSOR_ENVIROMENT_LANDSCAPE://4
        // nightmode disable
        GC0310_MIPI_WriteReg(handle, 0xd1, 0x4c);
        GC0310_MIPI_WriteReg(handle, 0xd2, 0x4c);

        GC0310_MIPI_WriteReg(handle, 0xfe, 0x01);
        GC0310_MIPI_WriteReg(handle, 0x3c, 0x20);
        GC0310_MIPI_WriteReg(handle, 0xfe, 0x00);
        SENSOR_LOGI("set_preview_mode: SENSOR_ENVIROMENT_LANDSCAPE\n");
        break;

    case 2: // SENSOR_ENVIROMENT_SPORTS://2
        // nightmode disable
        // YCP_saturation
        GC0310_MIPI_WriteReg(handle, 0xd1, 0x40);
        GC0310_MIPI_WriteReg(handle, 0xd2, 0x40);

        GC0310_MIPI_WriteReg(handle, 0xfe, 0x01);
        GC0310_MIPI_WriteReg(handle, 0x3c, 0x20);
        GC0310_MIPI_WriteReg(handle, 0xfe, 0x00);
        SENSOR_LOGI("set_preview_mode: SENSOR_ENVIROMENT_SPORTS\n");
        break;

    default:
        break;
    }

    SENSOR_Sleep(10);
    return 0;
}

SENSOR_REG_T GC0310_MIPI_image_effect_tab[][4] = {
    // effect normal
    {{0x43, 0x00}, {0xda, 0x00}, {0xdb, 0x00}, {0xff, 0xff}},
    // effect BLACKWHITE
    {{0x43, 0x02}, {0xda, 0x00}, {0xdb, 0x00}, {0xff, 0xff}},
    // effect RED pink
    {
        {0x43, 0x02}, {0xda, 0x10}, {0xdb, 0x50}, {0xff, 0xff},
    },
    // effect GREEN
    {
        {0x43, 0x02}, {0xda, 0xc0}, {0xdb, 0xc0}, {0xff, 0xff},
    },
    // effect  BLUE
    {{0x43, 0x02}, {0xda, 0x50}, {0xdb, 0xe0}, {0xff, 0xff}},
    // effect  YELLOW
    {{0x43, 0x02}, {0xda, 0x80}, {0xdb, 0x20}, {0xff, 0xff}},
    // effect NEGATIVE
    {{0x43, 0x01}, {0xda, 0x00}, {0xdb, 0x00}, {0xff, 0xff}},
    // effect ANTIQUE
    {{0x43, 0x02}, {0xda, 0xd2}, {0xdb, 0x28}, {0xff, 0xff}},
};

static unsigned long set_image_effect(SENSOR_HW_HANDLE handle,
                                      unsigned long effect_type) {
    uint16_t i;

    SENSOR_REG_T *sensor_reg_ptr =
        (SENSOR_REG_T *)GC0310_MIPI_image_effect_tab[effect_type];
    if (effect_type > 7)
        return 0;

    for (i = 0; (0xFF != sensor_reg_ptr[i].reg_addr) ||
                (0xFF != sensor_reg_ptr[i].reg_value);
         i++) {
        Sensor_WriteReg_8bits(sensor_reg_ptr[i].reg_addr,
                              sensor_reg_ptr[i].reg_value);
    }

    return 0;
}

static unsigned long GC0310_MIPI_After_Snapshot(SENSOR_HW_HANDLE handle,
                                                unsigned long param) {
    Sensor_SetMode((uint32_t)param);
    return SENSOR_SUCCESS;
}

static unsigned long
GC0310_MIPI_BeforeSnapshot(SENSOR_HW_HANDLE handle,
                           unsigned long sensor_snapshot_mode) {
    sensor_snapshot_mode &= 0xffff;
    Sensor_SetMode((uint32_t)sensor_snapshot_mode);
    Sensor_SetMode_WaitDone();

    switch (sensor_snapshot_mode) {
    case SENSOR_MODE_PREVIEW_ONE:
        SENSOR_LOGI("Capture VGA Size");
        break;
    case SENSOR_MODE_SNAPSHOT_ONE_FIRST:
    case SENSOR_MODE_SNAPSHOT_ONE_SECOND:
        break;
    default:
        break;
    }

    SENSOR_LOGI("SENSOR_GC0310: Before Snapshot");
    return 0;
}

static uint32_t GC0310_MIPI_StreamOn(SENSOR_HW_HANDLE handle, uint32_t param) {
    SENSOR_LOGI("Start");
    GC0310_MIPI_WriteReg(handle, 0xfe, 0x03);
    GC0310_MIPI_WriteReg(handle, 0x10, 0x94);
    GC0310_MIPI_WriteReg(handle, 0xfe, 0x00);

    return 0;
}

static uint32_t GC0310_MIPI_StreamOff(SENSOR_HW_HANDLE handle, uint32_t param) {
    SENSOR_LOGI("gx Stop 1");
    set_init(handle);
    // GC0310_MIPI_StreamOn(handle,1);
    // usleep(1000*1000);
    // get_brightness(handle);
    SENSOR_LOGI("gx Stop");
    GC0310_MIPI_WriteReg(handle, 0xfe, 0x03);
    GC0310_MIPI_WriteReg(handle, 0x10, 0x84);
    GC0310_MIPI_WriteReg(handle, 0xfe, 0x00);

    return 0;
}

static SENSOR_IOCTL_FUNC_TAB_T s_GC0310_MIPI_ioctl_func_tab = {
    // Internal
    .power = GC0310_MIPI_PowerOn,
    .identify = GC0310_MIPI_Identify,
    .get_trim = GC0310_GetResolutionTrimTab,
    .set_brightness = set_brightness,
    .set_contrast = set_contrast,
    .set_saturation = set_saturation, // set_saturation,
    .set_preview_mode = set_preview_mode,
    .set_image_effect = set_image_effect,
    .set_wb_mode = set_GC0310_MIPI_awb,
    .set_exposure_compensation = set_GC0310_MIPI_ev,
    .set_anti_banding_flicker = set_GC0310_MIPI_anti_flicker,
    .set_video_mode = set_GC0310_video_mode,
    .stream_on = GC0310_MIPI_StreamOn,
    .stream_off = GC0310_MIPI_StreamOff,
    .read_aec_info = GC0310_access_val,
    .cfg_otp = GC0310_access_val,
};
