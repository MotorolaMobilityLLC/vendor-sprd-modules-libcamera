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

#ifndef _AE_DATA_TYPES_H_
#define _AE_DATA_TYPES_H_

#include "cmr_types.h"

#define SENSOR_GAMMA_POINT_NUM 257  //此处重定义 被修改过 原值为256
#define AE_PIECEWISE_SAMPLE_NUM 0x10
#define AE_PIECEWISE_MAX_NUM 16
#define AEC_LINETIME_PRECESION (1000000000.0f) /*ns*/
#define AEC_HIST_BIN_MAX (1<<8)
#define AEC_MONITOR_DATA_SIZE_MAX (128 * 128)
#define AE_EV_LEVEL_MAX 16
#define AE_LIB_SCENE_MAX 8
#define AE_PIECEWISE_SAMPLE2_NUM 5

#ifndef _AE_COMMON_DATA_TYPE_DEF
#define _AE_COMMON_DATA_TYPE_DEF

#ifndef WIN32
#define AE_PUBLIC __attribute__ ((visibility("default")))
#else
#define AE_PUBLIC __declspec( dllexport )
#endif

#define AE_WEIGHT_UNIT 256
#define AE_BASE_GAIN 128
#define AE_FIX_PCT100 100
#define AE_FIX_PCT1024 1024
/*for the fixed data of ae data types*/
#define AE_FIXED_10BITS (1<<10)
#define AE_FIXED_8BITS (1<<8)
#define AE_FIXED_100 100
#define AE_REAL_2_FIXED(_val, _fixed_p) ((cmr_u32)((float)(_val) * (float)(_fixed_p) + 0.5))
#define AE_FIXED_2_REAL(_val, _fixed_p) ((float)(_val) / (float)(_fixed_p))

typedef cmr_handle ae_handle_t;
typedef cmr_s32 ae_sfixed_10bits;/*fix data accuracy: 1/1024*/
typedef cmr_s32 ae_sfixed_8bits;/*fix data accuracy: 1/256*/
typedef cmr_s32 ae_sfixed_100;/*fix data accuracy: 1/100*/

enum {
	AE_ROLE_ID_NORMAL = 0,
	AE_ROLE_ID_MASTER = 1,
	AE_ROLE_ID_SLAVE0 = 2,
	AE_ROLE_ID_SLAVE1 = 3,
};

enum {
	AE_CAM_REAR = 0,
	AE_CAM_FRONT = 1,
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
	AE_SCENE_FDR,
	AE_SCENE_FACEID,
	AE_SCENE_PANORAMA,
	AE_SCENE_VIDEO,
	AE_SCENE_VIDEO_EIS,
	AE_SCENE_MOD_MAX
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
	FLASH_MAIN_CLOSE,
	FLASH_LED_ON,
	FLASH_LED_OFF,
	FLASH_LED_AUTO,
	FLASH_MAX
};

