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
#define LOG_TAG "isp_blk_rgb_aem"
#include "isp_blocks_cfg.h"

cmr_s32 _pm_rgb_aem_init(void *dst_rgb_aem, void *src_rgb_aem, void *param1, void *param2)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_rgb_aem_param *dst_ptr = (struct isp_rgb_aem_param *)dst_rgb_aem;
	struct sensor_rgb_aem_param *src_ptr = (struct sensor_rgb_aem_param *)src_rgb_aem;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param1;
	UNUSED(param2);

	dst_ptr->cur.blk_num.w = src_ptr->blk_num.w;
	dst_ptr->cur.blk_num.h = src_ptr->blk_num.h;

	header_ptr->is_update = ISP_ONE;

	return rtn;
}

cmr_s32 _pm_rgb_aem_set_param(void *rgb_aem_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	UNUSED(rgb_aem_param);
	UNUSED(cmd);
	UNUSED(param_ptr0);
	UNUSED(param_ptr1);

	return rtn;
}

cmr_s32 _pm_rgb_aem_get_param(void *rgb_aem_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_rgb_aem_param *rgb_aem_ptr = (struct isp_rgb_aem_param *)rgb_aem_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	UNUSED(rtn_param1);

	param_data_ptr->id = ISP_BLK_RGB_AEM;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = &rgb_aem_ptr->cur;
		param_data_ptr->data_size = sizeof(rgb_aem_ptr->cur);
		break;

	default:
		break;
	}

	return rtn;
}
