/*
 * Copyright (C) 2018 The Android Open Source Project
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
#ifndef _ISP_MW_H_
#define _ISP_MW_H_

#include "isp_exif.h"
#include "isp_type.h"
#include "sprd_isp_k.h"
#include "cmr_sensor_info.h"

typedef cmr_int(*proc_callback) (cmr_handle handler_id, cmr_u32 mode, void *param_ptr, cmr_u32 param_len);

#define ISP_EVT_MASK	 0x0000FF00

#define ISP_FLASH_MAX_CELL	40
#define ISP_MODE_NUM_MAX 16


#define ISP_THREAD_QUEUE_NUM			(100)

#define ISP_CALLBACK_EVT                     0x00040000

#define ISP_AI_FD_NUM (20)
//#define ISP_AI_AE_STAT_SIZE (16384) /*128*128*/
#define ISP_AI_AE_STAT_SIZE (1024) /*32*32*/

enum isp_alg_set_cmd {
	ISP_AE_SET_GAIN,
	ISP_AE_SET_MONITOR,
	ISP_AE_SET_MONITOR_WIN,
	ISP_AE_SET_MONITOR_BYPASS,
	ISP_AE_SET_STATISTICS_MODE,
	ISP_AE_SET_STATS_MONITOR,
	ISP_AE_SET_RGB_GAIN,
	ISP_AE_SET_RGB_GAIN_FOR_4IN1,
	ISP_AE_SET_AE_CALLBACK,
	ISP_AE_SET_EXPOSURE,
	ISP_AE_EX_SET_EXPOSURE,
	ISP_AE_SET_FLASH_CHARGE,
	ISP_AE_SET_FLASH_TIME,
	ISP_AE_GET_SYSTEM_TIME,
	ISP_AE_GET_FLASH_CHARGE,
	ISP_AE_GET_FLASH_TIME,
	ISP_AE_FLASH_CTRL,
	ISP_AE_GET_RGB_GAIN,
	ISP_AE_SET_WBC_GAIN,
	ISP_AE_MULTI_WRITE,
	ISP_AE_SET_BLK_NUM,

	ISP_AF_SET_POS,
	ISP_AF_END_NOTICE,
	ISP_AF_START_NOTICE,
	ISP_AF_AE_AWB_LOCK,
	ISP_AF_AE_AWB_RELEASE,
	ISP_AF_AE_LOCK,
	ISP_AF_AE_UNLOCK,
	ISP_AF_AE_CAF_LOCK,
	ISP_AF_AE_CAF_UNLOCK,
	ISP_AF_AWB_LOCK,
	ISP_AF_AWB_UNLOCK,
	ISP_AF_LSC_LOCK,
	ISP_AF_LSC_UNLOCK,
	ISP_AF_NLM_LOCK,
	ISP_AF_NLM_UNLOCK,
	ISP_AF_SET_MONITOR,
	ISP_AF_SET_MONITOR_WIN,
	ISP_AF_GET_MONITOR_WIN_NUM,
	ISP_AF_SET_PD_INFO,
	ISP_AF_LENS_SET_POS,
	ISP_AF_LENS_GET_OTP,
	ISP_AF_GET_MOTOR_POS,
	ISP_AF_SET_MOTOR_BESTMODE,
	ISP_AF_GET_VCM_TEST_MODE,
	ISP_AF_SET_VCM_TEST_MODE,
	ISP_AF_GET_SYSTEM_TIME,
	ISP_PDAF_SET_CFG_PARAM,
	ISP_PDAF_BLOCK_CFG,
	ISP_PDAF_SET_BYPASS,
	ISP_PDAF_SET_WORK_MODE,
	ISP_PDAF_SET_SKIP_NUM,
	ISP_PDAF_SET_PPI_INFO,
	ISP_PDAF_SET_ROI,
	ISP_PDAF_SET_EXTRACTOR_BYPASS,
	ISP_AFL_SET_CFG_PARAM,
	ISP_AFL_SET_BYPASS,
	ISP_AFL_NEW_SET_CFG_PARAM,
	ISP_AFL_NEW_SET_BYPASS,
	ISP_AFL_SET_STATS_BUFFER,
	ISP_AFM_BYPASS,
	ISP_AFM_SKIP_NUM,
	ISP_AFM_MODE,
	ISP_AFM_IIR_NR_CFG,
	ISP_AFM_MODULES_CFG,
	ISP_AFM_TYPE2_STS,
	ISP_AFM_TYPE1_STS,
	ISP_AF_SET_TOF_INFO,
};

