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
#ifndef _ALC_AF_COMMON_H_
#define _ALC_AF_COMMON_H_
#include "cmr_types.h"

typedef void* alc_af_handle_t;
#define ALC_AF_MAGIC_START		0xe5a55e5a
#define ALC_AF_MAGIC_END		0x5e5ae5a5

enum alc_af_cmd {
	ALC_AF_CMD_SET_BASE 			= 0x1000,
	ALC_AF_CMD_SET_AF_MODE			= 0x1001,
	ALC_AF_CMD_SET_AF_POS			= 0x1002,
	ALC_AF_CMD_SET_SCENE_MODE		= 0x1003,
	ALC_AF_CMD_SET_AF_START			= 0x1004,
	ALC_AF_CMD_SET_AF_STOP			= 0x1005,
	ALC_AF_CMD_SET_AF_BYPASS		= 0x1006,
	ALC_AF_CMD_SET_AF_INFO			= 0x1007,
	ALC_AF_CMD_SET_ISP_START_INFO		= 0x1008,
	ALC_AF_CMD_SET_ISP_STOP_INFO		= 0x1009,
	ALC_AF_CMD_SET_AE_INFO			= 0x100A,
	ALC_AF_CMD_SET_AWB_INFO			= 0x100B,
	ALC_AF_CMD_SET_FLASH_NOTICE		= 0x100C,
	ALC_AF_CMD_SET_FD_UPDATE		= 0x100D,

	
	ALC_AF_CMD_GET_BASE			= 0x2000,
	ALC_AF_CMD_GET_AF_MODE			= 0X2001,
	ALC_AF_CMD_GET_AF_CUR_POS		= 0x2002,
	ALC_AF_CMD_GET_AF_INFO			= 0x2003,
	ALC_AF_CMD_GET_AF_VALUE			= 0x2004,

	ALC_AF_CMD_BURST_NOTICE = 0x2005,
	ALC_AF_CMD_GET_AF_MULTIWIN      	= 0x2006,
};

enum alc_af_mode{
	ALC_AF_MODE_NORMAL=0x00,
	ALC_AF_MODE_MACRO,
	ALC_AF_MODE_MANUAL,
	ALC_AF_MODE_CAF,
	ALC_AF_MODE_VIDEO_CAF,
	ALC_AF_MODE_MULTIWIN,// continue af and single af could work
	ALC_AF_MODE_MAX
};


enum alc_af_rtn {
	ALC_AF_SUCCESS = 0,
	ALC_AF_ERROR = 255
};

enum alc_af_env_mode {
	ALC_AF_OUTDOOR = 0,
	ALC_AF_INDOOR = 1,
	ALC_AF_DARK = 2,
};

