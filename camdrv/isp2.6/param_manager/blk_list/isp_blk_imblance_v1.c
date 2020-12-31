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
#define LOG_TAG "isp_blk_imblance"
#include "isp_blocks_cfg.h"

static cmr_u32 _pm_imblance_convert_param(
	void *dst_imblance_param, cmr_u32 strength_level,
	cmr_u32 mode_flag, cmr_u32 scene_flag)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 total_offset_units = 0;
	struct isp_imblance_param *dst_ptr = (struct isp_imblance_param *)dst_imblance_param;
	struct sensor_nlm_imbalance_level *imblance_param = PNULL;

	if (SENSOR_MULTI_MODE_FLAG != dst_ptr->nr_mode_setting) {
		imblance_param = (struct sensor_nlm_imbalance_level *)(dst_ptr->param_ptr);
	} else {
		cmr_u32 *multi_nr_map_ptr = PNULL;
		multi_nr_map_ptr = (cmr_u32 *) dst_ptr->scene_ptr;
		total_offset_units = _pm_calc_nr_addr_offset(mode_flag, scene_flag, multi_nr_map_ptr);
		imblance_param = (struct sensor_nlm_imbalance_level *)((cmr_u8 *) dst_ptr->param_ptr + \
				total_offset_units * dst_ptr->level_num * sizeof(struct sensor_nlm_imbalance_level));
	}
	strength_level = PM_CLIP(strength_level, 0, dst_ptr->level_num - 1);

	if (imblance_param != NULL) {
		dst_ptr->cur_v1.nlm_imblance_bypass = imblance_param[strength_level].imblance_bypass;
		dst_ptr->cur_v1.imblance_radial_1D_en = imblance_param[strength_level].imblance_1D.imblance_radial_1D_en;
		dst_ptr->cur_v1.nlm_imblance_hv_edge_thr[0] = imblance_param[strength_level].imblance_hv[0].imblance_hv_edge_thr;
		dst_ptr->cur_v1.nlm_imblance_slash_edge_thr[0] = imblance_param[strength_level].imblance_hv[0].imblance_hv_slash_edge_thr;
		dst_ptr->cur_v1.nlm_imblance_S_baohedu[0][0] = imblance_param[strength_level].imblance_S_baohedu[0][0];
		dst_ptr->cur_v1.nlm_imblance_S_baohedu[0][1] = imblance_param[strength_level].imblance_S_baohedu[0][1];
		dst_ptr->cur_v1.nlm_imblance_hv_flat_thr[0] = imblance_param[strength_level].imblance_hv[0].imblance_hv_flat_thr;
		dst_ptr->cur_v1.nlm_imblance_slash_flat_thr[0] = imblance_param[strength_level].imblance_hv[0].imblance_slash_flat_thr;
		dst_ptr->cur_v1.nlm_imblance_flag3_grid = imblance_param[strength_level].nlm_imblance_flag3.flag3_grid;
		dst_ptr->cur_v1.nlm_imblance_flag3_lum = imblance_param[strength_level].nlm_imblance_flag3.flag3_lum;
		dst_ptr->cur_v1.nlm_imblance_flag3_frez = imblance_param[strength_level].nlm_imblance_flag3.flag3_frez;

		dst_ptr->cur_v1.nlm_imblance_lumth1 = imblance_param[strength_level].imblance_lumth[0];
		dst_ptr->cur_v1.nlm_imblance_lumth2 = imblance_param[strength_level].imblance_lumth[1];

		dst_ptr->cur_v1.nlm_imblance_flag12_frezthr = imblance_param[strength_level].nlm_imblance_flag3.flag12_frezthr;
		dst_ptr->cur_v1.nlm_imblance_lum1_flag2_r = imblance_param[strength_level].lum_flag[0].nlm_imblance_lum_flag_2_r;
		dst_ptr->cur_v1.nlm_imblance_lum1_flag4_r = imblance_param[strength_level].lum_flag[0].nlm_imblance_lum_flag_4_r;
		dst_ptr->cur_v1.nlm_imblance_lum1_flag0_rs = imblance_param[strength_level].lum_flag[0].nlm_imblance_lum_flag_0_rs;
		dst_ptr->cur_v1.nlm_imblance_lum1_flag0_r = imblance_param[strength_level].lum_flag[0].nlm_imblance_lum_flag_0_r;
		dst_ptr->cur_v1.nlm_imblance_lum1_flag1_r = imblance_param[strength_level].lum_flag[0].nlm_imblance_lum_flag_1_r;

		dst_ptr->cur_v1.nlm_imblance_lum2_flag2_r = imblance_param[strength_level].lum_flag[1].nlm_imblance_lum_flag_2_r;
		dst_ptr->cur_v1.nlm_imblance_lum2_flag4_r = imblance_param[strength_level].lum_flag[1].nlm_imblance_lum_flag_4_r;
		dst_ptr->cur_v1.nlm_imblance_lum2_flag0_rs = imblance_param[strength_level].lum_flag[1].nlm_imblance_lum_flag_0_rs;
		dst_ptr->cur_v1.nlm_imblance_lum2_flag0_r = imblance_param[strength_level].lum_flag[1].nlm_imblance_lum_flag_0_r;
		dst_ptr->cur_v1.nlm_imblance_lum2_flag1_r = imblance_param[strength_level].lum_flag[1].nlm_imblance_lum_flag_1_r;

		dst_ptr->cur_v1.nlm_imblance_lum3_flag2_r = imblance_param[strength_level].lum_flag[2].nlm_imblance_lum_flag_2_r;
		dst_ptr->cur_v1.nlm_imblance_lum3_flag4_r = imblance_param[strength_level].lum_flag[2].nlm_imblance_lum_flag_4_r;
		dst_ptr->cur_v1.nlm_imblance_lum3_flag0_rs = imblance_param[strength_level].lum_flag[2].nlm_imblance_lum_flag_0_rs;
		dst_ptr->cur_v1.nlm_imblance_lum3_flag0_r = imblance_param[strength_level].lum_flag[2].nlm_imblance_lum_flag_0_r;
		dst_ptr->cur_v1.nlm_imblance_lum3_flag1_r = imblance_param[strength_level].lum_flag[2].nlm_imblance_lum_flag_1_r;

		dst_ptr->cur_v1.nlm_imblance_diff[0] = imblance_param[strength_level].imblance_diff[0];
		dst_ptr->cur_v1.nlm_imblance_faceRmin = imblance_param[strength_level].imblance_face_rbg.face_r.face_min;
		dst_ptr->cur_v1.nlm_imblance_faceRmax= imblance_param[strength_level].imblance_face_rbg.face_r.face_max;
		dst_ptr->cur_v1.nlm_imblance_faceGmin = imblance_param[strength_level].imblance_face_rbg.face_g.face_min;
		dst_ptr->cur_v1.nlm_imblance_faceGmax= imblance_param[strength_level].imblance_face_rbg.face_g.face_max;
		dst_ptr->cur_v1.nlm_imblance_faceBmin = imblance_param[strength_level].imblance_face_rbg.face_b.face_min;
		dst_ptr->cur_v1.nlm_imblance_faceBmax= imblance_param[strength_level].imblance_face_rbg.face_b.face_max;

		dst_ptr->cur_v1.nlm_imblance_hv_edge_thr[1] = imblance_param[strength_level].imblance_hv[1].imblance_hv_edge_thr;
		dst_ptr->cur_v1.nlm_imblance_hv_edge_thr[2] = imblance_param[strength_level].imblance_hv[2].imblance_hv_edge_thr;
		dst_ptr->cur_v1.nlm_imblance_slash_edge_thr[1] = imblance_param[strength_level].imblance_hv[1].imblance_hv_slash_edge_thr;
		dst_ptr->cur_v1.nlm_imblance_slash_edge_thr[2] = imblance_param[strength_level].imblance_hv[2].imblance_hv_slash_edge_thr;

		dst_ptr->cur_v1.nlm_imblance_hv_flat_thr[1] = imblance_param[strength_level].imblance_hv[1].imblance_hv_flat_thr;
		dst_ptr->cur_v1.nlm_imblance_hv_flat_thr[2] = imblance_param[strength_level].imblance_hv[2].imblance_hv_flat_thr;

		dst_ptr->cur_v1.nlm_imblance_slash_flat_thr[1] = imblance_param[strength_level].imblance_hv[1].imblance_slash_flat_thr;
		dst_ptr->cur_v1.nlm_imblance_slash_flat_thr[2] = imblance_param[strength_level].imblance_hv[2].imblance_slash_flat_thr;

		dst_ptr->cur_v1.nlm_imblance_S_baohedu[1][0] = imblance_param[strength_level].imblance_S_baohedu[1][0];
		dst_ptr->cur_v1.nlm_imblance_S_baohedu[1][1] = imblance_param[strength_level].imblance_S_baohedu[1][1];
		dst_ptr->cur_v1.nlm_imblance_S_baohedu[2][0] = imblance_param[strength_level].imblance_S_baohedu[2][0];
		dst_ptr->cur_v1.nlm_imblance_S_baohedu[2][1] = imblance_param[strength_level].imblance_S_baohedu[2][1];

		dst_ptr->cur_v1.nlm_imblance_lum1_flag3_r = imblance_param[strength_level].lum_flag[0].nlm_imblance_lum_flag_3_r;
		dst_ptr->cur_v1.nlm_imblance_lum2_flag3_r = imblance_param[strength_level].lum_flag[1].nlm_imblance_lum_flag_3_r;

		dst_ptr->cur_v1.nlm_imblance_lum3_flag3_r = imblance_param[strength_level].lum_flag[2].nlm_imblance_lum_flag_3_r;
		dst_ptr->cur_v1.imblance_sat_lumth = imblance_param[strength_level].imblance_sat_lumth;

		dst_ptr->cur_v1.nlm_imblance_diff[1] = imblance_param[strength_level].imblance_diff[1];
		dst_ptr->cur_v1.nlm_imblance_diff[2] = imblance_param[strength_level].imblance_diff[2];

		dst_ptr->cur_v1.nlm_imblance_ff_wt2 = imblance_param[strength_level].imblance_curve.imblance_curve_wt2;
		dst_ptr->cur_v1.nlm_imblance_ff_wt3 = imblance_param[strength_level].imblance_curve.imblance_curve_wt3;
		dst_ptr->cur_v1.nlm_imblance_ff_wt1 = imblance_param[strength_level].imblance_curve.imblance_curve_wt1;
		dst_ptr->cur_v1.nlm_imblance_ff_wt0 = imblance_param[strength_level].imblance_curve.imblance_curve_wt0;

		dst_ptr->cur_v1.nlm_imblance_ff_wr2 = imblance_param[strength_level].imblance_curve.imblance_curve_wr2;
		dst_ptr->cur_v1.nlm_imblance_ff_wr3 = imblance_param[strength_level].imblance_curve.imblance_curve_wr3;
		dst_ptr->cur_v1.nlm_imblance_ff_wr1 = imblance_param[strength_level].imblance_curve.imblance_curve_wr1;
		dst_ptr->cur_v1.nlm_imblance_ff_wr0 = imblance_param[strength_level].imblance_curve.imblance_curve_wr0;

		dst_ptr->cur_v1.imblance_radial_1D_coef_r0 = imblance_param[strength_level].imblance_1D.imblance_radial_1D_coef_r0;
		dst_ptr->cur_v1.imblance_radial_1D_coef_r1 = imblance_param[strength_level].imblance_1D.imblance_radial_1D_coef_r1;
		dst_ptr->cur_v1.imblance_radial_1D_coef_r2 = imblance_param[strength_level].imblance_1D.imblance_radial_1D_coef_r2;
		dst_ptr->cur_v1.nlm_imblance_ff_wr4 = imblance_param[strength_level].imblance_curve.imblance_curve_wr4;

		dst_ptr->cur_v1.imblance_radial_1D_coef_r3 = imblance_param[strength_level].imblance_1D.imblance_radial_1D_coef_r3;
		dst_ptr->cur_v1.imblance_radial_1D_coef_r4 = imblance_param[strength_level].imblance_1D.imblance_radial_1D_coef_r4;
		dst_ptr->cur_v1.imblance_radial_1D_protect_ratio_max = imblance_param[strength_level].imblance_1D.imblance_radial_1D_protect_ratio_max;
		dst_ptr->cur_v1.imblance_radial_1D_center_x = imblance_param[strength_level].imblance_1D.imblance_radial_1D_center_x;
		dst_ptr->cur_v1.imblance_radial_1D_center_y = imblance_param[strength_level].imblance_1D.imblance_radial_1D_center_y;
		dst_ptr->cur_v1.imblance_radial_1D_radius_thr = imblance_param[strength_level].imblance_1D.imblance_radial_1D_radius_thr;
		dst_ptr->cur_v1.imblance_radial_1D_radius_thr_factor = imblance_param[strength_level].imblance_1D.imblance_radial_1D_radius_thr_factor;
		dst_ptr->cur_v1.radius_base = imblance_param[strength_level].radius_base;
	}

	return rtn;
}

