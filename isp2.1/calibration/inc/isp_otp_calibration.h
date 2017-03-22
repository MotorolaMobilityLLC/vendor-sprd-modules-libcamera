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
/*------------------------------------------------------------------------------*
*				Dependencies					*
*-------------------------------------------------------------------------------*/
#ifndef _ISP_CALIBRATION_H_
#define _ISP_CALIBRATION_H_

#include "isp_otp_type.h"
#include "isp_type.h"

/*------------------------------------------------------------------------------*
*				Compiler Flag					*
*-------------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"
{
#endif
/*------------------------------------------------------------------------------*
				Micro Define					*
*-------------------------------------------------------------------------------*/
#define ISP_CALIBRATION_MAX_LSC_NUM 10

///////////////////////////////////////////////////////////////////////////////////

/*------------------------------------------------------------------------------*
*				Data Structures					*
*-------------------------------------------------------------------------------*/
struct isp_data_t {
	cmr_u32 size;
	void *data_ptr;
};

struct isp_cali_param {
	struct isp_data_t lsc_otp;
	struct isp_data_t awb_otp;
	struct isp_data_t golden;
	struct isp_data_t target_buf;
	/*0: gr, 1:r, 2: b, 3: gb*/
	cmr_u32 image_pattern;
};

struct isp_cali_info_t {
	cmr_u32 size;
};

struct isp_cali_awb_info {
	cmr_u32 verify[2];
	cmr_u16 golden_avg[4];
	cmr_u16 ramdon_avg[4];
};

struct isp_cali_lsc_map{
	cmr_u32 ct;
	cmr_u32 width;
	cmr_u32 height;
	cmr_u32 grid;
	cmr_u32 len;
	cmr_u32 offset;
};

struct isp_cali_lsc_info {
	cmr_u32 verify[2];
	cmr_u32 num;
	struct isp_cali_lsc_map map[ISP_CALIBRATION_MAX_LSC_NUM];
	void *data_area;
};

struct isp_cali_awb_gain {
	cmr_u32 r;
	cmr_u32 g;
	cmr_u32 b;
};

struct isp_cali_flash_info {
	struct isp_cali_awb_gain awb_gain;
	struct isp_cali_lsc_info lsc;
};

struct isp_otp_init_in {
	isp_handle handle_pm;
	isp_handle lsc_golden_data;
	struct isp_data_info calibration_param;
};

cmr_s32 isp_calibration_get_info(struct isp_data_t *golden_info, struct isp_cali_info_t *cali_info);

cmr_s32 isp_calibration(struct isp_cali_param *param, struct isp_data_t *result);

cmr_s32 isp_parse_calibration_data(struct isp_data_info*cali_data, struct  isp_data_t *lsc, struct isp_data_t *awb );

cmr_s32 isp_parse_flash_data(struct isp_data_t *flash_data, void *lsc_buf, cmr_u32 lsc_buf_size, cmr_u32 image_pattern,
					cmr_u32 gain_width, cmr_u32 gain_height, struct isp_cali_awb_gain *awb_gain);

cmr_int otp_ctrl_init(cmr_handle *isp_otp_handle, struct isp_otp_init_in *input_ptr);
cmr_int otp_ctrl_deinit(cmr_handle isp_handler);

/*------------------------------------------------------------------------------*
*				Compiler Flag					*
*-------------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/*------------------------------------------------------------------------------*/
#endif
// End

