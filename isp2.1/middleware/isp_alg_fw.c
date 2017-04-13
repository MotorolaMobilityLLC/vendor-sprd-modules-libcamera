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
#define LOG_TAG "isp_alg_fw"

#include <math.h>
#include "isp_alg_fw.h"
#include "lib_ctrl.h"
#include "isp_adpt.h"
#include "cmr_msg.h"
#include "ae_ctrl.h"
#include "awb_ctrl.h"
#include "smart_ctrl.h"
#include "af_ctrl.h"
#include "afl_ctrl.h"
#include "lsc_adv.h"
#include "isp_dev_access.h"
#include "isp_ioctrl.h"
#include "isp_param_file_update.h"
#include "pdaf_ctrl.h"

#include <dlfcn.h>

cmr_u32 isp_cur_bv;
cmr_u32 isp_cur_ct;

#define LSC_ADV_ENABLE
//#define ANTI_FLICKER_INFO_VERSION_NEW

cmr_s32 gAWBGainR = 1;
cmr_s32 gAWBGainB = 1;
cmr_s32 gCntSendMsgLsc = 0;
static cmr_u32 aeStatistic[32 * 32 * 4];

struct isp_awb_calc_info {
	struct ae_calc_out ae_result;
	struct isp_awb_statistic_info *ae_stat_ptr;
	struct isp_binning_statistic_info *awb_stat_ptr;
	cmr_u64 k_addr;
	cmr_u64 u_addr;
	cmr_u32 type;
};

struct isp_alsc_calc_info {
	cmr_u32 *stat_ptr;
	cmr_s32 image_width;
	cmr_s32 image_height;
	struct awb_size stat_img_size;
	struct awb_size win_size;
	cmr_u32 awb_ct;
	cmr_s32 awb_r_gain;
	cmr_s32 awb_b_gain;
	cmr_u32 stable;
};

struct isp_alg_sw_init_in {
	cmr_handle dev_access_handle;
	struct sensor_libuse_info *lib_use_info;
	struct isp_size size;
	struct sensor_otp_cust_info *otp_data;
	struct sensor_pdaf_info *pdaf_info;
};

typedef struct {
	int grid_size;
	int lpf_mode;
	int lpf_radius;
	int lpf_border;
	int border_patch;
	int border_expand;
	int shading_mode;
	int shading_pct;
} lsc2d_calib_param_t;

struct lsc_wrapper_ops {
	void (*lsc2d_grid_samples) (int w, int h, int gridx, int gridy, int *nx, int *ny);
	void (*lsc2d_calib_param_default) (lsc2d_calib_param_t * calib_param, int grid_size, int lpf_radius, int shading_pct);
	int (*lsc2d_table_preproc) (uint16_t * otp_chn[4], uint16_t * tbl_chn[4], int w, int h, int sx, int sy, lsc2d_calib_param_t * calib_param);
	int (*lsc2d_table_postproc) (uint16_t * tbl_chn[4], int w, int h, int sx, int sy, lsc2d_calib_param_t * calib_param);
};

static nsecs_t isp_get_timestamp(void)
{
	nsecs_t timestamp = 0;

	timestamp = systemTime(CLOCK_MONOTONIC);
	timestamp = timestamp / 1000000;

	return timestamp;
}

static cmr_int isp_get_rgb_gain(cmr_handle isp_fw_handle, cmr_u32 * param)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_pm_param_data param_data;
	struct isp_pm_ioctl_input input = { NULL, 0 };
	struct isp_pm_ioctl_output output = { NULL, 0 };
	struct isp_dev_rgb_gain_info *gain_info;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_fw_handle;

	memset(&param_data, 0, sizeof(param_data));

	BLOCK_PARAM_CFG(input, param_data, ISP_PM_BLK_ISP_SETTING, ISP_BLK_RGB_GAIN, NULL, 0);
	rtn = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_SINGLE_SETTING, &input, &output);
	if (ISP_SUCCESS == rtn && 1 == output.param_num) {
		gain_info = (struct isp_dev_rgb_gain_info *)output.param_data->data_ptr;
		*param = gain_info->global_gain;
	} else {
		*param = 4096;
	}

	ISP_LOGV("D-gain global gain ori: %d\n", *param);

	return rtn;

}

static cmr_int isp_alg_ae_callback(cmr_handle isp_alg_handle, cmr_int cb_type)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	enum isp_callback_cmd cmd = 0;

	if (NULL != cxt) {
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
		default:
			cmd = ISP_AE_STAB_CALLBACK;
			break;
		}

		if (cxt->commn_cxt.callback) {
			cxt->commn_cxt.callback(cxt->commn_cxt.caller_id, ISP_CALLBACK_EVT | cmd, NULL, 0);
		}
	}

	return rtn;
}

static cmr_int isp_ae_set_cb(cmr_handle isp_alg_handle, cmr_int type, void *param0, void *param1)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	switch (type) {
	case ISP_AE_SET_GAIN:
		rtn = cxt->ioctrl_ptr->set_gain(cxt->ioctrl_ptr->caller_handler, *(cmr_u32 *) param0);
		break;
	case ISP_AE_SET_EXPOSURE:
		rtn = cxt->ioctrl_ptr->set_exposure(cxt->ioctrl_ptr->caller_handler, *(cmr_u32 *) param0);
		break;
	case ISP_AE_EX_SET_EXPOSURE:
		rtn = cxt->ioctrl_ptr->ex_set_exposure(cxt->ioctrl_ptr->caller_handler, (cmr_u32) param0);
		break;
	case ISP_AE_SET_MONITOR:
		rtn = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AE_MONITOR, param0, param1);
		break;
	case ISP_AE_SET_MONITOR_WIN:
		rtn = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AE_MONITOR_WIN, param0, param1);
		break;
	case ISP_AE_SET_MONITOR_BYPASS:
		rtn = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AE_MONITOR_BYPASS, param0, param1);
		break;
	case ISP_AE_SET_STATISTICS_MODE:
		rtn = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AE_STATISTICS_MODE, param0, param1);
		break;
	case ISP_AE_SET_AE_CALLBACK:
		rtn = isp_alg_ae_callback(cxt, *(cmr_int *) param0);
		break;
	case ISP_AE_GET_SYSTEM_TIME:
		rtn = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_GET_AE_SYSTEM_TIME, param0, param1);
		break;
	case ISP_AE_SET_RGB_GAIN:
		rtn = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_RGB_GAIN, param0, param1);
		break;
	case ISP_AE_GET_FLASH_CHARGE:
		rtn = cxt->commn_cxt.ops.flash_get_charge(cxt->commn_cxt.caller_id, param0, param1);
		break;
	case ISP_AE_GET_FLASH_TIME:
		rtn = cxt->commn_cxt.ops.flash_get_time(cxt->commn_cxt.caller_id, param0, param1);
		break;
	case ISP_AE_SET_FLASH_CHARGE:
		rtn = cxt->commn_cxt.ops.flash_set_charge(cxt->commn_cxt.caller_id, param0, param1);
		break;
	case ISP_AE_SET_FLASH_TIME:
		rtn = cxt->commn_cxt.ops.flash_set_time(cxt->commn_cxt.caller_id, param0, param1);
	case ISP_AE_FLASH_CTRL:
		rtn = cxt->commn_cxt.ops.flash_ctrl(cxt->commn_cxt.caller_id, param0, param1);
		break;
	case ISP_AE_GET_RGB_GAIN:
		rtn = isp_get_rgb_gain(cxt, param0);
		break;
	default:
		break;
	}

	return rtn;
}

static cmr_int isp_af_set_cb(cmr_handle isp_alg_handle, cmr_int type, void *param0, void *param1)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	switch (type) {
	case ISP_AF_SET_POS:
		if (cxt->ioctrl_ptr->set_focus) {
			rtn = cxt->ioctrl_ptr->set_focus(cxt->ioctrl_ptr->caller_handler, *(cmr_u32 *) param0);
		}
		break;
	case ISP_AF_END_NOTICE:
		if (ISP_ZERO == cxt->commn_cxt.isp_callback_bypass) {
			rtn = cxt->commn_cxt.callback(cxt->commn_cxt.caller_id, ISP_CALLBACK_EVT | ISP_AF_NOTICE_CALLBACK, param0, sizeof(struct isp_af_notice));
		}
		break;
	case ISP_AF_START_NOTICE:
		rtn = cxt->commn_cxt.callback(cxt->commn_cxt.caller_id, ISP_CALLBACK_EVT | ISP_AF_NOTICE_CALLBACK, param0, sizeof(struct isp_af_notice));
		break;
	case ISP_AF_AE_AWB_LOCK:
		rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_SET_PAUSE, NULL, param1);
		rtn = awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_LOCK, NULL, NULL);
		break;
	case ISP_AF_AE_AWB_RELEASE:
		rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_SET_RESTORE, NULL, param1);
		rtn = awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_UNLOCK, NULL, NULL);
		break;
	case ISP_AF_AE_LOCK:
		rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_SET_PAUSE, NULL, param1);
		break;
	case ISP_AF_AE_UNLOCK:
		rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_SET_RESTORE, NULL, param1);
		break;
	case ISP_AF_AWB_LOCK:
		rtn = awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_LOCK, NULL, NULL);
		break;
	case ISP_AF_AWB_UNLOCK:
		rtn = awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_UNLOCK, NULL, NULL);
		break;
	case ISP_AF_LSC_LOCK:
		rtn = lsc_ctrl_ioctrl(cxt->lsc_cxt.handle, SMART_LSC_ALG_LOCK, NULL, NULL);
		break;
	case ISP_AF_LSC_UNLOCK:
		rtn = lsc_ctrl_ioctrl(cxt->lsc_cxt.handle, SMART_LSC_ALG_UNLOCK, NULL, NULL);
		break;
	case ISP_AF_NLM_LOCK:
		cxt->smart_cxt.lock_en = 1;
		break;
	case ISP_AF_NLM_UNLOCK:
		cxt->smart_cxt.lock_en = 0;
		break;
	case ISP_AF_SET_MONITOR:
		rtn = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AF_MONITOR, param0, param1);
		break;
	case ISP_AF_SET_MONITOR_WIN:
		rtn = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AF_MONITOR_WIN, param0, param1);
		break;
	case ISP_AF_GET_MONITOR_WIN_NUM:
		rtn = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_GET_AF_MONITOR_WIN_NUM, param0, param1);
		break;
	default:
		break;
	}

	return rtn;
}

static cmr_int isp_pdaf_set_cb(cmr_handle isp_alg_handle, cmr_int type, void *param0, void *param1)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;

	ISP_LOGV("isp_pdaf_set_cb type = 0x%lx", type);
	switch (type) {
	case ISP_AF_SET_PD_INFO:
		rtn = af_ctrl_ioctrl(cxt->af_cxt.handle, AF_CMD_SET_PD_INFO, param0, param1);
		break;
	case ISP_PDAF_SET_CFG_PARAM:
		rtn = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_PDAF_CFG_PARAM, param0, param1);
		break;
	default:
		break;
	}

	return rtn;
}

static cmr_int isp_afl_set_cb(cmr_handle isp_alg_handle, cmr_int type, void *param0, void *param1)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	switch (type) {
	case ISP_AFL_SET_CFG_PARAM:
		rtn = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AFL_CFG_PARAM, param0, param1);
		break;
	case ISP_AFL_NEW_SET_CFG_PARAM:
		rtn = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AFL_NEW_CFG_PARAM, param0, param1);
		break;
	case ISP_AFL_SET_BYPASS:
		rtn = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AFL_BYPASS, param0, param1);
		break;
	case ISP_AFL_NEW_SET_BYPASS:
		rtn = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AFL_NEW_BYPASS, param0, param1);
		break;
	default:
		break;
	}

	return rtn;
}

cmr_s32 alsc_calc(cmr_handle isp_alg_handle,
		  cmr_u32 * ae_stat_r, cmr_u32 * ae_stat_g, cmr_u32 * ae_stat_b,
		  struct awb_size * stat_img_size,
		  struct awb_size * win_size, cmr_s32 image_width, cmr_s32 image_height, cmr_u32 awb_ct, cmr_s32 awb_r_gain, cmr_s32 awb_b_gain, cmr_u32 ae_stable)
{
	cmr_s32 rtn = ISP_SUCCESS;
#ifdef 	LSC_ADV_ENABLE
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	lsc_adv_handle_t lsc_adv_handle = cxt->lsc_cxt.handle;
	isp_pm_handle_t pm_handle = cxt->handle_pm;
	struct isp_pm_ioctl_input io_pm_input = { NULL, 0 };
	struct isp_pm_ioctl_output io_pm_output = { NULL, 0 };
	struct isp_pm_param_data pm_param;

	struct alsc_ver_info lsc_ver = { 0 };
	rtn = lsc_ctrl_ioctrl(lsc_adv_handle, ALSC_GET_VER, NULL, (void *)&lsc_ver);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to Get ALSC ver info!");
	}

	if (lsc_ver.LSC_SPD_VERSION >= 2) {
		cmr_u32 i = 0;
		cmr_s32 bv0 = 0;

		memset(&pm_param, 0, sizeof(pm_param));

		if (NULL == ae_stat_r) {
			ISP_LOGE("fail to  check stat info param");
			return ISP_ERROR;
		}

		rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_GET_BV_BY_LUM_NEW, NULL, (void *)&bv0);

		BLOCK_PARAM_CFG(io_pm_input, pm_param, ISP_PM_BLK_LSC_INFO, ISP_BLK_2D_LSC, PNULL, 0);
		rtn = isp_pm_ioctl(pm_handle, ISP_PM_CMD_GET_SINGLE_SETTING, (void *)&io_pm_input, (void *)&io_pm_output);
		struct isp_lsc_info *lsc_info = (struct isp_lsc_info *)io_pm_output.param_data->data_ptr;
		struct isp_2d_lsc_param *lsc_tab_param_ptr = (struct isp_2d_lsc_param *)(cxt->lsc_cxt.lsc_tab_address);

		struct lsc_adv_calc_param calc_param;
		struct lsc_adv_calc_result calc_result = { 0 };
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

		gAWBGainR = awb_r_gain;
		gAWBGainB = awb_b_gain;

		calc_param.bv = bv0;
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

		/*AF lock ALSC */

		if (cxt->lsc_cxt.isp_smart_lsc_lock == 0) {
			cmr_u64 time0 = systemTime(CLOCK_MONOTONIC);

			rtn = lsc_ctrl_process(lsc_adv_handle, &calc_param, &calc_result);
			if (ISP_SUCCESS != rtn) {
				ISP_LOGE("fail to do lsc adv gain map calc");
				return rtn;
			}
			cmr_u64 time1 = systemTime(CLOCK_MONOTONIC);

			BLOCK_PARAM_CFG(io_pm_input, pm_param, ISP_PM_BLK_LSC_INFO, ISP_BLK_2D_LSC, PNULL, 0);
			io_pm_input.param_data_ptr = &pm_param;
			rtn = isp_pm_ioctl(pm_handle, ISP_PM_CMD_SET_OTHERS, &io_pm_input, NULL);
		}
	}
#endif
	return rtn;
}

