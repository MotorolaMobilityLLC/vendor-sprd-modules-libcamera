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

#ifndef _AE_COMMON_H_
#define _AE_COMMON_H_
#include "cmr_types.h"

#define AEC_LINETIME_PRECESION (1000000000.0f) /*ns*/
#define AE_ISO_NUM	6
#define AE_PARAM_VERIFY	0x61656165
#define AE_TABLE_32
#define AE_BAYER_CHNL_NUM 4
#define AE_PIECEWISE_MAX_NUM 16
#define AE_PIECEWISE_SAMPLE_NUM 0x10
#define AE_FD_NUM 20
#define AE_BASE_TABLE_SIZE 16384
#define NGT_BV 600
#define IDR_BV 900
#define ODR_BV 1500
#define INDOOR_THD 700
#define OUTDOOR_THD 1300


#ifndef _AE_COMMON_DATA_TYPE_DEF
#define _AE_COMMON_DATA_TYPE_DEF

#ifndef WIN32
#define AE_PUBLIC __attribute__ ((visibility("default")))
#else
#define AE_PUBLIC __declspec( dllexport )
#endif

#define AE_WEIGHT_UNIT 256
#define AE_BASE_GAIN 128
#define AE_FIX_PCT1024 1024
#define AE_FIX_PCT100 100

typedef cmr_handle ae_handle_t;

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

enum ae_iso_mode {
	AE_ISO_AUTO = 0x00,
	AE_ISO_100,
	AE_ISO_200,
	AE_ISO_400,
	AE_ISO_800,
	AE_ISO_1600,
	AE_ISO_3200,
	AE_ISO_6400,
	AE_ISO_12800,
	AE_ISO_MAX
};

enum ae_state {
	AE_STATE_NORMAL,
	AE_STATE_LOCKED,
	//AE_STATE_PAUSE,
	AE_STATE_SEARCHING,
	AE_STATE_CONVERGED,
	AE_STATE_FLASH_REQUIRED,
	AE_STATE_PRECAPTURE,
	AE_STATE_INACTIVE,
	AE_STATE_MAX
};

enum ae_scene_mode {
	AE_SCENE_NORMAL = 0x00,
	AE_SCENE_NIGHT,
	AE_SCENE_SPORT,
	AE_SCENE_PORTRAIT,
	AE_SCENE_LANDSPACE,
	AE_SCENE_FACEID_UNLOCK,
	AE_SCENE_PANORAMA,
	AE_SCENE_MOD_MAX
};

enum aec_work_mode {
	AEC_WORK_MODE_COMMON = 0x00,
	AEC_WORK_MODE_CAPTURE,
	AEC_WORK_MODE_VIDEO,
	AEC_WORK_MODE_MAX
};

enum alg_flash_type {
	FLASH_NONE,
	FLASH_PRE_BEFORE,
	FLASH_PRE_BEFORE_RECEIVE,
	FLASH_PRE,
	FLASH_PRE_RECEIVE,
	FLASH_PRE_AFTER,
	FLASH_PRE_AFTER_RECEIVE,
	FLASH_MAIN_BEFORE,
	FLASH_MAIN_BEFORE_RECEIVE,
	FLASH_MAIN,
	FLASH_MAIN_RECEIVE,
	FLASH_MAIN_AFTER,
	FLASH_MAIN_AFTER_RECEIVE,
	FLASH_LED_ON,
	FLASH_LED_OFF,
	FLASH_LED_AUTO,
	FLASH_MAX
};

enum ae_flicker_mode {
	AE_FLICKER_50HZ = 0x00,
	AE_FLICKER_60HZ,
	AE_FLICKER_OFF,
	AE_FLICKER_AUTO,
	AE_FLICKER_MAX
};

enum ae_sensor_role_type {
	AE_SENSOR_SINGLE = 0,
	AE_SENSOR_MASTER = 1,
	AE_SENSOR_SLAVE0 = 2,
	AE_SENSOR_SLAVE1 = 3,
	AE_SENSOR_MAX
};

enum ae_ai_scene_type {
	AE_AI_SCENE_DEFAULT,
	AE_AI_SCENE_FOOD,
	AE_AI_SCENE_PORTRAIT,
	AE_AI_SCENE_FOLIAGE,
	AE_AI_SCENE_SKY,
	AE_AI_SCENE_NIGHT,
	AE_AI_SCENE_BACKLIGHT,
	AE_AI_SCENE_TEXT,
	AE_AI_SCENE_SUNRISE,
	AE_AI_SCENE_BUILDING,
	AE_AI_SCENE_LANDSCAPE,
	AE_AI_SCENE_SNOW,
	AE_AI_SCENE_FIREWORK,
	AE_AI_SCENE_BEACH,
	AE_AI_SCENE_PET,
	AE_AI_SCENE_FLOWER,
	AE_AI_SCENE_MAX
};

