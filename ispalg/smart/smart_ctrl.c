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
#define LOG_TAG "smart_ctrl"
#include <math.h>
#include "smart_ctrl.h"
#include "isp_pm.h"
#include "lsc_adv.h"
#include "lib_ctrl.h"
#include "sensor_drv_u.h"
#include "debug_file.h"
#if __STDC_VERSION__ >= 199901L
#include <stdbool.h>
#else
typedef int bool;
#define false   (0)
#define true    (1)
#endif
#include "atm.h"
#include "isp_atm.h"
#include "isp_com_alg.h"
#include <stdio.h>
#include <cutils/properties.h>

#define ISP_SMART_MAGIC_FLAG 0xf7758521
#define array_size(array) (sizeof(array) / sizeof(array[0]))

#define DEBUG_BUF_SIZE (5 * 1024)
#define DEBUG_FILE_NAME "/data/mlog/smart.txt"

#define GATM_ENABLE 1

#ifdef ISP_LOGV
#undef ISP_LOGV
static cmr_int g_smart_log_level = LEVEL_OVER_LOGD;
extern long g_isp_log_level;
#define ISP_LOGV(format, ...)                                                  \
	ALOGD_IF((((g_smart_log_level >= LEVEL_OVER_LOGV)||(g_isp_log_level >= LEVEL_OVER_LOGV))&&(g_smart_log_level!=6)), DEBUG_STR format, DEBUG_ARGS, \
        ##__VA_ARGS__)
#endif

#define SENSOR_NUM 5
#define SMART_STASH_FILE "data/vendor/cameraserver/smart.file"

struct block_name_map {
	cmr_u32 block_id;
	char name[8];
};

struct tuning_param {
	cmr_u32 version;
	cmr_u32 bypass;
	struct isp_smart_param param;
};

static const char *s_smart_block_name[] = {
	"lsc",
	"color_cast",
	"cmc",
	"sat_depress",
	"hsv",
	"gamma",
	"gain_offset",
	"blc",
	"post_blc",
	"pdaf",
	"nlm",
	"rgb_dither",
	"bpc",
	"grgb",
	"cfae",
	"rgb_afm",
	"uvdiv",
	"3dnr_pre",
	"3dnr_cap",
	"edge",
	"yuv_precdn",
	"ynr",
	"uvcdn",
	"uv_postcdn",
	"iircnr_iir",
	"iir_yrandom",
	"yuv_noisefilter",
	"rgb_precdn",
	"bdn",
	"prfy",
	"cnr2",
	"imbalance",
	"ltm",
	"3dnr",
	"mfnr",

	"unkown"
};

struct smart_stash{
	cmr_s32 bv;
	cmr_s32 bv_gain;
	cmr_u32 ct;
};

struct smart_context {
	cmr_u32 magic_flag;
	pthread_mutex_t status_lock;
	struct atmctrl_cxt *handle_atm;
	struct tuning_param tuning_param[SMART_MAX_WORK_MODE];
	struct tuning_param tuning_param_org[SMART_MAX_WORK_MODE];
	struct tuning_param *cur_param;
	cmr_u32 work_mode;
	enum smart_ctrl_flash_mode flash_mode;
	struct smart_block_result calc_result[ISP_SMART_MAX_BLOCK_NUM];
	struct smart_block_result block_result;
	cmr_u8 debug_buf[DEBUG_BUF_SIZE];
	debug_handle_t debug_file;
	struct nr_data nr_param;
	cmr_handle caller_handle;
	isp_smart_cb smart_set_cb;

	cmr_u32 camera_id;
	struct smart_stash smart_stash_param;
	cmr_u32 smart_lock_frame;
    // new debug
	smart_debuginfo smt_dbginfo;
	cmr_u32 atm_version;
	struct atm_tune_param atm_tuning_param;
	struct ae_atm_tune_param_v1 atm_tuning_param_v1;
	enum smart_ctrl_atm_switch_state atm_switch_state;

};

static cmr_s32 is_print_log(void)
{
	char value[PROPERTY_VALUE_MAX] = { 0 };
	cmr_u32 is_print = 0;

	property_get("debug.camera.isp.smart", value, "0");

	if (!strcmp(value, "1")) {
		is_print = 1;
	}

	return is_print;
}

static cmr_s32 smart_ctl_save_stash(struct smart_context *cxt, void *in_param)
{
	UNUSED(in_param);
	int count = 0;
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 i = 0;
	cmr_u32 smart_camera_id = 0;
	cmr_s32 smart_stash_s[SENSOR_NUM][3];

	if (NULL == cxt) {
		ISP_LOGE("fail to get valid input param, in: %p\n", cxt);
		goto ERROR_EXIT;
	}

	ISP_LOGD("save_smart  camera_id = %d",cxt->camera_id);

	smart_camera_id = cxt->camera_id;

	if(smart_camera_id >= SENSOR_NUM){
		goto ERROR_EXIT;
	}

	memset((cmr_s32*)smart_stash_s, 0, sizeof(smart_stash_s));
	char file_name[1024];
	FILE* fp = NULL;
	snprintf(file_name, sizeof(file_name),"%scamera_%d_smart.file", SMART_STASH_FILE, smart_camera_id);
	fp = fopen(file_name, "rb");
	if (fp) {
		ISP_LOGD("read smart  file");
		count = fread((cmr_s32*)smart_stash_s,1,sizeof(smart_stash_s), fp);
		if(count < sizeof(smart_stash_s))
			ISP_LOGV("smart_ctl_save_stash:fread count error!");
		fclose(fp);
		fp = NULL;

	}else{
		ISP_LOGE(" no smart  file");
		goto SAVE_BIN;
	}

SAVE_BIN:

	smart_stash_s[smart_camera_id][0] =cxt->smart_stash_param.bv;
	smart_stash_s[smart_camera_id][1] =cxt->smart_stash_param.bv_gain;
	smart_stash_s[smart_camera_id][2] =cxt->smart_stash_param.ct;

	fp = fopen(file_name, "wb");
	if (fp) {
		fwrite((cmr_s32*)smart_stash_s,1,sizeof(smart_stash_s), fp);
		fclose(fp);
		fp = NULL;
		ISP_LOGD("save smart file ok ");
	} else {
		ISP_LOGE("save smart file fail");
	}

	for(i = 0; i < 3;i++){
		ISP_LOGD("camera[%d] = smart_save[%d] = %d ",cxt->camera_id,i,smart_stash_s[cxt->camera_id][i]);
	}

ERROR_EXIT:

	return rtn;
}

static cmr_s32 smart_ctl_apply_stash(struct smart_context *cxt, void *in_param)
{
	UNUSED(in_param);
	int count = 0;
	cmr_s32 rtn = ISP_SUCCESS;

	cmr_u32 smart_camera_id = 0;
	cmr_s32 smart_stash_r[SENSOR_NUM][3];

	ISP_LOGV("apply smart");

	if (NULL == cxt) {
		ISP_LOGE("fail to get valid input param, in: %p\n", cxt);
		goto ERROR_EXIT;
	}

	smart_camera_id = cxt->camera_id;

	if(smart_camera_id >= SENSOR_NUM){
		goto ERROR_EXIT;
	}

	char file_name[1024];
	FILE* fp = NULL;
	snprintf(file_name, sizeof(file_name),"%scamera_%d_smart.file", SMART_STASH_FILE, smart_camera_id);
	fp = fopen(file_name, "rb");
	if (fp) {
		memset((void*)smart_stash_r, 0, sizeof(smart_stash_r));
		count = fread((cmr_s32*)smart_stash_r,1,sizeof(smart_stash_r), fp);
		if(count < sizeof(smart_stash_r))
			ISP_LOGV("smart_ctl_apply_stash:fread count error!");
		fclose(fp);
		fp = NULL;

	 cxt->smart_stash_param.bv = smart_stash_r[smart_camera_id][0];
	 cxt->smart_stash_param.bv_gain = smart_stash_r[smart_camera_id][1];
	 cxt->smart_stash_param.ct = smart_stash_r[smart_camera_id][2];

	} else {
		ISP_LOGE("no smart stash bin file");
		goto ERROR_EXIT;
	}

	ISP_LOGV("camera[%d].bv = %d ",smart_camera_id,smart_stash_r[smart_camera_id][0]);
	ISP_LOGV("camera[%d].bv_gain = %d ",smart_camera_id,smart_stash_r[smart_camera_id][1]);
	ISP_LOGV("camera[%d].ct = %d ",smart_camera_id,smart_stash_r[smart_camera_id][2]);


ERROR_EXIT:

	return rtn;
}


static cmr_s32 check_handle_validate(smart_handle_t handle)
{
	cmr_s32 ret = ISP_SUCCESS;
	struct smart_context *cxt_ptr = (struct smart_context *)handle;

	if (NULL == handle) {
		ISP_LOGE("fail to get valid handle\n");
		return ISP_ERROR;
	}

	if (ISP_SMART_MAGIC_FLAG != cxt_ptr->magic_flag) {
		ISP_LOGE("fail to get valid magic\n");
		return ISP_ERROR;
	}

	return ret;
}

static cmr_s32 smart_ctl_set_workmode(struct smart_context *cxt, void *in_param)
{
	cmr_s32 rtn = ISP_SUCCESS;

	if (NULL == in_param) {
		ISP_LOGE("fail to get valid in param");
		return ISP_ERROR;
	}

	cmr_u32 work_mode = *(cmr_u32 *) in_param;

	if (cxt->tuning_param[work_mode].param.block_num > 0) {
		cxt->work_mode = work_mode;
	} else {
	        ISP_LOGV("set common work mode");
		cxt->work_mode = ISP_MODE_ID_COMMON;
	}

	return rtn;
}

static cmr_s32 smart_ctl_set_flash(struct smart_context *cxt, void *in_param)
{
	cmr_s32 rtn = ISP_SUCCESS;
	enum smart_ctrl_flash_mode flash_mode = 0x0;

	if (NULL == in_param) {
		ISP_LOGE("fail to get valid in param");
		return ISP_ERROR;
	}

	flash_mode = *(enum smart_ctrl_flash_mode *)in_param;

	if (flash_mode >= SMART_CTRL_FLASH_END) {
		ISP_LOGE("fail to get valid flash mode");
		return ISP_ERROR;
	}
	cxt->flash_mode = flash_mode;

	return rtn;
}

static cmr_s32 smart_ctl_set_atm_switch_state(struct smart_context *cxt, void *in_param)
{
	cmr_s32 rtn = ISP_SUCCESS;
	enum smart_ctrl_atm_switch_state atm_switch_state = SMART_CTRL_ATM_SWITCH_ON;

	if (NULL == in_param) {
		ISP_LOGE("fail to get valid in param");
		return ISP_ERROR;
	}

	atm_switch_state = *(enum smart_ctrl_atm_switch_state *)in_param;

	if (atm_switch_state >= SMART_CTRL_ATM_SWITCH_MAX) {
		ISP_LOGE("fail to get valid switch state");
		return ISP_ERROR;
	}
	cxt->atm_switch_state = atm_switch_state;

	return rtn;
}

static cmr_s32 smart_ctl_check_block_param(struct isp_smart_block_cfg *blk_cfg)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_smart_component_cfg *comp_cfg = NULL;
	struct isp_piecewise_func *func = NULL;
	struct isp_range *bv_range = NULL;
	cmr_u32 i = 0;
	cmr_u32 k = 0;

	for (i = 0; i < blk_cfg->component_num; i++) {
		comp_cfg = &blk_cfg->component[i];

		if (blk_cfg->smart_id >= ISP_SMART_MAX) {
			ISP_LOGV("block[%d]: smart_id is invalid.\n", i);
			rtn = ISP_ERROR;
			return rtn;
		}

		if (blk_cfg->block_id >= ISP_BLK_ID_MAX) {
			ISP_LOGV("block[%d]: block_id is invalid.\n", i);
			rtn = ISP_ERROR;
			return rtn;
		}

		for (k = 0; k < comp_cfg->section_num; k++) {
			func = &comp_cfg->func[k];
			bv_range = &comp_cfg->bv_range[k];

			if (bv_range->min > bv_range->max) {
				ISP_LOGV("section[%d]: bv_range is invalid.\n", k);
				rtn = ISP_ERROR;
				return rtn;
			}

			if ((k < comp_cfg->section_num - 1) && (bv_range->max > comp_cfg->bv_range[k + 1].min)) {
				ISP_LOGV("section[%d]: bv_range.max=%d, section[%d]: bv_range.min=%d, bv_range is invalid.",
					 k, bv_range->max, k + 1, comp_cfg->bv_range[k + 1].min);
				rtn = ISP_ERROR;
				return rtn;
			}
		}
	}

	return rtn;
}

