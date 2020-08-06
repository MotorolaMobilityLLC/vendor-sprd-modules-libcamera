/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#define LOG_TAG "af_sprd_adpt_v1"
#define ATRACE_TAG (ATRACE_TAG_CAMERA | ATRACE_TAG_HAL)
#include <cutils/trace.h>

#include <assert.h>
#include <cutils/properties.h>
#include <dlfcn.h>
#include <inttypes.h>

#include "af_ctrl.h"
#include "af_sprd_adpt_v1.h"

#ifndef UNUSED
#define     UNUSED(param)  (void)(param)
#endif

#define FOCUS_STAT_DATA_NUM 2

#define IMX351_PD_AREA_NUMBER 192
#define IMX351_PD_ROI_NUMBER 4
#define IMX351_SPC_SIZE 140	//70*2bytes

//IMX351 PD Result Array
static double IMX351_phase_difference[IMX351_PD_AREA_NUMBER];
static cmr_u32 IMX351_confidence_value[IMX351_PD_AREA_NUMBER];
static cmr_u32 g_FrameID = 0;

//#define Enable_mlog_AFtime

#ifdef Enable_mlog_AFtime

static char *focus_type_str[] = {
	"SAF",
	"CAF",
	"FAF",
	"PDAF",
	"TOF",
};

#define FOCUS_TYPE_STR(state)    focus_type_str[state]

#endif
//IMX351 PD OTP Address
//static cmr_u8 *pdaf_rdm_otp_data = NULL;
//[TOF_+++]

static cmr_u32 tof_FrameID = 0;
//[TOF_---]

#ifndef MAX
#define  MAX( _x, _y ) ( ((_x) > (_y)) ? (_x) : (_y) )
#endif

#ifndef MIN
#define  MIN( _x, _y ) ( ((_x) < (_y)) ? (_x) : (_y) )
#endif

#ifndef ABS_AB
#define ABS_AB(a, b)    ((a)>(b)? (a)-(b): (b)-(a))
#endif

#ifndef ABS
#define ABS(a)    ((a)>(0)? (a):(-a))
#endif

static const char *state_string[] = {
	"manual",
	"normal_af",
	"caf",
	"record caf",
	"faf",
	"fullscan",
	"picture",
	"objecttracking",
};

#define STATE_STRING(state)    state_string[state]

static const char *focus_state_str[] = {
	"af idle",
	"af searching",
	"af stop",
	"af stop inner",
};

#define FOCUS_STATE_STR(state)    focus_state_str[state]

static char AFlog_buffer[2048] = { 0 };

#if !defined(CONFIG_ISP_2_5) && !defined(CONFIG_ISP_2_6) && !defined(CONFIG_ISP_2_7)
static struct af_iir_nr_info_u af_iir_nr[3] = {
	{			// weak
	 .iir_nr_en = 1,
	 .iir_g0 = 378,
	 .iir_c1 = -676,
	 .iir_c2 = -324,
	 .iir_c3 = 512,
	 .iir_c4 = 1024,
	 .iir_c5 = 512,
	 .iir_g1 = 300,
	 .iir_c6 = -537,
	 .iir_c7 = -152,
	 .iir_c8 = 512,
	 .iir_c9 = 1024,
	 .iir_c10 = 512,
	 },
	{			// medium
	 .iir_nr_en = 1,
	 .iir_g0 = 185,
	 .iir_c1 = 0,
	 .iir_c2 = -229,
	 .iir_c3 = 512,
	 .iir_c4 = 1024,
	 .iir_c5 = 512,
	 .iir_g1 = 133,
	 .iir_c6 = 0,
	 .iir_c7 = -20,
	 .iir_c8 = 512,
	 .iir_c9 = 1024,
	 .iir_c10 = 512,
	 },
	{			// strong
	 .iir_nr_en = 1,
	 .iir_g0 = 81,
	 .iir_c1 = 460,
	 .iir_c2 = -270,
	 .iir_c3 = 512,
	 .iir_c4 = 1024,
	 .iir_c5 = 512,
	 .iir_g1 = 60,
	 .iir_c6 = 344,
	 .iir_c7 = 74,
	 .iir_c8 = 512,
	 .iir_c9 = 1024,
	 .iir_c10 = 512,
	 },
};

static struct af_enhanced_module_info_u af_enhanced_module = {
	.chl_sel = 0,
	.nr_mode = 2,
	.center_weight = 2,
	.fv_enhanced_mode = {5, 5},
	.clip_en = {0, 0},
	.max_th = {131071, 131071},
	.min_th = {100, 100},
	.fv_shift = {0, 0},
	.fv1_coeff = {
		      -2, -2, -2, -2, 16, -2, -2, -2, -2,
		      -3, 5, 3, 5, 0, -5, 3, -5, -3,
		      3, 5, -3, -5, 0, 5, -3, -5, 3,
		      0, -8, 0, -8, 16, 0, 0, 0, 0},
};
#endif

char libafv1_path[][20] = {
	"libspafv1.so",
	"libspafv1_le.so",
	"libaf_v2.so",
	"libaf_v3.so",
	"libaf_v4.so",
	"libaf_v5.so",
};

static cmr_s32 _check_handle(cmr_handle handle)
{
	cmr_s32 rtn = AFV1_SUCCESS;
	af_ctrl_t *af = (af_ctrl_t *) handle;

	if (NULL == af) {
		ISP_LOGE("fail to get valid cxt pointer");
		return AFV1_ERROR;
	}

	return rtn;
}

// misc
static cmr_u64 get_systemtime_ns()
{
	cmr_s64 timestamp = systemTime(CLOCK_MONOTONIC);
	return timestamp;
}

// afm hardware
static void afm_enable(af_ctrl_t * af)
{
	cmr_u32 bypass = 0;

	af->cb_ops.af_monitor_bypass(af->caller, (void *)&bypass);
}

static void afm_disable(af_ctrl_t * af)
{
	cmr_u32 bypass = 1;
	af->cb_ops.af_monitor_bypass(af->caller, (void *)&bypass);
}

static void afm_setup(af_ctrl_t * af)
{
#if !defined(CONFIG_ISP_2_5) && !defined(CONFIG_ISP_2_6) && !defined(CONFIG_ISP_2_7)
	struct af_enhanced_module_info_u afm_enhanced_module;
#endif
	cmr_u32 mode = 1;

	af->cb_ops.af_monitor_mode(af->caller, (void *)&mode);
#if !defined(CONFIG_ISP_2_5) && !defined(CONFIG_ISP_2_6) && !defined(CONFIG_ISP_2_7)
	af->cb_ops.af_monitor_iir_nr_cfg(af->caller, (void *)&(af_iir_nr[af->afm_tuning.iir_level]));

	memcpy(&(afm_enhanced_module), &af_enhanced_module, sizeof(struct af_enhanced_module_info_u));
	afm_enhanced_module.nr_mode = af->afm_tuning.nr_mode;
	afm_enhanced_module.center_weight = af->afm_tuning.cw_mode;
	afm_enhanced_module.fv_enhanced_mode[0] = af->afm_tuning.fv0_e;
	afm_enhanced_module.fv_enhanced_mode[1] = af->afm_tuning.fv1_e;
	af->cb_ops.af_monitor_module_cfg(af->caller, (void *)&(afm_enhanced_module));
#endif
}

static cmr_u32 afm_get_win_num(struct afctrl_init_in *input_param)
{
	cmr_u32 num;
	struct afctrl_init_in *input_ptr = input_param;
	input_ptr->cb_ops.get_monitor_win_num(input_ptr->caller, &num);
	ISP_LOGV("win_num %d", num);
	return num;
}

static void afm_set_win(af_ctrl_t * af, win_coord_t * win, cmr_s32 num, cmr_s32 hw_num)
{
	cmr_s32 i;
	struct af_monitor_win winparam;

	for (i = num; i < hw_num; ++i) {
		// some strange hardware behavior
		win[i].start_x = 0;
		win[i].start_y = 0;
		win[i].end_x = 2;
		win[i].end_y = 2;
	}
	ISP_LOGI("camera_id %d", af->camera_id);

#if defined(CONFIG_ISP_2_5) || defined(CONFIG_ISP_2_6) || defined(CONFIG_ISP_2_7)
#define PIXEL_OFFSET 100
	if (STATE_FAF == af->state && FACE_NONE != af->f_orientation) {	//face roi settings
		// crop enable
		cmr_u32 crop_eb = 1;
		struct af_monitor_tile_num tile_num;
		struct af_monitor_win_num win_num;
		cmr_u32 tmp_w = (win[8].end_x - win[0].start_x) / 3;	//af->isp_info.width;

		ISP_LOGI("roll_angle %d", af->roll_angle);
		// win num depends on gsensor
		win[9].start_x = win[0].start_x;
		win[9].start_y = win[0].start_y;
		win[9].end_x = win[8].end_x;
		win[9].end_y = win[8].end_y;
		if (FACE_UP == af->f_orientation || FACE_DOWN == af->f_orientation) {
			win_num.x = 4;
			win_num.y = 6;
			af->win_offset = 0;
			if (win[9].end_x + tmp_w + 4 > af->isp_info.width) {
				win[9].start_x = win[9].start_x - tmp_w;
				af->win_offset = 1;
			} else {
				win[9].end_x = win[9].end_x + tmp_w;
				af->win_offset = 0;
			}
		} else {
			win_num.x = 6;
			win_num.y = 3;
		}

		win[9].start_x = win[9].start_x > PIXEL_OFFSET ? win[9].start_x - PIXEL_OFFSET : 10;
		win[9].end_x = win[9].end_x + PIXEL_OFFSET > af->isp_info.width ? af->isp_info.width - 10 : win[9].end_x + PIXEL_OFFSET;
#if defined(CONFIG_ISP_2_5) || defined(CONFIG_ISP_2_6)
		if (win[9].end_x - win[9].start_x > 2336) {
			cmr_u32 center_x = (win[9].start_x + win[9].end_x) >> 1;
			switch (af->f_orientation) {
			case FACE_UP:
			case FACE_DOWN:
				win[9].start_x = center_x - (2336 >> 1);
				win[9].end_x = center_x + (2336 >> 1);
				break;
			case FACE_LEFT:
				win[9].end_x = win[9].start_x + 2336;
				break;
			case FACE_RIGHT:
				win[9].start_x = win[9].end_x - 2336;
				break;
			default:
				break;
			}
		}
#endif
		win[9].start_x = (win[9].start_x >> 1) << 1;
		win[9].start_y = (win[9].start_y >> 1) << 1;
		win[9].end_x = (win[9].end_x >> 1) << 1;
		win[9].end_y = (win[9].end_y >> 1) << 1;

		// crop size
		winparam.win_rect.x = win[9].start_x;
		winparam.win_rect.y = win[9].start_y;
		winparam.win_rect.w = win[9].end_x - win[9].start_x;
		winparam.win_rect.h = win[9].end_y - win[9].start_y;

		// tile num
		tile_num.x = win_num.x - 1;
		tile_num.y = win_num.y - 1;

		af->cb_ops.af_monitor_crop_eb(af->caller, &crop_eb);
		af->cb_ops.af_monitor_crop_size(af->caller, &winparam.win_rect);
		af->cb_ops.set_monitor_win_num(af->caller, &win_num);
		af->cb_ops.af_monitor_done_tile_num(af->caller, &tile_num);
		// the first roi in crop size
		winparam.win_rect.x = PIXEL_OFFSET;
		winparam.win_rect.y = 4;
		winparam.win_rect.w = (win[9].end_x - win[9].start_x - 2 * PIXEL_OFFSET) / win_num.x;
		winparam.win_rect.h = (win[9].end_y - win[9].start_y - 8) / win_num.y;
		winparam.win_rect.w = (winparam.win_rect.w >> 1) << 1;
		winparam.win_rect.h = (winparam.win_rect.h >> 1) << 1;
	} else if (STATE_FULLSCAN == af->state && (AF_ALG_BLUR_REAR == af->is_multi_mode)) {
		// crop enable
		cmr_u32 crop_eb = 1;
		struct af_monitor_tile_num tile_num;
		struct af_monitor_win_num win_num;

		win[9].start_x = (af->isp_info.width >> 2) + 4;
		win[9].end_x = af->isp_info.width - (af->isp_info.width >> 2) - 4;
		win[9].start_x = (win[9].start_x >> 1) << 1;
		win[9].start_y = (win[9].start_y >> 1) << 1;
		win[9].end_x = (win[9].end_x >> 1) << 1;
		win[9].end_y = (win[9].end_y >> 1) << 1;

		// crop size
		winparam.win_rect.x = win[9].start_x;
		winparam.win_rect.y = win[9].start_y;
		winparam.win_rect.w = win[9].end_x - win[9].start_x;
		winparam.win_rect.h = win[9].end_y - win[9].start_y;
		// win num
		win_num.x = 6;
		win_num.y = 12;
		// tile num
		tile_num.x = win_num.x - 1;
		tile_num.y = win_num.y - 1;

		af->cb_ops.af_monitor_crop_eb(af->caller, &crop_eb);
		af->cb_ops.af_monitor_crop_size(af->caller, &winparam.win_rect);
		af->cb_ops.set_monitor_win_num(af->caller, &win_num);
		af->cb_ops.af_monitor_done_tile_num(af->caller, &tile_num);
		// the first roi in crop size
		winparam.win_rect.x = PIXEL_OFFSET;
		winparam.win_rect.y = 4;
		winparam.win_rect.w = (win[9].end_x - win[9].start_x - 2 * PIXEL_OFFSET) / win_num.x;
		winparam.win_rect.h = (win[9].end_y - win[9].start_y - 8) / win_num.y;
		winparam.win_rect.w = (winparam.win_rect.w >> 1) << 1;
		winparam.win_rect.h = (winparam.win_rect.h >> 1) << 1;
	} else {
		// crop enable
		cmr_u32 crop_eb = 1;
		struct af_monitor_tile_num tile_num;
		struct af_monitor_win_num win_num;
		/*cmr_u32 tmp_w = (win[9].end_x - win[9].start_x)/3;//af->isp_info.width;

		   af->win_offset = 0;
		   if(win[9].end_x + tmp_w + 4 > af->isp_info.width) {
		   win[9].start_x = win[9].start_x - tmp_w;
		   af->win_offset = 1;
		   }else{
		   win[9].end_x = win[9].end_x + tmp_w;
		   af->win_offset = 0;
		   } */
		if (win[9].start_x > PIXEL_OFFSET) {
			win[9].start_x = win[9].start_x - PIXEL_OFFSET;
			win[9].end_x = win[9].end_x > af->isp_info.width ? af->isp_info.width - 10 : win[9].end_x;
		} else {
			win[9].start_x = 10;
			win[9].end_x = win[9].end_x + PIXEL_OFFSET > af->isp_info.width ? af->isp_info.width - 10 : win[9].end_x + PIXEL_OFFSET;
		}
		//win[9].start_x = win[9].start_x > PIXEL_OFFSET ? win[9].start_x - PIXEL_OFFSET : 10;
		//win[9].end_x = win[9].end_x + PIXEL_OFFSET > af->isp_info.width ? af->isp_info.width - 10 : win[9].end_x + PIXEL_OFFSET;
		win[9].start_x = (win[9].start_x >> 1) << 1;
		win[9].start_y = (win[9].start_y >> 1) << 1;
		win[9].end_x = (win[9].end_x >> 1) << 1;
		win[9].end_y = (win[9].end_y >> 1) << 1;

		// crop size
		winparam.win_rect.x = win[9].start_x;
		winparam.win_rect.y = win[9].start_y;
		winparam.win_rect.w = win[9].end_x - win[9].start_x;
		winparam.win_rect.h = win[9].end_y - win[9].start_y;
		// win num
		win_num.x = 6;
		win_num.y = 3;
		// tile num
		tile_num.x = win_num.x - 1;
		tile_num.y = win_num.y - 1;
		ISP_LOGD("win_num x,y %d%d", win_num.x, win_num.y);

		af->cb_ops.af_monitor_crop_eb(af->caller, &crop_eb);
		af->cb_ops.af_monitor_crop_size(af->caller, &winparam.win_rect);
		af->cb_ops.set_monitor_win_num(af->caller, &win_num);
		af->cb_ops.af_monitor_done_tile_num(af->caller, &tile_num);
		// the first roi in crop size
		winparam.win_rect.x = PIXEL_OFFSET;
		winparam.win_rect.y = 4;
		winparam.win_rect.w = (win[9].end_x - win[9].start_x - PIXEL_OFFSET) / win_num.x;
		winparam.win_rect.h = (win[9].end_y - win[9].start_y - 8) / win_num.y;
		winparam.win_rect.w = (winparam.win_rect.w >> 1) << 1;
		winparam.win_rect.h = (winparam.win_rect.h >> 1) << 1;
	}
#else
	winparam.win_pos = (struct af_win_rect *)win;	//todo : compare with kernel type
#endif

	af->cb_ops.set_monitor_win(af->caller, &winparam);
}

