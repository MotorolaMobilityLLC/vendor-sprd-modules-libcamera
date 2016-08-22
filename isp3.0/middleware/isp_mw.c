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
	void *isp_second_3a_addr;
	void *isp_shading_addr;
	cmr_u32 isp_3a_size;
	cmr_u32 isp_second_3a_size;
	cmr_u32 isp_shading_size;
	void *ae_tuning_addr;
	void *awb_tuning_addr;
	void *af_tuning_addr;
	void *second_ae_tuning_addr;
	void *second_awb_tuning_addr;
	void *second_af_tuning_addr;
	void *shading_addr;
	void *irp_addr;
	struct bin2_sep_info isp_dev_bin_info;
	void *isp_caf_addr;
	cmr_u32 isp_caf_size;
};

struct isp_mw_pdaf_info {
	void *pdaf_cbc_addr;
	cmr_u32 pdaf_cbc_size;
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
	struct isp_mw_pdaf_info pdaf_info;
	struct isp_mw_tunng_file_info tuning_bin_slv;
};

static cmr_int ispmw_create_thread(cmr_handle isp_mw_handle);
static void ispmw_dev_evt_cb(cmr_int evt, void *data, void *privdata);
/*************************************INTERNAK FUNCTION ***************************************/
cmr_int ispmw_create_thread(cmr_handle isp_mw_handle)
{
	cmr_int                                     ret = ISP_SUCCESS;

	UNUSED(isp_mw_handle);
	return ret;
}

void ispmw_dev_evt_cb(cmr_int evt, void *data, void *privdata)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp_mw_context                       *cxt = (struct isp_mw_context *)privdata;

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

void ispmw_dev_buf_cfg_evt_cb(cmr_handle isp_handle, isp_buf_cfg_evt_cb grab_event_cb)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp_mw_context                       *cxt = (struct isp_mw_context *)isp_handle;

	isp_dev_access_cfg_buf_evt_reg(cxt->isp_dev_handle, cxt->caller_handle, grab_event_cb);

}

cmr_int ispmw_parse_tuning_bin(cmr_handle isp_mw_handle)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp_mw_context                       *cxt = (struct isp_mw_context *)isp_mw_handle;

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
				cxt->tuning_bin.isp_dev_bin_info.puc_shading_bin_addr,
				cxt->tuning_bin.isp_dev_bin_info.uw_shading_bin_size,
				cxt->tuning_bin.isp_dev_bin_info.puc_irp_bin_addr,
				cxt->tuning_bin.isp_dev_bin_info.uw_irp_bin_size);
	}
	return ret;
}

cmr_int ispmw_get_tuning_bin(cmr_handle isp_mw_handle, const cmr_s8 *sensor_name)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp_mw_context                       *cxt = (struct isp_mw_context *)isp_mw_handle;
	FILE                                        *fp = NULL;
	cmr_u8                                      file_name[ISP_MW_FILE_NAME_LEN];

	/* get 3A bin */
	ISP_LOGI("sensor_name %s", sensor_name);
	sprintf((void *)&file_name[0], "/system/lib/tuning/%s_3a.bin", sensor_name);
	fp = fopen((void *)&file_name[0], "rb");
	if (NULL == fp) {
		ISP_LOGE("failed to open 3a tuning bin");
		ret = -ISP_ERROR;
		goto exit;
	}
	ISP_LOGV("sensor is %s", sensor_name);
	fseek(fp, 0, SEEK_END);
	cxt->tuning_bin.isp_3a_size = ftell(fp);
	if (0 == cxt->tuning_bin.isp_3a_size) {
		fclose(fp);
		ret = -ISP_ERROR;
		goto exit;
	}
	fseek(fp, 0, SEEK_SET);
	cxt->tuning_bin.isp_3a_addr = malloc(cxt->tuning_bin.isp_3a_size);
	if (NULL == cxt->tuning_bin.isp_3a_addr) {
		fclose(fp);
		ISP_LOGE("failed to malloc");
		ret = -ISP_ERROR;
		goto exit;
	}
	if (cxt->tuning_bin.isp_3a_size != fread(cxt->tuning_bin.isp_3a_addr, 1, cxt->tuning_bin.isp_3a_size, fp)) {
		fclose(fp);
		ISP_LOGE("failed to read 3a bin");
		ret = -ISP_ERROR;
		goto exit;
	}
	fclose(fp);

	memset(&file_name[0], 0, ISP_MW_FILE_NAME_LEN);

	/* get Shading bin */
	sprintf((void *)&file_name[0], "/system/lib/tuning/%s_shading.bin", sensor_name);
	fp = fopen((void *)&file_name[0], "rb");
	if (NULL == fp) {
		ISP_LOGE("failed to open shading bin");
		ret = -ISP_ERROR;
		goto exit;
	}
	fseek(fp, 0, SEEK_END);
	cxt->tuning_bin.isp_shading_size = ftell(fp);
	if (0 == cxt->tuning_bin.isp_shading_size) {
		fclose(fp);
		ret = -ISP_ERROR;
		goto exit;
	}
	fseek(fp, 0, SEEK_SET);
	cxt->tuning_bin.isp_shading_addr = malloc(cxt->tuning_bin.isp_shading_size);
	if (NULL == cxt->tuning_bin.isp_shading_addr) {
		fclose(fp);
		ISP_LOGE("failed to malloc");
		ret = -ISP_ERROR;
		goto exit;
	}
	if (cxt->tuning_bin.isp_shading_size != fread(cxt->tuning_bin.isp_shading_addr, 1, cxt->tuning_bin.isp_shading_size, fp)) {
		fclose(fp);
		ISP_LOGE("failed to read shading bin");
		ret = -ISP_ERROR;
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
		ISP_LOGI("3a bin size = %d, shading bin size = %d",
			cxt->tuning_bin.isp_3a_size, cxt->tuning_bin.isp_shading_size);
	}
	return ret;
}