static cmr_s32 smart_ctl_check_param(struct isp_smart_param *param)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 i = 0;
	cmr_u32 j = 0;
	struct isp_smart_block_cfg *blk_cfg = NULL;
	struct isp_smart_component_cfg *comp_cfg = NULL;
	struct isp_piecewise_func *func = NULL;
	struct isp_range *bv_range = NULL;

	if (0 == is_print_log())
		return rtn;

	ISP_LOGV("block_num:%d.", param->block_num);

	for (i = 0; i < param->block_num; i++) {
		blk_cfg = &param->block[i];

		ISP_LOGV("block[%d], smart id=%x, block id=%x, enable=%d, comp num=%d", i, blk_cfg->smart_id, blk_cfg->block_id, blk_cfg->enable, blk_cfg->component_num);

		if (blk_cfg->smart_id > ISP_SMART_MAX) {
			ISP_LOGE("block[%d]: smart_id is invalid.\n", i);
			return ISP_ERROR;
		}

		if (blk_cfg->block_id > ISP_BLK_ID_MAX) {
			ISP_LOGE("block[%d]: block_id is invalid.\n", i);
			return ISP_ERROR;
		}

		for (j = 0; j < blk_cfg->component_num; j++) {
			cmr_u32 k = 0;
			cmr_u32 m = 0;

			comp_cfg = &blk_cfg->component[j];

			ISP_LOGV("component[%d], section num=%d", j, comp_cfg->section_num);

			if (ISP_SMART_X_TYPE_BV == comp_cfg->x_type) {
				ISP_LOGV("x_type: bv");
			} else if (ISP_SMART_X_TYPE_BV_GAIN == comp_cfg->x_type) {
				ISP_LOGV("x_type: bv gain");
			} else if (ISP_SMART_X_TYPE_CT == comp_cfg->x_type) {
				ISP_LOGV("x_type: ct");
			} else if (ISP_SMART_X_TYPE_BV_CT == comp_cfg->x_type) {
				ISP_LOGV("x_type: bv ct");
			}else if (ISP_SMART_X_TYPE_BV_ABLWEIGHT == comp_cfg->x_type)
				ISP_LOGV("x_type: bv abl");

			if (ISP_SMART_Y_TYPE_VALUE == comp_cfg->y_type) {
				ISP_LOGV("y_type: value");
			} else if (ISP_SMART_Y_TYPE_WEIGHT_VALUE == comp_cfg->y_type) {
				ISP_LOGV("y_type: weight value");
			}

			ISP_LOGV("use_flash_value = %d", comp_cfg->use_flash_val);
			ISP_LOGV("flash_value = %d", comp_cfg->flash_val);

			for (k = 0; k < comp_cfg->section_num; k++) {
				func = &comp_cfg->func[k];
				bv_range = &comp_cfg->bv_range[k];

				ISP_LOGV("section[%d], bv=[%d, %d], func num=%d", k, comp_cfg->bv_range[k].min, comp_cfg->bv_range[k].max, func->num);

				if (bv_range->min > bv_range->max) {
					ISP_LOGV("section[%d]: bv_range is invalid.\n", k);
				}

				if ((k + 1 < comp_cfg->section_num) && (bv_range->max > comp_cfg->bv_range[k + 1].min)) {
					ISP_LOGV("section[%d]: bv_range.max=%d, section[%d]: bv_range.min=%d, bv_range is invalid.",
						 k, bv_range->max, k + 1, comp_cfg->bv_range[k + 1].min);
				}

			}

			if (0 == comp_cfg->section_num) {
				func = &comp_cfg->func[0];
				ISP_LOGV("func num=%d", func->num);
				for (m = 0; m < func->num; m++) {
					ISP_LOGV("[%d]=(%4d, %4d)", m, func->samples[m].x, func->samples[m].y);
				}
			}
		}
	}

	return rtn;
}

static cmr_s32 smart_ctl_parse_tuning_param(struct smart_tuning_param src[], struct tuning_param dst[], cmr_u32 num)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 i = 0;

	for (i = 0; i < num; i++) {
		if (NULL != src[i].data.data_ptr && src[i].data.size > 0) {
			if (0 == src[i].version) {
				rtn = smart_ctl_check_param((struct isp_smart_param *)src[i].data.data_ptr);

				if (ISP_SUCCESS == rtn) {
					dst[i].bypass = src[i].bypass;
					dst[i].version = src[i].version;
					memcpy(&dst[i].param, src[i].data.data_ptr, src[i].data.size);
				} else {
					ISP_LOGE("i=%d,smart_ctl_check_param_fail\n",i);
                                	return ISP_ERROR;
				}
			}
		}
	}
	return ISP_SUCCESS;
}

static cmr_s32 smart_ctl_get_update_param(struct smart_context *cxt, void *in_param)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct smart_init_param *param = NULL;

	if (NULL == in_param) {
		ISP_LOGE("fail to get valid input param, in: %p\n", param);
		goto ERROR_EXIT;
	}

	param = (struct smart_init_param *)in_param;

	rtn = smart_ctl_parse_tuning_param(param->tuning_param, cxt->tuning_param, SMART_MAX_WORK_MODE);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to parse tuning param, rtn = %d", rtn);
		goto ERROR_EXIT;
	}

	cxt->cur_param = &cxt->tuning_param[cxt->work_mode];

ERROR_EXIT:

	return rtn;
}

static cmr_s32 smart_ctl_piecewise_func_v1(struct isp_piecewise_func *func, cmr_s32 x, cmr_u32 weight_unit, struct isp_weight_value *result)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 num = func->num;
	struct isp_sample *samples = func->samples;
	cmr_s16 y = 0;
	cmr_u32 i = 0;

	if (0 == num)
		return ISP_ERROR;

	if (x <= samples[0].x) {
		y = samples[0].y;
		result->value[0] = y;
		result->value[1] = y;
		result->weight[0] = weight_unit;
		result->weight[1] = 0;
	} else if (x >= samples[num - 1].x) {
		y = samples[num - 1].y;
		result->value[0] = y;
		result->value[1] = y;
		result->weight[0] = weight_unit;
		result->weight[1] = 0;
	} else {
		rtn = ISP_ERROR;

		for (i = 0; i < num - 1; i++) {
			if (x >= samples[i].x && x < samples[i + 1].x) {
				if (0 != samples[i + 1].x - samples[i].x) {
					result->value[0] = samples[i].y;
					result->value[1] = samples[i + 1].y;

					result->weight[0] = (samples[i + 1].x - x) * weight_unit / (samples[i + 1].x - samples[i].x);
					result->weight[1] = weight_unit - result->weight[0];
				} else {
					result->value[0] = samples[i].y;
					result->value[1] = samples[i].y;
					result->weight[0] = weight_unit;
					result->weight[1] = 0;
				}

				rtn = ISP_SUCCESS;
				break;
			}
		}
	}

	return rtn;
}

static cmr_s32 smart_ctl_piecewise_func_v0(struct isp_piecewise_func *func, cmr_s32 x, cmr_s32 * result)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 num = func->num;
	struct isp_sample *samples = func->samples;
	cmr_s32 y = 0;
	cmr_u32 i = 0;

	if (0 == num)
		return ISP_ERROR;

	if (x <= samples[0].x) {
		y = samples[0].y;
	} else if (x >= samples[num - 1].x) {
		y = samples[num - 1].y;
	} else {
		rtn = ISP_ERROR;

		for (i = 0; i < num - 1; i++) {
			if (x >= samples[i].x && x < samples[i + 1].x) {
				if (0 != samples[i + 1].x - samples[i].x)
					y = samples[i].y + (x - samples[i].x) * (samples[i + 1].y - samples[i].y)
					    / (samples[i + 1].x - samples[i].x);
				else
					y = samples[i].y;

				rtn = ISP_SUCCESS;
				break;
			}
		}
	}

	*result = y;

	return rtn;
}

static cmr_s32 smart_crl_calc_func(struct isp_piecewise_func *func, cmr_u32 y_type, cmr_s32 x, struct isp_weight_value *result)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_s32 value = 0;

	if (NULL == func) {
		ISP_LOGI("fail_calc_func_null!\n");
		return ISP_ERROR;
	}

	switch (y_type) {
	case ISP_SMART_Y_TYPE_VALUE:
		rtn = smart_ctl_piecewise_func_v0(func, x, &value);
		result->value[0] = (cmr_s16) value;
		result->weight[0] = SMART_WEIGHT_UNIT;
		break;

	case ISP_SMART_Y_TYPE_WEIGHT_VALUE:
		rtn = smart_ctl_piecewise_func_v1(func, x, SMART_WEIGHT_UNIT, result);
		break;

	default:
		rtn = ISP_ERROR;
		break;
	}

	return rtn;
}

static cmr_s32 smart_ctl_calc_bv_section(struct isp_range bv_range[], cmr_u32 num, cmr_s32 bv, struct isp_weight_value *result)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u16 bv_distance[2] = { 0, 1 };
	cmr_u32 i = 0;

	if (num < 1)
		return ISP_ERROR;

	if (bv <= bv_range[0].max) {
		result->value[0] = 0;
		result->value[1] = 0;
	} else if (bv >= bv_range[num - 1].min) {
		result->value[0] = num - 1;
		result->value[1] = num - 1;
	} else {
		rtn = ISP_ERROR;

		for (i = 0; i < num - 1; i++) {

			if (bv >= bv_range[i].min && bv <= bv_range[i].max) {
				result->value[0] = i;
				result->value[1] = i;
				rtn = ISP_SUCCESS;
				break;
			} else if (bv > bv_range[i].max && bv < bv_range[i + 1].min) {

				/*mixed environment */
				bv_distance[0] = bv - bv_range[i].max;
				bv_distance[1] = bv_range[i + 1].min - bv;
				result->value[0] = i;
				result->value[1] = i + 1;
				rtn = ISP_SUCCESS;
				break;
			}
		}
	}

	if (ISP_SUCCESS == rtn) {
		/*calc weight value for mixed environment */
		result->weight[0] = bv_distance[1] * SMART_WEIGHT_UNIT / (bv_distance[0] + bv_distance[1]);
		result->weight[1] = SMART_WEIGHT_UNIT - result->weight[0];
	} else {
		ISP_LOGE("fail to calc bv_section");
	}

	return rtn;
}

static cmr_s32 smart_ctl_calc_component(struct isp_smart_component_cfg *cfg, struct smart_calc_param *param, struct smart_component_result *result, cmr_u32 smart_id)
{
	if ((NULL == cfg) || (NULL == param)) {
		ISP_LOGE("smart_ctl_calc_component param is null!\n");
		return ISP_ERROR;
	}

	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 i = 0;
	cmr_s32 bv = param->bv;
	cmr_s32 bv_gain = param->bv_gain;
	cmr_u32 ct = param->ct;
	cmr_u16 abl_weight = param->abl_weight;
	cmr_u32 cur_fps = param->fps;
	struct isp_weight_value func_result = { {0}, {0} };
	struct isp_weight_value *fix_data = (struct isp_weight_value *)result->fix_data;
	struct isp_weight_value bv_result = { {0}, {0} };
	struct isp_weight_value tmp_result[2] = { {{0}, {0}}, {{0}, {0}} };

	switch (cfg->x_type) {
	case ISP_SMART_X_TYPE_BV:
		rtn = smart_crl_calc_func(&cfg->func[0], cfg->y_type, bv, &func_result);
		break;

	case ISP_SMART_X_TYPE_BV_GAIN:
		rtn = smart_crl_calc_func(&cfg->func[0], cfg->y_type, bv_gain, &func_result);
		break;

	case ISP_SMART_X_TYPE_CT:
		rtn = smart_crl_calc_func(&cfg->func[0], cfg->y_type, ct, &func_result);
		break;

	case ISP_SMART_X_TYPE_BV_CT:
	case ISP_SMART_X_TYPE_BV_ABLWEIGHT:
		{
			rtn = smart_ctl_calc_bv_section(cfg->bv_range, cfg->section_num, bv, &bv_result);
			if (ISP_SUCCESS != rtn) {
				return rtn;
			}

			for (i = 0; i < 2; i++) {
				/*bv result return the index of the bv section */
				cmr_u32 bv_idx = (cmr_u32) bv_result.value[i];
				cmr_u32 bv_weight = bv_result.weight[i];

				if (bv_weight > 0 && bv_idx < cfg->section_num) {

					if(ISP_SMART_X_TYPE_BV_CT == cfg->x_type) 
						rtn = smart_crl_calc_func(&cfg->func[bv_idx], cfg->y_type, ct, &tmp_result[i]);
					else if(ISP_SMART_X_TYPE_BV_ABLWEIGHT == cfg->x_type)
						rtn = smart_crl_calc_func(&cfg->func[bv_idx], cfg->y_type, abl_weight, &tmp_result[i]);

					if (ISP_SUCCESS != rtn)
						return rtn;
				}
			}

			if (ISP_SMART_Y_TYPE_VALUE == cfg->y_type) {
				cmr_s32 sum = tmp_result[0].value[0] * bv_result.weight[0]
				    + tmp_result[1].value[0] * bv_result.weight[1];
				cmr_s32 weight = bv_result.weight[0] + bv_result.weight[1];

				if (weight > 0)
					func_result.value[0] = sum / weight;
				else
					rtn = ISP_ERROR;
			} else if (ISP_SMART_Y_TYPE_WEIGHT_VALUE == cfg->y_type) {
				if (0 == bv_result.weight[1]) {
					func_result = tmp_result[0];
				} else if (0 == bv_result.weight[0]) {
					func_result = tmp_result[1];
				} else {
					for (i = 0; i < 2; i++) {
						if (tmp_result[i].weight[0] > tmp_result[i].weight[1])
							func_result.value[i] = tmp_result[i].value[0];
						else
							func_result.value[i] = tmp_result[i].value[1];
					}

					func_result.weight[0] = bv_result.weight[0];
					func_result.weight[1] = bv_result.weight[1];
				}
			} else {
				rtn = ISP_ERROR;
			}
		}
		break;
	default:
		rtn = ISP_ERROR;
		break;

	}

	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to calc component, x type=%d", cfg->x_type);
		return ISP_ERROR;
	}

	switch (cfg->y_type) {
	case ISP_SMART_Y_TYPE_VALUE:
		result->size = sizeof(cmr_s32);
		result->fix_data[0] = func_result.value[0];
		ISP_LOGV("value = %d", result->fix_data[0]);

		if (smart_id == ISP_SMART_RAW_GTM) {
			fix_data->weight[0] = SMART_WEIGHT_UNIT;
			if(cfg->use_flash_val){
				cmr_u32 fps_limit = cfg->flash_val;
				if (cur_fps <= fps_limit ){
					fix_data->value[0] = cfg->default_val;
					ISP_LOGV("Disable GTM when low fps by setting cfg default_val:%d",fix_data->value[0]);
				}
			}
		}
		break;

	case ISP_SMART_Y_TYPE_WEIGHT_VALUE:
		if ((smart_id == ISP_SMART_CMC) || (smart_id == ISP_SMART_HSV) || (smart_id == ISP_SMART_HSV_NEW) || (smart_id == ISP_SMART_LNC)) {
			result->size = sizeof(bv_result) * 3;
			fix_data[0].weight[0] = bv_result.weight[0];
			fix_data[0].weight[1] = bv_result.weight[1];
			fix_data[0].value[0] = bv_result.value[0];
			fix_data[0].value[1] = bv_result.value[1];

			fix_data[1].weight[0] = tmp_result[0].weight[0];
			fix_data[1].weight[1] = tmp_result[0].weight[1];
			fix_data[1].value[0] = tmp_result[0].value[0];
			fix_data[1].value[1] = tmp_result[0].value[1];

			fix_data[2].weight[0] = tmp_result[1].weight[0];
			fix_data[2].weight[1] = tmp_result[1].weight[1];
			fix_data[2].value[0] = tmp_result[1].value[0];
			fix_data[2].value[1] = tmp_result[1].value[1];
			if(smart_id == ISP_SMART_HSV)
			ISP_LOGV("yzl add hsv fixdata0:%d,%d,%d,%d, fixdata1:%d,%d,%d,%d, fixdata2:%d,%d,%d,%d" , 
			fix_data[0].value[0], fix_data[0].value[1],fix_data[0].weight[0],fix_data[0].weight[1],
			fix_data[1].value[0] ,fix_data[1].value[1] ,fix_data[1].weight[0],fix_data[1].weight[1],
			fix_data[2].value[0] ,fix_data[2].value[1],fix_data[2].weight[0],fix_data[2].weight[1]);
			char value[PROPERTY_VALUE_MAX] = {0};
			int index = -1;
			if(smart_id == ISP_SMART_CMC)
			{
				property_get("debug.isp.smart.cmc.index", value, "-1");
				index = atoi(value);
			}
			else if((smart_id == ISP_SMART_HSV)||(smart_id == ISP_SMART_HSV_NEW))
			{
				property_get("debug.isp.smart.hsv.index", value, "-1");
				index = atoi(value);
			}
			if ((index >=0) && (index <=8)) {
				fix_data[1].value[0] = index;
				fix_data[1].value[1] = index;
				fix_data[2].value[0] = index;
				fix_data[2].value[1] = index;
			}

			if (1 == is_print_log()) {
				ISP_LOGV("value=(%d, %d), weight=(%d, %d)", fix_data[0].value[0], fix_data[0].value[1], fix_data[0].weight[0], fix_data[0].weight[1]);
			}
		} else {
			result->size = sizeof(func_result);
			fix_data->weight[0] = func_result.weight[0];
			fix_data->weight[1] = func_result.weight[1];
			fix_data->value[0] = func_result.value[0];
			fix_data->value[1] = func_result.value[1];

			if (smart_id == ISP_SMART_GAMMA) {
				char value[PROPERTY_VALUE_MAX] = {0};
				property_get("debug.isp.smart.gamma.index", value, "-1");
				int index = atoi(value);
				if ((index >=0) && (index <=8)) {
					fix_data->value[0] = index;
					fix_data->value[1] = index;
				}
			} else if (smart_id == ISP_SMART_RAW_GTM){
				if(cfg->use_flash_val){
					cmr_u32 fps_limit = cfg->flash_val;
					if (cur_fps <= fps_limit ){
						fix_data->value[0] = cfg->default_val;
						fix_data->value[1] = cfg->default_val;
						ISP_LOGV("Disable GTM when low fps by setting cfg:%d",fix_data->value[0]);
					}
				}
			}

			if (1 == is_print_log()) {
				ISP_LOGV("value=(%d, %d), weight=(%d, %d)", fix_data->value[0], fix_data->value[1], fix_data->weight[0], fix_data->weight[1]);
			}
		}
		break;

	default:
		rtn = ISP_ERROR;
		break;
	}

	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to calc component, y type=%d", cfg->y_type);
		return ISP_ERROR;
	}

	result->y_type = cfg->y_type;
	result->x_type = cfg->x_type;

	return rtn;
}

