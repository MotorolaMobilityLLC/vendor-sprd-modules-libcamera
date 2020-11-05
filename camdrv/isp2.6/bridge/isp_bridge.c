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
#include "cmr_common.h"

#define ISP_BLK_NUM_TO_SIZE(num) (3 * (num) * sizeof(cmr_u32))
#define ISP_AEM_STAT_BLK_NUM_MAX (128 * 128)
#define ISP_HIST_STAT_BLK_NUM_MAX (256)
#define ISP_AEM_STAT_SIZE_MAX ISP_BLK_NUM_TO_SIZE(ISP_AEM_STAT_BLK_NUM_MAX)
#define ISP_HIST_STAT_SIZE_MAX ISP_BLK_NUM_TO_SIZE(ISP_HIST_STAT_BLK_NUM_MAX)

struct ae_data {
	struct isp_size block_size;
	struct isp_rect block_rect;
	struct visible_region_info visible_region;
	struct ae_match_stats_data stats_data;
	cmr_u32 stat_blk_num;
	/* MW to ISP */
	struct aem_win_info win;
};

struct awb_data {
	struct awb_match_stats_data stats_data;
};

struct hist_data {
	/* MW to ISP */
	cmr_u32 idx;
	cmr_u32 sec;
	cmr_u32 usec;
	struct isp_rect win;
	struct isp_hist_statistic_info stats_info[3];
};

struct sensor_data {
	struct isp_size sensor_size;
};

struct match_data {
	struct ae_data ae;
	struct awb_data awb;
	struct hist_data hist;
	struct sensor_data sensor;
};

struct match_data_param {
	struct module_info module_info;
	float zoom_ratio;
	cmr_u32 frameId;

	struct aem_info aem_stat_info[CAM_SENSOR_MAX];
	struct ae_match_data ae_info[CAM_SENSOR_MAX];
	struct ae_sync_actual_data ae_sync_actual_output[CAM_SENSOR_MAX];
	cmr_u16 bv[CAM_SENSOR_MAX];
	cmr_u8 flash_state[CAM_SENSOR_MAX];

	struct awb_match_data awb_info[CAM_SENSOR_MAX];
	struct awb_gain_data awb_gain[CAM_SENSOR_MAX];

	struct fov_data fov_info[CAM_SENSOR_MAX];
	// TODO joseph af_role is totally different
	struct af_status_info af_info[CAM_SENSOR_MAX];
	struct af_manual_info af_manual[CAM_SENSOR_MAX];

	cmr_u32 sensor_mode[CAM_SENSOR_MAX];

	struct match_data data[CAM_SENSOR_MAX];
	struct ae_lib_output_data ae_lib_output[CAM_SENSOR_MAX];
	struct ae_sync_lib_outout_data ae_sync_lib_output[CAM_SENSOR_MAX];
	struct isp_hist_statistic_info y_hist[CAM_SENSOR_MAX];
};

struct awb_master_sync_data {
	cmr_u32 ct;
	cmr_u32 tint;
};

struct ispbr_context {
	cmr_u32 start_user_cnt;
	cmr_handle ispalg_fw_handles[CAM_SENSOR_MAX];

	sem_t module_sm;
	sem_t ae_sm;
	sem_t awb_sm;
	sem_t af_sm;
	struct match_data_param match_param;
	struct awb_master_sync_data awb_master_data;

