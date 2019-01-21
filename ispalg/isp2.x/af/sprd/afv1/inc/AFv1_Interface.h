/*
 *******************************************************************************
 * $Header$
 *
 *  Copyright (c) 2016-2025 Spreadtrum Inc. All rights reserved.
 *
 *  +-----------------------------------------------------------------+
 *  | THIS SOFTWARE IS FURNISHED UNDER A LICENSE AND MAY ONLY BE USED |
 *  | AND COPIED IN ACCORDANCE WITH THE TERMS AND CONDITIONS OF SUCH  |
 *  | A LICENSE AND WITH THE INCLUSION OF THE THIS COPY RIGHT NOTICE. |
 *  | THIS SOFTWARE OR ANY OTHER COPIES OF THIS SOFTWARE MAY NOT BE   |
 *  | PROVIDED OR OTHERWISE MADE AVAILABLE TO ANY OTHER PERSON. THE   |
 *  | OWNERSHIP AND TITLE OF THIS SOFTWARE IS NOT TRANSFERRED.        |
 *  |                                                                 |
 *  | THE INFORMATION IN THIS SOFTWARE IS SUBJECT TO CHANGE WITHOUT   |
 *  | ANY PRIOR NOTICE AND SHOULD NOT BE CONSTRUED AS A COMMITMENT BY |
 *  | SPREADTRUM INC.                                                 |
 *  +-----------------------------------------------------------------+
 *
 * $History$
 *
 *******************************************************************************
 */

/*!
 *******************************************************************************
 * Copyright 2016-2025 Spreadtrum, Inc. All rights reserved.
 *
 * \file
 * AF_Interface.h
 *
 * \brief
 * Interface for AF
 *
 * \date
 * 2016/01/03
 *
 * \author
 * Galen Tsai
 *
 *
 *******************************************************************************
 */

#ifndef __AFV1_INTERFACE_H__
#define  __AFV1_INTERFACE_H__

#define PD_MAX_AREA 16
#define G_SENSOR_Q_TOTAL (3)
#define MULTI_STATIC_TOTAL (9)
#define AFV1_SCENE_NUM 3
#define SPAF_MAX_ROI_NUM 5
#define SPAF_MAX_WIN_NUM 90

typedef enum _eAF_FILTER_TYPE {
	T_SOBEL9 = 0,
	T_SOBEL5,
	T_SPSMD,
	T_FV0,
	T_FV1,
	T_COV,
	T_TOTAL_FILTER_TYPE
} eAF_FILTER_TYPE;

typedef enum _eAF_OTP_TYPE {
	T_LENS_BY_DEFAULT = 0,
	T_LENS_BY_OTP,
	T_LENS_BY_TUNING,
} eAF_OTP_TYPE;

enum {
	SENSOR_X_AXIS,
	SENSOR_Y_AXIS,
	SENSOR_Z_AXIS,
	SENSOR_AXIS_TOTAL,
};

typedef enum _e_LOCK {
	LOCK = 0,
	UNLOCK,
} e_LOCK;

typedef enum _eAF_MODE {
	SAF = 0,		//single zone AF
	CAF,			//continue AF
	VAF,			//Video CAF
	FAF,			//Face AF
	MAF,			//Multi zone AF
	PDAF,			//PDAF
	TMODE_1,		//Test mode 1
	Wait_Trigger,		//wait for AF trigger
	TOF,			//[TOF_---] // Time of flight
	None,			//nothing to do
} eAF_MODE;

typedef enum _e_AF_TRIGGER {
	NO_TRIGGER = 0,
	AF_TRIGGER,
	RE_TRIGGER,
} e_AF_TRIGGER;

typedef enum _eAF_Triger_Type {
	RF_NORMAL = 0,		//noraml R/F search for AFT
	R_NORMAL,		//noraml Rough search for AFT
	F_NORMAL,		//noraml Fine search for AFT
	RF_FAST,		//Fast R/F search for AFT
	R_FAST,			//Fast Rough search for AFT
	F_FAST,			//Fast Fine search for AFT
	DEFOCUS,
	BOKEH,
	//RE_TRIGGER,   //no use
} eAF_Triger_Type;

