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
#define LOG_TAG "isp_blk_bchs"
#include "isp_blocks_cfg.h"

cmr_s32 _pm_bchs_init(void *dst_bchs_param, void *src_bchs_param, void *param1, void *param2)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct sensor_bchs_level *src_ptr = (struct sensor_bchs_level *)src_bchs_param;
	struct isp_bchs_param *dst_ptr = (struct isp_bchs_param *)dst_bchs_param;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param1;
	UNUSED(param2);

	memcpy((void *)dst_ptr->brigntness.bright_tab,
		(void *)src_ptr->brightness.factor, sizeof(dst_ptr->brigntness.bright_tab));
	memcpy((void *)dst_ptr->brigntness.scene_mode_tab,
		(void *)src_ptr->brightness.scenemode, sizeof(dst_ptr->brigntness.scene_mode_tab));

	memcpy((void *)dst_ptr->contrast.tab,
		(void *)src_ptr->contrast.factor, sizeof(dst_ptr->contrast.tab));
	memcpy((void *)dst_ptr->contrast.scene_mode_tab,
		(void *)src_ptr->contrast.scenemode, sizeof(dst_ptr->contrast.scene_mode_tab));

	memcpy((void *)dst_ptr->hue.tab_sin, (void *)src_ptr->hue.hue_sin, sizeof(dst_ptr->hue.tab_sin));
	memcpy((void *)dst_ptr->hue.tab_cos, (void *)src_ptr->hue.hue_cos, sizeof(dst_ptr->hue.tab_cos));

	memcpy((void *)dst_ptr->saturation.tab[0],
		(void *)src_ptr->saturation.csa_factor_u, sizeof(dst_ptr->saturation.tab[0]));
	memcpy((void *)dst_ptr->saturation.tab[1],
		(void *)src_ptr->saturation.csa_factor_v, sizeof(dst_ptr->saturation.tab[1]));
	memcpy((void *)dst_ptr->saturation.scene_mode_tab[0],
		(void *)src_ptr->saturation.scenemode[0], sizeof(dst_ptr->saturation.scene_mode_tab[0]));
	memcpy((void *)dst_ptr->saturation.scene_mode_tab[1],
		(void *)src_ptr->saturation.scenemode[1], sizeof(dst_ptr->saturation.scene_mode_tab[1]));

	dst_ptr->brigntness.cur_index = src_ptr->brightness.cur_index;
	dst_ptr->contrast.cur_index = src_ptr->contrast.cur_index;
	dst_ptr->hue.cur_idx = src_ptr->hue.cur_index;
	dst_ptr->saturation.cur_u_idx = src_ptr->saturation.index_u;
	dst_ptr->saturation.cur_v_idx = src_ptr->saturation.index_v;

	dst_ptr->cur.bchs_bypass = header_ptr->bypass;
	dst_ptr->cur.brta_en = !src_ptr->brightness.bypass;
	dst_ptr->cur.cnta_en = !src_ptr->contrast.bypass;
	dst_ptr->cur.hua_en = !src_ptr->hue.bypass;
	dst_ptr->cur.csa_en = !src_ptr->saturation.bypass;

	dst_ptr->cur.brta_factor = dst_ptr->brigntness.bright_tab[dst_ptr->brigntness.cur_index];
	dst_ptr->cur.cnta_factor = dst_ptr->contrast.tab[dst_ptr->contrast.cur_index];
	dst_ptr->cur.hua_cos_value = dst_ptr->hue.tab_cos[dst_ptr->hue.cur_idx];
	dst_ptr->cur.hua_sina_value = dst_ptr->hue.tab_sin[dst_ptr->hue.cur_idx];
	dst_ptr->cur.csa_factor_u = dst_ptr->saturation.tab[0][dst_ptr->saturation.cur_u_idx];
	dst_ptr->cur.csa_factor_v = dst_ptr->saturation.tab[1][dst_ptr->saturation.cur_v_idx];

	header_ptr->is_update = 1;

	return rtn;
}

