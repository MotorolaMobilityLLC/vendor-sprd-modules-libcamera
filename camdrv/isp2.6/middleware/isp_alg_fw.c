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
#define LOG_TAG "isp_alg_fw"

#include <math.h>
#include <dlfcn.h>
#include <cutils/properties.h>
#include "isp_alg_fw.h"
#include "cmr_msg.h"
#include "cmr_prop.h"
#include "isp_dev_access.h"
#include "isp_debug.h"
#include "isp_pm.h"
#include "awb.h"
#include "af_ctrl.h"
#include "ae_ctrl.h"
#include "afl_ctrl.h"
#include "awb_ctrl.h"
#include "smart_ctrl.h"
#include "lsc_adv.h"
#include "isp_bridge.h"
#include "pdaf_ctrl.h"

#define LIBCAM_ALG_FILE "libispalg.so"
#define CMC10(n) (((n)>>13)?((n)-(1<<14)):(n))


#if 0
enum statis_event_id {
	ISP_EVENT_SOF = 0,
	ISP_EVENT_RAW_DONE,
	ISP_EVENT_AEM_STATIS,
	ISP_EVENT_AFM_STATIS,
	ISP_EVENT_AFL_STATIS,
	ISP_EVENT_HIST_STATIS,
	ISP_EVENT_PDAF_STATIS,
	ISP_EVENT_MAX
};

struct isp_event_info {
	uint32_t irq_type;
	uint32_t irq_property;
	enum isp_statis_buf_type type;
	union {
		struct init {
			int mfd;
			uint32_t  buf_size;
		} init_data;
		struct block {
			uint32_t hw_addr;
		} block_data;
	} u;
	uint32_t uaddr[2];
	uint32_t kaddr[2];
	uint32_t sec;
	uint32_t usec;
	uint32_t frame_id;
};
struct isp_event_node {
	struct isp_event_info data;
	void *next;
};
#endif

struct commn_info {
	cmr_s32 isp_mode;
	cmr_u32 mode_flag;
	cmr_u32 multi_nr_flag;
	cmr_u32 scene_flag;
	cmr_u32 image_pattern;
	cmr_u32 param_index;
	cmr_u32 isp_callback_bypass;
	proc_callback callback;
	cmr_handle caller_id;
	cmr_u8 *log_isp;
	cmr_u32 log_isp_size;
	struct isp_size src;
	struct isp_ops ops;
	struct sensor_raw_resolution_info input_size_trim[ISP_INPUT_SIZE_NUM_MAX];
};

struct ae_info {
	cmr_handle handle;
	cmr_u32 sw_bypass;
	cmr_u8 *log_alc_ae;
	cmr_u32 log_alc_ae_size;
	cmr_u8 *log_alc;
	cmr_u32 log_alc_size;
	cmr_u8 *log_ae;
	cmr_u32 log_ae_size;
	cmr_uint vir_addr;
	cmr_int buf_size;
	cmr_int buf_num;
	cmr_uint phy_addr;
	cmr_uint mfd;
	cmr_int buf_property;
	void *buffer_client_data;
	struct ae_size win_num;
	struct ae_size win_size;
	cmr_u32 shift;
	cmr_u32 flash_version;
};

struct awb_info {
	cmr_handle handle;
	cmr_u32 sw_bypass;
	cmr_u32 alc_awb;
	cmr_s32 awb_pg_flag;
	cmr_u8 *log_alc_awb;
	cmr_u32 log_alc_awb_size;
	cmr_u8 *log_alc_lsc;
	cmr_u32 log_alc_lsc_size;
	cmr_u8 *log_awb;
	cmr_u32 log_awb_size;
	struct awb_ctrl_gain cur_gain;
};

struct smart_info {
	cmr_handle handle;
	cmr_u32 sw_bypass;
	cmr_u32 isp_smart_eb;
	cmr_u8 *log_smart;
	cmr_u32 log_smart_size;
	cmr_u8	lock_nlm_en;
	cmr_u8	lock_ee_en;
	cmr_u8	lock_precdn_en;
	cmr_u8	lock_cdn_en;
	cmr_u8	lock_postcdn_en;
	cmr_u8	lock_ccnr_en;
	cmr_u8	lock_ynr_en;
	cmr_s16 smart_block_eb[ISP_SMART_MAX_BLOCK_NUM];
	void *tunning_gamma_cur;
};

struct afl_info {
	cmr_handle handle;
	cmr_u32 version;
	cmr_u32 sw_bypass;
	cmr_uint vir_addr;
	cmr_int buf_size;
	cmr_int buf_num;
	cmr_uint phy_addr;
	cmr_u32 afl_mode;
	cmr_uint mfd;
	cmr_int buf_property;
	void *buffer_client_data;
};

struct af_info {
	cmr_handle handle;
	cmr_u32 sw_bypass;
	cmr_u8 *log_af;
	cmr_u32 log_af_size;
	struct af_monitor_win_num win_num;
};

struct aft_info {
	cmr_handle handle;
	cmr_u8 *log_aft;
	cmr_u32 log_aft_size;
};

struct pdaf_info {
	cmr_handle handle;
	cmr_u32 sw_bypass;
	cmr_u8 pdaf_support;
	cmr_u8 pdaf_en;
};

struct ebd_info {
	cmr_handle handle;
	cmr_u32 sw_bypass;
	cmr_u8 ebd_support;
	cmr_u8 ebd_en;
};

struct lsc_info {
	cmr_handle handle;
	cmr_u32 sw_bypass;
	void *lsc_info;
	void *lsc_tab_address;
	cmr_u32 lsc_tab_size;
	cmr_u32 isp_smart_lsc_lock;
	cmr_u8 *log_lsc;
	cmr_u32 log_lsc_size;
};


struct ispalg_ae_ctrl_ops {
	cmr_s32 (*init)(struct ae_init_in *input_ptr, cmr_handle *handle_ae, cmr_handle result);
	cmr_int (*deinit)(cmr_handle *isp_afl_handle);
	cmr_int (*process)(cmr_handle handle_ae, struct ae_calc_in *in_ptr, struct ae_calc_out *result);
	cmr_int (*ioctrl)(cmr_handle handle, enum ae_io_ctrl_cmd cmd, cmr_handle in_ptr, cmr_handle out_ptr);
};

struct ispalg_af_ctrl_ops {
	cmr_int (*init)(struct afctrl_init_in *input_ptr, cmr_handle *handle_af);
	cmr_int (*deinit)(cmr_handle handle);
	cmr_int (*process)(cmr_handle handle_af, void *in_ptr, struct afctrl_calc_out *result);
	cmr_int (*ioctrl)(cmr_handle handle_af, cmr_int cmd, void *in_ptr, void *out_ptr);
};

struct ispalg_afl_ctrl_ops {
	cmr_int (*init)(cmr_handle *isp_afl_handle, struct afl_ctrl_init_in *input_ptr);
	cmr_int (*deinit)(cmr_handle *isp_afl_handle);
	cmr_int (*process)(cmr_handle isp_afl_handle, struct afl_proc_in *in_ptr, struct afl_ctrl_proc_out *out_ptr);
	cmr_int (*config)(cmr_handle isp_afl_handle);
	cmr_int (*config_new)(cmr_handle isp_afl_handle);
	cmr_int (*ioctrl)(cmr_handle handle, enum afl_io_ctrl_cmd cmd, void *in_ptr, void *out_ptr);
};

struct ispalg_awb_ctrl_ops {
	cmr_int (*init)(struct awb_ctrl_init_param *input_ptr, cmr_handle *handle_awb);
	cmr_int (*deinit)(cmr_handle *handle_awb);
	cmr_int (*process)(cmr_handle handle_awb, struct awb_ctrl_calc_param *param, struct awb_ctrl_calc_result *result);
	cmr_int (*ioctrl)(cmr_handle handle_awb, enum awb_ctrl_cmd cmd, void *in_ptr, void *out_ptr);
};

struct ispalg_smart_ctrl_ops {
	smart_handle_t (*init)(struct smart_init_param *param, void *result);
	cmr_s32 (*deinit)(smart_handle_t *handle, void *param, void *result);
	cmr_s32 (*ioctrl)(smart_handle_t handle, cmr_u32 cmd, void *param, void *result);
	cmr_s32 (*calc)(cmr_handle handle_smart, struct smart_proc_input *in_ptr);
	cmr_s32 (*block_disable)(cmr_handle handle, cmr_u32 smart_id);
	cmr_s32 (*block_enable)(cmr_handle handle, cmr_u32 smart_id);
	cmr_s32 (*NR_disable)(cmr_handle handle, cmr_u32 is_diseb);
};

struct ispalg_pdaf_ctrl_ops {
	cmr_int (*init)(struct pdaf_ctrl_init_in *in, struct pdaf_ctrl_init_out *out, cmr_handle *handle);
	cmr_int (*deinit)(cmr_handle *handle);
	cmr_int (*process)(cmr_handle handle, struct pdaf_ctrl_process_in *in, struct pdaf_ctrl_process_out *out);
	cmr_int (*ioctrl)(cmr_handle handle, cmr_int cmd, struct pdaf_ctrl_param_in *in, struct pdaf_ctrl_param_out *out);
};

struct ispalg_lsc_ctrl_ops {
	cmr_int (*init)(struct lsc_adv_init_param *input_ptr, cmr_handle *handle_lsc);
	cmr_int (*deinit)(cmr_handle *handle_lsc);
	cmr_int (*process)(cmr_handle handle_lsc, struct lsc_adv_calc_param *in_ptr, struct lsc_adv_calc_result *result);
	cmr_int (*ioctrl)(cmr_handle handle_lsc, cmr_s32 cmd, void *in_ptr, void *out_ptr);
};

struct ispalg_lib_ops {
	struct ispalg_ae_ctrl_ops ae_ops;
	struct ispalg_af_ctrl_ops af_ops;
	struct ispalg_afl_ctrl_ops afl_ops;
	struct ispalg_awb_ctrl_ops awb_ops;
	struct ispalg_smart_ctrl_ops smart_ops;
	struct ispalg_pdaf_ctrl_ops pdaf_ops;
	struct ispalg_lsc_ctrl_ops lsc_ops;
};

struct isp_alg_sw_init_in {
	cmr_handle dev_access_handle;
	struct sensor_libuse_info *lib_use_info;
	struct isp_size size;
	struct sensor_otp_cust_info *otp_data;
	struct sensor_pdaf_info *pdaf_info;
	struct isp_size sensor_max_size;
};

struct isp_alg_fw_context {
	cmr_int camera_id;
	cmr_u8 aem_is_update;
	struct isp_awb_statistic_info aem_stats_data;
	struct isp_hist_statistic_info bayer_hist_stats[3];
	struct afctrl_ae_info ae_info;
	struct afctrl_awb_info awb_info;
	struct commn_info commn_cxt;
	struct sensor_data_info sn_cxt;
	struct ae_info ae_cxt;
	struct awb_info awb_cxt;
	struct smart_info smart_cxt;
	struct af_info af_cxt;
	struct aft_info aft_cxt;
	struct lsc_info lsc_cxt;
	struct afl_info afl_cxt;
	struct pdaf_info pdaf_cxt;
	struct ebd_info ebd_cxt;
	struct sensor_libuse_info *lib_use_info;
	struct sensor_raw_ioctrl *ioctrl_ptr;
	struct sensor_pdaf_info *pdaf_info;
	struct isp_mem_info mem_info;
	cmr_handle thr_handle;
	cmr_handle thr_afhandle;
	cmr_handle dev_access_handle;
	cmr_handle handle_pm;

	struct isp_sensor_fps_info sensor_fps;
	struct sensor_otp_cust_info *otp_data;
	cmr_u32 lsc_flash_onoff;
	cmr_u32 takepicture_mode;
	cmr_handle ispalg_lib_handle;
	struct ispalg_lib_ops ops;
	cmr_u8  is_master;
	cmr_u32 is_multi_mode;
	cmr_u32 work_mode;
	cmr_u32 zsl_flag;
	struct pm_workmode_info pm_mode_cxt;

#if 0
	/* thread for statis/irq reading/process */
	pthread_t read_thr_handle;
	pthread_t proc_thr_handle;
	cmr_s32 read_should_exit;
	cmr_s32 proc_should_exit;
	sem_t read_start_sem;
	sem_t read_exit_sem;
	sem_t proc_start_sem;
	sem_t proc_exit_sem;
	pthread_mutex_t list_lock;
	struct isp_event_node *event_list_head;
#endif
};

#define FEATRUE_ISP_FW_IOCTRL
#include "isp_ioctrl.c"
#undef FEATRUE_ISP_FW_IOCTRL

static nsecs_t ispalg_get_sys_timestamp(void)
{
	nsecs_t timestamp = 0;

	timestamp = systemTime(CLOCK_MONOTONIC);
	timestamp = timestamp / 1000000;

	return timestamp;
}

static cmr_int ispalg_get_rgb_gain(cmr_handle isp_fw_handle, cmr_u32 *param)
{
	cmr_s32 ret = ISP_SUCCESS;
	struct isp_pm_param_data param_data;
	struct isp_pm_ioctl_input input = { NULL, 0 };
	struct isp_pm_ioctl_output output = { NULL, 0 };
	struct dcam_dev_rgb_gain_info *gain_info;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_fw_handle;

	memset(&param_data, 0, sizeof(param_data));

	BLOCK_PARAM_CFG(input, param_data,
			ISP_PM_BLK_ISP_SETTING,
			ISP_BLK_RGB_GAIN, NULL, 0);
	ret = isp_pm_ioctl(cxt->handle_pm,
			ISP_PM_CMD_GET_SINGLE_SETTING,
			&input, &output);
	if (ISP_SUCCESS == ret && 1 == output.param_num) {
		gain_info = (struct dcam_dev_rgb_gain_info *)output.param_data->data_ptr;
		*param = gain_info->global_gain;
	} else {
		*param = 4096;
	}

	ISP_LOGV("D-gain global gain ori: %d\n", *param);

	return ret;
}

static cmr_int ispalg_set_rgb_gain(cmr_handle isp_fw_handle, void *param)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_fw_handle;
	struct isp_rgb_gain_info *inptr;
	struct dcam_dev_rgb_gain_info gain_info;
	struct isp_u_blocks_info block_info;

	if (!param) {
		ISP_LOGE("fail to get gain info.\n");
		return ISP_ERROR;
	}
	inptr = (struct isp_rgb_gain_info *)param;
	gain_info.bypass = inptr->bypass;
	gain_info.global_gain = inptr->global_gain;
	gain_info.r_gain = inptr->r_gain;
	gain_info.g_gain = inptr->g_gain;
	gain_info.b_gain = inptr->b_gain;
	block_info.block_info = &gain_info;
	/* todo: also set capture gain if 4in1 */
	block_info.scene_id = PM_SCENE_PRE;

	ISP_LOGV("global_gain : %d, r %d g %d b %d\n", gain_info.global_gain,
		gain_info.r_gain, gain_info.g_gain, gain_info.b_gain);
	ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_RGB_GAIN, &block_info, NULL);

	return ret;
}

static cmr_int ispalg_get_aem_param(cmr_handle isp_fw_handle, struct isp_rgb_aem_info *param)
{
	cmr_s32 ret = ISP_SUCCESS;
	struct isp_pm_ioctl_input input = { NULL, 0 };
	struct isp_pm_ioctl_output output = { NULL, 0 };
	struct isp_pm_param_data param_data;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_fw_handle;
	struct isp_size *aem_win;

	memset(&param_data, 0, sizeof(param_data));

	/* supported AE win num: 32x32, 64x64, 128x128 */
	BLOCK_PARAM_CFG(input, param_data, ISP_PM_BLK_AEM_WIN, ISP_BLK_RGB_AEM, NULL, 0);
	ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_SINGLE_SETTING, &input, &output);
	if (ISP_SUCCESS == ret && 1 == output.param_num) {
		aem_win = (struct isp_size *)output.param_data->data_ptr;
		param->blk_num.w = (aem_win->w < 32) ? 32 : aem_win->w;
		param->blk_num.h = (aem_win->h < 32) ? 32 : aem_win->h;
		ISP_LOGD("get blk_num %d %d, set to %d %d\n",
			aem_win->w, aem_win->h, param->blk_num.w, param->blk_num.h);
	} else {
		param->blk_num.w = 32;
		param->blk_num.h = 32;
		ISP_LOGV("set default blk_num 32x32\n");
	}

	return ret;
}

static cmr_int isp_prepare_atm_param(cmr_handle isp_alg_handle,
	struct smart_proc_input *smart_proc_in)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;

	smart_proc_in->r_info = cxt->aem_stats_data.r_info;
	smart_proc_in->g_info = cxt->aem_stats_data.g_info;
	smart_proc_in->b_info = cxt->aem_stats_data.b_info;
	smart_proc_in->win_num_w = cxt->ae_cxt.win_num.w;
	smart_proc_in->win_num_h = cxt->ae_cxt.win_num.h;
	smart_proc_in->aem_shift = cxt->ae_cxt.shift;
	smart_proc_in->win_size_w = cxt->ae_cxt.win_size.w;
	smart_proc_in->win_size_h = cxt->ae_cxt.win_size.h;

	if (smart_proc_in->r_info == NULL)
		ISP_LOGE("fail to access null r/g/b ptr %p/%p/%p\n",
			smart_proc_in->r_info,
			smart_proc_in->g_info,
			smart_proc_in->b_info);

	return ret;
}


static cmr_int ispalg_ae_callback(cmr_handle isp_alg_handle, cmr_int cb_type, void *data)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	enum isp_callback_cmd cmd = 0;

	switch (cb_type) {
	case AE_CB_FLASHING_CONVERGED:
	case AE_CB_CONVERGED:
	case AE_CB_CLOSE_PREFLASH:
	case AE_CB_PREFLASH_PERIOD_END:
	case AE_CB_CLOSE_MAIN_FLASH:
		cmd = ISP_AE_STAB_CALLBACK;
		break;
	case AE_CB_QUICKMODE_DOWN:
		cmd = ISP_QUICK_MODE_DOWN;
		break;
	case AE_CB_TOUCH_AE_NOTIFY:
	case AE_CB_STAB_NOTIFY:
		cmd = ISP_AE_STAB_NOTIFY;
		break;
	case AE_CB_AE_LOCK_NOTIFY:
		cmd = ISP_AE_LOCK_NOTIFY;
		break;
	case AE_CB_AE_UNLOCK_NOTIFY:
		cmd = ISP_AE_UNLOCK_NOTIFY;
		break;
	case AE_CB_HDR_START:
		cmd = ISP_HDR_EV_EFFECT_CALLBACK;
		break;
	case AE_CB_LED_NOTIFY:
		cmd = ISP_ONLINE_FLASH_CALLBACK;
		break;
	case AE_CB_FLASH_FIRED:
		cmd = ISP_AE_CB_FLASH_FIRED;
		break;
	case AE_CB_PROCESS_OUT:
		break;
	default:
		cmd = ISP_AE_STAB_CALLBACK;
		break;
	}

	if (cxt->commn_cxt.callback) {
		cxt->commn_cxt.callback(cxt->commn_cxt.caller_id, ISP_CALLBACK_EVT | cmd, data, 0);
	}

	return ret;
}

static cmr_int ispalg_set_ae_stats_mode(
	cmr_handle isp_alg_handle, cmr_u32 mode,
	cmr_u32 skip_number)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_u32 ae_mode;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;

	if (mode == AE_STATISTICS_MODE_SINGLE)
		ae_mode = AEM_MODE_SINGLE;
	else
		ae_mode = AEM_MODE_MULTI;

	ret = isp_dev_access_ioctl(cxt->dev_access_handle,
					   ISP_DEV_SET_AE_STATISTICS_MODE,
					   &ae_mode,
					   &skip_number);

	return ret;
}

static cmr_int ispalg_set_aem_win(cmr_handle isp_alg_handle, struct ae_monitor_info *aem_info)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct dcam_dev_aem_win aem_win;

	ISP_LOGD("win %d %d %d %d %d %d\n",
			aem_info->trim.x, aem_info->trim.y,
			aem_info->win_size.w, aem_info->win_size.h,
			aem_info->win_num.w, aem_info->win_num.h);

	cxt->ae_cxt.win_num.w = aem_info->win_num.w;
	cxt->ae_cxt.win_num.h = aem_info->win_num.h;
	cxt->ae_cxt.win_size.w = aem_info->win_size.w;
	cxt->ae_cxt.win_size.h = aem_info->win_size.h;
	aem_win.offset_x = aem_info->trim.x;
	aem_win.offset_y = aem_info->trim.y;
	aem_win.blk_width = aem_info->win_size.w;
	aem_win.blk_height = aem_info->win_size.h;
	aem_win.blk_num_x = aem_info->win_num.w;
	aem_win.blk_num_y = aem_info->win_num.h;

	ret = isp_dev_access_ioctl(cxt->dev_access_handle,
			ISP_DEV_SET_AE_MONITOR_WIN,
			&aem_win, NULL);
	return ret;
}

