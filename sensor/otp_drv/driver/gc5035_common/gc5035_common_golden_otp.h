/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * V1.0
 */
 /*History
 *Date                  Modification                                 Reason
 *
 */

#ifndef GC5035_COMMON_GOLDEN_OTP_H_
#define GC5035_COMMON_GOLDEN_OTP_H_

#include "otp_common.h"

static awb_target_packet_t gc5035_common_awb[AWB_MAX_LIGHT] = {
	{
		.R = 0x16e, .G = 0x2c1, .B = 0x1c4,
		.rg_ratio= 0x54f, .bg_ratio= 0x489, .GrGb_ratio = 0x400,
	},
};

static uint16_t gc5035_common_lsc[] = {

	};

#endif
