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

#define LOG_TAG "awb_adpt"
#define ATRACE_TAG (ATRACE_TAG_CAMERA | ATRACE_TAG_HAL)
#include <cutils/trace.h>

#include "awb_ctrl.h"
#include "awb_sprd_adpt.h"
//#include "awb.h"
#include "awblib.h"
#include "isp_adpt.h"
#include "dlfcn.h"
#include <cutils/properties.h>
#include <math.h>
//#include "isp_awb_queue.h"
#include "isp_debug.h"
#include <libloader.h>

#include <utils/Timers.h>

#define PI     3.14159
#define AWB_CTRL_MAGIC_BEGIN		0xe5a55e5a
#define AWB_CTRL_MAGIC_END		0x5e5ae5a5
#define AWB_CTRL_RESOLUTION_NUM 	8
#define AWB_CTRL_SCENEMODE_NUM	10

#define AWB_GAIN_PARAM_FILE_NAME_CAMERASERVER "/data/vendor/cameraserver/"
#define AWB_GAIN_PARAM_FILE_NAME_MEDIA "/data/misc/media/"

#define AWB_CTRL_TRUE			1
#define AWB_CTRL_FALSE			0
#define AWB_CTRL_LOCKMODE 1
#define AWB_CTRL_UNLOCKMODE 0
#define AWB_CTRL_SAFE_FREE(_p) \
do { \
	if (NULL != (_p)) {\
		free(_p); \
		_p = NULL; \
	} \
}while(0)

#define __MIN(a,b) (((a)<(b))?(a):(b))

static char libawb_path[][30] = {
	"libawb1.so",			//isp2.x lib
	"libawb.so",			//isp3.x.lib
	"libawb3.2.so",			//isp3.2 lib
	"libawb_v2.so",
	"libawb_v3.so",
	"libawb_v4.so",
	"libawb_v5.so",
};
//awblib.so api
struct awbsprd_lib_ops {
	//awblib_3.x port
	void *(*awb_init_v3) (struct awb_init_param_3_0 *init_param, struct awb_rgb_gain_3_0 *gain);
	cmr_s32(*awb_calc_v3) (void *awb_handle, struct awb_calc_param_3_0 * calc_param, struct awb_calc_result_3_0 * calc_result);
	cmr_s32(*awb_ioctrl_v3) (void *awb_handle, cmr_s32 cmd, void *param, void *out);
	cmr_s32(*awb_deinit_v3) (void *awb_handle);
};

struct awb_gain_queue {
	void *ct;
	void *r_gain;
	void *g_gain;
	void *b_gain;
	void *weight;

	cmr_u32 size;
};

struct awb_ae_stat {
	cmr_u32 r_info[16384];
	cmr_u32 g_info[16384];
	cmr_u32 b_info[16384];
};

struct awb_ctrl_cxt {
	/*must be the first one */
	cmr_u32 magic_begin;
	/*awb status lock */
	pthread_mutex_t status_lock;
	/*initialize parameter */
	struct awb_ctrl_init_param init_param;
	struct awb_init_param_3_0 awb_init_param_v3;
	/*camera id */
	cmr_u32 camera_id;			/* 0: back camera, 1: front camera */
	/*work mode */
	cmr_u32 work_mode;			/* 0: preview, 1:capture, 2:video */
	cmr_u32 param_index;		/* tuning param index */
	cmr_u32 snap_lock;			/* record lock awb frames after snapshot    */
	/*white balance mode: auto or manual */
	enum awb_ctrl_wb_mode wb_mode;
	/*scene mode */
	enum awb_ctrl_scene_mode scene_mode;
	/*format of statistic image */
	enum awb_ctrl_stat_img_format stat_img_format;
	/*statistic image size */
	struct awb_ctrl_size stat_img_size;
	struct awb_ctrl_size stat_win_size;

	cmr_u32 flash_update_awb;
	struct awb_ctrl_opt_info otp_info;
	/*flash info */
	struct awb_flash_info flash_info;
	/* ctrl touch flash awb */
	cmr_u32 flash_pre_state;
	/*current gain */
	struct awb_ctrl_gain cur_gain;
	/*current offset */
	struct awb_ctrl_offset cur_offset;
	/*output gain */
	struct awb_ctrl_gain output_gain;
	/*output ct */
	cmr_u32 output_ct;
	/*output_ct_mean*/
	int output_ct_mean;
	/*recover gain */
	struct awb_ctrl_gain recover_gain;
	/*recover ct */
	cmr_u32 recover_ct;
	/*recover awb mode */
	enum awb_ctrl_wb_mode recover_mode;
	/*awb lock info */
	struct awb_ctrl_lock_info lock_info;
	/*current ct */
	cmr_u32 cur_ct;
	/*current tint*/
	int cur_tint;
	/*algorithm handle */
	void *alg_handle;
	void *lib_handle;
	struct awbsprd_lib_ops lib_ops;
	struct third_lib_info *lib_info;
	struct awb_gain_queue gain_queue;
	cmr_u8 *log;
	cmr_u32 size;
	cmr_u32 frame_count;

	/*
	 * for dual camera sync
	 */
	cmr_u8 sensor_role;
	cmr_u32 is_multi_mode;
	cmr_u32 is_mono_sensor;
	func_isp_br_ioctrl ptr_isp_br_ioctrl;
	enum sensor_role_type sensor_role_type;
	struct awb_ctrl_calc_result awb_result;

	/*must be the last one */
	cmr_u32 magic_end;
	cmr_u32 color_support;
	struct awb_ae_stat master_ae_stat;
	struct awb_ae_stat slave_ae_stat;
	struct awb_aiscene_info_3_0 ai_scene_info_v3;
	struct awb_face_info_3_0 awb_face_info_v3;

	/*for save gain/ct to file*/
	struct awb_save_gain s_save_awb_param;
	struct awb_ctrl_gain gain_to_save;
	cmr_u32 ct_to_save;
	struct awb_ctrl_gain gain_to_save_auto;
	cmr_u32 ct_auto_to_save;
	cmr_u32 flash_awb_en;

};

static struct isp_size g_src_size[4] = {{0,0},{0,0},{0,0},{0,0}};
static struct awb_ctrl_size g_stat_img_size[4] = {{0,0},{0,0},{0,0},{0,0}};

static cmr_u32 _awb_get_gain(struct awb_ctrl_cxt *cxt, void *param);

static cmr_u32 _check_handle(awb_ctrl_handle_t handle)
{
	cmr_u32 rtn = AWB_CTRL_SUCCESS;
	struct awb_ctrl_cxt *cxt = (struct awb_ctrl_cxt *)handle;

	if (NULL == cxt) {
		ISP_LOGE("fail to get awb cxt");
		return AWB_CTRL_ERROR;
	}

	if (AWB_CTRL_MAGIC_BEGIN != cxt->magic_begin || AWB_CTRL_MAGIC_END != cxt->magic_end) {
		ISP_LOGE("fail to get invalid magic, begin = 0x%x, end = 0x%x", cxt->magic_begin, cxt->magic_end);
		return AWB_CTRL_ERROR;
	}

	return rtn;
}

static cmr_u32 _check_init_param(struct awb_ctrl_init_param *param)
{
	cmr_u32 rtn = AWB_CTRL_SUCCESS;
	UNUSED(param);
	return rtn;
}

static cmr_u32 _awb_set_gain_manualwb_v3(struct awb_ctrl_cxt *cxt)
{
	cmr_u32 rtn = AWB_CTRL_SUCCESS;
	cmr_u32 mawb_id = cxt->wb_mode;
	struct awb_rgb_gain_3_0 out_gain_mwb;
	if (AWB_CTRL_WB_MODE_AUTO != cxt->wb_mode) {
		if (mawb_id == AWB_CTRL_AWB_MODE_OFF) {
			cxt->output_gain.r = 1024;
			cxt->output_gain.g = 1024;
			cxt->output_gain.b = 1024;
			cxt->output_ct = 5000;
			cxt->output_ct_mean = 5000;
		} else if ((mawb_id > 0) && (mawb_id < 10))	 {
			// return mwb by mwb mode id
			//sunny,cloudy,and other module
			rtn = cxt->lib_ops.awb_ioctrl_v3(cxt->alg_handle, AWB_IOCTRL_GET_MWB_BY_MODEID_3_0, &mawb_id, &out_gain_mwb);
			cxt->output_gain.r = out_gain_mwb.r_gain;
			cxt->output_gain.g = out_gain_mwb.g_gain;
			cxt->output_gain.b = out_gain_mwb.b_gain;
			cxt->output_ct     = out_gain_mwb.ct;
			cxt->output_ct_mean = out_gain_mwb.ct;
			ISP_LOGV("get_rgb_gain by mwb mode id in awb_ioctrl, r_gain = %d, g_gain= %d, b_gain = %d ct = %d", cxt->output_gain.r, cxt->output_gain.g, cxt->output_gain.b, cxt->output_ct);
		} else {
			// return mwb by ct, (100K <= ct < 10000K)
			// To set output_gain/output_ct by CT
			rtn = cxt->lib_ops.awb_ioctrl_v3(cxt->alg_handle, AWB_IOCTRL_GET_MWB_BY_CT_3_0, &mawb_id, &out_gain_mwb);
			cxt->output_gain.r = out_gain_mwb.r_gain;
			cxt->output_gain.g = out_gain_mwb.g_gain;
			cxt->output_gain.b = out_gain_mwb.b_gain;
			cxt->output_ct     = out_gain_mwb.ct;
			cxt->output_ct_mean = out_gain_mwb.ct;
			ISP_LOGV("get_rgb_gain by mwb ct in awb_ioctrl, r_gain = %d, g_gain= %d, b_gain = %d ct = %d", cxt->output_gain.r, cxt->output_gain.g, cxt->output_gain.b, cxt->output_ct);
		}
		if ((cxt->otp_info.gldn_stat_info.r != 0) && (cxt->otp_info.gldn_stat_info.g != 0) && (cxt->otp_info.gldn_stat_info.b != 0)) {
			double otp_g_coef = (double)cxt->otp_info.rdm_stat_info.g / cxt->otp_info.gldn_stat_info.g;
			double otp_r_coef = (double)cxt->otp_info.rdm_stat_info.r / cxt->otp_info.gldn_stat_info.r;
			double otp_b_coef = (double)cxt->otp_info.rdm_stat_info.b / cxt->otp_info.gldn_stat_info.b;
			if (otp_g_coef != 0) {
				otp_r_coef = otp_r_coef / otp_g_coef;
				otp_b_coef = otp_b_coef / otp_g_coef;
				cxt->output_gain.r = (cmr_u32)(cxt->output_gain.r * otp_r_coef);
				cxt->output_gain.b = (cmr_u32)(cxt->output_gain.b * otp_b_coef);
			}
		}
		cxt->awb_result.gain.r = cxt->output_gain.r;
		cxt->awb_result.gain.g = cxt->output_gain.g;
		cxt->awb_result.gain.b = cxt->output_gain.b;
		cxt->awb_result.ct = cxt->output_ct;
	}
	else {
		ISP_LOGE("wb mode need manual mode");
		rtn = AWB_CTRL_ERROR;
	}
	return rtn;
}

