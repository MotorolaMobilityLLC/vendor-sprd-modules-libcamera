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

#ifndef _AE_COM_H_
#define _AE_COM_H_

#include "flat.h"
#include "fae.h"

#ifdef __cplusplus
extern "C" {
#endif

enum ae_alg_io_cmd {
	AE_ALG_CMD_SET_WEIGHT_TABLE,
	AE_ALG_CMD_SET_AE_TABLE,
	AE_ALG_CMD_SET_WORK_PARAM,
	AE_ALG_CMD_SET_QUICK_MODE,
	AE_ALG_CMD_SET_TARGET_LUM,
	AE_ALG_CMD_SET_AE_TABLE_RANGE,
	AE_ALG_CMD_SET_FLASH_PARAM,
	AE_ALG_CMD_SET_INDEX,
	AE_ALG_CMD_SET_EXP_ANIT,
	AE_ALG_CMD_SET_FIX_FPS,
	AE_ALG_CMD_SET_CVGN_PARAM,
	AE_ALG_CMD_SET_CONVERGE_SPEED,
	AE_ALG_CMD_SET_EV,
	AE_ALG_CMD_SET_FLICK_FLAG,
	AE_ALG_CMD_GET_EXP_BY_INDEX,
	AE_ALG_CMD_GET_WEIGHT,
	AE_ALG_CMD_GET_NEW_INDEX
};

struct ae_alg_init_in {
	cmr_u32 start_index;
	cmr_handle param_ptr;
	cmr_u32 size;
};

struct ae_alg_init_out {
	cmr_u32 start_index;
};

struct ae_alg_rgb_gain {
	cmr_u32 r;
	cmr_u32 g;
	cmr_u32 b;
};

struct ae_settings {
	cmr_u16 ver;
	cmr_s8 lock_ae;	/* 0:unlock 1:lock 2:pause 3:wait-lock */
	cmr_s32 pause_cnt;
	cmr_s8 manual_mode;	/* 0:exp&gain       1:table-index */
	cmr_u32 exp_line;	/* set exposure lines */
	cmr_u16 gain;	/* set gain: 128 means 1 time gain , user can set any value to it */
	cmr_u16 table_idx;	/* set ae-table-index */
	cmr_s16 min_fps;	/* e.g. 2000 means 20.00fps , 0 means Auto */
	cmr_s16 max_fps;
	cmr_u8 sensor_max_fps;	/*the fps of sensor setting: it always is 30fps in normal setting */
	cmr_s8 flash;	/*flash */
	cmr_s16 flash_ration;	/* mainflash : preflash -> 1x = 32 */
	cmr_s16 flash_target;
	cmr_s8 iso;
	cmr_s8 touch_scrn_status;	//touch screen,1: touch;0:no touch
	cmr_s8 touch_tuning_enable;	//for touch ae
	cmr_s8 ev_index;	/* not real value , just index !! */
	cmr_s8 flicker;	/* 50hz 0 60hz 1 */
	cmr_s8 flicker_mode;	/* auto 0 manual 1,2 */
	cmr_s8 FD_AE;	/* 0:off; 1:on */
	cmr_s8 metering_mode;
	cmr_s8 work_mode;	/* DC DV */
	cmr_s8 scene_mode;	/* pano sports night */
	cmr_s16 intelligent_module;	/* pano sports night */
	cmr_s8 af_info;	/*AF trigger info */
	cmr_s8 reserve_case;
	cmr_u8 *reserve_info;	/* reserve for future */
	cmr_s16 reserve_len;	/*len for reserve */
};

struct ae_alg_calc_param {
	struct ae_size frame_size;
	struct ae_size win_size;
	struct ae_size win_num;
	struct ae_alg_rgb_gain awb_gain;
	struct ae_exp_gain_table *ae_table;
	struct ae_size touch_tuning_win;	//for touch ae
	struct ae_trim touch_scrn_win;	//for touch ae
	struct face_tuning_param face_tp;	//for face tuning
	cmr_u8 *weight_table;
	cmr_u32 *stat_img;
	cmr_u8 monitor_shift;	//for ae monitor data overflow
	cmr_u8 win1_weight;	//for touch ae
	cmr_u8 win2_weight;	//for touch ae
	//cmr_u8 touch_tuning_enable;//for touch ae
	cmr_s16 min_exp_line;
	cmr_s16 max_gain;
	cmr_s16 min_gain;
	cmr_s16 start_index;
	cmr_s16 target_lum;
	cmr_s16 target_lum_zone;
	cmr_s16 line_time;
	cmr_s16 snr_max_fps;
	cmr_s16 snr_min_fps;
	cmr_u32 frame_id;
	cmr_u32 *r;
	cmr_u32 *g;
	cmr_u32 *b;
	cmr_s8 ae_initial;
	cmr_s8 alg_id;
	cmr_s32 effect_expline;
	cmr_s32 effect_gain;
	cmr_s32 effect_dummy;

//caliberation for bv match with lv
	float lv_cali_lv;
	float lv_cali_bv;
/*for mlog function*/
	cmr_u8 mlog_en;
/*modify the lib log level, if necissary*/
	cmr_u8 log_level;
//refer to convergence
	cmr_u8 ae_start_delay;
	cmr_s16 stride_config[2];
	cmr_s16 under_satu;
	cmr_s16 ae_satur;
	//for touch AE
	cmr_s8 to_ae_state;

//adv_alg module init
	struct ae1_fd_param ae1_finfo;
	cmr_handle adv[8];
	/*
	   0:region
	   1:flat
	   2: mulaes
	   3: touch ae
	   4: face ae
	   5:flash ae????
	 */
	struct ae_settings settings;
};

struct ae1_senseor_out {
	cmr_s8 stable;
	cmr_s8 f_stable;
	cmr_s16 cur_index;	/*the current index of ae table in ae now: 1~1024 */
	cmr_u32 exposure_time;	/*exposure time, unit: 0.1us */
	float cur_fps;	/*current fps:1~120 */
	cmr_u16 cur_exp_line;	/*current exposure line: the value is related to the resolution */
	cmr_u16 cur_dummy;	/*dummy line: the value is related to the resolution & fps */
	cmr_s16 cur_again;	/*current analog gain */
	cmr_s16 cur_dgain;	/*current digital gain */
};

struct ae_alg_calc_result {
	cmr_u32 version;	/*version No. for this structure */
	cmr_s16 cur_lum;	/*the average of image:0 ~255 */
	cmr_s16 target_lum;	/*the ae target lum: 0 ~255 */
	cmr_s16 target_zone;	/*ae target lum stable zone: 0~255 */
	cmr_s16 target_lum_ori;	/*the ae target lum(original): 0 ~255 */
	cmr_s16 target_zone_ori;	/*the ae target lum stable zone(original):0~255 */
	cmr_u32 frame_id;
	cmr_s16 cur_bv;	/*bv parameter */
	cmr_s16 cur_bv_nonmatch;
	cmr_s16 *histogram;	/*luma histogram of current frame */
	//for flash
	cmr_s32 flash_effect;
	cmr_s8 flash_status;
	cmr_s16 mflash_exp_line;
	cmr_s16 mflash_dummy;
	cmr_s16 mflash_gain;
	//for touch
	cmr_s8 tcAE_status;
	cmr_s8 tcRls_flag;
	//for face debug
	cmr_u32 face_lum;
	mulaes_stat *pmulaes;
	flat_stat *pflat;
	region_stat *pregion;
	touch_stat *ptc;	/*Bethany add touch info to debug info */
	fae_stat *pface_ae;
	struct ae1_senseor_out wts;
	cmr_handle log;
	cmr_u32 flag4idx;
	cmr_u32 *reserved;	/*resurve for future */
};

struct ae_alg_fun_tab {
	cmr_handle(*init) (struct ae_alg_init_in *, struct ae_alg_init_out *);
	cmr_s32(*deinit) (cmr_handle, cmr_handle, cmr_handle);
	cmr_s32(*calc) (cmr_handle, cmr_handle, cmr_handle);
	cmr_s32(*ioctrl) (cmr_handle, enum ae_alg_io_cmd, cmr_handle, cmr_handle);
};

#ifdef __cplusplus
}
#endif

#endif
