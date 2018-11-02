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
	cmr_u32 camera_init[SENSOR_NUM_MAX];
	cmr_handle isp_3afw_handles[SENSOR_NUM_MAX];
	struct match_data_param match_param;
	void *aem_sync_stat[SENSOR_NUM_MAX];
	cmr_u32 aem_sync_stat_size;
	void *awb_stat_data[SENSOR_NUM_MAX];
	cmr_u32 awb_stat_data_size;
	cmr_u32 aem_stat_blk_num[SENSOR_NUM_MAX];
	cmr_u32 slave_camera_id;
	cmr_u32 slave_sensor_mode;
	sem_t module_sm;
	sem_t ae_sm;
	sem_t awb_sm;
	sem_t af_sm;
	sem_t ae_wait_sm;
	sem_t awb_wait_sm;
	sem_t af_wait_sm;
};

static struct ispbr_context br_cxt;
static pthread_mutex_t g_br_mutex = PTHREAD_MUTEX_INITIALIZER;

cmr_handle isp_br_get_3a_handle(cmr_u32 camera_id)
{
	ISP_LOGV("slave_camera_id %d", camera_id);
	return br_cxt.isp_3afw_handles[camera_id];
}

cmr_int isp_br_ioctrl(cmr_u32 sensor_role, cmr_int cmd, void *in, void *out)
{
	struct ispbr_context *cxt = &br_cxt;

	ISP_LOGV("cmd=%lu", cmd);
	switch (cmd) {
	// AE
	case SET_MATCH_AE_DATA:
		sem_wait(&cxt->ae_sm);
		memcpy(&cxt->match_param.ae_info[sensor_role], in,
			sizeof(cxt->match_param.ae_info[sensor_role]));
		sem_post(&cxt->ae_sm);
		break;

	case GET_MATCH_AE_DATA:
		sem_wait(&cxt->ae_sm);
		memcpy(out, &cxt->match_param.ae_info[sensor_role],
			sizeof(cxt->match_param.ae_info[sensor_role]));
		sem_post(&cxt->ae_sm);
		break;

	case SET_AEM_SYNC_STAT:
		sem_wait(&cxt->module_sm);
		if (NULL != cxt->aem_sync_stat[sensor_role]) {
			memcpy(cxt->aem_sync_stat[sensor_role], in,
				3 * cxt->aem_stat_blk_num[sensor_role] * sizeof(cmr_u32));
		}
		sem_post(&cxt->module_sm);
		break;

	case GET_AEM_SYNC_STAT:
		sem_wait(&cxt->module_sm);
		if (NULL != cxt->aem_sync_stat[sensor_role]) {
			memcpy(out, cxt->aem_sync_stat[sensor_role],
				3 * cxt->aem_stat_blk_num[sensor_role] * sizeof(cmr_u32));
		}
		sem_post(&cxt->module_sm);
		break;

	case SET_AEM_STAT_BLK_NUM:
		sem_wait(&cxt->module_sm);
		cxt->aem_stat_blk_num[sensor_role] = *(cmr_u32 *)in;
		ISP_LOGV("sensor_role %d, aem_stat_blk_num %d",
			sensor_role, cxt->aem_stat_blk_num[sensor_role]);
		sem_post(&cxt->module_sm);
		break;

	case SET_MATCH_BV_DATA:
		sem_wait(&cxt->ae_sm);
		memcpy(&cxt->match_param.bv[sensor_role], in,
			sizeof(cxt->match_param.bv[sensor_role]));
		sem_post(&cxt->ae_sm);
		break;

	case GET_MATCH_BV_DATA:
		sem_wait(&cxt->ae_sm);
		memcpy(out, &cxt->match_param.bv[sensor_role],
			sizeof(cxt->match_param.bv[sensor_role]));
		sem_post(&cxt->ae_sm);
		break;

	// AWB
	case SET_MATCH_AWB_DATA:
		sem_wait(&cxt->awb_sm);
		memcpy(&cxt->match_param.awb_info[sensor_role], in,
			sizeof(cxt->match_param.awb_info[sensor_role]));
		sem_post(&cxt->awb_sm);
		break;

	case GET_MATCH_AWB_DATA:
		sem_wait(&cxt->awb_sm);
		memcpy(out, &cxt->match_param.awb_info[sensor_role],
			sizeof(cxt->match_param.awb_info[sensor_role]));
		sem_post(&cxt->awb_sm);
		break;

	case SET_STAT_AWB_DATA:
		sem_wait(&cxt->awb_sm);
		if (NULL != cxt->awb_stat_data[sensor_role]) {
			memcpy(cxt->awb_stat_data[sensor_role], in,
				cxt->awb_stat_data_size);
		}
		sem_post(&cxt->awb_sm);
		break;

	case GET_STAT_AWB_DATA:
		sem_wait(&cxt->awb_sm);
		if (NULL != cxt->awb_stat_data[sensor_role]) {
			memcpy(out, cxt->awb_stat_data[sensor_role],
				cxt->awb_stat_data_size);
		}
		sem_post(&cxt->awb_sm);
		break;

	case SET_GAIN_AWB_DATA:
		sem_wait(&cxt->awb_sm);
		memcpy(&cxt->match_param.awb_gain[sensor_role], in,
			sizeof(cxt->match_param.awb_gain[sensor_role]));
		sem_post(&cxt->awb_sm);
		break;

	case GET_GAIN_AWB_DATA:
		sem_wait(&cxt->awb_sm);
		memcpy(out, &cxt->match_param.awb_gain[sensor_role],
			sizeof(cxt->match_param.awb_gain[sensor_role]));
		sem_post(&cxt->awb_sm);
		break;

	case SET_FOV_DATA:
		sem_wait(&cxt->awb_sm);
		memcpy(&cxt->match_param.fov_info[sensor_role], in,
			sizeof(cxt->match_param.fov_info[sensor_role]));
		sem_post(&cxt->awb_sm);
		break;

	case GET_FOV_DATA:
		sem_wait(&cxt->awb_sm);
		memcpy(out, &cxt->match_param.fov_info[sensor_role],
			sizeof(cxt->match_param.fov_info[sensor_role]));
		sem_post(&cxt->awb_sm);
		break;

	// AF
	case SET_AF_STATUS_INFO:
		sem_wait(&cxt->af_sm);
		memcpy(&cxt->match_param.af_info[sensor_role], in,
			sizeof(cxt->match_param.af_info[sensor_role]));
		sem_post(&cxt->af_sm);
		break;

	case GET_AF_STATUS_INFO:
		sem_wait(&cxt->af_sm);
		memcpy(out, &cxt->match_param.af_info[sensor_role],
			sizeof(cxt->match_param.af_info[sensor_role]));
		sem_post(&cxt->af_sm);
		break;

	// OTP
	case SET_OTP_AE:
		sem_wait(&cxt->module_sm);
		memcpy(&cxt->match_param.module_info.module_otp_info.ae_otp[sensor_role], in,
			sizeof(cxt->match_param.module_info.module_otp_info.ae_otp[sensor_role]));
		sem_post(&cxt->module_sm);
		break;

	case GET_OTP_AE:
		sem_wait(&cxt->module_sm);
		memcpy(out, &cxt->match_param.module_info.module_otp_info.ae_otp[sensor_role],
			sizeof(cxt->match_param.module_info.module_otp_info.ae_otp[sensor_role]));
		sem_post(&cxt->module_sm);
		break;

	case SET_OTP_AWB:
		sem_wait(&cxt->module_sm);
		memcpy(&cxt->match_param.module_info.module_otp_info.awb_otp[sensor_role], in,
			sizeof(cxt->match_param.module_info.module_otp_info.awb_otp[sensor_role]));
		sem_post(&cxt->module_sm);
		break;

	case GET_OTP_AWB:
		sem_wait(&cxt->module_sm);
		memcpy(out, &cxt->match_param.module_info.module_otp_info.awb_otp[sensor_role],
			sizeof(cxt->match_param.module_info.module_otp_info.awb_otp[sensor_role]));
		sem_post(&cxt->module_sm);
		break;

	case SET_MODULE_INFO:
		sem_wait(&cxt->module_sm);
		memcpy(&cxt->match_param.module_info.module_sensor_info.sensor_info[sensor_role], in,
			sizeof(cxt->match_param.module_info.module_sensor_info.sensor_info[sensor_role]));
		sem_post(&cxt->module_sm);
		break;

	case GET_MODULE_INFO:
		sem_wait(&cxt->module_sm);
		memcpy(out, &cxt->match_param.module_info.module_sensor_info.sensor_info[sensor_role],
			sizeof(cxt->match_param.module_info.module_sensor_info.sensor_info[sensor_role]));
		sem_post(&cxt->module_sm);
		break;

	case GET_SLAVE_CAMERA_ID:
		sem_wait(&cxt->module_sm);
		memcpy(out, &cxt->slave_camera_id,
			sizeof(cxt->slave_camera_id));
		sem_post(&cxt->module_sm);
		break;

	case SET_SLAVE_SENSOR_MODE:
		sem_wait(&cxt->module_sm);
		memcpy(&cxt->slave_sensor_mode, in,
			sizeof(cxt->slave_sensor_mode));
		sem_post(&cxt->module_sm);
		break;

	case GET_SLAVE_SENSOR_MODE:
		sem_wait(&cxt->module_sm);
		memcpy(out, &cxt->slave_sensor_mode,
			sizeof(cxt->slave_sensor_mode));
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
	default:
		break;
	}
	ISP_LOGV("X");

	return 0;
}

