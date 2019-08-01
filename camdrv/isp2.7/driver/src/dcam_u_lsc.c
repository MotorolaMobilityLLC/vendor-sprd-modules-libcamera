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

#define LOG_TAG "dcam_u_lsc"

#include "isp_drv.h"

cmr_s32 dcam_u_lsc_block(cmr_handle handle, void *block_info)
{
	cmr_s32 ret = 0;
	struct isp_io_param param;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *block_ptr = NULL;
	struct dcam_dev_lsc_info *lens_info = NULL;

	if (!handle || !block_info) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, block_info);
		return -1;
	}

	file = (struct isp_file *)(handle);
	block_ptr = (struct isp_u_blocks_info *)block_info;
	lens_info = (struct dcam_dev_lsc_info *)(block_ptr->block_info);
	if (!lens_info) {
		ISP_LOGE("failed to get lens_info ptr!");
		return -1;
	}

	param.scene_id = block_ptr->scene_id;
	param.sub_block = DCAM_BLOCK_LSC;
	param.property = DCAM_PRO_LSC_BLOCK;
	param.property_param = lens_info;
	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}
