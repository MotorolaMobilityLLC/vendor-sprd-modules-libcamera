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

#define LOG_TAG "isp_u_raw_aem"

#include "isp_drv.h"

#define FEATURE_DCAM_AEM

cmr_s32 isp_u_raw_aem_block(cmr_handle handle, void *param_ptr)
{
#ifdef FEATURE_DCAM_AEM
	struct isp_u_blocks_info *raw_aem_ptr = NULL;
	raw_aem_ptr = (struct isp_u_blocks_info *)param_ptr;

	return dcam_u_raw_aem_block(handle, raw_aem_ptr->block_info);
#else
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *raw_aem_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	raw_aem_ptr = (struct isp_u_blocks_info *)param_ptr;

	if (((struct isp_dev_raw_aem_info *)raw_aem_ptr->block_info)->bypass > 1) {
		return ret;
	}

	param.isp_id = file->isp_id;
	param.scene_id = raw_aem_ptr->scene_id;
	param.sub_block = ISP_BLOCK_RAW_AEM;
	param.property = ISP_PRO_RAW_AEM_BLOCK;
	param.property_param = raw_aem_ptr->block_info;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
#endif
}

cmr_s32 isp_u_raw_aem_bypass(cmr_handle handle, void *param_ptr)
{
#if 0//def FEATURE_DCAM_AEM
	struct isp_u_blocks_info *raw_aem_ptr = NULL;
	raw_aem_ptr = (struct isp_u_blocks_info *)param_ptr;

	return dcam_u_raw_aem_bypass(handle, &raw_aem_ptr->bypass);
#else
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *raw_aem_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	raw_aem_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = raw_aem_ptr->scene_id;
	param.sub_block = ISP_BLOCK_RAW_AEM;
	param.property = ISP_PRO_RAW_AEM_BYPASS;
	param.property_param = &raw_aem_ptr->bypass;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
#endif
}

cmr_s32 isp_u_raw_aem_mode(cmr_handle handle, void *param_ptr)
{
#ifdef FEATURE_DCAM_AEM
	struct isp_u_blocks_info *raw_aem_ptr = NULL;
	raw_aem_ptr = (struct isp_u_blocks_info *)param_ptr;

	return dcam_u_raw_aem_mode(handle, raw_aem_ptr->stats_info.mode);
#else
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *raw_aem_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	raw_aem_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = raw_aem_ptr->scene_id;
	param.sub_block = ISP_BLOCK_RAW_AEM;
	param.property = ISP_PRO_RAW_AEM_MODE;
	param.property_param = &raw_aem_ptr->stats_info.mode;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
#endif
}

cmr_s32 isp_u_raw_aem_statistics(cmr_handle handle, cmr_u32 * r_info, cmr_u32 * g_info, cmr_u32 * b_info)
{
#if 0//def FEATURE_DCAM_AEM
	return dcam_u_raw_aem_statistics(handle, r_info, g_info, b_info);
#else
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;
	struct isp_raw_aem_statistics *aem_statistics = NULL;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	if (!r_info || !g_info || !b_info) {
		ISP_LOGE("data ptr is null error: 0x%lx 0x%lx 0x%lx", (cmr_uint) r_info, (cmr_uint) g_info, (cmr_uint) b_info);
		return -1;
	}

	aem_statistics = (struct isp_raw_aem_statistics *)malloc(sizeof(struct isp_raw_aem_statistics));
	if (!aem_statistics) {
		ISP_LOGE("NO mem");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_RAW_AEM;
	param.property = ISP_PRO_RAW_AEM_STATISTICS;
	memset(aem_statistics, 0x00, sizeof(struct isp_raw_aem_statistics));
	param.property_param = aem_statistics;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);
	if (0 == ret) {
		memcpy((void *)r_info, (void *)(aem_statistics->r), ISP_RAW_AEM_ITEM * 4);
		memcpy((void *)g_info, (void *)(aem_statistics->g), ISP_RAW_AEM_ITEM * 4);
		memcpy((void *)b_info, (void *)(aem_statistics->b), ISP_RAW_AEM_ITEM * 4);
	} else {
		ISP_LOGE("copy aem info error.");
	}

	if (aem_statistics) {
		free(aem_statistics);
		aem_statistics = NULL;
	}

	return ret;
#endif
}

