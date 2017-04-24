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
 * AF_Common.h
 *
 * \brief
 * common data for AF
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

#ifndef __AFV1_COMMON_H__
#define  __AFV1_COMMON_H__

#include <stdio.h>
#include <memory.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#ifndef WIN32
#include <jni.h>
#include <android/log.h>
#endif
#include "AFv1_Type.h"
#include "cmr_types.h"

/*1.System info*/
#define VERSION             "2.110"
#define SUB_VERSION             "-formal"
#define STRING(s) #s

/*2.function error code*/
#define ERR_SUCCESS         0x0000
#define ERR_FAIL            0x0001
#define ERR_UNKNOW          0x0002

#define TOTAL_POS 1024

//Data num for AF
#define TOTAL_AF_ZONE 10
#define MAX_SAMPLE_NUM	    25
#define TOTAL_SAMPLE_NUM	29
#define ROUGH_SAMPLE_NUM	25	//MAX((ROUGH_SAMPLE_NUM_L3+ROUGH_SAMPLE_NUM_L2),(ROUGH_START_POS_L1+ROUGH_START_POS_L2))
#define FINE_SAMPLE_NUM		10
#define MAX_TIME_SAMPLE_NUM	100
#define G_SENSOR_Q_TOTAL (3)

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

typedef enum _eAF_FV_RATIO_TYPE {
	T_R_Total = 0,
	T_R_J1,
	T_R_J2,
	T_R_J3,
	T_R_Slope_Total,
	T_R_SAMPLE_NUM
} eAF_FV_RATIO_TYPE;

typedef enum _eAF_MODE {
	SAF = 0,		//single zone AF
	CAF,			//continue AF
	VAF,			//Video CAF
	FAF,			//Face AF
	MAF,			//Multi zone AF
	PDAF,			//PDAF
	TMODE_1,		//Test mode 1
	Wait_Trigger		//wait for AF trigger
} eAF_MODE;

typedef enum _eAF_Triger_Type {
	RF_NORMAL = 0,		//noraml R/F search for AFT
	R_NORMAL,		//noraml Rough search for AFT
	F_NORMAL,		//noraml Fine search for AFT
	RF_FAST,		//Fast R/F search for AFT
	R_FAST,			//Fast Rough search for AFT
	F_FAST,			//Fast Fine search for AFT
	DEFOCUS,
} eAF_Triger_Type;

typedef enum _eSAF_Status {
	SAF_Status_Start = 0,
	SAF_Status_Init,
	SAF_Status_RSearch,
	SAF_Status_FSearch,
	SAF_Status_Move_Lens,
	SAF_Status_Stop
} eSAF_Status;

typedef enum _eCAF_Status {
	CAF_Status_Start = 0,
	CAF_Status_Init,
	CAF_Status_RSearch,
	CAF_Status_FSearch,
	CAF_Status_Move_Lens,
	CAF_Status_Stop
} eCAF_Status;

typedef enum _eSAF_Main_Process {
	SAF_Main_INIT = 0,
	SAF_Main_SEARCH,
	SAF_Main_DONE
} eSAF_Main_Process;

typedef enum _eCAF_Main_Process {
	CAF_Main_INIT = 0,
	CAF_Main_SEARCH,
	CAF_Main_DONE
} eCAF_Main_Process;

typedef enum _eSAF_Search_Process {
	SAF_Search_INIT = 0,
	SAF_Search_SET_ROUGH_SEARCH,
	SAF_Search_ROUGH_SEARCH,
	SAF_Search_ROUGH_HAVE_PEAK,
	SAF_Search_SET_FINE_SEARCH,
	SAF_Search_FINE_SEARCH,
	SAF_Search_FINE_HAVE_PEAK,
	SAF_Search_NO_PEAK,
	SAF_Search_DONE,
	SAF_Search_DONE_BY_UI,
	SAF_Search_TOTAL,

} eSAF_Search_Process;

