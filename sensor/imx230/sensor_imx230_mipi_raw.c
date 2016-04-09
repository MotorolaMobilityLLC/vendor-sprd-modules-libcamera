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
#include "../vcm/vcm_dw9800.h"

#include "sensor_imx230_raw_param_v3.c"

#define DW9800_VCM_SLAVE_ADDR (0x0c)

#define SENSOR_NAME			"imx230_mipi_raw"
#define I2C_SLAVE_ADDR			0x20    /* 16bit slave address*/


#define BINNING_FACTOR 			2
#define imx230_PID_ADDR			0x0016
#define imx230_PID_VALUE		0x02
#define imx230_VER_ADDR			0x0017
#define imx230_VER_VALUE		0x30


/* sensor parameters begin */
/* effective sensor output image size */
#define SNAPSHOT_WIDTH			5344
#define SNAPSHOT_HEIGHT			4016
#define PREVIEW_WIDTH			2672
#define PREVIEW_HEIGHT			2008

/*Mipi output*/
#define LANE_NUM			4
#define RAW_BITS			10

#define SNAPSHOT_MIPI_PER_LANE_BPS	1419
#define PREVIEW_MIPI_PER_LANE_BPS	800

/* please ref your spec */
#define FRAME_OFFSET			5
#define SENSOR_MAX_GAIN			0xF0
#define SENSOR_BASE_GAIN		0x20
#define SENSOR_MIN_SHUTTER		4

/* isp parameters, please don't change it*/
#if defined(CONFIG_CAMERA_ISP_VERSION_V3) || defined(CONFIG_CAMERA_ISP_VERSION_V4)
#define ISP_BASE_GAIN			0x80
#else
#define ISP_BASE_GAIN			0x10
#endif

/* please don't change it */
#define EX_MCLK				24

struct hdr_info_t {
	uint32_t capture_max_shutter;
	uint32_t capture_shutter;
	uint32_t capture_gain;
};

struct sensor_ev_info_t {
	uint16_t preview_shutter;
	uint16_t preview_gain;
};

static unsigned long imx230_access_val(unsigned long param);
static uint32_t imx230_init_mode_fps_info();

/*==============================================================================
 * Description:
 * global variable
 *============================================================================*/
static struct hdr_info_t s_hdr_info;
static uint32_t s_current_default_frame_length;
struct sensor_ev_info_t s_sensor_ev_info;

static SENSOR_IOCTL_FUNC_TAB_T s_imx230_ioctl_func_tab;
struct sensor_raw_info *s_imx230_mipi_raw_info_ptr = &s_imx230_mipi_raw_info;

