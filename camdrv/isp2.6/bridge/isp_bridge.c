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
#define LOG_TAG "ispbr"

#include "isp_bridge.h"

// TODO keep same with cmr_common.h
#define CAMERA_ID_MAX 16

#define ISP_AEM_STAT_BLK_NUM_MAX (128 * 128)
#define ISP_AEM_BLK_NUM_TO_SIZE(num) (3 * (num) * sizeof(cmr_u32))
#define ISP_AEM_STAT_SIZE_MAX ISP_AEM_BLK_NUM_TO_SIZE(ISP_AEM_STAT_BLK_NUM_MAX)

struct ae_data {
	struct isp_size block_size;
	struct isp_rect block_rect;
};

struct sensor_data {
	struct isp_size sensor_size;
};

struct match_data {
	struct ae_data ae;
	struct sensor_data sensor;
};

struct match_data_param {
	struct module_info module_info;

	struct aem_info aem_stat_info[CAM_SENSOR_MAX];
	struct ae_match_data ae_info[CAM_SENSOR_MAX];
	struct ae_sync_slave_data ae_sync_slave_info[CAM_SENSOR_MAX];
	struct ae_match_stats_data ae_stats_data[CAM_SENSOR_MAX];
	cmr_u32 aem_stat_blk_num[CAM_SENSOR_MAX];
	cmr_u16 bv[CAM_SENSOR_MAX];
	cmr_u8 flash_state[CAM_SENSOR_MAX];

	struct awb_match_data awb_info[CAM_SENSOR_MAX];
	struct awb_match_stats_data awb_stats_data[CAM_SENSOR_MAX];
	struct awb_gain_data awb_gain[CAM_SENSOR_MAX];

	struct fov_data fov_info[CAM_SENSOR_MAX];
	// TODO joseph af_role is totally different
	struct af_status_info af_info[CAM_SENSOR_MAX];
	struct af_manual_info af_manual[CAM_SENSOR_MAX];

	cmr_u32 sensor_mode[CAM_SENSOR_MAX];

	struct match_data data[CAM_SENSOR_MAX];
};

struct ispbr_context {
	cmr_u32 start_user_cnt;
	cmr_handle ispalg_fw_handles[CAM_SENSOR_MAX];

	sem_t module_sm;
	sem_t ae_sm;
	sem_t awb_sm;
	sem_t af_sm;
	struct match_data_param match_param;

	cmr_u32 ae_ref_camera_id;
	struct ae_rect ae_region[CAM_SENSOR_MAX];

	sem_t br_role_sm;
	cmr_u32 id2role[CAMERA_ID_MAX];
	cmr_u32 role2id[CAM_SENSOR_MAX];
};

static struct ispbr_context br_cxt;

static cmr_u32 g_br_user_cnt = 0;
static pthread_mutex_t g_br_mutex = PTHREAD_MUTEX_INITIALIZER;

static inline const char *get_role_name(cmr_u32 sensor_role) {
	/* keep align with roles in isp_com.h */
	static const char *role_names[CAM_SENSOR_MAX] = {
		[CAM_SENSOR_MASTER] = "m",
		[CAM_SENSOR_SLAVE0] = "s0",
		[CAM_SENSOR_SLAVE1] = "s1",
	};

	if (sensor_role >= CAM_SENSOR_MAX)
		return "(null)";

	return role_names[sensor_role];
}

static inline cmr_u32 user_cnt_inc() {
	cmr_u32 cnt = 0;

	pthread_mutex_lock(&g_br_mutex);
	cnt = ++g_br_user_cnt;
	pthread_mutex_unlock(&g_br_mutex);
	ISP_LOGV("cnt = %d", cnt);

	return cnt;
}

static inline cmr_u32 user_cnt_dec() {
	cmr_u32 cnt = 0;

	pthread_mutex_lock(&g_br_mutex);
	cnt = --g_br_user_cnt;
	pthread_mutex_unlock(&g_br_mutex);
	ISP_LOGV("cnt = %d", cnt);

	return cnt;
}

