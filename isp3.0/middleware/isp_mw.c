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
#define LOG_TAG "isp_mw"

#include <stdlib.h>
#include "cutils/properties.h"
#include <unistd.h>
#include "isp_mw.h"
#include "isp_3a_fw.h"
#include "isp_dev_access.h"
#include "isp_3a_adpt.h"
/**************************************** MACRO DEFINE *****************************************/
#define ISP_MW_FILE_NAME_LEN          100


/************************************* INTERNAL DATA TYPE ***************************************/
struct isp_mw_tunng_file_info {
	void *isp_3a_addr;
	void *isp_shading_addr;
	cmr_u32 isp_3a_size;
	cmr_u32 isp_shading_size;
	void *ae_tuning_addr;
	void *awb_tuning_addr;
	void *af_tuning_addr;
	void *shading_addr;
	void *irp_addr;
	struct bin2_sep_info isp_dev_bin_info;
};

struct isp_mw_context {
	cmr_u32 camera_id;
	cmr_u32 is_inited;
	cmr_handle caller_handle;
	cmr_handle isp_3a_handle;
	cmr_handle isp_dev_handle;
	cmr_malloc alloc_cb;
	cmr_free   free_cb;
	proc_callback caller_callback;
	struct isp_init_param input_param;
	struct isp_mw_tunng_file_info tuning_bin;
};

static cmr_int ispmw_create_thread(cmr_handle isp_mw_handle);
static void ispmw_dev_evt_cb(cmr_int evt, void* data, void* privdata);
/*************************************INTERNAK FUNCTION ***************************************/
cmr_int ispmw_create_thread(cmr_handle isp_mw_handle)
{
	cmr_int                                     ret = ISP_SUCCESS;

	return ret;
}


void ispmw_dev_evt_cb(cmr_int evt, void* data, void* privdata)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp_mw_context                       *cxt = (struct isp_mw_context*)privdata;

	if (!privdata) {
		ISP_LOGE("error,handle is NULL");
		goto exit;
	}

	switch (evt) {
	case ISP3A_DEV_STATICS_DONE:
	case ISP3A_DEV_SENSOR_SOF:
		ret = isp_3a_fw_receive_data(cxt->isp_3a_handle, evt, data);
		break;
	default:
		break;
	}
exit:
	ISP_LOGI("done");
}
cmr_int ispmw_parse_tuning_bin(cmr_handle isp_mw_handle)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp_mw_context                       *cxt = (struct isp_mw_context*)isp_mw_handle;

	cxt->tuning_bin.ae_tuning_addr = NULL;
	cxt->tuning_bin.awb_tuning_addr = NULL;
	cxt->tuning_bin.af_tuning_addr = NULL;
	if (cxt->tuning_bin.isp_3a_addr
		&& (0 != cxt->tuning_bin.isp_3a_size)) {
		ret = isp_separate_3a_bin(cxt->tuning_bin.isp_3a_addr,
								&cxt->tuning_bin.ae_tuning_addr,
								&cxt->tuning_bin.awb_tuning_addr,
								&cxt->tuning_bin.af_tuning_addr);
		ISP_LOGI("ae bin %p, awb bin %p, af bin %p",
				cxt->tuning_bin.ae_tuning_addr, cxt->tuning_bin.awb_tuning_addr, cxt->tuning_bin.af_tuning_addr);
	}
	if (cxt->tuning_bin.isp_shading_addr
		&& (0 != cxt->tuning_bin.isp_shading_size)) {
		ret = isp_separate_drv_bin_2(cxt->tuning_bin.isp_shading_addr,
								cxt->tuning_bin.isp_shading_size,
								&cxt->tuning_bin.isp_dev_bin_info);
		ISP_LOGI("shading bin %p size %d, irp bin %p size %d",
				cxt->tuning_bin.isp_dev_bin_info.puc_shading_bin_addr,cxt->tuning_bin.isp_dev_bin_info.uw_shading_bin_size,
				cxt->tuning_bin.isp_dev_bin_info.puc_irp_bin_addr, cxt->tuning_bin.isp_dev_bin_info.uw_irp_bin_size);
	}
	return ret;
}

