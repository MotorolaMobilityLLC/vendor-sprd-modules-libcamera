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
#ifndef _AWB_ALG_PARAM_H_
#define _AWB_ALG_PARAM_H_

/*------------------------------------------------------------------------------*
*				Dependencies					*
*-------------------------------------------------------------------------------*/
#ifndef WIN32
#include <sys/types.h>
#else
#include "sci_types.h"
#endif

#define AWB_ALG_PACKET_EN

#ifndef AWB_ALG_PACKET_EN
#include "sensor_raw.h"
#endif
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
#define AWB_ALG_ENVI_NUM	8
#define AWB_ALG_SCENE_NUM	4
#define AWB_ALG_PIECEWISE_SAMPLE_NUM	16
#define AWB_ALG_CT_INFO_NUM	8
#define AWB_ALG_WIN_COORD_NUM	20
#define AWB_CTRL_ENVI_NUM 8
#define AWB_CTRL_ENVI_NUM 8
/*------------------------------------------------------------------------------*
*				Data Structures					*
*-------------------------------------------------------------------------------*/
struct awb_alg_gain{
	cmr_u16 r;
	cmr_u16 g;
	cmr_u16 b;
	cmr_u16 padding;
};

struct awb_alg_range {
	cmr_s16 min;
	cmr_s16 max;
};

struct awb_alg_bv {
	cmr_u32 enable;
	cmr_u32 num;
	struct awb_alg_range bv_range[AWB_CTRL_ENVI_NUM];
};
struct awb_alg_ref_param {
	struct awb_alg_gain gain;
	cmr_u32 ct;
	cmr_u32 enable;
};

struct awb_alg_coord {
	cmr_u16 x;
	cmr_u16 yb;
	cmr_u16 yt;
	cmr_u16 padding;
};

struct awb_alg_sample {
	cmr_s16		x;
	cmr_s16		y;
};

struct awb_alg_piecewise_func {
	cmr_u32 num;
	struct awb_alg_sample samples[AWB_ALG_PIECEWISE_SAMPLE_NUM];
};

struct awb_alg_weight_of_ct_func {
	struct awb_alg_piecewise_func weight_func;
	cmr_u32 base;
};

struct awb_alg_ct_info {
	cmr_s32 data[AWB_ALG_CT_INFO_NUM];
};

struct awb_alg_value_range {
	cmr_s16 min;
	cmr_s16 max;
};

struct awb_alg_param_common {

	/*common parameters*/
	struct awb_alg_gain init_gain;
	cmr_u32 init_ct;
	cmr_u32 quick_mode;
	cmr_u32 alg_id;

	/*parameters for alg 0*/
	/*white window*/
	struct awb_alg_coord win[AWB_ALG_WIN_COORD_NUM];
	cmr_u8 target_zone;

	/*parameters for alg 1*/
	struct awb_alg_ct_info ct_info;
	struct awb_alg_weight_of_ct_func weight_of_ct_func[AWB_ALG_ENVI_NUM];
	struct awb_alg_weight_of_ct_func weight_of_count_func;
	struct awb_alg_value_range value_range[AWB_ALG_ENVI_NUM];
	struct awb_alg_ref_param ref_param[AWB_ALG_ENVI_NUM];
	cmr_u32 scene_factor[AWB_ALG_SCENE_NUM];
	cmr_u32 steady_speed;
	cmr_u32 debug_level;
};

struct awb_alg_pos_weight{
	cmr_u8 *data;
	cmr_u16 width;
	cmr_u16 height;
};

struct awb_alg_map {
	cmr_u16 *data;
	cmr_u32 size;
};

struct awb_alg_tuning_param {
	struct awb_alg_param_common common;
	struct awb_alg_pos_weight pos_weight;
	struct awb_alg_map win_map;
};

#ifndef AWB_ALG_PACKET_EN
struct awb_param {
	struct sensor_awb_param sen_awb_param;
	struct sensor_awb_map awb_map;
	struct sensor_awb_weight awb_weight;
};
#endif

struct awb_alg_weight {
	cmr_u16 *data;
	cmr_u32 size;
};
/*------------------------------------------------------------------------------*
*				Functions					*
*-------------------------------------------------------------------------------*/

#ifndef AWB_ALG_PACKET_EN
cmr_s32 awb_alg_param_packet(struct awb_param *awb_param,  cmr_u32 pack_buf_size, void *pack_buf);
#endif
cmr_s32 awb_alg_param_unpacket(void *pack_data, cmr_u32 data_size, struct awb_alg_tuning_param *alg_param);

/*------------------------------------------------------------------------------*
*				Compiler Flag					*
*-------------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/*------------------------------------------------------------------------------*/
#endif
// End

