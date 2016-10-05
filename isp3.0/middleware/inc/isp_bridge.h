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
#ifndef _ISP_BRIDGE_H_
#define _ISP_BRIDGE_H_
/*----------------------------------------------------------------------------*
**				Dependencies					*
**---------------------------------------------------------------------------*/

#include "isp_common_types.h"

cmr_int isp_br_init(cmr_int sensor_id, cmr_handle isp_3a_handle);
cmr_int isp_br_deinit(void);
cmr_int isp_br_ioctrl(cmr_int sensor_id, cmr_int cmd, void *in, void *out);

#endif