cmr_int ispmw_get_tuning_bin(cmr_handle isp_mw_handle, const cmr_s8 *sensor_name)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp_mw_context                       *cxt = (struct isp_mw_context*)isp_mw_handle;
	FILE                                        *fp = NULL;
	cmr_u8                                      file_name[ISP_MW_FILE_NAME_LEN];

	//get 3A bin
	sprintf((void*)&file_name[0],"/system/lib/tuning/%s_3a.bin",sensor_name);
	fp = fopen((void*)&file_name[0], "rb");
	if (NULL == fp) {
		ISP_LOGE("failed to open 3a tuning bin");
		goto exit;
	}
	ISP_LOGI("sensor is %s", sensor_name);
	fseek(fp,0,SEEK_END);
	cxt->tuning_bin.isp_3a_size = ftell(fp);
	if (0 == cxt->tuning_bin.isp_3a_size) {
		fclose(fp);
		goto exit;
	}
	fseek(fp,0,SEEK_SET);
	cxt->tuning_bin.isp_3a_addr = malloc(cxt->tuning_bin.isp_3a_size);
	if (NULL == cxt->tuning_bin.isp_3a_addr) {
		fclose(fp);
		ISP_LOGE("failed to malloc");
		goto exit;
	}
	if (cxt->tuning_bin.isp_3a_size != fread(cxt->tuning_bin.isp_3a_addr, 1, cxt->tuning_bin.isp_3a_size, fp)){
		fclose(fp);
		CMR_LOGE("failed to read 3a bin");
		goto exit;
	}
	fclose(fp);

	memset(&file_name[0], 0, ISP_MW_FILE_NAME_LEN);

	//get Shading bin
	sprintf((void*)&file_name[0],"/system/lib/tuning/%s_shading.bin",sensor_name);
	fp = fopen((void*)&file_name[0], "rb");
	if (NULL == fp) {
		ISP_LOGE("failed to open shading bin");
		goto exit;
	}
	fseek(fp,0,SEEK_END);
	cxt->tuning_bin.isp_shading_size = ftell(fp);
	if (0 == cxt->tuning_bin.isp_shading_size) {
		fclose(fp);
		goto exit;
	}
	fseek(fp,0,SEEK_SET);
	cxt->tuning_bin.isp_shading_addr = malloc(cxt->tuning_bin.isp_shading_size);
	if (NULL == cxt->tuning_bin.isp_shading_addr) {
		fclose(fp);
		ISP_LOGE("failed to malloc");
		goto exit;
	}
	if (cxt->tuning_bin.isp_shading_size != fread(cxt->tuning_bin.isp_shading_addr, 1, cxt->tuning_bin.isp_shading_size, fp)){
		fclose(fp);
		CMR_LOGE("failed to read shading bin");
		goto exit;
	}
	fclose(fp);
exit:
	if (ret) {
		if (cxt->tuning_bin.isp_3a_addr) {
			free(cxt->tuning_bin.isp_3a_addr);
			cxt->tuning_bin.isp_3a_addr = NULL;
			cxt->tuning_bin.isp_3a_size = 0;
		}
		if (cxt->tuning_bin.isp_shading_addr) {
			free(cxt->tuning_bin.isp_shading_addr);
			cxt->tuning_bin.isp_shading_addr = NULL;
			cxt->tuning_bin.isp_shading_size = 0;
		}
	} else {
		ISP_LOGI("3a bin size = %d, shading bin size = %d", cxt->tuning_bin.isp_3a_size, cxt->tuning_bin.isp_shading_size);
	}
	return ret;
}

