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
#define LOG_TAG "ae_ctrl"

#include "ae_ctrl.h"
#include <cutils/properties.h>
#include <string.h>

#define AECTRL_EVT_BASE            0x2000
#define AECTRL_EVT_INIT            AECTRL_EVT_BASE
#define AECTRL_EVT_DEINIT          (AECTRL_EVT_BASE + 1)
#define AECTRL_EVT_IOCTRL          (AECTRL_EVT_BASE + 2)
#define AECTRL_EVT_PROCESS         (AECTRL_EVT_BASE + 3)
#define AECTRL_EVT_EXIT            (AECTRL_EVT_BASE + 4)


struct ae_ctrl_msg_ctrl {
	cmr_int cmd;
	cmr_handle in;
	cmr_handle out;
};

static cmr_s32 ae_ex_set_exposure(cmr_handle handler, struct ae_exposure *in_param)
{
	struct aectrl_cxt *cxt_ptr = (struct aectrl_cxt *)handler;

	if (cxt_ptr->ae_set_cb) {
		cxt_ptr->ae_set_cb(cxt_ptr->caller_handle, ISP_AE_EX_SET_EXPOSURE, in_param, NULL);
	}

	return 0;
}

static cmr_s32 ae_set_exposure(cmr_handle handler, struct ae_exposure *in_param)
{
	struct aectrl_cxt *cxt_ptr = (struct aectrl_cxt *)handler;

	if (cxt_ptr->ae_set_cb) {
		cxt_ptr->ae_set_cb(cxt_ptr->caller_handle, ISP_AE_SET_EXPOSURE, &in_param->exposure, NULL);
	}

	return 0;
}

static cmr_s32 ae_set_blk_num(cmr_handle handler, struct ae_size *blk_num)
{
#if defined(CONFIG_ISP_2_5) || defined(CONFIG_ISP_2_6) || defined(CONFIG_ISP_2_7)
	struct aectrl_cxt *cxt_ptr = (struct aectrl_cxt *)handler;
	if (cxt_ptr->ae_set_cb) {
		cxt_ptr->ae_set_cb(cxt_ptr->caller_handle, ISP_AE_SET_BLK_NUM, blk_num, NULL);
	}
#else
	UNUSED(handler);
	UNUSED(blk_num);
#endif
	return 0;
}

static cmr_s32 ae_set_again(cmr_handle handler, struct ae_gain *in_param)
{
	struct aectrl_cxt *cxt_ptr = (struct aectrl_cxt *)handler;

	if (cxt_ptr->ae_set_cb) {
		cxt_ptr->ae_set_cb(cxt_ptr->caller_handle, ISP_AE_SET_GAIN, &in_param->gain, NULL);
	}

	return 0;
}

static cmr_int ae_write_multi_ae(cmr_handle handler, cmr_handle dualsnyc_ptr)
{
	struct aectrl_cxt *cxt_ptr = (struct aectrl_cxt *)handler;

	if (cxt_ptr->ae_set_cb) {
		cxt_ptr->ae_set_cb(cxt_ptr->caller_handle, ISP_AE_MULTI_WRITE, dualsnyc_ptr, NULL);
	}
	ISP_LOGV("AE set dual camear sync multi ae");
	return 0;
}

static cmr_int ae_bokeh_hdr_cb(cmr_handle handler, cmr_handle in_param)
{
#if defined(CONFIG_ISP_2_6) || defined(CONFIG_ISP_2_7)
	struct aectrl_cxt *cxt_ptr = (struct aectrl_cxt *)handler;
	if (cxt_ptr->ae_set_cb) {
		cxt_ptr->ae_set_cb(cxt_ptr->caller_handle, ISP_AE_HDR_BOKEH, in_param, NULL);
	}
#else
	UNUSED(handler);
	UNUSED(in_param);
#endif
	ISP_LOGV("AE set dual camear sync ae hdr");
	return 0;
}

static cmr_s32 ae_set_monitor(cmr_handle handler, cmr_u32 skip_number)
{
	struct aectrl_cxt *cxt_ptr = (struct aectrl_cxt *)handler;

	if (cxt_ptr->ae_set_cb) {
		cxt_ptr->ae_set_cb(cxt_ptr->caller_handle, ISP_AE_SET_MONITOR, &skip_number, NULL);
	}

	return 0;
}

static cmr_s32 ae_set_monitor_win(cmr_handle handler, struct ae_monitor_info *in_param)
{
	struct aectrl_cxt *cxt_ptr = (struct aectrl_cxt *)handler;

	if (cxt_ptr->ae_set_cb) {
		cxt_ptr->ae_set_cb(cxt_ptr->caller_handle, ISP_AE_SET_MONITOR_WIN, in_param, NULL);
	}

	return 0;
}