enum {
	AF_TIME_DCAM,
	AF_TIME_VCM,
	AF_TIME_CAPTURE,
	AF_TIME_TYPE_TOTAL,
};

typedef enum _e_RESULT {
	NO_PEAK = 0,
	HAVE_PEAK,
} e_RESULT;

// Interface Commands
typedef enum _AF_IOCTRL_CMD {
	AF_IOCTRL_TRIGGER = 0,
	AF_IOCTRL_STOP,
	AF_IOCTRL_Get_Result,
	AF_IOCTRL_SET_ROI,
	AF_IOCTRL_RECORD_HW_WINS,
	AF_IOCTRL_Record_Vcm_Pos,
	AF_IOCTRL_Get_Alg_Mode,
	AF_IOCTRL_Get_Bokeh_Result,
	AF_IOCTRL_Set_Time_Stamp,
	AF_IOCTRL_Set_Pre_Trigger_Data,
	AF_IOCTRL_Record_FV,
	AF_IOCTRL_Set_Dac_info,
	AF_IOCTRL_MAX,
} AF_IOCTRL_CMD;

#pragma pack(push,4)
// Interface Param Structure
typedef struct _AF_Result {
	cmr_u32 AF_Result;
	cmr_u32 af_mode;
} AF_Result;

struct spaf_saf_roi {
	cmr_u32 sx;
	cmr_u32 sy;
	cmr_u32 ex;
	cmr_u32 ey;
};

struct spaf_faf_roi {
	cmr_u32 sx;
	cmr_u32 sy;
	cmr_u32 ex;
	cmr_u32 ey;
	cmr_s32 yaw_angle;
	cmr_s32 roll_angle;
	cmr_u32 score;
};

typedef struct spaf_roi_s {
	cmr_u32 af_mode;
	cmr_u32 win_num;
	union {
		struct spaf_saf_roi saf_roi[SPAF_MAX_ROI_NUM];
		struct spaf_faf_roi face_roi[SPAF_MAX_ROI_NUM];
	};
} spaf_roi_t;

struct spaf_coordnicate {
	cmr_u32 sx;
	cmr_u32 sy;
	cmr_u32 ex;
	cmr_u32 ey;
};

typedef struct spaf_win_s {
	cmr_u32 win_num;
	struct spaf_coordnicate win[SPAF_MAX_WIN_NUM];
} spaf_win_t;

typedef struct _AF_Timestamp {
	cmr_u32 type;
	cmr_u64 time_stamp;
} AF_Timestamp;

typedef struct _af_stat_data_s {
	cmr_u32 roi_num;
	cmr_u32 stat_num;
	cmr_u64 *p_stat;
} _af_stat_data_t;

typedef struct pd_algo_result_s {
	cmr_u32 pd_enable;
	cmr_u32 effective_pos;
	cmr_u32 effective_frmid;
	cmr_u32 confidence[PD_MAX_AREA];
	double pd_value[PD_MAX_AREA];
	cmr_u32 pd_roi_dcc[PD_MAX_AREA];
	cmr_u32 pd_roi_num;
	cmr_u32 af_type;// notify to AF which mode PDAF is in
	cmr_u32 reserved[15];
} pd_algo_result_t;

typedef struct _IO_Face_area_s {
	cmr_u32 sx;
	cmr_u32 sy;
	cmr_u32 ex;
	cmr_u32 ey;
} IO_Face_area_t;

typedef struct motion_sensor_result_s {
	cmr_s64 timestamp;
	uint32_t sensor_g_posture;
	uint32_t sensor_g_queue_cnt;
	float g_sensor_queue[SENSOR_AXIS_TOTAL][G_SENSOR_Q_TOTAL];
	cmr_u32 reserved[12];
} motion_sensor_result_t;

