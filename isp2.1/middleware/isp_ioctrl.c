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

#define LOG_TAG "isp_ioctrl"

#include "lib_ctrl.h"
#include "isp_ioctrl.h"
#include "smart_ctrl.h"
#include "isp_drv.h"
#include "lsc_adv.h"
#include "ae_ctrl.h"
#include "awb_ctrl.h"
#include "af_ctrl.h"
#include "isp_alg_fw.h"
#include "isp_dev_access.h"
#include "isp_debug.h"
//#include "sensor_isp_param_merge.h"

#define ISP_REG_NUM                          20467
#define SEPARATE_GAMMA_IN_VIDEO
#define VIDEO_GAMMA_INDEX                    (8)

static int32_t _isp_set_awb_gain(cmr_handle isp_alg_handle)
{
	int32_t                          rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_awbc_cfg             awbc_cfg = {0,0,0,0,0,0};
	struct awb_gain                 result = {0,0,0};
	struct isp_pm_ioctl_input       ioctl_input = {NULL,0};
	struct isp_pm_param_data        ioctl_data = {0,0,0,NULL,0,{0}};

	rtn = awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_GET_GAIN, (void*)&result, NULL);
	ISP_TRACE_IF_FAIL(rtn, ("awb get gain error"));

	/*set awb gain*/
	awbc_cfg.r_gain            = result.r;
	awbc_cfg.g_gain            = result.g;
	awbc_cfg.b_gain            = result.b;
	awbc_cfg.r_offset          = 0;
	awbc_cfg.g_offset          = 0;
	awbc_cfg.b_offset          = 0;

	ioctl_data.id              = ISP_BLK_AWB_NEW;
	ioctl_data.cmd             = ISP_PM_BLK_AWBC;
	ioctl_data.data_ptr        = &awbc_cfg;
	ioctl_data.data_size       = sizeof(awbc_cfg);

	ioctl_input.param_data_ptr = &ioctl_data;
	ioctl_input.param_num      = 1;

	rtn = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_AWB, (void *)&ioctl_input, NULL);
	ISP_LOGI("set AWB_TAG: isp_pm_ioctl rtn=%d, gain=(%d, %d, %d)", rtn, awbc_cfg.r_gain, awbc_cfg.g_gain, awbc_cfg.b_gain);

	return rtn;
}

static int32_t _isp_set_awb_flash_gain(cmr_handle isp_alg_handle)
{
	int32_t                          rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_flash_param           *flash = NULL;
	struct awb_flash_info            flash_awb;
	uint32_t                         ae_effect = 0;

	memset((void*)&flash_awb, 0, sizeof(struct awb_flash_info));

	rtn = _isp_get_flash_cali_param(cxt->handle_pm, &flash);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("get flash cali parm failed");
		return rtn;
	}
	ISP_LOGI("flash param rgb ratio = (%d,%d,%d), lum_ratio = %d",
			flash->cur.r_ratio, flash->cur.g_ratio, flash->cur.b_ratio, flash->cur.lum_ratio);

	rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_GET_FLASH_EFFECT, NULL, &ae_effect);
	ISP_TRACE_IF_FAIL(rtn, ("ae get flash effect error"));

	flash_awb.effect        = ae_effect;
	flash_awb.flash_ratio.r = flash->cur.r_ratio;
	flash_awb.flash_ratio.g = flash->cur.g_ratio;
	flash_awb.flash_ratio.b = flash->cur.b_ratio;

	ISP_LOGI("FLASH_TAG get effect from ae effect = %d", ae_effect);

	rtn = awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_FLASHING, (void*)&flash_awb, NULL);
	ISP_TRACE_IF_FAIL(rtn, ("awb set flash gain error"));
	rtn = _isp_set_awb_gain(cxt);
	ISP_TRACE_IF_FAIL(rtn, ("awb set gain error"));

	return rtn;
}

static int32_t _smart_param_update(cmr_handle isp_alg_handle)
{
	int32_t                         rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	uint32_t                        i = 0;
	struct smart_init_param         smart_init_param;
	struct isp_pm_ioctl_input       pm_input = {NULL,0};
	struct isp_pm_ioctl_output      pm_output = {NULL,0};

	memset(&smart_init_param, 0, sizeof(smart_init_param));

	rtn = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_INIT_SMART, &pm_input, &pm_output);
	if ((ISP_SUCCESS == rtn) && pm_output.param_data) {
		for (i = 0; i < pm_output.param_num; ++i){
			smart_init_param.tuning_param[i].data.size = pm_output.param_data[i].data_size;
			smart_init_param.tuning_param[i].data.data_ptr = pm_output.param_data[i].data_ptr;
		}
	} else {
		ISP_LOGE("get smart init param failed");
		return rtn;
	}

	rtn = smart_ctl_ioctl(cxt->smart_cxt.handle, ISP_SMART_IOCTL_GET_UPDATE_PARAM, (void *)&smart_init_param, NULL);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("smart param reinit failed");
		return rtn;
	}

	ISP_LOGI("ISP_SMART: handle=%p, param=%p", cxt->smart_cxt.handle,
			pm_output.param_data->data_ptr);

	return rtn;
}

/**---------------------------------------------------------------------------*
**					IOCtrl Function Prototypes			*
**---------------------------------------------------------------------------*/
static cmr_int _ispAwbModeIOCtrl(cmr_handle isp_alg_handle, void *param_ptr, int (*call_back)())
{
	cmr_int                         rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_pm_ioctl_input       ioctl_input = {NULL,0};
	struct isp_pm_param_data        ioctl_data;
	struct isp_awbc_cfg             awbc_cfg;
	struct awb_gain                 result;
	uint32_t                        awb_mode = *(uint32_t*)param_ptr;
	uint32_t                        awb_id;

	memset((void*)&ioctl_data,0,sizeof(struct isp_pm_param_data));
	memset((void*)&awbc_cfg,0,sizeof(struct isp_awbc_cfg));
	memset((void*)&result,0,sizeof(struct awb_gain));
	UNUSED(call_back);

	switch (awb_mode) {
	case ISP_AWB_AUTO:
		awb_id = AWB_CTRL_WB_MODE_AUTO;
		break;
	case ISP_AWB_INDEX1:
		awb_id = AWB_CTRL_MWB_MODE_INCANDESCENT;
		break;
	case ISP_AWB_INDEX4:
		awb_id = AWB_CTRL_MWB_MODE_FLUORESCENT;
		break;
	case ISP_AWB_INDEX5:
		awb_id = AWB_CTRL_MWB_MODE_SUNNY;
		break;
	case ISP_AWB_INDEX6:
		awb_id = AWB_CTRL_MWB_MODE_CLOUDY;
		break;
	default:
		break;
	}

	ISP_LOGI("--IOCtrl--AWB_MODE--:0x%x", awb_id);
	rtn = awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_SET_WB_MODE, (void*)&awb_id, NULL);
	ISP_TRACE_IF_FAIL(rtn, ("awb set wb mode error"));

	rtn = awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_GET_GAIN, (void*)&result, NULL);
	ISP_TRACE_IF_FAIL(rtn, ("awb get gain error"));

	/*set awb gain*/
	awbc_cfg.r_gain            = result.r;
	awbc_cfg.g_gain            = result.g;
	awbc_cfg.b_gain            = result.b;
	awbc_cfg.r_offset          = 0;
	awbc_cfg.g_offset          = 0;
	awbc_cfg.b_offset          = 0;

	ioctl_data.id              = ISP_BLK_AWB_NEW;
	ioctl_data.cmd             = ISP_PM_BLK_AWBC;
	ioctl_data.data_ptr        = &awbc_cfg;
	ioctl_data.data_size       = sizeof(awbc_cfg);

	ioctl_input.param_data_ptr = &ioctl_data;
	ioctl_input.param_num      = 1;

	rtn = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_AWB, (void *)&ioctl_input, NULL);
	ISP_LOGI("AWB_TAG: isp_pm_ioctl rtn=%ld, gain=(%d, %d, %d)", rtn, awbc_cfg.r_gain, awbc_cfg.g_gain, awbc_cfg.b_gain);

	return rtn;
}

static cmr_int _ispAeAwbBypassIOCtrl(cmr_handle isp_alg_handle, void *param_ptr, int (*call_back)())
{
	cmr_int                         rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	uint32_t                        type = 0;
	uint32_t                        bypass = 0;
	UNUSED(call_back);

	if (NULL == param_ptr) {
		return ISP_PARAM_NULL;
	}

	type = *(uint32_t*)param_ptr;
	switch (type) {
	case 0: /*ae awb normal*/
		bypass = 0;
		rtn = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_RAW_AEM_BYPASS, &bypass, NULL);
		rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_SET_BYPASS, &bypass, NULL);
		break;
	case 1:
		break;
	case 2: /*ae by pass*/
		bypass = 1;
		rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_SET_BYPASS, &bypass, NULL);
		break;
	case 3: /*awb by pass*/
		bypass = 1;
		rtn = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_RAW_AEM_BYPASS, &bypass, NULL);
		break;
	default:
		break;
	}

	ISP_LOGI("type=%d", type);
	return rtn;
}

static cmr_int _ispEVIOCtrl(cmr_handle isp_alg_handle, void *param_ptr, int (*call_back)())
{
	cmr_int                         rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct ae_set_ev set_ev = {0};
	UNUSED(call_back);

	if (NULL == param_ptr) {
		return ISP_PARAM_NULL;
	}

	set_ev.level = *(uint32_t*)param_ptr;
	rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_SET_EV_OFFSET, &set_ev, NULL);

	ISP_LOGI("ISP_AE: AE_SET_EV_OFFSET=%d, rtn=%ld", set_ev.level, rtn);

	return rtn;
}

static cmr_int _ispFlickerIOCtrl(cmr_handle isp_alg_handle, void *param_ptr, int (*call_back)())
{
	cmr_int                         rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct ae_set_flicker           set_flicker = {0};
	cmr_int bypass = 0;
	UNUSED(call_back);

	if (NULL == param_ptr) {
		return ISP_PARAM_NULL;
	}

	cxt->afl_cxt.afl_mode = *(uint32_t*)param_ptr;
	set_flicker.mode = *(uint32_t*)param_ptr;
	rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_SET_FLICKER, &set_flicker, NULL);
	ISP_LOGI("ISP_AE: AE_SET_FLICKER=%d, rtn=%ld", set_flicker.mode, rtn);

	if (set_flicker.mode == AE_FLICKER_AUTO) {
		bypass = 0;
		isp_dev_anti_flicker_bypass(cxt->dev_access_handle, bypass);
	}

	return rtn;
}

static cmr_int _ispSnapshotNoticeIOCtrl(cmr_handle isp_alg_handle, void *param_ptr, int (*call_back)())
{
	cmr_int                         rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_snapshot_notice      *isp_notice = param_ptr;
	struct ae_snapshot_notice       ae_notice;
	UNUSED(call_back);

	if (NULL == cxt || NULL == isp_notice) {
		ISP_LOGE("handle %p isp_notice %p is NULL error", cxt, isp_notice);
		return ISP_PARAM_NULL;
	}

	ae_notice.type = isp_notice->type;
	ae_notice.preview_line_time = isp_notice->preview_line_time;
	ae_notice.capture_line_time = isp_notice->capture_line_time;
	rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_SET_SNAPSHOT_NOTICE, &ae_notice, NULL);

	ISP_LOGI("LiuY: %ld", rtn);
	return rtn;
}

