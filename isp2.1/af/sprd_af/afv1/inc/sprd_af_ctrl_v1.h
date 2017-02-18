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
#include <pthread.h>

#include "ae_ctrl_types.h"
#include "lsc_adv.h"
#include "ae_ctrl.h"

//#include "af_alg.h"
#include "isp_type.h"
#include "isp_com.h"
#include "isp_drv.h"
#include "isp_pm.h"
#include "isp_dev_access.h"

#include "AFv1_Common.h"
#include "AFv1_Interface.h"
#include "AFv1_Tune.h"

//#include "af_sprd_ctrl.h"
#include "aft_interface.h"
/*------------------------------------------------------------------------------*
*					Micro Define				*
*-------------------------------------------------------------------------------*/
#ifndef AFV1_TRUE
#define AFV1_TRUE (1)
#endif
#ifndef AFV1_FALSE
#define AFV1_FALSE (0)
#endif

//#define ISP_CALLBACK_EVT 0x00040000	// FIXME: should be defined in isp_app.h
//#define ISP_PROC_AF_IMG_DATA_UPDATE (1 << 3)	// FIXME
#define AF_SAVE_MLOG_STR     "persist.sys.isp.af.mlog"	/*save/no */

#define ISP_AF_END_FLAG 0x80000000

#define AFV1_MAGIC_START		0xe5a55e5a
#define AFV1_MAGIC_END		0x5e5ae5a5

#define AFV1_TRAC(_x_) AF_LOGI _x_
#define AFV1_RETURN_IF_FAIL(exp,warning) do{if(exp) {AF_TRAC(warning); return exp;}}while(0)
#define AFV1_TRACE_IF_FAIL(exp,warning) do{if(exp) {AF_TRAC(warning);}}while(0)

//#define AF_LOGD ISP_LOGD
//#define AF_LOGE ISP_LOGE

#define SPLIT_TEST      1
#define AF_WAIT_CAF_FINISH     1
#define AF_RING_BUFFER         0
#define AF_SYS_VERSION "-20161129-15"

//int32_t _smart_io_ctrl(isp_ctrl_context* handle, uint32_t cmd);
//int32_t lsc_adv_ioctrl(lsc_adv_handle_t handle, enum alsc_io_ctrl_cmd cmd, void *in_param, void *out_param);

/*------------------------------------------------------------------------------*
*					Data Structures				*
*-------------------------------------------------------------------------------*/
enum afv1_err_type {
	AFV1_SUCCESS = 0x00,
	AFV1_ERROR,
	AFV1_PARAM_ERROR,
	AFV1_PARAM_NULL,
	AFV1_FUN_NULL,
	AFV1_HANDLER_NULL,
	AFV1_HANDLER_ID_ERROR,
	AFV1_HANDLER_CXT_ERROR,
	AFV1_ALLOC_ERROR,
	AFV1_FREE_ERROR,
	AFV1_ERR_MAX
};

enum af_state {
	STATE_INACTIVE = 0,
	STATE_IDLE,		// isp is ready, waiting for cmd
	STATE_NORMAL_AF,	// single af or touch af
	STATE_CAF,
	STATE_RECORD_CAF,
	STATE_FAF,
};

static const char *state_string[] = {
	"inactive",
	"idle",
	"normal_af",
	"caf",
	"record caf",
	"faf",
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
	uint32_t width;
	uint32_t height;
	uint32_t win_num;
} isp_info_t;

#define MAX_ROI_NUM     32	// arbitrary, more than hardware wins

typedef struct _roi_rgb_y {
	int num;
	uint32_t Y_sum[MAX_ROI_NUM];
	uint32_t R_sum[MAX_ROI_NUM];
	uint32_t G_sum[MAX_ROI_NUM];
	uint32_t B_sum[MAX_ROI_NUM];
} roi_rgb_y_t;

typedef struct _lens_info {
	uint32_t pos;
} lens_info_t;

typedef struct _ae_info {
	uint32_t stable;
	uint32_t bv;
	uint32_t exp;		// 0.1us
	uint32_t gain;
	uint32_t gain_index;
	uint32_t win_size;
	struct ae_calc_out ae;
} ae_info_t;