typedef enum _eCAF_Search_Process {
	CAF_Search_INIT = 0,
	CAF_Search_SET_ROUGH_SEARCH,
	CAF_Search_ROUGH_SEARCH,
	CAF_Search_ROUGH_HAVE_PEAK,
	CAF_Search_SET_FINE_SEARCH,
	CAF_Search_FINE_SEARCH,
	CAF_Search_FINE_HAVE_PEAK,
	CAF_Search_NO_PEAK,
	CAF_Search_DONE,
	CAF_Search_DONE_BY_UI,
	CAF_Search_TOTAL,

} eCAF_Search_Process;

typedef enum _eAF_Result {
	AF_INIT = 0,
	AF_FIND_HC_PEAK,	//high confidence peak
	AF_FIND_LC_PEAK,	//low confidence peak
	AF_NO_PEAK,
	AF_TOO_FAR,
	AF_TOO_NEAR,
	AF_REVERSE,
	AF_SKY,
	AF_UNKNOW
} eAF_Result;

typedef enum _e_LOCK {
	LOCK = 0,
	UNLOCK,
} e_LOCK;

typedef enum _e_DIR {
	FAR2NEAR = 0,
	NEAR2FAR,
} e_DIR;

typedef enum _e_RESULT {
	NO_PEAK = 0,
	HAVE_PEAK,
} e_RESULT;

typedef enum _e_AF_TRIGGER {
	NO_TRIGGER = 0,
	AF_TRIGGER,
} e_AF_TRIGGER;

typedef enum _e_AF_BOOL {
	AF_FALSE = 0,
	AF_TRUE,
} e_AF_BOOL;

typedef enum _e_AF_AE_Gain {
	AE_1X = 0,
	AE_2X,
	AE_4X,
	AE_8X,
	AE_16X,
	AE_32X,
	AE_Gain_Total,
} e_AF_AE_Gain;

enum {
	PD_NO_HINT,
	PD_DIR_ONLY,
	PD_DIR_RANGE,
} PDAF_HINT_INFO;

enum {
	SENSOR_X_AXIS,
	SENSOR_Y_AXIS,
	SENSOR_Z_AXIS,	
	SENSOR_AXIS_TOTAL,	
};
//=========================================================================================//
// Public Structure Instance
//=========================================================================================//
//#pragma pack(push, 1)

typedef struct _AE_Report {
	cmr_u8 bAEisConverge;	//flag: check AE is converged or not
	cmr_s16 AE_BV;		//brightness value
	cmr_u16 AE_EXP;		//exposure time (ms)
	cmr_u16 AE_Gain;		//X128: gain1x = 128
	cmr_u32 AE_Pixel_Sum;	//AE pixel sum which needs to match AF blcok
	cmr_u16 AE_Idx;		//AE exposure level
} AE_Report;

typedef struct _AF_FV_DATA {
	cmr_u8 MaxIdx[MAX_SAMPLE_NUM];	//index of max statistic value
	cmr_u8 MinIdx[MAX_SAMPLE_NUM];	//index of min statistic value

	cmr_u8 BPMinIdx[MAX_SAMPLE_NUM];	//index of min statistic value before peak
	cmr_u8 APMinIdx[MAX_SAMPLE_NUM];	//index of min statistic value after peak

	cmr_u8 ICBP[MAX_SAMPLE_NUM];	//increase count before peak
	cmr_u8 DCBP[MAX_SAMPLE_NUM];	//decrease count before peak
	cmr_u8 EqBP[MAX_SAMPLE_NUM];	//equal count before peak
	cmr_u8 ICAP[MAX_SAMPLE_NUM];	//increase count after peak
	cmr_u8 DCAP[MAX_SAMPLE_NUM];	//decrease count after peak
	cmr_u8 EqAP[MAX_SAMPLE_NUM];	//equal count after peak
	cmr_s8 BPC[MAX_SAMPLE_NUM];	//ICBP - DCBP
	cmr_s8 APC[MAX_SAMPLE_NUM];	//ICAP - DCAP

	cmr_u64 Val[MAX_SAMPLE_NUM];	//statistic value
	cmr_u64 Start_Val;	//statistic value of start position

	cmr_u16 BP_R10000[T_R_SAMPLE_NUM][MAX_SAMPLE_NUM];	//FV ratio X 10000 before peak
	cmr_u16 AP_R10000[T_R_SAMPLE_NUM][MAX_SAMPLE_NUM];	//FV ratio X 10000 after peak

	cmr_u8 Search_Result[MAX_SAMPLE_NUM];	//search result of focus data
	cmr_u16 peak_pos[MAX_SAMPLE_NUM];	//peak position of focus data
	cmr_u64 PredictPeakFV[MAX_SAMPLE_NUM];	//statistic value

} AF_FV_DATA;