static const SENSOR_REG_T imx230_init_setting[] = {
	{0x0136, 0x18},
	{0x0137, 0x00},
	{0x4800, 0x0E},
	{0x4890, 0x01},
	{0x4D1E, 0x01},
	{0x4D1F, 0xFF},
	{0x4FA0, 0x00},
	{0x4FA1, 0x00},
	{0x4FA2, 0x00},
	{0x4FA3, 0x83},
	{0x6153, 0x01},
	{0x6156, 0x01},
	{0x69BB, 0x01},
	{0x69BC, 0x05},
	{0x69BD, 0x05},
	{0x69C1, 0x00},
	{0x69C4, 0x01},
	{0x69C6, 0x01},
	{0x7300, 0x00},
	{0x9009, 0x1A},
	{0xB040, 0x90},
	{0xB041, 0x14},
	{0xB042, 0x6B},
	{0xB043, 0x43},
	{0xB044, 0x63},
	{0xB045, 0x2A},
	{0xB046, 0x68},
	{0xB047, 0x06},
	{0xB048, 0x68},
	{0xB049, 0x07},
	{0xB04A, 0x68},
	{0xB04B, 0x04},
	{0xB04C, 0x68},
	{0xB04D, 0x05},
	{0xB04E, 0x68},
	{0xB04F, 0x16},
	{0xB050, 0x68},
	{0xB051, 0x17},
	{0xB052, 0x68},
	{0xB053, 0x74},
	{0xB054, 0x68},
	{0xB055, 0x75},
	{0xB056, 0x68},
	{0xB057, 0x76},
	{0xB058, 0x68},
	{0xB059, 0x77},
	{0xB05A, 0x68},
	{0xB05B, 0x7A},
	{0xB05C, 0x68},
	{0xB05D, 0x7B},
	{0xB05E, 0x68},
	{0xB05F, 0x0A},
	{0xB060, 0x68},
	{0xB061, 0x0B},
	{0xB062, 0x68},
	{0xB063, 0x08},
	{0xB064, 0x68},
	{0xB065, 0x09},
	{0xB066, 0x68},
	{0xB067, 0x0E},
	{0xB068, 0x68},
	{0xB069, 0x0F},
	{0xB06A, 0x68},
	{0xB06B, 0x0C},
	{0xB06C, 0x68},
	{0xB06D, 0x0D},
	{0xB06E, 0x68},
	{0xB06F, 0x13},
	{0xB070, 0x68},
	{0xB071, 0x12},
	{0xB072, 0x90},
	{0xB073, 0x0E},
	{0xD000, 0xDA},
	{0xD001, 0xDA},
	{0xD002, 0xAF},
	{0xD003, 0xE1},
	{0xD004, 0x55},
	{0xD005, 0x34},
	{0xD006, 0x21},
	{0xD007, 0x00},
	{0xD008, 0x1C},
	{0xD009, 0x80},
	{0xD00A, 0xFE},
	{0xD00B, 0xC5},
	{0xD00C, 0x55},
	{0xD00D, 0xDC},
	{0xD00E, 0xB6},
	{0xD00F, 0x00},
	{0xD010, 0x31},
	{0xD011, 0x02},
	{0xD012, 0x4A},
	{0xD013, 0x0E},
	{0xD014, 0x55},
	{0xD015, 0xF0},
	{0xD016, 0x1B},
	{0xD017, 0x00},
	{0xD018, 0xFA},
	{0xD019, 0x2C},
	{0xD01A, 0xF1},
	{0xD01B, 0x7E},
	{0xD01C, 0x55},
	{0xD01D, 0x1C},
	{0xD01E, 0xD8},
	{0xD01F, 0x00},
	{0xD020, 0x76},
	{0xD021, 0xC1},
	{0xD022, 0xBF},
	{0xD044, 0x40},
	{0xD045, 0xBA},
	{0xD046, 0x70},
	{0xD047, 0x47},
	{0xD048, 0xC0},
	{0xD049, 0xBA},
	{0xD04A, 0x70},
	{0xD04B, 0x47},
	{0xD04C, 0x82},
	{0xD04D, 0xF6},
	{0xD04E, 0xDA},
	{0xD04F, 0xFA},
	{0xD050, 0x00},
	{0xD051, 0xF0},
	{0xD052, 0x02},
	{0xD053, 0xF8},
	{0xD054, 0x81},
	{0xD055, 0xF6},
	{0xD056, 0xCE},
	{0xD057, 0xFD},
	{0xD058, 0x10},
	{0xD059, 0xB5},
	{0xD05A, 0x0D},
	{0xD05B, 0x48},
	{0xD05C, 0x40},
	{0xD05D, 0x7A},
	{0xD05E, 0x01},
	{0xD05F, 0x28},
	{0xD060, 0x15},
	{0xD061, 0xD1},
	{0xD062, 0x0C},
	{0xD063, 0x49},
	{0xD064, 0x0C},
	{0xD065, 0x46},
	{0xD066, 0x40},
	{0xD067, 0x3C},
	{0xD068, 0x48},
	{0xD069, 0x8A},
	{0xD06A, 0x62},
	{0xD06B, 0x8A},
	{0xD06C, 0x80},
	{0xD06D, 0x1A},
	{0xD06E, 0x8A},
	{0xD06F, 0x89},
	{0xD070, 0x00},
	{0xD071, 0xB2},
	{0xD072, 0x10},
	{0xD073, 0x18},
	{0xD074, 0x0A},
	{0xD075, 0x46},
	{0xD076, 0x20},
	{0xD077, 0x32},
	{0xD078, 0x12},
	{0xD079, 0x88},
	{0xD07A, 0x90},
	{0xD07B, 0x42},
	{0xD07C, 0x00},
	{0xD07D, 0xDA},
	{0xD07E, 0x10},
	{0xD07F, 0x46},
	{0xD080, 0x80},
	{0xD081, 0xB2},
	{0xD082, 0x88},
	{0xD083, 0x81},
	{0xD084, 0x84},
	{0xD085, 0xF6},
	{0xD086, 0x06},
	{0xD087, 0xF8},
	{0xD088, 0xE0},
	{0xD089, 0x67},
	{0xD08A, 0x85},
	{0xD08B, 0xF6},
	{0xD08C, 0x4B},
	{0xD08D, 0xFC},
	{0xD08E, 0x10},
	{0xD08F, 0xBD},
	{0xD090, 0x00},
	{0xD091, 0x18},
	{0xD092, 0x1E},
	{0xD093, 0x78},
	{0xD094, 0x00},
	{0xD095, 0x18},
	{0xD096, 0x17},
	{0xD097, 0x98},
	{0x5869, 0x01},

	{0x68A9, 0x00},
	{0x68C5, 0x00},
	{0x68DF, 0x00},
	{0x6953, 0x01},
	{0x6962, 0x3A},
	{0x69CD, 0x3A},
	{0x9258, 0x00},
	{0x933A, 0x02},
	{0x933B, 0x02},
	{0x934B, 0x1B},
	{0x934C, 0x0A},
	{0x9356, 0x8C},
	{0x9357, 0x50},
	{0x9358, 0x1B},
	{0x9359, 0x8C},
	{0x935A, 0x1B},
	{0x935B, 0x0A},
	{0x940D, 0x07},
	{0x940E, 0x07},
	{0x9414, 0x06},
	{0x945B, 0x07},
	{0x945D, 0x07},
	{0x9901, 0x35},
	{0x9903, 0x23},
	{0x9905, 0x23},
	{0x9906, 0x00},
	{0x9907, 0x31},
	{0x9908, 0x00},
	{0x9909, 0x1B},
	{0x990A, 0x00},
	{0x990B, 0x15},
	{0x990D, 0x3F},
	{0x990F, 0x3F},
	{0x9911, 0x3F},
	{0x9913, 0x64},
	{0x9915, 0x64},
	{0x9917, 0x64},
	{0x9919, 0x50},
	{0x991B, 0x60},
	{0x991D, 0x65},
	{0x991F, 0x01},
	{0x9921, 0x01},
	{0x9923, 0x01},
	{0x9925, 0x23},
	{0x9927, 0x23},
	{0x9929, 0x23},
	{0x992B, 0x2F},
	{0x992D, 0x1A},
	{0x992F, 0x14},
	{0x9931, 0x3F},
	{0x9933, 0x3F},
	{0x9935, 0x3F},
	{0x9937, 0x6B},
	{0x9939, 0x7C},
	{0x993B, 0x81},
	{0x9943, 0x0F},
	{0x9945, 0x0F},
	{0x9947, 0x0F},
	{0x9949, 0x0F},
	{0x994B, 0x0F},
	{0x994D, 0x0F},
	{0x994F, 0x42},
	{0x9951, 0x0F},
	{0x9953, 0x0B},
	{0x9955, 0x5A},
	{0x9957, 0x13},
	{0x9959, 0x0C},
	{0x995A, 0x00},
	{0x995B, 0x00},
	{0x995C, 0x00},
	{0x996B, 0x00},
	{0x996D, 0x10},
	{0x996F, 0x10},
	{0x9971, 0xC8},
	{0x9973, 0x32},
	{0x9975, 0x04},
	{0x9976, 0x0A},
	{0x99B0, 0x20},
	{0x99B1, 0x20},
	{0x99B2, 0x20},
	{0x99C6, 0x6E},
	{0x99C7, 0x6E},
	{0x99C8, 0x6E},
	{0x9A1F, 0x0A},
	{0x9AB0, 0x20},
	{0x9AB1, 0x20},
	{0x9AB2, 0x20},
	{0x9AC6, 0x6E},
	{0x9AC7, 0x6E},
	{0x9AC8, 0x6E},
	{0x9B01, 0x35},
	{0x9B03, 0x14},
	{0x9B05, 0x14},
	{0x9B07, 0x31},
	{0x9B08, 0x01},
	{0x9B09, 0x1B},
	{0x9B0A, 0x01},
	{0x9B0B, 0x15},
	{0x9B0D, 0x1E},
	{0x9B0F, 0x1E},
	{0x9B11, 0x1E},
	{0x9B13, 0x64},
	{0x9B15, 0x64},
	{0x9B17, 0x64},
	{0x9B19, 0x50},
	{0x9B1B, 0x60},
	{0x9B1D, 0x65},
	{0x9B1F, 0x01},
	{0x9B21, 0x01},
	{0x9B23, 0x01},
	{0x9B25, 0x14},
	{0x9B27, 0x14},
	{0x9B29, 0x14},
	{0x9B2B, 0x2F},
	{0x9B2D, 0x1A},
	{0x9B2F, 0x14},
	{0x9B31, 0x1E},
	{0x9B33, 0x1E},
	{0x9B35, 0x1E},
	{0x9B37, 0x6B},
	{0x9B39, 0x7C},
	{0x9B3B, 0x81},
	{0x9B43, 0x0F},
	{0x9B45, 0x0F},
	{0x9B47, 0x0F},
	{0x9B49, 0x0F},
	{0x9B4B, 0x0F},
	{0x9B4D, 0x0F},
	{0x9B4F, 0x2D},
	{0x9B51, 0x0B},
	{0x9B53, 0x08},
	{0x9B55, 0x40},
	{0x9B57, 0x0D},
	{0x9B59, 0x08},
	{0x9B5A, 0x00},
	{0x9B5B, 0x00},
	{0x9B5C, 0x00},
	{0x9B5D, 0x08},
	{0x9B5E, 0x0E},
	{0x9B60, 0x08},
	{0x9B61, 0x0E},
	{0x9B6B, 0x00},
	{0x9B6D, 0x10},
	{0x9B6F, 0x10},
	{0x9B71, 0xC8},
	{0x9B73, 0x32},
	{0x9B75, 0x04},
	{0x9B76, 0x0A},
	{0x9BB0, 0x20},
	{0x9BB1, 0x20},
	{0x9BB2, 0x20},
	{0x9BC6, 0x6E},
	{0x9BC7, 0x6E},
	{0x9BC8, 0x6E},
	{0x9BCC, 0x20},
	{0x9BCD, 0x20},
	{0x9BCE, 0x20},
	{0x9C01, 0x10},
	{0x9C03, 0x1D},
	{0x9C05, 0x20},
	{0x9C13, 0x10},
	{0x9C15, 0x10},
	{0x9C17, 0x10},
	{0x9C19, 0x04},
	{0x9C1B, 0x67},
	{0x9C1D, 0x80},
	{0x9C1F, 0x0A},
	{0x9C21, 0x29},
	{0x9C23, 0x32},
	{0x9C27, 0x56},
	{0x9C29, 0x60},
	{0x9C39, 0x67},
	{0x9C3B, 0x80},
	{0x9C3D, 0x80},
	{0x9C3F, 0x80},
	{0x9C41, 0x80},
	{0x9C55, 0xC8},
	{0x9C57, 0xC8},
	{0x9C59, 0xC8},
	{0x9C87, 0x48},
	{0x9C89, 0x48},
	{0x9C8B, 0x48},
	{0x9CB0, 0x20},
	{0x9CB1, 0x20},
	{0x9CB2, 0x20},
	{0x9CC6, 0x6E},
	{0x9CC7, 0x6E},
	{0x9CC8, 0x6E},
	{0x9D13, 0x10},
	{0x9D15, 0x10},
	{0x9D17, 0x10},
	{0x9D19, 0x04},
	{0x9D1B, 0x67},
	{0x9D1F, 0x0A},
	{0x9D21, 0x29},
	{0x9D23, 0x32},
	{0x9D55, 0xC8},
	{0x9D57, 0xC8},
	{0x9D59, 0xC8},
	{0x9D91, 0x20},
	{0x9D93, 0x20},
	{0x9D95, 0x20},
	{0x9E01, 0x10},
	{0x9E03, 0x1D},
	{0x9E13, 0x10},
	{0x9E15, 0x10},
	{0x9E17, 0x10},
	{0x9E19, 0x04},
	{0x9E1B, 0x67},
	{0x9E1D, 0x80},
	{0x9E1F, 0x0A},
	{0x9E21, 0x29},
	{0x9E23, 0x32},
	{0x9E25, 0x30},
	{0x9E27, 0x56},
	{0x9E29, 0x60},
	{0x9E39, 0x67},
	{0x9E3B, 0x80},
	{0x9E3D, 0x80},
	{0x9E3F, 0x80},
	{0x9E41, 0x80},
	{0x9E55, 0xC8},
	{0x9E57, 0xC8},
	{0x9E59, 0xC8},
	{0x9E91, 0x20},
	{0x9E93, 0x20},
	{0x9E95, 0x20},
	{0x9F8F, 0xA0},
	{0xA027, 0x67},
	{0xA029, 0x80},
	{0xA02D, 0x67},
	{0xA02F, 0x80},
	{0xA031, 0x80},
	{0xA033, 0x80},
	{0xA035, 0x80},
	{0xA037, 0x80},
	{0xA039, 0x80},
	{0xA03B, 0x80},
	{0xA067, 0x20},
	{0xA068, 0x20},
	{0xA069, 0x20},
	{0xA071, 0x48},
	{0xA073, 0x48},
	{0xA075, 0x48},
	{0xA08F, 0xA0},
	{0xA091, 0x3A},
	{0xA093, 0x3A},
	{0xA095, 0x0A},
	{0xA097, 0x0A},
	{0xA099, 0x0A},
	{0x9012, 0x03},
	{0x9098, 0x1A},
	{0x9099, 0x04},
	{0x909A, 0x20},
	{0x909B, 0x20},
	{0x909C, 0x13},
	{0x909D, 0x13},
	{0xA716, 0x13},
	{0xA801, 0x08},
	{0xA803, 0x0C},
	{0xA805, 0x10},
	{0xA806, 0x00},
	{0xA807, 0x18},
	{0xA808, 0x00},
	{0xA809, 0x20},
	{0xA80A, 0x00},
	{0xA80B, 0x30},
	{0xA80C, 0x00},
	{0xA80D, 0x40},
	{0xA80E, 0x00},
	{0xA80F, 0x60},
	{0xA810, 0x00},
	{0xA811, 0x80},
	{0xA812, 0x00},
	{0xA813, 0xC0},
	{0xA814, 0x01},
	{0xA815, 0x00},
	{0xA816, 0x01},
	{0xA817, 0x80},
	{0xA818, 0x02},
	{0xA819, 0x00},
	{0xA81A, 0x03},
	{0xA81B, 0x00},
	{0xA81C, 0x03},
	{0xA81D, 0xAC},
	{0xA838, 0x03},
	{0xA83C, 0x28},
	{0xA83D, 0x5F},
	{0xA881, 0x08},
	{0xA883, 0x0C},
	{0xA885, 0x10},
	{0xA886, 0x00},
	{0xA887, 0x18},
	{0xA888, 0x00},
	{0xA889, 0x20},
	{0xA88A, 0x00},
	{0xA88B, 0x30},
	{0xA88C, 0x00},
	{0xA88D, 0x40},
	{0xA88E, 0x00},
	{0xA88F, 0x60},
	{0xA890, 0x00},
	{0xA891, 0x80},
	{0xA892, 0x00},
	{0xA893, 0xC0},
	{0xA894, 0x01},
	{0xA895, 0x00},
	{0xA896, 0x01},
	{0xA897, 0x80},
	{0xA898, 0x02},
	{0xA899, 0x00},
	{0xA89A, 0x03},
	{0xA89B, 0x00},
	{0xA89C, 0x03},
	{0xA89D, 0xAC},
	{0xA8B8, 0x03},
	{0xA8BB, 0x13},
	{0xA8BC, 0x28},
	{0xA8BD, 0x25},
	{0xA8BE, 0x1D},
	{0xA8C0, 0x3A},
	{0xA8C1, 0xE0},
	{0xB24F, 0x80},
	{0x3198, 0x0F},
	{0x31A0, 0x04},
	{0x31A1, 0x03},
	{0x31A2, 0x02},
	{0x31A3, 0x01},
	{0x31A8, 0x18},
	{0x822C, 0x01},
	{0x8239, 0x01},
	{0x9503, 0x07},
	{0x9504, 0x07},
	{0x9505, 0x07},
	{0x9506, 0x00},
	{0x9507, 0x00},
	{0x9508, 0x00},
	{0x9526, 0x18},
	{0x9527, 0x18},
	{0x9528, 0x18},
	{0x8858, 0x00},
	{0x6B42, 0x40},
	{0x6B46, 0x00},
	{0x6B47, 0x4B},
	{0x6B4A, 0x00},
	{0x6B4B, 0x4B},
	{0x6B4E, 0x00},
	{0x6B4F, 0x4B},
	{0x6B44, 0x00},
	{0x6B45, 0x8C},
	{0x6B48, 0x00},
	{0x6B49, 0x8C},
	{0x6B4C, 0x00},
	{0x6B4D, 0x8C},
	{0x5041, 0x00},
};