static cmr_int _ispFlashNoticeIOCtrl(cmr_handle isp_alg_handle, void *param_ptr, int (*call_back)())
{
	cmr_int                         rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_flash_notice         *flash_notice = (struct isp_flash_notice*)param_ptr;
	struct ae_flash_notice          ae_notice;
	enum smart_ctrl_flash_mode      flash_mode = 0;
	enum awb_ctrl_flash_status      awb_flash_status = 0;
	UNUSED(call_back);

	if (NULL == cxt || NULL == flash_notice) {
		ISP_LOGE("handle %p,notice %p is NULL error", cxt, flash_notice);
		return ISP_PARAM_NULL;
	}

	ISP_LOGV("mode=%d", flash_notice->mode);
	switch (flash_notice->mode) {
	case ISP_FLASH_PRE_BEFORE:
		ae_notice.mode = AE_FLASH_PRE_BEFORE;
		rtn = af_ctrl_ioctrl(cxt->af_cxt.handle, AF_CMD_SET_FLASH_NOTICE, (void *)&(flash_notice->mode), NULL);
		ae_notice.power.max_charge = flash_notice->power.max_charge;
		ae_notice.power.max_time = flash_notice->power.max_time;
		ae_notice.capture_skip_num = flash_notice->capture_skip_num;
		rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_SET_FLASH_NOTICE, &ae_notice, NULL);
		rtn = awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_FLASH_BEFORE_P, NULL, NULL);
		awb_flash_status = AWB_FLASH_PRE_BEFORE;
		rtn = awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_SET_FLASH_STATUS, (void *)&awb_flash_status, NULL);
		flash_mode = SMART_CTRL_FLASH_PRE;
		rtn = smart_ctl_ioctl(cxt->smart_cxt.handle, ISP_SMART_IOCTL_SET_FLASH_MODE, (void *)&flash_mode, NULL);

		ISP_LOGI("ISP_AE: rtn=%ld", rtn);
		break;

	case ISP_FLASH_PRE_LIGHTING:
	{
		uint32_t ratio = 0;// = flash_notice->flash_ratio;
		struct isp_flash_param *flash_cali = NULL;
		rtn = _isp_get_flash_cali_param(cxt->handle_pm, &flash_cali);
		if (ISP_SUCCESS == rtn) {
			ISP_LOGI("flash param rgb ratio = (%d,%d,%d), lum_ratio = %d",
					flash_cali->cur.r_ratio, flash_cali->cur.g_ratio,
					flash_cali->cur.b_ratio, flash_cali->cur.lum_ratio);
			if (0 != flash_cali->cur.lum_ratio) {
				ratio = flash_cali->cur.lum_ratio;
			}
		}
		ae_notice.mode = AE_FLASH_PRE_LIGHTING;
		ae_notice.flash_ratio = ratio;
		rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_SET_FLASH_NOTICE, &ae_notice, NULL);
		rtn = awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_FLASH_OPEN_P, NULL, NULL);
	//	rtn = _isp_set_awb_flash_gain((cmr_handle)cxt);
		awb_flash_status = AWB_FLASH_PRE_LIGHTING;
		rtn = awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_SET_FLASH_STATUS, (void *)&awb_flash_status, NULL);
		flash_mode = SMART_CTRL_FLASH_PRE;
		rtn = smart_ctl_ioctl(cxt->smart_cxt.handle, ISP_SMART_IOCTL_SET_FLASH_MODE, (void *)&flash_mode, NULL);
		rtn = af_ctrl_ioctrl(cxt->af_cxt.handle, AF_CMD_SET_FLASH_NOTICE, (void *)&(flash_notice->mode), NULL);
		ISP_LOGI("ISP_AE: Flashing ratio=%d, rtn=%ld", ratio, rtn);
		break;
	}

	case ISP_FLASH_PRE_AFTER:
		ae_notice.mode = AE_FLASH_PRE_AFTER;
		ae_notice.will_capture = flash_notice->will_capture;
		rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_SET_FLASH_NOTICE, &ae_notice, NULL);
		rtn = awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_FLASH_CLOSE, NULL, NULL);
		rtn = _isp_set_awb_gain((cmr_handle)cxt);
		awb_flash_status = AWB_FLASH_PRE_AFTER;
		rtn = awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_SET_FLASH_STATUS, (void *)&awb_flash_status, NULL);
		flash_mode = SMART_CTRL_FLASH_CLOSE;
		rtn = smart_ctl_ioctl(cxt->smart_cxt.handle, ISP_SMART_IOCTL_SET_FLASH_MODE, (void *)&flash_mode, NULL);
		rtn = af_ctrl_ioctrl(cxt->af_cxt.handle, AF_CMD_SET_FLASH_NOTICE, (void *)&(flash_notice->mode), NULL);
		break;

	case ISP_FLASH_MAIN_AFTER:
	  //  rtn  = lsc_ctrl_ioctrl(cxt->lsc_cxt.handle, ALSC_FLASH_OFF, NULL, NULL);
		ae_notice.mode = AE_FLASH_MAIN_AFTER;
		rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_SET_FLASH_NOTICE, &ae_notice, NULL);
		rtn = awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_FLASH_CLOSE, NULL, NULL);
		rtn = _isp_set_awb_gain((cmr_handle)cxt);
		rtn = awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_UNLOCK, NULL, NULL);
		awb_flash_status = AWB_FLASH_MAIN_AFTER;
		rtn = awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_SET_FLASH_STATUS, (void *)&awb_flash_status, NULL);
		flash_mode = SMART_CTRL_FLASH_CLOSE;
		rtn = smart_ctl_ioctl(cxt->smart_cxt.handle, ISP_SMART_IOCTL_SET_FLASH_MODE, (void *)&flash_mode, NULL);
		rtn = af_ctrl_ioctrl(cxt->af_cxt.handle, AF_CMD_SET_FLASH_NOTICE, (void *)&(flash_notice->mode), NULL);
		break;

	case ISP_FLASH_MAIN_BEFORE:
	 //   rtn  = lsc_ctrl_ioctrl(cxt->lsc_cxt.handle, ALSC_FLASH_ON, NULL, NULL);
		ae_notice.mode = AE_FLASH_MAIN_BEFORE;
		rtn = af_ctrl_ioctrl(cxt->af_cxt.handle, AF_CMD_SET_FLASH_NOTICE, (void *)&(flash_notice->mode), NULL);
		rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_SET_FLASH_NOTICE, &ae_notice, NULL);
		rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_SET_EXP_GAIN, NULL, NULL);
		awb_flash_status = AWB_FLASH_MAIN_BEFORE;
		rtn = awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_SET_FLASH_STATUS, (void *)&awb_flash_status, NULL);
		rtn = _isp_set_awb_flash_gain((cmr_handle)cxt);
		rtn = awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_LOCK, NULL, NULL);
		break;

	case ISP_FLASH_MAIN_LIGHTING:
		//if (cxt->lib_use_info->ae_lib_info.product_id) {
			ae_notice.mode = AE_FLASH_MAIN_LIGHTING;
			rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_SET_FLASH_NOTICE, &ae_notice, NULL);
		//}
		rtn = awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_FLASH_OPEN_M, NULL, NULL);
		awb_flash_status = AWB_FLASH_MAIN_LIGHTING;
		rtn = awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_SET_FLASH_STATUS, (void *)&awb_flash_status, NULL);
		flash_mode = SMART_CTRL_FLASH_MAIN;
		rtn = smart_ctl_ioctl(cxt->smart_cxt.handle, ISP_SMART_IOCTL_SET_FLASH_MODE, (void *)&flash_mode, NULL);
		rtn = af_ctrl_ioctrl(cxt->af_cxt.handle, AF_CMD_SET_FLASH_NOTICE, (void *)&(flash_notice->mode), NULL);
		break;

	case ISP_FLASH_MAIN_AE_MEASURE:
		ae_notice.mode = AE_FLASH_MAIN_AE_MEASURE;
		ae_notice.flash_ratio = 0;
		rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_SET_FLASH_NOTICE, &ae_notice, NULL);
		awb_flash_status = AWB_FLASH_MAIN_MEASURE;
		rtn = awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_SET_FLASH_STATUS, (void *)&awb_flash_status, NULL);
		break;

	case ISP_FLASH_AF_DONE:
		if (cxt->lib_use_info->ae_lib_info.product_id) {
			ae_notice.mode = AE_FLASH_AF_DONE;
			rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_SET_FLASH_NOTICE, &ae_notice, NULL);
		}
		break;

	default:
		break;
	}

	return rtn;
}

static cmr_int _ispIsoIOCtrl(cmr_handle isp_alg_handle, void *param_ptr, int (*call_back)())
{
	cmr_int                         rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct ae_set_iso               set_iso = {0};
	UNUSED(call_back);

	if (NULL == param_ptr)
		return ISP_PARAM_NULL;

	set_iso.mode = *(uint32_t*)param_ptr;
	rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle,  AE_SET_ISO, &set_iso, NULL);
	ISP_LOGI("ISP_AE: AE_SET_ISO=%d, rtn=%ld", set_iso.mode, rtn);

	return rtn;
}

static cmr_int _ispBrightnessIOCtrl(cmr_handle isp_alg_handle, void *param_ptr, int (*call_back)())
{
	cmr_int                         rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_bright_cfg           cfg = {0};
	struct isp_pm_param_data        param_data;
	struct isp_pm_ioctl_input       input = {NULL, 0};
	struct isp_pm_ioctl_output      output = {NULL, 0};
	UNUSED(call_back);

	cfg.factor = *(uint32_t*)param_ptr;
	memset(&param_data, 0x0, sizeof(param_data));
	BLOCK_PARAM_CFG(input, param_data, ISP_PM_BLK_BRIGHT, ISP_BLK_BRIGHT, &cfg, sizeof(cfg));

	rtn = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_OTHERS, &input, &output);

	return rtn;
}

static cmr_int _ispContrastIOCtrl(cmr_handle isp_alg_handle, void *param_ptr, int (*call_back)())
{
	cmr_int                         rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_contrast_cfg         cfg = {0};
	struct isp_pm_param_data        param_data;
	struct isp_pm_ioctl_input       input = {NULL, 0};
	struct isp_pm_ioctl_output      output = {NULL, 0};
	UNUSED(call_back);

	cfg.factor = *(uint32_t*)param_ptr;
	memset(&param_data, 0x0, sizeof(param_data));
	BLOCK_PARAM_CFG(input, param_data, ISP_PM_BLK_CONTRAST, ISP_BLK_CONTRAST, &cfg, sizeof(cfg));

	rtn = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_OTHERS, &input, &output);

	return rtn;
}

static cmr_int _ispSaturationIOCtrl(cmr_handle isp_alg_handle, void *param_ptr, int (*call_back)())
{
	cmr_int                         rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_saturation_cfg       cfg = {0};
	struct isp_pm_param_data        param_data;
	struct isp_pm_ioctl_input       input = {NULL, 0};
	struct isp_pm_ioctl_output      output = {NULL, 0};
	UNUSED(call_back);

	cfg.factor = *(uint32_t*)param_ptr;
	memset(&param_data, 0x0, sizeof(param_data));
	BLOCK_PARAM_CFG(input, param_data, ISP_PM_BLK_SATURATION, ISP_BLK_SATURATION, &cfg, sizeof(cfg));

	rtn = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_OTHERS, &input, &output);

	return rtn;
}

static cmr_int _ispSharpnessIOCtrl(cmr_handle isp_alg_handle, void *param_ptr, int (*call_back)())
{
	cmr_int                         rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_edge_cfg             cfg = {0};
	struct isp_pm_param_data        param_data;
	struct isp_pm_ioctl_input       input = {NULL, 0};
	struct isp_pm_ioctl_output      output = {NULL, 0};
	UNUSED(call_back);

	cfg.factor = *(uint32_t*)param_ptr;
	memset(&param_data, 0x0, sizeof(param_data));
	BLOCK_PARAM_CFG(input, param_data, ISP_PM_BLK_EDGE_STRENGTH, ISP_BLK_EDGE, &cfg, sizeof(cfg));

	rtn = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_OTHERS, &input, &output);

	return rtn;
}