cmr_int isp_br_init(cmr_u32 camera_id, cmr_handle isp_3a_handle, cmr_u32 is_master)
{
	cmr_int ret = ISP_SUCCESS;
	struct ispbr_context *cxt = &br_cxt;
	void *aem_sync_stat = NULL;
	cmr_u32 aem_sync_stat_size = 0;
	void *awb_stat_data = NULL;
	cmr_u32 awb_stat_data_size = 0;

	ISP_LOGI("camera_id %d, is_master %d", camera_id, is_master);
	cxt->isp_3afw_handles[camera_id] = isp_3a_handle;

	if (!is_master) {
		cxt->slave_camera_id = camera_id;
		ISP_LOGI("slave_camera_id = %d", cxt->slave_camera_id);
	}

	pthread_mutex_lock(&g_br_mutex);
	cxt->user_cnt++;
	pthread_mutex_unlock(&g_br_mutex);
	ISP_LOGI("cnt = %d", cxt->user_cnt);

	if (cxt->aem_sync_stat[camera_id] == NULL) {
		aem_sync_stat_size = 3 * ISP_AEM_STAT_BLK_NUM * sizeof(cmr_u32);
		aem_sync_stat = (void *)malloc(aem_sync_stat_size);
		if (NULL == aem_sync_stat) {
			ret = ISP_ALLOC_ERROR;
			ISP_LOGE("fail to alloc aem_sync_stat");
			goto exit;
		}
		cxt->aem_sync_stat[camera_id] = aem_sync_stat;
		cxt->aem_sync_stat_size = aem_sync_stat_size;
		ISP_LOGV("camera %d aem_sync_stat %p", camera_id, aem_sync_stat);
	}

	if (cxt->awb_stat_data[camera_id] == NULL) {
		awb_stat_data_size = 3 * ISP_AEM_STAT_BLK_NUM * sizeof(cmr_u32);
		awb_stat_data = (void *)malloc(awb_stat_data_size);
		if (NULL == awb_stat_data) {
			ret = ISP_ALLOC_ERROR;
			ISP_LOGE("fail to alloc awb_stat_data");
			goto exit;
		}
		cxt->awb_stat_data[camera_id] = awb_stat_data;
		cxt->awb_stat_data_size = awb_stat_data_size;
		ISP_LOGV("camera %d awb_stat_data %p", camera_id, awb_stat_data);
	}

	if (cxt->user_cnt == 1) {
		sem_init(&cxt->ae_sm, 0, 1);
		sem_init(&cxt->awb_sm, 0, 1);
		sem_init(&cxt->af_sm, 0, 1);
		sem_init(&cxt->module_sm, 0, 1);
		sem_init(&cxt->ae_wait_sm, 0, 1);
		sem_init(&cxt->awb_wait_sm, 0, 1);
		sem_init(&cxt->af_wait_sm, 0, 1);
	}
	cxt->camera_init[camera_id] = 1;
	ISP_LOGD("camera %d done\n", camera_id);
	return ret;

exit:
	if (cxt->aem_sync_stat[camera_id]) {
		free(cxt->aem_sync_stat[camera_id]);
		cxt->aem_sync_stat[camera_id] = NULL;
	}
	if (cxt->awb_stat_data[camera_id]) {
		free(cxt->awb_stat_data[camera_id]);
		cxt->awb_stat_data[camera_id] = NULL;
	}

	cxt->isp_3afw_handles[camera_id] = NULL;
	pthread_mutex_lock(&g_br_mutex);
	cxt->user_cnt--;
	pthread_mutex_unlock(&g_br_mutex);
	return ret;
}

