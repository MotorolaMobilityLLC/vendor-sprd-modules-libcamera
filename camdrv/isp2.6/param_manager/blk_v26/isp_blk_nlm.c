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
#define LOG_TAG "isp_blk_nlm"
#include "isp_blocks_cfg.h"

cmr_u32 _pm_nlm_convert_param(void *dst_nlm_param,
		cmr_u32 strength_level, cmr_u32 mode_flag, cmr_u32 scene_flag)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_s32 i = 0, j = 0;
	cmr_u32 total_offset_units = 0;
	cmr_uint addr;
	struct isp_nlm_param *dst_ptr = (struct isp_nlm_param *)dst_nlm_param;

	struct sensor_nlm_level *nlm_param = NULL;
	struct sensor_vst_level *vst_param = NULL;
	struct sensor_ivst_level *ivst_param = NULL;

	if (SENSOR_MULTI_MODE_FLAG != dst_ptr->nr_mode_setting) {
		nlm_param = (struct sensor_nlm_level *)(dst_ptr->nlm_ptr);
		vst_param = (struct sensor_vst_level *)(dst_ptr->vst_ptr);
		ivst_param = (struct sensor_ivst_level *)(dst_ptr->ivst_ptr);
	} else {
		cmr_u32 *multi_nr_map_ptr = PNULL;
		multi_nr_map_ptr = (cmr_u32 *) dst_ptr->scene_ptr;

		total_offset_units = _pm_calc_nr_addr_offset(mode_flag, scene_flag, multi_nr_map_ptr);
		nlm_param = (struct sensor_nlm_level *)((cmr_u8 *) dst_ptr->nlm_ptr +
				total_offset_units * dst_ptr->level_num * sizeof(struct sensor_nlm_level));

		vst_param = (struct sensor_vst_level *)((cmr_u8 *) dst_ptr->vst_ptr +
				total_offset_units * dst_ptr->level_num * sizeof(struct sensor_vst_level));

		ivst_param = (struct sensor_ivst_level *)((cmr_u8 *) dst_ptr->ivst_ptr +
				total_offset_units * dst_ptr->level_num * sizeof(struct sensor_ivst_level));
	}

	strength_level = PM_CLIP(strength_level, 0, dst_ptr->level_num - 1);
	if (NULL == nlm_param) {
		return -1;
	} else {
		dst_ptr->cur.direction_mode_bypass = nlm_param[strength_level].nlm_dic.direction_mode_bypass;

		dst_ptr->cur.dist_mode = nlm_param[strength_level].nlm_dic.dist_mode;
		dst_ptr->cur.direction_cnt_th = nlm_param[strength_level].nlm_dic.cnt_th;
		dst_ptr->cur.tdist_min_th = nlm_param[strength_level].nlm_dic.tdist_min_th;
		dst_ptr->cur.diff_th = nlm_param[strength_level].nlm_dic.diff_th;

		for (i = 0; i < 72; i++) {
			dst_ptr->cur.lut_w[i] = nlm_param[strength_level].lut_w.lut_w[i];
		}

		if (vst_param != NULL) {
			addr = (cmr_uint)dst_ptr->cur.vst_table_addr;
			memcpy((void *)addr, (void *)vst_param[strength_level].vst_param, dst_ptr->cur.vst_len);
		}

		if (ivst_param != NULL) {
			addr = (cmr_uint)dst_ptr->cur.ivst_table_addr;
			memcpy((void *)addr, (void *)ivst_param[strength_level].ivst_param, dst_ptr->cur.ivst_len);
		}

		dst_ptr->cur.flat_opt_bypass = nlm_param[strength_level].first_lum.nlm_flat_opt_bypass;
		dst_ptr->cur.first_lum_byapss = nlm_param[strength_level].first_lum.first_lum_bypass;
		dst_ptr->cur.lum_th0 = nlm_param[strength_level].first_lum.lum_thr0;
		dst_ptr->cur.lum_th1 = nlm_param[strength_level].first_lum.lum_thr1;

		dst_ptr->cur.nlm_direction_addback_mode_bypass = nlm_param[strength_level].first_lum.dal[0].mode_bypass;
		for (i = 0; i < 3; i++) {
			dst_ptr->cur.w_shift[i] = nlm_param[strength_level].nlm_dic.w_shift[i];
			dst_ptr->cur.lum_flat_dec_strenth[i] = nlm_param[strength_level].first_lum.nlm_lum[i].nlm_texture.texture_dec_str;

			dst_ptr->cur.nlm_first_lum_flat_thresh_coef[i][0] = nlm_param[strength_level].first_lum.nlm_flat_thr[i].flat_thresh_coef0;
			dst_ptr->cur.nlm_first_lum_flat_thresh_max[i][0] = nlm_param[strength_level].first_lum.nlm_flat_thr[i].flat_thresh_max0;
			dst_ptr->cur.nlm_first_lum_flat_thresh_coef[i][1] = nlm_param[strength_level].first_lum.nlm_flat_thr[i].flat_thresh_coef1;
			dst_ptr->cur.nlm_first_lum_flat_thresh_max[i][1] = nlm_param[strength_level].first_lum.nlm_flat_thr[i].flat_thresh_max1;
			dst_ptr->cur.nlm_first_lum_flat_thresh_coef[i][2] = nlm_param[strength_level].first_lum.nlm_flat_thr[i].flat_thresh_coef2;
			dst_ptr->cur.nlm_first_lum_flat_thresh_max[i][2] = nlm_param[strength_level].first_lum.nlm_flat_thr[i].flat_thresh_max2;

			for (j = 0; j < 3; j++) {
				dst_ptr->cur.lum_flat[i][j].inc_strength = nlm_param[strength_level].first_lum.nlm_lum[i].nlm_flat[j].flat_inc_str;
				dst_ptr->cur.lum_flat[i][j].match_count = nlm_param[strength_level].first_lum.nlm_lum[i].nlm_flat[j].flat_match_cnt;
				dst_ptr->cur.lum_flat[i][j].thresh = nlm_param[strength_level].first_lum.nlm_lum[i].nlm_flat[j].flat_thresh;

				dst_ptr->cur.lum_flat_addback0[i][j] = nlm_param[strength_level].first_lum.nlm_lum[i].nlm_flat[j].addback0;	//for G channel
				dst_ptr->cur.lum_flat_addback1[i][j] = nlm_param[strength_level].first_lum.nlm_lum[i].nlm_flat[j].addback1;	//for R and B channel
				dst_ptr->cur.lum_flat_addback_max[i][j] = nlm_param[strength_level].first_lum.nlm_lum[i].nlm_flat[j].addback_clip_max;	//plus noise
				dst_ptr->cur.lum_flat_addback_min[i][j] = nlm_param[strength_level].first_lum.nlm_lum[i].nlm_flat[j].addback_clip_min;	//minus noise
			}
			dst_ptr->cur.lum_flat_addback0[i][3] = nlm_param[strength_level].first_lum.nlm_lum[i].nlm_texture.addback30;
			dst_ptr->cur.lum_flat_addback1[i][3] = nlm_param[strength_level].first_lum.nlm_lum[i].nlm_texture.addback31;
			dst_ptr->cur.lum_flat_addback_max[i][3] = nlm_param[strength_level].first_lum.nlm_lum[i].nlm_texture.addback_clip_max;
			dst_ptr->cur.lum_flat_addback_min[i][3] = nlm_param[strength_level].first_lum.nlm_lum[i].nlm_texture.addback_clip_min;

			for (j = 0; j < 4; j++) {
				dst_ptr->cur.nlm_radial_1D_radius_threshold_filter_ratio[i][j] = nlm_param[strength_level].radius_1d.radius[i][j].radius_threshold_filter_ratio;
				dst_ptr->cur.nlm_radial_1D_radius_threshold_filter_ratio_factor[i][j] = nlm_param[strength_level].radius_1d.radius[i][j].radius_threshold_filter_ratio_factor;
				dst_ptr->cur.nlm_radial_1D_coef2[i][j] = nlm_param[strength_level].radius_1d.radius[i][j].coef2;
				dst_ptr->cur.nlm_radial_1D_protect_gain_min[i][j] = nlm_param[strength_level].radius_1d.radius[i][j].protect_gain_min;

				dst_ptr->cur.nlm_first_lum_direction_addback[i][j] = nlm_param[strength_level].first_lum.dal[i].da[j].direction_addback;
				dst_ptr->cur.nlm_first_lum_direction_addback_noise_clip[i][j] = nlm_param[strength_level].first_lum.dal[i].da[j].direction_addback_noise_clip;
			}
		}

		dst_ptr->cur.radius_bypass = nlm_param[strength_level].radius_1d.cal_radius_bypass;
		dst_ptr->cur.nlm_radial_1D_bypass = nlm_param[strength_level].radius_1d.radial_1D_bypass;
		dst_ptr->cur.update_flat_thr_bypass = nlm_param[strength_level].radius_1d.update_flat_thr_bypass;
		dst_ptr->cur.nlm_radial_1D_center_x = nlm_param[strength_level].radius_1d.center_x;
		dst_ptr->cur.nlm_radial_1D_center_y = nlm_param[strength_level].radius_1d.center_y;
		dst_ptr->cur.nlm_radial_1D_radius_threshold = nlm_param[strength_level].radius_1d.radius_threshold;
		dst_ptr->cur.nlm_radial_1D_protect_gain_max = nlm_param[strength_level].radius_1d.protect_gain_max;

		dst_ptr->cur.nlm_radial_1D_radius_threshold_factor = nlm_param[strength_level].radius_1d.radius_threshold_factor;
		dst_ptr->cur.radius_base = nlm_param[strength_level].radius_base;

		dst_ptr->cur.simple_bpc_bypass = nlm_param[strength_level].simple_bpc.simple_bpc_bypass;
		dst_ptr->cur.simple_bpc_lum_th = nlm_param[strength_level].simple_bpc.simple_bpc_lum_thr;
		dst_ptr->cur.simple_bpc_th = nlm_param[strength_level].simple_bpc.simple_bpc_thr;
		dst_ptr->cur.imp_opt_bypass = nlm_param[strength_level].imp_opt_bypass;
		dst_ptr->cur.bypass = nlm_param[strength_level].nlm_bypass;
	}

	return rtn;
}

