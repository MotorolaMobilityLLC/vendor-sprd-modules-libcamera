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

#define LOG_TAG "isp_u_dev"

#include "isp_drv.h"

cmr_s32 isp_dev_open(cmr_s32 fd, cmr_handle *handle)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;

	file = (struct isp_file *)malloc(sizeof(struct isp_file));
	if (!file) {
		ret = -1;
		ISP_LOGE("fail to alloc memory.");
		return ret;
	}
	memset((void *)file, 0x00, sizeof(struct isp_file));

	if (fd < 0) {
		ret = -1;
		ISP_LOGE("fail to open device.");
		goto isp_free;
	}

	file->fd = fd;
	file->isp_id = 0;
	*handle = (cmr_handle) file;

	ISP_LOGI("fd %d handle %p", file->fd, file);
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
		ISP_LOGE("fail to get file handle.");
		ret = -1;
		return ret;
	}

	file = (struct isp_file *)handle;

	free((void *)file);
	file = NULL;


	return ret;
}


cmr_s32 isp_dev_reset(isp_handle handle)
{
	cmr_s32 ret = 0;
	cmr_u32 isp_id = 0;
	struct isp_file *file = NULL;

	if (!handle) {
		ISP_LOGE("fail to get handle.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	isp_id = file->isp_id;

	ret = ioctl(file->fd, SPRD_ISP_IO_RST, &isp_id);
	if (ret) {
		ISP_LOGE("fail to do reset.");
	}

	return ret;
}


cmr_s32 isp_dev_get_video_size(
			isp_handle handle, cmr_u32 *width, cmr_u32 *height)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct sprd_img_size size = {0};

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, SPRD_IMG_IO_CAPABILITY, &size);
	if (ret) {
		ISP_LOGE("fail to get video size.");
		return ret;
	}

	*width = size.w;
	*height = size.h;
	ISP_LOGI("get video size width=%d, height=%d", *width , *height);

	return ret;
}

cmr_s32 isp_dev_get_ktime(
	cmr_handle handle, cmr_u32 *sec, cmr_u32 *usec)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct sprd_img_time time;

	if (!handle) {
		ISP_LOGE("fail to get handle.");
		return -1;
	}
	if (!sec || ! usec) {
		ISP_LOGE("fail to get param.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	ret = ioctl(file->fd, SPRD_IMG_IO_GET_TIME, &time);
	if (ret) {
		ISP_LOGE("fail to get ktime %d.", ret);
	} else {
		*sec = time.sec;
		*usec = time.usec;
	}

	return ret;
}


cmr_s32 isp_dev_set_statis_buf(cmr_handle handle, struct isp_statis_buf_input *param)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;

	if (!handle) {
		ISP_LOGE("fail to get handle.");
		return -1;
	}
	if (!param) {
		ISP_LOGE("fail to get param.");
		return -1;
	}

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, SPRD_ISP_IO_SET_STATIS_BUF, param);
	if (ret) {
		ISP_LOGE("fail to set statis_buf %d.", ret);
	}

	return ret;
}

cmr_s32 isp_dev_cfg_start(cmr_handle handle)
{
	cmr_s32 ret = 0;
	cmr_u32 isp_id = 0;
	struct isp_file *file = NULL;

	if (!handle) {
		ISP_LOGE("fail to get handle.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	isp_id = file->isp_id;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_START, &isp_id);
	if (ret) {
		ISP_LOGE("fail to do cfg start.");
	}

	return ret;
}

cmr_s32 isp_dev_raw_proc(cmr_handle handle, void *param)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;

	if (!handle || !param) {
		ISP_LOGE("fail to get handle.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	ret = ioctl(file->fd, SPRD_ISP_IO_RAW_CAP, param);
	if (ret) {
		ISP_LOGE("fail to set statis_buf %d.", ret);
	}

	return ret;
}