typedef struct _AF_FV {
	AF_FV_DATA Filter[T_TOTAL_FILTER_TYPE];
	AE_Report AE_Rpt[MAX_SAMPLE_NUM];

} AF_FV;
#pragma pack(push,1)
typedef struct _AF_FILTER_TH {
	cmr_u16 UB_Ratio_TH[T_R_SAMPLE_NUM];	//The Up bound threshold of FV ratio, [0]:total, [1]:3 sample, [2]:5 sample, [3]:7 sample
	cmr_u16 LB_Ratio_TH[T_R_SAMPLE_NUM];	//The low bound threshold of FV ratio, [0]:total, [1]:3 sample, [2]:5 sample, [3]:7 sample
	cmr_u32 MIN_FV_TH;	//The threshold of FV to check the real curve
} AF_FILTER_TH;

typedef struct _AF_TH {
	AF_FILTER_TH Filter[T_TOTAL_FILTER_TYPE];

} AF_TH;
#pragma pack(pop)

typedef struct _SAF_SearchData {
	cmr_u8 SAF_RS_TotalFrame;	//total work frames during rough search
	cmr_u8 SAF_FS_TotalFrame;	//total work frames during fine search

	cmr_u8 SAF_RS_LensMoveCnt;	//total lens execute frame during rough search
	cmr_u8 SAF_FS_LensMoveCnt;	//total lens execute frame during fine search

	cmr_u8 SAF_RS_StatisticCnt;	//total statistic frame during rough search
	cmr_u8 SAF_FS_StatisticCnt;	//total statistic frame during fine search

	cmr_u8 SAF_RS_MaxSearchTableNum;	//maxima number of search table during rough search
	cmr_u8 SAF_FS_MaxSearchTableNum;	//maxima number of search table during fine search

	cmr_u8 SAF_RS_DIR;	//direction of rough search:Far to near or near to far
	cmr_u8 SAF_FS_DIR;	//direction of fine search:Far to near or near to far

	cmr_u8 SAF_Skip_Frame;	//skip this frame or not

	cmr_u16 SAF_RSearchTable[ROUGH_SAMPLE_NUM];
	cmr_u16 SAF_RSearchTableByTuning[TOTAL_SAMPLE_NUM];
	cmr_u16 SAF_FSearchTable[FINE_SAMPLE_NUM];

	AF_FV SAF_RFV;		//The FV data of rough search
	AF_FV SAF_FFV;		//The FV data of fine search

	AF_FV SAF_Pre_RFV;	//The last time FV data of rough search
	AF_FV SAF_Pre_FFV;	//The last time FV data of fine search

	AF_TH SAF_RS_TH;	//The threshold of rough search
	AF_TH SAF_FS_TH;	//The threshold of fine search

} SAF_SearchData;

typedef struct _CAF_SearchData {
	cmr_u8 CAF_RS_TotalFrame;
	cmr_u8 CAF_FS_TotalFrame;

	cmr_u16 CAF_RSearchTable[ROUGH_SAMPLE_NUM];
	cmr_u16 CAF_FSearchTable[FINE_SAMPLE_NUM];

	AF_FV CAF_RFV;
	AF_FV CAF_FFV;

	AF_FV CAF_Pre_RFV;
	AF_FV CAF_Pre_FFV;

} CAF_SearchData;

#pragma pack(push,1)
typedef struct _AF_Scan_Table {
	//Rough search
	cmr_u16 POS_L1;
	cmr_u16 POS_L2;
	cmr_u16 POS_L3;
	cmr_u16 POS_L4;

	cmr_u16 Sample_num_L1_L2;
	cmr_u16 Sample_num_L2_L3;
	cmr_u16 Sample_num_L3_L4;

	cmr_u16 Normal_Start_Idx;
	cmr_u16 Rough_Sample_Num;

	//Fine search
	cmr_u16 Fine_Sample_Num;
	cmr_u16 Fine_Search_Interval;
	cmr_u16 Fine_Init_Num;

} AF_Scan_Table;