static void _awb_save_gain(struct awb_save_gain *cxt, struct awb_ctrl_cxt *cxt_tmp)
{
	FILE *fp = NULL;
	char version[1024];
	char file_name[1024];
	property_get("ro.build.version.release", version, "");

	if (atoi(version) > 6) {
		snprintf(file_name, 1024, "%scamera_%d_awb.file", AWB_GAIN_PARAM_FILE_NAME_CAMERASERVER, cxt_tmp->camera_id);
		fp = fopen(file_name, "wb");
		if (fp) {
			ISP_LOGV("save_gain to:%s, camer_id[%d]: %d, %d, %d, %d\n", file_name, cxt_tmp->camera_id, cxt->r, cxt->g, cxt->b, cxt->ct);
			fwrite((char *)cxt, 1, sizeof(struct awb_save_gain), fp);
			fclose(fp);
			fp = NULL;
		}
	} else {
		snprintf(file_name, 1024, "%scamera_%d_awb.file", AWB_GAIN_PARAM_FILE_NAME_MEDIA, cxt_tmp->camera_id);
		fp = fopen(file_name, "wb");
		if (fp) {
			ISP_LOGV("save_gain to:%s, camera_id[%d]: %d, %d, %d, %d\n", file_name, cxt_tmp->camera_id, cxt->r, cxt->g, cxt->b, cxt->ct);
			fwrite((char *)cxt, 1, sizeof(struct awb_save_gain), fp);
			fclose(fp);
			fp = NULL;
		}
	}
}
static int _awb_save_gain_tofile(struct awb_ctrl_cxt *cxt)
{
	cmr_u32 rtn = AWB_CTRL_SUCCESS;
	if (cxt->flash_info.flash_enable == 0) {
		cxt->recover_gain.r = cxt->gain_to_save_auto.r;
		cxt->recover_gain.g = cxt->gain_to_save_auto.g;
		cxt->recover_gain.b = cxt->gain_to_save_auto.b;
		cxt->recover_ct		= cxt->ct_auto_to_save;
		cxt->s_save_awb_param.r = cxt->recover_gain.r;
		cxt->s_save_awb_param.g = cxt->recover_gain.g;
		cxt->s_save_awb_param.b = cxt->recover_gain.b;
		cxt->s_save_awb_param.ct = cxt->recover_ct;
		_awb_save_gain(&(cxt->s_save_awb_param), cxt);
	}
	return rtn;
}
static void _awb_read_gain(struct awb_save_gain *cxt, struct awb_ctrl_cxt *cxt_tmp)
{
	unsigned long count = 0;
	FILE *fp = NULL;
	char version[1024];
	char file_name[1024];
	property_get("ro.build.version.release", version, "");

	if (atoi(version) > 6) {
		snprintf(file_name, 1024, "%scamera_%d_awb.file", AWB_GAIN_PARAM_FILE_NAME_CAMERASERVER, cxt_tmp->camera_id);
		fp = fopen(file_name, "rb");
		if (fp) {
			memset((void *)cxt, 0, sizeof(struct awb_save_gain));
			count = fread((char *)cxt, 1, sizeof(struct awb_save_gain), fp);
			if(count < (sizeof(struct awb_save_gain)))
				ISP_LOGE("_awb_read_gain:fread count error!");
			fclose(fp);
			fp = NULL;
			ISP_LOGE("read_gain from:%s, camera_id[%d]: %d, %d, %d, %d\n", file_name, cxt_tmp->camera_id, cxt->r, cxt->g, cxt->b, cxt->ct);
		}
	} else {
		snprintf(file_name, 1024, "%scamera_%d_awb.file", AWB_GAIN_PARAM_FILE_NAME_MEDIA, cxt_tmp->camera_id);
		fp = fopen(file_name, "rb");
		if (fp) {
			memset((void *)cxt, 0, sizeof(struct awb_save_gain));
			count = fread((char *)cxt, 1, sizeof(struct awb_save_gain), fp);
			if(count < (sizeof(struct awb_save_gain)))
				ISP_LOGE("_awb_read_gain:fread count error!");
			fclose(fp);
			fp = NULL;
			ISP_LOGE("read_gain from:%s, camera_id[%d]: %d, %d, %d, %d\n", file_name, cxt_tmp->camera_id, cxt->r, cxt->g, cxt->b, cxt->ct);
		}
	}
}
static cmr_u32 _awb_read_file_for_init(struct awb_ctrl_cxt *cxt)
{
	cmr_u32 rtn = AWB_CTRL_SUCCESS;

	if(cxt->wb_mode != AWB_CTRL_WB_MODE_AUTO){
		_awb_set_gain_manualwb_v3(cxt);
		cxt->cur_gain.r = cxt->output_gain.r;
		cxt->cur_gain.g = cxt->output_gain.g;
		cxt->cur_gain.b = cxt->output_gain.b;
		cxt->cur_ct = cxt->output_ct;
		return rtn;
	}

	_awb_read_gain(&(cxt->s_save_awb_param), cxt);
	if (0 != cxt->s_save_awb_param.r && 0 != cxt->s_save_awb_param.g && 0 != cxt->s_save_awb_param.b) {
		cxt->output_gain.r = cxt->s_save_awb_param.r;
		cxt->output_gain.g = cxt->s_save_awb_param.g;
		cxt->output_gain.b = cxt->s_save_awb_param.b;
		cxt->output_ct = cxt->s_save_awb_param.ct;
		cxt->awb_result.gain.r = cxt->s_save_awb_param.r;
		cxt->awb_result.gain.g = cxt->s_save_awb_param.g;
		cxt->awb_result.gain.b = cxt->s_save_awb_param.b;
		cxt->awb_result.ct = cxt->s_save_awb_param.ct;
		cxt->cur_gain.r = cxt->s_save_awb_param.r;
		cxt->cur_gain.g = cxt->s_save_awb_param.g;
		cxt->cur_gain.b = cxt->s_save_awb_param.b;
		cxt->cur_ct = cxt->s_save_awb_param.ct;
	}
	return rtn;
}

static cmr_u32 _awb_set_wbmode_v3(struct awb_ctrl_cxt *cxt, void *in_param)
{
	cmr_u32 rtn = AWB_CTRL_SUCCESS;
	cmr_u32 awb_mode = *(cmr_u32 *) in_param;
	struct awb_gain awb_gain = { 0x0, 0x0, 0x0 };
	cmr_u32 orig_wbmode = cxt->wb_mode;
	cxt->wb_mode = awb_mode;
	rtn = _awb_get_gain(cxt, (void *)&awb_gain);
	ISP_LOGV("set wbmode:%d , original mode:%d" , awb_mode , orig_wbmode);
	if(awb_mode != AWB_CTRL_WB_MODE_AUTO)
	{
		if((orig_wbmode == AWB_CTRL_WB_MODE_AUTO) && (0 == cxt->flash_info.flash_enable))
		{
			ISP_LOGV("_awb_set_wbmode save recover gain:%d,%d,%d" , awb_gain.r,
				awb_gain.g,awb_gain.b);
			cxt->recover_gain.r = awb_gain.r;
			cxt->recover_gain.g = awb_gain.g;
			cxt->recover_gain.b = awb_gain.b;
			cxt->recover_ct = cxt->cur_ct;
			cxt->recover_mode = orig_wbmode;
		}
		ISP_LOGV("set wb mode:%d gain , orig mode:%d" , awb_mode , orig_wbmode);
		//recover gain from gain table;
		if(awb_mode != orig_wbmode)
			_awb_set_gain_manualwb_v3(cxt);
	} else {
		cxt->recover_gain.r = cxt->cur_gain.r;
		cxt->recover_gain.g = cxt->cur_gain.g;
		cxt->recover_gain.b = cxt->cur_gain.b;
		cxt->recover_ct = cxt->cur_ct;
		ISP_LOGV("wbmode changed to auto!");
	}

	return rtn;
}

static cmr_u32 _awb_get_wbmode(struct awb_ctrl_cxt *cxt, void *out_param)
{
	cmr_u32 rtn = AWB_CTRL_SUCCESS;

	*(cmr_u32 *) out_param = cxt->wb_mode;
	ISP_LOGV("AWB get mode = %d", *(cmr_u32 *) out_param);
	return rtn;
}

static cmr_u32 _awb_set_workmode(struct awb_ctrl_cxt *cxt, void *in_param)
{
	cmr_u32 rtn = AWB_CTRL_SUCCESS;
	cmr_u32 work_mode = *(cmr_u32 *) in_param;

	cxt->work_mode = work_mode;
	return rtn;
}

static cmr_u32 _awb_set_recgain(struct awb_ctrl_cxt *cxt, void *param)
{
	UNUSED(param);
	cmr_u32 rtn = AWB_CTRL_SUCCESS;
	struct awb_gain awb_gain = { 0x0, 0x0, 0x0 };

	rtn = _awb_get_gain(cxt, (void *)&awb_gain);

	cxt->recover_gain.r = awb_gain.r;
	cxt->recover_gain.g = awb_gain.g;
	cxt->recover_gain.b = awb_gain.b;

	cxt->recover_mode = cxt->wb_mode;
	cxt->recover_ct = cxt->cur_ct;
	ISP_LOGV("pre flashing mode = %d", cxt->flash_info.flash_mode);

	ISP_LOGV("FLASH_TAG: awb flash recover gain = (%d, %d, %d), recover mode = %d", cxt->recover_gain.r, cxt->recover_gain.g, cxt->recover_gain.b, cxt->recover_mode);
	return rtn;

}

static cmr_u32 _awb_get_gain(struct awb_ctrl_cxt *cxt, void *param)
{
	cmr_u32 rtn = AWB_CTRL_SUCCESS;
	struct awb_gain *awb_result = (struct awb_gain *)param;

	awb_result->r = cxt->output_gain.r;
	awb_result->g = cxt->output_gain.g;
	awb_result->b = cxt->output_gain.b;

	ISP_LOGV("_awb_get_gain = (%d,%d,%d)", awb_result->r, awb_result->g, awb_result->b);

	return rtn;

}

static cmr_u32 _awb_get_gain_and_offset(struct awb_ctrl_cxt *cxt, void *param)
{
	cmr_u32 rtn = AWB_CTRL_SUCCESS;

	if (param) {
		struct awb_gain_and_offset *awb_result = (struct awb_gain_and_offset *)param;

		awb_result->gain.r = cxt->output_gain.r;
		awb_result->gain.g = cxt->output_gain.g;
		awb_result->gain.b = cxt->output_gain.b;
		awb_result->offset.r_offset = cxt->cur_offset.r_offset;
		awb_result->offset.g_offset = cxt->cur_offset.g_offset;
		awb_result->offset.b_offset = cxt->cur_offset.b_offset;

		ISP_LOGV("gain_r,g,b = (%d,%d,%d)", awb_result->gain.r, awb_result->gain.g, awb_result->gain.b);
		ISP_LOGV("offset_r,g,b = (%d,%d,%d)", awb_result->offset.r_offset, awb_result->offset.g_offset, awb_result->offset.b_offset);
	}

	return rtn;

}

static cmr_u32 _awb_get_result_info(struct awb_ctrl_cxt *cxt, void *param)
{
	cmr_u32 rtn = AWB_CTRL_SUCCESS;
	struct awb_ctrl_calc_result *awb_result = (struct awb_ctrl_calc_result *)param;

	memcpy(awb_result, &cxt->awb_result, sizeof(struct awb_ctrl_calc_result));

	return rtn;

}


