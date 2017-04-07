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

#include <assert.h>
#include <cutils/properties.h>

#include "af_ctrl.h"
#include "af_sprd_adpt_v1.h"
#include "isp_adpt.h"
#include "dlfcn.h"

/*------------------------------------------------------------------------------*
*					Compiler Flag				*
*-------------------------------------------------------------------------------*/
#ifndef UNUSED
#define     UNUSED(param)  (void)(param)
#endif

/**---------------------------------------------------------------------------*
**				Micro Define					*
**----------------------------------------------------------------------------*/
#define FOCUS_STAT_DATA_NUM	2

static cmr_u32 iir_level, nr_mode, cw_mode, fv0_e, fv1_e;
static struct af_iir_nr_info af_iir_nr[3] = {
	{			//weak
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
	{			//medium
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
	{			//strong
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

static char fv1_coeff[36] = {
	-2, -2, -2, -2, 16, -2, -2, -2, -2,
	-3, 5, 3, 5, 0, -5, 3, -5, -3,
	3, 5, -3, -5, 0, 5, -3, -5, 3,
	0, -8, 0, -8, 16, 0, 0, 0, 0
};

char libafv1_path[][20] = {
	"libspafv1.so",
	"libaf_v1.so",
	"libaf_v2.so",
	"libaf_v3.so",
	"libaf_v4.so",
	"libaf_v5.so",
};

static cmr_s32 _check_handle(afv1_handle_t handle)
{
	cmr_s32 rtn = AFV1_SUCCESS;
//              struct af_context_t *cxt = (struct af_context_t *)handle;
	af_ctrl_t *af = (af_ctrl_t *) handle;

	if (NULL == af) {
		ISP_LOGE("fail to get valid cxt pointer");
		return AFV1_HANDLER_NULL;
	}
/*
	if (AF_MAGIC_START != cxt->magic_start || AF_MAGIC_END != cxt->magic_end) {
		ISP_LOGE("fail to get valid magic, begin = 0x%x, magic end = 0x%x", cxt->magic_start, cxt->magic_end);
		return AF_HANDLER_CXT_ERROR;
	}
*/
	return rtn;
}

#if 0
static cmr_s32 _caf_reset(afv1_handle_t handle);
cmr_int af_ioctrl(void *handle, cmr_int cmd, void *param0, void *param1);

static cmr_s32 _af_set_motor_pos(afv1_handle_t handle, cmr_u32 motot_pos)
{
	cmr_s32 rtn = AFV1_SUCCESS;
	struct af_context_t *af_cxt = (struct af_context_t *)handle;
	struct af_motor_pos set_pos;

	rtn = _check_handle(handle);
	if (AFV1_SUCCESS != rtn) {
		ISP_LOGE("fail to check cxt");
		return AFV1_ERROR;
	}

	set_pos.motor_pos = motot_pos;
	set_pos.skip_frame = 0;
	set_pos.wait_time = 0;
	af_cxt->go_position(af_cxt->caller, &set_pos);
	af_cxt->cur_af_pos = motot_pos;

	return rtn;
}

static cmr_s32 _alg_init(afv1_handle_t handle, struct afctrl_init_in *init_param, struct afctrl_init_out *result)
{
	cmr_s32 rtn = AFV1_SUCCESS;
	struct af_context_t *af_cxt = (struct af_context_t *)handle;
	struct af_alg_init_param alg_init_param;
	struct af_alg_init_result alg_result;
	cmr_u32 i;

	rtn = _check_handle(handle);
	if (AFV1_SUCCESS != rtn) {
		ISP_LOGE(" fail to check cxt");
		return AFV1_ERROR;
	}

	alg_init_param.tuning_param_cnt = init_param->tuning_param_cnt;
	alg_init_param.cur_tuning_mode = init_param->cur_tuning_mode;
	alg_init_param.init_mode = init_param->af_mode;
	alg_init_param.plat_info.afm_filter_type_cnt = init_param->plat_info.afm_filter_type_cnt;
	alg_init_param.plat_info.afm_win_max_cnt = init_param->plat_info.afm_win_max_cnt;
	if (init_param->tuning_param_cnt > 0) {
		alg_init_param.tuning_param = (struct af_alg_tuning_block_param *)malloc(sizeof(*alg_init_param.tuning_param) * init_param->tuning_param_cnt);
		if (NULL == alg_init_param.tuning_param) {
			ISP_LOGE("fail to malloc men for tuning_param");
			return AFV1_ERROR;
		}
		for (i = 0; i < init_param->tuning_param_cnt; i++) {
			alg_init_param.tuning_param[i].data = init_param->tuning_param[i].data;
			alg_init_param.tuning_param[i].data_len = init_param->tuning_param[i].data_len;
			alg_init_param.tuning_param[i].cfg_mode = init_param->tuning_param[i].cfg_mode;
		}
	}

	af_cxt->af_alg_handle = af_cxt->lib_ops.af_init(&alg_init_param, &alg_result);
	free(alg_init_param.tuning_param);
	if (NULL == af_cxt->af_alg_handle) {
		ISP_LOGE("fail to init af alg handle");
		return AFV1_ERROR;
	}
	result->init_motor_pos = alg_result.init_motor_pos;
	return rtn;

}

static cmr_s32 _cfg_af_calc_param(afv1_handle_t handle, struct af_calc_param *param, struct af_alg_calc_param *alg_calc_param)
{
	cmr_s32 rtn = AFV1_SUCCESS;
	struct af_context_t *af_cxt = (struct af_context_t *)handle;
	cmr_u32 i;

	rtn = _check_handle(handle);
	if (AFV1_SUCCESS != rtn) {
		ISP_LOGE("fail to check cxt");
		return AFV1_ERROR;
	}

	switch (param->data_type) {
	case AF_DATA_AF:
		alg_calc_param->active_data_type = AF_ALG_DATA_AF;
		{
			struct af_filter_info *filter_info = (struct af_filter_info *)param->data;
			alg_calc_param->afm_info.win_cfg.win_cnt = af_cxt->win_cfg.win_cnt;
			alg_calc_param->afm_info.win_cfg.win_sel_mode = af_cxt->win_cfg.win_sel_mode;
			if (0 == alg_calc_param->afm_info.win_cfg.win_cnt) {
				alg_calc_param->afm_info.win_cfg.win_cnt = 1;
			}
			for (i = 0; i < af_cxt->win_cfg.win_cnt; i++) {
				alg_calc_param->afm_info.win_cfg.win_pos[i].sx = af_cxt->win_cfg.win_pos[i].sx;
				alg_calc_param->afm_info.win_cfg.win_pos[i].sy = af_cxt->win_cfg.win_pos[i].sy;
				alg_calc_param->afm_info.win_cfg.win_pos[i].ex = af_cxt->win_cfg.win_pos[i].ex;
				alg_calc_param->afm_info.win_cfg.win_pos[i].ey = af_cxt->win_cfg.win_pos[i].ey;
				alg_calc_param->afm_info.win_cfg.win_prio[i] = af_cxt->win_cfg.win_prio[i];
			}
			alg_calc_param->afm_info.filter_info.filter_num = filter_info->filter_num;
			for (i = 0; i < filter_info->filter_num; i++) {
				alg_calc_param->afm_info.filter_info.filter_data[i].type = filter_info->filter_data[i].type;
				alg_calc_param->afm_info.filter_info.filter_data[i].data = filter_info->filter_data[i].data;
			}

		}
		break;

	case AF_DATA_IMG_BLK:
		alg_calc_param->active_data_type = AF_ALG_DATA_IMG_BLK;
		{
			struct af_img_blk_info *img_blk_info = (struct af_img_blk_info *)param->data;
			alg_calc_param->img_blk_info.block_w = img_blk_info->block_w;
			alg_calc_param->img_blk_info.block_h = img_blk_info->block_h;
			alg_calc_param->img_blk_info.pix_per_blk = img_blk_info->pix_per_blk;
			alg_calc_param->img_blk_info.chn_num = img_blk_info->chn_num;
			alg_calc_param->img_blk_info.data = img_blk_info->data;
		}
		break;

	case AF_DATA_AE:
		alg_calc_param->active_data_type = AF_ALG_DATA_AE;
		{
			struct af_ae_info *ae_info = (struct af_ae_info *)param->data;
			alg_calc_param->ae_info.exp_time = ae_info->exp_time;
			alg_calc_param->ae_info.gain = ae_info->gain;
			alg_calc_param->ae_info.cur_fps = ae_info->cur_fps;
			alg_calc_param->ae_info.cur_lum = ae_info->cur_lum;
			alg_calc_param->ae_info.target_lum = ae_info->target_lum;
			alg_calc_param->ae_info.is_stable = ae_info->is_stable;
		}
		break;

	case AF_DATA_FD:
		alg_calc_param->active_data_type = AF_ALG_DATA_FD;
		{
			struct af_fd_info *fd_info = (struct af_fd_info *)param->data;
			alg_calc_param->fd_info.face_num = fd_info->face_num;
			for (i = 0; i < fd_info->face_num; i++) {
				alg_calc_param->fd_info.face_pose[i].sx = fd_info->face_pose[i].sx;
				alg_calc_param->fd_info.face_pose[i].sy = fd_info->face_pose[i].sy;
				alg_calc_param->fd_info.face_pose[i].ex = fd_info->face_pose[i].ex;
				alg_calc_param->fd_info.face_pose[i].ey = fd_info->face_pose[i].ey;
			}
		}
		break;

	default:
		ISP_LOGE("fail to get valid cmd, type: %d", param->data_type);
		rtn = AFV1_ERROR;
		break;
	}

	return rtn;
}

static cmr_s32 _af_cfg_afm_win(afv1_handle_t handle, cmr_u32 type, cmr_u32 win_num, struct af_win_rect *win_pos)
{
	cmr_s32 rtn = AFV1_SUCCESS;
	struct af_context_t *af_cxt = (struct af_context_t *)handle;
//      isp_ctrl_context *ctrl_context = NULL;
	cmr_u32 hw_max_win_num;
	cmr_u32 i;
	cmr_u32 w, h;
	struct af_win_rect set_pos[MAX_AF_WIN];
	struct af_monitor_win monitor_win;

	rtn = _check_handle(handle);
	if (AFV1_SUCCESS != rtn) {
		ISP_LOGE("fail to check cxt");
		return AFV1_ERROR;
	}
//      ctrl_context = (isp_ctrl_context *)af_cxt->caller;

	rtn = af_cxt->get_monitor_win_num(af_cxt->caller, &hw_max_win_num);
	hw_max_win_num = hw_max_win_num > MAX_AF_WIN ? MAX_AF_WIN : hw_max_win_num;

	ISP_LOGV("max hw win num %d, cfg win num %d", hw_max_win_num, win_num);
	ISP_LOGV("isp w %d, h %d", af_cxt->src.w, af_cxt->src.h);

	if (0 == win_num) {
		w = af_cxt->src.w;
		h = af_cxt->src.h;
		win_num = 1;
		set_pos[0].sx = ((((w >> 1) - (w / 10)) >> 1) << 1);
		set_pos[0].sy = ((((h >> 1) - (h / 10)) >> 1) << 1);
		set_pos[0].ex = ((((w >> 1) + (w / 10)) >> 1) << 1);
		set_pos[0].ey = ((((h >> 1) + (h / 10)) >> 1) << 1);

	} else {
		for (i = 0; i < win_num; i++) {
			set_pos[i] = win_pos[i];
		}
	}

	af_cxt->win_cfg.win_cnt = win_num;
	af_cxt->win_cfg.win_sel_mode = 0;
	for (i = 0; i < win_num; i++) {
		af_cxt->win_cfg.win_pos[i].sx = set_pos[i].sx;
		af_cxt->win_cfg.win_pos[i].sy = set_pos[i].sy;
		af_cxt->win_cfg.win_pos[i].ex = set_pos[i].ex;
		af_cxt->win_cfg.win_pos[i].ey = set_pos[i].ey;
		af_cxt->win_cfg.win_prio[i] = 1;
	}

	ISP_LOGV("sx w %d, sy %d,ex w %d, ey %d", set_pos[0].sx, set_pos[0].sy, set_pos[0].ex, set_pos[0].ey);

	for (i = win_num; i < hw_max_win_num; i++) {
		set_pos[i] = set_pos[0];
	}
	monitor_win.type = type;
	monitor_win.win_pos = set_pos;
	rtn = af_cxt->set_monitor_win(af_cxt->caller, &monitor_win);
	return rtn;

}

static cmr_s32 _af_set_status(afv1_handle_t handle, cmr_u32 af_status)
{
	cmr_s32 rtn = AFV1_SUCCESS;
	struct af_context_t *af_cxt = (struct af_context_t *)handle;
	cmr_u32 set_status;

	rtn = _check_handle(handle);
	if (AFV1_SUCCESS != rtn) {
		ISP_LOGE("fail to check cxt");
		return AFV1_ERROR;
	}

	switch (af_status) {
	case AF_STATUS_START:
		set_status = AF_ALG_STATUS_START;
		break;
	case AF_STATUS_RUNNING:
		set_status = AF_ALG_STATUS_RUNNING;
		break;
	case AF_STATUS_FINISH:
		set_status = AF_ALG_STATUS_FINISH;
		break;
	case AF_STATUS_PAUSE:
		set_status = AF_ALG_STATUS_PAUSE;
		break;
	case AF_STATUS_RESUME:
		set_status = AF_ALG_STATUS_RESUME;
		break;
	case AF_STATUS_RESTART:
		set_status = AF_ALG_STATUS_RESTART;
		break;
	case AF_STATUS_STOP:
		set_status = AF_ALG_STATUS_STOP;
		break;
	default:
		ISP_LOGE("fail to get af status %d", af_status);
		rtn = AF_PARAM_ERROR;
		break;
	}
	if (AFV1_SUCCESS == rtn) {
		rtn = af_cxt->lib_ops.af_ioctrl(af_cxt->af_alg_handle, AF_ALG_CMD_SET_AF_STATUS, (void *)&set_status, NULL);
	}

	return rtn;

}

static cmr_s32 _af_set_start(afv1_handle_t handle, struct af_trig_info *trig_info)
{
	cmr_s32 rtn = AFV1_SUCCESS;
	struct af_context_t *af_cxt = (struct af_context_t *)handle;
	struct af_monitor_set monitor_set;
	cmr_u32 af_pos;

	rtn = _check_handle(handle);
	if (AFV1_SUCCESS != rtn) {
		ISP_LOGE("fail to check handle");
		return AFV1_ERROR;
	}
	rtn = af_cxt->lib_ops.af_ioctrl(af_cxt->af_alg_handle, AF_ALG_CMD_GET_AF_INIT_POS, (void *)&af_pos, NULL);
	rtn = _af_set_motor_pos(handle, af_pos);

	if (0 == af_cxt->ae_awb_lock_cnt) {
		rtn = af_cxt->ae_awb_lock(af_cxt->caller);
		af_cxt->ae_awb_lock_cnt++;
	}

	af_cxt->is_running = AF_TRUE;
	rtn = _af_cfg_afm_win(handle, 1, trig_info->win_num, trig_info->win_pos);
	rtn = _af_set_status(handle, AF_ALG_STATUS_START);
	monitor_set.bypass = 0;
	monitor_set.int_mode = 1;
	monitor_set.need_denoise = 0;
	monitor_set.skip_num = 0;
	monitor_set.type = 1;
	rtn = af_cxt->set_monitor(af_cxt->caller, &monitor_set, af_cxt->cur_envi);
	return rtn;
}

static cmr_s32 _af_end_proc(afv1_handle_t handle, struct af_result_param *result, cmr_u32 need_notice)
{
	cmr_s32 rtn = AFV1_SUCCESS;
	struct af_context_t *af_cxt = (struct af_context_t *)handle;
	struct af_monitor_set monitor_set;

	rtn = _check_handle(handle);
	if (AFV1_SUCCESS != rtn) {
		ISP_LOGE("fail to check handle");
		return AFV1_ERROR;
	}
	monitor_set.bypass = 1;
	monitor_set.int_mode = 1;
	monitor_set.need_denoise = 0;
	monitor_set.skip_num = 0;
	monitor_set.type = 1;
	rtn = af_cxt->set_monitor(af_cxt->caller, &monitor_set, af_cxt->cur_envi);
	if (need_notice) {
		af_cxt->end_notice(af_cxt->caller, result);
	}
	af_cxt->af_result = *result;
	af_cxt->af_has_suc_rec = AF_TRUE;
	if (af_cxt->ae_awb_lock_cnt) {
		af_cxt->ae_awb_release(af_cxt->caller);
		af_cxt->ae_awb_lock_cnt--;
	}
	af_cxt->is_running = AF_FALSE;

	return rtn;
}

static cmr_s32 _af_finish(afv1_handle_t handle, struct af_result_param *result)
{
	cmr_s32 rtn = AFV1_SUCCESS;
	struct af_context_t *af_cxt = (struct af_context_t *)handle;

	rtn = _check_handle(handle);
	if (AFV1_SUCCESS != rtn) {
		ISP_LOGE("fail to check handle");
		return AFV1_ERROR;
	}
	rtn = af_ioctrl(handle, AF_CMD_SET_AF_FINISH, (void *)result, NULL);

	return rtn;
}

static cmr_s32 _caf_trig_af_start(afv1_handle_t handle)
{
	cmr_s32 rtn = AFV1_SUCCESS;
	struct af_context_t *af_cxt = (struct af_context_t *)handle;
	struct af_trig_info trig_info;

	rtn = _check_handle(handle);
	if (AFV1_SUCCESS != rtn) {
		ISP_LOGE("fail to check handle");
		return AFV1_ERROR;
	}

	trig_info.win_num = 0;
	trig_info.mode = af_cxt->af_mode;

	rtn = af_ioctrl(handle, AF_CMD_SET_CAF_TRIG_START, (void *)&trig_info, NULL);

	return rtn;
}

static cmr_s32 _caf_reset(afv1_handle_t handle)
{
	cmr_s32 rtn = AFV1_SUCCESS;
	struct af_context_t *af_cxt = (struct af_context_t *)handle;

	rtn = _check_handle(handle);
	if (AFV1_SUCCESS != rtn) {
		ISP_LOGE("fail to check handle");
		return AFV1_ERROR;
	}

	rtn = af_ioctrl(handle, AF_CMD_SET_CAF_RESET, NULL, NULL);

	return rtn;
}

static cmr_s32 _caf_stop(afv1_handle_t handle)
{
	cmr_s32 rtn = AFV1_SUCCESS;
	struct af_context_t *af_cxt = (struct af_context_t *)handle;

	rtn = _check_handle(handle);
	if (AFV1_SUCCESS != rtn) {
		ISP_LOGE("fail to check handle");
		return AFV1_ERROR;
	}

	rtn = af_ioctrl(handle, AF_CMD_SET_CAF_STOP, NULL, NULL);

	return rtn;
}

static cmr_s32 _caf_reset_after_af(afv1_handle_t handle)
{
	cmr_s32 rtn = AFV1_SUCCESS;
	struct af_context_t *af_cxt = (struct af_context_t *)handle;

	rtn = _check_handle(handle);
	if (AFV1_SUCCESS != rtn) {
		ISP_LOGE("fail to check handle");
		return AFV1_ERROR;
	}

	if ((AF_MODE_CONTINUE == af_cxt->af_mode) || (AF_MODE_VIDEO == af_cxt->af_mode)) {
		rtn = _caf_reset(handle);
	}

	return rtn;
}
#endif

static cmr_s32 trigger_start(af_ctrl_t * af);
static cmr_s32 trigger_stop(af_ctrl_t * af);
static cmr_s32 trigger_set_mode(af_ctrl_t * af, enum aft_mode mode);
static cmr_s32 trigger_calc(af_ctrl_t * af, struct aft_proc_calc_param *prm, struct aft_proc_result *res);
static void caf_start(af_ctrl_t * af);
static void caf_stop_monitor(af_ctrl_t * af);
static cmr_u16 get_vcm_registor_pos(af_ctrl_t * af);
static cmr_s32 compare_timestamp(af_ctrl_t * af);
static ERRCODE af_clear_sem(af_ctrl_t * af);
static ERRCODE af_wait_caf_finish(af_ctrl_t * af);

static cmr_s32 af_sprd_set_mode(afv1_handle_t handle, void *in_param)
{
	af_ctrl_t *af = (af_ctrl_t *) handle;
	cmr_u32 af_mode = *(cmr_u32 *) in_param;
	cmr_s32 rtn = AFV1_SUCCESS;

	rtn = _check_handle(handle);
	if (AFV1_SUCCESS != rtn) {
		ISP_LOGE("fail to check handle");
		return AFV1_ERROR;
	}

	property_get("af_mode", AF_MODE, "none");
	if (0 == strcmp(AF_MODE, "none")) {
		ISP_LOGV("state = %s, mode = %d", STATE_STRING(af->state), af_mode);
		switch (af_mode) {
		case AF_MODE_NORMAL:
			af->request_mode = af_mode;
			//af->state = STATE_NORMAL_AF;
			trigger_set_mode(af, AFT_MODE_NORMAL);
			trigger_stop(af);
			break;
		case AF_MODE_CONTINUE:
		case AF_MODE_VIDEO:
			ISP_LOGV("af state = %s, caf state = %s", STATE_STRING(af->state), CAF_STATE_STR(af->caf_state));
			//face af is not worked for now
			//if (STATE_FAF == af->state) {
			//	return 0;
			//}
			af->request_mode = af_mode;
			af->state = AF_MODE_CONTINUE == af_mode ? STATE_CAF : STATE_RECORD_CAF;
			caf_start(af);
			break;

		case AF_MODE_PICTURE:
			ISP_LOGV("AF_mode = %d, SAF_Search_Process = %d, need_re_trigger :%d", af->fv.AF_mode, af->fv.sAF_Data.sAFInfo.SAF_Search_Process, af->need_re_trigger);
			if (af->need_re_trigger == 0){
				//trigger_stop(af);
			}
			af->takePicture_timeout = 0;
			if (Wait_Trigger != af->fv.AF_mode
			    || (SAF_Search_DONE != af->fv.sAF_Data.sAFInfo.SAF_Search_Process && SAF_Search_INIT != af->fv.sAF_Data.sAFInfo.SAF_Search_Process)
			    || af->need_re_trigger || DCAM_AFTER_VCM_NO == compare_timestamp(af)) {
				af_clear_sem(af);
				af_wait_caf_finish(af);
				af->state = STATE_CAF; // todo : af state should be STATE_NORMAL_AF
				caf_start(af); // todo : caf could not be started actually
			};
			ISP_LOGV("dcam_timestamp-vcm_timestamp = %lld ms", ((cmr_s64) af->dcam_timestamp - (cmr_s64) af->vcm_timestamp) / 1000000);
			get_vcm_registor_pos(af);
			break;
		case AF_MODE_FULLSCAN:
			af->request_mode = af_mode;
			af->state = STATE_FULLSCAN;
			trigger_set_mode(af, AFT_MODE_NORMAL);
			trigger_stop(af);
			break;
		default:
			ISP_LOGW("af_mode %d is not supported", af_mode);
			if ((STATE_CAF == af->state) || (STATE_RECORD_CAF == af->state)) {
				if (0 == af->need_re_trigger) {
					trigger_stop(af);
				}
			}
			break;
		}
	} else {
		ISP_LOGV("AF_MODE %s is not null, af test mode", AF_MODE);
		//set_af_test_mode(af,AF_MODE);// only one thread could call it
		get_vcm_registor_pos(af);	// get final vcm pos when in test mode
	}
	return rtn;
}

/***************************************************************************/
/* porting form 2.0 begin */
static ERRCODE if_get_motor_pos(uint16 * motor_pos, void *cookie);

// afm hardware
static void afm_enable(af_ctrl_t * af)
{
#if 1
	cmr_s32 rtn = AFV1_SUCCESS;
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)af->caller;
	struct isp_alg_fw_context *isp_ctx = (struct isp_alg_fw_context *)cxt_ptr->caller_handle;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_ctx->dev_access_handle;
	cmr_handle device = cxt->isp_driver_handle;
	isp_u_raw_afm_bypass(device, 0);
#else
	//struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)af->caller;
	struct af_monitor_set monitor_set;

	monitor_set.bypass = 0;
	monitor_set.int_mode = 1;
	monitor_set.need_denoise = 0;
	monitor_set.skip_num = 0;
	monitor_set.type = 1;
	rtn = af->set_monitor(af->caller, &monitor_set, af->curr_scene);
#endif
}

static void afm_disable(af_ctrl_t * af)
{
#if 1
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)af->caller;
	struct isp_alg_fw_context *isp_ctx = (struct isp_alg_fw_context *)cxt_ptr->caller_handle;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_ctx->dev_access_handle;
	cmr_handle device = cxt->isp_driver_handle;
	isp_u_raw_afm_bypass(device, 1);
#else
	//struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)af->caller;
/*		struct af_monitor_set monitor_set;

	monitor_set.bypass = 1;
	monitor_set.int_mode = 1;
	monitor_set.need_denoise = 0;
	monitor_set.skip_num = 0;
	monitor_set.type = 1;
	rtn = af->set_monitor(af->caller, &monitor_set, af->curr_scene);*/
#endif
}

static void afm_setup(af_ctrl_t * af)
{
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)af->caller;
	struct isp_alg_fw_context *isp_ctx = (struct isp_alg_fw_context *)cxt_ptr->caller_handle;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_ctx->dev_access_handle;
	cmr_handle device = cxt->isp_driver_handle;