static const SENSOR_REG_T imx230_5344x4016_setting[] = {
	{0x0114, 0x03},
	{0x0220, 0x00},
	{0x0221, 0x11},
	{0x0222, 0x01},
	{0x0340, 0x10},
	{0x0341, 0x2C},
	{0x0342, 0x17},
	{0x0343, 0x88},
	{0x0344, 0x00},
	{0x0345, 0x00},
	{0x0346, 0x00},
	{0x0347, 0x00},
	{0x0348, 0x14},
	{0x0349, 0xDF},
	{0x034A, 0x0F},
	{0x034B, 0xAF},
	{0x0381, 0x01},
	{0x0383, 0x01},
	{0x0385, 0x01},
	{0x0387, 0x01},
	{0x0900, 0x00},
	{0x0901, 0x11},
	{0x0902, 0x00},
	{0x3000, 0x74},
	{0x3001, 0x00},
	{0x305C, 0x11},
	{0x0112, 0x0A},
	{0x0113, 0x0A},
	{0x034C, 0x14},
	{0x034D, 0xE0},
	{0x034E, 0x0F},
	{0x034F, 0xB0},
	{0x0401, 0x00},
	{0x0404, 0x00},
	{0x0405, 0x10},
	{0x0408, 0x00},
	{0x0409, 0x00},
	{0x040A, 0x00},
	{0x040B, 0x00},
	{0x040C, 0x14},
	{0x040D, 0xE0},
	{0x040E, 0x0F},
	{0x040F, 0xB0},
	{0x0301, 0x04},
	{0x0303, 0x02},
	{0x0305, 0x04},
	{0x0306, 0x00},
	{0x0307, 0xC8},
	{0x0309, 0x0A},
	{0x030B, 0x01},
	{0x030D, 0x0F},
	{0x030E, 0x03},
	{0x030F, 0x77},
	{0x0310, 0x01},
	{0x0820, 0x16},
	{0x0821, 0x2C},
	{0x0822, 0xCC},
	{0x0823, 0xCC},
	{0x0202, 0x10},
	{0x0203, 0x22},
	{0x0224, 0x01},
	{0x0225, 0xF4},
	{0x0204, 0x00},
	{0x0205, 0x00},
	{0x0216, 0x00},
	{0x0217, 0x00},
	{0x020E, 0x01},
	{0x020F, 0x00},
	{0x0210, 0x01},
	{0x0211, 0x00},
	{0x0212, 0x01},
	{0x0213, 0x00},
	{0x0214, 0x01},
	{0x0215, 0x00},
	{0x3006, 0x01},
	{0x3007, 0x02},
	{0x31E0, 0x03},
	{0x31E1, 0xFF},
	{0x31E4, 0x02},
	{0x3A22, 0x20},
	{0x3A23, 0x14},
	{0x3A24, 0xE0},
	{0x3A25, 0x0F},
	{0x3A26, 0xB0},
	{0x3A2F, 0x00},
	{0x3A30, 0x00},
	{0x3A31, 0x00},
	{0x3A32, 0x00},
	{0x3A33, 0x14},
	{0x3A34, 0xDF},
	{0x3A35, 0x0F},
	{0x3A36, 0xAF},
	{0x3A37, 0x00},
	{0x3A38, 0x00},
	{0x3A39, 0x00},
	{0x3A21, 0x00},
	{0x3011, 0x00},
	{0x3013, 0x01},
};

static const SENSOR_REG_T imx230_2672x2008_setting[] = {
	{0x0114, 0x03},
	{0x0220, 0x00},
	{0x0221, 0x11},
	{0x0222, 0x01},
	{0x0340, 0x09},
	{0x0341, 0x10},
	{0x0342, 0x17},
	{0x0343, 0x88},
	{0x0344, 0x00},
	{0x0345, 0x00},
	{0x0346, 0x00},
	{0x0347, 0x00},
	{0x0348, 0x14},
	{0x0349, 0xDF},
	{0x034A, 0x0F},
	{0x034B, 0xAF},
	{0x0381, 0x01},
	{0x0383, 0x01},
	{0x0385, 0x01},
	{0x0387, 0x01},
	{0x0900, 0x01},
	{0x0901, 0x22},
	{0x0902, 0x02},
	{0x3000, 0x74},
	{0x3001, 0x00},
	{0x305C, 0x11},
	{0x0112, 0x0A},
	{0x0113, 0x0A},
	{0x034C, 0x0A},
	{0x034D, 0x70},
	{0x034E, 0x07},
	{0x034F, 0xD8},
	{0x0401, 0x00},
	{0x0404, 0x00},
	{0x0405, 0x10},
	{0x0408, 0x00},
	{0x0409, 0x00},
	{0x040A, 0x00},
	{0x040B, 0x00},
	{0x040C, 0x0A},
	{0x040D, 0x70},
	{0x040E, 0x07},
	{0x040F, 0xD8},
	{0x0301, 0x04},
	{0x0303, 0x02},
	{0x0305, 0x04},
	{0x0306, 0x00},
	{0x0307, 0x8C},
	{0x0309, 0x0A},
	{0x030B, 0x01},
	{0x030D, 0x0F},
	{0x030E, 0x01},
	{0x030F, 0xF4},
	{0x0310, 0x01},
	{0x0820, 0x0C},
	{0x0821, 0x80},
	{0x0822, 0x00},
	{0x0823, 0x00},
	{0x0202, 0x09},
	{0x0203, 0x06},
	{0x0224, 0x01},
	{0x0225, 0xF4},
	{0x0204, 0x00},
	{0x0205, 0x00},
	{0x0216, 0x00},
	{0x0217, 0x00},
	{0x020E, 0x01},
	{0x020F, 0x00},
	{0x0210, 0x01},
	{0x0211, 0x00},
	{0x0212, 0x01},
	{0x0213, 0x00},
	{0x0214, 0x01},
	{0x0215, 0x00},
	{0x3006, 0x01},
	{0x3007, 0x02},
	{0x31E0, 0x03},
	{0x31E1, 0xFF},
	{0x31E4, 0x02},
	{0x3A22, 0x20},
	{0x3A23, 0x14},
	{0x3A24, 0xE0},
	{0x3A25, 0x07},
	{0x3A26, 0xD8},
	{0x3A2F, 0x00},
	{0x3A30, 0x00},
	{0x3A31, 0x00},
	{0x3A32, 0x00},
	{0x3A33, 0x14},
	{0x3A34, 0xDF},
	{0x3A35, 0x0F},
	{0x3A36, 0xAF},
	{0x3A37, 0x00},
	{0x3A38, 0x01},
	{0x3A39, 0x00},
	{0x3A21, 0x00},
	{0x3011, 0x00},
	{0x3013, 0x01},
};