cmr_int isp_br_deinit(cmr_u32 camera_id)
{
	cmr_int ret = ISP_SUCCESS;
	struct ispbr_context *cxt = &br_cxt;

	if (cxt->camera_init[camera_id] == 0) {
		ISP_LOGI("camera %d is not inited\n", camera_id);
		return ret;
	}

	if (cxt->aem_sync_stat[camera_id]) {
		free(cxt->aem_sync_stat[camera_id]);
		cxt->aem_sync_stat[camera_id] = NULL;
	}
	if (cxt->awb_stat_data[camera_id]) {
		free(cxt->awb_stat_data[camera_id]);
		cxt->awb_stat_data[camera_id] = NULL;
	}
	cxt->isp_3afw_handles[camera_id] = NULL;

	pthread_mutex_lock(&g_br_mutex);
	cxt->user_cnt--;
	pthread_mutex_unlock(&g_br_mutex);
	ISP_LOGI("camera_id = %d, cnt = %d", camera_id, cxt->user_cnt);

	if (cxt->user_cnt == 0) {
		sem_destroy(&cxt->ae_sm);
		sem_destroy(&cxt->awb_sm);
		sem_destroy(&cxt->af_sm);
		sem_destroy(&cxt->module_sm);
		sem_destroy(&cxt->ae_wait_sm);
		sem_destroy(&cxt->awb_wait_sm);
		sem_destroy(&cxt->af_wait_sm);
	}
	cxt->camera_init[camera_id] = 0;
	return ret;
}