static cmr_s32 smart_ctl_calc_component_flash(struct isp_smart_component_cfg *cfg, struct smart_calc_param *param, struct smart_component_result *result, cmr_u32 smart_id)
{
	if ((NULL == cfg) || (NULL == param)) {
		ISP_LOGE("smart_ctl_calc_component_flash param is null!\n");
		return ISP_ERROR;
	}

	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_weight_value *fix_data = (struct isp_weight_value *)result->fix_data;

	switch (cfg->y_type) {
	case ISP_SMART_Y_TYPE_VALUE:
		result->size = sizeof(cmr_s32);
		result->fix_data[0] = cfg->flash_val;
		ISP_LOGV("value = %d", result->fix_data[0]);
		break;

	case ISP_SMART_Y_TYPE_WEIGHT_VALUE:
		if (smart_id == ISP_SMART_CMC) {
			result->size = sizeof(struct isp_weight_value) * 5;
			result->flash = 1;

			fix_data[3].weight[0] = param->flash_ratio1;
			fix_data[3].weight[1] = 256 - param->flash_ratio1;
			fix_data[3].value[0] = 7;
			fix_data[3].value[1] = 8;

			fix_data[4].weight[0] = param->flash_ratio;
			fix_data[4].weight[1] = 256-param->flash_ratio;
			fix_data[4].value[0] = 0;
			fix_data[4].value[1] = 0;

			ISP_LOGV("flash_weight=(%d, %d)", param->flash_ratio, param->flash_ratio1);
		}
		break;

	default:
		rtn = ISP_ERROR;
		break;

	}

	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to calc component_flash, y type=%d", cfg->y_type);
		return ISP_ERROR;
	}

	result->y_type = cfg->y_type;
	result->x_type = cfg->x_type;

	return rtn;
}

static cmr_s32 smart_ctl_calc_block(struct isp_smart_block_cfg *cfg, struct smart_calc_param *param,
				    struct smart_block_result *result, enum smart_ctrl_flash_mode flash_mode)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 i = 0;
	struct smart_component_result component_result = { 0, 0, 0, 0, 0, {0}, NULL };
	cmr_u32 component_num = cfg->component_num;
	cmr_u32 update_num = 0;

	if (0 == cfg->enable)
		return ISP_SUCCESS;

	rtn = smart_ctl_check_block_param(cfg);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to check block param\n");
		return rtn;
	}
	component_num = component_num > ISP_SMART_MAX_VALUE_NUM ? ISP_SMART_MAX_VALUE_NUM : component_num;

	if (1 == is_print_log()) {
		ISP_LOGV("use_flash_val = %d. block_id = %x. smart_id = %x.\n", cfg->component[i].use_flash_val, cfg->block_id, cfg->smart_id);
	}

	for (i = 0; i < component_num; i++) {
		if (1 == is_print_log()) {
			ISP_LOGV("ISP_TAG: flash_mode = %d, use_flash_val = %d. bv = %d, ct = %d abl_weight= %d\n", flash_mode, cfg->component[i].use_flash_val, param->bv, param->ct,param->abl_weight);
		}
		rtn = smart_ctl_calc_component(&cfg->component[i], param, &component_result, cfg->smart_id);

		if (SMART_CTRL_FLASH_MAIN == flash_mode && 1 == cfg->component[i].use_flash_val) {
			rtn = smart_ctl_calc_component_flash(&cfg->component[i], param, &component_result, cfg->smart_id);
		}

		if (ISP_SUCCESS == rtn) {
			result->component[i] = component_result;
			result->update = 1;
			update_num++;
		}
	}

	if (update_num > 0) {
		result->block_id = cfg->block_id;
		result->component_num = update_num;
		result->update = 1;
		result->smart_id = cfg->smart_id;
	}

	/*always return success for the update will be 0 if any error occured */
	return rtn;
}

static const char *smart_ctl_find_block_name(cmr_u32 smart_id)
{
	cmr_u32 smart_id_tmp = 0;
	smart_id_tmp = smart_id;

	if (smart_id_tmp >= (sizeof(s_smart_block_name)/sizeof(s_smart_block_name[0])))
		smart_id_tmp = sizeof(s_smart_block_name)/sizeof(s_smart_block_name[0]) - 1;
	//ISP_LOGV("smard_id_tmp max is %d",(int)(sizeof(s_smart_block_name)/sizeof(s_smart_block_name[0])));

	return s_smart_block_name[smart_id_tmp];
}

static void smart_ctl_print_debug_file(smart_handle_t handle,debug_handle_t debug_file, struct smart_calc_param *calc_param, struct smart_calc_result *result, char *debug_buf,smart_gamma_debuginfo *smt_dbg )
{
	struct smart_block_result *blk = NULL;
	struct smart_component_result *comp = NULL;
	struct isp_weight_value *weight_value = NULL;
	struct smart_context *cxt = NULL;
	cmr_u32 i = 0, j = 0;
	const char *block_name = NULL;
	cmr_s32 rtn = ISP_SUCCESS;
	char value[PROPERTY_VALUE_MAX] = { 0 };
	cxt = (struct smart_context *)handle;
	property_get("persist.vendor.cam.isp.smartdebug", value, "0");

	if (!strcmp(value, "0"))
		return;

	rtn = smart_debug_file_open(debug_file);
	if (ISP_SUCCESS != rtn)
		return;

	sprintf(debug_buf, "bv=%d, ct=%d, bv_gain=%d", calc_param->bv, calc_param->ct, calc_param->bv_gain);
	smart_debug_file_print(debug_file, debug_buf);

	for (i = 0; i < result->counts; i++) {
		blk = &result->block_result[i];

		if (!blk->update)
			continue;

		block_name = smart_ctl_find_block_name(blk->smart_id);

		for (j = 0; j < blk->component_num; j++) {
			comp = &blk->component[j];

			switch (comp->y_type) {
			case ISP_SMART_Y_TYPE_VALUE:
				snprintf(debug_buf,sizeof(cxt->debug_buf), "%s: [%d]:val=%d", block_name, j, comp->fix_data[0]);
				break;

			case ISP_SMART_Y_TYPE_WEIGHT_VALUE:
				weight_value = (struct isp_weight_value *)comp->fix_data;

				if (comp->x_type == ISP_SMART_X_TYPE_BV_CT) {
					snprintf(debug_buf, sizeof(cxt->debug_buf),"%s, [%d]:val=(%d, %d), w=(%d, %d) %d(%d, %d):(%d, %d) %d(%d, %d):(%d, %d)", block_name, j,
						weight_value[0].value[0], weight_value[0].value[1],
						weight_value[0].weight[0], weight_value[0].weight[1],
						weight_value[0].value[0], weight_value[1].value[0], weight_value[1].value[1], weight_value[1].weight[0], weight_value[1].weight[1],
						weight_value[0].value[1], weight_value[2].value[0], weight_value[2].value[1], weight_value[2].weight[0], weight_value[2].weight[1]
					    );
				} else {
					snprintf(debug_buf, sizeof(cxt->debug_buf),"%s, [%d]:val=(%d, %d), w=(%d, %d) ", block_name, j,
						weight_value[0].value[0], weight_value[0].value[1], weight_value[0].weight[0], weight_value[0].weight[1]
					    );
				}

				break;

			default:
				sprintf(debug_buf, "unknown y type");
				break;
			}

			smart_debug_file_print(debug_file, debug_buf);
		}
	}

	sprintf(debug_buf, "atm low_pt (%d->%d) hight_pt (%d->%d)",smt_dbg->uFinalLowBin,smt_dbg->uLowPT,smt_dbg->uFinalHighBin,smt_dbg->uHighPT);
	smart_debug_file_print(debug_file, debug_buf);
	smart_debug_file_close(debug_file);
}

static void smart_ctl_print_smart_result(cmr_u32 mode, struct smart_calc_result *result)
{
	struct smart_block_result *blk = NULL;
	struct smart_component_result *comp = NULL;
	struct isp_weight_value *weight_value = NULL;
	cmr_u32 i = 0, j = 0;
	const char *block_name = NULL;

	if (0 == is_print_log())
		return;

	ISP_LOGV("block num = %d", result->counts);

	for (i = 0; i < result->counts; i++) {
		blk = &result->block_result[i];
		block_name = smart_ctl_find_block_name(blk->smart_id);

		ISP_LOGI("block[%d]: %s, block_id=0x%x, smart_id=%d, update=%d, mode=%d", i, block_name, blk->block_id, blk->smart_id, blk->update, mode);

		if (!blk->update)
			continue;

		for (j = 0; j < blk->component_num; j++) {
			comp = &blk->component[j];

			switch (comp->y_type) {
			case ISP_SMART_Y_TYPE_VALUE:
				ISP_LOGI("SMART_MLOG %s smartID:%d: component[%d]: value=%d",block_name,blk->smart_id,j, comp->fix_data[0]);
				break;

			case ISP_SMART_Y_TYPE_WEIGHT_VALUE:
				weight_value = (struct isp_weight_value *)comp->fix_data;

				if (comp->x_type == ISP_SMART_X_TYPE_BV_CT) {
					ISP_LOGI("SMART_MLOG %s smartID:%d: component[%d]: value=(%d, %d), weight=(%d, %d), %d(%d, %d):(%d, %d), %d(%d, %d):(%d, %d)", block_name,blk->smart_id,j,
						 weight_value[0].value[0], weight_value[0].value[1],
						 weight_value[0].weight[0], weight_value[0].weight[1],
						 weight_value[0].value[0], weight_value[1].value[0], weight_value[1].value[1], weight_value[1].weight[0], weight_value[1].weight[1],
						 weight_value[0].value[1], weight_value[2].value[0], weight_value[2].value[1], weight_value[2].weight[0], weight_value[2].weight[1]
					    );
				} else {
					ISP_LOGI("SMART_MLOG %s smartID %d: component[%d]: value=(%d, %d), weight=(%d, %d)",block_name,blk->smart_id,j,
						 weight_value->value[0], weight_value->value[1], weight_value->weight[0], weight_value->weight[1]);
				}
				break;
			}
		}
	}
}