static cmr_u32 _awb_set_scene_info_v3(struct awb_ctrl_cxt *cxt, void *param)
{
	cmr_u32 rtn = AWB_CTRL_SUCCESS;
	struct awb_aiscene_info_old *ai_scene_info = param;
	if (param) {
		//get the ai_scene
		cxt->ai_scene_info_v3.cur_scene_id = ai_scene_info->cur_scene_id;
		memcpy(cxt->ai_scene_info_v3.task0,ai_scene_info->task0,sizeof(struct awb_ai_task0_result_3_0)*AI_SCENE_TASK0_MAX);
		memcpy(cxt->ai_scene_info_v3.task1,ai_scene_info->task1,sizeof(struct awb_ai_task1_result_3_0)*AI_SCENE_TASK1_MAX);
		memcpy(cxt->ai_scene_info_v3.task2,ai_scene_info->task2,sizeof(struct awb_ai_task2_result_3_0)*AI_SCENE_TASK2_MAX);
		ISP_LOGV("done.");
	}

	return rtn;
}

static cmr_u32 _awb_get_cur_gain(struct awb_ctrl_cxt *cxt, void *param)
{
	cmr_u32 rtn = AWB_CTRL_SUCCESS;
	struct awb_gain *awb_result = (struct awb_gain *)param;

	awb_result->r = cxt->cur_gain.r;
	awb_result->g = cxt->cur_gain.g;
	awb_result->b = cxt->cur_gain.b;

	ISP_LOGV("_awb_get_cur_gain = (%d,%d,%d)", awb_result->r, awb_result->g, awb_result->b);

	return rtn;

}

static cmr_u32 _awb_get_pix_cnt(struct awb_ctrl_cxt *cxt, void *param)
{
	cmr_u32 rtn = AWB_CTRL_SUCCESS;
	struct isp_size *in_ptr = (struct isp_size *)param;

	cxt->init_param.stat_win_size.w = in_ptr->w / 2 / cxt->init_param.stat_img_size.w * 2;
	cxt->init_param.stat_win_size.h = in_ptr->h / 2 / cxt->init_param.stat_img_size.h * 2;
	g_src_size[cxt->camera_id].w = in_ptr->w;
	g_src_size[cxt->camera_id].h = in_ptr->h;

	return rtn;
}

static cmr_u32 _awb_get_stat_size(struct awb_ctrl_cxt *cxt, void *param)
{
	cmr_u32 rtn = AWB_CTRL_SUCCESS;
	struct awb_size *stat_size = (struct awb_size *)param;

	stat_size->w = cxt->init_param.stat_img_size.w;
	stat_size->h = cxt->init_param.stat_img_size.h;

	ISP_LOGV("_awb_get_stat_size = (%d,%d)", stat_size->w, stat_size->h);

	return rtn;
}


static cmr_u32 _awb_get_ct_v3(struct awb_ctrl_cxt *cxt, void *param)
{
	cmr_u32 rtn = AWB_CTRL_SUCCESS;
	cmr_u32 *ct = (cmr_u32 *) param;

	*ct = cxt->output_ct_mean;

	ISP_LOGV("_awb_get_ct = %d", cxt->output_ct_mean);

	return rtn;
}

static cmr_u32 _awb_get_recgain(struct awb_ctrl_cxt *cxt, void *param)
{
	UNUSED(param);
	cmr_u32 rtn = AWB_CTRL_SUCCESS;
	struct awb_ctrl_gain awb_gain = { 0x0, 0x0, 0x0 };

	awb_gain.r = cxt->recover_gain.r;
	awb_gain.g = cxt->recover_gain.g;
	awb_gain.b = cxt->recover_gain.b;

	//awb_result.gain output_gain cur_gain, the 3 data is updated to PM
	cxt->awb_result.gain.r = cxt->recover_gain.r;
	cxt->awb_result.gain.g = cxt->recover_gain.g;
	cxt->awb_result.gain.b = cxt->recover_gain.b;
	cxt->awb_result.ct     = cxt->recover_ct;

	cxt->output_gain.r = cxt->recover_gain.r;
	cxt->output_gain.g = cxt->recover_gain.g;
	cxt->output_gain.b = cxt->recover_gain.b;
	cxt->output_ct = cxt->recover_ct;

	cxt->cur_gain.r = awb_gain.r;
	cxt->cur_gain.g = awb_gain.g;
	cxt->cur_gain.b = awb_gain.b;
	cxt->cur_ct = cxt->recover_ct;
	cxt->wb_mode = cxt->recover_mode;

	ISP_LOGV("after flashing mode = %d", cxt->flash_info.flash_mode);

	ISP_LOGV("FLASH_TAG: awb flash end  gain = (%d, %d, %d), recover mode = %d", cxt->cur_gain.r, cxt->cur_gain.g, cxt->cur_gain.b, cxt->wb_mode);

	return rtn;

}

static cmr_u32 awb_get_flash_awb_enable(struct awb_ctrl_cxt *cxt, void *out)
{
	cmr_u32 rtn = AWB_CTRL_SUCCESS;
	cmr_u32 *flash_awb_en = (cmr_u32 *)out;

	*flash_awb_en = cxt->flash_awb_en;
	ISP_LOGV("AWB Flash_awb_en %d",cxt->flash_awb_en);
	return rtn;
}


static cmr_u32 _awb_set_lock(struct awb_ctrl_cxt *cxt, void *param)
#if 1
{
	UNUSED(param);
	cmr_u32 rtn = AWB_CTRL_SUCCESS;

	cxt->lock_info.lock_num += 1;

	ISP_LOGV("AWB_TEST _awb_set_lock0: luck=%d, mode=%d", cxt->lock_info.lock_num, cxt->lock_info.lock_mode);

	if (0 != cxt->lock_info.lock_num) {

		cxt->lock_info.lock_mode = AWB_CTRL_LOCKMODE;

		cxt->lock_info.lock_gain.r = cxt->output_gain.r;
		cxt->lock_info.lock_gain.g = cxt->output_gain.g;
		cxt->lock_info.lock_gain.b = cxt->output_gain.b;

		cxt->lock_info.lock_ct = cxt->output_ct;
	}
	ISP_LOGV("AWB_TEST _awb_set_lock1: luck=%d, mode:%d   rgb[%d %d %d]", cxt->lock_info.lock_num, cxt->lock_info.lock_mode, cxt->output_gain.r, cxt->output_gain.g, cxt->output_gain.b);
	return rtn;

}

#else
{
	cmr_u32 rtn = AWB_CTRL_SUCCESS;

	cxt->lock_info.lock_num += 1;

	ISP_LOGV("AWB_TEST _awb_set_lock0: luck=%d, unluck=%", cxt->lock_info.lock_num, cxt->lock_info.unlock_num);

	if (0 == cxt->lock_info.lock_gain.r && 0 == cxt->lock_info.lock_gain.g && 0 == cxt->lock_info.lock_gain.b) {

		cxt->lock_info.lock_gain.r = cxt->output_gain.r;
		cxt->lock_info.lock_gain.g = cxt->output_gain.g;
		cxt->lock_info.lock_gain.b = cxt->output_gain.b;

		cxt->lock_info.lock_ct = cxt->output_ct;
	}
	ISP_LOGV("AWB_TEST _awb_set_lock1: luck=%d, unluck=%, mode:%d", cxt->lock_info.lock_num, cxt->lock_info.unlock_num, cxt->lock_info.lock_mode);
	return rtn;

}
#endif

static cmr_u32 _awb_get_unlock(struct awb_ctrl_cxt *cxt, void *param)
#if 1
{
	UNUSED(param);
	cmr_u32 rtn = AWB_CTRL_SUCCESS;
	ISP_LOGV("AWB_TEST _awb_get_unlock0: lock_num=%d, mode:=%d", cxt->lock_info.lock_num, cxt->lock_info.lock_mode);
	if (0 != cxt->lock_info.lock_num) {
		cxt->lock_info.lock_num -= 1;
	}

	if (0 == cxt->lock_info.lock_num) {
		cxt->lock_info.lock_mode = AWB_CTRL_UNLOCKMODE;
	}

	ISP_LOGV("AWB_TEST _awb_get_unlock1: lock_num=%d, mode:=%d", cxt->lock_info.lock_num, cxt->lock_info.lock_mode);

	return rtn;
}
#else
{
	cmr_u32 rtn = AWB_CTRL_SUCCESS;

	cxt->lock_info.unlock_num += 1;
	if (0 == cxt->lock_info.lock_num)
		rtn = AWB_CTRL_ERROR;

	ISP_LOGV("AWB_TEST _awb_get_unlock0: luck=%d, unluck=%d", cxt->lock_info.lock_num, cxt->lock_info.unlock_num);

	if (0 != cxt->lock_info.lock_num && cxt->lock_info.lock_num != cxt->lock_info.unlock_num)
		cxt->lock_info.lock_mode = AWB_CTRL_LOCKMODE;

	ISP_LOGV("AWB_TEST _awb_get_unlock1: luck=%d, unluck=%, mode:=%d", cxt->lock_info.lock_num, cxt->lock_info.unlock_num, cxt->lock_info.lock_mode);

	return rtn;

}
#endif

static cmr_u32 _awb_flash_snopshot_recovery(struct awb_ctrl_cxt *cxt, void *param)
{
	UNUSED(param);
	cmr_u32 rtn = AWB_CTRL_SUCCESS;

	cxt->flash_info.main_flash_enable = 1;
	cxt->lock_info.lock_flash_frame = 2;

	if (0 != cxt->lock_info.lock_gain.r && 0 != cxt->lock_info.lock_gain.g && 0 != cxt->lock_info.lock_gain.b) {
		cxt->recover_gain.r = cxt->lock_info.lock_gain.r;
		cxt->recover_gain.g = cxt->lock_info.lock_gain.g;
		cxt->recover_gain.b = cxt->lock_info.lock_gain.b;
		cxt->recover_ct = cxt->lock_info.lock_ct;
	}

	ISP_LOGV("Reset recgain for frames after flashing: (%d,%d,%d) %dK", cxt->recover_gain.r, cxt->recover_gain.g, cxt->recover_gain.b, cxt->recover_ct);

	return rtn;
}

static cmr_u32 _awb_set_flash_status(struct awb_ctrl_cxt *cxt, void *param)
{
	UNUSED(param);
	cmr_u32 rtn = AWB_CTRL_SUCCESS;
	enum awb_ctrl_flash_status *flash_status = (enum awb_ctrl_flash_status *)param;

	cxt->flash_info.flash_status = *flash_status;

	//auto mode and flash status ,not update
	if ((cxt->flash_info.flash_status == 4 || cxt->flash_info.flash_status == 3) && (AWB_CTRL_WB_MODE_AUTO == cxt->wb_mode)) {
		cxt->flash_update_awb = 0;
	} else {
		cxt->flash_update_awb = 1;
	}

	if (cxt->flash_info.flash_status == 2) {
		cxt->flash_pre_state = 2;
	} else {
		cxt->flash_pre_state = 0;
	}

	ISP_LOGV("flashing status = %d", cxt->flash_info.flash_status);

	return rtn;

}

static cmr_u32 _awb_set_tuning_param(struct awb_ctrl_cxt *cxt, void *param0)
{
	cmr_u32 rtn = AWB_CTRL_SUCCESS;
	UNUSED(cxt);
	rtn = _check_init_param(param0);
	if (AWB_CTRL_SUCCESS != rtn) {
		ISP_LOGE("fail to check init param");
		return AWB_CTRL_ERROR;
	}

	return rtn;
}


static cmr_u32 _awb_get_data_type_v3(struct awb_ctrl_cxt *cxt, void *param)
{
	cmr_u32 rtn = AWB_CTRL_SUCCESS;

	if (NULL == cxt) {
		ISP_LOGE("fail to get awb cxt");
		rtn = AWB_CTRL_ERROR;
		goto exit;
	}

	if (NULL == param) {
		ISP_LOGE("param is null");
		rtn = AWB_CTRL_ERROR;
		goto exit;
	}
	//for the turnning param is void pointer,so *data_type is cannot get
	cmr_s32 *data_type = (cmr_s32 *)param;
	*data_type = 1; //0 - binning4awb, 1 - aem

	ISP_LOGV("awb_get_data_type = %d", *data_type);
  exit:
	return rtn;
}

