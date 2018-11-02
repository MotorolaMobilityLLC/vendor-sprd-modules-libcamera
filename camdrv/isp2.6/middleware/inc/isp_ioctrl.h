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
#ifndef _ISP_IOCTRL_H_
#define _ISP_IOCTRL_H_

typedef cmr_int(*isp_io_fun) (cmr_handle isp_alg_handle, void *param_ptr);

struct isp_io_ctrl_fun {
	enum isp_ctrl_cmd cmd;
	isp_io_fun io_ctrl;
};

isp_io_fun  isp_ioctl_get_fun(enum isp_ctrl_cmd cmd);

#endif