typedef struct aftuning_coeff_s {
	cmr_u32 saf_coeff[8];
	cmr_u32 caf_coeff[8];
	cmr_u32 saf_stat_param[AE_Gain_Total];
	cmr_u32 caf_stat_param[AE_Gain_Total];
	cmr_u8 reserve[32 * 4];
} aftuning_coeff_t;

typedef struct aftuning_param_s {
	cmr_u32 enable_adapt_af;
	cmr_u32 _alg_select;
	cmr_u32 _lowlight_agc;
	cmr_u32 _flat_rto;
	cmr_u32 _falling_rto;
	cmr_u32 _rising_rto;
	cmr_u32 _stat_min_value[AE_Gain_Total];
	cmr_u32 _stat_min_diff[AE_Gain_Total];
	cmr_u32 _break_rto;
	cmr_u32 _turnback_rto;
	cmr_u32 _forcebreak_rto;
	cmr_u32 _break_count;
	cmr_u32 _max_shift_idx;
	cmr_u32 _lowlight_ma_count;
	cmr_s32 _posture_diff_slop;
	cmr_u32 _temporal_flat_slop;
	cmr_u32 _limit_search_interval;
	cmr_u32 _sky_scene_thr;
	cmr_u8 	reserve[128-1];
	cmr_u8	_min_fine_idx;
} aftuning_param_t;

typedef struct _AF_Tuning_Para {
	//AF Scan Table
	AF_Scan_Table Scan_Table_Para[AE_Gain_Total];

	//AF Threshold
	AF_TH SAF_RS_TH[AE_Gain_Total];	//The threshold of rough search
	AF_TH SAF_FS_TH[AE_Gain_Total];	//The threshold of fine search

	cmr_s32 dummy[200];
} AF_Tuning_Para;

typedef struct _Lens_Info {
	//Lens Info
	cmr_u16 Lens_MC_MIN;	//minimal mechenical position
	cmr_u16 Lens_MC_MAX;	//maximal mechenical position
	cmr_u16 Lens_INF;	//INF position
	cmr_u16 Lens_MACRO;	//MACRO position
	cmr_u16 Lens_Hyper;	//Hyper Focus position
	cmr_u16 One_Frame_Max_Move;	//skip one frame if move position is bigger than TH

} Lens_Info;

typedef struct _AF_Tuning {
	//Lens Info
	Lens_Info Lens_Para;
	AF_Tuning_Para SAFTuningPara;	//SAF parameters
	AF_Tuning_Para CAFTuningPara;	//CAF parameters
	AF_Tuning_Para VCAFTuningPara;	//Video CAF parameters
	cmr_u32 tuning_ver_code;	//ex Algo AF2.104 , tuning-0001-> 0x21040001
	cmr_u32 tuning_date_code;	//ex 20170306 -> 0x20170306
	aftuning_coeff_t af_coeff;	//AF coefficient for control speed and overshot
	aftuning_param_t adapt_af_param;	//adapt AF parameter
	cmr_u8 dummy[400];
} AF_Tuning;

#define AFAUTO_SCAN_STOP_NMAX (256)
#define FOCUS_STAT_WIN_TOTAL	(10)
#define MULTI_STATIC_TOTAL (9)
#define AF_RESULT_DATA_SIZE	(32)
#define AF_CHECK_SCENE_HISTORY	(15)
#define AF_SCENE_CAL_STDEV_TOTAL	(32)

