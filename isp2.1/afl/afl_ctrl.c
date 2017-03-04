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

#include "afl_ctrl.h"
#include "deflicker.h"
#include <cutils/properties.h>
#include <math.h>
#include "isp_dev_access.h"

#define ISP_AFL_BUFFER_LEN                   (3120 * 4 * 61)
#define ISP_SET_AFL_THR                      "isp.afl.thr"

#define AFLCTRL_EVT_BASE            0x2000
#define AFLCTRL_EVT_INIT            AFLCTRL_EVT_BASE
#define AFLCTRL_EVT_DEINIT          (AFLCTRL_EVT_BASE + 1)
#define AFLCTRL_EVT_IOCTRL          (AFLCTRL_EVT_BASE + 2)
#define AFLCTRL_EVT_PROCESS         (AFLCTRL_EVT_BASE + 3)
#define AFLCTRL_EVT_EXIT            (AFLCTRL_EVT_BASE + 4)

static int32_t _set_afl_thr(cmr_handle handle, int *thr)
{
#ifdef WIN32
	return ISP_SUCCESS;
#else
	int rtn = ISP_SUCCESS;
	uint32_t temp = 0;
	char temp_thr[4] = {0};
	int i = 0, j = 0;

	if(NULL == handle){
		ISP_LOGE(" _is_isp_reg_log param error ");
		return -1;
	}
	char value[PROPERTY_VALUE_MAX];

	property_get(ISP_SET_AFL_THR, value, "/dev/close_afl");
	ISP_LOGI("_set_afl_thr:%s", value);

	if (strcmp(value, "/dev/close_afl")) {
		for(i = 0; i < 9; i++) {
			for(j = 0; j < 3; j++){
				temp_thr[j] = value[3*i + j];
			}
			temp_thr[j] = '\0';
			temp = atoi(temp_thr);
			*thr++ = temp;

			if(0 == temp) {
				rtn = -1;
				ISP_LOGE("ERR:Invalid anti_flicker threshold param!");
				break;
			}
		}
	} else {
		rtn = -1;
		return rtn;
	}
#endif

	return rtn;
}

/*************This code just use for anti-flicker debug*******************/
#if 1
static FILE *fp = NULL;
static int32_t cnt = 0;

cmr_int afl_statistc_file_open()
{
	char                            file_name[40] = {0};
	char                            tmp_str[10];

	strcpy(file_name, CAMERA_DUMP_PATH);
	sprintf(tmp_str, "%d", cnt);
	strcat(file_name, tmp_str);

	strcat(file_name, "_afl");
	ISP_LOGE("file name %s", file_name);
	fp = fopen(file_name, "w+");
	if (NULL == fp) {
		ISP_LOGI("can not open file: %s \n", file_name);
		return -1;
	}

	return 0;
}

static int32_t afl_statistic_save_to_file(int32_t height, int32_t *addr, FILE *fp_ptr)
{
	int32_t                         i = 0;
	int32_t                         *ptr = addr;
	FILE                            *fp = fp_ptr;

	ISP_LOGI("addr %p line num %d", ptr, height);

	for (i = 0; i < height; i++) {
		fprintf(fp, "%d\n", *ptr);
		ptr++;
	}

	return 0;
}

void afl_statistic_file_close(FILE *fp)
{
	if (NULL != fp) {
		fclose(fp);
	}
}
#endif