static cmr_int ispalg_handle_sensor_sof(cmr_handle isp_alg_handle)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_pm_ioctl_input input = { NULL, 0 };
	struct isp_pm_ioctl_output output = { NULL, 0 };
	struct isp_pm_param_data *param_data = NULL;
	cmr_u32 i;

	ISP_CHECK_HANDLE_VALID(isp_alg_handle);

	rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_SET_PROC, NULL, NULL);
	ISP_TRACE_IF_FAIL(rtn, ("fail to set ae proc"));

	isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_ISP_SETTING, &input, &output);
	param_data = output.param_data;
	for (i = 0; i < output.param_num; i++) {
		if (ISP_BLK_AE_NEW == param_data->id) {
			if (ISP_PM_BLK_ISP_SETTING == param_data->cmd) {
				rtn = isp_dev_cfg_block(cxt->dev_access_handle, param_data->data_ptr, param_data->id);
				ISP_TRACE_IF_FAIL(rtn, ("isp_dev_cfg_block fail"));
			}
		} else {
			rtn = isp_dev_cfg_block(cxt->dev_access_handle, param_data->data_ptr, param_data->id);
			ISP_TRACE_IF_FAIL(rtn, ("isp_dev_cfg_block fail"));
		}

		if (ISP_BLK_2D_LSC == param_data->id) {
			rtn = isp_dev_lsc_update(cxt->dev_access_handle);
			ISP_TRACE_IF_FAIL(rtn, ("isp_dev_lsc_update fail"));
		}

		if (ISP_BLK_RGB_GAMC == param_data->id) {
			cxt->gamma_sof_cnt = 0;
			cxt->update_gamma_eb = 0;
		}
		param_data++;
	}

	rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_SET_TUNING_EB, NULL, NULL);
	ISP_TRACE_IF_FAIL(rtn, ("fail to set ae tuning eb"));

	rtn = isp_dev_comm_shadow(cxt->dev_access_handle, ISP_ONE);
	ISP_TRACE_IF_FAIL(rtn, ("fail to set dev shadow "));

	return rtn;
}

static cmr_int ispalg_aem_stat_data_parser(cmr_handle isp_alg_handle, void *data)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_awb_statistic_info *ae_stat_ptr = NULL;
	struct isp_statis_buf_input statis_buf;
	struct isp_statis_info *statis_info = (struct isp_statis_info *)data;
	cmr_u64 k_addr = 0;
	cmr_u64 u_addr = 0;
	cmr_u32 val0 = 0;
	cmr_u32 val1 = 0;
	cmr_u32 i = 0;

	ISP_CHECK_HANDLE_VALID(isp_alg_handle);
	k_addr = statis_info->phy_addr;
	u_addr = statis_info->vir_addr;

	ae_stat_ptr = &cxt->aem_stats;
	for (i = 0x00; i < ISP_RAW_AEM_ITEM; i++) {
		val0 = *((cmr_u32 *) u_addr + i * 2);
		val1 = *(((cmr_u32 *) u_addr) + i * 2 + 1);
		ae_stat_ptr->r_info[i] = ((val1 >> 11) & 0x1fffff) << cxt->ae_cxt.shift;
		ae_stat_ptr->g_info[i] = (((val1 & 0x7ff) << 11) | ((val0 >> 21) & 0x3ff)) << cxt->ae_cxt.shift;
		ae_stat_ptr->b_info[i] = (val0 & 0x1fffff) << cxt->ae_cxt.shift;
	}
	memset((void *)&statis_buf, 0, sizeof(statis_buf));
	statis_buf.buf_size = statis_info->buf_size;
	statis_buf.phy_addr = statis_info->phy_addr;
	statis_buf.vir_addr = statis_info->vir_addr;
	statis_buf.buf_property = ISP_AEM_BLOCK;
	statis_buf.buf_flag = 1;
	rtn = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_STSTIS_BUF, &statis_buf, NULL);
	if (rtn) {
		ISP_LOGE("fail to set statis buf");
	}
	cxt->aem_is_update = 1;
	return rtn;
}

cmr_int ispalg_start_ae_process(cmr_handle isp_alg_handle, struct isp_awb_calc_info * awb_calc_info)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct ae_calc_in in_param;
	struct awb_gain gain;
	struct ae_calc_out ae_result;
	nsecs_t system_time0 = 0;
	nsecs_t system_time1 = 0;

	rtn = awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_GET_GAIN, (void *)&gain, NULL);

	memset((void *)&ae_result, 0, sizeof(ae_result));
	memset((void *)&in_param, 0, sizeof(in_param));
	if ((0 == gain.r) || (0 == gain.g) || (0 == gain.b)) {
		in_param.awb_gain_r = 1024;
		in_param.awb_gain_g = 1024;
		in_param.awb_gain_b = 1024;
	} else {
		in_param.awb_gain_r = gain.r;
		in_param.awb_gain_g = gain.g;
		in_param.awb_gain_b = gain.b;
	}

	in_param.stat_fmt = AE_AEM_FMT_RGB;
	if (AE_AEM_FMT_RGB & in_param.stat_fmt) {
		in_param.rgb_stat_img = (cmr_u32 *) & cxt->aem_stats.r_info[0];
		in_param.stat_img = (cmr_u32 *) & cxt->aem_stats.r_info[0];
	}

	in_param.sec = cxt->ae_cxt.time.sec;
	in_param.usec = cxt->ae_cxt.time.usec;

	in_param.sensor_fps.mode = cxt->sensor_fps.mode;
	in_param.sensor_fps.max_fps = cxt->sensor_fps.max_fps;
	in_param.sensor_fps.min_fps = cxt->sensor_fps.min_fps;
	in_param.sensor_fps.is_high_fps = cxt->sensor_fps.is_high_fps;
	in_param.sensor_fps.high_fps_skip_num = cxt->sensor_fps.high_fps_skip_num;
	system_time0 = isp_get_timestamp();
	rtn = ae_ctrl_process(cxt->ae_cxt.handle, &in_param, &ae_result);
	cxt->smart_cxt.isp_smart_eb = 1;
	system_time1 = isp_get_timestamp();
	ISP_LOGV("SYSTEM_TEST-ae:%lldms", system_time1 - system_time0);

	if (AL_AE_LIB == cxt->lib_use_info->ae_lib_info.product_id) {
		cxt->ae_cxt.log_alc_ae = ae_result.log_ae.log;
		cxt->ae_cxt.log_alc_ae_size = ae_result.log_ae.size;
	}

	awb_calc_info->ae_result = ae_result;
	awb_calc_info->ae_stat_ptr = &cxt->aem_stats;

	awb_calc_info->awb_stat_ptr = &cxt->binning_stats;

	ISP_LOGV("done %ld", rtn);
	return rtn;
}

cmr_int ispalg_awb_pre_process(cmr_handle isp_alg_handle, struct isp_awb_calc_info * in_ptr, struct awb_ctrl_calc_param * out_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_pm_param_data param_data;
	struct ae_monitor_info info;
	float gain = 0;
	float exposure = 0;
	cmr_s32 bv = 0;
	cmr_s32 iso = 0;
	struct ae_get_ev ae_ev;
	memset(&ae_ev, 0, sizeof(ae_ev));

	if (!in_ptr || !out_ptr || !isp_alg_handle) {
		rtn = ISP_PARAM_NULL;
		goto exit;
	}

	rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_GET_BV_BY_LUM_NEW, NULL, (void *)&bv);
	rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_GET_ISO, NULL, (void *)&iso);
	rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_GET_GAIN, NULL, (void *)&gain);
	rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_GET_EXP, NULL, (void *)&exposure);
	rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_GET_EV, NULL, (void *)&ae_ev);

	out_ptr->quick_mode = 0;
	out_ptr->stat_img.chn_img.r = in_ptr->ae_stat_ptr->r_info;
	out_ptr->stat_img.chn_img.g = in_ptr->ae_stat_ptr->g_info;
	out_ptr->stat_img.chn_img.b = in_ptr->ae_stat_ptr->b_info;

	out_ptr->stat_img_awb.chn_img.r = in_ptr->awb_stat_ptr->r_info;
	out_ptr->stat_img_awb.chn_img.g = in_ptr->awb_stat_ptr->g_info;
	out_ptr->stat_img_awb.chn_img.b = in_ptr->awb_stat_ptr->b_info;
	out_ptr->stat_width_awb = in_ptr->awb_stat_ptr->binning_size.w;
	out_ptr->stat_height_awb = in_ptr->awb_stat_ptr->binning_size.h;

	out_ptr->bv = bv;
//ALC_S
	out_ptr->ae_info.bv = bv;
	out_ptr->ae_info.iso = iso;
	out_ptr->ae_info.gain = gain;
	out_ptr->ae_info.exposure = exposure;
	out_ptr->ae_info.f_value = 2.2;
	out_ptr->ae_info.stable = in_ptr->ae_result.is_stab;
	out_ptr->ae_info.ev_index = ae_ev.ev_index;
	memcpy(out_ptr->ae_info.ev_table, ae_ev.ev_tab, 16 * sizeof(cmr_s32));
//ALC_E
	rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_GET_MONITOR_INFO, NULL, (void *)&info);
	out_ptr->scalar_factor = (info.win_size.h / 2) * (info.win_size.w / 2);

// simulation info
	struct isp_pm_ioctl_input io_pm_input = { NULL, 0 };
	struct isp_pm_ioctl_output io_pm_output = { NULL, 0 };
	struct isp_pm_param_data pm_param;

	memset(&pm_param, 0, sizeof(pm_param));

	// CMC
	BLOCK_PARAM_CFG(io_pm_input, pm_param, ISP_PM_BLK_CMC10, ISP_BLK_CMC10, 0, 0);
	isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_SINGLE_SETTING, &io_pm_input, &io_pm_output);

	if (io_pm_output.param_data != NULL) {
		cmr_u16 *cmc_info = io_pm_output.param_data->data_ptr;

		if (cmc_info != NULL) {
#define CMC10(n) (((n)>>13)?((n)-(1<<14)):(n))
			cmr_s32 i;
			for (i = 0; i < 9; i++) {
				out_ptr->matrix[i] = CMC10(cmc_info[i]);
			}
		}
	}
	// GAMMA
	BLOCK_PARAM_CFG(io_pm_input, pm_param, ISP_PM_BLK_ISP_SETTING, ISP_BLK_RGB_GAMC, 0, 0);
	isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_SINGLE_SETTING, &io_pm_input, &io_pm_output);

	if (io_pm_output.param_data != NULL) {
		struct isp_dev_gamma_info_v1 *gamma_info = io_pm_output.param_data->data_ptr;

		if (gamma_info != NULL) {
			cmr_s32 i;
			for (i = 0; i < 256; i++) {
				out_ptr->gamma[i] = (gamma_info->nodes[i].node_y + gamma_info->nodes[i + 1].node_y) / 2;
			}
		}
	}
exit:
	ISP_LOGV("done %ld", rtn);
	return rtn;
}

cmr_int ispalg_awb_post_process(cmr_handle isp_alg_handle, struct awb_ctrl_calc_result * result)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_pm_ioctl_input ioctl_input;
	struct isp_pm_param_data ioctl_data;
	struct isp_awbc_cfg awbc_cfg;

	if (!result || !isp_alg_handle) {
		rtn = ISP_PARAM_NULL;
		goto exit;
	}

	memset((void *)&ioctl_input, 0, sizeof(ioctl_input));
	memset((void *)&ioctl_data, 0, sizeof(ioctl_data));
	memset((void *)&awbc_cfg, 0, sizeof(awbc_cfg));

	/*set awb gain */
	awbc_cfg.r_gain = result->gain.r;
	awbc_cfg.g_gain = result->gain.g;
	awbc_cfg.b_gain = result->gain.b;
	awbc_cfg.r_offset = 0;
	awbc_cfg.g_offset = 0;
	awbc_cfg.b_offset = 0;

	ioctl_data.id = ISP_BLK_AWB_NEW;
	ioctl_data.cmd = ISP_PM_BLK_AWBC;
	ioctl_data.data_ptr = &awbc_cfg;
	ioctl_data.data_size = sizeof(awbc_cfg);

	ioctl_input.param_data_ptr = &ioctl_data;
	ioctl_input.param_num = 1;

	if (0 == awbc_cfg.r_gain && 0 == awbc_cfg.g_gain && 0 == awbc_cfg.b_gain) {
		awbc_cfg.r_gain = 1800;
		awbc_cfg.g_gain = 1024;
		awbc_cfg.b_gain = 1536;
	}
	rtn = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_AWB, (void *)&ioctl_input, NULL);

	if (result->use_ccm) {
		struct isp_pm_param_data param_data;
		struct isp_pm_ioctl_input input = { NULL, 0 };
		struct isp_pm_ioctl_output output = { NULL, 0 };

		memset(&param_data, 0x0, sizeof(param_data));
		BLOCK_PARAM_CFG(input, param_data, ISP_PM_BLK_CMC10, ISP_BLK_CMC10, result->ccm, 9 * sizeof(cmr_u16));

		rtn = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_OTHERS, &input, &output);
		ISP_TRACE_IF_FAIL(rtn, ("fail to set isp block param"));

	}
	cxt->awb_cxt.log_awb = result->log_awb.log;
	cxt->awb_cxt.log_awb_size = result->log_awb.size;

	if (result->use_lsc) {
		struct isp_pm_param_data param_data;
		struct isp_pm_ioctl_input input = { NULL, 0 };
		struct isp_pm_ioctl_output output = { NULL, 0 };

		memset(&param_data, 0x0, sizeof(param_data));
		BLOCK_PARAM_CFG(input, param_data, ISP_PM_BLK_LSC_MEM_ADDR, ISP_BLK_2D_LSC, result->lsc, result->lsc_size);

		rtn = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_OTHERS, &input, &output);
		ISP_TRACE_IF_FAIL(rtn, ("fail to set isp block param"));

		cxt->awb_cxt.log_alc_lsc = result->log_lsc.log;
		cxt->awb_cxt.log_alc_lsc_size = result->log_lsc.size;
	}

	if (ISP_SUCCESS == rtn) {
		cxt->awb_cxt.alc_awb = result->use_ccm | (result->use_lsc << 8);
	}

exit:
	ISP_LOGV("done %ld", rtn);
	return rtn;
}

cmr_int ispalg_start_awb_process(cmr_handle isp_alg_handle, struct isp_awb_calc_info * awb_calc_info, struct awb_ctrl_calc_result * awb_result)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	nsecs_t system_time0 = 0;
	nsecs_t system_time1 = 0;
	struct awb_ctrl_calc_param param;

	if (!isp_alg_handle || !awb_calc_info || !awb_result) {
		rtn = ISP_PARAM_NULL;
		goto exit;
	}

	memset((void *)&param, 0, sizeof(param));

	rtn = ispalg_awb_pre_process((cmr_handle) cxt, awb_calc_info, &param);

	system_time0 = isp_get_timestamp();
	rtn = awb_ctrl_process(cxt->awb_cxt.handle, &param, awb_result);
	system_time1 = isp_get_timestamp();
	ISP_LOGV("SYSTEM_TEST-awb:%lldms", system_time1 - system_time0);

	rtn = ispalg_awb_post_process((cmr_handle) cxt, awb_result);

exit:
	ISP_LOGV("done %ld", rtn);
	return rtn;
}