	//isp_ctrl_context *isp = af->isp_ctx;
	//isp_handle device = isp->handle_device;

	// ISP_LOGV("scene = %d, spsmd_max = 0x%x, spsmd_min = 0x%x, sobel_max = 0x%x, sobel_min = 0x%x",
	//     af->curr_scene,
	//     af->filter_clip[af->curr_scene].spsmd_max, af->filter_clip[af->curr_scene].spsmd_min,
	//     af->filter_clip[af->curr_scene].sobel_max, af->filter_clip[af->curr_scene].sobel_min);

	af->thrd.sobel5_thr_min_red = af->filter_clip[af->curr_scene][af->ae.gain_index].sobel_min;
	af->thrd.sobel5_thr_max_red = af->filter_clip[af->curr_scene][af->ae.gain_index].sobel_max;
	af->thrd.sobel5_thr_min_green = af->filter_clip[af->curr_scene][af->ae.gain_index].sobel_min;
	af->thrd.sobel5_thr_max_green = af->filter_clip[af->curr_scene][af->ae.gain_index].sobel_max;
	af->thrd.sobel5_thr_min_blue = af->filter_clip[af->curr_scene][af->ae.gain_index].sobel_min;
	af->thrd.sobel5_thr_max_blue = af->filter_clip[af->curr_scene][af->ae.gain_index].sobel_max;
	af->thrd.sobel9_thr_min_red = af->filter_clip[af->curr_scene][af->ae.gain_index].sobel_min;
	af->thrd.sobel9_thr_max_red = af->filter_clip[af->curr_scene][af->ae.gain_index].sobel_max;
	af->thrd.sobel9_thr_min_green = af->filter_clip[af->curr_scene][af->ae.gain_index].sobel_min;
	af->thrd.sobel9_thr_max_green = af->filter_clip[af->curr_scene][af->ae.gain_index].sobel_max;
	af->thrd.sobel9_thr_min_blue = af->filter_clip[af->curr_scene][af->ae.gain_index].sobel_min;
	af->thrd.sobel9_thr_max_blue = af->filter_clip[af->curr_scene][af->ae.gain_index].sobel_max;
	af->thrd.spsmd_thr_min_red = af->filter_clip[af->curr_scene][af->ae.gain_index].spsmd_min;
	af->thrd.spsmd_thr_max_red = af->filter_clip[af->curr_scene][af->ae.gain_index].spsmd_max;
	af->thrd.spsmd_thr_min_green = af->filter_clip[af->curr_scene][af->ae.gain_index].spsmd_min;
	af->thrd.spsmd_thr_max_green = af->filter_clip[af->curr_scene][af->ae.gain_index].spsmd_max;
	af->thrd.spsmd_thr_min_blue = af->filter_clip[af->curr_scene][af->ae.gain_index].spsmd_min;
	af->thrd.spsmd_thr_max_blue = af->filter_clip[af->curr_scene][af->ae.gain_index].spsmd_max;
	if (af->stat_reg.force_write) {
		af->thrd.spsmd_thr_min_red = af->stat_reg.reg_param[0];
		af->thrd.spsmd_thr_max_red = af->stat_reg.reg_param[1];
		af->thrd.spsmd_thr_min_blue = af->thrd.spsmd_thr_min_green = af->thrd.spsmd_thr_min_red;
		af->thrd.spsmd_thr_max_blue = af->thrd.spsmd_thr_max_green = af->thrd.spsmd_thr_max_red;
		ISP_LOGV("force write reg: %d ~ %d \n", af->stat_reg.reg_param[0]
			, af->stat_reg.reg_param[1]);
	}
/*
	struct af_enhanced_module_info {
		cmr_u8 chl_sel;
		cmr_u8 nr_mode;
		cmr_u8 center_weight;
		cmr_u8 fv_enhanced_mode[2];
		cmr_u8 clip_en[2];
		cmr_u32 max_th[2];
		cmr_u32 min_th[2];
		cmr_u8 fv_shift[2];
		char fv1_coeff[36];
	};

	struct af_iir_nr_info {
		cmr_u8 iir_nr_en;
		cmr_s16 iir_g0;
		cmr_s16 iir_c1;
		cmr_s16 iir_c2;
		cmr_s16 iir_c3;
		cmr_s16 iir_c4;
		cmr_s16 iir_c5;
		cmr_s16 iir_g1;
		cmr_s16 iir_c6;
		cmr_s16 iir_c7;
		cmr_s16 iir_c8;
		cmr_s16 iir_c9;
		cmr_s16 iir_c10;
	};
*/
	memcpy(&(af->af_iir_nr), &(af_iir_nr[iir_level]), sizeof(struct af_iir_nr_info));
	af->af_enhanced_module.chl_sel = 0;
	af->af_enhanced_module.nr_mode = (cmr_u8) nr_mode;
	af->af_enhanced_module.center_weight = (cmr_u8) cw_mode;
	af->af_enhanced_module.fv_enhanced_mode[0] = (cmr_u8) fv0_e;
	af->af_enhanced_module.fv_enhanced_mode[1] = (cmr_u8) fv1_e;
	af->af_enhanced_module.clip_en[0] = 0;
	af->af_enhanced_module.clip_en[1] = 0;
	af->af_enhanced_module.max_th[0] = 131071;
	af->af_enhanced_module.max_th[1] = 131071;
	af->af_enhanced_module.min_th[0] = 100;
	af->af_enhanced_module.min_th[1] = 100;
	af->af_enhanced_module.fv_shift[0] = 0;
	af->af_enhanced_module.fv_shift[1] = 0;
	memcpy(&(af->af_enhanced_module.fv1_coeff), &fv1_coeff, sizeof(fv1_coeff));

	if (ISP_CHIP_ID_TSHARK3 == isp_dev_get_chip_id(device)) {
		isp_u_raw_afm_skip_num_clr(device, 1);
		isp_u_raw_afm_skip_num_clr(device, 0);
		isp_u_raw_afm_skip_num(device, 0);
		isp_u_raw_afm_spsmd_rtgbot_enable(device, 1);
		isp_u_raw_afm_spsmd_diagonal_enable(device, 1);
		isp_u_raw_afm_spsmd_square_en(device, 1);
		isp_u_raw_afm_overflow_protect(device, 0);
		isp_u_raw_afm_subfilter(device, 0, 1);
		isp_u_raw_afm_spsmd_touch_mode(device, 0);
		isp_u_raw_afm_shfit(device, 0, 0, 0);
		isp_u_raw_afm_threshold_rgb(device, &af->thrd);
		isp_u_raw_afm_mode(device, 1);
	} else {
#if 0
		isp_u_raw_afm_skip_num_clr(device, 1);
		isp_u_raw_afm_skip_num_clr(device, 0);
		isp_u_raw_afm_spsmd_rtgbot_enable(device, 1);
		isp_u_raw_afm_spsmd_diagonal_enable(device, 1);
		isp_u_raw_afm_spsmd_cal_mode(device, 1);
		isp_u_raw_afm_sel_filter1(device, 0);	// 0 stands for spsmd
		isp_u_raw_afm_sel_filter2(device, 1);	// 1 stands for sobel9x9
		isp_u_raw_afm_sobel_type(device, 0);
		isp_u_raw_afm_spsmd_type(device, 2);
		isp_u_raw_afm_sobel_threshold(device, af->thrd.sobel9_thr_min_green, af->thrd.sobel9_thr_max_green);
		isp_u_raw_afm_spsmd_threshold(device, af->thrd.spsmd_thr_min_green, af->thrd.spsmd_thr_max_green);
#endif
		isp_u_raw_afm_skip_num(device, af->afm_skip_num);
		isp_u_raw_afm_mode(device, 1);
		isp_u_raw_afm_iir_nr_cfg(cxt->isp_driver_handle, (void *)&(af->af_iir_nr));
		isp_u_raw_afm_modules_cfg(cxt->isp_driver_handle, (void *)&(af->af_enhanced_module));
	}
}

static cmr_u32 afm_get_win_num(struct afctrl_init_in *input_param)
{
	cmr_u32 num;
	struct afctrl_init_in *input_ptr = input_param;
	//isp_u_raw_afm_win_num(isp->handle_device, &num);
	input_ptr->get_monitor_win_num(input_ptr->caller, &num);
	ISP_LOGI("win_num %d", num);
	return num;
}

//static void afm_set_win(isp_ctrl_context *isp, win_coord_t *win, cmr_s32 num, cmr_s32 hw_num)
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

	//isp_u_raw_afm_win(isp->handle_device, win);
	winparam.win_pos = (struct af_win_rect *)win;	//gwb compare with kernel type

	af->set_monitor_win(af->caller, &winparam);
}

//static cmr_s32 afm_get_fv(isp_ctrl_context *isp, cmr_u64 *fv, cmr_u32 filter_mask, cmr_s32 roi_num,cmr_s32 ring_buffer)
static cmr_s32 afm_get_fv(af_ctrl_t * af, cmr_u64 * fv, cmr_u32 filter_mask, cmr_s32 roi_num, cmr_s32 ring_buffer)
{
	cmr_s32 i, num;
	cmr_u64 *p;
	//af_ctrl_t *af = (af_ctrl_t *)isp->handle_af;
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)af->caller;
	struct isp_alg_fw_context *isp_ctx = (struct isp_alg_fw_context *)cxt_ptr->caller_handle;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_ctx->dev_access_handle;
	cmr_handle device = cxt->isp_driver_handle;

	num = 0;
	p = fv;

	if (0 == ring_buffer) {
		if (ISP_CHIP_ID_TSHARK3 == isp_dev_get_chip_id(device)) {
			cmr_u32 data[105];

			isp_u_raw_afm_statistic_r6p9(device, data);

			if (filter_mask & SOBEL5_BIT) {
				num++;
				for (i = 0; i < roi_num; ++i)
					*p++ = data[1 + i * 3];
			}

			if (filter_mask & SOBEL9_BIT) {
				num++;
				for (i = 0; i < roi_num; ++i)
					*p++ = data[31 + i * 3];
			}

			if (filter_mask & SPSMD_BIT) {
				num++;
				for (i = 0; i < roi_num; ++i) {
					cmr_u64 high = data[95 + i / 2];
					high = (i & 0x01) ? ((high & 0x00FF0000) << 16) : ((high & 0x000000FF) << 32);
					*p++ = data[61 + i * 3] + high;
				}
			}

		} else {
			cmr_u32 data[25];

			if (filter_mask & SOBEL5_BIT) {
				num++;
				for (i = 0; i < roi_num; ++i)
					*p++ = 0;
			}

			if (filter_mask & SOBEL9_BIT) {
				num++;
				isp_u_raw_afm_type2_statistic(device, data);
				for (i = 0; i < roi_num; ++i)
					*p++ = data[i];
			}

			if (filter_mask & SPSMD_BIT) {
				num++;
				isp_u_raw_afm_type1_statistic(device, data);
				for (i = 0; i < roi_num; ++i)
					*p++ = data[i];
			}

			if (filter_mask & ENHANCED_BIT) {
				num++;
				/*
				   for (i = 0; i < roi_num; i++) {
				   ISP_LOGV("fv0[%d]:%ld,", i, af->af_fv_val.af_fv0[i]);
				   }

				   for (i = 0; i < roi_num; i++) {
				   ISP_LOGV("fv1[%d]:%ld,", i, af->af_fv_val.af_fv1[i]);
				   }
				 */
				for (i = 0; i < roi_num; ++i) {
					*p++ = af->af_fv_val.af_fv0[i];
				}
			}

		}
	} else {		// ring buffer path
		if (ISP_CHIP_ID_TSHARK3 == isp_dev_get_chip_id(device)) {
			//cmr_u32 data[105];

			//isp_u_raw_afm_statistic_r6p9(isp->handle_device, data);

			if (filter_mask & SOBEL5_BIT) {
				num++;
				for (i = 0; i < roi_num; ++i)
					//*p++ = data[1 + i * 3];
					*p++ = af->af_statistics[1 + i * 3];
			}

			if (filter_mask & SOBEL9_BIT) {
				num++;
				for (i = 0; i < roi_num; ++i)
					//*p++ = data[31 + i * 3];
					*p++ = af->af_statistics[31 + i * 3];
			}

			if (filter_mask & SPSMD_BIT) {
				num++;
				for (i = 0; i < roi_num; ++i) {
					//cmr_u64 high = data[95 + i / 2];
					cmr_u64 high = af->af_statistics[95 + i / 2];
					high = (i & 0x01) ? ((high & 0x00FF0000) << 16) : ((high & 0x000000FF) << 32);
					//*p++ = data[61 + i * 3] + high;
					*p++ = af->af_statistics[61 + i * 3] + high;
				}
			}
		} else {
			//cmr_u32 data[25];

			if (filter_mask & SOBEL5_BIT) {
				num++;
				for (i = 0; i < roi_num; ++i)
					*p++ = 0;
			}

			if (filter_mask & SOBEL9_BIT) {
				num++;
				//isp_u_raw_afm_type2_statistic(isp->handle_device, data);
				for (i = 0; i < roi_num; ++i)
					//*p++ = data[i];
					*p++ = af->af_statistics[i];
			}

			if (filter_mask & SPSMD_BIT) {
				num++;
				//isp_u_raw_afm_type1_statistic(isp->handle_device, data);
				for (i = 0; i < roi_num; ++i)
					//*p++ = data[i];
					*p++ = af->af_statistics[i];
			}

			if (filter_mask & ENHANCED_BIT) {
				num++;

				for (i = 0; i < 5; i++) {
					//ISP_LOGV("fv0[%d]:%ld,", i, af->af_fv_val.af_fv0[i]);
				}

				for (i = 0; i < 5; i++) {
					//ISP_LOGV("fv1[%d]:%ld,", i, af->af_fv_val.af_fv0[i]);
				}

				for (i = 0; i < roi_num; ++i) {
					//*p++ = af->af_fv_val.af_fv0[i];
				}
			}

		}
	}
	return num;
}

// vcm ops
static void set_vcm_chip_ops(af_ctrl_t * af)
{
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)af->caller;
	struct isp_alg_fw_context *isp_ctx = (struct isp_alg_fw_context *)cxt_ptr->caller_handle;
//      isp_ctrl_context *isp = af->isp_ctx;

	memset(&af->vcm_ops, 0, sizeof(af->vcm_ops));
	af->vcm_ops.set_pos = isp_ctx->ioctrl_ptr->set_pos;
	af->vcm_ops.get_otp = isp_ctx->ioctrl_ptr->get_otp;
	af->vcm_ops.set_motor_bestmode = isp_ctx->ioctrl_ptr->set_motor_bestmode;
	af->vcm_ops.set_test_vcm_mode = isp_ctx->ioctrl_ptr->set_test_vcm_mode;
	af->vcm_ops.get_test_vcm_mode = isp_ctx->ioctrl_ptr->get_test_vcm_mode;
	af->vcm_ops.get_motor_pos = isp_ctx->ioctrl_ptr->get_motor_pos;

}

// len
static cmr_s32 lens_get_pos(af_ctrl_t * af)
{
	// ISP_LOGV("pos = %d", af->lens.pos);
	return af->lens.pos;
}

static void lens_move_to(af_ctrl_t * af, cmr_s32 pos)
{
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)af->caller;
	struct isp_alg_fw_context *isp_ctx = (struct isp_alg_fw_context *)cxt_ptr->caller_handle;
	// ISP_LOGV("pos = %d", pos);
	cmr_u16 last_pos = 0;

	last_pos = lens_get_pos(af);

	if (NULL != af->vcm_ops.set_pos) {
		if (last_pos != pos)
			//af->vcm_ops.set_pos(af->caller, pos); // must be provided
			af->vcm_ops.set_pos(isp_ctx->ioctrl_ptr->caller_handler, pos);	// must be provided
		ISP_LOGV("af->vcm_ops.set_pos = %d", pos);
		af->lens.pos = pos;
	}
}

// notify
enum notify_event {
	NOTIFY_START = 0,
	NOTIFY_STOP,
};

typedef struct _notify_result {
	cmr_u32 win_num;
} notify_result_t;

static cmr_s32 compare_timestamp(af_ctrl_t * af)
{

	if (af->dcam_timestamp < af->vcm_timestamp + 100000000LL)
		return DCAM_AFTER_VCM_NO;
	else
		return DCAM_AFTER_VCM_YES;
}

static void notify_start(af_ctrl_t * af)
{
	ISP_LOGI(".");
	af->start_notice(af->caller);
}

static void notify_stop(af_ctrl_t * af, cmr_s32 win_num)
{
	ISP_LOGI(". %s ", (win_num) ? "Suc" : "Fail");
	struct af_result_param af_result;
	af_result.suc_win = win_num;

	af->inited_af_req = (AFV1_FALSE);
	af->vcm_stable = 1;
	if (DCAM_AFTER_VCM_YES == compare_timestamp(af)) {
		sem_post(&af->af_wait_caf);	// should be protected by af_work_lock mutex exclusives
	}
	af->end_notice(af->caller, &af_result);
}

static cmr_s32 split_win(const win_coord_t * in, cmr_s32 h_num, cmr_s32 v_num, win_coord_t * out)
{
	cmr_s32 h, v, num;
	cmr_u32 width, height, starty;

	assert(v_num > 0);
	assert(h_num > 0);

	ISP_LOGV("win: start_x = %d, start_y = %d, end_x = %d, end_y = %d", in->start_x, in->start_y, in->end_x, in->end_y);

	width = (in->end_x - in->start_x + 1) / h_num;
	height = (in->end_y - in->start_y + 1) / v_num;
	starty = in->start_y;
	num = 0;

	for (v = 0; v < v_num; ++v) {
		cmr_u32 startx = in->start_x;
		cmr_u32 endy = starty + height - 1;

		for (h = 0; h < h_num; ++h) {
			out[num].start_x = (startx >> 1) << 1;	// make sure coordinations are even
			out[num].end_x = ((startx + width - 1) >> 1) << 1;
			out[num].start_y = (starty >> 1) << 1;
			out[num].end_y = (endy >> 1) << 1;
			ISP_LOGV("win %d: start_x = %d, start_y = %d, end_x = %d, end_y = %d", num, out[num].start_x, out[num].start_y, out[num].end_x, out[num].end_y);

			num++;
			startx += width;
		}

		starty += height;
	}

	return num;
}