typedef struct _afscan_status_s {
	cmr_u32 n_stops;
	cmr_u32 pos_from;
	cmr_u32 pos_to;
	cmr_u32 pos_jump;
	cmr_u32 scan_idx;
	cmr_u32 stat_log[AFAUTO_SCAN_STOP_NMAX];
	cmr_u32 stat_sum_log[AFAUTO_SCAN_STOP_NMAX];
	cmr_u32 scan_tbl[AFAUTO_SCAN_STOP_NMAX];
	cmr_u32 frmid[AFAUTO_SCAN_STOP_NMAX];
	cmr_s32 bv_log[AFAUTO_SCAN_STOP_NMAX];
	cmr_u32 luma[AFAUTO_SCAN_STOP_NMAX];
	cmr_u32 gain[AFAUTO_SCAN_STOP_NMAX];
	cmr_u32 scan_tbl_posidx[AFAUTO_SCAN_STOP_NMAX];
	cmr_u32 scan_tbl_pos[AFAUTO_SCAN_STOP_NMAX];
	cmr_u32 scan_tbl_stat[AFAUTO_SCAN_STOP_NMAX];
	cmr_u32 scan_tbl_stat2[AFAUTO_SCAN_STOP_NMAX];
	cmr_u32 scan_tbl_macrv[AFAUTO_SCAN_STOP_NMAX];
	cmr_s32 scan_tbl_slop[AFAUTO_SCAN_STOP_NMAX];
	cmr_s32 scan_tbl_maslop[AFAUTO_SCAN_STOP_NMAX];
	cmr_u32 scan_tbl_luma[AFAUTO_SCAN_STOP_NMAX];
	cmr_u32 scan_tbl_jump[AFAUTO_SCAN_STOP_NMAX];
	cmr_u32 scan_tbl_pkidx[AFAUTO_SCAN_STOP_NMAX];
	cmr_u32 scan_tbl_start;
	cmr_u32 scan_tbl_end;
	cmr_u32 scan_interval;
	cmr_u32 peak_idx;
	cmr_u32 peak_pos;
	cmr_u32 peak_stat;
	cmr_u32 peak_stat_sum;
	cmr_u32 turnback_status;
	cmr_u32 valley_stat;
	cmr_u32 valley_idx;
	cmr_u32 last_idx;
	cmr_u32 last_stat;
	cmr_u32 alg_sts;
	cmr_u32 scan_dir;
	cmr_u32 last_dir;
	cmr_u32 init_pos;
	cmr_u32 tbl_falling_cnt[AFAUTO_SCAN_STOP_NMAX];
	cmr_u32 tbl_flat_cnt[AFAUTO_SCAN_STOP_NMAX];
	cmr_u32 turnback_cnt;
	cmr_u32 turnback_idx;
	cmr_u32 turnback_cond;
	cmr_u32 break_idx;
	cmr_u32 break_cond;
	cmr_u32 fine_begin;
	cmr_u32 fine_end;
	cmr_u32 fine_stat;
	cmr_u32 fine_pkidx;
	cmr_u32 blcpeak_idx;
	cmr_u32 bound_cnt;
	cmr_u32 stat_vari_rto;
	cmr_u32 stat_rising_rto;
	cmr_u32 stat_falling_rto;
	cmr_u32 vly_far_stat;
	cmr_u32 vly_far_rto;
	cmr_u32 vly_far_pos;
	cmr_u32 vly_near_stat;
	cmr_u32 vly_near_rto;
	cmr_u32 vly_near_pos;
	cmr_u32 breakcount;
	cmr_u32 breakratio;

	cmr_u32 frm_num;
	cmr_u32 pkfrm_num;
	cmr_u32 vly_far_idx;
	cmr_u32 vly_near_idx;
	cmr_u32 spdup_num;
	cmr_s32 local_scrtbl[AFAUTO_SCAN_STOP_NMAX];
	cmr_s32 localma_scrtbl[AFAUTO_SCAN_STOP_NMAX];
	cmr_s32 localsum_scrtbl[AFAUTO_SCAN_STOP_NMAX];
	cmr_s32 localluma_scrtbl[AFAUTO_SCAN_STOP_NMAX];
	cmr_u32 local_idx;
	cmr_u32 localma_idx;
	cmr_u32 localsum_idx;
	cmr_u32 localluma_idx;
	cmr_u32 lock_pos;
	//multi AF
	cmr_u32 multi_pkstat[MULTI_STATIC_TOTAL];
	cmr_u32 multi_pkpos[MULTI_STATIC_TOTAL];

	cmr_u32 min_stat_diff;
	cmr_u32 min_stat_val;

	cmr_u32 fine_range;
	cmr_u32 luma_slop;
	cmr_u32 scan_algorithm;

	cmr_u32 pos_far;
	cmr_u32 pos_near;
	cmr_u32 far_idx;
	cmr_u32 near_idx;
	cmr_s32 scan_tbl_full_slop[AFAUTO_SCAN_STOP_NMAX];
	cmr_s32 far_slop;
	cmr_s32 near_slop;
	cmr_u32 scan_tbl_slop_type;
	cmr_u32 far_falling_cnt;
	cmr_u32 near_falling_cnt;
	cmr_s32 lk_loacl_scrtbl[AFAUTO_SCAN_STOP_NMAX];
	cmr_u32 lk_local_idx;
	cmr_u32 focus_inf;
	cmr_u32 focus_macro;
	cmr_u32 peak_inverse;
	cmr_u32 peak_quad;
} afscan_status_t;

