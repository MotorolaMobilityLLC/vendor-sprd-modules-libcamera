/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "isp_blk_dre_pro"
#include "isp_blocks_cfg.h"

static void post_dre_pro_init(struct isp_dres_pro_param *dst_ptr,
			struct sensor_postdre_pro_param *src_ptr)
{
	struct isp_dres_pro_param *pdst = dst_ptr;
	struct sensor_postdre_pro_param *psrc = src_ptr;

	for(int i = 0; i < 16; i++) {
		pdst->levels[i].postdre_pro_param.enable = psrc[i].enable;
		pdst->levels[i].postdre_pro_param.strength = psrc[i].strength;
		pdst->levels[i].postdre_pro_param.texture_counter_en =
			psrc[i].texture_counter_en;
		pdst->levels[i].postdre_pro_param.text_point_thres =
			psrc[i].text_point_thres;
		pdst->levels[i].postdre_pro_param.text_prop_thres =
			psrc[i].text_prop_thres;
		pdst->levels[i].postdre_pro_param.tile_num_auto =
			psrc[i].tile_num_auto;
		pdst->levels[i].postdre_pro_param.tile_num_x = psrc[i].tile_num_x;
		pdst->levels[i].postdre_pro_param.tile_num_y = psrc[i].tile_num_y;
		pdst->levels[i].postdre_pro_param.text_point_alpha =
				psrc[i].text_point_alpha;
	}
}

static void pre_dre_pro_init(struct isp_dres_pro_param *dst_ptr,
			 struct sensor_predre_pro_param *src_ptr)
{
	struct isp_dres_pro_param *pdst = dst_ptr;
	struct sensor_predre_pro_param *psrc = src_ptr;

	for(int i = 0; i < 16; i++) {
		pdst->levels[i].predre_pro_param.enable = psrc[i].enable;
		pdst->levels[i].predre_pro_param.imgKey_setting_mode =
			psrc[i].imgKey_setting_mode;
		pdst->levels[i].predre_pro_param.tarNorm_setting_mode =
			psrc[i].tarNorm_setting_mode;
		pdst->levels[i].predre_pro_param.target_norm = psrc[i].target_norm;
		pdst->levels[i].predre_pro_param.imagekey = psrc[i].imagekey;
		pdst->levels[i].predre_pro_param.min_per = psrc[i].min_per;
		pdst->levels[i].predre_pro_param.max_per = psrc[i].max_per;
		pdst->levels[i].predre_pro_param.stat_step = psrc[i].stat_step;
		pdst->levels[i].predre_pro_param.low_thresh = psrc[i].low_thresh;
		pdst->levels[i].predre_pro_param.high_thresh = psrc[i].high_thresh;
		pdst->levels[i].predre_pro_param.uv_gain_ratio = psrc[i].uv_gain_ratio;
		pdst->levels[i].predre_pro_param.tarCoeff = psrc[i].tarCoeff;
	}
}

cmr_s32 _pm_dre_pro_init(void *dst_dre_param_pro, void *src_dre_param_pro, void *param1,
			void *param2)
{
	//cmr_u32 i = 0;
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 def_lv = 0;
	struct isp_dres_pro_param *dst_ptr =
		(struct isp_dres_pro_param *)dst_dre_param_pro;
	struct sensor_dre_pro_level *src_ptr =
		(struct sensor_dre_pro_level *)src_dre_param_pro;
	struct isp_pm_block_header *header_ptr =
		(struct isp_pm_block_header *)param1;
	UNUSED(param2);

	pre_dre_pro_init(dst_ptr, &src_ptr->predre_param[0]);
	post_dre_pro_init(dst_ptr, &src_ptr->postdre_param[0]);

	dst_ptr->levels[def_lv].predre_pro_param.enable &= !header_ptr->bypass;
	dst_ptr->levels[def_lv].postdre_pro_param.enable &= !header_ptr->bypass;
	ISP_LOGV("predre_en %d postdre_en %d",
		dst_ptr->levels[def_lv].predre_pro_param.enable,
		dst_ptr->levels[def_lv].postdre_pro_param.enable);

	memcpy(&dst_ptr->cur.predre_pro_param,
			&dst_ptr->levels[def_lv].predre_pro_param,
			sizeof(struct isp_predre_pro_param));
	memcpy(&dst_ptr->cur.postdre_pro_param,
			&dst_ptr->levels[def_lv].postdre_pro_param,
			sizeof(struct isp_postdre_pro_param));

	header_ptr->is_update = ISP_ONE;

	return rtn;
}

