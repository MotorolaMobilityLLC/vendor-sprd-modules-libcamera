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
#include <cutils/sched_policy.h>
#include <sys/resource.h>
#include <system/thread_defs.h>

#include "isp_alg_fw.h"
#include "cmr_msg.h"
#include "cmr_prop.h"
#include "isp_dev_access.h"
#include "isp_debug.h"
#include "isp_pm.h"
#include "awb.h"
#include "awblib.h"
#include "af_ctrl.h"
#include "ae_ctrl.h"
#include "afl_ctrl.h"
#include "awb_ctrl.h"
#include "smart_ctrl.h"
#include "lsc_adv.h"
#include "isp_bridge.h"
#include "pdaf_ctrl.h"
#include "ai_ctrl.h"
#include "tof_ctrl.h"
#include "isp_simulation.h"

#define LIBCAM_ALG_FILE "libispalg.so"
#define CMC10(n) (((n)>>13)?((n)-(1<<14)):(n))
#define MIN_FRAME_INTERVAL_MS  (10)

cmr_u32 isp_cur_bv;
cmr_u32 isp_cur_ct;

struct commn_info {
	cmr_s32 isp_pm_mode[PARAM_SET_MAX];
	cmr_u32 multi_nr_flag;
	cmr_u32 scene_flag;
	cmr_u32 ai_scene_id;
	cmr_u32 image_pattern;
	cmr_u32 param_index;
	cmr_u32 isp_callback_bypass;
	proc_callback callback;
	cmr_handle caller_id;
	cmr_u8 *log_isp;
	cmr_u32 log_isp_size;
	struct isp_size src;
	struct isp_size prv_size;
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
	struct ae_ctrl_ebd_info ebd_info;
	cmr_u32 shift;
	cmr_u32 flash_version;
};

struct awb_info {
	cmr_handle handle;
	cmr_u32 sw_bypass;
	cmr_u32 color_support;
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
	cmr_u8 atm_is_set;
	void *tunning_gamma_cur;
};

struct afl_info {
	cmr_handle handle;
	cmr_u32 version;
	cmr_u32 sw_bypass;
	cmr_u32 hw_bypass;
	cmr_uint vir_addr;
	cmr_int buf_size;
	cmr_int buf_num;
	cmr_uint phy_addr;
	cmr_u32 afl_mode;
	cmr_uint mfd;
	cmr_int buf_property;
	void *buffer_client_data;
	struct isp_statis_info afl_statis_info;
};

struct af_info {
	cmr_handle handle;
	cmr_u32 sw_bypass;
	cmr_u8 *log_af;
	cmr_u32 log_af_size;
	struct af_monitor_win_num win_num;
	cmr_u32 tof_support;
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
	cmr_u32 full_size_width;
	cmr_u32 full_size_height;
	cmr_u32 full_size_grid;
	cmr_u32 LSC_SPD_VERSION;

};


struct ai_info {
	cmr_handle handle;
	struct ai_ae_param ae_param;
};

struct ispalg_ae_ctrl_ops {
	cmr_s32 (*init)(struct ae_init_in *input_ptr, cmr_handle *handle_ae, cmr_handle result);
	cmr_int (*deinit)(cmr_handle *isp_afl_handle);
	cmr_int (*process)(cmr_handle handle_ae, struct ae_calc_in *in_ptr, struct ae_calc_out *result);
	cmr_int (*ioctrl)(cmr_handle handle, enum ae_io_ctrl_cmd cmd, cmr_handle in_ptr, cmr_handle out_ptr);
};

struct ispalg_ai_ctrl_ops {
	cmr_s32 (*init)(struct ai_init_in *input_ptr, cmr_handle *handle_ai, cmr_handle result);
	cmr_int (*deinit)(cmr_handle *handle_ai);
	cmr_int (*ioctrl)(cmr_handle handle_ai, enum ai_io_ctrl_cmd cmd, cmr_handle in_ptr, cmr_handle out_ptr);
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

struct ispalg_tof_ctrl_ops {
	cmr_int (*init)(struct tof_ctrl_init_in *input_ptr, cmr_handle *handle);
	cmr_int (*deinit)(cmr_handle *handle);
};

struct ispalg_lib_ops {
	struct ispalg_ae_ctrl_ops ae_ops;
	struct ispalg_af_ctrl_ops af_ops;
	struct ispalg_afl_ctrl_ops afl_ops;
	struct ispalg_awb_ctrl_ops awb_ops;
	struct ispalg_smart_ctrl_ops smart_ops;
	struct ispalg_pdaf_ctrl_ops pdaf_ops;
	struct ispalg_lsc_ctrl_ops lsc_ops;
	struct ispalg_tof_ctrl_ops tof_ops;
	struct ispalg_ai_ctrl_ops ai_ops;
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
	cmr_u8 bayerhist_update;
	cmr_u8 fw_started;
	cmr_u8 first_frm;
	cmr_u8 aethd_pri_set;
	nsecs_t last_sof_time;
	struct isp_awb_statistic_info aem_stats_data;
	struct isp_awb_statistic_info aem_ue;
	struct isp_awb_statistic_info aem_ae;
	struct isp_awb_statistic_info aem_oe;
	struct isp_awb_statistic_info cnt_ue;
	struct isp_awb_statistic_info cnt_oe;
	struct isp_hist_statistic_info bayer_hist_stats[3];
	struct isp_hist_statistic_info hist2_stats;
	struct ae_size hist2_roi;
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
	struct ai_info ai_cxt;
	struct sensor_libuse_info *lib_use_info;
	struct sensor_raw_ioctrl *ioctrl_ptr;
	struct sensor_pdaf_info *pdaf_info;
	struct isp_mem_info mem_info;
	cmr_handle thr_handle;
	cmr_handle thr_aehandle;
	cmr_handle thr_afhandle;
	cmr_handle dev_access_handle;
	cmr_handle handle_pm;
	cmr_handle tof_handle;
	struct dcam_dev_rgb_gain_info rgb_gain;

	struct isp_sensor_fps_info sensor_fps;
	struct sensor_otp_cust_info *otp_data;
	cmr_u32 lsc_flash_onoff;
	cmr_u32 takepicture_mode;
	struct isptool_scene_param simu_param;
	cmr_handle ispalg_lib_handle;
	struct ispalg_lib_ops ops;
	cmr_u8  is_master;
	cmr_u32 is_multi_mode;
	cmr_u32 sensor_role;
	cmr_u32 work_mode;
	cmr_u32 zsl_flag;

	cmr_u32 sn_mode;
	/* 4in1 */
	cmr_u32 is_4in1_sensor;
	cmr_u32 cam_4in1_mode; /* 1: 4c mode for capture */
	cmr_u32 lowlight_flag; /* low lux for capture */
	/* for 4x zoom focus */
	cmr_u32 last_ratio;
	cmr_u32 cur_ratio;
	cmr_s32 curr_bv;
	cmr_s32 noramosaic_4in1;
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

		cxt->rgb_gain.bypass = gain_info->bypass;
		cxt->rgb_gain.global_gain = gain_info->global_gain;
		cxt->rgb_gain.r_gain = gain_info->r_gain;
		cxt->rgb_gain.g_gain = gain_info->g_gain;
		cxt->rgb_gain.b_gain = gain_info->b_gain;
		ISP_LOGV("rgbgain_bypass = %d, global_gain = %d, r_gain = %d, g_gain = %d, b_gain = %d",
			cxt->rgb_gain.bypass, cxt->rgb_gain.global_gain,
			cxt->rgb_gain.r_gain, cxt->rgb_gain.g_gain, cxt->rgb_gain.b_gain);
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
	struct isptool_scene_param scene_param;

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

	cxt->rgb_gain.bypass=inptr->bypass;
	cxt->rgb_gain.global_gain=inptr->global_gain;
	cxt->rgb_gain.r_gain=inptr->r_gain;
	cxt->rgb_gain.g_gain=inptr->g_gain;
	cxt->rgb_gain.b_gain=inptr->b_gain;
	block_info.scene_id = PM_SCENE_PRE;

	if (isp_video_get_simulation_flag()) {
		ret = isp_sim_get_scene_parm(&scene_param);
		if (ISP_SUCCESS == ret) {
			gain_info.global_gain = scene_param.global_gain;
			cxt->rgb_gain.global_gain = gain_info.global_gain;
			ISP_LOGI("hwsim: global_gain :input[%d] scene_param[%d]\n",
                             inptr->global_gain , scene_param.global_gain);
		}
	}
	ISP_LOGV("global_gain : %d, r %d g %d b %d\n", gain_info.global_gain,
		gain_info.r_gain, gain_info.g_gain, gain_info.b_gain);
	ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_RGB_GAIN, &block_info, NULL);
	/* also set capture gain if 4in1 */
	if (cxt->noramosaic_4in1 || cxt->cam_4in1_mode) {
		/* this value for capture, need * 4
		 * not active while pm * 4
		 */
		block_info.scene_id = PM_SCENE_CAP;
		if (cxt->cam_4in1_mode)
			gain_info.global_gain *= 4;
		ISP_LOGV("global_gain : %d, r %d g %d b %d\n", gain_info.global_gain,
			gain_info.r_gain, gain_info.g_gain, gain_info.b_gain);
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_RGB_GAIN, &block_info, NULL);
		if (ret) {
			ISP_LOGW("fail to set rgb gain for 4in1 capture");
		}
	}

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
	struct ae_size *hdr_statis_size;

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
	case AE_CB_HDR_STATUS:
		cmd = ISP_AUTO_HDR_STATUS_CALLBACK;
		break;
	case AE_CB_HDR_STATIS_SIZE:
		hdr_statis_size = (struct ae_size *)data;
		hdr_statis_size->w = cxt->hist2_roi.w;
		hdr_statis_size->h = cxt->hist2_roi.h;
		return ret;
	case AE_CB_3DNR_NOTIFY:
		cmd = ISP_3DNR_CALLBACK;
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

static cmr_int ispalg_ai_callback(cmr_handle isp_alg_handle, cmr_int cb_type, void *data)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	enum isp_callback_cmd cmd = 0;

	if (!cxt) {
		ret = -ISP_PARAM_NULL;
		goto exit;
	}

	switch (cb_type) {
	case AI_CB_FOR_HAL: {
		cxt->commn_cxt.ai_scene_id = *(cmr_u8 *)data;
		ISP_LOGV("ai_scene_id %d", cxt->commn_cxt.ai_scene_id);
		cmd = ISP_AI_SCENE_TYPE_CALLBACK;
		if (cxt->commn_cxt.callback) {
			cxt->commn_cxt.callback(cxt->commn_cxt.caller_id, ISP_CALLBACK_EVT | cmd, data, 0);
		}
		break;
	}
	case AI_CB_FOR_AE: {
		if (cxt->ops.ae_ops.ioctrl)
			ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_SCENE_INFO, data, NULL);
		break;
	}
	case AI_CB_FOR_AWB: {
		if (cxt->ops.awb_ops.ioctrl)
			ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_SET_SCENE_INFO, data, NULL);
		break;
	}
	case AI_CB_FOR_AF: {
		if (cxt->ops.af_ops.ioctrl)
			ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle, AF_CMD_SET_SCENE_INFO, data, NULL);
		break;
	}
	default:
		break;
	}

exit:
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
	struct dcam_dev_hist_info bayerHist_info;

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

	cxt->ai_cxt.ae_param.blk_num_hor = (cmr_u16)aem_win.blk_num_x;
	cxt->ai_cxt.ae_param.blk_num_ver = (cmr_u16)aem_win.blk_num_y;
	cxt->ai_cxt.ae_param.blk_width = (cmr_u16)aem_win.blk_width;
	cxt->ai_cxt.ae_param.blk_height = (cmr_u16)aem_win.blk_height;
	cxt->ai_cxt.ae_param.ae_offset.x = aem_win.offset_x;
	cxt->ai_cxt.ae_param.ae_offset.y = aem_win.offset_y;

	ret = isp_dev_access_ioctl(cxt->dev_access_handle,
			ISP_DEV_SET_AE_MONITOR_WIN,
			&aem_win, NULL);

	/* temp enable and configure bayerhist to fixed mode */
	/* todo - configure bayer hist according to algo requirement */
	memset(&bayerHist_info, 0, sizeof(bayerHist_info));
	bayerHist_info.hist_bypass = 0;
	bayerHist_info.bayer_hist_endx = cxt->commn_cxt.prv_size.w;
	bayerHist_info.bayer_hist_endy = cxt->commn_cxt.prv_size.h;
	bayerHist_info.hist_mode_sel = 1;
	bayerHist_info.hist_mul_enable = 1;
	bayerHist_info.hist_initial_clear = 1;
	bayerHist_info.hist_skip_num_clr = 1;

	ret = isp_dev_access_ioctl(cxt->dev_access_handle,
			ISP_DEV_SET_BAYERHIST_CFG, &bayerHist_info, NULL);

	return ret;
}


static cmr_int ispalg_set_aem_thrd(cmr_handle isp_alg_handle, struct ae_monitor_info *in)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct dcam_dev_aem_thr aem_thrd;
	ISP_LOGD("aem_threshold, rgb high(%d %d %d)  rgb low(%d %d %d)\n",
	in->high_region_thrd.r,in->high_region_thrd.g,in->high_region_thrd.b,
	in->low_region_thrd.r,in->low_region_thrd.g,in->low_region_thrd.b);
	memset(&aem_thrd, 0, sizeof(struct dcam_dev_aem_thr));
	aem_thrd.aem_r_thr.low_thr = in->low_region_thrd.r;//in->r_low;
	aem_thrd.aem_r_thr.high_thr = in->high_region_thrd.r;//in->r_high;
	aem_thrd.aem_g_thr.low_thr = in->low_region_thrd.g;//in->g_low;
	aem_thrd.aem_g_thr.high_thr = in->high_region_thrd.g;
	aem_thrd.aem_b_thr.low_thr = in->low_region_thrd.b;
	aem_thrd.aem_b_thr.high_thr = in->high_region_thrd.b;
	ret = isp_dev_access_ioctl(cxt->dev_access_handle,
	ISP_DEV_SET_AE_RGB_THR, &aem_thrd, NULL);
	return ret;
}

