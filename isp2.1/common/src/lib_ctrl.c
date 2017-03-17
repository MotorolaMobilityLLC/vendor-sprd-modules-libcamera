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

#include "isp_com.h"
#include "awb_ctrl.h"
#include "ae_sprd_ctrl.h"
#include "af_ctrl.h"
#include "awb_al_ctrl.h"
#include "awb_sprd_ctrl.h"
#include "sensor_raw.h"
#include "ae_log.h"
#include "af_log.h"
#include "lib_ctrl.h"
#include "af_sprd_ctrl.h"
#include "sp_af_ctrl.h"
#include <dlfcn.h>
#include "ALC_AF_Ctrl.h"

#ifdef CONFIG_USE_ALC_AE
#include "ae_alc_ctrl.h"
#endif

struct awb_lib_fun awb_lib_fun;
struct ae_lib_fun ae_lib_fun;
struct af_lib_fun af_lib_fun;
struct al_awb_thirdlib_fun al_awb_thirdlib_fun;

char* al_libversion_choice(uint32_t version_id)
{
	ISP_LOGE("E");
	switch (version_id)
	{
		case AL_AWB_LIB_VERSION_0:
			return "/system/lib/libAl_Awb_ov13850r2a.so";
		case AL_AWB_LIB_VERSION_1:
			return "/system/lib/libAl_Awb_t4kb3.so";
	}

	return NULL;
}

uint32_t al_awb_lib_open(uint32_t version_id)
{
	void *handle;
	char *AWB_LIB;
	ISP_LOGE("E");
	AWB_LIB = al_libversion_choice(version_id);
	handle = dlopen(AWB_LIB, RTLD_NOW);
	if(NULL == handle) {
		ISP_LOGE("dlopen get handle error\n");
		return ISP_ERROR;
	}
	al_awb_thirdlib_fun.AlAwbInterfaceInit = (void *)dlsym(handle, "AlAwbInterfaceInit");
	al_awb_thirdlib_fun.AlAwbInterfaceDestroy =
		(void *)dlsym(handle, "AlAwbInterfaceDestroy");
	al_awb_thirdlib_fun.AlAwbInterfaceReset = (void *)dlsym(handle, "AlAwbInterfaceReset");
	al_awb_thirdlib_fun.AlAwbInterfaceMain =
		(void *)dlsym(handle, "AlAwbInterfaceMain");
	al_awb_thirdlib_fun.AlAwbInterfaceSendCommand =
		(void *)dlsym(handle, "AlAwbInterfaceSendCommand");
	al_awb_thirdlib_fun.AlAwbInterfaceShowVersion =
		(void *)dlsym(handle, "AlAwbInterfaceShowVersion");
	ISP_LOGE("awb_thirdlib_fun.AlAwbInterfaceInit= %p", al_awb_thirdlib_fun.AlAwbInterfaceInit);
	ISP_LOGE("AWB_LIB= %s", AWB_LIB);

	return 0;
}

