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
#ifndef _AE_UTILS_H_
#define _AE_UTILS_H_
/*----------------------------------------------------------------------------*
 **				 Dependencies				*
 **---------------------------------------------------------------------------*/
#include "ae_types.h"
#include "ae_ctrl_types.h"
/**---------------------------------------------------------------------------*
 **				 Compiler Flag				*
 **---------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {

#endif				/*  */
/**---------------------------------------------------------------------------*
**				Macro Define				*
**----------------------------------------------------------------------------*/
#define AE_FRAME_INFO_NUM 	8
/**---------------------------------------------------------------------------*
**				Data Structures				*
**---------------------------------------------------------------------------*/
struct ae_ctrl_time {
	cmr_u32 sec;
	cmr_u32 usec;
};

struct ae_ctrl_stats_info {
	struct ae_stat_img_info info;
	struct ae_ctrl_time eof_time;
	struct ae_ctrl_time write_sensor_time;
	struct ae_ctrl_time delay_time;
	struct ae_ctrl_time frame_time;
	cmr_u32 skip_frame;
	cmr_u32 delay_frame;
	cmr_u32 near_stab;
	cmr_u32 frame_id;
};

struct ae_history_info {
	struct ae_ctrl_stats_info stats_info[AE_FRAME_INFO_NUM];	//save the latest 8 exposure time and gain;
	//0: latest one;
	//list will be better
	cmr_u32 cur_stat_index;
	struct ae_ctrl_stats_info last_info;
	struct ae_ctrl_stats_info cur_info;
	struct ae_ctrl_stats_info effect_info;
	cmr_u32 sensor_effect_delay_num;
};

/**---------------------------------------------------------------------------*
** 				Function Defination			*
**---------------------------------------------------------------------------*/
cmr_s32 ae_utils_calc_func(struct ae_piecewise_func *func, cmr_u32 y_type, cmr_s32 x, struct ae_weight_value *result);
cmr_s32 ae_utils_get_effect_index(struct ae_history_info *history, cmr_u32 frame_id, cmr_s32 * effect_index, cmr_s32 * uneffect_index, cmr_u32 * is_only_calc_lum_flag);
cmr_s32 ae_utils_save_stat_info(struct ae_history_info *history, struct ae_ctrl_stats_info *cur_info);
cmr_s32 ae_utils_get_stat_info(struct ae_history_info *history, struct ae_ctrl_stats_info *stat_info, cmr_u32 cur_stat_index, cmr_u32 frame_id);
cmr_s32 ae_utils_get_delay_frame_num(struct ae_history_info *history, struct ae_ctrl_time *eof, struct ae_ctrl_time *write_sensor, cmr_u32 frame_time);

/**----------------------------------------------------------------------------*
**					Compiler Flag			*
**----------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
#endif