static cmr_s32 smart_ctl_parse_atm_tuning_param(struct atm_tune_param *src, cmr_u32 data_size, struct atm_tune_param *dst){

	cmr_s32 rtn = ISP_SUCCESS;

	if (src != NULL) {
		if ((src->version == 0 || src->version == 1) && data_size > 0 && dst != NULL) {
			memcpy(dst,src,sizeof(struct atm_tune_param));
		}
	} else {
		ISP_LOGE("smart_ctl_parse_atm_tuning_param error,src null");
		rtn = ISP_ERROR;
	}
	return rtn;
}

static cmr_s32 smart_ctl_parse_atm_tuning_param_v1(struct ae_atm_tune_param_v1 *src, cmr_u32 data_size, struct ae_atm_tune_param_v1 *dst){

	cmr_s32 rtn = ISP_SUCCESS;

	if (src != NULL) {
		if (src->version == 2 && data_size > 0 && dst != NULL) {
			memcpy(dst,src,sizeof(struct ae_atm_tune_param_v1));
		}
	} else {
		ISP_LOGE("smart_ctl_parse_atm_tuning_param error,src null");
		rtn = ISP_ERROR;
	}
	return rtn;
}

smart_handle_t smart_ctl_init(struct smart_init_param *param, void *result)
{
	cmr_s32 rtn = ISP_SUCCESS;
	smart_handle_t handle = NULL;
	struct smart_context *cxt = NULL;
	char smart_property[PROPERTY_VALUE_MAX];
	int val = 0;

	if (NULL == param) {
		ISP_LOGE("fail to check input param, in: %p, out: %p\n", param, result);
		goto param_failed;
	}

	/* create isp_smart_context handle. */
	cxt = (struct smart_context *)malloc(sizeof(struct smart_context));
	if (NULL == cxt) {
		ISP_LOGE("fail to malloc, size: %zd\n", sizeof(struct smart_context));
		goto malloc_failed;
	}

	/* initial isp_smart_contex is set zeros. */
	memset((void *)cxt, 0x00, sizeof(struct smart_context));

	cxt->camera_id = (cmr_u32)param->camera_id;
	cxt->smart_lock_frame = 5; //lock smart N frames when change mode;

	rtn = smart_ctl_parse_tuning_param(param->tuning_param, cxt->tuning_param, SMART_MAX_WORK_MODE);
	memcpy(cxt->tuning_param_org, cxt->tuning_param, sizeof(cxt->tuning_param_org));
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to parse tuning param, rtn %d", rtn);
		goto parse_tuning_failed;
	}


	rtn = smart_ctl_apply_stash(cxt, NULL);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to apply smart, rtn %d", rtn);
		goto parse_tuning_failed;
	}


#ifdef GATM_ENABLE
	{ // ATM
		struct atm_init_in atm_param_in;
		isp_atm_init(&atm_param_in, (cmr_handle*)&cxt->handle_atm);
		if(ATM_TUNING_VERSION_V0 == param->atm_size){
			struct atm_tune_param *atm_info = (struct atm_tune_param *)param->atm_info;
			rtn = smart_ctl_parse_atm_tuning_param(atm_info,param->atm_size,&cxt->atm_tuning_param);
			cxt->atm_version = cxt->atm_tuning_param.version;
			if (ISP_SUCCESS != rtn) {
				ISP_LOGE("fail to parse tuning param, rtn %d", rtn);
				goto parse_tuning_failed;
			}
		}else if(ATM_TUNING_VERSION_V1 == param->atm_size){
			struct ae_atm_tune_param_v1 *atm_info = (struct ae_atm_tune_param_v1 *)param->atm_info;
			rtn = smart_ctl_parse_atm_tuning_param_v1(atm_info,param->atm_size,&cxt->atm_tuning_param_v1);
			cxt->atm_version = cxt->atm_tuning_param_v1.version;
			if (ISP_SUCCESS != rtn) {
				ISP_LOGE("fail to parse tuning param v1, rtn %d", rtn);
				goto parse_tuning_failed;
			}
		}
	}
#endif
	cxt->smt_dbginfo.nr_param.ppi[0] = 0;
	cxt->smt_dbginfo.nr_param.ppi[1] = 9;
	cxt->smt_dbginfo.nr_param.ppi[2] = 0;
	cxt->smt_dbginfo.nr_param.bayer_nr[0] = 0;
	cxt->smt_dbginfo.nr_param.bayer_nr[1] = 10;
	cxt->smt_dbginfo.nr_param.bayer_nr[2] = 0;
	cxt->smt_dbginfo.nr_param.rgb_dither[0] = 0;
	cxt->smt_dbginfo.nr_param.rgb_dither[1] = 11;
	cxt->smt_dbginfo.nr_param.rgb_dither[2] = 0;
	cxt->smt_dbginfo.nr_param.bpc[0] = 0;
	cxt->smt_dbginfo.nr_param.bpc[1] = 12;
	cxt->smt_dbginfo.nr_param.bpc[2] = 0;
	cxt->smt_dbginfo.nr_param.grgb[0] = 0;
	cxt->smt_dbginfo.nr_param.grgb[1] = 13;
	cxt->smt_dbginfo.nr_param.grgb[2] = 0;
	cxt->smt_dbginfo.nr_param.cfae[0] = 0;
	cxt->smt_dbginfo.nr_param.cfae[1] = 14;
	cxt->smt_dbginfo.nr_param.cfae[2] = 0;
	cxt->smt_dbginfo.nr_param.rgb_afm[0] = 0;
	cxt->smt_dbginfo.nr_param.rgb_afm[1] = 15;
	cxt->smt_dbginfo.nr_param.rgb_afm[2] = 0;
	cxt->smt_dbginfo.nr_param.uvdiv[0] = 0;
	cxt->smt_dbginfo.nr_param.uvdiv[1] = 16;
	cxt->smt_dbginfo.nr_param.uvdiv[2] = 0;
	cxt->smt_dbginfo.nr_param.dnr3_pre[0] = 0;
	cxt->smt_dbginfo.nr_param.dnr3_pre[1] = 17;
	cxt->smt_dbginfo.nr_param.dnr3_pre[2] = 0;
	cxt->smt_dbginfo.nr_param.dnr3_cap[0] = 0;
	cxt->smt_dbginfo.nr_param.dnr3_cap[1] = 18;
	cxt->smt_dbginfo.nr_param.dnr3_cap[2] = 0;
	cxt->smt_dbginfo.nr_param.edge[0] = 0;
	cxt->smt_dbginfo.nr_param.edge[1] = 19;
	cxt->smt_dbginfo.nr_param.edge[2] = 0;
	cxt->smt_dbginfo.nr_param.precdn[0] = 0;
	cxt->smt_dbginfo.nr_param.precdn[1] = 20;
	cxt->smt_dbginfo.nr_param.precdn[2] = 0;
	cxt->smt_dbginfo.nr_param.ynr[0] = 0;
	cxt->smt_dbginfo.nr_param.ynr[1] = 21;
	cxt->smt_dbginfo.nr_param.ynr[2] = 0;
	cxt->smt_dbginfo.nr_param.cdn[0] = 0;
	cxt->smt_dbginfo.nr_param.cdn[1] = 22;
	cxt->smt_dbginfo.nr_param.cdn[2] = 0;
	cxt->smt_dbginfo.nr_param.postcdn[0] = 0;
	cxt->smt_dbginfo.nr_param.postcdn[1] = 23;
	cxt->smt_dbginfo.nr_param.postcdn[2] = 0;
	cxt->smt_dbginfo.nr_param.ccnr[0] = 0;
	cxt->smt_dbginfo.nr_param.ccnr[1] = 24;
	cxt->smt_dbginfo.nr_param.ccnr[2] = 0;
	cxt->smt_dbginfo.nr_param.iir_yrandom[0] = 0;
	cxt->smt_dbginfo.nr_param.iir_yrandom[1] = 25;
	cxt->smt_dbginfo.nr_param.iir_yrandom[2] = 0;
	cxt->smt_dbginfo.nr_param.noisefilter[0] = 0;
	cxt->smt_dbginfo.nr_param.noisefilter[1] = 26;
	cxt->smt_dbginfo.nr_param.noisefilter[2] = 0;
	cxt->smt_dbginfo.nr_param.ltm[0] = 0;
	cxt->smt_dbginfo.nr_param.ltm[1] = 3;
	cxt->smt_dbginfo.nr_param.ltm[2] = 0;
	cxt->smt_dbginfo.nr_param.imbalance[0] = 0;
	cxt->smt_dbginfo.nr_param.imbalance[1] = 5;
	cxt->smt_dbginfo.nr_param.imbalance[2] = 0;
	cxt->smt_dbginfo.nr_param.threednr[0] = 0;
	cxt->smt_dbginfo.nr_param.threednr[1] = 33;
	cxt->smt_dbginfo.nr_param.threednr[2] = 0;
	cxt->smt_dbginfo.nr_param.mfnr[0] = 0;
	cxt->smt_dbginfo.nr_param.mfnr[1] = 34;
	cxt->smt_dbginfo.nr_param.mfnr[2] = 0;

	cxt->magic_flag = ISP_SMART_MAGIC_FLAG;
	cxt->work_mode = 0;
	cxt->flash_mode = SMART_CTRL_FLASH_CLOSE;
	cxt->cur_param = &cxt->tuning_param[cxt->work_mode];
	cxt->debug_file = smart_debug_file_init(DEBUG_FILE_NAME, "wt");
	cxt->caller_handle = param->caller_handle;
	cxt->smart_set_cb = param->smart_set_cb;
	handle = (smart_handle_t) cxt;

	memset((cmr_handle) & smart_property, 0, sizeof(smart_property));
	property_get("persist.vendor.cam.isp.smart.logv", smart_property, "0");
	val = atoi(smart_property);
	if (0 < val)
		g_smart_log_level = val;

	ISP_LOGI("done rtn %d", rtn);
	return handle;

parse_tuning_failed:
	free(cxt);
	cxt = NULL;
malloc_failed:
param_failed:
	return handle;
}

int isp_bayer_hist(struct smart_proc_input *smart_proc_in,  unsigned long long *hist) {
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 i = 0;

	/*G R Bchannel, pdf and cdf related calc*/
	for(i = 0; i < 256; i++){
		hist[i] = smart_proc_in->bhist_stats[0].value[i] + smart_proc_in->bhist_stats[1].value[i] + smart_proc_in->bhist_stats[2].value[i];
	}
	return rtn;
}

void _get_8bitX_Y(int i4Bits, int i4InputLen, short *inputX, short *inputY, unsigned char *i4outputY) {
	short i2StartX = 0;
	int i, j;
	for ( i = 0; i < 256;i++) {
		for ( j = i2StartX; j < i4InputLen; j++) {
			if (inputX[j] >= (i<<(i4Bits-8))) {
			i2StartX = min(j+1, i4InputLen);
				if (j == 0) {
					i4outputY[i] = inputY[j];
				 } else {
					 int X1 = inputX[j-1];
					int X2 = inputX[j];
					int Y1 = inputY[j-1];
					int Y2 = inputY[j];
					int X = (i<<(i4Bits-8));
					if (X2 <= X1) {
						i4outputY[i] = Y1;
						ISP_LOGV("X2/X1 %d/%d\n", X2, X1);
					} else
						i4outputY[i] = max(min(Y1 + (X-X1)*(Y2-Y1)/(X2-X1), 255),0);
				}
				break;
			}
		}
	}
}

