/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "ai_sprd_adpt"

#include "ai_sprd_adpt.h"
#include "sprdaicapi.h"
#include "ai_ctrl.h"
#include "isp_type.h"
#include "pthread.h"
#include "cmr_types.h"
#include "inttypes.h"
#include <cutils/properties.h>

#ifdef ISP_LOGV
#undef ISP_LOGV
cmr_int g_ai_log_level = LEVEL_OVER_LOGD;
extern long g_isp_log_level;
#define ISP_LOGV(format, ...)                                                  \
	ALOGD_IF((((g_ai_log_level >= LEVEL_OVER_LOGV)||(g_isp_log_level >= LEVEL_OVER_LOGV))&&(g_ai_log_level!=6)), DEBUG_STR format, DEBUG_ARGS, \
        ##__VA_ARGS__)
#endif

struct ai_ctrl_cxt {
	struct aictrl_cxt *aictrl_cxt_ptr;
	struct ai_scene_detect_info scene_info;
	pthread_mutex_t data_sync_lock;
	aic_version_t aic_ver;
	aic_option_t aic_opt;
	aic_handle_t aic_handle;
	aic_aeminfo_t aic_aeminfo;
	aic_result_t aic_result;
	aic_faceinfo_t aic_faceinfo;
	aic_status_t aic_img_status;
	aic_image_t aic_image;
	enum ai_status aic_status;
	cmr_u32 fd_on_off;
	cmr_u32 temp_fd_frameid;
	cmr_u32 flash_enable;
};

cmr_handle ai_sprd_adpt_init(cmr_handle handle, cmr_handle param)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct ai_ctrl_cxt *cxt = NULL;
	char ai_property[PROPERTY_VALUE_MAX];
	int val = 0;

	UNUSED(param);

	cxt = (struct ai_ctrl_cxt *)malloc(sizeof(struct ai_ctrl_cxt));

	if (NULL == cxt) {
		rtn = ISP_ERROR;
		ISP_LOGE("fail to malloc");
		goto exit;
	}
	memset(cxt, 0, sizeof(*cxt));

	pthread_mutex_init(&cxt->data_sync_lock, NULL);

	if (NULL == handle) {
		ISP_LOGE("fail to get input param %p\r\n", handle);
		rtn = ISP_ERROR;
		goto exit;
	}
	cxt->aictrl_cxt_ptr = (struct aictrl_cxt *)handle;
	ISP_LOGD("ai_sprd_adpt_init, cameraId:%d\n", cxt->aictrl_cxt_ptr->cameraId);

	AIC_InitOption(&cxt->aic_opt);
	ISP_LOGI("01_aci opt: min_frame_interval: %d.\n", cxt->aic_opt.min_frame_interval);
	ISP_LOGI("01_aci opt: max_frame_interval: %d.\n", cxt->aic_opt.max_frame_interval);
	ISP_LOGI("01_aci opt: thread_num: %d.\n", cxt->aic_opt.thread_num);
	ISP_LOGI("01_aci opt: sync_with_worker: %d.\n", cxt->aic_opt.sync_with_worker);

	#ifdef CONFIG_ISP_2_5
	cxt->aic_opt.min_frame_interval = 4;
	cxt->aic_opt.thread_num = 1;
	#endif

	#ifdef CONFIG_ISP_2_6
	cxt->aic_opt.min_frame_interval = 2;
	cxt->aic_opt.thread_num = 1;
	#endif

	#ifdef CONFIG_ISP_2_7
	cxt->aic_opt.min_frame_interval = 2;
	cxt->aic_opt.thread_num = 1;
	#endif
	
	cxt->aic_opt.camera_id = cxt->aictrl_cxt_ptr->cameraId;

	ISP_LOGI("02_aci opt: min_frame_interval:%d, max_frame_interval:%d,\
	thread_num:%d, sync_with_worker:%d,cameraId:%d\n",\
	cxt->aic_opt.min_frame_interval, cxt->aic_opt.max_frame_interval,\
	cxt->aic_opt.thread_num, cxt->aic_opt.sync_with_worker, cxt->aic_opt.camera_id);

	if (0 != AIC_CreateHandle(&cxt->aic_handle, &cxt->aic_opt)) {
		ISP_LOGE("fail to creat aic handle.\n");
		rtn = ISP_ERROR;
		goto exit;
	}

	if (0 != AIC_GetVersion(&cxt->aic_ver)) {
		ISP_LOGW("fail to get aic version.\n");
	} else {
		ISP_LOGI("aci version: built data: %s.\n", cxt->aic_ver.built_date);
		ISP_LOGI("aci version: built time: %s.\n", cxt->aic_ver.built_time);
		ISP_LOGI("aci version: built rev: %s.\n", cxt->aic_ver.built_rev);
	}

	cxt->aic_status = AI_STATUS_IDLE;

	memset((cmr_handle) & ai_property, 0, sizeof(ai_property));
	property_get("persist.vendor.cam.isp.ai.logv", ai_property, "0");
	val = atoi(ai_property);
	if (0 < val)
		g_ai_log_level = val;

	ISP_LOGI("done.");

	return (cmr_handle)cxt;

exit:
	if (NULL != cxt) {
		free(cxt);
		cxt = NULL;
	}
	ISP_LOGE("fail to init ai %d", rtn);
	return NULL;
}