cmr_s32 _pm_nlm_init(void *dst_nlm_param, void *src_nlm_param, void *param1, void *param_ptr2)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_nlm_param *dst_ptr = (struct isp_nlm_param *)dst_nlm_param;
	struct isp_pm_nr_header_param *src_ptr = (struct isp_pm_nr_header_param *)src_nlm_param;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param1;
	UNUSED(param_ptr2);

	dst_ptr->cur.bypass = header_ptr->bypass;
	dst_ptr->cur.vst_bypass = header_ptr->bypass;
	dst_ptr->cur.ivst_bypass = header_ptr->bypass;
	dst_ptr->vst_map.size = VST_IVST_NUM * sizeof(cmr_u32);
	if (PNULL == dst_ptr->vst_map.data_ptr) {
		dst_ptr->vst_map.data_ptr = (void *)malloc(dst_ptr->vst_map.size);
		if (PNULL == dst_ptr->vst_map.data_ptr) {
			ISP_LOGE("fail to malloc !");
			rtn = ISP_ERROR;
			return rtn;
		}
	}
	memset((void *)dst_ptr->vst_map.data_ptr, 0x00, dst_ptr->vst_map.size);
	dst_ptr->cur.vst_len = dst_ptr->vst_map.size;
	dst_ptr->cur.vst_table_addr = (cmr_u64)dst_ptr->vst_map.data_ptr;

	dst_ptr->ivst_map.size = VST_IVST_NUM * sizeof(cmr_u32);
	if (PNULL == dst_ptr->ivst_map.data_ptr) {
		dst_ptr->ivst_map.data_ptr = (void *)malloc(dst_ptr->ivst_map.size);
		if (PNULL == dst_ptr->ivst_map.data_ptr) {
			ISP_LOGE("fail to malloc !");
			rtn = ISP_ERROR;
			return rtn;
		}
	}
	memset((void *)dst_ptr->ivst_map.data_ptr, 0x00, dst_ptr->ivst_map.size);
	dst_ptr->cur.ivst_len = dst_ptr->ivst_map.size;
	dst_ptr->cur.ivst_table_addr = (cmr_u64)dst_ptr->ivst_map.data_ptr;

	ISP_LOGV("vst tab %p, ivst tab %p\n", dst_ptr->vst_map.data_ptr, dst_ptr->ivst_map.data_ptr);

	dst_ptr->cur_level = src_ptr->default_strength_level;
	dst_ptr->level_num = src_ptr->level_number;
	dst_ptr->nlm_ptr = src_ptr->param_ptr;
	dst_ptr->vst_ptr = src_ptr->param1_ptr;
	dst_ptr->ivst_ptr = src_ptr->param2_ptr;
	dst_ptr->nr_mode_setting = src_ptr->nr_mode_setting;
	dst_ptr->scene_ptr = src_ptr->multi_nr_map_ptr;
	if (!header_ptr->bypass)
		rtn = _pm_nlm_convert_param(dst_ptr, dst_ptr->cur_level, ISP_MODE_ID_COMMON, ISP_SCENEMODE_AUTO);
	dst_ptr->cur.bypass |= header_ptr->bypass;
	dst_ptr->cur.vst_bypass = dst_ptr->cur.bypass;
	dst_ptr->cur.ivst_bypass = dst_ptr->cur.bypass;
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to  convert pm nlm param!");
		return rtn;
	}

	header_ptr->is_update = ISP_ONE;

	return rtn;
}