static cmr_s32 afm_set_fv(af_ctrl_t * af, void *in)
{
	cmr_u32 *af_fv_val = NULL;
	cmr_u8 i = 0;
	af_fv_val = (cmr_u32 *) in;
	char prop[256];
	int val = 0;
	property_get("debug.isp.af.fvlog", prop, "0");
	val = atoi(prop);

#if defined(CONFIG_ISP_2_5) || defined(CONFIG_ISP_2_6) || defined(CONFIG_ISP_2_7)

#if defined(CONFIG_ISP_2_5)
#define FV0_INDEX(block) (6 * ((block) >> 1) + ((block) & 0x01) + 4)
#define FV1_INDEX(block) (6 * ((block) >> 1) + ((block) & 0x01) + 2)
#endif

#if defined(CONFIG_ISP_2_6) || defined(CONFIG_ISP_2_7)
#define FV0_INDEX(block) ((block) * 3)
#define FV1_INDEX(block) (((block) * 3) + 1)
#endif

	if (STATE_FAF == af->state && FACE_NONE != af->f_orientation) {	//face FV mapping
		ISP_LOGI("roll_angle %d", af->roll_angle);
		if (FACE_UP == af->f_orientation || FACE_DOWN == af->f_orientation) {	//6x4
			if (FACE_UP == af->f_orientation) {
				for (i = 0; i < 6; i++) {	//3
					af->af_fv_val.af_fv0[i] = (cmr_u64) af_fv_val[FV0_INDEX(i / 3 * 4 + i % 3 + af->win_offset)];
					af->af_fv_val.af_fv1[i] = (cmr_u64) af_fv_val[FV1_INDEX(i / 3 * 4 + i % 3 + af->win_offset)];
					if (val == 1) {
						ISP_LOGI("i: %d, af_fv0[%d]: %15" PRIu64 ", j:%d, FV0_INDEX(%d):%d , af_fv_val[%d]:%d",
							 i, i, af->af_fv_val.af_fv0[i], i / 3 * 4 + i % 3 + af->win_offset, i / 3 * 4 + i % 3 + af->win_offset,
							 FV0_INDEX(i / 3 * 4 + i % 3 + af->win_offset), FV0_INDEX(i / 3 * 4 + i % 3 + af->win_offset),
							 af_fv_val[FV0_INDEX(i / 3 * 4 + i % 3 + af->win_offset)]);
					}
				}
				if (0 == af->win_offset) {
					af->af_fv_val.af_fv0[7] = (cmr_u64) af_fv_val[FV0_INDEX(9)];
					af->af_fv_val.af_fv1[7] = (cmr_u64) af_fv_val[FV1_INDEX(9)];
					af->af_fv_val.af_fv0[6] = (cmr_u64) af_fv_val[FV0_INDEX(20)] + af_fv_val[FV0_INDEX(21)];
					af->af_fv_val.af_fv1[6] = (cmr_u64) af_fv_val[FV1_INDEX(20)] + af_fv_val[FV1_INDEX(21)];
					af->af_fv_val.af_fv0[8] = (cmr_u64) af_fv_val[FV0_INDEX(22)] + af_fv_val[FV0_INDEX(21)];
					af->af_fv_val.af_fv1[8] = (cmr_u64) af_fv_val[FV1_INDEX(22)] + af_fv_val[FV1_INDEX(21)];
					if (val == 1) {
						ISP_LOGI("i: 6, af_fv0[6]: %15" PRIu64 ", j: 20, FV0_INDEX(20):%d ,FV0_INDEX(21):%d, af_fv_val[%d]:%d, af_fv_val[%d]:%d",
							 af->af_fv_val.af_fv0[6], FV0_INDEX(20), FV0_INDEX(21), FV0_INDEX(20), af_fv_val[FV0_INDEX(20)], FV0_INDEX(21),
							 af_fv_val[FV0_INDEX(21)]);
						ISP_LOGI("i: 7, af_fv0[7]: %15" PRIu64 ", j: 9, FV0_INDEX(9):%d , af_fv_val[%d]:%d", af->af_fv_val.af_fv0[7], FV0_INDEX(9),
							 FV0_INDEX(9), af_fv_val[FV0_INDEX(9)]);
						ISP_LOGI("i: 8, af_fv0[8]: %15" PRIu64 ", j: 22, FV0_INDEX(22):%d ,FV0_INDEX(21):%d, af_fv_val[%d]:%d, af_fv_val[%d]:%d",
							 af->af_fv_val.af_fv0[8], FV0_INDEX(22), FV0_INDEX(21), FV0_INDEX(22), af_fv_val[FV0_INDEX(22)], FV0_INDEX(21),
							 af_fv_val[FV0_INDEX(21)]);
					}
				} else {
					af->af_fv_val.af_fv0[7] = (cmr_u64) af_fv_val[FV0_INDEX(10)];
					af->af_fv_val.af_fv1[7] = (cmr_u64) af_fv_val[FV1_INDEX(10)];
					af->af_fv_val.af_fv0[6] = (cmr_u64) af_fv_val[FV0_INDEX(21)] + af_fv_val[FV0_INDEX(22)];
					af->af_fv_val.af_fv1[6] = (cmr_u64) af_fv_val[FV1_INDEX(21)] + af_fv_val[FV1_INDEX(22)];
					af->af_fv_val.af_fv0[8] = (cmr_u64) af_fv_val[FV0_INDEX(23)] + af_fv_val[FV0_INDEX(22)];
					af->af_fv_val.af_fv1[8] = (cmr_u64) af_fv_val[FV1_INDEX(23)] + af_fv_val[FV1_INDEX(22)];
					if (val == 1) {
						ISP_LOGE("i: 6, af_fv0[6]: %15" PRIu64 ", j: 21, FV0_INDEX(21):%d ,FV0_INDEX(22):%d, af_fv_val[%d]:%d, af_fv_val[%d]:%d",
							 af->af_fv_val.af_fv0[6], FV0_INDEX(21), FV0_INDEX(22), FV0_INDEX(21), af_fv_val[FV0_INDEX(21)], FV0_INDEX(22),
							 af_fv_val[FV0_INDEX(22)]);
						ISP_LOGE("i: 7, af_fv0[7]: %15" PRIu64 ", j: 10, FV0_INDEX(10):%d , af_fv_val[%d]:%d", af->af_fv_val.af_fv0[7],
							 FV0_INDEX(10), FV0_INDEX(10), af_fv_val[FV0_INDEX(10)]);
						ISP_LOGE("i: 8, af_fv0[8]: %15" PRIu64 ", j: 23, FV0_INDEX(23):%d ,FV0_INDEX(22):%d, af_fv_val[%d]:%d, af_fv_val[%d]:%d",
							 af->af_fv_val.af_fv0[8], FV0_INDEX(23), FV0_INDEX(22), FV0_INDEX(23), af_fv_val[FV0_INDEX(23)], FV0_INDEX(22),
							 af_fv_val[FV0_INDEX(22)]);
					}
				}
			} else if (FACE_DOWN == af->f_orientation) {
				for (i = 0; i < 6; i++) {	//3
					af->af_fv_val.af_fv0[i + 3] = (cmr_u64) af_fv_val[FV0_INDEX(i / 3 * 4 + i % 3 + 16 + af->win_offset)];
					af->af_fv_val.af_fv1[i + 3] = (cmr_u64) af_fv_val[FV1_INDEX(i / 3 * 4 + i % 3 + 16 + af->win_offset)];
					if (val == 1) {
						ISP_LOGE("i: %d, af_fv0[%d]: %15" PRIu64 ", j:%d, FV0_INDEX(%d):%d , af_fv_val[%d]:%d",
							 i + 3, i + 3, af->af_fv_val.af_fv0[i + 3], i / 3 * 4 + i % 3 + 16 + af->win_offset,
							 i / 3 * 4 + i % 3 + 16 + af->win_offset, FV0_INDEX(i / 3 * 4 + i % 3 + 16 + af->win_offset),
							 FV0_INDEX(i / 3 * 4 + i % 3 + 16 + af->win_offset), af_fv_val[FV0_INDEX(i / 3 * 4 + i % 3 + 16 + af->win_offset)]);
					}
				}
				if (0 == af->win_offset) {
					af->af_fv_val.af_fv0[1] = (cmr_u64) af_fv_val[FV0_INDEX(13)];
					af->af_fv_val.af_fv1[1] = (cmr_u64) af_fv_val[FV1_INDEX(13)];
					af->af_fv_val.af_fv0[0] = (cmr_u64) af_fv_val[FV0_INDEX(0)] + af_fv_val[FV0_INDEX(1)];
					af->af_fv_val.af_fv1[0] = (cmr_u64) af_fv_val[FV1_INDEX(0)] + af_fv_val[FV1_INDEX(1)];
					af->af_fv_val.af_fv0[2] = (cmr_u64) af_fv_val[FV0_INDEX(2)] + af_fv_val[FV0_INDEX(1)];
					af->af_fv_val.af_fv1[2] = (cmr_u64) af_fv_val[FV1_INDEX(2)] + af_fv_val[FV1_INDEX(1)];
					if (val == 1) {
						ISP_LOGE("i: 0, af_fv0[0]: %15" PRIu64 ", j: 0, FV0_INDEX(0):%d ,FV0_INDEX(1):%d, af_fv_val[%d]:%d, af_fv_val[%d]:%d",
							 af->af_fv_val.af_fv0[0], FV0_INDEX(0), FV0_INDEX(1), FV0_INDEX(0), af_fv_val[FV0_INDEX(0)], FV0_INDEX(1),
							 af_fv_val[FV0_INDEX(1)]);
						ISP_LOGE("i: 1, af_fv0[1]: %15" PRIu64 ", j: 13, FV0_INDEX(13):%d , af_fv_val[%d]:%d", af->af_fv_val.af_fv0[1],
							 FV0_INDEX(13), FV0_INDEX(13), af_fv_val[FV0_INDEX(13)]);
						ISP_LOGE("i: 2, af_fv0[2]: %15" PRIu64 ", j: 2, FV0_INDEX(2):%d ,FV0_INDEX(1):%d, af_fv_val[%d]:%d, af_fv_val[%d]:%d",
							 af->af_fv_val.af_fv0[2], FV0_INDEX(2), FV0_INDEX(1), FV0_INDEX(2), af_fv_val[FV0_INDEX(2)], FV0_INDEX(1),
							 af_fv_val[FV0_INDEX(1)]);
					}
				} else {
					af->af_fv_val.af_fv0[1] = (cmr_u64) af_fv_val[FV0_INDEX(14)];
					af->af_fv_val.af_fv1[1] = (cmr_u64) af_fv_val[FV1_INDEX(14)];
					af->af_fv_val.af_fv0[0] = (cmr_u64) af_fv_val[FV0_INDEX(1)] + af_fv_val[FV0_INDEX(2)];
					af->af_fv_val.af_fv1[0] = (cmr_u64) af_fv_val[FV1_INDEX(1)] + af_fv_val[FV1_INDEX(2)];
					af->af_fv_val.af_fv0[2] = (cmr_u64) af_fv_val[FV0_INDEX(3)] + af_fv_val[FV0_INDEX(2)];
					af->af_fv_val.af_fv1[2] = (cmr_u64) af_fv_val[FV1_INDEX(3)] + af_fv_val[FV1_INDEX(2)];
					if (val == 1) {
						ISP_LOGE("i: 0, af_fv0[0]: %15" PRIu64 ", j: 1, FV0_INDEX(1):%d ,FV0_INDEX(2):%d, af_fv_val[%d]:%d, af_fv_val[%d]:%d",
							 af->af_fv_val.af_fv0[0], FV0_INDEX(1), FV0_INDEX(2), FV0_INDEX(1), af_fv_val[FV0_INDEX(1)], FV0_INDEX(2),
							 af_fv_val[FV0_INDEX(2)]);
						ISP_LOGE("i: 1, af_fv0[1]: %15" PRIu64 ", j: 14, FV0_INDEX(14):%d , af_fv_val[%d]:%d", af->af_fv_val.af_fv0[1],
							 FV0_INDEX(14), FV0_INDEX(14), af_fv_val[FV0_INDEX(14)]);
						ISP_LOGE("i: 2, af_fv0[2]: %15" PRIu64 ", j: 3, FV0_INDEX(3):%d ,FV0_INDEX(2):%d, af_fv_val[%d]:%d, af_fv_val[%d]:%d",
							 af->af_fv_val.af_fv0[2], FV0_INDEX(3), FV0_INDEX(2), FV0_INDEX(3), af_fv_val[FV0_INDEX(3)], FV0_INDEX(2),
							 af_fv_val[FV0_INDEX(2)]);
					}
				}
			}
		} else if (FACE_RIGHT == af->f_orientation || FACE_LEFT == af->f_orientation) {	//3//3x6
			if (FACE_LEFT == af->f_orientation) {
				for (i = 0; i < 6; i++) {	//3
					af->af_fv_val.af_fv0[i / 2 * 3 + i % 2] = (cmr_u64) af_fv_val[FV0_INDEX(i / 2 * 6 + i % 2)];
					af->af_fv_val.af_fv1[i / 2 * 3 + i % 2] = (cmr_u64) af_fv_val[FV1_INDEX(i / 2 * 6 + i % 2)];
					if (val == 1) {
						ISP_LOGE("i: %d, af_fv0[%d]: %15" PRIu64 ", j:%d, FV0_INDEX(%d):%d , af_fv_val[%d]:%d",
							 i / 2 * 3 + i % 2, i / 2 * 3 + i % 2, af->af_fv_val.af_fv0[i / 2 * 3 + i % 2], i / 2 * 6 + i % 2,
							 i / 2 * 6 + i % 2, FV0_INDEX(i / 2 * 6 + i % 2), FV0_INDEX(i / 2 * 6 + i % 2), af_fv_val[FV0_INDEX(i / 2 * 6 + i % 2)]);
					}
				}
				af->af_fv_val.af_fv0[5] = (cmr_u64) af_fv_val[FV0_INDEX(8)];
				af->af_fv_val.af_fv1[5] = (cmr_u64) af_fv_val[FV1_INDEX(8)];
				af->af_fv_val.af_fv0[2] = (cmr_u64) af_fv_val[FV0_INDEX(5)] + af_fv_val[FV0_INDEX(11)];
				af->af_fv_val.af_fv1[2] = (cmr_u64) af_fv_val[FV1_INDEX(5)] + af_fv_val[FV1_INDEX(11)];
				af->af_fv_val.af_fv0[8] = (cmr_u64) af_fv_val[FV0_INDEX(17)] + af_fv_val[FV0_INDEX(11)];
				af->af_fv_val.af_fv1[8] = (cmr_u64) af_fv_val[FV1_INDEX(17)] + af_fv_val[FV1_INDEX(11)];
				if (val == 1) {
					ISP_LOGE("i: 2, af_fv0[2]: %15" PRIu64 ", j: 5, FV0_INDEX(5):%d ,FV0_INDEX(11):%d, af_fv_val[%d]:%d, af_fv_val[%d]:%d",
						 af->af_fv_val.af_fv0[2], FV0_INDEX(5), FV0_INDEX(11), FV0_INDEX(5), af_fv_val[FV0_INDEX(5)], FV0_INDEX(11),
						 af_fv_val[FV0_INDEX(11)]);
					ISP_LOGE("i: 5, af_fv0[5]: %15" PRIu64 ", j: 8, FV0_INDEX(8):%d , af_fv_val[%d]:%d", af->af_fv_val.af_fv0[5], FV0_INDEX(8),
						 FV0_INDEX(8), af_fv_val[FV0_INDEX(8)]);
					ISP_LOGE("i: 8, af_fv0[8]: %15" PRIu64 ", j: 17, FV0_INDEX(17):%d ,FV0_INDEX(11):%d, af_fv_val[%d]:%d, af_fv_val[%d]:%d",
						 af->af_fv_val.af_fv0[8], FV0_INDEX(17), FV0_INDEX(11), FV0_INDEX(17), af_fv_val[FV0_INDEX(17)], FV0_INDEX(11),
						 af_fv_val[FV0_INDEX(11)]);
				}
			} else if (FACE_RIGHT == af->f_orientation) {
				for (i = 0; i < 6; i++) {	//3
					af->af_fv_val.af_fv0[i / 2 * 3 + i % 2 + 1] = (cmr_u64) af_fv_val[FV0_INDEX(i / 2 * 6 + i % 2 + 4)];
					af->af_fv_val.af_fv1[i / 2 * 3 + i % 2 + 1] = (cmr_u64) af_fv_val[FV1_INDEX(i / 2 * 6 + i % 2 + 4)];
					if (val == 1) {
						ISP_LOGE("i: %d, af_fv0[%d]: %15" PRIu64 ", j:%d, FV0_INDEX(%d):%d , af_fv_val[%d]:%d",
							 i / 2 * 3 + i % 2 + 1, i / 2 * 3 + i % 2 + 1, af->af_fv_val.af_fv0[i / 2 * 3 + i % 2 + 1], i / 2 * 6 + i % 2 + 4,
							 i / 2 * 6 + i % 2 + 4, FV0_INDEX(i / 2 * 6 + i % 2 + 4), FV0_INDEX(i / 2 * 6 + i % 2 + 4),
							 af_fv_val[FV0_INDEX(i / 2 * 6 + i % 2 + 4)]);
					}
				}
				af->af_fv_val.af_fv0[3] = (cmr_u64) af_fv_val[FV0_INDEX(9)];
				af->af_fv_val.af_fv1[3] = (cmr_u64) af_fv_val[FV1_INDEX(9)];
				af->af_fv_val.af_fv0[0] = (cmr_u64) af_fv_val[FV0_INDEX(0)] + af_fv_val[FV0_INDEX(6)];
				af->af_fv_val.af_fv1[0] = (cmr_u64) af_fv_val[FV1_INDEX(0)] + af_fv_val[FV1_INDEX(6)];
				af->af_fv_val.af_fv0[6] = (cmr_u64) af_fv_val[FV0_INDEX(12)] + af_fv_val[FV0_INDEX(6)];
				af->af_fv_val.af_fv1[6] = (cmr_u64) af_fv_val[FV1_INDEX(12)] + af_fv_val[FV1_INDEX(6)];
				if (val == 1) {
					ISP_LOGE("i: 0, af_fv0[0]: %15" PRIu64 ", j: 0, FV0_INDEX(0):%d ,FV0_INDEX(6):%d, af_fv_val[%d]:%d, af_fv_val[%d]:%d",
						 af->af_fv_val.af_fv0[0], FV0_INDEX(0), FV0_INDEX(6), FV0_INDEX(0), af_fv_val[FV0_INDEX(0)], FV0_INDEX(6), af_fv_val[FV0_INDEX(6)]);
					ISP_LOGE("i: 3, af_fv0[3]: %15" PRIu64 ", j: 9, FV0_INDEX(9):%d , af_fv_val[%d]:%d", af->af_fv_val.af_fv0[3], FV0_INDEX(9),
						 FV0_INDEX(9), af_fv_val[FV0_INDEX(9)]);
					ISP_LOGE("i: 6, af_fv0[6]: %15" PRIu64 ", j: 12, FV0_INDEX(12):%d ,FV0_INDEX(6):%d, af_fv_val[%d]:%d, af_fv_val[%d]:%d",
						 af->af_fv_val.af_fv0[6], FV0_INDEX(12), FV0_INDEX(6), FV0_INDEX(12), af_fv_val[FV0_INDEX(12)], FV0_INDEX(6),
						 af_fv_val[FV0_INDEX(6)]);
				}
			}
		}
	} else if (STATE_FULLSCAN == af->state && (AF_ALG_BLUR_REAR == af->is_multi_mode)) {
		af->af_fv_val.af_fv0[9] = 0;
		af->af_fv_val.af_fv1[9] = 0;
		for (i = 0; i < 3; i++) {	//3//3x3 map to 12x6
			af->af_fv_val.af_fv0[i] = (cmr_u64) af_fv_val[FV0_INDEX(12 + 2 * i)] + af_fv_val[FV0_INDEX(13 + 2 * i)] +
			    af_fv_val[FV0_INDEX(18 + 2 * i)] + af_fv_val[FV0_INDEX(19 + 2 * i)];
			af->af_fv_val.af_fv1[i] = (cmr_u64) af_fv_val[FV1_INDEX(12 + 2 * i)] + af_fv_val[FV1_INDEX(13 + 2 * i)] +
			    af_fv_val[FV1_INDEX(18 + 2 * i)] + af_fv_val[FV1_INDEX(19 + 2 * i)];
			af->af_fv_val.af_fv0[i] = af->af_fv_val.af_fv0[i] << 1;
			af->af_fv_val.af_fv1[i] = af->af_fv_val.af_fv1[i] << 1;
			af->af_fv_val.af_fv0[9] += af->af_fv_val.af_fv0[i];
			af->af_fv_val.af_fv1[9] += af->af_fv_val.af_fv1[i];

			af->af_fv_val.af_fv0[i + 3] = (cmr_u64) af_fv_val[FV0_INDEX(30 + 2 * i)] + af_fv_val[FV0_INDEX(31 + 2 * i)] +
			    af_fv_val[FV0_INDEX(36 + 2 * i)] + af_fv_val[FV0_INDEX(37 + 2 * i)];
			af->af_fv_val.af_fv1[i + 3] = (cmr_u64) af_fv_val[FV1_INDEX(30 + 2 * i)] + af_fv_val[FV1_INDEX(31 + 2 * i)] +
			    af_fv_val[FV1_INDEX(36 + 2 * i)] + af_fv_val[FV1_INDEX(37 + 2 * i)];
			af->af_fv_val.af_fv0[i + 3] = af->af_fv_val.af_fv0[i + 3] << 1;
			af->af_fv_val.af_fv1[i + 3] = af->af_fv_val.af_fv1[i + 3] << 1;
			af->af_fv_val.af_fv0[9] += af->af_fv_val.af_fv0[i + 3];
			af->af_fv_val.af_fv1[9] += af->af_fv_val.af_fv1[i + 3];

			af->af_fv_val.af_fv0[i + 6] = (cmr_u64) af_fv_val[FV0_INDEX(48 + 2 * i)] + af_fv_val[FV0_INDEX(49 + 2 * i)] +
			    af_fv_val[FV0_INDEX(54 + 2 * i)] + af_fv_val[FV0_INDEX(55 + 2 * i)];
			af->af_fv_val.af_fv1[i + 6] = (cmr_u64) af_fv_val[FV1_INDEX(48 + 2 * i)] + af_fv_val[FV1_INDEX(49 + 2 * i)] +
			    af_fv_val[FV1_INDEX(54 + 2 * i)] + af_fv_val[FV1_INDEX(55 + 2 * i)];
			af->af_fv_val.af_fv0[i + 6] = af->af_fv_val.af_fv0[i + 6] << 1;
			af->af_fv_val.af_fv1[i + 6] = af->af_fv_val.af_fv1[i + 6] << 1;
			af->af_fv_val.af_fv0[9] += af->af_fv_val.af_fv0[i + 6];
			af->af_fv_val.af_fv1[9] += af->af_fv_val.af_fv1[i + 6];
		}
	} else {
		af->af_fv_val.af_fv0[9] = 0;
		af->af_fv_val.af_fv1[9] = 0;
		for (i = 0; i < 9; i++) {
			cmr_u8 j = i << 1;	// 3x3 map to 3x6
			af->af_fv_val.af_fv0[i] = af_fv_val[FV0_INDEX(j)] + af_fv_val[FV0_INDEX(j + 1)];
			af->af_fv_val.af_fv1[i] = af_fv_val[FV1_INDEX(j)] + af_fv_val[FV1_INDEX(j + 1)];
			af->af_fv_val.af_fv0[9] += af->af_fv_val.af_fv0[i];
			af->af_fv_val.af_fv1[9] += af->af_fv_val.af_fv1[i];
			if (val == 1) {
				ISP_LOGI("i: %d, af_fv0[%d]: %15" PRIu64 ", j:%d, FV0_INDEX(%d):%d ,FV0_INDEX(%d):%d, af_fv_val[%d]:%d, af_fv_val[%d]:%d",
					 i, i, af->af_fv_val.af_fv0[i], j, j, FV0_INDEX(j), j + 1, FV0_INDEX(j + 1), FV0_INDEX(j), af_fv_val[FV0_INDEX(j)],
					 FV0_INDEX(j + 1), af_fv_val[FV0_INDEX(j + 1)]);
			}
		}
	}
#endif

#ifdef CONFIG_ISP_2_4
	for (i = 0; i < 10; i++) {
		cmr_u64 high = af_fv_val[95 + i / 2];
		high = (i & 0x01) ? ((high & 0x00FF0000) << 16) : ((high & 0x000000FF) << 32);
		af->af_fv_val.af_fv0[i] = af_fv_val[61 + i * 3] + high;	// spsmd g channels

		high = af_fv_val[95 + i / 2];
		high = (i & 0x01) ? ((high & 0x0F000000) << 12) : ((high & 0x00000F00) << 24);
		af->af_fv_val.af_fv1[i] = af_fv_val[31 + i * 3] + high;	// soble9x9 g channels
	}
#endif

#if defined(CONFIG_ISP_2_1) || defined(CONFIG_ISP_2_2) || defined(CONFIG_ISP_2_3)	// ISP2.1/2.2/2.3 share same AFM filter,
	for (i = 0; i < 10; i++) {
		af->af_fv_val.af_fv0[i] = ((((cmr_u64) af_fv_val[20 + i]) & 0x00000fff) << 32) | (((cmr_u64) af_fv_val[i]));
		af->af_fv_val.af_fv1[i] = (((((cmr_u64) af_fv_val[20 + i]) >> 12) & 0x00000fff) << 32) | ((cmr_u64) af_fv_val[10 + i]);
	}
#endif
	return 0;
}

static cmr_s32 afm_get_fv(af_ctrl_t * af, cmr_u64 * fv, cmr_u32 filter_mask, cmr_s32 roi_num)
{
	cmr_s32 i = 0;
	cmr_u64 *p = fv;

	if (filter_mask & SOBEL5_BIT) {	// not impelmented in this AFM
	}

	if (filter_mask & SOBEL9_BIT) {	// not impelmented in this AFM
	}

	if (filter_mask & SPSMD_BIT) {	// not impelmented in this AFM
	}

	if (filter_mask & ENHANCED_BIT) {
		for (i = 0; i < roi_num; ++i) {
			//ISP_LOGI("fv0[%d]:%15" PRIu64 ", fv1[%d]:%15" PRIu64 ".", i, af->af_fv_val.af_fv0[i], i, af->af_fv_val.af_fv1[i]);
			*p++ = af->af_fv_val.af_fv0[i];
		}
	}

	return 0;
}

// start hardware
static cmr_s32 do_start_af(af_ctrl_t * af)
{
	afm_set_win(af, af->roi.win, af->roi.num, af->isp_info.win_num);
	afm_setup(af);
	afm_enable(af);
	return 0;
}

// stop hardware
static cmr_s32 do_stop_af(af_ctrl_t * af)
{
	afm_disable(af);
	return 0;
}

// len
static cmr_u16 lens_get_pos(af_ctrl_t * af)
{
	cmr_u16 pos = 0;

	if (NULL == af->cb_ops.af_get_motor_pos) {
		ISP_LOGE("af->af_get_motor_pos null");
	} else {
		af->cb_ops.af_get_motor_pos(af->caller, &pos);
	}
	if (0 == pos || pos > 2047) {
		pos = af->lens.pos;
	}

	ISP_LOGV("pos = %d", pos);
	return pos;
}

static void lens_move_to(af_ctrl_t * af, cmr_u16 pos)
{
	cmr_u16 last_pos = 0;

	if (NULL == af->cb_ops.af_set_motor_pos) {
		ISP_LOGE("af->af_set_motor_pos null error");
		return;
	}

	last_pos = lens_get_pos(af);
	if (last_pos != pos) {
		ISP_LOGD("pos = %d", pos);
		af->cb_ops.af_set_motor_pos(af->caller, pos);
		af->lens.pos = pos;
	} else {
		ISP_LOGV("pos %d was set last time", pos);
	}
}

static void af_set_default_roi(af_ctrl_t * af, cmr_u32 alg_mode)
{
	spaf_roi_t af_roi;

	ISP_LOGI("current af->algo_mode %d", af->algo_mode);
	memset(&af_roi, 0, sizeof(spaf_roi_t));
	af_roi.af_mode = alg_mode;
	af_roi.win_num = 0;
	af_roi.multi_mode = af->is_multi_mode;
	af_roi.zoom_ratio = af->zoom_ratio;
	af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_SET_ROI, &af_roi);
}

static cmr_s32 compare_timestamp(af_ctrl_t * af)
{

	if (af->dcam_timestamp < af->vcm_timestamp + 100000000LL)
		return DCAM_AFTER_VCM_NO;
	else
		return DCAM_AFTER_VCM_YES;
}

static void notify_start(af_ctrl_t * af, cmr_u32 focus_type)
{
	ISP_LOGV(".");
	struct afctrl_notice af_result;
	cmr_u32 win_index;

	win_index = (af->roi.num > 1) ? (af->roi.num - 1) : 0;
	memset(&af_result, 0, sizeof(af_result));
	af_result.valid_win = 0;
	af_result.focus_type = focus_type;
	af_result.motor_pos = 0;
	af_result.af_mode = af->request_mode;
	if (0 != win_index && win_index < MAX_ROI_NUM) {
		if (STATE_FAF == af->state && 1 == af->win.win_num) {
			af_result.af_roi.sx = af->win.face[0].sx;
			af_result.af_roi.sy = af->win.face[0].sy;
			af_result.af_roi.ex = af->win.face[0].ex;
			af_result.af_roi.ey = af->win.face[0].ey;
		} else {
			af_result.af_roi.sx = af->roi.win[win_index].start_x;
			af_result.af_roi.sy = af->roi.win[win_index].start_y;
			af_result.af_roi.ex = af->roi.win[win_index].end_x;
			af_result.af_roi.ey = af->roi.win[win_index].end_y;
		}
		ISP_LOGI("af roi %d %d %d %d %d, state %d, num%d", win_index, af_result.af_roi.sx, af_result.af_roi.sy, af_result.af_roi.ex, af_result.af_roi.ey, af->state,
			 af->win.win_num);
	} else {
		ISP_LOGI("set_wins data error");
	}
	af->cb_ops.start_notice(af->caller, &af_result);
}

static void notify_stop(af_ctrl_t * af, cmr_s32 win_num, cmr_u32 focus_type)
{
	struct afctrl_notice af_result;
	cmr_u32 win_index;

	win_index = (af->roi.num > 1) ? (af->roi.num - 1) : 0;
	memset((void *)&af_result, 0, sizeof(af_result));
	af_result.valid_win = win_num;
	af_result.focus_type = focus_type;
	af_result.motor_pos = af->lens.pos;
	af_result.af_mode = af->request_mode;
	if (0 != win_index && win_index < MAX_ROI_NUM) {
		if (STATE_FAF == af->state && 1 == af->win.win_num) {
			af_result.af_roi.sx = af->win.face[0].sx;
			af_result.af_roi.sy = af->win.face[0].sy;
			af_result.af_roi.ex = af->win.face[0].ex;
			af_result.af_roi.ey = af->win.face[0].ey;
		} else {
			af_result.af_roi.sx = af->roi.win[win_index].start_x;
			af_result.af_roi.sy = af->roi.win[win_index].start_y;
			af_result.af_roi.ex = af->roi.win[win_index].end_x;
			af_result.af_roi.ey = af->roi.win[win_index].end_y;
		}
		ISP_LOGI("af roi %d %d %d %d %d, state %d, num%d", win_index, af_result.af_roi.sx, af_result.af_roi.sy, af_result.af_roi.ex, af_result.af_roi.ey, af->state,
			 af->win.win_num);
	} else {
		ISP_LOGI("set_wins data error");
	}
	ISP_LOGV(". %s ", (win_num) ? "Suc" : "Fail");

	ISP_LOGI("af->focus_state %d", af->focus_state);
	if (AF_STOPPED != af->focus_state) {
		ISP_LOGI("notify to uplayer");
		af->cb_ops.end_notice(af->caller, &af_result);
	}
#ifdef Enable_mlog_AFtime

	af->AFtime.system_time1_1 = systemTime(CLOCK_MONOTONIC);
	af->AFtime.time_total = ((af->AFtime.system_time1_1 - af->AFtime.system_time0_1) / 1000000);

#endif
}

// i/f to AF model
static cmr_u8 if_set_wins(cmr_u32 index, cmr_u32 start_x, cmr_u32 start_y, cmr_u32 end_x, cmr_u32 end_y, void *cookie)
{
	af_ctrl_t *af = cookie;
	roi_info_t *roi = &af->roi;

	if (0 == index)
		roi->num = 1;
	else
		roi->num = roi->num + 1;

	if (roi->num <= sizeof(roi->win) / sizeof(roi->win[0])) {
		roi->win[roi->num - 1].start_x = start_x;
		roi->win[roi->num - 1].start_y = start_y;
		roi->win[roi->num - 1].end_x = end_x;
		roi->win[roi->num - 1].end_y = end_y;
		ISP_LOGV("if_set_wins %d %d %d %d %d", roi->num - 1, roi->win[roi->num - 1].start_x, roi->win[roi->num - 1].start_y, roi->win[roi->num - 1].end_x,
			 roi->win[roi->num - 1].end_y);
	}

	return 0;
}

static cmr_u8 if_get_win_info(cmr_u32 * hw_num, cmr_u32 * isp_w, cmr_u32 * isp_h, void *cookie)
{
	af_ctrl_t *af = cookie;

	*isp_w = af->isp_info.width;
	*isp_h = af->isp_info.height;
	*hw_num = af->isp_info.win_num;
	return 0;
}

static cmr_u8 if_get_sub_wins_ysum(Y_Sum * c_y_sum, void *cookie)
{
	af_ctrl_t *af = cookie;
	cmr_u32 index = 0;
	while (index < 10) {
		c_y_sum->y_sum[index] = af->roi_RGBY.Y_sum[index];
		index += 1;
	}
	return 0;
}

static cmr_u8 if_statistics_wait_cal_done(void *cookie)
{
	UNUSED(cookie);
	return 0;
}

