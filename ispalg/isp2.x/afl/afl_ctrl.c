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
#define LOG_TAG "afl_ctrl"
#include "afl_ctrl.h"
#include "deflicker.h"
#include <cutils/properties.h>
#include <math.h>

#define ISP_AFL_BUFFER_LEN                   (3120 * 4 * 61)
#define ISP_SET_AFL_THR                      "isp.afl.thr"

#define AFLCTRL_EVT_BASE            0x2000
#define AFLCTRL_EVT_INIT            AFLCTRL_EVT_BASE
#define AFLCTRL_EVT_DEINIT          (AFLCTRL_EVT_BASE + 1)
#define AFLCTRL_EVT_IOCTRL          (AFLCTRL_EVT_BASE + 2)
#define AFLCTRL_EVT_PROCESS         (AFLCTRL_EVT_BASE + 3)
#define AFLCTRL_EVT_EXIT            (AFLCTRL_EVT_BASE + 4)

static cmr_s32 cnt = 0;
static cmr_s32 _set_afl_thr(cmr_s32 * thr)
{
#ifdef WIN32
	return -1;
#else
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 temp = 0;
	char temp_thr[4] = { 0 };
	cmr_s32 i = 0, j = 0;
	char value[PROPERTY_VALUE_MAX];

	if (NULL == thr) {
		ISP_LOGE("fail to check thr,thr is NULL!");
		return -1;
	}

	property_get(ISP_SET_AFL_THR, value, "/dev/close_afl");
	ISP_LOGV(" _set_afl_thr:%s", value);

	if (strcmp(value, "/dev/close_afl")) {
		for (i = 0; i < 9; i++) {
			for (j = 0; j < 3; j++) {
				temp_thr[j] = value[3 * i + j];
			}
			temp_thr[j] = '\0';
			ISP_LOGV("temp_thr:%c, %c, %c", temp_thr[0], temp_thr[1], temp_thr[2]);
			temp = atoi(temp_thr);
			*thr++ = temp;

			if (0 == temp) {
				rtn = -1;
				ISP_LOGE("fail to check temp");
				break;
			}
		}
	} else {
		rtn = -1;
		return rtn;
	}
	return rtn;
#endif
}

static cmr_s32 afl_statistic_save_to_file(cmr_s32 height, cmr_s32 * addr)
{
	cmr_s32 i = 0;
	cmr_s32 *ptr = addr;
	FILE *fp = NULL;
	char file_name[100] = { 0 };
	char tmp_str[100];

	if (NULL == addr) {
		ISP_LOGE("fail to check param,param is NULL!");
		return -1;
	}

	ISP_LOGV("addr %p line num %d", ptr, height);

	strcpy(file_name, CAMERA_DUMP_PATH);
	sprintf(tmp_str, "%d", cnt);
	strcat(file_name, tmp_str);
	strcat(file_name, "_afl_statistic.txt");
	ISP_LOGV("file name %s", file_name);

	//fp = fopen(file_name, "wb");
	fp = fopen(file_name, "w+");
	if (NULL == fp) {
		ISP_LOGE("fail to open file: %s \n", file_name);
		return -1;
	}
	//fwrite((void*)ptr, 1, height * sizeof(cmr_s32), fp);
	for (i = 0; i < height; i++) {
		fprintf(fp, "%d\n", *ptr);
		ptr++;
	}

	if (NULL != fp) {
		fclose(fp);
	}

	return 0;
}

