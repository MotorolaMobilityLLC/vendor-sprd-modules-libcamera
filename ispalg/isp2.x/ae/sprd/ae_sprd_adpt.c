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
#include "ae_sprd_adpt.h"
#include "ae_sprd_adpt_internal.h"
#include "ae_sprd_flash_calibration.h"
#include "ae_misc.h"
#include "ae_debug.h"
#include "ae_ctrl.h"
#include "flash.h"
#include "isp_debug.h"
#include "sprd_hdr_api.h"
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
#include "sensor_exposure_queue.h"
#include "isp_adpt.h"
#include "ae_debug_info_parser.h"

#define AE_UPDATE_BASE_EOF 0
#define AE_UPDATE_BASE_SOF 0
#define AE_UPDAET_BASE_OFFSET AE_UPDATE_BASE_SOF
#define AE_TUNING_VER 1

#define AE_START_ID 0x71717567
#define AE_END_ID 	0x69656E64

#define AE_EXP_GAIN_PARAM_FILE_NAME_CAMERASERVER "/data/vendor/cameraserver/ae.file"
#define AE_EXP_GAIN_PARAM_FILE_NAME_MEDIA "/data/misc/media/ae.file"
#define AE_SAVE_MLOG     "persist.vendor.cam.isp.ae.mlog"
#define AE_SAVE_MLOG_DEFAULT ""
#define SENSOR_LINETIME_BASE   100	/*temp macro for flash, remove later, Andy.lin */
#define AE_FLASH_CALC_TIMES	60	/* prevent flash_pfOneIteration time out */
#define AE_THREAD_QUEUE_NUM		(50)
const char AE_MAGIC_TAG[] = "ae_debug_info";

//Dynamic dualcam AE_sync debug
#define DYNAMIC_AE_SYNC  1 // 1:enable dynamic AE_sync; 0:disable dynamic AE_sync
#define AEM_MASTER_STAT_FILE "/data/vendor/cameraserver/aem_master.file"
#define AEM_SLAVE_STAT_FILE "/data/vendor/cameraserver/aem_slave.file"
#define AEM_Y_STAT_FILE "/data/vendor/cameraserver/aem_y.file"
#define EPSINON 0.00000001
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
static cmr_s32 ae_set_pause(struct ae_ctrl_cxt *cxt);
static cmr_s32 ae_set_restore_cnt(struct ae_ctrl_cxt *cxt);
static cmr_s32 ae_set_force_pause(struct ae_ctrl_cxt *cxt, cmr_u32 enable);
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
static cmr_s32 ae_update_exp_data(struct ae_ctrl_cxt *cxt, struct ae_sensor_exp_data *exp_data, struct q_item *write_item, struct q_item *actual_item, cmr_u32 is_force)
{
	float sensor_gain = 0.0;
	float isp_gain = 0.0;
	cmr_u32 max_gain = cxt->sensor_max_gain;
	cmr_u32 min_gain = cxt->sensor_min_gain;

	if (exp_data->lib_data.gain > max_gain) {	/*gain : (sensor max gain, ~) */
		sensor_gain = max_gain;
		isp_gain = (double)exp_data->lib_data.gain / (double)max_gain;
	} else if (exp_data->lib_data.gain > min_gain) {	/*gain : (sensor_min_gain, sensor_max_gain) */
		if (0 == exp_data->lib_data.gain % cxt->sensor_gain_precision) {
			sensor_gain = exp_data->lib_data.gain;
			isp_gain = 1.0;
		} else {
			sensor_gain = (exp_data->lib_data.gain / cxt->sensor_gain_precision) * cxt->sensor_gain_precision;
			isp_gain = exp_data->lib_data.gain * 1.0 / sensor_gain;
		}
	} else {					/*gain : (~, sensor_min_gain) */
		sensor_gain = min_gain;
		isp_gain = (double)exp_data->lib_data.gain / (double)min_gain;
		if (isp_gain < 1.0) {
			ISP_LOGE("check sensor_cfg.min_gain %.2f %.2f", sensor_gain, isp_gain);
			isp_gain = 1.0;
		}
	}
	//ISP_LOGI("calc param: %f, %f, %d, sensor max gain%d\n", sensor_gain, isp_gain, (exp_data->lib_data.gain % cxt->sensor_gain_precision), max_gain);
	exp_data->lib_data.sensor_gain = sensor_gain;
	exp_data->lib_data.isp_gain = (isp_gain * 4096.0 + 0.5);

	if (is_force) {
		/**/ struct s_q_init_in init_in;
		struct s_q_init_out init_out;

		init_in.exp_line = exp_data->lib_data.exp_line;
		init_in.exp_time = exp_data->lib_data.exp_time;
		init_in.dmy_line = exp_data->lib_data.dummy;
		init_in.sensor_gain = exp_data->lib_data.sensor_gain;
		init_in.isp_gain = exp_data->lib_data.isp_gain;
		s_q_init(cxt->seq_handle, &init_in, &init_out);
		//*write_item = init_out.write_item;
		write_item->exp_line = init_in.exp_line;
		write_item->exp_time = init_in.exp_time;
		write_item->sensor_gain = init_in.sensor_gain;
		write_item->isp_gain = init_in.isp_gain;
		write_item->dumy_line = init_in.dmy_line;
		*actual_item = *write_item;
	} else {
		struct q_item input_item;

		input_item.exp_line = exp_data->lib_data.exp_line;
		input_item.exp_time = exp_data->lib_data.exp_time;
		input_item.dumy_line = exp_data->lib_data.dummy;
		input_item.sensor_gain = exp_data->lib_data.sensor_gain;
		input_item.isp_gain = exp_data->lib_data.isp_gain;
		s_q_put(cxt->seq_handle, &input_item, write_item, actual_item);
	}

	ISP_LOGV("type: %d, lib_out:(%d, %d, %d, %d)--write: (%d, %d, %d, %d)--actual: (%d, %d, %d, %d)\n",
			 is_force, exp_data->lib_data.exp_line, exp_data->lib_data.dummy, exp_data->lib_data.sensor_gain, exp_data->lib_data.isp_gain,
			 write_item->exp_line, write_item->dumy_line, write_item->sensor_gain, write_item->isp_gain,
			 actual_item->exp_line, actual_item->dumy_line, actual_item->sensor_gain, actual_item->isp_gain);

	return ISP_SUCCESS;

}

#ifndef CONFIG_ISP_2_2
static cmr_s32 ae_sync_write_to_sensor(struct ae_ctrl_cxt *cxt, struct ae_exposure_param *write_param)
{
	struct ae_exposure_param *prv_param = &cxt->exp_data.write_data;
	struct sensor_multi_ae_info ae_info[2];
	struct sensor_info info_master;
	struct sensor_info info_slave;
	struct ae_match_data ae_match_data_master;
	struct ae_match_data ae_match_data_slave;
	struct sensor_otp_ae_info ae_otp_master;
	struct sensor_otp_ae_info ae_otp_slave;
	cmr_u32 exp_line_slave;

	if (0 != write_param->exp_line && 0 != write_param->sensor_gain) {
		cmr_s32 size_index = cxt->snr_info.sensor_size_index;

		if ((write_param->exp_line != prv_param->exp_line)
			|| (write_param->dummy != prv_param->dummy)
			|| (prv_param->sensor_gain != write_param->sensor_gain)) {
			memset(&ae_info, 0, sizeof(ae_info));
			ae_info[0].count = 2;
			ae_info[0].exp.exposure = write_param->exp_line;
			ae_info[0].exp.dummy = write_param->dummy;
			ae_info[0].exp.size_index = size_index;
			ae_info[0].gain = write_param->sensor_gain & 0xffff;
			ISP_LOGV("ae_info[0] exposure %d dummy %d size_index %d gain %d", ae_info[0].exp.exposure, ae_info[0].exp.dummy, ae_info[0].exp.size_index, ae_info[0].gain);

			cxt->ptr_isp_br_ioctrl(CAM_SENSOR_MASTER, GET_MODULE_INFO, NULL, &info_master);
			cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE0, GET_MODULE_INFO, NULL, &info_slave);
			cxt->ptr_isp_br_ioctrl(CAM_SENSOR_MASTER, GET_OTP_AE, NULL, &ae_otp_master);
			cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE0, GET_OTP_AE, NULL, &ae_otp_slave);
			ISP_LOGV("master linetime %d", info_master.line_time);
			ISP_LOGV("slave linetime %d", info_slave.line_time);

			ISP_LOGV("AE@OTP master %d, slave %d", (int)ae_otp_master.gain_1x_exp, (int)ae_otp_slave.gain_1x_exp);

			if (ae_info[0].exp.exposure < (cmr_u32) info_master.min_exp_line) {
				ae_info[0].exp.exposure = (cmr_u32) info_master.min_exp_line;
			}

			cmr_u32 exposure_time = ae_info[0].exp.exposure * info_master.line_time;
			if (info_slave.line_time != 0) {
				exp_line_slave = exposure_time / info_slave.line_time;
			} else {
				exp_line_slave = ae_info[0].exp.exposure;
			}

			if ((ae_otp_master.gain_1x_exp == 0) || (ae_otp_slave.gain_1x_exp == 0)) {
				char value[PROPERTY_VALUE_MAX];
				ae_otp_master.gain_1x_exp = 10000;
				ae_otp_slave.gain_1x_exp = 7000;
				property_get("persist.vendor.cam.isp.ae.otp.master", value, "10000");
				if (strcmp(value, "0") != 0) {
					ae_otp_master.gain_1x_exp = atoi(value);
				}
				property_get("persist.vendor.cam.isp.ae.otp.slave", value, "7000");
				if (strcmp(value, "0") != 0) {
					ae_otp_slave.gain_1x_exp = atoi(value);
				}
			}
			ISP_LOGV("AE@OTP master %d, slave %d", (int)ae_otp_master.gain_1x_exp, (int)ae_otp_slave.gain_1x_exp);

			float iso_ratio = ((float)ae_otp_slave.gain_1x_exp) / ((float)ae_otp_master.gain_1x_exp);
			if (ae_info[0].gain > 128 * 8) {
				if ((ae_otp_master.gain_8x_exp != 0) && (ae_otp_slave.gain_8x_exp != 0)) {
					iso_ratio = ((float)ae_otp_slave.gain_8x_exp) / ((float)ae_otp_master.gain_8x_exp);
				}
			} else if (ae_info[0].gain > 128 * 4) {
				if ((ae_otp_master.gain_4x_exp != 0) && (ae_otp_slave.gain_4x_exp != 0)) {
					iso_ratio = ((float)ae_otp_slave.gain_4x_exp) / ((float)ae_otp_master.gain_4x_exp);
				}
			} else if (ae_info[0].gain > 128 * 2) {
				if ((ae_otp_master.gain_2x_exp != 0) && (ae_otp_slave.gain_2x_exp != 0)) {
					iso_ratio = ((float)ae_otp_slave.gain_2x_exp) / ((float)ae_otp_master.gain_2x_exp);
				}
			}

			if (iso_ratio < 1.0f) {
				if ((cmr_u32) (ae_info[0].gain * iso_ratio) > 128) {
					ae_info[1].exp.exposure = exp_line_slave;
					ae_info[1].gain = (cmr_u32) (ae_info[0].gain * iso_ratio + 0.5f);
				} else {
					ae_info[1].exp.exposure = (cmr_u32) (exp_line_slave * iso_ratio);
					if (ae_info[1].exp.exposure >= (cmr_u32) info_slave.min_exp_line) {
						ae_info[1].gain = (cmr_u32) (ae_info[0].gain * exp_line_slave * iso_ratio / ae_info[1].exp.exposure + 0.5f);
					} else {
						ae_info[1].exp.exposure = (cmr_u32) info_slave.min_exp_line;
						ae_info[1].gain = (cmr_u32) (ae_info[0].gain * ae_info[0].exp.exposure * info_master.line_time * iso_ratio / (ae_info[1].exp.exposure * info_slave.line_time) + 0.5f);

						if (ae_info[1].gain < 128) {	// master should sync with slave
							ae_info[0].gain = (cmr_u32) (ae_info[0].gain * 128.0f / ae_info[1].gain + 0.5f);
							ae_info[1].gain = 128;
						}
					}
				}
			} else {
				ae_info[1].exp.exposure = exp_line_slave;
				ae_info[1].gain = (cmr_u32) (ae_info[0].gain * iso_ratio + 0.5f);
			}

			ae_info[1].exp.dummy = write_param->dummy;
			ae_info[1].exp.size_index = size_index;
			ISP_LOGV("ae_info[1] exposure %d dummy %d size_index %d gain %d", ae_info[1].exp.exposure, ae_info[1].exp.dummy, ae_info[1].exp.size_index, ae_info[1].gain);

			if (cxt->isp_ops.write_multi_ae) {
				(*cxt->isp_ops.write_multi_ae) (cxt->isp_ops.isp_handler, ae_info);
			} else {
				ISP_LOGV("write_multi_ae is NULL");
			}
			ae_match_data_master.exp = ae_info[0].exp;
			ae_match_data_master.gain = ae_info[0].gain;
			ae_match_data_master.isp_gain = write_param->isp_gain;
			ae_match_data_slave.exp = ae_info[1].exp;
			ae_match_data_slave.gain = ae_info[1].gain;
			ae_match_data_slave.isp_gain = write_param->isp_gain;
			cxt->ptr_isp_br_ioctrl(CAM_SENSOR_MASTER, SET_MATCH_AE_DATA, &ae_match_data_master, NULL);
			cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE0, SET_MATCH_AE_DATA, &ae_match_data_slave, NULL);
		}
	} else {
		ISP_LOGE("exp data are invalidate: exp: %d, gain: %d\n", write_param->exp_line, write_param->gain);
	}

	if (0 != write_param->isp_gain) {
		double rgb_coeff = write_param->isp_gain * 1.0 / 4096;
		if (cxt->isp_ops.set_rgb_gain) {
			cxt->isp_ops.set_rgb_gain(cxt->isp_ops.isp_handler, rgb_coeff);
		}
	}

	cxt->ptr_isp_br_ioctrl(CAM_SENSOR_MASTER, SET_MATCH_BV_DATA, &cxt->cur_result.cur_bv, NULL);
	cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE0, SET_MATCH_BV_DATA, &cxt->cur_result.cur_bv, NULL);

	return ISP_SUCCESS;
}


static cmr_s32 ae_sync_write_to_sensor_normal(struct ae_ctrl_cxt *cxt, struct ae_exposure_param *write_param)
{
	struct ae_exposure_param *prv_param = &cxt->exp_data.write_data;
	struct sensor_multi_ae_info ae_info[2];
	struct sensor_info info_master;
	struct sensor_info info_slave;
	struct ae_match_data ae_match_data_master;
	struct ae_match_data ae_match_data_slave;
	struct sensor_otp_ae_info ae_otp_master;
	struct sensor_otp_ae_info ae_otp_slave;
	struct ae_misc_sync_in master_param = {0};
	struct ae_misc_sync_out slave_param = {0};
	struct ae_misc_sync_param in_param = {0};
	struct ae_sync_info master_ae_sync_info,  slave_ae_sync_info;
	struct ae_sync_para ae_sync_param;
	cmr_u32 ae_dynamic_flag;

	ISP_LOGV(" AE_sync mode:0x%x.\n",cxt->ae_sync_param.mode);

	if (cxt->ae_sync_param.mode)
		ae_dynamic_flag = 1;
	else
		ae_dynamic_flag = 0;


	if (0 != write_param->exp_line && 0 != write_param->sensor_gain) {
		cmr_s32 size_index = cxt->snr_info.sensor_size_index;

		if ((write_param->exp_line != prv_param->exp_line)
			|| (write_param->dummy != prv_param->dummy)
			|| (prv_param->sensor_gain != write_param->sensor_gain) || (ae_dynamic_flag == 1)) {
			memset(&ae_info, 0, sizeof(ae_info));
			ae_info[0].count = 2;
			ae_info[0].exp.exposure = write_param->exp_line;
			ae_info[0].exp.dummy = write_param->dummy;
			ae_info[0].exp.size_index = size_index;
			ae_info[0].gain = write_param->sensor_gain & 0xffff;
			ISP_LOGV("(sharkl3)normal mode:ae_info[0] exposure %d dummy %d size_index %d gain %d", ae_info[0].exp.exposure, ae_info[0].exp.dummy, ae_info[0].exp.size_index, ae_info[0].gain);

			cxt->ptr_isp_br_ioctrl(CAM_SENSOR_MASTER, GET_MODULE_INFO, NULL, &info_master);
			cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE0, GET_MODULE_INFO, NULL, &info_slave);
			cxt->ptr_isp_br_ioctrl(CAM_SENSOR_MASTER, GET_OTP_AE, NULL, &ae_otp_master);
			cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE0, GET_OTP_AE, NULL, &ae_otp_slave);
			ISP_LOGV("(sharkl3)normal mode:master linetime %d", info_master.line_time);
			ISP_LOGV("(sharkl3)normal mode:slave linetime %d", info_slave.line_time);

			if (ae_info[0].exp.exposure < (cmr_u32) info_master.min_exp_line) {
				ae_info[0].exp.exposure = (cmr_u32) info_master.min_exp_line;
			}

			
			cmr_u32 master_sync_aem[3 * 1024] = {0};
			cmr_u32 slave_sync_aem[3 * 1024] = {0};
			cmr_u32 rtn;

			if(cxt->is_multi_mode )
			{
				rtn = cxt->ptr_isp_br_ioctrl(CAM_SENSOR_MASTER, GET_AEM_SYNC_STAT, NULL,master_sync_aem);
				if(rtn){
					ISP_LOGE("(sharkl3)norma Dynamic AE_sync y_ratio  master_sync_aem is NULL error!");
				}

				rtn = cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE0, GET_AEM_SYNC_STAT, NULL,slave_sync_aem);
				if(rtn){
					ISP_LOGE("(sharkl3)norma Dynamic AE_sync y_ratio slave_sync_aem is NULL error!");
				}
			}

			memset(&master_ae_sync_info,0,sizeof(struct ae_sync_info));
			memset(&slave_ae_sync_info,0,sizeof(struct ae_sync_info));
			memset(&ae_sync_param,0,sizeof(struct ae_sync_para));

			master_ae_sync_info.line_time = info_master.line_time;
			master_ae_sync_info.max_again = info_master.max_again;
			master_ae_sync_info.min_again = info_master.min_again;
			master_ae_sync_info.min_exp_line =info_master.min_exp_line;
			master_ae_sync_info.sensor_gain_precision = info_master.sensor_gain_precision;
			master_ae_sync_info.exposure = write_param->exp_line;
			master_ae_sync_info.gain =  write_param->sensor_gain & 0xffff;
			memcpy(master_ae_sync_info.aem,master_sync_aem,3 * 1024 * sizeof(cmr_u32));
			memcpy(&master_ae_sync_info.ae_otp_info,&ae_otp_master,sizeof(struct sensor_otp_ae_info));

			slave_ae_sync_info.line_time = info_slave.line_time;
			slave_ae_sync_info.max_again = info_slave.max_again;
			slave_ae_sync_info.min_again = info_slave.min_again;
			slave_ae_sync_info.min_exp_line =info_slave.min_exp_line;
			slave_ae_sync_info.sensor_gain_precision = info_slave.sensor_gain_precision;
			memcpy(slave_ae_sync_info.aem,slave_sync_aem,3 * 1024 * sizeof(cmr_u32));
			memcpy(&slave_ae_sync_info.ae_otp_info,&ae_otp_slave,sizeof(struct sensor_otp_ae_info));

			memcpy(&ae_sync_param,&cxt->ae_sync_param,sizeof(struct ae_sync_para));

			master_param.master_ae_info = &master_ae_sync_info;
			slave_param.slave_ae_info = &slave_ae_sync_info;
			in_param.ae_sync_param = &ae_sync_param;

			ae_misc_sync_calculation(cxt->misc_handle,&in_param, &master_param, &slave_param);

			ae_info[1].exp.exposure =slave_ae_sync_info.exposure;
			ae_info[1].gain =slave_ae_sync_info.gain;
			ae_info[1].exp.dummy = (ae_info[0].exp.exposure + ae_info[0].exp.dummy)* info_master.line_time / info_slave.line_time - ae_info[1].exp.exposure;
			ae_info[1].exp.size_index = 2;
			ISP_LOGV("(sharkl3)normal mode:ae_info[1] exposure %d dummy %d size_index %d gain %d", ae_info[1].exp.exposure, ae_info[1].exp.dummy, ae_info[1].exp.size_index, ae_info[1].gain);

			if (cxt->isp_ops.write_multi_ae) {
				(*cxt->isp_ops.write_multi_ae) (cxt->isp_ops.isp_handler, ae_info);
			} else {
				ISP_LOGV("(sharkl3)normal mode:write_multi_ae is NULL");
			}
			ae_match_data_master.exp = ae_info[0].exp;
			ae_match_data_master.gain = ae_info[0].gain;
			ae_match_data_master.isp_gain = write_param->isp_gain;
			ae_match_data_slave.exp = ae_info[1].exp;
			ae_match_data_slave.gain = ae_info[1].gain;
			ae_match_data_slave.isp_gain = write_param->isp_gain;
			cxt->ptr_isp_br_ioctrl(CAM_SENSOR_MASTER, SET_MATCH_AE_DATA, &ae_match_data_master, NULL);
			cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE0, SET_MATCH_AE_DATA, &ae_match_data_slave, NULL);
			ISP_LOGV("(sharkl3)normal mode:master: exp: %d, gain: %d\n", ae_match_data_master.exp.exposure, ae_match_data_master.gain);
			ISP_LOGV("(sharkl3)normal mode:slave: exp: %d, gain: %d\n", ae_match_data_slave.exp.exposure, ae_match_data_slave.gain);
		}
	} else {
		ISP_LOGE("(sharkl3)normal mode:exp data are invalidate: exp: %d, gain: %d\n", write_param->exp_line, write_param->gain);
	}

	if (0 != write_param->isp_gain) {
		double rgb_coeff = write_param->isp_gain * 1.0 / 4096;
		if (cxt->isp_ops.set_rgb_gain) {
			cxt->isp_ops.set_rgb_gain(cxt->isp_ops.isp_handler, rgb_coeff);
		}
	}

	cxt->ptr_isp_br_ioctrl(CAM_SENSOR_MASTER, SET_MATCH_BV_DATA, &cxt->cur_result.cur_bv, NULL);
	cxt->ptr_isp_br_ioctrl(CAM_SENSOR_SLAVE0, SET_MATCH_BV_DATA, &cxt->cur_result.cur_bv, NULL);

	return ISP_SUCCESS;
}
#endif