static cmr_int ispalg_ae_set_cb(cmr_handle isp_alg_handle,
		cmr_int type, void *param0, void *param1)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct sensor_multi_ae_info *ae_info = NULL;
	struct isp_alg_fw_context *context = NULL;
	cmr_handle handle = NULL;
	cmr_u32 camera_id = 0;

	if (!cxt) {
		ISP_LOGE("fail to get valid cxt ptr\n");
		return ISP_PARAM_NULL;
	}

	switch (type) {
	case ISP_AE_MULTI_WRITE:
		{
			cmr_u32 index = 0;

#define index2role(index) (index)
			ae_info = (struct sensor_multi_ae_info *)param0;
			while (index < ae_info->count) {
				/*
				 * ae_info ordered by master, slave0, slave1, ...
				 * this is same as role definition
				 */
				isp_br_ioctrl(index2role(index), GET_CAMERA_ID, NULL, &camera_id);
				isp_br_ioctrl(CAM_SENSOR_MASTER, GET_ISPALG_FW_BY_ID, &camera_id, &handle);
				context = (struct isp_alg_fw_context *)handle;
				if (!context) {
					ISP_LOGE("slave context not ready");
					return ISP_ERROR;
				}

				ae_info[index].handle = context->ioctrl_ptr->caller_handler;
				ae_info[index].camera_id = context->camera_id;
				ae_info[index].exp.size_index = context->sn_mode;

				index++;
			}

			if (cxt->ioctrl_ptr->sns_ioctl)
				ret = cxt->ioctrl_ptr->sns_ioctl(cxt->ioctrl_ptr->caller_handler,
						CMD_SNS_IC_WRITE_MULTI_AE, ae_info);
#undef index2role
		}
		break;
	case ISP_AE_HDR_BOKEH:
		ISP_LOGV("ISP_AE_HDR_BOKEH");

		isp_br_ioctrl(CAM_SENSOR_SLAVE0, GET_CAMERA_ID, NULL, &camera_id);
		isp_br_ioctrl(CAM_SENSOR_MASTER, GET_ISPALG_FW_BY_ID, &camera_id, &handle);
		context = (struct isp_alg_fw_context *)handle;
		if (context != NULL) {
			ret = ispalg_ae_callback(context, AE_CB_HDR_START, (void *)param0);
		} else {
			ISP_LOGE("fail to get slave sensor handle , it is not ready");
			return ret;
		}
		break;
	case ISP_AE_SET_RGB_GAIN_SLAVE0:
		ISP_LOGV("ISP_AE_SET_RGB_GAIN_SLAVE0");

		isp_br_ioctrl(CAM_SENSOR_SLAVE0, GET_CAMERA_ID, NULL, &camera_id);
		isp_br_ioctrl(CAM_SENSOR_MASTER, GET_ISPALG_FW_BY_ID, &camera_id, &handle);
		context = (struct isp_alg_fw_context *)handle;
		if (context != NULL) {
			ret = ispalg_set_rgb_gain(context, param0);
		} else {
			ISP_LOGE("fail to get slave sensor handle , it is not ready");
			return ret;
		}
		break;
	case ISP_AE_SET_RGB_GAIN_SLAVE1:
		ISP_LOGV("ISP_AE_SET_RGB_GAIN_SLAVE1");

		isp_br_ioctrl(CAM_SENSOR_SLAVE1, GET_CAMERA_ID, NULL, &camera_id);
		isp_br_ioctrl(CAM_SENSOR_MASTER, GET_ISPALG_FW_BY_ID, &camera_id, &handle);
		context = (struct isp_alg_fw_context *)handle;
		if (context != NULL) {
			ret = ispalg_set_rgb_gain(context, param0);
		} else {
			ISP_LOGE("fail to get slave sensor handle , it is not ready");
			return ret;
		}
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
	case ISP_AE_SET_RGB_THRD:
		ret = ispalg_set_aem_thrd(cxt, param0);
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
		if (cxt->noramosaic_4in1 || cxt->cam_4in1_mode) {
			cfg.scene_id = PM_SCENE_CAP;
			cfg.block_info = &gain;
			ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AWB_GAIN, &cfg, NULL);
			if(ret != ISP_SUCCESS){
				ISP_LOGE("fail to set isp device access ioctl.");
				return ret;
			}
		}
		cfg.scene_id = PM_SCENE_PRE;
		cfg.block_info = &gain;
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AWB_GAIN, &cfg, NULL);
		break;
	}
	default:
		ISP_LOGV("unsupported ae cb: %lx\n", type);
		break;
	}

	return ret;
}

static cmr_int ispalg_af_mode_convert(struct isp_af_notice *isp_af)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_u32 af_mode = 0;

	af_mode = isp_af->af_mode;
	switch (af_mode) {
		case AF_MODE_NORMAL:
			isp_af->af_mode = ISP_FOCUS_TRIG;
			break;
		case AF_MODE_MACRO:
			isp_af->af_mode = ISP_FOCUS_MACRO;
			break;
		case AF_MODE_CONTINUE:
			isp_af->af_mode = ISP_FOCUS_CONTINUE;
			break;
		case AF_MODE_MANUAL:
			isp_af->af_mode = ISP_FOCUS_MANUAL;
			break;
		case AF_MODE_VIDEO:
			isp_af->af_mode = ISP_FOCUS_VIDEO;
			break;
		case AF_MODE_PICTURE:
			isp_af->af_mode = ISP_FOCUS_PICTURE;
			break;
		case AF_MODE_FULLSCAN:
			isp_af->af_mode = ISP_FOCUS_FULLSCAN;
			break;
		default:
			isp_af->af_mode = ISP_FOCUS_TRIG;
			break;
	}

	return ret;
}

static cmr_int ispalg_af_set_cb(cmr_handle isp_alg_handle, cmr_int type, void *param0, void *param1)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_u16 val = 0;
	cmr_u32 pos = 0;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;

	if (!cxt) {
		ISP_LOGE("fail to get valid cxt ptr\n");
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
			struct isp_af_notice *isp_af = (struct isp_af_notice *)param0;

			ret = ispalg_af_mode_convert(isp_af);

			ret = cxt->commn_cxt.callback(cxt->commn_cxt.caller_id,
				ISP_CALLBACK_EVT | ISP_AF_NOTICE_CALLBACK,
				isp_af, sizeof(struct isp_af_notice));
		}
		break;
	case AF_CB_CMD_SET_MOTOR_POS:
		val = *(cmr_u16 *)param0;
		pos = (cmr_u32)val;
		if (cxt->ioctrl_ptr->set_pos) {
			ret = cxt->ioctrl_ptr->set_pos(cxt->ioctrl_ptr->caller_handler, pos);
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
	case AF_CB_CMD_SET_MOTOR_STATUS:
		ret = cxt->commn_cxt.callback(cxt->commn_cxt.caller_id,
						ISP_CALLBACK_EVT | ISP_AF_VCM_NOTICE_CALLBACK,
						param0, sizeof(cmr_u32));
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
		ISP_LOGE("fail to get valid cxt ptr\n");
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

static cmr_int ispalg_tof_set_cb(cmr_handle isp_alg_handle, cmr_int type, void *param0, void *param1)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;

	switch (type) {
	case ISP_AF_SET_TOF_INFO:
		if (cxt->ops.af_ops.ioctrl)
			ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle, AF_CMD_SET_TOF_INFO, param0, param1);
		break;
	default:
		break;
	}

	return ret;
}

static cmr_int ispalg_afl_set_cb(cmr_handle isp_alg_handle, cmr_int type, void *param0, void *param1)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_u32 bypass;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;

	if (!cxt) {
		ISP_LOGE("fail to get valid cxt ptr\n");
		return ISP_PARAM_NULL;
	}
	switch (type) {
	case ISP_AFL_NEW_SET_CFG_PARAM:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AFL_NEW_CFG_PARAM, param0, param1);
		break;
	case ISP_AFL_NEW_SET_BYPASS:
		bypass = *(cmr_u32 *)param0;
		ISP_LOGV("hw_bypass= %d, new bypass %d\n", cxt->afl_cxt.hw_bypass, bypass);
		if ((cxt->afl_cxt.hw_bypass == 0) && (bypass == 0)) {
			ISP_LOGV("should not trigger afl when hw is working\n");
			break;
		}
		cxt->afl_cxt.hw_bypass = bypass;
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AFL_NEW_BYPASS, param0, param1);
		break;
	case ISP_AFL_SET_STATS_BUFFER:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_STSTIS_BUF, param0, param1);
		break;
	default:
		ISP_LOGV("unsupported afl cb: %lx\n", type);
		break;
	}

	return ret;
}
static cmr_int ispalg_ai_set_cb(cmr_handle isp_alg_handle, cmr_int type, void *param0, void *param1)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct ai_scene_detect_info *ai_info = NULL;
	enum isp_callback_cmd cmd = 0;

	switch (type) {
	case AI_CALLBACK_SCENE_INFO:
		cmd = ISP_AI_SCENE_INFO_CALLBACK;
		ai_info = (struct ai_scene_detect_info *)param0;
		cxt->commn_cxt.ai_scene_id = ai_info->cur_scene_id;
		ISP_LOGV("ai_scene_id %d", cxt->commn_cxt.ai_scene_id);
		if (cxt->commn_cxt.callback) {
			cxt->commn_cxt.callback(cxt->commn_cxt.caller_id,
				ISP_CALLBACK_EVT | cmd, ai_info, sizeof(struct ai_scene_detect_info));
		}
		break;
	case AI_CALLBACK_SET_CB:
		ret = ispalg_ai_callback(cxt, *(cmr_int *)param0, param1);
		break;
	default:
		break;
	}

	return ret;
}

static cmr_int ispalg_dump_rgbgamma_curve(struct sensor_rgbgamma_curve *gamma_cur)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_u32 i = 0;
	FILE *fp = NULL;
	char file_name[260] = { 0 };
	char loop_cnt_str[10] = { 0 };

	if (gamma_cur == NULL) {
		ISP_LOGE("fail to gamma param pointer.");
		return ISP_ERROR;
	}
	ret = isp_sim_get_mipi_raw_file_name(file_name);
	if (strlen(file_name)) {
		sprintf(loop_cnt_str, ".%d", isp_video_get_simulation_loop_count());
		strcat(file_name, loop_cnt_str);
		strcat(file_name, ".gamma.log");
		ISP_LOGI("dump to [%s] start\n", file_name);
	} else {
		ISP_LOGE("fail to get mipi raw file name.");
		return ISP_ERROR;
	}
	CMR_LOGD("file name %s", file_name);
	fp = fopen(file_name, "wb");
	if (fp == NULL) {
		ISP_LOGE("fail to open file");
		return ISP_ERROR;
	}

	fprintf(fp, "1.r gamma:\n");
	for (i=0; i < SENSOR_GAMMA_POINT_NUM; i++) {
		fprintf(fp, "<%d>:x[%d]y[%d] \n",
			i, gamma_cur->points_r[i].x, gamma_cur->points_r[i].y);
	}

	fprintf(fp, "2.g gamma:\n");
	for (i=0; i < SENSOR_GAMMA_POINT_NUM; i++) {
		fprintf(fp, "<%d>:x[%d]y[%d] \n",
			i, gamma_cur->points_g[i].x, gamma_cur->points_g[i].y);
	}

	fprintf(fp, "3.b gamma:\n");
	for (i=0; i < SENSOR_GAMMA_POINT_NUM; i++) {
		fprintf(fp, "<%d>:x[%d]y[%d] \n",
			i, gamma_cur->points_b[i].x, gamma_cur->points_b[i].y);
	}

	fflush(fp);
	fclose(fp);
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
	char value[PROPERTY_VALUE_MAX];

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
	{
		/* param0 should be (struct sensor_rgbgamma_curve *)  */
		void *gamma_cur = param0;

		if (cxt->smart_cxt.atm_is_set == 0) {
			memset(&pm_param, 0, sizeof(pm_param));
			BLOCK_PARAM_CFG(io_pm_input, pm_param,
				ISP_PM_BLK_GAMMA_CUR,
				ISP_BLK_RGB_GAMC,
				gamma_cur,
				sizeof(struct sensor_rgbgamma_curve));
			
			if (isp_video_get_simulation_flag()) {
				property_get("persist.vendor.cam.debug.simulation", value, "false");
				if (!strcmp(value, "true")) {
					ISP_LOGI("vvv:ISP_SMART_SET_GAMMA_CUR\n");
					ispalg_dump_rgbgamma_curve(gamma_cur);
				}
			}

			ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_OTHERS, &io_pm_input, NULL);
			ISP_TRACE_IF_FAIL(ret, ("fail to set gammar cur"));
			cxt->smart_cxt.atm_is_set = 1;
		}
		break;
	}
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

	ISP_LOGV("enter\n");

	memset(&pm_param, 0, sizeof(struct isp_pm_param_data));
	BLOCK_PARAM_CFG(io_pm_input, pm_param,
			ISP_PM_BLK_LSC_GET_LSCTAB,
			ISP_BLK_2D_LSC, NULL, 0);
	ret = isp_pm_ioctl(pm_handle,
			ISP_PM_CMD_GET_SINGLE_SETTING,
			(void *)&io_pm_input, (void *)&io_pm_output);

	if ((ret != ISP_SUCCESS) || (io_pm_output.param_num != 1)
		|| (io_pm_output.param_data->data_ptr == NULL)) {
		ISP_LOGE("fail to get lsc tab");
		return ISP_ERROR;
	}

	cxt->lsc_cxt.lsc_tab_address = io_pm_output.param_data->data_ptr;
	cxt->lsc_cxt.lsc_tab_size = io_pm_output.param_data->data_size;

	memset(&pm_param, 0, sizeof(struct isp_pm_param_data));
	BLOCK_PARAM_CFG(io_pm_input, pm_param,
			ISP_PM_BLK_LSC_INFO,
			ISP_BLK_2D_LSC, PNULL, 0);
	ret = isp_pm_ioctl(pm_handle,
			ISP_PM_CMD_GET_SINGLE_SETTING,
			(void *)&io_pm_input, (void *)&io_pm_output);

	if ((ret != ISP_SUCCESS) || (io_pm_output.param_num != 1)
		|| (io_pm_output.param_data->data_ptr == NULL)) {
		ISP_LOGE("fail to get lsc info");
		return ISP_ERROR;
	}
	cxt->lsc_cxt.lsc_info = (struct isp_lsc_info *)io_pm_output.param_data->data_ptr;

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
		calc_param.isp_mode = cxt->commn_cxt.isp_pm_mode[0];
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
		ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_ISP_ALL_SETTING, &input, &output);
	else
		ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_ISP_SETTING, &input, &output);
	ISP_RETURN_IF_FAIL(ret, ("fail to get isp block settings"));

	/* lock pm to avoid parameter is updated by algo while driver reading */
	isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_LOCK, NULL, NULL);

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
			if ((!IS_DCAM_BLOCK(param_data->id)) || (cxt->cam_4in1_mode || cxt->noramosaic_4in1)) {
				/* todo: refine for 4in1 sensor */
				isp_dev_cfg_block(cxt->dev_access_handle, &sub_block_info, param_data->id);
				ISP_LOGV("cfg block %x for cap.\n", param_data->id);
			}
			param_data++;
		}
	}

	isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_UNLOCK, NULL, NULL);

	/* Enable hist statis (Y-256). Only one will take effect. Driver MUST decide it */
	if (start) {
		struct isp_dev_hist2_info hist2_info;
		struct isp_dev_hist_info hist_info;

		memset(&hist2_info, 0, sizeof(struct isp_dev_hist2_info));
		hist2_info.bypass = 0;
		hist2_info.mode = 1; /* 0 - single;  1 - continue multi */
		hist2_info.hist_roi.end_x = cxt->commn_cxt.prv_size.w - 1;
		hist2_info.hist_roi.end_y = cxt->commn_cxt.prv_size.h - 1;
		sub_block_info.block_info = &hist2_info;
		sub_block_info.scene_id = PM_SCENE_PRE;
		isp_dev_cfg_block(cxt->dev_access_handle, &sub_block_info, ISP_BLK_HIST2);

		memset(&hist_info, 0, sizeof(struct isp_dev_hist_info));
		hist_info.bypass = 0;
		hist_info.mode = 1; /* 0 - single;  1 - continue multi */
		sub_block_info.block_info = &hist_info;
		sub_block_info.scene_id = PM_SCENE_PRE;
		isp_dev_cfg_block(cxt->dev_access_handle, &sub_block_info, ISP_BLK_HIST);
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
	cmr_u32 lowlight_tmp = 0;

	if (cxt->ops.af_ops.ioctrl) {
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_GET_SYSTEM_TIME, &sec, &usec);
		af_ts.timestamp = sec * 1000000000LL + usec * 1000LL;
		af_ts.capture = 0;
		ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle,
					AF_CMD_SET_DCAM_TIMESTAMP, (void *)(&af_ts), NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to set AF_CMD_SET_DCAM_TIMESTAMP"));
	}

	if (cxt->zsl_flag && cxt->cam_4in1_mode) {
		if (cxt->ops.ae_ops.ioctrl)
			ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_GET_LOWLIGHT_FLAG_BY_BV, NULL, (void *)&lowlight_tmp);

		if (lowlight_tmp != cxt->lowlight_flag) {
			cxt->lowlight_flag = lowlight_tmp;
			ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_LOWLIGHT_FLAG,
					&cxt->lowlight_flag, &cxt->commn_cxt.isp_pm_mode[1]);
		}
	}

	return ret;
}

