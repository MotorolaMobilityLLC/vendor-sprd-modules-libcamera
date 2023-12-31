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
#include <cutils/properties.h>

#include "aft_interface.h"
#include "AFv1_Interface.h"

#define AF_SYS_VERSION "2020111700"
#define AF_SAVE_MLOG_STR "persist.vendor.cam.isp.af.mlog"	/*save/no */

enum afv1_bool {
	AFV1_FALSE = 0,
	AFV1_TRUE,
};

enum afv1_err_type {
	AFV1_SUCCESS = 0x00,
	AFV1_ERROR,
	AFV1_ERR_MAX
};

enum _AF_Gsensor_Orientation {
	AF_G_NONE,
	AF_G_DEGREE0,
	AF_G_DEGREE1,
	AF_G_DEGREE2,
	AF_G_DEGREE3,
};

enum _e_Fcae_Orientation {
	FACE_NONE,
	FACE_UP,
	FACE_RIGHT,
	FACE_DOWN,
	FACE_LEFT,
} e_Fcae_Orientation;

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
	STATE_ENGINEER,
	STATE_OTAF,
};

enum focus_state {
	AF_IDLE,
	AF_SEARCHING,
	AF_STOPPED,
	AF_STOPPED_INNER,
};

enum dcam_after_vcm {
	DCAM_AFTER_VCM_NO = 0,
	DCAM_AFTER_VCM_YES
};

typedef struct _isp_info {
	cmr_u32 width;
	cmr_u32 height;
	cmr_u32 win_num;
} isp_info_t;

#define MAX_ROI_NUM     32	// hardware wins, arbitrary
#define ADPT_MAX_ROI_NUM     5	// focus area set by touch or face

typedef struct _roi_rgb_y {
	cmr_s32 num;
	cmr_u32 Y_sum[MAX_ROI_NUM];
	cmr_u32 R_sum[MAX_ROI_NUM];
	cmr_u32 G_sum[MAX_ROI_NUM];
	cmr_u32 B_sum[MAX_ROI_NUM];
} roi_rgb_y_t;