enum isp_callback_cmd {
	ISP_CTRL_CALLBACK = 0x00000000,
	ISP_PROC_CALLBACK = 0x00000100,
	ISP_AF_NOTICE_CALLBACK = 0x00000200,
	ISP_SKIP_FRAME_CALLBACK = 0x00000300,
	ISP_AE_STAB_CALLBACK = 0x00000400,
	ISP_AF_STAT_CALLBACK = 0x00000500,
	ISP_AF_STAT_END_CALLBACK = 0x00000600,
	ISP_AWB_STAT_CALLBACK = 0x00000700,
	ISP_CONTINUE_AF_NOTICE_CALLBACK = 0x00000800,
	ISP_AE_CHG_CALLBACK = 0x00000900,
	ISP_ONLINE_FLASH_CALLBACK = 0x00000A00,
	ISP_QUICK_MODE_DOWN = 0x00000B00,
	ISP_AE_STAB_NOTIFY = 0x00000C00,
	ISP_AE_LOCK_NOTIFY = 0x00000D00,
	ISP_AE_UNLOCK_NOTIFY = 0x00000E00,
	ISP_AE_SYNC_INFO = 0x00000F00,
	ISP_AE_EXP_TIME = 0x00001000,
	ISP_VCM_STEP = 0x00002000,
	ISP_HDR_EV_EFFECT_CALLBACK = 0x00003000,
	ISP_AE_CB_FLASH_FIRED = 0x00004000,
	ISP_AUTO_HDR_STATUS_CALLBACK = 0x00005000,
	ISP_AI_SCENE_INFO_CALLBACK = 0x00006000,
	ISP_AI_SCENE_TYPE_CALLBACK = 0x00007000,
	ISP_CALLBACK_CMD_MAX = 0xffffffff
};

enum isp_video_mode {
	ISP_VIDEO_MODE_CONTINUE = 0x00,
	ISP_VIDEO_MODE_SINGLE,
	ISP_VIDEO_MODE_MAX
};

enum isp_focus_mode {
	ISP_FOCUS_NONE = 0x00,
	ISP_FOCUS_TRIG,
	ISP_FOCUS_ZONE,
	ISP_FOCUS_MULTI_ZONE,
	ISP_FOCUS_MACRO,
	ISP_FOCUS_WIN,
	ISP_FOCUS_CONTINUE,
	ISP_FOCUS_MANUAL,
	ISP_FOCUS_VIDEO,
	ISP_FOCUS_BYPASS,
	ISP_FOCUS_MACRO_FIXED,
	ISP_FOCUS_PICTURE,
	ISP_FOCUS_FULLSCAN,
	ISP_FOCUS_MAX
};

enum isp_focus_move_mode {
	ISP_FOCUS_MOVE_START = 0x00,
	ISP_FOCUS_MOVE_END,
	ISP_FOCUS_MOVE_MAX
};

enum isp_flash_mode {
	ISP_FLASH_PRE_BEFORE,
	ISP_FLASH_PRE_LIGHTING,
	ISP_FLASH_PRE_AFTER,
	ISP_FLASH_MAIN_BEFORE,
	ISP_FLASH_MAIN_LIGHTING,
	ISP_FLASH_MAIN_AE_MEASURE,
	ISP_FLASH_MAIN_AFTER,
	ISP_FLASH_AF_DONE,
	ISP_FLASH_SLAVE_FLASH_OFF,
	ISP_FLASH_SLAVE_FLASH_TORCH,
	ISP_FLASH_SLAVE_FLASH_AUTO,
	ISP_FLASH_MODE_MAX
};

enum isp_ae_awb_lock_unlock_mode {
	ISP_AWB_UNLOCK = 0x00,
	ISP_AWB_LOCK,
	ISP_AE_AWB_LOCK = 0x09,
	ISP_AE_AWB_UNLOCK = 0x0a,
	ISP_AE_AWB_MAX
};

enum isp_ae_mode {
	ISP_AUTO = 0x00,
	ISP_NIGHT,
	ISP_SPORT,
	ISP_PORTRAIT,
	ISP_LANDSCAPE,
	ISP_AE_MODE_MAX
};

enum isp_awb_mode {
	ISP_AWB_INDEX0 = 0x00,
	ISP_AWB_INDEX1,
	ISP_AWB_INDEX2,
	ISP_AWB_INDEX3,
	ISP_AWB_INDEX4,
	ISP_AWB_INDEX5,
	ISP_AWB_INDEX6,
	ISP_AWB_INDEX7,
	ISP_AWB_INDEX8,
	ISP_AWB_AUTO,
	ISP_AWB_OFF,
	ISP_AWB_MAX
};

enum isp_format {
	ISP_DATA_YUV422_3FRAME = 0x00,
	ISP_DATA_YUV422_2FRAME,
	ISP_DATA_YVU422_2FRAME,
	ISP_DATA_YUYV,
	ISP_DATA_UYVY,
	ISP_DATA_YVYU,
	ISP_DATA_VYUY,
	ISP_DATA_YUV420_2FRAME,
	ISP_DATA_YVU420_2FRAME,
	ISP_DATA_YUV420_3_FRAME,
	ISP_DATA_NORMAL_RAW10,
	ISP_DATA_CSI2_RAW10,
	ISP_DATA_ALTEK_RAW10,
	ISP_DATA_FORMAT_MAX
};