static cmr_int ispalg_aem_stats_parser(cmr_handle isp_alg_handle, void *data)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_u32 ae_shift = 0, g_shift = 0;
	cmr_u32 i = 0;
	cmr_u32 blk_num = 0, blk_size = 0;
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
	blk_size = cxt->ae_cxt.win_size.w * cxt->ae_cxt.win_size.h / 4;
	uaddr = (cmr_u64 *)statis_info->uaddr;

	for (i = 0; i < blk_num; i++) {
#ifdef CONFIG_ISP_2_6 /* sharkl5/ROC1*/
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
		g_shift = 1;

#elif defined CONFIG_ISP_2_5 /* for SharkL3 */

		stats_val0 = *uaddr++;
		sum_b_oe = stats_val0 & 0x1fffff;
		sum_g_oe = (stats_val0 >> 21) & 0x3fffff;
		sum_r_oe = (stats_val0 >> 43) & 0x1fffff;

		stats_val0 = *uaddr++;
		sum_b_ue = stats_val0 & 0x1fffff;
		sum_g_ue = (stats_val0 >> 21) & 0x3fffff;
		sum_r_ue = (stats_val0 >> 43) & 0x1fffff;

		stats_val0 = *uaddr++;
		sum_b_ae = stats_val0 & 0x1fffff;
		sum_g_ae = (stats_val0 >> 21) & 0x3fffff;
		sum_r_ae = (stats_val0 >> 43) & 0x1fffff;

		stats_val0 = *uaddr++;
		cnt_b_ue = stats_val0 & 0x1fff;
		cnt_b_oe = (stats_val0 >> 16) & 0x1fff;
		cnt_r_ue = (stats_val0 >> 32) & 0x1fff;
		cnt_r_oe = (stats_val0 >> 48) & 0x1fff;

		stats_val1 = *uaddr++;
		cnt_g_ue = stats_val1 & 0x1fff;
		cnt_g_oe = (stats_val1 >> 16) & 0x1fff;
		g_shift = 0;

#else /* CONFIG_ISP_2_7  for  SharkL5Pro */

		//g
		stats_val0 = *uaddr++;
		stats_val1 = *uaddr++;
		sum_g_ue = (cmr_u32)(stats_val0 & 0x1ffffff);
		sum_g_ae = (cmr_u32)((stats_val0 >> 32) & 0x1ffffff);
		sum_g_oe = (cmr_u32)(stats_val1 & 0x1ffffff);
		cnt_g_ue = (cmr_u32)((stats_val1 >> 32) & 0x7fff);
		cnt_g_oe = (cmr_u32)((stats_val1 >> 48) & 0x7fff);
		//r
		stats_val0 = *uaddr++;
		stats_val1 = *uaddr++;
		sum_r_ue = (cmr_u32)(stats_val0 & 0x1ffffff);
		sum_r_ae = (cmr_u32)((stats_val0 >> 32) & 0x1ffffff);
		sum_r_oe = (cmr_u32)(stats_val1 & 0x1ffffff);
		cnt_r_ue = (cmr_u32)((stats_val1 >> 32) & 0x7fff);
		cnt_r_oe = (cmr_u32)((stats_val1 >> 48) & 0x7fff);
		//b
		stats_val0 = *uaddr++;
		stats_val1 = *uaddr++;
		sum_b_ue = (cmr_u32)(stats_val0 & 0x1ffffff);
		sum_b_ae = (cmr_u32)((stats_val0 >> 32) & 0x1ffffff);
		sum_b_oe = (cmr_u32)(stats_val1 & 0x1ffffff);
		cnt_b_ue = (cmr_u32)((stats_val1 >> 32) & 0x7fff);
		cnt_b_oe = (cmr_u32)((stats_val1 >> 48) & 0x7fff);
		g_shift = 1;
#endif

		cxt->cnt_ue.r_info[i] = cnt_r_ue;
		cxt->cnt_ue.g_info[i] = cnt_g_ue;
		cxt->cnt_ue.b_info[i] = cnt_b_ue;

		cxt->cnt_oe.r_info[i] = cnt_r_oe;
		cxt->cnt_oe.g_info[i] = cnt_g_oe;
		cxt->cnt_oe.b_info[i] = cnt_b_oe;

		cxt->aem_ae.r_info[i] = sum_r_ae << ae_shift;
		cxt->aem_ae.g_info[i] = sum_g_ae << ae_shift;
		cxt->aem_ae.b_info[i] = sum_b_ae << ae_shift;

		cxt->aem_ue.r_info[i] = sum_r_ue << ae_shift;
		cxt->aem_ue.g_info[i] = sum_g_ue << ae_shift;
		cxt->aem_ue.b_info[i] = sum_b_ue << ae_shift;

		cxt->aem_oe.r_info[i] = sum_r_oe << ae_shift;
		cxt->aem_oe.g_info[i] = sum_g_oe << ae_shift;
		cxt->aem_oe.b_info[i] = sum_b_oe << ae_shift;

		ae_stat_ptr->r_info[i] = (sum_r_oe + sum_r_ue + sum_r_ae) << ae_shift;
		ae_stat_ptr->g_info[i] = (sum_g_oe + sum_g_ue + sum_g_ae) << ae_shift;
		ae_stat_ptr->b_info[i] = (sum_b_oe + sum_b_ue + sum_b_ae) << ae_shift;

		/* from sharkl5 and after, aem output G is the sum of Gr & Gb
		  * which is different from previous version average Gr/Gb
		  * here we shift 1 to get average of G for AE algo compatibility */
		ae_stat_ptr->g_info[i] >>= g_shift;

		if (ae_stat_ptr->r_info[i] > (blk_size * 1023) ||
			ae_stat_ptr->g_info[i] > (blk_size * 1023) ||
			ae_stat_ptr->b_info[i] > (blk_size * 1023) )
			ISP_LOGE("data overflow. i %d, blk size %d %d,  r 0x%x, g 0x%x, b 0x%x\n",
				i, cxt->ae_cxt.win_size.w, cxt->ae_cxt.win_size.h,
				ae_stat_ptr->r_info[i], ae_stat_ptr->g_info[i], ae_stat_ptr->b_info[i]);
	}

	ae_stat_ptr->sec = statis_info->sec;
	ae_stat_ptr->usec = statis_info->usec;
	ISP_LOGV("frame_id %d, time %d.%06d\n",
		statis_info->frame_id, statis_info->sec, statis_info->usec);
	ISP_LOGV("sum[0]: r 0x%x, g 0x%x, b 0x%x cam[%d]\n",
		ae_stat_ptr->r_info[0], ae_stat_ptr->g_info[0], ae_stat_ptr->b_info[0], (cmr_u32)cxt->camera_id);
	ISP_LOGV("cnt: r 0x%x, 0x%x,  g 0x%x, 0x%x, b 0x%x, 0x%x cam[%d]\n",
		cnt_r_ue, cnt_r_oe, cnt_g_ue, cnt_g_oe, cnt_b_ue, cnt_b_oe, (cmr_u32)cxt->camera_id);

	if (isp_video_get_simulation_loop_count() == 0 &&
		cxt->takepicture_mode == CAMERA_ISP_SIMULATION_MODE) {
		isp_sim_save_ae_stats(ae_stat_ptr,
			cxt->ae_cxt.win_num.w,
			cxt->ae_cxt.win_num.h);
	}

	cxt->ai_cxt.ae_param.frame_id = statis_info->frame_id;
	cxt->ai_cxt.ae_param.zoom_ratio = statis_info->zoom_ratio;
	cxt->ai_cxt.ae_param.sec = statis_info->sec;
	cxt->ai_cxt.ae_param.usec = statis_info->usec;
	ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_STSTIS_BUF, statis_info, NULL);
	if (ret) {
		ISP_LOGE("fail to set statis buf");
	}

	/*
	  * This is a workaround for sharkl5: first frame aem value is smaller
	  * todo: find the root-cause of first frame aem data abnormal and fix it
	  */
	if (cxt->first_frm)
		cxt->first_frm = 0;
	else
		cxt->aem_is_update = 1;

	return ret;
}

static cmr_int ispalg_bayerhist_stats_parser(cmr_handle isp_alg_handle, void *data)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_u32 i, j;
	cmr_u64 *ptr;
	cmr_u64 val0, val1;
	struct isp_hist_statistic_info *hist_stats;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_statis_info *statis_info = (struct isp_statis_info *)data;

	cxt->bayerhist_update = 1;
	ptr = (cmr_u64 *)statis_info->uaddr;

	/* G */
	hist_stats = &cxt->bayer_hist_stats[0];
	hist_stats->bin = 256;
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
	hist_stats->bin = 256;
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
	hist_stats->bin = 256;
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
	ISP_LOGV("frm %d, time %d.%06d, data: r %d %d, g %d %d, b %d %d\n",
		statis_info->frame_id, statis_info->sec, statis_info->usec,
		cxt->bayer_hist_stats[0].value[0], cxt->bayer_hist_stats[0].value[1],
		cxt->bayer_hist_stats[1].value[0], cxt->bayer_hist_stats[1].value[1],
		cxt->bayer_hist_stats[2].value[0], cxt->bayer_hist_stats[2].value[1]);

	ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_STSTIS_BUF, statis_info, NULL);
	if (ret) {
		ISP_LOGE("fail to set statis buf");
	}

	return ret;
}

static cmr_int ispalg_hist2_stats_parser(cmr_handle isp_alg_handle, void *data)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_u32 i;
	cmr_u32 *ptr;
	struct isp_hist_statistic_info *hist_stats;
	struct ae_size *hist2_roi_info;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_statis_info *statis_info = (struct isp_statis_info *)data;

	ptr = (cmr_u32 *)statis_info->uaddr;

	/* update hist2 roi info */
	hist2_roi_info = &cxt->hist2_roi;
	hist2_roi_info->w = statis_info->width;
	hist2_roi_info->h = statis_info->height;

	/* Y */
	hist_stats = &cxt->hist2_stats;
	hist_stats->bin = 256;
	hist_stats->sec = statis_info->sec;
	hist_stats->usec = statis_info->usec;
	hist_stats->frame_id = statis_info->frame_id;

	for(i = 0; i < 256; i++) {
		hist_stats->value[i] = *(ptr + i);
	}

	ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_STSTIS_BUF, statis_info, NULL);
	if (ret) {
		ISP_LOGE("fail to set statis buf");
	}

	ISP_LOGV("frame %d, time %03d.%06d, image size %d %d\n", hist_stats->frame_id,
		hist_stats->sec, hist_stats->usec, hist2_roi_info->w, hist2_roi_info->h);
	ISP_LOGV("data %d %d %d %d,  %d %d %d %d\n",
		hist_stats->value[0], hist_stats->value[8] , hist_stats->value[16], hist_stats->value[32],
		hist_stats->value[64], hist_stats->value[128] , hist_stats->value[192], hist_stats->value[255]);
	return ret;
}

