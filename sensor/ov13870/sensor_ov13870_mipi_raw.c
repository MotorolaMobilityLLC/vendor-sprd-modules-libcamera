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
#include "sensor_raw.h"
#include "sensor_ov13870_raw_param.c"
#include "isp_param_file_update.h"
#include "af_bu64297gwz.h"

#define ov13870_I2C_ADDR_W        (0x6C >> 1)
#define ov13870_I2C_ADDR_R        (0x6C >> 1)

#define ov13870_RAW_PARAM_COM     0x0000

#ifndef RAW_INFO_END_ID
#define RAW_INFO_END_ID 0x71717567
#endif

#define ov13870_i2c_read_otp(addr)  ov13870_i2c_read_otp_set(handle,addr)

static uint32_t g_module_id = 0;
static struct sensor_raw_info* s_ov13870_mipi_raw_info_ptr = NULL;
static int s_capture_shutter = 0;
static int s_exposure_time = 0;

SENSOR_HW_HANDLE _ov13870_Create(void *privatedata);
void _ov13870_Destroy(SENSOR_HW_HANDLE handle);
static unsigned long _ov13870_GetResolutionTrimTab(SENSOR_HW_HANDLE handle, unsigned long param);
static unsigned long _ov13870_Identify(SENSOR_HW_HANDLE handle, unsigned long param);
static uint32_t _ov13870_GetRawInof(SENSOR_HW_HANDLE handle);
static unsigned long _ov13870_StreamOn(SENSOR_HW_HANDLE handle, unsigned long param);
static unsigned long _ov13870_StreamOff(SENSOR_HW_HANDLE handle, unsigned long param);
static uint32_t _ov13870_com_Identify_otp(SENSOR_HW_HANDLE handle, void* param_ptr);
static unsigned long _ov13870_PowerOn(SENSOR_HW_HANDLE handle, unsigned long power_on);
static unsigned long _ov13870_write_gain(SENSOR_HW_HANDLE handle, unsigned long param);
static unsigned long _ov13870_access_val(SENSOR_HW_HANDLE handle, unsigned long param);
static unsigned long ov13870_write_af(SENSOR_HW_HANDLE handle, unsigned long param);
static unsigned long _ov13870_GetExifInfo(SENSOR_HW_HANDLE handle, unsigned long param);
static unsigned long _ov13870_BeforeSnapshot(SENSOR_HW_HANDLE handle, unsigned long param);
static uint16_t _ov13870_get_shutter(SENSOR_HW_HANDLE handle);
static unsigned long _ov13870_ex_write_exposure(SENSOR_HW_HANDLE handle, unsigned long param);


static const struct raw_param_info_tab s_ov13870_raw_param_tab[] = {
	{ov13870_RAW_PARAM_COM, &s_ov13870_mipi_raw_info, _ov13870_com_Identify_otp, NULL},
	{RAW_INFO_END_ID, PNULL, PNULL, PNULL}
};


