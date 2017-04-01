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




 cmr_s32 _pm_hist_init_v1(void *dst_hist_param, void *src_hist_param, void *param1, void *param2)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param1;
	struct sensor_yuv_hists_param *src_ptr = (struct sensor_yuv_hists_param *)src_hist_param;
	struct isp_hist_param_v1 *dst_ptr = (struct isp_hist_param_v1 *)dst_hist_param;
	UNUSED(param2);

	dst_ptr->cur.bypass = header_ptr->bypass;

	dst_ptr->cur.skip_num = src_ptr->hist_skip_num;
	dst_ptr->cur.mode = src_ptr->hist_mode_sel;

	header_ptr->is_update = ISP_ONE;

	return rtn;
}

 cmr_s32 _pm_hist_set_param_v1(void *hist_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_pm_block_header *hist_header_ptr = (struct isp_pm_block_header *)param_ptr1;
	struct isp_hist_param_v1 *hist_ptr = (struct isp_hist_param_v1 *)hist_param;
	hist_header_ptr->is_update = ISP_ONE;

	switch (cmd) {
	case ISP_PM_BLK_HIST_BYPASS_V1:
		hist_ptr->cur.bypass = *((cmr_u32*)param_ptr0);
	break;

	default:
		hist_header_ptr->is_update = ISP_ZERO;
	break;
	}

	return rtn;
}

 cmr_s32 _pm_hist_get_param_v1(void *hist_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_hist_param_v1 *hist_ptr = (struct isp_hist_param_v1 *)hist_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	cmr_u32 *update_flag = (cmr_u32*)rtn_param1;

	param_data_ptr->id = ISP_BLK_HIST;
	param_data_ptr->cmd = ISP_PM_BLK_ISP_SETTING;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = &hist_ptr->cur;
		param_data_ptr->data_size = sizeof(hist_ptr->cur);
		*update_flag = ISP_ZERO;
	break;

	default:
	break;
	}
	return rtn;
}
