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
#define LOG_TAG "isp_blk_contrast"
#include "isp_blocks_cfg.h"

cmr_s32 _pm_contrast_init(void *dst_contrast, void *src_contrast, void *param1, void *param2)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct sensor_contrast_param *src_ptr = (struct sensor_contrast_param *)src_contrast;
	struct isp_contrast_param *dst_ptr = (struct isp_contrast_param *)dst_contrast;
	struct isp_pm_block_header *contrast_header_ptr = (struct isp_pm_block_header *)param1;
	UNUSED(param2);

	dst_ptr->cur_index = src_ptr->cur_index;
	dst_ptr->cur.bypass = contrast_header_ptr->bypass;
	dst_ptr->cur.factor = src_ptr->factor[src_ptr->cur_index];
	memcpy((void *)dst_ptr->tab, (void *)src_ptr->factor, sizeof(dst_ptr->tab));
	memcpy((void *)dst_ptr->scene_mode_tab, (void *)src_ptr->scenemode, sizeof(dst_ptr->scene_mode_tab));
	contrast_header_ptr->is_update = ISP_ONE;

	return rtn;
}

cmr_s32 _pm_contrast_set_param(void *contrast_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_contrast_param *contrast_ptr = (struct isp_contrast_param *)contrast_param;
	struct isp_pm_block_header *contrast_header_ptr = (struct isp_pm_block_header *)param_ptr1;

	contrast_header_ptr->is_update = ISP_ONE;

	switch (cmd) {
	case ISP_PM_BLK_CONTRAST_BYPASS:
		contrast_ptr->cur.bypass = *((cmr_u32 *) param_ptr0);
		break;

	case ISP_PM_BLK_CONTRAST:
		contrast_ptr->cur_index = *((cmr_u32 *) param_ptr0);
		contrast_ptr->cur.factor = contrast_ptr->tab[contrast_ptr->cur_index];
		break;

	case ISP_PM_BLK_SCENE_MODE:
		{
			cmr_u32 idx = *((cmr_u32 *) param_ptr0);
			if (0 == idx) {
				contrast_ptr->cur.factor = contrast_ptr->tab[contrast_ptr->cur_index];
			} else {
				contrast_ptr->cur.factor = contrast_ptr->scene_mode_tab[idx];
			}
		}
		break;

	default:
		contrast_header_ptr->is_update = ISP_ZERO;
		break;
	}

	return rtn;
}

cmr_s32 _pm_contrast_get_param(void *contrast_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_contrast_param *contrast_ptr = (struct isp_contrast_param *)contrast_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	cmr_u32 *update_flag = (cmr_u32 *) rtn_param1;

	param_data_ptr->id = ISP_BLK_CONTRAST;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
	{
		int j = 0;
		int ygam_node[257] = {0};

		if (contrast_ptr->cur.factor == 64)
			contrast_ptr->gamma_info.bypass = 1;
		else
			contrast_ptr->gamma_info.bypass = contrast_ptr->cur.bypass;

		if (0 == contrast_ptr->gamma_info.bypass) {
			for(j = 0; j <= 255; j++)
			{
				ygam_node[j] = (((j - 128)*(cmr_s32)contrast_ptr->cur.factor)>> 6) + 128;
				ygam_node[j] = ygam_node[j] < 0 ? 0 : ygam_node[j] > 255 ? 255 : ygam_node[j];
			}
			ygam_node[256] = ygam_node[255];

			for (j = 0; j < ISP_PINGPANG_YUV_YGAMMA_NUM; j++) {
				if (j < ISP_PINGPANG_YUV_YGAMMA_NUM - 1) {
					contrast_ptr->gamma_info.nodes[j].node_y = (ygam_node[j * 2] + ygam_node[j * 2 + 1]) >> 1;
				} else {
					contrast_ptr->gamma_info.nodes[j].node_y = ygam_node[j * 2 - 1];
				}
			}
		}
		param_data_ptr->data_ptr = (void *)&contrast_ptr->gamma_info;
		param_data_ptr->data_size = sizeof(contrast_ptr->gamma_info);
		*update_flag = 0;
		break;
	}

	case ISP_PM_BLK_CONTRAST_BYPASS:
		param_data_ptr->data_ptr = (void *)&contrast_ptr->cur.bypass;
		param_data_ptr->data_size = sizeof(contrast_ptr->cur.bypass);
		break;

	default:
		break;
	}

	return rtn;
}