static cmr_int ispalg_ae_set_cb(cmr_handle isp_alg_handle,
		cmr_int type, void *param0, void *param1)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	cmr_handle isp_3a_handle_slv;
	struct isp_alg_fw_context *cxt_slv = NULL;
	struct sensor_multi_ae_info *ae_info = NULL;

	if (!cxt) {
		ISP_LOGE("fail to get valid ctx ptr\n");
		return ISP_PARAM_NULL;
	}

	switch (type) {
	case ISP_AE_MULTI_WRITE:
		ISP_LOGV("ISP_AE_MULTI_WRITE");
		ae_info = (struct sensor_multi_ae_info *)param0;
		ae_info[0].handle = cxt->ioctrl_ptr->caller_handler;
		ae_info[0].camera_id = cxt->camera_id;
		if (ae_info[0].count == 2) {
			isp_3a_handle_slv = isp_br_get_3a_handle(cxt->camera_id + 2);
			cxt_slv = (struct isp_alg_fw_context *)isp_3a_handle_slv;
			if (cxt_slv != NULL) {
				ae_info[1].handle = cxt_slv->ioctrl_ptr->caller_handler;
				ae_info[1].camera_id = cxt->camera_id + 2;
			} else {
				ISP_LOGE("fail to get slave sensor handle , it is not ready");
				return ret;
			}
		}
		if (cxt->ioctrl_ptr->sns_ioctl)
			ret = cxt->ioctrl_ptr->sns_ioctl(cxt->ioctrl_ptr->caller_handler,
				CMD_SNS_IC_WRITE_MULTI_AE, ae_info);
		break;
	case ISP_AE_SET_GAIN:
		ret = cxt->ioctrl_ptr->set_gain(cxt->ioctrl_ptr->caller_handler, *(cmr_u32 *) param0);
		break;
	case ISP_AE_SET_EXPOSURE:
		ret = cxt->ioctrl_ptr->set_exposure(cxt->ioctrl_ptr->caller_handler, *(cmr_u32 *) param0);
		break;
	case ISP_AE_EX_SET_EXPOSURE:
		ret = cxt->ioctrl_ptr->ex_set_exposure(cxt->ioctrl_ptr->caller_handler, (cmr_uint) param0);
		break;
	case ISP_AE_SET_MONITOR:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AE_SKIP_NUM, param0, NULL);
		break;
	case ISP_AE_SET_MONITOR_WIN:
		ret = ispalg_set_aem_win(cxt, param0);
		break;
	case ISP_AE_SET_MONITOR_BYPASS:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AE_MONITOR_BYPASS, param0, NULL);
		break;
	case ISP_AE_SET_STATISTICS_MODE:
		ret = ispalg_set_ae_stats_mode(cxt, *(cmr_u32 *)param0, *(cmr_u32 *)param1);
		break;
	case ISP_AE_SET_AE_CALLBACK:
		ret = ispalg_ae_callback(cxt, *(cmr_int *) param0, param1);
		break;
	case ISP_AE_GET_SYSTEM_TIME:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_GET_SYSTEM_TIME, param0, param1);
		break;
	case ISP_AE_SET_RGB_GAIN:
		ret = ispalg_set_rgb_gain(cxt, param0);
		break;
	case ISP_AE_GET_FLASH_CHARGE:
		ret = cxt->commn_cxt.ops.flash_get_charge(cxt->commn_cxt.caller_id, param0, param1);
		break;
	case ISP_AE_GET_FLASH_TIME:
		ret = cxt->commn_cxt.ops.flash_get_time(cxt->commn_cxt.caller_id, param0, param1);
		break;
	case ISP_AE_SET_FLASH_CHARGE:
		ret = cxt->commn_cxt.ops.flash_set_charge(cxt->commn_cxt.caller_id, param0, param1);
		break;
	case ISP_AE_SET_FLASH_TIME:
		ret = cxt->commn_cxt.ops.flash_set_time(cxt->commn_cxt.caller_id, param0, param1);
		break;
	case ISP_AE_FLASH_CTRL:
		ret = cxt->commn_cxt.ops.flash_ctrl(cxt->commn_cxt.caller_id, param0, param1);
		break;
	case ISP_AE_GET_RGB_GAIN:
		ret = ispalg_get_rgb_gain(cxt, param0);
		break;
	case ISP_AE_SET_WBC_GAIN:
	{
		struct img_rgb_info gain;
		struct isp_u_blocks_info cfg;
		struct ae_awb_gain *awb_gain = (struct ae_awb_gain*)param0;
		gain.r = awb_gain->r;
		gain.gr = awb_gain->g;
		gain.gb = awb_gain->g;
		gain.b = awb_gain->b;
		/* todo: also set capture gain if 4in1 */
		cfg.scene_id = PM_SCENE_PRE;
		cfg.block_info = &gain;
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AWB_GAIN, &cfg, NULL);
		break;
	}
	default:
		ISP_LOGE("unsupported ae cb: %lx\n", type);
		break;
	}

	return ret;
}

static cmr_int ispalg_af_set_cb(cmr_handle isp_alg_handle, cmr_int type, void *param0, void *param1)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;

	if (!cxt) {
		ISP_LOGE("fail to get valid ctx ptr\n");
		return ISP_PARAM_NULL;
	}

	switch (type) {
	case ISP_AF_SET_POS:
		if (cxt->ioctrl_ptr->set_focus) {
			ret = cxt->ioctrl_ptr->set_focus(cxt->ioctrl_ptr->caller_handler, *(cmr_u32 *) param0);
		}
		break;
	case AF_CB_CMD_SET_END_NOTICE:
		if (ISP_ZERO == cxt->commn_cxt.isp_callback_bypass) {
			ret = cxt->commn_cxt.callback(cxt->commn_cxt.caller_id,
				ISP_CALLBACK_EVT | ISP_AF_NOTICE_CALLBACK,
				param0, sizeof(struct isp_af_notice));
		}
		break;
	case AF_CB_CMD_SET_MOTOR_POS:
		if (cxt->ioctrl_ptr->set_pos) {
			ret = cxt->ioctrl_ptr->set_pos(cxt->ioctrl_ptr->caller_handler, *(cmr_u32 *) param0);
		}
		break;
	case AF_CB_CMD_GET_LENS_OTP:
		if (cxt->ioctrl_ptr->get_otp) {
			ret = cxt->ioctrl_ptr->get_otp(cxt->ioctrl_ptr->caller_handler, param0, param1);
		}
		break;
	case AF_CB_CMD_SET_MOTOR_BESTMODE:
		if (cxt->ioctrl_ptr->set_motor_bestmode) {
			ret = cxt->ioctrl_ptr->set_motor_bestmode(cxt->ioctrl_ptr->caller_handler);
		}
		break;
	case AF_CB_CMD_GET_MOTOR_POS:
		if (cxt->ioctrl_ptr->get_motor_pos) {
			ret = cxt->ioctrl_ptr->get_motor_pos(cxt->ioctrl_ptr->caller_handler, param0);
		}
		break;
	case AF_CB_CMD_SET_VCM_TEST_MODE:
		if (cxt->ioctrl_ptr->set_test_vcm_mode) {
			ret = cxt->ioctrl_ptr->set_test_vcm_mode(cxt->ioctrl_ptr->caller_handler, (char *)param0);
		}
		break;
	case AF_CB_CMD_GET_VCM_TEST_MODE:
		if (cxt->ioctrl_ptr->get_test_vcm_mode) {
			ret = cxt->ioctrl_ptr->get_test_vcm_mode(cxt->ioctrl_ptr->caller_handler);
		}
		break;
	case AF_CB_CMD_SET_START_NOTICE:
		ret = cxt->commn_cxt.callback(cxt->commn_cxt.caller_id,
				ISP_CALLBACK_EVT | ISP_AF_NOTICE_CALLBACK,
				param0, sizeof(struct isp_af_notice));
		break;
	case ISP_AF_AE_AWB_LOCK:
		if (cxt->ops.ae_ops.ioctrl)
			ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_PAUSE, NULL, param1);
		if (cxt->ops.awb_ops.ioctrl)
			ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_LOCK, NULL, NULL);
		break;
	case ISP_AF_AE_AWB_RELEASE:
		if (cxt->ops.ae_ops.ioctrl)
			ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_RESTORE, NULL, param1);
		if (cxt->ops.awb_ops.ioctrl)
			ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_UNLOCK, NULL, NULL);
		break;
	case AF_CB_CMD_SET_AE_LOCK:
		if (cxt->ops.ae_ops.ioctrl)
			ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_PAUSE, NULL, param1);
		break;
	case AF_CB_CMD_SET_AE_UNLOCK:
		if (cxt->ops.ae_ops.ioctrl)
			ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_RESTORE, NULL, param1);
		break;
	case AF_CB_CMD_SET_AE_CAF_LOCK:
		if (cxt->ops.ae_ops.ioctrl)
			ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_CAF_LOCKAE_START, NULL, param1);
		break;
	case AF_CB_CMD_SET_AE_CAF_UNLOCK:
		if (cxt->ops.ae_ops.ioctrl)
			ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_CAF_LOCKAE_STOP, NULL, param1);
		break;
	case AF_CB_CMD_SET_AWB_LOCK:
		if (cxt->ops.awb_ops.ioctrl)
			ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_LOCK, NULL, NULL);
		break;
	case AF_CB_CMD_SET_AWB_UNLOCK:
		if (cxt->ops.awb_ops.ioctrl)
			ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_UNLOCK, NULL, NULL);
		break;
	case AF_CB_CMD_SET_LSC_LOCK:
		if (cxt->ops.lsc_ops.ioctrl)
			ret = cxt->ops.lsc_ops.ioctrl(cxt->lsc_cxt.handle, SMART_LSC_ALG_LOCK, NULL, NULL);
		break;
	case AF_CB_CMD_SET_LSC_UNLOCK:
		if (cxt->ops.lsc_ops.ioctrl)
			ret = cxt->ops.lsc_ops.ioctrl(cxt->lsc_cxt.handle, SMART_LSC_ALG_UNLOCK, NULL, NULL);
		break;
	case AF_CB_CMD_SET_NLM_LOCK:
		cxt->smart_cxt.lock_nlm_en = 1;
		break;
	case AF_CB_CMD_SET_NLM_UNLOCK:
		cxt->smart_cxt.lock_nlm_en = 0;
		break;
	case AF_CB_CMD_SET_AFM_BYPASS:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AFM_BYPASS, param0, param1);
		break;
	case AF_CB_CMD_SET_AFM_MODE:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AFM_WORK_MODE, param0, param1);
		break;
	case AF_CB_CMD_SET_AFM_SKIP_NUM:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AFM_SKIP_NUM, param0, param1);
		break;
	case AF_CB_CMD_SET_MONITOR_WIN:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AFM_MONITOR_WIN, param0, param1);
		break;
	case AF_CB_CMD_SET_MONITOR_WIN_NUM:
	{
		cxt->af_cxt.win_num = *(struct af_monitor_win_num *)param0;
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AFM_MONITOR_WIN_NUM, param0, param1);
		break;
	}
	case AF_CB_CMD_SET_AFM_CROP_EB:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AFM_CROP_EB, param0, param1);
		break;
	case AF_CB_CMD_SET_AFM_CROP_SIZE:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AFM_CROP_SIZE, param0, param1);
		break;
	case AF_CB_CMD_SET_AFM_DONE_TILE_NUM:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AFM_DONE_TILE_NUM, param0, param1);
		break;
	case AF_CB_CMD_GET_SYSTEM_TIME:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_GET_SYSTEM_TIME, param0, param1);
		break;
	default:
		ISP_LOGE("unsupported af cb: %lx\n", type);
		break;
	}

	return ret;
}

static cmr_int ispalg_pdaf_set_cb(cmr_handle isp_alg_handle, cmr_int type, void *param0, void *param1)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;

	if (!cxt) {
		ISP_LOGE("fail to get valid ctx ptr\n");
		return ISP_PARAM_NULL;
	}

	ISP_LOGV("isp_pdaf_set_cb type = 0x%lx", type);
	switch (type) {
	case ISP_AF_SET_PD_INFO:
		if (cxt->ops.af_ops.ioctrl)
			ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle, AF_CMD_SET_PD_INFO, param0, param1);
		break;
	case ISP_PDAF_SET_CFG_PARAM:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_PDAF_CFG_PARAM, param0, param1);
		break;
	case ISP_PDAF_SET_PPI_INFO:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_PDAF_PPI_INFO, param0, param1);
		break;
	case ISP_PDAF_SET_BYPASS:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_PDAF_BYPASS, param0, param1);
		break;
	case ISP_PDAF_SET_WORK_MODE:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_PDAF_WORK_MODE, param0, param1);
		break;
	case ISP_PDAF_SET_EXTRACTOR_BYPASS:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_PDAF_EXTRACTOR_BYPASS, param0, param1);
		break;
	case ISP_PDAF_SET_ROI:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_PDAF_ROI, param0, param1);
		break;
	case ISP_PDAF_SET_SKIP_NUM:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_PDAF_SKIP_NUM, param0, param1);
		break;
	default:
		ISP_LOGE("unsupported pdaf cb: %lx\n", type);
		break;
	}

	return ret;
}

static cmr_int ispalg_afl_set_cb(cmr_handle isp_alg_handle, cmr_int type, void *param0, void *param1)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;

	if (!cxt) {
		ISP_LOGE("fail to get valid ctx ptr\n");
		return ISP_PARAM_NULL;
	}
	switch (type) {
	case ISP_AFL_NEW_SET_CFG_PARAM:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AFL_NEW_CFG_PARAM, param0, param1);
		break;
	case ISP_AFL_NEW_SET_BYPASS:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AFL_NEW_BYPASS, param0, param1);
		break;
	case ISP_AFL_SET_STATS_BUFFER:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_STSTIS_BUF, param0, param1);
	default:
		ISP_LOGE("unsupported afl cb: %lx\n", type);
		break;
	}

	return ret;
}

static cmr_int ispalg_smart_set_cb(cmr_handle isp_alg_handle, cmr_int type, void *param0, void *param1)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_u32 i;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct smart_calc_result *smart_result = NULL;
	struct smart_block_result *block_result = NULL;
	struct isp_pm_ioctl_input io_pm_input = { NULL, 0 };
	struct isp_pm_param_data pm_param;

	UNUSED(param1);

	switch (type) {
	case ISP_SMART_SET_COMMON:
		smart_result = (struct smart_calc_result *)param0;
		for (i = 0; i < smart_result->counts; i++) {
			block_result = &smart_result->block_result[i];
			if (block_result->update == 0)
				continue;

			memset(&pm_param, 0, sizeof(pm_param));
			BLOCK_PARAM_CFG(io_pm_input, pm_param,
					ISP_PM_BLK_SMART_SETTING,
					block_result->block_id, block_result,
					sizeof(*block_result));
			pm_param.mode_id = block_result->mode_flag;
			ISP_LOGV("set param %d, id=%x, mode %d, data=%p",
					i, block_result->block_id, pm_param.mode_id, block_result);
			ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_SMART, &io_pm_input, NULL);
			ISP_TRACE_IF_FAIL(ret, ("fail to set smart"));
		}

		break;

	case ISP_SMART_SET_GAMMA_CUR:
		/* todo: callback from auto gamma */
		break;

	default:
		ISP_LOGE("fail to get smart callback cmd: %d", (cmr_u32)type);
		break;
	}

	return ret;
}

static cmr_s32 ispalg_alsc_get_info(cmr_handle isp_alg_handle)
{
	cmr_s32 ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	cmr_handle pm_handle = cxt->handle_pm;
	struct isp_pm_ioctl_input io_pm_input = { NULL, 0 };
	struct isp_pm_ioctl_output io_pm_output = { NULL, 0 };
	struct isp_pm_param_data pm_param;

	ISP_LOGD("enter\n");

	memset(&pm_param, 0, sizeof(struct isp_pm_param_data));
	BLOCK_PARAM_CFG(io_pm_input, pm_param,
			ISP_PM_BLK_LSC_GET_LSCTAB,
			ISP_BLK_2D_LSC, NULL, 0);
	ret = isp_pm_ioctl(pm_handle,
			ISP_PM_CMD_GET_SINGLE_SETTING,
			(void *)&io_pm_input, (void *)&io_pm_output);

	cxt->lsc_cxt.lsc_tab_address = io_pm_output.param_data->data_ptr;
	cxt->lsc_cxt.lsc_tab_size = io_pm_output.param_data->data_size;

	if (NULL == cxt->lsc_cxt.lsc_tab_address || ISP_SUCCESS != ret) {
		ISP_LOGE("fail to get lsc tab");
		return ISP_ERROR;
	}

	memset(&pm_param, 0, sizeof(struct isp_pm_param_data));
	BLOCK_PARAM_CFG(io_pm_input, pm_param,
			ISP_PM_BLK_LSC_INFO,
			ISP_BLK_2D_LSC, PNULL, 0);
	ret = isp_pm_ioctl(pm_handle,
			ISP_PM_CMD_GET_SINGLE_SETTING,
			(void *)&io_pm_input, (void *)&io_pm_output);

	cxt->lsc_cxt.lsc_info = (struct isp_lsc_info *)io_pm_output.param_data->data_ptr;
	if (NULL == cxt->lsc_cxt.lsc_info || ISP_SUCCESS != ret) {
		ISP_LOGE("fail to get lsc info");
		return ISP_ERROR;
	}

	return ISP_SUCCESS;
}


cmr_s32 ispalg_alsc_calc(cmr_handle isp_alg_handle,
		  cmr_u32 * ae_stat_r, cmr_u32 * ae_stat_g, cmr_u32 * ae_stat_b,
		  struct awb_size * stat_img_size,
		  struct awb_size * win_size,
		  cmr_u32 image_width, cmr_u32 image_height,
		  cmr_u32 awb_ct,
		  cmr_s32 awb_r_gain, cmr_s32 awb_b_gain,
		  cmr_u32 ae_stable, struct ae_ctrl_callback_in *ae_in)
{
	cmr_s32 ret = ISP_SUCCESS;
	cmr_u32 i = 0;
	cmr_s32 bv_gain = 0;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	lsc_adv_handle_t lsc_adv_handle = cxt->lsc_cxt.handle;
	cmr_handle pm_handle = cxt->handle_pm;
	struct isp_2d_lsc_param *lsc_tab_param_ptr;
	struct isp_lsc_info *lsc_info;
	struct alsc_ver_info lsc_ver = { 0 };
	struct alsc_update_info update_info = { 0, 0, NULL };
	struct lsc_adv_calc_param calc_param;
	struct lsc_adv_calc_result calc_result = { 0 };
	struct isp_pm_ioctl_input io_pm_input = { NULL, 0 };
	struct isp_pm_param_data pm_param;

	if (cxt->ops.ae_ops.ioctrl) {
		ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_GET_BV_BY_GAIN, NULL, (void *)&bv_gain);
		ISP_TRACE_IF_FAIL(ret, ("fail to AE_GET_BV_BY_GAIN"));
	}

	if (cxt->ops.lsc_ops.ioctrl)
		ret = cxt->ops.lsc_ops.ioctrl(lsc_adv_handle, ALSC_GET_VER, NULL, (void *)&lsc_ver);
	if (ISP_SUCCESS != ret) {
		ISP_LOGE("fail to Get ALSC ver info!");
	}

	if (lsc_ver.LSC_SPD_VERSION >= 2) {
		/* todo: move to fw_start */
		ret = ispalg_alsc_get_info(cxt);
		ISP_RETURN_IF_FAIL(ret, ("fail to do get lsc info"));
		lsc_tab_param_ptr = (struct isp_2d_lsc_param *)(cxt->lsc_cxt.lsc_tab_address);
		lsc_info = (struct isp_lsc_info *)cxt->lsc_cxt.lsc_info;

		memset(&calc_param, 0, sizeof(calc_param));
		calc_result.dst_gain = (cmr_u16 *) lsc_info->data_ptr;
		calc_param.stat_img.r = ae_stat_r;
		calc_param.stat_img.gr = ae_stat_g;
		calc_param.stat_img.gb = ae_stat_g;
		calc_param.stat_img.b = ae_stat_b;
		calc_param.stat_size.w = stat_img_size->w;
		calc_param.stat_size.h = stat_img_size->h;
		calc_param.gain_width = lsc_info->gain_w;
		calc_param.gain_height = lsc_info->gain_h;
		calc_param.block_size.w = win_size->w;
		calc_param.block_size.h = win_size->h;
		calc_param.lum_gain = (cmr_u16 *) lsc_info->param_ptr;
		calc_param.ct = awb_ct;
		calc_param.r_gain = awb_r_gain;
		calc_param.b_gain = awb_b_gain;
		calc_param.grid = lsc_info->grid;
		calc_param.captureFlashEnvRatio = ae_in->flash_param.captureFlashEnvRatio;
		calc_param.captureFlash1ofALLRatio = ae_in->flash_param.captureFlash1ofALLRatio;

		cxt->awb_cxt.cur_gain.r = awb_r_gain;
		cxt->awb_cxt.cur_gain.b = awb_b_gain;

		calc_param.bv = ae_in->ae_output.cur_bv;
		calc_param.bv_gain = bv_gain;
		calc_param.ae_stable = ae_stable;
		calc_param.isp_mode = cxt->commn_cxt.isp_mode;
		calc_param.isp_id = ISP_2_0;
		calc_param.camera_id = cxt->camera_id;
		calc_param.awb_pg_flag = cxt->awb_cxt.awb_pg_flag;

		calc_param.img_size.w = image_width;
		calc_param.img_size.h = image_height;

		for (i = 0; i < 9; i++) {
			calc_param.lsc_tab_address[i] = lsc_tab_param_ptr->map_tab[i].param_addr;
		}
		calc_param.lsc_tab_size = cxt->lsc_cxt.lsc_tab_size;

		if (cxt->lsc_cxt.isp_smart_lsc_lock == 0) {
			if (cxt->ops.lsc_ops.process)
				ret = cxt->ops.lsc_ops.process(lsc_adv_handle, &calc_param, &calc_result);
			if (ISP_SUCCESS != ret) {
				ISP_LOGE("fail to do lsc adv gain map calc");
				return ret;
			}
		}

		if (cxt->ops.lsc_ops.ioctrl)
			ret = cxt->ops.lsc_ops.ioctrl(lsc_adv_handle, ALSC_GET_UPDATE_INFO, NULL, (void *)&update_info);
		if (ISP_SUCCESS != ret)
			ISP_LOGE("fail to get ALSC update flag!");

		if (update_info.alsc_update_flag == 1){
			memset(&pm_param, 0, sizeof(struct isp_pm_param_data));
			BLOCK_PARAM_CFG(io_pm_input, pm_param,
				ISP_PM_BLK_LSC_MEM_ADDR,
				ISP_BLK_2D_LSC, update_info.lsc_buffer_addr,
				lsc_info->gain_w * lsc_info->gain_h * 4 * sizeof(cmr_u16));
			ret = isp_pm_ioctl(pm_handle, ISP_PM_CMD_SET_OTHERS, &io_pm_input, NULL);
		}
	}

	return ret;
}

static cmr_s32 ispalg_cfg_param(cmr_handle isp_alg_handle, cmr_u32 start)
{
	cmr_s32 ret = ISP_SUCCESS;
	cmr_u32 i = 0;
	cmr_u32 scene_id;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_pm_ioctl_input_param input;
	struct isp_pm_ioctl_output_param output;
	struct isp_pm_param_data *param_data;
	struct isp_u_blocks_info sub_block_info;

	memset((void *)&input, 0x00, sizeof(struct isp_pm_ioctl_input_param));
	memset((void *)&output, 0x00, sizeof(struct isp_pm_ioctl_output_param));
	memset((void *)&sub_block_info, 0x00, sizeof(sub_block_info));

	if (start)
		isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_ISP_ALL_SETTING, &input, &output);
	else
		isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_ISP_SETTING, &input, &output);

	/* work_mode: 1 - capture only, 0 - auto/preview */
	scene_id = cxt->work_mode ? PM_SCENE_CAP : PM_SCENE_PRE;
	param_data = output.prv_param_data;
	for (i = 0; i < output.prv_param_num; i++) {
		sub_block_info.block_info = param_data->data_ptr;
		sub_block_info.scene_id = scene_id;
		isp_dev_cfg_block(cxt->dev_access_handle, &sub_block_info, param_data->id);
		ISP_LOGV("cfg block %x for prev.\n", param_data->id);
		param_data++;
	}

	if ((cxt->work_mode == 0) && cxt->zsl_flag)  {
		param_data = output.cap_param_data;
		for (i = 0; i < output.cap_param_num; i++) {
			sub_block_info.block_info = param_data->data_ptr;
			sub_block_info.scene_id = PM_SCENE_CAP;
			if (!IS_DCAM_BLOCK(param_data->id)) {
				/* todo: refine for 4in1 sensor */
				isp_dev_cfg_block(cxt->dev_access_handle, &sub_block_info, param_data->id);
				ISP_LOGV("cfg block %x for cap.\n", param_data->id);
			}
			param_data++;
		}
	}

	return ret;
}