static cmr_u8 if_statistics_get_data(cmr_u64 fv[T_TOTAL_FILTER_TYPE], _af_stat_data_t * p_stat_data, void *cookie)
{
	af_ctrl_t *af = cookie;
	cmr_u64 spsmd[MAX_ROI_NUM];
	cmr_u64 sum = 0;
	memset(fv, 0, sizeof(fv[0]) * T_TOTAL_FILTER_TYPE);
	memset(&(spsmd[0]), 0, sizeof(cmr_u64) * MAX_ROI_NUM);
	afm_get_fv(af, spsmd, ENHANCED_BIT, af->roi.num);

	switch (af->state) {
	case STATE_FAF:
		sum = spsmd[0] + spsmd[1] + spsmd[2] + spsmd[3] + 8 * spsmd[4] + spsmd[5] + spsmd[6] + spsmd[7] + spsmd[8];
		fv[T_SPSMD] = sum;
		break;
	default:
		sum = spsmd[9];	//3///3x3 windows,the 9th window is biggest covering all the other window
		//sum = spsmd[1] + 8 * spsmd[2];        /// the 0th window cover 1st and 2nd window,1st window cover 2nd window
		fv[T_SPSMD] = sum;
		break;
	}

	if (p_stat_data) {	// for caf calc
		p_stat_data->roi_num = af->roi.num;
		p_stat_data->stat_num = FOCUS_STAT_DATA_NUM;
		p_stat_data->p_stat = &(af->af_fv_val.af_fv0[0]);
	}
	ISP_LOGV("[%d][%d]spsmd sum %" PRIu64 "", af->state, af->roi.num, sum);

	return 0;
}

static cmr_u8 if_statistics_set_data(cmr_u32 set_stat, void *cookie)
{
	af_ctrl_t *af = cookie;

	af->afm_tuning.fv0_e = (set_stat & 0x0f);
	af->afm_tuning.fv1_e = (set_stat & 0xf0) >> 4;
	af->afm_tuning.nr_mode = (set_stat & 0xff00) >> 8;
	af->afm_tuning.cw_mode = (set_stat & 0xff0000) >> 16;
	af->afm_tuning.iir_level = (set_stat & 0xff000000) >> 24;

	ISP_LOGV("[0x%x] fv0e %d, fv1e %d, nr %d, cw %d iir %d", set_stat, af->afm_tuning.fv0_e, af->afm_tuning.fv1_e, af->afm_tuning.nr_mode, af->afm_tuning.cw_mode,
		 af->afm_tuning.iir_level);
	afm_setup(af);
	return 0;
}

static cmr_u8 if_lens_get_pos(cmr_u16 * pos, void *cookie)
{
	af_ctrl_t *af = cookie;
	*pos = lens_get_pos(af);
	return 0;
}

static cmr_u8 if_lens_move_to(cmr_u16 pos, void *cookie)
{
	af_ctrl_t *af = cookie;
	AF_Timestamp timestamp;
	cmr_u32 sec, usec;

	af->cb_ops.af_get_system_time(af->caller, &sec, &usec);
	af->vcm_timestamp = (cmr_u64) sec *1000000000 + (cmr_u64) usec *1000;
	timestamp.type = AF_TIME_VCM;
	timestamp.time_stamp = af->vcm_timestamp;
	af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_SET_TIMESTAMP, &timestamp);
	lens_move_to(af, pos);
	return 0;
}

static cmr_u8 if_lens_wait_stop(void *cookie)
{
	UNUSED(cookie);
	return 0;
}

static cmr_u8 if_lock_partial_ae(cmr_u32 lock, void *cookie)
{
	af_ctrl_t *af = cookie;
	ISP_LOGV("%s, lock_num = %d", LOCK == lock ? "lock" : "unlock", af->ae_partial_lock_num);

	if (LOCK == ! !lock) {
		if (0 == af->ae_partial_lock_num) {
			af->cb_ops.lock_module(af->caller, AF_LOCKER_AE_CAF);
			af->ae_partial_lock_num++;
		}
	} else {
		if (af->ae_partial_lock_num) {
			af->cb_ops.unlock_module(af->caller, AF_LOCKER_AE_CAF);
			af->ae_partial_lock_num--;
		}
	}

	return 0;
}

static cmr_u8 if_lock_ae(e_LOCK lock, void *cookie)
{
	af_ctrl_t *af = cookie;
	ISP_LOGV("%s, lock_num = %d", LOCK == lock ? "lock" : "unlock", af->ae_lock_num);

	if (LOCK == lock) {
		if (0 == af->ae_lock_num) {
			af->cb_ops.lock_module(af->caller, AF_LOCKER_AE);
			af->ae_lock_num++;
		}
	} else {
		if (af->ae_lock_num) {
			af->cb_ops.unlock_module(af->caller, AF_LOCKER_AE);
			af->ae_lock_num--;
		}
	}

	return 0;
}

static cmr_u8 if_lock_awb(e_LOCK lock, void *cookie)
{
	af_ctrl_t *af = cookie;
	ISP_LOGV("%s, lock_num = %d", LOCK == lock ? "lock" : "unlock", af->awb_lock_num);

	if (LOCK == lock) {
		if (0 == af->awb_lock_num) {
			af->cb_ops.lock_module(af->caller, AF_LOCKER_AWB);
			af->awb_lock_num++;
		}
	} else {
		if (af->awb_lock_num) {
			af->cb_ops.unlock_module(af->caller, AF_LOCKER_AWB);
			af->awb_lock_num--;
		}
	}

	return 0;
}

static cmr_u8 if_lock_lsc(e_LOCK lock, void *cookie)
{
	af_ctrl_t *af = cookie;
	ISP_LOGV("%s, lock_num = %d", LOCK == lock ? "lock" : "unlock", af->lsc_lock_num);

	if (LOCK == lock) {
		if (0 == af->lsc_lock_num) {
			af->cb_ops.lock_module(af->caller, AF_LOCKER_LSC);
			af->lsc_lock_num++;
		}
	} else {
		if (af->lsc_lock_num) {
			af->cb_ops.unlock_module(af->caller, AF_LOCKER_LSC);
			af->lsc_lock_num--;
		}
	}

	return 0;
}

static cmr_u8 if_lock_nlm(e_LOCK lock, void *cookie)
{
	af_ctrl_t *af = cookie;
	ISP_LOGV("%s, lock_num = %d", LOCK == lock ? "lock" : "unlock", af->nlm_lock_num);

	if (LOCK == lock) {
		if (0 == af->nlm_lock_num) {
			af->cb_ops.lock_module(af->caller, AF_LOCKER_NLM);
			af->nlm_lock_num++;
		}
	} else {
		if (af->nlm_lock_num) {
			af->cb_ops.unlock_module(af->caller, AF_LOCKER_NLM);
			af->nlm_lock_num--;
		}
	}

	return 0;
}

static cmr_u8 if_get_sys_time(cmr_u64 * time, void *cookie)
{
	af_ctrl_t *af = (af_ctrl_t *) cookie;
	cmr_u32 sec, usec;

	af->cb_ops.af_get_system_time(af->caller, &sec, &usec);
	*time = (cmr_u64) sec *1000000000 + (cmr_u64) usec *1000;
	// *time = get_systemtime_ns();
	return 0;
}

static cmr_u8 if_sys_sleep_time(cmr_u16 sleep_time, void *cookie)
{
	UNUSED(cookie);
	int sleep_rtn = 0;
	// ISP_LOGV("vcm_timestamp %lld ms", (cmr_s64) af->vcm_timestamp);
	sleep_rtn = usleep(sleep_time * 1000);
	if (sleep_rtn)
		ISP_LOGE("error: no pause!");
	return 0;
}

static cmr_u8 if_get_ae_report(AE_Report * rpt, void *cookie)
{
	af_ctrl_t *af = (af_ctrl_t *) cookie;
	ae_info_t *ae = &af->ae;
	cmr_u32 line_time = ae->ae_report.line_time;
	cmr_u32 frame_len = ae->ae_report.frame_line;
	cmr_u32 dummy_line = ae->ae_report.cur_dummy;
	cmr_u32 exp_line = ae->ae_report.cur_exp_line;
	cmr_u32 frame_time;

	rpt->bAEisConverge = ae->ae_report.is_stab;
	rpt->AE_BV = ae->ae_report.bv;
	rpt->AE_EXP = (exp_line * line_time) / 10000;	// 0.1us -> ms
	rpt->AE_Gain = ae->ae_report.cur_again;
	rpt->AE_Pixel_Sum = af->Y_sum_normalize;

	frame_len = (frame_len > (exp_line + dummy_line)) ? frame_len : (exp_line + dummy_line);
	frame_time = frame_len * line_time;
	frame_time = frame_time > 0 ? frame_time : 1;
	rpt->cur_fps = (ae->ae_report.cur_fps) ? ae->ae_report.cur_fps : (1000000000 / MAX(1, (exp_line + dummy_line) * line_time));
	rpt->cur_lum = ae->ae_report.cur_lum;
	rpt->cur_index = ae->ae_report.cur_index;
	rpt->cur_ev = ae->ae_report.cur_ev;
	rpt->cur_iso = ae->ae_report.cur_iso;
	rpt->target_lum = ae->ae_report.target_lum;
	rpt->target_lum_ori = ae->ae_report.target_lum_ori;
	rpt->flag4idx = ae->ae_report.flag4idx;
	rpt->bisFlashOn = af->flash_on;
	rpt->near_stable = ae->ae_report.near_stab;
	ISP_LOGV("(near,full) = (%d,%d)", rpt->near_stable, rpt->bAEisConverge);
	return 0;
}

static cmr_u8 if_set_af_exif(const void *data, void *cookie)
{
	af_ctrl_t *af = cookie;
	UNUSED(data);
	property_get("persist.vendor.cam.isp.af.dump", af->AF_MODE, "none");

	if (0 == strcmp(af->AF_MODE, "on")) {
		FILE *fp = NULL;
		if (STATE_NORMAL_AF == af->state)
			fp = fopen("/data/vendor/cameraserver/saf_debug_info.jpg", "wb");
		else
			fp = fopen("/data/vendor/cameraserver/caf_debug_info.jpg", "wb");

		if (NULL == fp) {
			ISP_LOGE("dump af_debug_info failure");
			return 0;
		}
		fwrite("ISP_AF__", 1, strlen("ISP_AF__"), fp);
		fwrite(af->af_alg_cxt, 1, af->af_dump_info_len, fp);
		fclose(fp);
	}

	return 0;
}

static cmr_u8 if_get_otp(AF_OTP_Data * pAF_OTP, void *cookie)
{
	af_ctrl_t *af = cookie;

	if (af->otp_info.rdm_data.macro_cali > af->otp_info.rdm_data.infinite_cali) {
		pAF_OTP->bIsExist = (T_LENS_BY_OTP);
		pAF_OTP->INF = af->otp_info.rdm_data.infinite_cali;
		pAF_OTP->MACRO = af->otp_info.rdm_data.macro_cali;
		ISP_LOGI("get otp (infi,macro) = (%d,%d)", pAF_OTP->INF, pAF_OTP->MACRO);
	} else {
		ISP_LOGW("skip invalid otp (infi,macro) = (%d,%d)", af->otp_info.rdm_data.infinite_cali, af->otp_info.rdm_data.macro_cali);
	}

	return 0;
}

static cmr_u8 if_get_motor_pos(cmr_u16 * motor_pos, void *cookie)
{
	af_ctrl_t *af = cookie;
	if (NULL != motor_pos) {
		*motor_pos = lens_get_pos(af);
	}

	return 0;
}

static cmr_u8 if_set_motor_sacmode(void *cookie)
{
	af_ctrl_t *af = cookie;

	if (NULL != af->cb_ops.af_set_motor_bestmode)
		af->cb_ops.af_set_motor_bestmode(af->caller);

	return 0;
}

static cmr_u8 if_binfile_is_exist(cmr_u8 * bisExist, void *cookie)
{
	UNUSED(cookie);

	ISP_LOGV("Enter");
	*bisExist = 0;
	ISP_LOGV("Exit");
	return 0;
}

static cmr_u8 if_af_log(const char *format, ...)
{
	va_list arg;
	va_start(arg, format);
	vsnprintf(AFlog_buffer, 2048, format, arg);
	va_end(arg);
	ISP_LOGV("ISP_AFv1: %s", AFlog_buffer);

	return 0;
}

static cmr_u8 if_af_start_notify(eAF_MODE AF_mode, void *cookie)
{
	af_ctrl_t *af = cookie;

	cmr_u8 notify_type = 0;
	switch (AF_mode) {
	case SAF:
		notify_type = AF_FOCUS_SAF;
		break;
	case CAF:
		notify_type = AF_FOCUS_CAF;
		break;
	case FAF:
		notify_type = AF_FOCUS_FAF;
		break;
	case PDAF:
		notify_type = AF_FOCUS_PDAF;
		break;
	case TOF:
		notify_type = AF_FOCUS_TOF;
		break;

	default:
		notify_type = AF_FOCUS_CAF;
		break;
	}

	ISP_LOGI("notify_start: mode[%d], type[%d]!!", AF_mode, notify_type);

	if (AF_mode != SAF) {
		notify_start(af, notify_type);
	}
#ifdef Enable_mlog_AFtime
	af->AFtime.time_total = 0;
	af->AFtime.system_time0_1 = systemTime(CLOCK_MONOTONIC);
	if (AF_mode != SAF) {
		af->AFtime.AF_type = FOCUS_TYPE_STR(notify_type);
	} else {
		af->AFtime.AF_type = FOCUS_TYPE_STR(AF_FOCUS_SAF);
	}
#endif
	return 0;
}

static cmr_u8 if_af_end_notify(eAF_MODE AF_mode, cmr_u8 AF_Result, void *cookie)
{

	/*
	   UNUSED(AF_mode);
	   UNUSED(cookie);
	 */
	af_ctrl_t *af = cookie;
	cmr_u8 notify_type = 0;

	switch (AF_mode) {
	case SAF:
		notify_type = AF_FOCUS_SAF;
		break;
	case CAF:
		notify_type = AF_FOCUS_CAF;
		break;
	case FAF:
		notify_type = AF_FOCUS_FAF;
		break;
	case PDAF:
		notify_type = AF_FOCUS_PDAF;
		break;
	case TOF:
		notify_type = AF_FOCUS_TOF;
		break;

	default:
		notify_type = AF_FOCUS_CAF;
		break;
	}

	ISP_LOGI("notify_stop: mode[%d], type[%d], result[%d]!!!", AF_mode, notify_type, AF_Result);

	notify_stop(af, 1, notify_type);

	// debug only
	roi_info_t *r = &af->roi;
	cmr_u32 i;
	spaf_win_t afm_wins;
	afm_wins.win_num = r->num;
	for (i = 0; i < afm_wins.win_num; ++i) {
		afm_wins.win[i].sx = r->win[i].start_x;
		afm_wins.win[i].sy = r->win[i].start_y;
		afm_wins.win[i].ex = r->win[i].end_x;
		afm_wins.win[i].ey = r->win[i].end_y;
	}
	af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_SET_HW_WINS, &afm_wins);

	return 0;
}

static cmr_u8 if_clear_fd_stop_counter(cmr_u32 * FD_count, void *cookie)
{
	UNUSED(FD_count);
	UNUSED(cookie);

	return 0;
}

static cmr_u8 if_face_detection_get_data(spaf_face_info_t * FD, void *cookie)
{

	UNUSED(FD);
	UNUSED(cookie);

	af_ctrl_t *af = cookie;
	cmr_u8 i = 0;
	while (i < af->face_info.face_num && i < sizeof(FD->data) / sizeof(FD->data[0])) {
		FD->data[i].sx = af->face_info.face_info[i].sx;
		FD->data[i].sy = af->face_info.face_info[i].sy;
		FD->data[i].ex = af->face_info.face_info[i].ex;
		FD->data[i].ey = af->face_info.face_info[i].ey;
		FD->data[i].yaw_angle = af->face_info.face_info[i].pose;
		FD->data[i].roll_angle = af->face_info.face_info[i].angle;
		FD->data[i].score = 0;
		FD->num++;
		i++;
	}

	return 0;
}

static cmr_u8 if_phase_detection_get_data(pd_algo_result_t * pd_result, void *cookie)
{
	af_ctrl_t *af = cookie;

	memcpy(pd_result, &(af->pd), sizeof(pd_algo_result_t));
	// pd_result->pd_roi_dcc = 17;

	ISP_LOGV("AF_Lib data1 Fr[%d]En[%d]PD[%f]CONF[%d]DCC5[%d] ", pd_result->effective_frmid, pd_result->pd_enable,
		 pd_result->pd_value[4], pd_result->confidence[4], pd_result->pd_roi_dcc[5]);

	return 0;
}

static cmr_u8 if_motion_sensor_get_data(motion_sensor_result_t * ms_result, void *cookie)
{
	af_ctrl_t *af = cookie;
	cmr_u8 gsensor_x = 0, gsensor_y = 0, gsensor_z = 0;

	if (NULL == ms_result) {
		return 1;
	}

	ms_result->g_sensor_queue[SENSOR_X_AXIS][ms_result->sensor_g_queue_cnt] = af->gsensor_info.vertical_up;
	ms_result->g_sensor_queue[SENSOR_Y_AXIS][ms_result->sensor_g_queue_cnt] = af->gsensor_info.vertical_down;
	ms_result->g_sensor_queue[SENSOR_Z_AXIS][ms_result->sensor_g_queue_cnt] = af->gsensor_info.horizontal;
	ms_result->timestamp = af->gsensor_info.timestamp;

	gsensor_x = (ms_result->sensor_g_posture & 0x00ff0000) >> 16;
	gsensor_y = (ms_result->sensor_g_posture & 0x0000ff00) >> 8;
	gsensor_z = (ms_result->sensor_g_posture & 0x000000ff);
	if (0x10 == gsensor_x) {
		af->g_orientation = AF_G_DEGREE0;
	} else if (0x01 == gsensor_x) {
		af->g_orientation = AF_G_DEGREE2;
	} else if (0xff == gsensor_x && 0x10 == gsensor_y) {
		af->g_orientation = AF_G_DEGREE1;
	} else if (0xff == gsensor_x && 0x01 == gsensor_y) {
		af->g_orientation = AF_G_DEGREE3;
	} else {
		af->g_orientation = AF_G_NONE;
	}
	ISP_LOGV("ISP_AFv1: %x, %x", gsensor_x, gsensor_y);

	return 0;
}

static cmr_u8 if_set_bokeh_vcm_info(bokeh_motor_info * range, void *cookie)
{
	af_ctrl_t *af = cookie;
	af->realboekh_range.limited_infi = range->limited_infi;
	af->realboekh_range.limited_macro = range->limited_macro;
	af->realboekh_range.total_seg = range->total_seg;
	memcpy(&af->realboekh_range.vcm_dac[0], &range->vcm_dac[0], 20 * sizeof(cmr_u16));
	ISP_LOGI("range(%u %u), %u,%u,%u,%u,%u,%u,%u", af->realboekh_range.limited_infi, af->realboekh_range.limited_macro, range->vcm_dac[0], range->vcm_dac[1],
		 range->vcm_dac[2], range->vcm_dac[3], range->vcm_dac[4], range->vcm_dac[5], range->vcm_dac[6]);
	return 0;
}

// SharkLE Only ++
// helper function
// copy the original lens_move_to()
static void lens_move_to_sharkle(af_ctrl_t * af, cmr_u16 pos)
{
	// ISP_LOGD(" lens_move_to_sharkle, pos= %d",pos);

	cmr_u16 last_pos = 0;

	if (NULL == af->cb_ops.af_set_next_vcm_pos) {
		ISP_LOGE("af->af_set_next_vcm_pos null error");
		return;
	}

	last_pos = lens_get_pos(af);
	ISP_LOGD(" lens_move_to_sharkle, last_pos= %d", last_pos);

	if (last_pos != pos) {
		af->cb_ops.af_set_next_vcm_pos(af->caller, pos);
		af->lens.pos = pos;
	} else {
		ISP_LOGV("pos %d was set last time", pos);
	}
}

// original driver new support
static cmr_u8 if_af_set_pulse_line(cmr_u32 line, void *cookie)
{

	af_ctrl_t *af = cookie;
	ISP_LOGD(" if_af_set_pulse_line = %d", line);

	if (NULL != af->cb_ops.af_set_pulse_line)
		af->cb_ops.af_set_pulse_line(af->caller, line);

	return 0;
}

// copy the original if_lens_move_to
static cmr_u8 if_af_set_next_vcm_pos(cmr_u32 pos, void *cookie)
{
	ISP_LOGD(" if_af_set_next_vcm_pos, pos= %d", pos);

	af_ctrl_t *af = cookie;
	AF_Timestamp timestamp;
	af->vcm_timestamp = get_systemtime_ns();
	timestamp.type = AF_TIME_VCM;
	timestamp.time_stamp = af->vcm_timestamp;
	af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_SET_TIMESTAMP, &timestamp);
	lens_move_to_sharkle(af, pos);
	return 0;

	/*
	   af_ctrl_t *af = cookie;
	   if (NULL != af->af_set_next_vcm_pos)
	   af->af_set_next_vcm_pos(af->caller, pos);

	   return 0;
	 */
}

static cmr_u8 if_af_set_pulse_log(cmr_u32 flag, void *cookie)
{

	af_ctrl_t *af = cookie;
	ISP_LOGD(" if_af_set_pulse_log = %d", flag);

	if (NULL != af->cb_ops.af_set_pulse_log)
		af->cb_ops.af_set_pulse_log(af->caller, flag);

	return 0;
}

static cmr_u8 if_af_set_clear_next_vcm_pos(void *cookie)
{

	af_ctrl_t *af = cookie;

	if (NULL != af->cb_ops.af_set_clear_next_vcm_pos)
		af->cb_ops.af_set_clear_next_vcm_pos(af->caller);

	return 0;
}

// SharkLE Only --

//[TOF_+++]
static cmr_u8 if_get_tof_data(tof_measure_data_t * tof_result, void *cookie)
{
	//af_ctrl_t *af = cookie;
	UNUSED(cookie);
	//char value[PROPERTY_VALUE_MAX] = { '\0' };

#ifdef SHARK_L3_1

	VL53L0_RangingMeasurementData_t range_datas;

	vl53l0_getdata(&range_datas);

	tof_result->data.RangeDMaxMilliMeter = range_datas.RangeDMaxMilliMeter;
	tof_result->data.RangeMilliMeter = range_datas.RangeMilliMeter;
	tof_result->data.RangeStatus = range_datas.RangeStatus;

#else
	af_ctrl_t *af = cookie;
	memcpy(tof_result, &(af->tof), sizeof(tof_measure_data_t));
#endif

	return 0;
}

//[TOF_---]
// trigger stuffs
#define LOAD_SYMBOL(handle, sym, name) \
{sym=dlsym(handle, name); if(NULL==sym) {ISP_LOGE("dlsym fail: %s", name); return -1;}}

static cmr_s32 load_trigger_symbols(af_ctrl_t * af)
{
	LOAD_SYMBOL(af->trig_lib, af->trig_ops.init, "caf_trigger_init");
	LOAD_SYMBOL(af->trig_lib, af->trig_ops.deinit, "caf_trigger_deinit");
	LOAD_SYMBOL(af->trig_lib, af->trig_ops.calc, "caf_trigger_calculation");
	LOAD_SYMBOL(af->trig_lib, af->trig_ops.ioctrl, "caf_trigger_ioctrl");
	return 0;
}

static cmr_s32 load_trigger_lib(af_ctrl_t * af, const char *name)
{
	af->trig_lib = dlopen(name, RTLD_NOW);

	if (NULL == af->trig_lib) {
		ISP_LOGE("fail to load af trigger lib%s", name);
		return -1;
	}

	if (0 != load_trigger_symbols(af)) {
		dlclose(af->trig_lib);
		af->trig_lib = NULL;
		return -1;
	}

	return 0;
}

static cmr_s32 unload_trigger_lib(af_ctrl_t * af)
{
	if (af->trig_lib) {
		dlclose(af->trig_lib);
		af->trig_lib = NULL;
	}
	return 0;
}

static cmr_u8 if_aft_binfile_is_exist(cmr_u8 * is_exist, void *cookie)
{

	af_ctrl_t *af = cookie;
	char *aft_tuning_path = "/data/vendor/cameraserver/aft_tuning.bin";
	FILE *fp = NULL;

	if (0 == access(aft_tuning_path, R_OK)) {	// read request successs
		cmr_s32 len = 0;

		fp = fopen(aft_tuning_path, "rb");
		if (NULL == fp) {
			*is_exist = 0;
			return 0;
		}

		fseek(fp, 0, SEEK_END);
		len = ftell(fp);
		if (len < 0 || (cmr_u32) len != af->trig_ops.init_out.tuning_param_len) {
			ISP_LOGW("aft_tuning.bin len dismatch with aft_alg len %d", af->trig_ops.init_out.tuning_param_len);
			fclose(fp);
			*is_exist = 0;
			return 0;
		}

		fclose(fp);
		*is_exist = 1;
	} else {
		*is_exist = 0;
	}
	return 0;
}

static cmr_u8 if_is_aft_mlog(cmr_u32 * is_save, void *cookie)
{
	UNUSED(cookie);
	char value[PROPERTY_VALUE_MAX] = { '\0' };

	property_get(AF_SAVE_MLOG_STR, value, "no");

	if (!strcmp(value, "save")) {
		*is_save = 1;
	}
	ISP_LOGV("is_save %d", *is_save);
	return 0;
}

static cmr_u8 if_aft_log(cmr_u32 log_level, const char *format, ...)
{
	va_list arg;
	va_start(arg, format);
	vsnprintf(AFlog_buffer, 2048, format, arg);
	va_end(arg);
	switch (log_level) {
	case AFT_LOG_VERBOSE:
		ALOGV("%s", AFlog_buffer);
		break;
	case AFT_LOG_DEBUG:
		ALOGD("%s", AFlog_buffer);
		break;
	case AFT_LOG_INFO:
		ALOGI("%s", AFlog_buffer);
		break;
	case AFT_LOG_WARN:
		ALOGW("%s", AFlog_buffer);
		break;
	case AFT_LOG_ERROR:
		ALOGE("%s", AFlog_buffer);
		break;
	default:
		ISP_LOGV("default log level not support");
		break;
	}

	return 0;
}

