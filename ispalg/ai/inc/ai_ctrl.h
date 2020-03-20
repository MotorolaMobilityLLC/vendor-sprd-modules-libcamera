/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _AI_CTRL_H_
#define _AI_CTRL_H_

#include "isp_common_types.h"
#include "ai_ctrl_types.h"
#include "isp_mw.h"
#include "isp_adpt.h"

#ifdef __cplusplus
extern "C" {
#endif

	struct ai_init_in {
		cmr_handle caller_handle;
		struct aictrl_ops ai_ops;
		isp_ai_cb ai_set_cb;
		struct third_lib_info lib_param;
		cmr_u32 cameraId;
	};

	struct ai_init_out {
		cmr_handle param;
	};

	cmr_s32 ai_ctrl_init(struct ai_init_in *input_ptr, cmr_handle *handle_ai, cmr_handle result);
	cmr_int ai_ctrl_deinit(cmr_handle * handle_ai);
	cmr_int ai_ctrl_ioctrl(cmr_handle handle, enum ai_io_ctrl_cmd cmd, cmr_handle in_ptr, cmr_handle out_ptr);

#ifdef __cplusplus
}
#endif
#endif