static cmr_int ispalg_handle_sensor_sof(cmr_handle isp_alg_handle)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_u32 sec = 0;
	cmr_u32 usec = 0;
	struct isp_af_ts af_ts;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;

	ret = ispalg_cfg_param(cxt, 0);

	if (cxt->ops.af_ops.ioctrl) {
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_GET_SYSTEM_TIME, &sec, &usec);
		af_ts.timestamp = sec * 1000000000LL + usec * 1000LL;
		af_ts.capture = 0;
		ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle,
					AF_CMD_SET_DCAM_TIMESTAMP, (void *)(&af_ts), NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to set AF_CMD_SET_DCAM_TIMESTAMP"));
	}

	return ret;
}

static cmr_int ispalg_aem_stats_parser(cmr_handle isp_alg_handle, void *data)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_u32 ae_shift = 0;
	cmr_u32 i = 0;
	cmr_u32 blk_num = 0;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_awb_statistic_info *ae_stat_ptr = NULL;
	struct isp_statis_info *statis_info = (struct isp_statis_info *)data;

	cmr_u64 *uaddr;
	cmr_u64 stats_val0 = { 0 };
	cmr_u64 stats_val1 = { 0 };
	cmr_u32 sum_r_oe = 0;
	cmr_u32 sum_g_oe = 0;
	cmr_u32 sum_b_oe = 0;
	cmr_u32 sum_r_ue = 0;
	cmr_u32 sum_g_ue = 0;
	cmr_u32 sum_b_ue = 0;
	cmr_u32 sum_r_ae = 0;
	cmr_u32 sum_g_ae = 0;
	cmr_u32 sum_b_ae = 0;

	cmr_u32 cnt_r_oe = 0;
	cmr_u32 cnt_r_ue = 0;
	cmr_u32 cnt_b_oe = 0;
	cmr_u32 cnt_b_ue = 0;
	cmr_u32 cnt_g_oe = 0;
	cmr_u32 cnt_g_ue = 0;

	ae_stat_ptr = &cxt->aem_stats_data;
	ae_shift = cxt->ae_cxt.shift;
	blk_num = cxt->ae_cxt.win_num.w * cxt->ae_cxt.win_num.h;
	uaddr = (cmr_u64 *)statis_info->uaddr;
	for (i = 0; i < blk_num; i++) {
		stats_val0 = *uaddr++;
		stats_val1 = *uaddr++;
		sum_g_ue = (cmr_u32)(stats_val0 & 0xffffff);
		sum_g_ae = (cmr_u32)((stats_val0 >> 24) & 0xffffff);
		sum_g_oe = (cmr_u32)((stats_val0 >> 48) & 0xffff);
		sum_g_oe |= (cmr_u32)((stats_val1 & 0xff) << 16);
		cnt_g_ue += (cmr_u32)((stats_val1 >> 8) & 0x3fff);
		cnt_g_oe += (cmr_u32)((stats_val1 >> 22) & 0x3fff);

		stats_val0 = *uaddr++;
		stats_val1 = *uaddr++;
		sum_r_ue = (cmr_u32)(stats_val0 & 0xffffff);
		sum_r_ae = (cmr_u32)((stats_val0 >> 24) & 0xffffff);
		sum_r_oe = (cmr_u32)((stats_val0 >> 48) & 0xffff);
		sum_r_oe |= (cmr_u32)((stats_val1 & 0xff) << 16);
		cnt_r_ue += (cmr_u32)((stats_val1 >> 8) & 0x3fff);
		cnt_r_oe += (cmr_u32)((stats_val1 >> 22) & 0x3fff);

		stats_val0 = *uaddr++;
		stats_val1 = *uaddr++;
		sum_b_ue = (cmr_u32)(stats_val0 & 0xffffff);
		sum_b_ae = (cmr_u32)((stats_val0 >> 24) & 0xffffff);
		sum_b_oe = (cmr_u32)((stats_val0 >> 48) & 0xffff);
		sum_b_oe |= (cmr_u32)((stats_val1 & 0xff) << 16);
		cnt_b_ue += (cmr_u32)((stats_val1 >> 8) & 0x3fff);
		cnt_b_oe += (cmr_u32)((stats_val1 >> 22) & 0x3fff);

		ae_stat_ptr->r_info[i] = (sum_r_oe + sum_r_ue + sum_r_ae) << ae_shift;
		ae_stat_ptr->g_info[i] = (sum_g_oe + sum_g_ue + sum_g_ae) << ae_shift;
		ae_stat_ptr->b_info[i] = (sum_b_oe + sum_b_ue + sum_b_ae) << ae_shift;
	}
	ISP_LOGD("sum[0]: r 0x%x, g 0x%x, b 0x%x\n",
		ae_stat_ptr->r_info[0], ae_stat_ptr->g_info[0], ae_stat_ptr->b_info[0]);
	ISP_LOGD("cnt: r 0x%x, 0x%x,  g 0x%x, 0x%x, b 0x%x, 0x%x\n",
		cnt_r_ue, cnt_r_oe, cnt_g_ue, cnt_g_oe, cnt_b_ue, cnt_b_oe);

	ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_STSTIS_BUF, statis_info, NULL);
	if (ret) {
		ISP_LOGE("fail to set statis buf");
	}

	cxt->aem_is_update = 1;
	return ret;
}

static cmr_int ispalg_hist_stats_parser(cmr_handle isp_alg_handle, void *data)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_u32 i, j;
	cmr_u64 *ptr;
	cmr_u64 val0, val1;
	struct isp_hist_statistic_info *hist_stats;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_statis_info *statis_info = (struct isp_statis_info *)data;

	ptr = (cmr_u64 *)statis_info->uaddr;

	/* G */
	hist_stats = &cxt->bayer_hist_stats[0];
	hist_stats->sec = statis_info->sec;
	hist_stats->usec = statis_info->usec;
	hist_stats->frame_id = statis_info->frame_id;
	j = 0;
	for (i = 0; i < 51; i++) {
		val0 = *ptr++;
		val1 = *ptr++;
		hist_stats->value[j++] = (cmr_u32)(val0 & 0xffffff);
		hist_stats->value[j++] = (cmr_u32)((val0 >> 24) & 0xffffff);
		hist_stats->value[j++] = (cmr_u32)(((val1 & 0xff) << 16) | ((val0 >> 48) & 0xffff));
		hist_stats->value[j++] = (cmr_u32)((val1 >> 8) & 0xffffff);
		hist_stats->value[j++] = (cmr_u32)((val1 >> 32) & 0xffffff);
	}
	val0 = *ptr++;
	val1 = *ptr++;
	i++;
	hist_stats->value[j++] = (cmr_u32)(val0 & 0xffffff);

	/* R */
	hist_stats = &cxt->bayer_hist_stats[1];
	hist_stats->sec = statis_info->sec;
	hist_stats->usec = statis_info->usec;
	hist_stats->frame_id = statis_info->frame_id;
	j = 0;
	hist_stats->value[j++] = (cmr_u32)((val0 >> 24) & 0xffffff);
	hist_stats->value[j++] = (cmr_u32)(((val1 & 0xff) << 16) | ((val0 >> 48) & 0xffff));
	hist_stats->value[j++] = (cmr_u32)((val1 >> 8) & 0xffffff);
	hist_stats->value[j++] = (cmr_u32)((val1 >> 32) & 0xffffff);
	for ( ; i < 102; i++) {
		val0 = *ptr++;
		val1 = *ptr++;
		hist_stats->value[j++] = (cmr_u32)(val0 & 0xffffff);
		hist_stats->value[j++] = (cmr_u32)((val0 >> 24) & 0xffffff);
		hist_stats->value[j++] = (cmr_u32)(((val1 & 0xff) << 16) | ((val0 >> 48) & 0xffff));
		hist_stats->value[j++] = (cmr_u32)((val1 >> 8) & 0xffffff);
		hist_stats->value[j++] = (cmr_u32)((val1 >> 32) & 0xffffff);
	}
	val0 = *ptr++;
	val1 = *ptr++;
	i++;
	hist_stats->value[j++] = (cmr_u32)(val0 & 0xffffff);
	hist_stats->value[j++] = (cmr_u32)((val0 >> 24) & 0xffffff);

	/* B */
	hist_stats = &cxt->bayer_hist_stats[2];
	hist_stats->sec = statis_info->sec;
	hist_stats->usec = statis_info->usec;
	hist_stats->frame_id = statis_info->frame_id;
	j = 0;
	hist_stats->value[j++] = (cmr_u32)(((val1 & 0xff) << 16) | ((val0 >> 48) & 0xffff));
	hist_stats->value[j++] = (cmr_u32)((val1 >> 8) & 0xffffff);
	hist_stats->value[j++] = (cmr_u32)((val1 >> 32) & 0xffffff);
	for ( ; i < 153; i++) {
		val0 = *ptr++;
		val1 = *ptr++;
		hist_stats->value[j++] = (cmr_u32)(val0 & 0xffffff);
		hist_stats->value[j++] = (cmr_u32)((val0 >> 24) & 0xffffff);
		hist_stats->value[j++] = (cmr_u32)(((val1 & 0xff) << 16) | ((val0 >> 48) & 0xffff));
		hist_stats->value[j++] = (cmr_u32)((val1 >> 8) & 0xffffff);
		hist_stats->value[j++] = (cmr_u32)((val1 >> 32) & 0xffffff);
	}
	val0 = *ptr++;
	val1 = *ptr++;
	i ++;
	hist_stats->value[j++] = (cmr_u32)(val0 & 0xffffff);
	hist_stats->value[j++] = (cmr_u32)((val0 >> 24) & 0xffffff);
	hist_stats->value[j++] = (cmr_u32)(((val1 & 0xff) << 16) | ((val0 >> 48) & 0xffff));
	ISP_LOGD("data: r %d %d, g %d %d, b %d %d\n",
		cxt->bayer_hist_stats[0].value[0], cxt->bayer_hist_stats[0].value[1],
		cxt->bayer_hist_stats[1].value[0], cxt->bayer_hist_stats[1].value[1],
		cxt->bayer_hist_stats[2].value[0], cxt->bayer_hist_stats[2].value[1]);

	ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_STSTIS_BUF, statis_info, NULL);
	if (ret) {
		ISP_LOGE("fail to set statis buf");
	}

	return ret;
}

static cmr_int ispalg_3dnr_statis_parser(cmr_handle isp_alg_handle, void *data) {
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_statis_info *statis_info = (struct isp_statis_info *)data;

	/* just send buffer back for now */

	ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_STSTIS_BUF, statis_info, NULL);
	if (ret) {
		ISP_LOGE("fail to set statis buf");
	}

	return ret;
}

cmr_int ispalg_start_ae_process(cmr_handle isp_alg_handle)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_int nxt_flicker = 0;
	cmr_u32 awb_mode = 0;
	nsecs_t time_start = 0;
	nsecs_t time_end = 0;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct ae_calc_in in_param;
	struct ae_calc_out ae_result;
	struct awb_gain gain = {0, 0, 0};
	struct awb_gain cur_gain = {0, 0, 0};
	struct afl_ctrl_proc_out afl_info = {0, 0};

	if (cxt->ops.awb_ops.ioctrl) {
		ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_GET_GAIN, (void *)&gain, NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to AWB_CTRL_CMD_GET_GAIN"));
	}

	if (cxt->ops.awb_ops.ioctrl) {
		ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_GET_CUR_GAIN, (void *)&cur_gain, NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to AWB_CTRL_CMD_GET_CUR_GAIN"));
	}

	if(cxt->ops.awb_ops.ioctrl){
		ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle,
					AWB_CTRL_CMD_GET_WB_MODE, NULL, (void *)&awb_mode);
		ISP_TRACE_IF_FAIL(ret, ("fail to AWB_CTRL_CMD_GET_GAIN"));
	}

	if (cxt->ops.afl_ops.ioctrl) {
		ret = cxt->ops.afl_ops.ioctrl(cxt->afl_cxt.handle, AFL_GET_INFO, (void *)&afl_info, NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to AFL_GET_INFO"));
	}

	if (afl_info.flag) {
		if (afl_info.cur_flicker) {
			nxt_flicker = AE_FLICKER_50HZ;
		} else {
			nxt_flicker = AE_FLICKER_60HZ;
		}
		if (cxt->ops.ae_ops.ioctrl)
			ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_FLICKER, &nxt_flicker, NULL);
	}

	memset((void *)&ae_result, 0, sizeof(ae_result));
	memset((void *)&in_param, 0, sizeof(in_param));
	if ((0 == gain.r) || (0 == gain.g) || (0 == gain.b) ||
		(0 == cur_gain.r) || (0 == cur_gain.g) || (0 == cur_gain.b) ) {
		in_param.awb_gain_r = 1024;
		in_param.awb_gain_g = 1024;
		in_param.awb_gain_b = 1024;
		in_param.awb_cur_gain_r = 1024;
		in_param.awb_cur_gain_g = 1024;
		in_param.awb_cur_gain_b = 1024;
		in_param.awb_mode = awb_mode;
	} else {
		in_param.awb_gain_r = gain.r;
		in_param.awb_gain_g = gain.g;
		in_param.awb_gain_b = gain.b;
		in_param.awb_cur_gain_r = cur_gain.r;
		in_param.awb_cur_gain_g = cur_gain.g;
		in_param.awb_cur_gain_b = cur_gain.b;
		in_param.awb_mode = awb_mode;
	}

	in_param.stat_fmt = AE_AEM_FMT_RGB;
	in_param.rgb_stat_img = cxt->aem_stats_data.r_info;
	in_param.stat_img = cxt->aem_stats_data.r_info;
	in_param.sec = cxt->aem_stats_data.sec;
	in_param.usec = cxt->aem_stats_data.usec;
	in_param.is_update = cxt->aem_is_update;
	in_param.sensor_fps.mode = cxt->sensor_fps.mode;
	in_param.sensor_fps.max_fps = cxt->sensor_fps.max_fps;
	in_param.sensor_fps.min_fps = cxt->sensor_fps.min_fps;
	in_param.sensor_fps.is_high_fps = cxt->sensor_fps.is_high_fps;
	in_param.sensor_fps.high_fps_skip_num = cxt->sensor_fps.high_fps_skip_num;

	time_start = ispalg_get_sys_timestamp();
	if (cxt->ops.ae_ops.process) {
		ret = cxt->ops.ae_ops.process(cxt->ae_cxt.handle, &in_param, &ae_result);
		ISP_TRACE_IF_FAIL(ret, ("fail to ae process"));
	}
	cxt->smart_cxt.isp_smart_eb = 1;
	time_end = ispalg_get_sys_timestamp();
	ISP_LOGV("SYSTEM_TEST-ae:%d ms", (cmr_u32)(time_end - time_start));

	ISP_LOGV("done %ld", ret);
	return ret;
}

cmr_int ispalg_awb_pre_process(cmr_handle isp_alg_handle,
				struct ae_ctrl_callback_in *ae_in,
				struct awb_ctrl_calc_param * out_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_s32 i = 0;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct ae_monitor_info info;
	struct ae_get_ev ae_ev;
	struct isp_pm_ioctl_input io_pm_input = { NULL, 0 };
	struct isp_pm_ioctl_output io_pm_output = { NULL, 0 };
	struct isp_pm_param_data pm_param;

	memset(&ae_ev, 0, sizeof(ae_ev));
	memset(&info, 0, sizeof(info));
	ISP_LOGV("cur_iso:%d monitor_info h:%d w:%d again:%d ev_level:%d",
		ae_in->ae_output.cur_iso,
		ae_in->monitor_info.win_size.h,
		ae_in->monitor_info.win_size.w,
		ae_in->ae_output.cur_again,
		ae_in->ae_ev.ev_index);

	out_ptr->quick_mode = 0;
	out_ptr->stat_img.chn_img.r = cxt->aem_stats_data.r_info;
	out_ptr->stat_img.chn_img.g = cxt->aem_stats_data.g_info;
	out_ptr->stat_img.chn_img.b = cxt->aem_stats_data.b_info;

	out_ptr->bv = ae_in->ae_output.cur_bv;

	out_ptr->ae_info.bv = ae_in->ae_output.cur_bv;
	out_ptr->ae_info.iso = ae_in->ae_output.cur_iso;
	out_ptr->ae_info.gain = ae_in->ae_output.cur_again;
	out_ptr->ae_info.exposure = ae_in->ae_output.exposure_time;
	out_ptr->ae_info.f_value = 2.2;
	out_ptr->ae_info.stable = ae_in->ae_output.is_stab;
	out_ptr->ae_info.ev_index = ae_in->ae_ev.ev_index;
	memcpy(out_ptr->ae_info.ev_table, ae_in->ae_ev.ev_tab, 16 * sizeof(cmr_s32));

	out_ptr->scalar_factor = (ae_in->monitor_info.win_size.h / 2) * (ae_in->monitor_info.win_size.w / 2);

	memset(&pm_param, 0, sizeof(pm_param));

	BLOCK_PARAM_CFG(io_pm_input, pm_param,
			ISP_PM_BLK_CMC10, ISP_BLK_CMC10, 0, 0);
	ret = isp_pm_ioctl(cxt->handle_pm,
			ISP_PM_CMD_GET_SINGLE_SETTING,
			&io_pm_input, &io_pm_output);

	if (io_pm_output.param_data != NULL && ISP_SUCCESS == ret) {
		cmr_u16 *cmc_info = io_pm_output.param_data->data_ptr;

		if (cmc_info != NULL) {
			for (i = 0; i < 9; i++) {
				out_ptr->matrix[i] = CMC10(cmc_info[i]);
			}
		}
	}

	BLOCK_PARAM_CFG(io_pm_input, pm_param,
			ISP_PM_BLK_GAMMA, ISP_BLK_RGB_GAMC, 0, 0);
	ret = isp_pm_ioctl(cxt->handle_pm,
			ISP_PM_CMD_GET_SINGLE_SETTING,
			&io_pm_input, &io_pm_output);
#if 0
	if (io_pm_output.param_data != NULL && ISP_SUCCESS == ret) {
		struct isp_dev_gamma_info *gamma_info = io_pm_output.param_data->data_ptr;

		if (gamma_info != NULL) {
			for (i = 0; i < 256; i++) {
				out_ptr->gamma[i] = (gamma_info->gamc_nodes.nodes_r[i] + gamma_info->gamc_nodes.nodes_r[i + 1]) / 2;
			}
		}
	}
#endif

exit:
	ISP_LOGV("done %ld", ret);
	return ret;
}

cmr_int ispalg_awb_post_process(cmr_handle isp_alg_handle, struct awb_ctrl_calc_result *awb_output)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_u32 i = 0;
	cmr_u16 lsc_param_tmp[4] = {0};
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_pm_ioctl_input pm_input = { NULL, 0 };
	struct isp_pm_ioctl_output pm_output = { NULL, 0 };
	struct isp_pm_param_data pm_data;
	struct isp_awbc_cfg awbc_cfg;

	memset((void *)&awbc_cfg, 0, sizeof(awbc_cfg));

	awbc_cfg.r_gain = awb_output->gain.r;
	awbc_cfg.g_gain = awb_output->gain.g;
	awbc_cfg.b_gain = awb_output->gain.b;
	awbc_cfg.r_offset = 0;
	awbc_cfg.g_offset = 0;
	awbc_cfg.b_offset = 0;

	if (0 == awbc_cfg.r_gain && 0 == awbc_cfg.g_gain && 0 == awbc_cfg.b_gain) {
		awbc_cfg.r_gain = 1800;
		awbc_cfg.g_gain = 1024;
		awbc_cfg.b_gain = 1536;
	}
	if(awb_output->update_gain){
		BLOCK_PARAM_CFG(pm_input, pm_data,
				ISP_PM_BLK_AWBC, ISP_BLK_AWB_NEW,
				&awbc_cfg,  sizeof(awbc_cfg));
		ret = isp_pm_ioctl(cxt->handle_pm,
				ISP_PM_CMD_SET_AWB, (void *)&pm_input, NULL);
	}

	if (awb_output->use_ccm) {
		memset(&pm_data, 0x0, sizeof(pm_data));
		BLOCK_PARAM_CFG(pm_input, pm_data,
				ISP_PM_BLK_CMC10, ISP_BLK_CMC10,
				awb_output->ccm, 9 * sizeof(cmr_u16));
		ret = isp_pm_ioctl(cxt->handle_pm,
				ISP_PM_CMD_SET_OTHERS, &pm_input, &pm_output);
		ISP_TRACE_IF_FAIL(ret, ("fail to set isp block param"));

	}
	cxt->awb_cxt.log_awb = awb_output->log_awb.log;
	cxt->awb_cxt.log_awb_size = awb_output->log_awb.size;

	if (awb_output->use_lsc) {
		switch (cxt->commn_cxt.image_pattern) {
		case SENSOR_IMAGE_PATTERN_RAWRGB_GR:
		{
			for (i = 0; i < awb_output->lsc_size / 8; i++) {
				lsc_param_tmp[0] = *((cmr_u16 *)awb_output->lsc + i * 4+ 0);
				lsc_param_tmp[1] = *((cmr_u16 *)awb_output->lsc + i * 4+ 1);
				lsc_param_tmp[2] = *((cmr_u16 *)awb_output->lsc + i * 4+ 2);
				lsc_param_tmp[3] = *((cmr_u16 *)awb_output->lsc + i * 4+ 3);
				*((cmr_u16 *)awb_output->lsc + i * 4+ 0) = lsc_param_tmp[3];
				*((cmr_u16 *)awb_output->lsc + i * 4+ 1) = lsc_param_tmp[2];
				*((cmr_u16 *)awb_output->lsc + i * 4+ 2) = lsc_param_tmp[1];
				*((cmr_u16 *)awb_output->lsc + i * 4+ 3) = lsc_param_tmp[0];
			}
			break;
		}
		case SENSOR_IMAGE_PATTERN_RAWRGB_R:
		{
			for (i = 0; i < awb_output->lsc_size / 8; i++) {
				lsc_param_tmp[0] = *((cmr_u16 *)awb_output->lsc + i * 4+ 0);
				lsc_param_tmp[1] = *((cmr_u16 *)awb_output->lsc + i * 4+ 1);
				lsc_param_tmp[2] = *((cmr_u16 *)awb_output->lsc + i * 4+ 2);
				lsc_param_tmp[3] = *((cmr_u16 *)awb_output->lsc + i * 4+ 3);
				*((cmr_u16 *)awb_output->lsc + i * 4+ 0) = lsc_param_tmp[2];
				*((cmr_u16 *)awb_output->lsc + i * 4+ 1) = lsc_param_tmp[3];
				*((cmr_u16 *)awb_output->lsc + i * 4+ 2) = lsc_param_tmp[0];
				*((cmr_u16 *)awb_output->lsc + i * 4+ 3) = lsc_param_tmp[1];
			}
			break;
		}
		case SENSOR_IMAGE_PATTERN_RAWRGB_B:
		{
			for (i = 0; i < awb_output->lsc_size / 8; i++) {
				lsc_param_tmp[0] = *((cmr_u16 *)awb_output->lsc + i * 4+ 0);
				lsc_param_tmp[1] = *((cmr_u16 *)awb_output->lsc + i * 4+ 1);
				lsc_param_tmp[2] = *((cmr_u16 *)awb_output->lsc + i * 4+ 2);
				lsc_param_tmp[3] = *((cmr_u16 *)awb_output->lsc + i * 4+ 3);
				*((cmr_u16 *)awb_output->lsc + i * 4+ 0) = lsc_param_tmp[1];
				*((cmr_u16 *)awb_output->lsc + i * 4+ 1) = lsc_param_tmp[0];
				*((cmr_u16 *)awb_output->lsc + i * 4+ 2) = lsc_param_tmp[3];
				*((cmr_u16 *)awb_output->lsc + i * 4+ 3) = lsc_param_tmp[2];
			}
			break;
		}
		case SENSOR_IMAGE_PATTERN_RAWRGB_GB:
			break;
		default:
			break;
		}

		BLOCK_PARAM_CFG(pm_input, pm_data,
				ISP_PM_BLK_LSC_MEM_ADDR,
				ISP_BLK_2D_LSC, awb_output->lsc,
				awb_output->lsc_size);
		ret = isp_pm_ioctl(cxt->handle_pm,
				ISP_PM_CMD_SET_OTHERS, &pm_input, &pm_output);
		ISP_TRACE_IF_FAIL(ret, ("fail to set isp block param"));

		cxt->awb_cxt.log_alc_lsc = awb_output->log_lsc.log;
		cxt->awb_cxt.log_alc_lsc_size = awb_output->log_lsc.size;
	}

	if (ISP_SUCCESS == ret) {
		cxt->awb_cxt.alc_awb = awb_output->use_ccm | (awb_output->use_lsc << 8);
	}