static const SENSOR_REG_T ov13870_common_init[] = {
		{0x301e, 0x00},
		// Delay,1ms
		{0x0103, 0x01},
		{0x0300, 0xf8},
		{0x0300, 0xf8},
		{0x0300, 0xf8},
		{0x0300, 0xf8},
		{0x0302, 0x10},
		{0x0303, 0x00},
		{0x0304, 0x25},
		{0x031a, 0x01},
		{0x0316, 0x4b},
		{0x0317, 0x00},
		{0x0318, 0x04},
		{0x031d, 0x09},
		{0x0320, 0x11},
		{0x031e, 0x09},
		{0x3012, 0x41},
		{0x3016, 0xb4},
		{0x3018, 0xf0},
		{0x3019, 0xe1},
		{0x301b, 0x16},
		{0x3023, 0xb4},
		{0x3028, 0x0f},
		{0x3106, 0x00},
		{0x3400, 0x04},
		{0x340c, 0xff},
		{0x340d, 0xff},
		{0x3501, 0x0c},
		{0x3502, 0xc0},
		{0x3503, 0x80},
		{0x3505, 0x80},
		{0x3507, 0x00},
		{0x3508, 0x07},
		{0x3509, 0xff},
		{0x350a, 0x00},
		{0x350b, 0x01},
		{0x350e, 0x00},
		{0x350f, 0x01},
		{0x350c, 0x07},
		{0x350d, 0xff},
		{0x3511, 0x02},
		{0x3512, 0x00},
		{0x3600, 0x00},
		{0x3602, 0x0e},
		{0x3603, 0x00},
		{0x3608, 0xd7},
		{0x360a, 0x70},
		{0x360b, 0x10},
		{0x360c, 0x3a},
		{0x360d, 0x43},
		{0x360e, 0x00},
		{0x3611, 0x00},
		{0x3612, 0x00},
		{0x3613, 0x00},
		{0x3618, 0x94},
		{0x3619, 0x83},
		{0x361a, 0x24},
		{0x3621, 0x88},
		{0x3622, 0x88},
		{0x3623, 0x58},
		{0x3624, 0x83},
		{0x3626, 0x99},
		{0x3627, 0x00},
		{0x3628, 0x84},
		{0x3629, 0x00},
		{0x362a, 0x05},
		{0x3632, 0x00},
		{0x3633, 0x0a},
		{0x3634, 0x10},
		{0x3635, 0x10},
		{0x3636, 0x10},
		{0x3652, 0xff},
		{0x3653, 0xff},
		{0x3660, 0x40},
		{0x3661, 0x0c},
		{0x3662, 0x00},
		{0x3663, 0x00},
		{0x3666, 0xa7},
		{0x366a, 0x10},
		{0x366c, 0x54},
		{0x3701, 0x54},
		{0x3704, 0x28},
		{0x3706, 0x3c},
		{0x3709, 0xce},
		{0x370a, 0x00},
		{0x370b, 0xbc},
		{0x3711, 0x00},
		{0x3714, 0x67},
		{0x3715, 0x00},
		{0x371a, 0x1c},
		{0x373b, 0x10},
		{0x373d, 0x12},
		{0x3754, 0xee},
		{0x375a, 0x08},
		{0x3764, 0x24},
		{0x3765, 0x16},
		{0x3765, 0x16},
		{0x3768, 0x20},
		{0x376a, 0x10},
		{0x3798, 0x84},
		{0x3780, 0x00},
		{0x3781, 0x38},
		{0x3782, 0x00},
		{0x3783, 0x00},
		{0x37d9, 0x01},
		{0x37e3, 0x02},
		{0x3800, 0x00},
		{0x3801, 0x14},
		{0x3802, 0x00},
		{0x3803, 0x0c},
		{0x3804, 0x10},
		{0x3805, 0x8b},
		{0x3806, 0x0c},
		{0x3807, 0x43},
		{0x3808, 0x10},
		{0x3809, 0x80},
		{0x380a, 0x0c},
		{0x380b, 0x40},
		{0x380c, 0x07},
		{0x380d, 0x1a},
		{0x380e, 0x0c},
		{0x380f, 0xe4},
		{0x3810, 0x00},
		{0x3811, 0x04},
		{0x3813, 0x08},
		{0x3814, 0x11},
		{0x3815, 0x11},
		{0x3820, 0x00},
		{0x3821, 0x04},
		{0x383c, 0xa8},
		{0x383d, 0xff},
		{0x3842, 0x00},
		{0x3d85, 0x17},
		{0x3d8c, 0x6f},
		{0x3d8d, 0x97},
		{0x4000, 0xf8},
		{0x4010, 0x38},
		{0x4011, 0x01},
		{0x4012, 0x0c},
		{0x4015, 0x00},
		{0x4016, 0x1f},
		{0x4017, 0x00},
		{0x4018, 0x1f},
		{0x4020, 0x04},
		{0x4021, 0x00},
		{0x4022, 0x04},
		{0x4023, 0x00},
		{0x4024, 0x04},
		{0x4025, 0x00},
		{0x4026, 0x04},
		{0x4027, 0x00},
		{0x4030, 0x00},
		{0x4031, 0x00},
		{0x4032, 0x00},
		{0x4033, 0x00},
		{0x4034, 0x00},
		{0x4035, 0x00},
		{0x4036, 0x00},
		{0x4037, 0x00},
		{0x4038, 0x00},
		{0x4039, 0x00},
		{0x403a, 0x00},
		{0x403b, 0x00},
		{0x403c, 0x00},
		{0x403d, 0x00},
		{0x403e, 0x00},
		{0x403f, 0x00},
		{0x4040, 0x0f},
		{0x4041, 0xc0},
		{0x4042, 0x0f},
		{0x4043, 0xc0},
		{0x4044, 0x0f},
		{0x4045, 0xc0},
		{0x4046, 0x0f},
		{0x4047, 0xc0},
		{0x4048, 0x0f},
		{0x4049, 0xc0},
		{0x404a, 0x0f},
		{0x404b, 0xc0},
		{0x404c, 0x0f},
		{0x404d, 0xc0},
		{0x404e, 0x0f},
		{0x404f, 0xc0},
		{0x4056, 0x21},
		{0x401c, 0x00},
		{0x401d, 0x10},
		{0x4500, 0x24},
		{0x4501, 0x08},
		{0x4502, 0x00},
		{0x450a, 0x05},
		{0x4640, 0x01},
		{0x4641, 0x04},
		{0x4642, 0x22},
		{0x4643, 0x02},
		{0x4645, 0x03},
		{0x4809, 0x2b},
		{0x480e, 0x02},
		{0x4813, 0x90},
		{0x4837, 0x08},
		{0x4a00, 0x00},
		{0x4b05, 0x83},
		{0x4d00, 0x04},
		{0x4d01, 0x30},
		{0x4d02, 0xb7},
		{0x4d03, 0xaf},
		{0x4d04, 0xa9},
		{0x4d05, 0xa7},
		{0x5000, 0xa7},
		{0x5001, 0x0c},
		{0x5017, 0xeb},
		{0x5020, 0x04},
		{0x50c0, 0x01},
		{0x50c1, 0x00},
		{0x55ca, 0x03},
		{0x55cb, 0x01},
		{0x5700, 0x01},
		{0x5044, 0x00},
		{0x5045, 0x50},
		{0x5046, 0x10},
		{0x5047, 0x4f},
		{0x5048, 0x00},
		{0x5049, 0x30},
		{0x504a, 0x0c},
		{0x504b, 0x2f},
		{0x57bc, 0x10},
		{0x57bd, 0xa0},
		{0x57be, 0x0c},
		{0x57bf, 0x60},
		{0x57d4, 0x00},
		{0x57d5, 0x00},
		{0x57d6, 0x00},
		{0x57d7, 0x00},
		{0x57d8, 0x00},
		{0x57d9, 0x00},
		{0x57da, 0x00},
		{0x57db, 0x00},
		{0x57dc, 0x20},
		{0x57dd, 0x00},
		{0x57de, 0x20},
		{0x57df, 0x00},
		{0x57e0, 0x00},
		{0x57e1, 0x00},
		{0x57e2, 0x00},
		{0x57e3, 0x00},
		{0x57e4, 0x00},
		{0x57e5, 0x00},
		{0x57e6, 0x00},
		{0x57e7, 0x00},
		{0x57e8, 0x00},
		{0x57e9, 0x00},
		{0x57ea, 0x00},
		{0x57eb, 0x00},
		{0x57ec, 0x20},
		{0x57ed, 0x00},
		{0x57ee, 0x20},
		{0x57ef, 0x00},
		{0x57f0, 0x00},
		{0x57f1, 0x00},
		{0x57f2, 0x00},
		{0x57f3, 0x00},
		{0x57f4, 0x00},
		{0x57f5, 0x00},
		{0x57f6, 0x00},
		{0x57f7, 0x00},
		{0x57f8, 0x00},
		{0x57f9, 0x00},
		{0x57fa, 0x00},
		{0x57fb, 0x00},
		{0x57fc, 0x00},
		{0x57fd, 0x20},
		{0x57fe, 0x00},
		{0x57ff, 0x20},
		{0x5800, 0x00},
		{0x5801, 0x00},
		{0x5802, 0x00},
		{0x5803, 0x00},
		{0x5804, 0x00},
		{0x5805, 0x00},
		{0x5806, 0x00},
		{0x5807, 0x00},
		{0x5808, 0x00},
		{0x5809, 0x00},
		{0x580a, 0x00},
		{0x580b, 0x00},
		{0x580c, 0x00},
		{0x580d, 0x20},
		{0x580e, 0x00},
		{0x580f, 0x20},
		{0x5810, 0x00},
		{0x5811, 0x00},
		{0x5812, 0x00},
		{0x5813, 0x00},
		{0x5814, 0x00},
		{0x5815, 0x00},
		{0x5816, 0x00},
		{0x5817, 0x00},
		{0x5818, 0x00},
		{0x5819, 0x00},
		{0x581a, 0x00},
		{0x581b, 0x00},
		{0x581c, 0x20},
		{0x581d, 0x00},
		{0x581e, 0x20},
		{0x581f, 0x00},
		{0x5820, 0x00},
		{0x5821, 0x00},
		{0x5822, 0x00},
		{0x5823, 0x00},
		{0x5824, 0x00},
		{0x5825, 0x00},
		{0x5826, 0x00},
		{0x5827, 0x00},
		{0x5828, 0x00},
		{0x5829, 0x00},
		{0x582a, 0x00},
		{0x582b, 0x00},
		{0x582c, 0x20},
		{0x582d, 0x00},
		{0x582e, 0x20},
		{0x582f, 0x00},
		{0x5830, 0x00},
		{0x5831, 0x00},
		{0x5832, 0x00},
		{0x5833, 0x00},
		{0x5834, 0x00},
		{0x5835, 0x00},
		{0x5836, 0x00},
		{0x5837, 0x00},
		{0x5838, 0x00},
		{0x5839, 0x00},
		{0x583a, 0x00},
		{0x583b, 0x00},
		{0x583c, 0x00},
		{0x583d, 0x20},
		{0x583e, 0x00},
		{0x583f, 0x20},
		{0x5840, 0x00},
		{0x5841, 0x00},
		{0x5842, 0x00},
		{0x5843, 0x00},
		{0x5844, 0x00},
		{0x5845, 0x00},
		{0x5846, 0x00},
		{0x5847, 0x00},
		{0x5848, 0x00},
		{0x5849, 0x00},
		{0x584a, 0x00},
		{0x584b, 0x00},
		{0x584c, 0x00},
		{0x584d, 0x20},
		{0x584e, 0x00},
		{0x584f, 0x20},
		{0x5850, 0x00},
		{0x5851, 0x00},
		{0x5852, 0x00},
		{0x5853, 0x00},
		{0x5854, 0x00},
		{0x5855, 0x00},
		{0x5856, 0x00},
		{0x5857, 0x00},
		{0x5858, 0x00},
		{0x5859, 0x00},
		{0x585a, 0x00},
		{0x585b, 0x00},
		{0x585c, 0x00},
		{0x585d, 0x00},
		{0x585e, 0x00},
		{0x585f, 0x00},
		{0x5860, 0x00},
		{0x5861, 0x00},
		{0x5862, 0x00},
		{0x5863, 0x00},
		{0x5864, 0x08},
		{0x5865, 0x00},
		{0x5866, 0x00},
		{0x5867, 0x00},
		{0x5868, 0x08},
		{0x5869, 0x00},
		{0x586a, 0x00},
		{0x586b, 0x00},
		{0x586c, 0x00},
		{0x586d, 0x00},
		{0x586e, 0x00},
		{0x586f, 0x00},
		{0x5870, 0x00},
		{0x5871, 0x00},
		{0x5872, 0x00},
		{0x5873, 0x00},
		{0x5874, 0x00},
		{0x5875, 0x00},
		{0x5876, 0x00},
		{0x5877, 0x00},
		{0x5878, 0x00},
		{0x5879, 0x00},
		{0x587a, 0x00},
		{0x587b, 0x00},
		{0x587c, 0x00},
		{0x587d, 0x00},
		{0x587e, 0x00},
		{0x587f, 0x00},
		{0x5880, 0x00},
		{0x5881, 0x00},
		{0x5882, 0x00},
		{0x5883, 0x00},
		{0x5884, 0x0c},
		{0x5885, 0x00},
		{0x5886, 0x00},
		{0x5887, 0x00},
		{0x5888, 0x0c},
		{0x5889, 0x00},
		{0x588a, 0x00},
		{0x588b, 0x00},
		{0x588c, 0x00},
		{0x588d, 0x00},
		{0x588e, 0x00},
		{0x588f, 0x00},
		{0x5890, 0x00},
		{0x5891, 0x00},
		{0x5892, 0x00},
		{0x5893, 0x00},
		{0x5894, 0x00},
		{0x5895, 0x00},
		{0x5896, 0x00},
		{0x5897, 0x00},
		{0x5898, 0x00},
		{0x5899, 0x00},
		{0x589a, 0x00},
		{0x589b, 0x00},
		{0x589c, 0x00},
		{0x589d, 0x00},
		{0x589e, 0x00},
		{0x589f, 0x00},
		{0x58a0, 0x00},
		{0x58a1, 0x00},
		{0x58a2, 0x00},
		{0x58a3, 0x00},
		{0x58a4, 0x00},
		{0x58a5, 0x00},
		{0x58a6, 0x0c},
		{0x58a7, 0x00},
		{0x58a8, 0x00},
		{0x58a9, 0x00},
		{0x58aa, 0x0c},
		{0x58ab, 0x00},
		{0x58ac, 0x00},
		{0x58ad, 0x00},
		{0x58ae, 0x00},
		{0x58af, 0x00},
		{0x58b0, 0x00},
		{0x58b1, 0x00},
		{0x58b2, 0x00},
		{0x58b3, 0x00},
		{0x58b4, 0x00},
		{0x58b5, 0x00},
		{0x58b6, 0x00},
		{0x58b7, 0x00},
		{0x58b8, 0x00},
		{0x58b9, 0x00},
		{0x58ba, 0x00},
		{0x58bb, 0x00},
		{0x58bc, 0x00},
		{0x58bd, 0x00},
		{0x58be, 0x00},
		{0x58bf, 0x00},
		{0x58c0, 0x00},
		{0x58c1, 0x00},
		{0x58c2, 0x00},
		{0x58c3, 0x00},
		{0x58c4, 0x00},
		{0x58c5, 0x00},
		{0x58c6, 0x08},
		{0x58c7, 0x00},
		{0x58c8, 0x00},
		{0x58c9, 0x00},
		{0x58ca, 0x08},
		{0x58cb, 0x00},
		{0x58cc, 0x00},
		{0x58cd, 0x00},
		{0x58ce, 0x00},
		{0x58cf, 0x00},
		{0x58d0, 0x00},
		{0x58d1, 0x00},
		{0x58d2, 0x00},
		{0x58d3, 0x00},
		{0x58d4, 0x00},
		{0x58d5, 0x00},
		{0x58d6, 0x00},
		{0x58d7, 0x00},
		{0x58d8, 0x00},
		{0x58d9, 0x00},
		{0x58da, 0x00},
		{0x58db, 0x00},
		{0x58dc, 0x00},
		{0x58dd, 0x00},
		{0x58de, 0x00},
		{0x58df, 0x00},
		{0x58e0, 0x00},
		{0x58e1, 0x00},
		{0x58e2, 0x00},
		{0x58e3, 0x00},
		{0x58e4, 0x08},
		{0x58e5, 0x00},
		{0x58e6, 0x00},
		{0x58e7, 0x00},
		{0x58e8, 0x08},
		{0x58e9, 0x00},
		{0x58ea, 0x00},
		{0x58eb, 0x00},
		{0x58ec, 0x00},
		{0x58ed, 0x00},
		{0x58ee, 0x00},
		{0x58ef, 0x00},
		{0x58f0, 0x00},
		{0x58f1, 0x00},
		{0x58f2, 0x00},
		{0x58f3, 0x00},
		{0x58f4, 0x00},
		{0x58f5, 0x00},
		{0x58f6, 0x00},
		{0x58f7, 0x00},
		{0x58f8, 0x00},
		{0x58f9, 0x00},
		{0x58fa, 0x00},
		{0x58fb, 0x00},
		{0x58fc, 0x00},
		{0x58fd, 0x00},
		{0x58fe, 0x00},
		{0x58ff, 0x00},
		{0x5900, 0x00},
		{0x5901, 0x00},
		{0x5902, 0x00},
		{0x5903, 0x00},
		{0x5904, 0x0c},
		{0x5905, 0x00},
		{0x5906, 0x00},
		{0x5907, 0x00},
		{0x5908, 0x0c},
		{0x5909, 0x00},
		{0x590a, 0x00},
		{0x590b, 0x00},
		{0x590c, 0x00},
		{0x590d, 0x00},
		{0x590e, 0x00},
		{0x590f, 0x00},
		{0x5910, 0x00},
		{0x5911, 0x00},
		{0x5912, 0x00},
		{0x5913, 0x00},
		{0x5914, 0x00},
		{0x5915, 0x00},
		{0x5916, 0x00},
		{0x5917, 0x00},
		{0x5918, 0x00},
		{0x5919, 0x00},
		{0x591a, 0x00},
		{0x591b, 0x00},
		{0x591c, 0x00},
		{0x591d, 0x00},
		{0x591e, 0x00},
		{0x591f, 0x00},
		{0x5920, 0x00},
		{0x5921, 0x00},
		{0x5922, 0x00},
		{0x5923, 0x00},
		{0x5924, 0x00},
		{0x5925, 0x00},
		{0x5926, 0x0c},
		{0x5927, 0x00},
		{0x5928, 0x00},
		{0x5929, 0x00},
		{0x592a, 0x0c},
		{0x592b, 0x00},
		{0x592c, 0x00},
		{0x592d, 0x00},
		{0x592e, 0x00},
		{0x592f, 0x00},
		{0x5930, 0x00},
		{0x5931, 0x00},
		{0x5932, 0x00},
		{0x5933, 0x00},
		{0x5934, 0x00},
		{0x5935, 0x00},
		{0x5936, 0x00},
		{0x5937, 0x00},
		{0x5938, 0x00},
		{0x5939, 0x00},
		{0x593a, 0x00},
		{0x593b, 0x00},
		{0x593c, 0x00},
		{0x593d, 0x00},
		{0x593e, 0x00},
		{0x593f, 0x00},
		{0x5940, 0x00},
		{0x5941, 0x00},
		{0x5942, 0x00},
		{0x5943, 0x00},
		{0x5944, 0x00},
		{0x5945, 0x00},
		{0x5946, 0x08},
		{0x5947, 0x00},
		{0x5948, 0x00},
		{0x5949, 0x00},
		{0x594a, 0x08},
		{0x594b, 0x00},
		{0x594c, 0x00},
		{0x594d, 0x00},
		{0x594e, 0x00},
		{0x594f, 0x00},
		{0x5950, 0x00},
		{0x5951, 0x00},
		{0x5952, 0x00},
		{0x5953, 0x00},
		{0x5300, 0x01},
		{0x5370, 0x00},
		{0x5371, 0x00},
		{0x5372, 0x00},
		{0x5373, 0x00},
		{0x5374, 0x00},
		{0x5375, 0x00},
		{0x5376, 0x00},
		{0x5377, 0x00},
		{0x5378, 0x20},
		{0x5379, 0x00},
		{0x537a, 0x20},
		{0x537b, 0x00},
		{0x537c, 0x00},
		{0x537d, 0x00},
		{0x537e, 0x00},
		{0x537f, 0x00},
		{0x5380, 0x00},
		{0x5381, 0x00},
		{0x5382, 0x00},
		{0x5383, 0x00},
		{0x5384, 0x00},
		{0x5385, 0x00},
		{0x5386, 0x00},
		{0x5387, 0x00},
		{0x5388, 0x20},
		{0x5389, 0x00},
		{0x538a, 0x20},
		{0x538b, 0x00},
		{0x538c, 0x00},
		{0x538d, 0x00},
		{0x538e, 0x00},
		{0x538f, 0x00},
		{0x5390, 0x00},
		{0x5391, 0x00},
		{0x5392, 0x00},
		{0x5393, 0x00},
		{0x5394, 0x00},
		{0x5395, 0x00},
		{0x5396, 0x00},
		{0x5397, 0x00},
		{0x5398, 0x00},
		{0x5399, 0x20},
		{0x539a, 0x00},
		{0x539b, 0x20},
		{0x539c, 0x00},
		{0x539d, 0x00},
		{0x539e, 0x00},
		{0x539f, 0x00},
		{0x53a0, 0x00},
		{0x53a1, 0x00},
		{0x53a2, 0x00},
		{0x53a3, 0x00},
		{0x53a4, 0x00},
		{0x53a5, 0x00},
		{0x53a6, 0x00},
		{0x53a7, 0x00},
		{0x53a8, 0x00},
		{0x53a9, 0x20},
		{0x53aa, 0x00},
		{0x53ab, 0x20},
		{0x53ac, 0x00},
		{0x53ad, 0x00},
		{0x53ae, 0x00},
		{0x53af, 0x00},
		{0x53b0, 0x00},
		{0x53b1, 0x00},
		{0x53b2, 0x00},
		{0x53b3, 0x00},
		{0x53b4, 0x00},
		{0x53b5, 0x00},
		{0x53b6, 0x00},
		{0x53b7, 0x00},
		{0x53b8, 0x20},
		{0x53b9, 0x00},
		{0x53ba, 0x20},
		{0x53bb, 0x00},
		{0x53bc, 0x00},
		{0x53bd, 0x00},
		{0x53be, 0x00},
		{0x53bf, 0x00},
		{0x53c0, 0x00},
		{0x53c1, 0x00},
		{0x53c2, 0x00},
		{0x53c3, 0x00},
		{0x53c4, 0x00},
		{0x53c5, 0x00},
		{0x53c6, 0x00},
		{0x53c7, 0x00},
		{0x53c8, 0x20},
		{0x53c9, 0x00},
		{0x53ca, 0x20},
		{0x53cb, 0x00},
		{0x53cc, 0x00},
		{0x53cd, 0x00},
		{0x53ce, 0x00},
		{0x53cf, 0x00},
		{0x53d0, 0x00},
		{0x53d1, 0x00},
		{0x53d2, 0x00},
		{0x53d3, 0x00},
		{0x53d4, 0x00},
		{0x53d5, 0x00},
		{0x53d6, 0x00},
		{0x53d7, 0x00},
		{0x53d8, 0x00},
		{0x53d9, 0x20},
		{0x53da, 0x00},
		{0x53db, 0x20},
		{0x53dc, 0x00},
		{0x53dd, 0x00},
		{0x53de, 0x00},
		{0x53df, 0x00},
		{0x53e0, 0x00},
		{0x53e1, 0x00},
		{0x53e2, 0x00},
		{0x53e3, 0x00},
		{0x53e4, 0x00},
		{0x53e5, 0x00},
		{0x53e6, 0x00},
		{0x53e7, 0x00},
		{0x53e8, 0x00},
		{0x53e9, 0x20},
		{0x53ea, 0x00},
		{0x53eb, 0x20},
		{0x53ec, 0x00},
		{0x53ed, 0x00},
		{0x53ee, 0x00},
		{0x53ef, 0x00},
		{0x53f0, 0x00},
		{0x53f1, 0x00},
		{0x53f2, 0x00},
		{0x53f3, 0x00},
		{0x53f4, 0x20},
		{0x53f5, 0x00},
		{0x53f6, 0x20},
		{0x53f7, 0x00},
		{0x53f8, 0x70},
		{0x53f9, 0x00},
		{0x53fa, 0x70},
		{0x53fb, 0x00},
		{0x53fc, 0x20},
		{0x53fd, 0x00},
		{0x53fe, 0x20},
		{0x53ff, 0x00},
		{0x5400, 0x00},
		{0x5401, 0x00},
		{0x5402, 0x00},
		{0x5403, 0x00},
		{0x5404, 0x20},
		{0x5405, 0x00},
		{0x5406, 0x20},
		{0x5407, 0x00},
		{0x5408, 0x70},
		{0x5409, 0x00},
		{0x540a, 0x70},
		{0x540b, 0x00},
		{0x540c, 0x20},
		{0x540d, 0x00},
		{0x540e, 0x20},
		{0x540f, 0x00},
		{0x5410, 0x00},
		{0x5411, 0x00},
		{0x5412, 0x00},
		{0x5413, 0x00},
		{0x5414, 0x00},
		{0x5415, 0x20},
		{0x5416, 0x00},
		{0x5417, 0x20},
		{0x5418, 0x00},
		{0x5419, 0x70},
		{0x541a, 0x00},
		{0x541b, 0x70},
		{0x541c, 0x00},
		{0x541d, 0x20},
		{0x541e, 0x00},
		{0x541f, 0x20},
		{0x5420, 0x00},
		{0x5421, 0x00},
		{0x5422, 0x00},
		{0x5423, 0x00},
		{0x5424, 0x00},
		{0x5425, 0x20},
		{0x5426, 0x00},
		{0x5427, 0x20},
		{0x5428, 0x00},
		{0x5429, 0x70},
		{0x542a, 0x00},
		{0x542b, 0x70},
		{0x542c, 0x00},
		{0x542d, 0x20},
		{0x542e, 0x00},
		{0x542f, 0x20},
		{0x5430, 0x00},
		{0x5431, 0x00},
		{0x5432, 0x00},
		{0x5433, 0x00},
		{0x5434, 0x20},
		{0x5435, 0x00},
		{0x5436, 0x20},
		{0x5437, 0x00},
		{0x5438, 0x70},
		{0x5439, 0x00},
		{0x543a, 0x70},
		{0x543b, 0x00},
		{0x543c, 0x20},
		{0x543d, 0x00},
		{0x543e, 0x20},
		{0x543f, 0x00},
		{0x5440, 0x00},
		{0x5441, 0x00},
		{0x5442, 0x00},
		{0x5443, 0x00},
		{0x5444, 0x20},
		{0x5445, 0x00},
		{0x5446, 0x20},
		{0x5447, 0x00},
		{0x5448, 0x70},
		{0x5449, 0x00},
		{0x544a, 0x70},
		{0x544b, 0x00},
		{0x544c, 0x20},
		{0x544d, 0x00},
		{0x544e, 0x20},
		{0x544f, 0x00},
		{0x5450, 0x00},
		{0x5451, 0x00},
		{0x5452, 0x00},
		{0x5453, 0x00},
		{0x5454, 0x00},
		{0x5455, 0x20},
		{0x5456, 0x00},
		{0x5457, 0x20},
		{0x5458, 0x00},
		{0x5459, 0x70},
		{0x545a, 0x00},
		{0x545b, 0x70},
		{0x545c, 0x00},
		{0x545d, 0x20},
		{0x545e, 0x00},
		{0x545f, 0x20},
		{0x5460, 0x00},
		{0x5461, 0x00},
		{0x5462, 0x00},
		{0x5463, 0x00},
		{0x5464, 0x00},
		{0x5465, 0x20},
		{0x5466, 0x00},
		{0x5467, 0x20},
		{0x5468, 0x00},
		{0x5469, 0x70},
		{0x546a, 0x00},
		{0x546b, 0x70},
		{0x546c, 0x00},
		{0x546d, 0x20},
		{0x546e, 0x00},
		{0x546f, 0x20},
		{0x5954, 0x04},
		{0x5a90, 0x01},
		{0x5a10, 0x00},
		{0x5a11, 0x00},
		{0x5a12, 0x00},
		{0x5a13, 0x00},
		{0x5a14, 0x00},
		{0x5a15, 0x00},
		{0x5a16, 0x00},
		{0x5a17, 0x00},
		{0x5a18, 0x20},
		{0x5a19, 0x00},
		{0x5a1a, 0x20},
		{0x5a1b, 0x00},
		{0x5a1c, 0x00},
		{0x5a1d, 0x00},
		{0x5a1e, 0x00},
		{0x5a1f, 0x00},
		{0x5a20, 0x00},
		{0x5a21, 0x00},
		{0x5a22, 0x00},
		{0x5a23, 0x00},
		{0x5a24, 0x00},
		{0x5a25, 0x00},
		{0x5a26, 0x00},
		{0x5a27, 0x00},
		{0x5a28, 0x20},
		{0x5a29, 0x00},
		{0x5a2a, 0x20},
		{0x5a2b, 0x00},
		{0x5a2c, 0x00},
		{0x5a2d, 0x00},
		{0x5a2e, 0x00},
		{0x5a2f, 0x00},
		{0x5a30, 0x00},
		{0x5a31, 0x00},
		{0x5a32, 0x00},
		{0x5a33, 0x00},
		{0x5a34, 0x00},
		{0x5a35, 0x00},
		{0x5a36, 0x00},
		{0x5a37, 0x00},
		{0x5a38, 0x00},
		{0x5a39, 0x20},
		{0x5a3a, 0x00},
		{0x5a3b, 0x20},
		{0x5a3c, 0x00},
		{0x5a3d, 0x00},
		{0x5a3e, 0x00},
		{0x5a3f, 0x00},
		{0x5a40, 0x00},
		{0x5a41, 0x00},
		{0x5a42, 0x00},
		{0x5a43, 0x00},
		{0x5a44, 0x00},
		{0x5a45, 0x00},
		{0x5a46, 0x00},
		{0x5a47, 0x00},
		{0x5a48, 0x00},
		{0x5a49, 0x20},
		{0x5a4a, 0x00},
		{0x5a4b, 0x20},
		{0x5a4c, 0x00},
		{0x5a4d, 0x00},
		{0x5a4e, 0x00},
		{0x5a4f, 0x00},
		{0x5a50, 0x00},
		{0x5a51, 0x00},
		{0x5a52, 0x00},
		{0x5a53, 0x00},
		{0x5a54, 0x00},
		{0x5a55, 0x00},
		{0x5a56, 0x00},
		{0x5a57, 0x00},
		{0x5a58, 0x20},
		{0x5a59, 0x00},
		{0x5a5a, 0x20},
		{0x5a5b, 0x00},
		{0x5a5c, 0x00},
		{0x5a5d, 0x00},
		{0x5a5e, 0x00},
		{0x5a5f, 0x00},
		{0x5a60, 0x00},
		{0x5a61, 0x00},
		{0x5a62, 0x00},
		{0x5a63, 0x00},
		{0x5a64, 0x00},
		{0x5a65, 0x00},
		{0x5a66, 0x00},
		{0x5a67, 0x00},
		{0x5a68, 0x20},
		{0x5a69, 0x00},
		{0x5a6a, 0x20},
		{0x5a6b, 0x00},
		{0x5a6c, 0x00},
		{0x5a6d, 0x00},
		{0x5a6e, 0x00},
		{0x5a6f, 0x00},
		{0x5a70, 0x00},
		{0x5a71, 0x00},
		{0x5a72, 0x00},
		{0x5a73, 0x00},
		{0x5a74, 0x00},
		{0x5a75, 0x00},
		{0x5a76, 0x00},
		{0x5a77, 0x00},
		{0x5a78, 0x00},
		{0x5a79, 0x20},
		{0x5a7a, 0x00},
		{0x5a7b, 0x20},
		{0x5a7c, 0x00},
		{0x5a7d, 0x00},
		{0x5a7e, 0x00},
		{0x5a7f, 0x00},
		{0x5a80, 0x00},
		{0x5a81, 0x00},
		{0x5a82, 0x00},
		{0x5a83, 0x00},
		{0x5a84, 0x00},
		{0x5a85, 0x00},
		{0x5a86, 0x00},
		{0x5a87, 0x00},
		{0x5a88, 0x00},
		{0x5a89, 0x20},
		{0x5a8a, 0x00},
		{0x5a8b, 0x20},
		{0x5a8c, 0x00},
		{0x5a8d, 0x00},
		{0x5a8e, 0x00},
		{0x5a8f, 0x00},
		{0x5280, 0x00},
		{0x5281, 0x10},
		{0x5282, 0x0f},
		{0x5283, 0x37},
		{0x5285, 0x07},
		{0x5500, 0xbf},
		{0x5501, 0xf3},
		{0x5502, 0x4d},
		{0x5503, 0x1b},
		{0x5504, 0xe0},
		{0x5505, 0x10},
		{0x5506, 0x3f},
		{0x5507, 0x30},
		{0x5508, 0x04},
		{0x5509, 0x0f},
		{0x550a, 0x43},
		{0x5e01, 0xf3},
		{0x5e02, 0x4d},
		{0x5e03, 0x1b},
		{0x5e04, 0xe0},
		{0x5e05, 0x10},
		{0x5e06, 0x3f},
		{0x5e07, 0x30},
		{0x5e08, 0x04},
		{0x5e09, 0x0f},
		{0x5e0a, 0x43},
		{0x3025, 0x03},
		{0x3664, 0x03},
		{0x3668, 0xf0},
		{0x3669, 0x0e},
		{0x3406, 0x08},
		{0x3408, 0x03},
		{0x0304, 0x13},
		{0x4837, 0x0e},
		{0x5001, 0x08},
		{0x5580, 0x00},
		{0x5581, 0x04},
		{0x5582, 0x00},
		{0x5583, 0x08},
		{0x5584, 0x00},
		{0x5585, 0x10},
		{0x5586, 0x00},
		{0x5587, 0x20},
		{0x5588, 0x00},
		{0x5589, 0x40},
		{0x558a, 0x00},
		{0x558b, 0x80},
		{0x558c, 0x13},
		{0x558d, 0x12},
		{0x558e, 0x10},
		{0x558f, 0x0b},
		{0x5590, 0x0a},
		{0x5591, 0x0a},
		{0x5592, 0x13},
		{0x5593, 0x12},
		{0x5594, 0x10},
		{0x5595, 0x0b},
		{0x5596, 0x0a},
		{0x5597, 0x0a},
		{0x3508, 0x07},
		{0x3509, 0xc0},
		{0x350c, 0x07},
		{0x350d, 0xc0},
		{0x401a, 0x40},
};