static cmr_s32 ae_write_to_sensor(struct ae_ctrl_cxt *cxt, struct ae_exposure_param *write_param_ptr)
{
	struct ae_exposure_param tmp_param = *write_param_ptr;
	struct ae_exposure_param *write_param = &tmp_param;
	struct ae_exposure_param *prv_param = &cxt->exp_data.write_data;

	if ((cxt->zsl_flag == 0) && (cxt->binning_factor_before > cxt->binning_factor_after)) {
		tmp_param.exp_time = (cmr_u32) (1.0 * tmp_param.exp_time * cxt->binning_factor_before / cxt->binning_factor_after + 0.5);
		tmp_param.exp_line = (cmr_u32) (1.0 * tmp_param.exp_line * cxt->binning_factor_before / cxt->binning_factor_after + 0.5);
	}
	ISP_LOGV("exp_line %d, binning_factor_before %d, binning_factor_after %d, zsl_flag %d", tmp_param.exp_line, cxt->binning_factor_before, cxt->binning_factor_after, cxt->zsl_flag);
	if (0 != write_param->exp_line) {
		struct ae_exposure exp;
		cmr_s32 size_index = cxt->snr_info.sensor_size_index;
		if (cxt->isp_ops.ex_set_exposure) {
			memset(&exp, 0, sizeof(exp));
			exp.exposure = write_param->exp_line;
			exp.dummy = write_param->dummy;
			exp.size_index = size_index;
			if ((write_param->exp_line != prv_param->exp_line)
				|| (write_param->dummy != prv_param->dummy)) {
				(*cxt->isp_ops.ex_set_exposure) (cxt->isp_ops.isp_handler, &exp);
#ifdef CONFIG_ISP_2_2
				cmr_u64 exp_time = 0;
				cmr_int cb_type;
				exp_time = (cmr_u64) write_param->exp_time;
				cb_type = AE_CB_EXPTIME_NOTIFY;
				(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, cb_type, &exp_time);
#endif
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

static cmr_s32 ae_update_result_to_sensor(struct ae_ctrl_cxt *cxt, struct ae_sensor_exp_data *exp_data, cmr_u32 is_force)
{
	cmr_s32 ret = ISP_SUCCESS;
	struct ae_exposure_param write_param = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	struct q_item write_item = { 0, 0, 0, 0, 0 };
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

	cxt->glb_gain = (cmr_u32) (write_item.isp_gain * 1.0 / 4096 * cxt->backup_rgb_gain + 0.5);

	if (cxt->is_multi_mode == ISP_ALG_DUAL_SBS) {
#ifndef CONFIG_ISP_2_2
		if (cxt->is_master)
			ae_sync_write_to_sensor(cxt, &write_param);
#endif
	}
	else if ((cxt->is_multi_mode == ISP_ALG_DUAL_C_C||cxt->is_multi_mode ==ISP_ALG_DUAL_W_T||cxt->is_multi_mode ==ISP_ALG_DUAL_C_M)) {
#ifndef CONFIG_ISP_2_2
		if (cxt->is_master)
		ae_sync_write_to_sensor_normal(cxt, &write_param);
#else
		ae_write_to_sensor(cxt, &write_param);
#endif
	} else {
		ae_write_to_sensor(cxt, &write_param);
	}

	exp_data->write_data.exp_line = write_item.exp_line;
	exp_data->write_data.exp_time = write_item.exp_time;
	exp_data->write_data.dummy = write_item.dumy_line;
	exp_data->write_data.sensor_gain = write_item.sensor_gain;
	exp_data->write_data.isp_gain = write_item.isp_gain;

	exp_data->actual_data.exp_line = actual_item.exp_line;
	exp_data->actual_data.exp_time = actual_item.exp_time;
	exp_data->actual_data.dummy = actual_item.dumy_line;
	exp_data->actual_data.sensor_gain = actual_item.sensor_gain;
	exp_data->actual_data.isp_gain = actual_item.isp_gain;

	if (write_item.isp_gain && cxt->cam_4in1_mode && !cxt->cam_4in1_cap_flag) {
		double rgb_coeff = write_item.isp_gain * 1.0 / 4096 * 4;
		if (cxt->isp_ops.set_rgb_gain_4in1) {
			cxt->isp_ops.set_rgb_gain_4in1(cxt->isp_ops.isp_handler, rgb_coeff);
		}
	}
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
	cmr_u32 sensor_maxfps = cxt->cur_status.snr_max_fps;
	double product_max = 1.0 * cxt->cur_status.ae_table->exposure[cxt->cur_status.ae_table->max_index]
		* cxt->cur_status.line_time * cxt->cur_status.ae_table->again[cxt->cur_status.ae_table->max_index];
	max_gain = cxt->cur_status.ae_table->again[cxt->cur_status.ae_table->max_index];

	ISP_LOGV("max:src %.f, dst %.f, max index: %d\n", product, product_max, cxt->cur_status.ae_table->max_index);

	if (product_max < product) {
		product = product_max;
	}
	*dst_exp_param = *src_exp_param;
	if (AE_FLICKER_50HZ == cxt->cur_status.settings.flicker) {
		divisor_coeff = 1.0 / 100.0;
	} else if (AE_FLICKER_60HZ == cxt->cur_status.settings.flicker) {
		divisor_coeff = 1.0 / 120.0;
	} else {
		divisor_coeff = 1.0 / 100.0;
	}

	if (tmp_mn < 1.0 * AEC_LINETIME_PRECESION / (max_exp * cxt->cur_status.line_time)) {
		tmp_mn = 1.0 * AEC_LINETIME_PRECESION / (max_exp * cxt->cur_status.line_time);
	}

	cur_fps = 1.0 * AEC_LINETIME_PRECESION / src_exp_param->exp_time;
	if (cur_fps > sensor_maxfps) {
		cur_fps = sensor_maxfps;
	}

	if ((cmr_u32) (cur_fps + 0.5) >= (cmr_u32) (tmp_mn + 0.5)) {
		exp_cnts = (cmr_u32) (1.0 * src_exp_param->exp_time / (AEC_LINETIME_PRECESION * divisor_coeff) + 0.5);
		if (exp_cnts > 0) {
			dst_exp_param->exp_line = (cmr_u32) ((exp_cnts * divisor_coeff) * 1.0 * AEC_LINETIME_PRECESION / cxt->cur_status.line_time + 0.5);
			dst_exp_param->gain = (cmr_u32) (product / cxt->cur_status.line_time / dst_exp_param->exp_line + 0.5);
			if (dst_exp_param->gain < 128) {
				cmr_s32 tmp_gain = 0;
				for (i = exp_cnts - 1; i > 0; i--) {
					tmp_gain = (cmr_s32) (product / (1.0 * AEC_LINETIME_PRECESION * (i * divisor_coeff)) + 0.5);
					if (tmp_gain >= 128) {
						break;
					}
				}
				if (i > 0) {
					dst_exp_param->exp_line = (cmr_s16) ((i * divisor_coeff) * 1.0 * AEC_LINETIME_PRECESION / cxt->cur_status.line_time + 0.5);
					dst_exp_param->gain = (cmr_s16) (product / (dst_exp_param->exp_line * cxt->cur_status.line_time) + 0.5);
				} else {
					dst_exp_param->gain = 128;
					dst_exp_param->exp_line = (cmr_s16) (product / (dst_exp_param->gain * cxt->cur_status.line_time) + 0.5);
				}
			} else if (dst_exp_param->gain > max_gain) {
				dst_exp_param->gain = max_gain;
			}
		} else {
			/*due to the exposure time is smaller than 1/100 or 1/120, exp time do not need to adjust */
			dst_exp_param->exp_line = src_exp_param->exp_line;
			dst_exp_param->gain = src_exp_param->gain;
		}
		tmp_fps = 1.0 * AEC_LINETIME_PRECESION / (dst_exp_param->exp_line * cxt->cur_status.line_time);
		if (tmp_fps > sensor_maxfps) {
			tmp_fps = sensor_maxfps;
		}
		dst_exp_param->dummy = 0;
		if ((cmr_u32) (tmp_fps + 0.5) >= (cmr_u32) (tmp_mx + 0.5)) {
			if (tmp_mx < sensor_maxfps) {
				dummy = (cmr_u32) (1.0 * AEC_LINETIME_PRECESION / (cxt->cur_status.line_time * tmp_mx) - dst_exp_param->exp_line + 0.5);
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
			dst_exp_param->exp_line = (cmr_u32) ((exp_cnts * divisor_coeff) * 1.0 * AEC_LINETIME_PRECESION / cxt->cur_status.line_time + 0.5);
			dst_exp_param->gain = (cmr_u32) (product / cxt->cur_status.line_time / dst_exp_param->exp_line + 0.5);
			if (dst_exp_param->gain < 128) {
				cmr_s32 tmp_gain = 0;
				for (i = exp_cnts - 1; i > 0; i--) {
					tmp_gain = (cmr_s32) (product / (1.0 * AEC_LINETIME_PRECESION * (i * divisor_coeff)) + 0.5);
					if (tmp_gain >= 128) {
						break;
					}
				}
				if (i > 0) {
					dst_exp_param->exp_line = (cmr_s16) ((i * divisor_coeff) * 1.0 * AEC_LINETIME_PRECESION / cxt->cur_status.line_time + 0.5);
					dst_exp_param->gain = (cmr_s16) (product / (dst_exp_param->exp_line * cxt->cur_status.line_time) + 0.5);
				} else {
					dst_exp_param->gain = 128;
					dst_exp_param->exp_line = (cmr_s16) (product / (dst_exp_param->gain * cxt->cur_status.line_time) + 0.5);
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
		tmp_fps = 1.0 * AEC_LINETIME_PRECESION / (dst_exp_param->exp_line * cxt->cur_status.line_time);
		if (tmp_fps > sensor_maxfps) {
			tmp_fps = sensor_maxfps;
		}
		if ((cmr_u32) (tmp_fps + 0.5) >= (cmr_u32) (tmp_mx + 0.5)) {
			if (tmp_mx < sensor_maxfps) {
				dummy = (cmr_u32) (1.0 * AEC_LINETIME_PRECESION / (cxt->cur_status.line_time * tmp_mx) - dst_exp_param->exp_line + 0.5);
			}
		}

		if (dummy < 0) {
			dst_exp_param->dummy = 0;
		} else {
			dst_exp_param->dummy = dummy;
		}
	}

	dst_exp_param->exp_time = dst_exp_param->exp_line * cxt->cur_status.line_time;

	if (0 == cxt->cur_status.line_time)
		ISP_LOGE("Can't receive line_time from drvier!");

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
	struct ae_alg_calc_result *result_ptr;
	struct ae_alg_calc_param *sync_cur_status_ptr;

	sync_cur_status_ptr = &(cxt_ptr->sync_cur_status);
	result_ptr = &cxt_ptr->sync_cur_result;

	fps = AEC_LINETIME_PRECESION / (cxt_ptr->snr_info.line_time * (result_ptr->wts.cur_dummy + result_ptr->wts.cur_exp_line));
	if (fps > cxt_ptr->sync_cur_status.settings.max_fps) {
		fps = cxt_ptr->sync_cur_status.settings.max_fps;
	}

	pos =
		sprintf(log_str, "cam-id:%d frm-id:%d,flicker: %d\nidx(%d-%d):%d,cur-l:%d, tar-l:%d, bv(lv):%d, cali_bv: %d,expl(%d):%d, expt: %d, gain:%d, dmy:%d, FR(%d-%d):%.2f\n",
				cxt_ptr->camera_id, sync_cur_status_ptr->frame_id, sync_cur_status_ptr->settings.flicker, sync_cur_status_ptr->ae_table->min_index,
				sync_cur_status_ptr->ae_table->max_index, result_ptr->wts.cur_index, cxt_ptr->sync_cur_result.cur_lum, cxt_ptr->sync_cur_result.target_lum,
				cxt_ptr->cur_result.cur_bv, cxt_ptr->cur_result.cur_bv_nonmatch, cxt_ptr->snr_info.line_time, result_ptr->wts.cur_exp_line,
				result_ptr->wts.cur_exp_line * cxt_ptr->snr_info.line_time, result_ptr->wts.cur_again, result_ptr->wts.cur_dummy,
				cxt_ptr->sync_cur_status.settings.min_fps, cxt_ptr->sync_cur_status.settings.max_fps, fps);

	if (result_ptr->log) {
		pos += sprintf((char *)((char *)log_str + pos), "adv info:\n%s\n", (char *)result_ptr->log);
	}
}

static cmr_s32 ae_save_to_mlog_file(struct ae_ctrl_cxt *cxt, struct ae_misc_calc_out *result)
{
	cmr_s32 rtn = 0;
	char *tmp_str = (char *)cxt->debug_str;
	UNUSED(result);

	memset(tmp_str, 0, sizeof(cxt->debug_str));
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

static cmr_s32 ae_unpack_tunning_param(cmr_handle param, cmr_u32 size, struct ae_tuning_param *tuning_param)
{
	cmr_u32 *tmp = param;
	cmr_u32 version = 0;
	cmr_u32 verify = 0;

	UNUSED(size);

	if (NULL == param)
		return AE_ERROR;

	version = *tmp++;
	verify = *tmp++;
	if (AE_PARAM_VERIFY != verify || AE_TUNING_VER != version) {
		ISP_LOGE("fail to unpack param, verify=0x%x, version=%d", verify, version);
		return AE_ERROR;
	}

	memcpy(tuning_param, param, sizeof(struct ae_tuning_param));

	return AE_SUCCESS;
}

static cmr_u32 ae_calc_target_lum(cmr_u32 cur_target_lum, enum ae_level level, struct ae_ev_table *ev_table)
{
	cmr_s32 target_lum = 0;

	if (NULL == ev_table) {
		ISP_LOGE("fail to calc tar lum, table %p", ev_table);
		return target_lum;
	}

	if (ev_table->diff_num >= AE_EV_LEVEL_NUM)
		ev_table->diff_num = AE_EV_LEVEL_NUM - 1;

	if (level >= ev_table->diff_num)
		level = ev_table->diff_num - 1;

	ISP_LOGD("cur target lum=%d, ev diff=%d, level=%d", cur_target_lum, ev_table->lum_diff[level], level);

	target_lum = (cmr_s32) cur_target_lum + ev_table->lum_diff[level];
	target_lum = (target_lum < 0) ? 0 : target_lum;

	return (cmr_u32) target_lum;
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

#if 0
static cmr_s32 ae_get_iso(struct ae_ctrl_cxt *cxt, cmr_s32 * real_iso)
{
	cmr_s32 rtn = AE_SUCCESS;
	cmr_s32 iso = 0;
	cmr_s32 calc_iso = 0;
	cmr_s32 tmp_iso = 0;
	cmr_s32 real_gain = 0;
	cmr_s32 iso_shutter = 0;
	float f_tmp = 0;

	cmr_s32(*map)[15] = 0;

	if (NULL == cxt || NULL == real_iso) {
		ISP_LOGE("fail to get iso, cxt %p real_iso %p", cxt, real_iso);
		return AE_ERROR;
	}

	iso = cxt->cur_status.settings.iso;
	real_gain = cxt->cur_result.wts.cur_again;
	f_tmp = AEC_LINETIME_PRECESION / cxt->cur_result.wts.exposure_time;

	if (0.5 <= (f_tmp - (cmr_s32) f_tmp))
		iso_shutter = (cmr_s32) f_tmp + 1;
	else
		iso_shutter = (cmr_s32) f_tmp;
#if 0
	ISP_LOGV("iso_check %d", cxt->cur_status.frame_id % 20);
	ISP_LOGV("iso_check %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d", cxt->actual_cell[0].gain, cxt->actual_cell[1].gain, cxt->actual_cell[2].gain,
			 cxt->actual_cell[3].gain, cxt->actual_cell[4].gain, cxt->actual_cell[5].gain, cxt->actual_cell[6].gain, cxt->actual_cell[7].gain, cxt->actual_cell[8].gain,
			 cxt->actual_cell[9].gain, cxt->actual_cell[10].gain, cxt->actual_cell[11].gain, cxt->actual_cell[12].gain, cxt->actual_cell[13].gain,
			 cxt->actual_cell[14].gain, cxt->actual_cell[15].gain, cxt->actual_cell[16].gain, cxt->actual_cell[17].gain, cxt->actual_cell[18].gain, cxt->actual_cell[19].gain);
#endif
	if (AE_ISO_AUTO == iso) {
		switch (iso_shutter) {
		case 17:
			map = iso_shutter_mapping[0];
			break;
		case 20:
			map = iso_shutter_mapping[1];
			break;
		case 25:
			map = iso_shutter_mapping[2];
			break;
		case 33:
			map = iso_shutter_mapping[3];
			break;
		case 50:
			map = iso_shutter_mapping[4];
			break;
		case 100:
			map = iso_shutter_mapping[5];
			break;
		default:
			map = iso_shutter_mapping[6];
			break;
		}

		if (real_gain > *(*map + 14)) {
			calc_iso = 1250;
		} else if (real_gain > *(*map + 13)) {
			calc_iso = 1000;
		} else if (real_gain > *(*map + 12)) {
			calc_iso = 800;
		} else if (real_gain > *(*map + 11)) {
			calc_iso = 640;
		} else if (real_gain > *(*map + 10)) {
			calc_iso = 500;
		} else if (real_gain > *(*map + 9)) {
			calc_iso = 400;
		} else if (real_gain > *(*map + 8)) {
			calc_iso = 320;
		} else if (real_gain > *(*map + 7)) {
			calc_iso = 250;
		} else if (real_gain > *(*map + 6)) {
			calc_iso = 200;
		} else if (real_gain > *(*map + 5)) {
			calc_iso = 160;
		} else if (real_gain > *(*map + 4)) {
			calc_iso = 125;
		} else if (real_gain > *(*map + 3)) {
			calc_iso = 100;
		} else if (real_gain > *(*map + 2)) {
			calc_iso = 80;
		} else if (real_gain > *(*map + 1)) {
			calc_iso = 64;
		} else if (real_gain > *(*map)) {
			calc_iso = 50;
		} else {
			calc_iso = 50;
		}

	} else {
		calc_iso = (1 << (iso - 1)) * 100;
	}

	*real_iso = calc_iso;
	// ISP_LOGV("calc_iso iso_shutter %d\r\n", iso_shutter);
	// ISP_LOGV("calc_iso %d,real_gain %d,iso %d", calc_iso,
	// real_gain, iso);
	return rtn;
}
#else
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

	iso = cxt->cur_status.settings.iso;
	real_gain = cxt->cur_result.wts.cur_again;

	if (AE_ISO_AUTO == iso) {
		tmp_iso = real_gain * 5000 / 128;
		calc_iso = 0;
		if (tmp_iso < 890) {
			calc_iso = 0;
		} else if (tmp_iso < 1122) {
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
#endif

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

static cmr_s32 ae_get_lowlight_flag_by_bv(struct ae_ctrl_cxt *cxt, cmr_u32 * is_lowlight)
{
        cmr_s32 rtn = AE_SUCCESS;

        if (NULL == cxt || NULL == is_lowlight) {
                ISP_LOGE("fail to get is_lowlight by bv, cxt %p is_lowlight %p", cxt, is_lowlight);
                return AE_ERROR;
        }

        *is_lowlight = cxt->sync_cur_result.cur_bv < cxt->bv_thd ? 1:0;
        ISP_LOGV("get is_lowlight %d", *is_lowlight);
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
	cur_exp = cxt->cur_result.wts.exposure_time;
	real_gain = ae_get_real_gain(cxt->cur_result.wts.cur_again);

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
	cxt->cur_status.monitor_shift = 0;
	
	if (cxt->isp_ops.set_statistics_mode) {
		rtn = cxt->isp_ops.set_statistics_mode(cxt->isp_ops.isp_handler, (enum ae_statistics_mode)info.work_mode, info.skip_num);
	}
	if (cxt->isp_ops.set_monitor_win) {
		rtn = cxt->isp_ops.set_monitor_win(cxt->isp_ops.isp_handler, &info);
	}
	
	if (cxt->isp_ops.set_blk_num){
		rtn = cxt->isp_ops.set_blk_num(cxt->isp_ops.isp_handler, &info.win_num);
	}

	if (cxt->isp_ops.set_monitor) {
		rtn = cxt->isp_ops.set_monitor(cxt->isp_ops.isp_handler, info.skip_num);
	}

	ISP_LOGV("rtn=%d win_num(%dx%d), win_size(%dx%d) skip=%d mode=%d",rtn,info.win_num.w,info.win_num.h, info.win_size.w,info.win_size.h,info.skip_num,info.work_mode);
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
		goto EXIT;
	}

	rtn = ae_cfg_monitor_block(cxt);
  EXIT:
	return rtn;
}

static cmr_s32 ae_set_scaler_trim(struct ae_ctrl_cxt *cxt, struct ae_trim *trim)
{
	cmr_s32 rtn = AE_SUCCESS;

	if (trim) {
		rtn = ae_update_monitor_unit(cxt, trim);
		if (AE_SUCCESS != rtn) {
			goto EXIT;
		}
		rtn = ae_cfg_monitor_block(cxt);
	} else {
		ISP_LOGE("trim pointer is NULL\n");
		rtn = AE_ERROR;
	}
  EXIT:
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
		ISP_LOGI("ae_flash_status FLASH_PRE_BEFORE");
		cxt->cur_flicker = cxt->sync_cur_status.settings.flicker;	/*backup the flicker flag before flash
																	 * and will lock flicker result
																	 */
		cxt->flash_backup.exp_line = cxt->sync_cur_result.wts.cur_exp_line;
		cxt->flash_backup.exp_time = cxt->sync_cur_result.wts.cur_exp_line * cxt->cur_status.line_time;
		cxt->flash_backup.dummy = cxt->sync_cur_result.wts.cur_dummy;
		cxt->flash_backup.gain = cxt->sync_cur_result.wts.cur_again;
		cxt->flash_backup.line_time = cxt->cur_status.line_time;
		cxt->flash_backup.cur_index = cxt->sync_cur_result.wts.cur_index;
		cxt->flash_backup.bv = cxt->cur_result.cur_bv;
		if ((0 != cxt->flash_ver) && (0 == cxt->exposure_compensation.ae_compensation_flag))
			rtn = ae_set_force_pause_flash(cxt, 1);
		if (cxt->exposure_compensation.ae_compensation_flag)  {
			if(0==cxt->cur_status.settings.table_idx){
				struct ae_exp_compensation exp_comp;
				exp_comp.comp_val=0;
				exp_comp.comp_range.min=-16;
				exp_comp.comp_range.max=16;
				exp_comp.step_numerator=1;
				exp_comp.step_denominator=8;
				ae_set_exposure_compensation(cxt,&exp_comp);
			}
			ISP_LOGV("AE_FLASH_PRE_BEFORE store ae's table_idx : %d", cxt->flash_backup.table_idx);
		}
		cxt->flash_backup.table_idx = cxt->cur_status.settings.table_idx;
		cxt->send_once[0] = cxt->send_once[1] = cxt->send_once[2] = cxt->send_once[3] = cxt->send_once[4] = cxt->send_once[5] = 0;
		cxt->cur_status.settings.flash = FLASH_PRE_BEFORE;
		break;

	case AE_FLASH_PRE_LIGHTING:
		ISP_LOGI("ae_flash_status FLASH_PRE_LIGHTING");
		cxt->cur_status.settings.flash_target = cxt->cur_param->flash_param.target_lum;
		if (0 != cxt->flash_ver) {
			/*lock AE algorithm */
			cxt->pre_flash_skip = cxt->cur_param->flash_control_param.pre_flash_skip;
			cxt->aem_effect_delay = 0;
		}
		cxt->cur_status.settings.flash = FLASH_PRE;
		break;

	case AE_FLASH_PRE_AFTER:
		ISP_LOGI("ae_flash_status FLASH_PRE_AFTER");
		if (cxt->exposure_compensation.ae_compensation_flag) {
			if (cxt->cur_status.settings.manual_mode !=2)
				cxt->cur_status.settings.manual_mode = 1;
			cxt->cur_status.settings.table_idx = cxt->flash_backup.table_idx;
			ISP_LOGV("AE_FLASH_PRE_AFTER restore force ae's table_idx : %d, cur manual_mode:%d ", cxt->flash_backup.table_idx,cxt->cur_status.settings.manual_mode);
		} else {
			cxt->cur_result.wts.exposure_time = cxt->flash_backup.exp_time;
			cxt->cur_result.wts.cur_exp_line = cxt->flash_backup.exp_line;
			cxt->cur_result.wts.cur_again = cxt->flash_backup.gain;
			cxt->cur_result.wts.cur_dummy = cxt->flash_backup.dummy;
			cxt->cur_result.wts.cur_index = cxt->flash_backup.cur_index;
			cxt->cur_result.wts.cur_bv = cxt->flash_backup.bv;
			cxt->sync_cur_result.wts.exposure_time = cxt->cur_result.wts.exposure_time;
			cxt->sync_cur_result.wts.cur_exp_line = cxt->cur_result.wts.cur_exp_line;
			cxt->sync_cur_result.wts.cur_again = cxt->cur_result.wts.cur_again;
			cxt->sync_cur_result.wts.cur_dummy = cxt->cur_result.wts.cur_dummy;
			cxt->sync_cur_result.wts.cur_index = cxt->cur_result.wts.cur_index;
			cxt->sync_cur_result.wts.cur_bv = cxt->cur_result.wts.cur_bv;
			memset((void *)&cxt->exp_data, 0, sizeof(cxt->exp_data));
			cxt->exp_data.lib_data.exp_line = cxt->sync_cur_result.wts.cur_exp_line;
			cxt->exp_data.lib_data.exp_time = cxt->sync_cur_result.wts.exposure_time;
			cxt->exp_data.lib_data.gain = cxt->sync_cur_result.wts.cur_again;
			cxt->exp_data.lib_data.dummy = cxt->sync_cur_result.wts.cur_dummy;
			cxt->exp_data.lib_data.line_time = cxt->cur_status.line_time;
			rtn = ae_update_result_to_sensor(cxt, &cxt->exp_data, 0);
		}
		if((0==cxt->cur_status.settings.table_idx)&&(0 == cxt->app_mode)){ // app_mode is for 'ev' controling, which isn't for flash flow. flash won't change anything which ev convers.
			/* trigging the `LockAe/af`'s operation will firstly cause process pre-flash-flow before really setting the ae_compensation_flag. so
			 * this code prevents the table_idx's value 0.  */
			struct ae_exp_compensation exp_comp;
			exp_comp.comp_val=0;
			exp_comp.comp_range.min=-16;
			exp_comp.comp_range.max=16;
			exp_comp.step_numerator=1;
			exp_comp.step_denominator=8;
			ae_set_exposure_compensation(cxt,&exp_comp);
			ISP_LOGV("PRE AFTER table_idx is Zero,fix to %d", cxt->cur_status.settings.table_idx);
			cxt->flash_backup.table_idx = cxt->cur_status.settings.table_idx;
		}

		if ((0 != cxt->flash_ver) && (0 == cxt->exposure_compensation.ae_compensation_flag)) {
			struct ae_exp_compensation exp_comp;
			exp_comp.comp_val=0;
			exp_comp.comp_range.min=-16;
			exp_comp.comp_range.max=16;
			exp_comp.step_numerator=1;
			exp_comp.step_denominator=8;
			ae_set_exposure_compensation(cxt,&exp_comp);
			cxt->cur_status.settings.exp_is_transmit = 1;
			rtn = ae_set_force_pause_flash(cxt, 0);
			cxt->cur_status.settings.manual_mode = 1;
			ISP_LOGD("flash unlock AE_SET_FORCE_RESTORE, exp_is_transmit:%d", cxt->cur_status.settings.exp_is_transmit);
		}

		cxt->send_once[0] = cxt->send_once[1] = cxt->send_once[2] = cxt->send_once[3] = cxt->send_once[4] = cxt->send_once[5] = 0;
		cxt->cur_status.settings.flash = FLASH_PRE_AFTER;
		break;

	case AE_FLASH_MAIN_BEFORE:
		ISP_LOGI("ae_flash_status FLASH_MAIN_BEFORE");
		if ((0 != cxt->flash_ver) && (0 == cxt->exposure_compensation.ae_compensation_flag))
			rtn = ae_set_force_pause(cxt, 1);
		if (cxt->exposure_compensation.ae_compensation_flag )  {
			if(0==cxt->cur_status.settings.table_idx){
				struct ae_exp_compensation exp_comp;
				exp_comp.comp_val=0;
				exp_comp.comp_range.min=-16;
				exp_comp.comp_range.max=16;
				exp_comp.step_numerator=1;
				exp_comp.step_denominator=8;
				ae_set_exposure_compensation(cxt,&exp_comp);
			}

			cxt->flash_backup.table_idx = cxt->cur_status.settings.table_idx;
			ISP_LOGV("AE_FLASH_MAIN_BEFORE store ae's table_idx : %d", cxt->flash_backup.table_idx);
		}
		ISP_LOGV("AE_FLASH_MAIN_BEFORE table_idx is %d, manual_mode:%d ", cxt->flash_backup.table_idx,cxt->cur_status.settings.manual_mode);
		cxt->cur_status.settings.flash = FLASH_MAIN_BEFORE;
		break;

	case AE_FLASH_MAIN_AFTER:
		ISP_LOGI("ae_flash_status FLASH_MAIN_AFTER");
		cxt->has_mf = 1;

		if (cxt->exposure_compensation.ae_compensation_flag) {
			if (cxt->cur_status.settings.manual_mode !=2){
				cxt->cur_status.settings.manual_mode = 1;
				cxt->cur_status.settings.table_idx = cxt->flash_backup.table_idx;
				ISP_LOGV("AE_FLASH_MAIN_AFTER restore ae's table_idx : %d, cur manual_mode:%d", cxt->flash_backup.table_idx,cxt->cur_status.settings.manual_mode);
			}else{ // manual_mode 2 means table idx needs reseting.
				struct ae_exp_compensation exp_comp;
				exp_comp.comp_val=0;
				exp_comp.comp_range.min=-16;
				exp_comp.comp_range.max=16;
				exp_comp.step_numerator=1;
				exp_comp.step_denominator=8;
				ae_set_exposure_compensation(cxt,&exp_comp);
				ISP_LOGV("AE_FLASH_MAIN_AFTER restore ae's table_idx : %d, cur manual_mode:%d", cxt->flash_backup.table_idx,cxt->cur_status.settings.manual_mode);
			}

		}


		if ((0 != cxt->flash_ver) && (0 == cxt->exposure_compensation.ae_compensation_flag))
			rtn = ae_set_force_pause(cxt, 0);

		cxt->send_once[0] = cxt->send_once[1] = cxt->send_once[2] = cxt->send_once[3] = cxt->send_once[4] = cxt->send_once[5] = 0;
		cxt->cur_status.settings.flash = FLASH_MAIN_AFTER;
		break;

	case AE_FLASH_MAIN_AE_MEASURE:
		ISP_LOGV("ae_flash_status FLASH_MAIN_AE_MEASURE");
		break;

	case AE_FLASH_MAIN_LIGHTING:
		ISP_LOGI("ae_flash_status FLASH_MAIN_LIGHTING");
		if (0 != cxt->flash_ver) {
			cxt->cur_status.settings.flash = FLASH_MAIN;
		}
		break;

	case AE_LED_FLASH_ON:
		cxt->cur_status.led_state = 1;
		cxt->cur_status.settings.flash = FLASH_LED_ON;
		ISP_LOGI("ae_flash_status FLASH_LED_ON");
		break;

	case AE_LED_FLASH_OFF:
		cxt->cur_status.led_state = 0;
		cxt->cur_status.settings.flash = FLASH_NONE;
		ISP_LOGV("ae_flash_status FLASH_LED_OFF");
		break;

	case AE_LED_FLASH_AUTO:
		cxt->cur_status.led_state = 0;
		cxt->cur_status.settings.flash = FLASH_LED_AUTO;
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
	cxt->cur_status.flash_mode = *flash_mode;

	return rtn;
}

static cmr_s32 ae_cfg_monitor(struct ae_ctrl_cxt *cxt)
{
	cmr_s32 rtn = AE_SUCCESS;

	ae_cfg_monitor_block(cxt);

	ae_cfg_monitor_bypass(cxt);

	return rtn;
}

static cmr_s32 ae_cvt_exposure_time2line(struct ae_exp_gain_table *src[AE_FLICKER_NUM], struct ae_exp_gain_table *dst[AE_FLICKER_NUM], float linetime, cmr_s16 tablemode)
{
	cmr_s32 rtn = AE_SUCCESS;
	cmr_s32 i = 0;
	cmr_u32 exp_counts = 0;
	cmr_u32 thrd = 0;
	double product = 0.0;
	double tmp_1 = 0;

	cmr_s32 mx = src[AE_FLICKER_50HZ]->max_index;

	ISP_LOGV("exp2line %.2f %d %d\r\n", linetime, tablemode, mx);
	dst[AE_FLICKER_50HZ]->max_index = src[AE_FLICKER_50HZ]->max_index;
	dst[AE_FLICKER_50HZ]->min_index = src[AE_FLICKER_50HZ]->min_index;

	dst[AE_FLICKER_60HZ]->max_index = src[AE_FLICKER_50HZ]->max_index;
	dst[AE_FLICKER_60HZ]->min_index = src[AE_FLICKER_50HZ]->min_index;

	if (0 == tablemode) {
		for (i = 0; i <= mx; i++) {
			product = 1.0 * src[AE_FLICKER_50HZ]->exposure[i] * src[AE_FLICKER_50HZ]->again[i];
			tmp_1 = 1.0 * src[AE_FLICKER_50HZ]->exposure[i] / linetime;
			dst[AE_FLICKER_50HZ]->exposure[i] = (cmr_s32) (tmp_1 + 0.5);
			if (0 == dst[AE_FLICKER_50HZ]->exposure[i]) {
				dst[AE_FLICKER_50HZ]->exposure[i] = 1;
				ISP_LOGE("50HZ [%d]: exp: %d, and will be fixed to %d\n", i, src[AE_FLICKER_50HZ]->exposure[i], (cmr_u32) (dst[AE_FLICKER_50HZ]->exposure[i] * linetime + 0.5));
			}
			dst[AE_FLICKER_50HZ]->again[i] = (cmr_s32) (0.5 + 1.0 * product / (dst[AE_FLICKER_50HZ]->exposure[i] * linetime));
			if (dst[AE_FLICKER_50HZ]->again[i] < AE_BASE_GAIN) {
				dst[AE_FLICKER_50HZ]->again[i] = AE_BASE_GAIN;
			}
		}

		thrd = (cmr_u32) (1.0 / 120 * AEC_LINETIME_PRECESION + 0.5);
		for (i = 0; i <= mx; i++) {
			/*the exposure time is more than 1/120, so it should be the times of 1/120 */
			if (thrd <= src[AE_FLICKER_50HZ]->exposure[i]) {
				product = 1.0 * src[AE_FLICKER_50HZ]->exposure[i] * src[AE_FLICKER_50HZ]->again[i];
				exp_counts = (cmr_u32) (1.0 * src[AE_FLICKER_50HZ]->exposure[i] / thrd + 0.5);
				if (exp_counts < 1) {
					exp_counts = 1;
					ISP_LOGE("[%d]: exp: %d, and will be fixed to %d\n", i, src[AE_FLICKER_50HZ]->exposure[i], (cmr_u32) (exp_counts * thrd + 0.5));
				}
				dst[AE_FLICKER_60HZ]->exposure[i] = (cmr_s32) (1.0 * (exp_counts * thrd) / linetime + 0.5);
				dst[AE_FLICKER_60HZ]->again[i] = (cmr_s32) (1.0 * product / (dst[AE_FLICKER_60HZ]->exposure[i] * linetime) + 0.5);
				if (dst[AE_FLICKER_60HZ]->again[i] < AE_BASE_GAIN) {
					dst[AE_FLICKER_60HZ]->again[i] = AE_BASE_GAIN;
				}
			} else {
				dst[AE_FLICKER_60HZ]->exposure[i] = dst[AE_FLICKER_50HZ]->exposure[i];
				dst[AE_FLICKER_60HZ]->again[i] = dst[AE_FLICKER_50HZ]->again[i];
			}
		}
	} else {
		memcpy((void *)dst[AE_FLICKER_50HZ], (void *)src[AE_FLICKER_50HZ], sizeof(struct ae_exp_gain_table));

		thrd = (cmr_u32) (1.0 / 120 * AEC_LINETIME_PRECESION + 0.5);
		for (i = 0; i <= mx; i++) {
			if (thrd <= src[AE_FLICKER_50HZ]->exposure[i] * linetime) {
				product = 1.0 * src[AE_FLICKER_50HZ]->exposure[i] * src[AE_FLICKER_50HZ]->again[i];
				exp_counts = 1.0 * src[AE_FLICKER_50HZ]->exposure[i] * linetime / thrd;
				tmp_1 = (cmr_s32) (1.0 * exp_counts * thrd / linetime + 0.5);
				if (0 == (cmr_s32) tmp_1) {
					tmp_1 = 1;
					ISP_LOGE("[%d]: exp: %d, and will be fixed to %d\n", i, src[AE_FLICKER_50HZ]->exposure[i], (cmr_u32) (tmp_1 * linetime + 0.5));
				}
				dst[AE_FLICKER_60HZ]->exposure[i] = (cmr_u32) (tmp_1 + 0.5);
				dst[AE_FLICKER_60HZ]->again[i] = (cmr_u32) (1.0 * product / tmp_1 + 0.5);
				if (dst[AE_FLICKER_60HZ]->again[i] < AE_BASE_GAIN) {
					dst[AE_FLICKER_60HZ]->again[i] = AE_BASE_GAIN;
				}
			} else {
				dst[AE_FLICKER_60HZ]->exposure[i] = dst[AE_FLICKER_50HZ]->exposure[i];
				dst[AE_FLICKER_60HZ]->again[i] = dst[AE_FLICKER_50HZ]->again[i];
			}
		}
	}
	return rtn;
}

// set default parameters when initialization or DC-DV convertion
static cmr_s32 ae_set_ae_param(struct ae_ctrl_cxt *cxt, struct ae_init_in *init_param, struct ae_set_work_param *work_param, cmr_s8 init)
{
	cmr_s32 rtn = AE_SUCCESS;
	cmr_u32 i, j = 0;
	cmr_s8 cur_work_mode = AE_WORK_MODE_COMMON;
	struct ae_trim trim = { 0, 0, 0, 0 };
	struct ae_ev_table *ev_table = NULL;
	UNUSED(work_param);

	ISP_LOGV("cam-id %d, Init param %d\r\n", cxt->camera_id, init);

	if (AE_PARAM_INIT == init) {
		if (NULL == cxt || NULL == init_param) {
			ISP_LOGE("fail to set ae param, cxt %p, init_param %p", cxt, init_param);
			return AE_ERROR;
		}

		cur_work_mode = AE_WORK_MODE_COMMON;
		for (i = 0; i < init_param->param_num && i < AE_MAX_PARAM_NUM; ++i) {
			rtn = ae_unpack_tunning_param(init_param->param[i].param, init_param->param[i].size, &cxt->tuning_param[i]);
			memcpy(&cxt->tuning_param[i].backup_ae_table[0][0], &cxt->tuning_param[i].ae_table[0][0], AE_FLICKER_NUM * AE_ISO_NUM_NEW * sizeof(struct ae_exp_gain_table));

			if (AE_SUCCESS == rtn)
				cxt->tuning_param_enable[i] = 1;
			else
				cxt->tuning_param_enable[i] = 0;
		}

		for (j = 0; j < AE_SCENE_NUM; ++j) {
			memcpy(&cxt->back_scene_mode_ae_table[j][AE_FLICKER_50HZ], &cxt->tuning_param[0].scene_info[j].ae_table[AE_FLICKER_50HZ], AE_FLICKER_NUM * sizeof(struct ae_exp_gain_table));
		}

		for (i = 0; i < init_param->dflash_num && i < AE_MAX_PARAM_NUM; ++i) {
			memcpy(&cxt->dflash_param[i], init_param->flash_tuning[i].param, sizeof(struct flash_tune_param));
		}

		cxt->camera_id = init_param->camera_id;
		cxt->isp_ops = init_param->isp_ops;
		cxt->snr_info = init_param->resolution_info;
		cxt->cur_status.frame_size = init_param->resolution_info.frame_size;
		cxt->cur_status.line_time = init_param->resolution_info.line_time;
		cxt->cur_status.frame_id = 0;
		cxt->slw_prev_skip_num = 0;
		memset(&cxt->cur_result, 0, sizeof(struct ae_alg_calc_result));
	} else if (AE_PARAM_NON_INIT == init) {
		;
	}
	/* parameter of common mode should not be NULL */
	if (0 == cxt->tuning_param_enable[AE_WORK_MODE_COMMON]) {
		ISP_LOGE("fail to get tuning param");
		return AE_ERROR;
	}

	cxt->start_id = AE_START_ID;
	cxt->cur_param = &cxt->tuning_param[AE_WORK_MODE_COMMON];

	if(0 == init_param->monitor_win_num.w) {
		if(0 != cxt->cur_param->monitor_param.ae_monitor_win_num_w) {
			cxt->monitor_cfg.blk_num.w = cxt->cur_param->monitor_param.ae_monitor_win_num_w;
			cxt->monitor_cfg.blk_num.w = (128 < cxt->monitor_cfg.blk_num.w) ? 128 : cxt->monitor_cfg.blk_num.w;
		} else {
			cxt->monitor_cfg.blk_num.w = 32;
		}
	} else {
		cxt->monitor_cfg.blk_num.w = init_param->monitor_win_num.w;
	}

	if(0 == init_param->monitor_win_num.h) {
		if(0 != cxt->cur_param->monitor_param.ae_monitor_win_num_h) {
			cxt->monitor_cfg.blk_num.h = cxt->cur_param->monitor_param.ae_monitor_win_num_h;
			cxt->monitor_cfg.blk_num.h = (128 < cxt->monitor_cfg.blk_num.h) ? 128 : cxt->monitor_cfg.blk_num.h;
		} else {
			cxt->monitor_cfg.blk_num.w = 32;
		}
	} else {
		cxt->monitor_cfg.blk_num.h = init_param->monitor_win_num.h;
	}

	ISP_LOGV("monitor_blk_num_w %d, num_h %d", cxt->monitor_cfg.blk_num.w, cxt->monitor_cfg.blk_num.h);

	cxt->monitor_cfg.mode = AE_STATISTICS_MODE_CONTINUE;
	cxt->monitor_cfg.skip_num = 0;
	cxt->monitor_cfg.bypass = 0;
	/* set cxt->monitor_unit.trim & cxt->monitor_unit.win_size */
	trim.x = 0;
	trim.y = 0;
	trim.w = init_param->resolution_info.frame_size.w;
	trim.h = init_param->resolution_info.frame_size.h;
	ae_update_monitor_unit(cxt, &trim);

	for (i = 0; i < 16; ++i) {
		cxt->stable_zone_ev[i] = cxt->cur_param->stable_zone_ev[i];
	}
	for (i = 0; i < 32; ++i) {
		cxt->cnvg_stride_ev[i] = cxt->cur_param->cnvg_stride_ev[i];
	}
	cxt->cnvg_stride_ev_num = cxt->cur_param->cnvg_stride_ev_num;
	cxt->exp_skip_num = cxt->cur_param->sensor_cfg.exp_skip_num;
	cxt->gain_skip_num = cxt->cur_param->sensor_cfg.gain_skip_num;
	cxt->sensor_gain_precision = cxt->cur_param->sensor_cfg.gain_precision;
	if (0 == cxt->cur_param->sensor_cfg.max_gain) {
		cxt->sensor_max_gain = 16 * 128;
	} else {
		cxt->sensor_max_gain = cxt->cur_param->sensor_cfg.max_gain;
	}

	if (0 == cxt->sensor_gain_precision) {
		cxt->sensor_gain_precision = 1;
	}

	if (0 == cxt->cur_param->sensor_cfg.min_gain) {
		cxt->sensor_min_gain = 1 * 128;
	} else {
		cxt->sensor_min_gain = cxt->cur_param->sensor_cfg.min_gain;
	}

	if (0 == cxt->cur_param->sensor_cfg.min_exp_line) {
		cxt->min_exp_line = 4;
	} else {
		cxt->min_exp_line = cxt->cur_param->sensor_cfg.min_exp_line;
	}
	cxt->mod_update_list.is_scene = 1;	/*force update scene mode parameters */
	cxt->sync_cur_status.settings.scene_mode = AE_SCENE_NORMAL;
	cxt->cur_status.log_level = g_isp_log_level;
	cxt->cur_status.alg_id = cxt->cur_param->alg_id;
	cxt->cur_status.win_size.w = cxt->monitor_cfg.blk_size.w * cxt->monitor_cfg.blk_num.w/32;
	cxt->cur_status.win_size.h = cxt->monitor_cfg.blk_size.h * cxt->monitor_cfg.blk_num.h/32;
	cxt->cur_status.win_num.w = 32;
	cxt->cur_status.win_num.h = 32;
	cxt->cur_status.awb_gain.r = 1024;
	cxt->cur_status.awb_gain.g = 1024;
	cxt->cur_status.awb_gain.b = 1024;
	cxt->cur_status.awb_cur_gain.r = 1024;
	cxt->cur_status.awb_cur_gain.g = 1024;
	cxt->cur_status.awb_cur_gain.b = 1024;
	cxt->cur_status.awb_mode = 0;
	cxt->cur_status.ae_table = &cxt->cur_param->ae_table[cxt->cur_param->flicker_index][AE_ISO_AUTO];
	cxt->cur_status.ae_table->min_index = 0;
	cxt->cur_status.weight_table = cxt->cur_param->weight_table[AE_WEIGHT_CENTER].weight;
	cxt->cur_status.stat_img = NULL;
	
	if (init_param->is_multi_mode) {
		/* save master & slave sensor info */
		struct sensor_info sensor_info;
		sensor_info.max_again = cxt->sensor_max_gain;
		sensor_info.min_again = cxt->sensor_min_gain;
		sensor_info.sensor_gain_precision = cxt->sensor_gain_precision;
		sensor_info.min_exp_line = cxt->min_exp_line;
		sensor_info.line_time = cxt->cur_status.line_time;
#ifdef CONFIG_ISP_2_2
		rtn = cxt->ptr_isp_br_ioctrl(init_param->is_master ? CAM_SENSOR_MASTER : CAM_SENSOR_SLAVE0, init_param->is_master ? SET_MASTER_MODULE_INFO : SET_SLAVE_MODULE_INFO, &sensor_info, NULL);
#else
		if (init_param->is_multi_mode == ISP_ALG_DUAL_SBS) {
			rtn = cxt->ptr_isp_br_ioctrl(init_param->is_master ? CAM_SENSOR_MASTER : CAM_SENSOR_SLAVE0, SET_MODULE_INFO, &sensor_info, NULL);
		}
		else if((init_param->is_multi_mode == ISP_ALG_DUAL_C_C||init_param->is_multi_mode ==ISP_ALG_DUAL_W_T||init_param->is_multi_mode ==ISP_ALG_DUAL_C_M)) {
			rtn = cxt->ptr_isp_br_ioctrl(init_param->is_master ? CAM_SENSOR_MASTER : CAM_SENSOR_SLAVE0, SET_MODULE_INFO, &sensor_info, NULL);
		}
#endif

		ISP_LOGV("sensor info: role=%d, max_gain=%d, min_gain=%d, precision=%d, min_exp_line=%d, line_time=%d",
				 init_param->is_master, sensor_info.max_again, sensor_info.min_again, sensor_info.sensor_gain_precision, sensor_info.min_exp_line, sensor_info.line_time);
	}

	cxt->cur_status.start_index = cxt->cur_param->start_index;
	ev_table = &cxt->cur_param->ev_table;
	cxt->cur_status.target_lum = ae_calc_target_lum(cxt->cur_param->target_lum, ev_table->default_level, ev_table);
	cxt->cur_status.target_lum_zone = cxt->stable_zone_ev[ev_table->default_level];

	cxt->cur_status.b = NULL;
	cxt->cur_status.r = NULL;
	cxt->cur_status.g = NULL;
	/* adv_alg module init */
	cxt->cur_status.adv[0] = (cmr_handle) & cxt->cur_param->region_param;
	cxt->cur_status.adv[1] = (cmr_handle) & cxt->cur_param->flat_param;
	cxt->cur_status.adv[2] = (cmr_handle) & cxt->cur_param->mulaes_param;
	cxt->cur_status.adv[3] = (cmr_handle) & cxt->cur_param->touch_info;
	cxt->cur_status.adv[4] = (cmr_handle) & cxt->cur_param->face_param;
	cxt->cur_status.adv[6] = (cmr_handle) & cxt->cur_param->ai_param.backlight_param;
	cxt->cur_status.adv[7] = (cmr_handle) & cxt->cur_param->ai_param.sky_param;
	cxt->cur_status.adv[8] = (cmr_handle) & cxt->cur_param->ai_param.foliage_param;
	cxt->cur_status.adv[9] = (cmr_handle) & cxt->cur_param->ai_param.night_param;
	cxt->cur_status.adv[10] = (cmr_handle) & cxt->cur_param->ai_param.outdoor_param;
	cxt->cur_status.adv[11] = (cmr_handle) & cxt->cur_param->ai_param.indoor_param;
	cxt->cur_status.adv[12] = (cmr_handle) & cxt->cur_param->abl_param;
	cxt->cur_status.adv[13] = (cmr_handle) & cxt->cur_param->pcp_param;
	/* caliberation for bv match with lv */
	cxt->cur_status.lv_cali_bv = cxt->cur_param->lv_cali.bv_value;
	{
		cxt->cur_status.lv_cali_lv = log(cxt->cur_param->lv_cali.lux_value * 2.17) * 1.45;	//  (1 / ln2) = 1.45;
	}
	/* refer to convergence */
	cxt->cur_status.ae_start_delay = cxt->cur_param->enter_skip_num;
	cxt->cur_status.stride_config[0] = cxt->cnvg_stride_ev[ev_table->default_level * 2];
	cxt->cur_status.stride_config[1] = cxt->cnvg_stride_ev[ev_table->default_level * 2 + 1];
	cxt->cur_status.under_satu = 5;
	cxt->cur_status.ae_satur = 250;
	cxt->cur_status.settings.ver = 0;
	cxt->cur_status.settings.lock_ae = 0;
	/* Be effective when 1 == lock_ae */
	cxt->cur_status.settings.exp_line = 0;
	cxt->cur_status.settings.gain = 0;
	/* Be effective when being not in the video mode */
	cxt->cur_status.settings.min_fps = 5;
	cxt->cur_status.settings.max_fps = 30;
	cxt->cur_status.target_offset = 0;
	ISP_LOGV("cam-id %d, snr setting max fps: %d\n", cxt->camera_id, cxt->snr_info.snr_setting_max_fps);
	cxt->cur_status.settings.sensor_max_fps = 30;

	/* flash ae param */
	cxt->cur_status.settings.flash = 0;
	cxt->cur_status.settings.flash_ration = cxt->cur_param->flash_param.adjust_ratio;
	//cxt->flash_on_off_thr;
	ISP_LOGV("cam-id %d, flash_ration_mp: %d\n", cxt->camera_id, cxt->cur_status.settings.flash_ration);

	cxt->cur_status.settings.iso = AE_ISO_AUTO;
	cxt->cur_status.settings.ev_index = ev_table->default_level;
	cxt->cur_status.settings.flicker = cxt->cur_param->flicker_index;

	cxt->cur_status.settings.flicker_mode = 0;
	cxt->cur_status.settings.FD_AE = 0;
	cxt->cur_status.settings.metering_mode = AE_WEIGHT_CENTER;
	cxt->cur_status.settings.work_mode = cur_work_mode;
	cxt->cur_status.settings.scene_mode = AE_SCENE_NORMAL;
	cxt->cur_status.settings.intelligent_module = 0;

	cxt->cur_status.settings.reserve_case = 0;
	cxt->cur_status.settings.reserve_len = 0;	/* len for reserve */
	cxt->cur_status.settings.reserve_info = NULL;	/* reserve for future */

	cxt->touch_zone_param.zone_param = cxt->cur_param->touch_param;
	if (0 == cxt->touch_zone_param.zone_param.level_0_weight) {
		cxt->touch_zone_param.zone_param.level_0_weight = 1;
		cxt->touch_zone_param.zone_param.level_1_weight = 1;
		cxt->touch_zone_param.zone_param.level_2_weight = 1;
		cxt->touch_zone_param.zone_param.level_1_percent = 0;
		cxt->touch_zone_param.zone_param.level_2_percent = 0;
	}
	/* to keep the camera can work well when the touch ae setting is zero or NULL in the tuning file */
	cxt->cur_status.settings.touch_tuning_enable = cxt->cur_param->touch_info.enable;
	if ((1 == cxt->cur_param->touch_info.enable)	//for debug, if release, we need to change 0 to 1
		&& ((0 == cxt->cur_param->touch_info.touch_tuning_win.w)
			|| (0 == cxt->cur_param->touch_info.touch_tuning_win.h)
			|| ((0 == cxt->cur_param->touch_info.win1_weight)
				&& (0 == cxt->cur_param->touch_info.win2_weight)))) {
		cxt->cur_status.touch_tuning_win.w = cxt->snr_info.frame_size.w / 10;
		cxt->cur_status.touch_tuning_win.h = cxt->snr_info.frame_size.h / 10;
		cxt->cur_status.win1_weight = 4;
		cxt->cur_status.win2_weight = 3;
	} else {
		cxt->cur_status.touch_tuning_win = cxt->cur_param->touch_info.touch_tuning_win;
		cxt->cur_status.win1_weight = cxt->cur_param->touch_info.win1_weight;
		cxt->cur_status.win2_weight = cxt->cur_param->touch_info.win2_weight;
	}
	/* fd-ae param */
	//_fdae_init(cxt);

	/* control information for sensor update */
	cxt->checksum = ae_get_checksum();
	cxt->end_id = AE_END_ID;

	return AE_SUCCESS;
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
	cxt->cur_status.settings.lock_ae = AE_STATE_LOCKED;
	if (AE_CTRL_SET_INDEX == ae_ctrl_ptr->mode) {
		cxt->cur_status.settings.manual_mode = 1;	/*ae index */
		cxt->cur_status.settings.table_idx = ae_ctrl_ptr->index;
	} else {					/*exposure & gain */
		cxt->cur_status.settings.manual_mode = 0;
		cxt->cur_status.settings.exp_line = ae_ctrl_ptr->shutter;
		cxt->cur_status.settings.gain = ae_ctrl_ptr->again;
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
	ae_ctrl_ptr->index = cxt->sync_cur_result.wts.cur_index;
	ae_ctrl_ptr->lum = cxt->sync_cur_result.cur_lum;
	ae_ctrl_ptr->shutter = cxt->sync_cur_result.wts.cur_exp_line;
	ae_ctrl_ptr->dummy = cxt->sync_cur_result.wts.cur_dummy;
	ae_ctrl_ptr->again = cxt->sync_cur_result.wts.cur_again;
	ae_ctrl_ptr->dgain = cxt->sync_cur_result.wts.cur_dgain;
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
		cxt->cur_status.ae1_finfo.update_flag = 1;
		if (fd->face_num > 0) {
			cxt->cur_status.ae1_finfo.img_width = fd->width;
			cxt->cur_status.ae1_finfo.img_height = fd->height;
			cxt->cur_status.ae1_finfo.cur_info.face_num = fd->face_num;
			ISP_LOGV("FD_CTRL_NUM:%d,flag is %d\n", cxt->cur_status.ae1_finfo.cur_info.face_num, cxt->cur_status.ae1_finfo.update_flag);
			for (i = 0; i < fd->face_num; i++) {
				cxt->cur_status.ae1_finfo.cur_info.face_area[i].start_x = fd->face_area[i].rect.start_x;
				cxt->cur_status.ae1_finfo.cur_info.face_area[i].start_y = fd->face_area[i].rect.start_y;
				cxt->cur_status.ae1_finfo.cur_info.face_area[i].end_x = fd->face_area[i].rect.end_x;
				cxt->cur_status.ae1_finfo.cur_info.face_area[i].end_y = fd->face_area[i].rect.end_y;
				cxt->cur_status.ae1_finfo.cur_info.face_area[i].pose = fd->face_area[i].pose;
			}
		} else {
			cxt->cur_status.ae1_finfo.cur_info.face_num = 0;
			for (i = 0; i < fd->face_num; i++) {
				cxt->cur_status.ae1_finfo.cur_info.face_area[i].start_x = 0;
				cxt->cur_status.ae1_finfo.cur_info.face_area[i].start_y = 0;
				cxt->cur_status.ae1_finfo.cur_info.face_area[i].end_x = 0;
				cxt->cur_status.ae1_finfo.cur_info.face_area[i].end_y = 0;
				cxt->cur_status.ae1_finfo.cur_info.face_area[i].pose = -1;
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
		*(float *)result = cxt->cur_result.wts.exposure_time / AEC_LINETIME_PRECESION;
	}
	return AE_SUCCESS;
}

static cmr_s32 ae_get_metering_mode(struct ae_ctrl_cxt *cxt, cmr_handle result)
{
	if (result) {
		*(cmr_s8 *) result = cxt->cur_status.settings.metering_mode;
	}

	return AE_SUCCESS;
}

static cmr_s32 ae_bypass_algorithm(struct ae_ctrl_cxt *cxt, cmr_handle param)
{
	if (param) {
		if (1 == *(cmr_s32 *) param) {
			cxt->bypass = 1;
		} else {
			cxt->cur_status.settings.lock_ae = AE_STATE_NORMAL;
			cxt->cur_status.settings.pause_cnt = 0;
			cxt->bypass = 0;
		}
	}

	return AE_SUCCESS;
}

static cmr_s32 ae_printf_status_log(struct ae_ctrl_cxt *cxt, cmr_s8 scene_mod, struct ae_alg_calc_param *alg_status)
{
	cmr_s32 ret = AE_SUCCESS;
	UNUSED(cxt);
	ISP_LOGV("scene: %d\n", scene_mod);
	ISP_LOGV("target: %d, zone: %d\n", alg_status->target_lum, alg_status->target_lum_zone);
	ISP_LOGV("iso: %d\n", alg_status->settings.iso);
	ISP_LOGV("ev offset: %d\n", alg_status->settings.ev_index);
	ISP_LOGV("fps: [%d, %d]\n", alg_status->settings.min_fps, alg_status->settings.max_fps);
	ISP_LOGV("metering: %d--ptr%p\n", alg_status->settings.metering_mode, alg_status->weight_table);
	ISP_LOGV("flicker: %d, table: %p, range: [%d, %d]\n", alg_status->settings.flicker, alg_status->ae_table, alg_status->ae_table->min_index, alg_status->ae_table->max_index);
	return ret;
}

/*set_scene_mode just be called in ae_sprd_calculation,*/
static cmr_s32 ae_set_scene_mode(struct ae_ctrl_cxt *cxt, enum ae_scene_mode cur_scene_mod, enum ae_scene_mode nxt_scene_mod)
{
	cmr_s32 rtn = AE_SUCCESS;
	struct ae_tuning_param *cur_param = NULL;
	struct ae_scene_info *scene_info = NULL;
	struct ae_alg_calc_param *cur_status = NULL;
	struct ae_alg_calc_param *prv_status = NULL;
	cmr_u32 i = 0;
	cmr_s32 target_lum = 0;
	cmr_u32 iso = 0;
	cmr_u32 weight_mode = 0;
	cmr_s32 max_index = 0;
	prv_status = &cxt->prv_status;
	cur_status = &cxt->cur_status;
	if (nxt_scene_mod >= AE_SCENE_MOD_MAX) {
		ISP_LOGE("fail to set scene mod, %d\n", nxt_scene_mod);
		return AE_ERROR;
	}
	cur_param = cxt->cur_param;
	scene_info = &cur_param->scene_info[0];
	if ((AE_SCENE_NORMAL == cur_scene_mod) && (AE_SCENE_NORMAL == nxt_scene_mod)) {
		ISP_LOGV("normal  has special setting\n");
		goto SET_SCENE_MOD_EXIT;
	}

	if (AE_SCENE_NORMAL != nxt_scene_mod) {
		for (i = 0; i < AE_SCENE_NUM; ++i) {
			ISP_LOGV("%d: mod: %d, eb: %d\n", i, scene_info[i].scene_mode, scene_info[i].enable);
			if ((1 == scene_info[i].enable) && (nxt_scene_mod == scene_info[i].scene_mode)) {
				break;
			}
		}

		if ((i >= AE_SCENE_NUM) && (AE_SCENE_NORMAL != nxt_scene_mod)) {
			ISP_LOGV("Not has special scene setting, just using the normal setting\n");
			goto SET_SCENE_MOD_EXIT;
		}
	}

	if (AE_SCENE_NORMAL != nxt_scene_mod) {
		/*normal scene--> special scene */
		/*special scene--> special scene */
		ISP_LOGV("i and iso_index is %d,%d,%d", i, scene_info[i].iso_index, scene_info[i].weight_mode);
		iso = scene_info[i].iso_index;
		weight_mode = scene_info[i].weight_mode;

		if (iso >= AE_ISO_MAX || weight_mode >= AE_WEIGHT_MAX) {
			ISP_LOGE("fail to set scene mode iso=%d, weight_mode=%d", iso, weight_mode);
			rtn = AE_ERROR;
			goto SET_SCENE_MOD_EXIT;
		}

		if (AE_SCENE_NORMAL == cur_scene_mod) {	/*from normal scene to special scene */
			cxt->prv_status = *cur_status;	/*backup the normal scene's information */
		}
		/*ae table */
		max_index = scene_info[i].ae_table[0].max_index;
#if 1
		/*
		   for (cmr_s32 j = 0; j <= max_index; j++) {
		   ISP_LOGV("Current_status_exp_gain is %d,%d\n", cur_status->ae_table->exposure[j], cur_status->ae_table->again[j]);
		   }
		 */
		ISP_LOGV("CUR_STAT_EXP_GAIN : %d,%d,%d", cur_status->effect_expline, cur_status->effect_gain, cur_status->settings.iso);
		ISP_LOGV("TAB_EN IS %d", scene_info[i].table_enable);
		if (scene_info[i].table_enable) {
			ISP_LOGV("mode is %d", i);
			cur_status->ae_table = &scene_info[i].ae_table[cur_status->settings.flicker];
		} else {
			cur_status->ae_table = cxt->prv_status.ae_table;
		}
		cur_status->ae_table->min_index = 0;
#endif
		cur_status->settings.iso = scene_info[i].iso_index;
		if (AE_WORK_MODE_VIDEO != cur_status->settings.work_mode) {
			cur_status->settings.min_fps = scene_info[i].min_fps;
			cur_status->settings.max_fps = scene_info[i].max_fps;
		}
		target_lum = ae_calc_target_lum(scene_info[i].target_lum, scene_info[i].ev_offset, &cur_param->ev_table);
		cur_status->target_lum_zone = (cmr_s16) (cur_param->stable_zone_ev[cur_param->ev_table.default_level] * target_lum * 1.0 / cur_param->target_lum + 0.5);
		if (2 > cur_status->target_lum_zone) {
			cur_status->target_lum_zone = 2;
		}
		cur_status->target_lum = target_lum;
		cur_status->weight_table = (cmr_u8 *) & cur_param->weight_table[scene_info[i].weight_mode];
		cur_status->settings.metering_mode = scene_info[i].weight_mode;
		cur_status->settings.scene_mode = nxt_scene_mod;
	}

	if (AE_SCENE_NORMAL == nxt_scene_mod) {	/*special scene --> normal scene */
	  SET_SCENE_MOD_2_NOAMAL:
		iso = prv_status->settings.iso;
		weight_mode = prv_status->settings.metering_mode;
		if (iso >= AE_ISO_MAX || weight_mode >= AE_WEIGHT_MAX) {
			ISP_LOGE("fail to get iso=%d, weight_mode=%d", iso, weight_mode);
			rtn = AE_ERROR;
			goto SET_SCENE_MOD_EXIT;
		}
		if(CAMERA_MODE_MANUAL == cxt->app_mode)
			prv_status->settings.ev_index = cxt->manual_level;
		target_lum = ae_calc_target_lum(cur_param->target_lum, prv_status->settings.ev_index, &cur_param->ev_table);
		cur_status->target_lum = target_lum;
		cur_status->target_lum_zone = cur_param->stable_zone_ev[cur_param->ev_table.default_level];
		//cur_status->target_lum_zone = (cmr_s16)(cur_param->target_lum_zone * (target_lum * 1.0 / cur_param->target_lum) + 0.5);
		cur_status->settings.ev_index = prv_status->settings.ev_index;
		cur_status->settings.iso = prv_status->settings.iso;
		cur_status->settings.metering_mode = prv_status->settings.metering_mode;
		cur_status->weight_table = &cur_param->weight_table[prv_status->settings.metering_mode].weight[0];
		//cur_status->ae_table = &cur_param->ae_table[prv_status->settings.flicker][prv_status->settings.iso];
		cur_status->ae_table = &cur_param->ae_table[prv_status->settings.flicker][AE_ISO_AUTO];
		cur_status->settings.min_fps = prv_status->settings.min_fps;
		cur_status->settings.max_fps = prv_status->settings.max_fps;
		cur_status->settings.scene_mode = nxt_scene_mod;
	}
  SET_SCENE_MOD_EXIT:
	ISP_LOGV("cam-id %d, change scene mode from %d to %d, rtn=%d", cxt->camera_id, cur_scene_mod, nxt_scene_mod, rtn);
	return rtn;
}

static cmr_s32 ae_set_manual_mode(struct ae_ctrl_cxt *cxt, cmr_handle param)
{
	cmr_s32 rtn = AE_SUCCESS;
	cmr_s32 lock;

	if (param) {
		lock = !(*(cmr_u32 *) param);
		if (0 == *(cmr_u32 *) param) {
			cxt->cur_status.settings.manual_mode = 0;
			cxt->manual_exp_time = 0;
			cxt->manual_iso_value = 0;
			ae_set_force_pause(cxt, lock);
		} else if (0 == cxt->exposure_compensation.ae_compensation_flag) {
			ae_set_force_pause(cxt, lock);
		}
		ISP_LOGV("AE_SET_MANUAL_MODE: %d, manual: 0, auto: 1\n", *(cmr_u32 *) param);
	}
	return rtn;
}

static cmr_s32 ae_set_exp_time(struct ae_ctrl_cxt *cxt, cmr_handle param)
{
	cmr_s32 rtn = AE_SUCCESS;
	cmr_u32 exp_time = 10000000;	//init default value(10ms)
	cmr_u32 line_time = cxt->snr_info.line_time;
	cmr_u16 gain = 1280;		//init default value(10xgain)

	if (param) {
		exp_time = *(cmr_u32 *) param;
		if (exp_time > 0)
			cxt->manual_exp_time = exp_time;
		else {
			ISP_LOGE("set invalid expouser time: %d", exp_time);
			exp_time = cxt->cur_result.wts.exposure_time;
		}
		if (cxt->manual_iso_value > 0)
			gain = (cxt->manual_iso_value / 50) * 128;
		else
			gain = cxt->cur_result.wts.cur_again;	//reusing current gain

		if (AE_STATE_LOCKED == cxt->cur_status.settings.lock_ae) {
			cxt->cur_status.settings.exp_line = exp_time / line_time;
			cxt->cur_status.settings.gain = gain;
			cxt->cur_status.settings.manual_mode = 0;
			ISP_LOGV("exp_time: %d line_time: %d gain: %d iso: %d", exp_time, line_time, gain, cxt->manual_iso_value);
		}
	}
	return rtn;
}

static cmr_s32 ae_set_manual_iso(struct ae_ctrl_cxt *cxt, cmr_handle param)
{
	cmr_s32 rtn = AE_SUCCESS;
	cmr_u32 iso = 600;			//init default value(600)
	cmr_u32 exp_time = 10000000;	//init default value(10ms)
	cmr_u32 line_time = cxt->snr_info.line_time;
	cmr_u32 gain = 1280;		//init default value(10xgain)

	if (param) {
		iso = *(cmr_u32 *) param;
		if (iso > 0) {
			cxt->manual_iso_value = iso;
			gain = (iso / 50) * 128;
		} else
			gain = cxt->cur_result.wts.cur_again;	//reusing current gain
		if (cxt->manual_exp_time > 0)
			exp_time = cxt->manual_exp_time;
		else
			exp_time = cxt->cur_result.wts.exposure_time;	//reusing current exp_time

		if (AE_STATE_LOCKED == cxt->cur_status.settings.lock_ae) {
			cxt->cur_status.settings.exp_line = exp_time / line_time;
			cxt->cur_status.settings.gain = gain;
			ISP_LOGV("exp_time: %d line_time: %d gain: %d iso: %d", exp_time, line_time, gain, iso);
		}
	}
	return rtn;
}

static cmr_s32 ae_set_gain(struct ae_ctrl_cxt *cxt, cmr_handle param)
{
	cmr_s32 rtn = AE_SUCCESS;
	cmr_u32 sensitivity = *(cmr_u32 *) param;
	if (param) {
		if (AE_STATE_LOCKED == cxt->cur_status.settings.lock_ae) {
			cxt->cur_status.settings.gain = sensitivity * 128 / 50;
		}
	}
	return rtn;
}
static cmr_s32 ae_set_force_pause_flash(struct ae_ctrl_cxt *cxt, cmr_u32 enable)
{
       cmr_s32 ret = AE_SUCCESS;

       if (enable) {
		ISP_LOGI("cxt->expchanged:%d",cxt->expchanged);
		if(cxt->expchanged == 0){
			if(cxt->sync_cur_result.wts.cur_index==0) {
				cxt->sync_cur_result.wts.cur_index=280;
				ISP_LOGD("wts.cur_index is Zero");
			}
			cxt->exposure_compensation.ae_base_idx = cxt->sync_cur_result.wts.cur_index;
			ISP_LOGI("ae_base_idx:%d ",cxt->exposure_compensation.ae_base_idx);
		}
               cxt->cur_status.settings.lock_ae = AE_STATE_LOCKED;
               if (0 == cxt->cur_status.settings.pause_cnt) {
                       cxt->cur_status.settings.exp_line = 0;
                       cxt->cur_status.settings.gain = 0;
                       cxt->cur_status.settings.manual_mode = -1;
               }
       } else {
               if (2 > cxt->cur_status.settings.pause_cnt) {
                       cxt->cur_status.settings.lock_ae = AE_STATE_NORMAL;
                       cxt->cur_status.settings.pause_cnt = 0;
                       cxt->cur_status.settings.exp_line = 0;
                       cxt->cur_status.settings.gain = 0;
                       cxt->cur_status.settings.manual_mode = -1;
               }
       }
       cxt->cur_status.settings.force_lock_ae = enable;
       ISP_LOGD("PAUSE COUNT IS %d, lock: %d, %d, manual_mode:%d", cxt->cur_status.settings.pause_cnt, cxt->cur_status.settings.lock_ae, cxt->cur_status.settings.force_lock_ae,cxt->cur_status.settings.manual_mode);
       return ret;
}
static cmr_s32 ae_set_force_pause(struct ae_ctrl_cxt *cxt, cmr_u32 enable)
{
	cmr_s32 ret = AE_SUCCESS;

	if (enable) {
		if(!(cxt->is_snapshot)) {
			if(cxt->sync_cur_result.wts.cur_index==0) {
				cxt->sync_cur_result.wts.cur_index=280;
				ISP_LOGD("wts.cur_index is Zero");
			}
			cxt->exposure_compensation.ae_base_idx = cxt->sync_cur_result.wts.cur_index;
			ISP_LOGD("cxt->is_snapshot:%d, cxt->exposure_compensation.ae_base_idx:%d, cxt->sync_cur_result.wts.cur_index:%d",cxt->is_snapshot, cxt->exposure_compensation.ae_base_idx, cxt->sync_cur_result.wts.cur_index);
		}

		cxt->cur_status.settings.lock_ae = AE_STATE_LOCKED;
		if (0 == cxt->cur_status.settings.pause_cnt) {
			cxt->cur_status.settings.exp_line = 0;
			cxt->cur_status.settings.gain = 0;
			cxt->cur_status.settings.manual_mode = -1;
		}
	} else {
		if (2 > cxt->cur_status.settings.pause_cnt) {
			cxt->cur_status.settings.lock_ae = AE_STATE_NORMAL;
			cxt->cur_status.settings.pause_cnt = 0;
			cxt->cur_status.settings.exp_line = 0;
			cxt->cur_status.settings.gain = 0;
			cxt->cur_status.settings.manual_mode = -1;
		}
	}
	cxt->cur_status.settings.force_lock_ae = enable;
	ISP_LOGD("PAUSE COUNT IS %d, lock: %d, %d", cxt->cur_status.settings.pause_cnt, cxt->cur_status.settings.lock_ae, cxt->cur_status.settings.force_lock_ae);
	return ret;
}

static cmr_s32 ae_set_pause(struct ae_ctrl_cxt *cxt)
{
	cmr_s32 ret = AE_SUCCESS;

	if((!cxt->is_snapshot) && (!cxt->cur_status.settings.lock_ae))
		cxt->exposure_compensation.ae_base_idx = cxt->sync_cur_result.wts.cur_index;
	cxt->cur_status.settings.lock_ae = AE_STATE_LOCKED;
	if (0 == cxt->cur_status.settings.pause_cnt) {
		cxt->cur_status.settings.exp_line = 0;
		cxt->cur_status.settings.gain = 0;
		cxt->cur_status.settings.manual_mode = -1;
	}
	cxt->cur_status.settings.pause_cnt++;
	ISP_LOGD("PAUSE COUNT IS %d, lock: %d, %d", cxt->cur_status.settings.pause_cnt, cxt->cur_status.settings.lock_ae, cxt->cur_status.settings.force_lock_ae);
	return ret;
}

static cmr_s32 ae_set_restore_cnt(struct ae_ctrl_cxt *cxt)
{
	cmr_s32 ret = AE_SUCCESS;

	if (!cxt->cur_status.settings.pause_cnt)
		return ret;

	if (2 > cxt->cur_status.settings.pause_cnt) {
		if (0 == cxt->cur_status.settings.force_lock_ae) {
			cxt->cur_status.settings.lock_ae = AE_STATE_NORMAL;
			cxt->cur_status.settings.pause_cnt = 0;
			cxt->cur_status.settings.manual_mode = 0;
			ISP_LOGV("table_idx:%d->0",cxt->cur_status.settings.table_idx);
			cxt->cur_status.settings.table_idx = 0;
		}
	} else {
		cxt->cur_status.settings.pause_cnt--;
	}

	ISP_LOGD("PAUSE COUNT IS %d, lock: %d, %d", cxt->cur_status.settings.pause_cnt, cxt->cur_status.settings.lock_ae, cxt->cur_status.settings.force_lock_ae);
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

static cmr_s32 ae_set_soft_gain(struct ae_ctrl_cxt *cxt, cmr_u32 * src_aem_stat, cmr_u32 * dst_aem_stat, struct ae_size aem_blk_num, cmr_u32 isp_dgain)
{
	cmr_s32 ret = AE_SUCCESS;
	cmr_u32 i = 0, num = 0;
	cmr_u32 *src_r_ptr = NULL;
	cmr_u32 *src_g_ptr = NULL;
	cmr_u32 *src_b_ptr = NULL;
	cmr_u32 *dst_r_ptr = NULL;
	cmr_u32 *dst_g_ptr = NULL;
	cmr_u32 *dst_b_ptr = NULL;

	num = aem_blk_num.w * aem_blk_num.h;
	src_r_ptr = (cmr_u32 *) src_aem_stat;
	src_g_ptr = (cmr_u32 *) src_aem_stat + num;
	src_b_ptr = (cmr_u32 *) src_aem_stat + 2 * num;
	dst_r_ptr = (cmr_u32 *) dst_aem_stat;
	dst_g_ptr = (cmr_u32 *) dst_aem_stat + num;
	dst_b_ptr = (cmr_u32 *) dst_aem_stat + 2 * num;
	for (i = 0; i < num; ++i) {
		*dst_r_ptr = (cmr_u32) ((1.0 * (*src_r_ptr) * isp_dgain / 4096) * cxt->ob_rgb_gain + 0.5);
		*dst_g_ptr = (cmr_u32) ((1.0 * (*src_g_ptr) * isp_dgain / 4096) * cxt->ob_rgb_gain + 0.5);
		*dst_b_ptr = (cmr_u32) ((1.0 * (*src_b_ptr) * isp_dgain / 4096) * cxt->ob_rgb_gain + 0.5);
		src_r_ptr++;
		src_g_ptr++;
		src_b_ptr++;
		dst_r_ptr++;
		dst_g_ptr++;
		dst_b_ptr++;
	}

	return ret;
}

static cmr_s32 flash_pre_start(struct ae_ctrl_cxt *cxt)
{
	cmr_s32 rtn = AE_SUCCESS;
	struct Flash_pfStartInput in;
	struct Flash_pfStartOutput out;
	struct ae_alg_calc_param *current_status = &cxt->cur_status;
	cmr_u32 blk_num = 0;
	cmr_u32 i;
	/*reset flash debug information */
	memset(&cxt->flash_debug_buf[0], 0, sizeof(cxt->flash_debug_buf));
	cxt->flash_buf_len = 0;
	memset(&cxt->flash_esti_result, 0, sizeof(cxt->flash_esti_result));	/*reset result */

	in.minExposure = current_status->ae_table->exposure[current_status->ae_table->min_index] * current_status->line_time / SENSOR_LINETIME_BASE;
	in.maxExposure = current_status->ae_table->exposure[current_status->ae_table->max_index] * current_status->line_time / SENSOR_LINETIME_BASE;
	if(cxt->cur_flicker == 0) {
		if(in.maxExposure > 600000) {
			in.maxExposure = 600000;
		} else {
			in.maxExposure = in.maxExposure;
		}
	} else if(cxt->cur_flicker == 1) {
		if(in.maxExposure > 500000) {
			in.maxExposure = 500000;
		} else {
			in.maxExposure = in.maxExposure;
		}
	}
	ISP_LOGV("flash_cur_flicker %d, max_exp %f", cxt->cur_flicker, in.maxExposure);
	in.minGain = current_status->ae_table->again[current_status->ae_table->min_index];
	in.maxGain = current_status->ae_table->again[current_status->ae_table->max_index];
	in.minCapGain = current_status->ae_table->again[current_status->ae_table->min_index];
	in.maxCapGain = current_status->ae_table->again[current_status->ae_table->max_index];
	in.minCapExposure = current_status->ae_table->exposure[current_status->ae_table->min_index] * current_status->line_time / SENSOR_LINETIME_BASE;
	in.maxCapExposure = current_status->ae_table->exposure[current_status->ae_table->max_index] * current_status->line_time / SENSOR_LINETIME_BASE;
	in.aeExposure = current_status->effect_expline * current_status->line_time / SENSOR_LINETIME_BASE;
	in.aeGain = current_status->effect_gain;
	in.rGain = current_status->awb_cur_gain.r;
	in.gGain = current_status->awb_cur_gain.g;
	in.bGain = current_status->awb_cur_gain.b;
	in.isFlash = 0;				/*need to check the meaning */
	in.flickerMode = current_status->settings.flicker;
	in.staW = cxt->cur_status.win_num.w;
	in.staH = cxt->cur_status.win_num.h;
	blk_num = cxt->cur_status.win_num.w * cxt->cur_status.win_num.h;
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
	in.otp_rdm_r = cxt->awb_otp_info.rdm_stat_info.g;
	in.otp_rdm_r = cxt->awb_otp_info.rdm_stat_info.b;

	ISP_LOGV("gldn-(%d, %d, %d), rdm-(%d, %d, %d)\n",
			cxt->awb_otp_info.gldn_stat_info.r, cxt->awb_otp_info.gldn_stat_info.g, cxt->awb_otp_info.gldn_stat_info.b,
			cxt->awb_otp_info.rdm_stat_info.r, cxt->awb_otp_info.rdm_stat_info.g, cxt->awb_otp_info.rdm_stat_info.b);
	
	rtn = flash_pfStart(cxt->flash_alg_handle, &in, &out);
	out.nextExposure *= SENSOR_LINETIME_BASE;	//Andy.lin temp code!!!
	current_status->settings.manual_mode = 0;
	current_status->settings.table_idx = 0;	
	current_status->settings.exp_line = (cmr_u32) (out.nextExposure / cxt->cur_status.line_time + 0.5);
	current_status->settings.gain = out.nextGain;
	cxt->pre_flash_level1 = out.preflahLevel1;
	cxt->pre_flash_level2 = out.preflahLevel2;
	ISP_LOGV("ae_flash pre_b: flashbefore(%d, %d), preled_level(%d, %d), preflash(%d, %d), flicker:%s\n",
			 current_status->effect_expline, current_status->effect_gain, out.preflahLevel1, out.preflahLevel2, current_status->settings.exp_line, out.nextGain, in.flickerMode == 0 ? "50Hz" : "60Hz");

	cxt->flash_last_exp_line = current_status->settings.exp_line;
	cxt->flash_last_gain = current_status->settings.gain;

	return rtn;
}

static cmr_s32 flash_estimation(struct ae_ctrl_cxt *cxt)
{
	cmr_s32 rtn = AE_SUCCESS;
	cmr_u32 blk_num = 0;
	struct Flash_pfOneIterationInput *in = NULL;
	struct Flash_pfOneIterationOutput out;
	struct ae_alg_calc_param *current_status = &cxt->cur_status;
	memset((void *)&out, 0, sizeof(out));

	ISP_LOGV("pre_flash_skip %d, aem_effect_delay %d", cxt->pre_flash_skip, cxt->aem_effect_delay);

	if (1 == cxt->flash_esti_result.isEnd) {
		goto EXIT;
	}
#if 0
	orig_ev = ((cmr_s64) cxt->flash_last_exp_line) * cxt->flash_last_gain;
	cur_ev = ((cmr_s64) current_status->effect_expline) * current_status->effect_gain;
	ev_delta = abs((cmr_s32) (cur_ev - orig_ev));
	if (orig_ev) {
		ratio = (cmr_s32) (ev_delta * 100 / orig_ev);
		if (ratio < 5) {
			ISP_LOGV("ae_flash esti: sensor param do not take effect, ratio: %d, skip\n", ratio);
			return rtn;
		}
	}
#else
if ((cxt->flash_last_exp_line != current_status->effect_expline) || (1.0 * cxt->flash_last_gain / current_status->effect_gain < 0.97) || (1.0 * cxt->flash_last_gain / current_status->effect_gain > 1.03)) {
		ISP_LOGI("ae_flash esti: sensor param do not take effect, (%d, %d)-(%d, %d)skip\n",
				 cxt->flash_last_exp_line, cxt->flash_last_gain, current_status->effect_expline, current_status->effect_gain);
		goto EXIT;
	}
#endif
	/* preflash climbing time */
	if (0 < cxt->pre_flash_skip) {
		cxt->pre_flash_skip--;
		goto EXIT;
	}

	cxt->aem_effect_delay++;
	if (cxt->aem_effect_delay == cxt->cur_param->flash_control_param.aem_effect_delay) {
		cxt->aem_effect_delay = 0;

		blk_num = cxt->cur_status.win_num.w * cxt->cur_status.win_num.h;
		in = &cxt->flash_esti_input;
		in->aeExposure = current_status->effect_expline * current_status->line_time / SENSOR_LINETIME_BASE;
		in->aeGain = current_status->effect_gain;
		in->staW = cxt->cur_status.win_num.w;
		in->staH = cxt->cur_status.win_num.h;
		in->isFlash = 1;		/*need to check the meaning */
		memcpy((cmr_handle *) & in->rSta[0], (cmr_u16 *) & cxt->aem_stat_rgb[0], sizeof(in->rSta));
		memcpy((cmr_handle *) & in->gSta[0], ((cmr_u16 *) & cxt->aem_stat_rgb[0] + blk_num), sizeof(in->gSta));
		memcpy((cmr_handle *) & in->bSta[0], ((cmr_u16 *) & cxt->aem_stat_rgb[0] + 2 * blk_num), sizeof(in->bSta));

		flash_pfOneIteration(cxt->flash_alg_handle, in, &out);

		out.nextExposure *= SENSOR_LINETIME_BASE;	//Andy.lin temp code!!!
		out.captureExposure *= SENSOR_LINETIME_BASE;	//Andy.lin temp code!!!
		current_status->settings.manual_mode = 0;
		current_status->settings.table_idx = 0;
		current_status->settings.exp_line = (cmr_u32) (out.nextExposure / cxt->cur_status.line_time + 0.5);
		current_status->settings.gain = out.nextGain;
		cxt->flash_esti_result.isEnd = 0;
		ISP_LOGI("ae_flash esti: doing %d, %d", cxt->cur_status.settings.exp_line, cxt->cur_status.settings.gain);

		if (1 == out.isEnd) {
			ISP_LOGI("ae_flash esti: isEnd:%d, cap(%d, %d), led(%d, %d), rgb(%d, %d, %d)\n",
					 out.isEnd, current_status->settings.exp_line, out.captureGain, out.captureFlahLevel1, out.captureFlahLevel2, out.captureRGain, out.captureGGain, out.captureBGain);

			/*save the flash estimation results */
			cxt->flash_esti_result = out;
		}

		cxt->flash_last_exp_line = current_status->settings.exp_line;
		cxt->flash_last_gain = current_status->settings.gain;
	}
  EXIT:
	current_status->settings.manual_mode = 0;
	current_status->settings.table_idx = 0;
	current_status->settings.exp_line = cxt->flash_last_exp_line;
	current_status->settings.gain = cxt->flash_last_gain;
	return rtn;
}

static cmr_s32 flash_high_flash_reestimation(struct ae_ctrl_cxt *cxt)
{
	cmr_s32 rtn = AE_SUCCESS;
	cmr_u32 blk_num = 0;
	struct Flash_mfCalcInput *input = &cxt->flash_main_esti_input;
	struct Flash_mfCalcOutput *output = &cxt->flash_main_esti_result;
	memset((void *)input, 0, sizeof(struct Flash_mfCalcInput));
	blk_num = cxt->cur_status.win_num.w * cxt->cur_status.win_num.h;
	input->staW = cxt->cur_status.win_num.w;
	input->staH = cxt->cur_status.win_num.h;
	input->rGain = cxt->cur_status.awb_gain.r;
	input->gGain = cxt->cur_status.awb_gain.g;
	input->bGain = cxt->cur_status.awb_gain.b;
	input->wb_mode = cxt->cur_status.awb_mode;
	memcpy((cmr_handle *) & input->rSta[0], (cmr_u16 *) & cxt->aem_stat_rgb[0], sizeof(input->rSta));
	memcpy((cmr_handle *) & input->gSta[0], ((cmr_u16 *) & cxt->aem_stat_rgb[0] + blk_num), sizeof(input->gSta));
	memcpy((cmr_handle *) & input->bSta[0], ((cmr_u16 *) & cxt->aem_stat_rgb[0] + 2 * blk_num), sizeof(input->bSta));
	flash_mfCalc(cxt->flash_alg_handle, input, output);
	ISP_LOGV("high flash wb gain: %d, %d, %d\n", cxt->flash_main_esti_result.captureRGain, cxt->flash_main_esti_result.captureGGain, cxt->flash_main_esti_result.captureBGain);

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
		goto out;
		break;
	}

	if (flash_level1 == -1) {
		cxt->ae_leds_ctrl.led0_ctrl = 0;
	} else {
		cxt->ae_leds_ctrl.led0_ctrl = 1;
		cfg.led_idx = 1;
		element.index = flash_level1;
		cxt->isp_ops.flash_set_charge(cxt->isp_ops.isp_handler, &cfg, &element);
	}

	if (flash_level2 == -1) {
		cxt->ae_leds_ctrl.led1_ctrl = 0;
	} else {
		cxt->ae_leds_ctrl.led1_ctrl = 1;
		cfg.led_idx = 2;
		element.index = flash_level2;
		cxt->isp_ops.flash_set_charge(cxt->isp_ops.isp_handler, &cfg, &element);
	}

  out:
	return rtn;
}

static cmr_s32 ae_pre_process(struct ae_ctrl_cxt *cxt)
{
	cmr_s32 rtn = AE_SUCCESS;
	cmr_u32 Bavoid_rewrite_prev_param = 0;
	struct ae_alg_calc_param *current_status = &cxt->cur_status;

	ISP_LOGV("ae_video_fps_thr_high: %d, ae_video_fps_thr_low: %d\n", cxt->cur_param->ae_video_fps.ae_video_fps_thr_high, cxt->cur_param->ae_video_fps.ae_video_fps_thr_low);

	if (AE_WORK_MODE_VIDEO == current_status->settings.work_mode) {
		ISP_LOGV("%d, %d, %d, %d, %d", cxt->sync_cur_result.cur_bv, current_status->settings.min_fps, current_status->settings.max_fps, cxt->fps_range.min, cxt->fps_range.max);
		if (cxt->cur_param->ae_video_fps.ae_video_fps_thr_high < cxt->sync_cur_result.cur_bv) {
			current_status->settings.max_fps = cxt->fps_range.max;
			current_status->settings.min_fps = cxt->fps_range.max;
		} else if (cxt->cur_param->ae_video_fps.ae_video_fps_thr_low < cxt->sync_cur_result.cur_bv) {
			current_status->settings.max_fps = cxt->fps_range.max;
			current_status->settings.min_fps = cxt->fps_range.min;
		} else {
			current_status->settings.max_fps = cxt->fps_range.min;
			current_status->settings.min_fps = cxt->fps_range.min;
		}

	} else {
		current_status->settings.max_fps = cxt->fps_range.max;
		current_status->settings.min_fps = cxt->fps_range.min;
	}

	if (0 < cxt->cur_status.settings.flash) {
		ISP_LOGV("ae_flash: flicker lock to %d in flash: %d\n", cxt->cur_flicker, current_status->settings.flash);
		current_status->settings.flicker = cxt->cur_flicker;
	}

	if (0 != cxt->flash_ver) {
		if ((FLASH_PRE_BEFORE == current_status->settings.flash) || (FLASH_PRE == current_status->settings.flash)) {
			rtn = ae_stats_data_preprocess((cmr_u32 *) & cxt->sync_aem[0], (cmr_u16 *) & cxt->aem_stat_rgb[0], cxt->monitor_cfg.blk_size, cxt->cur_status.win_num, current_status->monitor_shift);
		}

		if ((FLASH_PRE_BEFORE == current_status->settings.flash) && !cxt->send_once[0]) {
			rtn = flash_pre_start(cxt);
		}

		if (FLASH_PRE == current_status->settings.flash) {
			rtn = flash_estimation(cxt);
		}

		if ((FLASH_MAIN_BEFORE == current_status->settings.flash) || (FLASH_MAIN == current_status->settings.flash)) {
			if (cxt->flash_esti_result.isEnd) {
				if (1 == cxt->exposure_compensation.ae_compensation_flag) {
					if(current_status->settings.manual_mode !=2){
						ISP_LOGV("manual_mode:%d,table_idx:%d",current_status->settings.manual_mode,cxt->cur_status.settings.table_idx);
						current_status->settings.manual_mode = 1;
						if(0==cxt->cur_status.settings.table_idx){
							struct ae_exp_compensation exp_comp;
							exp_comp.comp_val=0;
							exp_comp.comp_range.min=-16;
							exp_comp.comp_range.max=16;
							exp_comp.step_numerator=1;
							exp_comp.step_denominator=8;
							ae_set_exposure_compensation(cxt,&exp_comp);
						}

					}
				} else {
					current_status->settings.manual_mode = 0;
					current_status->settings.table_idx = 0;
					current_status->settings.exp_line = (cmr_u32) (cxt->flash_esti_result.captureExposure / current_status->line_time + 0.5);
					current_status->settings.gain = cxt->flash_esti_result.captureGain;
					/*
					 * during mainflash period, we should do updating param earlily.
					 * [T0] ae_flash1_callback: do-capture
					 *		we need update preview scene's ae param to sensor,which takes 2 frames'time.
					 * [T0+2] hal need do capture in the T0~T0+2
					 * [hainan.ren]
					 */
					if (((FLASH_MAIN_BEFORE_RECEIVE == cxt->cur_result.flash_status && FLASH_MAIN_BEFORE == current_status->settings.flash)
	|| (FLASH_MAIN_RECEIVE == cxt->cur_result.flash_status && FLASH_MAIN == current_status->settings.flash))
	&& (cxt->send_once[4] > (cxt->cur_param->flash_control_param.main_capture_count + cxt->cur_param->flash_control_param.main_set_count) + 2)) {
						Bavoid_rewrite_prev_param=1;
					}
					if (Bavoid_rewrite_prev_param==0) {
						current_status->settings.manual_mode = 0;
						current_status->settings.table_idx = 0;
						current_status->settings.exp_line = (cmr_u32) (cxt->flash_esti_result.captureExposure / current_status->line_time + 0.5);
						current_status->settings.gain = cxt->flash_esti_result.captureGain;
					}
					ISP_LOGV("table_idx:%d",cxt->cur_status.settings.table_idx);
				}

			} else {
				ISP_LOGE("ae_flash estimation does not work well");
			}
			if (FLASH_MAIN == current_status->settings.flash) {
				rtn = ae_stats_data_preprocess((cmr_u32 *) & cxt->sync_aem[0], (cmr_u16 *) & cxt->aem_stat_rgb[0], cxt->cur_status.win_size, cxt->cur_status.win_num, current_status->monitor_shift);
				flash_high_flash_reestimation(cxt);
				ISP_LOGV("ae_flash: main flash calc, rgb gain %d, %d, %d\n", cxt->flash_main_esti_result.captureRGain, cxt->flash_main_esti_result.captureGGain,
						 cxt->flash_main_esti_result.captureBGain);
			}

			ISP_LOGV("ae_flash main_b prinf: %.f, %d, %d, %d\n", cxt->flash_esti_result.captureExposure, current_status->settings.exp_line, current_status->settings.gain, current_status->line_time);
		}
	}

	return rtn;
}
static cmr_s32 sensor_param_updating_interface(struct ae_ctrl_cxt *cxt)
{
	cmr_s32 rtn = AE_SUCCESS;

	cxt->cur_status.settings.flicker = cxt->cur_flicker;
	if (cxt->camera_id && cxt->fdae.pause)
		cxt->fdae.pause = 0;
	cxt->cur_result.wts.exposure_time = cxt->flash_backup.exp_time;
	cxt->cur_result.wts.cur_exp_line = cxt->flash_backup.exp_line;
	cxt->cur_result.wts.cur_again = cxt->flash_backup.gain;
	cxt->cur_result.wts.cur_dummy = cxt->flash_backup.dummy;
	cxt->cur_result.wts.cur_index = cxt->flash_backup.cur_index;
	cxt->cur_result.wts.cur_bv = cxt->flash_backup.bv;
	cxt->sync_cur_result.wts.exposure_time = cxt->cur_result.wts.exposure_time;
	cxt->sync_cur_result.wts.cur_exp_line = cxt->cur_result.wts.cur_exp_line;
	cxt->sync_cur_result.wts.cur_again = cxt->cur_result.wts.cur_again;
	cxt->sync_cur_result.wts.cur_dummy = cxt->cur_result.wts.cur_dummy;
	cxt->sync_cur_result.wts.cur_index = cxt->cur_result.wts.cur_index;
	cxt->sync_cur_result.wts.cur_bv = cxt->cur_result.wts.cur_bv;

	memset((void *)&cxt->exp_data, 0, sizeof(cxt->exp_data));
	cxt->exp_data.lib_data.exp_line = cxt->sync_cur_result.wts.cur_exp_line;
	cxt->exp_data.lib_data.exp_time = cxt->sync_cur_result.wts.exposure_time;
	cxt->exp_data.lib_data.gain = cxt->sync_cur_result.wts.cur_again;
	cxt->exp_data.lib_data.dummy = cxt->sync_cur_result.wts.cur_dummy;
	cxt->exp_data.lib_data.line_time = cxt->cur_status.line_time;

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

	struct ae_alg_calc_param *current_status = &cxt->sync_cur_status;

	/* for flash algorithm 0 */
	if (0 == cxt->flash_ver) {
		if (FLASH_PRE_BEFORE_RECEIVE == cxt->cur_result.flash_status && FLASH_PRE_BEFORE == current_status->settings.flash) {
			ISP_LOGI("ae_flash0_status shake_1");
			if (0 == cxt->send_once[0]) {
				cxt->send_once[0]++;
				cb_type = AE_CB_CONVERGED;
				(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, cb_type, NULL);
				ISP_LOGI("ae_flash0_callback do-pre-open!\r\n");
			}
		}

		if (FLASH_PRE_RECEIVE == cxt->cur_result.flash_status && FLASH_PRE == current_status->settings.flash) {
			ISP_LOGI("ae_flash0_status shake_2 %d %d %d %d", cxt->cur_result.wts.stable, cxt->cur_result.cur_lum, cxt->cur_result.flash_effect, cxt->send_once[3]);
			cxt->send_once[3]++;
			if (cxt->cur_result.wts.stable || cxt->send_once[3] > AE_FLASH_CALC_TIMES) {
				if (0 == cxt->send_once[1]) {
					cxt->send_once[1]++;
					cb_type = AE_CB_CONVERGED;
					(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, cb_type, NULL);
					ISP_LOGI("ae_flash0_callback do-pre-close!\r\n");
				}
				cxt->flash_effect = cxt->cur_result.flash_effect;
			}
			cxt->send_once[0] = 0;
		}

		if (FLASH_PRE_AFTER_RECEIVE == cxt->cur_result.flash_status && FLASH_PRE_AFTER == current_status->settings.flash) {
			ISP_LOGI("ae_flash0_status shake_3 %d", cxt->cur_result.flash_effect);
			cxt->cur_status.settings.flash = FLASH_NONE;
			cxt->send_once[3] = 0;
			cxt->send_once[1] = 0;
		}

		if (FLASH_MAIN_BEFORE_RECEIVE == cxt->cur_result.flash_status && FLASH_MAIN_BEFORE == current_status->settings.flash) {
			ISP_LOGI("ae_flash0_status shake_4 %d %d", cxt->cur_result.wts.stable, cxt->cur_result.cur_lum);
			if (cxt->cur_result.wts.stable) {
				if (1 == cxt->send_once[2]) {
					cb_type = AE_CB_CONVERGED;
					(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, cb_type, NULL);
					ISP_LOGI("ae_flash0_callback do-main-flash!\r\n");
				} else if (4 == cxt->send_once[2]) {
					cb_type = AE_CB_CONVERGED;
					(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, cb_type, NULL);
					ISP_LOGI("ae_flash0_callback do-capture!\r\n");
				} else {
					ISP_LOGI("ae_flash0 wait-capture!\r\n");
				}
				cxt->send_once[2]++;
			}
		}

		if (FLASH_MAIN_AFTER_RECEIVE == cxt->cur_result.flash_status && FLASH_MAIN_AFTER == current_status->settings.flash) {

			ISP_LOGI("ae_flash0_status shake_6");
			cxt->cur_status.settings.flash = FLASH_NONE;
			cxt->send_once[0] = cxt->send_once[1] = cxt->send_once[2] = cxt->send_once[3] = 0;
		}
	} else {
		/* for new flash algorithm (flash algorithm1, dual flash) */
		ISP_LOGV("pre_open_count %d, pre_close_count %d",cxt->cur_param->flash_control_param.pre_open_count,cxt->cur_param->flash_control_param.pre_close_count);

		if (FLASH_PRE_BEFORE_RECEIVE == cxt->cur_result.flash_status && FLASH_PRE_BEFORE == current_status->settings.flash) {
			cxt->send_once[0]++;
			ISP_LOGI("ae_flash1_status shake_1");
			if (cxt->ebd_support)
				cxt->ebd_flash_stable_flag = 1;
			if (cxt->cur_param->flash_control_param.pre_open_count == cxt->send_once[0]) {
				ISP_LOGI("ae_flash p: led level: %d, %d\n", cxt->pre_flash_level1, cxt->pre_flash_level2);
				rtn = ae_set_flash_charge(cxt, AE_FLASH_TYPE_PREFLASH);

				cb_type = AE_CB_CONVERGED;
				(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, cb_type, NULL);
				cxt->cur_result.flash_status = FLASH_NONE;	/*flash status reset */
				ISP_LOGI("ae_flash1_callback do-pre-open!\r\n");
			}
		}

		if (FLASH_PRE_RECEIVE == cxt->cur_result.flash_status && FLASH_PRE == current_status->settings.flash) {
			ISP_LOGI("ae_flash1_status shake_2 %d %d %d", cxt->cur_result.wts.stable, cxt->cur_result.cur_lum, cxt->cur_result.flash_effect);
			cxt->send_once[3]++;	//prevent flash_pfOneIteration time out
			if (cxt->flash_esti_result.isEnd || cxt->send_once[3] > AE_FLASH_CALC_TIMES) {
				if (cxt->cur_param->flash_control_param.pre_close_count == cxt->send_once[1]) {
					cxt->send_once[3] = 0;
					cb_type = AE_CB_CONVERGED;
					(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, cb_type, NULL);
					ISP_LOGI("ae_flash1_callback do-pre-close!\r\n");
					cxt->cur_result.flash_status = FLASH_NONE;	/*flash status reset */
				}
				cxt->send_once[1]++;
			}
			cxt->send_once[0] = 0;
		}

		if (FLASH_PRE_AFTER_RECEIVE == cxt->cur_result.flash_status && FLASH_PRE_AFTER == current_status->settings.flash) {
			ISP_LOGI("ae_flash1_status shake_3 %d", cxt->cur_result.flash_effect);
			cxt->cur_status.settings.flash = FLASH_NONE;	/*flash status reset */
			cxt->send_once[0] = cxt->send_once[1] = cxt->send_once[2] = cxt->send_once[3] = cxt->send_once[4] = cxt->send_once[5] = 0;
		}

		if (1 < cxt->capture_skip_num)
			main_flash_counts = cxt->capture_skip_num - cxt->send_once[2];
		else
			main_flash_counts = -1;

		main_flash_capture_counts = cxt->cur_param->flash_control_param.main_capture_count + cxt->cur_param->flash_control_param.main_set_count;

		ISP_LOGV("main_flash_counts: %d, capture_skip_num: %d", main_flash_counts, cxt->capture_skip_num);
		ISP_LOGV("main_set_count: %d, main_capture_count: %d, main_flash_capture_counts: %d",
			cxt->cur_param->flash_control_param.main_set_count,
			cxt->cur_param->flash_control_param.main_capture_count,
			main_flash_capture_counts);

		if ((FLASH_MAIN_BEFORE_RECEIVE == cxt->cur_result.flash_status && FLASH_MAIN_BEFORE == current_status->settings.flash)
			|| (FLASH_MAIN_RECEIVE == cxt->cur_result.flash_status && FLASH_MAIN == current_status->settings.flash)) {
			ISP_LOGI("ae_flash1_status shake_4 %d, %d, cnt: %d\n", cxt->send_once[2], cxt->send_once[4], main_flash_counts);

			if (cxt->cur_param->flash_control_param.main_set_count == cxt->send_once[4]) {
				ISP_LOGI("ae_flash m: led level: %d, %d\n", cxt->flash_esti_result.captureFlahLevel1, cxt->flash_esti_result.captureFlahLevel2);
				rtn = ae_set_flash_charge(cxt, AE_FLASH_TYPE_MAIN);
				cb_type = AE_CB_CONVERGED;
				(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, cb_type, NULL);
				ISP_LOGI("ae_flash1_callback do-main-flash!\r\n");
			} else if (main_flash_capture_counts == cxt->send_once[4]) {
				if(cxt->ebd_support) {
					if((cxt->ebd_stable_flag) || (2 < cxt->send_once[5])) {
						cb_type = AE_CB_CONVERGED;
						(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, cb_type, NULL);
						cxt->cur_result.flash_status = FLASH_NONE;	/*flash status reset */
						ISP_LOGI("ae_flash1_callback do-capture!\r\n");
					} else {
						cxt->send_once[4]--;
						cxt->send_once[5]++;
					}
				} else {
					cb_type = AE_CB_CONVERGED;
					(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, cb_type, NULL);
					cxt->cur_result.flash_status = FLASH_NONE;	/*flash status reset */
					ISP_LOGI("ae_flash1_callback do-capture!\r\n");
				}
			} else if (main_flash_capture_counts + 2 == cxt->send_once[4]) {
				sensor_param_updating_interface(cxt);
				ISP_LOGI("ae_flash1_callback update next prev-param!\r\n");

				struct ae_flash_cfg cfg;
				cfg.led_idx = 0;
				cfg.type = AE_FLASH_TYPE_MAIN;
				cfg.led0_enable = 0;
				cfg.led1_enable = 0;
				cxt->isp_ops.flash_ctrl(cxt->isp_ops.isp_handler, &cfg, NULL);
			} else {
				if (1 > cxt->send_once[4]) {
					ISP_LOGI("ae_flash1 wait-main-flash!\r\n");
				} else if (cxt->cur_param->flash_control_param.main_capture_count < cxt->send_once[4]) {
					ISP_LOGI("ae_flash1 wait-get-image!\r\n");
				} else {
					ISP_LOGI("ae_flash1 wait-capture!\r\n");
				}
			}

		if (0 >= main_flash_counts) {
			cxt->send_once[4]++;
		}
		cxt->send_once[2]++;

		/*write flash awb gain*/
		if (1 == cxt->flash_esti_result.isEnd ) {
			if (cxt->isp_ops.set_wbc_gain) {
				struct ae_alg_rgb_gain awb_m_b_flash_gain;
				awb_m_b_flash_gain.r = cxt->flash_esti_result.captureRGain;
				awb_m_b_flash_gain.g = cxt->flash_esti_result.captureGGain;
				awb_m_b_flash_gain.b = cxt->flash_esti_result.captureBGain;
				cxt->isp_ops.set_wbc_gain(cxt->isp_ops.isp_handler, &awb_m_b_flash_gain);
				ISP_LOGV("flash_cap awb_m r %d, g %d, b %d", awb_m_b_flash_gain.r, awb_m_b_flash_gain.g, awb_m_b_flash_gain.b);
				}
			}

		if (1 == cxt->flash_main_esti_result.isEnd) {
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

		if (FLASH_MAIN_AFTER_RECEIVE == cxt->cur_result.flash_status && FLASH_MAIN_AFTER == current_status->settings.flash) {
			ISP_LOGI("ae_flash1_status shake_6");
			cxt->cur_status.settings.flash = FLASH_NONE;	/*flash status reset */
			cxt->send_once[0] = cxt->send_once[1] = cxt->send_once[2] = cxt->send_once[3] = cxt->send_once[4] = cxt->send_once[5]= 0;
			cxt->ebd_flash_stable_flag = 0;
			if (0 != cxt->flash_ver) {
				flash_finish(cxt);
			}
		}
	}

	/* for front flash algorithm (LED+LCD) */
	if (cxt->camera_id == 1 && cxt->cur_status.settings.flash == FLASH_LED_AUTO) {
		if ((cxt->sync_cur_result.cur_bv <= cxt->flash_swith.led_thr_down) && cxt->sync_cur_status.led_state == 0) {
			led_eb = 1;
			cxt->cur_status.led_state = 1;
			cb_type = AE_CB_LED_NOTIFY;
			(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, cb_type, &led_eb);
			ISP_LOGV("ae_flash1_callback do-led-open!\r\n");
			ISP_LOGV("camera_id %d, flash_status %d, cur_bv %d, led_open_thr %d, led_state %d",
					 cxt->camera_id, cxt->cur_status.settings.flash, cxt->sync_cur_result.cur_bv, cxt->flash_swith.led_thr_down, cxt->sync_cur_status.led_state);
		}

		if ((cxt->sync_cur_result.cur_bv >= cxt->flash_swith.led_thr_up) && cxt->sync_cur_status.led_state == 1) {
			led_eb = 0;
			cxt->cur_status.led_state = 0;
			cb_type = AE_CB_LED_NOTIFY;
			(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, cb_type, &led_eb);
			ISP_LOGV("ae_flash1_callback do-led-close!\r\n");
			ISP_LOGV("camera_id %d, flash_status %d, cur_bv %d, led_close_thr %d, led_state %d",
					 cxt->camera_id, cxt->cur_status.settings.flash, cxt->sync_cur_result.cur_bv, cxt->flash_swith.led_thr_up, cxt->sync_cur_status.led_state);
		}
	}

	/* notify APP if need autofocus or not, just in flash auto mode */
	if (cxt->camera_id == 0) {
		ISP_LOGV("flash open thr=%d, flash close thr=%d, bv=%d, flash_fired=%d, delay_cnt=%d",
				 cxt->flash_swith.led_thr_down, cxt->flash_swith.led_thr_up, cxt->sync_cur_result.cur_bv, cxt->sync_cur_status.flash_fired, cxt->delay_cnt);

		if (cxt->sync_cur_result.cur_bv < cxt->flash_swith.led_thr_down) {
			cxt->delay_cnt = 0;
			flash_fired = 1;
			cxt->cur_status.flash_fired = 1;
			cb_type = AE_CB_FLASH_FIRED;
			(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, cb_type, &flash_fired);
			ISP_LOGV("flash will fire!\r\n");
		}

		if ((cxt->sync_cur_result.cur_bv > cxt->flash_swith.led_thr_up) && (cxt->sync_cur_status.flash_fired == 1)) {
			if (cxt->delay_cnt == cxt->cur_param->flash_control_param.main_flash_notify_delay) {
				flash_fired = 0;
				cxt->cur_status.flash_fired = 0;
				cb_type = AE_CB_FLASH_FIRED;
				(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, cb_type, &flash_fired);
				ISP_LOGV("flash will not fire!\r\n");
			}
			cxt->delay_cnt++;
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
	struct tg_ae_ctrl_alc_log *debug_info_result = (struct tg_ae_ctrl_alc_log *)result;

	if (result) {
		memset((cmr_handle) & debug_info_in, 0, sizeof(struct ae_debug_info_packet_in));
		memset((cmr_handle) & debug_info_out, 0, sizeof(struct ae_debug_info_packet_out));

		alg_id_ptr = ae_debug_info_get_lib_version();
		debug_info_in.alg_id = cxt->cur_status.alg_id;
		debug_info_in.aem_stats = (cmr_handle) cxt->sync_aem;
		debug_info_in.base_aem_stats = (cmr_handle) cxt->cur_status.base_img;
		debug_info_in.alg_status = (cmr_handle) & cxt->sync_cur_status;
		debug_info_in.alg_results = (cmr_handle) & cxt->sync_cur_result;
		rtn = ae_misc_ioctrl(cxt->misc_handle, AEC_GET_Q_ARRAY, cxt->history_param, NULL);
		rtn = ae_misc_ioctrl(cxt->misc_handle, AEC_GET_PRV_PARAM, cxt->history_param, NULL);
		rtn = ae_misc_ioctrl(cxt->misc_handle, AEC_GET_AE_TABLE, cxt->history_param, NULL);
		debug_info_in.history_param = (cmr_handle) & cxt->history_param[0];
		debug_info_in.packet_buf = (cmr_handle) & cxt->debug_info_buf[0];
		memcpy((cmr_handle) & debug_info_in.alg_version[0], alg_id_ptr, sizeof(debug_info_in.alg_version));
		memcpy((cmr_handle) & debug_info_in.flash_version[0], &cxt->flash_ver, sizeof(debug_info_in.flash_version));

		if(debug_info_in.alg_id == 3)
			rtn = ae_simulation_info_packet((cmr_handle) & debug_info_in, (cmr_handle) & debug_info_out);
		else
			rtn = ae_debug_info_packet((cmr_handle) & debug_info_in, (cmr_handle) & debug_info_out);
		/*add flash debug information */
		memcpy((cmr_handle) & cxt->debug_info_buf[debug_info_out.size], (cmr_handle) & cxt->flash_debug_buf[0], cxt->flash_buf_len);

		debug_info_result->log = (cmr_u8 *) debug_info_in.packet_buf;
		debug_info_result->size = debug_info_out.size + cxt->flash_buf_len;
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

static cmr_s32 ae_make_calc_result(struct ae_ctrl_cxt *cxt, struct ae_alg_calc_result *alg_rt, struct ae_calc_results *result)
{
	cmr_s32 rtn = AE_SUCCESS;
	cmr_u32 i = 0x00;

	result->ae_output.cur_lum = alg_rt->cur_lum;
	result->ae_output.cur_again = alg_rt->wts.cur_again;
	result->ae_output.cur_exp_line = alg_rt->wts.cur_exp_line;
	result->ae_output.line_time = cxt->cur_status.line_time;
	result->ae_output.is_stab = alg_rt->wts.stable;
	result->ae_output.target_lum = alg_rt->target_lum;
	result->ae_output.target_lum_ori = alg_rt->target_lum_ori;
	result->ae_output.flag4idx = alg_rt->flag4idx;
	result->ae_output.face_stable = alg_rt->face_stable;
	result->ae_output.cur_bv = alg_rt->cur_bv;
	result->ae_output.exposure_time = cxt->cur_result.wts.exposure_time / AEC_LINETIME_PRECESION;
	result->ae_output.fps = alg_rt->wts.cur_fps;
	result->ae_output.reserved = alg_rt->privated_data;

	result->is_skip_cur_frame = 0;
	result->monitor_info.trim = cxt->monitor_cfg.trim;
	result->monitor_info.win_num = cxt->monitor_cfg.blk_num;
	result->monitor_info.win_size = cxt->monitor_cfg.blk_size;
	result->ae_ev.ev_index = cxt->cur_status.settings.ev_index;
	result->flash_param.captureFlashEnvRatio = cxt->flash_esti_result.captureFlashRatio;
	result->flash_param.captureFlash1ofALLRatio = cxt->flash_esti_result.captureFlash1Ratio;

	for (i = 0; i < cxt->cur_param->ev_table.diff_num; i++) {
		result->ae_ev.ev_tab[i] = cxt->cur_param->target_lum + cxt->cur_param->ev_table.lum_diff[i];
	}

	ae_get_iso(cxt, &result->ae_output.cur_iso);

	return rtn;
}

static cmr_s32 ae_make_isp_result(struct ae_ctrl_cxt *cxt, struct ae_alg_calc_result *alg_rt, struct ae_ctrl_callback_in *result)
{
	cmr_s32 rtn = AE_SUCCESS;
	cmr_u32 i = 0x00;

	result->ae_output.cur_lum = alg_rt->cur_lum;
	result->ae_output.cur_again = alg_rt->wts.cur_again;
	result->ae_output.cur_exp_line = alg_rt->wts.cur_exp_line;
	result->ae_output.line_time = cxt->cur_status.line_time;
	result->ae_output.is_stab = alg_rt->wts.stable;
	result->ae_output.target_lum = alg_rt->target_lum;
	result->ae_output.target_lum_ori = alg_rt->target_lum_ori;
	result->ae_output.flag4idx = alg_rt->flag4idx;
	result->ae_output.face_stable = alg_rt->face_stable;
	result->ae_output.cur_bv = alg_rt->cur_bv;
	result->ae_output.exposure_time = cxt->cur_result.wts.exposure_time / AEC_LINETIME_PRECESION;
	result->ae_output.fps = alg_rt->wts.cur_fps;
	result->ae_output.reserved = alg_rt->privated_data;

	result->is_skip_cur_frame = 0;
	result->monitor_info.trim = cxt->monitor_cfg.trim;
	result->monitor_info.win_num = cxt->cur_status.win_num;
	result->monitor_info.win_size = cxt->cur_status.win_size;
	result->ae_ev.ev_index = cxt->cur_status.settings.ev_index;
	result->flash_param.captureFlashEnvRatio = cxt->flash_esti_result.captureFlashRatio;
	result->flash_param.captureFlash1ofALLRatio = cxt->flash_esti_result.captureFlash1Ratio;

	for (i = 0; i < cxt->cur_param->ev_table.diff_num; i++) {
		result->ae_ev.ev_tab[i] = cxt->cur_param->target_lum + cxt->cur_param->ev_table.lum_diff[i];
	}

	ae_get_iso(cxt, &result->ae_output.cur_iso);

	return rtn;
}

#ifdef CONFIG_ISP_2_2
static void ae_mapping(struct ae_ctrl_cxt *cxt_ptr, struct match_data_param *multicam_aesync)
{
	struct sensor_otp_ae_info *ae_otp_master = NULL;
	struct sensor_otp_ae_info *ae_otp_slave = NULL;
	struct ae_alg_calc_result *ae_master_calc_out = NULL;
	struct ae_sync_out *slv_sync_result = NULL;
	cmr_u32 gain_master = 0;
	cmr_u32 gain_slave = 0;

	cmr_u32 master_line_time = 0;
	cmr_u32 slv_line_time = 0;
	cmr_u32 exp_line_slave = 0;
	cmr_u32 slave_dummy = 0;
	cmr_u32 tmp = 0;

	if (!cxt_ptr || !multicam_aesync) {
		ISP_LOGE("Static AE_sync param is NULL error!");
		return;
	}
	ISP_LOGV("Static  AE_sync mode!");
	ae_master_calc_out = &multicam_aesync->master_ae_info.ae_calc_result;
	slv_sync_result = &multicam_aesync->slave_ae_info.ae_sync_result;
	ae_otp_master = &(multicam_aesync->module_info.module_otp_info.master_ae_otp.otp_info);
	ae_otp_slave = &(multicam_aesync->module_info.module_otp_info.slave_ae_otp.otp_info);

	gain_master = ae_master_calc_out->wts.cur_again;
	gain_slave = gain_master;


	/* calculate gain */
	cmr_u32 exp_master_1x = 0;
	cmr_u32 exp_master_2x = 0;
	cmr_u32 exp_master_4x = 0;
	cmr_u32 exp_master_8x = 0;
	cmr_u32 exp_slave_1x = 0;
	cmr_u32 exp_slave_2x = 0;
	cmr_u32 exp_slave_4x = 0;
	cmr_u32 exp_slave_8x = 0;

	exp_master_1x = ae_otp_master->gain_1x_exp;
	exp_master_2x = ae_otp_master->gain_2x_exp;
	exp_master_4x = ae_otp_master->gain_4x_exp;
	exp_master_8x = ae_otp_master->gain_8x_exp;
	exp_slave_1x = ae_otp_slave->gain_1x_exp;
	exp_slave_2x = ae_otp_slave->gain_2x_exp;
	exp_slave_4x = ae_otp_slave->gain_4x_exp;
	exp_slave_8x = ae_otp_slave->gain_8x_exp;
	ISP_LOGV("Static AE_sync OTP master info:1x = %d,2x = %d,4x = %d,8x = %d.\n",exp_master_1x,exp_master_2x,exp_master_4x,exp_master_8x);
	ISP_LOGV("Static AE_sync OTP slave info:1x = %d,2x = %d,4x = %d,8x = %d.\n",exp_slave_1x,exp_slave_2x,exp_slave_4x,exp_slave_8x);

	if (exp_master_1x && exp_master_2x && exp_master_4x && exp_master_8x && exp_slave_1x && exp_slave_2x && exp_slave_4x && exp_slave_8x) {
		if (gain_master >= 8 * 128) {
			gain_slave = gain_master * exp_slave_8x / exp_master_8x;
		} else if (gain_master >= 4 * 128) {
			gain_slave = gain_master * (exp_slave_4x + exp_slave_8x) / (exp_master_4x + exp_master_8x);
		} else if (gain_master >= 2 * 128) {
			gain_slave = gain_master * (exp_slave_2x + exp_slave_4x) / (exp_master_2x + exp_master_4x);
		} else {
			gain_slave = gain_master * (exp_slave_1x + exp_slave_2x) / (exp_master_1x + exp_master_2x);
		}
	}

	/* calculate exposure line */
	master_line_time = multicam_aesync->module_info.module_sensor_info.master_sensor_info.line_time;
	slv_line_time = multicam_aesync->module_info.module_sensor_info.slave_sensor_info.line_time;

	if (slv_line_time > 0)
		exp_line_slave = ae_master_calc_out->wts.exposure_time / slv_line_time;
	else
		exp_line_slave = multicam_aesync->module_info.module_sensor_info.slave_sensor_info.min_exp_line;

	float iso_ratio = 1.0f;
	if ((exp_master_1x != 0) && (exp_slave_1x != 0)) {
		iso_ratio = ((float)exp_slave_1x) / ((float)exp_master_1x);
	}
	ISP_LOGV("Static AE_sync iso_ratio:%f.\n",iso_ratio);

	if (iso_ratio < 1.0f) {
		if ((cmr_u32) (gain_master * iso_ratio) > 128) {
			gain_slave = (cmr_u32) (gain_master * iso_ratio);
		} else {
			exp_line_slave = (cmr_u32) (exp_line_slave * iso_ratio);
		}
	} else {
		gain_slave = (cmr_u32) (gain_master * iso_ratio);
	}

	/* calculate dummy line */
	tmp = master_line_time * (ae_master_calc_out->wts.cur_exp_line + ae_master_calc_out->wts.cur_dummy);
	slave_dummy = tmp / slv_line_time - exp_line_slave;

	/*fulfill slave sync result */
	slv_sync_result->slave_ae.wts.cur_again = gain_slave;
	slv_sync_result->slave_ae.wts.exposure_time = ae_master_calc_out->wts.exposure_time;
	slv_sync_result->slave_ae.wts.cur_exp_line = exp_line_slave;
	slv_sync_result->slave_ae.wts.cur_dummy = slave_dummy;

	// TODO: calculate this value, or remove it!
	slv_sync_result->slave_ae.wts.cur_dgain = ae_master_calc_out->wts.cur_dgain;

	/* set flag to 1 if need update, otherwise  set to 0 */
	slv_sync_result->updata_flag = 1;
}

static cmr_s32 ae_dual_cam_sync_calc(struct ae_ctrl_cxt *cxt, struct match_data_param *dualcam_aesync)
{
	cmr_s32 ret = ISP_ERROR;

	if (!cxt || !dualcam_aesync) {
		ISP_LOGE("param is NULL error!");
		return ret;
	}
	// TODO: debug is_master=0
	ISP_LOGV("is_multi_mode=%d, is_master=%d", cxt->is_multi_mode, cxt->is_master);

	//get slave sensor aeinfo
	ret = cxt->ptr_isp_br_ioctrl(cxt->is_master ? CAM_SENSOR_MASTER : CAM_SENSOR_SLAVE0, GET_SLAVE_AECALC_RESULT, NULL, &dualcam_aesync->slave_ae_info.ae_calc_result);
	ret = cxt->ptr_isp_br_ioctrl(cxt->is_master ? CAM_SENSOR_MASTER : CAM_SENSOR_SLAVE0, GET_MASTER_AECALC_RESULT, NULL, &dualcam_aesync->master_ae_info.ae_calc_result);
	ret = cxt->ptr_isp_br_ioctrl(cxt->is_master ? CAM_SENSOR_MASTER : CAM_SENSOR_SLAVE0, GET_ALL_MODULE_AND_OTP, NULL, &dualcam_aesync->module_info);

	/* use ae_mapping to update slave's ae info, update slave_ae_info.ae_sync_result */
	ae_mapping(cxt, dualcam_aesync);

	/* store calculated values into ae_calc_result for reference */
	dualcam_aesync->slave_ae_info.ae_calc_result.wts.cur_again = dualcam_aesync->slave_ae_info.ae_sync_result.slave_ae.wts.cur_again;
	dualcam_aesync->slave_ae_info.ae_calc_result.wts.cur_exp_line = dualcam_aesync->slave_ae_info.ae_sync_result.slave_ae.wts.cur_exp_line;
	dualcam_aesync->slave_ae_info.ae_calc_result.wts.cur_dummy = dualcam_aesync->slave_ae_info.ae_sync_result.slave_ae.wts.cur_dummy;

	//save ae sync setting to slave sensor
	if (cxt->is_master) {
		ret = cxt->ptr_isp_br_ioctrl(cxt->is_master ? CAM_SENSOR_MASTER : CAM_SENSOR_SLAVE0, SET_SLAVE_AESYNC_SETTING, &dualcam_aesync->slave_ae_info.ae_sync_result, NULL);
	}

	return ret;
}
#endif

static cmr_s32 ae_get_flicker_switch_flag(struct ae_ctrl_cxt *cxt, cmr_handle in_param)
{
	cmr_s32 rtn = AE_SUCCESS;
	cmr_u32 cur_exp = 0;
	cmr_u32 *flag = (cmr_u32 *) in_param;

	if (in_param) {
		cur_exp = cxt->sync_cur_result.wts.exposure_time;
		// 50Hz/60Hz
		if (AE_FLICKER_50HZ == cxt->cur_status.settings.flicker) {
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
			element.index = led_ctl[0] - 1;
			cxt->isp_ops.flash_set_charge(cxt->isp_ops.isp_handler, &cfg, &element);
//set led_2
			cfg.led_idx = 2;
			//cfg.type = ISP_FLASH_TYPE_MAIN;
			cfg.type = type;
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
				goto EXIT;
			} else {
				fprintf(p, "shutter: %d  gain: %d\r\n", cxt->sync_cur_result.wts.cur_exp_line, cxt->sync_cur_result.wts.cur_again);

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
  EXIT:
	return;
}

static void ae_hdr_calculation(struct ae_ctrl_cxt *cxt, cmr_u32 in_max_frame_line, cmr_u32 in_min_frame_line, cmr_u32 in_exposure, cmr_u32 in_gain, cmr_u32 * out_gain, cmr_u32 * out_exp_l)
{
	cmr_u32 exp_line = 0;
	cmr_u32 exp_time = 0;
	cmr_u32 gain = in_gain;
	cmr_u32 exposure = in_exposure;
	cmr_u32 max_frame_line = in_max_frame_line;
	cmr_u32 min_frame_line = in_min_frame_line;

	if (cxt->cur_flicker == 0) {
		if (exposure > (max_frame_line * cxt->cur_status.line_time))
			exposure = max_frame_line * cxt->cur_status.line_time;
		else if (exposure < (min_frame_line * cxt->cur_status.line_time))
			exposure = min_frame_line * cxt->cur_status.line_time;

		if (exposure >= 10000000) {
			exp_time = (cmr_u32) (1.0 * exposure / 10000000 + 0.5) * 10000000;
			exp_line = (cmr_u32) (1.0 * exp_time / cxt->cur_status.line_time + 0.5);
			gain = (cmr_u32) (1.0 * exposure * gain / exp_time + 0.5);
		} else
			exp_line = (cmr_u32) (1.0 * exposure / cxt->cur_status.line_time + 0.5);
	}

	if (cxt->cur_flicker == 1) {
		if (exposure > (max_frame_line * cxt->cur_status.line_time))
			exposure = max_frame_line * cxt->cur_status.line_time;
		else if (exposure < (min_frame_line * cxt->cur_status.line_time))
			exposure = min_frame_line * cxt->cur_status.line_time;

		if (exposure >= 8333333) {
			exp_time = (cmr_u32) (1.0 * exposure / 8333333 + 0.5) * 8333333;
			exp_line = (cmr_u32) (1.0 * exp_time / cxt->cur_status.line_time + 0.5);
			gain = (cmr_u32) (1.0 * exposure * gain / exp_time + 0.5);
		} else
			exp_line = (cmr_u32) (1.0 * exposure / cxt->cur_status.line_time + 0.5);
	}

	*out_exp_l = exp_line;
	*out_gain = gain;

	ISP_LOGV("exposure %d, max_frame_line %d, min_frame_line %d, exp_time %d, exp_line %d\n", in_exposure, in_max_frame_line, in_min_frame_line, exp_time, exp_line);
}

static void ae_set_hdr_ctrl(struct ae_ctrl_cxt *cxt, struct ae_calc_in *param)
{
	UNUSED(param);
	cmr_u32 base_exposure_line = 0;
	cmr_u32 up_exposure = 0;
	cmr_u32 down_exposure = 0;
	cmr_u16 base_gain = 0;
	cmr_u16 base_idx = 0;
	cmr_u32 up_EV_offset = 0;
	cmr_u32 down_EV_offset = 0;
	cmr_u32 max_frame_line = 0;
	cmr_u32 min_frame_line = 0;
	cmr_u32 min_index = cxt->cur_status.ae_table->min_index;
	cmr_u32 exp_line = 0;
	cmr_u32 gain = 0;
	cmr_s32 counts = 0;
	cmr_u32 skip_num = 0;
	cmr_s8 hdr_callback_flag = 0;
#ifdef CONFIG_SUPPROT_AUTO_HDR
	float ev_result[2] = {0, 0};
	ev_result[0] = -cxt->hdr_calc_result.ev[0];
	ev_result[1] = cxt->hdr_calc_result.ev[1];
	ISP_LOGI("auto_hdr (cap) ev[0] %f ev[1] %f", ev_result[0], ev_result[1]);
#endif

	cxt->hdr_frame_cnt++;
	hdr_callback_flag = MAX((cmr_s8) cxt->hdr_cb_cnt, (cmr_s8) cxt->capture_skip_num);
	if (cxt->hdr_frame_cnt == hdr_callback_flag) {
#ifdef CONFIG_SUPPROT_AUTO_HDR
		(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, AE_CB_HDR_START, (void *)ev_result);
#else
		(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, AE_CB_HDR_START, NULL);
#endif
		ISP_LOGI("_isp_hdr_callback do-capture!\r\n");
	}

	skip_num = cxt->capture_skip_num;
	ISP_LOGV("skip_num %d", skip_num);
	counts = cxt->capture_skip_num - cxt->hdr_cb_cnt - cxt->hdr_frame_cnt;
	if (0 > counts) {
		up_EV_offset = cxt->cur_param->hdr_param.ev_plus_offset;
		if (up_EV_offset == 0)
			up_EV_offset = 150;
		down_EV_offset = cxt->cur_param->hdr_param.ev_minus_offset;
		if (down_EV_offset == 0)
			down_EV_offset = 150;
		max_frame_line = (cmr_u32) (1.0 * 1000000000 / cxt->fps_range.min / cxt->cur_status.line_time);
		min_frame_line = cxt->cur_status.ae_table->exposure[min_index];
		ISP_LOGV("counts %d\n", counts);
		ISP_LOGV("up_EV_offset %d\n", up_EV_offset);
		ISP_LOGV("down_EV_offset %d\n", down_EV_offset);
		ISP_LOGV("max_frame_line %d, min_frame_line %d, fps_min %d, cur_line_time %d\n", max_frame_line, min_frame_line, cxt->fps_range.min, cxt->cur_status.line_time);
		if (3 == cxt->hdr_flag) {
			base_idx = cxt->hdr_base_ae_idx;
			base_exposure_line = cxt->cur_status.ae_table->exposure[base_idx];
			base_gain = cxt->cur_status.ae_table->again[base_idx];
#ifdef CONFIG_SUPPROT_AUTO_HDR
		down_exposure = 1.0 / pow(2, cxt->hdr_calc_result.ev[0]) * base_exposure_line * cxt->cur_status.line_time;
		ISP_LOGI("down_exp %d, pow2 %f\n", down_exposure, pow(2, cxt->hdr_calc_result.ev[0]));
#else
		down_exposure = 1.0 / pow(2, down_EV_offset / 100.0) * base_exposure_line * cxt->cur_status.line_time;
		ISP_LOGI("down_exp %d, down_ev_offset %f, pow2 %f\n", down_exposure, down_EV_offset / 100.0, pow(2, down_EV_offset / 100.0));
#endif
		ae_hdr_calculation(cxt, max_frame_line, min_frame_line, down_exposure, base_gain, &gain, &exp_line);
		ISP_LOGV("base_idx: %d, base_exposure: %d, base_gain: %d, down_exposure: %d, exp_line: %d", base_idx, base_exposure_line, base_gain, down_exposure, exp_line);
		cxt->cur_status.settings.manual_mode = 0;
		cxt->cur_status.settings.exp_line = exp_line;
		cxt->cur_status.settings.gain = gain;
		cxt->hdr_flag--;
		ISP_LOGI("_isp_hdr_3: exp_line %d, gain %d\n", cxt->cur_status.settings.exp_line, cxt->cur_status.settings.gain);
		} else if (2 == cxt->hdr_flag) {
		base_idx = cxt->hdr_base_ae_idx;
		base_exposure_line = cxt->cur_status.ae_table->exposure[base_idx];
		base_gain = cxt->cur_status.ae_table->again[base_idx];
#ifdef CONFIG_SUPPROT_AUTO_HDR
		up_exposure = pow(2, cxt->hdr_calc_result.ev[1]) * base_exposure_line * cxt->cur_status.line_time;
		ISP_LOGI("up_exp %d, pow2 %f\n", up_exposure, pow(2, cxt->hdr_calc_result.ev[1]));
#else
		up_exposure = pow(2, up_EV_offset / 100.0) * base_exposure_line * cxt->cur_status.line_time;
		ISP_LOGI("up_exp %d, up_ev_offset %f, pow2 %f\n", up_exposure, up_EV_offset / 100.0, pow(2, up_EV_offset / 100.0));
#endif
		ae_hdr_calculation(cxt, max_frame_line, min_frame_line, up_exposure, base_gain, &gain, &exp_line);
		ISP_LOGV("base_idx: %d, base_exposure: %d, base_gain: %d, up_exposure: %d, exp_line: %d", base_idx, base_exposure_line, base_gain, up_exposure, exp_line);
		cxt->cur_status.settings.manual_mode = 0;
		cxt->cur_status.settings.exp_line = exp_line;
		cxt->cur_status.settings.gain = gain;
		cxt->hdr_flag--;
		ISP_LOGI("_isp_hdr_2: exp_line %d, gain %d\n", cxt->cur_status.settings.exp_line, cxt->cur_status.settings.gain);
		} else if (1 == cxt->hdr_flag) {
		base_idx = cxt->hdr_base_ae_idx;
		base_exposure_line = cxt->cur_status.ae_table->exposure[base_idx];
		base_gain = cxt->cur_status.ae_table->again[base_idx];
		cxt->cur_status.settings.manual_mode = 0;
		cxt->cur_status.settings.exp_line = base_exposure_line;
		cxt->cur_status.settings.gain = base_gain;
		cxt->hdr_flag--;
		ISP_LOGI("_isp_hdr_1: exp_line %d, gain %d\n", cxt->cur_status.settings.exp_line, cxt->cur_status.settings.gain);
		} else {
			;
		}
	}
}

static struct ae_exposure_param s_bakup_exp_param[4] = { {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0} };
static struct ae_exposure_param_switch s_ae_manual[4] = {{0,0,0,0,0,0},{0,0,0,0,0,0},{0,0,0,0,0,0},{0,0,0,0,0,0}};

static void ae_save_exp_gain_param(struct ae_exposure_param *param, cmr_u32 num, struct ae_exposure_param_switch * ae_manual_param)
{
	cmr_u32 i = 0;
	FILE *pf = NULL;
	char version[1024];
	property_get("ro.build.version.release", version, "");
	if (version[0] > '6') {
		pf = fopen(AE_EXP_GAIN_PARAM_FILE_NAME_CAMERASERVER, "wb");
		if (pf) {
			for (i = 0; i < num; ++i) {
				ISP_LOGV("write:[%d]: %d, %d, %d, %d, %d\n", i, param[i].exp_line, param[i].exp_time, param[i].dummy, param[i].gain, param[i].bv);
			}

			fwrite((char *)param, 1, num * sizeof(struct ae_exposure_param), pf);
			fwrite((char *)ae_manual_param, 1, num * sizeof(struct ae_exposure_param_switch), pf);
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
			fwrite((char *)ae_manual_param, 1, num * sizeof(struct ae_exposure_param_switch), pf);
			fclose(pf);
			pf = NULL;
		}
	}

}

static void ae_read_exp_gain_param(struct ae_exposure_param *param, cmr_u32 num, struct ae_exposure_param_switch * ae_manual_param)
{
	cmr_u32 i = 0;
	FILE *pf = NULL;
	char version[1024];
	property_get("ro.build.version.release", version, "");
	if (version[0] > '6') {
		pf = fopen(AE_EXP_GAIN_PARAM_FILE_NAME_CAMERASERVER, "rb");
		if (pf) {
			memset((void *)param, 0, sizeof(struct ae_exposure_param) * num);
			fread((char *)param, 1, num * sizeof(struct ae_exposure_param), pf);
			fread((char *)ae_manual_param, 1, num * sizeof(struct ae_exposure_param_switch), pf);
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
			fread((char *)param, 1, num * sizeof(struct ae_exposure_param), pf);
			fread((char *)ae_manual_param, 1, num * sizeof(struct ae_exposure_param_switch), pf);
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
		if (0 == cxt->sync_cur_result.wts.cur_exp_line && 0 == cxt->sync_cur_result.wts.cur_again) {
			cxt->last_exp_param.exp_line = cxt->cur_status.ae_table->exposure[cxt->cur_status.start_index];
			cxt->last_exp_param.exp_time = cxt->cur_status.ae_table->exposure[cxt->cur_status.start_index] * cxt->cur_status.line_time;
			cxt->last_exp_param.dummy = 0;
			cxt->last_exp_param.gain = cxt->cur_status.ae_table->again[cxt->cur_status.start_index];
			cxt->last_exp_param.line_time = cxt->cur_status.line_time;
			cxt->last_exp_param.cur_index = cxt->cur_status.start_index;
			cxt->last_index = cxt->cur_status.start_index;
			if (0 != cxt->cur_result.cur_bv)
				cxt->last_exp_param.bv = cxt->cur_result.cur_bv;
			else
				cxt->last_exp_param.bv = 1;
		} else {
			cxt->last_exp_param.exp_line = cxt->sync_cur_result.wts.cur_exp_line;
			cxt->last_exp_param.exp_time = cxt->sync_cur_result.wts.cur_exp_line * cxt->cur_status.line_time;
			cxt->last_exp_param.dummy = cxt->sync_cur_result.wts.cur_dummy;
			cxt->last_exp_param.gain = cxt->sync_cur_result.wts.cur_again;
			cxt->last_exp_param.line_time = cxt->cur_status.line_time;
			cxt->last_exp_param.cur_index = cxt->sync_cur_result.wts.cur_index;
			cxt->last_index = cxt->sync_cur_result.wts.cur_index;
			if (0 != cxt->cur_result.cur_bv)
				cxt->last_exp_param.bv = cxt->cur_result.cur_bv;
			else
				cxt->last_exp_param.bv = 1;
		}

		cxt->last_enable = 1;
		if(cxt->cur_status.settings.manual_mode==1){
			cxt->last_exp_param.target_offset = 0; // manual mode without target_offset
		}else{
			cxt->last_exp_param.target_offset = cxt->sync_cur_result.target_lum - cxt->sync_cur_result.target_lum_ori;
		}

		s_bakup_exp_param[cxt->camera_id] = cxt->last_exp_param;

		if((cxt->app_mode < 32)&&(cxt->app_mode >= 0)){
			cxt->mode_switch[cxt->app_mode].exp_line = cxt->last_exp_param.exp_line;
			cxt->mode_switch[cxt->app_mode].dummy = cxt->last_exp_param.dummy;
			cxt->mode_switch[cxt->app_mode].gain = cxt->last_exp_param.gain;
			cxt->mode_switch[cxt->app_mode].exp_time = cxt->last_exp_param.exp_time;
			cxt->mode_switch[cxt->app_mode].target_offset = cxt->last_exp_param.target_offset;
			cxt->mode_switch[cxt->app_mode].table_idx = cxt->last_exp_param.cur_index;
		}
		if(CAMERA_MODE_MANUAL == cxt->app_mode)
			s_ae_manual[cxt->camera_id] = cxt->mode_switch[cxt->app_mode];

		ae_save_exp_gain_param(&s_bakup_exp_param[0], sizeof(s_bakup_exp_param) / sizeof(struct ae_exposure_param), &s_ae_manual[0]);
		ISP_LOGI("AE_VIDEO_STOP(in preview) cam-id %d BV %d BV_backup %d E %d G %d lt %d W %d H %d,enable: %d", cxt->camera_id, cxt->last_exp_param.bv, s_bakup_exp_param[cxt->camera_id].bv, cxt->last_exp_param.exp_line, cxt->last_exp_param.gain, cxt->last_exp_param.line_time, cxt->snr_info.frame_size.w, cxt->snr_info.frame_size.h, cxt->last_enable);
	} else {
		if ((1 == cxt->is_snapshot) && ((FLASH_NONE == cxt->cur_status.settings.flash) || FLASH_MAIN_BEFORE <= cxt->cur_status.settings.flash)) {
			ae_set_restore_cnt(cxt);
		}
		ISP_LOGI("AE_VIDEO_STOP(in capture) cam-id %d BV %d BV_backup %d E %d G %d lt %d W %d H %d, enable: %d", cxt->camera_id, cxt->last_exp_param.bv, s_bakup_exp_param[cxt->camera_id].bv, cxt->last_exp_param.exp_line, cxt->last_exp_param.gain, cxt->last_exp_param.line_time, cxt->snr_info.frame_size.w, cxt->snr_info.frame_size.h, cxt->last_enable);
	}
}

static cmr_s32 ae_set_video_start(struct ae_ctrl_cxt *cxt, cmr_handle * param)
{
	cmr_s32 rtn = AE_SUCCESS;
	cmr_s32 ae_skip_num = 0;
	cmr_s32 mode = 0;
	struct ae_trim trim;
	cmr_u32 max_exp = 0;
	struct ae_exposure_param src_exp = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	struct ae_exposure_param dst_exp;
	struct ae_range fps_range;
	struct ae_set_work_param *work_info = (struct ae_set_work_param *)param;
	cmr_u32 k;
	cmr_u32 j;
	if (NULL == param) {
		ISP_LOGE("param is NULL \n");
		return AE_ERROR;
	}

	cxt->capture_skip_num = work_info->capture_skip_num;
	ISP_LOGV("capture_skip_num %d", cxt->capture_skip_num);

	cxt->zsl_flag = work_info->zsl_flag;
	ISP_LOGV("zsl_flag %d", cxt->zsl_flag);
	cxt->expchanged = 0;

	if((work_info->blk_num.w != work_info->blk_num.h) || (work_info->blk_num.w < 32) || (work_info->blk_num.w % 32)){
		work_info->blk_num.w = 32;
		work_info->blk_num.h = 32;
	}

	if (work_info->mode >= AE_WORK_MODE_MAX) {
		ISP_LOGE("fail to set work mode");
		work_info->mode = AE_WORK_MODE_COMMON;
	}

	if (1 == work_info->dv_mode)
		cxt->cur_status.settings.work_mode = AE_WORK_MODE_VIDEO;
	else
		cxt->cur_status.settings.work_mode = AE_WORK_MODE_COMMON;

	memset(&cxt->ctTab[0], 0, sizeof(cxt->ctTab));
	memset(&cxt->ctTabRg[0], 0, sizeof(cxt->ctTabRg));
	if (!ae_is_equal(work_info->ct_table.ct[0], 0.0)
		&& !ae_is_equal(work_info->ct_table.rg[0], 0.0)) {
		for (k = 0; k < 20; k++) {
			cxt->ctTab[k] = work_info->ct_table.ct[k];
			cxt->ctTabRg[k] = work_info->ct_table.rg[k];
		}
	}
	cxt->awb_otp_info = work_info->awb_otp_info;

	if (0 == work_info->is_snapshot) {
		cxt->cur_status.frame_id = 0;
		cxt->slw_prev_skip_num = 0;
		memset(&cxt->cur_result.wts, 0, sizeof(struct ae1_senseor_out));
		memset(&cxt->sync_cur_result.wts, 0, sizeof(struct ae1_senseor_out));
		cxt->send_once[0] = cxt->send_once[1] = cxt->send_once[2] = cxt->send_once[3] = 0;
	}

	if (0 != cxt->snr_info.binning_factor)
		cxt->binning_factor_before = cxt->snr_info.binning_factor;
	else
		cxt->binning_factor_before = 1;
	cxt->snr_info = work_info->resolution_info;
	cxt->monitor_cfg.blk_num.w = work_info->blk_num.w;
	cxt->monitor_cfg.blk_num.h = work_info->blk_num.h;
	if (0 != work_info->resolution_info.binning_factor)
		cxt->binning_factor_after = work_info->resolution_info.binning_factor;
	else
		cxt->binning_factor_after = 1;
	cxt->cur_status.frame_size = work_info->resolution_info.frame_size;
	cxt->cur_status.line_time = work_info->resolution_info.line_time;
	cxt->cur_status.snr_max_fps = work_info->sensor_fps.max_fps;
	cxt->cur_status.snr_min_fps = work_info->sensor_fps.min_fps;
	if (cxt->is_multi_mode == ISP_ALG_DUAL_SBS) {
#ifndef CONFIG_ISP_2_2
		/* save master & slave sensor info */
		struct sensor_info sensor_info;
		sensor_info.max_again = cxt->sensor_max_gain;
		sensor_info.min_again = cxt->sensor_min_gain;
		sensor_info.sensor_gain_precision = cxt->sensor_gain_precision;
		sensor_info.min_exp_line = cxt->min_exp_line;
		sensor_info.line_time = cxt->cur_status.line_time;
		rtn = cxt->ptr_isp_br_ioctrl(cxt->is_master ? CAM_SENSOR_MASTER : CAM_SENSOR_SLAVE0, SET_MODULE_INFO, &sensor_info, NULL);
		ISP_LOGV("sensor info: role=%d, max_gain=%d, min_gain=%d, precision=%d, min_exp_line=%d, line_time=%d",
				 cxt->is_master, sensor_info.max_again, sensor_info.min_again, sensor_info.sensor_gain_precision, sensor_info.min_exp_line, sensor_info.line_time);
#endif
	}

	if ((cxt->is_multi_mode == ISP_ALG_DUAL_C_C||cxt->is_multi_mode ==ISP_ALG_DUAL_W_T||cxt->is_multi_mode ==ISP_ALG_DUAL_C_M)) {
#ifndef CONFIG_ISP_2_2
		/* save master & slave sensor info */
		struct sensor_info sensor_info;
		sensor_info.max_again = cxt->sensor_max_gain;
		sensor_info.min_again = cxt->sensor_min_gain;
		sensor_info.sensor_gain_precision = cxt->sensor_gain_precision;
		sensor_info.min_exp_line = cxt->min_exp_line;
		sensor_info.line_time = cxt->cur_status.line_time;
		rtn = cxt->ptr_isp_br_ioctrl(cxt->is_master ? CAM_SENSOR_MASTER : CAM_SENSOR_SLAVE0, SET_MODULE_INFO, &sensor_info, NULL);
		ISP_LOGV("(sharkl3)normal mode:sensor info: role=%d, max_gain=%d, min_gain=%d, precision=%d, min_exp_line=%d, line_time=%d",
				 cxt->is_master, sensor_info.max_again, sensor_info.min_again, sensor_info.sensor_gain_precision, sensor_info.min_exp_line, sensor_info.line_time);
#endif
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

	cxt->calc_results.monitor_info.trim = cxt->monitor_cfg.trim;
	cxt->calc_results.monitor_info.win_num = cxt->monitor_cfg.blk_num;
	cxt->calc_results.monitor_info.win_size = cxt->monitor_cfg.blk_size;

	mode = AE_WORK_MODE_COMMON;	//AE block only be in common
	cxt->cur_status.settings.iso_special_mode = 1;

	memcpy(&cxt->tuning_param[mode].ae_table[0][0], &cxt->tuning_param[mode].backup_ae_table[0][0], AE_FLICKER_NUM * AE_ISO_NUM * sizeof(struct ae_exp_gain_table));
	{
		struct ae_exp_gain_table *src[AE_FLICKER_NUM] = { NULL, NULL };
		struct ae_exp_gain_table *dst[AE_FLICKER_NUM] = { NULL, NULL };
		cmr_u32 i;
		for (i = 0; i < AE_ISO_NUM_NEW; ++i) {
			if (0 != cxt->tuning_param[mode].ae_table[AE_FLICKER_50HZ][i].max_index) {
				src[AE_FLICKER_50HZ] = &cxt->tuning_param[mode].backup_ae_table[AE_FLICKER_50HZ][i];
				src[AE_FLICKER_60HZ] = &cxt->tuning_param[mode].backup_ae_table[AE_FLICKER_60HZ][i];
				dst[AE_FLICKER_50HZ] = &cxt->tuning_param[mode].ae_table[AE_FLICKER_50HZ][i];
				dst[AE_FLICKER_60HZ] = &cxt->tuning_param[mode].ae_table[AE_FLICKER_60HZ][i];

				ae_cvt_exposure_time2line(src, dst, cxt->cur_status.line_time, cxt->tuning_param[mode].ae_tbl_exp_mode);
			}
			dst[AE_FLICKER_50HZ]->min_index = 0;
			dst[AE_FLICKER_60HZ]->min_index = 0;
		}
	}

	for (j = 0; j < AE_SCENE_NUM; ++j) {
		struct ae_exp_gain_table *src[AE_FLICKER_NUM];
		struct ae_exp_gain_table *dst[AE_FLICKER_NUM];
		src[AE_FLICKER_50HZ] = &cxt->back_scene_mode_ae_table[j][AE_FLICKER_50HZ];
		src[AE_FLICKER_60HZ] = &cxt->back_scene_mode_ae_table[j][AE_FLICKER_60HZ];
		dst[AE_FLICKER_50HZ] = &cxt->tuning_param[mode].scene_info[j].ae_table[AE_FLICKER_50HZ];
		dst[AE_FLICKER_60HZ] = &cxt->tuning_param[mode].scene_info[j].ae_table[AE_FLICKER_60HZ];

		if (1 == cxt->tuning_param[mode].scene_info[j].table_enable) {
			ae_cvt_exposure_time2line(src, dst, cxt->cur_status.line_time, cxt->tuning_param[mode].scene_info[j].exp_tbl_mode);
			dst[AE_FLICKER_50HZ]->min_index = 0;
			dst[AE_FLICKER_60HZ]->min_index = 0;
		}
	}

	cxt->cur_status.ae_table = &cxt->cur_param->ae_table[cxt->cur_status.settings.flicker][AE_ISO_AUTO];
	if (AE_SCENE_NORMAL != cxt->sync_cur_status.settings.scene_mode) {
		cmr_u32 i = 0;
		struct ae_scene_info *scene_info = &cxt->cur_param->scene_info[0];
		for (i = 0; i < AE_SCENE_NUM; ++i) {
			ISP_LOGV("%d: mod: %d, eb: %d\n", i, scene_info[i].scene_mode, scene_info[i].enable);
			if ((1 == scene_info[i].enable) && ((cmr_u32) cxt->cur_status.settings.scene_mode == scene_info[i].scene_mode)) {
				break;
			}
		}
		if (AE_SCENE_NUM > i) {
			if (scene_info[i].table_enable) {
				cxt->cur_status.ae_table = &scene_info[i].ae_table[cxt->cur_status.settings.flicker];
			}
			if (0 != scene_info[i].target_lum) {
				cxt->cur_status.target_lum = scene_info[i].target_lum;
			}
		}
	}
	cxt->cur_status.ae_table->min_index = 0;

	if (1 == cxt->last_enable) {
		if (cxt->cur_status.line_time == cxt->last_exp_param.line_time) {
			src_exp.exp_line = cxt->last_exp_param.exp_line;
			src_exp.gain = cxt->last_exp_param.gain;
			src_exp.exp_time = cxt->last_exp_param.exp_time;
			src_exp.dummy = cxt->last_exp_param.dummy;
		} else {
			src_exp.exp_line = (cmr_u32) (1.0 * cxt->last_exp_param.exp_line * cxt->last_exp_param.line_time / cxt->cur_status.line_time + 0.5);
			if (cxt->min_exp_line > src_exp.exp_line)
				src_exp.exp_line = cxt->min_exp_line;
			src_exp.exp_time = src_exp.exp_line * cxt->cur_status.line_time;
			src_exp.gain = cxt->last_exp_param.gain;
			src_exp.dummy = cxt->last_exp_param.dummy;
		}
		src_exp.cur_index = cxt->last_index;
		if((cxt->app_mode < 32)&&(cxt->app_mode >= 0)){
			if(0 != cxt->mode_switch[cxt->app_mode].gain){
				src_exp.target_offset = cxt->mode_switch[cxt->app_mode].target_offset;
				src_exp.exp_line = cxt->mode_switch[cxt->app_mode].exp_line;
				src_exp.gain = cxt->mode_switch[cxt->app_mode].gain;
				src_exp.exp_time = cxt->mode_switch[cxt->app_mode].exp_time;
				src_exp.dummy = cxt->mode_switch[cxt->app_mode].dummy;
				src_exp.cur_index = cxt->mode_switch[cxt->app_mode].table_idx;
			}else{
				if((CAMERA_MODE_MANUAL == cxt->app_mode) && (0 != s_ae_manual[cxt->camera_id].gain)){
					src_exp.target_offset = s_ae_manual[cxt->camera_id].target_offset;
					src_exp.exp_line = s_ae_manual[cxt->camera_id].exp_line;
					src_exp.gain = s_ae_manual[cxt->camera_id].gain;
					src_exp.exp_time = s_ae_manual[cxt->camera_id].exp_time;
					src_exp.dummy = s_ae_manual[cxt->camera_id].dummy;
					src_exp.cur_index = s_ae_manual[cxt->app_mode].table_idx;
				}
				else if(0 != cxt->mode_switch[0].gain){
					src_exp.target_offset = cxt->mode_switch[0].target_offset;
					src_exp.exp_line = cxt->mode_switch[0].exp_line;
					src_exp.gain = cxt->mode_switch[0].gain;
					src_exp.exp_time = cxt->mode_switch[0].exp_time;
					src_exp.dummy = cxt->mode_switch[0].dummy;
					src_exp.cur_index = cxt->mode_switch[0].table_idx;
				}
			}
			if (work_info->is_snapshot && (cxt->cur_status.line_time != cxt->last_exp_param.line_time)){
				src_exp.exp_line = (cmr_u32) (1.0 * cxt->last_exp_param.exp_line * cxt->last_exp_param.line_time / cxt->cur_status.line_time + 0.5);
				if (cxt->min_exp_line > src_exp.exp_line)
					src_exp.exp_line = cxt->min_exp_line;
				src_exp.exp_time = src_exp.exp_line * cxt->cur_status.line_time;
			}
		}
	} else {
		ae_read_exp_gain_param(&s_bakup_exp_param[0], sizeof(s_bakup_exp_param) / sizeof(struct ae_exposure_param),&s_ae_manual[0]);
		if ((0 != s_bakup_exp_param[cxt->camera_id].exp_line)
			&& (0 != s_bakup_exp_param[cxt->camera_id].exp_time)
			&& (0 != s_bakup_exp_param[cxt->camera_id].gain)
			&& (0 != s_bakup_exp_param[cxt->camera_id].bv)) {
			src_exp.exp_line = s_bakup_exp_param[cxt->camera_id].exp_time / cxt->cur_status.line_time;
			src_exp.exp_time = s_bakup_exp_param[cxt->camera_id].exp_time;
			src_exp.gain = s_bakup_exp_param[cxt->camera_id].gain;
			src_exp.cur_index = s_bakup_exp_param[cxt->camera_id].cur_index;
			src_exp.target_offset = s_bakup_exp_param[cxt->camera_id].target_offset;
			cxt->sync_cur_result.cur_bv = cxt->cur_result.cur_bv = s_bakup_exp_param[cxt->camera_id].bv;
		} else {
			src_exp.exp_line = cxt->cur_status.ae_table->exposure[cxt->cur_status.start_index];
			src_exp.exp_time = cxt->cur_status.ae_table->exposure[cxt->cur_status.start_index] * cxt->snr_info.line_time;
			src_exp.gain = cxt->cur_status.ae_table->again[cxt->cur_status.start_index];
			src_exp.cur_index = cxt->cur_status.start_index;
			cxt->cur_result.wts.stable = 0;
			src_exp.target_offset = 0;
			cxt->sync_cur_result.cur_bv = cxt->cur_result.cur_bv = 500;
		}
	}

	ISP_LOGV("Camera_id %d, sync_BV %d, cur_BV %d, Backup_bv %d, Backup_exp_line %d, Backup_exp_time %d, Backup_dummy %d, Backup_gain %d",
			 cxt->camera_id,
			 cxt->sync_cur_result.cur_bv,
			 cxt->cur_result.cur_bv,
			 s_bakup_exp_param[cxt->camera_id].bv,
			 s_bakup_exp_param[cxt->camera_id].exp_line, s_bakup_exp_param[cxt->camera_id].exp_time, s_bakup_exp_param[cxt->camera_id].dummy, s_bakup_exp_param[cxt->camera_id].gain);

	if ((1 == cxt->last_enable) && (1 == work_info->is_snapshot)) {
		dst_exp.exp_time = src_exp.exp_time;
		dst_exp.exp_line = src_exp.exp_line;
		dst_exp.gain = src_exp.gain;
		dst_exp.dummy = src_exp.dummy;
		dst_exp.cur_index = src_exp.cur_index;
	} else {
		src_exp.dummy = 0;
		max_exp = cxt->cur_status.ae_table->exposure[cxt->cur_status.ae_table->max_index];
		if (work_info->sensor_fps.is_high_fps) {
			fps_range.min = work_info->sensor_fps.max_fps;
			fps_range.max = work_info->sensor_fps.max_fps;
			cxt->cur_status.snr_max_fps = work_info->sensor_fps.max_fps;
		} else {
			fps_range.min = cxt->fps_range.min;
			fps_range.max = cxt->fps_range.max;
		}
		ae_adjust_exp_gain(cxt, &src_exp, &fps_range, max_exp, &dst_exp);

	}
	cxt->cur_status.target_offset = src_exp.target_offset;
	cxt->cur_result.wts.exposure_time = dst_exp.exp_time;
	cxt->cur_result.wts.cur_exp_line = dst_exp.exp_line;
	cxt->cur_result.wts.cur_again = dst_exp.gain;
	cxt->cur_result.wts.cur_dgain = 0;
	cxt->cur_result.wts.cur_dummy = dst_exp.dummy;
	cxt->cur_result.wts.cur_index = dst_exp.cur_index;
	cxt->cur_result.wts.stable = 0;
	cxt->sync_cur_result.wts.exposure_time = cxt->cur_result.wts.exposure_time;
	cxt->sync_cur_result.wts.cur_exp_line = cxt->cur_result.wts.cur_exp_line;
	cxt->sync_cur_result.wts.cur_again = cxt->cur_result.wts.cur_again;
	cxt->sync_cur_result.wts.cur_dgain = cxt->cur_result.wts.cur_dgain;
	cxt->sync_cur_result.wts.cur_dummy = cxt->cur_result.wts.cur_dummy;
	cxt->sync_cur_result.wts.cur_index = cxt->cur_result.wts.cur_index;
	cxt->sync_cur_result.wts.stable = cxt->cur_result.wts.stable;

#ifdef CONFIG_ISP_2_2
	struct match_data_param dualcam_aesync;
	struct ae_alg_calc_result *current_result = NULL;
	struct sensor_info sensor_info;
	current_result = &cxt->sync_cur_result;
	sensor_info.max_again = cxt->sensor_max_gain;
	sensor_info.min_again = cxt->sensor_min_gain;
	sensor_info.sensor_gain_precision = cxt->sensor_gain_precision;
	sensor_info.min_exp_line = cxt->min_exp_line;
	sensor_info.line_time = cxt->cur_status.line_time;

	if (cxt->is_multi_mode && cxt->is_master) {
		rtn = cxt->ptr_isp_br_ioctrl(cxt->is_master ? CAM_SENSOR_MASTER : CAM_SENSOR_SLAVE0, SET_MASTER_MODULE_INFO , &sensor_info, NULL);
		memcpy(&dualcam_aesync.master_ae_info.ae_calc_result, current_result, sizeof(dualcam_aesync.master_ae_info.ae_calc_result));
		ISP_LOGV("[master] cur_fps=%f, cur_lum=%d, gain=%u, expline=%u(%d), exptime=%u, dummy=%u",
				 dualcam_aesync.master_ae_info.ae_calc_result.wts.cur_fps,
				 dualcam_aesync.master_ae_info.ae_calc_result.cur_lum,
				 dualcam_aesync.master_ae_info.ae_calc_result.wts.cur_again,
				 dualcam_aesync.master_ae_info.ae_calc_result.wts.cur_exp_line,
				 sensor_info.line_time,
				 dualcam_aesync.master_ae_info.ae_calc_result.wts.exposure_time, dualcam_aesync.master_ae_info.ae_calc_result.wts.cur_dummy);
		/* store calculated master's ae value */
		rtn = cxt->ptr_isp_br_ioctrl(cxt->is_master ? CAM_SENSOR_MASTER : CAM_SENSOR_SLAVE0, SET_MASTER_AECALC_RESULT, current_result, NULL);
	} else if (cxt->is_multi_mode && !cxt->is_master) {
		rtn = cxt->ptr_isp_br_ioctrl(cxt->is_master ? CAM_SENSOR_MASTER : CAM_SENSOR_SLAVE0, SET_SLAVE_MODULE_INFO, &sensor_info, NULL);
		/* use stored master's ae value to calculate slave's ae value */
		rtn = ae_dual_cam_sync_calc(cxt, &dualcam_aesync);

		current_result->wts.cur_exp_line = dualcam_aesync.slave_ae_info.ae_sync_result.slave_ae.wts.cur_exp_line;
		current_result->wts.exposure_time = dualcam_aesync.slave_ae_info.ae_sync_result.slave_ae.wts.exposure_time;
		current_result->wts.cur_again = dualcam_aesync.slave_ae_info.ae_sync_result.slave_ae.wts.cur_again;
		current_result->wts.cur_dummy = dualcam_aesync.slave_ae_info.ae_sync_result.slave_ae.wts.cur_dummy;
		cxt->sync_cur_result.wts.exposure_time = current_result->wts.exposure_time;
		cxt->sync_cur_result.wts.cur_exp_line = current_result->wts.cur_exp_line;
		cxt->sync_cur_result.wts.cur_again = current_result->wts.cur_again;
		cxt->sync_cur_result.wts.cur_dummy = current_result->wts.cur_dummy;
		ISP_LOGV("[slave ] cur_lum=%d, gain=%u, expline=%u(%d), exptime=%u, dummy=%u",
				 current_result->cur_lum,
				 dualcam_aesync.slave_ae_info.ae_sync_result.slave_ae.wts.cur_again,
				 dualcam_aesync.slave_ae_info.ae_sync_result.slave_ae.wts.cur_exp_line,
				 sensor_info.line_time,
				 dualcam_aesync.slave_ae_info.ae_sync_result.slave_ae.wts.exposure_time, dualcam_aesync.slave_ae_info.ae_sync_result.slave_ae.wts.cur_dummy);
		rtn = cxt->ptr_isp_br_ioctrl(cxt->is_master ? CAM_SENSOR_MASTER : CAM_SENSOR_SLAVE0, SET_SLAVE_AESYNC_SETTING, &dualcam_aesync.slave_ae_info.ae_sync_result, NULL);
	}
#endif

	ae_make_calc_result(cxt, &cxt->sync_cur_result, &cxt->calc_results);

	/*update parameters to sensor */
	memset((void *)&cxt->exp_data, 0, sizeof(cxt->exp_data));
	cxt->exp_data.lib_data.exp_line = cxt->sync_cur_result.wts.cur_exp_line;
	cxt->exp_data.lib_data.exp_time = cxt->sync_cur_result.wts.exposure_time;
	cxt->exp_data.lib_data.gain = cxt->sync_cur_result.wts.cur_again;
	cxt->exp_data.lib_data.dummy = cxt->sync_cur_result.wts.cur_dummy;
	cxt->exp_data.lib_data.line_time = cxt->cur_status.line_time;

	rtn = ae_update_result_to_sensor(cxt, &cxt->exp_data, 1);

	cxt->is_snapshot = work_info->is_snapshot;
	/*it is normal capture, not in flash mode */
	if ((1 == cxt->last_enable)
		&& ((FLASH_NONE == cxt->cur_status.settings.flash)
			|| (FLASH_LED_OFF == cxt->cur_status.settings.flash))) {
		if (0 == work_info->is_snapshot) {
			cxt->last_enable = 0;
			cxt->cur_status.settings.exp_line = cxt->sync_cur_result.wts.cur_exp_line;
			cxt->cur_status.settings.gain = cxt->sync_cur_result.wts.cur_again;
			cxt->cur_status.settings.is_snapshot =  work_info->is_snapshot;
			ae_set_restore_cnt(cxt);
		} else {
			ae_set_pause(cxt);
			cxt->cur_status.settings.manual_mode = 0;
			if(cxt->last_table_index){
				cxt->cur_status.settings.table_idx = cxt->last_table_index;
			}else{
				cxt->cur_status.settings.table_idx = 0;
			}
			cxt->cur_status.settings.exp_line = cxt->sync_cur_result.wts.cur_exp_line;
			cxt->cur_status.settings.gain = cxt->sync_cur_result.wts.cur_again;
			cxt->cur_status.settings.is_snapshot =  work_info->is_snapshot;
			ISP_LOGV("table_idx:%d",cxt->cur_status.settings.table_idx);
		}
	}

	cxt->is_first = 1;
	ISP_LOGI("AE_VIDEO_START cam-id %d BV %d lt %d W %d H %d , exp: %d, gain:%d flash: %d CAP %d, enable: %d",
			 cxt->camera_id, cxt->cur_result.cur_bv, cxt->cur_status.line_time, cxt->snr_info.frame_size.w,
			 cxt->snr_info.frame_size.h, cxt->sync_cur_result.wts.cur_exp_line, cxt->sync_cur_result.wts.cur_again, cxt->cur_status.settings.flash, work_info->is_snapshot, cxt->last_enable);

	return rtn;
}

static cmr_s32 ae_touch_ae_process(struct ae_ctrl_cxt *cxt, struct ae_alg_calc_result *result)
{
	cmr_s32 rtn = AE_SUCCESS;
	struct ae_alg_calc_result *current_result = (struct ae_alg_calc_result *)result;

	if(cxt->is_snapshot) {
		return rtn;
	}

	/*****touch status0:touch before/release;1:touch doing; 2: toch done and AE stable*****/
	ISP_LOGV("TCAECTL_status and ae_stable is %d,%d", current_result->tcAE_status, current_result->wts.stable);
	if (1 == current_result->tcAE_status && 1 == current_result->wts.stable) {
		ISP_LOGV("TCAE_start lock ae and pause cnt is %d\n", cxt->cur_status.settings.pause_cnt);
		rtn = ae_set_pause(cxt);
		current_result->tcAE_status = 2;
	}
	cxt->cur_status.to_ae_state = current_result->tcAE_status;
	ISP_LOGV("TCAECTL_unlock is (%d,%d)\n", current_result->tcAE_status, current_result->tcRls_flag);

	if (0 == current_result->tcAE_status && 1 == current_result->tcRls_flag) {
		rtn = ae_set_restore_cnt(cxt);
		ISP_LOGV("TCAE_start release lock ae");
		current_result->tcRls_flag = 0;
	}
	ISP_LOGV("TCAECTL_rls_ae_lock is %d", cxt->cur_status.settings.lock_ae);

	return rtn;
}

static cmr_s32 ae_set_dc_dv_mode(struct ae_ctrl_cxt *cxt, void *param)
{
	if (param) {
		if (1 == *(cmr_u32 *) param) {
			cxt->cur_status.settings.work_mode = AE_WORK_MODE_VIDEO;
		} else {
			cxt->cur_status.settings.work_mode = AE_WORK_MODE_COMMON;
		}
		ISP_LOGV("AE_SET_DC_DV %d", cxt->cur_status.settings.work_mode);
	}

	return AE_SUCCESS;
}

static cmr_s32 ae_set_scene(struct ae_ctrl_cxt *cxt, void *param)
{
	if (param) {
		struct ae_set_scene *scene_mode = param;

		if (scene_mode->mode < AE_SCENE_MOD_MAX) {
			cxt->cur_status.settings.scene_mode = (cmr_s8) scene_mode->mode;
			cxt->mod_update_list.is_scene = 1;
		}
		ISP_LOGV("AE_SET_SCENE %d\n", cxt->cur_status.settings.scene_mode);
	}

	return AE_SUCCESS;
}

static cmr_s32 ae_set_iso(struct ae_ctrl_cxt *cxt, void *param)
{
	if (param) {
		struct ae_set_iso *iso = param;

		if (iso->mode < AE_ISO_MAX) {
			cxt->cur_status.settings.iso = iso->mode;
			cxt->mod_update_list.is_miso = 1;
		}
		ISP_LOGV("AE_SET_ISO %d\n", cxt->cur_status.settings.iso);
		if (AE_SCENE_NORMAL == cxt->cur_status.settings.scene_mode) {
			cxt->cur_status.ae_table = &cxt->cur_param->ae_table[cxt->cur_status.settings.flicker][cxt->cur_status.settings.iso];
		}
	}
	return AE_SUCCESS;
}

static cmr_s32 ae_set_flicker(struct ae_ctrl_cxt *cxt, void *param)
{
	if (param) {
		struct ae_set_flicker *flicker = param;
		if (flicker->mode < AE_FLICKER_OFF) {
			cxt->cur_status.settings.flicker = flicker->mode;
			cxt->cur_status.ae_table = &cxt->cur_param->ae_table[cxt->cur_status.settings.flicker][AE_ISO_AUTO];
			if (AE_SCENE_NORMAL != cxt->sync_cur_status.settings.scene_mode) {
				cmr_u32 i = 0;
				struct ae_scene_info *scene_info = &cxt->cur_param->scene_info[0];
				for (i = 0; i < AE_SCENE_NUM; ++i) {
					ISP_LOGV("%d: mod: %d, eb: %d\n", i, scene_info[i].scene_mode, scene_info[i].enable);
					if ((1 == scene_info[i].enable) && ((cmr_u32) cxt->sync_cur_status.settings.scene_mode == scene_info[i].scene_mode)) {
						break;
					}
				}
				if (AE_SCENE_NUM > i) {
					if (scene_info[i].table_enable) {
						cxt->cur_status.ae_table = &scene_info[i].ae_table[cxt->cur_status.settings.flicker];
					}
				}
			}
			cxt->cur_status.ae_table->min_index = 0;
			ISP_LOGV("ae table: [%d, %d]\n", cxt->cur_status.ae_table->min_index, cxt->cur_status.ae_table->max_index);
		}
		ISP_LOGV("AE_SET_FLICKER %d\n", cxt->cur_status.settings.flicker);
	}
	return AE_SUCCESS;
}

static cmr_s32 ae_set_weight(struct ae_ctrl_cxt *cxt, void *param)
{
	if (param) {
		struct ae_set_weight *weight = param;

		if (weight->mode < AE_WEIGHT_MAX) {
			cxt->cur_status.settings.metering_mode = weight->mode;
		}
		ISP_LOGV("AE_SET_WEIGHT %d\n", cxt->cur_status.settings.metering_mode);
	}

	return AE_SUCCESS;
}

static cmr_s32 ae_set_touch_zone(struct ae_ctrl_cxt *cxt, void *param)
{
	cmr_s32 rtn = AE_SUCCESS;

	if (param) {
		struct ae_set_tuoch_zone *touch_zone = param;
		if ((touch_zone->touch_zone.w > 1) && (touch_zone->touch_zone.h > 1)) {
			cxt->cur_result.wts.stable = 0;
			cxt->cur_status.touch_scrn_win = touch_zone->touch_zone;
			cxt->cur_status.settings.touch_scrn_status = 1;
			cxt->cur_status.target_lum_zone = cxt->target_lum_zone_bak;

			if (cxt->cur_status.to_ae_state == 2)
				rtn = ae_set_restore_cnt(cxt);
			ISP_LOGV("AE_SET_TOUCH_ZONE ae triger %d", cxt->cur_status.settings.touch_scrn_status);
		} else {
			ISP_LOGV("AE_SET_TOUCH_ZONE touch ignore\n");
		}
	}

	return AE_SUCCESS;
}

static cmr_s32 ae_set_ev_offset(struct ae_ctrl_cxt *cxt, void *param)
{
	if (param) {
		struct ae_set_ev *ev = param;

		if (ev->level < AE_LEVEL_MAX) {
			cxt->mod_update_list.is_mev = 1;
			cxt->cur_status.settings.ev_index = ev->level;
			cxt->cur_status.target_lum = ae_calc_target_lum(cxt->cur_param->target_lum, cxt->cur_status.settings.ev_index, &cxt->cur_param->ev_table);
			cxt->cur_status.target_lum_zone = cxt->stable_zone_ev[cxt->cur_status.settings.ev_index];
			cxt->cur_status.stride_config[0] = cxt->cnvg_stride_ev[cxt->cur_status.settings.ev_index * 2];
			cxt->cur_status.stride_config[1] = cxt->cnvg_stride_ev[cxt->cur_status.settings.ev_index * 2 + 1];
		} else {
			/*ev auto */
			cxt->mod_update_list.is_mev = 0;
			cxt->cur_status.settings.ev_index = cxt->cur_param->ev_table.default_level;
			cxt->cur_status.target_lum = ae_calc_target_lum(cxt->cur_param->target_lum, cxt->cur_status.settings.ev_index, &cxt->cur_param->ev_table);
			cxt->cur_status.target_lum_zone = cxt->stable_zone_ev[cxt->cur_status.settings.ev_index];
			cxt->cur_status.stride_config[0] = cxt->cnvg_stride_ev[cxt->cur_status.settings.ev_index * 2];
			cxt->cur_status.stride_config[1] = cxt->cnvg_stride_ev[cxt->cur_status.settings.ev_index * 2 + 1];
		}
		ISP_LOGV("AE_SET_EV_OFFSET %d", cxt->cur_status.settings.ev_index);
	}
	return AE_SUCCESS;
}


static cmr_s32 ae_set_compensation_calc(struct ae_ctrl_cxt *cxt, cmr_u16 *out_idx)
{
	cmr_u16 calc_idx;
	cmr_u16 max_idx;
	cmr_s16 value = 0;
	float temp = 0.0;

	max_idx = cxt->cur_status.ae_table->max_index;

	temp = 1.0 * cxt->exposure_compensation.comp_val * cxt->exposure_compensation.step_numerator / cxt->exposure_compensation.step_denominator;
	temp = pow(2, temp);
	if (0 < cxt->exposure_compensation.comp_val) {
		value = (cmr_s16)(log(temp) / 0.0128 + 0.5);
		calc_idx = ((cxt->exposure_compensation.ae_base_idx + value) > max_idx) ? max_idx : (cxt->exposure_compensation.ae_base_idx + value);
	} else if (0 > cxt->exposure_compensation.comp_val) {
		value = (cmr_s16)(log(temp) / (-0.0132) + 0.5);
		calc_idx = (cxt->exposure_compensation.ae_base_idx < value) ? 0 : (cxt->exposure_compensation.ae_base_idx - value);
	} else {
		calc_idx = cxt->exposure_compensation.ae_base_idx;
		cxt->exposure_compensation.ae_ev_value = 1;
	}

	*out_idx = calc_idx;

	ISP_LOGD("value %d, temp %f calc_idx %d", value, temp, calc_idx);
	return AE_SUCCESS;
}

static cmr_s32 ae_set_compensation_calc_2(struct ae_ctrl_cxt *cxt, cmr_s16 *out_offset)
{
	float temp = 0.0;
	cmr_s16 target_offset = 0;

	temp = 1.0 * cxt->exposure_compensation.comp_val * cxt->exposure_compensation.step_numerator / cxt->exposure_compensation.step_denominator;
	ISP_LOGD("jhin1 comp_val %d step_numerator %d step_denominator %d \n",cxt->exposure_compensation.comp_val,cxt->exposure_compensation.step_numerator,cxt->exposure_compensation.step_denominator);
	temp = pow(2, temp);
	ISP_LOGV("jhin1 temp%f\n",temp);
	if (0 < cxt->exposure_compensation.comp_val) {
		target_offset = cxt->exposure_compensation.ae_base_target * temp;
	} else if (0 > cxt->exposure_compensation.comp_val) { 
		target_offset = cxt->exposure_compensation.ae_base_target * (temp -1);
	} else {
		target_offset = 0;
	}

	*out_offset = target_offset;

	ISP_LOGD("jhin1 value %d, temp %f ", target_offset, temp);
	return AE_SUCCESS;
}

static cmr_s32 ae_set_exposure_compensation(struct ae_ctrl_cxt *cxt, struct ae_exp_compensation *exp_comp)
{
	cmr_u16 change_idx = 0;
	cmr_s16 change_offset = 0;

	if (exp_comp) {
		if (1 == cxt->app_mode) {
			struct ae_set_ev ev;
			ev.level = exp_comp->comp_val + exp_comp->comp_range.max;
			if (ev.level < AE_LEVEL_MAX) {
				cxt->mod_update_list.is_mev = 1;
				cxt->cur_status.settings.ev_index = ev.level;
			} else {
				/*ev auto */
				cxt->mod_update_list.is_mev = 0;
				cxt->cur_status.settings.ev_index = cxt->cur_param->ev_table.default_level;
			}
			cxt->cur_status.target_lum = ae_calc_target_lum(cxt->cur_param->target_lum, cxt->cur_status.settings.ev_index, &cxt->cur_param->ev_table);
			cxt->cur_status.target_lum_zone = cxt->stable_zone_ev[cxt->cur_status.settings.ev_index];
			cxt->cur_status.stride_config[0] = cxt->cnvg_stride_ev[cxt->cur_status.settings.ev_index * 2];
			cxt->cur_status.stride_config[1] = cxt->cnvg_stride_ev[cxt->cur_status.settings.ev_index * 2 + 1];
			ISP_LOGD("ev.level:%d, comp_val: %d, comp_range.max:%d",ev.level, exp_comp->comp_val, exp_comp->comp_range.max);
		} else {
			if (cxt->cur_status.settings.force_lock_ae == 1) {
				cxt->exposure_compensation.comp_val = exp_comp->comp_val;
				cxt->exposure_compensation.step_numerator = exp_comp->step_numerator;
				cxt->exposure_compensation.step_denominator = exp_comp->step_denominator;
				if (0 ==exp_comp->comp_val && 0 == cxt->exposure_compensation.touch_ev_flag) {
					cxt->cur_status.settings.manual_mode = 2;
					if(cxt->exposure_compensation.ae_base_idx!=0){
						cxt->cur_status.settings.table_idx = cxt->exposure_compensation.ae_base_idx;
					}
				} else {
					cxt->exposure_compensation.touch_ev_flag = 1;//keep index (fps) on first touch ev
					ae_set_compensation_calc(cxt, &change_idx);
					cxt->expchanged = 1;
					cxt->cur_status.settings.manual_mode = 1;
					cxt->cur_status.settings.table_idx = change_idx;
					if(cxt->cur_status.settings.table_idx!=cxt->last_table_index){
						cxt->last_table_index = cxt->cur_status.settings.table_idx;
					}
				}
				ISP_LOGV("jhin manual_mode:%d, exp_comp->comp_val:%d,exp_comp->step_numerator:%d,exp_comp->step_denominator:%d", cxt->cur_status.settings.manual_mode, exp_comp->comp_val,exp_comp->step_numerator,exp_comp->step_denominator);
				ISP_LOGV("jhin change_idx:%d",change_idx);
			} else {
				if (cxt->exposure_compensation.comp_val == 0){
					cxt->exposure_compensation.ae_base_target = cxt->cur_param->target_lum;
				}
				cxt->exposure_compensation.comp_val = exp_comp->comp_val;
				cxt->exposure_compensation.step_numerator = exp_comp->step_numerator;
				cxt->exposure_compensation.step_denominator = exp_comp->step_denominator;
				ae_set_compensation_calc_2(cxt, &change_offset);
				cxt->cur_status.target_lum = cxt->exposure_compensation.ae_base_target + change_offset;
			}
		}
	}

	return AE_SUCCESS;
}

static cmr_s32 ae_set_scene_info(struct ae_ctrl_cxt *cxt,  void *param)
{
	if (param) {
		struct ai_scene_detect_info *scene_info = (struct ai_scene_detect_info *) param;

		memcpy(&cxt->cur_status.detect_scene, scene_info, sizeof(struct ai_scene_detect_info));
		ISP_LOGV("done");
	}

	return AE_SUCCESS;
}

static cmr_s32 ae_set_auto_hdr(struct ae_ctrl_cxt *cxt,  void *param)
{
	if (param) {
		cmr_u8 *menu_ctrl = (cmr_u8 *) param;
		cxt->hdr_menu_ctrl = *menu_ctrl;
		ISP_LOGE("hdr_menu %d", cxt->hdr_menu_ctrl);
	}
	return AE_SUCCESS;
}

static cmr_s32 ae_set_fps_info(struct ae_ctrl_cxt *cxt, void *param)
{
	if (param) {
		if (!cxt->high_fps_info.is_high_fps) {
			struct ae_set_fps *fps = param;
			cxt->fps_range.min = fps->min_fps;
			cxt->fps_range.max = fps->max_fps;
			ISP_LOGV("AE_SET_FPS (%d, %d)", fps->min_fps, fps->max_fps);
		}
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

		if (bv <= cxt->flash_swith.led_thr_down)
			*flash_eb = 1;
		else if (bv > cxt->flash_swith.led_thr_up)
			*flash_eb = 0;

		ISP_LOGV("AE_GET_FLASH_EB: flash_eb=%d, bv=%d, on_thr=%d, off_thr=%d", *flash_eb, bv, cxt->flash_swith.led_thr_down, cxt->flash_swith.led_thr_up);
	}
	return rtn;
}

static cmr_s32 ae_get_gain(struct ae_ctrl_cxt *cxt, void *result)
{
	if (result) {
		cmr_s32 *bv = (cmr_s32 *) result;

		*bv = cxt->cur_result.wts.cur_again;
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

static cmr_s32 ae_get_ev(struct ae_ctrl_cxt *cxt, void *result)
{
	if (result) {
		cmr_u32 i = 0x00;
		struct ae_ev_table *ev_table = &cxt->cur_param->ev_table;
		cmr_s32 target_lum = cxt->cur_param->target_lum;
		struct ae_get_ev *ev_info = (struct ae_get_ev *)result;
		ev_info->ev_index = cxt->cur_status.settings.ev_index;

		for (i = 0; i < ev_table->diff_num; i++) {
			ev_info->ev_tab[i] = target_lum + ev_table->lum_diff[i];
		}
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

static cmr_s32 ae_set_target_luma(struct ae_ctrl_cxt *cxt, void *param)
{
	if (param) {
		cmr_u32 *lum = (cmr_u32 *) param;

		cxt->cur_status.target_lum = *lum;
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

		*mode = cxt->cur_status.settings.flicker;
	}

	return AE_SUCCESS;
}

static cmr_s32 ae_set_hdr_start(struct ae_ctrl_cxt *cxt, void *param)
{
	if (param) {
		struct ae_hdr_param *hdr_param = (struct ae_hdr_param *)param;
		cxt->hdr_enable = hdr_param->hdr_enable;
		cxt->hdr_cb_cnt = hdr_param->ev_effect_valid_num;
		cxt->hdr_frame_cnt = 0;
		if (cxt->hdr_enable) {
			cxt->hdr_flag = 3;
			cxt->hdr_base_ae_idx = cxt->sync_cur_result.wts.cur_index;
			ae_set_force_pause(cxt, 1);
		} else {
			ae_set_force_pause(cxt, 0);
			cxt->cur_status.settings.lock_ae = AE_STATE_NORMAL;
		}
		ISP_LOGV("AE_SET_HDR: hdr_enable %d, hdr_cb_cnt %d, base_ae_idx %d, lock_ae_state %d, force_lock_ae_state %d",
			cxt->hdr_enable,
			cxt->hdr_cb_cnt,
			cxt->hdr_base_ae_idx,
			cxt->cur_status.settings.lock_ae,
			cxt->cur_status.settings.force_lock_ae);
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
		*fps = (cmr_u32) (cxt->sync_cur_result.wts.cur_fps + 0.5);
		ISP_LOGV("fps: %f\n", cxt->sync_cur_result.wts.cur_fps);
	} else {
		ISP_LOGE("result pointer is NULL");
		rtn = AE_ERROR;
	}

	return rtn;
}

static cmr_s32 ae_set_caf_lockae_start(struct ae_ctrl_cxt *cxt)
{
	cxt->target_lum_zone_bak = cxt->cur_status.target_lum_zone;

	if (cxt->cur_result.wts.stable) {
		cxt->cur_status.target_lum_zone = cxt->stable_zone_ev[15];
		if (cxt->cur_status.target_lum_zone < cxt->target_lum_zone_bak) {
			cxt->cur_status.target_lum_zone = cxt->target_lum_zone_bak;
		}
	}

	return AE_SUCCESS;
}

static cmr_s32 ae_set_caf_lockae_stop(struct ae_ctrl_cxt *cxt)
{
	cxt->cur_status.target_lum_zone = cxt->target_lum_zone_bak;

	return AE_SUCCESS;
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

#ifndef CONFIG_ISP_2_2
static cmr_s32 ae_set_isp_gain(struct ae_ctrl_cxt *cxt)
{
	if (cxt->is_multi_mode == ISP_ALG_DUAL_SBS && !cxt->is_master) {
		struct ae_match_data ae_match_data_slave;
		cxt->ptr_isp_br_ioctrl(cxt->is_master ? CAM_SENSOR_MASTER : CAM_SENSOR_SLAVE0, GET_MATCH_AE_DATA, NULL, &ae_match_data_slave);
		cmr_u32 isp_gain = ae_match_data_slave.isp_gain;
		if (0 != isp_gain) {
			double rgb_coeff = isp_gain * 1.0 / 4096;
			if (cxt->isp_ops.set_rgb_gain) {
				cxt->isp_ops.set_rgb_gain(cxt->isp_ops.isp_handler, rgb_coeff);
			}
		}
	}
	else if((cxt->is_multi_mode == ISP_ALG_DUAL_C_C||cxt->is_multi_mode ==ISP_ALG_DUAL_W_T||cxt->is_multi_mode ==ISP_ALG_DUAL_C_M)&& !cxt->is_master) {
		struct ae_match_data ae_match_data_slave;
		cxt->ptr_isp_br_ioctrl(cxt->is_master ? CAM_SENSOR_MASTER : CAM_SENSOR_SLAVE0, GET_MATCH_AE_DATA, NULL, &ae_match_data_slave);
		cmr_u32 isp_gain = ae_match_data_slave.isp_gain;
		if (0 != isp_gain) {
			double rgb_coeff = isp_gain * 1.0 / 4096;
			if (cxt->isp_ops.set_rgb_gain) {
				cxt->isp_ops.set_rgb_gain(cxt->isp_ops.isp_handler, rgb_coeff);
			}
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
#endif

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
		if (init_param->is_master) {
			ae_otp_info_ptr = init_param->otp_info_ptr->dual_otp.master_ae_info;
			module_info_ptr = init_param->otp_info_ptr->dual_otp.master_module_info;
			ISP_LOGV("pass ae otp, dual cam master");
		} else {
			ae_otp_info_ptr = init_param->otp_info_ptr->dual_otp.slave_ae_info;
			module_info_ptr = init_param->otp_info_ptr->dual_otp.slave_module_info;
			ISP_LOGV("pass ae otp, dual cam slave");
		}
	} else {
		ae_otp_info_ptr = NULL;
		module_info_ptr = NULL;

		if(init_param->is_multi_mode == ISP_ALG_SINGLE ){
			ISP_LOGI("single camera,dual ae sync no exist.\n");
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
			info.ae_target_lum = (rdm_otp_data[1] << 8) | rdm_otp_data[0];
			info.gain_1x_exp = (rdm_otp_data[5] << 24) | (rdm_otp_data[4] << 16) | (rdm_otp_data[3] << 8) | rdm_otp_data[2];
			info.gain_2x_exp = (rdm_otp_data[9] << 24) | (rdm_otp_data[8] << 16) | (rdm_otp_data[7] << 8) | rdm_otp_data[6];
			info.gain_4x_exp = (rdm_otp_data[13] << 24) | (rdm_otp_data[12] << 16) | (rdm_otp_data[11] << 8) | rdm_otp_data[10];
			info.gain_8x_exp = (rdm_otp_data[17] << 24) | (rdm_otp_data[16] << 16) | (rdm_otp_data[15] << 8) | rdm_otp_data[14];
			ISP_LOGV("ae otp map:(gain_1x_exp:%d),(gain_2x_exp:%d),(gain_4x_exp:%d),(gain_8x_exp:%d).\n",
					    (int)info.gain_1x_exp,(int)info.gain_2x_exp,(int)info.gain_4x_exp,(int)info.gain_8x_exp);

#ifdef CONFIG_ISP_2_2
			if (init_param->is_master) {
				rtn = init_param->ptr_isp_br_ioctrl(init_param->is_master ? CAM_SENSOR_MASTER : CAM_SENSOR_SLAVE0,SET_MASTER_OTP_AE, &info, NULL);
			} else {
				rtn = init_param->ptr_isp_br_ioctrl(init_param->is_master ? CAM_SENSOR_MASTER : CAM_SENSOR_SLAVE0,SET_SLAVE_OTP_AE, &info, NULL);
			}
#else
			rtn = init_param->ptr_isp_br_ioctrl(init_param->is_master ? CAM_SENSOR_MASTER : CAM_SENSOR_SLAVE0, SET_OTP_AE, &info, NULL);
#endif

			//ISP_LOGV("lum=%" PRIu16 ", 1x=%" PRIu64 ", 2x=%" PRIu64 ", 4x=%" PRIu64 ", 8x=%" PRIu64, info.ae_target_lum,info.gain_1x_exp,info.gain_2x_exp,info.gain_4x_exp,info.gain_8x_exp);
		} else {
			ISP_LOGE("ae rdm_otp_data = %p, rdm_otp_len = %d. Parser fail", rdm_otp_data, rdm_otp_len);
		}
	} else {
		if(init_param->is_multi_mode == ISP_ALG_SINGLE ){
			ISP_LOGI("single camera,dual ae sync no exist.\n");
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
		ISP_LOGI("sensor type not support");
		break;
	}
	return AE_SUCCESS;
}

static cmr_s32 ae_get_calc_reuslts(struct ae_ctrl_cxt *cxt, cmr_handle result)
{
	cmr_s32 rtn = AE_SUCCESS;
	struct ae_calc_results *calc_result = (struct ae_calc_results *)result;

	if (NULL == calc_result) {
		ISP_LOGE("results pointer is invalidated\n");
		rtn = AE_ERROR;
		return rtn;
	}

	memcpy(calc_result, &cxt->calc_results, sizeof(struct ae_calc_results));

	return rtn;
}

#define BLK_NUM_W_ALG 32
static void down_size_for_ae_stat(struct ae_ctrl_cxt *cxt, void * img_stat)
{
	cmr_u32 i,j,ii,jj;
	cmr_u64 r = 0, g = 0, b = 0;
	cmr_u64 sum, avg, max_value = 0x3ff;
	cmr_u32 blk_num_w = cxt->monitor_cfg.blk_num.w;
	cmr_u32 blk_num_h = cxt->monitor_cfg.blk_num.h;
	cmr_u32 bayer_pixels = cxt->monitor_cfg.blk_size.w * cxt->monitor_cfg.blk_size.h/4;
	cmr_u32 *src_aem_stat = (cmr_u32 *) img_stat;
	cmr_u32 *r_stat = (cmr_u32*)src_aem_stat;
	cmr_u32 *g_stat = (cmr_u32*)src_aem_stat + 16384;
	cmr_u32 *b_stat = (cmr_u32*)src_aem_stat + 2 * 16384;

	cmr_u32 ratio_h = blk_num_h/BLK_NUM_W_ALG;
	cmr_u32 ratio_w = blk_num_w/BLK_NUM_W_ALG;

	memset(cxt->sync_aem,0,1024 * 3* sizeof(cmr_u32));

	cxt->cur_status.base_size.h = cxt->monitor_cfg.blk_size.h;
	cxt->cur_status.base_size.w = cxt->monitor_cfg.blk_size.w;
	cxt->cur_status.base_num.w = blk_num_w;
	cxt->cur_status.base_num.h = blk_num_h;
	cxt->cur_status.win_num.w = 32;
	cxt->cur_status.win_num.h = 32;
	cxt->cur_status.win_size.w = ratio_w * cxt->monitor_cfg.blk_size.w;
	cxt->cur_status.win_size.h = ratio_h * cxt->monitor_cfg.blk_size.h;

	for (i = 0; i < blk_num_h; ++i) {
		ii = (cmr_u32)(i / ratio_h);
		for (j = 0; j < blk_num_w; ++j) {
			jj = j / ratio_w;
			/*for r channel */
			sum = r_stat[i * blk_num_w + j];
			avg = sum/ bayer_pixels;
			r = avg > max_value ? max_value : avg;

			/*for g channel */
			sum = g_stat[i * blk_num_w + j];
			avg = sum/ bayer_pixels;
			g = avg > max_value ? max_value : avg;

			/*for b channel */
			sum = b_stat[i * blk_num_w + j];
			avg = sum/ bayer_pixels;
			b = avg > max_value ? max_value : avg;

			cxt->sync_aem[ii * BLK_NUM_W_ALG + jj] += r/(ratio_w * ratio_h);
			cxt->sync_aem[ii * BLK_NUM_W_ALG + jj + 1024] += g/(ratio_w * ratio_h);
			cxt->sync_aem[ii * BLK_NUM_W_ALG + jj + 2048] += b/(ratio_w * ratio_h);

			cxt->cur_status.base_img[i * blk_num_w + j] = r;
			cxt->cur_status.base_img[i * blk_num_w + j + blk_num_w * blk_num_h] = g;
			cxt->cur_status.base_img[i * blk_num_w + j + 2 * blk_num_w * blk_num_h] = b;
		}
	}
}

static cmr_s32 ae_calculation_slow_motion(cmr_handle handle, cmr_handle param, cmr_handle result)
{
	cmr_s32 rtn = AE_ERROR;
	cmr_int cb_type;
	cmr_u32 stat_chnl_len = 0;
	struct ae_ctrl_cxt *cxt = NULL;
	struct ae_alg_calc_param *current_status;
	struct ae_alg_calc_result *current_result;
	struct ae_misc_calc_in misc_calc_in = { 0 };
	struct ae_misc_calc_out misc_calc_out = { 0 };
	struct ae_calc_in *calc_in = NULL;
	struct ae_calc_results *cur_calc_result = NULL;
	struct ae_ctrl_callback_in callback_in;
	cmr_s32 backup_expline = 0;
	cmr_s32 backup_gain = 0;
	cmr_s32 backup_expgain = 0;
	cmr_s32 effect_ebd_expgain = 0;
	UNUSED(result);

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
	// acc_info_print(cxt);
	cxt->cur_status.awb_gain.b = calc_in->awb_gain_b;
	cxt->cur_status.awb_gain.g = calc_in->awb_gain_g;
	cxt->cur_status.awb_gain.r = calc_in->awb_gain_r;
	cxt->cur_status.awb_cur_gain.b = calc_in->awb_cur_gain_b;
	cxt->cur_status.awb_cur_gain.g = calc_in->awb_cur_gain_g;
	cxt->cur_status.awb_cur_gain.r = calc_in->awb_cur_gain_r;
	cxt->cur_status.awb_mode = calc_in->awb_mode;
	//memcpy(cxt->sync_aem, calc_in->stat_img, 3 * 1024 * sizeof(cmr_u32));
	down_size_for_ae_stat(cxt, calc_in->stat_img);
	cxt->cur_status.stat_img = cxt->sync_aem;
	stat_chnl_len = calc_in->binning_stat_info.binning_size.w * calc_in->binning_stat_info.binning_size.h;
	memcpy(&cxt->sync_binning_stats_data[0], calc_in->binning_stat_info.r_info, stat_chnl_len * sizeof(cmr_u16));
	memcpy(&cxt->sync_binning_stats_data[stat_chnl_len], calc_in->binning_stat_info.g_info, stat_chnl_len * sizeof(cmr_u16));
	memcpy(&cxt->sync_binning_stats_data[2 * stat_chnl_len], calc_in->binning_stat_info.b_info, stat_chnl_len * sizeof(cmr_u16));
	cxt->binning_stat_size.w = calc_in->binning_stat_info.binning_size.w;
	cxt->binning_stat_size.h = calc_in->binning_stat_info.binning_size.h;
	cxt->cur_status.binning_stat_data = &cxt->sync_binning_stats_data[0];
	cxt->cur_status.binnig_stat_size.w = calc_in->binning_stat_info.binning_size.w;
	cxt->cur_status.binnig_stat_size.h = calc_in->binning_stat_info.binning_size.h;
	// get effective E&g
	cxt->cur_status.effect_expline = cxt->exp_data.actual_data.exp_line;
	cxt->cur_status.effect_dummy = cxt->exp_data.actual_data.dummy;
	cxt->cur_status.effect_gain = cxt->exp_data.actual_data.isp_gain * cxt->exp_data.actual_data.sensor_gain / 4096;

	backup_expline = cxt->cur_status.effect_expline;
	backup_gain = cxt->cur_status.effect_gain;
	backup_expgain = backup_expline*backup_gain;
	ISP_LOGV("ebd(slow motion): ae_lib effect_expline %d, effect_gain %d", cxt->cur_status.effect_expline, cxt->cur_status.effect_gain);
	if(cxt->ebd_support){
		cxt->cur_status.effect_expline =  calc_in->ebd_info.exposure_valid ?calc_in->ebd_info.exposure : cxt->exp_data.actual_data.exp_line;
#if CONFIG_ISP_2_3
		cxt->cur_status.effect_gain = (cmr_u32)(1.0 *calc_in->ebd_info.gain + 0.5);
#else
		cxt->cur_status.effect_gain = (cmr_u32)(1.0 *calc_in->ebd_info.gain * calc_in->isp_dgain.global_gain/(4096.0 *cxt->ob_rgb_gain)+ 0.5);
#endif
	effect_ebd_expgain = cxt->cur_status.effect_expline * cxt->cur_status.effect_gain;
	ISP_LOGV("ebd(slow motion): sensor effect_expline %d, effect_gain %d", cxt->cur_status.effect_expline, cxt->cur_status.effect_gain);
	}
	if(cxt->ebd_support) {
		if((1.0 * backup_expgain / effect_ebd_expgain >= 0.97) && (1.0 * backup_expgain / effect_ebd_expgain <= 1.03))
			cxt->ebd_stable_flag = 1;
		else
			cxt->ebd_stable_flag = 0;
		ISP_LOGV("ebd(slow motion): stable_flag %d", cxt->ebd_stable_flag);
	}
#if CONFIG_ISP_2_3
	/*it will be enable lately */
	ae_set_soft_gain(cxt, cxt->sync_aem, cxt->sync_aem, cxt->cur_status.win_num, cxt->exp_data.actual_data.isp_gain);
#endif
	cxt->cur_result.face_lum = current_result->face_lum;	//for debug face lum
	cxt->sync_aem[3 * 1024] = cxt->cur_status.frame_id;
	cxt->sync_aem[3 * 1024 + 1] = cxt->cur_status.effect_expline;
	cxt->sync_aem[3 * 1024 + 2] = cxt->cur_status.effect_dummy;
	cxt->sync_aem[3 * 1024 + 3] = cxt->cur_status.effect_gain;
	memcpy(current_status, &cxt->cur_status, sizeof(struct ae_alg_calc_param));
	memcpy(&cxt->cur_result, current_result, sizeof(struct ae_alg_calc_result));
	cxt->cur_param = &cxt->tuning_param[0];
	// change weight_table
	current_status->weight_table = cxt->cur_param->weight_table[current_status->settings.metering_mode].weight;
	// change ae_table
#if 0
	current_status->ae_table = &cxt->cur_param->scene_info[current_status->settings.scene_mode].ae_table[current_status->settings.flicker];
#else
	// for now video using
	current_status->ae_table = &cxt->cur_param->ae_table[current_status->settings.flicker][AE_ISO_AUTO];
	current_status->ae_table->min_index = 0;	//AE table start index = 0
#endif
	// change settings related by EV
	current_status->target_lum = ae_calc_target_lum(cxt->cur_param->target_lum, cxt->cur_status.settings.ev_index, &cxt->cur_param->ev_table);
	current_status->target_lum_zone = cxt->stable_zone_ev[current_status->settings.ev_index];
	current_status->stride_config[0] = cxt->cnvg_stride_ev[current_status->settings.ev_index * 2];
	current_status->stride_config[1] = cxt->cnvg_stride_ev[current_status->settings.ev_index * 2 + 1];

	current_status->ae_start_delay = 0;
	misc_calc_in.sync_settings = current_status;
	misc_calc_out.ae_output = &cxt->cur_result;
	cxt->sync_cur_status.settings.min_fps = cxt->high_fps_info.min_fps;
	cxt->sync_cur_status.settings.max_fps = cxt->high_fps_info.max_fps;
	cmr_u64 ae_time0 = systemTime(CLOCK_MONOTONIC);
	rtn = ae_misc_calculation(cxt->misc_handle, &misc_calc_in, &misc_calc_out);
	cmr_u64 ae_time1 = systemTime(CLOCK_MONOTONIC);
	ISP_LOGV("SYSTEM_TEST -ae	%dus ", (cmr_s32) ((ae_time1 - ae_time0) / 1000));
       if (cxt->cur_status.settings.exp_is_transmit ==1){
               cxt->cur_status.settings.exp_is_transmit = 0;   //release transmit expsoure value to ae
               if(cxt->cur_status.settings.manual_mode == 1){
                       cxt->cur_status.settings.manual_mode = -1;
               }
       }

	cxt->cur_status.ae1_finfo.update_flag = 0;	// add by match box for fd_ae reset the status

	memcpy(current_result, &cxt->cur_result, sizeof(struct ae_alg_calc_result));
	memcpy(&cur_calc_result->ae_result, current_result, sizeof(struct ae_alg_calc_result));
	ae_make_calc_result(cxt, current_result, cur_calc_result);

	/*just for debug: reset the status */
	if (1 == cxt->cur_status.settings.touch_scrn_status) {
		cxt->cur_status.settings.touch_scrn_status = 0;
	}

	cxt->exp_data.lib_data.exp_line = current_result->wts.cur_exp_line;
	cxt->exp_data.lib_data.gain = current_result->wts.cur_again;
	cxt->exp_data.lib_data.dummy = current_result->wts.cur_dummy;
	cxt->exp_data.lib_data.line_time = current_status->line_time;
	cxt->exp_data.lib_data.exp_time = current_result->wts.exposure_time;
	ae_update_result_to_sensor(cxt, &cxt->exp_data, 0);

/***********************************************************/
/* send STAB notify to HAL */

	if (cxt->isp_ops.callback) {
		cb_type = AE_CB_STAB_NOTIFY;
		(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, cb_type, &cur_calc_result->ae_output.is_stab);
	}


	if (1 == cxt->debug_enable) {
		ae_save_to_mlog_file(cxt, &misc_calc_out);
	}

	if (cxt->isp_ops.callback) {
		ae_make_isp_result(cxt, current_result, &callback_in);
		(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, AE_CB_PROCESS_OUT, &callback_in);
	}
	cxt->cur_status.frame_id++;

  ERROR_EXIT:
	return rtn;
}

cmr_s32 ae_calculation(cmr_handle handle, cmr_handle param, cmr_handle result)
{
	cmr_s32 rtn = AE_ERROR;
	cmr_int cb_type;
	cmr_s32 stop_skip_en;
	cmr_u32 stat_chnl_len = 0;
	struct ae_ctrl_cxt *cxt = NULL;
	struct ae_alg_calc_param *current_status = NULL;
	struct ae_alg_calc_result *current_result = NULL;
	struct ae_alg_calc_param *alg_status_ptr = NULL;	//DEBUG
	struct ae_misc_calc_in misc_calc_in = { 0 };
	struct ae_misc_calc_out misc_calc_out = { 0 };
	struct ae_calc_in *calc_in = NULL;
	struct ae_calc_results *cur_calc_result = NULL;
	struct ae_ctrl_callback_in callback_in;
	cmr_s32 backup_expline = 0;
	cmr_s32 backup_gain = 0;
	cmr_s32 backup_expgain = 0;
	cmr_s32 effect_ebd_expgain = 0;
	UNUSED(result);
#ifdef CONFIG_SUPPROT_AUTO_HDR
	struct _tag_hdr_detect_t hdr_param;
	struct _tag_hdr_stat_t hdr_stat;
	float ev_result[2] = {0,0};
	cmr_s8 auto_hdr_enable = 0;

#endif

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

#ifdef CONFIG_ISP_2_2
	enum sync_status ae_sync_status;
	enum sync_status ae_sync_status_temp;

	if (cxt->is_multi_mode && cxt->is_master) {
		rtn = cxt->ptr_isp_br_ioctrl(cxt->is_master ? CAM_SENSOR_MASTER : CAM_SENSOR_SLAVE0, GET_MASTER_AE_SYNC_STATUS, NULL, &ae_sync_status);
		if (ae_sync_status == SYNC_INIT) {
			ae_sync_status_temp = SYNC_RUN;
			rtn = cxt->ptr_isp_br_ioctrl(cxt->is_master ? CAM_SENSOR_MASTER : CAM_SENSOR_SLAVE0, SET_MASTER_AE_SYNC_STATUS, &ae_sync_status_temp, NULL);
		}

		rtn = cxt->ptr_isp_br_ioctrl(cxt->is_master ? CAM_SENSOR_MASTER : CAM_SENSOR_SLAVE0, GET_SLAVE_AE_SYNC_STATUS, NULL, &ae_sync_status_temp);

		if (ae_sync_status_temp != SYNC_RUN || ae_sync_status != SYNC_RUN) {
			ISP_LOGE("master: fail to get slave ae_sync_status  =%d", ae_sync_status);
			cxt->calc_results.is_skip_cur_frame = 1;
			return AE_SKIP_FRAME;
		}
	} else if (cxt->is_multi_mode && !cxt->is_master) {
		rtn = cxt->ptr_isp_br_ioctrl(cxt->is_master ? CAM_SENSOR_MASTER : CAM_SENSOR_SLAVE0, GET_SLAVE_AE_SYNC_STATUS, NULL, &ae_sync_status);
		if (ae_sync_status == SYNC_INIT) {
			ae_sync_status = SYNC_RUN;
			rtn = cxt->ptr_isp_br_ioctrl(cxt->is_master ? CAM_SENSOR_MASTER : CAM_SENSOR_SLAVE0, SET_SLAVE_AE_SYNC_STATUS, &ae_sync_status, NULL);
		}

		rtn = cxt->ptr_isp_br_ioctrl(cxt->is_master ? CAM_SENSOR_MASTER : CAM_SENSOR_SLAVE0, GET_MASTER_AE_SYNC_STATUS, NULL, &ae_sync_status);

		if (ae_sync_status != SYNC_RUN) {
			ISP_LOGE("slave: fail to get master ae_sync_status=%d", ae_sync_status);
			return AE_SKIP_FRAME;
		}
	}
#endif
	if (cxt->bypass) {
		ae_set_pause(cxt);
	}

	calc_in = (struct ae_calc_in *)param;

	// acc_info_print(cxt);
	cxt->cur_status.awb_gain.b = calc_in->awb_gain_b;
	cxt->cur_status.awb_gain.g = calc_in->awb_gain_g;
	cxt->cur_status.awb_gain.r = calc_in->awb_gain_r;
	cxt->cur_status.awb_cur_gain.b = calc_in->awb_cur_gain_b;
	cxt->cur_status.awb_cur_gain.g = calc_in->awb_cur_gain_g;
	cxt->cur_status.awb_cur_gain.r = calc_in->awb_cur_gain_r;
	cxt->cur_status.awb_mode = calc_in->awb_mode;
	//memcpy(cxt->sync_aem, calc_in->stat_img, 3 * 1024 * sizeof(cmr_u32));
	down_size_for_ae_stat(cxt, calc_in->stat_img);
	cxt->cur_status.stat_img = cxt->sync_aem;
	stat_chnl_len = calc_in->binning_stat_info.binning_size.w * calc_in->binning_stat_info.binning_size.h;
	memcpy(&cxt->sync_binning_stats_data[0], calc_in->binning_stat_info.r_info, stat_chnl_len * sizeof(cmr_u16));
	memcpy(&cxt->sync_binning_stats_data[stat_chnl_len], calc_in->binning_stat_info.g_info, stat_chnl_len * sizeof(cmr_u16));
	memcpy(&cxt->sync_binning_stats_data[2 * stat_chnl_len], calc_in->binning_stat_info.b_info, stat_chnl_len * sizeof(cmr_u16));
	cxt->binning_stat_size.w = calc_in->binning_stat_info.binning_size.w;
	cxt->binning_stat_size.h = calc_in->binning_stat_info.binning_size.h;
	cxt->cur_status.binning_stat_data = &cxt->sync_binning_stats_data[0];
	cxt->cur_status.binnig_stat_size.w = calc_in->binning_stat_info.binning_size.w;
	cxt->cur_status.binnig_stat_size.h = calc_in->binning_stat_info.binning_size.h;

	ISP_LOGV("AE_sync  is_multi_mode=%d, is_master=%d",cxt->is_multi_mode,cxt->is_master);

	static int ae_dynamic_sync = 0;
	if((cxt->is_multi_mode) && (cxt->is_master == 1) && (cxt->ae_sync_param.mode == 1))
	{
		ae_dynamic_sync = 1;
	}
	ISP_LOGV(" AE_sync mode:0x%x.\n",ae_dynamic_sync);

	if(ae_dynamic_sync == 1){
	cmr_s32 master_sync_aem[3*1024] = {0},slave_sync_aem[3*1024]= {0};
	cmr_s32 aem_blk_num = 1024;
	rtn = cxt->ptr_isp_br_ioctrl(cxt->is_master ? CAM_SENSOR_MASTER : CAM_SENSOR_SLAVE0, SET_AEM_STAT_BLK_NUM, &aem_blk_num, NULL);

	if(rtn)
	{
		ISP_LOGE("Dynamic AE_sync mode, set AEM blk num err.\n");
	}
	if(cxt->is_multi_mode && cxt->is_master)
	{
		for(int i = 0;i < 3*1024;i++)
		{
			master_sync_aem[i] = cxt->sync_aem[i] /4 ;
		}
		rtn = cxt->ptr_isp_br_ioctrl(cxt->is_master ? CAM_SENSOR_MASTER : CAM_SENSOR_SLAVE0, SET_AEM_SYNC_STAT, master_sync_aem, NULL );
		if(rtn)
		{
			ISP_LOGE("Dynamic AE_sync mode,  set master AEM data err.\n");
		}
		else
		{
			ISP_LOGV("Dynamic AE_sync mode,  master win_size.h=%d, win_size.w=%d",cxt->cur_status.win_size.h ,cxt->cur_status.win_size.w);
		}
	}

	else  if(cxt->is_multi_mode && !cxt->is_master)
	{
		for(int i = 0;i < 3*1024;i++)
		{
			slave_sync_aem[i] = cxt->sync_aem[i] /4;
		}
		rtn = cxt->ptr_isp_br_ioctrl(cxt->is_master ? CAM_SENSOR_MASTER : CAM_SENSOR_SLAVE0, SET_AEM_SYNC_STAT,slave_sync_aem, NULL );

		if(rtn)
		{
			ISP_LOGE("Dynamic AE_sync mode,  set slaves AEM data err.\n");
		}
		else
		{
			ISP_LOGV("Dynamic AE_sync mode,  slave win_size.h=%d, win_size.w=%d",cxt->cur_status.win_size.h ,cxt->cur_status.win_size.w);
		}
	}

# if 0 //used for dump master & slave data20171221s
	FILE *fp;
	int i,j;
	fp = fopen(AEM_MASTER_STAT_FILE,"w");

	//R
	for( j=0;j<32;j++)
	{
		for( i=0;i<32;i++)
		{
			fprintf(fp,"%d   ",master_sync_aem[j*32+i]  / 4);
		}
		fprintf(fp,"\n");
	}

	fprintf(fp,"\n\n");

	//G
	for( j=0;j<32;j++)
	{
		for( i=0;i<32;i++)
		{
			fprintf(fp,"%d   ",master_sync_aem[j*32+i+1024]   /4);
		}
		fprintf(fp,"\n");
	}
	fprintf(fp,"\n\n");

	//B
	for( j=0;j<32;j++)
	{
		for( i=0;i<32;i++)
		{
			fprintf(fp,"%d   ",master_sync_aem[j*32+i+2048]  /4);
		}
		fprintf(fp,"\n");
	}
	fprintf(fp,"\n\n");

	fclose(fp);



	fp = fopen(AEM_SLAVE_STAT_FILE,"w");

	//R
	for( j=0;j<32;j++)
	{
		for( i=0;i<32;i++)
		{
			fprintf(fp,"%d   ",slave_sync_aem[j*32+i]  /4);
		}
		fprintf(fp,"\n");
	}
	fprintf(fp,"\n\n");


	//G
	for( j=0;j<32;j++)
	{
		for( i=0;i<32;i++)
		{
			fprintf(fp,"%d   ",slave_sync_aem[j*32+i+1024]   /4);
		}
		fprintf(fp,"\n");
	}
	fprintf(fp,"\n\n");


	//B
	for( j=0;j<32;j++)
	{
		for( i=0;i<32;i++)
		{
			fprintf(fp,"%d   ",slave_sync_aem[j*32+i+2048]  /4);
		}
		fprintf(fp,"\n");
	}
	fprintf(fp,"\n\n");


	fclose(fp);
	#endif

	#if 0
	FILE *fpp;
	fpp = fopen(AEM_Y_STAT_FILE,"w");
	for( j=0;j<32;j++)
	{
		for( i=0;i<32;i++)
		{
			fprintf(fpp,"%3d /%3d = %f  ",y_slave_stat_aem[32*j+i],y_master_stat_aem[32*j+i],((float)y_slave_stat_aem[32*j+i] / (float)y_master_stat_aem[32*j+i]) );
		}
		fprintf(fpp,"\n");
	}
	fprintf(fpp,"\n\n");
	fclose(fpp);
	#endif

}

	// get effective E&g
	cxt->cur_status.effect_expline = cxt->exp_data.actual_data.exp_line;
	cxt->cur_status.effect_dummy = cxt->exp_data.actual_data.dummy;
	cxt->cur_status.effect_gain = (cmr_s32) (1.0 * cxt->exp_data.actual_data.isp_gain * cxt->exp_data.actual_data.sensor_gain / 4096.0 + 0.5);

	backup_expline = cxt->cur_status.effect_expline;
	backup_gain = cxt->cur_status.effect_gain;
	backup_expgain = backup_expline*backup_gain;
	ISP_LOGE("ebd: ae_lib effect_expline %d, effect_gain %d", cxt->cur_status.effect_expline, cxt->cur_status.effect_gain);
	if(cxt->ebd_support){
		cxt->cur_status.effect_expline =  calc_in->ebd_info.exposure_valid ?calc_in->ebd_info.exposure : cxt->exp_data.actual_data.exp_line;
#if CONFIG_ISP_2_3
		cxt->cur_status.effect_gain = (cmr_u32)(1.0 *calc_in->ebd_info.gain + 0.5);
#else
		cxt->cur_status.effect_gain = (cmr_u32)(1.0 *calc_in->ebd_info.gain * calc_in->isp_dgain.global_gain/(4096.0 *cxt->ob_rgb_gain)+ 0.5);
#endif
	effect_ebd_expgain = cxt->cur_status.effect_expline * cxt->cur_status.effect_gain;
	ISP_LOGE("ebd: sensor effect_expline %d, effect_gain %d", cxt->cur_status.effect_expline, cxt->cur_status.effect_gain);
	}
#if CONFIG_ISP_2_3
	/*it will be enable lately */
	ae_set_soft_gain(cxt, cxt->sync_aem, cxt->sync_aem, cxt->cur_status.win_num, cxt->exp_data.actual_data.isp_gain);
#endif
	cxt->sync_aem[3 * 1024] = cxt->cur_status.frame_id;
	cxt->sync_aem[3 * 1024 + 1] = cxt->cur_status.effect_expline;
	cxt->sync_aem[3 * 1024 + 2] = cxt->cur_status.effect_dummy;
	cxt->sync_aem[3 * 1024 + 3] = cxt->cur_status.effect_gain;

	ISP_LOGV("exp: %d, gain:%d line time: %d\n", cxt->cur_status.settings.exp_line, cxt->cur_status.settings.gain, cxt->cur_status.line_time);

//START
	alg_status_ptr = &cxt->cur_status;
	cxt->cur_param = &cxt->tuning_param[0];
	//alg_status_ptr = &cxt->cur_status;
	// change weight_table
	alg_status_ptr->weight_table = cxt->cur_param->weight_table[alg_status_ptr->settings.metering_mode].weight;
	// change ae_table
#if 0
	current_status->ae_table = &cxt->cur_param->scene_info[current_status->settings.scene_mode].ae_table[current_status->settings.flicker];
#else
	// for now video using
	ISP_LOGV("AE_TABLE IS %p\n", alg_status_ptr->ae_table);
	//alg_status_ptr->ae_table = &cxt->cur_param->ae_table[alg_status_ptr->settings.flicker][AE_ISO_AUTO];
	alg_status_ptr->ae_table->min_index = 0;	//AE table start index = 0
#endif
	/*
	   due to set_scene_mode just be called in ae_sprd_calculation,
	   and the prv_status just save the normal scene status
	 */
	if (AE_SCENE_NORMAL == cxt->sync_cur_status.settings.scene_mode) {
		cxt->prv_status = cxt->cur_status;
		if(CAMERA_MODE_MANUAL == cxt->app_mode)
			cxt->manual_level = cxt->prv_status.settings.ev_index;
		if (AE_SCENE_NORMAL != cxt->cur_status.settings.scene_mode) {
			cxt->prv_status.settings.scene_mode = AE_SCENE_NORMAL;
		}
	}
	{
		cmr_s8 cur_mod = cxt->sync_cur_status.settings.scene_mode;
		cmr_s8 nx_mod = cxt->cur_status.settings.scene_mode;
		if ((nx_mod != cur_mod) || cxt->is_first) {
			ISP_LOGV("before set scene mode: \n");
			ae_printf_status_log(cxt, cur_mod, &cxt->cur_status);
			ae_set_scene_mode(cxt, cur_mod, nx_mod);
			ISP_LOGV("after set scene mode: \n");
			ae_printf_status_log(cxt, nx_mod, &cxt->cur_status);
		}
	}
	// END
	if (1 == cxt->mod_update_list.is_miso) {
		cxt->cur_status.settings.iso_manual_status = 1;
	} else {
		cxt->cur_status.settings.iso_manual_status = 0;
	}
	cxt->mod_update_list.is_miso = 0;

	if (1 == cxt->mod_update_list.is_mev) {
		cxt->cur_status.settings.ev_manual_status = 1;
	} else {
		cxt->cur_status.settings.ev_manual_status = 0;
	}

	if(cxt->ebd_support) {
		if((1.0 * backup_expgain / effect_ebd_expgain >= 0.97) && (1.0 * backup_expgain / effect_ebd_expgain <= 1.03))
			cxt->ebd_stable_flag = 1;
		else
			cxt->ebd_stable_flag = 0;
		ISP_LOGE("ebd: stable_flag %d", cxt->ebd_stable_flag);
	}
	rtn = ae_pre_process(cxt);
	flash_calibration_script((cmr_handle) cxt);	/*for flash calibration */
	ae_set_led(cxt);

	memcpy(&cxt->cur_status.hist_info, &calc_in->hist_stats, sizeof(struct ae_hist_info));

#ifdef CONFIG_SUPPROT_AUTO_HDR
	if(!cxt->is_snapshot) {
		hdr_stat.hist256 = calc_in->hist_stats.value;
		for (int i = 0; i < 256; i++)
			ISP_LOGV("auto_hdr hist[%d] %d", i, hdr_stat.hist256[i]);
		hdr_stat.w = cxt->snr_info.frame_size.w;
		hdr_stat.h = cxt->snr_info.frame_size.h;
		hdr_param.thres_bright = 250;
		hdr_param.thres_dark = 20;
		auto_hdr_enable = (cmr_s8)sprd_hdr_scndet(&hdr_param, &hdr_stat, ev_result);
		cxt->hdr_calc_result.ev[0] = fabs(ev_result[0]);
		cxt->hdr_calc_result.ev[1] = ev_result[1];
		ISP_LOGI("auto_hdr bright %d dark %d w %d h %d ev[0] %f ev[1] %f", hdr_param.thres_bright, hdr_param.thres_dark, hdr_stat.w, hdr_stat.h, ev_result[0], ev_result[1]);
	}
	if (cxt->hdr_menu_ctrl) {
		cxt->hdr_calc_result.auto_hdr_enable = auto_hdr_enable;
		if(-1 == auto_hdr_enable)
			auto_hdr_enable = 0;
		(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, AE_CB_HDR_STATUS, &auto_hdr_enable);
	}
#endif

	if (cxt->hdr_enable) {
		ae_set_hdr_ctrl(cxt, param);
	}

	{
		char prop[PROPERTY_VALUE_MAX];
		int val_max = 0;
		int val_min = 0;
		property_get("persist.vendor.cam.isp.ae.fps", prop, "0");
		if (atoi(prop) != 0) {
			val_min = atoi(prop) % 100;
			val_max = atoi(prop) / 100;
			cxt->cur_status.settings.min_fps = val_min > 5 ? val_min : 5;
			cxt->cur_status.settings.max_fps = val_max;
		}
	}
	pthread_mutex_lock(&cxt->data_sync_lock);
	current_status = &cxt->sync_cur_status;
	current_result = &cxt->sync_cur_result;
	memcpy(current_status, &cxt->cur_status, sizeof(struct ae_alg_calc_param));
	memcpy(&cxt->cur_result, current_result, sizeof(struct ae_alg_calc_result));
	pthread_mutex_unlock(&cxt->data_sync_lock);

/***********************************************************/
	if(cxt->is_multi_mode) {
		if(cxt->is_master == 0)
			current_status->cam_id = AE_SENSOR_SLAVE0;
		else if(cxt->is_master == 1)
			current_status->cam_id = AE_SENSOR_MASTER;
		else
			ISP_LOGE("Error !can't not found camera id!");
	} else
		current_status->cam_id = AE_SENSOR_SINGLE;
		
	
	current_status->ae_start_delay = 0;
	misc_calc_in.sync_settings = current_status;
	misc_calc_out.ae_output = &cxt->cur_result;
	ATRACE_BEGIN(__FUNCTION__);
	cmr_u64 ae_time0 = systemTime(CLOCK_MONOTONIC);
	if (0 == cxt->skip_update_param_flag) {
		ISP_LOGV("table_index %d,manual_mode:%d ", cxt->cur_status.settings.table_idx,cxt->cur_status.settings.manual_mode);
		rtn = ae_misc_calculation(cxt->misc_handle, &misc_calc_in, &misc_calc_out);
	}
	cmr_u64 ae_time1 = systemTime(CLOCK_MONOTONIC);
	ATRACE_END();
	ISP_LOGV("skip_update_param_flag: %d", cxt->skip_update_param_flag);
	ISP_LOGV("SYSTEM_TEST -ae_test	 %dus ", (cmr_s32)((ae_time1 - ae_time0) / 1000));
	if (cxt->cur_status.settings.exp_is_transmit ==1){
	        cxt->cur_status.settings.exp_is_transmit = 0;   //release transmit expsoure value to ae
	        if(cxt->cur_status.settings.manual_mode == 1){
	                cxt->cur_status.settings.manual_mode = -1;
	        }
	}

	if (rtn) {
		ISP_LOGE("fail to calc ae misc");
		rtn = AE_ERROR;
		goto ERROR_EXIT;
	}
	cxt->cur_status.ae1_finfo.update_flag = 0;	// add by match box for fd_ae reset the status
	memset((cmr_handle) & cxt->cur_status.ae1_finfo.cur_info, 0, sizeof(cxt->cur_status.ae1_finfo.cur_info));

	/*just for debug: reset the status */
	if (1 == cxt->cur_status.settings.touch_scrn_status) {
		cxt->cur_status.settings.touch_scrn_status = 0;
	}

	pthread_mutex_lock(&cxt->data_sync_lock);
	rtn = ae_post_process(cxt);
	rtn = ae_touch_ae_process(cxt, &cxt->cur_result);
	memcpy(current_result, &cxt->cur_result, sizeof(struct ae_alg_calc_result));
	pthread_mutex_unlock(&cxt->data_sync_lock);

/***********************************************************/
/*update parameters to sensor*/

#ifdef CONFIG_ISP_2_2
	struct match_data_param dualcam_aesync;
	float cur_fps = 0;

	if (cxt->is_multi_mode && cxt->is_master) {
		memcpy(&dualcam_aesync.master_ae_info.ae_calc_result, current_result, sizeof(dualcam_aesync.master_ae_info.ae_calc_result));
		ISP_LOGV("[master] cur_fps=%f, cur_lum=%d, gain=%u, expline=%u, exptime=%u, dummy=%u",
				 dualcam_aesync.master_ae_info.ae_calc_result.wts.cur_fps,
				 dualcam_aesync.master_ae_info.ae_calc_result.cur_lum,
				 dualcam_aesync.master_ae_info.ae_calc_result.wts.cur_again,
				 dualcam_aesync.master_ae_info.ae_calc_result.wts.cur_exp_line,
				 dualcam_aesync.master_ae_info.ae_calc_result.wts.exposure_time, dualcam_aesync.master_ae_info.ae_calc_result.wts.cur_dummy);
		/* store calculated master's ae value */
		rtn = cxt->ptr_isp_br_ioctrl(cxt->is_master ? CAM_SENSOR_MASTER : CAM_SENSOR_SLAVE0, SET_MASTER_AECALC_RESULT, current_result, NULL);

		/* use stored master's ae value to calculate slave's ae value */
		rtn = ae_dual_cam_sync_calc(cxt, &dualcam_aesync);

	} else if (cxt->is_multi_mode && !cxt->is_master) {
		/* use slave's ae_sync_result as final ae value */
		rtn = cxt->ptr_isp_br_ioctrl(cxt->is_master ? CAM_SENSOR_MASTER : CAM_SENSOR_SLAVE0, GET_SLAVE_AESYNC_SETTING, NULL, &dualcam_aesync.slave_ae_info.ae_sync_result);

		current_result->wts.cur_exp_line = dualcam_aesync.slave_ae_info.ae_sync_result.slave_ae.wts.cur_exp_line;
		current_result->wts.exposure_time = dualcam_aesync.slave_ae_info.ae_sync_result.slave_ae.wts.exposure_time;
		current_result->wts.cur_again = dualcam_aesync.slave_ae_info.ae_sync_result.slave_ae.wts.cur_again;
		current_result->wts.cur_dummy = dualcam_aesync.slave_ae_info.ae_sync_result.slave_ae.wts.cur_dummy;

		cur_fps = (float)(AEC_LINETIME_PRECESION / ((current_result->wts.cur_exp_line + current_result->wts.cur_dummy) * current_status->line_time));

		ISP_LOGV("[slave ] cur_fps=%f, cur_lum=%d, gain=%u, expline=%u, exptime=%u, dummy=%u",
				 cur_fps,
				 current_result->cur_lum,
				 dualcam_aesync.slave_ae_info.ae_sync_result.slave_ae.wts.cur_again,
				 dualcam_aesync.slave_ae_info.ae_sync_result.slave_ae.wts.cur_exp_line,
				 dualcam_aesync.slave_ae_info.ae_sync_result.slave_ae.wts.exposure_time, dualcam_aesync.slave_ae_info.ae_sync_result.slave_ae.wts.cur_dummy);
	}
#else
	if (cxt->is_multi_mode == ISP_ALG_DUAL_SBS && (!cxt->is_master)) {
		cmr_s16 bv;
		cxt->ptr_isp_br_ioctrl(cxt->is_master ? CAM_SENSOR_MASTER : CAM_SENSOR_SLAVE0, GET_MATCH_BV_DATA, NULL, &bv);
		current_result->cur_bv = bv;

		struct ae_match_data ae_match_data_slave;
		cxt->ptr_isp_br_ioctrl(cxt->is_master ? CAM_SENSOR_MASTER : CAM_SENSOR_SLAVE0, GET_MATCH_AE_DATA, NULL, &ae_match_data_slave);

		current_result->wts.cur_again = ae_match_data_slave.gain * ae_match_data_slave.isp_gain / 4096;
		current_result->wts.cur_exp_line = ae_match_data_slave.exp.exposure;
		current_result->wts.exposure_time = current_result->wts.cur_exp_line * current_status->line_time;

		cxt->cur_result.cur_bv = current_result->cur_bv;
		cxt->cur_result.wts.cur_again = current_result->wts.cur_again;
		cxt->cur_result.wts.cur_exp_line = current_result->wts.cur_exp_line;
		cxt->cur_result.wts.exposure_time = current_result->wts.exposure_time;
		ISP_LOGV("cur_bv %d cur_again %d cur_exp_line %d exposure_time %d", current_result->cur_bv, current_result->wts.cur_again, current_result->wts.cur_exp_line, current_result->wts.exposure_time);
	}
	else if ((cxt->is_multi_mode == ISP_ALG_DUAL_C_C||cxt->is_multi_mode ==ISP_ALG_DUAL_W_T||cxt->is_multi_mode ==ISP_ALG_DUAL_C_M)&& (!cxt->is_master)) {
		cmr_s16 bv;
		cxt->ptr_isp_br_ioctrl(cxt->is_master ? CAM_SENSOR_MASTER : CAM_SENSOR_SLAVE0, GET_MATCH_BV_DATA, NULL, &bv);
		current_result->cur_bv = bv;

		struct ae_match_data ae_match_data_slave;
		cxt->ptr_isp_br_ioctrl(cxt->is_master ? CAM_SENSOR_MASTER : CAM_SENSOR_SLAVE0, GET_MATCH_AE_DATA, NULL, &ae_match_data_slave);

		current_result->wts.cur_again = ae_match_data_slave.gain * ae_match_data_slave.isp_gain / 4096;
		current_result->wts.cur_exp_line = ae_match_data_slave.exp.exposure;
		current_result->wts.exposure_time = current_result->wts.cur_exp_line * current_status->line_time;

		cxt->cur_result.cur_bv = current_result->cur_bv;
		cxt->cur_result.wts.cur_again = current_result->wts.cur_again;
		cxt->cur_result.wts.cur_exp_line = current_result->wts.cur_exp_line;
		cxt->cur_result.wts.exposure_time = current_result->wts.exposure_time;
		ISP_LOGV("(sharkl3)normal mode:[slave]cur_bv %d cur_again %d cur_exp_line %d exposure_time %d", current_result->cur_bv, current_result->wts.cur_again, current_result->wts.cur_exp_line, current_result->wts.exposure_time);
	}

#endif

	memcpy(&cur_calc_result->ae_result, current_result, sizeof(struct ae_alg_calc_result));
	ae_make_calc_result(cxt, current_result, cur_calc_result);

/*update parameters to sensor*/
	cxt->exp_data.lib_data.exp_line = current_result->wts.cur_exp_line;
	cxt->exp_data.lib_data.exp_time = current_result->wts.exposure_time;
	cxt->exp_data.lib_data.gain = current_result->wts.cur_again;
	cxt->exp_data.lib_data.dummy = current_result->wts.cur_dummy;
	cxt->exp_data.lib_data.line_time = current_status->line_time;

	if (cxt->has_mf) {
		if (cxt->has_mf_cnt==3) {
			cxt->has_mf = cxt->has_mf_cnt = 0;
			cb_type = AE_CB_CONVERGED;
			stop_skip_en = 1;
			(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, cb_type, &stop_skip_en);
			ISP_LOGI("skip_num stop \r\n");
		} else {
			ISP_LOGI("skip_num prepare\r\n");
			cxt->has_mf_cnt++;
		}
	}

	rtn = ae_update_result_to_sensor(cxt, &cxt->exp_data, 0);

#ifdef CONFIG_ISP_2_2
	if (cxt->is_multi_mode) {
		ISP_LOGV("notify ae info, camera id=%d", cxt->camera_id);
		cb_type = AE_CB_AE_CALCOUT_NOTIFY;
		(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, cb_type, &cur_calc_result->ae_output);
	}
#endif

	rtn = ae_touch_ae_process(cxt, current_result);
/* send STAB notify to HAL */

	if (cxt->isp_ops.callback) {
		cb_type = AE_CB_STAB_NOTIFY;
		(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, cb_type, &cur_calc_result->ae_output.is_stab);
	}

/***********************************************************/
/*display the AE running status*/
	if (1 == cxt->debug_enable) {
		ae_save_to_mlog_file(cxt, &misc_calc_out);
	}
/***********************************************************/
	cxt->cur_status.frame_id++;
	if (cxt->isp_ops.callback) {
		ae_make_isp_result(cxt, current_result, &callback_in);
		(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, AE_CB_PROCESS_OUT, &callback_in);
	}
	cxt->is_first = 0;
  ERROR_EXIT:
	return rtn;
}

cmr_s32 ae_sprd_calculation(cmr_handle handle, cmr_handle param, cmr_handle result)
{
	cmr_s32 rtn = AE_SUCCESS;
	struct ae_ctrl_cxt *cxt = (struct ae_ctrl_cxt *)handle;
	struct ae_calc_in *calc_in = (struct ae_calc_in *)param;

	ISP_LOGV("is_update %d", calc_in->is_update);
	if (cxt->high_fps_info.is_high_fps) {
		if ((calc_in->is_update) && (cxt->slw_prev_skip_num >= cxt->cur_param->enter_skip_num)) {
			rtn = ae_calculation_slow_motion(handle, param, result);
		} else {
			cxt->exp_data.lib_data.exp_line = cxt->sync_cur_result.wts.cur_exp_line;
			cxt->exp_data.lib_data.exp_time = cxt->sync_cur_result.wts.exposure_time;
			cxt->exp_data.lib_data.gain = cxt->sync_cur_result.wts.cur_again;
			cxt->exp_data.lib_data.dummy = cxt->sync_cur_result.wts.cur_dummy;
			cxt->exp_data.lib_data.line_time = cxt->cur_status.line_time;
			rtn = ae_update_result_to_sensor(cxt, &cxt->exp_data, 0);
		}
		if (calc_in->is_update) {
			cxt->slw_prev_skip_num++;
		}
	} else {
		if (calc_in->is_update)
			rtn = ae_calculation(handle, param, result);
	}

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

	case AE_GET_STAB:
		if (result) {
			*(cmr_u32 *) result = cxt->cur_result.wts.stable;
		}
		break;

	case AE_GET_FLASH_ENV_RATIO:
		rtn = ae_get_flash_env_ratio(cxt, result);
		break;

	case AE_GET_FLASH_ONE_OF_ALL_RATIO:
		rtn = ae_get_flash_one_in_all_ratio(cxt, result);
		break;

	case AE_GET_EV:
		rtn = ae_get_ev(cxt, result);
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
			*(cmr_u32 *) result = cxt->cur_result.wts.exposure_time / 100;
		}
		break;

	case AE_GET_SENSITIVITY:
		if (param) {
			*(cmr_u32 *) param = cxt->cur_result.wts.cur_again * 50 / 128;
		}
		break;

	case AE_GET_DEBUG_INFO:
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

	case AE_GET_LOWLIGHT_FLAG_BY_BV:
		rtn = ae_get_lowlight_flag_by_bv(cxt, (cmr_u32 *) result);
		break;
	case AE_GET_GLB_GAIN:
		if (result) {
			*(cmr_u32 *) result = cxt->glb_gain;
		}
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
		rtn = ae_set_pause(cxt);
		break;

	case AE_SET_RESTORE:
		rtn = ae_set_restore_cnt(cxt);
		break;

	case AE_SET_FORCE_PAUSE:
		cxt->expchanged = 0;
		rtn = ae_set_force_pause(cxt, 1);
		cxt->exposure_compensation.ae_compensation_flag = 1;
		ISP_LOGD("AE_SET_FORCE_PAUSE");
		break;

	case AE_SET_FORCE_RESTORE:
		rtn = ae_set_force_pause(cxt, 0);
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

	case AE_SET_TARGET_LUM:
		rtn = ae_set_target_luma(cxt, param);
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

	case AE_CAF_LOCKAE_START:
		rtn = ae_set_caf_lockae_start(cxt);
		break;

	case AE_CAF_LOCKAE_STOP:
		rtn = ae_set_caf_lockae_stop(cxt);
		break;

	case AE_SET_RGB_GAIN:
#ifndef CONFIG_ISP_2_2
		rtn = ae_set_isp_gain(cxt);
#endif
		break;

	case AE_SET_UPDATE_AUX_SENSOR:
		rtn = ae_set_aux_sensor(cxt, param, result);
		break;

	case AE_SET_EXPOSURE_COMPENSATION:
		rtn = ae_set_exposure_compensation(cxt, param);
		break;

	case AE_SET_CAP_FLAG:
		cxt->cam_4in1_cap_flag = *(cmr_u32 *)param;
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

	default:
		rtn = AE_ERROR;
		break;
	}
	return rtn;
}

cmr_s32 ae_sprd_io_ctrl(cmr_handle handle, cmr_s32 cmd, cmr_handle param, cmr_handle result)
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

cmr_s32 ae_sprd_deinit(cmr_handle handle, cmr_handle in_param, cmr_handle out_param)
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
	if (0 != rtn) {
		ISP_LOGE("fail to deinit flash, rtn: %d\n", rtn);
		rtn = AE_ERROR;
	} else {
		cxt->flash_alg_handle = NULL;
	}

	rtn = ae_misc_deinit(cxt->misc_handle, NULL, NULL);
	if(cxt->cur_status.base_img)
		free(cxt->cur_status.base_img);
	cxt->cur_status.base_img = NULL;

	if (AE_SUCCESS != rtn) {
		ISP_LOGE("fail to deinit ae misc ");
		rtn = AE_ERROR;
	}
	//ae_seq_reset(cxt->seq_handle);
	rtn = s_q_close(cxt->seq_handle);

	if (cxt->debug_enable) {
		if (cxt->debug_info_handle) {
			debug_file_deinit((debug_handle_t) cxt->debug_info_handle);
			cxt->debug_info_handle = (cmr_handle) NULL;
		}
	}
#ifdef CONFIG_ISP_2_2
	enum sync_status ae_sync_status = SYNC_DEINIT;
	if (cxt->is_multi_mode && cxt->is_master) {
		rtn = cxt->ptr_isp_br_ioctrl(cxt->is_master ? CAM_SENSOR_MASTER : CAM_SENSOR_SLAVE0, SET_MASTER_AE_SYNC_STATUS, &ae_sync_status, NULL);
	} else if (cxt->is_multi_mode && !cxt->is_master) {
		rtn = cxt->ptr_isp_br_ioctrl(cxt->is_master ? CAM_SENSOR_MASTER : CAM_SENSOR_SLAVE0, SET_SLAVE_AE_SYNC_STATUS, &ae_sync_status, NULL);

	}
#endif
	pthread_mutex_destroy(&cxt->data_sync_lock);
	ISP_LOGI("cam-id %d", cxt->camera_id);
	free(cxt);
	ISP_LOGI("done");

	return rtn;
}

cmr_handle ae_sprd_init(cmr_handle param, cmr_handle in_param)
{
	cmr_s32 rtn = AE_SUCCESS;
	char ae_property[PROPERTY_VALUE_MAX];
	struct ae_ctrl_cxt *cxt = NULL;
	struct ae_init_out *ae_init_out = NULL;
	struct ae_misc_init_in misc_init_in = { 0, 0, 0, NULL, 0 };
	struct ae_misc_init_out misc_init_out = { 0, {0} };
	struct s_q_open_param s_q_param = { 0, 0, 0 };
	struct ae_set_work_param work_param;
	struct ae_init_in *init_param = NULL;
	struct Flash_initInput flash_in;
	struct Flash_initOut flash_out;

#ifdef CONFIG_SUPPROT_AUTO_HDR
	struct _tag_hdr_version_t hdr_version;
	sprd_hdr_version(&hdr_version);
	ISP_LOGV("HDR_version :%s", hdr_version.built_rev);
#endif
	cxt = (struct ae_ctrl_cxt *)malloc(sizeof(struct ae_ctrl_cxt));

	if (NULL == cxt) {
		rtn = AE_ALLOC_ERROR;
		ISP_LOGE("fail to malloc");
		goto ERR_EXIT;
	}
	memset(&work_param, 0, sizeof(work_param));
	memset(cxt, 0, sizeof(*cxt));

	if (NULL == param) {
		ISP_LOGE("fail to get input param %p\r\n", param);
		rtn = AE_ERROR;
		goto ERR_EXIT;
	}
	init_param = (struct ae_init_in *)param;
	ae_init_out = (struct ae_init_out *)in_param;
	cxt->backup_rgb_gain = init_param->bakup_rgb_gain;
	cxt->ob_rgb_gain = 1.0 * cxt->backup_rgb_gain / 4096;
	ISP_LOGV("bakup_rgb_gain %d, ob_rgb_gain %f", cxt->backup_rgb_gain, cxt->ob_rgb_gain);

	cxt->ptr_isp_br_ioctrl = init_param->ptr_isp_br_ioctrl;

	cxt->is_master = init_param->is_master;
	cxt->is_multi_mode = init_param->is_multi_mode;
	cxt->ebd_support = init_param->ebd_support;
	cxt->bv_thd = init_param->bv_thd;
	cxt->cur_status.base_img = (cmr_u32 *)malloc(3*16384*sizeof(cmr_u32));
	if(NULL == cxt->cur_status.base_img){
                rtn = AE_ALLOC_ERROR;
                ISP_LOGE("fail to malloc");
                goto ERR_EXIT;
	}
	ISP_LOGI("is_multi_mode=%d\n", init_param->is_multi_mode);

	// parser ae otp info
	ae_parser_otp_info(init_param);

	work_param.mode = AE_WORK_MODE_COMMON;
	work_param.fly_eb = 1;
	work_param.resolution_info = init_param->resolution_info;

	rtn = ae_set_ae_param(cxt, init_param, &work_param, AE_PARAM_INIT);

        cxt->target_lum_zone_bak = cxt->cur_status.target_lum_zone;

	s_q_param.exp_valid_num = cxt->exp_skip_num + 1 + AE_UPDAET_BASE_OFFSET;
	s_q_param.sensor_gain_valid_num = cxt->gain_skip_num + 1 + AE_UPDAET_BASE_OFFSET;
	s_q_param.isp_gain_valid_num = 1 + AE_UPDAET_BASE_OFFSET;
	cxt->seq_handle = s_q_open(&s_q_param);
	/*Read tuning param from tuning param */

	/*start read front or rear flash tuning param */
	if (1 == cxt->camera_id) {
		if (0 != cxt->cur_param->flash_swith_param.flash_open_thr)
			cxt->flash_swith.led_thr_down = cxt->cur_param->flash_swith_param.flash_open_thr;
		else
			cxt->flash_swith.led_thr_down = 250;
		if (0 != cxt->cur_param->flash_swith_param.flash_close_thr)
			cxt->flash_swith.led_thr_up = cxt->cur_param->flash_swith_param.flash_close_thr;
		else
			cxt->flash_swith.led_thr_up = 500;
	}
	if (0 == cxt->camera_id) {
		if (0 != cxt->cur_param->flash_swith_param.flash_open_thr)
			cxt->flash_swith.led_thr_down = cxt->cur_param->flash_swith_param.flash_open_thr;
		else
			cxt->flash_swith.led_thr_down = 380;
		if (0 != cxt->cur_param->flash_swith_param.flash_close_thr)
			cxt->flash_swith.led_thr_up = cxt->cur_param->flash_swith_param.flash_close_thr;
		else
			cxt->flash_swith.led_thr_up = 480;
	}
	/*end to read front or rear flash tuning param */

	/*start read set video fps tuning param */
	if (0 == cxt->cur_param->ae_video_fps.ae_video_fps_thr_high)
		cxt->cur_param->ae_video_fps.ae_video_fps_thr_high = 1000;
	if (0 == cxt->cur_param->ae_video_fps.ae_video_fps_thr_low)
		cxt->cur_param->ae_video_fps.ae_video_fps_thr_low = -300;
	/*end to set video fps tuning param */

	/*start read flash control tuning param */
	if (0 == cxt->cur_param->flash_control_param.pre_flash_skip)
		cxt->cur_param->flash_control_param.pre_flash_skip = 3;
	if (0 == cxt->cur_param->flash_control_param.aem_effect_delay)
		cxt->cur_param->flash_control_param.aem_effect_delay = 2;
	if (0 == cxt->cur_param->flash_control_param.pre_open_count)
		cxt->cur_param->flash_control_param.pre_open_count = 3;
	if (0 == cxt->cur_param->flash_control_param.main_set_count)
		cxt->cur_param->flash_control_param.main_set_count = 1;/*the mini value of main set count can be 0 */
	if (0 == cxt->cur_param->flash_control_param.main_capture_count)
		cxt->cur_param->flash_control_param.main_capture_count = 5;
	if (0 == cxt->cur_param->flash_control_param.main_flash_notify_delay)
		cxt->cur_param->flash_control_param.main_flash_notify_delay = 6;
	/*end to read flash control tuning param */

	/* HJW_S: dual flash algorithm init */
	flash_in.debug_level = 1;	/*it will be removed in the future, and get it from dual flash tuning parameters */
	flash_in.tune_info = &cxt->dflash_param[0];
	flash_in.statH = 32;
	flash_in.statW = 32;
	cxt->flash_alg_handle = flash_init(&flash_in, &flash_out);
	flash_out.version = 1;		//remove later
	cxt->flash_ver = flash_out.version;
	ae_init_out->flash_ver = cxt->flash_ver;
	/*HJW_E */

	/*jhin add flash mode*/
	cxt->cur_status.flash_mode = 0;
	/*jhin add touch ev to reset */
	cxt->exposure_compensation.touch_ev_flag = 0;
	cxt->last_table_index = 0;
	cxt->bypass = init_param->has_force_bypass;
	if (init_param->has_force_bypass) {
		cxt->cur_param->touch_info.enable = 0;
		cxt->cur_param->face_param.face_tuning_enable = 0;
	}
	cxt->debug_enable = ae_is_mlog(cxt);
	cxt->cur_status.mlog_en = cxt->debug_enable;
	misc_init_in.alg_id = cxt->cur_status.alg_id;
	misc_init_in.flash_version = cxt->flash_ver;
	misc_init_in.start_index = cxt->cur_status.start_index;
	misc_init_in.param_ptr = &cxt->cur_status;
	misc_init_in.size = sizeof(cxt->cur_status);
	cxt->misc_handle = ae_misc_init(&misc_init_in, &misc_init_out);
	memcpy(cxt->alg_id, misc_init_out.alg_id, sizeof(cxt->alg_id));

	pthread_mutex_init(&cxt->data_sync_lock, NULL);

#ifndef CONFIG_ISP_2_2
	//get ae_sync_param
	if((init_param->is_master == 1) && (init_param->is_multi_mode)) {//dual camera && master sensor
	memcpy(&cxt->ae_sync_param, init_param->ae_sync_param.param, init_param->ae_sync_param.size);
	ISP_LOGI(" magic num:0x%x,size:%d.\n",cxt->ae_sync_param.magic_first_num,init_param->ae_sync_param.size);
	}
#endif

	/* set sensor exp/gain validate information */
	{
		if (cxt->isp_ops.set_shutter_gain_delay_info) {
			struct ae_exp_gain_delay_info delay_info = { 0, 0, 0 };
			delay_info.group_hold_flag = 0;
			delay_info.valid_exp_num = cxt->cur_param->sensor_cfg.exp_skip_num;
			delay_info.valid_gain_num = cxt->cur_param->sensor_cfg.gain_skip_num;
			cxt->isp_ops.set_shutter_gain_delay_info(cxt->isp_ops.isp_handler, (cmr_handle) (&delay_info));
		} else {
			ISP_LOGE("fail to set_shutter_gain_delay_info\n");
		}
	}

	/* update the sensor related information to cur_result structure */
	cxt->cur_result.wts.stable = 0;
	cxt->cur_result.wts.cur_dummy = 0;
	cxt->cur_result.wts.cur_index = cxt->cur_status.start_index;
	cxt->cur_result.wts.cur_again = cxt->cur_status.ae_table->again[cxt->cur_status.start_index];
	cxt->cur_result.wts.cur_dgain = 0;
	cxt->cur_result.wts.cur_exp_line = cxt->cur_status.ae_table->exposure[cxt->cur_status.start_index];
	cxt->cur_result.wts.exposure_time = cxt->cur_status.ae_table->exposure[cxt->cur_status.start_index] * cxt->snr_info.line_time;

	memset((cmr_handle) & ae_property, 0, sizeof(ae_property));
	property_get("persist.vendor.cam.isp.ae.manual", ae_property, "off");
	//ISP_LOGV("persist.vendor.cam.isp.ae.manual: %s", ae_property);
	if (!strcmp("on", ae_property)) {
		cxt->manual_ae_on = 1;
	} else {
		cxt->manual_ae_on = 0;
	}
	ISP_LOGI("done, cam-id %d, flash ver %d", cxt->camera_id, cxt->flash_ver);
	return (cmr_handle) cxt;

  ERR_EXIT:
	if (NULL != cxt) {
		free(cxt);
		cxt = NULL;
	}
	ISP_LOGE("fail to init ae %d", rtn);
	return NULL;
}

struct adpt_ops_type ae_sprd_adpt_ops_ver0 = {
	.adpt_init = ae_sprd_init,
	.adpt_deinit = ae_sprd_deinit,
	.adpt_process = ae_sprd_calculation,
	.adpt_ioctrl = ae_sprd_io_ctrl,
};
