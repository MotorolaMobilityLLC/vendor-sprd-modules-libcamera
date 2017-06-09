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

#include "ae_sprd_adpt.h"
#include "ae_misc.h"
#include "ae_debug.h"
#include "ae_ctrl.h"
#include "flash.h"
#include "isp_debug.h"
#include "awb.h"
#ifdef CONFIG_CAMERA_DUAL_SYNC
#include "isp_match.h"
#endif
#ifndef WIN32
#include <utils/Timers.h>
#include <cutils/properties.h>
#include <math.h>
#include <string.h>
#else
#include "stdio.h"
#include "ae_porint.h"
#endif

#include "cmr_msg.h"
#include "sensor_exposure_queue.h"
#include "isp_adpt.h"
#include "ae_debug_info_parser.h"

#define AE_UPDATE_BASE_EOF 0
#define AE_UPDATE_BASE_SOF 1
#define AE_UPDAET_BASE_OFFSET AE_UPDATE_BASE_SOF
#define AE_TUNING_VER 1

#define AE_START_ID 0x71717567
#define AE_END_ID 	0x69656E64

#define AE_SAVE_MLOG     "persist.sys.isp.ae.mlog"
#define AE_SAVE_MLOG_DEFAULT ""
#define SENSOR_EXP_NS_BASE     1000000000	/*unit:ns */
#define SENSOR_LINETIME_BASE   100	/*uint:0.1us */
#define AE_VIDEO_DECR_FPS_DARK_ENV_THRD 100 /*lower than LV1, if it is 0, disable this feature*/
/*
 * should be read from driver later
 */
#define AE_FLASH_ON_OFF_THR 380
#define AE_FLASH_CALC_TIMES	15	/* prevent flash_pfOneIteration time out */
#define AE_THREAD_QUEUE_NUM		(50)
const char AE_MAGIC_TAG[] = "ae_debug_info";

/*
 * for flash calibration
*/
struct flash_calibration_data
{
	uint32 version;
	int32 error;
	uint8 flashLevelNum1;
	uint8 flashLevelNum2;
	bool flashMask[1024];
	uint16 brightnessTable[1024];
	uint16 rTable[1024]; //g: 1024
	uint16 bTable[1024];
	uint16 preflashBrightness[1024];
	uint16 preflashCt[1024];
};

enum FlashCaliError
{
	FlashCali_too_close = -10000,
	FlashCali_too_far,
};

enum FlashCaliState
{
	FlashCali_start,
	FlashCali_ae,
	FlashCali_cali,
	FlashCali_end,
	FlashCali_none,
};

struct FCData
{
	float rBuf[1024];
	float gBuf[1024];
	float bBuf[1024];
	float expTime;
	int gain;
	float expTimeBase;
	int gainBase;

	int isDual;
	int maxCurrent1;
	int maxCurrent2;
	int maxCurrentAll;

	int testIndAll;
	int testInd;
	int isMainTab[128];
	int ind1Tab[128];
	int ind2Tab[128];
	int testMinFrm[128];
	int expReset[128];

	float expTab[128];
	int gainTab[128];

	float rData[128];
	float gData[128];
	float bData[128];

	int stateAeFrameCntSt;
	int stateCaliFrameCntSt2;
	int stateCaliFrameCntStSub;

	float rFrame[128][15];
	float gFrame[128][15];
	float bFrame[128][15];
	int frame;

	struct flash_calibration_data out;

};

struct ae_exposure_param {
	cmr_u32 cur_index;
	cmr_u32 line_time;
	cmr_u32 exp_line;
	cmr_u32 exp_time;
	cmr_u32 dummy;
	cmr_u32 gain;/*gain = sensor_gain * isp_gain*/
	cmr_u32 sensor_gain;
	cmr_u32 isp_gain;
};
#ifdef CONFIG_CAMERA_DUAL_SYNC
#include <sprd_sensor_k.h>
typedef enum { FALSE, TRUE } BOOL;
#endif

struct ae_sensor_exp_data {
	struct ae_exposure_param lib_data;/*AE lib output data*/
	struct ae_exposure_param actual_data;/*the actual effect data*/
	struct ae_exposure_param write_data;/*write to sensor data in current time*/
};

struct touch_zone_param {
	cmr_u32 enable;
	struct ae_weight_table weight_table;
	struct touch_zone zone_param;
};

enum initialization {
	AE_PARAM_NON_INIT,
	AE_PARAM_INIT
};

struct flash_cali_data {
	cmr_u16 ydata;		//1024
	cmr_u16 rdata;		// base on gdata1024
	cmr_u16 bdata;		// base on gdata1024
	cmr_s8 used;
};

struct ae_monitor_unit {
	cmr_u32 mode;
	struct ae_size win_num;
	struct ae_size win_size;
	struct ae_trim trim;
	struct ae_monitor_cfg cfg;
	cmr_u32 is_stop_monitor;
};

/**************************************************************************/
/*
* BEGIN: FDAE related definitions
*/
struct ae_fd_info {
	cmr_s8 enable;
	cmr_s8 pause;
	cmr_s8 allow_to_work;/* When allow_to_work==0, FDAE
				 * will never work */
	struct ae_fd_param face_info;	/* The current face information */
};
/*
* END: FDAE related definitions
*/
/**************************************************************************/
/*
* ae handler for isp_app
*/
struct ae_ctrl_cxt {
	cmr_u32 start_id;
	char alg_id[32];
	cmr_u32 checksum;
	cmr_u32 bypass;
	/*
	 * camera id: front camera or rear camera
	 */
	cmr_s8 camera_id;
	cmr_s8 is_snapshot;
	pthread_mutex_t data_sync_lock;
	/*
	 * ae control operation infaces
	 */
	struct ae_isp_ctrl_ops isp_ops;
	/*
	 * ae stat monitor config
	 */
	struct ae_monitor_unit monitor_unit;
	/*
	 * ae slow motion info
	 */
	struct isp_sensor_fps_info high_fps_info;
 	/*
	* ae fps range
	*/
	struct ae_range fps_range;
	/*
	* current flicker flag
	*/
	cmr_u32 cur_flicker;
	/*
	 * for ae tuning parameters
	 */
	struct ae_tuning_param tuning_param[AE_MAX_PARAM_NUM];
	cmr_s8 tuning_param_enable[AE_MAX_PARAM_NUM];
	struct ae_tuning_param *cur_param;
	struct ae_exp_gain_table back_scene_mode_ae_table[AE_SCENE_MAX][AE_FLICKER_NUM];
	/*
	 * sensor related information
	 */
	struct ae_resolution_info snr_info;
	/*
	 * ae current status: include some tuning
	 * param/calculatioin result and so on
	 */
	struct ae_alg_calc_param prv_status;	/*just backup the alg status of normal scene,
						   as switch from special scene mode to normal,
						   and we use is to recover the algorithm status
						 */
	struct ae_alg_calc_param cur_status;
	struct ae_alg_calc_param sync_cur_status;
	cmr_s32 target_lum_zone_bak;
	cmr_u32 sync_aem[3 * 1024 + 4];	/*0: frame id;1: exposure time, 2: dummy line, 3: gain; */
	/*
	 * convergence & stable zone
	 */
	cmr_s8 stable_zone_ev[16];
	cmr_u8 cnvg_stride_ev_num;
	cmr_s16 cnvg_stride_ev[32];
	/*
	 * Touch ae param
	 */
	struct touch_zone_param touch_zone_param;
	/*
	 * flash ae param
	 */
	 /*ST: for dual flash algorithm*/
	cmr_u8 flash_ver;
	cmr_s32 pre_flash_skip;
	cmr_s32 aem_effect_delay;
	struct ae_leds_ctrl ae_leds_ctrl;
	cmr_handle flash_alg_handle;
	cmr_u16 aem_stat_rgb[3 * 1024];/*save the average data of AEM Stats data*/
	cmr_u8 bakup_ae_status_for_flash;/* 0:unlock 1:lock 2:pause 3:wait-lock */
	cmr_u8 pre_flash_level1;
	cmr_u8 pre_flash_level2;
	struct Flash_pfOneIterationInput flash_esti_input;
	struct Flash_pfOneIterationOutput flash_esti_result;
	cmr_s32 flash_last_exp_line;
	cmr_s32 flash_last_gain;
	float ctTabRg[20];
	float ctTab[20];
	/*ED: for dual flash algorithm*/
	cmr_s16 flash_on_off_thr;
	cmr_u32 flash_effect;
	struct flash_cali_data flash_cali[32][32];
	cmr_u8 flash_debug_buf[256 * 1024];
	cmr_u32 flash_buf_len;
	/*
	 * fd-ae param
	 */
	struct ae_fd_info fdae;
	/*
	 * control information for sensor update
	 */
	struct ae_alg_calc_result cur_result;
	struct ae_alg_calc_result sync_cur_result;
	/*
	 * AE write/effective E&G queue
	 */
	cmr_handle seq_handle;
	cmr_s8 exp_skip_num;
	cmr_s8 gain_skip_num;
	cmr_s16 sensor_gain_precision;
	cmr_u16 sensor_max_gain;/*the max gain that sensor can be used*/
	cmr_u16 sensor_min_gain;/*the mini gain that sensor can be used*/
	cmr_u16 min_exp_line;/*the mini exposure line that sensor can be used*/
	struct ae_sensor_exp_data exp_data;
	/*
	 * recording when video stop
	 */
	struct ae_exposure_param last_exp_param;
	cmr_s32 last_index;
	cmr_s32 last_enable;
	/*
	 * just for debug information
	 */
	cmr_u8 debug_enable;
	char debug_file_name[256];
	cmr_uint debug_info_handle;
	cmr_u32 debug_str[512];
	cmr_u8 debug_info_buf[512 * 1024];
	//struct debug_ae_param debug_buf;
	/*
	 * for manual ae stat
	 */
	cmr_u8 manual_ae_on;
	/*
	 * flash_callback control
	 */
	cmr_s8 send_once[4];
	/*
	 * HDR control
	 */
	cmr_s8	hdr_enable;
	cmr_s8	hdr_frame_cnt;
	cmr_s8	hdr_cb_cnt;
	cmr_s8	hdr_flag;
	cmr_s16 hdr_up;
	cmr_s16 hdr_down;
	cmr_s16 hdr_base_ae_idx;
	/*
	 *dual flash simulation
	 */
	cmr_s8	led_record[2];
	/*
	 * ae misc layer handle
	 */
	cmr_handle misc_handle;
	cmr_u32 end_id;
	/*
	 * for dual camera sync
	 */
#ifdef  CONFIG_CAMERA_DUAL_SYNC
	cmr_u8 ae_role;
	cmr_u8 sensor_role;
	cmr_u32 is_multi_mode;
	func_isp_br_ioctrl ptr_isp_br_ioctrl;
	
//	struct ae_calc_result pre_write_exp_data_slv;
	struct ae_exposure_param pre_write_exp_data_slv;
//	struct ae_calc_result pre_write_exp_data;
	struct ae_exposure_param pre_write_exp_data;
	struct match_data_param dualcam_aesync_param;
#endif
};

#define AE_PRINT_TIME \
	do {                                                       \
                    nsecs_t timestamp = systemTime(CLOCK_MONOTONIC);   \
                    ISP_LOGV("timestamp = %lld.", timestamp/1000000);  \
	} while(0)

#ifndef MAX
#define  MAX( _x, _y ) ( ((_x) > (_y)) ? (_x) : (_y) )
#define  MIN( _x, _y ) ( ((_x) < (_y)) ? (_x) : (_y) )
#endif

static float _get_real_gain(cmr_u32 gain);
//static cmr_s32 _fdae_init(struct ae_ctrl_cxt *cxt);
static cmr_s32 _set_pause(struct ae_ctrl_cxt *cxt);
static cmr_s32 _set_restore_cnt(struct ae_ctrl_cxt *cxt);

/**---------------------------------------------------------------------------*
** 				Local Function Prototypes				*
**---------------------------------------------------------------------------*/

static cmr_s32 ae_info_print(struct ae_ctrl_cxt *cxt)
{
	cmr_s32 rtn = AE_SUCCESS;
	if (debug_print_enable()) {
		ISP_LOGV("cam_id %d, alg id: %d\n", cxt->camera_id, cxt->cur_status.alg_id);

		ISP_LOGV("frame id %d\r\n", cxt->cur_status.frame_id);

		ISP_LOGV("start idx %d   linetime %d\r\n", cxt->cur_status.start_index, cxt->cur_status.line_time);

		ISP_LOGV("mxidx %d   mnidx %d\r\n", cxt->cur_status.ae_table->max_index, cxt->cur_status.ae_table->min_index);

		ISP_LOGV("tar %d   tar_zone %d\r\n", cxt->cur_status.target_lum, cxt->cur_status.target_lum_zone);

		ISP_LOGV("flicker %d   ISO %d\r\n", cxt->cur_status.settings.flicker, cxt->cur_status.settings.iso);

		ISP_LOGV("work mod %d   scene mod %d\r\n", cxt->cur_status.settings.work_mode, cxt->cur_status.settings.scene_mode);

		ISP_LOGV("metering:%d\r\n", cxt->cur_status.settings.metering_mode);
	} else
		UNUSED(cxt);
	return rtn;
}
//uint32_t test[2][3] = {{1500, 256, 4096}, {375, 512, 8192}};
static cmr_s32 ae_exp_data_update(struct ae_ctrl_cxt *cxt, struct ae_sensor_exp_data *exp_data, struct q_item* write_item, struct q_item* actual_item, cmr_u32 is_force)
{
	float sensor_gain = 0.0;
	float isp_gain = 0.0;
	cmr_u32 max_gain = cxt->sensor_max_gain;
	cmr_u32 min_gain = cxt->sensor_min_gain;

	if (exp_data->lib_data.gain > max_gain) {/*gain : (sensor max gain, ~)*/
		sensor_gain = max_gain;
		isp_gain = (double)exp_data->lib_data.gain / (double)max_gain;
	} else if (exp_data->lib_data.gain > min_gain) {/*gain : (sensor_min_gain, sensor_max_gain)*/
		if (0 == exp_data->lib_data.gain % cxt->sensor_gain_precision) {
			sensor_gain = exp_data->lib_data.gain;
			isp_gain = 1.0;
		} else {
			sensor_gain = (exp_data->lib_data.gain * 1.0 / cxt->sensor_gain_precision) * cxt->sensor_gain_precision;
			isp_gain = exp_data->lib_data.gain * 1.0 / sensor_gain;
		}
	} else {/*gain : (~, sensor_min_gain)*/
		sensor_gain = min_gain;
		isp_gain = (double)exp_data->lib_data.gain / (double)min_gain;
	}

	exp_data->lib_data.sensor_gain = sensor_gain;
	exp_data->lib_data.isp_gain = isp_gain * 4096;
#if 0
	exp_data->lib_data.exp_line = test[hait_conts_flag % 2][0];
	exp_data->lib_data.sensor_gain= test[hait_conts_flag % 2][1];
	exp_data->lib_data.isp_gain = test[hait_conts_flag % 2][2];
	hait_conts_flag++;
#endif

	if (is_force) {/**/
		struct s_q_init_in init_in;
		struct s_q_init_out init_out;		

		init_in.exp_line = exp_data->lib_data.exp_line;
		init_in.exp_time = exp_data->lib_data.exp_time;
		init_in.dmy_line = exp_data->lib_data.dummy;
		init_in.sensor_gain = exp_data->lib_data.sensor_gain;
		init_in.isp_gain  = exp_data->lib_data.isp_gain;
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
		is_force, exp_data->lib_data.exp_line, exp_data->lib_data.dummy, exp_data->lib_data.sensor_gain, exp_data->lib_data.isp_gain,\
		write_item->exp_line, write_item->dumy_line, write_item->sensor_gain, write_item->isp_gain,\
		actual_item->exp_line, actual_item->dumy_line, actual_item->sensor_gain, actual_item->isp_gain);

	return ISP_SUCCESS;

}