static cmr_s32 awb_set_fd_param_v3(struct awb_ctrl_cxt *cxt, cmr_handle param)
{
	if (param) {
		struct awb_face_info_3_0 *fd = (struct awb_face_info_3_0 *)param;
		unsigned int i = 0;
		if (fd->face_num > 0) {
			cxt->awb_face_info_v3.img_width = fd->img_width;
			cxt->awb_face_info_v3.img_height = fd->img_height;
			cxt->awb_face_info_v3.face_num = fd->face_num;
			ISP_LOGV("FD_CTRL_NUM:%d.\n", cxt->awb_face_info_v3.face_num);
			for (i = 0; i < fd->face_num; i++) {
				cxt->awb_face_info_v3.face[i].start_x = fd->face[i].start_x;
				cxt->awb_face_info_v3.face[i].start_y = fd->face[i].start_y;
				cxt->awb_face_info_v3.face[i].end_x   = fd->face[i].end_x;
				cxt->awb_face_info_v3.face[i].end_y   = fd->face[i].end_y;
				cxt->awb_face_info_v3.face[i].pose    = fd->face[i].pose;
				cxt->awb_face_info_v3.face[i].score   = fd->face[i].score;
			}
		} else {
			cxt->awb_face_info_v3.face_num = 0;
			for (i = 0; i < fd->face_num; i++) {
				cxt->awb_face_info_v3.face[i].start_x 	= 0;
				cxt->awb_face_info_v3.face[i].start_y 	= 0;
				cxt->awb_face_info_v3.face[i].end_x 	= 0;
				cxt->awb_face_info_v3.face[i].end_y 	= 0;
				cxt->awb_face_info_v3.face[i].pose 		= 0;
				cxt->awb_face_info_v3.face[i].score 	= 0;
			}
		}
	} else {
		ISP_LOGE("AWB fail to fd, param %p", param);
		return AWB_ERROR;
	}

	return AWB_SUCCESS;
}

static cmr_u32 awbsprd_unload_lib(struct awb_ctrl_cxt *cxt)
{
	cmr_u32 rtn = AWB_CTRL_SUCCESS;

	if (NULL == cxt) {
		ISP_LOGE("fail to load awb lib param");
		rtn = AWB_CTRL_ERROR;
		goto exit;
	}

	if (cxt->lib_handle) {
#if 0
		dlclose(cxt->lib_handle);
#else
		put_lib_handle(cxt->lib_handle);
#endif
		cxt->lib_handle = NULL;
	}

  exit:
	return rtn;
}

static cmr_u32 awbsprd_load_lib(struct awb_ctrl_cxt *cxt)
{
	cmr_u32 rtn = AWB_CTRL_SUCCESS;
	cmr_u32 v_count = 0;
	cmr_u32 v_id = 0;

	if (NULL == cxt) {
		ISP_LOGE("fail to load awb lib param");
		rtn = AWB_CTRL_ERROR;
		goto exit;
	}
	//version_id = 0 according the isp3.x awb_lib
	//version_id = 1 according the isp2.x awb_lib
	v_id = cxt->lib_info->version_id;
	v_count = sizeof(libawb_path) / sizeof(libawb_path[0]);
	if (v_id >= v_count) {
		ISP_LOGE("fail to get awb lib version id,  version_id: %d", v_id);
		rtn = AWB_CTRL_ERROR;
		goto exit;
	}
	ISP_LOGI("awb lib v_count:%d, version_id:%d, version_name:%s", v_count, v_id, libawb_path[v_id]);
#if 0
	cxt->lib_handle = dlopen(libawb_path[v_id], RTLD_NOW);
#else
	cxt->lib_handle = get_lib_handle(libawb_path[v_id]);
#endif
	if (!cxt->lib_handle) {
		ISP_LOGE("fail to dlopen awb lib");
		rtn = AWB_CTRL_ERROR;
		goto exit;
	}
	ISP_LOGI("dlopen awb lib successful");
	//awblib 3.x
	if(v_id == 1) {
		ISP_LOGV("[AWB] version is isp3.x awblib.so");
		cxt->lib_ops.awb_init_v3 = dlsym(cxt->lib_handle, "awb_init");
		if (!cxt->lib_ops.awb_init_v3) {
			ISP_LOGE("fail to dlsym awb_init");
			rtn = AWB_CTRL_ERROR;
			goto load_error;
		}

		cxt->lib_ops.awb_calc_v3 = dlsym(cxt->lib_handle, "awb_calc");
		if (!cxt->lib_ops.awb_calc_v3) {
			ISP_LOGE("fail to dlsym awb_calculation");
			rtn = AWB_CTRL_ERROR;
			goto load_error;
		}

		cxt->lib_ops.awb_deinit_v3 = dlsym(cxt->lib_handle, "awb_deinit");
		if (!cxt->lib_ops.awb_deinit_v3) {
			ISP_LOGE("fail to dlsym awb_deinit");
			rtn = AWB_CTRL_ERROR;
			goto load_error;
		}

		cxt->lib_ops.awb_ioctrl_v3 = dlsym(cxt->lib_handle, "awb_ioctrl");
		if (!cxt->lib_ops.awb_ioctrl_v3) {
			ISP_LOGE("fail to dlsym awb_ioctrl");
			rtn = AWB_CTRL_ERROR;
			goto load_error;
		}
	} else {
		ISP_LOGE("[AWB] ERROR! the version is not isp3.0 awblib.so");
	}
	return AWB_CTRL_SUCCESS;
  load_error:
	awbsprd_unload_lib(cxt);
  exit:
	return rtn;
}

static cmr_u32 awb_get_debug_info(struct awb_ctrl_cxt *cxt, void *result)
{
	cmr_u32 rtn = AWB_SUCCESS;
	struct debug_awb_param *param = (struct debug_awb_param *)result;

	param->version = AWB_DEBUG_VERSION_ID;
	param->r_gain = cxt->output_gain.r;
	param->g_gain = cxt->output_gain.g;
	param->b_gain = cxt->output_gain.b;
	param->cur_ct = cxt->output_ct;
	param->cur_awb_mode = cxt->wb_mode;
	param->cur_work_mode = cxt->work_mode;
	/* awb calibration of golden sensor */
	param->golden_r = cxt->otp_info.gldn_stat_info.r;
	param->golden_g = cxt->otp_info.gldn_stat_info.g;
	param->golden_b = cxt->otp_info.gldn_stat_info.b;
	/* awb calibration of random sensor */
	param->random_r = cxt->otp_info.rdm_stat_info.r;
	param->random_g = cxt->otp_info.rdm_stat_info.g;
	param->random_b = cxt->otp_info.rdm_stat_info.b;
	return rtn;
}


static cmr_u32 _awb_get_flash_ct_table_v3(struct awb_ctrl_cxt *cxt, void *result)
{
	cmr_u32 rtn = AWB_SUCCESS;
	struct awb_ct_table_3_0 *param = (struct awb_ct_table_3_0 *)result;
	//awblib 3.x
	rtn = cxt->lib_ops.awb_ioctrl_v3(cxt->alg_handle, AWB_IOCTRL_GET_CTTABLE20_3_0, param,param);

	return rtn;

}


static cmr_u32 _awb_get_otp_info(struct awb_ctrl_cxt *cxt, void *result)
{
	cmr_u32 rtn = AWB_SUCCESS;
	struct awb_ctrl_opt_info *param = (struct awb_ctrl_opt_info*)result;
	ISP_LOGI("HJW: gldn-(%d, %d, %d), rdm-(%d, %d, %d)\n",
			cxt->otp_info.gldn_stat_info.r, cxt->otp_info.gldn_stat_info.g, cxt->otp_info.gldn_stat_info.b,
			cxt->otp_info.rdm_stat_info.r, cxt->otp_info.rdm_stat_info.g, cxt->otp_info.rdm_stat_info.b);
	*param = cxt->otp_info;

	return rtn;
}

static cmr_u32 awb_get_debug_info_for_display(struct awb_ctrl_cxt *cxt, void *result)
{
	cmr_u32 rtn = AWB_SUCCESS;
	struct debug_awb_display_param *emParam = (struct debug_awb_display_param *)result;
	emParam->version = AWB_DEBUG_VERSION_ID;
	emParam->r_gain = cxt->output_gain.r;
	emParam->g_gain = cxt->output_gain.g;
	emParam->b_gain = cxt->output_gain.b;
	emParam->cur_ct = cxt->output_ct;
	emParam->cur_awb_mode = cxt->wb_mode;
	emParam->cur_work_mode = cxt->work_mode;
	/* awb calibration of golden sensor */
	emParam->golden_r = cxt->otp_info.gldn_stat_info.r;
	emParam->golden_g = cxt->otp_info.gldn_stat_info.g;
	emParam->golden_b = cxt->otp_info.gldn_stat_info.b;
	/* awb calibration of random sensor */
	emParam->random_r = cxt->otp_info.rdm_stat_info.r;
	emParam->random_g = cxt->otp_info.rdm_stat_info.g;
	emParam->random_b = cxt->otp_info.rdm_stat_info.b;
	return rtn;
}

