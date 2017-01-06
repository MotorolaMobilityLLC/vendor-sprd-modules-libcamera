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
#ifndef _LIB_CTRL_H_
#define _LIB_CTRL_H_
#include "ae_ctrl_types.h"
#include "AlAwbInterface.h"

//awb start
enum awb_lib_product_id {
	SPRD_AWB_LIB				= 0x0,
	AL_AWB_LIB				= 0x80,
};

enum awb_lib_version_id {
	AWB_LIB_VERSION_0				= 0x0,
	AWB_LIB_VERSION_1				= 0x1,
	AWB_LIB_VERSION_2				= 0x2,
	AWB_LIB_VERSION_3				= 0x3,
	AWB_LIB_VERSION_4				= 0x4,
};

enum al_awb_lib_version_id {
	AL_AWB_LIB_VERSION_0				= 0x0,	//ov13850r2a
	AL_AWB_LIB_VERSION_1				= 0x1,	//t4kb3
	AL_AWB_LIB_VERSION_2				= 0x2,
	AL_AWB_LIB_VERSION_3				= 0x3,
	AL_AWB_LIB_VERSION_4				= 0x4,
};

typedef void* sprd_af_handle_t;
typedef void* sft_af_handle_t;
typedef void* awb_ctrl_handle_t;

struct awb_lib_fun {
	cmr_int (*awb_ctrl_init) (void *in, void *out);
	cmr_int (*awb_ctrl_deinit)(void* handle, void *in, void *out);
	cmr_int (*awb_ctrl_calculation)(void* handle, void *in, void *out);
	cmr_int (*awb_ctrl_ioctrl)(void* handle, cmr_int cmd, void *in, void *out);
};
//awb end

//ae start
enum ae_lib_product_id {
	SPRD_AE_LIB               = 0x0,
	AL_AE_LIB                 = 0x80,
};

enum ae_lib_version_id {
	AE_LIB_VERSION_0				= 0x0,
	AE_LIB_VERSION_1				= 0x1,
	AE_LIB_VERSION_2				= 0x2,
	AE_LIB_VERSION_3				= 0x3,
	AE_LIB_VERSION_4				= 0x4,
};

struct ae_lib_fun {
	cmr_int (*ae_init) (void *in, void *out);
	cmr_int (*ae_deinit)(void* handle, void *in, void *out);
	cmr_int (*ae_calculation)(void* handle, void *in, void *out);
	cmr_int (*ae_io_ctrl)(void* handle, cmr_int cmd, void *in, void *out);
	uint32_t product_id;
	uint32_t version_id;
};
//ae end

//af start
enum af_lib_product_id {
	SPRD_AF_LIB				= 0x0,
	SFT_AF_LIB				= 0x80,
	ALC_AF_LIB				= 0x81,
};

enum af_lib_version_id {
	AF_LIB_VERSION_0				= 0x0,
	AF_LIB_VERSION_1				= 0x1,
	AF_LIB_VERSION_2				= 0x2,
	AF_LIB_VERSION_3				= 0x3,
	AF_LIB_VERSION_4				= 0x4,
};

