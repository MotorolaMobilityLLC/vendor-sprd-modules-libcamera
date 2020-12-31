/*
 * Copyright (C) 2019 The Android Open Source Project
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
#define LOG_TAG "isp_blk_bpc"
#include "isp_blocks_cfg.h"

static cmr_u32 _pm_bpc_convert_param(void *dst_param,
	cmr_u32 strength_level, cmr_u32 mode_flag, cmr_u32 scene_flag)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_s32 i = 0;
	cmr_u32 total_offset_units = 0;
	struct isp_bpc_param *dst_ptr = (struct isp_bpc_param *)dst_param;
	struct sensor_bpc_level *bpc_param;

	if (SENSOR_MULTI_MODE_FLAG != dst_ptr->nr_mode_setting) {
		bpc_param = (struct sensor_bpc_level *)(dst_ptr->param_ptr);
	} else {
		cmr_u32 *multi_nr_map_ptr = PNULL;
		multi_nr_map_ptr = (cmr_u32 *) dst_ptr->scene_ptr;
		total_offset_units = _pm_calc_nr_addr_offset(mode_flag, scene_flag, multi_nr_map_ptr);
		bpc_param = (struct sensor_bpc_level *)((cmr_u8 *) dst_ptr->param_ptr + \
			total_offset_units * dst_ptr->level_num * sizeof(struct sensor_bpc_level));
	}
	strength_level = PM_CLIP(strength_level, 0, dst_ptr->level_num - 1);

	if (bpc_param != NULL) {
		dst_ptr->cur_v0.pos_out_continue_mode = bpc_param[strength_level].bpc_pos.continuous_mode;
		dst_ptr->cur_v0.pos_out_skip_num = bpc_param[strength_level].bpc_pos.skip_num;
		dst_ptr->cur_v0.bpc_mode = bpc_param[strength_level].bpc_comm.bpc_mode;
		dst_ptr->cur_v0.edge_hv_mode = bpc_param[strength_level].bpc_comm.hv_mode;
		dst_ptr->cur_v0.edge_rd_mode = bpc_param[strength_level].bpc_comm.rd_mode;
		dst_ptr->cur_v0.bpc_double_bypass = bpc_param[strength_level].bpc_comm.double_bypass;
		dst_ptr->cur_v0.bpc_three_bypass = bpc_param[strength_level].bpc_comm.three_bypass;
		dst_ptr->cur_v0.bpc_four_bypass = bpc_param[strength_level].bpc_comm.four_bypass;
		dst_ptr->cur_v0.is_mono_sensor = bpc_param[strength_level].bpc_comm.isMonoSensor;
		for (i = 0; i < 8; i++) {
			dst_ptr->cur_v0.lut_level[i] = bpc_param[strength_level].bpc_comm.lut_level[i];
			dst_ptr->cur_v0.slope_k[i] = bpc_param[strength_level].bpc_comm.slope_k[i];
			dst_ptr->cur_v0.intercept_b[i] = bpc_param[strength_level].bpc_comm.intercept_b[i];
		}
		for (i = 0; i < 4; i++) {
			dst_ptr->cur_v0.double_badpixel_th[i] = bpc_param[strength_level].bpc_thr.double_th[i];
			dst_ptr->cur_v0.three_badpixel_th[i] = bpc_param[strength_level].bpc_thr.three_th[i];
			dst_ptr->cur_v0.four_badpixel_th[i] = bpc_param[strength_level].bpc_thr.four_th[i];
		}
		dst_ptr->cur_v0.texture_th = bpc_param[strength_level].bpc_thr.texture_th;
		dst_ptr->cur_v0.flat_th = bpc_param[strength_level].bpc_thr.flat_th;
		for (i = 0; i < 3; i++) {
			dst_ptr->cur_v0.shift[i] = bpc_param[strength_level].bpc_thr.shift[i];
		}
		dst_ptr->cur_v0.edge_ratio_hv = bpc_param[strength_level].bpc_rules.hv_ratio;
		dst_ptr->cur_v0.edge_ratio_rd = bpc_param[strength_level].bpc_rules.rd_ratio;
		dst_ptr->cur_v0.high_offset = bpc_param[strength_level].bpc_rules.highoffset;
		dst_ptr->cur_v0.low_offset = bpc_param[strength_level].bpc_rules.lowoffset;
		dst_ptr->cur_v0.high_coeff = bpc_param[strength_level].bpc_rules.highcoeff;
		dst_ptr->cur_v0.low_coeff = bpc_param[strength_level].bpc_rules.lowcoeff;
		dst_ptr->cur_v0.min_coeff = bpc_param[strength_level].bpc_rules.k_val.min;
		dst_ptr->cur_v0.max_coeff = bpc_param[strength_level].bpc_rules.k_val.max;

		dst_ptr->cur_v0.map_addr = 0x00;
		dst_ptr->cur_v0.bad_pixel_pos_out_addr = 0x00;
		dst_ptr->cur_v0.bad_map_hw_fifo_clr_en = 0x00;
		dst_ptr->cur_v0.bad_pixel_num = 0x00;
	}
	return rtn;
}

cmr_s32 _pm_bpc_init(void *dst_bpc_param, void *src_bpc_param, void *param1, void *param2)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_pm_nr_header_param *src_ptr = (struct isp_pm_nr_header_param *)src_bpc_param;
	struct isp_bpc_param *dst_ptr = (struct isp_bpc_param *)dst_bpc_param;
	struct isp_pm_block_header *bpc_header_ptr = (struct isp_pm_block_header *)param1;
	UNUSED(param2);

	dst_ptr->cur_v0.bpc_mode_en_gc = 1;
	dst_ptr->cur_v0.bpc_mode_en = 1;
	dst_ptr->cur_v0.bpc_gc_cg_dis = 0;

	dst_ptr->cur_v0.rd_retain_num = 0xf;
	dst_ptr->cur_v0.rd_max_len_sel = 1;
	dst_ptr->cur_v0.wr_max_len_sel = 1;
	dst_ptr->cur_v0.bpc_blk_mode = 0;

	dst_ptr->cur_v0.bpc_bypass = bpc_header_ptr->bypass;
	dst_ptr->cur_level = src_ptr->default_strength_level;
	dst_ptr->level_num = src_ptr->level_number;
	dst_ptr->param_ptr = src_ptr->param_ptr;
	dst_ptr->scene_ptr = src_ptr->multi_nr_map_ptr;
	dst_ptr->nr_mode_setting = src_ptr->nr_mode_setting;

	if (!bpc_header_ptr->bypass)
		rtn = _pm_bpc_convert_param(dst_ptr, dst_ptr->cur_level, ISP_MODE_ID_COMMON, ISP_SCENEMODE_AUTO);
	dst_ptr->cur_v0.bpc_bypass |= bpc_header_ptr->bypass;
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to convert pm bpc param !");
		return rtn;
	}

	bpc_header_ptr->is_update = ISP_ONE;

	return rtn;
}

cmr_s32 _pm_bpc_set_param(void *bpc_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_bpc_param *dst_ptr = (struct isp_bpc_param *)bpc_param;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param_ptr1;

	switch (cmd) {
	case ISP_PM_BLK_BPC:
		break;

	case ISP_PM_BLK_BPC_BYPASS:
		dst_ptr->cur_v0.bpc_bypass = *((cmr_u32 *) param_ptr0);
		header_ptr->is_update = ISP_ONE;
		break;

	case ISP_PM_BLK_BPC_MODE:
		{
			cmr_u32 mode = *((cmr_u32 *) param_ptr0);
			dst_ptr->cur_v0.bpc_mode = mode;
			header_ptr->is_update = ISP_ONE;
		}
		break;

	case ISP_PM_BLK_BPC_THRD:
		break;

	case ISP_PM_BLK_BPC_MAP_ADDR:
		{
			/* need caller this enum to set this value. */
			header_ptr->is_update = ISP_ONE;
		}
		break;

	case ISP_PM_BLK_SMART_SETTING:
		{
			struct smart_block_result *block_result = (struct smart_block_result *)param_ptr0;
			struct isp_range val_range = { 0, 0 };
			cmr_u32 cur_level = 0;

			if (!block_result->update || header_ptr->bypass) {
				ISP_LOGV("do not need update\n");
				return ISP_SUCCESS;
			}

			val_range.min = 0;
			val_range.max = 255;
			rtn = _pm_check_smart_param(block_result, &val_range, 1, ISP_SMART_Y_TYPE_VALUE);
			if (ISP_SUCCESS != rtn) {
				ISP_LOGE("fail to check pm  smart param!");
				return rtn;
			}

			cur_level = (cmr_u32) block_result->component[0].fix_data[0];

			if (cur_level != dst_ptr->cur_level || nr_tool_flag[ISP_BLK_BPC_T] || block_result->mode_flag_changed) {
				dst_ptr->cur_level = cur_level;
				header_ptr->is_update = ISP_ONE;
				nr_tool_flag[ISP_BLK_BPC_T] = 0;

				rtn = _pm_bpc_convert_param(dst_ptr, dst_ptr->cur_level, header_ptr->mode_id, block_result->scene_flag);
				dst_ptr->cur_v0.bpc_bypass |= header_ptr->bypass;

				if (ISP_SUCCESS != rtn) {
					ISP_LOGE("fail to convert pm bpc param !");
					return rtn;
				}
			}
			ISP_LOGV("ISP_SMART: cmd=%d, update=%d, cur_v0.level=%d", cmd, header_ptr->is_update, dst_ptr->cur_level);
		}
		break;

	default:
		break;
	}

	return rtn;
}

cmr_s32 _pm_bpc_get_param(void *bpc_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_bpc_param *bpc_ptr = (struct isp_bpc_param *)bpc_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	cmr_u32 *update_flag = (cmr_u32 *) rtn_param1;

	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = (void *)&bpc_ptr->cur_v0;
		param_data_ptr->data_size = sizeof(bpc_ptr->cur_v0);
		*update_flag = 0;
		break;

	case ISP_PM_BLK_BPC_BYPASS:
		param_data_ptr->data_ptr = (void *)&bpc_ptr->cur_v0.bpc_bypass;
		param_data_ptr->data_size = sizeof(bpc_ptr->cur_v0.bpc_bypass);
		break;

	default:
		break;
	}

	return rtn;
}