static cmr_int ispalg_aeawb_post_process(cmr_handle isp_alg_handle, struct isp_awb_calc_info *awb_calc_info, struct awb_ctrl_calc_result *result)
{
	cmr_int rtn = ISP_SUCCESS;

	if (!isp_alg_handle || !awb_calc_info || !result) {
		rtn = ISP_PARAM_NULL;
		goto exit;
	}

	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_awb_statistic_info *ae_stat_ptr = awb_calc_info->ae_stat_ptr;
	struct ae_calc_out *ae_result = &awb_calc_info->ae_result;
	struct smart_proc_input smart_proc_in;
	struct ae_monitor_info info;
	struct awb_size win_size = { 0, 0 };
	nsecs_t system_time0 = 0;
	nsecs_t system_time1 = 0;
	cmr_s32 bv = 0;
	cmr_s32 bv_gain = 0;
	struct ae_out_bv ae_out_bv;

	memset(&smart_proc_in, 0, sizeof(smart_proc_in));
	memset(&ae_out_bv, 0, sizeof(ae_out_bv));

	CMR_MSG_INIT(message);

	system_time0 = isp_get_timestamp();
	if (1 == cxt->smart_cxt.isp_smart_eb) {

		struct alsc_ver_info lsc_ver = { 0 };
		rtn = lsc_ctrl_ioctrl(cxt->lsc_cxt.handle, ALSC_GET_VER, NULL, (void *)&lsc_ver);
		if (ISP_SUCCESS != rtn) {
			ISP_LOGE("fail to Get ALSC ver info!");
		}
		ISP_LOGV("LSC_SPD_VERSION = %d", lsc_ver.LSC_SPD_VERSION);

		rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_GET_BV_BY_LUM_NEW, NULL, (void *)&bv);
		ISP_TRACE_IF_FAIL(rtn, ("AE_GET_BV_BY_LUM_NEW fail "));
		rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_GET_BV_BY_GAIN, NULL, (void *)&bv_gain);
		ISP_TRACE_IF_FAIL(rtn, ("AE_GET_BV_BY_GAIN fail "));
		smart_proc_in.cal_para.bv = bv;
		smart_proc_in.cal_para.bv_gain = bv_gain;
		smart_proc_in.cal_para.ct = result->ct;
		smart_proc_in.alc_awb = cxt->awb_cxt.alc_awb;
		smart_proc_in.handle_pm = cxt->handle_pm;
		smart_proc_in.mode_flag = cxt->commn_cxt.mode_flag;
		smart_proc_in.scene_flag = cxt->commn_cxt.scene_flag;
		smart_proc_in.LSC_SPD_VERSION = lsc_ver.LSC_SPD_VERSION;
		smart_proc_in.lock_nlm = cxt->smart_cxt.lock_en;
		rtn = _smart_calc(cxt->smart_cxt.handle, &smart_proc_in);
		ISP_TRACE_IF_FAIL(rtn, ("_smart_calc fail "));
		cxt->smart_cxt.log_smart = smart_proc_in.log;
		cxt->smart_cxt.log_smart_size = smart_proc_in.size;

		gCntSendMsgLsc++;
		if (0 == gCntSendMsgLsc % 3) {

			rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_GET_MONITOR_INFO, NULL, (void *)&info);

			struct isp_pm_param_data param_data_alsc;
			struct isp_pm_ioctl_input param_data_alsc_input = { NULL, 0 };
			struct isp_pm_ioctl_output param_data_alsc_output = { NULL, 0 };
			memset(&param_data_alsc, 0, sizeof(param_data_alsc));

			BLOCK_PARAM_CFG(param_data_alsc_input, param_data_alsc, ISP_PM_BLK_LSC_GET_LSCTAB, ISP_BLK_2D_LSC, NULL, 0);
			rtn = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_SINGLE_SETTING, (void *)&param_data_alsc_input, (void *)&param_data_alsc_output);
			ISP_TRACE_IF_FAIL(rtn, ("ISP_PM_CMD_GET_SINGLE_SETTING fail "));
			cxt->lsc_cxt.lsc_tab_address = param_data_alsc_output.param_data->data_ptr;
			cxt->lsc_cxt.lsc_tab_size = param_data_alsc_output.param_data->data_size;

			/*send message to alsc process */
			struct isp_alsc_calc_info alsc_info;

			cmr_u32 *buf_stat_lsc = aeStatistic;
			cmr_u32 *ptr_r_stat = buf_stat_lsc;
			cmr_u32 *ptr_g_stat = buf_stat_lsc + 1024;
			cmr_u32 *ptr_b_stat = buf_stat_lsc + 1024 * 2;
			memcpy(ptr_r_stat, ae_stat_ptr->r_info, 1024 * sizeof(cmr_u32));
			memcpy(ptr_g_stat, ae_stat_ptr->g_info, 1024 * sizeof(cmr_u32));
			memcpy(ptr_b_stat, ae_stat_ptr->b_info, 1024 * sizeof(cmr_u32));

			alsc_info.stat_ptr = buf_stat_lsc;
			alsc_info.awb_b_gain = result->gain.b;
			alsc_info.awb_r_gain = result->gain.r;
			alsc_info.awb_ct = result->ct;
			alsc_info.stat_img_size.w = info.win_num.w;
			alsc_info.stat_img_size.h = info.win_num.h;
			alsc_info.stable = awb_calc_info->ae_result.is_stab;
			alsc_info.image_width = cxt->commn_cxt.src.w;
			alsc_info.image_height = cxt->commn_cxt.src.h;

			rtn =
			    alsc_calc(isp_alg_handle, ptr_r_stat, ptr_g_stat, ptr_b_stat, &alsc_info.stat_img_size, &alsc_info.win_size, alsc_info.image_width,
				      alsc_info.image_height, alsc_info.awb_ct, alsc_info.awb_r_gain, alsc_info.awb_b_gain, alsc_info.stable);
			ISP_TRACE_IF_FAIL(rtn, ("alsc_calc fail "));
		}
	}
	system_time1 = isp_get_timestamp();
	ISP_LOGV("SYSTEM_TEST-smart:%lldms", system_time1 - system_time0);

	isp_cur_bv = bv;
	isp_cur_ct = result->ct;

	ae_out_bv.ae_result = ae_result;
	ae_out_bv.bv = bv;
	rtn = af_ctrl_ioctrl(cxt->af_cxt.handle, AF_CMD_SET_AE_INFO, (void *)ae_stat_ptr, (void *)&ae_out_bv);
	ISP_TRACE_IF_FAIL(rtn, ("AF_CMD_SET_AE_INFO fail "));
	rtn = af_ctrl_ioctrl(cxt->af_cxt.handle, AF_CMD_SET_AWB_INFO, (void *)result, NULL);
	ISP_TRACE_IF_FAIL(rtn, ("AF_CMD_SET_AWB_INFO fail "));
	message.msg_type = ISP_CTRL_EVT_AF;
	message.sub_msg_type = AF_DATA_AE;
	message.sync_flag = CMR_MSG_SYNC_NONE;
	message.alloc_flag = 0;
	message.data = (void *)ae_result;
	rtn = cmr_thread_msg_send(cxt->thr_handle, &message);
	ISP_LOGV("done message_data %p rtn %ld", message.data, rtn);

	message.msg_type = ISP_CTRL_EVT_AF;
	message.sub_msg_type = AF_DATA_IMG_BLK;
	message.sync_flag = CMR_MSG_SYNC_NONE;
	message.alloc_flag = 0;
	message.data = (void *)ae_stat_ptr;
	rtn = cmr_thread_msg_send(cxt->thr_handle, &message);
	ISP_TRACE_IF_FAIL(rtn, ("cmr_thread_msg_send fail "));

exit:
	ISP_LOGV("done rtn %ld", rtn);
	return rtn;
}

cmr_int ispalg_ae_awb_process(cmr_handle isp_alg_handle)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_awb_calc_info awb_calc_info;
	struct awb_ctrl_calc_result awb_result;
	struct ae_calc_out ae_result;
	struct isp_awb_statistic_info *ae_stat_ptr = NULL;

	ISP_CHECK_HANDLE_VALID(isp_alg_handle);
	memset((void *)&awb_calc_info, 0, sizeof(awb_calc_info));
	memset(&awb_result, 0, sizeof(awb_result));
	memset(&ae_result, 0, sizeof(ae_result));

	rtn = ispalg_start_ae_process((cmr_handle) cxt, &awb_calc_info);

	if (rtn) {
		goto exit;
	}

	rtn = ispalg_start_awb_process((cmr_handle) cxt, &awb_calc_info, &awb_result);
	if (rtn) {
		goto exit;
	}

	rtn = ispalg_aeawb_post_process((cmr_handle) cxt, &awb_calc_info, &awb_result);

exit:

	return rtn;
}

cmr_int ispalg_afl_process(cmr_handle isp_alg_handle, void *data)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	cmr_int bypass = 0;
	cmr_u32 cur_flicker = 0;
	cmr_u32 cur_exp_flag = 0;
	cmr_s32 ae_exp_flag = 0;
	float ae_exp = 0.0;
	cmr_int nxt_flicker = 0;
	struct isp_awb_statistic_info ae_stat_ptr;
	struct isp_pm_param_data param_data;
	struct isp_pm_ioctl_input input = { NULL, 0 };
	struct isp_pm_ioctl_output output = { NULL, 0 };
	struct afl_proc_in afl_input;
	struct afl_ctrl_proc_out afl_output;
	struct isp_statis_info *statis_info = NULL;
	struct isp_statis_buf_input statis_buf;
	cmr_u32 k_addr = 0;
	cmr_u32 u_addr = 0;

	ISP_CHECK_HANDLE_VALID(isp_alg_handle);
	statis_info = (struct isp_statis_info *)data;
	k_addr = statis_info->phy_addr;
	u_addr = statis_info->vir_addr;
	//memcpy((void *)&ae_stat_ptr, (void *)u_addr, sizeof(struct isp_awb_statistic_info));
	ae_stat_ptr = cxt->aem_stats;

	bypass = 1;
	isp_dev_anti_flicker_bypass(cxt->dev_access_handle, bypass);

	rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_GET_FLICKER_MODE, NULL, &cur_flicker);
	ISP_LOGV("cur flicker mode %d", cur_flicker);

	//exposure 1/33 s  -- 302921 (+/-10)
	rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_GET_EXP, NULL, &ae_exp);
	if (fabs(ae_exp - 0.06) < 0.000001 || ae_exp > 0.06) {
		ae_exp_flag = 1;
	}
	ISP_LOGV("ae_exp %f; ae_exp_flag %d", ae_exp, ae_exp_flag);

	rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_GET_FLICKER_SWITCH_FLAG, &cur_exp_flag, NULL);
	ISP_LOGV("cur exposure flag %d", cur_exp_flag);

	afl_input.ae_stat_ptr = &ae_stat_ptr;
	afl_input.ae_exp_flag = ae_exp_flag;
	afl_input.cur_exp_flag = cur_exp_flag;
	afl_input.cur_flicker = cur_flicker;
	afl_input.vir_addr = u_addr;

	rtn = afl_ctrl_process(cxt->afl_cxt.handle, &afl_input, &afl_output);

	memset((void *)&statis_buf, 0, sizeof(statis_buf));
	statis_buf.buf_size = statis_info->buf_size;
	statis_buf.phy_addr = statis_info->phy_addr;
	statis_buf.vir_addr = statis_info->vir_addr;
	statis_buf.buf_property = ISP_AFL_BLOCK;
	statis_buf.buf_flag = 1;
	rtn = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_STSTIS_BUF, &statis_buf, NULL);
	if (rtn) {
		ISP_LOGE("fail to set statis buf");
	}
	//change ae table
	if (afl_output.flag) {
		if (afl_output.cur_flicker) {
			nxt_flicker = AE_FLICKER_50HZ;
		} else {
			nxt_flicker = AE_FLICKER_60HZ;
		}
		rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_SET_FLICKER, &nxt_flicker, NULL);
	}

	bypass = 0;
	isp_dev_anti_flicker_bypass(cxt->dev_access_handle, bypass);

exit:
	ISP_LOGV("done rtn %ld", rtn);
	return rtn;
}

static cmr_int ispalg_af_process(cmr_handle isp_alg_handle, cmr_u32 data_type, void *in_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct afctrl_calc_in calc_param;
	struct afctrl_calc_out calc_result;
	struct isp_statis_info *statis_info = NULL;
	cmr_u32 k_addr = 0;
	cmr_u32 u_addr = 0;
	cmr_s32 i = 0;

	ISP_CHECK_HANDLE_VALID(isp_alg_handle);
	memset((void *)&calc_param, 0, sizeof(calc_param));
	memset((void *)&calc_result, 0, sizeof(calc_result));
	ISP_LOGV("begin data_type %d", data_type);
	switch (data_type) {
	case AF_DATA_AF:{
			struct isp_statis_buf_input statis_buf;
			statis_info = (struct isp_statis_info *)in_ptr;
			k_addr = statis_info->kaddr;
			u_addr = statis_info->vir_addr;

			cmr_u32 af_temp[30];
			for (i = 0; i < 30; i++) {
				af_temp[i] = *((cmr_u32 *) u_addr + i);
			}
			calc_param.data_type = AF_DATA_AF;
			calc_param.sensor_fps = cxt->sensor_fps;
			calc_param.data = (void *)(af_temp);
			rtn = af_ctrl_process(cxt->af_cxt.handle, (void *)&calc_param, &calc_result);

			memset((void *)&statis_buf, 0, sizeof(statis_buf));
			statis_buf.buf_size = statis_info->buf_size;
			statis_buf.phy_addr = statis_info->phy_addr;
			statis_buf.vir_addr = statis_info->vir_addr;
			statis_buf.kaddr = statis_info->kaddr;
			statis_buf.buf_property = ISP_AFM_BLOCK;
			statis_buf.buf_flag = 1;
			rtn = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_STSTIS_BUF, &statis_buf, NULL);
			if (rtn) {
				ISP_LOGE("fail to set statis buf");
			}
			break;
		}
	case AF_DATA_IMG_BLK:{
			struct af_img_blk_info img_blk_info;
			memset((void *)&img_blk_info, 0, sizeof(img_blk_info));
			img_blk_info.block_w = 32;
			img_blk_info.block_h = 32;
			img_blk_info.chn_num = 3;
			img_blk_info.pix_per_blk = 1;
			img_blk_info.data = (cmr_u32 *) in_ptr;
			calc_param.data_type = AF_DATA_IMG_BLK;
			calc_param.data = (void *)(&img_blk_info);
			rtn = af_ctrl_process(cxt->af_cxt.handle, (void *)&calc_param, (void *)&calc_result);
			break;
		}
	case AF_DATA_AE:{
			struct af_ae_info ae_info;
			struct ae_calc_out *ae_result = (struct ae_calc_out *)in_ptr;
			cmr_u32 line_time = ae_result->line_time;
			cmr_u32 frame_len = ae_result->frame_line;
			cmr_u32 dummy_line = ae_result->cur_dummy;
			cmr_u32 exp_line = ae_result->cur_exp_line;
			cmr_u32 frame_time;

			memset((void *)&ae_info, 0, sizeof(ae_info));
			ae_info.exp_time = ae_result->cur_exp_line * line_time / 10;
			ae_info.gain = ae_result->cur_again;
			frame_len = (frame_len > (exp_line + dummy_line)) ? frame_len : (exp_line + dummy_line);
			frame_time = frame_len * line_time / 10;
			frame_time = frame_time > 0 ? frame_time : 1;
			ae_info.cur_fps = 1000000 / frame_time;
			ae_info.cur_lum = ae_result->cur_lum;
			ae_info.target_lum = 128;
			ae_info.is_stable = ae_result->is_stab;
			calc_param.data_type = AF_DATA_AE;
			calc_param.data = (void *)(&ae_info);
			rtn = af_ctrl_process(cxt->af_cxt.handle, (void *)&calc_param, (void *)&calc_result);
			break;
		}
	case AF_DATA_FD:{
			break;
		}
	default:{
			break;
		}
	}

	ISP_LOGV("done %ld", rtn);
	return rtn;
}

