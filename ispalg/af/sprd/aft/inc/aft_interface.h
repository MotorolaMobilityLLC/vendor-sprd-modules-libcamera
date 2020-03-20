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
 *
 *******************************************************************************
 */

#ifndef _AFT_INTERFACE_H_
#define _AFT_INTERFACE_H_

#include "cmr_types.h"

#ifdef  __cplusplus
// extern "C" {
#endif

#define AFT_INVALID_HANDLE NULL
#define MAX_AF_FILTER_CNT 10
#define MAX_AF_WIN 32
#define PD_MAX_AREA 16

enum aft_isp_mode {
	AFT_ISP_SINGLE = 0,
	AFT_ISP_DUAL_NORMAL,
	AFT_ISP_DUAL_SBS,
	AFT_ISP_CAMERA_MAX
};

enum aft_posture_type {
	AFT_POSTURE_ACCELEROMETER,
	AFT_POSTURE_MAGNETIC,
	AFT_POSTURE_ORIENTATION,
	AFT_POSTURE_GYRO,
	AFT_POSTURE_MAX
};

enum aft_err_type {
	AFT_SUCCESS = 0x00,
	AFT_ERROR,
	AFT_HANDLER_NULL,
	AFT_ERR_MAX
};

enum aft_mode {
	AFT_MODE_NORMAL = 0x00,
	AFT_MODE_MACRO,
	AFT_MODE_CONTINUE,
	AFT_MODE_VIDEO,
	AFT_MODE_MAX
};

enum aft_cmd {
	AFT_CMD_SET_BASE = 0x1000,
	AFT_CMD_SET_AF_MODE = 0x1001,
	AFT_CMD_SET_CAF_RESET = 0x1002,
	AFT_CMD_SET_CAF_STOP = 0x1003,
	AFT_CMD_SET_FLASH_STATUS = 0x1004,
	AFT_CMD_SET_FORCE_TRIGGER = 0x1005,

	AFT_CMD_GET_BASE = 0x2000,
	AFT_CMD_GET_FV_STATS_CFG = 0X2001,
	AFT_CMD_GET_AE_SKIP_INFO = 0X2002,
	AFT_CMD_GET_PD_WORKABLE = 0X2003,
	AFT_CMD_GET_SAF_TRIGGERABLE = 0x2004
};

enum aft_calc_data_type {
	AFT_DATA_AF,
	AFT_DATA_IMG_BLK,
	AFT_DATA_AE,
	AFT_DATA_SENSOR,
	AFT_DATA_CAF,
	AFT_DATA_PD,
	AFT_DATA_FD,
	AFT_DATA_TOF,
	AFT_DATA_MAX
};

enum aft_log_level {
	AFT_LOG_VERBOSE = 0,
	AFT_LOG_DEBUG,
	AFT_LOG_INFO,
	AFT_LOG_WARN,
	AFT_LOG_ERROR,
	AFT_LOG_MAX
};

enum aft_trigger_type {
	AFT_TRIG_NONE = 0x00,
	AFT_TRIG_CB,
	AFT_TRIG_PD,
	AFT_TRIG_FD,
	AFT_TRIG_TOF,
	AFT_TRIG_MAX
};

enum aft_cancel_type {
	AFT_CANC_NONE = 0x00,
	AFT_CANC_CB,
	AFT_CANC_PD,
	AFT_CANC_FD,
	AFT_CANC_TOF,
	AFT_CANC_FD_GONE,
	AFT_CANC_MAX
};

struct aft_af_win_rect {
	cmr_u32 sx;
	cmr_u32 sy;
	cmr_u32 ex;
	cmr_u32 ey;
};

struct aft_af_filter_data {
	cmr_u32 type;
	cmr_u64 *data;
	cmr_u32 data_len;
};

struct aft_af_filter_info {
	cmr_u32 filter_num;
	struct aft_af_filter_data filter_data[MAX_AF_FILTER_CNT];
};

struct aft_af_win_cfg {
	cmr_u32 win_cnt;
	struct aft_af_win_rect win_pos[MAX_AF_WIN];
	cmr_u32 win_prio[MAX_AF_WIN];
	cmr_u32 win_sel_mode;
};

struct aft_afm_info {
	struct aft_af_win_cfg win_cfg;
	struct aft_af_filter_info filter_info;
};

struct aft_img_blk_info {
	cmr_u32 block_w;
	cmr_u32 block_h;
	cmr_u32 pix_per_blk;
	cmr_u32 chn_num;
	cmr_u32 *data;
	cmr_u32 data_len;
};

