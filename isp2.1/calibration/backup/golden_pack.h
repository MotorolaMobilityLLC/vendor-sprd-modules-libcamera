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
 #ifndef _GOLDEN_PACK_H_
#define _GOLDEN_PACK_H_

/*------------------------------------------------------------------------------*
*				Dependencies					*
*-------------------------------------------------------------------------------*/
#include "basic_type.h"
#include "lsc_alg.h"
/*------------------------------------------------------------------------------*
*				Compiler Flag					*
*-------------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"
{
#endif
/*------------------------------------------------------------------------------*
				Micro Define					*
*-------------------------------------------------------------------------------*/
///////////////////////////////////////////////////////////////////////////////////

/*------------------------------------------------------------------------------*
*				Data Structures					*
*-------------------------------------------------------------------------------*/
struct golden_module_info {
	cmr_u32 core_version;
	cmr_u32 sensor_maker;
	cmr_u32 year;
	cmr_u32 month;
	cmr_u32 module_version;
	cmr_u32 release_number;
	cmr_u32 cal_dll_version;
	cmr_u32 cal_map_version;
};

struct golden_lsc_info {
	/* 1: tshark algorithm; 2: sharkl/tshark2 algorithm  */
	cmr_u32 alg_version;
	/*1: use 1d diff; 2: use 2d diff*/
	cmr_u32 alg_type;
	/* 0: normal gain (16 bits for one gain); 1: compress gain (14 bit for one gain)  */
	cmr_u32 compress;
	cmr_u32 base_gain;
	/* correction percent: 1-100 */
	cmr_u32 percent;
	cmr_u32 grid_width;
	cmr_u32 grid_height;
	/*0: gr, 1: r, 2: b, 3: gb*/
	cmr_u32 bayer_pattern;
	cmr_u32 img_width;
	cmr_u32 img_height;
	cmr_u32 gain_width;
	cmr_u32 gain_height;
	cmr_u32 center_x;
	cmr_u32 center_y;
	/*std gain*/
	struct lsc_gain_info std_gain;
	cmr_u32 std_ct;
	/*nonstd diff*/
	struct lsc_diff_1d_info nonstd_diff[MAX_NONSTD_IMAGE];
	cmr_u32 nonstd_ct[MAX_NONSTD_IMAGE];
	cmr_u32 nonstd_num;
};

struct golden_awb_info {
	cmr_u16 avg_r;
	cmr_u16 avg_g;
	cmr_u16 avg_b;
};

//calibration in para
struct golden_pack_param {
	struct golden_module_info module_info;
	struct golden_lsc_info lsc_info;
	struct golden_awb_info awb_info;
	void *target_buf;
	cmr_u32 target_buf_size;
};

struct golden_pack_result {
	cmr_u32 real_size;
};
/*------------------------------------------------------------------------------*
*				Functions														*
*-------------------------------------------------------------------------------*/
cmr_s32 golden_pack(struct golden_pack_param *param, struct golden_pack_result *result);
cmr_s32 get_golden_pack_size(struct golden_pack_param *param, cmr_u32 *size);
/*------------------------------------------------------------------------------*
*				Compiler Flag					*
*-------------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/*------------------------------------------------------------------------------*/

#endif
// End

