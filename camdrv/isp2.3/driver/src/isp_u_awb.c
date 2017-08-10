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

#define LOG_TAG "isp_u_awb"

#include "isp_drv.h"

cmr_s32 isp_u_awb_block(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *awb_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	awb_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = awb_ptr->scene_id;
	param.sub_block = ISP_BLOCK_AWB;
	param.property = ISP_PRO_AWB_BLOCK;
	param.property_param = awb_ptr->block_info;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_awbm_bypass(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *awb_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	awb_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = awb_ptr->scene_id;
	param.sub_block = ISP_BLOCK_AWB;
	param.property = ISP_PRO_AWBM_BYPASS;
	param.property_param = &awb_ptr->bypass;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_awbm_mode(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *awb_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	awb_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = awb_ptr->scene_id;
	param.sub_block = ISP_BLOCK_AWB;
	param.property = ISP_PRO_AWBM_MODE;
	param.property_param = &awb_ptr->mode;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_awbm_skip_num(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *awb_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	awb_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = awb_ptr->scene_id;
	param.sub_block = ISP_BLOCK_AWB;
	param.property = ISP_PRO_AWBM_SKIP_NUM;
	param.property_param = &awb_ptr->skip_num;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_awbm_block_offset(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *awb_ptr = NULL;
	struct isp_io_param param;
	struct isp_img_offset offset;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	awb_ptr = (struct isp_u_blocks_info *)param_ptr;

	offset.x = awb_ptr->offset.x;
	offset.y = awb_ptr->offset.y;

	param.isp_id = file->isp_id;
	param.scene_id = awb_ptr->scene_id;
	param.sub_block = ISP_BLOCK_AWB;
	param.property = ISP_PRO_AWBM_BLOCK_OFFSET;
	param.property_param = &offset;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_awbm_block_size(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *awb_ptr = NULL;
	struct isp_io_param param;
	struct isp_img_size size;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	awb_ptr = (struct isp_u_blocks_info *)param_ptr;

	size.width = awb_ptr->size.width;
	size.height = awb_ptr->size.height;

	param.isp_id = file->isp_id;
	param.scene_id = awb_ptr->scene_id;
	param.sub_block = ISP_BLOCK_AWB;
	param.property = ISP_PRO_AWBM_BLOCK_SIZE;
	param.property_param = &size;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_awbm_shift(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *awb_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	awb_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = awb_ptr->scene_id;
	param.sub_block = ISP_BLOCK_AWB;
	param.property = ISP_PRO_AWBM_SHIFT;
	param.property_param = &awb_ptr->shift;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_awbc_bypass(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *awb_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	awb_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = awb_ptr->scene_id;
	param.sub_block = ISP_BLOCK_AWB;
	param.property = ISP_PRO_AWBC_BYPASS;
	param.property_param = &awb_ptr->bypass;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_awbc_gain(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *awb_ptr = NULL;
	struct isp_io_param param;
	struct isp_awbc_rgb gain;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	awb_ptr = (struct isp_u_blocks_info *)param_ptr;

	gain.r = awb_ptr->awbc_rgb.r;
	gain.g = awb_ptr->awbc_rgb.g;
	gain.b = awb_ptr->awbc_rgb.b;

	param.isp_id = file->isp_id;
	param.scene_id = awb_ptr->scene_id;
	param.sub_block = ISP_BLOCK_AWB;
	param.property = ISP_PRO_AWBC_GAIN;
	param.property_param = &gain;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_awbc_thrd(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *awb_ptr = NULL;
	struct isp_io_param param;
	struct isp_awbc_rgb thrd;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	awb_ptr = (struct isp_u_blocks_info *)param_ptr;

	thrd.r = awb_ptr->awbc_rgb.r;
	thrd.g = awb_ptr->awbc_rgb.g;
	thrd.b = awb_ptr->awbc_rgb.b;

	param.isp_id = file->isp_id;
	param.scene_id = awb_ptr->scene_id;
	param.sub_block = ISP_BLOCK_AWB;
	param.property = ISP_PRO_AWBC_THRD;
	param.property_param = &thrd;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_awbc_gain_offset(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *awb_ptr = NULL;
	struct isp_io_param param;
	struct isp_awbc_rgb offset;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	awb_ptr = (struct isp_u_blocks_info *)param_ptr;

	offset.r = awb_ptr->awbc_rgb.r;
	offset.g = awb_ptr->awbc_rgb.g;
	offset.b = awb_ptr->awbc_rgb.b;

	param.isp_id = file->isp_id;
	param.scene_id = awb_ptr->scene_id;
	param.sub_block = ISP_BLOCK_AWB;
	param.property = ISP_PRO_AWBC_GAIN_OFFSET;
	param.property_param = &offset;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}
