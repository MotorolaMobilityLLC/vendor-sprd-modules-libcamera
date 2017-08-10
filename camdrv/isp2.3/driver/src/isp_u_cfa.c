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

#define LOG_TAG "isp_u_cfa"

#include "isp_drv.h"

cmr_s32 isp_u_cfa_block(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *cfa_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	cfa_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = cfa_ptr->scene_id;
	param.sub_block = ISP_BLOCK_CFA;
	param.property = ISP_PRO_CFA_BLOCK;
	param.property_param = cfa_ptr->block_info;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_cfa_thrd(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *cfa_ptr = NULL;
	struct isp_io_param param;
	struct isp_cfa_thrd thrd;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	cfa_ptr = (struct isp_u_blocks_info *)param_ptr;

	thrd.edge = cfa_ptr->cfa_thrd.edge;
	thrd.ctrl = cfa_ptr->cfa_thrd.ctrl;

	param.isp_id = file->isp_id;
	param.scene_id = cfa_ptr->scene_id;
	param.sub_block = ISP_BLOCK_CFA;
	param.property = ISP_PRO_CFA_THRD;
	param.property_param = &thrd;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_cfa_slice_size(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *cfa_ptr = NULL;
	struct isp_io_param param;
	struct isp_img_size size;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	cfa_ptr = (struct isp_u_blocks_info *)param_ptr;

	size.width = cfa_ptr->size.width;
	size.height = cfa_ptr->size.height;

	param.isp_id = file->isp_id;
	param.scene_id = cfa_ptr->scene_id;
	param.sub_block = ISP_BLOCK_CFA;
	param.property = ISP_PRO_CFA_SLICE_SIZE;
	param.property_param = &size;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_cfa_slice_info(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *cfa_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	cfa_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = cfa_ptr->scene_id;
	param.sub_block = ISP_BLOCK_CFA;
	param.property = ISP_PRO_CFA_SLICE_INFO;
	param.property_param = &cfa_ptr->info;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}