static cmr_int ispalg_hist2_process(cmr_handle isp_alg_handle, void *data)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;

	ret = ispalg_hist2_stats_parser(cxt, data);
	if (ret) {
		ISP_LOGE("fail to parse hist2 stats");
		return ret;
	}

	if (cxt->commn_cxt.callback) {
		ret = cxt->commn_cxt.callback(cxt->commn_cxt.caller_id,
					      ISP_CALLBACK_EVT | ISP_HIST_REPORT_CALLBACK,
					      cxt->hist2_stats.value,
					      sizeof(cmr_u32) * 256);
		if (ret) {
			ISP_LOGE("fail to report hist2 stats");
			return ret;
		}
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
	struct afl_ctrl_proc_out afl_info = {0, 0, 0};

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
	in_param.sum_ue_r = cxt->aem_ue.r_info;
	in_param.sum_ue_g = cxt->aem_ue.g_info;
	in_param.sum_ue_b = cxt->aem_ue.b_info;
	in_param.sum_ae_r = cxt->aem_ae.r_info;
	in_param.sum_ae_g = cxt->aem_ae.g_info;
	in_param.sum_ae_b = cxt->aem_ae.b_info;
	in_param.sum_oe_r = cxt->aem_oe.r_info;
	in_param.sum_oe_g = cxt->aem_oe.g_info;
	in_param.sum_oe_b = cxt->aem_oe.b_info;
	in_param.cnt_ue_r = cxt->cnt_ue.r_info;
	in_param.cnt_ue_g = cxt->cnt_ue.g_info;
	in_param.cnt_ue_b = cxt->cnt_ue.b_info;
	in_param.cnt_oe_r = cxt->cnt_oe.r_info;
	in_param.cnt_oe_g = cxt->cnt_oe.g_info;
	in_param.cnt_oe_b = cxt->cnt_oe.b_info;
	in_param.monoboottime = -1;
	in_param.is_last_frm = 0;
	in_param.time_diff = -1;
	in_param.is_update = cxt->aem_is_update;
	in_param.sensor_fps.mode = cxt->sensor_fps.mode;
	in_param.sensor_fps.max_fps = cxt->sensor_fps.max_fps;
	in_param.sensor_fps.min_fps = cxt->sensor_fps.min_fps;
	in_param.sensor_fps.is_high_fps = cxt->sensor_fps.is_high_fps;
	in_param.sensor_fps.high_fps_skip_num = cxt->sensor_fps.high_fps_skip_num;
	if (cxt->ebd_cxt.ebd_support) {
		memcpy(&in_param.ebd_info, &cxt->ae_cxt.ebd_info, sizeof(cxt->ae_cxt.ebd_info));
		in_param.isp_dgain.global_gain = cxt->rgb_gain.global_gain;
		in_param.isp_dgain.r_gain = cxt->rgb_gain.r_gain;
		in_param.isp_dgain.g_gain = cxt->rgb_gain.g_gain;
		in_param.isp_dgain.b_gain = cxt->rgb_gain.b_gain;
	}
	/* copy isp hist2 statis and pass to ae algo */
	memcpy((void *)&in_param.hist_stats, (void *)&cxt->hist2_stats,
		sizeof(struct isp_hist_statistic_info));

	if (cxt->bayerhist_update) {
		memcpy((void *)&in_param.bayerhist_stats[0],
			(void *)&cxt->bayer_hist_stats[0],
			sizeof(cxt->bayer_hist_stats));
		cxt->bayerhist_update = 0;
	}

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

	if (cxt->awb_cxt.color_support) {
		if (cxt->ioctrl_ptr->sns_ioctl) {
			cxt->ioctrl_ptr->sns_ioctl(cxt->ioctrl_ptr->caller_handler,
						CMD_SNS_IC_GET_CCT_DATA,
						&out_ptr->xyz_info);
		}
	}

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
		ISP_TRACE_IF_FAIL(ret, ("fail to set isp block param"));
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
			if (cxt->cam_4in1_mode)
				smart_proc_in.mode_flag = cxt->commn_cxt.isp_pm_mode[1];
			smart_proc_in.scene_flag = cxt->commn_cxt.scene_flag;
			smart_proc_in.ai_scene_id = cxt->commn_cxt.ai_scene_id;
			smart_proc_in.lock_nlm = cxt->smart_cxt.lock_nlm_en;
			smart_proc_in.lock_ee = cxt->smart_cxt.lock_ee_en;
			smart_proc_in.lock_precdn = cxt->smart_cxt.lock_precdn_en;
			smart_proc_in.lock_cdn = cxt->smart_cxt.lock_cdn_en;
			smart_proc_in.lock_postcdn = cxt->smart_cxt.lock_postcdn_en;
			smart_proc_in.lock_ccnr = cxt->smart_cxt.lock_ccnr_en;
			smart_proc_in.lock_ynr = cxt->smart_cxt.lock_ynr_en;
			smart_proc_in.cal_para.gamma_tab = cxt->smart_cxt.tunning_gamma_cur;
			isp_prepare_atm_param(isp_alg_handle, &smart_proc_in);
			cxt->smart_cxt.atm_is_set = 0;

			num = (cxt->zsl_flag) ? 2 : 1;
			for (i = 0; i < num; i++) {
				smart_proc_in.mode_flag = cxt->commn_cxt.isp_pm_mode[i];
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
			cxt->curr_bv = ae_in->ae_output.cur_bv;
			ISP_LOGD("ae_in->ae_output.cur_bv:%d lux", ae_in->ae_output.cur_bv);

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
					       cxt->commn_cxt.prv_size.w, cxt->commn_cxt.prv_size.h,
					       awb_output->ct, awb_output->gain.r, awb_output->gain.b,
					       ae_in->ae_output.is_stab, ae_in);
			ISP_TRACE_IF_FAIL(ret, ("fail to do alsc_calc"));
		}
	}
	time_end = ispalg_get_sys_timestamp();
	ISP_LOGV("SYSTEM_TEST-smart:%d ms", (cmr_u32)(time_end - time_start));

	isp_cur_bv = ae_in->ae_output.cur_bv;
	isp_cur_ct = awb_output->ct;

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
		ae_info->ae_rlt_info.near_stab = ae_in->ae_output.near_stab;
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
		cxt->ae_info.ae_rlt_info.is_stab = ae_result.ae_output.is_stab;
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
	cmr_u32 cur_flicker = 0;
	cmr_u32 cur_exp_flag = 0;
	cmr_s32 ae_exp_flag = 0;
	cmr_u32 app_mode = 0;
	float ae_exp = 0.0;
	struct isp_statis_info *statis_info = (struct isp_statis_info *)data;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct afl_proc_in afl_input;
	struct afl_ctrl_proc_out afl_output = {0, 0, 0};
	struct afl_ctrl_proc_out afl_info = {0, 0, 0};

	memset(&afl_input, 0, sizeof(afl_input));
	memset(&afl_output, 0, sizeof(afl_output));
	cxt->afl_cxt.hw_bypass = 1;

	if (cxt->afl_cxt.sw_bypass) {
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_STSTIS_BUF, statis_info, NULL);
		if (ret) {
			ISP_LOGE("fail to set statis buf");
		}
		cxt->afl_cxt.hw_bypass = 0;
		ret = isp_dev_access_ioctl(cxt->dev_access_handle,
					ISP_DEV_SET_AFL_NEW_BYPASS,
					&cxt->afl_cxt.hw_bypass, NULL);
		goto exit;
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

		ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_GET_APP_MODE, NULL, &app_mode);
		ISP_TRACE_IF_FAIL(ret, ("fail to AE_GET_APP_MODE"));
		ISP_LOGV("app_mode %d", app_mode);

	}

	if (cxt->ops.afl_ops.ioctrl) {
		ret = cxt->ops.afl_ops.ioctrl(cxt->afl_cxt.handle, AFL_GET_INFO, (void *)&afl_info, NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to AFL_GET_INFO"));
	}

	cxt->afl_cxt.afl_statis_info = *statis_info;
	afl_input.ae_stat_ptr = &cxt->aem_stats_data;
	afl_input.ae_exp_flag = ae_exp_flag;
	afl_input.cur_exp_flag = cur_exp_flag;
	afl_input.cur_flicker = cur_flicker;
	afl_input.vir_addr = statis_info->uaddr;
	afl_input.afl_mode = cxt->afl_cxt.afl_mode;
	afl_input.private_len = sizeof(struct isp_statis_info);
	afl_input.private_data = &cxt->afl_cxt.afl_statis_info;
	afl_input.ae_win_num.w = cxt->ae_cxt.win_num.w;
	afl_input.ae_win_num.h = cxt->ae_cxt.win_num.h;
	afl_input.max_fps = afl_info.max_fps;
	afl_input.app_mode = app_mode;
	ISP_LOGV("afl_mode %d\n",  cxt->afl_cxt.afl_mode);

	if (cxt->ops.afl_ops.process) {
		ret = cxt->ops.afl_ops.process(cxt->afl_cxt.handle, &afl_input, &afl_output);
		ISP_TRACE_IF_FAIL(ret, ("fail to afl process"));
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
			cmr_u32 *ptr, *dst;
			cmr_u32 *zoom_ratio;
			struct isp_statis_info *statis_info = NULL;

			statis_info = (struct isp_statis_info *)in_ptr;
			ptr = (cmr_u32 *)statis_info->uaddr;
			dst = &af_temp[0][0];
			blk_num = cxt->af_cxt.win_num.x * cxt->af_cxt.win_num.y;
#ifndef CONFIG_ISP_2_5
			for (i = 0; i < blk_num; i++) {
				af_temp[i][0] = *ptr++;
				af_temp[i][1] = *ptr++;
				af_temp[i][2] = *ptr++;
				ptr++;
			}
#else
			for (i = 0; i < sizeof(af_temp)/sizeof(af_temp[0][0]); i++) {
				*dst++ = *ptr++;
			}
#endif
			ISP_LOGV("data: %x %x %x %x %x %x\n",
				af_temp[0][0], af_temp[0][1], af_temp[0][2], af_temp[1][0], af_temp[1][1], af_temp[1][2]);
			ISP_LOGV("data: %x %x %x %x %x %x\n",
				af_temp[16][0], af_temp[16][1], af_temp[16][2], af_temp[17][0], af_temp[17][1], af_temp[17][2]);

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

			/* set zoom ratio to af ctrl */
			zoom_ratio = &statis_info->zoom_ratio;
			cxt->cur_ratio = *(zoom_ratio);
			if (cxt->last_ratio != cxt->cur_ratio) {
				if (cxt->ops.af_ops.ioctrl) {
					ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle, AF_CMD_SET_ZOOM_RATIO, (void *)zoom_ratio, NULL);
				}
				cxt->last_ratio = cxt->cur_ratio;
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
	void *pdaf_info = NULL;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_statis_info *statis_info = (struct isp_statis_info *)in_ptr;
	struct pdaf_ctrl_process_in pdaf_param_in;
	struct pdaf_ctrl_param_out pdaf_param_out;
	UNUSED(data_type);

	memset((void *)&pdaf_param_in, 0x00, sizeof(pdaf_param_in));
	memset((void *)&pdaf_param_out, 0x00, sizeof(pdaf_param_out));

	u_addr = statis_info->uaddr;
	pdaf_param_in.u_addr = statis_info->uaddr;

	switch (cxt->pdaf_cxt.pdaf_support) {
	case SENSOR_PDAF_TYPE1_ENABLE:
		pdaf_info = (cmr_s32 *)(u_addr);

		if (cxt->ops.af_ops.ioctrl) {
			ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle, AF_CMD_SET_TYPE1_PD_INFO, pdaf_info, NULL);
			ISP_TRACE_IF_FAIL(ret, ("fail to AF_CMD_SET_TYPE1_PD_INFO"));
		}
		break;
	case SENSOR_PDAF_TYPE2_ENABLE:
	case SENSOR_PDAF_TYPE3_ENABLE:
	case SENSOR_DUAL_PDAF_ENABLE:
		if (cxt->ops.pdaf_ops.ioctrl)
			cxt->ops.pdaf_ops.ioctrl(cxt->pdaf_cxt.handle, PDAF_CTRL_CMD_GET_BUSY, NULL, &pdaf_param_out);
		ISP_LOGV("pdaf_is_busy=%d\n", pdaf_param_out.is_busy);
		if (!pdaf_param_out.is_busy && !cxt->pdaf_cxt.sw_bypass) {
			if (cxt->ops.pdaf_ops.process)
				ret = cxt->ops.pdaf_ops.process(cxt->pdaf_cxt.handle, &pdaf_param_in, NULL);
		}
		break;
	default:
		ISP_LOGV("Invalid pdaf param:%d", cxt->pdaf_cxt.pdaf_support);
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
	struct sensor_embedded_info sensor_ebd_info;
	UNUSED(data_type);

	memset((void *)&sensor_ebd_info, 0x00, sizeof(sensor_ebd_info));
	sensor_ebd_info.embedded_data = (cmr_u8 *)statis_info->uaddr;
	if (cxt->ioctrl_ptr->sns_ioctl) {
		cxt->ioctrl_ptr->sns_ioctl(cxt->ioctrl_ptr->caller_handler,
					CMD_SNS_IC_GET_EBD_PARSE_DATA,
					&sensor_ebd_info);
	}

	cxt->ae_cxt.ebd_info.frame_id = sensor_ebd_info.parse_data.frame_count;
	cxt->ae_cxt.ebd_info.frame_id_valid =
				sensor_ebd_info.frame_count_valid;
	cxt->ae_cxt.ebd_info.exposure = sensor_ebd_info.parse_data.shutter;
	cxt->ae_cxt.ebd_info.exposure_valid =
				sensor_ebd_info.shutter_valid;
	cxt->ae_cxt.ebd_info.again = sensor_ebd_info.parse_data.again;
	cxt->ae_cxt.ebd_info.again_valid =
				sensor_ebd_info.again_valid;

	cxt->ae_cxt.ebd_info.dgain_gr = sensor_ebd_info.parse_data.dgain_gr;
	cxt->ae_cxt.ebd_info.dgain_r = sensor_ebd_info.parse_data.dgain_r;
	cxt->ae_cxt.ebd_info.dgain_b = sensor_ebd_info.parse_data.dgain_b;
	cxt->ae_cxt.ebd_info.dgain_gb = sensor_ebd_info.parse_data.dgain_gb;
	cxt->ae_cxt.ebd_info.gain = sensor_ebd_info.parse_data.gain;
	cxt->ae_cxt.ebd_info.dgain_valid = sensor_ebd_info.dgain_valid;

	ISP_LOGV("frame id %x %d, shutter %x %d, again %x %d",
			cxt->ae_cxt.ebd_info.frame_id,
			cxt->ae_cxt.ebd_info.frame_id_valid,
			cxt->ae_cxt.ebd_info.exposure,
			cxt->ae_cxt.ebd_info.exposure_valid,
			cxt->ae_cxt.ebd_info.again,
			cxt->ae_cxt.ebd_info.again_valid);

	ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_STSTIS_BUF, statis_info, NULL);

	return ret;
}