exit:
	ISP_LOGV("done %ld", ret);
	return ret;
}

cmr_int ispalg_start_awb_process(cmr_handle isp_alg_handle,
				struct ae_ctrl_callback_in *ae_in,
				struct awb_ctrl_calc_result * awb_output)
{
	cmr_int ret = ISP_SUCCESS;
	nsecs_t time_start = 0;
	nsecs_t time_end = 0;
	struct awb_ctrl_calc_param param;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;

	memset((void *)&param, 0, sizeof(param));

	ret = ispalg_awb_pre_process((cmr_handle) cxt, ae_in, &param);

	time_start = ispalg_get_sys_timestamp();
	if (cxt->ops.awb_ops.process) {
		ret = cxt->ops.awb_ops.process(cxt->awb_cxt.handle, &param, awb_output);
		ISP_TRACE_IF_FAIL(ret, ("fail to do awb process"));
	}
	time_end = ispalg_get_sys_timestamp();
	ISP_LOGV("SYSTEM_TEST-awb:%d ms", (cmr_u32)(time_end - time_start));

	if (cxt->ops.awb_ops.ioctrl) {
		ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle,
				AWB_CTRL_CMD_RESULT_INFO, (void *)awb_output, NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to AWB_CTRL_CMD_GET_GAIN"));
	}

	ret = ispalg_awb_post_process((cmr_handle) cxt, awb_output);

exit:
	ISP_LOGV("done %ld", ret);
	return ret;
}

static cmr_int ispalg_aeawb_post_process(cmr_handle isp_alg_handle,
					 struct ae_ctrl_callback_in *ae_in,
					 struct awb_ctrl_calc_result *awb_output)
{
	cmr_int ret = ISP_SUCCESS;
	int i, num;
	nsecs_t time_start = 0;
	nsecs_t time_end = 0;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct smart_proc_input smart_proc_in;
	struct ae_monitor_info info;
	struct afctrl_ae_info *ae_info;
	struct afctrl_awb_info *awb_info;
	struct awb_size stat_img_size;
	struct awb_size win_size;

	memset(&info, 0, sizeof(info));
	memset(&smart_proc_in, 0, sizeof(smart_proc_in));
	CMR_MSG_INIT(message);

	time_start = ispalg_get_sys_timestamp();
	if (1 == cxt->smart_cxt.isp_smart_eb) {
		ISP_LOGV("bv:%d exp_line:%d again:%d cur_lum:%d target_lum:%d FlashEnvRatio:%f Flash1ofALLRatio:%f",
			ae_in->ae_output.cur_bv,
			ae_in->ae_output.cur_exp_line,
			ae_in->ae_output.cur_again,
			ae_in->ae_output.cur_lum,
			ae_in->ae_output.target_lum,
			ae_in->flash_param.captureFlashEnvRatio,
			ae_in->flash_param.captureFlash1ofALLRatio);
		if (!cxt->smart_cxt.sw_bypass) {
			smart_proc_in.cal_para.bv = ae_in->ae_output.cur_bv;
			smart_proc_in.cal_para.bv_gain = ae_in->ae_output.cur_again;
			smart_proc_in.cal_para.flash_ratio = ae_in->flash_param.captureFlashEnvRatio * 256;
			smart_proc_in.cal_para.flash_ratio1 = ae_in->flash_param.captureFlash1ofALLRatio * 256;
			smart_proc_in.cal_para.ct = awb_output->ct;
			smart_proc_in.alc_awb = cxt->awb_cxt.alc_awb;
			smart_proc_in.mode_flag = cxt->commn_cxt.mode_flag;
			smart_proc_in.scene_flag = cxt->commn_cxt.scene_flag;
			smart_proc_in.lock_nlm = cxt->smart_cxt.lock_nlm_en;
			smart_proc_in.lock_ee = cxt->smart_cxt.lock_ee_en;
			smart_proc_in.lock_precdn = cxt->smart_cxt.lock_precdn_en;
			smart_proc_in.lock_cdn = cxt->smart_cxt.lock_cdn_en;
			smart_proc_in.lock_postcdn = cxt->smart_cxt.lock_postcdn_en;
			smart_proc_in.lock_ccnr = cxt->smart_cxt.lock_ccnr_en;
			smart_proc_in.lock_ynr = cxt->smart_cxt.lock_ynr_en;
			smart_proc_in.cal_para.gamma_tab = cxt->smart_cxt.tunning_gamma_cur;
			isp_prepare_atm_param(isp_alg_handle, &smart_proc_in);

			num = (cxt->zsl_flag) ? 2 : 1;
			for (i = 0; i < num; i++) {
				if (i == 0)
					smart_proc_in.mode_flag = cxt->pm_mode_cxt.mode_id;
				else
					smart_proc_in.mode_flag = cxt->pm_mode_cxt.cap_mode_id;
				if (cxt->ops.smart_ops.ioctrl) {
					ret = cxt->ops.smart_ops.ioctrl(cxt->smart_cxt.handle,
								ISP_SMART_IOCTL_SET_WORK_MODE,
								(void *)&smart_proc_in.mode_flag, NULL);
					ISP_TRACE_IF_FAIL(ret, ("fail to ISP_SMART_IOCTL_SET_WORK_MODE"));
				}
				if (cxt->ops.smart_ops.calc) {
					ret = cxt->ops.smart_ops.calc(cxt->smart_cxt.handle, &smart_proc_in);
					ISP_TRACE_IF_FAIL(ret, ("fail to do _smart_calc"));
				}
			}

			cxt->smart_cxt.log_smart = smart_proc_in.log;
			cxt->smart_cxt.log_smart_size = smart_proc_in.size;
		}

		if (!cxt->lsc_cxt.sw_bypass) {
			stat_img_size.w = ae_in->monitor_info.win_num.w;
			stat_img_size.h = ae_in->monitor_info.win_num.h;
			win_size.w = ae_in->monitor_info.win_num.w;
			win_size.h = ae_in->monitor_info.win_num.h;

			ret = ispalg_alsc_calc(isp_alg_handle,
					       cxt->aem_stats_data.r_info,
					       cxt->aem_stats_data.g_info,
					       cxt->aem_stats_data.b_info,
					       &stat_img_size, &win_size,
					       cxt->commn_cxt.src.w, cxt->commn_cxt.src.h,
					       awb_output->ct, awb_output->gain.r, awb_output->gain.b,
					       ae_in->ae_output.is_stab, ae_in);
			ISP_TRACE_IF_FAIL(ret, ("fail to do alsc_calc"));
		}
	}
	time_end = ispalg_get_sys_timestamp();
	ISP_LOGV("SYSTEM_TEST-smart:%d ms", (cmr_u32)(time_end - time_start));

	ae_info = &cxt->ae_info;
	awb_info = &cxt->awb_info;

	if (0 == ae_info->is_update) {
		memset((void *)ae_info, 0, sizeof(struct afctrl_ae_info));
		ae_info->img_blk_info.block_w = cxt->ae_cxt.win_num.w;
		ae_info->img_blk_info.block_h = cxt->ae_cxt.win_num.h;
		ae_info->img_blk_info.chn_num = 3;
		ae_info->img_blk_info.pix_per_blk = 1;
		ae_info->img_blk_info.data = (cmr_u32 *) &cxt->aem_stats_data;
		ae_info->ae_rlt_info.bv = ae_in->ae_output.cur_bv;
		ae_info->ae_rlt_info.is_stab = ae_in->ae_output.is_stab;
		ae_info->ae_rlt_info.cur_exp_line = ae_in->ae_output.cur_exp_line;
		ae_info->ae_rlt_info.cur_dummy = ae_in->ae_output.cur_dummy;
		ae_info->ae_rlt_info.frame_line = ae_in->ae_output.frame_line;
		ae_info->ae_rlt_info.line_time = ae_in->ae_output.line_time;
		ae_info->ae_rlt_info.cur_again = ae_in->ae_output.cur_again;
		ae_info->ae_rlt_info.cur_dgain = ae_in->ae_output.cur_dgain;
		ae_info->ae_rlt_info.cur_lum = ae_in->ae_output.cur_lum;
		ae_info->ae_rlt_info.target_lum = ae_in->ae_output.target_lum;
		ae_info->ae_rlt_info.target_lum_ori = ae_in->ae_output.target_lum_ori;
		ae_info->ae_rlt_info.flag4idx = ae_in->ae_output.flag4idx;
		ae_info->ae_rlt_info.face_stable= ae_in->ae_output.face_stable;
		ae_info->ae_rlt_info.cur_ev = ae_in->ae_output.cur_ev;
		ae_info->ae_rlt_info.cur_index = ae_in->ae_output.cur_index;
		ae_info->ae_rlt_info.cur_iso = ae_in->ae_output.cur_iso;
		ae_info->is_update = 1;
	}

	if (0 == awb_info->is_update) {
		memset((void *)awb_info, 0, sizeof(struct afctrl_awb_info));
		awb_info->r_gain = awb_output->gain.r;
		awb_info->g_gain = awb_output->gain.g;
		awb_info->b_gain = awb_output->gain.b;
		awb_info->is_update = 1;
	}
	message.msg_type = ISP_EVT_AF;
	message.sub_msg_type = AF_DATA_IMG_BLK;
	message.sync_flag = CMR_MSG_SYNC_NONE;
	ret = cmr_thread_msg_send(cxt->thr_afhandle, &message);
	if (ret) {
		ISP_LOGE("fail to send evt af, ret %ld", ret);
		free(message.data);
	}

exit:
	ISP_LOGV("done ret %ld", ret);
	return ret;
}

cmr_int ispalg_ae_process(cmr_handle isp_alg_handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;

	if (cxt->ae_cxt.sw_bypass) {
		return ret;
	}
	if (!cxt->aem_is_update) {
		ISP_LOGV("aem is not update\n");
		return ret;
	}

	ret = ispalg_start_ae_process((cmr_handle) cxt);

	return ret;
}

static cmr_int ispalg_awb_process(cmr_handle isp_alg_handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct awb_ctrl_calc_result awb_output;
	struct ae_calc_results ae_result;
	struct ae_ctrl_callback_in ae_ctrl_calc_result;

	if (cxt->awb_cxt.sw_bypass)
		return ret;

	if (!cxt->aem_is_update) {
		ISP_LOGV("aem is not update\n");
		goto exit;
	}

	memset(&awb_output, 0, sizeof(awb_output));
	memset(&ae_result, 0, sizeof(ae_result));
	memset(&ae_ctrl_calc_result, 0, sizeof(ae_ctrl_calc_result));

	if (cxt->ops.ae_ops.ioctrl) {
		cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_GET_CALC_RESULTS, NULL, &ae_result);
		ae_ctrl_calc_result.is_skip_cur_frame = ae_result.is_skip_cur_frame;
		ae_ctrl_calc_result.ae_output = ae_result.ae_output;
		ae_ctrl_calc_result.ae_result = ae_result.ae_result;
		ae_ctrl_calc_result.ae_ev = ae_result.ae_ev;
		ae_ctrl_calc_result.monitor_info = ae_result.monitor_info;
		ae_ctrl_calc_result.flash_param.captureFlashEnvRatio = ae_result.flash_param.captureFlashEnvRatio;
		ae_ctrl_calc_result.flash_param.captureFlash1ofALLRatio = ae_result.flash_param.captureFlash1ofALLRatio;
	}

	ret = ispalg_start_awb_process((cmr_handle) cxt, &ae_ctrl_calc_result, &awb_output);
	if (ret) {
		ISP_LOGE("fail to start awb process");
		goto exit;
	}

	ret = ispalg_aeawb_post_process((cmr_handle) cxt, &ae_ctrl_calc_result, &awb_output);
	if (ret) {
		ISP_LOGE("fail to proc aeawb ");
		goto exit;
	}
exit:
	return ret;
}

cmr_int ispalg_afl_process(cmr_handle isp_alg_handle, void *data)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_int bypass = 0;
	cmr_u32 cur_flicker = 0;
	cmr_u32 cur_exp_flag = 0;
	cmr_s32 ae_exp_flag = 0;
	float ae_exp = 0.0;
	struct isp_statis_info *statis_info = (struct isp_statis_info *)data;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct afl_proc_in afl_input;
	struct afl_ctrl_proc_out afl_output;
	struct isp_pm_param_data pm_afl_data;
	struct isp_pm_ioctl_input pm_afl_input = {NULL, 0};
	struct isp_pm_ioctl_output pm_afl_output = {NULL, 0};

	memset(&pm_afl_data, 0, sizeof(pm_afl_data));
	memset(&afl_input, 0, sizeof(afl_input));
	memset(&afl_output, 0, sizeof(afl_output));

	if (cxt->afl_cxt.sw_bypass) {
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_STSTIS_BUF, statis_info, NULL);
		if (ret) {
			ISP_LOGE("fail to set statis buf");
		}
		if(cxt->afl_cxt.version) {
			cmr_u32 bypass = 0;
			ret = isp_dev_access_ioctl(cxt->dev_access_handle,
						ISP_DEV_SET_AFL_NEW_BYPASS,
						&bypass, NULL);
		}
		goto exit;
	}

	bypass = 1;
	if(cxt->afl_cxt.version) {
		ret = isp_dev_access_ioctl(cxt->dev_access_handle,
						ISP_DEV_SET_AFL_NEW_BYPASS,
						&bypass, NULL);
	}

	if (cxt->ops.ae_ops.ioctrl) {
		ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_GET_FLICKER_MODE, NULL, &cur_flicker);
		ISP_TRACE_IF_FAIL(ret, ("fail to AE_GET_FLICKER_MODE"));
		ISP_LOGV("cur flicker mode %d", cur_flicker);

		ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_GET_EXP, NULL, &ae_exp);
		ISP_TRACE_IF_FAIL(ret, ("fail to AE_GET_EXP"));

		if (fabs(ae_exp - 0.04) < 0.000001 || ae_exp > 0.04) {
			ae_exp_flag = 1;
		}
		ISP_LOGV("ae_exp %f; ae_exp_flag %d", ae_exp, ae_exp_flag);

		ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_GET_FLICKER_SWITCH_FLAG, &cur_exp_flag, NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to AE_GET_FLICKER_SWITCH_FLAG"));
		ISP_LOGV("cur exposure flag %d", cur_exp_flag);
	}
	BLOCK_PARAM_CFG(pm_afl_input, pm_afl_data,
			ISP_PM_BLK_ISP_SETTING,
			ISP_BLK_ANTI_FLICKER, NULL, 0);
	ret = isp_pm_ioctl(cxt->handle_pm,
			ISP_PM_CMD_GET_SINGLE_SETTING,
			&pm_afl_input, &pm_afl_output);
	if (ISP_SUCCESS == ret && 1 == pm_afl_output.param_num) {
		afl_input.afl_param_ptr = (struct isp_antiflicker_param *)pm_afl_output.param_data->data_ptr;
		afl_input.pm_param_num = pm_afl_output.param_num;
	} else {
		afl_input.afl_param_ptr = NULL;
		afl_input.pm_param_num = 0;
	}

	afl_input.ae_stat_ptr = &cxt->aem_stats_data;
	afl_input.ae_exp_flag = ae_exp_flag;
	afl_input.cur_exp_flag = cur_exp_flag;
	afl_input.cur_flicker = cur_flicker;
	afl_input.vir_addr = statis_info->uaddr;
	afl_input.afl_mode = cxt->afl_cxt.afl_mode;
	afl_input.private_len = sizeof(struct isp_statis_info);
	afl_input.private_data = statis_info;

	if (cxt->ops.afl_ops.process) {
		ret = cxt->ops.afl_ops.process(cxt->afl_cxt.handle, &afl_input, &afl_output);
		ISP_TRACE_IF_FAIL(ret, ("fail to afl process"));
	}

	if (cxt->afl_cxt.afl_mode > AE_FLICKER_60HZ)
		bypass = 0;
	else
		bypass = 1;

	if(cxt->afl_cxt.version) {
		ret = isp_dev_access_ioctl(cxt->dev_access_handle,
						ISP_DEV_SET_AFL_NEW_BYPASS,
						&bypass, NULL);
	}

exit:
	ISP_LOGV("done ret %ld", ret);
	return ret;
}

static cmr_int ispalg_af_process(cmr_handle isp_alg_handle, cmr_u32 data_type, void *in_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct afctrl_calc_in calc_param;
	struct afctrl_calc_out calc_result;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;

	memset((void *)&calc_param, 0, sizeof(calc_param));
	memset((void *)&calc_result, 0, sizeof(calc_result));
	ISP_LOGV("begin data_type %d", data_type);

	switch (data_type) {
	case AF_DATA_AFM_STAT:
	case AF_DATA_AF: {
			cmr_u32 i = 0;
			cmr_u32 blk_num;
			cmr_u32 af_temp[15*20][3];
			cmr_u32 *ptr;
			struct isp_statis_info *statis_info = NULL;

			statis_info = (struct isp_statis_info *)in_ptr;
			ptr = (cmr_u32 *)statis_info->uaddr;
			blk_num = cxt->af_cxt.win_num.x * cxt->af_cxt.win_num.y;
			for (i = 0; i < blk_num; i++) {
				af_temp[i][0] = *ptr++;
				af_temp[i][1] = *ptr++;
				af_temp[i][2] = *ptr++;
				ptr++;
			}
			ISP_LOGD("data: %x %x %x\n", af_temp[0][0], af_temp[0][1], af_temp[0][2]);

			calc_param.data_type = AF_DATA_AF;
			calc_param.data = (void *)(af_temp);
			if (cxt->ops.af_ops.process && !cxt->af_cxt.sw_bypass) {
				ret = cxt->ops.af_ops.process(cxt->af_cxt.handle, (void *)&calc_param, &calc_result);
				ISP_TRACE_IF_FAIL(ret, ("fail to af process"));
			}

			/* send data to pd adpt layer */
			struct pdaf_ctrl_param_in pdaf_in;
			pdaf_in.af_addr = (void *)(af_temp);
			pdaf_in.af_addr_len = sizeof(af_temp);
			if (cxt->ops.pdaf_ops.process && !cxt->pdaf_cxt.sw_bypass) {
				ret = cxt->ops.pdaf_ops.ioctrl(cxt->pdaf_cxt.handle, PDAF_CTRL_CMD_SET_AFMFV, (void *)&pdaf_in, NULL);
			}

			ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_STSTIS_BUF, statis_info, NULL);
			if (ret) {
				ISP_LOGE("fail to set statis buf");
			}
			break;
		}
	case AF_DATA_IMG_BLK:
		if (cxt->af_cxt.sw_bypass)
			break;

		if (cxt->ops.af_ops.ioctrl) {
			struct afctrl_ae_info *ae_info = &cxt->ae_info;
			struct afctrl_awb_info *awb_info = &cxt->awb_info;
			if (1 == ae_info->is_update) {
				ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle, AF_CMD_SET_AE_INFO, (void *)ae_info, NULL);
				ISP_TRACE_IF_FAIL(ret, ("fail to AF_CMD_SET_AE_INFO"));
				ae_info->is_update = 0;
			}

			if (awb_info->is_update) {
				ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle, AF_CMD_SET_AWB_INFO, (void *)awb_info, NULL);
				ISP_TRACE_IF_FAIL(ret, ("fail to AF_CMD_SET_AWB_INFO"));
				awb_info->is_update = 0;
			}
		}
		calc_param.data_type = AF_DATA_IMG_BLK;
		if (cxt->ops.af_ops.process) {
			ret = cxt->ops.af_ops.process(cxt->af_cxt.handle, (void *)&calc_param, (void *)&calc_result);
		}
		break;
	case AF_DATA_AE:
		break;
	case AF_DATA_FD:
		break;
	default:
		break;
	}

	ISP_LOGV("done %ld", ret);
	return ret;
}

static cmr_int ispalg_pdaf_process(cmr_handle isp_alg_handle, cmr_u32 data_type, void *in_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_uint u_addr = 0;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_statis_info *statis_info = (struct isp_statis_info *)in_ptr;
	struct pdaf_ctrl_process_in pdaf_param_in;
	struct pdaf_ctrl_param_out pdaf_param_out;
	UNUSED(data_type);

	memset((void *)&pdaf_param_in, 0x00, sizeof(pdaf_param_in));
	memset((void *)&pdaf_param_out, 0x00, sizeof(pdaf_param_out));

	u_addr = statis_info->uaddr;
	pdaf_param_in.u_addr = statis_info->uaddr;
	if (SENSOR_PDAF_TYPE3_ENABLE == cxt->pdaf_cxt.pdaf_support){
		if (cxt->ops.pdaf_ops.ioctrl)
			cxt->ops.pdaf_ops.ioctrl(cxt->pdaf_cxt.handle, PDAF_CTRL_CMD_GET_BUSY, NULL, &pdaf_param_out);
		ISP_LOGV("pdaf_is_busy=%d\n", pdaf_param_out.is_busy);
		if (!pdaf_param_out.is_busy && !cxt->pdaf_cxt.sw_bypass) {
			if (cxt->ops.pdaf_ops.process)
				ret = cxt->ops.pdaf_ops.process(cxt->pdaf_cxt.handle, &pdaf_param_in, NULL);
		}
	} else if (SENSOR_PDAF_TYPE1_ENABLE == cxt->pdaf_cxt.pdaf_support){
		void *pdaf_info = (cmr_s32 *)(u_addr + ISP_PDAF_STATIS_BUF_SIZE / 2);

		if (cxt->ops.af_ops.ioctrl) {
			ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle, AF_CMD_SET_TYPE1_PD_INFO, pdaf_info, NULL);
			ISP_TRACE_IF_FAIL(ret, ("fail to AF_CMD_SET_TYPE1_PD_INFO"));
		}
	} else if (SENSOR_PDAF_TYPE2_ENABLE == cxt->pdaf_cxt.pdaf_support) {
		if (cxt->ops.pdaf_ops.ioctrl)
			cxt->ops.pdaf_ops.ioctrl(cxt->pdaf_cxt.handle, PDAF_CTRL_CMD_GET_BUSY, NULL, &pdaf_param_out);
		ISP_LOGV("pdaf_is_busy=%d\n", pdaf_param_out.is_busy);
		if (!pdaf_param_out.is_busy && !cxt->pdaf_cxt.sw_bypass) {
			if (cxt->ops.pdaf_ops.process)
				ret = cxt->ops.pdaf_ops.process(cxt->pdaf_cxt.handle, &pdaf_param_in, NULL);
		}
	}else if (SENSOR_DUAL_PDAF_ENABLE == cxt->pdaf_cxt.pdaf_support) {
		if (cxt->ops.pdaf_ops.ioctrl)
			cxt->ops.pdaf_ops.ioctrl(cxt->pdaf_cxt.handle, PDAF_CTRL_CMD_GET_BUSY, NULL, &pdaf_param_out);
		ISP_LOGV("pdaf_is_busy=%d\n", pdaf_param_out.is_busy);
		if (!pdaf_param_out.is_busy && !cxt->pdaf_cxt.sw_bypass) {
			if (cxt->ops.pdaf_ops.process)
				ret = cxt->ops.pdaf_ops.process(cxt->pdaf_cxt.handle, &pdaf_param_in, NULL);
		}
	}

	ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_STSTIS_BUF, statis_info, NULL);

	ISP_LOGV("done %ld, statis_info->uaddr = 0x%lx", ret, statis_info->uaddr);
	return ret;
}