typedef struct _lens_info {
	cmr_u16 pos;
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

struct af_adpt_saf_roi {
	cmr_u32 sx;
	cmr_u32 sy;
	cmr_u32 ex;
	cmr_u32 ey;
};

struct af_adpt_faf_roi {
	cmr_u32 sx;
	cmr_u32 sy;
	cmr_u32 ex;
	cmr_u32 ey;
	cmr_s32 yaw_angle;
	cmr_s32 roll_angle;
	cmr_u32 score;
};

struct af_adpt_roi_info {
	cmr_u32 af_mode;
	cmr_u32 win_num;
	union {
		struct af_adpt_saf_roi touch[ADPT_MAX_ROI_NUM];
		struct af_adpt_faf_roi face[ADPT_MAX_ROI_NUM];
	};
};

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

// af lib
typedef struct _af_lib_ops {
	void *(*init) (af_init_in * af_in, af_init_out * af_out);
	 cmr_u8(*deinit) (void *handle);
	 cmr_u8(*calc) (void *handle);
	 cmr_u8(*ioctrl) (void *handle, cmr_u32 cmd, void *in, void *out);
} af_lib_ops_t;

#define AF_LIB "libspafv1.so"

// caf trigger
typedef struct _caf_trigger_ops {
	aft_proc_handle_t handle;
	struct aft_init_out init_out;
	struct aft_ae_skip_info ae_skip_info;

	 cmr_s32(*init) (struct aft_init_in * init_in, struct aft_init_out * init_out, aft_proc_handle_t * handle);
	 cmr_s32(*deinit) (aft_proc_handle_t handle);

	 cmr_s32(*calc) (aft_proc_handle_t handle, struct aft_proc_calc_param * alg_calc_in, struct aft_proc_result * alg_calc_result);
	 cmr_s32(*ioctrl) (aft_proc_handle_t handle, enum aft_cmd cmd, void *param0, void *param1);
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

struct af_fullscan_info {
	/* Register Parameters */
	/* These params will depend on the AF setting */
	cmr_u8 row_num;		/* The number of AF windows with row (i.e. vertical) *//* depend on the AF Scanning */
	cmr_u8 column_num;	/* The number of AF windows with row (i.e. horizontal) *//* depend on the AF Scanning */
	cmr_u32 *win_peak_pos;	/* The seqence of peak position which be provided via struct isp_af_fullscan_info *//* depend on the AF Scanning */
	cmr_u16 vcm_dac_up_bound;
	cmr_u16 vcm_dac_low_bound;
	cmr_u16 boundary_ratio;	/* (Unit : Percentage) *//* depend on the AF Scanning */
	cmr_u32 af_peak_pos;
	cmr_u32 near_peak_pos;
	cmr_u32 far_peak_pos;
	cmr_u32 distance_reminder;
	cmr_u32 reserved[16];
};

typedef struct _af_fv_info {
	cmr_u64 af_fv0[10];	// [10]:10 ROI, sum of FV0
	cmr_u64 af_fv1[10];	// [10]:10 ROI, sum of FV1
} af_fv;

typedef struct _afm_tuning_param_sharkl2 {
	cmr_u8 iir_level;
	cmr_u8 nr_mode;
	cmr_u8 cw_mode;
	cmr_u8 fv0_e;
	cmr_u8 fv1_e;
	cmr_u8 dummy[3];	// 4 bytes align
} afm_tuning_sharkl2;

struct af_enhanced_module_info_u {
	cmr_u8 chl_sel;
	cmr_u8 nr_mode;
	cmr_u8 center_weight;
	cmr_u8 fv_enhanced_mode[2];
	cmr_u8 clip_en[2];
	cmr_u32 max_th[2];
	cmr_u32 min_th[2];
	cmr_u8 fv_shift[2];
	cmr_s8 fv1_coeff[36];
};

struct af_iir_nr_info_u {
	cmr_u8 iir_nr_en;
	cmr_s16 iir_g0;
	cmr_s16 iir_c1;
	cmr_s16 iir_c2;
	cmr_s16 iir_c3;
	cmr_s16 iir_c4;
	cmr_s16 iir_c5;
	cmr_s16 iir_g1;
	cmr_s16 iir_c6;
	cmr_s16 iir_c7;
	cmr_s16 iir_c8;
	cmr_s16 iir_c9;
	cmr_s16 iir_c10;
};

typedef struct _mlog_AFtime {
	nsecs_t time_total;
	nsecs_t system_time0_1;
	nsecs_t system_time1_1;
	char *AF_type;
} mlog_AFtime;

typedef struct _cts_params {
	float focus_distance;
	cmr_s32 frame_num;
	cmr_u32 set_cts_params_flag;
	cmr_u32 reserved[10];
} cts_params;

typedef struct _af_squeue {
	cmr_u32 size;
	cmr_s32 front;
	cmr_s32 rear;
	cts_params af_params[6];
} af_squeue;

typedef struct _af_ctrl {
	void *af_alg_cxt;	// AF_Data fv;
	cmr_u32 af_dump_info_len;
	cmr_u32 state;		// enum af_state state;
	cmr_u32 pre_state;	// enum af_state pre_state;
	cmr_u32 focus_state;	// enum caf_state caf_state;
	cmr_u32 algo_mode;	// eAF_MODE algo_mode;
	cmr_u32 takePicture_timeout;
	cmr_u32 request_mode;
	cmr_u64 vcm_timestamp;
	cmr_u64 dcam_timestamp;
	cmr_u64 takepic_timestamp;
	cmr_u32 Y_sum_trigger;
	cmr_u32 Y_sum_normalize;
	//cmr_u64 fv_combine[T_TOTAL_FILTER_TYPE];
	af_fv af_fv_val;
	struct afctrl_gsensor_info gsensor_info;
	cmr_u32 g_orientation;
	cmr_u32 f_orientation;
	cmr_s32 roll_angle;
	struct afctrl_face_info face_info;
	isp_info_t isp_info;
	lens_info_t lens;
	cmr_s32 flash_on;
	cmr_u32 win_offset;
	roi_info_t roi;
	roi_rgb_y_t roi_RGBY;
	ae_info_t ae;
	awb_info_t awb;
	struct af_aem_stats_data aem_stats;
	pd_algo_result_t pd;
	cmr_s32 ae_lock_num;
	cmr_s32 awb_lock_num;
	cmr_s32 lsc_lock_num;
	cmr_s32 nlm_lock_num;
	cmr_s32 ae_partial_lock_num;
	void *trig_lib;
	caf_trigger_ops_t trig_ops;
	void *af_lib;
	af_lib_ops_t af_ops;
	ae_cali_t ae_cali_data;
	cmr_u32 vcm_stable;
	cmr_u32 defocus;
	cmr_u8 bypass;
	cmr_handle caller;
	cmr_u32 win_peak_pos[MULTI_STATIC_TOTAL];
	cmr_u32 is_high_fps;
	cmr_u32 afm_skip_num;
	afm_tuning_sharkl2 afm_tuning;
	struct aft_proc_calc_param prm_trigger;
	isp_awb_statistic_hist_info_t rgb_stat;
	cmr_u32 trigger_source_type;
	char AF_MODE[PROPERTY_VALUE_MAX];
	struct afctrl_otp_info otp_info;
	cmr_u32 is_multi_mode;
	cmr_u8 *aftuning_data;
	cmr_u32 aftuning_data_len;
	cmr_u8 *pdaftuning_data;
	cmr_u32 pdaftuning_data_len;
	cmr_u8 *afttuning_data;
	cmr_u32 afttuning_data_len;
	cmr_u8 *toftuning_data;
	cmr_u32 toftuning_data_len;
	tof_measure_data_t tof;

	struct afctrl_cb_ops cb_ops;
	cmr_u8 *pdaf_rdm_otp_data;
	cmr_u32 trigger_counter;
	cmr_u32 cur_master_id;
	cmr_u32 next_master_id;
	cmr_u32 camera_id;
	cmr_u32 sensor_role;
	af_ctrl_br_ioctrl bridge_ctrl;
	struct realbokeh_vcm_range realboekh_range;
	struct realbokeh_golden_vcm_data golden_data;
	mlog_AFtime AFtime;
	cmr_u32 motor_status;
	cmr_u32 frame_counter;
	cmr_u16 af_otp_type;
	cmr_u32 cont_mode_trigger;
	cmr_u32 zoom_ratio;
	cmr_u32 last_request_mode;
	cmr_u32 range_L1;
	cmr_u32 range_L4;
	struct af_adpt_roi_info win;
	cmr_u32 ot_switch;	// objecttracking switch
	cmr_u32 pdaf_type;
	cmr_u32 slave_focus_cnt;
	cmr_u32 pdaf_support;
	af_squeue queue;
	cmr_u16 focus_distance_cnt;
	cmr_u32 focus_distance_result[3];
} af_ctrl_t;

cmr_handle sprd_afv1_init(void *in, void *out);
cmr_s32 sprd_afv1_deinit(cmr_handle handle, void *param, void *result);
cmr_s32 sprd_afv1_process(void *handle, void *in, void *out);
cmr_s32 sprd_afv1_ioctrl(void *handle, cmr_s32 cmd, void *param0, void *param1);

#endif