static cmr_s32 ae_set_rgb_thrd(cmr_handle handler, struct ae_monitor_info *in_param)
{

	struct aectrl_cxt *cxt_ptr = (struct aectrl_cxt *)handler;
	if (cxt_ptr->ae_set_cb) {
		cxt_ptr->ae_set_cb(cxt_ptr->caller_handle, ISP_AE_SET_RGB_THRD, in_param, NULL);
	}

	return 0;
}

static cmr_s32 ae_set_monitor_bypass(cmr_handle handler, cmr_u32 is_bypass)
{
	struct aectrl_cxt *cxt_ptr = (struct aectrl_cxt *)handler;

	if (cxt_ptr->ae_set_cb) {
		cxt_ptr->ae_set_cb(cxt_ptr->caller_handle, ISP_AE_SET_MONITOR_BYPASS, &is_bypass, NULL);
	}

	return 0;
}

static cmr_s32 ae_set_stats_monitor(cmr_handle handler, struct ae_stats_monitor_cfg *in_param)
{
	struct aectrl_cxt *cxt_ptr = (struct aectrl_cxt *)handler;

	if (cxt_ptr->ae_set_cb) {
		cxt_ptr->ae_set_cb(cxt_ptr->caller_handle, ISP_AE_SET_STATS_MONITOR, in_param, NULL);
	}

	return 0;
}

static cmr_s32 ae_set_bayer_hist(cmr_handle handler, struct ae_bayer_hist_cfg *in_param)
{
	struct aectrl_cxt *cxt_ptr = (struct aectrl_cxt *)handler;
	if (cxt_ptr->ae_set_cb) {
		cxt_ptr->ae_set_cb(cxt_ptr->caller_handle, ISP_AE_SET_BAYER_HIST, in_param, NULL);
	}

	return 0;
}

static cmr_s32 ae_set_aem_zoom_win(cmr_handle handler, struct ae_monitor_info *in_param)
{
	struct aectrl_cxt *cxt_ptr = (struct aectrl_cxt *)handler;
	if (cxt_ptr->ae_set_cb) {
		cxt_ptr->ae_set_cb(cxt_ptr->caller_handle, ISP_AE_SET_MONITOR_WIN, in_param, NULL);
	}
	return 0;
}

static cmr_s32 ae_set_statistics_mode(cmr_handle handler, enum ae_statistics_mode mode, cmr_u32 skip_number)
{
	struct aectrl_cxt *cxt_ptr = (struct aectrl_cxt *)handler;

	if (cxt_ptr->ae_set_cb) {
		cxt_ptr->ae_set_cb(cxt_ptr->caller_handle, ISP_AE_SET_STATISTICS_MODE, &mode, &skip_number);
	}

	return 0;
}

static cmr_s32 ae_callback(cmr_handle handler, cmr_int cb_type, cmr_handle param)
{
	struct aectrl_cxt *cxt_ptr = (struct aectrl_cxt *)handler;

	if (cxt_ptr->ae_set_cb) {
		cxt_ptr->ae_set_cb(cxt_ptr->caller_handle, ISP_AE_SET_AE_CALLBACK, &cb_type, param);
	}

	return 0;
}

static cmr_s32 ae_get_system_time(cmr_handle handler, cmr_u32 * sec, cmr_u32 * usec)
{
	struct aectrl_cxt *cxt_ptr = (struct aectrl_cxt *)handler;

	if (cxt_ptr->ae_set_cb) {
		cxt_ptr->ae_set_cb(cxt_ptr->caller_handle, ISP_AE_GET_SYSTEM_TIME, sec, usec);
	}

	return 0;
}

static cmr_s32 ae_flash_get_charge(cmr_handle handler, struct ae_flash_cfg *cfg_ptr, struct ae_flash_cell *cell_ptr)
{
	cmr_s32 ret = 0;
	struct aectrl_cxt *cxt_ptr = (struct aectrl_cxt *)handler;

	if (cxt_ptr->ae_set_cb) {
		cxt_ptr->ae_set_cb(cxt_ptr->caller_handle, ISP_AE_GET_FLASH_CHARGE, cfg_ptr, cell_ptr);
	}

	return ret;
}

static cmr_s32 ae_flash_get_time(cmr_handle handler, struct ae_flash_cfg *cfg_ptr, struct ae_flash_cell *cell_ptr)
{
	cmr_s32 ret = 0;
	struct aectrl_cxt *cxt_ptr = (struct aectrl_cxt *)handler;

	if (cxt_ptr->ae_set_cb) {
		cxt_ptr->ae_set_cb(cxt_ptr->caller_handle, ISP_AE_GET_FLASH_TIME, cfg_ptr, cell_ptr);
	}

	return ret;
}