static cmr_int _ispVideoModeIOCtrl(cmr_handle isp_alg_handle, void *param_ptr, int (*call_back)())
{
	cmr_int                         rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	int                             mode = 0;
	struct ae_set_fps               fps;
	struct isp_pm_param_data        param_data;
	struct isp_pm_ioctl_input       input = {NULL, 0};
	struct isp_pm_ioctl_output      output = {NULL, 0};
	UNUSED(call_back);
	memset((void*)&param_data , 0,sizeof(struct isp_pm_param_data));
	if (NULL == param_ptr) {
		ISP_LOGE("param is NULL error!");
		return ISP_ERROR;
	}

	ISP_LOGI("param val=%d", *((int*)param_ptr));

	if (0 == *((int*)param_ptr)) {
		mode = ISP_MODE_ID_PRV_0;
	} else {
		rtn = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_MODEID_BY_FPS, param_ptr, &mode);
		if (rtn) {
			ISP_LOGE("isp_pm_ioctl failed");
		}
	}

	fps.min_fps = *((uint32_t*)param_ptr);
	fps.max_fps = 0;//*((uint32_t*)param_ptr);
	rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_SET_FPS, &fps, NULL);
	ISP_TRACE_IF_FAIL(rtn, ("ae set fps error"));

	if (0 != *((uint32_t*)param_ptr)) {
		uint32_t work_mode = 2;
		rtn = awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_SET_WORK_MODE, &work_mode, NULL);
		ISP_RETURN_IF_FAIL(rtn, ("awb set_work_mode error"));
	} else {
		uint32_t work_mode = 0;
		rtn = awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_SET_WORK_MODE, &work_mode, NULL);
		ISP_RETURN_IF_FAIL(rtn, ("awb set_work_mode error"));
	}

#ifdef SEPARATE_GAMMA_IN_VIDEO
	if (*((uint32_t*)param_ptr) != 0) {
		uint32_t idx = VIDEO_GAMMA_INDEX;

		smart_ctl_block_disable(cxt->smart_cxt.handle, ISP_SMART_GAMMA);
		BLOCK_PARAM_CFG(input, param_data, ISP_PM_BLK_GAMMA, ISP_BLK_RGB_GAMC, &idx, sizeof(idx));
		isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_OTHERS, (void*)&input, (void*)&output);
	#ifdef Y_GAMMA_SMART_WITH_RGB_GAMMA
		BLOCK_PARAM_CFG(input, param_data, ISP_PM_BLK_YGAMMA, ISP_BLK_Y_GAMMC, &idx, sizeof(idx));
		isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_OTHERS, (void*)&input, (void*)&output);
	#endif
	} else {
		smart_ctl_block_enable_recover(cxt->smart_cxt.handle, ISP_SMART_GAMMA);
	}
#endif
	return rtn;
}

static cmr_int _ispRangeFpsIOCtrl(cmr_handle isp_alg_handle, void *param_ptr, int (*call_back)())
{
	cmr_int                         rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_range_fps            *range_fps = (struct isp_range_fps*)param_ptr;
	struct ae_set_fps               fps;
	UNUSED(call_back);

	if (NULL == range_fps) {
		ISP_LOGE("param is NULL error!");
		return ISP_PARAM_NULL;
	}

	ISP_LOGI("param val=%d", *((int*)param_ptr));

	fps.min_fps = range_fps->min_fps;
	fps.max_fps = range_fps->max_fps;
	rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_SET_FPS, &fps, NULL);

	return rtn;
}

static cmr_int _ispAeOnlineIOCtrl(cmr_handle isp_alg_handle, void *param_ptr, int (*call_back)())
{
	cmr_int                         rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	UNUSED(call_back);

	rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_SET_ONLINE_CTRL, param_ptr, param_ptr);

	return rtn;
}

static cmr_int _ispAeForceIOCtrl(cmr_handle isp_alg_handle, void* param_ptr, int(*call_back)())
{
	cmr_int                         rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct ae_calc_out              ae_result;
	uint32_t                        ae;
	UNUSED(call_back);

	memset((void *)&ae_result, 0,sizeof(struct ae_calc_out));

	if (NULL == param_ptr) {
		ISP_LOGE("param is NULL error!");
		return ISP_PARAM_NULL;
	}

	ae = *(uint32_t*)param_ptr;

	if (0 == ae) {//lock
		rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_SET_FORCE_PAUSE, NULL, (void*)&ae_result);
	} else {//unlock
		rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_SET_FORCE_RESTORE, NULL, (void*)&ae_result);
	}

	ISP_LOGI("rtn %ld", rtn);

	return rtn;
}

static cmr_int _ispGetAeStateIOCtrl(cmr_handle isp_alg_handle, void *param_ptr, int (*call_back)())
{
	cmr_int                         rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	uint32_t                        param = 0;
	UNUSED(call_back);

	if (NULL == param_ptr) {
		ISP_LOGE("param is NULL error!");
		return ISP_PARAM_NULL;
	}

	rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_GET_AE_STATE, NULL, (void*)&param);

	if (AE_STATE_LOCKED == param) {//lock
		*(uint32_t*)param_ptr = 0;
	} else {//unlock
		*(uint32_t*)param_ptr = 1;
	}

	ISP_LOGI("rtn %ld param %d ae %d", rtn, param, *(uint32_t*)param_ptr);

	return rtn;
}

static cmr_int _ispSetAeFpsIOCtrl(cmr_handle isp_alg_handle, void *param_ptr, int (*call_back)())
{
	cmr_int                         rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct ae_set_fps               *fps = (struct ae_set_fps*)param_ptr;
	UNUSED(call_back);

	if (NULL == fps) {
		ISP_LOGE("param is NULL error!");
		return ISP_PARAM_NULL;
	}

	ISP_LOGI("LLS min_fps =%d, max_fps = %d", fps->min_fps, fps->max_fps);

	rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_SET_FPS, fps, NULL);

	return rtn;
}

static int32_t ispGetAeDebugInfoIOCtrl(cmr_handle isp_alg_handle)
{
	int32_t rtn = ISP_SUCCESS;
	int32_t ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct tg_ae_ctrl_alc_log ae_log = {NULL, 0};

	if ( NULL == cxt) {
		ISP_LOGE("Get AE debug info error!");
		rtn =ISP_ERROR;
		return rtn;
	}

	if ( 0x00 == cxt->lib_use_info->ae_lib_info.product_id ) {
		rtn  = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_GET_DEBUG_INFO, NULL, (void *)&ae_log);
		if ( ISP_SUCCESS != rtn ) {
			ISP_LOGE("Get AE debug info failed!");
		}
		cxt->ae_cxt.log_ae = ae_log.log;
		cxt->ae_cxt.log_ae_size = ae_log.size;
		ret += rtn;
	}

	return ret;
}

static const char *DEBUG_MAGIC = "SPRD_ISP";  // 8 bytes
static const char *AE_START    = "ISP_AE__";
static const char *AE_END      = "ISP_AE__";
static const char *AF_START    = "ISP_AF__";
static const char *AF_END      = "ISP_AF__";
static const char *AWB_START   = "ISP_AWB_";
static const char *AWB_END     = "ISP_AWB_";
static const char *LSC_START   = "ISP_LSC_";
static const char *LSC_END     = "ISP_LSC_";
static const char *SMART_START   = "ISP_SMART_";
static const char *SMART_END     = "ISP_SMART_";

static size_t calc_log_size(const void *log, size_t size, const char *begin_magic, const char *end_magic)
{
    if (!log || !size)
        return 0;

    return size + strlen(begin_magic) + strlen(end_magic);
}

#define COPY_MAGIC(m) \
{size_t len; len = strlen(m); memcpy((char *)dst + off, m, len); off += len;}

static size_t copy_log(void *dst, const void *log, size_t size, const char *begin_magic, const char *end_magic)
{
    size_t off = 0;

    if (!log || !size)
        return 0;

    COPY_MAGIC(begin_magic);
    memcpy((char *)dst + off, log, size); off += size;
    COPY_MAGIC(end_magic);
    return off;
}

