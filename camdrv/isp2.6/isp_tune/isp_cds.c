/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "isp_cds_pm"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "isp_mw.h"
#include "isp_pm.h"
#include "sensor_raw.h"
#include "isp_blocks_cfg.h"
#include "isp_param_file_update.h"
#include "cmr_types.h"
#include "isp_cds.h"


static struct isp_cds_pm_init_blk pm_init_blk_array[] = {
	{ISP_BLK_ALSC, ISP_PM_CMD_GET_INIT_ALSC},
	{ISP_BLK_AWB_NEW, ISP_PM_CMD_GET_INIT_AWB},
	{ISP_BLK_SMART, ISP_PM_CMD_GET_INIT_SMART},
	{ISP_BLK_ATM_TUNE, ISP_PM_CMD_GET_ATM_PARAM},
	{ISP_BLK_AE_NEW, ISP_PM_CMD_GET_INIT_AE},
	{ISP_BLK_AE_SYNC, ISP_PM_CMD_GET_AE_SYNC},
	{ISP_BLK_HDR, ISP_PM_CMD_GET_HDR_PARAM},
	//{ISP_BLK_HDR3, ISP_PM_CMD_GET_HDR_PARAM},
};


static cmr_u32 isp_pmcmd_get_fun(cmr_u32 blk_id)
{
	cmr_u32 pmcmd = -1;
	cmr_u32 total_num = 0;
	cmr_u32 i = 0;

	total_num = sizeof(pm_init_blk_array) / sizeof(struct isp_cds_pm_init_blk);
	for (i = 0; i < total_num; i++) {
		if (blk_id == pm_init_blk_array[i].blk_id) {
			pmcmd = pm_init_blk_array[i].cmd;
			break;
		}
	}

	return pmcmd;
}


cmr_handle isp_cds_pm_init(void *input, char *param_path)
{
	cmr_handle pm_cxt_ptr = NULL;
	cmr_int i = 0;
	struct sensor_raw_info *raw_info_ptr = (struct sensor_raw_info *)input;
	struct isp_pm_init_input *pm_input = (struct isp_pm_init_input *)malloc(sizeof(struct isp_pm_init_input));
	struct isp_pm_init_output *pm_output = (struct isp_pm_init_output *)malloc(sizeof(struct isp_pm_init_output));

	/* init isp_pm_init_input for isp_pm_init */
	memset((void *)pm_input, 0, sizeof(struct isp_pm_init_input));
	memset((void *)pm_output, 0, sizeof(struct isp_pm_init_output));
	pm_output->multi_nr_flag = 0;

	pm_input->is_4in1_sensor = 0;
	pm_input->sensor_raw_info_ptr = raw_info_ptr;
	pm_input->nr_fix_info = &(raw_info_ptr->nr_fix);
	pm_input->push_param_path = (cmr_s8 *)param_path;

	for (i = 0; i < MAX_MODE_NUM; i++) {
		pm_input->tuning_data[i].data_ptr = raw_info_ptr->mode_ptr[i].addr;
		pm_input->tuning_data[i].size = raw_info_ptr->mode_ptr[i].len;
		pm_input->fix_data[i] = raw_info_ptr->fix_ptr[i];
	}

	pm_cxt_ptr = isp_pm_init(pm_input, pm_output);

	return pm_cxt_ptr;
}


