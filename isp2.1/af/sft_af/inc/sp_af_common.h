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
#ifndef _SP_AF_COMMON_H_
#define _SP_AF_COMMON_H_
#include <sys/types.h>

typedef void* sft_af_handle_t;
#define SFT_AF_MAGIC_START		0xe5a55e5a
#define SFT_AF_MAGIC_END		0x5e5ae5a5

enum sft_af_cmd {
	SFT_AF_CMD_SET_BASE 			= 0x1000,
	SFT_AF_CMD_SET_AF_MODE			= 0x1001,
	SFT_AF_CMD_SET_AF_POS			= 0x1002,
	SFT_AF_CMD_SET_SCENE_MODE		= 0x1003,
	SFT_AF_CMD_SET_AF_START			= 0x1004,
	SFT_AF_CMD_SET_AF_STOP			= 0x1005,
	SFT_AF_CMD_SET_AF_BYPASS		= 0x1006,
	SFT_AF_CMD_SET_AF_INFO			= 0x1007,
	SFT_AF_CMD_SET_ISP_START_INFO		= 0x1008,
	SFT_AF_CMD_SET_ISP_STOP_INFO		= 0x1009,
	SFT_AF_CMD_SET_AE_INFO			= 0x100A,
	SFT_AF_CMD_SET_AWB_INFO			= 0x100B,
	SFT_AF_CMD_SET_FLASH_NOTICE		= 0x100C,
	SFT_AF_CMD_SET_FD_UPDATE		= 0x100D,
	SFT_AF_CMD_SET_AF_PARAM			= 0x100E,

	SFT_AF_CMD_GET_BASE			= 0x2000,
	SFT_AF_CMD_GET_AF_MODE			= 0X2001,
	SFT_AF_CMD_GET_AF_CUR_POS		= 0x2002,
	SFT_AF_CMD_GET_AF_INFO			= 0x2003,
	SFT_AF_CMD_GET_AF_VALUE			= 0x2004,

	SFT_AF_CMD_BURST_NOTICE = 0x2005,
};

enum sft_af_mode{
	SFT_AF_MODE_NORMAL=0x00,
	SFT_AF_MODE_MACRO,
	SFT_AF_MODE_MANUAL,
	SFT_AF_MODE_CAF,
	SFT_AF_MODE_VIDEO_CAF,
	SFT_AF_MODE_MAX
};


enum sft_af_rtn {
	SFT_AF_SUCCESS = 0,
	SFT_AF_ERROR = 255
};

enum sft_af_env_mode {
	SFT_AF_OUTDOOR = 0,
	SFT_AF_INDOOR = 1,
	SFT_AF_DARK = 2,
};

struct win_coord {
	cmr_u32 start_x;
	cmr_u32 start_y;
	cmr_u32 end_x;
	cmr_u32 end_y;
};