// 4144x3106 30FPS
// frame length	3328
// 1H time	100
static const SENSOR_REG_T ov13870_4144x3106_4lane_setting[] = {
		{0x0300, 0xf8},
		{0x0303, 0x00},
		{0x0304, 0x13},
		{0x0317, 0x00},
		{0x3016, 0xb4},
		{0x3018, 0xf0},
		{0x3400, 0x04},
		{0x340c, 0xff},
		{0x340d, 0xff},
		{0x350d, 0xc0},
		{0x3600, 0x00},
		{0x3602, 0x0e},
		{0x360b, 0x10},
		{0x3621, 0x88},
		{0x3662, 0x00},
		{0x3666, 0xa7},
		{0x366c, 0x54},
		{0x3704, 0x28},
		{0x3706, 0x3c},
		{0x3709, 0xce},
		{0x370b, 0xbc},
		{0x37f5, 0x41},
		{0x3808, 0x10},
		{0x3809, 0x30},
		{0x380a, 0x0c},
		{0x380b, 0x22},
		{0x380c, 0x07},
		{0x380d, 0x1a},
		{0x380e, 0x0c},
		{0x380f, 0xe4},
		{0x3813, 0x08},
		{0x3814, 0x11},
		{0x3815, 0x11},
		{0x3820, 0x00},
		{0x3821, 0x04},
		{0x383c, 0xa8},
		{0x4016, 0x1f},
		{0x4018, 0x1f},
		{0x401e, 0x02},
		{0x401f, 0x00},
		{0x430f, 0x02},
		{0x4501, 0x08},
		{0x4837, 0x0e},
		{0x5000, 0xa7},
		{0x5001, 0x08},
		{0x5017, 0xeb},
		{0x55ca, 0x03},
		{0x55cb, 0x01},
		{0x5500, 0xbf},
};


