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
#define LOG_TAG "isp_brigde"

#include "isp_bridge.h"

struct ispbr_context {
	cmr_u32 user_cnt;
	cmr_handle isp_3afw_handles[SENSOR_NUM_MAX];
	sem_t ae_sm;
	sem_t awb_sm;
	sem_t module_sm;
	sem_t ae_wait_sm;
	sem_t awb_wait_sm;
	struct sensor_raw_ioctrl *ioctrl_ptr[SENSOR_NUM_MAX];
	struct match_data_param match_param;
	struct sensor_dual_otp_info *dual_otp[SENSOR_NUM_MAX];
	void *aem_sync_stat[SENSOR_NUM_MAX];
	cmr_u32 aem_sync_stat_size[SENSOR_NUM_MAX];
	cmr_u32 aem_stat_blk_num[SENSOR_NUM_MAX];
	cmr_u32 aem_stat_size[SENSOR_NUM_MAX];
};

static struct ispbr_context br_cxt;
static pthread_mutex_t g_br_mutex = PTHREAD_MUTEX_INITIALIZER;

cmr_handle isp_br_get_3a_handle(cmr_u32 camera_id)
{
	if (camera_id >= SENSOR_NUM_MAX) {
		ISP_LOGE("fail to camera_id %d", camera_id);
		return NULL;
	}
	return br_cxt.isp_3afw_handles[camera_id];

}
cmr_int isp_br_ioctrl(cmr_u32 camera_id, cmr_int cmd, void *in, void *out)
{
	struct ispbr_context *cxt = &br_cxt;

	if (camera_id >= SENSOR_NUM_MAX) {
		ISP_LOGE("fail to invalid camera_id %u", camera_id);
		return -ISP_PARAM_ERROR;
	}

	ISP_LOGV("E camera_id=%u, cmd=%lu", camera_id, cmd);

	switch (cmd) {
	case SET_MATCH_AWB_DATA:
		sem_wait(&cxt->awb_sm);
		memcpy(&cxt->match_param.awb_info[camera_id], in,
			sizeof(cxt->match_param.awb_info[camera_id]));
		sem_post(&cxt->awb_sm);
		break;
	case GET_MATCH_AWB_DATA:
		sem_wait(&cxt->awb_sm);
		memcpy(out, &cxt->match_param.awb_info[camera_id],
			sizeof(cxt->match_param.awb_info[camera_id]));
		sem_post(&cxt->awb_sm);
		break;
	case SET_MATCH_AE_DATA:
		sem_wait(&cxt->ae_sm);
		memcpy(&cxt->match_param.ae_info[camera_id], in,
			sizeof(cxt->match_param.ae_info[camera_id]));
		sem_post(&cxt->ae_sm);
		break;
	case GET_MATCH_AE_DATA:
		sem_wait(&cxt->ae_sm);
		memcpy(out, &cxt->match_param.ae_info[camera_id],
			sizeof(cxt->match_param.ae_info[camera_id]));
		sem_post(&cxt->ae_sm);
		break;
	case SET_MATCH_BV_DATA:
		sem_wait(&cxt->ae_sm);
		memcpy(&cxt->match_param.bv[camera_id], in,
			sizeof(cxt->match_param.bv[camera_id]));
		sem_post(&cxt->ae_sm);
		break;
	case GET_MATCH_BV_DATA:
		sem_wait(&cxt->ae_sm);
		memcpy(out, &cxt->match_param.bv[camera_id],
			sizeof(cxt->match_param.bv[camera_id]));
		sem_post(&cxt->ae_sm);
		break;
	case SET_STAT_AWB_DATA:
		sem_wait(&cxt->awb_sm);
		memcpy(cxt->match_param.awb_stat_data[camera_id], in,
			cxt->aem_sync_stat_size[camera_id]);
		sem_post(&cxt->awb_sm);
		break;
	case GET_STAT_AWB_DATA:
		sem_wait(&cxt->awb_sm);
		memcpy(out, cxt->match_param.awb_stat_data[camera_id],
			cxt->aem_sync_stat_size[camera_id]);
		sem_post(&cxt->awb_sm);
		break;
	case SET_GAIN_AWB_DATA:
		sem_wait(&cxt->awb_sm);
		memcpy(&cxt->match_param.awb_gain[camera_id], in,
			sizeof(cxt->match_param.awb_gain[camera_id]));
		sem_post(&cxt->awb_sm);
		break;
	case GET_GAIN_AWB_DATA:
		sem_wait(&cxt->awb_sm);
		memcpy(out, &cxt->match_param.awb_gain[camera_id],
			sizeof(cxt->match_param.awb_gain[camera_id]));
		sem_post(&cxt->awb_sm);
		break;
	case SET_MODULE_INFO:
		sem_wait(&cxt->module_sm);
		memcpy(&cxt->match_param.module_info.module_sensor_info.sensor_info[camera_id], in,
			sizeof(cxt->match_param.module_info.module_sensor_info.sensor_info[camera_id]));
		sem_post(&cxt->module_sm);
		break;
	case GET_MODULE_INFO:
		sem_wait(&cxt->module_sm);
		memcpy(out, &cxt->match_param.module_info.module_sensor_info.sensor_info[camera_id],
			sizeof(cxt->match_param.module_info.module_sensor_info.sensor_info[camera_id]));
		sem_post(&cxt->module_sm);
		break;
	case SET_OTP_AE:
		sem_wait(&cxt->module_sm);
		memcpy(&cxt->match_param.module_info.module_otp_info.ae_otp[camera_id], in,
			sizeof(cxt->match_param.module_info.module_otp_info.ae_otp[camera_id]));
		sem_post(&cxt->module_sm);
		break;
	case GET_OTP_AE:
		sem_wait(&cxt->module_sm);
		memcpy(out, &cxt->match_param.module_info.module_otp_info.ae_otp[camera_id],
			sizeof(cxt->match_param.module_info.module_otp_info.ae_otp[camera_id]));
		sem_post(&cxt->module_sm);
		break;
	case SET_OTP_AWB:
		sem_wait(&cxt->module_sm);
		memcpy(&cxt->match_param.module_info.module_otp_info.awb_otp[camera_id], in,
		sizeof(cxt->match_param.module_info.module_otp_info.awb_otp[camera_id]));
		sem_post(&cxt->module_sm);
		break;
	case GET_OTP_AWB:
		sem_wait(&cxt->module_sm);
		memcpy(out, &cxt->match_param.module_info.module_otp_info.awb_otp[camera_id],
		sizeof(cxt->match_param.module_info.module_otp_info.awb_otp[camera_id]));
		sem_post(&cxt->module_sm);
		break;
	case SET_ALL_MODULE_AND_OTP:
		ISP_LOGW("not implemented");
		break;
	case GET_ALL_MODULE_AND_OTP:
		sem_wait(&cxt->ae_sm);
		memcpy(out, &cxt->match_param.module_info, sizeof(cxt->match_param.module_info));
		sem_post(&cxt->ae_sm);
		break;
	case AE_WAIT_SEM:
		sem_wait(&cxt->ae_wait_sm);
		break;
	case AE_POST_SEM:
		sem_post(&cxt->ae_wait_sm);
		break;
	case AWB_WAIT_SEM:
		sem_wait(&cxt->awb_wait_sm);
		break;
	case AWB_POST_SEM:
		sem_post(&cxt->awb_wait_sm);
		break;
	case SET_AEM_SYNC_STAT:
		sem_wait(&cxt->module_sm);
		memcpy(cxt->aem_sync_stat[camera_id], in,
			3 * cxt->aem_stat_blk_num[camera_id] * sizeof(cmr_u32));
		sem_post(&cxt->module_sm);
		break;
	case GET_AEM_SYNC_STAT:
		sem_wait(&cxt->module_sm);
		memcpy(out, cxt->aem_sync_stat[camera_id],
			3 * cxt->aem_stat_blk_num[camera_id] * sizeof(cmr_u32));
		sem_post(&cxt->module_sm);
		break;
	case SET_AEM_STAT_BLK_NUM:
		sem_wait(&cxt->module_sm);
		cxt->aem_stat_blk_num[camera_id] = *(cmr_u32 *)in;
		ISP_LOGV("camera_id = %d, aem_stat_blk_num %d",
			camera_id, cxt->aem_stat_blk_num[camera_id]);
		sem_post(&cxt->module_sm);
		break;
	case SET_AEM_STAT_SIZE:
		sem_wait(&cxt->module_sm);
		cxt->aem_stat_size[camera_id] = *(cmr_u32 *)in;
		ISP_LOGV("camera_id = %d, aem_stat_size %d",
			camera_id, cxt->aem_stat_size[camera_id]);
		sem_post(&cxt->module_sm);
		break;
	default:
		break;
	}
	ISP_LOGV("X");

	return 0;
}