cmr_s32 _pm_imblance_init(void *dst_imblance_param, void *src_imblance_param, void *param1, void *param_ptr2)
{
	cmr_s32 rtn = ISP_SUCCESS;

	struct isp_imblance_param *dst_ptr = (struct isp_imblance_param *)dst_imblance_param;
	struct isp_pm_nr_header_param *src_ptr = (struct isp_pm_nr_header_param *)src_imblance_param;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param1;
	UNUSED(param_ptr2);

	dst_ptr->cur_v1.nlm_imblance_bypass = header_ptr->bypass;
	dst_ptr->cur_level = src_ptr->default_strength_level;
	dst_ptr->level_num = src_ptr->level_number;
	dst_ptr->param_ptr = src_ptr->param_ptr;
	dst_ptr->scene_ptr = src_ptr->multi_nr_map_ptr;
	dst_ptr->nr_mode_setting = src_ptr->nr_mode_setting;
	if (!header_ptr->bypass)
		rtn = _pm_imblance_convert_param(dst_ptr, dst_ptr->cur_level, ISP_MODE_ID_COMMON, ISP_SCENEMODE_AUTO);
	dst_ptr->cur_v1.nlm_imblance_bypass |= header_ptr->bypass;
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to convert pm imblance param !");
		return rtn;
	}

	header_ptr->is_update = ISP_ONE;
	return rtn;
}

