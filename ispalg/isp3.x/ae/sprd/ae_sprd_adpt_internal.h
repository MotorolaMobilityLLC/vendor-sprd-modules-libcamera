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

#ifndef _AE_SPRD_ADPT_INTERNAL_H_
#define _AE_SPRD_ADPT_INTERNAL_H_
#include "flash.h"
#include "ae_correction.h"
#include "ae_ctrl_types.h"
#include "ae_ctrl.h"
#include "isp_bridge.h"
#ifdef __cplusplus
extern "C" {
#endif

	struct ae_exposure_param {
		cmr_u32 cur_index;
		cmr_u32 line_time;
		cmr_u32 exp_line;
		cmr_u32 exp_time;
		cmr_s32 dummy;
		cmr_s32 frm_len;
		cmr_s32 frm_len_def;
		cmr_u32 gain;			/*gain = sensor_gain * isp_gain */
		cmr_u32 sensor_gain;
		cmr_u32 isp_gain;
		cmr_s32 target_offset;
		cmr_s32 bv;
		cmr_u32 table_idx;
		cmr_u32 is_ev_setting;
		cmr_u32 mode;
	};
	struct ae_ev_param {
		cmr_s16 target_lum;
		cmr_s16 target_lum_zone;
		cmr_s16 target_range_in_zone;
		cmr_s16 target_range_out_zone;
		cmr_s16 stride_config_0;
		cmr_s16 stride_config_1;
		cmr_s8 ev_index;
		cmr_u32 is_mev;
	};

	struct ae_exposure_param_switch {
		cmr_u32 target_offset;
		cmr_u32 exp_line;
		cmr_u32 exp_time;
		cmr_s32 dummy;
		cmr_u32 frm_len;
		cmr_u32 frm_len_def;
		cmr_u32 gain;
		cmr_u32 table_idx;
		cmr_u32 lum;
		cmr_u32 tarlum;
		cmr_u32 sensitivity;
		cmr_s8 ev_index;
	};
	struct ae_exposure_param_switch_m {
		cmr_u32 target_offset;
		cmr_u32 exp_line;
		cmr_u32 exp_time;
		cmr_s32 dummy;
		cmr_u32 frm_len;
		cmr_u32 frm_len_def;
		cmr_u32 gain;
		cmr_u32 table_idx;
		cmr_u32 manual_level;
	};

	struct ae_sensor_exp_data {
		struct ae_exposure_param lib_data;	/*AE lib output data */
		struct ae_exposure_param actual_data;	/*the actual effect data */
		struct ae_exposure_param write_data;	/*write to sensor data in current time */
	};

	enum initialization {
		AE_PARAM_NON_INIT,
		AE_PARAM_INIT
	};

	enum manual_mode_ev_status {
		AE_MANUAL_EV_INIT = 0xf0,
		AE_MANUAL_EV_SET
	};

	enum {
		AE_3DNR_ON,
		AE_3DNR_OFF,
		AE_3DNR_AUTO,
		AE_3DNR_MAX,
	};

	struct flash_cali_data {
		cmr_u16 ydata;			//1024
		cmr_u16 rdata;			// base on gdata1024
		cmr_u16 bdata;			// base on gdata1024
		cmr_s8 used;
	};

	struct ae_exposure_compensation {
		cmr_u16 ae_base_idx;
		cmr_u32 step_numerator;
		cmr_u32 step_denominator;
		cmr_s16 comp_val;
		cmr_u8 ae_compensation_flag;
		float ae_ev_value;
		cmr_u16 ae_base_target;
		cmr_u32 touch_ev_flag;
	};

	struct hdr_calc_result {
		float ev[2];
		cmr_s8 auto_hdr_enable;
	};

/**************************************************************************/
/*
* BEGIN: FDAE related definitions
*/
	struct ae_fd_info {
		cmr_s8 enable;
		cmr_s8 pause;
		cmr_s8 allow_to_work;	/* When allow_to_work==0, FDAE
								 * will never work */
		struct ae_fd_param face_info;	/* The current face information */
	};

/**************************************************************************/
/*
* ae handler for isp_app
*/
	struct ae_ctrl_cxt {
		cmr_u32 start_id;
		cmr_u16 major_id;
		cmr_u16 minor_id;
		cmr_u32 checksum;
		cmr_u32 bypass;
		cmr_u32 capture_skip_num;
		struct ae_flash_timing_param flash_timing_param;
		cmr_u32 zsl_flag;
		cmr_u32 skip_update_param_flag;
		cmr_u32 skip_update_param_cnt;
		cmr_u32 delay_cnt;
		cmr_u32 backup_rgb_gain;
		double ob_rgb_gain;
		pthread_mutex_t data_sync_lock;
		/*
		 * camera id: front camera or rear camera
		 */
		cmr_u32 multiColorLcdEn;
		cmr_s8 camera_id;
		cmr_s8 is_snapshot;
		cmr_u8 is_first;
		cmr_u8 is_ev_setting;
		/*
		 * ae control operation infaces
		 */
		struct ae_isp_ctrl_ops isp_ops;
		/*
		 * ae stat monitor config
		 */
		//struct ae_monitor_unit monitor_unit;
		/*
		 * ae slow motion info
		 */
		struct ae_sensor_fps_info high_fps_info;
		/*
		 * ae fps range
		 */
		struct ae_range fps_range;
		struct ae_range dv_fps_range;
		struct ae_range dc_fps_range;
		/*
		 * current flicker flag
		 */
		cmr_u32 cur_flicker;
		/*
		 * for ae tuning parameters
		 */
		struct ae_stats_monitor_cfg monitor_cfg;
		struct ae_bayer_hist_cfg bhist_cfg;
		struct flash_tune_param dflash_param[AE_MAX_PARAM_NUM];
		/*
		 * sensor related information
		 */
		struct ae_resolution_info snr_info;
		/*
		 * ae current status: include some tuning
		 * param/calculatioin result and so on
		 */
		struct ae_lib_calc_in prv_status;	/*just backup the alg status of normal scene,
												   as switch from special scene mode to normal,
												   and we use is to recover the algorithm status
												 */
		struct ae_lib_calc_in cur_status;
		struct ae_lib_calc_in sync_cur_status;
		cmr_s32 target_lum_zone_bak;
		struct tar_lum_range tar_lum_rang;//added by feifan.wang
		//cmr_u32 target_lum_range_in_bak;
		//cmr_u32 target_lum_range_out_bak;
		/*
		 * convergence & stable zone
		 */
		cmr_s8 stable_zone_ev[16];
		/* just for 4in1 featue */
		cmr_s16 bv_thd;
		/*
		 * flash ae param
		 */
		/*ST: for dual flash algorithm */
		struct ae_thd_param flash_thrd;
		cmr_u8 flash_ver;
		cmr_u8 flash_fired;
		cmr_s32 pre_flash_skip;
		cmr_s32 aem_effect_delay;
		struct ae_leds_ctrl ae_leds_ctrl;
		cmr_handle flash_alg_handle;
		cmr_u16 aem_stat_rgb[3 * 1024];	/*save the average data of AEM Stats data */
		cmr_u32 sync_aem[3 * 1024 + 4];	/*low resolution aem statistics data, 0: frame id;1: exposure time, 2: dummy line, 3: gain; */
		cmr_u32 sync_aem_high[3 * 128 * 128];	/*high resolution statistics data */
		struct ae_size binning_stat_size;
		cmr_u8 bakup_ae_status_for_flash;	/* 0:unlock 1:lock 2:pause 3:wait-lock */
		cmr_s16 pre_flash_level1;
		cmr_s16 pre_flash_level2;
		struct Flash_pfOneIterationInput flash_esti_input;
		struct Flash_pfOneIterationOutput flash_esti_result;
		struct Flash_mfCalcInput flash_main_esti_input;
		struct Flash_mfCalcOutput flash_main_esti_result;
		cmr_s32 flash_last_exp_line;
		cmr_s32 flash_last_gain;
		float ctTabRg[20];
		float ctTab[20];
		struct ae_opt_info awb_otp_info;
		/*ED: for dual flash algorithm */
		cmr_s16 flash_on_off_thr;
		cmr_u32 flash_effect;
		struct ae_exposure_param flash_backup;
		struct flash_cali_data flash_cali[32][32];
		cmr_u8 flash_debug_buf[256 * 1024];
		cmr_u32 flash_buf_len;
		cmr_u8 flash_cap_proc;
		/*
		 * fd-ae param
		 */
		struct ae_fd_info fdae;
		/*
		 * control information for sensor update
		 */
		struct ae_lib_calc_out cur_result;
		struct ae_lib_calc_out sync_cur_result;
		struct ae_calc_results calc_results;	/*ae current calculation results, and it is just for other algorithm block */
		/*
		 * AE write/effective E&G queue
		 */
		cmr_handle seq_handle;
		cmr_handle seq_handle_s0;
		cmr_handle seq_handle_s1;
		cmr_s8 exp_skip_num;
		cmr_s8 gain_skip_num;
		cmr_s16 sensor_gain_precision;
		cmr_u16 sensor_max_gain;	/*the max gain that sensor can be used */
		cmr_u16 sensor_min_gain;	/*the mini gain that sensor can be used */
		cmr_u16 min_exp_line;	/*the mini exposure line that sensor can be used */
		struct ae_sensor_exp_data exp_data;
		/*
		 * recording when video stop
		 */
		struct ae_exposure_param last_exp_param;
		cmr_s32 last_index;
		cmr_s32 last_enable;

		struct ae_exposure_param backup_touch; /* for backup current param when touch ae*/
		/*
		 * just for debug information
		 */
		cmr_u8 debug_enable;
		char debug_file_name[256];
		cmr_handle debug_info_handle;
		cmr_u32 debug_str[512];
		cmr_u8 debug_info_buf[512 * 1024];
		//struct debug_ae_param debug_buf;
		/*
		 * for manual ae stat
		 */
		cmr_u8 manual_ae_on;
		/*
		 *Save exposure & iso value on manual ae mode
		 */
		cmr_u32 manual_exp_time;
		cmr_u32 manual_exp_line_bkup;
		cmr_u32 manual_iso_value;
		cmr_u8 manual_ae_mode;
		/*
		 * flash_callback control
		 */
		cmr_s8 send_once[6];
		/*
		 * HDR control
		 */
		cmr_s8 hdr_enable;
		cmr_s8 hdr_frame_cnt;
		cmr_s8 hdr_cb_cnt;
		cmr_s8 hdr_flag;
		cmr_s16 hdr_up;
		cmr_s16 hdr_down;
		cmr_s16 hdr_base_ae_idx;
		cmr_u16 hdr_exp_line;
		cmr_s16 hdr_gain;
		cmr_u8 hdr_version;
		cmr_u8 hdr_menu_ctrl; /* auto: 1*/
		struct hdr_calc_result hdr_calc_result;
		cmr_s32 smooth_flag;
		cmr_s32 frameid;
		/*
		 * DRE control
		 */
		cmr_s8 dre_enable;
		cmr_u16 dre_exp_line;
		cmr_s16 dre_gain;
		cmr_s8 dre_frame_cnt;
		cmr_s8 threednr_mode_flag;
		/*
		 *dual flash simulation
		 */
		cmr_s8 led_record[2];
		/*
		 * ae misc layer handle
		 */
		cmr_handle misc_handle;
		/*
		 * for dual camera sync
		 */
		cmr_u8 is_master;
		cmr_u32 is_multi_mode;
		cmr_u8 sensor_role;
		func_isp_br_ioctrl ptr_isp_br_ioctrl;
		/*
		 * for binning facter = 2
		 */
		cmr_u16 binning_factor_cap;
		cmr_u16 binning_factor_prev;
		struct ae_exposure_compensation exposure_compensation;
		cmr_u32 slw_prev_skip_num;

		/*
		 * add flags for stopping skip frame during main flash after
		 **/
		cmr_u32 has_mf;
		cmr_u32 has_mf_cnt;
		cmr_u32 glb_gain;
		cmr_u32 ebd_support;
		cmr_u32 ebd_stable_flag;
		cmr_u32 ebd_flash_stable_flag;

		/* 4in1 param */
		cmr_u32 cam_4in1_mode;
		cmr_u32 cam_cap_flag;
		cmr_u32 cam_large_pix_num;
		cmr_u32 is_fourcell;

		/*for ev and AE/AE lock*/
		cmr_u32 app_mode;
		cmr_u32 manual_level;
		cmr_u32 munaul_iso_index;
		cmr_u32 last_cam_mode;
		cmr_u32 last_cur_lum;
		cmr_u32 last_table_index; /* for non-zsl, there is a 'ae_set_video_start' between pf and mf.which we need keep the previous table index */
		cmr_u32 app_mode_tarlum[64];
		cmr_u32 pf_wait_stable_cnt;
		cmr_u32 effect_index_index;
		cmr_u32 effect_index[4];
		cmr_u32 pf_with_touch;
		/*for debug*/
		cmr_s8 history_param[8 * 1024];
		cmr_s16 ae_comp_value;
		cmr_u16 flash_backup_aebaseidx;
		/* backup for ev*/
		struct ae_ev_param flash_ev_backup;
		cmr_u32 is_faceId_unlock;
		cmr_u32 face_lock_table_index;
		/* aelock, env change detection */
		cmr_u32 env_cum_changedCalc_delay_cnt;
		cmr_u32 env_cum_changed;
		cmr_s16 previous_lum;
		cmr_u32 pri_set;
		/*for callback to HAL*/
		cmr_u32 ae_cb_result[10];

		struct ae_scene_param_out ae_tbl_param;
		cmr_u32 pause_cnt;
		cmr_u32 force_lock_ae;
		cmr_u32 cur_param_target_lum;
		struct ae_ev_param_table ev_param_table;
		struct ae_thd_param ae_video_fps;
		struct ae_thd_param threednr_thrd;
		struct ae_thd_param cam_4in1_switch_thrd;
		cmr_u32 threednr_mode;
		cmr_u8 led_state;

		cmr_u32 *master_aem_stat;
		cmr_u32 *slave0_aem_stat;
		cmr_u32 *slave1_aem_stat;
		
		cmr_u32 *tune_buf;
		struct ae_frm_sync_param sync_param[4];
		struct ae_exposure_param_switch mode_switch[64];
		cmr_u8 mainFlashEn;
		cmr_u8 calcFirstFlag;
		cmr_u32 end_id;

	};
#ifdef __cplusplus
}
#endif
#endif
