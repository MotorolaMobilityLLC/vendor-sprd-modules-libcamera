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
#ifndef _ISP_IOCTRL_H_
#define _ISP_IOCTRL_H_

#include <sys/types.h>
#include "isp_com.h"
#include "cmr_type.h"
#include "isp_pm.h"

io_fun _ispGetIOCtrlFun(enum isp_ctrl_cmd cmd);

#endif
