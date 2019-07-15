/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _AE_TUNING_TYPE_H_
#define _AE_TUNING_TYPE_H_
#include "ae_correction.h"

#define AE_CFG_NUM 8
#define AE_EXP_GAIN_TABLE_SIZE 512
#define AE_WEIGHT_TABLE_SIZE	1024
#define AE_CVGN_NUM  4
#define AE_FLICKER_NUM 2

#define MULAES_CFG_NUM AE_CFG_NUM
#define REGION_CFG_NUM AE_CFG_NUM
#define FLAT_CFG_NUM AE_CFG_NUM
#define FACE_CFG_NUM AE_CFG_NUM
#define ABL_CFG_NUM AE_CFG_NUM
#define PCP_CFG_NUM AE_CFG_NUM
#define HM_CFG_NUM AE_CFG_NUM
#define AI_CFG_NUM AE_CFG_NUM
/*=============================================*/
struct ae_exp_anti {
	cmr_u32 enable;
	cmr_u8 hist_thr[40];
	cmr_u8 hist_weight[40];
	cmr_u8 pos_lut[256];
	cmr_u8 hist_thr_num;
	cmr_u8 adjust_thr;
	cmr_u8 stab_conter;
	cmr_u8 reserved1;

	cmr_u32 reserved[175];
};


struct ae_ev_cali_param {
	cmr_u32 index;
	cmr_u32 lux;
	cmr_u32 lv;
};

struct ae_ev_cali {
	cmr_u32 num;
	cmr_u32 min_lum;			// close all the module of after awb module
	struct ae_ev_cali_param tab[16];	// cali EV sequence is low to high
};
#define AE_OFFSET_NUM 20
struct ae_convergence_parm {
	cmr_u32 highcount;
	cmr_u32 lowcount;
	cmr_u32 highlum_offset_default[AE_OFFSET_NUM];
	cmr_u32 lowlum_offset_default[AE_OFFSET_NUM];
	cmr_u32 highlum_index[AE_OFFSET_NUM];
	cmr_u32 lowlum_index[AE_OFFSET_NUM];
};

struct ae_param {
	cmr_handle param;
	cmr_u32 size;
};

struct ae_exp_gain_delay_info {
	cmr_u8 group_hold_flag;
	cmr_u8 valid_exp_num;
	cmr_u8 valid_gain_num;
};
/*============================================*/
struct ae_param_tmp_001 {
	cmr_u32 version;
	cmr_u32 verify;
	cmr_u32 alg_id;
	cmr_u32 target_lum;
	cmr_u32 target_lum_zone;	// x16
	cmr_u32 convergence_speed;	// x16
	cmr_u32 flicker_index;
	cmr_u32 min_line;
	cmr_u32 start_index;
	cmr_u32 exp_skip_num;
	cmr_u32 gain_skip_num;
	//struct ae_stat_req stat_req;
	//struct ae_flash_tuning flash_tuning;
	//struct touch_zone touch_param;
	//struct ae_ev_table ev_table;
};

struct ae_param_tmp_002 {
	struct ae_exp_anti exp_anti;
	struct ae_ev_cali ev_cali;
	struct ae_convergence_parm cvgn_param[AE_CVGN_NUM];
};

struct mulaes_cfg {
	cmr_s16 x_idx;
	cmr_s16 y_lum;
};

struct mulaes_tuning_param {
	cmr_u8 enable;
	cmr_u8 num;
	cmr_u16 reserved;			/*1 * 4bytes */
	struct mulaes_cfg cfg[MULAES_CFG_NUM];	/*8 * 4bytes */
};								/*9 * 4bytes */

typedef struct {
	struct ae_range region_thrd[6];	/*u d l r */
	cmr_s16 up_max;
	cmr_s16 dwn_max;
	cmr_s16 vote_region[6];		/*u d l r */
} region_cfg;					/*16 * 4bytes */

struct region_tuning_param {
	cmr_u8 enable;
	cmr_u8 num;
	cmr_u16 reserved;			/*1 * 4bytes */
	region_cfg cfg_info[REGION_CFG_NUM];	/*total 8 group: 128 * 4bytes */
	struct ae_piecewise_func input_piecewise;	/*17 * 4bytes */
	struct ae_piecewise_func u_out_piecewise;	/*17 * 4bytes */
	struct ae_piecewise_func d_out_piecewise;	/*17 * 4bytes */
};								/*180 * 4bytes */

typedef struct {
	cmr_s16 thrd[2];
	cmr_s16 offset[2];
} flat_cfg;						/*2 * 4bytes */

struct flat_tuning_param {
	/*1 * 4bytes */
	cmr_u8 enable;
	cmr_u8 num;
	cmr_u16 reserved;
	/*flat tune param; total 8 group */
	flat_cfg cfg_info[FLAT_CFG_NUM];	/*16 * 4bytes */
	struct ae_piecewise_func out_piecewise;	/*17 * 4bytes */
	struct ae_piecewise_func in_piecewise;	/*17 * 4bytes */
};								/*51 * 4bytes */

struct face_cfg {
	cmr_s16 x_idx;
	cmr_u8 y_lum;
	cmr_u8 up_limit;
	cmr_u8 down_limit;
	cmr_u8 ratio_block;			//10~100 will trans 0~1
	cmr_u8 ratio_position;		//10~100 will trans 0~1
	cmr_u8 max_with_ratio;		//10~100 will trans 0~1
};

struct face_tuning_param {
	cmr_u8 face_tuning_enable;
	cmr_u8 face_target;			//except to get the face lum
	cmr_u8 face_tuning_lum1;	// scope is [0,256]
	cmr_u8 face_tuning_lum2;	//if face lum > this value, offset will set to be 0
	cmr_u16 cur_offset_weight;	//10~100 will trans 0~1
	cmr_u16 up_face_offset;		//limit up face offset
	cmr_u16 down_face_offset;	//limit down face offset
	cmr_u8 ratio_block;			//10~100 will trans 0~1
	cmr_u8 ratio_position;		//10~100 will trans 0~1
	cmr_u8 max_with_ratio;		//10~100 will trans 0~1
	cmr_u8 num;
	struct face_cfg cfg[FACE_CFG_NUM];	/*8 * 6bytes */
	cmr_u8 face_disappear_count;
	cmr_u8 face_block_threshold;
	cmr_u8 ae_thd;
	cmr_u8 face_low_thd;
	cmr_u8 face_high_thd;
	cmr_u8 reserved0;
	cmr_u16 reserved[2];		//?
};

struct ae_touch_param {
	cmr_u8 win2_weight;			//for touch ae
	cmr_u8 enable;				//for touch ae
	cmr_u8 win1_weight;			//for touch ae
	cmr_u8 reserved;			//for touch ae
	struct ae_size touch_tuning_win;	//for touch ae
};

struct ae_face_tune_param {
	cmr_s32 param_face_weight;	/* The ratio of face area weight (in percent) */
	cmr_s32 param_convergence_speed;	/* AE convergence speed */
	cmr_s32 param_lock_ae;		/* frames to lock AE */
	cmr_s32 param_lock_weight_has_face;	/* frames to lock the weight table, when has faces */
	cmr_s32 param_lock_weight_no_face;	/* frames to lock the weight table, when no faces */
	cmr_s32 param_shrink_face_ratio;	/* The ratio to shrink face area. In percent */
};

struct ae_weight_table {
	cmr_u8 weight[AE_WEIGHT_TABLE_SIZE];
};
struct ae_auto_iso_tab {
	cmr_u16 tbl[AE_FLICKER_NUM][AE_EXP_GAIN_TABLE_SIZE];
};

#endif