static cmr_s32 isp_cds_pm_set_aipro(cmr_handle handle, void *input)
{
	cmr_s32 ret = ISP_SUCCESS;
	struct ai_scene_detect_info *ai_info_ptr = NULL;
	cmr_handle pm_cxt_ptr = NULL;

	cmr_u32 i = 0, j = 0, cmd, offset;
	intptr_t start_addr = 0;
	struct isp_ai_update_param ai_update_data;
	struct isp_pm_mode_param *mode_common_ptr = NULL;
	struct isp_pm_ioctl_input ioctl_output = { PNULL, 0 };
	struct isp_pm_ioctl_input ioctl_input = { PNULL, 0 };
	struct isp_pm_param_data pm_param;
	struct isp_ai_bchs_param *bchs_cur[2] = { PNULL, PNULL };
	struct isp_ai_hsv_info *hsv_cur[2] = { PNULL, PNULL };
	struct isp_ai_ee_param *ee_cur[2] = { PNULL, PNULL };
	cmr_u32 blk_id = 0, blk_num = 0;
	cmr_u32 last_ai_scene = 0;
	cmr_u32 is_ai_scene_pro = 0;
	cmr_u16 cxt_smooth_ratio = 0;
	cmr_u16 smooth_ratio = 0;

#if defined (CONFIG_ISP_2_7) || defined (CONFIG_ISP_2_9)
	cmr_u32 hsv_blk_id = ISP_BLK_HSV_NEW2;
	cmr_u32 ee_blk_id = ISP_BLK_EE_V1;
	cmr_u32 bchs_blk_id[4] = { ISP_BLK_BCHS, ISP_BLK_ID_MAX,  ISP_BLK_ID_MAX, ISP_BLK_ID_MAX};
#elif defined CONFIG_ISP_2_8
	cmr_u32 hsv_blk_id = ISP_BLK_HSV_LUT;
	cmr_u32 ee_blk_id = ISP_BLK_EE_V1;
	cmr_u32 bchs_blk_id[4] = { ISP_BLK_BCHS, ISP_BLK_ID_MAX,  ISP_BLK_ID_MAX, ISP_BLK_ID_MAX};
#elif defined CONFIG_ISP_2_5
	cmr_u32 hsv_blk_id = ISP_BLK_HSV_NEW;
	cmr_u32 ee_blk_id = ISP_BLK_EDGE;
	cmr_u32 bchs_blk_id[4] = { ISP_BLK_BRIGHT, ISP_BLK_CONTRAST,  ISP_BLK_HUE, ISP_BLK_SATURATION};
#elif defined CONFIG_ISP_2_6
	cmr_u32 hsv_blk_id = ISP_BLK_HSV;
	cmr_u32 ee_blk_id = ISP_BLK_EE_V1;
	cmr_u32 bchs_blk_id[4] = { ISP_BLK_BCHS, ISP_BLK_ID_MAX,  ISP_BLK_ID_MAX, ISP_BLK_ID_MAX};
#endif

	pm_cxt_ptr = handle;
	start_addr = (intptr_t)pm_cxt_ptr;
	offset = sizeof(cmr_u32) * 6 + sizeof(pthread_mutex_t) + sizeof(void *) * (1 + PARAM_SET_MAX)
			+ sizeof(struct isp_context) * PARAM_SET_MAX
			+ sizeof(struct isp_pm_blocks_param) * PARAM_SET_MAX
			+ sizeof(struct isp_pm_param_data) * ISP_TUNE_BLOCK_MAX;
	mode_common_ptr = (struct isp_pm_mode_param *)(start_addr + offset);

	ai_info_ptr = (struct ai_scene_detect_info *)input;

	blk_num = mode_common_ptr->block_num;
	for(i = 0; i < blk_num; i++){
		blk_id = mode_common_ptr->header[i].block_id;
		if ( blk_id == ISP_BLK_AI_PRO_V1){
			is_ai_scene_pro = !mode_common_ptr->header[i].bypass;
			ISP_LOGV("is_ai_scene_pro = %d, blk_num = %d, i = %d", is_ai_scene_pro, blk_num, i);
			break;
		}
	}

	if (!is_ai_scene_pro || ai_info_ptr == NULL){
		ISP_LOGE("ai pro function is invalid!");
		return ret;
	}

	ISP_LOGV("ai_pro, last_ai_scene %d, cur_scene %d", last_ai_scene, ai_info_ptr->cur_scene_id);

	memset(&pm_param, 0, sizeof(pm_param));
	BLOCK_PARAM_CFG(ioctl_input, pm_param,
			ISP_PM_BLK_AI_SCENE_SMOOTH,
			ISP_BLK_AI_PRO_V1, PNULL, 0);
	ret = isp_pm_ioctl(pm_cxt_ptr, ISP_PM_CMD_GET_SINGLE_SETTING,
			(void *)&ioctl_input, (void *)&ioctl_output);
	if(ret){
		ISP_LOGE("fail to get AI PRO PARAM");
		return ret;
	}

	if (ioctl_output.param_num == 1 && ioctl_output.param_data_ptr && ioctl_output.param_data_ptr->data_ptr)
		smooth_ratio = *(cmr_u16 *)ioctl_output.param_data_ptr->data_ptr;

	if (smooth_ratio == 0) {
		ISP_LOGW("warning: smooth ratio is 0\n");
		smooth_ratio = 1;
	}

	if (last_ai_scene != ai_info_ptr->cur_scene_id) {
		ISP_LOGD("ai_pro, last ai_scene %d, cur scene %d, smooth %d", last_ai_scene, ai_info_ptr->cur_scene_id, smooth_ratio);

		cxt_smooth_ratio = smooth_ratio;
		last_ai_scene = ai_info_ptr->cur_scene_id;
		if (last_ai_scene == ISP_PM_AI_SCENE_PORTRAIT ||
			last_ai_scene == ISP_PM_AI_SCENE_NIGHT) {
			cxt_smooth_ratio = smooth_ratio = 1;
		}
	}

	ai_update_data.ai_status = 1;

	if (cxt_smooth_ratio == 0) {
		ai_update_data.smooth_factor = 0;
		ai_update_data.smooth_base = (cmr_s16)smooth_ratio;
	} else {
		ai_update_data.smooth_factor = (cmr_s16)smooth_ratio - cxt_smooth_ratio + 1;
		ai_update_data.smooth_base = (cmr_s16)smooth_ratio;
		ai_update_data.ai_scene = last_ai_scene;
		if (!last_ai_scene){
			ai_update_data.count = ai_update_data.smooth_factor;
			ai_update_data.smooth_factor = -1;
		}
	}

	/* addback hsv ai param */
	memset(&pm_param, 0, sizeof(pm_param));
	cmd = ISP_PM_CMD_GET_SINGLE_SETTING;
	BLOCK_PARAM_CFG(ioctl_input, pm_param,
			ISP_PM_BLK_AI_SCENE_HSV_PARAM,
			ISP_BLK_AI_PRO_V1, PNULL, 0);
	ret = isp_pm_ioctl(pm_cxt_ptr, cmd, (void *)&ioctl_input, (void *)&ioctl_output);
	if(ret)
		ISP_LOGE("fail to get hsv ai param");
	if (ioctl_output.param_num == 1 && ioctl_output.param_data_ptr && ioctl_output.param_data_ptr->data_ptr)
		hsv_cur[0] = (struct isp_ai_hsv_info *)ioctl_output.param_data_ptr->data_ptr;

	if (hsv_cur[0] && hsv_cur[0]->hue_adj_ai_eb) {
		if (cxt_smooth_ratio == 0 && !last_ai_scene)
			ISP_LOGV("do not update hsv ai param");
		else {
			ai_update_data.param_ptr = (void *)hsv_cur[0];
			memset(&pm_param, 0, sizeof(pm_param));
			BLOCK_PARAM_CFG(ioctl_input, pm_param,
				ISP_PM_BLK_AI_SCENE_UPDATE_HSV,
				hsv_blk_id,
				&ai_update_data,
				sizeof(struct isp_ai_update_param));

			ioctl_input.param_data_ptr->mode_id = ISP_MODE_ID_CAP_0;
			ret = isp_pm_ioctl(pm_cxt_ptr, ISP_PM_CMD_SET_AI_SCENE_PARAM, &ioctl_input, NULL);
		}
	}

	/* addback bchs ai param */
	memset(&pm_param, 0, sizeof(pm_param));
	cmd = ISP_PM_CMD_GET_SINGLE_SETTING;
	BLOCK_PARAM_CFG(ioctl_input, pm_param,
			ISP_PM_BLK_AI_SCENE_BCHS_PARAM,
			ISP_BLK_AI_PRO_V1, PNULL, 0);
	ret = isp_pm_ioctl(pm_cxt_ptr, cmd, (void *)&ioctl_input, (void *)&ioctl_output);
	if(ret)
		ISP_LOGE("fail to get bchs ai param");
	if (ioctl_output.param_num == 1 && ioctl_output.param_data_ptr && ioctl_output.param_data_ptr->data_ptr)
		bchs_cur[0] = (struct isp_ai_bchs_param *)ioctl_output.param_data_ptr->data_ptr;

	if (bchs_cur[0]) {
		if (cxt_smooth_ratio == 0 && !last_ai_scene)
			ISP_LOGV("do not update bchs ai param");
		else {
			ai_update_data.param_ptr = (void *)bchs_cur[0];
			for (j = 0; j < 4; j++) {
				if (bchs_blk_id[j] == ISP_BLK_ID_MAX)
					break;
				memset(&pm_param, 0, sizeof(pm_param));
				BLOCK_PARAM_CFG(ioctl_input, pm_param,
					ISP_PM_BLK_AI_SCENE_UPDATE_BCHS,
					bchs_blk_id[j],
					&ai_update_data,
					sizeof(struct isp_ai_update_param));

				ioctl_input.param_data_ptr->mode_id = ISP_MODE_ID_CAP_0;
				ret = isp_pm_ioctl(pm_cxt_ptr, ISP_PM_CMD_SET_AI_SCENE_PARAM, &ioctl_input, NULL);
			}
		}
	}

	/* addback ee ai param */
	memset(&pm_param, 0, sizeof(pm_param));
	cmd = ISP_PM_CMD_GET_SINGLE_SETTING;
	BLOCK_PARAM_CFG(ioctl_input, pm_param,
			ISP_PM_BLK_AI_SCENE_EE_PARAM,
			ISP_BLK_AI_PRO_V1, PNULL, 0);
	ret = isp_pm_ioctl(pm_cxt_ptr, cmd, (void *)&ioctl_input, (void *)&ioctl_output);
	if(ret)
		ISP_LOGE("fail to get ee ai param");
	if (ioctl_output.param_num == 1 && ioctl_output.param_data_ptr && ioctl_output.param_data_ptr->data_ptr)
		ee_cur[0] = (struct isp_ai_ee_param *)ioctl_output.param_data_ptr->data_ptr;

	ISP_LOGV("ai scene %d,  ee %p, enable %d", last_ai_scene, ee_cur[0], ee_cur[0]->ee_enable);
	if (ee_cur[0] && ee_cur[0] ->ee_enable) {
		if (cxt_smooth_ratio == 0 && !last_ai_scene)
			ISP_LOGV("do not update ee ai param");
		else {
			ai_update_data.param_ptr = (void *)ee_cur[0];
			memset(&pm_param, 0, sizeof(pm_param));
			BLOCK_PARAM_CFG(ioctl_input, pm_param,
				ISP_PM_BLK_AI_SCENE_UPDATE_EE,
				ee_blk_id, &ai_update_data,
				sizeof(struct isp_ai_update_param));

			ioctl_input.param_data_ptr->mode_id = ISP_MODE_ID_CAP_0;
			ret = isp_pm_ioctl(pm_cxt_ptr, ISP_PM_CMD_SET_AI_SCENE_PARAM, &ioctl_input, NULL);
		}
	}

	if (cxt_smooth_ratio > 0)
		cxt_smooth_ratio--;
	ISP_LOGV("cxt_smooth_ratio after %d.\n", cxt_smooth_ratio);
	return ret;
}