cmr_s32 _pm_nlm_set_param(void *nlm_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_nlm_param *nlm_ptr = (struct isp_nlm_param *)nlm_param;
	struct isp_pm_block_header *nlm_header_ptr = (struct isp_pm_block_header *)param_ptr1;

	switch (cmd) {
	case ISP_PM_BLK_NLM_BYPASS:
		nlm_ptr->cur.bypass = *((cmr_u32 *) param_ptr0);
		nlm_header_ptr->is_update = ISP_ONE;
		break;

	case ISP_PM_BLK_SMART_SETTING:
		{
			struct smart_block_result *block_result = (struct smart_block_result *)param_ptr0;
			struct isp_range val_range = { 0, 0 };
			cmr_u32 nlm_level = 0;

			if (!block_result->update || nlm_header_ptr->bypass) {
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

			nlm_level = (cmr_u32) block_result->component[0].fix_data[0];

			if (nlm_level != nlm_ptr->cur_level || nr_tool_flag[ISP_BLK_NLM_T] || block_result->mode_flag_changed) {
				nlm_ptr->cur_level = nlm_level;
				nlm_header_ptr->is_update = ISP_ONE;
				nr_tool_flag[ISP_BLK_NLM_T] = 0;

				rtn = _pm_nlm_convert_param(nlm_ptr, nlm_ptr->cur_level, nlm_header_ptr->mode_id, block_result->scene_flag);
				nlm_ptr->cur.bypass |= nlm_header_ptr->bypass;
				nlm_ptr->cur.vst_bypass = nlm_ptr->cur.bypass;
				nlm_ptr->cur.ivst_bypass = nlm_ptr->cur.bypass;
				if (ISP_SUCCESS != rtn) {
					ISP_LOGE("fail to  convert pm nlm param!");
					return rtn;
				}
			}
			ISP_LOGV("ISP_SMART_NR: cmd=%d, update=%d, nlm_level=%d",
				cmd, nlm_header_ptr->is_update, nlm_ptr->cur_level);
		}
		break;

	case ISP_PM_BLK_NLM_FDR_UPDATE: {
		cmr_s32 i, j;
		struct isp_nlm_factor *in = (struct isp_nlm_factor *)param_ptr0;
		struct isp_dev_nlm_info_v2 *cur = &nlm_ptr->cur;

		if (in->nlm_out_ratio0 != 0) {
			for (i = 0; i < 3; i++) {
				for (j = 0; j < 3; j++) {
					cmr_s32 val = (cmr_s32)cur->lum_flat[i][j].thresh;
					ISP_LOGV("thresh  %d, val %d\n", cur->lum_flat[i][j].thresh, val);
					val = val + in->nlm_out_ratio0;
					val = MAX(0, MIN(val, 9207));
					cur->lum_flat[i][j].thresh = (cmr_u16)(val & 0xFFFF);
				}
			}
		}

		if (in->nlm_out_ratio1 != 0) {
			for (i = 0; i < 3; i++) {
				for (j = 0; j < 4; j++) {
					cmr_s32 val = (cmr_s32)cur->lum_flat_addback_max[i][j];
					ISP_LOGV("lum_flat_addback_max  %d, val %d\n",
						cur->lum_flat_addback_max[i][j], val);
					val = val + in->nlm_out_ratio1;
					val = MAX(0, MIN(val, 1023));
					cur->lum_flat_addback_max[i][j] = (cmr_u16)(val & 0xFFFF);

					val = (cmr_s32)cur->lum_flat_addback_min[i][j];
					/* lum_flat_addback_min should be signed int, maybe negtive */
					ISP_LOGV("orig %d, val %d\n", cur->lum_flat_addback_min[i][j], val);
					val <<= 16;
					val >>= 16;
					ISP_LOGV("new val %d\n", val);

					val = val - in->nlm_out_ratio1;
					val =  MAX(-1024, MIN(val, 0));
					cur->lum_flat_addback_min[i][j] = (cmr_u16)(val & 0xFFFF);
				}
			}
		}

		if (in->nlm_out_ratio2 != 0) {
			for (i = 0; i < 3; i++) {
				for (j = 0; j < 3; j++) {
					cmr_s32 val = (cmr_s32)cur->nlm_first_lum_flat_thresh_coef[i][j];
					ISP_LOGV("nlm_first_lum_flat_thresh_coef  %d, val %d\n",
						cur->nlm_first_lum_flat_thresh_coef[i][j], val);

					val <<= 16;
					val >>= 16;
					val = val + in->nlm_out_ratio2;
					val = MAX(-8192, MIN(val, 8191));
					cur->nlm_first_lum_flat_thresh_coef[i][j]  = (cmr_u32)val;
				}
			}
		}

		if (in->nlm_out_ratio3 != 0) {
			for (i = 0; i < 3; i++) {
				for (j = 0; j < 3; j++) {
					cmr_s32 val = (cmr_s32)cur->nlm_first_lum_flat_thresh_max[i][j];

					ISP_LOGV("nlm_first_lum_flat_thresh_max  %d, val %d\n",
						cur->nlm_first_lum_flat_thresh_max[i][j], val);

					val = val + in->nlm_out_ratio3;
					val = MAX(0, MIN(val, 9207));
					cur->nlm_first_lum_flat_thresh_max[i][j]  = val;
				}
			}
		}

		if (in->nlm_out_ratio4 != 0) {
			for (i = 0; i < 3; i++) {
				for (j = 0; j < 3; j++) {
					cmr_s32 val = (cmr_s32)cur->lum_flat[i][j].inc_strength;

					ISP_LOGV("nlm_lum_flat_inc_strenth %d, val %d\n",
						cur->lum_flat[i][j].inc_strength, val);

					val = val + in->nlm_out_ratio4;
					cur->lum_flat[i][j].inc_strength = (cmr_u16)MAX(0, MIN(val, 192));
				}
			}
			for (i = 0; i < 3; i++) {
				cmr_s32 val = (cmr_s32)cur->lum_flat_dec_strenth[i];

				ISP_LOGV("nlm_lum_flat_dec_strenth %d, val %d\n",
					cur->lum_flat_dec_strenth[i], val);

				val = val + in->nlm_out_ratio4;
				cur->lum_flat_dec_strenth[i] = (cmr_u32)MAX(0, MIN(val, 192));
			}
		}

		nlm_header_ptr->is_update = ISP_ONE;
		break;
	}
	default:
		break;
	}

	return rtn;
}

cmr_s32 _pm_nlm_get_param(void *nlm_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_nlm_param *nlm_ptr = (struct isp_nlm_param *)nlm_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	cmr_u32 *update_flag = (cmr_u32 *) rtn_param1;

	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = &nlm_ptr->cur;
		param_data_ptr->data_size = sizeof(nlm_ptr->cur);
		*update_flag = 0;
		break;

	default:
		break;
	}

	return rtn;
}

cmr_s32 _pm_nlm_deinit(void *nlm_param)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_nlm_param *nlm_ptr = (struct isp_nlm_param *)nlm_param;

	if (PNULL != nlm_ptr->vst_map.data_ptr) {
		free(nlm_ptr->vst_map.data_ptr);
		nlm_ptr->vst_map.data_ptr = PNULL;
		nlm_ptr->vst_map.size = 0;
	}

	if (PNULL != nlm_ptr->ivst_map.data_ptr) {
		free(nlm_ptr->ivst_map.data_ptr);
		nlm_ptr->ivst_map.data_ptr = PNULL;
		nlm_ptr->ivst_map.size = 0;
	}
	return rtn;
}