cmr_int _get_atm_curve_v1(cmr_handle *handle,
	struct smart_proc_input *smart_proc_in,
	int i4BV, void *in_gamma, void *out_gamma,
	smart_gamma_debuginfo *dbginfo,
	struct ae_atm_tune_param_v1*atm_param) {
	char value[PROPERTY_VALUE_MAX];
	property_get("debug.camera.atm.update", value, "1");
	bool bATMUpdate = strtol(value, NULL, 10);
	property_get("debug.camera.atm.dump", value, "0");
	bool bATMDump = strtol(value, NULL, 10);
	property_get("debug.camera.atm.hist", value, "0");
	bool bATMDumpHist = strtol(value, NULL, 10);
	int ret, i;
	unsigned int u4GammaSize = 0;
	unsigned long long hist[256] = {0};
	unsigned char uConvCurY[256] = {0},
	uOutGamma[3][256];
	unsigned char uPrevGamma[3][256]= {{0},{0},{0}};
	unsigned short
	u2CurX[3][SENSOR_GAMMA_POINT_NUM] = {{0},{0},{0}},
	u2CurY[3][SENSOR_GAMMA_POINT_NUM] = {{0},{0},{0}};
	struct atm_calc_param ATMInput;
	struct atm_calc_result ATMOutput;
	memset(&ATMInput, 0, sizeof(struct atm_calc_param));
	memset(&ATMOutput, 0, sizeof(struct atm_calc_result));

	bool bAEMBinning = (smart_proc_in->win_size_w == 1 && smart_proc_in->win_size_h == 1);
	if (in_gamma == NULL || out_gamma == NULL)
		return ISP_ERROR;

	struct sensor_rgbgamma_curve *gamma_info = (struct sensor_rgbgamma_curve *)in_gamma;
	u4GammaSize = sizeof(struct sensor_rgbgamma_curve);
	{
		cmr_s32 i;
		for (i = 0; i < SENSOR_GAMMA_POINT_NUM; i++) {
			u2CurY[0][i] = gamma_info->points_r[i].y;
			u2CurX[0][i] = gamma_info->points_r[i].x;
			u2CurY[1][i] = gamma_info->points_g[i].y;
			u2CurX[1][i] = gamma_info->points_g[i].x;
			u2CurY[2][i] = gamma_info->points_b[i].y;
			u2CurX[2][i] = gamma_info->points_b[i].x;
			if (i == 0||i == 64 || i == 128 || i == 255)
			ISP_LOGV("%d x/y %d/%d\n", i, u2CurX[0][i], u2CurY[0][i]);
        	}
	}
	if(smart_proc_in->bhist_update && atm_param->input_type){
		ret = isp_bayer_hist(smart_proc_in, hist);
	}else{
		ret = isp_histAEM(smart_proc_in->r_info,
			smart_proc_in->g_info, smart_proc_in->b_info, hist, bAEMBinning,
			smart_proc_in->aem_shift,
			smart_proc_in->win_num_w, smart_proc_in->win_num_h,
			smart_proc_in->win_size_w, smart_proc_in->win_size_h);
	}
	if (ISP_SUCCESS != ret) {
		 ISP_LOGE("ATM aem_ptr %p,%p,%p, win_num/win_size %d/%d, %d/%d, shift %d\n",
		smart_proc_in->r_info,
		smart_proc_in->g_info,
		smart_proc_in->b_info,
		smart_proc_in->win_num_w, smart_proc_in->win_num_h,
		smart_proc_in->win_size_w, smart_proc_in->win_size_h,
		smart_proc_in->aem_shift);
		return ISP_ERROR;
	}
	if (bATMDumpHist == true) {
		ISP_LOGV("aem num w/h %d/%d, size w/h %d/%d, shift %d\n",
		smart_proc_in->win_num_w, smart_proc_in->win_num_h,
		smart_proc_in->win_size_w, smart_proc_in->win_size_h,
		smart_proc_in->aem_shift);
	for (i = 0; i < 256; i+=4)
	ISP_LOGV("Hist[%d-%d] = %llu,%llu,%llu,%llu\n",i,i+3,hist[i],hist[i+1],hist[i+2],hist[i+3]);
	}
	if (bATMDump == true) {
		for (i = 0; i < 256; i+=4)
			ISP_LOGV("Orig Gamma [%3d/%3d] [%3d/%3d] [%3d/%3d] [%3d/%3d]\n",
			u2CurX[1][i], u2CurY[1][i],
			u2CurX[1][i+1], u2CurY[1][i+1],
			u2CurX[1][i+2], u2CurY[1][i+2],
			u2CurX[1][i+3], u2CurY[1][i+3]);
	}
	_get_8bitX_Y(10, SENSOR_GAMMA_POINT_NUM, (short*)u2CurX[1], (short*)u2CurY[1],uConvCurY);
	if (bATMDump == true) {
	for (i = 0; i < 32; i+=4)
		ISP_LOGV("Conv Gamma [%3d/%3d] [%3d/%3d] [%3d/%3d] [%3d/%3d]\n",
		i, uConvCurY[i],
		i+1, uConvCurY[i+1],
		i+2, uConvCurY[i+2],
		i+3, uConvCurY[i+3]);
	}

	ISP_LOGV("atm Out Gamma %p \n", out_gamma);
	{
		ATMInput.stAlgoParams.i4BV = i4BV;
		/*
		ATMInput.stAlgoParams.i4BV = i4BV;
		ATMInput.stAlgoParams.i4LowPT = atm_param->i4LowPT;
		ATMInput.stAlgoParams.i4LowPcentThd = atm_param->i4LowPcentThd;
		ATMInput.stAlgoParams.i4LowRightThd   = atm_param->i4LowRightThd;
		ATMInput.stAlgoParams.i4LowLeftThd     = atm_param->i4LowLeftThd;
		ATMInput.stAlgoParams.i4HighPT = atm_param->i4HighPT;
		ATMInput.stAlgoParams.i4HighPcentThd = atm_param->i4HighPcentThd;
		ATMInput.stAlgoParams.i4HighLeftThd  = atm_param->i4HighLeftThd;
		ATMInput.stAlgoParams.i4HighRightThd = atm_param->i4HighRightThd;
		ATMInput.stAlgoParams.strBVLut.i4Len = atm_param->strBVLut.i4Len;
		memcpy(&ATMInput.stAlgoParams.strBVLut.i4X, atm_param->strBVLut.i4X, sizeof(cmr_s32)*8);
		memcpy(&ATMInput.stAlgoParams.strBVLut.i4Y, atm_param->strBVLut.i4Y, sizeof(cmr_s32)*8);
		*/
		memcpy(&ATMInput.atm_tune, atm_param, sizeof(struct ae_atm_tune_param_v1));

		ATMInput.pHist = hist;
		ATMInput.u4Bins = 256;
		ATMInput.uBaseGamma = uConvCurY;
		ATMInput.uModGamma = uConvCurY;
		ATMInput.bHistB4Gamma = true;
		ATMOutput.uGamma = uOutGamma[1];
		ATMInput.atm_version = ATMInput.atm_tune.version;

		isp_atm((cmr_handle*)handle, ATMInput, &ATMOutput);
		if (uOutGamma[1][255]) {
			memcpy(uPrevGamma[1], ATMOutput.uGamma, sizeof(uPrevGamma[1]));
		}

		isp_atm_smooth(200, 256, uPrevGamma[1], uOutGamma[1], uOutGamma[1]);
		memcpy(uPrevGamma[1], uOutGamma[1], sizeof(char)*256);
		if (bATMDump == true) {
			for (i = 0; i < 256; i+=4)
				ISP_LOGV("Out Gamma [%3d/%3d] [%3d/%3d] [%3d/%3d] [%3d/%3d]\n",
				i, uOutGamma[1][i],
				i+1, uOutGamma[1][i+1],
				i+2, uOutGamma[1][i+2],
				i+3, uOutGamma[1][i+3]);
		}
		 // rev to HW gamma curve
 		memcpy(out_gamma, in_gamma, u4GammaSize);
		if (bATMUpdate == true) {
			struct sensor_rgbgamma_curve *gamma_info = (struct sensor_rgbgamma_curve *)out_gamma;
			{
				for (i = 0; i < SENSOR_GAMMA_POINT_NUM; i++) {
				 	gamma_info->points_r[i].y =
					uOutGamma[1][min(gamma_info->points_r[i].x>>2, 255)];
					gamma_info->points_g[i].y =
					uOutGamma[1][min(gamma_info->points_g[i].x>>2, 255)];
					gamma_info->points_b[i].y =
					uOutGamma[1][min(gamma_info->points_b[i].x>>2, 255)];
				}
				// check write out
				if (bATMDump == true) {
					for (i = 0; i < SENSOR_GAMMA_POINT_NUM; i+=4) {
					ISP_LOGV("Final Gamma [%d] = %d\n",
					gamma_info->points_g[i].x,
					gamma_info->points_g[i].y);
					}
				} else {
					ISP_LOGV("Final Gamma [%3d/%3d] [%3d/%3d] [%3d/%3d] [%3d/%3d] \n",
					gamma_info->points_g[0].x,
					gamma_info->points_g[0].y,
					gamma_info->points_g[64].x,
					gamma_info->points_g[64].y,
					gamma_info->points_g[128].x,
					gamma_info->points_g[128].y,
					gamma_info->points_g[255].x,
					gamma_info->points_g[255].y);
				}
			}
		}
	}
	// fill debug info
	dbginfo->version = 1;
	memcpy(dbginfo->u8Hist, hist, sizeof(dbginfo->u8Hist));
	memcpy(dbginfo->u4RespCurve, ATMOutput.i4RespCurve, sizeof(dbginfo->u4RespCurve));
	memcpy(dbginfo->uOutputGamma[0], uOutGamma[1], 256);
	memcpy(dbginfo->uOutputGamma[1], uOutGamma[1], 256);
	memcpy(dbginfo->uOutputGamma[2], uOutGamma[1], 256);
	dbginfo->uLowPT         = ATMOutput.uLowPT;
	dbginfo->uHighPT        = ATMOutput.uHighPT;
	dbginfo->uFinalLowBin   = ATMOutput.uFinalLowBin;
	dbginfo->uFinalHighBin  = ATMOutput.uFinalHighBin;
	return ISP_SUCCESS;
}

cmr_int _get_atm_curve(cmr_handle *handle,
	struct smart_proc_input *smart_proc_in,
	int i4BV, void *in_gamma, void *out_gamma,
	smart_gamma_debuginfo *dbginfo,
	struct atm_tune_param *atm_param) {
	char value[PROPERTY_VALUE_MAX];
	property_get("debug.camera.atm.update", value, "1");
	bool bATMUpdate = strtol(value, NULL, 10);
	property_get("debug.camera.atm.dump", value, "0");
	bool bATMDump = strtol(value, NULL, 10);
	property_get("debug.camera.atm.hist", value, "0");
	bool bATMDumpHist = strtol(value, NULL, 10);
	int ret, i;
	unsigned int u4GammaSize = 0;
	unsigned long long hist[256] = {0};
	unsigned char uConvCurY[256] = {0},
	uOutGamma[3][256];
	unsigned char uPrevGamma[3][256] = {{0},{0},{0}};
	unsigned short
	u2CurX[3][SENSOR_GAMMA_POINT_NUM] = {{0},{0},{0}},
	u2CurY[3][SENSOR_GAMMA_POINT_NUM] = {{0},{0},{0}};
	struct atm_calc_param ATMInput;
	struct atm_calc_result ATMOutput;
	memset(&ATMInput, 0, sizeof(struct atm_calc_param));
	memset(&ATMOutput, 0, sizeof(struct atm_calc_result));

	bool bAEMBinning = (smart_proc_in->win_size_w == 1 && smart_proc_in->win_size_h == 1);
	if (in_gamma == NULL || out_gamma == NULL)
		return ISP_ERROR;

	struct sensor_rgbgamma_curve *gamma_info = (struct sensor_rgbgamma_curve *)in_gamma;
	u4GammaSize = sizeof(struct sensor_rgbgamma_curve);
	{
		cmr_s32 i;
		for (i = 0; i < SENSOR_GAMMA_POINT_NUM; i++) {
			u2CurY[0][i] = gamma_info->points_r[i].y;
			u2CurX[0][i] = gamma_info->points_r[i].x;
			u2CurY[1][i] = gamma_info->points_g[i].y;
			u2CurX[1][i] = gamma_info->points_g[i].x;
			u2CurY[2][i] = gamma_info->points_b[i].y;
			u2CurX[2][i] = gamma_info->points_b[i].x;
			if (i == 0||i == 64 || i == 128 || i == 255)
			ISP_LOGV("%d x/y %d/%d\n", i, u2CurX[0][i], u2CurY[0][i]);
        	}
	}
	ret = isp_histAEM(smart_proc_in->r_info,
		smart_proc_in->g_info, smart_proc_in->b_info, hist, bAEMBinning,
		smart_proc_in->aem_shift,
		smart_proc_in->win_num_w, smart_proc_in->win_num_h,
		smart_proc_in->win_size_w, smart_proc_in->win_size_h);
	if (ISP_SUCCESS != ret) {
		 ISP_LOGE("ATM aem_ptr %p,%p,%p, win_num/win_size %d/%d, %d/%d, shift %d\n",
		smart_proc_in->r_info,
		smart_proc_in->g_info,
		smart_proc_in->b_info,
		smart_proc_in->win_num_w, smart_proc_in->win_num_h,
		smart_proc_in->win_size_w, smart_proc_in->win_size_h,
		smart_proc_in->aem_shift);
		return ISP_ERROR;
	}
	if (bATMDumpHist == true) {
		ISP_LOGV("aem num w/h %d/%d, size w/h %d/%d, shift %d\n",
		smart_proc_in->win_num_w, smart_proc_in->win_num_h,
		smart_proc_in->win_size_w, smart_proc_in->win_size_h,
		smart_proc_in->aem_shift);
	for (i = 0; i < 256; i+=4)
	ISP_LOGV("Hist[%d-%d] = %llu,%llu,%llu,%llu\n",i,i+3,hist[i],hist[i+1],hist[i+2],hist[i+3]);
	}
	if (bATMDump == true) {
		for (i = 0; i < 256; i+=4)
			ISP_LOGV("Orig Gamma [%3d/%3d] [%3d/%3d] [%3d/%3d] [%3d/%3d]\n",
			u2CurX[1][i], u2CurY[1][i],
			u2CurX[1][i+1], u2CurY[1][i+1],
			u2CurX[1][i+2], u2CurY[1][i+2],
			u2CurX[1][i+3], u2CurY[1][i+3]);
	}
	_get_8bitX_Y(10, SENSOR_GAMMA_POINT_NUM, (short*)u2CurX[1], (short*)u2CurY[1],uConvCurY);
	if (bATMDump == true) {
	for (i = 0; i < 32; i+=4)
		ISP_LOGV("Conv Gamma [%3d/%3d] [%3d/%3d] [%3d/%3d] [%3d/%3d]\n",
		i, uConvCurY[i],
		i+1, uConvCurY[i+1],
		i+2, uConvCurY[i+2],
		i+3, uConvCurY[i+3]);
	}

	ISP_LOGV("atm Out Gamma %p \n", out_gamma);
	{
		ATMInput.stAlgoParams.i4BV = i4BV;
		ATMInput.stAlgoParams.i4LowPT = atm_param->i4LowPT;
		ATMInput.stAlgoParams.i4LowPcentThd = atm_param->i4LowPcentThd;
		ATMInput.stAlgoParams.i4LowRightThd   = atm_param->i4LowRightThd;
		ATMInput.stAlgoParams.i4LowLeftThd     = atm_param->i4LowLeftThd;
		ATMInput.stAlgoParams.i4HighPT = atm_param->i4HighPT;
		ATMInput.stAlgoParams.i4HighPcentThd = atm_param->i4HighPcentThd;
		ATMInput.stAlgoParams.i4HighLeftThd  = atm_param->i4HighLeftThd;
		ATMInput.stAlgoParams.i4HighRightThd = atm_param->i4HighRightThd;
		ATMInput.stAlgoParams.strBVLut.i4Len = atm_param->strBVLut.i4Len;
		memcpy(&ATMInput.stAlgoParams.strBVLut.i4X, atm_param->strBVLut.i4X, sizeof(cmr_s32)*8);
		memcpy(&ATMInput.stAlgoParams.strBVLut.i4Y, atm_param->strBVLut.i4Y, sizeof(cmr_s32)*8);

		ATMInput.pHist = hist;
		ATMInput.u4Bins = 256;
		ATMInput.uBaseGamma = uConvCurY;
		ATMInput.uModGamma = uConvCurY;
		ATMInput.bHistB4Gamma = true;
		ATMOutput.uGamma = uOutGamma[1];
		isp_atm((cmr_handle*)handle, ATMInput, &ATMOutput);

		if (uOutGamma[1][255]) {
			memcpy(uPrevGamma[1], ATMOutput.uGamma, sizeof(uPrevGamma[1]));
		}

		isp_atm_smooth(200, 256, uPrevGamma[1], uOutGamma[1], uOutGamma[1]);
		memcpy(uPrevGamma[1], uOutGamma[1], sizeof(char)*256);
		if (bATMDump == true) {
			for (i = 0; i < 256; i+=4)
				ISP_LOGV("Out Gamma [%3d/%3d] [%3d/%3d] [%3d/%3d] [%3d/%3d]\n",
				i, uOutGamma[1][i],
				i+1, uOutGamma[1][i+1],
				i+2, uOutGamma[1][i+2],
				i+3, uOutGamma[1][i+3]);
		}
		 // rev to HW gamma curve
 		memcpy(out_gamma, in_gamma, u4GammaSize);
		if (bATMUpdate == true) {
			struct sensor_rgbgamma_curve *gamma_info = (struct sensor_rgbgamma_curve *)out_gamma;
			{
				for (i = 0; i < SENSOR_GAMMA_POINT_NUM; i++) {
				 	gamma_info->points_r[i].y =
					uOutGamma[1][min(gamma_info->points_r[i].x>>2, 255)];
					gamma_info->points_g[i].y =
					uOutGamma[1][min(gamma_info->points_g[i].x>>2, 255)];
					gamma_info->points_b[i].y =
					uOutGamma[1][min(gamma_info->points_b[i].x>>2, 255)];
				}
				// check write out
				if (bATMDump == true) {
					for (i = 0; i < SENSOR_GAMMA_POINT_NUM; i+=4) {
					ISP_LOGV("Final Gamma [%d] = %d\n",
					gamma_info->points_g[i].x,
					gamma_info->points_g[i].y);
					}
				} else {
					ISP_LOGV("Final Gamma [%3d/%3d] [%3d/%3d] [%3d/%3d] [%3d/%3d] \n",
					gamma_info->points_g[0].x,
					gamma_info->points_g[0].y,
					gamma_info->points_g[64].x,
					gamma_info->points_g[64].y,
					gamma_info->points_g[128].x,
					gamma_info->points_g[128].y,
					gamma_info->points_g[255].x,
					gamma_info->points_g[255].y);
				}
			}
		}
	}
	// fill debug info
	dbginfo->version = 1;
	memcpy(dbginfo->u8Hist, hist, sizeof(dbginfo->u8Hist));
	memcpy(dbginfo->u4RespCurve, ATMOutput.i4RespCurve, sizeof(dbginfo->u4RespCurve));
	memcpy(dbginfo->uOutputGamma[0], uOutGamma[1], 256);
	memcpy(dbginfo->uOutputGamma[1], uOutGamma[1], 256);
	memcpy(dbginfo->uOutputGamma[2], uOutGamma[1], 256);
	dbginfo->uLowPT         = ATMOutput.uLowPT;
	dbginfo->uHighPT        = ATMOutput.uHighPT;
	dbginfo->uFinalLowBin   = ATMOutput.uFinalLowBin;
	dbginfo->uFinalHighBin  = ATMOutput.uFinalHighBin;
	return ISP_SUCCESS;
}