enum ai_task0 {
	AE_AI_SCENE_TASK0_INDOOR,
	AE_AI_SCENE_TASK0_OUTDOOR,
	AE_AI_SCENE_TASK0_MAX
};

enum ai_task1 {
	AE_AI_SCENE_TASK1_NIGHT,
	AE_AI_SCENE_TASK1_BACKLIGHT,
	AE_AI_SCENE_TASK1_SUNRISESET,
	AE_AI_SCENE_TASK1_FIREWORK,
	AE_AI_SCENE_TASK1_OTHERS,
	AE_AI_SCENE_TASK1_MAX
};

enum ai_task2 {
	AE_AI_SCENE_TASK2_FOOD,
	AE_AI_SCENE_TASK2_GREENPLANT,
	AE_AI_SCENE_TASK2_DOCUMENT,
	AE_AI_SCENE_TASK2_CATDOG,
	AE_AI_SCENE_TASK2_FLOWER,
	AE_AI_SCENE_TASK2_BLUESKY,
	AE_AI_SCENE_TASK2_BUILDING,
	AE_AI_SCENE_TASK2_SNOW,
	AE_AI_SCENE_TASK2_OTHERS,
	AE_AI_SCENE_TASK2_MAX
};

enum ae_binning_mode {
	AE_BNNG_MOD_AVG = 0,
	AE_BNNG_MOD_SUM,
};

struct ae_ct_table {
	float ct[20];
	float rg[20];
};

struct ae_weight_value {
	cmr_s16 value[2];
	cmr_s16 weight[2];
};

#ifndef _AE_SAMPLE_DEF_
#define _AE_SAMPLE_DEF_
struct ae_sample {
	cmr_s16 x;
	cmr_s16 y;
};
#endif

struct ae_piecewise_func {
	cmr_s32 num;
	struct ae_sample samples[AE_PIECEWISE_SAMPLE_NUM];
};

struct ae_range {
	cmr_s32 min;
	cmr_s32 max;
};

