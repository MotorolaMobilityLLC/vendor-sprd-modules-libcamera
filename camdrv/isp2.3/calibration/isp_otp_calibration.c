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
#define LOG_TAG "isp_otp_calibration"
#include "isp_otp_calibration.h"
#include "isp_calibration_lsc.h"
#include "isp_calibration_lsc_internal.h"
#include "random_unpack.h"
#include "random_pack.h"
#include "isp_pm.h"
#include "isp_type.h"
#include "isp_com.h"

#define ISP_CALI_ERROR	0XFF
#define ISP_CALI_SUCCESS 0
#define ISP_CALI_AWB_ID	0x1
#define ISP_CALI_LSC_ID 0x2
#define ISP_GOLDEN_START 0x53505244
#define ISP_VERSION_FLAG 0x5350
#define ISP_GOLDEN_V001	 ((ISP_VERSION_FLAG << 16) | 1)
#define ISP_GOLDEN_LSC_V001	 ((ISP_VERSION_FLAG << 16) | 0x1001)
#define ISP_GOLDEN_AWB_V001	 ((ISP_VERSION_FLAG << 16) | 0x2001)
#define ISP_AWB_BASE_GAIN 	1024

#define ISP_CALI_MIN_SIZE (200 * 1024)

struct golden_header {
	cmr_u32 start;
	cmr_u32 length;
	cmr_u32 version;
	cmr_u32 block_num;
};

struct block_info {
	cmr_u32 id;
	cmr_u32 offset;
	cmr_u32 length;
};

struct isp_cali_data {
	cmr_u32 version;
	cmr_u32 length;
	cmr_u32 block_num;
	struct block_info block[2];
	void *data;
};

static cmr_s32 _golden_parse(struct isp_data_t *golden, struct isp_data_t *lsc, struct isp_data_t *awb)
{
	cmr_u8 *start = PNULL;
	cmr_u8 *end = PNULL;
	struct golden_header *header = PNULL;
	struct block_info *block_info = PNULL;
	cmr_u8 module_index = 0xff;
	cmr_u8 lsc_index = 0xff;
	cmr_u8 awb_index = 0xff;
	cmr_u32 i = 0;

	if (PNULL == golden)
		return ISP_CALI_ERROR;

	if (PNULL == golden->data_ptr || 0 == golden->size)
		return ISP_CALI_ERROR;

	if (PNULL == lsc || PNULL == awb)
		return ISP_CALI_ERROR;

	/*check header */
	header = (struct golden_header *)golden->data_ptr;
	if (golden->size < sizeof(*header))
		return ISP_CALI_ERROR;

	if (ISP_GOLDEN_START != header->start)
		return ISP_CALI_ERROR;

	/*just support V001 now */
	if (ISP_GOLDEN_V001 != header->version)
		return ISP_CALI_ERROR;

	start = (cmr_u8 *) header;
	end = start + header->length;

	block_info = (struct block_info *)((cmr_u8 *) header + sizeof(*header));

	for (i = 0; i < header->block_num; i++) {

		switch (block_info[i].id) {
		case 1:
			/*module info */
			module_index = i;
			break;

		case 2:
			/*lsc info */
			lsc_index = i;
			break;

		case 3:
			/*awb info */
			awb_index = i;
			break;

		default:
			break;
		}
	}

	if (module_index >= header->block_num || lsc_index >= header->block_num || awb_index >= header->block_num)
		return ISP_CALI_ERROR;

	if (start + block_info[module_index].offset + block_info[module_index].length > end)
		return ISP_CALI_ERROR;

	if (start + block_info[lsc_index].offset + block_info[lsc_index].length > end)
		return ISP_CALI_ERROR;

	if (start + block_info[awb_index].offset + block_info[awb_index].length > end)
		return ISP_CALI_ERROR;

	lsc->data_ptr = (void *)&start[block_info[lsc_index].offset];
	lsc->size = block_info[lsc_index].length;

	awb->data_ptr = (void *)&start[block_info[awb_index].offset];
	awb->size = block_info[awb_index].length;

	return ISP_CALI_SUCCESS;
}