typedef struct af_ctrl_pd_info_s {
	cmr_u32 info_type;
	cmr_u32 predict_direction;
	cmr_u32 predict_peak;
	cmr_u32 predict_far_stop;
	cmr_u32 predict_near_stop;
	cmr_u32 phase_diff_value;
	cmr_u32 confidence_level;
} pd_info_t;

typedef struct _af_control_status_s {
	cmr_u32 state;
	cmr_u32 frmid;
	cmr_u32 scene_reg;
	cmr_u32 scene_result;
	cmr_u32 scene_event;
	cmr_u32 env_ena;
	cmr_u32 env_reconv_cnt;
	cmr_u32 env_reconv_limit;
	cmr_u32 motor_excitation;
	cmr_u32 idsp_enable;
	cmr_u32 idsp_resume_thd;
	cmr_u32 idsp_reconv_thd;
	cmr_u32 idsp_reconv_limit;
	cmr_u32 idsp_reconv_cnt;
	cmr_u32 stat_nsy_enable;
	cmr_u32 idsp_frmbuff_clr_cnt;
	cmr_u32 idsp_frmbuff_clr_limit;
	cmr_u32 idsp_reset_frmid;
	cmr_u32 debug_cb;
	cmr_u32 env_avgy_histroy[AF_CHECK_SCENE_HISTORY];
	pd_info_t pd_info;
} afctrl_status_t;

typedef struct af_scan_env_info_s {
	cmr_u32 fps;
	cmr_u32 ae_state;
	cmr_u32 exp_boundary;
	cmr_u32 y_avg;
	cmr_u32 y_tgt;
	cmr_u32 curr_gain;
	cmr_u32 exp_time;
	cmr_u32 y_stdev;
	cmr_s32 curr_bv;
	cmr_s32 next_bv;
	cmr_s32 diff_bv;
} scan_env_info_t;

typedef struct af_scan_info_s {
	cmr_u32 lowlight_af;
	cmr_u32 dark_af;
	cmr_u32 lowcontrast_af;
	cmr_u32 curr_fpos;
	scan_env_info_t env;
	scan_env_info_t report;
	cmr_u32 luma_slop;
	cmr_u32 times_result;
	cmr_u32 position_result[AF_RESULT_DATA_SIZE];
	cmr_u32 frmid_result[AF_RESULT_DATA_SIZE];
	cmr_u32 coast_result[AF_RESULT_DATA_SIZE];
	cmr_u32 ma_count;
	cmr_u32 posture_status;
} afscan_info_t;

/* ========================== Structure ============================ */
typedef struct afstat_frame_buffer_s {
	cmr_u32 curr_frm_stat[FOCUS_STAT_WIN_TOTAL];
	//cmr_u32 curr_frm_y[FOCUS_STAT_WIN_TOTAL];
	cmr_u32 last_frm_stat[FOCUS_STAT_WIN_TOTAL];
	//cmr_u32 last_frm_y[FOCUS_STAT_WIN_TOTAL];
	//cmr_u32 focus_block_idx;
	//cmr_u32 peak_block_edge;
	//cmr_u32 peak_block_y;
	//cmr_s32 peak_block_edge_rela;
	//cmr_s32 peak_block_y_rela;
	//cmr_s32 peak_block_around_rela[FOCUS_STAT_AROUND_BLOCK_DATA];
	cmr_u32 stat_weight;
	cmr_u32 stat_sum;
	cmr_u32 luma_avg;
	//cmr_u32 multi_grid_sum[MULTI_STATIC_TOTAL];
	cmr_u32 multi_stat_tbl[AFAUTO_SCAN_STOP_NMAX][MULTI_STATIC_TOTAL];	/*debug info of defocus function */
} afstat_frmbuf_t;