struct ae_ranges_type {
	cmr_u32 num;
	struct ae_range range[AE_PIECEWISE_MAX_NUM];
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

struct ae_rgb_l {
	cmr_u32 r;
	cmr_u32 g;
	cmr_u32 b;
};

struct ae_opt_info {
	struct ae_rgb_l gldn_stat_info;
	struct ae_rgb_l rdm_stat_info;
};

struct ae_alg_rgb_gain {
	cmr_u32 r;
	cmr_u32 g;
	cmr_u32 b;
};

struct ae_stats_gyro_info{
	/* Gyro data in float */
	cmr_u32 validate;
	cmr_s64 timestamp;
	float x;
	float y;
	float z;
};

struct ae_stats_accelerator_info {
	cmr_u32 validate;
	cmr_s64 timestamp;
	float x;
	float y;
	float z;
};

struct ae_stats_sensor_info {
	cmr_u32 aux_sensor_support;
	struct ae_stats_gyro_info gyro;
	struct ae_stats_accelerator_info accelerator;
};

struct ai_task0_rt {
	enum ai_task0 id;
	cmr_u16 reliability;
};

struct ai_task1_rt {
	enum ai_task1 id;
	cmr_u16 reliability;
};

struct ai_task2_rt {
	enum ai_task2 id;
	cmr_u16 reliability;
};

struct ai_scene_detect {
	cmr_u32 frame_id;
	enum ae_ai_scene_type scene_id;
	struct ai_task0_rt task0[AE_AI_SCENE_TASK0_MAX];
	struct ai_task1_rt task1[AE_AI_SCENE_TASK1_MAX];
	struct ai_task2_rt task2[AE_AI_SCENE_TASK2_MAX];
};

struct otp_ae_info {
    cmr_u16 ae_target_lum;
    cmr_u64 gain_1x_exp;
    cmr_u64 gain_2x_exp;
    cmr_u64 gain_4x_exp;
    cmr_u64 gain_8x_exp;
    cmr_u64 reserve;
};

struct ae_sync_info{			//ae_dynamic_sync struct
	cmr_s16 min_exp_line;
	cmr_s16 max_again;
	cmr_s16 min_again;
	cmr_s16 sensor_gain_precision;
	cmr_u32 line_time;
	cmr_u32 aem[3 * 1024];
	struct otp_ae_info ae_otp_info;
	cmr_u32 exposure;
	cmr_u32 gain;
	cmr_s32 dmy_line;
	cmr_u32 frm_len;
	cmr_u32 frm_len_def;
	cmr_u64 monoboottime;
};

//ae_sync_param
 struct ae_sync_para{
	cmr_u32 magic_first_num;
	cmr_u32 version;
	cmr_u32 mode ; //0: OTP mode;1:dynamic mode
	cmr_u32 y_ratio_chg_thr; //thr,cnt
	cmr_u32 y_ratio_chg_cnt ;
	cmr_u32 y_ratio_stb_thr;
	cmr_u32 y_ratio_stb_cnt;
	cmr_u32 adpt_speed; //adapt speed
	cmr_u8 soft_frm_sync;/*software frame sync--enable*/
	cmr_u8 adj_ratio;/*software frame sync--ajdust ratio: 0~100*/
	cmr_u8 reserved[2];
	cmr_u32 adj_thrd;/*software frame sync--adjust threshold: unit: us*/
	cmr_s8 reserve[20];
	cmr_u32 magic_end_num;
};

struct ae_alg_fun_tab {
	cmr_handle(*init) (cmr_handle, cmr_handle);
	cmr_s32(*deinit) (cmr_handle, cmr_handle, cmr_handle);
	cmr_s32(*calc) (cmr_handle, cmr_handle, cmr_handle);
	cmr_s32(*sync_calc) (cmr_handle,cmr_handle, cmr_handle, cmr_handle);
	cmr_s32(*ioctrl) (cmr_handle, cmr_u32, cmr_handle, cmr_handle);
};
#else
#ifdef AE3X_PORTING_DEBUG
#include "ae_data_types.h"
#endif
#endif

enum ae_environ_mod {
	ae_environ_night,
	ae_environ_lowlux,
	ae_environ_normal,
	ae_environ_hightlight,
	ae_environ_num,
};

enum {
	AE_3DNR_ON,
	AE_3DNR_OFF,
	AE_3DNR_AUTO,
	AE_3DNR_MAX,
};

#ifndef FD_AE_PARAM_DEF
#define FD_AE_PARAM_DEF
struct ae1_face {
	cmr_u32 start_x;
	cmr_u32 start_y;
	cmr_u32 end_x;
	cmr_u32 end_y;				/*4 x 4bytes */
	cmr_s32 pose;				/* face pose: frontal, half-profile, full-profile */
	cmr_u32 face_lum;
	cmr_s32 angle;
};

struct ae1_face_info {
	cmr_u16 face_num;
	cmr_u16 reserved;			/*1 x 4bytes */
	cmr_u32 rect[1024];			/*1024 x 4bytes */
	struct ae1_face face_area[20];	/*20 x 5 * 4bytes */
};								/*1125 x 4bytes */
struct ae1_fd_param {
	struct ae1_face_info cur_info;	/*1125 x 4bytes */
	cmr_u8 update_flag;
	cmr_u8 enable_flag;
	cmr_u16 reserved;			/*1 x 4bytes */
	cmr_u16 img_width;
	cmr_u16 img_height;			/*1 x 4bytes */
};
#endif

#ifndef _AE_TARGET_WEIGHT_FUNC_DEF
#define _AE_TARGET_WEIGHT_FUNC_DEF
#define AE_TARGET_WEIGHT_NUM 5
struct ae_target_weight_func {
	cmr_s32 num;
	struct ae_sample samples[AE_TARGET_WEIGHT_NUM];
};
#endif

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
	cmr_u32 min_fps;			// min fps
	cmr_u32 max_fps;			// fix fps flag
};

struct ae_flash_ctrl {
	cmr_u32 enable;
	cmr_u32 main_flash_lum;
	cmr_u32 convergence_speed;
};

struct ae_alg_init_in {
	cmr_u32 flash_version;
	cmr_u32 start_index;
	cmr_handle param_ptr;
	cmr_u32 size;
};

struct ae_alg_init_out {
	cmr_u32 start_index;
};

struct ae_hist_info {
	cmr_u32 value[256];
	cmr_s32 frame_id;
	cmr_u32 sec;
	cmr_u32 usec;
};

struct ae_buffer_param {
	cmr_u32 ae_target_smooth[5];
	cmr_s32 ae_target_offset_smooth[5];
	cmr_u32 ae_lum_array[3];
	cmr_s32 ae_bv_list[5];
	cmr_u32 ae_lv_array[3];
	cmr_u32 ae_motion_array[5];
};