cmr_int _smart_write_gamma(struct smart_context *cxt, struct isp_pm_param_data *pm_param, void *gamma) {
	UNUSED(pm_param);
	if (cxt->smart_set_cb) {
		cxt->smart_set_cb(cxt->caller_handle, ISP_SMART_SET_GAMMA_CUR, gamma, NULL);
		return ISP_SUCCESS;
	} else {
		return ISP_ERROR;
        }
}
cmr_int _smart_read_gamma(struct smart_proc_input *inptr, int idx, void *gamma) {
	int i;
	if (NULL != inptr->cal_para.gamma_tab) {
		struct sensor_rgbgamma_curve *gamma_cur[SENSOR_GAMMA_NUM];
		gamma_cur[0] = (struct sensor_rgbgamma_curve *)inptr->cal_para.gamma_tab;
		for (i = 1; i < SENSOR_GAMMA_NUM; i++)
			gamma_cur[i] = gamma_cur[i-1] + 1;

		memcpy(gamma, gamma_cur[idx], sizeof(struct sensor_rgbgamma_curve));
		return ISP_SUCCESS;
	} else {
		ISP_LOGE("fail to get gamma_tab!\n");
		return ISP_ERROR;
	}
}
cmr_int _smart_gamma(struct smart_proc_input *inptr,
	struct smart_block_result *block_result,
	void *output) {
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_weight_value gamc_value = { {0}, {0} };
	struct isp_range val_range = { 0, 0 };
	struct sensor_rgbgamma_curve gamma_curve[3];
	struct sensor_rgbgamma_curve *gamma_info = output;
	struct sensor_rgbgamma_curve tmp_dst;
	val_range.min = 0;
	val_range.max = SENSOR_GAMMA_NUM - 1;
	cmr_u32 abl_weighting = inptr->cal_para.abl_weight;
	ISP_LOGV("abl_weighting=%d\n", abl_weighting);
	struct isp_weight_value *weight_value = (struct isp_weight_value *)block_result->component[0].fix_data;
	if ((cmr_s32) weight_value->value[0] < val_range.min || (cmr_s32) weight_value->value[0] > val_range.max) {
		ISP_LOGE("fail to value range: %d ([%d, %d])\n", weight_value->value[0], val_range.min, val_range.max);
		return ISP_ERROR;
	}

	if ((cmr_s32) weight_value->value[1] < val_range.min || (cmr_s32) weight_value->value[1] > val_range.max) {
		ISP_LOGE("fail to value range: %d ([%d, %d])\n", weight_value->value[1], val_range.min, val_range.max);
		return ISP_ERROR;
	}
	#if 0
	if (NULL == inptr->cal_para.gamma_tab) {
		ISP_LOGE("fail to get gamma_tab!\n");
		return ISP_ERROR;
	}
	#endif

	weight_value = (struct isp_weight_value *)block_result->component[0].fix_data;
	gamc_value = *weight_value;
	gamc_value.weight[0] = gamc_value.weight[0] / (SMART_WEIGHT_UNIT / 16)
				* (SMART_WEIGHT_UNIT / 16);
	gamc_value.weight[1] = SMART_WEIGHT_UNIT - gamc_value.weight[0];
	ISP_LOGV("atm gamc_value wght %d/%d, value %d/%d\n",
		gamc_value.weight[0],
		gamc_value.weight[1],
		gamc_value.value[0],
		gamc_value.value[1]);

	// read orig gamma
	rtn = _smart_read_gamma(inptr, gamc_value.value[0], &gamma_curve[0]);
	if (ISP_SUCCESS != rtn) {
		return ISP_ERROR;
        }

	rtn = _smart_read_gamma(inptr, gamc_value.value[1], &gamma_curve[1]);
	if (ISP_SUCCESS != rtn) {
		return ISP_ERROR;
        }
	if (abl_weighting > 0) {
		rtn = _smart_read_gamma(inptr, 7, &gamma_curve[2]);//fix index 7 gamma for ABL
		if (ISP_SUCCESS != rtn) {
			return ISP_ERROR;
		}
	}
	{
		void *src_r[2] = { NULL };
		void *src_g[2] = { NULL };
		void *src_b[2] = { NULL };

		void *dst_r = NULL;
		void *dst_g = NULL;
		void *dst_b = NULL;

		src_r[0] = &gamma_curve[0].points_r[0].x;
		src_r[1] = &gamma_curve[1].points_r[0].x;
		dst_r = &tmp_dst.points_r;

		src_g[0] = &gamma_curve[0].points_g[0].x;
		src_g[1] = &gamma_curve[1].points_g[0].x;
		dst_g = &tmp_dst.points_g;

		src_b[0] = &gamma_curve[0].points_b[0].x;
		src_b[1] = &gamma_curve[1].points_b[0].x;
		dst_b = &tmp_dst.points_b;

		rtn = isp_interp_data(dst_r, src_r, gamc_value.weight, SENSOR_GAMMA_POINT_NUM * 2, ISP_INTERP_UINT16);

		rtn |= isp_interp_data(dst_g, src_g, gamc_value.weight, SENSOR_GAMMA_POINT_NUM * 2, ISP_INTERP_UINT16);

		rtn |= isp_interp_data(dst_b, src_b, gamc_value.weight, SENSOR_GAMMA_POINT_NUM * 2, ISP_INTERP_UINT16);
		if (abl_weighting > 0) {
			gamc_value.weight[0] = (((abl_weighting) * SMART_WEIGHT_UNIT/100)/(SMART_WEIGHT_UNIT / 16)) * (SMART_WEIGHT_UNIT / 16);
			gamc_value.weight[1] = SMART_WEIGHT_UNIT - gamc_value.weight[0];

			src_r[0] = &gamma_curve[2].points_r[0].x;
			src_r[1] = &tmp_dst.points_r[0].x;
			dst_r = &gamma_info->points_r;
			rtn = isp_interp_data(dst_r, src_r, gamc_value.weight, SENSOR_GAMMA_POINT_NUM * 2, ISP_INTERP_UINT16);

			src_g[0] = &gamma_curve[2].points_g[0].x;
			src_g[1] = &tmp_dst.points_g[0].x;
			dst_g = &gamma_info->points_g;
			rtn |= isp_interp_data(dst_g, src_g, gamc_value.weight, SENSOR_GAMMA_POINT_NUM * 2, ISP_INTERP_UINT16);

			src_b[0] = &gamma_curve[2].points_b[0].x;
			src_b[1] = &tmp_dst.points_b[0].x;
			dst_b = &gamma_info->points_b;
			rtn |= isp_interp_data(dst_b, src_b, gamc_value.weight, SENSOR_GAMMA_POINT_NUM * 2, ISP_INTERP_UINT16);

			ISP_LOGV("ABL gamma %d %d\n",gamc_value.weight[0],gamc_value.weight[1]);
		} else {
			memcpy(gamma_info, &tmp_dst, sizeof(struct sensor_rgbgamma_curve));
		}
		ISP_LOGV("atm orig gamma, value %d/%d, x/y %d/%d\n",
			gamc_value.value[0],
			gamc_value.value[1],
			gamma_curve[0].points_g[256].x,
			gamma_curve[0].points_g[256].y);

		if (ISP_SUCCESS != rtn) {
			ISP_LOGE("fail to interp gamc_g data\n");
			return rtn;
		}
	}
	return rtn;
}

static cmr_s32 smart_ctl_calc_atm(smart_handle_t handle,struct smart_proc_input * in_ptr,struct smart_calc_param *param,struct smart_block_result *block_result){
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_int ret = 0;
	int n = 0;
	cmr_u32 atm_enable = 0;
	char value[PROPERTY_VALUE_MAX];
	char value_1[PROPERTY_VALUE_MAX];
	property_get("debug.camera.atm.enable", value, "0");
	//bool bATMEnable = strtol(value, NULL, 10);
	property_get("debug.camera.atm.test", value_1, "0");
	bool bATMTest = strtol(value_1, NULL, 10);
	struct smart_context *cxt = NULL;
	struct sensor_rgbgamma_curve sm_gamma_out;
	struct sensor_rgbgamma_curve atm_gamma_out;
	struct isp_pm_ioctl_output io_pm_output = { NULL, 0 };
	enum smart_ctrl_atm_switch_state atm_switch_state = SMART_CTRL_ATM_SWITCH_ON;

	rtn = check_handle_validate(handle);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to get valid input handle, rtn:%d\n", rtn);
		rtn = ISP_ERROR;

		return rtn;
	}
	cxt = (struct smart_context *)handle;
	if(0 == cxt->atm_version || 1 == cxt->atm_version){
		atm_enable = cxt->atm_tuning_param.atmenable;
	}else if(2 == cxt->atm_version){
		atm_enable = cxt->atm_tuning_param_v1.enable;
	}
	atm_switch_state = cxt->atm_switch_state;

	if (!(strcmp(value, "on"))){
		atm_enable = 1;
	}else if(!(strcmp(value, "off"))){
		atm_enable = 0;
	}else{
		;
	}

	ISP_LOGV("atm_switch_state:%d\n", atm_switch_state);
	if ((atm_enable) && (SMART_CTRL_ATM_SWITCH_ON == atm_switch_state)) {
		ret = _smart_gamma(in_ptr, block_result, &sm_gamma_out);

		if (ISP_SUCCESS == ret) {
			if(0 == cxt->atm_version || 1 == cxt->atm_version){
				ret = _get_atm_curve((cmr_handle *)cxt->handle_atm,
									in_ptr, param->bv,&sm_gamma_out,&atm_gamma_out,
									&cxt->smt_dbginfo.smt_gma,&(cxt->atm_tuning_param));
			}else if(2 == cxt->atm_version){
				ret = _get_atm_curve_v1((cmr_handle *)cxt->handle_atm,
													in_ptr, param->bv,&sm_gamma_out,&atm_gamma_out,
													&cxt->smt_dbginfo.smt_gma,&(cxt->atm_tuning_param_v1));

			}
		}

		if (bATMTest){
			for (n = 0; n < 256; n++ ){
				atm_gamma_out.points_r[n].x = n*4;
				atm_gamma_out.points_r[n].y = 60;
				atm_gamma_out.points_b[n].x = n*4;
				atm_gamma_out.points_b[n].y = 60;
				atm_gamma_out.points_g[n].x = n*4;
				atm_gamma_out.points_g[n].y = 60;
			}
			atm_gamma_out.points_r[256].x = 1023;
			atm_gamma_out.points_r[256].y = 60;
			atm_gamma_out.points_b[256].x = 1023;
			atm_gamma_out.points_b[256].y = 60;
			atm_gamma_out.points_g[256].x = 1023;
			atm_gamma_out.points_g[256].y = 60;
		}

		if (ret == ISP_SUCCESS)
			_smart_write_gamma(cxt, io_pm_output.param_data, &atm_gamma_out);
		else {
			_smart_write_gamma(cxt, io_pm_output.param_data, &sm_gamma_out);
		}

		ISP_LOGV("ATM Enabled\n");
	} else {
		ISP_LOGV("ATM Disabled");
		ret = _smart_gamma(in_ptr, block_result, &sm_gamma_out);

		if (ISP_SUCCESS == ret) {
			_smart_write_gamma(cxt, io_pm_output.param_data,&sm_gamma_out);
		}
	}

	return rtn;
}