static inline cmr_u32 user_cnt_get() {
	return g_br_user_cnt;
}

static void role_clear(struct ispbr_context *cxt) {
	int i = 0;

	if (!cxt) {
		ISP_LOGE("invalid cxt");
		return;
	}

	sem_wait(&cxt->br_role_sm);
	for (i = 0; i < CAM_SENSOR_MAX; i++)
		cxt->role2id[i] = CAMERA_ID_MAX;
	for (i = 0; i < CAMERA_ID_MAX; i++)
		cxt->id2role[i] = CAM_SENSOR_MAX;
	sem_post(&cxt->br_role_sm);
}

static cmr_u32 role_add(struct ispbr_context *cxt,
		cmr_u32 camera_id, cmr_u32 is_master) {
	cmr_u32 sensor_role = CAM_SENSOR_MAX;
	int i = 0;

	if (!cxt) {
		ISP_LOGE("invalid cxt");
		return sensor_role;
	}

	if (camera_id >= CAMERA_ID_MAX) {
		ISP_LOGE("invalid camera_id %u", camera_id);
		return sensor_role;
	}

	sem_wait(&cxt->br_role_sm);
	if (is_master) {
		if (cxt->role2id[CAM_SENSOR_MASTER] != CAMERA_ID_MAX) {
			ISP_LOGE("fail to set %u as MASTER, already has %u",
					camera_id, cxt->role2id[CAM_SENSOR_MASTER]);
		} else {
			cxt->role2id[CAM_SENSOR_MASTER] = camera_id;
			cxt->id2role[camera_id] = CAM_SENSOR_MASTER;
			sensor_role = CAM_SENSOR_MASTER;
			ISP_LOGI("set %u as %s", camera_id, get_role_name(sensor_role));
		}
	} else {
		for (i = CAM_SENSOR_SLAVE0; i < CAM_SENSOR_MAX; i++) {
			if (cxt->role2id[i] == CAMERA_ID_MAX)
				break;
		}
		if (i == CAM_SENSOR_MAX) {
			ISP_LOGE("fail to set %u as SLAVE, reach max count", camera_id);
		} else {
			cxt->role2id[i] = camera_id;
			cxt->id2role[camera_id] = i;
			sensor_role = i;
			ISP_LOGI("set %u as %s", camera_id, get_role_name(sensor_role));
		}
	}
	sem_post(&cxt->br_role_sm);

	return sensor_role;
}

static void role_delete(struct ispbr_context *cxt, cmr_u32 camera_id) {
	cmr_u32 role = CAM_SENSOR_MAX;

	if (!cxt) {
		ISP_LOGE("invalid cxt");
		return;
	}

	if (camera_id >= CAMERA_ID_MAX) {
		ISP_LOGE("invalid camera_id %u", camera_id);
		return;
	}

	sem_wait(&cxt->br_role_sm);
	if (cxt->id2role[camera_id] == CAM_SENSOR_MAX) {
		ISP_LOGW("%u already deleted", camera_id);
	} else {
		role = cxt->id2role[camera_id];
		cxt->role2id[role] = CAMERA_ID_MAX;
		cxt->id2role[camera_id] = CAM_SENSOR_MAX;
		ISP_LOGI("delete %s %u", get_role_name(role), camera_id);
	}
	sem_post(&cxt->br_role_sm);
}

static cmr_u32 get_role_by_id(struct ispbr_context *cxt, cmr_u32 camera_id) {
	cmr_u32 role = CAM_SENSOR_MAX;

	sem_wait(&cxt->br_role_sm);
	role = cxt->id2role[camera_id];
	sem_post(&cxt->br_role_sm);

	return role;
}