struct lcd_ct_rgb_info {
	cmr_u32 ct;
	cmr_u8 r;
	cmr_u8 g;
	cmr_u8 b;
	cmr_u8 reserved;
};

struct lcd_ct_rgb_cfg {
	struct lcd_ct_rgb_info ct_rgb_info[32];
	cmr_u32 num;
};

struct lcd_ct_rgb_cfg s_lcd_ct_rgb_cfg_0 = {
	{
		{1500, 255, 111, 0, 0},
		{2000, 255, 141, 11, 0},
		{2500, 255, 166, 69, 0},
		{3000, 255, 185, 105, 0},
		{3500, 255, 201, 135, 0},
		{4000, 255, 213, 161, 0},
		{4500, 255, 223, 184, 0}, 
		{5000, 255, 231, 204, 0},
		{5500, 255, 238, 222, 0},
		{6000, 255, 244, 237, 0},
		{6500, 255, 249, 251, 0},
		{7000, 247, 245, 255, 0},
		{7500, 238, 239, 255, 0}, 
		{8000, 229, 233, 255, 0},
		{8500, 223, 229, 255, 0},
		{9000, 217, 225, 255, 0},
	},
	16
};
struct lcd_ct_rgb_cfg s_lcd_ct_rgb_cfg_1 = {
	{
		{1500, 255, 111, 0, 0},//0
		{2000, 255, 141, 11, 0},// 1
		{2500, 255, 166, 69, 0},// 2
		{3000, 255, 185, 105, 0},// 3
		{3500, 255, 201, 135, 0},// 4
		{4000, 255, 213, 161, 0},// 5
		{4500, 255, 223, 184, 0},// 6
		{5000, 255, 231, 204, 0},// 7
		{5500, 255, 238, 222, 0},// 8
		{6000, 255, 244, 237, 0},// 9
		{6500, 255, 249, 251, 0},// 10
		{7000, 247, 245, 255, 0},// 11
		{7500, 238, 239, 255, 0},// 12
		{8000, 229, 233, 255, 0},// 13
		{8500, 223, 229, 255, 0},// 14
		{9000, 217, 225, 255, 0},// 15
	},
	16
};

//////////////////////////////////////////////////////////////////////////////
static cmr_s32 ae_flash_set_charge(cmr_handle handler, struct ae_flash_cfg *cfg_ptr, struct ae_flash_element *element_ptr)
{
	cmr_s32 ret = 0;
	struct aectrl_cxt *cxt_ptr = (struct aectrl_cxt *)handler;
	cmr_u32 multiColorLcdEn = cfg_ptr->multiColorLcdEn;
	if (cxt_ptr->ae_set_cb) {
		struct ae_flash_element flash_element;
		if (multiColorLcdEn) {
			cmr_u32 index = 0;
			cmr_u32 index2 = 0;
			cmr_u32 bg_color = 0;
			cmr_u32 max_index = 0;
			struct lcd_ct_rgb_cfg *ct_rgb_cfg_ptr = NULL;
			if (1 == cfg_ptr->led_idx) {
				ct_rgb_cfg_ptr = &s_lcd_ct_rgb_cfg_0;
				max_index = s_lcd_ct_rgb_cfg_0.num - 1;
			}else {             //(2 == cfg_ptr->led_idx)
				ct_rgb_cfg_ptr = &s_lcd_ct_rgb_cfg_1;
				max_index = s_lcd_ct_rgb_cfg_1.num - 1;
			}

			index = element_ptr->index;
			if (element_ptr->index > max_index) {
				index = max_index;
				ISP_LOGW("the index = %d is incorrect, limited to %d\n", element_ptr->index, max_index);
			} else {
				index = element_ptr->index;
			}
			index2 = index;
			{
				char prop[128] = {0};
				property_get("persist.vendor.cam.isp.ae.lcdflash_idx", prop, "");
				if (prop[0] != '\0') {
					index2 = atoi(prop);
				}
				ISP_LOGD("led; %d, idx: %d, max: %d\n", cfg_ptr->led_idx, index2, max_index);
				bg_color = (ct_rgb_cfg_ptr->ct_rgb_info[index2].r<<16)
							|(ct_rgb_cfg_ptr->ct_rgb_info[index2].g<<8)
							|(ct_rgb_cfg_ptr->ct_rgb_info[index2].b);

				ISP_LOGD("%s, index: (%d, %d), ct: %d (%d, %d, %d)\n", prop, index, index2,
							ct_rgb_cfg_ptr->ct_rgb_info[index2].ct,
							ct_rgb_cfg_ptr->ct_rgb_info[index2].r,
							ct_rgb_cfg_ptr->ct_rgb_info[index2].g,
							ct_rgb_cfg_ptr->ct_rgb_info[index2].b);
				index = index2;
			}
			flash_element.index = index;
			flash_element.val = element_ptr->val;
			//flash_element.brightness = element_ptr->brightness;
			flash_element.brightness = 255;
			flash_element.color_temp= ct_rgb_cfg_ptr->ct_rgb_info[index].ct;
			flash_element.bg_color = bg_color;
		} else {
			flash_element.val = element_ptr->val;
			flash_element.index = element_ptr->index;
			flash_element.brightness = 255;
			flash_element.bg_color = ((255 << 16) | (255 << 8) | (255 << 0));
			flash_element.color_temp= 5000;
		}
		ISP_LOGV("led_idx=%d, led_type=%d, level=%d", cfg_ptr->led_idx, cfg_ptr->type, element_ptr->index);
		ISP_LOGD("index: %d, bright: %d, ct: %d, bg_color:0x%x, multiColorLcdEn:%d\n", flash_element.index, flash_element.brightness, flash_element.color_temp, flash_element.bg_color, cfg_ptr->multiColorLcdEn);
		cxt_ptr->ae_set_cb(cxt_ptr->caller_handle, ISP_AE_SET_FLASH_CHARGE, cfg_ptr, &flash_element);
	}

	return ret;
}