static cmr_s32 split_roi(af_ctrl_t * af)
{
	roi_info_t *r = &af->roi;
	win_coord_t win_info;

	if (1 != r->num)
		return 0;

	switch (af->state) {
	case STATE_FAF:
		win_info = r->win[0];
		r->num = split_win(&win_info, 3, 3, r->win);
		break;
	case STATE_FULLSCAN:	// 3x3 windows
		win_info = r->win[0];
		r->num = split_win(&win_info, 3, 3, r->win);
		break;
	default:
		/* 0   1   2
		   3   4   5
		   6   7   8 */
		r->win[9].start_x = r->win[0].start_x;
		r->win[9].end_x = r->win[0].end_x;
		r->win[9].start_y = r->win[0].start_y;
		r->win[9].end_y = r->win[0].end_y;
		win_info = r->win[0];
		r->num = split_win(&win_info, 3, 3, r->win);
		r->num = r->num + 1;
#if 0
		r->win[1].start_x = r->win[0].start_x + 1.0 * (r->win[0].end_x - r->win[0].start_x) / 5;
		r->win[1].end_x = r->win[0].end_x - 1.0 * (r->win[0].end_x - r->win[0].start_x) / 5;
		r->win[1].start_y = r->win[0].start_y + 1.0 * (r->win[0].end_y - r->win[0].start_y) / 5;
		r->win[1].end_y = r->win[0].end_y - 1.0 * (r->win[0].end_y - r->win[0].start_y) / 5;

		r->win[1].start_x = r->win[1].start_x & 0xfffffffe;	// make sure coordinations are even
		r->win[1].end_x = r->win[1].end_x & 0xfffffffe;
		r->win[1].start_y = r->win[1].start_y & 0xfffffffe;
		r->win[1].end_y = r->win[1].end_y & 0xfffffffe;

		r->win[2].start_x = r->win[0].start_x + 2.0 * (r->win[0].end_x - r->win[0].start_x) / 5;
		r->win[2].end_x = r->win[0].end_x - 2.0 * (r->win[0].end_x - r->win[0].start_x) / 5;
		r->win[2].start_y = r->win[0].start_y + 2.0 * (r->win[0].end_y - r->win[0].start_y) / 5;
		r->win[2].end_y = r->win[0].end_y - 2.0 * (r->win[0].end_y - r->win[0].start_y) / 5;

		r->win[2].start_x = r->win[2].start_x & 0xfffffffe;	// make sure coordinations are even
		r->win[2].end_x = r->win[2].end_x & 0xfffffffe;
		r->win[2].start_y = r->win[2].start_y & 0xfffffffe;
		r->win[2].end_y = r->win[2].end_y & 0xfffffffe;

		r->num = 3;
#endif
		break;
	}
	return r->num;
}

static void calc_default_roi(af_ctrl_t * af)
{
	isp_info_t *hw = &af->isp_info;
	roi_info_t *roi = &af->roi;

	cmr_u32 w = hw->width;
	cmr_u32 h = hw->height;

	switch (af->state) {
	case STATE_FAF:
		roi->num = 1;
		roi->win[0].start_x = (af->face_base.sx >> 1) << 1;	// make sure coordinations are even
		roi->win[0].start_y = (af->face_base.sy >> 1) << 1;
		roi->win[0].end_x = (af->face_base.ex >> 1) << 1;
		roi->win[0].end_y = (af->face_base.ey >> 1) << 1;
		break;
	case STATE_FULLSCAN:
		/* for bokeh center w/5*4 h/5*4 */
		roi->num = 1;
		roi->win[0].start_x = (((w >> 1) - (w * 4 / 10)) >> 1) << 1;	// make sure coordinations are even
		roi->win[0].start_y = (((h >> 1) - (h * 4 / 10)) >> 1) << 1;
		roi->win[0].end_x = (((w >> 1) + (w * 4 / 10)) >> 1) << 1;
		roi->win[0].end_y = (((h >> 1) + (h * 4 / 10)) >> 1) << 1;
		break;
	default:
		roi->num = 1;
		/* center w/5*3 * h/5*3 */
		roi->win[0].start_x = (((w >> 1) - (w * 3 / 10)) >> 1) << 1;	// make sure coordinations are even
		roi->win[0].start_y = (((h >> 1) - (h * 3 / 10)) >> 1) << 1;
		roi->win[0].end_x = (((w >> 1) + (w * 3 / 10)) >> 1) << 1;
		roi->win[0].end_y = (((h >> 1) + (h * 3 / 10)) >> 1) << 1;
		break;
	}
	ISP_LOGV("af_state %s win 0: start_x = %d, start_y = %d, end_x = %d, end_y = %d",
		STATE_STRING(af->state), roi->win[0].start_x, roi->win[0].start_y, roi->win[0].end_x, roi->win[0].end_y);
}

static void calc_roi(af_ctrl_t * af, const struct af_trig_info *win, eAF_MODE alg_mode)
{
	roi_info_t *roi = &af->roi;

	switch (alg_mode) {
	case SAF:
		af->win_config = &af->af_tuning_data.SAF_win;
		break;
	case CAF:
		af->win_config = &af->af_tuning_data.CAF_win;
		break;
	case VAF:
		af->win_config = &af->af_tuning_data.VAF_win;
		break;
	default:
		af->win_config = &af->af_tuning_data.CAF_win;
		break;
	}

	if (1 != af->af_tuning_data.flag || 1 != af->win_config->win_strategic) {	//default window config
		if (win)
			ISP_LOGV("valid_win = %d, mode = %d", win->win_num, win->mode);
		else
			ISP_LOGV("win is NULL, use default roi");

		if (!win || (0 == win->win_num)) {
			af->touch = 0;
			calc_default_roi(af);
		} else {
			cmr_u32 i;

			af->touch = 1;
			roi->num = win->win_num;
			for (i = 0; i < win->win_num; ++i) {
				roi->win[i].start_x = (win->win_pos[i].sx >> 1) << 1;	// make sure coordinations are even
				roi->win[i].start_y = (win->win_pos[i].sy >> 1) << 1;
				roi->win[i].end_x = (win->win_pos[i].ex >> 1) << 1;
				roi->win[i].end_y = (win->win_pos[i].ey >> 1) << 1;
				ISP_LOGV("win %d: start_x = %d, start_y = %d, end_x = %d, end_y = %d", i,
					roi->win[i].start_x, roi->win[i].start_y, roi->win[i].end_x, roi->win[i].end_y);
			}
		}
		split_roi(af);
	} else {
		if (!win || (0 == win->win_num)) {
			cmr_u32 i = 0;

			af->touch = 0;
			roi->num = af->win_config->valid_win_num;
			for (i = 0; i < roi->num; i++) {	// the last window is for caf trigger
				roi->win[i].start_x = (af->win_config->win_pos[i].start_x >> 1) << 1;	// make sure coordinations are even
				roi->win[i].start_y = (af->win_config->win_pos[i].start_y >> 1) << 1;
				roi->win[i].end_x = (af->win_config->win_pos[i].end_x >> 1) << 1;
				roi->win[i].end_y = (af->win_config->win_pos[i].end_y >> 1) << 1;
			}
		} else {	// TAF
			cmr_u32 i, taf_w, taf_h;
			isp_info_t *hw = &af->isp_info;

			af->touch = 1;
			roi->num = win->win_num;
			for (i = 0; i < win->win_num; ++i) {
				roi->win[i].start_x = (win->win_pos[i].sx >> 1) << 1;	// make sure coordinations are even
				roi->win[i].start_y = (win->win_pos[i].sy >> 1) << 1;
				roi->win[i].end_x = (win->win_pos[i].ex >> 1) << 1;
				roi->win[i].end_y = (win->win_pos[i].ey >> 1) << 1;
				ISP_LOGV("win %d: start_x = %d, start_y = %d, end_x = %d, end_y = %d", i,
					roi->win[i].start_x, roi->win[i].start_y, roi->win[i].end_x, roi->win[i].end_y);
			}

			taf_w = roi->win[0].end_x - roi->win[0].start_x;
			taf_h = roi->win[0].end_y - roi->win[0].start_y;

			roi->num = af->win_config->valid_win_num;
			for (i = 1; i < roi->num; i++) {
				roi->win[i].start_x = 1.0 * (af->win_config->win_pos[i].start_x - af->win_config->win_pos[0].start_x) / hw->width * taf_w + roi->win[0].start_x;
				roi->win[i].start_y = 1.0 * (af->win_config->win_pos[i].start_y - af->win_config->win_pos[0].start_y) / hw->height * taf_h + roi->win[0].start_y;
				roi->win[i].end_x = 1.0 * (af->win_config->win_pos[i].end_x - af->win_config->win_pos[0].start_x) / hw->width * taf_w + roi->win[0].start_x;
				roi->win[i].end_y = 1.0 * (af->win_config->win_pos[i].end_y - af->win_config->win_pos[0].start_y) / hw->height * taf_h + roi->win[0].start_y;

				roi->win[i].start_x = (roi->win[i].start_x >> 1) << 1;	// make sure coordinations are even
				roi->win[i].start_y = (roi->win[i].start_y >> 1) << 1;
				roi->win[i].end_x = (roi->win[i].end_x >> 1) << 1;
				roi->win[i].end_y = (roi->win[i].end_y >> 1) << 1;
			}
			roi->win[0].start_x = roi->win[0].start_x;
			roi->win[0].start_y = roi->win[0].start_y;
			roi->win[0].end_x = 1.0 * (af->win_config->win_pos[0].end_x - af->win_config->win_pos[0].start_x) / hw->width * taf_w + roi->win[0].start_x;
			roi->win[0].end_y = 1.0 * (af->win_config->win_pos[0].end_y - af->win_config->win_pos[0].start_y) / hw->height * taf_h + roi->win[0].start_y;

			roi->win[0].start_x = (roi->win[0].start_x >> 1) << 1;	// make sure coordinations are even
			roi->win[0].start_y = (roi->win[0].start_y >> 1) << 1;
			roi->win[0].end_x = (roi->win[0].end_x >> 1) << 1;
			roi->win[0].end_y = (roi->win[0].end_y >> 1) << 1;
		}
	}
}

static void log_roi(af_ctrl_t * af)
{
	cmr_u32 i;

	roi_info_t *r = &af->roi;
	AF_Win *w = &af->fv.AF_Win_Data;

	w->Set_Zone_Num = r->num;
	for (i = 0; i < r->num; ++i) {
		w->AF_Win_X[i] = r->win[i].start_x;
		w->AF_Win_Y[i] = r->win[i].start_y;
		w->AF_Win_W[i] = r->win[i].end_x - r->win[i].start_x + 1;
		w->AF_Win_H[i] = r->win[i].end_y - r->win[i].start_y + 1;
	}
}

//reload tuning data by  curr_scene
static void get_tuning_param(af_ctrl_t * af)
{

	if (1 == af->af_tuning_data.flag) {
		memcpy(&af->fv.AF_Tuning_Data, &af->af_tuning_data.AF_Tuning_Data[af->curr_scene], sizeof(af->fv.AF_Tuning_Data));
	}

	return;
}

// start hardware
static cmr_s32 do_start_af(af_ctrl_t * af)
{
//    isp_ctrl_context *isp = af->isp_ctx;

	log_roi(af);
	afm_set_win(af, af->roi.win, af->roi.num, af->isp_info.win_num);
	afm_setup(af);
	afm_enable(af);
	get_tuning_param(af);	//reload tuning data by  curr_scene
	return 0;
}

// stop hardware
static cmr_s32 do_stop_af(af_ctrl_t * af)
{
//    isp_ctrl_context *isp = af->isp_ctx;
	afm_disable(af);
	return 0;
}

static cmr_s32 faf_trigger_init(af_ctrl_t * af);
// saf stuffs
static void saf_start(af_ctrl_t * af, struct af_trig_info *win)
{
	AF_Trigger_Data aft_in;
	af->algo_mode = SAF;
	memset(&aft_in,0,sizeof(AF_Trigger_Data));
	aft_in.AFT_mode = af->algo_mode;
	aft_in.bisTrigger = AF_TRIGGER;
	aft_in.AF_Trigger_Type = (1==af->defocus)? (DEFOCUS):(RF_NORMAL);
	calc_roi(af, win, af->algo_mode);
	AF_Trigger(&af->fv,&aft_in);
	do_start_af(af);
	af->vcm_stable = 0;
	faf_trigger_init(af);
}

static void saf_stop(af_ctrl_t * af)
{
	// do_stop_af(af);
	pthread_mutex_lock(&af->af_work_lock);
	AF_STOP(&af->fv, af->algo_mode);
	AF_Process_Frame(&af->fv);
	ISP_LOGV("AF_mode = %d", af->fv.AF_mode);
	pthread_mutex_unlock(&af->af_work_lock);
	// let it finish its job
//    AF_Process_Frame(&af->fv);
//    AF_Process_Frame(&af->fv);
}

static cmr_s32 saf_process_frame(af_ctrl_t * af)
{
	AF_Process_Frame(&af->fv);

	// ISP_LOGV("AF_mode = %d", af->fv.AF_mode);
	if (Wait_Trigger == af->fv.AF_mode) {
		cmr_u8 res;

		AF_Get_SAF_Result(&af->fv, &res);
		// ISP_LOGV("Normal AF end, result = %d", res);

		ISP_LOGV("notify_stop");
		notify_stop(af, HAVE_PEAK == res ? 1 : 0);
		return 1;
	} else {
		return 0;
	}
}

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

static ERRCODE if_get_sys_time(uint64 * time, void *cookie);
static ERRCODE if_aft_binfile_is_exist(uint8 * is_exist, void *cookie);
static ERRCODE if_is_aft_mlog(cmr_u32 * is_save, void *cookie);
static ERRCODE if_aft_log(cmr_u32 log_level, const char *format, ...);

static cmr_s32 trigger_init(af_ctrl_t * af, const char *lib_name)
{
	struct aft_tuning_block_param aft_in;
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)af->caller;
	struct isp_alg_fw_context *isp_ctx = (struct isp_alg_fw_context *)cxt_ptr->caller_handle;

	if (0 != load_trigger_lib(af, lib_name))
		return -1;

	struct isp_pm_ioctl_output aft_pm_output;
	memset((void *)&aft_pm_output, 0, sizeof(aft_pm_output));
	isp_pm_ioctl(isp_ctx->handle_pm, ISP_PM_CMD_GET_INIT_AFT, NULL, &aft_pm_output);

	if (PNULL == aft_pm_output.param_data || PNULL == aft_pm_output.param_data[0].data_ptr || 0 == aft_pm_output.param_data[0].data_size) {
		ISP_LOGI("aft tuning param error ");
		aft_in.data_len = 0;
		aft_in.data = NULL;
	} else {
		ISP_LOGI("aft tuning param ok ");
		aft_in.data_len = aft_pm_output.param_data[0].data_size;
		aft_in.data = aft_pm_output.param_data[0].data_ptr;
#if 0
		FILE *fp = NULL;
		fp = fopen("/data/mlog/aft_tuning.bin", "wb");
		fwrite(aft_in.data, 1, aft_in.data_len, fp);
		fclose(fp);
		ISP_LOGV("aft tuning size = %d", aft_in.data_len);
#endif
	}
	af->trig_ops.handle.aft_ops.aft_cookie = af;
	af->trig_ops.handle.aft_ops.get_sys_time = if_get_sys_time;
	af->trig_ops.handle.aft_ops.binfile_is_exist = if_aft_binfile_is_exist;
	af->trig_ops.handle.aft_ops.is_aft_mlog = if_is_aft_mlog;
	af->trig_ops.handle.aft_ops.aft_log = if_aft_log;

	pthread_mutex_lock(&af->caf_work_lock);
	af->trig_ops.init(&aft_in, &af->trig_ops.handle);
	af->trig_ops.ioctrl(&af->trig_ops.handle, AFT_CMD_GET_AE_SKIP_INFO, &af->trig_ops.ae_skip_info, NULL);
	pthread_mutex_unlock(&af->caf_work_lock);
	return 0;
}

static cmr_s32 trigger_deinit(af_ctrl_t * af)
{
	pthread_mutex_lock(&af->caf_work_lock);
	af->trig_ops.deinit(&af->trig_ops.handle);
	pthread_mutex_unlock(&af->caf_work_lock);
	unload_trigger_lib(af);
	pthread_mutex_destroy(&af->caf_work_lock);
	return 0;
}

static cmr_s32 trigger_set_mode(af_ctrl_t * af, enum aft_mode mode)
{
	//cmr_s32 mode;

	//mode = STATE_CAF == af->state ? AFT_MODE_CONTINUE : AFT_MODE_VIDEO;
	pthread_mutex_lock(&af->caf_work_lock);
	af->trig_ops.ioctrl(&af->trig_ops.handle, AFT_CMD_SET_AF_MODE, &mode, NULL);
	pthread_mutex_unlock(&af->caf_work_lock);
	return 0;
}

static cmr_s32 trigger_start(af_ctrl_t * af)
{
	pthread_mutex_lock(&af->caf_work_lock);
	af->trig_ops.ioctrl(&af->trig_ops.handle, AFT_CMD_SET_CAF_RESET, NULL, NULL);
	pthread_mutex_unlock(&af->caf_work_lock);
	return 0;
}

static cmr_s32 trigger_stop(af_ctrl_t * af)
{
	pthread_mutex_lock(&af->caf_work_lock);
	af->trig_ops.ioctrl(&af->trig_ops.handle, AFT_CMD_SET_CAF_STOP, NULL, NULL);
	pthread_mutex_unlock(&af->caf_work_lock);
	return 0;
}

static cmr_s32 trigger_calc(af_ctrl_t * af, struct aft_proc_calc_param *prm, struct aft_proc_result *res)
{
	pthread_mutex_lock(&af->caf_work_lock);
	af->trig_ops.calc(&af->trig_ops.handle, prm, res);
	pthread_mutex_unlock(&af->caf_work_lock);
	return 0;
}

// caf stuffs
static void caf_start_search(af_ctrl_t * af, struct aft_proc_result *p_aft_result)
{
	AF_Trigger_Data aft_in;
	memset(&aft_in,0,sizeof(AF_Trigger_Data));
	aft_in.AFT_mode = af->algo_mode;
	aft_in.bisTrigger = AF_TRIGGER;
	aft_in.AF_Trigger_Type = (p_aft_result->is_need_rough_search)?(RF_NORMAL):(RF_FAST);
	AF_Trigger(&af->fv, &aft_in);
	do_start_af(af);
	if (1 == af->need_re_trigger) {
//      af->need_re_trigger = 0;
		if (af->request_mode != AF_MODE_CONTINUE && af->request_mode != AF_MODE_VIDEO) {
			caf_stop_monitor(af);
		}
	} else {
		notify_start(af);
	}
	af->vcm_stable = 0;
}

//static void caf_stop_search(af_ctrl_t *af, cmr_s32 res)
static void caf_stop_search(af_ctrl_t * af)
{
//    do_stop_af(af);

	pthread_mutex_lock(&af->af_work_lock);
	AF_STOP(&af->fv, af->algo_mode);
	AF_Process_Frame(&af->fv);
	ISP_LOGV("AF_mode = %d", af->fv.AF_mode);
	pthread_mutex_unlock(&af->af_work_lock);

	// let it finish its job
//    AF_Process_Frame(&af->fv);
//    AF_Process_Frame(&af->fv);

//    if (res >= 0)
//        notify_stop(af, res);
}

static void caf_start_monitor(af_ctrl_t * af)
{
	if (af->request_mode == AF_MODE_CONTINUE || af->request_mode == AF_MODE_VIDEO) {
		trigger_start(af);
	}
	do_start_af(af);
}

static void caf_stop_monitor(af_ctrl_t * af)
{
	//do_stop_af(af);
	trigger_stop(af);
}

static void caf_start(af_ctrl_t * af)
{
	enum aft_mode mode;
	ISP_LOGV("state = %s, caf_state = %s", STATE_STRING(af->state), CAF_STATE_STR(af->caf_state));

	if (STATE_RECORD_CAF == af->state)
		af->algo_mode = VAF;
	else
		af->algo_mode = CAF;

	calc_roi(af, NULL, af->algo_mode);

	mode = STATE_CAF == af->state ? AFT_MODE_CONTINUE : AFT_MODE_VIDEO;

	trigger_set_mode(af, mode);

	af->caf_state = CAF_MONITORING;
	caf_start_monitor(af);
}

static void caf_stop(af_ctrl_t * af)
{
	ISP_LOGV("caf_state = %s", CAF_STATE_STR(af->caf_state));

	switch (af->caf_state) {
	case CAF_MONITORING:
		caf_stop_monitor(af);
		break;

	case CAF_SEARCHING:
		//caf_stop_search(af, -1);
		caf_stop_search(af);
		break;

	default:
		break;
	}

	af->caf_state = CAF_IDLE;
	return;
}

