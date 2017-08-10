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

#define LOG_TAG "isp_u_edge"

#include "isp_drv.h"

cmr_s32 isp_u_edge_block(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *edge_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	edge_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = edge_ptr->scene_id;
	param.sub_block = ISP_BLOCK_EDGE;
	param.property = ISP_PRO_EDGE_BLOCK;
	param.property_param = edge_ptr->block_info;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_edge_bypass(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *edge_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	edge_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = edge_ptr->scene_id;
	param.sub_block = ISP_BLOCK_EDGE;
	param.property = ISP_PRO_EDGE_BYPASS;
	param.property_param = &edge_ptr->bypass;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_edge_param(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *edge_ptr = NULL;
	struct isp_io_param param;
	struct isp_edge_thrd edge_thrd;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	edge_ptr = (struct isp_u_blocks_info *)param_ptr;

	edge_thrd.detail = edge_ptr->edge_thrd.detail;
	edge_thrd.smooth = edge_ptr->edge_thrd.smooth;
	edge_thrd.strength = edge_ptr->edge_thrd.strength;

	param.isp_id = file->isp_id;
	param.scene_id = edge_ptr->scene_id;
	param.sub_block = ISP_BLOCK_EDGE;
	param.property = ISP_PRO_EDGE_PARAM;
	param.property_param = &edge_thrd;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}
