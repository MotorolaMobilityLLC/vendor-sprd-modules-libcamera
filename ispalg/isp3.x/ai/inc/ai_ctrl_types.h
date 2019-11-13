/*
 * Copyright (C) 2012 The Android Open Source Project
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

#ifndef _AI_CTRL_TYPES_H_
#define _AI_CTRL_TYPES_H_

#include "isp_common_types.h"
#include "isp_mw.h"
#include "isp_com.h"

#ifdef __cplusplus
extern "C" {
#endif

	enum ai_io_ctrl_cmd {
		/*
		 * warning if you wanna send async msg
		 * please add msg id below here
		 */
		AI_SYNC_MSG_BEGIN = 0x00,
		AI_SET_FD_PARAM,
		AI_SET_AE_PARAM,
		AI_SET_IMG_PARAM,
		AI_PROCESS_START,
		AI_PROCESS_STOP,
		AI_SET_FD_ON_OFF,
		AI_SET_FLASH_NOTICE,
		AI_SYNC_MSG_END,
		/*
		 * warning if you wanna set ioctrl directly
		 * please add msg id below here
		 */
		AI_DIRECT_MSG_BEGIN,
		AI_GET_IMG_FLAG,
		AI_GET_STATUS,
		AI_GET_DEBUG_INFO,
		AI_DIRECT_MSG_END,
		AI_IO_MAX
	};

	enum ai_cb_type {
		AI_CB_FOR_HAL,
		AI_CB_FOR_AE,
		AI_CB_FOR_AWB,
		AI_CB_FOR_AF,
		AI_CB_MAX
	};

	struct aictrl_ops {
		cmr_int(*callback) (cmr_handle ai_handle, cmr_int cb_type, cmr_handle param);
	};

	struct aictrl_work_lib {
		cmr_handle lib_handle;
		struct adpt_ops_type *adpt_ops;
		isp_ai_cb ai_set_cb;
	};

	struct aictrl_cxt {
		cmr_handle thr_handle;
		cmr_handle caller_handle;
		isp_ai_cb ai_set_cb;
		struct aictrl_work_lib work_lib;
		struct aictrl_ops ai_ops;
		struct ai_scene_detect_info scene_info;
		cmr_u32 cameraId;
	};


#ifdef __cplusplus
}
#endif
#endif