static cmr_u8 if_get_saf_extra_data(saf_extra_data_t * safex, void *cookie)
{
	af_ctrl_t *af = cookie;
	cmr_u32 pd_workable = 0;

	af->trig_ops.ioctrl(af->trig_ops.handle, AFT_CMD_GET_PD_WORKABLE, &pd_workable, NULL);
	safex->pd_enable = af->pd.pd_enable;
	safex->pd_workable = pd_workable;
	ISP_LOGD("enable %d, workable %d", safex->pd_enable, safex->pd_workable);
	return 0;
}

static cmr_s32 load_af_symbols(af_ctrl_t * af)
{
	LOAD_SYMBOL(af->af_lib, af->af_ops.init, "af_init");
	LOAD_SYMBOL(af->af_lib, af->af_ops.deinit, "af_deinit");
	LOAD_SYMBOL(af->af_lib, af->af_ops.calc, "af_process");
	LOAD_SYMBOL(af->af_lib, af->af_ops.ioctrl, "af_ioctrl");
	return 0;
}

static cmr_s32 load_af_lib(af_ctrl_t * af, const char *name)
{
	af->af_lib = dlopen(name, RTLD_NOW);

	if (NULL == af->af_lib) {
		cmr_u32 i = 0;
		while (i < sizeof(libafv1_path) / sizeof(libafv1_path[0]) && NULL == af->af_lib) {
			af->af_lib = dlopen(libafv1_path[i], RTLD_NOW);
			i++;
		}
		if (NULL == af->af_lib) {
			ISP_LOGE("fail to load af lib%s", name);
			return -1;
		}
	}

	if (0 != load_af_symbols(af)) {
		dlclose(af->af_lib);
		af->af_lib = NULL;
		return -1;
	}

	return 0;
}

static cmr_s32 unload_af_lib(af_ctrl_t * af)
{
	if (af->af_lib) {
		dlclose(af->af_lib);
		af->af_lib = NULL;
	}
	return 0;
}

/* initialization */
static void *af_lib_init(af_ctrl_t * af)
{
	af_init_in af_in;
	af_init_out af_out;
	void *alg_cxt = NULL;

	memset((void *)&af_in, 0, sizeof(af_init_in));
	memset((void *)&af_out, 0, sizeof(af_init_out));

	af_in.AF_Ops.cookie = af;
	af_in.AF_Ops.statistics_wait_cal_done = if_statistics_wait_cal_done;
	af_in.AF_Ops.statistics_get_data = if_statistics_get_data;
	af_in.AF_Ops.statistics_set_data = if_statistics_set_data;
	af_in.AF_Ops.lens_get_pos = if_lens_get_pos;
	af_in.AF_Ops.lens_move_to = if_lens_move_to;
	af_in.AF_Ops.lens_wait_stop = if_lens_wait_stop;
	af_in.AF_Ops.lock_ae = if_lock_ae;
	af_in.AF_Ops.lock_awb = if_lock_awb;
	af_in.AF_Ops.lock_lsc = if_lock_lsc;
	af_in.AF_Ops.get_sys_time = if_get_sys_time;
	af_in.AF_Ops.sys_sleep_time = if_sys_sleep_time;
	af_in.AF_Ops.get_ae_report = if_get_ae_report;
	af_in.AF_Ops.set_af_exif = if_set_af_exif;
	af_in.AF_Ops.get_otp_data = if_get_otp;
	af_in.AF_Ops.get_motor_pos = if_get_motor_pos;
	af_in.AF_Ops.set_motor_sacmode = if_set_motor_sacmode;
	af_in.AF_Ops.binfile_is_exist = if_binfile_is_exist;
	af_in.AF_Ops.af_log = if_af_log;
	af_in.AF_Ops.af_start_notify = if_af_start_notify;
	af_in.AF_Ops.af_end_notify = if_af_end_notify;
	af_in.AF_Ops.phase_detection_get_data = if_phase_detection_get_data;
	af_in.AF_Ops.motion_sensor_get_data = if_motion_sensor_get_data;
	af_in.AF_Ops.set_wins = if_set_wins;
	af_in.AF_Ops.get_win_info = if_get_win_info;
	af_in.AF_Ops.lock_ae_partial = if_lock_partial_ae;
	af_in.AF_Ops.face_detection_get_data = if_face_detection_get_data;
	af_in.AF_Ops.clear_fd_stop_counter = if_clear_fd_stop_counter;
	af_in.AF_Ops.set_bokeh_vcm_info = if_set_bokeh_vcm_info;
	// SharkLE Only ++
	af_in.AF_Ops.set_pulse_line = if_af_set_pulse_line;
	af_in.AF_Ops.set_next_vcm_pos = if_af_set_next_vcm_pos;
	af_in.AF_Ops.set_pulse_log = if_af_set_pulse_log;
	af_in.AF_Ops.set_clear_next_vcm_pos = if_af_set_clear_next_vcm_pos;
	// SharkLE Only --
	af_in.AF_Ops.get_tof_data = if_get_tof_data;
	af_in.AF_Ops.get_saf_extra_data = if_get_saf_extra_data;
	af_in.AF_Ops.get_sub_wins_ysum = if_get_sub_wins_ysum;

	af_in.tuning.data = af->aftuning_data;
	af_in.tuning.data_len = af->aftuning_data_len;
	if (af->pdaftuning_data != NULL) {
		af_in.tuning.pd_data = af->pdaftuning_data;
		af_in.tuning.pd_data_len = af->pdaftuning_data_len;
	} else {
		ISP_LOGI("PDAF Tuning NULL!");
	}
	if (af->toftuning_data != NULL) {
		af_in.tuning.tof_data = af->toftuning_data;
		af_in.tuning.tof_data_len = af->toftuning_data_len;
	} else {
		ISP_LOGI("TOF Tuning NULL!");
	}

	af_in.sys_version = AF_SYS_VERSION;
	af_in.camera_id = af->camera_id;
	af_in.pdaf_support = af->pdaf_support;
	if (0 != load_af_lib(af, AF_LIB))
		return NULL;

	alg_cxt = af->af_ops.init(&af_in, &af_out);
	if (NULL == alg_cxt) {
		unload_af_lib(af);
	}
	af->af_dump_info_len = af_out.af_dump_len;

	return alg_cxt;
}

static cmr_s32 trigger_init(af_ctrl_t * af, const char *lib_name)
{
	struct aft_init_in aft_in;
	struct aft_init_out aft_out;
	char value[PROPERTY_VALUE_MAX] = { '\0' };

	if (0 != load_trigger_lib(af, lib_name))
		return -1;

	if (NULL == af->afttuning_data || 0 == af->afttuning_data_len) {
		ISP_LOGW("aft tuning param error ");
		aft_in.tuning.data_len = 0;
		aft_in.tuning.data = NULL;
	} else {
		ISP_LOGI("aft tuning param ok ");
		aft_in.tuning.data_len = af->afttuning_data_len;
		aft_in.tuning.data = af->afttuning_data;

		property_get(AF_SAVE_MLOG_STR, value, "no");
		if (!strcmp(value, "save")) {
			FILE *fp = NULL;
			fp = fopen("/data/vendor/cameraserver/aft_tuning_params.bin", "wb");
			fwrite(aft_in.tuning.data, 1, aft_in.tuning.data_len, fp);
			fclose(fp);
			ISP_LOGV("aft tuning size = %d", aft_in.tuning.data_len);
		}
	}
	aft_in.tuning.data_len = af->afttuning_data_len;
	aft_in.tuning.data = af->afttuning_data;
	aft_in.aft_ops.aft_cookie = af;
	aft_in.aft_ops.get_sys_time = if_get_sys_time;
	aft_in.aft_ops.binfile_is_exist = if_aft_binfile_is_exist;
	aft_in.aft_ops.is_aft_mlog = if_is_aft_mlog;
	aft_in.aft_ops.aft_log = if_aft_log;
	if (AF_ALG_BLUR_PORTRAIT == af->is_multi_mode) {
		af->is_multi_mode = AF_ALG_DUAL_C_C;
	}
	aft_in.is_multi_mode = af->is_multi_mode;

	af->trig_ops.init(&aft_in, &aft_out, &af->trig_ops.handle);
	ISP_LOGV("af->trig_ops.handle = %p", af->trig_ops.handle);
	af->trig_ops.ioctrl(af->trig_ops.handle, AFT_CMD_GET_AE_SKIP_INFO, &af->trig_ops.ae_skip_info, NULL);
	af->trig_ops.init_out.tuning_param_len = aft_out.tuning_param_len;
	af->trig_ops.init_out.aft_dump_ptr = aft_out.aft_dump_ptr;
	af->trig_ops.init_out.aft_dump_len = aft_out.aft_dump_len;
	return 0;
}

static cmr_s32 trigger_set_mode(af_ctrl_t * af, enum aft_mode mode)
{
	af->trig_ops.ioctrl(af->trig_ops.handle, AFT_CMD_SET_AF_MODE, &mode, NULL);
	return 0;
}

static cmr_s32 trigger_set_flash_status(af_ctrl_t * af, enum af_flash_status flash_status)
{
	af->trig_ops.ioctrl(af->trig_ops.handle, AFT_CMD_SET_FLASH_STATUS, &flash_status, NULL);
	return 0;
}

static cmr_s32 trigger_notice_force(af_ctrl_t * af)
{
	af->trig_ops.ioctrl(af->trig_ops.handle, AFT_CMD_SET_FORCE_TRIGGER, NULL, NULL);
	return 0;
}

static cmr_s32 trigger_start(af_ctrl_t * af)
{
	af->trig_ops.ioctrl(af->trig_ops.handle, AFT_CMD_SET_CAF_RESET, NULL, NULL);
	return 0;
}

static cmr_s32 trigger_stop(af_ctrl_t * af)
{
	af->trig_ops.ioctrl(af->trig_ops.handle, AFT_CMD_SET_CAF_STOP, NULL, NULL);
	return 0;
}

static cmr_s32 trigger_calc(af_ctrl_t * af, struct aft_proc_calc_param *prm, struct aft_proc_result *res)
{
	af->trig_ops.calc(af->trig_ops.handle, prm, res);
	return 0;
}

static cmr_s32 trigger_deinit(af_ctrl_t * af)
{
	af->trig_ops.deinit(af->trig_ops.handle);
	unload_trigger_lib(af);
	return 0;
}

static cmr_s32 otaf_update_af_state(cmr_handle handle, cmr_u32 otaf_on);

// af process functions
static cmr_u32 af_get_defocus_param(char *string, defocus_param_t * defocus, cmr_u32 * times)
{
	char *token = NULL;
	cmr_u32 scan_from = 0;
	cmr_u32 scan_to = 0;
	cmr_u32 per_steps = 0;
	cmr_u32 ret = 0;

	token = strtok(string, ":");
	if (token != NULL) {
		scan_from = atoi(token);
		token = strtok(NULL, ":");
	}

	if (token != NULL) {
		scan_to = atoi(token);
		token = strtok(NULL, ":");
	}

	if (token != NULL) {
		per_steps = atoi(token);
		token = strtok(NULL, ":");
	}

	if (token != NULL) {
		*times = atoi(token);
	}
	defocus->scan_from = (scan_from > 0 && scan_from < 2047) ? scan_from : 0;
	defocus->scan_to = (scan_to > 0 && scan_to < 2047) ? scan_to : 0;
	defocus->per_steps = (per_steps > 0 && per_steps < 200) ? per_steps : 0;
	ISP_LOGI("scan_from %d, scan_to %d, per_steps %d, times %d", defocus->scan_from, defocus->scan_to, defocus->per_steps, *times);

	return ret;
}

static void saf_start(af_ctrl_t * af, struct af_trig_info *win)
{
	spaf_roi_t af_roi;
	AF_Trigger_Data aft_in;
	cmr_u32 i;

	af->algo_mode = SAF;
	memset(&af_roi, 0, sizeof(spaf_roi_t));
	af_roi.af_mode = af->algo_mode;
	if (NULL == win || 0 == win->win_num) {
		af_roi.win_num = 0;
		ISP_LOGI("win is NULL or win_num 0");
	} else {
		af_roi.win_num = win->win_num;
		if (af_roi.win_num > SPAF_MAX_ROI_NUM) {
			af_roi.win_num = SPAF_MAX_ROI_NUM;
		}
		for (i = 0; i < af_roi.win_num; ++i) {
			af_roi.saf_roi[i].sx = win->win_pos[i].sx;
			af_roi.saf_roi[i].sy = win->win_pos[i].sy;
			af_roi.saf_roi[i].ex = win->win_pos[i].ex;
			af_roi.saf_roi[i].ey = win->win_pos[i].ey;
		}
	}
	af_roi.multi_mode = af->is_multi_mode;
	af_roi.zoom_ratio = af->zoom_ratio;
	af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_SET_ROI, &af_roi);

	memset(&aft_in, 0, sizeof(AF_Trigger_Data));
	aft_in.AFT_mode = af->algo_mode;
	aft_in.bisTrigger = AF_TRIGGER;
	aft_in.AF_Trigger_Type = (1 == af->defocus) ? (DEFOCUS) : (RF_NORMAL);
	af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_SET_TRIGGER, &aft_in);
	do_start_af(af);
	af->vcm_stable = 0;
}

static void faf_start(af_ctrl_t * af, struct af_adpt_roi_info *win)
{
	char value[PROPERTY_VALUE_MAX] = { '\0' };
	AF_Trigger_Data aft_in;
	cmr_u32 times = 0;
	spaf_roi_t af_roi;
	cmr_u32 i;

	af->algo_mode = FAF;
	memset(&af_roi, 0, sizeof(spaf_roi_t));
	af_roi.af_mode = af->algo_mode;
	if (NULL == win || 0 == win->win_num) {
		af_roi.win_num = 0;
		ISP_LOGI("win is NULL or win_num 0");
	} else {
		af_roi.win_num = win->win_num;
		if (af_roi.win_num > SPAF_MAX_ROI_NUM) {
			af_roi.win_num = SPAF_MAX_ROI_NUM;
		}
		for (i = 0; i < af_roi.win_num; ++i) {
			af_roi.face_roi[i].sx = win->face[i].sx;
			af_roi.face_roi[i].sy = win->face[i].sy;
			af_roi.face_roi[i].ex = win->face[i].ex;
			af_roi.face_roi[i].ey = win->face[i].ey;
			af_roi.face_roi[i].yaw_angle = win->face[i].yaw_angle;
			af_roi.face_roi[i].roll_angle = win->face[i].roll_angle;
			af_roi.face_roi[i].score = win->face[i].score;
		}
	}
	af_roi.multi_mode = af->is_multi_mode;
	af_roi.zoom_ratio = af->zoom_ratio;
	af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_SET_ROI, &af_roi);

	memset(&aft_in, 0, sizeof(AF_Trigger_Data));
	aft_in.AFT_mode = af->algo_mode;
	aft_in.bisTrigger = AF_TRIGGER;
	property_get("persist.vendor.cam.isp.faf.defocus", value, "0");
	if (atoi(value) == 0) {
		aft_in.AF_Trigger_Type = RF_NORMAL;
	} else {
		af_get_defocus_param(value, &aft_in.defocus_param, &times);
		if (0 == times) {
			aft_in.AF_Trigger_Type = DEFOCUS;
		} else {
			if (0 == af->trigger_counter % times) {
				af->trigger_counter = 0;
				aft_in.AF_Trigger_Type = DEFOCUS;
			} else {
				aft_in.AF_Trigger_Type = RF_NORMAL;
			}
			ISP_LOGI("aft_in.AF_Trigger_Type %d, af->trigger_counter %d", aft_in.AF_Trigger_Type, af->trigger_counter);
			af->trigger_counter++;
		}
	}
	af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_SET_TRIGGER, &aft_in);
	do_start_af(af);
	af->vcm_stable = 0;
}

static void caf_start(af_ctrl_t * af, struct aft_proc_result *p_aft_result)
{
	char value[PROPERTY_VALUE_MAX] = { '\0' };
	AF_Trigger_Data aft_in;
	cmr_u32 times = 0;

	property_get("persist.vendor.cam.caf.enable", value, "1");
	if (atoi(value) != 1)
		return;

	af->algo_mode = STATE_CAF == af->state ? CAF : VAF;
	af_set_default_roi(af, af->algo_mode);

	memset(&aft_in, 0, sizeof(AF_Trigger_Data));
	aft_in.AFT_mode = af->algo_mode;
	aft_in.bisTrigger = AF_TRIGGER;
	aft_in.trigger_source = p_aft_result->is_caf_trig;
	property_get("persist.vendor.cam.isp.caf.defocus", value, "0");
	if (atoi(value) == 0) {
		aft_in.AF_Trigger_Type = (p_aft_result->is_need_rough_search) ? (RF_NORMAL) : (RF_FAST);
	} else {
		af_get_defocus_param(value, &aft_in.defocus_param, &times);
		if (0 == times) {
			aft_in.AF_Trigger_Type = DEFOCUS;
		} else {
			if (0 == af->trigger_counter % times) {
				af->trigger_counter = 0;
				aft_in.AF_Trigger_Type = DEFOCUS;
			} else {
				aft_in.AF_Trigger_Type = RF_NORMAL;
			}
			ISP_LOGI("aft_in.AF_Trigger_Type %d, af->trigger_counter %d", aft_in.AF_Trigger_Type, af->trigger_counter);
			af->trigger_counter++;
		}
	}
	af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_SET_TRIGGER, &aft_in);
	do_start_af(af);
	af->vcm_stable = 0;
}

//[TOF_+++]
static void tof_start(af_ctrl_t * af, e_AF_TRIGGER type, struct aft_proc_result *p_aft_result)
{
	char value[PROPERTY_VALUE_MAX] = { '\0' };
	AF_Trigger_Data aft_in;

	property_get("persist.vendor.cam.tof.enable", value, "1");
	if (atoi(value) != 1)
		return;

	af->algo_mode = TOF;

	memset(&aft_in, 0, sizeof(AF_Trigger_Data));
	aft_in.bisTrigger = type;
	//AF_Trigger_Type;
	aft_in.AFT_mode = af->algo_mode;
	//re_trigger;
	aft_in.trigger_source = p_aft_result->is_caf_trig;
	ISP_LOGI("tof current %d mode , %d", aft_in.AFT_mode, aft_in.trigger_source);

	if (aft_in.bisTrigger == AF_TRIGGER) {
		af_set_default_roi(af, af->algo_mode);

		af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_SET_TRIGGER, &aft_in);
		do_start_af(af);
	} else {
		af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_SET_TRIGGER, &aft_in);
	}

	af->vcm_stable = 0;
}

//[TOF_---]

static void pd_start(af_ctrl_t * af, e_AF_TRIGGER type, struct aft_proc_result *p_aft_result)
{
	char value[PROPERTY_VALUE_MAX] = { '\0' };
	AF_Trigger_Data aft_in;

	property_get("persist.vendor.cam.pd.enable", value, "1");
	if (atoi(value) != 1)
		return;

	af->algo_mode = PDAF;

	memset(&aft_in, 0, sizeof(AF_Trigger_Data));
	aft_in.bisTrigger = type;
	//AF_Trigger_Type;
	aft_in.AFT_mode = af->algo_mode;
	//re_trigger;
	aft_in.trigger_source = p_aft_result->is_caf_trig;
	ISP_LOGI("tof current %d mode , %d", aft_in.AFT_mode, aft_in.trigger_source);

	if (aft_in.bisTrigger == AF_TRIGGER) {
		af_set_default_roi(af, af->algo_mode);

		af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_SET_TRIGGER, &aft_in);
		do_start_af(af);
	} else {
		af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_SET_TRIGGER, &aft_in);
	}

	af->vcm_stable = 0;
}

static void otaf_start(af_ctrl_t * af)
{
	AF_Trigger_Data aft_in;

	af->algo_mode = OTAF;
	memset(&aft_in, 0, sizeof(AF_Trigger_Data));
	aft_in.AFT_mode = af->algo_mode;
	aft_in.bisTrigger = AF_TRIGGER;
	aft_in.AF_Trigger_Type = (1 == af->defocus) ? (DEFOCUS) : (RF_NORMAL);
	af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_SET_TRIGGER, &aft_in);
	af->focus_state = AF_SEARCHING;
	af->af_ops.calc(af->af_alg_cxt);	// fix trigger invalid issue, ot coord was set between set af mode and trigger.
}

static void af_process_frame(af_ctrl_t * af)
{
	cmr_u32 alg_mode;
	AF_Result af_result;

	af->af_ops.calc(af->af_alg_cxt);
	af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_GET_ALG_MODE, &alg_mode);
	if (Wait_Trigger == alg_mode) {
		af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_GET_RESULT, &af_result);
		ISP_LOGI("result = %d mode = %d ", af_result.AF_Result, af_result.af_mode);
		af->focus_state = AF_IDLE;
		trigger_start(af);
		if (STATE_FAF == af->state) {
			ISP_LOGI("pre_state %s", STATE_STRING(af->pre_state));
			af->state = af->pre_state;
		}
	}
}

static void af_stop_search(af_ctrl_t * af)
{
	cmr_u32 force_stop;

	ISP_LOGI("focus_state = %s", FOCUS_STATE_STR(af->focus_state));
	force_stop = AFV1_TRUE;
	af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_SET_CANCEL, &force_stop);	//modifiy for force stop to SAF/Flow control
	af->focus_state = AF_STOPPED;
	af->af_ops.calc(af->af_alg_cxt);

	if (STATE_FAF == af->state) {
		ISP_LOGI("pre_state %s", STATE_STRING(af->pre_state));
		af->state = af->pre_state;
	}
}

static void caf_monitor_trigger(af_ctrl_t * af, struct aft_proc_calc_param *prm, struct aft_proc_result *result)
{
	cmr_u32 force_stop = 0;

	if (AF_SEARCHING != af->focus_state) {
		if (result->is_caf_trig) {
			ISP_LOGI("lib trigger af %d", result->is_caf_trig);
			if (AFT_TRIG_FD == result->is_caf_trig) {
				af->win.win_num = 1;
				af->win.face[0].sx = prm->fd_info.face_info[0].sx;
				af->win.face[0].sy = prm->fd_info.face_info[0].sy;
				af->win.face[0].ex = prm->fd_info.face_info[0].ex;
				af->win.face[0].ey = prm->fd_info.face_info[0].ey;
				af->win.face[0].yaw_angle = prm->fd_info.face_info[0].yaw_angle;
				af->win.face[0].roll_angle = prm->fd_info.face_info[0].roll_angle;
				af->win.face[0].score = prm->fd_info.face_info[0].score;
				ISP_LOGI("face win num %d, x:%d y:%d e_x:%d e_y:%d, roll_angle %d", af->win.win_num, af->win.face[0].sx, af->win.face[0].sy,
					 af->win.face[0].ex, af->win.face[0].ey, af->win.face[0].roll_angle);
				af->roll_angle = af->win.face[0].roll_angle;
				if (af->roll_angle >= -180 && af->roll_angle <= 180) {
					if (af->roll_angle >= -45 && af->roll_angle <= 45) {
						af->f_orientation = FACE_UP;
					} else if ((af->roll_angle >= -180 && af->roll_angle <= -135) || (af->roll_angle >= 135 && af->roll_angle <= 180)) {
						af->f_orientation = FACE_DOWN;
					} else if (af->roll_angle > -135 && af->roll_angle < -45) {
						af->f_orientation = FACE_LEFT;
					} else if (af->roll_angle > 45 && af->roll_angle < 135) {
						af->f_orientation = FACE_RIGHT;
					}
				} else {
					af->f_orientation = FACE_NONE;
				}
				ISP_LOGI("af->f_orientation=%d", af->f_orientation);
				af->pre_state = af->state;
				af->state = STATE_FAF;
				faf_start(af, &af->win);
			} else if (AFT_TRIG_TOF == result->is_caf_trig /*&& af->tof.data.RangeStatus == 0 */ ) {	//[TOF_+++]
				//ISP_LOGV("ddd flag:%d. dis:%d, maxdis:%d, status:%d ", af->tof.tof_trigger_flag, af->tof.last_distance, af->tof.last_MAXdistance, af->tof.last_status );
				tof_start(af, AF_TRIGGER, result);	//[TOF_---]
			} else if (AFT_TRIG_PD == result->is_caf_trig) {
				pd_start(af, AF_TRIGGER, result);
			} else {
				caf_start(af, result);
			}
			af->focus_state = AF_SEARCHING;
		} else if (result->is_cancel_caf) {
			cmr_u32 alg_mode;
			af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_GET_ALG_MODE, &alg_mode);
			ISP_LOGW("cancel af while not searching AF_mode = %d", alg_mode);
		}
	} else {
		if (AFT_CANC_FD == result->is_cancel_caf || AFT_CANC_CB == result->is_cancel_caf || AFT_CANC_FD_GONE == result->is_cancel_caf) {
			ISP_LOGI("focus_state = %s, is_cancel_caf %d", FOCUS_STATE_STR(af->focus_state), result->is_cancel_caf);
			force_stop = AFV1_TRUE;
			af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_SET_CANCEL, &force_stop);	//modifiy for force stop to SAF/Flow control
			af->focus_state = AF_STOPPED_INNER;
			af->af_ops.calc(af->af_alg_cxt);

			if (STATE_FAF == af->state) {
				ISP_LOGI("pre_state %s", STATE_STRING(af->pre_state));
				af->state = af->pre_state;
			}
		}

		if (AFT_TRIG_FD == result->is_caf_trig || AFT_TRIG_CB == result->is_caf_trig) {
			ISP_LOGI("trigger cb fd while searching x, cancel_af %d, trigger_af %d", result->is_cancel_caf, result->is_caf_trig);
		} else if (AFT_TRIG_PD == result->is_caf_trig || AFT_TRIG_TOF == result->is_caf_trig) {
			ISP_LOGV("trigger pd tof while searching, cancel_af %d, trigger_af %d", result->is_cancel_caf, result->is_caf_trig);
		}

		if (AFT_TRIG_TOF == result->is_caf_trig) {
			ISP_LOGI("tof focus_state = %s,", FOCUS_STATE_STR(af->focus_state));
			force_stop = AFV1_TRUE;
			af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_SET_CANCEL, &force_stop);	//modifiy for force stop to SAF/Flow control
			af->focus_state = AF_STOPPED_INNER;
			af->af_ops.calc(af->af_alg_cxt);

			if (STATE_FAF == af->state) {
				ISP_LOGI("pre_state %s", STATE_STRING(af->pre_state));
				af->state = af->pre_state;
			}

			tof_start(af, AF_TRIGGER, result);
			af->focus_state = AF_SEARCHING;
		}
		/*else if (AFT_TRIG_PD == result->is_caf_trig) {
		   pd_start(af, RE_TRIGGER, result);
		   } */
	}
}