static cmr_s32 ae_write_to_sensor(struct ae_ctrl_cxt *cxt, struct ae_exposure_param *write_param)
{
	struct ae_exposure_param *prv_param = &cxt->exp_data.write_data;

	if (0 !=  write_param->exp_line) {
		struct ae_exposure exp;
		cmr_s32 size_index = cxt->snr_info.sensor_size_index;
		if (cxt->isp_ops.ex_set_exposure) {
			memset(&exp, 0, sizeof(exp));
			exp.exposure = write_param->exp_line;
			exp.dummy = write_param->dummy;
			exp.size_index = size_index;
			(*cxt->isp_ops.ex_set_exposure) (cxt->isp_ops.isp_handler, &exp);
			if ((write_param->exp_line != prv_param->exp_line)
				|| (write_param->dummy != prv_param->dummy)) {
				(*cxt->isp_ops.ex_set_exposure) (cxt->isp_ops.isp_handler, &exp);
			} else {
				ISP_LOGV("no_need_write exp");
				;
			}
		} else if (cxt->isp_ops.set_exposure) {
			cmr_u32 ae_expline = write_param->exp_line;
			memset(&exp, 0, sizeof(exp));
			ae_expline = ae_expline & 0x0000ffff;
			ae_expline |= (write_param->dummy < 0x10) & 0x0fff0000;
			ae_expline |= (size_index << 0x1c) & 0xf0000000;
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
static cmr_s32 ae_result_update_to_sensor(struct ae_ctrl_cxt *cxt, struct ae_sensor_exp_data *exp_data, cmr_u32 is_force)
{
	cmr_s32 ret = ISP_SUCCESS;
	struct ae_exposure_param write_param;
	struct q_item write_item;
	struct q_item actual_item;

	if (0 == cxt) {
		ISP_LOGE("cxt invalid, cxt: %p\n", cxt);
		ret = ISP_ERROR;
		return ret;
	}

	ae_exp_data_update(cxt, exp_data, &write_item, &actual_item, is_force);
	

				//ISP_LOGV("no_need_write exp");
	write_param.exp_line = write_item.exp_line;
	write_param.exp_time = write_item.exp_time;
	write_param.dummy = write_item.dumy_line;
				//ISP_LOGV("no_need_write exp");


	write_param.isp_gain = write_item.isp_gain;

	write_param.sensor_gain  =write_item.sensor_gain;
				//ISP_LOGV("no_need_write gain");
	ae_write_to_sensor(cxt, &write_param);

	//exp_data->write_data = write_item;
	//exp_data->actual_data = actual_item;
	exp_data->write_data.exp_line = write_item.exp_line;
	exp_data->write_data.exp_time = write_item.exp_time;
	exp_data->write_data.dummy = write_item.dumy_line;
	exp_data->write_data.sensor_gain = write_item.sensor_gain;
	exp_data->write_data.isp_gain= write_item.isp_gain;

	exp_data->actual_data.exp_line = actual_item.exp_line;
	exp_data->actual_data.exp_time = actual_item.exp_time;
	exp_data->actual_data.dummy = actual_item.dumy_line;
	exp_data->actual_data.sensor_gain = actual_item.sensor_gain;
	exp_data->actual_data.isp_gain= actual_item.isp_gain;

	return ret;
}

static cmr_s32 _is_ae_mlog(struct ae_ctrl_cxt *cxt)
{
	cmr_u32 ret = 0;
	cmr_s32 len = 0;
	cmr_s32 is_save = 0;
#ifndef WIN32
	char value[PROPERTY_VALUE_MAX];
	len = property_get(AE_SAVE_MLOG, value, AE_SAVE_MLOG_DEFAULT);
	if (len) {
		memcpy((cmr_handle) & cxt->debug_file_name[0], &value[0], len);
		cxt->debug_info_handle = (cmr_u32) debug_file_init(cxt->debug_file_name, "w+t");
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

static void _print_ae_debug_info(char *log_str, struct ae_ctrl_cxt *cxt_ptr)
{
	float fps = 0.0;
	cmr_u32 pos = 0;
	struct ae_alg_calc_result *result_ptr;
	struct ae_alg_calc_param *sync_cur_status_ptr;

	sync_cur_status_ptr = &(cxt_ptr->sync_cur_status);
	result_ptr = &cxt_ptr->sync_cur_result;

	fps = SENSOR_EXP_NS_BASE * 1.0 / (cxt_ptr->snr_info.line_time * (result_ptr->wts.cur_dummy + result_ptr->wts.cur_exp_line));
	if (fps > cxt_ptr->sync_cur_status.settings.max_fps) {
		fps = cxt_ptr->sync_cur_status.settings.max_fps;
	}

	pos =
	    sprintf(log_str, "cam-id:%d frm-id:%d,flicker: %d\nidx(%d-%d):%d,cur-l:%d, tar-l:%d, lv:%d, bv: %d,expl(%d ns):%d, expt: %d, gain:%d, dmy:%d, FR(%d-%d):%.2f\n",
		    cxt_ptr->camera_id, sync_cur_status_ptr->frame_id, sync_cur_status_ptr->settings.flicker, sync_cur_status_ptr->ae_table->min_index,
		    sync_cur_status_ptr->ae_table->max_index, result_ptr->wts.cur_index, cxt_ptr->sync_cur_result.cur_lum, cxt_ptr->sync_cur_result.target_lum,
		    cxt_ptr->cur_result.cur_bv, cxt_ptr->cur_result.cur_bv_nonmatch, cxt_ptr->snr_info.line_time, result_ptr->wts.cur_exp_line,
		    result_ptr->wts.cur_exp_line * cxt_ptr->snr_info.line_time / SENSOR_LINETIME_BASE, result_ptr->wts.cur_again, result_ptr->wts.cur_dummy,
		    cxt_ptr->sync_cur_status.settings.min_fps, cxt_ptr->sync_cur_status.settings.max_fps, fps);

	if (result_ptr->log) {
		pos += sprintf((char *)((char *)log_str + pos), "adv info:\n%s\n", result_ptr->log);
	}
}

static cmr_s32 save_to_mlog_file(struct ae_ctrl_cxt *cxt, struct ae_misc_calc_out *result)
{
	cmr_s32 rtn = 0;
	char *tmp_str = (char *)cxt->debug_str;
	UNUSED(result);

	memset(tmp_str, 0, sizeof(cxt->debug_str));
	rtn = debug_file_open((debug_handle_t) cxt->debug_info_handle, 1, 0);
	if (0 == rtn) {
		_print_ae_debug_info(tmp_str, cxt);
		debug_file_print((debug_handle_t) cxt->debug_info_handle, tmp_str);
		debug_file_close((debug_handle_t) cxt->debug_info_handle);
	}

	return rtn;
}

static cmr_u32 _get_checksum(void)
{
	#define AE_CHECKSUM_FLAG 1024
	cmr_u32 checksum = 0;

	checksum = (sizeof(struct ae_ctrl_cxt)) % AE_CHECKSUM_FLAG;

	return checksum;
}

static cmr_s32 _check_handle(cmr_handle handle)
{
	struct ae_ctrl_cxt *cxt = (struct ae_ctrl_cxt *)handle;
	cmr_u32 checksum = 0;
	if (NULL == handle) {
		ISP_LOGE("fail to check handle");
		return AE_ERROR;
	}
	checksum = _get_checksum();
	if ((AE_START_ID != cxt->start_id) || (AE_END_ID != cxt->end_id) || (checksum != cxt->checksum)) {
		ISP_LOGE("fail to get checksum, start_id:%d, end_id:%d, check sum:%d\n",
			cxt->start_id, cxt->end_id, cxt->checksum);
		return AE_ERROR;
	}

	return AE_SUCCESS;
}

static cmr_s32 _unpack_tunning_param(cmr_handle param, cmr_u32 size, struct ae_tuning_param *tuning_param)
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

static cmr_u32 _calc_target_lum(cmr_u32 cur_target_lum, enum ae_level level, struct ae_ev_table *ev_table)
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

	ISP_LOGV("cur target lum=%d, ev diff=%d, level=%d", cur_target_lum, ev_table->lum_diff[level], level);

	target_lum = (cmr_s32) cur_target_lum + ev_table->lum_diff[level];
	target_lum = (target_lum < 0) ? 0 : target_lum;

	return (cmr_u32) target_lum;
}

static cmr_s32 _update_monitor_unit(struct ae_ctrl_cxt *cxt, struct ae_trim *trim)
{
	cmr_s32 rtn = AE_SUCCESS;
	struct ae_monitor_unit *unit = NULL;

	if (NULL == cxt || NULL == trim) {
		ISP_LOGE("fail to update monitor unit, cxt=%p, work_info=%p", cxt, trim);
		return AE_ERROR;
	}
	unit = &cxt->monitor_unit;

	if (unit) {
		unit->win_size.w = ((trim->w / unit->win_num.w) / 2) * 2;
		unit->win_size.h = ((trim->h / unit->win_num.h) / 2) * 2;
		unit->trim.w = unit->win_size.w * unit->win_num.w;
		unit->trim.h = unit->win_size.h * unit->win_num.h;
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
	,			// 1/17
	{128, 170, 200, 230, 240, 310, 370, 450, 600, 800, 950, 1210, 1500, 1900, 2300}
	,			// 1/20
	{128, 170, 200, 230, 240, 330, 370, 450, 600, 800, 950, 1210, 1500, 1900, 2300}
	,			// 1/25
	{128, 170, 183, 228, 260, 320, 370, 450, 600, 800, 950, 1210, 1500, 1900, 2300}
	,			// 1/33
	{128, 162, 200, 228, 254, 320, 370, 450, 600, 800, 950, 1210, 1500, 1900, 2300}
	,			// 1/50
	{128, 162, 207, 255, 254, 320, 370, 450, 600, 800, 950, 1210, 1500, 1900, 2300}
	,			// 1/100
	{128, 190, 207, 245, 254, 320, 370, 450, 600, 800, 950, 1210, 1500, 1900, 2300}
};

#if 0
static cmr_s32 _get_iso(struct ae_ctrl_cxt *cxt, cmr_s32 * real_iso)
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
	f_tmp = 10000000.0 / cxt->cur_result.wts.exposure_time;

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
static cmr_s32 _get_iso(struct ae_ctrl_cxt *cxt, cmr_u32 * real_iso)
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
	//ISP_LOGV("calc_iso=%d,real_gain=%f,iso=%d", calc_iso, real_gain, iso);
	return rtn;
}
#endif

static void convert_trim_to_rect(struct ae_trim *trim, struct ae_rect *rect)
{
	rect->start_x = trim->x;
	rect->start_y = trim->y;
	rect->end_x = rect->start_x + trim->w - 1;
	rect->end_y = rect->start_y + trim->h - 1;

	if ((cmr_s32) (rect->end_x) < 0)
		rect->end_x = 0;
	if ((cmr_s32) (rect->end_y) < 0)
		rect->end_y = 0;
}

static void increase_rect_offset(struct ae_rect *rect, cmr_s32 x_offset, cmr_s32 y_offset, struct ae_trim *trim)
{
	cmr_s32 start_x = 0;
	cmr_s32 end_x = 0;
	cmr_s32 start_y = 0;
	cmr_s32 end_y = 0;

	if (NULL == rect || NULL == trim) {
		ISP_LOGE("fail to increase rect offset, rect %p trim %p", rect, trim);
		return;
	}

	start_x = rect->start_x;
	end_x = rect->end_x;
	start_y = rect->start_y;
	end_y = rect->end_y;

	if (start_x - x_offset < 0)
		start_x = 0;
	else
		start_x -= x_offset;
	rect->start_x = start_x;

	if (end_x + x_offset >= (cmr_s32) trim->w)
		end_x = (cmr_s32) (trim->w - 1);
	else
		end_x += x_offset;
	rect->end_x = end_x;

	if (start_y - y_offset < 0)
		rect->start_y = 0;
	else
		start_y -= y_offset;
	rect->start_y = start_y;

	if (end_y + y_offset >= (cmr_s32) trim->h)
		end_y = trim->h - 1;
	else
		end_y += y_offset;
	rect->end_y = end_y;
}

static cmr_s32 is_in_rect(cmr_u32 x, cmr_u32 y, struct ae_rect *rect)
{
	cmr_s32 is_in = 0;

	if (NULL == rect) {
		ISP_LOGE("fail to is in rect, rect %p", rect);
		return 0;
	}

	if (rect->start_x <= x && rect->start_y <= y && x <= rect->end_x && y <= rect->end_y) {
		is_in = 1;
	}

	return is_in;
}

static cmr_s32 _set_touch_zone(struct ae_ctrl_cxt *cxt, struct ae_trim *touch_zone)
{
#if 1
	UNUSED(cxt);
	UNUSED(touch_zone);
	return AE_SUCCESS;
#else
	cmr_s32 rtn = AE_SUCCESS;
	struct ae_trim level_0_trim;

	cmr_u32 org_w = 0;
	cmr_u32 org_h = 0;
	cmr_u32 new_w = 0;
	cmr_u32 new_h = 0;
	cmr_s32 touch_x = 0;
	cmr_s32 touch_y = 0;
	cmr_s32 touch_w = 0;
	cmr_s32 touch_h = 0;
	cmr_u32 i = 0;
	cmr_u32 j = 0;
	struct ae_rect level_0_rect;
	struct ae_rect level_1_rect;
	struct ae_rect level_2_rect;
	cmr_u32 level_1_x_offset = 0;
	cmr_u32 level_1_y_offset = 0;
	cmr_u32 level_2_x_offset = 0;
	cmr_u32 level_2_y_offset = 0;
	cmr_u32 level_0_weight = 0;
	cmr_u32 level_1_weight = 0;
	cmr_u32 level_2_weight = 0;
	struct ae_trim map_trim;
	cmr_u8 *tab_ptr = NULL;
	cmr_s32 weight_val = 0;

	if (NULL == cxt || NULL == touch_zone) {
		ISP_LOGE("fail to set touch zone, cxt %p touch_zone %p param is error!", cxt, touch_zone);
		return AE_ERROR;
	}
	org_w = cxt->resolution_info.frame_size.w;
	org_h = cxt->resolution_info.frame_size.h;
	new_w = cxt->monitor_unit.win_num.w;
	new_h = cxt->monitor_unit.win_num.h;

	touch_x = touch_zone->x;
	touch_y = touch_zone->y;
	touch_w = touch_zone->w;
	touch_h = touch_zone->h;
	ISP_LOGI("touch_x %d, touch_y %d, touch_w %d, touch_h %d", touch_x, touch_y, touch_w, touch_h);
	level_0_trim.x = touch_x * new_w / org_w;
	level_0_trim.y = touch_y * new_h / org_h;
	level_0_trim.w = touch_w * new_w / org_w;
	level_0_trim.h = touch_h * new_h / org_h;

	map_trim.x = 0;
	map_trim.y = 0;
	map_trim.w = new_w;
	map_trim.h = new_h;
	convert_trim_to_rect(&level_0_trim, &level_0_rect);
	level_1_x_offset = (cxt->touch_zone_param.zone_param.level_1_percent * level_0_trim.w) >> (6 + 1);
	level_1_y_offset = (cxt->touch_zone_param.zone_param.level_1_percent * level_0_trim.h) >> (6 + 1);

	level_2_x_offset = (cxt->touch_zone_param.zone_param.level_2_percent * level_0_trim.w) >> (6 + 1);
	level_2_y_offset = (cxt->touch_zone_param.zone_param.level_2_percent * level_0_trim.h) >> (6 + 1);

	// level 1 rect
	level_1_rect = level_0_rect;
	increase_rect_offset(&level_1_rect, level_1_x_offset, level_1_y_offset, &map_trim);
	// level 2 rect
	level_2_rect = level_1_rect;
	increase_rect_offset(&level_2_rect, level_2_x_offset, level_2_y_offset, &map_trim);

	level_0_weight = cxt->touch_zone_param.zone_param.level_0_weight;
	level_1_weight = cxt->touch_zone_param.zone_param.level_1_weight;
	level_2_weight = cxt->touch_zone_param.zone_param.level_2_weight;

	tab_ptr = cxt->touch_zone_param.weight_table.weight;
	for (j = 0; j < new_h; ++j) {
		for (i = 0; i < new_w; ++i) {
			weight_val = 0;

			// level 2 rect, big
			if (is_in_rect(i, j, &level_2_rect))
				weight_val = level_2_weight;
			// level 1 rect, middle
			if (is_in_rect(i, j, &level_1_rect))
				weight_val = level_1_weight;
			// level 0 rect, small
			if (is_in_rect(i, j, &level_0_rect))
				weight_val = level_0_weight;

			tab_ptr[i + new_w * j] = weight_val;
		}
	}

	{
		struct ae_weight_table *weight_tab;

		weight_tab = &cxt->touch_zone_param.weight_table;
		rtn = ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_SET_WEIGHT_TABLE, (cmr_handle) weight_tab, NULL);
		if (AE_SUCCESS == rtn) {
			cxt->cur_status.touch_zone = *touch_zone;
		}
	}
	cxt->touch_zone_param.enable = 1;

	return rtn;
#endif
}

static cmr_s32 _get_bv_by_lum_new(struct ae_ctrl_cxt *cxt, cmr_s32 * bv)
{
	cmr_s32 rtn = AE_SUCCESS;

	if (NULL == cxt || NULL == bv) {
		ISP_LOGE("fail to get bv by lum, cxt %p bv %p", cxt, bv);
		return AE_ERROR;
	}

	if (0 == cxt->cur_param->alg_id) {
		cmr_u32 cur_lum = 0;
		float real_gain = 0;
		cmr_u32 cur_exp = 0;
		*bv = 0;
		cur_lum = cxt->cur_result.cur_lum;
		cur_exp = cxt->cur_result.wts.exposure_time;
		real_gain = _get_real_gain(cxt->cur_result.wts.cur_again);
		if (0 == cur_lum) {
			cur_lum = 1;
		}
		*bv = (cmr_s32) (log2(((double)100000000 * cur_lum) / ((double)cur_exp * real_gain * 5)) * 16.0 + 0.5);
	} else if (2 == cxt->cur_param->alg_id) {
		*bv = cxt->sync_cur_result.cur_bv;
	}
	ISP_LOGV("real bv %d", *bv);
	return rtn;
}

static cmr_s32 _get_bv_by_lum(struct ae_ctrl_cxt *cxt, cmr_s32 * bv)
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
	real_gain = _get_real_gain(cxt->cur_result.wts.cur_again);

	if (0 == cur_lum)
		cur_lum = 1;
#ifdef AE_TABLE_32
	*bv = (cmr_s32) (log2(((double)100000000 * cur_lum) / ((double)cur_exp * real_gain * 5)) * 16.0 + 0.5);
#else
	*bv = (cmr_s32) (log2((double)(cur_exp * cur_lum) / (double)(real_gain * 5)) * 16.0 + 0.5);
#endif

	//ISP_LOGV("real bv=%d", *bv);

	return rtn;
}

static float _get_real_gain(cmr_u32 gain)
{	
	// x=real_gain/16
	float real_gain = 0;
	cmr_u32 cur_gain = gain;
	cmr_u32 i = 0;

	real_gain = gain * 1.0 / 8.0;	// / 128 * 16;

	return real_gain;
}

static cmr_s32 _get_bv_by_gain(struct ae_ctrl_cxt *cxt, cmr_s16 * gain, cmr_s32 * bv)
{
	cmr_s32 rtn = AE_SUCCESS;
	cmr_u32 cur_gain = 0;

	if (NULL == cxt || NULL == gain || NULL == bv) {
		ISP_LOGE("fail to get bv by gain, cxt %p cur_result %p bv %p", cxt, gain, bv);
		return AE_ERROR;
	}
	cur_gain = *gain;
	*bv = (cmr_u32)_get_real_gain(cur_gain);

	ISP_LOGV("bv=%d", *bv);

	return rtn;
}

static cmr_s32 _cfg_monitor_win(struct ae_ctrl_cxt *cxt)
{
	cmr_s32 rtn = AE_SUCCESS;

	if (cxt->isp_ops.set_monitor_win) {
		struct ae_monitor_info info;

		info.win_size = cxt->monitor_unit.win_size;
		info.trim = cxt->monitor_unit.trim;

		/*TBD remove it */
		cxt->cur_status.monitor_shift = 0;
		rtn = cxt->isp_ops.set_monitor_win(cxt->isp_ops.isp_handler, &info);
	}

	return rtn;
}

static cmr_s32 _cfg_monitor_bypass(struct ae_ctrl_cxt *cxt)
{
	cmr_s32 rtn = AE_SUCCESS;

	if (cxt->isp_ops.set_monitor_bypass) {
		cmr_u32 is_bypass = 0;

		is_bypass = cxt->monitor_unit.is_stop_monitor;
		// ISP_LOGI("AE_TEST: is_bypass=%d", is_bypass);
		rtn = cxt->isp_ops.set_monitor_bypass(cxt->isp_ops.isp_handler, is_bypass);
	}

	return rtn;
}

static cmr_s32 _cfg_set_aem_mode(struct ae_ctrl_cxt *cxt)
{
	cmr_s32 ret = AE_SUCCESS;

	if (cxt->isp_ops.set_statistics_mode) {
		cxt->isp_ops.set_statistics_mode(cxt->isp_ops.isp_handler, cxt->monitor_unit.mode, cxt->monitor_unit.cfg.skip_num);
	} else {
		ISP_LOGE("fail to set aem mode");
		ret = AE_ERROR;
	}

	return ret;
}

static cmr_s32 _set_g_stat(struct ae_ctrl_cxt *cxt, struct ae_stat_mode *stat_mode)
{
	cmr_s32 rtn = AE_SUCCESS;

	if (NULL == cxt || NULL == stat_mode) {
		ISP_LOGE("fail to set g stat, cxt %p stat_mode %p", cxt, stat_mode);
		return AE_ERROR;
	}

	rtn = _update_monitor_unit(cxt, &stat_mode->trim);
	if (AE_SUCCESS != rtn) {
		goto EXIT;
	}

	rtn = _cfg_monitor_win(cxt);
EXIT:
	return rtn;
}

static cmr_s32 _set_scaler_trim(struct ae_ctrl_cxt *cxt, struct ae_trim *trim)
{
	cmr_s32 rtn = AE_SUCCESS;

	rtn = _update_monitor_unit(cxt, trim);

	if (AE_SUCCESS != rtn) {
		goto EXIT;
	}

	rtn = _cfg_monitor_win(cxt);
EXIT:
	return rtn;
}

static cmr_s32 _set_snapshot_notice(struct ae_ctrl_cxt *cxt, struct ae_snapshot_notice *notice)
{
	cmr_s32 rtn = AE_SUCCESS;
	UNUSED(cxt);
	UNUSED(notice);
	return rtn;
}

static cmr_u32 get_cur_timestamp(struct ae_ctrl_cxt *cxt)
{
	cmr_s32 rtn = AE_SUCCESS;
	cmr_u32 sec = 0, usec = 0;
	cmr_u32 timestamp = 0;
	cmr_u64 calc_sec = 0;

	if (cxt->isp_ops.get_system_time) {
		rtn = (*cxt->isp_ops.get_system_time) (cxt->isp_ops.isp_handler, &sec, &usec);
		calc_sec = sec;
		timestamp = (cmr_u32) (calc_sec * 1000000 + usec) / 1000;
	}

	return timestamp;
}

static cmr_s32 _set_flash_notice(struct ae_ctrl_cxt *cxt, struct ae_flash_notice *flash_notice)
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
		cxt->cur_status.settings.flash = FLASH_PRE_BEFORE;
		cxt->cur_flicker = cxt->sync_cur_status.settings.flicker;/*backup the flicker flag before flash
														* and will lock flicker result
														*/
		if (0 != cxt->flash_ver)
			rtn = _set_pause(cxt);
		break;

	case AE_FLASH_PRE_LIGHTING:
		ISP_LOGI("ae_flash_status FLASH_PRE_LIGHTING");
		cxt->cur_status.settings.flash = FLASH_PRE;
		cxt->cur_status.settings.flash_target = cxt->cur_param->flash_param.target_lum;
		if (0 != cxt->flash_ver) {
			/*lock AE algorithm*/
			cxt->pre_flash_skip = 2;
			cxt->aem_effect_delay = 0;
		}
		break;

	case AE_FLASH_PRE_AFTER:
		ISP_LOGI("ae_flash_status FLASH_PRE_AFTER");
		cxt->cur_status.settings.flash = FLASH_PRE_AFTER;
		if (0 != cxt->flash_ver)
			rtn = _set_restore_cnt(cxt);
		break;

	case AE_FLASH_MAIN_BEFORE:
		ISP_LOGI("ae_flash_status FLASH_MAIN_BEFORE");
		cxt->cur_status.settings.flash = FLASH_MAIN_BEFORE;
		if (0 != cxt->flash_ver)
			rtn = _set_pause(cxt);
		break;

	case AE_FLASH_MAIN_AFTER:
		ISP_LOGI("ae_flash_status FLASH_MAIN_AFTER");
		cxt->cur_status.settings.flash = FLASH_MAIN_AFTER;
		cxt->cur_status.settings.flicker = cxt->cur_flicker;
		if (cxt->camera_id && cxt->fdae.pause)
			cxt->fdae.pause = 0;

		if (0 != cxt->flash_ver)
			rtn = _set_restore_cnt(cxt);
		break;

	case AE_FLASH_MAIN_AE_MEASURE:
		ISP_LOGI("ae_flash_status FLASH_MAIN_AE_MEASURE");
		break;

	case AE_FLASH_MAIN_LIGHTING:
		ISP_LOGI("ae_flash_status FLASH_MAIN_LIGHTING");
		break;
	default:
		rtn = AE_ERROR;
		break;
	}
	return rtn;
}

static cmr_s32 _cfg_monitor_skip(struct ae_ctrl_cxt *cxt, struct ae_monitor_cfg *cfg)
{
	cmr_s32 rtn = AE_SUCCESS;

	if (cxt->isp_ops.set_monitor) {
		// ISP_LOGI("AE_TEST set monitor:%d",
		// *(cmr_u32*)cfg);
		rtn = cxt->isp_ops.set_monitor(cxt->isp_ops.isp_handler, cfg);
	}

	return rtn;
}

static cmr_s32 _cfg_monitor(struct ae_ctrl_cxt *cxt)
{
	cmr_s32 rtn = AE_SUCCESS;

	_cfg_monitor_win(cxt);

	_cfg_monitor_skip(cxt, &cxt->monitor_unit.cfg);

	_cfg_monitor_bypass(cxt);

	return rtn;
}

static cmr_s32 exp_time2exp_line(struct ae_ctrl_cxt *cxt, struct ae_exp_gain_table src[AE_FLICKER_NUM], struct ae_exp_gain_table dst[AE_FLICKER_NUM], cmr_s16 linetime,
				 cmr_s16 tablemode)
{
	cmr_s32 rtn = AE_SUCCESS;
	cmr_s32 i = 0;
	float tmp_1 = 0;
	float tmp_2 = 0;
	cmr_s32 mx = src[AE_FLICKER_50HZ].max_index;

	ISP_LOGV("cam-id %d exp2line %d %d %d\r\n", cxt->camera_id, linetime, tablemode, mx);
	UNUSED(cxt);
	dst[AE_FLICKER_60HZ].max_index = src[AE_FLICKER_50HZ].max_index;
	dst[AE_FLICKER_60HZ].min_index = src[AE_FLICKER_50HZ].min_index;
	if (0 == tablemode) {
		for (i = 0; i <= mx; i++) {
			tmp_1 = src[AE_FLICKER_50HZ].exposure[i] / (float)linetime;
			dst[AE_FLICKER_50HZ].exposure[i] = (cmr_s32) tmp_1;

			if (0 == (cmr_s32) tmp_1)
				tmp_2 = 1;
			else
				tmp_2 = tmp_1 / (cmr_s32) tmp_1;

			dst[AE_FLICKER_50HZ].again[i] = (cmr_s32) (0.5 + tmp_2 * src[AE_FLICKER_50HZ].again[i]);
		}

		for (i = 0; i <= mx; i++) {
			if (83333 <= src[AE_FLICKER_50HZ].exposure[i]) {
				tmp_1 = dst[AE_FLICKER_50HZ].exposure[i] * 5 / 6.0;
				dst[AE_FLICKER_60HZ].exposure[i] = (cmr_s32) tmp_1;

				if (0 == (cmr_s32) tmp_1)
					tmp_2 = 1;
				else
					tmp_2 = (float)(dst[AE_FLICKER_50HZ].exposure[i]) / (cmr_s32) tmp_1;

				dst[AE_FLICKER_60HZ].again[i] = (cmr_s32) (0.5 + tmp_2 * dst[AE_FLICKER_50HZ].again[i]);
			} else {
				dst[AE_FLICKER_60HZ].exposure[i] = dst[AE_FLICKER_50HZ].exposure[i];
				dst[AE_FLICKER_60HZ].again[i] = dst[AE_FLICKER_50HZ].again[i];
			}
		}
	} else {
		for (i = 0; i <= mx; i++) {
			if (83333 <= dst[AE_FLICKER_50HZ].exposure[i] * linetime) {
				tmp_1 = dst[AE_FLICKER_50HZ].exposure[i] * 5 / 6.0;
				dst[AE_FLICKER_60HZ].exposure[i] = (cmr_s32) tmp_1;

				if (0 == (cmr_s32) tmp_1)
					tmp_2 = 1;
				else
					tmp_2 = (float)(dst[AE_FLICKER_50HZ].exposure[i]) / (cmr_s32) tmp_1;

				dst[AE_FLICKER_60HZ].again[i] = (cmr_s32) (0.5 + tmp_2 * dst[AE_FLICKER_50HZ].again[i]);
			} else {
				dst[AE_FLICKER_60HZ].exposure[i] = dst[AE_FLICKER_50HZ].exposure[i];
				dst[AE_FLICKER_60HZ].again[i] = dst[AE_FLICKER_50HZ].again[i];
			}
		}
	}
	return rtn;
}

static cmr_s32 exposure_time2line(struct ae_tuning_param *tp, cmr_s16 linetime, cmr_s16 tablemode)
{
	cmr_s32 rtn = AE_SUCCESS;
	cmr_s32 i = 0;
	float tmp_1 = 0;
	float tmp_2 = 0;
	cmr_s32 mx = tp->backup_ae_table[AE_FLICKER_50HZ][AE_ISO_AUTO].max_index;

	ISP_LOGV("exp2line %d %d %d\r\n", linetime, tablemode, mx);

	tp->ae_table[AE_FLICKER_60HZ][AE_ISO_AUTO].max_index = tp->backup_ae_table[AE_FLICKER_50HZ][AE_ISO_AUTO].max_index;
	tp->ae_table[AE_FLICKER_60HZ][AE_ISO_AUTO].min_index = tp->backup_ae_table[AE_FLICKER_50HZ][AE_ISO_AUTO].min_index;
	if (0 == tablemode) {
		for (i = 0; i <= mx; i++) {
			tmp_1 = tp->backup_ae_table[AE_FLICKER_50HZ][AE_ISO_AUTO].exposure[i] / (float)linetime;
			tp->ae_table[AE_FLICKER_50HZ][AE_ISO_AUTO].exposure[i] = (cmr_s32) tmp_1;

			if (0 == (cmr_s32) tmp_1)
				tmp_2 = 1;
			else
				tmp_2 = tmp_1 / (cmr_s32) tmp_1;

			tp->ae_table[AE_FLICKER_50HZ][AE_ISO_AUTO].again[i] = (cmr_s32) (0.5 + tmp_2 * tp->backup_ae_table[AE_FLICKER_50HZ][AE_ISO_AUTO].again[i]);
		}

		for (i = 0; i <= mx; i++) {
			if (83333 <= tp->backup_ae_table[AE_FLICKER_50HZ][AE_ISO_AUTO].exposure[i]) {
				tmp_1 = tp->ae_table[AE_FLICKER_50HZ][AE_ISO_AUTO].exposure[i] * 5 / 6.0;
				tp->ae_table[AE_FLICKER_60HZ][AE_ISO_AUTO].exposure[i] = (cmr_s32) tmp_1;

				if (0 == (cmr_s32) tmp_1)
					tmp_2 = 1;
				else
					tmp_2 = (float)(tp->ae_table[AE_FLICKER_50HZ][AE_ISO_AUTO].exposure[i]) / (cmr_s32) tmp_1;

				tp->ae_table[AE_FLICKER_60HZ][AE_ISO_AUTO].again[i] = (cmr_s32) (0.5 + tmp_2 * tp->ae_table[AE_FLICKER_50HZ][AE_ISO_AUTO].again[i]);
			} else {
				tp->ae_table[AE_FLICKER_60HZ][AE_ISO_AUTO].exposure[i] = tp->ae_table[AE_FLICKER_50HZ][AE_ISO_AUTO].exposure[i];
				tp->ae_table[AE_FLICKER_60HZ][AE_ISO_AUTO].again[i] = tp->ae_table[AE_FLICKER_50HZ][AE_ISO_AUTO].again[i];
			}
		}
	} else {
		for (i = 0; i <= mx; i++) {
			if (83333 <= tp->ae_table[AE_FLICKER_50HZ][AE_ISO_AUTO].exposure[i] * linetime) {
				tmp_1 = tp->ae_table[AE_FLICKER_50HZ][AE_ISO_AUTO].exposure[i] * 5 / 6.0;
				tp->ae_table[AE_FLICKER_60HZ][AE_ISO_AUTO].exposure[i] = (cmr_s32) tmp_1;

				if (0 == (cmr_s32) tmp_1)
					tmp_2 = 1;
				else
					tmp_2 = (float)(tp->ae_table[AE_FLICKER_50HZ][AE_ISO_AUTO].exposure[i]) / (cmr_s32) tmp_1;

				tp->ae_table[AE_FLICKER_60HZ][AE_ISO_AUTO].again[i] = (cmr_s32) (0.5 + tmp_2 * tp->ae_table[AE_FLICKER_50HZ][AE_ISO_AUTO].again[i]);
			} else {
				tp->ae_table[AE_FLICKER_60HZ][AE_ISO_AUTO].exposure[i] = tp->ae_table[AE_FLICKER_50HZ][AE_ISO_AUTO].exposure[i];
				tp->ae_table[AE_FLICKER_60HZ][AE_ISO_AUTO].again[i] = tp->ae_table[AE_FLICKER_50HZ][AE_ISO_AUTO].again[i];
			}
		}
	}
	return rtn;
}

// set default parameters when initialization or DC-DV convertion
static cmr_s32 _set_ae_param(struct ae_ctrl_cxt *cxt, struct ae_init_in *init_param, struct ae_set_work_param *work_param, cmr_s8 init)
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
		// ISP_LOGV("tunnig_param %d", sizeof(struct ae_tuning_param));
		// ISP_LOGV("param_num %d", init_param->param_num);

		cur_work_mode = AE_WORK_MODE_COMMON;
		for (i = 0; i < init_param->param_num && i < AE_MAX_PARAM_NUM; ++i) {
			rtn = _unpack_tunning_param(init_param->param[i].param, init_param->param[i].size, &cxt->tuning_param[i]);
			memcpy(&cxt->tuning_param[i].backup_ae_table[0][0], &cxt->tuning_param[i].ae_table[0][0], AE_FLICKER_NUM * AE_ISO_NUM *sizeof(struct ae_exp_gain_table));
			//exposure_time2line(&(cxt->tuning_param[i]), init_param->resolution_info.line_time / SENSOR_LINETIME_BASE, cxt->tuning_param[i].ae_tbl_exp_mode);

			for (j = 0; j < AE_SCENE_MAX; ++j) {
				memcpy(&cxt->back_scene_mode_ae_table[j][AE_FLICKER_50HZ], &cxt->tuning_param[i].scene_info[j].ae_table[AE_FLICKER_50HZ],
				       AE_FLICKER_NUM * sizeof(struct ae_exp_gain_table));
				//ISP_LOGV("special_scene table_enable_and_table_mode is: %d,%d,%d,%d\n",i,j,cxt->tuning_param[i].scene_info[j].table_enable,cxt->tuning_param[i].scene_info[j].exp_tbl_mode);
				if ((1 == cxt->tuning_param[i].scene_info[j].table_enable) && (0 == cxt->tuning_param[i].scene_info[j].exp_tbl_mode)) {
					//exp_time2exp_line(cxt,j, init_param->resolution_info.line_time,
					//                      cxt->tuning_param[i].scene_info[j].exp_tbl_mode, &(cxt->tuning_param[i].scene_info[j]));
					exp_time2exp_line(cxt, cxt->back_scene_mode_ae_table[j],
							  cxt->tuning_param[i].scene_info[j].ae_table,
							  init_param->resolution_info.line_time / SENSOR_LINETIME_BASE, cxt->tuning_param[i].scene_info[j].exp_tbl_mode);
				}
			}

			if (AE_SUCCESS == rtn)
				cxt->tuning_param_enable[i] = 1;
			else
				cxt->tuning_param_enable[i] = 0;
		}

		cxt->camera_id = init_param->camera_id;
		cxt->isp_ops = init_param->isp_ops;
		cxt->monitor_unit.win_num = init_param->monitor_win_num;

		cxt->snr_info = init_param->resolution_info;
		cxt->cur_status.frame_size = init_param->resolution_info.frame_size;
		cxt->cur_status.line_time = init_param->resolution_info.line_time / SENSOR_LINETIME_BASE;
		trim.x = 0;
		trim.y = 0;
		trim.w = init_param->resolution_info.frame_size.w;
		trim.h = init_param->resolution_info.frame_size.h;

		cxt->cur_status.frame_id = 0;
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

	cxt->monitor_unit.mode = AE_STATISTICS_MODE_CONTINUE;
	cxt->monitor_unit.cfg.skip_num = 0;
	cxt->monitor_unit.is_stop_monitor = 0;
	/* set cxt->monitor_unit.trim & cxt->monitor_unit.win_size */
	_update_monitor_unit(cxt, &trim);

	if (1 == cxt->tuning_param_enable[cur_work_mode])
		cxt->cur_param = &cxt->tuning_param[cur_work_mode];
	else
		cxt->cur_param = &cxt->tuning_param[AE_WORK_MODE_COMMON];

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

	cxt->cur_status.log_level = g_isp_log_level;
	cxt->cur_status.alg_id = cxt->cur_param->alg_id;
	cxt->cur_status.win_size = cxt->monitor_unit.win_size;
	cxt->cur_status.win_num = cxt->monitor_unit.win_num;
	cxt->cur_status.awb_gain.r = 1024;
	cxt->cur_status.awb_gain.g = 1024;
	cxt->cur_status.awb_gain.b = 1024;
	cxt->cur_status.ae_table = &cxt->cur_param->ae_table[cxt->cur_param->flicker_index][AE_ISO_AUTO];
	cxt->cur_status.ae_table->min_index = 0;
	cxt->cur_status.weight_table = cxt->cur_param->weight_table[AE_WEIGHT_CENTER].weight;
	cxt->cur_status.stat_img = NULL;