static cmr_int ispalg_pdaf_process(cmr_handle isp_alg_handle, cmr_u32 data_type, void *in_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_statis_info *statis_info = NULL;
	cmr_u32 k_addr = 0;
	cmr_u32 u_addr = 0;
	cmr_s32 i = 0;
	struct pdaf_ctrl_process_in pdaf_param_in;
	struct isp_statis_buf_input statis_buf;
	UNUSED(data_type);

	memset((void *)&pdaf_param_in, 0x00, sizeof(pdaf_param_in));
	ISP_CHECK_HANDLE_VALID(isp_alg_handle);

	statis_info = (struct isp_statis_info *)in_ptr;
	k_addr = statis_info->phy_addr;
	u_addr = statis_info->vir_addr;

	cmr_u32 pdaf_temp[30];
	for (i = 0; i < 30; i++) {
		pdaf_temp[i] = *((cmr_u32 *) u_addr + i);
	}

	pdaf_param_in.dBv = pdaf_temp[0];
	memset((void *)&statis_buf, 0, sizeof(statis_buf));

	pdaf_param_in.u_addr = u_addr;
	rtn = pdaf_ctrl_process(cxt->pdaf_cxt.handle, &pdaf_param_in, NULL);

	statis_buf.buf_size = statis_info->buf_size;
	statis_buf.phy_addr = statis_info->phy_addr;
	statis_buf.vir_addr = statis_info->vir_addr;
	statis_buf.kaddr = statis_info->kaddr;
	statis_buf.buf_property = ISP_PDAF_BLOCK;
	statis_buf.buf_flag = 1;
	rtn = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_STSTIS_BUF, &statis_buf, NULL);

	ISP_LOGV("done %ld", rtn);
	return rtn;
}

static cmr_u32 binning_data_cvt(cmr_u32 bayermode, cmr_u32 width, cmr_u32 height, cmr_u16 * raw_in, struct isp_binning_statistic_info *binning_info)
{
	cmr_u32 rtn = 0;
	cmr_u32 i, j;
	cmr_u32 *binning_r = binning_info->r_info;
	cmr_u32 *binning_g = binning_info->g_info;
	cmr_u32 *binning_b = binning_info->b_info;
	cmr_u32 blk_id = 0;
	cmr_u32 *binning_block[4] = { binning_g, binning_r, binning_b, binning_g };
	cmr_u16 pixel_type;

	if (NULL == raw_in || NULL == binning_r || NULL == binning_g || NULL == binning_b) {
		ISP_LOGE("fail to check input param");
		return -1;
	}

	if ((width % 2 != 0) || (height % 2 != 0)) {
		ISP_LOGE("fail to Alignment");
		return -1;
	}

	for (i = 0; i < height; i += 2) {
		for (j = 0; j < width; j += 2) {
			binning_r[blk_id] = 0;
			binning_g[blk_id] = 0;
			binning_b[blk_id] = 0;
			pixel_type = bayermode ^ ((((i) & 1) << 1) | ((j) & 1));
			binning_block[pixel_type][blk_id] += raw_in[i * width + j];

			pixel_type = bayermode ^ ((((i) & 1) << 1) | ((j + 1) & 1));
			binning_block[pixel_type][blk_id] += raw_in[i * width + j + 1];

			pixel_type = bayermode ^ ((((i + 1) & 1) << 1) | ((j) & 1));
			binning_block[pixel_type][blk_id] += raw_in[(i + 1) * width + j];

			pixel_type = bayermode ^ ((((i + 1) & 1) << 1) | ((j + 1) & 1));
			binning_block[pixel_type][blk_id] += raw_in[(i + 1) * width + j + 1];

			binning_g[blk_id] = (binning_g[blk_id] + 1) >> 1;

			blk_id++;
		}
	}

	binning_info->binning_size.w = width / 2;
	binning_info->binning_size.h = height / 2;
	return rtn;
}

static cmr_int ispalg_binning_stat_data_parser(cmr_handle isp_alg_handle, void *data)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_statis_buf_input statis_buf;
	struct isp_statis_info *statis_info = (struct isp_statis_info *)data;
	cmr_u64 k_addr = 0;
	cmr_u64 u_addr = 0;
	cmr_u32 val = 0;
	cmr_u32 last_val0 = 0;
	cmr_u32 last_val1 = 0;
	cmr_u32 binning_hx = 4;
	cmr_u32 binning_vx = 4;
	cmr_u32 src_w = cxt->commn_cxt.src.w;
	cmr_u32 src_h = cxt->commn_cxt.src.h;
	cmr_u32 binnng_w = (src_w >> binning_hx) & ~0x1;
	cmr_u32 binnng_h = (src_h >> binning_vx) & ~0x1;
	cmr_u32 double_binning_num = binnng_w * binnng_h / 6 * 2;
	cmr_u32 remainder = 0;
	cmr_u16 *binning_img_data = NULL;
	cmr_u16 *binning_img_ptr = NULL;
	cmr_u32 bayermode = cxt->commn_cxt.image_pattern;
	cmr_u32 i = 0;

	ISP_CHECK_HANDLE_VALID(isp_alg_handle);
	k_addr = statis_info->phy_addr;
	u_addr = statis_info->vir_addr;

	ISP_LOGV("bayer: %d, src_size=(%d,%d)\n", bayermode, src_w, src_h);

	binning_img_data = (cmr_u16 *) malloc(binnng_w * binnng_h * 2);
	if (binning_img_data == NULL) {
		ISP_LOGE("fail to malloc binning img data\n");
		return -1;
	}
	memset(binning_img_data, 0, binnng_w * binnng_h * 2);
	binning_img_ptr = binning_img_data;

	for (i = 0; i < double_binning_num; i++) {
		val = *((cmr_u32 *) u_addr + i);
		*binning_img_ptr++ = val & 0x3FF;
		*binning_img_ptr++ = (val >> 10) & 0x3FF;
		*binning_img_ptr++ = (val >> 20) & 0x3FF;
	}
	remainder = binnng_w * binnng_h % 6;
	last_val0 = *((cmr_u32 *) u_addr + i);
	last_val1 = *((cmr_u32 *) u_addr + i + 1);

	switch (remainder) {
	case 1:{
			*binning_img_ptr++ = last_val0 & 0x3FF;
			break;
		}
	case 2:{
			*binning_img_ptr++ = last_val0 & 0x3FF;
			*binning_img_ptr++ = (last_val0 >> 10) & 0x3FF;
			break;
		}
	case 3:{
			*binning_img_ptr++ = last_val0 & 0x3FF;
			*binning_img_ptr++ = (last_val0 >> 10) & 0x3FF;
			*binning_img_ptr++ = (last_val0 >> 20) & 0x3FF;
			break;
		}
	case 4:{
			*binning_img_ptr++ = last_val0 & 0x3FF;
			*binning_img_ptr++ = (last_val0 >> 10) & 0x3FF;
			*binning_img_ptr++ = (last_val0 >> 20) & 0x3FF;
			*binning_img_ptr++ = last_val1 & 0x3FF;
			break;
		}
	case 5:{
			*binning_img_ptr++ = last_val0 & 0x3FF;
			*binning_img_ptr++ = (last_val0 >> 10) & 0x3FF;
			*binning_img_ptr++ = (last_val0 >> 20) & 0x3FF;
			*binning_img_ptr++ = last_val1 & 0x3FF;
			*binning_img_ptr++ = (last_val1 >> 10) & 0x3FF;
			break;
		}
	default:
		break;
	}

	binning_data_cvt(bayermode, binnng_w, binnng_h, binning_img_data, &cxt->binning_stats);

	ISP_LOGV("binning_stats_size=(%d, %d)\n", cxt->binning_stats.binning_size.w, cxt->binning_stats.binning_size.h);

	memset((void *)&statis_buf, 0, sizeof(statis_buf));
	statis_buf.buf_size = statis_info->buf_size;
	statis_buf.phy_addr = statis_info->phy_addr;
	statis_buf.vir_addr = statis_info->vir_addr;
	statis_buf.buf_property = ISP_BINNING_BLOCK, statis_buf.buf_flag = 1;
	rtn = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_STSTIS_BUF, &statis_buf, NULL);
	if (rtn) {
		ISP_LOGE("fail to set statis buf");
	}

	if (binning_img_data != NULL) {
		free(binning_img_data);
		binning_img_data = NULL;
	}

	ISP_LOGV("done %ld", rtn);
	return rtn;

}

static cmr_int _ispProcessEndHandle(cmr_handle isp_alg_handle)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct ips_out_param callback_param = { 0x00 };
	struct isp_interface_param_v1 *interface_ptr_v1 = &cxt->commn_cxt.interface_param_v1;

	ISP_LOGV("isp start raw proc callback\n");
	if (NULL != cxt->commn_cxt.callback) {
		callback_param.output_height = interface_ptr_v1->data.input_size.h;
		ISP_LOGV("callback ISP_PROC_CALLBACK");
		cxt->commn_cxt.callback(cxt->commn_cxt.caller_id, ISP_CALLBACK_EVT | ISP_PROC_CALLBACK, (void *)&callback_param, sizeof(struct ips_out_param));
	}

	ISP_LOGV("isp end raw proc callback\n");
	return rtn;
}

void ispalg_dev_evt_cb(cmr_int evt, void *data, void *privdata)
{
	cmr_int rtn = ISP_SUCCESS;
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
	rtn = cmr_thread_msg_send(cxt->thr_handle, &message);
	if (rtn) {
		ISP_LOGE("fail to send a message, evt is %ld", evt);
		free(message.data);
	}

}

cmr_int isp_alg_thread_proc(struct cmr_msg *message, void *p_data)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)p_data;

	if (!message || !p_data) {
		ISP_LOGE("fail to check input param ");
		goto exit;
	}
	ISP_LOGV("message.msg_type 0x%x, data %p", message->msg_type, message->data);

	switch (message->msg_type) {
	case ISP_CTRL_EVT_TX:
		rtn = _ispProcessEndHandle((cmr_handle) cxt);
		break;
	case ISP_CTRL_EVT_AE:{
			rtn = ispalg_aem_stat_data_parser((cmr_handle) cxt, message->data);
			break;
		}
	case ISP_CTRL_EVT_SOF:
		if (cxt->gamma_sof_cnt_eb) {
			cxt->gamma_sof_cnt++;
			if (cxt->gamma_sof_cnt >= 2) {
				cxt->update_gamma_eb = 1;
			}
		}

		if (cxt->aem_is_update) {
			rtn = ispalg_ae_awb_process((cmr_handle) cxt);
			cxt->aem_is_update = 0;
			if (rtn)
				goto exit;
		}
		rtn = ispalg_handle_sensor_sof((cmr_handle) cxt);
		break;
	case ISP_PROC_AFL_DONE:
		if (cxt->afl_cxt.afl_mode > AE_FLICKER_60HZ) {
			rtn = ispalg_afl_process((cmr_handle) cxt, message->data);
		}
		break;
	case ISP_CTRL_EVT_AF:
		rtn = ispalg_af_process((cmr_handle) cxt, message->sub_msg_type, message->data);
		break;

	case ISP_CTRL_EVT_BINNING:
		rtn = ispalg_binning_stat_data_parser((cmr_handle) cxt, message->data);
		break;
	case ISP_CTRL_EVT_PDAF:
		rtn = ispalg_pdaf_process((cmr_handle) cxt, message->sub_msg_type, message->data);
		break;

	default:
		ISP_LOGV("don't support msg");
		break;
	}
exit:
	ISP_LOGV("done %ld", rtn);
	return rtn;
}

cmr_int isp_alg_create_thread(cmr_handle isp_alg_handle)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;

	rtn = cmr_thread_create(&cxt->thr_handle, ISP_THREAD_QUEUE_NUM, isp_alg_thread_proc, (void *)cxt);

	if (CMR_MSG_SUCCESS != rtn) {
		ISP_LOGE("fail to create process thread");
		rtn = ISP_ERROR;
	}

	return rtn;
}

cmr_int isp_alg_destroy_thread_proc(cmr_handle isp_alg_handle)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;

	if (!isp_alg_handle) {
		ISP_LOGE("fail to check isp_alg_handle");
		rtn = ISP_ERROR;
		goto exit;
	}

	if (cxt->thr_handle) {
		rtn = cmr_thread_destroy(cxt->thr_handle);
		if (!rtn) {
			cxt->thr_handle = (cmr_handle) NULL;
		} else {
			ISP_LOGE("fail to destroy process thread");
		}
	}
exit:
	ISP_LOGI("done %ld", rtn);
	return rtn;
}

static cmr_u32 _ispGetIspParamIndex(struct sensor_raw_resolution_info *input_size_trim, struct isp_size *size)
{
	cmr_u32 param_index = 0x01;
	cmr_u32 i;

	for (i = 0x01; i < ISP_INPUT_SIZE_NUM_MAX; i++) {
		if (size->h == input_size_trim[i].height) {
			param_index = i;
			break;
		}
	}

	return param_index;
}

static cmr_int isp_ae_sw_init(struct isp_alg_fw_context *cxt)
{
	cmr_int rtn = ISP_SUCCESS;
	struct ae_init_in ae_input;
	struct isp_pm_ioctl_output output;
	struct isp_pm_param_data *param_data = NULL;
	struct isp_flash_param *flash = NULL;
	cmr_u32 num = 0;
	cmr_u32 i = 0;

	memset(&output, 0, sizeof(output));
	memset((void *)&ae_input, 0, sizeof(ae_input));
	rtn = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_INIT_AE, NULL, &output);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to get ae init param");
		return rtn;
	}

	if (0 == output.param_num) {
		ISP_LOGE("fail to check param: ae param num=%d", output.param_num);
		return ISP_ERROR;
	}

	param_data = output.param_data;
	ae_input.has_force_bypass = param_data->user_data[0];
	for (i = 0; i < output.param_num; ++i) {
		if (NULL != param_data->data_ptr) {
			ae_input.param[num].param = param_data->data_ptr;
			ae_input.param[num].size = param_data->data_size;
			++num;
		}
		++param_data;
	}
	ae_input.param_num = num;
	ae_input.resolution_info.frame_size.w = cxt->commn_cxt.src.w;
	ae_input.resolution_info.frame_size.h = cxt->commn_cxt.src.h;
	ae_input.resolution_info.frame_line = cxt->commn_cxt.input_size_trim[1].frame_line;
	ae_input.resolution_info.line_time = cxt->commn_cxt.input_size_trim[1].line_time;
	ae_input.resolution_info.sensor_size_index = 1;
	ae_input.camera_id = cxt->camera_id;
	ae_input.lib_param = cxt->lib_use_info->ae_lib_info;
	ae_input.caller_handle = (cmr_handle) cxt;
	ae_input.ae_set_cb = isp_ae_set_cb;
	cxt->ae_cxt.win_num.w =32;
	cxt->ae_cxt.win_num.h = 32;
	ae_input.monitor_win_num.w = cxt->ae_cxt.win_num.w ;
	ae_input.monitor_win_num.h = cxt->ae_cxt.win_num.h;

	if (AL_AE_LIB == cxt->lib_use_info->ae_lib_info.product_id) {
		//AIS needs AWB infomation at Init
		struct isp_otp_info *otp_info = (struct isp_otp_info *)cxt->handle_otp;
		if (NULL != otp_info) {
			struct isp_cali_awb_info *awb_cali_info = otp_info->awb.data_ptr;
			ae_input.lsc_otp_golden = otp_info->lsc_golden;
			ae_input.lsc_otp_random = otp_info->lsc_random;
			ae_input.lsc_otp_width = otp_info->width;
			ae_input.lsc_otp_height = otp_info->height;
			if (NULL != awb_cali_info) {
				ae_input.otp_info.gldn_stat_info.r = awb_cali_info->golden_avg[0];
				ae_input.otp_info.gldn_stat_info.g = awb_cali_info->golden_avg[1];
				ae_input.otp_info.gldn_stat_info.b = awb_cali_info->golden_avg[2];
				ae_input.otp_info.rdm_stat_info.r = awb_cali_info->ramdon_avg[0];
				ae_input.otp_info.rdm_stat_info.g = awb_cali_info->ramdon_avg[1];
				ae_input.otp_info.rdm_stat_info.b = awb_cali_info->ramdon_avg[2];
			}
		}
	}

	rtn = _isp_get_flash_cali_param(cxt->handle_pm, &flash);
	rtn = ae_ctrl_init(&ae_input, &cxt->ae_cxt.handle);
	ISP_TRACE_IF_FAIL(rtn, ("fail to do ae_ctrl_init"));
	rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_SET_FLASH_ON_OFF_THR, (void *)&flash->cur.auto_flash_thr, NULL);

	return rtn;
}