enum isp_capture_mode {
	ISP_CAP_MODE_AUTO,
	ISP_CAP_MODE_ZSL,
	ISP_CAP_MODE_HDR,
	ISP_CAP_MODE_VIDEO,
	ISP_CAP_MODE_VIDEO_HDR,
	ISP_CAP_MODE_BRACKET,
	ISP_CAP_MODE_RAW_DATA,
	ISP_CAP_MODE_HIGHISO,
	ISP_CAP_MODE_HIGHISO_RAW_CAP,
	ISP_CAP_MODE_DRAM,
	ISP_CAP_MODE_MAX
};

enum isp_ctrl_cmd {
	ISP_CTRL_AWB_MODE = 0,
	ISP_CTRL_SCENE_MODE = 1,
	ISP_CTRL_AE_MEASURE_LUM = 2,
	ISP_CTRL_EV = 3,
	ISP_CTRL_FLICKER = 4,
	ISP_CTRL_AEAWB_BYPASS = 5,
	ISP_CTRL_SPECIAL_EFFECT = 6,
	ISP_CTRL_BRIGHTNESS = 7,
	ISP_CTRL_CONTRAST = 8,
	ISP_CTRL_HIST,
	ISP_CTRL_AUTO_CONTRAST,
	ISP_CTRL_SATURATION = 11,
	ISP_CTRL_AF,
	ISP_CTRL_AF_MODE = 13,
	ISP_CTRL_CSS,
	ISP_CTRL_HDR = 15,
	ISP_CTRL_GLOBAL_GAIN,
	ISP_CTRL_CHN_GAIN,
	ISP_CTRL_GET_EXIF_INFO,
	ISP_CTRL_ISO = 19,
	ISP_CTRL_WB_TRIM = 20,
	ISP_CTRL_PARAM_UPDATE,
	ISP_CTRL_FLASH_EG,
	ISP_CTRL_VIDEO_MODE,
	ISP_CTRL_AF_STOP = 24,
	ISP_CTRL_AE_TOUCH,
	ISP_CTRL_AE_INFO,
	ISP_CTRL_SHARPNESS,
	ISP_CTRL_GET_FAST_AE_STAB = 28,
	ISP_CTRL_GET_AE_STAB,
	ISP_CTRL_GET_AE_CHG,
	ISP_CTRL_GET_AWB_STAT,
	ISP_CTRL_GET_AF_STAT = 32,
	ISP_CTRL_GAMMA,
	ISP_CTRL_DENOISE,
	ISP_CTRL_SMART_AE,
	ISP_CTRL_CONTINUE_AF = 36,
	ISP_CTRL_AF_DENOISE,
	ISP_CTRL_FLASH_CTRL = 38,	// for isp tool
	ISP_CTRL_AE_CTRL = 39,	// for isp tool
	ISP_CTRL_AF_CTRL = 40,	// for isp tool
	ISP_CTRL_DENOISE_PARAM_READ,	//for isp tool
	ISP_CTRL_DUMP_REG,	//for isp tool
	ISP_CTRL_AF_END_INFO,	// for isp tool
	ISP_CTRL_FLASH_NOTICE = 44,
	ISP_CTRL_AE_FORCE_CTRL,	// for mp tool
	ISP_CTRL_GET_AE_STATE,	// for isp tool
	ISP_CTRL_SET_LUM,	// for isp tool
	ISP_CTRL_GET_LUM = 48,	// for isp tool
	ISP_CTRL_SET_AF_POS,	// for isp tool
	ISP_CTRL_GET_AF_POS,	// for isp tool
	ISP_CTRL_GET_AF_MODE,	// for isp tool
	ISP_CTRL_FACE_AREA = 52,
	ISP_CTRL_AF_FACE_AREA,
	ISP_CTRL_SCALER_TRIM,
	ISP_CTRL_START_3A,
	ISP_CTRL_STOP_3A = 56,
	IST_CTRL_SNAPSHOT_NOTICE,
	ISP_CTRL_SFT_READ,
	ISP_CTRL_SFT_WRITE,
	ISP_CTRL_SFT_SET_PASS = 60,
	ISP_CTRL_SFT_GET_AF_VALUE,
	ISP_CTRL_SFT_SET_BYPASS,
	ISP_CTRL_GET_AWB_GAIN,	// for mp tool
	ISP_CTRL_GET_AWB_CT = 64,
	ISP_CTRL_RANGE_FPS,
	ISP_CTRL_SET_AE_FPS,	// for LLS feature
	ISP_CTRL_BURST_NOTICE,
	ISP_CTRL_GET_INFO = 68,
	ISP_CTRL_SET_AE_NIGHT_MODE,
	ISP_CTRL_SET_AE_AWB_LOCK_UNLOCK,
	ISP_CTRL_SET_AE_LOCK_UNLOCK,
	ISP_CTRL_TOOL_SET_SCENE_PARAM = 72,
	ISP_CTRL_IFX_PARAM_UPDATE,
	ISP_CTRL_FORCE_AE_QUICK_MODE,
	ISP_CTRL_DENOISE_PARAM_UPDATE,	//for isp tool
	ISP_CTRL_SET_AE_EXP_TIME = 76,
	ISP_CTRL_SET_AE_SENSITIVITY,
	ISP_CTRL_SET_DZOOM_FACTOR,
	ISP_CTRL_SET_CONVERGENCE_REQ,
	ISP_CTRL_SET_SNAPSHOT_FINISHED = 80,
	ISP_CTRL_GET_EXIF_DEBUG_INFO,
	ISP_CTRL_GET_CUR_ADGAIN_EXP,
	ISP_CTRL_SET_FLASH_MODE,
	ISP_CTRL_SET_AE_MODE = 84,
	ISP_CTRL_SET_AE_FIX_EXP_TIME,
	ISP_CTRL_SET_AE_FIX_SENSITIVITY,
	ISP_CTRL_SET_AE_FIX_FRAM_DURA,
	ISP_CTRL_SET_AE_MANUAL_EXPTIME = 88,
	ISP_CTRL_SET_AE_MANUAL_GAIN,
	ISP_CTRL_SET_AE_MANUAL_ISO,
	ISP_CTRL_SET_AE_ENGINEER_MODE,
	ISP_CTRL_GET_YIMG_INFO = 92,
	ISP_CTRL_SET_PREV_YIMG,
	ISP_CTRL_SET_PREV_YUV,
	ISP_CTRL_SET_PREV_PDAF_RAW,
	ISP_CTRL_GET_VCM_INFO = 96,
	ISP_CTRL_GET_FPS,
	ISP_CTRL_GET_LEDS_CTRL,
	ISP_CTRL_GET_GLB_GAIN,
	ISP_CTRL_AE_EXP_COMPENSATION = 100,
	/* warning if you wanna send async msg
	 * please add msg id below here
	 */
	ISP_CTRL_SYNC_NONE_MSG_BEGIN,
	ISP_CTRL_SYNC_NONE_MSG_END,
	/* warning if you wanna set ioctrl directly
	 * please add msg id below here
	 */
	ISP_CTRL_DIRECT_MSG_BEGIN,
	ISP_CTRL_SET_AUX_SENSOR_INFO = 104,
	ISP_CTRL_DIRECT_MSG_END,
	ISP_CTRL_SET_DCAM_TIMESTAMP,
	ISP_CTRL_GET_FULLSCAN_INFO,
	ISP_CTRL_SET_AF_BYPASS = 108,
	ISP_CTRL_POST_3DNR,
	ISP_CTRL_POST_YNR,
	ISP_CTRL_3DNR,
	ISP_CTRL_GET_MICRODEPTH_PARAM = 112,
	ISP_CTRL_SET_MICRODEPTH_DEBUG_INFO,
	ISP_CTRL_SENSITIVITY,
	ISP_CTRL_GET_CNR2_EN,
	ISP_CTRL_GET_CNR2_PARAM = 116,
	ISP_CTRL_AUTO_HDR_MODE,
	ISP_CTRL_SET_CAP_FLAG,
	ISP_CTRL_AI_PROCESS_START,
	ISP_CTRL_AI_PROCESS_STOP = 120,
	ISP_CTRL_AI_SET_IMG_PARAM,
	ISP_CTRL_AI_GET_IMG_FLAG,
	ISP_CTRL_AI_GET_STATUS,
	/*camera mode which appearby right slip*/
	ISP_CTRL_SET_APP_MODE,
	ISP_CTRL_MAX
};