#ifdef CONFIG_CAMERA_DUAL_SYNC
	if (init_param->is_multi_mode) {
		/* save master & slave sensor info */
		struct sensor_info sensor_info;
//		sensor_info.max_again = cxt->cur_status.max_gain;
//		sensor_info.min_again = cxt->cur_status.min_gain;
		sensor_info.sensor_gain_precision = cxt->sensor_gain_precision;
//		sensor_info.min_exp_line = cxt->cur_status.min_exp_line;
		sensor_info.line_time = cxt->cur_status.line_time;

		rtn = cxt->ptr_isp_br_ioctrl(cxt->camera_id,
			init_param->sensor_role ? SET_MASTER_MODULE_INFO : SET_SLAVE_MODULE_INFO,
			&sensor_info,
			NULL);
		ISP_LOGI("sensor info: role=%d, max_gain=%d, min_gain=%d, precision=%d, min_exp_line=%d, line_time=%d",
			init_param->sensor_role,
			sensor_info.max_again,
			sensor_info.min_again,
			sensor_info.sensor_gain_precision,
			sensor_info.min_exp_line,
			sensor_info.line_time);
	}
#endif
	cxt->cur_status.start_index = cxt->cur_param->start_index;
	ev_table = &cxt->cur_param->ev_table;
	cxt->cur_status.target_lum = _calc_target_lum(cxt->cur_param->target_lum, ev_table->default_level, ev_table);
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
	/* caliberation for bv match with lv */
	//cxt->cur_param->lv_cali.lux_value = 11800;//hjw
	//cxt->cur_param->lv_cali.bv_value = 1118;//hjw
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

	ISP_LOGV("cam-id %d, snr setting max fps: %d\n", cxt->camera_id, cxt->snr_info.snr_setting_max_fps);
	//if (0 != cxt->snr_info.snr_setting_max_fps) {
	//      cxt->cur_status.settings.sensor_max_fps = 30;//cxt->snr_info.snr_setting_max_fps;
	//} else {
	cxt->cur_status.settings.sensor_max_fps = 30;
	//}

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
		//ISP_LOGV("TC_NO these setting at tuning file");
	} else {
		cxt->cur_status.touch_tuning_win = cxt->cur_param->touch_info.touch_tuning_win;
		cxt->cur_status.win1_weight = cxt->cur_param->touch_info.win1_weight;
		cxt->cur_status.win2_weight = cxt->cur_param->touch_info.win2_weight;
		//ISP_LOGV("TC_Have these setting at tuning file");
		//ISP_LOGV("TC_Beth-SPRD:touch tuning win info:W:%d,H:%d!", cxt->cur_param->touch_info.touch_tuning_win.w, cxt->cur_param->touch_info.touch_tuning_win.h);
	}
	/* fd-ae param */
	//_fdae_init(cxt);

	/* control information for sensor update */
	cxt->checksum = _get_checksum();
	cxt->end_id = AE_END_ID;

	/* set ae monitor work mode */
	_cfg_set_aem_mode(cxt);

	ISP_LOGI("cam-id %d, ALG_id %d   %zd\r\n", cxt->camera_id, cxt->cur_param->alg_id, sizeof(struct ae_tuning_param));
	//ISP_LOGV("DP %d   lv-bv %.3f\r\n", cxt->sensor_gain_precision, cxt->cur_status.cali_lv_eight);
	return AE_SUCCESS;
}

static cmr_s32 _sof_handler(struct ae_ctrl_cxt *cxt)
{
	cmr_s32 rtn = AE_SUCCESS;
	UNUSED(cxt);
	// _fdae_update_state(cxt);
	return rtn;
}

static cmr_s32 _ae_online_ctrl_set(struct ae_ctrl_cxt *cxt, cmr_handle param)
{
	cmr_s32 rtn = AE_SUCCESS;
	struct ae_online_ctrl *ae_ctrl_ptr = (struct ae_online_ctrl *)param;
	struct ae_misc_calc_out ctrl_result;
	cmr_u32 ctrl_index = 0;

	if ((NULL == cxt) || (NULL == param)) {
		ISP_LOGE("fail to set ae online ctrl, in %p out %p", cxt, param);
		return AE_PARAM_NULL;
	}

	ISP_LOGV("cam-id:%d, mode:%d, idx:%d, s:%d, g:%d\n", cxt->camera_id, ae_ctrl_ptr->mode,
		ae_ctrl_ptr->index, ae_ctrl_ptr->shutter, ae_ctrl_ptr->again);
	cxt->cur_status.settings.lock_ae = AE_STATE_LOCKED;
	if (AE_CTRL_SET_INDEX == ae_ctrl_ptr->mode) {
		cxt->cur_status.settings.manual_mode = 1;	/*ae index */
		cxt->cur_status.settings.table_idx = ae_ctrl_ptr->index;
	} else {		/*exposure & gain */
		cxt->cur_status.settings.manual_mode = 0;
		cxt->cur_status.settings.exp_line = ae_ctrl_ptr->shutter;
		cxt->cur_status.settings.gain = ae_ctrl_ptr->again;
	}

	return rtn;
}

static cmr_s32 _ae_online_ctrl_get(struct ae_ctrl_cxt *cxt, cmr_handle result)
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
	ISP_LOGV("cam-id:%d, idx:%d, s:%d, g:%d\n", cxt->camera_id, ae_ctrl_ptr->index,
		ae_ctrl_ptr->shutter, ae_ctrl_ptr->again);
	return rtn;
}

static cmr_s32 _tool_online_ctrl(struct ae_ctrl_cxt *cxt, cmr_handle param, cmr_handle result)
{
	cmr_s32 rtn = AE_SUCCESS;

	struct ae_online_ctrl *ae_ctrl_ptr = (struct ae_online_ctrl *)param;

	if ((AE_CTRL_SET_INDEX == ae_ctrl_ptr->mode)
	    || (AE_CTRL_SET == ae_ctrl_ptr->mode)) {
		rtn = _ae_online_ctrl_set(cxt, param);
	} else {
		rtn = _ae_online_ctrl_get(cxt, result);
	}

	return rtn;
}

static cmr_s32 _printf_status_log(struct ae_ctrl_cxt *cxt, cmr_s8 scene_mod, struct ae_alg_calc_param *alg_status)
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
static cmr_s32 _set_scene_mode(struct ae_ctrl_cxt *cxt, enum ae_scene_mode cur_scene_mod, enum ae_scene_mode nxt_scene_mod)
{
	cmr_s32 rtn = AE_SUCCESS;
	struct ae_tuning_param *cur_param = NULL;
	struct ae_scene_info *scene_info = NULL;
	struct ae_alg_calc_param *cur_status = NULL;
	struct ae_alg_calc_param *prv_status = NULL;
	struct ae_exp_gain_table *ae_table = NULL;
	struct ae_weight_table *weight_table = NULL;
	struct ae_set_fps fps_param;
	cmr_u32 i = 0;
	cmr_s32 target_lum = 0;
	cmr_u32 iso = 0;
	cmr_u32 weight_mode = 0;
	cmr_s32 max_index = 0;
	prv_status = &cxt->prv_status;
	cur_status = &cxt->cur_status;

	if (nxt_scene_mod >= AE_SCENE_MAX) {
		ISP_LOGE("fail to set scene mod, %d\n", nxt_scene_mod);
		return AE_ERROR;
	}
	cur_param = cxt->cur_param;
	scene_info = &cur_param->scene_info[0];
	if ((AE_SCENE_NORMAL == cur_scene_mod) && (AE_SCENE_NORMAL == nxt_scene_mod)) {
		ISP_LOGI("normal  has special setting\n");
		goto SET_SCENE_MOD_EXIT;
	}

	if (AE_SCENE_NORMAL != nxt_scene_mod) {
		for (i = 0; i < AE_SCENE_MAX; ++i) {
			ISP_LOGI("%d: mod: %d, eb: %d\n", i, scene_info[i].scene_mode, scene_info[i].enable);
			if ((1 == scene_info[i].enable) && (nxt_scene_mod == scene_info[i].scene_mode)) {
				break;
			}
		}

		if ((i >= AE_SCENE_MAX) && (AE_SCENE_NORMAL != nxt_scene_mod)) {
			ISP_LOGI("Not has special scene setting, just using the normal setting\n");
			goto SET_SCENE_MOD_EXIT;
		}
	}

	if (AE_SCENE_NORMAL != nxt_scene_mod) {
		/*normal scene--> special scene */
		/*special scene--> special scene */
		ISP_LOGI("i and iso_index is %d,%d,%d", i, scene_info[i].iso_index, scene_info[i].weight_mode);
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
		}else{
			cur_status->ae_table = cxt->prv_status.ae_table;
		}
		cur_status->ae_table->min_index = 0;
		/*
		for (cmr_s32 j = 0; j <= max_index; j++) {
			ISP_LOGV("Current_status_exp_gain is %d,%d\n", cur_status->ae_table->exposure[j], cur_status->ae_table->again[j]);
		}
		*/
#endif
		cur_status->settings.iso = scene_info[i].iso_index;
		cur_status->settings.min_fps = scene_info[i].min_fps;
		cur_status->settings.max_fps = scene_info[i].max_fps;
		target_lum = _calc_target_lum(scene_info[i].target_lum, scene_info[i].ev_offset, &cur_param->ev_table);
		cur_status->target_lum_zone = (cmr_s16)(cur_param->stable_zone_ev[cur_param->ev_table.default_level]*target_lum * 1.0 / cur_param->target_lum + 0.5);
		if(2 > cur_status->target_lum_zone){
			cur_status->target_lum_zone = 2;
		}
		cur_status->target_lum  = target_lum;
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
		target_lum = _calc_target_lum(cur_param->target_lum, prv_status->settings.ev_index, &cur_param->ev_table);
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
	ISP_LOGI("cam-id %d, change scene mode from %d to %d, rtn=%d", cxt->camera_id,
		cur_scene_mod, nxt_scene_mod, rtn);
	return rtn;
}
#if 0
/**************************************************************************/
/* *Begin: FDAE related functions */
static cmr_s32 _fdae_init(struct ae_ctrl_cxt *cxt)
{
	cmr_s32 rtn = AE_SUCCESS;

	cxt->fdae.allow_to_work = 1;
	cxt->fdae.enable = 1;
	cxt->fdae.face_info.face_num = 0;
	return rtn;
}

// add by matchbox for fd_info
static cmr_s32 _fd_info_init(struct ae_ctrl_cxt *cxt)
{
	cmr_s32 rtn = AE_SUCCESS;
	struct ae1_fd_param *ae1_fd = NULL;
	cmr_s32 i = 0;

	if (NULL == cxt) {
		ISP_LOGE("fail to init fd info");
		return AE_ERROR;
	}

	ae1_fd = &(cxt->cur_status.ae1_finfo);
	ae1_fd->enable_flag = 1;
	ae1_fd->update_flag = 0;
	ae1_fd->cur_info.face_num = 0;
	//ae1_fd->pre_info.face_num = 0;
	for (i = 0; i != 20; i++) {
		// init pre info
		/*
		   ae1_fd->pre_info.face_area[i].start_x = 0;
		   ae1_fd->pre_info.face_area[i].start_y = 0;
		   ae1_fd->pre_info.face_area[i].end_x = 0;
		   ae1_fd->pre_info.face_area[i].end_y = 0;
		   ae1_fd->pre_info.face_area[i].pose = -1;
		 */
		// init cur_info
		ae1_fd->cur_info.face_area[i].start_x = 0;
		ae1_fd->cur_info.face_area[i].start_y = 0;
		ae1_fd->cur_info.face_area[i].end_x = 0;
		ae1_fd->cur_info.face_area[i].end_y = 0;
		ae1_fd->cur_info.face_area[i].pose = -1;
	}
	for (i = 0; i != 1024; i++) {
		//ae1_fd->pre_info.rect[i] = 0;
		ae1_fd->cur_info.rect[i] = 0;
	}
	return rtn;
}

static cmr_s32 _fd_info_pre_set(struct ae_ctrl_cxt *cxt)
{
	cmr_s32 rtn = AE_SUCCESS;
	struct ae1_fd_param *ae1_fd = NULL;
	cmr_u32 i = 0;

	if (NULL == cxt) {
		ISP_LOGE("fail to set pre fd info");
		return AE_ERROR;
	}
	ae1_fd = &(cxt->cur_status.ae1_finfo);
	ae1_fd->update_flag = 0;	// set updata flag
	//ae1_fd->pre_info.face_num = ae1_fd->cur_info.face_num;
	ISP_LOGV("cam-id %d, face_num is %d\n", cxt->camera_id, cxt->fdae.face_info.face_num);
	//for (i = 0; i != ae1_fd->cur_info.face_num; i++) {
	for (i = 0; i < cxt->fdae.face_info.face_num; i++) {
		// save pre info
		/*
		   ae1_fd->pre_info.face_area[i].start_x = ae1_fd->cur_info.face_area[i].start_x;
		   ae1_fd->pre_info.face_area[i].start_y = ae1_fd->cur_info.face_area[i].start_y;
		   ae1_fd->pre_info.face_area[i].end_x = ae1_fd->cur_info.face_area[i].end_x;
		   ae1_fd->pre_info.face_area[i].end_y = ae1_fd->cur_info.face_area[i].end_y;
		   ae1_fd->pre_info.face_area[i].pose = ae1_fd->cur_info.face_area[i].pose;
		 */
		// reset cur_info
		ae1_fd->cur_info.face_area[i].start_x = 0;
		ae1_fd->cur_info.face_area[i].start_y = 0;
		ae1_fd->cur_info.face_area[i].end_x = 0;
		ae1_fd->cur_info.face_area[i].end_y = 0;
		ae1_fd->cur_info.face_area[i].pose = -1;
	}
	for (i = 0; i != 1024; i++) {
		//ae1_fd->pre_info.rect[i] = ae1_fd->cur_info.rect[i];
		ae1_fd->cur_info.rect[i] = 0;
	}
	return rtn;

}

static cmr_s32 _fd_info_set(struct ae_ctrl_cxt *cxt)
{
	cmr_s32 rtn = AE_SUCCESS;
	cmr_s32 i = 0;
	cmr_s32 x = 0;
	cmr_s32 y = 0;
	cmr_s32 height_tmp = 0;
	cmr_s32 width_tmp = 0;
	cmr_s32 win_w = 0;
	cmr_s32 win_h = 0;
	cmr_u32 value = 0;
	cmr_s32 f_w_shk_x = 0;	//face win shrink ratio at x
	cmr_s32 f_w_shk_y = 0;	//face win shrink ratio at y
	struct ae_fd_param *fd = NULL;
	struct ae1_fd_param *ae1_fd = NULL;

	if (NULL == cxt) {
		ISP_LOGE("fail to set fd info");
		return AE_ERROR;
	}
	win_w = cxt->monitor_unit.win_num.w;
	win_h = cxt->monitor_unit.win_num.h;
	fd = &(cxt->fdae.face_info);
	ae1_fd = &(cxt->cur_status.ae1_finfo);
	height_tmp = fd->height;
	width_tmp = fd->width;
	//ISP_LOGV("fd_ae:set update flag to 1");
	ae1_fd->update_flag = 1;	// set updata_flag
	ae1_fd->cur_info.face_num = fd->face_num;
	//ISP_LOGV("fd ae face number:%d ", ae1_fd->cur_info.face_num);
	if (0 == fd->face_num) {	// face num = 0 ,no mapping
		return AE_SUCCESS;
	}
	for (i = 0; i < fd->face_num; i++) {
		// map every face location info
		f_w_shk_x = (fd->face_area[i].rect.end_x - fd->face_area[i].rect.start_x + 1) * 40 / 200;
		f_w_shk_y = (fd->face_area[i].rect.end_y - fd->face_area[i].rect.start_y + 1) * 40 / 200;

		ae1_fd->cur_info.face_area[i].start_x = (fd->face_area[i].rect.start_x + f_w_shk_x) * win_w / width_tmp;
		ae1_fd->cur_info.face_area[i].end_x = (fd->face_area[i].rect.end_x - f_w_shk_x) * win_w / width_tmp;
		ae1_fd->cur_info.face_area[i].start_y = (fd->face_area[i].rect.start_y + f_w_shk_y) * win_h / height_tmp;
		ae1_fd->cur_info.face_area[i].end_y = (fd->face_area[i].rect.end_y - f_w_shk_y) * win_h / height_tmp;
		// set the frame face location info
		for (y = ae1_fd->cur_info.face_area[i].start_y; y <= ae1_fd->cur_info.face_area[i].end_y; y++) {
			for (x = ae1_fd->cur_info.face_area[i].start_x; x <= ae1_fd->cur_info.face_area[i].end_x; x++) {
				ae1_fd->cur_info.rect[x + y * win_w] = 1;
			}
		}
	}
	/* debug print */
/*
	 for(i = 0;i != 32;i++){
    ISP_LOGV("fd_rect:%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
    ae1_fd->cur_info.rect[i * 32 + 0],ae1_fd->cur_info.rect[i * 32 + 1],ae1_fd->cur_info.rect[i * 32 + 2],ae1_fd->cur_info.rect[i * 32 + 3],
    ae1_fd->cur_info.rect[i * 32 + 4],ae1_fd->cur_info.rect[i * 32 + 5],ae1_fd->cur_info.rect[i * 32 + 6],ae1_fd->cur_info.rect[i * 32 + 7],
    ae1_fd->cur_info.rect[i * 32 + 8],ae1_fd->cur_info.rect[i * 32 + 9],ae1_fd->cur_info.rect[i * 32 + 10],ae1_fd->cur_info.rect[i * 32 + 11],
    ae1_fd->cur_info.rect[i * 32 + 12],ae1_fd->cur_info.rect[i * 32 + 13],ae1_fd->cur_info.rect[i * 32 + 14],ae1_fd->cur_info.rect[i * 32 + 15],
    ae1_fd->cur_info.rect[i * 32 + 16],ae1_fd->cur_info.rect[i * 32 + 17],ae1_fd->cur_info.rect[i * 32 + 18],ae1_fd->cur_info.rect[i * 32 + 19],
    ae1_fd->cur_info.rect[i * 32 + 20],ae1_fd->cur_info.rect[i * 32 + 21],ae1_fd->cur_info.rect[i * 32 + 22],ae1_fd->cur_info.rect[i * 32 + 23],
    ae1_fd->cur_info.rect[i * 32 + 24],ae1_fd->cur_info.rect[i * 32 + 25],ae1_fd->cur_info.rect[i * 32 + 26],ae1_fd->cur_info.rect[i * 32 + 27],
    ae1_fd->cur_info.rect[i * 32 + 28],ae1_fd->cur_info.rect[i * 32 + 29],ae1_fd->cur_info.rect[i * 32 + 30],ae1_fd->cur_info.rect[i * 32 + 31]
    );
}
*/

	return rtn;
}

// add end
static cmr_s32 _fd_process(struct ae_ctrl_cxt *cxt, cmr_handle param)
{
	cmr_s32 rtn = AE_SUCCESS;
	struct ae_fd_param *fd = (struct ae_fd_param *)param;

	if (NULL == cxt || NULL == param) {
		ISP_LOGE("fail to process fd, cxt %p param %p", cxt, param);
		return AE_ERROR;
	}

	if (!(cxt->fdae.allow_to_work && cxt->fdae.enable)) {
		cxt->fdae.face_info.face_num = 0;
		return rtn;
	}
	ISP_LOGV("cam-id %d, fd num is %d", cxt->camera_id, fd->face_num);
	if (fd->face_num > 0) {
		memcpy(&(cxt->fdae.face_info), fd, sizeof(struct ae_fd_param));
	} else {
		cxt->fdae.face_info.face_num = 0;
		memset(&(cxt->fdae.face_info), 0, sizeof(struct ae_fd_param));
	}
	// add by matchbox for fd_ae
	_fd_info_pre_set(cxt);
	_fd_info_set(cxt);
	// add end;
	ISP_LOGV("cam-id %d, FDAE: sx %d sy %d", cxt->camera_id, fd->face_area[0].rect.start_x, fd->face_area[0].rect.start_y);
	ISP_LOGV("cam-id %d, FDAE: ex %d ey %d", cxt->camera_id, fd->face_area[0].rect.end_x, fd->face_area[0].rect.end_y);
	return rtn;
}

/*  END: FDAE related functions  */
/**************************************************************************/
#endif
static cmr_s32 _get_skip_frame_num(struct ae_ctrl_cxt *cxt, cmr_handle param, cmr_handle result)
{
	cmr_s32 rtn = AE_SUCCESS;
	cmr_s32 frame_num = 0;
	UNUSED(param);
	frame_num = cxt->monitor_unit.cfg.skip_num;

	if (result) {
		cmr_s32 *skip_num = (cmr_s32 *) result;

		*skip_num = frame_num;
	}

	ISP_LOGV("skip frame num=%d", frame_num);
	return rtn;
}

static cmr_s32 _get_normal_info(struct ae_ctrl_cxt *cxt, cmr_handle result)
{
	cmr_s32 rtn = AE_SUCCESS;
	struct ae_normal_info *info_ptr = (struct ae_normal_info *)result;
	info_ptr->exposure = cxt->cur_result.wts.exposure_time;
	info_ptr->fps = cxt->cur_result.wts.cur_fps;
	info_ptr->stable = cxt->cur_result.wts.stable;
	return rtn;
}

static cmr_s32 _set_sensor_exp_time(struct ae_ctrl_cxt *cxt, cmr_handle param)
{
	cmr_s32 rtn = AE_SUCCESS;
	cmr_u32 exp_time = *(cmr_u32 *) param;
	cmr_u32 line_time = cxt->snr_info.line_time / SENSOR_LINETIME_BASE;

	if (AE_STATE_LOCKED == cxt->cur_status.settings.lock_ae) {
		cxt->cur_status.settings.exp_line = exp_time * 10 / line_time;
	}
	return rtn;
}

static cmr_s32 _set_sensor_sensitivity(struct ae_ctrl_cxt *cxt, cmr_handle param)
{
	cmr_s32 rtn = AE_SUCCESS;
	cmr_u32 sensitivity = *(cmr_u32 *) param;

	if (AE_STATE_LOCKED == cxt->cur_status.settings.lock_ae) {
		cxt->cur_status.settings.gain = sensitivity * 128 / 50;
	}
	return rtn;
}

static cmr_s32 _set_pause(struct ae_ctrl_cxt *cxt)
{
	cmr_s32 ret = AE_SUCCESS;

	cxt->cur_status.settings.lock_ae = AE_STATE_LOCKED;
	if (0 == cxt->cur_status.settings.pause_cnt) {
		cxt->cur_status.settings.exp_line = 0;
		cxt->cur_status.settings.gain = 0;
		cxt->cur_status.settings.manual_mode = -1;
	}
	cxt->cur_status.settings.pause_cnt++;
	ISP_LOGV("PAUSE COUNT IS %d", cxt->cur_status.settings.pause_cnt);
	return ret;
}

static cmr_s32 _set_restore_cnt(struct ae_ctrl_cxt *cxt)
{
	cmr_s32 ret = AE_SUCCESS;

	if (!cxt->cur_status.settings.pause_cnt)
		return ret;

	if (2 > cxt->cur_status.settings.pause_cnt) {
		cxt->cur_status.settings.lock_ae = AE_STATE_NORMAL;
		cxt->cur_status.settings.pause_cnt = 0;
		cxt->cur_status.settings.manual_mode = 0;
	} else {
		cxt->cur_status.settings.pause_cnt--;
	}
	ISP_LOGV("PAUSE COUNT IS %d", cxt->cur_status.settings.pause_cnt);
	return ret;
}