static cmr_int ispalg_ebd_process(cmr_handle isp_alg_handle, cmr_u32 data_type, void *in_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_statis_info *statis_info = (struct isp_statis_info *)in_ptr;
	UNUSED(data_type);

	if (cxt->ebd_cxt.ebd_support){
		//isp_file_ebd_save_info(cxt->handle_file_debug, statis_info);
	}

	ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_STSTIS_BUF, statis_info, NULL);

	return ret;
}

static cmr_int ispalg_evt_process_cb(cmr_handle isp_alg_handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct ips_out_param callback_param = { 0x00 };

	ISP_LOGV("isp start raw proc callback\n");
	if (NULL != cxt->commn_cxt.callback) {
		callback_param.output_height = cxt->commn_cxt.src.h;
		ISP_LOGV("callback ISP_PROC_CALLBACK");
		cxt->commn_cxt.callback(cxt->commn_cxt.caller_id,
					ISP_CALLBACK_EVT | ISP_PROC_CALLBACK,
					(void *)&callback_param,
					sizeof(struct ips_out_param));
	}

	ISP_LOGV("isp end raw proc callback\n");
	return ret;
}

cmr_int ispalg_afthread_proc(struct cmr_msg *message, void *p_data)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)p_data;

	if (!message || !p_data) {
		ISP_LOGE("fail to check input param ");
		goto exit;
	}
	ISP_LOGV("message.msg_type 0x%x, data %p", message->msg_type, message->data);

	switch (message->msg_type) {
	case ISP_EVT_AF:
		ret = ispalg_af_process((cmr_handle) cxt, message->sub_msg_type, message->data);
		break;
	default:
		ISP_LOGV("don't support msg");
		break;
	}
exit:
	ISP_LOGV("done %ld", ret);
	return ret;

}

cmr_int ispalg_thread_proc(struct cmr_msg *message, void *p_data)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)p_data;

	if (!message || !p_data) {
		ISP_LOGE("fail to check input param ");
		goto exit;
	}
	ISP_LOGV("message.msg_type 0x%x, data %p", message->msg_type, message->data);

	switch (message->msg_type) {
	case ISP_EVT_TX:
		ret = ispalg_evt_process_cb((cmr_handle) cxt);
		break;
	case ISP_EVT_AE:
		ret = ispalg_aem_stats_parser((cmr_handle) cxt, message->data);
		//isp_br_ioctrl(cxt->camera_id, SET_STAT_AWB_DATA, &cxt->aem_stats, NULL);
		break;
	case ISP_EVT_SOF:
		ret = ispalg_ae_process((cmr_handle) cxt);
		if (ret)
			ISP_LOGE("fail to start ae process");
		ret = ispalg_awb_process((cmr_handle) cxt);
		if (ret)
			ISP_LOGE("fail to start awb process");
		cxt->aem_is_update = 0;
		ret = ispalg_handle_sensor_sof((cmr_handle) cxt);
		break;
	case ISP_EVT_AFL:
		ret = ispalg_afl_process((cmr_handle) cxt, message->data);
		break;
	case ISP_EVT_PDAF:
		ret = ispalg_pdaf_process((cmr_handle) cxt, message->sub_msg_type, message->data);
		break;
	case ISP_EVT_EBD:
		ret = ispalg_ebd_process((cmr_handle) cxt, message->sub_msg_type, message->data);
		break;
	case ISP_EVT_HIST:
		ret = ispalg_hist_stats_parser((cmr_handle) cxt, message->data);
		break;
	case ISP_EVT_3DNR:
		ret = ispalg_3dnr_statis_parser((cmr_handle) cxt, message->data);
	default:
		ISP_LOGV("don't support msg");
		break;
	}
exit:
	ISP_LOGV("done %ld", ret);
	return ret;
}

void ispalg_dev_evt_msg(cmr_int evt, void *data, void *privdata)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)privdata;
	CMR_MSG_INIT(message);

	if (!cxt) {
		ISP_LOGE("fail to check input param");
		return;
	}

	message.msg_type = evt;
	message.sub_msg_type = 0;
	message.sync_flag = CMR_MSG_SYNC_NONE;
	message.alloc_flag = 1;
	message.data = data;
	if (ISP_EVT_AF == message.msg_type) {
		message.sub_msg_type = AF_DATA_AFM_STAT;
		ret = cmr_thread_msg_send(cxt->thr_afhandle, &message);
	} else {
		ret = cmr_thread_msg_send(cxt->thr_handle, &message);
	}
	if (ret) {
		ISP_LOGE("fail to send a message, evt is %ld, ret: %ld, cxt: %p, af_handle: %p, handle: %p",
			evt, ret, cxt, cxt->thr_afhandle, cxt->thr_handle);
		free(message.data);
	}
}


#if 0
void insert_data_q_tail(struct isp_event_node **head,
	struct isp_event_node *newnode)
{
	struct isp_event_node *cur;
	if (*head == NULL) {
		*head = newnode;
		newnode->next = NULL;
	} else {
		cur = *head;
		while (cur->next != NULL) {
			cur = cur->next;
		}
		cur->next = newnode;
		newnode->next = NULL;
	}
}

void delete_node_from_dataq(struct isp_event_node **head,
	struct isp_event_node *del_node)
{
	struct isp_event_node *cur, *prev;

	if (*head == NULL) {
		ISP_LOGE("error: data queue should not be empty.\n");
		return;
	} else if(del_node == *head) {
		*head = del_node->next;
		del_node->next = NULL;
	} else {
		prev = *head;
		cur = prev->next;
		while (cur!= NULL) {
			if (cur == del_node) {
				prev->next = cur->next;
				del_node->next = NULL;
				break;
			}
			prev = cur;
			cur = cur->next;
		}
	}
}



static void * ispalg_event_read(void *param)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = NULL;
	struct isp_event_node *pnode;
	struct isp_event_node *new_node, *prev_node;
	struct isp_event_info rd_data;

	cxt = (struct isp_alg_fw_context *)param;

	do {
		ISP_LOGD("wait for read start.\n");
		sem_wait(&cxt->read_start_sem);
		ISP_LOGD("start read event...\n");

		if (cxt->read_should_exit)
			break;

		while (1) {
			ret = ispdev_get_statis(cxt, &rd_data);
			if (ret) {
				ISP_LOGE("failed to get statis or irq event.\n");
				break;
			}
			if (rd_data.irq_type >= ISP_EVENT_MAX) {
				ISP_LOGE("unknown irq type %d\n", rd_data.irq_type);
				break;
			}

			prev_node = new_node = pnode = NULL;

			pthread_mutex_lock(&cxt->list_lock);
			pnode = cxt->event_list_head;
			while (pnode) {
				if (pnode->data.irq_type == rd_data.irq_type) {
					ISP_LOGD("unprocessed data of event: %d\n", pnode->data.irq_type);
					break;
				}
				pnode = pnode->next;
			}

			if (pnode != NULL) {
				delete_node_from_dataq(&cxt->event_list_head, pnode);
				prev_node = pnode;
			}

			new_node = malloc(sizeof(struct statis_event_node));
			if (new_node == NULL) {
				ISP_LOGE("fail to malloc memory.\n");
				ret = ISP_ALLOC_ERROR;
				pthread_mutex_unlock(&cxt->list_lock);
				break;
			}
			memcpy(&new_node->data, &rd_data, sizeof(struct isp_irq_info));
			insert_data_q_tail(&cxt->event_list_head, new_node);

			pthread_mutex_unlock(&cxt->list_lock);
			sem_post(&cxt->dataq_sem);

			/* free unprocess data */
			if (prev_node) {
				switch(prev_node->data.irq_type) {
				case ISP_EVENT_AEM_STATIS:
					break;
				default:
					break;
				}
				free(prev_node);
			}
		};
	} while (1);

	sem_post(&cxt->read_exit_sem);
	ISP_LOGD("exit.\n");
	return NULL;
}

void * ispalg_event_proc(void *data)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt;
	struct isp_event_node *pnode;
//	struct statis_event_node *new_node, *prev_node;

	cxt = (struct isp_alg_fw_context *)data;

	do {
		ISP_LOGD("wait for proc start.\n");
		sem_wait(&cxt->proc_start_sem);
		ISP_LOGD("start proc event...\n");

		if (cxt->proc_should_exit)
			break;

		while(1) {
			pnode = NULL;
			pthread_mutex_lock(&cxt->list_lock);
			pnode = cxt->event_list_head;
			if (pnode == NULL) {
				pthread_mutex_unlock(&cxt->list_lock);
				break;
			}
			delete_node_from_dataq(&cxt->event_list_head, pnode);
			pthread_mutex_unlock(&cxt->list_lock);

			switch(pnode->data.irq_type) {
			case ISP_EVENT_SOF:
				ret = ispalg_ae_process((cmr_handle) cxt);
				if (ret)
					ISP_LOGE("fail to start ae process");
				ret = ispalg_awb_process((cmr_handle) cxt);
				if (ret)
					ISP_LOGE("fail to start awb process");
				cxt->aem_is_update = 0;
				ret = ispalg_handle_sensor_sof((cmr_handle) cxt, (void *)&pnode->data);

				break;

			case ISP_EVENT_RAW_DONE:
				ret = ispalg_evt_process_cb((cmr_handle) cxt);
				break;

			case ISP_EVENT_AEM_STATIS:
				ret = ispalg_aem_stats_parser((cmr_handle) cxt, (void *)&pnode->data);
				isp_br_ioctrl(cxt->camera_id, SET_STAT_AWB_DATA, (void *)&cxt->aem_stats, NULL);

				break;

			case ISP_EVENT_AFM_STATIS:
				ret = ispalg_af_process((cmr_handle) cxt, AF_DATA_AFM_STAT, (void *)&pnode->data);
				break;

			case ISP_EVENT_AFL_STATIS:
				ret = ispalg_afl_process((cmr_handle) cxt, (void *)&pnode->data);
				break;

			case ISP_EVENT_HIST_STATIS:
				break;
			case ISP_EVENT_PDAF_STATIS:
				ret = ispalg_pdaf_process((cmr_handle) cxt, (void *)&pnode->data);
				break;

			default:
				break;
			}
			free(pnode);
		}
	} while (1);

	sem_post(&cxt->proc_exit_sem);
	ISP_LOGD("exit.\n");
	return NULL;
}
#endif

static cmr_int ispalg_create_thread(cmr_handle isp_alg_handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;

	ret = cmr_thread_create(&cxt->thr_handle,
			ISP_THREAD_QUEUE_NUM,
			ispalg_thread_proc, (void *)cxt);

	if (CMR_MSG_SUCCESS != ret) {
		ISP_LOGE("fail to create isp algfw  process thread");
		ret = -ISP_ERROR;
	}
	ret = cmr_thread_set_name(cxt->thr_handle, "algfw");
	if (CMR_MSG_SUCCESS != ret) {
		ISP_LOGE("fail to set fw name");
		ret = CMR_MSG_SUCCESS;
	}

	ret = cmr_thread_create(&cxt->thr_afhandle,
			ISP_THREAD_QUEUE_NUM,
			ispalg_afthread_proc, (void *)cxt);

	if (CMR_MSG_SUCCESS != ret) {
		ISP_LOGE("fail to create afstats process thread");
		ret = -ISP_ERROR;
	}
	ret = cmr_thread_set_name(cxt->thr_afhandle, "afstats");
	if (CMR_MSG_SUCCESS != ret) {
		ISP_LOGE("fail to set af name");
		ret = CMR_MSG_SUCCESS;
	}

#if 0 /* maybe optimized for data queue instead of message queue. */
	pthread_attr_t attr;
	/* thread for reading statis & IRQs */
	pthread_mutex_init(&cxt->list_lock, NULL);
	sem_init(&cxt->read_start_sem, 0, 0);
	sem_init(&cxt->read_exit_sem, 0, 0);
	sem_init(&cxt->proc_start_sem, 0, 0);
	sem_init(&cxt->proc_exit_sem, 0, 0);
	cxt->event_list_head = NULL;
	cxt->proc_should_exit = 0;
	cxt->read_should_exit = 0;

	if (1) {
		pthread_attr_t attr;

		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
		ret = pthread_create(&cxt->read_thr_handle,
						&attr,
						ispalg_event_read,
						(void *)cxt);
		pthread_setname_np(cxt->read_thr_handle, "ispevt_read");
		pthread_attr_destroy(&attr);
	}

	if (1) {
		pthread_attr_t attr;

		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
		ret = pthread_create(&cxt->proc_thr_handle,
						&attr,
						ispalg_event_proc,
						(void *)cxt);
		pthread_setname_np(cxt->proc_thr_handle, "ispevt_read");
		pthread_attr_destroy(&attr);
	}
#endif

	return ret;
}

static cmr_int ispalg_destroy_thread_proc(cmr_handle isp_alg_handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;

	if (cxt->thr_handle) {
		ret = cmr_thread_destroy(cxt->thr_handle);
		if (!ret) {
			cxt->thr_handle = (cmr_handle) NULL;
		} else {
			ISP_LOGE("fail to destroy isp algfw process thread");
		}
	}

	if (cxt->thr_afhandle) {
		ret = cmr_thread_destroy(cxt->thr_afhandle);
		if (!ret) {
			cxt->thr_afhandle = (cmr_handle) NULL;
		} else {
			ISP_LOGE("fail to destroy afstats process thread");
		}
	}

exit:
	ISP_LOGI("done %ld", ret);
	return ret;
}

static cmr_u32 ispalg_get_param_index(
		struct sensor_raw_resolution_info *input_size_trim,
		struct isp_size *size)
{
	cmr_u32 i;
	cmr_u32 param_index = 0x01;

	for (i = 0x01; i < ISP_INPUT_SIZE_NUM_MAX; i++) {
		if (size->h == input_size_trim[i].height) {
			param_index = i;
			break;
		}
	}

	return param_index;
}

static cmr_int ispalg_pm_init(cmr_handle isp_alg_handle, struct isp_init_param *input_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct sensor_raw_info *sensor_raw_info_ptr  = PNULL;
	struct isp_pm_init_input pm_init_input;
	struct isp_pm_init_output pm_init_output;
	cmr_u32 i = 0;

	memset(&pm_init_input, 0, sizeof(pm_init_input));
	memset(&pm_init_output, 0, sizeof(pm_init_output));

	sensor_raw_info_ptr = (struct sensor_raw_info *)input_ptr->setting_param_ptr;
	for (i = 0; i < MAX_MODE_NUM; i++) {
		pm_init_input.tuning_data[i].data_ptr = sensor_raw_info_ptr->mode_ptr[i].addr;
		pm_init_input.tuning_data[i].size = sensor_raw_info_ptr->mode_ptr[i].len;
		pm_init_input.fix_data[i] = sensor_raw_info_ptr->fix_ptr[i];
	}
	pm_init_input.nr_fix_info = &(sensor_raw_info_ptr->nr_fix);
	pm_init_input.sensor_raw_info_ptr = sensor_raw_info_ptr;

	cxt->handle_pm = isp_pm_init(&pm_init_input, &pm_init_output);
	if (PNULL == cxt->handle_pm) {
		ISP_LOGE("fail to do isp_pm_init");
		return ISP_ERROR;
	}
	cxt->sn_cxt.sn_raw_info = sensor_raw_info_ptr;
	cxt->commn_cxt.multi_nr_flag = pm_init_output.multi_nr_flag;
	cxt->commn_cxt.src.w = input_ptr->size.w;
	cxt->commn_cxt.src.h = input_ptr->size.h;
	cxt->commn_cxt.callback = input_ptr->ctrl_callback;
	cxt->commn_cxt.caller_id = input_ptr->oem_handle;
	cxt->commn_cxt.ops = input_ptr->ops;

	cxt->ioctrl_ptr = sensor_raw_info_ptr->ioctrl_ptr;
	if (cxt->ioctrl_ptr == NULL) {
		ISP_LOGW("Warning: sensor ioctrl is null.");
	}

	cxt->commn_cxt.image_pattern = sensor_raw_info_ptr->resolution_info_ptr->image_pattern;
	memcpy(cxt->commn_cxt.input_size_trim,
	       sensor_raw_info_ptr->resolution_info_ptr->tab,
	       ISP_INPUT_SIZE_NUM_MAX * sizeof(struct sensor_raw_resolution_info));
	cxt->commn_cxt.param_index =
		ispalg_get_param_index(cxt->commn_cxt.input_size_trim, &input_ptr->size);

	return ret;
}

static cmr_int ispalg_ae_init(struct isp_alg_fw_context *cxt)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_u32 num = 0;
	cmr_u32 i = 0;
	cmr_u32 dflash_num = 0;
	struct ae_init_in ae_input;
	struct isp_rgb_aem_info aem_info;
	struct isp_pm_ioctl_output output;
	struct isp_pm_param_data *param_data = NULL;
	struct ae_init_out result;

	memset((void *)&result, 0, sizeof(result));
	memset((void *)&ae_input, 0, sizeof(ae_input));

	memset(&output, 0, sizeof(output));
	ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_INIT_DUAL_FLASH, NULL, &output);
	if (ISP_SUCCESS != ret) {
		ISP_LOGE("fail to get dual flash param");
		return ret;
	}

	if (0 == output.param_num) {
		ISP_LOGE("fail to check param: dual flash param num=%d", output.param_num);
		return ISP_ERROR;
	}
	param_data = output.param_data;
	for (i = 0; i < output.param_num; i++) {
		if (NULL != param_data->data_ptr) {
			ae_input.flash_tuning[dflash_num].param = param_data->data_ptr;
			ae_input.flash_tuning[dflash_num].size = param_data->data_size;
			++dflash_num;
		}
		++param_data;
	}

	memset(&output, 0, sizeof(output));
	ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_AE_SYNC, NULL, &output);
	if (ISP_SUCCESS != ret || 1 != output.param_num)
		ISP_LOGW("warn: get ae sync param ret %ld, num %d\n", ret, output.param_num);

	param_data = output.param_data;
	if (output.param_num == 1 && param_data->data_ptr != NULL) {
		ae_input.ae_sync_param.param = param_data->data_ptr;
		ae_input.ae_sync_param.size = param_data->data_size;
	} else {
		ae_input.ae_sync_param.param = NULL;
		ae_input.ae_sync_param.size = 0;
	}

	memset(&output, 0, sizeof(output));
	ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_INIT_AE, NULL, &output);
	if (ISP_SUCCESS != ret || output.param_num == 0) {
		ISP_LOGE("fail to get ae init param");
		return ret;
	}

	param_data = output.param_data;
	if (param_data)
		ae_input.has_force_bypass = param_data->user_data[0];
	for (i = 0; i < output.param_num; i++) {
		if (NULL != param_data->data_ptr) {
			ae_input.param[num].param = param_data->data_ptr;
			ae_input.param[num].size = param_data->data_size;
			++num;
		}
		++param_data;
	}
	ae_input.param_num = num;
	ae_input.dflash_num = dflash_num;
	ae_input.resolution_info.frame_size.w = cxt->commn_cxt.src.w;
	ae_input.resolution_info.frame_size.h = cxt->commn_cxt.src.h;
	ae_input.resolution_info.frame_line = cxt->commn_cxt.input_size_trim[1].frame_line;
	ae_input.resolution_info.line_time = cxt->commn_cxt.input_size_trim[1].line_time;
	ae_input.resolution_info.sensor_size_index = 1;
	ae_input.camera_id = cxt->camera_id;
	ae_input.lib_param = cxt->lib_use_info->ae_lib_info;
	ae_input.caller_handle = (cmr_handle) cxt;
	ae_input.ae_set_cb = ispalg_ae_set_cb;

	ret = ispalg_get_aem_param(cxt, &aem_info);
	cxt->ae_cxt.win_num.w = aem_info.blk_num.w;
	cxt->ae_cxt.win_num.h = aem_info.blk_num.h;

	ae_input.monitor_win_num.w = cxt->ae_cxt.win_num.w;
	ae_input.monitor_win_num.h = cxt->ae_cxt.win_num.h;
	ae_input.sensor_role = cxt->is_master;
	switch (cxt->is_multi_mode) {
	case ISP_SINGLE:
		ae_input.is_multi_mode = ISP_ALG_SINGLE;
		break;
	case ISP_DUAL_NORMAL:
		ae_input.is_multi_mode = ISP_ALG_DUAL_C_C;
		break;
	case ISP_BOKEH:
		ae_input.is_multi_mode = ISP_ALG_DUAL_C_C;
		break;
	case ISP_WIDETELE:
		ae_input.is_multi_mode = ISP_ALG_DUAL_W_T;
		break;
	default:
		ae_input.is_multi_mode = ISP_ALG_SINGLE;
		break;
	}
	ISP_LOGI("sensor_role=%d, is_multi_mode=%d",
		cxt->is_master, cxt->is_multi_mode);

	if (cxt->is_multi_mode) {
		ae_input.otp_info_ptr = cxt->otp_data;
		ae_input.is_master = cxt->is_master;
	}

	ae_input.ptr_isp_br_ioctrl = isp_br_ioctrl;

	if (cxt->ops.ae_ops.init) {
		ret = cxt->ops.ae_ops.init(&ae_input, &cxt->ae_cxt.handle, (cmr_handle)&result);
		ISP_TRACE_IF_FAIL(ret, ("fail to do ae_ctrl_init"));
	}
	cxt->ae_cxt.flash_version = result.flash_ver;
	ISP_LOGD("done. %ld\n", ret);
	return ret;
}