static void caf_search_process_af(af_ctrl_t * af)
{
	AF_Process_Frame(&af->fv);

	ISP_LOGV("AF_mode = %d", af->fv.AF_mode);
	if (Wait_Trigger == af->fv.AF_mode) {
		cmr_u8 res;

		AF_Get_SAF_Result(&af->fv, &res);
		ISP_LOGV("Normal AF end, result = %d", res);

		//caf_stop_search(af, HAVE_PEAK == res ? 1 : 0);
		//do_stop_af(af);
		if (1 == af->need_re_trigger)
			af->need_re_trigger = 0;
		if ((STATE_CAF == af->state) || (STATE_RECORD_CAF == af->state)) {
			ISP_LOGV("notify_stop");
			notify_stop(af, res);
			af->caf_state = CAF_MONITORING;
			caf_start_monitor(af);	//need reset trigger
			//do_start_af(af);
		}
		af->caf_first_stable = 0;
	}
}

static void caf_monitor_calc(af_ctrl_t * af, struct aft_proc_calc_param *prm)
{
	struct aft_proc_result res;

	memset(&res, 0, sizeof(res));

	trigger_calc(af, prm, &res);
	ISP_LOGV("is_caf_trig = %d, is_caf_trig = %d, is_need_rough_search = %d", res.is_caf_trig, res.is_caf_trig, res.is_need_rough_search);
	if (res.is_caf_trig || (AFV1_TRUE == af->inited_af_req && CAF_SEARCHING != af->caf_state)) {
		//caf_stop_monitor(af);
		af->caf_state = CAF_SEARCHING;
		caf_start_search(af, &res);
	} else if (res.is_cancel_caf && af->caf_state == CAF_SEARCHING) {
		pthread_mutex_lock(&af->af_work_lock);
		//notify_stop(af, 0);
		af->need_re_trigger = 1;
		AF_STOP(&af->fv, af->algo_mode);
		AF_Process_Frame(&af->fv);
		ISP_LOGV("AF_mode = %d", af->fv.AF_mode);
		af->caf_state = CAF_MONITORING;
		pthread_mutex_unlock(&af->af_work_lock);
		do_start_af(af);
	}
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

static struct aft_proc_calc_param prm_ae;
static void caf_monitor_process_ae(af_ctrl_t * af, const struct ae_calc_out *ae, isp_awb_statistic_hist_info_t * stat)
{
	struct aft_proc_calc_param *prm = &prm_ae;

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
	prm->ae_info.target_lum = 128;
	prm->ae_info.is_stable = ae->is_stab;
	prm->ae_info.bv = af->ae.bv;
	prm->ae_info.y_sum = af->Y_sum_trigger;
	prm->ae_info.cur_scene = af->curr_scene;
	prm->ae_info.registor_pos = lens_get_pos(af);	//if_get_motor_pos(&prm->ae_info.registor_pos, (void *)af);        //gwb 32 to 16
	// ISP_LOGV("exp_time = %d, gain = %d, cur_lum = %d, is_stable = %d",
	//   prm->ae_info.exp_time, prm->ae_info.gain, prm->ae_info.cur_lum, prm->ae_info.is_stable);

	caf_monitor_calc(af, prm);
}

static struct aft_proc_calc_param prm_af;
static void caf_monitor_process_af(af_ctrl_t * af)
{
	cmr_u64 fv[10];
	struct aft_proc_calc_param *prm = &prm_af;

	memset(fv, 0, sizeof(fv));
	memset(prm, 0, sizeof(struct aft_proc_calc_param));

	if (1 != af->af_tuning_data.flag || 1 != af->win_config->win_strategic) {	//default window config
/*
		cmr_s32 i;
		cmr_u64 sum;
*/
		afm_get_fv(af, fv, ENHANCED_BIT, af->roi.num, AF_RING_BUFFER);
/*
		sum = 0;
		for (i=0; i<9; ++i)
		sum += fv[i];
		ISP_LOGV("spsmd %lld, %lld, %lld, %lld, %lld, %lld, %lld, %lld, %lld: %lld",
		fv[0], fv[1], fv[2], fv[3], fv[4], fv[5], fv[6],
		fv[7], fv[8], sum);
		fv[0] = sum;
*/
		//ISP_LOGV("af->roi.num %d spsmd %lld", af->roi.num, fv[af->roi.num - 1]);
	} else {
		afm_get_fv(af, fv, ENHANCED_BIT, af->roi.num, AF_RING_BUFFER);
		fv[0] = fv[af->roi.num - 1];	// the fv in last window is for caf trigger
	}

	prm->afm_info.win_cfg.win_cnt = 1;
	prm->afm_info.win_cfg.win_pos[0].sx = af->roi.win[0].start_x;
	prm->afm_info.win_cfg.win_pos[0].sy = af->roi.win[0].start_y;
	prm->afm_info.win_cfg.win_pos[0].ex = af->roi.win[0].end_x;
	prm->afm_info.win_cfg.win_pos[0].ey = af->roi.win[0].end_y;
	prm->afm_info.filter_info.filter_num = 1;
	prm->afm_info.filter_info.filter_data[0].data = fv;
	prm->afm_info.filter_info.filter_data[0].type = 1;
	prm->active_data_type = AFT_DATA_AF;

	caf_monitor_calc(af, prm);
}

static void caf_process_af(af_ctrl_t * af)
{
	ISP_LOGV("caf_state = %s", CAF_STATE_STR(af->caf_state));
	if(CAF_SEARCHING == af->caf_state){
		pthread_mutex_lock(&af->af_work_lock);
		caf_search_process_af(af);
		//af->caf_state = CAF_MONITORING;
		//caf_start_monitor(af); //reset trigger after caf
		pthread_mutex_unlock(&af->af_work_lock);
	}
	//caf_monitor_process_af(af);
}

static void caf_process_ae(af_ctrl_t * af, const struct ae_calc_out *ae, isp_awb_statistic_hist_info_t * stat)
{
	//pthread_mutex_lock(&af->caf_lock);

	ISP_LOGV("caf_state = %s", CAF_STATE_STR(af->caf_state));

	//if (CAF_MONITORING == af->caf_state)
	{
		caf_monitor_process_ae(af, ae, stat);
	}

	//pthread_mutex_unlock(&af->caf_lock);
}

struct aft_proc_calc_param prm_sensor;
static void caf_process_sensor(af_ctrl_t * af, struct af_aux_sensor_info_t * in) {
    struct af_aux_sensor_info_t *aux_sensor_info = (struct af_aux_sensor_info_t *)in;
    uint32_t  sensor_type = aux_sensor_info->type;
    struct aft_proc_calc_param *prm = &prm_sensor;

    memset(prm, 0, sizeof(struct aft_proc_calc_param));
    switch(sensor_type) {
    case AF_ACCELEROMETER:
            prm->sensor_info.sensor_type = AFT_POSTURE_ACCELEROMETER;
            prm->sensor_info.x = aux_sensor_info->gsensor_info.vertical_down;
            prm->sensor_info.y = aux_sensor_info->gsensor_info.vertical_up;
            prm->sensor_info.z = aux_sensor_info->gsensor_info.horizontal;
            break;
    case AF_GYROSCOPE:
            prm->sensor_info.sensor_type = AFT_POSTURE_GYRO;
            prm->sensor_info.x = aux_sensor_info->gyro_info.x;
            prm->sensor_info.y = aux_sensor_info->gyro_info.y;
            prm->sensor_info.z = aux_sensor_info->gyro_info.z;
            break;
    default:
            break;
    }
    prm->active_data_type = AFT_DATA_SENSOR;
    ISP_LOGV("[%d] sensor type %d %f %f %f "
            ,af->state
            ,prm->sensor_info.sensor_type
            ,prm->sensor_info.x
            ,prm->sensor_info.y
            ,prm->sensor_info.z);
    caf_monitor_calc(af, prm);
}

static void suspend_caf(af_ctrl_t * af)
{
	ISP_LOGV("state = %s, pre_state = %s", STATE_STRING(af->state), STATE_STRING(af->pre_state));
	assert((STATE_CAF == af->state) || (STATE_RECORD_CAF == af->state));

	af->pre_state = af->state;
	af->state = STATE_IDLE;
	caf_stop(af);
}

static void resume_caf(af_ctrl_t * af)
{
	ISP_LOGV("state = %s, pre_state = %s", STATE_STRING(af->state), STATE_STRING(af->pre_state));
	assert((STATE_CAF == af->pre_state) || (STATE_RECORD_CAF == af->pre_state));

	af->state = af->pre_state;
	af->pre_state = STATE_IDLE;
	caf_start(af);
}

// faf stuffs
static cmr_s32 faf_process_frame(af_ctrl_t * af)
{
	AF_Process_Frame(&af->fv);

	// ISP_LOGV("AF_mode = %d", af->fv.AF_mode);
	if (Wait_Trigger == af->fv.AF_mode) {
		cmr_u8 res;

		AF_Get_SAF_Result(&af->fv, &res);
		// ISP_LOGV("Normal AF end, result = %d", res);

		//notify_stop(af, HAVE_PEAK == res ? 1 : 0);
		return 1;
	} else {
		return 0;
	}
}

static void faf_start(af_ctrl_t * af, struct af_trig_info *win)
{
	AF_Trigger_Data aft_in;
	af->algo_mode = CAF;
	memset(&aft_in,0,sizeof(AF_Trigger_Data));
	aft_in.AFT_mode = af->algo_mode;
	aft_in.bisTrigger = AF_TRIGGER;
	aft_in.AF_Trigger_Type = (RF_NORMAL);
	calc_roi(af, win, af->algo_mode);
	AF_Trigger(&af->fv, &aft_in);
	do_start_af(af);
	af->vcm_stable = 0;
}

static cmr_s32 face_dectect_trigger(af_ctrl_t * af)
{

	cmr_u16 i = 0, max_index = 0, trigger = 0;
	cmr_u32 diff_x = 0, diff_y = 0, diff_area = 0;
	cmr_u32 max_area = 0, area = 0;
	isp_info_t *isp_size = &af->isp_info;
	struct isp_face_area *face_info = &af->face_info;
	prime_face_base_info_t *face_base = &af->face_base;
	roi_info_t *roi = &af->roi;

	max_index = face_info->face_num;
	while (i < face_info->face_num) {	// pick face of maximum size
		ISP_LOGV("face_area%d (sx ex sy ey) = (%d %d %d %d) ", i, face_info->face_info[i].sx,
			face_info->face_info[i].ex, face_info->face_info[i].sy, face_info->face_info[i].ey);
		area = (face_info->face_info[i].ex - face_info->face_info[i].sx) * (face_info->face_info[i].ey - face_info->face_info[i].sy);
		if (max_area < area) {
			max_index = i;
			max_area = area;
		}
		i++;
	}

	if (max_index == face_info->face_num)
		return 0;

	diff_x =
	    face_base->ex + face_base->sx >
	    face_info->face_info[max_index].ex + face_info->face_info[max_index].sx ? face_base->ex +
	    face_base->sx - face_info->face_info[max_index].ex -
	    face_info->face_info[max_index].sx : face_info->face_info[max_index].ex + face_info->face_info[max_index].sx - face_base->ex - face_base->sx;
	diff_y =
	    face_base->ey + face_base->sy >
	    face_info->face_info[max_index].ey + face_info->face_info[max_index].sy ? face_base->ey +
	    face_base->sy - face_info->face_info[max_index].ey -
	    face_info->face_info[max_index].sy : face_info->face_info[max_index].ey + face_info->face_info[max_index].sy - face_base->ey - face_base->sy;
	diff_area = face_base->area > max_area ? face_base->area - max_area : max_area - face_base->area;

	if ((face_base->ex + face_base->sx) * face_base->diff_cx_thr < 100 * diff_x &&
	    (face_base->ey + face_base->sy) * face_base->diff_cy_thr < 100 * diff_y && face_base->area * face_base->diff_area_thr < 100 * diff_area) {
		ISP_LOGV("diff_cx diff_cy diff_area = %f %f %f", 1.0 * diff_x / (face_base->ex + face_base->sx),
			1.0 * diff_y / (face_base->ey + face_base->sy), 1.0 * diff_area / face_base->area);
		face_base->sx = face_info->face_info[max_index].sx;	// update base face
		face_base->ex = face_info->face_info[max_index].ex;
		face_base->sy = face_info->face_info[max_index].sy;
		face_base->ey = face_info->face_info[max_index].ey;
		face_base->area = max_area;

		face_base->diff_trigger = 1;
		face_base->converge_cnt = 0;
		if (0 == face_base->area)
			face_base->converge_cnt = face_base->converge_cnt_thr + 1;
	} else {
		if (1 == face_base->diff_trigger)
			face_base->converge_cnt++;
		else
			face_base->converge_cnt = 0;
	}

	if (1 == face_base->diff_trigger && face_base->converge_cnt > face_base->converge_cnt_thr) {
		face_base->diff_trigger = 0;
		face_base->converge_cnt = 0;
		trigger = 1;
	}

	return trigger;
}

static cmr_s32 faf_trigger_init(af_ctrl_t * af)
{				// all thrs are in percentage unit
	af->face_base.diff_area_thr = 40;
	af->face_base.diff_cx_thr = 80;
	af->face_base.diff_cy_thr = 80;
	af->face_base.converge_cnt_thr = 15;

	af->face_base.sx = 0;	// update base face
	af->face_base.ex = 0;
	af->face_base.sy = 0;
	af->face_base.ey = 0;
	af->face_base.area = 0;

	af->face_base.diff_trigger = 0;
	af->face_base.converge_cnt = 0;

	return 0;
}

// misc
static cmr_u64 get_systemtime_ns()
{
	cmr_s64 timestamp = systemTime(CLOCK_MONOTONIC);
	return timestamp;
}

static cmr_u32 Is_ae_stable(af_ctrl_t * af)
{
/*
	if (af->request_mode != AF_MODE_CONTINUE && af->request_mode != AF_MODE_VIDEO) {	// non caf mode
		return af->ae.stable;
	} else {
		return 1;
	}
*/
#if 0				//Andrew : The control layer does not interfere with the CAF mechanism
	if ((STATE_CAF == af->state || STATE_RECORD_CAF == af->state) && 0 == af->caf_first_stable && 1 == af->ae.stable) {
		af->caf_first_stable = 1;
		return 1;
	}

	if (1 == af->caf_first_stable)
		return 1;
#else
	if (STATE_CAF == af->state || STATE_RECORD_CAF == af->state) {
		return 1;
	}
#endif
	return af->ae.stable;
}

static void get_isp_size(af_ctrl_t * af, cmr_u16 * widith, cmr_u16 * height)
{

//      *widith = isp->input_size_trim[isp->param_index].width;
//      *height = isp->input_size_trim[isp->param_index].height;
	*widith = af->isp_info.width;
	*height = af->isp_info.height;

}

static void ae_calibration(af_ctrl_t * af, struct isp_awb_statistic_info *rgb)
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

static void set_af_RGBY(af_ctrl_t * af, struct isp_awb_statistic_info *rgb)
{
#define AE_BLOCK_W 32
#define AE_BLOCK_H 32

	cmr_u32 Y_sx = 0, Y_ex = 0, Y_sy = 0, Y_ey = 0, r_sum = 0, g_sum = 0, b_sum = 0, y_sum = 0;
	float af_area, ae_area;
	cmr_u16 width, height, i = 0, blockw, blockh, index;

	get_isp_size(af, &width, &height);
	af->roi_RGBY.num = af->roi.num;

	af->roi_RGBY.Y_sum[af->roi.num] = 0;
	for (i = 0; i < af->roi.num; i++) {
		Y_sx = af->roi.win[i].start_x / (width / AE_BLOCK_W);
		Y_ex = af->roi.win[i].end_x / (width / AE_BLOCK_W);
		Y_sy = af->roi.win[i].start_y / (height / AE_BLOCK_H);
		Y_ey = af->roi.win[i].end_y / (height / AE_BLOCK_H);
		//exception
		if (Y_sx == Y_ex)
			Y_ex = Y_sx + 1;
		//exception
		if (Y_sy == Y_ey)
			Y_ey = Y_sy + 1;

		r_sum = 0;
		g_sum = 0;
		b_sum = 0;
		for (blockw = Y_sx; blockw < Y_ex; blockw++) {
			for (blockh = Y_sy; blockh < Y_ey; blockh++) {
				index = blockw * AE_BLOCK_W + blockh;
				r_sum = r_sum + rgb->r_info[index];
				g_sum = g_sum + rgb->g_info[index];
				b_sum = b_sum + rgb->b_info[index];
			}
		}

		//af_area =  (af->roi.win[i].end_x - af->roi.win[i].start_x + 1) * (af->roi.win[i].end_y - af->roi.win[i].start_y + 1) ;
		ae_area = 1.0 * ((Y_ex - Y_sx) /** (width/AE_BLOCK_W)*/ ) *
		    ((Y_ey - Y_sy) /* * (height/AE_BLOCK_H) */ );
		y_sum = (((0.299 * r_sum) + (0.587 * g_sum) + (0.114 * b_sum)) / ae_area);
		af->roi_RGBY.Y_sum[i] = y_sum;
		af->roi_RGBY.R_sum[i] = r_sum;
		af->roi_RGBY.G_sum[i] = g_sum;
		af->roi_RGBY.B_sum[i] = b_sum;
		af->roi_RGBY.Y_sum[af->roi.num] += y_sum;
		//ISP_LOGV("y_sum[%d] = %d",i,y_sum);
	}

	switch (af->state) {
	case STATE_FAF:
		af->Y_sum_trigger = af->roi_RGBY.Y_sum[af->roi.num];
		af->Y_sum_normalize = af->roi_RGBY.Y_sum[af->roi.num];
		break;
	default:
		af->Y_sum_trigger = af->roi_RGBY.Y_sum[1];
		af->Y_sum_normalize = af->roi_RGBY.Y_sum[1];
		break;
	}

	property_get("af_mode", AF_MODE, "none");
	if (0 != strcmp(AF_MODE, "none")) {
		ae_calibration(af, rgb);
	}

}

static cmr_u32 calc_gain_index(cmr_u32 cur_gain)
{
	cmr_u32 gain = cur_gain >> 7, i = 0;	// 128 is 1 ae gain unit

	if (gain > (1 << GAIN_32x))
		gain = (1 << GAIN_32x);

	i = 0;
	while ((gain = (gain >> 1))) {	//gwb suggest parentheses around assignment used as truth value [-Wparentheses]
		i++;
	}
	return i;
}

static void set_ae_info(af_ctrl_t * af, const struct ae_calc_out *ae, cmr_s32 bv)
{
	ae_info_t *p = &af->ae;

	//ISP_LOGV("state = %s, bv = %d", STATE_STRING(af->state), bv);

	p->stable = ae->is_stab;
	p->bv = bv;
	p->exp = ae->cur_exp_line * ae->line_time;
	p->gain = ae->cur_again;
	p->gain_index = calc_gain_index(ae->cur_again);
	p->ae = *ae;

	if (bv >= af->bv_threshold[af->pre_scene][OUT_SCENE]) {
		af->curr_scene = OUT_SCENE;
	} else if (bv >= af->bv_threshold[af->pre_scene][INDOOR_SCENE]) {
		af->curr_scene = INDOOR_SCENE;
	} else {
		af->curr_scene = DARK_SCENE;
	}

	af->pre_scene = af->curr_scene;

	if (1 == af->af_tuning_data.flag) {	// dynamicly adjust gain in different scene
		memcpy(&af->fv.AF_Tuning_Data, &af->af_tuning_data.AF_Tuning_Data[af->curr_scene], sizeof(af->fv.AF_Tuning_Data));
	}

}

static void set_awb_info(af_ctrl_t * af, const struct awb_ctrl_calc_result *awb, const struct awb_gain *gain)
{
	UNUSED(awb);
	af->awb.r_gain = gain->r;
	af->awb.g_gain = gain->g;
	af->awb.b_gain = gain->b;
}

// i/f to AF model
static ERRCODE if_af_start_notify(eAF_MODE AF_mode, void *cookie)
{
	UNUSED(AF_mode);
	UNUSED(cookie);
	return 0;
}

static ERRCODE if_af_end_notify(eAF_MODE AF_mode, void *cookie)
{
	UNUSED(AF_mode);
	UNUSED(cookie);
	return 0;
}

static ERRCODE if_statistics_wait_cal_done(void *cookie)
{
	UNUSED(cookie);
	return 0;
}

static ERRCODE if_statistics_get_data(uint64 fv[T_TOTAL_FILTER_TYPE], _af_stat_data_t * p_stat_data, void *cookie)
{
	af_ctrl_t *af = cookie;
	//isp_ctrl_context *isp = af->isp_ctx;
	cmr_u64 spsmd[MAX_ROI_NUM];
	cmr_u32 i;
	memset(fv, 0, sizeof(fv[0]) * T_TOTAL_FILTER_TYPE);

	if (1 != af->af_tuning_data.flag || 1 != af->win_config->win_strategic) {	//default window config
		//cmr_s32 i;
		cmr_u64 sum;

		sum = 0;
		memset(&(spsmd[0]), 0, sizeof(cmr_u64) * MAX_ROI_NUM);
		afm_get_fv(af, spsmd, ENHANCED_BIT, af->roi.num, AF_RING_BUFFER);

		//sum = 0.2*spsmd[0]+spsmd[1]+3*spsmd[2];
		switch (af->state) {
		case STATE_FAF:
			sum = spsmd[0] + spsmd[1] + spsmd[2] + spsmd[3] + 8 * spsmd[4] + spsmd[5] + spsmd[6] + spsmd[7] + spsmd[8];
			fv[T_SPSMD] = sum;
			break;
		default:
			sum = spsmd[9];	//3///3x3 windows,the 9th window is biggest covering all the other window
#if 0
			sum = spsmd[1] + 8 * spsmd[2];	/// the 0th window cover 1st and 2nd window,1st window cover 2nd window
#endif
			fv[T_SPSMD] = sum;
			break;
		}

		if (p_stat_data) {	//for caf calc
			p_stat_data->roi_num = af->roi.num;
			p_stat_data->stat_num = FOCUS_STAT_DATA_NUM;
			/*
			   ISP_LOGV("Copy data struct %lld / %lld / %lld %lld / %lld / %lld "
			   ,af->af_fv_val.af_fv0[0],af->af_fv_val.af_fv0[1],af->af_fv_val.af_fv0[2]
			   ,af->af_fv_val.af_fv1[0],af->af_fv_val.af_fv1[1],af->af_fv_val.af_fv1[2]);

			   for (i = 0; i < p_stat_data->roi_num; i++) {
			   ISP_LOGV("fv0[%d]:%ld,", i, af->af_fv_val.af_fv0[i]);
			   }

			   for (i = 0; i < p_stat_data->roi_num; i++) {
			   ISP_LOGV("fv1[%d]:%ld,", i, af->af_fv_val.af_fv1[i]);
			   }
			 */
			p_stat_data->p_stat = &(af->af_fv_val.af_fv0[0]);
		}
		ISP_LOGV("[%d][%d]spsmd sum %lld", af->state, af->roi.num, sum);
		//_LOGD("fv[T_SPSMD] %lld", fv[T_SPSMD]);
	} else {
		cmr_u32 i;
		cmr_u64 sum;

		afm_get_fv(af, spsmd, ENHANCED_BIT, af->roi.num, AF_RING_BUFFER);

		sum = 0;
		for (i = 0; i < af->roi.num; ++i)	// for caf, the weight in last window is 0
			sum += spsmd[i] * af->win_config->win_weight[i];
		fv[T_SPSMD] = sum;
		ISP_LOGV("spsmd sum %lld", sum);
	}

	return 0;
}

static ERRCODE if_statistics_set_data(cmr_u32 set_stat, void *cookie)
{
	af_ctrl_t *af = cookie;

	fv0_e = (set_stat & 0x0f);
	fv1_e = (set_stat & 0xf0) >> 4;
	nr_mode = (set_stat & 0xff00) >> 8;
	cw_mode = (set_stat & 0xff0000) >> 16;
	iir_level = (set_stat & 0xff000000) >> 24;

	ISP_LOGI("[0x%x] fv0e %d, fv1e %d, nr %d, cw %d iir %d", set_stat, fv0_e, fv1_e, nr_mode, cw_mode, iir_level);
	afm_setup(af);
	return 0;
}

static ERRCODE if_phase_detection_get_data(pd_algo_result_t * pd_result, void *cookie)
{
	af_ctrl_t *af = cookie;

	memcpy(pd_result, &(af->pd), sizeof(pd_algo_result_t));
	pd_result->pd_roi_dcc = 17;

	return 0;
}

static ERRCODE if_motion_sensor_get_data(motion_sensor_result_t * ms_result, void *cookie)
{
	af_ctrl_t *af = cookie;

	if(NULL == ms_result){
		return 1;
	}

	ms_result->g_sensor_queue[SENSOR_X_AXIS][ms_result->sensor_g_queue_cnt] = af->gsensor_info.vertical_up;
	ms_result->g_sensor_queue[SENSOR_Y_AXIS][ms_result->sensor_g_queue_cnt] = af->gsensor_info.vertical_down;
	ms_result->g_sensor_queue[SENSOR_Z_AXIS][ms_result->sensor_g_queue_cnt] = af->gsensor_info.horizontal;
	ms_result->timestamp = af->gsensor_info.timestamp;

	return 0;
}

static ERRCODE if_lens_get_pos(uint16 * pos, void *cookie)
{
	af_ctrl_t *af = cookie;
	*pos = lens_get_pos(af);
	return 0;
}

static ERRCODE if_lens_move_to(uint16 pos, void *cookie)
{
	af_ctrl_t *af = cookie;

	lens_move_to(af, pos);
	return 0;
}

static ERRCODE if_lens_wait_stop(void *cookie)
{
	UNUSED(cookie);
	return 0;
}

static ERRCODE if_lock_ae(e_LOCK lock, void *cookie)
{
	af_ctrl_t *af = cookie;
	ISP_LOGV("%s, lock_num = %d", LOCK == lock ? "lock" : "unlock", af->ae_lock_num);

	if (LOCK == lock) {
		if (0 == af->ae_lock_num) {
			af->lock_module(af->caller, AF_LOCKER_AE);
			af->ae_lock_num++;
		}
	} else {
		if (af->ae_lock_num) {
			af->unlock_module(af->caller, AF_LOCKER_AE);
			af->ae_lock_num--;
		}
	}

	return 0;
}

static ERRCODE if_lock_awb(e_LOCK lock, void *cookie)
{
	af_ctrl_t *af = cookie;
	ISP_LOGV("%s, lock_num = %d", LOCK == lock ? "lock" : "unlock", af->awb_lock_num);

	if (LOCK == lock) {
		if (0 == af->awb_lock_num) {
			af->lock_module(af->caller, AF_LOCKER_AWB);
			af->awb_lock_num++;
		}
	} else {
		if (af->awb_lock_num) {
			af->unlock_module(af->caller, AF_LOCKER_AWB);
			af->awb_lock_num--;
		}
	}

	return 0;
}

static ERRCODE if_lock_lsc(e_LOCK lock, void *cookie)
{
	af_ctrl_t *af = cookie;
	ISP_LOGV("%s, lock_num = %d", LOCK == lock ? "lock" : "unlock", af->lsc_lock_num);

	if (LOCK == lock) {
		if (0 == af->lsc_lock_num) {
			af->lock_module(af->caller, AF_LOCKER_LSC);
			af->lsc_lock_num++;
		}
	} else {
		if (af->lsc_lock_num) {
			af->unlock_module(af->caller, AF_LOCKER_LSC);
			af->lsc_lock_num--;
		}
	}

	return 0;
}

static ERRCODE if_lock_nlm(e_LOCK lock, void *cookie)
{
	af_ctrl_t *af = cookie;
	ISP_LOGV("%s, lock_num = %d", LOCK == lock ? "lock" : "unlock", af->nlm_lock_num);

	if (LOCK == lock) {
		if (0 == af->nlm_lock_num) {
			af->lock_module(af->caller, AF_LOCKER_NLM);
			af->nlm_lock_num++;
		}
	} else {
		if (af->nlm_lock_num) {
			af->unlock_module(af->caller, AF_LOCKER_NLM);
			af->nlm_lock_num--;
		}
	}

	return 0;
}

static ERRCODE if_get_sys_time(uint64 * time, void *cookie)
{
	UNUSED(cookie);
	*time = get_systemtime_ns();
	return 0;
}

static ERRCODE if_sys_sleep_time(uint16 sleep_time, void *cookie)
{
	af_ctrl_t *af = (af_ctrl_t *) cookie;

	af->vcm_timestamp = get_systemtime_ns();
	//ISP_LOGV("vcm_timestamp %lld ms", (cmr_s64) af->vcm_timestamp);
	usleep(sleep_time * 1000);
	return 0;
}

static ERRCODE if_get_ae_report(AE_Report * rpt, void *cookie)
{
	af_ctrl_t *af = (af_ctrl_t *) cookie;
	ae_info_t *ae = &af->ae;

	rpt->bAEisConverge = ae->stable;
	rpt->AE_BV = ae->bv;
	rpt->AE_EXP = ae->exp / 10000;	// 0.1us -> ms
	rpt->AE_Gain = ae->gain;
	rpt->AE_Pixel_Sum = af->Y_sum_normalize;
	return 0;
}

static ERRCODE if_set_af_exif(const void *data, void *cookie)
{
	// TODO
	UNUSED(data);
	UNUSED(cookie);
	return 0;
}

static ERRCODE if_get_otp(AF_OTP_Data * pAF_OTP, void *cookie)
{
	// TODO
	af_ctrl_t *af = cookie;
	//isp_ctrl_context *isp = af->isp_ctx;
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)af->caller;
	struct isp_alg_fw_context *isp_ctx = (struct isp_alg_fw_context *)cxt_ptr->caller_handle;

	// get otp
	if (NULL != af->vcm_ops.get_otp) {
		//af->vcm_ops.get_otp(af->caller, &pAF_OTP->INF, &pAF_OTP->MACRO);
		af->vcm_ops.get_otp(isp_ctx->ioctrl_ptr->caller_handler, &pAF_OTP->INF, &pAF_OTP->MACRO);
		af->fv.AF_OTP.bIsExist = T_LENS_BY_OTP;
		ISP_LOGV("otp (infi,macro) = (%d,%d)", pAF_OTP->INF, pAF_OTP->MACRO);
	}

	if (isp_ctx->otp_data) {
		if (isp_ctx->otp_data->single_otp.af_info.macro_cali > isp_ctx->otp_data->single_otp.af_info.infinite_cali) {
			pAF_OTP->bIsExist = (T_LENS_BY_OTP);
			pAF_OTP->INF = isp_ctx->otp_data->single_otp.af_info.infinite_cali;
			pAF_OTP->MACRO = isp_ctx->otp_data->single_otp.af_info.macro_cali;
			ISP_LOGV("get otp (infi,macro) = (%d,%d)", pAF_OTP->INF, pAF_OTP->MACRO);
		} else {
			ISP_LOGV("skip invalid otp (infi,macro) = (%d,%d)", isp_ctx->otp_data->single_otp.af_info.infinite_cali, isp_ctx->otp_data->single_otp.af_info.macro_cali);
		}
	}

	return 0;
}