#define COPY_LOG(l, L) \
{size_t len = copy_log(cxt->commn_cxt.log_isp + off, cxt->l##_cxt.log_##l, cxt->l##_cxt.log_##l##_size, L##_START, L##_END); \
if (len) {log.l##_off = off; off += len; log.l##_len = len;} else {log.l##_off = 0;}}

static cmr_int _ispGetInfoIOCtrl(cmr_handle isp_alg_handle, void *param_ptr, int (*call_back)())
{
	cmr_int                         rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_info                 *info_ptr = param_ptr;
	uint32_t                        total_size = 0;
	uint32_t                        mem_offset = 0;
	uint32_t                        log_ae_size = 0;
	struct sprd_isp_debug_info *p;
	struct _isp_log_info log;
	size_t total, off;
	UNUSED(call_back);

	if (NULL == info_ptr) {
		ISP_LOGE("err,param is null");
		return ISP_PARAM_NULL;
	}

	if (AL_AE_LIB == cxt->lib_use_info->ae_lib_info.product_id) {
		log_ae_size = cxt->ae_cxt.log_alc_ae_size;
	}

	if (cxt->awb_cxt.alc_awb || log_ae_size) {
		total_size = cxt->awb_cxt.log_alc_awb_size + cxt->awb_cxt.log_alc_lsc_size;
		if (cxt->lib_use_info->ae_lib_info.product_id) {
			total_size += log_ae_size;
		}
		if (cxt->ae_cxt.log_alc_size < total_size) {
			if (cxt->ae_cxt.log_alc != NULL) {
				free (cxt->ae_cxt.log_alc);
				cxt->ae_cxt.log_alc = NULL;
			}
			cxt->ae_cxt.log_alc = malloc(total_size);
			if (cxt->ae_cxt.log_alc == NULL) {
				cxt->ae_cxt.log_alc_size = 0;
				return ISP_ERROR;
			}
			cxt->ae_cxt.log_alc_size = total_size;
		}

		if (cxt->awb_cxt.log_alc_awb != NULL) {
			memcpy(cxt->ae_cxt.log_alc, cxt->awb_cxt.log_alc_awb, cxt->awb_cxt.log_alc_awb_size);
		}
		mem_offset += cxt->awb_cxt.log_alc_awb_size;
		if (cxt->awb_cxt.log_alc_lsc != NULL) {
			memcpy(cxt->ae_cxt.log_alc + mem_offset, cxt->awb_cxt.log_alc_lsc, cxt->awb_cxt.log_alc_lsc_size);
		}
		mem_offset += cxt->awb_cxt.log_alc_lsc_size;
		if (cxt->lib_use_info->ae_lib_info.product_id) {
			if (cxt->ae_cxt.log_alc_ae != NULL) {
				memcpy(cxt->ae_cxt.log_alc + mem_offset, cxt->ae_cxt.log_alc_ae, cxt->ae_cxt.log_alc_ae_size);
			}
			mem_offset += cxt->ae_cxt.log_alc_ae_size;
		}

		info_ptr->addr = cxt->ae_cxt.log_alc;
		info_ptr->size = cxt->ae_cxt.log_alc_size;
	} else {
		if (ISP_SUCCESS != ispGetAeDebugInfoIOCtrl(cxt)) {
			ISP_LOGE("isp_get_debug_info failed");
		}

		total_size = sizeof(struct sprd_isp_debug_info) + sizeof(isp_log_info_t)
				+ calc_log_size(cxt->ae_cxt.log_ae, cxt->ae_cxt.log_ae_size, AE_START, AE_END)
				+ calc_log_size(cxt->af_cxt.log_af, cxt->af_cxt.log_af_size, AF_START, AF_END)
				+ calc_log_size(cxt->awb_cxt.log_awb, cxt->awb_cxt.log_awb_size, AWB_START, AWB_END)
				+ calc_log_size(cxt->lsc_cxt.log_lsc, cxt->lsc_cxt.log_lsc_size, LSC_START, LSC_END)
				+ calc_log_size(cxt->smart_cxt.log_smart, cxt->smart_cxt.log_smart_size, SMART_START, SMART_END)
				+ sizeof(uint32_t)/*for end flag*/;

		if (cxt->commn_cxt.log_isp_size < total_size) {
			if (cxt->commn_cxt.log_isp!= NULL) {
				free (cxt->commn_cxt.log_isp);
				cxt->commn_cxt.log_isp = NULL;
			}
			cxt->commn_cxt.log_isp = malloc(total_size);
			if (cxt->commn_cxt.log_isp == NULL) {
				ISP_LOGE("failed to malloc %d", total_size);
				cxt->commn_cxt.log_isp_size = 0;
				info_ptr->addr = 0;
				info_ptr->size = 0;
				return ISP_ERROR;
			}
			cxt->commn_cxt.log_isp_size = total_size;
		}

		p = (struct sprd_isp_debug_info *)cxt->commn_cxt.log_isp;
		p->debug_startflag = SPRD_DEBUG_START_FLAG;
		*((uint32_t*)((uint8_t*)p + total_size - 4))= SPRD_DEBUG_END_FLAG;
		p->debug_len	   = total_size;
		p->version_id	   = SPRD_DEBUG_VERSION_ID;

		memset(&log, 0, sizeof(log));
		memcpy(log.magic, DEBUG_MAGIC, sizeof(log.magic));
		log.ver = 0;

		off = sizeof(struct sprd_isp_debug_info) + sizeof(isp_log_info_t);
		COPY_LOG(ae, AE);
		COPY_LOG(af, AF);
		COPY_LOG(awb, AWB);
		COPY_LOG(lsc, LSC);
		COPY_LOG(smart, SMART);
		memcpy((char*)cxt->commn_cxt.log_isp + sizeof(struct sprd_isp_debug_info), &log, sizeof(log));

		info_ptr->addr = cxt->commn_cxt.log_isp;
		info_ptr->size = cxt->commn_cxt.log_isp_size;
    }

	//char AF_version[30];//AF-yyyymmdd-xx
	//uint32_t len = sizeof(AF_version);
	//rtn = af_ctrl_ioctrl(cxt->af_cxt.handle, AF_CMD_GET_AF_LIB_INFO, (void*)&AF_version, (void*)&len);

	ISP_LOGI("ISP INFO:addr 0x%p, size = %d", info_ptr->addr, info_ptr->size);
	return rtn;
}

static cmr_int _ispGetAwbGainIoctrl(cmr_handle isp_alg_handle, void *param_ptr, int (*call_back)())
{
	cmr_int                         rtn = ISP_SUCCESS;
	struct awb_gain                 result;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_awbc_cfg             *awbc_cfg = (struct isp_awbc_cfg*)param_ptr;
	UNUSED(call_back);

	if (NULL == awbc_cfg) {
		ISP_LOGE("param is NULL error!");
		return ISP_PARAM_NULL;
	}

	rtn = awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_GET_GAIN, (void *)&result, NULL);

	awbc_cfg->r_gain = result.r;
	awbc_cfg->g_gain = result.g;
	awbc_cfg->b_gain = result.b;
	awbc_cfg->r_offset = 0;
	awbc_cfg->g_offset = 0;
	awbc_cfg->b_offset = 0;

	ISP_LOGI("rtn %ld r %d g %d b %d", rtn, result.r, result.g, result.b);

	return rtn;
}

static cmr_int _ispAwbCtIOCtrl(cmr_handle isp_alg_handle, void *param_ptr, int (*call_back)())
{
	cmr_int                         rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	uint32_t                        param = 0;
	UNUSED(call_back);

	if (NULL == param_ptr) {
		ISP_LOGE("param is NULL error!");
		return ISP_PARAM_NULL;
	}

	rtn = awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_GET_CT, (void*)&param, NULL);
	*(uint32_t*)param_ptr = param;

	return rtn;
}

static cmr_int _ispSetLumIOCtrl(cmr_handle isp_alg_handle, void *param_ptr, int (*call_back)())
{
	cmr_int                         rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	uint32_t                        param = 0;
	UNUSED(call_back);

	if (NULL == param_ptr) {
		ISP_LOGE("param is NULL error!");
		return ISP_PARAM_NULL;
	}

	rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_SET_TARGET_LUM, param_ptr, (void*)&param);

	ISP_LOGI("rtn %ld param %d Lum %d", rtn, param, *(uint32_t*)param_ptr);

	return rtn;
}

static cmr_int _ispGetLumIOCtrl(cmr_handle isp_alg_handle, void *param_ptr, int (*call_back)())
{
	cmr_int                         rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	uint32_t                        param = 0;
	UNUSED(call_back);

	if (NULL == param_ptr) {
		ISP_LOGE("param is NULL error!");
		return ISP_PARAM_NULL;
	}

	rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_GET_LUM, NULL, (void*)&param);
	*(uint32_t*)param_ptr = param;

	ISP_LOGI("rtn %ld param %d Lum %d", rtn, param, *(uint32_t*)param_ptr);

	return rtn;
}

static cmr_int _ispHueIOCtrl(cmr_handle isp_alg_handle, void *param_ptr, int (*call_back)())
{
	cmr_int                         rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_hue_cfg              cfg = {0};
	struct isp_pm_param_data        param_data;
	struct isp_pm_ioctl_input       input = {NULL, 0};
	struct isp_pm_ioctl_output      output = {NULL, 0};
	UNUSED(call_back);

	if (NULL == param_ptr) {
		ISP_LOGE("param is NULL error!");
		return ISP_PARAM_NULL;
	}

	cfg.factor = *(uint32_t*)param_ptr;
	memset(&param_data, 0x0, sizeof(param_data));
	BLOCK_PARAM_CFG(input, param_data, ISP_PM_BLK_HUE, ISP_BLK_HUE, &cfg, sizeof(cfg));

	rtn = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_OTHERS, &input, &output);

	return rtn;
}

static cmr_int _ispAfStopIOCtrl(cmr_handle isp_alg_handle, void *param_ptr, int (*call_back)())
{
	cmr_int                         rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	UNUSED(param_ptr);
	UNUSED(call_back);

	rtn = af_ctrl_ioctrl(cxt->af_cxt.handle, AF_CMD_SET_AF_STOP, NULL, NULL);

	return rtn;
}

static cmr_int _ispOnlineFlashIOCtrl(cmr_handle isp_alg_handle, void *param_ptr, int (*call_back)())
{
	cmr_int                         rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	UNUSED(param_ptr);
	UNUSED(call_back);

	cxt->commn_cxt.callback(cxt->commn_cxt.caller_id, ISP_CALLBACK_EVT|ISP_ONLINE_FLASH_CALLBACK, param_ptr, 0);

	return rtn;
}

static cmr_int _ispAeMeasureLumIOCtrl(cmr_handle isp_alg_handle, void *param_ptr, int (*call_back)())
{
	cmr_int                         rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct ae_set_weight            set_weight = {0};
	UNUSED(call_back);

	if (NULL == param_ptr) {
		ISP_LOGE("param is NULL error!");
		return ISP_PARAM_NULL;
	}

	set_weight.mode = *(uint32_t*)param_ptr;
	rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_SET_WEIGHT, &set_weight, NULL);
	ISP_LOGI("ISP_AE: AE_SET_WEIGHT=%d, rtn=%ld", set_weight.mode, rtn);

	return rtn;
}

static cmr_int _ispSceneModeIOCtrl(cmr_handle isp_alg_handle, void *param_ptr, int (*call_back)())
{
	cmr_int                         rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct ae_set_scene             set_scene = {0};
	UNUSED(call_back);

	if (NULL == param_ptr) {
		ISP_LOGE("param is NULL error!");
		return ISP_PARAM_NULL;
	}

	set_scene.mode = *(uint32_t*)param_ptr;
	cxt->commn_cxt.scene_flag = set_scene.mode;
	rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_SET_SCENE_MODE, &set_scene, NULL);

	return rtn;
}

static cmr_int _ispSFTIORead(cmr_handle isp_alg_handle, void *param_ptr, int (*call_back)())
{
	cmr_int                         rtn = ISP_SUCCESS;
	UNUSED(isp_alg_handle);
	UNUSED(param_ptr);
	UNUSED(call_back);
	return rtn;
}

static cmr_int _ispSFTIOWrite(cmr_handle isp_alg_handle, void *param_ptr, int (*call_back)())
{
	cmr_int                         rtn = ISP_SUCCESS;
	UNUSED(isp_alg_handle);
	UNUSED(param_ptr);
	UNUSED(call_back);

	return rtn;
}

static cmr_int _ispSFTIOSetPass(cmr_handle isp_alg_handle, void *param_ptr, int (*call_back)())
{
	UNUSED(isp_alg_handle);
	UNUSED(param_ptr);
	UNUSED(call_back);

	return ISP_SUCCESS;
}

static cmr_int _ispSFTIOSetBypass(cmr_handle isp_alg_handle, void *param_ptr, int (*call_back)())
{
	UNUSED(isp_alg_handle);
	UNUSED(param_ptr);
	UNUSED(call_back);

	return ISP_SUCCESS;
}

static cmr_int _ispSFTIOGetAfValue(cmr_handle isp_alg_handle, void *param_ptr, int (*call_back)())
{
	cmr_int                         rtn = ISP_SUCCESS;
	UNUSED(isp_alg_handle);
	UNUSED(param_ptr);
	UNUSED(call_back);
	return rtn;
}

static cmr_int _ispAfIOGetFullScanInfo(cmr_handle isp_alg_handle, void *param_ptr, int (*call_back)()){
	cmr_int                         rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_af_fullscan_info* af_fullscan_info = (struct isp_af_fullscan_info*)param_ptr;
	UNUSED(call_back);

	rtn = af_ctrl_ioctrl(cxt->af_cxt.handle, AF_CMD_GET_AF_FULLSCAN_INFO, (void*)af_fullscan_info, NULL);

	return rtn;
}

static cmr_int _ispAfIOCtrl(cmr_handle isp_alg_handle, void *param_ptr, int (*call_back)())
{
	cmr_int                         rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_af_win* af_ptr = (struct isp_af_win*)param_ptr;
	struct af_trig_info trig_info;
	uint32_t i;
	UNUSED(call_back);

	trig_info.win_num = af_ptr->valid_win;
	switch (af_ptr->mode) {
	case ISP_FOCUS_TRIG:
		trig_info.mode = AF_MODE_NORMAL;
		break;
	case ISP_FOCUS_MACRO:
		trig_info.mode = AF_MODE_MACRO;
		break;
	case ISP_FOCUS_CONTINUE:
		trig_info.mode = AF_MODE_CONTINUE;
		break;
	case ISP_FOCUS_MANUAL:
		trig_info.mode = AF_MODE_MANUAL;
		break;
	case ISP_FOCUS_VIDEO:
		trig_info.mode = AF_MODE_VIDEO;
		break;
	default:
		trig_info.mode = AF_MODE_NORMAL;
		break;
	}

	for (i=0; i<trig_info.win_num; i++) {
		trig_info.win_pos[i].sx = af_ptr->win[i].start_x;
		trig_info.win_pos[i].sy = af_ptr->win[i].start_y;
		trig_info.win_pos[i].ex = af_ptr->win[i].end_x;
		trig_info.win_pos[i].ey = af_ptr->win[i].end_y;
	}
	rtn = af_ctrl_ioctrl(cxt->af_cxt.handle, AF_CMD_SET_AF_START, (void*)&trig_info, NULL);

	return rtn;
}

static cmr_int _ispBurstIONotice(cmr_handle isp_alg_handle, void *param_ptr, int (*call_back)())
{
	cmr_int                         rtn = ISP_SUCCESS;
	UNUSED(isp_alg_handle);
	UNUSED(param_ptr);
	UNUSED(call_back);

	return rtn;
}

