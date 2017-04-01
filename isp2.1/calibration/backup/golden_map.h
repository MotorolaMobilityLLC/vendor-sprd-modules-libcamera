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
#ifndef _GOLDEN_MAP_H_
#define _GOLDEN_MAP_H_

/*------------------------------------------------------------------------------*
*				Dependencies					*
*-------------------------------------------------------------------------------*/
#include "isp_otp_type.h"
/*------------------------------------------------------------------------------*
*				Compiler Flag					*
*-------------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif
/*------------------------------------------------------------------------------*
				Micro Define					*
*-------------------------------------------------------------------------------*/
#define GOLDEN_MODULE_BLOCK_ID		0x0001
#define GOLDEN_LSC_BLOCK_ID			0x0002
#define GOLDEN_AWB_BLOCK_ID			0x0003
/*block id for golden lsc sub block*/
#define GOLDEN_BLOCK_ID_LSC_BASIC			0x00020001
#define GOLDEN_BLOCK_ID_LSC_STD_GAIN		0x00020002
#define GOLDEN_BLOCK_ID_LSC_DIFF_GAIN		0x00020003

#define GOLDEN_VERSION_FLAG 		0x5350
#define GOLDEN_VERSION				((GOLDEN_VERSION_FLAG << 16) | 1)
#define GOLDEN_LSC_BLOCK_VERSION	0x53501001
#define GOLDEN_AWB_BLOCK_VERSION 0x53502001
#define GOLDEN_START				0x53505244
#define GOLDEN_END					0x53505244
/*------------------------------------------------------------------------------*
*				Data Structures					*
*-------------------------------------------------------------------------------*/
struct golden_header {
	cmr_u32 start;
	cmr_u32 length;
	cmr_u32 version;
	cmr_u32 block_num;
};

struct golden_block_info {
	cmr_u32 id;
	cmr_u32 offset;
	cmr_u32 size;
};

struct golden_map_module {
	cmr_u32 core_version;
	cmr_u32 sensor_maker;
	cmr_u32 year;
	cmr_u32 month;
	cmr_u32 module_version;
	cmr_u32 release_number;
	cmr_u32 cal_dll_version;
	cmr_u32 cal_map_version;
	cmr_u32 reserved[8];
};

/*information of LSC module*/
struct golden_lsc_header {
	cmr_u32 version;
	cmr_u32 length;
	cmr_u32 block_num;
};

struct golden_map_lsc_basic {
	cmr_u16 base_gain;
	cmr_u16 algorithm_version;
	cmr_u16 compress_flag;
	cmr_u16 image_width;
	cmr_u16 image_height;
	cmr_u16 gain_width;
	cmr_u16 gain_height;
	cmr_u16 optical_x;
	cmr_u16 optical_y;
	cmr_u16 grid_width;
	cmr_u16 grid_height;
	cmr_u16 percent;
	cmr_u16 bayer_pattern;
	cmr_u16 reserved;
};

struct golden_map_awb {
	cmr_u32 version;
	cmr_u16 avg_r;
	cmr_u16 avg_g;
	cmr_u16 avg_b;
	cmr_u16 reserved0;
	cmr_u16 reserved1[6];
};
/*------------------------------------------------------------------------------*
*				Compiler Flag					*
*-------------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/*------------------------------------------------------------------------------*/
#endif
// End