cmr_s32 isp_cds_pm_set_param(cmr_handle handle, enum isp_cds_pm_cmd cmd, void *input, void *output1, void *output2)
{
	cmr_s32 ret = ISP_SUCCESS;
	cmr_handle pm_cxt_ptr = handle;
	struct cds_pm_start *pm_start_ptr = NULL;
	struct pm_workmode_input  pm_input;
	struct pm_workmode_output pm_output;
	struct smart_calc_result *smart_result = NULL;
	struct smart_block_result *block_result = NULL;
	struct isp_pm_ioctl_input io_pm_input = { NULL, 0 };
	struct isp_pm_param_data pm_param;

	int i = 0;

	UNUSED(output2);

	if (PNULL == pm_cxt_ptr) {
		ISP_LOGE("fail to get valid param : pm_cxt_ptr is NULL");
		ret = ISP_ERROR;
		return ret;
	}

	switch (cmd) {
	case ISP_CDS_PM_CMD_SET_MODE:
	{
		pm_start_ptr = (struct cds_pm_start *)input;
		memset(&pm_input, 0, sizeof(struct pm_workmode_input));
		memset(&pm_output, 0, sizeof(struct pm_workmode_output));
		pm_input.pm_sets_num = 1;
		pm_input.mode[0] = pm_start_ptr->work_mode;
		pm_input.img_w[0] = pm_start_ptr->size.w;
		pm_input.img_h[0] = pm_start_ptr->size.h;
		pm_input.remosaic_type = pm_start_ptr->remosaic_type;

		ISP_LOGI("img size w %d, img size h %d", pm_start_ptr->size.w, pm_start_ptr->size.h);
		ret = isp_pm_ioctl(pm_cxt_ptr, ISP_PM_CMD_SET_MODE, &pm_input, &pm_output);
		ISP_RETURN_IF_FAIL(ret, ("fail to do isp_pm_ioctl"));

		*(cmr_u32 *)output1 = pm_output.mode_id[0];
		break;
	}
	case ISP_CDS_PM_CMD_SET_SMART:
	{
		smart_result = (struct smart_calc_result *)input;
		memset(&pm_param, 0, sizeof(pm_param));

		for (i = 0; i < smart_result->counts; i++) {
			block_result = &smart_result->block_result[i];
	
			if (ISP_SMART_LNC == block_result->smart_id){
				output1 = (void *)block_result->component[0].fix_data;
			}
			if (block_result->update == 0)
				continue;
			if (block_result->block_id == ISP_BLK_RGB_GAMC) {
				ISP_LOGV("skip gamma\n");
				continue;
			}
			
			BLOCK_PARAM_CFG(io_pm_input, pm_param, ISP_PM_BLK_SMART_SETTING,
					block_result->block_id, block_result, sizeof(*block_result));
			pm_param.mode_id = block_result->mode_flag;
			ISP_LOGV("set param %d, id=%x, mode %d, data=%p", i, block_result->block_id, pm_param.mode_id, block_result);
	
			ret = isp_pm_ioctl(pm_cxt_ptr, ISP_PM_CMD_SET_SMART, &io_pm_input, NULL);
			if(ret) ISP_LOGE("fail to set smart\n");
		}
		break;
	}
	case ISP_CDS_PM_CMD_SET_GAMMA:
	{
		void *gamma_cur = input;
		memset(&pm_param, 0, sizeof(pm_param));

		cmr_u32 cfg_set_id = PARAM_SET0;
		memset(&pm_param, 0, sizeof(pm_param));
		BLOCK_PARAM_CFG(io_pm_input, pm_param, ISP_PM_BLK_GAMMA_CUR, ISP_BLK_RGB_GAMC,
				gamma_cur, sizeof(struct sensor_rgbgamma_curve));
		ret = isp_pm_ioctl(pm_cxt_ptr, ISP_PM_CMD_SET_OTHERS, &io_pm_input, &cfg_set_id);
		if(ret) ISP_LOGE("fail to set gammar cur\n");
		break;
	}
	case ISP_CDS_PM_CMD_SET_AI_PRO:
	{
		ret = isp_cds_pm_set_aipro(pm_cxt_ptr, input);
		break;
	}
	default:
		break;
	}

	return ret;
}


