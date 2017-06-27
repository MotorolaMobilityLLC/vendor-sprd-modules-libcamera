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
#ifndef _SPRD_AF_CTRL_V1_H
#define _SPRD_AF_CTRL_V1_H

#include <utils/Timers.h>

#include "AFv1_Common.h"
#include "AFv1_Interface.h"
//#include "AFv1_Tune.h"

#include "aft_interface.h"

#define AF_SYS_VERSION "-20170511-01"
#define AF_SAVE_MLOG_STR "persist.sys.isp.af.mlog"	/*save/no */
#define AF_WAIT_CAF_TIMEOUT 3000000000;	//1s == (1000 * 1000 * 1000)ns

enum afv1_bool {
	AFV1_FALSE = 0,
	AFV1_TRUE,
};

enum afv1_err_type {
	AFV1_SUCCESS = 0x00,
	AFV1_ERROR,
	AFV1_ERR_MAX
};

enum _lock_block {
	LOCK_AE = 0x01,
	LOCK_LSC = 0x02,
	LOCK_NLM = 0x04,
	LOCK_AWB = 0x08,
};

enum af_state {
	STATE_MANUAL = 0,
	STATE_NORMAL_AF,	// single af or touch af
	STATE_CAF,
	STATE_RECORD_CAF,
	STATE_FAF,
	STATE_FULLSCAN,
	STATE_PICTURE,
};

static const char *state_string[] = {
	"manual",
	"normal_af",
	"caf",
	"record caf",
	"faf",
	"fullscan",
	"picture",
};

#define STATE_STRING(state)    state_string[state]

enum focus_state {
	AF_IDLE,
	AF_SEARCHING,
};

static const char *focus_state_str[] = {
	"af idle",
	"af searching",
};

#define FOCUS_STATE_STR(state)    focus_state_str[state]

enum dcam_after_vcm {
	DCAM_AFTER_VCM_NO = 0,
	DCAM_AFTER_VCM_YES
};

typedef struct _isp_info {
	cmr_u32 width;
	cmr_u32 height;
	cmr_u32 win_num;
} isp_info_t;

#define MAX_ROI_NUM     32	// arbitrary, more than hardware wins

typedef struct _roi_rgb_y {
	cmr_s32 num;
	cmr_u32 Y_sum[MAX_ROI_NUM];
	cmr_u32 R_sum[MAX_ROI_NUM];
	cmr_u32 G_sum[MAX_ROI_NUM];
	cmr_u32 B_sum[MAX_ROI_NUM];
} roi_rgb_y_t;

typedef struct _lens_info {
	cmr_u32 pos;
} lens_info_t;

typedef struct _ae_info {
	struct af_ae_calc_out ae_report;
	cmr_u32 win_size;
} ae_info_t;

typedef struct _awb_info {
	cmr_u32 r_gain;
	cmr_u32 g_gain;
	cmr_u32 b_gain;
} awb_info_t;

typedef struct _isp_awb_statistic_hist_info {
	cmr_u32 r_info[1024];
	cmr_u32 g_info[1024];
	cmr_u32 b_info[1024];
	cmr_u32 r_hist[256];
	cmr_u32 g_hist[256];
	cmr_u32 b_hist[256];
	cmr_u32 y_hist[256];
} isp_awb_statistic_hist_info_t;

typedef struct _win_coord {
	cmr_u32 start_x;
	cmr_u32 start_y;
	cmr_u32 end_x;
	cmr_u32 end_y;
} win_coord_t;

typedef struct _roi_info {
	cmr_u32 num;
	win_coord_t win[MAX_ROI_NUM];
} roi_info_t;

enum filter_type {
	FILTER_SOBEL5 = 0,
	FILTER_SOBEL9,
	FILTER_SPSMD,
	FILTER_ENHANCED,
	FILTER_NUM
};

#define SOBEL5_BIT  (1 << FILTER_SOBEL5)
#define SOBEL9_BIT  (1 << FILTER_SOBEL9)
#define SPSMD_BIT   (1 << FILTER_SPSMD)
#define ENHANCED_BIT   (1 << FILTER_ENHANCED)

// caf trigger
typedef struct _caf_trigger_ops {
	aft_proc_handle_t handle;
	struct aft_ae_skip_info ae_skip_info;
	 cmr_s32(*init) (struct aft_tuning_block_param * init_param, aft_proc_handle_t * handle);
	 cmr_s32(*deinit) (aft_proc_handle_t * handle);
	 cmr_s32(*calc) (aft_proc_handle_t * handle, struct aft_proc_calc_param * alg_calc_in, struct aft_proc_result * alg_calc_result);
	 cmr_s32(*ioctrl) (aft_proc_handle_t * handle, enum aft_cmd cmd, void *param0, void *param1);
} caf_trigger_ops_t;

#define CAF_TRIGGER_LIB "libspcaftrigger.so"

typedef struct _ae_cali {
	cmr_u32 r_avg[9];
	cmr_u32 g_avg[9];
	cmr_u32 b_avg[9];
	cmr_u32 r_avg_all;
	cmr_u32 g_avg_all;
	cmr_u32 b_avg_all;
} ae_cali_t;

typedef struct _focus_stat {
	cmr_u32 force_write;
	cmr_u32 reg_param[10];
} focus_stat_reg_t;

typedef struct _af_fv_info {
	cmr_u64 af_fv0[10];	//[10]:10 ROI, sum of FV0
	cmr_u64 af_fv1[10];	//[10]:10 ROI, sum of FV1
} af_fv;