/**
 line time: 10us
 MipiData Rate= 400Mbps/Lane
 Frame length lines = 828 line
 */
static const SENSOR_REG_T imx230_1280x720_setting[] = {
	{0x0114, 0x03},
	{0x0220, 0x00},
	{0x0221, 0x11},
	{0x0222, 0x01},
	{0x0340, 0x03},
	{0x0341, 0x3C},
	{0x0342, 0x17},
	{0x0343, 0x88},
	{0x0344, 0x00},
	{0x0345, 0x00},
	{0x0346, 0x02},
	{0x0347, 0x38},
	{0x0348, 0x14},
	{0x0349, 0xDF},
	{0x034A, 0x0D},
	{0x034B, 0x77},
	{0x0381, 0x01},
	{0x0383, 0x01},
	{0x0385, 0x01},
	{0x0387, 0x01},
	{0x0900, 0x01},
	{0x0901, 0x44},
	{0x0902, 0x02},
	{0x3000, 0x74},
	{0x3001, 0x00},
	{0x305C, 0x11},
	{0x0112, 0x0A},
	{0x0113, 0x0A},
	{0x034C, 0x05},
	{0x034D, 0x00},
	{0x034E, 0x02},
	{0x034F, 0xD0},
	{0x0401, 0x02},
	{0x0404, 0x00},
	{0x0405, 0x10},
	{0x0408, 0x00},
	{0x0409, 0x1C},
	{0x040A, 0x00},
	{0x040B, 0x00},
	{0x040C, 0x05},
	{0x040D, 0x00},
	{0x040E, 0x02},
	{0x040F, 0xD0},
	{0x0301, 0x04},
	{0x0303, 0x02},
	{0x0305, 0x04},
	{0x0306, 0x00},
	{0x0307, 0xC8},
	{0x0309, 0x0A},
	{0x030B, 0x01},
	{0x030D, 0x0F},
	{0x030E, 0x00},
	{0x030F, 0xFA},
	{0x0310, 0x01},
	{0x0820, 0x06},
	{0x0821, 0x40},
	{0x0822, 0x00},
	{0x0823, 0x00},
	{0x0202, 0x03},
	{0x0203, 0x32},
	{0x0224, 0x01},
	{0x0225, 0xF4},
	{0x0204, 0x00},
	{0x0205, 0x00},
	{0x0216, 0x00},
	{0x0217, 0x00},
	{0x020E, 0x01},
	{0x020F, 0x00},
	{0x0210, 0x01},
	{0x0211, 0x00},
	{0x0212, 0x01},
	{0x0213, 0x00},
	{0x0214, 0x01},
	{0x0215, 0x00},
	{0x3006, 0x01},
	{0x3007, 0x02},
	{0x31E0, 0x03},
	{0x31E1, 0xFF},
	{0x31E4, 0x02},
	{0x3A22, 0x20},
	{0x3A23, 0x14},
	{0x3A24, 0xE0},
	{0x3A25, 0x02},
	{0x3A26, 0xD0},
	{0x3A2F, 0x00},
	{0x3A30, 0x00},
	{0x3A31, 0x02},
	{0x3A32, 0x38},
	{0x3A33, 0x14},
	{0x3A34, 0xDF},
	{0x3A35, 0x0D},
	{0x3A36, 0x77},
	{0x3A37, 0x00},
	{0x3A38, 0x02},
	{0x3A39, 0x00},
	{0x3A21, 0x00},
	{0x3011, 0x00},
	{0x3013, 0x01}
};

static uint16_t sensorGainMapping[159][2] = {
	{64 , 1  },
	{65 , 8  },
	{66 , 13 },
	{67 , 23 },
	{68 , 27 },
	{69 , 36 },
	{70 , 41 },
	{71 , 49 },
	{72 , 53 },
	{73 , 61 },
	{74 , 69 },
	{75 , 73 },
	{76 , 80 },
	{77 , 88 },
	{78 , 91 },
	{79 , 98 },
	{80 , 101},
	{81 , 108},
	{82 , 111},
	{83 , 117},
	{84 , 120},
	{85 , 126},
	{86 , 132},
	{87 , 135},
	{88 , 140},
	{89 , 143},
	{90 , 148},
	{91 , 151},
	{92 , 156},
	{93 , 161},
	{94 , 163},
	{95 , 168},
	{96 , 170},
	{97 , 175},
	{98 , 177},
	{99 , 181},
	{100, 185},
	{101, 187},
	{102, 191},
	{103, 193},
	{104, 197},
	{105, 199},
	{106, 203},
	{107, 205},
	{108, 207},
	{109, 212},
	{110, 214},
	{111, 217},
	{112, 219},
	{113, 222},
	{114, 224},
	{115, 227},
	{116, 230},
	{117, 232},
	{118, 234},
	{119, 236},
	{120, 239},
	{122, 244},
	{123, 245},
	{124, 248},
	{125, 249},
	{126, 252},
	{127, 253},
	{128, 256},
	{129, 258},
	{130, 260},
	{131, 262},
	{132, 263},
	{133, 266},
	{134, 268},
	{136, 272},
	{138, 274},
	{139, 276},
	{140, 278},
	{141, 280},
	{143, 282},
	{144, 284},
	{145, 286},
	{147, 288},
	{148, 290},
	{149, 292},
	{150, 294},
	{152, 296},
	{153, 298},
	{155, 300},
	{156, 302},
	{157, 304},
	{159, 306},
	{161, 308},
	{162, 310},
	{164, 312},
	{166, 314},
	{167, 316},
	{169, 318},
	{171, 320},
	{172, 322},
	{174, 324},
	{176, 326},
	{178, 328},
	{180, 330},
	{182, 332},
	{184, 334},
	{186, 336},
	{188, 338},
	{191, 340},
	{193, 342},
	{195, 344},
	{197, 346},
	{200, 348},
	{202, 350},
	{205, 352},
	{207, 354},
	{210, 356},
	{212, 358},
	{216, 360},
	{218, 362},
	{221, 364},
	{225, 366},
	{228, 368},
	{231, 370},
	{234, 372},
	{237, 374},
	{241, 376},
	{244, 378},
	{248, 380},
	{252, 382},
	{256, 384},
	{260, 386},
	{264, 388},
	{269, 390},
	{273, 392},
	{278, 394},
	{282, 396},
	{287, 398},
	{292, 400},
	{298, 402},
	{303, 404},
	{309, 406},
	{315, 408},
	{321, 410},
	{328, 412},
	{334, 414},
	{341, 416},
	{349, 418},
	{356, 420},
	{364, 422},
	{372, 424},
	{381, 426},
	{390, 428},
	{399, 430},
	{410, 432},
	{420, 434},
	{431, 436},
	{443, 438},
	{455, 440},
	{468, 442},
	{482, 444},
	{497, 446},
	{512, 448}
};

static SENSOR_REG_TAB_INFO_T s_imx230_resolution_tab_raw[SENSOR_MODE_MAX] = {
	{ADDR_AND_LEN_OF_ARRAY(imx230_init_setting), 0, 0, EX_MCLK, SENSOR_IMAGE_FORMAT_RAW},
//	{ADDR_AND_LEN_OF_ARRAY(imx230_1280x720_setting), 1280, 720, EX_MCLK, SENSOR_IMAGE_FORMAT_RAW},
//	{ADDR_AND_LEN_OF_ARRAY(imx230_2672x2008_setting), 2672, 2008, EX_MCLK, SENSOR_IMAGE_FORMAT_RAW},
	{ADDR_AND_LEN_OF_ARRAY(imx230_5344x4016_setting), 5344, 4016, EX_MCLK, SENSOR_IMAGE_FORMAT_RAW},
};