static cmr_u32 _awb_parser_otp_info(struct awb_ctrl_init_param * param)
{
	cmr_u32 rtn = AWB_CTRL_SUCCESS;
	cmr_u8 *awb_rdm_otp_data = NULL;
	cmr_u16 awb_rdm_otp_len = 0;
	cmr_u16 *awb_golden_otp_data = NULL;
	cmr_u16 awb_golden_otp_len = 0;
	cmr_u32 otp_map_version = 0;
	cmr_u8 *module_info = NULL;
	cmr_u8 *awb_golden_otp_data_v1 = NULL;
	struct sensor_otp_section_info *awb_otp_info_ptr = NULL;
	struct sensor_otp_section_info *module_info_ptr = NULL;

	if (NULL != param->otp_info_ptr) {
		if (param->otp_info_ptr->otp_vendor == OTP_VENDOR_SINGLE) {
			awb_otp_info_ptr = param->otp_info_ptr->single_otp.iso_awb_info;
			module_info_ptr = param->otp_info_ptr->single_otp.module_info;
			ISP_LOGV("pass awb otp, single cam");
		} else if (param->otp_info_ptr->otp_vendor == OTP_VENDOR_SINGLE_CAM_DUAL || param->otp_info_ptr->otp_vendor == OTP_VENDOR_DUAL_CAM_DUAL) {
			if (param->is_master == 1) {
				awb_otp_info_ptr = param->otp_info_ptr->dual_otp.master_iso_awb_info;
				module_info_ptr = param->otp_info_ptr->dual_otp.master_module_info;
				ISP_LOGV("pass awb otp, dual cam master");
			} else {
				awb_otp_info_ptr = param->otp_info_ptr->dual_otp.slave_iso_awb_info;
				module_info_ptr = param->otp_info_ptr->dual_otp.slave_module_info;
				ISP_LOGV("pass awb otp, dual cam slave");
			}
		} else {
			awb_otp_info_ptr = NULL;
			module_info_ptr = NULL;
			ISP_LOGE("awb otp otp_vendor = %d", param->otp_info_ptr->otp_vendor);
		}
	} else {
		awb_otp_info_ptr = NULL;
		module_info_ptr = NULL;
		ISP_LOGE("awb otp info ptr is NULL");
	}

	if (NULL != awb_otp_info_ptr && NULL != module_info_ptr) {
		module_info = (cmr_u8 *) module_info_ptr->rdm_info.data_addr;

		if (NULL != module_info) {
			if ((module_info[4] == 0 && module_info[5] == 1)
				|| (module_info[4] == 0 && module_info[5] == 2)
				|| (module_info[4] == 0 && module_info[5] == 3)
				|| (module_info[4] == 0 && module_info[5] == 4)
				|| (module_info[4] == 0 && module_info[5] == 5)
				|| (module_info[4] == 1 && module_info[5] == 0 && (module_info[0] != 0x53 || module_info[1] != 0x50 || module_info[2] != 0x52 || module_info[3] != 0x44))
				|| (module_info[4] == 2 && module_info[5] == 0)
				|| (module_info[4] == 3 && module_info[5] == 0)
				|| (module_info[4] == 4 && module_info[5] == 0)
				|| (module_info[4] == 5 && module_info[5] == 0)) {
				ISP_LOGV("awb otp map v0.4 or v0.5");
				otp_map_version = 4;
				awb_rdm_otp_data = (cmr_u8 *) awb_otp_info_ptr->rdm_info.data_addr;
				awb_rdm_otp_len = awb_otp_info_ptr->rdm_info.data_size;
				awb_golden_otp_data = (cmr_u16 *) awb_otp_info_ptr->gld_info.data_addr;
				awb_golden_otp_len = awb_otp_info_ptr->gld_info.data_size;
			} else if (module_info[4] == 1 && (module_info[5] == 0 || module_info[5] == 1) && module_info[0] == 0x53 && module_info[1] == 0x50 && module_info[2] == 0x52 && module_info[3] == 0x44) {
				ISP_LOGV("awb otp map v1.0 or v1.1");
				otp_map_version = 1;
				awb_rdm_otp_data = (cmr_u8 *) awb_otp_info_ptr->rdm_info.data_addr + 1;
				awb_rdm_otp_len = awb_otp_info_ptr->rdm_info.data_size;
				awb_golden_otp_data_v1 = (cmr_u8 *) awb_otp_info_ptr->rdm_info.data_addr + 1 + 6;
				awb_golden_otp_len = awb_otp_info_ptr->rdm_info.data_size;
			} else {
				ISP_LOGE("awb otp map version error");
				awb_rdm_otp_data = NULL;
				awb_golden_otp_data = NULL;
				awb_golden_otp_data_v1 = NULL;
			}
		} else {
			ISP_LOGE("awb module_info is NULL");
			awb_rdm_otp_data = NULL;
			awb_golden_otp_data = NULL;
			awb_golden_otp_data_v1 = NULL;
		}

		if (NULL != awb_rdm_otp_data && 0 != awb_rdm_otp_len) {
			param->otp_info.rdm_stat_info.r = (awb_rdm_otp_data[1] << 8) | awb_rdm_otp_data[0];
			param->otp_info.rdm_stat_info.g = (awb_rdm_otp_data[3] << 8) | awb_rdm_otp_data[2];
			param->otp_info.rdm_stat_info.b = (awb_rdm_otp_data[5] << 8) | awb_rdm_otp_data[4];
			ISP_LOGV("awb otp random [%d %d %d ]", param->otp_info.rdm_stat_info.r, param->otp_info.rdm_stat_info.g, param->otp_info.rdm_stat_info.b);
		} else {
			param->otp_info.rdm_stat_info.r = 0;
			param->otp_info.rdm_stat_info.g = 0;
			param->otp_info.rdm_stat_info.b = 0;
			ISP_LOGE("awb_rdm_otp_data = %p, awb_rdm_otp_len = %d. Parser fail", awb_rdm_otp_data, awb_rdm_otp_len);
		}

		if (otp_map_version == 4 && NULL != awb_golden_otp_data && 0 != awb_golden_otp_len) {
			param->otp_info.gldn_stat_info.r = awb_golden_otp_data[0];
			param->otp_info.gldn_stat_info.g = awb_golden_otp_data[1];
			param->otp_info.gldn_stat_info.b = awb_golden_otp_data[2];
			ISP_LOGV("awb otp golden [%d %d %d]", param->otp_info.gldn_stat_info.r, param->otp_info.gldn_stat_info.g, param->otp_info.gldn_stat_info.b);
		} else if (otp_map_version == 1 && NULL != awb_golden_otp_data_v1 && 0 != awb_golden_otp_len) {
			param->otp_info.gldn_stat_info.r = (awb_golden_otp_data_v1[1] << 8) | awb_golden_otp_data_v1[0];
			param->otp_info.gldn_stat_info.g = (awb_golden_otp_data_v1[3] << 8) | awb_golden_otp_data_v1[2];
			param->otp_info.gldn_stat_info.b = (awb_golden_otp_data_v1[5] << 8) | awb_golden_otp_data_v1[4];
			ISP_LOGV("awb otp golden [%d %d %d]", param->otp_info.gldn_stat_info.r, param->otp_info.gldn_stat_info.g, param->otp_info.gldn_stat_info.b);
		} else {
			param->otp_info.gldn_stat_info.r = 0;
			param->otp_info.gldn_stat_info.g = 0;
			param->otp_info.gldn_stat_info.b = 0;
			ISP_LOGE("awb_golden_otp_data = %p, awb_golden_otp_len = %d. Parser fail", awb_golden_otp_data, awb_golden_otp_len);
		}
	} else {
		ISP_LOGE("awb awb_otp_info_ptr = %p, module_info_ptr = %p. Parser fail !", awb_otp_info_ptr, module_info_ptr);
		param->otp_info.rdm_stat_info.r = 0;
		param->otp_info.rdm_stat_info.g = 0;
		param->otp_info.rdm_stat_info.b = 0;
		param->otp_info.gldn_stat_info.r = 0;
		param->otp_info.gldn_stat_info.g = 0;
		param->otp_info.gldn_stat_info.b = 0;
	}

	return rtn;
}

/*------------------------------------------------------------------------------*
*					public functions			*
*-------------------------------------------------------------------------------*/
/* awb_sprd_ctrl_init--
*@ handle: instance
*@ param: input param
*@ result: output result
*@ return:
*@           AWB_CTRL_INVALID_HANDLE: failed
*@	     others: awb ctrl handle
*/

awb_ctrl_handle_t awb_sprd_ctrl_init_v3(void *in, void *out)
{
	ISP_LOGI("[awb_3x] start awb_sprd_ctrl_init");
	cmr_u32 rtn = AWB_CTRL_SUCCESS;
	struct awb_ctrl_init_param *param = (struct awb_ctrl_init_param *)in;
	struct awb_ctrl_init_result *result = (struct awb_ctrl_init_result *)out;
	struct awb_ctrl_cxt *cxt = NULL;
	struct sensor_otp_awb_info otp_info;

	if (NULL == param || NULL == result) {
		ISP_LOGE("fail to init awb, invalid param: param=%p, result=%p", param, result);
		goto ERROR_EXIT;
	}

	cxt = (struct awb_ctrl_cxt *)malloc(sizeof(struct awb_ctrl_cxt));
	if (NULL == cxt) {
		ISP_LOGE("fail to malloc awb ctrl cxt");
		goto ERROR_EXIT;
	}
	memset(cxt, 0, sizeof(struct awb_ctrl_cxt));
	cxt->lib_info = &param->lib_param;

	rtn = awbsprd_load_lib(cxt);
	if (AWB_CTRL_SUCCESS != rtn) {
		ISP_LOGE("fail to load awb lib");
		goto ERROR_EXIT;
	}
	else {
		ISP_LOGE("load awb lib success!");
	}

	cxt->snap_lock = 0;			// recovery snapshot awb continus frames
	cxt->flash_info.flash_enable = 0;
	cxt->lock_info.lock_flash_frame = 0;
	cxt->flash_info.main_flash_enable = 0;
	cxt->magic_begin = AWB_CTRL_MAGIC_BEGIN;
	cxt->magic_end = AWB_CTRL_MAGIC_END;
	cxt->flash_update_awb = 1;
	cxt->flash_pre_state = 0;
	cxt->color_support = param->color_support;
	cxt->sensor_role = param->is_master;
	cxt->sensor_role_type = param->sensor_role;
	cxt->is_multi_mode = param->is_multi_mode;
	cxt->is_mono_sensor = param->is_mono_sensor;
	cxt->ptr_isp_br_ioctrl = param->ptr_isp_br_ioctrl;
	cxt->flash_awb_en = 0;
	ISP_LOGI("is_multi_mode=%d , color_support=%d\n", param->is_multi_mode , cxt->color_support);
#if 0
	if(cxt->sensor_role == 1)
	{
		cxt->sensor_role_type = CAM_SENSOR_MASTER;
		#if 0
		if(NULL != cxt->ptr_isp_br_ioctrl)
			cxt->ptr_isp_br_ioctrl(CAM_SENSOR_MASTER , SET_FOV_DATA , &param->fov_info , NULL);
		#endif
	}
	else {
		cxt->sensor_role_type = CAM_SENSOR_SLAVE0;
		#if 0
		if(NULL != cxt->ptr_isp_br_ioctrl)
			cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE0 , SET_FOV_DATA , &param->fov_info , NULL);
		#endif
	}
#endif
	// paser awb otp info
	ISP_LOGI("start parse the awb_otp_info");
	_awb_parser_otp_info(param);
	ISP_LOGI("end parse the awb_otp_info");
	pthread_mutex_init(&cxt->status_lock, NULL);
	ISP_LOGI("create the lock thread for awb success!");

	if((param->stat_img_size_ae.w != param->stat_img_size_ae.h) || (param->stat_img_size_ae.w < 32) || (param->stat_img_size_ae.w % 32)){	
		param->stat_img_size_ae.w = 32;
		param->stat_img_size_ae.h = 32;
	}
	g_src_size[param->camera_id].w = param->src_size.w;
	g_src_size[param->camera_id].h = param->src_size.h;
	g_stat_img_size[param->camera_id].w = param->stat_img_size_ae.w;
	g_stat_img_size[param->camera_id].h = param->stat_img_size_ae.h;

	cxt->stat_img_size = param->stat_img_size;
	cxt->init_param.stat_img_size.w = param->stat_img_size_ae.w;
	cxt->init_param.stat_img_size.h = param->stat_img_size_ae.h;
	cxt->awb_init_param_v3.camera_id = param->camera_id;
	cxt->awb_init_param_v3.otp_unit_r = param->otp_info.rdm_stat_info.r;
	cxt->awb_init_param_v3.otp_unit_g = param->otp_info.rdm_stat_info.g;
	cxt->awb_init_param_v3.otp_unit_b = param->otp_info.rdm_stat_info.b;
	//get the turnning param
	cxt->awb_init_param_v3.tool_param = param->tuning_param;

	cxt->otp_info.gldn_stat_info.r = param->otp_info.gldn_stat_info.r;
	cxt->otp_info.gldn_stat_info.g = param->otp_info.gldn_stat_info.g;
	cxt->otp_info.gldn_stat_info.b = param->otp_info.gldn_stat_info.b;
	cxt->otp_info.rdm_stat_info.r  = param->otp_info.rdm_stat_info.r;
	cxt->otp_info.rdm_stat_info.g  = param->otp_info.rdm_stat_info.g;
	cxt->otp_info.rdm_stat_info.b  = param->otp_info.rdm_stat_info.b;

	struct awb_rgb_gain_3_0 awb_gain_v3;
	memset(&awb_gain_v3,0,sizeof(struct awb_rgb_gain_3_0));
	ISP_LOGV("start the awb_init() in the awblib");
	if(cxt->lib_ops.awb_init_v3 != NULL){
	cxt->alg_handle = cxt->lib_ops.awb_init_v3(&cxt->awb_init_param_v3, &awb_gain_v3);
	}
	else{
		ISP_LOGE("fail to awb_init");
		goto ERROR_EXIT;
    }

	result->gain.r = awb_gain_v3.r_gain;
	result->gain.g = awb_gain_v3.g_gain;
	result->gain.b = awb_gain_v3.b_gain;
	result->ct     = awb_gain_v3.ct;
	result->tint   = awb_gain_v3.tint;
	result->ct_mean= awb_gain_v3.ct_mean;
	ISP_LOGV("the awb_gain_r by awb_lib_init is = %d",awb_gain_v3.r_gain);
	ISP_LOGV("the awb_gain_g by awb_lib_init is = %d",awb_gain_v3.g_gain);
	ISP_LOGV("the awb_gain_b by awb_lib_init is = %d",awb_gain_v3.b_gain);


	cxt->cur_gain.r = result->gain.r;
	cxt->cur_gain.g = result->gain.g;
	cxt->cur_gain.b = result->gain.b;
	cxt->cur_ct 	= result->ct;
	cxt->cur_tint   = result->ct_mean;
	cxt->camera_id  = param->camera_id;

	cxt->output_gain.r = result->gain.r;
	cxt->output_gain.g = result->gain.g;
	cxt->output_gain.b = result->gain.b;
	cxt->output_ct_mean= result->ct_mean;
	cxt->output_ct 	   = result->ct;

	otp_info.otp_golden_r = cxt->otp_info.gldn_stat_info.r;
	otp_info.otp_golden_g = cxt->otp_info.gldn_stat_info.g;
	otp_info.otp_golden_b = cxt->otp_info.gldn_stat_info.b;
	otp_info.otp_random_r = cxt->otp_info.rdm_stat_info.r;
	otp_info.otp_random_g = cxt->otp_info.rdm_stat_info.g;
	otp_info.otp_random_b = cxt->otp_info.rdm_stat_info.b;

