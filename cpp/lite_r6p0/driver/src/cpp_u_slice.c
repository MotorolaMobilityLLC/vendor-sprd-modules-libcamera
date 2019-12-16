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
#define LOG_TAG "cpp_u_slice"

#include "cpp_u_slice.h"

#define SCALE_FRAME_WIDTH_MAX          8192
#define SCALE_FRAME_HEIGHT_MAX         8192
#define SCALE_FRAME_WIDTH_MIN               64
#define SCALE_FRAME_HEIGHT_MIN              32
#define BP_TRIM_SIZE_MIN			32
#define BP_TRIM_SIZE_MAX		8192
#define SCALE_WIDTH_MIN			64
#define SCALE_HEIGHT_MIN			32
#define SCALE_SLICE_OUT_WIDTH_MAX 2400
#define SCALE_FRAME_OUT_WIDTH_MAX           768
#define SCALE_DECI_FAC_MAX                  3

int cpp_u_input_param_check(struct sprd_cpp_scale_cfg_parm *cfg_parm)
{
	int ret = CMR_CAMERA_INVALID_PARAM;

	if (!cfg_parm) {
		CMR_LOGE("fail to get valid input ptr\n");
		return ret;
	}

	CMR_LOGV("input_addr 0x%x, 0x%x, 0x%x input_addr_vir 0x%x, 0x%x,0x%x\n",
			cfg_parm->input_addr.y, cfg_parm->input_addr.u,
			cfg_parm->input_addr.v, cfg_parm->input_addr_vir.y,
			cfg_parm->input_addr_vir.u, cfg_parm->input_addr_vir.v);
	CMR_LOGV("output_addr 0x%x, 0x%x, 0x%x output_addr_vir 0x%x, 0x%x,0x%x\n",
			cfg_parm->output_addr.y, cfg_parm->output_addr.u,
			cfg_parm->output_addr.v, cfg_parm->output_addr_vir.y,
			cfg_parm->output_addr_vir.u, cfg_parm->output_addr_vir.v);
 
	CMR_LOGV("in_size %d %d in_rect %d %d %d %d out_size %d %d\n",
			cfg_parm->input_size.w, cfg_parm->input_size.h,
			cfg_parm->input_rect.x, cfg_parm->input_rect.y,
			cfg_parm->input_rect.w, cfg_parm->input_rect.h,
			cfg_parm->output_size.w, cfg_parm->output_size.h);
	CMR_LOGV("schor %d  scver %d,input fmt %d,inputyendian %d\n",
			cfg_parm->scale_deci.hor, cfg_parm->scale_deci.ver,
			cfg_parm->input_format,
			cfg_parm->input_endian.y_endian);
	CMR_LOGV("scoutfmt %d, yendian %d, sctrim x%d y%d w%d h%d\n",
			cfg_parm->output_format,
			cfg_parm->output_endian.y_endian,
			cfg_parm->sc_trim.x, cfg_parm->sc_trim.y,
			cfg_parm->sc_trim.w, cfg_parm->sc_trim.h);
	CMR_LOGV("bptrim x%d y%d w%d h%d\n",
			cfg_parm->bp_trim.x, cfg_parm->bp_trim.y,
			cfg_parm->bp_trim.w, cfg_parm->bp_trim.h);

	/* sc_trim */
	if ((cfg_parm->sc_trim.w == 0) ||
			(cfg_parm->sc_trim.h == 0)) {
		cfg_parm->sc_trim.w = cfg_parm->input_rect.w >>
			cfg_parm->scale_deci.hor;
		cfg_parm->sc_trim.w = ALIGN_DOWN(cfg_parm->sc_trim.w, 2);
		cfg_parm->sc_trim.h = cfg_parm->input_rect.h >>
			cfg_parm->scale_deci.ver;
		cfg_parm->sc_trim.h = ALIGN_DOWN(cfg_parm->sc_trim.h, 2);
		cfg_parm->sc_trim.x = 0;
		cfg_parm->sc_trim.y = 0;
	CMR_LOGV("sctrim x%d y%d w%d h%d\n",
			cfg_parm->sc_trim.x, cfg_parm->sc_trim.y,
			cfg_parm->sc_trim.w, cfg_parm->sc_trim.h);
	}

	if (cfg_parm->input_size.w > SCALE_FRAME_WIDTH_MAX ||
			cfg_parm->input_size.h > SCALE_FRAME_HEIGHT_MAX ||
			cfg_parm->input_size.w < SCALE_FRAME_WIDTH_MIN ||
			cfg_parm->input_size.h < SCALE_FRAME_HEIGHT_MIN) {
		CMR_LOGE("fail to get valid src size:%d %d\n",
				cfg_parm->input_size.w, cfg_parm->input_size.h);
		return ret;
	} else if (cfg_parm->input_rect.w < SCALE_WIDTH_MIN ||
			cfg_parm->input_rect.h < SCALE_HEIGHT_MIN) {
		CMR_LOGE("fail to get valid rect1 %d %d %d %d\n",
				cfg_parm->input_rect.x, cfg_parm->input_rect.y,
				cfg_parm->input_rect.w, cfg_parm->input_rect.h);
		return ret;
	} else if (cfg_parm->input_size.w < cfg_parm->input_rect.w +
			cfg_parm->input_rect.x ||
			cfg_parm->input_size.h < cfg_parm->input_rect.h +
			cfg_parm->input_rect.y) {
		CMR_LOGE("fail to get valid rect2 %d %d %d %d %d %d\n",
				cfg_parm->input_size.w, cfg_parm->input_size.h,
				cfg_parm->input_rect.x, cfg_parm->input_rect.y,
				cfg_parm->input_rect.w, cfg_parm->input_rect.h);
		return ret;
	} else if (cfg_parm->sc_trim.w < SCALE_WIDTH_MIN ||
			cfg_parm->sc_trim.h < SCALE_HEIGHT_MIN ||
			cfg_parm->sc_trim.w > cfg_parm->input_rect.w ||
			cfg_parm->sc_trim.h > cfg_parm->input_rect.h ||
			((cfg_parm->sc_trim.x + cfg_parm->sc_trim.w) >
			 (cfg_parm->input_rect.w >>
			  cfg_parm->scale_deci.hor)) ||
			((cfg_parm->sc_trim.y + cfg_parm->sc_trim.h) >
			 (cfg_parm->input_rect.h >>
			  cfg_parm->scale_deci.ver))) {
		CMR_LOGE("fail to get valid trim size %d %d %d %d\n",
				cfg_parm->sc_trim.x, cfg_parm->sc_trim.y,
				cfg_parm->sc_trim.w, cfg_parm->sc_trim.h);
		return ret;
	} else if (cfg_parm->scale_deci.hor > SCALE_DECI_FAC_MAX ||
			cfg_parm->scale_deci.ver > SCALE_DECI_FAC_MAX) {
		CMR_LOGE("fail to get deci value %d %d\n",
				cfg_parm->scale_deci.hor,
				cfg_parm->scale_deci.ver);
		return ret;
	} else if ((cfg_parm->sc_trim.w > cfg_parm->output_size.w * 4) ||
			(cfg_parm->sc_trim.w * 4 < cfg_parm->output_size.w)) {
		CMR_LOGE("vaild scaler ratio, scaler ratio is 1/4~4\n");
		return ret;
	}

	/* input/output align */
	if (MOD(cfg_parm->input_size.w, 8) != 0) {
		CMR_LOGE("fail to get src scale pitch size %d\n",
				cfg_parm->input_size.w);
		return ret;
	}

	if (cfg_parm->input_format == SCALE_YUV420) {
		if ((MOD2(cfg_parm->input_size.w) != 0) ||
				(MOD2(cfg_parm->input_size.h) != 0) ||
				(MOD2(cfg_parm->input_rect.h) != 0) ||
				(MOD2(cfg_parm->input_rect.y) != 0)) {
			CMR_LOGE("failed input size rect align size\n");
			return ret;
		}
	}

	if ((MOD2(cfg_parm->input_rect.x) != 0) ||
			(cfg_parm->input_rect.w % 2 != 0)) {
		CMR_LOGE("failed input  rect align size2\n");
		return ret;
	}

	if (MOD8(cfg_parm->output_pitch) != 0) {
		CMR_LOGE("fail to get sc dst pitch align size not 8: %d\n",
				cfg_parm->output_pitch);
		return ret;
	}

	if (MOD2(cfg_parm->output_size.w) != 0) {
		CMR_LOGE("fail to get dst width align 2: %d\n",
				cfg_parm->output_size.w);
		return ret;
	}

	if (cfg_parm->output_format == SCALE_YUV420) {
		if (MOD(cfg_parm->output_size.h, 4) != 0) {
			CMR_LOGE("fail to get dst height align 4: %d\n",
					cfg_parm->output_size.h);
			return ret;
		}
	} else {
		if (MOD2(cfg_parm->output_size.h) != 0) {
			CMR_LOGE("fail to get dst height align 2: %d\n",
					cfg_parm->output_size.h);
			return ret;
		}
	}

	if ((MOD2(cfg_parm->sc_trim.x) != 0) ||
			(MOD2(cfg_parm->sc_trim.w) != 0) ||
			(MOD2(cfg_parm->sc_trim.y) != 0) ||
			(MOD2(cfg_parm->sc_trim.h) != 0)) {
		CMR_LOGE("failed sc trim align size\n");
		return ret;
	}

	/* bp param check */
	if ((cfg_parm->scale_mode == SCALE_MODE_2OUT) &&
			((cfg_parm->output_size.w >
			  SCALE_SLICE_OUT_WIDTH_MAX) ||
			 (cfg_parm->scale_deci.hor > 0) ||
			 cfg_parm->scale_deci.ver > 0)) {
		CMR_LOGE("got vaild scale mode failed bp enable check");
		return ret;
	}

	if ((cfg_parm->bp_trim.x > SCALE_FRAME_WIDTH_MAX) ||
			(cfg_parm->bp_trim.y > SCALE_FRAME_HEIGHT_MAX) ||
			(cfg_parm->bp_trim.w > SCALE_FRAME_WIDTH_MAX) ||
			(cfg_parm->bp_trim.h > SCALE_FRAME_HEIGHT_MAX) ||
			(cfg_parm->input_rect.w < cfg_parm->bp_trim.x +
			 cfg_parm->bp_trim.w) ||
			(cfg_parm->input_rect.h < cfg_parm->bp_trim.y +
			 cfg_parm->bp_trim.h)) {
		CMR_LOGE("got vaild bp trim size\n");
		return ret;
	}

	if ((MOD2(cfg_parm->bp_trim.x) != 0) ||
			(MOD2(cfg_parm->bp_trim.w) != 0)) {
		CMR_LOGE("failed bp trim align size1\n");
		return ret;
	}

	if (MOD8(cfg_parm->bpout_pitch) != 0) {
		CMR_LOGE("fail to get bp dst pitch align size not 8: %d\n",
				cfg_parm->bpout_pitch);
		return ret;
	}

	if (cfg_parm->input_format == SCALE_YUV420) {
		if ((MOD2(cfg_parm->bp_trim.y) != 0) ||
				(MOD2(cfg_parm->bp_trim.h) != 0)) {
			CMR_LOGE("failed bp trim align size2\n");
		return ret;
		}
	}
	CMR_LOGV("2in_size %d %d in_rect %d %d %d %d out_size %d %d\n",
			cfg_parm->input_size.w, cfg_parm->input_size.h,
			cfg_parm->input_rect.x, cfg_parm->input_rect.y,
			cfg_parm->input_rect.w, cfg_parm->input_rect.h,
			cfg_parm->output_size.w, cfg_parm->output_size.h);
	CMR_LOGV("2schor %d  scver %d,input fmt %d,inputyendian %d\n",
			cfg_parm->scale_deci.hor, cfg_parm->scale_deci.ver,
			cfg_parm->input_format,
			cfg_parm->input_endian.y_endian);
	CMR_LOGV("2scoutfmt %d, yendian %d, sctrim x%d y%d w%d h%d\n",
			cfg_parm->output_format,
			cfg_parm->output_endian.y_endian,
			cfg_parm->sc_trim.x, cfg_parm->sc_trim.y,
			cfg_parm->sc_trim.w, cfg_parm->sc_trim.h);
	CMR_LOGV("2check finished bptrim x%d y%d w%d h%d\n",
			cfg_parm->bp_trim.x, cfg_parm->bp_trim.y,
			cfg_parm->bp_trim.w, cfg_parm->bp_trim.h);
	return CMR_CAMERA_SUCCESS;
}