static cmr_int ispalg_awb_init(struct isp_alg_fw_context *cxt)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_pm_ioctl_input input;
	struct isp_pm_ioctl_output output;
	struct awb_ctrl_init_param param;
	struct ae_monitor_info info;

	memset((void *)&input, 0, sizeof(input));
	memset((void *)&output, 0, sizeof(output));
	memset((void *)&param, 0, sizeof(param));
	memset((void *)&info, 0, sizeof(info));

	cxt->awb_cxt.cur_gain.r = 1;
	cxt->awb_cxt.cur_gain.b = 1;

	ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_INIT_AWB, &input, &output);
	ISP_TRACE_IF_FAIL(ret, ("fail to get awb init param"));

	if (cxt->ops.ae_ops.ioctrl) {
		ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_GET_MONITOR_INFO, NULL, (void *)&info);
		ISP_TRACE_IF_FAIL(ret, ("fail to get ae monitor info"));
	}

	param.camera_id = cxt->camera_id;
	param.base_gain = 1024;
	param.awb_enable = 1;
	param.wb_mode = 0;
	param.stat_img_format = AWB_CTRL_STAT_IMG_CHN;
	param.stat_img_size.w = info.win_num.w;
	param.stat_img_size.h = info.win_num.h;
	param.stat_win_size.w = info.win_size.w;
	param.stat_win_size.h = info.win_size.h;

	ISP_LOGE("awb get ae win %d %d %d %d %d %d\n", info.trim.x, info.trim.y,
		info.win_size.w, info.win_size.h, info.win_num.w, info.win_num.h);

	param.tuning_param = output.param_data->data_ptr;
	param.param_size = output.param_data->data_size;
	param.lib_param = cxt->lib_use_info->awb_lib_info;
	ISP_LOGV("param addr is %p size %d", param.tuning_param, param.param_size);

	param.otp_info_ptr = cxt->otp_data;
	param.is_master = cxt->is_master;
	param.sensor_role = cxt->is_master;

	switch (cxt->is_multi_mode) {
	case ISP_SINGLE:
		param.is_multi_mode = ISP_ALG_SINGLE;
		break;
	case ISP_DUAL_NORMAL:
		param.is_multi_mode = ISP_ALG_DUAL_C_C;
		break;
	case ISP_BOKEH:
		param.is_multi_mode = ISP_ALG_DUAL_C_C;
		break;
	case ISP_WIDETELE:
		param.is_multi_mode = ISP_ALG_DUAL_W_T;
		break;
	default:
		param.is_multi_mode = ISP_ALG_SINGLE;
		break;
	}
	ISP_LOGI("sensor_role=%d, is_multi_mode=%d",
		cxt->is_master, cxt->is_multi_mode);
	param.ptr_isp_br_ioctrl = isp_br_ioctrl;

	if (cxt->ops.awb_ops.init) {
		ret = cxt->ops.awb_ops.init(&param, &cxt->awb_cxt.handle);
		ISP_TRACE_IF_FAIL(ret, ("fail to do awb_ctrl_init"));
	}
	ISP_LOGI("done %ld", ret);
	return ret;
}

static cmr_int ispalg_afl_init(struct isp_alg_fw_context *cxt, struct isp_alg_sw_init_in *input_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct afl_ctrl_init_in afl_input;

	/*0:afl_old mode, 1:afl_new mode*/
	cxt->afl_cxt.version = 1;
	afl_input.dev_handle = cxt->dev_access_handle;
	afl_input.size.w = input_ptr->size.w;
	afl_input.size.h = input_ptr->size.h;
	afl_input.vir_addr = &cxt->afl_cxt.vir_addr;
	afl_input.caller_handle = (cmr_handle) cxt;
	afl_input.afl_set_cb = ispalg_afl_set_cb;
	afl_input.version = cxt->afl_cxt.version;
	if (cxt->ops.afl_ops.init)
		ret = cxt->ops.afl_ops.init(&cxt->afl_cxt.handle, &afl_input);
exit:
	ISP_LOGI("done %ld", ret);
	return ret;
}

static cmr_int ispalg_smart_init(struct isp_alg_fw_context *cxt)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_u32 i = 0;
	cmr_u32 mode;
	struct smart_init_param smart_init_param;
	struct isp_pm_ioctl_output pm_output;

	cxt->smart_cxt.isp_smart_eb = 0;
	memset(&smart_init_param, 0, sizeof(smart_init_param));
	memset(&pm_output, 0, sizeof(pm_output));

	ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_INIT_SMART, NULL, &pm_output);
	if (ISP_SUCCESS == ret) {
		for (i = 0; i < pm_output.param_num; ++i) {
			mode = pm_output.param_data[i].mode_id;
			smart_init_param.tuning_param[mode].data.size = pm_output.param_data[i].data_size;
			smart_init_param.tuning_param[mode].data.data_ptr = pm_output.param_data[i].data_ptr;
		}
	} else {
		ISP_LOGE("fail to get smart init param ");
		return ret;
	}
	smart_init_param.caller_handle = (cmr_handle)cxt;
	smart_init_param.smart_set_cb = ispalg_smart_set_cb;

	ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_ATM_PARAM, NULL, &pm_output);
	if (ISP_SUCCESS != ret || pm_output.param_num != 1) {
		ISP_LOGE("fail to get atm param");
		return ret;
	}
	if (pm_output.param_data->data_ptr) {
		smart_init_param.atm_info = pm_output.param_data->data_ptr;
		smart_init_param.atm_size = pm_output.param_data->data_size;
	}

	if (cxt->ops.smart_ops.init) {
		cxt->smart_cxt.handle = cxt->ops.smart_ops.init(&smart_init_param, NULL);
		if (NULL == cxt->smart_cxt.handle) {
			ISP_LOGE("fail to do smart init");
			return ret;
		}
	}
exit:
	ISP_LOGI("done %ld", ret);
	return ret;
}

static cmr_int ispalg_af_init(struct isp_alg_fw_context *cxt)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_u32 is_af_support = 1;
	struct afctrl_init_in af_input;
	struct af_log_info af_param = {NULL, 0};
	struct af_log_info aft_param = {NULL, 0};
	struct isp_pm_ioctl_output output = { NULL, 0 };

	if (NULL == cxt->ioctrl_ptr || NULL == cxt->ioctrl_ptr->set_pos)
		is_af_support = 0;

	memset((void *)&af_input, 0, sizeof(af_input));

	af_input.camera_id = cxt->camera_id;
	af_input.lib_param = cxt->lib_use_info->af_lib_info;
	af_input.caller_handle = (cmr_handle) cxt;
	af_input.af_set_cb = ispalg_af_set_cb;
	af_input.src.w = cxt->commn_cxt.src.w;
	af_input.src.h = cxt->commn_cxt.src.h;
	af_input.is_supoprt = is_af_support;

	if(1 == is_af_support) {
		//get af tuning parameters
		memset((void *)&output, 0, sizeof(output));
		ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_INIT_AF_NEW, NULL, &output);
		if(ISP_SUCCESS == ret && NULL != output.param_data){
			af_input.aftuning_data = output.param_data[0].data_ptr;
			af_input.aftuning_data_len = output.param_data[0].data_size;
		}

		//get af trigger tuning parameters
		memset((void *)&output, 0, sizeof(output));
		ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_INIT_AFT, NULL, &output);
		if(ISP_SUCCESS == ret && NULL != output.param_data){
			af_input.afttuning_data = output.param_data[0].data_ptr;
			af_input.afttuning_data_len = output.param_data[0].data_size;
		}

		//get pdaf tuning parameters
		memset((void *)&output, 0, sizeof(output));
		ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_INIT_PDAF, NULL, &output);
		if(ISP_SUCCESS == ret && 1 == output.param_num && NULL != output.param_data) {
			af_input.pdaftuning_data = output.param_data[0].data_ptr;
			af_input.pdaftuning_data_len = output.param_data[0].data_size;
		}
	}

	switch (cxt->is_multi_mode) {
	case ISP_SINGLE:
		af_input.is_multi_mode = AF_ALG_SINGLE;
		break;
	case ISP_DUAL_NORMAL:
		af_input.is_multi_mode = AF_ALG_DUAL_C_C;
		break;
	case ISP_BLUR_REAR:
		af_input.is_multi_mode = AF_ALG_BLUR_REAR;
		break;
	case ISP_BOKEH:
		af_input.is_multi_mode = AF_ALG_DUAL_C_C;
		break;
	case ISP_WIDETELE:
		af_input.is_multi_mode = AF_ALG_DUAL_W_T;
		break;
	default:
		af_input.is_multi_mode = AF_ALG_SINGLE;
		break;
	}

	ISP_LOGI("sensor_role=%d, is_multi_mode=%d",
		cxt->is_master, cxt->is_multi_mode);

	af_input.otp_info_ptr = cxt->otp_data;
	af_input.is_master = cxt->is_master;

	if (cxt->ops.af_ops.init) {
		ret = cxt->ops.af_ops.init(&af_input, &cxt->af_cxt.handle);
		ISP_TRACE_IF_FAIL(ret, ("fail to do af_ctrl_init"));
	}

	if (cxt->ops.af_ops.ioctrl) {
		ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle, AF_CMD_GET_AF_LOG_INFO, (void *)&af_param, NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to get_af_log_info"));
		cxt->af_cxt.log_af = af_param.log_cxt;
		cxt->af_cxt.log_af_size = af_param.log_len;
		ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle, AF_CMD_GET_AFT_LOG_INFO, (void *)&aft_param, NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to get_aft_log_info"));
		cxt->aft_cxt.log_aft = aft_param.log_cxt;
		cxt->aft_cxt.log_aft_size = aft_param.log_len;
	}

	return ret;
}

static cmr_int ispalg_pdaf_init(struct isp_alg_fw_context *cxt, struct isp_alg_sw_init_in *input_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct pdaf_ctrl_init_in pdaf_input;
	struct pdaf_ctrl_init_out pdaf_output;

	memset(&pdaf_input, 0x00, sizeof(pdaf_input));
	memset(&pdaf_output, 0x00, sizeof(pdaf_output));

	pdaf_input.camera_id = cxt->camera_id;
	pdaf_input.caller_handle = (cmr_handle) cxt;
	pdaf_input.pdaf_support = cxt->pdaf_cxt.pdaf_support;
	pdaf_input.pdaf_set_cb = ispalg_pdaf_set_cb;
	pdaf_input.pd_info = input_ptr->pdaf_info;
	pdaf_input.sensor_max_size = input_ptr->sensor_max_size;
	pdaf_input.handle_pm = cxt->handle_pm;

	if (SENSOR_PDAF_TYPE3_ENABLE == cxt->pdaf_cxt.pdaf_support ||
		SENSOR_PDAF_TYPE2_ENABLE == cxt->pdaf_cxt.pdaf_support) {
		pdaf_input.otp_info_ptr = cxt->otp_data;
		pdaf_input.is_master= cxt->is_master;
	}

	if (cxt->ops.pdaf_ops.init)
		ret = cxt->ops.pdaf_ops.init(&pdaf_input, &pdaf_output, &cxt->pdaf_cxt.handle);

exit:
	if (ret) {
		ISP_LOGE("fail to do PDAF initialize");
	}
	ISP_LOGI("done %ld", ret);

	return ret;
}

static cmr_int ispalg_lsc_init(struct isp_alg_fw_context *cxt)
{
	cmr_u32 ret = ISP_SUCCESS;
	cmr_s32 i = 0;
	cmr_handle pm_handle = cxt->handle_pm;
	lsc_adv_handle_t lsc_adv_handle = NULL;
	struct lsc_adv_init_param lsc_param;
	struct isp_2d_lsc_param *lsc_tab_param_ptr;
	struct isp_lsc_info *lsc_info;
	struct isp_pm_ioctl_input io_pm_input = { PNULL, 0 };
	struct isp_pm_ioctl_output io_pm_output = { PNULL, 0 };

	memset(&lsc_param, 0, sizeof(struct lsc_adv_init_param));
	memset(&io_pm_input, 0, sizeof(struct isp_pm_ioctl_input));
	memset(&io_pm_output, 0, sizeof(struct isp_pm_ioctl_output));

	ISP_LOGV(" _alsc_init");

	ret = ispalg_alsc_get_info(cxt);
	ISP_RETURN_IF_FAIL(ret, ("fail to do get lsc info"));
	lsc_tab_param_ptr = (struct isp_2d_lsc_param *)(cxt->lsc_cxt.lsc_tab_address);
	lsc_info = (struct isp_lsc_info *)cxt->lsc_cxt.lsc_info;

	ret = isp_pm_ioctl(pm_handle, ISP_PM_CMD_GET_INIT_ALSC, &io_pm_input, &io_pm_output);
	ISP_TRACE_IF_FAIL(ret, ("fail to do get init alsc"));

	if (io_pm_output.param_data->data_size != sizeof(struct lsc2_tune_param)) {
		lsc_param.tune_param_ptr = NULL;
		ISP_LOGE("fail to get alsc param from sensor file");
	} else {
		lsc_param.tune_param_ptr = io_pm_output.param_data->data_ptr;
	}

	lsc_param.otp_info_ptr = cxt->otp_data;
	lsc_param.is_master = cxt->is_master;

	for (i = 0; i < 9; i++) {
		lsc_param.lsc_tab_address[i] = lsc_tab_param_ptr->map_tab[i].param_addr;
	}

	lsc_param.img_height = lsc_tab_param_ptr->resolution.h;
	lsc_param.img_width = lsc_tab_param_ptr->resolution.w;
	lsc_param.gain_width = lsc_info->gain_w;
	lsc_param.gain_height = lsc_info->gain_h;
	lsc_param.lum_gain = (cmr_u16 *) lsc_info->data_ptr;
	lsc_param.grid = lsc_info->grid;
	lsc_param.camera_id = cxt->camera_id;
	lsc_param.lib_param = cxt->lib_use_info->lsc_lib_info;

	switch (cxt->commn_cxt.image_pattern) {
	case SENSOR_IMAGE_PATTERN_RAWRGB_GR:
		lsc_param.gain_pattern = LSC_GAIN_PATTERN_RGGB;
		break;

	case SENSOR_IMAGE_PATTERN_RAWRGB_R:
		lsc_param.gain_pattern = LSC_GAIN_PATTERN_GRBG;
		break;

	case SENSOR_IMAGE_PATTERN_RAWRGB_B:
		lsc_param.gain_pattern = LSC_GAIN_PATTERN_GBRG;
		break;

	case SENSOR_IMAGE_PATTERN_RAWRGB_GB:
		lsc_param.gain_pattern = LSC_GAIN_PATTERN_BGGR;
		break;

	default:
		break;
	}
#if 0
	lsc_param.output_gain_pattern = LSC_GAIN_PATTERN_BGGR;      //camdrv set output lsc pattern
	lsc_param.change_pattern_flag = 1;
	ISP_LOGV("alsc_init, gain_pattern=%d, output_gain_pattern=%d, flag=%d",
		lsc_param.gain_pattern, lsc_param.output_gain_pattern, lsc_param.change_pattern_flag);
#endif
	lsc_param.is_master = cxt->is_master;
	lsc_param.is_multi_mode = cxt->is_multi_mode;

	if (NULL == cxt->lsc_cxt.handle) {
		if (cxt->ops.lsc_ops.init) {
			ret = cxt->ops.lsc_ops.init(&lsc_param, &lsc_adv_handle);
			if (NULL == lsc_adv_handle) {
				ISP_LOGE("fail to do lsc adv init");
				return ISP_ERROR;
			}
		}
		cxt->lsc_cxt.handle = lsc_adv_handle;
	}

	return ret;
}

static cmr_int ispalg_bypass_init(struct isp_alg_fw_context *cxt)
{
	char value[PROPERTY_VALUE_MAX] = { 0x00 };
	cmr_u32 val;

	property_get(PROP_ISP_AE_BYPASS, value, "0");
	val = atoi(value);
	if (val < 2)
		cxt->ae_cxt.sw_bypass = val;

	property_get(PROP_ISP_AF_BYPASS, value, "0");
	val = atoi(value);
	if (val < 2)
		cxt->af_cxt.sw_bypass = val;

	property_get(PROP_ISP_AWB_BYPASS, value, "0");
	val = atoi(value);
	if (val < 2)
		cxt->awb_cxt.sw_bypass = val;

	property_get(PROP_ISP_LSC_BYPASS, value, "0");
	val = atoi(value);
	if (val < 2)
		cxt->lsc_cxt.sw_bypass = val;

	property_get(PROP_ISP_PDAF_BYPASS, value, "0");
	val = atoi(value);
	if (val < 2)
		cxt->pdaf_cxt.sw_bypass = val;

	property_get(PROP_ISP_AFL_BYPASS, value, "0");
	val = atoi(value);
	if (val < 2)
		cxt->afl_cxt.sw_bypass = val;

	property_get("persist.vendor.camera.bypass.smart", value, "0");
	val = atoi(value);
	if (val < 2)
		cxt->smart_cxt.sw_bypass = val;

	ISP_LOGI("ae sw bypass: %d\n", cxt->ae_cxt.sw_bypass);
	ISP_LOGI("af sw bypass: %d\n", cxt->af_cxt.sw_bypass);
	ISP_LOGI("awb sw bypass: %d\n", cxt->awb_cxt.sw_bypass);
	ISP_LOGI("lsc sw bypass: %d\n", cxt->lsc_cxt.sw_bypass);
	ISP_LOGI("afl sw bypass: %d\n", cxt->afl_cxt.sw_bypass);
	ISP_LOGI("pdaf sw bypass: %d\n", cxt->pdaf_cxt.sw_bypass);
	ISP_LOGI("smart sw bypass: %d\n", cxt->smart_cxt.sw_bypass);
	return ISP_SUCCESS;
}

static cmr_u32 ispalg_init(struct isp_alg_fw_context *cxt, struct isp_alg_sw_init_in *input_ptr)
{
	cmr_int ret = ISP_SUCCESS;

	ret = ispalg_ae_init(cxt);
	ISP_TRACE_IF_FAIL(ret, ("fail to do ae_init"));

	ret = ispalg_afl_init(cxt, input_ptr);
	ISP_RETURN_IF_FAIL(ret, ("fail to do afl_init"));

	ret = ispalg_awb_init(cxt);
	ISP_TRACE_IF_FAIL(ret, ("fail to do awb_init"));

	ret = ispalg_smart_init(cxt);
	ISP_TRACE_IF_FAIL(ret, ("fail to do smart_init"));

	ret = ispalg_af_init(cxt);
	ISP_TRACE_IF_FAIL(ret, ("fail to do af_init"));

	ret = ispalg_pdaf_init(cxt, input_ptr);
	ISP_TRACE_IF_FAIL(ret, ("fail to do pdaf_init"));

	ret = ispalg_lsc_init(cxt);
	ISP_TRACE_IF_FAIL(ret, ("fail to do lsc_init"));

	ret = ispalg_bypass_init(cxt);
	ISP_TRACE_IF_FAIL(ret, ("fail to do bypass_init"));

	ISP_LOGI("done %ld", ret);
	return ret;
}

static cmr_u32 ispalg_deinit(cmr_handle isp_alg_handle)
{
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;

	if (cxt->ops.pdaf_ops.deinit)
		cxt->ops.pdaf_ops.deinit(&cxt->pdaf_cxt.handle);

	if (cxt->ops.af_ops.deinit)
		cxt->ops.af_ops.deinit(&cxt->af_cxt.handle);

	if (cxt->ops.ae_ops.deinit)
		cxt->ops.ae_ops.deinit(&cxt->ae_cxt.handle);

	if (cxt->ops.lsc_ops.deinit)
		cxt->ops.lsc_ops.deinit(&cxt->lsc_cxt.handle);

	if (cxt->ops.smart_ops.deinit)
		cxt->ops.smart_ops.deinit(&cxt->smart_cxt.handle, NULL, NULL);

	if (cxt->ops.awb_ops.deinit)
		cxt->ops.awb_ops.deinit(&cxt->awb_cxt.handle);

	if (cxt->ops.afl_ops.deinit)
		cxt->ops.afl_ops.deinit(&cxt->afl_cxt.handle);

	ISP_LOGI("done");
	return ISP_SUCCESS;
}

