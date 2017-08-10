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

#define LOG_TAG "isp_u_bpc"

#include "isp_drv.h"

cmr_s32 isp_u_bpc_block(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *bpc_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	bpc_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = bpc_ptr->scene_id;
	param.sub_block = ISP_BLOCK_BPC;
	param.property = ISP_PRO_BPC_BLOCK;
	param.property_param = bpc_ptr->block_info;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_bpc_bypass(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *bpc_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	bpc_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = bpc_ptr->scene_id;
	param.sub_block = ISP_BLOCK_BPC;
	param.property = ISP_PRO_BPC_BYPASS;
	param.property_param = &bpc_ptr->bypass;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_bpc_mode(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *bpc_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	bpc_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = bpc_ptr->scene_id;
	param.sub_block = ISP_BLOCK_BPC;
	param.property = ISP_PRO_BPC_MODE;
	param.property_param = &bpc_ptr->mode;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_bpc_param_common(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *bpc_ptr = NULL;
	struct isp_io_param param;
	struct isp_bpc_common bpc_param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	bpc_ptr = (struct isp_u_blocks_info *)param_ptr;

	bpc_param.pattern_type = bpc_ptr->bpc_param.pattern_type;
	bpc_param.detect_thrd = bpc_ptr->bpc_param.detect_thrd;
	bpc_param.super_bad_thrd = bpc_ptr->bpc_param.super_bad_thrd;

	param.isp_id = file->isp_id;
	param.scene_id = bpc_ptr->scene_id;
	param.sub_block = ISP_BLOCK_BPC;
	param.property = ISP_PRO_BPC_PARAM_COMMON;
	param.property_param = &bpc_param;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_bpc_thrd(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *bpc_ptr = NULL;
	struct isp_io_param param;
	struct isp_bpc_thrd thrd;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	bpc_ptr = (struct isp_u_blocks_info *)param_ptr;

	thrd.flat = bpc_ptr->bpc_thrd.flat;
	thrd.std = bpc_ptr->bpc_thrd.std;
	thrd.texture = bpc_ptr->bpc_thrd.texture;

	param.isp_id = file->isp_id;
	param.scene_id = bpc_ptr->scene_id;
	param.sub_block = ISP_BLOCK_BPC;
	param.property = ISP_PRO_BPC_THRD;
	param.property_param = &thrd;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_bpc_map_addr(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *bpc_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	bpc_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = bpc_ptr->scene_id;
	param.sub_block = ISP_BLOCK_BPC;
	param.property = ISP_PRO_BPC_MAP_ADDR;
	param.property_param = &bpc_ptr->addr;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_bpc_pixel_num(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *bpc_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	bpc_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = bpc_ptr->scene_id;
	param.sub_block = ISP_BLOCK_BPC;
	param.property = ISP_PRO_BPC_PIXEL_NUM;
	param.property_param = &bpc_ptr->pixel_num;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}