static const SENSOR_REG_T ov13870_2112x1568_4lane_setting[] = {
		{0x0300, 0xf9},
		{0x0303, 0x10},
		{0x0304, 0x25},
		{0x0317, 0x01},
		{0x3016, 0x96},
		{0x3018, 0x70},
		{0x3400, 0x00},
		{0x340c, 0x02},
		{0x340d, 0x78},
		{0x350d, 0xff},
		{0x3600, 0x18},
		{0x3602, 0x0f},
		{0x360b, 0x0f},
		{0x3621, 0x8f},
		{0x3662, 0x40},
		{0x3666, 0xa5},
		{0x366c, 0x04},
		{0x3704, 0x26},
		{0x3706, 0x38},
		{0x3709, 0x80},
		{0x370b, 0xac},
		{0x37f5, 0x20},
		{0x3808, 0x08},
		{0x3809, 0x40},
		{0x380a, 0x06},
		{0x380b, 0x20},
		{0x380c, 0x04},
		{0x380d, 0x8c},
		{0x380e, 0x0d},
		{0x380f, 0x68},
		{0x3813, 0x0c},
		{0x3814, 0x31},
		{0x3815, 0x31},
		{0x3820, 0x01},
		{0x3821, 0x44},
		{0x383c, 0x88},
		{0x4016, 0x0f},
		{0x4018, 0x0f},
		{0x401e, 0x01},
		{0x401f, 0xa0},
		{0x430f, 0x0a},
		{0x4501, 0x00},
		{0x4837, 0x18},
		{0x5000, 0x86},
		{0x5001, 0x00},
		{0x5017, 0xe3},
		{0x55ca, 0x07},
		{0x55cb, 0x03},
		{0x5500, 0xbc},
};