uint32_t isp_awblib_init(struct sensor_libuse_info* libuse_info, struct awb_lib_fun* awb_lib_fun)
{
	uint32_t rtn = AWB_CTRL_SUCCESS;
	struct third_lib_info 	awb_lib_info;
	uint32_t awb_producer_id  	= 0;
	uint32_t awb_lib_version  	= 0;
	uint32_t al_awb_lib_version  	= 0;

	ISP_LOGE("E");
	if (libuse_info) {
		awb_lib_info = libuse_info->awb_lib_info;
		awb_producer_id = awb_lib_info.product_id;
		awb_lib_version = awb_lib_info.version_id;
		ISP_LOGE("awb_producer_id= ____0x%x", awb_producer_id);
		ISP_LOGE("awb_lib_version= ____0x%x", awb_lib_version);
	}

	switch (awb_producer_id)
	{
	case SPRD_AWB_LIB:
		switch (awb_lib_version)
		{
		case AWB_LIB_VERSION_0:
			//awb_lib_fun->awb_ctrl_init 		= awb_sprd_ctrl_init;
			//awb_lib_fun->awb_ctrl_deinit		= awb_sprd_ctrl_deinit;
			//awb_lib_fun->awb_ctrl_calculation	= awb_sprd_ctrl_calculation;
			//awb_lib_fun->awb_ctrl_ioctrl		= awb_sprd_ctrl_ioctrl;
			break;
		case AWB_LIB_VERSION_1:
		default :
			AWB_CTRL_LOGE("awb invalid lib version = 0x%x", awb_lib_version);
			rtn = AWB_CTRL_ERROR;
		}
		break;

	case AL_AWB_LIB:
		al_awb_lib_version = awb_lib_version;
		al_awb_lib_open(al_awb_lib_version);
		awb_lib_fun->awb_ctrl_init 		= awb_al_ctrl_init;
		awb_lib_fun->awb_ctrl_deinit		= awb_al_ctrl_deinit;
		awb_lib_fun->awb_ctrl_calculation	= awb_al_ctrl_calculation;
		awb_lib_fun->awb_ctrl_ioctrl		= awb_al_ctrl_ioctrl;
		break;

	default:
		AWB_CTRL_LOGE("awb invalid lib producer id = 0x%x", awb_producer_id);
		rtn = AWB_CTRL_ERROR;
	}

	return rtn;
}

uint32_t isp_aelib_init(struct sensor_libuse_info* libuse_info, struct ae_lib_fun* ae_lib_fun)
{
	uint32_t rtn = AE_SUCCESS;
	struct third_lib_info 	ae_lib_info;
	uint32_t ae_producer_id  	= 0;
	uint32_t ae_lib_version  	= 0;

	AE_LOGE("E");
	if (libuse_info) {
		ae_lib_info  = libuse_info->ae_lib_info;
		ae_producer_id = ae_lib_info.product_id;
		ae_lib_version = ae_lib_info.version_id;
		AE_LOGE("ae_producer_id= ____0x%x", ae_producer_id);
	}

	ae_lib_fun->product_id = ae_producer_id;
	ae_lib_fun->version_id = ae_lib_version;

	switch (ae_producer_id)
	{
	case SPRD_AE_LIB:
		switch (ae_lib_version)
		{
		case AE_LIB_VERSION_0:
			break;
		case AE_LIB_VERSION_1:
		default :
			AE_LOGE("ae invalid lib version = 0x%x", ae_lib_version);
			rtn = AE_ERROR;
		}
		break;
	default:
		AE_LOGE("ae invalid lib producer id = 0x%x", ae_producer_id);
		rtn = AE_ERROR;
	}

	return rtn;
}