static cmr_u32 get_id_by_role(struct ispbr_context *cxt, cmr_u32 sensor_role) {
	cmr_u32 id = CAMERA_ID_MAX;

	sem_wait(&cxt->br_role_sm);
	id = cxt->role2id[sensor_role];
	sem_post(&cxt->br_role_sm);

	return id;
}

static inline void semaphore_init(struct ispbr_context *cxt) {
	sem_init(&cxt->ae_sm, 0, 1);
	sem_init(&cxt->awb_sm, 0, 1);
	sem_init(&cxt->af_sm, 0, 1);
	sem_init(&cxt->module_sm, 0, 1);
	sem_init(&cxt->br_role_sm, 0, 1);
}

static inline void semaphore_deinit(struct ispbr_context *cxt) {
	sem_destroy(&cxt->ae_sm);
	sem_destroy(&cxt->awb_sm);
	sem_destroy(&cxt->af_sm);
	sem_destroy(&cxt->module_sm);
	sem_destroy(&cxt->br_role_sm);
}

static cmr_int stats_data_alloc(struct ispbr_context *cxt, cmr_u32 sensor_role) {
	cmr_int ret = ISP_SUCCESS;

	memset(&cxt->match_param.ae_stats_data[sensor_role], 0, sizeof(struct ae_match_stats_data));
	cxt->match_param.ae_stats_data[sensor_role].stats_data = malloc(ISP_AEM_STAT_SIZE_MAX);
	if (!cxt->match_param.ae_stats_data[sensor_role].stats_data) {
		ISP_LOGE("fail to alloc AE stats data");
		ret = ISP_ALLOC_ERROR;
		goto ae_alloc_fail;
	}

	memset(&cxt->match_param.awb_stats_data[sensor_role], 0, sizeof(struct awb_match_stats_data));
	cxt->match_param.awb_stats_data[sensor_role].stats_data = malloc(ISP_AEM_STAT_SIZE_MAX);
	if (!cxt->match_param.awb_stats_data[sensor_role].stats_data) {
		ISP_LOGE("fail to alloc AWB stats data");
		ret = ISP_ALLOC_ERROR;
		goto awb_alloc_fail;
	}

	return ret;

awb_alloc_fail:
	free(cxt->match_param.ae_stats_data[sensor_role].stats_data);
	cxt->match_param.ae_stats_data[sensor_role].stats_data = NULL;

ae_alloc_fail:

	return ret;
}

static void stats_data_free(struct ispbr_context *cxt, cmr_u32 sensor_role) {
	free(cxt->match_param.awb_stats_data[sensor_role].stats_data);
	cxt->match_param.awb_stats_data[sensor_role].stats_data = NULL;
	free(cxt->match_param.ae_stats_data[sensor_role].stats_data);
	cxt->match_param.ae_stats_data[sensor_role].stats_data = NULL;
}