/*@@for, video, 240fps
{0x380e, 0x04},
{0x380f, 0xca},

@@, for, video, 180fps
{0x380e, 0x06},
{0x380f, 0x62},

@@, for, video, 120fps
{0x380e, 0x09},
{0x380f, 0x92},*/
// 180fps
static const SENSOR_REG_T ov13870_1280x720_4lane_setting[] = {
		{0x0300, 0xf8},
		{0x0303, 0x00},
		{0x0304, 0x25},
		{0x0317, 0x00},
		{0x3016, 0x96},
		{0x3018, 0x70},
		{0x3400, 0x00},
		{0x340c, 0x02},
		{0x340d, 0x78},
		{0x350d, 0xff},
		{0x3600, 0x0c},
		{0x3602, 0x0f},
		{0x360b, 0x0f},
		{0x3621, 0x8f},
		{0x3662, 0x40},
		{0x3666, 0xa5},
		{0x366c, 0x04},
		{0x3704, 0x22},
		{0x3706, 0x38},
		{0x3709, 0x40},
		{0x370b, 0xac},
		{0x37f5, 0x20},
		{0x3808, 0x05},
		{0x3809, 0x00},
		{0x380a, 0x02},
		{0x380b, 0xD0},
		{0x380c, 0x02},
		{0x380d, 0x64},
		{0x380e, 0x06},
		{0x380f, 0x62},
		{0x3813, 0x0c},
		{0x3814, 0x31},
		{0x3815, 0x31},
		{0x3820, 0x01},
		{0x3821, 0x05},
		{0x383c, 0x88},
		{0x4016, 0x0f},
		{0x4018, 0x0f},
		{0x401e, 0x01},
		{0x401f, 0xa0},
		{0x430f, 0x0a},
		{0x4501, 0x08},
		{0x4837, 0x08},
		{0x5000, 0x86},
		{0x5001, 0x00},
		{0x5017, 0xf3},
		{0x55ca, 0x07},
		{0x55cb, 0x03},
		{0x5500, 0xbc},
};

static SENSOR_REG_TAB_INFO_T s_ov13870_resolution_Tab_RAW[9] = {
	{ADDR_AND_LEN_OF_ARRAY(ov13870_common_init), 0, 0, 24, SENSOR_IMAGE_FORMAT_RAW},
	{ADDR_AND_LEN_OF_ARRAY(ov13870_1280x720_4lane_setting), 1280, 720, 24, SENSOR_IMAGE_FORMAT_RAW},
	//{ADDR_AND_LEN_OF_ARRAY(ov13870_2112x1568_4lane_setting), 2112, 1568, 24, SENSOR_IMAGE_FORMAT_RAW},
	{ADDR_AND_LEN_OF_ARRAY(ov13870_4144x3106_4lane_setting), 4144, 3106, 24, SENSOR_IMAGE_FORMAT_RAW},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
};

static SENSOR_TRIM_T s_ov13870_Resolution_Trim_Tab[SENSOR_MODE_MAX] = {
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 1280, 720, 3416, 1280, 1634, {0, 0,  1280, 720}},
	//{0, 0, 2112, 1568, 10000, 640, 3328, {0, 0,  2112, 1568}},
	{0, 0, 4144, 3106, 10000, 1200, 3328, {0, 0, 4144, 3106}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
};

static const SENSOR_REG_T s_ov13870_2576x1932_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
	/*video mode 0: ?fps*/
	{
		{0xffff, 0xff}
	},
	/* video mode 1:?fps*/
	{
		{0xffff, 0xff}
	},
	/* video mode 2:?fps*/
	{
		{0xffff, 0xff}
	},
	/* video mode 3:?fps*/
	{
		{0xffff, 0xff}
	}
};

static SENSOR_VIDEO_INFO_T s_ov13870_video_info[] = {
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{30, 30, 164, 100}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},(SENSOR_REG_T**)s_ov13870_2576x1932_video_tab},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL}
};

static SENSOR_IOCTL_FUNC_TAB_T s_ov13870_ioctl_func_tab = {
	PNULL,
	_ov13870_PowerOn,
	PNULL,
	_ov13870_Identify,

	PNULL,// write register
	PNULL,// read  register
	PNULL,
	_ov13870_GetResolutionTrimTab,

	// External
	PNULL,
	PNULL,
	PNULL,

	PNULL,//_ov13870_set_brightness,
	PNULL,// _ov13870_set_contrast,
	PNULL,
	PNULL,//_ov13870_set_saturation,

	PNULL,//_ov13870_set_work_mode,
	PNULL,//_ov13870_set_image_effect,

	PNULL,//_ov13870_BeforeSnapshot,
	PNULL,//_ov13870_after_snapshot,
	PNULL,//_ov13870_flash,
	PNULL,//read_ae_value
	PNULL,//_ov13870_write_exposure,
	PNULL,//read_gain_value
	_ov13870_write_gain,
	PNULL,//read_gain_scale
	PNULL,//set_frame_rate
	PNULL,//ov13870_write_af,
	PNULL,
	PNULL,//_ov13870_set_awb,
	PNULL,
	PNULL,
	PNULL,//_ov13870_set_ev,
	PNULL,
	PNULL,
	PNULL,
	_ov13870_GetExifInfo,
	PNULL,//_ov13870_ExtFunc,
	PNULL,//_ov13870_set_anti_flicker,
	PNULL,//_ov13870_set_video_mode,
	PNULL,//pick_jpeg_stream
	PNULL,//meter_mode
	PNULL,//get_status
	_ov13870_StreamOn,
	_ov13870_StreamOff,
	_ov13870_access_val, //ov13870_cfg_otp,
	_ov13870_ex_write_exposure
};

static SENSOR_STATIC_INFO_T s_ov13870_static_info = {
	200,	//f-number,focal ratio
	357,	//focal_length;
	0,	//max_fps,max fps of sensor's all settings,it will be calculated from sensor mode fps
	16,	//max_adgain,AD-gain
	0,	//ois_supported;
	0,	//pdaf_supported;
	1,	//exp_valid_frame_num;N+2-1
	64,	//clamp_level,black level
	1,	//adgain_valid_frame_num;N+1-1
};