typedef struct _awb_info {
	uint32_t r_gain;
	uint32_t g_gain;
	uint32_t b_gain;
} awb_info_t;

typedef struct _isp_awb_statistic_hist_info {
	uint32_t r_info[1024];
	uint32_t g_info[1024];
	uint32_t b_info[1024];
	uint32_t r_hist[256];
	uint32_t g_hist[256];
	uint32_t b_hist[256];
	uint32_t y_hist[256];
} isp_awb_statistic_hist_info_t;

enum AF_RINGBUFFER {
	AF_RING_BUFFER_NO,
	AF_RING_BUFFER_YES,
};
enum scene {
	OUT_SCENE = 0,
	INDOOR_SCENE,		//INDOOR_SCENE,
	DARK_SCENE,		//DARK_SCENE,
	SCENE_NUM,
};

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
	uint32_t spsmd_max;
	uint32_t spsmd_min;
	uint32_t sobel_max;
	uint32_t sobel_min;
} filter_clip_t;

typedef struct _win_coord {
	uint32_t start_x;
	uint32_t start_y;
	uint32_t end_x;
	uint32_t end_y;
} win_coord_t;

typedef struct _AF_Window_Config {
	uint8_t valid_win_num;
	uint8_t win_strategic;
	win_coord_t win_pos[25];
	uint32_t win_weight[25];
} AF_Window_Config;

typedef struct _af_tuning_param {
	uint8_t flag;		// Tuning parameter switch, 1 enable tuning parameter, 0 disenable it
	filter_clip_t filter_clip[SCENE_NUM][GAIN_TOTAL];	// AF filter threshold
	int32_t bv_threshold[SCENE_NUM][SCENE_NUM];	//BV threshold
	AF_Window_Config SAF_win;	// SAF window config
	AF_Window_Config CAF_win;	// CAF window config
	AF_Window_Config VAF_win;	// VAF window config
	// default param for indoor/outdoor/dark
	AF_Tuning AF_Tuning_Data[SCENE_NUM];	// Algorithm related parameter
	uint8_t dummy[101];	// for 4-bytes alignment issue
} af_tuning_param_t;

#pragma pack(pop)

typedef struct _roi_info {
	uint32_t num;
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
	 int32_t(*init) (struct aft_tuning_block_param * init_param, aft_proc_handle_t * handle);
	 int32_t(*deinit) (aft_proc_handle_t * handle);
	 int32_t(*calc) (aft_proc_handle_t * handle, struct aft_proc_calc_param * alg_calc_in,
			 struct aft_proc_result * alg_calc_result);
	 int32_t(*ioctrl) (aft_proc_handle_t * handle, enum aft_cmd cmd, void *param0, void *param1);
} caf_trigger_ops_t;

#define CAF_TRIGGER_LIB "libspcaftrigger.so"

typedef struct _ae_cali {
	uint32_t r_avg[9];
	uint32_t g_avg[9];
	uint32_t b_avg[9];
	uint32_t r_avg_all;
	uint32_t g_avg_all;
	uint32_t b_avg_all;
} ae_cali_t;

typedef struct _vcm_ops {
	uint32_t (*set_pos) (void* handle, uint16_t pos);
	uint32_t (*get_otp) (void* handle, uint16_t * inf, uint16_t * macro);
	uint32_t (*set_motor_bestmode) (void* handle);
	uint32_t (*set_test_vcm_mode) (void* handle, char *vcm_mode);
	uint32_t (*get_test_vcm_mode) (void* handle);
	uint32_t (*get_motor_pos) (void* handle, uint16_t * pos);
} vcm_ops_t;

typedef struct _prime_face_base_info {
	uint32_t sx;
	uint32_t sy;
	uint32_t ex;
	uint32_t ey;
	uint32_t area;
	uint32_t diff_area_thr;
	uint32_t diff_cx_thr;
	uint32_t diff_cy_thr;
	uint16_t converge_cnt_thr;
	uint16_t converge_cnt;
	uint16_t diff_trigger;
} prime_face_base_info_t;