static cmr_s32 smart_ctl_calculation(smart_handle_t handle, struct smart_calc_param *param, struct smart_calc_result *result,struct smart_proc_input * in_ptr)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 i = 0;
	struct smart_context *cxt = NULL;
	struct tuning_param *cur_param = NULL;
	struct isp_smart_param *smart_param = NULL;
	struct smart_block_result *blk = NULL;
	cmr_u32 update_block_num = 0;
	enum smart_ctrl_flash_mode flash_mode = SMART_CTRL_FLASH_CLOSE;
	struct smart_block_result *block_result = NULL;

	rtn = check_handle_validate(handle);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to get valid input handle, rtn:%d\n", rtn);
		rtn = ISP_ERROR;

		return rtn;
	}

	cxt = (struct smart_context *)handle;

	if(cxt->smart_lock_frame != 0){
		ISP_LOGI("lock smart frame = %d",cxt->smart_lock_frame);
		param->bv = cxt->smart_stash_param.bv;
		param->bv_gain = cxt->smart_stash_param.bv_gain;
		param->ct = cxt->smart_stash_param.ct ;
		cxt->smart_lock_frame--;
	}else{
		cxt->smart_stash_param.bv = param->bv;
		cxt->smart_stash_param.bv_gain = param->bv_gain;
		cxt->smart_stash_param.ct = param->ct;
	}

	pthread_mutex_lock(&cxt->status_lock);
	ISP_LOGV("SMART_TAG: smart work mode = %d cxt->cur_param = %p", cxt->work_mode, cxt->cur_param);
	if (ISP_MODE_ID_PRV_0 == cxt->work_mode)
		cxt->cur_param = &cxt->tuning_param[ISP_MODE_ID_COMMON];
	else
		cxt->cur_param = &cxt->tuning_param[cxt->work_mode];

	cur_param = cxt->cur_param;
	flash_mode = cxt->flash_mode;

	if (1 == cur_param->bypass) {
		rtn = ISP_SUCCESS;
		goto EXIT;
	}

	smart_param = &cur_param->param;
	smart_ctl_check_param(smart_param);

	for (i = 0; i < smart_param->block_num; i++) {
		cxt->block_result.update = 0;
		rtn = smart_ctl_calc_block(&smart_param->block[i], param, &cxt->block_result, flash_mode);

		if (1 == cxt->block_result.update) {
			cxt->calc_result[update_block_num] = cxt->block_result;
			update_block_num++;
		}
	}

	result->counts = update_block_num;
	result->block_result = cxt->calc_result;

	//do atm
	for (i = 0; i < result->counts; i++) {
		ISP_LOGV("do ATM");
		block_result = &result->block_result[i];
		if (block_result->block_id == ISP_BLK_RGB_GAMC) {
			if ((block_result->mode_flag != in_ptr->mode_flag) || (block_result->scene_flag != in_ptr->scene_flag)) {
				block_result->mode_flag_changed = 1;
				block_result->mode_flag = in_ptr->mode_flag;
				block_result->scene_flag = in_ptr->scene_flag;
			}
			rtn = smart_ctl_calc_atm(handle,in_ptr,param,block_result);
			} else
				continue;
	}
	ISP_LOGV("bv=%d, ct=%d, flash=%d", param->bv, param->ct, flash_mode);
	smart_ctl_print_smart_result(cxt->flash_mode, result);
	smart_ctl_print_debug_file(handle,cxt->debug_file, param, result, (char *)cxt->debug_buf,&(cxt->smt_dbginfo.smt_gma));
	for (i = 0; i < result->counts; i++) {
		blk = &result->block_result[i];
		if (blk->smart_id > 8) {
			switch (blk->smart_id) {
			case 9:
			    cxt->smt_dbginfo.nr_param.ppi[0] = 1;
			    cxt->smt_dbginfo.nr_param.ppi[2] = blk->component[0].fix_data[0];
			    break;
			case 10:
			    cxt->smt_dbginfo.nr_param.bayer_nr[0] = 1;
			    cxt->smt_dbginfo.nr_param.bayer_nr[2] = blk->component[0].fix_data[0];
			    break;
			case 11:
			    cxt->smt_dbginfo.nr_param.rgb_dither[0] = 1;
			    cxt->smt_dbginfo.nr_param.rgb_dither[2] = blk->component[0].fix_data[0];
			    break;
			case 12:
			    cxt->smt_dbginfo.nr_param.bpc[0] = 1;
			    cxt->smt_dbginfo.nr_param.bpc[2] = blk->component[0].fix_data[0];
			    break;
			case 13:
			    cxt->smt_dbginfo.nr_param.grgb[0] = 1;
			    cxt->smt_dbginfo.nr_param.grgb[2] = blk->component[0].fix_data[0];
			    break;
			case 14:
			    cxt->smt_dbginfo.nr_param.cfae[0] = 1;
			    cxt->smt_dbginfo.nr_param.cfae[2] = blk->component[0].fix_data[0];
			    break;
			case 15:
			    cxt->smt_dbginfo.nr_param.rgb_afm[0] = 1;
			    cxt->smt_dbginfo.nr_param.rgb_afm[2] = blk->component[0].fix_data[0];
			    break;
			case 16:
			    cxt->smt_dbginfo.nr_param.uvdiv[0] = 1;
			    cxt->smt_dbginfo.nr_param.uvdiv[2] = blk->component[0].fix_data[0];
			    break;
			case 17:
			    cxt->smt_dbginfo.nr_param.dnr3_pre[0] = 1;
			    cxt->smt_dbginfo.nr_param.dnr3_pre[2] = blk->component[0].fix_data[0];
			    break;
			case 18:
			    cxt->smt_dbginfo.nr_param.dnr3_cap[0] = 1;
			    cxt->smt_dbginfo.nr_param.dnr3_cap[2] = blk->component[0].fix_data[0];
			    break;
			case 19:
			    cxt->smt_dbginfo.nr_param.edge[0] = 1;
			    cxt->smt_dbginfo.nr_param.edge[2] = blk->component[0].fix_data[0];
			    break;
			case 20:
			    cxt->smt_dbginfo.nr_param.precdn[0] = 1;
			    cxt->smt_dbginfo.nr_param.precdn[2] = blk->component[0].fix_data[0];
			    break;
			case 21:
			    cxt->smt_dbginfo.nr_param.ynr[0] = 1;
			    cxt->smt_dbginfo.nr_param.ynr[2] = blk->component[0].fix_data[0];
			    break;
			case 22:
			    cxt->smt_dbginfo.nr_param.cdn[0] = 1;
			    cxt->smt_dbginfo.nr_param.cdn[2] = blk->component[0].fix_data[0];
			    break;
			case 23:
			    cxt->smt_dbginfo.nr_param.postcdn[0] = 1;
			    cxt->smt_dbginfo.nr_param.postcdn[2] = blk->component[0].fix_data[0];
			    break;
			case 24:
			    cxt->smt_dbginfo.nr_param.ccnr[0] = 1;
			    cxt->smt_dbginfo.nr_param.ccnr[2] = blk->component[0].fix_data[0];
			    break;
			case 25:
			    cxt->smt_dbginfo.nr_param.iir_yrandom[0] = 1;
			    cxt->smt_dbginfo.nr_param.iir_yrandom[2] = blk->component[0].fix_data[0];
			    break;
			case 26:
			    cxt->smt_dbginfo.nr_param.noisefilter[0] = 1;
			    cxt->smt_dbginfo.nr_param.noisefilter[2] = blk->component[0].fix_data[0];
			    break;
			case ISP_SMART_LTM:
			    cxt->smt_dbginfo.nr_param.ltm[0] = 1;
			    cxt->smt_dbginfo.nr_param.ltm[2] = blk->component[0].fix_data[0];
			    break;
			case ISP_SMART_IMBALANCE:
			    cxt->smt_dbginfo.nr_param.imbalance[0] = 1;
			    cxt->smt_dbginfo.nr_param.imbalance[2] = blk->component[0].fix_data[0];
			    break;
			case ISP_SMART_3DNR:
			    cxt->smt_dbginfo.nr_param.threednr[0] = 1;
			    cxt->smt_dbginfo.nr_param.threednr[2] = blk->component[0].fix_data[0];
			    break;
			case ISP_SMART_MFNR:
			    cxt->smt_dbginfo.nr_param.mfnr[0] = 1;
			    cxt->smt_dbginfo.nr_param.mfnr[2] = blk->component[0].fix_data[0];
			    break;
			default:
			    break;
			}
		}
	}

EXIT:

	pthread_mutex_unlock(&cxt->status_lock);
	return rtn;
}

cmr_s32 smart_ctl_deinit(smart_handle_t * handle, void *param, void *result)
{
	UNUSED(param);
	UNUSED(result);
	cmr_s32 rtn = ISP_SUCCESS;
	struct smart_context *cxt_ptr = *handle;

	rtn = check_handle_validate(*handle);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to check handle, rtn  %d\n", rtn);
		rtn = ISP_ERROR;
		goto ERROR_EXIT;
	}
#ifdef GATM_ENABLE
	if (NULL != cxt_ptr->handle_atm) {
	    isp_atm_deinit((cmr_handle*)&cxt_ptr->handle_atm);
	}
#endif

	rtn = smart_ctl_save_stash(cxt_ptr, NULL);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to save smart, rtn  %d\n", rtn);
		rtn = ISP_ERROR;
		goto ERROR_EXIT;
	}

	if (NULL != cxt_ptr->debug_file) {
		smart_debug_file_deinit(cxt_ptr->debug_file);
		cxt_ptr->debug_file = NULL;
	}

	pthread_mutex_destroy(&cxt_ptr->status_lock);

ERROR_EXIT:
	if (cxt_ptr) {
		free((void *)cxt_ptr);
		*handle = NULL;
	}
	ISP_LOGI("done %d", rtn);
	return rtn;
}

cmr_s32 smart_ctl_ioctl(smart_handle_t handle, cmr_u32 cmd, void *param, void *result)
{
	UNUSED(result);
	cmr_s32 rtn = ISP_SUCCESS;
	struct smart_context *cxt_ptr = NULL;

	rtn = check_handle_validate(handle);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to check handle, rtn %d\n", rtn);
		return rtn;
	}

	cxt_ptr = (struct smart_context *)handle;
	pthread_mutex_lock(&cxt_ptr->status_lock);

	switch (cmd) {
	case ISP_SMART_IOCTL_SET_WORK_MODE:
		rtn = smart_ctl_set_workmode(cxt_ptr, param);
		break;

	case ISP_SMART_IOCTL_SET_FLASH_MODE:
		rtn = smart_ctl_set_flash(cxt_ptr, param);
		break;

	case ISP_SMART_IOCTL_GET_UPDATE_PARAM:
		rtn = smart_ctl_get_update_param(cxt_ptr, param);
		break;

	case ISP_SMART_IOCTL_SET_ATM_SWITCH_STATE:
		rtn = smart_ctl_set_atm_switch_state(cxt_ptr, param);
		break;

	default:
		ISP_LOGE("fail to get valid cmd %d", cmd);
		rtn = ISP_ERROR;
		break;
	}

ERROR_EXIT:

	pthread_mutex_unlock(&cxt_ptr->status_lock);
	return rtn;
}

cmr_s32 smart_ctl_block_eb(smart_handle_t handle, void *block_eb, cmr_u32 is_eb)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 i = 0;
	cmr_s16 *block_eb_ptr = (cmr_s16 *) block_eb;
	struct smart_context *cxt = NULL;
	struct tuning_param *cur_param = NULL;
	struct isp_smart_param *smart_param = NULL;

	rtn = check_handle_validate(handle);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to get input handle");
		return ISP_SUCCESS;
	}

	cxt = (struct smart_context *)handle;

	if (ISP_MODE_ID_PRV_0 == cxt->work_mode) {
		cxt->cur_param = &cxt->tuning_param[ISP_MODE_ID_COMMON];
	} else {
		cxt->cur_param = &cxt->tuning_param[cxt->work_mode];
	}

	cur_param = cxt->cur_param;

	if (1 == cur_param->bypass) {
		ISP_LOGV("current paramter is bypass");
		return ISP_SUCCESS;
	}

	smart_param = &cur_param->param;

	if (ISP_SMART_MAX_BLOCK_NUM < smart_param->block_num) {
		ISP_LOGE("fail to get smart block number %d", smart_param->block_num);
		return ISP_ERROR;
	}

	for (i = 0; i < smart_param->block_num; i++) {
		if (ISP_SMART_LNC == smart_param->block[i].smart_id || ISP_SMART_CMC == smart_param->block[i].smart_id || ISP_SMART_GAMMA == smart_param->block[i].smart_id) {
			if (is_eb) {
				smart_param->block[i].enable = *block_eb_ptr;
			} else {
				*block_eb_ptr = smart_param->block[i].enable;
				smart_param->block[i].enable = 0;
			}
		}
		block_eb_ptr++;
	}

	return rtn;
}

