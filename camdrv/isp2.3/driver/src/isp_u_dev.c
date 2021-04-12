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

#define LOG_TAG "isp_u_dev"

#include "isp_drv.h"

#define ISP_REGISTER_MAX_NUM 20

cmr_s32 isp_dev_open(cmr_s32 fd, cmr_handle *handle)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;

	file = malloc(sizeof(struct isp_file));
	if (!file) {
		ret = -1;
		ISP_LOGE("fail to alloc memory");
		return ret;
	}

	if (fd < 0) {
		ret = -1;
		ISP_LOGE("fail to open device");
		goto isp_free;
	}

	file->fd = fd;
	file->isp_id = 0;
	*handle = (cmr_handle) file;

	return ret;

isp_free:
	free((void *)file);
	file = NULL;

	return ret;
}

cmr_s32 isp_dev_close(cmr_handle handle)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;

	if (!handle) {
		ISP_LOGE("fail to check file handle");
		ret = -1;
		return ret;
	}

	file = (struct isp_file *)handle;

	free((void *)file);
	file = NULL;


	return ret;
}

cmr_s32 isp_dev_reset(cmr_handle handle)
{
	cmr_s32 ret = 0;
	cmr_u32 isp_id = 0;
	struct isp_file *file = NULL;

	if (!handle) {
		ISP_LOGE("fail to check handle");
		return -1;
	}

	file = (struct isp_file *)(handle);
	isp_id = file->isp_id;

	ret = ioctl(file->fd, SPRD_ISP_IO_RST, &isp_id);
	if (ret) {
		ISP_LOGE("fail to reset isp hardawre");
	}

	return ret;
}

cmr_s32 isp_dev_set_statis_buf(cmr_handle handle, struct isp_statis_buf_input *param)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;

	if (!handle) {
		ISP_LOGE("fail to check handle");
		return -1;
	}
	if (!param) {
		ISP_LOGE("fail to check param");
		return -1;
	}

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, SPRD_ISP_IO_SET_STATIS_BUF, param);
	if (ret) {
		ISP_LOGE("fail to isp_dev_set_statis_buf 0x%x", ret);
	}

	return ret;
}

cmr_s32 isp_dev_mask_3a_int(cmr_handle handle)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	cmr_u32 mask = 0;

	if (!handle) {
		ISP_LOGE("fail to check handle");
		return -1;
	}

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, SPRD_ISP_IO_MASK_3A, mask);
	if (ret) {
		ISP_LOGE("fail to set mask %d", ret);
	}

	return ret;
}

cmr_s32 isp_dev_set_slice_raw_info(cmr_handle handle, struct isp_raw_proc_info *param)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;

	if (!handle) {
		ISP_LOGE("fail to check handle");
		return -1;
	}
	if (!param) {
		ISP_LOGE("fail to check param");
		return -1;
	}

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, SPRD_ISP_IO_RAW_CAP, param);
	if (ret) {
		ISP_LOGE("fail to set raw slice info 0x%x", ret);
	}

	return ret;
}