#ifdef CONFIG_CAMERA_DUAL_SYNC
cmr_int ispmw_parse_tuning_bin_slv(cmr_handle isp_mw_handle)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp_mw_context                       *cxt = (struct isp_mw_context *)isp_mw_handle;

	cxt->tuning_bin_slv.ae_tuning_addr = NULL;
	cxt->tuning_bin_slv.awb_tuning_addr = NULL;
	cxt->tuning_bin_slv.af_tuning_addr = NULL;
	if (cxt->tuning_bin_slv.isp_3a_addr
		&& (0 != cxt->tuning_bin_slv.isp_3a_size)) {
		ret = isp_separate_3a_bin(cxt->tuning_bin_slv.isp_3a_addr,
								&cxt->tuning_bin_slv.ae_tuning_addr,
								&cxt->tuning_bin_slv.awb_tuning_addr,
								&cxt->tuning_bin_slv.af_tuning_addr);
		ISP_LOGI("ae bin %p, awb bin %p, af bin %p",
				cxt->tuning_bin_slv.ae_tuning_addr, cxt->tuning_bin_slv.awb_tuning_addr, cxt->tuning_bin_slv.af_tuning_addr);
	}

	return ret;
}

cmr_int ispmw_get_tuning_bin_slv(cmr_handle isp_mw_handle, const cmr_s8 *sensor_name)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp_mw_context                       *cxt = (struct isp_mw_context *)isp_mw_handle;
	FILE                                        *fp = NULL;
	cmr_u8                                      file_name[ISP_MW_FILE_NAME_LEN];

	/* get 3A bin */
	ISP_LOGI("sensor_name %s", sensor_name);
	sprintf((void *)&file_name[0], "/system/lib/tuning/%s_3a.bin", sensor_name);
	fp = fopen((void *)&file_name[0], "rb");
	if (NULL == fp) {
		ISP_LOGE("failed to open 3a tuning bin");
		ret = -ISP_ERROR;
		goto exit;
	}
	ISP_LOGV("sensor is %s", sensor_name);
	fseek(fp, 0, SEEK_END);
	cxt->tuning_bin_slv.isp_3a_size = ftell(fp);
	if (0 == cxt->tuning_bin_slv.isp_3a_size) {
		fclose(fp);
		ret = -ISP_ERROR;
		goto exit;
	}
	fseek(fp, 0, SEEK_SET);
	cxt->tuning_bin_slv.isp_3a_addr = malloc(cxt->tuning_bin_slv.isp_3a_size);
	if (NULL == cxt->tuning_bin_slv.isp_3a_addr) {
		fclose(fp);
		ISP_LOGE("failed to malloc");
		ret = -ISP_ERROR;
		goto exit;
	}
	if (cxt->tuning_bin_slv.isp_3a_size != fread(cxt->tuning_bin_slv.isp_3a_addr, 1, cxt->tuning_bin_slv.isp_3a_size, fp)) {
		fclose(fp);
		ISP_LOGE("failed to read 3a bin");
		ret = -ISP_ERROR;
		goto exit;
	}
	fclose(fp);