cmr_int ispalg_ai_process(cmr_handle isp_alg_handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	enum ai_status ai_sta = AI_STATUS_MAX;

	ISP_CHECK_HANDLE_VALID(isp_alg_handle);

	if (cxt->ops.ai_ops.ioctrl) {
		ret = cxt->ops.ai_ops.ioctrl(cxt->ai_cxt.handle, AI_GET_STATUS, (void *)(&ai_sta), NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to AI_GET_STATUS"));
	}
	if (AI_STATUS_PROCESSING != ai_sta) {
		ISP_LOGV("AI detection doesn't work.");
		return ret;
	}

	cxt->ai_cxt.ae_param.timestamp = (cmr_u64)(cxt->ai_cxt.ae_param.sec) * 1000000000 +
		(cmr_u64)(cxt->ai_cxt.ae_param.usec) * 1000;

	cxt->ai_cxt.ae_param.ae_stat.r_info = cxt->aem_stats_data.r_info;
	cxt->ai_cxt.ae_param.ae_stat.g_info = cxt->aem_stats_data.g_info;
	cxt->ai_cxt.ae_param.ae_stat.b_info = cxt->aem_stats_data.b_info;
	cxt->ai_cxt.ae_param.stable = cxt->ae_info.ae_rlt_info.is_stab;

	cxt->ai_cxt.ae_param.blk_num_hor = cxt->ae_cxt.win_num.w;
	cxt->ai_cxt.ae_param.blk_num_ver = cxt->ae_cxt.win_num.h;
	cxt->ai_cxt.ae_param.curr_bv = cxt->curr_bv;
	ISP_LOGV("ai ae info: blk_num_hor: %d, blk_num_ver: %d.",
		cxt->ai_cxt.ae_param.blk_num_hor, cxt->ai_cxt.ae_param.blk_num_ver);
	ISP_LOGD("ai ae info: frame_id: %d, timestamp: %llu.",
		cxt->ai_cxt.ae_param.frame_id, (unsigned long long)cxt->ai_cxt.ae_param.timestamp);

	if (cxt->ops.ai_ops.ioctrl) {
		ret = cxt->ops.ai_ops.ioctrl(cxt->ai_cxt.handle, AI_SET_AE_PARAM, (void *)(&cxt->ai_cxt.ae_param), NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to AI_SET_AE_PARAM"));
	}

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

cmr_int ispalg_aethread_proc(struct cmr_msg *message, void *p_data)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_u32 is_raw_capture = 0;
	char value[PROPERTY_VALUE_MAX];
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)p_data;
	CMR_MSG_INIT(message1);

	if (!message || !p_data) {
		ISP_LOGE("fail to check input param ");
		goto exit;
	}
	ISP_LOGV("message.msg_type 0x%x, data %p", message->msg_type, message->data);

	/* set priority to -10 */
	/* same as grab/aectrl to make sure whole AE process priority */
	if (cxt->aethd_pri_set == 0) {
		cmr_s32 priority = -10;

		setpriority(PRIO_PROCESS, 0, priority);
		/* todo: set_sched_policy() applied in Android P, Android Q needs new API for it */
		//set_sched_policy(0, SP_FOREGROUND);
		cxt->aethd_pri_set = 1;
		ISP_LOGI("set priority to %d", priority);
	}

	switch (message->msg_type) {
	case ISP_EVT_AE: {
		struct isp_statis_info *statis_info = (struct isp_statis_info *)message->data;

		ISP_LOGV("aem no.%d, timestamp %03d.%06d\n", statis_info->frame_id, statis_info->sec, statis_info->usec);

		ret = ispalg_aem_stats_parser((cmr_handle) cxt, message->data);
		struct isp_awb_statistic_info *ae_stat_ptr = (struct isp_awb_statistic_info *)&cxt->aem_stats_data;
		if (cxt->is_multi_mode)
			ISP_LOGV("is_master :%d\n", cxt->is_master);
		isp_br_ioctrl(cxt->sensor_role, SET_STAT_AWB_DATA, ae_stat_ptr, NULL);
		ret = ispalg_ai_process((cmr_handle)cxt);
		break;
	}
	case ISP_EVT_SOF: {
		struct sprd_irq_info *irq_info = (struct sprd_irq_info *)message->data;

		ISP_LOGV("sof no.%d, timestamp %03d.%06d\n", irq_info->frame_id, irq_info->sec, irq_info->usec);

		if (cxt->fw_started == 0) {
			/* workaround for raw capture with flash
			  * because no statis data for raw, force AE process running to return flash messege to HAL
			  */
			property_get("persist.vendor.cam.raw.mode", value, "jpeg");
			if (!strcmp(value, "raw"))
				is_raw_capture = 1;
			if (is_raw_capture == 0)
				break;
			ISP_LOGD("force AE process for raw capture\n");
			cxt->aem_is_update = 1;
			ret = ispalg_start_ae_process((cmr_handle) cxt);
			break;
		}

		ret = ispalg_ae_process((cmr_handle) cxt);
		if (ret)
			ISP_LOGE("fail to start ae process");
		ret = ispalg_awb_process((cmr_handle) cxt);
		if (ret)
			ISP_LOGE("fail to start awb process");
		cxt->aem_is_update = 0;

		ret = ispalg_handle_sensor_sof((cmr_handle) cxt);

		message1.msg_type = ISP_EVT_CFG;
		message1.sync_flag = CMR_MSG_SYNC_NONE;
		ret = cmr_thread_msg_send(cxt->thr_handle, &message1);
		break;
	}
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
	nsecs_t cur_time;
	cmr_u32 timems_diff;
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
		ret = ispalg_bayerhist_stats_parser((cmr_handle) cxt, message->data);
		break;
	case ISP_EVT_HIST2:
		ret = ispalg_hist2_process((cmr_handle) cxt, message->data);
		break;
	case ISP_EVT_3DNR:
		ret = ispalg_3dnr_statis_parser((cmr_handle) cxt, message->data);
		break;
	case ISP_EVT_CFG:
		cur_time = ispalg_get_sys_timestamp();
		timems_diff = (cmr_u32)(cur_time - cxt->last_sof_time);
		if (timems_diff < MIN_FRAME_INTERVAL_MS) {
			ISP_LOGW("time interval is too small: %d\n", timems_diff);
		}
		cxt->last_sof_time = cur_time;
		ret = ispalg_cfg_param(cxt, 0);
		break;
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

	if (evt == ISP_EVT_SOF && data) {
		struct sprd_irq_info *irq_info = (struct sprd_irq_info *)data;;
		ISP_LOGV("sof no.%d, timestamp %03d.%06d\n",
			irq_info->frame_id, irq_info->sec, irq_info->usec);
	}
	if (evt == ISP_EVT_AE && data) {
		struct isp_statis_info *statis_info = (struct isp_statis_info *)data;
		ISP_LOGV("aem no.%d, timestamp %03d.%06d\n",
			statis_info->frame_id, statis_info->sec, statis_info->usec);
	}

	message.msg_type = evt;
	message.sub_msg_type = 0;
	message.sync_flag = CMR_MSG_SYNC_NONE;
	message.alloc_flag = 1;
	message.data = data;
	if (ISP_EVT_AF == message.msg_type) {
		message.sub_msg_type = AF_DATA_AFM_STAT;
		ret = cmr_thread_msg_send(cxt->thr_afhandle, &message);
	} else if (evt == ISP_EVT_AE || evt == ISP_EVT_SOF) {
		ret = cmr_thread_msg_send(cxt->thr_aehandle, &message);
	} else {
		ret = cmr_thread_msg_send(cxt->thr_handle, &message);
	}
	if (ret) {
		ISP_LOGE("fail to send a message, evt is %ld, ret: %ld, cxt: %p, af_handle: %p, handle: %p",
			evt, ret, cxt, cxt->thr_afhandle, cxt->thr_handle);
		free(message.data);
	}
}

static cmr_int ispalg_create_thread(cmr_handle isp_alg_handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;

	ret = cmr_thread_create(&cxt->thr_handle,
			ISP_THREAD_QUEUE_NUM,
			ispalg_thread_proc, (void *)cxt);

	if (CMR_MSG_SUCCESS != ret) {
		ISP_LOGE("fail to create isp algfw  process thread");
		return ISP_ERROR;
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
		cmr_thread_destroy(cxt->thr_handle);
		cxt->thr_handle = (cmr_handle) NULL;
		return ISP_ERROR;
	}
	ret = cmr_thread_set_name(cxt->thr_afhandle, "afstats");
	if (CMR_MSG_SUCCESS != ret) {
		ISP_LOGE("fail to set af name");
		ret = CMR_MSG_SUCCESS;
	}



	ret = cmr_thread_create(&cxt->thr_aehandle,
			ISP_THREAD_QUEUE_NUM,
			ispalg_aethread_proc, (void *)cxt);

	if (CMR_MSG_SUCCESS != ret) {
		ISP_LOGE("fail to create algae process thread");
		cmr_thread_destroy(cxt->thr_handle);
		cxt->thr_handle = (cmr_handle) NULL;
		cmr_thread_destroy(cxt->thr_afhandle);
		cxt->thr_afhandle = (cmr_handle) NULL;
		return ISP_ERROR;
	}
	ret = cmr_thread_set_name(cxt->thr_aehandle, "algae");
	if (CMR_MSG_SUCCESS != ret) {
		ISP_LOGE("fail to set algae name");
		ret = CMR_MSG_SUCCESS;
	}

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

	if (cxt->thr_aehandle) {
		ret = cmr_thread_destroy(cxt->thr_aehandle);
		if (!ret) {
			cxt->thr_aehandle = (cmr_handle) NULL;
		} else {
			ISP_LOGE("fail to destroy algae process thread");
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
	pm_init_input.is_4in1_sensor = cxt->is_4in1_sensor;

	cxt->handle_pm = isp_pm_init(&pm_init_input, &pm_init_output);
	if (PNULL == cxt->handle_pm) {
		ISP_LOGE("fail to do isp_pm_init");
		return ISP_ERROR;
	}
	cxt->sn_cxt.sn_raw_info = sensor_raw_info_ptr;
	cxt->commn_cxt.multi_nr_flag = pm_init_output.multi_nr_flag;
	cxt->commn_cxt.src.w = input_ptr->size.w;
	cxt->commn_cxt.src.h = input_ptr->size.h;
	cxt->commn_cxt.prv_size.w = input_ptr->size.w;
	cxt->commn_cxt.prv_size.h = input_ptr->size.h;
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
	if (ISP_SUCCESS != ret || 0 == output.param_num) {
		ISP_LOGW("warn: get dual flash param, ret %ld, num %d\n", ret, output.param_num);
	} else {
		param_data = output.param_data;
		for (i = 0; i < output.param_num; i++) {
			if (NULL != param_data->data_ptr && (dflash_num < AE_MAX_PARAM_NUM)) {
				ae_input.flash_tuning[dflash_num].param = param_data->data_ptr;
				ae_input.flash_tuning[dflash_num].size = param_data->data_size;
				++dflash_num;
			}
			++param_data;
		}
	}

	/* 4in1 bv_thd */
	if (cxt->is_4in1_sensor) {
		memset(&output, 0, sizeof(output));
	        ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_4IN1_PARAM, NULL, &output);
	        if (ISP_SUCCESS == ret && 0 == output.param_num) {
		        param_data = output.param_data;
		        for (i = 0; i < output.param_num; ++i) {
		                if (NULL != param_data->data_ptr) {
		                        ae_input.bv_thd = *(cmr_s16 *)param_data->data_ptr;
		                }
		                ++param_data;
		        }
	        } else {
		        ISP_LOGW("warn: no 4in1 param for 4in1 sensor!");
			ae_input.bv_thd = 0;
	        }
	}

	memset(&output, 0, sizeof(output));
	ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_AE_SYNC, NULL, &output);
	if (ISP_SUCCESS != ret || 1 != output.param_num) {
		ISP_LOGW("warn: get ae sync param ret %ld, num %d\n", ret, output.param_num);
	} else {
		param_data = output.param_data;
		if (param_data->data_ptr != NULL) {
			ae_input.ae_sync_param.param = param_data->data_ptr;
			ae_input.ae_sync_param.size = param_data->data_size;
		}
	}

	memset(&output, 0, sizeof(output));
	ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_INIT_AE, NULL, &output);
	if (ISP_SUCCESS != ret || output.param_num == 0) {
		ISP_LOGW("warn: get ae init param, ret %ld, num %d\n", ret, output.param_num);
	} else {
		param_data = output.param_data;
		if (param_data)
			ae_input.has_force_bypass = param_data->user_data[0];
		for (i = 0; (i < output.param_num) && param_data; i++) {
			if (NULL != param_data->data_ptr && (num < AE_MAX_PARAM_NUM)) {
				ae_input.param[num].param = param_data->data_ptr;
				ae_input.param[num].size = param_data->data_size;
				++num;
			}
			++param_data;
		}
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
	ae_input.ebd_support = cxt->ebd_cxt.ebd_support;
	ae_input.ae_set_cb = ispalg_ae_set_cb;

	ret = ispalg_get_aem_param(cxt, &aem_info);
	cxt->ae_cxt.win_num.w = aem_info.blk_num.w;
	cxt->ae_cxt.win_num.h = aem_info.blk_num.h;

	ae_input.monitor_win_num.w = cxt->ae_cxt.win_num.w;
	ae_input.monitor_win_num.h = cxt->ae_cxt.win_num.h;
	ae_input.sensor_role = cxt->sensor_role;
 	ae_input.is_master = cxt->is_master;
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
	case ISP_WIDETELEULTRAWIDE:
		ae_input.is_multi_mode = ISP_ALG_TRIBLE_W_T_UW;
		break;
	default:
		ae_input.is_multi_mode = ISP_ALG_SINGLE;
		break;
	}

	// TODO joseph run AE separately until lib ready
	if (ae_input.is_multi_mode == ISP_ALG_TRIBLE_W_T_UW) {
		char value[PROPERTY_VALUE_MAX] = { 0x00 };
		property_get("persist.vendor.cam.debug.ae_sync", value, "0");

		if (atoi(value) == 0) {
			ae_input.is_multi_mode = ISP_ALG_SINGLE;
		}
	}

	ISP_LOGI("camera_id %u, is_master %u, is_multi_mode %u, sensor_role %u",
			ae_input.camera_id, ae_input.is_master,
			ae_input.is_multi_mode, ae_input.sensor_role);

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
	param.stat_img_size_ae.w = info.win_num.w;
	param.stat_img_size_ae.h = info.win_num.h;
	param.stat_win_size.w = info.win_size.w;
	param.stat_win_size.h = info.win_size.h;

	ISP_LOGI("awb get ae win %d %d %d %d %d %d\n", info.trim.x, info.trim.y,
		info.win_size.w, info.win_size.h, info.win_num.w, info.win_num.h);

	ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_INIT_AWB, &input, &output);
	ISP_TRACE_IF_FAIL(ret, ("fail to get awb init param"));
	if (ret == ISP_SUCCESS && output.param_data != NULL) {
		param.tuning_param = output.param_data->data_ptr;
		param.param_size = output.param_data->data_size;
	}
	param.lib_param = cxt->lib_use_info->awb_lib_info;
	ISP_LOGV("param addr is %p size %d", param.tuning_param, param.param_size);

	param.otp_info_ptr = cxt->otp_data;
	param.is_master = cxt->is_master;
	param.sensor_role = cxt->sensor_role;
	param.color_support = cxt->awb_cxt.color_support;

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
	case ISP_WIDETELEULTRAWIDE:
		param.is_multi_mode = ISP_ALG_TRIBLE_W_T_UW;
		break;
	default:
		param.is_multi_mode = ISP_ALG_SINGLE;
		break;
	}
	ISP_LOGI("is_master %u, sensor_role %u, is_multi_mode %u",
			cxt->is_master, cxt->sensor_role, cxt->is_multi_mode);
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
		ISP_LOGW("warn: no atm param. ret %ld, num %d\n", ret, pm_output.param_num);
	} else {
		if (pm_output.param_data->data_ptr) {
			smart_init_param.atm_info = pm_output.param_data->data_ptr;
			smart_init_param.atm_size = pm_output.param_data->data_size;
		}
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
	cmr_u32 zoom_ratio;
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
	af_input.pdaf_type = cxt->pdaf_cxt.pdaf_support;
	cxt->af_cxt.sw_bypass = 0;

	//get af tuning parameters
	memset((void *)&output, 0, sizeof(output));
	ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_INIT_AF_NEW, NULL, &output);
	if(ISP_SUCCESS == ret && NULL != output.param_data){
		/* get bypass flag from pm */
		cxt->af_cxt.sw_bypass = output.param_data[0].user_data[0];
		ISP_LOGD("cxt->af_cxt.sw_bypass %d\n", cxt->af_cxt.sw_bypass);
		if (1 == is_af_support) {
			af_input.aftuning_data = output.param_data[0].data_ptr;
			af_input.aftuning_data_len = output.param_data[0].data_size;
		}
	}

	if(1 == is_af_support) {
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

		//[TOF_tuning]get tof tuning parameters
		memset((void *)&output, 0, sizeof(output));
		ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_INIT_TOF, NULL, &output);
		if(ISP_SUCCESS == ret && NULL != output.param_data){
			af_input.toftuning_data = output.param_data[0].data_ptr;
			af_input.toftuning_data_len = output.param_data[0].data_size;
		}
	}

	af_input.is_master = cxt->is_master;
	af_input.sensor_role = cxt->sensor_role;
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
	case ISP_BLUR_PORTRAIT:
		af_input.is_multi_mode = AF_ALG_BLUR_PORTRAIT;
		break;
	case ISP_WIDETELEULTRAWIDE:
		af_input.is_multi_mode = AF_ALG_TRIBLE_W_T_UW;
		break;
	default:
		af_input.is_multi_mode = AF_ALG_SINGLE;
		break;
	}

	ISP_LOGI("camera_id %u, is_master %u, is_multi_mode %u, sensor_role %u, pdaf_type %d",
			af_input.camera_id, af_input.is_master, af_input.is_multi_mode,
			af_input.sensor_role, af_input.pdaf_type);

	af_input.otp_info_ptr = cxt->otp_data;
	af_input.br_ctrl = isp_br_ioctrl;

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

	/* init ratio for 4x zoom */
	if (cxt->ops.af_ops.ioctrl) {
		zoom_ratio = 1000;
		ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle, AF_CMD_SET_ZOOM_RATIO, (void *)&zoom_ratio, NULL);
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
	struct alsc_ver_info lsc_ver = { 0 };
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

	if ((ret != ISP_SUCCESS) || (io_pm_output.param_data == NULL)) {
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

	/*  alsc tuning param should be private and parsed in alsc lib */
	//struct lsc2_tune_param* param = (struct lsc2_tune_param*)lsc_param.tune_param_ptr;
	cxt->lsc_cxt.full_size_width = lsc_tab_param_ptr->resolution.w;
	cxt->lsc_cxt.full_size_height = lsc_tab_param_ptr->resolution.h;
	cxt->lsc_cxt.full_size_grid = lsc_tab_param_ptr->lsc_info.grid;

	ISP_LOGI("[ALSC] full_img_size[%d,%d], table_w_h_gird[%d,%d,%d]",
		lsc_tab_param_ptr->resolution.w,lsc_tab_param_ptr->resolution.h,
		lsc_tab_param_ptr->lsc_info.gain_w,lsc_tab_param_ptr->lsc_info.gain_h,
		lsc_tab_param_ptr->lsc_info.grid);

	/* gain_pattern is for input tab generated by ISP tool
	  * the following pattern translating due to history reason
	  * do not modify it unless ISP tool changed */
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
	/* change_pattern_flag =1 means output_gain_pattern will be used by LSC alg lib
	  * output_gain_pattern is LSC algo lib output tab pattern
	  * this pattern is for hardware reading and using
	  * in dcam_if_lite r1p0, lsc block always apply BGGR pattern no matter what's the sensor pattern
	  * do not modify it unless hardware changed */
	lsc_param.output_gain_pattern = LSC_GAIN_PATTERN_BGGR;
	lsc_param.change_pattern_flag = 1;
	ISP_LOGV("alsc_init, gain_pattern=%d, output_gain_pattern=%d, flag=%d",
		lsc_param.gain_pattern, lsc_param.output_gain_pattern, lsc_param.change_pattern_flag);

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

	if (cxt->ops.lsc_ops.ioctrl)
		ret = cxt->ops.lsc_ops.ioctrl(lsc_adv_handle, ALSC_GET_VER, NULL, (void *)&lsc_ver);
	if (ISP_SUCCESS != ret) {
		ISP_LOGE("fail to Get ALSC ver info!");
		cxt->lsc_cxt.LSC_SPD_VERSION = 4;
	} else {
		cxt->lsc_cxt.LSC_SPD_VERSION =  lsc_ver.LSC_SPD_VERSION;
		ISP_LOGD("Get ALSC ver: %d\n", lsc_ver.LSC_SPD_VERSION);
	}

	return ret;
}