static ERRCODE if_get_motor_pos(cmr_u16 * motor_pos, void *cookie)
{
	// TODO
	af_ctrl_t *af = cookie;
	//isp_ctrl_context *isp = af->isp_ctx;
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)af->caller;
	struct isp_alg_fw_context *isp_ctx = (struct isp_alg_fw_context *)cxt_ptr->caller_handle;
	// read
	if (NULL != af->vcm_ops.get_motor_pos) {
		//af->vcm_ops.get_motor_pos(af->caller, motor_pos);
		af->vcm_ops.get_motor_pos(isp_ctx->ioctrl_ptr->caller_handler, motor_pos);
		ISP_LOGV("motor pos in register %d", *motor_pos);
	}

	return 0;
}

static ERRCODE if_set_motor_sacmode(void *cookie)
{
	// TODO
	af_ctrl_t *af = cookie;
	//isp_ctrl_context *isp = af->isp_ctx;
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)af->caller;
	struct isp_alg_fw_context *isp_ctx = (struct isp_alg_fw_context *)cxt_ptr->caller_handle;

	if (NULL != af->vcm_ops.set_motor_bestmode)
		//af->vcm_ops.set_motor_bestmode(af->caller);
		af->vcm_ops.set_motor_bestmode(isp_ctx->ioctrl_ptr->caller_handler);

	return 0;
}

static ERRCODE if_binfile_is_exist(uint8 * bisExist, void *cookie)
{
	// TODO
	af_ctrl_t *af = cookie;
	cmr_s32 rtn = AFV1_SUCCESS;

	char *af_tuning_path = "/data/misc/cameraserver/af_tuning.bin";
	FILE *fp = NULL;
	ISP_LOGV("B");
	if (0 == access(af_tuning_path, R_OK)) {	//read request successs
		cmr_u32 len = 0;

		fp = fopen(af_tuning_path, "rb");
		if (NULL == fp) {
			*bisExist = 0;
			return 0;
		}

		fseek(fp, 0, SEEK_END);
		len = ftell(fp);
		if (sizeof(af->af_tuning_data) != len) {
			ISP_LOGV("af_tuning.bin len dismatch with af_alg len %d", sizeof(af->af_tuning_data));
			fclose(fp);
			*bisExist = 0;
			return 0;
		}

		fseek(fp, 0, SEEK_SET);
		rtn = fread(&af->af_tuning_data, 1, len, fp);
		if (rtn != sizeof(af->af_tuning_data)) {
			ISP_LOGV("read bin size error");
			fclose(fp);
			*bisExist = 0;
			return 0;
		}
		fclose(fp);

		if (0 == af->af_tuning_data.flag) {
			ISP_LOGV("skip af_tuning");
			*bisExist = 0;
		} else {
			af->soft_landing_dly = af->af_tuning_data.soft_landing_dly;
			af->soft_landing_step = af->af_tuning_data.soft_landing_step;
			memcpy(&af->filter_clip[0], &af->af_tuning_data.filter_clip[0], sizeof(af->af_tuning_data.filter_clip));
			memcpy(&af->bv_threshold[0], &af->af_tuning_data.bv_threshold[0], sizeof(af->af_tuning_data.bv_threshold));
			af->fv.AF_OTP.bIsExist = T_LENS_BY_TUNING;
			memcpy(&af->fv.AF_Tuning_Data, &af->af_tuning_data.AF_Tuning_Data[INDOOR_SCENE], sizeof(af->fv.AF_Tuning_Data));
			af->pre_scene = INDOOR_SCENE;
			ISP_LOGV("load af_tuning succeed");
			*bisExist = 1;
		}
	} else {
		if (T_LENS_BY_TUNING == af->fv.AF_OTP.bIsExist || 1 == af->af_tuning_data.flag) {	// tuning form sensor file
			*bisExist = 1;
		} else {
			*bisExist = 0;
		}
	}

	{			//andrew: how to use OTP data to optimize scan table
		char value[PROPERTY_VALUE_MAX] = { '\0' };

		property_get("af_set_opt_value", value, "none");	//infi~macro
		if (0 != strcmp(value, "none")) {
			char *p1 = value;
			af->fv.AF_OTP.bIsExist = T_LENS_BY_OTP;
			while (*p1 != '~' && *p1 != '\0')
				p1++;
			*p1++ = '\0';
			af->fv.AF_OTP.INF = atoi(value);
			af->fv.AF_OTP.MACRO = atoi(p1);
			ISP_LOGV("adb AF OPT succeed (INFI MACRO)=(%d %d)", af->fv.AF_OTP.INF, af->fv.AF_OTP.MACRO);
		}
	}
	ISP_LOGV("E");
	return 0;
}

static ERRCODE if_get_vcm_param(cmr_u32 *param, void *cookie)
{
	// TODO
	af_ctrl_t *af = cookie;

	// get otp
	if (NULL != af && NULL != param) {
		param[0] = af->af_tuning_data.vcm_hysteresis;
		ISP_LOGV("vcm_hysteresis = (%d)", af->af_tuning_data.vcm_hysteresis);
	}

	return 0;
}
static char AFlog_buffer[2048] = { 0 };

static ERRCODE if_af_log(const char *format, ...)
{
//      char buffer[2048]={0};
	va_list arg;
	va_start(arg, format);
	vsnprintf(AFlog_buffer, 2048, format, arg);
	va_end(arg);
	ISP_LOGI("ISP_AFv1: %s", AFlog_buffer);

	return 0;
}

