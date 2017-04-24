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

#include "isp_dev_access.h"

#include "AFv1_Common.h"
#include "AFv1_Interface.h"
#include "AFv1_Tune.h"

#include "aft_interface.h"

#define AF_SYS_VERSION "-20170225-02"
#define AF_SAVE_MLOG_STR "persist.sys.isp.af.mlog"	/*save/no */
#define AF_WAIT_CAF_TIMEOUT 200000000;	//1s == (1000 * 1000 * 1000)ns

#define BOKEH_BOUNDARY_RATIO 8	//based on 10
#define BOKEH_SCAN_FROM 212	//limited in [0,1023]
#define BOKEH_SCAN_TO 342	//limited in [0,1023]
#define BOKEH_SCAN_STEP 7	//at least 20

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
	LOCK_NLM = 0x04
};

enum af_state {
	STATE_INACTIVE = 0,
	STATE_IDLE,		// isp is ready, waiting for cmd
	STATE_NORMAL_AF,	// single af or touch af
	STATE_CAF,
	STATE_RECORD_CAF,
	STATE_FAF,
	STATE_FULLSCAN
};

static const char *state_string[] = {
	"inactive",
	"idle",
	"normal_af",
	"caf",
	"record caf",
	"faf",
	"fullscan"
};

#define STATE_STRING(state)    state_string[state]

enum caf_state {
	CAF_IDLE,
	CAF_MONITORING,
	CAF_SEARCHING
};

static const char *caf_state_str[] = {
	"caf idle",
	"caf monitoring",
	"caf searching"
};

enum dcam_after_vcm {
	DCAM_AFTER_VCM_NO = 0,
	DCAM_AFTER_VCM_YES
};

#define CAF_STATE_STR(state)    caf_state_str[state]

static char AF_MODE[300] = { '\0' };
static char AF_POS[100] = { '\0' };

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
	cmr_u32 stable;
	cmr_u32 bv;
	cmr_u32 exp;		// 0.1us
	cmr_u32 gain;
	cmr_u32 gain_index;
	cmr_u32 win_size;
	struct ae_calc_out ae;
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

enum AF_AE_GAIN {
	GAIN_1x = 0,
	GAIN_2x,
	GAIN_4x,
	GAIN_8x,
	GAIN_16x,
	GAIN_32x,
	GAIN_TOTAL
};

#pragma pack(push,1)
typedef struct _filter_clip {
	cmr_u32 spsmd_max;
	cmr_u32 spsmd_min;
	cmr_u32 sobel_max;
	cmr_u32 sobel_min;
} filter_clip_t;

typedef struct _win_coord {
	cmr_u32 start_x;
	cmr_u32 start_y;
	cmr_u32 end_x;
	cmr_u32 end_y;
} win_coord_t;

typedef struct _AF_Window_Config {
	cmr_u8 valid_win_num;
	cmr_u8 win_strategic;
	win_coord_t win_pos[25];
	cmr_u32 win_weight[25];
} AF_Window_Config;

typedef struct _af_tuning_param {
	cmr_u8 flag;		// Tuning parameter switch, 1 enable tuning parameter, 0 disenable it
	filter_clip_t filter_clip[SCENE_NUM][GAIN_TOTAL];	// AF filter threshold
	cmr_s32 bv_threshold[SCENE_NUM][SCENE_NUM];	//BV threshold
	AF_Window_Config SAF_win;	// SAF window config
	AF_Window_Config CAF_win;	// CAF window config
	AF_Window_Config VAF_win;	// VAF window config
	// default param for indoor/outdoor/dark
	AF_Tuning AF_Tuning_Data[SCENE_NUM];	// Algorithm related parameter
	cmr_u8 soft_landing_dly;
	cmr_u8 soft_landing_step;
	cmr_u8 vcm_hysteresis;
	cmr_u8 dummy[98];	// for 4-bytes alignment issue
} af_tuning_param_t;

#pragma pack(pop)

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

typedef struct _vcm_ops {
	cmr_u32(*set_pos) (void *handle, cmr_u16 pos);
	cmr_u32(*get_otp) (void *handle, cmr_u16 * inf, cmr_u16 * macro);
	cmr_u32(*set_motor_bestmode) (void *handle);
	cmr_u32(*set_test_vcm_mode) (void *handle, char *vcm_mode);
	cmr_u32(*get_test_vcm_mode) (void *handle);
	cmr_u32(*get_motor_pos) (void *handle, cmr_u16 * pos);
} vcm_ops_t;

typedef struct _prime_face_base_info {
	cmr_u32 sx;
	cmr_u32 sy;
	cmr_u32 ex;
	cmr_u32 ey;
	cmr_u32 area;
	cmr_u32 diff_area_thr;
	cmr_u32 diff_cx_thr;
	cmr_u32 diff_cy_thr;
	cmr_u16 converge_cnt_thr;
	cmr_u16 converge_cnt;
	cmr_u16 diff_trigger;
} prime_face_base_info_t;

typedef struct _focus_stat {
	cmr_u32 force_write;
	cmr_u32 reg_param[10];
} focus_stat_reg_t;

