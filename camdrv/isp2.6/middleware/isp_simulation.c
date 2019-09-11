/*
 * Copyright (C) 2012 The Android Open Source Project
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

#define LOG_TAG "isp_simulation"

#include "cmr_log.h"
#include "cmr_common.h"
#include "isp_com.h"

static struct isptool_scene_param g_scene_param;
static char s_r_file_name[200] = { 0 };

cmr_int isp_sim_set_mipi_raw_file_name(char *file_name)
{
	if (file_name) {
		strcpy(s_r_file_name, file_name);
	}
	return 0;
}

cmr_int isp_sim_get_mipi_raw_file_name(char *file_name)
{
	if (file_name)
		strncpy(file_name, s_r_file_name, strrchr(s_r_file_name, '.')-s_r_file_name);

	return 0;
}

cmr_int isp_sim_get_no_lsc_ae_stats(struct isp_awb_statistic_info *awb_statis, cmr_u32 *stat_w, cmr_u32 *stat_h)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_u32 i = 0;
	FILE *fp = NULL;
	char *tmp_ptr = NULL;
	char file_name[260] = { 0 };

	if (awb_statis == NULL) {
		ISP_LOGE("fail to get ae statis pointer.");
		return ISP_ERROR;
	}
	tmp_ptr = s_r_file_name;
	if (strlen(tmp_ptr)) {
		strncpy(file_name, tmp_ptr, strrchr(tmp_ptr, '.')-tmp_ptr);
		strcat(file_name, ".no_lsc_aem");
	} else {
		ISP_LOGE("fail to get mipi raw file name.");
		return ISP_ERROR;
	}

	fp = fopen(file_name, "rb");
	if (fp == NULL) {
		ISP_LOGE("fail to open file");
		return ISP_ERROR;
	}

	if(EOF==fscanf(fp, "stat_w:%d\n", stat_w)){
                ISP_LOGD("end to file");
	}
	if(EOF==fscanf(fp, "stat_h:%d\n", stat_h)){
		ISP_LOGD("end to file");
	}
	while (!feof(fp)) {
		if(EOF==fscanf(fp, "blk_id:%d ", &i)){
			ISP_LOGD("end to file");
		}
		if(EOF==fscanf(fp, "R:%d G:%d B:%d\n",
			&awb_statis->r_info[i], &awb_statis->g_info[i], &awb_statis->b_info[i])){
			ISP_LOGD("end to file");
		}
	}

	fclose(fp);

	return ret;
}

cmr_int isp_sim_save_ae_stats(struct isp_awb_statistic_info *awb_statis, cmr_u32 stat_w, cmr_u32 stat_h)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_u32 i = 0;
	char *tmp_ptr = NULL;
	FILE *fp = NULL;
	char file_name[260] = { 0 };

	ISP_LOGI("hwsim:w[%d] h[%d]\n", stat_w, stat_h);

	if (awb_statis == NULL) {
		ISP_LOGE("fail to get ae statis pointer.");
		return ISP_ERROR;
	}
	tmp_ptr = s_r_file_name;
	if (strlen(tmp_ptr)) {
		strncpy(file_name, tmp_ptr, strrchr(tmp_ptr, '.')-tmp_ptr);
		strcat(file_name, ".aem");
	} else {
		ISP_LOGE("fail to get mipi raw file name.");
		return ISP_ERROR;
	}
	fp = fopen(file_name, "wb");
	if (fp == NULL) {
		ISP_LOGE("fail to open file");
		return ISP_ERROR;
	}

	fprintf(fp, "stat_w:%d\n", stat_w);
	fprintf(fp, "stat_h:%d\n", stat_h);
	for (i=0; i < stat_w*stat_h; i++) {
		fprintf(fp, "blk_id:%d R:%d G:%d B:%d\n",
			i, awb_statis->r_info[i], awb_statis->g_info[i], awb_statis->b_info[i]);
	}
	fflush(fp);
	fclose(fp);

	return ret;
}

cmr_int isp_sim_get_ae_stats(struct isp_awb_statistic_info *awb_statis, cmr_u32 *stat_w, cmr_u32 *stat_h)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_u32 i = 0;
	FILE *fp = NULL;
	char *tmp_ptr = NULL;
	char file_name[260] = { 0 };

	ISP_LOGI("hwsim:w[%d] h[%d]\n", *stat_w, *stat_h);

	if (awb_statis == NULL) {
		ISP_LOGE("fail to get ae statis pointer.");
		return ISP_ERROR;
	}
	tmp_ptr = s_r_file_name;
	if (strlen(tmp_ptr)) {
		strncpy(file_name, tmp_ptr, strrchr(tmp_ptr, '.')-tmp_ptr);
		strcat(file_name, ".aem");
	} else {
		ISP_LOGE("fail to get mipi raw file name.");
		return ISP_ERROR;
	}

	fp = fopen(file_name, "rb");
	if (fp == NULL) {
		ISP_LOGE("fail to open file[%s]\n",file_name);
		return ISP_ERROR;
	}

	if(EOF == fscanf(fp, "stat_w:%d\n", stat_w)){
		ISP_LOGD("to the end of file.");
	}
	if(EOF == fscanf(fp, "stat_h:%d\n", stat_h)){
		ISP_LOGD("to the end of file.");
	}

	while (!feof(fp)) {
		if(EOF == fscanf(fp, "blk_id:%d ", &i)){
			ISP_LOGD("to the end of file.");
		}
		if(EOF == fscanf(fp, "R:%d G:%d B:%d\n",
			&awb_statis->r_info[i], &awb_statis->g_info[i], &awb_statis->b_info[i])){
			ISP_LOGD("to the end of file.");
		}
		ISP_LOGV("blk_id:%d R:%d G:%d B:%d",
			i, awb_statis->r_info[i], awb_statis->g_info[i], awb_statis->b_info[i]);
	}

	fclose(fp);

	return ret;
}

cmr_int isp_sim_set_scene_parm(struct isptool_scene_param *scene_param)
{
	cmr_int ret = ISP_SUCCESS;

	if (scene_param != NULL) {
		memcpy(&g_scene_param, scene_param, sizeof(struct isptool_scene_param));
	} else {
		ISP_LOGE("fail to get scene param pointer.");
		ret = ISP_PARAM_NULL;
	}

	return ret;
}

cmr_int isp_sim_get_scene_parm(struct isptool_scene_param *scene_param)
{
	cmr_int ret = ISP_SUCCESS;

	if (scene_param != NULL) {
		memcpy(scene_param, &g_scene_param, sizeof(struct isptool_scene_param));
	} else {
		ISP_LOGE("fail to get scene param pointer.");
		ret = ISP_PARAM_NULL;
	}

	return ret;
}