static void caf_monitor_calc(af_ctrl_t * af, struct aft_proc_calc_param *prm)
{
	struct aft_proc_result res;
	memset(&res, 0, sizeof(res));
	if (AF_ALG_TRIBLE_W_T_UW == af->is_multi_mode || AF_ALG_DUAL_W_T == af->is_multi_mode) {
		if (af->sensor_role != AF_ROLE_MASTER && AFT_DATA_PD == prm->active_data_type) {
			trigger_calc(af, prm, &res);
		} else if (AF_ROLE_MASTER == af->sensor_role) {
			trigger_calc(af, prm, &res);
		}
	} else {
		trigger_calc(af, prm, &res);
	}

	ISP_LOGV("is_caf_trig = %d, is_cancel_caf = %d, is_need_rough_search = %d", res.is_caf_trig, res.is_cancel_caf, res.is_need_rough_search);

	if ((0 == af->flash_on) && (STATE_CAF == af->state || STATE_RECORD_CAF == af->state || STATE_FAF == af->state) && AF_ROLE_MASTER == af->sensor_role) {
		caf_monitor_trigger(af, prm, &res);
	}
}

static void caf_monitor_af(af_ctrl_t * af)
{
	cmr_u64 fv[10];
	struct aft_proc_calc_param *prm = &(af->prm_trigger);

	memset(fv, 0, sizeof(fv));
	memset(prm, 0, sizeof(struct aft_proc_calc_param));
	afm_get_fv(af, fv, ENHANCED_BIT, af->roi.num);

	prm->afm_info.win_cfg.win_cnt = af->roi.num;
	memcpy(prm->afm_info.win_cfg.win_pos, af->roi.win, af->roi.num * sizeof(win_coord_t));	// be carefull that the af->roi.num should be less than af->roi.win number and prm->afm_info.win_cfg.win_pos number
	prm->afm_info.filter_info.filter_num = 1;
	prm->afm_info.filter_info.filter_data[0].data = fv;
	prm->afm_info.filter_info.filter_data[0].type = 1;
	prm->active_data_type = AFT_DATA_AF;

	caf_monitor_calc(af, prm);
}

#define CALC_HIST(sum, gain, pixels, awb_gain, max, hist) \
{cmr_u64 v=((cmr_u64)(sum)*(gain))/((cmr_u64)(pixels)*(awb_gain)); \
v=v>(max)?(max):v; hist[v]++;}

static void calc_histogram(af_ctrl_t * af, isp_awb_statistic_hist_info_t * stat)
{
	cmr_u32 gain_r = af->awb.r_gain;
	cmr_u32 gain_g = af->awb.g_gain;
	cmr_u32 gain_b = af->awb.b_gain;
	cmr_u32 pixels = af->ae.win_size;
	cmr_u32 awb_base_gain = 1024;
	cmr_u32 max_value = 255;
	cmr_u32 i, j;

	if (pixels < 1)
		return;

	memset(stat->r_hist, 0, sizeof(stat->r_hist));
	memset(stat->g_hist, 0, sizeof(stat->g_hist));
	memset(stat->b_hist, 0, sizeof(stat->b_hist));

	cmr_u32 ae_skip_line = 0;
	if (af->trig_ops.ae_skip_info.ae_select_support) {
		ae_skip_line = af->trig_ops.ae_skip_info.ae_skip_line;
	}

	for (i = ae_skip_line; i < (32 - ae_skip_line); ++i) {
		for (j = ae_skip_line; j < (32 - ae_skip_line); ++j) {
			CALC_HIST(stat->r_info[32 * i + j], gain_r, pixels, awb_base_gain, max_value, stat->r_hist);
			CALC_HIST(stat->g_info[32 * i + j], gain_g, pixels, awb_base_gain, max_value, stat->g_hist);
			CALC_HIST(stat->b_info[32 * i + j], gain_b, pixels, awb_base_gain, max_value, stat->b_hist);
		}
	}
}

static void caf_monitor_ae(af_ctrl_t * af, const struct af_ae_calc_out *ae, isp_awb_statistic_hist_info_t * stat)
{
	struct aft_proc_calc_param *prm = &(af->prm_trigger);
	ISP_LOGV("focus_state = %s", FOCUS_STATE_STR(af->focus_state));

	memset(prm, 0, sizeof(struct aft_proc_calc_param));

	calc_histogram(af, stat);

	prm->active_data_type = AFT_DATA_IMG_BLK;
	prm->img_blk_info.block_w = 32;
	prm->img_blk_info.block_h = 32;
	prm->img_blk_info.pix_per_blk = af->ae.win_size;
	prm->img_blk_info.chn_num = 3;
	prm->img_blk_info.data = (cmr_u32 *) stat;
	prm->ae_info.exp_time = ae->cur_exp_line * ae->line_time / 10;
	prm->ae_info.gain = ae->cur_again;
	prm->ae_info.cur_lum = ae->cur_lum;
	prm->ae_info.target_lum = ae->target_lum;
	prm->ae_info.is_stable = ae->is_stab;
	prm->ae_info.flag4idx = ae->flag4idx;
	prm->ae_info.face_stable = ae->face_stable;
	prm->ae_info.face_ae_enable = ae->face_ae_enable;
	prm->ae_info.bv = ae->bv;
	prm->ae_info.y_sum = af->Y_sum_trigger;
	prm->ae_info.cur_scene = OUT_SCENE;
	prm->ae_info.registor_pos = (cmr_u32) lens_get_pos(af);
	//ISP_LOGI("exp_time = %d, gain = %d, cur_lum = %d, is_stable = %d, bv = %d", prm->ae_info.exp_time, prm->ae_info.gain, prm->ae_info.cur_lum, prm->ae_info.is_stable, prm->ae_info.bv);

	caf_monitor_calc(af, prm);
}

static void caf_monitor_sensor(af_ctrl_t * af, struct afctrl_sensor_info_t *in)
{
	struct afctrl_sensor_info_t *aux_sensor_info = (struct afctrl_sensor_info_t *)in;
	uint32_t sensor_type = aux_sensor_info->type;
	struct aft_proc_calc_param *prm = &(af->prm_trigger);

	memset(prm, 0, sizeof(struct aft_proc_calc_param));
	switch (sensor_type) {
	case AF_SENSOR_ACCELEROMETER:
		prm->sensor_info.sensor_type = AFT_POSTURE_ACCELEROMETER;
		prm->sensor_info.x = aux_sensor_info->gsensor_info.vertical_down;
		prm->sensor_info.y = aux_sensor_info->gsensor_info.vertical_up;
		prm->sensor_info.z = aux_sensor_info->gsensor_info.horizontal;
		break;
	case AF_SENSOR_GYROSCOPE:
		prm->sensor_info.sensor_type = AFT_POSTURE_GYRO;
		prm->sensor_info.x = aux_sensor_info->gyro_info.x;
		prm->sensor_info.y = aux_sensor_info->gyro_info.y;
		prm->sensor_info.z = aux_sensor_info->gyro_info.z;
		break;
	default:
		break;
	}
	prm->active_data_type = AFT_DATA_SENSOR;
	ISP_LOGV("[%d] sensor type %d %f %f %f ", af->state, prm->sensor_info.sensor_type, prm->sensor_info.x, prm->sensor_info.y, prm->sensor_info.z);
	caf_monitor_calc(af, prm);
}

static void caf_monitor_phase_diff(af_ctrl_t * af)
{
	struct aft_proc_calc_param *prm = &(af->prm_trigger);

	if (MULTIZONE != af->pd.af_type) {	//MULTIZONE mode should consider the central roi for trigger
		memset(prm, 0, sizeof(struct aft_proc_calc_param));
		prm->active_data_type = AFT_DATA_PD;
		prm->pd_info.pd_enable = af->pd.pd_enable;
		prm->pd_info.effective_frmid = af->pd.effective_frmid;
		prm->pd_info.pd_roi_num = af->pd.pd_roi_num;
		memcpy(&(prm->pd_info.confidence[0]), &(af->pd.confidence[0]), sizeof(cmr_u32) * (MIN(af->pd.pd_roi_num, PD_MAX_AREA)));
		memcpy(&(prm->pd_info.pd_value[0]), &(af->pd.pd_value[0]), sizeof(double) * (MIN(af->pd.pd_roi_num, PD_MAX_AREA)));
		memcpy(&(prm->pd_info.pd_roi_dcc[0]), &(af->pd.pd_roi_dcc[0]), sizeof(cmr_u32) * (16));

		prm->comm_info.otp_inf_pos = af->otp_info.rdm_data.infinite_cali;
		prm->comm_info.otp_macro_pos = af->otp_info.rdm_data.macro_cali;
		prm->comm_info.registor_pos = (cmr_u32) lens_get_pos(af);
		ISP_LOGV("F[%d]C[%d]PD[%f]DCC[%d] pd data in[%d] ", prm->pd_info.effective_frmid, af->pd.confidence[0], af->pd.pd_value[0], af->pd.pd_roi_dcc[0],
			 prm->pd_info.pd_enable);
		caf_monitor_calc(af, prm);
	}

	return;
}

static void caf_monitor_fd(af_ctrl_t * af)
{

	struct aft_proc_calc_param *prm = &(af->prm_trigger);
	cmr_u8 i = 0;

	memset(prm, 0, sizeof(struct aft_proc_calc_param));
	prm->active_data_type = AFT_DATA_FD;

	i = sizeof(prm->fd_info.face_info) / sizeof(prm->fd_info.face_info[0]);
	prm->fd_info.face_num = i > af->face_info.face_num ? af->face_info.face_num : i;

	i = 0;
	while (i < prm->fd_info.face_num) {
		prm->fd_info.face_info[i].sx = af->face_info.face_info[i].sx;
		prm->fd_info.face_info[i].sy = af->face_info.face_info[i].sy;
		prm->fd_info.face_info[i].ex = af->face_info.face_info[i].ex;
		prm->fd_info.face_info[i].ey = af->face_info.face_info[i].ey;
		prm->fd_info.face_info[i].yaw_angle = af->face_info.face_info[i].pose;
		prm->fd_info.face_info[i].roll_angle = af->face_info.face_info[i].angle;
		prm->fd_info.face_info[i].score = 0;
		i++;
	}
	prm->fd_info.frame_width = af->face_info.frame_width;
	prm->fd_info.frame_height = af->face_info.frame_height;

	caf_monitor_calc(af, prm);
}

//[TOF_+++]
static void caf_monitor_tof(af_ctrl_t * af)
{
	struct aft_proc_calc_param *prm = &(af->prm_trigger);

	memset(prm, 0, sizeof(struct aft_proc_calc_param));
	prm->active_data_type = AFT_DATA_TOF;
	//prm->tof_info.tof_enable = af->pd.pd_enable;
	//prm->tof_info.effective_frmid = af->pd.effective_frmid;
	prm->tof_info.status = af->tof.data.RangeStatus;
	prm->tof_info.distance = af->tof.data.RangeMilliMeter;
	prm->tof_info.MAXdistance = af->tof.data.RangeDMaxMilliMeter;

	caf_monitor_calc(af, prm);

	return;
}

//[TOF_---]

static void caf_monitor_process(af_ctrl_t * af)
{
	if (af->trigger_source_type & AF_DATA_FD) {
		af->trigger_source_type &= (~AF_DATA_FD);
		caf_monitor_fd(af);
	}
	//[TOF_+++]
	/* move to af_sprd_set_tof_info()
	   if (af->trigger_source_type & AF_DATA_TOF) {
	   af->trigger_source_type &= (~AF_DATA_TOF);
	   caf_monitor_tof(af);
	   }
	 */
	//[TOF_---]

	if (af->trigger_source_type & AF_DATA_PD) {
		af->trigger_source_type &= (~AF_DATA_PD);
		caf_monitor_phase_diff(af);
	}

	if (af->trigger_source_type & AF_DATA_AF) {
		af->trigger_source_type &= (~AF_DATA_AF);
		caf_monitor_af(af);
	}

	if (af->trigger_source_type & AF_DATA_AE) {
		af->trigger_source_type &= (~AF_DATA_AE);
		caf_monitor_ae(af, &(af->ae.ae_report), &(af->rgb_stat));
	}

	if (af->trigger_source_type & AF_DATA_G) {
		struct afctrl_sensor_info_t aux_sensor_info;
		aux_sensor_info.type = AF_SENSOR_ACCELEROMETER;
		aux_sensor_info.gsensor_info.vertical_up = af->gsensor_info.vertical_up;
		aux_sensor_info.gsensor_info.vertical_down = af->gsensor_info.vertical_down;
		aux_sensor_info.gsensor_info.horizontal = af->gsensor_info.horizontal;
		aux_sensor_info.gsensor_info.timestamp = af->gsensor_info.timestamp;
		caf_monitor_sensor(af, &aux_sensor_info);
		af->trigger_source_type &= (~AF_DATA_G);
	}

	return;
}

// af ioctrl functions
static cmr_s32 af_sprd_set_af_mode(cmr_handle handle, void *param0)
{
	af_ctrl_t *af = (af_ctrl_t *) handle;
	cmr_u32 af_mode = *(cmr_u32 *) param0;
	cmr_s32 rtn = AFV1_SUCCESS;
	enum aft_mode mode;
	cmr_u16 pos = 0;

	ISP_LOGI("af state = %s, focus state = %s, set af_mode = %d", STATE_STRING(af->state), FOCUS_STATE_STR(af->focus_state), af_mode);
	property_get("vendor.cam.af_mode", af->AF_MODE, "none");
	if (0 != strcmp(af->AF_MODE, "none")) {
		ISP_LOGI("AF_MODE %s is not null, af test mode", af->AF_MODE);
		pos = lens_get_pos(af);
		ISP_LOGV("VCM registor pos :%d", pos);
		af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_SET_REG_POS, &pos);
		return rtn;
	}

	if (AF_SEARCHING == af->focus_state) {
		ISP_LOGI("last af was not done, af state %s, pre_state %s", STATE_STRING(af->state), STATE_STRING(af->pre_state));
		af_stop_search(af);
	}

	af->pre_state = af->state;

	if (AF_MODE_MANUAL == af_mode) {
		af->last_request_mode = af_mode;
	}

	switch (af_mode) {
	case AF_MODE_NORMAL:
		if (AF_ALG_BLUR_REAR == af->is_multi_mode) {
			af->request_mode = af_mode;
			af->state = STATE_FULLSCAN;
		} else {
			af->request_mode = af_mode;
			af->state = STATE_NORMAL_AF;
		}
		trigger_set_mode(af, AFT_MODE_NORMAL);
		trigger_start(af);
		break;
	case AF_MODE_CONTINUE:
	case AF_MODE_VIDEO:
		af->request_mode = af_mode;
		af->state = AF_MODE_CONTINUE == af_mode ? STATE_CAF : STATE_RECORD_CAF;
		af->algo_mode = STATE_CAF == af->state ? CAF : VAF;
		af_set_default_roi(af, af->algo_mode);
		do_start_af(af);
		mode = STATE_CAF == af->state ? AFT_MODE_CONTINUE : AFT_MODE_VIDEO;
		trigger_set_mode(af, mode);
		trigger_start(af);

		if (AF_MODE_MANUAL == af->last_request_mode) {
			af->last_request_mode = af_mode;
			trigger_notice_force(af);
		}

		break;
	case AF_MODE_PICTURE:
		break;
	case AF_MODE_FULLSCAN:
		af->request_mode = af_mode;
		af->state = STATE_FULLSCAN;
		trigger_set_mode(af, AFT_MODE_NORMAL);
		trigger_stop(af);
		break;
	case AF_MODE_MANUAL:
		af->request_mode = af_mode;
		trigger_stop(af);
		break;
	default:
		ISP_LOGW("af_mode %d is not supported", af_mode);
		break;
	}

	return rtn;
}

static cmr_s32 af_sprd_set_af_trigger(cmr_handle handle, void *param0)
{
	af_ctrl_t *af = (af_ctrl_t *) handle;
	struct af_trig_info *win = (struct af_trig_info *)param0;	//win = (struct isp_af_win *)param;
	AF_Trigger_Data aft_in;
	cmr_s32 rtn = AFV1_SUCCESS;
	int pro_rtn = 0;

	ISP_LOGI("trigger af state = %s", STATE_STRING(af->state));
	pro_rtn = property_set("vendor.cam.af_mode", "none");
	if (pro_rtn)
		ISP_LOGE("unable to property set!");
	af->test_loop_quit = 1;

	if (STATE_FULLSCAN == af->state) {
		af->algo_mode = CAF;
		memset(&aft_in, 0, sizeof(AF_Trigger_Data));
		aft_in.AFT_mode = af->algo_mode;
		aft_in.bisTrigger = AF_TRIGGER;
		aft_in.AF_Trigger_Type = BOKEH;
		af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_SET_TRIGGER, &aft_in);
		do_start_af(af);
	} else if (STATE_NORMAL_AF == af->state) {
		saf_start(af, win);
	} else if (STATE_CAF == af->state || STATE_RECORD_CAF == af->state || STATE_FAF == af->state) {
		af->cont_mode_trigger = 1;
		trigger_set_mode(af, AFT_MODE_NORMAL);
		trigger_stop(af);
		if (AF_SEARCHING == af->focus_state) {
			ISP_LOGI("last af was not done, af state %s, pre_state %s", STATE_STRING(af->state), STATE_STRING(af->pre_state));
			af_stop_search(af);
		}
		saf_start(af, win);
	} else if (STATE_OTAF == af->state) {
		af_stop_search(af);
		otaf_update_af_state(af, AFV1_FALSE);
		saf_start(af, win);
	}
	af->focus_state = AF_SEARCHING;
	ISP_LOGV("saf start done \n");
	return rtn;
}

static cmr_s32 af_sprd_set_af_cancel(cmr_handle handle, void *param0)
{
	UNUSED(param0);
	af_ctrl_t *af = (af_ctrl_t *) handle;
	cmr_s32 rtn = AFV1_SUCCESS;
	ISP_LOGI("cancel af state = %s", STATE_STRING(af->state));

	if (AF_SEARCHING == af->focus_state) {
		ISP_LOGW("serious problem, current af is not done, af state %s", STATE_STRING(af->state));
		//wait saf/caf done, caf : for camera enter, switch to manual; saf : normal non zsl
		af_stop_search(af);
	}
	if (1 == af->cont_mode_trigger) {
		af->cont_mode_trigger = 0;
		if (AF_MODE_CONTINUE == af->request_mode) {
			trigger_set_mode(af, AFT_MODE_CONTINUE);
		} else if (AF_MODE_VIDEO == af->request_mode) {
			trigger_set_mode(af, AFT_MODE_VIDEO);
		}
		trigger_start(af);
	}

	return rtn;
}

static cmr_s32 af_sprd_set_af_bypass(cmr_handle handle, void *param0)
{
	af_ctrl_t *af = (af_ctrl_t *) handle;
	char value[PROPERTY_VALUE_MAX] = { '\0' };
	cmr_s32 rtn = AFV1_SUCCESS;

	if (NULL == param0) {
		ISP_LOGE("param null error");
		rtn = AFV1_ERROR;
		return rtn;
	}

	property_get("persist.vendor.cam.isp.af.bypass", value, "0");
	if (atoi(value) == 0) {
		ISP_LOGI("param = %d", *(cmr_u32 *) param0);
		af->bypass = *(cmr_u32 *) param0;
	} else {
		ISP_LOGI("af bypass cmd is NOT allowed %d", atoi(value));
	}

	return rtn;
}

static cmr_s32 af_sprd_set_flash_notice(cmr_handle handle, void *param0)
{
	af_ctrl_t *af = (af_ctrl_t *) handle;
	cmr_u32 flash_status = *(cmr_u32 *) param0;
	cmr_s32 rtn = AFV1_SUCCESS;

	ISP_LOGV("flash_status %u", flash_status);
	switch (flash_status) {
	case AF_FLASH_PRE_BEFORE:
	case AF_FLASH_PRE_LIGHTING:
	case AF_FLASH_MAIN_BEFORE:
	case AF_FLASH_MAIN_LIGHTING:
		if (0 == af->flash_on) {
			af->flash_on = 1;
		}
		break;
	case AF_FLASH_MAIN_AFTER:
	case AF_FLASH_PRE_AFTER:
		if (1 == af->flash_on) {
			af->flash_on = 0;
			if (af->request_mode != AF_MODE_MANUAL) {
				trigger_start(af);
			}
		}
		break;
	default:
		break;
	}
	trigger_set_flash_status(af, flash_status);
	return rtn;
}

static void ae_calc_win_size(af_ctrl_t * af, struct afctrl_fwstart_info *param)
{
	cmr_u32 w, h;
	if (param->size.w && param->size.h) {
		w = ((param->size.w / 32) >> 1) << 1;
		h = ((param->size.h / 32) >> 1) << 1;
		af->ae.win_size = w * h;
	} else {
		af->ae.win_size = 1;
	}
}

static cmr_s32 af_sprd_set_video_start(cmr_handle handle, void *param0)
{
	af_ctrl_t *af = (af_ctrl_t *) handle;
	cmr_u32 afm_skip_num = 0;
	struct afctrl_fwstart_info *in_ptr = (struct afctrl_fwstart_info *)param0;

	ae_calc_win_size(af, in_ptr);
	af->isp_info.width = in_ptr->size.w;
	af->isp_info.height = in_ptr->size.h;
	ISP_LOGI("af state = %s, focus state = %s; image width = %d, height = %d", STATE_STRING(af->state), FOCUS_STATE_STR(af->focus_state), in_ptr->size.w, in_ptr->size.h);

	if (in_ptr->sensor_fps.is_high_fps) {
		afm_skip_num = in_ptr->sensor_fps.high_fps_skip_num - 1;
		if (afm_skip_num != af->afm_skip_num) {
			af->afm_skip_num = afm_skip_num;
		}
	} else {
		af->afm_skip_num = 0;
	}
	ISP_LOGI("afm_skip_num %d, camera_id = %d", af->afm_skip_num, af->camera_id);
	af->cb_ops.af_monitor_skip_num(af->caller, (void *)&af->afm_skip_num);

	af_set_default_roi(af, af->algo_mode);
	do_start_af(af);

	property_get("vendor.cam.af_mode", af->AF_MODE, "none");
	if (0 != strcmp(af->AF_MODE, "none")) {
		ISP_LOGI("AF_MODE %s is not null, af test mode", af->AF_MODE);
		return AFV1_SUCCESS;
	}
	if (AF_STOPPED == af->focus_state) {
		trigger_notice_force(af);
	}

	if (STATE_CAF == af->state || STATE_RECORD_CAF == af->state || STATE_NORMAL_AF == af->state) {
		trigger_start(af);	// for hdr capture no af mode update at whole procedure
	}

	ISP_LOGV("af->is_multi_mode = %d, af->sensor_role %d", af->is_multi_mode, af->sensor_role);
#if 0
	// debug only
	bokeh_distance_info bdi;
	memset(&bdi, 0, sizeof(bokeh_distance_info));
	bdi.total_seg = 7;
	bdi.distance[0] = 50;
	bdi.distance[1] = 60;
	bdi.distance[2] = 70;
	bdi.distance[3] = 80;
	bdi.distance[4] = 100;
	bdi.distance[5] = 120;
	bdi.distance[6] = 150;
	af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_SET_BOKEH_DISTANCE, &bdi);
#endif
	return AFV1_SUCCESS;
}

static cmr_s32 af_sprd_set_video_stop(cmr_handle handle, void *param0)
{
	UNUSED(param0);
	af_ctrl_t *af = (af_ctrl_t *) handle;
	ISP_LOGI("af state = %s, focus state = %s", STATE_STRING(af->state), FOCUS_STATE_STR(af->focus_state));

	if (STATE_CAF == af->state || STATE_RECORD_CAF == af->state || STATE_NORMAL_AF == af->state) {
		trigger_stop(af);
	}

	if (AF_SEARCHING == af->focus_state) {
		ISP_LOGW("serious problem, current af is not done, af state %s", STATE_STRING(af->state));
		af_stop_search(af);	//wait saf/caf done, caf : for camera enter, switch to manual; saf : normal non zsl
	}
	do_stop_af(af);
	return AFV1_SUCCESS;
}