typedef struct _af_fv_info {
	uint64 af_fv0[10];	//[10]:10 ROI, sum of FV0
	uint64 af_fv1[10];	//[10]:10 ROI, sum of FV1
} af_fv;

typedef struct _Bokeh_tuning_param {
	cmr_u16 from_pos;
	cmr_u16 to_pos;
	cmr_u16 move_step;
	cmr_u16 vcm_dac_up_bound;
	cmr_u16 vcm_dac_low_bound;
	cmr_u16 boundary_ratio;	/*  (Unit : Percentage) *//* depend on the AF Scanning */
} Bokeh_tuning_param;

typedef struct _af_ctrl {
	char af_version[40];
	enum af_state state;
	enum af_state pre_state;
	enum caf_state caf_state;
	enum scene curr_scene;
	eAF_MODE algo_mode;
	cmr_u32 takePicture_timeout;
	cmr_u32 request_mode;	// enum isp_focus_mode af_mode
	cmr_u32 need_re_trigger;
	cmr_u64 vcm_timestamp;
	cmr_u64 dcam_timestamp;
	cmr_u64 takepic_timestamp;
	AF_Data fv;
	//Andrew : close address begin for easy parsing
	af_fv af_fv_val;
	struct af_iir_nr_info af_iir_nr;
	struct af_enhanced_module_info af_enhanced_module;
	struct afm_thrd_rgb thrd;
	struct isp_face_area face_info;
	cmr_u32 Y_sum_trigger;
	cmr_u32 Y_sum_normalize;
	uint64 fv_combine[T_TOTAL_FILTER_TYPE];
	struct af_gsensor_info gsensor_info;
	prime_face_base_info_t face_base;
//    isp_ctrl_context    *isp_ctx;
	pthread_mutex_t af_work_lock;
	pthread_mutex_t caf_work_lock;
	sem_t af_wait_caf;
	af_tuning_param_t af_tuning_data;
	AF_Window_Config *win_config;
	isp_info_t isp_info;
	lens_info_t lens;
	cmr_s32 touch;
	cmr_s32 flash_on;
	roi_info_t roi;
	roi_rgb_y_t roi_RGBY;
	ae_info_t ae;
	awb_info_t awb;
	pd_algo_result_t pd;
	filter_clip_t filter_clip[SCENE_NUM][GAIN_TOTAL];
	cmr_s32 bv_threshold[SCENE_NUM][SCENE_NUM];
	cmr_u8 pre_scene;
	cmr_s32 ae_lock_num;
	cmr_s32 awb_lock_num;
	cmr_s32 lsc_lock_num;
	cmr_s32 nlm_lock_num;
	void *trig_lib;
	caf_trigger_ops_t trig_ops;
	//pthread_mutex_t     caf_lock;
	cmr_handle af_task;
	cmr_u64 last_frame_ts;	// timestamp of last frame
	ae_cali_t ae_cali_data;
	vcm_ops_t vcm_ops;
	cmr_u32 vcm_stable;
	cmr_u32 af_statistics[105];	// for maximum size in all chip to accomodate af statistics
	cmr_s32 node_type;
	cmr_u64 k_addr;
	cmr_u64 u_addr;
	focus_stat_reg_t stat_reg;
	cmr_u32 defocus;
	cmr_u8 bypass;
	cmr_u8 soft_landing_dly;
	cmr_u8 soft_landing_step;
	cmr_u8 caf_first_stable;
	cmr_u32 inited_af_req;
	//non-zsl,easy for motor moving and capturing
	cmr_u8 test_loop_quit;
	pthread_t test_loop_handle;
	//porting from isp2.1 af 1.0
	pthread_mutex_t status_lock;
	void *caller;
	cmr_u32 win_peak_pos[MULTI_STATIC_TOTAL];
	cmr_u32 is_high_fps;
	cmr_u32 afm_skip_num;
	//for Bokeh
	Bokeh_tuning_param bokeh_param;
	//cmr_s32(*go_position) (void *handle, struct af_motor_pos * in_param);
	 cmr_s32(*end_notice) (void *handle, struct af_result_param * in_param);
	 cmr_s32(*start_notice) (void *handle);
	 cmr_s32(*set_monitor) (void *handle, struct af_monitor_set * in_param, cmr_u32 cur_envi);
	 cmr_s32(*set_monitor_win) (void *handler, struct af_monitor_win * in_param);
	 cmr_s32(*get_monitor_win_num) (void *handler, cmr_u32 * win_num);
	 cmr_s32(*lock_module) (void *handle, cmr_int af_locker_type);
	 cmr_s32(*unlock_module) (void *handle, cmr_int af_locker_type);
} af_ctrl_t;

typedef struct _test_mode_command {
	char *command;
	uint64 key;
	void (*command_func) (af_ctrl_t * af, char *test_param);
} test_mode_command_t;

cmr_handle sprd_afv1_init(void *in, void *out);
cmr_s32 sprd_afv1_deinit(cmr_handle handle, void *param, void *result);
cmr_s32 sprd_afv1_process(void *handle, void *in, void *out);
cmr_s32 sprd_afv1_ioctrl(void *handle, cmr_s32 cmd, void *param0, void *param1);

#endif