static cmr_s32 ae_flash_set_time(cmr_handle handler, struct ae_flash_cfg *cfg_ptr, struct ae_flash_element *element_ptr)
{
	cmr_s32 ret = 0;
	struct aectrl_cxt *cxt_ptr = (struct aectrl_cxt *)handler;

	if (cxt_ptr->ae_set_cb) {
		cxt_ptr->ae_set_cb(cxt_ptr->caller_handle, ISP_AE_SET_FLASH_TIME, cfg_ptr, element_ptr);
	}

	return ret;
}

static cmr_s32 ae_flash_ctrl_enable(cmr_handle handler, struct ae_flash_cfg *cfg_ptr, struct ae_flash_element *element_ptr)
{
	cmr_s32 ret = 0;
	struct aectrl_cxt *cxt_ptr = (struct aectrl_cxt *)handler;

	if (cxt_ptr->ae_set_cb) {
		ISP_LOGV("led_en=%d, led_type=%d", cfg_ptr->led_idx, cfg_ptr->type);
		cxt_ptr->ae_set_cb(cxt_ptr->caller_handle, ISP_AE_FLASH_CTRL, cfg_ptr, element_ptr);
	}

	return ret;
}

static cmr_s32 __ae_set_rgb_gain(cmr_handle handler, double gain, cmr_u32 flag)
{
	cmr_int rtn = ISP_SUCCESS;
	cmr_u32 final_gain = 0;
	cmr_u32 rgb_gain_offset = 4096;
	struct isp_rgb_gain_info gain_info;
	struct aectrl_cxt *cxt_ptr = (struct aectrl_cxt *)handler;

	memset(&gain_info, 0x00, sizeof(gain_info));

	if (cxt_ptr->ae_set_cb) {
		final_gain = (cmr_u32) (gain * cxt_ptr->bakup_rgb_gain + 0.5);
		ISP_LOGV("d-gain: coeff: %f-%d-%d\n", gain, cxt_ptr->bakup_rgb_gain, final_gain);

		gain_info.bypass = 0;
		gain_info.global_gain = final_gain;
		gain_info.r_gain = rgb_gain_offset;
		gain_info.g_gain = rgb_gain_offset;
		gain_info.b_gain = rgb_gain_offset;

		if(0 == flag)
			cxt_ptr->ae_set_cb(cxt_ptr->caller_handle, ISP_AE_SET_RGB_GAIN, (void *)&gain_info, NULL);
		else if(1 == flag)
			cxt_ptr->ae_set_cb(cxt_ptr->caller_handle, ISP_AE_SET_RGB_GAIN_FOR_4IN1, (void *)&gain_info, NULL);
		else if(2 == flag)
			cxt_ptr->ae_set_cb(cxt_ptr->caller_handle, ISP_AE_SET_RGB_GAIN_SLAVE0, (void *)&gain_info, NULL);
		else if(3 == flag)
			cxt_ptr->ae_set_cb(cxt_ptr->caller_handle, ISP_AE_SET_RGB_GAIN_SLAVE1, (void *)&gain_info, NULL);
	}
	return rtn;
}

static cmr_s32 ae_set_rgb_gain(cmr_handle handler, double gain)
{
	return __ae_set_rgb_gain(handler, gain, 0);
}

static cmr_s32 ae_set_rgb_gain_4in1(cmr_handle handler, double gain)
{
	return __ae_set_rgb_gain(handler, gain, 1);
}