static ERRCODE if_aft_log(cmr_u32 log_level, const char *format, ...)
{
//      char buffer[2048]={0};
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

static ERRCODE if_aft_binfile_is_exist(uint8 * is_exist, void *cookie)
{

	af_ctrl_t *af = cookie;
	char *aft_tuning_path = "/data/misc/cameraserver/aft_tuning.bin";
	FILE *fp = NULL;

	if (0 == access(aft_tuning_path, R_OK)) {	//read request successs
		cmr_u16 len = 0;

		fp = fopen(aft_tuning_path, "rb");
		if (NULL == fp) {
			*is_exist = 0;
			return 0;
		}

		fseek(fp, 0, SEEK_END);
		len = ftell(fp);
		if (len != af->trig_ops.handle.tuning_param_len) {
			ISP_LOGV("aft_tuning.bin len dismatch with aft_alg len %d", af->trig_ops.handle.tuning_param_len);
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

static ERRCODE if_is_aft_mlog(cmr_u32 * is_save, void *cookie)
{
	UNUSED(cookie);
#ifndef WIN32
	char value[PROPERTY_VALUE_MAX] = { '\0' };

	property_get(AF_SAVE_MLOG_STR, value, "no");

	if (!strcmp(value, "save")) {
		*is_save = 1;
	}
#endif
	ISP_LOGV("is_save %d", *is_save);
	return 0;
}

/* interface */

/* initialization */
static void load_settings(af_ctrl_t * af, struct isp_pm_ioctl_output *af_pm_output)
{
	// TODO: load from tuning parameters
	//tuning data from common_mode
	cmr_u8 i = 0;
	for (i = 0; i < GAIN_TOTAL; i++) {
		af->filter_clip[OUT_SCENE][i].spsmd_max = 0xFFFF;
		af->filter_clip[OUT_SCENE][i].spsmd_min = 0x0000;
		af->filter_clip[OUT_SCENE][i].sobel_max = 0xFFFF;
		af->filter_clip[OUT_SCENE][i].sobel_min = 0x0000;
		af->filter_clip[INDOOR_SCENE][i].spsmd_max = 0xFFFF;
		af->filter_clip[INDOOR_SCENE][i].spsmd_min = 0x14;
		af->filter_clip[INDOOR_SCENE][i].sobel_max = 0x008F;
		af->filter_clip[INDOOR_SCENE][i].sobel_min = 0x0070;
		af->filter_clip[DARK_SCENE][i].spsmd_max = 0xFFFF;
		af->filter_clip[DARK_SCENE][i].spsmd_min = 0x000C;
		af->filter_clip[DARK_SCENE][i].sobel_max = 0xFFFF;
		af->filter_clip[DARK_SCENE][i].sobel_min = 0x0004;
	}

	af->bv_threshold[OUT_SCENE][OUT_SCENE] = 151 - 10;
	af->bv_threshold[OUT_SCENE][INDOOR_SCENE] = 55;
	af->bv_threshold[OUT_SCENE][DARK_SCENE] = 0;

	af->bv_threshold[INDOOR_SCENE][OUT_SCENE] = 151 + 10;
	af->bv_threshold[INDOOR_SCENE][INDOOR_SCENE] = 55 - 10;
	af->bv_threshold[INDOOR_SCENE][DARK_SCENE] = 0;

	af->bv_threshold[DARK_SCENE][OUT_SCENE] = 151;
	af->bv_threshold[DARK_SCENE][INDOOR_SCENE] = 55 + 10;
	af->bv_threshold[DARK_SCENE][DARK_SCENE] = 0;

	af->soft_landing_dly = 0;	//10;  //avoid vcm crash
	af->soft_landing_step = 0;	//20;

	if (PNULL == af_pm_output->param_data) {
		ISP_LOGV("sensor tuning param null");
	} else if (PNULL == af_pm_output->param_data[0].data_ptr) {
		ISP_LOGV("sensor tuning param data null");
	} else if (af_pm_output->param_data[0].data_size != sizeof(af->af_tuning_data)) {
		ISP_LOGV("sensor tuning param size dismatch");
	} else {
		memcpy(&af->af_tuning_data, af_pm_output->param_data[0].data_ptr, sizeof(af->af_tuning_data));
		ISP_LOGV("sensor tuning param size match");
		if (1 == af->af_tuning_data.flag) {
			af->soft_landing_dly = af->af_tuning_data.soft_landing_dly;
			af->soft_landing_step = af->af_tuning_data.soft_landing_step;
			memcpy(af->filter_clip, af->af_tuning_data.filter_clip, sizeof(af->filter_clip));
			memcpy(af->bv_threshold, af->af_tuning_data.bv_threshold, sizeof(af->bv_threshold));
			memcpy(&af->fv.AF_Tuning_Data, &af->af_tuning_data.AF_Tuning_Data[INDOOR_SCENE], sizeof(af->fv.AF_Tuning_Data));
			af->fv.AF_OTP.bIsExist = T_LENS_BY_TUNING;
			ISP_LOGV("sensor tuning param take effect");
		} else {
			ISP_LOGV("sensor tuning param take no effect");
		}
	}
	af->soft_landing_dly = 0;	//10;  //avoid vcm crash
	af->soft_landing_step = 0;	//20;
	af->pre_scene = INDOOR_SCENE;

}

///
static ERRCODE af_wait_caf_finish(af_ctrl_t * af)
{
	cmr_s32 rtn;
	struct timespec ts;

	if (clock_gettime(CLOCK_REALTIME, &ts)) {
		rtn = -1;
	} else {
		ts.tv_sec += AF_WAIT_CAF_FINISH;
		ts.tv_nsec += 200 * 1000 * 1000;	//1s == (1000 * 1000 * 1000)

/*
	if (ts.tv_nsec + ISP_PROCESS_NSEC_TIMEOUT >= (1000 * 1000 * 1000)) {
		ts.tv_nsec = ts.tv_nsec + ISP_PROCESS_NSEC_TIMEOUT - (1000 * 1000 * 1000);
		ts.tv_sec ++;
	} else {
		ts.tv_nsec += ISP_PROCESS_NSEC_TIMEOUT;
	}
*/
		rtn = sem_timedwait(&af->af_wait_caf, &ts);
		if (rtn) {
			af->takePicture_timeout = 1;
			ISP_LOGV("af wait caf timeout");
		} else {
			af->takePicture_timeout = 2;
			ISP_LOGV("af wait caf finished");
		}
	}
	return 0;
}

static ERRCODE af_clear_sem(af_ctrl_t * af)
{
	cmr_s32 tmpVal = 0;

	pthread_mutex_lock(&af->af_work_lock);
	sem_getvalue(&af->af_wait_caf, &tmpVal);
	while (0 < tmpVal) {
		sem_wait(&af->af_wait_caf);
		sem_getvalue(&af->af_wait_caf, &tmpVal);
	}
	pthread_mutex_unlock(&af->af_work_lock);

	return 0;
}

static cmr_u16 get_vcm_registor_pos(af_ctrl_t * af)
{
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)af->caller;
	struct isp_alg_fw_context *isp_ctx = (struct isp_alg_fw_context *)cxt_ptr->caller_handle;
	cmr_u16 pos = 0;

	if (NULL != af->vcm_ops.get_motor_pos) {
		//af->vcm_ops.get_motor_pos(af->caller, &pos);
		af->vcm_ops.get_motor_pos(isp_ctx->ioctrl_ptr->caller_handler, &pos);
		af->fv.vcm_register = pos;
		ISP_LOGV("VCM registor pos :%d", af->fv.vcm_register);
	}

	return pos;
}

typedef enum _lock_block {
	LOCK_AE = 0x01,
	LOCK_LSC = 0x02,
	LOCK_NLM = 0x04
} lock_block_t;
/*
struct sensor_ex_exposure {
	cmr_u32 exposure;
	cmr_u32 dummy;
	cmr_u32 size_index;
};
*/
static void get_vcm_mode(af_ctrl_t * af, char *vcm_mode)
{
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)af->caller;
	struct isp_alg_fw_context *isp_ctx = (struct isp_alg_fw_context *)cxt_ptr->caller_handle;
	UNUSED(vcm_mode);
	if (NULL != af->vcm_ops.get_test_vcm_mode)
		//af->vcm_ops.get_test_vcm_mode(af->caller);
		af->vcm_ops.get_test_vcm_mode(isp_ctx->ioctrl_ptr->caller_handler);

	return;
}

static void set_vcm_mode(af_ctrl_t * af, char *vcm_mode)
{
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)af->caller;
	struct isp_alg_fw_context *isp_ctx = (struct isp_alg_fw_context *)cxt_ptr->caller_handle;

	if (NULL != af->vcm_ops.set_test_vcm_mode)
		//af->vcm_ops.set_test_vcm_mode(af->caller, vcm_mode);
		af->vcm_ops.set_test_vcm_mode(isp_ctx->ioctrl_ptr->caller_handler, vcm_mode);

	return;
}

static void lock_block(af_ctrl_t * af, char *block)
{
	cmr_u32 lock = 0;

	lock = atoi(block);

	if (lock & LOCK_AE)
		if_lock_ae(LOCK, af);
	if (lock & LOCK_LSC)
		if_lock_lsc(LOCK, af);
	if (lock & LOCK_NLM)
		if_lock_nlm(LOCK, af);

	return;
}

static void set_ae_gain_exp(af_ctrl_t * af, char *ae_param)
{
	UNUSED(af);
	UNUSED(ae_param);
#if 0				//gwb
	char *p1 = ae_param;
	isp_ctrl_context *isp = (isp_ctrl_context *) af->isp_ctx;
	struct sensor_ex_exposure ex_exposure;

	while (*p1 != '~' && *p1 != '\0')
		p1++;
	*p1++ = '\0';

	isp->ioctrl_ptr->set_gain(atoi(ae_param));
	ae_param = p1;
	ex_exposure.exposure = atoi(ae_param);
	ex_exposure.dummy = 0;
	ex_exposure.size_index = 0;
	isp->ioctrl_ptr->ex_set_exposure(&ex_exposure);
#endif
	return;
}

static void set_manual(af_ctrl_t * af, char *test_param)
{
	UNUSED(test_param);
	af->state = STATE_IDLE;
	af->caf_state = CAF_IDLE;
	//property_set("af_set_pos","0");// to fix lens to position 0
	trigger_stop(af);

	ISP_LOGV("Now is in ISP_FOCUS_MANUAL mode");
	ISP_LOGV("pls adb shell setprop \"af_set_pos\" 0~1023 to fix lens position");
}

static void trigger_caf(af_ctrl_t * af, char *test_param)
{
	AF_Trigger_Data aft_in;

	char *p1 = test_param;
	char *p2;
	char *p3;
	while (*p1 != '~' && *p1 != '\0')
        p1++;
	*p1++ = '\0';
	p2 = p1;
	while (*p2 != '~' && *p2 != '\0')
        p2++;
	*p2++ = '\0';
	p3 = p2;
	while (*p3 != '~' && *p3 != '\0')
        p3++;
	*p3++ = '\0';
	memset(&aft_in,0,sizeof(AF_Trigger_Data));
	af->request_mode = AF_MODE_NORMAL;	//not need trigger to work when caf_start_monitor
	af->state = STATE_CAF;
	af->caf_state = CAF_SEARCHING;
	af->algo_mode = CAF;
	aft_in.AFT_mode = af->algo_mode;
	aft_in.bisTrigger = AF_TRIGGER;
	aft_in.AF_Trigger_Type = atoi(test_param);
	aft_in.defocus_param.scan_from = (atoi(p1)>0 && atoi(p1)<1023)? (atoi(p1)):(0);
	aft_in.defocus_param.scan_to = (atoi(p2)>0 && atoi(p2)<1023)? (atoi(p2)):(0);
	aft_in.defocus_param.per_steps = (atoi(p3)>0 && atoi(p3)<200)? (atoi(p3)):(0);

	trigger_stop(af);
	AF_Trigger(&af->fv, &aft_in);	//test_param is in _eAF_Triger_Type,     RF_NORMAL = 0,        //noraml R/F search for AFT RF_FAST = 3,              //Fast R/F search for AFT
	do_start_af(af);
}

static void trigger_saf(af_ctrl_t * af, char *test_param)
{
	AF_Trigger_Data aft_in;
	memset(&aft_in,0,sizeof(AF_Trigger_Data));
	UNUSED(test_param);
	af->request_mode = AF_MODE_NORMAL;
	af->state = STATE_NORMAL_AF;
	af->caf_state = CAF_IDLE;
	//af->defocus = (1 == atoi(test_param))? (1):(af->defocus);
	//saf_start(af, NULL);  //SAF, win is NULL using default
	af->algo_mode = SAF;
	aft_in.AFT_mode = af->algo_mode;
	aft_in.bisTrigger = AF_TRIGGER;
	aft_in.AF_Trigger_Type = (1==af->defocus)? DEFOCUS : RF_NORMAL;
	do_start_af(af);
	AF_Trigger(&af->fv,&aft_in);
	af->vcm_stable = 0;
}

static void calibration_ae_mean(af_ctrl_t * af, char *test_param)
{
	FILE *fp = fopen("/data/misc/cameraserver/calibration_ae_mean.txt", "ab");
	cmr_u8 i = 0;
	UNUSED(test_param);
	if_lock_lsc(LOCK, af);
	if_lock_ae(LOCK, af);
	if_statistics_get_data(af->fv_combine, NULL, af);
	for (i = 0; i < 9; i++) {
		ISP_LOGV
		    ("pos %d AE_MEAN_WIN_%d R %d G %d B %d r_avg_all %d g_avg_all %d b_avg_all %d FV %lld\n",
		     get_vcm_registor_pos(af), i, af->ae_cali_data.r_avg[i], af->ae_cali_data.g_avg[i],
		     af->ae_cali_data.b_avg[i], af->ae_cali_data.r_avg_all, af->ae_cali_data.g_avg_all, af->ae_cali_data.b_avg_all, af->fv_combine[T_SPSMD]);
		fprintf(fp,
			"pos %d AE_MEAN_WIN_%d R %d G %d B %d r_avg_all %d g_avg_all %d b_avg_all %d FV %lld\n",
			get_vcm_registor_pos(af), i, af->ae_cali_data.r_avg[i], af->ae_cali_data.g_avg[i],
			af->ae_cali_data.b_avg[i], af->ae_cali_data.r_avg_all, af->ae_cali_data.g_avg_all, af->ae_cali_data.b_avg_all, af->fv_combine[T_SPSMD]);
	}
	fclose(fp);

}

static void trigger_defocus(af_ctrl_t * af, char *test_param)
{
	char *p1 = test_param;

	while (*p1 != '~' && *p1 != '\0')
		p1++;
	*p1++ = '\0';

	af->defocus = atoi(test_param);
	ISP_LOGV("af->defocus : %d \n", af->defocus);
/*
	af->request_mode = AF_MODE_NORMAL;
	af->state = STATE_NORMAL_AF;
	af->caf_state = CAF_IDLE;
	saf_start(af, NULL);	//SAF, win is NULL using default
*/
	return;
}

static void dump_focus_log(af_ctrl_t * af, char *test_param)
{
	char *p1 = test_param;

	while (*p1 != '~' && *p1 != '\0')
		p1++;
	*p1++ = '\0';

	af->fv.dump_log = atoi(test_param);

	ISP_LOGV("af->fv.dump_log : %d \n", af->fv.dump_log);
	return;
}

static void set_focus_stat_reg(af_ctrl_t * af, char *test_param)
{
	char *p1 = test_param;
	char *p2;
	while (*p1 != '~' && *p1 != '\0')
		p1++;
	*p1++ = '\0';

	p2 = p1;
	while (*p2 != '~' && *p2 != '\0')
		p2++;
	*p2++ = '\0';

	memset(&(af->stat_reg), 0, sizeof(focus_stat_reg_t));
	af->stat_reg.force_write = (1 == atoi(test_param)) ? (1) : (0);
	af->stat_reg.reg_param[0] = atoi(p1);
	af->stat_reg.reg_param[1] = atoi(p2);
	ISP_LOGV("%s - fw:%d p0:%d p1:%d \n", __FUNCTION__, af->stat_reg.force_write, af->stat_reg.reg_param[0]
		, af->stat_reg.reg_param[1]);

	return;
}

static void set_roi(af_ctrl_t * af, char *test_param)
{
	char *p1 = NULL;
	char *p2 = NULL;
	char *string = NULL;
	uint32_t len = 0;
	uint8 num = 0;
	roi_info_t *r = &af->roi;
	FILE *fp = NULL;
	UNUSED(test_param);

	if (0 == access("/data/misc/cameraserver/AF_roi.bin", R_OK)) {
		fp = fopen("/data/misc/cameraserver/AF_roi.bin", "rb");
		if (NULL == fp) {
			ISP_LOGI("open file AF_roi.bin fails");
			return;
		}

		fseek(fp, 0, SEEK_END);
		len = ftell(fp);
		string = malloc(len);
		if (NULL == string) {
			ISP_LOGI("malloc len of file AF_roi.bin fails");
			fclose(fp);
			return;
		}
		fseek(fp, 0, SEEK_SET);
		fread(string, 1, len, fp);
		fclose(fp);
		//parsing argumets start
		p1 = p2 = string;
		while (*p2 != '~' && *p2 != '\0')
			p2++;
		*p2++ = '\0';

		r->num = atoi(p1);

		num = 0;
		while (num < r->num) {	//set AF ROI
			p1 = p2;
			while (*p2 != '~' && *p2 != '\0')
				p2++;
			*p2++ = '\0';
			r->win[num].start_x = atoi(p1);
			r->win[num].start_x = (r->win[num].start_x >> 1) << 1;
			p1 = p2;
			while (*p2 != '~' && *p2 != '\0')
				p2++;
			*p2++ = '\0';
			r->win[num].start_y = atoi(p1);
			r->win[num].start_y = (r->win[num].start_y >> 1) << 1;
			p1 = p2;
			while (*p2 != '~' && *p2 != '\0')
				p2++;
			*p2++ = '\0';
			r->win[num].end_x = atoi(p1);
			r->win[num].end_x = (r->win[num].end_x >> 1) << 1;
			p1 = p2;
			while (*p2 != '~' && *p2 != '\0')
				p2++;
			*p2++ = '\0';
			r->win[num].end_y = atoi(p1);
			r->win[num].end_y = (r->win[num].end_y >> 1) << 1;
			ISP_LOGI("ROI %d win,(startx,starty,endx,endy) = (%d,%d,%d,%d)", num, r->win[num].start_x, r->win[num].start_y, r->win[num].end_x, r->win[num].end_y);
			num++;
		}
		//parsing argumets end
		if (NULL != string)
			free(string);

	} else {
		ISP_LOGI("file AF_roi.bin doesn't exist");
		return;
	}

	return;
}

static test_mode_command_t test_mode_set[] = {
	{"ISP_FOCUS_MANUAL", 0, &set_manual},
	{"ISP_FOCUS_CAF", 0, &trigger_caf},
	{"ISP_FOCUS_SAF", 0, &trigger_saf},
	{"ISP_FOCUS_CALIBRATION_AE_MEAN", 0, &calibration_ae_mean},
	{"ISP_FOCUS_VCM_SET_MODE", 0, &set_vcm_mode},
	{"ISP_FOCUS_VCM_GET_MODE", 0, &get_vcm_mode},
	{"ISP_FOCUS_LOCK_BLOCK", 0, &lock_block},
	{"ISP_FOCUS_SET_AE_GAIN_EXPOSURE", 0, &set_ae_gain_exp},
	{"ISP_FOCUS_DEFOCUS", 0, &trigger_defocus},
	{"ISP_FOCUS_DUMP_LOG", 0, &dump_focus_log},
	{"ISP_FOCUS_STAT_REG", 0, &set_focus_stat_reg},
	{"ISP_FOCUS_SET_ROI", 0, &set_roi},
	{"ISP_DEFAULT", 0, NULL},
};

static void set_af_test_mode(af_ctrl_t * af, char *af_mode)
{
#define CALCULATE_KEY(string,string_const) key=1; \
	while( *string!='~' && *string!='\0' ){ \
		key=key+*string; \
		string++; \
	} \
	if( 0==string_const ) \
		*string = '\0';

	char *p1 = af_mode;
	uint64 key = 0, i = 0;

	CALCULATE_KEY(p1, 0);

	while (i < sizeof(test_mode_set) / sizeof(test_mode_set[0])) {
		if (key == test_mode_set[i].key)
			break;
		i++;
	}

	if (sizeof(test_mode_set) / sizeof(test_mode_set[0]) <= i) {	// out of range in test mode,so initialize its ops
		ISP_LOGV("AF test mode Command is undefined,start af test mode initialization");
		i = 0;
		while (i < sizeof(test_mode_set) / sizeof(test_mode_set[0])) {
			p1 = test_mode_set[i].command;
			CALCULATE_KEY(p1, 1);
			test_mode_set[i].key = key;
			i++;
		}
		set_manual(af, NULL);
		return;
	}

	if (NULL != test_mode_set[i].command_func)
		test_mode_set[i].command_func(af, p1 + 1);
}

/* called each frame */
static cmr_s32 af_test_lens(af_ctrl_t * af)
{
	pthread_mutex_lock(&af->af_work_lock);
	AF_STOP(&af->fv, af->algo_mode);
	AF_Process_Frame(&af->fv);
	ISP_LOGV("AF_mode = %d", af->fv.AF_mode);
	pthread_mutex_unlock(&af->af_work_lock);

	ISP_LOGV("af_pos_set3 %s", AF_POS);
	lens_move_to(af, atoi(AF_POS));
	ISP_LOGV("af_pos_set4 %s", AF_POS);
	return 0;
}

#if AF_RING_BUFFER
static cmr_s32 af_dequeue_in_ringbuffer(af_ctrl_t * af)
{

//      isp_ctrl_context *isp = af->isp_ctx;
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)af->caller;
	struct isp_alg_fw_context *isp_ctx = (struct isp_alg_fw_context *)cxt_ptr->caller_handle;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_ctx->dev_access_handle;
	cmr_handle device = cxt->isp_driver_handle;

	cmr_s32 rtn;

	if (ISP_CHIP_ID_TSHARK3 == isp_dev_get_chip_id(device)) {
		af->node_type = ISP_NODE_TYPE_RAWAFM;
		rtn = isp_u_bq_dequeue_buf(device, &af->k_addr, &af->u_addr, af->node_type);
		ISP_LOGV("tshark3 chip dequeue Finished. rtn=%d, k_addr = 0x%x, u_addr = 0x%x", rtn, af->k_addr, af->u_addr);
		if (rtn || (0 == af->u_addr) || (0 == af->k_addr)) {
			ISP_LOGE("fail to get k_addr or u_addr, %d", rtn);
			return AF_RING_BUFFER_NO;
		}

		memcpy(af->af_statistics, af->u_addr, 105 * 4);

		return AF_RING_BUFFER_YES;
	} else {
		af->node_type = ISP_NODE_TYPE_RAWAFM;
		rtn = isp_u_bq_dequeue_buf(device, &af->k_addr, &af->u_addr, af->node_type);
		ISP_LOGV("dequeue Finished. rtn=%d, k_addr = 0x%x, u_addr = 0x%x", rtn, af->k_addr, af->u_addr);
		if (rtn || (0 == af->u_addr) || (0 == af->k_addr)) {
			ISP_LOGE("fail to get k_addr or u_addr, %d", rtn);
			return AF_RING_BUFFER_NO;
		}

		memcpy(af->af_statistics, af->u_addr, 25 * 4);	// will be 25*4*2,waiting for kernel compelement

		return AF_RING_BUFFER_YES;
	}
}

static cmr_s32 af_enqueue_in_ringbuffer(af_ctrl_t * af)
{

//      isp_ctrl_context *isp = af->isp_ctx;
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)af->caller;
	struct isp_alg_fw_context *isp_ctx = (struct isp_alg_fw_context *)cxt_ptr->caller_handle;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_ctx->dev_access_handle;
	cmr_handle device = cxt->isp_driver_handle;

	cmr_s32 rtn;

	if (ISP_CHIP_ID_TSHARK3 == isp_dev_get_chip_id(device)) {
		af->node_type = ISP_NODE_TYPE_RAWAFM;

//              isp->isp_smart_eb = 1;//gwb
		isp_u_bq_enqueue_buf(device, af->k_addr, af->u_addr, af->node_type);
		ISP_LOGV("isp_u_af_transaddr: Enqueue! k_addr = 0x%x, u_addr = 0x%x", rtn, af->k_addr, af->u_addr);
	} else {
		af->node_type = ISP_NODE_TYPE_RAWAFM;

//              isp->isp_smart_eb = 1;
		isp_u_bq_enqueue_buf(device, af->k_addr, af->u_addr, af->node_type);
		ISP_LOGV("isp_u_af_transaddr: Enqueue! k_addr = 0x%x, u_addr = 0x%x", rtn, af->k_addr, af->u_addr);
	}

	return 0;
}
#endif
static void ae_calc_win_size(af_ctrl_t * af, struct isp_video_start *param)
{
	if (param->size.w && param->size.h) {
		cmr_u32 w, h;
		w = ((param->size.w / 32) >> 1) << 1;
		h = ((param->size.h / 32) >> 1) << 1;
		af->ae.win_size = w * h;
	} else {
		af->ae.win_size = 1;
	}
}

/* porting form 2.0 end */
/***************************************************************************/
#if 0
static cmr_s32 afsprd_unload_lib(struct af_context_t *cxt)
{
	cmr_s32 rtn = AFV1_SUCCESS;

	if (NULL == cxt) {
		ISP_LOGE("fail to get param");
		rtn = AF_PARAM_NULL;
		goto exit;
	}

	if (NULL != cxt->lib_handle) {
		dlclose(cxt->lib_handle);
		cxt->lib_handle = NULL;
	}

exit:
	return rtn;
}
#endif
cmr_s32 sprd_afv1_deinit(cmr_handle handle, void *param, void *result)
{
	UNUSED(param);
	UNUSED(result);
#if 1
//    isp_ctrl_context *isp = (isp_ctrl_context *)handle;
	ISP_LOGV("E");
	af_ctrl_t *af = (af_ctrl_t *) handle;
	cmr_s32 rtn = AFV1_SUCCESS;

//    assert(isp);
//    af_ctrl_t *af = isp->handle_af;
//    assert(af);
	rtn = _check_handle(handle);
	if (AFV1_SUCCESS != rtn) {
		ISP_LOGE("fail to check handle");
		return AFV1_ERROR;
	}

	lens_move_to(af, 0);
	afm_disable(af);

	pthread_mutex_destroy(&af->af_work_lock);
	sem_destroy(&af->af_wait_caf);
	AF_deinit(&af->fv);
	trigger_deinit(af);
	//pthread_mutex_destroy(&af->caf_lock);
	property_set("af_mode", "none");
	property_set("af_set_pos", "none");
//    isp->handle_af = NULL;
	memset(af, 0, sizeof(*af));
	free(af);
	af = NULL;

	return rtn;
#else
//------------------------------------------------------
	UNUSED(param);
	UNUSED(result);
	cmr_int rtn = AFV1_SUCCESS;
	struct af_context_t *af_cxt = (struct af_context_t *)handle;
	struct af_monitor_set monitor_set;

	rtn = _check_handle(handle);
	if (AFV1_SUCCESS != rtn) {
		ISP_LOGE("fail to check handle");
		return AFV1_ERROR;
	}

	af_cxt = (struct af_context_t *)handle;

	monitor_set.bypass = 1;
	monitor_set.int_mode = 1;
	monitor_set.need_denoise = 0;
	monitor_set.skip_num = 0;
	monitor_set.type = 1;
	rtn = af_cxt->set_monitor(af_cxt->caller, &monitor_set, af_cxt->cur_envi);

	if (af_cxt->af_alg_handle) {
		rtn = af_cxt->lib_ops.af_deinit(af_cxt->af_alg_handle);
		if (rtn) {
			ISP_LOGE("fail to deinit af alg, rtn %ld", rtn);
		}
	}
	pthread_mutex_destroy(&af_cxt->status_lock);

	afsprd_unload_lib(af_cxt);
	memset(af_cxt, 0, sizeof(*af_cxt));
	free(af_cxt);

	ISP_LOGI("LiuY: done %ld", rtn);
	return rtn;
#endif
}

