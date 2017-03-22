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
#ifndef _ISP_OTP_H
#define _ISP_OTP_H

#include <stdio.h>

cmr_s32  start_camera(void  *pdev);
cmr_s32  stop_camera(void  *pdev);

cmr_s32 isp_otp_needstopprev(cmr_u8 *data_buf, cmr_u32 *data_size);
cmr_s32 isp_otp_write(cmr_handle isp_handler, cmr_u8 *buf, cmr_u32 *len);
cmr_s32 isp_otp_read(cmr_handle isp_handler, cmr_u8 *data_buf, cmr_u32 *data_size);

cmr_s32 read_sensor_shutter(cmr_u32 *shutter_val);
cmr_s32 read_sensor_gain(cmr_u32 *gain_val);
cmr_s32 read_position(cmr_handle handler, cmr_u32 *pos);
cmr_s32 read_otp_awb_gain(cmr_handle handler, void *awbc_cfg);
#endif