typedef struct defocus_param_s {
	cmr_u32 scan_from;
	cmr_u32 scan_to;
	cmr_u32 per_steps;
} defocus_param_t;

typedef struct afdbg_ctrl_s {
	cmr_u32 alg_msg;
	cmr_u32 dump_info;
	defocus_param_t defocus;
} afdbg_ctrl_t;

typedef struct _af_process_s {
	afctrl_status_t ctrl_status;
	afscan_status_t scan_status;
	afscan_info_t scan_info;
	afstat_frmbuf_t stat_data;
	afdbg_ctrl_t dbg_ctrl;
	aftuning_param_t adapt_af_param;	//adapt AF parameter
	cmr_u8 	reserve[128*4];	//for temp debug
} _af_process_t;

typedef struct motion_sensor_result_s {
	cmr_s64 timestamp;
	uint32_t sensor_g_posture;
	uint32_t sensor_g_queue_cnt;
	float g_sensor_queue[SENSOR_AXIS_TOTAL][G_SENSOR_Q_TOTAL];
	cmr_u32 reserved[12];
} motion_sensor_result_t;	

typedef struct pd_algo_result_s {
	cmr_u32 pd_enable;
	cmr_u32 effective_pos;
	cmr_u32 effective_frmid;
	cmr_u32 confidence;
	double pd_value;
	cmr_u16 pd_roi_dcc;
	cmr_u8 reserved[10];	//aligment to 4 byte
} pd_algo_result_t;

typedef struct motion_sensor_data_s {
	cmr_u32 sensor_type;
	float x;
	float y;
	float z;
	cmr_u32 reserved[12];	
} motion_sensor_data_t;

#pragma pack(pop)

typedef struct _CAF_Tuning_Para {
	cmr_s32 dummy;
} CAF_Tuning_Para;

typedef struct _SAF_INFO {
	eAF_Triger_Type Cur_AFT_Type;	//the search method
	cmr_u8 SAF_Main_Process;	//the process state of SAF main
	cmr_u8 SAF_Search_Process;	//the process state of SAF search
	cmr_u8 SAF_Status;
	cmr_u8 SAF_RResult;	//rough search result
	cmr_u8 SAF_FResult;	//fine search result
	cmr_u8 SAF_Total_work_frame;	//total work frames during SAF work
	cmr_u8 SAF_AE_Gain;	//AE gain
	cmr_u16 SAF_Start_POS;	//focus position before AF work
	cmr_u16 SAF_Cur_POS;	//current focus positon
	cmr_u16 SAF_Final_POS;	//final move positon
	cmr_u16 SAF_Final_VCM_POS;	//final move positon load from VCM
	cmr_u16 SAF_RPeak_POS;	//Peak positon of rough search
	cmr_u16 SAF_FPeak_POS;	//Peak positon of fine search
	cmr_u64 SAF_SYS_TIME_ENTER[MAX_TIME_SAMPLE_NUM];	//save each time while entering SAF search
	cmr_u64 SAF_SYS_TIME_EXIT[MAX_TIME_SAMPLE_NUM];	//save each time while entering SAF search
	Lens_Info Lens_Para;	//current lens parameters
	AF_Scan_Table SAF_Scan_Table_Para;	//current scan table parameters
} SAF_INFO;

typedef struct _CAF_INFO {
	cmr_u8 CAF_mode;

} CAF_INFO;

typedef struct _SAF_Data {
	SAF_INFO sAFInfo;
	SAF_SearchData sAFSearchData;

} SAF_Data;

typedef struct _CAF_Data {
	CAF_INFO cAFInfo;
	CAF_SearchData cAFSearchData;
	CAF_Tuning_Para cAFTuningPara;

} CAF_Data;

