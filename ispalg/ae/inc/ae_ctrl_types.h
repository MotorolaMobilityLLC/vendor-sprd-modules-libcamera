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

#ifndef _AE_CTRL_TYPES_H_
#define _AE_CTRL_TYPES_H_

#include "ae_correction.h"
#include "ae_ctrl_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AE_MAX_PARAM_NUM 1
#define AE_TBL_MAX_INDEX 256
#define AE_FD_NUM 20
#define AE_FLASH_MAX_CELL	40

	enum ae_level {
		AE_LEVEL0 = 0x00,
		AE_LEVEL1,
		AE_LEVEL2,
		Ae_LEVEL3,
		AE_LEVEL4,
		AE_LEVEL5,
		AE_LEVEL6,
		AE_LEVEL7,
		AE_LEVEL8,
		AE_LEVEL9,
		AE_LEVEL10,
		AE_LEVEL11,
		AE_LEVEL12,
		AE_LEVEL13,
		AE_LEVEL14,
		AE_LEVEL15,
		AE_LEVEL_MAX,
		AE_LEVEL_AUTO
	};

	enum ae_weight_mode {
		AE_WEIGHT_AVG = 0x00,
		AE_WEIGHT_CENTER,
		AE_WEIGHT_SPOT,
		AE_WEIGHT_MAX
	};

	enum ae_io_ctrl_cmd {
		/*
		 * warning if you wanna send async msg
		 * please add msg id below here
		 */
		AE_SYNC_MSG_BEGIN = 0x00,
		AE_SET_G_STAT,
		AE_SET_FORCE_PAUSE,		//for mp tool, not change by af or other
		AE_SET_FORCE_RESTORE,	//for mp tool, not change by af or other
		AE_SET_TARGET_LUM,
		AE_SET_SNAPSHOT_NOTICE,
		AE_SET_ONLINE_CTRL,
		AE_SET_FD_PARAM,
		AE_SET_FLASH_ON_OFF_THR,
		AE_SET_NIGHT_MODE,
		AE_SET_FORCE_QUICK_MODE,
		AE_SET_MANUAL_MODE,
		AE_SET_MODE_PARAMS,
		AE_SET_EXP_TIME,
		AE_SET_SENSITIVITY,
		AE_SET_DC_DV,
		AE_VIDEO_STOP,
		AE_VIDEO_START,
		AE_HDR_START,
		AE_CAP_EV_ADJUST_START,
		AE_CAF_LOCKAE_START,
		AE_CAF_LOCKAE_STOP,
		AE_SET_BYPASS,
		AE_SET_FLICKER,
		AE_SET_SCENE_MODE,
		AE_SET_ISO,
		AE_SET_MANUAL_ISO,
		AE_SET_FPS,
		AE_SET_EV_OFFSET,
		AE_SET_WEIGHT,
		AE_SET_STAT_TRIM,
		AE_SET_INDEX,
		AE_SET_EXP_GAIN,
		AE_SET_CAP_EXP,
		AE_SET_PAUSE,
		AE_SET_RESTORE,
		AE_SET_FLASH_NOTICE,
		AE_CTRL_SET_FLASH_MODE,
		AE_SET_RGB_GAIN,
		AE_SET_UPDATE_AUX_SENSOR,
		AE_SET_EXPOSURE_COMPENSATION,
		AE_SET_AUTO_HDR,
		AE_SET_CAP_FLAG,
		AE_SET_SCENE_INFO,
		AE_SET_APP_MODE,
		AE_SET_TOUCH_ZONE,
		AE_SET_3DNR_MODE,
		AE_SET_3DNR_THR,
		AE_SET_AF_STATUS,
		AE_FDR_START,
		AE_SET_AUTO_FDR,
		AE_SET_PROF_MODE,
		AE_SYNC_MSG_END,
		/*
		 * warning if you wanna set ioctrl directly
		 * please add msg id below here
		 */
		AE_DIRECT_MSG_BEGIN,
		AE_GET_LUM,
		AE_GET_INDEX,
		AE_GET_EXP_GAIN,
		AE_GET_ISO,
		AE_GET_EV,
		AE_GET_FPS,
		AE_GET_STAB,
		AE_GET_FLASH_EFFECT,
		AE_GET_AE_STATE,
		AE_GET_FLASH_EB,
		AE_GET_BV_BY_LUM,
		AE_GET_BV_BY_GAIN,
		AE_GET_BV_BY_LUM_NEW,
		AE_GET_BV_BY_GAIN_NEW,
		AE_GET_LOWLIGHT_FLAG_BY_BV,
		AE_GET_FLASH_ENV_RATIO,
		AE_GET_FLASH_ONE_OF_ALL_RATIO,
		AE_GET_MONITOR_INFO,
		AE_GET_FLICKER_MODE,
		AE_GET_FLICKER_SWITCH_FLAG,
		AE_GET_EXP,
		AE_GET_GAIN,
		AE_GET_CUR_WEIGHT,
		AE_GET_SKIP_FRAME_NUM,
		AE_GET_AIS_HANDLE,
		AE_GET_EXP_TIME,		//only for raw picture filename:0.1us
		AE_GET_SENSITIVITY,
		AE_GET_EM_PARAM,
		AE_GET_DEBUG_INFO,
		AE_GET_FLASH_WB_GAIN,
		AE_GET_LEDS_CTRL,
		AE_GET_CALC_RESULTS,
		AE_GET_GLB_GAIN,
		AE_GET_FLASH_SKIP_FRAME_NUM,
		AE_GET_APP_MODE,
		AE_GET_DC_DV_FPS_RANGE,
		AE_DIRECT_MSG_END,
		AE_IO_MAX
	};

	enum ae_work_mode {
		AE_WORK_MODE_COMMON,
		AE_WORK_MODE_CAPTURE,
		AE_WORK_MODE_VIDEO,
		AE_WORK_MODE_MAX
	};

	enum ae_cb_type {
		AE_CB_CONVERGED,
		AE_CB_FLASHING_CONVERGED,
		AE_CB_QUICKMODE_DOWN,
		AE_CB_STAB_NOTIFY,
		AE_CB_AE_LOCK_NOTIFY,
		AE_CB_AE_UNLOCK_NOTIFY,
		AE_CB_TOUCH_AE_NOTIFY,	//for touch ae,notify
		AE_CB_RECOVER_GAIN,
		AE_CB_PREFLASH_PERIOD_END,
		AE_CB_CLOSE_MAIN_FLASH,
		AE_CB_HDR_START,
		AE_CB_HDR_STATUS,
		AE_CB_HDR_EXP_GAIN,
		AE_CB_HDR_TUNING_PARAM_INDEX,
		AE_CB_EV_ADJUST_NOTIFY,
		AE_CB_EV_ADJUST_PARMA,
		AE_CB_LED_NOTIFY,
		AE_CB_FLASH_FIRED,
		AE_CB_PROCESS_OUT,
		AE_CB_AE_CALCOUT_NOTIFY,	//for Binding frame and calc ae dt
		AE_CB_EXPTIME_NOTIFY,
		AE_CB_HDR_STATIS_SIZE,
		AE_CB_3DNR_NOTIFY,
		AE_CB_FDR_START,
		AE_CB_FDR_STATUS,
		AE_CB_PROCESS_RESULT,
		AE_CB_MAX
	};
	enum ae_cb_result_notify_hal {
		AE_CB_RESULT_STAB,
		AE_CB_RESULT_BLS_VALUE,
		AE_CB_RESULT_BLS_ENABLE,
		AE_CB_RESULT_BV_VALUE,
		AE_CB_RESULT_FACA_LUM,
		AE_CB_RESULT_MAX
	};

	enum ae_flash_mode {
		AE_FLASH_PRE_BEFORE,
		AE_FLASH_PRE_LIGHTING,
		AE_FLASH_PRE_AFTER,
		AE_FLASH_MAIN_BEFORE,
		AE_FLASH_MAIN_LIGHTING,
		AE_FLASH_MAIN_AE_MEASURE,
		AE_FLASH_MAIN_AFTER,
		AE_FLASH_MAIN_CLOSE,
		AE_FLASH_AF_DONE,
		AE_LCD_FLASH_PRE_START,
		AE_LCD_FLASH_PRE_END,
		AE_LED_FLASH_ON,
		AE_LED_FLASH_OFF,
		AE_LED_FLASH_AUTO,
		AE_FLASH_MODE_MAX
	};

	enum ae_statistics_mode {
		AE_STATISTICS_MODE_SINGLE,
		AE_STATISTICS_MODE_CONTINUE,
		AE_STATISTICS_MODE_MAX
	};

	enum ae_bayer_hist_mode {
		AE_BAYER_HIST_MODE_SINGLE,
		AE_BAYER_HIST_MODE_CONTINUE,
		AE_BAYER_HIST_MODE_MAX
	};

	enum ae_aem_fmt {
		AE_AEM_FMT_RGB = 0x0001,
		AE_AEM_FMT_YIQ = 0x0002,
		AE_AEM_FMT_BINNING = 0x0004,
		AE_AEM_FMT_MAX
	};

	enum ae_flash_type {
		AE_FLASH_TYPE_PREFLASH,
		AE_FLASH_TYPE_MAIN,
		AE_FLASH_TYPE_MAX
	};

	enum ae_aux_sensor_type {
		AE_ACCELEROMETER,
		AE_MAGNETIC_FIELD,
		AE_GYROSCOPE,
		AE_LIGHT,
		AE_PROXIMITY,
	};

	enum ae_snapshot_tpye {
		AE_SNAPSHOT_NULL = 0,
		AE_SNAPSHOT_DRE,
		AE_SNAPSHOT_GTM,
		AE_SNAPSHOT_NIGHT_DNS,
	} ;

	struct ae_set_fps {
		cmr_u32 min_fps;			// min fps
		cmr_u32 max_fps;			// fix fps flag
	};

	struct ae_gyro_info {
		cmr_u32 valid;
		cmr_s64 timestamp;
		float x;
		float y;
		float z;
	};

	struct ae_gsensor_info {
		cmr_u32 valid;
		cmr_s64 timestamp;
		float vertical_up;
		float vertical_down;
		float horizontal;
	};

	struct ae_aux_sensor_info {
		enum ae_aux_sensor_type type;
		union {
			struct ae_gyro_info gyro_info;
			struct ae_gsensor_info gsensor_info;
		};
	};

	struct ae_set_flicker {
		enum ae_flicker_mode mode;
	};

	struct ae_set_weight {
		enum ae_weight_mode mode;
	};

	struct ae_set_scene {
		enum ae_scene_mode mode;
	};
	
	struct ae_ct_table {
		cmr_s32 ct[20];
		float rg[20];
	};
	
	struct ae_resolution_info {
		struct ae_size frame_size;
		cmr_u32 line_time;
		cmr_u32 frame_line;
		cmr_u32 sensor_size_index;
		cmr_u32 snr_setting_max_fps;
		cmr_u32 binning_factor;
	};

	struct ae_measure_highflash {
		cmr_u32 highflash_flag;
		cmr_u32 capture_skip_num;
	};

	struct ae_sensor_fps_info {
		cmr_u32 mode;			//sensor mode
		cmr_u32 max_fps;
		cmr_u32 min_fps;
		cmr_u32 is_high_fps;
		cmr_u32 high_fps_skip_num;
	};

	struct ae_set_work_param {
		cmr_u16 fly_eb;
		cmr_u16 is_snapshot;
		cmr_u32 shift;
		cmr_u32 dv_mode;
		struct ae_size win_num;/*stats block number*/
		struct ae_size win_size;/*stats block size*/
		struct ae_rect stats_rect;/*stats data ROI*/
		enum ae_work_mode mode;
		struct ae_resolution_info resolution_info;
		struct ae_measure_highflash highflash_measure;
		struct ae_sensor_fps_info sensor_fps;
		struct ae_ct_table ct_table;
		struct ae_opt_info awb_otp_info;
		cmr_u32 capture_skip_num;
		cmr_u32 zsl_flag;
		cmr_u16 binning_factor;
		struct ae_size blk_num;
		cmr_u32 cam_4in1_mode;
		cmr_u32 noramosaic_4in1;
	};

	struct ae_set_iso {
		enum ae_iso_mode mode;
	};

	struct ae_set_pfs {
		cmr_u32 fps;			// min fps
		cmr_u32 fix_fps;		// fix fps flag
	};

	struct ae_set_ev {
		enum ae_level level;
	};

	struct ae_get_ev {
		enum ae_level ev_index;
		cmr_s32 ev_tab[16];
	};

	struct ae_set_tuoch_zone {
		struct ae_trim touch_zone;
	};

	struct ae_set_index {
		cmr_u32 index;
	};

	struct ae_exposure {
		cmr_u32 exposure;
		cmr_u32 dummy;
		cmr_u32 size_index;
		cmr_u32 exp_time;
	};

	struct ae_gain {
		cmr_u32 gain;
	};

	struct ae_exposure_gain {
		cmr_s8 set_mode;		//0: exp&gain   1: table-index
		float exposure;
		cmr_s16 index;
		cmr_s16 expline;
		cmr_s16 dummy;
		cmr_s16 again;
		cmr_s16 dgain;
	};

	struct ae_monitor_info {
		cmr_u32 shift;
		cmr_u32 work_mode;/*single or continue mode*/
		cmr_u32 skip_num;/*skip num: default value is 0*/
		struct ae_size win_size;
		struct ae_size win_num;
		struct ae_trim trim;
		struct ae_rgb_l high_region_thrd;/*it will come from ae algorithm tuning parameter, not from AEM param*/
		struct ae_rgb_l low_region_thrd;/*it will come from ae algorithm tuning parameter, not from AEM param*/
	};

	struct ae_blk_num {
		cmr_u32 x;
		cmr_u32 y;
	};

	struct ae_scene_mode_info {
		enum ae_scene_mode scene_mode;
		struct ae_set_fps fps;
	};

	struct ae_mode_info {
		/*after dcam init, those para will be configered by app */
		cmr_u32 enable;
		cmr_u32 ev_offset;
		cmr_u32 mode;
		cmr_u32 iso;
		cmr_u32 flicker;
		cmr_u32 fix_fps;
		cmr_u32 min_fps;
		cmr_u32 weight;
		cmr_u32 index_default;
	};

	struct ae_normal_info {
		//cmr_u32 gain; TOB
		cmr_u32 exposure;		//unit: s
		cmr_s16 fps;
		cmr_s8 stable;
	};

	struct ae_flash_element {
		cmr_u16 index;
		cmr_u16 val;
		cmr_u16 brightness;
		cmr_u16 color_temp;
		cmr_u32 bg_color;
	};

	struct ae_flash_cell {
		cmr_u8 type;
		cmr_u8 count;
		cmr_u8 def_val;
		struct ae_flash_element element[AE_FLASH_MAX_CELL];
	};

	struct ae_flash_cfg {
		cmr_u32 type;			// enum isp_flash_type
		cmr_u32 led_idx;		//enum isp_flash_led
		cmr_u32 led0_enable;
		cmr_u32 led1_enable;
		cmr_u32 multiColorLcdEn;
	};

	struct ae_awb_gain {
		cmr_u32 r;
		cmr_u32 g;
		cmr_u32 b;
	};

	struct ae_fps_range {
		cmr_u32 dc_fps_min;
		cmr_u32 dc_fps_max;
		cmr_u32 dv_fps_min;
		cmr_u32 dv_fps_max;
	};

	struct ae_stats_monitor_cfg {
		cmr_u32 bypass;
		cmr_u32 skip_num;
		cmr_u32 shift;
		cmr_u32 mode;
		cmr_u32 data_type;
		struct ae_size blk_size;
		struct ae_size blk_num;
		struct ae_trim trim;
		struct ae_rgb_l high_region_thrd;
		struct ae_rgb_l low_region_thrd;
	};

	struct ae_isp_ctrl_ops {
		cmr_handle isp_handler;

		 cmr_s32(*set_exposure) (cmr_handle handler, struct ae_exposure * in_param);
		 cmr_s32(*set_again) (cmr_handle handler, struct ae_gain * in_param);
		 cmr_s32(*set_monitor) (cmr_handle handler, cmr_u32 skip_number);
		 cmr_s32(*set_monitor_win) (cmr_handle handler, struct ae_monitor_info * in_param);
		 cmr_s32(*set_monitor_rgb_thd) (cmr_handle handler, struct ae_monitor_info * in_param);//added by beth for aem
		 cmr_s32(*callback) (cmr_handle handler, cmr_int cb_type, cmr_handle param);
		 cmr_s32(*set_monitor_bypass) (cmr_handle handler, cmr_u32 is_bypass);
		 cmr_s32(*get_system_time) (cmr_handle handler, cmr_u32 * sec, cmr_u32 * usec);
		 cmr_s32(*set_statistics_mode) (cmr_handle handler, enum ae_statistics_mode mode, cmr_u32 skip_number);

		 cmr_s32(*flash_get_charge) (cmr_handle handler, struct ae_flash_cfg * cfg_ptr, struct ae_flash_cell * cell_ptr);
		 cmr_s32(*flash_get_time) (cmr_handle handler, struct ae_flash_cfg * cfg_ptr, struct ae_flash_cell * cell_ptr);
		 cmr_s32(*flash_set_charge) (cmr_handle handler, struct ae_flash_cfg * cfg_ptr, struct ae_flash_element * element_ptr);
		 cmr_s32(*flash_set_time) (cmr_handle handler, struct ae_flash_cfg * cfg_ptr, struct ae_flash_element * element_ptr);
		 cmr_s32(*flash_ctrl) (cmr_handle handler, struct ae_flash_cfg * cfg_ptr, struct ae_flash_element * element_ptr);

		 cmr_s32(*ex_set_exposure) (cmr_handle handler, struct ae_exposure * in_param);
		 cmr_s32(*lcd_set_awb) (cmr_handle handler, cmr_u32 effect);
		 cmr_s32(*set_rgb_gain) (cmr_handle handler, double rgb_gain_coeff);
		 cmr_s32(*set_wbc_gain) (cmr_handle handler, struct ae_alg_rgb_gain * awb_gain);
		 cmr_s32(*set_shutter_gain_delay_info) (cmr_handle handler, cmr_handle param);
		 cmr_int(*write_multi_ae) (cmr_handle handler, void *ae_info);
		 cmr_int(*ae_bokeh_hdr_cb) (cmr_handle handler, void *in_param);
		 cmr_s32(*set_stats_monitor) (cmr_handle handler, struct ae_stats_monitor_cfg * in_param);
		 cmr_s32(*set_blk_num) (cmr_handle handler, struct ae_size *blk_num);
		 cmr_s32(*set_rgb_gain_4in1) (cmr_handle handler, double rgb_gain_coeff);
		 cmr_s32(*set_rgb_gain_slave) (cmr_handle handler, double rgb_gain_coeff);
		 cmr_s32(*set_rgb_gain_slave0) (cmr_handle handler, double rgb_gain_coeff);
		 cmr_s32(*set_rgb_gain_slave1) (cmr_handle handler, double rgb_gain_coeff);
 		 cmr_s32(*set_bayer_hist) (cmr_handle handler, struct ae_bayer_hist_cfg * in_param);
		 cmr_s32(*set_aem_zoom_win) (cmr_handle handler, struct ae_monitor_info * in_param);
	};

	struct ae_stat_img_info {
		cmr_u32 frame_id;
		cmr_s32 index;
		cmr_u32 exposure;
		cmr_u32 again;
		cmr_u32 dgain;
	};

	struct ae_stat_mode {
		cmr_u32 mode;			//0:normal; 1: G width
		cmr_u32 will_capture;
		struct ae_trim trim;
	};

	struct ae_snapshot_notice {
		cmr_u32 type;
		cmr_u32 preview_line_time;
		cmr_u32 capture_line_time;
	};

	enum ae_online_ctrl_mode {
		AE_CTRL_SET_INDEX = 0x00,
		AE_CTRL_SET,
		AE_CTRL_GET,
		AE_CTRL_MODE_MAX
	};

	struct ae_online_ctrl {
		enum ae_online_ctrl_mode mode;
		cmr_u32 index;
		cmr_u32 lum;
		cmr_u32 shutter;
		cmr_u32 dummy;
		cmr_u32 again;
		cmr_u32 dgain;
		cmr_u32 skipa;
		cmr_u32 skipd;
	};

	struct ae_face {
		struct ae_rect rect;
		cmr_u32 face_lum;
		cmr_s32 pose;			/* face pose: frontal, half-profile, full-profile */
		cmr_s32 angle;
		cmr_s32 yaw_angle;
		cmr_s32 roll_angle;
	};

	struct ae_fd_param {
		cmr_u16 width;
		cmr_u16 height;
		cmr_u16 face_num;
		struct ae_face face_area[AE_FD_NUM];
	};
	struct cts_ae_params{
		cmr_u32 exp_time;
		cmr_u32 sensitivity;
		cmr_u32 ae_mode;/*0:auto 1:shutter&&iso 2:shutter first 3:iso first*/
		cmr_s32 frame_number;
	};
	struct ae_hdr_param {
		cmr_u32 hdr_enable;
		cmr_u32 ev_effect_valid_num;
	};

	struct ae_ev_adj_param {
		cmr_u32 enable;
		cmr_u32 ev_effect_valid_num;
		cmr_u32 ev_adjust_cnt;
		enum ae_snapshot_tpye type;
		float ev_value[16];
	};

	struct ae_fdr_param {
		cmr_u32 fdr_enable;
		cmr_u32 ev_effect_valid_num;
		cmr_u32 ev_effect_cnt;
	};

	struct ae_flash_power {
		cmr_s32 max_charge;		//mA
		cmr_s32 max_time;		//ms
	};

	struct ae_flash_notice {
		cmr_u32 mode;			//enum isp_flash_mode
		cmr_u32 will_capture;
		union {
			cmr_u32 flash_ratio;
			struct ae_flash_power power;
		};
		cmr_u32 capture_skip_num;
		cmr_u32 lcd_flash_tune_a;
		cmr_u32 lcd_flash_tune_b;
	};

	struct ae_leds_ctrl {
		cmr_u32 led0_ctrl;
		cmr_u32 led1_ctrl;
	};

	struct ae_ctrl_alc_log {
		cmr_u8 *log;
		cmr_u32 size;
	};

	struct ae_calc_out {
		cmr_u32 cur_lum;
		cmr_u32 cur_index;
		cmr_u32 cur_ev;
		cmr_u32 cur_exp_line;
		cmr_u32 cur_dummy;
		cmr_u32 cur_again;
		cmr_u32 cur_dgain;
		cmr_u32 cur_iso;
		cmr_u32 is_stab;
		cmr_u32 near_stab;
		cmr_u32 line_time;
		cmr_u32 frame_line;
		cmr_u32 target_lum;
		cmr_u32 flag;
		float *ae_data;
		cmr_s32 ae_data_size;
		cmr_u32 target_lum_ori;
		cmr_u32 flag4idx;
		cmr_u32 face_stable;
		cmr_s32 cur_bv;
		float exposure_time;
		cmr_u32 sec;
		cmr_u32 usec;
		cmr_u16 abl_weight;
		cmr_s64 monoboottime;
		struct ae_ctrl_alc_log log_ae;
		cmr_u32 fps;
		cmr_u32 face_enable;
		cmr_u32 face_lum;
		cmr_u32 reserved;
	};

	struct ae_flash_param {
		float captureFlashEnvRatio;
		float captureFlash1ofALLRatio;
	};

	struct ae_calc_results {
		cmr_u32 is_skip_cur_frame;
		struct ae_calc_out ae_output;
		struct ae_get_ev ae_ev;
		struct ae_monitor_info monitor_info;
		struct ae_flash_param flash_param;
	};

	struct ae_binning_stats_info {
		cmr_u32 *r_info;
		cmr_u32 *g_info;
		cmr_u32 *b_info;
		struct ae_size binning_size;
	};

	struct ae_exp_compensation{
		cmr_s32 comp_val;
		struct ae_range comp_range;
		cmr_s32 step_numerator;
		cmr_s32 step_denominator;
	};

#ifdef __cplusplus
}
#endif
#endif