static cmr_int aflctrl_process(struct isp_anti_flicker_cfg *cxt_ptr, struct afl_proc_in *in_ptr, struct afl_ctrl_proc_out *out_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	cmr_int ret = 0;
	cmr_s32 thr[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	struct isp_awb_statistic_info *ae_stat_ptr = NULL;
	cmr_u32 cur_flicker = 0;
	cmr_u32 cur_exp_flag = 0;
	cmr_s32 ae_exp_flag = 0;
	cmr_u32 i = 0;
	cmr_int flag = 0;
	cmr_s32 *addr = NULL;

	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("fail to check param is NULL!");
		goto exit;
	}

	ae_stat_ptr = in_ptr->ae_stat_ptr;
	cur_flicker = in_ptr->cur_flicker;
	cur_exp_flag = in_ptr->cur_exp_flag;
	ae_exp_flag = in_ptr->ae_exp_flag;
	addr = (cmr_s32 *)(cmr_uint)in_ptr->vir_addr;

	if (cur_exp_flag) {
		if (cur_flicker) {
			ret = _set_afl_thr(thr);
			if (0 == ret) {
				ISP_LOGV("%d %d %d %d %d %d %d %d %d", thr[0], thr[1], thr[2], thr[3], thr[4], thr[5], thr[6], thr[7], thr[8]);
				ISP_LOGV("60Hz setting working");
			} else {
				thr[0] = 200;
				thr[1] = 20;
				thr[2] = 160;
				thr[3] = (ae_exp_flag == 1) ? 100 : 140;//160
				thr[4] = 100;
				thr[5] = 4;
				thr[6] = 30;
				thr[7] = 20;
				thr[8] = 120;
				ISP_LOGI("60Hz using default threshold");
			}
		} else {
			ret = _set_afl_thr(thr);
			if (0 == ret) {
				ISP_LOGV("%d %d %d %d %d %d %d %d %d", thr[0], thr[1], thr[2], thr[3], thr[4], thr[5], thr[6], thr[7], thr[8]);
				ISP_LOGV("50Hz setting working");
			} else {
				thr[0] = 200;
				thr[1] = 20;
				thr[2] = 160;
				thr[3] = (ae_exp_flag == 1) ? 100 : 280;
				thr[4] = 100;
				thr[5] = 4;
				thr[6] = 30;
				thr[7] = 20;
				thr[8] = 120;
				ISP_LOGI("50Hz using default threshold");
			}
		}

		for (i = 0; i < cxt_ptr->frame_num; i++) {
			if (cur_flicker) {
				flag = antiflcker_sw_process(cxt_ptr->width,
							     cxt_ptr->height, addr, 0, thr[0], thr[1],
							     thr[2], thr[3], thr[4], thr[5], thr[6],
							     thr[7], thr[8], (cmr_s32 *) ae_stat_ptr->r_info, (cmr_s32 *) ae_stat_ptr->g_info, (cmr_s32 *) ae_stat_ptr->b_info);
				ISP_LOGV("flag %ld %s", flag, "60Hz");
			} else {
				flag = antiflcker_sw_process(cxt_ptr->width,
							     cxt_ptr->height, addr, 1, thr[0], thr[1],
							     thr[2], thr[3], thr[4], thr[5], thr[6],
							     thr[7], thr[8], (cmr_s32 *) ae_stat_ptr->r_info, (cmr_s32 *) ae_stat_ptr->g_info, (cmr_s32 *) ae_stat_ptr->b_info);
				ISP_LOGV("flag %ld %s", flag, "50Hz");
			}
			if (flag)
				break;

			addr += cxt_ptr->vheight;
		}
	}

	out_ptr->flag = flag;
	out_ptr->cur_flicker = cur_flicker;

exit:
	ISP_LOGV("done %ld", rtn);
	return rtn;
}

static cmr_int aflctrl_ctrl_thr_proc(struct cmr_msg *message, void *p_data)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_anti_flicker_cfg *cxt_ptr = (struct isp_anti_flicker_cfg *)p_data;

	if (!message || !p_data) {
		ISP_LOGE("fail to check param");
		goto exit;
	}
	ISP_LOGV("message.msg_type 0x%x, data %p", message->msg_type, message->data);

	switch (message->msg_type) {
	case AFLCTRL_EVT_PROCESS:
		rtn = aflctrl_process(cxt_ptr, (struct afl_proc_in *)message->data, &cxt_ptr->proc_out);
		break;
	default:
		ISP_LOGE("fail to proc,don't support msg");
		break;
	}

exit:
	ISP_LOGV("done %ld", rtn);
	return rtn;
}

static cmr_int aflctrl_create_thread(struct isp_anti_flicker_cfg *cxt_ptr)
{
	cmr_int rtn = ISP_SUCCESS;

	rtn = cmr_thread_create(&cxt_ptr->thr_handle, ISP_THREAD_QUEUE_NUM, aflctrl_ctrl_thr_proc, (void *)cxt_ptr);
	if (rtn) {
		ISP_LOGE("fail to create ctrl thread ");
		rtn = ISP_ERROR;
	}

	ISP_LOGI("afl_ctrl thread rtn %ld", rtn);
	return rtn;
}