enum isp_capbility_cmd {
	ISP_VIDEO_SIZE,
	ISP_LOW_LUX_EB,
	ISP_CUR_ISO,
	ISP_DENOISE_INFO,
	ISP_CTRL_GET_AE_LUM,	//for LLS feature
	ISP_CAPBILITY_MAX
};

enum isp_ae_lock_unlock_mode {
	ISP_AE_UNLOCK,
	ISP_AE_LOCK,
	ISP_AE_LOCK_UNLOCK_MAX
};

enum isp_flash_led_tag {
	ISP_FLASH_LED_0 = 0x0001,
	ISP_FLASH_LED_1 = 0x0002
};


enum {
	ISP_SINGLE = 0,
	ISP_DUAL_NORMAL,
	ISP_DUAL_SBS,
	ISP_BLUR_REAR,
	ISP_BOKEH,
	ISP_WIDETELE,
	ISP_CAMERA_MAX
};

enum isp_4in1_mode {
	ISP_4IN1_OFF,
	ISP_4IN1_BINNING,
	ISP_4IN1_FULL,
	ISP_4IN1_MAX,
};

enum isp_ai_scene_type {
	ISP_AI_SCENE_DEFAULT,
	ISP_AI_SCENE_FOOD,
	ISP_AI_SCENE_PORTRAIT,
	ISP_AI_SCENE_FOLIAGE,
	ISP_AI_SCENE_SKY,
	ISP_AI_SCENE_NIGHT,
	ISP_AI_SCENE_BACKLIGHT,
	ISP_AI_SCENE_TEXT,
	ISP_AI_SCENE_SUNRISE,
	ISP_AI_SCENE_BUILDING,
	ISP_AI_SCENE_LANDSCAPE,
	ISP_AI_SCENE_SNOW,
	ISP_AI_SCENE_FIREWORK,
	ISP_AI_SCENE_BEACH,
	ISP_AI_SCENE_PET,
	ISP_AI_SCENE_FLOWER,
	ISP_AI_SCENE_MAX
};