static cmr_s32 _cali_lsc(struct isp_data_t *golden, struct isp_data_t *otp, cmr_u32 image_pattern, struct isp_data_t *target_buf, cmr_u32 * size)
{
	cmr_s32 rtn = ISP_CALI_SUCCESS;
	struct isp_calibration_lsc_golden_info lsc_golden_info;
	struct isp_calibration_lsc_calc_in lsc_calc_param;
	struct isp_calibration_lsc_calc_out lsc_calc_result;
	cmr_u32 i = 0;
	cmr_u32 target_buf_size = 0;
	memset(&lsc_golden_info, 0x00, sizeof(lsc_golden_info));
	memset(&lsc_calc_param, 0x00, sizeof(lsc_calc_param));
	memset(&lsc_calc_result, 0x00, sizeof(lsc_calc_result));

	struct isp_cali_lsc_info *lsc_info = PNULL;
	cmr_u32 header_size = 0;
	cmr_u32 data_size = 0;

	if (PNULL == golden || PNULL == otp || PNULL == target_buf || PNULL == size)
		return ISP_CALI_ERROR;

#if 0// OTP must be removed by Qin.wang
	rtn = isp_calibration_lsc_get_golden_info(golden->data_ptr, golden->size, &lsc_golden_info);
	if (ISP_CALI_SUCCESS != rtn)
		return rtn;
#endif
	lsc_info = (struct isp_cali_lsc_info *)target_buf->data_ptr;
	if (PNULL == lsc_info)
		return ISP_CALI_ERROR;

	if (target_buf->size < sizeof(struct isp_cali_lsc_info))
		return ISP_CALI_ERROR;

	header_size = (cmr_u8 *) & lsc_info->data_area - (cmr_u8 *) lsc_info;
	target_buf_size = target_buf->size - header_size;

	lsc_calc_param.golden_data = golden->data_ptr;
	lsc_calc_param.golden_data_size = golden->size;
	lsc_calc_param.otp_data = otp->data_ptr;
	lsc_calc_param.otp_data_size = otp->size;
	lsc_calc_param.img_width = lsc_golden_info.img_width;
	lsc_calc_param.img_height = lsc_golden_info.img_height;
	lsc_calc_param.target_buf = (void *)&lsc_info->data_area;
	lsc_calc_param.target_buf_size = target_buf_size;

	/*convert image bayer pattern to lsc gain pattern */
	switch (image_pattern) {
	case 0:		/*gr */
		lsc_calc_param.lsc_pattern = ISP_CALIBRATION_RGGB;
		break;

	case 1:		/*r */
		lsc_calc_param.lsc_pattern = ISP_CALIBRATION_GRBG;
		break;

	case 2:		/*b */
		lsc_calc_param.lsc_pattern = ISP_CALIBRATION_GBRG;
		break;

	case 3:		/*gb */
		lsc_calc_param.lsc_pattern = ISP_CALIBRATION_BGGR;
		break;

	default:
		return ISP_CALI_ERROR;
	}

#if 0// OTP must be removed by Qin.wang
	rtn = isp_calibration_lsc_calc(&lsc_calc_param, &lsc_calc_result);
	if (ISP_CALI_SUCCESS != rtn)
		return rtn;
#endif
	lsc_info->num = lsc_calc_result.lsc_param_num;
	for (i = 0; i < lsc_info->num; i++) {
		lsc_info->map[i].ct = lsc_calc_result.lsc_param[i].light_ct;
		lsc_info->map[i].width = lsc_calc_result.width;
		lsc_info->map[i].height = lsc_calc_result.height;
		lsc_info->map[i].grid = lsc_calc_result.grid;
		lsc_info->map[i].offset = data_size;
		lsc_info->map[i].len = lsc_calc_result.lsc_param[i].size;

		data_size += lsc_calc_result.lsc_param[i].size;
	}

	*size = data_size + header_size;

	return rtn;
}