enum ae_flicker_mode {
	AE_FLICKER_50HZ = 0x00,
	AE_FLICKER_60HZ,
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

/*below structure drived from above struct,space goes down because the reserved space not enough for hm at that time.*/
struct ae_piecewise_funcv2 {
	cmr_s32 num;
	struct ae_sample samples[AE_PIECEWISE_SAMPLE2_NUM];
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
	struct ae_alg_rgb_gain awb_gain;
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
#endif


enum ae_mode_type {
	AE_MODE_AUTO = 0,/*AUTO*/
	AE_MODE_AUTO_SHUTTER_PRI = 1,/*AUTO MODE, SHUTTER PRIORITY*/
	AE_MODE_AUTO_ISO_PRI = 2,/*AUTO MODE, ISO PRIORITY*/
	AE_MODE_MANUAL_EXP_GAIN = 3,/*MANUAL AE: MODE 0: BY SHUTTER/ISO*/
	AE_MODE_MANUAL_IDX = 4,/*MANUAL AE: MODE 1: BY AE TABLE INDEX*/
	AE_MODE_MANUAL_LOCK = 5,/*MANUAL AE: Just lock ae and don't converge*/
};

enum ae_ev_mode_type {
	AE_EV_MOD_COM_VAL,
	AE_EV_MOD_OFFSET,
};

enum ae_mod_idx_type {
	AE_MOD_IDX_EXP = 0,
	AE_MOD_IDX_GAIN = 1,
	AE_MOD_IDX_ISO = 1,
};

struct ae_hist_win_info {
	cmr_u32 idx;
	cmr_u32 sec;
	cmr_u32 usec;
	cmr_u32 start_x;
	cmr_u32 start_y;
	cmr_u32 end_x;
	cmr_u32 end_y;
};

struct ae_monitor_cfg {
	struct ae_rect monitor_rect;
	struct ae_size blk_size;
	struct ae_size blk_num;
	cmr_u8 monitor_shift;		//for ae monitor data overflow
	cmr_u16 oe_thrd;
	cmr_u16 ue_thrd;
	cmr_u32 data_type;
};

struct ae_monitor_item_data_type {
	cmr_u32 oe_stats_data;
	cmr_u32 oe_conts;
	cmr_u32 med_stats_data;
	cmr_u32 med_conts;
	cmr_u32 ue_stats_data;
	cmr_u32 ue_conts;
};

#ifndef _AE_POINT_TYPE_DEF_
#define _AE_POINT_TYPE_DEF_
struct ae_point_type {
	cmr_s16 x;
	cmr_s16 y;
};
#endif

struct ae_stats_data_type {
	cmr_u32 *stat_data;/*10bit*/
	cmr_u32 counts_per_pixel;/*the size of ae monitor block: blk_size.w * blk_size.h*/
	cmr_u32 shift;
	struct ae_size size;/*stats data resolution*/
	struct ae_point_type blk_st_pt;/*aem roi start offset*/
	struct ae_size blk_size;
};

struct ae_monitor_data_type {
	struct ae_monitor_item_data_type stats_data[3][AEC_MONITOR_DATA_SIZE_MAX];
	cmr_u32 shift;
	struct ae_size size;/*stats data resolution*/
	struct ae_point_type blk_st_pt;/*aem roi start offset*/
	struct ae_size blk_size;
};

struct ae_bayer_hist_cfg {
	cmr_u32 bypass;
	cmr_u32 skip_num;
	cmr_u32 shift;
	cmr_u32 mode;
	struct ae_rect hist_rect;
};

struct ae_hist_data_type {
	cmr_u32 hist_data[AEC_HIST_BIN_MAX];
	cmr_u32 hist_bin;
	struct ae_size img_size;
};

struct ae_face_data_type {
	struct ae_rect face_rect;
	cmr_s32 pose;	/* face pose: frontal, half-profile, full-profile */
	cmr_u32 face_lum;
	cmr_s32 angle;
	cmr_s32 yaw_angle;
	cmr_s32 roll_angle;
};

struct ae_ev_setting_param {
	cmr_u32 cam_id;
	cmr_u32 ae_idx;
	cmr_u32 exp_time;
	cmr_u32 line_time;
	cmr_u32 exp_line;	
	cmr_u32 dmy_line;
	cmr_u32 frm_len;
	cmr_u32 ae_gain;
	cmr_u32 calc_y;
};

struct ae_sync_gain_param {
	cmr_u32 sensor_gain;
	cmr_u32 isp_gain;
};

struct ae_face_param {
	cmr_u32 face_num;
	struct ae_face_data_type face_data[20];
	struct ae_size img_size;/*image size*/
};

struct ae_mode_param {
	/*  0: all auto mode; 
	    1: shutter fix, iso auto;
	    2: shutter auto, iso fix;
	    3: manual mode 0 in exp/gain type;
	    4: manual mode 1 in ae index type;
	    5: manual mode keep last param;
	*/
	enum ae_mode_type mode;
	union {
		cmr_u32 exp_gain[2];            /* 0: exp_time; 1: gain or ISO value*/
		cmr_u32 ae_idx;                 /* set ae-table-index */		
	} value;
};

struct ae_control_timing_param{
	/*sensor limitation setting*/
	cmr_u16 max_gain;/*sensor max gain:include sensor a-gain and d-gain */
	cmr_u8 min_gain;/*sensor min gain */
	cmr_u8 min_exp_line;/*sensor min exposure line*/
	cmr_u8 gain_precision;/*sensor gain precision*/
	/*sensor & iso related control timing setting*/
	cmr_u8 exp_skip_num;
	cmr_u8 gain_skip_num;
	cmr_u8 isp_gain_skip_num;
	cmr_u8 group_hold_en;/*enable the sensor group hold function*/
	cmr_u8 reserved[3];
};

struct ae_flash_timing_param {
	cmr_u8 pre_param_update_delay;/*the timming of ev setting of 1st pre-flash, that is from flash algorithm,
	                                                          default vaule is 0, it be updated immediately in pre-flash before mode*/
	cmr_u8 pre_open_delay;/*the timming of open pre-flash, the default value is 0, it be opened immediately after pre-flash before mode*/
	cmr_u8 estimate_delay;/*the interval frame of flash calculation*/
	