#ifndef CONFIG_ISP_2_2
	ISP_LOGI("start the isp_br_ioctrl()");
	if (cxt->ptr_isp_br_ioctrl != NULL) {
		rtn = cxt->ptr_isp_br_ioctrl(cxt->sensor_role_type, SET_OTP_AWB, &otp_info, NULL);
	}
	ISP_LOGI("end the isp_br_ioctrl()");
#endif
	_awb_read_file_for_init(cxt);

	//init recover_gain & awb result gain
	cxt->recover_gain.r = cxt->output_gain.r;
	cxt->recover_gain.g = cxt->output_gain.g;
	cxt->recover_gain.b = cxt->output_gain.b;
	cxt->gain_to_save_auto.r = cxt->output_gain.r;
	cxt->gain_to_save_auto.g = cxt->output_gain.g;
	cxt->gain_to_save_auto.b = cxt->output_gain.b;
	cxt->recover_ct = cxt->output_ct;
	cxt->recover_mode = cxt->wb_mode;
	cxt->awb_result.gain.r = cxt->recover_gain.r;
	cxt->awb_result.gain.b = cxt->recover_gain.b;
	cxt->awb_result.gain.g = cxt->recover_gain.g;
	cxt->awb_result.ct = cxt->recover_ct;
	ISP_LOGD("AWB init: (%d,%d,%d), AWB_OTP rr = %d, rg = %d, rb = %d", cxt->output_gain.r, cxt->output_gain.g, cxt->output_gain.b,
			 cxt->awb_init_param_v3.otp_unit_r, cxt->awb_init_param_v3.otp_unit_g, cxt->awb_init_param_v3.otp_unit_b);
	pthread_mutex_init(&cxt->status_lock, NULL);
	ISP_LOGI("done");
	return (awb_ctrl_handle_t) cxt;

  ERROR_EXIT:
	awbsprd_unload_lib(cxt);
	AWB_CTRL_SAFE_FREE(cxt);
	ISP_LOGE("fail to init awb %d", rtn);
	return AWB_CTRL_INVALID_HANDLE;
}


/* awb_sprd_ctrl_deinit--
*@ handle: instance
*@ param: input param
*@ result: output result
*@ return:
*@           0: successful
*@	     others: failed
*/


cmr_s32 awb_sprd_ctrl_deinit_v3(void *handle, void *in, void *out)
{
	UNUSED(in);
	UNUSED(out);
	cmr_s32 rtn = AWB_CTRL_SUCCESS;
	struct awb_ctrl_cxt *cxt = (struct awb_ctrl_cxt *)handle;

	rtn = _check_handle(handle);

	if (AWB_CTRL_SUCCESS != rtn) {
		ISP_LOGE("fail to deinit awb");
		rtn = AWB_CTRL_ERROR;
		goto EXIT;
	}

	//save recover gain
	ISP_LOGV("awb_sprd_ctrl_deinit before _awb_save_gain_tofile");
	_awb_save_gain_tofile(cxt);

	pthread_mutex_destroy(&cxt->status_lock);

	rtn = cxt->lib_ops.awb_deinit_v3(cxt->alg_handle);
	if (AWB_CTRL_SUCCESS != rtn) {
		ISP_LOGE("fail to deinit awb_v3 \n");
	}

	rtn = awbsprd_unload_lib(cxt);
	if (AWB_CTRL_SUCCESS != rtn) {
		ISP_LOGE("fail to unload awb lib");
		goto EXIT;
	}

	memset(cxt, 0, sizeof(*cxt));
	AWB_CTRL_SAFE_FREE(cxt);

	ISP_LOGI("done");
	return rtn;
  EXIT:
	memset(cxt, 0, sizeof(*cxt));
	AWB_CTRL_SAFE_FREE(cxt);
	ISP_LOGE("fail to deinit awb");
	return rtn;
}


#if 0
static void calculate_fov(void* input , float* fov)
{
	struct drv_fov_info *fovinfo = (struct drv_fov_info*)input;
	*fov =  (2* atan(fovinfo->physical_size[0]/(2* fovinfo->focal_lengths))) * 180 / PI;
	ISP_LOGV("fov info physical_size0 is:%f , focal_lengths:%f" , fovinfo->physical_size[0] , fovinfo->focal_lengths);
	return;
}
#endif
/* awb_sprd_ctrl_calculation--
*@ handle: instance
*@ param: input param
*@ result: output result
*@ return:
*@           0: successful
*@	     others: failed
*/

