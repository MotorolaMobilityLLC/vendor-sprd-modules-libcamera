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

#include "ae_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AE_MAX_PARAM_NUM 8
#define AE_TBL_MAX_INDEX 256
#define AE_FD_NUM 20
#define AE_FLASH_MAX_CELL	40

enum ae_write_action {
	AE_WRITE_EXP_GAIN,
	AE_WRITE_RECORD,
	AE_WRITE_NONE
};

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
	AE_LEVEL_MAX
};

enum ae_weight_mode {
	AE_WEIGHT_AVG = 0x00,
	AE_WEIGHT_CENTER,
	AE_WEIGHT_SPOT,
	AE_WEIGHT_MAX
};

enum ae_scene_mode {
	AE_SCENE_NORMAL = 0x00,
	AE_SCENE_NIGHT,
	AE_SCENE_SPORT,
	AE_SCENE_PORTRAIT,
	AE_SCENE_LANDSPACE,
	AE_SCENE_PANORAMA,
	AE_SCENE_MAX
};

enum ae_iso_mode {
	AE_ISO_AUTO = 0x00,
	AE_ISO_100,
	AE_ISO_200,
	AE_ISO_400,
	AE_ISO_800,
	AE_ISO_1600,
	AE_ISO_MAX
};

enum ae_flicker_mode {
	AE_FLICKER_50HZ = 0x00,
	AE_FLICKER_60HZ,
	AE_FLICKER_OFF,
	AE_FLICKER_AUTO,
	AE_FLICKER_MAX
};

enum ae_io_ctrl_cmd {
	AE_GET_LUM = 0x00,
	AE_GET_INDEX,
	AE_GET_EXP_GAIN,
	AE_GET_ISO,
	AE_GET_EV,
	AE_GET_FPS,
	AE_GET_STAB,
	AE_SET_BYPASS,
	AE_SET_FLICKER,
	AE_SET_SCENE_MODE,
	AE_SET_ISO,
	AE_SET_FPS,
	AE_SET_EV_OFFSET,
	AE_SET_WEIGHT,
	AE_SET_STAT_TRIM,
	AE_SET_TOUCH_ZONE,
	AE_SET_INDEX,
	AE_SET_EXP_GAIN,
	AE_SET_CAP_EXP,
	AE_SET_PROC,
	AE_SET_WORK_MODE,
	AE_SET_PAUSE,
	AE_SET_RESTORE,
	AE_SET_FLASH_NOTICE,
	AE_GET_FLASH_EFFECT,
	AE_GET_AE_STATE,
	AE_GET_FLASH_EB,
	AE_SET_TUNING_EB,
	AE_GET_BV_BY_LUM,
	AE_GET_BV_BY_GAIN,
	AE_GET_BV_BY_LUM_NEW,
	AE_GET_BV_BY_GAIN_NEW,
	AE_GET_FLASH_ENV_RATIO,
	AE_GET_FLASH_ONE_OF_ALL_RATIO,
	AE_SET_G_STAT,
	AE_SET_FORCE_PAUSE,	//for mp tool, not change by af or other
	AE_SET_FORCE_RESTORE,	//for mp tool, not change by af or other
	AE_SET_TARGET_LUM,
	AE_SET_SNAPSHOT_NOTICE,
	AE_GET_MONITOR_INFO,
	AE_GET_FLICKER_MODE,
	AE_SET_ONLINE_CTRL,
	AE_SET_EXP_ANIT,
	AE_GET_FLICKER_SWITCH_FLAG,
	AE_SET_FD_ENABLE,
	AE_SET_FD_PARAM,
	AE_GET_EXP,
	AE_GET_GAIN,
	AE_GET_CUR_WEIGHT,
	AE_GET_SKIP_FRAME_NUM,
	AE_SET_FLASH_ON_OFF_THR,
	AE_SET_NIGHT_MODE,
	AE_SET_NORMAL_MODE,
	AE_GET_NORMAL_INFO,
	AE_GET_AIS_HANDLE,
	AE_SET_SPORT_MODE,
	AE_SET_PANORAMA_START,
	AE_SET_PANORAMA_STOP,
	AE_SET_FORCE_QUICK_MODE,
	AE_SET_EXP_TIME,
	AE_SET_SENSITIVITY,
	AE_GET_EXP_TIME,
	AE_GET_SENSITIVITY,
	AE_GET_EM_PARAM,
	AE_GET_DEBUG_INFO,
	AE_INTELLIGENT_OPEN,
	AE_INTELLIGENT_CLOSE,
	AE_SET_DC_DV,
	AE_VIDEO_STOP,
	AE_VIDEO_START,
	AE_GET_AF_INFO,
	AE_HDR_START,
	AE_GET_FLASH_WB_GAIN,
	AE_CAF_LOCKAE_START,
	AE_CAF_LOCKAE_STOP,
	AE_GET_LEDS_CTRL,
	AE_IO_MAX
};