exit:
	if (ret) {
		if (cxt->tuning_bin_slv.isp_3a_addr) {
			free(cxt->tuning_bin_slv.isp_3a_addr);
			cxt->tuning_bin_slv.isp_3a_addr = NULL;
			cxt->tuning_bin_slv.isp_3a_size = 0;
		}
	} else {
		ISP_LOGI("3a bin size = %d", cxt->tuning_bin_slv.isp_3a_size);
	}
	return ret;
}
#endif
cmr_int ispmw_get_caf_tuning_bin(cmr_handle isp_mw_handle, const cmr_s8 *sensor_name)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp_mw_context                       *cxt = (struct isp_mw_context *)isp_mw_handle;
	FILE                                        *fp = NULL;
	cmr_u8                                      file_name[ISP_MW_FILE_NAME_LEN];

	/* get caf tuning bin */
	sprintf((void *)&file_name[0], "/system/lib/tuning/%s_caf.bin", sensor_name);
	fp = fopen((void *)&file_name[0], "rb");
	if (NULL == fp) {
		ISP_LOGE("failed to open caf bin");
		goto exit;
	}
	fseek(fp, 0, SEEK_END);
	cxt->tuning_bin.isp_caf_size = ftell(fp);
	if (0 == cxt->tuning_bin.isp_caf_size) {
		fclose(fp);
		goto exit;
	}
	fseek(fp, 0, SEEK_SET);
	cxt->tuning_bin.isp_caf_addr = malloc(cxt->tuning_bin.isp_caf_size);
	if (NULL == cxt->tuning_bin.isp_caf_addr) {
		fclose(fp);
		ISP_LOGE("failed to malloc");
		goto exit;
	}
	if (cxt->tuning_bin.isp_caf_size != fread(cxt->tuning_bin.isp_caf_addr, 1, cxt->tuning_bin.isp_caf_size, fp)) {
		fclose(fp);
		ISP_LOGE("failed to read caf bin");
		ret = -ISP_ERROR;
		goto exit;
	}
	fclose(fp);
exit:
	if (ret) {
		if (cxt->tuning_bin.isp_caf_addr) {
			free(cxt->tuning_bin.isp_caf_addr);
			cxt->tuning_bin.isp_caf_addr = NULL;
			cxt->tuning_bin.isp_caf_size = 0;
		}
	} else {
		ISP_LOGI("3a bin size = %p, caf bin size = %d",
			cxt->tuning_bin.isp_caf_addr, cxt->tuning_bin.isp_caf_size);
	}

	return ret;
}

cmr_int ispmw_get_pdaf_cbc_bin(cmr_handle isp_mw_handle, const cmr_s8 *sensor_name)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp_mw_context                       *cxt = (struct isp_mw_context *)isp_mw_handle;
	FILE                                        *fp = NULL;
	cmr_u8                                      file_name[ISP_MW_FILE_NAME_LEN];

	/* get caf tuning bin */
	sprintf((void *)&file_name[0], "/system/lib/tuning/%s_cbc.bin", sensor_name);
	fp = fopen((void *)&file_name[0], "rb");
	if (NULL == fp) {
		ISP_LOGE("failed to open caf bin");
		goto exit;
	}
	fseek(fp, 0, SEEK_END);
	cxt->pdaf_info.pdaf_cbc_size = ftell(fp);
	if (0 == cxt->pdaf_info.pdaf_cbc_size) {
		fclose(fp);
		goto exit;
	}
	fseek(fp, 0, SEEK_SET);
	cxt->pdaf_info.pdaf_cbc_addr = malloc(cxt->pdaf_info.pdaf_cbc_size);
	if (NULL == cxt->pdaf_info.pdaf_cbc_addr) {
		fclose(fp);
		ISP_LOGE("failed to malloc");
		goto exit;
	}
	if (cxt->pdaf_info.pdaf_cbc_size != fread(cxt->pdaf_info.pdaf_cbc_addr,
						  1,
						  cxt->pdaf_info.pdaf_cbc_size, fp)) {
		fclose(fp);
		ISP_LOGE("failed to read caf bin");
		ret = -ISP_ERROR;
		goto exit;
	}
	fclose(fp);