typedef struct _AE_Report {
	cmr_u8 bAEisConverge;	//flag: check AE is converged or not
	cmr_s16 AE_BV;		//brightness value
	cmr_u16 AE_EXP;		//exposure time (ms)
	cmr_u16 AE_Gain;	//X128: gain1x = 128
	cmr_u32 AE_Pixel_Sum;	//AE pixel sum which needs to match AF blcok
	cmr_u16 AE_Idx;		//AE exposure level
	cmr_u32 cur_fps;
	cmr_u32 cur_lum;
	cmr_u32 cur_index;
	cmr_u32 cur_ev;
	cmr_u32 cur_iso;
	cmr_u32 target_lum;
	cmr_u32 target_lum_ori;
	cmr_u32 flag4idx;
	cmr_s32 bisFlashOn;
	cmr_u8 ae_stable_cnt;
	cmr_u8 reserved[39];
} AE_Report;

typedef struct _AF_OTP_Data {
	cmr_u8 bIsExist;
	cmr_u16 INF;
	cmr_u16 MACRO;
	cmr_u32 reserved[10];
} AF_OTP_Data;

typedef struct _tof_sensor_result {
	cmr_u32 TimeStamp;
	cmr_u32 MeasurementTimeUsec;
	cmr_u16 RangeMilliMeter;
	cmr_u16 RangeDMaxMilliMeter;
	cmr_u32 SignalRateRtnMegaCps;	//which is effectively a measure of target reflectance
	cmr_u32 AmbientRateRtnMegaCps;	//which is effectively a measure of the ambient light
	cmr_u16 EffectiveSpadRtnCount;	//(SPAD)Single photon avalanche diode
	cmr_u8 ZoneId;
	cmr_u8 RangeFractionalPart;
	cmr_u8 RangeStatus;
} tof_sensor_result;

typedef struct _tof_measure_data_s {
	tof_sensor_result data;
	cmr_u8 tof_enable;
	cmr_u32 effective_frmid;
	cmr_u32 last_status;
	cmr_u32 last_distance;
	cmr_u32 last_MAXdistance;
	cmr_u32 tof_trigger_flag;
	cmr_u32 reserved[20];

} tof_measure_data_t;

typedef struct _AF_Ctrl_Ops {
	void *cookie;
	 cmr_u8(*statistics_wait_cal_done) (void *cookie);
	 cmr_u8(*statistics_get_data) (cmr_u64 fv[T_TOTAL_FILTER_TYPE], _af_stat_data_t * p_stat_data, void *cookie);
	 cmr_u8(*statistics_set_data) (cmr_u32 set_stat, void *cookie);
	 cmr_u8(*clear_fd_stop_counter) (cmr_u32 * FD_count, void *cookie);
	 cmr_u8(*phase_detection_get_data) (pd_algo_result_t * pd_result, void *cookie);
	 cmr_u8(*face_detection_get_data) (IO_Face_area_t * FD_IO, void *cookie);
	 cmr_u8(*motion_sensor_get_data) (motion_sensor_result_t * ms_result, void *cookie);
	 cmr_u8(*lens_get_pos) (cmr_u16 * pos, void *cookie);
	 cmr_u8(*lens_move_to) (cmr_u16 pos, void *cookie);
	 cmr_u8(*lens_wait_stop) (void *cookie);
	 cmr_u8(*lock_ae) (e_LOCK bisLock, void *cookie);
	 cmr_u8(*lock_awb) (e_LOCK bisLock, void *cookie);
	 cmr_u8(*lock_lsc) (e_LOCK bisLock, void *cookie);
	 cmr_u8(*get_sys_time) (cmr_u64 * pTime, void *cookie);
	 cmr_u8(*get_ae_report) (AE_Report * pAE_rpt, void *cookie);
	 cmr_u8(*set_af_exif) (const void *pAF_data, void *cookie);
	 cmr_u8(*sys_sleep_time) (cmr_u16 sleep_time, void *cookie);
	 cmr_u8(*get_otp_data) (AF_OTP_Data * pAF_OTP, void *cookie);
	 cmr_u8(*get_motor_pos) (cmr_u16 * motor_pos, void *cookie);
	 cmr_u8(*set_motor_sacmode) (void *cookie);
	 cmr_u8(*binfile_is_exist) (cmr_u8 * bisExist, void *cookie);
	 cmr_u8(*get_vcm_param) (cmr_u32 * param, void *cookie);
	 cmr_u8(*af_log) (const char *format, ...);
	 cmr_u8(*af_start_notify) (eAF_MODE AF_mode, void *cookie);
	 cmr_u8(*af_end_notify) (eAF_MODE AF_mode, cmr_u8 result, void *cookie);
	 cmr_u8(*set_wins) (cmr_u32 index, cmr_u32 start_x, cmr_u32 start_y, cmr_u32 end_x, cmr_u32 end_y, void *cookie);
	 cmr_u8(*get_win_info) (cmr_u32 * hw_num, cmr_u32 * isp_w, cmr_u32 * isp_h, void *cookie);
	 cmr_u8(*lock_ae_partial) (cmr_u32 is_lock, void *cookie);
	//SharkLE Only ++
	 cmr_u8(*set_pulse_line) (cmr_u32 line, void *cookie);
	 cmr_u8(*set_next_vcm_pos) (cmr_u32 pos, void *cookie);
	 cmr_u8(*set_pulse_log) (cmr_u32 flag, void *cookie);
	 cmr_u8(*set_clear_next_vcm_pos) (void *cookie);
	//SharkLE Only --

	//[TOF_+++]
	 cmr_u8(*get_tof_data) (tof_measure_data_t * tof_result, void *cookie);
	//[TOF_---]

} AF_Ctrl_Ops;

