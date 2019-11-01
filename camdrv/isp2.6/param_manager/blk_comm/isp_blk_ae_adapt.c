/*
 * Copyright (C) 2018 The Android Open Source Project
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
#define LOG_TAG "isp_blk_ae_adapt"
#include "isp_blocks_cfg.h"

cmr_s32 _pm_ae_adapt_init(void *dst_ae_adapt, void *src_ae_adapt, void *param1, void *param2)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_ae_adapt_param *dst_ptr = (struct isp_ae_adapt_param *)dst_ae_adapt;
	struct sensor_ae_adapt_param *src_ptr = (struct sensor_ae_adapt_param *)src_ae_adapt;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param1;
	UNUSED(param2);

	dst_ptr->binning_factor = src_ptr->binning_factor;

	header_ptr->is_update = ISP_ONE;

	return rtn;
}

cmr_s32 _pm_ae_adapt_set_param(void *ae_adapt_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	UNUSED(ae_adapt_param);
	UNUSED(cmd);
	UNUSED(param_ptr0);
	UNUSED(param_ptr1);

	return rtn;
}

cmr_s32 _pm_ae_adapt_get_param(void *ae_adapt_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_ae_adapt_param *ae_adapt_ptr = (struct isp_ae_adapt_param *)ae_adapt_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	cmr_u32 *update_flag = (cmr_u32 *) rtn_param1;

	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = &ae_adapt_ptr->binning_factor;
		param_data_ptr->data_size = sizeof(ae_adapt_ptr->binning_factor);
		*update_flag = ISP_ZERO;
		break;

	default:
		break;
	}

	return rtn;
}