exit:
	if (ret) {
		if (cxt->pdaf_info.pdaf_cbc_addr) {
			free(cxt->pdaf_info.pdaf_cbc_addr);
			cxt->pdaf_info.pdaf_cbc_addr = NULL;
			cxt->pdaf_info.pdaf_cbc_size = 0;
		}
	} else {
		ISP_LOGI("pdaf_cbc_addr = %p, size = %d",
			cxt->pdaf_info.pdaf_cbc_addr, cxt->pdaf_info.pdaf_cbc_size);
	}

	return ret;
}

cmr_int ispmw_put_pdaf_cbc_bin(cmr_handle isp_mw_handle)
{
	struct isp_mw_context                       *cxt = (struct isp_mw_context *)isp_mw_handle;

	if (cxt->pdaf_info.pdaf_cbc_addr) {
		free(cxt->pdaf_info.pdaf_cbc_addr);
		cxt->pdaf_info.pdaf_cbc_addr = NULL;
		cxt->pdaf_info.pdaf_cbc_size = 0;
	}

	return 0;
}

cmr_int ispmw_put_tuning_bin(cmr_handle isp_mw_handle)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp_mw_context                       *cxt = (struct isp_mw_context *)isp_mw_handle;

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
	if (cxt->tuning_bin.isp_caf_addr) {
		free(cxt->tuning_bin.isp_caf_addr);
		cxt->tuning_bin.isp_caf_addr = NULL;
		cxt->tuning_bin.isp_caf_size = 0;
	}
#ifdef CONFIG_CAMERA_DUAL_SYNC
	if (cxt->tuning_bin_slv.isp_3a_addr) {
		free(cxt->tuning_bin_slv.isp_3a_addr);
		cxt->tuning_bin_slv.isp_3a_addr = NULL;
		cxt->tuning_bin_slv.isp_3a_size = 0;
	}
#endif
	return ret;
}

cmr_int ispmw_get_second_tuning_bin(cmr_handle isp_mw_handle, const cmr_s8 *sensor_name)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp_mw_context                       *cxt = (struct isp_mw_context *)isp_mw_handle;
	FILE                                        *fp = NULL;
	cmr_u8                                      file_name[ISP_MW_FILE_NAME_LEN];

	//get 3A bin
	ISP_LOGI("sensor_name %s", sensor_name);
	sprintf((void *)&file_name[0], "/system/lib/tuning/%s_3a.bin", sensor_name);
	fp = fopen((void *)&file_name[0], "rb");
	if (NULL == fp) {
		goto exit;
	}
	ISP_LOGI("sensor is %s", sensor_name);
	fseek(fp, 0, SEEK_END);
	cxt->tuning_bin.isp_second_3a_size = ftell(fp);
	if (0 == cxt->tuning_bin.isp_second_3a_size) {
		fclose(fp);
		goto exit;
	}
	fseek(fp, 0, SEEK_SET);
	cxt->tuning_bin.isp_second_3a_addr = malloc(cxt->tuning_bin.isp_second_3a_size);
	if (NULL == cxt->tuning_bin.isp_second_3a_addr) {
		fclose(fp);
		ISP_LOGE("failed to malloc");
		goto exit;
	}
	if (cxt->tuning_bin.isp_second_3a_size !=
	    fread(cxt->tuning_bin.isp_second_3a_addr, 1, cxt->tuning_bin.isp_second_3a_size, fp)) {
		fclose(fp);
		ISP_LOGE("failed to read 3a bin");
		goto exit;
	}
	fclose(fp);
exit:
	if (ret) {
		if (cxt->tuning_bin.isp_second_3a_addr) {
			free(cxt->tuning_bin.isp_second_3a_addr);
			cxt->tuning_bin.isp_second_3a_addr = NULL;
			cxt->tuning_bin.isp_second_3a_size = 0;
		}
	} else {
		ISP_LOGI("3a bin size = %d, shading bin size = %d", cxt->tuning_bin.isp_second_3a_size, cxt->tuning_bin.isp_shading_size);
	}
	return ret;
}

cmr_int ispmw_put_second_tuning_bin(cmr_handle isp_mw_handle)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp_mw_context                       *cxt = (struct isp_mw_context *)isp_mw_handle;

	if (cxt->tuning_bin.isp_second_3a_addr) {
		free(cxt->tuning_bin.isp_second_3a_addr);
		cxt->tuning_bin.isp_second_3a_addr = NULL;
		cxt->tuning_bin.isp_second_3a_size = 0;
	}
	return ret;
}