cmr_int afl_ctrl_init(cmr_handle * isp_afl_handle, struct afl_ctrl_init_in * input_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_anti_flicker_cfg *cxt_ptr = NULL;
	if (!input_ptr || !isp_afl_handle) {
		rtn = ISP_PARAM_NULL;
		goto exit;
	}
	*isp_afl_handle = NULL;
	rtn = antiflcker_sw_init();
	if (rtn) {
		ISP_LOGE("fail to do antiflcker_sw_init");
		return ISP_ERROR;
	}

	cxt_ptr = (struct isp_anti_flicker_cfg *)malloc(sizeof(struct isp_anti_flicker_cfg));
	if (NULL == cxt_ptr) {
		ISP_LOGE("fail to do:malloc");
		return ISP_ERROR;
	}
	memset((void *)cxt_ptr, 0x00, sizeof(*cxt_ptr));

	cxt_ptr->bypass = 0;
	cxt_ptr->skip_frame_num = 1;
	cxt_ptr->mode = 0;
	cxt_ptr->line_step = 0;
	cxt_ptr->frame_num = 3;	//1~15
	cxt_ptr->vheight = input_ptr->size.h;
	cxt_ptr->start_col = 0;
	cxt_ptr->end_col = input_ptr->size.w - 1;
	cxt_ptr->dev_handle = input_ptr->dev_handle;
	cxt_ptr->vir_addr = (cmr_int) input_ptr->vir_addr;
	cxt_ptr->afl_set_cb = input_ptr->afl_set_cb;
	cxt_ptr->caller_handle = input_ptr->caller_handle;
	rtn = aflctrl_create_thread(cxt_ptr);
exit:
	if (rtn) {
		if (cxt_ptr) {
			free((void *)cxt_ptr);
			cxt_ptr = NULL;
		}
	} else {
		*isp_afl_handle = (void *)cxt_ptr;
	}
	ISP_LOGI("done %ld", rtn);
	return rtn;
}

cmr_int afl_ctrl_cfg(isp_handle isp_afl_handle)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_anti_flicker_cfg *cxt_ptr = (struct isp_anti_flicker_cfg *)isp_afl_handle;;
	struct isp_dev_anti_flicker_info afl_info;
	cxt_ptr->vheight = cxt_ptr->height;
	cxt_ptr->start_col = 0;
	cxt_ptr->end_col = cxt_ptr->width - 1;

	afl_info.bypass = 0;
	afl_info.skip_frame_num = 0;
	afl_info.mode = 0;
	afl_info.line_step = 0;
	afl_info.frame_num = 3;
	afl_info.start_col = 0;
	afl_info.end_col = cxt_ptr->width - 1;
	afl_info.vheight = cxt_ptr->height;
	afl_info.img_size.height = cxt_ptr->height;
	afl_info.img_size.width = cxt_ptr->width;
	if (cxt_ptr->afl_set_cb) {
		cxt_ptr->afl_set_cb(cxt_ptr->caller_handle, ISP_AFL_SET_CFG_PARAM, &afl_info, NULL);
	}
	ISP_LOGI("done %ld", rtn);
	return rtn;
}

cmr_int aflnew_ctrl_cfg(isp_handle isp_afl_handle)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_anti_flicker_cfg *cxt_ptr = (struct isp_anti_flicker_cfg *)isp_afl_handle;
	struct isp_dev_anti_flicker_new_info afl_info_v3;

	cxt_ptr->vheight = cxt_ptr->height;
	cxt_ptr->start_col = 0;
	cxt_ptr->end_col = cxt_ptr->width - 1;

	afl_info_v3.bypass = cxt_ptr->bypass;
	afl_info_v3.mode = cxt_ptr->mode;
	afl_info_v3.skip_frame_num = cxt_ptr->skip_frame_num;
	afl_info_v3.afl_stepx = 0xA0000;
	afl_info_v3.afl_stepy = 0x100000;
	afl_info_v3.frame_num = cxt_ptr->frame_num;
	afl_info_v3.start_col = cxt_ptr->start_col;
	afl_info_v3.end_col = cxt_ptr->end_col;
	afl_info_v3.step_x_region = 0x280000;
	afl_info_v3.step_y_region = 0x1E0000;
	afl_info_v3.step_x_start_region = 0;
	afl_info_v3.step_x_end_region = 640;

	afl_info_v3.img_size.width = cxt_ptr->width;
	afl_info_v3.img_size.height = cxt_ptr->height;

	if (cxt_ptr->afl_set_cb) {
		cxt_ptr->afl_set_cb(cxt_ptr->caller_handle, ISP_AFL_NEW_SET_CFG_PARAM, &afl_info_v3, NULL);
	}

	ISP_LOGI("done %ld", rtn);
	return rtn;
}

