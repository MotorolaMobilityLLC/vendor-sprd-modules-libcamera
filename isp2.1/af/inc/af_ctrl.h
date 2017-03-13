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
/*------------------------------------------------------------------------------*
*					Dependencies				*
*-------------------------------------------------------------------------------*/
#include <sys/types.h>
#include "isp_com.h"
#include "af_sprd_ctrl.h"
#include "sprd_af_ctrl_v1.h"

#include "isp_alg_fw.h"

#ifdef WIN32
#include "sci_type.h"
#endif

/*------------------------------------------------------------------------------*
*					Compiler Flag				*
*-------------------------------------------------------------------------------*/
#ifdef  __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------*
*					Data Prototype				*
*-------------------------------------------------------------------------------*/
enum af_mode{
	AF_MODE_NORMAL=0x00,
	AF_MODE_MACRO,
	AF_MODE_CONTINUE,
	AF_MODE_VIDEO,
	AF_MODE_MANUAL,
	AF_MODE_PICTURE,
	AF_MODE_MAX
};

enum af_cmd {
	AF_CMD_SET_BASE             = 0x1000,
	AF_CMD_SET_AF_MODE          = 0x1001,
	AF_CMD_SET_AF_POS           = 0x1002,
	AF_CMD_SET_TUNING_MODE      = 0x1003,
	AF_CMD_SET_SCENE_MODE       = 0x1004,
	AF_CMD_SET_AF_START         = 0x1005,
	AF_CMD_SET_AF_STOP          = 0x1006,
	AF_CMD_SET_AF_RESTART       = 0x1007,
	AF_CMD_SET_CAF_RESET        = 0x1008,
	AF_CMD_SET_CAF_STOP         = 0x1009,
	AF_CMD_SET_AF_FINISH        = 0x100A,
	AF_CMD_SET_AF_BYPASS        = 0x100B,
	AF_CMD_SET_DEFAULT_AF_WIN   = 0x100C,
	AF_CMD_SET_FLASH_NOTICE     = 0x100D,
	AF_CMD_SET_ISP_START_INFO   = 0x100E,
	AF_CMD_SET_ISP_STOP_INFO    = 0x100F,
	AF_CMD_SET_ISP_TOOL_AF_TEST = 0x1010,
	AF_CMD_SET_CAF_TRIG_START   = 0x1011,
	AF_CMD_SET_AE_INFO          = 0x1012,
	AF_CMD_SET_AWB_INFO         = 0x1013,
	AF_CMD_SET_FACE_DETECT      = 0x1014,
	AF_CMD_SET_DCAM_TIMESTAMP   = 0x1015,
	AF_CMD_SET_PD_INFO	    = 0x1016,
	AF_CMD_SET_UPDATE_AUX_SENSOR = 0x1017,

	AF_CMD_GET_BASE             = 0x2000,
	AF_CMD_GET_AF_MODE          = 0X2001,
	AF_CMD_GET_AF_CUR_POS       = 0x2002,
	AF_CMD_GET_AF_INIT_POS      = 0x2003,
	AF_CMD_GET_MULTI_WIN_CFG    = 0x2004,
	AF_CMD_GET_AF_LIB_INFO      = 0x2005,
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

struct afctrl_init_in{
	uint32_t af_bypass;
	void* caller;
	uint32_t af_mode;
	uint32_t tuning_param_cnt;
	uint32_t cur_tuning_mode;
	uint32_t camera_id;
	isp_af_cb af_set_cb;
	cmr_handle caller_handle;
	struct third_lib_info lib_param;
	struct af_plat_info plat_info;
	struct af_tuning_param *tuning_param;
	struct isp_size src;
	int32_t(*go_position) (void* handle,struct af_motor_pos* in_param);
	int32_t(*end_notice) (void* handle,struct af_result_param* in_param);
	int32_t(*start_notice) (void* handle);
	int32_t(*set_monitor) (void* handle, struct af_monitor_set* in_param, uint32_t cur_envi);
	int32_t(*set_monitor_win) (void* handle, struct af_monitor_win* in_param);
	int32_t(*get_monitor_win_num) (void* handle, uint32_t *win_num);
	int32_t(*ae_awb_lock) (void* handle);
	int32_t(*ae_awb_release) (void* handle);
	int32_t(*lock_module) (void* handle, cmr_int af_locker_type);
	int32_t(*unlock_module) (void* handle, cmr_int af_locker_type);
};

struct afctrl_init_out {
	uint32_t init_motor_pos;
};

struct afctrl_calc_in {
	uint32_t data_type;
	void* data;
	struct isp_sensor_fps_info sensor_fps;
};

struct afctrl_calc_out{
	uint32_t motor_pos;
	uint32_t suc_win;
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

cmr_int af_ctrl_init(struct afctrl_init_in *input_ptr, cmr_handle *handle_af);
cmr_int af_ctrl_deinit(cmr_handle *handle_af);
cmr_int af_ctrl_process(cmr_handle handle_af, void *in_ptr, struct afctrl_calc_out *result);
cmr_int af_ctrl_ioctrl(cmr_handle handle_af, cmr_int cmd, void *in_ptr, void *out_ptr);
/*------------------------------------------------------------------------------*
*					Compiler Flag				*
*-------------------------------------------------------------------------------*/
#ifdef	 __cplusplus
}
#endif
/*-----------------------------------------------------------------------------*/
#endif
// End
