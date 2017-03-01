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
#ifndef _ISP_ALG_FW_H_
#define _ISP_ALG_FW_H_
/*----------------------------------------------------------------------------*
**				Dependencies					*
**---------------------------------------------------------------------------*/

#include <sys/types.h>
#include "isp_com.h"
#include "cmr_type.h"
#include "isp_pm.h"
#include "sprd_isp_r6p10.h"

struct commn_info {
	cmr_s32 isp_mode;
	cmr_u32 mode_flag;
	cmr_u32 multi_nr_flag;
	cmr_u32 scene_flag;
	cmr_u32 image_pattern;
	cmr_u32 param_index;
	cmr_u32 isp_callback_bypass;
	proc_callback callback;
	cmr_handle caller_id;
	uint8_t *log_isp;
	uint32_t log_isp_size;
	struct isp_size src;
	struct isp_ops ops;
	struct isp_interface_param_v1 interface_param_v1;
	struct sensor_raw_resolution_info input_size_trim[ISP_INPUT_SIZE_NUM_MAX];
};

struct ae_info {
	cmr_handle handle;
	struct isp_time time;
	uint8_t* log_alc_ae;
	uint32_t log_alc_ae_size;
	uint8_t* log_alc;
	uint32_t log_alc_size;
	uint8_t *log_ae;
	uint32_t log_ae_size;
	cmr_uint vir_addr;
	cmr_int buf_size;
	cmr_int buf_num;
	cmr_uint phy_addr;
	cmr_uint mfd;
	cmr_int buf_property;
	void*  buffer_client_data;
};

struct awb_info {
	cmr_handle handle;
	uint32_t alc_awb;
	int awb_pg_flag;
	uint8_t* log_alc_awb;
	uint32_t log_alc_awb_size;
	uint8_t* log_alc_lsc;
	uint32_t log_alc_lsc_size;
	uint8_t* log_awb;
    uint32_t log_awb_size;
};

struct smart_info {
	cmr_handle handle;
	uint32_t isp_smart_eb;
	uint8_t *log_smart;
	uint32_t log_smart_size;
	uint8_t lock_en;
};

struct afl_info {
	cmr_handle handle;
	cmr_uint vir_addr;
	cmr_int buf_size;
	cmr_int buf_num;
	cmr_uint phy_addr;
	cmr_u32 afl_mode;
	cmr_uint mfd;
	cmr_int buf_property;
	void*  buffer_client_data;
};

struct af_info {
	cmr_handle handle;
	uint8_t *log_af;
	uint32_t log_af_size;
};

struct lsc_info {
	cmr_handle handle;
	void * lsc_tab_address;
	uint32_t lsc_tab_size;
	uint32_t isp_smart_lsc_lock;
	uint8_t *log_lsc;
	uint32_t log_lsc_size;
};

struct isp_alg_fw_context {
	cmr_int camera_id;
	cmr_u8 aem_is_update;
	struct isp_awb_statistic_info aem_stats;
	struct commn_info commn_cxt;
	struct sensor_data_info sn_cxt;
	struct ae_info ae_cxt;
	struct awb_info awb_cxt;
	struct smart_info smart_cxt;
	struct af_info af_cxt;
	struct lsc_info lsc_cxt;
	struct afl_info afl_cxt;
	struct sensor_libuse_info *lib_use_info;
	struct sensor_raw_ioctrl* ioctrl_ptr;
	cmr_handle thr_handle;
	cmr_handle dev_access_handle;
	cmr_handle handle_pm;
	cmr_handle handle_otp;

	uint32_t gamma_sof_cnt;
	uint32_t gamma_sof_cnt_eb;
	uint32_t update_gamma_eb;
	struct isp_sensor_fps_info sensor_fps;
};

struct isp_alg_fw_init_in {
	cmr_handle dev_access_handle;
	struct isp_init_param *init_param;
};

cmr_int isp_alg_fw_init(struct isp_alg_fw_init_in *input_ptr, cmr_handle *isp_alg_handle);
cmr_int isp_alg_fw_deinit(cmr_handle isp_alg_handle);
cmr_int isp_alg_fw_ioctl(isp_handle isp_alg_handle, enum isp_ctrl_cmd io_cmd, void *param_ptr, int (*call_back)());
cmr_int isp_alg_fw_start(cmr_handle isp_alg_handle, struct isp_video_start *in_ptr);
cmr_int isp_alg_fw_stop(cmr_handle isp_alg_handle);
cmr_int isp_alg_proc_start(cmr_handle isp_alg_handle, struct ips_in_param *in_ptr);
cmr_int isp_alg_proc_next(cmr_handle isp_alg_handle, struct ipn_in_param *in_ptr);
cmr_int isp_alg_fw_capability(cmr_handle isp_alg_handle, enum isp_capbility_cmd cmd, void* param_ptr);

/**---------------------------------------------------------------------------*/

#endif