struct ae_exp_param {
	cmr_u32 frm_id;
	cmr_s32 index;
	cmr_u32 predict_lum;
	cmr_u32 exp_time;
	cmr_u32 exp_line;
	cmr_s32 dummy_line;
	cmr_u32 gain;
	cmr_s32 trend;
};

struct ae_ev_convert {
	float ev_change;
};

struct ae_zoom_info {
	cmr_u32 start_x;
	cmr_u32 start_y;
	cmr_u32 end_x;
	cmr_u32 end_y;
	cmr_u32 zoom_ratio;
};

struct ae_settings {
	cmr_u16 ver;
	cmr_u8 force_lock_ae;
	cmr_s8 lock_ae;				/* 0:unlock 1:lock 2:pause 3:wait-lock */
	cmr_s8 exp_is_transmit;		/*ae crtl unlock transmit exppsoure value wo ae lib  0:unable 1:enable*/
	cmr_s32 pause_cnt;
	cmr_s8 manual_mode;			/* 0:exp&gain       1:table-index */
	cmr_u32 exp_line;			/* set exposure lines */
	cmr_u16 gain;				/* set gain: 128 means 1 time gain , user can set any value to it */
	cmr_u16 table_idx;			/* set ae-table-index */
	cmr_u16 min_fps;				/* e.g. 2000 means 20.00fps , 0 means Auto */
	cmr_u16 max_fps;
	cmr_u16 sensor_max_fps;		/*the fps of sensor setting: it always is 30fps in normal setting */
	cmr_s8 flash;				/*flash */
	cmr_s16 flash_ration;		/* mainflash : preflash -> 1x = 32 */
	cmr_s16 flash_target;
	cmr_s8 iso;
	cmr_s8 touch_scrn_status;	//touch screen,1: touch;0:no touch
	cmr_s8 touch_tuning_enable;	//for touch ae
	cmr_s8 ev_index;			/* not real value , just index !! */
	cmr_s8 flicker;				/* 50hz 0 60hz 1 */
	cmr_s8 flicker_mode;		/* auto 0 manual 1,2 */
	cmr_s8 FD_AE;				/* 0:off; 1:on */
	cmr_s8 metering_mode;
	cmr_s8 work_mode;			/* DC DV */
	cmr_s8 scene_mode;			/* pano sports night */
	cmr_s16 intelligent_module;	/* pano sports night */
	cmr_s8 af_info;				/*AF trigger info */
	cmr_s8 reserve_case;			/*0 normal mode, all auto
								    1 manual AE:all fix: the exp/gain by APP
                                                                2 Shutter priority: shutter fix, ISO auto
                                                                3 ISO priority: ISO fix, shutter fix.*/
	cmr_u32 iso_special_mode;
	cmr_u32 iso_manual_status;	/*iso manual setting */
	cmr_u32 ev_manual_status;	/*ev manual setting */
	cmr_u8 *reserve_info;		/* reserve for future */
	cmr_s16 reserve_len;		/*len for reserve */
	cmr_u8 is_snapshot;
	cmr_u8 threednr_mode;
	cmr_s16 led_thr_up;		/* judge flash auto mode  flash unable up threahold*/
	cmr_s16 led_thr_down;	/* judge flash auto mode  flash enable down threahold*/
	cmr_u8 touch_ev_flag;
	cmr_u8 af_status;
	cmr_u8 is_fourcell;/*1: four cell mode(indoor&low-light); 0: remosaic(outdoor); -1: invalidate*/
};

