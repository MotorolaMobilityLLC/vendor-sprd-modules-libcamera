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
#ifndef _RANDOM_MAP_H_
#define _RANDOM_MAP_H_

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
#define RANDOM_LSC_BLOCK_ID 0x0002
#define RANDOM_AWB_BLOCK_ID 0x0003

#define RANDOM_VERSION 1
#define RANDOM_LSC_BLOCK_VERSION 0x53501001
#define RANDOM_AWB_BLOCK_VERSION 0x53502001
#define RANDOM_START 0x53505244
#define RANDOM_END 0x53505244

///////////////////////////////////////////////////////////////////////////////////
struct random_map_lsc {
	cmr_u32 version;
	cmr_u16 data_length;
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
	cmr_u16 gain_num;
};

struct random_map_awb {
	cmr_u32 version;
	cmr_u16 avg_r;
	cmr_u16 avg_g;
	cmr_u16 avg_b;
	cmr_u16 reserved;
};

struct random_block_info {
	cmr_u32 id;
	cmr_u32 offset;
	cmr_u32 size;
};

struct random_header {
	cmr_u32 verify;
	cmr_u32 size;
	cmr_u32 version;
	cmr_u32 block_num;
};
/*------------------------------------------------------------------------------*
*				Data Structures					*
*-------------------------------------------------------------------------------*

/*------------------------------------------------------------------------------*
*				Compiler Flag					*
*-------------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/*------------------------------------------------------------------------------*/
#endif
// End