static cmr_s32 ai_check_handle(cmr_handle handle)
{
	if (NULL == handle) {
		ISP_LOGE("fail to check handle");
		return ISP_ERROR;
	}

	return ISP_SUCCESS;
}

static cmr_s32 ai_sprd_set_ae_param(cmr_handle handle, struct ai_ae_param *ae_param, struct ai_scene_detect_info *scene_info)
{
	cmr_s32 rtn = ISP_SUCCESS, i;
	struct ai_ctrl_cxt *cxt = (struct ai_ctrl_cxt *)handle;
	enum ai_scene_type scene_id;

	ISP_LOGV("frameID: %d, zoom_ratio: %d.", ae_param->frame_id, ae_param->zoom_ratio);
	ISP_LOGV("blk_width: %d, blk_height: %d.", ae_param->blk_width, ae_param->blk_height);
	ISP_LOGV("blk_num_hor: %d, blk_num_ver: %d.", ae_param->blk_num_hor, ae_param->blk_num_ver);
	ISP_LOGV("timestamp: %"PRIu64".", ae_param->timestamp);
	ISP_LOGV("r_g_b_info: %p, %p, %p.", ae_param->ae_stat.r_info, ae_param->ae_stat.g_info, ae_param->ae_stat.b_info);
	ISP_LOGV("r_g_b_info data: %d, %d, %d.", *ae_param->ae_stat.r_info, *ae_param->ae_stat.g_info, *ae_param->ae_stat.b_info);
	ISP_LOGV("curr_bv: %d, flash_enable: %d.", ae_param->curr_bv, cxt->flash_enable);

	cxt->aic_aeminfo.frame_id = ae_param->frame_id;
	cxt->aic_aeminfo.timestamp = ae_param->timestamp;
	cxt->aic_aeminfo.ae_stat.r_stat = ae_param->ae_stat.r_info;
	cxt->aic_aeminfo.ae_stat.g_stat = ae_param->ae_stat.g_info;
	cxt->aic_aeminfo.ae_stat.b_stat = ae_param->ae_stat.b_info;
	memcpy(&cxt->aic_aeminfo.ae_rect, &ae_param->ae_rect, sizeof(struct ai_rect));
	cxt->aic_aeminfo.blk_width = ae_param->blk_width;
	cxt->aic_aeminfo.blk_height = ae_param->blk_height;
	cxt->aic_aeminfo.blk_num_hor = ae_param->blk_num_hor;
	cxt->aic_aeminfo.blk_num_ver = ae_param->blk_num_ver;
	cxt->aic_aeminfo.zoom_ratio = ae_param->zoom_ratio;
	cxt->aic_aeminfo.curr_bv = ae_param->curr_bv;
	cxt->aic_aeminfo.flash_enable = cxt->flash_enable;
	cxt->aic_aeminfo.stable = ae_param->stable;
	cxt->aic_aeminfo.data_valid = 1;
	ISP_LOGV("ae_param_stable: %d, aic_aeminfo_stable:%d", ae_param->stable, cxt->aic_aeminfo.stable);

	if (0 != AIC_SetAemInfo(cxt->aic_handle, &cxt->aic_aeminfo, &cxt->aic_result)) {
		rtn = ISP_ERROR;
		ISP_LOGE("fail to AIC_SetAemInfo.");
		goto exit;
	}

	ISP_LOGV("ai result: frameID: %d, label: %d.", cxt->aic_result.frame_id, cxt->aic_result.scene_label);
	ISP_LOGV("ai result: task0: %d, %d; task1: %d, %d; task2: %d, %d.",
		cxt->aic_result.task0[0].id, cxt->aic_result.task0[0].score,
		cxt->aic_result.task1[0].id, cxt->aic_result.task1[0].score,
		cxt->aic_result.task2[0].id, cxt->aic_result.task2[0].score);

	scene_info->frame_id = cxt->aic_result.frame_id;
	for (i = 0; i < AI_SCENE_TASK0_MAX; i++) {
		scene_info->task0[i].id = cxt->aic_result.task0[i].id;
		scene_info->task0[i].score = cxt->aic_result.task0[i].score;
	}
	for (i = 0; i < AI_SCENE_TASK1_MAX; i++) {
		scene_info->task1[i].id = cxt->aic_result.task1[i].id;
		scene_info->task1[i].score = cxt->aic_result.task1[i].score;
	}
	for (i = 0; i < AI_SCENE_TASK2_MAX; i++) {
		scene_info->task2[i].id = cxt->aic_result.task2[i].id;
		scene_info->task2[i].score = cxt->aic_result.task2[i].score;
	}

	switch(cxt->aic_result.scene_label) {
		case SC_LABEL_NONE:
			scene_id = AI_SCENE_DEFAULT;
			break;
		case SC_LABEL_PEOPLE:
			scene_id = AI_SCENE_PORTRAIT;
			break;
		case SC_LABEL_NIGHT:
			scene_id = AI_SCENE_NIGHT;
			break;
		case SC_LABEL_BACKLIGHT:
			scene_id = AI_SCENE_BACKLIGHT;
			break;
		case SC_LABEL_SUNRISESET:
			scene_id = AI_SCENE_SUNRISE;
			break;
		case SC_LABEL_FIREWORK:
			scene_id = AI_SCENE_FIREWORK;
			break;
		case SC_LABEL_FOOD:
			scene_id = AI_SCENE_FOOD;
			break;
		case SC_LABEL_GREENPLANT:
			scene_id = AI_SCENE_FOLIAGE;
			break;
		case SC_LABEL_DOCUMENT:
			scene_id = AI_SCENE_TEXT;
			break;
		case SC_LABEL_CATDOG:
			scene_id = AI_SCENE_PET;
			break;
		case SC_LABEL_FLOWER:
			scene_id = AI_SCENE_FLOWER;
			break;
		case SC_LABEL_BLUESKY:
			scene_id = AI_SCENE_SKY;
			break;
		case SC_LABEL_BUILDING:
			scene_id = AI_SCENE_BUILDING;
			break;
		case SC_LABEL_SNOW:
			scene_id = AI_SCENE_SNOW;
			break;
		default:
			scene_id = AI_SCENE_DEFAULT;
			break;
	}

	scene_info->cur_scene_id = scene_id;
	memcpy(&(cxt->scene_info), scene_info, sizeof(struct ai_scene_detect_info));

	ISP_LOGV("done. cur_scene_id: %d.", scene_info->cur_scene_id);
	return rtn;

exit:
	ISP_LOGE("fail to set ae param. rtn: %d.", rtn);
	return rtn;
}