struct af_lib_fun {
	sprd_af_handle_t (*af_init_interface)(isp_ctrl_context* handle);
	int32_t (*af_calc_interface)(isp_ctrl_context* handle);
	int32_t (*af_deinit_interface)(sft_af_handle_t handle);
	cmr_int (*af_ioctrl_interface)(void *handle,cmr_int cmd,
			void *param0, void *param1);
	int32_t (*af_ioctrl_set_flash_notice)(isp_handle isp_handler,
			void* param_ptr, int(*call_back)());
	int32_t (*af_ioctrl_set_af_info)(isp_handle isp_handler,
			void* param_ptr, int(*call_back)());
	int32_t (*af_ioctrl_get_af_info)(isp_handle isp_handler,
			void* param_ptr, int(*call_back)());
	int32_t (*af_ioctrl_get_af_value)(isp_handle isp_handler,
			void* param_ptr, int(*call_back)());
	int32_t (*af_ioctrl_burst_notice)(isp_handle isp_handler,
			void* param_ptr, int(*call_back)());
	int32_t (*af_ioctrl_set_af_mode)(isp_handle isp_handler,
			void* param_ptr, int(*call_back)());
	int32_t (*af_ioctrl_get_af_mode)(isp_handle isp_handler,
			void* param_ptr, int(*call_back)());
	int32_t (*af_ioctrl_ioread)(isp_handle isp_handler,
			void* param_ptr, int(*call_back)());
	int32_t (*af_ioctrl_iowrite)(isp_handle isp_handler,
			void* param_ptr, int(*call_back)());
	int32_t (*af_ioctrl_set_fd_update)(isp_handle isp_handler,
			void* param_ptr, int(*call_back)());
	int32_t (*af_ioctrl_af_start)(isp_handle isp_handler,
			void* param_ptr, int(*call_back)());
	int32_t (*af_ioctrl_set_isp_start_info)(isp_handle isp_handler,
			struct isp_video_start* param_ptr);
	int32_t (*af_ioctrl_af_info)(isp_handle isp_handler,
			void* param_ptr, int(*call_back)());
	int32_t (*af_ioctrl_set_isp_stop_info)(isp_handle isp_handler);
	int32_t (*af_ioctrl_set_ae_awb_info)(isp_ctrl_context* handle,
			void *ae_result,
			void *awb_result,
			void *bv,
			void *rgb_statistics);
	int32_t (*af_ioctrl_thread_msg_send)(isp_ctrl_context* handle,
			struct ae_calc_out* ae_result,
			struct cmr_msg* msg);
	int32_t (*sft_af_ioctrl_set_fd_update)(isp_handle isp_handler,
			void* param_ptr, int(*call_back)());
	int32_t (*af_image_data_update)(isp_ctrl_context* handle);
	int32_t (*af_ioctrl_get_af_cur_pos)(isp_handle isp_handler,
			void* param_ptr, int(*call_back)());
	int32_t (*af_ioctrl_set_af_pos)(isp_handle isp_handler,
			void* param_ptr, int(*call_back)());
	int32_t (*af_ioctrl_set_af_bypass)(isp_handle isp_handler,
			void* param_ptr, int(*call_back)());
	int32_t (*af_ioctrl_set_af_stop)(isp_handle isp_handler,
			void* param_ptr, int(*call_back)());
	int32_t (*af_ioctrl_set_af_param)(isp_handle isp_handler);
	// if af_posture_support be set ,af_posture_set_handle and af_posture_info_update must also be set
	int32_t (*af_posture_support)(isp_ctrl_context* handle,void* sensordevice);
	int32_t (*af_posture_set_handle)(isp_ctrl_context* handle,uint32_t sensor_type,int sensor_handle);
	int32_t (*af_posture_info_update)(isp_ctrl_context* handle,uint32_t sensor_type,void* data1,void *data2);


};
//af end

struct al_awb_thirdlib_fun {
	void    (*AlAwbInterfaceInit)(TT_AlAwbInterface*        pptInterface);
	void    (*AlAwbInterfaceDestroy)(TT_AlAwbInterface*     pptInterface);
	void    (*AlAwbInterfaceReset)(TT_AlAwbInterface*       pptInterface);
	void    (*AlAwbInterfaceMain)(TT_AlAwbInterface*        pptInterface);
	UI_16   (*AlAwbInterfaceSendCommand)(TT_AlAwbInterface* pptInterface, TT_AlAisCmd* pptCmd);
	void    (*AlAwbInterfaceShowVersion)(TT_AlAwbInterface* pptInterface);
};

enum lsc_lib_product_id{
	SPRD_LSC_LIB				= 0x0,
};

enum lsc_lib_id{
	LSC_LIB_VERSION_0 = 0x01,
	LSC_LIB_VERSION_1 = 0x02,
	LSC_LIB_VERSION_2 = 0x03,
};

struct lsc_lib_fun{
	int32_t (* alsc_calc)(void* handle,
						struct lsc_adv_calc_param *param,
						struct lsc_adv_calc_result *adv_calc_result);
	void* (* alsc_init)(struct lsc_adv_init_param *param);

	int32_t (* alsc_deinit)(void* handle);
	int32_t (*alsc_io_ctrl)(void* handler, enum alsc_io_ctrl_cmd cmd, void *in_param,
			void *out_param);
	uint32_t product_id;
	uint32_t version_id;
};

uint32_t isp_awblib_init(struct sensor_libuse_info* libuse_info,
		struct awb_lib_fun* awb_lib_fun);
uint32_t isp_aelib_init(struct sensor_libuse_info* libuse_info,
		struct ae_lib_fun* ae_lib_fun);
uint32_t isp_aflib_init(struct sensor_libuse_info* libuse_info,
		struct af_lib_fun* af_lib_fun);

uint32_t isp_lsclib_init(struct sensor_libuse_info* libuse_info, struct lsc_lib_fun* lsc_lib_fun);

uint32_t aaa_lib_init(isp_ctrl_context* handle, struct sensor_libuse_info* libuse_info);
#endif