cmr_s32 _pm_imblance_set_param(void *imblance_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_imblance_param *imblance_ptr = (struct isp_imblance_param *)imblance_param;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param_ptr1;

	switch (cmd) {
	case ISP_PM_BLK_SMART_SETTING:
		{
			struct smart_block_result *block_result = (struct smart_block_result *)param_ptr0;
			struct isp_range val_range = { 0, 0 };
			cmr_u32 cur_level = 0;

			if (0 == block_result->update || header_ptr->bypass) {
				ISP_LOGV("do not need update\n");
				return ISP_SUCCESS;
			}

			val_range.min = 0;
			val_range.max = 255;
			rtn = _pm_check_smart_param(block_result, &val_range, 1, ISP_SMART_Y_TYPE_VALUE);
			if (ISP_SUCCESS != rtn) {
				ISP_LOGE("fail to check pm smart param !");
				return rtn;
			}

			cur_level = (cmr_u32) block_result->component[0].fix_data[0];

			if (cur_level != imblance_ptr->cur_level || nr_tool_flag[ISP_BLK_IMBALANCEE_T] || block_result->mode_flag_changed) {
				imblance_ptr->cur_level = cur_level;
				header_ptr->is_update = 1;
				nr_tool_flag[ISP_BLK_IMBALANCEE_T] = 0;

				rtn = _pm_imblance_convert_param(imblance_ptr,
					imblance_ptr->cur_level, header_ptr->mode_id, block_result->scene_flag);
				imblance_ptr->cur_v1.nlm_imblance_bypass |= header_ptr->bypass;
				if (ISP_SUCCESS != rtn) {
					ISP_LOGE("fail to convert pm imblance param");
					return rtn;
				}
			}
			ISP_LOGV("ISP_SMART: cmd = %d, is_update = %d, cur_level=%d",
				cmd, header_ptr->is_update, imblance_ptr->cur_level);
		}
		break;

	default:
		break;
	}

	return rtn;
}

cmr_s32 _pm_imblance_get_param(void *imblance_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_imblance_param *imblance_ptr = (struct isp_imblance_param *)imblance_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	cmr_u32 *update_flag = (cmr_u32 *) rtn_param1;

	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = &imblance_ptr->cur_v1;
		param_data_ptr->data_size = sizeof(imblance_ptr->cur_v1);
		*update_flag = 0;
		break;
	default:
		break;
	}

	return rtn;
}