cmr_s32 _pm_dre_pro_set_param(void *dre_pro_param, cmr_u32 cmd, void *param_ptr0,
			void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_pm_block_header *header_ptr =
		(struct isp_pm_block_header *)param_ptr1;
	struct isp_dres_pro_param *dst_ptr = (struct isp_dres_pro_param *)dre_pro_param;

	header_ptr->is_update = ISP_ONE;
	switch (cmd) {
	case ISP_PM_BLK_SMART_SETTING: {
		struct smart_block_result *block_result =
			(struct smart_block_result *)param_ptr0;
		struct isp_range val_range = { 0, 0 };
		cmr_u32 level = 0;
		val_range.min = 0;
		val_range.max = 255;

		if (0 == block_result->update) {
			ISP_LOGI("do not need update\n");
			return ISP_SUCCESS;
		}


		rtn = _pm_check_smart_param(block_result, &val_range, 1,
					ISP_SMART_Y_TYPE_VALUE);
		if (ISP_SUCCESS != rtn) {
			ISP_LOGE("fail to check pm smart param !");
			return rtn;
		}

		level = (cmr_u32) block_result->component[0].fix_data[0];
		if (ISP_SUCCESS != rtn) {
			ISP_LOGE("fail to check pm smart param !");
			return rtn;
		}

		dst_ptr->cur.predre_pro_param.enable =
			dst_ptr->levels[level].predre_pro_param.enable;
		dst_ptr->cur.predre_pro_param.enable &= !header_ptr->bypass;
		dst_ptr->cur.predre_pro_param.imgKey_setting_mode =
			dst_ptr->levels[level].predre_pro_param.imgKey_setting_mode;
		dst_ptr->cur.predre_pro_param.tarNorm_setting_mode =
			dst_ptr->levels[level].predre_pro_param.tarNorm_setting_mode;
		dst_ptr->cur.predre_pro_param.target_norm =
			dst_ptr->levels[level].predre_pro_param.target_norm;
		dst_ptr->cur.predre_pro_param.imagekey =
			dst_ptr->levels[level].predre_pro_param.imagekey;
		dst_ptr->cur.predre_pro_param.min_per =
			dst_ptr->levels[level].predre_pro_param.min_per;
		dst_ptr->cur.predre_pro_param.max_per =
			dst_ptr->levels[level].predre_pro_param.max_per;
		dst_ptr->cur.predre_pro_param.stat_step =
			dst_ptr->levels[level].predre_pro_param.stat_step;
		dst_ptr->cur.predre_pro_param.low_thresh =
			dst_ptr->levels[level].predre_pro_param.low_thresh;
		dst_ptr->cur.predre_pro_param.high_thresh =
			dst_ptr->levels[level].predre_pro_param.high_thresh;
		dst_ptr->levels[level].predre_pro_param.uv_gain_ratio =
			dst_ptr->levels[level].predre_pro_param.uv_gain_ratio;
		dst_ptr->cur.predre_pro_param.tarCoeff =
			dst_ptr->levels[level].predre_pro_param.tarCoeff;

		dst_ptr->cur.postdre_pro_param.enable =
			dst_ptr->levels[level].postdre_pro_param.enable;
		dst_ptr->cur.postdre_pro_param.enable &= !header_ptr->bypass;
		dst_ptr->cur.postdre_pro_param.tile_num_x =
			dst_ptr->levels[level].postdre_pro_param.tile_num_x;
		dst_ptr->cur.postdre_pro_param.tile_num_y =
			dst_ptr->levels[level].postdre_pro_param.tile_num_y;
		dst_ptr->cur.postdre_pro_param.strength =
			dst_ptr->levels[level].postdre_pro_param.strength;
		dst_ptr->cur.postdre_pro_param.text_point_thres =
			dst_ptr->levels[level].postdre_pro_param.text_point_thres;
		dst_ptr->cur.postdre_pro_param.text_prop_thres =
			dst_ptr->levels[level].postdre_pro_param.text_prop_thres;
		dst_ptr->cur.postdre_pro_param.texture_counter_en =
			dst_ptr->levels[level].postdre_pro_param.texture_counter_en;
		dst_ptr->cur.postdre_pro_param.tile_num_auto =
			dst_ptr->levels[level].postdre_pro_param.tile_num_auto;
		dst_ptr->cur.postdre_pro_param.text_point_alpha =
			dst_ptr->levels[level].postdre_pro_param.text_point_alpha;
		ISP_LOGV("cur level = %x, pre en = %x, post en = %x",
			level, dst_ptr->cur.predre_pro_param.enable,
			dst_ptr->cur.postdre_pro_param.enable);
		}
		break;

	default:
		break;
	}

	ISP_LOGV("ISP_SMART_DRE_PRO: cmd=%d, update=%d",
		 cmd, header_ptr->is_update);

	return rtn;
}

cmr_s32 _pm_dre_pro_get_param(void *dre_param, cmr_u32 cmd, void *rtn_param0,
			void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_dres_pro_param *dre_ptr = (struct isp_dres_pro_param *)dre_param;
	struct isp_pm_param_data *param_data_ptr =
		(struct isp_pm_param_data *)rtn_param0;
	cmr_u32 *update_flag = (cmr_u32 *) rtn_param1;

	param_data_ptr->id = ISP_BLK_DRE;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = &dre_ptr->cur;
		param_data_ptr->data_size = sizeof(dre_ptr->cur);
		*update_flag = ISP_ZERO;
		break;

	default:
		break;
	}

	return rtn;
}