cmr_int isp_br_ioctrl(cmr_u32 sensor_role, cmr_int cmd, void *in, void *out)
{
	struct ispbr_context *cxt = &br_cxt;
	struct match_data *data = NULL;

	if (cmd != GET_SENSOR_ROLE && sensor_role >= CAM_SENSOR_MAX) {
		ISP_LOGE("invalid sensor role %u, cmd %ld", sensor_role, cmd);
		return ISP_ERROR;
	}

	data = &cxt->match_param.data[sensor_role];

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

	case SET_FLASH_STATE:
		sem_wait(&cxt->ae_sm);
		memcpy(&cxt->match_param.flash_state[sensor_role], in,
			sizeof(cxt->match_param.flash_state[sensor_role]));
		sem_post(&cxt->ae_sm);
		break;
	
	case GET_FLASH_STATE:
		sem_wait(&cxt->ae_sm);
		memcpy(out, &cxt->match_param.flash_state[sensor_role],
			sizeof(cxt->match_param.flash_state[sensor_role]));
		sem_post(&cxt->ae_sm);
		break;

	case SET_SLAVE_AEM_INFO:
		{
			sem_wait(&cxt->module_sm);
			if (in)
				memcpy(&cxt->match_param.aem_stat_info[sensor_role],
						in, sizeof(struct aem_info));
			sem_post(&cxt->module_sm);
		}
		break;

	case GET_SLAVE_AEM_INFO:
		{
			sem_wait(&cxt->module_sm);
			if (out)
				memcpy(out,
						&cxt->match_param.aem_stat_info[sensor_role],
						sizeof(struct aem_info));
			sem_post(&cxt->module_sm);
		}
		break;	

	case GET_STAT_AWB_DATA_AE:
		{
			sem_wait(&cxt->module_sm);
			// TODO joseph out is type of 'cmr_u32 **', will fail on 64-bit system
			//*(cmr_u32 *)out = (cmr_u32)cxt->awb_stat_data[sensor_role];//tmp code !! tianhui.wang
			cmr_u32 **awb_data = (cmr_u32 **)out;
			// TODO dangerous action!
			*awb_data = cxt->match_param.awb_stats_data[sensor_role].stats_data;
			sem_post(&cxt->module_sm);
		}
		break;

	case SET_AEM_SYNC_STAT:
		{
			struct ae_match_stats_data *stats_data = (struct ae_match_stats_data *)in;

			sem_wait(&cxt->module_sm);
			if (cxt->match_param.ae_stats_data[sensor_role].stats_data
					&& stats_data && stats_data->stats_data) {
				cxt->match_param.ae_stats_data[sensor_role].monoboottime
					= stats_data->monoboottime;
				cxt->match_param.ae_stats_data[sensor_role].is_last_frm
					= stats_data->is_last_frm;
				cxt->match_param.ae_stats_data[sensor_role].len
					= ISP_AEM_BLK_NUM_TO_SIZE(cxt->match_param.aem_stat_blk_num[sensor_role]);
				memcpy(cxt->match_param.ae_stats_data[sensor_role].stats_data,
						stats_data->stats_data,
						cxt->match_param.ae_stats_data[sensor_role].len);
			}
			sem_post(&cxt->module_sm);
 		}
		break;

	case GET_AEM_SYNC_STAT:
		{
			struct ae_match_stats_data *stats_data = (struct ae_match_stats_data *)out;

			sem_wait(&cxt->module_sm);
			if (cxt->match_param.ae_stats_data[sensor_role].stats_data
					&& stats_data && stats_data->stats_data) {
				stats_data->monoboottime
					= cxt->match_param.ae_stats_data[sensor_role].monoboottime;
				stats_data->is_last_frm
					= cxt->match_param.ae_stats_data[sensor_role].is_last_frm;
				// TODO joseph stats_data->len is not used in SET_AEM_SYNC_STAT
				stats_data->len
					= ISP_AEM_BLK_NUM_TO_SIZE(cxt->match_param.aem_stat_blk_num[sensor_role]);
				memcpy(stats_data->stats_data,
						cxt->match_param.ae_stats_data[sensor_role].stats_data,
						cxt->match_param.ae_stats_data[sensor_role].len);
			}
			sem_post(&cxt->module_sm);
 		}
		break;

	case SET_AEM_STAT_BLK_NUM:
		sem_wait(&cxt->module_sm);
		cxt->match_param.aem_stat_blk_num[sensor_role] = *(cmr_u32 *)in;
		ISP_LOGV("sensor_role %d, aem_stat_blk_num %d",
			sensor_role, cxt->match_param.aem_stat_blk_num[sensor_role]);
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

	case SET_AE_BLOCK_SIZE:
		{
			struct isp_size *size = (struct isp_size *)in;

			sem_wait(&cxt->ae_sm);
			data->ae.block_size = *size;
			sem_post(&cxt->ae_sm);
		}
		break;

	case SET_AE_WINDOW_RECT:
		{
			struct isp_rect *rect = (struct isp_rect *)in;

			sem_wait(&cxt->ae_sm);
			data->ae.block_rect = *rect;
			sem_post(&cxt->ae_sm);
		}
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
		if (cxt->match_param.awb_stats_data[sensor_role].stats_data && in)
			memcpy(cxt->match_param.awb_stats_data[sensor_role].stats_data,
					in, ISP_AEM_STAT_SIZE_MAX);
		sem_post(&cxt->awb_sm);
		break;

	case GET_STAT_AWB_DATA:
		sem_wait(&cxt->awb_sm);
		if (cxt->match_param.awb_stats_data[sensor_role].stats_data && out)
			memcpy(out,
					cxt->match_param.awb_stats_data[sensor_role].stats_data,
					ISP_AEM_STAT_SIZE_MAX);
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

	case GET_CAMERA_ID:
		{
			cmr_u32 *id = (cmr_u32 *)out;

			sem_wait(&cxt->module_sm);
			*id = get_id_by_role(cxt, sensor_role);
			sem_post(&cxt->module_sm);
		}
		break;

	case GET_SENSOR_ROLE:
		{
			cmr_u32 *id = (cmr_u32 *)in;
			cmr_u32 *role = (cmr_u32 *)out;

			if (*id < CAMERA_ID_MAX) {
				*role = get_role_by_id(cxt, *id);
			} else {
				ISP_LOGE("invalid camera id %u", *id);
				*role = CAM_SENSOR_MAX;
			}
		}
		break;

	case GET_ISPALG_FW:
		{
			cmr_handle *handle = (cmr_handle *)out;

			*handle = cxt->ispalg_fw_handles[sensor_role];
		}
		break;

	case SET_SLAVE_SENSOR_MODE:
		{
			sem_wait(&cxt->module_sm);
			if (in)
				memcpy(&cxt->match_param.sensor_mode[sensor_role],
						in, sizeof(cmr_u32));
			sem_post(&cxt->module_sm);
		}
		break;

	case GET_SLAVE_SENSOR_MODE:
		{
			sem_wait(&cxt->module_sm);
			if (out)
				memcpy(out,
						&cxt->match_param.sensor_mode[sensor_role],
						sizeof(cmr_u32));
			sem_post(&cxt->module_sm);
		}
		break;

	case SET_SENSOR_SIZE:
		{
			struct isp_size *size = (struct isp_size *)in;

			sem_wait(&cxt->ae_sm);
			data->sensor.sensor_size = *size;
			sem_post(&cxt->ae_sm);
			ISP_LOGD("set %s sensor size %ux%u",
					get_role_name(sensor_role), size->w, size->h);
		}
		break;

	case SET_USER_COUNT:
		sem_wait(&cxt->module_sm);
		//if (user_cnt_get() > 1) {
			if(*(cmr_u32 *)in) {
				cxt->start_user_cnt++;
			}
			else {
				cxt->start_user_cnt--;
			}
		//}
		sem_post(&cxt->module_sm);
		break;

	case GET_USER_COUNT:
		sem_wait(&cxt->module_sm);
		memcpy(out, &cxt->start_user_cnt,sizeof(cxt->start_user_cnt));
		sem_post(&cxt->module_sm);
		break;

	case GET_SENSOR_COUNT:
		sem_wait(&cxt->module_sm);
		if (out) {
			cmr_u32 *cnt = (cmr_u32 *)out;
			*cnt = user_cnt_get();
		}
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
	case SET_AE_TARGET_REGION:
		{
			struct ae_target_region *info = (struct ae_target_region *)in;

			sem_wait(&cxt->ae_sm);
			cxt->ae_region[sensor_role].start_x = info->start_x;
			cxt->ae_region[sensor_role].start_y = info->start_y;
			cxt->ae_region[sensor_role].end_x = info->start_x + info->width;
			cxt->ae_region[sensor_role].end_y = info->start_x + info->height;
			ISP_LOGD("set %s AE region %u %u %u %u",
					get_role_name(sensor_role),
					cxt->ae_region[sensor_role].start_x,
					cxt->ae_region[sensor_role].start_y,
					cxt->ae_region[sensor_role].end_x,
					cxt->ae_region[sensor_role].end_y);
			sem_post(&cxt->ae_sm);
		}
		break;
	case SET_AE_REF_CAMERA_ID:
		{
			sem_wait(&cxt->ae_sm);
			cmr_u32 *id = (cmr_u32 *)in;
			cxt->ae_ref_camera_id = *id;
			sem_post(&cxt->ae_sm);
		}
		break;
	case GET_AE_SYNC_DATA:
		{
			struct ae_sync_data *info = (struct ae_sync_data *)out;

			sem_wait(&cxt->ae_sm);
			info->num = user_cnt_get();
			info->ref_camera_id = cxt->ae_ref_camera_id;
			info->target_rect = cxt->ae_region[sensor_role];
			info->block_size = data->ae.block_size;
			info->block_rect = data->ae.block_rect;
			info->sensor_size = data->sensor.sensor_size;
			sem_post(&cxt->ae_sm);
		}
		break;
		case SET_SYNC_SLAVE_AE_DATA:
			sem_wait(&cxt->ae_sm);
			memcpy(&cxt->match_param.ae_sync_slave_info[sensor_role], in,
				sizeof(cxt->match_param.ae_sync_slave_info[sensor_role]));
			sem_post(&cxt->ae_sm);
		break;

		case GET_SYNC_SLAVE_AE_DATA:
			sem_wait(&cxt->ae_sm);
			memcpy(out, &cxt->match_param.ae_sync_slave_info[sensor_role],
				sizeof(cxt->match_param.ae_sync_slave_info[sensor_role]));
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
	cmr_u32 sensor_role = CAM_SENSOR_MAX;

	ISP_LOGI("camera_id %u, is_master %u, isp_handle %p",
			camera_id, is_master, isp_3a_handle);

	if (user_cnt_inc() == 1) {
		ISP_LOGI("init ispbr_context %p", cxt);
		semaphore_init(cxt);
		role_clear(cxt);
		cxt->start_user_cnt = 0;
		cxt->ae_ref_camera_id = 0;
	}

	sensor_role = role_add(cxt, camera_id, is_master);
	if (sensor_role == CAM_SENSOR_MAX)
		goto role_add_fail;

	ret = stats_data_alloc(cxt, sensor_role);
	if (ret)
		goto alloc_fail;

	cxt->ispalg_fw_handles[sensor_role] = isp_3a_handle;

	return ret;

alloc_fail:
	role_delete(cxt, camera_id);

role_add_fail:
	if (user_cnt_dec() == 0) {
		cxt->ae_ref_camera_id = 0;
		cxt->start_user_cnt = 0;
		role_clear(cxt);
		semaphore_deinit(cxt);
		ISP_LOGI("de-init ispbr_context %p", cxt);
	}

	return ret;
}

cmr_int isp_br_deinit(cmr_u32 camera_id)
{
	cmr_int ret = ISP_SUCCESS;
	struct ispbr_context *cxt = &br_cxt;
	cmr_u32 sensor_role = CAM_SENSOR_MAX;

	sensor_role = get_role_by_id(cxt, camera_id);

	cxt->ispalg_fw_handles[sensor_role] = NULL;

	if (sensor_role < CAM_SENSOR_MAX)
		stats_data_free(cxt, sensor_role);

	role_delete(cxt, camera_id);

	if (user_cnt_dec() == 0) {
		cxt->ae_ref_camera_id = 0;
		cxt->start_user_cnt = 0;
		role_clear(cxt);
		semaphore_deinit(cxt);
		ISP_LOGI("de-init ispbr_context %p", cxt);
	}

	return ret;
}