cmr_s32 isp_u_raw_aem_skip_num(cmr_handle handle, void *param_ptr)
{
#ifdef FEATURE_DCAM_AEM
	struct isp_u_blocks_info *raw_aem_ptr = NULL;
	raw_aem_ptr = (struct isp_u_blocks_info *)param_ptr;

	return dcam_u_raw_aem_skip_num(handle, raw_aem_ptr->stats_info.skip_num);
#else
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *raw_aem_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	raw_aem_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = raw_aem_ptr->scene_id;
	param.sub_block = ISP_BLOCK_RAW_AEM;
	param.property = ISP_PRO_RAW_AEM_SKIP_NUM;
	param.property_param = &raw_aem_ptr->stats_info.skip_num;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
#endif
}

cmr_s32 isp_u_raw_aem_shift(cmr_handle handle, void *param_ptr)
{
#ifdef FEATURE_DCAM_AEM
	struct isp_u_blocks_info *raw_aem_ptr = NULL;
	raw_aem_ptr = (struct isp_u_blocks_info *)param_ptr;

	return dcam_u_raw_aem_shift(handle, raw_aem_ptr->shift);
#else
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *raw_aem_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	raw_aem_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = raw_aem_ptr->scene_id;
	param.sub_block = ISP_BLOCK_RAW_AEM;
	param.property = ISP_PRO_RAW_AEM_SHIFT;
	param.property_param = raw_aem_ptr->shift;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
#endif
}

cmr_s32 isp_u_raw_aem_offset(cmr_handle handle, void *param_ptr)
{
#ifdef FEATURE_DCAM_AEM
	struct isp_u_blocks_info *raw_aem_ptr = NULL;
	raw_aem_ptr = (struct isp_u_blocks_info *)param_ptr;

	return dcam_u_raw_aem_offset(handle,
			raw_aem_ptr->win_info.offset.x,
			raw_aem_ptr->win_info.offset.y);
#else
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *raw_aem_ptr = NULL;
	struct isp_io_param param;
	struct isp_img_offset offset;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	raw_aem_ptr = (struct isp_u_blocks_info *)param_ptr;

	offset.x = raw_aem_ptr->win_info.offset.x;
	offset.y = raw_aem_ptr->win_info.offset.y;

	param.isp_id = file->isp_id;
	param.scene_id = raw_aem_ptr->scene_id;
	param.sub_block = ISP_BLOCK_RAW_AEM;
	param.property = ISP_PRO_RAW_AEM_OFFSET;
	param.property_param = &offset;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
#endif
}

cmr_s32 isp_u_raw_aem_blk_size(cmr_handle handle, void *param_ptr)
{
#ifdef FEATURE_DCAM_AEM
	struct isp_u_blocks_info *raw_aem_ptr = NULL;
	raw_aem_ptr = (struct isp_u_blocks_info *)param_ptr;

	return dcam_u_raw_aem_blk_size(handle,
			raw_aem_ptr->win_info.size.width,
			raw_aem_ptr->win_info.size.height);
#else
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *raw_aem_ptr = NULL;
	struct isp_io_param param;
	struct isp_img_size size;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	raw_aem_ptr = (struct isp_u_blocks_info *)param_ptr;

	size.width = raw_aem_ptr->win_info.size.width;
	size.height = raw_aem_ptr->win_info.size.height;

	param.isp_id = file->isp_id;
	param.scene_id = raw_aem_ptr->scene_id;
	param.sub_block = ISP_BLOCK_RAW_AEM;
	param.property = ISP_PRO_RAW_AEM_BLK_SIZE;
	param.property_param = &size;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
#endif
}

cmr_s32 isp_u_raw_aem_slice_size(cmr_handle handle, void *param_ptr)
{
#if 0//def FEATURE_DCAM_AEM
	struct isp_u_blocks_info *raw_aem_ptr = NULL;
	raw_aem_ptr = (struct isp_u_blocks_info *)param_ptr;

	return dcam_u_raw_aem_slice_size(handle, &raw_aem_ptr->size);
#else
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *raw_aem_ptr = NULL;
	struct isp_io_param param;
	struct isp_img_size size;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	raw_aem_ptr = (struct isp_u_blocks_info *)param_ptr;

	size.width = raw_aem_ptr->size.width;
	size.height = raw_aem_ptr->size.height;

	param.isp_id = file->isp_id;
	param.scene_id = raw_aem_ptr->scene_id;
	param.sub_block = ISP_BLOCK_RAW_AEM;
	param.property = ISP_PRO_RAW_AEM_SLICE_SIZE;
	param.property_param = &size;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
#endif
}