static cmr_s32 _cali_awb(struct isp_data_t *golden, struct isp_data_t *otp, struct isp_data_t *target_buf, cmr_u32 * size)
{
	cmr_s32 rtn = ISP_CALI_SUCCESS;
	cmr_u32 version = 0;
	cmr_u8 *start = 0;
	struct isp_cali_awb_info *awb_info = PNULL;

	if (PNULL == golden || PNULL == otp || PNULL == target_buf || PNULL == size)
		return ISP_CALI_ERROR;

	awb_info = (struct isp_cali_awb_info *)target_buf->data_ptr;
	if (PNULL == awb_info)
		return ISP_CALI_ERROR;

	if (target_buf->size < sizeof(struct isp_cali_awb_info))
		return ISP_CALI_ERROR;

	start = (cmr_u8 *) golden->data_ptr;
	version = *(cmr_u32 *) start;
	start += 4;

	if (ISP_GOLDEN_AWB_V001 != version)
		return ISP_CALI_ERROR;

	awb_info->golden_avg[0] = *(cmr_u16 *) start;
	start += 2;
	awb_info->golden_avg[1] = *(cmr_u16 *) start;
	start += 2;
	awb_info->golden_avg[2] = *(cmr_u16 *) start;
	//start += 2;

	start = (cmr_u8 *) otp->data_ptr;
	version = *(cmr_u32 *) start;
	start += 4;
	awb_info->ramdon_avg[0] = *(cmr_u16 *) start;
	start += 2;
	awb_info->ramdon_avg[1] = *(cmr_u16 *) start;
	start += 2;
	awb_info->ramdon_avg[2] = *(cmr_u16 *) start;
	start += 2;

	*size = sizeof(*awb_info);

	return rtn;
}

cmr_s32 isp_calibration_get_info(struct isp_data_t * golden_info, struct isp_cali_info_t * cali_info)
{
	if (PNULL == cali_info || PNULL == golden_info)
		return ISP_CALI_ERROR;

	cali_info->size = 200 * 1024;

	return ISP_CALI_SUCCESS;
}

cmr_s32 isp_calibration(struct isp_cali_param * param, struct isp_data_t * result)
{
	cmr_s32 rtn = ISP_CALI_SUCCESS;
	struct isp_data_t golden_lsc = { 0, PNULL };
	struct isp_data_t golden_awb = { 0, PNULL };
	struct isp_data_t tmp = { 0, PNULL };
	struct isp_cali_data *cali_data = PNULL;
	cmr_u8 *cur = PNULL;
	cmr_u8 *start = PNULL;
	cmr_u32 awb_size = 0;
	cmr_u32 lsc_size = 0;

	if (PNULL == param || PNULL == result)
		return ISP_CALI_ERROR;

	if (PNULL == param->target_buf.data_ptr || param->target_buf.size < ISP_CALI_MIN_SIZE)
		return ISP_CALI_ERROR;

	rtn = _golden_parse(&param->golden, &golden_lsc, &golden_awb);
	if (ISP_CALI_SUCCESS != rtn)
		return rtn;

	memset(param->target_buf.data_ptr, 0, param->target_buf.size);

	start = (cmr_u8 *) param->target_buf.data_ptr;
	cali_data = (struct isp_cali_data *)start;
	cur = (cmr_u8 *) & cali_data->data;

	cali_data->version = 0x1;
	cali_data->block_num = 2;

	/*write AWB */
	cali_data->block[0].id = ISP_CALI_AWB_ID;
	cali_data->block[0].offset = cur - start;

	tmp.data_ptr = (void *)cur;
	tmp.size = param->target_buf.size - cali_data->block[0].offset;

	rtn = _cali_awb(&golden_awb, &param->awb_otp, &tmp, &awb_size);
	if (ISP_CALI_SUCCESS != rtn)
		return rtn;

	cali_data->block[0].length = awb_size;
	cur += cali_data->block[0].length;

	/*write LSC */
	cali_data->block[1].id = ISP_CALI_LSC_ID;
	cali_data->block[1].offset = cur - start;

	tmp.data_ptr = (void *)cur;
	tmp.size = param->target_buf.size - cali_data->block[1].offset;

	rtn = _cali_lsc(&golden_lsc, &param->lsc_otp, param->image_pattern, &tmp, &lsc_size);
	if (ISP_CALI_SUCCESS != rtn)
		return rtn;

	cali_data->block[1].length = lsc_size;
	cur += cali_data->block[1].length;
	cali_data->length = cur - start;

	result->data_ptr = param->target_buf.data_ptr;
	result->size = cali_data->length;

	return ISP_CALI_SUCCESS;
}

