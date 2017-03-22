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
 #ifndef _RANDOM_PACK_H_
#define _RANDOM_PACK_H_

/*------------------------------------------------------------------------------*
*				Dependencies					*
*-------------------------------------------------------------------------------*/
//#include "sci_types.h"
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
struct random_lsc_info {
	/* 1: tshark algorithm; 2: sharkl/tshark2 algorithm  */
	cmr_u32 alg_version;
	/* 0: normal gain (16 bits for one gain); 1: compress gain (14 bit for one gain)  */
	cmr_u32 compress;
	cmr_u32 base_gain;
	/* correction percent: 1-100 */
	cmr_u32 percent;
	cmr_u32 grid_width;
	cmr_u32 grid_height;
	/*four channel gain placed one by one*/
	cmr_u16 *chn_gain[4];
	cmr_u16 chn_gain_size;
	/*0: gr, 1: r, 2: b, 3: gb*/
	cmr_u32 bayer_pattern;
	cmr_u32 img_width;
	cmr_u32 img_height;
	cmr_u32 gain_width;
	cmr_u32 gain_height;
	cmr_u32 center_x;
	cmr_u32 center_y;
};

struct random_awb_info
{
	/*average value of std image*/
	cmr_u16 avg_r;
	cmr_u16 avg_g;
	cmr_u16 avg_b;
};

//calibration in para
struct random_pack_param {
	struct random_lsc_info lsc_info;
	struct random_awb_info awb_info;
	void *target_buf;
	cmr_u32 target_buf_size;
};

struct random_pack_result {
	cmr_u32 real_size;
};

/*------------------------------------------------------------------------------*
*				Functions														*
*-------------------------------------------------------------------------------*/
cmr_s32 random_pack(struct random_pack_param *param, struct random_pack_result *result);
cmr_s32 get_random_pack_size(cmr_u32 chn_gain_size, cmr_u32 *size);
/*------------------------------------------------------------------------------*
*				Compiler Flag					*
*-------------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/*------------------------------------------------------------------------------*/

#endif
// End