typedef struct _focus_stat {
	unsigned int force_write;
	unsigned int reg_param[10];
} focus_stat_reg_t;

typedef struct _af_fv_info {
	uint64 af_fv0[10];	//[10]:10 ROI, sum of FV0
	uint64 af_fv1[10];	//[10]:10 ROI, sum of FV1
} af_fv;

typedef struct _af_ctrl {
	char af_version[40];
	enum af_state state;
	enum af_state pre_state;
	enum caf_state caf_state;
	enum scene curr_scene;
	eAF_MODE algo_mode;
	uint32_t takePicture_timeout;
	uint32_t request_mode;	// enum isp_focus_mode af_mode
	uint32_t need_re_trigger;
	uint64_t vcm_timestamp;
	uint64_t dcam_timestamp;
	uint64_t takepic_timestamp;
	AF_Data fv;
	struct afm_thrd_rgb thrd;
	struct isp_face_area face_info;
	uint32_t Y_sum_trigger;
	uint32_t Y_sum_normalize;
	uint64 fv_combine[T_TOTAL_FILTER_TYPE];
	prime_face_base_info_t face_base;
//    isp_ctrl_context    *isp_ctx;
	pthread_mutex_t af_work_lock;
	pthread_mutex_t caf_work_lock;
	sem_t af_wait_caf;
	af_tuning_param_t af_tuning_data;
	AF_Window_Config *win_config;
	isp_info_t isp_info;
	lens_info_t lens;
	int touch;
	int flash_on;
	roi_info_t roi;
	roi_rgb_y_t roi_RGBY;
	ae_info_t ae;
	awb_info_t awb;
	filter_clip_t filter_clip[SCENE_NUM][GAIN_TOTAL];
	int32_t bv_threshold[SCENE_NUM][SCENE_NUM];
	uint8_t pre_scene;
	int ae_lock_num;
	int awb_lock_num;
	int lsc_lock_num;
	void *trig_lib;
	caf_trigger_ops_t trig_ops;
	//pthread_mutex_t     caf_lock;
	cmr_handle af_task;
	uint64_t last_frame_ts;	// timestamp of last frame
	ae_cali_t ae_cali_data;
	vcm_ops_t vcm_ops;
	uint32_t vcm_stable;
	uint32_t af_statistics[105];	// for maximum size in all chip to accomodate af statistics
	int32_t node_type;
	uint64_t k_addr;
	uint64_t u_addr;
	focus_stat_reg_t stat_reg;
	unsigned int defocus;
	uint8_t bypass;
	uint8_t soft_landing_dly;
	uint8_t soft_landing_step;
	unsigned int inited_af_req;
	af_fv	af_fv_val;
	struct af_iir_nr_info af_iir_nr;
	struct af_enhanced_module_info af_enhanced_module;

	//porting from isp2.1 af 1.0
	pthread_mutex_t status_lock;
	void *caller;
	 int32_t(*go_position) (void *handle, struct af_motor_pos * in_param);
	 int32_t(*end_notice) (void *handle, struct af_result_param * in_param);
	 int32_t(*start_notice) (void *handle);
	 int32_t(*set_monitor) (void *handle, struct af_monitor_set * in_param, uint32_t cur_envi);
	 int32_t(*set_monitor_win) (void *handler, struct af_monitor_win * in_param);
	 int32_t(*get_monitor_win_num) (void *handler, uint32_t * win_num);
	 int32_t(*ae_awb_lock) (void *handle);
	 int32_t(*ae_awb_release) (void *handle);
} af_ctrl_t;

typedef struct _test_mode_command {
	char *command;
	uint64 key;
	void (*command_func) (af_ctrl_t * af, char *test_param);
} test_mode_command_t;

typedef void *afv1_handle_t;

struct ae_out_bv {
	struct ae_calc_out *ae_result;
	int32_t bv;
};

cmr_handle sprd_afv1_init(void *in, void *out);
cmr_int sprd_afv1_deinit(cmr_handle handle, void *param, void *result);
cmr_int sprd_afv1_process(void *handle, void *in, void *out);
cmr_int sprd_afv1_ioctrl(void *handle, cmr_int cmd, void *param0, void *param1);

#endif