cmr_s32 _pm_bchs_set_param(void *bchs_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_bchs_param *dst_ptr = (struct isp_bchs_param *)bchs_param;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param_ptr1;

	header_ptr->is_update = ISP_ONE;

	switch (cmd) {
	case ISP_PM_BLK_BCHS_BYPASS:
		dst_ptr->cur.bchs_bypass = *((cmr_u32 *) param_ptr0);
		break;

	case ISP_PM_BLK_BRIGHT:
		dst_ptr->brigntness.cur_index = *((cmr_u32 *) param_ptr0);
		dst_ptr->cur.brta_factor = dst_ptr->brigntness.bright_tab[dst_ptr->brigntness.cur_index];
		break;

	case ISP_PM_BLK_CONTRAST:
		dst_ptr->contrast.cur_index = *((cmr_u32 *) param_ptr0);
		dst_ptr->cur.cnta_factor = dst_ptr->contrast.tab[dst_ptr->contrast.cur_index];
		break;

	case ISP_PM_BLK_HUE:
		dst_ptr->hue.cur_idx = *((cmr_u32 *) param_ptr0);
		dst_ptr->cur.hua_cos_value = dst_ptr->hue.tab_cos[dst_ptr->hue.cur_idx];
		dst_ptr->cur.hua_sina_value = dst_ptr->hue.tab_sin[dst_ptr->hue.cur_idx];
		break;

	case ISP_PM_BLK_SATURATION:
		dst_ptr->saturation.cur_u_idx = *((cmr_u32 *) param_ptr0);
		dst_ptr->saturation.cur_v_idx = *((cmr_u32 *) param_ptr0);
		dst_ptr->cur.csa_factor_u = dst_ptr->saturation.tab[0][dst_ptr->saturation.cur_u_idx];
		dst_ptr->cur.csa_factor_v = dst_ptr->saturation.tab[1][dst_ptr->saturation.cur_v_idx];
		break;

	case ISP_PM_BLK_SCENE_MODE:
		{
			cmr_u32 idx = *((cmr_u32 *) param_ptr0);
			if (0 == idx) {
				dst_ptr->cur.brta_factor = dst_ptr->brigntness.bright_tab[dst_ptr->brigntness.cur_index];
				dst_ptr->cur.cnta_factor = dst_ptr->contrast.tab[dst_ptr->contrast.cur_index];
				dst_ptr->cur.hua_cos_value = dst_ptr->hue.tab_cos[dst_ptr->hue.cur_idx];
				dst_ptr->cur.hua_sina_value = dst_ptr->hue.tab_sin[dst_ptr->hue.cur_idx];
				dst_ptr->cur.csa_factor_u = dst_ptr->saturation.tab[0][dst_ptr->saturation.cur_u_idx];
				dst_ptr->cur.csa_factor_v = dst_ptr->saturation.tab[1][dst_ptr->saturation.cur_v_idx];
			} else {
				dst_ptr->cur.brta_factor = dst_ptr->brigntness.scene_mode_tab[idx];
				dst_ptr->cur.cnta_factor = dst_ptr->contrast.scene_mode_tab[idx];
				dst_ptr->cur.csa_factor_u = dst_ptr->saturation.scene_mode_tab[0][idx];
				dst_ptr->cur.csa_factor_v = dst_ptr->saturation.scene_mode_tab[1][idx];
			}
		}
		break;

	default:
		header_ptr->is_update = ISP_ZERO;
		break;
	}

	return rtn;
}

cmr_s32 _pm_bchs_get_param(void *bchs_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_bchs_param *dst_ptr = (struct isp_bchs_param *)bchs_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	cmr_u32 *update_flag = (cmr_u32 *) rtn_param1;

	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = (void *)&dst_ptr->cur;
		param_data_ptr->data_size = sizeof(dst_ptr->cur);
		*update_flag = 0;
		break;

	default:
		break;
	}

	return rtn;
}