static cmr_int _ispSpecialEffectIOCtrl(cmr_handle isp_alg_handle, void *param_ptr, int (*call_back)())
{
	cmr_int                         rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_pm_param_data        param_data;
	struct isp_pm_ioctl_input       input = {NULL, 0};
	struct isp_pm_ioctl_output      output = {NULL, 0};
	UNUSED(call_back);

	memset((void*)&param_data, 0,sizeof(struct isp_pm_param_data));

	BLOCK_PARAM_CFG(input, param_data, ISP_PM_BLK_SPECIAL_EFFECT, ISP_BLK_CCE, param_ptr, sizeof(param_ptr));
	isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_SPECIAL_EFFECT, (void*)&input, (void*)&output);

	return rtn;
}

static cmr_int _ispFixParamUpdateIOCtrl(cmr_handle isp_alg_handle, void *param_ptr, int (*call_back)())
{
	cmr_int                         rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct sensor_raw_info          *sensor_raw_info_ptr= NULL;
	struct isp_pm_init_input        input;
	uint32_t                        i;
	struct sensor_version_info *version_info = PNULL;
	uint32_t param_source = 0;
	struct isp_pm_ioctl_input awb_input= {NULL, 0};
	struct isp_pm_ioctl_output awb_output = {NULL, 0};
	struct awb_data_info awb_data_ptr = {NULL, 0};
	UNUSED(param_ptr);
	UNUSED(call_back);

	ISP_LOGI("ISP_TOOL:_ispFixParamUpdateIOCtrl");

	if (NULL == isp_alg_handle || NULL == cxt->sn_cxt.sn_raw_info) {
		ISP_LOGE("update param error");
		rtn = ISP_ERROR;
		return rtn;
	}
	sensor_raw_info_ptr = (struct sensor_raw_info *)cxt->sn_cxt.sn_raw_info;
	if(sensor_raw_info_ptr == NULL) {
		ISP_LOGI("ISP_TOOL:sensor_raw_info_ptr is  null");
	}

	version_info =  (struct sensor_version_info *)sensor_raw_info_ptr->version_info;
	input.sensor_name = version_info->sensor_ver_name.sensor_name;

	if (NULL == cxt->handle_pm) {
		ISP_LOGE("param is NULL error!");
		return ISP_PARAM_NULL;
	}

	/* set param source flag */
	param_source = ISP_PARAM_FROM_TOOL;
	rtn = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_PARAM_SOURCE,  (void *)&param_source, NULL);

	input.num = MAX_MODE_NUM;
	for (i = 0; i < MAX_MODE_NUM; i++) {
		if (NULL != sensor_raw_info_ptr->mode_ptr[i].addr) {
			input.tuning_data[i].data_ptr = sensor_raw_info_ptr->mode_ptr[i].addr;
			input.tuning_data[i].size     = sensor_raw_info_ptr->mode_ptr[i].len;
			input.fix_data[i] = sensor_raw_info_ptr->fix_ptr[i];
		} else {
			input.tuning_data[i].data_ptr = NULL;
			input.tuning_data[i].size = 0;
		}
	}
	input.nr_fix_info = &(sensor_raw_info_ptr->nr_fix);

	rtn = isp_pm_update(cxt->handle_pm, ISP_PM_CMD_UPDATE_ALL_PARAMS, &input, PNULL);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("isp param update failed");
		return rtn;
	}
	param_source = 0;
	rtn = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_PARAM_SOURCE,  (void *)&param_source, NULL);

	rtn = _smart_param_update((cmr_handle)cxt);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("smart param update failed");
		return rtn;
	}
	rtn = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_INIT_AWB, &awb_input, &awb_output);
	if (ISP_SUCCESS == rtn && awb_output.param_num) {
		awb_data_ptr.data_ptr = (void *)awb_output.param_data->data_ptr;
		awb_data_ptr.data_size = awb_output.param_data->data_size;
		awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_SET_UPDATE_TUNING_PARAM, (void *)&awb_data_ptr, NULL);
	}

	{
		struct isp_pm_ioctl_input input = {NULL, 0};
		struct isp_pm_ioctl_output output = {NULL, 0};
		struct isp_flash_param *flash_param_ptr=NULL;

		rtn = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_INIT_AE, &input, &output);
		if (ISP_SUCCESS == rtn && output.param_num) {
			int32_t bypass = 0;
			uint32_t target_lum = 0;
			uint32_t *target_lum_ptr = NULL;

			bypass = output.param_data->user_data[0];
			target_lum_ptr = (uint32_t *)output.param_data->data_ptr;
			target_lum = target_lum_ptr[3];
			ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_SET_BYPASS, &bypass, NULL);
			ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_SET_TARGET_LUM, &target_lum, NULL);
		}

		rtn = _isp_get_flash_cali_param(cxt->handle_pm, &flash_param_ptr);
		if (ISP_SUCCESS == rtn) {
			ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_SET_FLASH_ON_OFF_THR, (void*)&flash_param_ptr->cur.auto_flash_thr, NULL);
		}
	}
	return rtn;
}

static cmr_int _ispGetAdgainExpInfo(cmr_handle isp_alg_handle, void *param_ptr, int (*call_back)())
{
	cmr_int                                     rtn = ISP_SUCCESS;
	struct isp_alg_fw_context                   *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_adgain_exp_info                  *info_ptr = (struct isp_adgain_exp_info *)param_ptr;
	float gain = 0;
	uint32_t exp_time = 0;
	cmr_int bv =0;
	UNUSED(call_back);

	rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_GET_GAIN, NULL, (void *)&gain);
	rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_GET_EXP_TIME, NULL, (void *)&exp_time);
	rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_GET_BV_BY_LUM_NEW, NULL, (void *)&bv);

	if (!rtn) {
		info_ptr->adgain 	= (uint32_t)gain;
		info_ptr->exp_time 	= exp_time;
		info_ptr->bv 		= bv;
	}
	ISP_LOGV("adgain = %d, exp = %d, bv = %d", info_ptr->adgain, info_ptr->exp_time, info_ptr->bv);
	return rtn;
}

static cmr_int _ispParamUpdateIOCtrl(cmr_handle isp_alg_handle, void *param_ptr, int (*call_back)())
{
	cmr_int                         rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_pm_init_input        input;
	uint32_t param_source = 0;
	struct isp_mode_param           *mode_param_ptr = param_ptr;
	uint32_t                        i;
	struct isp_pm_ioctl_input awb_input= {NULL, 0};
	struct isp_pm_ioctl_output awb_output = {NULL, 0};
	struct awb_data_info awb_data_ptr = {NULL, 0};
	UNUSED(call_back);

	ISP_LOGI("--IOCtrl--PARAM_UPDATE--");

	if (NULL == mode_param_ptr) {
		ISP_LOGE("param is NULL error!");
		return ISP_PARAM_NULL;
	}

	if (NULL == cxt || NULL == cxt->handle_pm) {
		ISP_LOGE("param is NULL error!");
		return ISP_PARAM_NULL;
	}

	param_source = ISP_PARAM_FROM_TOOL;
	rtn = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_PARAM_SOURCE, (void *)&param_source, NULL);

	input.num = MAX_MODE_NUM;
	for (i = 0; i < MAX_MODE_NUM; i++) {
		if (mode_param_ptr->mode_id == i) {
			input.tuning_data[i].data_ptr = mode_param_ptr;
			input.tuning_data[i].size = mode_param_ptr->size;
		} else {
			input.tuning_data[i].data_ptr = NULL;
			input.tuning_data[i].size = 0;
		}
		mode_param_ptr = (struct isp_mode_param*)((uint8_t*)mode_param_ptr + mode_param_ptr->size);
	}

	rtn = isp_pm_update(cxt->handle_pm, ISP_PM_CMD_UPDATE_ALL_PARAMS, &input, NULL);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("isp param update failed");
		return rtn;
	}
	param_source = 0;
	rtn = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_PARAM_SOURCE,  (void *)&param_source, NULL);

	rtn = _smart_param_update((cmr_handle)cxt);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("smart param update failed");
		return rtn;
	}

	rtn = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_INIT_AWB, &awb_input, &awb_output);
	if (ISP_SUCCESS == rtn && awb_output.param_num) {
		awb_data_ptr.data_ptr = (void *)awb_output.param_data->data_ptr;
		awb_data_ptr.data_size = awb_output.param_data->data_size;
		awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_SET_UPDATE_TUNING_PARAM, (void *)&awb_data_ptr, NULL);
	}

	{
		struct isp_pm_ioctl_input input = {NULL, 0};
		struct isp_pm_ioctl_output output = {NULL, 0};

		rtn = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_INIT_AE, &input, &output);
		if (ISP_SUCCESS == rtn && output.param_num) {
			int32_t bypass = 0;

			bypass = output.param_data->user_data[0];
			ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_SET_BYPASS, &bypass, NULL);
		}
	}
	return rtn;
}

static cmr_int _ispAeTouchIOCtrl(cmr_handle isp_alg_handle, void* param_ptr, int(*call_back)())
{
	cmr_int                         rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	int                             out_param = 0;
	struct isp_pos_rect             *rect = NULL;
	struct ae_set_tuoch_zone        touch_zone;
	UNUSED(call_back);

	if (NULL == param_ptr) {
		ISP_LOGE("param is NULL error");
		return ISP_PARAM_NULL;
	}

	memset(&touch_zone, 0, sizeof(touch_zone));
	rect = (struct isp_pos_rect*)param_ptr;
	touch_zone.touch_zone.x = rect->start_x;
	touch_zone.touch_zone.y = rect->start_y;
	touch_zone.touch_zone.w = rect->end_x - rect->start_x + 1;
	touch_zone.touch_zone.h = rect->end_y - rect->start_y + 1;
	rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_SET_TOUCH_ZONE, &touch_zone, &out_param);

	ISP_LOGV("w,h=(%d,%d)", touch_zone.touch_zone.w, touch_zone.touch_zone.h);

	return rtn;
}

static cmr_int _ispAfModeIOCtrl(cmr_handle isp_alg_handle, void *param_ptr, int (*call_back)())
{
	cmr_int                          rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	uint32_t set_mode;
	UNUSED(call_back);

	switch (*(uint32_t *)param_ptr) {
	case ISP_FOCUS_MACRO:
		set_mode = AF_MODE_MACRO;
		break;
	case ISP_FOCUS_CONTINUE:
		set_mode = AF_MODE_CONTINUE;
		break;
	case ISP_FOCUS_VIDEO:
		set_mode = AF_MODE_VIDEO;
		break;
	case ISP_FOCUS_MANUAL:
		set_mode = AF_MODE_MANUAL;
		break;
	case ISP_FOCUS_PICTURE:
		set_mode = AF_MODE_PICTURE;
		break;
	case ISP_FOCUS_FULLSCAN:
		set_mode = AF_MODE_FULLSCAN;
		break;
	default:
		set_mode = AF_MODE_NORMAL;
		break;
	}

	rtn = af_ctrl_ioctrl(cxt->af_cxt.handle, AF_CMD_SET_AF_MODE, (void*)&set_mode, NULL);

	return rtn;
}

static cmr_int _ispGetAfModeIOCtrl(cmr_handle isp_alg_handle, void *param_ptr, int (*call_back)())
{
	cmr_int                         rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	uint32_t param = 0;
	UNUSED(call_back);

	rtn = af_ctrl_ioctrl(cxt->af_cxt.handle, AF_CMD_GET_AF_MODE, (void*)&param, NULL);

	switch (param) {
	case AF_MODE_NORMAL:
		*(uint32_t*)param_ptr = ISP_FOCUS_TRIG;
		break;
	case AF_MODE_MACRO:
		*(uint32_t*)param_ptr = ISP_FOCUS_MACRO;
		break;
	case AF_MODE_CONTINUE:
		*(uint32_t*)param_ptr = ISP_FOCUS_CONTINUE;
		break;
	case AF_MODE_MANUAL:
		*(uint32_t*)param_ptr  = ISP_FOCUS_MANUAL;
		break;
	case AF_MODE_VIDEO:
		*(uint32_t*)param_ptr = ISP_FOCUS_VIDEO;
		break;
	default:
		*(uint32_t*)param_ptr = ISP_FOCUS_TRIG;
		break;
	}

	return rtn;
}

