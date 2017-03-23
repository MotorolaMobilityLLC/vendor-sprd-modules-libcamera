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
#ifndef _TOUCH_AE_H_
#define _TOUCH_AE_H_
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

/**---------------------------------------------------------------------------*
**				Data Structures				*
**---------------------------------------------------------------------------*/
	struct ae_touch_param {
		cmr_u8 win2_weight;//for touch ae
		cmr_u8 enable;//for touch ae
		cmr_u8 win1_weight;//for touch ae
		cmr_u8 reserved;//for touch ae
		struct ae_size touch_tuning_win;//for touch ae
	};

	typedef struct  {
		cmr_u8 tuning_enable;
		cmr_s8 tc_scrn_enable;
		cmr_u8 mlog_en;
		struct ae_trim touch_scrn_coord;
		struct ae_size touch_tuning_window;
		struct ae_size small_win_numVH;
		struct ae_size small_win_wh;
		struct ae_size image_wh;
		cmr_u8 * touch_roi_weight;
		cmr_u8 * ydata;
		cmr_u32 stat_size;
		cmr_u8 base_win_weight;
		cmr_u8 tcROI_win_weight;
		float real_lum;
		float real_target;
	} touch_in;//tuning info

	typedef struct {
		cmr_s16 tar_offset;
		float ratio;
		float touch_roi_lum;
		cmr_s8 tcAE_state;//for release
		cmr_s8 release_flag;//for release
		char* log;
	} touch_rt;//result info
	
	typedef struct  {
		cmr_u8 mlog_en;
		cmr_u8 debug_status;
		touch_in in_touch_ae;
		touch_rt result_touch_ae;
		cmr_s16 cur_bv;
		cmr_s16 pre_bv;
		float deltaLUM;
		cmr_u32 touchae_status;
		touch_in cur_status;
		touch_in prv_status;
		cmr_u32 touchae_weight[1024];
		cmr_u32 log_buf[256];
	} touch_stat;

/**---------------------------------------------------------------------------*
** 				Function Defination			*
**---------------------------------------------------------------------------*/
	cmr_s32 touch_ae_init(touch_stat * cxt, touch_in * in_touch_ae);
	cmr_s32 touch_ae_calc(touch_stat * cxt);
	cmr_s32 release_touch_ae(touch_stat * cxt, cmr_s8 ae_state, cmr_s16 bv);
	cmr_s32 touch_ae_deinit(touch_stat * cxt);

/**----------------------------------------------------------------------------*
**					Compiler Flag												     *
**----------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif/*  */

#endif/*  */