static void ae_calibration(af_ctrl_t * af, struct af_img_blk_statistic *rgb)
{
	cmr_u32 i, j, r_sum[9], g_sum[9], b_sum[9];

	memset(r_sum, 0, sizeof(r_sum));
	memset(g_sum, 0, sizeof(g_sum));
	memset(b_sum, 0, sizeof(b_sum));

	for (i = 0; i < 32; i++) {
		for (j = 0; j < 32; j++) {
			r_sum[(i / 11) * 3 + j / 11] += rgb->r_info[i * 32 + j] / af->ae.win_size;
			g_sum[(i / 11) * 3 + j / 11] += rgb->g_info[i * 32 + j] / af->ae.win_size;
			b_sum[(i / 11) * 3 + j / 11] += rgb->b_info[i * 32 + j] / af->ae.win_size;
		}
	}
	af->ae_cali_data.r_avg[0] = r_sum[0] / 121;
	af->ae_cali_data.r_avg_all = af->ae_cali_data.r_avg[0];
	af->ae_cali_data.r_avg[1] = r_sum[1] / 121;
	af->ae_cali_data.r_avg_all += af->ae_cali_data.r_avg[1];
	af->ae_cali_data.r_avg[2] = r_sum[2] / 110;
	af->ae_cali_data.r_avg_all += af->ae_cali_data.r_avg[2];
	af->ae_cali_data.r_avg[3] = r_sum[3] / 121;
	af->ae_cali_data.r_avg_all += af->ae_cali_data.r_avg[3];
	af->ae_cali_data.r_avg[4] = r_sum[4] / 121;
	af->ae_cali_data.r_avg_all += af->ae_cali_data.r_avg[4];
	af->ae_cali_data.r_avg[5] = r_sum[5] / 110;
	af->ae_cali_data.r_avg_all += af->ae_cali_data.r_avg[5];
	af->ae_cali_data.r_avg[6] = r_sum[6] / 110;
	af->ae_cali_data.r_avg_all += af->ae_cali_data.r_avg[6];
	af->ae_cali_data.r_avg[7] = r_sum[7] / 110;
	af->ae_cali_data.r_avg_all += af->ae_cali_data.r_avg[7];
	af->ae_cali_data.r_avg[8] = r_sum[8] / 100;
	af->ae_cali_data.r_avg_all += af->ae_cali_data.r_avg[8];
	af->ae_cali_data.r_avg_all /= 9;

	af->ae_cali_data.g_avg[0] = g_sum[0] / 121;
	af->ae_cali_data.g_avg_all = af->ae_cali_data.g_avg[0];
	af->ae_cali_data.g_avg[1] = g_sum[1] / 121;
	af->ae_cali_data.g_avg_all += af->ae_cali_data.g_avg[1];
	af->ae_cali_data.g_avg[2] = g_sum[2] / 110;
	af->ae_cali_data.g_avg_all += af->ae_cali_data.g_avg[2];
	af->ae_cali_data.g_avg[3] = g_sum[3] / 121;
	af->ae_cali_data.g_avg_all += af->ae_cali_data.g_avg[3];
	af->ae_cali_data.g_avg[4] = g_sum[4] / 121;
	af->ae_cali_data.g_avg_all += af->ae_cali_data.g_avg[4];
	af->ae_cali_data.g_avg[5] = g_sum[5] / 110;
	af->ae_cali_data.g_avg_all += af->ae_cali_data.g_avg[5];
	af->ae_cali_data.g_avg[6] = g_sum[6] / 110;
	af->ae_cali_data.g_avg_all += af->ae_cali_data.g_avg[6];
	af->ae_cali_data.g_avg[7] = g_sum[7] / 110;
	af->ae_cali_data.g_avg_all += af->ae_cali_data.g_avg[7];
	af->ae_cali_data.g_avg[8] = g_sum[8] / 100;
	af->ae_cali_data.g_avg_all += af->ae_cali_data.g_avg[8];
	af->ae_cali_data.g_avg_all /= 9;

	af->ae_cali_data.b_avg[0] = b_sum[0] / 121;
	af->ae_cali_data.b_avg_all = af->ae_cali_data.b_avg[0];
	af->ae_cali_data.b_avg[1] = b_sum[1] / 121;
	af->ae_cali_data.b_avg_all += af->ae_cali_data.b_avg[1];
	af->ae_cali_data.b_avg[2] = b_sum[2] / 110;
	af->ae_cali_data.b_avg_all += af->ae_cali_data.b_avg[2];
	af->ae_cali_data.b_avg[3] = b_sum[3] / 121;
	af->ae_cali_data.b_avg_all += af->ae_cali_data.b_avg[3];
	af->ae_cali_data.b_avg[4] = b_sum[4] / 121;
	af->ae_cali_data.b_avg_all += af->ae_cali_data.b_avg[4];
	af->ae_cali_data.b_avg[5] = b_sum[5] / 110;
	af->ae_cali_data.b_avg_all += af->ae_cali_data.b_avg[5];
	af->ae_cali_data.b_avg[6] = b_sum[6] / 110;
	af->ae_cali_data.b_avg_all += af->ae_cali_data.b_avg[6];
	af->ae_cali_data.b_avg[7] = b_sum[7] / 110;
	af->ae_cali_data.b_avg_all += af->ae_cali_data.b_avg[7];
	af->ae_cali_data.b_avg[8] = b_sum[8] / 100;
	af->ae_cali_data.b_avg_all += af->ae_cali_data.b_avg[8];
	af->ae_cali_data.b_avg_all /= 9;

	ISP_LOGV("(r,g,b) in block4 is (%d,%d,%d)", af->ae_cali_data.r_avg[4], af->ae_cali_data.g_avg[4], af->ae_cali_data.b_avg[4]);
}

static void scl_for_ae_stat(struct af_img_blk_info *rgb, isp_awb_statistic_hist_info_t * dst_data)
{
	cmr_u32 i, j, ii, jj;
	cmr_u64 sum;
	cmr_u32 blk_num_w = (rgb->block_w < 32) ? 32 : rgb->block_w;
	cmr_u32 blk_num_h = (rgb->block_h < 32) ? 32 : rgb->block_h;
	cmr_u32 ratio_h = blk_num_h / 32;
	cmr_u32 ratio_w = blk_num_w / 32;
	cmr_u32 *src_data = (cmr_u32 *) rgb->data;
	cmr_u32 *r_stat = (cmr_u32 *) src_data;
	cmr_u32 *g_stat = (cmr_u32 *) src_data + 16384;
	cmr_u32 *b_stat = (cmr_u32 *) src_data + 2 * 16384;
	cmr_u32 *dst_r = dst_data->r_info;
	cmr_u32 *dst_g = dst_data->g_info;
	cmr_u32 *dst_b = dst_data->b_info;
	memset(dst_r, 0, 1024 * sizeof(cmr_u32));
	memset(dst_g, 0, 1024 * sizeof(cmr_u32));
	memset(dst_b, 0, 1024 * sizeof(cmr_u32));

	for (i = 0; i < blk_num_h; ++i) {
		ii = (cmr_u32) (i / ratio_h);
		for (j = 0; j < blk_num_w; ++j) {
			jj = j / ratio_w;
			/*for r channel */
			sum = r_stat[i * blk_num_w + j];
			dst_r[ii * 32 + jj] += sum;

			/*for g channel */
			sum = g_stat[i * blk_num_w + j];
			dst_g[ii * 32 + jj] += sum;

			/*for b channel */
			sum = b_stat[i * blk_num_w + j];
			dst_b[ii * 32 + jj] += sum;
		}
	}
}

static void set_af_RGBY(af_ctrl_t * af, struct af_img_blk_info *rgb)
{
#define AE_BLOCK_W 32
#define AE_BLOCK_H 32

	cmr_u32 Y_sx = 0, Y_ex = 0, Y_sy = 0, Y_ey = 0, r_sum = 0, g_sum = 0, b_sum = 0, y_sum = 0;
	float ae_area;
	cmr_u16 width, height, i = 0, blockw, blockh, index;

	width = af->isp_info.width;
	height = af->isp_info.height;

	//memcpy(&(af->rgb_stat.r_info[0]), rgb->r_info, sizeof(af->rgb_stat.r_info));
	//memcpy(&(af->rgb_stat.g_info[0]), rgb->g_info, sizeof(af->rgb_stat.g_info));
	//memcpy(&(af->rgb_stat.b_info[0]), rgb->b_info, sizeof(af->rgb_stat.b_info));
	scl_for_ae_stat(rgb, &(af->rgb_stat));

	af->roi_RGBY.num = af->roi.num;

	af->roi_RGBY.Y_sum[af->roi.num] = 0;
	for (i = 0; i < af->roi.num; i++) {
		Y_sx = af->roi.win[i].start_x / (width / AE_BLOCK_W);
		Y_ex = af->roi.win[i].end_x / (width / AE_BLOCK_W);
		Y_sy = af->roi.win[i].start_y / (height / AE_BLOCK_H);
		Y_ey = af->roi.win[i].end_y / (height / AE_BLOCK_H);
		// exception
		if (Y_sx == Y_ex)
			Y_ex = Y_sx + 1;
		// exception
		if (Y_sy == Y_ey)
			Y_ey = Y_sy + 1;

		r_sum = 0;
		g_sum = 0;
		b_sum = 0;
		for (blockw = Y_sx; blockw <= Y_ex; blockw++) {
			for (blockh = Y_sy; blockh <= Y_ey; blockh++) {
				index = blockh * AE_BLOCK_W + blockw;
				r_sum = r_sum + af->rgb_stat.r_info[index];
				g_sum = g_sum + af->rgb_stat.g_info[index];
				b_sum = b_sum + af->rgb_stat.b_info[index];
			}
		}

		ae_area = 1.0 * (Y_ex - Y_sx + 1) * (Y_ey - Y_sy + 1);
		y_sum = (((0.299 * r_sum) + (0.587 * g_sum) + (0.114 * b_sum)) / ae_area);
		af->roi_RGBY.R_sum[i] = r_sum;
		af->roi_RGBY.G_sum[i] = g_sum;
		af->roi_RGBY.B_sum[i] = b_sum;
		af->roi_RGBY.Y_sum[i] = y_sum;
		// ISP_LOGV("y_sum[%d] = %d",i,y_sum);
	}

	if (0 != af->roi.num) {
		switch (af->state) {
		case STATE_FAF:
			af->Y_sum_trigger = af->roi_RGBY.Y_sum[af->roi.num - 1];
			af->Y_sum_normalize = af->roi_RGBY.Y_sum[af->roi.num - 1];
			break;
		default:
			af->Y_sum_trigger = af->roi_RGBY.Y_sum[af->roi.num - 1];
			af->Y_sum_normalize = af->roi_RGBY.Y_sum[af->roi.num - 1];
			break;
		}
	}

	property_get("vendor.cam.af_mode", af->AF_MODE, "none");
	if (0 != strcmp(af->AF_MODE, "none")) {	// test mode only
		ae_calibration(af, (struct af_img_blk_statistic *)rgb->data);
	}

}

static cmr_s32 af_sprd_set_ae_info(cmr_handle handle, void *param0)
{
	af_ctrl_t *af = (af_ctrl_t *) handle;
	struct afctrl_ae_info *ae_info = (struct afctrl_ae_info *)param0;
	struct af_img_blk_info *ae_stat_ptr = (struct af_img_blk_info *)&(ae_info->img_blk_info);
	cmr_s32 rtn = AFV1_SUCCESS;
	if ((0 == af->isp_info.width) || (0 == af->isp_info.height)) {
		return AFV1_ERROR;
	}

	set_af_RGBY(af, (void *)ae_stat_ptr);
	memcpy(&(af->ae.ae_report), &(ae_info->ae_rlt_info), sizeof(struct af_ae_calc_out));
	af->trigger_source_type |= AF_DATA_AE;
	return rtn;
}

static cmr_s32 af_sprd_set_awb_info(cmr_handle handle, void *param0)
{
	af_ctrl_t *af = (af_ctrl_t *) handle;
	struct afctrl_awb_info *awb = (struct afctrl_awb_info *)param0;
	af->awb.r_gain = awb->r_gain;
	af->awb.g_gain = awb->g_gain;
	af->awb.b_gain = awb->b_gain;
	// af->trigger_source_type |= AF_DATA_AWB;
	return AFV1_SUCCESS;
}

static cmr_s32 af_sprd_set_face_detect(cmr_handle handle, void *param0)
{
	af_ctrl_t *af = (af_ctrl_t *) handle;
	struct afctrl_face_info *face = (struct afctrl_face_info *)param0;
	cmr_s32 rtn = AFV1_SUCCESS;
	cmr_u8 i = 0;
	if (NULL != face /* && 0 != face->face_num */ ) {
		memcpy(&af->face_info, face, sizeof(struct afctrl_face_info));
		af->trigger_source_type |= AF_DATA_FD;
	}
	if (NULL != face && 0 != face->face_num) {
		for (i = 0; i < face->face_num; i++) {
			ISP_LOGV("idx=%d,sx=%d, sy=%d,ex=%d,ey=%d,roll_angle=%d", i, face->face_info[i].sx, face->face_info[i].sy, face->face_info[i].ex, face->face_info[i].ey,
				 face->face_info[i].angle);
		}
	}
	return rtn;
}

static cmr_s32 af_sprd_set_dcam_timestamp(cmr_handle handle, void *param0)
{
	af_ctrl_t *af = (af_ctrl_t *) handle;
	struct afctrl_ts_info *af_ts = (struct afctrl_ts_info *)param0;
	cmr_s32 timecompare = 0;
	cmr_u16 pos[2] = { 0 };
	AF_Timestamp timestamp;

	timecompare = compare_timestamp(af);
	if (0 == af_ts->capture) {
		af->dcam_timestamp = af_ts->timestamp;
		timestamp.type = AF_TIME_DCAM;
		timestamp.time_stamp = af->dcam_timestamp;
		af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_SET_TIMESTAMP, &timestamp);
		//ISP_LOGI("dcam_timestamp %" PRIu64 " ", (cmr_s64) af->dcam_timestamp);
		if (AF_IDLE == af->focus_state && DCAM_AFTER_VCM_YES == timecompare && 0 == af->vcm_stable) {
			af->vcm_stable = 1;
		}
	} else if (1 == af_ts->capture) {
		af->takepic_timestamp = af_ts->timestamp;
		timestamp.type = AF_TIME_CAPTURE;
		timestamp.time_stamp = af->takepic_timestamp;
		af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_SET_TIMESTAMP, &timestamp);
		//ISP_LOGI("takepic_timestamp %" PRIu64 " ", (cmr_s64) af->takepic_timestamp);
		ISP_LOGV("takepic_timestamp - vcm_timestamp =%" PRId64 " ms", ((cmr_s64) af->takepic_timestamp - (cmr_s64) af->vcm_timestamp) / 1000000);

		if (0 == af->ts_counter) {
			pos[0] = lens_get_pos(af);
			ISP_LOGV("VCM registor pos0 :%d", pos[0]);
			af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_SET_REG_POS, &pos[0]);
		} else if (1 == af->ts_counter) {
			pos[1] = lens_get_pos(af);
			ISP_LOGV("VCM registor pos1 :%d", pos[1]);
		}
		af->ts_counter++;
		if (2 == af->ts_counter) {
			af->ts_counter = 0;
		}
	}

	return AFV1_SUCCESS;
}

static cmr_s32 af_sprd_set_pd_info(cmr_handle handle, void *param0)
{

	char value[PROPERTY_VALUE_MAX] = { '\0' };

	property_get("persist.vendor.cam.pd.enable", value, "1");
	if (atoi(value) != 1) {
		UNUSED(handle);
		ISP_LOGI("af_sprd_set_pd_info Disable! E %p", param0);
		return AFV1_SUCCESS;
	}

	af_ctrl_t *af = (af_ctrl_t *) handle;
	struct pd_result *pd_calc_result = (struct pd_result *)param0;

	memset(&(af->pd), 0, sizeof(pd_algo_result_t));
	af->pd.effective_frmid = (cmr_u32) pd_calc_result->pdGetFrameID;
	af->pd.pd_enable = (af->pd.effective_frmid) ? 1 : 0;
	af->pd.pd_roi_num = pd_calc_result->pd_roi_num;
	af->pd.af_type = pd_calc_result->af_type;
	// transfer full phase diff data value to algorithm
	memcpy(&(af->pd.confidence[0]), &(pd_calc_result->pdConf[0]), sizeof(cmr_u32) * (af->pd.pd_roi_num));
	memcpy(&(af->pd.pd_value[0]), &(pd_calc_result->pdPhaseDiff[0]), sizeof(double) * (af->pd.pd_roi_num));
	memcpy(&(af->pd.pd_roi_dcc[0]), &(pd_calc_result->pdDCCGain[0]), sizeof(cmr_u32) * (16));
	af->trigger_source_type |= AF_DATA_PD;
	ISP_LOGV("PD\t%lf\t%lf\t%lf\t%lf\n", pd_calc_result->pdPhaseDiff[0], pd_calc_result->pdPhaseDiff[1], pd_calc_result->pdPhaseDiff[2], pd_calc_result->pdPhaseDiff[3]);
	ISP_LOGV("Conf\t%d\t%d\t%d\t%d Total [%d]\n", pd_calc_result->pdConf[0], pd_calc_result->pdConf[1], pd_calc_result->pdConf[2], pd_calc_result->pdConf[3],
		 af->pd.pd_roi_num);
	ISP_LOGV("[%d]PD_GetResult pd_calc_result.pdConf[4] = %d, pd_calc_result.pdPhaseDiff[4] = %lf, pd_calc_result->pdDCCGain[4] = %d, af_type %d",
		 pd_calc_result->pdGetFrameID, pd_calc_result->pdConf[4], pd_calc_result->pdPhaseDiff[4], pd_calc_result->pdDCCGain[4], pd_calc_result->af_type);

	return AFV1_SUCCESS;
}

static cmr_s32 af_sprd_set_type1_pd_info(cmr_handle handle, void *param0)
{
	char prop[256];
	int val = 0;

	property_get("debug.isp.s3pdaf.disable", prop, "0");
	val = atoi(prop);

	if (val == 1) {
		UNUSED(handle);
		ISP_LOGI("af_sprd_set_type1_pd_info Disable! E %p", param0);
		return AFV1_SUCCESS;
	}
	//UNUSED(handle);
	//UNUSED(param0);

	af_ctrl_t *af = (af_ctrl_t *) handle;

	cmr_u32 i = 0;
	cmr_u32 index = 0;
	cmr_u8 sign_flag = 0;
	cmr_s32 raw = 0;
	cmr_u8 *pIMX351Buf = (cmr_u8 *) param0;

	if (NULL == af->pdaf_rdm_otp_data || NULL == param0) {
		ISP_LOGE("typ1pd input error! otp%p param%p", af->pdaf_rdm_otp_data, param0);
		return AFV1_ERROR;
	}

	ISP_LOGV("af_sprd_set_type1_pd_info E %p %p", param0, pIMX351Buf);

	for (i = 10; i < 970; i += 5) {
		IMX351_confidence_value[index] = (*(pIMX351Buf + i) << 3) | ((*(pIMX351Buf + i + 1) & 0xE0) >> 5);

		sign_flag = (*(pIMX351Buf + i + 1) & 0x10) >> 4;
		raw = ((*(pIMX351Buf + i + 1) & 0x0F) << 6) | ((*(pIMX351Buf + i + 2) & 0xFC) >> 2);

		if (sign_flag) {
			raw = -(((~raw) & 0x3FF) + 1);
		}

		IMX351_phase_difference[index] = (double)raw / 16;
		index++;
	}

	g_FrameID++;
	af->pd.effective_frmid = g_FrameID;
	af->pd.pd_enable = (af->pd.effective_frmid) ? 1 : 0;
	af->pd.pd_roi_num = IMX351_PD_ROI_NUMBER + 2;

	// transfer full phase diff data value to algorithm (8*6 fixed area mode)
	af->pd.confidence[0] = IMX351_confidence_value[35];
	af->pd.confidence[1] = IMX351_confidence_value[36];
	af->pd.confidence[2] = IMX351_confidence_value[51];
	af->pd.confidence[3] = IMX351_confidence_value[52];
	af->pd.confidence[4] = af->pd.confidence[0];	//TODO

	af->pd.pd_value[0] = IMX351_phase_difference[35];
	af->pd.pd_value[1] = IMX351_phase_difference[36];
	af->pd.pd_value[2] = IMX351_phase_difference[51];
	af->pd.pd_value[3] = IMX351_phase_difference[52];
	af->pd.pd_value[4] = af->pd.pd_value[0];	//TODO

	//PD OTP Address: pdaf_rdm_otp_data
	af->pd.pd_roi_dcc[0] = (*(af->pdaf_rdm_otp_data + IMX351_SPC_SIZE + 19 * 2) << 8) + *(af->pdaf_rdm_otp_data + IMX351_SPC_SIZE + 19 * 2 + 1);
	af->pd.pd_roi_dcc[1] = (*(af->pdaf_rdm_otp_data + IMX351_SPC_SIZE + 20 * 2) << 8) + *(af->pdaf_rdm_otp_data + IMX351_SPC_SIZE + 20 * 2 + 1);
	af->pd.pd_roi_dcc[2] = (*(af->pdaf_rdm_otp_data + IMX351_SPC_SIZE + 27 * 2) << 8) + *(af->pdaf_rdm_otp_data + IMX351_SPC_SIZE + 27 * 2 + 1);
	af->pd.pd_roi_dcc[3] = (*(af->pdaf_rdm_otp_data + IMX351_SPC_SIZE + 28 * 2) << 8) + *(af->pdaf_rdm_otp_data + IMX351_SPC_SIZE + 28 * 2 + 1);
	af->pd.pd_roi_dcc[4] = af->pd.pd_roi_dcc[0];

	//dcc[5]: Save Sensor Information for Tuning.
	af->pd.pd_roi_dcc[5] = 1001;	//IMX351: 1001

	double DCC[4];
	DCC[0] = (double)af->pd.pd_roi_dcc[0] / 512;
	DCC[1] = (double)af->pd.pd_roi_dcc[1] / 512;
	DCC[2] = (double)af->pd.pd_roi_dcc[2] / 512;
	DCC[3] = (double)af->pd.pd_roi_dcc[3] / 512;

	af->trigger_source_type |= AF_DATA_PD;
	//ISP_LOGI("DCC[%d]\t[%d]\n", *(af->pdaf_rdm_otp_data+0), *(af->pdaf_rdm_otp_data+1));
	ISP_LOGV("PD\t%lf\t%lf\t%lf\t%lf\n", af->pd.pd_value[0], af->pd.pd_value[1], af->pd.pd_value[2], af->pd.pd_value[3]);
	ISP_LOGV("Conf\t%d\t%d\t%d\t%d \n", af->pd.confidence[0], af->pd.confidence[1], af->pd.confidence[2], af->pd.confidence[3]);
	ISP_LOGV("DCC\t%d\t%d\t%d\t%d \n", af->pd.pd_roi_dcc[0], af->pd.pd_roi_dcc[1], af->pd.pd_roi_dcc[2], af->pd.pd_roi_dcc[3]);
	ISP_LOGV("DCC[u7.9]\t%lf\t%lf\t%lf\t%lf\n", DCC[0], DCC[1], DCC[2], DCC[3]);
	ISP_LOGV
	    ("Frame[%d]PD_GetResult pdPhaseDiff[4] = %lf, pdConf[4] = %d, pdDCCGain[4] = %d",
	     af->pd.effective_frmid, af->pd.pd_value[4], af->pd.confidence[4], af->pd.pd_roi_dcc[4]);

	return AFV1_SUCCESS;
}

static cmr_s32 af_sprd_set_update_aux_sensor(cmr_handle handle, void *param0)
{
	af_ctrl_t *af = (af_ctrl_t *) handle;
	struct afctrl_sensor_info_t *aux_sensor_info = (struct afctrl_sensor_info_t *)param0;

	switch (aux_sensor_info->type) {
	case AF_SENSOR_ACCELEROMETER:
		ISP_LOGV("accelerometer vertical_up = %f vertical_down = %f horizontal = %f", aux_sensor_info->gsensor_info.vertical_up,
			 aux_sensor_info->gsensor_info.vertical_down, aux_sensor_info->gsensor_info.horizontal);
		af->gsensor_info.vertical_up = aux_sensor_info->gsensor_info.vertical_up;
		af->gsensor_info.vertical_down = aux_sensor_info->gsensor_info.vertical_down;
		af->gsensor_info.horizontal = aux_sensor_info->gsensor_info.horizontal;
		af->gsensor_info.timestamp = aux_sensor_info->gsensor_info.timestamp;
		af->gsensor_info.valid = 1;
		af->trigger_source_type |= AF_DATA_G;
		break;
	case AF_SENSOR_MAGNETIC_FIELD:
		ISP_LOGV("magnetic field E");
		break;
	case AF_SENSOR_GYROSCOPE:
		ISP_LOGV("gyro E");
		break;
	case AF_SENSOR_LIGHT:
		ISP_LOGV("light E");
		break;
	case AF_SENSOR_PROXIMITY:
		ISP_LOGV("proximity E");
		break;
	default:
		ISP_LOGI("sensor type not support");
		break;
	}

	return AFV1_SUCCESS;
}

//[TOF_+++]
static cmr_s32 af_sprd_set_tof_info(cmr_handle handle, void *param0)
{
	af_ctrl_t *af = (af_ctrl_t *) handle;
	char value[PROPERTY_VALUE_MAX] = { '\0' };
	struct tof_result *tof_info = (struct tof_result *)param0;

	property_get("persist.vendor.cam.tof.enable", value, "1");

	if (atoi(value) != 1) {
		ISP_LOGI("TOF Disable! E %p", param0);
		return AFV1_ERROR;
	}

	if (NULL == tof_info)
		return AFV1_ERROR;

	memset(&(af->tof.data), 0, sizeof(struct tof_result));

	tof_FrameID = (tof_FrameID >= (0xfffffffe - 1)) ? (0) : (tof_FrameID + 1);
	af->tof.effective_frmid = tof_FrameID;
	af->tof.tof_enable = (af->tof.effective_frmid) ? 1 : 0;
	//af->trigger_source_type |= AF_DATA_TOF;

	memcpy(&af->tof.data, tof_info, sizeof(struct tof_result));

	caf_monitor_tof(af);

	/*
	   af->tof.data.RangeMilliMeter = tof_info->RangeMilliMeter;
	   af->tof.data.RangeStatus = tof_info->RangeStatus;
	   af->tof.data.RangeDMaxMilliMeter = tof_info->RangeDMaxMilliMeter;
	 */

	//ISP_LOGV("1.saf_sprd range:%d, status:%d, Dmax:%d",tof_info->RangeMilliMeter, tof_info->RangeStatus, tof_info->RangeDMaxMilliMeter);

	ISP_LOGV("2.af_sprd range:%d, status:%d, Dmax:%d", af->tof.data.RangeMilliMeter, af->tof.data.RangeStatus, af->tof.data.RangeDMaxMilliMeter);

	return AFV1_SUCCESS;
}

