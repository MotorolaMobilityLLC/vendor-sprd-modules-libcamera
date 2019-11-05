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
#define LOG_TAG "isp_blk_saturation"
#include "isp_blocks_cfg.h"

cmr_s32 _pm_saturation_init(void *dst_csa_param, void *src_csa_param, void *param1, void *param_ptr2)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 i = 0, j = 0;
	struct isp_chrom_saturation_param *dst_csa_ptr = (struct isp_chrom_saturation_param *)dst_csa_param;
	struct sensor_saturation_ai_param *src_csa_ptr = (struct sensor_saturation_ai_param *)src_csa_param;
	struct isp_pm_block_header *csa_header_ptr = (struct isp_pm_block_header *)param1;
	UNUSED(param_ptr2);

	dst_csa_ptr->cur.bypass = csa_header_ptr->bypass;
	dst_csa_ptr->cur.factor_u = src_csa_ptr->csa_factor_u[src_csa_ptr->index_u];
	dst_csa_ptr->cur.factor_v = src_csa_ptr->csa_factor_v[src_csa_ptr->index_v];
	dst_csa_ptr->cur_u_idx = src_csa_ptr->index_u;
	dst_csa_ptr->cur_v_idx = src_csa_ptr->index_v;
	for (i = 0; i < SENSOR_SATURATION_NUM; i++) {
		dst_csa_ptr->tab[0][i] = src_csa_ptr->csa_factor_u[i];
		dst_csa_ptr->tab[1][i] = src_csa_ptr->csa_factor_v[i];
	}
	for (i = 0; i < MAX_SCENEMODE_NUM; i++) {
		dst_csa_ptr->scene_mode_tab[0][i] = src_csa_ptr->scenemode[0][i];
		dst_csa_ptr->scene_mode_tab[1][i] = src_csa_ptr->scenemode[1][i];
	}

	csa_header_ptr->is_update = ISP_ONE;

	ISP_LOGV("init saturation, index u,v:%d,%d cur_factor u,v:%d,%d",
		 dst_csa_ptr->cur_u_idx,
		 dst_csa_ptr->cur_v_idx,
		 dst_csa_ptr->cur.factor_u,
		 dst_csa_ptr->cur.factor_v);

	return rtn;
}