static cmr_int isp_awb_sw_init(struct isp_alg_fw_context *cxt)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_pm_ioctl_input input;
	struct isp_pm_ioctl_output output;
	struct awb_ctrl_init_param param;
	struct ae_monitor_info info;

	memset((void *)&input, 0, sizeof(input));
	memset((void *)&output, 0, sizeof(output));
	memset((void *)&param, 0, sizeof(param));

	rtn = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_INIT_AWB, &input, &output);
	ISP_TRACE_IF_FAIL(rtn, ("fail to get awb init param"));

	rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_GET_MONITOR_INFO, NULL, (void *)&info);
	ISP_TRACE_IF_FAIL(rtn, ("fail to get ae monitor info"));
	if (ISP_SUCCESS == rtn) {
		if (AL_AE_LIB == cxt->lib_use_info->ae_lib_info.product_id) {
			void *ais_handle = NULL;
			rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_GET_AIS_HANDLE, NULL, (void *)&ais_handle);
			ISP_TRACE_IF_FAIL(rtn, ("fail to get ae ais handle"));
			param.priv_handle = ais_handle;
			param.awb_enable = 1;
		} else {
			//if use AIS, AWB this does not  need for awb_ctrl
			param.camera_id = cxt->camera_id;
			param.base_gain = 1024;
			param.awb_enable = 1;
			param.wb_mode = 0;
			param.stat_img_format = AWB_CTRL_STAT_IMG_CHN;
			param.stat_img_size.w = info.win_num.w;
			param.stat_img_size.h = info.win_num.h;
			param.stat_win_size.w = info.win_size.w;
			param.stat_win_size.h = info.win_size.h;

			param.stat_img_size.w = cxt->binning_stats.binning_size.w;
			param.stat_img_size.h = cxt->binning_stats.binning_size.h;

			param.tuning_param = output.param_data->data_ptr;
			param.param_size = output.param_data->data_size;
			param.lib_param = cxt->lib_use_info->awb_lib_info;
			ISP_LOGV(" param addr is %p size %d", param.tuning_param, param.param_size);
			struct isp_otp_info *otp_info = (struct isp_otp_info *)cxt->handle_otp;
			if (NULL != otp_info) {
				struct isp_cali_awb_info *awb_cali_info = otp_info->awb.data_ptr;
				param.lsc_otp_golden = otp_info->lsc_golden;
				param.lsc_otp_random = otp_info->lsc_random;
				param.lsc_otp_width = otp_info->width;
				param.lsc_otp_height = otp_info->height;
			}
			if (NULL != cxt->otp_data) {
				param.otp_info.gldn_stat_info.r = cxt->otp_data->single_otp.awb_golden_info.gain_r;
				param.otp_info.gldn_stat_info.g = cxt->otp_data->single_otp.awb_golden_info.gain_g;
				param.otp_info.gldn_stat_info.b = cxt->otp_data->single_otp.awb_golden_info.gain_b;
				param.otp_info.rdm_stat_info.r = cxt->otp_data->single_otp.iso_awb_info.gain_r;
				param.otp_info.rdm_stat_info.g = cxt->otp_data->single_otp.iso_awb_info.gain_g;
				param.otp_info.rdm_stat_info.b = cxt->otp_data->single_otp.iso_awb_info.gain_b;
				ISP_LOGV("otp golden [%d %d %d]  rdn [%d %d %d ]", param.otp_info.gldn_stat_info.r, param.otp_info.gldn_stat_info.g,
					 param.otp_info.gldn_stat_info.b, param.otp_info.rdm_stat_info.r, param.otp_info.rdm_stat_info.g, param.otp_info.rdm_stat_info.b);
			} else {
				ISP_LOGV("otp is not used");
				param.otp_info.gldn_stat_info.r = 0;
				param.otp_info.gldn_stat_info.g = 0;
				param.otp_info.gldn_stat_info.b = 0;
				param.otp_info.rdm_stat_info.r = 0;
				param.otp_info.rdm_stat_info.g = 0;
				param.otp_info.rdm_stat_info.b = 0;
			}
		}

		rtn = awb_ctrl_init(&param, &cxt->awb_cxt.handle);
		ISP_TRACE_IF_FAIL(rtn, ("fail to do awb_ctrl_init"));

	} else {
		ISP_LOGE("fail to get awb init param!");
	}
	ISP_LOGI("done %ld", rtn);
	return rtn;
}

static cmr_int isp_afl_sw_init(struct isp_alg_fw_context *cxt, struct isp_alg_sw_init_in *input_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	struct afl_ctrl_init_in afl_input;
	if (!cxt || !input_ptr) {
		rtn = ISP_PARAM_ERROR;
		goto exit;
	}

	afl_input.dev_handle = cxt->dev_access_handle;
	afl_input.size.w = input_ptr->size.w;
	afl_input.size.h = input_ptr->size.h;
	afl_input.vir_addr = &cxt->afl_cxt.vir_addr;
	afl_input.caller_handle = (cmr_handle) cxt;
	afl_input.afl_set_cb = isp_afl_set_cb;
	rtn = afl_ctrl_init(&cxt->afl_cxt.handle, &afl_input);
exit:
	ISP_LOGI("done %ld", rtn);
	return rtn;
}

static cmr_int isp_smart_sw_init(struct isp_alg_fw_context *cxt)
{
	cmr_int rtn = ISP_SUCCESS;
	struct smart_init_param smart_init_param;
	struct isp_pm_ioctl_input pm_input;
	struct isp_pm_ioctl_output pm_output;
	cmr_u32 i = 0;

	memset(&pm_input, 0, sizeof(pm_input));
	memset(&pm_output, 0, sizeof(pm_output));

	cxt->smart_cxt.isp_smart_eb = 0;
	memset(&smart_init_param, 0, sizeof(smart_init_param));

	rtn = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_INIT_SMART, &pm_input, &pm_output);
	if (ISP_SUCCESS == rtn) {
		for (i = 0; i < pm_output.param_num; ++i) {
			smart_init_param.tuning_param[i].data.size = pm_output.param_data[i].data_size;
			smart_init_param.tuning_param[i].data.data_ptr = pm_output.param_data[i].data_ptr;
		}
	} else {
		ISP_LOGE("fail to get smart init param ");
		return rtn;
	}

	cxt->smart_cxt.handle = smart_ctl_init(&smart_init_param, NULL);
	if (NULL == cxt->smart_cxt.handle) {
		ISP_LOGE("fail to do smart init");
		return rtn;
	}
exit:
	ISP_LOGI("done %ld", rtn);
	return rtn;
}

static cmr_int isp_af_sw_init(struct isp_alg_fw_context *cxt)
{
	cmr_int rtn = ISP_SUCCESS;
	struct afctrl_init_in af_input;
	struct isp_pm_ioctl_input af_pm_input;
	struct isp_pm_ioctl_output af_pm_output;
	//struct af_tuning_param *af_tuning = NULL;
	cmr_u32 i;

	memset((void *)&af_input, 0, sizeof(af_input));
	memset((void *)&af_pm_input, 0, sizeof(af_pm_input));
	memset((void *)&af_pm_output, 0, sizeof(af_pm_output));

	af_input.camera_id = cxt->camera_id;
	af_input.lib_param = cxt->lib_use_info->af_lib_info;
	af_input.caller_handle = (cmr_handle) cxt;
	af_input.af_set_cb = isp_af_set_cb;
	af_input.src.w = cxt->commn_cxt.src.w;
	af_input.src.h = cxt->commn_cxt.src.h;

#if 0				//used for af1.0
	rtn = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_INIT_AF, &af_pm_input, &af_pm_output);
	if (ISP_SUCCESS == rtn) {
		af_input.af_bypass = 0;
		af_input.af_mode = 0;
		af_input.tuning_param_cnt = af_pm_output.param_num;
		af_input.cur_tuning_mode = 0;
		af_tuning = (struct af_tuning_param *)malloc(sizeof(struct af_tuning_param) * af_pm_output.param_num);
		if (NULL == af_tuning) {
			ISP_LOGE("fail to malloc af_tuning buf!");
			return ISP_ERROR;
		}
		for (i = 0; i < af_pm_output.param_num; i++) {
			af_tuning[i].cfg_mode = (af_pm_output.param_data->id & 0xffff0000) >> 16;
			af_tuning[i].data = af_pm_output.param_data->data_ptr;
			af_tuning[i].data_len = af_pm_output.param_data->data_size;
			af_pm_output.param_data++;
		}

		af_input.tuning_param = af_tuning;
		af_input.plat_info.afm_filter_type_cnt = 1;
		af_input.plat_info.afm_win_max_cnt = 9;
		af_input.plat_info.isp_w = cxt->commn_cxt.input_size_trim[cxt->commn_cxt.param_index].width;
		af_input.plat_info.isp_h = cxt->commn_cxt.input_size_trim[cxt->commn_cxt.param_index].height;
	}
#endif
	rtn = af_ctrl_init(&af_input, &cxt->af_cxt.handle);
	ISP_TRACE_IF_FAIL(rtn, ("fail to do af_ctrl_init"));
#if 0
exit:
	if (af_tuning) {
		free(af_tuning);
		af_tuning = NULL;
	}
#endif
	return rtn;
}

static cmr_int isp_pdaf_sw_init(struct isp_alg_fw_context *cxt, struct isp_alg_sw_init_in *input_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	struct pdaf_ctrl_init_in pdaf_input;
	struct pdaf_ctrl_init_out pdaf_output;

	memset(&pdaf_input, 0x00, sizeof(pdaf_input));
	memset(&pdaf_output, 0x00, sizeof(pdaf_output));

	pdaf_input.camera_id = cxt->camera_id;
	pdaf_input.caller_handle = (cmr_handle) cxt;;

	pdaf_input.pdaf_set_cb = isp_pdaf_set_cb;
	pdaf_input.pd_info = input_ptr->pdaf_info;

	rtn = pdaf_ctrl_init(&pdaf_input, &pdaf_output, &cxt->pdaf_cxt.handle);

exit:
	if (rtn) {
		ISP_LOGE("fail to do PDAF initialize");
	}
	ISP_LOGI("done %ld", rtn);

	return rtn;
}

static int lsc_gain_14bits_to_16bits(unsigned short *src_14bits, unsigned short *dst_16bits, unsigned int size_bytes)
{
	unsigned int gain_compressed_bits = 14;
	unsigned int gain_origin_bits = 16;
	unsigned int i = 0;
	unsigned int j = 0;
	unsigned int bit_left = 0;
	unsigned int bit_buf = 0;
	unsigned int offset = 0;
	unsigned int dst_gain_num = 0;
	unsigned int src_uncompensate_bytes = size_bytes * gain_compressed_bits % gain_origin_bits;
	unsigned int cmp_bits = size_bytes * gain_compressed_bits;
	unsigned int src_bytes = (cmp_bits + gain_origin_bits - 1) / gain_origin_bits * (gain_origin_bits / 8);

	if (0 == src_bytes || 0 != (src_bytes & 1)) {
		return 0;
	}

	for (i = 0; i < src_bytes / 2; i++) {
		bit_buf |= src_14bits[i] << bit_left;
		bit_left += 16;

		if (bit_left > gain_compressed_bits) {
			offset = 0;
			while (bit_left >= gain_compressed_bits) {
				dst_16bits[j] = (unsigned short)(bit_buf & 0x3fff);
				j++;
				bit_left -= gain_compressed_bits;
				bit_buf = (bit_buf >> gain_compressed_bits);
			}
		}
	}

	if (gain_compressed_bits == src_uncompensate_bytes) {
		dst_gain_num = j - 1;
	} else {
		dst_gain_num = j;
	}

	return dst_gain_num;
}

static uint16_t *lsc_table_wrapper(uint16_t * lsc_otp_tbl, int grid, int image_width, int image_height, int *tbl_w, int *tbl_h)
{
	int rtn = ISP_SUCCESS;
	lsc2d_calib_param_t calib_param;
	int lpf_radius = 16;
	int shading_pct = 100;
	int nx, ny, sx, sy;
	uint16_t *otp_chn[4], *tbl_chn[4];
	int w = image_width / 2;
	int h = image_height / 2;
	uint16_t *lsc_table = NULL;

	void *lsc_handle = dlopen("libsprdlsc.so", RTLD_NOW);
	if (!lsc_handle) {
		ISP_LOGE("fail to dlopen libsprdlsc lib");
		rtn = ISP_ERROR;
		return lsc_table;
	}

	struct lsc_wrapper_ops lsc_ops;

	lsc_ops.lsc2d_grid_samples = dlsym(lsc_handle, "lsc2d_grid_samples");
	if (!lsc_ops.lsc2d_grid_samples) {
		ISP_LOGE("fail to dlsym lsc2d_grid_samples");
		rtn = ISP_ERROR;
		goto error_dlsym;
	}

	lsc_ops.lsc2d_calib_param_default = dlsym(lsc_handle, "lsc2d_calib_param_default");
	if (!lsc_ops.lsc2d_calib_param_default) {
		ISP_LOGE("fail to dlsym lsc2d_calib_param_default");
		rtn = ISP_ERROR;
		goto error_dlsym;
	}

	lsc_ops.lsc2d_table_preproc = dlsym(lsc_handle, "lsc2d_table_preproc");
	if (!lsc_ops.lsc2d_table_preproc) {
		ISP_LOGE("fail to dlsym lsc2d_table_preproc");
		rtn = ISP_ERROR;
		goto error_dlsym;
	}

	lsc_ops.lsc2d_table_postproc = dlsym(lsc_handle, "lsc2d_table_postproc");
	if (!lsc_ops.lsc2d_table_postproc) {
		ISP_LOGE("fail to dlsym lsc2d_table_postproc");
		rtn = ISP_ERROR;
		goto error_dlsym;
	}

	lsc_ops.lsc2d_grid_samples(w, h, grid, grid, &nx, &ny);
	sx = nx + 2;
	sy = ny + 2;

	lsc_table = (uint16_t *) malloc(4 * sx * sy * sizeof(uint16_t));

	*tbl_w = sx;
	*tbl_h = sy;

	for (int i = 0; i < 4; i++) {
		otp_chn[i] = lsc_otp_tbl + i * nx * ny;
		tbl_chn[i] = lsc_table + i * sx * sy;
	}

	lsc_ops.lsc2d_calib_param_default(&calib_param, grid, lpf_radius, shading_pct);

	lsc_ops.lsc2d_table_preproc(otp_chn, tbl_chn, w, h, sx, sy, &calib_param);

	lsc_ops.lsc2d_table_postproc(tbl_chn, w, h, sx, sy, &calib_param);

error_dlsym:
	dlclose(lsc_handle);
	lsc_handle = NULL;


	return lsc_table;
}