enum ae_work_mode {
	AE_WORK_MODE_COMMON,
	AE_WORK_MODE_CAPTURE,
	AE_WORK_MODE_VIDEO,
	AE_WORK_MODE_MAX
};

enum ae_state {
	AE_STATE_NORMAL,
	AE_STATE_LOCKED,
	AE_STATE_PAUSE,
	AE_STATE_SEARCHING,
	AE_STATE_CONVERGED,
	AE_STATE_FLASH_REQUIRED,
	AE_STATE_PRECAPTURE,
	AE_STATE_INACTIVE,
	AE_STATE_MAX
};

enum ae_cb_type {
	AE_CB_CONVERGED,
	AE_CB_FLASHING_CONVERGED,
	AE_CB_QUICKMODE_DOWN,
	AE_CB_STAB_NOTIFY,
	AE_CB_AE_LOCK_NOTIFY,
	AE_CB_AE_UNLOCK_NOTIFY,
	AE_CB_TOUCH_AE_NOTIFY,	//for touch ae,notify
	AE_CB_CLOSE_PREFLASH,
	AE_CB_PREFLASH_PERIOD_END,
	AE_CB_CLOSE_MAIN_FLASH,
	AE_CB_HDR_START,
	AE_CB_MAX
};

enum ae_flash_mode {
	AE_FLASH_PRE_BEFORE,
	AE_FLASH_PRE_LIGHTING,
	AE_FLASH_PRE_AFTER,
	AE_FLASH_MAIN_BEFORE,
	AE_FLASH_MAIN_LIGHTING,
	AE_FLASH_MAIN_AE_MEASURE,
	AE_FLASH_MAIN_AFTER,
	AE_FLASH_AF_DONE,
	AE_LCD_FLASH_PRE_START,
	AE_LCD_FLASH_PRE_END,
	AE_FLASH_MODE_MAX
};