static SENSOR_TRIM_T s_imx230_resolution_trim_tab[SENSOR_MODE_MAX] = {
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
//	{0, 0, 1280, 720, 100 , PREVIEW_MIPI_PER_LANE_BPS, 828, {0, 0, 1280, 720}},
//	{0, 0, 2672, 2008, 144 , PREVIEW_MIPI_PER_LANE_BPS, 2320, {0, 0, 2672, 2008}},
	{0, 0, 5344, 4016, 5008, SNAPSHOT_MIPI_PER_LANE_BPS, 4140, {0, 0, 5344, 4016}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}}
};

static const SENSOR_REG_T s_imx230_preview_size_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
	/*video mode 0: ?fps */
	{
	 {0xffff, 0xff}
	 },
	/* video mode 1:?fps */
	{
	 {0xffff, 0xff}
	 },
	/* video mode 2:?fps */
	{
	 {0xffff, 0xff}
	 },
	/* video mode 3:?fps */
	{
	 {0xffff, 0xff}
	 }
};

static const SENSOR_REG_T s_imx230_capture_size_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
	/*video mode 0: ?fps */
	{
	 {0xffff, 0xff}
	 },
	/* video mode 1:?fps */
	{
	 {0xffff, 0xff}
	 },
	/* video mode 2:?fps */
	{
	 {0xffff, 0xff}
	 },
	/* video mode 3:?fps */
	{
	 {0xffff, 0xff}
	 }
};

static SENSOR_VIDEO_INFO_T s_imx230_video_info[SENSOR_MODE_MAX] = {
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{30, 30, 270, 90}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
	 (SENSOR_REG_T **) s_imx230_preview_size_video_tab},
	{{{2, 5, 338, 1000}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
	 (SENSOR_REG_T **) s_imx230_capture_size_video_tab},
};

/*==============================================================================
 * Description:
 * set video mode
 *
 *============================================================================*/
static uint32_t imx230_set_video_mode(uint32_t param)
{
	SENSOR_REG_T_PTR sensor_reg_ptr;
	uint16_t i = 0x00;
	uint32_t mode;

	if (param >= SENSOR_VIDEO_MODE_MAX)
		return 0;

	if (SENSOR_SUCCESS != Sensor_GetMode(&mode)) {
		SENSOR_PRINT("fail.");
		return SENSOR_FAIL;
	}

	if (PNULL == s_imx230_video_info[mode].setting_ptr) {
		SENSOR_PRINT("fail.");
		return SENSOR_FAIL;
	}

	sensor_reg_ptr = (SENSOR_REG_T_PTR) & s_imx230_video_info[mode].setting_ptr[param];
	if (PNULL == sensor_reg_ptr) {
		SENSOR_PRINT("fail.");
		return SENSOR_FAIL;
	}

	for (i = 0x00; (0xffff != sensor_reg_ptr[i].reg_addr)
	     || (0xff != sensor_reg_ptr[i].reg_value); i++) {
		Sensor_WriteReg(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
	}

	return 0;
}

/*==============================================================================
 * Description:
 * sensor static info need by isp
 * please modify this variable acording your spec
 *============================================================================*/
static SENSOR_STATIC_INFO_T s_imx230_static_info = {
	220,	//f-number,focal ratio
	462,	//focal_length;
	0,	//max_fps,max fps of sensor's all settings,it will be calculated from sensor mode fps
	16,	//max_adgain,AD-gain
	0,	//ois_supported;
	0,	//pdaf_supported;
	1,	//exp_valid_frame_num;N+2-1
	64,	//clamp_level,black level
	1,	//adgain_valid_frame_num;N+1-1
};

/*==============================================================================
 * Description:
 * sensor fps info related to sensor mode need by isp
 * please modify this variable acording your spec
 *============================================================================*/
static SENSOR_MODE_FPS_INFO_T s_imx230_mode_fps_info = {
	0,	//is_init;
	{{SENSOR_MODE_COMMON_INIT,0,1,0,0},
	{SENSOR_MODE_PREVIEW_ONE,0,1,0,0},
	{SENSOR_MODE_SNAPSHOT_ONE_FIRST,0,1,0,0},
	{SENSOR_MODE_SNAPSHOT_ONE_SECOND,0,1,0,0},
	{SENSOR_MODE_SNAPSHOT_ONE_THIRD,0,1,0,0},
	{SENSOR_MODE_PREVIEW_TWO,0,1,0,0},
	{SENSOR_MODE_SNAPSHOT_TWO_FIRST,0,1,0,0},
	{SENSOR_MODE_SNAPSHOT_TWO_SECOND,0,1,0,0},
	{SENSOR_MODE_SNAPSHOT_TWO_THIRD,0,1,0,0}}
};

/*==============================================================================
 * Description:
 * sensor all info
 * please modify this variable acording your spec
 *============================================================================*/
SENSOR_INFO_T g_imx230_mipi_raw_info = {
	/* salve i2c write address */
	(I2C_SLAVE_ADDR >> 1),
	/* salve i2c read address */
	(I2C_SLAVE_ADDR >> 1),
	/*bit0: 0: i2c register value is 8 bit, 1: i2c register value is 16 bit */
	SENSOR_I2C_REG_16BIT | SENSOR_I2C_VAL_8BIT | SENSOR_I2C_FREQ_400,
	/* bit2: 0:negative; 1:positive -> polarily of horizontal synchronization signal
	 * bit4: 0:negative; 1:positive -> polarily of vertical synchronization signal
	 * other bit: reseved
	 */
	SENSOR_HW_SIGNAL_PCLK_P | SENSOR_HW_SIGNAL_VSYNC_P | SENSOR_HW_SIGNAL_HSYNC_P,
	/* preview mode */
	SENSOR_ENVIROMENT_NORMAL | SENSOR_ENVIROMENT_NIGHT,
	/* image effect */
	SENSOR_IMAGE_EFFECT_NORMAL |
	    SENSOR_IMAGE_EFFECT_BLACKWHITE |
	    SENSOR_IMAGE_EFFECT_RED |
	    SENSOR_IMAGE_EFFECT_GREEN | SENSOR_IMAGE_EFFECT_BLUE | SENSOR_IMAGE_EFFECT_YELLOW |
	    SENSOR_IMAGE_EFFECT_NEGATIVE | SENSOR_IMAGE_EFFECT_CANVAS,

	/* while balance mode */
	0,
	/* bit[0:7]: count of step in brightness, contrast, sharpness, saturation
	 * bit[8:31] reseved
	 */
	7,
	/* reset pulse level */
	SENSOR_LOW_PULSE_RESET,
	/* reset pulse width(ms) */
	50,
	/* 1: high level valid; 0: low level valid */
	SENSOR_LOW_LEVEL_PWDN,
	/* count of identify code */
	1,
	/* supply two code to identify sensor.
	 * for Example: index = 0-> Device id, index = 1 -> version id
	 * customer could ignore it.
	 */
	{{imx230_PID_ADDR, imx230_PID_VALUE}
	 ,
	 {imx230_VER_ADDR, imx230_VER_VALUE}
	 }
	,
	/* voltage of avdd */
	SENSOR_AVDD_2500MV,
	/* max width of source image */
	SNAPSHOT_WIDTH,
	/* max height of source image */
	SNAPSHOT_HEIGHT,
	/* name of sensor */
	SENSOR_NAME,
	/* define in SENSOR_IMAGE_FORMAT_E enum,SENSOR_IMAGE_FORMAT_MAX
	 * if set to SENSOR_IMAGE_FORMAT_MAX here,
	 * image format depent on SENSOR_REG_TAB_INFO_T
	 */
	SENSOR_IMAGE_FORMAT_RAW,
	/*  pattern of input image form sensor */
	SENSOR_IMAGE_PATTERN_RAWRGB_R,
	/* point to resolution table information structure */
	s_imx230_resolution_tab_raw,
	/* point to ioctl function table */
	&s_imx230_ioctl_func_tab,
	/* information and table about Rawrgb sensor */
	&s_imx230_mipi_raw_info_ptr,
	/* extend information about sensor
	 * like &g_imx132_ext_info
	 */
	NULL,
	/* voltage of iovdd */
	SENSOR_AVDD_1800MV,
	/* voltage of dvdd */
	SENSOR_AVDD_1200MV,
	/* skip frame num before preview */
	1,
	/* skip frame num before capture */
	1,
	/* deci frame num during preview */
	0,
	/* deci frame num during video preview */
	0,
	0,//threshold_eb
	0,//threshold_mode
	0,//threshold_start
	0,//threshold_end
	0,//i2c_dev_handler
	{SENSOR_INTERFACE_TYPE_CSI2, LANE_NUM, RAW_BITS, 0},//sensor_interface
	0,//video_tab_info_ptr
	/* skip frame num while change setting */
	1,
	/* horizontal  view angle*/
	35,
	/* vertical view angle*/
	35,
	"imx230v1"//sensor version info
};

/*==============================================================================
 * Description:
 * get default frame length
 *
 *============================================================================*/
static uint32_t imx230_get_default_frame_length(uint32_t mode)
{
	return s_imx230_resolution_trim_tab[mode].frame_line;
}

/*==============================================================================
 * Description:
 * write group-hold on to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void imx230_group_hold_on(void)
{
	SENSOR_PRINT("E");

	//Sensor_WriteReg(0x0104, 0x01);
}

/*==============================================================================
 * Description:
 * write group-hold off to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void imx230_group_hold_off(void)
{
	SENSOR_PRINT("E");

	//Sensor_WriteReg(0x0104, 0x00);
}


/*==============================================================================
 * Description:
 * read gain from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static uint16_t imx230_read_gain(void)
{
	uint16_t gain_l = 0;

	gain_l = Sensor_ReadReg(0x0205);

	return gain_l;
}

static uint16_t gain2reg(const uint16_t gain)
{
	uint16_t i;

	for (i = 0; i < (159-1); i++) {
		if (gain <= sensorGainMapping[i][0]){
			break;
		}
	}

	return sensorGainMapping[i][1];
}

/*==============================================================================
 * Description:
 * write gain to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void imx230_write_gain(uint32_t gain)
{
	uint32_t sensor_again = 0;

/*
	sensor_again=256-(256*SENSOR_BASE_GAIN/gain);
	sensor_again=sensor_again&0xFF;

	if (SENSOR_MAX_GAIN < sensor_again)
			sensor_again = SENSOR_MAX_GAIN;
	SENSOR_PRINT("sensor_again=0x%x",sensor_again);
	Sensor_WriteReg(0x0205, sensor_again);
*/
	sensor_again = gain2reg(gain);

	SENSOR_PRINT("sensor_again=%d", sensor_again);

	Sensor_WriteReg(0x0204, (sensor_again>>8)& 0xFF);
	Sensor_WriteReg(0x0205, sensor_again & 0xFF);

	imx230_group_hold_off();

}

