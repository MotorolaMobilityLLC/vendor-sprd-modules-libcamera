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

#ifndef _AE_TYPES_H_
#define _AE_TYPES_H_

/*----------------------------------------------------------------------------*
 **				 Dependencies					*
 **---------------------------------------------------------------------------*/
#ifdef CONFIG_FOR_TIZEN
#include "stdint.h"
#elif WIN32
#include "ae_porting.h"
#else
#include "cmr_types.h"
#endif
/**---------------------------------------------------------------------------*
 **				 Compiler Flag					*
 **---------------------------------------------------------------------------*/
#define AE_EXP_GAIN_TABLE_SIZE 512
#define AE_WEIGHT_TABLE_SIZE	1024
#define AE_ISO_NUM	6
#define AE_ISO_NUM_NEW 8
#define AE_SCENE_NUM	8
#define AE_FLICKER_NUM 2
#define AE_WEIGHT_TABLE_NUM 3
#define AE_EV_LEVEL_NUM 16
#define AE_PARAM_VERIFY	0x61656165
#define AE_OFFSET_NUM 20
#define AE_CVGN_NUM  4
#define AE_TABLE_32
#define AE_BAYER_CHNL_NUM 4
#define AE_PIECEWISE_MAX_NUM 16
#define AE_WEIGHT_UNIT 256
#define AE_FIX_PCT 1024
#define AE_PIECEWISE_SAMPLE_NUM 0x10
#define AE_CFG_NUM 8

enum ae_environ_mod {
	ae_environ_night,
	ae_environ_lowlux,
	ae_environ_normal,
	ae_environ_hightlight,
	ae_environ_num,
};

enum ae_return_value {
	AE_SUCCESS = 0x00,
	AE_ERROR,
	AE_PARAM_ERROR,
	AE_PARAM_NULL,
	AE_FUN_NULL,
	AE_HANDLER_NULL,
	AE_HANDLER_ID_ERROR,
	AE_ALLOC_ERROR,
	AE_FREE_ERROR,
	AE_DO_NOT_WRITE_SENSOR,
	AE_SKIP_FRAME,
	AE_RTN_MAX
};

enum ae_calc_func_y_type {
	AE_CALC_FUNC_Y_TYPE_VALUE = 0,
	AE_CALC_FUNC_Y_TYPE_WEIGHT_VALUE = 1,
};

typedef cmr_handle ae_handle_t;
struct ae_weight_value {
	cmr_s16 value[2];
	cmr_s16 weight[2];
};

struct ae_sample {
	cmr_s16 x;
	cmr_s16 y;
};

struct ae_piecewise_func {
	cmr_s32 num;
	struct ae_sample samples[AE_PIECEWISE_SAMPLE_NUM];
};

struct ae_range {
	cmr_s32 min;
	cmr_s32 max;
};

struct ae_size {
	cmr_u32 w;
	cmr_u32 h;
};

struct ae_trim {
	cmr_u32 x;
	cmr_u32 y;
	cmr_u32 w;
	cmr_u32 h;
};

struct ae_rect {
	cmr_u32 start_x;
	cmr_u32 start_y;
	cmr_u32 end_x;
	cmr_u32 end_y;
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

struct ae_set_fps {
	cmr_u32 min_fps;	// min fps
	cmr_u32 max_fps;	// fix fps flag
};

struct ae_exp_gain_table {
	cmr_s32 min_index;
	cmr_s32 max_index;
	cmr_u32 exposure[AE_EXP_GAIN_TABLE_SIZE];
	cmr_u32 dummy[AE_EXP_GAIN_TABLE_SIZE];
	cmr_u16 again[AE_EXP_GAIN_TABLE_SIZE];
	cmr_u16 dgain[AE_EXP_GAIN_TABLE_SIZE];
};

struct ae_weight_table {
	cmr_u8 weight[AE_WEIGHT_TABLE_SIZE];
};

struct ae_ev_table {
	cmr_s32 lum_diff[AE_EV_LEVEL_NUM];
	/*number of level */
	cmr_u32 diff_num;
	/*index of default */
	cmr_u32 default_level;
};

struct ae_flash_ctrl {
	cmr_u32 enable;
	cmr_u32 main_flash_lum;
	cmr_u32 convergence_speed;
};

struct touch_zone {
	cmr_u32 level_0_weight;
	cmr_u32 level_1_weight;
	cmr_u32 level_1_percent;	//x64
	cmr_u32 level_2_weight;
	cmr_u32 level_2_percent;	//x64
};

struct ae_flash_tuning {
	cmr_u32 exposure_index;
};

struct ae_stat_req {
	cmr_u32 mode;		//0:normal, 1:G(center area)
	cmr_u32 G_width;	//100:G mode(100x100)
};

struct ae_auto_iso_tab {
	cmr_u16 tbl[AE_FLICKER_NUM][AE_EXP_GAIN_TABLE_SIZE];
};

struct ae_ev_cali_param {
	cmr_u32 index;
	cmr_u32 lux;
	cmr_u32 lv;
};

struct ae_ev_cali {
	cmr_u32 num;
	cmr_u32 min_lum;	// close all the module of after awb module
	struct ae_ev_cali_param tab[16];	// cali EV sequence is low to high
};

struct ae_rgb_l {
	cmr_u32 r;
	cmr_u32 g;
	cmr_u32 b;
};

struct ae_opt_info {
	struct ae_rgb_l gldn_stat_info;
	struct ae_rgb_l rdm_stat_info;
};

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

struct ae_convergence_parm {
	cmr_u32 highcount;
	cmr_u32 lowcount;
	cmr_u32 highlum_offset_default[AE_OFFSET_NUM];
	cmr_u32 lowlum_offset_default[AE_OFFSET_NUM];
	cmr_u32 highlum_index[AE_OFFSET_NUM];
	cmr_u32 lowlum_index[AE_OFFSET_NUM];
};

struct ae_flash_tuning_param {
	cmr_u8 skip_num;
	cmr_u8 target_lum;
	cmr_u8 adjust_ratio;	/* 1x --> 32 */
	cmr_u8 reserved;
};

struct ae_sensor_cfg {
	cmr_u16 max_gain;
	cmr_u16 min_gain;
	cmr_u8 gain_precision;
	cmr_u8 exp_skip_num;
	cmr_u8 gain_skip_num;
	cmr_u8 reserved;
};

struct ae_lv_calibration {
	cmr_u16 lux_value;
	cmr_s16 bv_value;
};

struct ae_face_tune_param {
	cmr_s32 param_face_weight;	/* The ratio of face area weight (in percent) */
	cmr_s32 param_convergence_speed;	/* AE convergence speed */
	cmr_s32 param_lock_ae;	/* frames to lock AE */
	cmr_s32 param_lock_weight_has_face;	/* frames to lock the weight table, when has faces */
	cmr_s32 param_lock_weight_no_face;	/* frames to lock the weight table, when no faces */
	cmr_s32 param_shrink_face_ratio;	/* The ratio to shrink face area. In percent */
};

struct ae_scene_info {
	cmr_u32 enable;
	cmr_u32 scene_mode;
	cmr_u32 target_lum;
	cmr_u32 iso_index;
	cmr_u32 ev_offset;
	cmr_u32 max_fps;
	cmr_u32 min_fps;
	cmr_u32 weight_mode;
	//cmr_u32 default_index;
	cmr_u8 table_enable;
	cmr_u8 exp_tbl_mode;
	cmr_u16 reserved0;
	cmr_u32 reserved1;
	struct ae_exp_gain_table ae_table[AE_FLICKER_NUM];
};
/**---------------------------------------------------------------------------*/
#endif