cmr_s32 awb_sprd_ctrl_calculation_v3(void *handle, void *in, void *out)
{
	cmr_s32 rtn = AWB_CTRL_SUCCESS;
	struct awb_ctrl_cxt *cxt = (struct awb_ctrl_cxt *)handle;
	struct awb_ctrl_calc_param param;
	struct awb_ctrl_calc_result result;
	struct awb_colorsensor_info_3_0 xyz_color_info;
	UNUSED(out);

	if (NULL == in) {
		ISP_LOGE("fail to calc awb, invalid param: param=%p", in);
		return AWB_CTRL_ERROR;
	}

	memcpy(&param, in, sizeof(struct awb_ctrl_calc_param));
	memset(&result, 0, sizeof(struct awb_ctrl_calc_result));

	rtn = _check_handle(handle);
	if (AWB_CTRL_SUCCESS != rtn) {
		ISP_LOGE("fail to check handle");
		return AWB_CTRL_ERROR;
	}
	else {
		ISP_LOGE("check handle success");
	}

	struct awb_calc_param_3_0  calc_param_v3;
	struct awb_calc_result_3_0 calc_result_v3;
	memset(&calc_param_v3, 0x00, sizeof(calc_param_v3));
	memset(&calc_result_v3, 0x00, sizeof(calc_result_v3));
	/*
	if (((awb_tool_param*)(cxt->awb_init_param.tool_param))->stat_type) {
		calc_param.stat_img.r_stat = param.stat_img.chn_img.r;
		calc_param.stat_img.g_stat = param.stat_img.chn_img.g;
		calc_param.stat_img.b_stat = param.stat_img.chn_img.b;
		calc_param.stat_img.b_pixel_cnt = (cxt->init_param.stat_win_size.w * cxt->init_param.stat_win_size.h) / 4;
		calc_param.stat_img.g_pixel_cnt = (cxt->init_param.stat_win_size.w * cxt->init_param.stat_win_size.h) / 4;
		calc_param.stat_img.r_pixel_cnt = (cxt->init_param.stat_win_size.w * cxt->init_param.stat_win_size.h) / 4;
		calc_param.stat_img.width_stat = cxt->init_param.stat_img_size.w;
		calc_param.stat_img.height_stat = cxt->init_param.stat_img_size.h;
	} else {
		calc_param.stat_img.r_stat = param.stat_img_awb.chn_img.r;
		calc_param.stat_img.g_stat = param.stat_img_awb.chn_img.g;
		calc_param.stat_img.b_stat = param.stat_img_awb.chn_img.b;
		calc_param.stat_img.width_stat= param.stat_width_awb;
		calc_param.stat_img.height_stat = param.stat_height_awb;
		calc_param.stat_img.r_pixel_cnt = 1;
		calc_param.stat_img.g_pixel_cnt = 1;
		calc_param.stat_img.b_pixel_cnt = 1;
	}*/
	calc_param_v3.frame_index = cxt->frame_count;
	calc_param_v3.stat_img_3_0.r_stat = param.stat_img.chn_img.r;
	calc_param_v3.stat_img_3_0.g_stat = param.stat_img.chn_img.g;
	calc_param_v3.stat_img_3_0.b_stat = param.stat_img.chn_img.b;
	calc_param_v3.stat_img_3_0.b_pixel_cnt = (cxt->init_param.stat_win_size.w * cxt->init_param.stat_win_size.h) / 4;
	calc_param_v3.stat_img_3_0.g_pixel_cnt = (cxt->init_param.stat_win_size.w * cxt->init_param.stat_win_size.h) / 4;
	calc_param_v3.stat_img_3_0.r_pixel_cnt = (cxt->init_param.stat_win_size.w * cxt->init_param.stat_win_size.h) / 4;
	calc_param_v3.stat_img_3_0.width_stat = cxt->init_param.stat_img_size.w;
	calc_param_v3.stat_img_3_0.height_stat = cxt->init_param.stat_img_size.h;
	calc_param_v3.bv = param.bv;
	calc_param_v3.iso = param.ae_info.iso;
	//print calc_param info
	ISP_LOGV("the calc_param.stat_img:r_pixel_cnt = %d, g_pixel_cnt = %d, b_pixel_cnt = %d, width_stat = %d, height_stat = %d, bv = %d, iso = %d",\
			calc_param_v3.stat_img_3_0.r_pixel_cnt, calc_param_v3.stat_img_3_0.g_pixel_cnt, calc_param_v3.stat_img_3_0.b_pixel_cnt,\
			calc_param_v3.stat_img_3_0.width_stat, calc_param_v3.stat_img_3_0.height_stat, calc_param_v3.bv, calc_param_v3.iso);
	//color sensor info
	calc_param_v3.colorsensor_info = NULL;
	if(cxt->color_support == 1)
	{
		xyz_color_info.x_data = param.xyz_info.x_data;
		xyz_color_info.y_data = param.xyz_info.y_data;
		xyz_color_info.z_data = param.xyz_info.z_data;
		xyz_color_info.ir_data = param.xyz_info.ir_data;
		xyz_color_info.x_raw = param.xyz_info.x_raw;
		xyz_color_info.y_raw = param.xyz_info.y_raw;
		xyz_color_info.z_raw = param.xyz_info.z_raw;
		xyz_color_info.ir_raw = param.xyz_info.ir_raw;
		xyz_color_info.atime = 0;
		xyz_color_info.again = 0;
		xyz_color_info.lux = param.xyz_info.lux_data;
		xyz_color_info.cct = param.xyz_info.cct_data;
		calc_param_v3.colorsensor_info = &xyz_color_info;
		ISP_LOGV("color support , x:%d,y:%d,z:%d,ir:%d, xraw:%d,yraw:%d,zraw:%d,irraw:%d",
				xyz_color_info.x_data,xyz_color_info.y_data,xyz_color_info.z_data,xyz_color_info.ir_data,
				xyz_color_info.x_raw,xyz_color_info.y_raw,xyz_color_info.z_raw,xyz_color_info.ir_raw);
	}

	//face_info
	calc_param_v3.face_info = &cxt->awb_face_info_v3;
	ISP_LOGV("FACE num:%d,sx:%d,sy:%d,ex:%d,ey:%d,score:%d.\n",cxt->awb_face_info_v3.face_num,cxt->awb_face_info_v3.face[0].start_x,cxt->awb_face_info_v3.face[0].start_y,\
			cxt->awb_face_info_v3.face[0].end_x,cxt->awb_face_info_v3.face[0].end_y,cxt->awb_face_info_v3.face[0].score);

	//AI_info
	calc_param_v3.aiscene_info = &cxt->ai_scene_info_v3;
	memcpy(calc_param_v3.matrix, param.matrix, 9 * sizeof(cmr_s32));
	memcpy(calc_param_v3.gamma, param.gamma, 256);

	ATRACE_BEGIN(__FUNCTION__);
	ISP_LOGV("[awb_3x] awb_calc() in awblib is running");
	cmr_u64 time0 = systemTime(CLOCK_MONOTONIC);
	rtn = cxt->lib_ops.awb_calc_v3(cxt->alg_handle, &calc_param_v3, &calc_result_v3);
	cmr_u64 time1 = systemTime(CLOCK_MONOTONIC);
	ATRACE_END();
	ISP_LOGV("AWB %dx%d: (%d,%d,%d) %dK, %dus", calc_param_v3.stat_img_3_0.width_stat, calc_param_v3.stat_img_3_0.height_stat, calc_result_v3.awb_gain.r_gain, calc_result_v3.awb_gain.g_gain,
			 calc_result_v3.awb_gain.b_gain, calc_result_v3.awb_gain.ct, (cmr_s32) ((time1 - time0) / 1000));

	if (_awb_get_cmd_property() == 1){
		ISP_LOGI("[AWB_TEST] calc frame_count: %d, awb_camera_id: %d --(0: back camera, 1: front camera), awb_work_mode: %d --(0: preview, 1:capture, 2:video),\
			awb_mode: %d --(0:auto,1:sunny,2:cloudy,3:fluorescent,4:incandescent,5:user0,6:user1,7:off), awb_lock_status: %d --(1: lock, 0: unlock), lib_calc time :%dus,\
			calc_param.stat_img_size: (%dx,%d), calc_result.Gain: (%d,%d,%d) calc_result.CT: %dK",cxt->frame_count, cxt->camera_id, cxt->work_mode, cxt->wb_mode, cxt->lock_info.lock_mode,\
			(cmr_s32)((time1 - time0) / 1000), calc_param_v3.stat_img_3_0.width_stat, calc_param_v3.stat_img_3_0.height_stat, calc_result_v3.awb_gain.r_gain, calc_result_v3.awb_gain.g_gain, calc_result_v3.awb_gain.b_gain, calc_result_v3.awb_gain.ct );
	}
	result.gain.r = calc_result_v3.awb_gain.r_gain;
	result.gain.g = calc_result_v3.awb_gain.g_gain;
	result.gain.b = calc_result_v3.awb_gain.b_gain;
	result.ct     = calc_result_v3.awb_gain.ct;


	result.log_awb.log = calc_result_v3.log_buffer;
	result.log_awb.size = calc_result_v3.log_size;
	result.update_gain = cxt->flash_update_awb;

	cxt->cur_gain.r = result.gain.r;
	cxt->cur_gain.g = result.gain.g;
	cxt->cur_gain.b = result.gain.b;
	cxt->cur_ct = result.ct;

	cxt->output_gain.r  = cxt->cur_gain.r;
	cxt->output_gain.g  = cxt->cur_gain.g;
	cxt->output_gain.b  = cxt->cur_gain.b;

	cxt->output_ct_mean = calc_result_v3.awb_gain.ct_mean;
	cxt->output_ct		= cxt->cur_ct;

	cxt->log = calc_result_v3.log_buffer;
	cxt->size = calc_result_v3.log_size;

	//lock awb after snapshot
	if (cxt->snap_lock != 0) {
		cxt->output_gain.r = cxt->recover_gain.r;
		cxt->output_gain.g = cxt->recover_gain.g;
		cxt->output_gain.b = cxt->recover_gain.b;
		cxt->output_ct = cxt->recover_ct;
		cxt->snap_lock -= 1;
	}
	//scenemode & mwb change
	if (AWB_CTRL_SCENEMODE_AUTO == cxt->scene_mode) {
		cmr_u32 mawb_id = cxt->wb_mode;
		struct awb_rgb_gain_3_0 out_gain_mwb;
		if (AWB_CTRL_WB_MODE_AUTO != cxt->wb_mode) {
			//set gain offset to zero
			cxt->cur_offset.r_offset = 0;
			cxt->cur_offset.g_offset = 0;
			cxt->cur_offset.b_offset = 0;
			if (mawb_id == AWB_CTRL_AWB_MODE_OFF) {
				cxt->output_gain.r = 1024;
				cxt->output_gain.g = 1024;
				cxt->output_gain.b = 1024;
				cxt->output_ct = 5000;
				cxt->output_ct_mean = 5000;
			} else if ((mawb_id > 0) && (mawb_id < 10))	{
				// return mwb by mwb mode id
				//by awb_ioctrl to get the gain and ct
				rtn = cxt->lib_ops.awb_ioctrl_v3(cxt->alg_handle, AWB_IOCTRL_GET_MWB_BY_MODEID_3_0, &mawb_id, &out_gain_mwb);
				cxt->output_gain.r = out_gain_mwb.r_gain;
				cxt->output_gain.g = out_gain_mwb.g_gain;
				cxt->output_gain.b = out_gain_mwb.b_gain;
				cxt->output_ct	   = out_gain_mwb.ct;
				cxt->output_ct_mean = out_gain_mwb.ct;
				ISP_LOGV("get_rgb_gain by mwb mode id in awb_ioctrl, r_gain = %d, g_gain= %d, b_gain = %d ct = %d", cxt->output_gain.r, cxt->output_gain.g, cxt->output_gain.b, cxt->output_ct);
			} else {
			// return mwb by ct, (100K <= ct < 10000K)
			//by awb_ioctrl to get the gain and ct
			rtn = cxt->lib_ops.awb_ioctrl_v3(cxt->alg_handle, AWB_IOCTRL_GET_MWB_BY_CT_3_0, &mawb_id, &out_gain_mwb);
			cxt->output_gain.r = out_gain_mwb.r_gain;
			cxt->output_gain.g = out_gain_mwb.g_gain;
			cxt->output_gain.b = out_gain_mwb.b_gain;
			cxt->output_ct     = out_gain_mwb.ct;
			cxt->output_ct_mean = out_gain_mwb.ct;
			ISP_LOGV("get_rgb_gain by mwb ct in awb_ioctrl, r_gain = %d, g_gain= %d, b_gain = %d ct = %d", cxt->output_gain.r, cxt->output_gain.g, cxt->output_gain.b, cxt->output_ct);
			}
			if ((cxt->otp_info.gldn_stat_info.r != 0) && (cxt->otp_info.gldn_stat_info.g != 0) && (cxt->otp_info.gldn_stat_info.b != 0)) {
				double otp_g_coef = (double)cxt->otp_info.rdm_stat_info.g / cxt->otp_info.gldn_stat_info.g;
				double otp_r_coef = (double)cxt->otp_info.rdm_stat_info.r / cxt->otp_info.gldn_stat_info.r;
				double otp_b_coef = (double)cxt->otp_info.rdm_stat_info.b / cxt->otp_info.gldn_stat_info.b;

				if (otp_g_coef != 0) {
					otp_r_coef = otp_r_coef / otp_g_coef;
					otp_b_coef = otp_b_coef / otp_g_coef;
					cxt->output_gain.r = (cmr_u32)(cxt->output_gain.r * otp_r_coef);
					cxt->output_gain.b = (cmr_u32)(cxt->output_gain.b * otp_b_coef);
				}
			}
		}
	}

	//lock mode
	if (AWB_CTRL_LOCKMODE == cxt->lock_info.lock_mode && cxt->wb_mode == 0) {
		cxt->output_gain.r = cxt->lock_info.lock_gain.r;
		cxt->output_gain.g = cxt->lock_info.lock_gain.g;
		cxt->output_gain.b = cxt->lock_info.lock_gain.b;
		cxt->output_ct     = cxt->lock_info.lock_ct;
		cxt->cur_gain.r = cxt->lock_info.lock_gain.r;
		cxt->cur_gain.g = cxt->lock_info.lock_gain.g;
		cxt->cur_gain.b = cxt->lock_info.lock_gain.b;
		cxt->cur_ct = cxt->lock_info.lock_ct;
	}
	//only pre flash after
	if (cxt->flash_pre_state != 0 && cxt->wb_mode == 0) {
		cxt->output_gain.r = cxt->recover_gain.r;
		cxt->output_gain.g = cxt->recover_gain.g;
		cxt->output_gain.b = cxt->recover_gain.b;
		cxt->output_ct = cxt->recover_ct;
		cxt->flash_pre_state -= 1;
	}
	//lock awb after flash
	if (cxt->flash_info.main_flash_enable == 1 && cxt->lock_info.lock_flash_frame != 0 && cxt->wb_mode == 0) {
		cxt->output_gain.r = cxt->recover_gain.r;
		cxt->output_gain.g = cxt->recover_gain.g;
		cxt->output_gain.b = cxt->recover_gain.b;
		cxt->output_ct = cxt->recover_ct;
		cxt->lock_info.lock_flash_frame -= 1;
	} else {
		cxt->flash_info.main_flash_enable = 0;
	}

//  ISP_LOGD("cxt->snap_lock =%d lock_mode =%d main_flash_enable =%d  lock_flash_frame =%d ",cxt->snap_lock,cxt->lock_info.lock_mode,cxt->flash_info.main_flash_enable,cxt->lock_info.lock_flash_frame);
	ISP_LOGV("AWB result : (%d,%d,%d) %dK , fram_count : %d , camera_id : %d", cxt->output_gain.r, cxt->output_gain.g, cxt->output_gain.b, cxt->output_ct, cxt->frame_count, cxt->camera_id);
	if (_awb_get_cmd_property() == 1) {
		ISP_LOGD("AWB result : (%d,%d,%d) %dK , fram_count : %d , camera_id : %d", cxt->output_gain.r, cxt->output_gain.g, cxt->output_gain.b, cxt->output_ct, cxt->frame_count, cxt->camera_id);
	}

	//set the gain/ct to_save_file
	cxt->frame_count 	= cxt->frame_count + 1;
	//set the gain/ct to_save_file
	//for auto mode
	if(cxt->wb_mode == AWB_CTRL_WB_MODE_AUTO)
	{
		cxt->gain_to_save_auto.r = cxt->output_gain.r;
		cxt->gain_to_save_auto.g = cxt->output_gain.g;
		cxt->gain_to_save_auto.b = cxt->output_gain.b;
		cxt->ct_auto_to_save	 = cxt->output_ct;
	}else{
		cxt->gain_to_save_auto.r = result.gain.r;
		cxt->gain_to_save_auto.g = result.gain.g;
		cxt->gain_to_save_auto.b = result.gain.b;
		cxt->ct_auto_to_save	 = result.ct;
	}
	//set the gain/ct to update sensor
	result.gain.r = cxt->output_gain.r;
	result.gain.g = cxt->output_gain.g;
	result.gain.b = cxt->output_gain.b;
	result.offset.r_offset = cxt->cur_offset.r_offset;
	result.offset.g_offset = cxt->cur_offset.g_offset;
	result.offset.b_offset = cxt->cur_offset.b_offset;
	result.ct = cxt->output_ct;
	/*
	if ((cxt->is_multi_mode == ISP_ALG_DUAL_SBS) && (cxt->ptr_isp_br_ioctrl != NULL)) {
		cxt->ptr_isp_br_ioctrl(cxt->sensor_role_type, SET_GAIN_AWB_DATA, &result.gain, NULL);
	}

	if ((cxt->is_multi_mode == ISP_ALG_DUAL_C_C) && (cxt->ptr_isp_br_ioctrl != NULL)) {
		cxt->ptr_isp_br_ioctrl(cxt->sensor_role_type , SET_GAIN_AWB_DATA, &result.gain, NULL);
		cxt->ptr_isp_br_ioctrl(cxt->sensor_role_type , SET_MATCH_AWB_DATA, &result.ct , NULL);
	}

	if ((cxt->is_multi_mode == ISP_ALG_TRIBLE_W_T_UW) && (cxt->ptr_isp_br_ioctrl != NULL)) {
		cxt->ptr_isp_br_ioctrl(cxt->sensor_role_type , SET_GAIN_AWB_DATA, &result.gain, NULL);
		cxt->ptr_isp_br_ioctrl(cxt->sensor_role_type , SET_MATCH_AWB_DATA, &result.ct , NULL);
	}
	*/
	//set gain by isp_bridge
	if (cxt->ptr_isp_br_ioctrl != NULL) {
	cxt->ptr_isp_br_ioctrl(cxt->sensor_role_type , SET_GAIN_AWB_DATA, &result.gain, NULL);
	cxt->ptr_isp_br_ioctrl(cxt->sensor_role_type , SET_MATCH_AWB_DATA, &result.ct , NULL);
	}

	pthread_mutex_lock(&cxt->status_lock);

	memcpy(&cxt->awb_result, &result, sizeof(struct awb_ctrl_calc_result));

	pthread_mutex_unlock(&cxt->status_lock);

	return rtn;
}



