/*
 * Copyright (C) 2018 The Android Open Source Project
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

#define LOG_TAG "dcam_u_afm"

#include "isp_drv.h"

cmr_s32 dcam_u_afm_block(cmr_handle handle, void *block_info)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;
	struct isp_u_blocks_info *block_param = (struct isp_u_blocks_info *)block_info;

	if (!handle || !block_info) {
		ISP_LOGE("fail to get handle: handle = %p, block_info = %p.", handle, block_info);
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.scene_id = block_param->scene_id;
	param.sub_block = DCAM_BLOCK_AFM;
	param.property = DCAM_PRO_AFM_BLOCK;
	param.property_param = block_param->block_info;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 dcam_u_afm_bypass(cmr_handle handle, cmr_u32 bypass)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		ISP_LOGE("fail to get handle.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.scene_id = 0;
	param.sub_block = DCAM_BLOCK_AFM;
	param.property = DCAM_PRO_AFM_BYPASS;
	param.property_param = &bypass;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);
	return ret;
}

cmr_s32 dcam_u_afm_mode(cmr_handle handle, cmr_u32 mode)
{
	cmr_s32 ret = 0;

	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		ISP_LOGE("fail to get handle.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.scene_id = 0;
	param.sub_block = DCAM_BLOCK_AFM;
	param.property = DCAM_PRO_AFM_MODE;
	param.property_param = &mode;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 dcam_u_afm_skip_num(cmr_handle handle, cmr_u32 skip_num)
{
	cmr_s32 ret = 0;

	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		ISP_LOGE("fail to get handle.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.scene_id = 0;
	param.sub_block = DCAM_BLOCK_AFM;
	param.property = DCAM_PRO_AFM_SKIPNUM;
	param.property_param = &skip_num;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 dcam_u_afm_win(cmr_handle handle, void *win_range)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		ISP_LOGE("fail to get handle.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.scene_id = 0;
	param.sub_block = DCAM_BLOCK_AFM;
	param.property = DCAM_PRO_AFM_WIN;
	param.property_param = win_range;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 dcam_u_afm_win_num(cmr_handle handle, void *win_num)
{
	cmr_s32 ret = 0;

	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		ISP_LOGE("fail to get handle.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.scene_id = 0;
	param.sub_block = DCAM_BLOCK_AFM;
	param.property = DCAM_PRO_AFM_WIN_NUM;
	param.property_param = win_num;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 dcam_u_afm_crop_eb(cmr_handle handle, cmr_u32 crop_eb)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		ISP_LOGE("fail to get handle.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.scene_id = 0;
	param.sub_block = DCAM_BLOCK_AFM;
	param.property = DCAM_PRO_AFM_CROP_EB;
	param.property_param = &crop_eb;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 dcam_u_afm_crop_size(cmr_handle handle, void *crop_size)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		ISP_LOGE("fail to get handle.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.scene_id = 0;
	param.sub_block = DCAM_BLOCK_AFM;
	param.property = DCAM_PRO_AFM_CROP_SIZE;
	param.property_param = crop_size;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 dcam_u_afm_done_tilenum(cmr_handle handle, void *done_tile_num)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		ISP_LOGE("fail to get handle.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.scene_id = 0;
	param.sub_block = DCAM_BLOCK_AFM;
	param.property = DCAM_PRO_AFM_DONE_TILENUM;
	param.property_param = done_tile_num;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}