cmr_int isp_br_init(cmr_u32 camera_id, cmr_handle isp_3a_handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct ispbr_context *cxt = &br_cxt;
	void *aem_sync_stat = NULL;
	cmr_u32 aem_sync_stat_size = 0;
	void *awb_stat_data = NULL;
	cmr_u32 awb_stat_data_size = 0;
	cmr_u32 i = 0;

	ISP_LOGI("camera_id %d", camera_id);
	cxt->isp_3afw_handles[camera_id] = isp_3a_handle;
	pthread_mutex_lock(&g_br_mutex);
	cxt->user_cnt++;
	pthread_mutex_unlock(&g_br_mutex);
	ISP_LOGI("cnt = %d", cxt->user_cnt);
	if (1 == cxt->user_cnt) {
		sem_init(&cxt->ae_sm, 0, 1);
		sem_init(&cxt->awb_sm, 0, 1);
		sem_init(&cxt->module_sm, 0, 1);
		sem_init(&cxt->ae_wait_sm, 0, 1);
		sem_init(&cxt->awb_wait_sm, 0, 1);
	}

	aem_sync_stat_size = 3 * ISP_AEM_STAT_BLK_NUM * sizeof(cmr_u32);
	cxt->aem_sync_stat_size[camera_id] = aem_sync_stat_size;
	aem_sync_stat = (void *)malloc(aem_sync_stat_size);
	if (NULL == aem_sync_stat) {
		ret = ISP_ALLOC_ERROR;
		ISP_LOGE("fail to alloc aem_sync_stat");
		goto exit;
	}
	cxt->aem_sync_stat[camera_id] = aem_sync_stat;
	ISP_LOGV("aem_sync_stat %p", cxt->aem_sync_stat[camera_id]);

	awb_stat_data_size = 3 * ISP_AEM_STAT_BLK_NUM * sizeof(cmr_u32);
	cxt->match_param.awb_stat_data_size[camera_id] = awb_stat_data_size;
	awb_stat_data = (void *)malloc(awb_stat_data_size);
	if (NULL == awb_stat_data) {
		ret = ISP_ALLOC_ERROR;
		ISP_LOGE("fail to alloc awb_stat_data");
		goto exit;
	}
	cxt->match_param.awb_stat_data[camera_id] = awb_stat_data;
	ISP_LOGV("awb_stat_data %p", cxt->match_param.awb_stat_data[camera_id]);

	return ret;
exit:
	for (i = 0; i < SENSOR_NUM_MAX; i++) {
		if (cxt->aem_sync_stat[i]) {
			free(cxt->aem_sync_stat[i]);
			cxt->aem_sync_stat[i] = NULL;
		}
		if (cxt->match_param.awb_stat_data[i]) {
			free(cxt->match_param.awb_stat_data[i]);
			cxt->match_param.awb_stat_data[i] = NULL;
		}
	}
	return ret;
}