typedef struct _afm_tuning_param_sharkl2 {
	cmr_u8 iir_level;
	cmr_u8 nr_mode;
	cmr_u8 cw_mode;
	cmr_u8 fv0_e;
	cmr_u8 fv1_e;
	cmr_u8 dummy[3];	// 4 bytes align
} afm_tuning_sharkl2;

typedef struct _af_ctrl {
	void *af_alg_cxt;	//AF_Data fv;
	cmr_u32 af_dump_info_len;
	cmr_u32 state;		//enum af_state state;
	cmr_u32 pre_state;	//enum af_state pre_state;
	cmr_u32 focus_state;	//enum caf_state caf_state;
	cmr_u32 algo_mode;	//eAF_MODE algo_mode;
	cmr_u32 takePicture_timeout;
	cmr_u32 request_mode;
	cmr_u32 need_re_trigger;
	cmr_u64 vcm_timestamp;
	cmr_u64 dcam_timestamp;
	cmr_u64 takepic_timestamp;
	cmr_u32 Y_sum_trigger;
	cmr_u32 Y_sum_normalize;
	cmr_u64 fv_combine[T_TOTAL_FILTER_TYPE];
	af_fv af_fv_val;
	struct af_iir_nr_info af_iir_nr;
	struct af_enhanced_module_info af_enhanced_module;
	struct afm_thrd_rgb thrd;
	struct af_gsensor_info gsensor_info;
	struct isp_face_area face_info;
	pthread_mutex_t af_work_lock;
	pthread_mutex_t caf_work_lock;
	sem_t af_wait_caf;
	isp_info_t isp_info;
	lens_info_t lens;
	cmr_s32 flash_on;
	roi_info_t roi;
	roi_rgb_y_t roi_RGBY;
	ae_info_t ae;
	awb_info_t awb;
	pd_algo_result_t pd;
	cmr_s32 ae_lock_num;
	cmr_s32 awb_lock_num;
	cmr_s32 lsc_lock_num;
	cmr_s32 nlm_lock_num;
	cmr_s32 ae_partial_lock_num;
	void *trig_lib;
	caf_trigger_ops_t trig_ops;
	ae_cali_t ae_cali_data;
	cmr_u32 vcm_stable;
	focus_stat_reg_t stat_reg;
	cmr_u32 defocus;
	cmr_u8 bypass;
	cmr_u32 force_trigger;
	//non-zsl,easy for motor moving and capturing
	cmr_u8 test_loop_quit;
	pthread_t test_loop_handle;
	pthread_mutex_t status_lock;
	cmr_handle caller;
	cmr_handle handle_pm;
	cmr_u32 win_peak_pos[MULTI_STATIC_TOTAL];
	cmr_u32 is_high_fps;
	cmr_u32 afm_skip_num;
	afm_tuning_sharkl2 afm_tuning;
	struct aft_proc_calc_param prm_ae;
	struct aft_proc_calc_param prm_af;
	struct aft_proc_calc_param prm_sensor;
	struct aft_proc_calc_param prm_pd;
	struct aft_proc_calc_param prm_fd;
	isp_awb_statistic_hist_info_t rgb_stat;
	cmr_u32 trigger_source_type;
	struct af_ctrl_otp_info otp_info;
	//cmr_s32(*go_position) (void *handle, struct af_motor_pos * in_param);
	 cmr_s32(*end_notice) (void *handle, struct af_result_param * in_param);
	 cmr_s32(*start_notice) (void *handle);
	 cmr_s32(*set_monitor) (void *handle, struct af_monitor_set * in_param, cmr_u32 cur_envi);
	 cmr_s32(*set_monitor_win) (void *handler, struct af_monitor_win * in_param);
	 cmr_s32(*get_monitor_win_num) (void *handler, cmr_u32 * win_num);
	 cmr_s32(*lock_module) (void *handle, cmr_int af_locker_type);
	 cmr_s32(*unlock_module) (void *handle, cmr_int af_locker_type);
	 cmr_u32(*af_lens_move) (void *handle, cmr_u16 pos);
	 cmr_u32(*af_get_motor_pos) (void *handle, cmr_u16 * motor_pos);
	 cmr_u32(*af_get_otp) (void *handle, uint16_t * inf, uint16_t * macro);
	 cmr_u32(*af_set_motor_bestmode) (void *handle);
	 cmr_u32(*af_get_test_vcm_mode) (void *handle);
	 cmr_u32(*af_set_test_vcm_mode) (void *handle, char *vcm_mode);
	 cmr_s32(*af_monitor_bypass) (void *handle, cmr_u32 * bypass);
	 cmr_s32(*af_monitor_skip_num) (void *handle, cmr_u32 * afm_skip_num);
	 cmr_s32(*af_monitor_mode) (void *handle, cmr_u32 * afm_mode);
	 cmr_s32(*af_monitor_iir_nr_cfg) (void *handle, struct af_iir_nr_info * af_iir_nr);
	 cmr_s32(*af_monitor_module_cfg) (void *handle, struct af_enhanced_module_info * af_enhanced_module);
} af_ctrl_t;

typedef struct _test_mode_command {
	char *command;
	cmr_u64 key;
	void (*command_func) (af_ctrl_t * af, char *test_param);
} test_mode_command_t;

cmr_handle sprd_afv1_init(void *in, void *out);
cmr_s32 sprd_afv1_deinit(cmr_handle handle, void *param, void *result);
cmr_s32 sprd_afv1_process(void *handle, void *in, void *out);
cmr_s32 sprd_afv1_ioctrl(void *handle, cmr_s32 cmd, void *param0, void *param1);

#endif