static cmr_s32 ae_set_rgb_gain_slave0(cmr_handle handler, double gain)
{
	return __ae_set_rgb_gain(handler, gain, 2);
}

static cmr_s32 ae_set_rgb_gain_slave1(cmr_handle handler, double gain)
{
	return __ae_set_rgb_gain(handler, gain, 3);
}

static cmr_s32 ae_set_wbc_gain(cmr_handle handler, struct ae_alg_rgb_gain *awb_gain)
{
	cmr_int rtn = ISP_SUCCESS;
	struct aectrl_cxt *cxt_ptr = (struct aectrl_cxt *)handler;
	struct ae_awb_gain wbc_gain;
	wbc_gain.r = awb_gain->r;
	wbc_gain.g = awb_gain->g;
	wbc_gain.b = awb_gain->b;

	if (cxt_ptr->ae_set_cb) {
		cxt_ptr->ae_set_cb(cxt_ptr->caller_handle, ISP_AE_SET_WBC_GAIN, &wbc_gain, NULL);
	}

	return rtn;
}

static cmr_int ae_get_rgb_gain(cmr_handle handler, cmr_u32 * gain)
{
	cmr_int rtn = ISP_SUCCESS;
	struct aectrl_cxt *cxt_ptr = (struct aectrl_cxt *)handler;

	if (cxt_ptr->ae_set_cb) {
		cxt_ptr->ae_set_cb(cxt_ptr->caller_handle, ISP_AE_GET_RGB_GAIN, gain, NULL);
	}

	return rtn;
}

static cmr_s32 ae_set_shutter_gain_delay_info(cmr_handle handler, cmr_handle param)
{
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *) handler;

	if ((NULL == ctrl_context) || (NULL == param)) {
		ISP_LOGE("fail to set delay info, cxt: %p, param: %p\n", ctrl_context, param);
		return ISP_PARAM_NULL;
	}

	struct ae_valid_fn valid_info = { 0, 0, 0 };
	struct ae_exp_gain_delay_info *delay_info_ptr = (struct ae_exp_gain_delay_info *)param;

	valid_info.group_hold_flag = delay_info_ptr->group_hold_flag;
	valid_info.valid_expo_num = delay_info_ptr->valid_exp_num;
	valid_info.valid_gain_num = delay_info_ptr->valid_gain_num;

	return 0;
}

static cmr_int aectrl_deinit_adpt(struct aectrl_cxt *cxt_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	struct aectrl_work_lib *lib_ptr = NULL;

	if (!cxt_ptr) {
		ISP_LOGE("fail to deinit");
		goto aectrl_deinit_adpt_exit;
	}

	lib_ptr = &cxt_ptr->work_lib;
	if (lib_ptr->adpt_ops->adpt_deinit) {
		rtn = lib_ptr->adpt_ops->adpt_deinit(lib_ptr->lib_handle, NULL, NULL);
		lib_ptr->lib_handle = NULL;
	} else {
		ISP_LOGI("fail to do adpt_deinit");
	}

  aectrl_deinit_adpt_exit:
	ISP_LOGV("done %ld", rtn);
	return rtn;
}

static cmr_int aectrl_ioctrl(struct aectrl_cxt *cxt_ptr, enum ae_io_ctrl_cmd cmd, cmr_handle in_ptr, cmr_handle out_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	struct aectrl_work_lib *lib_ptr = NULL;

	if (!cxt_ptr) {
		ISP_LOGE("fail to check param");
		goto aectrl_ioctrl_exit;
	}

	lib_ptr = &cxt_ptr->work_lib;
	if (lib_ptr->adpt_ops->adpt_ioctrl) {
		rtn = lib_ptr->adpt_ops->adpt_ioctrl(lib_ptr->lib_handle, cmd, in_ptr, out_ptr);
	} else {
		ISP_LOGI("ioctrl fun is NULL");
	}

  aectrl_ioctrl_exit:
	ISP_LOGV("done %ld", rtn);
	return rtn;
}

static cmr_int aectrl_process(struct aectrl_cxt *cxt_ptr, struct ae_calc_in *in_ptr, struct ae_calc_out *out_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	struct aectrl_work_lib *lib_ptr = NULL;

	if (!cxt_ptr) {
		ISP_LOGE("fail to check param, param is NULL!");
		goto aectrl_process_exit;
	}

	lib_ptr = &cxt_ptr->work_lib;
	if (lib_ptr->adpt_ops->adpt_process) {
		rtn = lib_ptr->adpt_ops->adpt_process(lib_ptr->lib_handle, in_ptr, out_ptr);
	} else {
		ISP_LOGI("ioctrl fun is NULL");
	}

  aectrl_process_exit:
	ISP_LOGV("done %ld", rtn);
	return rtn;
}