cmr_s32 isp_cds_pm_get_param(cmr_handle handle, enum isp_cds_pm_cmd cmd, void *input, void *output1, void *output2)
{
	cmr_s32 ret = ISP_SUCCESS;
	cmr_handle pm_cxt_ptr = handle;
	intptr_t start_addr = 0;
	intptr_t isp_cxt_ptr = 0;
	struct isp_pm_ioctl_input pm_input;
	struct isp_pm_ioctl_output pm_output;
	struct isp_block_cfg *blk_cfg = NULL;
	struct dynamic_block_info *blk_ptr = NULL;
	struct dynamic_block_addr *block_addr_ptr = NULL;
	struct isp_pm_param_data *param_data = NULL;
	struct isp_pm_blocks_param *block_param_data = NULL;
	struct cds_pm_init_blk *blk_array_ptr = NULL;
	struct isp_pm_ioctl_input *ioctl_output = NULL;
	struct isp_pm_ioctl_input ioctl_input;
	struct isp_pm_setting_params setting_output;
	struct isp_pm_param_data ioctl_data;

	cmr_s32 param_set = 0;
	cmr_s32 size = 0;
	cmr_s32 blk_id;
	cmr_s32 offset;
	int i = 0, counts = 0;
	cmr_u32 pmcmd;

	if (PNULL == pm_cxt_ptr) {
		ISP_LOGE("fail to get valid param : pm_cxt_ptr is NULL");
		ret = ISP_ERROR;
		return ret;
	}

	switch (cmd) {
	case ISP_CDS_PM_CMD_GET_GAMMA_TAB:
	{
		memset(&ioctl_data, 0, sizeof(ioctl_data));
		ioctl_output = output1;
		BLOCK_PARAM_CFG(ioctl_input, ioctl_data, ISP_PM_BLK_GAMMA_TAB,
			ISP_BLK_RGB_GAMC, PNULL, 0);
		ret = isp_pm_ioctl(pm_cxt_ptr, ISP_PM_CMD_GET_SINGLE_SETTING, (void *)&ioctl_input, (void *)ioctl_output);

		if(!ret && (ioctl_output->param_num == 1) && ioctl_output->param_data_ptr) {
			ISP_LOGV("param_data_ptr %p data_ptr %p", ioctl_output->param_data_ptr, ioctl_output->param_data_ptr->data_ptr);
		} else {
			ISP_LOGE("fail to get gamma tab");
			output1 = NULL;
			ret = ISP_ERROR;
		}
		break;
	}
	case ISP_CDS_PM_CMD_GET_INIT_PARAM:
	{
		blk_array_ptr = (struct cds_pm_init_blk *)input;
		memset((void *)&pm_input, 0, sizeof(pm_input));
		memset((void *)&pm_output, 0, sizeof(pm_output));
		size = ISP_TUNE_BLOCK_MAX * sizeof(struct isp_pm_param_data);

		while(0 != blk_array_ptr->blk_id) {
			pmcmd = isp_pmcmd_get_fun(blk_array_ptr->blk_id);
			if(-1 == pmcmd) {
				blk_array_ptr++;
				continue;
			}
			blk_array_ptr->param_data_ptr = (struct isp_pm_param_data *)malloc(size);
			ret = isp_pm_ioctl(pm_cxt_ptr, pmcmd, &pm_input, &pm_output);
			if (0 != ret) {
				ISP_LOGE("fail to do isp_pm_ioctl for 0x%x", blk_array_ptr->blk_id);
			}

			memcpy(blk_array_ptr->param_data_ptr, (void *)pm_output.param_data, size);
			blk_array_ptr->param_size = size;
			blk_array_ptr->param_counts = pm_output.param_num;
			blk_array_ptr++;
		}
		break;
	}
	case ISP_CDS_PM_CMD_GET_ALL_PM_PARAM:
	{
		blk_ptr = (struct dynamic_block_info *)output1;
		block_addr_ptr = (struct dynamic_block_addr *)output2;
		start_addr = (intptr_t)pm_cxt_ptr;
		offset = sizeof(cmr_u32)*6 + sizeof(pthread_mutex_t) + sizeof(void *);
		isp_cxt_ptr = start_addr + offset;
		offset = offset + sizeof(struct isp_context) * PARAM_SET_MAX;
		block_param_data = (struct isp_pm_blocks_param *)(start_addr + offset);

		blk_ptr->blk_num = block_param_data[param_set].block_num;
		ISP_LOGI("cds pm block num all %d\n", blk_ptr->blk_num);

		for(i = 0; i < blk_ptr->blk_num; i++){
			blk_id = block_param_data[param_set].header[i].block_id;
			blk_cfg = isp_pm_get_block_cfg(blk_id);
			if(NULL == blk_cfg){
				continue;
			}
			blk_ptr->blk_id[counts] = blk_id;
			block_addr_ptr->blk_id = blk_id;
			block_addr_ptr->data_addr = (void *)(isp_cxt_ptr + blk_cfg->offset);
			block_addr_ptr->data_size = blk_cfg->param_size;
			block_addr_ptr++;
			counts++;
		}
		blk_ptr->blk_num = counts;
		ISP_LOGI("cds pm block num %d\n", blk_ptr->blk_num);
		break;
	}
	case ISP_CDS_PM_CMD_GET_ALL_ISP_PARAM:
	{
		blk_ptr = (struct dynamic_block_info *)output1;
		block_addr_ptr = (struct dynamic_block_addr *)output2;
		memset((void *)&setting_output, 0x00, sizeof(struct isp_pm_setting_params));
		ISP_LOGI("init for isp_pm_ioctl");
		ret = isp_pm_ioctl(pm_cxt_ptr, ISP_PM_CMD_GET_ISP_ALL_SETTING, NULL, &setting_output);
		if (0 != ret) {
			ISP_LOGE("fail to do isp_pm_ioctl");
			return ret;
		}

		param_data = setting_output.prv_param_data;
		blk_ptr->blk_num = setting_output.prv_param_num;
		input = (void *)param_data;
		ISP_LOGI("cds isp block num %d\n", blk_ptr->blk_num);

		for(i = 0; i < blk_ptr->blk_num; i++, param_data++){
			blk_id = param_data->id;
			blk_ptr->blk_id[i] = blk_id;
			block_addr_ptr->blk_id = blk_id;
			block_addr_ptr->data_addr = param_data->data_ptr;
			block_addr_ptr->data_size = param_data->data_size;
			block_addr_ptr++;
		}
		break;
	}
	default:
		break;
	}

	return ret;
}


cmr_s32 isp_cds_pm_ioctl(cmr_handle handle, enum isp_cds_pm_cmd cmd, void *input, void *output1, void *output2)
{
	cmr_s32 ret = ISP_SUCCESS;

	if (NULL == handle) {
		ISP_LOGE("pm handle is invaid");
		return ISP_ERROR;
	}

	switch ((cmd & isp_cds_pm_cmd_mask)) {
	case ISP_CDS_PM_CMD_SET_BASE:
		ret = isp_cds_pm_set_param(handle, cmd, input, output1, output2);
		break;
	case ISP_CDS_PM_CMD_GET_BASE:
		ret = isp_cds_pm_get_param(handle, cmd, input, output1, output2);
		break;
	default:
		break;
	}

	return ret;
}


cmr_s32 isp_cds_pm_deinit(cmr_handle handle)
{
	cmr_s32 ret = ISP_SUCCESS;

	ret = isp_pm_deinit(handle);

	return ret;
}