static cmr_int ispalg_load_library(cmr_handle adpt_handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)adpt_handle;

	cxt->ispalg_lib_handle = dlopen(LIBCAM_ALG_FILE, RTLD_NOW);
	if (!cxt->ispalg_lib_handle) {
		ISP_LOGE("fail to dlopen");
		goto error_dlopen;
	}

	cxt->ops.af_ops.init = dlsym(cxt->ispalg_lib_handle, "af_ctrl_init");
	if (!cxt->ops.af_ops.init) {
		ISP_LOGE("fail to dlsym af_ops.init");
		goto error_dlsym;
	}
	cxt->ops.af_ops.deinit = dlsym(cxt->ispalg_lib_handle, "af_ctrl_deinit");
	if (!cxt->ops.af_ops.deinit) {
		ISP_LOGE("fail to dlsym af_ops.deinit");
		goto error_dlsym;
	}
	cxt->ops.af_ops.process = dlsym(cxt->ispalg_lib_handle, "af_ctrl_process");
	if (!cxt->ops.af_ops.process) {
		ISP_LOGE("fail to dlsym af_ops.process");
		goto error_dlsym;
	}
	cxt->ops.af_ops.ioctrl = dlsym(cxt->ispalg_lib_handle, "af_ctrl_ioctrl");
	if (!cxt->ops.af_ops.ioctrl) {
		ISP_LOGE("fail to dlsym af_ops.ioctrl");
		goto error_dlsym;
	}

	cxt->ops.afl_ops.init = dlsym(cxt->ispalg_lib_handle, "afl_ctrl_init");
	if (!cxt->ops.afl_ops.init) {
		ISP_LOGE("fail to dlsym afl_ops.init");
		goto error_dlsym;
	}
	cxt->ops.afl_ops.deinit = dlsym(cxt->ispalg_lib_handle, "afl_ctrl_deinit");
	if (!cxt->ops.afl_ops.deinit) {
		ISP_LOGE("fail to dlsym afl_ops.deinit");
		goto error_dlsym;
	}
	cxt->ops.afl_ops.process = dlsym(cxt->ispalg_lib_handle, "afl_ctrl_process");
	if (!cxt->ops.afl_ops.process) {
		ISP_LOGE("fail to dlsym afl_ops.process");
		goto error_dlsym;
	}
	cxt->ops.afl_ops.ioctrl= dlsym(cxt->ispalg_lib_handle, "afl_ctrl_ioctrl");
	if (!cxt->ops.afl_ops.ioctrl) {
		ISP_LOGE("fail to dlsym afl_ops.afl_ctrl_ioctrl");
		goto error_dlsym;
	}
	cxt->ops.afl_ops.config = dlsym(cxt->ispalg_lib_handle, "afl_ctrl_cfg");
	if (!cxt->ops.afl_ops.config) {
		ISP_LOGE("fail to dlsym afl_ops.cfg");
		goto error_dlsym;
	}
	cxt->ops.afl_ops.config_new = dlsym(cxt->ispalg_lib_handle, "aflnew_ctrl_cfg");
	if (!cxt->ops.afl_ops.config_new) {
		ISP_LOGE("fail to dlsym afl_ops.cfg_new");
		goto error_dlsym;
	}

	cxt->ops.ae_ops.init = dlsym(cxt->ispalg_lib_handle, "ae_ctrl_init");
	if (!cxt->ops.ae_ops.init) {
		ISP_LOGE("fail to dlsym ae_ops.init");
		goto error_dlsym;
	}
	cxt->ops.ae_ops.deinit = dlsym(cxt->ispalg_lib_handle, "ae_ctrl_deinit");
	if (!cxt->ops.ae_ops.deinit) {
		ISP_LOGE("fail to dlsym ae_ops.deinit");
		goto error_dlsym;
	}
	cxt->ops.ae_ops.process = dlsym(cxt->ispalg_lib_handle, "ae_ctrl_process");
	if (!cxt->ops.ae_ops.process) {
		ISP_LOGE("fail to dlsym ae_ops.process");
		goto error_dlsym;
	}
	cxt->ops.ae_ops.ioctrl = dlsym(cxt->ispalg_lib_handle, "ae_ctrl_ioctrl");
	if (!cxt->ops.ae_ops.ioctrl) {
		ISP_LOGE("fail to dlsym ae_ops.ioctrl");
		goto error_dlsym;
	}

	cxt->ops.awb_ops.init = dlsym(cxt->ispalg_lib_handle, "awb_ctrl_init");
	if (!cxt->ops.awb_ops.init) {
		ISP_LOGE("fail to dlsym awb_ops.init");
		goto error_dlsym;
	}
	cxt->ops.awb_ops.deinit = dlsym(cxt->ispalg_lib_handle, "awb_ctrl_deinit");
	if (!cxt->ops.awb_ops.deinit) {
		ISP_LOGE("fail to dlsym awb_ops.deinit");
		goto error_dlsym;
	}
	cxt->ops.awb_ops.process = dlsym(cxt->ispalg_lib_handle, "awb_ctrl_process");
	if (!cxt->ops.awb_ops.process) {
		ISP_LOGE("fail to dlsym awb_ops.process");
		goto error_dlsym;
	}
	cxt->ops.awb_ops.ioctrl = dlsym(cxt->ispalg_lib_handle, "awb_ctrl_ioctrl");
	if (!cxt->ops.awb_ops.ioctrl) {
		ISP_LOGE("fail to dlsym awb_ops.ioctrl");
		goto error_dlsym;
	}

	cxt->ops.pdaf_ops.init = dlsym(cxt->ispalg_lib_handle, "pdaf_ctrl_init");
	if (!cxt->ops.pdaf_ops.init) {
		ISP_LOGE("fail to dlsym pdaf_ops.init");
		goto error_dlsym;
	}
	cxt->ops.pdaf_ops.deinit = dlsym(cxt->ispalg_lib_handle, "pdaf_ctrl_deinit");
	if (!cxt->ops.pdaf_ops.deinit) {
		ISP_LOGE("fail to dlsym pdaf_ops.deinit");
		goto error_dlsym;
	}
	cxt->ops.pdaf_ops.process = dlsym(cxt->ispalg_lib_handle, "pdaf_ctrl_process");
	if (!cxt->ops.pdaf_ops.process) {
		ISP_LOGE("fail to dlsym pdaf_ops.process");
		goto error_dlsym;
	}
	cxt->ops.pdaf_ops.ioctrl = dlsym(cxt->ispalg_lib_handle, "pdaf_ctrl_ioctrl");
	if (!cxt->ops.pdaf_ops.ioctrl) {
		ISP_LOGE("fail to dlsym pdaf_ops.ioctrl");
		goto error_dlsym;
	}

	cxt->ops.smart_ops.init = dlsym(cxt->ispalg_lib_handle, "smart_ctl_init");
	if (!cxt->ops.smart_ops.init) {
		ISP_LOGE("fail to dlsym smart_ops.init");
		goto error_dlsym;
	}
	cxt->ops.smart_ops.deinit = dlsym(cxt->ispalg_lib_handle, "smart_ctl_deinit");
	if (!cxt->ops.smart_ops.deinit) {
		ISP_LOGE("fail to dlsym smart_ops.deinit");
		goto error_dlsym;
	}
	cxt->ops.smart_ops.ioctrl = dlsym(cxt->ispalg_lib_handle, "smart_ctl_ioctl");
	if (!cxt->ops.smart_ops.ioctrl) {
		ISP_LOGE("fail to dlsym smart_ops.ioctrl");
		goto error_dlsym;
	}
	cxt->ops.smart_ops.calc = dlsym(cxt->ispalg_lib_handle, "_smart_calc");
	if (!cxt->ops.smart_ops.calc) {
		ISP_LOGE("fail to dlsym smart_ops.calc");
		goto error_dlsym;
	}
	cxt->ops.smart_ops.block_disable = dlsym(cxt->ispalg_lib_handle, "smart_ctl_block_disable");
	if (!cxt->ops.smart_ops.block_disable) {
		ISP_LOGE("fail to dlsym smart_ops.block_disable");
		goto error_dlsym;
	}
	cxt->ops.smart_ops.block_enable = dlsym(cxt->ispalg_lib_handle, "smart_ctl_block_enable_recover");
	if (!cxt->ops.smart_ops.block_enable) {
		ISP_LOGE("fail to dlsym smart_ops.block_enable");
		goto error_dlsym;
	}
	cxt->ops.smart_ops.NR_disable = dlsym(cxt->ispalg_lib_handle, "smart_ctl_NR_block_disable");
	if (!cxt->ops.smart_ops.NR_disable) {
		ISP_LOGE("fail to dlsym smart_ops.NR_disable");
		goto error_dlsym;
	}

	cxt->ops.lsc_ops.init = dlsym(cxt->ispalg_lib_handle, "lsc_ctrl_init");
	if (!cxt->ops.lsc_ops.init) {
		ISP_LOGE("fail to dlsym lsc_ops.init");
		goto error_dlsym;
	}
	cxt->ops.lsc_ops.deinit = dlsym(cxt->ispalg_lib_handle, "lsc_ctrl_deinit");
	if (!cxt->ops.lsc_ops.deinit) {
		ISP_LOGE("fail to dlsym lsc_ops.deinit");
		goto error_dlsym;
	}
	cxt->ops.lsc_ops.process = dlsym(cxt->ispalg_lib_handle, "lsc_ctrl_process");
	if (!cxt->ops.lsc_ops.process) {
		ISP_LOGE("fail to dlsym lsc_ops.process");
		goto error_dlsym;
	}
	cxt->ops.lsc_ops.ioctrl = dlsym(cxt->ispalg_lib_handle, "lsc_ctrl_ioctrl");
	if (!cxt->ops.lsc_ops.ioctrl) {
		ISP_LOGE("fail to dlsym lsc_ops.ioctrl");
		goto error_dlsym;
	}

	return 0;
error_dlsym:
	dlclose(cxt->ispalg_lib_handle);
	cxt->ispalg_lib_handle = NULL;
error_dlopen:

	return ret;
}

static cmr_int ispalg_libops_init(cmr_handle adpt_handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct  isp_alg_fw_context *)adpt_handle;

	ret = ispalg_load_library(cxt);
	ISP_LOGI("done %ld", ret);
	return ret;
}

static cmr_int ispalg_ae_set_work_mode(
		cmr_handle isp_alg_handle, cmr_u32 new_mode,
		cmr_u32 fly_mode, struct isp_video_start *param_ptr)
{
	cmr_s32 ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct ae_set_work_param ae_param;
	enum ae_work_mode ae_mode = 0;
	struct isp_pm_ioctl_output output = { NULL, 0 };

	memset(&ae_param, 0, sizeof(ae_param));

	switch (new_mode) {
	case ISP_MODE_ID_PRV_0:
	case ISP_MODE_ID_PRV_1:
	case ISP_MODE_ID_PRV_2:
	case ISP_MODE_ID_PRV_3:
		ae_mode = AE_WORK_MODE_COMMON;
		break;

	case ISP_MODE_ID_CAP_0:
	case ISP_MODE_ID_CAP_1:
	case ISP_MODE_ID_CAP_2:
	case ISP_MODE_ID_CAP_3:
		ae_mode = AE_WORK_MODE_CAPTURE;
		break;

	case ISP_MODE_ID_VIDEO_0:
	case ISP_MODE_ID_VIDEO_1:
	case ISP_MODE_ID_VIDEO_2:
	case ISP_MODE_ID_VIDEO_3:
		ae_mode = AE_WORK_MODE_VIDEO;
		break;

	default:
		break;
	}

	ae_param.mode = ae_mode;
	ae_param.fly_eb = fly_mode;
	ae_param.highflash_measure.highflash_flag = param_ptr->is_need_flash;
	ae_param.highflash_measure.capture_skip_num = param_ptr->capture_skip_num;
	ae_param.capture_skip_num = param_ptr->capture_skip_num;
	ae_param.zsl_flag = param_ptr->capture_mode;
	ae_param.resolution_info.frame_size.w = cxt->commn_cxt.src.w;
	ae_param.resolution_info.frame_size.h = cxt->commn_cxt.src.h;
	ae_param.resolution_info.frame_line = cxt->commn_cxt.input_size_trim[cxt->commn_cxt.param_index].frame_line;
	ae_param.resolution_info.line_time = cxt->commn_cxt.input_size_trim[cxt->commn_cxt.param_index].line_time;
	ae_param.resolution_info.sensor_size_index = cxt->commn_cxt.param_index;
	ae_param.is_snapshot = param_ptr->is_snapshot;
	ae_param.dv_mode = param_ptr->dv_mode;

	ae_param.sensor_fps.mode = param_ptr->sensor_fps.mode;
	ae_param.sensor_fps.max_fps = param_ptr->sensor_fps.max_fps;
	ae_param.sensor_fps.min_fps = param_ptr->sensor_fps.min_fps;
	ae_param.sensor_fps.is_high_fps = param_ptr->sensor_fps.is_high_fps;
	ae_param.sensor_fps.high_fps_skip_num = param_ptr->sensor_fps.high_fps_skip_num;
	ae_param.win_num.h = cxt->ae_cxt.win_num.h;
	ae_param.win_num.w = cxt->ae_cxt.win_num.w;

	ae_param.win_size.w = ((ae_param.resolution_info.frame_size.w / ae_param.win_num.w) / 2) * 2;
	ae_param.win_size.h = ((ae_param.resolution_info.frame_size.h / ae_param.win_num.h) / 2) * 2;
	ae_param.shift = 0; /* no shift in AEM block of sharkl5/roc1*/
	cxt->ae_cxt.shift = ae_param.shift;

	ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_AE_ADAPT_PARAM, NULL, &output);
	if(ISP_SUCCESS == ret && 1 == output.param_num && NULL != output.param_data) {
		struct isp_ae_adapt_info *data;

		data = output.param_data[0].data_ptr;
		ae_param.binning_factor = data->binning_factor;
		ISP_LOGD("binning_factor = %d\n", ae_param.binning_factor);
	}

	if (cxt->ops.awb_ops.ioctrl) {
		ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle,
				AWB_CTRL_CMD_GET_CT_TABLE20, NULL, (void *)&ae_param.ct_table);
		ISP_TRACE_IF_FAIL(ret, ("fail to AWB_CTRL_CMD_GET_CT_TABLE20"));
	}

	if (cxt->ops.ae_ops.ioctrl) {
		ISP_LOGD("trigger ae start.");
		ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_VIDEO_START, &ae_param, NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to AE_VIDEO_START"));
	}

	return ret;
}

static cmr_int ispalg_update_alg_param(cmr_handle isp_alg_handle)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_u32 ct = 0;
	cmr_s32 bv = 0;
	cmr_s32 bv_gain = 0;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct smart_proc_input smart_proc_in;
	struct awb_gain result;
	struct isp_pm_ioctl_input ioctl_output = { PNULL, 0 };
	struct isp_pm_ioctl_input ioctl_input;
	struct isp_pm_param_data ioctl_data;
	struct isp_awbc_cfg awbc_cfg;

	cxt->aem_is_update = 0;
	memset(&result, 0, sizeof(result));
	if (cxt->ops.awb_ops.ioctrl) {
		ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_GET_GAIN, (void *)&result, NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to AWB_CTRL_CMD_GET_GAIN"));
	}

	awbc_cfg.r_gain = result.r;
	awbc_cfg.g_gain = result.g;
	awbc_cfg.b_gain = result.b;
	awbc_cfg.r_offset = 0;
	awbc_cfg.g_offset = 0;
	awbc_cfg.b_offset = 0;
	ioctl_data.id = ISP_BLK_AWB_NEW;
	ioctl_data.cmd = ISP_PM_BLK_AWBC;
	ioctl_data.data_ptr = &awbc_cfg;
	ioctl_data.data_size = sizeof(awbc_cfg);
	ioctl_input.param_data_ptr = &ioctl_data;
	ioctl_input.param_num = 1;
	ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_AWB, (void *)&ioctl_input, NULL);
	if (cxt->ops.awb_ops.ioctrl) {
		ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_GET_CT, (void *)&ct, NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to AWB_CTRL_CMD_GET_CT"));
	}
	if (cxt->ops.ae_ops.ioctrl) {
		ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_GET_BV_BY_LUM_NEW, NULL, (void *)&bv);
		ISP_TRACE_IF_FAIL(ret, ("fail to AE_GET_BV_BY_LUM_NEW"));
		ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_GET_BV_BY_GAIN, NULL, (void *)&bv_gain);
		ISP_TRACE_IF_FAIL(ret, ("fail to AE_GET_BV_BY_GAIN"));
	}


	memset(&ioctl_data, 0, sizeof(ioctl_data));
	BLOCK_PARAM_CFG(ioctl_input, ioctl_data,
			ISP_PM_BLK_GAMMA_TAB,
			ISP_BLK_RGB_GAMC, PNULL, 0);
	ret = isp_pm_ioctl(cxt->handle_pm,
			ISP_PM_CMD_GET_SINGLE_SETTING,
			(void *)&ioctl_input, (void *)&ioctl_output);
	ISP_TRACE_IF_FAIL(ret, ("fail to get GAMMA TAB"));
	if (ioctl_output.param_num == 1 && ioctl_output.param_data_ptr && ioctl_output.param_data_ptr->data_ptr)
		cxt->smart_cxt.tunning_gamma_cur = ioctl_output.param_data_ptr->data_ptr;

	memset(&smart_proc_in, 0, sizeof(smart_proc_in));
	if ((cxt->smart_cxt.sw_bypass == 0) && (0 != bv_gain) && (0 != ct)) {
		int i, num = (cxt->zsl_flag) ? 2 : 1;

		smart_proc_in.cal_para.bv = bv;
		smart_proc_in.cal_para.bv_gain = bv_gain;
		smart_proc_in.cal_para.ct = ct;
		smart_proc_in.alc_awb = cxt->awb_cxt.alc_awb;
		smart_proc_in.scene_flag = cxt->commn_cxt.scene_flag;
		smart_proc_in.cal_para.gamma_tab = cxt->smart_cxt.tunning_gamma_cur;
		isp_prepare_atm_param(isp_alg_handle, &smart_proc_in);

		for (i = 0; i < num; i++) {
			if (i == 0)
				smart_proc_in.mode_flag = cxt->pm_mode_cxt.mode_id;
			else
				smart_proc_in.mode_flag = cxt->pm_mode_cxt.cap_mode_id;
			if (cxt->ops.smart_ops.ioctrl) {
				ret = cxt->ops.smart_ops.ioctrl(cxt->smart_cxt.handle,
							ISP_SMART_IOCTL_SET_WORK_MODE,
							(void *)&smart_proc_in.mode_flag, NULL);
				ISP_TRACE_IF_FAIL(ret, ("fail to ISP_SMART_IOCTL_SET_WORK_MODE"));
			}
			if (cxt->ops.smart_ops.calc)
				ret = cxt->ops.smart_ops.calc(cxt->smart_cxt.handle, &smart_proc_in);
		}
	}

	return ret;
}