cmr_int isp_br_deinit(cmr_u32 camera_id)
{
	cmr_int ret = ISP_SUCCESS;
	struct ispbr_context *cxt = &br_cxt;
	cmr_u8 i = 0;

	cxt->isp_3afw_handles[camera_id] = NULL;
	pthread_mutex_lock(&g_br_mutex);
	cxt->user_cnt--;
	pthread_mutex_unlock(&g_br_mutex);
	ISP_LOGI("cnt = %d", cxt->user_cnt);
	if (0 == cxt->user_cnt) {
		sem_destroy(&cxt->ae_sm);
		sem_destroy(&cxt->awb_sm);
		sem_destroy(&cxt->module_sm);
		sem_destroy(&cxt->ae_wait_sm);
		sem_destroy(&cxt->awb_wait_sm);
		for (i = 0; i < SENSOR_NUM_MAX; i++)
			cxt->isp_3afw_handles[i] = NULL;
	}

	if (NULL != cxt->aem_sync_stat[camera_id]) {
		free(cxt->aem_sync_stat[camera_id]);
		cxt->aem_sync_stat[camera_id] = NULL;
	}
	if (NULL != cxt->match_param.awb_stat_data[camera_id]) {
		free(cxt->match_param.awb_stat_data[camera_id]);
		cxt->match_param.awb_stat_data[camera_id] = NULL;
	}

	return ret;
}

cmr_int isp_br_save_dual_otp(cmr_u32 camera_id, struct sensor_dual_otp_info *dual_otp)
{
	cmr_int ret = ISP_SUCCESS;
	struct ispbr_context *cxt = &br_cxt;

	if (camera_id >= SENSOR_NUM_MAX) {
		ISP_LOGE("fail to save camera_id %d dual otp", camera_id);
		ret = ISP_PARAM_ERROR;
		goto exit;
	}
	cxt->dual_otp[camera_id] = dual_otp;
exit:
	return ret;
}

cmr_int isp_br_get_dual_otp(cmr_u32 camera_id, struct sensor_dual_otp_info **dual_otp)
{
	cmr_int ret = ISP_SUCCESS;
	struct ispbr_context *cxt = &br_cxt;

	if (camera_id >= SENSOR_NUM_MAX) {
		ISP_LOGE("fail to get camera_id %d dual otp", camera_id);
		ret = ISP_PARAM_ERROR;
		goto exit;
	}
	*dual_otp = cxt->dual_otp[camera_id];
exit:
	return ret;
}