struct aft_ae_info {
	cmr_u32 exp_time;
	cmr_u32 gain;
	cmr_u32 cur_lum;
	cmr_u32 target_lum;
	cmr_u32 is_stable;
	cmr_u32 flag4idx;
	cmr_u32 face_stable;
	cmr_u32 bv;
	cmr_u32 y_sum;
	cmr_u32 cur_scene;
	cmr_u32 registor_pos;
	cmr_u32 face_ae_enable;
};

struct aft_sensor_info {
	cmr_u32 sensor_type;
	float x;
	float y;
	float z;
};

struct aft_phase_diff_info {
	cmr_u32 pd_enable;
	cmr_u32 effective_pos;
	cmr_u32 effective_frmid;
	cmr_u32 confidence[PD_MAX_AREA];
	double pd_value[PD_MAX_AREA];
	cmr_u32 pd_roi_dcc[PD_MAX_AREA];
	cmr_u32 pd_roi_num;
	cmr_u32 reserved[16];
};

struct aft_tof_info {
	cmr_u32 tof_enable;
	cmr_u32 effective_pos;
	cmr_u32 effective_frmid;
	cmr_u32 status;
	cmr_u32 distance;
	cmr_u32 MAXdistance;
	cmr_u32 reserved[20];
};

struct isp_face_coor {
	cmr_u32 sx;
	cmr_u32 sy;
	cmr_u32 ex;
	cmr_u32 ey;
	cmr_s32 yaw_angle;
	cmr_s32 roll_angle;
	cmr_u32 score;
};

struct aft_face_info {
	cmr_u16 face_num;
	cmr_u16 frame_width;
	cmr_u16 frame_height;
	struct isp_face_coor face_info[10];
};

struct aft_common_info {
	cmr_u32 registor_pos;
	cmr_u32 otp_inf_pos;
	cmr_u32 otp_macro_pos;
	cmr_u32 reserved[16];
};

struct caf_time_stamp {
	cmr_u32 time_stamp_sec;
	cmr_u32 time_stamp_us;
};

struct aft_caf_blk_info {
	cmr_u16 caf_token_id;
	cmr_u16 frame_id;
	cmr_u8 valid_column_num;
	cmr_u8 valid_row_num;
	struct caf_time_stamp time_stamp;
	cmr_u32 *data;
	cmr_u32 data_len;
};

struct aft_proc_result {
	cmr_u32 is_caf_trig;
	cmr_u32 is_need_rough_search;
	cmr_u32 is_cancel_caf;
};

struct aft_proc_calc_param {
	cmr_u32 active_data_type;
	struct aft_common_info comm_info;
	struct aft_ae_info ae_info;
	union {
		struct aft_afm_info afm_info;
		struct aft_img_blk_info img_blk_info;
		struct aft_sensor_info sensor_info;
		struct aft_caf_blk_info caf_blk_info;
		struct aft_phase_diff_info pd_info;
		struct aft_face_info fd_info;
		struct aft_tof_info tof_info;
	};
};

struct aft_caf_stats_cfg {
	cmr_u8 roi_left_ration;
	cmr_u8 roi_top_ration;
	cmr_u8 roi_width_ration;
	cmr_u8 roi_height_ration;
	cmr_u8 num_blk_hor;
	cmr_u8 num_blk_ver;
};

struct aft_ae_skip_info {
	cmr_u32 ae_select_support;
	cmr_u32 ae_skip_line;
};

struct aft_tuning_block_param {
	cmr_u8 *data;
	cmr_u32 data_len;
};

struct aft_ctrl_ops {
	cmr_u8(*get_sys_time) (cmr_u64 * p_time, void *cookie);
	cmr_u8(*binfile_is_exist) (cmr_u8 * is_exist, void *cookie);
	cmr_u8(*is_aft_mlog) (cmr_u32 * is_mlog, void *cookie);
	cmr_u8(*aft_log) (cmr_u32 log_level, const char *format, ...);
	void *aft_cookie;
};

struct aft_init_in {
	struct aft_tuning_block_param tuning;
	struct aft_ctrl_ops aft_ops;
	cmr_u32 is_multi_mode;
};

struct aft_init_out {
	void *aft_dump_ptr;
	cmr_u32 aft_dump_len;
	cmr_u32 tuning_param_len;
};

typedef void *aft_proc_handle_t;

cmr_s32 caf_trigger_init(struct aft_init_in *init_in, struct aft_init_out *init_out, aft_proc_handle_t * handle);
cmr_s32 caf_trigger_deinit(aft_proc_handle_t handle);
cmr_s32 caf_trigger_calculation(aft_proc_handle_t handle, struct aft_proc_calc_param *aft_calc_in, struct aft_proc_result *aft_calc_result);
cmr_s32 caf_trigger_ioctrl(aft_proc_handle_t handle, enum aft_cmd cmd, void *param0, void *param1);

#ifdef	 __cplusplus
// }
#endif

#endif