cmr_int ispmw_parse_second_tuning_bin(cmr_handle isp_mw_handle)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp_mw_context                       *cxt = (struct isp_mw_context *)isp_mw_handle;

	cxt->tuning_bin.second_ae_tuning_addr = NULL;
	cxt->tuning_bin.second_awb_tuning_addr = NULL;
	cxt->tuning_bin.second_af_tuning_addr = NULL;
	if (cxt->tuning_bin.isp_second_3a_addr
		&& (0 != cxt->tuning_bin.isp_second_3a_size)) {
		ret = isp_separate_3a_bin(cxt->tuning_bin.isp_second_3a_addr,
								&cxt->tuning_bin.second_ae_tuning_addr,
								&cxt->tuning_bin.second_awb_tuning_addr,
								&cxt->tuning_bin.second_af_tuning_addr);
		ISP_LOGI("ae bin %p, awb bin %p, af bin %p",
				cxt->tuning_bin.second_ae_tuning_addr, cxt->tuning_bin.second_awb_tuning_addr, cxt->tuning_bin.second_af_tuning_addr);
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

	isp_init_log_level();

	if (!input_ptr || !isp_handle) {
		ISP_LOGE("init param is null,input_ptr is 0x%lx,isp_handle is 0x%lx", (cmr_uint)input_ptr, (cmr_uint)isp_handle);
		ret = ISP_PARAM_NULL;
		goto exit;
	}

	*isp_handle = NULL;
	cxt = (struct isp_mw_context *)malloc(sizeof(struct isp_mw_context));
	if (NULL == cxt) {
		ISP_LOGE("failed to malloc");
		ret = ISP_ALLOC_ERROR;
		goto exit;
	}
	cmr_bzero(cxt, sizeof(*cxt));

	ret = ispmw_get_tuning_bin((cmr_handle)cxt, (const cmr_s8 *)input_ptr->ex_info.name);
	if (ret) {
		goto exit;
	}
	if (input_ptr->ex_info.af_supported) {
		ret = ispmw_get_caf_tuning_bin((cmr_handle)cxt, (const cmr_s8 *)input_ptr->ex_info.name);
		if (ret) {
			ISP_LOGE("failed to get caf tuning bin error");
			goto exit;
		}
		if (input_ptr->ex_info.pdaf_supported) {
			ret = ispmw_get_pdaf_cbc_bin((cmr_handle)cxt, (const cmr_s8 *)input_ptr->ex_info.name);
			if (ret) {
				ISP_LOGE("failed to get pdaf cbc bin");
				goto exit;
			}
		}
	}
	ret = ispmw_parse_tuning_bin((cmr_handle)cxt);
	if (ret) {
		goto exit;
	}
#ifdef CONFIG_CAMERA_DUAL_SYNC
	if (input_ptr->is_refocus && 0 == input_ptr->camera_id && input_ptr->ex_info_slv.name) {
		ret = ispmw_get_tuning_bin_slv((cmr_handle)cxt,(const cmr_s8*)input_ptr->ex_info_slv.name);
		if (ret) {
			goto exit;
		}
		ret = ispmw_parse_tuning_bin_slv((cmr_handle)cxt);
		if (ret) {
			goto exit;
		}
	}
#endif
	cxt->camera_id = input_ptr->camera_id;
	cxt->caller_handle = input_ptr->oem_handle;
	cxt->caller_callback = input_ptr->ctrl_callback;
	cxt->alloc_cb = input_ptr->alloc_cb;
	cxt->free_cb = input_ptr->free_cb;

	cmr_bzero(&isp_dev_input, sizeof(isp_dev_input));
	isp_dev_input.camera_id = input_ptr->camera_id;
	isp_dev_input.caller_handle = (cmr_handle)cxt;
	isp_dev_input.mem_cb_handle = input_ptr->oem_handle;
	isp_dev_input.shading_bin_addr = cxt->tuning_bin.isp_dev_bin_info.puc_shading_bin_addr;
	isp_dev_input.shading_bin_size = cxt->tuning_bin.isp_dev_bin_info.uw_shading_bin_size;
	isp_dev_input.irp_bin_addr = cxt->tuning_bin.isp_dev_bin_info.puc_irp_bin_addr;
	isp_dev_input.irp_bin_size = cxt->tuning_bin.isp_dev_bin_info.uw_irp_bin_size;
	isp_dev_input.pdaf_cbcp_bin_addr = cxt->pdaf_info.pdaf_cbc_addr;
	isp_dev_input.pdaf_cbc_bin_size = cxt->pdaf_info.pdaf_cbc_size;
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
	isp3a_input.bin_info.isp_caf_addr = cxt->tuning_bin.isp_caf_addr;
	isp3a_input.bin_info.isp_caf_size = cxt->tuning_bin.isp_caf_size;
	isp3a_input.ex_info = input_ptr->ex_info;
	isp3a_input.otp_data = input_ptr->otp_data;
	isp3a_input.pdaf_info = input_ptr->pdaf_info;

#ifdef CONFIG_CAMERA_DUAL_SYNC
	isp3a_input.is_refocus = input_ptr->is_refocus;
	//isp3a_input.bin_info_slv.ae_addr = cxt->tuning_bin_slv.ae_tuning_addr;
	isp3a_input.bin_info_slv.awb_addr = cxt->tuning_bin_slv.awb_tuning_addr;
	//isp3a_input.bin_info_slv.af_addr = cxt->tuning_bin_slv.af_tuning_addr;
	isp3a_input.bin_info_slv.isp_3a_addr = cxt->tuning_bin_slv.isp_3a_addr;
	isp3a_input.bin_info_slv.isp_3a_size = cxt->tuning_bin_slv.isp_3a_size;
	isp3a_input.dual_otp = input_ptr->dual_otp;
	isp3a_input.setting_param_ptr_slv = input_ptr->setting_param_ptr_slv;
	isp3a_input.ex_info_slv = input_ptr->ex_info_slv;
#endif
	if (cxt->tuning_bin.isp_dev_bin_info.puc_shading_bin_addr
		&& cxt->tuning_bin.isp_dev_bin_info.uw_shading_bin_size >= (114 + sizeof(struct sensor_otp_iso_awb_info))) {
		/*for bin otp data: shading addr offset +114*/
		isp3a_input.bin_info.otp_data_addr = (struct sensor_otp_iso_awb_info *)(cxt->tuning_bin.isp_dev_bin_info.puc_shading_bin_addr + 114);
		ISP_LOGV("bin otp data iso=%d, r=%d,g=%d,b=%d",
			 isp3a_input.bin_info.otp_data_addr->iso,
			 isp3a_input.bin_info.otp_data_addr->gain_r,
			 isp3a_input.bin_info.otp_data_addr->gain_g,
			 isp3a_input.bin_info.otp_data_addr->gain_b);
	}
	ret = isp_3a_fw_init(&isp3a_input, &cxt->isp_3a_handle);
exit:
	if (ret) {
		if (cxt) {
			ispmw_put_tuning_bin(cxt);
			ispmw_put_pdaf_cbc_bin(cxt);
			ret = isp_dev_access_deinit(cxt->isp_dev_handle);
			if (ret)
				ISP_LOGE("isp_dev_access_deinit fail %ld", ret);
			ret = isp_3a_fw_deinit(cxt->isp_3a_handle);
			if (ret)
				ISP_LOGE("isp_3a_fw_deinit fail %ld", ret);
			free((void *)cxt);
		}
	} else {
		cxt->is_inited = 1;
		*isp_handle = (cmr_handle)cxt;
	}
	ISP_LOGI("done %ld", ret);
	return ret;
}

