/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef _AFL_CTRL_H_
#define _AFL_CTRL_H_

#include "isp_com.h"

struct afl_ctrl_init_in {
	cmr_handle dev_handle;
	cmr_handle vir_addr;
	cmr_handle vir_addr_region;
	cmr_u32 camera_id;
	struct isp_size size;
	isp_afl_cb afl_set_cb;
	cmr_handle caller_handle;
	cmr_s8 version;
};

enum afl_io_ctrl_cmd {
	AFL_GET_INFO = 0x00,
	AFL_SET_BYPASS,
	AFL_NEW_SET_BYPASS,
	AFL_SET_IMG_SIZE,
	AFL_SET_MAX_FPS,
};

struct afl_ae_stat_win_num {
	cmr_u32 w;
	cmr_u32 h;
};

struct afl_proc_in {
	cmr_u32 cur_flicker;
	cmr_u32 cur_exp_flag;
	cmr_s32 ae_exp_flag;
	cmr_s32 pm_param_num;
	cmr_uint vir_addr;
	cmr_uint vir_addr_region;
	struct isp_awb_statistic_info *ae_stat_ptr;
	struct isp_antiflicker_param *afl_param_ptr;
	cmr_u32 afl_mode;
	void *private_data;
	cmr_u32 private_len;
	struct afl_ae_stat_win_num ae_win_num;
	cmr_u32 max_fps;
	cmr_u32 app_mode;
};

struct afl_ctrl_param_in {
	union {
		struct isp_size img_size;
	};
};

cmr_int afl_ctrl_init(cmr_handle *isp_afl_handle, struct afl_ctrl_init_in *input_ptr);
cmr_int afl_ctrl_process(cmr_handle isp_afl_handle, struct afl_proc_in *in_ptr, struct afl_ctrl_proc_out *out_ptr);
cmr_int afl_ctrl_cfg(cmr_handle isp_afl_handle);
cmr_int aflnew_ctrl_cfg(cmr_handle isp_afl_handle);
cmr_int afl_ctrl_deinit(cmr_handle *isp_afl_handle);
cmr_int afl_ctrl_ioctrl(cmr_handle handle, enum afl_io_ctrl_cmd cmd, void *in_ptr, void *out_ptr);

#endif