static SENSOR_MODE_FPS_INFO_T s_ov13870_mode_fps_info = {
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

SENSOR_INFO_T g_ov13870_mipi_raw_info = {
	ov13870_I2C_ADDR_W,	// salve i2c write address
	ov13870_I2C_ADDR_R,	// salve i2c read address

	SENSOR_I2C_REG_16BIT | SENSOR_I2C_VAL_8BIT | SENSOR_I2C_FREQ_400,	// bit0: 0: i2c register value is 8 bit, 1: i2c register value is 16 bit
	SENSOR_HW_SIGNAL_PCLK_N | SENSOR_HW_SIGNAL_VSYNC_N | SENSOR_HW_SIGNAL_HSYNC_P,	// bit0: 0:negative; 1:positive -> polarily of pixel clock
	// bit2: 0:negative; 1:positive -> polarily of horizontal synchronization signal
	// bit4: 0:negative; 1:positive -> polarily of vertical synchronization signal
	// other bit: reseved

	// preview mode
	SENSOR_ENVIROMENT_NORMAL | SENSOR_ENVIROMENT_NIGHT,

	// image effect
	SENSOR_IMAGE_EFFECT_NORMAL |
	    SENSOR_IMAGE_EFFECT_BLACKWHITE |
	    SENSOR_IMAGE_EFFECT_RED |
	    SENSOR_IMAGE_EFFECT_GREEN |
	    SENSOR_IMAGE_EFFECT_BLUE |
	    SENSOR_IMAGE_EFFECT_YELLOW |
	    SENSOR_IMAGE_EFFECT_NEGATIVE | SENSOR_IMAGE_EFFECT_CANVAS,

	// while balance mode
	0,

	7,			// bit[0:7]: count of step in brightness, contrast, sharpness, saturation
	// bit[8:31] reseved

	SENSOR_LOW_PULSE_RESET,	// reset pulse level
	5,			// reset pulse width(ms)

	SENSOR_LOW_LEVEL_PWDN,	// 1: high level valid; 0: low level valid

	1,			// count of identify code
	{{0x300A, 0x01},		// supply two code to identify sensor.
	{0x300B, 0x38}},		// for Example: index = 0-> Device id, index = 1 -> version id

//	SENSOR_AVDD_3000MV,	// voltage of avdd
	SENSOR_AVDD_2800MV,	// voltage of avdd

	4144,			// max width of source image
	3106,			// max height of source image
	(cmr_s8 *)"ov13870_mipi_raw",		// name of sensor

	SENSOR_IMAGE_FORMAT_RAW,	// define in SENSOR_IMAGE_FORMAT_E enum,SENSOR_IMAGE_FORMAT_MAX
	// if set to SENSOR_IMAGE_FORMAT_MAX here, image format depent on SENSOR_REG_TAB_INFO_T

	SENSOR_IMAGE_PATTERN_RAWRGB_B,//SENSOR_IMAGE_PATTERN_RAWRGB_R,// pattern of input image form sensor;

	s_ov13870_resolution_Tab_RAW,	// point to resolution table information structure
	&s_ov13870_ioctl_func_tab,	// point to ioctl function table
	&s_ov13870_mipi_raw_info_ptr,		// information and table about Rawrgb sensor
	NULL,			//&g_ov13870_ext_info,                // extend information about sensor
	SENSOR_AVDD_1800MV,	// iovdd
	SENSOR_AVDD_1200MV,	// dvdd
	3,			// skip frame num before preview
	3,			// skip frame num before capture
	0,			// deci frame num during preview
	0,			// deci frame num during video preview

	0,
	0,
	0,
	0,
	0,
	{SENSOR_INTERFACE_TYPE_CSI2, 4, 10, 0}, // csi, 4lanes, 10bits, 0:packed, 1: half-word
	s_ov13870_video_info,
	3,			// skip frame num while change setting
	48,			// horizontal view angle
	48,			// vertical view angle
	(cmr_s8 *)"ov13870_truly_v1",		// version info of sensor
};

static struct sensor_raw_info* Sensor_GetContext(SENSOR_HW_HANDLE handle)
{
	return s_ov13870_mipi_raw_info_ptr;
}

#define param_update(x1,x2) sprintf(name,"/data/ov13870_%s.bin",x1);\
				if(0==access(name,R_OK))\
				{\
					FILE* fp = NULL;\
					SENSOR_PRINT("param file %s exists",name);\
					if( NULL!=(fp=fopen(name,"rb")) ){\
						fread((void*)x2,1,sizeof(x2),fp);\
						fclose(fp);\
					}else{\
						SENSOR_PRINT("param open %s failure",name);\
					}\
				}\
				memset(name,0,sizeof(name))

static unsigned long _ov13870_GetResolutionTrimTab(SENSOR_HW_HANDLE handle, unsigned long param)
{
	SENSOR_PRINT("0x%lx",  (unsigned long)s_ov13870_Resolution_Trim_Tab);
	return (unsigned long) s_ov13870_Resolution_Trim_Tab;
}

static uint32_t _ov13870_init_mode_fps_info(SENSOR_HW_HANDLE handle)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_PRINT("_ov13870_init_mode_fps_info:E");
	if(!s_ov13870_mode_fps_info.is_init) {
		uint32_t i,modn,tempfps = 0;
		SENSOR_PRINT("_ov13870_init_mode_fps_info:start init");
		for(i = 0;i < NUMBER_OF_ARRAY(s_ov13870_Resolution_Trim_Tab); i++) {
			//max fps should be multiple of 30,it calulated from line_time and frame_line
			tempfps = s_ov13870_Resolution_Trim_Tab[i].line_time*s_ov13870_Resolution_Trim_Tab[i].frame_line;
			if(0 != tempfps) {
				tempfps = 1000000000/tempfps;
				modn = tempfps / 30;
				if(tempfps > modn*30)
					modn++;
				s_ov13870_mode_fps_info.sensor_mode_fps[i].max_fps = modn*30;
				if(s_ov13870_mode_fps_info.sensor_mode_fps[i].max_fps > 30) {
					s_ov13870_mode_fps_info.sensor_mode_fps[i].is_high_fps = 1;
					s_ov13870_mode_fps_info.sensor_mode_fps[i].high_fps_skip_num =
						s_ov13870_mode_fps_info.sensor_mode_fps[i].max_fps/30;
				}
				if(s_ov13870_mode_fps_info.sensor_mode_fps[i].max_fps >
						s_ov13870_static_info.max_fps) {
					s_ov13870_static_info.max_fps =
						s_ov13870_mode_fps_info.sensor_mode_fps[i].max_fps;
				}
			}
			SENSOR_PRINT("mode %d,tempfps %d,frame_len %d,line_time: %d ",i,tempfps,
					s_ov13870_Resolution_Trim_Tab[i].frame_line,
					s_ov13870_Resolution_Trim_Tab[i].line_time);
			SENSOR_PRINT("mode %d,max_fps: %d ",
					i,s_ov13870_mode_fps_info.sensor_mode_fps[i].max_fps);
			SENSOR_PRINT("is_high_fps: %d,highfps_skip_num %d",
					s_ov13870_mode_fps_info.sensor_mode_fps[i].is_high_fps,
					s_ov13870_mode_fps_info.sensor_mode_fps[i].high_fps_skip_num);
		}
		s_ov13870_mode_fps_info.is_init = 1;
	}
	SENSOR_PRINT("_ov13870_init_mode_fps_info:X");
	return rtn;
}

SENSOR_HW_HANDLE _ov13870_Create(void *privatedata)
{
	SENSOR_PRINT("_ov13870_Create  IN");
	if (NULL == privatedata) {
		SENSOR_PRINT("_ov13870_Create invalied para");
		return NULL;
	}
	SENSOR_HW_HANDLE sensor_hw_handle = (SENSOR_HW_HANDLE)malloc(sizeof(SENSOR_HW_HANDLE));
	if (NULL == sensor_hw_handle) {
		SENSOR_PRINT("failed to create sensor_hw_handle");
		return NULL;
	}
	cmr_bzero(sensor_hw_handle, sizeof(SENSOR_HW_HANDLE));
	sensor_hw_handle->privatedata = privatedata;

	return sensor_hw_handle;
}

void _ov13870_Destroy(SENSOR_HW_HANDLE handle)
{
	SENSOR_PRINT("_ov13870_Destroy  IN");
	if (NULL == handle) {
		SENSOR_PRINT("_ov13870_Destroy handle null");
	}
	free((void*)handle->privatedata);
	handle->privatedata = NULL;
}

static unsigned long _ov13870_Identify(SENSOR_HW_HANDLE handle, unsigned long param)
{
	uint16_t chip_id0 = 0x00;
	uint16_t chip_id1 = 0x00;
	uint16_t chip_id2 = 0x00;
	uint32_t ret_value = SENSOR_FAIL;
	uint16_t i=0;
	uint16_t ret;

	SENSOR_PRINT_ERR("SENSOR_ov13870: mipi raw identify\n");

	chip_id0 = Sensor_ReadReg(0x300A);
	chip_id1 = Sensor_ReadReg(0x300B);
	chip_id2 = Sensor_ReadReg(0x300C);

	if (0x01 == chip_id0 && 0x38 == chip_id1 && 0x70 == chip_id2) {
		SENSOR_PRINT_ERR("SENSOR_ov13870: Identify: chip_id:%x%x%x", chip_id0, chip_id1,chip_id2);
		SENSOR_PRINT_ERR("SENSOR_ov13870: this is ov13870 sensor !");
		ret_value=_ov13870_GetRawInof(handle);
		if (SENSOR_SUCCESS != ret_value) {
			SENSOR_PRINT_ERR("SENSOR_ov13870: the module is unknow error !");
		}
		/*bu64297gwz_init(handle);*/
		_ov13870_init_mode_fps_info(handle);
	} else {
		SENSOR_PRINT_ERR("SENSOR_ov13870: identify fail,chip_id:%x,%x,%x", chip_id0, chip_id1,chip_id2);
	}

	return ret_value;
}

static uint32_t _ov13870_GetMaxFrameLine(uint32_t index)
{
	uint32_t max_line=0x00;
	SENSOR_TRIM_T_PTR trim_ptr=s_ov13870_Resolution_Trim_Tab;

	max_line=trim_ptr[index].frame_line;

	return max_line;
}

static uint32_t _ov13870_get_VTS(SENSOR_HW_HANDLE handle)
{
	// read VTS from register settings
	uint32_t VTS;

	VTS = Sensor_ReadReg(0x380e);//total vertical size[15:8] high byte
	VTS = ((VTS & 0x7f)<<8) + Sensor_ReadReg(0x380f);

	return VTS;
}

static unsigned long _ov13870_set_VTS(SENSOR_HW_HANDLE handle,  unsigned long VTS)
{
	// write VTS to registers
	uint16_t temp = 0;


	temp = (VTS >> 8) & 0x7f;
	Sensor_WriteReg(0x380e, temp);

	temp = VTS & 0xff;
	Sensor_WriteReg(0x380f, temp);
	return 0;
}