//[TOF_---]

static cmr_s32 af_sprd_get_fullscan_info(cmr_handle handle, void *param0)
{
	af_ctrl_t *af = (af_ctrl_t *) handle;
	struct af_fullscan_info *af_fullscan_info = (struct af_fullscan_info *)param0;
	Bokeh_Result result;
	result.win_peak_pos_num = sizeof(af->win_peak_pos) / sizeof(af->win_peak_pos[0]);
	result.win_peak_pos = af->win_peak_pos;
	af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_GET_BOKEH_RESULT, &result);
	if (NULL != af_fullscan_info) {
		af_fullscan_info->row_num = result.row_num;
		af_fullscan_info->column_num = result.column_num;
		af_fullscan_info->win_peak_pos = result.win_peak_pos;
		af_fullscan_info->vcm_dac_low_bound = result.vcm_dac_low_bound;
		af_fullscan_info->vcm_dac_up_bound = result.vcm_dac_up_bound;
		af_fullscan_info->boundary_ratio = result.boundary_ratio;

		af_fullscan_info->af_peak_pos = result.af_peak_pos;
		af_fullscan_info->near_peak_pos = result.near_peak_pos;
		af_fullscan_info->far_peak_pos = result.far_peak_pos;
		af_fullscan_info->distance_reminder = result.distance_reminder;
	}
	return AFV1_SUCCESS;
}

// SharkLE Only ++
static cmr_s32 af_sprd_set_dac_info(cmr_handle handle, void *param0)
{
	af_ctrl_t *af = (af_ctrl_t *) handle;
	af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_SET_DAC_INFO, param0);

	return AFV1_SUCCESS;
}

// SharkLE Only --

static cmr_s32 af_sprd_set_realbokeh_distance(cmr_handle handle, void *param0)
{
	af_ctrl_t *af = (af_ctrl_t *) handle;
	struct realbokeh_distance *rb_dis = (struct realbokeh_distance *)param0;
	bokeh_distance_info bdi;
	cmr_u16 i = 0;

	memset(&bdi, 0, sizeof(bokeh_distance_info));
	bdi.total_seg = rb_dis->total_seg;
	for (i = 0; i < rb_dis->total_seg; i++) {
		bdi.distance[i] = rb_dis->distance[i];
	}
	af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_SET_BOKEH_DISTANCE, &bdi);

	return AFV1_SUCCESS;
}

static cmr_s32 af_sprd_set_zoom_ratio(cmr_handle handle, void *param0)
{
	UNUSED(handle);
	af_ctrl_t *af = (af_ctrl_t *) handle;
	cmr_s32 rtn = AFV1_SUCCESS;

	if (NULL == param0) {
		ISP_LOGE("param null error");
		rtn = AFV1_ERROR;
		return rtn;
	}
	ISP_LOGI("zoom ratio = %d", *(cmr_u32 *) param0);
	af->zoom_ratio = *(cmr_u32 *) param0;

	return rtn;
}

static cmr_s32 af_sprd_set_ot_switch(cmr_handle handle, void *param0)
{
	UNUSED(handle);
	af_ctrl_t *af = (af_ctrl_t *) handle;
	cmr_s32 rtn = AFV1_SUCCESS;

	if (NULL == param0) {
		ISP_LOGE("param null error");
		rtn = AFV1_ERROR;
		return rtn;
	}

	ISP_LOGI("ot switch = %d", *(cmr_u32 *) param0);
	if (AF_PDAF_DUAL != af->pdaf_type) {
		ISP_LOGE("sensor do not support otaf %d", af->pdaf_type);
		rtn = AFV1_ERROR;
		return rtn;
	}

	af->ot_switch = *(cmr_u32 *) param0;

	if (AF_SEARCHING == af->focus_state) {
		ISP_LOGW("serious problem, current af is not done, af state %s", STATE_STRING(af->state));
		af_stop_search(af);
	}

	ISP_LOGW("af->ot_switch %d, af state %s", af->ot_switch, STATE_STRING(af->state));
	return rtn;
}

static cmr_s32 otaf_update_af_state(cmr_handle handle, cmr_u32 otaf_on)
{
	UNUSED(handle);
	af_ctrl_t *af = (af_ctrl_t *) handle;
	cmr_s32 rtn = AFV1_SUCCESS;

	if (AFV1_TRUE == otaf_on) {
		af->state = STATE_OTAF;
		trigger_stop(af);
	} else if (AFV1_FALSE == otaf_on) {
		switch (af->request_mode) {
		case AF_MODE_NORMAL:
			af->state = STATE_NORMAL_AF;
			trigger_start(af);
			break;
		case AF_MODE_CONTINUE:
		case AF_MODE_VIDEO:
			af->state = AF_MODE_CONTINUE == af->request_mode ? STATE_CAF : STATE_RECORD_CAF;
			trigger_start(af);
			break;
		case AF_MODE_FULLSCAN:
			af->state = STATE_FULLSCAN;
			trigger_stop(af);
			break;
		case AF_MODE_MANUAL:
			af->state = STATE_MANUAL;
			trigger_stop(af);
			break;
		default:
			ISP_LOGW("af_mode %d is not supported", af->request_mode);
			break;
		}
	} else {
		ISP_LOGE("af->otaf_on %d error", otaf_on);
	}
	ISP_LOGW("otaf_on %d, af state %s", otaf_on, STATE_STRING(af->state));
	return rtn;
}

static cmr_s32 af_sprd_set_ot_info(cmr_handle handle, void *param0)
{
	UNUSED(handle);
	af_ctrl_t *af = (af_ctrl_t *) handle;
	struct afctrl_ot_info *ot_info = (struct afctrl_ot_info *)param0;
	cmr_s32 rtn = AFV1_SUCCESS;
	char prop[256];

	if (NULL == param0) {
		ISP_LOGE("param null error");
		rtn = AFV1_ERROR;
		return rtn;
	}

	if (AF_PDAF_DUAL != af->pdaf_type) {
		ISP_LOGV("sensor do not support otaf %d", af->pdaf_type);
		rtn = AFV1_ERROR;
		return rtn;
	}

	if (AFV1_FALSE == af->ot_switch) {
		ISP_LOGE("ot switch is off");
		rtn = AFV1_ERROR;
		return rtn;
	}

	property_get("debug.isp.af.logenable", prop, "0");
	if (atoi(prop) != 0) {
		ISP_LOGI("ot switch = %d, status = %d, focus state %d", af->ot_switch, ot_info->otstatus, af->focus_state);
		ISP_LOGI("ot cx,cy (%d,%d), image w,h (%d,%d)", ot_info->centorX, ot_info->centorY, ot_info->imageW, ot_info->imageH);
	}

	switch (ot_info->otstatus) {
	case AF_OT_LOCKED:
		if (AF_SEARCHING != af->focus_state) {
			if (0 != ot_info->centorX && 0 != ot_info->centorY) {
				otaf_update_af_state(af, AFV1_TRUE);
				otaf_start(af);
			}
		} else if (STATE_OTAF == af->state) {
			if (0 == ot_info->centorX || 0 == ot_info->centorY) {
				af_stop_search(af);
				otaf_update_af_state(af, AFV1_FALSE);
			}
		}
		break;
	case AF_OT_MISSING:
		break;
	case AF_OT_UNLOCKED:
		if (AF_SEARCHING == af->focus_state && STATE_OTAF == af->state) {
			af_stop_search(af);
			otaf_update_af_state(af, AFV1_FALSE);
		}
		break;
	case AF_OT_STOPPED:
		if (AF_SEARCHING == af->focus_state) {
			af_stop_search(af);
			otaf_update_af_state(af, AFV1_FALSE);
		}
		break;
	default:
		break;
	}
	return rtn;
}

cmr_s32 af_sprd_adpt_inctrl(cmr_handle handle, cmr_s32 cmd, void *param0, void *param1)
{
	UNUSED(param1);
	af_ctrl_t *af = (af_ctrl_t *) handle;
	cmr_int rtn = AFV1_SUCCESS;

	switch (cmd) {
	case AF_CMD_SET_AF_POS:
		if (NULL != af->cb_ops.af_set_motor_pos) {
			if (af->request_mode == AF_MODE_MANUAL)
				*(cmr_u16 *) param0 = af->range_L1 + (*(cmr_u16 *) param0) * (af->range_L4 - af->range_L1) / 1023;	//re-map the lens moving range when MF mode(L1 L4)
			af->cb_ops.af_set_motor_pos(af->caller, *(cmr_u16 *) param0);
			af->lens.pos = *(cmr_u16 *) param0;
			af->motor_status = AF_MOTOR_SET;
			af->frame_counter = 0;
		}
		break;

	case AF_CMD_SET_AF_MODE:
		rtn = af_sprd_set_af_mode(handle, param0);
		break;

	case AF_CMD_SET_TUNING_MODE:
		break;
	case AF_CMD_SET_SCENE_MODE:
		break;

	case AF_CMD_SET_AF_STOP:
		rtn = af_sprd_set_af_cancel(handle, param0);
		break;

	case AF_CMD_SET_AF_RESTART:
		break;
	case AF_CMD_SET_CAF_RESET:
		break;
	case AF_CMD_SET_CAF_STOP:
		break;
	case AF_CMD_SET_AF_FINISH:
		break;

	case AF_CMD_SET_AF_BYPASS:
		rtn = af_sprd_set_af_bypass(handle, param0);
		break;

	case AF_CMD_SET_DEFAULT_AF_WIN:
		break;

	case AF_CMD_SET_FLASH_NOTICE:
		rtn = af_sprd_set_flash_notice(handle, param0);
		break;

	case AF_CMD_SET_ISP_START_INFO:
		rtn = af_sprd_set_video_start(handle, param0);
		break;

	case AF_CMD_SET_ISP_TOOL_AF_TEST:
		break;
	case AF_CMD_SET_CAF_TRIG_START:
		break;

	case AF_CMD_SET_AE_INFO:
		rtn = af_sprd_set_ae_info(handle, param0);
		break;

	case AF_CMD_SET_AWB_INFO:
		rtn = af_sprd_set_awb_info(handle, param0);
		break;

	case AF_CMD_SET_FACE_DETECT:
		rtn = af_sprd_set_face_detect(handle, param0);
		break;

	case AF_CMD_SET_PD_INFO:
		rtn = af_sprd_set_pd_info(handle, param0);
		ISP_LOGV("pdaf set callback end");
		break;
	case AF_CMD_SET_TYPE1_PD_INFO:
		rtn = af_sprd_set_type1_pd_info(handle, param0);
		ISP_LOGV("pdaf type1 set pd info");
		break;
		//[TOF_+++]
	case AF_CMD_SET_TOF_INFO:
		rtn = af_sprd_set_tof_info(handle, param0);
		ISP_LOGV("tof set tof info");
		break;
		//[TOF_---]
	case AF_CMD_SET_UPDATE_AUX_SENSOR:
		rtn = af_sprd_set_update_aux_sensor(handle, param0);
		break;

	case AF_CMD_SET_AF_START:
		rtn = af_sprd_set_af_trigger(handle, param0);
		break;

	case AF_CMD_SET_ISP_STOP_INFO:
		rtn = af_sprd_set_video_stop(handle, param0);
		break;

	case AF_CMD_SET_DCAM_TIMESTAMP:
		rtn = af_sprd_set_dcam_timestamp(handle, param0);
		break;
		// SharkLE Only ++
	case AF_CMD_SET_DAC_INFO:
		rtn = af_sprd_set_dac_info(handle, param0);
		break;
		// SharkLE Only --
	case AF_CMD_SET_SCENE_INFO:
		break;

	case AF_CMD_SET_REALBOKEH_DISTANCE:
		rtn = af_sprd_set_realbokeh_distance(handle, param0);
		break;

	case AF_CMD_SET_ZOOM_RATIO:
		rtn = af_sprd_set_zoom_ratio(handle, param0);
		break;

	case AF_CMD_SET_OT_SWITCH:
		rtn = af_sprd_set_ot_switch(handle, param0);
		break;

	case AF_CMD_SET_OT_INFO:
		rtn = af_sprd_set_ot_info(handle, param0);
		break;

	default:
		ISP_LOGW("set cmd not support! cmd: %d", cmd);
		rtn = AFV1_ERROR;
		break;
	}

	return rtn;
}

cmr_s32 af_sprd_adpt_outctrl(cmr_handle handle, cmr_s32 cmd, void *param0, void *param1)
{
	UNUSED(param1);
	af_ctrl_t *af = (af_ctrl_t *) handle;
	cmr_int rtn = AFV1_SUCCESS;
	switch (cmd) {
	case AF_CMD_GET_AF_MODE:
		*(cmr_u32 *) param0 = af->request_mode;
		break;

	case AF_CMD_GET_AF_CUR_POS:
		*(cmr_u32 *) param0 = (cmr_u32) lens_get_pos(af);
		break;

	case AF_CMD_GET_AF_FULLSCAN_INFO:
		rtn = af_sprd_get_fullscan_info(handle, param0);
		break;

	case AF_CMD_GET_AF_LOG_INFO:
		{
			struct af_log_info *log_info = (struct af_log_info *)param0;
			log_info->log_cxt = af->af_alg_cxt;
			log_info->log_len = af->af_dump_info_len;
			ISP_LOGV("Get AF Log info 0x%x ", log_info->log_len);
			break;
		}
	case AF_CMD_GET_AFT_LOG_INFO:
		{
			struct af_log_info *log_info = (struct af_log_info *)param0;
			log_info->log_cxt = af->trig_ops.init_out.aft_dump_ptr;
			log_info->log_len = af->trig_ops.init_out.aft_dump_len;
			ISP_LOGV("Get AFT Log info 0x%x ", log_info->log_len);
			break;
		}
	case AF_CMD_GET_REALBOKEH_LIMITED_RANGE:
		{
			struct realbokeh_vcm_range *limited = (struct realbokeh_vcm_range *)param0;
			limited->limited_infi = af->realboekh_range.limited_infi;
			limited->limited_macro = af->realboekh_range.limited_macro;
			memcpy(&limited->vcm_dac[0], &af->realboekh_range.vcm_dac[0], 20 * sizeof(cmr_u16));
			limited->total_seg = af->realboekh_range.total_seg;

			ISP_LOGI("Get RealBokeh LimitedRange info (%u %u), %u,%u,%u,%u,%u,%u,%u", limited->limited_infi, limited->limited_macro, limited->vcm_dac[0],
				 limited->vcm_dac[1], limited->vcm_dac[2], limited->vcm_dac[3], limited->vcm_dac[4], limited->vcm_dac[5], limited->vcm_dac[6]);
			break;
		}
	case AF_CMD_GET_BOKEH_GOLDEN_DATA:
		{
			struct realbokeh_golden_vcm_data *golden_data = (struct realbokeh_golden_vcm_data *)param0;
			golden_data->golden_count = af->golden_data.golden_count;
			golden_data->golden_macro = af->golden_data.golden_macro;
			golden_data->golden_infinity = af->golden_data.golden_infinity;
			ISP_LOGD("golden_macro=%d,golden_infinity=%d,golden_count=%d", af->golden_data.golden_macro, af->golden_data.golden_infinity, af->golden_data.golden_count);
			memcpy(&golden_data->golden_distance[0], &af->golden_data.golden_distance[0], (golden_data->golden_count) * sizeof(cmr_u16));
			memcpy(&golden_data->golden_vcm[0], &af->golden_data.golden_vcm[0], (golden_data->golden_count) * sizeof(cmr_u16));
			for (cmr_u8 i = 0; i < golden_data->golden_count; i++) {
				ISP_LOGD("golden_distance =%d,golden_vcm = %d", golden_data->golden_distance[i], golden_data->golden_vcm[i]);
			}
			break;
		}
	case AF_CMD_GET_OTP_DATA:
		{
			struct afctrl_otp_data *otp_data = (struct afctrl_otp_data *)param0;
			otp_data->otp_type = af->af_otp_type;
			otp_data->infinity = af->otp_info.rdm_data.infinite_cali;
			otp_data->macro = af->otp_info.rdm_data.macro_cali;
			ISP_LOGD("otp type %d, inf,macro(%d,%d)", otp_data->otp_type, otp_data->infinity, otp_data->macro);
			break;
		}
	default:
		ISP_LOGW("cmd not support! cmd: %d", cmd);
		rtn = AFV1_ERROR;
		break;
	}

	return rtn;
}

cmr_s32 af_otp_info_parser(struct afctrl_init_in * init_param)
{
	struct sensor_otp_section_info *af_otp_info_ptr = NULL;
	struct sensor_otp_section_info *module_info_ptr = NULL;
	cmr_u16 af_rdm_otp_len = 0;
	cmr_u8 *module_info = NULL;
	cmr_u8 *af_rdm_otp_data = NULL;

	if (NULL != init_param->otp_info_ptr) {
		if (init_param->otp_info_ptr->otp_vendor == OTP_VENDOR_SINGLE) {
			af_otp_info_ptr = init_param->otp_info_ptr->single_otp.af_info;
			module_info_ptr = init_param->otp_info_ptr->single_otp.module_info;
			ISP_LOGV("pass af otp, single cam");
		} else if (init_param->otp_info_ptr->otp_vendor == OTP_VENDOR_SINGLE_CAM_DUAL || init_param->otp_info_ptr->otp_vendor == OTP_VENDOR_DUAL_CAM_DUAL) {
			if (init_param->is_master == 1) {
				af_otp_info_ptr = init_param->otp_info_ptr->dual_otp.master_af_info;
				module_info_ptr = init_param->otp_info_ptr->dual_otp.master_module_info;
				ISP_LOGV("pass af otp, dual cam master");
			} else {
				af_otp_info_ptr = init_param->otp_info_ptr->dual_otp.slave_af_info;
				module_info_ptr = init_param->otp_info_ptr->dual_otp.slave_module_info;
				ISP_LOGV("pass af otp, dual cam slave");
			}
		}
	} else {
		af_otp_info_ptr = NULL;
		module_info_ptr = NULL;
		ISP_LOGE("af otp_info_ptr is NULL");
	}

	if (NULL != af_otp_info_ptr && NULL != module_info_ptr) {
		af_rdm_otp_len = af_otp_info_ptr->rdm_info.data_size;
		module_info = (cmr_u8 *) module_info_ptr->rdm_info.data_addr;

		if (NULL != module_info) {
			if ((module_info[4] == 0 && module_info[5] == 1)
			    || (module_info[4] == 0 && module_info[5] == 2)
			    || (module_info[4] == 0 && module_info[5] == 3)
			    || (module_info[4] == 0 && module_info[5] == 4)
			    || (module_info[4] == 0 && module_info[5] == 5)
			    || (module_info[4] == 1 && module_info[5] == 0
				&& (module_info[0] != 0x53 || module_info[1] != 0x50 || module_info[2] != 0x52 || module_info[3] != 0x44))
			    || (module_info[4] == 2 && module_info[5] == 0) || (module_info[4] == 3 && module_info[5] == 0) || (module_info[4] == 4 && module_info[5] == 0)
			    || (module_info[4] == 5 && module_info[5] == 0)) {
				ISP_LOGV("af otp map v0.4 or v0.5");
				af_rdm_otp_data = (cmr_u8 *) af_otp_info_ptr->rdm_info.data_addr;
			} else if (module_info[4] == 1 && (module_info[5] == 0 || module_info[5] == 1) && module_info[0] == 0x53 && module_info[1] == 0x50 && module_info[2] == 0x52
				   && module_info[3] == 0x44) {
				ISP_LOGV("af otp map v1.0 or v1.1");
				af_rdm_otp_data = (cmr_u8 *) af_otp_info_ptr->rdm_info.data_addr + 1;
			} else {
				af_rdm_otp_data = NULL;
				ISP_LOGE("af otp map version error");
			}
		} else {
			af_rdm_otp_data = NULL;
			ISP_LOGE("af module_info is NULL");
		}

		if (NULL != af_rdm_otp_data && 0 != af_rdm_otp_len) {
			init_param->otp_info.rdm_data.infinite_cali = (af_rdm_otp_data[1] << 8) | af_rdm_otp_data[0];
			init_param->otp_info.rdm_data.macro_cali = (af_rdm_otp_data[3] << 8) | af_rdm_otp_data[2];
		} else {
			ISP_LOGE("af_rdm_otp_data = %p, af_rdm_otp_len = %d. Parser fail !", af_rdm_otp_data, af_rdm_otp_len);
			init_param->otp_info.rdm_data.infinite_cali = 0;
			init_param->otp_info.rdm_data.macro_cali = 0;
		}
	} else {
		ISP_LOGE("af otp_info_ptr = %p, module_info_ptr = %p. Parser fail !", af_otp_info_ptr, module_info_ptr);
	}

	return AFV1_SUCCESS;
}

cmr_s32 pd_otp_info_parser(af_ctrl_t * af, struct afctrl_init_in * in_p)
{
	char prop[256];
	int val = 0;

	property_get("debug.isp.s3pdaf.disable", prop, "0");
	val = atoi(prop);

	if (val == 1) {
		ISP_LOGI("pd_otp_info_parser Disable!");
		return AFV1_SUCCESS;
	}

	struct sensor_otp_section_info *pdaf_otp_info_ptr = NULL;
	struct sensor_otp_section_info *module_info_ptr = NULL;

	cmr_u8 *module_info = NULL;

	if (NULL != in_p->otp_info_ptr) {
		if (in_p->otp_info_ptr->otp_vendor == OTP_VENDOR_SINGLE) {
			pdaf_otp_info_ptr = in_p->otp_info_ptr->single_otp.pdaf_info;
			module_info_ptr = in_p->otp_info_ptr->single_otp.module_info;
			ISP_LOGV("pass type1 pdaf otp, single cam");
		} else if (in_p->otp_info_ptr->otp_vendor == OTP_VENDOR_SINGLE_CAM_DUAL || in_p->otp_info_ptr->otp_vendor == OTP_VENDOR_DUAL_CAM_DUAL) {
			if (in_p->is_master == 1) {
				pdaf_otp_info_ptr = in_p->otp_info_ptr->dual_otp.master_pdaf_info;
				module_info_ptr = in_p->otp_info_ptr->dual_otp.master_module_info;
				ISP_LOGV("pass type1 pdaf otp, dual cam master");
			} else {
				pdaf_otp_info_ptr = NULL;
				module_info_ptr = NULL;
				ISP_LOGV("dual cam slave type1 pdaf is NULL");
			}
		}
	} else {
		pdaf_otp_info_ptr = NULL;
		module_info_ptr = NULL;
		ISP_LOGE("type1 pdaf otp_info_ptr is NULL");
	}

	if (NULL != pdaf_otp_info_ptr && NULL != module_info_ptr) {
		module_info = (cmr_u8 *) module_info_ptr->rdm_info.data_addr;

		if (NULL != pdaf_otp_info_ptr->rdm_info.data_addr && NULL != module_info) {
			if ((module_info[4] == 0 && module_info[5] == 1)
			    || (module_info[4] == 0 && module_info[5] == 2)
			    || (module_info[4] == 0 && module_info[5] == 3)
			    || (module_info[4] == 0 && module_info[5] == 4)
			    || (module_info[4] == 0 && module_info[5] == 5)
			    || (module_info[4] == 1 && module_info[5] == 0
				&& (module_info[0] != 0x53 || module_info[1] != 0x50 || module_info[2] != 0x52 || module_info[3] != 0x44))
			    || (module_info[4] == 2 && module_info[5] == 0)
			    || (module_info[4] == 3 && module_info[5] == 0)
			    || (module_info[4] == 4 && module_info[5] == 0)
			    || (module_info[4] == 5 && module_info[5] == 0)) {
				af->pdaf_rdm_otp_data = (cmr_u8 *) pdaf_otp_info_ptr->rdm_info.data_addr;
				ISP_LOGV("type1 pdaf otp map v0.4 or v0.5, Addr:%p", af->pdaf_rdm_otp_data);
			} else if (module_info[4] == 1 && module_info[5] == 0 && module_info[0] == 0x53 && module_info[1] == 0x50 && module_info[2] == 0x52
				   && module_info[3] == 0x44) {
				af->pdaf_rdm_otp_data = (cmr_u8 *) pdaf_otp_info_ptr->rdm_info.data_addr + 1;
				ISP_LOGV("type1 pdaf otp map v1.0, Addr:%p, 0:[%d]1:[%d]", af->pdaf_rdm_otp_data, *(af->pdaf_rdm_otp_data + 0), *(af->pdaf_rdm_otp_data + 1));
			} else {
				af->pdaf_rdm_otp_data = NULL;
				ISP_LOGE("type1 pdaf otp map version error");
			}
		} else {
			af->pdaf_rdm_otp_data = NULL;
			ISP_LOGE("type1 pdaf module_info is NULL");
		}

	} else {
		af->pdaf_rdm_otp_data = NULL;
		ISP_LOGE("type1 pdaf otp_info_ptr = %p, module_info_ptr = %p. Parser fail !", pdaf_otp_info_ptr, module_info_ptr);
	}

	return AFV1_SUCCESS;
}