#if 0
static cmr_s32 afsprd_load_lib(struct af_context_t *cxt)
{
	cmr_s32 rtn = AFV1_SUCCESS;
	cmr_u32 v_count = 0;
	cmr_u32 v_id = 0;

	if (NULL == cxt) {
		ISP_LOGE("fail to check handle");
		rtn = AF_PARAM_NULL;
		goto exit;
	}

	v_count = sizeof(libafv1_path) / sizeof(libafv1_path[0]);
	v_id = cxt->lib_info->version_id;

	if (v_id >= v_count) {
		ISP_LOGE("fail to get version id :%d", v_id);
		rtn = AFV1_ERROR;
		goto exit;
	}
	ISP_LOGI("af lib v_count:%d, v_id:%d, version_name:%s", v_count, v_id, libafv1_path[v_id]);

	cxt->lib_handle = dlopen(libafv1_path[v_id], RTLD_NOW);
	if (!cxt->lib_handle) {
		ISP_LOGE("fail to dlopen af lib");
		rtn = AFV1_ERROR;
		goto exit;
	}

	cxt->lib_ops.af_init = dlsym(cxt->lib_handle, "af_alg_init");
	cxt->lib_ops.af_calculation = dlsym(cxt->lib_handle, "af_alg_calculation");
	cxt->lib_ops.af_deinit = dlsym(cxt->lib_handle, "af_alg_deinit");
	cxt->lib_ops.af_ioctrl = dlsym(cxt->lib_handle, "af_alg_ioctrl");

	if (NULL == cxt->lib_ops.af_init || NULL == cxt->lib_ops.af_calculation || NULL == cxt->lib_ops.af_deinit || NULL == cxt->lib_ops.af_ioctrl) {
		ISP_LOGE("fail to dlsym af lib");
		rtn = AFV1_ERROR;
		goto load_error;
	}

	return rtn;

load_error:
	afsprd_unload_lib(cxt);

exit:
	return rtn;
}
#endif
cmr_handle sprd_afv1_init(void *in, void *out)
{
	af_ctrl_t *af = NULL;
#if 1
	ISP_LOGI("B");
	struct afctrl_init_in *init_param = (struct afctrl_init_in *)in;
	struct afctrl_init_out *result = (struct afctrl_init_out *)out;
	struct isp_alg_fw_context *isp_ctx = NULL;
	struct isp_pm_ioctl_input af_pm_input;
	struct isp_pm_ioctl_output af_pm_output;
	cmr_s32 rtn = AFV1_SUCCESS;

	if (NULL == init_param) {
		ISP_LOGE("fail to init param:%p, result:%p", init_param, result);
		goto INIT_ERROR_EXIT;
	}
	isp_ctx = (struct isp_alg_fw_context *)init_param->caller_handle;

	memset((void *)&af_pm_input, 0, sizeof(af_pm_input));
	memset((void *)&af_pm_output, 0, sizeof(af_pm_output));

	rtn = isp_pm_ioctl(isp_ctx->handle_pm, ISP_PM_CMD_GET_INIT_AF_NEW, NULL, &af_pm_output);
	if (ISP_SUCCESS == rtn) {
		ISP_LOGV("load af tuning params succeed");
#if 0
		//struct af_tuning_param *af_tuning = NULL;

		af_tuning = (struct af_tuning_param *)malloc(sizeof(struct af_tuning_param) * af_pm_output.param_num);
		if (NULL == af_tuning) {
			ISP_LOGE("fail to malloc");
			return ISP_MALLOC_ERROR;
		}
		for (i = 0; i < af_pm_output.param_num; i++) {
			af_tuning[i].cfg_mode = (af_pm_output.param_data->id & 0xffff0000) >> 16;
			af_tuning[i].data = af_pm_output.param_data->data_ptr;
			af_tuning[i].data_len = af_pm_output.param_data->data_size;
			af_pm_output.param_data++;
		}
		in.tuning_param = af_tuning;
#endif
	} else {
		ISP_LOGV("load af tuning params failed");
		return NULL;
	}

	af = (af_ctrl_t *) malloc(sizeof(*af));
	if (NULL == af) {
		ISP_LOGV("malloc fail");
		return NULL;
	}
	//afm_disable(af);//gwb
	//afm_setup(isp); // default settings

	memset(af, 0, sizeof(*af));
	af->isp_info.width = init_param->src.w;	//init_param->plat_info.isp_w;
	af->isp_info.height = init_param->src.h;	//init_param->plat_info.isp_h;
	af->isp_info.win_num = afm_get_win_num(init_param);
	af->caller = init_param->caller;
	//af->go_position = init_param->go_position;
	af->end_notice = init_param->end_notice;
	af->start_notice = init_param->start_notice;
	af->set_monitor = init_param->set_monitor;
	af->set_monitor_win = init_param->set_monitor_win;
	af->get_monitor_win_num = init_param->get_monitor_win_num;
	//af->ae_awb_lock = init_param->ae_awb_lock;
	//af->ae_awb_release = init_param->ae_awb_release;
	af->lock_module = init_param->lock_module;
	af->unlock_module = init_param->unlock_module;

	af->fv.AF_Ops.cookie = af;
	af->fv.AF_Ops.statistics_wait_cal_done = if_statistics_wait_cal_done;
	af->fv.AF_Ops.statistics_get_data = if_statistics_get_data;
	af->fv.AF_Ops.statistics_set_data = if_statistics_set_data;
	af->fv.AF_Ops.lens_get_pos = if_lens_get_pos;
	af->fv.AF_Ops.lens_move_to = if_lens_move_to;
	af->fv.AF_Ops.lens_wait_stop = if_lens_wait_stop;
	af->fv.AF_Ops.lock_ae = if_lock_ae;
	af->fv.AF_Ops.lock_awb = if_lock_awb;
	af->fv.AF_Ops.lock_lsc = if_lock_lsc;
	af->fv.AF_Ops.get_sys_time = if_get_sys_time;
	af->fv.AF_Ops.sys_sleep_time = if_sys_sleep_time;
	af->fv.AF_Ops.get_ae_report = if_get_ae_report;
	af->fv.AF_Ops.set_af_exif = if_set_af_exif;
	af->fv.AF_Ops.get_otp_data = if_get_otp;
	af->fv.AF_Ops.get_motor_pos = if_get_motor_pos;
	af->fv.AF_Ops.set_motor_sacmode = if_set_motor_sacmode;
	af->fv.AF_Ops.binfile_is_exist = if_binfile_is_exist;
	af->fv.AF_Ops.af_log = if_af_log;
	af->fv.AF_Ops.af_start_notify = if_af_start_notify;
	af->fv.AF_Ops.af_end_notify = if_af_end_notify;
	af->fv.AF_Ops.phase_detection_get_data = if_phase_detection_get_data;
	af->fv.AF_Ops.motion_sensor_get_data = if_motion_sensor_get_data;
	af->fv.AF_Ops.get_vcm_param = if_get_vcm_param;
	af->curr_scene = INDOOR_SCENE;

	af->ae_lock_num = 1;
	af->awb_lock_num = 0;
	af->lsc_lock_num = 0;
	af->nlm_lock_num = 0;
	af->caf_first_stable = 0;

	pthread_mutex_init(&af->af_work_lock, NULL);
	pthread_mutex_init(&af->caf_work_lock, NULL);
	sem_init(&af->af_wait_caf, 0, 0);
	af->dcam_timestamp = 0xffffffffffffffff;
	set_vcm_chip_ops(af);
	load_settings(af, &af_pm_output);
	AF_init(&af->fv);
	Get_AF_tuning_Data(&af->fv);

	faf_trigger_init(af);
	if (trigger_init(af, CAF_TRIGGER_LIB) != 0) {
		ISP_LOGE("fail to init trigger");
		goto INIT_ERROR_EXIT;
	}
	//pthread_mutex_init(&af->caf_lock, NULL);

	ISP_LOGV("width = %d, height = %d, win_num = %d", af->isp_info.width, af->isp_info.height, af->isp_info.win_num);

	isp_ctx->af_cxt.log_af = (cmr_u8 *) af;
	isp_ctx->af_cxt.log_af_size = sizeof(*af);
	lens_move_to(af, af->fv.AF_OTP.INF);
	/*
	   AF process need to do af once when af init done.
	 */
	af->inited_af_req = AFV1_TRUE;
	assert(sizeof(af->af_version) >= strlen("AF-") + strlen(af->fv.AF_Version) + strlen(AF_SYS_VERSION));
	memcpy(af->af_version, "AF-", strlen("AF-"));
	memcpy(af->af_version + strlen("AF-"), af->fv.AF_Version, sizeof(af->fv.AF_Version));
	memcpy(af->af_version + strlen("AF-") + strlen((char *)af->fv.AF_Version), AF_SYS_VERSION, strlen(AF_SYS_VERSION));
	ISP_LOGV("AFVER %s lib mem 0x%x ", af->af_version, sizeof(AF_Data));
	property_set("af_mode", "none");
	{
		FILE *fp = NULL;
		af_tuning_param_t tuning_data;
		fp = fopen("/data/misc/cameraserver/af_tuning_default.bin", "wb");
		if (fp == NULL) {
			ISP_LOGE("fail to init af\n");
			af = NULL;
			return (cmr_handle) af;
		}
		memset(&tuning_data, 0, sizeof(tuning_data));
		memcpy(tuning_data.filter_clip, af->filter_clip, sizeof(af->filter_clip));
		memcpy(tuning_data.bv_threshold, af->bv_threshold, sizeof(af->bv_threshold));

		memcpy(&tuning_data.SAF_win, &af->af_tuning_data.SAF_win, sizeof(AF_Window_Config));
		memcpy(&tuning_data.CAF_win, &af->af_tuning_data.CAF_win, sizeof(AF_Window_Config));
		memcpy(&tuning_data.VAF_win, &af->af_tuning_data.VAF_win, sizeof(AF_Window_Config));

		memcpy(&tuning_data.AF_Tuning_Data[OUT_SCENE], &af->fv.AF_Tuning_Data, sizeof(af->fv.AF_Tuning_Data));
		memcpy(&tuning_data.AF_Tuning_Data[INDOOR_SCENE], &af->fv.AF_Tuning_Data, sizeof(af->fv.AF_Tuning_Data));
		memcpy(&tuning_data.AF_Tuning_Data[DARK_SCENE], &af->fv.AF_Tuning_Data, sizeof(af->fv.AF_Tuning_Data));

		fwrite(&tuning_data, 1, sizeof(tuning_data), fp);
		fclose(fp);
		ISP_LOGV("sizeof(tuning_data) = %d", sizeof(tuning_data));
	}
	iir_level = 1;
	nr_mode = 2;
	cw_mode = 2;
	fv0_e = fv1_e = 5;
	return af;
#if 0
ISP_MALLOC_ERROR:
	if (af_tuning) {
		free(af_tuning);
		af_tuning = NULL;
	}
#endif
INIT_ERROR_EXIT:
	if (NULL != af) {
		rtn = sprd_afv1_deinit((cmr_handle) af, NULL, NULL);
		if (rtn) {
			ISP_LOGE("fail to deinit af, rtn %d", rtn);
		}
	}
	af = NULL;
	ISP_LOGI("E");
	return (cmr_handle) af;
#else
//------------------------------------------------------------------------
	struct af_context_t *af_cxt = NULL;
	struct afctrl_init_in *init_param = (struct afctrl_init_in *)in;
	struct afctrl_init_out *result = (struct afctrl_init_out *)out;
	cmr_s32 rtn = AFV1_SUCCESS;
	if (NULL == init_param) {
		ISP_LOGE("fail to init param, init_param:%p , result:%p", init_param, result);
		goto INIT_ERROR_EXIT;
	}

	af_cxt = (struct af_context_t *)malloc(sizeof(struct af_context_t));
	if (NULL == af_cxt) {
		ISP_LOGE("fail to malloc");
		goto INIT_ERROR_EXIT;
	}
	ISP_LOGI("af_init start");
	memset(af_cxt, 0, sizeof(struct af_context_t));
	af_cxt->magic_start = AFV1_MAGIC_START;
	af_cxt->magic_end = AFV1_MAGIC_END;
	af_cxt->af_mode = init_param->af_mode;
	af_cxt->bypass = init_param->af_bypass;
	af_cxt->caller = init_param->caller;
	af_cxt->go_position = init_param->go_position;
	af_cxt->end_notice = init_param->end_notice;
	af_cxt->start_notice = init_param->start_notice;
	af_cxt->set_monitor = init_param->set_monitor;
	af_cxt->set_monitor_win = init_param->set_monitor_win;
	af_cxt->get_monitor_win_num = init_param->get_monitor_win_num;
	af_cxt->ae_awb_lock = init_param->ae_awb_lock;
	af_cxt->ae_awb_release = init_param->ae_awb_release;
	af_cxt->plat_info = init_param->plat_info;
	af_cxt->cur_envi = AF_ENVI_INDOOR;
	af_cxt->bv_thr[0] = 60;
	af_cxt->bv_thr[1] = 150;
	af_cxt->rgbdiff_thr[0] = 8;
	af_cxt->rgbdiff_thr[1] = 8;
	af_cxt->rgbdiff_thr[2] = 8;
	af_cxt->src.w = init_param->src.w;
	af_cxt->src.h = init_param->src.h;
	af_cxt->lib_info = &init_param->lib_param;

	rtn = afsprd_load_lib(af_cxt);
	if (AFV1_SUCCESS != rtn) {
		ISP_LOGE("fail to load af lib");
		goto INIT_ERROR_EXIT;
	}

	rtn = _alg_init((afv1_handle_t) af_cxt, init_param, result);
	if (AFV1_SUCCESS != rtn) {
		ISP_LOGE("fail to init af alg handle");
		goto INIT_ERROR_EXIT;
	}

	af_cxt->init_flag = AF_TRUE;
	af_cxt->af_has_suc_rec = AF_FALSE;
	af_cxt->ae_awb_lock_cnt = 0;
	//_af_set_motor_pos((afv1_handle_t)af_cxt,result->init_motor_pos);
	pthread_mutex_init(&af_cxt->status_lock, NULL);

	ISP_LOGI("af_init end ");
	return (afv1_handle_t) af_cxt;

INIT_ERROR_EXIT:
	if (af_cxt) {
		rtn = sprd_afv1_deinit((cmr_handle) af_cxt, NULL, NULL);
		if (rtn) {
			ISP_LOGE("fail to deinit af, rtn %d", rtn);
		}
	}
	af_cxt = NULL;
	return (cmr_handle) af_cxt;
#endif
}

static cmr_int af_sprd_adpt_update_aux_sensor(cmr_handle handle, void *in)
{
	af_ctrl_t *cxt = (af_ctrl_t *) handle;
	struct af_aux_sensor_info_t *aux_sensor_info = (struct af_aux_sensor_info_t *)in;

	switch (aux_sensor_info->type) {
	case AF_ACCELEROMETER:
		ISP_LOGV("accelerometer vertical_up = %f vertical_down = %f horizontal = %f",
			aux_sensor_info->gsensor_info.vertical_up, aux_sensor_info->gsensor_info.vertical_down, aux_sensor_info->gsensor_info.horizontal);
		cxt->gsensor_info.vertical_up = aux_sensor_info->gsensor_info.vertical_up;
		cxt->gsensor_info.vertical_down = aux_sensor_info->gsensor_info.vertical_down;
		cxt->gsensor_info.horizontal = aux_sensor_info->gsensor_info.horizontal;
		cxt->gsensor_info.timestamp = aux_sensor_info->gsensor_info.timestamp;
		cxt->gsensor_info.valid = 1;
		break;
	case AF_MAGNETIC_FIELD:
		ISP_LOGV("magnetic field E");
		break;
	case AF_GYROSCOPE:
		/*TBD trans_data_to CAF */
		ISP_LOGV("af_sprd_adpt_update_aux_sensor");
		//afaltek_adpt_trans_data_to_caf(cxt, aux_sensor_info, AFT_DATA_SENSOR);
		break;
	case AF_LIGHT:
		ISP_LOGV("light E");
		break;
	case AF_PROXIMITY:
		ISP_LOGV("proximity E");
		break;
	default:
		ISP_LOGI("sensor type not support");
		break;
	}

	return ISP_SUCCESS;
}

cmr_s32 sprd_afv1_process(afv1_handle_t handle, void *in, void *out)
{
#if 1
	af_ctrl_t *af = (af_ctrl_t *) handle;
	struct afctrl_calc_in *inparam = (struct afctrl_calc_in *)in;
	struct afctrl_calc_out *result = (struct afctrl_calc_out *)out;
	cmr_int rtn = AFV1_SUCCESS;
	cmr_int i = 0;
	nsecs_t system_time0 = 0;
	nsecs_t system_time1 = 0;
	ISP_LOGV("B");

	rtn = _check_handle(handle);
	if (AFV1_SUCCESS != rtn) {
		ISP_LOGE("fail to check handle");
		return AFV1_ERROR;
	}

	if (NULL == inparam) {
		ISP_LOGE("fail to get input param");
		return AFV1_ERROR;
	} else if (NULL == inparam->data) {
		ISP_LOGE("fail to get input param data");
		return AFV1_ERROR;
	}

	if (1 == af->bypass)
		return 0;

	property_get("af_mode", AF_MODE, "none");
	if (0 != strcmp(AF_MODE, "none")) {
		set_af_test_mode(af, AF_MODE);
		property_set("af_mode", "ISP_DEFAULT");
		property_get("af_set_pos", AF_POS, "none");
		ISP_LOGV("test AF_MODE %s, AF_POS %s", AF_MODE, AF_POS);
		if (0 != strcmp(AF_POS, "none")) {
			af_test_lens(af);
			property_set("af_set_pos", "none");
			return 0;
		}
	}
	// ISP_LOGV("state = %s, pre_state = %s, cur mode = %d", STATE_STRING(af->state), STATE_STRING(af->pre_state), af->request_mode);
	if (1 != Is_ae_stable(af)) {
		ISP_LOGV("ae not stable in non caf mode");
		return 0;
	}
	//AF ring buffer
#if (AF_RING_BUFFER)
	if (AF_RING_BUFFER_YES != af_dequeue_in_ringbuffer(af)) {
		return 0;
	}
#endif

	system_time0 = systemTime(CLOCK_MONOTONIC) / 1000000LL;

	ISP_LOGV("state = %s, caf_state = %s", STATE_STRING(af->state), CAF_STATE_STR(af->caf_state));
	switch (inparam->data_type) {
	case AF_DATA_AF:
		{
			cmr_u32 *af_fv_val = NULL;
			cmr_s32 afm_skip_num = 0;
			af_fv_val = (cmr_u32 *) (inparam->data);

			for (i = 0; i < 10; i++) {
				af->af_fv_val.af_fv0[i] = ((((cmr_u64) af_fv_val[20 + i]) & 0x00000fff) << 32) | (((cmr_u64) af_fv_val[i]));
				af->af_fv_val.af_fv1[i] = (((((cmr_u64) af_fv_val[20 + i]) >> 12) & 0x00000fff) << 32) | ((cmr_u64) af_fv_val[10 + i]);
			}
			af->afm_skip_num = 0;
			if (inparam->sensor_fps.is_high_fps) {
				afm_skip_num = inparam->sensor_fps.high_fps_skip_num - 1;
				if (afm_skip_num > 0)
					af->afm_skip_num = afm_skip_num;
				else
					af->afm_skip_num = 0;
				ISP_LOGI("af.skip_num %d", af->afm_skip_num);
			}

			//transfer fv data to trigger
			caf_monitor_process_af(af);

			switch (af->state) {
			case STATE_NORMAL_AF:
				pthread_mutex_lock(&af->af_work_lock);
				if (saf_process_frame(af)) {
					af->state = STATE_IDLE;
					//saf_stop(af);
					//do_stop_af(af);
				}
				trigger_start(af); //reset trigger after saf
				pthread_mutex_unlock(&af->af_work_lock);
				break;
			case STATE_FULLSCAN:
			case STATE_CAF:
			case STATE_RECORD_CAF:
				//do caf when af is triggered.
				caf_process_af(af);
				break;
			case STATE_FAF:
				pthread_mutex_lock(&af->af_work_lock);
				if (faf_process_frame(af)) {
					af->state = STATE_CAF;
					caf_start(af);
				}
				pthread_mutex_unlock(&af->af_work_lock);
				break;
			default:
				pthread_mutex_lock(&af->af_work_lock);
				AF_Process_Frame(&af->fv);
				ISP_LOGV("AF_mode = %d", af->fv.AF_mode);
				pthread_mutex_unlock(&af->af_work_lock);
				break;
			}
			break;
		}

	case AF_DATA_IMG_BLK:
		{
			struct af_img_blk_info *img_blk_info = (struct af_img_blk_info *)inparam->data;
			isp_awb_statistic_hist_info_t af_tmp;
			if (NULL != img_blk_info->data) {
				struct isp_awb_statistic_info *ae_stat = (struct isp_awb_statistic_info *)img_blk_info->data;
				memcpy(af_tmp.r_info, ae_stat->r_info, sizeof(af_tmp.r_info));
				memcpy(af_tmp.g_info, ae_stat->g_info, sizeof(af_tmp.g_info));
				memcpy(af_tmp.b_info, ae_stat->b_info, sizeof(af_tmp.b_info));

				isp_awb_statistic_hist_info_t *stat = &af_tmp;
				caf_process_ae(af, &af->ae.ae, stat);
			}
			break;
		}

	case AF_DATA_AE:
		{
			break;
		}

	default:
		{
			ISP_LOGV("unsupport data type! type: %d", inparam->data_type);
			rtn = AFV1_ERROR;
			break;
		}

	}
	system_time1 = systemTime(CLOCK_MONOTONIC) / 1000000LL;
	ISP_LOGV("SYSTEM_TEST-af:%lldms", system_time1 - system_time0);

#if (AF_RING_BUFFER)
	af_enqueue_in_ringbuffer(af);
#endif
	ISP_LOGV("E");
	return rtn;
#else
	cmr_int rtn = AFV1_SUCCESS;
	struct af_context_t *af_cxt = (struct af_context_t *)handle;
	struct af_calc_param *param = (struct af_calc_param *)in;
	struct af_result_param *result = (struct af_result_param *)out;
	struct af_alg_calc_param alg_calc_param;
	struct af_alg_result alg_calc_result;

	rtn = _check_handle(handle);
	if (AFV1_SUCCESS != rtn) {
		ISP_LOGE("fail to check handle");
		return AFV1_ERROR;
	}

	if (1 == af_cxt->isp_tool_af_test) {	// isp tool af test
		ISP_LOGV("ISP_TOOL_AF_TEST");
		return rtn;
	}

	_cfg_af_calc_param(handle, param, &alg_calc_param);
	alg_calc_result = af_cxt->alg_result;
	alg_calc_result.is_caf_trig = AF_FALSE;
	alg_calc_result.is_finish = AF_FALSE;
	alg_calc_param.cur_motor_pos = af_cxt->cur_af_pos;
	alg_calc_result.motor_pos = af_cxt->cur_af_pos;
	alg_calc_param.af_has_suc_rec = af_cxt->af_has_suc_rec;
	rtn = af_cxt->lib_ops.af_calculation(af_cxt->af_alg_handle, &alg_calc_param, &alg_calc_result);

	if (af_cxt->cur_af_pos != alg_calc_result.motor_pos) {
		_af_set_motor_pos(handle, alg_calc_result.motor_pos);
	}
	if ((!af_cxt->alg_result.is_finish) && alg_calc_result.is_finish) {
		result->motor_pos = alg_calc_result.motor_pos;
		result->suc_win = alg_calc_result.suc_win;
		_af_finish(handle, result);
		_caf_reset_after_af(handle);
	} else if ((!af_cxt->alg_result.is_caf_trig) && alg_calc_result.is_caf_trig) {
		af_cxt->start_notice(af_cxt->caller);
		_caf_trig_af_start(handle);
	} else if ((af_cxt->af_mode == AF_ALG_MODE_CONTINUE || af_cxt->af_mode == AF_ALG_MODE_VIDEO)
		   && af_cxt->alg_result.is_caf_trig_in_saf) {
		af_cxt->start_notice(af_cxt->caller);
		_caf_trig_af_start(handle);
	}
	af_cxt->alg_result = alg_calc_result;
	if ((af_cxt->af_mode == AF_ALG_MODE_CONTINUE || af_cxt->af_mode == AF_ALG_MODE_VIDEO)
	    && af_cxt->alg_result.is_caf_trig_in_saf) {
		af_cxt->alg_result.is_caf_trig_in_saf = AF_FALSE;
	}
	ISP_LOGI("LiuY done %ld", rtn);
	return rtn;
#endif
}