static cmr_int isp_lsc_sw_init(struct isp_alg_fw_context *cxt)
{
	cmr_u32 rtn = ISP_SUCCESS;
	cmr_s32 i = 0;
	lsc_adv_handle_t lsc_adv_handle = NULL;
	struct lsc_adv_init_param lsc_param;
	isp_pm_handle_t pm_handle = cxt->handle_pm;
	uint16_t *lsc_table = NULL;

	struct isp_pm_ioctl_input io_pm_input;
	struct isp_pm_ioctl_output io_pm_output;
	struct isp_pm_param_data pm_param;
	struct isp_pm_ioctl_input get_pm_input;
	struct isp_pm_ioctl_output get_pm_output;
	struct isp_pm_ioctl_input pm_tab_input;
	struct isp_pm_ioctl_output pm_tab_output;
	struct isp_pm_param_data pm_tab_param;

	memset(&lsc_param, 0, sizeof(struct lsc_adv_init_param));
	memset(&io_pm_input, 0, sizeof(struct isp_pm_ioctl_input));
	memset(&io_pm_output, 0, sizeof(struct isp_pm_ioctl_output));
	memset(&pm_param, 0, sizeof(struct isp_pm_param_data));
	memset(&get_pm_input, 0, sizeof(struct isp_pm_ioctl_input));
	memset(&get_pm_output, 0, sizeof(struct isp_pm_ioctl_output));
	memset(&pm_tab_input, 0, sizeof(struct isp_pm_ioctl_input));
	memset(&pm_tab_output, 0, sizeof(struct isp_pm_ioctl_output));
	memset(&pm_tab_param, 0, sizeof(struct isp_pm_param_data));

	BLOCK_PARAM_CFG(pm_tab_input, pm_tab_param, ISP_PM_BLK_LSC_GET_LSCTAB, ISP_BLK_2D_LSC, NULL, 0);
	rtn = isp_pm_ioctl(pm_handle, ISP_PM_CMD_GET_SINGLE_SETTING, (void *)&pm_tab_input, (void *)&pm_tab_output);
	cxt->lsc_cxt.lsc_tab_address = pm_tab_output.param_data->data_ptr;
	struct isp_2d_lsc_param *lsc_tab_param_ptr = (struct isp_2d_lsc_param *)(cxt->lsc_cxt.lsc_tab_address);

	BLOCK_PARAM_CFG(io_pm_input, pm_param, ISP_PM_BLK_LSC_INFO, ISP_BLK_2D_LSC, PNULL, 0);
	rtn = isp_pm_ioctl(pm_handle, ISP_PM_CMD_GET_SINGLE_SETTING, (void *)&io_pm_input, (void *)&io_pm_output);
	struct isp_lsc_info *lsc_info = (struct isp_lsc_info *)io_pm_output.param_data->data_ptr;

	rtn = isp_pm_ioctl(pm_handle, ISP_PM_CMD_GET_INIT_ALSC, &get_pm_input, &get_pm_output);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to get alsc init param");
	}

	ISP_LOGV(" _alsc_init");

	if (get_pm_output.param_data->data_size != sizeof(struct lsc2_tune_param)) {
		lsc_param.tune_param_ptr = NULL;
		ISP_LOGE("fail to get alsc param from sensor file");
	} else {
		lsc_param.tune_param_ptr = get_pm_output.param_data->data_ptr;
	}

	//_alsc_set_param(&lsc_param);   // for LSC2.X neet to reopen

	//get lsc & optical center otp data
	if (cxt->otp_data != NULL) {
		int original_lens_bits = 16;
		int compressed_lens_bits = 14;
		int otp_grid = 96;
		int lsc_otp_len = cxt->otp_data->single_otp.lsc_info.lsc_data_size;
		int lsc_otp_len_chn = lsc_otp_len / 4;
		int lsc_otp_chn_gain_num = lsc_otp_len_chn * 8 / compressed_lens_bits;
		int lsc_ori_chn_len = lsc_otp_chn_gain_num * sizeof(uint16_t);
		int gain_w, gain_h;
		uint8_t *lsc_otp_addr = cxt->otp_data->single_otp.lsc_info.lsc_data_addr;

		if ((lsc_otp_addr != NULL) && (lsc_otp_len != 0)) {

			uint16_t *lsc_16_bits = (uint16_t *) malloc(lsc_ori_chn_len * 4);
			lsc_gain_14bits_to_16bits((unsigned short *)(lsc_otp_addr + lsc_otp_len_chn * 0), lsc_16_bits + lsc_otp_chn_gain_num * 0, lsc_otp_chn_gain_num);
			lsc_gain_14bits_to_16bits((unsigned short *)(lsc_otp_addr + lsc_otp_len_chn * 1), lsc_16_bits + lsc_otp_chn_gain_num * 1, lsc_otp_chn_gain_num);
			lsc_gain_14bits_to_16bits((unsigned short *)(lsc_otp_addr + lsc_otp_len_chn * 2), lsc_16_bits + lsc_otp_chn_gain_num * 2, lsc_otp_chn_gain_num);
			lsc_gain_14bits_to_16bits((unsigned short *)(lsc_otp_addr + lsc_otp_len_chn * 3), lsc_16_bits + lsc_otp_chn_gain_num * 3, lsc_otp_chn_gain_num);

			lsc_table = lsc_table_wrapper(lsc_16_bits, otp_grid, lsc_tab_param_ptr->resolution.w, lsc_tab_param_ptr->resolution.h, &gain_w, &gain_h);	//  wrapper otp table
			free(lsc_16_bits);
			if (lsc_table == NULL) {
				rtn = ISP_ERROR;
				return rtn;
			}
			lsc_param.lsc_otp_table_width = gain_w;
			lsc_param.lsc_otp_table_height = gain_h;
			lsc_param.lsc_otp_table_addr = lsc_table;
			lsc_param.lsc_otp_table_en = 1;
		}

		lsc_param.lsc_otp_oc_r_x = cxt->otp_data->single_otp.optical_center_info.R.x;
		lsc_param.lsc_otp_oc_r_y = cxt->otp_data->single_otp.optical_center_info.R.y;
		lsc_param.lsc_otp_oc_gr_x = cxt->otp_data->single_otp.optical_center_info.GR.x;
		lsc_param.lsc_otp_oc_gr_y = cxt->otp_data->single_otp.optical_center_info.GR.y;
		lsc_param.lsc_otp_oc_gb_x = cxt->otp_data->single_otp.optical_center_info.GB.x;
		lsc_param.lsc_otp_oc_gb_y = cxt->otp_data->single_otp.optical_center_info.GB.y;
		lsc_param.lsc_otp_oc_b_x = cxt->otp_data->single_otp.optical_center_info.B.x;
		lsc_param.lsc_otp_oc_b_y = cxt->otp_data->single_otp.optical_center_info.B.y;
		lsc_param.lsc_otp_oc_en = 1;
	}

	for (i = 0; i < 9; i++) {
		lsc_param.lsc_tab_address[i] = lsc_tab_param_ptr->map_tab[i].param_addr;
	}

	lsc_param.gain_width = lsc_info->gain_w;
	lsc_param.gain_height = lsc_info->gain_h;
	lsc_param.lum_gain = (cmr_u16 *) lsc_info->data_ptr;
	lsc_param.grid = lsc_info->grid;
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
	if (NULL == cxt->lsc_cxt.handle) {
		rtn = lsc_ctrl_init(&lsc_param, &lsc_adv_handle);
		if (NULL == lsc_adv_handle) {
			ISP_LOGE("fail to do lsc adv init");
			if (NULL != lsc_table)
				free(lsc_table);
			return ISP_ERROR;
		}

		cxt->lsc_cxt.handle = lsc_adv_handle;
	}
	if (NULL != lsc_table)
		free(lsc_table);

	return rtn;
}

static cmr_u32 isp_alg_sw_init(struct isp_alg_fw_context *cxt, struct isp_alg_sw_init_in *input_ptr)
{
	cmr_int rtn = ISP_SUCCESS;

	rtn = isp_afl_sw_init(cxt, input_ptr);
	ISP_RETURN_IF_FAIL(rtn, ("fail to do anti_flicker param update"));

	rtn = isp_ae_sw_init(cxt);
	ISP_TRACE_IF_FAIL(rtn, ("fail to do ae_ctrl_init"));

	rtn = isp_awb_sw_init(cxt);
	ISP_TRACE_IF_FAIL(rtn, ("fail to do awb_ctrl_init"));

	rtn = isp_smart_sw_init(cxt);
	ISP_TRACE_IF_FAIL(rtn, ("fail to do _smart_init"));

	rtn = isp_af_sw_init(cxt);
	ISP_TRACE_IF_FAIL(rtn, ("fail to do af_ctrl_init"));

	rtn = isp_pdaf_sw_init(cxt, input_ptr);
	ISP_TRACE_IF_FAIL(rtn, ("fail to do pdaf_ctrl_init"));

	rtn = isp_lsc_sw_init(cxt);
	ISP_TRACE_IF_FAIL(rtn, ("fail to do _smart_lsc_init"));

	ISP_LOGI("done %ld", rtn);
	return rtn;
}

static cmr_int isp_pm_sw_init(cmr_handle isp_alg_handle, struct isp_init_param *input_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct sensor_raw_info *sensor_raw_info_ptr = (struct sensor_raw_info *)input_ptr->setting_param_ptr;
	struct sensor_version_info *version_info = PNULL;
	struct isp_pm_init_input input;
	struct isp_otp_init_in otp_input;
	isp_ctrl_context isp_ctrl_cxt;
	cmr_u32 i = 0;

	memset(&isp_ctrl_cxt, 0, sizeof(isp_ctrl_cxt));
	cxt->sn_cxt.sn_raw_info = sensor_raw_info_ptr;
	isp_pm_raw_para_update_from_file(sensor_raw_info_ptr);
	memcpy((void *)cxt->sn_cxt.isp_init_data, (void *)input_ptr->mode_ptr, ISP_MODE_NUM_MAX * sizeof(struct isp_data_info));

	input.num = MAX_MODE_NUM;
	input.isp_ctrl_cxt_handle = &isp_ctrl_cxt;
	version_info = (struct sensor_version_info *)sensor_raw_info_ptr->version_info;
	input.sensor_name = version_info->sensor_ver_name.sensor_name;

	for (i = 0; i < MAX_MODE_NUM; i++) {
		input.tuning_data[i].data_ptr = sensor_raw_info_ptr->mode_ptr[i].addr;
		input.tuning_data[i].size = sensor_raw_info_ptr->mode_ptr[i].len;
		input.fix_data[i] = sensor_raw_info_ptr->fix_ptr[i];
	}
	input.nr_fix_info = &(sensor_raw_info_ptr->nr_fix);

	cxt->handle_pm = isp_pm_init(&input, NULL);
	cxt->commn_cxt.multi_nr_flag = isp_ctrl_cxt.multi_nr_flag;
	cxt->commn_cxt.src.w = input_ptr->size.w;
	cxt->commn_cxt.src.h = input_ptr->size.h;
	cxt->camera_id = input_ptr->camera_id;

	cxt->commn_cxt.callback = input_ptr->ctrl_callback;
	cxt->commn_cxt.caller_id = input_ptr->oem_handle;
	cxt->commn_cxt.ops = input_ptr->ops;

	/* init sensor param */
	cxt->ioctrl_ptr = sensor_raw_info_ptr->ioctrl_ptr;
	cxt->commn_cxt.image_pattern = sensor_raw_info_ptr->resolution_info_ptr->image_pattern;
	memcpy(cxt->commn_cxt.input_size_trim, sensor_raw_info_ptr->resolution_info_ptr->tab, ISP_INPUT_SIZE_NUM_MAX * sizeof(struct sensor_raw_resolution_info));
	cxt->commn_cxt.param_index = _ispGetIspParamIndex(cxt->commn_cxt.input_size_trim, &input_ptr->size);

	/*Notice: otp_init must be called before _ispAlgInit */
	otp_input.handle_pm = cxt->handle_pm;
	otp_input.lsc_golden_data = input_ptr->sensor_lsc_golden_data;
	otp_input.calibration_param = input_ptr->calibration_param;
	rtn = otp_ctrl_init(&cxt->handle_otp, &otp_input);
	ISP_TRACE_IF_FAIL(rtn, ("fail to do _otp_init"));

	return rtn;
}

static cmr_u32 isp_alg_sw_deinit(cmr_handle isp_alg_handle)
{
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;

	af_ctrl_deinit(&cxt->af_cxt.handle);

	lsc_ctrl_deinit(&cxt->lsc_cxt.handle);

	smart_ctl_deinit(&cxt->smart_cxt.handle, NULL, NULL);

	awb_ctrl_deinit(&cxt->awb_cxt.handle);

	ae_ctrl_deinit(&cxt->ae_cxt.handle);

	afl_ctrl_deinit(&cxt->afl_cxt.handle);

	pdaf_ctrl_deinit(&cxt->pdaf_cxt.handle);

	ISP_LOGI("done");
	return ISP_SUCCESS;
}

cmr_int isp_alg_fw_init(struct isp_alg_fw_init_in * input_ptr, cmr_handle * isp_alg_handle)
{
	cmr_int rtn = ISP_SUCCESS;

	if (!input_ptr || !isp_alg_handle) {
		ISP_LOGE("fail to check input param, 0x%lx", (cmr_uint) input_ptr);
		rtn = ISP_PARAM_NULL;
		goto exit;
	}

	struct isp_alg_fw_context *cxt = NULL;
	struct isp_alg_sw_init_in isp_alg_input;
	struct sensor_raw_info *sensor_raw_info_ptr = (struct sensor_raw_info *)input_ptr->init_param->setting_param_ptr;
	struct sensor_libuse_info *libuse_info = NULL;
	cmr_u32 *binning_info = NULL;
	cmr_u32 max_binning_num = ISP_BINNING_MAX_STAT_W * ISP_BINNING_MAX_STAT_H / 4;

	*isp_alg_handle = NULL;

	cxt = (struct isp_alg_fw_context *)malloc(sizeof(struct isp_alg_fw_context));
	if (!cxt) {
		ISP_LOGE("fail to malloc");
		rtn = ISP_ALLOC_ERROR;
		goto exit;
	}
	cmr_bzero(cxt, sizeof(*cxt));

	rtn = isp_pm_sw_init(cxt, input_ptr->init_param);

	cxt->dev_access_handle = input_ptr->dev_access_handle;
	isp_alg_input.lib_use_info = sensor_raw_info_ptr->libuse_info;
	isp_alg_input.size.w = input_ptr->init_param->size.w;
	isp_alg_input.size.h = input_ptr->init_param->size.h;
	cxt->lib_use_info = sensor_raw_info_ptr->libuse_info;

	cxt->otp_data = input_ptr->init_param->otp_data;
	isp_alg_input.otp_data = input_ptr->init_param->otp_data;
	isp_alg_input.pdaf_info = input_ptr->init_param->pdaf_info;

	binning_info = (cmr_u32 *) malloc(max_binning_num * 3 * sizeof(cmr_u32));
	if (!binning_info) {
		ISP_LOGE("fail to malloc binning buf");
		rtn = ISP_ALLOC_ERROR;
		goto exit;
	}
	cmr_bzero(binning_info, max_binning_num * 3 * sizeof(cmr_u32));
	cxt->binning_stats.r_info = binning_info;
	cxt->binning_stats.g_info = binning_info + max_binning_num;
	cxt->binning_stats.b_info = cxt->binning_stats.g_info + max_binning_num;

	cmr_u32 binning_hx = 4;
	cmr_u32 binning_vx = 4;
	cmr_u32 src_w = cxt->commn_cxt.src.w;
	cmr_u32 src_h = cxt->commn_cxt.src.h;
	cmr_u32 binnng_w = (src_w >> binning_hx) & ~0x1;
	cmr_u32 binnng_h = (src_h >> binning_vx) & ~0x1;
	cxt->binning_stats.binning_size.w = binnng_w / 2;
	cxt->binning_stats.binning_size.h = binnng_h / 2;
	cxt->pdaf_cxt.pdaf_support = input_ptr->init_param->ex_info.pdaf_supported;

	rtn = isp_alg_sw_init(cxt, &isp_alg_input);
	if (rtn) {
		goto exit;
	}

	rtn = isp_alg_create_thread((cmr_handle) cxt);

exit:
	if (rtn) {
		if (cxt) {
			isp_alg_destroy_thread_proc((cmr_handle) cxt);
			isp_alg_sw_deinit((cmr_handle) cxt);
			if (binning_info) {
				free((void *)binning_info);
			}
			free((void *)cxt);
		}
	} else {
		*isp_alg_handle = (cmr_handle) cxt;
		isp_dev_access_evt_reg(cxt->dev_access_handle, ispalg_dev_evt_cb, (cmr_handle) cxt);
	}

	ISP_LOGI("done %ld", rtn);
	return rtn;
}

