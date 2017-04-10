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

#ifndef _ISP_AF_H_
#define _ISP_AF_H_

#include "isp_alg_fw.h"

#ifdef WIN32
#include "sci_type.h"
#endif

#ifdef  __cplusplus
extern "C" {
#endif

#define MAX_AF_WINS 32

enum af_err_type {
	AF_SUCCESS = 0x00,
	AF_ERROR,
	AF_ERR_MAX
};

enum scene {
	OUT_SCENE = 0,
	INDOOR_SCENE,	//INDOOR_SCENE,
	DARK_SCENE,	//DARK_SCENE,
	SCENE_NUM,
};

enum af_mode {
	AF_MODE_NORMAL = 0x00,
	AF_MODE_MACRO,
	AF_MODE_CONTINUE,
	AF_MODE_VIDEO,
	AF_MODE_MANUAL,
	AF_MODE_PICTURE,
	AF_MODE_FULLSCAN,
	AF_MODE_MAX
};

enum af_cmd {
	AF_CMD_SET_BASE = 0x1000,
	AF_CMD_SET_AF_MODE = 0x1001,
	AF_CMD_SET_AF_POS = 0x1002,
	AF_CMD_SET_TUNING_MODE = 0x1003,
	AF_CMD_SET_SCENE_MODE = 0x1004,
	AF_CMD_SET_AF_START = 0x1005,
	AF_CMD_SET_AF_STOP = 0x1006,
	AF_CMD_SET_AF_RESTART = 0x1007,
	AF_CMD_SET_CAF_RESET = 0x1008,
	AF_CMD_SET_CAF_STOP = 0x1009,
	AF_CMD_SET_AF_FINISH = 0x100A,
	AF_CMD_SET_AF_BYPASS = 0x100B,
	AF_CMD_SET_DEFAULT_AF_WIN = 0x100C,
	AF_CMD_SET_FLASH_NOTICE = 0x100D,
	AF_CMD_SET_ISP_START_INFO = 0x100E,
	AF_CMD_SET_ISP_STOP_INFO = 0x100F,
	AF_CMD_SET_ISP_TOOL_AF_TEST = 0x1010,
	AF_CMD_SET_CAF_TRIG_START = 0x1011,
	AF_CMD_SET_AE_INFO = 0x1012,
	AF_CMD_SET_AWB_INFO = 0x1013,
	AF_CMD_SET_FACE_DETECT = 0x1014,
	AF_CMD_SET_DCAM_TIMESTAMP = 0x1015,
	AF_CMD_SET_PD_INFO = 0x1016,
	AF_CMD_SET_UPDATE_AUX_SENSOR = 0x1017,

	AF_CMD_GET_BASE = 0x2000,
	AF_CMD_GET_AF_MODE = 0X2001,
	AF_CMD_GET_AF_CUR_POS = 0x2002,
	AF_CMD_GET_AF_INIT_POS = 0x2003,
	AF_CMD_GET_MULTI_WIN_CFG = 0x2004,
	AF_CMD_GET_AF_LIB_INFO = 0x2005,
	AF_CMD_GET_AF_FULLSCAN_INFO = 0x2006,
};

enum af_calc_data_type {
	AF_DATA_AF,
	AF_DATA_IMG_BLK,
	AF_DATA_AE,
	AF_DATA_FD,
	AF_DATA_MAX
};

enum af_locker_type {
	AF_LOCKER_AE,
	AF_LOCKER_AWB,
	AF_LOCKER_LSC,
	AF_LOCKER_NLM,
	AF_LOCKER_MAX
};
/* used for af1.0 */
/*
struct af_plat_info {
	cmr_u32 afm_filter_type_cnt;
	cmr_u32 afm_win_max_cnt;
	cmr_u32 isp_w;
	cmr_u32 isp_h;
};

struct af_tuning_param {
	cmr_u8 *data;
	cmr_u32 data_len;
	cmr_u32 cfg_mode;
};

struct af_filter_data {
	cmr_u32 type;
	cmr_u64 *data;
};

struct af_filter_info {
	cmr_u32 filter_num;
	struct af_filter_data *filter_data;
};
*/
struct ae_out_bv {
	struct ae_calc_out *ae_result;
	cmr_s32 bv;
};

struct af_motor_pos {
	cmr_u32 motor_pos;
	cmr_u32 skip_frame;
	cmr_u32 wait_time;
};

struct af_result_param {
	cmr_u32 motor_pos;
	cmr_u32 suc_win;
};

struct af_monitor_set {
	cmr_u32 type;
	cmr_u32 bypass;
	cmr_u32 int_mode;
	cmr_u32 skip_num;
	cmr_u32 need_denoise;
};

struct af_win_rect {
	cmr_u32 sx;
	cmr_u32 sy;
	cmr_u32 ex;
	cmr_u32 ey;
};

struct af_monitor_win {
	cmr_u32 type;
	struct af_win_rect *win_pos;
};

struct af_trig_info {
	cmr_u32 win_num;
	cmr_u32 mode;
	struct af_win_rect win_pos[MAX_AF_WINS];
};

struct afctrl_init_in {
//      cmr_u32 af_bypass;
	void *caller;
//      cmr_u32 af_mode;
//      cmr_u32 tuning_param_cnt;
//      cmr_u32 cur_tuning_mode;
	cmr_u32 camera_id;
	isp_af_cb af_set_cb;
	cmr_handle caller_handle;
	struct third_lib_info lib_param;
//      struct af_plat_info plat_info;
//      struct af_tuning_param *tuning_param;
	struct isp_size src;
	 cmr_s32(*go_position) (void *handle, struct af_motor_pos * in_param);
	 cmr_s32(*start_notice) (void *handle);
	 cmr_s32(*end_notice) (void *handle, struct af_result_param * in_param);
	 cmr_s32(*set_monitor) (void *handle, struct af_monitor_set * in_param, cmr_u32 cur_envi);
	 cmr_s32(*set_monitor_win) (void *handle, struct af_monitor_win * in_param);
	 cmr_s32(*get_monitor_win_num) (void *handle, cmr_u32 * win_num);
//      cmr_s32(*ae_awb_lock) (void* handle);
//      cmr_s32(*ae_awb_release) (void* handle);
	 cmr_s32(*lock_module) (void *handle, cmr_int af_locker_type);
	 cmr_s32(*unlock_module) (void *handle, cmr_int af_locker_type);
};

struct afctrl_init_out {
	cmr_u32 init_motor_pos;
};

struct af_img_blk_info {
	cmr_u32 block_w;
	cmr_u32 block_h;
	cmr_u32 pix_per_blk;
	cmr_u32 chn_num;
	cmr_u32 *data;
};

struct af_ae_info {
	cmr_u32 exp_time;	//us
	cmr_u32 gain;	//256 --> 1X
	cmr_u32 cur_fps;
	cmr_u32 cur_lum;
	cmr_u32 target_lum;
	cmr_u32 is_stable;
};

struct afctrl_calc_in {
	cmr_u32 data_type;
	void *data;
	struct isp_sensor_fps_info sensor_fps;
};

struct afctrl_calc_out {
	cmr_u32 motor_pos;
	cmr_u32 suc_win;
};

struct afctrl_work_lib {
	cmr_handle lib_handle;
	struct adpt_ops_type *adpt_ops;
};

struct afctrl_cxt {
	cmr_handle thr_handle;
	cmr_handle caller_handle;
	struct afctrl_work_lib work_lib;
	struct afctrl_calc_out proc_out;
	isp_af_cb af_set_cb;
};

#define AREA_LOOP 4

struct pd_result {
	/*TBD get reset from */
	cmr_s32 pdConf[AREA_LOOP + 1];
	double pdPhaseDiff[AREA_LOOP + 1];
	cmr_s32 pdGetFrameID;
};

cmr_int af_ctrl_init(struct afctrl_init_in *input_ptr, cmr_handle * handle_af);
cmr_int af_ctrl_deinit(cmr_handle * handle_af);
cmr_int af_ctrl_process(cmr_handle handle_af, void *in_ptr, struct afctrl_calc_out *result);
cmr_int af_ctrl_ioctrl(cmr_handle handle_af, cmr_int cmd, void *in_ptr, void *out_ptr);

#ifdef	 __cplusplus
}
#endif

#endif