static cmr_int _ispAfInfoIOCtrl(cmr_handle isp_alg_handle, void *param_ptr, int (*call_back)())
{
	cmr_int                         rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_af_ctrl* af_ctrl_ptr = (struct isp_af_ctrl*)param_ptr;
	struct af_monitor_set monitor_set;
	uint32_t isp_tool_af_test;
	UNUSED(call_back);

	if (ISP_CTRL_SET==af_ctrl_ptr->mode) {
		isp_tool_af_test = 1;
		rtn = af_ctrl_ioctrl(cxt->af_cxt.handle, AF_CMD_SET_ISP_TOOL_AF_TEST, &isp_tool_af_test, NULL);
		monitor_set.bypass = 0;
		monitor_set.int_mode = 1;
		monitor_set.need_denoise = 0;
		monitor_set.skip_num = 0;
		monitor_set.type = 1;
		cmr_u32 af_envi_type = AF_ENVI_INDOOR;
		rtn = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AF_MONITOR, (void *)&monitor_set, (void *)&af_envi_type);
		rtn = af_ctrl_ioctrl(cxt->af_cxt.handle, AF_CMD_SET_DEFAULT_AF_WIN, NULL, NULL);
		rtn = af_ctrl_ioctrl(cxt->af_cxt.handle, AF_CMD_SET_AF_POS, (void*)&af_ctrl_ptr->step, NULL);
	} else if (ISP_CTRL_GET == af_ctrl_ptr->mode){
		uint32_t cur_pos;
		struct isp_af_statistic_info afm_stat;
		uint32_t i;
		memset((void*)&afm_stat, 0, sizeof(afm_stat));
		rtn = af_ctrl_ioctrl(cxt->af_cxt.handle, AF_CMD_GET_AF_CUR_POS, (void*)&cur_pos, NULL);
		rtn = isp_dev_raw_afm_type1_statistic(cxt->dev_access_handle, (void*)afm_stat.info_tshark3);
		af_ctrl_ptr->step = cur_pos;
		af_ctrl_ptr->num = 9;
		for (i=0;i<af_ctrl_ptr->num;i++) {
			af_ctrl_ptr->stat_value[i] = afm_stat.info_tshark3[i];
		}
	} else {
		isp_tool_af_test = 0;
		cmr_u32 bypass = 1;
		rtn = af_ctrl_ioctrl(cxt->af_cxt.handle, AF_CMD_SET_ISP_TOOL_AF_TEST, (void *)&isp_tool_af_test, NULL);
		rtn = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_RAW_AFM_BYPASS, (void *)&bypass, NULL);
	}

	return rtn;
}

static cmr_int _ispGetAfPosIOCtrl(cmr_handle isp_alg_handle, void *param_ptr, int (*call_back)())
{
	cmr_int                         rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	UNUSED(call_back);

	rtn = af_ctrl_ioctrl(cxt->af_cxt.handle, AF_CMD_GET_AF_CUR_POS, param_ptr, NULL);

	return rtn;
}

static cmr_int _ispSetAfPosIOCtrl(cmr_handle isp_alg_handle, void *param_ptr, int (*call_back)())
{
	cmr_int                         rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	UNUSED(call_back);

	rtn = af_ctrl_ioctrl(cxt->af_cxt.handle, AF_CMD_SET_AF_POS, param_ptr, NULL);

	return rtn;
}

static cmr_int _ispRegIOCtrl(cmr_handle isp_alg_handle, void *param_ptr, int (*call_back)())
{
	cmr_int                         rtn = ISP_SUCCESS;
	UNUSED(isp_alg_handle);
	UNUSED(param_ptr);
	UNUSED(call_back);
	return rtn;
}

static cmr_int _ispScalerTrimIOCtrl(cmr_handle isp_alg_handle, void *param_ptr, int (*call_back)())
{
	cmr_int                         rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_trim_size            *trim = (struct isp_trim_size*)param_ptr;
	UNUSED(call_back);

	if (NULL != trim) {
		struct ae_trim scaler;

		scaler.x = trim->x;
		scaler.y = trim->y;
		scaler.w = trim->w;
		scaler.h = trim->h;

		rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_SET_STAT_TRIM, &scaler, NULL);
	}

	return rtn;
}

static cmr_int _ispFaceAreaIOCtrl(cmr_handle isp_alg_handle, void *param_ptr, int (*call_back)())
{
	cmr_int                         rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_face_area            *face_area = (struct isp_face_area*)param_ptr;
	UNUSED(call_back);

	if (NULL != face_area) {
		struct ae_fd_param fd_param;
		int32_t i;

		fd_param.width    = face_area->frame_width;
		fd_param.height   = face_area->frame_height;
		fd_param.face_num = face_area->face_num;
		for (i = 0; i < fd_param.face_num; ++i) {
			fd_param.face_area[i].rect.start_x = face_area->face_info[i].sx;
			fd_param.face_area[i].rect.start_y = face_area->face_info[i].sy;
			fd_param.face_area[i].rect.end_x   = face_area->face_info[i].ex;
			fd_param.face_area[i].rect.end_y   = face_area->face_info[i].ey;
			fd_param.face_area[i].face_lum     = face_area->face_info[i].brightness;
			fd_param.face_area[i].pose         = face_area->face_info[i].pose;
		}

		rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_SET_FD_PARAM, &fd_param, NULL);

		rtn = af_ctrl_ioctrl(cxt->af_cxt.handle, AF_CMD_SET_FACE_DETECT, (void*)param_ptr, NULL);
	}

	return rtn;
}

static cmr_int _ispStart3AIOCtrl(cmr_handle isp_alg_handle, void *param_ptr, int (*call_back)())
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct ae_calc_out              ae_result;
	uint32_t                        af_bypass = 0;
	UNUSED(param_ptr);
	UNUSED(call_back);

	memset((void*)&ae_result, 0, sizeof(struct ae_calc_out));

	rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_SET_FORCE_RESTORE, NULL, (void*)&ae_result);
	rtn = awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_UNLOCK, NULL, NULL);
	rtn = af_ctrl_ioctrl(cxt->af_cxt.handle, AF_CMD_SET_AF_BYPASS, (void *)&af_bypass, NULL);

	ISP_LOGI("done");

	return ISP_SUCCESS;
}

static cmr_int _ispStop3AIOCtrl(cmr_handle isp_alg_handle, void *param_ptr, int (*call_back)())
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct ae_calc_out              ae_result;
	uint32_t                        af_bypass = 1;
	UNUSED(param_ptr);
	UNUSED(call_back);

	memset((void*)&ae_result, 0, sizeof(struct ae_calc_out));

	rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_SET_FORCE_PAUSE, NULL, (void*)&ae_result);
	rtn = awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_LOCK, NULL, NULL);
	rtn = af_ctrl_ioctrl(cxt->af_cxt.handle, AF_CMD_SET_AF_BYPASS, (void *)&af_bypass, NULL);

	ISP_LOGI("done");

	return ISP_SUCCESS;
}

static cmr_int _ispHdrIOCtrl(cmr_handle isp_alg_handle, void *param_ptr, int (*call_back)())
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	int16_t                         smart_block_eb[ISP_SMART_MAX_BLOCK_NUM];
	SENSOR_EXT_FUN_PARAM_T          hdr_ev_param;
	struct isp_hdr_ev_param         *isp_hdr = NULL;
	UNUSED(call_back);

	if (NULL == isp_alg_handle || NULL == param_ptr) {
		ISP_LOGE("cxt=%p param_ptr=%p is NULL error", isp_alg_handle, param_ptr);
		return ISP_PARAM_NULL;
	}
	//add for STACK ERROR
	return ISP_SUCCESS;
	isp_hdr = (struct isp_hdr_ev_param*)param_ptr;

	memset(&smart_block_eb, 0x00, sizeof(smart_block_eb));

	smart_ctl_block_eb(cxt->smart_cxt.handle, &smart_block_eb, 0);
	rtn = awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_LOCK, NULL, NULL);

	hdr_ev_param.cmd   = SENSOR_EXT_EV;
	hdr_ev_param.param = isp_hdr->level & 0xFF;
	cxt->ioctrl_ptr->ext_fuc(cxt->ioctrl_ptr->caller_handler,&hdr_ev_param);
	rtn = awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_UNLOCK, NULL, NULL);
	smart_ctl_block_eb(cxt->smart_cxt.handle, &smart_block_eb, 1);
	rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_GET_SKIP_FRAME_NUM, NULL, &isp_hdr->skip_frame_num);

	return ISP_SUCCESS;
}

static cmr_int _ispSetAeNightModeIOCtrl(cmr_handle isp_alg_handle, void *param_ptr, int (*call_back)())
{
	cmr_int                         rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	uint32_t                        night_mode=0;
	UNUSED(call_back);

	if (NULL == param_ptr) {
		return ISP_PARAM_NULL;
	}

	night_mode = *(uint32_t*)param_ptr;
	rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_SET_NIGHT_MODE, &night_mode, NULL);

	ISP_LOGI("ISP_AE: AE_SET_NIGHT_MODE=%d, rtn=%ld", night_mode, rtn);

	return rtn;
}

static cmr_int _ispSetAeAwbLockUnlock(cmr_handle isp_alg_handle, void *param_ptr, int (*call_back)())
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct ae_calc_out              ae_result;
	uint32_t                        ae_awb_mode=0;
	UNUSED(call_back);

	if (NULL == param_ptr) {
		return ISP_PARAM_NULL;
	}
	memset((void*)&ae_result, 0, sizeof(struct ae_calc_out));

	ae_awb_mode = *(uint32_t*)param_ptr;
	if (ISP_AE_AWB_LOCK == ae_awb_mode) { // AE & AWB Lock
		ISP_LOGI("AE & AWB Lock");
		rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_SET_PAUSE, NULL, (void*)&ae_result);
		rtn = awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_LOCK, NULL, NULL);
	} else if (ISP_AE_AWB_UNLOCK == ae_awb_mode) { // AE & AWB Unlock
		ISP_LOGI("AE & AWB Un-Lock\n");
		rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_SET_RESTORE, NULL, (void*)&ae_result);
		rtn = awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_UNLOCK, NULL, NULL);
	} else {
		ISP_LOGI("Unsupported AE & AWB mode (%d)\n", ae_awb_mode);
	}

	return ISP_SUCCESS;
}

static cmr_int _ispSetAeLockUnlock(cmr_handle isp_alg_handle, void *param_ptr, int (*call_back)())
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct ae_calc_out              ae_result;
	uint32_t                        ae_mode;
	UNUSED(call_back);

	if (NULL == param_ptr) {
		return ISP_PARAM_NULL;
	}
	memset((void*)&ae_result, 0, sizeof(struct ae_calc_out));

	ae_mode = *(uint32_t*)param_ptr;
	if (ISP_AE_LOCK == ae_mode) { // AE & AWB Lock
		ISP_LOGI("AE Lock\n");
		rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_SET_PAUSE, NULL, (void*)&ae_result);
	} else if (ISP_AE_UNLOCK == ae_mode) { // AE  Unlock
		ISP_LOGI("AE Un-Lock\n");
		rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_SET_RESTORE, NULL, (void*)&ae_result);
	} else {
		ISP_LOGI("Unsupported AE  mode (%d)\n", ae_mode);
	}

	return ISP_SUCCESS;
}

