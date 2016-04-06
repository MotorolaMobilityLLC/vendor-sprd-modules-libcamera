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

#ifndef _AFT_INTERFACE_H_
#define _AFT_INTERFACE_H_
/*------------------------------------------------------------------------------*
*					Dependencies				*
*-------------------------------------------------------------------------------*/
#include <sys/types.h>

#include "cmr_types.h"


/*------------------------------------------------------------------------------*
*					Compiler Flag				*
*-------------------------------------------------------------------------------*/
#ifdef  __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------*
*					Data Structures				*
*-------------------------------------------------------------------------------*/
#define AFT_INVALID_HANDLE NULL
#define MAX_AF_FILTER_CNT 10
#define MAX_AF_WIN 32

typedef void* aft_proc_handle_t;

enum aft_posture_type {
	AFT_POSTURE_ACCELEROMETER,
	AFT_POSTURE_MAGNETIC,
	AFT_POSTURE_ORIENTATION,
	AFT_POSTURE_GYRO,
	AFT_POSTURE_MAX
};

enum aft_err_type {
	AFT_SUCCESS = 0x00,
	AFT_ERROR,
	AFT_HANDLER_NULL,
	AFT_ERR_MAX
};

enum aft_mode{
	AFT_MODE_NORMAL = 0x00,
	AFT_MODE_MACRO,
	AFT_MODE_CONTINUE,
	AFT_MODE_VIDEO,
	AFT_MODE_MAX
};

enum aft_calc_data_type {
	AFT_DATA_AF,
	AFT_DATA_IMG_BLK,
	AFT_DATA_AE,
	AFT_DATA_SENSOR,
	AFT_DATA_MAX

};

enum aft_cmd {
	AFT_CMD_SET_BASE 			= 0x1000,
	AFT_CMD_SET_AF_MODE			= 0x1001,
	AFT_CMD_SET_CAF_RESET		= 0x1002,
	AFT_CMD_SET_CAF_STOP			= 0x1003,
};

struct aft_tuning_block_param {
	cmr_u8 *data;
	cmr_u32 data_len;
};


struct aft_af_win_rect {
	cmr_u32 sx;
	cmr_u32 sy;
	cmr_u32 ex;
	cmr_u32 ey;
};

struct aft_af_filter_data {
	cmr_u32 type;
	cmr_u64 *data;
};

struct aft_af_filter_info {
	cmr_u32 filter_num;
	struct aft_af_filter_data filter_data[MAX_AF_FILTER_CNT];
};

struct aft_af_win_cfg {
	cmr_u32 win_cnt;
	struct aft_af_win_rect win_pos[MAX_AF_WIN];
	cmr_u32 win_prio[MAX_AF_WIN];
	cmr_u32 win_sel_mode;
};

struct aft_afm_info {
	struct aft_af_win_cfg win_cfg;
	struct aft_af_filter_info filter_info;
};

struct aft_img_blk_info {
	cmr_u32 block_w;
	cmr_u32 block_h;
	cmr_u32 pix_per_blk;
	cmr_u32 chn_num;
	cmr_u32 *data;
//	cmr_u32 hist_array_y[1024];
};


struct aft_ae_info {
	cmr_u32 exp_time;  //us
	cmr_u32 gain;   //256 --> 1X
	cmr_u32 cur_lum;
	cmr_u32 target_lum;
	cmr_u32 is_stable;
};

struct aft_sensor_info {
	cmr_u32 sensor_type;
//	cmr_s64 timestamp;
	float x;
	float y;
	float z;
};

struct aft_proc_result {
	cmr_u32 is_caf_trig;
	cmr_u32 is_caf_trig_in_taf;
	cmr_u32 is_need_rough_search;
};

struct aft_proc_calc_param {
	cmr_u32 active_data_type;
	cmr_u32 af_has_suc_rec;
	struct aft_afm_info afm_info;
	struct aft_img_blk_info img_blk_info;
	struct aft_ae_info ae_info;
	struct aft_sensor_info sensor_info;
};


/*------------------------------------------------------------------------------*
*					Data Prototype				*
*-------------------------------------------------------------------------------*/

signed int caf_trigger_init(struct aft_tuning_block_param *init_param, aft_proc_handle_t *handle);
signed int caf_trigger_deinit(aft_proc_handle_t handle);
signed int caf_trigger_calculation(aft_proc_handle_t handle,
				struct aft_proc_calc_param *aft_calc_in,
				struct aft_proc_result *aft_calc_result);
signed int caf_trigger_ioctrl(aft_proc_handle_t handle, enum aft_cmd cmd, void *param0, void *param1);

/*------------------------------------------------------------------------------*
*					Compiler Flag				*
*-------------------------------------------------------------------------------*/
#ifdef	 __cplusplus
}
#endif
/*-----------------------------------------------------------------------------*/
#endif
// End