/*==============================================================================
 * Description:
 * read frame length from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static uint16_t imx230_read_frame_length(void)
{
	uint16_t frame_len_h = 0;
	uint16_t frame_len_l = 0;

	frame_len_h = Sensor_ReadReg(0x0340) & 0xff;
	frame_len_l = Sensor_ReadReg(0x0341) & 0xff;

	return ((frame_len_h << 8) | frame_len_l);
}

/*==============================================================================
 * Description:
 * write frame length to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void imx230_write_frame_length(uint32_t frame_len)
{
	Sensor_WriteReg(0x0340, (frame_len >> 8) & 0xff);
	Sensor_WriteReg(0x0341, frame_len & 0xff);
}

/*==============================================================================
 * Description:
 * read shutter from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t imx230_read_shutter(void)
{
	uint16_t shutter_h = 0;
	uint16_t shutter_l = 0;

	shutter_h = Sensor_ReadReg(0x0202) & 0xff;
	shutter_l = Sensor_ReadReg(0x0203) & 0xff;

	return (shutter_h << 8) | shutter_l;
}

/*==============================================================================
 * Description:
 * write shutter to sensor registers
 * please pay attention to the frame length
 * please modify this function acording your spec
 *============================================================================*/
static void imx230_write_shutter(uint32_t shutter)
{
	Sensor_WriteReg(0x0202, (shutter >> 8) & 0xff);
	Sensor_WriteReg(0x0203, shutter & 0xff);
}

/*==============================================================================
 * Description:
 * write exposure to sensor registers and get current shutter
 * please pay attention to the frame length
 * please don't change this function if it's necessary
 *============================================================================*/
static uint16_t imx230_update_exposure(uint32_t shutter,uint32_t dummy_line)
{
	uint32_t dest_fr_len = 0;
	uint32_t cur_fr_len = 0;
	uint32_t fr_len = s_current_default_frame_length;

	imx230_group_hold_on();
/*
	if (1 == SUPPORT_AUTO_FRAME_LENGTH)
		goto write_sensor_shutter;
*/
	dest_fr_len = ((shutter + dummy_line+FRAME_OFFSET) > fr_len) ? (shutter +dummy_line+ FRAME_OFFSET) : fr_len;

	cur_fr_len = imx230_read_frame_length();

	if (shutter < SENSOR_MIN_SHUTTER)
		shutter = SENSOR_MIN_SHUTTER;

	if (dest_fr_len != cur_fr_len)
		imx230_write_frame_length(dest_fr_len);
write_sensor_shutter:
	/* write shutter to sensor registers */
	imx230_write_shutter(shutter);
	return shutter;
}


/*==============================================================================
 * Description:
 * sensor power on
 * please modify this function acording your spec
 *============================================================================*/
static unsigned long imx230_power_on(unsigned long power_on)
{
	SENSOR_AVDD_VAL_E dvdd_val = g_imx230_mipi_raw_info.dvdd_val;
	SENSOR_AVDD_VAL_E avdd_val = g_imx230_mipi_raw_info.avdd_val;
	SENSOR_AVDD_VAL_E iovdd_val = g_imx230_mipi_raw_info.iovdd_val;
	BOOLEAN power_down = g_imx230_mipi_raw_info.power_down_level;
	BOOLEAN reset_level = g_imx230_mipi_raw_info.reset_pulse_level;

	if (SENSOR_TRUE == power_on) {
		Sensor_PowerDown(power_down);
		Sensor_SetResetLevel(reset_level);
		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		Sensor_SetVoltage(SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED);
		usleep(10 * 1000);
		Sensor_SetVoltage(dvdd_val, avdd_val, iovdd_val);
		usleep(10 * 1000);
		Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);
		usleep(10 * 1000);
		Sensor_PowerDown(!power_down);
		Sensor_SetResetLevel(!reset_level);
		Sensor_SetMonitorVoltage(SENSOR_AVDD_2800MV);
	} else {
		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		Sensor_SetVoltage(SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED);
		Sensor_Reset(reset_level);
		Sensor_PowerDown(power_down);
		Sensor_SetMonitorVoltage(SENSOR_AVDD_CLOSED);
	}
	SENSOR_PRINT("(1:on, 0:off): %ld", power_on);
	return SENSOR_SUCCESS;
}

/*==============================================================================
 * Description:
 * calculate fps for every sensor mode according to frame_line and line_time
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t imx230_init_mode_fps_info()
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_PRINT("imx230_init_mode_fps_info:E");
	if(!s_imx230_mode_fps_info.is_init) {
		uint32_t i,modn,tempfps = 0;
		SENSOR_PRINT("imx230_init_mode_fps_info:start init");
		for(i = 0;i < NUMBER_OF_ARRAY(s_imx230_resolution_trim_tab); i++) {
			//max fps should be multiple of 30,it calulated from line_time and frame_line
			tempfps = s_imx230_resolution_trim_tab[i].line_time*s_imx230_resolution_trim_tab[i].frame_line;
				if(0 != tempfps) {
					tempfps = 10000000/tempfps;
				modn = tempfps / 30;
				if(tempfps > modn*30)
					modn++;
				s_imx230_mode_fps_info.sensor_mode_fps[i].max_fps = modn*30;
				if(s_imx230_mode_fps_info.sensor_mode_fps[i].max_fps > 30) {
					s_imx230_mode_fps_info.sensor_mode_fps[i].is_high_fps = 1;
					s_imx230_mode_fps_info.sensor_mode_fps[i].high_fps_skip_num =
						s_imx230_mode_fps_info.sensor_mode_fps[i].max_fps/30;
				}
				if(s_imx230_mode_fps_info.sensor_mode_fps[i].max_fps >
						s_imx230_static_info.max_fps) {
					s_imx230_static_info.max_fps =
						s_imx230_mode_fps_info.sensor_mode_fps[i].max_fps;
				}
			}
			SENSOR_PRINT("mode %d,tempfps %d,frame_len %d,line_time: %d ",i,tempfps,
					s_imx230_resolution_trim_tab[i].frame_line,
					s_imx230_resolution_trim_tab[i].line_time);
			SENSOR_PRINT("mode %d,max_fps: %d ",
					i,s_imx230_mode_fps_info.sensor_mode_fps[i].max_fps);
			SENSOR_PRINT("is_high_fps: %d,highfps_skip_num %d",
					s_imx230_mode_fps_info.sensor_mode_fps[i].is_high_fps,
					s_imx230_mode_fps_info.sensor_mode_fps[i].high_fps_skip_num);
		}
		s_imx230_mode_fps_info.is_init = 1;
	}
	SENSOR_PRINT("imx230_init_mode_fps_info:X");
	return rtn;
}

/*==============================================================================
 * Description:
 * identify sensor id
 * please modify this function acording your spec
 *============================================================================*/