uint32_t isp_aflib_init(struct sensor_libuse_info* libuse_info, struct af_lib_fun* af_lib_fun)
{
	uint32_t rtn = AF_SUCCESS;
	struct third_lib_info af_lib_info;
	uint32_t af_producer_id  	= 0;
	uint32_t af_lib_version  	= 0;

	AF_LOGE("E");
	if (libuse_info) {
		af_lib_info  = libuse_info->af_lib_info;
		af_producer_id = af_lib_info.product_id;
		af_lib_version = af_lib_info.version_id;
		AF_LOGE("af_producer_id= ____0x%x", af_producer_id);
	}
	AF_LOGE("af_producer_id= ____0x%x", af_producer_id);
	memset(af_lib_fun,0,sizeof(struct af_lib_fun));
	switch (af_producer_id)
	{
	case SPRD_AF_LIB:
		switch (af_lib_version)
		{
		case AF_LIB_VERSION_0:
			break;
		case AF_LIB_VERSION_1:
		default :
			AF_LOGE("af invalid lib version = 0x%x", af_lib_version);
			rtn = AF_ERROR;
		}
		break;
	case SFT_AF_LIB:
		switch (af_lib_version)
		{
		case AF_LIB_VERSION_0:
#if 0
			af_lib_fun->af_init_interface		= sft_af_init;
			af_lib_fun->af_calc_interface		= sft_af_calc;
			af_lib_fun->af_deinit_interface		= sft_af_deinit;
			af_lib_fun->af_ioctrl_interface		= sft_af_ioctrl;
			af_lib_fun->af_ioctrl_set_flash_notice	= sft_af_ioctrl_set_flash_notice;
			af_lib_fun->af_ioctrl_get_af_value	= sft_af_ioctrl_get_af_value;
			af_lib_fun->af_ioctrl_burst_notice	= sft_af_ioctrl_burst_notice;
			af_lib_fun->af_ioctrl_set_af_mode	= sft_af_ioctrl_set_af_mode;
			af_lib_fun->af_ioctrl_get_af_mode	= sft_af_ioctrl_get_af_mode;
			af_lib_fun->af_ioctrl_ioread		= sft_af_ioctrl_ioread;
			af_lib_fun->af_ioctrl_iowrite		= sft_af_ioctrl_iowrite;
			af_lib_fun->af_ioctrl_set_fd_update	= sft_af_ioctrl_set_fd_update;
			af_lib_fun->af_ioctrl_af_start		= sft_af_ioctrl_af_start;
			af_lib_fun->af_ioctrl_set_isp_start_info	= sft_af_ioctrl_set_isp_start_info;
			af_lib_fun->af_ioctrl_set_isp_stop_info	= sft_af_ioctrl_set_isp_stop_info;
			af_lib_fun->af_ioctrl_set_ae_awb_info	= sft_af_ioctrl_set_ae_awb_info;
			af_lib_fun->af_ioctrl_get_af_cur_pos	= sft_af_ioctrl_get_af_cur_pos;
			af_lib_fun->af_ioctrl_set_af_pos	= sft_af_ioctrl_set_af_pos;
			af_lib_fun->af_ioctrl_set_af_bypass	= sft_af_ioctrl_set_af_bypass;
			af_lib_fun->af_ioctrl_set_af_stop	= sft_af_ioctrl_set_af_stop;
			af_lib_fun->af_ioctrl_set_af_param = sft_af_ioctrl_set_af_param;
#endif
			break;
		case AF_LIB_VERSION_1:
		default :
			AF_LOGE("af invalid lib version = 0x%x", af_lib_version);
			rtn = AF_ERROR;
		}
		break;
	case ALC_AF_LIB:
		switch (af_lib_version)
		{
		case AF_LIB_VERSION_0:
#if 0
			af_lib_fun->af_init_interface		= alc_af_init;
			af_lib_fun->af_calc_interface		= alc_af_calc;
			af_lib_fun->af_deinit_interface		= alc_af_deinit;
			af_lib_fun->af_ioctrl_interface		= alc_af_ioctrl;
			af_lib_fun->af_ioctrl_set_af_mode	= alc_af_ioctrl_set_af_mode;
			af_lib_fun->af_ioctrl_set_fd_update	= alc_af_ioctrl_set_fd_update;
			af_lib_fun->af_ioctrl_af_start		= alc_af_ioctrl_af_start;
			af_lib_fun->af_ioctrl_set_isp_start_info	= alc_af_ioctrl_set_isp_start_info;
			af_lib_fun->af_ioctrl_set_ae_awb_info	= alc_af_ioctrl_set_ae_awb_info;
#endif
		case AF_LIB_VERSION_1:
		default :
			AF_LOGE("af invalid lib version = 0x%x", af_lib_version);
		}
		break;
	default:
		AF_LOGE("af invalid lib producer id = 0x%x", af_producer_id);
		rtn = AF_ERROR;
	}

	return rtn;
}

uint32_t aaa_lib_init(isp_ctrl_context* handle, struct sensor_libuse_info* libuse_info)
{
	memset(&awb_lib_fun, 0x00, sizeof(awb_lib_fun));
	memset(&ae_lib_fun, 0x00, sizeof(ae_lib_fun));
	memset(&af_lib_fun, 0x00, sizeof(af_lib_fun));

	isp_awblib_init(libuse_info, &awb_lib_fun);
	handle->awb_lib_fun = &awb_lib_fun;
	isp_aelib_init(libuse_info, &ae_lib_fun);
	handle->ae_lib_fun = &ae_lib_fun;
	isp_aflib_init(libuse_info, &af_lib_fun);
	handle->af_lib_fun = &af_lib_fun;

	return 0;
}