struct isp_flash_cfg {
	cmr_u32 type;
	cmr_u32 led_idx;
	cmr_u32 led0_enable;
	cmr_u32 led1_enable;
};

struct isp_3dnr_ctrl_param {
	cmr_u32 enable;
	cmr_u32 count;
};

struct isp_adgain_exp_info {
	cmr_u32 adgain;
	cmr_u32 exp_time;
	cmr_u32 bv;
	cmr_u32 lowlight_flag;
};

struct isp_yimg_info {
	cmr_uint yaddr[2];
	cmr_u32 lock[2];
};

struct yimg_info {
	cmr_uint y_addr[2];
	cmr_uint y_size;
	cmr_s32 ready[2];
	cmr_s32 sec;
	cmr_s32 usec;
	cmr_uint camera_id;
};

struct yuv_info_t {
	cmr_uint camera_id;
	cmr_u8 *yuv_addr;
	cmr_u32 width;
	cmr_u32 height;
};

struct pd_frame_in {
	cmr_handle caller_handle;
	cmr_u32 camera_id;
	void *private_data;
};

struct isp_af_notice {
	cmr_u32 mode;
	cmr_u32 valid_win;
	cmr_u32 focus_type;
	cmr_u32 reserved[10];
};

enum isp_flash_type {
	ISP_FLASH_TYPE_PREFLASH,
	ISP_FLASH_TYPE_MAIN,
	ISP_FLASH_TYPE_MAX
};

struct isp_flash_power {
	cmr_s32 max_charge;	//mA
	cmr_s32 max_time;	//ms
};

struct isp_flash_led_info {
	struct isp_flash_power power_0;
	struct isp_flash_power power_1;
	cmr_u32 led_tag;
};

struct isp_flash_notice {
	enum isp_flash_mode mode;
	cmr_u32 flash_ratio;
	cmr_u32 will_capture;
	struct isp_flash_power power;
	cmr_s32 capture_skip_num;
	struct isp_flash_led_info led_info;
};

struct isp_af_win {
	enum isp_focus_mode mode;
	struct isp_pos_rect win[9];
	cmr_u32 valid_win;
	cmr_u32 ae_touch;
	struct isp_pos_rect ae_touch_rect;
};

struct isp_af_fullscan_info {
	cmr_u8 row_num;
	cmr_u8 column_num;
	cmr_u32 *win_peak_pos;
	cmr_u16 vcm_dac_up_bound;
	cmr_u16 vcm_dac_low_bound;
	cmr_u16 boundary_ratio;
	cmr_u32 af_peak_pos;
	cmr_u32 near_peak_pos;
	cmr_u32 far_peak_pos;
	cmr_u32 distance_reminder;
	cmr_u32 reserved[16];
};

enum af_aux_sensor_type {
	AF_ACCELEROMETER,
	AF_MAGNETIC_FIELD,
	AF_GYROSCOPE,
	AF_LIGHT,
	AF_PROXIMITY,
};

struct af_gyro_info_t {
	cmr_s64 timestamp;
	float x;
	float y;
	float z;
};

struct af_gsensor_info {
	cmr_s64 timestamp;
	float vertical_up;
	float vertical_down;
	float horizontal;
	cmr_u32 valid;
};

struct af_aux_sensor_info_t {
	enum af_aux_sensor_type type;
	union {
		struct af_gyro_info_t gyro_info;
		struct af_gsensor_info gsensor_info;
	};
};

struct isp_af_ts {
	cmr_u64 timestamp;
	cmr_u32 capture;
};

struct isp_face_info {
	cmr_u32 sx;
	cmr_u32 sy;
	cmr_u32 ex;
	cmr_u32 ey;
	cmr_u32 brightness;
	cmr_s32 pose;
	cmr_s32 angle;
	cmr_s32 yaw_angle;
	cmr_s32 roll_angle;
	cmr_u32 score;
	cmr_u32 id;
};

