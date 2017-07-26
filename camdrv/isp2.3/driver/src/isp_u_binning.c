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

#define LOG_TAG "isp_u_binning4awb"

#include "isp_drv.h"

cmr_s32 isp_u_binning4awb_block(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *binnging_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	binnging_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = binnging_ptr->scene_id;
	param.sub_block = ISP_BLOCK_BINNING;
	param.property = ISP_PRO_BINNING_BLOCK;
	param.property_param = binnging_ptr->block_info;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_binning4awb_bypass(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *binnging_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	binnging_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = binnging_ptr->scene_id;
	param.sub_block = ISP_BLOCK_BINNING;
	param.property = ISP_PRO_BINNING_BYPASS;
	param.property_param = &binnging_ptr->bypass;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_binning4awb_endian(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *binnging_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	binnging_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = binnging_ptr->scene_id;
	param.sub_block = ISP_BLOCK_BINNING;
	param.property = ISP_PRO_BINNING_ENDIAN;
	param.property_param = &binnging_ptr->endian;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_binning4awb_scaling_ratio(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *binnging_ptr = NULL;
	struct isp_io_param param;
	struct isp_scaling_ratio scaling_ratio;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	binnging_ptr = (struct isp_u_blocks_info *)param_ptr;

	scaling_ratio.vertical = binnging_ptr->scaling_ratio.vertical;
	scaling_ratio.horizontal = binnging_ptr->scaling_ratio.horizontal;

	param.isp_id = file->isp_id;
	param.scene_id = binnging_ptr->scene_id;
	param.sub_block = ISP_BLOCK_BINNING;
	param.property = ISP_PRO_BINNING_SCALING_RATIO;
	param.property_param = &scaling_ratio;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_binning4awb_get_scaling_ratio(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *binnging_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	binnging_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_BINNING;
	param.property = ISP_PRO_BINNING_GET_SCALING_RATIO;
	param.property_param = &binnging_ptr->scaling_ratio;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);


	return ret;
}

cmr_s32 isp_u_binning4awb_mem_addr(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *binnging_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	binnging_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = binnging_ptr->scene_id;
	param.sub_block = ISP_BLOCK_BINNING;
	param.property = ISP_PRO_BINNING_MEM_ADDR;
	param.property_param = &binnging_ptr->phy_addr;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_binning4awb_statistics_buf(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *binnging_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	binnging_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = binnging_ptr->scene_id;
	param.sub_block = ISP_BLOCK_BINNING;
	param.property = ISP_PRO_BINNING_STATISTICS_BUF;
	param.property_param = binnging_ptr->buf_id;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_binning4awb_transaddr(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *binnging_ptr = NULL;
	struct isp_io_param param;
	struct isp_b4awb_phys phys_addr;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	binnging_ptr = (struct isp_u_blocks_info *)param_ptr;

	phys_addr.phys0 = binnging_ptr->phys_addr.phys0;
	phys_addr.phys1 = binnging_ptr->phys_addr.phys1;

	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_BINNING;
	param.property = ISP_PRO_BINNING_TRANSADDR;
	param.property_param = &phys_addr;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_binning4awb_initbuf(cmr_handle handle)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		ISP_LOGE("failed to get ptr: %p", handle);
		return -1;
	}

	file = (struct isp_file *)(handle);

	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_BINNING;
	param.property = ISP_PRO_BINNING_INITBUF;
	param.property_param = &ret;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}
