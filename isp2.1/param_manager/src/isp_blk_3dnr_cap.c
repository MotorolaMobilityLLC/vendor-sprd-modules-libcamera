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


isp_u32 _pm_3d_nr_cap_convert_param(void *dst_3d_nr_param, isp_u32 strength_level, isp_u32 mode_flag, isp_u32 scene_flag)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_s32 i = 0;
	isp_u32 total_offset_units = 0;
	struct isp_3d_nr_cap_param *dst_ptr = (struct isp_3d_nr_cap_param*)dst_3d_nr_param;
	struct sensor_3dnr_level *nr_3d_param = PNULL;

	if (SENSOR_MULTI_MODE_FLAG != dst_ptr->nr_mode_setting) {
		nr_3d_param = (struct sensor_3dnr_level *)(dst_ptr->param_ptr);
	} else {
		isp_u32 *multi_nr_map_ptr = PNULL;
		multi_nr_map_ptr = (isp_u32 *)dst_ptr->scene_ptr;
		total_offset_units = _pm_calc_nr_addr_offset(mode_flag, scene_flag, multi_nr_map_ptr);
		nr_3d_param = (struct sensor_3dnr_level *)((isp_u8 *)dst_ptr->param_ptr +
			total_offset_units * dst_ptr->level_num * sizeof(struct sensor_3dnr_level));
	}
	strength_level = PM_CLIP(strength_level, 0, dst_ptr->level_num - 1);
	if (nr_3d_param != NULL) {
		dst_ptr->cur.blend_bypass= nr_3d_param[strength_level].bypass;

		dst_ptr->cur.yuv_src_weight.y = nr_3d_param[strength_level].yuv_cfg.y_cfg.src_wgt;
		dst_ptr->cur.yuv_src_weight.u = nr_3d_param[strength_level].yuv_cfg.u_cfg.src_wgt;
		dst_ptr->cur.yuv_src_weight.v = nr_3d_param[strength_level].yuv_cfg.v_cfg.src_wgt;
		dst_ptr->cur.yuv_noise_thr.y = nr_3d_param[strength_level].yuv_cfg.y_cfg.nr_thr;
		dst_ptr->cur.yuv_noise_thr.u = nr_3d_param[strength_level].yuv_cfg.u_cfg.nr_thr;
		dst_ptr->cur.yuv_noise_thr.v = nr_3d_param[strength_level].yuv_cfg.v_cfg.nr_thr;
		dst_ptr->cur.yuv_noise_weight.y = nr_3d_param[strength_level].yuv_cfg.y_cfg.nr_wgt;
		dst_ptr->cur.yuv_noise_weight.u = nr_3d_param[strength_level].yuv_cfg.u_cfg.nr_wgt;
		dst_ptr->cur.yuv_noise_weight.v = nr_3d_param[strength_level].yuv_cfg.v_cfg.nr_wgt;
		dst_ptr->cur.u_thr_min= nr_3d_param[strength_level].sensor_3dnr_cor.u_range.min;
		dst_ptr->cur.u_thr_max= nr_3d_param[strength_level].sensor_3dnr_cor.u_range.max;
		dst_ptr->cur.v_thr_min= nr_3d_param[strength_level].sensor_3dnr_cor.v_range.min;
		dst_ptr->cur.v_thr_max= nr_3d_param[strength_level].sensor_3dnr_cor.v_range.max;
		for(i = 0; i < 9; i++) {
			dst_ptr->cur.yuv_threshold[i].y = nr_3d_param[strength_level].yuv_cfg.y_cfg.thr_polyline[i];
			dst_ptr->cur.yuv_threshold[i].u = nr_3d_param[strength_level].yuv_cfg.u_cfg.thr_polyline[i];
			dst_ptr->cur.yuv_threshold[i].v = nr_3d_param[strength_level].yuv_cfg.v_cfg.thr_polyline[i];
			dst_ptr->cur.yuv_intens_gain[i].y = nr_3d_param[strength_level].yuv_cfg.y_cfg.gain_polyline[i];
			dst_ptr->cur.yuv_intens_gain[i].u = nr_3d_param[strength_level].yuv_cfg.u_cfg.gain_polyline[i];
			dst_ptr->cur.yuv_intens_gain[i].v = nr_3d_param[strength_level].yuv_cfg.v_cfg.gain_polyline[i];
		}
		for(i = 0; i < 11; i++) {
			dst_ptr->cur.gradient_w[i] = nr_3d_param[strength_level].yuv_cfg.grad_wgt_polyline[i];
		}
		for(i = 0; i < 4; i++) {
			dst_ptr->cur.uv_thr_factor[i].u = nr_3d_param[strength_level].sensor_3dnr_cor.uv_factor.u_thr[i];
			dst_ptr->cur.uv_thr_factor[i].v = nr_3d_param[strength_level].sensor_3dnr_cor.uv_factor.v_thr[i];
			dst_ptr->cur.uv_div_factor[i].u = nr_3d_param[strength_level].sensor_3dnr_cor.uv_factor.u_div[i];
			dst_ptr->cur.uv_div_factor[i].v = nr_3d_param[strength_level].sensor_3dnr_cor.uv_factor.v_div[i];
		}
		for(i = 0; i < 3; i++) {
			dst_ptr->cur.r_value[i] = nr_3d_param[strength_level].sensor_3dnr_cor.r_circle[i];
		}
	}
	return rtn;
}
isp_s32 _pm_3d_nr_cap_init(void *dst_3d_nr_param, void *src_3d_nr_param, void *param1, void *param_ptr2)
{
	isp_s32 rtn = ISP_SUCCESS;

	struct sensor_nr_header_param *src_ptr = (struct sensor_nr_header_param*)src_3d_nr_param;
	struct isp_3d_nr_cap_param *dst_ptr = (struct isp_3d_nr_cap_param*)dst_3d_nr_param;
	struct isp_pm_block_header *nr_3d_header_ptr = (struct isp_pm_block_header*)param1;
	UNUSED(param_ptr2);

	dst_ptr->cur.blend_bypass = nr_3d_header_ptr->bypass;

	dst_ptr->cur_level = src_ptr->default_strength_level;
	dst_ptr->level_num = src_ptr->level_number;
	dst_ptr->param_ptr = src_ptr->param_ptr;
	dst_ptr->scene_ptr= src_ptr->multi_nr_map_ptr;
	dst_ptr->nr_mode_setting = src_ptr->nr_mode_setting;

	rtn = _pm_3d_nr_cap_convert_param(dst_ptr, dst_ptr->cur_level, ISP_MODE_ID_COMMON, ISP_SCENEMODE_AUTO);
	dst_ptr->cur.blend_bypass |= nr_3d_header_ptr->bypass;
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("ISP_PM_PWD_CONVERT_PARAM: error!");
		return rtn;
	}
	nr_3d_header_ptr->is_update = ISP_ONE;

	return rtn;
}