static cmr_s32 ai_io_ctrl_sync(cmr_handle handle, cmr_s32 cmd, cmr_handle param, cmr_handle result)
{
	cmr_s32 rtn = ISP_SUCCESS, i;
	struct ai_ctrl_cxt *cxt = NULL;
	struct aictrl_cxt *aictrl_cxt_ptr = NULL;
	struct ai_img_param *ai_img_ptr = NULL;

	rtn = ai_check_handle(handle);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to check handle %p", handle);
		return ISP_ERROR;
	}

	cxt = (struct ai_ctrl_cxt *)handle;
	aictrl_cxt_ptr = (struct aictrl_cxt *)cxt->aictrl_cxt_ptr;

	ISP_LOGV("cmd: %d.", cmd);

	switch (cmd) {
	case AI_SET_FD_PARAM:
		if (AI_STATUS_PROCESSING != cxt->aic_status) {
			ISP_LOGD("ai set fd doesn't work. status: %d.", cxt->aic_status);
			rtn = ISP_ERROR;
			goto exit;
		}
		if (!param) {
			ISP_LOGD("fail to set fd param");
			rtn = ISP_ERROR;
			goto exit;
		}
		memcpy(&cxt->aic_faceinfo, param, sizeof(struct ai_fd_param));
		ISP_LOGV("ai fd: width: %d, height: %d, facenum: %d.", cxt->aic_faceinfo.width, cxt->aic_faceinfo.height, cxt->aic_faceinfo.face_num);
		ISP_LOGV("ai fd: frame_id: %d, timestamp: %"PRIu64".", cxt->aic_faceinfo.frame_id, cxt->aic_faceinfo.timestamp);
		for (i = 0; i < cxt->aic_faceinfo.face_num; i++) {
			cxt->aic_faceinfo.face_area[i].score = cxt->aic_faceinfo.face_area[i].score * 10;
			ISP_LOGV("ai fd: face %d, x: %d, y: %d, w: %d, h: %d.",
				i, cxt->aic_faceinfo.face_area[i].x, cxt->aic_faceinfo.face_area[i].y, cxt->aic_faceinfo.face_area[i].width, cxt->aic_faceinfo.face_area[i].height);
			ISP_LOGV("ai fd: yaw: %d, roll: %d, score: %d, human_id: %d.",
				cxt->aic_faceinfo.face_area[i].yaw_angle, cxt->aic_faceinfo.face_area[i].roll_angle, cxt->aic_faceinfo.face_area[i].score, cxt->aic_faceinfo.face_area[i].human_id);
		}
		cxt->temp_fd_frameid = cxt->aic_faceinfo.frame_id;

		if(1 == cxt->fd_on_off){
			if (0 != AIC_SetFaceInfo(cxt->aic_handle, &cxt->aic_faceinfo)) {
				rtn = ISP_ERROR;
				ISP_LOGE("fail to AIC_SetFaceInfo.");
				goto exit;
			}
		}
		break;
	case AI_SET_FD_ON_OFF:
		if (!param) {
			ISP_LOGE("fail to set fd on off");
			rtn = ISP_ERROR;
			goto exit;
		}
		cxt->fd_on_off = *(cmr_u32 *)param;
		ISP_LOGD("FD_ON_OFF flag is: %d", cxt->fd_on_off);

		if(0 == cxt->fd_on_off){
			memset(&cxt->aic_faceinfo, 0, sizeof(struct ai_fd_param));
			cxt->aic_faceinfo.frame_id = cxt->temp_fd_frameid + 1;
                	cxt->aic_faceinfo.width = 1;
                	cxt->aic_faceinfo.height = 1;

			if (0 != AIC_SetFaceInfo(cxt->aic_handle, &cxt->aic_faceinfo)) {
				rtn = ISP_ERROR;
				ISP_LOGE("fail to AIC_SetFaceInfo.");
				goto exit;
			}
		}
		break;
	case AI_SET_FLASH_NOTICE:
		if (!param) {
			ISP_LOGE("fail to set flash notice");
			goto exit;
		}
		cmr_u32 flash_status = *(cmr_u32 *) param;
		switch (flash_status) {
			case AI_FLASH_PRE_BEFORE:
			case AI_FLASH_PRE_LIGHTING:
			case AI_FLASH_MAIN_BEFORE:
			case AI_FLASH_MAIN_LIGHTING:
				cxt->flash_enable = 1;
				break;

			case AI_FLASH_MAIN_AFTER:
			case AI_FLASH_PRE_AFTER:
				cxt->flash_enable = 0;
				break;

			default:
				break;

			ISP_LOGI("flash_enable flag is: %d", cxt->flash_enable);
		}
		break;
	case AI_SET_AE_PARAM:
		if (AI_STATUS_PROCESSING != cxt->aic_status) {
			ISP_LOGD("ai set ae param doesn't work. status: %d.", cxt->aic_status);
			rtn = ISP_ERROR;
			goto exit;
		}
		if (!param) {
			ISP_LOGD("fail to set ae param. param is null.");
			rtn = ISP_ERROR;
			goto exit;
		}
		if (!result) {
			ISP_LOGD("fail to set ae param. result is null.");
			rtn = ISP_ERROR;
			goto exit;
		}
		if (0 != ai_sprd_set_ae_param(cxt, (struct ai_ae_param *)param, (struct ai_scene_detect_info *)result)) {
			ISP_LOGD("fail to set ae param.");
			rtn = ISP_ERROR;
			goto exit;
		}
		break;
	case AI_SET_IMG_PARAM:
		if (AI_STATUS_PROCESSING != cxt->aic_status) {
			ISP_LOGD("ai set image param doesn't work. status: %d.", cxt->aic_status);
			rtn = ISP_ERROR;
			goto exit;
		}
		if (!param) {
			ISP_LOGD("fail to set img data");
			rtn = ISP_ERROR;
			goto exit;
		}
		ai_img_ptr = (struct ai_img_param *)param;
		cxt->aic_image.frame_id = ai_img_ptr->frame_id;
		cxt->aic_image.timestamp = ai_img_ptr->timestamp;
		cxt->aic_image.sd_img.csp = SD_CSP_NV21;
		cxt->aic_image.sd_img.width = ai_img_ptr->width;
		cxt->aic_image.sd_img.height = ai_img_ptr->height;
		cxt->aic_image.sd_img.plane = 2;
		cxt->aic_image.sd_img.stride[0] = ai_img_ptr->img_y_pitch;
		cxt->aic_image.sd_img.stride[1] = ai_img_ptr->img_uv_pitch;
		cxt->aic_image.sd_img.data[0] = (uint8_t *)((uint64_t)ai_img_ptr->img_buf.img_y);
		cxt->aic_image.sd_img.data[1] = (uint8_t *)((uint64_t)ai_img_ptr->img_buf.img_uv);
		cxt->aic_image.sd_img.bufptr = (uint8_t *)((uint64_t)ai_img_ptr->img_buf.img_y);
		cxt->aic_image.sd_img.bufsize = (ai_img_ptr->img_y_pitch * ai_img_ptr->height)+(ai_img_ptr->img_uv_pitch * ai_img_ptr->height/2);
		cxt->aic_image.sd_img.is_continuous = ai_img_ptr->is_continuous;
		cxt->aic_image.orientation = (SD_ORNT)(ai_img_ptr->orientation);
		ISP_LOGV("ai img: frame_id: %d. timestamp: %"PRIu64"", cxt->aic_image.frame_id, cxt->aic_image.timestamp);
		if (0 != AIC_SetImageData(cxt->aic_handle, &cxt->aic_image)) {
			rtn = ISP_ERROR;
			ISP_LOGE("fail to AIC_SetImageData.");
			goto exit;
		}
		break;
	case AI_PROCESS_START:
		if (AI_STATUS_PROCESSING == cxt->aic_status) {
			ISP_LOGD("ai is still in-processing. status: %d.", cxt->aic_status);
			rtn = ISP_ERROR;
			goto exit;
		}
		if (!param) {
			ISP_LOGE("fail to set START work_mode");
			goto exit;
		}
		cmr_s32 ai_work_mode = *(cmr_s32 *)param;
		cmr_s32 aic_work_mode = 0;

		switch (ai_work_mode) {
			case AI_WORKMODE_FULL:
				aic_work_mode = AIC_WORKMODE_FULL;
				break;
			case AI_WORKMODE_PORTRAIT:
				aic_work_mode = AIC_WORKMODE_PORTRAIT;
				break;
			default:
				aic_work_mode = AIC_WORKMODE_FULL;
				break;
		}
		AIC_StartProcess(cxt->aic_handle,aic_work_mode);
		cxt->aic_status = AI_STATUS_PROCESSING;
		ISP_LOGI("AI start,work_mode %d",aic_work_mode);
		break;
	case AI_PROCESS_STOP:
		if (AI_STATUS_IDLE == cxt->aic_status) {
			ISP_LOGD("ai already idle. status: %d.", cxt->aic_status);
			rtn = ISP_ERROR;
			goto exit;
		}
		AIC_StopProcess(cxt->aic_handle);
		cxt->aic_status = AI_STATUS_IDLE;
		ISP_LOGI("AI stop.");
		break;

	default:
		ISP_LOGW("cmd is invalid. cmd: %d.", cmd);
		break;
	}