typedef struct _af_tuning_block_param {
	cmr_u8 *data;
	cmr_u32 data_len;
} af_tuning_block_param;

typedef struct _haf_tuning_param_s {
	cmr_u8 *pd_data;
	cmr_u32 pd_data_len;
	cmr_u8 *tof_data;
	cmr_u32 tof_data_len;
} haf_tuning_param_t;

typedef struct defocus_param_s {
	cmr_u32 scan_from;
	cmr_u32 scan_to;
	cmr_u32 per_steps;
} defocus_param_t;

typedef struct _AF_Trigger_Data {
	cmr_u8 bisTrigger;
	cmr_u32 AF_Trigger_Type;
	cmr_u32 AFT_mode;
	defocus_param_t defocus_param;
	cmr_u32 re_trigger;
	cmr_u32 trigger_source;
	cmr_u32 reserved[6];
} AF_Trigger_Data;

typedef struct _Bokeh_Result {
	cmr_u8 row_num;		/* The number of AF windows with row (i.e. vertical) *//* depend on the AF Scanning */
	cmr_u8 column_num;	/* The number of AF windows with row (i.e. horizontal) *//* depend on the AF Scanning */
	cmr_u32 win_peak_pos_num;
	cmr_u32 *win_peak_pos;	/* The seqence of peak position which be provided via struct isp_af_fullscan_info *//* depend on the AF Scanning */
	cmr_u16 vcm_dac_up_bound;
	cmr_u16 vcm_dac_low_bound;
	cmr_u16 boundary_ratio;	/*  (Unit : Percentage) *//* depend on the AF Scanning */
	cmr_u32 af_peak_pos;
	cmr_u32 near_peak_pos;
	cmr_u32 far_peak_pos;
	cmr_u32 distance_reminder;
	cmr_u32 reserved[16];
} Bokeh_Result;
#pragma pack(pop)

void *AF_init(AF_Ctrl_Ops * AF_Ops, af_tuning_block_param * af_tuning_data, haf_tuning_param_t * haf_tune_data, cmr_u32 * dump_info_len, char *sys_version);
cmr_u8 AF_deinit(void *handle);
cmr_u8 AF_Process_Frame(void *handle);
cmr_u8 AF_IOCtrl_process(void *handle, AF_IOCTRL_CMD cmd, void *param);

#endif
