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
	struct sensor_contrast_ai_param *src_ptr = (struct sensor_contrast_ai_param *)src_contrast;
	struct isp_contrast_param *dst_ptr = (struct isp_contrast_param *)dst_contrast;
	struct isp_pm_block_header *contrast_header_ptr = (struct isp_pm_block_header *)param1;
	UNUSED(param2);

	dst_ptr->cur_index = src_ptr->cur_index;
	dst_ptr->cur.bypass = contrast_header_ptr->bypass;
	dst_ptr->cur.factor = src_ptr->factor[src_ptr->cur_index];
	memcpy((void *)dst_ptr->tab, (void *)src_ptr->factor, sizeof(dst_ptr->tab));
	memcpy((void *)dst_ptr->scene_mode_tab, (void *)src_ptr->scenemode, sizeof(dst_ptr->scene_mode_tab));
	contrast_header_ptr->is_update = ISP_ONE;

	ISP_LOGD("init contrast, index:%d cur_factor:%d",
		 dst_ptr->cur_index,
		 dst_ptr->cur.factor);

	return rtn;
}

cmr_s32 _pm_contrast_set_param(void *contrast_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_contrast_param *contrast_ptr = (struct isp_contrast_param *)contrast_param;
	struct isp_pm_block_header *contrast_header_ptr = (struct isp_pm_block_header *)param_ptr1;

	contrast_header_ptr->is_update = ISP_ONE;

	switch (cmd) {
	case ISP_PM_BLK_SMART_SETTING:
		{
			struct smart_block_result *block_result =
				(struct smart_block_result *)param_ptr0;
			struct isp_weight_value *weight_value = NULL;
			struct isp_weight_value *bv_value = NULL;
			void *dst = NULL;
			void *src_map[2] = {NULL, NULL};
			cmr_u16 weight[2] = {0, 0};
			cmr_u32 data_num = 0;
			struct isp_range val_range = {0, 0};

			if (0 == block_result->update) {
				ISP_LOGV("do not need update\n");
				return ISP_SUCCESS;
			}

			val_range.min = 0;
			val_range.max = 255;
			rtn = _pm_check_smart_param(block_result, &val_range, 1,
						ISP_SMART_Y_TYPE_WEIGHT_VALUE);
			if (ISP_SUCCESS != rtn) {
				ISP_LOGE("fail to check pm smart param !");
				return rtn;
			}

			dst = &contrast_ptr->cur.factor;
			data_num = sizeof(cmr_u8);
			weight_value = (struct isp_weight_value *)
				block_result->component[0].fix_data;
			bv_value = &weight_value[0];

			src_map[0] = (void *)&contrast_ptr->tab[bv_value->value[0]];
			src_map[1] = (void *)&contrast_ptr->tab[bv_value->value[1]];
			weight[0] = bv_value->weight[0];
			weight[1] = bv_value->weight[1];
			weight[0] = weight[0] / (SMART_WEIGHT_UNIT / 16) *
						(SMART_WEIGHT_UNIT / 16);
			weight[1] = SMART_WEIGHT_UNIT - weight[0];
			isp_interp_data(dst, src_map, weight, data_num,
					ISP_INTERP_UINT8);
		}
		break;

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

	ISP_LOGD("set contrast param, factor:%d index:%d",
		contrast_ptr->cur.factor,
		contrast_ptr->cur_index);

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
		param_data_ptr->data_ptr = (void *)&contrast_ptr->cur;
		param_data_ptr->data_size = sizeof(contrast_ptr->cur);
		*update_flag = 0;
		break;

	case ISP_PM_BLK_CONTRAST_BYPASS:
		param_data_ptr->data_ptr = (void *)&contrast_ptr->cur.bypass;
		param_data_ptr->data_size = sizeof(contrast_ptr->cur.bypass);
		break;

	default:
		break;
	}

	return rtn;
}