cmr_s32 isp_parse_calibration_data(struct isp_data_info * cali_data, struct isp_data_t * lsc, struct isp_data_t * awb)
{
	cmr_s32 rtn = ISP_CALI_SUCCESS;
	struct isp_cali_data *header = PNULL;
	cmr_u8 *start = PNULL;
	cmr_u8 *end = PNULL;
	cmr_u8 lsc_index = 0xff;
	cmr_u8 awb_index = 0xff;
	cmr_u32 i;

	if (NULL == cali_data || NULL == lsc || NULL == awb)
		return ISP_CALI_ERROR;

	header = (struct isp_cali_data *)cali_data->data_ptr;
	start = (cmr_u8 *) header;

	if (PNULL == header || cali_data->size < sizeof(*header))
		return ISP_CALI_ERROR;

	end = start + header->length;

	if (2 != header->block_num)
		return ISP_CALI_ERROR;

	for (i = 0; i < header->block_num; i++) {

		switch (header->block[i].id) {
		case ISP_CALI_AWB_ID:
			awb_index = i;
			break;

		case ISP_CALI_LSC_ID:
			lsc_index = i;
			break;
		}
	}

	if (awb_index >= header->block_num || lsc_index >= header->block_num)
		return ISP_CALI_ERROR;

	if (start + header->block[awb_index].offset + header->block[awb_index].length > end)
		return ISP_CALI_ERROR;

	if (start + header->block[lsc_index].offset + header->block[lsc_index].length > end)
		return ISP_CALI_ERROR;

	if (PNULL != lsc) {
		lsc->data_ptr = (void *)(start + header->block[lsc_index].offset);
		lsc->size = header->block[lsc_index].length;
	}

	if (PNULL != awb) {
		awb->data_ptr = (void *)(start + header->block[awb_index].offset);
		awb->size = header->block[awb_index].length;
	}

	return rtn;
}

