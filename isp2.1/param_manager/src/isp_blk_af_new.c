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

#include "isp_blocks_cfg.h"


isp_s32 _pm_af_new_init(void *dst_af_new, void *src_af_new, void* param1, void* param2)
{
	isp_s32 rtn = ISP_SUCCESS;
	UNUSED(param2);

	return rtn;
}

isp_s32 _pm_af_new_set_param(void *af_new_param, isp_u32 cmd, void* param_ptr0, void* param_ptr1)
{
	isp_s32 rtn = ISP_SUCCESS;
	UNUSED(cmd);

	return rtn;
}

isp_s32 _pm_af_new_get_param(void *af_new_param, isp_u32 cmd, void* rtn_param0, void* rtn_param1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data*)rtn_param0;
	isp_u32 *update_flag =(isp_u32*)rtn_param1;

	param_data_ptr->id = ISP_BLK_AF_NEW;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
	break;

	default:
	break;
	}

	return rtn;
}
