/*
 *******************************************************************************
 * $Header$
 *
 *  Copyright (c) 2016-2025 Unisoc Communications Inc. All rights reserved.
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
	OTAF,			//objecttracking af
} eAF_MODE;

typedef enum _e_AF_MULTI_MODE {
	SPAF_SINGLE = 0,
	SPAF_DUAL_C_C,
	SPAF_DUAL_SBS,
	SPAF_BLUR_REAR,
	SPAF_DUAL_W_T,
	SPAF_DUAL_C_M,
	SPAF_CAMERA_MAX
} e_AF_MULTI_MODE;

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

typedef enum _af_io_cmd {
	AF_IOCTRL_SET_BASE = 0x100,
	AF_IOCTRL_SET_ROI,
	AF_IOCTRL_SET_HW_WINS,
	AF_IOCTRL_SET_TRIGGER,
	AF_IOCTRL_SET_CANCEL,
	AF_IOCTRL_SET_TIMESTAMP,
	AF_IOCTRL_SET_REG_POS,
	AF_IOCTRL_SET_FV,
	AF_IOCTRL_SET_DAC_INFO,
	AF_IOCTRL_SET_PRE_TRIGGER_DATA,
	AF_IOCTRL_SET_BOKEH_DISTANCE,
	AF_IOCTRL_SET_MAX,

	AF_IOCTRL_GET_BASE = 0x200,
	AF_IOCTRL_GET_OTP,
	AF_IOCTRL_GET_LENS_RANGE,
	AF_IOCTRL_GET_ALG_MODE,
	AF_IOCTRL_GET_RESULT,
	AF_IOCTRL_GET_BOKEH_GOLDEN_DATA,
	AF_IOCTRL_GET_BOKEH_RESULT,	// out of use
	AF_IOCTRL_GET_MAX,
} af_io_cmd;

typedef enum pdaf_support {
	PDAF_UNSUPPORTED = 0,
	PDAF_SHIELD_TYPE1,
	PDAF_SHIELD_TYPE2,
	PDAF_SHIELD_TYPE3,
	PDAF_DUAL_MODE1,
	PDAF_DUAL_MODE3,
	PDAF_DUAL_MODE4,
	PDAF_MAX
} pdaf_support;

#pragma pack(push,4)
typedef struct _AF_Result {
	cmr_u32 AF_Result;
	cmr_u32 af_mode;
} AF_Result;

struct spaf_saf_data {
	cmr_u32 sx;
	cmr_u32 sy;
	cmr_u32 ex;
	cmr_u32 ey;
};

struct spaf_face_data {
	cmr_u32 sx;
	cmr_u32 sy;
	cmr_u32 ex;
	cmr_u32 ey;
	cmr_s32 yaw_angle;
	cmr_s32 roll_angle;
	cmr_u32 score;
};

typedef struct spaf_face_info_s {
	struct spaf_face_data data[10];
	cmr_u32 num;
} spaf_face_info_t;

typedef struct spaf_roi_s {
	cmr_u32 af_mode;
	cmr_u32 win_num;
	union {
		struct spaf_saf_data saf_roi[SPAF_MAX_ROI_NUM];
		struct spaf_face_data face_roi[SPAF_MAX_ROI_NUM];
	};
	cmr_u32 multi_mode;
	cmr_u32 zoom_ratio;
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
	cmr_u32 af_type;	// notify to AF which mode PDAF is in
	cmr_u32 reserved[15];
} pd_algo_result_t;

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
	cmr_u8 near_stable;
	cmr_u8 reserved[38];
} AE_Report;

typedef struct _Y_Sum {
	cmr_u32 y_sum[10];
} Y_Sum;

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

typedef struct _bokeh_distance_info {
	cmr_u16 total_seg;
	cmr_u16 distance[20];
	cmr_u16 reserved[20];
} bokeh_distance_info;

typedef struct _bokeh_motor_info {
	cmr_u16 limited_infi;	// calibrated for 30cm
	cmr_u16 limited_macro;	// calibrated for 150cm
	cmr_u16 total_seg;
	cmr_u16 vcm_dac[20];
	cmr_u16 reserved[20];
} bokeh_motor_info;

typedef struct _bokeh_golden_data_info {
	cmr_u16 golden_macro;
	cmr_u16 golden_infinity;
	cmr_u16 golden_count;
	cmr_u16 golden_distance[40];
	cmr_u16 golden_vcm[40];
	cmr_u16 reserved[10];
} bokeh_golden_data_info;

struct AFtoPD_info_param {
	cmr_u16 Center_X;
	cmr_u16 Center_Y;
	cmr_u16 sWidth;
	cmr_u16 sHeight;
};

typedef struct _ROIinfo_param {
	cmr_u16 ROI_Size;
	struct AFtoPD_info_param ROI_info[45];
} ROIinfo;

typedef struct _saf_extra_data_s {
	cmr_u8 pd_enable;
	cmr_u8 pd_workable;
	cmr_u32 reserved[20];
} saf_extra_data_t;

typedef struct _lens_range_info {
	cmr_u16 range_L1;
	cmr_u16 range_L4;
	cmr_u16 reserved[10];
} lens_range_info;

typedef struct _AF_Ctrl_Ops {
	void *cookie;
	 cmr_u8(*statistics_wait_cal_done) (void *cookie);
	 cmr_u8(*statistics_get_data) (cmr_u64 fv[T_TOTAL_FILTER_TYPE], _af_stat_data_t * p_stat_data, void *cookie);
	 cmr_u8(*statistics_set_data) (cmr_u32 set_stat, void *cookie);
	 cmr_u8(*clear_fd_stop_counter) (cmr_u32 * FD_count, void *cookie);
	 cmr_u8(*phase_detection_get_data) (pd_algo_result_t * pd_result, void *cookie);
	 cmr_u8(*face_detection_get_data) (spaf_face_info_t * FD_IO, void *cookie);
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
	 cmr_u8(*set_bokeh_vcm_info) (bokeh_motor_info * range, void *cookie);

	//SharkLE Only ++
	 cmr_u8(*set_pulse_line) (cmr_u32 line, void *cookie);
	 cmr_u8(*set_next_vcm_pos) (cmr_u32 pos, void *cookie);
	 cmr_u8(*set_pulse_log) (cmr_u32 flag, void *cookie);
	 cmr_u8(*set_clear_next_vcm_pos) (void *cookie);
	//SharkLE Only --

	 cmr_u8(*get_tof_data) (tof_measure_data_t * tof_result, void *cookie);
	 cmr_u8(*set_Gridinfo_to_PD) (eAF_MODE AF_mode, ROIinfo * PD_ROI, void *cookie);
	 cmr_u8(*get_saf_extra_data) (saf_extra_data_t * saf_extra, void *cookie);
	 cmr_u8(*get_sub_wins_ysum) (Y_Sum * c_y_sum, void *cookie);
} AF_Ctrl_Ops;

typedef struct _af_tuning_block_param {
	cmr_u8 *data;
	cmr_u32 data_len;
	cmr_u8 *pd_data;
	cmr_u32 pd_data_len;
	cmr_u8 *tof_data;
	cmr_u32 tof_data_len;
} af_tuning_block_param;

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

typedef struct _af_init_in {
	AF_Ctrl_Ops AF_Ops;
	af_tuning_block_param tuning;
	char *sys_version;
	cmr_u32 camera_id;
	cmr_u32 pdaf_support;
	cmr_u32 reserved[10];
} af_init_in;

typedef struct _af_init_out {
	void *af_dump_ptr;
	cmr_u32 af_dump_len;
	cmr_u32 reserved[10];
} af_init_out;
#pragma pack(pop)

void *af_init(af_init_in * af_in, af_init_out * af_out);
cmr_u8 af_deinit(void *handle);
cmr_u8 af_process(void *handle);
cmr_u8 af_ioctrl(void *handle, cmr_u32 cmd, void *param);

#endif