static int32_t _aem_stat_preprocess(cmr_u32 *src_aem_stat, cmr_u16 *dst_aem_stat, struct ae_size aem_blk_size, struct ae_size aem_blk_num, cmr_u8 aem_shift)
{
	cmr_s32 rtn = AE_SUCCESS;
	cmr_u64 bayer_pixels = aem_blk_size.w * aem_blk_size.h / 4;
	cmr_u32 stat_blocks = aem_blk_num.w * aem_blk_num.h;
	cmr_u32 *src_r_stat = (cmr_u32*)src_aem_stat;
	cmr_u32 *src_g_stat = (cmr_u32*)src_aem_stat + stat_blocks;
	cmr_u32 *src_b_stat = (cmr_u32*)src_aem_stat + 2 * stat_blocks;
	cmr_u16 *dst_r = (cmr_u16*)dst_aem_stat;
	cmr_u16 *dst_g = (cmr_u16*)dst_aem_stat + stat_blocks;
	cmr_u16 *dst_b = (cmr_u16*)dst_aem_stat + stat_blocks * 2;
	cmr_u16 max_value = 1023;
	cmr_u64 sum = 0;
	cmr_u16 avg = 0;
	uint32_t i = 0;
	uint16_t r = 0, g = 0, b = 0;

	if (bayer_pixels < 1)
		return AE_ERROR;

	for (i = 0; i < stat_blocks; i++)
	{
/*for r channel */
		sum = *src_r_stat++;
		sum = sum << aem_shift;
		avg = sum / bayer_pixels;
		r = avg > max_value ? max_value : avg;

/*for g channel */
		sum = *src_g_stat++;
		sum = sum << aem_shift;
		avg = sum / bayer_pixels;
		g = avg > max_value ? max_value : avg;

/*for b channel */
		sum = *src_b_stat++;
		sum = sum << aem_shift;
		avg = sum / bayer_pixels;
		b = avg > max_value ? max_value : avg;

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
	struct ae_alg_calc_param *current_status = &cxt->cur_status;
	uint32_t blk_num = 0;

	/*reset flash debug information*/
	memset(&cxt->flash_debug_buf[0], 0, sizeof(cxt->flash_debug_buf));
	cxt->flash_buf_len = 0;
	memset(&cxt->flash_esti_result, 0, sizeof(cxt->flash_esti_result));/*reset result*/

	in.minExposure  = current_status->ae_table->exposure[current_status->ae_table->min_index] * current_status->line_time;
	in.maxExposure  = current_status->ae_table->exposure[current_status->ae_table->max_index] * current_status->line_time;
	in.minGain = current_status->ae_table->again[current_status->ae_table->min_index];
	in.maxGain = current_status->ae_table->again[current_status->ae_table->max_index];
	in.minCapGain = current_status->ae_table->again[current_status->ae_table->min_index];
	in.maxCapGain  = current_status->ae_table->again[current_status->ae_table->max_index];
	in.minCapExposure  = current_status->ae_table->exposure[current_status->ae_table->min_index] * current_status->line_time;
	in.maxCapExposure  = current_status->ae_table->exposure[current_status->ae_table->max_index] * current_status->line_time;
	in.aeExposure = current_status->effect_expline * current_status->line_time;
	in.aeGain = current_status->effect_gain;
	in.rGain = current_status->awb_gain.r;
	in.gGain = current_status->awb_gain.g;
	in.bGain = current_status->awb_gain.b;
	in.isFlash = 0;/*need to check the meaning*/
	in.staW = cxt->monitor_unit.win_num.w;
	in.staH = cxt->monitor_unit.win_num.h;
	blk_num = cxt->monitor_unit.win_num.w * cxt->monitor_unit.win_num.h;
	in.rSta = (cmr_u16*)&cxt->aem_stat_rgb[0];
	in.gSta = (cmr_u16*)&cxt->aem_stat_rgb[0] + blk_num;
	in.bSta = (cmr_u16*)&cxt->aem_stat_rgb[0] + 2 * blk_num;
	rtn = flash_pfStart(cxt->flash_alg_handle, &in, &out);

	current_status->settings.manual_mode = 0;
	current_status->settings.table_idx = 0;
	current_status->settings.exp_line = (cmr_u32) (out.nextExposure / cxt->cur_status.line_time + 0.5);
	current_status->settings.gain = out.nextGain;
	cxt->pre_flash_level1 = out.preflahLevel1;
	cxt->pre_flash_level2 = out.preflahLevel2;
	ISP_LOGV("ae_flash pre_b: flashbefore(%d, %d), preled_level(%d, %d), preflash(%d, %d)\n",\
		current_status->effect_expline, current_status->effect_gain,\
		out.preflahLevel1, out.preflahLevel2, current_status->settings.exp_line, out.nextGain);

	cxt->flash_last_exp_line = current_status->settings.exp_line;
	cxt->flash_last_gain = current_status->settings.gain;

	return rtn;
}

static cmr_s32 flash_estimation(struct ae_ctrl_cxt *cxt)
{
	cmr_s32 rtn = AE_SUCCESS;
	cmr_s64 cur_ev = 0;
	cmr_s64 orig_ev = 0;
	cmr_s64 ev_delta = 0;
	cmr_u32 ratio = 0;
	cmr_u32 blk_num = 0;
	struct Flash_pfOneIterationInput *in = NULL;
	struct Flash_pfOneIterationOutput out;
	struct ae_alg_calc_param *current_status = &cxt->cur_status;
	memset((void*)&out, 0, sizeof(out));

	if (1 == cxt->flash_esti_result.isEnd) {
		goto EXIT;
	}
	/* preflash climbing time*/
	if (0 < cxt->pre_flash_skip) {
		cxt->pre_flash_skip--;
		goto EXIT;
	}
#if 0
	orig_ev = ((cmr_s64)cxt->flash_last_exp_line) * cxt->flash_last_gain;
	cur_ev = ((cmr_s64)current_status->effect_expline) * current_status->effect_gain;
	ev_delta = abs((cmr_s32)(cur_ev - orig_ev));
	if (orig_ev) {
		ratio = (cmr_s32)(ev_delta * 100 / orig_ev);
		if (ratio < 5) {
			ISP_LOGV("ae_flash esti: sensor param do not take effect, ratio: %d, skip\n", ratio);
			return rtn;
		}
	}
#else
	if ((cxt->flash_last_exp_line != current_status->effect_expline) ||
		(cxt->flash_last_gain != current_status->effect_gain)) {
		ISP_LOGV("ae_flash esti: sensor param do not take effect, (%d, %d)-(%d, %d)skip\n",\
					cxt->flash_last_exp_line, cxt->flash_last_gain,
					current_status->effect_expline, current_status->effect_gain);
		goto EXIT;
	}
#endif
	cxt->aem_effect_delay++;
	if (cxt->aem_effect_delay == 3) {
		cxt->aem_effect_delay = 0;

	blk_num = cxt->monitor_unit.win_num.w * cxt->monitor_unit.win_num.h;
	in = &cxt->flash_esti_input;
	in->aeExposure = current_status->effect_expline * current_status->line_time;
	in->aeGain = current_status->effect_gain;
	in->staW = cxt->monitor_unit.win_num.w;
	in->staH = cxt->monitor_unit.win_num.h;
	in->isFlash = 1;/*need to check the meaning*/
	memcpy((cmr_handle*)&in->rSta[0], (cmr_u16*)&cxt->aem_stat_rgb[0], sizeof(in->rSta));
	memcpy((cmr_handle*)&in->gSta[0], ((cmr_u16*)&cxt->aem_stat_rgb[0] + blk_num), sizeof(in->gSta));
	memcpy((cmr_handle*)&in->bSta[0], ((cmr_u16*)&cxt->aem_stat_rgb[0] + 2 * blk_num), sizeof(in->bSta));

	flash_pfOneIteration(cxt->flash_alg_handle, in, &out);

	current_status->settings.manual_mode = 0;
	current_status->settings.table_idx = 0;
	current_status->settings.exp_line = (cmr_u32) (out.nextExposure / cxt->cur_status.line_time + 0.5);
	current_status->settings.gain = out.nextGain;
	cxt->flash_esti_result.isEnd = 0;
	ISP_LOGV("ae_flash esti: doing %d, %d", cxt->cur_status.settings.exp_line, cxt->cur_status.settings.gain);

	if (1 == out.isEnd) {
		/*save flash debug information*/
		memset(&cxt->flash_debug_buf[0], 0, sizeof(cxt->flash_debug_buf));
		cxt->flash_buf_len  =0;
		memcpy((cmr_handle)&cxt->flash_debug_buf[0], out.debugBuffer, out.debugSize);
		cxt->flash_buf_len = out.debugSize;

		ISP_LOGV("ae_flash esti: isEnd:%d, cap(%d, %d), led(%d, %d), rgb(%d, %d, %d)\n",\
					out.isEnd, (cmr_u32) (out.captureExposure / cxt->cur_status.line_time + 0.5),\
					out.captureGain, out.captureFlahLevel1, out.captureFlahLevel2,\
					out.captureRGain, out.captureGGain, out.captureBGain);

		/*save the flash estimation results*/
		cxt->flash_esti_result = out;
	}

	cxt->flash_last_exp_line  = current_status->settings.exp_line;
	cxt->flash_last_gain =  current_status->settings.gain;
	}
EXIT:
	current_status->settings.manual_mode = 0;
	current_status->settings.table_idx = 0;
	current_status->settings.exp_line = cxt->flash_last_exp_line;
	current_status->settings.gain = cxt->flash_last_gain;

	return rtn;
}

static cmr_s32 flash_finish(struct ae_ctrl_cxt *cxt)
{
	cmr_s32 rtn = AE_SUCCESS;

	rtn = flash_pfEnd(cxt->flash_alg_handle);

	/*reset flash control status*/
	cxt->pre_flash_skip = 0;
	cxt->flash_last_exp_line = 0;
	cxt->flash_last_gain = 0;
	memset(&cxt->flash_esti_input, 0, sizeof(cxt->flash_esti_input));
	memset(&cxt->flash_esti_result, 0, sizeof(cxt->flash_esti_result));
	//memset(&cxt->flash_debug_buf[0], 0, sizeof(cxt->flash_debug_buf));
	//cxt->flash_buf_len = 0;

	return rtn;
}

static cmr_s32 ae_set_flash_charge(struct ae_ctrl_cxt *cxt, enum ae_flash_type flash_type)
{
	cmr_s32 rtn = AE_SUCCESS;
	cmr_u32 flash_level1 = 0;
	cmr_u32 flash_level2 = 0;
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

	if (!flash_level1) {
		cxt->ae_leds_ctrl.led0_ctrl = 0;//flash lib level 0: flashoff,
	} else {
		cxt->ae_leds_ctrl.led0_ctrl = 1;
		cfg.led_idx = 1;
		element.index = flash_level1 - 1;//flash lib level 1~32 corresponding flash drv level 0~127
		cxt->isp_ops.flash_set_charge(cxt->isp_ops.isp_handler, &cfg, &element);
	}

	if (!flash_level2) {
		cxt->ae_leds_ctrl.led1_ctrl = 0;
	} else {
		cxt->ae_leds_ctrl.led1_ctrl = 1;
		cfg.led_idx = 2;
		element.index = flash_level2 - 1;
		cxt->isp_ops.flash_set_charge(cxt->isp_ops.isp_handler, &cfg, &element);
	}

out:
	return rtn;
}

static cmr_s32 _ae_pre_process(struct ae_ctrl_cxt *cxt)
{
	cmr_s32 rtn = AE_SUCCESS;
	struct ae_alg_calc_param *current_status = &cxt->cur_status;

	if ((AE_WORK_MODE_VIDEO == current_status->settings.work_mode)
		&& (AE_VIDEO_DECR_FPS_DARK_ENV_THRD > cxt->sync_cur_result.cur_bv)) {
			/*only adjust the fps to [15, 15] in dark environment during video mode*/
			current_status->settings.max_fps  = cxt->fps_range.min;
			current_status->settings.min_fps  = cxt->fps_range.min;
			ISP_LOGV("fps: %d, %d just fix to 15fps in dark during video, bv%d\n",
				cxt->fps_range.min, cxt->fps_range.max, cxt->sync_cur_result.cur_bv);
	} else {
		current_status->settings.max_fps = cxt->fps_range.max;
		current_status->settings.min_fps = cxt->fps_range.min;
	}

	if (0 < cxt->cur_status.settings.flash) {
		ISP_LOGI("ae_flash: flicker lock to %d in flash: %d\n", cxt->cur_flicker, current_status->settings.flash);
		current_status->settings.flicker = cxt->cur_flicker;
	}

	if (0 != cxt->flash_ver) {
		if ((FLASH_PRE_BEFORE == current_status->settings.flash) ||
			(FLASH_PRE == current_status->settings.flash)) {
			rtn = _aem_stat_preprocess((cmr_u32*)&cxt->sync_aem[0],
									   (cmr_u16*)&cxt->aem_stat_rgb[0],
									    cxt->monitor_unit.win_size,
									    cxt->monitor_unit.win_num,
									    current_status->monitor_shift);
		}

		if ((FLASH_PRE_BEFORE == current_status->settings.flash) &&
			!cxt->send_once[0]) {
			rtn = flash_pre_start(cxt);
		}

		if (FLASH_PRE == current_status->settings.flash) {
			rtn = flash_estimation(cxt);
		}

		if ((FLASH_MAIN_BEFORE == current_status->settings.flash) ||
			(FLASH_MAIN == current_status->settings.flash)) {
			if (cxt->flash_esti_result.isEnd) {
				current_status->settings.manual_mode = 0;
				current_status->settings.table_idx = 0;
				current_status->settings.exp_line =
					(cmr_u32) (cxt->flash_esti_result.captureExposure  / current_status->line_time + 0.5);
				current_status->settings.gain = cxt->flash_esti_result.captureGain;
			} else {
				ISP_LOGW("ae_flash estimation does not work well");
			}

			ISP_LOGV("ae_flash main_b prinf: %.f, %d, %d, %d\n",\
				cxt->flash_esti_result.captureExposure,\
				current_status->settings.exp_line,\
				current_status->settings.gain,\
				current_status->line_time);
		}
	}

	return rtn;
}

static cmr_s32 _ae_post_process(struct ae_ctrl_cxt *cxt)
{
	cmr_s32 rtn = AE_SUCCESS;
	struct ae_alg_calc_param *current_status = &cxt->sync_cur_status;

	/* for flash algorithm 0 */
	if (0 == cxt->flash_ver) {
		if (FLASH_PRE_BEFORE_RECEIVE == cxt->cur_result.flash_status &&
			FLASH_PRE_BEFORE == current_status->settings.flash) {
			ISP_LOGI("ae_flash0_status shake_1");
			if (0 == cxt->send_once[0]) {
				cxt->send_once[0]++;
#ifdef CONFIG_CAMERA_DUAL_SYNC
				(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, AE_CB_CONVERGED, NULL);
#else
				(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, AE_CB_CONVERGED);
#endif
				ISP_LOGI("ae_flash0_callback do-pre-open!\r\n");
			}
		}

		if (FLASH_PRE_RECEIVE == cxt->cur_result.flash_status &&
			FLASH_PRE == current_status->settings.flash) {
			ISP_LOGI("ae_flash0_status shake_2 %d %d %d %d", cxt->cur_result.wts.stable,
				cxt->cur_result.cur_lum, cxt->cur_result.flash_effect, cxt->send_once[3]);
			cxt->send_once[3]++;
			if (cxt->cur_result.wts.stable || cxt->send_once[3] > AE_FLASH_CALC_TIMES) {
				if (0 == cxt->send_once[1]) {
					cxt->send_once[1]++;
#ifdef CONFIG_CAMERA_DUAL_SYNC
					(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, AE_CB_CONVERGED, NULL);
#else
					(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, AE_CB_CONVERGED);
#endif
					ISP_LOGI("ae_flash0_callback do-pre-close!\r\n");
				}
				cxt->flash_effect = cxt->cur_result.flash_effect;
			}
			cxt->send_once[0] = 0;
		}

		if (FLASH_PRE_AFTER_RECEIVE == cxt->cur_result.flash_status &&
			FLASH_PRE_AFTER == current_status->settings.flash) {
			ISP_LOGI("ae_flash0_status shake_3 %d", cxt->cur_result.flash_effect);
			cxt->cur_status.settings.flash = FLASH_NONE;
			cxt->send_once[3] = 0;
			cxt->send_once[1] = 0;
		}

		if (FLASH_MAIN_BEFORE_RECEIVE == cxt->cur_result.flash_status &&
			FLASH_MAIN_BEFORE == current_status->settings.flash) {
			ISP_LOGI("ae_flash0_status shake_4 %d %d", cxt->cur_result.wts.stable, cxt->cur_result.cur_lum);
			if (cxt->cur_result.wts.stable) {
				if (1 == cxt->send_once[2]) {
#ifdef CONFIG_CAMERA_DUAL_SYNC
					(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, AE_CB_CONVERGED, NULL);
#else
					(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, AE_CB_CONVERGED);
#endif
					ISP_LOGI("ae_flash0_callback do-main-flash!\r\n");
				} else if (4 == cxt->send_once[2]) {
#ifdef  CONFIG_CAMERA_DUAL_SYNC
					(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, AE_CB_CONVERGED, NULL);
#else
					(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, AE_CB_CONVERGED);
#endif
					ISP_LOGI("ae_flash0_callback do-capture!\r\n");
				} else {
					ISP_LOGI("ae_flash0 wait-capture!\r\n");
				}
				cxt->send_once[2]++;
			}
		}

		if (FLASH_MAIN_AFTER_RECEIVE == cxt->cur_result.flash_status &&
			FLASH_MAIN_AFTER == current_status->settings.flash) {
			ISP_LOGI("ae_flash0_status shake_6");
			cxt->cur_status.settings.flash = FLASH_NONE;
			cxt->send_once[0] = cxt->send_once[1] = cxt->send_once[2] = cxt->send_once[3] = 0;
		}
	}else {
	/* for new flash algorithm (flash algorithm1, dual flash) */
		if (FLASH_PRE_BEFORE_RECEIVE == cxt->cur_result.flash_status &&
			FLASH_PRE_BEFORE == current_status->settings.flash) {
			cxt->send_once[0]++;
			ISP_LOGI("ae_flash1_status shake_1");
			if (3 == cxt->send_once[0]) {
				ISP_LOGI("ae_flash p: led level: %d, %d\n", cxt->pre_flash_level1, cxt->pre_flash_level2);
				rtn = ae_set_flash_charge(cxt, AE_FLASH_TYPE_PREFLASH);

#ifdef CONFIG_CAMERA_DUAL_SYNC
				(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, AE_CB_CONVERGED, NULL);
#else
				(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, AE_CB_CONVERGED);
#endif
				cxt->cur_result.flash_status = FLASH_NONE;/*flash status reset*/
				ISP_LOGI("ae_flash1_callback do-pre-open!\r\n");
			}
		}

		if (FLASH_PRE_RECEIVE == cxt->cur_result.flash_status &&
			FLASH_PRE == current_status->settings.flash) {
			ISP_LOGI("ae_flash1_status shake_2 %d %d %d", cxt->cur_result.wts.stable,
				cxt->cur_result.cur_lum, cxt->cur_result.flash_effect);
			cxt->send_once[3]++;//prevent flash_pfOneIteration time out
			if (cxt->flash_esti_result.isEnd || cxt->send_once[3] > AE_FLASH_CALC_TIMES) {
				if (0 == cxt->send_once[1]) {
					cxt->send_once[1]++;
					cxt->send_once[3] = 0;
#ifdef CONFIG_CAMERA_DUAL_SYNC
					(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, AE_CB_CONVERGED, NULL);
#else
					(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, AE_CB_CONVERGED);
#endif
					ISP_LOGI("ae_flash1_callback do-pre-close!\r\n");
					cxt->cur_result.flash_status = FLASH_NONE;/*flash status reset*/
				}
			}
			cxt->send_once[0] = 0;
		}

		if (FLASH_PRE_AFTER_RECEIVE == cxt->cur_result.flash_status &&
			FLASH_PRE_AFTER == current_status->settings.flash) {
			ISP_LOGI("ae_flash1_status shake_3 %d", cxt->cur_result.flash_effect);
			cxt->cur_status.settings.flash = FLASH_NONE;/*flash status reset*/
			cxt->send_once[1] = 0;
		}

		if (FLASH_MAIN_BEFORE_RECEIVE == cxt->cur_result.flash_status &&
			FLASH_MAIN_BEFORE == current_status->settings.flash) {
			ISP_LOGI("ae_flash1_status shake_4 %d %d", cxt->cur_result.wts.stable, cxt->cur_result.cur_lum);
			//cxt->cur_status.settings.lock_ae = AE_STATE_PAUSE;/**/
			if (cxt->cur_result.wts.stable) {
				if (1 == cxt->send_once[2]) {
					ISP_LOGI("ae_flash m: led level: %d, %d\n", cxt->flash_esti_result.captureFlahLevel1,
								cxt->flash_esti_result.captureFlahLevel2);
					rtn = ae_set_flash_charge(cxt, AE_FLASH_TYPE_MAIN);
#ifdef  CONFIG_CAMERA_DUAL_SYNC
					(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, AE_CB_CONVERGED, NULL);
#else
					(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, AE_CB_CONVERGED);
#endif
					ISP_LOGI("ae_flash1_callback do-main-flash!\r\n");
				} else if (5 == cxt->send_once[2]) {
#ifdef  CONFIG_CAMERA_DUAL_SYNC
					(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, AE_CB_CONVERGED, NULL);
#else
					(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, AE_CB_CONVERGED);
#endif
					cxt->cur_result.flash_status = FLASH_NONE;/*flash status reset*/
					ISP_LOGI("ae_flash1_callback do-capture!\r\n");
				} else {
					ISP_LOGI("ae_flash1 wait-capture!\r\n");
				}
				cxt->send_once[2]++;
			}
		}

		if (FLASH_MAIN_AFTER_RECEIVE == cxt->cur_result.flash_status &&
			FLASH_MAIN_AFTER == current_status->settings.flash) {
			ISP_LOGI("ae_flash1_status shake_6");
			cxt->cur_status.settings.flash = FLASH_NONE;/*flash status reset*/
			cxt->send_once[0] = cxt->send_once[1] = cxt->send_once[2] = cxt->send_once[3] = 0;
			if (0 != cxt->flash_ver) {
				flash_finish(cxt);
			}
		}
	}

	return AE_SUCCESS;
}

static cmr_s32 ae_set_magic_tag(struct debug_ae_param *param_ptr)
{
	cmr_s32 rtn = AE_SUCCESS;

	cmr_u32 len = 0;
	len = strlen(AE_MAGIC_TAG);
	if (len >= sizeof(param_ptr->magic)) {
		ISP_LOGE("fail to set magic tag\n");
		return AE_ERROR;
	}
	memcpy((cmr_handle)&param_ptr->magic[0], AE_MAGIC_TAG, len);

	return rtn;
}

// wanghao @2015-12-22
static cmr_s32 ae_get_debug_info(struct ae_ctrl_cxt *cxt, cmr_handle result)
{
	cmr_s32 rtn = AE_SUCCESS;
	cmr_u32 len = 0;

#if 0
	struct debug_ae_param *param = (struct debug_ae_param *)result;
	rtn = ae_set_magic_tag(param);
	if (AE_SUCCESS != rtn) {
		goto  DEBUG_INFO_EXIT;
	}
	param->version = AE_DEBUG_VERSION_ID;
	memcpy(param->alg_id, cxt->alg_id, sizeof(param->alg_id));
	param->lock_status = cxt->sync_cur_status.settings.lock_ae;
	param->FD_AE_status = cxt->fdae.enable;
	param->cur_lum = cxt->sync_cur_result.cur_lum;
	param->target_lum = cxt->sync_cur_result.target_lum;
	param->target_zone = cxt->sync_cur_result.target_zone;
	param->target_zone_ori = cxt->sync_cur_result.target_zone_ori;
	param->max_index = cxt->sync_cur_status.ae_table->max_index;
	param->cur_index = cxt->sync_cur_result.wts.cur_index;
	param->min_index = cxt->sync_cur_status.ae_table->min_index;
	param->max_fps = cxt->sync_cur_status.settings.max_fps;
	param->min_fps = cxt->sync_cur_status.settings.min_fps;
	param->cur_fps = cxt->sync_cur_result.wts.cur_fps;
	param->cur_ev = cxt->sync_cur_status.settings.ev_index;
	param->cur_bv = cxt->sync_cur_result.cur_bv;
	param->line_time = cxt->sync_cur_status.line_time;
	param->cur_exp_line = cxt->sync_cur_result.wts.cur_exp_line;
	param->exposure_time = cxt->sync_cur_result.wts.exposure_time;
	param->cur_dummy = cxt->sync_cur_result.wts.cur_dummy;
	param->frame_line = cxt->sync_cur_result.wts.cur_exp_line + cxt->sync_cur_result.wts.cur_dummy;
	param->cur_again = cxt->sync_cur_result.wts.cur_again;
	param->cur_dgain = cxt->sync_cur_result.wts.cur_dgain;
	param->cur_iso = cxt->sync_cur_status.settings.iso;
	param->cur_flicker = cxt->sync_cur_status.settings.flicker;
	param->cur_scene = cxt->sync_cur_status.settings.scene_mode;
	param->flash_effect = cxt->sync_cur_result.flash_effect;
	param->is_stab = cxt->sync_cur_result.wts.stable;
	param->cur_work_mode = cxt->sync_cur_status.settings.work_mode;
	param->cur_metering_mode = (cmr_u32) cxt->sync_cur_status.settings.metering_mode;
	// metering_mode
	/* Bethany
	* add some  touch  info  to  debug  info */
	param->TC_AE_status = cxt->sync_cur_status.settings.touch_tuning_enable;
	/* Bethany
	*add some  touch  info  to  debug  info*/
	param->TC_cur_lum = cxt->sync_cur_result.ptc->result_touch_ae.touch_roi_lum;
	/* Bethany
	*add some touch info to debug info
	*/
		param->TC_target_offset = cxt->sync_cur_result.ptc->result_touch_ae.tar_offset;
	param->mulaes_target_offset = cxt->sync_cur_result.pmulaes->result_mulaes.tar_offset;
	param->region_target_offset = cxt->sync_cur_result.pregion->result_region.tar_offset_u + cxt->sync_cur_result.pregion->result_region.tar_offset_d;
	param->flat_target_offset = cxt->sync_cur_result.pflat->result_flat.tar_offset;
	// memcpy((cmr_handle*)&param->histogram[0],
	// (cmr_handle*)cxt->sync_cur_result.histogram, sizeof(cmr_u32)
	// * 256);
	memcpy((cmr_handle) & param->r_stat_info[0], (cmr_handle) & cxt->sync_aem[0], sizeof(cmr_u32) * 1024);
	memcpy((cmr_handle) & param->g_stat_info[0], (cmr_handle) & cxt->sync_aem[1024], sizeof(cmr_u32) * 1024);
	memcpy((cmr_handle) & param->b_stat_info[0], (cmr_handle) & cxt->sync_aem[2048], sizeof(cmr_u32) * 1024);
	len = sizeof(*(cxt->sync_cur_result.pregion)) + sizeof(*(cxt->sync_cur_result.pflat)) + sizeof(*(cxt->sync_cur_result.pmulaes));
	/*
	* ISP_LOGD("total_len %d", sizeof(struct debug_ae_param));
	* ISP_LOGD("total_i-metering_len %d prl %d pfl %d pml
	* %d\r\n", len, sizeof(*(cxt->sync_cur_result.pregion)),
	* sizeof(*(cxt->sync_cur_result.pflat)),
	* sizeof(*(cxt->sync_cur_result.pmulaes)));
	* ISP_LOGD("total_i-metering_region %d\r\n",
	* sizeof(cxt->sync_cur_result.pregion->in_region));
	*/
	if(len < sizeof(param->reserved)) {
		memcpy((cmr_handle)&param->reserved[0], (cmr_handle) cxt->sync_cur_result.pregion, sizeof(*(cxt->sync_cur_result.pregion)));
		len = sizeof(*(cxt->sync_cur_result.pregion)) / sizeof(cmr_u32);
		memcpy((cmr_handle)&param->reserved[len], (cmr_handle)cxt->sync_cur_result.pflat, sizeof(*(cxt->sync_cur_result.pflat)));
		len = len + (sizeof(*(cxt->sync_cur_result.pflat)) / sizeof(cmr_u32));
		memcpy((cmr_handle)&param->reserved[len], (cmr_handle) cxt->sync_cur_result.pmulaes, sizeof(*(cxt->sync_cur_result.pmulaes)));
		}
	DEBUG_INFO_EXIT:
#else
	struct ae_debug_info_packet_in debug_info_in;
	struct ae_debug_info_packet_out debug_info_out;
	char *alg_id_ptr = NULL;
	struct tg_ae_ctrl_alc_log *debug_info_result = (struct tg_ae_ctrl_alc_log *)result;

	memset((cmr_handle) & debug_info_in, 0, sizeof(struct ae_debug_info_packet_in));
	memset((cmr_handle) & debug_info_out, 0, sizeof(struct ae_debug_info_packet_out));
	alg_id_ptr = ae_debug_info_get_lib_version();
	debug_info_in.aem_stats = (cmr_handle) cxt->sync_aem;
	debug_info_in.alg_status = (cmr_handle) & cxt->sync_cur_status;
	debug_info_in.alg_results = (cmr_handle) & cxt->sync_cur_result;
	debug_info_in.packet_buf = (cmr_handle) & cxt->debug_info_buf[0];
	memcpy((cmr_handle) & debug_info_in.id[0], alg_id_ptr, sizeof(debug_info_in.id));

	rtn = ae_debug_info_packet((cmr_handle) & debug_info_in, (cmr_handle) & debug_info_out);
	/*add flash debug information*/
	memcpy((cmr_handle)&cxt->debug_info_buf[debug_info_out.size], (cmr_handle)&cxt->flash_debug_buf[0], cxt->flash_buf_len);

	debug_info_result->log = (cmr_u8 *) debug_info_in.packet_buf;
	debug_info_result->size = debug_info_out.size + cxt->flash_buf_len;
#endif

	return rtn;
}

// wanghao @2015-12-23
static cmr_s32 ae_get_debug_info_for_display(struct ae_ctrl_cxt *cxt, cmr_handle result)
{
	cmr_s32 rtn = AE_SUCCESS;
	UNUSED(cxt);
	UNUSED(result);
#if 0
	struct debug_ae_display_param *emParam = (struct debug_ae_display_param *)result;
	emParam->version = AE_DEBUG_VERSION_ID;
	emParam->lock_status = cxt->cur_status.settings.lock_ae;
	emParam->FD_AE_status = cxt->fdae.enable;
	emParam->cur_lum = cxt->cur_result.cur_lum;
	emParam->target_lum = cxt->cur_result.target_lum;
	emParam->target_zone = cxt->cur_param->target_lum_zone;;
	emParam->max_index = cxt->cur_status.ae_table->max_index;
	emParam->cur_index = cxt->cur_result.wts.cur_index;
	emParam->min_index = cxt->cur_status.ae_table->min_index;
	emParam->max_fps = cxt->cur_status.settings.max_fps;
	emParam->min_fps = cxt->cur_status.settings.max_fps;
	emParam->exposure_time = cxt->cur_result.wts.exposure_time;
	emParam->cur_ev = cxt->cur_status.settings.ev_index;
	emParam->cur_exp_line = cxt->cur_result.wts.cur_exp_line;
	emParam->cur_dummy = cxt->cur_result.wts.cur_dummy;
	emParam->cur_again = cxt->cur_result.wts.cur_again;
	emParam->cur_dgain = cxt->cur_result.wts.cur_dgain;
	emParam->cur_iso = cxt->cur_status.settings.iso;
	emParam->cur_flicker = cxt->cur_status.settings.flicker;
	emParam->cur_scene = cxt->cur_status.settings.scene_mode;
	emParam->flash_effect = cxt->cur_status.settings.flash_ration;
	emParam->is_stab = cxt->cur_result.wts.stable;
	emParam->line_time = cxt->cur_status.line_time;
	emParam->frame_line = cxt->cur_result.wts.cur_exp_line + cxt->cur_result.wts.cur_dummy;
	emParam->cur_work_mode = cxt->cur_status.settings.work_mode;
	emParam->cur_bv = cxt->sync_cur_result.cur_bv;
	emParam->cur_fps = cxt->cur_result.wts.cur_fps;

	emParam->cur_metering_mode = (cmr_u32) cxt->cur_status.settings.metering_mode;	// metering_mode
#endif
	return rtn;
}

static cmr_s32 make_isp_result(struct ae_alg_calc_result *alg_rt, struct ae_calc_out *result)
{
	cmr_s32 rtn = AE_SUCCESS;

	result->cur_lum = alg_rt->cur_lum;
	result->cur_again = alg_rt->wts.cur_again;
	result->cur_exp_line = alg_rt->wts.cur_exp_line;
	result->line_time = alg_rt->wts.exposure_time / alg_rt->wts.cur_exp_line;
	result->is_stab = alg_rt->wts.stable;
	result->target_lum = alg_rt->target_lum;
	result->target_lum_ori = alg_rt->target_lum_ori;
	result->flag4idx = alg_rt->flag4idx;

	return rtn;
}

cmr_handle ae_sprd_init(cmr_handle param, cmr_handle in_param)
{
	cmr_s32 rtn = AE_SUCCESS;
	cmr_s32 i = 0;
	char ae_property[PROPERTY_VALUE_MAX];
	cmr_handle seq_handle = NULL;
	struct ae_ctrl_cxt *cxt = NULL;
	struct ae_init_out *ae_init_out = NULL;
	struct ae_misc_init_in misc_init_in = { 0, 0, 0, NULL, 0 };
	struct ae_misc_init_out misc_init_out = { 0, {0} };
	struct s_q_open_param s_q_param = {0, 0, 0};
	struct ae_set_work_param work_param;
	struct ae_init_in *init_param = NULL;
	struct Flash_initInput flash_in;
	struct Flash_initOut flash_out;

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

#ifdef CONFIG_CAMERA_DUAL_SYNC
	cxt->ae_role = init_param->ae_role;
	cxt->sensor_role = init_param->sensor_role;
	cxt->is_multi_mode = init_param->is_multi_mode;
	cxt->ptr_isp_br_ioctrl = init_param->ptr_isp_br_ioctrl;
	ISP_LOGI("is_multi_mode=%d\n", init_param->is_multi_mode);
#endif

	work_param.mode = AE_WORK_MODE_COMMON;
	work_param.fly_eb = 1;
	work_param.resolution_info = init_param->resolution_info;

	/*ISP_LOGV("resol w %d h %d lt %d",
	   work_param.resolution_info.frame_size.w,
	   work_param.resolution_info.frame_size.h,
	   work_param.resolution_info.line_time); */

	cxt->cur_status.ae_initial = AE_PARAM_INIT;
	rtn = _set_ae_param(cxt, init_param, &work_param, AE_PARAM_INIT);
	/* init fd_info add by matchbox for fd_ae */
	//rtn = _fd_info_init(cxt);

	s_q_param.exp_valid_num = cxt->exp_skip_num + 1 + AE_UPDAET_BASE_OFFSET;
	s_q_param.sensor_gain_valid_num = cxt->gain_skip_num +  1 + AE_UPDAET_BASE_OFFSET;
	s_q_param.isp_gain_valid_num = 1 + AE_UPDAET_BASE_OFFSET;
	cxt->seq_handle = s_q_open(&s_q_param);

	/* HJW_S: dual flash algorithm init */
      for(i=0 ; i< 20; i++){
		flash_in.ctTab[i] = init_param->ct_table.ct[i];
		flash_in.ctTabRg[i] = init_param->ct_table.rg[i];
		cxt->ctTab[i] = init_param->ct_table.ct[i];
		cxt->ctTabRg[i] = init_param->ct_table.rg[i];
        }

	flash_in.debug_level = 1;/*it will be removed in the future, and get it from dual flash tuning parameters*/
	flash_in.tune_info = NULL;/*it will be removed in the future, and get it from dual flash tuning parameters*/
	flash_in.statH  = cxt->monitor_unit.win_num.h;
	flash_in.statW = cxt->monitor_unit.win_num.w;
	cxt->flash_alg_handle = flash_init(&flash_in, &flash_out);
	cxt->flash_ver  = flash_out.version;
	cxt->flash_ver = 1;//temp code for dual flash, remove later, 1 for new dualflash, 0 for old flash
	ae_init_out->flash_ver = cxt->flash_ver;
	/*HJW_E*/

	cxt->bypass = init_param->has_force_bypass;
	if (init_param->has_force_bypass) {
		cxt->cur_param->touch_info.enable = 0;
		cxt->cur_param->face_param.face_tuning_enable = 0;
	}
	cxt->debug_enable = _is_ae_mlog(cxt);
	cxt->cur_status.mlog_en = cxt->debug_enable;
	misc_init_in.alg_id = cxt->cur_status.alg_id;
	misc_init_in.flash_version = cxt->flash_ver;
	misc_init_in.start_index = cxt->cur_status.start_index;
	misc_init_in.param_ptr = &cxt->cur_status;
	misc_init_in.size = sizeof(cxt->cur_status);
	cxt->misc_handle = ae_misc_init(&misc_init_in, &misc_init_out);
	memcpy(cxt->alg_id, misc_init_out.alg_id, sizeof(cxt->alg_id));

	pthread_mutex_init(&cxt->data_sync_lock, NULL);

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
	cxt->sync_cur_result.cur_bv = 500;

	memset((cmr_handle) & ae_property, 0, sizeof(ae_property));
	property_get("persist.sys.isp.ae.manual", ae_property, "off");
	//ISP_LOGV("persist.sys.isp.ae.manual: %s", ae_property);
	if (!strcmp("on", ae_property)) {
		cxt->manual_ae_on = 1;
	} else {
		cxt->manual_ae_on = 0;
	}
	ISP_LOGI("done, cam-id %d", cxt->camera_id);
	return (cmr_handle) cxt;
ERR_EXIT:
	if (NULL != cxt) {
		free(cxt);
		cxt = NULL;
	}
	ISP_LOGE("fail to init ae %d", rtn);
	return NULL;
}

static cmr_s32 _get_flicker_switch_flag(struct ae_ctrl_cxt *cxt, cmr_handle in_param)
{
	cmr_s32 rtn = AE_SUCCESS;
	cmr_u32 cur_exp = 0;
	cmr_u32 *flag = (cmr_u32 *) in_param;

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
	return rtn;
}

//============================================================
//input and output
//name: property name
//n: contains n int in property values string
//data: property values string to int data
//return: number of valid data, max: 10
static int get_prop_multi(const char* name, int n, int* data)
{
#define MAX_PROP_DATA_NUM 10
	int i;
	char str[300] = "";
	char strTemp[300]="";
	for (i = 0; i < n; i++)
		data[i] = 0;

	property_get(name, str, "");

	int ret = -1;
	if (strlen(str) == 0)
		return 0;

#define FIND_NUM 1
#define FIND_ZERO 0
	int state = FIND_NUM;
	int charCnt = 0;
	int dataCnt = 0;
	ret = 0;
	int len;
	len = strlen(str);
	for (i = 0; i < len; i++)
	{
		if (state == FIND_NUM)
		{
			if (str[i] == ' ' || str[i] == '\t')
			{

			}
			else
			{
				strTemp[charCnt] = str[i];
				charCnt++;
				state = FIND_ZERO;
			}
		}
		else
		{
			if (str[i] == ' ' || str[i] == '\t')
			{
				state = FIND_NUM;
				if (dataCnt < MAX_PROP_DATA_NUM)
				{
					strTemp[charCnt] = 0;
					data[dataCnt] = atoi(strTemp);
					dataCnt++;
					charCnt = 0;
				}
			}
			else
			{
				strTemp[charCnt] = str[i];
				charCnt++;
			}
		}
	}
	if (charCnt!=0)
	{
		if (dataCnt < MAX_PROP_DATA_NUM)
		{
			strTemp[charCnt] = 0;
			data[dataCnt] = atoi(strTemp);
			dataCnt++;
		}
	}
	ret = dataCnt;
	return ret;
}


static int _aem_stat_preprocess2(cmr_u32 *src_aem_stat,
                                 float *dst_r,
                                 float *dst_g,
                                 float *dst_b,
                                 struct ae_size aem_blk_size,
                                 struct ae_size aem_blk_num,
                                 cmr_u8 aem_shift)
{
	cmr_s32 rtn = AE_SUCCESS;
	cmr_u64 bayer_pixels = aem_blk_size.w * aem_blk_size.h / 4;
	cmr_u32 stat_blocks = aem_blk_num.w * aem_blk_num.h;
	cmr_u32 *src_r_stat = (cmr_u32*)src_aem_stat;
	cmr_u32 *src_g_stat = (cmr_u32*)src_aem_stat + stat_blocks;
	cmr_u32 *src_b_stat = (cmr_u32*)src_aem_stat + 2 * stat_blocks;
	//cmr_u16 *dst_r = (float*)dst_aem_stat;
	//cmr_u16 *dst_g = (float*)dst_aem_stat + stat_blocks;
	//cmr_u16 *dst_b = (float*)dst_aem_stat + stat_blocks * 2;
	double max_value = 1023;
	double sum = 0;
	double avg = 0;
	uint32_t i = 0;
	double r = 0, g = 0, b = 0;

	double mul_shift=1;
	for (i = 0; i < aem_shift; i++)
		mul_shift = mul_shift * 2;

	if (bayer_pixels < 1)
		return AE_ERROR;

	for (i = 0; i < stat_blocks; i++)
	{
/*for r channel */
		sum = *src_r_stat++;
		sum = sum * mul_shift;
		avg = sum / bayer_pixels;
		r = avg > max_value ? max_value : avg;

/*for g channel */
		sum = *src_g_stat++;
		sum = sum * mul_shift;
		avg = sum / bayer_pixels;
		g = avg > max_value ? max_value : avg;

/*for b channel */
		sum = *src_b_stat++;
		sum = sum * mul_shift;
		avg = sum / bayer_pixels;
		b = avg > max_value ? max_value : avg;

		dst_r[i] = r;
		dst_g[i] = g;
		dst_b[i] = b;
	}

	return rtn;
}
static int getCenterMean(cmr_u32 *src_aem_stat,
                          float *dst_r,
                          float *dst_g,
                          float *dst_b,
                          struct ae_size aem_blk_size,
                          struct ae_size aem_blk_num,
                          cmr_u8 aem_shift,
                          float* rmean,
                          float* gmean,
                          float* bmean
                          )
{
    int ret;
    ret = _aem_stat_preprocess2(src_aem_stat,
                                dst_r,
                                dst_g,
                                dst_b,
                                aem_blk_size,
                                aem_blk_num,
                                aem_shift);
    int i;
    int j;
    float rsum=0;
    float gsum=0;
    float bsum=0;
    for(i=13;i<19;i++)
        for(j=13;j<19;j++)
        {
            int ind;
            ind = j*32+i;
            rsum+=dst_r[ind];
            gsum+=dst_g[ind];
            bsum+=dst_b[ind];
        }
    *rmean =  rsum/36;
    *gmean =  gsum/36;
    *bmean =  bsum/36;
    return ret;
}

static void control_led(struct ae_ctrl_cxt *cxt, int onoff, int isMainflash, int led1, int led2) //0~31
{
	UNUSED(cxt);
	UNUSED(onoff);
	UNUSED(isMainflash);
	UNUSED(led1);
	UNUSED(led2);

	ISP_LOGE("control_led %d %d %d %d",onoff, isMainflash, led1, led2);
	int type;
	struct ae_flash_cfg cfg;
	struct ae_flash_element element;
	if(isMainflash==0)
		type = ISP_FLASH_TYPE_PREFLASH;
	else
		type = ISP_FLASH_TYPE_MAIN;


	if(onoff==0)
	{
		cfg.led_idx = 0;
		cfg.type = type;
		cfg.led0_enable = 0;
		cfg.led1_enable = 0;
		cxt->isp_ops.flash_ctrl(cxt->isp_ops.isp_handler, &cfg, NULL);
	}
	else
	{
		int led1_driver_ind=led1-1;
		int led2_driver_ind=led2-1;
		if(led1_driver_ind<0)
			led1_driver_ind=0;
		if(led2_driver_ind<0)
			led2_driver_ind=0;

		cfg.led_idx = 1;
		cfg.type = type;
		element.index = led1_driver_ind;
		cxt->isp_ops.flash_set_charge(cxt->isp_ops.isp_handler, &cfg, &element);

		cfg.led_idx = 2;
		cfg.type = type;
		element.index = led2_driver_ind;
		cxt->isp_ops.flash_set_charge(cxt->isp_ops.isp_handler, &cfg, &element);

		if(led1==0)
			cfg.led0_enable = 0;
		else
			cfg.led0_enable = 1;

		if(led2==0)
			cfg.led1_enable = 0;
		else
			cfg.led1_enable = 1;

		cfg.type = type;
		cxt->isp_ops.flash_ctrl(cxt->isp_ops.isp_handler, &cfg, NULL);
	}

}

static int _round(float a)
{
	if (a > 0)
		return (int)(a + 0.5);
	else
		return (int)(a - 0.5);
}


void calRgbFrameData(int isMainFlash, float* rRaw, float* gRaw, float* bRaw, float* r, float* g, float* b)
{
	float rs=0;
	float gs=0;
	float bs=0;
	int i;
	if (isMainFlash == 0)
	{
		for (i = 5; i < 15; i++)
		{
			rs += rRaw[i];
			gs += gRaw[i];
			bs += bRaw[i];
		}
		rs /= 10;
		gs /= 10;
		bs /= 10;
	}
	else
	{
		rs = (rRaw[7] + rRaw[8]) / 2;
		gs = (gRaw[7] + gRaw[8]) / 2;
		bs = (bRaw[7] + bRaw[8]) / 2;
	}
	*r = rs;
	*g = gs;
	*b = bs;
}

float calLedmA(int isMain, int led1, int led2)
{
	float i1;
	float i2;
	if (isMain)
	{
		i1 = 1500.0 * led1 / 32;
		i2 = 1500.0 * led2 / 32;
	}
	else
	{
		i1 = 188.0 * led1 / 32;
		i2 = 188.0 * led2 / 32;
	}
	return i1 + i2;
}
int calSleepFrame(int isMain, int led1, int led2, float maxA1, float maxA2)
{
	float i1;
	float i2;
	i1 = calLedmA(isMain, led1, 0);
	i2 = calLedmA(isMain, 0, led2);
	double t1;
	double t2;
	t1 = 4000.0 * (i1 - 200) / (maxA1 - 200);
	t2 = 4000.0 * (i2 - 200) / (maxA2 - 200);
	double t;
	t = t1;
	if (t2 > t1)
		t = t2;
	if (t < 0)
		t = 0;
	if (t > 5000)
		t = 5000;
	int frm;
	frm = t / 30;
	return frm;
}

static double interp(double x1, double y1, double x2, double y2, double x)
{
	double y;
	if (x1 == x2)
		return (y2 + y1) / 2;
	y = y1 + (y2 - y1) / (x2 - x1)*(x - x1);
	return y;
}


//rgTab increasing
float interpCt(float* rgTab, float* ctTab, int numTab, float rg)
{	
	int i;
	if (rg < rgTab[0])
	{
		return ctTab[0];
	}
	else if (rg > rgTab[numTab-1])
	{
		return ctTab[numTab - 1];
	}

	for (i = 1; i < numTab; i++)
	{
		if (rgTab[i] > rg)
		{
			return interp(rgTab[i - 1], ctTab[i - 1], rgTab[i], ctTab[i], rg);
		}
	}
	
	return 10000.0;
}

static void flashCalibration(struct ae_ctrl_cxt *cxt)
{
#define FC_INIT_SHUTTER 100000
#define FC_INIT_GAIN 128
	struct ae_alg_calc_param *cur_status = &cxt->sync_cur_status;
	static int frameCount = 0;
	static int caliState = FlashCali_start;
	static struct FCData* caliData = 0;
	int propValue[10];
	int propRet;
	propRet = get_prop_multi("persist.sys.isp.ae.flash_cali", 5, propValue);

	if (propRet <= 0)
		return;

	if (propValue[0] == 0)
	{
		control_led(cxt, 0, 0, 0, 0);
		caliState = FlashCali_start;
	}
	if (propValue[0] == 1)
	{


		ISP_LOGD("qqfc state=%d", caliState);
		if (caliState == FlashCali_start)
		{
			ISP_LOGD("qqfc FlashCali_start");
			caliData = (struct FCData*)malloc(sizeof(struct FCData));
			caliData->out.error = 0;
			//0
			//1: flash led number
			//2: led max current 1
			//3: led max current 2
			int isDual = 1;
			int maxCurrent1 = 1500;
			int maxCurrent2 = 1500;
			int maxCurrentAll = 1500;

			if (propRet >= 2)
			{
				if (propValue[1] == 1)
					isDual = 0;
				else
					isDual = 1;
			}
			if (propRet >= 5)
			{
				maxCurrent1 = propValue[2];
				maxCurrent2 = propValue[3];
				maxCurrentAll = propValue[4];
			}

			caliData->isDual = isDual;
			caliData->maxCurrent1 = maxCurrent1;
			caliData->maxCurrent2 = maxCurrent2;
			caliData->maxCurrentAll = maxCurrentAll;


			frameCount = 0;
			cxt->cur_status.settings.lock_ae = AE_STATE_LOCKED; //lock ae
			control_led(cxt, 1, 0, 1, 0);
			int expInit = FC_INIT_SHUTTER;
			int gainInit = FC_INIT_GAIN;

			int lineTime = cxt->cur_status.line_time;
			caliData->expTime = expInit;
			caliData->gain = gainInit;

			//gen test
			caliData->testInd = 0;
			int i;
			int id = 0;
			for (i = 0; i<32; i++)
			{
				caliData->expReset[id] = 0;
				if (i == 0)
					caliData->expReset[id] = 1;
				caliData->ind1Tab[id] = i;
				caliData->ind2Tab[id] = 0;
				caliData->testMinFrm[id] = 0;
				caliData->isMainTab[id] = 0;
				id++;
			}

			for (i = 0; i<32; i++)
			{
				caliData->expReset[id] = 0;
				if(i==0)
					caliData->expReset[id] = 1;
				caliData->ind1Tab[id] = 0;
				caliData->ind2Tab[id] = i;
				caliData->testMinFrm[id] = 0;
				caliData->isMainTab[id] = 0;
				id++;
			}
			for (i = 0; i<32; i++)
			{
				caliData->expReset[id] = 0;
				if (i == 0)
					caliData->expReset[id] = 1;
				caliData->ind1Tab[id] = i;
				caliData->ind2Tab[id] = 0;
				if (calLedmA(1,i,0) < caliData->maxCurrent1)
				{
					int frm = calSleepFrame(1, i, 0, caliData->maxCurrent1, caliData->maxCurrent2);
					caliData->testMinFrm[id] = frm;
					caliData->isMainTab[id] = 1;
					id++;
				}
			}
			for (i = 0; i<32; i++)
			{
				caliData->expReset[id] = 0;
				if (i == 0)
					caliData->expReset[id] = 1;
				caliData->ind1Tab[id] = 0;
				caliData->ind2Tab[id] = i;
				if (calLedmA(1, 0, i) < caliData->maxCurrent2)
				{
					int frm = calSleepFrame(1, 0, i, caliData->maxCurrent1, caliData->maxCurrent2);
					caliData->testMinFrm[id] = frm;
					caliData->isMainTab[id] = 1;
					id++;
				}
			}
			caliData->testIndAll = id;

			//
			caliData->stateAeFrameCntSt = 1;
			//caliData->stateCaliFrameCntSt = 0;

			caliState = FlashCali_ae;



		}
		else if (caliState == FlashCali_ae)
		{
			int frm = frameCount - caliData->stateAeFrameCntSt;
			if ( (frm % 3 == 0 ) && (frm!=0))
			{
				float rmean;
				float gmean;
				float bmean;
				struct ae_alg_calc_param *current_status = &cxt->sync_cur_status;
				getCenterMean((cmr_u32*)&cxt->sync_aem[0],
					caliData->rBuf,
					caliData->gBuf,
					caliData->bBuf,
					cxt->monitor_unit.win_size,
					cxt->monitor_unit.win_num,
					current_status->monitor_shift,
					&rmean,
					&gmean,
					&bmean);
				ISP_LOGD("qqfc frmCnt=%d exp=%d %d, g=%f", (int)frameCount, (int)caliData->expTime, (int)caliData->gain, gmean);

				if ( (gmean>200 && gmean<400) ||
					 (caliData->expTime==500000 && caliData->gain == 8 * 128))
				{
					caliData->stateCaliFrameCntSt2 = frameCount+1;
					caliData->expTimeBase = caliData->expTime;
					caliData->gainBase = caliData->gain;
					caliState = FlashCali_cali;
				}
				else
				{
					if (gmean<10)
					{
						caliData->expTime *= 25;
						caliData->gain = caliData->gain;
					}
					else
					{
						caliData->expTime *= 300 / gmean;
						caliData->gain = caliData->gain;
					}
					if (caliData->expTime>500000)
					{
						float ratio = caliData->expTime / 500000.0f;
						caliData->expTime = 500000;
						caliData->gain *= ratio;
					}
					else if (caliData->expTime<10000)
					{
						float ratio = caliData->expTime / 10000.0f;
						caliData->expTime = 10000;
						caliData->gain *= ratio;
					}
					if (caliData->gain<128)
						caliData->gain = 128;
					else if (caliData->gain>8 * 128)
						caliData->gain = 8 * 128;
				}
			}

		}
		else if (caliState == FlashCali_cali)
		{
			int frmCntState;
			int frmCnt;
			frmCntState = frameCount - caliData->stateCaliFrameCntSt2;
			if (frmCntState == 0)
			{
				caliData->testInd = 0;
				caliData->stateCaliFrameCntStSub = caliData->stateCaliFrameCntSt2;
			}

			frmCnt = frameCount - caliData->stateCaliFrameCntStSub;
			ISP_LOGD("qqfc caliData->testInd=%d",caliData->testInd);
			if (frmCnt == 0)
			{
				int id;
				id = caliData->testInd;
				int led1;
				int led2;
				led1 = caliData->ind1Tab[id];
				led2 = caliData->ind2Tab[id];
				if (caliData->expReset[id] == 1)
				{
					caliData->expTime = caliData->expTimeBase;
					caliData->gain = caliData->gainBase;
				}
				caliData->expTab[id] = caliData->expTime;
				caliData->gainTab[id] = caliData->gain;
				if(led1==0 && led2 == 0)
					control_led(cxt, 0,0,0,0);
				else
					control_led(cxt, 1, caliData->isMainTab[id], led1, led2);
			}
			else if (frmCnt == 3)
			{
				float rmean;
				float gmean;
				float bmean;
				struct ae_alg_calc_param *current_status = &cxt->sync_cur_status;
				getCenterMean((cmr_u32*)&cxt->sync_aem[0],
					caliData->rBuf,
					caliData->gBuf,
					caliData->bBuf,
					cxt->monitor_unit.win_size,
					cxt->monitor_unit.win_num,
					current_status->monitor_shift,
					&rmean,
					&gmean,
					&bmean);
				if (gmean > 600)
				{

					double rat=2;
					rat = gmean / 300.0;
					double rat1;
					double rat2;
					double rat3;

					if (caliData->expTime > 300000)
					{
						rat1 = rat;
						int expTest;
						expTest = caliData->expTime / rat1;
						if (expTest < 300000)
							expTest = 300000;
						rat1 = (double)caliData->expTime / expTest;
						caliData->expTime = expTest;
					}
					else
						rat1 = 1;

					if (caliData->gain > 128)
					{
						rat2 = rat / rat1;
						int gainTest= caliData->gain;
						gainTest /= rat2;
						if (gainTest < 128)
							gainTest = 128;
						rat2 = (double)caliData->gain / gainTest;
						caliData->gain = gainTest;
					}
					else
					{
						rat2 = 1;
					}
					{
						rat3 = rat / rat1 / rat2;
						int expTest;
						expTest = caliData->expTime / rat3;
						if (expTest < 10000)
							expTest = 10000;
						rat3 = (double)caliData->expTime / expTest;
						caliData->expTime = expTest;

						if (expTest == 10000 && caliData->gain == 128)
						{
							char err[] = "error: the exposure is min\n";
#ifdef WIN32
							printf(err);
#endif
							ISP_LOGE("%s",err);
							caliData->out.error = FlashCali_too_close;

						}
						else
						{
							caliData->stateCaliFrameCntStSub = frameCount + 1;
						}
					}
				}
				else
				{
					caliData->rFrame[caliData->testInd][frmCnt] = rmean;
					caliData->gFrame[caliData->testInd][frmCnt] = gmean;
					caliData->bFrame[caliData->testInd][frmCnt] = bmean;
				}
			}
			else if (frmCnt < 15)
			{
				float rmean;
				float gmean;
				float bmean;
				struct ae_alg_calc_param *current_status = &cxt->sync_cur_status;
				getCenterMean((cmr_u32*)&cxt->sync_aem[0],
					caliData->rBuf,
					caliData->gBuf,
					caliData->bBuf,
					cxt->monitor_unit.win_size,
					cxt->monitor_unit.win_num,
					current_status->monitor_shift,
					&rmean,
					&gmean,
					&bmean);
				caliData->rFrame[caliData->testInd][frmCnt] = rmean;
				caliData->gFrame[caliData->testInd][frmCnt] = gmean;
				caliData->bFrame[caliData->testInd][frmCnt] = bmean;


			}
			else if(frmCnt > caliData->testMinFrm[caliData->testInd])
			{
				float r;
				float g;
				float b;
				calRgbFrameData(caliData->isMainTab[caliData->testInd], caliData->rFrame[caliData->testInd], caliData->gFrame[caliData->testInd], caliData->bFrame[caliData->testInd], &r, &g, &b);
				caliData->rData[caliData->testInd] = r;
				caliData->gData[caliData->testInd] = g;
				caliData->bData[caliData->testInd] = b;
				caliData->testInd++;
				caliData->stateCaliFrameCntStSub = frameCount+1;
				if(caliData->testInd==caliData->testIndAll)
					caliState = FlashCali_end;
			}

		}
		else if (caliState == FlashCali_end)
		{

			int i;
			float rTab1[32];
			float gTab1[32];
			float bTab1[32];
			float rTab2[32];
			float gTab2[32];
			float bTab2[32];
			float rTab1Main[32];
			float gTab1Main[32];
			float bTab1Main[32];
			float rTab2Main[32];
			float gTab2Main[32];
			float bTab2Main[32];

			double rbase;
			double gbase;
			double bbase;
			rbase = caliData->rData[0];
			gbase = caliData->gData[0];
			bbase = caliData->bData[0];
			for (i = 0; i < 32; i++)
			{
				gTab1[i] = -1;
				gTab2[i] = -1;
				gTab1Main[i] = -1;
				gTab2Main[i] = -1;
			}
			rTab1[0] = 0;
			gTab1[0] = 0;
			bTab1[0] = 0;
			rTab2[0] = 0;
			gTab2[0] = 0;
			bTab2[0] = 0;
			rTab1Main[0] = 0;
			gTab1Main[0] = 0;
			bTab1Main[0] = 0;
			rTab2Main[0] = 0;
			gTab2Main[0] = 0;
			bTab2Main[0] = 0;
			double maxV = 0;
			for (i = 0; i < caliData->testIndAll; i++)
			{
				double rat;
				rat = (double)caliData->expTimeBase* caliData->gainBase
					/ caliData->gainTab[i] / caliData->expTab[i];

				int id1;
				int id2;
				id1 = caliData->ind1Tab[i];
				id2 = caliData->ind2Tab[i];

				float r;
				float g;
				float b;

				r = caliData->rData[i] * rat - rbase;
				g = caliData->gData[i] * rat - gbase;
				b = caliData->bData[i] * rat - bbase;
				if (maxV < g)
					maxV = g;
				if (caliData->ind1Tab[i] != 0 && caliData->isMainTab[i])
				{
					rTab1Main[id1] = r;
					gTab1Main[id1] = g;
					bTab1Main[id1] = b;
				}
				else if (caliData->ind1Tab[i] != 0 && caliData->isMainTab[i]==0)
				{
					rTab1[id1] = r;
					gTab1[id1] = g;
					bTab1[id1] = b;
				}
				else if (caliData->ind2Tab[i] != 0 && caliData->isMainTab[i])
				{
					rTab2Main[id2] = r;
					gTab2Main[id2] = g;
					bTab2Main[id2] = b;
				}
				else if (caliData->ind2Tab[i] != 0 && caliData->isMainTab[i] == 0)
				{
					rTab2[id2] = r;
					gTab2[id2] = g;
					bTab2[id2] = b;
				}
			}
			double sc = 0;
			sc = 65536.0 / maxV/2;
			for (i = 0; i < 32; i++)
			{
				rTab1[i] *= sc;
				gTab1[i] *= sc;
				bTab1[i] *= sc;
				rTab2[i] *= sc;
				gTab2[i] *= sc;
				bTab2[i] *= sc;
				rTab1Main[i] *= sc;
				gTab1Main[i] *= sc;
				bTab1Main[i] *= sc;
				rTab2Main[i] *= sc;
				gTab2Main[i] *= sc;
				bTab2Main[i] *= sc;
			}

			caliData->out.version = 1;
			caliData->out.flashLevelNum1 = 32;
			if (caliData->isDual)
				caliData->out.flashLevelNum2 = 32;
			else
				caliData->out.flashLevelNum2 = 1;


			//--------------------------
			//--------------------------
			//@@

			float rgtab[20];
			float cttab[20];
			for (i = 0; i < 20; i++)
			{
				rgtab[i] = cxt->ctTabRg[i];
				cttab[i] = cxt->ctTab[i];	
			}

			//--------------------------
			//--------------------------

			for (i = 0; i < 1024; i++)
			{
				int id1;
				int id2;
				id1 = i % 32;
				id2 = i / 32;

				caliData->out.brightnessTable[i] = -1;
				if (gTab1Main[id1] != -1 && gTab2Main[id2] != -1)
				{
					caliData->out.brightnessTable[i] = _round(gTab1Main[id1] + gTab2Main[id2]);
					double r;
					double g;
					double b;
					r = rTab1Main[id1] + rTab2Main[id2];
					g = gTab1Main[id1] + gTab2Main[id2];
					b = bTab1Main[id1] + bTab2Main[id2];
					caliData->out.rTable[i] = _round(1024 * g/r);
					caliData->out.bTable[i] = _round(1024 * g/b);
				}
				caliData->out.preflashBrightness[i] = _round(gTab1[id1] + gTab2[id2]);

				double rg;
				caliData->out.preflashCt[i] = 0;
				if (gTab1[id1] + gTab2[id2] != 0)
				{
					rg = (rTab1[id1] + rTab2[id2]) / (gTab1[id1] + gTab2[id2]);
					caliData->out.preflashCt[i] = interpCt(rgtab, cttab, 20, rg);
				}

			}

			for (i = 0; i < 1024; i++)
			{
				int id1;
				int id2;
				id1 = i % 32;
				id2 = i / 32;
				int ma1;
				int ma2;
				int maAll;

				ma1 = calLedmA(1, id1, 0);
				ma2 = calLedmA(1, 0, id2);
				maAll = ma1 + ma2;


				caliData->out.flashMask[i] = 0;
				if (ma1 > caliData->maxCurrent1)
					;
				else if (ma2 > caliData->maxCurrent2)
					;
				else if ((ma1 + ma2) > caliData->maxCurrentAll)
					;
				else if ((int)caliData->out.brightnessTable[i] == -1)
					;
				else
				{
					caliData->out.flashMask[i] = 1;
				}
			}
			caliData->out.flashMask[0] = 0;

			FILE* fp;


			int propValue[10];
			int propRet;
			propRet = get_prop_multi("persist.sys.isp.ae.fc_debug", 1, propValue);

			if (propValue[0] == 1)
			{
#ifdef WIN32
				fp = fopen("d:\\temp\\fc_debug.txt", "wt");
#else
				fp = fopen("/data/misc/cameraserver/fc_debug.txt", "wt");
#endif
				fprintf(fp, "\nmask\n");
				for (i = 0; i < 1024; i++)
				{
					fprintf(fp, "%d\t", (int)caliData->out.flashMask[i]);
					if (i % 32 == 31)
						fprintf(fp, "\n");
				}
				fprintf(fp, "\nbrightness\n");
				for (i = 0; i < 1024; i++)
				{
					fprintf(fp, "%d\t", (int)caliData->out.brightnessTable[i]);
					if (i % 32 == 31)
						fprintf(fp, "\n");
				}
				fprintf(fp, "\nr tab\n");
				for (i = 0; i < 1024; i++)
				{
					fprintf(fp, "%d\t", (int)caliData->out.rTable[i]);
					if (i % 32 == 31)
						fprintf(fp, "\n");
				}
				fprintf(fp, "\nb tab\n");
				for (i = 0; i < 1024; i++)
				{
					fprintf(fp, "%d\t", (int)caliData->out.bTable[i]);
					if (i % 32 == 31)
						fprintf(fp, "\n");
				}
				fprintf(fp, "\npre bright\n");
				for (i = 0; i < 1024; i++)
				{
					fprintf(fp, "%d\t", (int)caliData->out.preflashBrightness[i]);
					if (i % 32 == 31)
						fprintf(fp, "\n");
				}
				fprintf(fp, "\npre ct\n");
				for (i = 0; i < 1024; i++)
				{
					fprintf(fp, "%d\t", (int)caliData->out.preflashCt[i]);
					if (i % 32 == 31)
						fprintf(fp, "\n");
				}
				fclose(fp);

#ifdef WIN32
				fp = fopen("d:\\temp\\fc_debug.bin", "wb");
#else
				fp = fopen("/data/misc/cameraserver/fc_debug.bin", "wb");
#endif
				fwrite(caliData, 1, sizeof(struct FCData), fp);
				fclose(fp);

			}


			if (caliData->out.error != 0)
			{
#ifdef WIN32
				fp = fopen("d:\\temp\\fc_error.txt", "wt");
#else
				fp = fopen("/data/misc/cameraserver/fc_error.txt", "wt");
#endif
				if (caliData->out.error == FlashCali_too_close)
				{
					fprintf(fp, "error: chart to camera is to close!\n");
				}

				fclose(fp);
			}
			else
			{
#ifdef WIN32
				fp = fopen("d:\\temp\\flashcalibration.bin", "wb");
#else
				fp = fopen("/data/misc/cameraserver/flashcalibration.bin", "wb");
#endif
				fwrite(&caliData->out, 1, sizeof(struct flash_calibration_data), fp);
				fclose(fp);
			}


			free(caliData);
			caliState = FlashCali_none;


		}
		else
		{
			//none
#ifdef WIN32
			printf("state=none\n");
#endif
			ISP_LOGD("flash calibration done!!\n");
		}

		if (caliState < FlashCali_end)
		{
			ISP_LOGD("qqfc exp=%d %d", (int)caliData->expTime, (int)caliData->gain);
			int lineTime = cxt->cur_status.line_time;
			cur_status->settings.manual_mode = 0;
			cur_status->settings.table_idx = 0;
			cur_status->settings.exp_line = caliData->expTime / lineTime;
			cur_status->settings.gain = caliData->gain;
		}
	}
	frameCount++;
}


static void _set_led2(struct ae_ctrl_cxt *cxt)
{
	struct ae_alg_calc_param *cur_status = &cxt->cur_status;
	int propValue[10];
	int ret;
	static int led_onOff=0;
	static int led_isMain=0;
	static int led_led1=0;
	static int led_led2=0;

	ret = get_prop_multi("persist.sys.isp.ae.fc_led", 4, propValue);
	if(ret>0)
	{
		if(led_onOff==propValue[0] && led_isMain==propValue[1] && led_led1==propValue[2] && led_led2==propValue[3])
		{
		}
		else
		{
			led_onOff=propValue[0];
			led_isMain=propValue[1];
			led_led1=propValue[2];
			led_led2=propValue[3];
			control_led(cxt, led_onOff, led_isMain, led_led1, led_led2);
			ISP_LOGD("qqfc led control %d %d %d %d", led_onOff, led_isMain, led_led1, led_led2);
		}
	}

	static int exp_exp_line=0;
	static int exp_exp_time=0;
	static int exp_dummy=0;
	static int exp_isp_gain=0;
	static int exp_sensor_gain=0;
	ret = get_prop_multi("persist.sys.isp.ae.fc_exp", 5, propValue);
	if(ret>0)
	{
		if(exp_exp_line==propValue[0] &&
		    exp_exp_time==propValue[1] &&
		    exp_dummy==propValue[2] &&
		    exp_isp_gain==propValue[3] &&
		    exp_sensor_gain==propValue[4])
		{
		}
		else
		{
			exp_exp_line=propValue[0];
			exp_exp_time=propValue[1];
			exp_dummy=propValue[2];
			exp_isp_gain=propValue[3];
			exp_sensor_gain=propValue[4];
			cur_status->settings.manual_mode = 0;
			cur_status->settings.table_idx = 0;
			cur_status->settings.exp_line = exp_exp_line;
			cur_status->settings.gain = exp_isp_gain * exp_sensor_gain / 4096;
			ISP_LOGD("qqfc set exp %d %d %d %d %d", exp_exp_line, exp_exp_time, exp_dummy, exp_isp_gain, exp_sensor_gain);
		}
	}

	static int lock_lock=0;
	ret = get_prop_multi("persist.sys.isp.ae.fc_lock", 1, propValue);
	if(ret>0)
	{
		if(lock_lock==propValue[0])
		{
		}
		else
		{
			lock_lock=propValue[0];
			if(lock_lock==1)
			{
				//_set_pause(cxt);
				cxt->cur_status.settings.lock_ae = AE_STATE_LOCKED;
				ISP_LOGD("qqfc lock");
			}

		}
	}

	static int lock_unlock=0;
	ret = get_prop_multi("persist.sys.isp.ae.fc_unlock", 1, propValue);
	if(ret>0)
	{
		if(lock_unlock==propValue[0])
		{
		}
		else
		{
			lock_unlock=propValue[0];
			if(lock_unlock==1)
			{
			   // _set_restore_cnt(cxt);
			   cxt->cur_status.settings.lock_ae = AE_STATE_NORMAL;
			    ISP_LOGD("qqfc unlock");
			}
		}
	}

	static int exp2=0;
	ret = get_prop_multi("persist.sys.isp.ae.fc_exp2", 1, propValue);
	if(ret>0)
	{
		if(exp2==propValue[0])
		{
		}
		else
		{
			exp2=propValue[0];
			if(exp2==1)
			{			
				cur_status->settings.manual_mode = 0;
				cur_status->settings.table_idx = 0;
				cur_status->settings.exp_line = 800;
				cur_status->settings.gain = 256;				
			}
		}
	}

	static int exp1=0;
	ret = get_prop_multi("persist.sys.isp.ae.fc_exp1", 1, propValue);
	if(ret>0)
	{
		if(exp1==propValue[0])
		{
		}
		else
		{
			exp1=propValue[0];
			if(exp1==1)
			{
				cur_status->settings.manual_mode = 0;
				cur_status->settings.table_idx = 0;
				cur_status->settings.exp_line = 100;
				cur_status->settings.gain = 256;
	    			ISP_LOGD("qqfc exp1 set");
			}
		}
	}


}

static void _set_flash_calibration_script(struct ae_ctrl_cxt *cxt)
{	
	char str[50];
	memset((void*)&str[0], 0, sizeof(str));
	property_get("persist.sys.isp.ae.fc_stript", str, "");
	if (!strcmp(str, "on")) {
		flashCalibration(cxt);
		_set_led2(cxt);
	}
}

static void _set_led(struct ae_ctrl_cxt *cxt)
{
	char str[50];
	cmr_s16 i = 0;
	cmr_s16 j = 0;
	float tmp = 0;
	cmr_u32 type = ISP_FLASH_TYPE_MAIN;	//ISP_FLASH_TYPE_MAIN   ISP_FLASH_TYPE_PREFLASH
	cmr_s8 led_ctl[2] = { 0, 0 };
	struct ae_flash_cfg cfg;
	struct ae_flash_element element;

	memset(str, 0, sizeof(str));
	property_get("persist.sys.isp.ae.manual", str, "");
	if ((strcmp(str, "fasim") & strcmp(str, "facali") & strcmp(str, "facali-pre") & strcmp(str, "led"))) {
		//ISP_LOGV("isp_set_led_noctl!\r\n");
	} else {
		if (!strcmp(str, "facali"))
			type = ISP_FLASH_TYPE_MAIN;
		else if (!strcmp(str, " facali-pre"))
			type = ISP_FLASH_TYPE_PREFLASH;
		else
			type = ISP_FLASH_TYPE_PREFLASH;

		memset(str, 0, sizeof(str));
		property_get("persist.sys.isp.ae.led", str, "");
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
		property_get("persist.sys.isp.ae.facali.dump", str, "");
		if (!strcmp(str, "on")) {
			FILE *p = NULL;
			p = fopen("/data/misc/cameraserver/flashcali.txt", "w+");
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

static void ae_hdr_ctrl(struct ae_ctrl_cxt *cxt, struct ae_calc_in *param)
{
	UNUSED(param);
	cxt->hdr_up = cxt->hdr_down = 50;

	ISP_LOGV("cam-id %d, cxt->hdr_frame_cnt=%d", cxt->camera_id, cxt->hdr_frame_cnt);
	if (cxt->hdr_frame_cnt == cxt->hdr_cb_cnt && cxt->hdr_enable == 1) {
		cxt->hdr_frame_cnt = 0;
#ifdef CONFIG_CAMERA_DUAL_SYNC
		(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, AE_CB_HDR_START, NULL);
#else
		(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, AE_CB_HDR_START);
#endif
		ISP_LOGI("ae_hdr do-callback");
	}

	if (3 == cxt->hdr_flag) {
		cxt->cur_status.settings.manual_mode = 1;
		cxt->cur_status.settings.table_idx = cxt->hdr_base_ae_idx - cxt->hdr_up;
		cxt->hdr_flag--;
		cxt->hdr_frame_cnt++;
		ISP_LOGI("_isp_hdr_3: %d\n", cxt->cur_status.settings.table_idx);
	} else if (2 == cxt->hdr_flag) {
		cxt->cur_status.settings.manual_mode = 1;
		cxt->cur_status.settings.table_idx = cxt->hdr_base_ae_idx + cxt->hdr_down;
		cxt->hdr_flag--;
		cxt->hdr_frame_cnt++;
		ISP_LOGI("_isp_hdr_2: %d\n", cxt->cur_status.settings.table_idx);
	} else if (1 == cxt->hdr_flag) {
		cxt->cur_status.settings.manual_mode = 1;
		cxt->cur_status.settings.table_idx = cxt->hdr_base_ae_idx;
		cxt->hdr_flag--;
		cxt->hdr_frame_cnt++;
		ISP_LOGI("_isp_hdr_1: %d\n", cxt->cur_status.settings.table_idx);
	} else {
		;
	}
	return;
}
struct ae_exposure_param s_bakup_exp_param[2] = {{0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0}};
static void _set_ae_video_stop(struct ae_ctrl_cxt *cxt)
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
		} else {
			cxt->last_exp_param.exp_line = cxt->sync_cur_result.wts.cur_exp_line;
			cxt->last_exp_param.exp_time = cxt->sync_cur_result.wts.cur_exp_line * cxt->cur_status.line_time;
			cxt->last_exp_param.dummy = cxt->sync_cur_result.wts.cur_dummy;
			cxt->last_exp_param.gain = cxt->sync_cur_result.wts.cur_again;
			cxt->last_exp_param.line_time = cxt->cur_status.line_time;
			cxt->last_exp_param.cur_index = cxt->sync_cur_result.wts.cur_index;
			cxt->last_index = cxt->sync_cur_result.wts.cur_index;
		}
		cxt->last_enable = 1;
		s_bakup_exp_param[cxt->camera_id] = cxt->last_exp_param;
		ISP_LOGI("AE_VIDEO_STOP(in preview) cam-id %d E %d G %d lt %d W %d H %d", cxt->camera_id , cxt->last_exp_param.exp_line,
				cxt->last_exp_param.gain, cxt->last_exp_param.line_time, cxt->snr_info.frame_size.w, cxt->snr_info.frame_size.h);
	}else {
		if ((1 == cxt->is_snapshot) &&
			((FLASH_NONE == cxt->cur_status.settings.flash) || FLASH_MAIN_BEFORE <= cxt->cur_status.settings.flash)) {
			_set_restore_cnt(cxt);
		}
		ISP_LOGI("AE_VIDEO_STOP(in capture) cam-id %d E %d G %d lt %d W %d H %d", cxt->camera_id , cxt->last_exp_param.exp_line,
				cxt->last_exp_param.gain, cxt->last_exp_param.line_time, cxt->snr_info.frame_size.w, cxt->snr_info.frame_size.h);
	}
}

static cmr_s32 _set_ae_video_start(struct ae_ctrl_cxt *cxt, cmr_handle *param)
{
	cmr_s32 rtn = AE_SUCCESS;
	cmr_s32 i;
	cmr_s32 again;
	float rgb_gain_coeff;
	cmr_s32 ae_skip_num = 0;
	cmr_s32 mode = 0;
	struct ae_trim trim;
	struct ae_set_work_param *work_info = (struct ae_set_work_param *)param;

	if (work_info->mode >= AE_WORK_MODE_MAX) {
		ISP_LOGE("fail to set work mode");
		work_info->mode = AE_WORK_MODE_COMMON;
	}

	if (0 == work_info->is_snapshot) {
		cxt->cur_status.frame_id = 0;
		memset(&cxt->cur_result.wts, 0, sizeof(struct ae1_senseor_out));
		memset(&cxt->sync_cur_result.wts, 0, sizeof(struct ae1_senseor_out));
		cxt->send_once[0] = cxt->send_once[1] = cxt->send_once[2] = cxt->send_once[3] = 0;
	}

	cxt->snr_info = work_info->resolution_info;
	cxt->cur_status.frame_size = work_info->resolution_info.frame_size;
	cxt->cur_status.line_time = work_info->resolution_info.line_time / SENSOR_LINETIME_BASE;
	cxt->cur_status.snr_max_fps = work_info->sensor_fps.max_fps;
	cxt->cur_status.snr_min_fps = work_info->sensor_fps.min_fps;

	cxt->start_id = AE_START_ID;
	cxt->monitor_unit.mode = AE_STATISTICS_MODE_CONTINUE;
	cxt->monitor_unit.cfg.skip_num = 0;
	cxt->monitor_unit.is_stop_monitor = 0;
	cxt->high_fps_info.is_high_fps = work_info->sensor_fps.is_high_fps;

	if (work_info->sensor_fps.is_high_fps) {
		ae_skip_num = work_info->sensor_fps.high_fps_skip_num - 1;
		if (ae_skip_num > 0) {
			cxt->monitor_unit.cfg.skip_num = ae_skip_num;
			cxt->high_fps_info.min_fps = work_info->sensor_fps.max_fps;
			cxt->high_fps_info.max_fps = work_info->sensor_fps.max_fps;
		} else {
			cxt->monitor_unit.cfg.skip_num = 0;
			ISP_LOGI("cxt->monitor_unit.cfg.skip_num %d", cxt->monitor_unit.cfg.skip_num);
		}
	}

	trim.x = 0;
	trim.y = 0;
	trim.w = work_info->resolution_info.frame_size.w;
	trim.h = work_info->resolution_info.frame_size.h;
	_update_monitor_unit(cxt, &trim);
	_cfg_set_aem_mode(cxt);
	_cfg_monitor(cxt);

	cxt->cur_status.win_size = cxt->monitor_unit.win_size;
	cxt->cur_status.win_num = cxt->monitor_unit.win_num;
	if (1 == cxt->tuning_param_enable[work_info->mode])
		mode = work_info->mode;
	else
		mode = AE_WORK_MODE_COMMON;
	
	memcpy(&cxt->tuning_param[mode].ae_table[0][0],\
		&cxt->tuning_param[mode].backup_ae_table[0][0],\
		AE_FLICKER_NUM * AE_ISO_NUM * sizeof(struct ae_exp_gain_table));

	exposure_time2line(&(cxt->tuning_param[mode]), cxt->cur_status.line_time,	cxt->tuning_param[mode].ae_tbl_exp_mode);

	for (cmr_s32 j = 0; j < AE_SCENE_MAX; ++j) {
		exp_time2exp_line(cxt, cxt->back_scene_mode_ae_table[j],
				  cxt->tuning_param[work_info->mode].scene_info[j].ae_table,
				  cxt->cur_status.line_time,
				  cxt->tuning_param[work_info->mode].scene_info[j].exp_tbl_mode);
	}

	cxt->cur_status.ae_table = &cxt->cur_param->ae_table[cxt->cur_status.settings.flicker][AE_ISO_AUTO];
	cxt->sync_cur_status.settings.scene_mode = AE_SCENE_NORMAL;
	if (1 == cxt->last_enable) {
		if (cxt->cur_status.line_time == (cmr_s16)cxt->last_exp_param.line_time) {
			cxt->cur_result.wts.cur_exp_line = cxt->last_exp_param.exp_line;
			cxt->cur_result.wts.cur_again = cxt->last_exp_param.gain;
			cxt->cur_result.wts.cur_dummy = cxt->last_exp_param.dummy;
		} else {
			cxt->cur_result.wts.cur_exp_line = cxt->last_exp_param.exp_line * cxt->last_exp_param.line_time / (float)cxt->cur_status.line_time;
			if (cxt->min_exp_line > cxt->cur_result.wts.cur_exp_line)
				cxt->cur_result.wts.cur_exp_line = cxt->min_exp_line;

			cxt->cur_result.wts.cur_again = cxt->last_exp_param.gain;
			cxt->cur_result.wts.cur_dummy = cxt->last_exp_param.dummy;
		}
		cxt->cur_result.wts.cur_index = cxt->last_index;
	} else {
		if ((0 != s_bakup_exp_param[cxt->camera_id].exp_line)\
			&& (0 != s_bakup_exp_param[cxt->camera_id].exp_time)\
			&& (0 != s_bakup_exp_param[cxt->camera_id].gain)) {
			cxt->cur_result.wts.cur_exp_line = s_bakup_exp_param[cxt->camera_id].exp_time / cxt->cur_status.line_time;
			cxt->cur_result.wts.exposure_time = s_bakup_exp_param[cxt->camera_id].exp_time;
			cxt->cur_result.wts.cur_again = s_bakup_exp_param[cxt->camera_id].gain;
			cxt->cur_result.wts.cur_dgain = 0;
			cxt->cur_result.wts.cur_dummy = 0;
			cxt->cur_result.wts.cur_index  = s_bakup_exp_param[cxt->camera_id].cur_index;
			cxt->cur_result.wts.stable = 0;	
		} else {
			cxt->cur_result.wts.cur_exp_line = cxt->cur_status.ae_table->exposure[cxt->cur_status.start_index];
			cxt->cur_result.wts.exposure_time = cxt->cur_status.ae_table->exposure[cxt->cur_status.start_index] * cxt->snr_info.line_time;
			cxt->cur_result.wts.cur_again = cxt->cur_status.ae_table->again[cxt->cur_status.start_index];
			cxt->cur_result.wts.cur_dgain = 0;
			cxt->cur_result.wts.cur_dummy = 0;
			cxt->cur_result.wts.cur_index  =cxt->cur_status.start_index;
			cxt->cur_result.wts.stable = 0;	
		}
	}

	cxt->sync_cur_result.wts.exposure_time = cxt->cur_result.wts.exposure_time;
	cxt->sync_cur_result.wts.cur_exp_line = cxt->cur_result.wts.cur_exp_line;
	cxt->sync_cur_result.wts.cur_again = cxt->cur_result.wts.cur_again;
	cxt->sync_cur_result.wts.cur_dgain = cxt->cur_result.wts.cur_dgain;
	cxt->sync_cur_result.wts.cur_dummy = cxt->cur_result.wts.cur_dummy;
	cxt->sync_cur_result.wts.cur_index = cxt->cur_result.wts.cur_index;
	cxt->sync_cur_result.wts.stable = cxt->cur_result.wts.stable;

	/*update parameters to sensor*/
	memset((void*)&cxt->exp_data, 0, sizeof(cxt->exp_data));
	cxt->exp_data.lib_data.exp_line = cxt->sync_cur_result.wts.cur_exp_line;
	cxt->exp_data.lib_data.exp_time = cxt->sync_cur_result.wts.exposure_time;
	cxt->exp_data.lib_data.gain = cxt->sync_cur_result.wts.cur_again;
	cxt->exp_data.lib_data.dummy = cxt->sync_cur_result.wts.cur_dummy;
	cxt->exp_data.lib_data.line_time = cxt->cur_status.line_time;
	rtn = ae_result_update_to_sensor(cxt, &cxt->exp_data, 1);

	/*it is normal capture, not in flash mode*/
	if ((1 == work_info->is_snapshot) && (0 == cxt->cur_status.settings.flash)) {
		ISP_LOGV("nozsl cap will lock ae");
		_set_pause(cxt);
		cxt->cur_status.settings.manual_mode = 0;
		cxt->cur_status.settings.table_idx = 0;
		cxt->cur_status.settings.exp_line = cxt->sync_cur_result.wts.cur_exp_line;
		cxt->cur_status.settings.gain = cxt->sync_cur_result.wts.cur_again;
	}
	ISP_LOGI("AE_VIDEO_START cam-id %d lt %d W %d H %d CAP %d", cxt->camera_id, cxt->cur_status.line_time,
		cxt->snr_info.frame_size.w, cxt->snr_info.frame_size.h, work_info->is_snapshot);

	cxt->last_enable = 0;
	cxt->is_snapshot = work_info->is_snapshot;
	return rtn;
}

static cmr_s32 _touch_ae_process(struct ae_ctrl_cxt *cxt, struct ae_alg_calc_result *result)
{
	cmr_s32 rtn = AE_SUCCESS;
	struct ae_alg_calc_result *current_result = (struct ae_alg_calc_result *)result;

	/*****touch status0:touch before/release;1:touch doing; 2: toch done and AE stable*****/
	ISP_LOGV("TCAECTL_status and ae_stable is %d,%d", current_result->tcAE_status, current_result->wts.stable);
	if (1 == current_result->tcAE_status && 1 == current_result->wts.stable) {
		ISP_LOGV("TCAE_start lock ae and pause cnt is %d\n",cxt->cur_status.settings.pause_cnt);
		rtn = _set_pause(cxt);
		current_result->tcAE_status = 2;
	}
	cxt->cur_status.to_ae_state = current_result->tcAE_status;
	ISP_LOGV("TCAECTL_unlock is (%d,%d)\n", current_result->tcAE_status, current_result->tcRls_flag);

	if (0 == current_result->tcAE_status && 1 == current_result->tcRls_flag) {
		rtn = _set_restore_cnt(cxt);
		ISP_LOGV("TCAE_start release lock ae");
		current_result->tcRls_flag = 0;
	}
	ISP_LOGV("TCAECTL_rls_ae_lock is %d", cxt->cur_status.settings.lock_ae);

	return rtn;
}

static cmr_s32 ae_calculation_slow_motion(cmr_handle handle, cmr_handle param, cmr_handle result)
{
	cmr_s32 rtn = AE_ERROR;
	cmr_s32 i = 0;
	struct ae_ctrl_cxt *cxt = NULL;
	struct ae_alg_calc_param *current_status;
	struct ae_alg_calc_result *current_result;
	struct ae_misc_calc_in misc_calc_in = { 0 };
	struct ae_misc_calc_out misc_calc_out = { 0 };
	struct ae_calc_in *calc_in = NULL;
	struct ae_calc_out *calc_out = NULL;
	struct ae_exposure_param exp_param;

	cmr_s32 max_again;
	double rgb_gain_coeff;
	cmr_s32 expline;
	cmr_s32 dummy;
	cmr_s32 again;

	if ((NULL == param) || (NULL == result)) {
		ISP_LOGE("fail to get param, in %p out %p", param, result);
		return AE_PARAM_NULL;
	}

	rtn = _check_handle(handle);
	if (AE_SUCCESS != rtn) {
		ISP_LOGE("fail to check handle, ret: %d\n", rtn);
		return AE_HANDLER_NULL;
	}

	cxt = (struct ae_ctrl_cxt *)handle;
	ISP_LOGV("cam-id %d, is slow motion %d", cxt->camera_id, cxt->high_fps_info.is_high_fps);
	pthread_mutex_lock(&cxt->data_sync_lock);

	current_status = &cxt->sync_cur_status;
	current_result = &cxt->sync_cur_result;

	calc_in = (struct ae_calc_in *)param;
	calc_out = (struct ae_calc_out *)result;

	// acc_info_print(cxt);
	cxt->cur_status.awb_gain.b = calc_in->awb_gain_b;
	cxt->cur_status.awb_gain.g = calc_in->awb_gain_g;
	cxt->cur_status.awb_gain.r = calc_in->awb_gain_r;
	memcpy(cxt->sync_aem, calc_in->stat_img, 3 * 1024 * sizeof(cmr_u32));
	cxt->cur_status.stat_img = cxt->sync_aem;
	// get effective E&g
	cxt->cur_status.effect_expline  = cxt->exp_data.actual_data.exp_line;
	cxt->cur_status.effect_dummy  =cxt->exp_data.actual_data.dummy;
	cxt->cur_status.effect_gain = cxt->exp_data.actual_data.isp_gain * cxt->exp_data.actual_data.sensor_gain / 4096;

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
	current_status->target_lum = _calc_target_lum(cxt->cur_param->target_lum, cxt->cur_status.settings.ev_index, &cxt->cur_param->ev_table);
	current_status->target_lum_zone = cxt->stable_zone_ev[current_status->settings.ev_index];
	current_status->stride_config[0] = cxt->cnvg_stride_ev[current_status->settings.ev_index * 2];
	current_status->stride_config[1] = cxt->cnvg_stride_ev[current_status->settings.ev_index * 2 + 1];
	// ISP_LOGV("e_expline %d e_gain %d\r\n",
	// current_status.effect_expline,
	// current_status.effect_gain);
	// skip the first aem data (error data)
	if (current_status->ae_start_delay <= current_status->frame_id) {
		misc_calc_in.sync_settings = current_status;
		misc_calc_out.ae_output = &cxt->cur_result;
		// ISP_LOGV("ae_flash_status calc %d",
		// current_status.settings.flash);
		// ISP_LOGV("fd_ae: updata_flag:%d ef %d",
		// current_status->ae1_finfo.update_flag,
		// current_status->ae1_finfo.enable_flag);
		cxt->sync_cur_status.settings.min_fps = cxt->high_fps_info.min_fps;
		cxt->sync_cur_status.settings.max_fps = cxt->high_fps_info.max_fps;
		//ISP_LOGV("slow motion fps=(%d, %d)", cxt->cur_status.settings.min_fps, cxt->cur_status.settings.max_fps);
		rtn = ae_misc_calculation(cxt->misc_handle, &misc_calc_in, &misc_calc_out);
		cxt->cur_status.ae1_finfo.update_flag = 0;	// add by match box for fd_ae reset the status

		memcpy(current_result, &cxt->cur_result, sizeof(struct ae_alg_calc_result));
		make_isp_result(current_result, calc_out);

		/*just for debug: reset the status */
		if (1 == cxt->cur_status.settings.touch_scrn_status) {
			cxt->cur_status.settings.touch_scrn_status = 0;
		}

		cxt->exp_data.lib_data.exp_line = current_result->wts.cur_exp_line;
		cxt->exp_data.lib_data.gain = current_result->wts.cur_again;
		cxt->exp_data.lib_data.dummy = current_result->wts.cur_dummy;
		cxt->exp_data.lib_data.line_time = current_status->line_time;
		cxt->exp_data.lib_data.exp_time = current_result->wts.exposure_time;
		ae_result_update_to_sensor(cxt, &cxt->exp_data, 1);
	} else {
		;
	}
/***********************************************************/
/* send STAB notify to HAL */
    if (calc_out && calc_out->is_stab) {
        if (cxt->isp_ops.callback)
#ifdef CONFIG_CAMERA_DUAL_SYNC
		(*cxt->isp_ops.callback)(cxt->isp_ops.isp_handler, AE_CB_STAB_NOTIFY,NULL);
#else
	(*cxt->isp_ops.callback)(cxt->isp_ops.isp_handler, AE_CB_STAB_NOTIFY);
#endif
	}

    if (1 == cxt->debug_enable) {
	save_to_mlog_file(cxt, &misc_calc_out);
	}

	cxt->cur_status.frame_id++;
	//ISP_LOGV("AE_V2_frame id = %d\r\n", cxt->cur_status.frame_id);
	//ISP_LOGV("rt_expline %d rt_gain %d rt_dummy %d\r\n", rt.expline, rt.gain, rt.dummy);
	cxt->cur_status.ae_initial = AE_PARAM_NON_INIT;
ERROR_EXIT:
	pthread_mutex_unlock(&cxt->data_sync_lock);
	return rtn;
}

#ifdef CONFIG_CAMERA_DUAL_SYNC

static cmr_s32 ae_set_role(struct ae_ctrl_cxt *cxt_ptr, cmr_u8 ae_role)
{
	cmr_s32 ret = ISP_ERROR;

	if (!cxt_ptr) {
		ISP_LOGE("param %p is NULL error!", cxt_ptr);
		goto exit;
	}
	cxt_ptr->ae_role=ae_role;
	ISP_LOGI("camera_id %d master = %d", cxt_ptr->camera_id, cxt_ptr->ae_role);
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%d !!!", ret);
	return ret;
}

/* by james.xiong */
static void ae_mapping(struct ae_ctrl_cxt *cxt_ptr, struct match_data_param *multicam_aesync)
{
	struct sensor_otp_ae_info* ae_otp_master = NULL;
	struct sensor_otp_ae_info* ae_otp_slave = NULL;
	struct ae_alg_calc_result* ae_master_calc_out = NULL;
	struct ae_sync_out *slv_sync_result = NULL;
	cmr_u32 gain_master = 0;
	cmr_u32 gain_slave = 0;
	cmr_u32 exp_master_1x = 0;
	cmr_u32 exp_master_2x = 0;
	cmr_u32 exp_master_4x = 0;
	cmr_u32 exp_master_8x = 0;
	cmr_u32 exp_slave_1x = 0;
	cmr_u32 exp_slave_2x = 0;
	cmr_u32 exp_slave_4x = 0;
	cmr_u32 exp_slave_8x = 0;
	cmr_s16 master_line_time = 0;
	cmr_s16 slv_line_time = 0;
	cmr_u32 exp_line_slave = 0;
	cmr_u32 slave_dummy = 0;
	cmr_u32 tmp = 0;

	if (!cxt_ptr || !multicam_aesync) {
		ISP_LOGE("param is NULL error!");
		return;
	}

	ae_master_calc_out = &multicam_aesync->master_ae_info.ae_calc_result;
	slv_sync_result = &multicam_aesync->slave_ae_info.ae_sync_result;
	ae_otp_master = &(multicam_aesync->module_info.module_otp_info.master_ae_otp.otp_info);
	ae_otp_slave = &(multicam_aesync->module_info.module_otp_info.slave_ae_otp.otp_info);

	/* calculate gain */
	gain_master = ae_master_calc_out->wts.cur_again;
	gain_slave = gain_master;
	exp_master_1x = ae_otp_master->gain_1x_exp;
	exp_master_2x = ae_otp_master->gain_2x_exp;
	exp_master_4x = ae_otp_master->gain_4x_exp;
	exp_master_8x = ae_otp_master->gain_8x_exp;
	exp_slave_1x = ae_otp_slave->gain_1x_exp;
	exp_slave_2x = ae_otp_slave->gain_2x_exp;
	exp_slave_4x = ae_otp_slave->gain_4x_exp;
	exp_slave_8x = ae_otp_slave->gain_8x_exp;
	if (exp_master_1x && exp_master_2x && exp_master_4x && exp_master_8x
		&& exp_slave_1x && exp_slave_2x && exp_slave_4x && exp_slave_8x) {
		if (gain_master >= 8*128) {
			gain_slave = gain_master * exp_slave_8x / exp_master_8x;
		} else if (gain_master >= 4*128) {
			gain_slave = gain_master * (exp_slave_4x + exp_slave_8x) / (exp_master_4x + exp_master_8x);
		} else if (gain_master >= 2*128) {
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

	/* calculate dummy line */
	tmp = master_line_time * (ae_master_calc_out->wts.cur_exp_line + ae_master_calc_out->wts.cur_dummy);
	slave_dummy = tmp / slv_line_time - exp_line_slave;

	/*fulfill slave sync result */
	slv_sync_result->slave_ae.wts.cur_again= gain_slave;
	slv_sync_result->slave_ae.wts.exposure_time = ae_master_calc_out->wts.exposure_time;
	slv_sync_result->slave_ae.wts.cur_exp_line = exp_line_slave;
	slv_sync_result->slave_ae.wts.cur_dummy = slave_dummy;

	// TODO: calculate this value, or remove it!
	slv_sync_result->slave_ae.wts.cur_dgain= ae_master_calc_out->wts.cur_dgain;
	
	/* set flag to 1 if need update, otherwise  set to 0 */
	slv_sync_result->updata_flag = 1;
}

static cmr_s32 sprd_get_sync_info_from_lib(struct ae_ctrl_cxt *cxt_ptr, struct match_data_param *multicam_aesync)
{
	cmr_s32  ret = ISP_ERROR;

	ISP_LOGV("camera_id=%d, ae_role=%d", cxt_ptr->camera_id, cxt_ptr->ae_role);

	if (!cxt_ptr || !multicam_aesync) {
		ISP_LOGE("param is NULL error!");
		return ret;
	}

	/* use ae_mapping to update slave's ae info, update slave_ae_info.ae_sync_result */
	ae_mapping(cxt_ptr, multicam_aesync);

	/* store calculated values into ae_calc_result for reference */
	// TODO: check fields
	multicam_aesync->slave_ae_info.ae_calc_result.wts.cur_again =
		multicam_aesync->slave_ae_info.ae_sync_result.slave_ae.wts.cur_again;
	multicam_aesync->slave_ae_info.ae_calc_result.wts.cur_exp_line =
		multicam_aesync->slave_ae_info.ae_sync_result.slave_ae.wts.cur_exp_line;
	multicam_aesync->slave_ae_info.ae_calc_result.wts.cur_dummy =
		multicam_aesync->slave_ae_info.ae_sync_result.slave_ae.wts.cur_dummy;

	return ret;
}


static cmr_s32 dualcamera_aesync_calc(struct ae_ctrl_cxt *cxt,  struct match_data_param *dualcam_aesync)
{
	cmr_s32  ret = ISP_ERROR;

	if (!cxt || !dualcam_aesync) {
		ISP_LOGE("param is NULL error!");
		return ret;
	}

	// TODO: debug sensor_role=0
	ISP_LOGV("is_multi_mode=%d, ae_role=%d, sensor_role=%d",
		cxt->is_multi_mode,
		cxt->ae_role,
		cxt->sensor_role);

	//get slave sensor aeinfo
	if (cxt->sensor_role)
	{
		ret = cxt->ptr_isp_br_ioctrl(cxt->camera_id,
			GET_SLAVE_AECALC_RESULT,
			NULL,
			&dualcam_aesync->slave_ae_info.ae_calc_result);

		ret = cxt->ptr_isp_br_ioctrl(cxt->camera_id,
			GET_MASTER_AECALC_RESULT,
			NULL,
			&dualcam_aesync->master_ae_info.ae_calc_result);

		ret= cxt->ptr_isp_br_ioctrl(cxt->camera_id,
			GET_ALL_MODULE_AND_OTP,
			NULL,
			&dualcam_aesync->module_info);
	}

	//need check ret
	ret = sprd_get_sync_info_from_lib(cxt, dualcam_aesync);

	if (cxt->sensor_role)
	{
		//save ae sync setting to slave sensor
		ret = cxt->ptr_isp_br_ioctrl(cxt->camera_id,
			SET_SLAVE_AESYNC_SETTING,
			&dualcam_aesync->slave_ae_info.ae_sync_result,
			NULL);
	}

	return ret;
}
#endif

cmr_s32 ae_calculation(cmr_handle handle, cmr_handle param, cmr_handle result)
{
	cmr_s32 rtn = AE_ERROR;
	cmr_s32 i = 0;
	char ae_exp[PROPERTY_VALUE_MAX];
	char ae_gain[PROPERTY_VALUE_MAX];
	struct ae_ctrl_cxt *cxt = NULL;
	struct ae_alg_calc_param *current_status = NULL;
	struct ae_alg_calc_result *current_result = NULL;
	struct ae_alg_calc_param *alg_status_ptr = NULL;	//DEBUG
	struct ae_misc_calc_in misc_calc_in = { 0 };
	struct ae_misc_calc_out misc_calc_out = { 0 };
	struct ae_calc_in *calc_in = NULL;
	struct ae_calc_out *calc_out = NULL;

	if ((NULL == param) || (NULL == result)) {
		ISP_LOGE("fail to get param, in %p out %p", param, result);
		return AE_PARAM_NULL;
	}

	rtn = _check_handle(handle);
	if (AE_SUCCESS != rtn) {
		ISP_LOGE("fail to check handle, ret: %d\n", rtn);
		return AE_HANDLER_NULL;
	}
	cxt = (struct ae_ctrl_cxt *)handle;

	pthread_mutex_lock(&cxt->data_sync_lock);
	if (cxt->bypass) {
		_set_pause(cxt);
	}

	calc_in = (struct ae_calc_in *)param;
	calc_out = (struct ae_calc_out *)result;

	// acc_info_print(cxt);
	cxt->cur_status.awb_gain.b = calc_in->awb_gain_b;
	cxt->cur_status.awb_gain.g = calc_in->awb_gain_g;
	cxt->cur_status.awb_gain.r = calc_in->awb_gain_r;
	memcpy(cxt->sync_aem, calc_in->stat_img, 3 * 1024 * sizeof(cmr_u32));
	cxt->cur_status.stat_img = cxt->sync_aem;
	
	// get effective E&g
	cxt->cur_status.effect_expline  = cxt->exp_data.actual_data.exp_line;
	cxt->cur_status.effect_dummy  =cxt->exp_data.actual_data.dummy;
	cxt->cur_status.effect_gain = cxt->exp_data.actual_data.isp_gain * cxt->exp_data.actual_data.sensor_gain / 4096;
	
	cxt->sync_aem[3 * 1024] = cxt->cur_status.frame_id;
	cxt->sync_aem[3 * 1024 + 1] = cxt->cur_status.effect_expline;
	cxt->sync_aem[3 * 1024 + 2] = cxt->cur_status.effect_dummy;
	cxt->sync_aem[3 * 1024 + 3] = cxt->cur_status.effect_gain;

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
	ISP_LOGV("AE_TABLE IS %p\n",alg_status_ptr->ae_table );
	//alg_status_ptr->ae_table = &cxt->cur_param->ae_table[alg_status_ptr->settings.flicker][AE_ISO_AUTO];
	alg_status_ptr->ae_table->min_index = 0;	//AE table start index = 0
#endif
	/*
	   due to set_scene_mode just be called in ae_sprd_calculation,
	   and the prv_status just save the normal scene status
 	*/
	if (AE_SCENE_NORMAL == cxt->sync_cur_status.settings.scene_mode) {
		cxt->prv_status = cxt->cur_status;
		if (AE_SCENE_NORMAL != cxt->cur_status.settings.scene_mode) {
			cxt->prv_status.settings.scene_mode = AE_SCENE_NORMAL;
		}
	}
	{
		cmr_s8 cur_mod = cxt->sync_cur_status.settings.scene_mode;
		cmr_s8 nx_mod = cxt->cur_status.settings.scene_mode;
		if (nx_mod != cur_mod) {
			ISP_LOGV("before set scene mode: \n");
			_printf_status_log(cxt, cur_mod, &cxt->cur_status);
			_set_scene_mode(cxt, cur_mod, nx_mod);
			ISP_LOGV("after set scene mode: \n");
			_printf_status_log(cxt, nx_mod, &cxt->cur_status);
		}
	}
	// END

	rtn = _ae_pre_process(cxt);
	_set_flash_calibration_script(cxt);/*for flash calibration*/
	_set_led(cxt);
	ae_hdr_ctrl(cxt, param);

	current_status = &cxt->sync_cur_status;
	current_result = &cxt->sync_cur_result;
	memcpy(current_status, &cxt->cur_status, sizeof(struct ae_alg_calc_param));
	memcpy(&cxt->cur_result, current_result, sizeof(struct ae_alg_calc_result));	

	if ((current_status->ae_start_delay <= current_status->frame_id)) {
		misc_calc_in.sync_settings = current_status;
		misc_calc_out.ae_output = &cxt->cur_result;
		rtn = ae_misc_calculation(cxt->misc_handle, &misc_calc_in, &misc_calc_out);
		if (rtn) {
			ISP_LOGE("fail to calc ae misc");
			rtn = AE_ERROR;
			goto ERROR_EXIT;
		}
		cxt->cur_status.ae1_finfo.update_flag = 0;	// add by match box for fd_ae reset the status
		memset((cmr_handle) & cxt->cur_status.ae1_finfo.cur_info, 0, sizeof(cxt->cur_status.ae1_finfo.cur_info));

		ISP_LOGV("idx4mx_flag:%d,tarLF:%d,tarLO:%d\n",calc_out->flag4idx,calc_out->target_lum,calc_out->target_lum_ori);
		{
			/*just for debug: reset the status */
			if (1 == cxt->cur_status.settings.touch_scrn_status) {
				cxt->cur_status.settings.touch_scrn_status = 0;
			}
		}
		// wait-pause function
		rtn = _ae_post_process(cxt);
		memcpy(current_result, &cxt->cur_result, sizeof(struct ae_alg_calc_result));
		make_isp_result(current_result, calc_out);
	}

/***********************************************************/
/*update parameters to sensor*/

#ifdef CONFIG_CAMERA_DUAL_SYNC
		struct match_data_param dualcam_aesync;
		if(cxt->is_multi_mode && cxt->ae_role)
		{
			memcpy(&dualcam_aesync.master_ae_info.ae_calc_result,
				current_result, sizeof(dualcam_aesync.master_ae_info.ae_calc_result));
			ISP_LOGI("[master] cur_fps=%f, gain=%u, expline=%u, exptime=%u, dummy=%u",
				dualcam_aesync.master_ae_info.ae_calc_result.wts.cur_fps,
				dualcam_aesync.master_ae_info.ae_calc_result.wts.cur_again,
				dualcam_aesync.master_ae_info.ae_calc_result.wts.cur_exp_line,
				dualcam_aesync.master_ae_info.ae_calc_result.wts.exposure_time,
				dualcam_aesync.master_ae_info.ae_calc_result.wts.cur_dummy);
			/* store calculated master's ae value */
			rtn = cxt->ptr_isp_br_ioctrl(cxt->camera_id, SET_MASTER_AECALC_RESULT, current_result, NULL);

			/* use stored master's ae value to calculate slave's ae value */
			rtn = dualcamera_aesync_calc(cxt, &dualcam_aesync);
		
		}
		else  if(cxt->is_multi_mode && !cxt->ae_role)
		{
			/* use slave's ae_sync_result as final ae value */
			rtn = cxt->ptr_isp_br_ioctrl(cxt->camera_id, GET_SLAVE_AESYNC_SETTING,
				NULL, &dualcam_aesync.slave_ae_info.ae_sync_result.slave_ae);
			ISP_LOGI("[slave ] cur_fps=%f, gain=%u, expline=%u, exptime=%u, dummy=%u",
				cxt->cur_result.wts.cur_fps,
				dualcam_aesync.slave_ae_info.ae_sync_result.slave_ae.wts.cur_again,
				dualcam_aesync.slave_ae_info.ae_sync_result.slave_ae.wts.cur_exp_line,
				dualcam_aesync.slave_ae_info.ae_sync_result.slave_ae.wts.exposure_time,
				dualcam_aesync.slave_ae_info.ae_sync_result.slave_ae.wts.cur_dummy);

			current_result->wts.cur_exp_line= dualcam_aesync.slave_ae_info.ae_sync_result.slave_ae.wts.cur_exp_line;
			current_result->wts.exposure_time=  dualcam_aesync.slave_ae_info.ae_sync_result.slave_ae.wts.exposure_time;
			current_result->wts.cur_again= dualcam_aesync.slave_ae_info.ae_sync_result.slave_ae.wts.cur_again;
			current_result->wts.cur_dummy = dualcam_aesync.slave_ae_info.ae_sync_result.slave_ae.wts.cur_dummy;			

			
		}
#endif

/*update parameters to sensor*/
	cxt->exp_data.lib_data.exp_line = current_result->wts.cur_exp_line;
	cxt->exp_data.lib_data.exp_time = current_result->wts.exposure_time;
	cxt->exp_data.lib_data.gain = current_result->wts.cur_again;
	cxt->exp_data.lib_data.dummy = current_result->wts.cur_dummy;
	cxt->exp_data.lib_data.line_time = current_status->line_time;
	rtn = ae_result_update_to_sensor(cxt, &cxt->exp_data, 0);

#ifdef CONFIG_CAMERA_DUAL_SYNC
	if(cxt->is_multi_mode )
	{
		ISP_LOGD("notify ae info, camear id=%d",cxt->camera_id);

		(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, AE_CB_AE_CALCOUT_NOTIFY, calc_out);

	}
#endif
	rtn = _touch_ae_process(cxt, current_result);
/* send STAB notify to HAL */
	if (calc_out && calc_out->is_stab) {
        if (cxt->isp_ops.callback)
#ifdef  CONFIG_CAMERA_DUAL_SYNC
		(*cxt->isp_ops.callback)(cxt->isp_ops.isp_handler, AE_CB_STAB_NOTIFY, NULL);
#else
		(*cxt->isp_ops.callback)(cxt->isp_ops.isp_handler, AE_CB_STAB_NOTIFY);
#endif
        }
/***********************************************************/
/*display the AE running status*/
    if (1 == cxt->debug_enable) {
	save_to_mlog_file(cxt, &misc_calc_out);
    }
/***********************************************************/
	cxt->cur_status.frame_id++;
	//ISP_LOGV("AE_V2_frame id = %d\r\n", cxt->cur_status.frame_id);
	//ISP_LOGV("rt_expline %d rt_gain %d rt_dummy %d\r\n", rt.expline, rt.gain, rt.dummy);

	cxt->cur_status.ae_initial = AE_PARAM_NON_INIT;
ERROR_EXIT:
	pthread_mutex_unlock(&cxt->data_sync_lock);
	return rtn;
}

cmr_s32 ae_sprd_calculation(cmr_handle handle, cmr_handle param, cmr_handle result)
{
	cmr_s32 rtn = AE_ERROR;
	struct ae_ctrl_cxt *cxt = (struct ae_ctrl_cxt *)handle;

	if (cxt->last_enable) {
		ISP_LOGI("video_stop to video_start, ae calc skip\n");
		return AE_SUCCESS;
	}

	if (cxt->high_fps_info.is_high_fps)
		rtn = ae_calculation_slow_motion(handle, param, result);
	else
		rtn = ae_calculation(handle, param, result);

	return rtn;
}

cmr_s32 ae_sprd_io_ctrl(cmr_handle handle, cmr_s32 cmd, cmr_handle param, cmr_handle result)
{
	cmr_s32 rtn = AE_SUCCESS;
	struct ae_ctrl_cxt *cxt = NULL;

	rtn = _check_handle(handle);
	if (AE_SUCCESS != rtn) {
		ISP_LOGE("fail to check handle %p", handle);
		return AE_HANDLER_NULL;
	}

	cxt = (struct ae_ctrl_cxt *)handle;
	pthread_mutex_lock(&cxt->data_sync_lock);
	switch (cmd) {
	case AE_SET_PROC:
		_sof_handler(cxt);
		break;

	case AE_SET_DC_DV:
		if (param) {
			if (1 == *(cmr_u32 *) param)
				cxt->cur_status.settings.work_mode = AE_WORK_MODE_VIDEO;
			else
				cxt->cur_status.settings.work_mode = AE_WORK_MODE_COMMON;
		ISP_LOGI("AE_SET_DC_DV %d", cxt->cur_status.settings.work_mode);
		}
		break;

	case AE_SET_WORK_MODE:
		break;

	case AE_SET_SCENE_MODE:
		if (param) {
			struct ae_set_scene *scene_mode = param;

			if (scene_mode->mode < AE_SCENE_MAX) {
				cxt->cur_status.settings.scene_mode = (cmr_s8) scene_mode->mode;
			}
		ISP_LOGI("AE_SET_SCENE %d\n", cxt->cur_status.settings.scene_mode);
		}
		break;

	case AE_SET_ISO:
		if (param) {
			struct ae_set_iso *iso = param;

			if (iso->mode < AE_ISO_MAX) {
				cxt->cur_status.settings.iso = iso->mode;
			}
		ISP_LOGI("AE_SET_ISO %d\n", cxt->cur_status.settings.iso);
		}
		break;

	case AE_GET_ISO:
		if (result) {
			rtn = _get_iso(cxt, result);
		}
		break;

	case AE_SET_FLICKER:
		if (param) {
			struct ae_set_flicker *flicker = param;
			if (flicker->mode < AE_FLICKER_OFF) {
				cxt->cur_status.settings.flicker = flicker->mode;
				cxt->cur_status.ae_table = &cxt->cur_param->ae_table[cxt->cur_status.settings.flicker][AE_ISO_AUTO];
			}
		ISP_LOGI("AE_SET_FLICKER %d\n", cxt->cur_status.settings.flicker);
		}
		break;

	case AE_SET_WEIGHT:
		if (param) {
			struct ae_set_weight *weight = param;

			if (weight->mode < AE_WEIGHT_MAX) {
				cxt->cur_status.settings.metering_mode = weight->mode;
			}
		ISP_LOGI("AE_SET_WEIGHT %d\n", cxt->cur_status.settings.metering_mode);
		}
		break;

	case AE_SET_TOUCH_ZONE:
		if (param) {
			struct ae_set_tuoch_zone *touch_zone = param;
			if ((touch_zone->touch_zone.w > 1) &&
				(touch_zone->touch_zone.h > 1)) {
				cxt->cur_result.wts.stable = 0;
				cxt->cur_status.touch_scrn_win = touch_zone->touch_zone;
				cxt->cur_status.settings.touch_scrn_status = 1;

				if (cxt->cur_status.to_ae_state == 2)
					rtn = _set_restore_cnt(cxt);
				ISP_LOGI("AE_SET_TOUCH_ZONE ae triger %d", cxt->cur_status.settings.touch_scrn_status);
			} else {
				ISP_LOGW("AE_SET_TOUCH_ZONE touch ignore\n");
			}
		}
		break;

	case AE_SET_EV_OFFSET:
		if (param) {
			struct ae_set_ev *ev = param;

			if (ev->level < AE_LEVEL_MAX) {
				cxt->cur_status.settings.ev_index = ev->level;
				cxt->cur_status.target_lum = _calc_target_lum(cxt->cur_param->target_lum,
					cxt->cur_status.settings.ev_index, &cxt->cur_param->ev_table);
				cxt->cur_status.target_lum_zone = cxt->stable_zone_ev[cxt->cur_status.settings.ev_index];
				cxt->cur_status.stride_config[0] = cxt->cnvg_stride_ev[cxt->cur_status.settings.ev_index * 2];
				cxt->cur_status.stride_config[1] = cxt->cnvg_stride_ev[cxt->cur_status.settings.ev_index * 2 + 1];
			}
		ISP_LOGI("AE_SET_EV_OFFSET %d", cxt->cur_status.settings.ev_index);
		}
		break;

	case AE_SET_FPS:
		if (param) {
			if (!cxt->high_fps_info.is_high_fps) {
				struct ae_set_fps *fps = param;
				cxt->fps_range.min = fps->min_fps;
				cxt->fps_range.max = fps->max_fps;
				ISP_LOGI("AE_SET_FPS (%d, %d)", fps->min_fps, fps->max_fps);
			}
		}
		break;

	case AE_SET_WAIT_PAUSE:
		if (0 == cxt->cur_status.settings.pause_cnt) {
//			cxt->bakup_ae_lock_status = cxt->cur_status.settings.lock_ae;
		}
		cxt->cur_status.settings.lock_ae = AE_STATE_WAIT_PAUSE;
		cxt->cur_status.settings.pause_cnt++;
		break;

	case AE_SET_PAUSE:
		rtn = _set_pause(cxt);
		break;

	case AE_SET_RESTORE_CNT:
		rtn = _set_restore_cnt(cxt);
		break;

	case AE_SET_RESTORE:
		rtn = _set_restore_cnt(cxt);
		break;

	case AE_SET_FORCE_PAUSE:
		cxt->cur_status.settings.lock_ae = AE_STATE_LOCKED;
		if (param) {
			struct ae_exposure_gain *set = param;

			cxt->cur_status.settings.manual_mode = set->set_mode;
			cxt->cur_status.settings.table_idx = set->index;
			cxt->cur_status.settings.exp_line = (cmr_u32) (10 * set->exposure / cxt->cur_status.line_time + 0.5);
			cxt->cur_status.settings.gain = set->again;
		}
		break;

	case AE_SET_FORCE_RESTORE:
		cxt->cur_status.settings.lock_ae = AE_STATE_NORMAL;
		cxt->cur_status.settings.pause_cnt = 0;
		break;

	case AE_SET_FLASH_NOTICE:
		rtn = _set_flash_notice(cxt, param);
		break;

	case AE_GET_FLASH_EFFECT:
		if (result) {
			cmr_u32 *effect = (cmr_u32 *) result;

			*effect = cxt->flash_effect;
			ISP_LOGI("flash_effect %d", *effect);
		}
		break;

	case AE_GET_AE_STATE:
		/*
		 * if (result) { cmr_u32 *ae_state =
		 * (cmr_u32*)result; *ae_state = cxt->ae_state;
		 * }
		 */
		break;

	case AE_GET_FLASH_EB:
		if (result) {
			cmr_u32 *flash_eb = (cmr_u32 *) result;
			cmr_s32 bv = 0;
			cmr_s32 bv_thr = cxt->flash_on_off_thr;

			if (0 >= bv_thr)
				bv_thr = AE_FLASH_ON_OFF_THR;

			rtn = _get_bv_by_lum_new(cxt, &bv);

			if (bv < bv_thr)
				*flash_eb = 1;
			else
				*flash_eb = 0;

			ISP_LOGI("AE_GET_FLASH_EB: flash_eb=%d, bv=%d, thr=%d", *flash_eb, bv, bv_thr);
		}
		break;

	case AE_SET_FLASH_ON_OFF_THR:
		if (param) {
			cxt->flash_on_off_thr = *(cmr_s32 *) param;
			if (0 >= cxt->flash_on_off_thr) {
				cxt->flash_on_off_thr = AE_FLASH_ON_OFF_THR;
			}
			ISP_LOGI("AE_SET_FLASH_ON_OFF_THR: thr %d", cxt->flash_on_off_thr);
		}
		break;

	case AE_SET_TUNING_EB:
		break;

	case AE_GET_BV_BY_GAIN:
	case AE_GET_BV_BY_GAIN_NEW:
		if (result) {
			cmr_s32 *bv = (cmr_s32 *) result;

			*bv = cxt->cur_result.wts.cur_again;
		}
		break;

	case AE_GET_FLASH_ENV_RATIO:
		if (result) {
			float *captureFlashEnvRatio = (float *) result;
			*captureFlashEnvRatio = cxt->flash_esti_result.captureFlashRatio;
		}
		break;
	case AE_GET_FLASH_ONE_OF_ALL_RATIO:
		if (result) {
			float *captureFlash1ofALLRatio = (float *) result;
			*captureFlash1ofALLRatio = cxt->flash_esti_result.captureFlash1Ratio;
		}
		break;
	case AE_GET_EV:
		if (result) {
			cmr_u32 i = 0x00;
			struct ae_ev_table *ev_table = &cxt->cur_param->ev_table;
			cmr_s32 target_lum = cxt->cur_param->target_lum;
			struct ae_get_ev *ev_info = (struct ae_get_ev *)result;
			ev_info->ev_index = cxt->cur_status.settings.ev_index;

			for (i = 0; i < ev_table->diff_num; i++) {
				ev_info->ev_tab[i] = target_lum + ev_table->lum_diff[i];
			}
			// ISP_LOGV("AE default target %d
			// cur_ev_level %d\r\n", target_lum,
			// ev_info->ev_index);
			rtn = AE_SUCCESS;
		}
		break;

	case AE_GET_BV_BY_LUM:
		if (result) {
			cmr_s32 *bv = (cmr_s32 *) result;

			rtn = _get_bv_by_lum(cxt, bv);
		}
		break;

	case AE_GET_BV_BY_LUM_NEW:
		if (result) {
			cmr_s32 *bv = (cmr_s32 *) result;

			rtn = _get_bv_by_lum_new(cxt, bv);
		}
		break;

	case AE_SET_STAT_TRIM:
		if (param) {
			struct ae_trim *trim = (struct ae_trim *)param;

			rtn = _set_scaler_trim(cxt, trim);
		}
		break;

	case AE_SET_G_STAT:
		if (param) {
			struct ae_stat_mode *stat_mode = (struct ae_stat_mode *)param;

			rtn = _set_g_stat(cxt, stat_mode);
		}
		break;

	case AE_GET_LUM:
		if (result) {
			cmr_u32 *lum = (cmr_u32 *) result;

			*lum = cxt->cur_result.cur_lum;
		}
		break;

	case AE_SET_TARGET_LUM:
		if (param) {
			cmr_u32 *lum = (cmr_u32 *) param;

			cxt->cur_status.target_lum = *lum;
		}
		break;

	case AE_SET_SNAPSHOT_NOTICE:
		if (param) {
			rtn = _set_snapshot_notice(cxt, param);
		}
		break;

	case AE_GET_MONITOR_INFO:
		if (result) {
			struct ae_monitor_info *info = result;

			info->win_size = cxt->monitor_unit.win_size;
			info->win_num = cxt->monitor_unit.win_num;
			info->trim = cxt->monitor_unit.trim;
		}
		break;

	case AE_GET_FLICKER_MODE:
		if (result) {
			cmr_u32 *mode = result;

			*mode = cxt->cur_status.settings.flicker;
		}
		break;

	case AE_SET_ONLINE_CTRL:
		if (param) {
			rtn = _tool_online_ctrl(cxt, param, result);
		}
		break;

	case AE_SET_EXP_GAIN:
		break;

	case AE_SET_EXP_ANIT:
		break;

	case AE_SET_FD_ENABLE:
		if (param) {
			cxt->fdae.allow_to_work = *((cmr_u32 *) param);
		}
		break;

	case AE_SET_FD_PARAM:
		if (param) {
			struct ae_fd_param *fd = (struct ae_fd_param *)param;
			cmr_s32 i = 0;
			cxt->cur_status.ae1_finfo.update_flag = 1;
			if(fd->face_num > 0){
				cxt->cur_status.ae1_finfo.img_width = fd->width;
				cxt->cur_status.ae1_finfo.img_height =fd->height;
				cxt->cur_status.ae1_finfo.cur_info.face_num = fd->face_num;
				ISP_LOGI("FD_CTRL_NUM:%d,flag is %d\n", cxt->cur_status.ae1_finfo.cur_info.face_num,
					cxt->cur_status.ae1_finfo.update_flag);
				for(i = 0; i < fd->face_num; i++){
					cxt->cur_status.ae1_finfo.cur_info.face_area[i].start_x = fd->face_area[i].rect.start_x;
					cxt->cur_status.ae1_finfo.cur_info.face_area[i].start_y = fd->face_area[i].rect.start_y;
					cxt->cur_status.ae1_finfo.cur_info.face_area[i].end_x = fd->face_area[i].rect.end_x;
					cxt->cur_status.ae1_finfo.cur_info.face_area[i].end_y = fd->face_area[i].rect.end_y;
					cxt->cur_status.ae1_finfo.cur_info.face_area[i].pose = fd->face_area[i].pose;
					ISP_LOGI("FD_CTRL_LTN:start(x,y):(%d,%d),end(x,y):(%d,%d),pose:%d\n",
						cxt->cur_status.ae1_finfo.cur_info.face_area[i].start_x,
						cxt->cur_status.ae1_finfo.cur_info.face_area[i].start_y,
						cxt->cur_status.ae1_finfo.cur_info.face_area[i].end_x,
						cxt->cur_status.ae1_finfo.cur_info.face_area[i].end_y,
						cxt->cur_status.ae1_finfo.cur_info.face_area[i].pose);
				}
			}else{
				cxt->cur_status.ae1_finfo.cur_info.face_num = 0;
				for(i = 0; i < fd->face_num; i++){
					cxt->cur_status.ae1_finfo.cur_info.face_area[i].start_x = 0;
					cxt->cur_status.ae1_finfo.cur_info.face_area[i].start_y = 0;
					cxt->cur_status.ae1_finfo.cur_info.face_area[i].end_x = 0;
					cxt->cur_status.ae1_finfo.cur_info.face_area[i].end_y = 0;
					cxt->cur_status.ae1_finfo.cur_info.face_area[i].pose = -1;
				}
			}
		}else{
			ISP_LOGE("fail to fd, param %p", param);
			return AE_ERROR;
		}
		break;

#ifdef CONFIG_CAMERA_DUAL_SYNC
	case AE_CTRL_SET_ROLE:
		if (param) {
			cmr_u8 ae_role= *((cmr_u8 *) param);
			rtn = ae_set_role(cxt, ae_role);
		}
		break;
#endif


	case AE_GET_GAIN:
		if (result) {
			*(float *)result = cxt->cur_result.wts.cur_again;
		}
		break;

	case AE_GET_EXP:
		if (result) {
			*(float *)result = cxt->cur_result.wts.exposure_time / 10000000.0;
			// ISP_LOGV("wts.exposure_time %1.f
			// %d\r\n", *(float *)result,
			// cxt->cur_result.wts.exposure_time);
		}
		break;

	case AE_GET_FLICKER_SWITCH_FLAG:
		if (param) {
			rtn = _get_flicker_switch_flag(cxt, param);
		}
		break;

	case AE_GET_CUR_WEIGHT:
		*(cmr_s8 *) result = cxt->cur_status.settings.metering_mode;
		break;

	case AE_GET_SKIP_FRAME_NUM:
		rtn = _get_skip_frame_num(cxt, param, result);
		break;

	case AE_SET_NIGHT_MODE:
		cxt->cur_status.settings.scene_mode = AE_SCENE_NIGHT;
		break;

	case AE_SET_NORMAL_MODE:
		cxt->cur_status.settings.scene_mode = AE_SCENE_NORMAL;
		break;

	case AE_SET_BYPASS:
		if (param) {
			if (1 == *(cmr_s32 *) param) {
				cxt->bypass = 1;
			} else {
				cxt->cur_status.settings.lock_ae = AE_STATE_NORMAL;
				cxt->cur_status.settings.pause_cnt = 0;
				cxt->bypass = 0;
			}
		}
		break;

	case AE_GET_NORMAL_INFO:
		if (result) {
			rtn = _get_normal_info(cxt, result);
		}
		break;

	case AE_SET_SPORT_MODE:
		cxt->cur_status.settings.scene_mode = AE_SCENE_SPORT;
		break;

	case AE_SET_PANORAMA_START:
		cxt->cur_status.settings.scene_mode = AE_SCENE_PANORAMA;
		break;

	case AE_SET_PANORAMA_STOP:
		cxt->cur_status.settings.scene_mode = AE_SCENE_NORMAL;
		break;

	case AE_SET_FORCE_QUICK_MODE:
		break;

	case AE_SET_EXP_TIME:
		if (param) {
			_set_sensor_exp_time(cxt, param);
		}
		break;

	case AE_SET_SENSITIVITY:
		if (param) {
			_set_sensor_sensitivity(cxt, param);
		}
		break;

	case AE_GET_EXP_TIME:
		if (result) {
			*(cmr_u32 *) result = cxt->cur_result.wts.exposure_time;
		}
		break;

	case AE_GET_SENSITIVITY:
		if (param) {
			*(cmr_u32 *) param = cxt->cur_result.wts.cur_again * 50 / 128;
		}
		break;

	case AE_GET_DEBUG_INFO:
		if (result) {
			rtn = ae_get_debug_info(cxt, result);
		}
		break;

	case AE_GET_EM_PARAM:
		if (result) {
			rtn = ae_get_debug_info_for_display(cxt, result);
		}
		break;

	case AE_INTELLIGENT_OPEN:
		cxt->cur_status.settings.intelligent_module = 1;
		break;

	case AE_INTELLIGENT_CLOSE:
		cxt->cur_status.settings.intelligent_module = 0;
		break;

	case AE_VIDEO_STOP:
		_set_ae_video_stop(cxt);
		break;

	case AE_VIDEO_START:
		if (param) {
			rtn = _set_ae_video_start(cxt, param);
		}
		break;

	case AE_GET_AF_INFO:
		if (param) {
			cxt->cur_status.settings.af_info = *((cmr_s8 *) param);
		} else {
			cxt->cur_status.settings.af_info = 0;
		}
		break;

	case AE_HDR_START:
		if (param) {
			struct ae_hdr_param	*hdr_param = (struct ae_hdr_param *)param;
			cxt->hdr_enable = hdr_param->hdr_enable;
			cxt->hdr_cb_cnt = hdr_param->ev_effect_valid_num - 1;
			cxt->hdr_frame_cnt = 0;
			if(cxt->hdr_enable){
				cxt->hdr_flag = 3;
				cxt->hdr_base_ae_idx = cxt->sync_cur_result.wts.cur_index;
			}else{
				cxt->cur_status.settings.lock_ae = AE_STATE_NORMAL;
			}
		ISP_LOGI("AE_SET_HDR: hdr_enable %d, base_ae_idx %d", cxt->hdr_enable, cxt->hdr_base_ae_idx);
		}
		break;

	case AE_GET_FLASH_WB_GAIN:
		if (result) {
			if (cxt->flash_ver) {
				struct ae_awb_gain *flash_awb_gain = (struct ae_awb_gain*)result;
				flash_awb_gain->r = cxt->flash_esti_result.captureRGain;
				flash_awb_gain->g = cxt->flash_esti_result.captureGGain;
				flash_awb_gain->b = cxt->flash_esti_result.captureBGain;
				ISP_LOGI("dual flash: awb gain: %d, %d, %d\n", flash_awb_gain->r,
					flash_awb_gain->g, flash_awb_gain->b);
				rtn = AE_SUCCESS;
			} else {
				rtn  =AE_ERROR;
			}
		} else {
			rtn  =AE_ERROR;
		}
		break;

	case AE_GET_FPS:
		if (result) {
			cmr_u32 *fps = (cmr_u32*)result;
			*fps = (cmr_u32)(cxt->sync_cur_result.wts.cur_fps + 0.5);
			ISP_LOGI("fps: %f\n", cxt->sync_cur_result.wts.cur_fps);
		} else {
			ISP_LOGE("result pointer is NULL");
			rtn  =AE_ERROR;
		}
		break;

	case AE_CAF_LOCKAE_START:
		cxt->target_lum_zone_bak = cxt->cur_status.target_lum_zone;

		if (cxt->cur_result.wts.stable)
		{
			cxt->cur_status.target_lum_zone = cxt->stable_zone_ev[15];
			if (cxt->cur_status.target_lum_zone < cxt->target_lum_zone_bak)
			{
				cxt->cur_status.target_lum_zone = cxt->target_lum_zone_bak;
			}
		}
		break;

	case AE_CAF_LOCKAE_STOP:
		cxt->cur_status.target_lum_zone = cxt->target_lum_zone_bak;
		break;

	case AE_GET_LEDS_CTRL:
		if (result) {
			struct ae_leds_ctrl *ae_leds_ctrl = (struct ae_leds_ctrl *)result;

			ae_leds_ctrl->led0_ctrl = cxt->ae_leds_ctrl.led0_ctrl;
			ae_leds_ctrl->led1_ctrl = cxt->ae_leds_ctrl.led1_ctrl;
			ISP_LOGI("AE_SET_LED led0_enable=%d, led1_enable=%d", ae_leds_ctrl->led0_ctrl,
			ae_leds_ctrl->led1_ctrl);
		}
		break;

	default:
		rtn = AE_ERROR;
		break;
	}
	pthread_mutex_unlock(&cxt->data_sync_lock);

	return rtn;
}

cmr_s32 ae_sprd_deinit(cmr_handle handle, cmr_handle in_param, cmr_handle out_param)
{
	cmr_s32 rtn = AE_SUCCESS;
	struct ae_ctrl_cxt *cxt = NULL;
	UNUSED(in_param);
	UNUSED(out_param);

	rtn = _check_handle(handle);
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

	if (AE_SUCCESS != rtn) {
		ISP_LOGE("fail to deinit ae misc ");
		rtn = AE_ERROR;
	}
	//ae_seq_reset(cxt->seq_handle);
	rtn = s_q_close(cxt->seq_handle);

	if (cxt->debug_enable) {
		if (cxt->debug_info_handle) {
			debug_file_deinit((debug_handle_t) cxt->debug_info_handle);
			cxt->debug_info_handle = (cmr_u32) NULL;
		}
	}

	pthread_mutex_destroy(&cxt->data_sync_lock);
	ISP_LOGI("cam-id %d", cxt->camera_id);
	free(cxt);
	ISP_LOGI("done");

	return rtn;
}

struct adpt_ops_type ae_sprd_adpt_ops_ver0 = {
	.adpt_init = ae_sprd_init,
	.adpt_deinit = ae_sprd_deinit,
	.adpt_process = ae_sprd_calculation,
	.adpt_ioctrl = ae_sprd_io_ctrl,
};
