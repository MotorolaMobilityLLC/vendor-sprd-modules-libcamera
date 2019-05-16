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
#ifndef _CPP_U_DRV_H_
#define _CPP_U_DRV_H_
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include "sprd_cpp.h"
#include "cmr_type.h"
#include "slice_drv.h"
#include "cmr_common.h"

struct cpp_rot_file {
	cmr_int fd;
};

struct cpp_rot_param {
	cmr_handle handle;
	cmr_int host_fd;
	struct sprd_cpp_rot_cfg_parm *rot_cfg_param;
};

struct cpp_scale_param {
	cmr_handle handle;
	cmr_int	host_fd;
	struct sprd_cpp_scale_cfg_parm *scale_cfg_param;
};

struct sc_file {
	cmr_int fd;
};

cmr_int cpp_rot_open(cmr_handle *handle);
cmr_int cpp_rot_close(cmr_handle handle);
cmr_int cpp_rot_start(struct cpp_rot_param *rot_param);
cmr_int cpp_scale_open(cmr_handle *handle);
cmr_int cpp_scale_close(cmr_handle handle);
cmr_int cpp_scale_start(struct cpp_scale_param *scale_param);

#endif