LOCAL unsigned long  OV13870_set_shutter(SENSOR_HW_HANDLE handle, unsigned long shutter)
{
	// write shutter, in number of line period
	uint16_t temp = 0;

	shutter = shutter & 0xffff;
	temp = shutter & 0xff;
	Sensor_WriteReg(0x3502, temp);

	temp = (shutter >> 8) & 0xff;
	Sensor_WriteReg(0x3501, temp);

	return 0;
}

static unsigned long _ov13870_write_exp_dummy(SENSOR_HW_HANDLE handle, uint16_t expsure_line,
								uint16_t dummy_line, uint16_t size_index)

{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint32_t frame_len_cur = 0;
	uint32_t frame_len = 0;
	uint32_t max_frame_len = 0;
	uint32_t linetime = 0;
	uint32_t frame_offset = 8;
	uint32_t offset = 0;


	SENSOR_PRINT("exp line:%d, dummy:%d, size_index:%d",
				expsure_line, dummy_line, size_index);

	max_frame_len =_ov13870_GetMaxFrameLine(size_index);
	if (0 != max_frame_len) {
		if (dummy_line > frame_offset)
			offset = dummy_line;
		else
			offset = frame_offset;
		frame_len = ((expsure_line + offset) > max_frame_len) ? (expsure_line + offset) : max_frame_len;

		frame_len_cur = _ov13870_get_VTS(handle);

		SENSOR_PRINT("frame_len: %d, frame_len_cur:%d", frame_len, frame_len_cur);

		if(frame_len_cur != frame_len){
			_ov13870_set_VTS(handle, frame_len);
		}
	}
	OV13870_set_shutter(handle, expsure_line);

	s_capture_shutter = expsure_line;
	linetime=s_ov13870_Resolution_Trim_Tab[size_index].line_time;
	s_exposure_time = s_capture_shutter * linetime / 1000;
	Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_EXPOSURETIME, s_capture_shutter);

	return ret_value;
}

static cmr_u8 ov13870_i2c_read_otp_set(SENSOR_HW_HANDLE handle, cmr_u16 addr)
{
	return sensor_grc_read_i2c(0xA0 >> 1, addr, BITS_ADDR16_REG8);
}

static int ov13870_otp_init(SENSOR_HW_HANDLE handle)
{
	return 0;
}

static int ov13870_otp_read_data(SENSOR_HW_HANDLE handle)
{
	cmr_u16 i = 0;
	cmr_u8 high_val = 0;
	cmr_u8 low_val = 0;
	cmr_u32 checksum = 0;

	return 0;
}

static unsigned long ov13870_otp_read(SENSOR_HW_HANDLE handle, SENSOR_VAL_T* param)
{
	struct sensor_otp_cust_info *otp_info = NULL;

	return 0;
}

static unsigned long ov13870_parse_otp(SENSOR_HW_HANDLE handle, SENSOR_VAL_T* param)
{
	cmr_u8 *buff = NULL;
	struct sensor_otp_cust_info *otp_info = NULL;
	cmr_u16 i = 0;
	cmr_u16 j = 0;
	cmr_u8 high_val = 0;
	cmr_u8 low_val = 0;
	cmr_u32 checksum = 0;

	return 0;
}

static unsigned long _ov13870_ex_write_exposure(SENSOR_HW_HANDLE handle, unsigned long param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t exposure_line = 0x00;
	uint16_t dummy_line = 0x00;
	uint16_t size_index=0x00;
	struct sensor_ex_exposure  *ex = (struct sensor_ex_exposure*)param;

	if (!param) {
		SENSOR_PRINT_ERR("param is NULL !!!");
		return ret_value;
	}

	exposure_line = ex->exposure;
	dummy_line = ex->dummy;
	size_index = ex->size_index;

	ret_value = _ov13870_write_exp_dummy(handle, exposure_line, dummy_line, size_index);

	return ret_value;
}

static unsigned long _ov13870_write_gain(SENSOR_HW_HANDLE handle, unsigned long param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t value = 0x00;
	uint32_t real_gain = 0;
	uint32_t a_gain = 0;
	uint32_t d_gain = 0;


	real_gain = param >> 2;

	SENSOR_PRINT("SENSOR_OV13870: real_gain:0x%x, param: 0x%x", real_gain, param);

	if (real_gain < 16 * 32) {
		a_gain = real_gain;
		d_gain = 1 * 32;
	} else {
		a_gain = 512;
		d_gain = real_gain / 256;
	}

	value = (a_gain >> 8) & 0xff;
	Sensor_WriteReg(0x3508, value);
	value = a_gain & 0xff;
	Sensor_WriteReg(0x3509, value);


#if 0
	value = (d_gain >> 8) & 0x0f;
	Sensor_WriteReg(0x350e, value);
	value = d_gain & 0xff;
	Sensor_WriteReg(0x350f, value);
#endif

	return ret_value;
}

static unsigned long ov13870_write_af(SENSOR_HW_HANDLE handle, unsigned long param)
{

	return 0;
}


static unsigned long _ov13870_BeforeSnapshot(SENSOR_HW_HANDLE handle, unsigned long param)
{
	uint8_t ret_l, ret_m, ret_h;
	uint32_t capture_exposure, preview_maxline;
	uint32_t capture_maxline, preview_exposure;
	uint32_t capture_mode = param & 0xffff;
	uint32_t preview_mode = (param >> 0x10 ) & 0xffff;
	uint32_t prv_linetime=s_ov13870_Resolution_Trim_Tab[preview_mode].line_time;
	uint32_t cap_linetime = s_ov13870_Resolution_Trim_Tab[capture_mode].line_time;

	SENSOR_PRINT("SENSOR_ov13870: BeforeSnapshot mode: 0x%08lx",param);

	if (preview_mode == capture_mode) {
		SENSOR_PRINT("SENSOR_ov13870: prv mode equal to capmode");
		goto CFG_INFO;
	}


	CFG_INFO:
	s_capture_shutter = _ov13870_get_shutter(handle);
	Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_EXPOSURETIME, s_capture_shutter);
	s_exposure_time = s_capture_shutter * cap_linetime / 1000;

	return SENSOR_SUCCESS;

}

static uint32_t _ov13870_GetRawInof(SENSOR_HW_HANDLE handle)
{
	uint32_t rtn=SENSOR_SUCCESS;
	struct raw_param_info_tab* tab_ptr = (struct raw_param_info_tab*)s_ov13870_raw_param_tab;
	uint32_t param_id;
	uint32_t i=0x00;

	/*read param id from sensor omap*/
	param_id=ov13870_RAW_PARAM_COM;

	for (i=0x00; ; i++) {
		g_module_id = i;
		if (RAW_INFO_END_ID==tab_ptr[i].param_id) {
			if (NULL==s_ov13870_mipi_raw_info_ptr) {
				SENSOR_PRINT("SENSOR_ov13870: _ov13870_GetRawInof no param error");
				rtn=SENSOR_FAIL;
			}
			SENSOR_PRINT("SENSOR_ov13870: _ov13870_GetRawInof end");
			break;
		}
		else if (PNULL!=tab_ptr[i].identify_otp) {
			if (SENSOR_SUCCESS==tab_ptr[i].identify_otp(0)) {
				s_ov13870_mipi_raw_info_ptr = tab_ptr[i].info_ptr;
				SENSOR_PRINT("SENSOR_ov13870: _ov13870_GetRawInof success");
				break;
			}
		}
	}

	return rtn;
}

static unsigned long _ov13870_GetExifInfo(SENSOR_HW_HANDLE handle, unsigned long param)
{
	LOCAL EXIF_SPEC_PIC_TAKING_COND_T sexif;

	sexif.ExposureTime.numerator = s_exposure_time;
	sexif.ExposureTime.denominator = 1000000;

	return (unsigned long) & sexif;
}


static unsigned long _ov13870_StreamOn(SENSOR_HW_HANDLE handle, unsigned long param)
{
	SENSOR_PRINT("SENSOR_ov13870: StreamOn");

	Sensor_WriteReg(0x0100, 0x01);

	return 0;
}

static unsigned long _ov13870_StreamOff(SENSOR_HW_HANDLE handle, unsigned long param)
{
	SENSOR_PRINT("SENSOR_ov13870: StreamOff");

	Sensor_WriteReg(0x0100, 0x00);

	usleep(150*1000);

	return 0;
}

static uint32_t _ov13870_com_Identify_otp(SENSOR_HW_HANDLE handle, void* param_ptr)
{
	uint32_t rtn=SENSOR_FAIL;
	uint32_t param_id;

	SENSOR_PRINT("SENSOR_ov13870: _ov13870_com_Identify_otp");

	/*read param id from sensor omap*/
	param_id=ov13870_RAW_PARAM_COM;

	if(ov13870_RAW_PARAM_COM==param_id){
		rtn=SENSOR_SUCCESS;
	}

	return rtn;
}