static cmr_int _ispDenoiseParamRead(cmr_handle isp_alg_handle, void *param_ptr, int (*call_back)())
{
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct sensor_raw_info *raw_sensor_ptr = cxt->sn_cxt.sn_raw_info;
	struct isp_mode_param *mode_common_ptr = (struct isp_mode_param *)raw_sensor_ptr->mode_ptr[0].addr;
	struct denoise_param_update *update_param = (struct denoise_param_update*)param_ptr;
	uint32_t i;
	struct sensor_raw_fix_info *fix_data_ptr = PNULL;
	struct sensor_nr_param *nr_param_ptr = PNULL;
	struct sensor_nr_fix_info *nr_fix = PNULL;
	fix_data_ptr = raw_sensor_ptr->fix_ptr[0];
	nr_fix = &raw_sensor_ptr->nr_fix;
	UNUSED(call_back);

	if (NULL == param_ptr) {
		return ISP_PARAM_NULL;
	}

//	if(cxt->commn_cxt.multi_nr_flag == SENSOR_MULTI_MODE_FLAG) {
		update_param->multi_nr_flag = SENSOR_MULTI_MODE_FLAG;
		update_param->nr_scene_map_ptr = nr_fix->nr_scene_ptr;
		update_param->nr_level_number_map_ptr = nr_fix->nr_level_number_ptr;
		update_param->nr_default_level_map_ptr = nr_fix->nr_default_level_ptr;
	if (update_param->nr_level_number_map_ptr) {
		 ISP_LOGI("ISP_TOOL:update_param->nr_level_number_map_ptr sizeof = %d", sizeof(update_param->nr_level_number_map_ptr));
	} else {
		 ISP_LOGI("ISP_TOOL:_ispDenoiseParamRead nr map is null");
	}
//	} else {
//		update_param->multi_nr_flag = 0x0;
//		update_param->nr_scene_map_ptr = PNULL;
//		update_param->nr_level_map_ptr = PNULL;
//	}
//	update_param->multi_nr_flag = SENSOR_MULTI_MODE_FLAG;
//	update_param->nr_scene_map_ptr = fix_data_ptr->nr.nr_map_group.nr_scene_map_ptr;
//	update_param->nr_level_map_ptr = fix_data_ptr->nr.nr_map_group.nr_level_map_ptr;
	for (i = 0; i < mode_common_ptr->block_num; i++) {
		struct isp_block_header *header = &(mode_common_ptr->block_header[i]);
		uint8_t *data = (uint8_t*)mode_common_ptr + header->offset;

		switch (header->block_id) {
			case ISP_BLK_PDAF_CORRECT: {
				update_param->pdaf_correction_level_ptr = (struct sensor_pdaf_correction_level *)fix_data_ptr->nr.nr_set_group.pdaf_correct;//0x14
				break;
			}
			case ISP_BLK_BPC: {
				update_param->bpc_level_ptr = (struct sensor_bpc_level *)fix_data_ptr->nr.nr_set_group.bpc;//0x19
				break;
			}
			case ISP_BLK_GRGB: {
				update_param->grgb_level_ptr = (struct sensor_grgb_level *)fix_data_ptr->nr.nr_set_group.grgb;//0x1A
				break;
			}
			case ISP_BLK_NLM: {
				update_param->nlm_level_ptr = (struct sensor_nlm_level *)fix_data_ptr->nr.nr_set_group.nlm;//0x15
				update_param->vst_level_ptr = (struct sensor_vst_level *)fix_data_ptr->nr.nr_set_group.vst;//0x16
				update_param->ivst_level_ptr = (struct sensor_ivst_level *)fix_data_ptr->nr.nr_set_group.ivst;//0x17
				break;
			}
			case ISP_BLK_RGB_DITHER: {
				update_param->rgb_dither_level_ptr = (struct sensor_rgb_dither_level *)fix_data_ptr->nr.nr_set_group.rgb_dither;//0x18
				break;
			}
			case ISP_BLK_CFA: {
				update_param->cfae_level_ptr = (struct sensor_cfai_level *)fix_data_ptr->nr.nr_set_group.cfa;//0x1B
				break;
			}
			case ISP_BLK_RGB_AFM: {
				update_param->rgb_afm_level_ptr = (struct sensor_rgb_afm_level *)fix_data_ptr->nr.nr_set_group.rgb_afm;//0x1C
				break;
			}
			case ISP_BLK_UVDIV: {
				update_param->cce_uvdiv_level_ptr = (struct sensor_cce_uvdiv_level *)fix_data_ptr->nr.nr_set_group.uvdiv;//0x1D
				break;
			}
			case ISP_BLK_3DNR_PRE: {
				update_param->dnr_pre_level_ptr = (struct sensor_3dnr_level *)fix_data_ptr->nr.nr_set_group.nr3d_pre;//0x1E
				break;
			}
			case ISP_BLK_3DNR_CAP: {
				update_param->dnr_cap_level_ptr = (struct sensor_3dnr_level *)fix_data_ptr->nr.nr_set_group.nr3d_cap;//0x1F
				break;
			}
			case ISP_BLK_EDGE: {
				update_param->ee_level_ptr = (struct sensor_ee_level *)fix_data_ptr->nr.nr_set_group.edge;//0x20
				break;
			}
			case ISP_BLK_YUV_PRECDN: {
				update_param->yuv_precdn_level_ptr = (struct sensor_yuv_precdn_level *)fix_data_ptr->nr.nr_set_group.yuv_precdn;//0x21
				break;
			}
			case ISP_BLK_YNR: {
				update_param->ynr_level_ptr = (struct sensor_ynr_level *)fix_data_ptr->nr.nr_set_group.ynr;//0x22
				break;
			}
			case ISP_BLK_UV_CDN: {
				update_param->uv_cdn_level_ptr = (struct sensor_uv_cdn_level *)fix_data_ptr->nr.nr_set_group.cdn;//0x23
				break;
			}
			case ISP_BLK_UV_POSTCDN: {
				update_param->uv_postcdn_level_ptr = (struct sensor_uv_postcdn_level *)fix_data_ptr->nr.nr_set_group.postcdn;//0x24
				break;
			}
			case ISP_BLK_IIRCNR_IIR: {
				update_param->iircnr_level_ptr = (struct sensor_iircnr_level *)fix_data_ptr->nr.nr_set_group.iircnr;//0x25
				break;
			}
			case ISP_BLK_YUV_NOISEFILTER: {
				update_param->yuv_noisefilter_level_ptr = (struct sensor_yuv_noisefilter_level *)fix_data_ptr->nr.nr_set_group.yuv_noisefilter;//0x26
				break;
			}
			default:
				break;
		}
	}

	return ISP_SUCCESS;
}

static cmr_int _ispSensorDenoiseParamUpdate(cmr_handle isp_alg_handle, void *param_ptr, int (*call_back)())
{
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct sensor_raw_info          *raw_sensor_ptr = cxt->sn_cxt.sn_raw_info;
	struct isp_mode_param *mode_common_ptr = (struct isp_mode_param *)raw_sensor_ptr->mode_ptr[0].addr;
	struct denoise_param_update     *update_param = (struct denoise_param_update*)param_ptr;
	uint32_t i;
	UNUSED(call_back);

	if (NULL == param_ptr) {
		return ISP_PARAM_NULL;
	}
#if 0
	for (i = 0; i < mode_common_ptr->block_num; i++) {
		struct isp_block_header *header = &(mode_common_ptr->block_header[i]);
		uint8_t *data = (uint8_t*)mode_common_ptr + header->offset;

		switch (header->block_id) {
		case ISP_BLK_BPC: {
			struct sensor_bpc_param* block = (struct sensor_bpc_param*)data;
			block->param_ptr = (isp_uint *)update_param->bpc_level_ptr;
			block->reserved[0] = update_param->multi_mode_enable;
			break;
		}
		case ISP_BLK_GRGB: {
			struct sensor_grgb_param* block = (struct sensor_grgb_param*)data;
			block->param_ptr = (isp_uint *)update_param->grgb_level_ptr;
			block->reserved[0] = update_param->multi_mode_enable;
			break;
		}
		case ISP_BLK_NLM: {
			struct sensor_nlm_param* block = (struct sensor_nlm_param*)data;
			block->param_nlm_ptr = (isp_uint *)update_param->nlm_level_ptr;
			block->param_vst_ptr = (isp_uint *)update_param->vst_level_ptr;
			block->param_ivst_ptr = (isp_uint *)update_param->ivst_level_ptr;
			//block->param_flat_offset_ptr = (isp_uint *)update_param->flat_offset_level_ptr;
			//block->reserved[0] = update_param->multi_mode_enable;
			break;
		}
		case ISP_BLK_CFA: {
			struct sensor_cfa_param* block = (struct sensor_cfa_param*)data;
			block->param_ptr = (isp_uint *)update_param->cfae_level_ptr;
			block->reserved[0] = update_param->multi_mode_enable;
			break;
		}
		case ISP_BLK_YUV_PRECDN: {
			struct sensor_yuv_precdn_param* block = (struct sensor_yuv_precdn_param*)data;
			block->param_ptr = (isp_uint *)update_param->yuv_precdn_level_ptr;
			block->reserved3[0] = update_param->multi_mode_enable;
			break;
		}
		case ISP_BLK_UV_CDN: {
			struct sensor_uv_cdn_param* block = (struct sensor_uv_cdn_param*)data;
			block->param_ptr = (isp_uint *)update_param->uv_cdn_level_ptr;
			block->reserved2[0] = update_param->multi_mode_enable;
			break;
		}
		case ISP_BLK_EDGE: {
			struct sensor_ee_param* block = (struct sensor_ee_param*)data;
			block->param_ptr = (isp_uint *)update_param->ee_level_ptr;
			block->reserved[0] = update_param->multi_mode_enable;
			break;
		}
		case ISP_BLK_UV_POSTCDN: {
			struct sensor_uv_postcdn_param* block = (struct sensor_uv_postcdn_param*)data;
			block->param_ptr = (isp_uint *)update_param->uv_postcdn_level_ptr;
			block->reserved[0] = update_param->multi_mode_enable;
			break;
		}
		case ISP_BLK_IIRCNR_IIR: {
			struct sensor_iircnr_param* block = (struct sensor_iircnr_param*)data;
			block->param_ptr = (isp_uint *)update_param->iircnr_level_ptr;
			block->reserved[0] = update_param->multi_mode_enable;
			break;
		}
		case ISP_BLK_IIRCNR_YRANDOM: {
			struct sensor_iircnr_yrandom_param* block = (struct sensor_iircnr_yrandom_param*)data;
			block->param_ptr = (isp_uint *)update_param->iircnr_yrandom_level_ptr;
			block->reserved[0] = update_param->multi_mode_enable;
			break;
		}
		case ISP_BLK_UVDIV: {
			struct sensor_cce_uvdiv_param* block = (struct sensor_cce_uvdiv_param*)data;
			block->param_ptr = (isp_uint *)update_param->cce_uvdiv_level_ptr;
			block->reserved1[0] = update_param->multi_mode_enable;
			break;
		}
		default:
			break;
		}
	}
#endif
	ISP_LOGI("_ispSensorDenoiseParamUpdate over");
	return ISP_SUCCESS;
}

static cmr_int _ispDumpReg(cmr_handle isp_alg_handle, void *param_ptr, int (*call_back)())
{
	cmr_int                         ret = ISP_SUCCESS;
	UNUSED(param_ptr);
	UNUSED(isp_alg_handle);
	UNUSED(call_back);

	return ret;
}