cmr_int isp_alg_fw_deinit(cmr_handle isp_alg_handle)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	if (!cxt) {
		ISP_LOGE("fail to get cxt pointer");
		goto exit;
	}
	isp_alg_destroy_thread_proc((cmr_handle) cxt);

	rtn = isp_alg_sw_deinit((cmr_handle) cxt);
	ISP_TRACE_IF_FAIL(rtn, ("fail to do _ispAlgDeInit"));

	rtn = isp_pm_deinit(cxt->handle_pm, NULL, NULL);
	ISP_TRACE_IF_FAIL(rtn, ("fail to do isp_pm_deinit"));

	rtn = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_RESET, NULL, NULL);
	ISP_TRACE_IF_FAIL(rtn, ("fail to do isp uncfg"));

	rtn = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_STOP, NULL, NULL);
	ISP_TRACE_IF_FAIL(rtn, ("fail to do isp_dev_stop"));

	otp_ctrl_deinit(cxt->handle_otp);

	if (cxt->ae_cxt.log_alc) {
		free(cxt->ae_cxt.log_alc);
		cxt->ae_cxt.log_alc = NULL;
	}

	if (cxt->commn_cxt.log_isp) {
		free(cxt->commn_cxt.log_isp);
		cxt->commn_cxt.log_isp = NULL;
	}

	if (cxt->binning_stats.r_info) {
		free((void *)cxt->binning_stats.r_info);
	}

	if (cxt) {
		free((void *)cxt);
		cxt = NULL;
	}

exit:
	ISP_LOGI("done %d", rtn);
	return rtn;
}

static cmr_s32 isp_alg_cfg(cmr_handle isp_alg_handle)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_pm_ioctl_input input;
	struct isp_pm_ioctl_output output;
	struct isp_pm_param_data *param_data;
	cmr_u32 i = 0;

	cxt->gamma_sof_cnt = 0;
	cxt->gamma_sof_cnt_eb = 0;
	cxt->update_gamma_eb = 0;

	rtn = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_RESET, NULL, NULL);
	ISP_TRACE_IF_FAIL(rtn, ("fail to do isp_dev_reset"));

	isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_ISP_ALL_SETTING, &input, &output);
	param_data = output.param_data;
	for (i = 0; i < output.param_num; i++) {
		isp_dev_cfg_block(cxt->dev_access_handle, param_data->data_ptr, param_data->id);
		if (ISP_BLK_2D_LSC == param_data->id) {
			isp_dev_lsc_update(cxt->dev_access_handle);
		}
		param_data++;
	}

	((struct isp_anti_flicker_cfg *)cxt->afl_cxt.handle)->width = cxt->commn_cxt.src.w;
	((struct isp_anti_flicker_cfg *)cxt->afl_cxt.handle)->height = cxt->commn_cxt.src.h;
#ifdef ANTI_FLICKER_INFO_VERSION_NEW
	rtn = aflnew_ctrl_cfg(cxt->afl_cxt.handle);
#else
	rtn = afl_ctrl_cfg(cxt->afl_cxt.handle);
#endif
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to do anti_flicker param update");
		return rtn;
	}

	return rtn;
}

static cmr_int ae_set_work_mode(cmr_handle isp_alg_handle, cmr_u32 new_mode, cmr_u32 fly_mode, struct isp_video_start *param_ptr)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct ae_set_work_param ae_param;
	enum ae_work_mode ae_mode = 0;

	memset(&ae_param, 0, sizeof(ae_param));
	/*ae should known preview/capture/video work mode */
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
	ae_param.resolution_info.frame_size.w = cxt->commn_cxt.src.w;
	ae_param.resolution_info.frame_size.h = cxt->commn_cxt.src.h;
	ae_param.resolution_info.frame_line = cxt->commn_cxt.input_size_trim[cxt->commn_cxt.param_index].frame_line;
	ae_param.resolution_info.line_time = cxt->commn_cxt.input_size_trim[cxt->commn_cxt.param_index].line_time;
	ae_param.resolution_info.sensor_size_index = cxt->commn_cxt.param_index;
	ae_param.is_snapshot = param_ptr->is_snapshot;

	ae_param.sensor_fps.mode = param_ptr->sensor_fps.mode;
	ae_param.sensor_fps.max_fps = param_ptr->sensor_fps.max_fps;
	ae_param.sensor_fps.min_fps = param_ptr->sensor_fps.min_fps;
	ae_param.sensor_fps.is_high_fps = param_ptr->sensor_fps.is_high_fps;
	ae_param.sensor_fps.high_fps_skip_num = param_ptr->sensor_fps.high_fps_skip_num;
	ae_param.win_num.h = cxt->ae_cxt.win_num.h;
	ae_param.win_num.w = cxt->ae_cxt.win_num.w;

	ae_param.win_size.w = ((ae_param.resolution_info.frame_size.w / ae_param.win_num.w) / 2) * 2;
	ae_param.win_size.h = ((ae_param.resolution_info.frame_size.h / ae_param.win_num.h) / 2) * 2;
	if (ae_param.win_size.w < 120) {
		ae_param.shift = 0;
	} else {
		ae_param.shift = 1;
	}
	cxt->ae_cxt.shift = ae_param.shift;
	rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_VIDEO_START, &ae_param, NULL);
	rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_SET_DC_DV, &param_ptr->dv_mode, NULL);
	rtn = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AE_SHIFT, &ae_param.shift, NULL);

	return rtn;
}

static cmr_int isp_update_alg_param(cmr_handle isp_alg_handle)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct smart_proc_input smart_proc_in;
	struct awb_gain result;
	struct isp_pm_ioctl_input ioctl_input;
	struct isp_pm_param_data ioctl_data;
	struct isp_awbc_cfg awbc_cfg;
	cmr_u32 ct = 0;
	cmr_s32 bv = 0;
	cmr_s32 bv_gain = 0;

	/*update aem information */
	cxt->aem_is_update = 0;
//      memset((void*)&cxt->aem_stats, 0, sizeof(cxt->aem_stats));

	/*update awb gain */
	rtn = awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_GET_GAIN, (void *)&result, NULL);
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
	rtn = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_AWB, (void *)&ioctl_input, NULL);

	/*update smart param */
	rtn = awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_GET_CT, (void *)&ct, NULL);
	rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_GET_BV_BY_LUM_NEW, NULL, (void *)&bv);
	rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_GET_BV_BY_GAIN, NULL, (void *)&bv_gain);
	rtn = smart_ctl_ioctl(cxt->smart_cxt.handle, ISP_SMART_IOCTL_SET_WORK_MODE, (void *)&cxt->commn_cxt.isp_mode, NULL);
	memset(&smart_proc_in, 0, sizeof(smart_proc_in));
	if ((0 != bv_gain) && (0 != ct)) {

		struct alsc_ver_info lsc_ver = { 0 };
		rtn = lsc_ctrl_ioctrl(cxt->lsc_cxt.handle, ALSC_GET_VER, NULL, (void *)&lsc_ver);
		if (ISP_SUCCESS != rtn) {
			ISP_LOGE("fail to Get ALSC ver info!");
		}

		smart_proc_in.cal_para.bv = bv;
		smart_proc_in.cal_para.bv_gain = bv_gain;
		smart_proc_in.cal_para.ct = ct;
		smart_proc_in.alc_awb = cxt->awb_cxt.alc_awb;
		smart_proc_in.handle_pm = cxt->handle_pm;
		smart_proc_in.mode_flag = cxt->commn_cxt.mode_flag;
		smart_proc_in.scene_flag = cxt->commn_cxt.scene_flag;
		smart_proc_in.LSC_SPD_VERSION = lsc_ver.LSC_SPD_VERSION;
		rtn = _smart_calc(cxt->smart_cxt.handle, &smart_proc_in);
	}
//#ifdef LSC_ADV_ENABLE
#if  0
	cmr_handle lsc_adv_handle = cxt->lsc_cxt.handle;
	struct isp_pm_ioctl_input input = { PNULL, 0 };
	struct isp_pm_ioctl_output output = { PNULL, 0 };
	struct isp_pm_param_data param_data;
	struct lsc_adv_calc_param calc_param;
	struct lsc_adv_calc_result calc_result = { 0 };
	cmr_u32 i = 0;
	memset(&param_data, 0, sizeof(param_data));
	memset(&calc_param, 0, sizeof(calc_param));

	BLOCK_PARAM_CFG(input, param_data, ISP_PM_BLK_LSC_INFO, ISP_BLK_2D_LSC, PNULL, 0);
	rtn = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_SINGLE_SETTING, (void *)&input, (void *)&output);
	struct isp_lsc_info *lsc_info = (struct isp_lsc_info *)output.param_data->data_ptr;

	struct isp_2d_lsc_param *lsc_tab_pram_ptr = (struct isp_2d_lsc_param *)(cxt->lsc_cxt.lsc_tab_address);

	calc_result.dst_gain = (cmr_u16 *) lsc_info->data_ptr;
	struct awb_size stat_img_size;
	struct awb_size win_size;
	struct isp_ae_grgb_statistic_info *stat_info;
	BLOCK_PARAM_CFG(input, param_data, ISP_PM_BLK_AEM_STATISTIC, ISP_BLK_AE_NEW, NULL, 0);
	isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_SINGLE_SETTING, (void *)&input, (void *)&output);
	stat_info = output.param_data->data_ptr;

	awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_GET_STAT_SIZE, (void *)&stat_img_size, NULL);
	awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_GET_WIN_SIZE, (void *)&win_size, NULL);

	ISP_LOGV("0x%x, 0x%x, 0x%x", stat_info->r_info, stat_info->g_info, stat_info->b_info);
//      calc_param.stat_img.r  = stat_info->r_info;
//      calc_param.stat_img.gr = stat_info->g_info;
//      calc_param.stat_img.gb = stat_info->g_info;
//      calc_param.stat_img.b  = stat_info->b_info;
	calc_param.stat_img.r = cxt->aem_stats.r_info;
	calc_param.stat_img.gr = cxt->aem_stats.g_info;
	calc_param.stat_img.gb = cxt->aem_stats.g_info;
	calc_param.stat_img.b = cxt->aem_stats.b_info;
	calc_param.stat_size.w = stat_img_size.w;
	calc_param.stat_size.h = stat_img_size.h;
	calc_param.gain_width = lsc_info->gain_w;
	calc_param.gain_height = lsc_info->gain_h;
	calc_param.lum_gain = (cmr_u16 *) lsc_info->param_ptr;
	calc_param.block_size.w = win_size.w;
	calc_param.block_size.h = win_size.h;
	calc_param.ct = ct;
	calc_param.bv = isp_cur_bv;
	calc_param.isp_id = ISP_2_0;

	calc_param.r_gain = gAWBGainR;
	calc_param.b_gain = gAWBGainB;
	calc_param.img_size.w = cxt->commn_cxt.src.w;
	calc_param.img_size.h = cxt->commn_cxt.src.h;

	if (lsc_tab_pram_ptr == 0)
		return 0;
	else {
		for (i = 0; i < 9; i++) {
			calc_param.lsc_tab_address[i] = lsc_tab_pram_ptr->map_tab[i].param_addr;
		}
	}

	lsc_ctrl_process(lsc_adv_handle, &calc_param, &calc_result);

	BLOCK_PARAM_CFG(input, param_data, ISP_PM_BLK_LSC_INFO, ISP_BLK_2D_LSC, PNULL, 0);
	input.param_data_ptr = &param_data;

	rtn = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_OTHERS, &input, NULL);

#endif

	return rtn;
}

static cmr_int isp_update_alsc_param(cmr_handle isp_alg_handle)
{
#ifdef LSC_ADV_ENABLE
    cmr_int rtn = ISP_SUCCESS;
         struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
         cmr_handle lsc_adv_handle = cxt->lsc_cxt.handle;
         struct isp_pm_ioctl_input input = { PNULL, 0 };
         struct isp_pm_ioctl_output output = { PNULL, 0 };
         struct isp_pm_param_data param_data;
         struct lsc_adv_calc_param calc_param;
         struct lsc_adv_calc_result calc_result = { 0 };
         cmr_u32 i = 0;
         memset(&param_data, 0, sizeof(param_data));
         memset(&calc_param, 0, sizeof(calc_param));
 
         BLOCK_PARAM_CFG(input, param_data, ISP_PM_BLK_LSC_INFO, ISP_BLK_2D_LSC, PNULL, 0);
         rtn = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_SINGLE_SETTING, (void *)&input, (void *)&output);
         struct isp_lsc_info *lsc_info = (struct isp_lsc_info *)output.param_data->data_ptr;
 
         struct isp_2d_lsc_param *lsc_tab_pram_ptr = (struct isp_2d_lsc_param *)(cxt->lsc_cxt.lsc_tab_address);
 
         calc_result.dst_gain = (cmr_u16 *) lsc_info->data_ptr;
         struct awb_size stat_img_size;
         struct awb_size win_size;
         struct isp_ae_grgb_statistic_info *stat_info;
         BLOCK_PARAM_CFG(input, param_data, ISP_PM_BLK_AEM_STATISTIC, ISP_BLK_AE_NEW, NULL, 0);
         isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_SINGLE_SETTING, (void *)&input, (void *)&output);
         stat_info = output.param_data->data_ptr;
 
         awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_GET_STAT_SIZE, (void *)&stat_img_size, NULL);
         awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_GET_WIN_SIZE, (void *)&win_size, NULL);
 
         calc_param.stat_img.r = cxt->aem_stats.r_info;
         calc_param.stat_img.gr = cxt->aem_stats.g_info;
         calc_param.stat_img.gb = cxt->aem_stats.g_info;
         calc_param.stat_img.b = cxt->aem_stats.b_info;
         calc_param.stat_size.w = stat_img_size.w;
         calc_param.stat_size.h = stat_img_size.h;
         calc_param.gain_width = lsc_info->gain_w;
         calc_param.gain_height = lsc_info->gain_h;
         calc_param.lum_gain = (cmr_u16 *) lsc_info->param_ptr;
         calc_param.block_size.w = win_size.w;
         calc_param.block_size.h = win_size.h;
         calc_param.bv = isp_cur_bv;
         calc_param.isp_id = ISP_2_0;
 
         calc_param.r_gain = gAWBGainR;
         calc_param.b_gain = gAWBGainB;
         calc_param.img_size.w = cxt->commn_cxt.src.w;
         calc_param.img_size.h = cxt->commn_cxt.src.h;
 
         if (lsc_tab_pram_ptr == 0)
                  return 0;
         else {
                   for (i = 0; i < 9; i++) {
                            calc_param.lsc_tab_address[i] = lsc_tab_pram_ptr->map_tab[i].param_addr;
                   }
         }
 
         lsc_ctrl_process(lsc_adv_handle, &calc_param, &calc_result);
 		 BLOCK_PARAM_CFG(input, param_data, ISP_PM_BLK_LSC_INFO, ISP_BLK_2D_LSC, PNULL, 0);
         input.param_data_ptr = &param_data;
 
         rtn = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_OTHERS, &input, NULL);
 
#endif
 
         return rtn;
 
}