cmr_s32 _pm_saturation_set_param(void *csa_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_chrom_saturation_param *csa_ptr = (struct isp_chrom_saturation_param *)csa_param;
	struct isp_pm_block_header *csa_header_ptr = (struct isp_pm_block_header *)param_ptr1;

	switch (cmd) {
	case ISP_PM_BLK_SATURATION_BYPASS:
		csa_ptr->cur.bypass = *((cmr_u32 *) param_ptr0);
		csa_header_ptr->is_update = ISP_ONE;
		break;

	case ISP_PM_BLK_SATURATION:
		{
			cmr_u32 level = *((cmr_u32 *) param_ptr0);
			csa_ptr->cur_u_idx = level;
			csa_ptr->cur_v_idx = level;
			csa_ptr->cur.factor_u = csa_ptr->tab[0][csa_ptr->cur_u_idx];
			csa_ptr->cur.factor_v = csa_ptr->tab[1][csa_ptr->cur_v_idx];
			csa_header_ptr->is_update = ISP_ONE;
		}
		break;

	case ISP_PM_BLK_SCENE_MODE:
		{
			cmr_u32 idx = *((cmr_u32 *) param_ptr0);
			if (0 == idx) {
				csa_ptr->cur.factor_u = csa_ptr->tab[0][csa_ptr->cur_u_idx];
				csa_ptr->cur.factor_v = csa_ptr->tab[1][csa_ptr->cur_v_idx];
			} else {
				csa_ptr->cur.factor_u = csa_ptr->scene_mode_tab[0][idx];
				csa_ptr->cur.factor_v = csa_ptr->scene_mode_tab[1][idx];
			}
			csa_header_ptr->is_update = ISP_ONE;
		}
		break;

	case ISP_PM_BLK_SMART_SETTING:
		{
			struct smart_block_result *block_result =
				(struct smart_block_result *)param_ptr0;
			struct isp_range val_range = { 0, 0 };
			cmr_u32 reduce_percent = 0;
			cmr_u32 factor_u = 0;
			cmr_u32 factor_v = 0;
			cmr_u32 smart_id = 0;

			struct isp_weight_value *weight_value = NULL;
			struct isp_weight_value *bv_value = NULL;
			void *dst = NULL;
			void *dst1 = NULL;
			void *src_mapu[2] = {NULL, NULL};
			void *src_mapv[2] = {NULL, NULL};
			cmr_u16 weight[2] = {0, 0};
			cmr_u32 data_num = 0;

			val_range.min = 0;
			val_range.max = 255;

			if (0 == block_result->update) {
				ISP_LOGV("do not need update\n");
				return ISP_SUCCESS;
			}

			smart_id = block_result->smart_id;
			ISP_LOGD("check smart_id:%d", smart_id);

			switch(smart_id){
			case ISP_SMART_SATURATION_DEPRESS:
				rtn = _pm_check_smart_param(block_result, &val_range, 1, ISP_SMART_Y_TYPE_VALUE);
				if (ISP_SUCCESS != rtn) {
					ISP_LOGE("fail to check pm smart param !");
					return rtn;
				}

				reduce_percent = (cmr_u32) block_result->component[0].fix_data[0];
				factor_u = csa_ptr->tab[0][csa_ptr->cur_u_idx];
				factor_v = csa_ptr->tab[1][csa_ptr->cur_v_idx];
				csa_ptr->cur.factor_u = (factor_u * reduce_percent) / 255;
				csa_ptr->cur.factor_v = (factor_v * reduce_percent) / 255;
				csa_header_ptr->is_update = ISP_ONE;

				break;
			case ISP_SMART_SATURATION:
				rtn = _pm_check_smart_param(block_result, &val_range, 1, ISP_SMART_Y_TYPE_WEIGHT_VALUE);
				if (ISP_SUCCESS != rtn) {
					ISP_LOGE("fail to check pm smart param !");
					return rtn;
				}

				dst = &csa_ptr->cur.factor_u;
				dst1 = &csa_ptr->cur.factor_v;
				data_num = sizeof(cmr_u8);
				weight_value = (struct isp_weight_value *)block_result->component[0].fix_data;
				bv_value = &weight_value[0];

				src_mapu[0] = (void *)&csa_ptr->tab[0][bv_value->value[0]];
				src_mapu[1] = (void *)&csa_ptr->tab[0][bv_value->value[1]];
				src_mapv[0] = (void *)&csa_ptr->tab[1][bv_value->value[0]];
				src_mapv[1] = (void *)&csa_ptr->tab[1][bv_value->value[1]];
				weight[0] = bv_value->weight[0];
				weight[1] = bv_value->weight[1];
				weight[0] = weight[0] / (SMART_WEIGHT_UNIT / 16) *
							(SMART_WEIGHT_UNIT / 16);
				weight[1] = SMART_WEIGHT_UNIT - weight[0];
				isp_interp_data(dst, src_mapu, weight, data_num,
						ISP_INTERP_UINT8);
				isp_interp_data(dst1, src_mapv, weight,
						data_num, ISP_INTERP_UINT8);
				csa_header_ptr->is_update = ISP_ONE;
				break;
			default:
				ISP_LOGI("invalid smart id: %d", smart_id);
				break;
			}
		}
		break;

	default:
		break;
	}

	ISP_LOGV("set saturation param: index u,v:%d,%d cur_factor u,v:%d,%d",
		 csa_ptr->cur_u_idx,
		 csa_ptr->cur_v_idx,
		 csa_ptr->cur.factor_u,
		 csa_ptr->cur.factor_v);


	return rtn;
}

cmr_s32 _pm_saturation_get_param(void *csa_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_chrom_saturation_param *csa_ptr = (struct isp_chrom_saturation_param *)csa_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	cmr_u32 *update_flag = (cmr_u32 *) rtn_param1;

	param_data_ptr->id = ISP_BLK_SATURATION;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = (void *)&csa_ptr->cur;
		param_data_ptr->data_size = sizeof(csa_ptr->cur);
		*update_flag = 0;
		break;

	default:
		break;
	}

	return rtn;
}