static cmr_int aectrl_ctrl_thr_proc(struct cmr_msg *message, cmr_handle p_data)
{
	cmr_int rtn = ISP_SUCCESS;
	struct aectrl_cxt *cxt_ptr = (struct aectrl_cxt *)p_data;

	if (!message || !p_data) {
		ISP_LOGE("fail to check param");
		goto aectrl_ctrl_thr_proc_exit;
	}
	ISP_LOGV("message.msg_type 0x%x, data %p", message->msg_type, message->data);

	switch (message->msg_type) {
	case AECTRL_EVT_INIT:
		break;
	case AECTRL_EVT_DEINIT:
		rtn = aectrl_deinit_adpt(cxt_ptr);
		break;
	case AECTRL_EVT_EXIT:
		break;
	case AECTRL_EVT_IOCTRL:
		rtn = aectrl_ioctrl(cxt_ptr, ((struct ae_ctrl_msg_ctrl *)message->data)->cmd, ((struct ae_ctrl_msg_ctrl *)message->data)->in, ((struct ae_ctrl_msg_ctrl *)message->data)->out);
		break;
	case AECTRL_EVT_PROCESS:
		rtn = aectrl_process(cxt_ptr, (struct ae_calc_in *)message->data, NULL);
		break;
	default:
		ISP_LOGE("fail to check param, don't support msg");
		break;
	}

  aectrl_ctrl_thr_proc_exit:
	ISP_LOGV("done %ld", rtn);
	return rtn;
}

static cmr_int aectrl_create_thread(struct aectrl_cxt *cxt_ptr)
{
	cmr_int rtn = ISP_SUCCESS;

	rtn = cmr_thread_create(&cxt_ptr->thr_handle, ISP_THREAD_QUEUE_NUM, aectrl_ctrl_thr_proc, (cmr_handle) cxt_ptr);
	if (rtn) {
		ISP_LOGE("fail to create ctrl thread");
		rtn = ISP_ERROR;
		goto aectrl_create_thread_exit;
	}
	rtn = cmr_thread_set_name(cxt_ptr->thr_handle, "aectrl");
	if (CMR_MSG_SUCCESS != rtn) {
		ISP_LOGE("fail to set aectrl name");
		rtn = CMR_MSG_SUCCESS;
	}
  aectrl_create_thread_exit:
	ISP_LOGV("done %ld", rtn);
	return rtn;
}

static cmr_int aectrl_destroy_thread(struct aectrl_cxt *cxt_ptr)
{
	cmr_int rtn = ISP_SUCCESS;

	if (!cxt_ptr) {
		ISP_LOGE("fail to check input param");
		rtn = ISP_ERROR;
		goto aectrl_destroy_thread_exit;
	}

	if (cxt_ptr->thr_handle) {
		rtn = cmr_thread_destroy(cxt_ptr->thr_handle);
		if (!rtn) {
			cxt_ptr->thr_handle = NULL;
		} else {
			ISP_LOGE("fail to destroy ctrl thread %ld", rtn);
		}
	}
  aectrl_destroy_thread_exit:
	ISP_LOGV("done %ld", rtn);
	return rtn;
}

static cmr_int aectrl_init_lib(struct aectrl_cxt *cxt_ptr, struct ae_init_in *in_ptr, struct ae_init_out *out_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct aectrl_work_lib *lib_ptr = NULL;

	if (!cxt_ptr) {
		ISP_LOGE("fail to check param,param is NULL!");
		goto aectrl_init_lib_exit;
	} else {
		ISP_LOGE("ae cxt: %p\n", cxt_ptr);
	}

	lib_ptr = &cxt_ptr->work_lib;
	if (lib_ptr->adpt_ops) {
		if (lib_ptr->adpt_ops->adpt_init) {
			lib_ptr->lib_handle = lib_ptr->adpt_ops->adpt_init(in_ptr, out_ptr);
		} else {
			ISP_LOGW("adpt_ops_init fun is NULL");
		}
	} else {
		ISP_LOGW("adpt_ops fun is NULL");
	}

  aectrl_init_lib_exit:
	ISP_LOGV("done %ld", ret);
	return ret;
}