struct ae_alg_calc_param {
	cmr_u32 cam_id;
	cmr_u32 rear_sub; /*0: rear 1: sub*/
	cmr_u32 ai_mode_en;
	struct ae_size frame_size;
	struct ae_size win_size;
	struct ae_size win_num;
	struct ae_alg_rgb_gain awb_gain;
	struct ae_exp_gain_table *ae_table;
	struct ae_size touch_tuning_win;	//for touch ae
	struct ae_trim touch_scrn_win;	//for touch ae
	cmr_u8 *weight_table;
	struct ae_stats_sensor_info aux_sensor_data;
	cmr_u32 posture_calibration;
	cmr_u32 *stat_img;/*10bit*/
	cmr_u32 *base_img;/*10bit*/
	struct ae_size base_size;
	struct ae_size base_num;
	cmr_u16 *binning_stat_data;
	struct ae_size binnig_stat_size;
	cmr_u8 monitor_shift;		//for ae monitor data overflow
	cmr_u8 win1_weight;			//for touch ae
	cmr_u8 win2_weight;			//for touch ae
	cmr_s8 target_offset;
	cmr_s16 base_target;/*save the default target from tuning*/
	cmr_s16 target_lum;
	cmr_s16 target_lum_zone;/*for AE2.2*/
	cmr_u16 target_range_in_zone;/*for AE2.5*/
	cmr_u16 target_range_out_zone;/*for AE2.5*/
	cmr_u16 target_range_near_zone;/*for AE2.5*/
	cmr_s16 start_index;
	cmr_u32 line_time;
	cmr_u16 snr_max_fps;
	cmr_u16 snr_min_fps;
	cmr_u32 frame_id;
	cmr_u32 *r;
	cmr_u32 *g;
	cmr_u32 *b;
	cmr_s8 ae_initial;
	cmr_u32 alg_id;
	cmr_u8 effect_binning_mod;
	cmr_s32 effect_expline;
	cmr_s32 effect_gain;
	cmr_s32 effect_dummy;
	cmr_s32 effect_frm_len;
	cmr_s32 effect_idx;
	cmr_u8 led_state;			//0:off, 1:on
	cmr_u8 flash_fired;		//just notify APP in flash auto
	cmr_s32 flash_mode;		//0:off, 1:force, 3:auto
	cmr_u8 threednr_status;
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
	//for face AE
	struct ae1_fd_param ae1_finfo;
//adv_alg module init
	cmr_handle adv[10];
	/*
	   0: region
	   1: flat
	   2: mulaes
	   3: touch ae
	   4: face ae
	   5: flash ae
	   6: AI
	   7: abl
	   8: hm
	   9: face adv
	 */
	struct ae_settings settings;
	cmr_u32 awb_mode;
	struct ae_alg_rgb_gain awb_cur_gain;
	struct ai_scene_detect detect_scene;
	struct ae_hist_info hist_info;
	struct ae_ev_convert ev_convert;
	cmr_u32 simulate_close_fd_trigger;
	cmr_u32 simulate_close_tc_trigger;
	cmr_u8 fast_cvgn_disab;
	cmr_u32 debug_info_size;
	cmr_u32 cam_4in1_cap_flag;
	struct ae_zoom_info zoom_info;
};

struct ae1_senseor_out {
	cmr_s8 stable;
	cmr_s8 f_stable;
	cmr_u8 near_stable;
	cmr_s16 cur_index;			/*the current index of ae table in ae now: 1~1024 */
	cmr_u32 exposure_time;		/*exposure time, unit: 0.1us */
	float cur_fps;				/*current fps:1~120 */
	cmr_u16 cur_exp_line;		/*current exposure line: the value is related to the resolution */
	cmr_u16 cur_dummy;			/*dummy line: the value is related to the resolution & fps */
	cmr_s16 cur_again;			/*current analog gain */
	cmr_s16 cur_dgain;			/*current digital gain */
	cmr_s32 cur_bv;
	cmr_u32 frm_len;
	cmr_u32 frm_len_def;
	cmr_u8 binning_mode;
};

struct ae_alg_calc_result {
	cmr_u32 version;			/*version No. for this structure */
	cmr_s16 cur_lum;			/*the lum of image:0 ~255 */
	cmr_s16 cur_lum_avg;		/*the lum without weight of image:0 ~255*/
	cmr_s16 target_lum;			/*the ae target lum: 0 ~255 */
	cmr_s16 target_range_in;		/*ae target lum stable zone: 0~255 */
	cmr_s16 target_range_out;
	cmr_s16 target_zone;		/*ae target lum stable zone: 0~255 */
	cmr_s16 target_lum_ori;		/*the ae target lum(original): 0 ~255 */
	cmr_s16 target_zone_ori;	/*the ae target lum stable zone(original):0~255 */
	cmr_u32 frame_id;
	cmr_s16 cur_bv;				/*bv parameter */
	cmr_s16 cur_bv_nonmatch;
	cmr_s16 *histogram;			/*luma histogram of current frame */
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
	void *pmulaes;
	void *pflat;
	void *pregion;
	void *pai;
	void *pabl;
	void *ppcp;
	void *phm;
	void *pns;
	void *ptc;					/*Bethany add touch info to debug info */
	void *pface_ae;
	struct ae1_senseor_out wts;
	cmr_handle log;
	cmr_u32 flag4idx;
	cmr_u32 face_stable;
	cmr_u32 face_trigger;
	cmr_u32 target_offset;
	cmr_u32 privated_data;
	cmr_u32 abl_weighting;
};

#endif
