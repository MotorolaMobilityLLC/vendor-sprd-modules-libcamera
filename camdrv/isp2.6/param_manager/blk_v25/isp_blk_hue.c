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
#define LOG_TAG "isp_blk_hue"
#include "isp_blocks_cfg.h"

cmr_s32 _pm_hue_init(void *dst_hue_param, void *src_hue_param, void *param1, void *param_ptr2)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_hue_param_v0 *dst_hue_ptr = (struct isp_hue_param_v0 *)dst_hue_param;
	struct sensor_hue_param *src_hue_ptr = (struct sensor_hue_param *)src_hue_param;
	struct isp_pm_block_header *hue_header_ptr = (struct isp_pm_block_header *)param1;
	UNUSED(param_ptr2);

	dst_hue_ptr->cur.bypass = hue_header_ptr->bypass;
	if (src_hue_ptr->cur_index < 16) {
		dst_hue_ptr->cur.theta = src_hue_ptr->hue_theta[src_hue_ptr->cur_index];
	} else {
		ISP_LOGE("fail to make subscript not out of bounds-array");
	}
	hue_header_ptr->is_update = ISP_ONE;

	return rtn;
}

cmr_s32 _pm_hue_set_param(void *hue_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_hue_param_v0 *hue_ptr = (struct isp_hue_param_v0 *)hue_param;
	struct isp_pm_block_header *hue_header_ptr = (struct isp_pm_block_header *)param_ptr1;

	switch (cmd) {
	case ISP_PM_BLK_HUE_BYPASS:
		hue_ptr->cur.bypass = *((cmr_u32 *) param_ptr0);
		hue_header_ptr->is_update = ISP_ONE;
		break;

	case ISP_PM_BLK_HUE:
		{
			cmr_u32 level = *((cmr_u32 *) param_ptr0);
			hue_ptr->cur_idx = level;
			hue_ptr->cur.theta = hue_ptr->tab[hue_ptr->cur_idx];
			hue_header_ptr->is_update = ISP_ONE;
		}
		break;

	case ISP_PM_BLK_AI_SCENE_UPDATE_BCHS:
		{
			cmr_s16 smooth_factor, smooth_base, ai_status;
			struct isp_ai_update_param *cfg_data;
			struct isp_ai_bchs_param *bchs_cur;
			cmr_s32 theta;
			cmr_u32 ai_scene = 0;
			cmr_u32 count = 0;

			cfg_data = (struct isp_ai_update_param *)param_ptr0;
			bchs_cur = (struct isp_ai_bchs_param *)cfg_data->param_ptr;
			smooth_factor = cfg_data->smooth_factor;
			smooth_base = cfg_data->smooth_base;
			ai_status = cfg_data->ai_status;
			ai_scene = cfg_data->ai_scene;
			count = cfg_data->count;
			if (smooth_factor == 0)
				break;

			if (ai_status){
				if (!ai_scene){
					theta = hue_ptr->cur.theta;
					if (bchs_cur->ai_hue_v1.hue_adj_ai_eb || smooth_factor) {
						theta += bchs_cur->ai_hue_v1.theta_offset * smooth_factor / smooth_base;
						theta = MAX(-180, MIN(180,  theta));
						if ((theta < hue_ptr->tab[hue_ptr->cur_idx]) ||  (count == smooth_base))
							theta = hue_ptr->tab[hue_ptr->cur_idx];
					}
					hue_ptr->cur.theta = theta;
				} else {
					theta = hue_ptr->tab[hue_ptr->cur_idx];
					if (bchs_cur->ai_hue_v1.hue_adj_ai_eb || smooth_factor) {
						theta += bchs_cur->ai_hue_v1.theta_offset * smooth_factor / smooth_base;
						theta = MAX(-180, MIN(180,  theta));
					}
					hue_ptr->cur.theta = theta;
				}
			} else {
				hue_ptr->cur.theta = hue_ptr->tab[hue_ptr->cur_idx];
			}
			hue_header_ptr->is_update = ISP_ONE;
		}
		break;

	default:
		hue_header_ptr->is_update = ISP_ZERO;
		break;
	}

	return rtn;
}

cmr_s32 _pm_hue_get_param(void *hue_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_hue_param_v0 *hue_ptr = (struct isp_hue_param_v0 *)hue_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	cmr_u32 *update_flag = (cmr_u32 *) rtn_param1;

	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = (void *)&hue_ptr->cur;
		param_data_ptr->data_size = sizeof(hue_ptr->cur);
		*update_flag = 0;
		break;

	default:
		break;
	}

	return rtn;
}