static cmr_u8 set_bokeh_golden_data_info(af_ctrl_t * af, bokeh_golden_data_info * bokeh_golden_data)
{
	af->golden_data.golden_count = bokeh_golden_data->golden_count;
	af->golden_data.golden_macro = bokeh_golden_data->golden_macro;
	af->golden_data.golden_infinity = bokeh_golden_data->golden_infinity;
	ISP_LOGD("golden_macro=%d,golden_infinity=%d,golden_count=%d", af->golden_data.golden_macro, af->golden_data.golden_infinity, af->golden_data.golden_count);
	memcpy(&af->golden_data.golden_distance[0], &bokeh_golden_data->golden_distance[0], (bokeh_golden_data->golden_count) * sizeof(cmr_u16));
	memcpy(&af->golden_data.golden_vcm[0], &bokeh_golden_data->golden_vcm[0], (bokeh_golden_data->golden_count) * sizeof(cmr_u16));
	for (cmr_u8 i = 0; i < af->golden_data.golden_count; i++) {
		ISP_LOGD("golden_distance =%d,golden_vcm = %d", af->golden_data.golden_distance[i], af->golden_data.golden_vcm[i]);
	}
	return 0;
}

cmr_handle sprd_afv1_init(void *in, void *out)
{
	af_ctrl_t *af = NULL;
	char value[PROPERTY_VALUE_MAX] = { '\0' };
	ISP_LOGI("Enter");
	struct afctrl_init_in *init_param = (struct afctrl_init_in *)in;
	struct afctrl_init_out *result = (struct afctrl_init_out *)out;
	AF_OTP_Data otp_info;
	lens_range_info lens_range;
	bokeh_golden_data_info golden_data;
	struct af_sync_info af_sync;
	memset((void *)&otp_info, 0, sizeof(AF_OTP_Data));
	memset(&af_sync, 0, sizeof(struct af_sync_info));
	if (NULL == init_param) {
		ISP_LOGE("fail to init param:%p, result:%p", init_param, result);
		return NULL;
	}

	af = (af_ctrl_t *) malloc(sizeof(*af));
	if (NULL == af) {
		ISP_LOGE("fail to malloc af_ctrl_t");
		return NULL;
	}
	memset(af, 0, sizeof(*af));

	// parser af otp info
	af_otp_info_parser(init_param);

	// parser type1 pdaf otp info
	pd_otp_info_parser(af, init_param);

	init_param->otp_info.gldn_data.infinite_cali = 0;
	init_param->otp_info.gldn_data.macro_cali = 0;
	ISP_LOGV("af otp golden [%d %d]  rdm [%d %d]",
		 init_param->otp_info.gldn_data.infinite_cali, init_param->otp_info.gldn_data.macro_cali, init_param->otp_info.rdm_data.infinite_cali,
		 init_param->otp_info.rdm_data.macro_cali);

	if (NULL == init_param->aftuning_data || 0 == init_param->aftuning_data_len) {
		ISP_LOGE("fail to get sensor tuning param data");
		free(af);
		af = NULL;
		return NULL;
	}

	af->isp_info.width = init_param->src.w;
	af->isp_info.height = init_param->src.h;

#if defined(CONFIG_ISP_2_5) || defined(CONFIG_ISP_2_6) || defined(CONFIG_ISP_2_7)
	// sharkl3 & later arch with many rois
	af->isp_info.win_num = 10;	// keep invariant as original
#else
	af->isp_info.win_num = afm_get_win_num(init_param);
#endif

	af->caller = init_param->caller;
	af->otp_info.gldn_data.infinite_cali = init_param->otp_info.gldn_data.infinite_cali;
	af->otp_info.gldn_data.macro_cali = init_param->otp_info.gldn_data.macro_cali;
	af->otp_info.rdm_data.infinite_cali = init_param->otp_info.rdm_data.infinite_cali;
	af->otp_info.rdm_data.macro_cali = init_param->otp_info.rdm_data.macro_cali;
	af->is_multi_mode = init_param->is_multi_mode;
	af->pdaf_type = init_param->pdaf_type;
	af->cb_ops = init_param->cb_ops;

	// set tuning buffer pointer
	af->aftuning_data = init_param->aftuning_data;
	af->aftuning_data_len = init_param->aftuning_data_len;
	af->pdaftuning_data = init_param->pdaftuning_data;
	af->pdaftuning_data_len = init_param->pdaftuning_data_len;
	af->afttuning_data = init_param->afttuning_data;
	af->afttuning_data_len = init_param->afttuning_data_len;

	//[TOF_+++]
	af->toftuning_data = init_param->toftuning_data;
	af->toftuning_data_len = init_param->toftuning_data_len;
	//[TOF_---]

	af->camera_id = init_param->camera_id;
	af->bridge_ctrl = init_param->br_ctrl;
	af->pdaf_support = init_param->pdaf_support;
	af->slave_focus_cnt = 0;

	if (AF_ALG_DUAL_W_T == af->is_multi_mode) {
		if (1 == init_param->is_master) {
			af->sensor_role = AF_ROLE_MASTER;
		} else {
			af->sensor_role = AF_ROLE_SLAVE;
		}
	}

	af->bridge_ctrl(0, GET_AF_SYNC_INFO, NULL, &af_sync);
	if (AF_ALG_TRIBLE_W_T_UW == af->is_multi_mode) {
		if (af->camera_id == af_sync.cur_master_id) {
			af->sensor_role = AF_ROLE_MASTER;
		} else {
			af->sensor_role = AF_ROLE_SLAVE;
		}
		af->cur_master_id = af_sync.cur_master_id;
	}

	ISP_LOGI("is_multi_mode %d, cameraid %d, cur_master_id %d,sensor_role %d, pdaf_type %d", af->is_multi_mode,
		 af->camera_id, af_sync.cur_master_id, af->sensor_role, af->pdaf_type);
	ISP_LOGI("width = %d, height = %d, win_num = %d", af->isp_info.width, af->isp_info.height, af->isp_info.win_num);
	ISP_LOGV
	    ("module otp data (infi,macro) = (%d,%d), gldn (infi,macro) = (%d,%d)",
	     af->otp_info.rdm_data.infinite_cali, af->otp_info.rdm_data.macro_cali, af->otp_info.gldn_data.infinite_cali, af->otp_info.gldn_data.macro_cali);

	af->af_alg_cxt = af_lib_init(af);
	if (NULL == af->af_alg_cxt) {
		ISP_LOGE("fail to init lib func AF_init");
		free(af);
		af = NULL;
		return NULL;
	}

	af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_GET_LENS_RANGE, &lens_range);
	af->range_L1 = lens_range.range_L1;
	af->range_L4 = lens_range.range_L4;
	ISP_LOGD("af->range_L1 %d, af->range_L4 %d", af->range_L1, af->range_L4);

	if (trigger_init(af, CAF_TRIGGER_LIB) != 0) {
		ISP_LOGE("fail to init trigger");
		goto ERROR_INIT;
	}
	af->trigger_source_type = 0;

	af->pre_state = af->state = STATE_CAF;
	af->focus_state = AF_IDLE;

	af->ae_lock_num = 0;
	af->awb_lock_num = 0;
	af->lsc_lock_num = 0;
	af->nlm_lock_num = 0;
	af->ae_partial_lock_num = 0;

	af->afm_tuning.iir_level = 1;
	af->afm_tuning.nr_mode = 2;
	af->afm_tuning.cw_mode = 2;
	af->afm_tuning.fv0_e = 5;
	af->afm_tuning.fv1_e = 5;

	af->dcam_timestamp = 0xffffffffffffffff;
	af->test_loop_quit = 1;
	//property_set("vendor.cam.af_mode", "none");

	//[TOF_+++]
	af->tof.tof_trigger_flag = 0;
	//[TOF_---]
	af->motor_status = AF_MOTOR_IDLE;
	af->frame_counter = 0;

	result->log_info.log_cxt = (cmr_u8 *) af->af_alg_cxt;
	result->log_info.log_len = af->af_dump_info_len;

	af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_GET_OTP, &otp_info);
	af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_GET_BOKEH_GOLDEN_DATA, &golden_data);
	set_bokeh_golden_data_info(af, &golden_data);
	ISP_LOGI("otp bIsExist = %d, (inf, macro) = %d,%d.", otp_info.bIsExist, otp_info.INF, otp_info.MACRO);
	af->af_otp_type = otp_info.bIsExist;
	if (T_LENS_BY_OTP != otp_info.bIsExist) {
		af->otp_info.rdm_data.infinite_cali = otp_info.INF;
		af->otp_info.rdm_data.macro_cali = otp_info.MACRO;
	}

	property_get("persist.vendor.cam.isp.af.bypass", value, "0");
	af->bypass = ! !atoi(value);
	ISP_LOGV("property af bypass %s[%d]", value, ! !atoi(value));

	ISP_LOGI("Exit");
	return (cmr_handle) af;

ERROR_INIT:
	af->af_ops.deinit(af->af_alg_cxt);
	unload_af_lib(af);
	memset(af, 0, sizeof(*af));
	free(af);
	af = NULL;

	return (cmr_handle) af;
}

cmr_s32 sprd_afv1_deinit(cmr_handle handle, void *param, void *result)
{
	UNUSED(param);
	UNUSED(result);
	ISP_LOGI("Enter");
	af_ctrl_t *af = (af_ctrl_t *) handle;
	cmr_s32 rtn = AFV1_SUCCESS;
	int w1 = 0, w2 = 0;

	rtn = _check_handle(handle);
	if (AFV1_SUCCESS != rtn) {
		ISP_LOGE("fail to check handle");
		return AFV1_ERROR;
	}

	w1 = property_set("vendor.cam.af_mode", "none");
	ISP_LOGI("w1=%d", w1);
	if (w1) {
		ISP_LOGE("change af_mode to none fail");
	}
	w2 = property_set("vendor.cam.af_set_pos", "none");
	if (w2) {
		ISP_LOGE("set af_pos to none fail");
	}
	if (0 == af->test_loop_quit) {
		af->test_loop_quit = 1;
		pthread_join(af->test_loop_handle, NULL);
		af->test_loop_handle = 0;
	}

	afm_disable(af);
	trigger_deinit(af);

	af->af_ops.deinit(af->af_alg_cxt);
	unload_af_lib(af);

	memset(af, 0, sizeof(*af));
	free(af);
	af = NULL;
	ISP_LOGI("Exit");
	return rtn;
}

cmr_s32 sprd_afv1_process(cmr_handle handle, void *in, void *out)
{
	char value[PROPERTY_VALUE_MAX] = { '\0' };
	char *take_value = NULL;
	cmr_u32 property_id = 0;
	cmr_u32 property_dac = 0;
	af_ctrl_t *af = (af_ctrl_t *) handle;
	struct afctrl_calc_in *inparam = (struct afctrl_calc_in *)in;
	nsecs_t system_time0 = 0;
	nsecs_t system_time1 = 0;
	nsecs_t system_time_trigger = 0;
	nsecs_t system_time_force = 0;
	cmr_s32 rtn = AFV1_SUCCESS;
	UNUSED(out);
	struct af_status_info status_info;
	struct af_status_info status_master;
	struct aft_proc_result sync_result;
	struct af_sync_info af_sync;
	cmr_u32 pd_workable = 0;
	cmr_u16 i = 0, max_index = 0;
	cmr_u32 max_area = 0, area = 0;

	memset(&status_info, 0, sizeof(struct af_status_info));
	memset(&status_master, 0, sizeof(struct af_status_info));
	memset(&sync_result, 0, sizeof(struct aft_proc_result));
	memset(&af_sync, 0, sizeof(struct af_sync_info));

	rtn = _check_handle(handle);
	if (AFV1_SUCCESS != rtn) {
		ISP_LOGE("fail to check handle");
		return AFV1_ERROR;
	}

	if (NULL == inparam || (AF_DATA_AF == inparam->data_type && NULL == inparam->data)) {
		ISP_LOGE("fail to get input param data");
		return AFV1_ERROR;
	}

	property_get("debug.isp.af.idpos", value, "0");
	take_value = strtok(value, ":");
	if (take_value != NULL) {
		property_id = atoi(take_value);
		take_value = strtok(NULL, ":");
	}
	if (take_value != NULL) {
		property_dac = atoi(take_value);
		take_value = strtok(NULL, ":");
	}
	if (af->camera_id == property_id && 0 != property_dac) {
		lens_move_to(af, property_dac);
		return 0;
	}

	if (AF_DATA_IMG_BLK == inparam->data_type) {
		if (AF_MOTOR_SET == af->motor_status) {
			af->frame_counter++;
			if (3 == af->frame_counter) {
				af->frame_counter = 0;
				af->motor_status = AF_MOTOR_DONE;
				if (NULL != af->cb_ops.af_set_motor_status) {
					af->cb_ops.af_set_motor_status(af->caller, &af->motor_status);
				}
				af->motor_status = AF_MOTOR_IDLE;
			}
		}
	}

	if (1 == af->bypass) {
		return 0;
	}

	system_time0 = systemTime(CLOCK_MONOTONIC);
	ATRACE_BEGIN(__FUNCTION__);
	ISP_LOGV("state = %s, focus_state = %s, data_type %d", STATE_STRING(af->state), FOCUS_STATE_STR(af->focus_state), inparam->data_type);
	switch (inparam->data_type) {
	case AF_DATA_AF:
		afm_set_fv(af, inparam->data);
		af->trigger_source_type |= AF_DATA_AF;
		break;
	case AF_DATA_IMG_BLK:
		if (STATE_CAF == af->state || STATE_RECORD_CAF == af->state || STATE_FAF == af->state || STATE_NORMAL_AF == af->state) {
			caf_monitor_process(af);
		}
		break;
	default:
		ISP_LOGV("unsupported data type: %d", inparam->data_type);
		rtn = AFV1_ERROR;
		break;
	}
	ATRACE_END();
	system_time_trigger = systemTime(CLOCK_MONOTONIC);
	ISP_LOGV("SYSTEM_TEST-trigger:%dus", (cmr_s32) ((system_time_trigger - system_time0) / 1000));

	if (af->bridge_ctrl != NULL && (AF_ALG_TRIBLE_W_T_UW == af->is_multi_mode || AF_ALG_DUAL_W_T == af->is_multi_mode)
	    && af->sensor_role != AF_ROLE_MASTER && STATE_NORMAL_AF != af->state) {
		af->bridge_ctrl(AF_ROLE_MASTER, GET_AF_STATUS_INFO, NULL, &status_master);
		ISP_LOGV("camera_id %d, mode%d, state%d, status%d, pos%d", af->camera_id, status_master.af_mode, status_master.af_state, status_master.af_status,
			 status_master.af_position);
		if (AF_SEARCHING == status_master.af_status && (AF_IDLE == af->focus_state || AF_STOPPED == af->focus_state)
		    && af->slave_focus_cnt == 0) {
			switch (status_master.af_state) {
			case STATE_NORMAL_AF:
				break;
			case STATE_CAF:
			case STATE_RECORD_CAF:
				af->trig_ops.ioctrl(af->trig_ops.handle, AFT_CMD_GET_PD_WORKABLE, &pd_workable, NULL);
				sync_result.is_cancel_caf = AFT_CANC_NONE;
				sync_result.is_need_rough_search = 0;
				if (AFV1_TRUE == pd_workable) {
					sync_result.is_caf_trig = AFT_TRIG_PD;
					pd_start(af, AF_TRIGGER, &sync_result);
				} else {
					sync_result.is_caf_trig = AFT_TRIG_CB;
					caf_start(af, &sync_result);
				}
				af->focus_state = AF_SEARCHING;
				break;
			case STATE_FAF:
				ISP_LOGI("face->face_num =%d", af->face_info.face_num);
				if (0 == af->face_info.face_num) {
					af->trig_ops.ioctrl(af->trig_ops.handle, AFT_CMD_GET_PD_WORKABLE, &pd_workable, NULL);
					if (AFV1_TRUE == pd_workable) {
						sync_result.is_caf_trig = AFT_TRIG_PD;
					} else {
						sync_result.is_caf_trig = AFT_TRIG_CB;
					}
					sync_result.is_cancel_caf = AFT_CANC_NONE;
					sync_result.is_need_rough_search = 0;
					caf_start(af, &sync_result);
					af->focus_state = AF_SEARCHING;
				} else if (af->face_info.face_num != 0) {
					for (i = 0; i < af->face_info.face_num; i++) {
						if (af->face_info.face_info[i].sx <= af->face_info.frame_width && af->face_info.face_info[i].ex <= af->face_info.frame_width &&
						    af->face_info.face_info[i].sy <= af->face_info.frame_height && af->face_info.face_info[i].ey <= af->face_info.frame_height) {
							area =
							    ABS(af->face_info.face_info[i].ex - af->face_info.face_info[i].sx) * ABS(af->face_info.face_info[i].ey -
																     af->face_info.face_info[i].sy);
							if (max_area < area) {
								max_index = i;
								max_area = area;
							}
						}
					}
					if (max_index == af->face_info.face_num || 0 == max_area)
						return rtn;

					af->win.win_num = 1;
					af->win.face[0].sx = af->face_info.face_info[max_index].sx;
					af->win.face[0].ex = af->face_info.face_info[max_index].ex;
					af->win.face[0].sy = af->face_info.face_info[max_index].sy;
					af->win.face[0].ey = af->face_info.face_info[max_index].ey;
					af->win.face[0].roll_angle = af->face_info.face_info[max_index].angle;
					af->roll_angle = af->win.face[0].roll_angle;

					if (af->roll_angle >= -180 && af->roll_angle <= 180) {
						if (af->roll_angle >= -45 && af->roll_angle <= 45) {
							af->f_orientation = FACE_UP;
						} else if ((af->roll_angle >= -180 && af->roll_angle <= -135) || (af->roll_angle >= 135 && af->roll_angle <= 180)) {
							af->f_orientation = FACE_DOWN;
						} else if (af->roll_angle > -135 && af->roll_angle < -45) {
							af->f_orientation = FACE_LEFT;
						} else if (af->roll_angle > 45 && af->roll_angle < 135) {
							af->f_orientation = FACE_RIGHT;
						}
					} else {
						af->f_orientation = FACE_NONE;
					}
					af->pre_state = af->state;
					af->state = STATE_FAF;
					faf_start(af, &af->win);
					af->focus_state = AF_SEARCHING;
				}
				break;
			default:
				break;
			}
			return rtn;
		} else if (AF_SEARCHING == status_master.af_status && AF_SEARCHING == af->focus_state) {
			af->slave_focus_cnt = 1;
		} else if ((AF_STOPPED == status_master.af_status || AF_STOPPED_INNER == status_master.af_status || AF_IDLE == status_master.af_status)
			   && (AF_IDLE == af->focus_state || AF_STOPPED == af->focus_state)) {
			af->slave_focus_cnt = 0;
		} else if ((AF_STOPPED == status_master.af_status || AF_STOPPED_INNER == status_master.af_status)
			   && AF_SEARCHING == af->focus_state) {
			af->slave_focus_cnt = 0;
			ISP_LOGI("sync stop af state %s", STATE_STRING(af->state));
			af_stop_search(af);
			return rtn;
		}

	}

	if (af->bridge_ctrl != NULL && (AF_ALG_DUAL_W_T == af->is_multi_mode || AF_ALG_TRIBLE_W_T_UW == af->is_multi_mode)) {
		af->bridge_ctrl(0, GET_AF_SYNC_INFO, NULL, &af_sync);
		if (af->cur_master_id != af_sync.cur_master_id) {
			af->cur_master_id = af_sync.cur_master_id;
			ISP_LOGI("af->cur_master_id : %d,af_sync.cur_master_id : %d", af->cur_master_id, af_sync.cur_master_id);
			if (af->camera_id == af_sync.cur_master_id) {
				af->sensor_role = AF_ROLE_MASTER;
				if (AF_SEARCHING != status_master.af_status) {
					trigger_start(af);
				} else if (AF_SEARCHING == status_master.af_status) {
					if (STATE_FAF != status_master.af_status) {
					} else if (STATE_FAF == status_master.af_status && STATE_FAF != af->state) {
						if (0 == af->face_info.face_num) {
							ISP_LOGI("face do not sync!");
						} else {
							af_stop_search(af);
							af->af_ops.calc(af->af_alg_cxt);
							for (i = 0; i < af->face_info.face_num; i++) {
								if (af->face_info.face_info[i].sx <= af->face_info.frame_width
								    && af->face_info.face_info[i].ex <= af->face_info.frame_width
								    && af->face_info.face_info[i].sy <= af->face_info.frame_height
								    && af->face_info.face_info[i].ey <= af->face_info.frame_height) {
									area =
									    ABS(af->face_info.face_info[i].ex - af->face_info.face_info[i].sx) * ABS(af->face_info.face_info[i].ey -
																		     af->face_info.face_info[i].sy);
									if (max_area < area) {
										max_index = i;
										max_area = area;
									}
								}
							}
							if (max_index == af->face_info.face_num || 0 == max_area)
								return rtn;

							af->win.win_num = 1;
							af->win.face[0].sx = af->face_info.face_info[max_index].sx;
							af->win.face[0].ex = af->face_info.face_info[max_index].ex;
							af->win.face[0].sy = af->face_info.face_info[max_index].sy;
							af->win.face[0].ey = af->face_info.face_info[max_index].ey;
							af->win.face[0].roll_angle = af->face_info.face_info[max_index].angle;

							af->roll_angle = af->win.face[0].roll_angle;
							if (af->roll_angle >= -180 && af->roll_angle <= 180) {
								if (af->roll_angle >= -45 && af->roll_angle <= 45) {
									af->f_orientation = FACE_UP;
								} else if ((af->roll_angle >= -180 && af->roll_angle <= -135) || (af->roll_angle >= 135 && af->roll_angle <= 180)) {
									af->f_orientation = FACE_DOWN;
								} else if (af->roll_angle > -135 && af->roll_angle < -45) {
									af->f_orientation = FACE_LEFT;
								} else if (af->roll_angle > 45 && af->roll_angle < 135) {
									af->f_orientation = FACE_RIGHT;
								}
							} else {
								af->f_orientation = FACE_NONE;
							}
							af->pre_state = af->state;
							af->state = STATE_FAF;
							faf_start(af, &af->win);
							af->focus_state = AF_SEARCHING;
						}
					}
				}
			} else {
				af->sensor_role = AF_ROLE_SLAVE;
			}
		}
	}

	system_time1 = systemTime(CLOCK_MONOTONIC);
	ATRACE_BEGIN(__FUNCTION__);
	if (AF_DATA_AF == inparam->data_type) {
		af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_SET_PRE_TRIGGER_DATA, NULL);
		switch (af->state) {
		case STATE_NORMAL_AF:
		case STATE_FAF:
		case STATE_OTAF:
			if (AF_SEARCHING == af->focus_state) {
				af_process_frame(af);
			}
			break;
		case STATE_FULLSCAN:
		case STATE_CAF:
		case STATE_RECORD_CAF:
			if (AF_SEARCHING == af->focus_state) {
				af_process_frame(af);
			} else {
				//af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_SET_PRE_TRIGGER_DATA, NULL);
			}
			break;
		case STATE_ENGINEER:
			af->af_ops.calc(af->af_alg_cxt);
			break;
		default:
			ISP_LOGW("af->state %s is not supported", STATE_STRING(af->state));
			break;
		}
	}
	ATRACE_END();
	system_time_force = systemTime(CLOCK_MONOTONIC);
	ISP_LOGV("SYSTEM_TEST-af:%dus", (cmr_s32) ((system_time_force - system_time1) / 1000));

#ifdef Enable_mlog_AFtime
	char value[PROPERTY_VALUE_MAX] = { '\0' };

	property_get("persist.vendor.cam.AFTIME.enable", value, "0");

	if (atoi(value) == 1) {

		FILE *pf = NULL;
		const char saveLogFile[50] = "/data/mlog/aftime.txt";
		pf = fopen(saveLogFile, "wb");
		if (NULL != pf) {

			fprintf(pf, "\n\n\n");
			fprintf(pf, "TYPE:%s\n\n", af->AFtime.AF_type);
			fprintf(pf, "FOCUS TIME:%ld ms\n", (unsigned long)af->AFtime.time_total);
			fclose(pf);
		}
	}
#endif
	if (af->bridge_ctrl != NULL && (AF_ALG_DUAL_W_T == af->is_multi_mode || AF_ALG_TRIBLE_W_T_UW == af->is_multi_mode)
	    && AF_ROLE_MASTER == af->sensor_role) {
		status_info.af_mode = af->request_mode;
		status_info.af_state = af->state;
		status_info.af_status = af->focus_state;
		status_info.af_position = af->lens.pos;
		af->bridge_ctrl(AF_ROLE_MASTER, SET_AF_STATUS_INFO, &status_info, NULL);
	}
	return rtn;
}

cmr_s32 sprd_afv1_ioctrl(cmr_handle handle, cmr_s32 cmd, void *param0, void *param1)
{
	cmr_int rtn = AFV1_SUCCESS;
	rtn = _check_handle(handle);
	if (AFV1_SUCCESS != rtn) {
		ISP_LOGE("fail to check cxt");
		return AFV1_ERROR;
	}

	ISP_LOGV("cmd is 0x%x", cmd);
	if ((AF_CMD_SET_BASE < cmd) && (AF_CMD_SET_MAX > cmd))
		rtn = af_sprd_adpt_inctrl(handle, cmd, param0, param1);
	else if ((AF_CMD_GET_BASE < cmd) && (AF_CMD_GET_MAX > cmd))
		rtn = af_sprd_adpt_outctrl(handle, cmd, param0, param1);
	ISP_LOGV("rtn %ld", rtn);
	return rtn;
}

struct adpt_ops_type af_sprd_adpt_ops_ver1 = {
	.adpt_init = sprd_afv1_init,
	.adpt_deinit = sprd_afv1_deinit,
	.adpt_process = sprd_afv1_process,
	.adpt_ioctrl = sprd_afv1_ioctrl,
};