static cmr_int ispalg_ai_init(struct isp_alg_fw_context *cxt)
{
	cmr_int ret = ISP_SUCCESS;
	struct ai_init_in ai_input;
	struct ai_init_out result;

	if (!cxt) {
		ret = ISP_PARAM_ERROR;
		goto exit;
	}

	memset((void *)&result, 0, sizeof(result));
	memset((void *)&ai_input, 0, sizeof(ai_input));

	ai_input.caller_handle = (cmr_handle)cxt;
	ai_input.ai_set_cb = ispalg_ai_set_cb;

	if (cxt->ops.ai_ops.init)
		ret = cxt->ops.ai_ops.init(&ai_input, &cxt->ai_cxt.handle, (cmr_handle)&result);
exit:
	ISP_LOGI("done %ld", ret);
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
		cxt->af_cxt.sw_bypass |= val; /* af_bypass(pm) or this */

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

static cmr_int ispalg_tof_init(struct isp_alg_fw_context *cxt)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_handle  tof_handle = NULL;
	struct tof_ctrl_init_in tof_ctrl_init;

	if (!cxt->af_cxt.tof_support) {
		ISP_LOGI("tof don't support !");
		return ret;
	}

	tof_ctrl_init.tof_set_cb = ispalg_tof_set_cb;
	tof_ctrl_init.caller_handle = (cmr_handle) cxt;

	if (NULL == cxt->tof_handle) {
		if (cxt->ops.tof_ops.init) {
			ret = cxt->ops.tof_ops.init(&tof_ctrl_init, &tof_handle);
			if (NULL == tof_handle) {
				ISP_LOGE("fail to do tof init");
				return ISP_ERROR;
			}
		}

		cxt->tof_handle = tof_handle;
	}

	return ret;
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

	ret = ispalg_tof_init(cxt);
	ISP_TRACE_IF_FAIL(ret, ("fail to do tof_init"));

	ret = ispalg_lsc_init(cxt);
	ISP_TRACE_IF_FAIL(ret, ("fail to do lsc_init"));

	ret = ispalg_ai_init(cxt);
	ISP_TRACE_IF_FAIL(ret, ("fail to do ai_init"));
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

	if (cxt->ops.tof_ops.deinit)
		cxt->ops.tof_ops.deinit(&cxt->tof_handle);

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

	if (cxt->ops.ai_ops.deinit)
		cxt->ops.ai_ops.deinit(&cxt->ai_cxt.handle);
	ISP_LOGI("done");
	return ISP_SUCCESS;
}