struct sft_af_ctrl_ops{
	cmr_s32 (*cb_set_afm_bypass)(sft_af_handle_t handle, cmr_u32 bypass);
	cmr_s32 (*cb_set_afm_mode)(sft_af_handle_t handle, cmr_u32 mode);
	cmr_s32 (*cb_set_afm_skip_num)(sft_af_handle_t handle, cmr_u32 skip_num);
	cmr_s32 (*cb_set_afm_skip_num_clr)(sft_af_handle_t handle);
	cmr_s32 (*cb_set_afm_spsmd_rtgbot_enable)(sft_af_handle_t handle, cmr_u32 enable);
	cmr_s32 (*cb_set_afm_spsmd_diagonal_enable)(sft_af_handle_t handle, cmr_u32 enable);
	cmr_s32 (*cb_set_afm_spsmd_cal_mode)(sft_af_handle_t handle, cmr_u32 mode);
	cmr_s32 (*cb_set_afm_sel_filter1)(sft_af_handle_t handle, cmr_u32 sel_filter);
	cmr_s32 (*cb_set_afm_sel_filter2)(sft_af_handle_t handle, cmr_u32 sel_filter);
	cmr_s32 (*cb_set_afm_sobel_type)(sft_af_handle_t handle, cmr_u32 type);
	cmr_s32 (*cb_set_afm_spsmd_type)(sft_af_handle_t handle, cmr_u32 type);
	cmr_s32 (*cb_set_afm_sobel_threshold)(sft_af_handle_t handle, cmr_u32 min, cmr_u32 max);
	cmr_s32 (*cb_set_afm_spsmd_threshold)(sft_af_handle_t handle, cmr_u32 min, cmr_u32 max);
	cmr_s32 (*cb_set_afm_slice_size)(sft_af_handle_t handle, cmr_u32 width, cmr_u32 height);
	cmr_s32 (*cb_set_afm_win)(sft_af_handle_t handle, struct win_coord *win_range);
	cmr_s32 (*cb_get_afm_type1_statistic)(sft_af_handle_t handle, cmr_u32 *statis);
	cmr_s32 (*cb_get_afm_type2_statistic)(sft_af_handle_t handle, cmr_u32 *statis);
	cmr_s32 (*cb_set_active_win)(sft_af_handle_t handle, cmr_u32 active_win);
	cmr_s32 (*cb_get_cur_env_mode)(sft_af_handle_t handle, cmr_u8 *mode);
	cmr_s32 (*cb_set_motor_pos)(sft_af_handle_t handle, cmr_u32 pos);
	cmr_s32 (*cb_lock_ae)(sft_af_handle_t handle);
	cmr_s32 (*cb_lock_awb)(sft_af_handle_t handle);
	cmr_s32 (*cb_unlock_ae)(sft_af_handle_t handle);
	cmr_s32 (*cb_unlock_awb)(sft_af_handle_t handle);
	cmr_u32 (*cb_get_ae_lum)(sft_af_handle_t handle);
	cmr_u32 (*cb_get_ae_status)(sft_af_handle_t handle);
	cmr_u32 (*cb_get_awb_status)(sft_af_handle_t handle);
	cmr_s32 (*cb_get_isp_size)(sft_af_handle_t handle, cmr_u16 *widith, cmr_u16 *height);
	cmr_s32 (*cb_af_finish_notice)(sft_af_handle_t handle, cmr_u32 result);
	void (*cb_sft_af_log)(const char* format, ...);
	cmr_s32 (*cb_set_sp_afm_cfg)(sft_af_handle_t handle);
	cmr_s32 (*cb_set_sp_afm_win)(sft_af_handle_t handle, struct win_coord *win_range);
	cmr_s32 (*cb_get_sp_afm_statistic)(sft_af_handle_t handle, cmr_u32 *statis);
	cmr_s32 (*cb_sp_write_i2c)(sft_af_handle_t handle,cmr_u16 slave_addr, cmr_u8 *cmd, cmr_u16 cmd_length);
	cmr_s32 (*cd_sp_read_i2c)(sft_af_handle_t handle,cmr_u16 slave_addr, cmr_u8 *cmd, cmr_u16 cmd_length);
	cmr_s32 (*cd_sp_get_cur_prv_mode)(sft_af_handle_t handle,cmr_u32 *mode);
	cmr_s32 (*cb_af_move_start_notice)(sft_af_handle_t handle);
	cmr_s32 (*cb_af_pos_update)(sft_af_handle_t handle, cmr_u32 pos);
};

//typedef union{
//	cmr_u32 val;
//	struct{
//		cmr_u32
//			BYPASS_FLAG 	:1,	// [0] 0:work, 1:bypass
//			AFM_MODE 		:1,	// [6] 1
//			SOURCE_POS  	:1,	// 0:RGB, 1:YCbCr
//			SHIFT 			:5,	// [5:1] ???????
//			SKIP_NUM 		:4, // [10:7] 0
//			SKIP_NUM_CLEAR 	:1, // [11] 0
//			FORMAT 			:3,	// [15:13] 111
//			IIR_BYPASS 		:1,	// [16] 0:work, 1:bypass
//			Reserve1		:1,
//			Reserve2		:1,
//			Reserve3		:1,
//			Reserve4		:1,
//			Reserve5		:1,
//			Reserve6		:1,
//			Reserve7		:1,
//			Reserve8		:1,
//			Reserve9		:1,
//			Reserve10		:1,
//			Reserve11		:1,
//			Reserve12		:1,
//			Reserve13		:1,
//			Reserve14		:1,
//			Reserve15		:1;
//	}dot;
//}sp_afm_cfg_info_regs;

struct sp_afm_cfg_info{
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

struct sft_af_face_info {
	cmr_u32   sx;
	cmr_u32   sy;
	cmr_u32   ex;
	cmr_u32   ey;
	cmr_u32   brightness;
	cmr_s32    pose;
};

struct sft_af_face_area {
	cmr_u16 type;//focus or ae,
	cmr_u16 face_num;
	cmr_u16 frame_width;
	cmr_u16 frame_height;
	struct sft_af_face_info face_info[10];
};



#endif
