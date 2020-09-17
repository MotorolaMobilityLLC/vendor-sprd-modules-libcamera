/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _ISP_COM_H_
#define _ISP_COM_H_

#ifndef WIN32
#include <sys/types.h>
#include <utils/Log.h>
#endif

#include "sprd_img.h"
#include "isp_type.h"
#include "isp_app.h"
#include "sensor_raw.h"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>


#ifdef	 __cplusplus
extern "C" {
#endif

#ifdef CONFIG_USE_CAMERASERVER_PROC
#define CAMERA_DUMP_PATH  "/data/vendor/cameraserver/"
#else
#define CAMERA_DUMP_PATH  "/data/misc/media/"
#endif
#define ISP_SLICE_WIN_NUM 0x0b
#define ISP_SLICE_WIN_NUM_V1 0x18
#define ISP_CMC_NUM 0x09
#define ISP_COLOR_TEMPRATURE_NUM 0x09
#define ISP_RAWAEM_BUF_SIZE   (4*3*1024)
#define ISP_BQ_AEM_CNT                      3
#define ISP_INPUT_SIZE_NUM_MAX 0x09
#define ISP_GAMMA_SAMPLE_NUM 26
#define ISP_CCE_COEF_COLOR_CAST 0
#define ISP_CCE_COEF_GAIN_OFFSET 1

#define AI_FD_NUM (20)
//#define AI_AE_STAT_SIZE (16384) /*128*128*/
#define AI_AE_STAT_SIZE (1024) /*32*32*/

typedef cmr_int(*isp_ai_cb) (cmr_handle handle, cmr_int type, void *param0, void *param1);

#define CLIP(in, bottom, top) {if(in<bottom) in=bottom; if(in>top) in=top;}
	typedef cmr_int(*io_fun) (cmr_handle isp_alg_handle, void *param_ptr, cmr_s32(*call_back) ());
	enum sensor_role_type {
		CAM_SENSOR_MASTER = 0,
		CAM_SENSOR_SLAVE0,
		CAM_SENSOR_SLAVE1,
		CAM_SENSOR_MAX,
	};
	enum isp_multi_mode {
		ISP_ALG_SINGLE = 0,
		ISP_ALG_DUAL_C_C,
		ISP_ALG_DUAL_SBS,
		ISP_ALG_BLUR_REAR,
		ISP_ALG_DUAL_W_T,
		ISP_ALG_DUAL_C_M,
		ISP_ALG_TRIBLE_W_T_UW,
		ISP_ALG_CAMERA_MAX
	};

	struct isp_lsc_statistic_info{
		cmr_u32 r_info[16384];
		cmr_u32 g_info[16384];
		cmr_u32 b_info[16384];
		cmr_u32 sec;
		cmr_u32 usec;
	};

	struct isp_awb_statistic_info {
		cmr_u32 r_info[16384];
		cmr_u32 g_info[16384];
		cmr_u32 b_info[16384];
		cmr_u32 sec;
		cmr_u32 usec;
	};

	struct isp_hist_statistic_info {
		cmr_u32 value[256];
		cmr_s32 frame_id;
		cmr_u32 sec;
		cmr_u32 usec;
		cmr_u32 bin;
	};

	struct isp_system {
		isp_handle caller_id;
		proc_callback callback;
		cmr_u32 isp_callback_bypass;
		pthread_t monitor_thread;
		isp_handle monitor_queue;
		cmr_u32 monitor_status;

		isp_handle thread_ctrl;
		isp_handle thread_afl_proc;
		isp_handle thread_af_proc;
		isp_handle thread_awb_proc;
		struct isp_ops ops;
	};

	struct isp_otp_info {
		struct isp_data_info lsc;
		struct isp_data_info awb;

		void *lsc_random;
		void *lsc_golden;
		cmr_u32 width;
		cmr_u32 height;
	};

	struct isp_ae_info {
		cmr_s32 bv;
		float gain;
		float exposure;
		float f_value;
		cmr_u32 stable;
	};

	typedef struct {
		/* isp_ctrl private */
#ifndef WIN32
		struct isp_system system;
#endif
		cmr_u32 camera_id;
		uint isp_mode;

		//new param
		void *dev_access_handle;
		void *alg_fw_handle;
		//void *isp_otp_handle;

		/* isp_driver */
		void *handle_device;

		/* 4A algorithm */
		void *handle_ae;
		void *handle_af;
		void *handle_awb;
		void *handle_smart;
		void *handle_lsc_adv;

		/* isp param manager */
		void *handle_pm;

		/* sensor param */
		cmr_u32 param_index;
		struct sensor_raw_resolution_info
		 input_size_trim[ISP_INPUT_SIZE_NUM_MAX];
		struct sensor_raw_ioctrl *ioctrl_ptr;

		cmr_u32 alc_awb;
		cmr_s32 awb_pg_flag;
		cmr_u8 *log_alc_awb;
		cmr_u32 log_alc_awb_size;
		cmr_u8 *log_alc_lsc;
		cmr_u32 log_alc_lsc_size;
		cmr_u8 *log_alc;
		cmr_u32 log_alc_size;
		cmr_u8 *log_alc_ae;
		cmr_u32 log_alc_ae_size;

		struct awb_lib_fun *awb_lib_fun;
		struct ae_lib_fun *ae_lib_fun;
		struct af_lib_fun *af_lib_fun;

		struct isp_ops ops;

		struct sensor_raw_info *sn_raw_info;

		/*for new param struct */
		struct isp_data_info isp_init_data[MAX_MODE_NUM];
		struct isp_data_info isp_update_data[MAX_MODE_NUM];	/*for isp_tool */

		cmr_u32 gamma_sof_cnt;
		cmr_u32 gamma_sof_cnt_eb;
		cmr_u32 update_gamma_eb;
		cmr_u32 mode_flag;
		cmr_u32 scene_flag;
		cmr_u32 multi_nr_flag;
		cmr_s8 *sensor_name;
	} isp_ctrl_context;

	typedef void* afl_handle_tt;

	struct afl_version_t
	{
		uint8_t major;
		uint8_t minor;
		uint8_t micro;
		uint8_t nano;
		char built_date[0x20];
		char built_time[0x20];
		char built_rev[0x100];
	};

	struct afl_ctrl_proc_out {
		cmr_int flag;
		cmr_int cur_flicker;
		cmr_u32 max_fps;
	};

	struct isp_anti_flicker_cfg {
		cmr_u32 bypass;
		afl_handle_tt afl_handle;
		struct afl_version_t afl_version;
		pthread_mutex_t status_lock;
		cmr_u32 mode;
		cmr_u32 skip_frame_num;
		cmr_u32 line_step;
		cmr_u32 frame_num;
		cmr_u32 vheight;
		cmr_u32 start_col;
		cmr_u32 end_col;
		void *addr;
		cmr_handle thr_handle;
		cmr_handle caller_handle;
		cmr_uint vir_addr;
		cmr_uint height;
		cmr_uint width;
		cmr_uint skip_num_clr;
		cmr_uint afl_glb_total_num;
		cmr_uint afl_region_total_num;
		struct afl_ctrl_proc_out proc_out;
		isp_afl_cb afl_set_cb;
		cmr_int flag;
		cmr_int cur_flicker;
		cmr_s8 version;
		cmr_u32 cam_4in1_mode;
		cmr_u32 max_fps;
		cmr_u32 camera_id;
	};

	struct isp_antiflicker_param {
		cmr_u32 normal_50hz_thrd;
		cmr_u32 lowlight_50hz_thrd;
		cmr_u32 normal_60hz_thrd;
		cmr_u32 lowlight_60hz_thrd;
		cmr_u16 thr[8];
	};

	struct isp_ae_adapt_info {
		cmr_u16 binning_factor; // 1x = 128
	};

	struct isp_rgb_gain_info {
		cmr_u32 bypass;
		cmr_u32 global_gain;
		cmr_u32 r_gain;
		cmr_u32 g_gain;
		cmr_u32 b_gain;
	};

	struct isp_rgb_aem_info {
		struct isp_size blk_num;
	};

	enum ai_callback_type {
		AI_CALLBACK_SCENE_INFO,
		AI_CALLBACK_SET_CB,
		AI_CALLBACK_MAX
	};

	enum ai_scene_type {
		AI_SCENE_DEFAULT,
		AI_SCENE_FOOD,
		AI_SCENE_PORTRAIT,
		AI_SCENE_FOLIAGE,
		AI_SCENE_SKY,
		AI_SCENE_NIGHT,
		AI_SCENE_BACKLIGHT,
		AI_SCENE_TEXT,
		AI_SCENE_SUNRISE,
		AI_SCENE_BUILDING,
		AI_SCENE_LANDSCAPE,
		AI_SCENE_SNOW,
		AI_SCENE_FIREWORK,
		AI_SCENE_BEACH,
		AI_SCENE_PET,
		AI_SCENE_FLOWER,
		AI_SCENE_MAX
	};

	enum ai_rotation {
		AI_SD_ORNT_0,
		AI_SD_ORNT_90,
		AI_SD_ORNT_180,
		AI_SD_ORNT_270,
		AI_SD_ORNT_MAX
	};

	struct ai_rect {
		cmr_u16 start_x;
		cmr_u16 start_y;
		cmr_u16 width;
		cmr_u16 height;
	};

	struct ai_face_info {
		struct ai_rect rect; /* Face rectangle */
		cmr_s16 yaw_angle; /* Out-of-plane rotation angle (Yaw);In [-90, +90] degrees; */
		cmr_s16 roll_angle; /* In-plane rotation angle (Roll); In (-180, +180] degrees; */
		cmr_u16 score; /* Confidence score; In [0, 1000] */
		cmr_u16 id; /* Human ID Number */
	};

	struct ai_fd_param {
		cmr_u16 width;
		cmr_u16 height;
		cmr_u32 frame_id;
		cmr_u64 timestamp;
		struct ai_face_info face_area[AI_FD_NUM];
		cmr_u16 face_num;
	};

	struct ai_ae_statistic_info {
		cmr_u32 *r_info;
		cmr_u32 *g_info;
		cmr_u32 *b_info;
	};

	struct ai_ae_param {
		cmr_u32 frame_id;
		cmr_u64 timestamp;
		cmr_u32 sec;
		cmr_u32 usec;
		struct ai_ae_statistic_info ae_stat;
		struct ai_rect ae_rect;
		struct img_offset ae_offset;
		cmr_u16 blk_width;
		cmr_u16 blk_height;
		cmr_u16 blk_num_hor;
		cmr_u16 blk_num_ver;
		cmr_u32 zoom_ratio;
		cmr_s32 curr_bv;
		cmr_u16 stable;
		cmr_u16 app_mode;
	};

	struct ai_img_buf {
		cmr_u32 img_y;
		cmr_u32 img_uv;
	};

	struct ai_img_param {
		struct ai_img_buf img_buf;
		cmr_u32 frame_id;
		cmr_u64 timestamp;
		cmr_u32 width;
		cmr_u32 height;
		cmr_u32 img_y_pitch;
		cmr_u32 img_uv_pitch;
		cmr_u32 is_continuous;
		enum ai_rotation orientation;
	};

	enum ai_status {
		AI_STATUS_IDLE,
		AI_STATUS_PROCESSING,
		AI_STATUS_MAX
	};

	enum ai_task_0 {
		AI_SCENE_TASK0_INDOOR,
		AI_SCENE_TASK0_OUTDOOR,
		AI_SCENE_TASK0_MAX
	};

	enum ai_task_1 {
		AI_SCENE_TASK1_NIGHT,
		AI_SCENE_TASK1_BACKLIGHT,
		AI_SCENE_TASK1_SUNRISESET,
		AI_SCENE_TASK1_FIREWORK,
		AI_SCENE_TASK1_OTHERS,
		AI_SCENE_TASK1_MAX
	};

	enum ai_task_2 {
		AI_SCENE_TASK2_FOOD,
		AI_SCENE_TASK2_GREENPLANT,
		AI_SCENE_TASK2_DOCUMENT,
		AI_SCENE_TASK2_CATDOG,
		AI_SCENE_TASK2_FLOWER,
		AI_SCENE_TASK2_BLUESKY,
		AI_SCENE_TASK2_BUILDING,
		AI_SCENE_TASK2_SNOW,
		AI_SCENE_TASK2_OTHERS,
		AI_SCENE_TASK2_MAX
	};

	struct ai_task0_result {
		enum ai_task_0 id;
		cmr_u16 score;
	};

	struct ai_task1_result {
		enum ai_task_1 id;
		cmr_u16 score;
	};

	struct ai_task2_result {
		enum ai_task_2 id;
		cmr_u16 score;
	};

	struct ai_scene_detect_info {
		cmr_u32 frame_id;
		enum ai_scene_type cur_scene_id;
		struct ai_task0_result task0[AI_SCENE_TASK0_MAX];
		struct ai_task1_result task1[AI_SCENE_TASK1_MAX];
		struct ai_task2_result task2[AI_SCENE_TASK2_MAX];
	};

	enum ai_img_flag {
		IMAGE_DATA_NOT_REQUIRED,
		IMAGE_DATA_REQUIRED,
		IMAGE_DATA_MAX
	};

	enum ai_work_mode {
		AI_WORKMODE_FULL = 1,
		AI_WORKMODE_PORTRAIT = 2,
		AI_WORKMODE_MAX
	};

	enum ai_flash_status {
		AI_FLASH_PRE_BEFORE,
		AI_FLASH_PRE_LIGHTING,
		AI_FLASH_PRE_AFTER,
		AI_FLASH_MAIN_BEFORE,
		AI_FLASH_MAIN_LIGHTING,
		AI_FLASH_MAIN_AE_MEASURE,
		AI_FLASH_MAIN_AFTER,
		AI_FLASH_AF_DONE,
		AI_FLASH_SLAVE_FLASH_OFF,
		AI_FLASH_SLAVE_FLASH_TORCH,
		AI_FLASH_SLAVE_FLASH_AUTO,
		AI_FLASH_MODE_MAX
	};

	struct ai_img_status {
		cmr_u32 frame_id;
		cmr_s32 frame_state;
		enum ai_img_flag img_flag;
	};

#ifdef	 __cplusplus
}
#endif
#endif