static unsigned long _ov13870_PowerOn(SENSOR_HW_HANDLE handle, unsigned long power_on)
{
	SENSOR_AVDD_VAL_E dvdd_val = g_ov13870_mipi_raw_info.dvdd_val;
	SENSOR_AVDD_VAL_E avdd_val = g_ov13870_mipi_raw_info.avdd_val;
	SENSOR_AVDD_VAL_E iovdd_val = g_ov13870_mipi_raw_info.iovdd_val;
	BOOLEAN power_down = g_ov13870_mipi_raw_info.power_down_level;
	BOOLEAN reset_level = g_ov13870_mipi_raw_info.reset_pulse_level;

	uint8_t pid_value = 0x00;

	if (SENSOR_TRUE == power_on) {
		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		Sensor_SetMonitorVoltage(SENSOR_AVDD_CLOSED);
		Sensor_SetVoltage(SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED);
		Sensor_SetResetLevel(reset_level);
		Sensor_PowerDown(power_down);
		usleep(10*1000);

		// Open power
		Sensor_SetMonitorVoltage(SENSOR_AVDD_2800MV);
		usleep(1000);
		Sensor_SetIovddVoltage(iovdd_val);
		usleep(1000);
		Sensor_SetAvddVoltage(avdd_val);
		usleep(1000);
		Sensor_SetDvddVoltage(dvdd_val);

		usleep(10*1000);
		Sensor_PowerDown(!power_down);
		usleep(1000);
		Sensor_SetResetLevel(!reset_level);
		usleep(20*1000);
		Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);
		usleep(20*1000);

	} else {
		usleep(4*1000);
		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		usleep(1000);
		Sensor_SetResetLevel(reset_level);
		Sensor_PowerDown(power_down);
		usleep(1000);
		Sensor_SetDvddVoltage(SENSOR_AVDD_CLOSED);
		usleep(1000);
		Sensor_SetIovddVoltage(SENSOR_AVDD_CLOSED);
		Sensor_SetAvddVoltage(SENSOR_AVDD_CLOSED);

		Sensor_SetMonitorVoltage(SENSOR_AVDD_CLOSED);
	}

	SENSOR_PRINT_ERR("13870: _ov13870_PowerOn(1:on, 0:off): %ld, reset_level %d, power_down %d", power_on, reset_level, power_down);
	return SENSOR_SUCCESS;
}

static uint32_t _ov13870_write_otp_gain(SENSOR_HW_HANDLE handle, uint32_t *param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t value = 0x00;

	SENSOR_PRINT("SENSOR_ov13870: write_gain:0x%x\n", *param);

	//ret_value = Sensor_WriteReg(0x104, 0x01);
	value = (*param)>>0x08;
	ret_value = Sensor_WriteReg(0x204, value);
	value = (*param)&0xff;
	ret_value = Sensor_WriteReg(0x205, value);
	ret_value = Sensor_WriteReg(0x104, 0x00);

	return ret_value;
}

static uint32_t _ov13870_read_otp_gain(SENSOR_HW_HANDLE handle, uint32_t *param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	uint16_t gain_h = 0;
	uint16_t gain_l = 0;
	#if 1 // for MP tool //!??
	gain_h = Sensor_ReadReg(0x0204) & 0xff;
	gain_l = Sensor_ReadReg(0x0205) & 0xff;
	*param = ((gain_h << 8) | gain_l);
	#else
	*param = s_set_gain;
	#endif
	SENSOR_PRINT("SENSOR_ov13870: gain: %d", *param);

	return rtn;
}

static uint16_t _ov13870_get_shutter(SENSOR_HW_HANDLE handle)
{
	// read shutter, in number of line period
	uint16_t shutter_h = 0;
	uint16_t shutter_l = 0;
#if 1  // MP tool //!??
	shutter_h = Sensor_ReadReg(0x0202) & 0xff;
	shutter_l = Sensor_ReadReg(0x0203) & 0xff;

	return (shutter_h << 8) | shutter_l;
#else
	return s_set_exposure;
#endif
}

static uint32_t _ov13870_get_static_info(SENSOR_HW_HANDLE handle, uint32_t *param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	struct sensor_ex_info *ex_info;
	uint32_t up = 0;
	uint32_t down = 0;
	//make sure we have get max fps of all settings.
	if(!s_ov13870_mode_fps_info.is_init) {
		_ov13870_init_mode_fps_info(handle);
	}
	ex_info = (struct sensor_ex_info*)param;
	ex_info->f_num = s_ov13870_static_info.f_num;
	ex_info->focal_length = s_ov13870_static_info.focal_length;
	ex_info->max_fps = s_ov13870_static_info.max_fps;
	ex_info->max_adgain = s_ov13870_static_info.max_adgain;
	ex_info->ois_supported = s_ov13870_static_info.ois_supported;
	ex_info->pdaf_supported = s_ov13870_static_info.pdaf_supported;
	ex_info->exp_valid_frame_num = s_ov13870_static_info.exp_valid_frame_num;
	ex_info->clamp_level = s_ov13870_static_info.clamp_level;
	ex_info->adgain_valid_frame_num = s_ov13870_static_info.adgain_valid_frame_num;
	ex_info->preview_skip_num = g_ov13870_mipi_raw_info.preview_skip_num;
	ex_info->capture_skip_num = g_ov13870_mipi_raw_info.capture_skip_num;
	ex_info->name = g_ov13870_mipi_raw_info.name;
	ex_info->sensor_version_info = g_ov13870_mipi_raw_info.sensor_version_info;
	bu64297gwz_get_pose_dis(handle, &up, &down);
	ex_info->pos_dis.up2hori = up;
	ex_info->pos_dis.hori2down = down;
	SENSOR_PRINT("SENSOR_ov13870: f_num: %d", ex_info->f_num);
	SENSOR_PRINT("SENSOR_ov13870: max_fps: %d", ex_info->max_fps);
	SENSOR_PRINT("SENSOR_ov13870: max_adgain: %d", ex_info->max_adgain);
	SENSOR_PRINT("SENSOR_ov13870: ois_supported: %d", ex_info->ois_supported);
	SENSOR_PRINT("SENSOR_ov13870: pdaf_supported: %d", ex_info->pdaf_supported);
	SENSOR_PRINT("SENSOR_ov13870: exp_valid_frame_num: %d", ex_info->exp_valid_frame_num);
	SENSOR_PRINT("SENSOR_ov13870: clam_level: %d", ex_info->clamp_level);
	SENSOR_PRINT("SENSOR_ov13870: adgain_valid_frame_num: %d", ex_info->adgain_valid_frame_num);
	SENSOR_PRINT("SENSOR_ov13870: sensor name is: %s", ex_info->name);
	SENSOR_PRINT("SENSOR_ov13870: sensor version info is: %s", ex_info->sensor_version_info);
	SENSOR_PRINT("SENSOR_ov13870: up2h %d h2down %d", ex_info->pos_dis.up2hori, ex_info->pos_dis.hori2down);

	return rtn;
}


static uint32_t _ov13870_get_fps_info(SENSOR_HW_HANDLE handle, uint32_t *param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_MODE_FPS_T *fps_info;
	//make sure have inited fps of every sensor mode.
	if(!s_ov13870_mode_fps_info.is_init) {
		_ov13870_init_mode_fps_info(handle);
	}
	fps_info = (SENSOR_MODE_FPS_T*)param;
	uint32_t sensor_mode = fps_info->mode;
	fps_info->max_fps = s_ov13870_mode_fps_info.sensor_mode_fps[sensor_mode].max_fps;
	fps_info->min_fps = s_ov13870_mode_fps_info.sensor_mode_fps[sensor_mode].min_fps;
	fps_info->is_high_fps = s_ov13870_mode_fps_info.sensor_mode_fps[sensor_mode].is_high_fps;
	fps_info->high_fps_skip_num = s_ov13870_mode_fps_info.sensor_mode_fps[sensor_mode].high_fps_skip_num;
	SENSOR_PRINT("SENSOR_ov13870: mode %d, max_fps: %d",fps_info->mode, fps_info->max_fps);
	SENSOR_PRINT("SENSOR_ov13870: min_fps: %d", fps_info->min_fps);
	SENSOR_PRINT("SENSOR_ov13870: is_high_fps: %d", fps_info->is_high_fps);
	SENSOR_PRINT("SENSOR_ov13870: high_fps_skip_num: %d", fps_info->high_fps_skip_num);

	return rtn;
}

static unsigned long _ov13870_access_val(SENSOR_HW_HANDLE handle, unsigned long param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_VAL_T* param_ptr = (SENSOR_VAL_T*)param;
	uint16_t tmp;

	SENSOR_PRINT("SENSOR_ov13870: _ov13870_access_val E param_ptr = %p", param_ptr);
	if(!param_ptr){
		return rtn;
	}

	SENSOR_PRINT("SENSOR_ov13870: param_ptr->type=%x", param_ptr->type);
	switch(param_ptr->type)
	{
		case SENSOR_VAL_TYPE_INIT_OTP:
			//ov13870_otp_init(handle);
			break;
		case SENSOR_VAL_TYPE_SHUTTER:
			//*((uint32_t*)param_ptr->pval) = _ov13870_get_shutter(handle);
			break;
		case SENSOR_VAL_TYPE_READ_VCM:
			//rtn = _ov13870_read_vcm(param_ptr->pval);
			break;
		case SENSOR_VAL_TYPE_WRITE_VCM:
			//rtn = _ov13870_write_vcm(param_ptr->pval);
			break;
		case SENSOR_VAL_TYPE_READ_OTP:
			//ov13870_otp_read(handle, param_ptr);
			break;
		case SENSOR_VAL_TYPE_PARSE_OTP:
			//ov13870_parse_otp(handle, param_ptr);
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
			*(uint32_t*)param_ptr->pval = 0;//cur_af_pos;
			break;
		case SENSOR_VAL_TYPE_WRITE_OTP_GAIN:
			//rtn = _ov13870_write_otp_gain(handle, param_ptr->pval);
			break;
		case SENSOR_VAL_TYPE_READ_OTP_GAIN:
			//rtn = _ov13870_read_otp_gain(handle, param_ptr->pval);
			break;
		case SENSOR_VAL_TYPE_GET_STATIC_INFO:
			rtn = _ov13870_get_static_info(handle, param_ptr->pval);
			break;
		case SENSOR_VAL_TYPE_GET_FPS_INFO:
			rtn = _ov13870_get_fps_info(handle, param_ptr->pval);
			break;
		default:
			break;
	}

	SENSOR_PRINT("SENSOR_ov13870: _ov13870_access_val X");

	return rtn;
}