cmr_int ispmw_put_tuning_bin(cmr_handle isp_mw_handle)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp_mw_context                       *cxt = (struct isp_mw_context*)isp_mw_handle;

	if (cxt->tuning_bin.isp_3a_addr) {
		free(cxt->tuning_bin.isp_3a_addr);
		cxt->tuning_bin.isp_3a_addr = NULL;
		cxt->tuning_bin.isp_shading_size = 0;
	}
	if (cxt->tuning_bin.isp_shading_addr) {
		free(cxt->tuning_bin.isp_shading_addr);
		cxt->tuning_bin.isp_shading_addr = NULL;
		cxt->tuning_bin.isp_shading_size = 0;
	}
	return ret;
}
/*************************************EXTERNAL FUNCTION ***************************************/
cmr_int isp_init(struct isp_init_param *input_ptr, cmr_handle *isp_handle)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp_3a_fw_init_in                    isp3a_input;
	struct isp_dev_init_in                      isp_dev_input;
	struct isp_mw_context                       *cxt = NULL;

	if (!input_ptr || !isp_handle) {
		ISP_LOGE("init param is null,input_ptr is 0x%lx,isp_handle is 0x%lx", (cmr_uint)input_ptr, (cmr_uint)isp_handle);
		ret = ISP_PARAM_NULL;
		goto exit;
	}

	*isp_handle = NULL;
	cxt = (struct isp_mw_context*)malloc(sizeof(struct isp_mw_context));
	if (NULL == cxt) {
		ISP_LOGE("failed to malloc");
		ret = ISP_ALLOC_ERROR;
		goto exit;
	}
	cmr_bzero(cxt, sizeof(*cxt));

	ret = ispmw_get_tuning_bin((cmr_handle)cxt,(const cmr_s8*)input_ptr->ex_info.name);
	if (ret) {
		goto exit;
	}
	ret = ispmw_parse_tuning_bin((cmr_handle)cxt);
	if (ret) {
		goto exit;
	}
	cxt->camera_id = input_ptr->camera_id;
	cxt->caller_handle = input_ptr->oem_handle;
	cxt->caller_callback = input_ptr->ctrl_callback;
	cxt->alloc_cb = input_ptr->alloc_cb;
	cxt->free_cb = input_ptr->free_cb;

	cmr_bzero(&isp_dev_input, sizeof(isp_dev_input));
	isp_dev_input.camera_id = input_ptr->camera_id;
	isp_dev_input.caller_handle = (cmr_handle)cxt;
	isp_dev_input.mem_cb_handle = input_ptr->oem_handle;
	memcpy(&isp_dev_input.init_param, input_ptr, sizeof(struct isp_init_param));
	ret = isp_dev_access_init(&isp_dev_input, &cxt->isp_dev_handle);
	if (ret) {
		goto exit;
	}
	cmr_bzero(&isp3a_input, sizeof(isp3a_input));
	isp3a_input.setting_param_ptr = input_ptr->setting_param_ptr;
	isp3a_input.isp_mw_callback = input_ptr->ctrl_callback;
	isp3a_input.caller_handle = input_ptr->oem_handle;
	isp3a_input.dev_access_handle = cxt->isp_dev_handle;
	isp3a_input.camera_id = input_ptr->camera_id;
	isp3a_input.bin_info.ae_addr = cxt->tuning_bin.ae_tuning_addr;
	isp3a_input.bin_info.awb_addr = cxt->tuning_bin.awb_tuning_addr;
	isp3a_input.bin_info.af_addr = cxt->tuning_bin.af_tuning_addr;
	isp3a_input.ops = input_ptr->ops;
	isp3a_input.bin_info.isp_3a_addr = cxt->tuning_bin.isp_3a_addr;
	isp3a_input.bin_info.isp_3a_size = cxt->tuning_bin.isp_3a_size;
	isp3a_input.bin_info.isp_shading_addr = cxt->tuning_bin.isp_shading_addr;
	isp3a_input.bin_info.isp_shading_size = cxt->tuning_bin.isp_shading_size;
	isp3a_input.ex_info = input_ptr->ex_info;
	isp3a_input.otp_data = input_ptr->otp_data;
	ret = isp_3a_fw_init(&isp3a_input, &cxt->isp_3a_handle);
exit:
	if (ret) {
		if (cxt) {
			ispmw_put_tuning_bin((cmr_handle)cxt);
			isp_3a_fw_deinit(cxt->isp_3a_handle);
			isp_dev_access_deinit(cxt->isp_dev_handle);
			free((void*)cxt);
		}
	} else {
		cxt->is_inited = 1;
		*isp_handle = (cmr_handle)cxt;
	}
	ISP_LOGI("done %ld",ret);
	return ret;;
}

cmr_int isp_deinit(cmr_handle isp_handle)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp_mw_context                       *cxt = (struct isp_mw_context *)isp_handle;

	ISP_CHECK_HANDLE_VALID(isp_handle);

	isp_3a_fw_deinit(cxt->isp_3a_handle);
	isp_dev_access_deinit(cxt->isp_dev_handle);
	ispmw_put_tuning_bin((cmr_handle)cxt);
	free((void*)cxt);

	return ret;
}

cmr_int isp_capability(cmr_handle isp_handle, enum isp_capbility_cmd cmd, void *param_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp_mw_context                       *cxt = (struct isp_mw_context *)isp_handle;

	ISP_CHECK_HANDLE_VALID(isp_handle);

	if (!param_ptr) {
		ISP_LOGE("input is NULL");
		ret = ISP_PARAM_NULL;
		goto exit;
	}
	ISP_LOGI("CMD 0x%x", cmd);
	switch (cmd) {
	case ISP_VIDEO_SIZE:
	case ISP_CAPTURE_SIZE:
	case ISP_REG_VAL:
		ret = isp_dev_access_capability(cxt->isp_dev_handle, cmd, param_ptr);
		break;
	case ISP_LOW_LUX_EB:
	case ISP_CUR_ISO:
	case ISP_CTRL_GET_AE_LUM:
		ret = isp_3a_fw_capability(cxt->isp_3a_handle, cmd, param_ptr);
		break;
	case ISP_DENOISE_LEVEL:
	case ISP_DENOISE_INFO:
	default:
		break;
	}
exit:
	return ret;
}