cmr_int isp_alg_fw_start(cmr_handle isp_alg_handle, struct isp_video_start * in_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_interface_param_v1 *interface_ptr_v1 = &cxt->commn_cxt.interface_param_v1;
	struct isp_statis_mem_info statis_mem_input;
	struct isp_size org_size;
	cmr_s32 mode = 0, dv_mode = 0;

	if (!isp_alg_handle || !in_ptr) {
		rtn = ISP_PARAM_ERROR;
		goto exit;
	}

	cxt->sensor_fps.mode = in_ptr->sensor_fps.mode;
	cxt->sensor_fps.max_fps = in_ptr->sensor_fps.max_fps;
	cxt->sensor_fps.min_fps = in_ptr->sensor_fps.min_fps;
	cxt->sensor_fps.is_high_fps = in_ptr->sensor_fps.is_high_fps;
	cxt->sensor_fps.high_fps_skip_num = in_ptr->sensor_fps.high_fps_skip_num;

	org_size.w = cxt->commn_cxt.src.w;
	org_size.h = cxt->commn_cxt.src.h;
	cxt->commn_cxt.src.w = in_ptr->size.w;
	cxt->commn_cxt.src.h = in_ptr->size.h;

	memset(&statis_mem_input, 0, sizeof(struct isp_statis_mem_info));
	statis_mem_input.buffer_client_data = in_ptr->buffer_client_data;
	statis_mem_input.cb_of_malloc = in_ptr->cb_of_malloc;
	statis_mem_input.cb_of_free = in_ptr->cb_of_free;
	statis_mem_input.isp_lsc_physaddr = in_ptr->lsc_phys_addr;
	statis_mem_input.isp_lsc_virtaddr = in_ptr->lsc_virt_addr;
	statis_mem_input.lsc_mfd = in_ptr->lsc_mfd;

	rtn = isp_dev_statis_buf_malloc(cxt->dev_access_handle, &statis_mem_input);
	interface_ptr_v1->data.work_mode = ISP_CONTINUE_MODE;
	interface_ptr_v1->data.input = ISP_CAP_MODE;
	interface_ptr_v1->data.input_format = in_ptr->format;
	interface_ptr_v1->data.format_pattern = cxt->commn_cxt.image_pattern;
	interface_ptr_v1->data.input_size.w = in_ptr->size.w;
	interface_ptr_v1->data.input_size.h = in_ptr->size.h;
	interface_ptr_v1->data.output_format = ISP_DATA_UYVY;
	interface_ptr_v1->data.output = ISP_DCAM_MODE;
	interface_ptr_v1->data.slice_height = in_ptr->size.h;

	rtn = isp_dev_set_interface(interface_ptr_v1);
	ISP_RETURN_IF_FAIL(rtn, ("fail to set param"));

	switch (in_ptr->work_mode) {
	case 0:		/*preview */
		rtn = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_MODEID_BY_RESOLUTION, in_ptr, &mode);
		break;
	case 1:		/*capture */
		rtn = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_MODEID_BY_RESOLUTION, in_ptr, &mode);
		break;
	case 2:
		mode = ISP_MODE_ID_VIDEO_0;
		break;
	default:
		mode = ISP_MODE_ID_PRV_0;
		break;
	}
	if (SENSOR_MULTI_MODE_FLAG != cxt->commn_cxt.multi_nr_flag) {
		if ((mode != cxt->commn_cxt.isp_mode) && (org_size.w != cxt->commn_cxt.src.w)) {
			cxt->commn_cxt.isp_mode = mode;
			rtn = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_MODE, &cxt->commn_cxt.isp_mode, NULL);
		}
	} else {

		if (0 != in_ptr->dv_mode) {
			rtn = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_DV_MODEID_BY_RESOLUTION, in_ptr, &dv_mode);
			cxt->commn_cxt.mode_flag = dv_mode;
		} else {
			cxt->commn_cxt.mode_flag = mode;
		}
		if (cxt->commn_cxt.mode_flag != (cmr_u32) cxt->commn_cxt.isp_mode) {
			cxt->commn_cxt.isp_mode = cxt->commn_cxt.mode_flag;
			rtn = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_MODE, &cxt->commn_cxt.isp_mode, NULL);
		}
	}

	/* isp param index */
	cxt->commn_cxt.param_index = _ispGetIspParamIndex(cxt->commn_cxt.input_size_trim, &in_ptr->size);
	/* todo: base on param_index to get sensor line_time/frame_line */

	rtn = isp_update_alg_param(cxt);
	ISP_RETURN_IF_FAIL(rtn, ("fail to isp smart param calc"));

	/*TBD pdaf_support will get form sensor,pdaf_en will get from oem*/
	ISP_LOGI("cxt->pdaf_cxt.pdaf_support = %d, cxt->pdaf_cxt.pdaf_en = %d",
		cxt->pdaf_cxt.pdaf_support, cxt->pdaf_cxt.pdaf_en);
	if (cxt->pdaf_cxt.pdaf_support && cxt->pdaf_cxt.pdaf_en) {
		rtn = pdaf_ctrl_ioctrl(cxt->pdaf_cxt.handle, PDAF_CTRL_CMD_SET_PARAM, NULL, NULL);

	}
	ISP_RETURN_IF_FAIL(rtn, ("fail to cfg pdaf param"));

	rtn = isp_dev_trans_addr(cxt->dev_access_handle);
	ISP_RETURN_IF_FAIL(rtn, ("fail to trans isp buff"));

	rtn = isp_alg_cfg(cxt);
	ISP_RETURN_IF_FAIL(rtn, ("fail to do isp cfg"));
	rtn = isp_update_alsc_param(cxt);
	rtn = awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_SET_WORK_MODE, &in_ptr->work_mode, NULL);
	rtn = awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_GET_PIX_CNT, &in_ptr->size, NULL);
	ISP_RETURN_IF_FAIL(rtn, ("fail to set_awb_work_mode"));

	rtn = ae_set_work_mode(cxt, mode, 1, in_ptr);
	ISP_RETURN_IF_FAIL(rtn, ("fail to do ae cfg"));

	rtn = isp_dev_start(cxt->dev_access_handle, interface_ptr_v1);
	ISP_RETURN_IF_FAIL(rtn, ("fail to do video isp start"));
	cxt->gamma_sof_cnt_eb = 1;

	if (cxt->af_cxt.handle && ((ISP_VIDEO_MODE_CONTINUE == in_ptr->mode))) {
		rtn = af_ctrl_ioctrl(cxt->af_cxt.handle, AF_CMD_SET_ISP_START_INFO, in_ptr, NULL);
	}
exit:
	ISP_LOGV("done %ld", rtn);
	return rtn;
}

cmr_int isp_alg_fw_stop(cmr_handle isp_alg_handle)
{
	cmr_int rtn = ISP_SUCCESS;

	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_VIDEO_STOP, NULL, NULL);
	rtn = awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_VIDEO_STOP_NOTIFY, NULL, NULL);
	ISP_RETURN_IF_FAIL(rtn, ("fail to do isp cfg"));

exit:
	ISP_LOGV("done %ld", rtn);
	return rtn;
}

cmr_int isp_slice_raw_proc(struct isp_alg_fw_context * cxt, struct ips_in_param * in_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_raw_proc_info slice_raw_info;;
	struct isp_dev_access_context *isp_dev = NULL;
	struct isp_file *file = NULL;

	if (!cxt || !in_ptr) {
		rtn = ISP_PARAM_ERROR;
		goto exit;
	}

	isp_dev = (struct isp_dev_access_context *)cxt->dev_access_handle;
	file = (struct isp_file *)(isp_dev->isp_driver_handle);
	memset((void *)&slice_raw_info, 0x0, sizeof(struct isp_raw_proc_info));

	slice_raw_info.in_size.width = in_ptr->src_frame.img_size.w;
	slice_raw_info.in_size.height = in_ptr->src_frame.img_size.h;
	slice_raw_info.out_size.width = in_ptr->dst_frame.img_size.w;
	slice_raw_info.out_size.height = in_ptr->dst_frame.img_size.h;
	slice_raw_info.img_fd = in_ptr->dst_frame.img_fd.y;
	slice_raw_info.img_vir.chn0 = in_ptr->dst_frame.img_addr_vir.chn0;
	slice_raw_info.img_vir.chn1 = in_ptr->dst_frame.img_addr_vir.chn1;
	slice_raw_info.img_vir.chn2 = in_ptr->dst_frame.img_addr_vir.chn2;
	slice_raw_info.img_offset.chn0 = in_ptr->dst_frame.img_addr_phy.chn0;
	slice_raw_info.img_offset.chn1 = in_ptr->dst_frame.img_addr_phy.chn1;
	slice_raw_info.img_offset.chn2 = in_ptr->dst_frame.img_addr_phy.chn2;

	rtn = ioctl(file->fd, SPRD_ISP_IO_RAW_CAP, &slice_raw_info);
	ISP_RETURN_IF_FAIL(rtn, ("fail to do isp raw cap ioctl"));

	isp_u_fetch_start_isp(isp_dev->isp_driver_handle, ISP_ONE);

exit:
	return rtn;
}

cmr_int isp_alg_proc_start(cmr_handle isp_alg_handle, struct ips_in_param * in_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_interface_param_v1 *interface_ptr_v1 = &cxt->commn_cxt.interface_param_v1;
	struct isp_size org_size;
	cmr_s32 mode = 0;

	ISP_LOGV("isp proc start\n");
	org_size.w = cxt->commn_cxt.src.w;
	org_size.h = cxt->commn_cxt.src.h;

	cxt->commn_cxt.src.w = in_ptr->src_frame.img_size.w;
	cxt->commn_cxt.src.h = in_ptr->src_frame.img_size.h;

	interface_ptr_v1->data.work_mode = ISP_SINGLE_MODE;
	interface_ptr_v1->data.input = ISP_EMC_MODE;
	interface_ptr_v1->data.input_format = in_ptr->src_frame.img_fmt;

	if (INVALID_FORMAT_PATTERN == in_ptr->src_frame.format_pattern) {
		interface_ptr_v1->data.format_pattern = cxt->commn_cxt.image_pattern;
	} else {
		interface_ptr_v1->data.format_pattern = cxt->commn_cxt.image_pattern;;
	}
	interface_ptr_v1->data.input_size.w = in_ptr->src_frame.img_size.w;
	interface_ptr_v1->data.input_size.h = in_ptr->src_frame.img_size.h;
	interface_ptr_v1->data.input_addr.chn0 = in_ptr->src_frame.img_addr_phy.chn0;
	interface_ptr_v1->data.input_addr.chn1 = in_ptr->src_frame.img_addr_phy.chn1;
	interface_ptr_v1->data.input_addr.chn2 = in_ptr->src_frame.img_addr_phy.chn2;
	interface_ptr_v1->data.input_vir.chn0 = in_ptr->src_frame.img_addr_vir.chn0;
	interface_ptr_v1->data.input_vir.chn1 = in_ptr->src_frame.img_addr_vir.chn1;
	interface_ptr_v1->data.input_vir.chn2 = in_ptr->src_frame.img_addr_vir.chn2;
	interface_ptr_v1->data.input_fd = in_ptr->src_frame.img_fd.y;
	interface_ptr_v1->data.slice_height = in_ptr->src_frame.img_size.h;

	interface_ptr_v1->data.output_format = in_ptr->dst_frame.img_fmt;
	interface_ptr_v1->data.output = ISP_EMC_MODE;
	interface_ptr_v1->data.output_addr.chn0 = in_ptr->dst_frame.img_addr_phy.chn0;
	interface_ptr_v1->data.output_addr.chn1 = in_ptr->dst_frame.img_addr_phy.chn1;
	interface_ptr_v1->data.output_addr.chn2 = in_ptr->dst_frame.img_addr_phy.chn2;

	rtn = isp_dev_set_interface(interface_ptr_v1);
	ISP_RETURN_IF_FAIL(rtn, ("fail to set param"));

	rtn = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_MODEID_BY_RESOLUTION, in_ptr, &mode);
	ISP_RETURN_IF_FAIL(rtn, ("fail to get isp_mode"));

	if (org_size.w != cxt->commn_cxt.src.w) {
		cxt->commn_cxt.isp_mode = mode;
		cxt->commn_cxt.mode_flag = mode;
		rtn = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_MODE, &cxt->commn_cxt.isp_mode, NULL);
		ISP_RETURN_IF_FAIL(rtn, ("isp_pm_ioctl fail "));
	}

	/* isp param index */
	cxt->commn_cxt.param_index = _ispGetIspParamIndex(cxt->commn_cxt.input_size_trim, &in_ptr->src_frame.img_size);
	/* todo: base on param_index to get sensor line_time/frame_line */

	rtn = isp_dev_trans_addr(cxt->dev_access_handle);
	ISP_RETURN_IF_FAIL(rtn, ("fail to trans isp buff"));

	rtn = isp_alg_cfg(cxt);
	ISP_RETURN_IF_FAIL(rtn, ("fail to do isp cfg"));

	rtn = isp_dev_start(cxt->dev_access_handle, interface_ptr_v1);
	ISP_RETURN_IF_FAIL(rtn, ("fail to video isp start"));
	cxt->gamma_sof_cnt_eb = 1;

	ISP_LOGV("isp start raw proc\n");
	rtn = isp_slice_raw_proc(cxt, in_ptr);
	ISP_RETURN_IF_FAIL(rtn, ("fail to isp_slice_raw_proc"));

exit:
	ISP_LOGV("done %ld", rtn);
	return rtn;
}

cmr_int isp_alg_proc_next(cmr_handle isp_alg_handle, struct ipn_in_param * in_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	UNUSED(in_ptr);
	/*do not support silce capture function now */
	ISP_LOGV("If need slice capture process, add releated code!");
exit:
	ISP_LOGV("done %ld", rtn);
	return rtn;
}

cmr_int isp_alg_fw_ioctl(cmr_handle isp_alg_handle, enum isp_ctrl_cmd io_cmd, void *param_ptr, cmr_s32(*call_back) ())
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	enum isp_ctrl_cmd cmd = io_cmd & 0x7fffffff;
	io_fun io_ctrl = NULL;

	cxt->commn_cxt.isp_callback_bypass = io_cmd & 0x80000000;
	io_ctrl = _ispGetIOCtrlFun(cmd);
	if (NULL != io_ctrl) {
		rtn = io_ctrl(cxt, param_ptr, call_back);
	} else {
		ISP_LOGV("io_ctrl fun is null, cmd %d", cmd);
	}

	if (NULL != cxt->commn_cxt.callback) {
		cxt->commn_cxt.callback(cxt->commn_cxt.caller_id, ISP_CALLBACK_EVT | ISP_CTRL_CALLBACK | cmd, NULL, ISP_ZERO);
	}

	return rtn;
}

cmr_int isp_alg_fw_capability(cmr_handle isp_alg_handle, enum isp_capbility_cmd cmd, void *param_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;

	switch (cmd) {
	case ISP_LOW_LUX_EB:{
			cmr_u32 out_param = 0;
			rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_GET_FLASH_EB, NULL, &out_param);
			*((cmr_u32 *) param_ptr) = out_param;
			break;
		}
	case ISP_CUR_ISO:{
			cmr_u32 out_param = 0;
			rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_GET_ISO, NULL, &out_param);
			*((cmr_u32 *) param_ptr) = out_param;
			break;
		}
	case ISP_CTRL_GET_AE_LUM:{
			cmr_u32 out_param = 0;
			rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_GET_BV_BY_LUM_NEW, NULL, &out_param);
			*((cmr_u32 *) param_ptr) = out_param;
			break;
		}
	default:
		break;
	}

	ISP_LOGV("done %ld", rtn);
	return rtn;
}