	cmr_u32 ae_ref_camera_id;
	struct ae_rect_data ae_region[CAM_SENSOR_MAX];

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

static inline cmr_u32 user_cnt_inc(struct ispbr_context *cxt) {
	cmr_u32 cnt = 0;

	pthread_mutex_lock(&g_br_mutex);
	cnt = ++g_br_user_cnt;

	if (cnt == 1) {
		ISP_LOGI("init ispbr_context %p", cxt);
		semaphore_init(cxt);
		role_clear(cxt);
		cxt->start_user_cnt = 0;
		cxt->ae_ref_camera_id = 0;
	}

	pthread_mutex_unlock(&g_br_mutex);
	ISP_LOGV("cnt = %d", cnt);

	return cnt;
}

static inline cmr_u32 user_cnt_dec(struct ispbr_context *cxt) {
	cmr_u32 cnt = 0;

	pthread_mutex_lock(&g_br_mutex);
	cnt = --g_br_user_cnt;

	if (cnt == 0) {
		cxt->ae_ref_camera_id = 0;
		cxt->start_user_cnt = 0;
		role_clear(cxt);
		semaphore_deinit(cxt);
		ISP_LOGI("de-init ispbr_context %p", cxt);
	}

	pthread_mutex_unlock(&g_br_mutex);
	ISP_LOGV("cnt = %d", cnt);

	return cnt;
}

static inline cmr_u32 user_cnt_get() {
	return g_br_user_cnt;
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

static cmr_int stats_data_alloc(struct ispbr_context *cxt, cmr_u32 sensor_role) {
	cmr_int ret = ISP_SUCCESS;
	struct match_data *data = &cxt->match_param.data[sensor_role];

	data->ae.stats_data.stats_data = malloc(ISP_AEM_STAT_SIZE_MAX);
	if (!data->ae.stats_data.stats_data) {
		ISP_LOGE("fail to alloc AE stats data");
		ret = -ISP_ALLOC_ERROR;
		goto ae_alloc_fail;
	}

	data->awb.stats_data.stats_data = malloc(ISP_AEM_STAT_SIZE_MAX);
	if (!data->awb.stats_data.stats_data) {
		ISP_LOGE("fail to alloc AWB stats data");
		ret = -ISP_ALLOC_ERROR;
		goto awb_alloc_fail;
	}

	return ret;

awb_alloc_fail:
	free(data->ae.stats_data.stats_data);
	data->ae.stats_data.stats_data = NULL;

ae_alloc_fail:

	return ret;
}

static void stats_data_free(struct ispbr_context *cxt, cmr_u32 sensor_role) {
	struct match_data *data = &cxt->match_param.data[sensor_role];

	free(data->awb.stats_data.stats_data);
	data->awb.stats_data.stats_data = NULL;
	free(data->ae.stats_data.stats_data);
	data->ae.stats_data.stats_data = NULL;
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
		{
			sem_wait(&cxt->ae_sm);
			if (out)
				memcpy(out, &cxt->match_param.ae_info[sensor_role],
					sizeof(cxt->match_param.ae_info[sensor_role]));
			sem_post(&cxt->ae_sm);
		}
		break;

	case SET_Y_HIST_PARAM:
		{
			sem_wait(&cxt->ae_sm);
			memcpy(&cxt->match_param.y_hist[sensor_role], in,
				sizeof(cxt->match_param.y_hist[sensor_role]));
			sem_post(&cxt->ae_sm);
		}
		break;

	case GET_Y_HIST_PARAM:
		{
			sem_wait(&cxt->ae_sm);
			if (out)
				memcpy(out, &cxt->match_param.y_hist[sensor_role],
					sizeof(cxt->match_param.y_hist[sensor_role]));
			sem_post(&cxt->ae_sm);
		}
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
			*awb_data = data->awb.stats_data.stats_data;
			sem_post(&cxt->module_sm);
		}
		break;

	case SET_AEM_SYNC_STAT:
		{
			struct ae_match_stats_data *stats_data = (struct ae_match_stats_data *)in;

			sem_wait(&cxt->module_sm);
			if (stats_data && stats_data->stats_data && data->ae.stats_data.stats_data) {
				data->ae.stats_data.monoboottime = stats_data->monoboottime;
				data->ae.stats_data.is_last_frm = stats_data->is_last_frm;
				data->ae.stats_data.len = stats_data->len;
				memcpy(data->ae.stats_data.stats_data,
						stats_data->stats_data, data->ae.stats_data.len);
			}
			sem_post(&cxt->module_sm);
 		}
		break;

	case GET_AEM_SYNC_STAT:
		{
			struct ae_match_stats_data *stats_data = (struct ae_match_stats_data *)out;

			sem_wait(&cxt->module_sm);
			if (stats_data && stats_data->stats_data && data->ae.stats_data.stats_data) {
				stats_data->monoboottime = data->ae.stats_data.monoboottime;
				stats_data->is_last_frm = data->ae.stats_data.is_last_frm;
				// TODO joseph stats_data->len is not used in SET_AEM_SYNC_STAT
				stats_data->len = ISP_BLK_NUM_TO_SIZE(data->ae.stat_blk_num);
				memcpy(stats_data->stats_data,
						data->ae.stats_data.stats_data, data->ae.stats_data.len);
			}
			sem_post(&cxt->module_sm);
 		}
		break;

	case SET_AEM_STAT_BLK_NUM:
		sem_wait(&cxt->module_sm);
		if (in) {
			cmr_u32 *blk_num = (cmr_u32 *)in;
			data->ae.stat_blk_num = *blk_num;
			ISP_LOGV("set %s AEM stat_blk_num %u",
					get_role_name(sensor_role), data->ae.stat_blk_num);
		}
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

	case SET_AE_WIN:
		{
			if (in) {
				struct aem_win_info *win = (struct aem_win_info *)in;

				sem_wait(&cxt->ae_sm);
				data->ae.win = *win;

				ISP_LOGV("set %s AE offset %d %d, blk_num %u %u, blk_size %u %u",
						get_role_name(sensor_role), win->offset_x, win->offset_y,
						win->blk_num_x, win->blk_num_y, win->blk_size_x, win->blk_size_y);
				sem_post(&cxt->ae_sm);
			}
		}
		break;

	case GET_AE_WIN:
		{
			if (out) {
				struct aem_win_info *win = (struct aem_win_info *)out;

				sem_wait(&cxt->ae_sm);
				*win = data->ae.win;
				sem_post(&cxt->ae_sm);
			}
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
		if (in && data->awb.stats_data.stats_data)
			memcpy(data->awb.stats_data.stats_data, in, ISP_AEM_STAT_SIZE_MAX);
		sem_post(&cxt->awb_sm);
		break;

	case GET_STAT_AWB_DATA:
		sem_wait(&cxt->awb_sm);
		if (out && data->awb.stats_data.stats_data)
			memcpy(out, data->awb.stats_data.stats_data, ISP_AEM_STAT_SIZE_MAX);
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
	case SET_MASTER_AWB_DATA:
		ISP_LOGV("SET_MASTER_AWB_DATA : ");
		if(in) {
			struct awb_master_sync_data *in_tmp = (struct awb_master_sync_data *)in;
			sem_wait(&cxt->awb_sm);
			memcpy(&cxt->awb_master_data, in_tmp, sizeof(cxt->awb_master_data));
			ISP_LOGV("set master_awb_data: %d, %d", cxt->awb_master_data.ct, cxt->awb_master_data.tint);
			sem_post(&cxt->awb_sm);
		} else {
			ISP_LOGE("input param is NULL");
		}
		break;
	case GET_MASTER_AWB_DATA:
		ISP_LOGV("GET_MASTER_AWB_DATA : ");
		struct awb_master_sync_data *out_tmp = (struct awb_master_sync_data *)out;
		sem_wait(&cxt->awb_sm);
		memcpy(out_tmp, &cxt->awb_master_data, sizeof(cxt->awb_master_data));
		ISP_LOGV("get master_awb_data: %d, %d", out_tmp->ct, out_tmp->tint);
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

	case SET_HIST_WIN:
		{
			if (in) {
				struct img_rect *win = (struct img_rect *)in;

				sem_wait(&cxt->ae_sm);
				data->hist.win.st_x = win->start_x;
				data->hist.win.st_y = win->start_y;
				data->hist.win.width = win->width;
				data->hist.win.height = win->height;
				ISP_LOGV("set %s HIST win %u %u %u %u", get_role_name(sensor_role),
						win->start_x, win->start_y, win->width, win->height);
				sem_post(&cxt->ae_sm);
			}
		}
		break;

	case GET_HIST_WIN:
		{
			if (out) {
				struct isp_rect *win = (struct isp_rect *)out;

				sem_wait(&cxt->ae_sm);
				*win = data->hist.win;
				sem_post(&cxt->ae_sm);
			}
		}
		break;

	case SET_HIST_PARAM:
		{
			if (in) {
				struct hist_param *param = (struct hist_param *)in;

				sem_wait(&cxt->ae_sm);
				data->hist.idx = param->idx;
				data->hist.sec = param->sec;
				data->hist.usec = param->usec;
				data->hist.win = param->win;
				ISP_LOGV("set %s HIST idx %u, ts %u.%u, win %d %d %u %u",
						get_role_name(sensor_role), param->idx,
						param->sec, param->usec,
						param->win.st_x, param->win.st_y,
						param->win.width, param->win.height);
				sem_post(&cxt->ae_sm);
			}
		}
		break;

	case GET_HIST_PARAM:
		{
			if (out) {
				struct hist_param *param = (struct hist_param *)out;

				sem_wait(&cxt->ae_sm);
				param->idx = data->hist.idx;
				param->sec = data->hist.sec;
				param->usec = data->hist.usec;
				param->win = data->hist.win;
				sem_post(&cxt->ae_sm);
			}
		}
		break;

	case SET_HIST_STATS:
		{
			if (in) {
				sem_wait(&cxt->ae_sm);
				memcpy(data->hist.stats_info, in, sizeof(data->hist.stats_info));
				sem_post(&cxt->ae_sm);
			}
		}
		break;

	case GET_HIST_STATS:
		{
			if (out) {
				sem_wait(&cxt->ae_sm);
				memcpy(out, data->hist.stats_info, sizeof(data->hist.stats_info));
				sem_post(&cxt->ae_sm);
			}
		}
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
        case GET_AF_SYNC_INFO:
		if (out) {
			struct af_sync_info *info = (struct af_sync_info *)out;
			sem_wait(&cxt->ae_sm);
			info->cur_master_id = cxt->ae_ref_camera_id;
			info->camera_id = 0;
			info->sensor_role = 0;
			sem_post(&cxt->ae_sm);
		}

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
			ISP_LOGV("set %s sensor size %ux%u",
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

	case SET_GLOBAL_ZOOM_RATIO:
		{
			if (in) {
				float *ratio = (float *)in;

				sem_wait(&cxt->module_sm);
				cxt->match_param.zoom_ratio = *ratio;
				ISP_LOGV("set %s ratio %f", get_role_name(sensor_role), *ratio);
				sem_post(&cxt->module_sm);
			}
		}
		break;

	case GET_GLOBAL_ZOOM_RATIO:
		{
			if (out) {
				float *ratio = (float *)out;

				sem_wait(&cxt->module_sm);
				*ratio = cxt->match_param.zoom_ratio;;
				sem_post(&cxt->module_sm);
			}
		}
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
			ISP_LOGV("set %s AE region %u %u %u %u",
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
	case SET_AE_VISIBLE_REGION:
		{
			sem_wait(&cxt->ae_sm);
			if (in) {
				struct visible_region_info *info = (struct visible_region_info *)in;
				ISP_LOGV("set %s AE visible %u %u %u %u, serial %u",
						get_role_name(sensor_role),
						info->region.start_x,
						info->region.start_y,
						info->region.width,
						info->region.height,
						info->serial_no);
				data->ae.visible_region = *info;
			}
			sem_post(&cxt->ae_sm);
		}
		break;
	case GET_AE_VISIBLE_REGION:
		{
			sem_wait(&cxt->ae_sm);
			if (out) {
				struct visible_region_info *info = (struct visible_region_info *)out;
				*info = data->ae.visible_region;
			}
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

	case SET_SYNC_SLAVE_ACTUAL_DATA:
		sem_wait(&cxt->ae_sm);
		memcpy(&cxt->match_param.ae_sync_actual_output[sensor_role], in,
			sizeof(cxt->match_param.ae_sync_actual_output[sensor_role]));
		sem_post(&cxt->ae_sm);
		break;

	case GET_SYNC_SLAVE_ACTUAL_DATA:
		sem_wait(&cxt->ae_sm);
		memcpy(out, &cxt->match_param.ae_sync_actual_output[sensor_role],
			sizeof(cxt->match_param.ae_sync_actual_output[sensor_role]));
		sem_post(&cxt->ae_sm);
		break;

	case SET_SYNC_SLAVE_LIB_OUTPUT:
		sem_wait(&cxt->ae_sm);
		memcpy(&cxt->match_param.ae_lib_output[sensor_role], in,
			sizeof(cxt->match_param.ae_lib_output[sensor_role]));
		sem_post(&cxt->ae_sm);
		break;

	case GET_SYNC_SLAVE_LIB_OUTPUT:
		sem_wait(&cxt->ae_sm);
		memcpy(out, &cxt->match_param.ae_lib_output[sensor_role],
			sizeof(cxt->match_param.ae_lib_output[sensor_role]));
		sem_post(&cxt->ae_sm);
		break;

	case SET_SYNC_SLAVE_SYNC_OUTPUT:
			sem_wait(&cxt->ae_sm);
			memcpy(&cxt->match_param.ae_sync_lib_output[sensor_role], in,
				sizeof(cxt->match_param.ae_sync_lib_output[sensor_role]));
			sem_post(&cxt->ae_sm);
		break;

	case GET_SYNC_SLAVE_SYNC_OUTPUT:
		sem_wait(&cxt->ae_sm);
			memcpy(out, &cxt->match_param.ae_sync_lib_output[sensor_role],
				sizeof(cxt->match_param.ae_sync_lib_output[sensor_role]));
			sem_post(&cxt->ae_sm);
		break;

	case SET_SYNC_SLAVE_AEM_INFO:

		sem_wait(&cxt->ae_sm);
		if (in && data->ae.stats_data.stats_data)
			memcpy(data->ae.stats_data.stats_data, in, ISP_BLK_NUM_TO_SIZE(32*32));
		sem_post(&cxt->ae_sm);
		break;

	case GET_SYNC_SLAVE_AEM_INFO:

		sem_wait(&cxt->ae_sm);
		if (out && data->ae.stats_data.stats_data)
			memcpy(out, data->ae.stats_data.stats_data, ISP_BLK_NUM_TO_SIZE(32*32));
		sem_post(&cxt->ae_sm);
		break;

	case SET_FRAME_ID:
		{
			if (in) {
				cmr_u32 *frameId = (cmr_u32 *)in;

				sem_wait(&cxt->module_sm);
				cxt->match_param.frameId = *frameId;
				sem_post(&cxt->module_sm);
			}
		}
		break;

	case GET_FRAME_ID:
		{
			if (out) {
				cmr_u32 *frameId = (cmr_u32 *)out;
				sem_wait(&cxt->module_sm);
				*frameId = cxt->match_param.frameId;;
				sem_post(&cxt->module_sm);
			}
		}
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

	user_cnt_inc(cxt);

	sensor_role = role_add(cxt, camera_id, is_master);
	if (sensor_role == CAM_SENSOR_MAX) {
        ret = -ISP_ERROR;
		goto role_add_fail;
    }

	ret = stats_data_alloc(cxt, sensor_role);
	if (ret)
		goto alloc_fail;

	cxt->ispalg_fw_handles[sensor_role] = isp_3a_handle;

	return ret;

alloc_fail:
	role_delete(cxt, camera_id);

role_add_fail:
	user_cnt_dec(cxt);

	return ret;
}

cmr_int isp_br_deinit(cmr_u32 camera_id)
{
	cmr_int ret = ISP_SUCCESS;
	struct ispbr_context *cxt = &br_cxt;
	cmr_u32 sensor_role = CAM_SENSOR_MAX;

	ISP_LOGI("camera_id %u", camera_id);

	sensor_role = get_role_by_id(cxt, camera_id);

	cxt->ispalg_fw_handles[sensor_role] = NULL;

	if (sensor_role < CAM_SENSOR_MAX)
		stats_data_free(cxt, sensor_role);

	role_delete(cxt, camera_id);

	user_cnt_dec(cxt);

	return ret;
}
