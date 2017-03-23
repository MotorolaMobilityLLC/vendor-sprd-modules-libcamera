/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef _REGION_H_
#define _REGION_H_
/*----------------------------------------------------------------------------*
 **				 Dependencies				*
 **---------------------------------------------------------------------------*/
#include "ae_types.h"
/**---------------------------------------------------------------------------*
 **				 Compiler Flag				*
 **---------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {

#endif/*  */
/**---------------------------------------------------------------------------*
**				Macro Define				*
**----------------------------------------------------------------------------*/
#define REGION_CFG_NUM AE_CFG_NUM
/**---------------------------------------------------------------------------*
**				Data Structures				*
**---------------------------------------------------------------------------*/
	typedef struct {
		struct ae_range region_thrd[6];/*u d l r */
		cmr_s16 up_max;
		cmr_s16 dwn_max;
		cmr_s16 vote_region[6];/*u d l r */
	} region_cfg;/*16 * 4bytes */

	struct region_tuning_param {
		cmr_u8 enable;
		cmr_u8 num;
		cmr_u16 reserved;	/*1 * 4bytes */
		region_cfg cfg_info[REGION_CFG_NUM];	/*total 8 group: 128 * 4bytes */
		struct ae_piecewise_func input_piecewise;	/*17 * 4bytes */
		struct ae_piecewise_func u_out_piecewise;	/*17 * 4bytes */
		struct ae_piecewise_func d_out_piecewise;	/*17 * 4bytes */
	};/*180 * 4bytes */

	typedef struct {
		cmr_u8 mlog_en;
		cmr_u8 * ydata;
		cmr_u8 * pos_weight;
		cmr_s16 stat_size;
		cmr_s16 match_lv;
		float real_lum;
		float comp_target;
	} region_in;//tuning info

	typedef struct  {
		cmr_s16 tar_offset_u;
		cmr_s16 tar_offset_d;
		cmr_s16 input_interpolation[4];
		float u_strength;
		float d_strength;
		float degree;
		char *log;
	} region_rt;	//result info

	typedef struct  {
		cmr_u8 enable;
		cmr_u8 debug_level;
		cmr_u8 mlog_en;
		struct region_tuning_param tune_param;
		region_in in_region;
		region_rt result_region;
		cmr_s8 region_num;
		float region_lum[10];
		/*algorithm runtime status*/
		cmr_s16 lv_record;
		cmr_s16 region_thd[12];	//u d l r
		cmr_s16 up_max;
		cmr_s16 down_max;
		float vote_region[6];	//u d l r
		float over_lum;
		cmr_u32 log_buf[256];
	} region_stat;

/**---------------------------------------------------------------------------*
** 				Function Defination			*
**---------------------------------------------------------------------------*/
	cmr_s32 region_init(region_stat * cxt, struct region_tuning_param *tune_ptr);
	cmr_s32 region_calc(region_stat * cxt);
	cmr_s32 region_deinit(region_stat * cxt);

/**----------------------------------------------------------------------------*
**					Compiler Flag												     *
**----------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif/*  */
#endif/*  */