static cmr_int aectrl_init_adpt(struct aectrl_cxt *cxt_ptr, struct ae_init_in *in_ptr, struct ae_init_out *out_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	int *ae_tuning_param = NULL;
	int alg_version_param = 0;
	int major_id = 0;
	int minor_id = 0;

	if (!cxt_ptr) {
		ISP_LOGE("fail to check param,param is NULL!");
		goto aectrl_init_adpt_exit;
	}

	/* find vendor adpter */
	// uint32 version id;
	// uint32 verify;
	// uint16_t major_id;
	// uint16_t minor_id;
	ae_tuning_param = (int *)in_ptr->param[0].param;
	alg_version_param = * (ae_tuning_param + 2);
	major_id = alg_version_param & 0xffff;
	minor_id = alg_version_param >> 16;

	ISP_LOGI("aectrl_init_adpt, major_id:%d, minor_id:%d", major_id, minor_id);

	if((major_id ==3) && (minor_id ==1)){
		ISP_LOGI("AE3.X");
		in_ptr->lib_param.version_id = 1; //ae3.x
	} else {
		ISP_LOGI("AE2.X");
		in_ptr->lib_param.version_id = 0; //ae2.x
	}

	#ifdef CONFIG_ISP_2_7
	in_ptr->lib_param.version_id = 1; //ae3.x
	#endif

	rtn = adpt_get_ops(ADPT_LIB_AE, &in_ptr->lib_param, &cxt_ptr->work_lib.adpt_ops);
	if (rtn) {
		ISP_LOGE("fail to get adapter layer ret = %ld", rtn);
		goto aectrl_init_adpt_exit;
	}

	rtn = aectrl_init_lib(cxt_ptr, in_ptr, out_ptr);

  aectrl_init_adpt_exit:
	ISP_LOGI("done %ld", rtn);
	return rtn;
}

cmr_int ae_ctrl_ioctrl(cmr_handle handle, enum ae_io_ctrl_cmd cmd, cmr_handle in_ptr, cmr_handle out_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	struct aectrl_cxt *cxt_ptr = (struct aectrl_cxt *)handle;
	struct ae_ctrl_msg_ctrl msg_ctrl;

	ISP_CHECK_HANDLE_VALID(handle);
	CMR_MSG_INIT(message);
	if ((AE_SYNC_MSG_BEGIN < cmd) && (AE_SYNC_MSG_END > cmd)) {
		msg_ctrl.cmd = cmd;
		msg_ctrl.in = in_ptr;
		msg_ctrl.out = out_ptr;
		message.data = &msg_ctrl;
		message.msg_type = AECTRL_EVT_IOCTRL;
		message.sync_flag = CMR_MSG_SYNC_PROCESSED;
		rtn = cmr_thread_msg_send(cxt_ptr->thr_handle, &message);
	} else if ((AE_DIRECT_MSG_BEGIN < cmd) && (AE_DIRECT_MSG_END > cmd)) {
		rtn = aectrl_ioctrl(cxt_ptr, cmd, in_ptr, out_ptr);
	}

	ISP_LOGV("cmd = %d,done %ld", cmd, rtn);

	return rtn;
}