static cmr_int ispalg_update_alsc_result(cmr_handle isp_alg_handle, cmr_handle out_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_s32 i =0;
	cmr_s32 lsc_result_size = 0;
	cmr_u16 *lsc_result_address_new = NULL;
	struct isp_2d_lsc_param *lsc_tab_param_ptr;
	struct isp_lsc_info *lsc_info_new;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct alsc_fwstart_info *fwstart_info = (struct alsc_fwstart_info *)out_ptr;
	struct isp_pm_ioctl_input input = { PNULL, 0 };
	struct isp_pm_param_data pm_param;

	if (cxt->lsc_cxt.sw_bypass)
		return 0;

	memset(&input, 0, sizeof(struct isp_pm_ioctl_input));
	memset(&pm_param, 0, sizeof(pm_param));

	ret = ispalg_alsc_get_info(cxt);
	ISP_RETURN_IF_FAIL(ret, ("fail to do get lsc info"));
	lsc_tab_param_ptr = (struct isp_2d_lsc_param *)(cxt->lsc_cxt.lsc_tab_address);
	lsc_info_new = (struct isp_lsc_info *)cxt->lsc_cxt.lsc_info;

	for (i = 0; i < 9; i++)
		fwstart_info->lsc_tab_address_new[i] = lsc_tab_param_ptr->map_tab[i].param_addr;
	fwstart_info->gain_width_new = lsc_info_new->gain_w;
	fwstart_info->gain_height_new = lsc_info_new->gain_h;
	fwstart_info->image_pattern_new = cxt->commn_cxt.image_pattern;
	fwstart_info->grid_new = lsc_info_new->grid;
	fwstart_info->camera_id = cxt->camera_id;

	lsc_result_size = lsc_info_new->gain_w * lsc_info_new->gain_h * 4 * sizeof(cmr_u16);
	lsc_result_address_new = (cmr_u16*)malloc(lsc_result_size);
	if (NULL == lsc_result_address_new) {
		ret = ISP_ALLOC_ERROR;
		ISP_LOGE("fail to alloc lsc_result_address_new");
		return ret;
	}
	fwstart_info->lsc_result_address_new = lsc_result_address_new;

	if (cxt->ops.lsc_ops.ioctrl) {
		ret = cxt->ops.lsc_ops.ioctrl(cxt->lsc_cxt.handle, ALSC_FW_START, (void *)fwstart_info, NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to ALSC_FW_START"));
	}

	BLOCK_PARAM_CFG(input, pm_param,
			ISP_PM_BLK_LSC_MEM_ADDR,
			ISP_BLK_2D_LSC,
			lsc_result_address_new, lsc_result_size);
	ret = isp_pm_ioctl(cxt->handle_pm,
			ISP_PM_CMD_SET_OTHERS,
			&input, NULL);

	free(lsc_result_address_new);
	lsc_result_address_new = NULL;

	return ret;
}

cmr_int isp_alg_fw_start(cmr_handle isp_alg_handle, struct isp_video_start * in_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_s32 mode = 0, prv_mode = 0, cap_mode = 0;
	cmr_u32 sn_mode = 0;
	char value[PROPERTY_VALUE_MAX] = { 0x00 };
	struct isp_size orig_size;
	struct isp_rgb_aem_info aem_info;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct alsc_fwstart_info fwstart_info = { NULL, {NULL}, 0, 0, 5, 0, 0};
	struct afctrl_fwstart_info af_start_info;
	struct dcam_dev_vc2_control vch2_info;
	struct sensor_pdaf_info *pdaf_info = NULL;

	if (!isp_alg_handle || !in_ptr) {
		ISP_LOGE("fail to get valid ptr.");
		ret = ISP_PARAM_ERROR;
		return ret;
	}

	ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_RESET, &in_ptr->size.w, &in_ptr->size.h);

	cxt->work_mode = in_ptr->work_mode;
	cxt->zsl_flag = in_ptr->zsl_flag;
	cxt->sensor_fps.mode = in_ptr->sensor_fps.mode;
	cxt->sensor_fps.max_fps = in_ptr->sensor_fps.max_fps;
	cxt->sensor_fps.min_fps = in_ptr->sensor_fps.min_fps;
	cxt->sensor_fps.is_high_fps = in_ptr->sensor_fps.is_high_fps;
	cxt->sensor_fps.high_fps_skip_num = in_ptr->sensor_fps.high_fps_skip_num;

	orig_size.w = cxt->commn_cxt.src.w;
	orig_size.h = cxt->commn_cxt.src.h;
	cxt->commn_cxt.src.w = in_ptr->size.w;
	cxt->commn_cxt.src.h = in_ptr->size.h;

	memset(&cxt->mem_info, 0, sizeof(struct isp_mem_info));
	cxt->mem_info.alloc_cb = in_ptr->alloc_cb;
	cxt->mem_info.free_cb = in_ptr->free_cb;
	cxt->mem_info.oem_handle = in_ptr->oem_handle;

	ret = ispalg_get_aem_param(cxt, &aem_info);
	cxt->ae_cxt.win_num.w = aem_info.blk_num.w;
	cxt->ae_cxt.win_num.h = aem_info.blk_num.h;

	/* malloc statis/lsc and other buffers and mapping buffers to dev. */
	ret = isp_dev_prepare_buf(cxt->dev_access_handle, &cxt->mem_info);
	ISP_RETURN_IF_FAIL(ret, ("fail to prepare buf"));

	switch (in_ptr->work_mode) {
	case 0:		/*preview */
	{
		if (in_ptr->dv_mode) {
			ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_DV_MODEID_BY_RESOLUTION, in_ptr, &prv_mode);
			mode = prv_mode;
		} else {
			ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_PRV_MODEID_BY_RESOLUTION, in_ptr, &prv_mode);
			ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_CAP_MODEID_BY_RESOLUTION, in_ptr, &cap_mode);
			mode = prv_mode;
		}
		break;
	}
	case 1:		/* capture only */
	{
		ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_CAP_MODEID_BY_RESOLUTION, in_ptr, &cap_mode);
		mode = cap_mode;
		prv_mode = cap_mode;
		break;
	}
	case 2:
		mode = ISP_MODE_ID_VIDEO_0;
		break;
	default:
		mode = ISP_MODE_ID_PRV_0;
		break;
	}
	ISP_LOGD("work_mode %d, pm mode %d %d %d\n", cxt->work_mode, mode, prv_mode, cap_mode);

	cxt->commn_cxt.isp_mode = mode;
	cxt->commn_cxt.mode_flag = mode;
	cxt->pm_mode_cxt.mode_id = mode;
	cxt->pm_mode_cxt.prv_mode_id = prv_mode;
	cxt->pm_mode_cxt.cap_mode_id = ISP_TUNE_MODE_INVALID;
	if (cxt->zsl_flag)
		cxt->pm_mode_cxt.cap_mode_id = cap_mode;
	ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_MODE, &cxt->pm_mode_cxt, NULL);

	cxt->commn_cxt.param_index = ispalg_get_param_index(cxt->commn_cxt.input_size_trim, &in_ptr->size);

	ret = ispalg_update_alg_param(cxt);
	ISP_RETURN_IF_FAIL(ret, ("fail to isp smart param calc"));

	ISP_LOGI("pdaf_support = %d, pdaf_enable = %d, is_multi_mode = %d",
			cxt->pdaf_cxt.pdaf_support, in_ptr->pdaf_enable, cxt->is_multi_mode);
	if (SENSOR_PDAF_TYPE3_ENABLE == cxt->pdaf_cxt.pdaf_support
		&& in_ptr->pdaf_enable && !cxt->is_multi_mode) {
		if (cxt->ops.pdaf_ops.ioctrl) {
			ISP_LOGI("open pdaf type 3");
			ret = cxt->ops.pdaf_ops.ioctrl(
					cxt->pdaf_cxt.handle,
					PDAF_CTRL_CMD_SET_PARAM,
					NULL, NULL);
			ISP_RETURN_IF_FAIL(ret, ("fail to cfg pdaf"));
		}
	}

	property_get("debug.camera.pdaf.type1.supprot", (char *)value, "0");
	sn_mode = in_ptr->resolution_info.size_index;
	pdaf_info = cxt->pdaf_info;
	ISP_LOGV("sn_mode = 0x%x", sn_mode);
	if(pdaf_info && pdaf_info->sns_mode
		&& SENSOR_PDAF_TYPE1_ENABLE == cxt->pdaf_cxt.pdaf_support) {
		ISP_LOGI("pdaf_info->sns_mode = %d",pdaf_info->sns_mode[sn_mode]);
		if ((pdaf_info->sns_mode[sn_mode]) || (1 == atoi(value))) {
			vch2_info.bypass = pdaf_info->vch2_info.bypass;
			vch2_info.vch2_vc = pdaf_info->vch2_info.vch2_vc;
			vch2_info.vch2_data_type = pdaf_info->vch2_info.vch2_data_type;
			vch2_info.vch2_mode = pdaf_info->vch2_info.vch2_mode;
			ISP_LOGI("vch2_info.bypass = 0x%x, vc = 0x%x, data_type = 0x%x, mode = 0x%x",
					vch2_info.bypass, vch2_info.vch2_vc,
					vch2_info.vch2_data_type, vch2_info.vch2_mode);
			ret = isp_dev_access_ioctl(cxt->dev_access_handle,
					ISP_DEV_SET_PDAF_TYPE1_CFG, &vch2_info, 0);
		}
	}

	if (pdaf_info && in_ptr->pdaf_enable
		&& SENSOR_PDAF_TYPE2_ENABLE == cxt->pdaf_cxt.pdaf_support  ) {
		vch2_info.bypass = pdaf_info->vch2_info.bypass;
		vch2_info.vch2_vc = pdaf_info->vch2_info.vch2_vc;
		vch2_info.vch2_data_type = pdaf_info->vch2_info.vch2_data_type;
		vch2_info.vch2_mode = pdaf_info->vch2_info.vch2_mode;
		ISP_LOGI("vch2_info.bypass = 0x%x, vc = 0x%x, data_type = 0x%x, mode = 0x%x",
				vch2_info.bypass, vch2_info.vch2_vc,
				vch2_info.vch2_data_type, vch2_info.vch2_mode);
		ret = isp_dev_access_ioctl(cxt->dev_access_handle,
				ISP_DEV_SET_PDAF_TYPE2_CFG, &vch2_info, 0);
	}
	if (pdaf_info && in_ptr->pdaf_enable
		&& SENSOR_DUAL_PDAF_ENABLE == cxt->pdaf_cxt.pdaf_support  ) {
		vch2_info.bypass = pdaf_info->vch2_info.bypass;
		vch2_info.vch2_vc = pdaf_info->vch2_info.vch2_vc;
		vch2_info.vch2_data_type = pdaf_info->vch2_info.vch2_data_type;
		vch2_info.vch2_mode = pdaf_info->vch2_info.vch2_mode;
		ISP_LOGI("vch2_info.bypass = 0x%x, vc = 0x%x, data_type = 0x%x, mode = 0x%x",
				vch2_info.bypass, vch2_info.vch2_vc,
				vch2_info.vch2_data_type, vch2_info.vch2_mode);
		ret = isp_dev_access_ioctl(cxt->dev_access_handle,
				ISP_DEV_SET_DUAL_PDAF_CFG, &vch2_info, 0);
	}

	if (cxt->ebd_cxt.ebd_support) {
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_EBD_CFG, 0, 0);
		ISP_RETURN_IF_FAIL(ret, ("fail to cfg embedded line"));
	}

	ret = ispalg_update_alsc_result(cxt, (void *)&fwstart_info);
	ISP_RETURN_IF_FAIL(ret, ("fail to update alsc result"));

	ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_CFG_START, NULL, NULL);
	ISP_TRACE_IF_FAIL(ret, ("fail to do cfg start"));

	ret = ispalg_cfg_param(cxt, 1);
	ISP_RETURN_IF_FAIL(ret, ("fail to do ispalg_cfg_param"));

	if (cxt->afl_cxt.handle) {
		((struct isp_anti_flicker_cfg *)cxt->afl_cxt.handle)->width = cxt->commn_cxt.src.w;
		((struct isp_anti_flicker_cfg *)cxt->afl_cxt.handle)->height = cxt->commn_cxt.src.h;
	}

	if(cxt->afl_cxt.version) {
		if (cxt->ops.afl_ops.config_new)
			ret = cxt->ops.afl_ops.config_new(cxt->afl_cxt.handle);
	}
	ISP_TRACE_IF_FAIL(ret, ("fail to do anti_flicker param update"));

	if (cxt->ops.awb_ops.ioctrl) {
		ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle,
				AWB_CTRL_CMD_SET_WORK_MODE,
				&in_ptr->work_mode, NULL);
		ISP_RETURN_IF_FAIL(ret, ("fail to set_awb_work_mode"));
		ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle,
				AWB_CTRL_CMD_GET_PIX_CNT, &in_ptr->size, NULL);
		ISP_RETURN_IF_FAIL(ret, ("fail to get_awb_pix_cnt"));
	}

	ret = ispalg_ae_set_work_mode(cxt, mode, 1, in_ptr);
	ISP_RETURN_IF_FAIL(ret, ("fail to do ae cfg"));

	memset(&af_start_info, 0, sizeof(struct afctrl_fwstart_info));
	af_start_info.sensor_fps.is_high_fps = in_ptr->sensor_fps.is_high_fps;
	af_start_info.sensor_fps.high_fps_skip_num = in_ptr->sensor_fps.high_fps_skip_num;
	if (cxt->af_cxt.handle && ((ISP_VIDEO_MODE_CONTINUE == in_ptr->mode))) {
		if (cxt->ops.af_ops.ioctrl) {
			af_start_info.size = in_ptr->size;
			ISP_LOGD("trigger AF video start.  size %d %d\n", af_start_info.size.w, af_start_info.size.h);
			ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle,
					AF_CMD_SET_ISP_START_INFO,
					(void *)&af_start_info, NULL);
			ISP_TRACE_IF_FAIL(ret, ("fail to set af_start_info"));
		}
	}
	if (cxt->ops.lsc_ops.ioctrl) {
		ret = cxt->ops.lsc_ops.ioctrl(cxt->lsc_cxt.handle, ALSC_FW_START_END, (void *)&fwstart_info, NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to end alsc_fw_start"));
	}

exit:
	ISP_LOGD("done %ld", ret);
	return ret;
}


cmr_int isp_alg_fw_stop(cmr_handle isp_alg_handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;

	if (cxt->ops.ae_ops.ioctrl) {
		ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_VIDEO_STOP, NULL, NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to AE_VIDEO_STOP"));
	}
	if (cxt->ops.awb_ops.ioctrl) {
		ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_VIDEO_STOP_NOTIFY, NULL, NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to AWB_CTRL_CMD_VIDEO_STOP_NOTIFY"));
	}
	if (cxt->ops.af_ops.ioctrl) {
		ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle, AF_CMD_SET_ISP_STOP_INFO, NULL, NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to AF_CMD_SET_ISP_STOP_INFO"));
	}
	if (cxt->ops.lsc_ops.ioctrl) {
		ret = cxt->ops.lsc_ops.ioctrl(cxt->lsc_cxt.handle, ALSC_FW_STOP, NULL, NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to ALSC_FW_STOP"));
	}

	ISP_RETURN_IF_FAIL(ret, ("fail to stop isp alg fw"));

exit:
	ISP_LOGV("done %ld", ret);
	return ret;
}

cmr_int isp_alg_fw_proc_start(cmr_handle isp_alg_handle, struct ips_in_param *in_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_s32 i = 0;
	cmr_s32 mode = 0;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_video_start param;
	struct isp_lsc_info *lsc_info_new = NULL;
	struct isp_2d_lsc_param *lsc_tab_param_ptr = NULL;
	struct alsc_fwprocstart_info fwprocstart_info = { NULL, {NULL}, 0, 0, 5, 0, 0};
	struct isp_raw_proc_info raw_proc_in;

	if (!isp_alg_handle || !in_ptr) {
		ISP_LOGE("fail to get valid ptr.");
		ret = ISP_PARAM_ERROR;
		return ret;
	}

	ISP_LOGD("isp proc start\n");
	ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_RESET, NULL, NULL);
	ISP_TRACE_IF_FAIL(ret, ("fail to do isp_dev_reset"));

	cxt->commn_cxt.src.w = in_ptr->src_frame.img_size.w;
	cxt->commn_cxt.src.h = in_ptr->src_frame.img_size.h;

	cxt->mem_info.alloc_cb = in_ptr->alloc_cb;
	cxt->mem_info.free_cb = in_ptr->free_cb;
	cxt->mem_info.oem_handle = in_ptr->oem_handle;

	memset(&raw_proc_in, 0, sizeof(raw_proc_in));
	raw_proc_in.src_format = IMG_PIX_FMT_GREY;
	raw_proc_in.src_pattern =  cxt->commn_cxt.image_pattern;
	raw_proc_in.src_size.width = in_ptr->src_frame.img_size.w;
	raw_proc_in.src_size.height = in_ptr->src_frame.img_size.h;
	raw_proc_in.dst_format = in_ptr->dst_frame.img_fmt;
	raw_proc_in.dst_size.width = in_ptr->dst_frame.img_size.w;
	raw_proc_in.dst_size.height = in_ptr->dst_frame.img_size.h;
	raw_proc_in.fd_src = in_ptr->src_frame.img_fd.y;
	raw_proc_in.src_offset = in_ptr->src_frame.img_addr_phy.chn0;
	/* dst1 is final dst image (YUV image after ISP) */
	raw_proc_in.fd_dst1 = in_ptr->dst_frame.img_fd.y;
	raw_proc_in.dst1_offset = in_ptr->dst_frame.img_addr_phy.chn0;

	/* dst0 is middle image (raw image after DCAM processing) */
	/* if HAL does not set this buffer (fd <= 0) kernel will allocate one */
	/* HAL may use same buffer as src buffer, but apply offset in it */
	raw_proc_in.fd_dst0 = in_ptr->dst2_frame.img_fd.y;
	raw_proc_in.dst0_offset = in_ptr->dst2_frame.img_addr_phy.chn0;

	/* RAW_PROC_PRE, kernel will init the whole channel for proc. */
	/* Then we can config parameters to right register or ISP context buffer */
	raw_proc_in.cmd = RAW_PROC_PRE;
	ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_RAW_PROC, &raw_proc_in, NULL);
	ISP_RETURN_IF_FAIL(ret, ("fail to do isp_dev_raw_proc"));

	/* malloc statis/lsc and other buffers and mapping buffers to dev. */
	ret = isp_dev_prepare_buf(cxt->dev_access_handle, &cxt->mem_info);
	ISP_RETURN_IF_FAIL(ret, ("fail to prepare buf"));


	param.size.w = cxt->commn_cxt.src.w;
	param.size.h = cxt->commn_cxt.src.h;
	ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_CAP_MODEID_BY_RESOLUTION, &param, &mode);
	ISP_RETURN_IF_FAIL(ret, ("fail to get isp_mode"));

	cxt->commn_cxt.isp_mode = mode;
	cxt->commn_cxt.mode_flag = mode;
	cxt->pm_mode_cxt.mode_id = mode;
	cxt->pm_mode_cxt.prv_mode_id = mode;
	cxt->pm_mode_cxt.cap_mode_id = ISP_TUNE_MODE_INVALID;
	ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_MODE, &cxt->pm_mode_cxt, NULL);
	ISP_RETURN_IF_FAIL(ret, ("fail to do isp_pm_ioctl"));

	cxt->commn_cxt.param_index =
		ispalg_get_param_index(cxt->commn_cxt.input_size_trim, &in_ptr->src_frame.img_size);

	if (cxt->takepicture_mode != CAMERA_ISP_SIMULATION_MODE) {
		ret = ispalg_update_alg_param(cxt);
		ISP_RETURN_IF_FAIL(ret, ("fail to update alg parm"));

		ret = ispalg_update_alsc_result(cxt, (void *)&fwprocstart_info);
		ISP_RETURN_IF_FAIL(ret, ("fail to update alsc result"));
	}

	ret = ispalg_alsc_get_info(cxt);
	ISP_RETURN_IF_FAIL(ret, ("fail to do get lsc info"));
	lsc_tab_param_ptr = (struct isp_2d_lsc_param *)(cxt->lsc_cxt.lsc_tab_address);
	lsc_info_new = (struct isp_lsc_info *)cxt->lsc_cxt.lsc_info;

	fwprocstart_info.lsc_result_address_new = (cmr_u16 *) lsc_info_new->data_ptr;
	for (i = 0; i < 9; i++)
		fwprocstart_info.lsc_tab_address_new[i] = lsc_tab_param_ptr->map_tab[i].param_addr;
	fwprocstart_info.gain_width_new = lsc_info_new->gain_w;
	fwprocstart_info.gain_height_new = lsc_info_new->gain_h;
	fwprocstart_info.image_pattern_new = cxt->commn_cxt.image_pattern;
	fwprocstart_info.grid_new  = lsc_info_new->grid;
	fwprocstart_info.camera_id = cxt->camera_id;

	if (cxt->ops.lsc_ops.ioctrl)
		ret = cxt->ops.lsc_ops.ioctrl(cxt->lsc_cxt.handle, ALSC_FW_PROC_START, (void *)&fwprocstart_info, NULL);

	ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_CFG_START, NULL, NULL);
	ISP_TRACE_IF_FAIL(ret, ("fail to do cfg start"));

	ret = ispalg_cfg_param(cxt, 1);
	ISP_RETURN_IF_FAIL(ret, ("fail to do isp cfg"));

	param.is_need_flash = 0;
	param.capture_skip_num = 0;
	param.is_snapshot = 1;
	param.dv_mode = 0;
	param.sensor_fps.mode = in_ptr->sensor_fps.mode;
	param.sensor_fps.max_fps = in_ptr->sensor_fps.max_fps;
	param.sensor_fps.min_fps = in_ptr->sensor_fps.min_fps;
	param.sensor_fps.is_high_fps = in_ptr->sensor_fps.is_high_fps;
	param.sensor_fps.high_fps_skip_num = in_ptr->sensor_fps.high_fps_skip_num;
	ret = ispalg_ae_set_work_mode(cxt, mode, 0, &param);

	if (cxt->ops.ae_ops.ioctrl) {
		ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_RGB_GAIN, NULL, NULL);
		ISP_RETURN_IF_FAIL(ret, ("fail to set rgb gain"));
	}
	if (cxt->ops.lsc_ops.ioctrl)
		ret = cxt->ops.lsc_ops.ioctrl(cxt->lsc_cxt.handle, ALSC_FW_PROC_START_END, (void *)&fwprocstart_info, NULL);

	/* RAW_PROC_POST, kernel will set all buffers and trigger raw image processing*/
	ISP_LOGV("start raw proc\n");
	raw_proc_in.cmd = RAW_PROC_POST;
	ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_RAW_PROC, &raw_proc_in, NULL);
	ISP_TRACE_IF_FAIL(ret, ("fail to do isp_dev_raw_proc"));

exit:
	ISP_LOGV("done %ld", ret);
	return ret;
}


cmr_int isp_alg_fw_ioctl(cmr_handle isp_alg_handle, enum isp_ctrl_cmd io_cmd, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	enum isp_ctrl_cmd cmd = io_cmd & 0x7fffffff;
	isp_io_fun io_ctrl = NULL;

	cxt->commn_cxt.isp_callback_bypass = io_cmd & 0x80000000;
	io_ctrl = isp_ioctl_get_fun(cmd);
	if (NULL != io_ctrl) {
		ret = io_ctrl(cxt, param_ptr);
	} else {
		ISP_LOGV("io_ctrl fun is null, cmd %d", cmd);
	}

	if (NULL != cxt->commn_cxt.callback) {
		cxt->commn_cxt.callback(cxt->commn_cxt.caller_id,
			ISP_CALLBACK_EVT | ISP_CTRL_CALLBACK | cmd, NULL, ISP_ZERO);
	}

	return ret;
}

cmr_int isp_alg_fw_capability(cmr_handle isp_alg_handle, enum isp_capbility_cmd cmd, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	cmr_u32 out_param = 0;

	switch (cmd) {
	case ISP_LOW_LUX_EB:
		if (cxt->ops.ae_ops.ioctrl)
			ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_GET_FLASH_EB, NULL, &out_param);
		*((cmr_u32 *) param_ptr) = out_param;
		break;
	case ISP_CUR_ISO:
		if (cxt->ops.ae_ops.ioctrl)
			ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_GET_ISO, NULL, &out_param);
		*((cmr_u32 *) param_ptr) = out_param;
		break;
	case ISP_CTRL_GET_AE_LUM:
		if (cxt->ops.ae_ops.ioctrl)
			ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_GET_BV_BY_LUM_NEW, NULL, &out_param);
		*((cmr_u32 *) param_ptr) = out_param;
		break;
	default:
		break;
	}

	ISP_LOGV("done %ld", ret);
	return ret;
}


cmr_int isp_alg_fw_init(struct isp_alg_fw_init_in * input_ptr, cmr_handle * isp_alg_handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = NULL;
	struct isp_alg_sw_init_in isp_alg_input;
	struct sensor_raw_info *sensor_raw_info_ptr = NULL;

	if (!input_ptr || !isp_alg_handle) {
		ISP_LOGE("fail to check input param, 0x%lx", (cmr_uint) input_ptr);
		ret = ISP_PARAM_NULL;
		return ret;
	}
	*isp_alg_handle = NULL;

	cxt = (struct isp_alg_fw_context *)malloc(sizeof(struct isp_alg_fw_context));
	if (!cxt) {
		ISP_LOGE("fail to malloc");
		ret = ISP_ALLOC_ERROR;
		return ret;
	}
	memset(cxt, 0, sizeof(struct isp_alg_fw_context));

	sensor_raw_info_ptr =
		(struct sensor_raw_info *)input_ptr->init_param->setting_param_ptr;
	cxt->dev_access_handle = input_ptr->dev_access_handle;
	cxt->lib_use_info = sensor_raw_info_ptr->libuse_info;
	cxt->otp_data = input_ptr->init_param->otp_data;
	cxt->camera_id = input_ptr->init_param->camera_id;
	cxt->is_master = input_ptr->init_param->is_master;
	cxt->is_multi_mode = input_ptr->init_param->multi_mode;
	cxt->pdaf_cxt.pdaf_support = input_ptr->init_param->ex_info.pdaf_supported;
	//cxt->ebd_cxt.ebd_support = input_ptr->init_param->ex_info.ebd_supported;
	ISP_LOGV("camera_id = %ld, master %d\n", cxt->camera_id, cxt->is_master);

	cxt->pdaf_info = (struct sensor_pdaf_info *)malloc(sizeof(struct sensor_pdaf_info));
	if (!cxt->pdaf_info) {
		ISP_LOGE("fail to malloc pdaf_info buf");
		ret = ISP_ALLOC_ERROR;
		goto err_mem;
	}
	if (input_ptr->init_param->pdaf_info)
		memcpy(cxt->pdaf_info, input_ptr->init_param->pdaf_info, sizeof(struct sensor_pdaf_info));

	ret = ispalg_libops_init(cxt);
	if (ret) {
		ISP_LOGE("fail to init alg library and ops");
		goto err_mem;
	}

	ret = ispalg_pm_init(cxt, input_ptr->init_param);
	if (ISP_SUCCESS != ret) {
		ISP_LOGE("fail to do ispalg_pm_init");
		goto err_mem;
	}

	ret = isp_br_init(cxt->camera_id, cxt, cxt->is_master);
	if (ret) {
		ISP_LOGE("fail to init isp bridge");
		goto err_br;
	}

	isp_alg_input.lib_use_info = cxt->lib_use_info;
	isp_alg_input.size.w = input_ptr->init_param->size.w;
	isp_alg_input.size.h = input_ptr->init_param->size.h;
	isp_alg_input.otp_data = cxt->otp_data;
	isp_alg_input.pdaf_info = input_ptr->init_param->pdaf_info;
	isp_alg_input.sensor_max_size = input_ptr->init_param->sensor_max_size;
	ret = ispalg_init(cxt, &isp_alg_input);
	if (ret) {
		ISP_LOGE("fail to init ispalg\n");
		goto err_alg;
	}

	ret = ispalg_create_thread((cmr_handle)cxt);
	if (ret) {
		ISP_LOGE("fail to create thread.\n");
		goto err_thrd;
	}
	isp_dev_access_evt_reg(cxt->dev_access_handle, ispalg_dev_evt_msg, (cmr_handle) cxt);

	*isp_alg_handle = (cmr_handle)cxt;

	ISP_LOGI("done %ld", ret);
	return 0;

err_thrd:
	ispalg_deinit((cmr_handle) cxt);
err_alg:
	isp_br_deinit(cxt->camera_id);
err_br:
	isp_pm_deinit(cxt->handle_pm);
err_mem:
	if (cxt->ispalg_lib_handle) {
		dlclose(cxt->ispalg_lib_handle);
		cxt->ispalg_lib_handle = NULL;
	}
	if (cxt->pdaf_info)
		free((void *)cxt->pdaf_info);
	free((void *)cxt);
	ISP_LOGE("done: %ld", ret);
	return ret;
}

cmr_int isp_alg_fw_deinit(cmr_handle isp_alg_handle)
{
	cmr_s32 ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;

	if (!cxt) {
		ISP_LOGE("fail to get cxt pointer");
		ret = ISP_PARAM_NULL;
		return ret;
	}

	ispalg_destroy_thread_proc((cmr_handle) cxt);
	ispalg_deinit((cmr_handle) cxt);
	isp_br_deinit(cxt->camera_id);
	isp_pm_deinit(cxt->handle_pm);
	if (cxt->ispalg_lib_handle) {
		dlclose(cxt->ispalg_lib_handle);
		cxt->ispalg_lib_handle = NULL;
	}
	isp_dev_free_buf(cxt->dev_access_handle, &cxt->mem_info);

	free((void *)cxt->pdaf_info);
	free((void *)cxt);

	ISP_LOGI("done %d", ret);
	return ret;
}