cmr_int otp_ctrl_init(cmr_handle * isp_otp_handle, struct isp_otp_init_in *input_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_data_info *calibration_param = (struct isp_data_info *)&input_ptr->calibration_param;
	struct isp_otp_info *otp_info = NULL;
	struct isp_data_t lsc = { 0, PNULL };
	struct isp_data_t awb = { 0, PNULL };
	struct isp_pm_param_data update_param;
	memset(&update_param, 0x00, sizeof(update_param));

	ISP_LOGI("E");
	if (!input_ptr || !isp_otp_handle) {
		ISP_LOGE("fail to check init param,input_ptr is 0x%lx & handler is 0x%lx", (cmr_uint) input_ptr, (cmr_uint) isp_otp_handle);
		rtn = ISP_PARAM_NULL;
		goto exit;
	}
	*isp_otp_handle = NULL;

	if (NULL == calibration_param->data_ptr || 0 == calibration_param->size) {
		ISP_LOGW("fail to check calibration_param: %p, %d!", calibration_param->data_ptr, calibration_param->size);
		return ISP_SUCCESS;
	}

	otp_info = (struct isp_otp_info *)malloc(sizeof(struct isp_otp_info));
	if (NULL == otp_info) {
		ISP_LOGE("fail to check param");
		rtn = ISP_ERROR;
		goto exit;
	}
	memset((void *)otp_info, 0x00, sizeof(struct isp_otp_info));

	rtn = isp_parse_calibration_data(calibration_param, &lsc, &awb);
	if (ISP_SUCCESS != rtn) {
		/*do not return error */
		ISP_LOGE("fail to parse_calibration_data!");
		goto exit;
	}

	ISP_LOGV("lsc data: (%p, %d), awb data: (%p, %d)", lsc.data_ptr, lsc.size, awb.data_ptr, awb.size);

	otp_info->awb.data_ptr = (void *)malloc(awb.size);
	if (NULL != otp_info->awb.data_ptr) {
		otp_info->awb.size = awb.size;
		memcpy(otp_info->awb.data_ptr, awb.data_ptr, otp_info->awb.size);
	}

	otp_info->lsc.data_ptr = (void *)malloc(lsc.size);
	if (NULL != otp_info->lsc.data_ptr) {
		otp_info->lsc.size = lsc.size;
		memcpy(otp_info->lsc.data_ptr, lsc.data_ptr, otp_info->lsc.size);
	}
#ifdef CONFIG_USE_ALC_AWB
	struct isp_cali_lsc_info *cali_lsc_ptr = otp_info->lsc.data_ptr;
	if (cali_lsc_ptr) {
		otp_info->width = cali_lsc_ptr->map[0].width;
		otp_info->height = cali_lsc_ptr->map[0].height;
		otp_info->lsc_random = (cmr_u16 *) ((cmr_u8 *) & cali_lsc_ptr->data_area + cali_lsc_ptr->map[0].offset);
		otp_info->lsc_golden = input_ptr->lsc_golden_data;
	}
#else
	update_param.id = ISP_BLK_2D_LSC;
	update_param.cmd = ISP_PM_CMD_UPDATE_LSC_OTP;
	update_param.data_ptr = lsc.data_ptr;
	update_param.data_size = lsc.size;
	rtn = isp_pm_update(input_ptr->handle_pm, ISP_PM_CMD_UPDATE_LSC_OTP, &update_param, NULL);
	if (ISP_SUCCESS != rtn) {
		/*do not return error */
		ISP_LOGE("fail to parse_calibration_data!");
		goto exit;
	}
#endif

exit:
	if (rtn) {
		if (otp_info) {
			if (otp_info->awb.data_ptr) {
				free(otp_info->awb.data_ptr);
				otp_info->awb.data_ptr = NULL;
				otp_info->awb.size = 0;
			}
			if (otp_info->lsc.data_ptr) {
				free(otp_info->lsc.data_ptr);
				otp_info->lsc.data_ptr = NULL;
				otp_info->lsc.size = 0;
			}
			free((void *)otp_info);
			otp_info = NULL;
		}
	} else {
		*isp_otp_handle = (cmr_handle) otp_info;
	}
	ISP_LOGI("done, 0x%lx", rtn);

	return ISP_SUCCESS;
}

cmr_int otp_ctrl_deinit(cmr_handle isp_otp_handle)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_otp_info *otp_info = (struct isp_otp_info *)isp_otp_handle;

	ISP_LOGI("E");
	ISP_CHECK_HANDLE_VALID(isp_otp_handle);

	if (NULL != otp_info->awb.data_ptr) {
		free(otp_info->awb.data_ptr);
		otp_info->awb.data_ptr = NULL;
		otp_info->awb.size = 0;
	}

	if (NULL != otp_info->lsc.data_ptr) {
		free(otp_info->lsc.data_ptr);
		otp_info->lsc.data_ptr = NULL;
		otp_info->lsc.size = 0;
	}

	free((void *)otp_info);
	otp_info = NULL;

	ISP_LOGI("X");

	return rtn;
}