/* awb_sprd_ctrl_ioctrl--
*@ handle: instance
*@ param: input param
*@ result: output result
*@ return:
*@           0: successful
*@	     others: failed
*/


cmr_s32 awb_sprd_ctrl_ioctrl_v3(void *handle, cmr_s32 cmd, void *in, void *out)
{
	cmr_s32 rtn = AWB_CTRL_SUCCESS;
	struct awb_ctrl_cxt *cxt = (struct awb_ctrl_cxt *)handle;

	rtn = _check_handle(handle);
	if (AWB_CTRL_SUCCESS != rtn) {
		ISP_LOGE("fail to check handle");
		return AWB_CTRL_ERROR;
	}

	pthread_mutex_lock(&cxt->status_lock);

	switch (cmd) {
	case AWB_CTRL_CMD_SET_WB_MODE:
		rtn = _awb_set_wbmode_v3(cxt, in);
		break;

	case AWB_CTRL_CMD_GET_WB_MODE:
		rtn = _awb_get_wbmode(cxt, out);
		break;

	case AWB_CTRL_CMD_SET_WORK_MODE:
	ISP_LOGV("AWB_CTRL_CMD_SET_WORK_MODE lock_mode = %d  cxt->flash_info.flash_enable =%d",cxt->lock_info.lock_mode,cxt->flash_info.flash_enable);
		rtn = _awb_set_workmode(cxt, in);
		if(cxt->wb_mode == AWB_CTRL_WB_MODE_AUTO)
		{
			_awb_get_recgain(cxt , in);
			ISP_LOGV("AWB_CTRL_CMD_SET_WORK_MODE get recgain");
			if (AWB_CTRL_SUCCESS != rtn) {
				ISP_LOGE("fail to _awb_get_recgain");
				return AWB_CTRL_ERROR;
			}
		}
		else {
			//recover gain from tabel
			ISP_LOGV("AWB_CTRL_CMD_SET_WORK_MODE before _awb_set_gain_manualwb_v3");
			_awb_set_gain_manualwb_v3(cxt);
		}
		break;

	case AWB_CTRL_CMD_SET_AE_STAT_WIN_NUM:
		cxt->init_param.stat_img_size.w = ((struct isp_size *)in)->w;
		cxt->init_param.stat_img_size.h = ((struct isp_size *)in)->h;

		g_stat_img_size[cxt->camera_id].w = cxt->init_param.stat_img_size.w;
		g_stat_img_size[cxt->camera_id].h = cxt->init_param.stat_img_size.h;
		break;

	case AWB_CTRL_CMD_GET_GAIN:
		rtn = _awb_get_gain(cxt, in);
		break;

	case AWB_CTRL_CMD_GET_GAIN_AND_OFFSET:
		rtn = _awb_get_gain_and_offset(cxt, in);
		break;

	case AWB_CTRL_CMD_GET_CUR_GAIN:
		rtn = _awb_get_cur_gain(cxt, in);
		break;

	case AWB_CTRL_CMD_RESULT_INFO:
		rtn = _awb_get_result_info(cxt, in);
		break;

	case AWB_CTRL_CMD_SET_SCENE_INFO:
		rtn = _awb_set_scene_info_v3(cxt, in);
		break;

	case AWB_CTRL_CMD_FLASH_OPEN_M:
		ISP_LOGV("FLASH_TAG: AWB_CTRL_CMD_FLASH_OPEN_M");
		cxt->flash_info.flash_mode = AWB_CTRL_FLASH_MAIN;
		break;

	case AWB_CTRL_CMD_FLASH_OPEN_P:
		ISP_LOGV("FLASH_TAG: AWB_CTRL_CMD_FLASH_OPEN_P");
		cxt->flash_info.flash_mode = AWB_CTRL_FLASH_PRE;
		break;

	case AWB_CTRL_CMD_FLASH_CLOSE:
		ISP_LOGV("FLASH_TAG: AWB_CTRL_CMD_FLASH_CLOSE");
		if (cxt->wb_mode == 0) {
			if ((AWB_CTRL_FLASH_PRE == cxt->flash_info.flash_mode) || (AWB_CTRL_FLASH_MAIN == cxt->flash_info.flash_mode)) {
				rtn = _awb_get_recgain(cxt, in);
			}
		}
		cxt->flash_info.flash_mode = AWB_CTRL_FLASH_END;
		cxt->flash_info.flash_enable = AWB_CTRL_FLASH_END;
		cxt->awb_result.update_gain = cxt->flash_update_awb;

		break;

	case AWB_CTRL_CMD_FLASH_BEFORE_P:
		ISP_LOGV("FLASH_TAG: AWB_CTRL_CMD_FLASH_BEFORE_P");
		cxt->flash_info.flash_enable = 1;
		if (cxt->wb_mode == 0) {
			rtn = _awb_set_recgain(cxt, in);
		}
		break;

	case AWB_CTRL_CMD_LOCK:
		rtn = _awb_set_lock(cxt, in);
		break;

	case AWB_CTRL_CMD_UNLOCK:
		rtn = _awb_get_unlock(cxt, in);
		break;

	case AWB_CTRL_CMD_FLASH_SNOP:
		rtn = _awb_flash_snopshot_recovery(cxt, in);
		break;

	case AWB_CTRL_CMD_GET_STAT_SIZE:
		rtn = _awb_get_stat_size(cxt, in);
		break;

	case AWB_CTRL_CMD_GET_PIX_CNT:
		rtn = _awb_get_pix_cnt(cxt, in);
		break;

	case AWB_CTRL_CMD_GET_CT:
		rtn = _awb_get_ct_v3(cxt, in);
		break;

	case AWB_CTRL_CMD_SET_FLASH_STATUS:
		rtn = _awb_set_flash_status(cxt, in);
		break;

	case AWB_CTRL_CMD_GET_FLASH_AWB_ENABLE:
		rtn = awb_get_flash_awb_enable(cxt, out);
		break;

	case AWB_CTRL_CMD_SET_MAINFLASH_EN:
	case AWB_CTRL_CMD_SET_FLASH_RATIO:
		break;

	case AWB_CTRL_CMD_SET_UPDATE_TUNING_PARAM:
		rtn = _awb_set_tuning_param(cxt, in);
		break;

	case AWB_CTRL_CMD_GET_DEBUG_INFO:
		if (out) {
			rtn = awb_get_debug_info(cxt, out);
		}
		break;

	case AWB_CTRL_CMD_EM_GET_PARAM:
		rtn = awb_get_debug_info_for_display(cxt, out);
		break;

	case AWB_CTRL_CMD_VIDEO_STOP_NOTIFY:
	ISP_LOGV("AWB_CTRL_CMD_VIDEO_STOP_NOTIFY  cxt->lock_info.lock_mode =%d  flash_mode =%d workmode:%d , wbmode:%d",cxt->lock_info.lock_mode,cxt->flash_info.flash_enable , cxt->work_mode , cxt->wb_mode);
		//flash may not be opened. but if opened , not save gain
		if((cxt->work_mode != AWB_MODE_CAPTURE) && (cxt->flash_info.flash_enable == 0) && (cxt->wb_mode == AWB_CTRL_WB_MODE_AUTO))
		{
			rtn = _awb_set_recgain(cxt, in);
		}
		ISP_LOGV("_awb_save_gain_tofile");
		_awb_save_gain_tofile(cxt);
		break;

	case AWB_CTRL_CMD_GET_CT_TABLE20:
		rtn = _awb_get_flash_ct_table_v3(cxt, out);
		break;

	case AWB_CTRL_CMD_GET_OTP_INFO:
		rtn = _awb_get_otp_info(cxt, out);
		break;

	case AWB_CTRL_CMD_GET_DATA_TYPE:
		rtn = _awb_get_data_type_v3(cxt, out);
		break;
	case AWB_CTRL_CMD_SET_FACE_DETECT:
		rtn = awb_set_fd_param_v3(cxt, in);
		break;
	case AWB_CTRL_CMD_READ_FILE:
		rtn = _awb_read_file_for_init(cxt);
		break;
	default:
		ISP_LOGE("fail to get invalid cmd %d", cmd);
		rtn = AWB_CTRL_ERROR;
		break;
	}

	pthread_mutex_unlock(&cxt->status_lock);
	return rtn;

}


struct adpt_ops_type awb_sprd_adpt_ops_ver3 = {
	.adpt_init = awb_sprd_ctrl_init_v3,
	.adpt_deinit = awb_sprd_ctrl_deinit_v3,
	.adpt_process = awb_sprd_ctrl_calculation_v3,
	.adpt_ioctrl= awb_sprd_ctrl_ioctrl_v3,
};