static cmr_int ispalg_load_library(cmr_handle adpt_handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)adpt_handle;

	cxt->ispalg_lib_handle = dlopen(LIBCAM_ALG_FILE, RTLD_NOW);
	if (!cxt->ispalg_lib_handle) {
		ISP_LOGE("fail to dlopen (%s)",dlerror());
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

	cxt->ops.tof_ops.init = dlsym(cxt->ispalg_lib_handle, "tof_ctrl_init");
	if (!cxt->ops.tof_ops.init) {
		ISP_LOGE("fail to dlsym tof_ops.init");
		goto error_dlsym;
	}
	cxt->ops.tof_ops.deinit = dlsym(cxt->ispalg_lib_handle, "tof_ctrl_deinit");
	if (!cxt->ops.tof_ops.deinit) {
		ISP_LOGE("fail to dlsym tof_ops.deinit");
		goto error_dlsym;
	}

	cxt->ops.ai_ops.init = dlsym(cxt->ispalg_lib_handle, "ai_ctrl_init");
	if (!cxt->ops.ai_ops.init) {
		ISP_LOGE("fail to dlsym ai_ops.init");
		goto error_dlsym;
	}
	cxt->ops.ai_ops.deinit = dlsym(cxt->ispalg_lib_handle, "ai_ctrl_deinit");
	if (!cxt->ops.ai_ops.deinit) {
		ISP_LOGE("fail to dlsym ai_ops.deinit");
		goto error_dlsym;
	}
	cxt->ops.ai_ops.ioctrl = dlsym(cxt->ispalg_lib_handle, "ai_ctrl_ioctrl");
	if (!cxt->ops.ai_ops.ioctrl) {
		ISP_LOGE("fail to dlsym ai_ops.ioctrl");
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
	struct isp_rgb_aem_info aem_info;
	cmr_u32 max_fps = 0;

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
	/* ae_param.zsl_flag = param_ptr->capture_mode; */
	ae_param.noramosaic_4in1 = cxt->noramosaic_4in1;
	ae_param.resolution_info.frame_size.w = cxt->commn_cxt.prv_size.w;
	ae_param.resolution_info.frame_size.h = cxt->commn_cxt.prv_size.h;
	ae_param.resolution_info.frame_line = cxt->commn_cxt.input_size_trim[cxt->commn_cxt.param_index].frame_line;
	ae_param.resolution_info.line_time = cxt->commn_cxt.input_size_trim[cxt->commn_cxt.param_index].line_time;
	ae_param.resolution_info.sensor_size_index = cxt->commn_cxt.param_index;
	ae_param.resolution_info.snr_setting_max_fps = param_ptr->sensor_fps.max_fps;
	ae_param.is_snapshot = param_ptr->is_snapshot;
	ae_param.dv_mode = param_ptr->dv_mode;

	ae_param.sensor_fps.mode = param_ptr->sensor_fps.mode;
	ae_param.sensor_fps.max_fps = param_ptr->sensor_fps.max_fps;
	ae_param.sensor_fps.min_fps = param_ptr->sensor_fps.min_fps;
	ae_param.sensor_fps.is_high_fps = param_ptr->sensor_fps.is_high_fps;
	ae_param.sensor_fps.high_fps_skip_num = param_ptr->sensor_fps.high_fps_skip_num;

	ret = ispalg_get_aem_param(cxt, &aem_info);
	cxt->ae_cxt.win_num.w = aem_info.blk_num.w;
	cxt->ae_cxt.win_num.h = aem_info.blk_num.h;

	ae_param.win_num.h = cxt->ae_cxt.win_num.h;
	ae_param.win_num.w = cxt->ae_cxt.win_num.w;
	ae_param.blk_num = ae_param.win_num;

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

	max_fps = ae_param.sensor_fps.max_fps;
	if (cxt->ops.afl_ops.ioctrl) {
		ret = cxt->ops.afl_ops.ioctrl(cxt->afl_cxt.handle, AFL_SET_MAX_FPS, (void *)&max_fps, NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to AFL_MAX_FPS"));
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

	if (cxt->takepicture_mode == CAMERA_ISP_SIMULATION_MODE) {
		awbc_cfg.r_gain = cxt->simu_param.awb_gain_r;
		awbc_cfg.g_gain = cxt->simu_param.awb_gain_g;
		awbc_cfg.b_gain = cxt->simu_param.awb_gain_b;
		awbc_cfg.r_offset = 0;
		awbc_cfg.g_offset = 0;
		awbc_cfg.b_offset = 0;
		ct = cxt->simu_param.smart_ct;
		bv = cxt->simu_param.smart_bv;
		bv_gain = cxt->simu_param.gain;
	} else {
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
	}

	BLOCK_PARAM_CFG(ioctl_input, ioctl_data,
			ISP_PM_BLK_AWBC, ISP_BLK_AWB_NEW,
			&awbc_cfg,  sizeof(awbc_cfg));
	ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_AWB, (void *)&ioctl_input, NULL);

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
		smart_proc_in.ai_scene_id = cxt->commn_cxt.ai_scene_id;
		smart_proc_in.cal_para.gamma_tab = cxt->smart_cxt.tunning_gamma_cur;
		isp_prepare_atm_param(isp_alg_handle, &smart_proc_in);
		cxt->smart_cxt.atm_is_set = 0;

		for (i = 0; i < num; i++) {
			smart_proc_in.mode_flag = cxt->commn_cxt.isp_pm_mode[i];
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

static cmr_int ispalg_update_smart_param(cmr_handle isp_alg_handle)
{

	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct smart_proc_input smart_proc_in;
	struct isp_pm_ioctl_input ioctl_output = { PNULL, 0 };
	struct isp_pm_ioctl_input ioctl_input;
	struct isp_pm_param_data ioctl_data;
	struct isptool_scene_param scene_param;

	memset(&smart_proc_in, 0, sizeof(smart_proc_in));
	memset(&scene_param, 0, sizeof(scene_param));

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

	isp_sim_get_scene_parm(&scene_param);

	if ((cxt->smart_cxt.sw_bypass == 0) && (0 != scene_param.gain) && (0 != scene_param.smart_ct)) {
		int i, num = (cxt->zsl_flag) ? 2 : 1;

		smart_proc_in.cal_para.bv = scene_param.smart_bv;
		smart_proc_in.cal_para.bv_gain = scene_param.gain;
		smart_proc_in.cal_para.ct = scene_param.smart_ct;
		smart_proc_in.alc_awb = cxt->awb_cxt.alc_awb;
		smart_proc_in.scene_flag = cxt->commn_cxt.scene_flag;
		smart_proc_in.cal_para.gamma_tab = cxt->smart_cxt.tunning_gamma_cur;
		isp_prepare_atm_param(isp_alg_handle, &smart_proc_in);
		cxt->smart_cxt.atm_is_set = 0;


		ISP_LOGI("bv=%d, bv_gain=%d, ct=%d, alc_awb=%d, mode_flag=%d, scene_flag=%d\n",
			smart_proc_in.cal_para.bv,
			smart_proc_in.cal_para.bv_gain,
			smart_proc_in.cal_para.ct,
			smart_proc_in.alc_awb,
			smart_proc_in.mode_flag,
			smart_proc_in.scene_flag);


		for (i = 0; i < num; i++) {
			smart_proc_in.mode_flag = cxt->commn_cxt.isp_pm_mode[i];
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
	enum isp_pm_cmd  cmd;
	cmr_s32 i =0;
	cmr_s32 lsc_result_size = 0;
	cmr_u16 *lsc_result_address_new = NULL;
	struct isp_size pic_size;
	struct isp_2d_lsc_param *lsc_tab_param_ptr;
	struct isp_lsc_info *lsc_info_new;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct alsc_fwstart_info *fwstart_info = (struct alsc_fwstart_info *)out_ptr;
	struct isp_pm_ioctl_input input = { PNULL, 0 };
	struct isp_pm_ioctl_input input2 = { PNULL, 0 };
	struct isp_pm_param_data pm_param;
	struct isp_pm_param_data param_data_grid;
	cmr_u32 adaptive_size_info[3] = {0, 0, 0};

	if (cxt->lsc_cxt.sw_bypass)
		return 0;

	pic_size = cxt->commn_cxt.prv_size;
	fwstart_info->img_width_new  = cxt->commn_cxt.prv_size.w;
	fwstart_info->img_height_new = cxt->commn_cxt.prv_size.h;

	memset(&input, 0, sizeof(struct isp_pm_ioctl_input));
	memset(&pm_param, 0, sizeof(pm_param));

	ret = ispalg_alsc_get_info(cxt);
	ISP_RETURN_IF_FAIL(ret, ("fail to do get lsc info"));
	lsc_tab_param_ptr = (struct isp_2d_lsc_param *)(cxt->lsc_cxt.lsc_tab_address);
	lsc_info_new = (struct isp_lsc_info *)cxt->lsc_cxt.lsc_info;

	ISP_LOGD("size0 (%d, %d), size1 (%d %d), lsc size (%d %d),  grid %d, gain_w %d gain_h %d\n",
		pic_size.w, pic_size.h, cxt->commn_cxt.src.w, cxt->commn_cxt.src.h,
		lsc_tab_param_ptr->resolution.w, lsc_tab_param_ptr->resolution.h,
		lsc_info_new->grid, lsc_info_new->gain_w, lsc_info_new->gain_h);

	for (i = 0; (i < (cxt->zsl_flag ? 2 : 1)) && (cxt->lsc_cxt.LSC_SPD_VERSION >= 6); i++) {

		if (i == 0) {
			pic_size = cxt->commn_cxt.prv_size;
			cmd = ISP_PM_CMD_SET_GRID0;
		} else {
			pic_size = cxt->commn_cxt.src;
			cmd = ISP_PM_CMD_SET_GRID1;
		}

		ISP_LOGI("[ALSC] lsc_pm_normalization, new_image_size[%d,%d], full_image_size[%d,%d]",
			pic_size.w, pic_size.h, cxt->lsc_cxt.full_size_width, cxt->lsc_cxt.full_size_height);

		if((cxt->lsc_cxt.full_size_width * pic_size.h) == (cxt->lsc_cxt.full_size_height * pic_size.w)){
			ISP_LOGI("[ALSC] lsc_pm_normalization case1, n binning");
			adaptive_size_info[0] = pic_size.w;
			adaptive_size_info[1] = pic_size.h;
			adaptive_size_info[2] = cxt->lsc_cxt.full_size_grid * pic_size.w / cxt->lsc_cxt.full_size_width;
			memset(&param_data_grid, 0, sizeof(param_data_grid));
			BLOCK_PARAM_CFG(input2, param_data_grid,
				ISP_PM_BLK_LSC_UPDATE_GRID,
				ISP_BLK_2D_LSC,
				&adaptive_size_info[0], 0);
			ret = isp_pm_ioctl(cxt->handle_pm, cmd, &input2, NULL);
		} else if ((pic_size.w == 1280) && (pic_size.h == 720)) {
			ISP_LOGI("[ALSC] lsc_pm_normalization case2. image size %d %d\n", pic_size.w, pic_size.h);
			adaptive_size_info[0] = cxt->commn_cxt.src.w;
			adaptive_size_info[1] = cxt->commn_cxt.src.h;
			adaptive_size_info[2] = 32;
			memset(&param_data_grid, 0, sizeof(param_data_grid));
			BLOCK_PARAM_CFG(input2, param_data_grid,
				ISP_PM_BLK_LSC_UPDATE_GRID,
				ISP_BLK_2D_LSC,
				&adaptive_size_info[0], 0);
			ret = isp_pm_ioctl(cxt->handle_pm, cmd, &input2, NULL);
		} else  if ((pic_size.w == 1920) && (pic_size.h == 1080)) {
			ISP_LOGI("[ALSC] lsc_pm_normalization case3. image size %d %d\n", pic_size.w, pic_size.h);
			adaptive_size_info[0] = cxt->commn_cxt.src.w;
			adaptive_size_info[1] = cxt->commn_cxt.src.h;
			adaptive_size_info[2] = 48;
			memset(&param_data_grid, 0, sizeof(param_data_grid));
			BLOCK_PARAM_CFG(input2, param_data_grid,
				ISP_PM_BLK_LSC_UPDATE_GRID,
				ISP_BLK_2D_LSC,
				&adaptive_size_info[0], 0);
			ret = isp_pm_ioctl(cxt->handle_pm, cmd, &input2, NULL);
		} else{
			ISP_LOGE("[ALSC] lsc_pm_normalization error");
		}
	}

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
	cmr_int isp_pdaf_type = 0;
	cmr_int i = 0;
	cmr_u32 sn_mode = 0;
	char value[PROPERTY_VALUE_MAX] = { 0x00 };
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct alsc_fwstart_info fwstart_info = { NULL, {NULL}, 0, 0, 5, 0, 0, 0, 0};
	struct afctrl_fwstart_info af_start_info;
	struct dcam_dev_vc2_control vch2_info;
	struct sensor_pdaf_info *pdaf_info = NULL;
	struct pm_workmode_input  pm_input;
	struct pm_workmode_output pm_output;
	struct pdaf_ppi_info ppi_info;

	if (!isp_alg_handle || !in_ptr) {
		ISP_LOGE("fail to get valid ptr.");
		ret = ISP_PARAM_ERROR;
		return ret;
	}

	ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_RESET, NULL, NULL);

	cxt->first_frm = 1;
	cxt->aem_is_update = 0;
	cxt->work_mode = in_ptr->work_mode;
	cxt->zsl_flag = in_ptr->zsl_flag;
	cxt->sensor_fps.mode = in_ptr->sensor_fps.mode;
	cxt->sensor_fps.max_fps = in_ptr->sensor_fps.max_fps;
	cxt->sensor_fps.min_fps = in_ptr->sensor_fps.min_fps;
	cxt->sensor_fps.is_high_fps = in_ptr->sensor_fps.is_high_fps;
	cxt->sensor_fps.high_fps_skip_num = in_ptr->sensor_fps.high_fps_skip_num;
	cxt->cam_4in1_mode = in_ptr->mode_4in1;
	cxt->noramosaic_4in1= in_ptr->noramosaic_4in1;
	cxt->commn_cxt.src.w = in_ptr->size.w;
	cxt->commn_cxt.src.h = in_ptr->size.h;
	cxt->commn_cxt.prv_size = cxt->commn_cxt.src;
	if (cxt->cam_4in1_mode || cxt->noramosaic_4in1) {
		cxt->commn_cxt.prv_size.w >>= 1;
		cxt->commn_cxt.prv_size.h >>= 1;
	}

	ISP_LOGD("work_mode %d, is_dv %d, zsl %d,  size %d %d, prv_size %d %d,  4in1_prev %d %d\n",
		in_ptr->work_mode, in_ptr->dv_mode, in_ptr->zsl_flag,
		in_ptr->size.w, in_ptr->size.h, cxt->commn_cxt.prv_size.w, cxt->commn_cxt.prv_size.h,
		(cmr_u32)in_ptr->mode_4in1, cxt->noramosaic_4in1);

	cxt->mem_info.alloc_cb = in_ptr->alloc_cb;
	cxt->mem_info.free_cb = in_ptr->free_cb;
	cxt->mem_info.oem_handle = in_ptr->oem_handle;
	sn_mode = in_ptr->resolution_info.size_index;
	cxt->sn_mode = sn_mode;
	ISP_LOGV("is_master = %d, sn_mode = %d", cxt->is_master, sn_mode);
	if (!cxt->is_master) {
		isp_br_ioctrl(CAM_SENSOR_SLAVE0, SET_SLAVE_SENSOR_MODE, &sn_mode, NULL);
	}

	/* malloc statis/lsc and other buffers and mapping buffers to dev. */
	ret = isp_dev_prepare_buf(cxt->dev_access_handle, &cxt->mem_info);
	ISP_RETURN_IF_FAIL(ret, ("fail to prepare buf"));

	/* initialize isp pm */
	memset(&pm_input, 0, sizeof(struct pm_workmode_input));
	memset(&pm_output, 0, sizeof(struct pm_workmode_output));
	pm_input.pm_sets_num = 1;
	switch (in_ptr->work_mode) {
	case 0:		/*preview / video */
	{
		if (in_ptr->dv_mode)
			pm_input.mode[0] = WORKMODE_VIDEO;
		else
			pm_input.mode[0] = WORKMODE_PREVIEW;
		pm_input.img_w[0] = cxt->commn_cxt.src.w;
		pm_input.img_h[0] = cxt->commn_cxt.src.h;
		if (cxt->cam_4in1_mode) {
			pm_input.cam_4in1_mode = 1;
			pm_input.define[0] = 1; /* todo: workaround for 4in1 binning prev*/
			pm_input.img_w[0] >>= 1;
			pm_input.img_h[0] >>= 1;
		}
		if (cxt->noramosaic_4in1){
			pm_input.noramosaic_4in1 = 1;
			pm_input.define[0] = 1;
			pm_input.img_w[0] >>= 1;
			pm_input.img_h[0] >>= 1;
		}
		if (cxt->zsl_flag)  {
			pm_input.pm_sets_num++;
			pm_input.mode[1] = WORKMODE_CAPTURE;
			pm_input.img_w[1] = cxt->commn_cxt.src.w;
			pm_input.img_h[1] = cxt->commn_cxt.src.h;
		}
		break;
	}
	case 1:		/* capture only */
	{
		pm_input.mode[0] = WORKMODE_CAPTURE;
		pm_input.img_w[0] = cxt->commn_cxt.src.w;
		pm_input.img_h[0] = cxt->commn_cxt.src.h;
		break;
	}
	default:
		pm_input.mode[0] = WORKMODE_PREVIEW;
		pm_input.img_w[0] = cxt->commn_cxt.src.w;
		pm_input.img_h[0] = cxt->commn_cxt.src.h;
		break;
	}

	ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_MODE, &pm_input, &pm_output);
	ISP_RETURN_IF_FAIL(ret, ("fail to isp pm set mode"));

	cxt->commn_cxt.isp_pm_mode[0] = pm_output.mode_id[0];
	if (cxt->zsl_flag)
		cxt->commn_cxt.isp_pm_mode[1] = pm_output.mode_id[1];
	else
		cxt->commn_cxt.isp_pm_mode[1] = ISP_TUNE_MODE_INVALID;
	ISP_LOGD("pm mode 0x%x 0x%x\n", cxt->commn_cxt.isp_pm_mode[0], cxt->commn_cxt.isp_pm_mode[1]);

	cxt->commn_cxt.param_index = ispalg_get_param_index(cxt->commn_cxt.input_size_trim, &in_ptr->size);

	ret = ispalg_update_alg_param(cxt);
	ISP_RETURN_IF_FAIL(ret, ("fail to isp smart param calc"));

	ISP_LOGI("pdaf_support = %d, pdaf_enable = %d, is_multi_mode = %d",
			cxt->pdaf_cxt.pdaf_support, in_ptr->pdaf_enable, cxt->is_multi_mode);

	if (SENSOR_PDAF_TYPE3_ENABLE == cxt->pdaf_cxt.pdaf_support
		&& in_ptr->pdaf_enable) {
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
	ISP_LOGV("sn_mode:0x%x, type1.support:%d", sn_mode, atoi(value));

	/* start config PDAF information */
	if (in_ptr->pdaf_enable && pdaf_info) {

		switch (cxt->pdaf_cxt.pdaf_support){
		case SENSOR_PDAF_TYPE1_ENABLE:
			if (pdaf_info->sns_mode &&
					(pdaf_info->sns_mode[sn_mode] || (1 == atoi(value)))){
				ISP_LOGI("pdaf_info->sns_mode = %d",pdaf_info->sns_mode[sn_mode]);
				isp_pdaf_type = ISP_DEV_SET_PDAF_TYPE1_CFG;
			}
			break;
		case SENSOR_PDAF_TYPE2_ENABLE:
			isp_pdaf_type = ISP_DEV_SET_PDAF_TYPE2_CFG;
			break;
		case SENSOR_PDAF_TYPE3_ENABLE:
			isp_pdaf_type = ISP_DEV_SET_PDAF_TYPE3_CFG;
			/* config pdaf block: ROI, coordinate, window */
			ppi_info.block.start_x = pdaf_info->pd_offset_x;
			ppi_info.block.end_x = pdaf_info->pd_end_x;
			ppi_info.block.start_y = pdaf_info->pd_offset_y;
			ppi_info.block.end_y = pdaf_info->pd_end_y;
			ppi_info.block_size.height = pdaf_info->pd_block_h;
			ppi_info.block_size.width = pdaf_info->pd_block_w;
			ppi_info.pd_pos_size = pdaf_info->pd_pos_size;

			for (i = 0; i < pdaf_info->pd_pos_size * 2; i++) {
				ppi_info.pattern_pixel_is_right[i] = pdaf_info->pd_is_right[i];
				ppi_info.pattern_pixel_row[i] = pdaf_info->pd_pos_row[i];
				ppi_info.pattern_pixel_col[i] = pdaf_info->pd_pos_col[i];
			}
			isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_PDAF_PPI_INFO, &ppi_info, NULL);
			break;
		case SENSOR_DUAL_PDAF_ENABLE:
			isp_pdaf_type = ISP_DEV_SET_DUAL_PDAF_CFG;
			break;
		default:
			ISP_LOGI("PDAF param is invalid");
			break;
		}

		vch2_info.bypass = pdaf_info->vch2_info.bypass;
		vch2_info.vch2_vc = pdaf_info->vch2_info.vch2_vc;
		vch2_info.vch2_data_type = pdaf_info->vch2_info.vch2_data_type;
		vch2_info.vch2_mode = pdaf_info->vch2_info.vch2_mode;
		ISP_LOGI("vch2_info.bypass = 0x%x, vc = 0x%x, data_type = 0x%x, mode = 0x%x",
				vch2_info.bypass, vch2_info.vch2_vc,
				vch2_info.vch2_data_type, vch2_info.vch2_mode);
		if (isp_pdaf_type != 0)
			ret = isp_dev_access_ioctl(cxt->dev_access_handle, isp_pdaf_type, &vch2_info, 0);
	} /* end config pdaf */

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
		((struct isp_anti_flicker_cfg *)cxt->afl_cxt.handle)->width = cxt->commn_cxt.prv_size.w;
		((struct isp_anti_flicker_cfg *)cxt->afl_cxt.handle)->height = cxt->commn_cxt.prv_size.h;
	}

	cxt->afl_cxt.hw_bypass = 1;
	if(cxt->afl_cxt.version) {
		if (cxt->ops.afl_ops.config_new)
			ret = cxt->ops.afl_ops.config_new(cxt->afl_cxt.handle);
	}
	ISP_TRACE_IF_FAIL(ret, ("fail to do anti_flicker param update"));

	ret = ispalg_ae_set_work_mode(cxt, cxt->commn_cxt.isp_pm_mode[0], 1, in_ptr);
	ISP_RETURN_IF_FAIL(ret, ("fail to do ae cfg"));

	if (cxt->ops.awb_ops.ioctrl) {
		ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle,
				AWB_CTRL_CMD_SET_WORK_MODE,
				&in_ptr->work_mode, NULL);
		ISP_RETURN_IF_FAIL(ret, ("fail to set_awb_work_mode"));
		ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle,
				AWB_CTRL_CMD_SET_AE_STAT_WIN_NUM,
				&cxt->ae_cxt.win_num, NULL);
		ISP_RETURN_IF_FAIL(ret, ("fail to set_awb_aem_stat_win"));

		ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle,
				AWB_CTRL_CMD_GET_PIX_CNT, &cxt->commn_cxt.prv_size, NULL);
		ISP_RETURN_IF_FAIL(ret, ("fail to get_awb_pix_cnt"));
	}

	memset(&af_start_info, 0, sizeof(struct afctrl_fwstart_info));
	af_start_info.sensor_fps.is_high_fps = in_ptr->sensor_fps.is_high_fps;
	af_start_info.sensor_fps.high_fps_skip_num = in_ptr->sensor_fps.high_fps_skip_num;

	if (cxt->af_cxt.handle && cxt->ops.af_ops.ioctrl) {
		ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle,
					AF_CMD_SET_AF_BYPASS,
					(void *)&cxt->af_cxt.sw_bypass, NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to set af_bypass"));

		if (ISP_VIDEO_MODE_CONTINUE == in_ptr->mode) {
			af_start_info.size = cxt->commn_cxt.prv_size;
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

	cxt->fw_started = 1;
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
	cxt->fw_started = 0;
	ISP_LOGV("done %ld", ret);
	return ret;
}

static cmr_int ispalg_dump_alsc_info(struct alsc_do_simulation *alsc_param, cmr_u32 gain_w, cmr_u32 gain_h)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_u32 i = 0;
	FILE *fp = NULL;
	char file_name[260] = { 0 };
	char loop_cnt_str[10] = { 0 };

	if (alsc_param == NULL) {
		ISP_LOGE("fail to awb param pointer.");
		return ISP_ERROR;
	}
	ret = isp_sim_get_mipi_raw_file_name(file_name);
	if (strlen(file_name)) {
		sprintf(loop_cnt_str, ".%d", isp_video_get_simulation_loop_count());
		strcat(file_name, loop_cnt_str);
		strcat(file_name, ".alsc.log");
	} else {
		ISP_LOGE("fail to get mipi raw file name.");
		return ISP_ERROR;
	}
	CMR_LOGD("file name %s", file_name);
	fp = fopen(file_name, "wb");
	if (fp == NULL) {
		ISP_LOGE("fail to open file");
		return ISP_ERROR;
	}

	fprintf(fp, "1.input alsc param:\n");
	fprintf(fp, "bv:%d\n", alsc_param->bv);
	fprintf(fp, "ct:%d\n", alsc_param->ct);
	fprintf(fp, "bv_gain:%d\n", alsc_param->bv_gain);

	fprintf(fp, "2.output alsc table:\n");
	for (i=0; i < gain_w*gain_h; i++) {
		fprintf(fp, "[%d]:%d \n",
			i, alsc_param->sim_output_table[i]);
	}
	fflush(fp);
	fclose(fp);
	return ret;
}



static cmr_int ispalg_alsc_update(cmr_handle isp_alg_handle)
{
	cmr_s32 ret = ISP_SUCCESS;
	enum isp_pm_cmd  cmd;
	cmr_u32 adaptive_size_info[3] = {0, 0, 0};
	struct isp_size pic_size;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	lsc_adv_handle_t lsc_adv_handle = cxt->lsc_cxt.handle;
	cmr_handle pm_handle = cxt->handle_pm;
	struct lsc_adv_calc_param calc_param;
	struct alsc_ver_info lsc_ver = { 0 };
	struct isp_pm_ioctl_input io_pm_input = { NULL, 0 };
	struct isp_pm_ioctl_output io_pm_output = { NULL, 0 };
	struct isp_pm_param_data pm_param;
	struct isp_pm_ioctl_input pm_tab_input = { NULL, 0 };
	struct isp_pm_ioctl_output pm_tab_output = { NULL, 0 };
	struct isp_pm_ioctl_input io_pm_input_alsc = { NULL, 0 };
	struct isp_pm_param_data param_data_alsc;
	struct alsc_do_simulation do_sim;
	cmr_u16 *sim_output_table = NULL;
	char value[PROPERTY_VALUE_MAX];

	memset(&calc_param, 0, sizeof(struct lsc_adv_calc_param));
	memset(&io_pm_input, 0, sizeof(struct isp_pm_ioctl_input));
	memset(&io_pm_output, 0, sizeof(struct isp_pm_ioctl_output));
	memset(&pm_param, 0, sizeof(struct isp_pm_param_data));
	memset(&pm_tab_input, 0, sizeof(struct isp_pm_ioctl_input));
	memset(&pm_tab_output, 0, sizeof(struct isp_pm_ioctl_output));
	memset(&io_pm_input_alsc, 0, sizeof(struct isp_pm_ioctl_input));
	memset(&param_data_alsc, 0, sizeof(struct isp_pm_param_data));
	memset(&do_sim, 0, sizeof(struct alsc_do_simulation));

	ISP_LOGI("enter\n");

	if (cxt->ops.lsc_ops.ioctrl) {
		ret = cxt->ops.lsc_ops.ioctrl(lsc_adv_handle, ALSC_GET_VER, NULL, (void *)&lsc_ver);
		if (ISP_SUCCESS != ret) {
			ISP_LOGE("fail to Get ALSC ver info!");
		}
	}

	if (cxt->lsc_cxt.LSC_SPD_VERSION >= 6) {

		pic_size = cxt->commn_cxt.src;
		cmd = ISP_PM_CMD_SET_GRID0;

		ISP_LOGI("[ALSC] lsc_pm_normalization, new_image_size[%d,%d], full_image_size[%d,%d]",
			pic_size.w, pic_size.h, cxt->lsc_cxt.full_size_width, cxt->lsc_cxt.full_size_height);

		if((cxt->lsc_cxt.full_size_width * pic_size.h) == (cxt->lsc_cxt.full_size_height * pic_size.w)){
			ISP_LOGI("[ALSC] lsc_pm_normalization case1, n binning");
			adaptive_size_info[0] = pic_size.w;
			adaptive_size_info[1] = pic_size.h;
			adaptive_size_info[2] = cxt->lsc_cxt.full_size_grid * pic_size.w / cxt->lsc_cxt.full_size_width;
			memset(&pm_param, 0, sizeof(pm_param));
			BLOCK_PARAM_CFG(io_pm_input, pm_param,
				ISP_PM_BLK_LSC_UPDATE_GRID,
				ISP_BLK_2D_LSC,
				&adaptive_size_info[0], 0);
			ret = isp_pm_ioctl(cxt->handle_pm, cmd, &io_pm_input, NULL);
		} else if ((pic_size.w == 1280) && (pic_size.h == 720)) {
			ISP_LOGI("[ALSC] lsc_pm_normalization case2. image size %d %d\n", pic_size.w, pic_size.h);
			adaptive_size_info[0] = cxt->commn_cxt.src.w;
			adaptive_size_info[1] = cxt->commn_cxt.src.h;
			adaptive_size_info[2] = 32;
			memset(&pm_param, 0, sizeof(pm_param));
			BLOCK_PARAM_CFG(io_pm_input, pm_param,
				ISP_PM_BLK_LSC_UPDATE_GRID,
				ISP_BLK_2D_LSC,
				&adaptive_size_info[0], 0);
			ret = isp_pm_ioctl(cxt->handle_pm, cmd, &io_pm_input, NULL);
		} else {
			ISP_LOGE("[ALSC] lsc_pm_normalization error");
		}
	}

	if (cxt->takepicture_mode != CAMERA_ISP_SIMULATION_MODE)
		return ISP_SUCCESS;

	if (lsc_ver.LSC_SPD_VERSION >= 2) {
		memset(&pm_param, 0, sizeof(pm_param));
		BLOCK_PARAM_CFG(io_pm_input, pm_param, ISP_PM_BLK_LSC_INFO, ISP_BLK_2D_LSC, PNULL, 0);
		ret = isp_pm_ioctl(pm_handle, ISP_PM_CMD_GET_SINGLE_SETTING, (void *)&io_pm_input, (void *)&io_pm_output);
		struct isp_lsc_info *lsc_info = (struct isp_lsc_info *)io_pm_output.param_data->data_ptr;
		if (NULL == lsc_info || ISP_SUCCESS != ret) {
			ISP_LOGE("fail to get lsc info");
			return ISP_ERROR;
		}

		cmr_u32 stat_w = 0;
		cmr_u32 stat_h = 0;
		struct isp_awb_statistic_info awb_stat;
		struct isptool_scene_param scene_param;
		memset(&awb_stat, 0, sizeof(awb_stat));

		ret = isp_sim_get_no_lsc_ae_stats((void *)&awb_stat, &stat_w, &stat_h);
		if (ret) {
			ISP_LOGE("fail to get ae stats");
		}

		ret = isp_sim_get_scene_parm(&scene_param);
		if(ret) {
			ISP_LOGE("fail to get scene parameter");
		}

		sim_output_table = (cmr_u16*)malloc(lsc_info->gain_w * lsc_info->gain_h * 4 * sizeof(cmr_u16));
		do_sim.stat_r = awb_stat.r_info;
		do_sim.stat_g = awb_stat.g_info;
		do_sim.stat_b = awb_stat.b_info;
		do_sim.ct = scene_param.smart_ct;
		do_sim.bv = scene_param.smart_bv;
		do_sim.bv_gain = scene_param.global_gain;
		do_sim.sim_output_table = sim_output_table;
		if (cxt->ops.lsc_ops.ioctrl) {
			ret = cxt->ops.lsc_ops.ioctrl(lsc_adv_handle, ALSC_DO_SIMULATION, (void *)&do_sim, NULL);
			if (ISP_SUCCESS != ret)
				ISP_LOGE("fail to do ALSC_DO_SIMULATION!");
		}
		BLOCK_PARAM_CFG(io_pm_input_alsc, param_data_alsc,
				ISP_PM_BLK_LSC_MEM_ADDR, ISP_BLK_2D_LSC,
				do_sim.sim_output_table,
				lsc_info->gain_w * lsc_info->gain_h * 4 * sizeof(cmr_u16));
		ret = isp_pm_ioctl(pm_handle, ISP_PM_CMD_SET_OTHERS, &io_pm_input_alsc, NULL);
		property_get("persist.vendor.cam.debug.simulation", value, "false");
		if (!strcmp(value, "true")) {
			ispalg_dump_alsc_info(&do_sim, lsc_info->gain_w, lsc_info->gain_h);
		}
		free(sim_output_table);
		sim_output_table = NULL;
	}
	return ret;
}

cmr_int isp_alg_fw_proc_start(cmr_handle isp_alg_handle, struct ips_in_param *in_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_video_start param;
	struct isp_raw_proc_info raw_proc_in;
	struct pm_workmode_input  pm_input;
	struct pm_workmode_output pm_output;

	if (!isp_alg_handle || !in_ptr) {
		ISP_LOGE("fail to get valid ptr.");
		ret = ISP_PARAM_ERROR;
		return ret;
	}

	ISP_LOGD("isp proc start\n");
	ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_RESET, NULL, NULL);
	ISP_TRACE_IF_FAIL(ret, ("fail to do isp_dev_reset"));

	cxt->zsl_flag = 0;
	cxt->work_mode = PM_SCENE_CAP; /* 0 - preview, 1 - capture  */
	cxt->commn_cxt.src.w = in_ptr->src_frame.img_size.w;
	cxt->commn_cxt.src.h = in_ptr->src_frame.img_size.h;

	cxt->mem_info.alloc_cb = in_ptr->alloc_cb;
	cxt->mem_info.free_cb = in_ptr->free_cb;
	cxt->mem_info.oem_handle = in_ptr->oem_handle;

	ISP_LOGI("takepicture_mode[%d] camera_id[%d] cxt[%p]\n", 
			cxt->takepicture_mode,
			(unsigned int)cxt->camera_id,
			cxt);

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

	if (cxt->takepicture_mode == CAMERA_ISP_SIMULATION_MODE) {
			raw_proc_in.scene = RAW_PROC_SCENE_HWSIM;
	} else {
			raw_proc_in.scene = RAW_PROC_SCENE_RAWCAP;
	}

	ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_RAW_PROC, &raw_proc_in, NULL);
	ISP_RETURN_IF_FAIL(ret, ("fail to do isp_dev_raw_proc"));

	/* malloc statis/lsc and other buffers and mapping buffers to dev. */
	ret = isp_dev_prepare_buf(cxt->dev_access_handle, &cxt->mem_info);
	ISP_RETURN_IF_FAIL(ret, ("fail to prepare buf"));


	memset(&pm_input, 0, sizeof(struct pm_workmode_input));
	memset(&pm_output, 0, sizeof(struct pm_workmode_output));
	pm_input.pm_sets_num = 1;
	pm_input.mode[0] = WORKMODE_CAPTURE;
	pm_input.img_w[0] = cxt->commn_cxt.src.w;
	pm_input.img_h[0] = cxt->commn_cxt.src.h;