/* convert_param_to_calc used to */
/* translate parm from drv to algorithms */
void convert_param_to_calc(struct sprd_cpp_scale_cfg_parm *cfg_parm,
	struct sprd_cpp_scale_slice_parm *slice_parm)
{
	int slice_count = 0;

	slice_parm->crop_en = 1;
	slice_parm->scaler_path_param.scaler_en = 1;
	slice_parm->scaler_path_param.trim_eb = 1;
	slice_parm->bypass_path_param.trim_eb = 1;
	
	if (cfg_parm->scale_mode == SCALE_MODE_2OUT)
		slice_parm->bypass_path_param.bypass_eb = 1;
	
	slice_parm->img_w = cfg_parm->input_size.w;
	slice_parm->img_h = cfg_parm->input_size.h;
	slice_parm->img_format = cfg_parm->input_format;
	slice_parm->crop_start_x = cfg_parm->input_rect.x;
	slice_parm->crop_start_y = cfg_parm->input_rect.y;
	slice_parm->crop_width = cfg_parm->input_rect.w;
	slice_parm->crop_height = cfg_parm->input_rect.h;

	if (cfg_parm->output_size.w <= SCALE_SLICE_OUT_WIDTH_MAX)
		slice_parm->slice_w = 0;
	else {
		//slice_parm->bypass_path_param.enable = 0;
		slice_count = ALIGN_UP((cfg_parm->output_size.w /
			SCALE_SLICE_OUT_WIDTH_MAX + (cfg_parm->output_size.w %
			SCALE_SLICE_OUT_WIDTH_MAX ? 1 : 0)), 2);
		slice_parm->slice_w = cfg_parm->input_rect.w /
			slice_count;
	}
	CMR_LOGD("slice_w %d\n", slice_parm->slice_w);
	slice_parm->deci_param.hor =cfg_parm->scale_deci.hor;
	slice_parm->deci_param.ver =cfg_parm->scale_deci.ver;
	