static cmr_int aflctrl_destroy_thread(struct isp_anti_flicker_cfg *cxt)
{
	cmr_int rtn = ISP_SUCCESS;

	if (!cxt) {
		ISP_LOGE("fail to check param, in parm is NULL");
		rtn = ISP_ERROR;
		goto exit;
	}

	if (cxt->thr_handle) {
		rtn = cmr_thread_destroy(cxt->thr_handle);
		if (!rtn) {
			cxt->thr_handle = NULL;
		} else {
			ISP_LOGE("fail to destroy ctrl thread %ld", rtn);
		}
	}
exit:
	ISP_LOGI("done %ld", rtn);
	return rtn;
}

cmr_int afl_ctrl_deinit(cmr_handle * isp_afl_handle)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_anti_flicker_cfg *cxt_ptr = *isp_afl_handle;
	cmr_int bypass = 1;
	CMR_MSG_INIT(message);

	ISP_CHECK_HANDLE_VALID(isp_afl_handle);
	if (NULL == cxt_ptr) {
		ISP_LOGE("fail to check param, in parm is NULL");
		rtn = ISP_ERROR;
		return rtn;
	}

	message.msg_type = AFLCTRL_EVT_DEINIT;
	message.sync_flag = CMR_MSG_SYNC_PROCESSED;
	message.alloc_flag = 0;
	message.data = NULL;
	rtn = cmr_thread_msg_send(cxt_ptr->thr_handle, &message);
	if (rtn) {
		ISP_LOGE("fail to send msg to main thr %ld", rtn);
		goto exit;
	}

	if (NULL == cxt_ptr->dev_handle) {
		ISP_LOGE("fail to check param, cxt_ptr->dev_handle is NULL");
		rtn = ISP_ERROR;
		return rtn;
	}
	if (cxt_ptr->afl_set_cb) {
#ifdef ANTI_FLICKER_INFO_VERSION_NEW
		cxt_ptr->afl_set_cb(cxt_ptr->caller_handle, ISP_AFL_NEW_SET_BYPASS, &bypass, NULL);
#else
		cxt_ptr->afl_set_cb(cxt_ptr->caller_handle, ISP_AFL_SET_BYPASS, &bypass, NULL);
#endif
	}
	rtn = aflctrl_destroy_thread(cxt_ptr);
	if (rtn) {
		ISP_LOGE("fail to destroy aflctrl thread.");
		rtn = antiflcker_sw_deinit();
		if (rtn)
			ISP_LOGE("fail to do antiflcker deinit.");
		goto exit;
	}

	rtn = antiflcker_sw_deinit();
	if (rtn) {
		ISP_LOGE("fail to do antiflcker deinit.");
		return ISP_ERROR;
	}

exit:
	if (NULL != cxt_ptr) {
		free((void *)cxt_ptr);
		*isp_afl_handle = NULL;
	}

	ISP_LOGI("done %ld", rtn);
	return rtn;
}

cmr_int afl_ctrl_process(cmr_handle isp_afl_handle, struct afl_proc_in * in_ptr, struct afl_ctrl_proc_out * out_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_anti_flicker_cfg *cxt_ptr = (struct isp_anti_flicker_cfg *)isp_afl_handle;
	if (!isp_afl_handle || !in_ptr) {
		rtn = ISP_PARAM_ERROR;
		goto exit;
	}
	ISP_LOGV("begin %ld", rtn);

	CMR_MSG_INIT(message);
	message.data = malloc(sizeof(*in_ptr));
	if (!message.data) {
		ISP_LOGE("fail to malloc msg");
		rtn = ISP_ALLOC_ERROR;
		goto exit;
	}
	memcpy(message.data, in_ptr, sizeof(*in_ptr));
	message.alloc_flag = 1;

	message.msg_type = AFLCTRL_EVT_PROCESS;
	message.sync_flag = CMR_MSG_SYNC_PROCESSED;
	rtn = cmr_thread_msg_send(cxt_ptr->thr_handle, &message);
	if (out_ptr) {
		*out_ptr = cxt_ptr->proc_out;
	}
exit:
	ISP_LOGV("done %ld", rtn);
	return rtn;
}