cmr_s32 ae_ctrl_init(struct ae_init_in * input_ptr, cmr_handle * handle_ae, cmr_handle result)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct aectrl_cxt *cxt_ptr = NULL;

	input_ptr->isp_ops.set_again = ae_set_again;
	input_ptr->isp_ops.set_exposure = ae_set_exposure;
	input_ptr->isp_ops.set_monitor_win = ae_set_monitor_win;
	input_ptr->isp_ops.set_monitor_rgb_thd = ae_set_rgb_thrd;//added by beth for aem
	input_ptr->isp_ops.set_monitor = ae_set_monitor;
	input_ptr->isp_ops.callback = ae_callback;
	input_ptr->isp_ops.set_monitor_bypass = ae_set_monitor_bypass;
	input_ptr->isp_ops.get_system_time = ae_get_system_time;
	input_ptr->isp_ops.set_statistics_mode = ae_set_statistics_mode;
	input_ptr->isp_ops.flash_get_charge = ae_flash_get_charge;
	input_ptr->isp_ops.flash_get_time = ae_flash_get_time;
	input_ptr->isp_ops.flash_set_charge = ae_flash_set_charge;
	input_ptr->isp_ops.flash_set_time = ae_flash_set_time;
	input_ptr->isp_ops.flash_ctrl = ae_flash_ctrl_enable;
	input_ptr->isp_ops.ex_set_exposure = ae_ex_set_exposure;
	input_ptr->isp_ops.set_rgb_gain = ae_set_rgb_gain;
	input_ptr->isp_ops.set_shutter_gain_delay_info = ae_set_shutter_gain_delay_info;
	input_ptr->isp_ops.set_wbc_gain = ae_set_wbc_gain;
	input_ptr->isp_ops.write_multi_ae = ae_write_multi_ae;
	input_ptr->isp_ops.ae_bokeh_hdr_cb = ae_bokeh_hdr_cb;
	input_ptr->isp_ops.set_stats_monitor = ae_set_stats_monitor;
	input_ptr->isp_ops.set_blk_num = ae_set_blk_num;
	input_ptr->isp_ops.set_rgb_gain_4in1 = ae_set_rgb_gain_4in1;
	input_ptr->isp_ops.set_rgb_gain_slave0 = ae_set_rgb_gain_slave0;
	input_ptr->isp_ops.set_rgb_gain_slave1 = ae_set_rgb_gain_slave1;
	input_ptr->isp_ops.set_bayer_hist = ae_set_bayer_hist;
	input_ptr->isp_ops.set_aem_zoom_win = ae_set_aem_zoom_win;

	cxt_ptr = (struct aectrl_cxt *)calloc(1,sizeof(*cxt_ptr));
	if (NULL == cxt_ptr) {
		ISP_LOGE("fail to create ae ctrl context!");
		rtn = ISP_ALLOC_ERROR;
		goto ae_ctrl_init_exit;
	}

	input_ptr->isp_ops.isp_handler = (cmr_handle) cxt_ptr;
	cxt_ptr->caller_handle = input_ptr->caller_handle;
	cxt_ptr->ae_set_cb = input_ptr->ae_set_cb;

	rtn = aectrl_create_thread(cxt_ptr);
	if (rtn) {
		goto ae_ctrl_init_exit;
	}

	ae_get_rgb_gain(cxt_ptr, &cxt_ptr->bakup_rgb_gain);
	ISP_LOGV("bakup_rgb_gain %d", cxt_ptr->bakup_rgb_gain);

	input_ptr->bakup_rgb_gain = cxt_ptr->bakup_rgb_gain;

	rtn = aectrl_init_adpt(cxt_ptr, input_ptr, result);
	if (rtn) {
		goto error_adpt_init;
	}

	*handle_ae = (cmr_handle) cxt_ptr;

	ISP_LOGV("done %d", rtn);
	return rtn;

  error_adpt_init:
	aectrl_destroy_thread(cxt_ptr);

  ae_ctrl_init_exit:
	if (cxt_ptr) {
		free((cmr_handle) cxt_ptr);
		cxt_ptr = NULL;
	}

	return rtn;
}

cmr_int ae_ctrl_deinit(cmr_handle * handle_ae)
{
	cmr_int rtn = ISP_SUCCESS;
	struct aectrl_cxt *cxt_ptr = *handle_ae;
	CMR_MSG_INIT(message);

	if (!cxt_ptr) {
		ISP_LOGE("fail to check param, in parm is NULL");
		return ISP_ERROR;
	}

	message.msg_type = AECTRL_EVT_DEINIT;
	message.sync_flag = CMR_MSG_SYNC_PROCESSED;
	message.alloc_flag = 0;
	message.data = NULL;
	rtn = cmr_thread_msg_send(cxt_ptr->thr_handle, &message);
	if (rtn) {
		ISP_LOGE("fail to send msg to main thr %ld", rtn);
		goto ae_ctrl_deinit_exit;
	}

	rtn = aectrl_destroy_thread(cxt_ptr);
	if (rtn) {
		ISP_LOGE("fail to destroy aectrl thread %ld", rtn);
		goto ae_ctrl_deinit_exit;
	}

  ae_ctrl_deinit_exit:
	if (cxt_ptr) {
		free((cmr_handle) cxt_ptr);
		*handle_ae = NULL;
	}

	ISP_LOGV("done %ld", rtn);
	return rtn;
}

cmr_int ae_ctrl_process(cmr_handle handle_ae, struct ae_calc_in * in_ptr, struct ae_calc_out * result)
{
	cmr_int rtn = ISP_SUCCESS;
	struct aectrl_cxt *cxt_ptr = (struct aectrl_cxt *)handle_ae;
	UNUSED(result);

	ISP_CHECK_HANDLE_VALID(handle_ae);
	CMR_MSG_INIT(message);

	message.data = malloc(sizeof(struct ae_calc_in));
	if (!message.data) {
		ISP_LOGE("fail to malloc msg");
		rtn = ISP_ALLOC_ERROR;
		goto ae_ctrl_process_exit;
	}
	memcpy(message.data, (cmr_handle) in_ptr, sizeof(struct ae_calc_in));
	message.alloc_flag = 1;

	message.msg_type = AECTRL_EVT_PROCESS;
	message.sync_flag = CMR_MSG_SYNC_NONE;
	rtn = cmr_thread_msg_send(cxt_ptr->thr_handle, &message);

	if (rtn) {
		ISP_LOGE("fail to send msg to main thr %ld", rtn);
		if (message.data)
			free(message.data);
		goto ae_ctrl_process_exit;
	}

  ae_ctrl_process_exit:
	ISP_LOGV("done %ld", rtn);
	return rtn;
}