	cmr_u8 main_param_update_delay;/*the timming of ev setting of 1st main-flash, that is for main-flash,
	                                                          default vaule is 0, it be updated immediately in pre-flash before mode*/
	cmr_u8 main_open_delay;/*the timming of open main-flash, the default value is 0, it be opened immediately after main-flash before mode*/
	cmr_u8 main_capture_delay;/*the frame delay of capturing, PS: it is from main-flash-before*/
	cmr_u8 pre_skip_num;/*the skip frame number of pre-flash*/
	cmr_u8 main_skip_num;/*the skip frame number of main-flash*/
};

struct ae_rgbgamma_curve {
	struct ae_sample points_r[SENSOR_GAMMA_POINT_NUM];/*gamma curve for r channel*/
	struct ae_sample points_g[SENSOR_GAMMA_POINT_NUM];/*gamma curve for g channel*/
	struct ae_sample points_b[SENSOR_GAMMA_POINT_NUM];/*gamma curve for b channel*/
};

struct ae_ygamma_curve {
	struct ae_sample points_y[SENSOR_GAMMA_POINT_NUM];/*gamma curve for y channel*/
};

struct ae_gamma_param{
	cmr_u32 type;/*0: full rgb gamma; 1: ygamma*/
	union {
		struct ae_rgbgamma_curve rgb_gamma;
		struct ae_ygamma_curve ygamma; 
	}data;
};

struct ae_compensation_param {
	cmr_u8 mode;/*0(AE_EV_MOD_COM_VAL): ae compensation;
				    1(AE_EV_MOD_OFFSET): EV setting in professial mode*/
	union {
		float ev_value;
		cmr_s32 ev_index;
	}value;
};

struct ae_thd_param {
	cmr_s16 thd_up;
	cmr_s16 thd_down;
};

struct ae_ev_param_item {
	cmr_s16 lum_diff;
	cmr_u8 stable_zone_in;
	cmr_u8 stable_zone_out;
};

struct ae_ev_param_table {
	struct ae_ev_param_item items[AE_EV_LEVEL_MAX];
	/* number of level */
	cmr_u32 diff_num;
	/* index of default */
	cmr_u32 default_level;
};

struct ae_bayer_hist_param {
	cmr_u32 cam_id;
	cmr_s32 frame_id;
	struct ae_hist_data_type hist_data[3];
	struct ae_rect hist_roi;
};

struct ae_touch_roi_param {
	cmr_u32 cam_id;
	cmr_s32 frame_id;
	struct ae_trim touch_roi;
};

struct ae_debug_info {
	struct ae_bayer_hist_param hist_info[3];
	struct ae_touch_roi_param touch_info[3];
};

#endif