typedef struct _AF_OTP_Data {
	cmr_u8 bIsExist;
	cmr_u16 INF;
	cmr_u16 MACRO;

} AF_OTP_Data;

typedef struct _af_stat_data_s {
	cmr_u32 roi_num;
	cmr_u32 stat_num;
	cmr_u64 *p_stat;
} _af_stat_data_t;

typedef struct _AF_Ctrl_Ops {
	ERRCODE(*statistics_wait_cal_done) (void *cookie);
	ERRCODE(*statistics_get_data) (cmr_u64 fv[T_TOTAL_FILTER_TYPE], _af_stat_data_t * p_stat_data, void *cookie);
	ERRCODE(*statistics_set_data) (cmr_u32 set_stat, void *cookie);
	ERRCODE(*phase_detection_get_data) (pd_algo_result_t * pd_result, void *cookie);
	ERRCODE(*motion_sensor_get_data)(motion_sensor_result_t * ms_result, void *cookie);
	ERRCODE(*lens_get_pos) (cmr_u16 * pos, void *cookie);
	ERRCODE(*lens_move_to) (cmr_u16 pos, void *cookie);
	ERRCODE(*lens_wait_stop) (void *cookie);
	ERRCODE(*lock_ae) (e_LOCK bisLock, void *cookie);
	ERRCODE(*lock_awb) (e_LOCK bisLock, void *cookie);
	ERRCODE(*lock_lsc) (e_LOCK bisLock, void *cookie);
	ERRCODE(*get_sys_time) (cmr_u64 * pTime, void *cookie);
	ERRCODE(*get_ae_report) (AE_Report * pAE_rpt, void *cookie);
	ERRCODE(*set_af_exif) (const void *pAF_data, void *cookie);
	ERRCODE(*sys_sleep_time) (cmr_u16 sleep_time, void *cookie);
	ERRCODE(*get_otp_data) (AF_OTP_Data * pAF_OTP, void *cookie);
	ERRCODE(*get_motor_pos) (cmr_u16 * motor_pos, void *cookie);
	ERRCODE(*set_motor_sacmode) (void *cookie);
	ERRCODE(*binfile_is_exist) (cmr_u8 * bisExist, void *cookie);
	ERRCODE(*get_vcm_param) (cmr_u32 *param, void *cookie);	
	ERRCODE(*af_log) (const char *format, ...);
	 ERRCODE(*af_start_notify) (eAF_MODE AF_mode, void *cookie);
	 ERRCODE(*af_end_notify) (eAF_MODE AF_mode, void *cookie);
	void *cookie;
} AF_Ctrl_Ops;

typedef struct _AF_Trigger_Data {
	cmr_u8 bisTrigger;
	eAF_Triger_Type AF_Trigger_Type;
	eAF_MODE AFT_mode;
	defocus_param_t defocus_param;
	cmr_u32 reserved[8];
} AF_Trigger_Data;

typedef struct _AF_Win {
	cmr_u16 Set_Zone_Num;	//FV zone number
	cmr_u16 AF_Win_X[TOTAL_AF_ZONE];
	cmr_u16 AF_Win_Y[TOTAL_AF_ZONE];
	cmr_u16 AF_Win_W[TOTAL_AF_ZONE];
	cmr_u16 AF_Win_H[TOTAL_AF_ZONE];

} AF_Win;

typedef struct _AF_Data {
	eAF_MODE AF_mode;
	eAF_MODE Pre_AF_mode;
	AF_Trigger_Data AFT_Data;
	SAF_Data sAF_Data;
	CAF_Data cAF_Data;
	AE_Report AE_Rpt[MAX_TIME_SAMPLE_NUM];
	AF_OTP_Data AF_OTP;
	AF_Win AF_Win_Data;
	cmr_u32 vcm_register;
	cmr_s8 AF_Version[10];
	AF_Tuning AF_Tuning_Data;
	_af_process_t af_proc_data;
	motion_sensor_result_t sensor_result;
	cmr_u32 dump_log;
	AF_Ctrl_Ops AF_Ops;
} AF_Data;

//#pragma pack(pop)

#endif
