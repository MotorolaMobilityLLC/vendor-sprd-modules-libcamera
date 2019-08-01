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

#define LOG_TAG "dcam_u_aem"

#include "isp_drv.h"


	
cmr_s32 dcam_u_aem_bypass(cmr_handle handle, cmr_u32 bypass)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		ISP_LOGE("fail to get handle: handle = %p", handle);
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.scene_id = 0;
	param.sub_block = DCAM_BLOCK_AEM;
	param.property = DCAM_PRO_AEM_BYPASS;
	param.property_param = &bypass;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 dcam_u_aem_mode(cmr_handle handle, cmr_u32 mode)
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
	param.sub_block = DCAM_BLOCK_AEM;
	param.property = DCAM_PRO_AEM_MODE;
	param.property_param = &mode;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 dcam_u_aem_skip_num(cmr_handle handle, cmr_u32 skip_num)
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
	param.sub_block = DCAM_BLOCK_AEM;
	param.property = DCAM_PRO_AEM_SKIPNUM;
	param.property_param = &skip_num;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 dcam_u_aem_win(cmr_handle handle, void *aem_win)
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
	param.sub_block = DCAM_BLOCK_AEM;
	param.property = DCAM_PRO_AEM_WIN;
	param.property_param = aem_win;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 dcam_u_aem_rgb_thr(cmr_handle handle, void *rgb_thr)
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
	param.sub_block = DCAM_BLOCK_AEM;
	param.property = DCAM_PRO_AEM_RGB_THR;
	param.property_param = rgb_thr;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}