static unsigned long imx230_identify(unsigned long param)
{
	uint8_t pid_value = 0x00;
	uint8_t ver_value = 0x00;
	uint32_t ret_value = SENSOR_FAIL;
	UNUSED(param);

	SENSOR_PRINT("mipi raw identify");

	pid_value = Sensor_ReadReg(imx230_PID_ADDR);

	if (imx230_PID_VALUE == pid_value) {
		ver_value = Sensor_ReadReg(imx230_VER_ADDR);
		SENSOR_PRINT("Identify: PID = %x, VER = %x", pid_value, ver_value);
		if (imx230_VER_VALUE == ver_value) {
			ret_value = SENSOR_SUCCESS;
			SENSOR_PRINT_HIGH("this is imx230 sensor");
			vcm_dw9800_init();
			imx230_init_mode_fps_info();
		} else {
			SENSOR_PRINT_HIGH("Identify this is %x%x sensor", pid_value, ver_value);
		}
	} else {
		SENSOR_PRINT_HIGH("identify fail, pid_value = %x", pid_value);
	}

	return ret_value;
}

/*==============================================================================
 * Description:
 * get resolution trim
 *
 *============================================================================*/
static unsigned long imx230_get_resolution_trim_tab(unsigned long param)
{
	UNUSED(param);
	return (unsigned long) s_imx230_resolution_trim_tab;
}

/*==============================================================================
 * Description:
 * before snapshot
 * you can change this function if it's necessary
 *============================================================================*/
static unsigned long imx230_before_snapshot(unsigned long param)
{
	uint32_t cap_shutter = 0;
	uint32_t prv_shutter = 0;
	uint32_t gain = 0;
	uint32_t cap_gain = 0;
	uint32_t capture_mode = param & 0xffff;
	uint32_t preview_mode = (param >> 0x10) & 0xffff;

	uint32_t prv_linetime = s_imx230_resolution_trim_tab[preview_mode].line_time;
	uint32_t cap_linetime = s_imx230_resolution_trim_tab[capture_mode].line_time;

	s_current_default_frame_length = imx230_get_default_frame_length(capture_mode);
	SENSOR_PRINT("capture_mode = %d", capture_mode);

	if (preview_mode == capture_mode) {
		cap_shutter = s_sensor_ev_info.preview_shutter;
		cap_gain = s_sensor_ev_info.preview_gain;
		goto snapshot_info;
	}

	prv_shutter = s_sensor_ev_info.preview_shutter;	//imx132_read_shutter();
	gain = s_sensor_ev_info.preview_gain;	//imx132_read_gain();

	Sensor_SetMode(capture_mode);
	Sensor_SetMode_WaitDone();

	cap_shutter = prv_shutter * prv_linetime / cap_linetime * BINNING_FACTOR;

	while (gain >= (2 * SENSOR_BASE_GAIN)) {
		if (cap_shutter * 2 > s_current_default_frame_length)
			break;
		cap_shutter = cap_shutter * 2;
		gain = gain / 2;
	}

	cap_shutter = imx230_update_exposure(cap_shutter,0);
	cap_gain = gain;
	imx230_write_gain(cap_gain);
	SENSOR_PRINT("preview_shutter = 0x%x, preview_gain = 0x%x",
		     s_sensor_ev_info.preview_shutter, s_sensor_ev_info.preview_gain);

	SENSOR_PRINT("capture_shutter = 0x%x, capture_gain = 0x%x", cap_shutter, cap_gain);
snapshot_info:
	s_hdr_info.capture_shutter = cap_shutter; //imx132_read_shutter();
	s_hdr_info.capture_gain = cap_gain; //imx132_read_gain();
	/* limit HDR capture min fps to 10;
	 * MaxFrameTime = 1000000*0.1us;
	 */
	s_hdr_info.capture_max_shutter = 1000000 / cap_linetime;

	Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_EXPOSURETIME, cap_shutter);

	return SENSOR_SUCCESS;
}

/*==============================================================================
 * Description:
 * get the shutter from isp
 * please don't change this function unless it's necessary
 *============================================================================*/
static unsigned long imx230_write_exposure(unsigned long param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t exposure_line = 0x00;
	uint16_t dummy_line = 0x00;
	uint16_t mode = 0x00;

	exposure_line = param & 0xffff;
	dummy_line = (param >> 0x10) & 0xfff; /*for cits frame rate test*/
	mode = (param >> 0x1c) & 0x0f;

	SENSOR_PRINT("current mode = %d, exposure_line = %d, dummy_line=%d", mode, exposure_line,dummy_line);
	s_current_default_frame_length = imx230_get_default_frame_length(mode);

	s_sensor_ev_info.preview_shutter = imx230_update_exposure(exposure_line,dummy_line);

	return ret_value;
}

static unsigned long imx230_ex_write_exposure(unsigned long param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t exposure_line = 0x00;
	uint16_t dummy_line = 0x00;
	uint16_t mode = 0x00;
	struct sensor_ex_exposure  *ex = (struct sensor_ex_exposure*)param;

	if (!param) {
		SENSOR_PRINT_ERR("param is NULL !!!");
		return ret_value;
	}

	exposure_line = ex->exposure;
	dummy_line = ex->dummy;
	mode = ex->size_index;

	SENSOR_PRINT("current mode = %d, exposure_line = %d, dummy_line=%d", mode, exposure_line,dummy_line);
	s_current_default_frame_length = imx230_get_default_frame_length(mode);

	s_sensor_ev_info.preview_shutter = imx230_update_exposure(exposure_line,dummy_line);

	return ret_value;
}

/*==============================================================================
 * Description:
 * get the parameter from isp to real gain
 * you mustn't change the funcion !
 *============================================================================*/
static uint32_t isp_to_real_gain(uint32_t param)
{
	uint32_t real_gain = 0;
#if defined(CONFIG_CAMERA_ISP_VERSION_V3) || defined(CONFIG_CAMERA_ISP_VERSION_V4)
	real_gain=param;
#else
	real_gain = ((param & 0xf) + 16) * (((param >> 4) & 0x01) + 1);
	real_gain = real_gain * (((param >> 5) & 0x01) + 1) * (((param >> 6) & 0x01) + 1);
	real_gain = real_gain * (((param >> 7) & 0x01) + 1) * (((param >> 8) & 0x01) + 1);
	real_gain = real_gain * (((param >> 9) & 0x01) + 1) * (((param >> 10) & 0x01) + 1);
	real_gain = real_gain * (((param >> 11) & 0x01) + 1);
#endif

	return real_gain;
}

/*==============================================================================
 * Description:
 * write gain value to sensor
 * you can change this function if it's necessary
 *============================================================================*/
static unsigned long imx230_write_gain_value(unsigned long param)
{
	unsigned long ret_value = SENSOR_SUCCESS;
	uint32_t real_gain = 0;

	/*real_gain = isp_to_real_gain(param);*/

	real_gain = param * SENSOR_BASE_GAIN / ISP_BASE_GAIN;

	SENSOR_PRINT("real_gain = %d", real_gain);

	s_sensor_ev_info.preview_gain = real_gain;
	imx230_write_gain(real_gain);

	return ret_value;
}

/*==============================================================================
 * Description:
 * increase gain or shutter for hdr
 *
 *============================================================================*/
static void imx230_increase_hdr_exposure(uint8_t ev_multiplier)
{
	uint32_t shutter_multiply = s_hdr_info.capture_max_shutter / s_hdr_info.capture_shutter;
	uint32_t gain = 0;

	if (0 == shutter_multiply)
		shutter_multiply = 1;

	if (shutter_multiply >= ev_multiplier) {
		imx230_update_exposure(s_hdr_info.capture_shutter * ev_multiplier,0);
		imx230_write_gain(s_hdr_info.capture_gain);
	} else {
		gain = s_hdr_info.capture_gain * ev_multiplier / shutter_multiply;
		imx230_update_exposure(s_hdr_info.capture_shutter * shutter_multiply,0);
		imx230_write_gain(gain);
	}
}

/*==============================================================================
 * Description:
 * decrease gain or shutter for hdr
 *
 *============================================================================*/
static void imx230_decrease_hdr_exposure(uint8_t ev_divisor)
{
	uint16_t gain_multiply = 0;
	uint32_t shutter = 0;
	gain_multiply = s_hdr_info.capture_gain / SENSOR_BASE_GAIN;

	if (gain_multiply >= ev_divisor) {
		imx230_update_exposure(s_hdr_info.capture_shutter,0);
		imx230_write_gain(s_hdr_info.capture_gain / ev_divisor);

	} else {
		shutter = s_hdr_info.capture_shutter * gain_multiply / ev_divisor;
		imx230_update_exposure(shutter,0);
		imx230_write_gain(s_hdr_info.capture_gain / gain_multiply);
	}
}

/*==============================================================================
 * Description:
 * set hdr ev
 * you can change this function if it's necessary
 *============================================================================*/
static uint32_t imx230_set_hdr_ev(unsigned long param)
{
	uint32_t ret = SENSOR_SUCCESS;
	SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR) param;

	uint32_t ev = ext_ptr->param;
	uint8_t ev_divisor, ev_multiplier;

	switch (ev) {
	case SENSOR_HDR_EV_LEVE_0:
		ev_divisor = 2;
		imx230_decrease_hdr_exposure(ev_divisor);
		break;
	case SENSOR_HDR_EV_LEVE_1:
		ev_multiplier = 2;
		imx230_increase_hdr_exposure(ev_multiplier);
		break;
	case SENSOR_HDR_EV_LEVE_2:
		ev_multiplier = 1;
		imx230_increase_hdr_exposure(ev_multiplier);
		break;
	default:
		break;
	}
	return ret;
}