	slice_parm->scaler_path_param.scaler_init_phase_hor = 0;
	slice_parm->scaler_path_param.scaler_output_format =
		cfg_parm->output_format;
	slice_parm->scaler_path_param.scaler_des_size_x =
		cfg_parm->output_size.w;
	slice_parm->scaler_path_param.scaler_des_size_y =
		cfg_parm->output_size.h;
	slice_parm->scaler_path_param.scaler_des_pitch =
		cfg_parm->output_pitch;
	slice_parm->scaler_path_param.trim_size_x =
		cfg_parm->sc_trim.w;
	slice_parm->scaler_path_param.trim_size_y =
		cfg_parm->sc_trim.h;
	slice_parm->scaler_path_param.trim_start_x =
		cfg_parm->sc_trim.x;
	slice_parm->scaler_path_param.trim_start_y =
		cfg_parm->sc_trim.y;
	slice_parm->bypass_path_param.trim_size_x =
		cfg_parm->bp_trim.w;
	slice_parm->bypass_path_param.trim_size_y =
		cfg_parm->bp_trim.h;
	slice_parm->bypass_path_param.trim_start_x =
		cfg_parm->bp_trim.x;
	slice_parm->bypass_path_param.trim_start_y =
		cfg_parm->bp_trim.y;
	slice_parm->bypass_path_param.bp_des_pitch =
		cfg_parm->bpout_pitch;
}
