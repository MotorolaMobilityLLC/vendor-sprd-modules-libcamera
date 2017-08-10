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

#define LOG_TAG "isp_u_raw_afm"

#include "isp_drv.h"

cmr_s32 isp_u_raw_afm_block(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *raw_afm_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	raw_afm_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = raw_afm_ptr->scene_id;
	param.sub_block = ISP_BLOCK_RAW_AFM;
	param.property = ISP_PRO_RGB_AFM_BLOCK;
	param.property_param = raw_afm_ptr->block_info;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);
	return ret;
}

cmr_s32 isp_u_raw_afm_slice_size(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *raw_afm_ptr = NULL;
	struct isp_io_param param;
	struct isp_img_size size;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	raw_afm_ptr = (struct isp_u_blocks_info *)param_ptr;

	size.width = raw_afm_ptr->size.width;
	size.height = raw_afm_ptr->size.height;

	param.isp_id = file->isp_id;
	param.scene_id = raw_afm_ptr->scene_id;
	param.sub_block = ISP_BLOCK_RAW_AFM;
	param.property = ISP_PRO_RGB_AFM_FRAME_SIZE;
	param.property_param = &size;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);
	return ret;
}

cmr_s32 isp_u_raw_afm_iir_nr_cfg(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;

	struct isp_file *file = NULL;
	struct isp_u_blocks_info *raw_afm_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	raw_afm_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = raw_afm_ptr->scene_id;
	param.sub_block = ISP_BLOCK_RAW_AFM;
	param.property = ISP_PRO_RGB_AFM_IIR_NR_CFG;
	param.property_param = raw_afm_ptr->block_info;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_raw_afm_modules_cfg(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;

	struct isp_file *file = NULL;
	struct isp_u_blocks_info *raw_afm_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	raw_afm_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = raw_afm_ptr->scene_id;
	param.sub_block = ISP_BLOCK_RAW_AFM;
	param.property = ISP_PRO_RGB_AFM_MODULE_CFG;
	param.property_param = raw_afm_ptr->block_info;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_raw_afm_type1_statistic(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	UNUSED(handle);
	UNUSED(param_ptr);
#if 0				/*modify for Solve compile problem */
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *afm_v1_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	afm_v1_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = afm_v1_ptr->scene_id;
	param.sub_block = ISP_BLOCK_AFM_V1;
	param.property = ISP_PRO_RGB_AFM_TYPE1_STATISTIC;
	param.property_param = afm_v1_ptr->statis;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);
#endif
	return ret;
}

cmr_s32 isp_u_raw_afm_type2_statistic(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	UNUSED(handle);
	UNUSED(param_ptr);
#if 0				/*modify for Solve compile problem */
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *afm_v1_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	afm_v1_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = afm_v1_ptr->scene_id;
	param.sub_block = ISP_BLOCK_AFM_V1;
	param.property = ISP_PRO_RGB_AFM_TYPE2_STATISTIC;
	param.property_param = afm_v1_ptr->statis;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);
#endif
	return ret;
}

cmr_s32 isp_u_raw_afm_statistic_r6p9(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	UNUSED(handle);
	UNUSED(param_ptr);
#if 0
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *afm_v1_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	afm_v1_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = afm_v1_ptr->scene_id;
	param.sub_block = ISP_BLOCK_AFM_V1;
	param.property = ISP_PRO_RGB_AFM_STATISTIC;
	param.property_param = afm_v1_ptr->statis;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);
#endif
	return ret;
}

cmr_s32 isp_u_raw_afm_bypass(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *raw_afm_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	raw_afm_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = raw_afm_ptr->scene_id;
	param.sub_block = ISP_BLOCK_RAW_AFM;
	param.property = ISP_PRO_RGB_AFM_BYPASS;
	param.property_param = &raw_afm_ptr->afm_info.bypass;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);
	return ret;
}

