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

#ifdef WIN32
#define ALC_AF_LOG
#define ALC_AF_LOGW
#define ALC_AF_LOGI
#define ALC_AF_LOGD
#define ALC_AF_LOGV
#else
#define ALC_AF_DEBUG_STR     "ALC_AF: %d, %s: "
#define ALC_AF_DEBUG_ARGS    __LINE__,__FUNCTION__

#define ALC_AF_LOG(format,...) ALOGE(ALC_AF_DEBUG_STR format, ALC_AF_DEBUG_ARGS, ##__VA_ARGS__)
#define ALC_AF_LOGE(format,...) ALOGE(ALC_AF_DEBUG_STR format, ALC_AF_DEBUG_ARGS, ##__VA_ARGS__)
#define ALC_AF_LOGW(format,...) ALOGW(ALC_AF_DEBUG_STR format, ALC_AF_DEBUG_ARGS, ##__VA_ARGS__)
#define ALC_AF_LOGI(format,...) ALOGI(ALC_AF_DEBUG_STR format, ALC_AF_DEBUG_ARGS, ##__VA_ARGS__)
#define ALC_AF_LOGD(format,...) ALOGD(ALC_AF_DEBUG_STR format, ALC_AF_DEBUG_ARGS, ##__VA_ARGS__)
#define ALC_AF_LOGV(format,...) ALOGV(ALC_AF_DEBUG_STR format, ALC_AF_DEBUG_ARGS, ##__VA_ARGS__)
#endif

#ifndef _ALC_AF_CTRL_H_
#define _ALC_AF_CTRL_H_
#include <sys/types.h>
#include "isp_com.h"
#include "ALC_AF_Data.h"
#include "ALC_AF_Common.h"
#include "AfIF.h"//tes_kano_0822

//#define ALC_AF_LOGE af_ops->cb_alc_af_log
struct alc_af_context {
	cmr_u32 magic_start;
	void *isp_handle;
	cmr_u32 bypass;
	cmr_u32 active_win;
	cmr_u32 is_inited;
	cmr_u32 ae_is_stab;
	cmr_u32 ae_cur_lum;
	cmr_u32 ae_is_locked;
	cmr_u32 awb_is_stab;
	cmr_u32 awb_is_locked;
	cmr_u32 cur_env_mode;
	cmr_u32 af_mode;
	cmr_u32 cur_pos;
	cmr_u32 is_runing;

	struct alc_afm_cfg_info	sprd_filter;
	struct alc_af_ctrl_ops af_ctrl_ops;
	cmr_u32 touch_win_cnt;
	struct alc_win_coord win_pos[25];
	cmr_u32 cur_awb_r_gain;
	cmr_u32 cur_awb_g_gain;
	cmr_u32 cur_awb_b_gain;
	cmr_u32 cur_fps;
	cmr_u32 cur_frame_time;
	cmr_u32 cur_exp_time;
	cmr_u32 caf_active;
	cmr_u32 flash_on;
	struct alc_af_face_area fd_info;
	cmr_u32 cur_ae_again;
	cmr_u32 cur_ae_dgain;
	cmr_u32 cur_ae_iso;
	cmr_u32 cur_ae_bv;
	cmr_u32 cur_ae_ev;
	cmr_u32 magic_end;
	TT_AfIfBuf ttAfIfBuf;//tes_kano_0822
	
	//tes_kano_0902
	cmr_u32 r_info[1024];
	cmr_u32 g_info[1024];
	cmr_u32 b_info[1024];
	cmr_u32 touch_af;
	cmr_u32 fd_af_start_cnt ;
	
	cmr_u32 sensor_w;
	cmr_u32 sensor_h;
};

////////////////////////// Funcs //////////////////////////


///////////////////// for System ///////////////
alc_af_handle_t alc_af_init(void* isp_handle);
cmr_s32 alc_af_deinit(void* isp_handle);
cmr_s32 alc_af_calc(isp_ctrl_context* handle);
cmr_s32 alc_af_ioctrl(alc_af_handle_t handle, enum alc_af_cmd cmd,
				void *param0, void *param1);


cmr_s32 alc_af_ioctrl_af_start(isp_handle isp_handler, void* param_ptr, cmr_s32(*call_back)());

cmr_s32 alc_af_ioctrl_set_fd_update(isp_handle isp_handler, void* param_ptr, cmr_s32(*call_back)());

cmr_s32 alc_af_ioctrl_set_isp_start_info(isp_handle isp_handler, struct isp_video_start* param_ptr);

cmr_s32 alc_af_ioctrl_set_ae_awb_info(isp_ctrl_context* handle,
		void* ae_result,
		void* awb_result,
		void* bv,
		void *rgb_statistics);

cmr_s32 alc_af_ioctrl_set_af_mode(isp_handle isp_handler, void* param_ptr, cmr_s32(*call_back)());

void Al_AF_Running(alc_af_handle_t handle , TT_AfIfBuf* ppAfIfBuf);
void Al_AF_Prv_Start_Notice(alc_af_handle_t handle , TT_AfIfBuf *ppAfIfBuf);

#endif