static cmr_int aflctrl_process(struct isp_anti_flicker_cfg *cxt_ptr, struct afl_proc_in *in_ptr, struct afl_ctrl_proc_out *out_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	int32_t thr[9] = {0,0,0,0,0,0,0,0,0};
	struct isp_awb_statistic_info   *ae_stat_ptr = in_ptr->ae_stat_ptr;
	uint32_t cur_flicker = in_ptr->cur_flicker;
	uint32_t cur_exp_flag = in_ptr->cur_exp_flag;
	int32_t ae_exp_flag = in_ptr->ae_exp_flag;
	uint32_t nxt_flicker = 0;
	cmr_int debug_index = 0;
	uint32_t i = 0;
	cmr_int flag = 0;
	int32_t *addr = NULL;
	addr = (int32_t *)in_ptr->vir_addr;
	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param is NULL error!");
		goto exit;
	}

	if(cur_exp_flag) {
		if(cur_flicker) {
			//rtn = _set_afl_thr(NULL, thr);
			ISP_LOGV("%d %d %d %d %d %d %d %d %d", thr[0], thr[1],
				 thr[2], thr[3], thr[4], thr[5], thr[6], thr[7], thr[8]);
			ISP_LOGV("60Hz setting working");
			thr[0] = 200;
			thr[1] = 20;
			thr[2] = 160;
			thr[3] = (ae_exp_flag==1) ? 100 : 240;
			thr[4] = 100;
			thr[5] = 4;
			thr[6] = 30;
			thr[7] = 20;
			thr[8] = 120;
		} else {
			//rtn = _set_afl_thr(NULL, thr);
			ISP_LOGV("%d %d %d %d %d %d %d %d %d", thr[0], thr[1],
				 thr[2], thr[3], thr[4], thr[5], thr[6], thr[7], thr[8]);
			ISP_LOGV("50Hz setting working");
			thr[0] = 200;
			thr[1] = 20;
			thr[2] = 160;
			thr[3] = 200;
			thr[3] = (ae_exp_flag==1) ? 100 : 280;
			thr[4] = 100;
			thr[5] = 4;
			thr[6] = 30;
			thr[7] = 20;
			thr[8] = 120;
		}

		for(i = 0;i < cxt_ptr->frame_num;i++) {
			if(cur_flicker) {
				flag = antiflcker_sw_process(cxt_ptr->width,
					cxt_ptr->height, addr, 0, thr[0], thr[1],
					thr[2], thr[3], thr[4], thr[5], thr[6],
					thr[7], thr[8], (int *)ae_stat_ptr->r_info,
					(int *)ae_stat_ptr->g_info,
					(int *)ae_stat_ptr->b_info);
				ISP_LOGV("flag %ld %s", flag, "60Hz");
			} else {
				flag = antiflcker_sw_process(cxt_ptr->width,
					cxt_ptr->height, addr, 1, thr[0], thr[1],
					thr[2], thr[3], thr[4], thr[5], thr[6],
					thr[7], thr[8], (int *)ae_stat_ptr->r_info,
					(int *)ae_stat_ptr->g_info,
					(int *)ae_stat_ptr->b_info);
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
	ISP_LOGI("done %ld", rtn);
	return rtn;
}

static cmr_int aflctrl_ctrl_thr_proc(struct cmr_msg *message, void *p_data)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_anti_flicker_cfg *cxt_ptr = (struct isp_anti_flicker_cfg *)p_data;

	if (!message || !p_data) {
		ISP_LOGE("param error");
		goto exit;
	}
	ISP_LOGI("message.msg_type 0x%x, data %p", message->msg_type,
		 message->data);

	switch (message->msg_type) {
	case AFLCTRL_EVT_PROCESS:
		rtn = aflctrl_process(cxt_ptr, (struct afl_proc_in*)message->data, &cxt_ptr->proc_out);
		break;
	default:
		ISP_LOGE("don't support msg");
		break;
	}

exit:
	ISP_LOGI("done %ld", rtn);
	return rtn;
}

static cmr_int aflctrl_create_thread(struct isp_anti_flicker_cfg *cxt_ptr)
{
	cmr_int rtn = ISP_SUCCESS;

	rtn = cmr_thread_create(&cxt_ptr->thr_handle, ISP_THREAD_QUEUE_NUM, aflctrl_ctrl_thr_proc, (void*)cxt_ptr);
	if (rtn) {
		ISP_LOGE("create ctrl thread error");
		rtn = ISP_ERROR;
	}

	ISP_LOGI("LiuY:afl_ctrl thread rtn %ld", rtn);
	return rtn;
}

cmr_int afl_ctrl_init(cmr_handle *isp_afl_handle, struct afl_ctrl_init_in *input_ptr)
{
	cmr_int                         rtn = ISP_SUCCESS;
	struct isp_anti_flicker_cfg     *cxt_ptr = NULL;
	void                            *afl_buf_ptr = NULL;

	if (!input_ptr || !isp_afl_handle) {
		rtn = ISP_PARAM_NULL;
		goto exit;
	}
	*isp_afl_handle = NULL;
#if 1/*Solve compile problem*/
	rtn = antiflcker_sw_init();
#endif
	if (rtn) {
		ISP_LOGE("AFL_TAG:antiflcker_sw_init failed");
		return ISP_ERROR;
	}

	cxt_ptr = (struct isp_anti_flicker_cfg*)malloc(sizeof(struct isp_anti_flicker_cfg));
	if (NULL == cxt_ptr){
		ISP_LOGE("AFL_TAG:malloc failed");
		return ISP_ERROR;
	}
	memset((void *)cxt_ptr, 0x00, sizeof(*cxt_ptr));

	afl_buf_ptr = (void*)malloc(ISP_AFL_BUFFER_LEN); //(handle->src.h * 4 * 16);//max frame_num 15
	if (NULL == afl_buf_ptr) {
		ISP_LOGE("AFL_TAG:malloc failed");
		free(cxt_ptr);
		return ISP_ERROR;
	}

	cxt_ptr->bypass = 0;
	cxt_ptr->skip_frame_num = 1;
	cxt_ptr->mode = 0;
	cxt_ptr->line_step = 0;
	cxt_ptr->frame_num = 3;//1~15
	cxt_ptr->vheight = input_ptr->size.h;
	//cxt_ptr->vwidth = input_ptr->size.w;
	cxt_ptr->start_col = input_ptr->size.w;
	cxt_ptr->end_col = input_ptr->size.w - 1;
	cxt_ptr->addr = afl_buf_ptr;
	cxt_ptr->dev_handle = input_ptr->dev_handle;
	cxt_ptr->vir_addr = (cmr_int)input_ptr->vir_addr;

	rtn = aflctrl_create_thread(cxt_ptr);

exit:
	if (rtn) {
		if (cxt_ptr) {
			if (cxt_ptr->addr) {
				free(cxt_ptr->addr);
				cxt_ptr->addr = NULL;
			}
			free((void*)cxt_ptr);
			cxt_ptr = NULL;
		}
	} else {
		*isp_afl_handle = (void *)cxt_ptr;
	}

	ISP_LOGI("LiuY: done %ld", rtn);
	return rtn;
}

cmr_int afl_ctrl_cfg(isp_handle isp_afl_handle)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_anti_flicker_cfg *cxt_ptr = (struct isp_anti_flicker_cfg*)isp_afl_handle;;
	//struct isp_dev_anti_flicker_info_v1 afl_info;
	struct isp_dev_anti_flicker_info afl_info;
#if 1

	afl_info.bypass         = 0;
	afl_info.skip_frame_num = 2;
	afl_info.mode           = 0;
	afl_info.line_step      = 2;
	afl_info.frame_num      = 3;
	afl_info.start_col      = 0;
	afl_info.end_col        = cxt_ptr->width - 1;
	afl_info.vheight        = cxt_ptr->height;
	//afl_info.afl_total_num =
	afl_info.img_size.height = cxt_ptr->height;
	afl_info.img_size.width = cxt_ptr->width;
	rtn = isp_dev_access_ioctl(cxt_ptr->dev_handle, ISP_DEV_SET_AFL_BLOCK, &afl_info, NULL);
#endif
#if 0
	afl_info.bypass         = 0;
	afl_info.skip_frame_num = 3;
	afl_info.mode           = 1;
	//afl_info.line_step      = cxt_ptr->line_step;
	afl_info.afl_stepx      = 0xA0000;
	afl_info.afl_stepy      = 0x100000;
	afl_info.frame_num      = 0x3;
	afl_info.start_col      = 0;
	afl_info.end_col      = 640;
	afl_info.step_x_region      = 0x280000;
	afl_info.step_y_region      = 0x1E0000;
	afl_info.step_x_start_region      = 0;
	afl_info.step_x_end_region      = 640;
	afl_info.step_x_end_region      = 640;
	afl_info.img_size.height = cxt_ptr->height;
	afl_info.img_size.width = cxt_ptr->width;
	rtn = isp_dev_access_ioctl(cxt_ptr->dev_handle, ISP_DEV_SET_AFL_NEW_BLOCK, &afl_info, NULL);
#endif

	ISP_LOGI("LiuY: done %ld", rtn);
	return rtn;
}