struct isp_face_area {
	cmr_u16 type;
	cmr_u16 face_num;
	cmr_u16 frame_width;
	cmr_u16 frame_height;
	struct isp_face_info face_info[10];
	cmr_u32 frame_id;
	cmr_s64 timestamp;
};

struct isp_img_mfd {
	cmr_u32 y;
	cmr_u32 u;
	cmr_u32 v;
};

struct isp_img_frm {
	enum isp_format img_fmt;
	struct isp_size img_size;
	struct isp_img_mfd img_fd;
	struct isp_addr img_addr_phy;
	struct isp_addr img_addr_vir;
	cmr_u32 format_pattern;
};


struct isp_flash_element {
	cmr_u16 index;
	cmr_u16 val;
};

struct isp_flash_cell {
	cmr_u8 type;
	cmr_u8 count;
	cmr_u8 def_val;
	struct isp_flash_element element[ISP_FLASH_MAX_CELL];
};

struct isp_sensor_ex_info {
	cmr_u32 f_num;
	cmr_u32 focal_length;
	cmr_u32 max_fps;
	cmr_u32 max_adgain;
	cmr_u32 ois_supported;
	cmr_u32 pdaf_supported;
	cmr_u32 ebd_supported;
	cmr_u32 exp_valid_frame_num;
	cmr_u32 clamp_level;
	cmr_u32 adgain_valid_frame_num;
	cmr_u32 preview_skip_num;
	cmr_u32 capture_skip_num;
	cmr_s8 *name;
	cmr_s8 *sensor_version_info;
	struct af_pose_dis pos_dis;
	cmr_u32 af_supported;
	cmr_u32 tof_support;
	cmr_u32 color_support;
	struct drv_fov_info fov_info;
};


struct isp_video_limit {
	cmr_u32 width;
	cmr_u32 height;
	cmr_u32 res;
};

struct isp_sensor_fps_info {
	cmr_u32 mode;
	cmr_u32 max_fps;
	cmr_u32 min_fps;
	cmr_u32 is_high_fps;
	cmr_u32 high_fps_skip_num;
};

struct isp_range_fps {
	cmr_u16 min_fps;
	cmr_u16 max_fps;
};

struct isp_ae_fps {
	cmr_u32 min_fps;
	cmr_u32 max_fps;
};

struct isp_hdr_param {
	cmr_u32 hdr_enable;
	cmr_u32 ev_effect_valid_num;
};

struct isp_info {
	void *addr;
	cmr_s32 size;
};


struct isp_sensor_resolution_info {
	struct isp_size yuv_img_size;
	struct isp_size sensor_size;
	struct isp_rect crop;
	struct isp_range_fps fps;
	cmr_u32 line_time;
	cmr_u32 frame_line;
	cmr_u32 size_index;
	cmr_u32 max_gain;
	struct isp_size sensor_max_size;
	struct isp_size sensor_output_size;
};


struct ips_in_param {
	struct isp_img_frm src_frame;
	cmr_u32 src_avail_height;
	cmr_u32 src_slice_height;
	struct isp_img_frm dst_frame;
	cmr_u32 dst_slice_height;

	struct isp_img_frm dst2_frame;
	cmr_u32 dst2_slice_height;
	struct isp_sensor_resolution_info resolution_info;
	struct isp_sensor_fps_info sensor_fps;
	cmr_u32 cap_mode;
	cmr_handle oem_handle;
	cmr_malloc alloc_cb;
	cmr_free free_cb;
	cmr_u32 sensor_id;
};

struct ips_out_param {
	cmr_u32 output_height;
};

struct ipn_in_param {
	cmr_u32 src_avail_height;
	cmr_u32 src_slice_height;
	struct isp_addr img_addr_phy;
	struct isp_addr src_addr_phy;
	struct isp_addr dst_addr_phy;
};

struct isp_video_start {
	cmr_u16 is_snapshot;
	cmr_u32 dv_mode;
	cmr_u32 zsl_flag;
	void *cb_of_malloc;
	void *cb_of_free;
	void *buffer_client_data;

	struct isp_size size;
	struct isp_size dcam_size;
	struct isp_sensor_resolution_info resolution_info;
	cmr_u16 is_refocus;
	enum isp_format format;
	enum isp_video_mode mode;
	cmr_u32 work_mode;
	cmr_u32 capture_mode;	//enum isp_capture_mode
	cmr_uint lsc_buf_size;
	cmr_uint lsc_buf_num;
	cmr_uint lsc_phys_addr;
	cmr_uint lsc_virt_addr;
	cmr_s32 lsc_mfd;
	cmr_u32 is_need_flash;
	cmr_u32 capture_skip_num;
	struct isp_sensor_fps_info sensor_fps;
	struct isp_size live_view_sz;
	cmr_u8 pdaf_enable;
	cmr_handle oem_handle;
	cmr_malloc alloc_cb;
	cmr_free free_cb;
	cmr_u32 is_4in1_sensor;
	cmr_uint mode_4in1;
};