#if 0
	ISP_LOGV("flag %d %d\n", cxt->is_4in1_sensor, cxt->cam_4in1_mode);
	if (cxt->cam_4in1_mode) {
		cmr_u32 lowlight_tmp = 0;

		pm_input.cam_4in1_mode = cxt->cam_4in1_mode;
		/* 4in1 postprocess full size, ignore lowlight_flag */
		isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_LOWLIGHT_FLAG, &lowlight_tmp,
			&cxt->commn_cxt.isp_pm_mode[1]);
	}
#endif

	ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_MODE, &pm_input, &pm_output);
	ISP_RETURN_IF_FAIL(ret, ("fail to do isp_pm_ioctl"));

	ISP_LOGV("mode %d %d\n", pm_output.mode_id[0],pm_output.mode_id[1]);
	cxt->commn_cxt.isp_pm_mode[0] = pm_output.mode_id[0];
	cxt->commn_cxt.isp_pm_mode[1] = ISP_TUNE_MODE_INVALID;

	cxt->commn_cxt.param_index =
		ispalg_get_param_index(cxt->commn_cxt.input_size_trim, &in_ptr->src_frame.img_size);

	if (cxt->takepicture_mode != CAMERA_ISP_SIMULATION_MODE) {
		ret = ispalg_update_alg_param(cxt);
		ISP_RETURN_IF_FAIL(ret, ("fail to update alg parm"));
	} else {
		ret = ispalg_update_smart_param(cxt);
		ISP_RETURN_IF_FAIL(ret, ("fail to update smart parm"));
	}

	ispalg_alsc_update(isp_alg_handle);

	ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_CFG_START, NULL, NULL);
	ISP_TRACE_IF_FAIL(ret, ("fail to do cfg start"));

	ret = ispalg_cfg_param(cxt, 1);
	ISP_RETURN_IF_FAIL(ret, ("fail to do isp cfg"));

	if (cxt->takepicture_mode == CAMERA_ISP_SIMULATION_MODE) {
		param.is_need_flash = 0;
		param.capture_skip_num = 0;
		param.is_snapshot = 1;
		param.dv_mode = 0;
		param.sensor_fps.mode = in_ptr->sensor_fps.mode;
		param.sensor_fps.max_fps = in_ptr->sensor_fps.max_fps;
		param.sensor_fps.min_fps = in_ptr->sensor_fps.min_fps;
		param.sensor_fps.is_high_fps = in_ptr->sensor_fps.is_high_fps;
		param.sensor_fps.high_fps_skip_num = in_ptr->sensor_fps.high_fps_skip_num;
		ret = ispalg_ae_set_work_mode(cxt, cxt->commn_cxt.isp_pm_mode[0], 0, &param);
		ISP_RETURN_IF_FAIL(ret, ("fail to set ae work mode"));
	}

	if (cxt->ops.ae_ops.ioctrl) {
		ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_RGB_GAIN, NULL, NULL);
		ISP_RETURN_IF_FAIL(ret, ("fail to set rgb gain"));
	}

	/* PDAF TYPE 3 PD info stored in sensor info, not in pm */
	if (SENSOR_PDAF_TYPE3_ENABLE == cxt->pdaf_cxt.pdaf_support) {
		if (cxt->ops.pdaf_ops.ioctrl) {
			ISP_LOGI("hwsim: pdaf3 set param\n");
			ret = cxt->ops.pdaf_ops.ioctrl(
					cxt->pdaf_cxt.handle,
					PDAF_CTRL_CMD_SET_PARAM,
					NULL, NULL);
			ISP_RETURN_IF_FAIL(ret, ("fail to cfg pdaf"));
		}
	}


	/* RAW_PROC_POST, kernel will set all buffers and trigger raw image processing*/
	ISP_LOGV("start raw proc\n");
	raw_proc_in.cmd = RAW_PROC_POST;
	ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_RAW_PROC, &raw_proc_in, NULL);
	ISP_TRACE_IF_FAIL(ret, ("fail to do isp_dev_raw_proc"));

	/* restore to default mode, or else normal take picture will still be treated as simulation mode. */
	if (isp_video_get_simulation_flag()) {
		/* hwsim will do twice pipeline */
		ISP_LOGI("takepicture_mode[%d]\n", cxt->takepicture_mode);
	} else {
		cxt->takepicture_mode = CAMERA_NORMAL_MODE;
	}
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
	cxt->af_cxt.tof_support = input_ptr->init_param->ex_info.tof_support;
	cxt->pdaf_cxt.pdaf_support = input_ptr->init_param->ex_info.pdaf_supported;
	cxt->ebd_cxt.ebd_support = input_ptr->init_param->ex_info.ebd_supported;
	cxt->awb_cxt.color_support = input_ptr->init_param->ex_info.color_support;
	cxt->is_4in1_sensor = input_ptr->init_param->is_4in1_sensor;
	ISP_LOGI("camera_id = %ld, master %d, is_4in1_sensor %d\n", cxt->camera_id,
		cxt->is_master, cxt->is_4in1_sensor);


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

	if (cxt->is_multi_mode) {
		cmr_u32 id = cxt->camera_id;
		cmr_u32 role = CAM_SENSOR_MAX;

		ret = isp_br_ioctrl(CAM_SENSOR_MASTER, GET_SENSOR_ROLE, &id, &role);
		if (ret) {
			ISP_LOGE("fail to get sensor role");
			goto err_alg;
		}

		cxt->sensor_role = role;
		ISP_LOGI("sensor_role %u", role);
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

	if (cxt->ae_cxt.log_alc) {
		free(cxt->ae_cxt.log_alc);
		cxt->ae_cxt.log_alc = NULL;
	}

	if (cxt->commn_cxt.log_isp) {
		free(cxt->commn_cxt.log_isp);
		cxt->commn_cxt.log_isp = NULL;
	}

	free((void *)cxt->pdaf_info);
	free((void *)cxt);

	ISP_LOGI("done %d", ret);
	return ret;
}
