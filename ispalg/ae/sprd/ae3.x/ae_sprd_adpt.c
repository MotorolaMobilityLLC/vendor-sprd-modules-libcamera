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
#define LOG_TAG "ae_sprd_adpt"
#define ATRACE_TAG (ATRACE_TAG_CAMERA | ATRACE_TAG_HAL)
#include <cutils/trace.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "inc/ae_sprd_adpt.h"
#include "ae_sprd_adpt_internal.h"
#include "ae_sprd_flash_calibration.h"
#include "ae/inc/ae_correction.h"
#include "ae/inc/ae_debug.h"
#include "ae_ctrl.h"
#include "flash/inc/flash.h"
#include "isp_debug.h"
#include "hdr/inc/sprd_hdr_api.h"
#ifdef WIN32
#include "stdio.h"
#include "ae_porint.h"
#else
#include <utils/Timers.h>
#include <cutils/properties.h>
#include <math.h>
#include <string.h>

#endif

#include "cmr_msg.h"
#include "inc/sensor_exposure_queue.h"
#include "isp_adpt.h"
#include "ae_debug_info_parser.h"
#include "dlfcn.h"

#ifdef ISP_LOGV
#undef ISP_LOGV
static cmr_int g_ae_log_level = LEVEL_OVER_LOGD;
static cmr_int g_ae_perf_log_level = LEVEL_OVER_LOGD;
extern long g_isp_log_level;
#define ISP_LOGV(format, ...)                                                  \
	ALOGD_IF((((g_ae_log_level >= LEVEL_OVER_LOGV)||(g_isp_log_level >= LEVEL_OVER_LOGV))&&(g_ae_log_level!=6)), DEBUG_STR format, DEBUG_ARGS, \
        ##__VA_ARGS__)
#define ISP_LOG_PERF(format, ...)                                                  \
	ALOGD_IF(g_ae_perf_log_level >= LEVEL_OVER_LOGV, DEBUG_STR format, DEBUG_ARGS, \
        ##__VA_ARGS__)

#endif

#define DUMP_AEM_DATA 0
#define AE_UPDATE_BASE_EOF 0
#define AE_UPDATE_BASE_SOF 0
#define AE_UPDAET_BASE_OFFSET AE_UPDATE_BASE_SOF
#define AE_TUNING_VER 1

#define AE_START_ID 0x71717567
#define AE_END_ID 	0x69656E64
#define AE_ADPT_CTRL_VER	"20190830-1354"

#define AE_EXP_GAIN_PARAM_FILE_NAME_CAMERASERVER "/data/vendor/cameraserver/ae.file"
#define AE_EXP_GAIN_PARAM_FILE_NAME_MEDIA "/data/misc/media/ae.file"
#define AE_SAVE_MLOG     "persist.vendor.cam.isp.ae.mlog"
#define AE_SAVE_MLOG_DEFAULT ""
#define SENSOR_LINETIME_BASE   100	/*temp macro for flash, remove later, Andy.lin */
#define AE_FLASH_CALC_TIMES	60	/* prevent flash_pfOneIteration time out */
#define AE_THREAD_QUEUE_NUM		(50)
const char AE_MAGIC_TAG_3_x[] = "ae_debug_info";

//Dynamic dualcam AE_sync debug
#define DYNAMIC_AE_SYNC  1 // 1:enable dynamic AE_sync; 0:disable dynamic AE_sync
#define AEM_MASTER_STAT_FILE "/data/vendor/cameraserver/aem_master.file"
#define AEM_SLAVE_STAT_FILE "/data/vendor/cameraserver/aem_slave.file"
#define AEM_Y_STAT_FILE "/data/vendor/cameraserver/aem_y.file"
#define EPSINON 0.00000001
#define FLASHMAIN_MAXCAP_60HZ 60000000
#define FLASHMAIN_MAXCAP_50HZ 50000000
#define FLASHMAIN_MINCAP 10000000
#define BLK_NUM_W_ALG 32
/**************************************************************************/

#define AE_PRINT_TIME \
	do {                                                       \
                    nsecs_t timestamp = systemTime(CLOCK_MONOTONIC);   \
                    ISP_LOGV("timestamp = %lld.", timestamp/1000000);  \
	} while(0)

#ifndef MAX
#define  MAX( _x, _y ) ( ((_x) > (_y)) ? (_x) : (_y) )
#define  MIN( _x, _y ) ( ((_x) < (_y)) ? (_x) : (_y) )
#endif

static float ae_get_real_gain(cmr_u32 gain);
static cmr_s32 ae_set_pause(struct ae_ctrl_cxt *cxt, int call);
static cmr_s32 ae_set_restore_cnt(struct ae_ctrl_cxt *cxt, int call);
static cmr_s32 ae_set_force_pause(struct ae_ctrl_cxt *cxt, cmr_u32 enable, int call);
static cmr_s32 ae_set_force_pause_flash(struct ae_ctrl_cxt *cxt, cmr_u32 enable);
static cmr_s32 ae_set_skip_update(struct ae_ctrl_cxt *cxt);
static cmr_s32 ae_set_restore_skip_update_cnt(struct ae_ctrl_cxt *cxt);
static cmr_s32 ae_round(float a);
static cmr_s32 ae_io_ctrl_direct(cmr_handle handle, cmr_s32 cmd, cmr_handle param, cmr_handle result);
static cmr_s32 ae_io_ctrl_sync(cmr_handle handle, cmr_s32 cmd, cmr_handle param, cmr_handle result);
static cmr_s32 ae_set_exposure_compensation(struct ae_ctrl_cxt *cxt, struct ae_exp_compensation *exp_comp);
/**---------------------------------------------------------------------------*
** 				Local Function Prototypes				*
**---------------------------------------------------------------------------*/
static cmr_u32 ae_is_equal (double a, double b)
{
	cmr_u32 is_equal = 0;

	if (fabs(a - b) <= EPSINON) {
		is_equal = 1;
	}

	return is_equal;
}

static void ae_parse_isp_gain(struct ae_ctrl_cxt *cxt, cmr_u32 is_master, cmr_u32 gain, cmr_u32 *snr_gain, cmr_u32 *dcam_gain)
{
	float sensor_gain = 0.0;
	float isp_gain = 0.0;
	struct sensor_info info_slave0;
	struct sensor_info info_slave1;
	struct sensor_info info_master;
	cmr_u32 max_gain = cxt->sensor_max_gain;
	cmr_u32 min_gain = cxt->sensor_min_gain;
	UNUSED(is_master);

	if ((CAM_SENSOR_MASTER == cxt->sensor_role) && cxt->is_multi_mode){
		cxt->ptr_isp_br_ioctrl(CAM_SENSOR_MASTER, GET_MODULE_INFO, NULL, &info_master);
		max_gain = info_master.max_again;
		min_gain = info_master.min_again;
		ISP_LOGV("sync:master,max_gain:%d,min_gain:%d", max_gain, min_gain);
	}else if ((CAM_SENSOR_SLAVE0 == cxt->sensor_role)  && cxt->is_multi_mode){
		cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE0, GET_MODULE_INFO, NULL, &info_slave0);
		max_gain = info_slave0.max_again;
		min_gain = info_slave0.min_again;
		ISP_LOGV("sync:slave0,max_gain:%d,min_gain:%d", max_gain, min_gain);
	}else if ((CAM_SENSOR_SLAVE1 == cxt->sensor_role)  && cxt->is_multi_mode){
		cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE1, GET_MODULE_INFO, NULL, &info_slave1);
		max_gain = info_slave1.max_again;
		min_gain = info_slave1.min_again;
		ISP_LOGV("sync:slave1,max_gain:%d,min_gain:%d", max_gain, min_gain);
	}

	if (gain > max_gain) {	/*gain : (sensor max gain, ~) */
		sensor_gain = max_gain;
		isp_gain = (double)gain / (double)max_gain;
	} else if (gain > min_gain) {	/*gain : (sensor_min_gain, sensor_max_gain) */
		if (0 == gain % cxt->sensor_gain_precision) {
			sensor_gain = gain;
			isp_gain = 1.0;
		} else {
			sensor_gain = (gain / cxt->sensor_gain_precision) * cxt->sensor_gain_precision;
			isp_gain = gain * 1.0 / sensor_gain;
		}
	} else {					/*gain : (~, sensor_min_gain) */
		sensor_gain = min_gain;
		isp_gain = (double)gain / (double)min_gain;
		if (isp_gain < 1.0) {
			ISP_LOGW("check sensor_cfg.min_gain %.2f %.2f", sensor_gain, isp_gain);
			isp_gain = 1.0;
		}
	}

	if(snr_gain){
		*snr_gain = sensor_gain;
		ISP_LOGV("snr_gain:%d", *snr_gain);
	}
	if(dcam_gain){
                *dcam_gain = (isp_gain * 4096.0 + 0.5);
                ISP_LOGV("dcam_gain:%d", *dcam_gain);
	}
        ISP_LOGV("sync:sensor_role:%d", cxt->sensor_role);

}

static cmr_s32 ae_update_exp_data(struct ae_ctrl_cxt *cxt, struct ae_sensor_exp_data *exp_data, struct q_item *write_item, struct q_item *actual_item, cmr_u32 is_force)
{
	ae_parse_isp_gain(cxt, cxt->is_master, exp_data->lib_data.gain, &exp_data->lib_data.sensor_gain, &exp_data->lib_data.isp_gain);

	ISP_LOGV("sync:ae gain: cameraId = %d, sensor_role = %d, is_master= %d, %d = %d * (%d/4096)\n",
			cxt->camera_id,
			cxt->sensor_role,
			cxt->is_master,
			exp_data->lib_data.gain,
			exp_data->lib_data.sensor_gain,
			exp_data->lib_data.isp_gain);

	if (is_force) {
		/**/ struct s_q_init_in init_in = {0, 0, 0, 0, 0, 0, 0};
		struct s_q_init_out init_out = {{0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0}};

		init_in.exp_line = exp_data->lib_data.exp_line;
		init_in.exp_time = exp_data->lib_data.exp_time;
		init_in.dmy_line = exp_data->lib_data.dummy;
		init_in.frm_len = exp_data->lib_data.frm_len;
		//init_in.frm_len_def = exp_data->lib_data.frm_len_def;
		init_in.sensor_gain = exp_data->lib_data.sensor_gain;
		init_in.isp_gain = exp_data->lib_data.isp_gain;

		s_q_init(cxt->seq_handle, &init_in, &init_out);

		//*write_item = init_out.write_item;
		write_item->exp_line = init_in.exp_line;
		write_item->exp_time = init_in.exp_time;
		write_item->sensor_gain = init_in.sensor_gain;
		write_item->isp_gain = init_in.isp_gain;
		write_item->dumy_line = init_in.dmy_line;
		write_item->frm_len = init_in.frm_len;
		//write_item->frm_len_def = init_in.frm_len_def;
		*actual_item = *write_item;
	} else {
		struct q_item input_item;

		input_item.exp_line = exp_data->lib_data.exp_line;
		input_item.exp_time = exp_data->lib_data.exp_time;
		input_item.dumy_line = exp_data->lib_data.dummy;
		input_item.frm_len = exp_data->lib_data.frm_len;
		//input_item.frm_len_def= exp_data->lib_data.frm_len_def;
		input_item.isp_gain = exp_data->lib_data.isp_gain;
		input_item.sensor_gain = exp_data->lib_data.sensor_gain;

		ISP_LOGV("sync:ae_update_exp_data: sensor_role:%d, exp_line:%d, exp_time:%d,isp_gain:%d, sensor_gain:%d",\
			cxt->sensor_role, input_item.exp_line, input_item.exp_time, input_item.isp_gain, input_item.sensor_gain);

		s_q_put(cxt->seq_handle, &input_item, write_item, actual_item);
	}

	ISP_LOGV("sync:sensor_role:%d, is_master:%d, type: %d, lib_out:(exp_line:%d, dummy:%d, sensor_gain:%d, isp_gain:%d, exp_time:%d)--write: (exp_line:%d, dumy_line:%d, sensor_gain:%d, isp_gain:%d, exp_time:%d)--actual: (%d, %d, %d, %d, %d)\n",
			 cxt->sensor_role, cxt->is_master, is_force, exp_data->lib_data.exp_line, exp_data->lib_data.dummy, exp_data->lib_data.sensor_gain, exp_data->lib_data.isp_gain, exp_data->lib_data.exp_time,\
			 write_item->exp_line, write_item->dumy_line, write_item->sensor_gain, write_item->isp_gain, write_item->exp_time,\
			 actual_item->exp_line, actual_item->dumy_line, actual_item->sensor_gain, actual_item->isp_gain, actual_item->exp_time);

	return ISP_SUCCESS;

}

static cmr_s32 ae_write_to_sensor(struct ae_ctrl_cxt *cxt, struct ae_exposure_param *write_param_ptr)
{
	struct ae_exposure_param tmp_param = *write_param_ptr;
	struct ae_exposure_param *write_param = &tmp_param;
	struct ae_exposure_param *prv_param = &cxt->exp_data.write_data;

	if ((cxt->zsl_flag == 0) && (cxt->is_snapshot) && (cxt->binning_factor_cap != cxt->binning_factor_prev)) {
		if(!cxt->binning_factor_prev)
			cxt->binning_factor_prev = 128;

		tmp_param.exp_time = (cmr_u32) (1.0 * tmp_param.exp_time * cxt->binning_factor_cap / cxt->binning_factor_prev + 0.5);
		tmp_param.exp_line = (cmr_u32) (1.0 * tmp_param.exp_line * cxt->binning_factor_cap / cxt->binning_factor_prev + 0.5);
	}
	ISP_LOGV("exp_line %d, binning_factor %d / %d, zsl_flag %d", tmp_param.exp_line, cxt->binning_factor_cap, cxt->binning_factor_prev,cxt->zsl_flag);

	if (0 != write_param->exp_line) {
		struct ae_exposure exp;
		cmr_s32 size_index = cxt->snr_info.sensor_size_index;
		if (cxt->isp_ops.ex_set_exposure) {
			memset(&exp, 0, sizeof(exp));
			exp.exposure = write_param->exp_line;
			exp.dummy = write_param->dummy;
			exp.size_index = size_index;
			exp.exp_time = write_param->exp_time;
			if ((write_param->exp_line != prv_param->exp_line)
				|| (write_param->dummy != prv_param->dummy)) {
				(*cxt->isp_ops.ex_set_exposure) (cxt->isp_ops.isp_handler, &exp);
				cmr_u64 exp_time = 0;
				cmr_int cb_type;
				exp_time = (cmr_u64) write_param->exp_time;
				cb_type = AE_CB_EXPTIME_NOTIFY;
				if(NULL != cxt->isp_ops.callback){
					(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, cb_type, &exp_time);
				}else{
					ISP_LOGE("isp_ops.callback is null");
				}
			} else {
				ISP_LOGV("no_need_write exp");
				;
			}
		} else if (cxt->isp_ops.set_exposure) {
			cmr_u32 ae_expline = write_param->exp_line;
			memset(&exp, 0, sizeof(exp));
			ae_expline = ae_expline & 0x0000ffff;
			ae_expline |= ((write_param->dummy << 0x10) & 0x0fff0000);
			ae_expline |= ((size_index << 0x1c) & 0xf0000000);
			exp.exposure = ae_expline;
			if ((write_param->exp_line != prv_param->exp_line)
				|| (write_param->dummy != prv_param->dummy)) {
				(*cxt->isp_ops.set_exposure) (cxt->isp_ops.isp_handler, &exp);
			} else {
				ISP_LOGV("no_need_write exp");
				;
			}
		}
	} else {
		ISP_LOGE("fail to write exp %d", write_param->exp_line);
	}
	if (0 != write_param->sensor_gain) {
		struct ae_gain gain;
		if (cxt->isp_ops.set_again) {
			memset(&gain, 0, sizeof(gain));
			gain.gain = write_param->sensor_gain & 0xffff;
			if (prv_param->sensor_gain != write_param->sensor_gain) {
				(*cxt->isp_ops.set_again) (cxt->isp_ops.isp_handler, &gain);
			} else {
				ISP_LOGV("no_need_write gain");
				;
			}
		}
	} else {
		ISP_LOGE("fail to write aegain %d", write_param->sensor_gain);
	}
	if (0 != write_param->isp_gain) {
		double rgb_coeff = write_param->isp_gain * 1.0 / 4096;
		if (cxt->isp_ops.set_rgb_gain) {
			cxt->isp_ops.set_rgb_gain(cxt->isp_ops.isp_handler, rgb_coeff);
		}
	}
	return ISP_SUCCESS;
}

static void _aem_stat_preprocess_dulpslave(struct aem_info *slave_aem_info, cmr_u32 * img_stat, cmr_u32 * dst_img_stat)
{
	cmr_u32 i,j,ii,jj = 0;
	cmr_u32 tmp_r = 0,tmp_g = 0,tmp_b = 0;
	cmr_u32 bayer_pixels = slave_aem_info->aem_stat_blk_pixels/4;
	cmr_u32 blk_num_w = slave_aem_info->aem_stat_win_w;
	cmr_u32 blk_num_h = slave_aem_info->aem_stat_win_h;
	cmr_u32 *src_aem_stat = img_stat;
	cmr_u32 *r_stat = (cmr_u32*)src_aem_stat;
	cmr_u32 *g_stat = (cmr_u32*)src_aem_stat + 16384;
	cmr_u32 *b_stat = (cmr_u32*)src_aem_stat + 2 * 16384;

	ISP_LOGV("win_num=[%d x %d], bayer_pixels %d",blk_num_w,blk_num_h,bayer_pixels);

	blk_num_w = (blk_num_w < 32) ? 32:blk_num_w;
	blk_num_h = (blk_num_h < 32) ? 32:blk_num_h;

	cmr_u32 ratio_w = blk_num_w/32;
	cmr_u32 ratio_h = blk_num_h/32;

	for(i = 0; i < BLK_NUM_W_ALG; i++){
		for(j = 0; j < BLK_NUM_W_ALG; j++){
			for(ii = 0; ii < ratio_w; ii++){
				for(jj = 0; jj < ratio_h; jj++){
					cmr_u32 idx = i * ratio_w * ratio_h * BLK_NUM_W_ALG + j * ratio_w + ii * ratio_h * BLK_NUM_W_ALG + jj;
					tmp_r += r_stat[idx]/bayer_pixels/(ratio_w * ratio_h);
					tmp_g += g_stat[idx]/bayer_pixels/(ratio_w * ratio_h);
					tmp_b += b_stat[idx]/bayer_pixels/(ratio_w * ratio_h);
				}
			}
			dst_img_stat[i * BLK_NUM_W_ALG + j] = (tmp_r>>2);
			dst_img_stat[i * BLK_NUM_W_ALG + j + 1024] = (tmp_g>>2);
			dst_img_stat[i * BLK_NUM_W_ALG + j + 2048] = (tmp_b>>2);
			tmp_r = tmp_g = tmp_b = 0;
		}
	}
}

static cmr_s32 ae_sync_in_ae_lib_data_dump(struct ae_frm_sync_param *sync_param_in)
{
	ISP_LOGV("sync:---------to lib data dump start---------");
	ISP_LOGV("sync:to lib data dump, is_benchmark:%d",sync_param_in->is_benchmark);
	#if 0
	ISP_LOGD("sync:to lib data dump, min_exp_line:%d",sync_param_in->min_exp_line);
	ISP_LOGD("sync:to lib data dump, max_exp_line:%d",sync_param_in->max_exp_line);
	ISP_LOGD("sync:to lib data dump, min_gain:%d",sync_param_in->min_gain);
	ISP_LOGD("sync:to lib data dump, max_gain:%d",sync_param_in->max_gain);
	ISP_LOGD("sync:to lib data dump, sensor_gain_precision:%d",sync_param_in->sensor_gain_precision);
	ISP_LOGD("sync:to lib data dump, frm_len_def:%d",sync_param_in->frm_len_def);
	ISP_LOGD("sync:to lib data dump, otp:gain_1x_exp:%d",(int)sync_param_in->otp_info.gain_1x_exp);
	ISP_LOGD("sync:to lib data dump, otp:gain_2x_exp:%d",(int)sync_param_in->otp_info.gain_2x_exp);
	ISP_LOGD("sync:to lib data dump, otp:gain_4x_exp:%d",(int)sync_param_in->otp_info.gain_4x_exp);
	ISP_LOGD("sync:to lib data dump, otp:gain_8x_exp:%d",(int)sync_param_in->otp_info.gain_8x_exp);
	#endif
	ISP_LOGV("sync:to lib data dump, roi_rect:(start_x:%d, end_x:%d)",sync_param_in->aem_roi_rect.start_x, sync_param_in->aem_roi_rect.end_x);
	ISP_LOGV("sync:to lib data dump, roi_rect:(start_y:%d, end_y:%d)",sync_param_in->aem_roi_rect.start_y, sync_param_in->aem_roi_rect.end_y);
	ISP_LOGV("sync:to lib data dump, image_size:(w:%d, h:%d)",sync_param_in->img_size.w, sync_param_in->img_size.h);
	ISP_LOGV("sync:to lib data dump, block_size:(w:%d, h:%d)",sync_param_in->blk_size.w, sync_param_in->blk_size.h);
	ISP_LOGV("sync:to lib data dump, block_rect:(st_x:%d, st_y:%d)",sync_param_in->block_rect.x, sync_param_in->block_rect.y);
	ISP_LOGV("sync:to lib data dump, ev_setting_lib_out:line_time:%d",sync_param_in->ev_setting.line_time);
	ISP_LOGV("sync:to lib data dump, ev_setting_lib_out:exp_time:%d",sync_param_in->ev_setting.exp_time);
	ISP_LOGV("sync:to lib data dump, ev_setting_lib_out:exp_line:%d",sync_param_in->ev_setting.exp_line);
	ISP_LOGV("sync:to lib data dump, ev_setting_lib_out:dmy_line:%d",sync_param_in->ev_setting.dmy_line);
	ISP_LOGV("sync:to lib data dump, ev_setting_lib_out:frm_len:%d",sync_param_in->ev_setting.frm_len);
	ISP_LOGV("sync:to lib data dump, ev_setting_lib_out:ae_gain:%d",sync_param_in->ev_setting.ae_gain);
	ISP_LOGV("sync:to lib data dump, effect_param:line_time:%d",sync_param_in->effect_param.line_time);
	ISP_LOGV("sync:to lib data dump, effect_param:exp_time:%d",sync_param_in->effect_param.exp_time);
	ISP_LOGV("sync:to lib data dump, effect_param:exp_line:%d",sync_param_in->effect_param.exp_line);
	ISP_LOGV("sync:to lib data dump, effect_param:dmy_line:%d",sync_param_in->effect_param.dmy_line);
	ISP_LOGV("sync:to lib data dump, effect_param:frm_len:%d",sync_param_in->effect_param.frm_len);
	ISP_LOGV("sync:to lib data dump, effect_param:ae_gain:%d",sync_param_in->effect_param.ae_gain);
	ISP_LOGV("sync:---------to lib data dump end---------");
	return ISP_SUCCESS;
}

static cmr_s32 ae_sync_write_to_sensor_normal(struct ae_ctrl_cxt *cxt, struct ae_exposure_param *write_param)
{
	ISP_LOGV("sync:-----ae_sync_write_to_sensor_normal------sensor_role:%d---start----", cxt->sensor_role);
	cmr_u32 rtn;
	//struct ae_exposure_param *prv_param = &cxt->exp_data.write_data;
	struct sensor_multi_ae_info ae_info[3];
	struct sensor_info info_master = {0};
	struct sensor_info info_slave[2] = {{0}, {0}};
	struct ae_sync_data sync_info_master = {0};
	struct ae_sync_data sync_info_slave[2] = {{0},{0}};
	struct ae_match_data ae_data_slave[2] = {{0}, {0}};
	struct ae_match_data ae_match_data_master = {0};
	struct ae_match_data ae_match_data_slave[2] = {{0},{0}};
	struct sensor_otp_ae_info ae_otp_master = {0};
	struct sensor_otp_ae_info ae_otp_slave[2] = {{0},{0}};
	struct ae_lib_frm_sync_out out_param_parse = {0};
	struct ae_lib_frm_sync_out out_param_lib = {0};
	struct ae_lib_frm_sync_in in_param = {0};
	in_param.sync_param[0] = &cxt->sync_param[0];
	in_param.sync_param[1] = &cxt->sync_param[1];
	in_param.sync_param[2] = &cxt->sync_param[2];
	in_param.sync_param[3] = &cxt->sync_param[3];

	struct ae_frm_sync_param *master_ae_sync_info_ptr = in_param.sync_param[0];
	struct ae_frm_sync_param *slave0_ae_sync_info_ptr = in_param.sync_param[1];
	struct ae_frm_sync_param *slave1_ae_sync_info_ptr = in_param.sync_param[2];
	struct ae_match_stats_data stats_data_master = {0};
	struct ae_match_stats_data stats_data_slave[2] = {{0},{0}};
	struct aem_info slave_aem_info[2] = {{0},{0}};
	cmr_u32 dcam_gain[3] = {0, 0, 0};

	memset(&ae_info, 0, sizeof(ae_info));

	if (0 != write_param->exp_line && 0 != write_param->sensor_gain) {
		cmr_s32 size_index = cxt->snr_info.sensor_size_index;

		ae_info[0].exp.exposure = write_param->exp_line;
		ae_info[0].exp.dummy = write_param->dummy;
		ae_info[0].exp.size_index = size_index;
		ae_info[0].gain = write_param->sensor_gain & 0xffff;

		cxt->ptr_isp_br_ioctrl(CAM_SENSOR_MASTER, GET_MODULE_INFO, NULL, &info_master);
		cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE0, GET_MODULE_INFO, NULL, &info_slave[0]);
		cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE1, GET_MODULE_INFO, NULL, &info_slave[1]);
		cxt->ptr_isp_br_ioctrl(CAM_SENSOR_MASTER, GET_OTP_AE, NULL, &ae_otp_master);
		cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE0, GET_OTP_AE, NULL, &ae_otp_slave[0]);
		cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE1, GET_OTP_AE, NULL, &ae_otp_slave[1]);
		cxt->ptr_isp_br_ioctrl(CAM_SENSOR_MASTER, GET_AE_SYNC_DATA, NULL, &sync_info_master);
		cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE0, GET_AE_SYNC_DATA, NULL, &sync_info_slave[0]);
		cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE1, GET_AE_SYNC_DATA, NULL, &sync_info_slave[1]);
		cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE0, GET_MATCH_AE_DATA, NULL, &ae_data_slave[0]);
		cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE1, GET_MATCH_AE_DATA, NULL, &ae_data_slave[1]);

		ISP_LOGV("sync:normal mode:master linetime %d", info_master.line_time);
		ISP_LOGV("sync:normal mode:slave0 linetime %d", info_slave[0].line_time);
		ISP_LOGV("sync:normal mode:slave1 linetime %d", info_slave[1].line_time);
		ISP_LOGV("sync:work sensor num:%d, refcameraID:%d", sync_info_master.num, sync_info_master.ref_camera_id);

		if (ae_info[0].exp.exposure < (cmr_u32) info_master.min_exp_line) {
			ae_info[0].exp.exposure = (cmr_u32) info_master.min_exp_line;
		}

		memset(master_ae_sync_info_ptr,0,sizeof(struct ae_frm_sync_param));
		memset(slave0_ae_sync_info_ptr,0,sizeof(struct ae_frm_sync_param));
		memset(slave1_ae_sync_info_ptr,0,sizeof(struct ae_frm_sync_param));

		if(cxt->is_multi_mode) {
			stats_data_master.len = sizeof(master_ae_sync_info_ptr->aem);
			stats_data_master.stats_data = &master_ae_sync_info_ptr->aem[0];
			stats_data_slave[0].len = sizeof(slave0_ae_sync_info_ptr->aem);
			stats_data_slave[0].stats_data = &slave0_ae_sync_info_ptr->aem[0];
			stats_data_slave[1].len = sizeof(slave1_ae_sync_info_ptr->aem);
			stats_data_slave[1].stats_data = &slave1_ae_sync_info_ptr->aem[0];
			for(cmr_u32 i = 0; i < 3 * 1024; i++) {
				stats_data_master.stats_data[i] = cxt->sync_aem[i]>>2;
			}

			rtn = cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE0, GET_SLAVE_AEM_INFO, NULL, &slave_aem_info[0]);
			if(rtn){
				ISP_LOGE("GET_SLAVE0_AEM_INFO rtn = %d",rtn);
			}

			rtn = cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE1, GET_SLAVE_AEM_INFO, NULL, &slave_aem_info[1]);
			if(rtn){
				ISP_LOGE("GET_SLAVE1_AEM_INFO rtn = %d",rtn);
			}

			rtn = cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE0, GET_STAT_AWB_DATA_AE, NULL, &cxt->slave0_aem_stat);
			if(rtn){
				ISP_LOGE("norma Dynamic AE_sync y_ratio slave0_sync_aem is NULL error!");
			}

			rtn = cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE1, GET_STAT_AWB_DATA_AE, NULL, &cxt->slave1_aem_stat);
			if(rtn){
				ISP_LOGE("norma Dynamic AE_sync y_ratio slave1_sync_aem is NULL error!");
			}

			if(slave_aem_info[0].aem_stat_blk_pixels)
				_aem_stat_preprocess_dulpslave(&slave_aem_info[0], cxt->slave0_aem_stat, stats_data_slave[0].stats_data);
			else
				ISP_LOGE("sync:aem_stat_blk_pixels null");

			if (cxt->is_multi_mode ==ISP_ALG_TRIBLE_W_T_UW) {
				if(slave_aem_info[1].aem_stat_blk_pixels)
					_aem_stat_preprocess_dulpslave(&slave_aem_info[1], cxt->slave1_aem_stat, stats_data_slave[1].stats_data);
				else {
					ISP_LOGE("sync:sensor_role:%d,is_multi_mode:%d, slave1 aem_stat_blk_pixels null", cxt->sensor_role, cxt->is_multi_mode);	
				}
			}
		}

		ISP_LOGV("sync:ae_sync_write_to_sensor_normal, write_param, exp_line:%d, ae_gain:%d, dmy_line:%d, exp_time:%d",\
		write_param->exp_line, write_param->sensor_gain * write_param->isp_gain /4096,\
		write_param->dummy, write_param->exp_time);

		master_ae_sync_info_ptr->monoboottime = stats_data_master.monoboottime;
		master_ae_sync_info_ptr->max_gain = info_master.max_again;
		master_ae_sync_info_ptr->min_gain = info_master.min_again;
		master_ae_sync_info_ptr->min_exp_line =info_master.min_exp_line;
		master_ae_sync_info_ptr->sensor_gain_precision = info_master.sensor_gain_precision;
		master_ae_sync_info_ptr->ev_setting.line_time = info_master.line_time;
		master_ae_sync_info_ptr->ev_setting.exp_line = write_param->exp_line;
		master_ae_sync_info_ptr->ev_setting.ae_gain =  write_param->sensor_gain * write_param->isp_gain /4096;
		master_ae_sync_info_ptr->ev_setting.dmy_line = write_param->dummy;
		master_ae_sync_info_ptr->ev_setting.exp_time = write_param->exp_time;
		master_ae_sync_info_ptr->ev_setting.frm_len = cxt->cur_result.ev_setting.frm_len;
		master_ae_sync_info_ptr->frm_len_def = info_master.frm_len_def;
		master_ae_sync_info_ptr->otp_info.gain_1x_exp = ae_otp_master.gain_1x_exp;
		master_ae_sync_info_ptr->otp_info.gain_2x_exp = ae_otp_master.gain_2x_exp;
		master_ae_sync_info_ptr->otp_info.gain_4x_exp = ae_otp_master.gain_4x_exp;
		master_ae_sync_info_ptr->otp_info.gain_8x_exp = ae_otp_master.gain_8x_exp;
		master_ae_sync_info_ptr->otp_info.ae_target_lum = ae_otp_master.ae_target_lum;
		master_ae_sync_info_ptr->otp_info.reserve = ae_otp_master.reserve;
		memcpy(&master_ae_sync_info_ptr->blk_size, &cxt->monitor_cfg.blk_size, sizeof(struct ae_size));
		memcpy(&master_ae_sync_info_ptr->img_size,&cxt->cur_status.img_size, sizeof(struct ae_size));
		memcpy(&master_ae_sync_info_ptr->aem_roi_rect, &sync_info_master.target_rect, sizeof(struct ae_rect));
		master_ae_sync_info_ptr->is_benchmark = sync_info_master.ref_camera_id;
		master_ae_sync_info_ptr->blks_num.w = BLK_NUM_W_ALG;
		master_ae_sync_info_ptr->blks_num.h = BLK_NUM_W_ALG;

		slave0_ae_sync_info_ptr->monoboottime = stats_data_slave[0].monoboottime;
		slave0_ae_sync_info_ptr->ev_setting.line_time = info_slave[0].line_time;
		slave0_ae_sync_info_ptr->max_gain = info_slave[0].max_again;
		slave0_ae_sync_info_ptr->min_gain = info_slave[0].min_again;
		slave0_ae_sync_info_ptr->min_exp_line =info_slave[0].min_exp_line;
		slave0_ae_sync_info_ptr->sensor_gain_precision = info_slave[0].sensor_gain_precision;
		slave0_ae_sync_info_ptr->frm_len_def = info_slave[0].frm_len_def;
		slave0_ae_sync_info_ptr->otp_info.gain_1x_exp = ae_otp_slave[0].gain_1x_exp;
		slave0_ae_sync_info_ptr->otp_info.gain_2x_exp = ae_otp_slave[0].gain_2x_exp;
		slave0_ae_sync_info_ptr->otp_info.gain_4x_exp = ae_otp_slave[0].gain_4x_exp;
		slave0_ae_sync_info_ptr->otp_info.gain_8x_exp = ae_otp_slave[0].gain_8x_exp;
		slave0_ae_sync_info_ptr->otp_info.ae_target_lum = ae_otp_slave[0].ae_target_lum;
		slave0_ae_sync_info_ptr->otp_info.reserve = ae_otp_slave[0].reserve;
		memcpy(&slave0_ae_sync_info_ptr->blk_size, &cxt->monitor_cfg.blk_size, sizeof(struct ae_size));
		memcpy(&slave0_ae_sync_info_ptr->img_size,&cxt->cur_status.img_size, sizeof(struct ae_size));
		memcpy(&slave0_ae_sync_info_ptr->aem_roi_rect, &sync_info_slave[0].target_rect, sizeof(struct ae_rect));
		slave0_ae_sync_info_ptr->is_benchmark = sync_info_slave[0].ref_camera_id;
		slave0_ae_sync_info_ptr->blks_num.w = BLK_NUM_W_ALG;
		slave0_ae_sync_info_ptr->blks_num.h = BLK_NUM_W_ALG;
		slave0_ae_sync_info_ptr->ev_setting.exp_line = ae_data_slave[0].exp.exposure;
		slave0_ae_sync_info_ptr->ev_setting.ae_gain =  ae_data_slave[0].gain * ae_data_slave[0].isp_gain /4096;
		slave0_ae_sync_info_ptr->ev_setting.dmy_line = ae_data_slave[0].exp.dummy;
		slave0_ae_sync_info_ptr->ev_setting.exp_time = ae_data_slave[0].exp.exposure*info_slave[0].line_time ;


		if (cxt->is_multi_mode ==ISP_ALG_TRIBLE_W_T_UW) {

			slave1_ae_sync_info_ptr->monoboottime = stats_data_slave[1].monoboottime;
			slave1_ae_sync_info_ptr->ev_setting.line_time = info_slave[1].line_time;
			slave1_ae_sync_info_ptr->max_gain = info_slave[1].max_again;
			slave1_ae_sync_info_ptr->min_gain = info_slave[1].min_again;
			slave1_ae_sync_info_ptr->min_exp_line =info_slave[1].min_exp_line;
			slave1_ae_sync_info_ptr->sensor_gain_precision = info_slave[1].sensor_gain_precision;
			slave1_ae_sync_info_ptr->frm_len_def = info_slave[1].frm_len_def;
			slave1_ae_sync_info_ptr->otp_info.gain_1x_exp = ae_otp_slave[1].gain_1x_exp;
			slave1_ae_sync_info_ptr->otp_info.gain_2x_exp = ae_otp_slave[1].gain_2x_exp;
			slave1_ae_sync_info_ptr->otp_info.gain_4x_exp = ae_otp_slave[1].gain_4x_exp;
			slave1_ae_sync_info_ptr->otp_info.gain_8x_exp = ae_otp_slave[1].gain_8x_exp;
			slave1_ae_sync_info_ptr->otp_info.ae_target_lum = ae_otp_slave[1].ae_target_lum;
			slave1_ae_sync_info_ptr->otp_info.reserve = ae_otp_slave[1].reserve;
			memcpy(&slave1_ae_sync_info_ptr->blk_size, &cxt->monitor_cfg.blk_size, sizeof(struct ae_size));
			memcpy(&slave1_ae_sync_info_ptr->img_size,&cxt->cur_status.img_size, sizeof(struct ae_size));
			memcpy(&slave1_ae_sync_info_ptr->aem_roi_rect, &sync_info_slave[1].target_rect, sizeof(struct ae_rect));
			slave1_ae_sync_info_ptr->blks_num.w = BLK_NUM_W_ALG;
			slave1_ae_sync_info_ptr->blks_num.h = BLK_NUM_W_ALG;
			slave1_ae_sync_info_ptr->is_benchmark = sync_info_slave[1].ref_camera_id;
			slave1_ae_sync_info_ptr->ev_setting.exp_line = ae_data_slave[1].exp.exposure;
			slave1_ae_sync_info_ptr->ev_setting.ae_gain =  ae_data_slave[1].gain * ae_data_slave[1].isp_gain /4096;
			slave1_ae_sync_info_ptr->ev_setting.dmy_line = ae_data_slave[1].exp.dummy;
			slave1_ae_sync_info_ptr->ev_setting.exp_time = ae_data_slave[1].exp.exposure*info_slave[1].line_time;

			#if 0
			if (3 == sync_info_master.ref_camera_id) {
				in_param.sync_param[0] = slave1_ae_sync_info_ptr;
				in_param.sync_param[2] = master_ae_sync_info_ptr;
			} else if (2 == sync_info_master.ref_camera_id) {
				in_param.sync_param[0] = slave0_ae_sync_info_ptr;
				in_param.sync_param[1] = master_ae_sync_info_ptr;
			}
			#endif

			in_param.mode = AE_SYNC_1;
			in_param.num = sync_info_master.num;
			ae_sync_in_ae_lib_data_dump(in_param.sync_param[0]);
			ae_sync_in_ae_lib_data_dump(in_param.sync_param[1]);
			ae_sync_in_ae_lib_data_dump(in_param.sync_param[2]);

			if ((info_slave[0].line_time != 0) && (info_slave[1].line_time != 0)) {
				ISP_LOGV("sync:all sensor work");
				ae_lib_frame_sync_calculation(cxt->misc_handle, &in_param, &out_param_lib);

				ISP_LOGV("sync:0 lib out ae_gain:%d, exp_time:%d, exp_line:%d,line_time:%d",
				        out_param_lib.ev_setting[0].ae_gain, out_param_lib.ev_setting[0].exp_time,
				        out_param_lib.ev_setting[0].exp_line,out_param_lib.ev_setting[0].line_time );
				ISP_LOGV("sync:1 lib out ae_gain:%d, exp_time:%d, exp_line:%d,line_time:%d",
				        out_param_lib.ev_setting[1].ae_gain, out_param_lib.ev_setting[1].exp_time,
				        out_param_lib.ev_setting[1].exp_line,out_param_lib.ev_setting[1].line_time );
				ISP_LOGV("sync:2 lib out ae_gain:%d, exp_time:%d, exp_line:%d,line_time:%d",
				        out_param_lib.ev_setting[2].ae_gain, out_param_lib.ev_setting[2].exp_time,
				        out_param_lib.ev_setting[2].exp_line,out_param_lib.ev_setting[2].line_time );
				#if 0
				if (0 == sync_info_master.ref_camera_id) {
					memcpy(&out_param_parse.ev_setting[0], &out_param_lib.ev_setting[0], sizeof(struct ae_ev_setting_param));
					memcpy(&out_param_parse.ev_setting[1], &out_param_lib.ev_setting[1], sizeof(struct ae_ev_setting_param));
					memcpy(&out_param_parse.ev_setting[2], &out_param_lib.ev_setting[2], sizeof(struct ae_ev_setting_param));
				}else if (2 == sync_info_master.ref_camera_id) {
					memcpy(&out_param_parse.ev_setting[0], &out_param_lib.ev_setting[1], sizeof(struct ae_ev_setting_param));
					memcpy(&out_param_parse.ev_setting[1], &out_param_lib.ev_setting[0], sizeof(struct ae_ev_setting_param));
					memcpy(&out_param_parse.ev_setting[2], &out_param_lib.ev_setting[2], sizeof(struct ae_ev_setting_param));
				}else {         //(3 == sync_info_master.ref_camera_id)
					memcpy(&out_param_parse.ev_setting[0], &out_param_lib.ev_setting[2], sizeof(struct ae_ev_setting_param));
					memcpy(&out_param_parse.ev_setting[1], &out_param_lib.ev_setting[1], sizeof(struct ae_ev_setting_param));
					memcpy(&out_param_parse.ev_setting[2], &out_param_lib.ev_setting[0], sizeof(struct ae_ev_setting_param));
				}
				#endif

				memcpy(&out_param_parse.ev_setting[0], &out_param_lib.ev_setting[0], sizeof(struct ae_ev_setting_param));
				memcpy(&out_param_parse.ev_setting[1], &out_param_lib.ev_setting[1], sizeof(struct ae_ev_setting_param));
				memcpy(&out_param_parse.ev_setting[2], &out_param_lib.ev_setting[2], sizeof(struct ae_ev_setting_param));

				for(int i = 0; i < 3; i++) {
					ae_info[i].exp.exposure = out_param_parse.ev_setting[i].exp_line;
					ae_info[i].exp.dummy = out_param_parse.ev_setting[i].dmy_line;
					ae_parse_isp_gain(cxt, !i,out_param_parse.ev_setting[i].ae_gain, &ae_info[i].gain, &dcam_gain[i]);
				}
				ae_info[1].exp.size_index = 3;//此参数会被mw更新   to do
			}else {
				ISP_LOGV("sync:not all sensor work");
			}
		}else {
			in_param.mode = AE_SYNC_0;
			in_param.num = sync_info_master.num;
			ae_sync_in_ae_lib_data_dump(in_param.sync_param[0]);
			ae_sync_in_ae_lib_data_dump(in_param.sync_param[1]);
			ae_lib_frame_sync_calculation(cxt->misc_handle, &in_param, &out_param_lib);

			ISP_LOGV("sync:0 lib out ae_gain:%d, exp_time:%d, exp_line:%d,line_time:%d",
			        out_param_lib.ev_setting[0].ae_gain, out_param_lib.ev_setting[0].exp_time,
			        out_param_lib.ev_setting[0].exp_line,out_param_lib.ev_setting[0].line_time );
			ISP_LOGV("sync:1 lib out ae_gain:%d, exp_time:%d, exp_line:%d,line_time:%d",
			        out_param_lib.ev_setting[1].ae_gain, out_param_lib.ev_setting[1].exp_time,
			        out_param_lib.ev_setting[1].exp_line,out_param_lib.ev_setting[1].line_time );

			for(int i = 0; i < 2; i++) {
				ae_info[i].exp.exposure = out_param_lib.ev_setting[i].exp_line;
				ae_info[i].exp.dummy = out_param_lib.ev_setting[i].dmy_line;
				ae_parse_isp_gain(cxt, !i,out_param_lib.ev_setting[i].ae_gain, &ae_info[i].gain, &dcam_gain[i]);
			}
			ae_info[1].exp.size_index = 2;//此参数会被mw更新
		}

		ISP_LOGV("sync:ae_info0, write_multi_ae data:gain:%d, exposure_line:%d", ae_info[0].gain, ae_info[0].exp.exposure);
		ISP_LOGV("sync:ae_info1, write_multi_ae data:gain:%d, exposure_line:%d", ae_info[1].gain, ae_info[1].exp.exposure);
		ISP_LOGV("sync:ae_info2, write_multi_ae data:gain:%d, exposure_line:%d", ae_info[2].gain, ae_info[2].exp.exposure);

		if (cxt->is_multi_mode == ISP_ALG_TRIBLE_W_T_UW) {
			ae_info[0].count = out_param_lib.num;
			if ((info_slave[0].line_time != 0) && (info_slave[1].line_time != 0)) {
				if (cxt->isp_ops.write_multi_ae) {
					(*cxt->isp_ops.write_multi_ae) (cxt->isp_ops.isp_handler, ae_info);
					ISP_LOGV("sync:write_multi_ae");
				} else {
					ISP_LOGV("sync:write_multi_ae is NULL");
				}
			}
		} else {
			ae_info[0].count = 2;
			if (cxt->isp_ops.write_multi_ae) {
				(*cxt->isp_ops.write_multi_ae) (cxt->isp_ops.isp_handler, ae_info);
			ISP_LOGV("sync:write_multi_ae");
			} else {
				ISP_LOGV("sync:write_multi_ae is NULL");
			}
		}

		ISP_LOGV("sync:M:ae_info[0] exposure_line %d dummy %d size_index %d gain %d, dcam_gain %d", ae_info[0].exp.exposure, ae_info[0].exp.dummy, ae_info[0].exp.size_index, ae_info[0].gain, dcam_gain[0]);
		ISP_LOGV("sync:S0:ae_info[1] exposure_line %d dummy %d size_index %d gain %d, dcam_gain %d", ae_info[1].exp.exposure, ae_info[1].exp.dummy, ae_info[1].exp.size_index, ae_info[1].gain, dcam_gain[1]);
		ISP_LOGV("sync:S1:ae_info[2] exposure_line %d dummy %d size_index %d gain %d, dcam_gain %d", ae_info[2].exp.exposure, ae_info[2].exp.dummy, ae_info[2].exp.size_index, ae_info[2].gain, dcam_gain[2]);

		//ae_match_data_master.exp = ae_info[0].exp;
		memcpy(&ae_match_data_master.exp, &ae_info[0].exp, sizeof(struct sensor_ex_exposure));
		ae_match_data_master.gain = ae_info[0].gain;
		ae_match_data_master.isp_gain = write_param->isp_gain;
		ae_match_data_master.frame_len = master_ae_sync_info_ptr->ev_setting.frm_len;
		ae_match_data_master.frame_len_def = master_ae_sync_info_ptr->frm_len_def;
		//ae_match_data_slave[0].exp = ae_info[1].exp;
		memcpy(&ae_match_data_slave[0].exp, &ae_info[1].exp, sizeof(struct sensor_ex_exposure));
		ae_match_data_slave[0].gain = ae_info[1].gain;
		ae_match_data_slave[0].isp_gain = write_param->isp_gain;
		ae_match_data_slave[0].frame_len = slave0_ae_sync_info_ptr->ev_setting.frm_len;
		ae_match_data_slave[0].frame_len_def = slave0_ae_sync_info_ptr->frm_len_def;
		cxt->ptr_isp_br_ioctrl(CAM_SENSOR_MASTER, SET_MATCH_AE_DATA, &ae_match_data_master, NULL);
		cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE0, SET_MATCH_AE_DATA, &ae_match_data_slave[0], NULL);

		if (cxt->is_multi_mode == ISP_ALG_TRIBLE_W_T_UW) {
			//ae_match_data_slave[1].exp = ae_info[2].exp;
			memcpy(&ae_match_data_slave[1].exp, &ae_info[2].exp, sizeof(struct sensor_ex_exposure));
			ae_match_data_slave[1].gain = ae_info[2].gain;
			ae_match_data_slave[1].isp_gain = write_param->isp_gain;
			ae_match_data_slave[1].frame_len = slave1_ae_sync_info_ptr->ev_setting.frm_len;
			ae_match_data_slave[1].frame_len_def = slave1_ae_sync_info_ptr->frm_len_def;
			cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE1, SET_MATCH_AE_DATA, &ae_match_data_slave[1], NULL);
		}

		if (0 != dcam_gain[0]) {
			double rgb_coeff = dcam_gain[0] * 1.0 / 4096;
			if (cxt->isp_ops.set_rgb_gain) {
				cxt->isp_ops.set_rgb_gain(cxt->isp_ops.isp_handler, rgb_coeff);
			}
		}

		if (cxt->is_multi_mode == ISP_ALG_TRIBLE_W_T_UW) {
			if (0 != dcam_gain[1]) {
				double rgb_coeff = dcam_gain[1] * 1.0 / 4096;
				if (cxt->isp_ops.set_rgb_gain_slave0) {
					cxt->isp_ops.set_rgb_gain_slave0(cxt->isp_ops.isp_handler, rgb_coeff);
				}
			}

			if (0 != dcam_gain[2]) {
				double rgb_coeff = dcam_gain[2] * 1.0 / 4096;
				if (cxt->isp_ops.set_rgb_gain_slave1) {
					cxt->isp_ops.set_rgb_gain_slave1(cxt->isp_ops.isp_handler, rgb_coeff);
				}
			}
		} else {
			if (0 != dcam_gain[1]) {
				double rgb_coeff = dcam_gain[1] * 1.0 / 4096;
				if (cxt->isp_ops.set_rgb_gain_slave0) {
					cxt->isp_ops.set_rgb_gain_slave0(cxt->isp_ops.isp_handler, rgb_coeff);
				}
			}
		}
	} else {
		ISP_LOGE("sync:normal mode:exp data are invalidate: exp: %d, gain: %d\n", write_param->exp_line, write_param->gain);
	}

	ISP_LOGV("sync: cur_bv:%d", cxt->cur_result.cur_bv);

	cxt->ptr_isp_br_ioctrl(CAM_SENSOR_MASTER, SET_MATCH_BV_DATA, &cxt->cur_result.cur_bv, NULL);
	cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE0, SET_MATCH_BV_DATA, &cxt->cur_result.cur_bv, NULL);

	if (cxt->is_multi_mode == ISP_ALG_TRIBLE_W_T_UW) {
		cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE1, SET_MATCH_BV_DATA, &cxt->cur_result.cur_bv, NULL);
	}

	ISP_LOGV("sync:-----ae_sync_write_to_sensor_normal------sensor_role:%d----end----", cxt->sensor_role);
	return ISP_SUCCESS;
}

static cmr_s32 ae_sync_lib_out_data_process(struct ae_ctrl_cxt *cxt, struct ae_lib_frm_sync_out *out_param_lib, struct ae_sensor_exp_data *exp_data_sync)
{
	ISP_LOGV("sync:-----ae_sync_lib_out_data_process------start----");
	struct q_item write_item[3] = {{0}, {0}, {0}};
	struct q_item actual_item[3] = {{0}, {0}, {0}};
	struct q_item input_item[3] = {{0}, {0}, {0}};
	struct ae_sync_gain_param ae_sync_gain[3] = {{0}, {0}, {0}};
	struct ae_exposure_param write_param = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	struct ae_sync_lib_outout_data ae_sync_lib_output = {0};

	ae_parse_isp_gain(cxt, 1, out_param_lib->ev_setting[0].ae_gain, &ae_sync_gain[0].sensor_gain, &ae_sync_gain[0].isp_gain);
	ISP_LOGV("ae_sync_lib_out_data_process, master, gain_info, ae_gain:%d, sensor_gain:%d, isp_gain:%d",  out_param_lib->ev_setting[0].ae_gain, ae_sync_gain[0].sensor_gain, ae_sync_gain[0].isp_gain);
	input_item[0].exp_line = out_param_lib->ev_setting[0].exp_line;
	input_item[0].exp_time = out_param_lib->ev_setting[0].exp_time;
	input_item[0].isp_gain = ae_sync_gain[0].isp_gain;
	input_item[0].sensor_gain = ae_sync_gain[0].sensor_gain;
	input_item[0].dumy_line = out_param_lib->ev_setting[0].dmy_line;
	input_item[0].frm_len = out_param_lib->ev_setting[0].frm_len;
	ISP_LOGV("ae_sync_lib_out_data_process, master, input_info, exp_line:%d, exp_time:%d, isp_gain:%d, sensor_gain:%d", input_item[0].exp_line, input_item[0].exp_time, input_item[0].isp_gain, input_item[0].sensor_gain);

	s_q_put(cxt->seq_handle, &input_item[0], &write_item[0], &actual_item[0]);

	/*save slave lib sync output data to bridge*/
	ae_sync_lib_output.ae_gain = out_param_lib->ev_setting[1].ae_gain;
	ae_sync_lib_output.exp_time = out_param_lib->ev_setting[1].exp_time;
	ae_sync_lib_output.exp_line = out_param_lib->ev_setting[1].exp_line;
	ae_sync_lib_output.dmy_line = out_param_lib->ev_setting[1].dmy_line;
	ae_sync_lib_output.frm_len = out_param_lib->ev_setting[1].frm_len;
	cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE0, SET_SYNC_SLAVE_SYNC_OUTPUT, &ae_sync_lib_output, NULL);

	/*write master data to sensor*/
	write_param.exp_line = write_item[0].exp_line;
	write_param.exp_time = write_item[0].exp_time;
	write_param.dummy = write_item[0].dumy_line;
	write_param.isp_gain = write_item[0].isp_gain;
	write_param.sensor_gain = write_item[0].sensor_gain;
	write_param.frm_len = write_item[0].frm_len;
	cxt->glb_gain = (cmr_u32) (write_item[0].isp_gain * 1.0 / 4096 * cxt->backup_rgb_gain + 0.5);

	ISP_LOGV("ae_sync_lib_out_data_process, master, write_param, sensor_role:%d, multi_mode:%d, exp_line:%d, exp_time:%d, isp_gain:%d, sensor_gain:%d", \
			cxt->sensor_role, cxt->is_multi_mode, write_param.exp_line, write_param.exp_time, write_param.isp_gain, write_param.sensor_gain);

	ISP_LOGV("ae_sync_lib_out_data_process, master, actual_param, sensor_role:%d, multi_mode:%d, exp_line:%d, exp_time:%d, isp_gain:%d, sensor_gain:%d", \
			cxt->sensor_role, cxt->is_multi_mode, actual_item[0].exp_line, actual_item[0].exp_time, actual_item[0].isp_gain, actual_item[0].sensor_gain);

	ae_write_to_sensor(cxt, &write_param);

	exp_data_sync->write_data.exp_line = write_item[0].exp_line;
	exp_data_sync->write_data.exp_time = write_item[0].exp_time;
	exp_data_sync->write_data.dummy = write_item[0].dumy_line;
	exp_data_sync->write_data.sensor_gain = write_item[0].sensor_gain;
	exp_data_sync->write_data.isp_gain = write_item[0].isp_gain;
	exp_data_sync->write_data.frm_len = write_item[0].frm_len;

	exp_data_sync->actual_data.exp_line = actual_item[0].exp_line;
	exp_data_sync->actual_data.exp_time = actual_item[0].exp_time;
	exp_data_sync->actual_data.dummy = actual_item[0].dumy_line;
	exp_data_sync->actual_data.sensor_gain = actual_item[0].sensor_gain;
	exp_data_sync->actual_data.isp_gain = actual_item[0].isp_gain;
	exp_data_sync->actual_data.frm_len = actual_item[0].frm_len;
	exp_data_sync->actual_data.gain = actual_item[0].sensor_gain * actual_item[0].isp_gain/4096;

	ISP_LOGV("ae_sync_lib_out_data_process, master, gain:%d", exp_data_sync->actual_data.gain);

	if(cxt->cam_cap_flag && cxt->cam_large_pix_num) {
		double rgb_coeff = 0;
		rgb_coeff = write_item[0].isp_gain * 1.0 / 4096;
		if (cxt->isp_ops.set_rgb_gain_4in1) {
			cxt->isp_ops.set_rgb_gain_4in1(cxt->isp_ops.isp_handler, rgb_coeff);
		}
	}
	ISP_LOGV("sync:-----ae_sync_lib_out_data_process------end----");
	return ISP_SUCCESS;
}

static cmr_s32 ae_sync_process(struct ae_ctrl_cxt *cxt, struct ae_sensor_exp_data *exp_data_sync)
{
	ISP_LOGV("sync:-----ae_sync_process------start----");
	ISP_LOGV("sync:ae_sync_process, write_param, exp_line:%d, ae_gain:%d, dmy_line:%d, exp_time:%d", exp_data_sync->lib_data.exp_line, exp_data_sync->lib_data.gain, exp_data_sync->lib_data.dummy, exp_data_sync->lib_data.exp_time);
	struct sensor_info info_master = {0};
	struct sensor_info info_slave[2] = {{0}, {0}};
	struct ae_sync_data sync_info_master = {0};
	struct ae_sync_data sync_info_slave[2] = {{0},{0}};
	struct ae_sync_actual_data ae_sync_actual[2] = {{0}, {0}};
	struct sensor_otp_ae_info ae_otp_master = {0};
	struct sensor_otp_ae_info ae_otp_slave[2] = {{0},{0}};
	struct ae_lib_frm_sync_out out_param_lib = {0};
	struct ae_lib_frm_sync_in in_param = {0};
	struct ae_lib_output_data ae_slave_lib_output = {0};

	in_param.sync_param[0] = &cxt->sync_param[0];
	in_param.sync_param[1] = &cxt->sync_param[1];

	struct ae_frm_sync_param *master_ae_sync_info_ptr = in_param.sync_param[0];
	struct ae_frm_sync_param *slave0_ae_sync_info_ptr = in_param.sync_param[1];
	struct ae_match_stats_data stats_data_master = {0};
	struct ae_match_stats_data stats_data_slave[2] = {{0},{0}};
	struct aem_info slave_aem_info[2] = {{0},{0}};

	memset(master_ae_sync_info_ptr,0,sizeof(struct ae_frm_sync_param));
	memset(slave0_ae_sync_info_ptr,0,sizeof(struct ae_frm_sync_param));

	if (0 != exp_data_sync->lib_data.exp_line && 0 != exp_data_sync->lib_data.gain) {

		cxt->ptr_isp_br_ioctrl(CAM_SENSOR_MASTER, GET_MODULE_INFO, NULL, &info_master);
		cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE0, GET_MODULE_INFO, NULL, &info_slave[0]);
		cxt->ptr_isp_br_ioctrl(CAM_SENSOR_MASTER, GET_OTP_AE, NULL, &ae_otp_master);
		cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE0, GET_OTP_AE, NULL, &ae_otp_slave[0]);
		cxt->ptr_isp_br_ioctrl(CAM_SENSOR_MASTER, GET_AE_SYNC_DATA, NULL, &sync_info_master);
		cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE0, GET_AE_SYNC_DATA, NULL, &sync_info_slave[0]);
		cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE0, GET_SYNC_SLAVE_ACTUAL_DATA, NULL, &ae_sync_actual[0]);

		cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE0, GET_SLAVE_AEM_INFO, NULL, &slave_aem_info[0]);
		cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE0, GET_STAT_AWB_DATA_AE, NULL, &cxt->slave0_aem_stat);

		cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE0, GET_SYNC_SLAVE_LIB_OUTPUT, NULL, &ae_slave_lib_output);

		stats_data_master.len = sizeof(master_ae_sync_info_ptr->aem);
		stats_data_master.stats_data = &master_ae_sync_info_ptr->aem[0];
		stats_data_slave[0].len = sizeof(slave0_ae_sync_info_ptr->aem);
		stats_data_slave[0].stats_data = &slave0_ae_sync_info_ptr->aem[0];

		for(cmr_u32 i = 0; i < 3 * 1024; i++) {
			stats_data_master.stats_data[i] = cxt->sync_aem[i]>>2;
		}

		if(slave_aem_info[0].aem_stat_blk_pixels)
			_aem_stat_preprocess_dulpslave(&slave_aem_info[0], cxt->slave0_aem_stat, stats_data_slave[0].stats_data);

		master_ae_sync_info_ptr->monoboottime = stats_data_master.monoboottime;
		master_ae_sync_info_ptr->max_gain = info_master.max_again;
		master_ae_sync_info_ptr->min_gain = info_master.min_again;
		master_ae_sync_info_ptr->min_exp_line =info_master.min_exp_line;
		master_ae_sync_info_ptr->sensor_gain_precision = info_master.sensor_gain_precision;
		master_ae_sync_info_ptr->ev_setting.line_time = info_master.line_time;
		master_ae_sync_info_ptr->ev_setting.exp_line = exp_data_sync->lib_data.exp_line;
		master_ae_sync_info_ptr->ev_setting.ae_gain =  exp_data_sync->lib_data.gain;
		master_ae_sync_info_ptr->ev_setting.dmy_line = exp_data_sync->lib_data.dummy;
		master_ae_sync_info_ptr->ev_setting.exp_time = exp_data_sync->lib_data.exp_time;
		master_ae_sync_info_ptr->ev_setting.frm_len = exp_data_sync->lib_data.frm_len;
		master_ae_sync_info_ptr->effect_param.line_time = info_master.line_time;
		master_ae_sync_info_ptr->effect_param.exp_line = exp_data_sync->actual_data.exp_line;
		master_ae_sync_info_ptr->effect_param.ae_gain =  exp_data_sync->actual_data.gain;
		master_ae_sync_info_ptr->effect_param.dmy_line = exp_data_sync->actual_data.dummy;
		master_ae_sync_info_ptr->effect_param.exp_time = exp_data_sync->actual_data.exp_time;
		master_ae_sync_info_ptr->effect_param.frm_len = exp_data_sync->actual_data.frm_len;
		master_ae_sync_info_ptr->frm_len_def = info_master.frm_len_def;
		master_ae_sync_info_ptr->otp_info.gain_1x_exp = ae_otp_master.gain_1x_exp;
		master_ae_sync_info_ptr->otp_info.gain_2x_exp = ae_otp_master.gain_2x_exp;
		master_ae_sync_info_ptr->otp_info.gain_4x_exp = ae_otp_master.gain_4x_exp;
		master_ae_sync_info_ptr->otp_info.gain_8x_exp = ae_otp_master.gain_8x_exp;
		master_ae_sync_info_ptr->otp_info.ae_target_lum = ae_otp_master.ae_target_lum;
		master_ae_sync_info_ptr->otp_info.reserve = ae_otp_master.reserve;
		memcpy(&master_ae_sync_info_ptr->blk_size, &sync_info_master.block_size, sizeof(struct ae_size));
		memcpy(&master_ae_sync_info_ptr->img_size,&sync_info_master.sensor_size, sizeof(struct ae_size));
		memcpy(&master_ae_sync_info_ptr->aem_roi_rect, &sync_info_master.target_rect, sizeof(struct ae_rect));
		memcpy(&master_ae_sync_info_ptr->block_rect, &sync_info_master.block_rect, sizeof(struct isp_rect));
		master_ae_sync_info_ptr->is_benchmark = sync_info_master.ref_camera_id;
		master_ae_sync_info_ptr->blks_num.w = BLK_NUM_W_ALG;
		master_ae_sync_info_ptr->blks_num.h = BLK_NUM_W_ALG;

		slave0_ae_sync_info_ptr->monoboottime = stats_data_slave[0].monoboottime;
		slave0_ae_sync_info_ptr->ev_setting.line_time = info_slave[0].line_time;
		slave0_ae_sync_info_ptr->max_gain = info_slave[0].max_again;
		slave0_ae_sync_info_ptr->min_gain = info_slave[0].min_again;
		slave0_ae_sync_info_ptr->min_exp_line =info_slave[0].min_exp_line;
		slave0_ae_sync_info_ptr->sensor_gain_precision = info_slave[0].sensor_gain_precision;
		slave0_ae_sync_info_ptr->frm_len_def = info_slave[0].frm_len_def;
		slave0_ae_sync_info_ptr->otp_info.gain_1x_exp = ae_otp_slave[0].gain_1x_exp;
		slave0_ae_sync_info_ptr->otp_info.gain_2x_exp = ae_otp_slave[0].gain_2x_exp;
		slave0_ae_sync_info_ptr->otp_info.gain_4x_exp = ae_otp_slave[0].gain_4x_exp;
		slave0_ae_sync_info_ptr->otp_info.gain_8x_exp = ae_otp_slave[0].gain_8x_exp;
		slave0_ae_sync_info_ptr->otp_info.ae_target_lum = ae_otp_slave[0].ae_target_lum;
		slave0_ae_sync_info_ptr->otp_info.reserve = ae_otp_slave[0].reserve;
		memcpy(&slave0_ae_sync_info_ptr->blk_size, &sync_info_slave[0].block_size, sizeof(struct ae_size));
		memcpy(&slave0_ae_sync_info_ptr->img_size,&sync_info_slave[0].sensor_size, sizeof(struct ae_size));
		memcpy(&slave0_ae_sync_info_ptr->aem_roi_rect, &sync_info_slave[0].target_rect, sizeof(struct ae_rect));
		memcpy(&slave0_ae_sync_info_ptr->block_rect, &sync_info_slave[0].block_rect, sizeof(struct isp_rect));
		slave0_ae_sync_info_ptr->is_benchmark = sync_info_slave[0].ref_camera_id;
		slave0_ae_sync_info_ptr->blks_num.w = BLK_NUM_W_ALG;
		slave0_ae_sync_info_ptr->blks_num.h = BLK_NUM_W_ALG;
		slave0_ae_sync_info_ptr->ev_setting.exp_line = ae_slave_lib_output.exp_line;
		slave0_ae_sync_info_ptr->ev_setting.ae_gain = ae_slave_lib_output.gain;
		slave0_ae_sync_info_ptr->ev_setting.dmy_line = ae_slave_lib_output.dummy;
		slave0_ae_sync_info_ptr->ev_setting.exp_time = ae_slave_lib_output.exp_time;
		slave0_ae_sync_info_ptr->ev_setting.frm_len = ae_slave_lib_output.frm_len;
		slave0_ae_sync_info_ptr->effect_param.line_time = info_slave[0].line_time;
		slave0_ae_sync_info_ptr->effect_param.exp_line = ae_sync_actual[0].exp_line;
		slave0_ae_sync_info_ptr->effect_param.ae_gain =  ae_sync_actual[0].ae_gain;
		slave0_ae_sync_info_ptr->effect_param.dmy_line = ae_sync_actual[0].dmy_line;
		slave0_ae_sync_info_ptr->effect_param.exp_time = ae_sync_actual[0].exp_time;
		slave0_ae_sync_info_ptr->effect_param.frm_len =  ae_sync_actual[0].frm_len;

		if (cxt->is_multi_mode == ISP_ALG_TRIBLE_W_T_UW) {
			in_param.mode = AE_SYNC_1;
		} else {
			in_param.mode = AE_SYNC_0;
		}

		in_param.num = sync_info_master.num;

		for(int i = 0; i < in_param.num; i++) {
			ISP_LOGV("-----ae_sync_in_ae_lib_data_dump------i:%d----", i);
			ae_sync_in_ae_lib_data_dump(in_param.sync_param[i]);
		}

		ae_lib_frame_sync_calculation(cxt->misc_handle, &in_param, &out_param_lib);

		for(int i = 0; i < in_param.num; i++) {
			ISP_LOGV("sync:sensor:%d, lib out ae_gain:%d, exp_time:%d, exp_line:%d,line_time:%d",
					i, out_param_lib.ev_setting[i].ae_gain, out_param_lib.ev_setting[i].exp_time,
					out_param_lib.ev_setting[i].exp_line, out_param_lib.ev_setting[i].line_time);
		}
	}

	#if 0
	cxt->ptr_isp_br_ioctrl(CAM_SENSOR_MASTER, SET_MATCH_BV_DATA, &cxt->cur_result.cur_bv, NULL);
	cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE0, SET_MATCH_BV_DATA, &cxt->cur_result.cur_bv, NULL);
	#endif

	ae_sync_lib_out_data_process(cxt, &out_param_lib, exp_data_sync);

	ISP_LOGV("sync:-----ae_sync_process----------end----");
	return ISP_SUCCESS;
}



static cmr_s32 ae_update_result_to_sensor(struct ae_ctrl_cxt *cxt, struct ae_sensor_exp_data *exp_data, cmr_u32 is_force)
{
	ISP_LOGV("-----ae_update_result_to_sensor start----, cameraId:%d", cxt->camera_id);
	cmr_s32 ret = ISP_SUCCESS;
	struct ae_exposure_param write_param = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	struct q_item write_item = { 0, 0, 0, 0, 0, 0, 0};
	struct q_item actual_item;

	if (0 == cxt) {
		ISP_LOGE("cxt invalid, cxt: %p\n", cxt);
		ret = ISP_ERROR;
		return ret;
	}

	ae_update_exp_data(cxt, exp_data, &write_item, &actual_item, is_force);

	write_param.exp_line = write_item.exp_line;
	write_param.exp_time = write_item.exp_time;
	write_param.dummy = write_item.dumy_line;
	write_param.isp_gain = write_item.isp_gain;
	write_param.sensor_gain = write_item.sensor_gain;
	write_param.frm_len = write_item.frm_len;
	cxt->glb_gain = (cmr_u32) (write_item.isp_gain * 1.0 / 4096 * cxt->backup_rgb_gain + 0.5);

	ISP_LOGV("write_param, sensor_role:%d, multi_mode:%d, exp_line:%d, exp_time:%d, isp_gain:%d, sensor_gain:%d", \
			cxt->sensor_role, cxt->is_multi_mode, write_param.exp_line, write_param.exp_time, write_param.isp_gain, write_param.sensor_gain);

	ae_write_to_sensor(cxt, &write_param);

	exp_data->write_data.exp_line = write_item.exp_line;
	exp_data->write_data.exp_time = write_item.exp_time;
	exp_data->write_data.dummy = write_item.dumy_line;
	exp_data->write_data.sensor_gain = write_item.sensor_gain;
	exp_data->write_data.isp_gain = write_item.isp_gain;
	exp_data->write_data.frm_len = write_item.frm_len;

	exp_data->actual_data.exp_line = actual_item.exp_line;
	exp_data->actual_data.exp_time = actual_item.exp_time;
	exp_data->actual_data.dummy = actual_item.dumy_line;
	exp_data->actual_data.sensor_gain = actual_item.sensor_gain;
	exp_data->actual_data.isp_gain = actual_item.isp_gain;
	exp_data->actual_data.frm_len = actual_item.frm_len;

	if(cxt->cam_cap_flag && cxt->cam_large_pix_num){
		double rgb_coeff = 0;
		rgb_coeff = write_item.isp_gain * 1.0 / 4096;
		if (cxt->isp_ops.set_rgb_gain_4in1) {
			cxt->isp_ops.set_rgb_gain_4in1(cxt->isp_ops.isp_handler, rgb_coeff);
		}
	}
	ISP_LOGV("-----ae_update_result_to_sensor end----");
	return ret;
}

static cmr_s32 ae_update_result_to_slave_sensor(struct ae_ctrl_cxt *cxt, struct ae_sensor_exp_data *exp_data, cmr_u32 is_force)
{
	ISP_LOGV("-----ae_update_result_to_slave_sensor start----, cameraId:%d", cxt->camera_id);
	cmr_s32 ret = ISP_SUCCESS;
	struct ae_exposure_param write_param = {0};
	struct q_item write_item = {0};
	struct q_item actual_item = {0};
	struct q_item input_item = {0};
	struct ae_sync_lib_outout_data ae_sync_lib_output = {0};
	struct ae_sync_gain_param ae_sync_gain = {0};
	struct ae_sync_actual_data ae_sync_actual_output = {0};
	UNUSED(is_force);

	if (0 == cxt) {
		ISP_LOGE("cxt invalid, cxt: %p\n", cxt);
		ret = ISP_ERROR;
		return ret;
	}

	cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE0, GET_SYNC_SLAVE_SYNC_OUTPUT, NULL, &ae_sync_lib_output);

	ae_parse_isp_gain(cxt, 0, ae_sync_lib_output.ae_gain, &ae_sync_gain.sensor_gain, &ae_sync_gain.isp_gain);
	ISP_LOGV("ae_update_result_to_slave_sensor, sync_lib_out, gain_info, ae_gain:%d, sensor_gain:%d, isp_gain:%d", ae_sync_lib_output.ae_gain, ae_sync_gain.sensor_gain, ae_sync_gain.isp_gain);
	input_item.exp_line = ae_sync_lib_output.exp_line;
	input_item.exp_time = ae_sync_lib_output.exp_time;
	input_item.isp_gain = ae_sync_gain.isp_gain;
	input_item.sensor_gain = ae_sync_gain.sensor_gain;
	input_item.dumy_line = ae_sync_lib_output.dmy_line;
	input_item.frm_len = ae_sync_lib_output.frm_len;
	ISP_LOGV("ae_update_result_to_slave_sensor, input_info, exp_line:%d, exp_time:%d, isp_gain:%d, sensor_gain:%d", input_item.exp_line, input_item.exp_time, input_item.isp_gain, input_item.sensor_gain);

	s_q_put(cxt->seq_handle, &input_item, &write_item, &actual_item);

	write_param.exp_line = write_item.exp_line;
	write_param.exp_time = write_item.exp_time;
	write_param.dummy = write_item.dumy_line;
	write_param.isp_gain = write_item.isp_gain;
	write_param.sensor_gain = write_item.sensor_gain;
	write_param.frm_len = write_item.frm_len;
	cxt->glb_gain = (cmr_u32) (write_item.isp_gain * 1.0 / 4096 * cxt->backup_rgb_gain + 0.5);

	ISP_LOGV("ae_update_result_to_slave_sensor, write_param, sensor_role:%d, multi_mode:%d, exp_line:%d, exp_time:%d, isp_gain:%d, sensor_gain:%d", \
			cxt->sensor_role, cxt->is_multi_mode, write_param.exp_line, write_param.exp_time, write_param.isp_gain, write_param.sensor_gain);
	ISP_LOGV("ae_update_result_to_slave_sensor, actual_param, sensor_role:%d, multi_mode:%d, exp_line:%d, exp_time:%d, isp_gain:%d, sensor_gain:%d", \
			cxt->sensor_role, cxt->is_multi_mode, actual_item.exp_line, actual_item.exp_time, actual_item.isp_gain, actual_item.sensor_gain);

	ae_write_to_sensor(cxt, &write_param);

	exp_data->write_data.exp_line = write_item.exp_line;
	exp_data->write_data.exp_time = write_item.exp_time;
	exp_data->write_data.dummy = write_item.dumy_line;
	exp_data->write_data.sensor_gain = write_item.sensor_gain;
	exp_data->write_data.isp_gain = write_item.isp_gain;
	exp_data->write_data.frm_len = write_item.frm_len;

	exp_data->actual_data.exp_line = actual_item.exp_line;
	exp_data->actual_data.exp_time = actual_item.exp_time;
	exp_data->actual_data.dummy = actual_item.dumy_line;
	exp_data->actual_data.sensor_gain = actual_item.sensor_gain;
	exp_data->actual_data.isp_gain = actual_item.isp_gain;
	exp_data->actual_data.frm_len = actual_item.frm_len;

	/*save slave actual data to bridge*/
	ae_sync_actual_output.ae_gain = actual_item.isp_gain * actual_item.sensor_gain /4096;
	ae_sync_actual_output.exp_time = actual_item.exp_time;
	ae_sync_actual_output.exp_line = actual_item.exp_line;
	ae_sync_actual_output.dmy_line = actual_item.dumy_line;
	ae_sync_actual_output.frm_len = actual_item.frm_len;
	ae_sync_actual_output.frame_len_def = actual_item.frm_len_def;
	ae_sync_actual_output.sensor_gain = actual_item.sensor_gain;
	ae_sync_actual_output.isp_gain = actual_item.isp_gain;
	cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE0, SET_SYNC_SLAVE_ACTUAL_DATA, &ae_sync_actual_output, NULL);

	if(cxt->cam_cap_flag && cxt->cam_large_pix_num) {
		double rgb_coeff = 0;
		rgb_coeff = write_item.isp_gain * 1.0 / 4096;
		if (cxt->isp_ops.set_rgb_gain_4in1) {
			cxt->isp_ops.set_rgb_gain_4in1(cxt->isp_ops.isp_handler, rgb_coeff);
		}
	}
	ISP_LOGV("-----ae_update_result_to_slave_sensor end----");
	return ret;
}

static cmr_s32 ae_adjust_exp_gain(struct ae_ctrl_cxt *cxt, struct ae_exposure_param *src_exp_param, struct ae_range *fps_range, cmr_u32 max_exp, struct ae_exposure_param *dst_exp_param)
{
	cmr_s32 ret = ISP_SUCCESS;
	cmr_u32 max_gain = 0;
	cmr_s32 i = 0;
	cmr_s32 dummy = 0;
	double divisor_coeff = 0.0;
	double cur_fps = 0.0, tmp_fps = 0.0;
	double tmp_mn = fps_range->min;
	double tmp_mx = fps_range->max;
	double product = 1.0 * src_exp_param->exp_time * src_exp_param->gain;
	cmr_u32 exp_cnts = 0;
	cmr_u32 sensor_maxfps = cxt->cur_status.adv_param.sensor_fps_range.max;
	double product_max = 1.0 * cxt->ae_tbl_param.max_exp * cxt->ae_tbl_param.max_gain;
	max_gain = cxt->ae_tbl_param.max_gain;

	ISP_LOGV("max:src %.f, dst %.f, max index: %d\n", product, product_max, cxt->ae_tbl_param.max_index);

	if (product_max < product) {
		product = product_max;
	}
	*dst_exp_param = *src_exp_param;
	if (AE_FLICKER_50HZ == cxt->cur_status.adv_param.flicker) {
		divisor_coeff = 1.0 / 100.0;
	} else if (AE_FLICKER_60HZ == cxt->cur_status.adv_param.flicker) {
		divisor_coeff = 1.0 / 120.0;
	} else {
		divisor_coeff = 1.0 / 100.0;
	}

	if(cxt->high_fps_info.is_high_fps)
		divisor_coeff = 1.0 / 120.0;

	if (tmp_mn < 1.0 * AEC_LINETIME_PRECESION / (max_exp * cxt->cur_status.adv_param.cur_ev_setting.line_time)) {
		tmp_mn = 1.0 * AEC_LINETIME_PRECESION / (max_exp * cxt->cur_status.adv_param.cur_ev_setting.line_time);
	}

	cur_fps = 1.0 * AEC_LINETIME_PRECESION / src_exp_param->exp_time;
	if (cur_fps > sensor_maxfps) {
		cur_fps = sensor_maxfps;
	}

	if ((cmr_u32) (cur_fps + 0.5) >= (cmr_u32) (tmp_mn + 0.5)) {
		exp_cnts = (cmr_u32) (1.0 * src_exp_param->exp_time / (AEC_LINETIME_PRECESION * divisor_coeff) + 0.5);
		if (exp_cnts > 0) {
			dst_exp_param->exp_line = (cmr_u32) ((exp_cnts * divisor_coeff) * 1.0 * AEC_LINETIME_PRECESION / cxt->cur_status.adv_param.cur_ev_setting.line_time + 0.5);
			dst_exp_param->gain = (cmr_u32) (product / cxt->cur_status.adv_param.cur_ev_setting.line_time / dst_exp_param->exp_line + 0.5);
			if (dst_exp_param->gain < 128) {
				cmr_s32 tmp_gain = 0;
				for (i = exp_cnts - 1; i > 0; i--) {
					tmp_gain = (cmr_s32) (product / (1.0 * AEC_LINETIME_PRECESION * (i * divisor_coeff)) + 0.5);
					if (tmp_gain >= 128) {
						break;
					}
				}
				if (i > 0) {
					dst_exp_param->exp_line = (cmr_s16) ((i * divisor_coeff) * 1.0 * AEC_LINETIME_PRECESION / cxt->cur_status.adv_param.cur_ev_setting.line_time + 0.5);
					dst_exp_param->gain = (cmr_s16) (product / (dst_exp_param->exp_line * cxt->cur_status.adv_param.cur_ev_setting.line_time) + 0.5);
				} else {
					dst_exp_param->gain = 128;
					dst_exp_param->exp_line = (cmr_s16) (product / (dst_exp_param->gain * cxt->cur_status.adv_param.cur_ev_setting.line_time) + 0.5);
				}
			} else if (dst_exp_param->gain > max_gain) {
				dst_exp_param->gain = max_gain;
			}
		} else {
			/*due to the exposure time is smaller than 1/100 or 1/120, exp time do not need to adjust */
			dst_exp_param->exp_line = src_exp_param->exp_line;
			dst_exp_param->gain = src_exp_param->gain;
		}
		tmp_fps = 1.0 * AEC_LINETIME_PRECESION / (dst_exp_param->exp_line * cxt->cur_status.adv_param.cur_ev_setting.line_time);
		if (tmp_fps > sensor_maxfps) {
			tmp_fps = sensor_maxfps;
		}
		dst_exp_param->dummy = 0;
		if ((cmr_u32) (tmp_fps + 0.5) >= (cmr_u32) (tmp_mx + 0.5)) {
			if (tmp_mx < sensor_maxfps) {
				dummy = (cmr_u32) (1.0 * AEC_LINETIME_PRECESION / (cxt->cur_status.adv_param.cur_ev_setting.line_time * tmp_mx) - dst_exp_param->exp_line + 0.5);
			}
		}

		if (dummy < 0) {
			dst_exp_param->dummy = 0;
		} else {
			dst_exp_param->dummy = dummy;
		}
	} else {
		exp_cnts = (cmr_u32) (1.0 / tmp_mn / divisor_coeff + 0.5);
		if (exp_cnts > 0) {
			dst_exp_param->exp_line = (cmr_u32) ((exp_cnts * divisor_coeff) * 1.0 * AEC_LINETIME_PRECESION / cxt->cur_status.adv_param.cur_ev_setting.line_time + 0.5);
			dst_exp_param->gain = (cmr_u32) (product / cxt->cur_status.adv_param.cur_ev_setting.line_time / dst_exp_param->exp_line + 0.5);
			if (dst_exp_param->gain < 128) {
				cmr_s32 tmp_gain = 0;
				for (i = exp_cnts - 1; i > 0; i--) {
					tmp_gain = (cmr_s32) (product / (1.0 * AEC_LINETIME_PRECESION * (i * divisor_coeff)) + 0.5);
					if (tmp_gain >= 128) {
						break;
					}
				}
				if (i > 0) {
					dst_exp_param->exp_line = (cmr_s16) ((i * divisor_coeff) * 1.0 * AEC_LINETIME_PRECESION / cxt->cur_status.adv_param.cur_ev_setting.line_time + 0.5);
					dst_exp_param->gain = (cmr_s16) (product / (dst_exp_param->exp_line * cxt->cur_status.adv_param.cur_ev_setting.line_time) + 0.5);
				} else {
					dst_exp_param->gain = 128;
					dst_exp_param->exp_line = (cmr_s16) (product / (dst_exp_param->gain * cxt->cur_status.adv_param.cur_ev_setting.line_time) + 0.5);
				}
			} else if (dst_exp_param->gain > max_gain) {
				dst_exp_param->gain = max_gain;
			}
		} else {
			/*due to the exposure time is smaller than 1/100 or 1/120, exp time do not need to adjust */
			dst_exp_param->gain = src_exp_param->gain;
			dst_exp_param->exp_line = src_exp_param->exp_line;
		}
		dst_exp_param->dummy = 0;
		tmp_fps = 1.0 * AEC_LINETIME_PRECESION / (dst_exp_param->exp_line * cxt->cur_status.adv_param.cur_ev_setting.line_time);
		if (tmp_fps > sensor_maxfps) {
			tmp_fps = sensor_maxfps;
		}
		if ((cmr_u32) (tmp_fps + 0.5) >= (cmr_u32) (tmp_mx + 0.5)) {
			if (tmp_mx < sensor_maxfps) {
				dummy = (cmr_u32) (1.0 * AEC_LINETIME_PRECESION / (cxt->cur_status.adv_param.cur_ev_setting.line_time * tmp_mx) - dst_exp_param->exp_line + 0.5);
			}
		}

		if (dummy < 0) {
			dst_exp_param->dummy = 0;
		} else {
			dst_exp_param->dummy = dummy;
		}
	}
	dst_exp_param->frm_len = dst_exp_param->exp_line + dst_exp_param->dummy;
	dst_exp_param->frm_len_def = 1.0 * AEC_LINETIME_PRECESION / (cxt->cur_status.adv_param.cur_ev_setting.line_time * cxt->snr_info.snr_setting_max_fps);
	dst_exp_param->exp_time = dst_exp_param->exp_line * cxt->cur_status.adv_param.cur_ev_setting.line_time;

	if (0 == cxt->cur_status.adv_param.cur_ev_setting.line_time) {
		ISP_LOGE("Can't receive line_time from drvier!");
	}

	ISP_LOGV("fps: %d, %d,max exp l: %d src: %d, %d, %d, dst:%d, %d, %d\n",
			 fps_range->min, fps_range->max, max_exp, src_exp_param->exp_line, src_exp_param->dummy, src_exp_param->gain, dst_exp_param->exp_line, dst_exp_param->dummy, dst_exp_param->gain);

	return ret;
}

static cmr_s32 ae_is_mlog(struct ae_ctrl_cxt *cxt)
{
	cmr_u32 ret = 0;
	cmr_s32 len = 0;
	cmr_s32 is_save = 0;
#ifndef WIN32
	char value[PROPERTY_VALUE_MAX];
	len = property_get(AE_SAVE_MLOG, value, AE_SAVE_MLOG_DEFAULT);
	if (len) {
		memcpy((cmr_handle) & cxt->debug_file_name[0], &value[0], len);
		cxt->debug_info_handle = (cmr_handle) debug_file_init(cxt->debug_file_name, "w+t");
		if (cxt->debug_info_handle) {
			ret = debug_file_open((debug_handle_t) cxt->debug_info_handle, 1, 0);
			if (0 == ret) {
				is_save = 1;
				debug_file_close((debug_handle_t) cxt->debug_info_handle);
			}
		}
	}
#endif
	return is_save;
}

static void ae_print_debug_info(char *log_str, struct ae_ctrl_cxt *cxt_ptr)
{
	float fps = 0.0;
	cmr_u32 pos = 0;
	struct ae_lib_calc_out *result_ptr;
	struct ae_lib_calc_in *sync_cur_status_ptr;

	sync_cur_status_ptr = &(cxt_ptr->sync_cur_status);
	result_ptr = &cxt_ptr->sync_cur_result;

	fps = AEC_LINETIME_PRECESION / (cxt_ptr->snr_info.line_time * (result_ptr->ev_setting.dmy_line + result_ptr->ev_setting.exp_line));
	if (fps > cxt_ptr->sync_cur_status.adv_param.fps_range.max) {
		fps = cxt_ptr->sync_cur_status.adv_param.fps_range.max;
	}

	pos =
		sprintf(log_str, "cam-id:%d frm-id:%d,flicker: %d\nidx(%d-%d):%d,cur-l:%d, tar-l:%d, bv(lv):%d, cali_bv: %d,expl(%d):%d, expt: %d, gain:%d, dmy:%d, FR(%d-%d):%.2f,\
		ae sync M: img[%d, %d] roi[%d, %d, %d, %d] rst[exp %d, gain %d, lum %d] S0: img[%d, %d] roi[%d, %d, %d, %d] rst[exp %d, gain %d, lum %d] S1: img[%d, %d] roi[%d, %d, %d, %d] rst[exp %d, gain %d, lum %d]\n",
				cxt_ptr->camera_id, sync_cur_status_ptr->frm_id, sync_cur_status_ptr->adv_param.flicker, cxt_ptr->ae_tbl_param.min_index,
				cxt_ptr->ae_tbl_param.max_index, result_ptr->ev_setting.ae_idx, cxt_ptr->sync_cur_result.cur_lum, cxt_ptr->sync_cur_result.target_lum,
				cxt_ptr->cur_result.cur_bv, cxt_ptr->cur_result.cur_bv_nonmatch, cxt_ptr->snr_info.line_time, result_ptr->ev_setting.exp_line,
				result_ptr->ev_setting.exp_line * cxt_ptr->snr_info.line_time, result_ptr->ev_setting.ae_gain, result_ptr->ev_setting.dmy_line,
				cxt_ptr->sync_cur_status.adv_param.fps_range.min, cxt_ptr->sync_cur_status.adv_param.fps_range.max, fps,
				cxt_ptr->sync_param[0].img_size.w, cxt_ptr->sync_param[0].img_size.h, cxt_ptr->sync_param[0].aem_roi_rect.start_x, cxt_ptr->sync_param[0].aem_roi_rect.start_y, cxt_ptr->sync_param[0].aem_roi_rect.end_x, cxt_ptr->sync_param[0].aem_roi_rect.end_y,
				cxt_ptr->sync_param[0].ev_setting.exp_time, cxt_ptr->sync_param[0].ev_setting.ae_gain, cxt_ptr->sync_param[0].ev_setting.calc_y,
				cxt_ptr->sync_param[1].img_size.w, cxt_ptr->sync_param[1].img_size.h, cxt_ptr->sync_param[1].aem_roi_rect.start_x, cxt_ptr->sync_param[1].aem_roi_rect.start_y, cxt_ptr->sync_param[1].aem_roi_rect.end_x, cxt_ptr->sync_param[1].aem_roi_rect.end_y,
				cxt_ptr->sync_param[1].ev_setting.exp_time, cxt_ptr->sync_param[1].ev_setting.ae_gain, cxt_ptr->sync_param[1].ev_setting.calc_y,
				cxt_ptr->sync_param[2].img_size.w, cxt_ptr->sync_param[2].img_size.h, cxt_ptr->sync_param[2].aem_roi_rect.start_x, cxt_ptr->sync_param[2].aem_roi_rect.start_y, cxt_ptr->sync_param[2].aem_roi_rect.end_x, cxt_ptr->sync_param[2].aem_roi_rect.end_y,
				cxt_ptr->sync_param[2].ev_setting.exp_time, cxt_ptr->sync_param[2].ev_setting.ae_gain, cxt_ptr->sync_param[2].ev_setting.calc_y);

	if (result_ptr->log_buf) {
		pos += sprintf((char *)((char *)log_str + pos), "adv info:\n%s\n", (char *)result_ptr->log_buf);
	}
}

static cmr_s32 ae_save_to_mlog_file(struct ae_ctrl_cxt *cxt, struct ae_lib_calc_out *result)
{
	cmr_s32 rtn = 0;
	cmr_s32 mlog_switch = 0;
	char *tmp_str = (char *)cxt->debug_str;
	char mlog_ptr[PROPERTY_VALUE_MAX] = {0};
	UNUSED(result);
	memset(tmp_str, 0, sizeof(cxt->debug_str));

	property_get("persist.vendor.cam.isp.ae.mlog.switch", mlog_ptr, "");
	if (!strcmp(mlog_ptr, "on"))
		mlog_switch = 1;

	if((1 == mlog_switch)&&(1 != cxt->camera_id)){
		cmr_s32 sensor_type = 0;
		char prop_str[PROPERTY_VALUE_MAX] = {0};
		property_get("persist.vendor.cam.isp.ae.sensor.type", prop_str, "");
		if (!strcmp(prop_str, "w"))
			sensor_type = 0;
		else if (!strcmp(prop_str, "uw"))
			sensor_type = 2;
		else if (!strcmp(prop_str, "t"))
			sensor_type = 3;
		if(sensor_type != cxt->camera_id)
			return rtn;
	}
	rtn = debug_file_open((debug_handle_t) cxt->debug_info_handle, 1, 0);
	if (0 == rtn) {
		ae_print_debug_info(tmp_str, cxt);
		debug_file_print((debug_handle_t) cxt->debug_info_handle, tmp_str);
		debug_file_close((debug_handle_t) cxt->debug_info_handle);
	}

	return rtn;
}

static cmr_u32 ae_get_checksum(void)
{
#define AE_CHECKSUM_FLAG 1024
	cmr_u32 checksum = 0;

	checksum = (sizeof(struct ae_ctrl_cxt)) % AE_CHECKSUM_FLAG;

	return checksum;
}

static cmr_s32 ae_check_handle(cmr_handle handle)
{
	struct ae_ctrl_cxt *cxt = (struct ae_ctrl_cxt *)handle;
	cmr_u32 checksum = 0;
	if (NULL == handle) {
		ISP_LOGE("fail to check handle");
		return AE_ERROR;
	}
	checksum = ae_get_checksum();
	if ((AE_START_ID != cxt->start_id) || (AE_END_ID != cxt->end_id) || (checksum != cxt->checksum)) {
		ISP_LOGE("fail to get checksum, start_id:%d, end_id:%d, check sum:%d\n", cxt->start_id, cxt->end_id, cxt->checksum);
		return AE_ERROR;
	}

	return AE_SUCCESS;
}

static cmr_s32 ae_update_monitor_unit(struct ae_ctrl_cxt *cxt, struct ae_trim *trim)
{
	cmr_s32 rtn = AE_SUCCESS;
	struct ae_stats_monitor_cfg *unit = NULL;

	if (NULL == cxt || NULL == trim) {
		ISP_LOGE("fail to update monitor unit, cxt=%p, work_info=%p", cxt, trim);
		return AE_ERROR;
	}
	unit = &cxt->monitor_cfg;

	if (unit) {
		unit->blk_size.w = ((trim->w / unit->blk_num.w) / 2) * 2;
		unit->blk_size.h = ((trim->h / unit->blk_num.h) / 2) * 2;
		unit->trim.w = unit->blk_size.w * unit->blk_num.w;
		unit->trim.h = unit->blk_size.h * unit->blk_num.h;
		unit->trim.x = trim->x + (trim->w - unit->trim.w) / 2;
		unit->trim.x = (unit->trim.x / 2) * 2;
		unit->trim.y = trim->y + (trim->h - unit->trim.h) / 2;
		unit->trim.y = (unit->trim.y / 2) * 2;
	}
	return rtn;
}

static cmr_s32 ae_update_bayer_hist(struct ae_ctrl_cxt *cxt, struct ae_rect *hist_rect)
{
	cmr_s32 rtn = AE_SUCCESS;
	struct ae_bayer_hist_cfg *bhist = NULL;

	if (NULL == cxt || NULL == hist_rect) {
		ISP_LOGE("fail to update monitor unit, cxt=%p, work_info=%p", cxt, hist_rect);
		return AE_ERROR;
	}
	bhist = &cxt->bhist_cfg;

	if (bhist) {
		bhist->hist_rect.start_x = hist_rect->start_x;
		bhist->hist_rect.start_y = hist_rect->start_y;
		bhist->hist_rect.end_x = hist_rect->end_x;
		bhist->hist_rect.end_y = hist_rect->end_y;
	}

	return rtn;
}


cmr_s32 iso_shutter_mapping[7][15] = {
	// 50 ,64 ,80 ,100
	// ,125
	// ,160 ,200 ,250 ,320
	// ,400 ,500 ,640 ,800
	// ,1000,1250
	{128, 170, 200, 230, 270, 300, 370, 490, 600, 800, 950, 1210, 1500, 1900, 2300}
	,							// 1/17
	{128, 170, 200, 230, 240, 310, 370, 450, 600, 800, 950, 1210, 1500, 1900, 2300}
	,							// 1/20
	{128, 170, 200, 230, 240, 330, 370, 450, 600, 800, 950, 1210, 1500, 1900, 2300}
	,							// 1/25
	{128, 170, 183, 228, 260, 320, 370, 450, 600, 800, 950, 1210, 1500, 1900, 2300}
	,							// 1/33
	{128, 162, 200, 228, 254, 320, 370, 450, 600, 800, 950, 1210, 1500, 1900, 2300}
	,							// 1/50
	{128, 162, 207, 255, 254, 320, 370, 450, 600, 800, 950, 1210, 1500, 1900, 2300}
	,							// 1/100
	{128, 190, 207, 245, 254, 320, 370, 450, 600, 800, 950, 1210, 1500, 1900, 2300}
};

static cmr_s32 ae_get_iso(struct ae_ctrl_cxt *cxt, cmr_u32 * real_iso)
{
	cmr_s32 rtn = AE_SUCCESS;
	cmr_s32 iso = 0;
	cmr_s32 calc_iso = 0;
	float real_gain = 0;
	float tmp_iso = 0;

	if (NULL == cxt || NULL == real_iso) {
		ISP_LOGE("fail to get iso, cxt %p real_iso %p", cxt, real_iso);
		return AE_ERROR;
	}

	iso = cxt->cur_status.adv_param.iso;
	real_gain = cxt->cur_result.ev_setting.ae_gain;

	if (AE_ISO_AUTO == iso) {
		tmp_iso = real_gain * 5000 / 128;
		calc_iso = 0;
		if (tmp_iso < 1122) {
			calc_iso = 10;
		} else if (tmp_iso < 1414) {
			calc_iso = 12;
		} else if (tmp_iso < 1782) {
			calc_iso = 16;
		} else if (tmp_iso < 2245) {
			calc_iso = 20;
		} else if (tmp_iso < 2828) {
			calc_iso = 25;
		} else if (tmp_iso < 3564) {
			calc_iso = 32;
		} else if (tmp_iso < 4490) {
			calc_iso = 40;
		} else if (tmp_iso < 5657) {
			calc_iso = 50;
		} else if (tmp_iso < 7127) {
			calc_iso = 64;
		} else if (tmp_iso < 8909) {
			calc_iso = 80;
		} else if (tmp_iso < 11220) {
			calc_iso = 100;
		} else if (tmp_iso < 14140) {
			calc_iso = 125;
		} else if (tmp_iso < 17820) {
			calc_iso = 160;
		} else if (tmp_iso < 22450) {
			calc_iso = 200;
		} else if (tmp_iso < 28280) {
			calc_iso = 250;
		} else if (tmp_iso < 35640) {
			calc_iso = 320;
		} else if (tmp_iso < 44900) {
			calc_iso = 400;
		} else if (tmp_iso < 56570) {
			calc_iso = 500;
		} else if (tmp_iso < 71270) {
			calc_iso = 640;
		} else if (tmp_iso < 89090) {
			calc_iso = 800;
		} else if (tmp_iso < 112200) {
			calc_iso = 1000;
		} else if (tmp_iso < 141400) {
			calc_iso = 1250;
		} else if (tmp_iso < 178200) {
			calc_iso = 1600;
		} else if (tmp_iso < 224500) {
			calc_iso = 2000;
		} else if (tmp_iso < 282800) {
			calc_iso = 2500;
		} else if (tmp_iso < 356400) {
			calc_iso = 3200;
		} else if (tmp_iso < 449000) {
			calc_iso = 4000;
		} else if (tmp_iso < 565700) {
			calc_iso = 5000;
		} else if (tmp_iso < 712700) {
			calc_iso = 6400;
		} else if (tmp_iso < 890900) {
			calc_iso = 8000;
		} else if (tmp_iso < 1122000) {
			calc_iso = 10000;
		} else if (tmp_iso < 1414000) {
			calc_iso = 12500;
		} else if (tmp_iso < 1782000) {
			calc_iso = 16000;
		}
	} else {
		calc_iso = (1 << (iso - 1)) * 100;
	}

	*real_iso = calc_iso;
	return rtn;
}

static cmr_s32 ae_get_bv_by_lum_new(struct ae_ctrl_cxt *cxt, cmr_s32 * bv)
{
	cmr_s32 rtn = AE_SUCCESS;

	if (NULL == cxt || NULL == bv) {
		ISP_LOGE("fail to get bv by lum, cxt %p bv %p", cxt, bv);
		return AE_ERROR;
	}

	*bv = cxt->sync_cur_result.cur_bv;
	ISP_LOGV("real bv %d", *bv);
	return rtn;
}

static cmr_s32 ae_get_bv_by_lum(struct ae_ctrl_cxt *cxt, cmr_s32 * bv)
{
	cmr_s32 rtn = AE_SUCCESS;
	cmr_u32 cur_lum = 0;
	float real_gain = 0;
	cmr_u32 cur_exp = 0;

	if (NULL == cxt || NULL == bv) {
		ISP_LOGE("fail to get bv by lum, cxt %p bv %p", cxt, bv);
		return AE_ERROR;
	}

	*bv = 0;
	cur_lum = cxt->cur_result.cur_lum;
	cur_exp = cxt->cur_result.ev_setting.exp_time;
	real_gain = ae_get_real_gain(cxt->cur_result.ev_setting.ae_gain);

	if (0 == cur_lum)
		cur_lum = 1;
	*bv = (cmr_s32) (log2((double)(cur_exp * cur_lum) / (double)(real_gain * 5)) * 16.0 + 0.5);

	return rtn;
}

static float ae_get_real_gain(cmr_u32 gain)
{
	float real_gain = 0;

	real_gain = gain * 1.0 / 8.0;	// / 128 * 16;

	return real_gain;
}

static cmr_s32 ae_cfg_monitor_bypass(struct ae_ctrl_cxt *cxt)
{
	cmr_s32 rtn = AE_SUCCESS;

	if (cxt->isp_ops.set_monitor_bypass) {
		cmr_u32 is_bypass = 0;

		is_bypass = cxt->monitor_cfg.bypass;
		rtn = cxt->isp_ops.set_monitor_bypass(cxt->isp_ops.isp_handler, is_bypass);
	}

	return rtn;
}

static cmr_s32 ae_cfg_monitor_block(struct ae_ctrl_cxt *cxt)
{
	cmr_s32 rtn = AE_SUCCESS;
	struct ae_monitor_info info;

	info.win_num = cxt->monitor_cfg.blk_num;
	info.win_size = cxt->monitor_cfg.blk_size;
	info.trim = cxt->monitor_cfg.trim;
	info.work_mode = cxt->monitor_cfg.mode;
	info.skip_num = cxt->monitor_cfg.skip_num;
	info.high_region_thrd = cxt->monitor_cfg.high_region_thrd;
	info.low_region_thrd = cxt->monitor_cfg.low_region_thrd;

	if (CAM_SENSOR_MASTER == cxt->sensor_role) {
		rtn = cxt->ptr_isp_br_ioctrl(CAM_SENSOR_MASTER, SET_AE_BLOCK_SIZE, &info.win_size, NULL);
		rtn = cxt->ptr_isp_br_ioctrl(CAM_SENSOR_MASTER, SET_AE_WINDOW_RECT, &info.trim, NULL);
	} else if (CAM_SENSOR_SLAVE0 == cxt->sensor_role) {
		rtn = cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE0, SET_AE_BLOCK_SIZE, &info.win_size, NULL);
		rtn = cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE0, SET_AE_WINDOW_RECT, &info.trim, NULL);
	} else if (CAM_SENSOR_SLAVE1 == cxt->sensor_role) {
		rtn = cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE1, SET_AE_BLOCK_SIZE, &info.win_size, NULL);
		rtn = cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE1, SET_AE_WINDOW_RECT, &info.trim, NULL);
	}

	if (cxt->isp_ops.set_statistics_mode) {
		rtn = cxt->isp_ops.set_statistics_mode(cxt->isp_ops.isp_handler, (enum ae_statistics_mode)info.work_mode, info.skip_num);
	}
	
	if (cxt->isp_ops.set_monitor_win) {
		rtn = cxt->isp_ops.set_monitor_win(cxt->isp_ops.isp_handler, &info);
	}
	
	if (cxt->isp_ops.set_monitor_rgb_thd) {
		rtn = cxt->isp_ops.set_monitor_rgb_thd(cxt->isp_ops.isp_handler, &info);
	}//added by beth for aem
	
	if (cxt->isp_ops.set_blk_num){
		rtn = cxt->isp_ops.set_blk_num(cxt->isp_ops.isp_handler, &info.win_num);
	}

	if (cxt->isp_ops.set_monitor) {
		rtn = cxt->isp_ops.set_monitor(cxt->isp_ops.isp_handler, info.skip_num);
	}

	ISP_LOGI("rtn=%d win_num(%dx%d), win_size(%dx%d) skip=%d mode=%d",rtn,info.win_num.w,info.win_num.h, info.win_size.w,info.win_size.h,info.skip_num,info.work_mode);
	return rtn;
}

static cmr_s32 ae_set_g_stat(struct ae_ctrl_cxt *cxt, struct ae_stat_mode *stat_mode)
{
	cmr_s32 rtn = AE_SUCCESS;

	if (NULL == cxt || NULL == stat_mode) {
		ISP_LOGE("fail to set g stat, cxt %p stat_mode %p", cxt, stat_mode);
		return AE_ERROR;
	}

	rtn = ae_update_monitor_unit(cxt, &stat_mode->trim);
	if (AE_SUCCESS != rtn) {
		goto ae_set_g_stat_exit;
	}

	rtn = ae_cfg_monitor_block(cxt);

  ae_set_g_stat_exit:
	return rtn;
}

static cmr_s32 ae_set_scaler_trim(struct ae_ctrl_cxt *cxt, struct ae_trim *trim)
{
	cmr_s32 rtn = AE_SUCCESS;

	if (trim) {
		rtn = ae_update_monitor_unit(cxt, trim);
		if (AE_SUCCESS != rtn) {
			goto ae_set_scaler_trim_exit;
		}
		rtn = ae_cfg_monitor_block(cxt);
	} else {
		ISP_LOGE("trim pointer is NULL\n");
		rtn = AE_ERROR;
	}

  ae_set_scaler_trim_exit:
	return rtn;
}

static cmr_s32 ae_set_monitor(struct ae_ctrl_cxt *cxt, struct ae_trim *trim)
{
	cmr_s32 rtn = AE_SUCCESS;
	struct ae_stats_monitor_cfg *monitor_cfg = NULL;

	if (NULL == cxt || NULL == trim) {
		ISP_LOGE("fail to update monitor unit, cxt=%p, work_info=%p", cxt, trim);
		return AE_ERROR;
	}
	monitor_cfg = &cxt->monitor_cfg;

	if (monitor_cfg) {
		monitor_cfg->blk_size.w = ((trim->w / monitor_cfg->blk_num.w) / 2) * 2;
		monitor_cfg->blk_size.h = ((trim->h / monitor_cfg->blk_num.h) / 2) * 2;
		monitor_cfg->trim.w = monitor_cfg->blk_size.w * monitor_cfg->blk_num.w;
		monitor_cfg->trim.h = monitor_cfg->blk_size.h * monitor_cfg->blk_num.h;
		monitor_cfg->trim.x = trim->x + (trim->w - monitor_cfg->trim.w) / 2;
		monitor_cfg->trim.x = (monitor_cfg->trim.x / 2) * 2;
		monitor_cfg->trim.y = trim->y + (trim->h - monitor_cfg->trim.h) / 2;
		monitor_cfg->trim.y = (monitor_cfg->trim.y / 2) * 2;
	}

	if (cxt->isp_ops.set_stats_monitor) {
		cxt->isp_ops.set_stats_monitor(cxt->isp_ops.isp_handler, monitor_cfg);
	} else {
		ISP_LOGE("fail to set aem mode");
		return AE_ERROR;
	}
	return rtn;
}

static cmr_s32 ae_set_bayer_hist(struct ae_ctrl_cxt *cxt, struct ae_rect *hist_rect)
{
	cmr_s32 rtn = AE_SUCCESS;
	struct ae_bayer_hist_cfg *bhist_cfg = NULL;

	if (NULL == cxt || NULL == hist_rect) {
		ISP_LOGE("fail to update monitor unit, cxt=%p, work_info=%p", cxt, hist_rect);
		return AE_ERROR;
	}
	bhist_cfg = &cxt->bhist_cfg;

	ISP_LOGV("ae_set_bayer_hist, cameraId:%d, start_x:%d, start_y:%d, end_x:%d, end_y:%d",\
		cxt->camera_id, hist_rect->start_x, hist_rect->start_y, hist_rect->end_x, hist_rect->end_y);

	if (bhist_cfg) {
		bhist_cfg->bypass = cxt->bhist_cfg.bypass;
		bhist_cfg->mode = cxt->bhist_cfg.mode;
		bhist_cfg->skip_num = cxt->bhist_cfg.skip_num;
		bhist_cfg->hist_rect.start_x = hist_rect->start_x;
		bhist_cfg->hist_rect.start_y = hist_rect->start_y;
		bhist_cfg->hist_rect.end_x = hist_rect->end_x;
		bhist_cfg->hist_rect.end_y = hist_rect->end_y;
	}

	if (cxt->isp_ops.set_bayer_hist) {
		cxt->isp_ops.set_bayer_hist(cxt->isp_ops.isp_handler, bhist_cfg);
	} else {
		ISP_LOGE("fail to set bayer hist");
		return AE_ERROR;
	}
	return rtn;
}

static cmr_s32 ae_set_zoom_win_info(struct ae_ctrl_cxt *cxt)
{
	cmr_s32 rtn = AE_SUCCESS;
	struct ae_monitor_info info = {0};
	if (NULL == cxt) {
		ISP_LOGE("ae_set_aem_roi_st, cxt=%p", cxt);
		return AE_ERROR;
	}

	ISP_LOGV("aem_roi_st, cameraId:%d, x:%d, y:%d", cxt->camera_id, cxt->cur_result.aem_roi_st.x, cxt->cur_result.aem_roi_st.y);
	ISP_LOGV("aem_blk_size, cameraId:%d, w:%d, h:%d", cxt->camera_id, cxt->cur_result.aem_blk_size.w, cxt->cur_result.aem_blk_size.h);
	ISP_LOGV("blk_num, cameraId:%d, w:%d, h:%d", cxt->camera_id, cxt->monitor_cfg.blk_num.w, cxt->monitor_cfg.blk_num.h);

	if ((cxt->cur_result.aem_roi_st.x == cxt->sync_cur_result.aem_roi_st.x) && (cxt->cur_result.aem_roi_st.y == cxt->sync_cur_result.aem_roi_st.y)\
		&& (cxt->cur_result.aem_blk_size.w == cxt->sync_cur_result.aem_blk_size.w) && (cxt->cur_result.aem_blk_size.h == cxt->sync_cur_result.aem_blk_size.h)) {
		return rtn;
	} else {
		info.win_size.w = cxt->cur_result.aem_blk_size.w;
		info.win_size.h = cxt->cur_result.aem_blk_size.h;
		info.trim.x = cxt->cur_result.aem_roi_st.x;
		info.trim.y = cxt->cur_result.aem_roi_st.y;
		info.trim.w = cxt->monitor_cfg.blk_num.w * cxt->cur_result.aem_blk_size.w;
		info.trim.h = cxt->monitor_cfg.blk_num.h * cxt->cur_result.aem_blk_size.h;
		info.win_num.w = cxt->monitor_cfg.blk_num.w;
		info.win_num.h = cxt->monitor_cfg.blk_num.h;
	}

	if (cxt->isp_ops.set_aem_zoom_win) {
		cxt->isp_ops.set_aem_zoom_win(cxt->isp_ops.isp_handler, &info);
	} else {
		ISP_LOGE("set_aem_zoom_win");
		return AE_ERROR;
	}
	return rtn;
}


static void ae_reset_base_index(struct ae_ctrl_cxt *cxt)
{
	struct ae_exp_compensation exp_comp;
	exp_comp.comp_val=0;
	exp_comp.comp_range.min=-16;
	exp_comp.comp_range.max=16;
	exp_comp.step_numerator=1;
	exp_comp.step_denominator=8;
	ae_set_exposure_compensation(cxt,&exp_comp);
	ISP_LOGV("table_idx is Zero,fix to %d", cxt->cur_status.adv_param.cur_ev_setting.ae_idx);
}

static cmr_s32 ae_set_flash_notice(struct ae_ctrl_cxt *cxt, struct ae_flash_notice *flash_notice)
{
	cmr_s32 rtn = AE_SUCCESS;
	enum ae_flash_mode mode = 0;

	if ((NULL == cxt) || (NULL == flash_notice)) {
		ISP_LOGE("fail to set flash notice, cxt %p flash_notice %p", cxt, flash_notice);
		return AE_PARAM_NULL;
	}

	mode = flash_notice->mode;
	switch (mode) {
	case AE_FLASH_PRE_BEFORE:
		ISP_LOGD("ae_flash_status FLASH_PRE_BEFORE, cameraId:%d", cxt->camera_id);
		cxt->flash_backup.exp_line = cxt->sync_cur_result.ev_setting.exp_line;
		cxt->flash_backup.exp_time = cxt->sync_cur_result.ev_setting.exp_time;
		cxt->flash_backup.dummy = cxt->sync_cur_result.ev_setting.dmy_line;
		cxt->flash_backup.gain = cxt->sync_cur_result.ev_setting.ae_gain;
		cxt->flash_backup.line_time = cxt->cur_status.adv_param.cur_ev_setting.line_time;
		ISP_LOGD("ae_flash_status FLASH_PRE_BEFORE, backupdata, cameraId:%d, exp_line:%d, exp_time:%d, dummy:%d, gain:%d",
				cxt->camera_id, cxt->flash_backup.exp_line, cxt->flash_backup.exp_time, cxt->flash_backup.dummy, cxt->flash_backup.gain);

		cxt->cur_flicker = cxt->sync_cur_status.adv_param.flicker;
		cxt->flash_backup.cur_index = cxt->sync_cur_result.ev_setting.ae_idx;
		cxt->flash_backup.bv = cxt->cur_result.cur_bv;

		if ((0 != cxt->flash_ver) && (0 == cxt->exposure_compensation.ae_compensation_flag))
			rtn = ae_set_force_pause_flash(cxt, 1);
		if (cxt->exposure_compensation.ae_compensation_flag)  {
			cxt->flash_backup.table_idx = cxt->cur_status.adv_param.mode_param.value.ae_idx;
			ISP_LOGV("AE_FLASH_PRE_BEFORE force ae's table_idx : %d", cxt->flash_backup.table_idx);
		}

		if ((cxt->cur_status.adv_param.mode_param.mode == AE_MODE_AUTO_ISO_PRI) && (CAMERA_MODE_MANUAL == cxt->app_mode)) {
			cxt->cur_status.adv_param.mode_param.mode = AE_MODE_MANUAL_EXP_GAIN;
			cxt->flash_backup.mode = AE_MODE_AUTO_ISO_PRI;
			ISP_LOGD("AE_FLASH_PRE_BEFORE back mode: %d, manual_mode:%d", cxt->flash_backup.mode, cxt->cur_status.adv_param.mode_param.mode);
		}

		cxt->send_once[0] = cxt->send_once[1] = cxt->send_once[2] = cxt->send_once[3] = cxt->send_once[4] = 0;
		cxt->cur_status.adv_param.flash = FLASH_PRE_BEFORE;
		break;

	case AE_FLASH_PRE_LIGHTING:
		ISP_LOGD("ae_flash_status FLASH_PRE_LIGHTING, cameraId:%d", cxt->camera_id);
		if(cxt->cur_status.adv_param.flash != FLASH_PRE_BEFORE){
			ISP_LOGE("previous cxt->cur_status.adv_param.flash:%d, SHOULD BE FLASH_PRE_BEFORE",cxt->cur_status.adv_param.flash);
			rtn = AE_ERROR;
			break;
		}
		if (0 != cxt->flash_ver) {
			/*lock AE algorithm */
			cxt->pre_flash_skip = 0;//cxt->flash_timing_param.pre_param_update_delay;
			cxt->aem_effect_delay = 0;
		}
		cxt->cur_status.adv_param.flash = FLASH_PRE;
		break;

	case AE_FLASH_PRE_AFTER:
		ISP_LOGD("ae_flash_status FLASH_PRE_AFTER, cameraId:%d", cxt->camera_id);
		if(cxt->cur_status.adv_param.flash != FLASH_PRE){
			ISP_LOGE("previous cxt->cur_status.adv_param.flash:%d, SHOULD BE FLASH_PRE",cxt->cur_status.adv_param.flash);
			rtn = AE_ERROR;
			break;
		}

		if ((CAMERA_MODE_MANUAL == cxt->app_mode) && (cxt->flash_backup.mode == AE_MODE_AUTO_ISO_PRI)) {
			cxt->cur_status.adv_param.mode_param.mode = AE_MODE_AUTO_ISO_PRI;
		}

		cxt->cur_result.ev_setting.exp_time = cxt->flash_backup.exp_time;
		cxt->cur_result.ev_setting.exp_line = cxt->flash_backup.exp_line;
		cxt->cur_result.ev_setting.ae_gain = cxt->flash_backup.gain;
		cxt->cur_result.ev_setting.dmy_line = cxt->flash_backup.dummy;
		cxt->cur_result.ev_setting.ae_idx = cxt->flash_backup.cur_index;
		cxt->cur_result.cur_bv = cxt->flash_backup.bv;
		cxt->cur_result.ev_setting.frm_len = cxt->flash_backup.frm_len;

		cxt->sync_cur_result.ev_setting.exp_time = cxt->cur_result.ev_setting.exp_time;
		cxt->sync_cur_result.ev_setting.exp_line = cxt->cur_result.ev_setting.exp_line;
		cxt->sync_cur_result.ev_setting.ae_gain = cxt->cur_result.ev_setting.ae_gain;
		cxt->sync_cur_result.ev_setting.dmy_line = cxt->cur_result.ev_setting.dmy_line;
		cxt->sync_cur_result.ev_setting.ae_idx = cxt->cur_result.ev_setting.ae_idx;
		cxt->sync_cur_result.cur_bv = cxt->cur_result.cur_bv;
		cxt->sync_cur_result.ev_setting.frm_len = cxt->cur_result.ev_setting.frm_len;

		memset((void *)&cxt->exp_data, 0, sizeof(cxt->exp_data));
		cxt->exp_data.lib_data.exp_line = cxt->sync_cur_result.ev_setting.exp_line;
		cxt->exp_data.lib_data.exp_time = cxt->sync_cur_result.ev_setting.exp_time;
		cxt->exp_data.lib_data.gain = cxt->sync_cur_result.ev_setting.ae_gain;
		cxt->exp_data.lib_data.dummy = cxt->sync_cur_result.ev_setting.dmy_line;
		cxt->exp_data.lib_data.line_time = cxt->cur_status.adv_param.cur_ev_setting.line_time;
		cxt->exp_data.lib_data.frm_len = cxt->sync_cur_result.ev_setting.frm_len;

		rtn = ae_update_result_to_sensor(cxt, &cxt->exp_data, 0);
		if((0==cxt->cur_status.adv_param.cur_ev_setting.ae_idx)&&(0 == cxt->app_mode)){
			//ae_reset_base_index(cxt);
			cxt->flash_backup.table_idx = cxt->cur_status.adv_param.cur_ev_setting.ae_idx;//注意此处
		}

		if ((0 != cxt->flash_ver) && (0 == cxt->exposure_compensation.ae_compensation_flag)) {
			//ae_reset_base_index(cxt);
			rtn = ae_set_force_pause_flash(cxt, 0);
			ISP_LOGD("flash unlock AE_SET_FORCE_RESTORE");
		}

		cxt->send_once[0] = cxt->send_once[1] = cxt->send_once[2] = cxt->send_once[3] = cxt->send_once[4] = cxt->send_once[5] = 0;
		cxt->cur_status.adv_param.flash = FLASH_PRE_AFTER;
		cxt->env_cum_changedCalc_delay_cnt = 0;
		cxt->env_cum_changed = 0;
		break;

	case AE_FLASH_MAIN_BEFORE:
		ISP_LOGD("ae_flash_status FLASH_MAIN_BEFORE, cameraId:%d", cxt->camera_id);
		if((cxt->cur_status.adv_param.flash != FLASH_PRE_AFTER)&&(cxt->cur_status.adv_param.flash != FLASH_NONE)){
			ISP_LOGE("previous cxt->cur_status.adv_param.flash:%d, SHOULD BE FLASH_PRE_AFTER",cxt->cur_status.adv_param.flash);
			rtn = AE_ERROR;
			break;
		}

		if ((cxt->cur_status.adv_param.mode_param.mode == AE_MODE_AUTO_ISO_PRI) && (CAMERA_MODE_MANUAL == cxt->app_mode)) {
			cxt->cur_status.adv_param.mode_param.mode = AE_MODE_MANUAL_EXP_GAIN;
		}

		if ((0 != cxt->flash_ver) && (0 == cxt->exposure_compensation.ae_compensation_flag))
			rtn = ae_set_force_pause(cxt, 1, 1);

		ISP_LOGV("AE_FLASH_MAIN_BEFORE table_idx is %d, manual_mode:%d ", cxt->flash_backup.table_idx,cxt->cur_status.adv_param.mode_param.mode);
		cxt->cur_status.adv_param.flash = FLASH_MAIN_BEFORE;
		break;

	case AE_FLASH_MAIN_CLOSE:
		ISP_LOGD("ae_flash_status FLASH_MAIN_CLOSE, cameraId:%d", cxt->camera_id);
		if(cxt->cur_status.adv_param.flash != FLASH_MAIN){
			ISP_LOGE("previous cxt->cur_status.adv_param.flash:%d, SHOULD BE FLASH_MAIN",cxt->cur_status.adv_param.flash);
			rtn = AE_ERROR;
			break;
		}
		cxt->cur_status.adv_param.flash = FLASH_MAIN_CLOSE;

		break;

	case AE_FLASH_MAIN_AFTER:
		ISP_LOGD("ae_flash_status FLASH_MAIN_AFTER, cameraId:%d", cxt->camera_id);
		if(cxt->cur_status.adv_param.flash != FLASH_MAIN_CLOSE){
			ISP_LOGE("previous cxt->cur_status.adv_param.flash:%d, SHOULD BE FLASH_MAIN_CLOSE",cxt->cur_status.adv_param.flash);
			rtn = AE_ERROR;
			break;
		}
		cxt->has_mf = 1;
		if (cxt->exposure_compensation.ae_compensation_flag) {
			cxt->cur_status.adv_param.mode_param.mode = AE_MODE_AUTO;
			//cxt->cur_status.adv_param.cur_ev_setting.ae_idx = cxt->flash_backup.table_idx;//注意此处
			ISP_LOGV("AE_FLASH_MAIN_AFTER restore ae's table_idx : %d, cur manual_mode:%d", cxt->flash_backup.table_idx,cxt->cur_status.adv_param.mode_param.mode);
		}

		if ((0 != cxt->flash_ver) && (0 == cxt->exposure_compensation.ae_compensation_flag))
			rtn = ae_set_force_pause(cxt, 0, 2);

		if ((CAMERA_MODE_MANUAL == cxt->app_mode) && (cxt->flash_backup.mode == AE_MODE_AUTO_ISO_PRI)) {
			cxt->cur_status.adv_param.mode_param.mode = AE_MODE_AUTO_ISO_PRI;			
			cxt->flash_backup.mode = AE_MODE_AUTO;
			ISP_LOGD("flash_backup mode reset");
		}

		cxt->send_once[0] = cxt->send_once[1] = cxt->send_once[2] = cxt->send_once[3] = cxt->send_once[4] = cxt->send_once[5] = 0;
		cxt->cur_status.adv_param.flash = FLASH_MAIN_AFTER;
		break;

	case AE_FLASH_MAIN_AE_MEASURE:
		ISP_LOGV("ae_flash_status FLASH_MAIN_AE_MEASURE");
		break;

	case AE_FLASH_MAIN_LIGHTING:
		ISP_LOGD("ae_flash_status FLASH_MAIN_LIGHTING, cameraId:%d", cxt->camera_id);
		if(cxt->cur_status.adv_param.flash != FLASH_MAIN_BEFORE){
			ISP_LOGE("previous cxt->cur_status.adv_param.flash:%d, SHOULD BE FLASH_MAIN_BEFORE",cxt->cur_status.adv_param.flash);
			rtn = AE_ERROR;
			break;
		}
		if (0 != cxt->flash_ver) {
			cxt->cur_status.adv_param.flash = FLASH_MAIN;
		}
		break;

	case AE_LED_FLASH_ON:
		cxt->led_state = 1;
		cxt->cur_status.adv_param.flash = FLASH_LED_ON;
		ISP_LOGD("ae_flash_status FLASH_LED_ON");
		break;

	case AE_LED_FLASH_OFF:
		cxt->led_state = 0;
		cxt->cur_status.adv_param.flash = FLASH_NONE;
		ISP_LOGV("ae_flash_status FLASH_LED_OFF");
		break;

	case AE_LED_FLASH_AUTO:
		cxt->led_state = 0;
		cxt->cur_status.adv_param.flash = FLASH_LED_AUTO;
		ISP_LOGV("ae_flash_status FLASH_LED_AUTO");
		break;

	default:
		rtn = AE_ERROR;
		break;
	}
	return rtn;
}

static cmr_s32 ae_crtl_set_flash_mode (struct ae_ctrl_cxt *cxt,cmr_handle param )
{
	cmr_s32 rtn = AE_SUCCESS;

	if ((NULL == cxt) || (NULL == param)) {
		ISP_LOGE("fail to set flash mode, cxt %p ", cxt);
		return AE_PARAM_NULL;
	}

	cmr_s32 *flash_mode = (cmr_s32 *) param;
	cxt->cur_status.adv_param.flash_mode = *flash_mode;

	return rtn;
}

static cmr_s32 ae_cfg_monitor(struct ae_ctrl_cxt *cxt)
{
	cmr_s32 rtn = AE_SUCCESS;

	ae_cfg_monitor_block(cxt);

	ae_cfg_monitor_bypass(cxt);

	return rtn;
}

static cmr_s32 ae_set_online_ctrl(struct ae_ctrl_cxt *cxt, cmr_handle param)
{
	cmr_s32 rtn = AE_SUCCESS;
	struct ae_online_ctrl *ae_ctrl_ptr = (struct ae_online_ctrl *)param;

	if ((NULL == cxt) || (NULL == param)) {
		ISP_LOGE("fail to set ae online ctrl, in %p out %p", cxt, param);
		return AE_PARAM_NULL;
	}

	ISP_LOGV("cam-id:%d, mode:%d, idx:%d, s:%d, g:%d\n", cxt->camera_id, ae_ctrl_ptr->mode, ae_ctrl_ptr->index, ae_ctrl_ptr->shutter, ae_ctrl_ptr->again);
	cxt->cur_status.adv_param.lock = AE_STATE_LOCKED;
	if (AE_CTRL_SET_INDEX == ae_ctrl_ptr->mode) {
		cxt->cur_status.adv_param.mode_param.mode = AE_MODE_MANUAL_IDX;	/*ae index */
		cxt->cur_status.adv_param.mode_param.value.ae_idx = ae_ctrl_ptr->index;
	} else {					/*exposure & gain */
		cxt->cur_status.adv_param.mode_param.mode = AE_MODE_MANUAL_EXP_GAIN;
		cxt->cur_status.adv_param.mode_param.value.exp_gain[0] = ae_ctrl_ptr->shutter * cxt->cur_status.adv_param.cur_ev_setting.line_time;
		cxt->cur_status.adv_param.mode_param.value.exp_gain[1] = ae_ctrl_ptr->again;
	}

	return rtn;
}

static cmr_s32 ae_get_online_ctrl(struct ae_ctrl_cxt *cxt, cmr_handle result)
{
	cmr_s32 rtn = AE_SUCCESS;
	struct ae_online_ctrl *ae_ctrl_ptr = (struct ae_online_ctrl *)result;

	if ((NULL == cxt) || (NULL == result)) {
		ISP_LOGE("fail to get ae online ctrl, in %p out %p", cxt, result);
		return AE_PARAM_NULL;
	}
	ae_ctrl_ptr->index = cxt->sync_cur_result.ev_setting.ae_idx;
	ae_ctrl_ptr->lum = cxt->sync_cur_result.cur_lum;
	ae_ctrl_ptr->shutter = cxt->sync_cur_result.ev_setting.exp_line;
	ae_ctrl_ptr->dummy = cxt->sync_cur_result.ev_setting.dmy_line;
	ae_ctrl_ptr->again = cxt->sync_cur_result.ev_setting.ae_gain;
	ae_ctrl_ptr->skipa = 0;
	ae_ctrl_ptr->skipd = 0;
	ISP_LOGV("cam-id:%d, idx:%d, s:%d, g:%d\n", cxt->camera_id, ae_ctrl_ptr->index, ae_ctrl_ptr->shutter, ae_ctrl_ptr->again);
	return rtn;
}

static cmr_s32 ae_tool_online_ctrl(struct ae_ctrl_cxt *cxt, cmr_handle param, cmr_handle result)
{
	cmr_s32 rtn = AE_SUCCESS;
	struct ae_online_ctrl *ae_ctrl_ptr = (struct ae_online_ctrl *)param;

	if (NULL == param) {
		ISP_LOGE("param is NULL\n");
		return AE_ERROR;
	}
	if ((AE_CTRL_SET_INDEX == ae_ctrl_ptr->mode)
		|| (AE_CTRL_SET == ae_ctrl_ptr->mode)) {
		rtn = ae_set_online_ctrl(cxt, param);
	} else {
		rtn = ae_get_online_ctrl(cxt, result);
	}

	return rtn;
}

static cmr_s32 ae_set_fd_param(struct ae_ctrl_cxt *cxt, cmr_handle param)
{
	if (param) {
		struct ae_fd_param *fd = (struct ae_fd_param *)param;
		cmr_s32 i = 0;

		if (fd->face_num > 0) {
			cxt->cur_status.adv_param.face_data.img_size.w = fd->width;
			cxt->cur_status.adv_param.face_data.img_size.h = fd->height;
			cxt->cur_status.adv_param.face_data.face_num = fd->face_num;
			ISP_LOGV("FD_CTRL_NUM:%d", cxt->cur_status.adv_param.face_data.face_num);
			for (i = 0; i < fd->face_num; i++) {
				cxt->cur_status.adv_param.face_data.face_data[i].face_rect.start_x = fd->face_area[i].rect.start_x;
				cxt->cur_status.adv_param.face_data.face_data[i].face_rect.start_y = fd->face_area[i].rect.start_y;
				cxt->cur_status.adv_param.face_data.face_data[i].face_rect.end_x = fd->face_area[i].rect.end_x;
				cxt->cur_status.adv_param.face_data.face_data[i].face_rect.end_y = fd->face_area[i].rect.end_y;
				cxt->cur_status.adv_param.face_data.face_data[i].pose = fd->face_area[i].pose;
				cxt->cur_status.adv_param.face_data.face_data[i].face_lum = fd->face_area[i].face_lum;
				cxt->cur_status.adv_param.face_data.face_data[i].angle = fd->face_area[i].angle;
			}
		} else {
			cxt->cur_status.adv_param.face_data.face_num = 0;
			for (i = 0; i < fd->face_num; i++) {
				cxt->cur_status.adv_param.face_data.face_data[i].face_rect.start_x = 0;
				cxt->cur_status.adv_param.face_data.face_data[i].face_rect.start_y = 0;
				cxt->cur_status.adv_param.face_data.face_data[i].face_rect.end_x = 0;
				cxt->cur_status.adv_param.face_data.face_data[i].face_rect.end_y = 0;
				cxt->cur_status.adv_param.face_data.face_data[i].pose = -1;
				cxt->cur_status.adv_param.face_data.face_data[i].face_lum = 0;
				cxt->cur_status.adv_param.face_data.face_data[i].angle = 0;
			}
		}
	} else {
		ISP_LOGE("fail to fd, param %p", param);
		return AE_ERROR;
	}

	return AE_SUCCESS;
}

static cmr_s32 ae_get_exp_time(struct ae_ctrl_cxt *cxt, cmr_handle result)
{
	if (result) {
		*(float *)result = cxt->cur_result.ev_setting.exp_time / AEC_LINETIME_PRECESION;
	}
	return AE_SUCCESS;
}

static cmr_s32 ae_get_metering_mode(struct ae_ctrl_cxt *cxt, cmr_handle result)
{
	if (result) {
		*(cmr_s8 *) result = cxt->cur_status.adv_param.metering_mode;
	}

	return AE_SUCCESS;
}

static cmr_s32 ae_bypass_algorithm(struct ae_ctrl_cxt *cxt, cmr_handle param)
{
	if (param) {
		if (1 == *(cmr_s32 *) param) {
			cxt->bypass = 1;
		} else {
			cxt->cur_status.adv_param.lock = AE_STATE_NORMAL;
			cxt->bypass = 0;
		}
	}

	return AE_SUCCESS;
}

static cmr_s32 ae_printf_status_log(struct ae_ctrl_cxt *cxt, cmr_s8 scene_mod, struct ae_lib_calc_in *alg_status)
{
	cmr_s32 ret = AE_SUCCESS;
	UNUSED(cxt);
	ISP_LOGV("scene: %d\n", scene_mod);
	ISP_LOGV("iso: %d\n", alg_status->adv_param.iso);
	ISP_LOGV("ev offset: %d\n", alg_status->adv_param.comp_param.value.ev_index);
	ISP_LOGV("fps: [%d, %d]\n", alg_status->adv_param.fps_range.min, alg_status->adv_param.fps_range.max);
	return ret;
}

static cmr_s32 ae_set_manual_mode(struct ae_ctrl_cxt *cxt, cmr_handle param)
{
	cmr_s32 rtn = AE_SUCCESS;

	if (param) {
		if(*(cmr_u32 *) param == cxt->manual_ae_mode){
			ISP_LOGD("ae mode no change,no need to set");
			return AE_SUCCESS;
			}
		cxt->manual_ae_mode = *(cmr_u32 *) param;
		if (!cxt->manual_ae_mode) {
			if(cxt->manual_iso_value){
				cxt->cur_status.adv_param.mode_param.mode = AE_MODE_MANUAL_EXP_GAIN;
				ae_set_force_pause(cxt, 1, 3);
			}else{
				cxt->cur_status.adv_param.mode_param.mode = AE_MODE_AUTO_SHUTTER_PRI;
				ae_set_force_pause(cxt, 0, 4);
			}
			cxt->cur_status.adv_param.mode_param.value.exp_gain[0] = cxt->manual_exp_line_bkup ;
		}else if (0 == cxt->exposure_compensation.ae_compensation_flag) {//on
			ae_set_force_pause(cxt, 0, 5);
			cxt->manual_exp_time = 0;
			if(cxt->manual_iso_value){
				cxt->cur_status.adv_param.mode_param.value.exp_gain[1] = cxt->manual_iso_value;
				cxt->cur_status.adv_param.mode_param.mode = AE_MODE_AUTO_ISO_PRI;
			}else{
				cxt->cur_status.adv_param.mode_param.mode = AE_MODE_AUTO;
			}
		}
		ISP_LOGD("manual_mode %d, manual_iso_value %d ,exp %d,mode %d", *(cmr_u32 *) param,cxt->manual_iso_value,cxt->cur_status.adv_param.mode_param.value.exp_gain[0],cxt->cur_status.adv_param.mode_param.mode);
	}
	return rtn;
}

static cmr_s32 ae_set_exp_time(struct ae_ctrl_cxt *cxt, cmr_handle param)
{
	cmr_s32 rtn = AE_SUCCESS;
	cmr_u32 exp_time = 10000000;

	if (param) {
		exp_time = *(cmr_u32 *) param;
		if (exp_time > 0) {
			if(cxt->manual_exp_time == exp_time){
				ISP_LOGD("shutter no change,no need to set");
				return AE_SUCCESS;
			}
			cxt->manual_exp_time = exp_time;
		}
		if(cxt->manual_iso_value){
			ae_set_force_pause(cxt, 1, 6);
			cxt->cur_status.adv_param.mode_param.mode = AE_MODE_MANUAL_EXP_GAIN;
			cxt->cur_status.adv_param.mode_param.value.exp_gain[1] = cxt->manual_iso_value;

		}else{
			ae_set_force_pause(cxt, 0, 7);
			cxt->cur_status.adv_param.mode_param.mode = AE_MODE_AUTO_SHUTTER_PRI;
		}
		cxt->cur_status.adv_param.mode_param.value.exp_gain[0] = cxt->manual_exp_time;
		cxt->manual_exp_line_bkup = cxt->manual_exp_time;
		ISP_LOGD("manual_exp_time %d, manual_iso_value %d, exp %d, mode %d",cxt->manual_exp_time,cxt->manual_iso_value, cxt->cur_status.adv_param.mode_param.value.exp_gain[0], cxt->cur_status.adv_param.mode_param.mode);
	}
	return rtn;
}

static cmr_s32 ae_set_manual_iso(struct ae_ctrl_cxt *cxt, cmr_handle param)
{
	UNUSED(cxt);
	UNUSED(param);
	ISP_LOGD("Not support !");
	return AE_SUCCESS;
}


static cmr_s32 ae_set_gain(struct ae_ctrl_cxt *cxt, cmr_handle param)
{
	cmr_s32 rtn = AE_SUCCESS;
	if (param) {
                cmr_u32 sensitivity = *(cmr_u32 *)param;
		if (AE_STATE_LOCKED == cxt->cur_status.adv_param.lock) {
			cxt->cur_status.adv_param.mode_param.value.exp_gain[1] = sensitivity * 128 / 50;
		}
	}
	return rtn;
}

static cmr_s32 ae_set_force_pause_flash(struct ae_ctrl_cxt *cxt, cmr_u32 enable)
{
	cmr_s32 ret = AE_SUCCESS;

	if (enable) {
		cxt->exposure_compensation.ae_base_idx = cxt->sync_cur_result.ev_setting.ae_idx;
		ISP_LOGD("ae_base_idx:%d ",cxt->exposure_compensation.ae_base_idx);
		cxt->cur_status.adv_param.lock = AE_STATE_LOCKED;
	} else {
		if (2 > cxt->pause_cnt) {
			cxt->cur_status.adv_param.lock = AE_STATE_NORMAL;
			cxt->pause_cnt = 0;
			if(cxt->cur_status.adv_param.mode_param.mode != AE_MODE_AUTO_SHUTTER_PRI)
				cxt->cur_status.adv_param.mode_param.value.exp_gain[0] = 0;
			if(cxt->cur_status.adv_param.mode_param.mode != AE_MODE_AUTO_ISO_PRI)
				cxt->cur_status.adv_param.mode_param.value.exp_gain[1] = 0;

			if(AE_MODE_MANUAL_IDX == cxt->cur_status.adv_param.mode_param.mode)
				cxt->cur_status.adv_param.mode_param.mode = AE_MODE_AUTO;
		}
	}
	cxt->force_lock_ae = enable;
	ISP_LOGD("cameraId:%d, PAUSE COUNT IS %d, lock: %d, %d, manual_mode:%d", cxt->camera_id, cxt->pause_cnt, cxt->cur_status.adv_param.lock, cxt->force_lock_ae,cxt->cur_status.adv_param.mode_param.mode);
	return ret;
}

static cmr_s32 ae_set_force_pause(struct ae_ctrl_cxt *cxt, cmr_u32 enable, int call)
{
	cmr_s32 ret = AE_SUCCESS;

	if (enable) {
		if(!cxt->is_snapshot) {
			cxt->exposure_compensation.ae_base_idx = cxt->sync_cur_result.ev_setting.ae_idx;
			ISP_LOGD("ae_base_idx:%d ",cxt->exposure_compensation.ae_base_idx);
		}
		cxt->cur_status.adv_param.lock = AE_STATE_LOCKED;
		cxt->cur_status.adv_param.app_force_lock = AE_STATE_LOCKED;
		} else {
		if (2 > cxt->pause_cnt) {
			cxt->cur_status.adv_param.lock = AE_STATE_NORMAL;
			cxt->pause_cnt = 0;
			if(cxt->cur_status.adv_param.mode_param.mode != AE_MODE_AUTO_SHUTTER_PRI)
				cxt->cur_status.adv_param.mode_param.value.exp_gain[0] = 0;
			if(cxt->cur_status.adv_param.mode_param.mode != AE_MODE_AUTO_ISO_PRI)
				cxt->cur_status.adv_param.mode_param.value.exp_gain[1] = 0;

			if(AE_MODE_MANUAL_IDX == cxt->cur_status.adv_param.mode_param.mode)
				cxt->cur_status.adv_param.mode_param.mode = AE_MODE_AUTO;
		}
		cxt->cur_status.adv_param.app_force_lock = AE_STATE_NORMAL;
	}
	cxt->force_lock_ae = enable;
	ISP_LOGD("cameraId:%d, PAUSE COUNT IS %d, lock: %d, %d, manual_mode:%d, call = %d", cxt->camera_id,cxt->pause_cnt, cxt->cur_status.adv_param.lock, cxt->force_lock_ae,cxt->cur_status.adv_param.mode_param.mode, call);
	return ret;
}

static cmr_s32 ae_set_pause(struct ae_ctrl_cxt *cxt, int call)
{
	cmr_s32 ret = AE_SUCCESS;

	if((!cxt->is_snapshot) && (!cxt->cur_status.adv_param.lock)){
		cxt->exposure_compensation.ae_base_idx = cxt->cur_result.ev_setting.ae_idx;
		ISP_LOGD("ae_base_idx:%d",cxt->exposure_compensation.ae_base_idx);
	}
	cxt->cur_status.adv_param.lock = AE_STATE_LOCKED;
	cxt->pause_cnt++;
	ISP_LOGD("cameraId:%d, PAUSE COUNT IS %d, lock: %d, call=%d", cxt->camera_id, cxt->pause_cnt, cxt->cur_status.adv_param.lock, call);
	return ret;
}

static cmr_s32 ae_set_restore_cnt(struct ae_ctrl_cxt *cxt, int call)
{
	cmr_s32 ret = AE_SUCCESS;

	if (!cxt->pause_cnt)
		return ret;

	if (2 > cxt->pause_cnt) {
		if(AE_STATE_NORMAL == cxt->cur_status.adv_param.app_force_lock){
			cxt->cur_status.adv_param.lock = AE_STATE_NORMAL;
			cxt->pause_cnt = 0;
		}
	} else {
		cxt->pause_cnt--;
	}

	ISP_LOGD("cameraId:%d, PAUSE COUNT IS %d, lock: %d,call=%d", cxt->camera_id, cxt->pause_cnt, cxt->cur_status.adv_param.lock, call);
	return ret;
}

static cmr_s32 ae_set_skip_update(struct ae_ctrl_cxt *cxt)
{
	cmr_s32 ret = AE_SUCCESS;
	cxt->skip_update_param_flag = 1;
	if (0 == cxt->skip_update_param_cnt)
		cxt->skip_update_param_cnt++;
	ISP_LOGV("Skip_update_param: flag %d, count %d", cxt->skip_update_param_flag, cxt->skip_update_param_cnt);
	return ret;
}

static cmr_s32 ae_set_restore_skip_update_cnt(struct ae_ctrl_cxt *cxt)
{
	cmr_s32 ret = AE_SUCCESS;
	if (0 == cxt->skip_update_param_cnt)
		cxt->skip_update_param_flag = 0;
	else
		cxt->skip_update_param_cnt--;
	ISP_LOGV("Skip_update_param: flag %d, count %d", cxt->skip_update_param_flag, cxt->skip_update_param_cnt);
	return ret;
}

static int32_t ae_stats_data_preprocess(cmr_u32 * src_aem_stat, cmr_u16 * dst_aem_stat, struct ae_size aem_blk_size, struct ae_size aem_blk_num, cmr_u8 aem_shift)
{
	cmr_s32 rtn = AE_SUCCESS;
	cmr_u32 stat_blocks = aem_blk_num.w * aem_blk_num.h;
	cmr_u32 *src_r_stat = (cmr_u32 *) src_aem_stat;
	cmr_u32 *src_g_stat = (cmr_u32 *) src_aem_stat + stat_blocks;
	cmr_u32 *src_b_stat = (cmr_u32 *) src_aem_stat + 2 * stat_blocks;
	cmr_u16 *dst_r = (cmr_u16 *) dst_aem_stat;
	cmr_u16 *dst_g = (cmr_u16 *) dst_aem_stat + stat_blocks;
	cmr_u16 *dst_b = (cmr_u16 *) dst_aem_stat + stat_blocks * 2;
	cmr_u16 max_value = 1023;
	cmr_u32 i = 0;
	cmr_u16 r = 0, g = 0, b = 0;

	UNUSED(aem_blk_size);

	for (i = 0; i < stat_blocks; i++) {
/*for r channel */
		r = *src_r_stat++;
		r = r << aem_shift;
		r = r > max_value ? max_value : r;

/*for g channel */
		g = *src_g_stat++;
		g = g << aem_shift;
		g = g > max_value ? max_value : g;

/*for b channel */
		b = *src_b_stat++;
		b = b << aem_shift;
		b = b > max_value ? max_value : b;

		dst_r[i] = r;
		dst_g[i] = g;
		dst_b[i] = b;
	}

	return rtn;
}

static cmr_s32 flash_pre_start(struct ae_ctrl_cxt *cxt)
{
	cmr_s32 rtn = AE_SUCCESS;
	struct Flash_pfStartInput in;
	struct Flash_pfStartOutput out;
	struct ae_lib_calc_in *current_status = &cxt->cur_status;
	cmr_u32 blk_num = 0;
	cmr_u32 i;
	/*reset flash debug information */
	memset(&cxt->flash_debug_buf[0], 0, sizeof(cxt->flash_debug_buf));
	cxt->flash_buf_len = 0;
	memset(&cxt->flash_esti_result, 0, sizeof(cxt->flash_esti_result));	/*reset result */

	in.minExposure = cxt->ae_tbl_param.min_exp / SENSOR_LINETIME_BASE;
	in.maxExposure = cxt->ae_tbl_param.max_exp / SENSOR_LINETIME_BASE;
	if(cxt->cur_flicker == 0) {
		if(in.maxExposure > 600000) {
			in.maxExposure = 600000;
		}
	} else if(cxt->cur_flicker == 1) {
		if(in.maxExposure > 500000) {
			in.maxExposure = 500000;
		}
	}
	ISP_LOGV("flash_cur_flicker %d, max_exp %f", cxt->cur_flicker, in.maxExposure);
	in.minGain = cxt->ae_tbl_param.min_gain;
	in.maxGain = cxt->ae_tbl_param.max_gain;
	in.minCapGain = cxt->ae_tbl_param.min_gain;
	in.maxCapGain = cxt->ae_tbl_param.max_gain;
	in.minCapExposure = cxt->ae_tbl_param.min_exp / SENSOR_LINETIME_BASE;
	in.maxCapExposure = cxt->ae_tbl_param.max_exp / SENSOR_LINETIME_BASE;
	if(cxt->cur_flicker == 0) {
		if(in.maxCapExposure> 600000) {
			in.maxCapExposure= 600000;
		} else {
			in.maxCapExposure= in.maxExposure;
		}
	} else if(cxt->cur_flicker == 1) {
		if(in.maxCapExposure> 500000) {
			in.maxCapExposure= 500000;
		} else {
			in.maxCapExposure= in.maxExposure;
		}
	}

	in.aeExposure = current_status->adv_param.cur_ev_setting.exp_line * current_status->adv_param.cur_ev_setting.line_time / SENSOR_LINETIME_BASE;
	in.aeGain = current_status->adv_param.cur_ev_setting.ae_gain;

	in.rGain = current_status->awb_gain.r;
	in.gGain = current_status->awb_gain.g;
	in.bGain = current_status->awb_gain.b;
	in.isFlash = 0;				/*need to check the meaning */
	in.flickerMode = current_status->adv_param.flicker;
	in.staW = cxt->cur_status.stats_data_basic.size.w;
	in.staH = cxt->cur_status.stats_data_basic.size.h;
	blk_num = cxt->cur_status.stats_data_basic.size.w * cxt->cur_status.stats_data_basic.size.h;
	in.rSta = (cmr_u16 *) & cxt->aem_stat_rgb[0];
	in.gSta = (cmr_u16 *) & cxt->aem_stat_rgb[0] + blk_num;
	in.bSta = (cmr_u16 *) & cxt->aem_stat_rgb[0] + 2 * blk_num;

	for (i = 0; i < 20; i++) {
		in.ctTab[i] = cxt->ctTab[i];
		in.ctTabRg[i] = cxt->ctTabRg[i];
	}

	in.otp_gldn_r = cxt->awb_otp_info.gldn_stat_info.r;
	in.otp_gldn_g = cxt->awb_otp_info.gldn_stat_info.g;
	in.otp_gldn_b = cxt->awb_otp_info.gldn_stat_info.b;
	in.otp_rdm_r = cxt->awb_otp_info.rdm_stat_info.r;
	in.otp_rdm_g = cxt->awb_otp_info.rdm_stat_info.g;
	in.otp_rdm_b = cxt->awb_otp_info.rdm_stat_info.b;

	ISP_LOGV("gldn-(%d, %d, %d), rdm-(%d, %d, %d)\n",
			cxt->awb_otp_info.gldn_stat_info.r, cxt->awb_otp_info.gldn_stat_info.g, cxt->awb_otp_info.gldn_stat_info.b,
			cxt->awb_otp_info.rdm_stat_info.r, cxt->awb_otp_info.rdm_stat_info.g, cxt->awb_otp_info.rdm_stat_info.b);
	
	rtn = flash_pfStart(cxt->flash_alg_handle, &in, &out);
	out.nextExposure *= SENSOR_LINETIME_BASE;
	current_status->adv_param.mode_param.value.exp_gain[0] = (cmr_u32) (out.nextExposure);
	current_status->adv_param.mode_param.value.exp_gain[1] = out.nextGain;
	cxt->pre_flash_level1 = out.preflahLevel1;
	cxt->pre_flash_level2 = out.preflahLevel2;
	cxt->flash_last_exp_line = (cmr_u32)(1.0 * out.nextExposure / cxt->cur_status.adv_param.cur_ev_setting.line_time + 0.5);
	cxt->flash_last_gain = out.nextGain;
	ISP_LOGD("ae_flash pre_b: flashbefore(%d, %d), preled_level(%d, %d), preflash(%d, %d), flicker:%s\n",
			 current_status->adv_param.cur_ev_setting.exp_line, current_status->adv_param.cur_ev_setting.ae_gain, out.preflahLevel1, out.preflahLevel2, cxt->flash_last_exp_line, out.nextGain, in.flickerMode == 0 ? "50Hz" : "60Hz");

	return rtn;
}

static cmr_s32 flash_estimation(struct ae_ctrl_cxt *cxt)
{
	cmr_s32 rtn = AE_SUCCESS;
	cmr_u32 blk_num = 0;
	struct Flash_pfOneIterationInput *in = NULL;
	struct Flash_pfOneIterationOutput out;
	struct ae_lib_calc_in *current_status = &cxt->cur_status;
	memset((void *)&out, 0, sizeof(out));

	ISP_LOGV("pre_flash_skip %d, aem_effect_delay %d", cxt->pre_flash_skip, cxt->aem_effect_delay);

	if (1 == cxt->flash_esti_result.isEnd) {
		goto flash_estimation_exit;
	}

	if (((cmr_u32)cxt->flash_last_exp_line != current_status->adv_param.cur_ev_setting.exp_line) || (1.0 * cxt->flash_last_gain / current_status->adv_param.cur_ev_setting.ae_gain < 0.97) || (1.0 * cxt->flash_last_gain / current_status->adv_param.cur_ev_setting.ae_gain > 1.03)) {
		ISP_LOGD("ae_flash esti: sensor param do not take effect, (%d, %d)-(%d, %d)skip\n",
				 cxt->flash_last_exp_line, cxt->flash_last_gain, current_status->adv_param.cur_ev_setting.exp_line, current_status->adv_param.cur_ev_setting.ae_gain);
		goto flash_estimation_exit;
	}

	/* preflash climbing time */
	if (0 < cxt->pre_flash_skip) {
		cxt->pre_flash_skip--;
		goto flash_estimation_exit;
	}

	cxt->aem_effect_delay++;
	if (cxt->aem_effect_delay == cxt->flash_timing_param.pre_param_update_delay) {
		cxt->aem_effect_delay = 0;

		blk_num = cxt->cur_status.stats_data_basic.size.w * cxt->cur_status.stats_data_basic.size.h;
		in = &cxt->flash_esti_input;
		in->aeExposure = current_status->adv_param.cur_ev_setting.exp_line * current_status->adv_param.cur_ev_setting.line_time / SENSOR_LINETIME_BASE;
		in->aeGain = current_status->adv_param.cur_ev_setting.ae_gain;
		in->staW = cxt->cur_status.stats_data_basic.size.w;
		in->staH = cxt->cur_status.stats_data_basic.size.h;
		in->isFlash = 1;		/*need to check the meaning */
		memcpy((cmr_handle *) & in->rSta[0], (cmr_u16 *) & cxt->aem_stat_rgb[0], sizeof(in->rSta));
		memcpy((cmr_handle *) & in->gSta[0], ((cmr_u16 *) & cxt->aem_stat_rgb[0] + blk_num), sizeof(in->gSta));
		memcpy((cmr_handle *) & in->bSta[0], ((cmr_u16 *) & cxt->aem_stat_rgb[0] + 2 * blk_num), sizeof(in->bSta));

		flash_pfOneIteration(cxt->flash_alg_handle, in, &out);

		out.nextExposure *= SENSOR_LINETIME_BASE;
		out.captureExposure *= SENSOR_LINETIME_BASE;
		current_status->adv_param.mode_param.value.exp_gain[0] = (cmr_u32)out.nextExposure;
		current_status->adv_param.mode_param.value.exp_gain[1] = out.nextGain;
		cxt->flash_esti_result.isEnd = 0;
		ISP_LOGD("ae_flash esti: doing %d, %d", cxt->cur_status.adv_param.mode_param.value.exp_gain[0], cxt->cur_status.adv_param.mode_param.value.exp_gain[1]);

		if (1 == out.isEnd) {
			ISP_LOGD("ae_flash esti: isEnd:%d, cap(%d, %d), led(%d, %d), rgb(%d, %d, %d)\n",
					 out.isEnd, current_status->adv_param.mode_param.value.exp_gain[0], out.captureGain, out.captureFlahLevel1, out.captureFlahLevel2, out.captureRGain, out.captureGGain, out.captureBGain);

			/*save the flash estimation results */
			cxt->flash_esti_result = out;
		}

		cxt->flash_last_exp_line = (cmr_u32)(1.0 * out.nextExposure / cxt->cur_status.adv_param.cur_ev_setting.line_time + 0.5);
		cxt->flash_last_gain = current_status->adv_param.mode_param.value.exp_gain[1];
	}

  flash_estimation_exit:
	current_status->adv_param.mode_param.value.exp_gain[0] = cxt->flash_last_exp_line * cxt->cur_status.adv_param.cur_ev_setting.line_time;
	current_status->adv_param.mode_param.value.exp_gain[1] = cxt->flash_last_gain;
	return rtn;
}

static cmr_s32 flash_high_flash_reestimation(struct ae_ctrl_cxt *cxt)
{
	cmr_s32 rtn = AE_SUCCESS;
	cmr_u32 blk_num = 0;
	struct Flash_mfCalcInput *input = &cxt->flash_main_esti_input;
	struct Flash_mfCalcOutput *output = &cxt->flash_main_esti_result;
	memset((void *)input, 0, sizeof(struct Flash_mfCalcInput));
	blk_num = cxt->cur_status.stats_data_basic.size.w * cxt->cur_status.stats_data_basic.size.h;
	input->staW = cxt->cur_status.stats_data_basic.size.w;
	input->staH = cxt->cur_status.stats_data_basic.size.h;
	input->rGain = cxt->cur_status.awb_gain.r;
	input->gGain = cxt->cur_status.awb_gain.g;
	input->bGain = cxt->cur_status.awb_gain.b;
	input->wb_mode = cxt->cur_status.adv_param.awb_mode;
	memcpy((cmr_handle *) & input->rSta[0], (cmr_u16 *) & cxt->aem_stat_rgb[0], sizeof(input->rSta));
	memcpy((cmr_handle *) & input->gSta[0], ((cmr_u16 *) & cxt->aem_stat_rgb[0] + blk_num), sizeof(input->gSta));
	memcpy((cmr_handle *) & input->bSta[0], ((cmr_u16 *) & cxt->aem_stat_rgb[0] + 2 * blk_num), sizeof(input->bSta));
	flash_mfCalc(cxt->flash_alg_handle, input, output);
	ISP_LOGD("high flash wb gain: %d, %d, %d\n", cxt->flash_main_esti_result.captureRGain, cxt->flash_main_esti_result.captureGGain, cxt->flash_main_esti_result.captureBGain);

	/*save flash debug information */
	memset(&cxt->flash_debug_buf[0], 0, sizeof(cxt->flash_debug_buf));
	if (cxt->flash_esti_result.debugSize > sizeof(cxt->flash_debug_buf)) {
		cxt->flash_buf_len = sizeof(cxt->flash_debug_buf);
	} else {
		cxt->flash_buf_len = cxt->flash_esti_result.debugSize;
	}
	memcpy((cmr_handle) & cxt->flash_debug_buf[0], cxt->flash_esti_result.debugBuffer, cxt->flash_buf_len);

	return rtn;
}

static cmr_s32 flash_finish(struct ae_ctrl_cxt *cxt)
{
	cmr_s32 rtn = AE_SUCCESS;

	rtn = flash_pfEnd(cxt->flash_alg_handle);

	/*reset flash control status */
	cxt->pre_flash_skip = 0;
	cxt->flash_last_exp_line = 0;
	cxt->flash_last_gain = 0;
	memset(&cxt->flash_esti_input, 0, sizeof(cxt->flash_esti_input));
	memset(&cxt->flash_esti_result, 0, sizeof(cxt->flash_esti_result));
	memset(&cxt->flash_main_esti_input, 0, sizeof(cxt->flash_main_esti_input));
	memset(&cxt->flash_main_esti_result, 0, sizeof(cxt->flash_main_esti_result));

	return rtn;
}

static cmr_s32 ae_set_flash_charge(struct ae_ctrl_cxt *cxt, enum ae_flash_type flash_type)
{
	cmr_s32 rtn = AE_SUCCESS;
	cmr_s16 flash_level1 = 0;
	cmr_s16 flash_level2 = 0;
	struct ae_flash_cfg cfg;
	struct ae_flash_element element;

	switch (flash_type) {
	case AE_FLASH_TYPE_PREFLASH:
		flash_level1 = cxt->pre_flash_level1;
		flash_level2 = cxt->pre_flash_level2;
		cfg.type = AE_FLASH_TYPE_PREFLASH;
		break;
	case AE_FLASH_TYPE_MAIN:
		flash_level1 = cxt->flash_esti_result.captureFlahLevel1;
		flash_level2 = cxt->flash_esti_result.captureFlahLevel2;
		cfg.type = AE_FLASH_TYPE_MAIN;
		break;
	default:
		goto ae_set_flash_charge_exit;
		break;
	}

	if (flash_level1 == -1) {
		cxt->ae_leds_ctrl.led0_ctrl = 0;
	} else {
		cxt->ae_leds_ctrl.led0_ctrl = 1;
		cfg.led_idx = 1;
		cfg.multiColorLcdEn = cxt->multiColorLcdEn;
		element.index = flash_level1;
		cxt->isp_ops.flash_set_charge(cxt->isp_ops.isp_handler, &cfg, &element);
	}

	if (flash_level2 == -1) {
		cxt->ae_leds_ctrl.led1_ctrl = 0;
	} else {
		cxt->ae_leds_ctrl.led1_ctrl = 1;
		cfg.led_idx = 2;
		cfg.multiColorLcdEn = cxt->multiColorLcdEn;
		element.index = flash_level2;
		cxt->isp_ops.flash_set_charge(cxt->isp_ops.isp_handler, &cfg, &element);
	}

  ae_set_flash_charge_exit:
	return rtn;
}

static cmr_s32 ae_pre_process(struct ae_ctrl_cxt *cxt)
{
	cmr_s32 rtn = AE_SUCCESS;
	struct ae_lib_calc_in *current_status = &cxt->cur_status;

	ISP_LOGV("cameraId:%d, ae_video_fps_thr_high: %d, ae_video_fps_thr_low: %d\n", cxt->camera_id, cxt->ae_video_fps.thd_up, cxt->ae_video_fps.thd_down);

	if (AE_WORK_MODE_VIDEO == current_status->adv_param.work_mode) {
		ISP_LOGV("%d, %d, %d, %d, %d", cxt->sync_cur_result.cur_bv, current_status->adv_param.fps_range.min, current_status->adv_param.fps_range.max, cxt->fps_range.min, cxt->fps_range.max);
		if (cxt->ae_video_fps.thd_up < cxt->sync_cur_result.cur_bv) {
			current_status->adv_param.fps_range.max = cxt->fps_range.max;
			current_status->adv_param.fps_range.min = cxt->fps_range.max;
		} else if (cxt->ae_video_fps.thd_down < cxt->sync_cur_result.cur_bv) {
			current_status->adv_param.fps_range.max = cxt->fps_range.max;
			current_status->adv_param.fps_range.min = cxt->fps_range.min;
		} else {
			current_status->adv_param.fps_range.max = cxt->fps_range.min;
			current_status->adv_param.fps_range.min = cxt->fps_range.min;
		}

	} else {
		current_status->adv_param.fps_range.max = cxt->fps_range.max;
		current_status->adv_param.fps_range.min = cxt->fps_range.min;
	}

	if (cxt->cam_4in1_mode) {
		if (cxt->sync_cur_result.cur_bv < cxt->cam_4in1_switch_thrd.thd_down) {
			cxt->is_fourcell = 1;
		} else if (cxt->sync_cur_result.cur_bv > cxt->cam_4in1_switch_thrd.thd_up) {
			cxt->is_fourcell = 0;
		}

		ISP_LOGV("bv[%d, %d]:%d, 4cell:mode:%d %d\n",
				cxt->cam_4in1_switch_thrd.thd_down,
				cxt->cam_4in1_switch_thrd.thd_up,
				cxt->sync_cur_result.cur_bv,
				cxt->cam_4in1_mode,
				cxt->is_fourcell);
	}

	if (0 < cxt->cur_status.adv_param.flash) {
		ISP_LOGV("ae_flash: flicker lock to %d in flash: %d\n", cxt->cur_flicker, current_status->adv_param.flash);
		current_status->adv_param.flicker = cxt->cur_flicker;
	}

	if (0 != cxt->flash_ver) {
		if ((FLASH_PRE_BEFORE == current_status->adv_param.flash) || (FLASH_PRE == current_status->adv_param.flash)) {
			rtn = ae_stats_data_preprocess((cmr_u32 *) & cxt->sync_aem[0], (cmr_u16 *) & cxt->aem_stat_rgb[0], cxt->monitor_cfg.blk_size, cxt->cur_status.stats_data_basic.size, current_status->stats_data_basic.shift);
		}

		if ((FLASH_PRE_BEFORE == current_status->adv_param.flash) && !cxt->send_once[0]) {
			rtn = flash_pre_start(cxt);
		}

		if (FLASH_PRE == current_status->adv_param.flash) {
			rtn = flash_estimation(cxt);
		}
	}

	if ((FLASH_MAIN_BEFORE == current_status->adv_param.flash) || (FLASH_MAIN == current_status->adv_param.flash)) {
		if (cxt->flash_esti_result.isEnd) {
			uint32 capGain = 0;
			cmr_u32 capExp = 0.0;
			if(current_status->adv_param.mode_param.mode == AE_MODE_AUTO){
				capExp = (cmr_u32)cxt->flash_esti_result.captureExposure;
				capGain = cxt->flash_esti_result.captureGain;
			}
			else if(current_status->adv_param.mode_param.mode == AE_MODE_AUTO_ISO_PRI){ // shutter auto , iso fix
				capGain = cxt->sync_cur_result.ev_setting.ae_gain;
				capExp = (cmr_u32)(cxt->flash_esti_result.captureExposure) * cxt->flash_esti_result.captureGain/cxt->sync_cur_result.ev_setting.ae_gain;
			}
			else if(current_status->adv_param.mode_param.mode == AE_MODE_AUTO_SHUTTER_PRI){ // shutter fix , iso auto
				capExp = cxt->sync_cur_result.ev_setting.exp_time;
				capGain = (cmr_u32)(((float)(cxt->flash_esti_result.captureGain)) *cxt->flash_esti_result.captureExposure / ((float)(cxt->sync_cur_result.ev_setting.exp_time)));
				if(capGain > cxt->ae_tbl_param.max_gain) /*max gain is 8192*/
					capGain = cxt->ae_tbl_param.max_gain;
				ISP_LOGV("ae_flash, sync_result_exp_time:%f, flash_esti_result_captureGain:%f, flash_esti_result_captureExposure:%f", (float)cxt->sync_cur_result.ev_setting.exp_time, (float)(cxt->flash_esti_result.captureGain), cxt->flash_esti_result.captureExposure);
			}
			else if(current_status->adv_param.mode_param.mode == AE_MODE_MANUAL_EXP_GAIN){
				capExp = cxt->sync_cur_result.ev_setting.exp_time;
				capGain = cxt->sync_cur_result.ev_setting.ae_gain;
			}
			//current_status->adv_param.mode_param.mode = AE_MODE_MANUAL_EXP_GAIN;
			current_status->adv_param.mode_param.value.exp_gain[0] = capExp;
			current_status->adv_param.mode_param.value.exp_gain[1] = capGain;
			ISP_LOGD("ae_flash:mode:%d,capExp:%d, capGain:%d", current_status->adv_param.mode_param.mode, current_status->adv_param.mode_param.value.exp_gain[0], current_status->adv_param.mode_param.value.exp_gain[1]);
		} else {
			ISP_LOGE("ae_flash estimation does not work well");
		}
		if (FLASH_MAIN == current_status->adv_param.flash) {
			rtn = ae_stats_data_preprocess((cmr_u32 *) & cxt->sync_aem[0], (cmr_u16 *) & cxt->aem_stat_rgb[0], cxt->cur_status.stats_data_basic.blk_size, cxt->cur_status.stats_data_basic.size, current_status->stats_data_basic.shift);
			ISP_LOGD("main_flash, mainFlashEn:%d\n", cxt->mainFlashEn);
			if (cxt->mainFlashEn !=0) {
				flash_high_flash_reestimation(cxt);
			}
			ISP_LOGV("ae_flash: main flash calc, rgb gain %d, %d, %d\n", cxt->flash_main_esti_result.captureRGain, cxt->flash_main_esti_result.captureGGain, cxt->flash_main_esti_result.captureBGain);
		}

		ISP_LOGV("ae_flash main_b prinf: %.f, %d, %d, %d\n", cxt->flash_esti_result.captureExposure, current_status->adv_param.mode_param.value.exp_gain[0], current_status->adv_param.mode_param.value.exp_gain[1], current_status->adv_param.cur_ev_setting.line_time);		
	}

	return rtn;
}

static cmr_s32 sensor_param_updating_interface(struct ae_ctrl_cxt *cxt)
{
	cmr_s32 rtn = AE_SUCCESS;

	cxt->cur_status.adv_param.flicker = cxt->cur_flicker;
	if (cxt->camera_id && cxt->fdae.pause) {
		cxt->fdae.pause = 0;
	}
	cxt->cur_result.ev_setting.exp_time = cxt->flash_backup.exp_time;
	cxt->cur_result.ev_setting.exp_line = cxt->flash_backup.exp_line;
	cxt->cur_result.ev_setting.ae_gain = cxt->flash_backup.gain;
	cxt->cur_result.ev_setting.dmy_line = cxt->flash_backup.dummy;
	cxt->cur_result.ev_setting.ae_idx = cxt->flash_backup.cur_index;
	cxt->cur_result.cur_bv = cxt->flash_backup.bv;
	cxt->cur_result.ev_setting.frm_len = cxt->flash_backup.frm_len;

	cxt->sync_cur_result.ev_setting.exp_time = cxt->cur_result.ev_setting.exp_time;
	cxt->sync_cur_result.ev_setting.exp_line = cxt->cur_result.ev_setting.exp_line;
	cxt->sync_cur_result.ev_setting.ae_gain = cxt->cur_result.ev_setting.ae_gain;
	cxt->sync_cur_result.ev_setting.dmy_line = cxt->cur_result.ev_setting.dmy_line;
	cxt->sync_cur_result.ev_setting.ae_idx = cxt->cur_result.ev_setting.ae_idx;
	cxt->sync_cur_result.cur_bv = cxt->cur_result.cur_bv;
	cxt->sync_cur_result.ev_setting.frm_len = cxt->cur_result.ev_setting.frm_len;

	memset((void *)&cxt->exp_data, 0, sizeof(cxt->exp_data));
	cxt->exp_data.lib_data.exp_line = cxt->sync_cur_result.ev_setting.exp_line;
	cxt->exp_data.lib_data.exp_time = cxt->sync_cur_result.ev_setting.exp_time;
	cxt->exp_data.lib_data.gain = cxt->sync_cur_result.ev_setting.ae_gain;
	cxt->exp_data.lib_data.dummy = cxt->sync_cur_result.ev_setting.dmy_line;
	cxt->exp_data.lib_data.line_time = cxt->cur_status.adv_param.cur_ev_setting.line_time;
	cxt->exp_data.lib_data.frm_len = cxt->sync_cur_result.ev_setting.frm_len;

	rtn = ae_update_result_to_sensor(cxt, &cxt->exp_data, 0);

	ae_set_skip_update(cxt);
	return rtn;
}

static cmr_s32 ae_post_process(struct ae_ctrl_cxt *cxt)
{
	cmr_s32 rtn = AE_SUCCESS;
	cmr_s32 led_eb;
	cmr_s32 flash_fired;
	cmr_int cb_type;
	cmr_s32 main_flash_counts = 0;
	cmr_s32 main_flash_capture_counts = 0;

	struct ae_lib_calc_in *current_status = &cxt->sync_cur_status;

	/* for flash algorithm 0 */
	if (cxt->flash_ver) {
		/* for new flash algorithm (flash algorithm1, dual flash) */
		ISP_LOGV("cameraId:%d, pre_open_count %d, pre_close_count %d", cxt->camera_id, cxt->flash_timing_param.pre_open_delay,cxt->flash_timing_param.estimate_delay);

		if (FLASH_PRE_BEFORE_RECEIVE == cxt->cur_result.flash_status && FLASH_PRE_BEFORE == current_status->adv_param.flash) {
			cxt->send_once[0]++;
			ISP_LOGD("ae_flash1_status shake_1");
			if (cxt->flash_timing_param.pre_open_delay == cxt->send_once[0]) {
				ISP_LOGD("ae_flash p: led level: %d, %d\n", cxt->pre_flash_level1, cxt->pre_flash_level2);
				rtn = ae_set_flash_charge(cxt, AE_FLASH_TYPE_PREFLASH);

				cb_type = AE_CB_CONVERGED;
				(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, cb_type, NULL);
				cxt->cur_result.flash_status = FLASH_NONE;	/*flash status reset */
				ISP_LOGD("ae_flash1_callback do-pre-open!\r\n");
			}
		}

		if (FLASH_PRE_RECEIVE == cxt->cur_result.flash_status && FLASH_PRE == current_status->adv_param.flash) {
			ISP_LOGD("ae_flash1_status shake_2 %d %d  estimate_delay %d, flash_status:%d", cxt->cur_result.stable, cxt->cur_result.cur_lum, cxt->flash_timing_param.estimate_delay, cxt->cur_result.flash_status);
			cxt->send_once[3]++;	//prevent flash_pfOneIteration time out
			if (cxt->flash_esti_result.isEnd || cxt->send_once[3] > AE_FLASH_CALC_TIMES) {
				if (cxt->flash_timing_param.estimate_delay == cxt->send_once[1]) {
					cxt->send_once[3] = 0;
					cb_type = AE_CB_CONVERGED;
					(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, cb_type, NULL);
					cxt->cur_result.flash_status = FLASH_NONE;	/*flash status reset */
					ISP_LOGD("ae_flash1_callback do-pre-close, flash_status:%d", cxt->cur_result.flash_status);
				}
				cxt->send_once[1]++;
			}
			cxt->send_once[0] = 0;
		}

		if (FLASH_PRE_AFTER_RECEIVE == cxt->cur_result.flash_status && FLASH_PRE_AFTER == current_status->adv_param.flash) {
			cxt->cur_status.adv_param.flash = FLASH_NONE;	/*flash status reset */
			cxt->send_once[0] = cxt->send_once[1] = cxt->send_once[2] = cxt->send_once[3] = cxt->send_once[4] = 0;
		}

		if (1 < cxt->capture_skip_num)
			main_flash_counts = cxt->capture_skip_num - cxt->send_once[2];
		else
			main_flash_counts = -1;

		main_flash_capture_counts = cxt->flash_timing_param.main_open_delay + cxt->flash_timing_param.main_param_update_delay;

		ISP_LOGV("main_flash_counts: %d, capture_skip_num: %d", main_flash_counts, cxt->capture_skip_num);
		ISP_LOGV("main_param_update_delay: %d, main_open_delay: %d, main_flash_capture_counts: %d",
			cxt->flash_timing_param.main_param_update_delay,
			cxt->flash_timing_param.main_open_delay,
			main_flash_capture_counts);

		/*flash's self protect will close main flash advance when shutter is 1/5s*/
		if ((cxt->cur_status.adv_param.mode_param.mode == AE_MODE_AUTO_SHUTTER_PRI) && (200000000 == cxt->sync_cur_result.ev_setting.exp_time)) {
			main_flash_capture_counts = main_flash_capture_counts - 2;
		}

		if ((FLASH_MAIN_BEFORE_RECEIVE == cxt->cur_result.flash_status && FLASH_MAIN_BEFORE == current_status->adv_param.flash)
			|| (FLASH_MAIN_RECEIVE == cxt->cur_result.flash_status && FLASH_MAIN == current_status->adv_param.flash)
			|| (FLASH_MAIN_CLOSE == current_status->adv_param.flash)) {
			ISP_LOGD("ae_flash1_status shake_4 %d, %d, cnt: %d, flash_status:%d", cxt->send_once[2], cxt->send_once[4], main_flash_counts, current_status->adv_param.flash);

			if (cxt->flash_timing_param.main_param_update_delay == cxt->send_once[4]) {
				ISP_LOGD("ae_flash m: led level: %d, %d\n", cxt->flash_esti_result.captureFlahLevel1, cxt->flash_esti_result.captureFlahLevel2);
				rtn = ae_set_flash_charge(cxt, AE_FLASH_TYPE_MAIN);
				cb_type = AE_CB_CONVERGED;
				(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, cb_type, NULL);
				ISP_LOGD("ae_flash1_callback do-main-flash!\r\n");
			} else if (((main_flash_capture_counts == cxt->send_once[4]) && (cxt->mainFlashEn != 0)) || ((2 == cxt->send_once[4]) && (cxt->mainFlashEn == 0)))  {
				cb_type = AE_CB_CONVERGED;
				(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, cb_type, NULL);
				cxt->cur_result.flash_status = FLASH_NONE;	/*flash status reset */
				ISP_LOGD("ae_flash1_callback do-capture!\r\n");
			} else if (main_flash_capture_counts + 2 == cxt->send_once[4]) {
				#if 0
				sensor_param_updating_interface(cxt);
				ISP_LOGD("ae_flash1_callback update next prev-param!\r\n");

				struct ae_flash_cfg cfg;
				cfg.led_idx = 0;
				cfg.type = AE_FLASH_TYPE_MAIN;
				cfg.led0_enable = 0;
				cfg.led1_enable = 0;
				cxt->isp_ops.flash_ctrl(cxt->isp_ops.isp_handler, &cfg, NULL);
				#endif
			} else {
				if (1 > cxt->send_once[4]) {
					ISP_LOGD("ae_flash1 wait-main-flash!\r\n");
				} else if (cxt->flash_timing_param.main_open_delay < cxt->send_once[4]) {
					ISP_LOGD("ae_flash1 wait-get-image!\r\n");
				} else {
					ISP_LOGD("ae_flash1 wait-capture!\r\n");
				}
			}

			if (FLASH_MAIN_CLOSE == current_status->adv_param.flash) {
				sensor_param_updating_interface(cxt);
				ISP_LOGD("ae_flash1_callback update next prev-param!\r\n");

				cb_type = AE_CB_RECOVER_GAIN;
				(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, cb_type, NULL);
				ISP_LOGD("ae_flash1_callback revover_gain!\r\n");
			}

			if (0 >= main_flash_counts) {
			cxt->send_once[4]++;
			}
			cxt->send_once[2]++;

			/*write flash awb gain*/
			if ((1 == cxt->flash_esti_result.isEnd ) && (cxt->mainFlashEn == 0)) {
				if (cxt->isp_ops.set_wbc_gain) {
					struct ae_alg_rgb_gain awb_m_b_flash_gain;
					awb_m_b_flash_gain.r = cxt->flash_esti_result.captureRGain;
					awb_m_b_flash_gain.g = cxt->flash_esti_result.captureGGain;
					awb_m_b_flash_gain.b = cxt->flash_esti_result.captureBGain;
					cxt->isp_ops.set_wbc_gain(cxt->isp_ops.isp_handler, &awb_m_b_flash_gain);
					ISP_LOGV("flash_cap awb_m r %d, g %d, b %d", awb_m_b_flash_gain.r, awb_m_b_flash_gain.g, awb_m_b_flash_gain.b);
				}
			}

			if ((1 == cxt->flash_main_esti_result.isEnd) && (cxt->send_once[4] <= main_flash_capture_counts) && (cxt->mainFlashEn == 1)) {
				if (cxt->isp_ops.set_wbc_gain) {
					struct ae_alg_rgb_gain awb_gain;
					awb_gain.r = cxt->flash_main_esti_result.captureRGain;
					awb_gain.g = cxt->flash_main_esti_result.captureGGain;
					awb_gain.b = cxt->flash_main_esti_result.captureBGain;
					cxt->isp_ops.set_wbc_gain(cxt->isp_ops.isp_handler, &awb_gain);
					ISP_LOGV("flash_cap awb r %d, g %d, b %d", awb_gain.r, awb_gain.g, awb_gain.b);
				}
			}
		}

		if (FLASH_MAIN_AFTER_RECEIVE == cxt->cur_result.flash_status && FLASH_MAIN_AFTER == current_status->adv_param.flash) {
			ISP_LOGD("ae_flash1_status shake_6");
			cxt->cur_status.adv_param.flash = FLASH_NONE;	/*flash status reset */
			cxt->send_once[0] = cxt->send_once[1] = cxt->send_once[2] = cxt->send_once[3] = cxt->send_once[4] = 0;
			if (0 != cxt->flash_ver) {
				flash_finish(cxt);
			}
		}
	}

	/* for front flash algorithm (LED+LCD) */
	if (cxt->camera_id == 1 && cxt->cur_status.adv_param.flash == FLASH_LED_AUTO) {
		if ((cxt->sync_cur_result.cur_bv <= cxt->flash_thrd.thd_down) && cxt->led_state == 0) {
			led_eb = 1;
			cxt->led_state = 1;
			cb_type = AE_CB_LED_NOTIFY;
			(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, cb_type, &led_eb);
			ISP_LOGV("ae_flash1_callback do-led-open!\r\n");
			ISP_LOGV("camera_id %d, flash_status %d, cur_bv %d, led_open_thr %d, led_state %d",
					 cxt->camera_id, cxt->cur_status.adv_param.flash, cxt->sync_cur_result.cur_bv, cxt->flash_thrd.thd_down, cxt->led_state);
		}

		if ((cxt->sync_cur_result.cur_bv >= cxt->flash_thrd.thd_up) && cxt->led_state == 1) {
			led_eb = 0;
			cxt->led_state = 0;
			cb_type = AE_CB_LED_NOTIFY;
			(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, cb_type, &led_eb);
			ISP_LOGV("ae_flash1_callback do-led-close!\r\n");
			ISP_LOGV("camera_id %d, flash_status %d, cur_bv %d, led_close_thr %d, led_state %d",
					 cxt->camera_id, cxt->cur_status.adv_param.flash, cxt->sync_cur_result.cur_bv, cxt->flash_thrd.thd_up, cxt->led_state);
		}
	}

	/* notify APP if need autofocus or not, just in flash auto mode */
	if (cxt->camera_id == 0 || cxt->camera_id == 2 || cxt->camera_id == 3){
		ISP_LOGV("flash open thr=%d, flash close thr=%d, bv=%d, flash_fired=%d, delay_cnt=%d",
				 cxt->flash_thrd.thd_down, cxt->flash_thrd.thd_up, cxt->sync_cur_result.cur_bv, cxt->flash_fired, cxt->delay_cnt);

		if (cxt->sync_cur_result.cur_bv < cxt->flash_thrd.thd_down) {
			cxt->delay_cnt = 0;
			flash_fired = 1;
			cxt->flash_fired = 1;
			cb_type = AE_CB_FLASH_FIRED;
			(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, cb_type, &flash_fired);
			ISP_LOGV("flash will fire!\r\n");
		}

		if ((cxt->sync_cur_result.cur_bv > cxt->flash_thrd.thd_up) && (cxt->flash_fired == 1)) {
			if (cxt->delay_cnt == cxt->flash_timing_param.main_capture_delay) {
				flash_fired = 0;
				cxt->flash_fired = 0;
				cb_type = AE_CB_FLASH_FIRED;
				(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, cb_type, &flash_fired);
				ISP_LOGV("flash will not fire!\r\n");
			}
			cxt->delay_cnt++;
		}
	}

	if (AE_3DNR_AUTO == cxt->threednr_mode) {
		cmr_u32 is_update = 0, is_en = 0;
		if (cxt->sync_cur_result.cur_bv < cxt->threednr_thrd.thd_down) {
			is_update = 1;
			is_en = 1;
			cxt->threednr_mode_flag = 1;
		}else if (cxt->sync_cur_result.cur_bv > cxt->threednr_thrd.thd_up) {
			is_update = 1;
			is_en = 0;
			cxt->threednr_mode_flag = 0;
		}
		if (is_update) {
			cb_type = AE_CB_3DNR_NOTIFY;
			(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, cb_type, &is_en);
			ISP_LOGV("auto-3dnr: bv: %d,[%d, %d], 3dnr: %d",
				cxt->sync_cur_result.cur_bv,cxt->threednr_thrd.thd_down,cxt->threednr_thrd.thd_up, is_en);
		}
	}

	return AE_SUCCESS;
}

static cmr_s32 ae_get_debug_info(struct ae_ctrl_cxt *cxt, cmr_handle result)
{
	cmr_s32 rtn = AE_SUCCESS;
	struct ae_debug_info_packet_in debug_info_in;
	struct ae_debug_info_packet_out debug_info_out;

	char *alg_id_ptr = NULL;
	cmr_u32 buf_size_max = 0, debug_info_len = 0;;
	struct tg_ae_ctrl_alc_log *debug_info_result = (struct tg_ae_ctrl_alc_log *)result;

	if (result) {
		memset((cmr_handle) & debug_info_in, 0, sizeof(struct ae_debug_info_packet_in));
		memset((cmr_handle) & debug_info_out, 0, sizeof(struct ae_debug_info_packet_out));

		alg_id_ptr = ae_debug_get_lib_version();
		//debug_info_in.alg_id = cxt->alg_id;
		//debug_info_in.aem_stats = (cmr_handle) cxt->sync_aem;
		//debug_info_in.base_aem_stats = (cmr_handle) cxt->cur_status.adv_param.stats_data_high.stat_data;
		//debug_info_in.alg_status = (cmr_handle) & cxt->sync_cur_status;
		//debug_info_in.alg_results = (cmr_handle) & cxt->sync_cur_result;
		//debug_info_in.history_param = (cmr_handle) & cxt->history_param[0];
		//debug_info_in.packet_buf = (cmr_handle) & cxt->debug_info_buf[0];
		memcpy((cmr_handle) & debug_info_in.alg_version[0], alg_id_ptr, sizeof(debug_info_in.alg_version));
		memcpy((cmr_handle) & debug_info_in.flash_version[0], &cxt->flash_ver, sizeof(debug_info_in.flash_version));

		debug_info_in.data.alg_handle = cxt->misc_handle;
		debug_info_in.major_id = cxt->major_id;
		debug_info_in.minor_id = cxt->minor_id;
		debug_info_in.packet_buf = &cxt->debug_info_buf[0];
		buf_size_max = sizeof(cxt->debug_info_buf) / sizeof(cxt->debug_info_buf[0]);
		debug_info_in.packet_size_max = buf_size_max;
		rtn = ae_debug_packet((cmr_handle) &debug_info_in, (cmr_handle) &debug_info_out);
		/*add flash debug information */
		if (buf_size_max < (debug_info_out.size + cxt->flash_buf_len))
			debug_info_len = buf_size_max - debug_info_out.size;
                else
			debug_info_len = cxt->flash_buf_len;
                if(debug_info_len < sizeof(cxt->flash_debug_buf))
		        memcpy((cmr_handle) &cxt->debug_info_buf[debug_info_out.size], (cmr_handle) &cxt->flash_debug_buf[0], debug_info_len);
                else
                        memcpy((cmr_handle) &cxt->debug_info_buf[debug_info_out.size], (cmr_handle) &cxt->flash_debug_buf[0], sizeof(cxt->flash_debug_buf));
		debug_info_result->log = (cmr_u8 *)debug_info_in.packet_buf;
		debug_info_result->size = debug_info_out.size + cxt->flash_buf_len;

		ISP_LOGD("bug info buf max: %d, ae: %d, flash:%d\n", buf_size_max, debug_info_out.size, cxt->flash_buf_len);
	} else {
		ISP_LOGE("result pointer is NULL\n");
		rtn = AE_ERROR;
	}

	return rtn;
}

static cmr_s32 ae_get_debug_info_for_display(struct ae_ctrl_cxt *cxt, cmr_handle result)
{
	cmr_s32 rtn = AE_SUCCESS;
	UNUSED(cxt);
	UNUSED(result);
	return rtn;
}

static cmr_s32 ae_make_calc_result(struct ae_ctrl_cxt *cxt, struct ae_lib_calc_out *alg_rt, struct ae_calc_results_3_x *result)
{
	cmr_s32 rtn = AE_SUCCESS;
	cmr_u32 i = 0x00;
	//struct  ae_ev_param_table ev_table;

	result->ae_output.cur_lum = alg_rt->cur_lum;
	result->ae_output.cur_again = alg_rt->ev_setting.ae_gain;
	result->ae_output.cur_exp_line = alg_rt->ev_setting.exp_line;
	result->ae_output.line_time = cxt->cur_status.adv_param.cur_ev_setting.line_time;
	result->ae_output.is_stab = alg_rt->stable;
	result->ae_output.target_lum = alg_rt->target_lum;
	result->ae_output.face_stable = alg_rt->face_stable;
	result->ae_output.cur_bv = alg_rt->cur_bv;
	result->ae_output.abl_weight = alg_rt->abl_confidence;
	result->ae_output.exposure_time = cxt->cur_result.ev_setting.exp_time / AEC_LINETIME_PRECESION;
	result->ae_output.fps = alg_rt->cur_fps;
	result->ae_output.face_enable = alg_rt->face_enable;
	result->ae_output.reserved = alg_rt->privated_data;

	result->is_skip_cur_frame = 0;
	result->monitor_info.trim = cxt->monitor_cfg.trim;
	result->monitor_info.win_num = cxt->monitor_cfg.blk_num;
	result->monitor_info.win_size = cxt->monitor_cfg.blk_size;
	result->ae_ev.ev_index = cxt->cur_status.adv_param.comp_param.value.ev_index;
	result->flash_param.captureFlashEnvRatio = cxt->flash_esti_result.captureFlashRatio;
	result->flash_param.captureFlash1ofALLRatio = cxt->flash_esti_result.captureFlash1Ratio;

	for (i = 0; i < cxt->ev_param_table.diff_num; i++) {
		result->ae_ev.ev_tab[i] = cxt->cur_param_target_lum + cxt->ev_param_table.items[i].lum_diff;
	}

	ae_get_iso(cxt, &result->ae_output.cur_iso);

	return rtn;
}

static cmr_s32 ae_make_isp_result(struct ae_ctrl_cxt *cxt, struct ae_lib_calc_out *alg_rt, struct ae_ctrl_callback_in *result)
{
	cmr_s32 rtn = AE_SUCCESS;
	cmr_u32 i = 0x00;
	//struct  ae_ev_param_table ev_table;
	result->ae_output.cur_lum = alg_rt->cur_lum;
	result->ae_output.cur_again = alg_rt->ev_setting.ae_gain;
	result->ae_output.cur_exp_line = alg_rt->ev_setting.exp_line;
	result->ae_output.line_time = cxt->cur_status.adv_param.cur_ev_setting.line_time;
	result->ae_output.is_stab = alg_rt->stable;
	result->ae_output.target_lum = alg_rt->target_lum;
	//result->ae_output.target_lum_ori = alg_rt->target_lum_ori;
	//result->ae_output.flag4idx = alg_rt->flag4idx;
	result->ae_output.face_stable = alg_rt->face_stable;
	result->ae_output.cur_bv = alg_rt->cur_bv;
	result->ae_output.abl_weight = alg_rt->abl_confidence;
	result->ae_output.exposure_time = cxt->cur_result.ev_setting.exp_time / AEC_LINETIME_PRECESION;
	result->ae_output.fps = alg_rt->cur_fps;
	result->ae_output.face_enable = alg_rt->face_enable;
	result->ae_output.reserved = alg_rt->privated_data;

	result->is_skip_cur_frame = 0;
	result->monitor_info.trim = cxt->monitor_cfg.trim;
	result->monitor_info.win_num = cxt->cur_status.stats_data_basic.size;
	result->monitor_info.win_size = cxt->cur_status.stats_data_basic.blk_size;
	result->ae_ev.ev_index = cxt->cur_status.adv_param.comp_param.value.ev_index;
	result->flash_param.captureFlashEnvRatio = cxt->flash_esti_result.captureFlashRatio;
	result->flash_param.captureFlash1ofALLRatio = cxt->flash_esti_result.captureFlash1Ratio;

	for (i = 0; i < cxt->ev_param_table.diff_num; i++) {
		result->ae_ev.ev_tab[i] = cxt->cur_param_target_lum + cxt->ev_param_table.items[i].lum_diff;
	}

	ae_get_iso(cxt, &result->ae_output.cur_iso);

	return rtn;
}

static cmr_s32 ae_get_flicker_switch_flag(struct ae_ctrl_cxt *cxt, cmr_handle in_param)
{
	cmr_s32 rtn = AE_SUCCESS;
	cmr_u32 cur_exp = 0;
	cmr_u32 *flag = (cmr_u32 *) in_param;

	if (in_param) {
		cur_exp = cxt->sync_cur_result.ev_setting.exp_time;
		// 50Hz/60Hz
		if (AE_FLICKER_50HZ == cxt->cur_status.adv_param.flicker) {
			if (cur_exp < 100000) {
				*flag = 0;
			} else {
				*flag = 1;
			}
		} else {
			if (cur_exp < 83333) {
				*flag = 0;
			} else {
				*flag = 1;
			}
		}

		ISP_LOGV("ANTI_FLAG: %d, %d, %d", cur_exp, cxt->snr_info.line_time, *flag);
	}
	return rtn;
}

static void ae_set_led(struct ae_ctrl_cxt *cxt)
{
	char str[PROPERTY_VALUE_MAX];
	cmr_s16 i = 0;
	cmr_s16 j = 0;
	float tmp = 0;
	cmr_u32 type = AE_FLASH_TYPE_MAIN;	//ISP_FLASH_TYPE_MAIN   ISP_FLASH_TYPE_PREFLASH
	cmr_s8 led_ctl[2] = { 0, 0 };
	struct ae_flash_cfg cfg;
	struct ae_flash_element element;

	memset(str, 0, sizeof(str));
	property_get("persist.vendor.cam.isp.ae.manual", str, "");
	if ((strcmp(str, "fasim") & strcmp(str, "facali") & strcmp(str, "facali-pre") & strcmp(str, "led"))) {
		//ISP_LOGV("isp_set_led_noctl!\r\n");
	} else {
		if (!strcmp(str, "facali"))
			type = AE_FLASH_TYPE_MAIN;
		else if (!strcmp(str, " facali-pre"))
			type = AE_FLASH_TYPE_PREFLASH;
		else
			type = AE_FLASH_TYPE_PREFLASH;

		memset(str, 0, sizeof(str));
		property_get("persist.vendor.cam.isp.ae.led", str, "");
		if ('\0' == str[i]) {
			return;
		} else {
			while (' ' == str[i])
				i++;

			while (('0' <= str[i] && '9' >= str[i]) || ' ' == str[i]) {
				if (' ' == str[i]) {
					if (' ' == str[i + 1]) {
						;
					} else {
						if (j > 0)
							j = 1;
						else
							j++;
					}
				} else {
					led_ctl[j] = 10 * led_ctl[j] + str[i] - '0';
				}
				i++;
			}
		}

		ISP_LOGV("isp_set_led: %d %d\r\n", led_ctl[0], led_ctl[1]);
		if (0 == led_ctl[0] || 0 == led_ctl[1]) {
//close
			cfg.led_idx = 0;
			//cfg.type = ISP_FLASH_TYPE_MAIN;
			cfg.type = type;
			cxt->isp_ops.flash_ctrl(cxt->isp_ops.isp_handler, &cfg, NULL);

			cxt->led_record[0] = led_ctl[0];
			cxt->led_record[1] = led_ctl[1];
		} else if (cxt->led_record[0] != led_ctl[0] || cxt->led_record[1] != led_ctl[1]) {
//set led_1
			cfg.led_idx = 1;
			//cfg.type = ISP_FLASH_TYPE_MAIN;
			cfg.type = type;
			cfg.multiColorLcdEn = cxt->multiColorLcdEn;
			element.index = led_ctl[0] - 1;
			cxt->isp_ops.flash_set_charge(cxt->isp_ops.isp_handler, &cfg, &element);
//set led_2
			cfg.led_idx = 2;
			//cfg.type = ISP_FLASH_TYPE_MAIN;
			cfg.type = type;
			cfg.multiColorLcdEn = cxt->multiColorLcdEn;
			element.index = led_ctl[1] - 1;
			cxt->isp_ops.flash_set_charge(cxt->isp_ops.isp_handler, &cfg, &element);
//open
			cfg.led_idx = 1;
			//cfg.type = ISP_FLASH_TYPE_MAIN;
			cfg.type = type;
			cxt->isp_ops.flash_ctrl(cxt->isp_ops.isp_handler, &cfg, NULL);

			cxt->led_record[0] = led_ctl[0];
			cxt->led_record[1] = led_ctl[1];
		} else {
			cxt->flash_cali[led_ctl[0]][led_ctl[1]].used = 1;
			cxt->flash_cali[led_ctl[0]][led_ctl[1]].ydata = cxt->sync_cur_result.cur_lum * 4;
			tmp = 1024.0 / cxt->cur_status.awb_gain.g;
			cxt->flash_cali[led_ctl[0]][led_ctl[1]].rdata = (cmr_u16) (cxt->cur_status.awb_gain.r * tmp);
			cxt->flash_cali[led_ctl[0]][led_ctl[1]].bdata = (cmr_u16) (cxt->cur_status.awb_gain.b * tmp);
		}

		memset(str, 0, sizeof(str));
		property_get("persist.vendor.cam.isp.ae.facali.dump", str, "");
		if (!strcmp(str, "on")) {
			FILE *p = NULL;
			p = fopen("/data/vendor/cameraserver/flashcali.txt", "w+");
			if (!p) {
				ISP_LOGW("Write flash cali file error!!\r\n");
				goto ae_set_led_exit;
			} else {
				fprintf(p, "shutter: %d  gain: %d\r\n", cxt->sync_cur_result.ev_setting.exp_line, cxt->sync_cur_result.ev_setting.ae_gain);

				fprintf(p, "Used\r\n");
				for (i = 0; i < 32; i++) {
					for (j = 0; j < 32; j++) {
						fprintf(p, "%1d ", cxt->flash_cali[i][j].used);
					}
					fprintf(p, "\r\n");
				}

				fprintf(p, "Ydata\r\n");
				for (i = 0; i < 32; i++) {
					for (j = 0; j < 32; j++) {
						fprintf(p, "%4d ", cxt->flash_cali[i][j].ydata);
					}
					fprintf(p, "\r\n");
				}

				fprintf(p, "R_gain\r\n");
				for (i = 0; i < 32; i++) {
					for (j = 0; j < 32; j++) {
						fprintf(p, "%4d ", cxt->flash_cali[i][j].rdata);
					}
					fprintf(p, "\r\n");
				}

				fprintf(p, "B_gain\r\n");
				for (i = 0; i < 32; i++) {
					for (j = 0; j < 32; j++) {
						fprintf(p, "%4d ", cxt->flash_cali[i][j].bdata);
					}
					fprintf(p, "\r\n");
				}
			}
			fclose(p);
		} else if (!strcmp(str, "clear")) {
			memset(cxt->flash_cali, 0, sizeof(cxt->flash_cali));
		} else {
			;
		}
	}
  ae_set_led_exit:
	return;
}

static void ae_hdr_calculation(struct ae_ctrl_cxt *cxt, cmr_u32 in_max_frame_line, cmr_u32 in_min_frame_line, cmr_u32 in_exposure, cmr_u32 base_exposure, cmr_u32 in_gain, cmr_u32 * out_gain, cmr_u32 * out_exp_l)
{
	cmr_u32 exp_line = 0;
	cmr_u32 exp_time = 0;
	cmr_u32 gain = in_gain;
	cmr_u32 exposure = in_exposure;
	cmr_u32 max_frame_line = in_max_frame_line;
	cmr_u32 min_frame_line = in_min_frame_line;
	cmr_u32 fliker_unit_time = 10000000;
	UNUSED(base_exposure);

	if (exposure > (max_frame_line * cxt->cur_status.adv_param.cur_ev_setting.line_time))
		exposure = max_frame_line * cxt->cur_status.adv_param.cur_ev_setting.line_time;
	else if (exposure < (min_frame_line * cxt->cur_status.adv_param.cur_ev_setting.line_time))
		exposure = min_frame_line * cxt->cur_status.adv_param.cur_ev_setting.line_time;

	if (cxt->cur_flicker == 0)
		fliker_unit_time = 10000000;
	else if(cxt->cur_flicker == 1)
		fliker_unit_time = 8333333;

	if(exposure > fliker_unit_time){
		exp_time = (cmr_u32) (1.0 * exposure / fliker_unit_time + 0.5) * fliker_unit_time;
		exp_line = (cmr_u32) (1.0 * exp_time / cxt->cur_status.adv_param.cur_ev_setting.line_time + 0.5);
		gain = (cmr_u32) (1.0 * exposure * gain / exp_time + 0.5);
		if(gain < 128){
			exp_time = (cmr_u32) (1.0 * exposure / fliker_unit_time ) * fliker_unit_time;
			exp_line = (cmr_u32) (1.0 * exp_time / cxt->cur_status.adv_param.cur_ev_setting.line_time + 0.5);
			gain = (cmr_u32) (1.0 * exposure * gain / exp_time + 0.5);
		}
	}else{
		exp_time = fliker_unit_time;
		exp_line = (cmr_u32) (1.0 * exp_time / cxt->cur_status.adv_param.cur_ev_setting.line_time + 0.5);
		gain = (cmr_u32) (1.0 * exposure * gain / exp_time + 0.5);
		if(gain < 128){
			gain = in_gain;
			exp_line = (cmr_u32) (1.0 * in_exposure / cxt->cur_status.adv_param.cur_ev_setting.line_time + 0.5);
			ISP_LOGD("HDR calc fail for Gain<128 ");
		}
	}
	*out_exp_l = exp_line;
	*out_gain = gain;

	ISP_LOGD("exposure %d, max_frame_line %d, min_frame_line %d, exp_time %d, exp_line %d, gain %d", in_exposure, in_max_frame_line, in_min_frame_line, exp_time, exp_line, gain);
}

static void ae_set_hdr_ctrl(struct ae_ctrl_cxt *cxt, struct ae_calc_in *param)
{
	UNUSED(param);
	cmr_u32 base_exposure_line = 0;
	cmr_u32 up_exposure = 0;
	cmr_u32 down_exposure = 0;
	cmr_u16 base_gain = 0;
	cmr_u32 max_frame_line = 0;
	cmr_u32 min_frame_line = 0;
	cmr_u32 exp_line = 0;
	cmr_u32 gain = 0;
	cmr_s32 counts = 0;
	cmr_u32 skip_num = 0;
	cmr_s8 hdr_callback_flag = 0;
#ifdef CONFIG_SUPPROT_AUTO_HDR
	float ev_result[2] = {0, 0};
	ev_result[0] = -cxt->hdr_calc_result.ev[0];
	ev_result[1] = cxt->hdr_calc_result.ev[1];
	ISP_LOGD("auto_hdr (cap) ev[0] %f ev[1] %f", ev_result[0], ev_result[1]);
#endif

	cxt->hdr_frame_cnt++;
	hdr_callback_flag = MAX((cmr_s8) cxt->hdr_cb_cnt, (cmr_s8) cxt->capture_skip_num);
	if (cxt->hdr_frame_cnt == hdr_callback_flag) {
#ifdef CONFIG_SUPPROT_AUTO_HDR
		(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, AE_CB_HDR_START, (void *)ev_result);
		if (cxt->is_multi_mode && cxt->isp_ops.ae_bokeh_hdr_cb) {
			(*cxt->isp_ops.ae_bokeh_hdr_cb) (cxt->isp_ops.isp_handler, (void *)ev_result);
		}
#else
		(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, AE_CB_HDR_START, NULL);
#endif
		ISP_LOGD("_isp_hdr_callback do-capture!\r\n");
	}

	skip_num = cxt->capture_skip_num;
	ISP_LOGV("skip_num %d", skip_num);
	counts = cxt->capture_skip_num - cxt->hdr_cb_cnt - cxt->hdr_frame_cnt;
	if (0 > counts) {
		max_frame_line = (cmr_u32) (1.0 * 1000000000 / cxt->fps_range.min / cxt->cur_status.adv_param.cur_ev_setting.line_time);
		min_frame_line = (cmr_u32) (1.0 * cxt->ae_tbl_param.min_exp / cxt->cur_status.adv_param.cur_ev_setting.line_time + 0.5);
		ISP_LOGV("counts %d\n", counts);
		ISP_LOGV("max_frame_line %d, min_frame_line %d, fps_min %d, cur_line_time %d\n", max_frame_line, min_frame_line, cxt->fps_range.min, cxt->cur_status.adv_param.cur_ev_setting.line_time);
		if (3 == cxt->hdr_flag) {
			base_exposure_line = cxt->hdr_exp_line;
			base_gain = cxt->hdr_gain;
			down_exposure = 1.0 / pow(2, cxt->hdr_calc_result.ev[0]) * base_exposure_line * cxt->cur_status.adv_param.cur_ev_setting.line_time;
			ISP_LOGD("down_exp %d, pow2 %f\n", down_exposure, pow(2, cxt->hdr_calc_result.ev[0]));
			ae_hdr_calculation(cxt, max_frame_line, min_frame_line, down_exposure, base_exposure_line, base_gain, &gain, &exp_line);
			ISP_LOGV("base_exposure: %d, base_gain: %d, down_exposure: %d, exp_line: %d", base_exposure_line, base_gain, down_exposure, exp_line);
			cxt->cur_status.adv_param.mode_param.value.exp_gain[0] = exp_line * cxt->cur_status.adv_param.cur_ev_setting.line_time;
			cxt->cur_status.adv_param.mode_param.value.exp_gain[1] = gain;
			cxt->cur_status.adv_param.mode_param.mode = AE_MODE_MANUAL_EXP_GAIN;
			cxt->cur_status.adv_param.prof_mode = 1;
			cxt->hdr_flag--;
			ISP_LOGD("_isp_hdr_3: exp_line %d, gain %d\n", exp_line, gain);
		} else if (2 == cxt->hdr_flag) {
			base_exposure_line = cxt->hdr_exp_line;
			base_gain = cxt->hdr_gain;
			up_exposure = pow(2, cxt->hdr_calc_result.ev[1]) * base_exposure_line * cxt->cur_status.adv_param.cur_ev_setting.line_time;
			ISP_LOGD("up_exp %d, pow2 %f\n", up_exposure, pow(2, cxt->hdr_calc_result.ev[1]));
			ae_hdr_calculation(cxt, max_frame_line, min_frame_line, up_exposure, base_exposure_line, base_gain, &gain, &exp_line);
			if(exp_line * gain < base_exposure_line * base_gain){
				exp_line = base_exposure_line;
				gain = base_gain;
			}
			ISP_LOGV("base_exposure: %d, base_gain: %d, up_exposure: %d, exp_line: %d", base_exposure_line, base_gain, up_exposure, exp_line);
			cxt->cur_status.adv_param.mode_param.value.exp_gain[0] = exp_line * cxt->cur_status.adv_param.cur_ev_setting.line_time;
			cxt->cur_status.adv_param.mode_param.value.exp_gain[1] = gain;
			cxt->cur_status.adv_param.mode_param.mode = AE_MODE_MANUAL_EXP_GAIN;
			cxt->cur_status.adv_param.prof_mode = 1;
			cxt->hdr_flag--;
			ISP_LOGD("_isp_hdr_2: exp_line %d, gain %d\n", exp_line, gain);
		} else {
			base_exposure_line = cxt->hdr_exp_line;
			base_gain = cxt->hdr_gain;
			cxt->cur_status.adv_param.mode_param.value.exp_gain[0] = base_exposure_line * cxt->cur_status.adv_param.cur_ev_setting.line_time;
			cxt->cur_status.adv_param.mode_param.value.exp_gain[1] = base_gain;
			cxt->cur_status.adv_param.mode_param.mode = AE_MODE_MANUAL_EXP_GAIN;
			cxt->cur_status.adv_param.prof_mode = 1;
			cxt->hdr_flag--;
			ISP_LOGD("_isp_hdr_1: exp_line %d, gain %d\n", base_exposure_line, base_gain);
		}
	}
}

static void ae_set_ev_adjust_ctrl(struct ae_ctrl_cxt *cxt, struct ae_calc_in *param)
{
	UNUSED(param);
	cmr_u32 base_exposure_line = 0;
	cmr_u32 down_exposure = 0;
	cmr_u16 base_gain = 0;
	cmr_u32 max_frame_line = 0;
	cmr_u32 min_frame_line = 0;
	cmr_u32 exp_line = 0;
	cmr_u32 gain = 0;
	float down_EV_offset =-(cxt->sync_cur_result.evd_value / 100.0);
	cmr_s8 callback_flag = 0;
	cmr_s8 callback_frame = 0;

	cxt->ev_adj_frame_cnt++;
	callback_frame = MAX((cmr_s8)cxt->ev_adj_ev_effect_valid_num,(cmr_s8)cxt->capture_skip_num);
	if (callback_frame == cxt->ev_adj_frame_cnt) {
		if(0 == cxt->sync_cur_result.evd_value)
			callback_flag = 0;
		else
			callback_flag = 1;
		(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, AE_CB_EV_ADJUST_NOTIFY, &callback_flag);
		ISP_LOGD("isp_callback do-capture %d",callback_flag);
	}

	max_frame_line = (cmr_u32) (1.0 * 1000000000 / cxt->fps_range.min / cxt->cur_status.adv_param.cur_ev_setting.line_time);
	min_frame_line = (cmr_u32) (1.0 * cxt->ae_tbl_param.min_exp / cxt->cur_status.adv_param.cur_ev_setting.line_time + 0.5);

	ISP_LOGV("max_frame_line %d, min_frame_line %d, fps_min %d, cur_line_time %d\n", max_frame_line, min_frame_line, cxt->fps_range.min, cxt->cur_status.adv_param.cur_ev_setting.line_time);
	if ( cxt->ev_adj_flag <= cxt->ev_adjust_cnt ){
		base_exposure_line = cxt->ev_adj_exp_line;
		base_gain = cxt->ev_adj_gain;
		down_exposure =1.0 / pow(2,down_EV_offset) * base_exposure_line * cxt->cur_status.adv_param.cur_ev_setting.line_time;
		ISP_LOGD("down_exp %d, pow2 %f\n", down_exposure,  down_EV_offset);
		ae_hdr_calculation(cxt, max_frame_line, min_frame_line, down_exposure, base_exposure_line, base_gain, &gain, &exp_line);
		ISP_LOGV("base_exposure: %d, base_gain: %d, down_exposure: %d, exp_line: %d", base_exposure_line, base_gain, down_exposure, exp_line);
		cxt->cur_status.adv_param.mode_param.value.exp_gain[0] = exp_line * cxt->cur_status.adv_param.cur_ev_setting.line_time;
		cxt->cur_status.adv_param.mode_param.value.exp_gain[1] = gain;
		cxt->cur_status.adv_param.mode_param.mode = AE_MODE_MANUAL_EXP_GAIN;
		cxt->cur_status.adv_param.prof_mode = 1;
		cxt->ev_adj_flag++;
		ISP_LOGD("isp_down_exp: exp_line %d, gain %d\n", exp_line, gain);
	}else {
		base_exposure_line = cxt->ev_adj_exp_line;
		base_gain = cxt->ev_adj_gain;
		cxt->cur_status.adv_param.mode_param.value.exp_gain[0] = base_exposure_line * cxt->cur_status.adv_param.cur_ev_setting.line_time;
		cxt->cur_status.adv_param.mode_param.value.exp_gain[1] = base_gain;
		cxt->cur_status.adv_param.mode_param.mode = AE_MODE_MANUAL_EXP_GAIN;
		cxt->cur_status.adv_param.prof_mode = 1;
		ISP_LOGD("isp_normal_exp: exp_line %d, gain %d\n", base_exposure_line, base_gain);
	}
}

static struct ae_exposure_param s_bakup_exp_param[8]={{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},};
static struct ae_exposure_param_switch_m s_ae_manual[8] ={{0,0,0,0,0,0,0,0,0,0},};

static void ae_save_exp_gain_param(struct ae_exposure_param *param, cmr_u32 num, struct ae_exposure_param_switch_m * ae_manual_param)
{
	cmr_u32 i = 0;
	FILE *pf = NULL;
	char version[1024];
	property_get("ro.build.version.release", version, "");
	if (atoi(version) > 6) {
		pf = fopen(AE_EXP_GAIN_PARAM_FILE_NAME_CAMERASERVER, "wb");
		if (pf) {
			for (i = 0; i < num; ++i) {
				ISP_LOGV("write:[%d]: %d, %d, %d, %d, %d\n", i, param[i].exp_line, param[i].exp_time, param[i].dummy, param[i].gain, param[i].bv);
			}

			fwrite((char *)param, 1, num * sizeof(struct ae_exposure_param), pf);
			fwrite((char *)ae_manual_param, 1, num * sizeof(struct ae_exposure_param_switch_m), pf);
			fclose(pf);
			pf = NULL;
		}
	} else {
		pf = fopen(AE_EXP_GAIN_PARAM_FILE_NAME_MEDIA, "wb");
		if (pf) {
			for (i = 0; i < num; ++i) {
				ISP_LOGV("write:[%d]: %d, %d, %d, %d, %d\n", i, param[i].exp_line, param[i].exp_time, param[i].dummy, param[i].gain, param[i].bv);
			}

			fwrite((char *)param, 1, num * sizeof(struct ae_exposure_param), pf);
			fwrite((char *)ae_manual_param, 1, num * sizeof(struct ae_exposure_param_switch_m), pf);
			fclose(pf);
			pf = NULL;
		}
	}

}

static void ae_read_exp_gain_param(struct ae_exposure_param *param, cmr_u32 num, struct ae_exposure_param_switch_m * ae_manual_param)
{
	cmr_u32 i = 0;
	FILE *pf = NULL;
	char version[1024];
	int sz = 0;
	property_get("ro.build.version.release", version, "");
	if (atoi(version) > 6) {
		pf = fopen(AE_EXP_GAIN_PARAM_FILE_NAME_CAMERASERVER, "rb");
		if (pf) {
			memset((void *)param, 0, sizeof(struct ae_exposure_param) * num);
			sz = fread((char *)param, 1, num * sizeof(struct ae_exposure_param), pf);
			if(sz != (num *sizeof(struct ae_exposure_param)))
			        ISP_LOGE("fread param faild");
			sz = fread((char *)ae_manual_param, 1, num * sizeof(struct ae_exposure_param_switch_m), pf);
                        if(sz != (num * sizeof(struct ae_exposure_param_switch_m)))
                                ISP_LOGE("fread ae_manual_param faild");
			fclose(pf);
			pf = NULL;

			for (i = 0; i < num; ++i) {
				ISP_LOGV("read[%d]: %d, %d, %d, %d, %d\n", i, param[i].exp_line, param[i].exp_time, param[i].dummy, param[i].gain, param[i].bv);
			}
		}
	} else {
		pf = fopen(AE_EXP_GAIN_PARAM_FILE_NAME_MEDIA, "rb");
		if (pf) {
			memset((void *)param, 0, sizeof(struct ae_exposure_param) * num);
			sz = fread((char *)param, 1, num * sizeof(struct ae_exposure_param), pf);
			if(sz != (num * sizeof(struct ae_exposure_param)))
                                ISP_LOGE("fread param faild");
			sz = fread((char *)ae_manual_param, 1, num * sizeof(struct ae_exposure_param_switch_m), pf);
			if(sz != (num * sizeof(struct ae_exposure_param_switch_m)))
                                ISP_LOGE("fread ae_manual_param faild");
			fclose(pf);
			pf = NULL;

			for (i = 0; i < num; ++i) {
				ISP_LOGV("read[%d]: %d, %d, %d, %d, %d\n", i, param[i].exp_line, param[i].exp_time, param[i].dummy, param[i].gain, param[i].bv);
			}
		}
	}

}

static void ae_set_video_stop(struct ae_ctrl_cxt *cxt)
{
	if (0 == cxt->is_snapshot) {
		if (0 == cxt->sync_cur_result.ev_setting.exp_line && 0 == cxt->sync_cur_result.ev_setting.ae_gain) {
			cxt->last_exp_param.exp_line = cxt->ae_tbl_param.def_expline;
			cxt->last_exp_param.exp_time = cxt->ae_tbl_param.def_expline * cxt->cur_status.adv_param.cur_ev_setting.line_time;
			cxt->last_exp_param.dummy = 0;
			cxt->last_exp_param.gain = cxt->ae_tbl_param.def_expline;
			cxt->last_exp_param.line_time = cxt->cur_status.adv_param.cur_ev_setting.line_time;
			cxt->last_exp_param.cur_index = cxt->ae_tbl_param.def_index;
			cxt->last_exp_param.target_luma = cxt->ae_tbl_param.target_lum;
			cxt->last_cur_lum = cxt->cur_result.cur_lum;
			cxt->last_index = cxt->ae_tbl_param.def_index;
			if (0 != cxt->cur_result.cur_bv)
				cxt->last_exp_param.bv = cxt->cur_result.cur_bv;
			else
				cxt->last_exp_param.bv = 1;
		} else {
			cxt->last_exp_param.exp_line = cxt->sync_cur_result.ev_setting.exp_line;
			cxt->last_exp_param.exp_time = cxt->sync_cur_result.ev_setting.exp_time;
			cxt->last_exp_param.dummy = cxt->sync_cur_result.ev_setting.dmy_line;
			cxt->last_exp_param.gain = cxt->sync_cur_result.ev_setting.ae_gain;
			cxt->last_exp_param.line_time = cxt->cur_status.adv_param.cur_ev_setting.line_time;
			cxt->last_exp_param.cur_index = cxt->sync_cur_result.ev_setting.ae_idx;
			cxt->last_exp_param.target_luma = cxt->sync_cur_result.target_lum;
			cxt->last_cur_lum = cxt->sync_cur_result.cur_lum;
			cxt->last_index = cxt->sync_cur_result.ev_setting.ae_idx;
			if (0 != cxt->cur_result.cur_bv)
				cxt->last_exp_param.bv = cxt->cur_result.cur_bv;
			else
				cxt->last_exp_param.bv = 1;
		}

		cxt->last_exp_param.target_offset = 0; // manual mode without target_offset
		cxt->last_exp_param.is_ev_setting = cxt->is_ev_setting;

		if ((cxt->is_multi_mode == ISP_ALG_DUAL_C_C) && (cxt->sensor_role != CAM_SENSOR_MASTER)) {
			struct ae_sync_lib_outout_data ae_sync_lib_output = {0};
			cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE0, GET_SYNC_SLAVE_SYNC_OUTPUT, NULL, &ae_sync_lib_output);
			cxt->last_exp_param.exp_line = ae_sync_lib_output.exp_line;
			cxt->last_exp_param.exp_time = ae_sync_lib_output.exp_time;
			cxt->last_exp_param.dummy = ae_sync_lib_output.dmy_line;
			cxt->last_exp_param.gain = ae_sync_lib_output.ae_gain;
			ISP_LOGD("GET_SYNC_SLAVE_SYNC_OUTPUT:exp_line:%d, dummy:%d, gain:%d", cxt->last_exp_param.exp_line, cxt->last_exp_param.dummy, cxt->last_exp_param.gain);
		}

		if(0 != cxt->sync_cur_result.touch_status){
			cxt->last_exp_param.exp_line = cxt->backup_touch.exp_line;
			cxt->last_exp_param.exp_time = cxt->backup_touch.exp_time;
			cxt->last_exp_param.dummy = cxt->backup_touch.dummy;
			cxt->last_exp_param.gain = cxt->backup_touch.gain;
			cxt->last_exp_param.line_time = cxt->backup_touch.line_time;
			ISP_LOGD("use param befor touch.exp line: %d gain: %d",cxt->last_exp_param.exp_line,cxt->last_exp_param.gain);
		}
		if(CAMERA_MODE_MANUAL != cxt->app_mode)
			s_bakup_exp_param[cxt->camera_id] = cxt->last_exp_param;

		if((cxt->app_mode < 64)){
			cxt->mode_switch[cxt->app_mode].exp_line = cxt->last_exp_param.exp_line;
			cxt->mode_switch[cxt->app_mode].dummy = cxt->last_exp_param.dummy;
			cxt->mode_switch[cxt->app_mode].gain = cxt->last_exp_param.gain;
			cxt->mode_switch[cxt->app_mode].exp_time = cxt->last_exp_param.exp_time;
			cxt->mode_switch[cxt->app_mode].target_offset = cxt->last_exp_param.target_offset;
			cxt->mode_switch[cxt->app_mode].table_idx = cxt->last_exp_param.cur_index;
			cxt->mode_switch[cxt->app_mode].lum = cxt->sync_cur_result.cur_lum;
			cxt->mode_switch[cxt->app_mode].tarlum = cxt->sync_cur_result.target_lum;
			cxt->app_mode_tarlum[cxt->app_mode] = cxt->sync_cur_result.target_lum;
			if(cxt->mode_switch[cxt->app_mode].lum){
				cxt->mode_switch[cxt->app_mode].sensitivity = (cxt->last_exp_param.exp_time) / 1000000 * (cxt->last_exp_param.gain) / (cxt->mode_switch[cxt->app_mode].lum);
				if(0 == cxt->mode_switch[cxt->app_mode].sensitivity)
					cxt->mode_switch[cxt->app_mode].sensitivity = 1;
				ISP_LOGV("sensitivity %d exp_time %d gain %d luma %d ",cxt->mode_switch[cxt->app_mode].sensitivity,cxt->last_exp_param.exp_time,cxt->last_exp_param.gain,cxt->mode_switch[cxt->app_mode].lum);
			}
		}

		if(CAMERA_MODE_MANUAL == cxt->app_mode){
			s_ae_manual[cxt->camera_id].exp_line = cxt->last_exp_param.exp_line;
			s_ae_manual[cxt->camera_id].dummy = cxt->last_exp_param.dummy;
			s_ae_manual[cxt->camera_id].gain = cxt->last_exp_param.gain;
			s_ae_manual[cxt->camera_id].exp_time = cxt->last_exp_param.exp_time;
			s_ae_manual[cxt->camera_id].target_offset = cxt->last_exp_param.target_offset;
			s_ae_manual[cxt->camera_id].table_idx = cxt->last_exp_param.cur_index;
			s_ae_manual[cxt->camera_id].manual_level = cxt->manual_level;
			s_ae_manual[cxt->camera_id].target_luma = cxt->last_exp_param.target_luma;
		}

		cxt->last_cam_mode = (cxt->app_mode | (cxt->camera_id << 16) | (1U << 31));

		if (cxt->is_multi_mode){
			cmr_u32 in = 0;
			if (CAM_SENSOR_MASTER == cxt->sensor_role) {
				cxt->ptr_isp_br_ioctrl(CAM_SENSOR_MASTER, SET_USER_COUNT, &in, NULL);
			} else if (CAM_SENSOR_SLAVE0 == cxt->sensor_role) {
				cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE0, SET_USER_COUNT, &in, NULL);
			} else if (CAM_SENSOR_SLAVE1 == cxt->sensor_role) {
				cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE1, SET_USER_COUNT, &in, NULL);
			}
		}

		ae_save_exp_gain_param(&s_bakup_exp_param[0], sizeof(s_bakup_exp_param) / sizeof(struct ae_exposure_param), &s_ae_manual[0]);
		ISP_LOGI("AE_VIDEO_STOP(in preview) cam-id %d BV %d BV_backup %d E %d G %d lt %d W %d H %d,enable: %d", cxt->camera_id, cxt->last_exp_param.bv, s_bakup_exp_param[cxt->camera_id].bv, cxt->last_exp_param.exp_line, cxt->last_exp_param.gain, cxt->cur_status.adv_param.cur_ev_setting.line_time, cxt->snr_info.frame_size.w, cxt->snr_info.frame_size.h, cxt->last_enable);
	} else {
		if ((1 == cxt->is_snapshot) && ((FLASH_NONE == cxt->cur_status.adv_param.flash) || FLASH_MAIN_BEFORE <= cxt->cur_status.adv_param.flash)) {
			ae_set_restore_cnt(cxt, 1);
		}
		ISP_LOGI("AE_VIDEO_STOP(in capture) cam-id %d BV %d BV_backup %d E %d G %d lt %d W %d H %d, enable: %d", cxt->camera_id, cxt->last_exp_param.bv, s_bakup_exp_param[cxt->camera_id].bv, cxt->last_exp_param.exp_line, cxt->last_exp_param.gain, cxt->cur_status.adv_param.cur_ev_setting.line_time, cxt->snr_info.frame_size.w, cxt->snr_info.frame_size.h, cxt->last_enable);
	}
	cxt->calcFirstFlag = 0;
}

static cmr_s32 ae_set_video_start(struct ae_ctrl_cxt *cxt, cmr_handle * param)
{
	cmr_s32 rtn = AE_SUCCESS;
	cmr_s32 ae_skip_num = 0;
	struct ae_trim trim;
	struct ae_rect bayer_hist_trim = { 0, 0, 0, 0 };
	cmr_u32 max_expl = 0;
	
	struct ae_exposure_param src_exp = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	struct ae_exposure_param dst_exp;
	struct ae_range fps_range;
	struct ae_set_work_param *work_info = (struct ae_set_work_param *)param;
	struct ae_scene_param_in scene_param_in;

	cmr_u32 k;
	cmr_u32 last_cam_mode = 0;
	cmr_u32 ae_target_lum = 0;

	if (NULL == param) {
		ISP_LOGE("param is NULL \n");
		return AE_ERROR;
	}

	if(!cxt->pri_set){
		cxt->pri_set = 1;
		ISP_LOGI("setpriority = %d",setpriority(PRIO_PROCESS, 0, -10));
		//set_sched_policy(0, SP_FOREGROUND);
	}

	cxt->capture_skip_num = work_info->capture_skip_num;
	cxt->cam_large_pix_num = work_info->noramosaic_4in1;
	cxt->zsl_flag = work_info->zsl_flag;
	cxt->is_snapshot = work_info->is_snapshot;
	cxt->snr_info = work_info->resolution_info;

	if((cxt->monitor_cfg.blk_num.w != cxt->monitor_cfg.blk_num.h) || (cxt->monitor_cfg.blk_num.w < 32) || (cxt->monitor_cfg.blk_num.w % 32)){
		cxt->monitor_cfg.blk_num.w = 32;
		cxt->monitor_cfg.blk_num.h = 32;
	}

	if (work_info->mode >= AE_WORK_MODE_MAX) {
		ISP_LOGE("fail to set work mode");
		work_info->mode = AE_WORK_MODE_COMMON;
	}

	if (1 == work_info->dv_mode)
		cxt->cur_status.adv_param.work_mode = AE_WORK_MODE_VIDEO;//ok
	else
		cxt->cur_status.adv_param.work_mode = AE_WORK_MODE_COMMON;//ok

	memset(&cxt->ctTab[0], 0, sizeof(cxt->ctTab));
	memset(&cxt->ctTabRg[0], 0, sizeof(cxt->ctTabRg));
	if (!ae_is_equal((float)(work_info->ct_table.ct[0]), 0.0)
		&& !ae_is_equal(work_info->ct_table.rg[0], 0.0)) {
		for (k = 0; k < 20; k++) {
			cxt->ctTab[k] = (float)(work_info->ct_table.ct[k]);
			cxt->ctTabRg[k] = work_info->ct_table.rg[k];
		}
	}
	ISP_LOGD("ct[0]:%f", cxt->ctTab[0]);
	cxt->awb_otp_info = work_info->awb_otp_info;

	if (0 == work_info->is_snapshot) {
		cxt->cur_status.frm_id = 0;//ok
		cxt->slw_prev_skip_num = 0;
		cxt->send_once[0] = cxt->send_once[1] = cxt->send_once[2] = cxt->send_once[3] = 0;
		last_cam_mode = (cxt->app_mode | (cxt->camera_id << 16) | (1U << 31));
	}


	if (0 != work_info->binning_factor){
		if (work_info->is_snapshot)
			cxt->binning_factor_cap = work_info->binning_factor;
		else
			cxt->binning_factor_prev = work_info->binning_factor;
	}
	else{
		cxt->binning_factor_cap = 128;
		cxt->binning_factor_prev = 128;
	}
	cxt->cur_status.img_size = work_info->resolution_info.frame_size;//ok
	cxt->cur_status.adv_param.cur_ev_setting.line_time = work_info->resolution_info.line_time;
	cxt->cur_status.adv_param.sensor_fps_range.max = work_info->sensor_fps.max_fps;
	cxt->cur_status.adv_param.sensor_fps_range.min = work_info->sensor_fps.min_fps;
	cxt->cur_status.adv_param.reserve_case = 0;

	ISP_LOGD("sync: is_multi_mode:%d, is_master:%d, sensor_role:%d", cxt->is_multi_mode, cxt->is_master, cxt->sensor_role);

	if (cxt->is_multi_mode) {
		/* save master & slave sensor info */
		struct sensor_info sensor_info;
		//cmr_u32 in = 1;

		sensor_info.max_again = cxt->sensor_max_gain;
		sensor_info.min_again = cxt->sensor_min_gain;
		sensor_info.sensor_gain_precision = cxt->sensor_gain_precision;
		sensor_info.min_exp_line = cxt->min_exp_line;
		sensor_info.line_time = cxt->cur_status.adv_param.cur_ev_setting.line_time;
		sensor_info.frm_len_def = AEC_LINETIME_PRECESION / (1.0 * work_info->resolution_info.snr_setting_max_fps * cxt->cur_status.adv_param.cur_ev_setting.line_time);

		if (CAM_SENSOR_MASTER == cxt->sensor_role) {
			rtn = cxt->ptr_isp_br_ioctrl(CAM_SENSOR_MASTER, SET_MODULE_INFO, &sensor_info, NULL);
		} else if (CAM_SENSOR_SLAVE0 == cxt->sensor_role) {
			rtn = cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE0, SET_MODULE_INFO, &sensor_info, NULL);
		} else if (CAM_SENSOR_SLAVE1 == cxt->sensor_role) {
			rtn = cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE1, SET_MODULE_INFO, &sensor_info, NULL);
		}
	}
	cxt->start_id = AE_START_ID;
	cxt->monitor_cfg.mode = AE_STATISTICS_MODE_CONTINUE;
	cxt->monitor_cfg.skip_num = 0;
	cxt->monitor_cfg.bypass = 0;
	cxt->high_fps_info.is_high_fps = work_info->sensor_fps.is_high_fps;
	cxt->cam_4in1_mode = work_info->cam_4in1_mode;

	if (work_info->sensor_fps.is_high_fps) {
		ae_skip_num = work_info->sensor_fps.high_fps_skip_num - 1;
		if (ae_skip_num > 0) {
			cxt->monitor_cfg.skip_num = ae_skip_num;
			cxt->high_fps_info.min_fps = work_info->sensor_fps.max_fps;
			cxt->high_fps_info.max_fps = work_info->sensor_fps.max_fps;
		} else {
			cxt->monitor_cfg.skip_num = 0;
			ISP_LOGV("cxt->monitor_cfg.skip_num %d", cxt->monitor_cfg.skip_num);
		}
	}

	trim.x = 0;
	trim.y = 0;
	trim.w = work_info->resolution_info.frame_size.w;
	trim.h = work_info->resolution_info.frame_size.h;
	ae_update_monitor_unit(cxt, &trim);
	ae_cfg_monitor(cxt);
	/*for sharkle monitor */
	ae_set_monitor(cxt, &trim);

	ISP_LOGD("shift = %d",work_info->shift);
	cxt->bhist_cfg.mode = AE_BAYER_HIST_MODE_CONTINUE;
	cxt->bhist_cfg.skip_num = 0;
	cxt->bhist_cfg.bypass = 0;
	bayer_hist_trim.start_x = 0;
	bayer_hist_trim.start_y = 0;
	bayer_hist_trim.end_x = work_info->resolution_info.frame_size.w;
	bayer_hist_trim.end_y = work_info->resolution_info.frame_size.h;
	/*for bayer hist monitor*/
	ae_update_bayer_hist(cxt, &bayer_hist_trim);
	ae_set_bayer_hist(cxt, &bayer_hist_trim);//this value should be from cxt->bhist_cfg,this value should be from ae_set_ae_init_param;but now it is from above assignment operator at ae_update_bayer_hist 
	cxt->cur_status.bhist_size.start_x = cxt->bhist_cfg.hist_rect.start_x;
	cxt->cur_status.bhist_size.start_y = cxt->bhist_cfg.hist_rect.start_y;
	cxt->cur_status.bhist_size.end_x = cxt->bhist_cfg.hist_rect.end_x;
	cxt->cur_status.bhist_size.end_y = cxt->bhist_cfg.hist_rect.end_y;

	if (cxt->is_multi_mode == ISP_ALG_DUAL_C_C || cxt->is_multi_mode ==ISP_ALG_DUAL_W_T || cxt->is_multi_mode ==ISP_ALG_DUAL_C_M) {
		if((CAM_SENSOR_SLAVE0 == cxt->sensor_role) && cxt->is_multi_mode){
			struct aem_info slave_aem_info;
			slave_aem_info.aem_stat_blk_pixels = cxt->monitor_cfg.blk_size.w * cxt->monitor_cfg.blk_size.h;
			slave_aem_info.aem_stat_win_w = cxt->monitor_cfg.blk_num.w;
			slave_aem_info.aem_stat_win_h = cxt->monitor_cfg.blk_num.h;

			rtn = cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE0, SET_SLAVE_AEM_INFO, &slave_aem_info, NULL);
			ISP_LOGD("sync:slave0, SET_SLAVE_AEM_INFO");
		}
	}

	cxt->calc_results.monitor_info.trim = cxt->monitor_cfg.trim;
	cxt->calc_results.monitor_info.win_num = cxt->monitor_cfg.blk_num;
	cxt->calc_results.monitor_info.win_size = cxt->monitor_cfg.blk_size;

	if(CAMERA_MODE_MANUAL == cxt->app_mode)
	{
		cxt->cur_status.adv_param.prof_mode = 1;
	} else {
		cxt->cur_status.adv_param.prof_mode = 0;
		cxt->cur_status.adv_param.comp_param.mode = 0;
		cxt->cur_status.adv_param.mode_param.mode = AE_MODE_AUTO;
	}

	scene_param_in.scene_mod = cxt->cur_status.adv_param.scene_mode;
	scene_param_in.iso_mod = cxt->cur_status.adv_param.iso;
	scene_param_in.flicker_mod = cxt->cur_status.adv_param.flicker;
	
	ae_lib_ioctrl(cxt->misc_handle, AE_LIB_GET_SCENE_PARAM, &scene_param_in, &cxt->ae_tbl_param);
	ae_target_lum = cxt->ae_tbl_param.target_lum;
	cxt->cur_param_target_lum = cxt->ae_tbl_param.target_lum;

	if (1 == cxt->last_enable) {
		if (cxt->cur_status.adv_param.cur_ev_setting.line_time == cxt->last_exp_param.line_time) {
			src_exp.exp_line = cxt->last_exp_param.exp_line;
			src_exp.gain = cxt->last_exp_param.gain;
			src_exp.exp_time = cxt->last_exp_param.exp_time;
			src_exp.dummy = cxt->last_exp_param.dummy;
			src_exp.frm_len = cxt->last_exp_param.frm_len;
			src_exp.frm_len_def = cxt->last_exp_param.frm_len_def;
		} else {
			src_exp.exp_line = (cmr_u32) (1.0 * cxt->last_exp_param.exp_line * cxt->last_exp_param.line_time / cxt->cur_status.adv_param.cur_ev_setting.line_time + 0.5);
			if (cxt->min_exp_line > src_exp.exp_line)
				src_exp.exp_line = cxt->min_exp_line;
			src_exp.exp_time = src_exp.exp_line * cxt->cur_status.adv_param.cur_ev_setting.line_time;
			src_exp.gain = cxt->last_exp_param.gain;
			src_exp.dummy = cxt->last_exp_param.dummy;
			src_exp.frm_len = cxt->last_exp_param.frm_len;
			src_exp.frm_len_def = cxt->last_exp_param.frm_len_def;
			last_cam_mode = 0;
		}
		src_exp.cur_index = cxt->last_index;
		src_exp.target_offset = cxt->last_exp_param.target_offset;
		src_exp.target_luma = cxt->last_exp_param.target_luma;

		if((cxt->app_mode < 64) && (!work_info->is_snapshot)){
			cmr_u32 last_app_mode = cxt->last_cam_mode & 0xff;
			cmr_u32 last_sensitivity = cxt->mode_switch[last_app_mode].sensitivity;
			cmr_u32 cur_sensitivity = cxt->mode_switch[cxt->app_mode].sensitivity;
			if(0 == cur_sensitivity)
				cur_sensitivity = last_sensitivity;

			if((CAMERA_MODE_MANUAL == cxt->app_mode) && (0 != s_ae_manual[cxt->camera_id].gain)){
				src_exp.target_offset = s_ae_manual[cxt->camera_id].target_offset;
				src_exp.exp_line = s_ae_manual[cxt->camera_id].exp_line;
				src_exp.gain = s_ae_manual[cxt->camera_id].gain;
				src_exp.exp_time = s_ae_manual[cxt->camera_id].exp_time;
				src_exp.dummy = s_ae_manual[cxt->camera_id].dummy;
				src_exp.frm_len = s_ae_manual[cxt->camera_id].frm_len;
				src_exp.frm_len_def = s_ae_manual[cxt->camera_id].frm_len_def;
				src_exp.cur_index = s_ae_manual[cxt->camera_id].table_idx;
				src_exp.target_luma = s_ae_manual[cxt->camera_id].target_luma;
				cxt->manual_level = s_ae_manual[cxt->camera_id].manual_level;
			}
			else if((cxt->sync_cur_result.cur_bv < cxt->flash_thrd.thd_down) && (0 != cxt->mode_switch[cxt->app_mode].gain)){
				src_exp.target_offset = cxt->mode_switch[cxt->app_mode].target_offset;
				src_exp.exp_line = cxt->mode_switch[cxt->app_mode].exp_line;
				src_exp.gain = cxt->mode_switch[cxt->app_mode].gain;
				src_exp.exp_time = cxt->mode_switch[cxt->app_mode].exp_time;
				src_exp.dummy = cxt->mode_switch[cxt->app_mode].dummy;
				src_exp.cur_index = cxt->mode_switch[cxt->app_mode].table_idx;
				src_exp.frm_len = cxt->mode_switch[cxt->app_mode].frm_len;
				src_exp.frm_len_def = cxt->mode_switch[cxt->app_mode].frm_len_def;
				src_exp.target_luma =  cxt->mode_switch[cxt->app_mode].tarlum;
			}
			else {
				if(cxt->app_mode_tarlum[cxt->app_mode])
					ae_target_lum = cxt->app_mode_tarlum[cxt->app_mode];
				src_exp.target_luma = ae_target_lum;

				if(ae_target_lum && (ISP_ALG_SINGLE == cxt->is_multi_mode)){
					ISP_LOGD("1. exp_line=%d  gain=%d",src_exp.exp_line, src_exp.gain);
					cmr_u32 tmp_gain = 0;
					cxt->last_cur_lum = cxt->last_cur_lum ? cxt->last_cur_lum : 1;
					tmp_gain = (cmr_u32) (1.0 * src_exp.gain * ae_target_lum * cur_sensitivity / cxt->last_cur_lum / last_sensitivity + 0.5);
					if(tmp_gain > cxt->ae_tbl_param.max_gain){
						tmp_gain = cxt->ae_tbl_param.max_gain;
						src_exp.exp_line = src_exp.exp_line * ae_target_lum * src_exp.gain / (tmp_gain * cxt->last_cur_lum);
						max_expl = (cmr_u32) (1.0 * cxt->ae_tbl_param.max_exp / cxt->cur_status.adv_param.cur_ev_setting.line_time + 0.5);
						if(src_exp.exp_line > max_expl)
							src_exp.exp_line = max_expl;
						src_exp.exp_time = src_exp.exp_line * cxt->cur_status.adv_param.cur_ev_setting.line_time;
					}
					src_exp.gain = tmp_gain;
					ISP_LOGD("2. exp_line=%d  gain=%d tar_lum=(%d %d) sensitivity(%d %d)",src_exp.exp_line, src_exp.gain, cxt->last_cur_lum, ae_target_lum,last_sensitivity,cur_sensitivity);
				}
			}

			if (work_info->is_snapshot && (cxt->cur_status.adv_param.cur_ev_setting.line_time != cxt->last_exp_param.line_time)){
				src_exp.exp_line = (cmr_u32) (1.0 * cxt->last_exp_param.exp_line * cxt->last_exp_param.line_time / cxt->cur_status.adv_param.cur_ev_setting.line_time + 0.5);
				if (cxt->min_exp_line > src_exp.exp_line) {
					src_exp.exp_line = cxt->min_exp_line;
				}
				src_exp.exp_time = src_exp.exp_line * cxt->cur_status.adv_param.cur_ev_setting.line_time;
			}
		}
	} else {
		ae_read_exp_gain_param(&s_bakup_exp_param[0], sizeof(s_bakup_exp_param) / sizeof(struct ae_exposure_param),&s_ae_manual[0]);
		if((CAMERA_MODE_MANUAL == cxt->app_mode) &&(0 != s_ae_manual[cxt->camera_id].gain)){
			src_exp.target_offset = s_ae_manual[cxt->camera_id].target_offset;
			src_exp.exp_line = s_ae_manual[cxt->camera_id].exp_line;
			src_exp.gain = s_ae_manual[cxt->camera_id].gain;
			src_exp.exp_time = s_ae_manual[cxt->camera_id].exp_time;
			src_exp.dummy = s_ae_manual[cxt->camera_id].dummy;
			src_exp.frm_len = s_ae_manual[cxt->camera_id].frm_len;
			src_exp.frm_len_def = s_ae_manual[cxt->camera_id].frm_len_def;
			src_exp.cur_index = s_ae_manual[cxt->camera_id].table_idx;
			src_exp.target_luma = s_ae_manual[cxt->camera_id].target_luma;
			cxt->manual_level = s_ae_manual[cxt->camera_id].manual_level;
			ISP_LOGD("manual param read from ae.file");
		} else	if ((0 != s_bakup_exp_param[cxt->camera_id].exp_line)
			&& (0 != s_bakup_exp_param[cxt->camera_id].exp_time)
			&& (0 == s_bakup_exp_param[cxt->camera_id].is_ev_setting)
			&& (0 != s_bakup_exp_param[cxt->camera_id].gain)
			&& (0 != s_bakup_exp_param[cxt->camera_id].bv)) {
			src_exp.exp_line = s_bakup_exp_param[cxt->camera_id].exp_time / cxt->cur_status.adv_param.cur_ev_setting.line_time;
			src_exp.exp_time = s_bakup_exp_param[cxt->camera_id].exp_time;
			src_exp.gain = s_bakup_exp_param[cxt->camera_id].gain;
			src_exp.cur_index = s_bakup_exp_param[cxt->camera_id].cur_index;
			src_exp.target_offset = s_bakup_exp_param[cxt->camera_id].target_offset;
			src_exp.frm_len = s_bakup_exp_param[cxt->camera_id].frm_len;
			src_exp.frm_len_def = s_bakup_exp_param[cxt->camera_id].frm_len_def;
			src_exp.target_luma = s_bakup_exp_param[cxt->camera_id].target_luma;
			cxt->sync_cur_result.cur_bv = cxt->cur_result.cur_bv = s_bakup_exp_param[cxt->camera_id].bv;
			ISP_LOGD("file expl=%d, gain=%d",src_exp.exp_line,src_exp.gain);
		}else{
			src_exp.exp_line = cxt->ae_tbl_param.def_expline;
			src_exp.exp_time =  src_exp.exp_line * cxt->snr_info.line_time;
			src_exp.gain = cxt->ae_tbl_param.def_gain;
			src_exp.cur_index = cxt->ae_tbl_param.def_index;
			src_exp.frm_len = cxt->cur_status.adv_param.cur_ev_setting.frm_len;
			src_exp.dummy = cxt->cur_status.adv_param.cur_ev_setting.dmy_line;
			src_exp.target_luma = cxt->ae_tbl_param.target_lum;
			cxt->cur_result.stable = 0;
			src_exp.target_offset = 0;
			cxt->sync_cur_result.cur_bv = cxt->cur_result.cur_bv = 500;
			ISP_LOGD("def expl=%d, gain=%d",src_exp.exp_line,src_exp.gain);
		}
	}
	cxt->cur_status.adv_param.last_target = src_exp.target_luma;
	ISP_LOGD("last_target=%d",cxt->cur_status.adv_param.last_target);

	if (((1 == cxt->last_enable) && ((1 == work_info->is_snapshot) || (last_cam_mode == cxt->last_cam_mode))) || (CAMERA_MODE_MANUAL == cxt->app_mode)) {
		dst_exp.exp_time = src_exp.exp_time;
		dst_exp.exp_line = src_exp.exp_line;
		dst_exp.gain = src_exp.gain;
		dst_exp.dummy = src_exp.dummy;
		dst_exp.cur_index = src_exp.cur_index;
		dst_exp.frm_len = src_exp.frm_len;
		dst_exp.frm_len_def = src_exp.frm_len_def;
	} else {
		src_exp.dummy = 0;
		max_expl = (cmr_u32) (1.0 * cxt->ae_tbl_param.max_exp / cxt->cur_status.adv_param.cur_ev_setting.line_time + 0.5);
		if (work_info->sensor_fps.is_high_fps) {
			fps_range.min = work_info->sensor_fps.max_fps;
			fps_range.max = work_info->sensor_fps.max_fps;
			cxt->cur_status.adv_param.sensor_fps_range.max = work_info->sensor_fps.max_fps;
		}else if(CAMERA_MODE_SLOWMOTION == cxt->app_mode){
			fps_range.min = cxt->cur_status.adv_param.sensor_fps_range.max;
			fps_range.max = cxt->cur_status.adv_param.sensor_fps_range.max;
		}else {
			fps_range.min = cxt->fps_range.min;
			fps_range.max = cxt->fps_range.max;
		}
		ae_adjust_exp_gain(cxt, &src_exp, &fps_range, max_expl, &dst_exp);

	}

	cxt->cur_result.ev_setting.exp_time = dst_exp.exp_time;
	cxt->cur_result.ev_setting.exp_line = dst_exp.exp_line;
	cxt->cur_result.ev_setting.ae_gain = dst_exp.gain;
	cxt->cur_result.ev_setting.dmy_line = dst_exp.dummy;
	cxt->cur_result.ev_setting.ae_idx = dst_exp.cur_index;
	cxt->cur_result.stable = 0;
	cxt->cur_result.ev_setting.frm_len = dst_exp.frm_len;
	cxt->sync_cur_result.ev_setting.exp_time = cxt->cur_result.ev_setting.exp_time;
	cxt->sync_cur_result.ev_setting.exp_line = cxt->cur_result.ev_setting.exp_line;
	cxt->sync_cur_result.ev_setting.ae_gain = cxt->cur_result.ev_setting.ae_gain;
	cxt->sync_cur_result.ev_setting.dmy_line = cxt->cur_result.ev_setting.dmy_line;
	cxt->sync_cur_result.ev_setting.ae_idx = cxt->cur_result.ev_setting.ae_idx;
	cxt->sync_cur_result.stable = cxt->cur_result.stable;
	cxt->sync_cur_result.ev_setting.frm_len = cxt->cur_result.ev_setting.frm_len;

	cxt->effect_index_index = 0;

	ae_make_calc_result(cxt, &cxt->sync_cur_result, &cxt->calc_results);

	/*update parameters to sensor */
	memset((void *)&cxt->exp_data, 0, sizeof(cxt->exp_data));
	cxt->exp_data.lib_data.exp_line = cxt->sync_cur_result.ev_setting.exp_line;
	cxt->exp_data.lib_data.exp_time = cxt->sync_cur_result.ev_setting.exp_time;
	cxt->exp_data.lib_data.gain = cxt->sync_cur_result.ev_setting.ae_gain;
	cxt->exp_data.lib_data.dummy = cxt->sync_cur_result.ev_setting.dmy_line;
	cxt->exp_data.lib_data.line_time = cxt->cur_status.adv_param.cur_ev_setting.line_time;
	cxt->exp_data.lib_data.frm_len = cxt->sync_cur_result.ev_setting.frm_len;

	ISP_LOGD("ae_set_video_start, ae_update_result_to_sensor,sensor_role:%d",cxt->sensor_role);
	rtn = ae_update_result_to_sensor(cxt, &cxt->exp_data, 1);

	/*it is normal capture, not in flash mode */
	if ((1 == cxt->last_enable)
		&& ((FLASH_NONE == cxt->cur_status.adv_param.flash)
			|| (FLASH_LED_OFF == cxt->cur_status.adv_param.flash))) {
		if (0 == work_info->is_snapshot) {
			//cxt->last_enable = 0;
			cxt->cur_status.adv_param.cur_ev_setting.exp_line = cxt->sync_cur_result.ev_setting.exp_line;
			cxt->cur_status.adv_param.cur_ev_setting.ae_gain = cxt->sync_cur_result.ev_setting.ae_gain;
			cxt->cur_status.adv_param.is_snapshot =  work_info->is_snapshot;//ok
			ae_set_restore_cnt(cxt,2);
		} else {
			ae_set_pause(cxt,0);
			if(cxt->last_table_index){
				cxt->cur_status.adv_param.cur_ev_setting.ae_idx = cxt->last_table_index;
			}else{
				cxt->cur_status.adv_param.cur_ev_setting.ae_idx = 0;
			}
			cxt->cur_status.adv_param.cur_ev_setting.exp_line = cxt->sync_cur_result.ev_setting.exp_line;
			cxt->cur_status.adv_param.cur_ev_setting.ae_gain = cxt->sync_cur_result.ev_setting.ae_gain;
			cxt->cur_status.adv_param.is_snapshot =  work_info->is_snapshot;//ok
			ISP_LOGV("table_idx:%d",cxt->cur_status.adv_param.cur_ev_setting.ae_idx);
		}
	}

	cxt->is_first = 1;
	ISP_LOGI("AE_VIDEO_START cam-id %d BV %d lt %d W %d H %d , exp: %d, gain:%d flash: %d CAP %d, enable: %d, large_pix_num %d",
			 cxt->camera_id, cxt->cur_result.cur_bv, cxt->cur_status.adv_param.cur_ev_setting.line_time, cxt->snr_info.frame_size.w,
			 cxt->snr_info.frame_size.h, cxt->sync_cur_result.ev_setting.exp_line, cxt->sync_cur_result.ev_setting.ae_gain,
			 cxt->cur_status.adv_param.flash, work_info->is_snapshot, cxt->last_enable, cxt->cam_large_pix_num);

	return rtn;
}

static cmr_s32 ae_set_dc_dv_mode(struct ae_ctrl_cxt *cxt, void *param)
{
	if (param) {
		if (1 == *(cmr_u32 *) param) {
			cxt->cur_status.adv_param.work_mode = AE_WORK_MODE_VIDEO;
		} else {
			cxt->cur_status.adv_param.work_mode = AE_WORK_MODE_COMMON;
		}
		ISP_LOGV("AE_SET_DC_DV %d", cxt->cur_status.adv_param.work_mode);
	}

	return AE_SUCCESS;
}

static cmr_s32 ae_set_scene(struct ae_ctrl_cxt *cxt, void *param)
{
	if (param) {
		struct ae_set_scene *scene_mode = param;

		if (scene_mode->mode < AE_SCENE_MOD_MAX)
			cxt->cur_status.adv_param.scene_mode = (cmr_s8) scene_mode->mode;

		ISP_LOGD("AE_SET_SCENE %d\n", cxt->cur_status.adv_param.scene_mode);
	}

	return AE_SUCCESS;
}

static cmr_s32 ae_set_iso(struct ae_ctrl_cxt *cxt, void *param)
{
	if (param) {
		struct ae_set_iso *iso = param;
		if (iso->mode < AE_ISO_MAX) {
			if(cxt->cur_status.adv_param.iso == iso->mode){
				ISP_LOGD("iso mode no change,no need to set");
				return AE_SUCCESS;
			}
			cxt->cur_status.adv_param.iso = iso->mode;
		}
		switch (iso->mode) {
		case AE_ISO_100:
			cxt->manual_iso_value = 256;
			break;
		case AE_ISO_200:
			cxt->manual_iso_value = 512;
			break;
		case AE_ISO_400:
			cxt->manual_iso_value = 1024;
			break;
		case AE_ISO_800:
			cxt->manual_iso_value = 2048;
			break;
		case AE_ISO_1600:
			cxt->manual_iso_value = 4096;
			break;
		case AE_ISO_AUTO:
		case AE_ISO_MAX:
		default:
			cxt->manual_iso_value = 0;
			break;
		}

		if(cxt->manual_exp_time){
			if(cxt->manual_iso_value){
				ae_set_force_pause(cxt, 1, 8);
				cxt->cur_status.adv_param.mode_param.value.exp_gain[1] = cxt->manual_iso_value;
				cxt->cur_status.adv_param.mode_param.mode = AE_MODE_MANUAL_EXP_GAIN;
			}else{
				ae_set_force_pause(cxt, 0, 9);
				cxt->cur_status.adv_param.mode_param.mode = AE_MODE_AUTO_SHUTTER_PRI;
			}
			cxt->cur_status.adv_param.mode_param.value.exp_gain[0] = cxt->manual_exp_time;
		} else {
			if(cxt->manual_iso_value){
				ae_set_force_pause(cxt, 0, 10);
				cxt->cur_status.adv_param.mode_param.value.exp_gain[1] = cxt->manual_iso_value;
				cxt->cur_status.adv_param.mode_param.mode = AE_MODE_AUTO_ISO_PRI;

			}else{
				ae_set_force_pause(cxt, 0, 11);
				cxt->cur_status.adv_param.mode_param.mode = AE_MODE_AUTO;
			}
		}
		ISP_LOGD("manual_exp_time %d, manual_iso_value %d, exp %d, mode %d",cxt->manual_exp_time,cxt->manual_iso_value, cxt->cur_status.adv_param.mode_param.value.exp_gain[0], cxt->cur_status.adv_param.mode_param.mode);
	}
	return AE_SUCCESS;
}

static cmr_s32 ae_set_flicker(struct ae_ctrl_cxt *cxt, void *param)
{
	if (param) {
		struct ae_set_flicker *flicker = param;
		if (flicker->mode < AE_FLICKER_MAX) 
			cxt->cur_status.adv_param.flicker = flicker->mode;
		ISP_LOGV("AE_SET_FLICKER %d\n", cxt->cur_status.adv_param.flicker);
	}
	return AE_SUCCESS;
}

static cmr_s32 ae_set_weight(struct ae_ctrl_cxt *cxt, void *param)
{
	if (param) {
		struct ae_set_weight *weight = param;

		if (weight->mode < AE_WEIGHT_MAX) {
			cxt->cur_status.adv_param.metering_mode = weight->mode;
		}
		ISP_LOGV("AE_SET_WEIGHT %d\n", cxt->cur_status.adv_param.metering_mode);
	}

	return AE_SUCCESS;
}

static cmr_s32 ae_set_touch_zone(struct ae_ctrl_cxt *cxt, void *param)
{
	cmr_s32 rtn = AE_SUCCESS;

	if (param) {
		struct ae_set_tuoch_zone *touch_zone = param;
		if ((touch_zone->touch_zone.w > 1) && (touch_zone->touch_zone.h > 1)) {
			cxt->cur_result.stable = 0;
			cxt->cur_status.adv_param.touch_roi_flag = 1;
			cxt->cur_status.adv_param.touch_roi = touch_zone->touch_zone;

			cxt->backup_touch.exp_line = cxt->sync_cur_result.ev_setting.exp_line;
			cxt->backup_touch.exp_time = cxt->sync_cur_result.ev_setting.exp_line * cxt->cur_status.adv_param.cur_ev_setting.line_time;
			cxt->backup_touch.dummy = cxt->sync_cur_result.ev_setting.dmy_line;
			cxt->backup_touch.gain = cxt->sync_cur_result.ev_setting.ae_gain;
			cxt->backup_touch.line_time = cxt->cur_status.adv_param.cur_ev_setting.line_time;

			ISP_LOGD("AE_SET_TOUCH_ZONE ae triger");
		} else {
			ISP_LOGV("AE_SET_TOUCH_ZONE touch ignore\n");
		}
	}

	return rtn;
}

static cmr_s32 ae_set_3dnr_mode(struct ae_ctrl_cxt *cxt, cmr_u32 *mode)
{
	if(2 == *mode)
		cxt->threednr_mode = AE_3DNR_AUTO;
	else if(1 == *mode)
		cxt->threednr_mode = AE_3DNR_ON;
	else if(0 == *mode)
		cxt->threednr_mode = AE_3DNR_OFF;

	ISP_LOGD("threednr_mode=%d",cxt->threednr_mode);

	return AE_SUCCESS;
}

static cmr_s32 ae_set_3dnr_thr(struct ae_ctrl_cxt *cxt, cmr_handle *mode)
{
	struct ae_thd_param *ae_thrd = (struct ae_thd_param *)mode;
	cxt->threednr_thrd.thd_up = ae_thrd->thd_up;
	cxt->threednr_thrd.thd_down = ae_thrd->thd_down;

	return AE_SUCCESS;
}

static cmr_s32 ae_set_ev_offset(struct ae_ctrl_cxt *cxt, void *param)
{
	if (param) {
		struct ae_set_ev *ev = param;

		if (ev->level < AE_LEVEL_MAX){
			cxt->cur_status.adv_param.comp_param.mode = 1;
			cxt->cur_status.adv_param.prof_mode = 1;
			cxt->cur_status.adv_param.comp_param.value.ev_index = ev->level;
		} else {
			cxt->cur_status.adv_param.comp_param.mode = 0;
			cxt->cur_status.adv_param.prof_mode = 0;
			cxt->cur_status.adv_param.comp_param.value.ev_value = 0.0;
		}

		ISP_LOGV("AE_SET_EV_OFFSET %d", cxt->cur_status.adv_param.comp_param.value.ev_index);
	}
	return AE_SUCCESS;
}

static cmr_s32 ae_set_exposure_compensation(struct ae_ctrl_cxt *cxt, struct ae_exp_compensation *exp_comp)
{
	if (exp_comp) {
		if(exp_comp->comp_val)
			cxt->is_ev_setting = 1;
		else
			cxt->is_ev_setting = 0;

		if (((1 == cxt->app_mode) && (cxt->cur_status.adv_param.flash == FLASH_NONE)) || ((1 == cxt->app_mode) && (cxt->cur_status.adv_param.flash == FLASH_MAIN_AFTER))) {
			struct ae_set_ev ev;
			ev.level = exp_comp->comp_val + exp_comp->comp_range.max;
			if (ev.level < AE_LEVEL_MAX) {
				cxt->cur_status.adv_param.comp_param.value.ev_index = ev.level;
			} else {
				cxt->cur_status.adv_param.comp_param.value.ev_index = 0;
			}
			cxt->cur_status.adv_param.comp_param.mode = 1;
			ISP_LOGD("ev.level:%d, comp_val: %d, comp_range.max:%d",ev.level, exp_comp->comp_val, exp_comp->comp_range.max);
		} else {
			cxt->cur_status.adv_param.comp_param.mode = 0;
			cxt->cur_status.adv_param.comp_param.value.ev_value = 1.0 * exp_comp->comp_val * exp_comp->step_numerator / exp_comp->step_denominator;
			ISP_LOGD("comp_val=%d, ev_value=%f, range=[%d,%d], lock=%d, step_numerator=%d, step_denominator = %d",
				exp_comp->comp_val, cxt->cur_status.adv_param.comp_param.value.ev_value, exp_comp->comp_range.min, exp_comp->comp_range.max, cxt->force_lock_ae,
				exp_comp->step_numerator, exp_comp->step_denominator);
		}
	}

	return AE_SUCCESS;
}

static cmr_s32 ae_set_scene_info(struct ae_ctrl_cxt *cxt,  void *param)
{
	if (param) {
		struct ai_scene_detect_info *scene_info = (struct ai_scene_detect_info *) param;

		memcpy(&cxt->cur_status.adv_param.detect_scene, scene_info, sizeof(struct ai_scene_detect_info));
		ISP_LOGV("done");
	}

	return AE_SUCCESS;
}

static cmr_s32 ae_set_auto_hdr(struct ae_ctrl_cxt *cxt,  void *param)
{
	if (param) {
		cmr_u8 *menu_ctrl = (cmr_u8 *) param;
		cxt->hdr_menu_ctrl = *menu_ctrl;
		ISP_LOGI("hdr_menu %d", cxt->hdr_menu_ctrl);
	}
	return AE_SUCCESS;
}

static cmr_s32 ae_set_auto_fdr(struct ae_ctrl_cxt *cxt,  void *param)
{
	if (param) {
		cmr_u8 *menu_ctrl = (cmr_u8 *) param;
		cxt->fdr_menu_ctrl = *menu_ctrl;
		ISP_LOGI("fdr_menu %d", cxt->fdr_menu_ctrl);
	}
	return AE_SUCCESS;
}

static cmr_s32 ae_set_fps_info(struct ae_ctrl_cxt *cxt, void *param)
{
	if (param) {
			struct ae_set_fps *fps = param;
			cxt->fps_range.min = fps->min_fps;
			cxt->fps_range.max = fps->max_fps;
			ISP_LOGV("AE_SET_FPS (%d, %d)", fps->min_fps, fps->max_fps);
	}

	return AE_SUCCESS;
}

static cmr_s32 ae_get_flash_effect(struct ae_ctrl_cxt *cxt, void *result)
{
	if (result) {
		cmr_u32 *effect = (cmr_u32 *) result;

		*effect = cxt->flash_effect;
		ISP_LOGV("flash_effect %d", *effect);
	}

	return AE_SUCCESS;
}

static cmr_s32 ae_get_flash_enable(struct ae_ctrl_cxt *cxt, void *result)
{
	cmr_s32 rtn = AE_SUCCESS;
	if (result) {
		cmr_u32 *flash_eb = (cmr_u32 *) result;
		cmr_s32 bv = 0;

		rtn = ae_get_bv_by_lum_new(cxt, &bv);

		if (bv <= cxt->flash_thrd.thd_down && cxt->last_enable && (cxt->sync_cur_result.cur_lum < 200))
			*flash_eb = 1;
		else if (bv > cxt->flash_thrd.thd_up)
			*flash_eb = 0;
		else
			*flash_eb = 0;

		ISP_LOGD("AE_GET_FLASH_EB: cameraId=%d, flash_eb=%d, bv=%d, on_thr=%d, off_thr=%d",cxt->camera_id, *flash_eb, bv, cxt->flash_thrd.thd_down, cxt->flash_thrd.thd_up);
	}
	return rtn;
}

static cmr_s32 ae_get_gain(struct ae_ctrl_cxt *cxt, void *result)
{
	if (result) {
		cmr_s32 *bv = (cmr_s32 *) result;

		*bv = cxt->cur_result.ev_setting.ae_gain;
	}
	return AE_SUCCESS;
}

static cmr_s32 ae_get_flash_env_ratio(struct ae_ctrl_cxt *cxt, void *result)
{
	if (result) {
		float *captureFlashEnvRatio = (float *)result;
		*captureFlashEnvRatio = cxt->flash_esti_result.captureFlashRatio;
	}

	return AE_SUCCESS;
}

static cmr_s32 ae_get_flash_one_in_all_ratio(struct ae_ctrl_cxt *cxt, void *result)
{
	if (result) {
		float *captureFlash1ofALLRatio = (float *)result;
		*captureFlash1ofALLRatio = cxt->flash_esti_result.captureFlash1Ratio;
	}

	return AE_SUCCESS;
}

static cmr_s32 ae_get_luma(struct ae_ctrl_cxt *cxt, void *result)
{
	if (result) {
		cmr_u32 *lum = (cmr_u32 *) result;
		*lum = cxt->cur_result.cur_lum;
	}

	return AE_SUCCESS;
}

static cmr_s32 ae_get_monitor_info(struct ae_ctrl_cxt *cxt, void *result)
{
	if (result) {
		struct ae_monitor_info *info = result;
		info->win_size = cxt->monitor_cfg.blk_size;
		info->win_num = cxt->monitor_cfg.blk_num;
		info->trim = cxt->monitor_cfg.trim;
	}

	return AE_SUCCESS;
}

static cmr_s32 ae_get_flicker_mode(struct ae_ctrl_cxt *cxt, void *result)
{
	if (result) {
		cmr_u32 *mode = result;

		*mode = cxt->cur_status.adv_param.flicker;
	}

	return AE_SUCCESS;
}

static cmr_s32 ae_get_app_mode(struct ae_ctrl_cxt *cxt, void *result)
{
	if (result) {
		cmr_u32 *mode = result;
		*mode = cxt->app_mode;
		ISP_LOGV("app_mode:%d", *mode);
	}
	return AE_SUCCESS;
}
static cmr_s32 ae_get_dc_dv_fps_range(struct ae_ctrl_cxt *cxt, void *result)
{
	if (result) {
		struct ae_fps_range *fps = (struct ae_fps_range *)result;
		fps->dc_fps_max = cxt->dc_fps_range.max;
		fps->dc_fps_min = cxt->dc_fps_range.min;
		fps->dv_fps_max = cxt->dv_fps_range.max;
		fps->dv_fps_min = cxt->dv_fps_range.min;
		ISP_LOGD("dc min/max:(%d %d) dv min/max:(%d %d)", fps->dc_fps_min,fps->dc_fps_max,fps->dv_fps_min,fps->dv_fps_max);
	}else{
		ISP_LOGE("result pointer is NULL");
	}
	return AE_SUCCESS;
}

static cmr_s32 ae_get_lowlight_flag_by_bv(struct ae_ctrl_cxt *cxt, cmr_u32 * is_lowlight)
{
        cmr_s32 rtn = AE_SUCCESS;

        if (NULL == cxt || NULL == is_lowlight) {
                ISP_LOGE("fail to get is_lowlight by bv, cxt %p is_lowlight %p", cxt, is_lowlight);
                return AE_ERROR;
        }

        *is_lowlight = cxt->is_fourcell;
        ISP_LOGV("get is_lowlight %d", *is_lowlight);
        return rtn;
}

static cmr_s32 ae_set_hdr_start(struct ae_ctrl_cxt *cxt, void *param)
{
	if((ISP_ALG_DUAL_C_C == cxt->is_multi_mode) && (CAM_SENSOR_MASTER != cxt->sensor_role)){
		ISP_LOGD("[HDR]is_multi_mode=%d",cxt->is_multi_mode);
	}
	else if (param) {
		struct ae_hdr_param *hdr_param = (struct ae_hdr_param *)param;
		cxt->hdr_enable = hdr_param->hdr_enable;
		cxt->hdr_cb_cnt = hdr_param->ev_effect_valid_num;
		cxt->hdr_frame_cnt = 0;
		if (cxt->hdr_enable) {
			cxt->hdr_flag = 3;
			cxt->hdr_exp_line = cxt->sync_cur_result.ev_setting.exp_line;
			cxt->hdr_gain = cxt->sync_cur_result.ev_setting.ae_gain;
			ae_set_force_pause(cxt, 1, 12);
		} else {
			ae_set_force_pause(cxt, 0, 13);
			cxt->cur_status.adv_param.lock = AE_STATE_NORMAL;
			cxt->cur_status.adv_param.prof_mode = 0;
			cxt->cur_status.adv_param.mode_param.mode = AE_MODE_AUTO;
		}
		ISP_LOGD("AE_SET_HDR: hdr_enable %d, hdr_cb_cnt %d, expl %d, gain %d, lock_ae_state %d",
			cxt->hdr_enable,
			cxt->hdr_cb_cnt,
			cxt->hdr_exp_line,
			cxt->hdr_gain,
			cxt->cur_status.adv_param.lock);
	}

	return AE_SUCCESS;
}

static cmr_s32 ae_set_ev_adjust_start(struct ae_ctrl_cxt *cxt, void *param)
{
	if((ISP_ALG_DUAL_C_C == cxt->is_multi_mode) && (CAM_SENSOR_MASTER != cxt->sensor_role)){
		ISP_LOGD("[EV_ADJ]is_multi_mode=%d",cxt->is_multi_mode);
	}
	else if (param) {
		struct ae_ev_adj_param *ev_adj_param = (struct ae_ev_adj_param *)param;
		cxt->ev_adj_enable = ev_adj_param->enable;
		cxt->ev_adj_ev_effect_valid_num = ev_adj_param->ev_effect_valid_num;
		cxt->ev_adjust_cnt = ev_adj_param->ev_adjust_cnt;
		cxt->ev_adj_snp_type = ev_adj_param->type;
		cxt->ev_adj_frame_cnt = 0;
		if (cxt->ev_adj_enable) {
			cxt->ev_adj_flag = 1;
			cxt->ev_adj_exp_line = cxt->sync_cur_result.ev_setting.exp_line;
			cxt->ev_adj_gain = cxt->sync_cur_result.ev_setting.ae_gain;
			ae_set_pause(cxt, 4);
		} else {
			ae_set_restore_cnt(cxt, 6);
			cxt->ev_adj_flag = 0;
			cxt->cur_status.adv_param.prof_mode = 0;
			cxt->cur_status.adv_param.mode_param.mode = AE_MODE_AUTO;
		}
		ISP_LOGD("AE_SET_EV_ADJ:enable %d, expl %d, gain %d, lock_ae_state %d",
			cxt->ev_adj_enable,
			cxt->ev_adj_exp_line,
			cxt->ev_adj_gain,
			cxt->cur_status.adv_param.lock);
	}

	return AE_SUCCESS;
}

static void ae_set_fdr_ctrl(struct ae_ctrl_cxt *cxt, struct ae_calc_in *param)
{
	UNUSED(param);
	cmr_u32 base_exposure_line = 0;
	cmr_u32 down_exposure = 0;
	cmr_u16 base_gain = 0;
	cmr_u32 max_frame_line = 0;
	cmr_u32 min_frame_line = 0;
	cmr_u32 exp_line = 0;
	cmr_u32 gain = 0;
	float ev_result = 0.0;

	ev_result = cxt->fdr_down_ev;
	ISP_LOGD("fdr (cap) ev[0] %f", ev_result);

	cxt->fdr_frame_cnt++;
	if (cxt->fdr_cb_cnt == cxt->fdr_frame_cnt) {
		(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, AE_CB_FDR_START, NULL);
		ISP_LOGD("_isp_fdr_callback do-capture!\r\n");
	}
		max_frame_line = (cmr_u32) (1.0 * 1000000000 / cxt->fps_range.min / cxt->cur_status.adv_param.cur_ev_setting.line_time);
		min_frame_line = (cmr_u32) (1.0 * cxt->ae_tbl_param.min_exp / cxt->cur_status.adv_param.cur_ev_setting.line_time + 0.5);

		ISP_LOGV("max_frame_line %d, min_frame_line %d, fps_min %d, cur_line_time %d\n", max_frame_line, min_frame_line, cxt->fps_range.min, cxt->cur_status.adv_param.cur_ev_setting.line_time);
		//if (cxt->fdr_frame_cnt <= cxt->fdr_flag ) {
			base_exposure_line = cxt->fdr_exp_line;
			base_gain = cxt->fdr_gain;
			down_exposure = pow(2,ev_result)* base_exposure_line * cxt->cur_status.adv_param.cur_ev_setting.line_time;
			ISP_LOGD("down_exp %d, pow2 %f\n", down_exposure,  ev_result);
			ae_hdr_calculation(cxt, max_frame_line, min_frame_line, down_exposure, base_exposure_line, base_gain, &gain, &exp_line);
			ISP_LOGV("base_exposure: %d, base_gain: %d, down_exposure: %d, exp_line: %d", base_exposure_line, base_gain, down_exposure, exp_line);
			cxt->cur_status.adv_param.mode_param.value.exp_gain[0] = exp_line * cxt->cur_status.adv_param.cur_ev_setting.line_time;
			cxt->cur_status.adv_param.mode_param.value.exp_gain[1] = gain;
			cxt->cur_status.adv_param.mode_param.mode = AE_MODE_MANUAL_EXP_GAIN;
			cxt->cur_status.adv_param.prof_mode = 1;
			ISP_LOGD("_isp_fdr_down_exp: exp_line %d, gain %d\n", exp_line, gain);
		/*}else{
			base_exposure_line = cxt->fdr_exp_line;
			base_gain = cxt->fdr_gain;
			cxt->cur_status.adv_param.mode_param.value.exp_gain[0] = base_exposure_line * cxt->cur_status.adv_param.cur_ev_setting.line_time;
			cxt->cur_status.adv_param.mode_param.value.exp_gain[1] = base_gain;
			cxt->cur_status.adv_param.mode_param.mode = AE_MODE_MANUAL_EXP_GAIN;
			cxt->cur_status.adv_param.prof_mode = 1;
			ISP_LOGD("_isp_fdr_normal_exp: exp_line %d, gain %d\n", base_exposure_line, base_gain);
		}
		*/
}

static void ae_set_fdr_detect(struct ae_ctrl_cxt *cxt, struct ae_calc_in *param)
{
	cmr_s8 auto_fdr_enable = 0;
	struct ae_size fdr_stat_size = {0,0};
	float ev_result = 0.0;

	if(!cxt->is_snapshot) {
		struct fdr_stat_t fdr_stat = {0,};
		fdr_stat.hist256 = param->hist_stats.value;
		if (cxt->isp_ops.callback) {
			(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, AE_CB_HDR_STATIS_SIZE, &fdr_stat_size);
		}
		if(fdr_stat_size.w && fdr_stat_size.h) {
			fdr_stat.w = fdr_stat_size.w;
			fdr_stat.h = fdr_stat_size.h;
		} else {
			fdr_stat.w = cxt->snr_info.frame_size.w;
			fdr_stat.h = cxt->snr_info.frame_size.h;
		}

		fdr_stat.fd_param.face_num = cxt->cur_status.adv_param.face_data.face_num;
		fdr_stat.fd_param.height = cxt->cur_status.adv_param.face_data.img_size.h;
		fdr_stat.fd_param.width = cxt->cur_status.adv_param.face_data.img_size.w;

		for (int i = 0; i < fdr_stat.fd_param.face_num; i++) {
			fdr_stat.fd_param.face_area[i].angle = cxt->cur_status.adv_param.face_data.face_data[i].angle;
			fdr_stat.fd_param.face_area[i].face_lum = cxt->cur_status.adv_param.face_data.face_data[i].face_lum;
			fdr_stat.fd_param.face_area[i].pose = cxt->cur_status.adv_param.face_data.face_data[i].pose;
			fdr_stat.fd_param.face_area[i].rect.start_x = cxt->cur_status.adv_param.face_data.face_data[i].face_rect.start_x;
			fdr_stat.fd_param.face_area[i].rect.start_y = cxt->cur_status.adv_param.face_data.face_data[i].face_rect.start_y;
			fdr_stat.fd_param.face_area[i].rect.end_x = cxt->cur_status.adv_param.face_data.face_data[i].face_rect.end_x;
			fdr_stat.fd_param.face_area[i].rect.end_y = cxt->cur_status.adv_param.face_data.face_data[i].face_rect.end_y;
		}
		fdr_stat.base_target_lum = cxt->sync_cur_result.base_target_lum;
		fdr_stat.target_lum = cxt->sync_cur_result.target_lum;
		fdr_stat.face_stable = cxt->sync_cur_result.face_stable;
		fdr_stat.camera_id = cxt->camera_id;

		if(cxt->fdr_tuning_param && cxt->fdr_ops.fdr_scndet)
			auto_fdr_enable = (cmr_s8)cxt->fdr_ops.fdr_scndet(cxt->fdr_tuning_param, &fdr_stat, &ev_result,&cxt->fdr_det_status);

		cxt->fdr_calc_ev = ev_result;

		ISP_LOGV("auto_fdr camera_id %d w %d h %d face_num %d ev %f ",fdr_stat.camera_id, fdr_stat.w, fdr_stat.h, fdr_stat.fd_param.face_num, ev_result);
		ISP_LOGV("auto_fdr base target %d target %d face_stable %d ", fdr_stat.base_target_lum, fdr_stat.target_lum, fdr_stat.face_stable);
	}
	if (cxt->fdr_menu_ctrl) {
		cxt->hdr_calc_result.auto_hdr_enable = auto_fdr_enable;
		if(-1 == auto_fdr_enable)
			auto_fdr_enable = 0;
		if(cxt->isp_ops.callback)
			(*cxt->isp_ops.callback)(cxt->isp_ops.isp_handler, AE_CB_FDR_STATUS, &auto_fdr_enable);
		else
			ISP_LOGE("isp_ops.callback is NULL");
	}
}

static cmr_s32 ae_set_fdr_start(struct ae_ctrl_cxt *cxt, void *param)
{
	if((ISP_ALG_DUAL_C_C == cxt->is_multi_mode) && (CAM_SENSOR_MASTER != cxt->sensor_role)){
		ISP_LOGD("[FDR]is_multi_mode=%d",cxt->is_multi_mode);
	}
	else if (param) {
		struct ae_fdr_param *fdr_param = (struct ae_fdr_param *)param;
		cxt->fdr_enable = fdr_param->fdr_enable;
		cxt->fdr_cb_cnt = fdr_param->ev_effect_valid_num;
		cxt->fdr_flag = fdr_param->ev_effect_cnt;
		cxt->fdr_frame_cnt = 0;
		if (cxt->fdr_enable) {
			cxt->fdr_exp_line = cxt->sync_cur_result.ev_setting.exp_line;
			cxt->fdr_gain = cxt->sync_cur_result.ev_setting.ae_gain;
			cxt->fdr_down_ev = cxt->fdr_calc_ev;
			//ae_set_force_pause(cxt, 1, 16);//lock/unlock is handled by FDR capture processing
		} else {
			//ae_set_force_pause(cxt, 0, 17);
			cxt->cur_status.adv_param.prof_mode = 0;
			cxt->cur_status.adv_param.mode_param.mode = AE_MODE_AUTO;
			cxt->fdr_down_ev = 0;
			cxt->cur_result.ev_setting.exp_line = cxt->fdr_exp_line;
			cxt->cur_result.ev_setting.ae_gain = cxt->fdr_gain;
			ISP_LOGD("isp_fdr_normal_exp: exp_line %d, gain %d\n", cxt->fdr_exp_line, cxt->fdr_gain);
		}
		ISP_LOGD("AE_SET_FDR: fdr_enable %d, fdr_cb_cnt %d, expl %d, gain %d, lock_ae_state %d,fdr_down_ev %f,fdr_flag %d",
			cxt->fdr_enable,
			cxt->fdr_cb_cnt,
			cxt->fdr_exp_line,
			cxt->fdr_gain,
			cxt->cur_status.adv_param.lock,
			cxt->fdr_down_ev,
			cxt->fdr_flag);
	}

	return AE_SUCCESS;
}

static cmr_s32 ae_get_flash_wb_gain(struct ae_ctrl_cxt *cxt, void *result)
{
	cmr_s32 rtn = AE_SUCCESS;
	if (result) {
		if (cxt->flash_ver) {
			struct ae_awb_gain *flash_awb_gain = (struct ae_awb_gain *)result;
			flash_awb_gain->r = cxt->flash_esti_result.captureRGain;
			flash_awb_gain->g = cxt->flash_esti_result.captureGGain;
			flash_awb_gain->b = cxt->flash_esti_result.captureBGain;
			ISP_LOGV("dual flash: awb gain: %d, %d, %d\n", flash_awb_gain->r, flash_awb_gain->g, flash_awb_gain->b);
			rtn = AE_SUCCESS;
		} else {
			rtn = AE_ERROR;
		}
	} else {
		rtn = AE_ERROR;
	}

	return rtn;
}

static cmr_s32 ae_get_fps(struct ae_ctrl_cxt *cxt, void *result)
{
	cmr_s32 rtn = AE_SUCCESS;

	if (result) {
		cmr_u32 *fps = (cmr_u32 *) result;
		*fps = (cmr_u32) (cxt->sync_cur_result.cur_fps + 0.5);
		ISP_LOGV("fps: %f\n", cxt->sync_cur_result.cur_fps);
	} else {
		ISP_LOGE("result pointer is NULL");
		rtn = AE_ERROR;
	}

	return rtn;
}

static cmr_s32 ae_set_caf_lockae_start(struct ae_ctrl_cxt *cxt)
{
	cmr_s32 rtn = AE_SUCCESS;
	rtn = ae_lib_ioctrl(cxt->misc_handle, AE_LIB_SET_CAF_START, NULL, NULL);
	return rtn;
}

static cmr_s32 ae_set_caf_lockae_stop(struct ae_ctrl_cxt *cxt)
{
	cmr_s32 rtn = AE_SUCCESS;
	rtn = ae_lib_ioctrl(cxt->misc_handle, AE_LIB_SET_CAF_STOP, NULL, NULL);
	return rtn;
}

static cmr_s32 ae_get_led_ctrl(struct ae_ctrl_cxt *cxt, cmr_handle result)
{
	if (result) {
		struct ae_leds_ctrl *ae_leds_ctrl = (struct ae_leds_ctrl *)result;

		ae_leds_ctrl->led0_ctrl = cxt->ae_leds_ctrl.led0_ctrl;
		ae_leds_ctrl->led1_ctrl = cxt->ae_leds_ctrl.led1_ctrl;
		ISP_LOGV("AE_SET_LED led0_enable=%d, led1_enable=%d", ae_leds_ctrl->led0_ctrl, ae_leds_ctrl->led1_ctrl);
	}

	return AE_SUCCESS;
}


static cmr_s32 ae_set_isp_gain(struct ae_ctrl_cxt *cxt)
{
        struct ae_match_data ae_match_data_slave;
	if (cxt->is_multi_mode == ISP_ALG_DUAL_SBS && (CAM_SENSOR_MASTER != cxt->sensor_role)) {
		cxt->ptr_isp_br_ioctrl((CAM_SENSOR_MASTER == cxt->sensor_role) ? CAM_SENSOR_MASTER : CAM_SENSOR_SLAVE0, GET_MATCH_AE_DATA, NULL, &ae_match_data_slave);
		cmr_u32 isp_gain = ae_match_data_slave.isp_gain;
		if (0 != isp_gain) {
			double rgb_coeff = isp_gain * 1.0 / 4096;
			if (cxt->isp_ops.set_rgb_gain)
				cxt->isp_ops.set_rgb_gain(cxt->isp_ops.isp_handler, rgb_coeff);
		}
	}
	else if((cxt->is_multi_mode == ISP_ALG_DUAL_C_C || cxt->is_multi_mode ==ISP_ALG_DUAL_W_T || cxt->is_multi_mode ==ISP_ALG_DUAL_C_M   \
	        || cxt->is_multi_mode ==ISP_ALG_TRIBLE_W_T_UW)&& (CAM_SENSOR_MASTER != cxt->sensor_role)) {
		if (CAM_SENSOR_SLAVE0 == cxt->sensor_role)
			cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE0, GET_MATCH_AE_DATA, NULL, &ae_match_data_slave);
		else                    //(CAM_SENSOR_SLAVE1 == cxt->sensor_role)
			cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE1, GET_MATCH_AE_DATA, NULL, &ae_match_data_slave);
		cmr_u32 isp_gain = ae_match_data_slave.isp_gain;
		if (0 != isp_gain) {
			double rgb_coeff = isp_gain * 1.0 / 4096;
			if(cxt->isp_ops.set_rgb_gain)
				cxt->isp_ops.set_rgb_gain(cxt->isp_ops.isp_handler, rgb_coeff);
		}
	}
	else {
		if (0 != cxt->exp_data.actual_data.isp_gain) {
			double rgb_coeff = cxt->exp_data.actual_data.isp_gain * 1.0 / 4096.0;
			if (cxt->isp_ops.set_rgb_gain)
				cxt->isp_ops.set_rgb_gain(cxt->isp_ops.isp_handler, rgb_coeff);
		}
	}
	return AE_SUCCESS;
}

static cmr_s32 ae_parser_otp_info(struct ae_init_in *init_param)
{
	cmr_s32 rtn = AE_SUCCESS;
	struct sensor_otp_section_info *ae_otp_info_ptr = NULL;
	struct sensor_otp_section_info *module_info_ptr = NULL;
	struct sensor_otp_ae_info info;
	cmr_u8 *rdm_otp_data;
	cmr_u16 rdm_otp_len;
	cmr_u8 *module_info;

	if (NULL != init_param->otp_info_ptr) {
		if (CAM_SENSOR_MASTER == init_param->sensor_role) {
			ae_otp_info_ptr = init_param->otp_info_ptr->triple_otp.master_ae_info;
			module_info_ptr = init_param->otp_info_ptr->triple_otp.master_module_info;
			ISP_LOGD("pass ae otp, triple cam master");
		} else if (CAM_SENSOR_SLAVE0 == init_param->sensor_role) {
			ae_otp_info_ptr = init_param->otp_info_ptr->triple_otp.slave0_ae_info;
			module_info_ptr = init_param->otp_info_ptr->triple_otp.slave0_module_info;
			ISP_LOGD("pass ae otp, triple cam slave0");
		} else if (CAM_SENSOR_SLAVE1 == init_param->sensor_role) {
			ae_otp_info_ptr = init_param->otp_info_ptr->triple_otp.slave1_ae_info;
			module_info_ptr = init_param->otp_info_ptr->triple_otp.slave1_module_info;
			ISP_LOGD("pass ae otp, triple cam slave1");
		}
	} else {
		ae_otp_info_ptr = NULL;
		module_info_ptr = NULL;

		if(init_param->is_multi_mode == ISP_ALG_SINGLE ){
			ISP_LOGD("single camera,dual ae sync no exist.\n");
		}else{
			ISP_LOGE("ae otp_info_ptr is NULL.\n");
		}
	}

	if (NULL != ae_otp_info_ptr && NULL != module_info_ptr) {
		rdm_otp_len = ae_otp_info_ptr->rdm_info.data_size;
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
				ISP_LOGV("ae otp map v0.4 or v0.5");
				rdm_otp_data = (cmr_u8 *) ae_otp_info_ptr->rdm_info.data_addr;
			} else if (module_info[4] == 1 && module_info[5] == 0 && module_info[0] == 0x53 && module_info[1] == 0x50 && module_info[2] == 0x52 && module_info[3] == 0x44) {
				ISP_LOGV("ae otp map v1.0");
				rdm_otp_data = (cmr_u8 *) ae_otp_info_ptr->rdm_info.data_addr + 1;
			} else {
				rdm_otp_data = NULL;
				ISP_LOGE("ae otp map version error");
			}
		} else {
			rdm_otp_data = NULL;
			ISP_LOGE("ae module_info is NULL");
		}

		if (NULL != rdm_otp_data && 0 != rdm_otp_len) {
			info.ae_target_lum = (cmr_u64)((rdm_otp_data[1] << 8) | rdm_otp_data[0]);
			info.gain_1x_exp = (cmr_u64)((rdm_otp_data[5] << 24) | (rdm_otp_data[4] << 16) | (rdm_otp_data[3] << 8) | rdm_otp_data[2]);
			info.gain_2x_exp = (cmr_u64)((rdm_otp_data[9] << 24) | (rdm_otp_data[8] << 16) | (rdm_otp_data[7] << 8) | rdm_otp_data[6]);
			info.gain_4x_exp = (cmr_u64)((rdm_otp_data[13] << 24) | (rdm_otp_data[12] << 16) | (rdm_otp_data[11] << 8) | rdm_otp_data[10]);
			info.gain_8x_exp = (cmr_u64)((rdm_otp_data[17] << 24) | (rdm_otp_data[16] << 16) | (rdm_otp_data[15] << 8) | rdm_otp_data[14]);
			ISP_LOGV("ae otp map:(gain_1x_exp:%d),(gain_2x_exp:%d),(gain_4x_exp:%d),(gain_8x_exp:%d).\n",
					    (int)info.gain_1x_exp,(int)info.gain_2x_exp,(int)info.gain_4x_exp,(int)info.gain_8x_exp);

			if (CAM_SENSOR_MASTER == init_param->sensor_role) {
				rtn = init_param->ptr_isp_br_ioctrl(CAM_SENSOR_MASTER, SET_OTP_AE, &info, NULL);
			} else if (CAM_SENSOR_SLAVE0 == init_param->sensor_role) {
				rtn = init_param->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE0, SET_OTP_AE, &info, NULL);
			} else if (CAM_SENSOR_SLAVE1 == init_param->sensor_role) {
				rtn = init_param->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE1, SET_OTP_AE, &info, NULL);
			}
			//ISP_LOGV("lum=%" PRIu16 ", 1x=%" PRIu64 ", 2x=%" PRIu64 ", 4x=%" PRIu64 ", 8x=%" PRIu64, info.ae_target_lum,info.gain_1x_exp,info.gain_2x_exp,info.gain_4x_exp,info.gain_8x_exp);
		} else {
			ISP_LOGE("ae rdm_otp_data = %p, rdm_otp_len = %d. Parser fail", rdm_otp_data, rdm_otp_len);
		}
	} else {
		if(init_param->is_multi_mode == ISP_ALG_SINGLE ){
			ISP_LOGD("single camera,dual ae sync no exist.\n");
		}else{
			ISP_LOGE("ae ae_otp_info_ptr = %p, module_info_ptr = %p. Parser fail !", ae_otp_info_ptr, module_info_ptr);
		}
	}

	return AE_SUCCESS;
}

static cmr_s32 ae_set_aux_sensor(struct ae_ctrl_cxt *cxt, cmr_handle param0, cmr_handle param1)
{
	struct ae_aux_sensor_info *aux_sensor_info_ptr = (struct ae_aux_sensor_info *)param0;
	UNUSED(param1);

	switch (aux_sensor_info_ptr->type) {
	case AE_ACCELEROMETER:
		ISP_LOGV("accelerometer E\n");
		cxt->cur_status.aux_sensor_data.accelerator.validate = aux_sensor_info_ptr->gsensor_info.valid;
		cxt->cur_status.aux_sensor_data.accelerator.timestamp = aux_sensor_info_ptr->gsensor_info.timestamp;
		cxt->cur_status.aux_sensor_data.accelerator.x = aux_sensor_info_ptr->gsensor_info.vertical_down;
		cxt->cur_status.aux_sensor_data.accelerator.y = aux_sensor_info_ptr->gsensor_info.vertical_up;
		cxt->cur_status.aux_sensor_data.accelerator.z = aux_sensor_info_ptr->gsensor_info.horizontal;
		break;

	case AE_MAGNETIC_FIELD:
		ISP_LOGV("magnetic field E\n");
		break;

	case AE_GYROSCOPE:
		ISP_LOGV("gyro E\n");
		cxt->cur_status.aux_sensor_data.gyro.validate = aux_sensor_info_ptr->gyro_info.valid;
		cxt->cur_status.aux_sensor_data.gyro.timestamp = aux_sensor_info_ptr->gyro_info.timestamp;
		cxt->cur_status.aux_sensor_data.gyro.x = aux_sensor_info_ptr->gyro_info.x;
		cxt->cur_status.aux_sensor_data.gyro.y = aux_sensor_info_ptr->gyro_info.y;
		cxt->cur_status.aux_sensor_data.gyro.z = aux_sensor_info_ptr->gyro_info.z;
		break;

	case AE_LIGHT:
		ISP_LOGV("light E\n");
		break;

	case AE_PROXIMITY:
		ISP_LOGV("proximity E\n");
		break;

	default:
		ISP_LOGD("sensor type not support");
		break;
	}
	return AE_SUCCESS;
}

static cmr_s32 ae_get_calc_reuslts(struct ae_ctrl_cxt *cxt, cmr_handle result)
{
	cmr_s32 rtn = AE_SUCCESS;
	struct ae_calc_results *calc_result = (struct ae_calc_results*)result;

	if (NULL == calc_result) {
		ISP_LOGE("results pointer is invalidated\n");
		rtn = AE_ERROR;
		return rtn;
	}

	calc_result->is_skip_cur_frame = cxt->calc_results.is_skip_cur_frame;
	memcpy(&calc_result->ae_result_3_x, &cxt->calc_results.ae_result, sizeof(struct ae_calc_results_3_x));
	memcpy(&calc_result->ae_output, &cxt->calc_results.ae_output, sizeof(struct ae_calc_out));
	memcpy(&calc_result->ae_ev, &cxt->calc_results.ae_ev, sizeof(struct ae_get_ev));
	memcpy(&calc_result->monitor_info, &cxt->calc_results.monitor_info, sizeof(struct ae_monitor_info));
	memcpy(&calc_result->flash_param, &cxt->calc_results.flash_param, sizeof(struct ae_flash_param));

	return rtn;
}

static void ae_binning_for_aem_statsv2(struct ae_ctrl_cxt *cxt, struct ae_calc_in *aem_stat_ptr)
{
	cmr_u64 sum_oe = 0,sum_me = 0,sum_ue = 0;
	cmr_u64 num_oe = 0,num_me = 0,num_ue = 0;
	cmr_u32 avg_r = 0,avg_g = 0,avg_b = 0;
	cmr_u32 tmp_r = 0,tmp_g = 0,tmp_b = 0;
	cmr_u32 i = 0,j = 0;
	cmr_u32 ii = 0,jj = 0;
	cmr_u32 blk_num_w = cxt->monitor_cfg.blk_num.w;//64
	cmr_u32 blk_num_h = cxt->monitor_cfg.blk_num.h;
	//static cmr_u32 sync_aem_new[3 * 128 * 128];
	memset(&cxt->sync_aem[0],0,sizeof(cxt->sync_aem));
	cmr_u32 ratio_h = blk_num_h/BLK_NUM_W_ALG;///2
	cmr_u32 ratio_w = blk_num_w/BLK_NUM_W_ALG;
	cmr_u32 bayer_chnl = cxt->monitor_cfg.blk_size.w * cxt->monitor_cfg.blk_size.h/4;
	cmr_u32 bayer_chnl_g = cxt->monitor_cfg.blk_size.w * cxt->monitor_cfg.blk_size.h/2;
	
	cxt->cur_status.adv_param.stats_data_adv.size.h = cxt->monitor_cfg.blk_num.h;
	cxt->cur_status.adv_param.stats_data_adv.size.w = cxt->monitor_cfg.blk_num.w;
	cxt->cur_status.adv_param.stats_data_adv.blk_size.w = cxt->monitor_cfg.blk_size.w;
	cxt->cur_status.adv_param.stats_data_adv.blk_size.h = cxt->monitor_cfg.blk_size.h;

	//cxt->cur_status.stats_data_basicv2.stat_data = &sync_aem_new[0];
	cxt->cur_status.stats_data_basic.stat_data = &cxt->sync_aem[0];
	cxt->cur_status.stats_data_basic.counts_per_pixel = 1;//bayer_pixels;
	cxt->cur_status.stats_data_basic.size.h = 32;
	cxt->cur_status.stats_data_basic.size.w = 32;
	cxt->cur_status.stats_data_basic.shift = 0;
	cxt->cur_status.stats_data_basic.blk_size.w = ratio_w * cxt->monitor_cfg.blk_size.w;
	cxt->cur_status.stats_data_basic.blk_size.h = ratio_h * cxt->monitor_cfg.blk_size.h;

	for(i = 0; i < BLK_NUM_W_ALG; i++){
		for(j = 0; j < BLK_NUM_W_ALG; j++){
			for(ii = 0; ii < ratio_w; ii++){
				for(jj = 0; jj < ratio_h; jj++){
					cmr_u32 idx = i * ratio_w * ratio_h * BLK_NUM_W_ALG + j * ratio_w + ii * ratio_h * BLK_NUM_W_ALG + jj;//if 64*64:(0,1,64,65;2,3,66,67;.......)
					//r channel
						sum_oe = aem_stat_ptr->sum_oe_r[idx];
						sum_ue = aem_stat_ptr->sum_ue_r[idx];
						sum_me = aem_stat_ptr->sum_ae_r[idx];
						num_oe = aem_stat_ptr->cnt_oe_r[idx];
						num_ue = aem_stat_ptr->cnt_ue_r[idx];
						num_me = bayer_chnl - num_oe - num_ue;
						avg_r = (sum_oe + sum_ue + sum_me)/ bayer_chnl;
						tmp_r += avg_r/(ratio_w * ratio_h);//binning is average of adjacent pixels
						
						cxt->cur_status.adv_param.stats_data_adv.stats_data[0][idx].oe_stats_data = sum_oe;
						cxt->cur_status.adv_param.stats_data_adv.stats_data[0][idx].med_stats_data = sum_me;
						cxt->cur_status.adv_param.stats_data_adv.stats_data[0][idx].ue_stats_data = sum_ue;
						cxt->cur_status.adv_param.stats_data_adv.stats_data[0][idx].oe_conts = num_oe;
						cxt->cur_status.adv_param.stats_data_adv.stats_data[0][idx].med_conts = num_me;
						cxt->cur_status.adv_param.stats_data_adv.stats_data[0][idx].ue_conts = num_ue;

					//g channel
						sum_oe = aem_stat_ptr->sum_oe_g[idx];
						sum_ue = aem_stat_ptr->sum_ue_g[idx];
						sum_me = aem_stat_ptr->sum_ae_g[idx];
						num_oe = aem_stat_ptr->cnt_oe_g[idx];
						num_ue = aem_stat_ptr->cnt_ue_g[idx];
						num_me = bayer_chnl_g - num_oe - num_ue;
						avg_g = (sum_oe + sum_ue + sum_me)/ bayer_chnl_g;
						tmp_g += avg_g/(ratio_w * ratio_h);//binning is average of adjacent pixels
						
						cxt->cur_status.adv_param.stats_data_adv.stats_data[1][idx].oe_stats_data = sum_oe;
						cxt->cur_status.adv_param.stats_data_adv.stats_data[1][idx].med_stats_data = sum_me;
						cxt->cur_status.adv_param.stats_data_adv.stats_data[1][idx].ue_stats_data = sum_ue;
						cxt->cur_status.adv_param.stats_data_adv.stats_data[1][idx].oe_conts = num_oe;
						cxt->cur_status.adv_param.stats_data_adv.stats_data[1][idx].med_conts = num_me;
						cxt->cur_status.adv_param.stats_data_adv.stats_data[1][idx].ue_conts = num_ue;

					//b channel
						sum_oe = aem_stat_ptr->sum_oe_b[idx];
						sum_ue = aem_stat_ptr->sum_ue_b[idx];
						sum_me = aem_stat_ptr->sum_ae_b[idx];
						num_oe = aem_stat_ptr->cnt_oe_b[idx];
						num_ue = aem_stat_ptr->cnt_ue_b[idx];
						num_me = bayer_chnl - num_oe - num_ue;
						avg_b = (sum_oe + sum_ue + sum_me)/ bayer_chnl;
						tmp_b += avg_b/(ratio_w * ratio_h);//binning is average of adjacent pixels
						
						cxt->cur_status.adv_param.stats_data_adv.stats_data[2][idx].oe_stats_data = sum_oe;
						cxt->cur_status.adv_param.stats_data_adv.stats_data[2][idx].med_stats_data = sum_me;
						cxt->cur_status.adv_param.stats_data_adv.stats_data[2][idx].ue_stats_data = sum_ue;
						cxt->cur_status.adv_param.stats_data_adv.stats_data[2][idx].oe_conts = num_oe;
						cxt->cur_status.adv_param.stats_data_adv.stats_data[2][idx].med_conts = num_me;
						cxt->cur_status.adv_param.stats_data_adv.stats_data[2][idx].ue_conts = num_ue;
				}
			}
			
			cxt->sync_aem[i * BLK_NUM_W_ALG + j] = tmp_r;
			cxt->sync_aem[i * BLK_NUM_W_ALG + j + 1024] = tmp_g;
			cxt->sync_aem[i * BLK_NUM_W_ALG + j + 2048] = tmp_b;
			tmp_r = tmp_g = tmp_b = 0;
		}
	}
#if 0
	cmr_u32 idx = 0;
	num_me = 0;
	for(i = 0; i < BLK_NUM_W_ALG; i++){
		for(j = 0; j < BLK_NUM_W_ALG; j++){
			sum_oe = aem_stat_ptr->sum_oe_r[idx];
			sum_ue = aem_stat_ptr->sum_ue_r[idx];
			sum_me = aem_stat_ptr->sum_ae_r[idx];
			num_oe = aem_stat_ptr->cnt_oe_r[idx];
			num_ue = aem_stat_ptr->cnt_ue_r[idx];
			tmp_r = (sum_oe + sum_ue + sum_me)/ bayer_chnl;


			sum_oe = aem_stat_ptr->sum_oe_g[idx];
			sum_ue = aem_stat_ptr->sum_ue_g[idx];
			sum_me = aem_stat_ptr->sum_ae_g[idx];
			num_oe = aem_stat_ptr->cnt_oe_g[idx];
			num_ue = aem_stat_ptr->cnt_ue_g[idx];
			tmp_g = (sum_oe + sum_ue + sum_me)/ bayer_chnl_g;

			sum_oe = aem_stat_ptr->sum_oe_b[idx];
			sum_ue = aem_stat_ptr->sum_ue_b[idx];
			sum_me = aem_stat_ptr->sum_ae_b[idx];
			num_oe = aem_stat_ptr->cnt_oe_b[idx];
			num_ue = aem_stat_ptr->cnt_ue_b[idx];
			tmp_b = (sum_oe + sum_ue + sum_me)/ bayer_chnl;
			
			sync_aem_new[idx] = tmp_r;
			sync_aem_new[idx + 64 * 64] = tmp_g;
			sync_aem_new[idx + 64 * 64 * 2] = tmp_b;
			idx++;
		}
	}
#endif
#if 0
	 {
		char fname11[256];
		char dir_str11[256] ="/data/vendor/cameraserver/";
		sprintf(fname11, "%saem_new_binning_avg4RGB.txt",dir_str11);
		FILE* fp11 = fopen(fname11, "w");
		if (fp11){
			for (int ii = 0; ii < 64; ii++){
				for (int jj = 0; jj < 64; jj++ ){
					fprintf(fp11,"%d,%d,%d  ",cxt->cur_status.stats_data_basicv2.stat_data[ii * 32 + jj],
						cxt->cur_status.stats_data_basicv2.stat_data[ii * 32 + jj + 32*32],
						cxt->cur_status.stats_data_basicv2.stat_data[ii * 32 + jj + 2 * 32*32] );
					//fprintf(fp11,"%d,%d,%d  ",sync_aem_new[ii * 64 + jj],
					//	sync_aem_new[ii * 64 + jj + 64*64],
					//	sync_aem_new[ii * 64 + jj + 2 * 64*64]);
				}
					fprintf(fp11,"\n");
			}
		}
		fclose(fp11);
	}
#endif
#if 0
{
		char fname22[256];
		char dir_str22[256] ="/data/vendor/cameraserver/";
		sprintf(fname22, "%saem_new_sum.txt",dir_str22);
		FILE* fp22 = fopen(fname22, "w");
		if (fp22){
			for (int ii = 0; ii < 64; ii++){
				for (int jj = 0; jj < 64; jj++ ){
				fprintf(fp22,"%d,%d,%d,%d,%d,%d,%d,%d,%d     ",
					cxt->cur_status.adv_param.stats_data_adv.stats_data[0][ii * 64 + jj].oe_stats_data,
					cxt->cur_status.adv_param.stats_data_adv.stats_data[0][ii * 64 + jj].ue_stats_data,
					cxt->cur_status.adv_param.stats_data_adv.stats_data[0][ii * 64 + jj].med_stats_data,
					cxt->cur_status.adv_param.stats_data_adv.stats_data[1][ii * 64 + jj].oe_stats_data,
					cxt->cur_status.adv_param.stats_data_adv.stats_data[1][ii * 64 + jj].ue_stats_data,
					cxt->cur_status.adv_param.stats_data_adv.stats_data[1][ii * 64 + jj].med_stats_data,
					cxt->cur_status.adv_param.stats_data_adv.stats_data[2][ii * 64 + jj].oe_stats_data,
					cxt->cur_status.adv_param.stats_data_adv.stats_data[2][ii * 64 + jj].ue_stats_data,
					cxt->cur_status.adv_param.stats_data_adv.stats_data[2][ii * 64 + jj].med_stats_data);
				}
					fprintf(fp22,"\n");
			}
		}
		fclose(fp22);
	}
#endif
}


static void ae_binning_for_aem_stats(struct ae_ctrl_cxt *cxt, void * img_stat)
{
	cmr_u32 i,j,ii,jj = 0;
	//cmr_u64 r = 0, g = 0, b = 0;
	cmr_u64 avg;
	cmr_u32 tmp_r = 0,tmp_g = 0,tmp_b = 0;
	cmr_u32 blk_num_w = cxt->monitor_cfg.blk_num.w;
	cmr_u32 blk_num_h = cxt->monitor_cfg.blk_num.h;
	cmr_u32 bayer_pixels = cxt->monitor_cfg.blk_size.w * cxt->monitor_cfg.blk_size.h/4;
	cmr_u32 *src_aem_stat = (cmr_u32 *) img_stat;
	cmr_u32 *r_stat = (cmr_u32*)src_aem_stat;
	cmr_u32 *g_stat = (cmr_u32*)src_aem_stat + 16384;
	cmr_u32 *b_stat = (cmr_u32*)src_aem_stat + 2 * 16384;

	cmr_u32 ratio_h = blk_num_h / BLK_NUM_W_ALG;
	cmr_u32 ratio_w = blk_num_w / BLK_NUM_W_ALG;
	memset(&cxt->sync_aem[0],0,sizeof(cxt->sync_aem));
	
	//cxt->cur_status.adv_param.stats_data_high.stat_data = img_stat;
	cxt->cur_status.adv_param.stats_data_high.stat_data = &cxt->sync_aem_high[0];
	cxt->cur_status.adv_param.stats_data_high.counts_per_pixel = 1;//bayer_pixels;
	cxt->cur_status.adv_param.stats_data_high.size.h = blk_num_h;
	cxt->cur_status.adv_param.stats_data_high.size.w = blk_num_w;
	cxt->cur_status.adv_param.stats_data_high.blk_size.w = cxt->monitor_cfg.blk_size.w;
	cxt->cur_status.adv_param.stats_data_high.blk_size.h = cxt->monitor_cfg.blk_size.h;

	cxt->cur_status.stats_data_basic.stat_data = &cxt->sync_aem[0];
	cxt->cur_status.stats_data_basic.counts_per_pixel = 1;//ratio_h * ratio_w * bayer_pixels;
	cxt->cur_status.stats_data_basic.size.h = 32;
	cxt->cur_status.stats_data_basic.size.w = 32;
	cxt->cur_status.stats_data_basic.shift = 0;
	cxt->cur_status.stats_data_basic.blk_size.w = ratio_w * cxt->monitor_cfg.blk_size.w;
	cxt->cur_status.stats_data_basic.blk_size.h = ratio_h * cxt->monitor_cfg.blk_size.h;

	for(i = 0; i < BLK_NUM_W_ALG; i++){
		for(j = 0; j < BLK_NUM_W_ALG; j++){
			for(ii = 0; ii < ratio_w; ii++){
				for(jj = 0; jj < ratio_h; jj++){
					cmr_u32 idx = i * ratio_w * ratio_h * BLK_NUM_W_ALG + j * ratio_w + ii * ratio_h * BLK_NUM_W_ALG + jj;
					avg = r_stat[idx]/bayer_pixels;
					tmp_r += avg/(ratio_w * ratio_h);
					cxt->cur_status.adv_param.stats_data_high.stat_data[idx] = avg;

					avg = g_stat[idx]/bayer_pixels;
					tmp_g += avg/(ratio_w * ratio_h);
					cxt->cur_status.adv_param.stats_data_high.stat_data[idx + blk_num_w * blk_num_h] = avg;

					avg = b_stat[idx]/bayer_pixels;
					tmp_b += avg/(ratio_w * ratio_h);
					cxt->cur_status.adv_param.stats_data_high.stat_data[idx+ 2 * blk_num_w * blk_num_h] = avg;
				}
			}
			cxt->sync_aem[i * BLK_NUM_W_ALG + j] = tmp_r;
			cxt->sync_aem[i * BLK_NUM_W_ALG + j + 1024] = tmp_g;
			cxt->sync_aem[i * BLK_NUM_W_ALG + j + 2048] = tmp_b;
			tmp_r = tmp_g = tmp_b = 0;
		}
	}
#if 0
 	{
		char fname11[256];
		char dir_str11[256] ="/data/vendor/cameraserver/";
		sprintf(fname11, "%saem_old_sum.txt",dir_str11);
		FILE* fp11 = fopen(fname11, "w");
		if (fp11){
			for (int ii = 0; ii < 64; ii++){
				for (int jj = 0; jj < 64; jj++ ){

				fprintf(fp11,"%d,%d,%d  ",r_stat[ii * 64 + jj],
						g_stat[ii * 64 + jj],
						b_stat[ii * 64 + jj] );


	/*				fprintf(fp11,"%d,%d,%d  ",cxt->cur_status.adv_param.stats_data_high.stat_data[ii * 64 + jj],
						cxt->cur_status.adv_param.stats_data_high.stat_data[ii * 64 + jj + 64*64],
						cxt->cur_status.adv_param.stats_data_high.stat_data[ii * 64 + jj + 2 * 64*64] );*/
					//fprintf(fp11,"%d,%d,%d  ",cxt->cur_status.stats_data_basic.stat_data[ii * 32 + jj],
					//	cxt->cur_status.adv_param.stats_data_high.stat_data[ii * 32 + jj + 32*32],
					//	cxt->cur_status.adv_param.stats_data_high.stat_data[ii * 32 + jj + 2 * 32*32] );
				}
					fprintf(fp11,"\n");
			}
		}
		fclose(fp11);
	}
#endif
}

static cmr_s32 ae_calculation_slow_motion(cmr_handle handle, cmr_handle param, cmr_handle result)
{
	cmr_s32 rtn = AE_ERROR;
	cmr_int cb_type;
	//cmr_u32 stat_chnl_len = 0;
	struct ae_ctrl_cxt *cxt = NULL;
	struct ae_lib_calc_in *current_status;
	struct ae_lib_calc_out *current_result;
	struct ae_lib_calc_in *misc_calc_in = NULL;
	struct ae_lib_calc_out *misc_calc_out = NULL;
	struct ae_calc_in *calc_in = NULL;
	struct ae_calc_results_3_x *cur_calc_result = NULL;
	struct ae_ctrl_callback_in callback_in;
	cmr_s32 backup_expline = 0;
	cmr_s32 backup_gain = 0;
	cmr_s32 backup_expgain = 0;
	cmr_s32 effect_ebd_expgain = 0;
	UNUSED(result);
	cmr_s32 aem_type = 0;

	if (NULL == param) {
		ISP_LOGE("fail to get param, in %p", param);
		return AE_PARAM_NULL;
	}

	rtn = ae_check_handle(handle);
	if (AE_SUCCESS != rtn) {
		ISP_LOGE("fail to check handle, ret: %d\n", rtn);
		return AE_HANDLER_NULL;
	}

	cxt = (struct ae_ctrl_cxt *)handle;

	current_status = &cxt->sync_cur_status;
	current_result = &cxt->sync_cur_result;
	cur_calc_result = &cxt->calc_results;

	calc_in = (struct ae_calc_in *)param;
	aem_type = cxt->monitor_cfg.data_type;
	// acc_info_print(cxt);
	cxt->cur_status.awb_gain.b = calc_in->awb_gain_b;
	cxt->cur_status.awb_gain.g = calc_in->awb_gain_g;
	cxt->cur_status.awb_gain.r = calc_in->awb_gain_r;
	cxt->cur_status.adv_param.awb_mode = calc_in->awb_mode;

	/*get bayserhist when not ISP_ALG_TRIBLE_W_T_UW mode*/
	if (cxt->is_multi_mode < ISP_ALG_TRIBLE_W_T_UW) {
		memcpy(&cxt->cur_status.adv_param.bhist_data[0].hist_data, &calc_in->bayerhist_stats[0].value, 256 * sizeof(cmr_u32));
		memcpy(&cxt->cur_status.adv_param.bhist_data[1].hist_data, &calc_in->bayerhist_stats[1].value, 256 * sizeof(cmr_u32));
		memcpy(&cxt->cur_status.adv_param.bhist_data[2].hist_data, &calc_in->bayerhist_stats[2].value, 256 * sizeof(cmr_u32));
		cxt->cur_status.adv_param.bhist_data[0].hist_bin = calc_in->bayerhist_stats[0].bin;
		cxt->cur_status.adv_param.bhist_data[1].hist_bin = calc_in->bayerhist_stats[1].bin;
		cxt->cur_status.adv_param.bhist_data[2].hist_bin = calc_in->bayerhist_stats[2].bin;
	}
	
	if(0 == aem_type){
		ae_binning_for_aem_stats(cxt, calc_in->stat_img);
		cxt->cur_status.stats_data_basic.stat_data = cxt->sync_aem;
	}else{
		ae_binning_for_aem_statsv2(cxt, calc_in);
		cxt->cur_status.stats_data_basic.stat_data = cxt->sync_aem;
	}

	// get effective E&g
	cxt->cur_status.adv_param.cur_ev_setting.ae_idx = cxt->exp_data.actual_data.cur_index;
	cxt->cur_status.adv_param.cur_ev_setting.frm_len = cxt->exp_data.actual_data.frm_len;
	cxt->cur_status.adv_param.cur_ev_setting.exp_line = cxt->exp_data.actual_data.exp_line;
	cxt->cur_status.adv_param.cur_ev_setting.exp_time = cxt->exp_data.actual_data.exp_time;
	cxt->cur_status.adv_param.cur_ev_setting.dmy_line = cxt->exp_data.actual_data.dummy;
	cxt->cur_status.adv_param.cur_ev_setting.ae_gain = (cmr_s32) (1.0 * cxt->exp_data.actual_data.isp_gain * cxt->exp_data.actual_data.sensor_gain / 4096.0 + 0.5);

	backup_expline = cxt->cur_status.adv_param.cur_ev_setting.exp_line;
	backup_gain = cxt->cur_status.adv_param.cur_ev_setting.ae_gain;
	backup_expgain = backup_expline*backup_gain;
	ISP_LOGV("ebd(slow motion): ae_lib effect_expline %d, effect_gain %d", cxt->cur_status.adv_param.cur_ev_setting.exp_line, cxt->cur_status.adv_param.cur_ev_setting.ae_gain);
	if(cxt->ebd_support){
		cxt->cur_status.adv_param.cur_ev_setting.exp_line =  calc_in->ebd_info.exposure_valid ?calc_in->ebd_info.exposure : cxt->exp_data.actual_data.exp_line;
		cxt->cur_status.adv_param.cur_ev_setting.ae_gain = (cmr_u32)(1.0 *calc_in->ebd_info.gain * calc_in->isp_dgain.global_gain/(4096.0 *cxt->ob_rgb_gain)+ 0.5);
		effect_ebd_expgain = cxt->cur_status.adv_param.cur_ev_setting.exp_line * cxt->cur_status.adv_param.cur_ev_setting.ae_gain;
		ISP_LOGV("ebd(slow motion): sensor effect_expline %d, effect_gain %d", cxt->cur_status.adv_param.cur_ev_setting.exp_line, cxt->cur_status.adv_param.cur_ev_setting.ae_gain);

		if((1.0 * backup_expgain / effect_ebd_expgain >= 0.97) && (1.0 * backup_expgain / effect_ebd_expgain <= 1.03))
			cxt->ebd_stable_flag = 1;
		else
			cxt->ebd_stable_flag = 0;
		ISP_LOGV("ebd(slow motion): stable_flag %d", cxt->ebd_stable_flag);
	}

	cxt->sync_aem[3 * 1024] = cxt->cur_status.frm_id;
	cxt->sync_aem[3 * 1024 + 1] = cxt->cur_status.adv_param.cur_ev_setting.exp_line;
	cxt->sync_aem[3 * 1024 + 2] = cxt->cur_status.adv_param.cur_ev_setting.dmy_line;
	cxt->sync_aem[3 * 1024 + 3] = cxt->cur_status.adv_param.cur_ev_setting.ae_gain;
	memcpy(current_status, &cxt->cur_status, sizeof(struct ae_lib_calc_in));
	memcpy(&cxt->cur_result, current_result, sizeof(struct ae_lib_calc_out));

	misc_calc_in = current_status;
	misc_calc_out = &cxt->cur_result;
	cxt->sync_cur_status.adv_param.fps_range.min = cxt->high_fps_info.min_fps;
	cxt->sync_cur_status.adv_param.fps_range.max = cxt->high_fps_info.max_fps;
	cmr_u64 ae_time0 = systemTime(CLOCK_MONOTONIC);
	rtn = ae_lib_calculation(cxt->misc_handle, misc_calc_in, misc_calc_out);
	cmr_u64 ae_time1 = systemTime(CLOCK_MONOTONIC);
	ISP_LOGV("SYSTEM_TEST -ae	%dus ", (cmr_s32) ((ae_time1 - ae_time0) / 1000));

	memcpy(current_result, &cxt->cur_result, sizeof(struct ae_lib_calc_out));
	memcpy(&cur_calc_result->ae_result, current_result, sizeof(struct ae_lib_calc_out));
	ae_make_calc_result(cxt, current_result, cur_calc_result);

	/*just for debug: reset the status */
	if (1 == cxt->cur_status.adv_param.touch_roi_flag) {
		cxt->cur_status.adv_param.touch_roi_flag = 0;
	}

	cxt->exp_data.lib_data.exp_line = current_result->ev_setting.exp_line;
	cxt->exp_data.lib_data.gain = current_result->ev_setting.ae_gain;
	cxt->exp_data.lib_data.dummy = current_result->ev_setting.dmy_line;
	cxt->exp_data.lib_data.line_time = current_status->adv_param.cur_ev_setting.line_time;
	cxt->exp_data.lib_data.exp_time = current_result->ev_setting.exp_time;
	cxt->exp_data.lib_data.frm_len = current_result->ev_setting.frm_len;

	ae_update_result_to_sensor(cxt, &cxt->exp_data, 0);

/***********************************************************/
/* send STAB notify to HAL */
	if (cxt->isp_ops.callback) {
		cb_type = AE_CB_STAB_NOTIFY;
		cxt->ae_cb_result[AE_CB_RESULT_STAB] = cur_calc_result->ae_output.is_stab;
		cxt->ae_cb_result[AE_CB_RESULT_BV_VALUE] = cur_calc_result->ae_output.cur_bv;

		(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, cb_type, &cxt->ae_cb_result);
		ISP_LOGV("normal notify stable_flag %d", cur_calc_result->ae_output.is_stab);
	}

	if (1 == cxt->debug_enable) {
		ae_save_to_mlog_file(cxt, misc_calc_out);
	}

	if (cxt->isp_ops.callback) {
		ae_make_isp_result(cxt, current_result, &callback_in);
		(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, AE_CB_PROCESS_OUT, &callback_in);
	}
	cxt->cur_status.frm_id++;

  ERROR_EXIT:
	return rtn;
}

static void ae_calculation_debug(struct ae_ctrl_cxt *cxt, struct ae_calc_in *calc_in)
{
	{
	uint32_t n = 0;
	uint32_t i = 0;
	uint32_t j = 0;
	char filename[256];
	char dir_str[256] ="/data/vendor/cameraserver/";
	uint32_t ww = cxt->monitor_cfg.blk_num.w;
	uint32_t hh = cxt->monitor_cfg.blk_num.h;
	ISP_LOGD("w and h %d,%d\n",ww,hh);
	sprintf(filename, "%smw-aem_sum_new.txt",dir_str);
	FILE* fp = fopen(filename, "w");

	if (fp){
		//for (n = 0;n < 3; n++){
			n = 0;
			for (j = 0; j < 64; j++){
				for (i = 0; i < 64; i++ ){
					fprintf(fp,"%d,%d,%d,%d,%d,%d,%d,%d,%d	   ",
						calc_in->sum_ue_r[n * ww * hh + j * ww + i],calc_in->sum_ae_r[n * ww * hh + j * ww + i],calc_in->sum_oe_r[n * ww * hh + j * ww + i],
						calc_in->sum_ue_g[n * ww * hh + j * ww + i],calc_in->sum_ae_g[n * ww * hh + j * ww + i],calc_in->sum_oe_g[n * ww * hh + j * ww + i],
						calc_in->sum_ue_b[n * ww * hh + j * ww + i],calc_in->sum_ae_b[n * ww * hh + j * ww + i],calc_in->sum_oe_b[n * ww * hh + j * ww + i]);
				}
				fprintf(fp,"\n");
			}
		//}
	}
	fclose(fp);
	}

	{
		uint32_t nn = 0;
		uint32_t ii = 0;
		uint32_t jj = 0;
		char filename33[256];
		char dir_str33[256] ="/data/vendor/cameraserver/";
		uint32_t ww = cxt->monitor_cfg.blk_num.w;
		uint32_t hh = cxt->monitor_cfg.blk_num.h;
		ISP_LOGD("w and h %d,%d\n",ww,hh);
		sprintf(filename33, "%smw-aem_new_num.txt",dir_str33);
		FILE* fp33 = fopen(filename33, "w");

		if (fp33){
			for (nn = 0;nn < 3; nn++){
				for (jj = 0; jj < ww; jj++){
					for (ii = 0; ii < hh; ii++ ){
						fprintf(fp33,"%d,%d,%d,%d,%d,%d 	",
							calc_in->cnt_oe_r[nn * ww * hh + jj * ww + ii],calc_in->cnt_ue_r[nn * ww * hh + jj * ww + ii],
							calc_in->cnt_oe_g[nn * ww * hh + jj * ww + ii],calc_in->cnt_ue_g[nn * ww * hh + jj * ww + ii],
							calc_in->cnt_oe_b[nn * ww * hh + jj * ww + ii],calc_in->cnt_ue_b[nn * ww * hh + jj * ww + ii]);
					}
					fprintf(fp33,"\n");
				}
			}
		}
		fclose(fp33);
	}

	{
	char fname[256];
	char dir_str1[256] ="/data/vendor/cameraserver/";
	sprintf(fname, "%smw-aem_sum_old.txt",dir_str1);
	FILE* fp0 = fopen(fname, "w");
	if (fp0){
		for (int ii = 0; ii < 128; ii++){
			for (int jj = 0; jj < 128; jj++ ){
				fprintf(fp0,"%d,%d,%d  ",calc_in->stat_img[ii * 128 + jj],calc_in->stat_img[ii * 128 + jj + 128*128],calc_in->stat_img[ii * 128 + jj + 2 * 128*128] );
			}
				fprintf(fp0,"\n");
		}
	}
	fclose(fp0);
	}
}

static cmr_u32 ae_load_fdr_lib(struct ae_ctrl_cxt *cxt)
{
	cmr_s32 rtn = AE_SUCCESS;
	cxt->fdr_lib_handle = dlopen("libsprdfdr.so", RTLD_NOW);
	if (!cxt->fdr_lib_handle) {
		ISP_LOGE("fail to dlopen fdr lib");
		rtn = AE_ERROR;
		goto error_dlopen;
	}
	cxt->fdr_ops.fdr_scndet = dlsym(cxt->fdr_lib_handle, "sprd_fdr_scndet");
	if (!cxt->fdr_ops.fdr_scndet) {
		ISP_LOGE("fail to dlsym sprd_fdr_scndet");
		goto error_dlsym;
	}else
		ISP_LOGI("FDR dlsym ok");
	return rtn;
	error_dlsym:
	dlclose(cxt->fdr_lib_handle);
	cxt->fdr_lib_handle = NULL;

	error_dlopen:
	return rtn;
}

static cmr_s32 ae_calculation(cmr_handle handle, cmr_handle param, cmr_handle result)
{
	cmr_s32 rtn = AE_ERROR;
	cmr_int cb_type;
	//cmr_s32 stop_skip_en;
	//cmr_u32 stat_chnl_len = 0;
	struct ae_ctrl_cxt *cxt = NULL;
	struct ae_lib_calc_in *current_status = NULL;
	struct ae_lib_calc_out *current_result = NULL;
	struct ae_lib_calc_in *misc_calc_in = NULL;
	struct ae_lib_calc_out *misc_calc_out = NULL;
	struct ae_calc_in *calc_in = NULL;
	struct ae_calc_results_3_x *cur_calc_result = NULL;
	struct ae_ctrl_callback_in callback_in;
	cmr_s32 backup_expline = 0;
	cmr_s32 backup_gain = 0;
	cmr_s32 backup_expgain = 0;
	cmr_s32 effect_ebd_expgain = 0;
	UNUSED(result);
	cmr_s32 aem_type = 0;
	//cmr_s16 bv = 0;
	cmr_u32 sensor_num = 0;//aem static data ready sensor num
#ifdef CONFIG_SUPPROT_AUTO_HDR
	struct _tag_hdr_detect_t hdr_param;
	struct _tag_hdr_stat_t hdr_stat;
	float ev_result[2] = {0,0};
	cmr_s8 auto_hdr_enable = 0;
#endif
	struct ae_sync_data aem_sync_info = {0};
	struct ae_ctrl_visible_region_info zoom_roi = {0};
	struct ae_ctrl_visible_region_info ae_debug_zoom_roi[3] = {{0},{0},{0}};
	struct ae_ctrl_win_info ae_win_info = {0};
	struct ae_ctrl_hist_win_info hist_win_info = {0};
	struct ae_ctrl_hist_win_info ae_debug_hist_win_info[3] = {{0},{0},{0}};
	float zoom_ratio = 0;

	cmr_u32 ae_debug_framId[3] = {0};
	cmr_u32 i,j,k = 0;
	struct isp_hist_statistic_info ae_debug_m_get_bayerhist_stats[3];
	struct isp_hist_statistic_info ae_debug_s0_get_bayerhist_stats[3];
	struct isp_hist_statistic_info ae_debug_s1_get_bayerhist_stats[3];

	if (NULL == param) {
		ISP_LOGE("fail to get param, in %p", param);
		return AE_PARAM_NULL;
	}

	rtn = ae_check_handle(handle);
	if (AE_SUCCESS != rtn) {
		ISP_LOGE("fail to check handle, ret: %d\n", rtn);
		return AE_HANDLER_NULL;
	}
	cxt = (struct ae_ctrl_cxt *)handle;
	cur_calc_result = &cxt->calc_results;
	ae_set_restore_skip_update_cnt(cxt);

	if (cxt->bypass) {
		ae_set_pause(cxt,2);
	}

	calc_in = (struct ae_calc_in *)param;

	aem_type = cxt->monitor_cfg.data_type;
	ISP_LOGV("is_multi_mode:%d, cameraId:%d, sensor_role:%d, aem type is:%d",cxt->is_multi_mode, cxt->camera_id, cxt->sensor_role, aem_type);
	// acc_info_print(cxt);
	cxt->cur_status.awb_gain.b = calc_in->awb_gain_b;
	cxt->cur_status.awb_gain.g = calc_in->awb_gain_g;
	cxt->cur_status.awb_gain.r = calc_in->awb_gain_r;
	cxt->cur_status.adv_param.awb_mode = calc_in->awb_mode;

	/*get aem static data current sensor*/
	if(0 == aem_type) {
		ae_binning_for_aem_stats(cxt, calc_in->stat_img);
		cxt->cur_status.stats_data_basic.stat_data = cxt->sync_aem;
		}else{
		ae_binning_for_aem_statsv2(cxt, calc_in);
		cxt->cur_status.stats_data_basic.stat_data = cxt->sync_aem;
	}

	/*send multi_mode to lib*/
	cxt->cur_status.is_multi_mode = cxt->is_multi_mode;

	/*set frame id to bridge*/
	if (CAM_SENSOR_MASTER == cxt->sensor_role) {
		cxt->ptr_isp_br_ioctrl(CAM_SENSOR_MASTER, SET_FRAME_ID, &cxt->cur_status.frm_id, NULL);
	} else if (CAM_SENSOR_SLAVE0 == cxt->sensor_role) {
		cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE0, SET_FRAME_ID, &cxt->cur_status.frm_id, NULL);
	} else {
		cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE1, SET_FRAME_ID, &cxt->cur_status.frm_id, NULL);
	}

	/*get frame id for ae debug*/
	cxt->ptr_isp_br_ioctrl(CAM_SENSOR_MASTER, GET_FRAME_ID, NULL, &ae_debug_framId[0]);
	cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE0, GET_FRAME_ID, NULL, &ae_debug_framId[1]);
	cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE1, GET_FRAME_ID, NULL, &ae_debug_framId[2]);
	ISP_LOGV("ae_debug, frameId, m:%d, s0:%d, s1:%d", ae_debug_framId[0], ae_debug_framId[1], ae_debug_framId[2]);

	/*get bayserhist stat*/
	memcpy(&cxt->cur_status.adv_param.bhist_data[0].hist_data, &calc_in->bayerhist_stats[0].value, 256 * sizeof(cmr_u32));
	memcpy(&cxt->cur_status.adv_param.bhist_data[1].hist_data, &calc_in->bayerhist_stats[1].value, 256 * sizeof(cmr_u32));
	memcpy(&cxt->cur_status.adv_param.bhist_data[2].hist_data, &calc_in->bayerhist_stats[2].value, 256 * sizeof(cmr_u32));
	cxt->cur_status.adv_param.bhist_data[0].hist_bin = calc_in->bayerhist_stats[0].bin;
	cxt->cur_status.adv_param.bhist_data[1].hist_bin = calc_in->bayerhist_stats[1].bin;
	cxt->cur_status.adv_param.bhist_data[2].hist_bin = calc_in->bayerhist_stats[2].bin;

	if (CAM_SENSOR_MASTER == cxt->sensor_role) {
		cxt->ptr_isp_br_ioctrl(CAM_SENSOR_MASTER, SET_HIST_STATS, calc_in->bayerhist_stats, NULL);
	} else if (CAM_SENSOR_SLAVE0 == cxt->sensor_role) {
		cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE0, SET_HIST_STATS, calc_in->bayerhist_stats, NULL);
	} else {
		cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE1, SET_HIST_STATS, calc_in->bayerhist_stats, NULL);
	}

	/*add hist stat info to ae debug info*/
	cxt->ptr_isp_br_ioctrl(CAM_SENSOR_MASTER, GET_HIST_STATS, NULL, ae_debug_m_get_bayerhist_stats);
	cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE0, GET_HIST_STATS, NULL, ae_debug_s0_get_bayerhist_stats);
	cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE1, GET_HIST_STATS, NULL, ae_debug_s1_get_bayerhist_stats);

	for (k = 0; k <3 ; k++) {
		memcpy(&cxt->cur_status.debug_info.hist_info[0].hist_data[k].hist_data, &ae_debug_m_get_bayerhist_stats[k].value, 256 * sizeof(cmr_u32));
		cxt->cur_status.debug_info.hist_info[0].hist_data[k].hist_bin = ae_debug_m_get_bayerhist_stats[k].bin;
	}

	for (k = 0; k <3 ; k++) {
		memcpy(&cxt->cur_status.debug_info.hist_info[1].hist_data[k].hist_data, &ae_debug_s0_get_bayerhist_stats[k].value, 256 * sizeof(cmr_u32));
		cxt->cur_status.debug_info.hist_info[1].hist_data[k].hist_bin = ae_debug_s0_get_bayerhist_stats[k].bin;
	}

	for (k = 0; k <3 ; k++) {
		memcpy(&cxt->cur_status.debug_info.hist_info[2].hist_data[k].hist_data, &ae_debug_s1_get_bayerhist_stats[k].value, 256 * sizeof(cmr_u32));
		cxt->cur_status.debug_info.hist_info[2].hist_data[k].hist_bin = ae_debug_s1_get_bayerhist_stats[k].bin;
	}

	/*get hist roi info*/
	if (CAM_SENSOR_MASTER == cxt->sensor_role) {
		cxt->ptr_isp_br_ioctrl(CAM_SENSOR_MASTER, GET_HIST_PARAM, NULL, &hist_win_info);
	} else if (CAM_SENSOR_SLAVE0 == cxt->sensor_role) {
		cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE0, GET_HIST_PARAM, NULL, &hist_win_info);
	} else {
		cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE1, GET_HIST_PARAM, NULL, &hist_win_info);
	}
	cxt->cur_status.adv_param.hist_roi.idx = hist_win_info.idx;
	cxt->cur_status.adv_param.hist_roi.sec = hist_win_info.sec;
	cxt->cur_status.adv_param.hist_roi.usec = hist_win_info.usec;
	cxt->cur_status.adv_param.hist_roi.start_x = hist_win_info.start_x;
	cxt->cur_status.adv_param.hist_roi.start_y = hist_win_info.start_y;
	cxt->cur_status.adv_param.hist_roi.end_x = hist_win_info.start_x + hist_win_info.width;
	cxt->cur_status.adv_param.hist_roi.end_y = hist_win_info.start_y + hist_win_info.height;

	ISP_LOGV("hist roi, sensor_role:%d, cameraId:%d, idx:%d, sec:%d, usec:%d, start_x:%d, start_y:%d, end_x:%d, end_y:%d",
	cxt->sensor_role, cxt->camera_id, cxt->cur_status.adv_param.hist_roi.idx, cxt->cur_status.adv_param.hist_roi.sec, cxt->cur_status.adv_param.hist_roi.usec, \
	cxt->cur_status.adv_param.hist_roi.start_x, hist_win_info.start_y, cxt->cur_status.adv_param.hist_roi.end_x, cxt->cur_status.adv_param.hist_roi.end_y);

	/*add hist roi info to ae debug info*/
	cxt->ptr_isp_br_ioctrl(CAM_SENSOR_MASTER, GET_HIST_PARAM, NULL, &ae_debug_hist_win_info[0]);
	cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE0, GET_HIST_PARAM, NULL, &ae_debug_hist_win_info[1]);
	cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE1, GET_HIST_PARAM, NULL, &ae_debug_hist_win_info[2]);

	cxt->cur_status.debug_info.hist_info[0].cam_id = 0;
	cxt->cur_status.debug_info.hist_info[1].cam_id = 2;
	cxt->cur_status.debug_info.hist_info[2].cam_id = 3;

	for (j = 0; j < 3; j++) {
		cxt->cur_status.debug_info.hist_info[j].frame_id = ae_debug_framId[j];
		cxt->cur_status.debug_info.hist_info[j].hist_roi.start_x = ae_debug_hist_win_info[j].start_x;
		cxt->cur_status.debug_info.hist_info[j].hist_roi.start_y= ae_debug_hist_win_info[j].start_y;
		cxt->cur_status.debug_info.hist_info[j].hist_roi.end_x = ae_debug_hist_win_info[j].width + ae_debug_hist_win_info[j].start_x;
		cxt->cur_status.debug_info.hist_info[j].hist_roi.end_y = ae_debug_hist_win_info[j].height + ae_debug_hist_win_info[j].start_y;

		ISP_LOGV("ae debug, hist roi, j:%d, frameId:%d, start_x:%d, start_y:%d, end_x:%d, end_x:%d", j, cxt->cur_status.debug_info.hist_info[j].frame_id,\
		cxt->cur_status.debug_info.hist_info[j].hist_roi.start_x, cxt->cur_status.debug_info.hist_info[j].hist_roi.start_y, \
		cxt->cur_status.debug_info.hist_info[j].hist_roi.end_x, cxt->cur_status.debug_info.hist_info[j].hist_roi.end_y);
	}

	/*get aem roi info*/
	if (CAM_SENSOR_MASTER == cxt->sensor_role) {
		cxt->ptr_isp_br_ioctrl(CAM_SENSOR_MASTER, GET_AE_WIN, NULL, &ae_win_info);
	} else if (CAM_SENSOR_SLAVE0 == cxt->sensor_role) {
		cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE0, GET_AE_WIN, NULL, &ae_win_info);
	} else {
		cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE1, GET_AE_WIN, NULL, &ae_win_info);
	}

	ISP_LOGV("aem roi, sensro_role:%d, cameraId:%d, blk_num_x:%d, blk_num_y:%d, blk_size_x:%d, blk_size_y:%d",\
	cxt->sensor_role, cxt->camera_id,\
	ae_win_info.blk_num_x, ae_win_info.blk_num_y, ae_win_info.blk_size_x, ae_win_info.blk_size_y);

	cxt->cur_status.aem_roi.start_x = ae_win_info.offset_x;
	cxt->cur_status.aem_roi.start_y = ae_win_info.offset_y;
	cxt->cur_status.aem_roi.end_x = ae_win_info.offset_x + ae_win_info.blk_num_x*ae_win_info.blk_size_x;
	cxt->cur_status.aem_roi.end_y = ae_win_info.offset_y + ae_win_info.blk_num_y*ae_win_info.blk_size_y;
	cxt->cur_status.adv_param.stats_data_adv.blk_size.w = ae_win_info.blk_size_x;
	cxt->cur_status.adv_param.stats_data_adv.blk_size.h = ae_win_info.blk_size_y;

	/*get ae sync data for ISP_ALG_TRIBLE_W_T_UW mode*/
	if (cxt->is_multi_mode == ISP_ALG_TRIBLE_W_T_UW) {

		/*get zoom roi info*/
		if (CAM_SENSOR_MASTER == cxt->sensor_role) {
			cxt->ptr_isp_br_ioctrl(CAM_SENSOR_MASTER, GET_AE_VISIBLE_REGION, NULL, &zoom_roi);
		} else if (CAM_SENSOR_SLAVE0 == cxt->sensor_role) {
			cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE0, GET_AE_VISIBLE_REGION, NULL, &zoom_roi);
		} else {
			cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE1, GET_AE_VISIBLE_REGION, NULL, &zoom_roi);
		}

		ISP_LOGV("zoom roi, sensor_role:%d, cameraId:%d, start_x:%d, start_y:%d, width:%d, height:%d",\
		cxt->sensor_role, cxt->camera_id,\
		zoom_roi.region.start_x, zoom_roi.region.start_y, zoom_roi.region.width, zoom_roi.region.height);

		cxt->cur_status.zoom_roi.start_x = zoom_roi.region.start_x;
		cxt->cur_status.zoom_roi.start_y = zoom_roi.region.start_y;
		cxt->cur_status.zoom_roi.end_x = zoom_roi.region.start_x + zoom_roi.region.width;
		cxt->cur_status.zoom_roi.end_y = zoom_roi.region.start_y + zoom_roi.region.height;

		/*add zoom roi info to ae debug info*/
		cxt->ptr_isp_br_ioctrl(CAM_SENSOR_MASTER, GET_AE_VISIBLE_REGION, NULL, &ae_debug_zoom_roi[0]);
		cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE0, GET_AE_VISIBLE_REGION, NULL, &ae_debug_zoom_roi[1]);
		cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE1, GET_AE_VISIBLE_REGION, NULL, &ae_debug_zoom_roi[2]);

		cxt->cur_status.debug_info.touch_info[0].cam_id = 0;
		cxt->cur_status.debug_info.touch_info[1].cam_id = 2;
		cxt->cur_status.debug_info.touch_info[2].cam_id = 3;

		for (i = 0; i++; i < 3) {
			cxt->cur_status.debug_info.touch_info[i].frame_id = ae_debug_framId[i];
			cxt->cur_status.debug_info.touch_info[i].touch_roi.x = ae_debug_zoom_roi[i].region.start_x;
			cxt->cur_status.debug_info.touch_info[i].touch_roi.y = ae_debug_zoom_roi[i].region.start_y;
			cxt->cur_status.debug_info.touch_info[i].touch_roi.w = ae_debug_zoom_roi[i].region.width;
			cxt->cur_status.debug_info.touch_info[i].touch_roi.h = ae_debug_zoom_roi[i].region.height;
			ISP_LOGV("ae_debug, touch info, i:%d, frameId:%d, start_x:%d, start_y:%d, width:%d, height:%d", i, cxt->cur_status.debug_info.touch_info[i].frame_id,\
			cxt->cur_status.debug_info.touch_info[i].touch_roi.x, cxt->cur_status.debug_info.touch_info[i].touch_roi.y, cxt->cur_status.debug_info.touch_info[i].touch_roi.w, cxt->cur_status.debug_info.touch_info[i].touch_roi.h);
		}

		/*get zoom ratio*/
		if (CAM_SENSOR_MASTER == cxt->sensor_role) {
			cxt->ptr_isp_br_ioctrl(CAM_SENSOR_MASTER, GET_GLOBAL_ZOOM_RATIO, NULL, &zoom_ratio);
		} else if (CAM_SENSOR_SLAVE0 == cxt->sensor_role) {
			cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE0, GET_GLOBAL_ZOOM_RATIO, NULL, &zoom_ratio);
		} else {
			cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE1, GET_GLOBAL_ZOOM_RATIO, NULL, &zoom_ratio);
		}

		ISP_LOGV("zoom_ratio, zoom_ratio:%f\n", zoom_ratio);
		cxt->cur_status.zoom_ratio = zoom_ratio;

		/*get ref cameraId*/
		cxt->ptr_isp_br_ioctrl(CAM_SENSOR_MASTER, GET_AE_SYNC_DATA, NULL, &aem_sync_info);
		cxt->cur_status.ref_camera_id = aem_sync_info.ref_camera_id;
		ISP_LOGV("ref_camera_id, sensor_role:%d, cameraId:%d, ref_cameraId:%d", cxt->sensor_role, cxt->camera_id, aem_sync_info.ref_camera_id);

	}

	#if 0
	ae_calculation_debug(cxt, calc_in);
	#endif

	// get effective E&g
	cxt->cur_status.adv_param.cur_ev_setting.ae_idx = cxt->exp_data.actual_data.cur_index;
	cxt->cur_status.adv_param.cur_ev_setting.frm_len = cxt->exp_data.actual_data.frm_len;
	cxt->cur_status.adv_param.cur_ev_setting.exp_line = cxt->exp_data.actual_data.exp_line;
	cxt->cur_status.adv_param.cur_ev_setting.exp_time = cxt->exp_data.actual_data.exp_time;
	cxt->cur_status.adv_param.cur_ev_setting.dmy_line = cxt->exp_data.actual_data.dummy;
	cxt->cur_status.adv_param.cur_ev_setting.ae_gain = (cmr_s32) (1.0 * cxt->exp_data.actual_data.isp_gain * cxt->exp_data.actual_data.sensor_gain / 4096.0 + 0.5);

	backup_expline = cxt->cur_status.adv_param.cur_ev_setting.exp_line;
	backup_gain = cxt->cur_status.adv_param.cur_ev_setting.ae_gain;
	backup_expgain = backup_expline*backup_gain;
	ISP_LOGV("ebd: ae_lib effect_expline %d, effect_gain %d(%d, %d)",
			cxt->cur_status.adv_param.cur_ev_setting.exp_line,
			cxt->cur_status.adv_param.cur_ev_setting.ae_gain,
			cxt->exp_data.actual_data.isp_gain,
			cxt->exp_data.actual_data.sensor_gain);

	if(cxt->ebd_support){
		cxt->cur_status.adv_param.cur_ev_setting.exp_line =  calc_in->ebd_info.exposure_valid ?calc_in->ebd_info.exposure : cxt->exp_data.actual_data.exp_line;
		cxt->cur_status.adv_param.cur_ev_setting.ae_gain = (cmr_u32)(1.0 *calc_in->ebd_info.gain * calc_in->isp_dgain.global_gain/(4096.0 *cxt->ob_rgb_gain)+ 0.5);

		effect_ebd_expgain = cxt->cur_status.adv_param.cur_ev_setting.exp_line * cxt->cur_status.adv_param.cur_ev_setting.ae_gain;
		ISP_LOGV("ebd: sensor effect_expline %d, effect_gain %d", cxt->cur_status.adv_param.cur_ev_setting.exp_line, cxt->cur_status.adv_param.cur_ev_setting.ae_gain);
	}

	cxt->sync_aem[3 * 1024] = cxt->cur_status.frm_id;
	cxt->sync_aem[3 * 1024 + 1] = cxt->cur_status.adv_param.cur_ev_setting.exp_line;
	cxt->sync_aem[3 * 1024 + 2] = cxt->cur_status.adv_param.cur_ev_setting.dmy_line;
	cxt->sync_aem[3 * 1024 + 3] = cxt->cur_status.adv_param.cur_ev_setting.ae_gain;

	ISP_LOGV("exp: %d, gain:%d line time: %d\n", cxt->cur_status.adv_param.cur_ev_setting.exp_line, cxt->cur_status.adv_param.cur_ev_setting.ae_gain, cxt->cur_status.adv_param.cur_ev_setting.line_time);

	if(cxt->ebd_support) {
		if((1.0 * backup_expgain / effect_ebd_expgain >= 0.97) && (1.0 * backup_expgain / effect_ebd_expgain <= 1.03))
			cxt->ebd_stable_flag = 1;
		else
			cxt->ebd_stable_flag = 0;
		ISP_LOGE("ebd: stable_flag %d", cxt->ebd_stable_flag);
	}

	cxt->effect_index_index++;
	if(cxt->effect_index_index > 3)
		cxt->effect_index_index = 0;
	cxt->effect_index[cxt->effect_index_index] = cxt->sync_cur_result.ev_setting.ae_idx;

	rtn = ae_pre_process(cxt);
	flash_calibration_script((cmr_handle) cxt);/*for flash calibration*/
	ae_set_led(cxt);

#ifdef CONFIG_SUPPROT_AUTO_HDR
	if(!cxt->is_snapshot) {
		struct ae_size hdr_stat_size;
		hdr_stat.hist256 = calc_in->hist_stats.value;
		hdr_stat_size.w = 0;
		hdr_stat_size.h = 0;
		if (cxt->isp_ops.callback) {
			(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, AE_CB_HDR_STATIS_SIZE, &hdr_stat_size);
		}
		if(hdr_stat_size.w && hdr_stat_size.h) {
			hdr_stat.w = hdr_stat_size.w;
			hdr_stat.h = hdr_stat_size.h;
		} else {
			hdr_stat.w = cxt->snr_info.frame_size.w;
			hdr_stat.h = cxt->snr_info.frame_size.h;
		}
		hdr_param.thres_bright = 250;
		hdr_param.thres_dark = 20;
		auto_hdr_enable = (cmr_s8)sprd_hdr_scndet_multi_inst(&hdr_param, &hdr_stat, ev_result,&cxt->smooth_flag,&cxt->frameid);
		cxt->hdr_calc_result.ev[0] = fabs(ev_result[0]);
		cxt->hdr_calc_result.ev[1] = ev_result[1];

		cxt->cur_status.adv_param.hist_data.img_size.w = hdr_stat.w;
		cxt->cur_status.adv_param.hist_data.img_size.h = hdr_stat.h;
		cxt->cur_status.adv_param.hist_data.hist_bin = 256;
		memcpy(cxt->cur_status.adv_param.hist_data.hist_data, hdr_stat.hist256 ,cxt->cur_status.adv_param.hist_data.hist_bin * sizeof(cmr_u32));

		ISP_LOGV("auto_hdr bright %d dark %d w %d h %d ev[0] %f ev[1] %f", hdr_param.thres_bright, hdr_param.thres_dark, hdr_stat.w, hdr_stat.h, ev_result[0], ev_result[1]);
	}
	if (cxt->hdr_menu_ctrl) {
		cxt->hdr_calc_result.auto_hdr_enable = auto_hdr_enable;
		if(-1 == auto_hdr_enable)
			auto_hdr_enable = 0;
		if(cxt->isp_ops.callback)
			(*cxt->isp_ops.callback)(cxt->isp_ops.isp_handler, AE_CB_HDR_STATUS, &auto_hdr_enable);
		else
			ISP_LOGE("isp_ops.callback is NULL");
	}

	if (cxt->hdr_enable) {
		ae_set_hdr_ctrl(cxt, param);
	}
#endif

	ae_set_fdr_detect(cxt,param);

	if (cxt->ev_adj_enable)
		ae_set_ev_adjust_ctrl(cxt, param);
	if (cxt->fdr_enable)
		ae_set_fdr_ctrl(cxt, param);
	{
		char prop[PROPERTY_VALUE_MAX];
		int val_max = 0;
		int val_min = 0;
		property_get("persist.vendor.cam.isp.ae.fps", prop, "0");
		if (atoi(prop) != 0) {
			val_min = atoi(prop) % 100;
			val_max = atoi(prop) / 100;
			cxt->cur_status.adv_param.fps_range.min = val_min > 5 ? val_min : 5;
			cxt->cur_status.adv_param.fps_range.max = val_max;
		}
	}
	pthread_mutex_lock(&cxt->data_sync_lock);
	current_status = &cxt->sync_cur_status;
	current_result = &cxt->sync_cur_result;
	memcpy(current_status, &cxt->cur_status, sizeof(struct ae_lib_calc_in));
	memcpy(&cxt->cur_result, current_result, sizeof(struct ae_lib_calc_out));
	pthread_mutex_unlock(&cxt->data_sync_lock);

/***********************************************************/
	misc_calc_in = current_status;
	misc_calc_out = &cxt->cur_result;
	ATRACE_BEGIN(__FUNCTION__);
	cmr_u64 ae_time0 = systemTime(CLOCK_MONOTONIC);

	ISP_LOGV("misc_calc_in:camerId:%d, is_multi_mode:%d, ae_gain:%d, exp_time:%d", cxt->camera_id, cxt->is_multi_mode, cxt->cur_status.adv_param.mode_param.value.exp_gain[1], cxt->cur_status.adv_param.mode_param.value.exp_gain[0]);

	if (0 == cxt->skip_update_param_flag) {
		rtn = ae_lib_calculation(cxt->misc_handle, misc_calc_in, misc_calc_out);
	}
	ISP_LOGV("misc_calc_out: cameraId:%d, ae_idx:%d, ae_gain:%d, exp_time:%d", cxt->camera_id, cxt->cur_result.ev_setting.ae_idx, cxt->cur_result.ev_setting.ae_gain, cxt->cur_result.ev_setting.exp_time);

	ae_set_zoom_win_info(cxt);

	ae_set_bayer_hist(cxt, &cxt->cur_result.adjust_hist_roi);

	cmr_u64 ae_time1 = systemTime(CLOCK_MONOTONIC);
	ATRACE_END();
	ISP_LOGV("skip_update_param_flag: %d", cxt->skip_update_param_flag);
	ISP_LOG_PERF("SYSTEM_TEST -ae_test	 %dus ", (cmr_s32)((ae_time1 - ae_time0) / 1000));

	if (rtn) {
		ISP_LOGE("fail to calc ae misc");
		rtn = AE_ERROR;
		goto ae_calculation_error_exit;
	}

	memset((cmr_handle) & cxt->cur_status.adv_param.face_data, 0, sizeof(struct ae_face_param));

	/*just for debug: reset the status */
	if (1 == cxt->cur_status.adv_param.touch_roi_flag) {
		cxt->cur_status.adv_param.touch_roi_flag = 0;
	}
	rtn = ae_post_process(cxt);

/***********************************************************/
/*update parameters to sensor*/
	#if 0
	if (cxt->is_multi_mode == ISP_ALG_DUAL_C_C || cxt->is_multi_mode ==ISP_ALG_DUAL_W_T || cxt->is_multi_mode ==ISP_ALG_DUAL_C_M) 
	{
		if (CAM_SENSOR_SLAVE0 == cxt->sensor_role) {
			cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE0, GET_MATCH_BV_DATA, NULL, &bv);
			cxt->cur_result.cur_bv = bv;
		}
	}
	#endif

	memcpy(&cur_calc_result->ae_result, &cxt->cur_result, sizeof(struct ae_lib_calc_out));
	ae_make_calc_result(cxt, &cxt->cur_result, cur_calc_result);
	cxt->exp_data.lib_data.exp_line = cxt->cur_result.ev_setting.exp_line;
	cxt->exp_data.lib_data.exp_time = cxt->cur_result.ev_setting.exp_time;
	cxt->exp_data.lib_data.gain = cxt->cur_result.ev_setting.ae_gain;
	cxt->exp_data.lib_data.dummy = cxt->cur_result.ev_setting.dmy_line;
	cxt->exp_data.lib_data.line_time = current_status->adv_param.cur_ev_setting.line_time;
	cxt->exp_data.lib_data.frm_len = cxt->cur_result.ev_setting.frm_len;

	ISP_LOGV("ae_calculation, ae_update_result_to_sensor BEFORE, cameraId:%d, sensor_role:%d, gain:%d, exp_line:%d, line_time:%d, exp_time:%d",\
	cxt->camera_id, cxt->sensor_role, cxt->exp_data.lib_data.gain, cxt->exp_data.lib_data.exp_line, cxt->exp_data.lib_data.line_time, cxt->exp_data.lib_data.exp_time);

	if ((cxt->is_multi_mode == ISP_ALG_SINGLE) || (cxt->is_multi_mode == ISP_ALG_TRIBLE_W_T_UW)) {

		rtn = ae_update_result_to_sensor(cxt, &cxt->exp_data, 0);

	} else if (cxt->is_multi_mode == ISP_ALG_DUAL_C_C || cxt->is_multi_mode ==ISP_ALG_DUAL_W_T || cxt->is_multi_mode ==ISP_ALG_DUAL_C_M) {

		if (CAM_SENSOR_MASTER == cxt->sensor_role) {
			cxt->ptr_isp_br_ioctrl(CAM_SENSOR_MASTER, GET_USER_COUNT, NULL, &sensor_num);
			if (sensor_num > 1) {
				ae_sync_process(cxt, &cxt->exp_data);
			} else {
				rtn = ae_update_result_to_sensor(cxt, &cxt->exp_data, 0);
			}
		} else {
			/*save slave lib output to bridge*/
			struct ae_lib_output_data ae_lib_output = {0};
			ae_lib_output.dummy = cxt->exp_data.lib_data.dummy;
			ae_lib_output.exp_line = cxt->exp_data.lib_data.exp_line;
			ae_lib_output.exp_time = cxt->exp_data.lib_data.exp_time;	
			ae_lib_output.gain = cxt->exp_data.lib_data.gain;
			ae_lib_output.line_time = cxt->exp_data.lib_data.line_time;
			ae_lib_output.frm_len = cxt->exp_data.lib_data.frm_len;
			ISP_LOGV("ae_calculation, slave lib outout save to bridge, cameraId:%d, sensor_role:%d, gain:%d, exp_line:%d, line_time:%d, exp_time:%d",\
			cxt->camera_id, cxt->sensor_role, ae_lib_output.gain, ae_lib_output.exp_line, ae_lib_output.line_time, ae_lib_output.exp_time);
			cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE0, SET_SYNC_SLAVE_LIB_OUTPUT, &ae_lib_output, NULL);

			/*get sync slave output data and write sensor*/
			ae_update_result_to_slave_sensor(cxt, &cxt->exp_data, 0);
		}
	}
	#if 0
	if (cxt->has_mf) {
		if (cxt->has_mf_cnt==3) {
			cxt->has_mf = cxt->has_mf_cnt = 0;
			cb_type = AE_CB_CONVERGED;
			stop_skip_en = 1;
			if(cxt->isp_ops.callback)
			        (*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, cb_type, &stop_skip_en);
			ISP_LOGD("skip_num stop \r\n");
		} else {
			ISP_LOGD("skip_num prepare\r\n");
			cxt->has_mf_cnt++;
		}
	}
	#endif

/* send STAB notify to HAL */
	if (cxt->isp_ops.callback) {
		cb_type = AE_CB_STAB_NOTIFY;
		cxt->ae_cb_result[AE_CB_RESULT_STAB] = cur_calc_result->ae_output.is_stab;
		cxt->ae_cb_result[AE_CB_RESULT_BV_VALUE] = cur_calc_result->ae_output.cur_bv;
		cxt->ae_cb_result[AE_CB_RESULT_BLS_VALUE] = cur_calc_result->ae_output.abl_weight;

		(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, cb_type, &cxt->ae_cb_result);
		ISP_LOGV("normal notify stable_flag %d", cur_calc_result->ae_output.is_stab);
	}

	if(!( (cxt->cur_status.adv_param.flash >= FLASH_PRE_BEFORE) && (cxt->cur_status.adv_param.flash <= FLASH_PRE_AFTER))){
		if (cxt->cur_status.adv_param.mode_param.mode == AE_MODE_MANUAL_EXP_GAIN) {
			cxt->cur_status.adv_param.mode_param.value.exp_gain[0] = cxt->manual_exp_line_bkup;
			cxt->cur_status.adv_param.mode_param.value.exp_gain[1] = cxt->manual_iso_value;
		}else if (cxt->cur_status.adv_param.mode_param.mode == AE_MODE_AUTO_SHUTTER_PRI) {
			cxt->cur_status.adv_param.mode_param.value.exp_gain[0] = cxt->manual_exp_line_bkup;
		}else if (cxt->cur_status.adv_param.mode_param.mode == AE_MODE_AUTO_ISO_PRI) {
			cxt->cur_status.adv_param.mode_param.value.exp_gain[1] = cxt->manual_iso_value;
		}
	}
/***********************************************************/
	pthread_mutex_lock(&cxt->data_sync_lock);
	memcpy(current_result, &cxt->cur_result, sizeof(struct ae_lib_calc_out));
	pthread_mutex_unlock(&cxt->data_sync_lock);


	if (cxt->isp_ops.callback) {
		ae_make_isp_result(cxt, current_result, &callback_in);
		(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, AE_CB_PROCESS_OUT, &callback_in);
	}
/***********************************************************/
/*display the AE running status*/
	if (1 == cxt->debug_enable) {
		ae_save_to_mlog_file(cxt, misc_calc_out);
	}

	cxt->cur_status.frm_id++;
	cxt->is_first = 0;

  ae_calculation_error_exit:
	return rtn;
}

cmr_s32 ae_sprd_calculation_v1(cmr_handle handle, cmr_handle param, cmr_handle result)
{
	cmr_s32 rtn = AE_SUCCESS;
	struct ae_ctrl_cxt *cxt = (struct ae_ctrl_cxt *)handle;
	struct ae_calc_in *calc_in = (struct ae_calc_in *)param;

	ISP_LOGV("is_update %d", calc_in->is_update);
	if (cxt->high_fps_info.is_high_fps) {
		if (calc_in->is_update) {
			if ((cxt->is_multi_mode) && (0 == cxt->calcFirstFlag)) {
				ISP_LOGV("sync:SET_USER_COUNT");
				cmr_u32 in = 1;
				if (CAM_SENSOR_MASTER == cxt->sensor_role) {
					rtn = cxt->ptr_isp_br_ioctrl(CAM_SENSOR_MASTER, SET_USER_COUNT, &in, NULL);
				} else if (CAM_SENSOR_SLAVE0 == cxt->sensor_role) {
					rtn = cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE0, SET_USER_COUNT, &in, NULL);
				} else if (CAM_SENSOR_SLAVE1 == cxt->sensor_role) {
					rtn = cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE1, SET_USER_COUNT, &in, NULL);
				}
				cxt->calcFirstFlag = 1;
			}
			rtn = ae_calculation_slow_motion(handle, param, result);
		} else {
			cxt->exp_data.lib_data.exp_line = cxt->sync_cur_result.ev_setting.exp_line;
			cxt->exp_data.lib_data.exp_time = cxt->sync_cur_result.ev_setting.exp_time;
			cxt->exp_data.lib_data.gain = cxt->sync_cur_result.ev_setting.ae_gain;
			cxt->exp_data.lib_data.dummy = cxt->sync_cur_result.ev_setting.dmy_line;
			cxt->exp_data.lib_data.line_time = cxt->cur_status.adv_param.cur_ev_setting.line_time;
			cxt->exp_data.lib_data.frm_len = cxt->sync_cur_result.ev_setting.frm_len;
			//cxt->exp_data.lib_data.frm_len_def = cxt->sync_cur_result.ev_setting.frm_len_def;

			rtn = ae_update_result_to_sensor(cxt, &cxt->exp_data, 0);
		}

		if (calc_in->is_update) {
			cxt->slw_prev_skip_num++;
		}
	} else {
		if (calc_in->is_update) {
			if ((cxt->is_multi_mode) && (0 == cxt->calcFirstFlag)) {
				ISP_LOGV("sync:SET_USER_COUNT");
				cmr_u32 in = 1;
				if (CAM_SENSOR_MASTER == cxt->sensor_role) {
					rtn = cxt->ptr_isp_br_ioctrl(CAM_SENSOR_MASTER, SET_USER_COUNT, &in, NULL);
				} else if (CAM_SENSOR_SLAVE0 == cxt->sensor_role) {
					rtn = cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE0, SET_USER_COUNT, &in, NULL);
				} else if (CAM_SENSOR_SLAVE1 == cxt->sensor_role) {
					rtn = cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE1, SET_USER_COUNT, &in, NULL);
				}
				cxt->calcFirstFlag = 1;
			}
			rtn = ae_calculation(handle, param, result);
		}
	}

	if(!cxt->is_snapshot)
		cxt->last_enable = 1;

	return rtn;
}

static cmr_s32 ae_io_ctrl_direct(cmr_handle handle, cmr_s32 cmd, cmr_handle param, cmr_handle result)
{
	cmr_s32 rtn = AE_SUCCESS;
	struct ae_ctrl_cxt *cxt = NULL;

	rtn = ae_check_handle(handle);
	if (AE_SUCCESS != rtn) {
		ISP_LOGE("fail to check handle %p", handle);
		return AE_HANDLER_NULL;
	}

	cxt = (struct ae_ctrl_cxt *)handle;
	pthread_mutex_lock(&cxt->data_sync_lock);
	switch (cmd) {
	case AE_GET_CALC_RESULTS:
		rtn = ae_get_calc_reuslts(cxt, result);
		break;

	case AE_GET_ISO:
		rtn = ae_get_iso(cxt, (cmr_u32 *) result);
		break;

	case AE_GET_FLASH_EFFECT:
		rtn = ae_get_flash_effect(cxt, result);
		break;

	case AE_GET_AE_STATE:
		break;

	case AE_GET_FLASH_EB:
		rtn = ae_get_flash_enable(cxt, result);
		break;

	case AE_GET_BV_BY_GAIN:
		rtn = ae_get_gain(cxt, result);
		break;

	case AE_GET_FLASH_ENV_RATIO:
		rtn = ae_get_flash_env_ratio(cxt, result);
		break;

	case AE_GET_FLASH_ONE_OF_ALL_RATIO:
		rtn = ae_get_flash_one_in_all_ratio(cxt, result);
		break;

	case AE_GET_BV_BY_LUM:
		rtn = ae_get_bv_by_lum(cxt, (cmr_s32 *) result);
		break;

	case AE_GET_BV_BY_LUM_NEW:
		rtn = ae_get_bv_by_lum_new(cxt, (cmr_s32 *) result);
		break;

	case AE_GET_LUM:
		rtn = ae_get_luma(cxt, result);
		break;

	case AE_SET_SNAPSHOT_NOTICE:
		break;

	case AE_GET_MONITOR_INFO:
		rtn = ae_get_monitor_info(cxt, result);
		break;

	case AE_GET_FLICKER_MODE:
		rtn = ae_get_flicker_mode(cxt, result);
		break;

	case AE_GET_GAIN:
		rtn = ae_get_gain(cxt, result);
		break;

	case AE_GET_EXP:
		rtn = ae_get_exp_time(cxt, result);
		break;

	case AE_GET_FLICKER_SWITCH_FLAG:
		rtn = ae_get_flicker_switch_flag(cxt, param);
		break;

	case AE_GET_CUR_WEIGHT:
		rtn = ae_get_metering_mode(cxt, result);
		break;

	case AE_GET_EXP_TIME:
		if (result) {
			*(cmr_u32 *) result = cxt->cur_result.ev_setting.exp_time / 100;
		}
		break;

	case AE_GET_SENSITIVITY:
		if (param) {
			*(cmr_u32 *) param = cxt->cur_result.ev_setting.ae_gain * 50 / 128;
		}
		break;

	case AE_GET_DEBUG_INFO:
		ISP_LOGD("AE_GET_DEBUG_INFO");
		rtn = ae_get_debug_info(cxt, result);
		break;

	case AE_GET_EM_PARAM:
		rtn = ae_get_debug_info_for_display(cxt, result);
		break;

	case AE_GET_FLASH_WB_GAIN:
		rtn = ae_get_flash_wb_gain(cxt, result);
		break;

	case AE_GET_FPS:
		rtn = ae_get_fps(cxt, result);
		break;

	case AE_GET_LEDS_CTRL:
		rtn = ae_get_led_ctrl(cxt, result);
		break;

	case AE_GET_GLB_GAIN:
		if (result) {
			*(cmr_u32 *) result = cxt->glb_gain;
		}
		break;
	case AE_GET_LOWLIGHT_FLAG_BY_BV:
		rtn = ae_get_lowlight_flag_by_bv(cxt, (cmr_u32 *) result);
		break;
	case AE_GET_FLASH_SKIP_FRAME_NUM:
		if (result){
			*(cmr_u32 *) result = cxt->flash_timing_param.main_skip_num;
			ISP_LOGD("main_skip_num %d ",cxt->flash_timing_param.main_skip_num);
		}
		break;
	case AE_GET_APP_MODE:
		rtn = ae_get_app_mode(cxt, result);
		break;
	case AE_GET_DC_DV_FPS_RANGE:
		rtn = ae_get_dc_dv_fps_range(cxt, result);
		break;
	default:
		rtn = AE_ERROR;
		break;
	}
	pthread_mutex_unlock(&cxt->data_sync_lock);
	return rtn;
}

static cmr_s32 ae_io_ctrl_sync(cmr_handle handle, cmr_s32 cmd, cmr_handle param, cmr_handle result)
{
	cmr_s32 rtn = AE_SUCCESS;
	struct ae_ctrl_cxt *cxt = NULL;

	rtn = ae_check_handle(handle);
	if (AE_SUCCESS != rtn) {
		ISP_LOGE("fail to check handle %p", handle);
		return AE_HANDLER_NULL;
	}

	cxt = (struct ae_ctrl_cxt *)handle;

	switch (cmd) {
	case AE_SET_DC_DV:
		rtn = ae_set_dc_dv_mode(cxt, param);
		break;

	case AE_SET_SCENE_MODE:
		rtn = ae_set_scene(cxt, param);
		break;

	case AE_SET_ISO:
		rtn = ae_set_iso(cxt, param);
		break;

	case AE_SET_MANUAL_ISO:
		rtn = ae_set_manual_iso(cxt, param);
		break;

	case AE_SET_FLICKER:
		rtn = ae_set_flicker(cxt, param);
		break;

	case AE_SET_WEIGHT:
		rtn = ae_set_weight(cxt, param);
		break;

	case AE_SET_EV_OFFSET:
		rtn = ae_set_ev_offset(cxt, param);
		break;

	case AE_SET_FPS:
		rtn = ae_set_fps_info(cxt, param);
		break;

	case AE_SET_PAUSE:
		rtn = ae_set_pause(cxt,3);
		break;

	case AE_SET_RESTORE:
		rtn = ae_set_restore_cnt(cxt,5);
		break;

	case AE_SET_FORCE_PAUSE:
		rtn = ae_set_force_pause(cxt, 1, 14);
		cxt->exposure_compensation.ae_compensation_flag = 1;
		ISP_LOGD("AE_SET_FORCE_PAUSE");
		break;

	case AE_SET_FORCE_RESTORE:
		rtn = ae_set_force_pause(cxt, 0, 15);
		cxt->exposure_compensation.ae_compensation_flag = 0;
		ISP_LOGD("AE_SET_FORCE_RESTORE");
		break;

	case AE_SET_FLASH_NOTICE:
		rtn = ae_set_flash_notice(cxt, param);
		break;

	case AE_CTRL_SET_FLASH_MODE:
		rtn = ae_crtl_set_flash_mode(cxt, param);
		break;

	case AE_SET_STAT_TRIM:
		rtn = ae_set_scaler_trim(cxt, (struct ae_trim *)param);
		break;

	case AE_SET_G_STAT:
		rtn = ae_set_g_stat(cxt, (struct ae_stat_mode *)param);
		break;

	case AE_SET_ONLINE_CTRL:
		rtn = ae_tool_online_ctrl(cxt, param, result);
		break;

	case AE_SET_EXP_GAIN:
		break;

	case AE_SET_FD_PARAM:
		rtn = ae_set_fd_param(cxt, param);
		break;

	case AE_SET_NIGHT_MODE:
		break;

	case AE_SET_BYPASS:
		rtn = ae_bypass_algorithm(cxt, param);
		break;

	case AE_SET_FORCE_QUICK_MODE:
		break;

	case AE_SET_MANUAL_MODE:
		rtn = ae_set_manual_mode(cxt, param);
		break;

	case AE_SET_EXP_TIME:
		rtn = ae_set_exp_time(cxt, param);
		break;

	case AE_SET_SENSITIVITY:
		rtn = ae_set_gain(cxt, param);
		break;

	case AE_VIDEO_STOP:
		ae_set_video_stop(cxt);
		break;

	case AE_VIDEO_START:
		rtn = ae_set_video_start(cxt, param);
		break;

	case AE_HDR_START:
		rtn = ae_set_hdr_start(cxt, param);
		break;

	case AE_CAP_EV_ADJUST_START:
		rtn = ae_set_ev_adjust_start(cxt, param);
		break;

	case AE_CAF_LOCKAE_START:
		rtn = ae_set_caf_lockae_start(cxt);
		break;

	case AE_FDR_START:
		rtn = ae_set_fdr_start(cxt, param);
		break;

	case AE_SET_AUTO_FDR:
		rtn = ae_set_auto_fdr(cxt, param);
		break;

	case AE_CAF_LOCKAE_STOP:
		rtn = ae_set_caf_lockae_stop(cxt);
		break;

	case AE_SET_RGB_GAIN:
		rtn = ae_set_isp_gain(cxt);
		break;

	case AE_SET_UPDATE_AUX_SENSOR:
		rtn = ae_set_aux_sensor(cxt, param, result);
		break;

	case AE_SET_EXPOSURE_COMPENSATION:
		rtn = ae_set_exposure_compensation(cxt, param);
		break;

	case AE_SET_CAP_FLAG:
		cxt->cam_cap_flag = *(cmr_u32 *)param;
		ISP_LOGD("cam_cap_flag=%d", cxt->cam_cap_flag);
		break;

	case AE_SET_SCENE_INFO:
		rtn = ae_set_scene_info(cxt, param);
		break;

	case AE_SET_AUTO_HDR:
#ifdef CONFIG_SUPPROT_AUTO_HDR
		rtn = ae_set_auto_hdr(cxt, param);
#endif
		break;

	case AE_SET_APP_MODE:
		cxt->app_mode = *(cmr_u32 *) param;
		ISP_LOGD("app_mode=%d", cxt->app_mode);
		break;

	case AE_SET_TOUCH_ZONE:
		rtn = ae_set_touch_zone(cxt, param);
		break;

	case AE_SET_3DNR_MODE:
		rtn = ae_set_3dnr_mode(cxt, param);
		break;

	case AE_SET_3DNR_THR:
		rtn = ae_set_3dnr_thr(cxt, param);
		break;

	case AE_SET_AF_STATUS:
		cxt->cur_status.adv_param.af_status = *(cmr_u32 *) param;
		break;

	default:
		rtn = AE_ERROR;
		break;
	}
	return rtn;
}

cmr_s32 ae_sprd_io_ctrl_v1(cmr_handle handle, cmr_s32 cmd, cmr_handle param, cmr_handle result)
{
	cmr_s32 rtn = AE_SUCCESS;

	rtn = ae_check_handle(handle);
	if (AE_SUCCESS != rtn) {
		ISP_LOGE("fail to check handle %p", handle);
		return AE_HANDLER_NULL;
	}

	if ((AE_SYNC_MSG_BEGIN < cmd) && (AE_SYNC_MSG_END > cmd)) {
		rtn = ae_io_ctrl_sync(handle, cmd, param, result);
	} else if ((AE_DIRECT_MSG_BEGIN < cmd) && (AE_DIRECT_MSG_END > cmd)) {
		rtn = ae_io_ctrl_direct(handle, cmd, param, result);
	} else {
		ISP_LOGE("fail to find cmd %d", cmd);
		return AE_ERROR;
	}
	return rtn;
}

cmr_s32 ae_sprd_deinit_v1(cmr_handle handle, cmr_handle in_param, cmr_handle out_param)
{
	cmr_s32 rtn = AE_SUCCESS;
	struct ae_ctrl_cxt *cxt = NULL;
	UNUSED(in_param);
	UNUSED(out_param);

	rtn = ae_check_handle(handle);
	if (AE_SUCCESS != rtn) {
		return AE_ERROR;
	}
	cxt = (struct ae_ctrl_cxt *)handle;

	rtn = flash_deinit(cxt->flash_alg_handle);
	if (0 != rtn)
		ISP_LOGE("fail to deinit flash, rtn: %d\n", rtn);
	else
		cxt->flash_alg_handle = NULL;

	rtn = ae_lib_deinit(cxt->misc_handle, NULL, NULL);

	if (AE_SUCCESS != rtn)
		ISP_LOGE("fail to deinit ae misc， rtn: %d\n", rtn);
	//ae_seq_reset(cxt->seq_handle);

	rtn = s_q_close(cxt->seq_handle);
	if (AE_SUCCESS != rtn)
		ISP_LOGE("fail to deinit ae misc， rtn: %d\n", rtn);

	if (cxt->debug_enable) {
		if (cxt->debug_info_handle) {
			debug_file_deinit((debug_handle_t) cxt->debug_info_handle);
			cxt->debug_info_handle = (cmr_handle) NULL;
		}
	}

	if (cxt->tune_buf) {
		free(cxt->tune_buf);
		cxt->tune_buf = NULL;
	}

	pthread_mutex_destroy(&cxt->data_sync_lock);
	ISP_LOGD("cam-id %d", cxt->camera_id);
	free(cxt);
	ISP_LOGD("done");

	return rtn;
}

static void ae_set_ae_init_param(struct ae_ctrl_cxt *cxt, struct ae_lib_init_out *misc_init_out)
{
	cxt->major_id = misc_init_out->major_id;
	cxt->minor_id = misc_init_out->minor_id;
	cxt->sensor_max_gain = misc_init_out->ctrl_timing_param.max_gain;
	cxt->sensor_max_gain = cxt->sensor_max_gain? cxt->sensor_max_gain : 16 * 128;
	cxt->sensor_min_gain = misc_init_out->ctrl_timing_param.min_gain;
	cxt->sensor_min_gain = cxt->sensor_min_gain? cxt->sensor_min_gain : 1 * 128;
	cxt->min_exp_line = misc_init_out->ctrl_timing_param.min_exp_line;
	cxt->min_exp_line = cxt->min_exp_line? cxt->min_exp_line : 4;
	cxt->sensor_gain_precision = misc_init_out->ctrl_timing_param.gain_precision;
	cxt->exp_skip_num = misc_init_out->ctrl_timing_param.exp_skip_num;
	cxt->gain_skip_num = misc_init_out->ctrl_timing_param.gain_skip_num;

	cxt->cur_status.adv_param.cur_ev_setting.line_time = misc_init_out->ev_setting.line_time;
	cxt->cur_status.adv_param.cur_ev_setting.ae_idx = misc_init_out->ev_setting.ae_idx;
	cxt->cur_status.adv_param.cur_ev_setting.exp_time = misc_init_out->ev_setting.exp_time;
	cxt->cur_status.adv_param.cur_ev_setting.exp_line = misc_init_out->ev_setting.exp_line;
	cxt->cur_status.adv_param.cur_ev_setting.dmy_line = misc_init_out->ev_setting.dmy_line;
	cxt->cur_status.adv_param.cur_ev_setting.frm_len = misc_init_out->ev_setting.frm_len;
	cxt->cur_status.adv_param.cur_ev_setting.ae_gain = misc_init_out->ev_setting.ae_gain;

	cxt->cur_result.ev_setting.ae_idx = misc_init_out->ev_setting.ae_idx;
	cxt->cur_result.ev_setting.exp_time = misc_init_out->ev_setting.exp_time;
	cxt->cur_result.ev_setting.exp_line = misc_init_out->ev_setting.exp_line;
	cxt->cur_result.ev_setting.dmy_line = misc_init_out->ev_setting.dmy_line;
	cxt->cur_result.ev_setting.frm_len = misc_init_out->ev_setting.frm_len;
	cxt->cur_result.ev_setting.ae_gain = misc_init_out->ev_setting.ae_gain;

	cxt->monitor_cfg.trim.x = misc_init_out->aem_cfg.monitor_rect.start_x;
	cxt->monitor_cfg.trim.y = misc_init_out->aem_cfg.monitor_rect.start_y;
	cxt->monitor_cfg.trim.w = misc_init_out->aem_cfg.monitor_rect.end_x - misc_init_out->aem_cfg.monitor_rect.start_x;
	cxt->monitor_cfg.trim.h = misc_init_out->aem_cfg.monitor_rect.end_y - misc_init_out->aem_cfg.monitor_rect.start_y;
	cxt->monitor_cfg.blk_size.w = misc_init_out->aem_cfg.blk_size.w;
	cxt->monitor_cfg.blk_size.h = misc_init_out->aem_cfg.blk_size.h;
	cxt->monitor_cfg.blk_num.w = misc_init_out->aem_cfg.blk_num.w;
	cxt->monitor_cfg.blk_num.h = misc_init_out->aem_cfg.blk_num.h;
	cxt->monitor_cfg.shift = misc_init_out->aem_cfg.monitor_shift;
	cxt->monitor_cfg.data_type = misc_init_out->aem_cfg.data_type;

	//cxt->monitor_cfg.oe_thrd = misc_init_out->aem_cfg.oe_thrd;
	//cxt->monitor_cfg.ue_thrd = misc_init_out->aem_cfg.ue_thrd;
	//because to match the setting in the middleware 
	cxt->monitor_cfg.high_region_thrd.r = misc_init_out->aem_cfg.oe_thrd;
	cxt->monitor_cfg.high_region_thrd.g = misc_init_out->aem_cfg.oe_thrd;
	cxt->monitor_cfg.high_region_thrd.b = misc_init_out->aem_cfg.oe_thrd;
	cxt->monitor_cfg.low_region_thrd.r = misc_init_out->aem_cfg.ue_thrd;
	cxt->monitor_cfg.low_region_thrd.g = misc_init_out->aem_cfg.ue_thrd;
	cxt->monitor_cfg.low_region_thrd.b = misc_init_out->aem_cfg.ue_thrd;
	//bayer hist tuning setting from lib parser
	cxt->bhist_cfg.hist_rect.start_x = misc_init_out->bhist_cfg.hist_rect.start_x;
	cxt->bhist_cfg.hist_rect.start_y = misc_init_out->bhist_cfg.hist_rect.start_y;
	cxt->bhist_cfg.hist_rect.end_x = misc_init_out->bhist_cfg.hist_rect.end_x;
	cxt->bhist_cfg.hist_rect.end_y = misc_init_out->bhist_cfg.hist_rect.end_y;
	cxt->bhist_cfg.bypass = misc_init_out->bhist_cfg.bypass;
	cxt->bhist_cfg.mode = misc_init_out->bhist_cfg.mode;
	cxt->bhist_cfg.shift = misc_init_out->bhist_cfg.shift;
	cxt->bhist_cfg.skip_num = misc_init_out->bhist_cfg.skip_num;
	
	cxt->flash_timing_param = misc_init_out->flash_timing_param;
	if (0 == cxt->flash_timing_param.pre_param_update_delay)
		cxt->flash_timing_param.pre_param_update_delay = 2;
	if (0 == cxt->flash_timing_param.pre_open_delay)
		cxt->flash_timing_param.pre_open_delay = 2;
	if (0 == cxt->flash_timing_param.estimate_delay)
		cxt->flash_timing_param.estimate_delay = 3;
	if (0 == cxt->flash_timing_param.main_param_update_delay)
		cxt->flash_timing_param.main_param_update_delay = 1;
	if (0 == cxt->flash_timing_param.main_open_delay)
		cxt->flash_timing_param.main_open_delay = 5;
	if (0 == cxt->flash_timing_param.main_capture_delay)
		cxt->flash_timing_param.main_capture_delay = 6;

	cxt->cur_status.adv_param.fps_range.min = misc_init_out->fps_range.min;
	cxt->cur_status.adv_param.fps_range.max = misc_init_out->fps_range.max;

	cxt->cur_status.adv_param.lock = misc_init_out->lock;
	cxt->cur_status.adv_param.metering_mode = misc_init_out->metering_mode;
	cxt->cur_status.adv_param.iso = misc_init_out->iso;
	cxt->cur_status.adv_param.flicker = misc_init_out->flicker;
	cxt->cur_status.adv_param.scene_mode = misc_init_out->scene_mode;
	cxt->cur_status.awb_gain.r = 1024;
	cxt->cur_status.awb_gain.g = 1024;
	cxt->cur_status.awb_gain.b = 1024;
	cxt->cur_status.adv_param.awb_mode = 0;
	cxt->cur_status.adv_param.is_faceID = 0;
	cxt->cur_status.fno = 2.0;
	// = misc_init_out->ae_mode;
	cxt->cur_status.adv_param.comp_param.value.ev_index = misc_init_out->evd_val;

	cxt->ev_param_table = misc_init_out->ev_param;

	cxt->dv_fps_range.max = misc_init_out->dv_fps.max ? misc_init_out->dv_fps.max : 30;
	cxt->dv_fps_range.min = misc_init_out->dv_fps.min ? misc_init_out->dv_fps.min : 5;
	cxt->dc_fps_range.max = misc_init_out->dc_fps.max ? misc_init_out->dc_fps.max : 30;
	cxt->dc_fps_range.min = misc_init_out->dc_fps.min ? misc_init_out->dc_fps.min : 5;

	cxt->flash_thrd = misc_init_out->thrd_param[0];
	cxt->threednr_thrd = misc_init_out->thrd_param[1];
	cxt->ae_video_fps = misc_init_out->thrd_param[2];
	cxt->cam_4in1_switch_thrd = misc_init_out->thrd_param[3];

	if (1 == cxt->camera_id) {
		cxt->flash_thrd.thd_down = cxt->flash_thrd.thd_down ? cxt->flash_thrd.thd_down : 250;
		cxt->flash_thrd.thd_up = cxt->flash_thrd.thd_up ? cxt->flash_thrd.thd_up : 500;
		cxt->threednr_thrd.thd_down = cxt->threednr_thrd.thd_down ? cxt->threednr_thrd.thd_down : 250;
		cxt->threednr_thrd.thd_up = cxt->threednr_thrd.thd_up ? cxt->threednr_thrd.thd_up : 500;
		cxt->ae_video_fps.thd_down = cxt->ae_video_fps.thd_down ? cxt->ae_video_fps.thd_down : 250;
		cxt->ae_video_fps.thd_up = cxt->ae_video_fps.thd_up ? cxt->ae_video_fps.thd_up : 500;
		cxt->cam_4in1_switch_thrd.thd_down = cxt->cam_4in1_switch_thrd.thd_down ? cxt->cam_4in1_switch_thrd.thd_down : 500;
		cxt->cam_4in1_switch_thrd.thd_up = cxt->cam_4in1_switch_thrd.thd_up ? cxt->cam_4in1_switch_thrd.thd_up : 800;
	}
	if (0 == cxt->camera_id || 2 == cxt->camera_id || 3 == cxt->camera_id) {
		cxt->flash_thrd.thd_down = cxt->flash_thrd.thd_down ? cxt->flash_thrd.thd_down : 380;
		cxt->flash_thrd.thd_up = cxt->flash_thrd.thd_up ? cxt->flash_thrd.thd_up : 480;
		cxt->threednr_thrd.thd_down = cxt->threednr_thrd.thd_down ? cxt->threednr_thrd.thd_down : 380;
		cxt->threednr_thrd.thd_up = cxt->threednr_thrd.thd_up ? cxt->threednr_thrd.thd_up : 480;
		cxt->ae_video_fps.thd_down = cxt->ae_video_fps.thd_down ? cxt->ae_video_fps.thd_down : 380;
		cxt->ae_video_fps.thd_up = cxt->ae_video_fps.thd_up ? cxt->ae_video_fps.thd_up : 480;
		cxt->cam_4in1_switch_thrd.thd_down = cxt->cam_4in1_switch_thrd.thd_down ? cxt->cam_4in1_switch_thrd.thd_down : 500;
		cxt->cam_4in1_switch_thrd.thd_up = cxt->cam_4in1_switch_thrd.thd_up ? cxt->cam_4in1_switch_thrd.thd_up : 800;
	}
}

cmr_handle ae_sprd_init_v1(cmr_handle param, cmr_handle in_param)
{
	cmr_s32 rtn = AE_SUCCESS;
	char ae_property[PROPERTY_VALUE_MAX];
	struct ae_ctrl_cxt *cxt = NULL;
	struct ae_init_out *ae_init_out = NULL;
	struct ae_lib_init_in misc_init_in;
	struct ae_lib_init_out misc_init_out;
	struct s_q_open_param s_q_param = { 0, 0, 0 };

	struct ae_init_in *init_param = NULL;
	struct Flash_initInput flash_in;
	struct Flash_initOut flash_out;

#ifdef CONFIG_SUPPROT_AUTO_HDR
	struct _tag_hdr_version_t hdr_version;
	sprd_hdr_version(&hdr_version);
	ISP_LOGD("HDR_version :%s", hdr_version.built_rev);
#endif

	cxt = (struct ae_ctrl_cxt *)calloc(1,sizeof(struct ae_ctrl_cxt));

	if (NULL == cxt) {
		rtn = AE_ALLOC_ERROR;
		ISP_LOGE("fail to calloc");
		goto ae_sprd_init_err_exit;
	}
	memset(&misc_init_in, 0, sizeof(misc_init_in));
	memset(&misc_init_out, 0, sizeof(misc_init_out));

	if (NULL == param) {
		ISP_LOGE("fail to get input param %p\r\n", param);
		rtn = AE_ERROR;
		goto ae_sprd_init_err_exit;
	}
	init_param = (struct ae_init_in *)param;
	ae_init_out = (struct ae_init_out *)in_param;
	cxt->backup_rgb_gain = init_param->bakup_rgb_gain;
	cxt->ob_rgb_gain = 1.0 * cxt->backup_rgb_gain / 4096;
	ISP_LOGD("bakup_rgb_gain %d, ob_rgb_gain %f", cxt->backup_rgb_gain, cxt->ob_rgb_gain);

	cxt->ptr_isp_br_ioctrl = init_param->ptr_isp_br_ioctrl;
	cxt->isp_ops = init_param->isp_ops;
	cxt->start_id = AE_START_ID;
	cxt->checksum = ae_get_checksum();
	cxt->end_id = AE_END_ID;
	cxt->camera_id = init_param->camera_id;
	cxt->snr_info = init_param->resolution_info;
	cxt->cur_status.img_size = init_param->resolution_info.frame_size;
	cxt->cur_status.adv_param.cur_ev_setting.line_time = init_param->resolution_info.line_time;
	cxt->cur_status.frm_id = 0;
	cxt->slw_prev_skip_num = 0;
	cxt->calcFirstFlag = 0;

	cxt->is_master = init_param->is_master;
	cxt->is_multi_mode = init_param->is_multi_mode;
	cxt->ebd_support = init_param->ebd_support;
	cxt->bv_thd = init_param->bv_thd;
	cxt->sensor_role = init_param->sensor_role;
	cxt->fdr_tuning_param = init_param->fdr_tuning_param;

	ISP_LOGD("sync:is_multi_mode=%d, sensor_role=%d, ebd_support=%d, is_master=%d\n", init_param->is_multi_mode, init_param->sensor_role, init_param->ebd_support, cxt->is_master);
	// parser ae otp info
	ae_parser_otp_info(init_param);
	/* HJW_S: dual flash algorithm init */

	memcpy(&cxt->dflash_param[0], init_param->flash_tuning[0].param, sizeof(struct flash_tune_param));
	ISP_LOGD("multiColorLcdEn = %d", cxt->dflash_param[0].multiColorLcdEn);
	flash_in.debug_level = 1;	/*it will be removed in the future, and get it from dual flash tuning parameters */
	flash_in.tune_info = &cxt->dflash_param[0];
	flash_in.statH = 32;
	flash_in.statW = 32;
	cxt->flash_alg_handle = flash_init(&flash_in, &flash_out);
	flash_out.version = 1;		//remove later
	cxt->flash_ver = flash_out.version;
	ae_init_out->flash_ver = cxt->flash_ver;
	cxt->mainFlashEn = flash_out.mainFlashEn;
	cxt->multiColorLcdEn = cxt->dflash_param[0].multiColorLcdEn;

	/*jhin add flash mode*/
	cxt->cur_status.adv_param.flash_mode = 0;//ok
	/*jhin add touch ev to reset */
	cxt->exposure_compensation.touch_ev_flag = 0;
	cxt->last_table_index = 0;
	cxt->ae_comp_value = 0;
	cxt->env_cum_changedCalc_delay_cnt = 0;
	cxt->env_cum_changed = 0;
	cxt->previous_lum = 0;
	cxt->manual_ae_mode = 1;
	cxt->bypass = init_param->has_force_bypass;
	cxt->manual_level = AE_MANUAL_EV_INIT;
	cxt->debug_enable = ae_is_mlog(cxt);

	ae_load_fdr_lib(cxt);

	ISP_LOGD("camera_id: %d, is master: %d, is_multi_mode: %d, sensor_role:%d", cxt->camera_id, init_param->is_master, init_param->is_multi_mode, cxt->sensor_role);
	if((cxt->sensor_role == CAM_SENSOR_MASTER) && (init_param->is_multi_mode)) {//dual camera && master sensor
		misc_init_in.dual_cam_tuning_param = init_param->ae_sync_param.param;
		misc_init_in.cam_id = AE_ROLE_ID_MASTER;//ok
	} else {
		misc_init_in.dual_cam_tuning_param = NULL;
		if (cxt->sensor_role == CAM_SENSOR_MASTER) {
			misc_init_in.cam_id = AE_ROLE_ID_NORMAL;
		} else if (cxt->sensor_role == CAM_SENSOR_SLAVE0) {
			misc_init_in.cam_id = AE_ROLE_ID_SLAVE0;
		} if (cxt->sensor_role == CAM_SENSOR_SLAVE1) {
			misc_init_in.cam_id = AE_ROLE_ID_SLAVE1;
		}
	}

	if (0 == cxt->camera_id) {
		misc_init_in.cam_id = misc_init_in.cam_id | ((AE_CAM_REAR&0xff)<<16);
	}else if (1 == cxt->camera_id)  {
		misc_init_in.cam_id = misc_init_in.cam_id | ((AE_CAM_FRONT& 0xff)<<16);
	} else if (2 == cxt->camera_id) {
		misc_init_in.cam_id = misc_init_in.cam_id | ((AE_CAM_REAR&0xff)<<16);
	} else if (3 == cxt->camera_id) {
		misc_init_in.cam_id = misc_init_in.cam_id | ((AE_CAM_REAR&0xff)<<16);
	}
	#if 0 //misc_init_in.cam_id NO change appers
	if (init_param->is_multi_mode) {
		if (0 == init_param->is_master) {
			misc_init_in.cam_id = misc_init_in.cam_id | ((AE_ROLE_ID_MASTER & 0xff));
		} else if (1 == init_param->is_master){
			misc_init_in.cam_id = misc_init_in.cam_id | ((AE_ROLE_ID_SLAVE0 & 0xff));
		} else if (2 == init_param->is_master) {
			misc_init_in.cam_id = misc_init_in.cam_id | ((AE_ROLE_ID_SLAVE1 & 0xff));
		}
	}
	#endif

	
	misc_init_in.mlog_en = cxt->debug_enable;//ok
	misc_init_in.log_level = g_isp_log_level;
	misc_init_in.line_time = init_param->resolution_info.line_time;
	misc_init_in.tuning_param = init_param->param[0].param;
	{
		char prop_str[PROPERTY_VALUE_MAX] = {0};
		property_get("persist.vendor.cam.isp.ae.read_tune_bin", prop_str, "0");
		if (1 == atoi(prop_str)) {
			FILE* pf  = NULL;
			char file_name[128] = {0};
			if (0 == cxt->camera_id) {/*rear camera*/
				sprintf(file_name, "%s", "data/vendor/cameraserver/ae_common_b.bin");/*get the rear camera ae tune param bin*/
			} else if (1 == cxt->camera_id) {
				sprintf(file_name, "%s", "data/vendor/cameraserver/ae_common_f.bin");/*get the front camera ae tune param bin*/
			} else if (2 == cxt->camera_id) {
				sprintf(file_name, "%s", "data/vendor/cameraserver/ae_common_b_s0.bin");/*get the rear slave0 camera ae tune param bin*/
			} else if (3 == cxt->camera_id) {
				sprintf(file_name, "%s", "data/vendor/cameraserver/ae_common_b_s1.bin");/*get the rear slave1 camera ae tune param bin*/
			}
			pf = fopen(file_name, "rb");
			ISP_LOGD("cam id:%d, tune bin: %s, pf: %p\n", cxt->camera_id, file_name, pf);
			if (pf) {
				cmr_u32 len = 0;
				fseek(pf, 0L, SEEK_END);
				len = ftell(pf);/*to get the size of bin file */
				fseek(pf, 0L, SEEK_SET);/*move the index flag to the start position*/
				if(len != -1){
                                        cxt->tune_buf = malloc(len);
                                        int sz = 0;
                                        sz = fread(cxt->tune_buf, 1, len, pf);
                                        if(sz != len)
                                                ISP_LOGE("ae_sprd_init fread faild");
                                        fclose(pf);
				}else
                                        ISP_LOGE("ae_sprd_init fseek faild");
				misc_init_in.tuning_param = (void*)&cxt->tune_buf[0];
			} else
				ISP_LOGW("read tune bin failed: %s\n", file_name);
		}
	}
	cxt->misc_handle = ae_lib_init(&misc_init_in, &misc_init_out);
	ae_set_ae_init_param(cxt, &misc_init_out);

	s_q_param.exp_valid_num = cxt->exp_skip_num + 1 + AE_UPDAET_BASE_OFFSET;
	s_q_param.sensor_gain_valid_num = cxt->gain_skip_num + 1 + AE_UPDAET_BASE_OFFSET;
	s_q_param.isp_gain_valid_num = 1 + AE_UPDAET_BASE_OFFSET;
	cxt->seq_handle = s_q_open(&s_q_param);

	pthread_mutex_init(&cxt->data_sync_lock, NULL);

	/* set sensor exp/gain validate information */
	if (cxt->isp_ops.set_shutter_gain_delay_info) {
		struct ae_exp_gain_delay_info delay_info = { 0, 0, 0 };
		delay_info.group_hold_flag = 0;
		delay_info.valid_exp_num = cxt->exp_skip_num;
		delay_info.valid_gain_num = cxt->gain_skip_num;
		cxt->isp_ops.set_shutter_gain_delay_info(cxt->isp_ops.isp_handler, (cmr_handle) (&delay_info));
	} else {
		ISP_LOGE("fail to set_shutter_gain_delay_info\n");
	}

	cxt->is_faceId_unlock = init_param->is_faceId_unlock;
	cxt->face_lock_table_index = 4;
	memset((cmr_handle) & ae_property, 0, sizeof(ae_property));
	property_get("persist.vendor.cam.isp.ae.manual", ae_property, "off");
	//ISP_LOGV("persist.vendor.cam.isp.ae.manual: %s", ae_property);
	if (!strcmp("on", ae_property)) {
		cxt->manual_ae_on = 1;
	} else {
		cxt->manual_ae_on = 0;
	}
	int val = 0;
	memset((cmr_handle) & ae_property, 0, sizeof(ae_property));
	property_get("persist.vendor.cam.isp.ae.perflog", ae_property, "0");
	val = atoi(ae_property);
	if (0 < val)
		g_ae_perf_log_level = val+LEVEL_OVER_LOGD;

	memset((cmr_handle) & ae_property, 0, sizeof(ae_property));
	property_get("persist.vendor.cam.isp.ae.logv", ae_property, "0");
	val = atoi(ae_property);
	if (0 < val)
		g_ae_log_level = val;

	ISP_LOGD("done, handle: %p, cam-id %d, flash ver %d, alg_id %d.%d ver [%s]", cxt, cxt->camera_id, cxt->flash_ver, cxt->major_id, cxt->minor_id, AE_ADPT_CTRL_VER);

	return (cmr_handle) cxt;

  ae_sprd_init_err_exit:
	if (NULL != cxt) {
		free(cxt);
		cxt = NULL;
	}
	ISP_LOGE("fail to init ae %d", rtn);
	return NULL;
}

struct adpt_ops_type ae_sprd_adpt_ops_ver1 = {
	.adpt_init = ae_sprd_init_v1,
	.adpt_deinit = ae_sprd_deinit_v1,
	.adpt_process = ae_sprd_calculation_v1,
	.adpt_ioctrl = ae_sprd_io_ctrl_v1,
};