struct isp_img_param {
	cmr_u32 img_fmt;
	cmr_u32 channel_id;
	cmr_u32 base_id;
	cmr_u32 count;
	cmr_u32 length;
	cmr_u32 slice_height;
	cmr_u32 start_buf_id;
	cmr_u32 is_reserved_buf;
	cmr_u32 flag;
	cmr_u32 index;
	struct isp_size img_size;
	struct isp_img_mfd img_fd;
	struct isp_addr addr;
	struct isp_addr addr_vir;
	cmr_uint zsl_private;
};

struct isp_buffer {
	cmr_u8 *buffer;
	cmr_s32 fd;
};

struct isp_3dnr_info {
	struct isp_buffer image[3];
	cmr_u32 width;
	cmr_u32 height;
	cmr_s8 mv_x;
	cmr_s8 mv_y;
	cmr_u8 blending_no;
};

struct isp_exp_compensation{
	cmr_s32 comp_val;
	struct isp_range_l comp_range;
	cmr_s32 step_numerator;
	cmr_s32 step_denominator;
};

struct isp_ops {
	cmr_s32 (*flash_get_charge)(void *handler, struct isp_flash_cfg *cfg_ptr, struct isp_flash_cell *cell);
	cmr_s32 (*flash_get_time)(void *handler, struct isp_flash_cfg *cfg_ptr, struct isp_flash_cell *cell);
	cmr_s32 (*flash_set_charge)(void *handler, struct isp_flash_cfg *cfg_ptr, struct isp_flash_element *element);
	cmr_s32 (*flash_set_time)(void *handler, struct isp_flash_cfg *cfg_ptr, struct isp_flash_element *element);
	cmr_s32 (*flash_ctrl)(void *handler, struct isp_flash_cfg *cfg_ptr, struct isp_flash_element *element);
	cmr_s32 (*set_pulse_line)(void *handler, cmr_u32 pulse_line);
	cmr_s32 (*set_next_vcm_pos)(void *handler, cmr_s32 pos);
	cmr_s32 (*set_pulse_log)(void *handler, cmr_u32 enable);
};

struct isp_init_param {
	void *setting_param_ptr;
	struct isp_size size;
	proc_callback ctrl_callback;
	cmr_handle oem_handle;
	struct isp_data_info calibration_param;
	cmr_u32 camera_id;
	void *sensor_lsc_golden_data;
	struct isp_ops ops;
	struct isp_data_info mode_ptr[ISP_MODE_NUM_MAX];
	cmr_malloc alloc_cb;
	cmr_free free_cb;
	void *setting_param_list_ptr[3];
	struct isp_sensor_ex_info ex_info;
	struct sensor_otp_cust_info *otp_data;
	struct sensor_data_info pdaf_otp;
	struct sensor_pdaf_info *pdaf_info;
	struct isp_size sensor_max_size;

	cmr_u32 image_pattern;
	cmr_s32 dcam_fd;
	uint32_t multi_mode;
	uint32_t is_master;
	uint32_t is_4in1_sensor;
};

struct isp_sw_filter_weights
{
	cmr_u8 distWeight[9];
	cmr_u8 rangWeight[128];
};

struct isp_sw_cnr2_info {
	cmr_u8 filter_en[4];
	cmr_u8 rangTh[4][2];
	struct isp_sw_filter_weights weight[4][2];
};

struct img_offset {
	uint32_t x;
	uint32_t y;
};

struct isp_ai_rect {
	cmr_u16 start_x;
	cmr_u16 start_y;
	cmr_u16 width;
	cmr_u16 height;
};

struct isp_ai_face_info {
	struct isp_ai_rect rect; /* Face rectangle */
	cmr_s16 yaw_angle; /* Out-of-plane rotation angle (Yaw);In [-90, +90] degrees; */
	cmr_s16 roll_angle; /* In-plane rotation angle (Roll); In (-180, +180] degrees; */
	cmr_u16 score; /* Confidence score; In [0, 1000] */
	cmr_u16 id; /* Human ID Number */
};

struct isp_ai_fd_param {
	cmr_u16 width;
	cmr_u16 height;
	cmr_u32 frame_id;
	cmr_u64 timestamp;
	struct isp_ai_face_info face_area[ISP_AI_FD_NUM];
	cmr_u16 face_num;
};

struct isp_ai_ae_statistic_info {
	cmr_u32 *r_info;
	cmr_u32 *g_info;
	cmr_u32 *b_info;
};