isp_s32 _pm_3d_nr_cap_set_param(void *nr_3d_param, isp_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header*)param_ptr1;
	struct isp_3d_nr_cap_param *dst_ptr = (struct isp_3d_nr_cap_param*)nr_3d_param;

	switch (cmd) {
	case ISP_PM_BLK_3D_NR_BYPASS:
		dst_ptr->cur.blend_bypass = *((isp_u32*)param_ptr0);
		header_ptr->is_update = ISP_ONE;
	break;

	case ISP_PM_BLK_3D_NR_STRENGTH_LEVEL:
		dst_ptr->cur_level= *((isp_u32*)param_ptr0);
		header_ptr->is_update = ISP_ONE;
	break;

	case ISP_PM_BLK_SMART_SETTING:
	{
		struct smart_block_result *block_result = (struct smart_block_result*)param_ptr0;
		struct isp_range val_range = {0, 0};
		isp_u32 level = 0;

		val_range.min = 0;
		val_range.max = 255;

		rtn = _pm_check_smart_param(block_result, &val_range, 1, ISP_SMART_Y_TYPE_VALUE);
		if (ISP_SUCCESS != rtn) {
			ISP_LOGE("ISP_PM_BLK_SMART_SETTING: wrong param !");
			return rtn;
		}

		level = (isp_u32)block_result->component[0].fix_data[0];

		if (level != dst_ptr->cur_level || nr_tool_flag[0] || block_result->mode_flag_changed) {
			dst_ptr->cur_level = level;
			header_ptr->is_update = 1;
			nr_tool_flag[0] = 0;
			block_result->mode_flag_changed = 0;

			rtn = _pm_3d_nr_cap_convert_param(dst_ptr,dst_ptr->cur_level,block_result->mode_flag,block_result->scene_flag);
			dst_ptr->cur.blend_bypass |= header_ptr->bypass;
			if (ISP_SUCCESS != rtn) {
				ISP_LOGE("ISP_PM_EDGE_CONVERT_PARAM: error!");
				return rtn;
			}
		}
	}
	break;

	default:

	break;
	}

	return rtn;
}

isp_s32 _pm_3d_nr_cap_get_param(void *nr_3d_param, isp_u32 cmd, void* rtn_param0, void* rtn_param1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_3d_nr_cap_param *nr_3d_ptr = (struct isp_3d_nr_cap_param*)nr_3d_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data*)rtn_param0;
	isp_u32 *update_flag = (isp_u32*)rtn_param1;

	param_data_ptr->id = ISP_BLK_3DNR_CAP;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = &nr_3d_ptr->cur;
		param_data_ptr->data_size = sizeof(nr_3d_ptr->cur);
		*update_flag = ISP_ZERO;
	break;

	default:
	break;
	}

	return rtn;
}