cmr_s32 sprd_afv1_ioctrl(void *handle, cmr_s32 cmd, void *param0, void *param1)
{
	UNUSED(param1);
	cmr_int rtn = AFV1_SUCCESS;
	af_ctrl_t *af = (af_ctrl_t *) handle;
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)af->caller;
	struct isp_alg_fw_context *isp_ctx = (struct isp_alg_fw_context *)cxt_ptr->caller_handle;
	struct isp_video_start *in_ptr = NULL;
	AF_Trigger_Data aft_in;

	rtn = _check_handle(handle);
	if (AFV1_SUCCESS != rtn) {
		ISP_LOGE("fail to check cxt");
		return AFV1_ERROR;
	}

	pthread_mutex_lock(&af->status_lock);
	ISP_LOGV("cmd is 0x%x", cmd);
	switch (cmd) {
	case AF_CMD_SET_AF_MODE:
		rtn = af_sprd_set_mode(handle, param0);
		break;

	case AF_CMD_SET_AF_POS:
		//rtn = _af_set_motor_pos(handle, *(cmr_u32 *) param0);
		if (NULL != af->vcm_ops.set_pos) {
			af->vcm_ops.set_pos(isp_ctx->ioctrl_ptr->caller_handler, *(cmr_u16 *) param0);
		}
		break;

	case AF_CMD_SET_TUNING_MODE:
/*
	rtn = af_cxt->lib_ops.af_ioctrl(af_cxt->af_alg_handle,AF_ALG_CMD_SET_CAF_STOP,NULL,NULL);
	rtn = _af_set_status(handle,AF_ALG_STATUS_STOP);
	if (af_cxt->is_running) {
		af_cxt->af_result.suc_win = 0;
		rtn = _af_end_proc(handle,&af_cxt->af_result,AF_TRUE);
	}

	rtn = af_cxt->lib_ops.af_ioctrl(af_cxt->af_alg_handle,AF_ALG_CMD_SET_TUNING_MODE,param0,NULL);
*/
		break;
	case AF_CMD_SET_ISP_TOOL_AF_TEST:
//              af->isp_tool_af_test = *(cmr_u32*)param0;
		break;
	case AF_CMD_SET_SCENE_MODE:

		break;
	case AF_CMD_SET_AF_START:
#if 1
		{
			property_set("af_mode", "none");
			//win = (struct isp_af_win *)param;
			struct af_trig_info *win = (struct af_trig_info *)param0;

			//ISP_LOGV("state = %s, win = %d", STATE_STRING(af->state), win->valid_win);
			if (STATE_INACTIVE == af->state) {
				rtn = 0;
				break;
			}

			if (STATE_FULLSCAN == af->state) {
				af->caf_state = CAF_SEARCHING;
				af->algo_mode = CAF;
				memset(&aft_in,0,sizeof(AF_Trigger_Data));
				aft_in.AFT_mode = af->algo_mode;
				aft_in.bisTrigger = AF_TRIGGER;
				aft_in.AF_Trigger_Type = (1==af->defocus)? DEFOCUS : RF_NORMAL;
				AF_Trigger(&af->fv, &aft_in);
				do_start_af(af);
				break;
			}

			if (STATE_IDLE != af->state) {
				if (((STATE_CAF == af->state) || (STATE_RECORD_CAF == af->state))
				    /*&& (0 != win->valid_win) */
				    ) {
					suspend_caf(af);
				} else {
					ISP_LOGV("notify_stop");
					notify_stop(af, 0);
					rtn = 0;
					break;
				}
			}

			af->state = STATE_NORMAL_AF;
			saf_start(af, win);
			break;
		}
#else
		{
			cmr_u32 af_mode = af_cxt->af_mode;
			struct af_trig_info *trig_info = (struct af_trig_info *)param0;

			if (((AF_MODE_CONTINUE == af_mode) || (AF_MODE_VIDEO == af_mode))
			    && (0 == af_cxt->flash_on)
			    && (0 == trig_info->win_num)) {
				if (af_cxt->is_running) {
					break;
				} else {
					af_cxt->af_result.suc_win = 1;
					rtn = _af_end_proc(handle, &af_cxt->af_result, AF_TRUE);
					break;
				}
			}

			af_cxt->isp_tool_af_test = 0;
			rtn = af_cxt->lib_ops.af_ioctrl(af_cxt->af_alg_handle, AF_ALG_CMD_SET_CAF_STOP, NULL, NULL);
			rtn = _af_set_status(handle, AF_ALG_STATUS_STOP);
			rtn = _af_set_start(handle, (struct af_trig_info *)param0);
			break;
		}
#endif
		break;
	case AF_CMD_SET_CAF_TRIG_START:
/*
	af_cxt->isp_tool_af_test = 0;
	rtn = af_cxt->lib_ops.af_ioctrl(af_cxt->af_alg_handle,AF_ALG_CMD_SET_CAF_STOP,NULL,NULL);
	rtn = _af_set_status(handle,AF_ALG_STATUS_STOP);
	rtn = _af_set_start(handle,(struct af_trig_info *)param0);
*/
		break;

	case AF_CMD_SET_AF_STOP:
#if 1
		ISP_LOGV("state = %s", STATE_STRING(af->state));
#else
		rtn = af_cxt->lib_ops.af_ioctrl(af_cxt->af_alg_handle, AF_ALG_CMD_SET_CAF_STOP, NULL, NULL);
		rtn = _af_set_status(handle, AF_ALG_STATUS_STOP);
		af_cxt->af_result.suc_win = 0;
		rtn = _af_end_proc(handle, &af_cxt->af_result, AF_TRUE);
#endif
		break;

	case AF_CMD_SET_AF_RESTART:

		break;

	case AF_CMD_SET_CAF_RESET:
//              rtn = af_cxt->lib_ops.af_ioctrl(af_cxt->af_alg_handle,AF_ALG_CMD_SET_CAF_RESET,NULL,NULL);
		break;

	case AF_CMD_SET_CAF_STOP:
//              rtn = af_cxt->lib_ops.af_ioctrl(af_cxt->af_alg_handle,AF_ALG_CMD_SET_CAF_STOP,NULL,NULL);
		break;
	case AF_CMD_SET_AF_FINISH:
//              if (af_cxt->is_running) {
//                      rtn = _af_end_proc(handle,(struct af_result_param *)param0,AF_TRUE);
//              }
		break;
	case AF_CMD_SET_AF_BYPASS:
		af->bypass = *(cmr_u32 *) param0;
#if 0
		af_cxt->bypass = *(cmr_u32 *) param0;
		if (af_cxt->bypass) {
			rtn = af_cxt->lib_ops.af_ioctrl(af_cxt->af_alg_handle, AF_ALG_CMD_SET_CAF_STOP, NULL, NULL);
			rtn = _af_set_status(handle, AF_ALG_STATUS_STOP);
		} else {
			if ((af_cxt->af_mode == AF_ALG_MODE_CONTINUE) || (af_cxt->af_mode == AF_ALG_MODE_VIDEO)) {
				rtn = af_sprd_set_mode(handle, (void *)&af_cxt->af_mode);
			}
		}
#endif
		break;

	case AF_CMD_SET_DEFAULT_AF_WIN:
		//rtn = _af_cfg_afm_win(handle, 1, 0, NULL);
		break;

	case AF_CMD_SET_FLASH_NOTICE:{
			cmr_u32 flash_status;
			cmr_u32 af_mode = af->request_mode;

			flash_status = *(cmr_u32 *) param0;

			switch (flash_status) {
			case ISP_FLASH_PRE_BEFORE:
			case ISP_FLASH_PRE_LIGHTING:
			case ISP_FLASH_MAIN_BEFORE:
			case ISP_FLASH_MAIN_LIGHTING:
				if (((AF_MODE_CONTINUE == af_mode) || (AF_MODE_VIDEO == af_mode))) {
					//rtn = af_cxt->lib_ops.af_ioctrl(af_cxt->af_alg_handle,AF_ALG_CMD_SET_CAF_STOP,NULL,NULL);
				}
				//if (af->is_running) {
				//rtn = _af_end_proc(handle,&af->af_result,AF_FALSE);
				//}
				if (0 == af->flash_on) {
					af->flash_on = 1;
				}

				break;

			case ISP_FLASH_PRE_AFTER:
			case ISP_FLASH_MAIN_AFTER:
				if (((AF_MODE_CONTINUE == af_mode) || (AF_MODE_VIDEO == af_mode))) {
					//rtn = af_cxt->lib_ops.af_ioctrl(af_cxt->af_alg_handle,AF_ALG_CMD_SET_CAF_RESET,NULL,NULL);
				}
				if (0 == af->flash_on) {
					af->flash_on = 0;
				}
				break;
			default:
				break;

			}

			break;
		}

	case AF_CMD_SET_ISP_START_INFO:
		in_ptr = (struct isp_video_start *)param0;
		af->isp_info.width = in_ptr->size.w;
		af->isp_info.height = in_ptr->size.h;
		ISP_LOGV("isp start af width = %d, height = %d", in_ptr->size.w, in_ptr->size.h);
		ISP_LOGV("isp start af state = %s", STATE_STRING(af->state));
		//af->state = af->pre_state = STATE_IDLE;
		property_get("af_mode", AF_MODE, "none");
		if (0 == strcmp(AF_MODE, "none")) {
			af->state = af->pre_state = STATE_CAF;
			caf_start(af);
			ae_calc_win_size(af, param0);
		}
		break;

	case AF_CMD_SET_ISP_STOP_INFO:
//              if (af_cxt->is_running) {
//                      rtn = _af_end_proc(handle,&af_cxt->af_result,AF_FALSE);
//              }
		ISP_LOGV("isp stop af state = %s", STATE_STRING(af->state));
		if (STATE_IDLE != af->state)
			do_stop_af(af);

		break;

	case AF_CMD_SET_AE_INFO:{
			struct isp_awb_statistic_info *ae_stat_ptr = param0;
			struct ae_out_bv *ae_out = (struct ae_out_bv *)param1;
			struct ae_calc_out *ae_result = ae_out->ae_result;
			set_af_RGBY(af, (void *)ae_stat_ptr);
			set_ae_info(af, ae_result, ae_out->bv);
/*
	struct ae_calc_out *ae_result = (struct ae_calc_out*)param0;
	cmr_s32 *bv = (cmr_s32 *)param1;
	af_cxt->ae_cur_lum = ae_result->cur_lum;
	af_cxt->ae_is_stab = ae_result->is_stab;

	if (ae_result->cur_exp_line && ae_result->line_time)
		af_cxt->cur_fps = 100000000/(ae_result->cur_exp_line*ae_result->line_time);
	af_cxt->cur_ae_again = ae_result->cur_again;
	af_cxt->cur_ae_bv = *bv;
	if (*bv <= af_cxt->bv_thr[0]){
		af_cxt->cur_envi = AF_ENVI_LOWLUX;
	}else if (*bv < af_cxt->bv_thr[1] && *bv > af_cxt->bv_thr[0]) {
		af_cxt->cur_envi = AF_ENVI_INDOOR;
	}else {
		af_cxt->cur_envi = AF_ENVI_OUTDOOR;
	}
*/
			break;
		}

	case AF_CMD_SET_AWB_INFO:{
			struct awb_ctrl_calc_result *result = (struct awb_ctrl_calc_result *)param0;
			struct awb_gain gain;	// = result->gain
			gain.r = result->gain.r;
			gain.g = result->gain.g;
			gain.b = result->gain.b;
			set_awb_info(af, result, &gain);
/*
	struct awb_ctrl_calc_result *awb_result = (struct awb_ctrl_calc_result*)param0;
	cmr_u32 r_diff;
	cmr_u32 g_diff;
	cmr_u32 b_diff;

	r_diff = awb_result->gain.r > af_cxt->cur_awb_r_gain ? awb_result->gain.r - af_cxt->cur_awb_r_gain : af_cxt->cur_awb_r_gain - awb_result->gain.r;
	g_diff = awb_result->gain.g > af_cxt->cur_awb_g_gain ? awb_result->gain.g - af_cxt->cur_awb_g_gain : af_cxt->cur_awb_g_gain - awb_result->gain.g;
	b_diff = awb_result->gain.b > af_cxt->cur_awb_b_gain ? awb_result->gain.b - af_cxt->cur_awb_b_gain : af_cxt->cur_awb_b_gain - awb_result->gain.b;

	if ((r_diff <= af_cxt->rgbdiff_thr[0])
		&& (g_diff <= af_cxt->rgbdiff_thr[1])
		&& (b_diff <= af_cxt->rgbdiff_thr[2])) {
		af_cxt->awb_is_stab = 1;
	} else {
		af_cxt->awb_is_stab = 0;
	}
	af_cxt->cur_awb_r_gain = awb_result->gain.r;
	af_cxt->cur_awb_g_gain = awb_result->gain.g;
	af_cxt->cur_awb_b_gain = awb_result->gain.b;
	ISP_LOGI("awb_is_stab %d ",af_cxt->awb_is_stab);
*/
			break;
		}

	case AF_CMD_GET_AF_MODE:
		*(cmr_u32 *) param0 = af->request_mode;
		break;

	case AF_CMD_GET_AF_CUR_POS:
		*(cmr_u32 *) param0 = lens_get_pos(af);;
		break;
	case AF_CMD_SET_FACE_DETECT:
		{
			break;
#if 0
			struct isp_face_area *face = (struct isp_face_area *)param0;

			ISP_LOGV("face detect af state = %s", STATE_STRING(af->state));
			if (STATE_INACTIVE == af->state)
				return 0;

			ISP_LOGV("type = %d, face_num = %d", face->type, face->face_num);

			if (STATE_NORMAL_AF == af->state) {
				return 0;
			} else if (STATE_IDLE != af->state) {
				memcpy(&af->face_info, face, sizeof(struct isp_face_area));
				if (face_dectect_trigger(af)) {
					if (STATE_CAF == af->state || STATE_RECORD_CAF == af->state) {
						suspend_caf(af);
					} else if (STATE_FAF == af->state) {
						pthread_mutex_lock(&af->af_work_lock);
						AF_STOP(&af->fv, af->algo_mode);
						AF_Process_Frame(&af->fv);
						ISP_LOGV("AF_mode = %d", af->fv.AF_mode);
						pthread_mutex_unlock(&af->af_work_lock);
					}
					af->state = STATE_FAF;
					faf_start(af, NULL);
					ISP_LOGV("FAF Trigger");
				}
			}
			break;
#endif
		}
	case AF_CMD_SET_DCAM_TIMESTAMP:
		{
			struct isp_af_ts *af_ts = (struct isp_af_ts *)param0;
			if (0 == af_ts->capture) {
				af->dcam_timestamp = af_ts->timestamp;
				//ISP_LOGV("dcam_timestamp %lld ms", (cmr_s64) af->dcam_timestamp);
				if (DCAM_AFTER_VCM_YES == compare_timestamp(af) && 1 == af->vcm_stable) {
					sem_post(&af->af_wait_caf);
				}
			} else if (1 == af_ts->capture) {
				af->takepic_timestamp = af_ts->timestamp;
				//ISP_LOGV("takepic_timestamp %lld ms", (cmr_s64) af->takepic_timestamp);
				ISP_LOGV("takepic_timestamp - vcm_timestamp =%lld ms", ((cmr_s64) af->takepic_timestamp - (cmr_s64) af->vcm_timestamp) / 1000000);
			}
			break;
		}
	case AF_CMD_GET_AF_LIB_INFO:
		{
			//af_ctrl_t *af = ((isp_ctrl_context *)handle)->handle_af;
			char *version = (char *)param0;
			cmr_u32 len = *(cmr_u32 *) param1;
			memset(version, '\0', len);
			memcpy(version, "AF-", 3);
			if (len - 3 >= sizeof(af->fv.AF_Version)) {
				uint8 i = 0;
				memcpy(version + 3, af->fv.AF_Version, sizeof(af->fv.AF_Version));
				i = strlen((char *)af->fv.AF_Version) + 3;
				memcpy(version + i, "-20160927-15", 12);
			}
			break;
		}

	case AF_CMD_SET_PD_INFO:
		{
			struct pd_result *pd_calc_result = (struct pd_result *)param0;
			af->pd.pd_enable = (af->pd.effective_frmid) ? 1 : 0;
			af->pd.effective_frmid = (cmr_u32) pd_calc_result->pdGetFrameID;
			af->pd.confidence = (cmr_u32) pd_calc_result->pdConf[4];
			af->pd.pd_value = pd_calc_result->pdPhaseDiff[4];
			ISP_LOGI("[%d]PD_GetResult pd_calc_result.pdConf[4] = %d, pd_calc_result.pdPhaseDiff[4] = %lf", pd_calc_result->pdGetFrameID,
				pd_calc_result->pdConf[4], pd_calc_result->pdPhaseDiff[4]);
			ISP_LOGI("pdaf set callback end");
		}
		break;
	case AF_CMD_SET_UPDATE_AUX_SENSOR:
		rtn = af_sprd_adpt_update_aux_sensor(handle, param0);
		break;
	case AF_CMD_GET_AF_FULLSCAN_INFO:{
			cmr_u32 i = 0;
			struct isp_af_fullscan_info *af_fullscan_info = (struct isp_af_fullscan_info *)param0;

			while (i < sizeof(af->win_peak_pos) / sizeof(af->win_peak_pos[0])) {
				af->win_peak_pos[i] = af->fv.af_proc_data.scan_status.multi_pkpos[i];
				i++;
			}
			if (NULL != af_fullscan_info) {
				af_fullscan_info->row_num = 3;
				af_fullscan_info->column_num = 3;
				af_fullscan_info->win_peak_pos = af->win_peak_pos;
			}
			if (isp_ctx->otp_data) {
				if (isp_ctx->otp_data->single_otp.af_info.macro_cali > isp_ctx->otp_data->single_otp.af_info.infinite_cali) {
					af_fullscan_info->vcm_dac_low_bound = isp_ctx->otp_data->single_otp.af_info.infinite_cali;
					af_fullscan_info->vcm_dac_up_bound = isp_ctx->otp_data->single_otp.af_info.macro_cali;
				}
			} else {
				af_fullscan_info->vcm_dac_low_bound = 0;
				af_fullscan_info->vcm_dac_up_bound = 1023;
			}
			break;
		}
	default:
		ISP_LOGW("cmd not support! cmd: %d", cmd);
		rtn = AFV1_ERROR;
		break;

	}

	pthread_mutex_unlock(&af->status_lock);
	ISP_LOGV("E %ld", rtn);
	return rtn;

}

struct adpt_ops_type af_sprd_adpt_ops_ver1 = {
	.adpt_init = sprd_afv1_init,
	.adpt_deinit = sprd_afv1_deinit,
	.adpt_process = sprd_afv1_process,
	.adpt_ioctrl = sprd_afv1_ioctrl,
};

/*------------------------------------------------------------------------------*/