struct alc_win_coord {
	cmr_u32 start_x;
	cmr_u32 start_y;
	cmr_u32 end_x;
	cmr_u32 end_y;
} ;

 struct alc_af_ctrl_ops{
	cmr_s32 (*cb_set_afm_bypass)(alc_af_handle_t handle, cmr_u32 bypass);
	cmr_s32 (*cb_set_afm_mode)(alc_af_handle_t handle, cmr_u32 mode);
	cmr_s32 (*cb_set_afm_skip_num)(alc_af_handle_t handle, cmr_u32 skip_num);
	cmr_s32 (*cb_set_afm_skip_num_clr)(alc_af_handle_t handle);
	cmr_s32 (*cb_set_afm_spsmd_rtgbot_enable)(alc_af_handle_t handle, cmr_u32 enable);
	cmr_s32 (*cb_set_afm_spsmd_diagonal_enable)(alc_af_handle_t handle, cmr_u32 enable);
	cmr_s32 (*cb_set_afm_spsmd_cal_mode)(alc_af_handle_t handle, cmr_u32 mode);
	cmr_s32 (*cb_set_afm_sel_filter1)(alc_af_handle_t handle, cmr_u32 sel_filter);
	cmr_s32 (*cb_set_afm_sel_filter2)(alc_af_handle_t handle, cmr_u32 sel_filter);
	cmr_s32 (*cb_set_afm_sobel_type)(alc_af_handle_t handle, cmr_u32 type);
	cmr_s32 (*cb_set_afm_spsmd_type)(alc_af_handle_t handle, cmr_u32 type);
	cmr_s32 (*cb_set_afm_sobel_threshold)(alc_af_handle_t handle, cmr_u32 min, cmr_u32 max);
	cmr_s32 (*cb_set_afm_spsmd_threshold)(alc_af_handle_t handle, cmr_u32 min, cmr_u32 max);
	cmr_s32 (*cb_set_afm_slice_size)(alc_af_handle_t handle, cmr_u32 width, cmr_u32 height);
	cmr_s32 (*cb_set_afm_win)(alc_af_handle_t handle, struct alc_win_coord *win_range);
	cmr_s32 (*cb_get_afm_type1_statistic)(alc_af_handle_t handle, cmr_u32 *statis);
	cmr_s32 (*cb_get_afm_type2_statistic)(alc_af_handle_t handle, cmr_u32 *statis);
	cmr_s32 (*cb_set_active_win)(alc_af_handle_t handle, cmr_u32 active_win);
	cmr_s32 (*cb_get_cur_env_mode)(alc_af_handle_t handle, cmr_u8 *mode);
	cmr_s32 (*cb_set_motor_pos)(alc_af_handle_t handle, cmr_u32 pos);
	cmr_s32 (*cb_lock_ae)(alc_af_handle_t handle);
	cmr_s32 (*cb_lock_awb)(alc_af_handle_t handle);
	cmr_s32 (*cb_unlock_ae)(alc_af_handle_t handle);
	cmr_s32 (*cb_unlock_awb)(alc_af_handle_t handle);
	cmr_u32 (*cb_get_ae_lum)(alc_af_handle_t handle);
	cmr_u32 (*cb_get_ae_status)(alc_af_handle_t handle);
	cmr_u32 (*cb_get_awb_status)(alc_af_handle_t handle);
	cmr_s32 (*cb_get_isp_size)(alc_af_handle_t handle, cmr_u16 *widith, cmr_u16 *height);
	cmr_s32 (*cb_af_finish_notice)(alc_af_handle_t handle, cmr_u32 result);
	void (*cb_alc_af_log)(const char* format, ...);
	cmr_s32 (*cb_set_sp_afm_cfg)(alc_af_handle_t handle);
	cmr_s32 (*cb_set_sp_afm_win)(alc_af_handle_t handle, struct alc_win_coord *win_range);
	cmr_s32 (*cb_get_sp_afm_statistic)(alc_af_handle_t handle, cmr_u32 *statis);
	cmr_s32 (*cb_sp_write_i2c)(alc_af_handle_t handle,cmr_u16 slave_addr, cmr_u8 *cmd, cmr_u16 cmd_length);
	cmr_s32 (*cd_sp_read_i2c)(alc_af_handle_t handle,cmr_u16 slave_addr, cmr_u8 *cmd, cmr_u16 cmd_length);
	cmr_s32 (*cd_sp_get_cur_prv_mode)(alc_af_handle_t handle,cmr_u32 *mode);
	cmr_s32 (*cb_af_move_start_notice)(alc_af_handle_t handle);
	cmr_s32 (*cb_af_pos_update)(alc_af_handle_t handle, cmr_u32 pos);

	cmr_s32 (*cb_get_motor_range)(alc_af_handle_t handle, cmr_u16* min_pos,cmr_u16* max_pos);
	cmr_s32 (*cb_get_lens_info)(alc_af_handle_t handle, cmr_u16 *f_num, cmr_u16 *f_len);
	cmr_s32 (*cb_get_accelerator_sensor_info)(alc_af_handle_t handle, cmr_u16* posture);
	cmr_s32 (*cb_get_magnetic_sensor_info)(alc_af_handle_t handle, cmr_u16* ispinning);
};

struct alc_afm_cfg_info{
	cmr_u32 bypass;			//   [0] 0:work, 1:bypass 
	cmr_u32 mode;				//   [6] 1
	cmr_u32 source_pos;		//   0:RGB, 1:YCbCr
	cmr_u32 shift;				//   [5:1] ???????
	cmr_u32 skip_num;			//   [10:7] 0
	cmr_u32 skip_num_clear;	//   [11] 0
	cmr_u32 format;			//   [15:13] 111
	cmr_u32 iir_bypass;		//   [16] 0:work, 1:bypass
	//sp_afm_cfg_info_regs	cfg;
	cmr_s16 				IIR_c[11];		 
};

struct alc_af_face_info {
	cmr_u32   sx;
	cmr_u32   sy;
	cmr_u32   ex;
	cmr_u32   ey;
	cmr_u32   brightness;
	cmr_s32    pose;
};

struct alc_af_face_area {
	cmr_u16 type;//focus or ae,
	cmr_u16 face_num;
	cmr_u16 frame_width;
	cmr_u16 frame_height;
	struct alc_af_face_info face_info[10];
};



#endif