cmr_s32 isp_u_raw_afm_mode(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;

	struct isp_file *file = NULL;
	struct isp_u_blocks_info *raw_afm_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	raw_afm_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = raw_afm_ptr->scene_id;
	param.sub_block = ISP_BLOCK_RAW_AFM;
	param.property = ISP_PRO_RGB_AFM_MODE;
	param.property_param = &raw_afm_ptr->mode;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_raw_afm_skip_num(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;

	struct isp_file *file = NULL;
	struct isp_u_blocks_info *raw_afm_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	raw_afm_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = raw_afm_ptr->scene_id;
	param.sub_block = ISP_BLOCK_RAW_AFM;
	param.property = ISP_PRO_RGB_AFM_SKIP_NUM;
	param.property_param = &raw_afm_ptr->afm_info.skip_num;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_raw_afm_skip_num_clr(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *raw_afm_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	raw_afm_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = raw_afm_ptr->scene_id;
	param.sub_block = ISP_BLOCK_RAW_AFM;
	param.property = ISP_PRO_RGB_AFM_SKIP_NUM_CLR;
	param.property_param = &raw_afm_ptr->clear;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_raw_afm_spsmd_rtgbot_enable(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	UNUSED(handle);
	UNUSED(param_ptr);
#if 0
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *afm_v1_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	afm_v1_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = afm_v1_ptr->scene_id;
	param.sub_block = ISP_BLOCK_AFM_V1;
	param.property = ISP_PRO_RGB_AFM_SPSMD_RTGBOT_ENABLE;
	param.property_param = &afm_v1_ptr->enable;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);
#endif
	return ret;
}

cmr_s32 isp_u_raw_afm_spsmd_diagonal_enable(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	UNUSED(handle);
	UNUSED(param_ptr);
#if 0				/*modify for Solve compile problem */
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *afm_v1_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	afm_v1_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = afm_v1_ptr->scene_id;
	param.sub_block = ISP_BLOCK_AFM_V1;
	param.property = ISP_PRO_RGB_AFM_SPSMD_DIAGONAL_ENABLE;
	param.property_param = &afm_v1_ptr->enable;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);
#endif
	return ret;
}

cmr_s32 isp_u_raw_afm_spsmd_cal_mode(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	UNUSED(handle);
	UNUSED(param_ptr);
#if 0				/*modify for Solve compile problem */
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *afm_v1_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	afm_v1_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = afm_v1_ptr->scene_id;
	param.sub_block = ISP_BLOCK_AFM_V1;
	param.property = ISP_PRO_RGB_AFM_SPSMD_CAL_MOD;
	param.property_param = &afm_v1_ptr->mode;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);
#endif
	return ret;
}

cmr_s32 isp_u_raw_afm_spsmd_square_en(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	UNUSED(handle);
	UNUSED(param_ptr);
#if 0				/*modify for Solve compile problem */
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *afm_v1_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	afm_v1_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = afm_v1_ptr->scene_id;
	param.sub_block = ISP_BLOCK_AFM_V1;
	param.property = ISP_PRO_RGB_AFM_SPSMD_SQUARE_ENABLE;
	param.property_param = &afm_v1_ptr->enable;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);
#endif
	return ret;
}

cmr_s32 isp_u_raw_afm_sel_filter1(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	UNUSED(handle);
	UNUSED(param_ptr);
#if 0				/*modify for Solve compile problem */
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *afm_v1_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	afm_v1_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = afm_v1_ptr->scene_id;
	param.sub_block = ISP_BLOCK_AFM_V1;
	param.property = ISP_PRO_RGB_AFM_SEL_FILTER1;
	param.property_param = &afm_v1_ptr->sel_filter;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);
#endif
	return ret;
}

cmr_s32 isp_u_raw_afm_sel_filter2(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	UNUSED(handle);
	UNUSED(param_ptr);
#if 0				/*modify for Solve compile problem */
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *afm_v1_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	afm_v1_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = afm_v1_ptr->scene_id;
	param.sub_block = ISP_BLOCK_AFM_V1;
	param.property = ISP_PRO_RGB_AFM_SEL_FILTER2;
	param.property_param = &afm_v1_ptr->sel_filter;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);
#endif
	return ret;
}

cmr_s32 isp_u_raw_afm_sobel_type(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	UNUSED(handle);
	UNUSED(param_ptr);
#if 0				/*modify for Solve compile problem */
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *afm_v1_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	afm_v1_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = afm_v1_ptr->scene_id;
	param.sub_block = ISP_BLOCK_AFM_V1;
	param.property = ISP_PRO_RGB_AFM_SOBEL_TYPE;
	param.property_param = &afm_v1_ptr->type;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);
#endif
	return ret;
}

cmr_s32 isp_u_raw_afm_overflow_protect(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	UNUSED(handle);
	UNUSED(param_ptr);
#if 0				/*modify for Solve compile problem */
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *afm_v1_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	afm_v1_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = afm_v1_ptr->scene_id;
	param.sub_block = ISP_BLOCK_AFM_V1;
	param.property = ISP_PRO_RGB_AFM_OVERFLOW_PROTECT;
	param.property_param = &afm_v1_ptr->enable;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);
#endif
	return ret;
}

cmr_s32 isp_u_raw_afm_spsmd_type(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	UNUSED(handle);
	UNUSED(param_ptr);
#if 0				/*modify for Solve compile problem */
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *afm_v1_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	afm_v1_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = afm_v1_ptr->scene_id;
	param.sub_block = ISP_BLOCK_AFM_V1;
	param.property = ISP_PRO_RGB_AFM_SPSMD_TYPE;
	param.property_param = &afm_v1_ptr->type;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);
#endif
	return ret;
}

