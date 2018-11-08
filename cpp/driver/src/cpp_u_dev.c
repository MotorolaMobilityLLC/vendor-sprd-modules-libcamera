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

 #define LOG_TAG "cpp_u_dev"

#include "cpp_u_dev.h"
#include "sprd_cpp.h"
#include "slice_drv.h"
#include "cpp_u_slice.h"
#include <string.h>
#include <time.h>

static char cpp_dev_name[50] = "/dev/sprd_cpp";

cmr_int cpp_rot_open(cmr_handle *handle)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;
	struct cpp_rot_file *file = NULL;
	cmr_int fd = -1;
	cmr_u32 val = 1;

	file = malloc(sizeof(struct cpp_rot_file));
	if (!file) {
		ret = -CMR_CAMERA_FAIL;
		goto rot_o_out;
	}

	fd = open(cpp_dev_name, O_RDWR, 0);
	if (fd < 0) {
		CMR_LOGE("Fail to open rotation device.\n");
		goto rot_o_free;
	}

	ret = ioctl(fd, SPRD_CPP_IO_OPEN_ROT, &val);
	if (ret) {
		CMR_LOGE("Fail to send SPRD_CPP_IO_OPEN_ROT.\n");
		goto rot_o_free;
	}

	file->fd = fd;
	*handle = (cmr_handle)file;
	goto rot_o_out;

rot_o_free:
	if (fd >= 0) {
		close(fd);
		fd = -1;
	}
	if (file)
		free(file);
	file = NULL;
rot_o_out:

	return ret;
}

cmr_int cpp_rot_close(cmr_handle handle)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;
	struct cpp_rot_file *file = (struct cpp_rot_file *)(handle);

	if (!file)
		goto rot_c_out;

	if (file->fd == -1) {
		CMR_LOGE("Invalid fd\n");
		ret = -CMR_CAMERA_FAIL;
		goto rot_c_free;
	}

	close(file->fd);

rot_c_free:
	free(file);

rot_c_out:
	CMR_LOGI("ret=%ld\n", ret);
	return ret;
}

cmr_int cpp_rot_start(struct cpp_rot_param *rot_param)
{
	struct sprd_cpp_rot_cfg_parm *rot_cfg = NULL;
	cmr_int ret = CMR_CAMERA_SUCCESS;
	cmr_int fd = -1;
	struct cpp_rot_file *file = NULL;

	if (!rot_param) {
		CMR_LOGE("Invalid Param!\n");
		ret = -CMR_CAMERA_FAIL;
		goto rot_s_exit;
	}

	rot_cfg = rot_param->rot_cfg_param;
	if (rot_param->host_fd > 0)
		fd = rot_param->host_fd;
	else {
		file = (struct cpp_rot_file *)rot_param->handle;
		if (!file) {
			CMR_LOGE("Invalid Param rot_file !\n");
			ret = -CMR_CAMERA_FAIL;
			goto rot_s_exit;
		}

		fd = file->fd;
		if (fd < 0) {
			CMR_LOGE("Invalid Param handle!\n");
			ret = -CMR_CAMERA_FAIL;
			goto rot_s_exit;
		}
	}

	ret = ioctl(fd, SPRD_CPP_IO_START_ROT, rot_cfg);
	if (ret) {
		CMR_LOGE("start rot fail. ret = %ld\n", ret);
		ret = -CMR_CAMERA_FAIL;
		goto rot_s_exit;
	}

rot_s_exit:

	CMR_LOGI("rot X ret=%ld", ret);
	return ret;
}

cmr_int cpp_scale_open(cmr_handle *handle)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;
	cmr_int fd = -1;
	struct sc_file *file = NULL;
	cmr_u32 val = 1;

	file = malloc(sizeof(struct sc_file));
	if (!file) {
		CMR_LOGE("scale error: no memory for file\n");
		ret = CMR_CAMERA_NO_MEM;
		goto sc_o_out;
	}

	fd = open(cpp_dev_name, O_RDWR, 0);
	if (fd < 0) {
		CMR_LOGE("Fail to open scale device.\n");
		goto sc_o_free;
	}

	ret = ioctl(fd, SPRD_CPP_IO_OPEN_SCALE, &val);
	if (ret)
		goto sc_o_free;

	file->fd = fd;
	*handle = (cmr_handle)file;

	goto sc_o_out;

sc_o_free:
	if (fd >= 0) {
		close(fd);
		fd = -1;
	}
	if (file)
		free(file);
	file = NULL;
sc_o_out:

	return ret;
}

cmr_int cpp_scale_close(cmr_handle handle)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;
	struct sc_file *file = (struct sc_file *)(handle);

	if (!file) {
		CMR_LOGI("scale fail: file hand is null\n");
		ret = CMR_CAMERA_INVALID_PARAM;
		goto sc_c_out;
	}

	if (-1 == file->fd) {
		CMR_LOGE("Invalid fd\n");
		ret = -CMR_CAMERA_FAIL;
		goto sc_c_free;
	}

	close(file->fd);

sc_c_free:
	free(file);

sc_c_out:
	CMR_LOGI("scale close device exit\n");

	return ret;
}

cmr_int cpp_scale_start(struct cpp_scale_param *scale_param)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;
	cmr_int fd = -1;
	struct sprd_cpp_scale_cfg_parm *sc_cfg = NULL;
	struct sc_file *file = NULL;
#ifdef CPP_LITE_R5P0
	slice_drv_param_t *slice_parm = NULL;
#endif

	if (!scale_param || !scale_param->scale_cfg_param) {
		CMR_LOGE("Invalid Param!\n");
		ret = -CMR_CAMERA_FAIL;
		goto sc_exit;
	}

	sc_cfg = scale_param->scale_cfg_param;
#ifdef CPP_LITE_R5P0
	slice_parm = &scale_param->scale_cfg_param->slice_param;
#endif
	if (scale_param->host_fd > 0)
		fd = scale_param->host_fd;
	else {
		file = (struct sc_file *)scale_param->handle;
		if (!file) {
			CMR_LOGE("Invalid Param sc_file !\n");
			ret = -CMR_CAMERA_FAIL;
			goto sc_exit;
		}
		fd = file->fd;
		if (fd < 0) {
			CMR_LOGE("Invalid Param handle!\n");
			ret = -CMR_CAMERA_FAIL;
			goto sc_exit;
		}
	}
#ifdef CPP_LITE_R5P0
	ret = cpp_u_input_param_check(sc_cfg);
	if (ret) {
		CMR_LOGE("Invalid input Param from cmr_scale!!\n");
		ret = -CMR_CAMERA_FAIL;
		goto sc_exit;
	}
	convert_param_to_calc(sc_cfg, slice_parm);
	slice_drv_param_calc(slice_parm);
#endif
	ret = ioctl(fd, SPRD_CPP_IO_START_SCALE, sc_cfg);
	if (ret) {
		CMR_LOGE("scale done error\n");
		goto sc_exit;
	}

sc_exit:
	CMR_LOGI("scale X ret=%ld\n", ret);
	return ret;
}
