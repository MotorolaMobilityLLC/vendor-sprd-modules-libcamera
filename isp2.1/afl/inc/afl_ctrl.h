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

#ifdef __cplusplus
extern "C"
{
#endif

struct afl_ctrl_init_in {
	cmr_handle dev_handle;
	cmr_handle vir_addr;
	struct isp_size size;
};

struct afl_proc_in {
	uint32_t cur_flicker;
	uint32_t cur_exp_flag;
	int32_t ae_exp_flag;
	uint32_t vir_addr;
	struct isp_awb_statistic_info *ae_stat_ptr;
};

cmr_int afl_ctrl_init(cmr_handle *isp_afl_handle, struct afl_ctrl_init_in *input_ptr);
cmr_int afl_ctrl_process(cmr_handle isp_afl_handle, struct afl_proc_in *in_ptr, struct afl_ctrl_proc_out *out_ptr);
cmr_int afl_ctrl_cfg(cmr_handle isp_afl_handle);
cmr_int afl_ctrl_deinit(cmr_handle isp_afl_handle);

#ifdef __cplusplus
}
#endif

#endif