/*==============================================================================
 * Description:
 * extra functoin
 * you can add functions reference SENSOR_EXT_FUNC_CMD_E which from sensor_drv_u.h
 *============================================================================*/
static unsigned long imx230_ext_func(unsigned long param)
{
	unsigned long rtn = SENSOR_SUCCESS;
	SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR) param;

	SENSOR_PRINT("ext_ptr->cmd: %d", ext_ptr->cmd);
	switch (ext_ptr->cmd) {
	case SENSOR_EXT_EV:
		rtn = imx230_set_hdr_ev(param);
		break;
	default:
		break;
	}

	return rtn;
}

/*==============================================================================
 * Description:
 * mipi stream on
 * please modify this function acording your spec
 *============================================================================*/
static unsigned long imx230_stream_on(unsigned long param)
{
	SENSOR_PRINT("E");
	UNUSED(param);
	Sensor_WriteReg(0x0100, 0x01);
	/*delay*/
	usleep(10 * 1000);

	return 0;
}

/*==============================================================================
 * Description:
 * mipi stream off
 * please modify this function acording your spec
 *============================================================================*/
static unsigned long imx230_stream_off(unsigned long param)
{
	SENSOR_PRINT("E");
	UNUSED(param);
	Sensor_WriteReg(0x0100, 0x00);
	/*delay*/
	usleep(10 * 1000);

	return 0;
}

static unsigned long imx230_write_af(unsigned long param)
{
	return vcm_dw9800_set_position(param);
}

static uint32_t imx230_get_static_info(uint32_t *param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	struct sensor_ex_info *ex_info;
	//make sure we have get max fps of all settings.
	if(!s_imx230_mode_fps_info.is_init) {
		imx230_init_mode_fps_info();
	}
	ex_info = (struct sensor_ex_info*)param;
	ex_info->f_num = s_imx230_static_info.f_num;
	ex_info->focal_length = s_imx230_static_info.focal_length;
	ex_info->max_fps = s_imx230_static_info.max_fps;
	ex_info->max_adgain = s_imx230_static_info.max_adgain;
	ex_info->ois_supported = s_imx230_static_info.ois_supported;
	ex_info->pdaf_supported = s_imx230_static_info.pdaf_supported;
	ex_info->exp_valid_frame_num = s_imx230_static_info.exp_valid_frame_num;
	ex_info->clamp_level = s_imx230_static_info.clamp_level;
	ex_info->adgain_valid_frame_num = s_imx230_static_info.adgain_valid_frame_num;
	ex_info->preview_skip_num = g_imx230_mipi_raw_info.preview_skip_num;
	ex_info->capture_skip_num = g_imx230_mipi_raw_info.capture_skip_num;
	ex_info->name = g_imx230_mipi_raw_info.name;
	ex_info->sensor_version_info = g_imx230_mipi_raw_info.sensor_version_info;
	SENSOR_PRINT("SENSOR_IMX230: f_num: %d", ex_info->f_num);
	SENSOR_PRINT("SENSOR_IMX230: max_fps: %d", ex_info->max_fps);
	SENSOR_PRINT("SENSOR_IMX230: max_adgain: %d", ex_info->max_adgain);
	SENSOR_PRINT("SENSOR_IMX230: ois_supported: %d", ex_info->ois_supported);
	SENSOR_PRINT("SENSOR_IMX230: pdaf_supported: %d", ex_info->pdaf_supported);
	SENSOR_PRINT("SENSOR_IMX230: exp_valid_frame_num: %d", ex_info->exp_valid_frame_num);
	SENSOR_PRINT("SENSOR_IMX230: clam_level: %d", ex_info->clamp_level);
	SENSOR_PRINT("SENSOR_IMX230: adgain_valid_frame_num: %d", ex_info->adgain_valid_frame_num);
	SENSOR_PRINT("SENSOR_IMX230: sensor name is: %s", ex_info->name);
	SENSOR_PRINT("SENSOR_IMX230: sensor version info is: %s", ex_info->sensor_version_info);

	return rtn;
}


static uint32_t imx230_get_fps_info(uint32_t *param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_MODE_FPS_T *fps_info;
	//make sure have inited fps of every sensor mode.
	if(!s_imx230_mode_fps_info.is_init) {
		imx230_init_mode_fps_info();
	}
	fps_info = (SENSOR_MODE_FPS_T*)param;
	uint32_t sensor_mode = fps_info->mode;
	fps_info->max_fps = s_imx230_mode_fps_info.sensor_mode_fps[sensor_mode].max_fps;
	fps_info->min_fps = s_imx230_mode_fps_info.sensor_mode_fps[sensor_mode].min_fps;
	fps_info->is_high_fps = s_imx230_mode_fps_info.sensor_mode_fps[sensor_mode].is_high_fps;
	fps_info->high_fps_skip_num = s_imx230_mode_fps_info.sensor_mode_fps[sensor_mode].high_fps_skip_num;
	SENSOR_PRINT("SENSOR_IMX230: mode %d, max_fps: %d",fps_info->mode, fps_info->max_fps);
	SENSOR_PRINT("SENSOR_IMX230: min_fps: %d", fps_info->min_fps);
	SENSOR_PRINT("SENSOR_IMX230: is_high_fps: %d", fps_info->is_high_fps);
	SENSOR_PRINT("SENSOR_IMX230: high_fps_skip_num: %d", fps_info->high_fps_skip_num);

	return rtn;
}

static unsigned long imx230_access_val(unsigned long param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_VAL_T* param_ptr = (SENSOR_VAL_T*)param;
	uint16_t tmp;

	SENSOR_PRINT("SENSOR_IMX230: _imx230_access_val E param_ptr = %p", param_ptr);
	if(!param_ptr){
		return rtn;
	}

	SENSOR_PRINT("SENSOR_IMX230: param_ptr->type=%x", param_ptr->type);
	switch(param_ptr->type)
	{
		case SENSOR_VAL_TYPE_INIT_OTP:
			//imx230_otp_init();
			break;
		case SENSOR_VAL_TYPE_SHUTTER:
			//*((uint32_t*)param_ptr->pval) = imx230_get_shutter();
			break;
		case SENSOR_VAL_TYPE_READ_VCM:
			//rtn = imx230_read_vcm(param_ptr->pval);
			break;
		case SENSOR_VAL_TYPE_WRITE_VCM:
			//rtn = imx230_write_vcm(param_ptr->pval);
			break;
		case SENSOR_VAL_TYPE_READ_OTP:
			//imx230_otp_read(param_ptr);
			break;
		case SENSOR_VAL_TYPE_WRITE_OTP:
			//rtn = _hi544_write_otp((uint32_t)param_ptr->pval);
			break;
		case SENSOR_VAL_TYPE_GET_RELOADINFO:
			{
//				struct isp_calibration_info **p= (struct isp_calibration_info **)param_ptr->pval;
//				*p=&calibration_info;
			}
			break;
		case SENSOR_VAL_TYPE_GET_AFPOSITION:
			//*(uint32_t*)param_ptr->pval = 0;//cur_af_pos;
			break;
		case SENSOR_VAL_TYPE_WRITE_OTP_GAIN:
			//rtn = imx230_write_otp_gain(param_ptr->pval);
			break;
		case SENSOR_VAL_TYPE_READ_OTP_GAIN:
			//rtn = imx230_read_otp_gain(param_ptr->pval);
			break;
		case SENSOR_VAL_TYPE_GET_STATIC_INFO:
			rtn = imx230_get_static_info(param_ptr->pval);
			break;
		case SENSOR_VAL_TYPE_GET_FPS_INFO:
			rtn = imx230_get_fps_info(param_ptr->pval);
			break;
		default:
			break;
	}

	SENSOR_PRINT("SENSOR_IMX230: _imx230_access_val X");

	return rtn;
}

/*==============================================================================
 * Description:
 * all ioctl functoins
 * you can add functions reference SENSOR_IOCTL_FUNC_TAB_T from sensor_drv_u.h
 *
 * add ioctl functions like this:
 * .power = imx132_power_on,
 *============================================================================*/
static SENSOR_IOCTL_FUNC_TAB_T s_imx230_ioctl_func_tab = {
	.power = imx230_power_on,
	.identify = imx230_identify,
	.get_trim = imx230_get_resolution_trim_tab,
	.before_snapshort = imx230_before_snapshot,
	.ex_write_exp = imx230_ex_write_exposure,
	.write_gain_value = imx230_write_gain_value,
	//.set_focus = imx230_ext_func,
	//.set_video_mode = imx132_set_video_mode,
	.stream_on = imx230_stream_on,
	.stream_off = imx230_stream_off,
	.af_enable = imx230_write_af,
	//.group_hold_on = imx132_group_hold_on,
	//.group_hold_of = imx132_group_hold_off,
	.cfg_otp = imx230_access_val,
};