enum ae_statistics_mode {
	AE_STATISTICS_MODE_SINGLE,
	AE_STATISTICS_MODE_CONTINUE,
	AE_STATISTICS_MODE_MAX
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
	FLASH_MAX
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

struct ae_resolution_info {
	struct ae_size frame_size;
	cmr_u32 line_time;
	cmr_u32 frame_line;
	cmr_u32 sensor_size_index;
	cmr_u32 snr_setting_max_fps;
};

struct ae_measure_highflash {
	cmr_u32 highflash_flag;
	cmr_u32 capture_skip_num;
};

struct ae_sensor_fps_info {
	cmr_u32 mode;	//sensor mode
	cmr_u32 max_fps;
	cmr_u32 min_fps;
	cmr_u32 is_high_fps;
	cmr_u32 high_fps_skip_num;
};

struct ae_set_work_param {
	cmr_u16 fly_eb;
	cmr_u16 is_snapshot;
	cmr_u32 shift;
	struct ae_size win_num;
	struct ae_size win_size;
	enum ae_work_mode mode;
	struct ae_resolution_info resolution_info;
	struct ae_measure_highflash highflash_measure;
	struct ae_sensor_fps_info sensor_fps;
};

struct ae_set_iso {
	enum ae_iso_mode mode;
};

struct ae_set_pfs {
	cmr_u32 fps;	// min fps
	cmr_u32 fix_fps;	// fix fps flag
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
};

struct ae_gain {
	cmr_u32 gain;
};

struct ae_exposure_gain {
	cmr_s8 set_mode;	//0: exp&gain   1: table-index
	float exposure;
	cmr_s16 index;
	cmr_s16 expline;
	cmr_s16 dummy;
	cmr_s16 again;
	cmr_s16 dgain;
};

struct ae_monitor_cfg {
	cmr_u32 skip_num;
};

struct ae_monitor_info {
	cmr_u32 shift;
	struct ae_size win_size;
	struct ae_size win_num;
	struct ae_trim trim;
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
	cmr_u32 exposure;	//unit: s
	cmr_s16 fps;
	cmr_s8 stable;
};

struct ae_flash_element {
	cmr_u16 index;
	cmr_u16 val;
};

struct ae_flash_cell {
	cmr_u8 type;
	cmr_u8 count;
	cmr_u8 def_val;
	struct ae_flash_element element[AE_FLASH_MAX_CELL];
};

struct ae_flash_cfg {
	cmr_u32 type;	// enum isp_flash_type
	cmr_u32 led_idx;	//enum isp_flash_led
	cmr_u32 led0_enable;
	cmr_u32 led1_enable;
};

struct ae_isp_ctrl_ops {
	cmr_handle isp_handler;
	 cmr_s32(*set_exposure) (cmr_handle handler, struct ae_exposure * in_param);
	 cmr_s32(*set_again) (cmr_handle handler, struct ae_gain * in_param);
	 cmr_s32(*set_monitor) (cmr_handle handler, struct ae_monitor_cfg * in_param);
	 cmr_s32(*set_monitor_win) (cmr_handle handler, struct ae_monitor_info * in_param);
	 cmr_s32(*callback) (cmr_handle handler, cmr_u32 cb_type);
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
	 cmr_s32(*set_shutter_gain_delay_info) (cmr_handle handler, cmr_handle param);
};

struct ae_stat_img_info {
	cmr_u32 frame_id;
	cmr_s32 index;
	cmr_u32 exposure;
	cmr_u32 again;
	cmr_u32 dgain;
};

struct tg_ae_ctrl_alc_log {
	cmr_u8 *log;
	cmr_u32 size;
};

struct ae_stat_mode {
	cmr_u32 mode;	//0:normal; 1: G width
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
	cmr_s32 pose;	/* face pose: frontal, half-profile, full-profile */
};

struct ae_fd_param {
	cmr_u16 width;
	cmr_u16 height;
	cmr_u16 face_num;
	struct ae_face face_area[AE_FD_NUM];
};

struct ae_hdr_param {
	cmr_u32 hdr_enable;
	cmr_u32 ev_effect_valid_num;
};

struct ae_flash_power {
	cmr_s32 max_charge;	//mA
	cmr_s32 max_time;	//ms
};

struct ae_flash_notice {
	cmr_u32 mode;	//enum isp_flash_mode
	cmr_u32 will_capture;
	union {
		cmr_u32 flash_ratio;
		struct ae_flash_power power;
	};
	cmr_u32 capture_skip_num;
	cmr_u32 lcd_flash_tune_a;
	cmr_u32 lcd_flash_tune_b;
};

struct ae_awb_gain {
	cmr_u32 r;
	cmr_u32 g;
	cmr_u32 b;
};

struct ae_leds_ctrl {
    cmr_u32 led0_ctrl;
    cmr_u32 led1_ctrl;
};

#ifdef __cplusplus
}
#endif

#endif