exit:
	return rtn;
}

static cmr_s32 ai_io_ctrl_direct(cmr_handle handle, cmr_s32 cmd, cmr_handle param, cmr_handle result)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct ai_ctrl_cxt *cxt = NULL;
	struct ai_img_status *ai_img_status_ptr = NULL;
	struct tg_ai_ctrl_alc_log *debug_info_result = NULL;

	//UNUSED(param);
	UNUSED(result);

	rtn = ai_check_handle(handle);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to check handle %p", handle);
		return ISP_PARAM_NULL;
	}

	cxt = (struct ai_ctrl_cxt *)handle;
	debug_info_result = (struct tg_ai_ctrl_alc_log *)result;
	pthread_mutex_lock(&cxt->data_sync_lock);
	switch (cmd) {
	case AI_GET_STATUS:
		if (!param) {
			ISP_LOGD("fail to get ai status.");
			goto exit;
		}
		memcpy(param, &cxt->aic_status, sizeof(enum ai_status));
		break;
	case AI_GET_IMG_FLAG:
		if (AI_STATUS_PROCESSING != cxt->aic_status) {
			ISP_LOGD("ai get image param doesn't work. status: %d.", cxt->aic_status);
			rtn = ISP_ERROR;
			goto exit;
		}
		if (!param) {
			ISP_LOGW("fail to get ai img status.");
			rtn = ISP_ERROR;
			goto exit;
		}
		ai_img_status_ptr = (struct ai_img_status *)param;
		memcpy(&cxt->aic_img_status, param, sizeof(struct ai_img_status));
		if (0 != AIC_CheckFrameStatus(cxt->aic_handle, &cxt->aic_img_status)) {
			rtn = ISP_ERROR;
			ISP_LOGE("fail to AIC_CheckFrameStatus.");
			goto exit;
		}
		switch (cxt->aic_img_status.img_flag) {
			case AIC_IMAGE_DATA_NOT_REQUIRED:
				ai_img_status_ptr->img_flag = IMAGE_DATA_NOT_REQUIRED;
				break;
			case AIC_IMAGE_DATA_REQUIRED:
				ai_img_status_ptr->img_flag = IMAGE_DATA_REQUIRED;
				break;
			default:
				ISP_LOGW("status is invalid. status: %d.", cxt->aic_img_status.img_flag);
				ai_img_status_ptr->img_flag = IMAGE_DATA_NOT_REQUIRED;
				break;
		}
		ISP_LOGV("img_flag is: %d", ai_img_status_ptr->img_flag);
		break;
	case AI_GET_DEBUG_INFO:
		ISP_LOGD("AI_GET_DEBUG_INFO, scence id:%d", cxt->scene_info.cur_scene_id);
		debug_info_result->log =(cmr_u8 *)(&(cxt->scene_info));
		debug_info_result->size = sizeof(struct ai_scene_detect_info);

	default:
		rtn = ISP_ERROR;
		break;
	}