cmr_s32 isp_u_raw_afm_subfilter(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	UNUSED(handle);
	UNUSED(param_ptr);
#if 0				/*modify for Solve compile problem */
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *afm_v1_ptr = NULL;
	struct isp_io_param param;
	struct afm_subfilter subfilter;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	afm_v1_ptr = (struct isp_u_blocks_info *)param_ptr;

	subfilter.average = afm_v1_ptr->subfilter.average;
	subfilter.median = afm_v1_ptr->subfilter.median;

	param.isp_id = file->isp_id;
	param.scene_id = afm_v1_ptr->scene_id;
	param.sub_block = ISP_BLOCK_AFM_V1;
	param.property = ISP_PRO_RGB_AFM_SUBFILTER;
	param.property_param = &subfilter;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);
#endif
	return ret;
}

cmr_s32 isp_u_raw_afm_spsmd_touch_mode(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	UNUSED(handle);
	UNUSED(param_ptr);
#if 0
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *afm_v1_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	afm_v1_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = afm_v1_ptr->scene_id;
	param.sub_block = ISP_BLOCK_AFM_V1;
	param.property = ISP_PRO_RGB_AFM_SPSMD_TOUCH_MODE;
	param.property_param = &afm_v1_ptr->mode;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);
#endif
	return ret;
}

cmr_s32 isp_u_raw_afm_shfit(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	UNUSED(handle);
	UNUSED(param_ptr);
#if 0
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *afm_v1_ptr = NULL;
	struct isp_io_param param;
	struct afm_shift shift;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	afm_v1_ptr = (struct isp_u_blocks_info *)param_ptr;

	shift.shift_spsmd = afm_v1_ptr->afm_shift.shift_spsmd;
	shift.shift_sobel5 = afm_v1_ptr->afm_shift.shift_sobel5;
	shift.shift_sobel9 = afm_v1_ptr->afm_shift.shift_sobel9;

	param.isp_id = file->isp_id;
	param.scene_id = afm_v1_ptr->scene_id;
	param.sub_block = ISP_BLOCK_AFM_V1;
	param.property = ISP_PRO_RGB_AFM_SHFIT;
	param.property_param = &shift;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);
#endif
	return ret;
}

cmr_s32 isp_u_raw_afm_sobel_threshold(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	UNUSED(handle);
	UNUSED(param_ptr);
#if 0
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *afm_v1_ptr = NULL;
	struct isp_io_param param;
	struct sobel_thrd thrd;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	afm_v1_ptr = (struct isp_u_blocks_info *)param_ptr;

	thrd.min = afm_v1_ptr->sobel_thrd.min;
	thrd.max = afm_v1_ptr->sobel_thrd.max;

	param.isp_id = file->isp_id;
	param.scene_id = afm_v1_ptr->scene_id;
	param.sub_block = ISP_BLOCK_AFM_V1;
	param.property = ISP_PRO_RGB_AFM_SOBEL_THRESHOLD;
	param.property_param = &thrd;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);
#endif
	return ret;
}

cmr_s32 isp_u_raw_afm_spsmd_threshold(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	UNUSED(handle);
	UNUSED(param_ptr);
#if 0
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *afm_v1_ptr = NULL;
	struct isp_io_param param;
	struct spsmd_thrd thrd;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	afm_v1_ptr = (struct isp_u_blocks_info *)param_ptr;

	thrd.min = afm_v1_ptr->spsmd_thrd.min;
	thrd.max = afm_v1_ptr->spsmd_thrd.max;

	param.isp_id = file->isp_id;
	param.scene_id = afm_v1_ptr->scene_id;
	param.sub_block = ISP_BLOCK_AFM_V1;
	param.property = ISP_PRO_RGB_AFM_SPSMD_THRESHOLD;
	param.property_param = &thrd;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);
#endif
	return ret;
}

cmr_s32 isp_u_raw_afm_threshold_rgb(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	UNUSED(handle);
	UNUSED(param_ptr);
#if 0
	cmr_u32 num = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *afm_v1_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	afm_v1_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = afm_v1_ptr->scene_id;
	param.sub_block = ISP_BLOCK_AFM_V1;
	param.property = ISP_PRO_RGB_AFM_THRESHOLD_RGB;
	param.property_param = afm_v1_ptr->thr_rgb;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);
#endif
	return ret;
}

cmr_s32 isp_u_raw_afm_win(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *raw_afm_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	raw_afm_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = raw_afm_ptr->scene_id;
	param.sub_block = ISP_BLOCK_RAW_AFM;
	param.property = ISP_PRO_RGB_AFM_WIN;
	param.property_param = raw_afm_ptr->win_range;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_raw_afm_win_num(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *raw_afm_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	raw_afm_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_RAW_AFM;
	param.property = ISP_PRO_RGB_AFM_WIN_NUM;
	param.property_param = (void *)raw_afm_ptr->win_num;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}