static cmr_int aflctrl_destroy_thread(struct isp_anti_flicker_cfg *cxt)
{
	cmr_int rtn = ISP_SUCCESS;

	if (!cxt) {
		ISP_LOGE("in parm error");
		rtn = ISP_ERROR;
		goto exit;
	}

	if (cxt->thr_handle) {
		rtn = cmr_thread_destroy(cxt->thr_handle);
		if (!rtn) {
			cxt->thr_handle = NULL;
		} else {
			ISP_LOGE("failed to destroy ctrl thread %ld", rtn);
		}
	}
exit:
	ISP_LOGI("done %ld", rtn);
	return rtn;
}

cmr_int afl_ctrl_deinit(cmr_handle isp_afl_handle)
{
	cmr_int                         rtn = ISP_SUCCESS;
	struct isp_anti_flicker_cfg     *cxt_ptr = (struct isp_anti_flicker_cfg*)isp_afl_handle;
	cmr_int bypass = 1;

	ISP_CHECK_HANDLE_VALID(isp_afl_handle);

	isp_dev_anti_flicker_bypass(cxt_ptr->dev_handle, bypass);

	rtn = aflctrl_destroy_thread(cxt_ptr);

	if (cxt_ptr) {
		if (cxt_ptr->addr) {
			free(cxt_ptr->addr);
			cxt_ptr->addr = NULL;
		}
		free((void*)cxt_ptr);
		cxt_ptr = NULL;
	}
#if 1/*Solve compile problem*/
	rtn = antiflcker_sw_deinit();
#endif
	if (rtn) {
		ISP_LOGE("AFL_TAG:antiflcker_sw_deinit error");
		return ISP_ERROR;
	}

	ISP_LOGI("done %ld", rtn);
	return rtn;
}

cmr_int afl_ctrl_process(cmr_handle isp_afl_handle, struct afl_proc_in *in_ptr, struct afl_ctrl_proc_out *out_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_anti_flicker_cfg     *cxt_ptr = (struct isp_anti_flicker_cfg*)isp_afl_handle;
	if (!isp_afl_handle || !in_ptr) {
		rtn = ISP_PARAM_ERROR;
		goto exit;
	}
	ISP_LOGI("LiuY: begin %ld", rtn);

	CMR_MSG_INIT(message);
	ISP_LOGI("LiuY: begin %ld", rtn);
	message.data = malloc(sizeof(*in_ptr));
	if (!message.data) {
		ISP_LOGE("failed to malloc msg");
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
	ISP_LOGI("LiuY: done %ld", rtn);
	return rtn;
}
