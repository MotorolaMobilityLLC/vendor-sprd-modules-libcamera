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

cmr_s32 isp_u_awb_block(cmr_handle handle, void *block_info)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle || !block_info) {
		ISP_LOGE("handle is null error: 0x%lx 0x%lx", (cmr_uint) handle, (cmr_uint) block_info);
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_AWB;
	param.property = ISP_PRO_AWB_BLOCK;
	param.property_param = block_info;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_awbm_bypass(cmr_handle handle, cmr_u32 bypass)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_AWB;
	param.property = ISP_PRO_AWBM_BYPASS;
	param.property_param = &bypass;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_awbm_mode(cmr_handle handle, cmr_u32 mode)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_AWB;
	param.property = ISP_PRO_AWBM_MODE;
	param.property_param = &mode;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_awbm_skip_num(cmr_handle handle, cmr_u32 num)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_AWB;
	param.property = ISP_PRO_AWBM_SKIP_NUM;
	param.property_param = &num;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_awbm_block_offset(cmr_handle handle, cmr_u32 x, cmr_u32 y)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;
	struct isp_img_offset offset;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_AWB;
	param.property = ISP_PRO_AWBM_BLOCK_OFFSET;
	offset.x = x;
	offset.y = y;
	param.property_param = &offset;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_awbm_block_size(cmr_handle handle, cmr_u32 w, cmr_u32 h)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;
	struct isp_img_size size;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_AWB;
	param.property = ISP_PRO_AWBM_BLOCK_SIZE;
	size.width = w;
	size.height = h;
	param.property_param = &size;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_awbm_shift(cmr_handle handle, cmr_u32 shift)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_AWB;
	param.property = ISP_PRO_AWBM_SHIFT;
	param.property_param = &shift;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_awbc_bypass(cmr_handle handle, cmr_u32 bypass)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_AWB;
	param.property = ISP_PRO_AWBC_BYPASS;
	param.property_param = &bypass;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_awbc_gain(cmr_handle handle, cmr_u32 r, cmr_u32 g, cmr_u32 b)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;
	struct isp_awbc_rgb gain;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_AWB;
	param.property = ISP_PRO_AWBC_GAIN;
	gain.r = r;
	gain.g = g;
	gain.b = b;
	param.property_param = &gain;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_awbc_thrd(cmr_handle handle, cmr_u32 r, cmr_u32 g, cmr_u32 b)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;
	struct isp_awbc_rgb thrd;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_AWB;
	param.property = ISP_PRO_AWBC_THRD;
	thrd.r = r;
	thrd.g = g;
	thrd.b = b;
	param.property_param = &thrd;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_awbc_gain_offset(cmr_handle handle, cmr_u32 r, cmr_u32 g, cmr_u32 b)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;
	struct isp_awbc_rgb offset;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_AWB;
	param.property = ISP_PRO_AWBC_GAIN_OFFSET;
	offset.r = r;
	offset.g = g;
	offset.b = b;
	param.property_param = &offset;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}
