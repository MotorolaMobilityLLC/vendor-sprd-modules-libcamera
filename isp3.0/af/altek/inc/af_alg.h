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

#ifndef _AF_ALG_H_
#define _AF_ALG_H_
/*------------------------------------------------------------------------------*
*					Dependencies				*
*-------------------------------------------------------------------------------*/
#include <sys/types.h>

#ifdef WIN32
#include "sci_type.h"
#endif


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
#define AF_INVALID_HANDLE NULL
#define MAX_AF_FILTER_CNT 10
#define MAX_AF_WIN 32

typedef void* caf_alg_handle_t;


enum af_alg_err_type {
	AF_ALG_SUCCESS = 0x00,
	AF_ALG_ERROR,
	AF_ALG_HANDLER_NULL,
	AF_ALG_ERR_MAX
};

enum af_alg_mode{
	AF_ALG_MODE_NORMAL = 0x00,
	AF_ALG_MODE_MACRO,
	AF_ALG_MODE_CONTINUE,
	AF_ALG_MODE_VIDEO,
	AF_ALG_MODE_MAX
};

enum af_alg_calc_data_type {
	AF_ALG_DATA_AF,
	AF_ALG_DATA_IMG_BLK,
	AF_ALG_DATA_AE,
	AF_ALG_DATA_GYRO,
	AF_ALG_DATA_GSENSOR,
	AF_ALG_DATA_MAX

};

enum af_alg_cmd {
	AF_ALG_CMD_SET_BASE 			= 0x1000,
	AF_ALG_CMD_SET_AF_MODE			= 0x1001,
	AF_ALG_CMD_SET_CAF_RESET		= 0x1002,
	AF_ALG_CMD_SET_CAF_STOP			= 0x1003,
};

struct af_alg_tuning_block_param {
	cmr_u8 *data;
	cmr_u32 data_len;
};


struct af_alg_win_rect {
	cmr_u32 sx;
	cmr_u32 sy;
	cmr_u32 ex;
	cmr_u32 ey;
};

struct af_alg_filter_data {
	uint32_t type;
	uint64_t *data;
};

struct af_alg_filter_info {
	cmr_u32 filter_num;
	struct af_alg_filter_data filter_data[MAX_AF_FILTER_CNT];
};

struct af_alg_af_win_cfg {
	cmr_u32 win_cnt;
	struct af_alg_win_rect win_pos[MAX_AF_WIN];
	cmr_u32 win_prio[MAX_AF_WIN];
	cmr_u32 win_sel_mode;
};

struct af_alg_afm_info {
	struct af_alg_af_win_cfg win_cfg;
	struct af_alg_filter_info filter_info;
};

struct af_alg_img_blk_info {
	cmr_u32 block_w;
	cmr_u32 block_h;
	cmr_u32 pix_per_blk;
	cmr_u32 chn_num;
	cmr_u32 *data;
};


struct af_alg_ae_info {
	cmr_u32 exp_time;  //us
	cmr_u32 gain;   //256 --> 1X
	cmr_u32 cur_lum;
	cmr_u32 target_lum;
	cmr_u32 is_stable;
	cmr_u32 *hist_data;
};

struct af_alg_gyro_info {
	cmr_u32 timetamp;
	cmr_u32 x;
	cmr_u32 y;
	cmr_u32 z;
};

struct af_alg_gsensor_info {
	cmr_u32 timetamp;
	cmr_u32 x;
	cmr_u32 y;
	cmr_u32 z;
};

struct caf_alg_result {
	cmr_u32 is_caf_trig;
	cmr_u32 is_caf_trig_in_taf;
};

struct caf_alg_calc_param {
	cmr_u32 active_data_type;
	cmr_u32 af_has_suc_rec;
	struct af_alg_afm_info afm_info;
	struct af_alg_img_blk_info img_blk_info;
	struct af_alg_ae_info ae_info;
	struct af_alg_gyro_info gyro_info;
	struct af_alg_gsensor_info gsensor_info;
};


/*------------------------------------------------------------------------------*
*					Data Prototype				*
*-------------------------------------------------------------------------------*/

signed int caf_trigger_init(struct af_alg_tuning_block_param *init_param, caf_alg_handle_t *handle);
signed int caf_trigger_deinit(caf_alg_handle_t handle);
signed int caf_trigger_calculation(caf_alg_handle_t handle,
				struct caf_alg_calc_param *alg_calc_in,
				struct caf_alg_result *alg_calc_result);
signed int caf_trigger_ioctrl(caf_alg_handle_t handle, enum af_alg_cmd cmd, void *param0, void *param1);

/*------------------------------------------------------------------------------*
*					Compiler Flag				*
*-------------------------------------------------------------------------------*/
#ifdef	 __cplusplus
}
#endif
/*-----------------------------------------------------------------------------*/
#endif
// End