cmr_s32 smart_ctl_block_enable_recover(smart_handle_t handle, cmr_u32 smart_id)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 i = 0;
	struct smart_context *cxt = NULL;
	struct tuning_param *cur_param = NULL;
	struct tuning_param *org_param = NULL;
	struct isp_smart_param *smart_param = NULL;

	rtn = check_handle_validate(handle);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to get input handle");
		return ISP_ERROR;
	}

	cxt = (struct smart_context *)handle;

	for (int j = 0; j < SMART_MAX_WORK_MODE; j++) {
		if(cxt->tuning_param[j].param.block_num > 0) {
			cxt->cur_param = &cxt->tuning_param[j];
			org_param = &cxt->tuning_param_org[j];

			cur_param = cxt->cur_param;

			if (1 == cur_param->bypass) {
				ISP_LOGV("current paramter is bypass");
				return ISP_SUCCESS;
			}

			smart_param = &cur_param->param;

			if (ISP_SMART_MAX_BLOCK_NUM < smart_param->block_num) {
				ISP_LOGE("fail to get smart block number %d", smart_param->block_num);
				return ISP_ERROR;
			}

			for (i = 0; i < smart_param->block_num; i++) {
				if (smart_id == smart_param->block[i].smart_id) {
					smart_param->block[i].enable = org_param->param.block[i].enable;
					ISP_LOGE("YJW:%d\n",smart_id);
					break;
				}
			}
		}
	}
	return rtn;
}

cmr_s32 smart_ctl_NR_block_disable(smart_handle_t handle, cmr_u32 is_diseb)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 i = 0;
	struct smart_context *cxt = NULL;
	struct tuning_param *cur_param = NULL;
	struct isp_smart_param *smart_param = NULL;
	struct tuning_param *org_param = NULL;


	rtn = check_handle_validate(handle);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to get input handle");
		return ISP_ERROR;
	}

	cxt = (struct smart_context *)handle;

	if (ISP_MODE_ID_PRV_0 == cxt->work_mode) {
		cxt->cur_param = &cxt->tuning_param[ISP_MODE_ID_COMMON];
		org_param = &cxt->tuning_param_org[ISP_MODE_ID_COMMON];
	} else {
		cxt->cur_param = &cxt->tuning_param[cxt->work_mode];
		org_param = &cxt->tuning_param_org[cxt->work_mode];
	}

	cur_param = cxt->cur_param;

	if (1 == cur_param->bypass) {
		ISP_LOGV("current paramter is bypass");
		return ISP_SUCCESS;
	}

	smart_param = &cur_param->param;

	if (ISP_SMART_MAX_BLOCK_NUM < smart_param->block_num) {
		ISP_LOGE("fail to get smart block number %d", smart_param->block_num);
		return ISP_ERROR;
	}

	for (i = 0; i < smart_param->block_num; i++) {
			if (ISP_SMART_EDGE == smart_param->block[i].smart_id ||
				ISP_SMART_PRFY== smart_param->block[i].smart_id ||
				ISP_SMART_BPC == smart_param->block[i].smart_id ||
				ISP_SMART_NLM == smart_param->block[i].smart_id ||
				ISP_SMART_RGB_PRECDN == smart_param->block[i].smart_id ||
				ISP_SMART_YUV_PRECDN == smart_param->block[i].smart_id ||
				ISP_SMART_UV_POSTCDN == smart_param->block[i].smart_id ||
				ISP_SMART_IIRCNR_IIR == smart_param->block[i].smart_id ||
				ISP_SMART_BDN == smart_param->block[i].smart_id ||
				ISP_SMART_UVDIV == smart_param->block[i].smart_id ||
				ISP_SMART_IIR_YRANDOM == smart_param->block[i].smart_id ||
				ISP_SMART_RGB_AFM == smart_param->block[i].smart_id||
				ISP_SMART_PDAF_CORRECT == smart_param->block[i].smart_id ||
				ISP_SMART_RGB_DITHER == smart_param->block[i].smart_id ||
				ISP_SMART_GRGB == smart_param->block[i].smart_id ||
				ISP_SMART_UVDIV == smart_param->block[i].smart_id ||
				ISP_SMART_3DNR_PRE == smart_param->block[i].smart_id ||
				ISP_SMART_3DNR_CAP == smart_param->block[i].smart_id ||
				ISP_SMART_YNR == smart_param->block[i].smart_id ||
				ISP_SMART_UVCDN == smart_param->block[i].smart_id ||
				ISP_SMART_YUV_NOISEFILTER == smart_param->block[i].smart_id||
				ISP_SMART_LTM == smart_param->block[i].smart_id ||
				ISP_SMART_IMBALANCE == smart_param->block[i].smart_id||
				ISP_SMART_3DNR == smart_param->block[i].smart_id ||
				ISP_SMART_MFNR == smart_param->block[i].smart_id) {
				if (is_diseb){
					smart_param->block[i].enable = 0;
					} else {
					smart_param->block[i].enable = org_param->param.block[i].enable;
					}
				}
			}

	return rtn;
}

cmr_s32 smart_ctl_block_disable(smart_handle_t handle, cmr_u32 smart_id)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 i = 0;
	struct smart_context *cxt = NULL;
	struct tuning_param *cur_param = NULL;
	struct isp_smart_param *smart_param = NULL;

	rtn = check_handle_validate(handle);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to get input handle");
		return ISP_ERROR;
	}

	cxt = (struct smart_context *)handle;

	for (int j = 0; j < SMART_MAX_WORK_MODE; j++) {
		if(cxt->tuning_param[j].param.block_num > 0) {
			cxt->cur_param = &cxt->tuning_param[j];

			cur_param = cxt->cur_param;

			if (1 == cur_param->bypass) {
				ISP_LOGV("current paramter is bypass");
				return ISP_SUCCESS;
			}

			smart_param = &cur_param->param;

			if (ISP_SMART_MAX_BLOCK_NUM < smart_param->block_num) {
				ISP_LOGE("fail to get smart block number %d", smart_param->block_num);
				return ISP_ERROR;
			}

			for (i = 0; i < smart_param->block_num; i++) {
				if (smart_id == smart_param->block[i].smart_id) {
					smart_param->block[i].enable = 0;
					break;
				}
			}
		}
	}

	return rtn;
}

cmr_int _smart_calc(cmr_handle handle_smart, struct smart_proc_input * in_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	struct smart_calc_param *smart_calc_param = NULL;
	struct smart_calc_result smart_calc_result = { NULL, 0 };
	struct isp_pm_ioctl_input io_pm_input = { NULL, 0 };
	struct isp_pm_param_data pm_param = { 0, 0, 0, NULL, 0, {0} };
	struct smart_block_result *block_result = NULL;
	struct smart_context *cxt = NULL;
	cmr_u32 alc_awb = 0;
	cmr_u32 i = 0;

	if (!handle_smart || !in_ptr) {
		rtn = ISP_PARAM_ERROR;
		goto exit;
	}

	smart_calc_param = &in_ptr->cal_para;
	alc_awb = in_ptr->alc_awb;
	rtn = smart_ctl_calculation(handle_smart, smart_calc_param, &smart_calc_result,in_ptr);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to init smart");
		return rtn;
	}

	cxt = (struct smart_context *)handle_smart;

	in_ptr->log = (cmr_u8 *) & (cxt->smt_dbginfo);
	in_ptr->size = (sizeof(cxt->smt_dbginfo));

	//use LSC_SPD_VERSION to control the output of smart lsc
	if (in_ptr->lsc_sprd_version >= 3) {
		for (i = 0; i < smart_calc_result.counts; i++) {
			if (ISP_SMART_LNC == smart_calc_result.block_result[i].smart_id) {
				smart_calc_result.block_result[i].update = 0;
			}
		}
	}

	// lock nr block
	if (in_ptr->lock_nlm == 1) {
		for (i = 0; i < smart_calc_result.counts; i++) {
			if (ISP_SMART_NLM == smart_calc_result.block_result[i].smart_id) {
				smart_calc_result.block_result[i].update = 0;
			}
		}
	}

	if (in_ptr->lock_ee == 1) {
		for (i = 0; i < smart_calc_result.counts; i++) {
			if (ISP_SMART_EDGE== smart_calc_result.block_result[i].smart_id) {
				smart_calc_result.block_result[i].update = 0;
			}
		}
	}

	if (in_ptr->lock_precdn == 1) {
		for (i = 0; i < smart_calc_result.counts; i++) {
			if (ISP_SMART_YUV_PRECDN== smart_calc_result.block_result[i].smart_id) {
				smart_calc_result.block_result[i].update = 0;
			}
		}
	}

	if (in_ptr->lock_cdn == 1) {
		for (i = 0; i < smart_calc_result.counts; i++) {
			if (ISP_SMART_UVCDN== smart_calc_result.block_result[i].smart_id) {
				smart_calc_result.block_result[i].update = 0;
			}
		}
	}

	if (in_ptr->lock_postcdn == 1) {
		for (i = 0; i < smart_calc_result.counts; i++) {
			if (ISP_SMART_UV_POSTCDN== smart_calc_result.block_result[i].smart_id) {
				smart_calc_result.block_result[i].update = 0;
			}
		}
	}

	if (in_ptr->lock_ccnr== 1) {
		for (i = 0; i < smart_calc_result.counts; i++) {
			if (ISP_SMART_IIRCNR_IIR== smart_calc_result.block_result[i].smart_id) {
				smart_calc_result.block_result[i].update = 0;
			}
		}
	}

	if (in_ptr->lock_ynr== 1) {
		for (i = 0; i < smart_calc_result.counts; i++) {
			if (ISP_SMART_YNR== smart_calc_result.block_result[i].smart_id) {
				smart_calc_result.block_result[i].update = 0;
			}
		}
	}
	if(in_ptr->lock_ltm == 1) {
		for (i = 0; i < smart_calc_result.counts; i++) {
                        if (ISP_SMART_LTM== smart_calc_result.block_result[i].smart_id) {
                                smart_calc_result.block_result[i].update = 0;
			}
		}
	}
	if(in_ptr->lock_imbalance == 1) {
                for (i = 0; i < smart_calc_result.counts; i++) {
                        if (ISP_SMART_IMBALANCE== smart_calc_result.block_result[i].smart_id) {
                                smart_calc_result.block_result[i].update = 0;
			}
		}
	}

	/*use alc ccm, disable spreadtrum smart ccm */
	for (i = 0; i < smart_calc_result.counts; i++) {
		switch (smart_calc_result.block_result[i].smart_id) {
		case ISP_SMART_CMC:
			if (alc_awb & (1 << 0)) {
				smart_calc_result.block_result[i].update = 0;
			}
			break;
		case ISP_SMART_LNC:
			if (alc_awb & (1 << 8)) {
				smart_calc_result.block_result[i].update = 0;
			}
			break;
		case ISP_SMART_COLOR_CAST:
			if (alc_awb & (1 << 0)) {
				smart_calc_result.block_result[i].update = 0;
			}
			break;
		case ISP_SMART_GAIN_OFFSET:
			if (alc_awb & (1 << 0)) {
				smart_calc_result.block_result[i].update = 0;
			}
			break;
//		case ISP_SMART_SATURATION_DEPRESS:
//			smart_calc_result.block_result[i].block_id = ISP_BLK_SATURATION;
//			break;
//		case ISP_SMART_GAMMA:
//		    smart_calc_result.block_result[i].update = 0;
//		    break;
		default:
			break;
		}
	}

	for (i = 0; i < smart_calc_result.counts; i++) {
		block_result = &smart_calc_result.block_result[i];
		if ((block_result->mode_flag != in_ptr->mode_flag) || (block_result->scene_flag != in_ptr->scene_flag)) {
			block_result->mode_flag_changed = 1;
			block_result->mode_flag = in_ptr->mode_flag;
			block_result->scene_flag = in_ptr->scene_flag;
		}
		block_result->ai_scene_id = in_ptr->ai_scene_id;
		block_result->ai_scene_pro_flag = in_ptr->ai_scene_pro_flag;

#if !defined(CONFIG_ISP_2_4)
		if (block_result->block_id == ISP_BLK_RGB_GAMC)
		    block_result->update = 0;
#endif

		/* If mw implements cb, then set pm in callback. */
		if (cxt->smart_set_cb)
			continue;

		pm_param.cmd = ISP_PM_BLK_SMART_SETTING;
		pm_param.id = block_result->block_id;
		pm_param.data_size = sizeof(*block_result);
		pm_param.data_ptr = (void *)block_result;

		io_pm_input.param_data_ptr = &pm_param;
		io_pm_input.param_num = 1;

		ISP_LOGV("ISP_SMART: set param %d, id=%x, data=%p, size=%d", i, pm_param.id, pm_param.data_ptr, pm_param.data_size);
		rtn = isp_pm_ioctl(in_ptr->handle_pm, ISP_PM_CMD_SET_SMART, &io_pm_input, NULL);
#ifdef Y_GAMMA_SMART_WITH_RGB_GAMMA
		if (ISP_BLK_RGB_GAMC == pm_param.id) {
			pm_param.id = ISP_BLK_Y_GAMMC;
			rtn = isp_pm_ioctl(in_ptr->handle_pm, ISP_PM_CMD_SET_SMART, &io_pm_input, NULL);
		}
#endif
	}

	/* If mw implements cb, then set pm in callback. */
	if (cxt->smart_set_cb) {
		rtn = cxt->smart_set_cb(cxt->caller_handle, ISP_SMART_SET_COMMON, &smart_calc_result, NULL);
	}

#if !defined(CONFIG_ISP_2_4)
	// reset to modified gamma

#endif // !defined(CONFIG_ISP_2_4)
exit:
	ISP_LOGV("done %ld", rtn);
	return rtn;
}
