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
#ifndef _AE_CORRECTION_H_
#define _AE_CORRECTION_H_
#include "ae_data_types.h"

#ifdef __cplusplus
extern "C" {
#endif

enum ae_cmd_type {
	AE_LIB_SET_BASE = 0x1000,
	AE_LIB_SET_START,
	AE_LIB_SET_STOP,
	AE_LIB_SET_CAF_START,
	AE_LIB_SET_CAF_STOP,
	AE_LIB_SET_CMD_MAX,
	AE_LIB_GET_BASE = 0x2000,
	AE_LIB_GET_DEBUG_INFO,
	AE_LIB_GET_ALG_ID,
	AE_LIB_GET_SCENE_PARAM,
	AE_LIB_GET_AEM_PARAM,
	AE_LIB_GET_CMD_MAX,
	AE_LIB_CMD_MAX
};

enum ae_sync_type {
	AE_SYNC_0,/*fps & brightness sync: for bokeh*/
	AE_SYNC_1,/*for brigness sync: for zoom*/
};

struct tar_lum_range{//added by feifan.wang
	cmr_u32 target_lum_range_in_bak;
	cmr_u32 target_lum_range_out_bak;
};

struct ae_lib_init_in {
	cmr_u32 cam_id;
	cmr_u32 mlog_en;
	cmr_u32 log_level;
	cmr_u32 line_time;
	struct ae_size img_size;/*image resolution*/
	void *tuning_param;
	void *multi_cam_tuning_param;
	/*will be used in the future*/
	void *gamma_param;/*gamma curve param*/
	void *smart_gamma_param;/*smart gamma param*/
	void *atm_param;
	void *ygamma_param;/*gamma curve param*/
	void *smart_ygamma_param;/*smart gamma param*/
	/*otp information*/
	void *otp_param;/*otp information*/
};

struct ae_lib_init_out {
	cmr_u16 major_id;
	cmr_u16 minor_id;
	struct ae_control_timing_param ctrl_timing_param;
	struct ae_ev_setting_param ev_setting;/*init ev setting*/
	struct ae_monitor_cfg aem_cfg;/*aem cfg from ae tuning param*/
	struct ae_bayer_hist_cfg bhist_cfg;/*bayer hist config from tuning*/
	struct ae_flash_timing_param flash_timing_param;
	struct ae_gamma_param gamma_param;	
	struct ae_range fps_range;/*the fps range of ae table*/
	struct ae_thd_param thrd_param[AE_LIB_SCENE_MAX];/*0: auto flash; 1: auto 3DNR, 2: auto fps adjust in video mode*/
	struct ae_ev_param_table ev_param;
	struct ae_range dc_fps;/*normal dc preview fps, 100x-->1x*/
	struct ae_range dv_fps;/*video fps, 100x-->1x*/
	cmr_u8 lock;				/* default: 0-unlock, 0:unlock 1:lock */
	/*AE profession Setting*/
	cmr_u8 metering_mode;	/*default: center metering; the metering mode*/
	cmr_u32 iso;				/*iso auto*/
	cmr_u8 flicker;			/* 50hz 0 60hz 1 */
	cmr_u8 scene_mode;		/* default: normal: spano sports night */
	cmr_u8 ae_mode;			/*ae mode*/
	float evd_val;			/*ev value*/
};

struct ae_adv_param {
	union{
		struct ae_stats_data_type stats_data_high;/*the high resolution aem stats data*/
		struct ae_monitor_data_type stats_data_adv;/*will switch to struct ae_monitor_data_type soon*/
	}data;
	/*Histogram Data*/
	struct ae_hist_data_type hist_data;/*yuv histogram data in yuv domain*/
	//struct ae_rect hist_roi;
	struct ae_hist_win_info hist_roi;/*raw hist roi and other info*/
	struct ae_hist_data_type bhist_data[3];/*raw rgb histogram data in raw bayer domain*/
	/*ASD: advanced Scene Detection: based on AI*/
	struct ai_scene_detect detect_scene;
	/*Touch AE ROI*/
	cmr_u8 touch_roi_flag;
	struct ae_trim touch_roi;	/*touch ROI*/
	/*Face ROI*/
	struct ae_face_param face_data;
	/*AE Normal Parameters*/
	struct ae_range sensor_fps_range;/*the limitation fps setting of sensor*/
	struct ae_range fps_range;/*the fps setting from APP*/
	struct ae_ev_setting_param cur_ev_setting;/*the ev setting of the current frame*/
	struct ae_mode_param mode_param;
	cmr_u8 lock;				/* 0:unlock 1:lock */	
	cmr_u8 work_mode;		/* DC DV */
	cmr_u8 high_res_mode;  /*high_res_mode*/
	cmr_u8 flash;				/*flash */
	cmr_u8 flash_mode;		/*flash mode:0:off;1:on;2:torch;3:auto;*/
	cmr_u8 awb_mode;		/*auto or manual mode*/
	cmr_u8 orig_direct;		/*the phone original direction*/

	/*AE profession Setting*/
	cmr_u8 metering_mode;	/*the metering mode*/
	cmr_u32 iso;
	cmr_u8 iso_data_type;
	cmr_u8 flicker;			/* 50hz 0 60hz 1 */
	cmr_u8 scene_mode;		/* pano sports night */
	cmr_u8 is_faceID;
	struct ae_compensation_param comp_param;

	cmr_u8 af_status;			/*AF trigger info */
	cmr_u8 log_level;
	cmr_u8 is_snapshot;
	cmr_u8 prof_mode;		/*in professional mode*/
	cmr_u8 reserve_case;		/*will be removed, 0: normal mode, 1: just for debug mode, and manual control the exp/gain by APP*/
	cmr_u8 app_force_lock;
	cmr_s16 last_target;
	cmr_u32 face_flag;
	cmr_u8 special_fps_mode;
	cmr_u32 cur_lum;
	void *smart_gamma_param;/*smart out gamma param*/
};

struct ae_lib_calc_in {
	cmr_u32 frm_id;
	/*basic information*/
	cmr_u32 ref_camera_id;
	cmr_u32 cam_id;
	cmr_u32 is_multi_mode;
	struct ae_size img_size;/*image resolution*/
	struct ae_rect bhist_size;
	/*AE Stats Data*/
	float fno;/*F-Number*/
	struct ae_rect aem_roi;
	struct ae_rect zoom_roi;
	float zoom_ratio;
	struct ae_stats_data_type stats_data_basic;/*the normal aem stats data*/
	/*will switch to struct ae_monitor_data_type soon,now low resolution still used the old struct*/
	//struct ae_stats_data_type stats_data_basicv2;//under-exposure,normal-exposure,over-exposure
	/*AWB gain*/
	struct ae_alg_rgb_gain awb_gain;
	/*Aux Sensor Data*/
	struct ae_stats_sensor_info aux_sensor_data;
	/*advanced information*/
	struct ae_adv_param adv_param;
	/*debug info*/
	struct ae_debug_info debug_info;
	/*ATM crtl*/
	cmr_u32 atm_lock;
};

struct ae_lib_calc_out  {
	cmr_u32 frame_id;
	cmr_u32 stable;
	cmr_u32 face_stable;
	cmr_u32 face_enable;
	cmr_u32 face_luma;
	cmr_u32 near_stable;
	cmr_s32 cur_bv;
	cmr_s32 cur_bv_nonmatch;
	cmr_u16 cur_lum;			/*the lum of image:0 ~255 */
	cmr_u16 cur_lum_avg;	/*the lum without weight of image:0 ~255*/
	cmr_u16 target_lum;
	cmr_u16 base_target_lum;		//no face AE target luma
	cmr_u16 stab_zone_in;
	cmr_u16 stab_zone_out;
	cmr_u32 flash_status;
	cmr_s8 touch_status;
	float cur_fps;				/*current fps:1~120 */
	cmr_u16 abl_confidence;
	cmr_s32 evd_value;
	struct ae_ev_setting_param ev_setting;
	//struct ae_rgbgamma_curve gamma_curve;/*will be used in future*/
	//struct ae_ygamma_curve ygamma_curve;/*will be used in future*/
	cmr_u32 atmenable_for_crtl;/*add by jhin for atm enable*/
	struct ae_gamma_param gamma_param;
	/*AEM ROI setting*/
	struct ae_point_type aem_roi_st;
	struct ae_size aem_blk_size;
	/*Bayer Hist ROI setting*/

	struct ae_rect adjust_hist_roi;

	/*APEX parameters*/
	float bv;
	float av;
	float tv;
	float sv;
	/*mlog information(LCD display)*/
	void *log_buf;
	cmr_u32 log_len;
	/*debug information(JPG Exif)*/
	void *debug_info;
	cmr_u32 debug_len;
	/*privated information*/
	cmr_u32 privated_data;
	cmr_u32 face_flag;		/*face status flag*/
	cmr_u32 cvg_skip_flag;
};

struct ae_alg_id_info {
	cmr_u16 major_id;
	cmr_u16 minor_id;
};

struct ae_scene_param_in {
	enum ae_scene_mode scene_mod;
	enum ae_iso_mode iso_mod;
	enum ae_flicker_mode flicker_mod;
};

struct ae_scene_param_out {
	cmr_u32 target_lum;
	cmr_u16 min_index;
	cmr_u16 max_index;
	cmr_u16 min_exp;
	cmr_u16 min_gain;
	cmr_u32 max_exp;
	cmr_u16 max_gain;

	cmr_u16 def_index;
	cmr_u16 def_expline;
	cmr_u16 def_gain;
};

struct ae_otp_info {
	cmr_u64 gain_1x_exp;
	cmr_u64 gain_2x_exp;
	cmr_u64 gain_4x_exp;
	cmr_u64 gain_8x_exp;
	cmr_u16 ae_target_lum;
	cmr_u16 reserve;
};

struct ae_frm_sync_param {
	/*ae ev setting limitation*/
	cmr_u32 cam_id;
	cmr_u32 is_benchmark;
	cmr_s16 min_exp_line;
	cmr_s16 max_exp_line;
	cmr_s16 min_gain;
	cmr_s16 max_gain;
	cmr_u32 sensor_gain_precision;
	cmr_u32 frm_len_def;
	/*the OTP information*/
	struct ae_otp_info otp_info;
	/*for Hybrid Zoom*/
	struct ae_rect zoom_roi;
	cmr_u32 zoom_ratio;
	/*AWB gain*/
	struct ae_alg_rgb_gain awb_gain;
	/*the AEM statistic data, and other information*/
	cmr_u32 aem[3 * 1024];/*aem statistics data*/
	struct ae_rect aem_roi_rect;/*AEM Statistic data ROI*/
	struct ae_size img_size;/*the resolution of AEM*/
	struct ae_size blks_num;
	struct ae_size blk_size;
	struct ae_trim block_rect;
	cmr_u64 monoboottime;
	/*the effect ev setting of AEM stats data*/
	struct ae_ev_setting_param effect_param;
	/*the EV setting, that come from the ae lib*/
	struct ae_ev_setting_param ev_setting;
	cmr_u32 hist_data[AEC_HIST_BIN_MAX];
};

struct ae_lib_frm_sync_in{//ae_dynamic_sync struct
	cmr_u32 mode;/*0 boken 1 ae sync*/
	cmr_u32 num;
	cmr_u32 ae_sync_type; /*0: fix mapping 1:dynamic mapping*/
	cmr_u32 bmk_cam_id;
	cmr_u32 tar_cam_id;
	struct ae_frm_sync_param* sync_param[4];/*0:nor 1:rear 2:wide 3:tele*/
};

struct ae_lib_frm_sync_out {
	cmr_u32 num;
	cmr_u32 sync_stable;
	struct ae_ev_setting_param ev_setting[4];/*it follow the input's order*/
};

AE_PUBLIC cmr_handle ae_lib_init(struct ae_lib_init_in *in_param, struct ae_lib_init_out *out_param);
AE_PUBLIC cmr_s32 ae_lib_calculation(cmr_handle handle, struct ae_lib_calc_in *in_param, struct ae_lib_calc_out *out_param);
AE_PUBLIC cmr_s32 ae_lib_ioctrl(cmr_handle handle, cmr_u32 cmd, cmr_handle in_param, cmr_handle out_param);
//cmr_s32 ae_set_param(cmr_handle handle, cmr_u32 cmd, cmr_handle in_param, cmr_handle out_param);
//cmr_s32 ae_get_param(cmr_handle handle, cmr_u32 cmd, cmr_handle in_param, cmr_handle out_param);
AE_PUBLIC cmr_s32 ae_lib_frame_sync_calculation(cmr_handle handle, void *in_param, void *out_param);
AE_PUBLIC cmr_s32 ae_lib_deinit(cmr_handle handle, cmr_handle in_param, cmr_handle out_param);
#ifdef __cplusplus
}
#endif

#endif