cmr_int isp_ioctl(cmr_handle isp_handle, enum isp_ctrl_cmd cmd, void *param_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp_mw_context                       *cxt = (struct isp_mw_context *)isp_handle;

	ISP_CHECK_HANDLE_VALID(isp_handle);

	ret = isp_3a_fw_ioctl(cxt->isp_3a_handle, cmd, param_ptr);

	return ret;
}

cmr_int isp_video_start(cmr_handle isp_handle, struct isp_video_start *param_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp_mw_context                       *cxt = (struct isp_mw_context *)isp_handle;
	struct isp_3a_cfg_param                     cfg_param;
	struct isp_3a_get_dld_in                    dld_in;
	struct isp_3a_dld_sequence                  dld_seq;
	struct isp_dev_access_start_in              dev_in;

	ISP_CHECK_HANDLE_VALID(isp_handle);

	if (NULL == param_ptr) {
		ISP_LOGE("error,input is NULL");
		goto exit;
	}
	ret = isp_3a_fw_start(cxt->isp_3a_handle, param_ptr);
	if (ret) {
		ISP_LOGE("failed to start 3a");
		goto exit;
	}
	ret = isp_3a_fw_get_cfg(cxt->isp_3a_handle, &dev_in.hw_cfg);
	if (ret) {
		ISP_LOGE("failed to get cfg");
		goto stop_exit;
	}
	dld_in.op_mode = ISP3A_OPMODE_NORMALLV;
	ret = isp_3a_fw_get_dldseq(cxt->isp_3a_handle, &dld_in, &dev_in.dld_seq);
	if (ret) {
		ISP_LOGE("failed to get dldseq");
		goto stop_exit;
	}

	dev_in.common_in = *param_ptr;
	ret = isp_dev_access_start_multiframe(cxt->isp_dev_handle, &dev_in);
	if (ISP_SUCCESS == ret) {
		goto exit;
	}
stop_exit:
	isp_3a_fw_stop(cxt->isp_3a_handle);
exit:
	ISP_LOGI("done %ld", ret);
	return ret;
}

cmr_int isp_video_stop(cmr_handle isp_handle)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp_mw_context                       *cxt = (struct isp_mw_context *)isp_handle;

	ISP_CHECK_HANDLE_VALID(isp_handle);

	ret = isp_dev_access_stop_multiframe(cxt->isp_dev_handle);
	ret = isp_3a_fw_stop(cxt->isp_3a_handle);

	return ret;
}

cmr_int isp_proc_start(cmr_handle isp_handle, struct ips_in_param *input_ptr, struct ips_out_param *output_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp_mw_context                       *cxt = (struct isp_mw_context *)isp_handle;
	struct isp_dev_postproc_in                  dev_in;
	struct isp_dev_postproc_out                 dev_out;
	struct isp_3a_get_dld_in                    dld_in;

	ISP_LOGE("isp_proc_start");
	ret = isp_3a_fw_get_cfg(cxt->isp_3a_handle, &dev_in.hw_cfg);
	if (ret) {
		ISP_LOGE("failed to get cfg");
		goto exit;
	}
	dld_in.op_mode = ISP3A_OPMODE_NORMALLV;
	ret = isp_3a_fw_get_dldseq(cxt->isp_3a_handle, &dld_in, &dev_in.dldseq);
	if (ret) {
		ISP_LOGE("failed to get dldseq");
		goto exit;
	}
	ret = isp_3a_fw_get_awb_gain(cxt->isp_3a_handle, &dev_in.awb_gain, &dev_in.awb_gain_balanced);
	if (ret) {
		ISP_LOGE("failed to get awb gain");
		goto  exit;
	}

	memcpy(&dev_in.src_frame, &input_ptr->src_frame, sizeof(struct isp_img_frm));
	memcpy(&dev_in.dst_frame, &input_ptr->dst_frame, sizeof(struct isp_img_frm));
	memcpy(&dev_in.dst2_frame, &input_ptr->dst2_frame, sizeof(struct isp_img_frm));
	ret = isp_dev_access_start_postproc(cxt->isp_dev_handle, &dev_in, &dev_out);
exit:
	return ret;
}

cmr_int isp_proc_next(cmr_handle isp_handle, struct ipn_in_param *input_ptr, struct ips_out_param *output_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp_mw_context                       *cxt = (struct isp_mw_context *)isp_handle;

	return ret;
}