static cmr_int _ispToolSetSceneParam(cmr_handle isp_alg_handle, void *param_ptr, int(*call_back)())
{
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isptool_scene_param *scene_parm = NULL;
	struct isp_pm_ioctl_input ioctl_input;
	struct isp_pm_param_data ioctl_data;
	struct isp_awbc_cfg awbc_cfg;
	struct smart_proc_input smart_proc_in;
	uint32_t rtn = ISP_SUCCESS;
	UNUSED(call_back);
	memset((void*)&smart_proc_in, 0, sizeof(struct smart_proc_input));

	scene_parm = (struct isptool_scene_param *)param_ptr;
	/*set awb gain*/
	awbc_cfg.r_gain = scene_parm->awb_gain_r;
	awbc_cfg.g_gain = scene_parm->awb_gain_g;
	awbc_cfg.b_gain = scene_parm->awb_gain_b;
	awbc_cfg.r_offset = 0;
	awbc_cfg.g_offset = 0;
	awbc_cfg.b_offset = 0;

	ioctl_data.id = ISP_BLK_AWB_NEW;
	ioctl_data.cmd = ISP_PM_BLK_AWBC;
	ioctl_data.data_ptr = &awbc_cfg;
	ioctl_data.data_size = sizeof(awbc_cfg);

	ioctl_input.param_data_ptr = &ioctl_data;
	ioctl_input.param_num = 1;

	if(0 == awbc_cfg.r_gain && 0 == awbc_cfg.g_gain && 0 == awbc_cfg.b_gain) {
		awbc_cfg.r_gain = 1800;
		awbc_cfg.g_gain = 1024;
		awbc_cfg.b_gain = 1536;
	}

	ISP_LOGI("AWB_TAG: isp_pm_ioctl rtn=%d, gain=(%d, %d, %d)", rtn, awbc_cfg.r_gain, awbc_cfg.g_gain, awbc_cfg.b_gain);
	rtn = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_AWB, (void *)&ioctl_input, NULL);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("set awb gain failed");
		return rtn;
	}

	smart_proc_in.cal_para.bv = scene_parm->smart_bv;
	smart_proc_in.cal_para.bv_gain = scene_parm->gain;
	smart_proc_in.cal_para.ct = scene_parm->smart_ct;
	smart_proc_in.alc_awb = cxt->awb_cxt.alc_awb;
	smart_proc_in.handle_pm = cxt->handle_pm;
	smart_proc_in.mode_flag = cxt->commn_cxt.mode_flag;
	rtn = _smart_calc(cxt->smart_cxt.handle, &smart_proc_in);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("set smart gain failed");
		return rtn;
	}

	return rtn;
}

static cmr_int _ispForceAeQuickMode(cmr_handle isp_alg_handle, void *param_ptr, int(*call_back)())
{
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	cmr_int rtn = ISP_SUCCESS;
	uint32_t force_quick_mode = *(uint32_t*)param_ptr;
	UNUSED(call_back);

	rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_SET_FORCE_QUICK_MODE, (void*)&force_quick_mode, NULL);
	return rtn;
}

static cmr_int _ispSetAeExpTime(cmr_handle isp_alg_handle, void *param_ptr, int(*call_back)())
{
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	cmr_int rtn = ISP_SUCCESS;
	uint32_t exp_time = *(uint32_t*)param_ptr;
	UNUSED(call_back);

	rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_SET_EXP_TIME, (void*)&exp_time, NULL);
	return rtn;
}

static cmr_int _ispSetAeSensitivity(cmr_handle isp_alg_handle, void *param_ptr, int(*call_back)())
{
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	cmr_int rtn = ISP_SUCCESS;
	uint32_t sensitivity = *(uint32_t*)param_ptr;
	UNUSED(call_back);

	rtn = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_SET_SENSITIVITY, (void*)&sensitivity, NULL);
	return rtn;
}

static cmr_int _ispSetCaptureRawMode(isp_handle isp_handler, void *param_ptr, int(*call_back)())
{
	isp_ctrl_context* handle = (isp_ctrl_context*)isp_handler;
	cmr_int rtn = ISP_SUCCESS;
	struct isp_video_start isp_video_start;
	struct img_size *size_param = (struct img_size *)param_ptr;
	int mode = 0;
	UNUSED(call_back);
	memset((void*)&isp_video_start, 0, sizeof(struct isp_video_start));
	isp_video_start.work_mode = 1;
	isp_video_start.size.w = size_param->width;
	isp_video_start.size.h = size_param->height;
	//rtn = isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_GET_MODEID_BY_RESOLUTION, &isp_video_start, &mode);
	handle->mode_flag = mode;
	return rtn;
}

static cmr_int _ispSetDcamTimestamp(cmr_handle isp_alg_handle, void *param_ptr, int(*call_back)())
{
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	cmr_int rtn = ISP_SUCCESS;
	struct isp_af_ts *af_ts = (struct isp_af_ts*)param_ptr;
	UNUSED(call_back);

	rtn = af_ctrl_ioctrl(cxt->af_cxt.handle, AF_CMD_SET_DCAM_TIMESTAMP, (void*)af_ts, NULL);
	return rtn;
}

static cmr_int _ispSetAuxSensorInfo(cmr_handle isp_alg_handle, void *param_ptr, int(*call_back)())
{
	cmr_int 				    ret = ISP_SUCCESS;
	struct isp_alg_fw_context 		    *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	UNUSED(call_back);

	ret = af_ctrl_ioctrl(cxt->af_cxt.handle,
			     AF_CMD_SET_UPDATE_AUX_SENSOR,
			     (void *)param_ptr, NULL);

	return ret;
}

static struct isp_io_ctrl_fun _s_isp_io_ctrl_fun_tab[] = {
	{IST_CTRL_SNAPSHOT_NOTICE,           _ispSnapshotNoticeIOCtrl},
	{ISP_CTRL_AE_MEASURE_LUM,            _ispAeMeasureLumIOCtrl},
	{ISP_CTRL_EV,                        _ispEVIOCtrl},
	{ISP_CTRL_FLICKER,                   _ispFlickerIOCtrl},
	{ISP_CTRL_ISO,                       _ispIsoIOCtrl},
	{ISP_CTRL_AE_TOUCH,                  _ispAeTouchIOCtrl},
	{ISP_CTRL_FLASH_NOTICE,              _ispFlashNoticeIOCtrl},
	{ISP_CTRL_VIDEO_MODE,                _ispVideoModeIOCtrl},
	{ISP_CTRL_SCALER_TRIM,               _ispScalerTrimIOCtrl},
	{ISP_CTRL_RANGE_FPS,                 _ispRangeFpsIOCtrl},
	{ISP_CTRL_FACE_AREA,                 _ispFaceAreaIOCtrl},

	{ISP_CTRL_AEAWB_BYPASS,              _ispAeAwbBypassIOCtrl},
	{ISP_CTRL_AWB_MODE,                  _ispAwbModeIOCtrl},

	{ISP_CTRL_AF,                        _ispAfIOCtrl},
	{ISP_CTRL_GET_FULLSCAN_INFO,                        _ispAfIOGetFullScanInfo},
	{ISP_CTRL_BURST_NOTICE,              _ispBurstIONotice},
	{ISP_CTRL_SFT_READ,                  _ispSFTIORead},
	{ISP_CTRL_SFT_WRITE,                 _ispSFTIOWrite},
	{ISP_CTRL_SFT_SET_PASS,              _ispSFTIOSetPass},// added for sft
	{ISP_CTRL_SFT_SET_BYPASS,            _ispSFTIOSetBypass},// added for sft
	{ISP_CTRL_SFT_GET_AF_VALUE,          _ispSFTIOGetAfValue},// added for sft
	{ISP_CTRL_AF_MODE,                   _ispAfModeIOCtrl},
	{ISP_CTRL_AF_STOP,                   _ispAfStopIOCtrl},
	{ISP_CTRL_FLASH_CTRL,                _ispOnlineFlashIOCtrl},

	{ISP_CTRL_SCENE_MODE,                _ispSceneModeIOCtrl},
	{ISP_CTRL_SPECIAL_EFFECT,            _ispSpecialEffectIOCtrl},
	{ISP_CTRL_BRIGHTNESS,                _ispBrightnessIOCtrl},
	{ISP_CTRL_CONTRAST,                  _ispContrastIOCtrl},
	{ISP_CTRL_SATURATION,                _ispSaturationIOCtrl},
	{ISP_CTRL_SHARPNESS,                 _ispSharpnessIOCtrl},
	{ISP_CTRL_HDR,                       _ispHdrIOCtrl},

	{ISP_CTRL_PARAM_UPDATE,              _ispParamUpdateIOCtrl},
	{ISP_CTRL_IFX_PARAM_UPDATE,          _ispFixParamUpdateIOCtrl},
	{ISP_CTRL_GET_CUR_ADGAIN_EXP,		 _ispGetAdgainExpInfo},
	{ISP_CTRL_AE_CTRL,                   _ispAeOnlineIOCtrl}, // for isp tool cali
	{ISP_CTRL_SET_LUM,                   _ispSetLumIOCtrl}, // for tool cali
	{ISP_CTRL_GET_LUM,                   _ispGetLumIOCtrl}, // for tool cali
	{ISP_CTRL_AF_CTRL,                   _ispAfInfoIOCtrl}, // for tool cali
	{ISP_CTRL_SET_AF_POS,                _ispSetAfPosIOCtrl}, // for tool cali
	{ISP_CTRL_GET_AF_POS,                _ispGetAfPosIOCtrl}, // for tool cali
	{ISP_CTRL_GET_AF_MODE,               _ispGetAfModeIOCtrl}, // for tool cali
	{ISP_CTRL_REG_CTRL,                  _ispRegIOCtrl}, // for tool cali
	{ISP_CTRL_AF_END_INFO,               _ispRegIOCtrl}, // for tool cali
	{ISP_CTRL_DENOISE_PARAM_READ,        _ispDenoiseParamRead}, //for tool cali
	{ISP_CTRL_DUMP_REG,   			     _ispDumpReg}, //for tool cali
	{ISP_CTRL_START_3A,                  _ispStart3AIOCtrl},
	{ISP_CTRL_STOP_3A,                   _ispStop3AIOCtrl},

	{ISP_CTRL_AE_FORCE_CTRL,             _ispAeForceIOCtrl}, // for mp tool cali
	{ISP_CTRL_GET_AE_STATE,              _ispGetAeStateIOCtrl}, // for mp tool cali
	{ISP_CTRL_GET_AWB_GAIN,              _ispGetAwbGainIoctrl}, // for mp tool cali
	{ISP_CTRL_GET_AWB_CT,				 _ispAwbCtIOCtrl},
	{ISP_CTRL_SET_AE_FPS,                _ispSetAeFpsIOCtrl},  //for LLS feature
	{ISP_CTRL_GET_INFO,                  _ispGetInfoIOCtrl},
	{ISP_CTRL_SET_AE_NIGHT_MODE,         _ispSetAeNightModeIOCtrl},
	{ISP_CTRL_SET_AE_AWB_LOCK_UNLOCK,    _ispSetAeAwbLockUnlock}, // AE & AWB Lock or Unlock
	{ISP_CTRL_SET_AE_LOCK_UNLOCK,        _ispSetAeLockUnlock},//AE Lock or Unlock
	{ISP_CTRL_TOOL_SET_SCENE_PARAM,      _ispToolSetSceneParam}, // for tool scene param setting
	{ISP_CTRL_FORCE_AE_QUICK_MODE,       _ispForceAeQuickMode},
	{ISP_CTRL_DENOISE_PARAM_UPDATE,      _ispSensorDenoiseParamUpdate}, //for tool cali
	{ISP_CTRL_SET_AE_EXP_TIME,           _ispSetAeExpTime},
	{ISP_CTRL_SET_AE_SENSITIVITY,        _ispSetAeSensitivity},
	{ISP_CTRL_SET_CAPTURE_RAW_MODE,      _ispSetCaptureRawMode},
	{ISP_CTRL_SET_DCAM_TIMESTAMP,        _ispSetDcamTimestamp},
	{ISP_CTRL_SET_AUX_SENSOR_INFO,     _ispSetAuxSensorInfo},

	{ISP_CTRL_MAX,                       NULL}
};

io_fun _ispGetIOCtrlFun(enum isp_ctrl_cmd cmd)
{
	io_fun                          io_ctrl = NULL;
	uint32_t                        total_num;
	uint32_t                        i;

	total_num = sizeof(_s_isp_io_ctrl_fun_tab) / sizeof(struct isp_io_ctrl_fun);
	for (i = 0; i < total_num; i++) {
		if (cmd == _s_isp_io_ctrl_fun_tab[i].cmd) {
			io_ctrl = _s_isp_io_ctrl_fun_tab[i].io_ctrl;
			break;
		}
	}

	return io_ctrl;
}