exit:
	pthread_mutex_unlock(&cxt->data_sync_lock);
	return rtn;
}

cmr_s32 ai_sprd_adpt_io_ctrl(cmr_handle handle, cmr_s32 cmd, cmr_handle param, cmr_handle result)
{
	cmr_s32 rtn = ISP_SUCCESS;

	rtn = ai_check_handle(handle);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to check handle %p", handle);
		return ISP_ERROR;
	}

	if ((AI_SYNC_MSG_BEGIN < cmd) && (AI_SYNC_MSG_END > cmd)) {
		rtn = ai_io_ctrl_sync(handle, cmd, param, result);
	} else if ((AI_DIRECT_MSG_BEGIN < cmd) && (AI_DIRECT_MSG_END > cmd)) {
		rtn = ai_io_ctrl_direct(handle, cmd, param, result);
	} else {
		ISP_LOGE("fail to find cmd %d", cmd);
		return ISP_ERROR;
	}

	return rtn;
}

cmr_s32 ai_sprd_adpt_deinit(cmr_handle handle, cmr_handle in_param, cmr_handle out_param)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct ai_ctrl_cxt *cxt = NULL;
	UNUSED(in_param);
	UNUSED(out_param);

	ISP_LOGI("enter");
	rtn = ai_check_handle(handle);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGW("ai_check_handle fail");
		return ISP_ERROR;
	}
	cxt = (struct ai_ctrl_cxt *)handle;

	AIC_DeleteHandle(&cxt->aic_handle);

	cxt->aic_status = AI_STATUS_IDLE;
	pthread_mutex_destroy(&cxt->data_sync_lock);

	free(cxt);
	cxt = NULL;

	ISP_LOGI("done");
	return rtn;
}

cmr_s32 ai_sprd_adpt_process(cmr_handle handle, cmr_handle param, cmr_handle result)
{
	cmr_s32 rtn = ISP_SUCCESS;
	UNUSED(param);
	UNUSED(result);

	rtn = ai_check_handle(handle);
	if (ISP_SUCCESS != rtn) {
		return ISP_ERROR;
	}

	return 0;
}

struct adpt_ops_type sprd_ai_adpt_ops = {
	.adpt_init = ai_sprd_adpt_init,
	.adpt_deinit = ai_sprd_adpt_deinit,
	.adpt_process = ai_sprd_adpt_process,
	.adpt_ioctrl = ai_sprd_adpt_io_ctrl,
};