cmr_int isp_deinit(cmr_handle isp_handle)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp_mw_context                       *cxt = (struct isp_mw_context *)isp_handle;

	ISP_CHECK_HANDLE_VALID(isp_handle);

	ret = isp_dev_access_deinit(cxt->isp_dev_handle);
	if (ret)
		ISP_LOGE("failed to isp_dev_access_deinit %ld", ret);
	ret = isp_3a_fw_deinit(cxt->isp_3a_handle);
	if (ret)
		ISP_LOGE("failed to isp_3a_fw_deinit %ld", ret);
	ispmw_put_tuning_bin(cxt);
	ispmw_put_pdaf_cbc_bin(cxt);
	free((void *)cxt);

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
	char                                        file_name[128];

	ISP_CHECK_HANDLE_VALID(isp_handle);

	if (NULL == param_ptr) {
		ISP_LOGE("error,input is NULL");
		goto exit;
	}

	ISP_LOGI("isp size:%d,%d", param_ptr->size.w, param_ptr->size.h);
	sprintf(file_name, "imx230_mipi_raw_%d", param_ptr->size.w);
	ret = ispmw_get_second_tuning_bin((cmr_handle)cxt, (const cmr_s8 *)file_name);
	if (ret) {
		goto exit;
	}
	ret = ispmw_parse_second_tuning_bin((cmr_handle)cxt);
	if (ret) {
		goto exit;
	}

	param_ptr->tuning_ae_addr = cxt->tuning_bin.second_ae_tuning_addr;

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
	ret = isp_3a_fw_get_iso_speed(cxt->isp_3a_handle, &dev_in.hw_iso_speed);
	if (ret) {
		ISP_LOGE("failed to get hw_iso_speed");
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
	ispmw_put_second_tuning_bin((cmr_handle)cxt);

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
	UNUSED(output_ptr);
	ret = isp_3a_fw_get_cfg(cxt->isp_3a_handle, &dev_in.hw_cfg);
	if (ret) {
		ISP_LOGE("failed to get cfg");
		goto exit;
	}
	ret = isp_3a_fw_get_iso_speed(cxt->isp_3a_handle, &dev_in.hw_iso_speed);
	if (ret) {
		ISP_LOGE("failed to get hw_iso_speed");
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

	dev_in.cap_mode = input_ptr->cap_mode;
	memcpy(&dev_in.src_frame, &input_ptr->src_frame, sizeof(struct isp_img_frm));
	memcpy(&dev_in.dst_frame, &input_ptr->dst_frame, sizeof(struct isp_img_frm));
	memcpy(&dev_in.dst2_frame, &input_ptr->dst2_frame, sizeof(struct isp_img_frm));
	memcpy(&dev_in.resolution_info, &input_ptr->resolution_info, sizeof(struct isp_sensor_resolution_info));
	ret = isp_dev_access_start_postproc(cxt->isp_dev_handle, &dev_in, &dev_out);
	ret = isp_3a_fw_stop(cxt->isp_3a_handle);
exit:
	return ret;
}

cmr_int isp_proc_next(cmr_handle isp_handle, struct ipn_in_param *input_ptr, struct ips_out_param *output_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp_mw_context                       *cxt = (struct isp_mw_context *)isp_handle;

	UNUSED(input_ptr);
	UNUSED(output_ptr);
	return ret;
}

cmr_int isp_cap_buff_cfg(cmr_handle isp_handle, struct isp_img_param *buf_cfg)
{
	cmr_int                    ret = ISP_SUCCESS;
	struct isp_mw_context      *cxt = (struct isp_mw_context *)isp_handle;
	cmr_u32                  i;
	struct isp_dev_img_param parm;

	memset(&parm, 0, sizeof(struct isp_dev_img_param));

	if (NULL == buf_cfg || NULL == cxt->isp_dev_handle) {
		ISP_LOGE("Para invalid");
		return -1;
	}

	parm.channel_id = buf_cfg->channel_id;
	parm.base_id = buf_cfg->base_id;
	parm.addr.chn0 = buf_cfg->addr.chn0;
	parm.addr.chn1 = buf_cfg->addr.chn1;
	parm.addr.chn2 = buf_cfg->addr.chn2;
	parm.addr_vir.chn0 = buf_cfg->addr_vir.chn0;
	parm.addr_vir.chn1 = buf_cfg->addr_vir.chn1;
	parm.addr_vir.chn2 = buf_cfg->addr_vir.chn2;
	parm.index           = buf_cfg->index;
	parm.is_reserved_buf  = buf_cfg->is_reserved_buf;
	parm.flag         = buf_cfg->flag;
	parm.zsl_private      = buf_cfg->zsl_private;
	parm.img_fd.y      = buf_cfg->img_fd.y;
	parm.img_fd.u      = buf_cfg->img_fd.u;
	parm.img_fd.v      = buf_cfg->img_fd.v;


	ISP_LOGI("fd=0x%x, offset: y=0x%lx, u=0x%lx, v=0x%lx, is_reserved_buf=%d",
		 buf_cfg->img_fd.y,
		 buf_cfg->addr.chn0, buf_cfg->addr.chn1,
		 buf_cfg->addr.chn2, buf_cfg->is_reserved_buf);

	ret = isp_dev_access_cap_buf_cfg(cxt->isp_dev_handle, &parm);

	return ret;
}