struct isp_ai_ae_param {
	cmr_u32 frame_id;
	cmr_u64 timestamp;
	cmr_u32 sec;
	cmr_u32 usec;
	struct isp_ai_ae_statistic_info ae_stat;
	struct isp_ai_rect ae_rect;
	struct img_offset ae_offset;
	cmr_u16 blk_width;
	cmr_u16 blk_height;
	cmr_u16 blk_num_hor;
	cmr_u16 blk_num_ver;
	cmr_u32 zoom_ratio;
};

struct isp_ai_img_buf {
	cmr_u32 img_y;
	cmr_u32 img_uv;
};

struct isp_ai_img_param {
	struct isp_ai_img_buf img_buf;
	cmr_u32 frame_id;
	cmr_u64 timestamp;
};

enum isp_ai_status {
	ISP_AI_STATUS_IDLE,
	ISP_AI_STATUS_PROCESSING,
	ISP_AI_STATUS_MAX
};

enum isp_ai_task_0 {
	ISP_AI_SCENE_TASK0_INDOOR,
	ISP_AI_SCENE_TASK0_OUTDOOR,
	ISP_AI_SCENE_TASK0_MAX
};

enum isp_ai_task_1 {
	ISP_AI_SCENE_TASK1_NIGHT,
	ISP_AI_SCENE_TASK1_BACKLIGHT,
	ISP_AI_SCENE_TASK1_SUNRISESET,
	ISP_AI_SCENE_TASK1_FIREWORK,
	ISP_AI_SCENE_TASK1_OTHERS,
	ISP_AI_SCENE_TASK1_MAX
};

enum isp_ai_task_2 {
	ISP_AI_SCENE_TASK2_FOOD,
	ISP_AI_SCENE_TASK2_GREENPLANT,
	ISP_AI_SCENE_TASK2_DOCUMENT,
	ISP_AI_SCENE_TASK2_CATDOG,
	ISP_AI_SCENE_TASK2_FLOWER,
	ISP_AI_SCENE_TASK2_BLUESKY,
	ISP_AI_SCENE_TASK2_BUILDING,
	ISP_AI_SCENE_TASK2_SNOW,
	ISP_AI_SCENE_TASK2_OTHERS,
	ISP_AI_SCENE_TASK2_MAX
};

struct isp_ai_task0_result {
	enum isp_ai_task_0 id;
	cmr_u16 score;
};

struct isp_ai_task1_result {
	enum isp_ai_task_1 id;
	cmr_u16 score;
};

struct isp_ai_task2_result {
	enum isp_ai_task_2 id;
	cmr_u16 score;
};

struct isp_ai_scene_detect_info {
	cmr_u32 frame_id;
	enum isp_ai_scene_type cur_scene_id;
	struct isp_ai_task0_result task0[ISP_AI_SCENE_TASK0_MAX];
	struct isp_ai_task1_result task1[ISP_AI_SCENE_TASK1_MAX];
	struct isp_ai_task2_result task2[ISP_AI_SCENE_TASK2_MAX];
};

enum isp_ai_img_flag {
	ISP_IMAGE_DATA_NOT_REQUIRED,
	ISP_IMAGE_DATA_REQUIRED,
	ISP_IMAGE_DATA_MAX
};

struct isp_ai_img_status {
	cmr_u32 frame_id;
	cmr_s32 frame_state;
	enum isp_ai_img_flag img_flag;
};


typedef cmr_int(*isp_ae_cb) (cmr_handle handle, cmr_int type, void *param0, void *param1);
typedef cmr_int(*isp_af_cb) (cmr_handle handle, cmr_int type, void *param0, void *param1);
typedef cmr_int(*isp_pdaf_cb) (cmr_handle handle, cmr_int type, void *param0, void *param1);
typedef cmr_int(*isp_afl_cb) (cmr_handle handle, cmr_int type, void *param0, void *param1);

cmr_int isp_init(struct isp_init_param *ptr, cmr_handle *handle);
cmr_int isp_deinit(cmr_handle handle);
cmr_int isp_capability(cmr_handle handle, enum isp_capbility_cmd cmd, void *param_ptr);
cmr_int isp_ioctl(cmr_handle handle, enum isp_ctrl_cmd cmd, void *param_ptr);
cmr_int isp_video_start(cmr_handle handle, struct isp_video_start *param_ptr);
cmr_int isp_video_stop(cmr_handle handle);
cmr_int isp_proc_start(cmr_handle handle, struct ips_in_param *in_param_ptr, struct ips_out_param *out_ptr);


cmr_int isp_proc_next(cmr_handle handle, struct ipn_in_param *in_ptr, struct ips_out_param *out_ptr);
void ispmw_dev_buf_cfg_evt_cb(cmr_handle handle, isp_buf_cfg_evt_cb grab_event_cb);
void isp_statis_evt_cb(cmr_int evt, void *data, void *privdata);
void isp_irq_proc_evt_cb(cmr_int evt, void *data, void *privdata);
#endif
