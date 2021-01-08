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
#define LOG_TAG "isp_alg_fw"

#include <math.h>
#include <dlfcn.h>
#include <cutils/properties.h>
#include <cutils/sched_policy.h>
#include <sys/resource.h>
#include <system/thread_defs.h>

#include "isp_alg_fw.h"
#include "cmr_msg.h"
#include "cmr_prop.h"
#include "isp_dev_access.h"
#include "isp_debug.h"
#include "isp_pm.h"
#include "awb.h"
#include "awblib.h"
#include "af_ctrl.h"
#include "ae_ctrl.h"
#include "afl_ctrl.h"
#include "awb_ctrl.h"
#include "smart_ctrl.h"
#include "lsc_adv.h"
#include "isp_bridge.h"
#include "pdaf_ctrl.h"
#include "ai_ctrl.h"
#include "tof_ctrl.h"
#include "isp_simulation.h"
#include <libloader.h>

#define LIBCAM_ALG_FILE "libispalg.so"
#define CMC10(n) (((n)>>13)?((n)-(1<<14)):(n))
#define MIN_FRAME_INTERVAL_MS  (10)

#define DBGINFO_QLEN (1 << 6)
#define DBGINFO_IDMASK (DBGINFO_QLEN - 1)

enum {
	FW_INIT_GOING = 0,
	FW_INIT_DONE,
	FW_INIT_ERR,
};

enum {
	AI_CMD_INIT,
	AI_CMD_START,
	AI_CMD_STOP,
	AI_CMD_WAIT,
	AI_CMD_MAX
};

enum {
	FDR_STATUS_NONE = 0,
	FDR_STATUS_CAPTURE,
	FDR_STATUS_PROC,
};

enum {
	PARAM_CFG_DEFAULT = 0,
	PARAM_CFG_START = 1,
	PARAM_CFG_FDRL,
	PARAM_CFG_FDRH,
};

cmr_u32 isp_cur_bv;
cmr_u32 isp_cur_ct;
cmr_u32 isp_abl_weight;
static int s_dbg_ver;

struct BITMAPFILEHEADER {
	unsigned short bfType;
	unsigned int bfSize;
	unsigned short bfReserved1;
	unsigned short bfReserved2;
	unsigned int bfOffBits;
} __attribute__ ((packed));

struct BITMAPINFOHEADER {
	unsigned int biSize;
	unsigned int biWidth;
	unsigned int biHeight;
	unsigned short biPlanes;
	unsigned short biBitCount;
	unsigned int biCompression;
	unsigned int biSizeImage;
	unsigned int biXPelsPerMeter;
	unsigned int biYPelsPerMeter;
	unsigned int biClrUsed;
	unsigned int biClrImportant;
};

struct cam_debug_info {
	cmr_s8 sensor_name[64];
	char file_tags[128];
	FILE *fp_dat;
	FILE *fp_info;
};

struct commn_info {
	cmr_s32 isp_pm_mode[PARAM_SET_MAX];
	cmr_u32 multi_nr_flag;
	cmr_u32 nr_scene_flag;
	cmr_u32 ai_scene_id;
	cmr_u32 image_pattern;
	cmr_u32 param_index;
	cmr_u32 isp_callback_bypass;
	proc_callback callback;
	cmr_handle caller_id;
	cmr_u8 *log_isp;
	cmr_u32 log_isp_size;
	cmr_u32 is_faceId_unlock;
	struct isp_size sensor_max_size;
	struct isp_size src;
	struct isp_size prv_size;
	struct isp_ops ops;
	struct sensor_raw_resolution_info input_size_trim[ISP_INPUT_SIZE_NUM_MAX];
};

struct sensor_pm_info {
	struct sensor_raw_info *sn_raw_info;
};

struct ae_info {
	cmr_handle handle;
	cmr_u32 sw_bypass;
	cmr_u8 *log_alc_ae;
	cmr_u32 log_alc_ae_size;
	cmr_u8 *log_alc;
	cmr_u32 log_alc_size;
	cmr_u8 *log_ae;
	cmr_u32 log_ae_size;
	cmr_uint vir_addr;
	cmr_int buf_size;
	cmr_int buf_num;
	cmr_uint phy_addr;
	cmr_uint mfd;
	cmr_int buf_property;
	void *buffer_client_data;
	struct ae_size win_num;
	struct ae_size win_size;
	struct ae_ctrl_ebd_info ebd_info;
	cmr_u32 shift;
	cmr_u32 flash_version;
};

struct awb_info {
	cmr_handle handle;
	cmr_u32 sw_bypass;
	cmr_u32 color_support;
	cmr_u32 alc_awb;
	cmr_s32 awb_pg_flag;
	cmr_u8 *log_alc_awb;
	cmr_u32 log_alc_awb_size;
	cmr_u8 *log_alc_lsc;
	cmr_u32 log_alc_lsc_size;
	cmr_u8 *log_awb;
	cmr_u32 log_awb_size;
	struct awb_ctrl_gain cur_gain;
};

struct smart_info {
	cmr_handle handle;
	cmr_u32 sw_bypass;
	cmr_u32 isp_smart_eb;
	cmr_u32 cur_set_id;
	cmr_u8 *log_smart;
	cmr_u32 log_smart_size;
	cmr_u8	lock_nlm_en;
	cmr_u8	lock_ee_en;
	cmr_u8	lock_precdn_en;
	cmr_u8	lock_cdn_en;
	cmr_u8	lock_postcdn_en;
	cmr_u8	lock_ccnr_en;
	cmr_u8	lock_ynr_en;
	cmr_s16 smart_block_eb[ISP_SMART_MAX_BLOCK_NUM];
	void *tunning_gamma_cur[PARAM_SET_MAX];
};

struct afl_info {
	cmr_handle handle;
	cmr_u32 version;
	cmr_u32 sw_bypass;
	cmr_u32 hw_bypass;
	cmr_uint vir_addr;
	cmr_int buf_size;
	cmr_int buf_num;
	cmr_uint phy_addr;
	cmr_u32 afl_mode;
	cmr_uint mfd;
	cmr_int buf_property;
	void *buffer_client_data;
	struct isp_statis_info afl_statis_info;
};

struct af_info {
	cmr_handle handle;
	cmr_u32 sw_bypass;
	cmr_u8 *log_af;
	cmr_u32 log_af_size;
	struct af_monitor_win_num win_num;
	cmr_u32 tof_support;
};

struct aft_info {
	cmr_handle handle;
	cmr_u8 *log_aft;
	cmr_u32 log_aft_size;
};

struct pdaf_info {
	cmr_handle handle;
	cmr_u32 sw_bypass;
	cmr_u8 pdaf_support;
	cmr_u8 pdaf_en;
};

struct ebd_info {
	cmr_handle handle;
	cmr_u32 sw_bypass;
	cmr_u8 ebd_support;
	cmr_u8 ebd_en;
};

struct lsc_info {
	cmr_handle handle;
	cmr_u32 sw_bypass;
	cmr_u32 use_lscm;
	struct lsc_size lscm_win_num;
	struct lsc_size lscm_win_size;
	void *lsc_info;
	void *lsc_tab_address;
	cmr_u32 lsc_tab_size;
	cmr_u32 isp_smart_lsc_lock;
	cmr_u8 *log_lsc;
	cmr_u32 log_lsc_size;
	cmr_u32 full_size_width;
	cmr_u32 full_size_height;
	cmr_u32 full_size_grid;
	cmr_u32 LSC_SPD_VERSION;
};

struct swalgo_pm_info {
	struct smart_block_result smart_postee;
	struct isp_blkpm_t postee;
};

struct fdr_info {
	cmr_u32 fdr_init;
	cmr_u32 fdr_start;
	cmr_u32 frm_idx;
	cmr_s32 tuning_param_size;
	void *tuning_param_ptr;

	struct img_rgb_info awb_gain;
	struct dcam_dev_rgb_gain_info rgb_gain;
	struct dcam_dev_blc_info blc_data;

	/* following is for debug */
	struct isp_fdr_dbgdata dbg_data;
	struct smart_proc_input smart_proc_in;
	cmr_u32 smart_in;

	cmr_u8 *log_fdr;
	cmr_u32 log_fdr_size;
};

struct ai_info {
	cmr_handle handle;
	cmr_u8 *log_ai;
	cmr_u32 log_ai_size;
	struct ai_ae_param ae_param;
};

struct ispalg_ae_ctrl_ops {
	cmr_s32 (*init)(struct ae_init_in *input_ptr, cmr_handle *handle_ae, cmr_handle result);
	cmr_int (*deinit)(cmr_handle *isp_afl_handle);
	cmr_int (*process)(cmr_handle handle_ae, struct ae_calc_in *in_ptr, struct ae_calc_out *result);
	cmr_int (*ioctrl)(cmr_handle handle, enum ae_io_ctrl_cmd cmd, cmr_handle in_ptr, cmr_handle out_ptr);
};

struct ispalg_ai_ctrl_ops {
	cmr_s32 (*init)(struct ai_init_in *input_ptr, cmr_handle *handle_ai, cmr_handle result);
	cmr_int (*deinit)(cmr_handle *handle_ai);
	cmr_int (*ioctrl)(cmr_handle handle_ai, enum ai_io_ctrl_cmd cmd, cmr_handle in_ptr, cmr_handle out_ptr);
};

struct ispalg_af_ctrl_ops {
	cmr_int (*init)(struct afctrl_init_in *input_ptr, cmr_handle *handle_af);
	cmr_int (*deinit)(cmr_handle handle);
	cmr_int (*process)(cmr_handle handle_af, void *in_ptr, struct afctrl_calc_out *result);
	cmr_int (*ioctrl)(cmr_handle handle_af, cmr_int cmd, void *in_ptr, void *out_ptr);
};

struct ispalg_afl_ctrl_ops {
	cmr_int (*init)(cmr_handle *isp_afl_handle, struct afl_ctrl_init_in *input_ptr);
	cmr_int (*deinit)(cmr_handle *isp_afl_handle);
	cmr_int (*process)(cmr_handle isp_afl_handle, struct afl_proc_in *in_ptr, struct afl_ctrl_proc_out *out_ptr);
	cmr_int (*config)(cmr_handle isp_afl_handle);
	cmr_int (*config_new)(cmr_handle isp_afl_handle);
	cmr_int (*ioctrl)(cmr_handle handle, enum afl_io_ctrl_cmd cmd, void *in_ptr, void *out_ptr);
};

struct ispalg_awb_ctrl_ops {
	cmr_int (*init)(struct awb_ctrl_init_param *input_ptr, cmr_handle *handle_awb);
	cmr_int (*deinit)(cmr_handle *handle_awb);
	cmr_int (*process)(cmr_handle handle_awb, struct awb_ctrl_calc_param *param, struct awb_ctrl_calc_result *result);
	cmr_int (*ioctrl)(cmr_handle handle_awb, enum awb_ctrl_cmd cmd, void *in_ptr, void *out_ptr);
};

struct ispalg_smart_ctrl_ops {
	smart_handle_t (*init)(struct smart_init_param *param, void *result);
	cmr_s32 (*deinit)(smart_handle_t *handle, void *param, void *result);
	cmr_s32 (*ioctrl)(smart_handle_t handle, cmr_u32 cmd, void *param, void *result);
	cmr_s32 (*calc)(cmr_handle handle_smart, struct smart_proc_input *in_ptr);
	cmr_s32 (*block_disable)(cmr_handle handle, cmr_u32 smart_id);
	cmr_s32 (*block_enable)(cmr_handle handle, cmr_u32 smart_id);
	cmr_s32 (*NR_disable)(cmr_handle handle, cmr_u32 is_diseb);
};

struct ispalg_pdaf_ctrl_ops {
	cmr_int (*init)(struct pdaf_ctrl_init_in *in, struct pdaf_ctrl_init_out *out, cmr_handle *handle);
	cmr_int (*deinit)(cmr_handle *handle);
	cmr_int (*process)(cmr_handle handle, struct pdaf_ctrl_process_in *in, struct pdaf_ctrl_process_out *out);
	cmr_int (*ioctrl)(cmr_handle handle, cmr_int cmd, struct pdaf_ctrl_param_in *in, struct pdaf_ctrl_param_out *out);
};

struct ispalg_lsc_ctrl_ops {
	cmr_int (*init)(struct lsc_adv_init_param *input_ptr, cmr_handle *handle_lsc);
	cmr_int (*deinit)(cmr_handle *handle_lsc);
	cmr_int (*process)(cmr_handle handle_lsc, struct lsc_adv_calc_param *in_ptr, struct lsc_adv_calc_result *result);
	cmr_int (*ioctrl)(cmr_handle handle_lsc, cmr_s32 cmd, void *in_ptr, void *out_ptr);
};

struct ispalg_tof_ctrl_ops {
	cmr_int (*init)(struct tof_ctrl_init_in *input_ptr, cmr_handle *handle);
	cmr_int (*deinit)(cmr_handle *handle);
};

struct ispalg_lib_ops {
	struct ispalg_ae_ctrl_ops ae_ops;
	struct ispalg_af_ctrl_ops af_ops;
	struct ispalg_afl_ctrl_ops afl_ops;
	struct ispalg_awb_ctrl_ops awb_ops;
	struct ispalg_smart_ctrl_ops smart_ops;
	struct ispalg_pdaf_ctrl_ops pdaf_ops;
	struct ispalg_lsc_ctrl_ops lsc_ops;
	struct ispalg_tof_ctrl_ops tof_ops;
	struct ispalg_ai_ctrl_ops ai_ops;
};

struct debuginfo_message{
	cmr_u8 *msg_log;
	cmr_u32 msg_size;
	cmr_u32 buf_size;
	cmr_s32 frame_id;
};

struct debuginfo_queue{
	struct debuginfo_message *head;
	cmr_u32 cur;
	cmr_u32 total_num;
};

struct isp_alg_fw_context {
	cmr_u32 frame_id_sof;
	cmr_int camera_id;
	void *init_cxt;
	cmr_u8 init_status;
	cmr_u8 ai_init_status;
	cmr_u8 alg_enable;
	cmr_u8 aem_is_update;
	cmr_u8 bayerhist_update;
	cmr_u8 lscm_is_update;
	cmr_u8 fw_started;
	cmr_u8 first_frm;
	cmr_u8 aethd_pri_set;
	nsecs_t last_sof_time;
	pthread_mutex_t pm_getting_lock;
	struct isp_awb_statistic_info aem_stats_data;
	struct isp_awb_statistic_info aem_ue;
	struct isp_awb_statistic_info aem_ae;
	struct isp_awb_statistic_info aem_oe;
	struct isp_awb_statistic_info cnt_ue;
	struct isp_awb_statistic_info cnt_oe;
	struct isp_lsc_statistic_info lscm_stats_data;
	struct isp_hist_statistic_info bayer_hist_stats[3];
	struct isp_hist_statistic_info hist2_stats;
	struct ae_size hist2_roi;
	struct afctrl_ae_info ae_info;
	struct afctrl_awb_info awb_info;
	struct commn_info commn_cxt;
	struct sensor_pm_info sn_cxt;
	struct ae_info ae_cxt;
	struct awb_info awb_cxt;
	struct smart_info smart_cxt;
	struct af_info af_cxt;
	struct aft_info aft_cxt;
	struct lsc_info lsc_cxt;
	struct afl_info afl_cxt;
	struct pdaf_info pdaf_cxt;
	struct ebd_info ebd_cxt;
	struct ai_info ai_cxt;
	struct fdr_info fdr_cxt;
	struct swalgo_pm_info swpm_ctx;
	struct sensor_libuse_info *lib_use_info;
	struct sensor_raw_ioctrl *ioctrl_ptr;
	struct sensor_pdaf_info *pdaf_info;
	struct isp_mem_info stats_mem_info;
	cmr_handle thr_handle;
	cmr_handle thr_aehandle;
	cmr_handle thr_afhandle;
	cmr_handle thr_aihandle;
	cmr_handle dev_access_handle;
	cmr_handle handle_pm;
	cmr_handle tof_handle;
	struct dcam_dev_rgb_gain_info rgb_gain;

	struct isp_sensor_fps_info sensor_fps;
	struct sensor_otp_cust_info *otp_data;
	cmr_u32 lsc_flash_onoff;
	cmr_u32 takepicture_mode;
	struct isptool_scene_param simu_param;
	cmr_handle ispalg_lib_handle;
	struct ispalg_lib_ops ops;
	cmr_u8  is_master;
	cmr_u32 is_multi_mode;
	cmr_u32 sensor_role;
	cmr_u32 work_mode;
	cmr_u32 zsl_flag;
	cmr_u32 sn_mode;
	cmr_u32 gtm_ltm_on;
	cmr_u32 long_expose_support;
	/* for 4x zoom focus */
	cmr_u32 last_ratio;
	cmr_u32 cur_ratio;
	cmr_s32 curr_bv;
	cmr_u32 abl_weight;
	/* new 4in1 solution2, 20191028 */
	cmr_u32 is_4in1_sensor; /* bind with sensor,not refer to sensor's output size */
	cmr_u32 remosaic_type; /* 1: software, 2: hardware, 0:other(sensor output bin size) */
	cmr_u32 ambient_highlight; /* 4in1: 1:highlight,0:lowlight; other sensor:0 */
	cmr_u32 is_high_res_mode; /* 1: high resolution mode */
	cmr_u32 fix_highlight; /* 0: not fix, 1: highlight, 2:lowlight */

	/* for debug */
	cmr_u32 is_simulator;
	cmr_u32 save_data;
	struct cam_debug_info dbg_cxt;
	cmr_u32 ai_scene_flag;
	cmr_u16 smooth_ratio;
	cmr_u32 is_ai_scene_pro;
	cmr_u16 app_mode;

	pthread_mutex_t debuginfo_queue_lock;
	struct debuginfo_queue ae_queue;
	struct debuginfo_queue alsc_queue;
	struct debuginfo_queue ai_queue;
	struct debuginfo_queue awb_queue;
	struct debuginfo_queue af_queue;
	struct debuginfo_queue aft_queue;
	struct debuginfo_queue smart_queue;
};

struct fw_init_local {
	cmr_u32 lib_init_status;
	cmr_u32 ae_init_status;
	cmr_u32 af_init_status;
	cmr_u32 afl_init_status;
	cmr_u32 awb_init_status;
	cmr_u32 lsc_init_status;
	cmr_u32 smart_init_status;
	cmr_u32 pdaf_init_status;
	cmr_u32 tof_init_status;

	pthread_t init_thread_handle;

	pthread_t lib_thread_handle;
	pthread_t ae_thread_handle;
	pthread_t af_thread_handle;
	pthread_t afl_thread_handle;
	pthread_t awb_thread_handle;
	pthread_t lsc_thread_handle;
	pthread_t smart_thread_handle;
	pthread_t pdaf_thread_handle;
	pthread_t tof_thread_handle;

	struct isp_alg_fw_context *cxt;
	struct isp_pm_init_input pm_init_input;
};

cmr_int ispalg_aithread_proc(struct cmr_msg *message, void *p_data);

#define FEATRUE_ISP_FW_IOCTRL
#include "isp_ioctrl.c"
#undef FEATRUE_ISP_FW_IOCTRL

static nsecs_t ispalg_get_sys_timestamp(void)
{
	nsecs_t timestamp = 0;

	timestamp = systemTime(CLOCK_MONOTONIC);
	timestamp = timestamp / 1000000;

	return timestamp;
}

static cmr_int dump_aem_pic(struct isp_alg_fw_context *cxt,
	struct isp_statis_info *statis_info, cmr_u32 pixel_num,
	cmr_u32 w, cmr_u32 h, cmr_u32 * r, cmr_u32 * g, cmr_u32 * b)
{
	cmr_u32 i, j, size, val, pitch;
	cmr_u32 shift = 2; // 10bits => 8bits
	cmr_u8 *buf, *ptr;
	char file_name[512];
	FILE *fp = NULL;
	nsecs_t cur_time = 0;
	struct BITMAPFILEHEADER bfheader;
	struct BITMAPINFOHEADER biheader;

	pitch = (w * 3 + 3) & ~3;
	size = pitch * h * 3;
	buf = (cmr_u8 *)malloc(size);
	if (buf == NULL) {
		ISP_LOGD("fail to malloc buf size %d\n", size);
		return -1;
	}

	memset(buf, 0, size);
	for (j = 0; j < h; j++) {
		ptr = buf + pitch * j;
		for (i = 0; i < w; i++) {
			val = *b++;
			val /= pixel_num;
			val >>= shift;
			*ptr++ = (cmr_u8)(val & 0xff);

			val = *g++;
			val /= pixel_num;
			val >>= shift;
			*ptr++ = (cmr_u8)(val & 0xff);

			val = *r++;
			val /= pixel_num;
			val >>= shift;
			*ptr++ = (cmr_u8)(val & 0xff);
		}
	}

	cur_time = ispalg_get_sys_timestamp();
	sprintf(file_name, "%scam%d_aem_t%08d_frm%05d.bmp", CAMERA_DUMP_PATH,
		(cmr_u32)cxt->camera_id, (cmr_u32)cur_time, statis_info->frame_id);

	fp = fopen(file_name, "wb");
	if (fp == NULL) {
		ISP_LOGE("fail to open aem pic file %s\n", file_name);
		free(buf);
		return -1;
	}

	memset(&bfheader, 0, sizeof(bfheader));
	bfheader.bfType = 'B' | ('M' << 8);
	bfheader.bfSize = sizeof(bfheader) + sizeof(biheader) + pitch * w;
	bfheader.bfOffBits = sizeof(bfheader) + sizeof(biheader);

	if (fwrite(&bfheader, 1, sizeof(bfheader), fp) != sizeof(bfheader)) {
		goto exit;
	}

	memset(&biheader, 0, sizeof(biheader));
	biheader.biSize = sizeof(biheader);
	biheader.biPlanes = 1;
	biheader.biWidth = w;
	biheader.biHeight = 0 - h;
	biheader.biBitCount = 24;
	biheader.biSizeImage = pitch * h;
	if (fwrite(&biheader, 1, sizeof(biheader), fp) != sizeof(biheader)) {
		goto exit;
	}

	size = pitch * h;
	if (fwrite(buf, 1, size, fp) != size) {
		goto exit;
	}

	ISP_LOGD("file %s done\n", file_name);
exit:
	fclose(fp);
	free(buf);

	return 0;
}

static cmr_s32 dump_lsc_data(struct isp_alg_fw_context *cxt, cmr_u32 start, cmr_u32 scene, void *data)
{
	FILE *fp = NULL;
	char file_name[256];
	struct dcam_dev_lsc_info *lsc;
	cmr_uint addr;
	cmr_u32 i;
	cmr_s16 *wtab;
	cmr_u16 *gain_tab;
	nsecs_t cur_time = 0;

	lsc = (struct dcam_dev_lsc_info *)data;
	cur_time = ispalg_get_sys_timestamp();
	if (start)
		sprintf(file_name, "%scam%d_lsc_t%08d_%s_start.txt", CAMERA_DUMP_PATH,
			(cmr_u32)cxt->camera_id, (cmr_u32)cur_time,
			(scene == PM_SCENE_PRE) ? "prev" : ((scene == PM_SCENE_CAP) ? "cap" : "fdr"));
	else
		sprintf(file_name, "%scam%d_lsc_t%08d_%s.txt", CAMERA_DUMP_PATH,
			(cmr_u32)cxt->camera_id, (cmr_u32)cur_time,
			(scene == PM_SCENE_PRE) ? "prev" : ((scene == PM_SCENE_CAP) ? "cap" : "fdr"));

	fp = fopen(file_name, "w+");
	if (fp == NULL) {
		ISP_LOGE("fail to open lsc data file %s\n", file_name);
		return -1;
	}

	fprintf(fp, "====== lsc base info ==========\n");
	fprintf(fp, "bypass = %d\n", lsc->bypass);
	fprintf(fp, "update_all = %d\n", lsc->update_all);
	fprintf(fp, "grid = %d\n", lsc->grid_width);
	fprintf(fp, "grid_x_num = %d\n", lsc->grid_x_num);
	fprintf(fp, "grid_y_num = %d\n", lsc->grid_y_num);
	fprintf(fp, "grid_num_t = %d\n", lsc->grid_num_t);
	fprintf(fp, "grid tab size = %d\n", lsc->gridtab_len);
	fprintf(fp, "weight tab size = %d\n", lsc->weight_num);

	addr = (cmr_uint)lsc->weight_tab_addr;
	wtab = (cmr_s16 *)addr;

	fprintf(fp, "\n====== weight tab ========\n");
	for (i = 0; i < (lsc->weight_num / sizeof(cmr_s16)); i += 3)
		fprintf(fp, "idx %04d: %d, %d, %d\n", i, wtab[i],  wtab[i + 1], wtab[i + 2]);

	addr = (cmr_uint)lsc->grid_tab_addr;
	gain_tab = (cmr_u16 *)addr;

	fprintf(fp, "\n====== gain tab ========\n");
	for (i = 0; i < (lsc->gridtab_len / sizeof(cmr_u16)); i += 4)
		fprintf(fp, "idx %04d:  0x%04x 0x%04x 0x%04x 0x%04x\n", i,
			gain_tab[i], gain_tab[i + 1], gain_tab[i + 2], gain_tab[i + 3]);

	fclose(fp);
	ISP_LOGD("write lsc data file %s done\n", file_name);

	return 0;
}

static cmr_int ispalg_get_rgb_gain(cmr_handle isp_fw_handle, cmr_u32 *param)
{
	cmr_s32 ret = ISP_SUCCESS;
	struct isp_pm_param_data param_data;
	struct isp_pm_ioctl_input input = { NULL, 0 };
	struct isp_pm_ioctl_output output = { NULL, 0 };
	struct dcam_dev_rgb_gain_info *gain_info;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_fw_handle;

	memset(&param_data, 0, sizeof(param_data));

	BLOCK_PARAM_CFG(input, param_data,
			ISP_PM_BLK_ISP_SETTING,
			ISP_BLK_RGB_GAIN, NULL, 0);
	pthread_mutex_lock(&cxt->pm_getting_lock);
	ret = isp_pm_ioctl(cxt->handle_pm,
			ISP_PM_CMD_GET_SINGLE_SETTING,
			&input, &output);
	if (ISP_SUCCESS == ret && 1 == output.param_num) {
		gain_info = (struct dcam_dev_rgb_gain_info *)output.param_data->data_ptr;
		*param = gain_info->global_gain;

		cxt->rgb_gain.bypass = gain_info->bypass;
		cxt->rgb_gain.global_gain = gain_info->global_gain;
		cxt->rgb_gain.r_gain = gain_info->r_gain;
		cxt->rgb_gain.g_gain = gain_info->g_gain;
		cxt->rgb_gain.b_gain = gain_info->b_gain;
		ISP_LOGV("rgbgain_bypass = %d, global_gain = %d, r_gain = %d, g_gain = %d, b_gain = %d",
			cxt->rgb_gain.bypass, cxt->rgb_gain.global_gain,
			cxt->rgb_gain.r_gain, cxt->rgb_gain.g_gain, cxt->rgb_gain.b_gain);
	} else {
		*param = 4096;
	}
	pthread_mutex_unlock(&cxt->pm_getting_lock);

	ISP_LOGV("D-gain global gain ori: %d\n", *param);

	return ret;
}

static cmr_int ispalg_set_rgb_gain(cmr_handle isp_fw_handle, void *param)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_fw_handle;
	struct isp_rgb_gain_info *inptr;
	struct dcam_dev_rgb_gain_info gain_info;
	struct isp_u_blocks_info block_info;
	struct isptool_scene_param scene_param;

	if (!param) {
		ISP_LOGE("fail to get gain info.\n");
		return ISP_ERROR;
	}
	inptr = (struct isp_rgb_gain_info *)param;
	gain_info.bypass = inptr->bypass;
	gain_info.global_gain = inptr->global_gain;
	gain_info.r_gain = inptr->r_gain;
	gain_info.g_gain = inptr->g_gain;
	gain_info.b_gain = inptr->b_gain;
	block_info.block_info = &gain_info;

	cxt->rgb_gain.bypass=inptr->bypass;
	cxt->rgb_gain.global_gain=inptr->global_gain;
	cxt->rgb_gain.r_gain=inptr->r_gain;
	cxt->rgb_gain.g_gain=inptr->g_gain;
	cxt->rgb_gain.b_gain=inptr->b_gain;
	block_info.scene_id = PM_SCENE_PRE;

	if (isp_video_get_simulation_flag()) {
		ret = isp_sim_get_scene_parm(&scene_param);
		if (ISP_SUCCESS == ret) {
			gain_info.global_gain = scene_param.global_gain;
			cxt->rgb_gain.global_gain = gain_info.global_gain;
			ISP_LOGI("hwsim: global_gain :input[%d] scene_param[%d]\n",
                             inptr->global_gain , scene_param.global_gain);
		}
	}
	ISP_LOGV("global_gain : %d, r %d g %d b %d\n", gain_info.global_gain,
		gain_info.r_gain, gain_info.g_gain, gain_info.b_gain);
	ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_RGB_GAIN, &block_info, NULL);
	if (cxt->remosaic_type) {
		block_info.scene_id = PM_SCENE_CAP;

		ISP_LOGV("global_gain : %d, r %d g %d b %d\n", gain_info.global_gain,
			gain_info.r_gain, gain_info.g_gain, gain_info.b_gain);
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_RGB_GAIN, &block_info, NULL);
		if (ret) {
			ISP_LOGW("fail to set rgb gain for 4in1 capture");
		}
	}

	return ret;
}

static cmr_int ispalg_get_aem_param(cmr_handle isp_fw_handle, struct isp_rgb_aem_info *param)
{
	cmr_s32 ret = ISP_SUCCESS;
	struct isp_pm_ioctl_input input = { NULL, 0 };
	struct isp_pm_ioctl_output output = { NULL, 0 };
	struct isp_pm_param_data param_data;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_fw_handle;
	struct isp_size *aem_win;

	memset(&param_data, 0, sizeof(param_data));

	pthread_mutex_lock(&cxt->pm_getting_lock);

	/* supported AE win num: 32x32, 64x64, 128x128 */
	BLOCK_PARAM_CFG(input, param_data, ISP_PM_BLK_AEM_WIN, ISP_BLK_RGB_AEM, NULL, 0);
	ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_SINGLE_SETTING, &input, &output);
	if (ISP_SUCCESS == ret && 1 == output.param_num) {
		aem_win = (struct isp_size *)output.param_data->data_ptr;
		param->blk_num.w = (aem_win->w < 32) ? 32 : aem_win->w;
		param->blk_num.h = (aem_win->h < 32) ? 32 : aem_win->h;
		ISP_LOGD("get blk_num %d %d, set to %d %d\n",
			aem_win->w, aem_win->h, param->blk_num.w, param->blk_num.h);
	} else {
		param->blk_num.w = 32;
		param->blk_num.h = 32;
		ISP_LOGV("set default blk_num 32x32\n");
	}
	pthread_mutex_unlock(&cxt->pm_getting_lock);

	return ret;
}

static cmr_int isp_prepare_atm_param(cmr_handle isp_alg_handle,
	struct smart_proc_input *smart_proc_in)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;

	smart_proc_in->r_info = cxt->aem_stats_data.r_info;
	smart_proc_in->g_info = cxt->aem_stats_data.g_info;
	smart_proc_in->b_info = cxt->aem_stats_data.b_info;
	smart_proc_in->win_num_w = cxt->ae_cxt.win_num.w;
	smart_proc_in->win_num_h = cxt->ae_cxt.win_num.h;
	smart_proc_in->aem_shift = cxt->ae_cxt.shift;
	smart_proc_in->win_size_w = cxt->ae_cxt.win_size.w;
	smart_proc_in->win_size_h = cxt->ae_cxt.win_size.h;
	smart_proc_in->bhist_update = cxt->bayerhist_update;
	smart_proc_in->bhist_stats[0].bin = cxt->bayer_hist_stats[0].bin;
	smart_proc_in->bhist_stats[1].bin = cxt->bayer_hist_stats[1].bin;
	smart_proc_in->bhist_stats[2].bin = cxt->bayer_hist_stats[2].bin;
	smart_proc_in->bhist_stats[0].sec = cxt->bayer_hist_stats[0].sec;
	smart_proc_in->bhist_stats[0].usec = cxt->bayer_hist_stats[0].usec;
	smart_proc_in->bhist_stats[0].frame_id = cxt->bayer_hist_stats[0].frame_id;
	memcpy(&smart_proc_in->bhist_stats[0].value,&cxt->bayer_hist_stats[0].value,256 * sizeof(cmr_u32));
	memcpy(&smart_proc_in->bhist_stats[1].value,&cxt->bayer_hist_stats[1].value,256 * sizeof(cmr_u32));
	memcpy(&smart_proc_in->bhist_stats[2].value,&cxt->bayer_hist_stats[2].value,256 * sizeof(cmr_u32));
	if (smart_proc_in->r_info == NULL)
		ISP_LOGE("fail to access null r/g/b ptr %p/%p/%p\n",
			smart_proc_in->r_info,
			smart_proc_in->g_info,
			smart_proc_in->b_info);

	return ret;
}


static cmr_int ispalg_ae_callback(cmr_handle isp_alg_handle, cmr_int cb_type, void *data)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	enum isp_callback_cmd cmd = 0;
	struct ae_size *hdr_statis_size;

	switch (cb_type) {
	case AE_CB_FLASHING_CONVERGED:
	case AE_CB_CONVERGED:
	case AE_CB_RECOVER_GAIN:
	case AE_CB_PREFLASH_PERIOD_END:
	case AE_CB_CLOSE_MAIN_FLASH:
		cmd = ISP_AE_STAB_CALLBACK;
		break;
	case AE_CB_QUICKMODE_DOWN:
		cmd = ISP_QUICK_MODE_DOWN;
		break;
	case AE_CB_TOUCH_AE_NOTIFY:
	case AE_CB_STAB_NOTIFY:
		cmd = ISP_AE_STAB_NOTIFY;
		break;
	case AE_CB_AE_LOCK_NOTIFY:
		cmd = ISP_AE_LOCK_NOTIFY;
		break;
	case AE_CB_AE_UNLOCK_NOTIFY:
		cmd = ISP_AE_UNLOCK_NOTIFY;
		break;
	case AE_CB_HDR_START:
		cmd = ISP_HDR_EV_EFFECT_CALLBACK;
		break;
	case AE_CB_LED_NOTIFY:
		cmd = ISP_ONLINE_FLASH_CALLBACK;
		break;
	case AE_CB_FLASH_FIRED:
		cmd = ISP_AE_CB_FLASH_FIRED;
		break;
	case AE_CB_PROCESS_RESULT:
		cmd = ISP_AE_PARAM_CALLBACK;
		break;
	case AE_CB_HDR_STATUS:
		cmd = ISP_AUTO_HDR_STATUS_CALLBACK;
		break;
	case AE_CB_HDR_STATIS_SIZE:
		hdr_statis_size = (struct ae_size *)data;
		hdr_statis_size->w = cxt->hist2_roi.w;
		hdr_statis_size->h = cxt->hist2_roi.h;
		return ret;
	case AE_CB_FDR_START:
		cmd = ISP_FDR_EV_EFFECT_CALLBACK;
		break;
	case AE_CB_FDR_STATUS:
		cmd = ISP_AUTO_FDR_STATUS_CALLBACK;
		break;
	case AE_CB_3DNR_NOTIFY:
		cmd = ISP_3DNR_CALLBACK;
		break;
	case AE_CB_EXPTIME_NOTIFY:
		cmd = ISP_AE_EXP_TIME;
		break;
	case AE_CB_EV_ADJUST_NOTIFY:
		cmd = ISP_EV_EFFECT_CALLBACK;
		break;
	case AE_CB_SYNC_STABLE:
		cmd = ISP_AE_SYNC_STATUS_CALLBACK;
		break;
	case AE_CB_LONG_EXP_START:
		cmd = ISP_LONGEXP_SKIPNUM_CALLBACK;
		break;
	default:
		cmd = ISP_AE_STAB_CALLBACK;
		break;
	}

	if (cxt->commn_cxt.callback) {
		cxt->commn_cxt.callback(cxt->commn_cxt.caller_id, ISP_CALLBACK_EVT | cmd, data, 0);
	}

	return ret;
}

static cmr_int ispalg_ai_callback(cmr_handle isp_alg_handle, cmr_int cb_type, void *data)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	enum isp_callback_cmd cmd = 0;

	if (!cxt) {
		ret = -ISP_PARAM_NULL;
		goto exit;
	}

	switch (cb_type) {
	case AI_CB_FOR_HAL: {
		cxt->commn_cxt.ai_scene_id = *(cmr_u8 *)data;
		ISP_LOGV("ai_scene_id %d", cxt->commn_cxt.ai_scene_id);
		cmd = ISP_AI_SCENE_TYPE_CALLBACK;
		if (cxt->commn_cxt.callback) {
			cxt->commn_cxt.callback(cxt->commn_cxt.caller_id, ISP_CALLBACK_EVT | cmd, data, 0);
		}
		break;
	}
	case AI_CB_FOR_AE: {
		struct ai_scene_detect_info scene_info = *(struct ai_scene_detect_info *) data;
		if (cxt->is_ai_scene_pro || scene_info.cur_scene_id == AI_SCENE_NIGHT)
			scene_info.cur_scene_id = AI_SCENE_DEFAULT;
		if (cxt->ops.ae_ops.ioctrl)
			ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_SCENE_INFO, &scene_info, NULL);
		break;
	}
	case AI_CB_FOR_AWB: {
		if (cxt->ops.awb_ops.ioctrl)
			ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_SET_SCENE_INFO, data, NULL);
		break;
	}
	case AI_CB_FOR_AF: {
		if (cxt->ops.af_ops.ioctrl)
			ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle, AF_CMD_SET_SCENE_INFO, data, NULL);
		break;
	}
	default:
		break;
	}

exit:
	return ret;
}

static cmr_int ispalg_set_lsc_monitor(cmr_handle isp_alg_handle, struct lsc_monitor_info *lscm_info)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct dcam_dev_lscm_param lscm_monitor;

	ISP_LOGD("win %d %d %d %d %d %d\n",
			lscm_info->trim.x, lscm_info->trim.y,
			lscm_info->win_size.w, lscm_info->win_size.h,
			lscm_info->win_num.w, lscm_info->win_num.h);

	cxt->lsc_cxt.lscm_win_num.w = lscm_info->win_num.w;
	cxt->lsc_cxt.lscm_win_num.h = lscm_info->win_num.h;
	cxt->lsc_cxt.lscm_win_size.w = lscm_info->win_size.w;
	cxt->lsc_cxt.lscm_win_size.h = lscm_info->win_size.h;
	lscm_monitor.mode = lscm_info->work_mode;
	lscm_monitor.skip_num = lscm_info->skip_num;
	lscm_monitor.offset_x = lscm_info->trim.x;
	lscm_monitor.offset_y = lscm_info->trim.y;
	lscm_monitor.blk_width = lscm_info->win_size.w;
	lscm_monitor.blk_height = lscm_info->win_size.h;
	lscm_monitor.blk_num_x = lscm_info->win_num.w;
	lscm_monitor.blk_num_y = lscm_info->win_num.h;

	ret = isp_dev_access_ioctl(cxt->dev_access_handle,
			ISP_DEV_SET_LSC_MONITOR,
			&lscm_monitor, NULL);
	return ret;
}

static cmr_int ispalg_set_ae_stats_mode(
	cmr_handle isp_alg_handle, cmr_u32 mode,
	cmr_u32 skip_number)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_u32 ae_mode;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;

	if (mode == AE_STATISTICS_MODE_SINGLE)
		ae_mode = AEM_MODE_SINGLE;
	else
		ae_mode = AEM_MODE_MULTI;

	ret = isp_dev_access_ioctl(cxt->dev_access_handle,
					   ISP_DEV_SET_AE_STATISTICS_MODE,
					   &ae_mode,
					   &skip_number);

	return ret;
}

static cmr_int ispalg_set_aem_win(cmr_handle isp_alg_handle, struct ae_monitor_info *aem_info)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct dcam_dev_aem_win aem_win;

	ISP_LOGD("win %d %d %d %d %d %d\n",
			aem_info->trim.x, aem_info->trim.y,
			aem_info->win_size.w, aem_info->win_size.h,
			aem_info->win_num.w, aem_info->win_num.h);

	cxt->ae_cxt.win_num.w = aem_info->win_num.w;
	cxt->ae_cxt.win_num.h = aem_info->win_num.h;
	cxt->ae_cxt.win_size.w = aem_info->win_size.w;
	cxt->ae_cxt.win_size.h = aem_info->win_size.h;
	aem_win.offset_x = aem_info->trim.x;
	aem_win.offset_y = aem_info->trim.y;
	aem_win.blk_width = aem_info->win_size.w;
	aem_win.blk_height = aem_info->win_size.h;
	aem_win.blk_num_x = aem_info->win_num.w;
	aem_win.blk_num_y = aem_info->win_num.h;

	cxt->ai_cxt.ae_param.blk_num_hor = (cmr_u16)aem_win.blk_num_x;
	cxt->ai_cxt.ae_param.blk_num_ver = (cmr_u16)aem_win.blk_num_y;
	cxt->ai_cxt.ae_param.blk_width = (cmr_u16)aem_win.blk_width;
	cxt->ai_cxt.ae_param.blk_height = (cmr_u16)aem_win.blk_height;
	cxt->ai_cxt.ae_param.ae_offset.x = aem_win.offset_x;
	cxt->ai_cxt.ae_param.ae_offset.y = aem_win.offset_y;

	ret = isp_dev_access_ioctl(cxt->dev_access_handle,
			ISP_DEV_SET_AE_MONITOR_WIN,
			&aem_win, NULL);

	return ret;
}


static cmr_int ispalg_set_aem_thrd(cmr_handle isp_alg_handle, struct ae_monitor_info *in)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct dcam_dev_aem_thr aem_thrd;
	ISP_LOGD("aem_threshold, rgb high(%d %d %d)  rgb low(%d %d %d)\n",
	in->high_region_thrd.r,in->high_region_thrd.g,in->high_region_thrd.b,
	in->low_region_thrd.r,in->low_region_thrd.g,in->low_region_thrd.b);
	memset(&aem_thrd, 0, sizeof(struct dcam_dev_aem_thr));
	aem_thrd.aem_r_thr.low_thr = in->low_region_thrd.r;//in->r_low;
	aem_thrd.aem_r_thr.high_thr = in->high_region_thrd.r;//in->r_high;
	aem_thrd.aem_g_thr.low_thr = in->low_region_thrd.g;//in->g_low;
	aem_thrd.aem_g_thr.high_thr = in->high_region_thrd.g;
	aem_thrd.aem_b_thr.low_thr = in->low_region_thrd.b;
	aem_thrd.aem_b_thr.high_thr = in->high_region_thrd.b;
	ret = isp_dev_access_ioctl(cxt->dev_access_handle,
	ISP_DEV_SET_AE_RGB_THR, &aem_thrd, NULL);

	return ret;
}

static cmr_int ispalg_set_bayerhist(cmr_handle isp_alg_handle, struct ae_bayer_hist_cfg *cfg)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct dcam_dev_hist_info bayerHist_info;

	if (cfg == NULL) {
		ISP_LOGE("fail to get ae_bayer_hist_cfg ptr\n");
		return ISP_ERROR;
	}

	ISP_LOGD("cam%ld, bypass %d, mode %d, win %d %d %d %d\n", cxt->camera_id,
		cfg->bypass, cfg->mode,
		cfg->hist_rect.start_x, cfg->hist_rect.start_y,
		cfg->hist_rect.end_x, cfg->hist_rect.end_y);

	memset(&bayerHist_info, 0, sizeof(bayerHist_info));
	bayerHist_info.hist_bypass = cfg->bypass;
	bayerHist_info.bayer_hist_stx = cfg->hist_rect.start_x;
	bayerHist_info.bayer_hist_sty = cfg->hist_rect.start_y;
	bayerHist_info.bayer_hist_endx = cfg->hist_rect.end_x;
	bayerHist_info.bayer_hist_endy = cfg->hist_rect.end_y;
	bayerHist_info.hist_mode_sel = 1;
	bayerHist_info.hist_mul_enable = 1;
	bayerHist_info.hist_initial_clear = 1;
	bayerHist_info.hist_skip_num_clr = 1;

	ret = isp_dev_access_ioctl(cxt->dev_access_handle,
			ISP_DEV_SET_BAYERHIST_CFG, &bayerHist_info, NULL);

	return ret;
}



static cmr_int ispalg_lsc_set_cb(cmr_handle isp_alg_handle,
		cmr_int type, void *param0, void *param1)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	UNUSED(param1);

	if (!cxt) {
		ISP_LOGE("fail to get valid cxt ptr\n");
		return ISP_PARAM_NULL;
	}

	switch (type) {
	case ISP_LSC_SET_MONITOR:
		ret = ispalg_set_lsc_monitor(cxt, param0);
		break;
	case ISP_LSC_SET_MONITOR_BYPASS:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_LSC_MONITOR_BYPASS, param0, NULL);
		break;
	default:
		ISP_LOGV("unsupported ae cb: %lx\n", type);
		break;
	}

	return ret;
}

static cmr_int ispalg_awb_set_cb(cmr_handle isp_alg_handle,
		cmr_int type, void *param0, void *param1)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	UNUSED(param1);

	if (!cxt) {
		ISP_LOGE("fail to get valid cxt ptr\n");
		return ISP_PARAM_NULL;
	}

	switch (type) {
	case ISP_AWB_SET_WBC_GAIN:
		{
		struct img_rgb_info gain;
		struct isp_u_blocks_info cfg;
		struct awb_ctrl_gain *awb_gain = (struct awb_ctrl_gain*)param0;
		gain.r = awb_gain->r;
		gain.gr = awb_gain->g;
		gain.gb = awb_gain->g;
		gain.b = awb_gain->b;
		/* todo: also set capture gain if 4in1 */
		if (cxt->remosaic_type) {
			cfg.scene_id = PM_SCENE_CAP;
			cfg.block_info = &gain;
			ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AWB_GAIN, (void*)&cfg, param1);
			if(ret != ISP_SUCCESS){
				ISP_LOGE("fail to set isp device access ioctl.");
				return ret;
			}
		}
		cfg.scene_id = PM_SCENE_PRE;
		cfg.block_info = &gain;
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AWB_GAIN, (void*)&cfg, param1);
		break;
		}
	default:
		ISP_LOGV("unsupported awb cb: %lx\n", type);
		break;
	}

	return ret;
}


static cmr_int ispalg_ae_set_cb(cmr_handle isp_alg_handle,
		cmr_int type, void *param0, void *param1)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct sensor_multi_ae_info *ae_info = NULL;
	struct isp_alg_fw_context *context = NULL;
	cmr_handle handle = NULL;

	if (!cxt) {
		ISP_LOGE("fail to get valid cxt ptr\n");
		return ISP_PARAM_NULL;
	}

	switch (type) {
	case ISP_AE_MULTI_WRITE:
		{
			struct sensor_multi_ae_info info[CAM_SENSOR_MAX];
			cmr_u32 index = 0, count = 0;

#define index2role(index) (index)
			ae_info = (struct sensor_multi_ae_info *)param0;
			while (index < ae_info->count) {
				/*
				 * ae_info ordered by master, slave0, slave1, ...
				 * this is same as role definition
				 */
				isp_br_ioctrl(index2role(index), GET_ISPALG_FW, NULL, &handle);
				context = (struct isp_alg_fw_context *)handle;
				if (!context) {
					ISP_LOGE("slave context not ready");
					return ISP_ERROR;
				}

				ae_info[index].handle = context->ioctrl_ptr->caller_handler;
				ae_info[index].camera_id = context->camera_id;
				ae_info[index].exp.size_index = context->sn_mode;

				index++;
			}
			index = 0;
			while (index < ae_info->count) {
				if (!ae_info[index].ignore)
					info[count++] = ae_info[index];
				index++;
			}
			info->count = count;

			if (cxt->ioctrl_ptr->sns_ioctl)
				ret = cxt->ioctrl_ptr->sns_ioctl(cxt->ioctrl_ptr->caller_handler,
						CMD_SNS_IC_WRITE_MULTI_AE, info);
#undef index2role
		}
		break;
	case ISP_AE_HDR_BOKEH:
		ISP_LOGV("ISP_AE_HDR_BOKEH");

		isp_br_ioctrl(CAM_SENSOR_SLAVE0, GET_ISPALG_FW, NULL, &handle);
		context = (struct isp_alg_fw_context *)handle;
		if (context != NULL) {
			ret = ispalg_ae_callback(context, AE_CB_HDR_START, (void *)param0);
		} else {
			ISP_LOGE("fail to get slave sensor handle , it is not ready");
			return ret;
		}
		break;
	case ISP_AE_SET_RGB_GAIN_SLAVE0:
		ISP_LOGV("ISP_AE_SET_RGB_GAIN_SLAVE0");

		isp_br_ioctrl(CAM_SENSOR_SLAVE0, GET_ISPALG_FW, NULL, &handle);
		context = (struct isp_alg_fw_context *)handle;
		if (context != NULL) {
			ret = ispalg_set_rgb_gain(context, param0);
		} else {
			ISP_LOGE("fail to get slave sensor handle , it is not ready");
			return ret;
		}
		break;
	case ISP_AE_SET_RGB_GAIN_SLAVE1:
		ISP_LOGV("ISP_AE_SET_RGB_GAIN_SLAVE1");

		isp_br_ioctrl(CAM_SENSOR_SLAVE1, GET_ISPALG_FW, NULL, &handle);
		context = (struct isp_alg_fw_context *)handle;
		if (context != NULL) {
			ret = ispalg_set_rgb_gain(context, param0);
		} else {
			ISP_LOGE("fail to get slave sensor handle , it is not ready");
			return ret;
		}
		break;
	case ISP_AE_SET_GAIN:
		ret = cxt->ioctrl_ptr->set_gain(cxt->ioctrl_ptr->caller_handler, *(cmr_u32 *) param0);
		break;
	case ISP_AE_SET_EXPOSURE:
		ret = cxt->ioctrl_ptr->set_exposure(cxt->ioctrl_ptr->caller_handler, *(cmr_u32 *) param0);
		break;
	case ISP_AE_EX_SET_EXPOSURE:
		ret = cxt->ioctrl_ptr->ex_set_exposure(cxt->ioctrl_ptr->caller_handler, (cmr_uint) param0);
		break;
	case ISP_AE_SET_MONITOR:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AE_SKIP_NUM, param0, NULL);
		break;
	case ISP_AE_SET_MONITOR_WIN:
		ret = ispalg_set_aem_win(cxt, param0);
		break;
	case ISP_AE_SET_RGB_THRD:
		ret = ispalg_set_aem_thrd(cxt, param0);
		break;
	case ISP_AE_SET_MONITOR_BYPASS:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AE_MONITOR_BYPASS, param0, NULL);
		break;
	case ISP_AE_SET_STATISTICS_MODE:
		ret = ispalg_set_ae_stats_mode(cxt, *(cmr_u32 *)param0, *(cmr_u32 *)param1);
		break;
	case ISP_AE_SET_AE_CALLBACK:
		ret = ispalg_ae_callback(cxt, *(cmr_int *) param0, param1);
		break;
	case ISP_AE_GET_SYSTEM_TIME:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_GET_SYSTEM_TIME, param0, param1);
		break;
	case ISP_AE_SET_RGB_GAIN:
		ret = ispalg_set_rgb_gain(cxt, param0);
		break;
	case ISP_AE_GET_FLASH_CHARGE:
		ret = cxt->commn_cxt.ops.flash_get_charge(cxt->commn_cxt.caller_id, param0, param1);
		break;
	case ISP_AE_GET_FLASH_TIME:
		ret = cxt->commn_cxt.ops.flash_get_time(cxt->commn_cxt.caller_id, param0, param1);
		break;
	case ISP_AE_SET_FLASH_CHARGE:
		ret = cxt->commn_cxt.ops.flash_set_charge(cxt->commn_cxt.caller_id, param0, param1);
		break;
	case ISP_AE_SET_FLASH_TIME:
		ret = cxt->commn_cxt.ops.flash_set_time(cxt->commn_cxt.caller_id, param0, param1);
		break;
	case ISP_AE_FLASH_CTRL:
		ret = cxt->commn_cxt.ops.flash_ctrl(cxt->commn_cxt.caller_id, param0, param1);
		break;
	case ISP_AE_GET_RGB_GAIN:
		ret = ispalg_get_rgb_gain(cxt, param0);
		break;
	case ISP_AE_SET_BAYER_HIST:
		ret = ispalg_set_bayerhist(cxt, param0);
		break;
	case ISP_AE_SET_WBC_GAIN:
	{
		struct img_rgb_info gain;
		struct isp_u_blocks_info cfg;
		struct ae_awb_gain *awb_gain = (struct ae_awb_gain*)param0;
		gain.r = awb_gain->r;
		gain.gr = awb_gain->g;
		gain.gb = awb_gain->g;
		gain.b = awb_gain->b;
		/* todo: also set capture gain if 4in1 */
		if (cxt->remosaic_type) {
			cfg.scene_id = PM_SCENE_CAP;
			cfg.block_info = &gain;
			ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AWB_GAIN, &cfg, NULL);
			if(ret != ISP_SUCCESS){
				ISP_LOGE("fail to set isp device access ioctl.");
				return ret;
			}
		}
		cfg.scene_id = PM_SCENE_PRE;
		cfg.block_info = &gain;
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AWB_GAIN, &cfg, NULL);
		break;
	}
	default:
		ISP_LOGV("unsupported ae cb: %lx\n", type);
		break;
	}

	return ret;
}

static cmr_int ispalg_af_mode_convert(struct isp_af_notice *isp_af)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_u32 af_mode = 0;

	af_mode = isp_af->af_mode;
	switch (af_mode) {
		case AF_MODE_NORMAL:
			isp_af->af_mode = ISP_FOCUS_TRIG;
			break;
		case AF_MODE_MACRO:
			isp_af->af_mode = ISP_FOCUS_MACRO;
			break;
		case AF_MODE_CONTINUE:
			isp_af->af_mode = ISP_FOCUS_CONTINUE;
			break;
		case AF_MODE_MANUAL:
			isp_af->af_mode = ISP_FOCUS_MANUAL;
			break;
		case AF_MODE_VIDEO:
			isp_af->af_mode = ISP_FOCUS_VIDEO;
			break;
		case AF_MODE_PICTURE:
			isp_af->af_mode = ISP_FOCUS_PICTURE;
			break;
		case AF_MODE_FULLSCAN:
			isp_af->af_mode = ISP_FOCUS_FULLSCAN;
			break;
		default:
			isp_af->af_mode = ISP_FOCUS_TRIG;
			break;
	}

	return ret;
}

static cmr_int ispalg_af_set_cb(cmr_handle isp_alg_handle, cmr_int type, void *param0, void *param1)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_u16 val = 0;
	cmr_u32 pos = 0;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	cmr_u32 af_get_aelock = 0;

	if (!cxt) {
		ISP_LOGE("fail to get valid cxt ptr\n");
		return ISP_PARAM_NULL;
	}

	switch (type) {
	case ISP_AF_SET_POS:
		if (cxt->ioctrl_ptr->set_focus) {
			ret = cxt->ioctrl_ptr->set_focus(cxt->ioctrl_ptr->caller_handler, *(cmr_u32 *) param0);
		}
		break;
	case AF_CB_CMD_SET_END_NOTICE:
		if (ISP_ZERO == cxt->commn_cxt.isp_callback_bypass) {
			struct isp_af_notice *isp_af = (struct isp_af_notice *)param0;

			ret = ispalg_af_mode_convert(isp_af);

			ret = cxt->commn_cxt.callback(cxt->commn_cxt.caller_id,
				ISP_CALLBACK_EVT | ISP_AF_NOTICE_CALLBACK,
				isp_af, sizeof(struct isp_af_notice));
		}
		break;
	case AF_CB_CMD_SET_MOTOR_POS:
		val = *(cmr_u16 *)param0;
		pos = (cmr_u32)val;
		if (cxt->ioctrl_ptr->set_pos) {
			ret = cxt->ioctrl_ptr->set_pos(cxt->ioctrl_ptr->caller_handler, pos);
		}
		break;
	case AF_CB_CMD_GET_LENS_OTP:
		if (cxt->ioctrl_ptr->get_otp) {
			ret = cxt->ioctrl_ptr->get_otp(cxt->ioctrl_ptr->caller_handler, param0, param1);
		}
		break;
	case AF_CB_CMD_SET_MOTOR_BESTMODE:
		if (cxt->ioctrl_ptr->set_motor_bestmode) {
			ret = cxt->ioctrl_ptr->set_motor_bestmode(cxt->ioctrl_ptr->caller_handler);
		}
		break;
	case AF_CB_CMD_GET_MOTOR_POS:
		if (cxt->ioctrl_ptr->get_motor_pos) {
			ret = cxt->ioctrl_ptr->get_motor_pos(cxt->ioctrl_ptr->caller_handler, param0);
		}
		break;
	case AF_CB_CMD_SET_VCM_TEST_MODE:
		if (cxt->ioctrl_ptr->set_test_vcm_mode) {
			ret = cxt->ioctrl_ptr->set_test_vcm_mode(cxt->ioctrl_ptr->caller_handler, (char *)param0);
		}
		break;
	case AF_CB_CMD_GET_VCM_TEST_MODE:
		if (cxt->ioctrl_ptr->get_test_vcm_mode) {
			ret = cxt->ioctrl_ptr->get_test_vcm_mode(cxt->ioctrl_ptr->caller_handler);
		}
		break;
	case AF_CB_CMD_SET_START_NOTICE:
		ret = cxt->commn_cxt.callback(cxt->commn_cxt.caller_id,
				ISP_CALLBACK_EVT | ISP_AF_NOTICE_CALLBACK,
				param0, sizeof(struct isp_af_notice));
		break;
	case ISP_AF_AE_AWB_LOCK:
		if (cxt->ops.ae_ops.ioctrl)
			ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_PAUSE, NULL, param1);
		if (cxt->ops.awb_ops.ioctrl)
			ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_LOCK, NULL, NULL);
		break;
	case ISP_AF_AE_AWB_RELEASE:
		if (cxt->ops.ae_ops.ioctrl)
			ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_RESTORE, NULL, param1);
		if (cxt->ops.awb_ops.ioctrl)
			ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_UNLOCK, NULL, NULL);
		break;
	case AF_CB_CMD_SET_AE_LOCK:
		af_get_aelock = 1;
		if (cxt->ops.ae_ops.ioctrl)
			ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_PAUSE, NULL, param1);

		if (cxt->ops.ae_ops.ioctrl)
			ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_AF_STATUS, (void *)&af_get_aelock, NULL);
		break;
	case AF_CB_CMD_SET_AE_UNLOCK:
		af_get_aelock = 0;
		if (cxt->ops.ae_ops.ioctrl)
			ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_RESTORE, NULL, param1);

		if (cxt->ops.ae_ops.ioctrl)
			ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_AF_STATUS, (void *)&af_get_aelock,NULL);
		break;
	case AF_CB_CMD_SET_AE_CAF_LOCK:
		if (cxt->ops.ae_ops.ioctrl)
			ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_CAF_LOCKAE_START, NULL, param1);
		break;
	case AF_CB_CMD_SET_AE_CAF_UNLOCK:
		if (cxt->ops.ae_ops.ioctrl)
			ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_CAF_LOCKAE_STOP, NULL, param1);
		break;
	case AF_CB_CMD_SET_AWB_LOCK:
		if (cxt->ops.awb_ops.ioctrl)
			ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_LOCK, NULL, NULL);
		break;
	case AF_CB_CMD_SET_AWB_UNLOCK:
		if (cxt->ops.awb_ops.ioctrl)
			ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_UNLOCK, NULL, NULL);
		break;
	case AF_CB_CMD_SET_LSC_LOCK:
		if (cxt->ops.lsc_ops.ioctrl)
			ret = cxt->ops.lsc_ops.ioctrl(cxt->lsc_cxt.handle, SMART_LSC_ALG_LOCK, NULL, NULL);
		break;
	case AF_CB_CMD_SET_LSC_UNLOCK:
		if (cxt->ops.lsc_ops.ioctrl)
			ret = cxt->ops.lsc_ops.ioctrl(cxt->lsc_cxt.handle, SMART_LSC_ALG_UNLOCK, NULL, NULL);
		break;
	case AF_CB_CMD_SET_NLM_LOCK:
		cxt->smart_cxt.lock_nlm_en = 1;
		break;
	case AF_CB_CMD_SET_NLM_UNLOCK:
		cxt->smart_cxt.lock_nlm_en = 0;
		break;
	case AF_CB_CMD_SET_AFM_BYPASS:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AFM_BYPASS, param0, param1);
		break;
	case AF_CB_CMD_SET_AFM_MODE:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AFM_WORK_MODE, param0, param1);
		break;
	case AF_CB_CMD_SET_AFM_SKIP_NUM:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AFM_SKIP_NUM, param0, param1);
		break;
	case AF_CB_CMD_SET_MONITOR_WIN:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AFM_MONITOR_WIN, param0, param1);
		break;
	case AF_CB_CMD_SET_MONITOR_WIN_NUM:
	{
		cxt->af_cxt.win_num = *(struct af_monitor_win_num *)param0;
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AFM_MONITOR_WIN_NUM, param0, param1);
		break;
	}
	case AF_CB_CMD_SET_AFM_CROP_EB:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AFM_CROP_EB, param0, param1);
		break;
	case AF_CB_CMD_SET_AFM_CROP_SIZE:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AFM_CROP_SIZE, param0, param1);
		break;
	case AF_CB_CMD_SET_AFM_DONE_TILE_NUM:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AFM_DONE_TILE_NUM, param0, param1);
		break;
	case AF_CB_CMD_GET_SYSTEM_TIME:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_GET_SYSTEM_TIME, param0, param1);
		break;
	case AF_CB_CMD_SET_MOTOR_STATUS:
		ret = cxt->commn_cxt.callback(cxt->commn_cxt.caller_id,
						ISP_CALLBACK_EVT | ISP_AF_VCM_NOTICE_CALLBACK,
						param0, sizeof(cmr_u32));
		break;
	default:
		ISP_LOGE("unsupported af cb: %lx\n", type);
		break;
	}

	return ret;
}

static cmr_int ispalg_pdaf_set_cb(cmr_handle isp_alg_handle, cmr_int type, void *param0, void *param1)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;

	if (!cxt) {
		ISP_LOGE("fail to get valid cxt ptr\n");
		return ISP_PARAM_NULL;
	}

	ISP_LOGV("isp_pdaf_set_cb type = 0x%lx", type);
	switch (type) {
	case ISP_AF_SET_PD_INFO:
		if (cxt->ops.af_ops.ioctrl)
			ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle, AF_CMD_SET_PD_INFO, param0, param1);
		break;
	case ISP_PDAF_SET_CFG_PARAM:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_PDAF_CFG_PARAM, param0, param1);
		break;
	case ISP_PDAF_SET_PPI_INFO:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_PDAF_PPI_INFO, param0, param1);
		break;
	case ISP_PDAF_SET_BYPASS:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_PDAF_BYPASS, param0, param1);
		break;
	case ISP_PDAF_SET_WORK_MODE:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_PDAF_WORK_MODE, param0, param1);
		break;
	case ISP_PDAF_SET_EXTRACTOR_BYPASS:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_PDAF_EXTRACTOR_BYPASS, param0, param1);
		break;
	case ISP_PDAF_SET_ROI:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_PDAF_ROI, param0, param1);
		break;
	case ISP_PDAF_SET_SKIP_NUM:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_PDAF_SKIP_NUM, param0, param1);
		break;
	default:
		ISP_LOGE("unsupported pdaf cb: %lx\n", type);
		break;
	}

	return ret;
}

static cmr_int ispalg_tof_set_cb(cmr_handle isp_alg_handle, cmr_int type, void *param0, void *param1)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;

	switch (type) {
	case ISP_AF_SET_TOF_INFO:
		if (cxt->ops.af_ops.ioctrl)
			ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle, AF_CMD_SET_TOF_INFO, param0, param1);
		break;
	default:
		break;
	}

	return ret;
}

static cmr_int ispalg_afl_set_cb(cmr_handle isp_alg_handle, cmr_int type, void *param0, void *param1)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_u32 bypass;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;

	if (!cxt) {
		ISP_LOGE("fail to get valid cxt ptr\n");
		return ISP_PARAM_NULL;
	}
	switch (type) {
	case ISP_AFL_NEW_SET_CFG_PARAM:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AFL_NEW_CFG_PARAM, param0, param1);
		break;
	case ISP_AFL_NEW_SET_BYPASS:
		bypass = *(cmr_u32 *)param0;
		ISP_LOGV("hw_bypass= %d, new bypass %d\n", cxt->afl_cxt.hw_bypass, bypass);
		if ((cxt->afl_cxt.hw_bypass == 0) && (bypass == 0)) {
			ISP_LOGV("should not trigger afl when hw is working\n");
			break;
		}
		cxt->afl_cxt.hw_bypass = bypass;
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AFL_NEW_BYPASS, param0, param1);
		break;
	case ISP_AFL_SET_STATS_BUFFER:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_STSTIS_BUF, param0, param1);
		break;
	default:
		ISP_LOGV("unsupported afl cb: %lx\n", type);
		break;
	}

	return ret;
}
static cmr_int ispalg_ai_set_cb(cmr_handle isp_alg_handle, cmr_int type, void *param0, void *param1)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct ai_scene_detect_info *ai_info = NULL;
	enum isp_callback_cmd cmd = 0;

	switch (type) {
	case AI_CALLBACK_SCENE_INFO:
		cmd = ISP_AI_SCENE_INFO_CALLBACK;
		ai_info = (struct ai_scene_detect_info *)param0;
		cxt->commn_cxt.ai_scene_id = ai_info->cur_scene_id;
		ISP_LOGV("ai_scene_id %d", cxt->commn_cxt.ai_scene_id);
		if (cxt->commn_cxt.callback) {
			cxt->commn_cxt.callback(cxt->commn_cxt.caller_id,
				ISP_CALLBACK_EVT | cmd, ai_info, sizeof(struct ai_scene_detect_info));
		}
		break;
	case AI_CALLBACK_SET_CB:
		ret = ispalg_ai_callback(cxt, *(cmr_int *)param0, param1);
		break;
	default:
		break;
	}

	return ret;
}

static cmr_int ispalg_dump_rgbgamma_curve(struct sensor_rgbgamma_curve *gamma_cur)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_u32 i = 0;
	FILE *fp = NULL;
	char file_name[260] = { 0 };
	char loop_cnt_str[10] = { 0 };

	if (gamma_cur == NULL) {
		ISP_LOGE("fail to gamma param pointer.");
		return ISP_ERROR;
	}
	ret = isp_sim_get_mipi_raw_file_name(file_name);
	if (strlen(file_name)) {
		sprintf(loop_cnt_str, ".%d", isp_video_get_simulation_loop_count());
		strcat(file_name, loop_cnt_str);
		strcat(file_name, ".gamma.log");
		ISP_LOGI("dump to [%s] start\n", file_name);
	} else {
		ISP_LOGE("fail to get mipi raw file name.");
		return ISP_ERROR;
	}
	CMR_LOGD("file name %s", file_name);
	fp = fopen(file_name, "wb");
	if (fp == NULL) {
		ISP_LOGE("fail to open file");
		return ISP_ERROR;
	}

	fprintf(fp, "1.r gamma:\n");
	for (i=0; i < SENSOR_GAMMA_POINT_NUM; i++) {
		fprintf(fp, "<%d>:x[%d]y[%d] \n",
			i, gamma_cur->points_r[i].x, gamma_cur->points_r[i].y);
	}

	fprintf(fp, "2.g gamma:\n");
	for (i=0; i < SENSOR_GAMMA_POINT_NUM; i++) {
		fprintf(fp, "<%d>:x[%d]y[%d] \n",
			i, gamma_cur->points_g[i].x, gamma_cur->points_g[i].y);
	}

	fprintf(fp, "3.b gamma:\n");
	for (i=0; i < SENSOR_GAMMA_POINT_NUM; i++) {
		fprintf(fp, "<%d>:x[%d]y[%d] \n",
			i, gamma_cur->points_b[i].x, gamma_cur->points_b[i].y);
	}

	fflush(fp);
	fclose(fp);
	return ret;
}

static cmr_int ispalg_smart_set_cb(cmr_handle isp_alg_handle, cmr_int type, void *param0, void *param1)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_u32 i;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct smart_calc_result *smart_result = NULL;
	struct smart_block_result *block_result = NULL;
	struct isp_pm_ioctl_input io_pm_input = { NULL, 0 };
	struct isp_pm_param_data pm_param;
	char value[PROPERTY_VALUE_MAX];

	UNUSED(param1);

	switch (type) {
	case ISP_SMART_SET_COMMON:
		smart_result = (struct smart_calc_result *)param0;
		for (i = 0; i < smart_result->counts; i++) {
			block_result = &smart_result->block_result[i];

			if (ISP_SMART_LNC == block_result->smart_id){
				if (cxt->ops.lsc_ops.ioctrl)
					cxt->ops.lsc_ops.ioctrl(cxt->lsc_cxt.handle, ALSC_SET_SMART_RESULT, (void *)block_result->component[0].fix_data, NULL);	
			}

			if (block_result->update == 0)
				continue;
			if (cxt->first_frm && (block_result->block_id == ISP_BLK_RGB_GAMC)) {
				ISP_LOGV("skip gamma\n");
				continue;
			}

			if (block_result->block_id == ISP_BLK_POST_EE) {
				if (cxt->smart_cxt.cur_set_id == 2) {
					ISP_LOGD("set %d,  postee 0x%04x 0x%x, %d %d, %d %d, %d %d, %d %d, %d %d,  fixed 0x%08x 0x%08x\n",
						cxt->smart_cxt.cur_set_id, block_result->block_id, block_result->smart_id,
						block_result->update, block_result->component_num,
						block_result->mode_flag, block_result->scene_flag,
						block_result->ai_scene_id, block_result->mode_flag_changed,
						block_result->component[0].y_type, block_result->component[0].x_type,
						block_result->component[0].offset, block_result->component[0].size,
						block_result->component[0].fix_data[0], block_result->component[0].fix_data[1]);
					if (block_result->update)
						cxt->swpm_ctx.smart_postee = *block_result;
				}
				continue;
			}
			memset(&pm_param, 0, sizeof(pm_param));
			BLOCK_PARAM_CFG(io_pm_input, pm_param,
					ISP_PM_BLK_SMART_SETTING,
					block_result->block_id, block_result,
					sizeof(*block_result));
			pm_param.mode_id = block_result->mode_flag;
			ISP_LOGV("set param %d, id=%x, mode %d, data=%p",
					i, block_result->block_id, pm_param.mode_id, block_result);

			if (cxt->fdr_cxt.fdr_start == FDR_STATUS_CAPTURE)
				ISP_LOGD("set %d, idx %d, id=%x, mode %d, data=%p",
					cxt->smart_cxt.cur_set_id, i, block_result->block_id, pm_param.mode_id, block_result);

			ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_SMART, &io_pm_input, NULL);
			ISP_TRACE_IF_FAIL(ret, ("fail to set smart"));
		}

		break;

	case ISP_SMART_SET_GAMMA_CUR:
	{
		/* param0 should be (struct sensor_rgbgamma_curve *)  */
		void *gamma_cur = param0;
		cmr_u32 cfg_set_id = cxt->smart_cxt.cur_set_id;

		if (cxt->first_frm && !cxt->is_simulator) {
			ISP_LOGD("skip gamma\n");
			break;
		}
		memset(&pm_param, 0, sizeof(pm_param));
		BLOCK_PARAM_CFG(io_pm_input, pm_param,
			ISP_PM_BLK_GAMMA_CUR,
			ISP_BLK_RGB_GAMC,
			gamma_cur,
			sizeof(struct sensor_rgbgamma_curve));

		if (isp_video_get_simulation_flag()) {
			property_get("persist.vendor.cam.debug.simulation", value, "false");
			if (!strcmp(value, "true")) {
				ISP_LOGI("vvv:ISP_SMART_SET_GAMMA_CUR\n");
				ispalg_dump_rgbgamma_curve(gamma_cur);
			}
		}

		if (cxt->fdr_cxt.fdr_start)
			ISP_LOGD("set %d set gamma cur\n",cxt->smart_cxt.cur_set_id);

		ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_OTHERS, &io_pm_input, &cfg_set_id);
		ISP_TRACE_IF_FAIL(ret, ("fail to set gammar cur"));
		break;
	}
	default:
		ISP_LOGE("fail to get smart callback cmd: %d", (cmr_u32)type);
		break;
	}

	return ret;
}

static cmr_s32 ispalg_alsc_get_info(cmr_handle isp_alg_handle)
{
	cmr_s32 ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	cmr_handle pm_handle = cxt->handle_pm;
	struct isp_pm_ioctl_input io_pm_input = { NULL, 0 };
	struct isp_pm_ioctl_output io_pm_output = { NULL, 0 };
	struct isp_pm_param_data pm_param;

	ISP_LOGV("enter\n");

	pthread_mutex_lock(&cxt->pm_getting_lock);

	memset(&pm_param, 0, sizeof(struct isp_pm_param_data));
	BLOCK_PARAM_CFG(io_pm_input, pm_param,
			ISP_PM_BLK_LSC_GET_LSCTAB,
			ISP_BLK_2D_LSC, NULL, 0);
	ret = isp_pm_ioctl(pm_handle,
			ISP_PM_CMD_GET_SINGLE_SETTING,
			(void *)&io_pm_input, (void *)&io_pm_output);

	if ((ret != ISP_SUCCESS) || (io_pm_output.param_num != 1)
		|| (io_pm_output.param_data->data_ptr == NULL)) {
		pthread_mutex_unlock(&cxt->pm_getting_lock);
		ISP_LOGE("fail to get lsc tab");
		return ISP_ERROR;
	}

	cxt->lsc_cxt.lsc_tab_address = io_pm_output.param_data->data_ptr;
	cxt->lsc_cxt.lsc_tab_size = io_pm_output.param_data->data_size;

	memset(&pm_param, 0, sizeof(struct isp_pm_param_data));
	BLOCK_PARAM_CFG(io_pm_input, pm_param,
			ISP_PM_BLK_LSC_INFO,
			ISP_BLK_2D_LSC, PNULL, 0);
	ret = isp_pm_ioctl(pm_handle,
			ISP_PM_CMD_GET_SINGLE_SETTING,
			(void *)&io_pm_input, (void *)&io_pm_output);

	if ((ret != ISP_SUCCESS) || (io_pm_output.param_num != 1)
		|| (io_pm_output.param_data->data_ptr == NULL)) {
		pthread_mutex_unlock(&cxt->pm_getting_lock);
		ISP_LOGE("fail to get lsc info");
		return ISP_ERROR;
	}
	cxt->lsc_cxt.lsc_info = (struct isp_lsc_info *)io_pm_output.param_data->data_ptr;
	pthread_mutex_unlock(&cxt->pm_getting_lock);

	return ISP_SUCCESS;
}


cmr_s32 ispalg_alsc_calc(cmr_handle isp_alg_handle,
		  cmr_u32 * stat_r, cmr_u32 * stat_g, cmr_u32 * stat_b,
		  struct awb_size * stat_img_size,
		  struct awb_size * win_size,
		  cmr_u32 image_width, cmr_u32 image_height,
		  cmr_u32 awb_ct,
		  cmr_s32 awb_r_gain, cmr_s32 awb_b_gain,
		  cmr_u32 ae_stable, struct ae_ctrl_callback_in *ae_in)
{
	cmr_s32 ret = ISP_SUCCESS;
	cmr_u32 i = 0;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	lsc_adv_handle_t lsc_adv_handle = cxt->lsc_cxt.handle;
	cmr_handle pm_handle = cxt->handle_pm;
	struct isp_2d_lsc_param *lsc_tab_param_ptr;
	struct isp_lsc_info *lsc_info;
	struct alsc_update_info update_info = { 0, 0, NULL };
	struct lsc_adv_calc_param calc_param;
	struct lsc_adv_calc_result calc_result = { 0 };
	struct isp_pm_ioctl_input io_pm_input = { NULL, 0 };
	struct isp_pm_param_data pm_param;

	if (cxt->lsc_cxt.LSC_SPD_VERSION >= 2) {
		/* todo: move to fw_start */
		ret = ispalg_alsc_get_info(cxt);
		ISP_RETURN_IF_FAIL(ret, ("fail to do get lsc info"));
		lsc_tab_param_ptr = (struct isp_2d_lsc_param *)(cxt->lsc_cxt.lsc_tab_address);
		lsc_info = (struct isp_lsc_info *)cxt->lsc_cxt.lsc_info;

		memset(&calc_param, 0, sizeof(calc_param));
		calc_result.dst_gain = (cmr_u16 *) lsc_info->data_ptr;
		calc_param.stat_img.r = stat_r;
		calc_param.stat_img.gr = stat_g;
		calc_param.stat_img.gb = stat_g;
		calc_param.stat_img.b = stat_b;
		calc_param.stat_size.w = stat_img_size->w;
		calc_param.stat_size.h = stat_img_size->h;
		calc_param.gain_width = lsc_info->gain_w;
		calc_param.gain_height = lsc_info->gain_h;
		calc_param.block_size.w = win_size->w;
		calc_param.block_size.h = win_size->h;
		calc_param.lum_gain = (cmr_u16 *) lsc_info->param_ptr;
		calc_param.ct = awb_ct;
		calc_param.r_gain = awb_r_gain;
		calc_param.b_gain = awb_b_gain;
		calc_param.grid = lsc_info->grid;
		calc_param.captureFlashEnvRatio = ae_in->flash_param.captureFlashEnvRatio;
		calc_param.captureFlash1ofALLRatio = ae_in->flash_param.captureFlash1ofALLRatio;

		cxt->awb_cxt.cur_gain.r = awb_r_gain;
		cxt->awb_cxt.cur_gain.b = awb_b_gain;

		calc_param.bv = ae_in->ae_output.cur_bv;
		calc_param.bv_gain = ae_in->ae_output.cur_again;
		calc_param.ae_stable = ae_stable;
		calc_param.isp_mode = cxt->commn_cxt.isp_pm_mode[0];
		calc_param.isp_id = ISP_2_0;
		calc_param.camera_id = cxt->camera_id;
		calc_param.awb_pg_flag = cxt->awb_cxt.awb_pg_flag;

		calc_param.img_size.w = image_width;
		calc_param.img_size.h = image_height;

		for (i = 0; i < 9; i++) {
			calc_param.lsc_tab_address[i] = lsc_tab_param_ptr->map_tab[i].param_addr;
		}
		calc_param.lsc_tab_size = cxt->lsc_cxt.lsc_tab_size;

		if (cxt->lsc_cxt.isp_smart_lsc_lock == 0) {
			if (cxt->ops.lsc_ops.process){
				ret = cxt->ops.lsc_ops.process(lsc_adv_handle, &calc_param, &calc_result);
				if (ISP_SUCCESS != ret) {
					ISP_LOGE("fail to do lsc adv gain map calc");
					return ret;
				}
			}
		}

		if (cxt->ops.lsc_ops.ioctrl){
			ret = cxt->ops.lsc_ops.ioctrl(lsc_adv_handle, ALSC_GET_UPDATE_INFO, NULL, (void *)&update_info);
			if (ISP_SUCCESS != ret)
				ISP_LOGE("fail to get ALSC update flag!");

			if (update_info.alsc_update_flag == 1){
				memset(&pm_param, 0, sizeof(struct isp_pm_param_data));
				BLOCK_PARAM_CFG(io_pm_input, pm_param,
					ISP_PM_BLK_LSC_MEM_ADDR,
					ISP_BLK_2D_LSC, update_info.lsc_buffer_addr,
					lsc_info->gain_w * lsc_info->gain_h * 4 * sizeof(cmr_u16));
				ret = isp_pm_ioctl(pm_handle, ISP_PM_CMD_SET_OTHERS, &io_pm_input, NULL);
			}
		}
	}

	return ret;
}

static cmr_int ispalg_ai_pro_param_compatible(cmr_handle isp_alg_handle)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_u32 num = 0, i = 0, j = 0, cmd;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_ai_update_param ai_update_data;
	struct sensor_raw_info *raw_sensor_ptr = cxt->sn_cxt.sn_raw_info;
	struct isp_mode_param *mode_common_ptr = (struct isp_mode_param *)raw_sensor_ptr->mode_ptr[0].addr;
	struct isp_pm_ioctl_input ioctl_output = { PNULL, 0 };
	struct isp_pm_ioctl_input ioctl_input = { PNULL, 0 };
	struct isp_pm_param_data pm_param;
	struct isp_ai_bchs_param *bchs_cur[2] = { PNULL, PNULL };
	struct isp_ai_ee_param *ee_cur[2] = { PNULL, PNULL };
	cmr_u32 blk_id = 0, blk_num = 0;
	cmr_u16 smooth_ratio = 0;
	enum ai_status ai_sta = AI_STATUS_MAX;
#if defined (CONFIG_ISP_2_7) || defined (CONFIG_ISP_2_9)
	cmr_u32 hsv_blk_id = ISP_BLK_HSV_NEW2;
	cmr_u32 ee_blk_id = ISP_BLK_EE_V1;
	cmr_u32 bchs_blk_id[4] = { ISP_BLK_BCHS, ISP_BLK_ID_MAX,  ISP_BLK_ID_MAX, ISP_BLK_ID_MAX};
#elif defined CONFIG_ISP_2_8
	cmr_u32 hsv_blk_id = ISP_BLK_HSV_LUT;
	cmr_u32 ee_blk_id = ISP_BLK_EE_V1;
	cmr_u32 bchs_blk_id[4] = { ISP_BLK_BCHS, ISP_BLK_ID_MAX,  ISP_BLK_ID_MAX, ISP_BLK_ID_MAX};
#elif defined CONFIG_ISP_2_5
	cmr_u32 hsv_blk_id = ISP_BLK_HSV_NEW;
	cmr_u32 ee_blk_id = ISP_BLK_EDGE;
	cmr_u32 bchs_blk_id[4] = { ISP_BLK_BRIGHT, ISP_BLK_CONTRAST,  ISP_BLK_HUE, ISP_BLK_SATURATION};
#elif defined CONFIG_ISP_2_6
	cmr_u32 hsv_blk_id = ISP_BLK_HSV;
	cmr_u32 ee_blk_id = ISP_BLK_EE_V1;
	cmr_u32 bchs_blk_id[4] = { ISP_BLK_BCHS, ISP_BLK_ID_MAX,  ISP_BLK_ID_MAX, ISP_BLK_ID_MAX};
#endif

	if (cxt->ops.ai_ops.ioctrl) {
		ret = cxt->ops.ai_ops.ioctrl(cxt->ai_cxt.handle, AI_GET_STATUS, (void *)(&ai_sta), NULL);
		ISP_RETURN_IF_FAIL(ret, ("fail to AI_GET_STATUS"));
	}

	blk_num = mode_common_ptr->block_num;
	for(i = 0; i < blk_num; i++){
		blk_id = mode_common_ptr->block_header[i].block_id;
		if ( blk_id == ISP_BLK_AI_PRO_V1)
			cxt->is_ai_scene_pro = !mode_common_ptr->block_header[i].bypass;
		ISP_LOGV("cxt->is_ai_scene_pro = %d, blk_num = %d, i = %d", cxt->is_ai_scene_pro, blk_num, i);
	}
	ISP_LOGD("cxt->is_ai_scene_pro = %d", cxt->is_ai_scene_pro);
	if (!cxt->is_ai_scene_pro ||
		((ai_sta != AI_STATUS_PROCESSING) && !cxt->ai_scene_flag))
		return ret;

	ISP_LOGV("cam%ld ai_pro, run_status %d,  last ai_scene %d, cur scene %d",
		cxt->camera_id, ai_sta, cxt->ai_scene_flag, cxt->commn_cxt.ai_scene_id);

	num = (cxt->zsl_flag) ? 2 : 1;
	memset(&pm_param, 0, sizeof(pm_param));
	BLOCK_PARAM_CFG(ioctl_input, pm_param,
			ISP_PM_BLK_AI_SCENE_SMOOTH,
			ISP_BLK_AI_PRO_V1, PNULL, 0);
	ret = isp_pm_ioctl(cxt->handle_pm,
			ISP_PM_CMD_GET_SINGLE_SETTING,
			(void *)&ioctl_input, (void *)&ioctl_output);
			ISP_TRACE_IF_FAIL(ret, ("fail to get AI PRO PARAM"));

	if (ioctl_output.param_num == 1 && ioctl_output.param_data_ptr && ioctl_output.param_data_ptr->data_ptr)
		smooth_ratio = *(cmr_u16 *)ioctl_output.param_data_ptr->data_ptr;

	if (smooth_ratio == 0) {
		ISP_LOGW("warning: smooth ratio is 0\n");
		smooth_ratio = 1;
	}

	if (cxt->ai_scene_flag != cxt->commn_cxt.ai_scene_id) {
		ISP_LOGD("cam%ld ai_pro, run_status %d,  last ai_scene %d, cur scene %d, smooth %d",
			cxt->camera_id, ai_sta, cxt->ai_scene_flag, cxt->commn_cxt.ai_scene_id, smooth_ratio);

		cxt->smooth_ratio = smooth_ratio;
		cxt->ai_scene_flag = cxt->commn_cxt.ai_scene_id;
		if (cxt->commn_cxt.ai_scene_id == ISP_PM_AI_SCENE_PORTRAIT ||
			cxt->commn_cxt.ai_scene_id == ISP_PM_AI_SCENE_NIGHT) {
			cxt->smooth_ratio = smooth_ratio = 1;
		}
	}

	if (ai_sta != AI_STATUS_PROCESSING) {
		ai_update_data.ai_status = 0;
		cxt->smooth_ratio = smooth_ratio = 1;
		ISP_LOGD("cam%ld  ai_stopped\n", cxt->camera_id);
	} else {
		ai_update_data.ai_status = 1;
	}

	if (cxt->smooth_ratio == 0) {
		ai_update_data.smooth_factor = 0;
		ai_update_data.smooth_base = (cmr_s16)smooth_ratio;
	} else {
		ai_update_data.smooth_factor = (cmr_s16)smooth_ratio - cxt->smooth_ratio + 1;
		ai_update_data.smooth_base = (cmr_s16)smooth_ratio;
		ai_update_data.ai_scene = cxt->ai_scene_flag;
		if (!cxt->ai_scene_flag){
			ai_update_data.count = ai_update_data.smooth_factor;
			ai_update_data.smooth_factor = -1;
		}
	}

	for (i = 0; i < num; i++) {
		memset(&pm_param, 0, sizeof(pm_param));
		cmd = (i == 0) ? ISP_PM_CMD_GET_SINGLE_SETTING : ISP_PM_CMD_GET_CAP_SINGLE_SETTING;
		BLOCK_PARAM_CFG(ioctl_input, pm_param,
				ISP_PM_BLK_AI_SCENE_HSV_PARAM,
				ISP_BLK_AI_PRO_V1, PNULL, 0);
		ret = isp_pm_ioctl(cxt->handle_pm,
				cmd, (void *)&ioctl_input, (void *)&ioctl_output);
		ISP_TRACE_IF_FAIL(ret, ("fail to get hsv ai PARAM"));
#ifndef CONFIG_ISP_2_8
		struct isp_ai_hsv_info *hsv_cur[2] = { PNULL, PNULL };
		if (ioctl_output.param_num == 1 && ioctl_output.param_data_ptr && ioctl_output.param_data_ptr->data_ptr)
			hsv_cur[i] = (struct isp_ai_hsv_info *)ioctl_output.param_data_ptr->data_ptr;

		if (hsv_cur[i] && hsv_cur[i]->hue_adj_ai_eb) {
			if (cxt->smooth_ratio == 0 && !cxt->ai_scene_flag)
				continue;

			ai_update_data.param_ptr = (void *)hsv_cur[i];
			memset(&pm_param, 0, sizeof(pm_param));
			BLOCK_PARAM_CFG(ioctl_input, pm_param,
				ISP_PM_BLK_AI_SCENE_UPDATE_HSV,
				hsv_blk_id,
				&ai_update_data,
				sizeof(struct isp_ai_update_param));

			ioctl_input.param_data_ptr->mode_id = cxt->commn_cxt.isp_pm_mode[i];
			ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_AI_SCENE_PARAM, &ioctl_input, NULL);
		}
#else
		struct isp_ai_hsv_info_v1 *hsv_cur_v1[2] = { PNULL, PNULL };
		if (ioctl_output.param_num == 1 && ioctl_output.param_data_ptr && ioctl_output.param_data_ptr->data_ptr)
			hsv_cur_v1[i] = (struct isp_ai_hsv_info_v1 *)ioctl_output.param_data_ptr->data_ptr;

		if (hsv_cur_v1[i] && hsv_cur_v1[i]->hue_offset_enable) {
			if (cxt->smooth_ratio == 0 && !cxt->ai_scene_flag)
				continue;

			ai_update_data.param_ptr = (void *)hsv_cur_v1[i];
			memset(&pm_param, 0, sizeof(pm_param));
			BLOCK_PARAM_CFG(ioctl_input, pm_param,
				ISP_PM_BLK_AI_SCENE_UPDATE_HSV,
				hsv_blk_id,
				&ai_update_data,
				sizeof(struct isp_ai_update_param));

			ioctl_input.param_data_ptr->mode_id = cxt->commn_cxt.isp_pm_mode[i];
			ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_AI_SCENE_PARAM, &ioctl_input, NULL);
		}
#endif
	}

	for (i = 0; i < num; i++) {
		memset(&pm_param, 0, sizeof(pm_param));
		cmd = (i == 0) ? ISP_PM_CMD_GET_SINGLE_SETTING : ISP_PM_CMD_GET_CAP_SINGLE_SETTING;
		BLOCK_PARAM_CFG(ioctl_input, pm_param,
				ISP_PM_BLK_AI_SCENE_BCHS_PARAM,
				ISP_BLK_AI_PRO_V1, PNULL, 0);
		ret = isp_pm_ioctl(cxt->handle_pm,
				cmd, (void *)&ioctl_input, (void *)&ioctl_output);
		ISP_TRACE_IF_FAIL(ret, ("fail to get bchs ai PARAM"));
		if (ioctl_output.param_num == 1 && ioctl_output.param_data_ptr && ioctl_output.param_data_ptr->data_ptr)
			bchs_cur[i] = (struct isp_ai_bchs_param *)ioctl_output.param_data_ptr->data_ptr;

		if (bchs_cur[i]) {
			if (cxt->smooth_ratio == 0 && !cxt->ai_scene_flag)
				continue;

			ai_update_data.param_ptr = (void *)bchs_cur[i];
			for (j = 0; j < 4; j++) {
				if (bchs_blk_id[j] == ISP_BLK_ID_MAX)
					break;
				memset(&pm_param, 0, sizeof(pm_param));
				BLOCK_PARAM_CFG(ioctl_input, pm_param,
					ISP_PM_BLK_AI_SCENE_UPDATE_BCHS,
					bchs_blk_id[j],
					&ai_update_data,
					sizeof(struct isp_ai_update_param));

				ioctl_input.param_data_ptr->mode_id = cxt->commn_cxt.isp_pm_mode[i];
				ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_AI_SCENE_PARAM, &ioctl_input, NULL);
			}
		}
	}

	for (i = 0; i < num; i++) {
		memset(&pm_param, 0, sizeof(pm_param));
		cmd = (i == 0) ? ISP_PM_CMD_GET_SINGLE_SETTING : ISP_PM_CMD_GET_CAP_SINGLE_SETTING;
		BLOCK_PARAM_CFG(ioctl_input, pm_param,
				ISP_PM_BLK_AI_SCENE_EE_PARAM,
				ISP_BLK_AI_PRO_V1, PNULL, 0);
		ret = isp_pm_ioctl(cxt->handle_pm,
				cmd, (void *)&ioctl_input, (void *)&ioctl_output);
		ISP_TRACE_IF_FAIL(ret, ("fail to get bchs ai PARAM"));
		if (ioctl_output.param_num == 1 && ioctl_output.param_data_ptr && ioctl_output.param_data_ptr->data_ptr)
			ee_cur[i] = (struct isp_ai_ee_param *)ioctl_output.param_data_ptr->data_ptr;

		if (ee_cur[i] && ee_cur[i] ->ee_enable) {
			if (cxt->smooth_ratio == 0 && !cxt->ai_scene_flag)
				continue;

			ai_update_data.param_ptr = (void *)ee_cur[i];
			memset(&pm_param, 0, sizeof(pm_param));
			BLOCK_PARAM_CFG(ioctl_input, pm_param,
				ISP_PM_BLK_AI_SCENE_UPDATE_EE,
				ee_blk_id, &ai_update_data,
				sizeof(struct isp_ai_update_param));

			ioctl_input.param_data_ptr->mode_id = cxt->commn_cxt.isp_pm_mode[i];
			ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_AI_SCENE_PARAM, &ioctl_input, NULL);
		}
	}

	if (cxt->smooth_ratio > 0)
		(cxt->smooth_ratio)--;
	ISP_LOGV("cxt->smooth_ratio after %d.\n", cxt->smooth_ratio);
	return ret;
}

enum isp_pm_cmd ispalg_get_param_type(cmr_u32 cfg_type)
{
	if (cfg_type == PARAM_CFG_START){
		return ISP_PM_CMD_GET_ISP_ALL_SETTING;
	}
	else if (cfg_type == PARAM_CFG_DEFAULT){
		return ISP_PM_CMD_GET_ISP_SETTING;
	}
	else{
		return ISP_PM_CMD_GET_ISP_FDR_ALL_SETTING;
	}
}

static void ispalg_cfg_blks(cmr_handle isp_alg_handle,
		struct isp_pm_param_data *param_data, cmr_u32 param_num,
		cmr_u32 gtm_ltm_on, cmr_u32 scene_id, cmr_u32 start, int dump_lsc)
{
	cmr_u32 i = 0, is_gtm_blk, gtm_bypass, dcam_shared, blk_skip;
	struct dcam_dev_lsc_info *lsc = NULL;
	struct isp_u_blocks_info sub_block_info = { 0, NULL };
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;

	sub_block_info.scene_id = scene_id;
	dcam_shared = ((cxt->work_mode == 0) && cxt->zsl_flag &&
		(scene_id == PM_SCENE_CAP) && (cxt->remosaic_type == 0)) ? 1 : 0;

	for (i = 0; i < param_num; i++) {
		blk_skip = (IS_DCAM_BLOCK(param_data->id) && dcam_shared) ?  1 : 0;
		is_gtm_blk = ((param_data->id == ISP_BLK_RAW_GTM) ||
					(param_data->id == ISP_BLK_RGB_LTM) ||
					(param_data->id == ISP_BLK_YUV_LTM)) ? 1 : 0;
		gtm_bypass = ((gtm_ltm_on == 0) && is_gtm_blk) ? 1 : 0;
		if (blk_skip || gtm_bypass) {
			ISP_LOGV("skip block 0x%04x for scene %d, dcam_skip %d, gtm_bypass %d\n",
				param_data->id, scene_id, blk_skip, gtm_bypass);
			param_data++;
			continue;
		}

		sub_block_info.block_info = param_data->data_ptr;
		if (param_data->id == ISP_BLK_2D_LSC) {
			if (scene_id == PM_SCENE_FDRL || scene_id == PM_SCENE_FDRH) {
				lsc = (struct dcam_dev_lsc_info *)sub_block_info.block_info;
				lsc->update_all = 1;
			}
			if (dump_lsc)
				dump_lsc_data(cxt, start, scene_id, param_data->data_ptr);
		}

		isp_dev_cfg_block(cxt->dev_access_handle, &sub_block_info, param_data->id);
		ISP_LOGV("cfg block 0x%04x for scene %d\n", param_data->id, scene_id);
		param_data++;
	}
}

static cmr_s32 ispalg_cfg_param(cmr_handle isp_alg_handle, cmr_u32 cfg_type)
{
	cmr_s32 ret = ISP_SUCCESS;
	cmr_u32 scene_id, start;
	cmr_u32 gtm_ltm_on = 0, work_mode;
	cmr_u32 param_num = 0;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_pm_setting_params output;
	struct isp_pm_param_data *param_data;
	struct isp_u_blocks_info sub_block_info;
	enum isp_pm_cmd cmd = 0;
	char value[PROPERTY_VALUE_MAX] = { 0x00 };
	int dump_lsc;

	property_get("debug.vendor.cam.pm.lsc.dump", value, "0");
	dump_lsc = !!atoi(value);

	pthread_mutex_lock(&cxt->pm_getting_lock);

	memset((void *)&output, 0x00, sizeof(struct isp_pm_setting_params));

	if (cfg_type < PARAM_CFG_FDRL)
		ispalg_ai_pro_param_compatible((cmr_handle) cxt);

	cmd = ispalg_get_param_type(cfg_type);
	ret = isp_pm_ioctl(cxt->handle_pm, cmd, NULL, &output);


	ISP_RETURN_IF_FAIL(ret, ("fail to get isp block settings"));

	/* lock pm to avoid parameter is updated by algo while driver reading */
	isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_LOCK, NULL, NULL);

	/* work_mode: 1 - capture only, 0 - auto/preview */
	if ((cfg_type == PARAM_CFG_FDRL) || (cfg_type == PARAM_CFG_FDRH)) {
		gtm_ltm_on = 0;
		work_mode = 1;
		scene_id = (cfg_type == PARAM_CFG_FDRL) ? PM_SCENE_FDRL : PM_SCENE_FDRH;
		ISP_LOGD("cam%ld cfg FDR param %d\n", cxt->camera_id, cfg_type);
	} else {
		gtm_ltm_on = (cxt->gtm_ltm_on == 1) ? 1 : 0;
		work_mode = cxt->work_mode;
		scene_id = cxt->work_mode ? PM_SCENE_CAP : PM_SCENE_PRE;
	}
	start = (cfg_type == PARAM_CFG_START) ?  1 : 0;

	param_data = output.prv_param_data;
	param_num = output.prv_param_num;
	ispalg_cfg_blks((cmr_handle) cxt, param_data, param_num, gtm_ltm_on, scene_id, start, dump_lsc);

	if ((work_mode == 0) && cxt->zsl_flag) {
		param_data = output.cap_param_data;
		param_num = output.cap_param_num;
		ispalg_cfg_blks((cmr_handle) cxt, param_data, param_num, gtm_ltm_on, PM_SCENE_CAP, start, dump_lsc);
	}

	isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_UNLOCK, NULL, NULL);
	pthread_mutex_unlock(&cxt->pm_getting_lock);

	/* Enable hist statis (Y-256). Only one will take effect. Driver MUST decide it */
	if (start) {
		struct isp_dev_hist2_info hist2_info;
		struct isp_dev_hist_info hist_info;

		memset(&hist2_info, 0, sizeof(struct isp_dev_hist2_info));
		hist2_info.bypass = 0;
		hist2_info.mode = 1; /* 0 - single;  1 - continue multi */
		hist2_info.hist_roi.end_x = cxt->commn_cxt.prv_size.w - 1;
		hist2_info.hist_roi.end_y = cxt->commn_cxt.prv_size.h - 1;
		sub_block_info.block_info = &hist2_info;
		sub_block_info.scene_id = PM_SCENE_PRE;
		isp_dev_cfg_block(cxt->dev_access_handle, &sub_block_info, ISP_BLK_HIST2);

		memset(&hist_info, 0, sizeof(struct isp_dev_hist_info));
		hist_info.bypass = 0;
		hist_info.mode = 1; /* 0 - single;  1 - continue multi */
		sub_block_info.block_info = &hist_info;
		sub_block_info.scene_id = PM_SCENE_PRE;
		isp_dev_cfg_block(cxt->dev_access_handle, &sub_block_info, ISP_BLK_HIST);
	}

	return ret;
}

static cmr_int ispalg_handle_sensor_sof(cmr_handle isp_alg_handle)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_u32 sec = 0;
	cmr_u32 usec = 0;
	struct isp_af_ts af_ts;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	cmr_u32 tmp = 0;
	struct af_log_info af_param = {NULL, 0};
	struct af_log_info aft_param = {NULL, 0};
	struct debuginfo_message ae_msg = { NULL, 0, 0, 0 };
	struct debuginfo_message ai_msg = { NULL, 0, 0, 0 };
	struct debuginfo_message alsc_msg = { NULL, 0, 0, 0 };
	struct debuginfo_message af_msg = { NULL, 0, 0, 0 };
	struct debuginfo_message aft_msg = { NULL, 0, 0, 0 };

	if (cxt->ops.af_ops.ioctrl) {
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_GET_SYSTEM_TIME, &sec, &usec);
		af_ts.timestamp = sec * 1000000000LL + usec * 1000LL;
		af_ts.capture = 0;
		ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle,
					AF_CMD_SET_DCAM_TIMESTAMP, (void *)(&af_ts), NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to set AF_CMD_SET_DCAM_TIMESTAMP"));
	}

	/* should not update info during FDR post-process for long delay */
	if (s_dbg_ver && (cxt->frame_id_sof > 0) && (cxt->fdr_cxt.fdr_start != FDR_STATUS_PROC)) {
		if (ISP_SUCCESS != ispctl_get_ae_debug_info(cxt)) {
			ISP_LOGE("fail to get ae debug info");
		}
		ae_msg.msg_log = cxt->ae_cxt.log_ae;
		ae_msg.msg_size = cxt->ae_cxt.log_ae_size;
		ae_msg.frame_id = cxt->frame_id_sof - 1;
		pthread_mutex_lock(&cxt->debuginfo_queue_lock);
		debuginfo_eq(&cxt->ae_queue, &ae_msg);
		pthread_mutex_unlock(&cxt->debuginfo_queue_lock);

		if (ISP_SUCCESS != ispctl_get_ai_debug_info(cxt)) {
			ISP_LOGE("fail to get ai debug info");
		}
		ai_msg.msg_log = cxt->ai_cxt.log_ai;
		ai_msg.msg_size = cxt->ai_cxt.log_ai_size;
		ai_msg.frame_id = cxt->frame_id_sof - 1;
		pthread_mutex_lock(&cxt->debuginfo_queue_lock);
		debuginfo_eq(&cxt->ai_queue, &ai_msg);
		pthread_mutex_unlock(&cxt->debuginfo_queue_lock);

		if (ISP_SUCCESS != ispctl_get_alsc_debug_info(cxt)) {
			ISP_LOGE("fail to get alsc debug info");
		}
		alsc_msg.msg_log = cxt->lsc_cxt.log_lsc;
		alsc_msg.msg_size = cxt->lsc_cxt.log_lsc_size;
		alsc_msg.frame_id = cxt->frame_id_sof - 1;

		pthread_mutex_lock(&cxt->debuginfo_queue_lock);
		debuginfo_eq(&cxt->alsc_queue, &alsc_msg);
		pthread_mutex_unlock(&cxt->debuginfo_queue_lock);

		if (cxt->ops.af_ops.ioctrl) {
			ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle, AF_CMD_GET_AF_LOG_INFO, (void *)&af_param, NULL);
			ISP_TRACE_IF_FAIL(ret, ("fail to get_af_log_info"));
			ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle, AF_CMD_GET_AFT_LOG_INFO, (void *)&aft_param, NULL);
			ISP_TRACE_IF_FAIL(ret, ("fail to get_aft_log_info"));
		}
		af_msg.msg_log = af_param.log_cxt;
		af_msg.msg_size = af_param.log_len;
		af_msg.frame_id = cxt->frame_id_sof - 1;
		pthread_mutex_lock(&cxt->debuginfo_queue_lock);
		debuginfo_eq(&cxt->af_queue, &af_msg);
		pthread_mutex_unlock(&cxt->debuginfo_queue_lock);

		aft_msg.msg_log = aft_param.log_cxt;
		aft_msg.msg_size = aft_param.log_len;
		aft_msg.frame_id = cxt->frame_id_sof - 1;
		pthread_mutex_lock(&cxt->debuginfo_queue_lock);
		debuginfo_eq(&cxt->aft_queue, &aft_msg);
		pthread_mutex_unlock(&cxt->debuginfo_queue_lock);
	}

	/* set ambient_highlight here, if need please get from cxt */
	if (cxt->is_high_res_mode) { /* 4in1:need check ambient brightness */
		if (cxt->ops.ae_ops.ioctrl)
			ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_GET_LOWLIGHT_FLAG_BY_BV, NULL, (void *)&tmp);
		tmp = !tmp; /* need be 1 when high light */
		if (ret)
			tmp = cxt->ambient_highlight;
		if (cxt->fix_highlight) /* !=0, fix to highlight or lowlight */
			tmp = (cxt->fix_highlight == 1) ? 1 : 0;
		if (tmp != cxt->ambient_highlight) { /* value change */
			ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_LOWLIGHT_FLAG,
				&tmp, &cxt->commn_cxt.isp_pm_mode[1]);
			ISP_LOGI("ambient_highlight = %d", tmp);
		}
		cxt->ambient_highlight = tmp;
	}

	return ret;
}


static cmr_int ispalg_param_parser(cmr_handle isp_alg_handle, void *data)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_statis_info *statis_info = (struct isp_statis_info *)data;
	struct debug_base_info *pinfo;

	pinfo = (struct debug_base_info *)statis_info->uaddr;

	ISP_LOGD("cam%ld, debug info %d %d %d; dsize %d,  fid %d t: %d.%06d (fid %d t: %d.%06d)\n",
		cxt->camera_id, pinfo->dcam_cid, pinfo->isp_cid, pinfo->scene_id,
		pinfo->size, pinfo->frame_id, pinfo->sec, pinfo->usec,
		statis_info->frame_id, statis_info->sec, statis_info->usec);

	if ((pinfo->frame_id != statis_info->frame_id)
		|| (pinfo->sec != statis_info->sec)
		|| (statis_info->usec != pinfo->usec))
	ISP_LOGE("cam%ld, error:   debug info %d %d %d;   fid %d %d %d (fid1 %d %d %d)\n",
		cxt->camera_id, pinfo->dcam_cid, pinfo->isp_cid, pinfo->scene_id,
		pinfo->frame_id, pinfo->sec, pinfo->usec,
		statis_info->frame_id, statis_info->sec, statis_info->usec);

	if (pinfo->scene_id == PM_SCENE_FDRL ||
		pinfo->scene_id == PM_SCENE_FDRH ||
		pinfo->scene_id == PM_SCENE_CAP) {
		char data_name[8] = { 'D', 'R', 'V', '_', 'R', 'E', 'G' };
		struct cam_debug_data_header hdr;

		ISP_LOGD("scene %d,  debug info %d %d;  dsize %d,  fid %d   t: %d.%06d\n",
			pinfo->scene_id, pinfo->dcam_cid, pinfo->isp_cid,
			pinfo->size, pinfo->frame_id, pinfo->sec, pinfo->usec);

		memset(&hdr, 0, sizeof(struct cam_debug_data_header));
		hdr.fixed_bytes = CAMDBG_FIXED_BYTES;
		hdr.data_type = CAMINFO_DRVPM;
		hdr.data_size = (cmr_s32)(sizeof(struct debug_base_info) + pinfo->size);
		memcpy(&hdr.data_name, data_name, sizeof(hdr.data_name));

		if (cxt->dbg_cxt.fp_dat) {
			fwrite(&hdr, 1, sizeof(struct cam_debug_data_header), cxt->dbg_cxt.fp_dat);
			fwrite((void *)statis_info->uaddr, 1, hdr.data_size, cxt->dbg_cxt.fp_dat);
		}
	}

	ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_STSTIS_BUF, statis_info, NULL);
	if (ret) {
		ISP_LOGE("fail to set statis buf");
	}

	return ret;
}


static cmr_int ispalg_aem_stats_parser(cmr_handle isp_alg_handle, void *data)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_u32 ae_shift = 0, g_shift = 0;
	cmr_u32 i = 0;
	cmr_u32 blk_num = 0, blk_size = 0;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_awb_statistic_info *ae_stat_ptr = NULL;
	struct isp_statis_info *statis_info = (struct isp_statis_info *)data;
	struct dcam_dev_aem_win *ae_win;
	struct aem_win_info win;
	char value[PROPERTY_VALUE_MAX] = { 0x00 };
	int dump_aem;

	cmr_u64 *uaddr;
	cmr_u64 stats_val0 = { 0 };
	cmr_u64 stats_val1 = { 0 };
	cmr_u32 sum_r_oe = 0;
	cmr_u32 sum_g_oe = 0;
	cmr_u32 sum_b_oe = 0;
	cmr_u32 sum_r_ue = 0;
	cmr_u32 sum_g_ue = 0;
	cmr_u32 sum_b_ue = 0;
	cmr_u32 sum_r_ae = 0;
	cmr_u32 sum_g_ae = 0;
	cmr_u32 sum_b_ae = 0;

	cmr_u32 cnt_r_oe = 0;
	cmr_u32 cnt_r_ue = 0;
	cmr_u32 cnt_b_oe = 0;
	cmr_u32 cnt_b_ue = 0;
	cmr_u32 cnt_g_oe = 0;
	cmr_u32 cnt_g_ue = 0;

	ret = isp_dev_access_ioctl(cxt->dev_access_handle,
			ISP_DEV_INVALIDATE_STSTIS_BUFCACHE, statis_info, NULL);

	ae_stat_ptr = &cxt->aem_stats_data;
	ae_shift = cxt->ae_cxt.shift;

	ae_win = (struct dcam_dev_aem_win *)statis_info->uaddr;
	blk_num = ae_win->blk_num_x * ae_win->blk_num_y;
	blk_size = ae_win->blk_width * ae_win->blk_height / 4;

	ISP_LOGV("frame_id %d, time %d.%06d, ratio 0x%08x, aem win(%d %d %d %d %d %d)\n",
		statis_info->frame_id, statis_info->sec, statis_info->usec,
		statis_info->zoom_ratio,
		ae_win->offset_x, ae_win->offset_y,
		ae_win->blk_num_x, ae_win->blk_num_y,
		ae_win->blk_width, ae_win->blk_height);

	win.offset_x = (cmr_s16)(ae_win->offset_x & 0xFFFF);
	win.offset_y = (cmr_s16)(ae_win->offset_y & 0xFFFF);
	win.blk_num_x = ae_win->blk_num_x;
	win.blk_num_y = ae_win->blk_num_y;
	win.blk_size_x = ae_win->blk_width;
	win.blk_size_y = ae_win->blk_height;
	isp_br_ioctrl(cxt->sensor_role, SET_AE_WIN, &win, NULL);

	uaddr = (cmr_u64 *)(statis_info->uaddr + STATIS_AEM_HEADER_SIZE);
	for (i = 0; i < blk_num; i++) {
#ifdef CONFIG_ISP_2_6 /* sharkl5/ROC1*/
		stats_val0 = *uaddr++;
		stats_val1 = *uaddr++;
		sum_g_ue = (cmr_u32)(stats_val0 & 0xffffff);
		sum_g_ae = (cmr_u32)((stats_val0 >> 24) & 0xffffff);
		sum_g_oe = (cmr_u32)((stats_val0 >> 48) & 0xffff);
		sum_g_oe |= (cmr_u32)((stats_val1 & 0xff) << 16);
		cnt_g_ue += (cmr_u32)((stats_val1 >> 8) & 0x3fff);
		cnt_g_oe += (cmr_u32)((stats_val1 >> 22) & 0x3fff);

		stats_val0 = *uaddr++;
		stats_val1 = *uaddr++;
		sum_r_ue = (cmr_u32)(stats_val0 & 0xffffff);
		sum_r_ae = (cmr_u32)((stats_val0 >> 24) & 0xffffff);
		sum_r_oe = (cmr_u32)((stats_val0 >> 48) & 0xffff);
		sum_r_oe |= (cmr_u32)((stats_val1 & 0xff) << 16);
		cnt_r_ue += (cmr_u32)((stats_val1 >> 8) & 0x3fff);
		cnt_r_oe += (cmr_u32)((stats_val1 >> 22) & 0x3fff);

		stats_val0 = *uaddr++;
		stats_val1 = *uaddr++;
		sum_b_ue = (cmr_u32)(stats_val0 & 0xffffff);
		sum_b_ae = (cmr_u32)((stats_val0 >> 24) & 0xffffff);
		sum_b_oe = (cmr_u32)((stats_val0 >> 48) & 0xffff);
		sum_b_oe |= (cmr_u32)((stats_val1 & 0xff) << 16);
		cnt_b_ue += (cmr_u32)((stats_val1 >> 8) & 0x3fff);
		cnt_b_oe += (cmr_u32)((stats_val1 >> 22) & 0x3fff);
		g_shift = 1;

#elif defined CONFIG_ISP_2_5 /* for SharkL3 */

		stats_val0 = *uaddr++;
		sum_b_oe = stats_val0 & 0x1fffff;
		sum_g_oe = (stats_val0 >> 21) & 0x3fffff;
		sum_r_oe = (stats_val0 >> 43) & 0x1fffff;

		stats_val0 = *uaddr++;
		sum_b_ue = stats_val0 & 0x1fffff;
		sum_g_ue = (stats_val0 >> 21) & 0x3fffff;
		sum_r_ue = (stats_val0 >> 43) & 0x1fffff;

		stats_val0 = *uaddr++;
		sum_b_ae = stats_val0 & 0x1fffff;
		sum_g_ae = (stats_val0 >> 21) & 0x3fffff;
		sum_r_ae = (stats_val0 >> 43) & 0x1fffff;

		stats_val0 = *uaddr++;
		cnt_b_ue = stats_val0 & 0x1fff;
		cnt_b_oe = (stats_val0 >> 16) & 0x1fff;
		cnt_r_ue = (stats_val0 >> 32) & 0x1fff;
		cnt_r_oe = (stats_val0 >> 48) & 0x1fff;

		stats_val1 = *uaddr++;
		cnt_g_ue = stats_val1 & 0x1fff;
		cnt_g_oe = (stats_val1 >> 16) & 0x1fff;
		g_shift = 0;

#else /* CONFIG_ISP_2_7  for  SharkL5Pro */

		//g
		stats_val0 = *uaddr++;
		stats_val1 = *uaddr++;
		sum_g_ue = (cmr_u32)(stats_val0 & 0x1ffffff);
		sum_g_ae = (cmr_u32)((stats_val0 >> 32) & 0x1ffffff);
		sum_g_oe = (cmr_u32)(stats_val1 & 0x1ffffff);
		cnt_g_ue = (cmr_u32)((stats_val1 >> 32) & 0x7fff);
		cnt_g_oe = (cmr_u32)((stats_val1 >> 48) & 0x7fff);
		//r
		stats_val0 = *uaddr++;
		stats_val1 = *uaddr++;
		sum_r_ue = (cmr_u32)(stats_val0 & 0x1ffffff);
		sum_r_ae = (cmr_u32)((stats_val0 >> 32) & 0x1ffffff);
		sum_r_oe = (cmr_u32)(stats_val1 & 0x1ffffff);
		cnt_r_ue = (cmr_u32)((stats_val1 >> 32) & 0x7fff);
		cnt_r_oe = (cmr_u32)((stats_val1 >> 48) & 0x7fff);
		//b
		stats_val0 = *uaddr++;
		stats_val1 = *uaddr++;
		sum_b_ue = (cmr_u32)(stats_val0 & 0x1ffffff);
		sum_b_ae = (cmr_u32)((stats_val0 >> 32) & 0x1ffffff);
		sum_b_oe = (cmr_u32)(stats_val1 & 0x1ffffff);
		cnt_b_ue = (cmr_u32)((stats_val1 >> 32) & 0x7fff);
		cnt_b_oe = (cmr_u32)((stats_val1 >> 48) & 0x7fff);
		g_shift = 1;
#endif

		cxt->cnt_ue.r_info[i] = cnt_r_ue;
		cxt->cnt_ue.g_info[i] = cnt_g_ue;
		cxt->cnt_ue.b_info[i] = cnt_b_ue;

		cxt->cnt_oe.r_info[i] = cnt_r_oe;
		cxt->cnt_oe.g_info[i] = cnt_g_oe;
		cxt->cnt_oe.b_info[i] = cnt_b_oe;

		cxt->aem_ae.r_info[i] = sum_r_ae << ae_shift;
		cxt->aem_ae.g_info[i] = sum_g_ae << ae_shift;
		cxt->aem_ae.b_info[i] = sum_b_ae << ae_shift;

		cxt->aem_ue.r_info[i] = sum_r_ue << ae_shift;
		cxt->aem_ue.g_info[i] = sum_g_ue << ae_shift;
		cxt->aem_ue.b_info[i] = sum_b_ue << ae_shift;

		cxt->aem_oe.r_info[i] = sum_r_oe << ae_shift;
		cxt->aem_oe.g_info[i] = sum_g_oe << ae_shift;
		cxt->aem_oe.b_info[i] = sum_b_oe << ae_shift;

		ae_stat_ptr->r_info[i] = (sum_r_oe + sum_r_ue + sum_r_ae) << ae_shift;
		ae_stat_ptr->g_info[i] = (sum_g_oe + sum_g_ue + sum_g_ae) << ae_shift;
		ae_stat_ptr->b_info[i] = (sum_b_oe + sum_b_ue + sum_b_ae) << ae_shift;

		/* from sharkl5 and after, aem output G is the sum of Gr & Gb
		  * which is different from previous version average Gr/Gb
		  * here we shift 1 to get average of G for AE algo compatibility */
		ae_stat_ptr->g_info[i] >>= g_shift;

		if (ae_stat_ptr->r_info[i] > (blk_size * 1023) ||
			ae_stat_ptr->g_info[i] > (blk_size * 1023) ||
			ae_stat_ptr->b_info[i] > (blk_size * 1023) )
			ISP_LOGV("data overflow. i %d, blk size %d %d,  r 0x%x, g 0x%x, b 0x%x\n",
				i, cxt->ae_cxt.win_size.w, cxt->ae_cxt.win_size.h,
				ae_stat_ptr->r_info[i], ae_stat_ptr->g_info[i], ae_stat_ptr->b_info[i]);
	}

	property_get("debug.vendor.cam.aem_stats.dump", value, "0");
	dump_aem = !!atoi(value);
	if (dump_aem) {
		dump_aem_pic(cxt, statis_info, blk_size,
			ae_win->blk_num_x, ae_win->blk_num_y,
			ae_stat_ptr->r_info, ae_stat_ptr->g_info, ae_stat_ptr->b_info);
	}

	ae_stat_ptr->sec = statis_info->sec;
	ae_stat_ptr->usec = statis_info->usec;

	ISP_LOGV("frame_id %d, time %d.%06d\n",
		statis_info->frame_id, statis_info->sec, statis_info->usec);
	ISP_LOGV("sum[0]: r 0x%x, g 0x%x, b 0x%x cam[%d]\n",
		ae_stat_ptr->r_info[0], ae_stat_ptr->g_info[0], ae_stat_ptr->b_info[0], (cmr_u32)cxt->camera_id);
	ISP_LOGV("cnt: r 0x%x, 0x%x,  g 0x%x, 0x%x, b 0x%x, 0x%x cam[%d]\n",
		cnt_r_ue, cnt_r_oe, cnt_g_ue, cnt_g_oe, cnt_b_ue, cnt_b_oe, (cmr_u32)cxt->camera_id);

	if (isp_video_get_simulation_loop_count() == 0 &&
		cxt->takepicture_mode == CAMERA_ISP_SIMULATION_MODE) {
		isp_sim_save_ae_stats(ae_stat_ptr,
			cxt->ae_cxt.win_num.w,
			cxt->ae_cxt.win_num.h);
	}

	cxt->ai_cxt.ae_param.frame_id = statis_info->frame_id;
	cxt->ai_cxt.ae_param.zoom_ratio = statis_info->zoom_ratio;
	cxt->ai_cxt.ae_param.sec = statis_info->sec;
	cxt->ai_cxt.ae_param.usec = statis_info->usec;

	ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_STSTIS_BUF, statis_info, NULL);
	if (ret) {
		ISP_LOGE("fail to set statis buf");
	}

	/*
	  * This is a workaround for sharkl5: first frame aem value is smaller
	  * todo: find the root-cause of first frame aem data abnormal and fix it
	  */
	if (cxt->first_frm)
		cxt->first_frm = 0;
	else
		cxt->aem_is_update = 1;

	return ret;
}

static cmr_int ispalg_lscm_stats_parser(cmr_handle isp_alg_handle, void *data)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_u32 i = 0;
	cmr_u32 blk_num = 0;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_lsc_statistic_info *lscm_stat_ptr = NULL;
	struct isp_statis_info *statis_info = (struct isp_statis_info *)data;

	cmr_u64 *uaddr;
	cmr_u64 stats_val0 = { 0 };
	cmr_u64 stats_val1 = { 0 };
	cmr_u32 sum_g = 0;
	cmr_u32 sum_g1 = 0;

	ret = isp_dev_access_ioctl(cxt->dev_access_handle,
			ISP_DEV_INVALIDATE_STSTIS_BUFCACHE, statis_info, NULL);

	lscm_stat_ptr = &cxt->lscm_stats_data;
	blk_num = cxt->lsc_cxt.lscm_win_num.w * cxt->lsc_cxt.lscm_win_num.h;
	uaddr = (cmr_u64 *)statis_info->uaddr;
	for (i = 0; i < blk_num; i++) {
		stats_val0 = *uaddr++;
		stats_val1 = *uaddr++;
		lscm_stat_ptr->b_info[i] = (cmr_u32)(stats_val0 & 0xffffff);
		lscm_stat_ptr->r_info[i] = (cmr_u32)((stats_val0 >> 24) & 0xffffff);
		sum_g = (cmr_u32)((stats_val0 >> 48) & 0xffff);
		sum_g1 = (cmr_u32)(stats_val1 & 0x1ff);
		lscm_stat_ptr->g_info[i] = (sum_g1 << 16) + sum_g;
	}

	ISP_LOGV("sum[0]: r %d, g %d, b %d cam[%d]\n",
		lscm_stat_ptr->r_info[0], lscm_stat_ptr->g_info[0], lscm_stat_ptr->b_info[0], (cmr_u32)cxt->camera_id);

	ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_STSTIS_BUF, statis_info, NULL);
	if (ret) {
		ISP_LOGE("fail to set statis buf");
	}

	cxt->lscm_is_update = 1;
	cxt->lsc_cxt.use_lscm = 1;

	return ret;
}

static cmr_int ispalg_bayerhist_stats_parser(cmr_handle isp_alg_handle, void *data)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_u32 i, j;
	cmr_u64 *ptr;
	cmr_u64 val0, val1;
	struct isp_hist_statistic_info *hist_stats;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_statis_info *statis_info = (struct isp_statis_info *)data;
	struct dcam_dev_hist_info *hist_info;
	struct img_rect win;
	struct hist_param param;

	ret = isp_dev_access_ioctl(cxt->dev_access_handle,
			ISP_DEV_INVALIDATE_STSTIS_BUFCACHE, statis_info, NULL);

	cxt->bayerhist_update = 1;

	hist_info = (struct dcam_dev_hist_info *)statis_info->uaddr;
	win.start_x = hist_info->bayer_hist_stx;
	win.start_y = hist_info->bayer_hist_sty;
	win.width = hist_info->bayer_hist_endx - hist_info->bayer_hist_stx;
	win.height = hist_info->bayer_hist_endy - hist_info->bayer_hist_sty;
	isp_br_ioctrl(cxt->sensor_role, SET_HIST_WIN, &win, NULL);

	// TODO when below API works properly, remove above one
	param.win.st_x = hist_info->bayer_hist_stx;
	param.win.st_y = hist_info->bayer_hist_sty;
	param.win.width = hist_info->bayer_hist_endx - hist_info->bayer_hist_stx;
	param.win.height = hist_info->bayer_hist_endy - hist_info->bayer_hist_sty;
	param.idx = statis_info->frame_id;
	param.sec = statis_info->sec;
	param.usec = statis_info->usec;
	isp_br_ioctrl(cxt->sensor_role, SET_HIST_PARAM, &param, NULL);

	ptr = (cmr_u64 *)(statis_info->uaddr + STATIS_HIST_HEADER_SIZE);

#if defined (CONFIG_ISP_2_6) || defined (CONFIG_ISP_2_7) || defined (CONFIG_ISP_2_8)
	/* G */
	hist_stats = &cxt->bayer_hist_stats[0];
	hist_stats->bin = 256;
	hist_stats->sec = statis_info->sec;
	hist_stats->usec = statis_info->usec;
	hist_stats->frame_id = statis_info->frame_id;
	j = 0;
	for (i = 0; i < 51; i++) {
		val0 = *ptr++;
		val1 = *ptr++;
		hist_stats->value[j++] = (cmr_u32)(val0 & 0xffffff);
		hist_stats->value[j++] = (cmr_u32)((val0 >> 24) & 0xffffff);
		hist_stats->value[j++] = (cmr_u32)(((val1 & 0xff) << 16) | ((val0 >> 48) & 0xffff));
		hist_stats->value[j++] = (cmr_u32)((val1 >> 8) & 0xffffff);
		hist_stats->value[j++] = (cmr_u32)((val1 >> 32) & 0xffffff);
	}
	val0 = *ptr++;
	val1 = *ptr++;
	i++;
	hist_stats->value[j++] = (cmr_u32)(val0 & 0xffffff);

	/* R */
	hist_stats = &cxt->bayer_hist_stats[1];
	hist_stats->bin = 256;
	hist_stats->sec = statis_info->sec;
	hist_stats->usec = statis_info->usec;
	hist_stats->frame_id = statis_info->frame_id;
	j = 0;
	hist_stats->value[j++] = (cmr_u32)((val0 >> 24) & 0xffffff);
	hist_stats->value[j++] = (cmr_u32)(((val1 & 0xff) << 16) | ((val0 >> 48) & 0xffff));
	hist_stats->value[j++] = (cmr_u32)((val1 >> 8) & 0xffffff);
	hist_stats->value[j++] = (cmr_u32)((val1 >> 32) & 0xffffff);
	for ( ; i < 102; i++) {
		val0 = *ptr++;
		val1 = *ptr++;
		hist_stats->value[j++] = (cmr_u32)(val0 & 0xffffff);
		hist_stats->value[j++] = (cmr_u32)((val0 >> 24) & 0xffffff);
		hist_stats->value[j++] = (cmr_u32)(((val1 & 0xff) << 16) | ((val0 >> 48) & 0xffff));
		hist_stats->value[j++] = (cmr_u32)((val1 >> 8) & 0xffffff);
		hist_stats->value[j++] = (cmr_u32)((val1 >> 32) & 0xffffff);
	}
	val0 = *ptr++;
	val1 = *ptr++;
	i++;
	hist_stats->value[j++] = (cmr_u32)(val0 & 0xffffff);
	hist_stats->value[j++] = (cmr_u32)((val0 >> 24) & 0xffffff);

	/* B */
	hist_stats = &cxt->bayer_hist_stats[2];
	hist_stats->bin = 256;
	hist_stats->sec = statis_info->sec;
	hist_stats->usec = statis_info->usec;
	hist_stats->frame_id = statis_info->frame_id;
	j = 0;
	hist_stats->value[j++] = (cmr_u32)(((val1 & 0xff) << 16) | ((val0 >> 48) & 0xffff));
	hist_stats->value[j++] = (cmr_u32)((val1 >> 8) & 0xffffff);
	hist_stats->value[j++] = (cmr_u32)((val1 >> 32) & 0xffffff);
	for ( ; i < 153; i++) {
		val0 = *ptr++;
		val1 = *ptr++;
		hist_stats->value[j++] = (cmr_u32)(val0 & 0xffffff);
		hist_stats->value[j++] = (cmr_u32)((val0 >> 24) & 0xffffff);
		hist_stats->value[j++] = (cmr_u32)(((val1 & 0xff) << 16) | ((val0 >> 48) & 0xffff));
		hist_stats->value[j++] = (cmr_u32)((val1 >> 8) & 0xffffff);
		hist_stats->value[j++] = (cmr_u32)((val1 >> 32) & 0xffffff);
	}
	val0 = *ptr++;
	val1 = *ptr++;
	i ++;
	hist_stats->value[j++] = (cmr_u32)(val0 & 0xffffff);
	hist_stats->value[j++] = (cmr_u32)((val0 >> 24) & 0xffffff);
	hist_stats->value[j++] = (cmr_u32)(((val1 & 0xff) << 16) | ((val0 >> 48) & 0xffff));

#else
	/* Change from 24bits to 25bits per value from QogirN6pro*/
	/* G */
	hist_stats = &cxt->bayer_hist_stats[0];
	hist_stats->bin = 256;
	hist_stats->sec = statis_info->sec;
	hist_stats->usec = statis_info->usec;
	hist_stats->frame_id = statis_info->frame_id;
	j = 0;
	for (i = 0; i < 51; i++) {
		val0 = *ptr++;
		val1 = *ptr++;
		hist_stats->value[j++] = (cmr_u32)(val0 & 0x1ffffff);
		hist_stats->value[j++] = (cmr_u32)((val0 >> 25) & 0x1ffffff);
		hist_stats->value[j++] = (cmr_u32)(((val1 & 0x7ff) << 14) | ((val0 >> 50) & 0x3fff));
		hist_stats->value[j++] = (cmr_u32)((val1 >> 11) & 0x1ffffff);
		hist_stats->value[j++] = (cmr_u32)((val1 >> 36) & 0x1ffffff);
	}
	val0 = *ptr++;
	val1 = *ptr++;
	i++;
	hist_stats->value[j++] = (cmr_u32)(val0 & 0x1ffffff);

	/* R */
	hist_stats = &cxt->bayer_hist_stats[1];
	hist_stats->bin = 256;
	hist_stats->sec = statis_info->sec;
	hist_stats->usec = statis_info->usec;
	hist_stats->frame_id = statis_info->frame_id;
	j = 0;
	hist_stats->value[j++] = (cmr_u32)((val0 >> 25) & 0x1ffffff);
	hist_stats->value[j++] = (cmr_u32)(((val1 & 0x7ff) << 14) | ((val0 >> 50) & 0x3fff));
	hist_stats->value[j++] = (cmr_u32)((val1 >> 11) & 0x1ffffff);
	hist_stats->value[j++] = (cmr_u32)((val1 >> 36) & 0x1ffffff);
	for ( ; i < 102; i++) {
		val0 = *ptr++;
		val1 = *ptr++;
		hist_stats->value[j++] = (cmr_u32)(val0 & 0x1ffffff);
		hist_stats->value[j++] = (cmr_u32)((val0 >> 25) & 0x1ffffff);
		hist_stats->value[j++] = (cmr_u32)(((val1 & 0x7ff) << 14) | ((val0 >> 50) & 0x3fff));
		hist_stats->value[j++] = (cmr_u32)((val1 >> 11) & 0x1ffffff);
		hist_stats->value[j++] = (cmr_u32)((val1 >> 36) & 0x1ffffff);
	}
	val0 = *ptr++;
	val1 = *ptr++;
	i++;
	hist_stats->value[j++] = (cmr_u32)(val0 & 0x1ffffff);
	hist_stats->value[j++] = (cmr_u32)((val0 >> 25) & 0x1ffffff);

	/* B */
	hist_stats = &cxt->bayer_hist_stats[2];
	hist_stats->bin = 256;
	hist_stats->sec = statis_info->sec;
	hist_stats->usec = statis_info->usec;
	hist_stats->frame_id = statis_info->frame_id;
	j = 0;
	hist_stats->value[j++] = (cmr_u32)(((val1 & 0x7ff) << 14) | ((val0 >> 50) & 0x3fff));
	hist_stats->value[j++] = (cmr_u32)((val1 >> 11) & 0x1ffffff);
	hist_stats->value[j++] = (cmr_u32)((val1 >> 36) & 0x1ffffff);
	for ( ; i < 153; i++) {
		val0 = *ptr++;
		val1 = *ptr++;
		hist_stats->value[j++] = (cmr_u32)(val0 & 0x1ffffff);
		hist_stats->value[j++] = (cmr_u32)((val0 >> 25) & 0x1ffffff);
		hist_stats->value[j++] = (cmr_u32)(((val1 & 0x7ff) << 14) | ((val0 >> 50) & 0x3fff));
		hist_stats->value[j++] = (cmr_u32)((val1 >> 11) & 0x1ffffff);
		hist_stats->value[j++] = (cmr_u32)((val1 >> 36) & 0x1ffffff);
	}
	val0 = *ptr++;
	val1 = *ptr++;
	i ++;
	hist_stats->value[j++] = (cmr_u32)(val0 & 0x1ffffff);
	hist_stats->value[j++] = (cmr_u32)((val0 >> 25) & 0x1ffffff);
	hist_stats->value[j++] = (cmr_u32)(((val1 & 0x7ff) << 14) | ((val0 >> 50) & 0x3fff));

	if (cxt->save_data){
		FILE *fp = NULL;
		char file_name[256];
		cmr_u32 pix_cnt[3] = { 0, 0, 0 };
		sprintf(file_name, "%scam%d_bayerhsit_frm%05d.txt", CAMERA_DUMP_PATH,
			(cmr_u32)cxt->camera_id, statis_info->frame_id);
		fp = fopen(file_name, "w");
		if (fp != NULL) {
			for (j = 0; j < 256; j++) {
				fprintf(fp,  "No.%d,  cnt G %08d  R %08d  B %08d\n", j,
					cxt->bayer_hist_stats[0].value[i],
					cxt->bayer_hist_stats[1].value[i],
					cxt->bayer_hist_stats[2].value[i]);
				pix_cnt[0] += cxt->bayer_hist_stats[0].value[i];
				pix_cnt[1] += cxt->bayer_hist_stats[1].value[i];
				pix_cnt[2] += cxt->bayer_hist_stats[2].value[i];
			}
			fprintf(fp, "hist ROI (%d %d %d %d),  total cnt G %08d,  R %08d,  B %08d\n",
				win.start_x, win.start_y, win.width, win.height,
				pix_cnt[0], pix_cnt[1], pix_cnt[2]);
			fclose(fp);
		}
	}
#endif

	ISP_LOGD("cam%ld, frm %d, time %d.%06d, roi (%d %d %d %d),  data: r %d %d, g %d %d, b %d %d\n",
		cxt->camera_id, statis_info->frame_id, statis_info->sec, statis_info->usec,
		hist_info->bayer_hist_stx, hist_info->bayer_hist_sty,
		hist_info->bayer_hist_endx, hist_info->bayer_hist_endy,
		cxt->bayer_hist_stats[0].value[0], cxt->bayer_hist_stats[0].value[1],
		cxt->bayer_hist_stats[1].value[0], cxt->bayer_hist_stats[1].value[1],
		cxt->bayer_hist_stats[2].value[0], cxt->bayer_hist_stats[2].value[1]);

	ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_STSTIS_BUF, statis_info, NULL);
	if (ret) {
		ISP_LOGE("fail to set statis buf");
	}

	return ret;
}

static cmr_int ispalg_hist2_stats_parser(cmr_handle isp_alg_handle, void *data)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_u32 i;
	cmr_u32 *ptr;
	struct isp_hist_statistic_info *hist_stats;
	struct ae_size *hist2_roi_info;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_statis_info *statis_info = (struct isp_statis_info *)data;

	ptr = (cmr_u32 *)statis_info->uaddr;

	/* update hist2 roi info */
	hist2_roi_info = &cxt->hist2_roi;
	hist2_roi_info->w = statis_info->width;
	hist2_roi_info->h = statis_info->height;

	/* Y */
	hist_stats = &cxt->hist2_stats;
	hist_stats->bin = 256;
	hist_stats->sec = statis_info->sec;
	hist_stats->usec = statis_info->usec;
	hist_stats->frame_id = statis_info->frame_id;

	for(i = 0; i < 256; i++) {
		hist_stats->value[i] = *(ptr + i);
	}

	ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_STSTIS_BUF, statis_info, NULL);
	if (ret) {
		ISP_LOGE("fail to set statis buf");
	}

	ISP_LOGV("frame %d, time %03d.%06d, image size %d %d\n", hist_stats->frame_id,
		hist_stats->sec, hist_stats->usec, hist2_roi_info->w, hist2_roi_info->h);
	ISP_LOGV("data %d %d %d %d,  %d %d %d %d\n",
		hist_stats->value[0], hist_stats->value[8] , hist_stats->value[16], hist_stats->value[32],
		hist_stats->value[64], hist_stats->value[128] , hist_stats->value[192], hist_stats->value[255]);
	return ret;
}

static cmr_int ispalg_hist2_process(cmr_handle isp_alg_handle, void *data)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;

	ret = ispalg_hist2_stats_parser(cxt, data);
	if (ret) {
		ISP_LOGE("fail to parse hist2 stats");
		return ret;
	}

	if (cxt->commn_cxt.callback) {
		ret = cxt->commn_cxt.callback(cxt->commn_cxt.caller_id,
					      ISP_CALLBACK_EVT | ISP_HIST_REPORT_CALLBACK,
					      cxt->hist2_stats.value,
					      sizeof(cmr_u32) * 256);
		if (ret) {
			ISP_LOGE("fail to report hist2 stats");
			return ret;
		}
	}

	return ret;
}

static cmr_int ispalg_3dnr_statis_parser(cmr_handle isp_alg_handle, void *data) {
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_statis_info *statis_info = (struct isp_statis_info *)data;

	/* just send buffer back for now */

	ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_STSTIS_BUF, statis_info, NULL);
	if (ret) {
		ISP_LOGE("fail to set statis buf");
	}

	return ret;
}

cmr_int ispalg_start_ae_process(cmr_handle isp_alg_handle)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_int nxt_flicker = 0;
	cmr_u32 awb_mode = 0;
	nsecs_t time_start = 0;
	nsecs_t time_end = 0;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct ae_calc_in in_param;
	struct ae_calc_out ae_result;
	struct awb_gain gain = {0, 0, 0};
	struct awb_gain cur_gain = {0, 0, 0};
	struct afl_ctrl_proc_out afl_info = {0, 0, 0};

	if (cxt->ops.awb_ops.ioctrl) {
		ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_GET_GAIN, (void *)&gain, NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to AWB_CTRL_CMD_GET_GAIN"));
	}

	if (cxt->ops.awb_ops.ioctrl) {
		ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_GET_CUR_GAIN, (void *)&cur_gain, NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to AWB_CTRL_CMD_GET_CUR_GAIN"));
	}

	if(cxt->ops.awb_ops.ioctrl){
		ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle,
					AWB_CTRL_CMD_GET_WB_MODE, NULL, (void *)&awb_mode);
		ISP_TRACE_IF_FAIL(ret, ("fail to AWB_CTRL_CMD_GET_GAIN"));
	}

	if (cxt->ops.afl_ops.ioctrl) {
		ret = cxt->ops.afl_ops.ioctrl(cxt->afl_cxt.handle, AFL_GET_INFO, (void *)&afl_info, NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to AFL_GET_INFO"));
	}

	if (afl_info.flag) {
		if (afl_info.cur_flicker) {
			nxt_flicker = AE_FLICKER_50HZ;
		} else {
			nxt_flicker = AE_FLICKER_60HZ;
		}
		if (cxt->ops.ae_ops.ioctrl)
			ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_FLICKER, &nxt_flicker, NULL);
	}

	memset((void *)&ae_result, 0, sizeof(ae_result));
	memset((void *)&in_param, 0, sizeof(in_param));
	if ((0 == gain.r) || (0 == gain.g) || (0 == gain.b) ||
		(0 == cur_gain.r) || (0 == cur_gain.g) || (0 == cur_gain.b) ) {
		in_param.awb_gain_r = 1024;
		in_param.awb_gain_g = 1024;
		in_param.awb_gain_b = 1024;
		in_param.awb_cur_gain_r = 1024;
		in_param.awb_cur_gain_g = 1024;
		in_param.awb_cur_gain_b = 1024;
		in_param.awb_mode = awb_mode;
	} else {
		in_param.awb_gain_r = gain.r;
		in_param.awb_gain_g = gain.g;
		in_param.awb_gain_b = gain.b;
		in_param.awb_cur_gain_r = cur_gain.r;
		in_param.awb_cur_gain_g = cur_gain.g;
		in_param.awb_cur_gain_b = cur_gain.b;
		in_param.awb_mode = awb_mode;
	}

	in_param.stat_fmt = AE_AEM_FMT_RGB;
	in_param.rgb_stat_img = cxt->aem_stats_data.r_info;
	in_param.stat_img = cxt->aem_stats_data.r_info;
	in_param.sec = cxt->aem_stats_data.sec;
	in_param.usec = cxt->aem_stats_data.usec;
	in_param.sum_ue_r = cxt->aem_ue.r_info;
	in_param.sum_ue_g = cxt->aem_ue.g_info;
	in_param.sum_ue_b = cxt->aem_ue.b_info;
	in_param.sum_ae_r = cxt->aem_ae.r_info;
	in_param.sum_ae_g = cxt->aem_ae.g_info;
	in_param.sum_ae_b = cxt->aem_ae.b_info;
	in_param.sum_oe_r = cxt->aem_oe.r_info;
	in_param.sum_oe_g = cxt->aem_oe.g_info;
	in_param.sum_oe_b = cxt->aem_oe.b_info;
	in_param.cnt_ue_r = cxt->cnt_ue.r_info;
	in_param.cnt_ue_g = cxt->cnt_ue.g_info;
	in_param.cnt_ue_b = cxt->cnt_ue.b_info;
	in_param.cnt_oe_r = cxt->cnt_oe.r_info;
	in_param.cnt_oe_g = cxt->cnt_oe.g_info;
	in_param.cnt_oe_b = cxt->cnt_oe.b_info;
	in_param.monoboottime = -1;
	in_param.is_last_frm = 0;
	in_param.time_diff = -1;
	in_param.is_update = cxt->aem_is_update;
	in_param.sensor_fps.mode = cxt->sensor_fps.mode;
	in_param.sensor_fps.max_fps = cxt->sensor_fps.max_fps;
	in_param.sensor_fps.min_fps = cxt->sensor_fps.min_fps;
	in_param.sensor_fps.is_high_fps = cxt->sensor_fps.is_high_fps;
	in_param.sensor_fps.high_fps_skip_num = cxt->sensor_fps.high_fps_skip_num;
	if (cxt->ebd_cxt.ebd_support) {
		memcpy(&in_param.ebd_info, &cxt->ae_cxt.ebd_info, sizeof(cxt->ae_cxt.ebd_info));
		in_param.isp_dgain.global_gain = cxt->rgb_gain.global_gain;
		in_param.isp_dgain.r_gain = cxt->rgb_gain.r_gain;
		in_param.isp_dgain.g_gain = cxt->rgb_gain.g_gain;
		in_param.isp_dgain.b_gain = cxt->rgb_gain.b_gain;
	}
	/* copy isp hist2 statis and pass to ae algo */
	memcpy((void *)&in_param.hist_stats, (void *)&cxt->hist2_stats,
		sizeof(struct isp_hist_statistic_info));

	if (cxt->bayerhist_update) {
		memcpy((void *)&in_param.bayerhist_stats[0],
			(void *)&cxt->bayer_hist_stats[0],
			sizeof(cxt->bayer_hist_stats));
	}

	time_start = ispalg_get_sys_timestamp();
	if (cxt->ops.ae_ops.process) {
		ret = cxt->ops.ae_ops.process(cxt->ae_cxt.handle, &in_param, &ae_result);
		ISP_TRACE_IF_FAIL(ret, ("fail to ae process"));
	}
	cxt->smart_cxt.isp_smart_eb = 1;
	time_end = ispalg_get_sys_timestamp();
	ISP_LOGV("SYSTEM_TEST-ae:%d ms", (cmr_u32)(time_end - time_start));

	ISP_LOGV("done %ld", ret);
	return ret;
}

cmr_int ispalg_awb_pre_process(cmr_handle isp_alg_handle,
				struct ae_ctrl_callback_in *ae_in,
				struct awb_ctrl_calc_param * out_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_s32 i = 0;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct ae_monitor_info info;
	struct ae_get_ev ae_ev;
	struct isp_pm_ioctl_input io_pm_input = { NULL, 0 };
	struct isp_pm_ioctl_output io_pm_output = { NULL, 0 };
	struct isp_pm_param_data pm_param;

	memset(&ae_ev, 0, sizeof(ae_ev));
	memset(&info, 0, sizeof(info));
	ISP_LOGV("cur_iso:%d monitor_info h:%d w:%d again:%d ev_level:%d",
		ae_in->ae_output.cur_iso,
		ae_in->monitor_info.win_size.h,
		ae_in->monitor_info.win_size.w,
		ae_in->ae_output.cur_again,
		ae_in->ae_ev.ev_index);

	out_ptr->quick_mode = 0;
	out_ptr->stat_img.chn_img.r = cxt->aem_stats_data.r_info;
	out_ptr->stat_img.chn_img.g = cxt->aem_stats_data.g_info;
	out_ptr->stat_img.chn_img.b = cxt->aem_stats_data.b_info;

	out_ptr->bv = ae_in->ae_output.cur_bv;

	out_ptr->ae_info.bv = ae_in->ae_output.cur_bv;
	out_ptr->ae_info.iso = ae_in->ae_output.cur_iso;
	out_ptr->ae_info.gain = ae_in->ae_output.cur_again;
	out_ptr->ae_info.exposure = ae_in->ae_output.exposure_time;
	out_ptr->ae_info.f_value = 2.2;
	out_ptr->ae_info.stable = ae_in->ae_output.is_stab;
	out_ptr->ae_info.ev_index = ae_in->ae_ev.ev_index;
	memcpy(out_ptr->ae_info.ev_table, ae_in->ae_ev.ev_tab, 16 * sizeof(cmr_s32));

	if (cxt->fdr_cxt.fdr_start)
		ISP_LOGD("fdr_start %d, lum %d, %d, gain %d %d iso %d, bv %d %d,  ex_time %f, %d, sec %04d.%06d\n",
			cxt->fdr_cxt.fdr_start, ae_in->ae_output.cur_lum, ae_in->ae_output.cur_index,
			ae_in->ae_output.cur_again, ae_in->ae_output.cur_dgain, ae_in->ae_output.cur_iso,
			ae_in->ae_output.cur_bv, ae_in->ae_output.cur_ev, ae_in->ae_output.exposure_time,
			ae_in->ae_output.is_stab, ae_in->ae_output.sec, ae_in->ae_output.usec);
	if (cxt->fdr_cxt.fdr_start == FDR_STATUS_CAPTURE)
		cxt->fdr_cxt.frm_idx++;

	out_ptr->scalar_factor = (ae_in->monitor_info.win_size.h / 2) * (ae_in->monitor_info.win_size.w / 2);

	if (cxt->awb_cxt.color_support) {
		if (cxt->ioctrl_ptr->sns_ioctl) {
			cxt->ioctrl_ptr->sns_ioctl(cxt->ioctrl_ptr->caller_handler,
						CMD_SNS_IC_GET_CCT_DATA,
						&out_ptr->xyz_info);
		}
	}

	memset(&pm_param, 0, sizeof(pm_param));

	BLOCK_PARAM_CFG(io_pm_input, pm_param,
			ISP_PM_BLK_CMC10, ISP_BLK_CMC10, 0, 0);
	ret = isp_pm_ioctl(cxt->handle_pm,
			ISP_PM_CMD_GET_SINGLE_SETTING,
			&io_pm_input, &io_pm_output);

	if (io_pm_output.param_data != NULL && ISP_SUCCESS == ret) {
		cmr_u16 *cmc_info = io_pm_output.param_data->data_ptr;

		if (cmc_info != NULL) {
			for (i = 0; i < 9; i++) {
				out_ptr->matrix[i] = CMC10(cmc_info[i]);
			}
		}
	}

	BLOCK_PARAM_CFG(io_pm_input, pm_param,
			ISP_PM_BLK_GAMMA, ISP_BLK_RGB_GAMC, 0, 0);
	ret = isp_pm_ioctl(cxt->handle_pm,
			ISP_PM_CMD_GET_SINGLE_SETTING,
			&io_pm_input, &io_pm_output);
#if 1
	if (io_pm_output.param_data != NULL && ISP_SUCCESS == ret) {
		struct isp_dev_gamma_info *gamma_info = io_pm_output.param_data->data_ptr;

		if (gamma_info != NULL) {
			for (i = 0; i < 256; i++) {
				out_ptr->gamma[i] = (gamma_info->gain_r[i] + gamma_info->gain_r[i + 1]) / 2;
			}
		}
	}
#endif

exit:
	ISP_LOGV("done %ld", ret);
	return ret;
}

cmr_int ispalg_awb_post_process(cmr_handle isp_alg_handle, struct awb_ctrl_calc_result *awb_output)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_u32 i = 0;
	cmr_u16 lsc_param_tmp[4] = {0};
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_pm_ioctl_input pm_input = { NULL, 0 };
	struct isp_pm_param_data pm_data;
	struct isp_awbc_cfg awbc_cfg;

	memset((void *)&awbc_cfg, 0, sizeof(awbc_cfg));

	awbc_cfg.r_gain = awb_output->gain.r;
	awbc_cfg.g_gain = awb_output->gain.g;
	awbc_cfg.b_gain = awb_output->gain.b;
	awbc_cfg.r_offset = 0;
	awbc_cfg.g_offset = 0;
	awbc_cfg.b_offset = 0;

	if (0 == awbc_cfg.r_gain && 0 == awbc_cfg.g_gain && 0 == awbc_cfg.b_gain) {
		awbc_cfg.r_gain = 1800;
		awbc_cfg.g_gain = 1024;
		awbc_cfg.b_gain = 1536;
	}
	if(awb_output->update_gain){
		BLOCK_PARAM_CFG(pm_input, pm_data,
				ISP_PM_BLK_AWBC, ISP_BLK_AWB_NEW,
				&awbc_cfg,  sizeof(awbc_cfg));
		ret = isp_pm_ioctl(cxt->handle_pm,
				ISP_PM_CMD_SET_AWB, (void *)&pm_input, NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to set isp block param"));
	}

	if (awb_output->use_ccm) {
		memset(&pm_data, 0x0, sizeof(pm_data));
		BLOCK_PARAM_CFG(pm_input, pm_data,
				ISP_PM_BLK_CMC10, ISP_BLK_CMC10,
				awb_output->ccm, 9 * sizeof(cmr_u16));
		ret = isp_pm_ioctl(cxt->handle_pm,
				ISP_PM_CMD_SET_OTHERS, &pm_input, NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to set isp block param"));

	}

	if (awb_output->use_lsc) {
		switch (cxt->commn_cxt.image_pattern) {
		case SENSOR_IMAGE_PATTERN_RAWRGB_GR:
		{
			for (i = 0; i < awb_output->lsc_size / 8; i++) {
				lsc_param_tmp[0] = *((cmr_u16 *)awb_output->lsc + i * 4+ 0);
				lsc_param_tmp[1] = *((cmr_u16 *)awb_output->lsc + i * 4+ 1);
				lsc_param_tmp[2] = *((cmr_u16 *)awb_output->lsc + i * 4+ 2);
				lsc_param_tmp[3] = *((cmr_u16 *)awb_output->lsc + i * 4+ 3);
				*((cmr_u16 *)awb_output->lsc + i * 4+ 0) = lsc_param_tmp[3];
				*((cmr_u16 *)awb_output->lsc + i * 4+ 1) = lsc_param_tmp[2];
				*((cmr_u16 *)awb_output->lsc + i * 4+ 2) = lsc_param_tmp[1];
				*((cmr_u16 *)awb_output->lsc + i * 4+ 3) = lsc_param_tmp[0];
			}
			break;
		}
		case SENSOR_IMAGE_PATTERN_RAWRGB_R:
		{
			for (i = 0; i < awb_output->lsc_size / 8; i++) {
				lsc_param_tmp[0] = *((cmr_u16 *)awb_output->lsc + i * 4+ 0);
				lsc_param_tmp[1] = *((cmr_u16 *)awb_output->lsc + i * 4+ 1);
				lsc_param_tmp[2] = *((cmr_u16 *)awb_output->lsc + i * 4+ 2);
				lsc_param_tmp[3] = *((cmr_u16 *)awb_output->lsc + i * 4+ 3);
				*((cmr_u16 *)awb_output->lsc + i * 4+ 0) = lsc_param_tmp[2];
				*((cmr_u16 *)awb_output->lsc + i * 4+ 1) = lsc_param_tmp[3];
				*((cmr_u16 *)awb_output->lsc + i * 4+ 2) = lsc_param_tmp[0];
				*((cmr_u16 *)awb_output->lsc + i * 4+ 3) = lsc_param_tmp[1];
			}
			break;
		}
		case SENSOR_IMAGE_PATTERN_RAWRGB_B:
		{
			for (i = 0; i < awb_output->lsc_size / 8; i++) {
				lsc_param_tmp[0] = *((cmr_u16 *)awb_output->lsc + i * 4+ 0);
				lsc_param_tmp[1] = *((cmr_u16 *)awb_output->lsc + i * 4+ 1);
				lsc_param_tmp[2] = *((cmr_u16 *)awb_output->lsc + i * 4+ 2);
				lsc_param_tmp[3] = *((cmr_u16 *)awb_output->lsc + i * 4+ 3);
				*((cmr_u16 *)awb_output->lsc + i * 4+ 0) = lsc_param_tmp[1];
				*((cmr_u16 *)awb_output->lsc + i * 4+ 1) = lsc_param_tmp[0];
				*((cmr_u16 *)awb_output->lsc + i * 4+ 2) = lsc_param_tmp[3];
				*((cmr_u16 *)awb_output->lsc + i * 4+ 3) = lsc_param_tmp[2];
			}
			break;
		}
		case SENSOR_IMAGE_PATTERN_RAWRGB_GB:
			break;
		default:
			break;
		}

		BLOCK_PARAM_CFG(pm_input, pm_data,
				ISP_PM_BLK_LSC_MEM_ADDR,
				ISP_BLK_2D_LSC, awb_output->lsc,
				awb_output->lsc_size);
		ret = isp_pm_ioctl(cxt->handle_pm,
				ISP_PM_CMD_SET_OTHERS, &pm_input, NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to set isp block param"));

		cxt->awb_cxt.log_alc_lsc = awb_output->log_lsc.log;
		cxt->awb_cxt.log_alc_lsc_size = awb_output->log_lsc.size;
	}

	if (ISP_SUCCESS == ret) {
		cxt->awb_cxt.alc_awb = awb_output->use_ccm | (awb_output->use_lsc << 8);
	}

exit:
	ISP_LOGV("done %ld", ret);
	return ret;
}

cmr_int ispalg_start_awb_process(cmr_handle isp_alg_handle,
				struct ae_ctrl_callback_in *ae_in,
				struct awb_ctrl_calc_result * awb_output)
{
	cmr_int ret = ISP_SUCCESS;
	nsecs_t time_start = 0;
	nsecs_t time_end = 0;
	struct awb_ctrl_calc_param param;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct debuginfo_message awb_msg = { NULL, 0, 0, 0 };

	memset((void *)&param, 0, sizeof(param));

	ret = ispalg_awb_pre_process((cmr_handle) cxt, ae_in, &param);

	time_start = ispalg_get_sys_timestamp();
	if (cxt->ops.awb_ops.process) {
		ret = cxt->ops.awb_ops.process(cxt->awb_cxt.handle, &param, awb_output);
		ISP_TRACE_IF_FAIL(ret, ("fail to do awb process"));
	}
	time_end = ispalg_get_sys_timestamp();
	ISP_LOGV("SYSTEM_TEST-awb:%d ms", (cmr_u32)(time_end - time_start));

	if (cxt->ops.awb_ops.ioctrl) {
		ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle,
				AWB_CTRL_CMD_RESULT_INFO, (void *)awb_output, NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to AWB_CTRL_CMD_GET_GAIN"));
	}

	if (s_dbg_ver && (cxt->frame_id_sof > 0) && (cxt->fdr_cxt.fdr_start != FDR_STATUS_PROC)) {
		awb_msg.msg_log = awb_output->log_awb.log;
		awb_msg.msg_size = awb_output->log_awb.size;
		awb_msg.frame_id = cxt->frame_id_sof - 1;
		pthread_mutex_lock(&cxt->debuginfo_queue_lock);
		debuginfo_eq(&cxt->awb_queue, &awb_msg);
		pthread_mutex_unlock(&cxt->debuginfo_queue_lock);
	}

	ret = ispalg_awb_post_process((cmr_handle) cxt, awb_output);

exit:
	ISP_LOGV("done %ld", ret);
	return ret;
}

static cmr_int ispalg_aeawb_post_process(cmr_handle isp_alg_handle,
					 struct ae_ctrl_callback_in *ae_in,
					 struct awb_ctrl_calc_result *awb_output)
{
	cmr_int ret = ISP_SUCCESS;
	int i, num;
	nsecs_t time_start = 0;
	nsecs_t time_end = 0;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct smart_proc_input smart_proc_in;
	struct ae_monitor_info info;
	struct afctrl_ae_info *ae_info;
	struct afctrl_awb_info *awb_info;
	struct awb_size stat_img_size;
	struct awb_size win_size;

	memset(&info, 0, sizeof(info));
	memset(&smart_proc_in, 0, sizeof(smart_proc_in));
	CMR_MSG_INIT(message);

	time_start = ispalg_get_sys_timestamp();
	if (1 == cxt->smart_cxt.isp_smart_eb) {
		ISP_LOGV("bv:%d exp_line:%d again:%d cur_lum:%d target_lum:%d FlashEnvRatio:%f Flash1ofALLRatio:%f abl_weight %d",
			ae_in->ae_output.cur_bv,
			ae_in->ae_output.cur_exp_line,
			ae_in->ae_output.cur_again,
			ae_in->ae_output.cur_lum,
			ae_in->ae_output.target_lum,
			ae_in->flash_param.captureFlashEnvRatio,
			ae_in->flash_param.captureFlash1ofALLRatio,
			ae_in->ae_output.abl_weight);
		if (!cxt->smart_cxt.sw_bypass) {
			smart_proc_in.cal_para.bv = ae_in->ae_output.cur_bv;
			smart_proc_in.cal_para.bv_gain = ae_in->ae_output.cur_again;
			smart_proc_in.cal_para.flash_ratio = ae_in->flash_param.captureFlashEnvRatio * 256;
			smart_proc_in.cal_para.flash_ratio1 = ae_in->flash_param.captureFlash1ofALLRatio * 256;
			smart_proc_in.cal_para.ct = awb_output->ct;
			smart_proc_in.cal_para.abl_weight = ae_in->ae_output.abl_weight;
			smart_proc_in.cal_para.fps = ae_in->ae_output.fps;
			smart_proc_in.alc_awb = cxt->awb_cxt.alc_awb;
			if (cxt->remosaic_type == 1)
				smart_proc_in.mode_flag = cxt->commn_cxt.isp_pm_mode[1];
			smart_proc_in.scene_flag = cxt->commn_cxt.nr_scene_flag;
			smart_proc_in.ai_scene_id = cxt->commn_cxt.ai_scene_id;
			smart_proc_in.lock_nlm = cxt->smart_cxt.lock_nlm_en;
			smart_proc_in.lock_ee = cxt->smart_cxt.lock_ee_en;
			smart_proc_in.lock_precdn = cxt->smart_cxt.lock_precdn_en;
			smart_proc_in.lock_cdn = cxt->smart_cxt.lock_cdn_en;
			smart_proc_in.lock_postcdn = cxt->smart_cxt.lock_postcdn_en;
			smart_proc_in.lock_ccnr = cxt->smart_cxt.lock_ccnr_en;
			smart_proc_in.lock_ynr = cxt->smart_cxt.lock_ynr_en;
			smart_proc_in.ai_scene_pro_flag = cxt->is_ai_scene_pro;
			isp_prepare_atm_param(isp_alg_handle, &smart_proc_in);

			num = (cxt->zsl_flag) ? 2 : 1;
			num = (cxt->fdr_cxt.fdr_start == FDR_STATUS_CAPTURE) ? 3 : num;
			cxt->fdr_cxt.smart_in = 0;
			for (i = 0; i < num; i++) {
				/* skip normal capture if FDR start */
				if (cxt->fdr_cxt.fdr_start && i == 1)
					continue;
				cxt->smart_cxt.cur_set_id = i;
				smart_proc_in.cal_para.gamma_tab = cxt->smart_cxt.tunning_gamma_cur[i];
				smart_proc_in.mode_flag = cxt->commn_cxt.isp_pm_mode[i];

				if (cxt->save_data && (i == 2) && cxt->fdr_cxt.fdr_start) {
					cxt->fdr_cxt.smart_in = 1;
					ISP_LOGD("smart in for fdr\n");
					memcpy(&cxt->fdr_cxt.smart_proc_in, &smart_proc_in, sizeof(struct smart_proc_input));
				}

				if (cxt->ops.smart_ops.ioctrl) {
					ret = cxt->ops.smart_ops.ioctrl(cxt->smart_cxt.handle,
								ISP_SMART_IOCTL_SET_WORK_MODE,
								(void *)&smart_proc_in.mode_flag, NULL);
					ISP_TRACE_IF_FAIL(ret, ("fail to ISP_SMART_IOCTL_SET_WORK_MODE"));
				}
				if (cxt->ops.smart_ops.calc) {
					ret = cxt->ops.smart_ops.calc(cxt->smart_cxt.handle, &smart_proc_in);
					ISP_TRACE_IF_FAIL(ret, ("fail to do _smart_calc"));
				}
			}
			cxt->fdr_cxt.smart_in = 0;

			cxt->curr_bv = ae_in->ae_output.cur_bv;
			cxt->abl_weight = ae_in->ae_output.abl_weight;
			cxt->smart_cxt.log_smart = smart_proc_in.log;
			cxt->smart_cxt.log_smart_size = smart_proc_in.size;

			if (s_dbg_ver && (cxt->fdr_cxt.fdr_start != FDR_STATUS_PROC)) {
				struct debuginfo_message smart_msg = { NULL, 0, 0, 0 };
				smart_msg.msg_log = smart_proc_in.log;
				smart_msg.msg_size = smart_proc_in.size;
				smart_msg.frame_id = cxt->frame_id_sof;

				pthread_mutex_lock(&cxt->debuginfo_queue_lock);
				debuginfo_eq(&cxt->smart_queue, &smart_msg);
				pthread_mutex_unlock(&cxt->debuginfo_queue_lock);
			}
		}

		if (!cxt->lsc_cxt.sw_bypass && cxt->lsc_cxt.use_lscm) {
			stat_img_size.w = cxt->lsc_cxt.lscm_win_num.w;
			stat_img_size.h = cxt->lsc_cxt.lscm_win_num.h;
			win_size.w = cxt->lsc_cxt.lscm_win_num.w;
			win_size.h = cxt->lsc_cxt.lscm_win_num.h;
			ret = ispalg_alsc_calc(isp_alg_handle,
					       cxt->lscm_stats_data.r_info,
					       cxt->lscm_stats_data.g_info,
					       cxt->lscm_stats_data.b_info,
					       &stat_img_size, &win_size,
					       cxt->commn_cxt.prv_size.w, cxt->commn_cxt.prv_size.h,
					       awb_output->ct, awb_output->gain.r, awb_output->gain.b,
					       ae_in->ae_output.is_stab, ae_in);
			ISP_TRACE_IF_FAIL(ret, ("fail to do alsc_calc"));
		} else if (!cxt->lsc_cxt.sw_bypass && !cxt->lsc_cxt.use_lscm) {
			stat_img_size.w = ae_in->monitor_info.win_num.w;
			stat_img_size.h = ae_in->monitor_info.win_num.h;
			win_size.w = ae_in->monitor_info.win_num.w;
			win_size.h = ae_in->monitor_info.win_num.h;
			ret = ispalg_alsc_calc(isp_alg_handle,
					       cxt->aem_stats_data.r_info,
					       cxt->aem_stats_data.g_info,
					       cxt->aem_stats_data.b_info,
					       &stat_img_size, &win_size,
					       cxt->commn_cxt.prv_size.w, cxt->commn_cxt.prv_size.h,
					       awb_output->ct, awb_output->gain.r, awb_output->gain.b,
					       ae_in->ae_output.is_stab, ae_in);
			ISP_TRACE_IF_FAIL(ret, ("fail to do alsc_calc"));
		}
	}
	time_end = ispalg_get_sys_timestamp();
	ISP_LOGV("SYSTEM_TEST-smart:%d ms", (cmr_u32)(time_end - time_start));

	isp_cur_bv = ae_in->ae_output.cur_bv;
	isp_cur_ct = awb_output->ct;
	isp_abl_weight = ae_in->ae_output.abl_weight;

	ae_info = &cxt->ae_info;
	awb_info = &cxt->awb_info;

	if (0 == ae_info->is_update) {
		memset((void *)ae_info, 0, sizeof(struct afctrl_ae_info));
		ae_info->img_blk_info.block_w = cxt->ae_cxt.win_num.w;
		ae_info->img_blk_info.block_h = cxt->ae_cxt.win_num.h;
		ae_info->img_blk_info.chn_num = 3;
		ae_info->img_blk_info.pix_per_blk = 1;
		ae_info->img_blk_info.data = (cmr_u32 *) &cxt->aem_stats_data;
		ae_info->ae_rlt_info.bv = ae_in->ae_output.cur_bv;
		ae_info->ae_rlt_info.abl_weight = ae_in->ae_output.abl_weight;
		ae_info->ae_rlt_info.is_stab = ae_in->ae_output.is_stab;
		ae_info->ae_rlt_info.near_stab = ae_in->ae_output.near_stab;
		ae_info->ae_rlt_info.cur_exp_line = ae_in->ae_output.cur_exp_line;
		ae_info->ae_rlt_info.cur_dummy = ae_in->ae_output.cur_dummy;
		ae_info->ae_rlt_info.frame_line = ae_in->ae_output.frame_line;
		ae_info->ae_rlt_info.line_time = ae_in->ae_output.line_time;
		ae_info->ae_rlt_info.cur_again = ae_in->ae_output.cur_again;
		ae_info->ae_rlt_info.cur_dgain = ae_in->ae_output.cur_dgain;
		ae_info->ae_rlt_info.cur_lum = ae_in->ae_output.cur_lum;
		ae_info->ae_rlt_info.target_lum = ae_in->ae_output.target_lum;
		ae_info->ae_rlt_info.target_lum_ori = ae_in->ae_output.target_lum_ori;
		ae_info->ae_rlt_info.flag4idx = ae_in->ae_output.flag4idx;
		ae_info->ae_rlt_info.face_stable= ae_in->ae_output.face_stable;
		ae_info->ae_rlt_info.face_ae_enable = ae_in->ae_output.face_enable;
		ae_info->ae_rlt_info.cur_ev = ae_in->ae_output.cur_ev;
		ae_info->ae_rlt_info.cur_index = ae_in->ae_output.cur_index;
		ae_info->ae_rlt_info.cur_iso = ae_in->ae_output.cur_iso;
		ae_info->ae_rlt_info.cur_fps = ae_in->ae_output.fps;
		ae_info->is_update = 1;
	}

	if (0 == awb_info->is_update) {
		memset((void *)awb_info, 0, sizeof(struct afctrl_awb_info));
		awb_info->r_gain = awb_output->gain.r;
		awb_info->g_gain = awb_output->gain.g;
		awb_info->b_gain = awb_output->gain.b;
		awb_info->is_update = 1;
	}
	message.msg_type = ISP_EVT_AF;
	message.sub_msg_type = AF_DATA_IMG_BLK;
	message.sync_flag = CMR_MSG_SYNC_NONE;
	ret = cmr_thread_msg_send(cxt->thr_afhandle, &message);
	if (ret) {
		ISP_LOGE("fail to send evt af, ret %ld", ret);
		free(message.data);
	}

exit:
	ISP_LOGV("done ret %ld", ret);
	return ret;
}

cmr_int ispalg_ae_process(cmr_handle isp_alg_handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;

	if (cxt->ae_cxt.sw_bypass) {
		return ret;
	}
	if (!cxt->aem_is_update) {
		ISP_LOGV("aem is not update\n");
		return ret;
	}

	ret = ispalg_start_ae_process((cmr_handle) cxt);

	return ret;
}

static cmr_int ispalg_awb_process(cmr_handle isp_alg_handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct awb_ctrl_calc_result awb_output;
	struct ae_calc_results ae_result;
	struct ae_ctrl_callback_in ae_ctrl_calc_result;

	if (cxt->awb_cxt.sw_bypass)
		return ret;

	if (!cxt->aem_is_update) {
		ISP_LOGV("aem is not update\n");
		goto exit;
	}

	memset(&awb_output, 0, sizeof(awb_output));
	memset(&ae_result, 0, sizeof(ae_result));
	memset(&ae_ctrl_calc_result, 0, sizeof(ae_ctrl_calc_result));

	if (cxt->ops.ae_ops.ioctrl) {
		cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_GET_CALC_RESULTS, NULL, &ae_result);
		ae_ctrl_calc_result.is_skip_cur_frame = ae_result.is_skip_cur_frame;
		ae_ctrl_calc_result.ae_output = ae_result.ae_output;
		//ae_ctrl_calc_result.ae_result = ae_result.ae_result;
		ae_ctrl_calc_result.ae_ev = ae_result.ae_ev;
		ae_ctrl_calc_result.monitor_info = ae_result.monitor_info;
		ae_ctrl_calc_result.flash_param.captureFlashEnvRatio = ae_result.flash_param.captureFlashEnvRatio;
		ae_ctrl_calc_result.flash_param.captureFlash1ofALLRatio = ae_result.flash_param.captureFlash1ofALLRatio;
		cxt->ae_info.ae_rlt_info.is_stab = ae_result.ae_output.is_stab;
	}

	ret = ispalg_start_awb_process((cmr_handle) cxt, &ae_ctrl_calc_result, &awb_output);
	if (ret) {
		ISP_LOGE("fail to start awb process");
		goto exit;
	}

	ret = ispalg_aeawb_post_process((cmr_handle) cxt, &ae_ctrl_calc_result, &awb_output);
	if (ret) {
		ISP_LOGE("fail to proc aeawb ");
		goto exit;
	}
exit:
	return ret;
}

cmr_int ispalg_afl_process(cmr_handle isp_alg_handle, void *data)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_u32 cur_flicker = 0;
	cmr_u32 cur_exp_flag = 0;
	cmr_s32 ae_exp_flag = 0;
	cmr_u32 app_mode = 0;
	float ae_exp = 0.0;
	struct isp_statis_info *statis_info = (struct isp_statis_info *)data;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct afl_proc_in afl_input;
	struct afl_ctrl_proc_out afl_output = {0, 0, 0};
	struct afl_ctrl_proc_out afl_info = {0, 0, 0};

	memset(&afl_input, 0, sizeof(afl_input));
	memset(&afl_output, 0, sizeof(afl_output));
	cxt->afl_cxt.hw_bypass = 1;

	if (cxt->afl_cxt.sw_bypass) {
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_STSTIS_BUF, statis_info, NULL);
		if (ret) {
			ISP_LOGE("fail to set statis buf");
		}
		cxt->afl_cxt.hw_bypass = 0;
		ret = isp_dev_access_ioctl(cxt->dev_access_handle,
					ISP_DEV_SET_AFL_NEW_BYPASS,
					&cxt->afl_cxt.hw_bypass, NULL);
		goto exit;
	}


	if (cxt->afl_cxt.afl_mode <= AE_FLICKER_60HZ) {
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_STSTIS_BUF, statis_info, NULL);
		if (ret) {
			ISP_LOGE("fail to set statis buf");
		}
		ISP_LOGE("new afl_mode %d,  bypass %d\n", cxt->afl_cxt.afl_mode, cxt->afl_cxt.hw_bypass);
		goto exit;
	}


	if (cxt->ops.ae_ops.ioctrl) {
		ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_GET_FLICKER_MODE, NULL, &cur_flicker);
		ISP_TRACE_IF_FAIL(ret, ("fail to AE_GET_FLICKER_MODE"));
		ISP_LOGV("cur flicker mode %d", cur_flicker);

		ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_GET_EXP, NULL, &ae_exp);
		ISP_TRACE_IF_FAIL(ret, ("fail to AE_GET_EXP"));

		if ((fabs(ae_exp - 0.04) < 0.000001 || ae_exp > 0.04) && (cxt->curr_bv <= 360)) {
			ae_exp_flag = 1;
		}
		ISP_LOGV("ae_exp %f; ae_exp_flag %d", ae_exp, ae_exp_flag);

		ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_GET_FLICKER_SWITCH_FLAG, &cur_exp_flag, NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to AE_GET_FLICKER_SWITCH_FLAG"));
		ISP_LOGV("cur exposure flag %d", cur_exp_flag);

		ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_GET_APP_MODE, NULL, &app_mode);
		ISP_TRACE_IF_FAIL(ret, ("fail to AE_GET_APP_MODE"));
		ISP_LOGV("app_mode %d", app_mode);

	}

	if (cxt->ops.afl_ops.ioctrl) {
		ret = cxt->ops.afl_ops.ioctrl(cxt->afl_cxt.handle, AFL_GET_INFO, (void *)&afl_info, NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to AFL_GET_INFO"));
	}

	ret = isp_dev_access_ioctl(cxt->dev_access_handle,
			ISP_DEV_INVALIDATE_STSTIS_BUFCACHE, statis_info, NULL);

	cxt->afl_cxt.afl_statis_info = *statis_info;
	afl_input.ae_stat_ptr = &cxt->aem_stats_data;
	afl_input.ae_exp_flag = ae_exp_flag;
	afl_input.cur_exp_flag = cur_exp_flag;
	afl_input.cur_flicker = cur_flicker;
	afl_input.vir_addr = statis_info->uaddr;
	afl_input.vir_addr_region = statis_info->uaddr + AFL_RBUF_OFFSET;
	afl_input.afl_mode = cxt->afl_cxt.afl_mode;
	afl_input.private_len = sizeof(struct isp_statis_info);
	afl_input.private_data = &cxt->afl_cxt.afl_statis_info;
	afl_input.ae_win_num.w = cxt->ae_cxt.win_num.w;
	afl_input.ae_win_num.h = cxt->ae_cxt.win_num.h;
	afl_input.max_fps = afl_info.max_fps;
	afl_input.app_mode = app_mode;
	ISP_LOGV("afl_mode %d\n",  cxt->afl_cxt.afl_mode);

	if (cxt->ops.afl_ops.process) {
		ret = cxt->ops.afl_ops.process(cxt->afl_cxt.handle, &afl_input, &afl_output);
		ISP_TRACE_IF_FAIL(ret, ("fail to afl process"));
	}

exit:
	ISP_LOGV("done ret %ld", ret);
	return ret;
}

static cmr_int ispalg_af_process(cmr_handle isp_alg_handle, cmr_u32 data_type, void *in_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct afctrl_calc_in calc_param;
	struct afctrl_calc_out calc_result;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;

	memset((void *)&calc_param, 0, sizeof(calc_param));
	memset((void *)&calc_result, 0, sizeof(calc_result));
	ISP_LOGV("begin data_type %d", data_type);

	switch (data_type) {
	case AF_DATA_AFM_STAT:
	case AF_DATA_AF: {
			cmr_u32 i = 0;
			cmr_u32 blk_num;
			cmr_u32 af_temp[15*20][3];
			cmr_u32 *ptr, *dst;
			cmr_u32 *zoom_ratio;
			struct isp_statis_info *statis_info = (struct isp_statis_info *)in_ptr;

			ret = isp_dev_access_ioctl(cxt->dev_access_handle,
					ISP_DEV_INVALIDATE_STSTIS_BUFCACHE, statis_info, NULL);

			ptr = (cmr_u32 *)statis_info->uaddr;
			dst = &af_temp[0][0];
			blk_num = cxt->af_cxt.win_num.x * cxt->af_cxt.win_num.y;
#ifndef CONFIG_ISP_2_5
			for (i = 0; i < blk_num; i++) {
				af_temp[i][0] = *ptr++;
				af_temp[i][1] = *ptr++;
				af_temp[i][2] = *ptr++;
				ptr++;
			}
#else
			for (i = 0; i < sizeof(af_temp)/sizeof(af_temp[0][0]); i++) {
				*dst++ = *ptr++;
			}
#endif

			ISP_LOGV("data: %x %x %x %x %x %x\n",
				af_temp[0][0], af_temp[0][1], af_temp[0][2], af_temp[1][0], af_temp[1][1], af_temp[1][2]);
			ISP_LOGV("data: %x %x %x %x %x %x\n",
				af_temp[16][0], af_temp[16][1], af_temp[16][2], af_temp[17][0], af_temp[17][1], af_temp[17][2]);

			calc_param.data_type = AF_DATA_AF;
			calc_param.data = (void *)(af_temp);
			if (cxt->ops.af_ops.process && !cxt->af_cxt.sw_bypass) {
				ret = cxt->ops.af_ops.process(cxt->af_cxt.handle, (void *)&calc_param, &calc_result);
				ISP_TRACE_IF_FAIL(ret, ("fail to af process"));
			}

			/* send data to pd adpt layer */
			struct pdaf_ctrl_param_in pdaf_in;
			pdaf_in.af_addr = (void *)(af_temp);
			pdaf_in.af_addr_len = sizeof(af_temp);
			if (cxt->ops.pdaf_ops.process && !cxt->pdaf_cxt.sw_bypass) {
				ret = cxt->ops.pdaf_ops.ioctrl(cxt->pdaf_cxt.handle, PDAF_CTRL_CMD_SET_AFMFV, (void *)&pdaf_in, NULL);
			}

			/* set zoom ratio to af ctrl */
			zoom_ratio = &statis_info->zoom_ratio;
			cxt->cur_ratio = *(zoom_ratio);
			if (cxt->last_ratio != cxt->cur_ratio) {
				if (cxt->ops.af_ops.ioctrl) {
					ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle, AF_CMD_SET_ZOOM_RATIO, (void *)zoom_ratio, NULL);
				}
				cxt->last_ratio = cxt->cur_ratio;
			}

			ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_STSTIS_BUF, statis_info, NULL);
			if (ret) {
				ISP_LOGE("fail to set statis buf");
			}
			break;
		}
	case AF_DATA_IMG_BLK:
		if (cxt->af_cxt.sw_bypass)
			break;

		if (cxt->ops.af_ops.ioctrl) {
			struct afctrl_ae_info *ae_info = &cxt->ae_info;
			struct afctrl_awb_info *awb_info = &cxt->awb_info;
			if (1 == ae_info->is_update) {
				ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle, AF_CMD_SET_AE_INFO, (void *)ae_info, NULL);
				ISP_TRACE_IF_FAIL(ret, ("fail to AF_CMD_SET_AE_INFO"));
				ae_info->is_update = 0;
			}

			if (awb_info->is_update) {
				ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle, AF_CMD_SET_AWB_INFO, (void *)awb_info, NULL);
				ISP_TRACE_IF_FAIL(ret, ("fail to AF_CMD_SET_AWB_INFO"));
				awb_info->is_update = 0;
			}
		}
		calc_param.data_type = AF_DATA_IMG_BLK;
		if (cxt->ops.af_ops.process) {
			ret = cxt->ops.af_ops.process(cxt->af_cxt.handle, (void *)&calc_param, (void *)&calc_result);
		}
		break;
	case AF_DATA_AE:
		break;
	case AF_DATA_FD:
		break;
	default:
		break;
	}

	ISP_LOGV("done %ld", ret);
	return ret;
}

static cmr_int ispalg_pdaf_process(cmr_handle isp_alg_handle, cmr_u32 data_type, void *in_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_uint u_addr = 0;
	cmr_u32 offset;
	void *pdaf_info = NULL;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_statis_info *statis_info = (struct isp_statis_info *)in_ptr;
	struct pdaf_ctrl_process_in pdaf_param_in;
	struct pdaf_ctrl_param_out pdaf_param_out;
	UNUSED(data_type);

	ret = isp_dev_access_ioctl(cxt->dev_access_handle,
			ISP_DEV_INVALIDATE_STSTIS_BUFCACHE, statis_info, NULL);

	memset((void *)&pdaf_param_in, 0x00, sizeof(pdaf_param_in));
	memset((void *)&pdaf_param_out, 0x00, sizeof(pdaf_param_out));

	offset = statis_info->buf_size / 2;
	u_addr = statis_info->uaddr;
	pdaf_param_in.u_addr = statis_info->uaddr;
	pdaf_param_in.u_addr_right = statis_info->uaddr + offset;
	ISP_LOGV("addr 0x%lx, addr1 0x%lx, offset %x\n", statis_info->uaddr, pdaf_param_in.u_addr_right, offset);

	switch (cxt->pdaf_cxt.pdaf_support) {
	case SENSOR_PDAF_TYPE1_ENABLE:
		pdaf_info = (cmr_s32 *)(u_addr);

		if (cxt->ops.af_ops.ioctrl) {
			ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle, AF_CMD_SET_TYPE1_PD_INFO, pdaf_info, NULL);
			ISP_TRACE_IF_FAIL(ret, ("fail to AF_CMD_SET_TYPE1_PD_INFO"));
		}
		break;
	case SENSOR_PDAF_TYPE2_ENABLE:
	case SENSOR_PDAF_TYPE3_ENABLE:
	case SENSOR_DUAL_PDAF_ENABLE:
		if (cxt->ops.pdaf_ops.ioctrl)
			cxt->ops.pdaf_ops.ioctrl(cxt->pdaf_cxt.handle, PDAF_CTRL_CMD_GET_BUSY, NULL, &pdaf_param_out);
		ISP_LOGV("pdaf_is_busy=%d\n", pdaf_param_out.is_busy);
		if (!pdaf_param_out.is_busy && !cxt->pdaf_cxt.sw_bypass) {
			if (cxt->ops.pdaf_ops.process)
				ret = cxt->ops.pdaf_ops.process(cxt->pdaf_cxt.handle, &pdaf_param_in, NULL);
		}
		break;
	default:
		ISP_LOGV("Invalid pdaf param:%d", cxt->pdaf_cxt.pdaf_support);
	}

	ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_STSTIS_BUF, statis_info, NULL);

	ISP_LOGV("done %ld, statis_info->uaddr = 0x%lx", ret, statis_info->uaddr);
	return ret;
}

static cmr_int ispalg_ebd_process(cmr_handle isp_alg_handle, cmr_u32 data_type, void *in_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_statis_info *statis_info = (struct isp_statis_info *)in_ptr;
	struct sensor_embedded_info sensor_ebd_info;
	UNUSED(data_type);

	ret = isp_dev_access_ioctl(cxt->dev_access_handle,
			ISP_DEV_INVALIDATE_STSTIS_BUFCACHE, statis_info, NULL);

	memset((void *)&sensor_ebd_info, 0x00, sizeof(sensor_ebd_info));
	sensor_ebd_info.embedded_data = (cmr_u8 *)statis_info->uaddr;
	if (cxt->ioctrl_ptr->sns_ioctl) {
		cxt->ioctrl_ptr->sns_ioctl(cxt->ioctrl_ptr->caller_handler,
					CMD_SNS_IC_GET_EBD_PARSE_DATA,
					&sensor_ebd_info);
	}

	cxt->ae_cxt.ebd_info.frame_id = sensor_ebd_info.parse_data.frame_count;
	cxt->ae_cxt.ebd_info.frame_id_valid =
				sensor_ebd_info.frame_count_valid;
	cxt->ae_cxt.ebd_info.exposure = sensor_ebd_info.parse_data.shutter;
	cxt->ae_cxt.ebd_info.exposure_valid =
				sensor_ebd_info.shutter_valid;
	cxt->ae_cxt.ebd_info.again = sensor_ebd_info.parse_data.again;
	cxt->ae_cxt.ebd_info.again_valid =
				sensor_ebd_info.again_valid;

	cxt->ae_cxt.ebd_info.dgain_gr = sensor_ebd_info.parse_data.dgain_gr;
	cxt->ae_cxt.ebd_info.dgain_r = sensor_ebd_info.parse_data.dgain_r;
	cxt->ae_cxt.ebd_info.dgain_b = sensor_ebd_info.parse_data.dgain_b;
	cxt->ae_cxt.ebd_info.dgain_gb = sensor_ebd_info.parse_data.dgain_gb;
	cxt->ae_cxt.ebd_info.gain = sensor_ebd_info.parse_data.gain;
	cxt->ae_cxt.ebd_info.dgain_valid = sensor_ebd_info.dgain_valid;

	ISP_LOGV("frame id %x %d, shutter %x %d, again %x %d",
			cxt->ae_cxt.ebd_info.frame_id,
			cxt->ae_cxt.ebd_info.frame_id_valid,
			cxt->ae_cxt.ebd_info.exposure,
			cxt->ae_cxt.ebd_info.exposure_valid,
			cxt->ae_cxt.ebd_info.again,
			cxt->ae_cxt.ebd_info.again_valid);

	ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_STSTIS_BUF, statis_info, NULL);

	return ret;
}

cmr_int ispalg_ai_process(cmr_handle isp_alg_handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	enum ai_status ai_sta = AI_STATUS_MAX;

	ISP_CHECK_HANDLE_VALID(isp_alg_handle);

	if (cxt->ops.ai_ops.ioctrl) {
		ret = cxt->ops.ai_ops.ioctrl(cxt->ai_cxt.handle, AI_GET_STATUS, (void *)(&ai_sta), NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to AI_GET_STATUS"));
	}
	if (AI_STATUS_PROCESSING != ai_sta) {
		ISP_LOGV("AI detection doesn't work.");
		return ret;
	}

	cxt->ai_cxt.ae_param.timestamp = (cmr_u64)(cxt->ai_cxt.ae_param.sec) * 1000000000 +
		(cmr_u64)(cxt->ai_cxt.ae_param.usec) * 1000;

	cxt->ai_cxt.ae_param.ae_stat.r_info = cxt->aem_stats_data.r_info;
	cxt->ai_cxt.ae_param.ae_stat.g_info = cxt->aem_stats_data.g_info;
	cxt->ai_cxt.ae_param.ae_stat.b_info = cxt->aem_stats_data.b_info;
	cxt->ai_cxt.ae_param.stable = cxt->ae_info.ae_rlt_info.is_stab;

	cxt->ai_cxt.ae_param.blk_num_hor = cxt->ae_cxt.win_num.w;
	cxt->ai_cxt.ae_param.blk_num_ver = cxt->ae_cxt.win_num.h;
	cxt->ai_cxt.ae_param.curr_bv = cxt->curr_bv;
	cxt->ai_cxt.ae_param.app_mode= cxt->app_mode;
	ISP_LOGV("ai ae info: blk_num_hor: %d, blk_num_ver: %d.",
		cxt->ai_cxt.ae_param.blk_num_hor, cxt->ai_cxt.ae_param.blk_num_ver);
	ISP_LOGD("ai ae info: frame_id: %d, timestamp: %llu,%d.",
		cxt->ai_cxt.ae_param.frame_id, (unsigned long long)cxt->ai_cxt.ae_param.timestamp,cxt->ai_cxt.ae_param.app_mode);

	if (cxt->ops.ai_ops.ioctrl) {
		ret = cxt->ops.ai_ops.ioctrl(cxt->ai_cxt.handle, AI_SET_AE_PARAM, (void *)(&cxt->ai_cxt.ae_param), NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to AI_SET_AE_PARAM"));
	}

	return ret;
}

static cmr_int ispalg_evt_process_cb(cmr_handle isp_alg_handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct ips_out_param callback_param = { 0x00 };

	ISP_LOGV("isp start raw proc callback\n");
	if (NULL != cxt->commn_cxt.callback) {
		callback_param.output_height = cxt->commn_cxt.src.h;
		ISP_LOGV("callback ISP_PROC_CALLBACK");
		cxt->commn_cxt.callback(cxt->commn_cxt.caller_id,
					ISP_CALLBACK_EVT | ISP_PROC_CALLBACK,
					(void *)&callback_param,
					sizeof(struct ips_out_param));
	}

	ISP_LOGV("isp end raw proc callback\n");
	return ret;
}

cmr_int ispalg_afthread_proc(struct cmr_msg *message, void *p_data)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)p_data;

	if (!message || !p_data || cxt->fw_started == 0) {
		ISP_LOGE("fail to check input param ");
		goto exit;
	}
	ISP_LOGV("message.msg_type 0x%x, data %p", message->msg_type, message->data);

	switch (message->msg_type) {
	case ISP_EVT_AF:
		ret = ispalg_af_process((cmr_handle) cxt, message->sub_msg_type, message->data);
		break;
	default:
		ISP_LOGV("don't support msg");
		break;
	}
exit:
	ISP_LOGV("done %ld", ret);
	return ret;
}

cmr_int ispalg_aethread_proc(struct cmr_msg *message, void *p_data)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_u32 is_raw_capture = 0;
	cmr_u8 tool_eb = 0;
	char value[PROPERTY_VALUE_MAX];
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)p_data;
	CMR_MSG_INIT(message1);

	if (!message || !p_data) {
		ISP_LOGE("fail to check input param ");
		goto exit;
	}
	ISP_LOGV("message.msg_type 0x%x, data %p", message->msg_type, message->data);

	/* set priority to -10 */
	/* same as grab/aectrl to make sure whole AE process priority */
	if (cxt->aethd_pri_set == 0) {
		cmr_s32 priority = -10;

		setpriority(PRIO_PROCESS, 0, priority);
		/* todo: set_sched_policy() applied in Android P, Android Q needs new API for it */
		//set_sched_policy(0, SP_FOREGROUND);
		cxt->aethd_pri_set = 1;
		ISP_LOGI("set priority to %d", priority);
	}

	switch (message->msg_type) {
	case ISP_EVT_AE: {
		struct isp_statis_info *statis_info = (struct isp_statis_info *)message->data;

		if (cxt->fw_started == 0 && cxt->takepicture_mode != CAMERA_ISP_SIMULATION_MODE)
			break;

		ISP_LOGV("aem no.%d, timestamp %03d.%06d\n", statis_info->frame_id, statis_info->sec, statis_info->usec);

		ret = ispalg_aem_stats_parser((cmr_handle) cxt, message->data);
		struct isp_awb_statistic_info *ae_stat_ptr = (struct isp_awb_statistic_info *)&cxt->aem_stats_data;
		if (cxt->is_multi_mode)
			ISP_LOGV("is_master :%d\n", cxt->is_master);
		isp_br_ioctrl(cxt->sensor_role, SET_STAT_AWB_DATA, ae_stat_ptr, NULL);
		ret = ispalg_ai_process((cmr_handle)cxt);
		if (cxt->takepicture_mode == CAMERA_ISP_SIMULATION_MODE) {
			ret = ispalg_handle_sensor_sof((cmr_handle) cxt);
		}
		ISP_LOGE("takepicture_mode[%d] camera_id[%d] cxt[%p]\n",
			cxt->takepicture_mode,
			(unsigned int)cxt->camera_id,cxt);
		break;
	}
	case ISP_EVT_SOF: {
		struct sprd_irq_info *irq_info = (struct sprd_irq_info *)message->data;
		cxt->frame_id_sof = irq_info->frame_id;
		ISP_LOGV("sof no.%d, timestamp %03d.%06d\n", irq_info->frame_id, irq_info->sec, irq_info->usec);

		if (cxt->fw_started == 0) {
			/* workaround for raw capture with flash
			  * because no statis data for raw, force AE process running to return flash messege to HAL
			  */
			property_get("persist.vendor.cam.raw.mode", value, "jpeg");
			if (!strcmp(value, "raw"))
				is_raw_capture = 1;
			if (is_raw_capture == 0)
				break;
			ISP_LOGD("force AE process for raw capture\n");
			cxt->aem_is_update = 1;
			ret = ispalg_start_ae_process((cmr_handle) cxt);
			break;
		}

		property_get("persist.vendor.cam.isptool.mode.enable", value, "false");
		if (!strcmp(value, "true"))
			tool_eb = 1;

		/* 4in1 full size capture(non-zsl), awb don't need work */
		if (((cxt->is_high_res_mode == 1) && (cxt->work_mode == 1) && (cxt->zsl_flag == 0))
			|| ((tool_eb == 1) && (cxt->work_mode == 1))) {
			cxt->aem_is_update = 1;
			ret = ispalg_ae_process((cmr_handle)cxt);
			ISP_LOGD("high res high light capture.\n");
		} else {
			ret = ispalg_ae_process((cmr_handle) cxt);
			ret |= ispalg_awb_process((cmr_handle) cxt);
		}
		if (ret)
			ISP_LOGE("fail to start ae or awb process");

		cxt->aem_is_update = 0;

		ret = ispalg_handle_sensor_sof((cmr_handle) cxt);

		message1.msg_type = ISP_EVT_CFG;
		message1.sub_msg_type = PARAM_CFG_DEFAULT;
		message1.sync_flag = CMR_MSG_SYNC_NONE;
		ret = cmr_thread_msg_send(cxt->thr_handle, &message1);
		break;
	}
	default:
		ISP_LOGV("don't support msg");
		break;
	}
exit:
	ISP_LOGV("done %ld", ret);
	return ret;
}

cmr_int ispalg_thread_proc(struct cmr_msg *message, void *p_data)
{
	cmr_int ret = ISP_SUCCESS;
	nsecs_t cur_time;
	cmr_u32 timems_diff;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)p_data;

	if (!message || !p_data) {
		ISP_LOGE("fail to check input param ");
		goto exit;
	}

	if((cxt->fw_started == 0) && (message->msg_type != ISP_EVT_TX)
		&& (message->msg_type != ISP_EVT_PARAM)) {
		ISP_LOGD("fw stopped, drop stats event 0x%x\n", message->msg_type);
		goto exit;
	}

	ISP_LOGV("message.msg_type 0x%x, data %p", message->msg_type, message->data);

	switch (message->msg_type) {
	case ISP_EVT_TX:
		ret = ispalg_evt_process_cb((cmr_handle) cxt);
		break;
	case ISP_EVT_AFL:
		ret = ispalg_afl_process((cmr_handle) cxt, message->data);
		break;
	case ISP_EVT_PDAF:
		ret = ispalg_pdaf_process((cmr_handle) cxt, message->sub_msg_type, message->data);
		break;
	case ISP_EVT_EBD:
		ret = ispalg_ebd_process((cmr_handle) cxt, message->sub_msg_type, message->data);
		break;
	case ISP_EVT_HIST:
		ret = ispalg_bayerhist_stats_parser((cmr_handle) cxt, message->data);
		break;
	case ISP_EVT_HIST2:
		ret = ispalg_hist2_process((cmr_handle) cxt, message->data);
		break;
	case ISP_EVT_3DNR:
		ret = ispalg_3dnr_statis_parser((cmr_handle) cxt, message->data);
		break;
	case ISP_EVT_CFG:
		if (message->sub_msg_type != PARAM_CFG_DEFAULT) {
			ret = ispalg_cfg_param(cxt, message->sub_msg_type);
			break;
		}
		cur_time = ispalg_get_sys_timestamp();
		timems_diff = (cmr_u32)(cur_time - cxt->last_sof_time);
		if (timems_diff < MIN_FRAME_INTERVAL_MS) {
			ISP_LOGW("time interval is too small: %d\n", timems_diff);
		}
		cxt->last_sof_time = cur_time;
		ret = ispalg_cfg_param(cxt, message->sub_msg_type);
		break;
	case ISP_EVT_LSC:
		ret = ispalg_lscm_stats_parser((cmr_handle) cxt, message->data);
		break;
	case ISP_EVT_PARAM:
		ret = ispalg_param_parser((cmr_handle) cxt, message->data);
		break;
	default:
		ISP_LOGV("don't support msg");
		break;
	}
exit:
	ISP_LOGV("done %ld", ret);
	return ret;
}

void ispalg_dev_evt_msg(cmr_int evt, void *data, void *privdata)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)privdata;
	CMR_MSG_INIT(message);

	if (!cxt) {
		ISP_LOGE("fail to check input param");
		return;
	}

	if (evt == ISP_EVT_SOF && data) {
		struct sprd_irq_info *irq_info = (struct sprd_irq_info *)data;;
		ISP_LOGV("sof no.%d, timestamp %03d.%06d\n",
			irq_info->frame_id, irq_info->sec, irq_info->usec);
	}
	if (evt == ISP_EVT_AE && data) {
		struct isp_statis_info *statis_info = (struct isp_statis_info *)data;
		ISP_LOGV("aem no.%d, timestamp %03d.%06d\n",
			statis_info->frame_id, statis_info->sec, statis_info->usec);
	}

	message.msg_type = evt;
	message.sub_msg_type = 0;
	message.sync_flag = CMR_MSG_SYNC_NONE;
	message.alloc_flag = 1;
	message.data = data;
	if (ISP_EVT_AF == message.msg_type) {
		message.sub_msg_type = AF_DATA_AFM_STAT;
		ret = cmr_thread_msg_send(cxt->thr_afhandle, &message);
	} else if (evt == ISP_EVT_AE || evt == ISP_EVT_SOF) {
		ret = cmr_thread_msg_send(cxt->thr_aehandle, &message);
	} else {
		ret = cmr_thread_msg_send(cxt->thr_handle, &message);
	}
	if (ret) {
		ISP_LOGE("fail to send a message, evt is %ld, ret: %ld, cxt: %p, af_handle: %p, handle: %p",
			evt, ret, cxt, cxt->thr_afhandle, cxt->thr_handle);
		free(message.data);
	}
}

static cmr_int ispalg_create_thread(cmr_handle isp_alg_handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	CMR_MSG_INIT(message);

	ret = cmr_thread_create(&cxt->thr_handle, ISP_THREAD_QUEUE_NUM,
			ispalg_thread_proc, (void *)cxt, "algfw");

	if (CMR_MSG_SUCCESS != ret) {
		ISP_LOGE("fail to create isp algfw  process thread");
		return ISP_ERROR;
	}

	ret = cmr_thread_create(&cxt->thr_afhandle,	ISP_THREAD_QUEUE_NUM,
			ispalg_afthread_proc, (void *)cxt, "afstats");

	if (CMR_MSG_SUCCESS != ret) {
		ISP_LOGE("fail to create afstats process thread");
		cmr_thread_destroy(cxt->thr_handle);
		cxt->thr_handle = (cmr_handle) NULL;
		return ISP_ERROR;
	}

	ret = cmr_thread_create(&cxt->thr_aehandle,	ISP_THREAD_QUEUE_NUM,
			ispalg_aethread_proc, (void *)cxt, "algae");

	if (CMR_MSG_SUCCESS != ret) {
		ISP_LOGE("fail to create algae process thread");
		cmr_thread_destroy(cxt->thr_handle);
		cxt->thr_handle = (cmr_handle) NULL;
		cmr_thread_destroy(cxt->thr_afhandle);
		cxt->thr_afhandle = (cmr_handle) NULL;
		return ISP_ERROR;
	}

	ret = cmr_thread_create(&cxt->thr_aihandle,
			ISP_THREAD_QUEUE_NUM,
			ispalg_aithread_proc, (void *)cxt, "algai");
	if (CMR_MSG_SUCCESS != ret) {
		cxt->thr_aihandle = (cmr_handle) NULL;
		ISP_LOGE("fail to create isp ai process thread");
		return CMR_MSG_SUCCESS;
	}

	ISP_LOGD("cam%ld send AI init message\n", cxt->camera_id);
	message.msg_type = AI_CMD_INIT;
	message.sync_flag = CMR_MSG_SYNC_NONE;
	cmr_thread_msg_send(cxt->thr_aihandle, &message);

	return ret;
}

static cmr_int ispalg_destroy_thread_proc(cmr_handle isp_alg_handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;

	if (cxt->thr_handle) {
		ret = cmr_thread_destroy(cxt->thr_handle);
		if (!ret) {
			cxt->thr_handle = (cmr_handle) NULL;
		} else {
			ISP_LOGE("fail to destroy isp algfw process thread");
		}
	}

	if (cxt->thr_afhandle) {
		ret = cmr_thread_destroy(cxt->thr_afhandle);
		if (!ret) {
			cxt->thr_afhandle = (cmr_handle) NULL;
		} else {
			ISP_LOGE("fail to destroy afstats process thread");
		}
	}

	if (cxt->thr_aehandle) {
		ret = cmr_thread_destroy(cxt->thr_aehandle);
		if (!ret) {
			cxt->thr_aehandle = (cmr_handle) NULL;
		} else {
			ISP_LOGE("fail to destroy algae process thread");
		}
	}

	if (cxt->thr_aihandle) {
		ret = cmr_thread_destroy(cxt->thr_aihandle);
		if (!ret) {
			cxt->thr_aihandle = (cmr_handle) NULL;
		} else {
			ISP_LOGE("fail to destroy isp alg ai process thread");
		}
	}

	ISP_LOGI("done %ld", ret);
	return ret;
}

static cmr_u32 ispalg_get_param_index(
		struct sensor_raw_resolution_info *input_size_trim,
		struct isp_size *size)
{
	cmr_u32 i;
	cmr_u32 param_index = 0x01;

	for (i = 0x01; i < ISP_INPUT_SIZE_NUM_MAX; i++) {
		if (size->h == input_size_trim[i].height) {
			param_index = i;
			break;
		}
	}

	return param_index;
}

static cmr_int ispalg_pm_init_sync(
	struct fw_init_local *init_cxt,
	struct isp_init_param *input_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = init_cxt->cxt;
	struct sensor_raw_info *sensor_raw_info_ptr  = PNULL;
	struct isp_pm_init_input pm_init_input;
	cmr_u32 i = 0;

	memset(&pm_init_input, 0, sizeof(pm_init_input));
	sensor_raw_info_ptr = (struct sensor_raw_info *)input_ptr->setting_param_ptr;
	for (i = 0; i < MAX_MODE_NUM; i++) {
		pm_init_input.tuning_data[i].data_ptr = sensor_raw_info_ptr->mode_ptr[i].addr;
		pm_init_input.tuning_data[i].size = sensor_raw_info_ptr->mode_ptr[i].len;
		pm_init_input.fix_data[i] = sensor_raw_info_ptr->fix_ptr[i];
	}
	pm_init_input.nr_fix_info = &(sensor_raw_info_ptr->nr_fix);
	pm_init_input.sensor_raw_info_ptr = sensor_raw_info_ptr;
	pm_init_input.is_4in1_sensor = cxt->is_4in1_sensor;
	pm_init_input.push_param_path = input_ptr->param_path;

	cxt->sn_cxt.sn_raw_info = sensor_raw_info_ptr;
	cxt->ioctrl_ptr = sensor_raw_info_ptr->ioctrl_ptr;
	if (cxt->ioctrl_ptr == NULL) {
		ISP_LOGW("Warning: sensor ioctrl is null.");
	}

	cxt->commn_cxt.image_pattern = sensor_raw_info_ptr->resolution_info_ptr->image_pattern;
	memcpy(cxt->commn_cxt.input_size_trim,
	       sensor_raw_info_ptr->resolution_info_ptr->tab,
	       ISP_INPUT_SIZE_NUM_MAX * sizeof(struct sensor_raw_resolution_info));
	cxt->commn_cxt.param_index =
		ispalg_get_param_index(cxt->commn_cxt.input_size_trim, &input_ptr->size);

	memcpy(&init_cxt->pm_init_input, &pm_init_input, sizeof(struct isp_pm_init_input));
	return ret;
}


/* async init */
static cmr_int ispalg_pm_init(struct fw_init_local *init_cxt)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = init_cxt->cxt;
	struct isp_pm_init_input pm_init_input;
	struct isp_pm_init_output pm_init_output;

	ISP_LOGD("cam%ld start\n", cxt->camera_id);

	memcpy(&pm_init_input, &init_cxt->pm_init_input, sizeof(struct isp_pm_init_input));
	memset(&pm_init_output, 0, sizeof(pm_init_output));

	cxt->handle_pm = isp_pm_init(&pm_init_input, &pm_init_output);
	if (PNULL == cxt->handle_pm) {
		ISP_LOGE("fail to do isp_pm_init");
		return ISP_ERROR;
	}
	cxt->commn_cxt.multi_nr_flag = pm_init_output.multi_nr_flag;
	pthread_mutex_init(&cxt->pm_getting_lock, NULL);

	ISP_LOGD("cam%ld done\n", cxt->camera_id);
	return ret;
}

static cmr_int ispalg_ae_init(struct isp_alg_fw_context *cxt)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_u32 num = 0;
	cmr_u32 i = 0;
	cmr_u32 dflash_num = 0;
	struct ae_init_in ae_input;
	struct isp_rgb_aem_info aem_info;
	struct isp_pm_ioctl_output output;
	struct isp_pm_param_data *param_data = NULL;
	struct ae_init_out result;

	memset((void *)&result, 0, sizeof(result));
	memset((void *)&ae_input, 0, sizeof(ae_input));

	pthread_mutex_lock(&cxt->pm_getting_lock);

	memset(&output, 0, sizeof(output));
	ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_INIT_DUAL_FLASH, NULL, &output);
	if (ISP_SUCCESS != ret || 0 == output.param_num) {
		ISP_LOGW("warn: get dual flash param, ret %ld, num %d\n", ret, output.param_num);
	} else {
		param_data = output.param_data;
		for (i = 0; i < output.param_num; i++) {
			if (NULL != param_data->data_ptr && (dflash_num < AE_MAX_PARAM_NUM)) {
				ae_input.flash_tuning[dflash_num].param = param_data->data_ptr;
				ae_input.flash_tuning[dflash_num].size = param_data->data_size;
				++dflash_num;
			}
			++param_data;
		}
	}

	/*  get tuning param for FDR algo */
	memset(&output, 0, sizeof(output));
	cxt->fdr_cxt.tuning_param_ptr = NULL;
	cxt->fdr_cxt.tuning_param_size = 0;
	ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_FDR_PARAM, NULL, &output);
	if (ISP_SUCCESS != ret || output.param_num < 1 || output.param_data->data_ptr == NULL) {
		ISP_LOGD("cam%ld, fdr param is not found. ret %ld, num %d, ptr %p\n",
			cxt->camera_id, ret, output.param_num, output.param_data->data_ptr);
	} else {
		cxt->fdr_cxt.tuning_param_ptr = output.param_data->data_ptr;
		cxt->fdr_cxt.tuning_param_size = output.param_data->data_size;
		ISP_LOGD("cam%ld, get fdr param, num %d,  ptr %p, size %d\n", cxt->camera_id,
			output.param_num, output.param_data->data_ptr, output.param_data->data_size);
	}

	memset(&output, 0, sizeof(output));
	ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_AE_SYNC, NULL, &output);
	if (ISP_SUCCESS != ret || 1 != output.param_num) {
		ISP_LOGW("warn: get ae sync param ret %ld, num %d\n", ret, output.param_num);
	} else {
		param_data = output.param_data;
		if (param_data->data_ptr != NULL) {
			ae_input.ae_sync_param.param = param_data->data_ptr;
			ae_input.ae_sync_param.size = param_data->data_size;
		}
	}

	memset(&output, 0, sizeof(output));
	ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_INIT_AE, NULL, &output);
	if (ISP_SUCCESS != ret || output.param_num == 0) {
		ISP_LOGW("warn: get ae init param, ret %ld, num %d\n", ret, output.param_num);
	} else {
		param_data = output.param_data;
		if (param_data)
			ae_input.has_force_bypass = param_data->user_data[0];
		for (i = 0; (i < output.param_num) && param_data; i++) {
			if (NULL != param_data->data_ptr && (num < AE_MAX_PARAM_NUM)) {
				ae_input.param[num].param = param_data->data_ptr;
				ae_input.param[num].size = param_data->data_size;
				++num;
			}
			++param_data;
		}
	}
	pthread_mutex_unlock(&cxt->pm_getting_lock);

	ae_input.param_num = num;
	ae_input.dflash_num = dflash_num;
	ae_input.fdr_tuning_param = cxt->fdr_cxt.tuning_param_ptr;
	ae_input.fdr_tuning_size = cxt->fdr_cxt.tuning_param_size;
	ae_input.resolution_info.frame_size.w = cxt->commn_cxt.src.w;
	ae_input.resolution_info.frame_size.h = cxt->commn_cxt.src.h;
	ae_input.resolution_info.frame_line = cxt->commn_cxt.input_size_trim[1].frame_line;
	ae_input.resolution_info.line_time = cxt->commn_cxt.input_size_trim[1].line_time;
	ae_input.resolution_info.sensor_size_index = 1;
	ae_input.camera_id = cxt->camera_id;
	ae_input.lib_param = cxt->lib_use_info->ae_lib_info;
	ae_input.caller_handle = (cmr_handle) cxt;
	ae_input.ebd_support = cxt->ebd_cxt.ebd_support;
	ae_input.ae_set_cb = ispalg_ae_set_cb;

	ret = ispalg_get_aem_param(cxt, &aem_info);
	cxt->ae_cxt.win_num.w = aem_info.blk_num.w;
	cxt->ae_cxt.win_num.h = aem_info.blk_num.h;

	ae_input.monitor_win_num.w = cxt->ae_cxt.win_num.w;
	ae_input.monitor_win_num.h = cxt->ae_cxt.win_num.h;
	ae_input.sensor_role = cxt->sensor_role;
 	ae_input.is_master = cxt->is_master;
	ae_input.is_faceId_unlock = cxt->commn_cxt.is_faceId_unlock;

	switch (cxt->is_multi_mode) {
	case ISP_SINGLE:
		ae_input.is_multi_mode = ISP_ALG_SINGLE;
		break;
	case ISP_DUAL_NORMAL:
		ae_input.is_multi_mode = ISP_ALG_DUAL_C_C;
		break;
	case ISP_BOKEH:
		ae_input.is_multi_mode = ISP_ALG_DUAL_C_C;
		break;
	case ISP_WIDETELE:
		ae_input.is_multi_mode = ISP_ALG_DUAL_W_T;
		break;
	case ISP_WIDETELEULTRAWIDE:
#ifdef CONFIG_ISP_2_8
		ae_input.is_multi_mode = ISP_ALG_TRIBLE_W_T_UW_SYNC;
#else
		ae_input.is_multi_mode = ISP_ALG_TRIBLE_W_T_UW;
#endif
		break;
	default:
		ae_input.is_multi_mode = ISP_ALG_SINGLE;
		break;
	}

	if (ae_input.is_multi_mode == ISP_ALG_TRIBLE_W_T_UW ||
		ae_input.is_multi_mode == ISP_ALG_TRIBLE_W_T_UW_SYNC) {
		char value[PROPERTY_VALUE_MAX] = { 0x00 };
		property_get("persist.vendor.cam.debug.ae_sync", value, "1");

		if (atoi(value) == 0) {
			ae_input.is_multi_mode = ISP_ALG_SINGLE;
		}
	}

	ae_input.long_expose_support = cxt->long_expose_support;

	ISP_LOGI("camera_id %u, is_master %u, is_multi_mode %u, sensor_role %u",
			ae_input.camera_id, ae_input.is_master,
			ae_input.is_multi_mode, ae_input.sensor_role);

	ae_input.ptr_isp_br_ioctrl = isp_br_ioctrl;

	if (cxt->ops.ae_ops.init) {
		ret = cxt->ops.ae_ops.init(&ae_input, &cxt->ae_cxt.handle, (cmr_handle)&result);
		ISP_TRACE_IF_FAIL(ret, ("fail to do ae_ctrl_init"));
	}
	cxt->ae_cxt.flash_version = result.flash_ver;
	ISP_LOGD("done. %ld\n", ret);
	return ret;
}

static cmr_int ispalg_awb_init(struct isp_alg_fw_context *cxt)
{
	cmr_int ret = ISP_SUCCESS;
	char awb_ver[PROPERTY_VALUE_MAX];
	struct isp_pm_ioctl_input input;
	struct isp_pm_ioctl_output output;
	struct awb_ctrl_init_param param;
	struct isp_rgb_aem_info aem_info;
	struct ae_monitor_info info;

	memset((void *)&input, 0, sizeof(input));
	memset((void *)&output, 0, sizeof(output));
	memset((void *)&param, 0, sizeof(param));
	memset((void *)&info, 0, sizeof(info));

	cxt->awb_cxt.cur_gain.r = 1;
	cxt->awb_cxt.cur_gain.b = 1;

	if (cxt->ops.ae_ops.ioctrl) {
		ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_GET_MONITOR_INFO, NULL, (void *)&info);
		ISP_TRACE_IF_FAIL(ret, ("fail to get ae monitor info"));
		if (ret) {
			ret = ispalg_get_aem_param(cxt, &aem_info);
			info.win_num.w = aem_info.blk_num.w;
			info.win_num.h = aem_info.blk_num.h;
			info.win_size.w = ((cxt->commn_cxt.src.w / aem_info.blk_num.w) & ~7);
			info.win_size.h = ((cxt->commn_cxt.src.h / aem_info.blk_num.h) & ~7);
		}
	}

	param.camera_id = cxt->camera_id;
	param.base_gain = 1024;
	param.awb_enable = 1;
	param.wb_mode = 0;
	param.stat_img_format = AWB_CTRL_STAT_IMG_CHN;
	param.stat_img_size.w = info.win_num.w;
	param.stat_img_size.h = info.win_num.h;
	param.stat_img_size_ae.w = info.win_num.w;
	param.stat_img_size_ae.h = info.win_num.h;
	param.stat_win_size.w = info.win_size.w;
	param.stat_win_size.h = info.win_size.h;
	param.awb_set_cb = ispalg_awb_set_cb;
	param.caller_handle = (cmr_handle) cxt;

	ISP_LOGI("awb get ae win %d %d %d %d %d %d\n", info.trim.x, info.trim.y,
		info.win_size.w, info.win_size.h, info.win_num.w, info.win_num.h);

	pthread_mutex_lock(&cxt->pm_getting_lock);
	ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_INIT_AWB, &input, &output);
	ISP_TRACE_IF_FAIL(ret, ("fail to get awb init param"));
	if (ret == ISP_SUCCESS && output.param_data != NULL) {
		param.tuning_param = output.param_data->data_ptr;
		param.param_size = output.param_data->data_size;
	}
	pthread_mutex_unlock(&cxt->pm_getting_lock);

	param.lib_param = cxt->lib_use_info->awb_lib_info;
	ISP_LOGV("param addr is %p size %d", param.tuning_param, param.param_size);

	param.otp_info_ptr = cxt->otp_data;
	param.is_master = cxt->is_master;
	param.sensor_role = cxt->sensor_role;
	param.color_support = cxt->awb_cxt.color_support;


	/* here can select awb3.x using tuning_param */
	ISP_LOGI("AWB VERSION = %x", *((int *)output.param_data->data_ptr +1));

	/* get awb version for HWSIM */
	if (*((int *)output.param_data->data_ptr +1) == 0x000c0007)
            sprintf(awb_ver, "%s\n", "awb2.x");
	else
            sprintf(awb_ver, "%s\n", "awb3.x");

	/* ispctl_calc_awb wil use it to load diffrent lib */
	property_set("persist.vendor.cam.isp.awb", awb_ver);


	switch (cxt->is_multi_mode) {
	case ISP_SINGLE:
		param.is_multi_mode = ISP_ALG_SINGLE;
		break;
	case ISP_DUAL_NORMAL:
		param.is_multi_mode = ISP_ALG_DUAL_C_C;
		break;
	case ISP_BOKEH:
		param.is_multi_mode = ISP_ALG_DUAL_C_C;
		break;
	case ISP_WIDETELE:
		param.is_multi_mode = ISP_ALG_DUAL_W_T;
		break;
	case ISP_WIDETELEULTRAWIDE:
#ifdef CONFIG_ISP_2_8
		param.is_multi_mode = ISP_ALG_TRIBLE_W_T_UW_SYNC;
#else
		param.is_multi_mode = ISP_ALG_TRIBLE_W_T_UW;
#endif
		break;
	default:
		param.is_multi_mode = ISP_ALG_SINGLE;
		break;
	}
	ISP_LOGI("is_master %u, sensor_role %u, is_multi_mode %u",
			cxt->is_master, cxt->sensor_role, cxt->is_multi_mode);
	param.ptr_isp_br_ioctrl = isp_br_ioctrl;

	if (cxt->ops.awb_ops.init) {
		ret = cxt->ops.awb_ops.init(&param, &cxt->awb_cxt.handle);
		ISP_TRACE_IF_FAIL(ret, ("fail to do awb_ctrl_init"));
	}
	ISP_LOGI("done %ld", ret);
	return ret;
}

static cmr_int ispalg_afl_init(struct isp_alg_fw_context *cxt)
{
	cmr_int ret = ISP_SUCCESS;
	struct afl_ctrl_init_in afl_input;

	/*0:afl_old mode, 1:afl_new mode*/
	cxt->afl_cxt.version = 1;
	afl_input.dev_handle = cxt->dev_access_handle;
	afl_input.camera_id = cxt->camera_id;
	afl_input.size = cxt->commn_cxt.src;
	afl_input.vir_addr = &cxt->afl_cxt.vir_addr;
	afl_input.caller_handle = (cmr_handle) cxt;
	afl_input.afl_set_cb = ispalg_afl_set_cb;
	afl_input.version = cxt->afl_cxt.version;
	if (cxt->ops.afl_ops.init)
		ret = cxt->ops.afl_ops.init(&cxt->afl_cxt.handle, &afl_input);
exit:
	ISP_LOGI("done %ld", ret);
	return ret;
}

static cmr_int ispalg_smart_init(struct isp_alg_fw_context *cxt)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_u32 i = 0;
	cmr_u32 mode;
	struct smart_init_param smart_init_param;
	struct isp_pm_ioctl_output pm_output;

	cxt->smart_cxt.isp_smart_eb = 0;
	memset(&smart_init_param, 0, sizeof(smart_init_param));
	memset(&pm_output, 0, sizeof(pm_output));

	pthread_mutex_lock(&cxt->pm_getting_lock);
	ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_INIT_SMART, NULL, &pm_output);
	if (ISP_SUCCESS == ret) {
		for (i = 0; i < pm_output.param_num; ++i) {
			mode = pm_output.param_data[i].mode_id;
			smart_init_param.tuning_param[mode].data.size = pm_output.param_data[i].data_size;
			smart_init_param.tuning_param[mode].data.data_ptr = pm_output.param_data[i].data_ptr;
		}
	} else {
		pthread_mutex_unlock(&cxt->pm_getting_lock);
		ISP_LOGE("fail to get smart init param ");
		return ret;
	}
	smart_init_param.caller_handle = (cmr_handle)cxt;
	smart_init_param.smart_set_cb = ispalg_smart_set_cb;
	smart_init_param.camera_id = cxt->camera_id;
	ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_ATM_PARAM, NULL, &pm_output);
	if (ISP_SUCCESS != ret || pm_output.param_num != 1) {
		ISP_LOGW("warn: no atm param. ret %ld, num %d\n", ret, pm_output.param_num);
	} else {
		if (pm_output.param_data->data_ptr) {
			smart_init_param.atm_info = pm_output.param_data->data_ptr;
			smart_init_param.atm_size = pm_output.param_data->data_size;
		}
	}
	pthread_mutex_unlock(&cxt->pm_getting_lock);

	if (cxt->ops.smart_ops.init) {
		cxt->smart_cxt.handle = cxt->ops.smart_ops.init(&smart_init_param, NULL);
		if (NULL == cxt->smart_cxt.handle) {
			ISP_LOGE("fail to do smart init");
			return ret;
		}
	}
exit:
	ISP_LOGI("done %ld", ret);
	return ret;
}

static cmr_int ispalg_af_init(struct isp_alg_fw_context *cxt)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_u32 is_af_support = 1;
	cmr_u32 zoom_ratio;
	struct afctrl_init_in af_input;
	struct af_log_info af_param = {NULL, 0};
	struct af_log_info aft_param = {NULL, 0};
	struct isp_pm_ioctl_output output = { NULL, 0 };

	if (NULL == cxt->ioctrl_ptr || NULL == cxt->ioctrl_ptr->set_pos)
		is_af_support = 0;

	memset((void *)&af_input, 0, sizeof(af_input));

	af_input.camera_id = cxt->camera_id;
	af_input.lib_param = cxt->lib_use_info->af_lib_info;
	af_input.caller_handle = (cmr_handle) cxt;
	af_input.af_set_cb = ispalg_af_set_cb;
	af_input.src.w = cxt->commn_cxt.src.w;
	af_input.src.h = cxt->commn_cxt.src.h;
	af_input.is_supoprt = is_af_support;
	af_input.pdaf_type = cxt->pdaf_cxt.pdaf_support;
	cxt->af_cxt.sw_bypass = 0;

	pthread_mutex_lock(&cxt->pm_getting_lock);

	//get af tuning parameters
	memset((void *)&output, 0, sizeof(output));
	ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_INIT_AF_NEW, NULL, &output);
	if(ISP_SUCCESS == ret && NULL != output.param_data){
		/* get bypass flag from pm */
		cxt->af_cxt.sw_bypass = output.param_data[0].user_data[0];
		ISP_LOGD("cxt->af_cxt.sw_bypass %d\n", cxt->af_cxt.sw_bypass);
		if (1 == is_af_support) {
			af_input.aftuning_data = output.param_data[0].data_ptr;
			af_input.aftuning_data_len = output.param_data[0].data_size;
		}
	}

	if(1 == is_af_support) {
		//get af trigger tuning parameters
		memset((void *)&output, 0, sizeof(output));
		ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_INIT_AFT, NULL, &output);
		if(ISP_SUCCESS == ret && NULL != output.param_data){
			af_input.afttuning_data = output.param_data[0].data_ptr;
			af_input.afttuning_data_len = output.param_data[0].data_size;
		}

		//get pdaf tuning parameters
		memset((void *)&output, 0, sizeof(output));
		ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_INIT_PDAF, NULL, &output);
		if(ISP_SUCCESS == ret && 1 == output.param_num && NULL != output.param_data) {
			af_input.pdaftuning_data = output.param_data[0].data_ptr;
			af_input.pdaftuning_data_len = output.param_data[0].data_size;
		}

		//[TOF_tuning]get tof tuning parameters
		memset((void *)&output, 0, sizeof(output));
		ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_INIT_TOF, NULL, &output);
		if(ISP_SUCCESS == ret && NULL != output.param_data){
			af_input.toftuning_data = output.param_data[0].data_ptr;
			af_input.toftuning_data_len = output.param_data[0].data_size;
		}
	}
	pthread_mutex_unlock(&cxt->pm_getting_lock);

	af_input.is_master = cxt->is_master;
	af_input.sensor_role = cxt->sensor_role;
	switch (cxt->is_multi_mode) {
	case ISP_SINGLE:
		af_input.is_multi_mode = AF_ALG_SINGLE;
		break;
	case ISP_DUAL_NORMAL:
		af_input.is_multi_mode = AF_ALG_DUAL_C_C;
		break;
	case ISP_BLUR_REAR:
		af_input.is_multi_mode = AF_ALG_BLUR_REAR;
		break;
	case ISP_BOKEH:
		af_input.is_multi_mode = AF_ALG_DUAL_C_C;
		break;
	case ISP_WIDETELE:
		af_input.is_multi_mode = AF_ALG_DUAL_W_T;
		break;
	case ISP_BLUR_PORTRAIT:
		af_input.is_multi_mode = AF_ALG_BLUR_PORTRAIT;
		break;
	case ISP_WIDETELEULTRAWIDE:
#ifdef CONFIG_ISP_2_8
		af_input.is_multi_mode = AF_ALG_TRIBLE_MT_SMOOTH;
#else
		af_input.is_multi_mode = AF_ALG_TRIBLE_W_T_UW;
#endif
		break;
	default:
		af_input.is_multi_mode = AF_ALG_SINGLE;
		break;
	}

	ISP_LOGI("camera_id %u, is_master %u, is_multi_mode %u, sensor_role %u, pdaf_type %d",
			af_input.camera_id, af_input.is_master, af_input.is_multi_mode,
			af_input.sensor_role, af_input.pdaf_type);

	af_input.otp_info_ptr = cxt->otp_data;
	af_input.br_ctrl = isp_br_ioctrl;

	if (cxt->ops.af_ops.init) {
		ret = cxt->ops.af_ops.init(&af_input, &cxt->af_cxt.handle);
		ISP_TRACE_IF_FAIL(ret, ("fail to do af_ctrl_init"));
	}

	if (cxt->ops.af_ops.ioctrl) {
		ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle, AF_CMD_GET_AF_LOG_INFO, (void *)&af_param, NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to get_af_log_info"));
		cxt->af_cxt.log_af = af_param.log_cxt;
		cxt->af_cxt.log_af_size = af_param.log_len;
		ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle, AF_CMD_GET_AFT_LOG_INFO, (void *)&aft_param, NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to get_aft_log_info"));
		cxt->aft_cxt.log_aft = aft_param.log_cxt;
		cxt->aft_cxt.log_aft_size = aft_param.log_len;
	}

	/* init ratio for 4x zoom */
	if (cxt->ops.af_ops.ioctrl) {
		zoom_ratio = 1000;
		ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle, AF_CMD_SET_ZOOM_RATIO, (void *)&zoom_ratio, NULL);
	}

	return ret;
}

static cmr_int ispalg_pdaf_init(struct isp_alg_fw_context *cxt)
{
	cmr_int ret = ISP_SUCCESS;
	struct pdaf_ctrl_init_in pdaf_input;
	struct pdaf_ctrl_init_out pdaf_output;

	if (!cxt->pdaf_cxt.pdaf_support || cxt->pdaf_info == NULL) {
		ISP_LOGI("cam%ld no pdaf %d %p\n", cxt->camera_id,
			cxt->pdaf_cxt.pdaf_support, cxt->pdaf_info);
		cxt->pdaf_cxt.pdaf_support = 0;
		cxt->pdaf_cxt.pdaf_en = 0;
		return ret;
	}
	ISP_LOGD("cam%ld start\n", cxt->camera_id);

	memset(&pdaf_input, 0x00, sizeof(pdaf_input));
	memset(&pdaf_output, 0x00, sizeof(pdaf_output));

	pdaf_input.camera_id = cxt->camera_id;
	pdaf_input.caller_handle = (cmr_handle) cxt;
	pdaf_input.pdaf_support = cxt->pdaf_cxt.pdaf_support;
	pdaf_input.pdaf_set_cb = ispalg_pdaf_set_cb;
	pdaf_input.pd_info = cxt->pdaf_info;
	pdaf_input.sensor_max_size = cxt->commn_cxt.sensor_max_size;

	if (SENSOR_PDAF_TYPE3_ENABLE == cxt->pdaf_cxt.pdaf_support ||
		SENSOR_PDAF_TYPE2_ENABLE == cxt->pdaf_cxt.pdaf_support) {
		pdaf_input.otp_info_ptr = cxt->otp_data;
		pdaf_input.is_master= cxt->is_master;
	}

	if (cxt->ops.pdaf_ops.init)
		ret = cxt->ops.pdaf_ops.init(&pdaf_input, &pdaf_output, &cxt->pdaf_cxt.handle);

exit:
	if (ret) {
		ISP_LOGE("fail to do PDAF initialize");
	}
	ISP_LOGI("done %ld", ret);

	return ret;
}

static cmr_int ispalg_lsc_init(struct isp_alg_fw_context *cxt)
{
	cmr_u32 ret = ISP_SUCCESS;
	cmr_s32 i = 0;
	cmr_handle pm_handle = cxt->handle_pm;
	lsc_adv_handle_t lsc_adv_handle = NULL;
	struct lsc_adv_init_param lsc_param;
	struct isp_2d_lsc_param *lsc_tab_param_ptr;
	struct isp_lsc_info *lsc_info;
	struct alsc_ver_info lsc_ver = { 0 };
	struct isp_pm_ioctl_input io_pm_input = { PNULL, 0 };
	struct isp_pm_ioctl_output io_pm_output = { PNULL, 0 };

	memset(&lsc_param, 0, sizeof(struct lsc_adv_init_param));
	memset(&io_pm_input, 0, sizeof(struct isp_pm_ioctl_input));
	memset(&io_pm_output, 0, sizeof(struct isp_pm_ioctl_output));

	ISP_LOGV(" _alsc_init");

	ret = ispalg_alsc_get_info(cxt);
	ISP_RETURN_IF_FAIL(ret, ("fail to do get lsc info"));
	lsc_tab_param_ptr = (struct isp_2d_lsc_param *)(cxt->lsc_cxt.lsc_tab_address);
	lsc_info = (struct isp_lsc_info *)cxt->lsc_cxt.lsc_info;

	pthread_mutex_lock(&cxt->pm_getting_lock);
	ret = isp_pm_ioctl(pm_handle, ISP_PM_CMD_GET_INIT_ALSC, &io_pm_input, &io_pm_output);
	ISP_TRACE_IF_FAIL(ret, ("fail to do get init alsc"));

	if ((ret != ISP_SUCCESS) || (io_pm_output.param_data == NULL)) {
		lsc_param.tune_param_ptr = NULL;
		ISP_LOGE("fail to get alsc param from sensor file");
	} else {
		lsc_param.tune_param_ptr = io_pm_output.param_data->data_ptr;
	}
	pthread_mutex_unlock(&cxt->pm_getting_lock);

	lsc_param.otp_info_ptr = cxt->otp_data;
	lsc_param.is_master = cxt->is_master;

	for (i = 0; i < 9; i++) {
		lsc_param.lsc_tab_address[i] = lsc_tab_param_ptr->map_tab[i].param_addr;
		lsc_param.rg_gradient[i] = lsc_tab_param_ptr->map_tab[i].rg;
		lsc_param.bg_gradient[i] = lsc_tab_param_ptr->map_tab[i].bg;
	}

	lsc_param.img_height = lsc_tab_param_ptr->resolution.h;
	lsc_param.img_width = lsc_tab_param_ptr->resolution.w;
	lsc_param.gain_width = lsc_info->gain_w;
	lsc_param.gain_height = lsc_info->gain_h;
	lsc_param.lum_gain = (cmr_u16 *) lsc_info->data_ptr;
	lsc_param.grid = lsc_info->grid;
	lsc_param.camera_id = cxt->camera_id;
	lsc_param.lib_param = cxt->lib_use_info->lsc_lib_info;
	lsc_param.lib_param.version_id = 1;
	lsc_param.caller_handle = (cmr_handle) cxt;
	lsc_param.lsc_set_cb = ispalg_lsc_set_cb;
#if defined(CONFIG_ISP_2_5)
	lsc_param.lib_param.version_id = 0;
	lsc_param.caller_handle = NULL;
	lsc_param.lsc_set_cb = NULL;
#elif defined(CONFIG_ISP_2_6)
	lsc_param.caller_handle = NULL;
	lsc_param.lsc_set_cb = NULL;
#endif
	ISP_LOGD("lsc version_id %d, set_cb %p %p\n", lsc_param.lib_param.version_id,
		lsc_param.caller_handle, lsc_param.lsc_set_cb);

	/*  alsc tuning param should be private and parsed in alsc lib */
	//struct lsc2_tune_param* param = (struct lsc2_tune_param*)lsc_param.tune_param_ptr;
	cxt->lsc_cxt.full_size_width = lsc_tab_param_ptr->resolution.w;
	cxt->lsc_cxt.full_size_height = lsc_tab_param_ptr->resolution.h;
	cxt->lsc_cxt.full_size_grid = lsc_tab_param_ptr->lsc_info.grid;

	ISP_LOGD("[ALSC] full_img_size[%d,%d], table_w_h_gird[%d,%d,%d]",
		lsc_tab_param_ptr->resolution.w,lsc_tab_param_ptr->resolution.h,
		lsc_tab_param_ptr->lsc_info.gain_w,lsc_tab_param_ptr->lsc_info.gain_h,
		lsc_tab_param_ptr->lsc_info.grid);

	/* gain_pattern is for input tab generated by ISP tool
	  * the following pattern translating due to history reason
	  * do not modify it unless ISP tool changed */
	switch (cxt->commn_cxt.image_pattern) {
	case SENSOR_IMAGE_PATTERN_RAWRGB_GR:
		lsc_param.gain_pattern = LSC_GAIN_PATTERN_RGGB;
		break;

	case SENSOR_IMAGE_PATTERN_RAWRGB_R:
		lsc_param.gain_pattern = LSC_GAIN_PATTERN_GRBG;
		break;

	case SENSOR_IMAGE_PATTERN_RAWRGB_B:
		lsc_param.gain_pattern = LSC_GAIN_PATTERN_GBRG;
		break;

	case SENSOR_IMAGE_PATTERN_RAWRGB_GB:
		lsc_param.gain_pattern = LSC_GAIN_PATTERN_BGGR;
		break;

	default:
		break;
	}
	/* change_pattern_flag =1 means output_gain_pattern will be used by LSC alg lib
	  * output_gain_pattern is LSC algo lib output tab pattern
	  * this pattern is for hardware reading and using
	  * in dcam_if_lite r1p0, lsc block always apply BGGR pattern no matter what's the sensor pattern
	  * do not modify it unless hardware changed */
	lsc_param.output_gain_pattern = LSC_GAIN_PATTERN_BGGR;
	lsc_param.change_pattern_flag = 1;
	ISP_LOGV("alsc_init, gain_pattern=%d, output_gain_pattern=%d, flag=%d",
		lsc_param.gain_pattern, lsc_param.output_gain_pattern, lsc_param.change_pattern_flag);

	lsc_param.is_master = cxt->is_master;
	lsc_param.is_multi_mode = cxt->is_multi_mode;

	if (NULL == cxt->lsc_cxt.handle) {
		if (cxt->ops.lsc_ops.init) {
			ret = cxt->ops.lsc_ops.init(&lsc_param, &lsc_adv_handle);
			if (NULL == lsc_adv_handle) {
				cxt->ops.lsc_ops.init = NULL;
				cxt->ops.lsc_ops.ioctrl = NULL;
				cxt->ops.lsc_ops.process = NULL;
				cxt->ops.lsc_ops.deinit = NULL;
				ISP_LOGE("fail to do lsc adv init, camera_id=%d", lsc_param.camera_id);
			}
			cxt->lsc_cxt.handle = lsc_adv_handle;
		}
	}

	if (cxt->ops.lsc_ops.ioctrl){
		ret = cxt->ops.lsc_ops.ioctrl(lsc_adv_handle, ALSC_GET_VER, NULL, (void *)&lsc_ver);
		if (ISP_SUCCESS != ret) {
			ISP_LOGE("fail to Get ALSC ver info!");
			cxt->lsc_cxt.LSC_SPD_VERSION = 1;
		} else {
			cxt->lsc_cxt.LSC_SPD_VERSION =  lsc_ver.LSC_SPD_VERSION;
			ISP_LOGD("Get ALSC ver: %d\n", lsc_ver.LSC_SPD_VERSION);
		}
	} else {
		cxt->lsc_cxt.LSC_SPD_VERSION = 1;
		ISP_LOGD("lsc_ops is NULL, LSC_SPD_VERSION: %d\n", cxt->lsc_cxt.LSC_SPD_VERSION);
	}

	return ret;
}

static cmr_int ispalg_ai_init(struct isp_alg_fw_context *cxt)
{
	cmr_int ret = ISP_SUCCESS;
	struct ai_init_in ai_input;
	struct ai_init_out result;

	memset((void *)&result, 0, sizeof(result));
	memset((void *)&ai_input, 0, sizeof(ai_input));

	ai_input.caller_handle = (cmr_handle)cxt;
	ai_input.ai_set_cb = ispalg_ai_set_cb;
	ai_input.cameraId = cxt->camera_id;
	ISP_LOGD("cameraId: %d", ai_input.cameraId);

	if (cxt->ops.ai_ops.init)
		ret = cxt->ops.ai_ops.init(&ai_input, &cxt->ai_cxt.handle, (cmr_handle)&result);
	if (ret) {
		ISP_LOGE("cam%ld failed to init ai\n", cxt->camera_id);
		cxt->ai_cxt.handle = (cmr_handle)NULL;
		cxt->ai_init_status = FW_INIT_ERR;
		goto exit;
	}

	cxt->ops.ai_ops.ioctrl = dlsym(cxt->ispalg_lib_handle, "ai_ctrl_ioctrl");
	if (!cxt->ops.ai_ops.ioctrl) {
		ISP_LOGE("fail to dlsym ai_ops.ioctrl");
	}
	cxt->ai_init_status = FW_INIT_DONE;
exit:
	ISP_LOGI("done %ld", ret);
	return ret;
}

static cmr_int ispalg_bypass_init(struct isp_alg_fw_context *cxt)
{
	char value[PROPERTY_VALUE_MAX] = { 0x00 };
	cmr_u32 val;

	property_get(PROP_ISP_AE_BYPASS, value, "0");
	val = atoi(value);
	if (val < 2)
		cxt->ae_cxt.sw_bypass = val;

	property_get(PROP_ISP_AF_BYPASS, value, "0");
	val = atoi(value);
	if (val < 2)
		cxt->af_cxt.sw_bypass |= val; /* af_bypass(pm) or this */

	property_get(PROP_ISP_AWB_BYPASS, value, "0");
	val = atoi(value);
	if (val < 2)
		cxt->awb_cxt.sw_bypass = val;

	property_get(PROP_ISP_LSC_BYPASS, value, "0");
	val = atoi(value);
	if (val < 2)
		cxt->lsc_cxt.sw_bypass = val;

	property_get(PROP_ISP_PDAF_BYPASS, value, "0");
	val = atoi(value);
	if (val < 2)
		cxt->pdaf_cxt.sw_bypass = val;

	property_get(PROP_ISP_AFL_BYPASS, value, "0");
	val = atoi(value);
	if (val < 2)
		cxt->afl_cxt.sw_bypass = val;

	property_get("persist.vendor.camera.bypass.smart", value, "0");
	val = atoi(value);
	if (val < 2)
		cxt->smart_cxt.sw_bypass = val;

	if (cxt->is_simulator) {
		cxt->ae_cxt.sw_bypass = 1;
		cxt->af_cxt.sw_bypass = 1;
		cxt->afl_cxt.sw_bypass = 1;
		cxt->pdaf_cxt.sw_bypass = 1;
	}
	ISP_LOGI("ae sw bypass123: %d\n", cxt->ae_cxt.sw_bypass);
	ISP_LOGI("af sw bypass: %d\n", cxt->af_cxt.sw_bypass);
	ISP_LOGI("awb sw bypass: %d\n", cxt->awb_cxt.sw_bypass);
	ISP_LOGI("lsc sw bypass: %d\n", cxt->lsc_cxt.sw_bypass);
	ISP_LOGI("afl sw bypass: %d\n", cxt->afl_cxt.sw_bypass);
	ISP_LOGI("pdaf sw bypass: %d\n", cxt->pdaf_cxt.sw_bypass);
	ISP_LOGI("smart sw bypass: %d\n", cxt->smart_cxt.sw_bypass);
	return ISP_SUCCESS;
}

static cmr_int ispalg_tof_init(struct isp_alg_fw_context *cxt)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_handle  tof_handle = NULL;
	struct tof_ctrl_init_in tof_ctrl_init;

	if (!cxt->af_cxt.tof_support) {
		ISP_LOGI("tof don't support !");
		return ret;
	}

	tof_ctrl_init.tof_set_cb = ispalg_tof_set_cb;
	tof_ctrl_init.caller_handle = (cmr_handle) cxt;

	if (NULL == cxt->tof_handle) {
		if (cxt->ops.tof_ops.init) {
			ret = cxt->ops.tof_ops.init(&tof_ctrl_init, &tof_handle);
			if (NULL == tof_handle) {
				ISP_LOGE("fail to do tof init");
				return ISP_ERROR;
			}
		}

		cxt->tof_handle = tof_handle;
	}

	ISP_LOGD("cam%ld done\n", cxt->camera_id);
	return ret;
}

static cmr_u32 ispalg_deinit(cmr_handle isp_alg_handle)
{
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;

	if (cxt->ops.pdaf_ops.deinit && cxt->pdaf_cxt.handle)
		cxt->ops.pdaf_ops.deinit(&cxt->pdaf_cxt.handle);

	if (cxt->ops.tof_ops.deinit && cxt->tof_handle)
		cxt->ops.tof_ops.deinit(&cxt->tof_handle);

	if (cxt->ops.af_ops.deinit && cxt->af_cxt.handle)
		cxt->ops.af_ops.deinit(&cxt->af_cxt.handle);

	if (cxt->ops.ae_ops.deinit && cxt->ae_cxt.handle)
		cxt->ops.ae_ops.deinit(&cxt->ae_cxt.handle);

	if (cxt->ops.lsc_ops.deinit && cxt->lsc_cxt.handle)
		cxt->ops.lsc_ops.deinit(&cxt->lsc_cxt.handle);

	if (cxt->ops.smart_ops.deinit && cxt->smart_cxt.handle)
		cxt->ops.smart_ops.deinit(&cxt->smart_cxt.handle, NULL, NULL);

	if (cxt->ops.awb_ops.deinit && cxt->awb_cxt.handle)
		cxt->ops.awb_ops.deinit(&cxt->awb_cxt.handle);

	if (cxt->ops.afl_ops.deinit && cxt->afl_cxt.handle)
		cxt->ops.afl_ops.deinit(&cxt->afl_cxt.handle);

	if (cxt->ops.ai_ops.deinit && cxt->ai_cxt.handle)
		cxt->ops.ai_ops.deinit(&cxt->ai_cxt.handle);
	ISP_LOGI("done");
	return ISP_SUCCESS;
}

static int ispalg_af_ops_library(struct isp_alg_fw_context *cxt)
{
	cxt->ops.af_ops.init = dlsym(cxt->ispalg_lib_handle, "af_ctrl_init");
	if (!cxt->ops.af_ops.init) {
		ISP_LOGE("fail to dlsym af_ops.init");
		return -1;
	}
	cxt->ops.af_ops.deinit = dlsym(cxt->ispalg_lib_handle, "af_ctrl_deinit");
	if (!cxt->ops.af_ops.deinit) {
		ISP_LOGE("fail to dlsym af_ops.deinit");
		return -1;
	}
	cxt->ops.af_ops.process = dlsym(cxt->ispalg_lib_handle, "af_ctrl_process");
	if (!cxt->ops.af_ops.process) {
		ISP_LOGE("fail to dlsym af_ops.process");
		return -1;
	}
	cxt->ops.af_ops.ioctrl = dlsym(cxt->ispalg_lib_handle, "af_ctrl_ioctrl");
	if (!cxt->ops.af_ops.ioctrl) {
		ISP_LOGE("fail to dlsym af_ops.ioctrl");
		return -1;
	}
	return 0;
}

static cmr_int ispalg_load_library(cmr_handle adpt_handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)adpt_handle;

	ISP_LOGD("cam%ld start\n", cxt->camera_id);

#if 0
	cxt->ispalg_lib_handle = dlopen(LIBCAM_ALG_FILE, RTLD_LAZY);//RTLD_LAZY,RTLD_NOW
#else
    cxt->ispalg_lib_handle = get_lib_handle(LIBCAM_ALG_FILE);
#endif
	if (!cxt->ispalg_lib_handle) {
		ISP_LOGE("fail to dlopen (%s)",dlerror());
		goto error_dlopen;
	}

	int af_ops;
	af_ops = ispalg_af_ops_library(cxt);
	if(af_ops == -1)
		goto error_dlsym;

	cxt->ops.afl_ops.init = dlsym(cxt->ispalg_lib_handle, "afl_ctrl_init");
	if (!cxt->ops.afl_ops.init) {
		ISP_LOGE("fail to dlsym afl_ops.init");
		goto error_dlsym;
	}
	cxt->ops.afl_ops.deinit = dlsym(cxt->ispalg_lib_handle, "afl_ctrl_deinit");
	if (!cxt->ops.afl_ops.deinit) {
		ISP_LOGE("fail to dlsym afl_ops.deinit");
		goto error_dlsym;
	}
	cxt->ops.afl_ops.process = dlsym(cxt->ispalg_lib_handle, "afl_ctrl_process");
	if (!cxt->ops.afl_ops.process) {
		ISP_LOGE("fail to dlsym afl_ops.process");
		goto error_dlsym;
	}
	cxt->ops.afl_ops.ioctrl= dlsym(cxt->ispalg_lib_handle, "afl_ctrl_ioctrl");
	if (!cxt->ops.afl_ops.ioctrl) {
		ISP_LOGE("fail to dlsym afl_ops.afl_ctrl_ioctrl");
		goto error_dlsym;
	}
	cxt->ops.afl_ops.config = dlsym(cxt->ispalg_lib_handle, "afl_ctrl_cfg");
	if (!cxt->ops.afl_ops.config) {
		ISP_LOGE("fail to dlsym afl_ops.cfg");
		goto error_dlsym;
	}
	cxt->ops.afl_ops.config_new = dlsym(cxt->ispalg_lib_handle, "aflnew_ctrl_cfg");
	if (!cxt->ops.afl_ops.config_new) {
		ISP_LOGE("fail to dlsym afl_ops.cfg_new");
		goto error_dlsym;
	}

	cxt->ops.ae_ops.init = dlsym(cxt->ispalg_lib_handle, "ae_ctrl_init");
	if (!cxt->ops.ae_ops.init) {
		ISP_LOGE("fail to dlsym ae_ops.init");
		goto error_dlsym;
	}
	cxt->ops.ae_ops.deinit = dlsym(cxt->ispalg_lib_handle, "ae_ctrl_deinit");
	if (!cxt->ops.ae_ops.deinit) {
		ISP_LOGE("fail to dlsym ae_ops.deinit");
		goto error_dlsym;
	}
	cxt->ops.ae_ops.process = dlsym(cxt->ispalg_lib_handle, "ae_ctrl_process");
	if (!cxt->ops.ae_ops.process) {
		ISP_LOGE("fail to dlsym ae_ops.process");
		goto error_dlsym;
	}
	cxt->ops.ae_ops.ioctrl = dlsym(cxt->ispalg_lib_handle, "ae_ctrl_ioctrl");
	if (!cxt->ops.ae_ops.ioctrl) {
		ISP_LOGE("fail to dlsym ae_ops.ioctrl");
		goto error_dlsym;
	}

	cxt->ops.awb_ops.init = dlsym(cxt->ispalg_lib_handle, "awb_ctrl_init");
	if (!cxt->ops.awb_ops.init) {
		ISP_LOGE("fail to dlsym awb_ops.init");
		goto error_dlsym;
	}
	cxt->ops.awb_ops.deinit = dlsym(cxt->ispalg_lib_handle, "awb_ctrl_deinit");
	if (!cxt->ops.awb_ops.deinit) {
		ISP_LOGE("fail to dlsym awb_ops.deinit");
		goto error_dlsym;
	}
	cxt->ops.awb_ops.process = dlsym(cxt->ispalg_lib_handle, "awb_ctrl_process");
	if (!cxt->ops.awb_ops.process) {
		ISP_LOGE("fail to dlsym awb_ops.process");
		goto error_dlsym;
	}
	cxt->ops.awb_ops.ioctrl = dlsym(cxt->ispalg_lib_handle, "awb_ctrl_ioctrl");
	if (!cxt->ops.awb_ops.ioctrl) {
		ISP_LOGE("fail to dlsym awb_ops.ioctrl");
		goto error_dlsym;
	}

	cxt->ops.pdaf_ops.init = dlsym(cxt->ispalg_lib_handle, "pdaf_ctrl_init");
	if (!cxt->ops.pdaf_ops.init) {
		ISP_LOGE("fail to dlsym pdaf_ops.init");
		goto error_dlsym;
	}
	cxt->ops.pdaf_ops.deinit = dlsym(cxt->ispalg_lib_handle, "pdaf_ctrl_deinit");
	if (!cxt->ops.pdaf_ops.deinit) {
		ISP_LOGE("fail to dlsym pdaf_ops.deinit");
		goto error_dlsym;
	}
	cxt->ops.pdaf_ops.process = dlsym(cxt->ispalg_lib_handle, "pdaf_ctrl_process");
	if (!cxt->ops.pdaf_ops.process) {
		ISP_LOGE("fail to dlsym pdaf_ops.process");
		goto error_dlsym;
	}
	cxt->ops.pdaf_ops.ioctrl = dlsym(cxt->ispalg_lib_handle, "pdaf_ctrl_ioctrl");
	if (!cxt->ops.pdaf_ops.ioctrl) {
		ISP_LOGE("fail to dlsym pdaf_ops.ioctrl");
		goto error_dlsym;
	}

	cxt->ops.smart_ops.init = dlsym(cxt->ispalg_lib_handle, "smart_ctl_init");
	if (!cxt->ops.smart_ops.init) {
		ISP_LOGE("fail to dlsym smart_ops.init");
		goto error_dlsym;
	}
	cxt->ops.smart_ops.deinit = dlsym(cxt->ispalg_lib_handle, "smart_ctl_deinit");
	if (!cxt->ops.smart_ops.deinit) {
		ISP_LOGE("fail to dlsym smart_ops.deinit");
		goto error_dlsym;
	}
	cxt->ops.smart_ops.ioctrl = dlsym(cxt->ispalg_lib_handle, "smart_ctl_ioctl");
	if (!cxt->ops.smart_ops.ioctrl) {
		ISP_LOGE("fail to dlsym smart_ops.ioctrl");
		goto error_dlsym;
	}
	cxt->ops.smart_ops.calc = dlsym(cxt->ispalg_lib_handle, "_smart_calc");
	if (!cxt->ops.smart_ops.calc) {
		ISP_LOGE("fail to dlsym smart_ops.calc");
		goto error_dlsym;
	}
	cxt->ops.smart_ops.block_disable = dlsym(cxt->ispalg_lib_handle, "smart_ctl_block_disable");
	if (!cxt->ops.smart_ops.block_disable) {
		ISP_LOGE("fail to dlsym smart_ops.block_disable");
		goto error_dlsym;
	}
	cxt->ops.smart_ops.block_enable = dlsym(cxt->ispalg_lib_handle, "smart_ctl_block_enable_recover");
	if (!cxt->ops.smart_ops.block_enable) {
		ISP_LOGE("fail to dlsym smart_ops.block_enable");
		goto error_dlsym;
	}
	cxt->ops.smart_ops.NR_disable = dlsym(cxt->ispalg_lib_handle, "smart_ctl_NR_block_disable");
	if (!cxt->ops.smart_ops.NR_disable) {
		ISP_LOGE("fail to dlsym smart_ops.NR_disable");
		goto error_dlsym;
	}

	cxt->ops.lsc_ops.init = dlsym(cxt->ispalg_lib_handle, "lsc_ctrl_init");
	if (!cxt->ops.lsc_ops.init) {
		ISP_LOGE("fail to dlsym lsc_ops.init");
		goto error_dlsym;
	}
	cxt->ops.lsc_ops.deinit = dlsym(cxt->ispalg_lib_handle, "lsc_ctrl_deinit");
	if (!cxt->ops.lsc_ops.deinit) {
		ISP_LOGE("fail to dlsym lsc_ops.deinit");
		goto error_dlsym;
	}
	cxt->ops.lsc_ops.process = dlsym(cxt->ispalg_lib_handle, "lsc_ctrl_process");
	if (!cxt->ops.lsc_ops.process) {
		ISP_LOGE("fail to dlsym lsc_ops.process");
		goto error_dlsym;
	}
	cxt->ops.lsc_ops.ioctrl = dlsym(cxt->ispalg_lib_handle, "lsc_ctrl_ioctrl");
	if (!cxt->ops.lsc_ops.ioctrl) {
		ISP_LOGE("fail to dlsym lsc_ops.ioctrl");
		goto error_dlsym;
	}

	cxt->ops.tof_ops.init = dlsym(cxt->ispalg_lib_handle, "tof_ctrl_init");
	if (!cxt->ops.tof_ops.init) {
		ISP_LOGE("fail to dlsym tof_ops.init");
		goto error_dlsym;
	}
	cxt->ops.tof_ops.deinit = dlsym(cxt->ispalg_lib_handle, "tof_ctrl_deinit");
	if (!cxt->ops.tof_ops.deinit) {
		ISP_LOGE("fail to dlsym tof_ops.deinit");
		goto error_dlsym;
	}

	cxt->ops.ai_ops.init = dlsym(cxt->ispalg_lib_handle, "ai_ctrl_init");
	if (!cxt->ops.ai_ops.init) {
		ISP_LOGE("fail to dlsym ai_ops.init");
		goto error_dlsym;
	}
	cxt->ops.ai_ops.deinit = dlsym(cxt->ispalg_lib_handle, "ai_ctrl_deinit");
	if (!cxt->ops.ai_ops.deinit) {
		ISP_LOGE("fail to dlsym ai_ops.deinit");
		goto error_dlsym;
	}

	ISP_LOGD("cam%ld done\n", cxt->camera_id);
	return 0;
error_dlsym:
#if 0
    dlclose(cxt->ispalg_lib_handle);
#else
    put_lib_handle(cxt->ispalg_lib_handle);
#endif
	cxt->ispalg_lib_handle = NULL;
error_dlopen:

	return ret;
}

/************************* new asyn init ****************/
static void * ispalg_libops_init_thread(void *data)
{
	cmr_int ret = ISP_SUCCESS;
	struct fw_init_local *init_cxt = (struct fw_init_local *)data;
	struct isp_alg_fw_context *cxt  = init_cxt->cxt;;

	ret = ispalg_load_library(cxt);

	if (ret)
		init_cxt->lib_init_status = FW_INIT_ERR;
	else
		init_cxt->lib_init_status = FW_INIT_DONE;
	return NULL;
}

cmr_int ispalg_aithread_proc(struct cmr_msg *message, void *p_data)
{
	cmr_int ret = ISP_SUCCESS;
	enum isp_ctrl_cmd cmd = ISP_CTRL_MAX;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)p_data;
	isp_io_fun io_ctrl = NULL;

	if (!message || !p_data) {
		ISP_LOGE("fail to check input param ");
		goto exit;
	}

	ISP_LOGD("cam%ld ai_msg_type %d, %d, data %p", cxt->camera_id,
		message->msg_type, message->sub_msg_type, message->data);

	switch (message->msg_type) {
	case AI_CMD_INIT:
		ret = ispalg_ai_init(cxt);
		break;
	case AI_CMD_START:
		cmd = ISP_CTRL_AI_PROCESS_START;
		break;
	case AI_CMD_STOP:
		cmd = ISP_CTRL_AI_PROCESS_STOP;
		break;
	case AI_CMD_WAIT:
		ISP_LOGI("cam%ld wait here\n", cxt->camera_id);
		break;
	default:
		ISP_LOGV("don't support msg");
		break;
	}

	if (cmd != ISP_CTRL_MAX)
		io_ctrl = isp_ioctl_get_fun(cmd);

	if (io_ctrl) {
		ret = io_ctrl(cxt, message->data);
		ISP_LOGD("cam%ld ai ctl_cmd %d, ret %ld, data %p, val %d\n", cxt->camera_id,
			cmd, ret, message->data, *(cmr_u32 *)message->data);
		if (NULL != cxt->commn_cxt.callback || !message->sub_msg_type) {
			cxt->commn_cxt.callback(cxt->commn_cxt.caller_id,
			ISP_CALLBACK_EVT | ISP_CTRL_CALLBACK | cmd, NULL, ISP_ZERO);
		}
	}

exit:
	ISP_LOGD("done %ld", ret);
	return ret;
}

static void * ispalg_ae_init_thread(void *data)
{
	cmr_int ret = ISP_SUCCESS;
	struct fw_init_local *init_cxt = (struct fw_init_local *)data;
	struct isp_alg_fw_context *cxt  = init_cxt->cxt;;

	ret = ispalg_ae_init(cxt);

	if (ret)
		init_cxt->ae_init_status = FW_INIT_ERR;
	else
		init_cxt->ae_init_status = FW_INIT_DONE;
	return NULL;
}

static void * ispalg_af_init_thread(void *data)
{
	cmr_int ret = ISP_SUCCESS;
	struct fw_init_local *init_cxt = (struct fw_init_local *)data;
	struct isp_alg_fw_context *cxt  = init_cxt->cxt;;

	ret = ispalg_af_init(cxt);

	if (ret)
		init_cxt->af_init_status = FW_INIT_ERR;
	else
		init_cxt->af_init_status = FW_INIT_DONE;
	return NULL;
}

static void * ispalg_afl_init_thread(void *data)
{
	cmr_int ret = ISP_SUCCESS;
	struct fw_init_local *init_cxt = (struct fw_init_local *)data;
	struct isp_alg_fw_context *cxt  = init_cxt->cxt;;

	ret = ispalg_afl_init(cxt);

	if (ret)
		init_cxt->afl_init_status = FW_INIT_ERR;
	else
		init_cxt->afl_init_status = FW_INIT_DONE;
	return NULL;
}

static void * ispalg_awb_init_thread(void *data)
{
	cmr_int ret = ISP_SUCCESS;
	struct fw_init_local *init_cxt = (struct fw_init_local *)data;
	struct isp_alg_fw_context *cxt  = init_cxt->cxt;;

	ret = ispalg_awb_init(cxt);

	if (ret)
		init_cxt->awb_init_status = FW_INIT_ERR;
	else
		init_cxt->awb_init_status = FW_INIT_DONE;
	return NULL;
}

static void * ispalg_lsc_init_thread(void *data)
{
	cmr_int ret = ISP_SUCCESS;
	struct fw_init_local *init_cxt = (struct fw_init_local *)data;
	struct isp_alg_fw_context *cxt  = init_cxt->cxt;;

	ret = ispalg_lsc_init(cxt);

	if (ret)
		init_cxt->lsc_init_status = FW_INIT_ERR;
	else
		init_cxt->lsc_init_status = FW_INIT_DONE;
	return NULL;
}

static void * ispalg_smart_init_thread(void *data)
{
	cmr_int ret = ISP_SUCCESS;
	struct fw_init_local *init_cxt = (struct fw_init_local *)data;
	struct isp_alg_fw_context *cxt  = init_cxt->cxt;;

	ret = ispalg_smart_init(cxt);

	if (ret)
		init_cxt->smart_init_status = FW_INIT_ERR;
	else
		init_cxt->smart_init_status = FW_INIT_DONE;
	return NULL;
}

static void * ispalg_pdaf_init_thread(void *data)
{
	cmr_int ret = ISP_SUCCESS;
	struct fw_init_local *init_cxt = (struct fw_init_local *)data;
	struct isp_alg_fw_context *cxt  = init_cxt->cxt;;

	ret = ispalg_pdaf_init(cxt);

	if (ret)
		init_cxt->pdaf_init_status = FW_INIT_ERR;
	else
		init_cxt->pdaf_init_status = FW_INIT_DONE;
	return NULL;
}

static void * ispalg_tof_init_thread(void *data)
{
	cmr_int ret = ISP_SUCCESS;
	struct fw_init_local *init_cxt = (struct fw_init_local *)data;
	struct isp_alg_fw_context *cxt  = init_cxt->cxt;;

	ret = ispalg_tof_init(cxt);

	if (ret)
		init_cxt->tof_init_status = FW_INIT_ERR;
	else
		init_cxt->tof_init_status = FW_INIT_DONE;
	return NULL;
}

static cmr_u32 ispalg_init_async(struct fw_init_local *init_cxt)
{
	cmr_int ret = ISP_SUCCESS;
	pthread_attr_t attr;
	struct isp_alg_fw_context *cxt = init_cxt->cxt;

	/*  AE */
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	ret = pthread_create(&init_cxt->ae_thread_handle, &attr,
							ispalg_ae_init_thread,
							(void *)init_cxt);
	pthread_setname_np(init_cxt->ae_thread_handle, "isp_ae_init");
	pthread_attr_destroy(&attr);
	if (ret) {
		ISP_LOGE("cam%ld fail to create ae thread %ld\n", cxt->camera_id, ret);
		init_cxt->ae_init_status = FW_INIT_ERR;
		init_cxt->ae_thread_handle = 0;
	}

	/*  SMART */
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	ret = pthread_create(&init_cxt->smart_thread_handle, &attr,
							ispalg_smart_init_thread,
							(void *)init_cxt);
	pthread_setname_np(init_cxt->smart_thread_handle, "isp_smart_init");
	pthread_attr_destroy(&attr);
	if (ret) {
		ISP_LOGE("cam%ld fail to create smart thread %ld\n", cxt->camera_id, ret);
		init_cxt->smart_init_status = FW_INIT_ERR;
		init_cxt->smart_thread_handle = 0;
	}

	/*  LSC */
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	ret = pthread_create(&init_cxt->lsc_thread_handle, &attr,
							ispalg_lsc_init_thread,
							(void *)init_cxt);
	pthread_setname_np(init_cxt->lsc_thread_handle, "isp_lsc_init");
	pthread_attr_destroy(&attr);
	if (ret) {
		ISP_LOGE("cam%ld fail to create lsc thread %ld\n", cxt->camera_id, ret);
		init_cxt->lsc_init_status = FW_INIT_ERR;
		init_cxt->lsc_thread_handle = 0;
	}

	/*  AF */
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	ret = pthread_create(&init_cxt->af_thread_handle, &attr,
							ispalg_af_init_thread,
							(void *)init_cxt);
	pthread_setname_np(init_cxt->af_thread_handle, "isp_af_init");
	pthread_attr_destroy(&attr);
	if (ret) {
		ISP_LOGE("cam%ld fail to create af thread %ld\n", cxt->camera_id, ret);
		init_cxt->af_init_status = FW_INIT_ERR;
		init_cxt->af_thread_handle = 0;
	}

	/*  AWB */
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	ret = pthread_create(&init_cxt->awb_thread_handle, &attr,
							ispalg_awb_init_thread,
							(void *)init_cxt);
	pthread_setname_np(init_cxt->awb_thread_handle, "isp_awb_init");
	pthread_attr_destroy(&attr);
	if (ret) {
		ISP_LOGE("cam%ld fail to create awb thread %ld\n", cxt->camera_id, ret);
		init_cxt->awb_init_status = FW_INIT_ERR;
		init_cxt->awb_thread_handle = 0;
	}

	/*  PDAF */
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	ret = pthread_create(&init_cxt->pdaf_thread_handle, &attr,
							ispalg_pdaf_init_thread,
							(void *)init_cxt);
	pthread_setname_np(init_cxt->pdaf_thread_handle, "isp_pdaf_init");
	pthread_attr_destroy(&attr);
	if (ret) {
		ISP_LOGE("cam%ld fail to create pdaf thread %ld\n", cxt->camera_id, ret);
		init_cxt->pdaf_init_status = FW_INIT_ERR;
		init_cxt->pdaf_thread_handle = 0;
	}

	/*  AFL */
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	ret = pthread_create(&init_cxt->afl_thread_handle, &attr,
							ispalg_afl_init_thread,
							(void *)init_cxt);
	pthread_setname_np(init_cxt->afl_thread_handle, "isp_afl_init");
	pthread_attr_destroy(&attr);
	if (ret) {
		ISP_LOGE("cam%ld fail to create afl thread %ld\n", cxt->camera_id, ret);
		init_cxt->afl_init_status = FW_INIT_ERR;
		init_cxt->afl_thread_handle = 0;
	}

	/*  TOF */
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	ret = pthread_create(&init_cxt->tof_thread_handle, &attr,
							ispalg_tof_init_thread,
							(void *)init_cxt);
	pthread_setname_np(init_cxt->tof_thread_handle, "isp_tof_init");
	pthread_attr_destroy(&attr);
	if (ret) {
		ISP_LOGE("cam%ld fail to create tof thread %ld\n", cxt->camera_id, ret);
		init_cxt->tof_init_status = FW_INIT_ERR;
		init_cxt->tof_thread_handle = 0;
	}

	ispalg_bypass_init(cxt);

	ISP_LOGV("cam%ld done\n", cxt->camera_id);
	return 0;
}

static cmr_u32 ispalg_init_async_done(struct fw_init_local *init_cxt)
{
	void *dummy;

	if (init_cxt->ae_thread_handle) {
		pthread_join(init_cxt->ae_thread_handle, &dummy);
		init_cxt->ae_thread_handle = 0;
	}

	if (init_cxt->af_thread_handle) {
		pthread_join(init_cxt->af_thread_handle, &dummy);
		init_cxt->af_thread_handle = 0;
	}

	if (init_cxt->afl_thread_handle) {
		pthread_join(init_cxt->afl_thread_handle, &dummy);
		init_cxt->afl_thread_handle = 0;
	}

	if (init_cxt->awb_thread_handle) {
		pthread_join(init_cxt->awb_thread_handle, &dummy);
		init_cxt->awb_thread_handle = 0;
	}

	if (init_cxt->smart_thread_handle) {
		pthread_join(init_cxt->smart_thread_handle, &dummy);
		init_cxt->smart_thread_handle = 0;
	}

	if (init_cxt->lsc_thread_handle) {
		pthread_join(init_cxt->lsc_thread_handle, &dummy);
		init_cxt->lsc_thread_handle = 0;
	}

	if (init_cxt->pdaf_thread_handle) {
		pthread_join(init_cxt->pdaf_thread_handle, &dummy);
		init_cxt->pdaf_thread_handle = 0;
	}

	if (init_cxt->tof_thread_handle) {
		pthread_join(init_cxt->tof_thread_handle, &dummy);
		init_cxt->tof_thread_handle = 0;
	}

	return 0;
}

static void * isp_alg_fw_init_async(void *data)
{
	cmr_int ret = ISP_SUCCESS;
	void *dummy;
	pthread_attr_t attr;
	struct fw_init_local *init_cxt = (struct fw_init_local *)data;
	struct isp_alg_fw_context *cxt;

	if (data == NULL || init_cxt->cxt == NULL) {
		ISP_LOGE("error: NULL ptr\n");
		return NULL;
	}

	cxt = init_cxt->cxt;
	ISP_LOGD("cam%ld enter\n", cxt->camera_id);

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	ret = pthread_create(&init_cxt->lib_thread_handle, &attr,
						ispalg_libops_init_thread,
						(void *)init_cxt);
	pthread_setname_np(init_cxt->lib_thread_handle, "load_isplib");
	pthread_attr_destroy(&attr);
	if (ret) {
		init_cxt->lib_thread_handle = 0;
		ISP_LOGE("cam%ld fail to create init thread %ld\n", cxt->camera_id, ret);
		goto exit;
	}

	ret = ispalg_pm_init(init_cxt);
	if (ISP_SUCCESS != ret) {
		ISP_LOGE("fail to do ispalg_pm_init");
		goto exit;
	}

	ret = isp_br_init(cxt->camera_id, cxt, cxt->is_master);
	if (ret) {
		ISP_LOGE("fail to init isp bridge");
		goto exit;
	}

	if (cxt->is_multi_mode) {
		cmr_u32 id = cxt->camera_id;
		cmr_u32 role = CAM_SENSOR_MAX;

		ret = isp_br_ioctrl(CAM_SENSOR_MASTER, GET_SENSOR_ROLE, &id, &role);
		if (ret) {
			ISP_LOGE("fail to get sensor role");
			goto exit;
		}

		cxt->sensor_role = role;
		ISP_LOGD("cam%ld sensor_role %u", cxt->camera_id, role);
	}

	ISP_LOGD("cam%ld wait libload\n", cxt->camera_id);
	/* sync with libalg loading done */
	if (init_cxt->lib_thread_handle) {
		pthread_join(init_cxt->lib_thread_handle, &dummy);
		init_cxt->lib_thread_handle = 0;
	}

	if (init_cxt->lib_init_status == FW_INIT_ERR) {
		ISP_LOGE("cam%ld load libalg error\n", cxt->camera_id);
		goto exit;
	}
	ISP_LOGD("cam%ld wait libload done\n", cxt->camera_id);

	ispalg_init_async(init_cxt);

	ret = ispalg_create_thread((cmr_handle)cxt);
	if (ret) {
		ISP_LOGE("fail to create thread.\n");
		goto exit;
	}
	isp_dev_access_evt_reg(cxt->dev_access_handle, ispalg_dev_evt_msg, (cmr_handle) cxt);

	pthread_mutex_init(&cxt->debuginfo_queue_lock, NULL);
	if (s_dbg_ver) {
		debuginfo_q_alloc(&cxt->ae_queue);
		debuginfo_q_alloc(&cxt->alsc_queue);
		debuginfo_q_alloc(&cxt->ai_queue);
		debuginfo_q_alloc(&cxt->awb_queue);
		debuginfo_q_alloc(&cxt->af_queue);
		debuginfo_q_alloc(&cxt->aft_queue);
		debuginfo_q_alloc(&cxt->smart_queue);
	}

	ISP_LOGD("cam%ld wait isp alg\n", cxt->camera_id);

	/* sync with ispalg all done */
	ispalg_init_async_done(init_cxt);

	ISP_LOGD("cam%ld wait isp alg done\n", cxt->camera_id);

	cxt->init_status = FW_INIT_DONE;
	ISP_LOGI("cam%ld done, alg enable %d, init %d\n", cxt->camera_id, cxt->alg_enable, cxt->init_status);
	return NULL;

exit:
	if (init_cxt->lib_thread_handle) {
		pthread_join(init_cxt->lib_thread_handle, &dummy);
		init_cxt->lib_thread_handle = 0;
	}
	ispalg_init_async_done(init_cxt);
	cxt->init_status = FW_INIT_ERR;
	ISP_LOGE("cam%ld init error\n", cxt->camera_id);
	return NULL;
}

static cmr_u32 isp_alg_fw_init_async_done(struct isp_alg_fw_context *cxt)
{
	void *dummy;
	struct fw_init_local *init_cxt = cxt->init_cxt;

	if (init_cxt == NULL)
		return 0;

	if (init_cxt->init_thread_handle) {
		pthread_join(init_cxt->init_thread_handle, &dummy);
		init_cxt->init_thread_handle = 0;
	}
	if (init_cxt->lib_thread_handle) {
		pthread_join(init_cxt->lib_thread_handle, &dummy);
		init_cxt->lib_thread_handle = 0;
	}
	ispalg_init_async_done(init_cxt);

	free(cxt->init_cxt);
	cxt->init_cxt = NULL;

	return 0;
}
/************************* new asyn init  end ****************/


static cmr_int ispalg_ae_set_work_mode(
		cmr_handle isp_alg_handle, cmr_u32 new_mode,
		cmr_u32 fly_mode, struct isp_video_start *param_ptr)
{
	cmr_s32 ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct ae_set_work_param ae_param;
	enum ae_work_mode ae_mode = 0;
	struct isp_pm_ioctl_output ioctl_output = { PNULL, 0 };
	struct isp_pm_ioctl_input ioctl_input;
	struct isp_pm_param_data ioctl_data;
	struct isp_rgb_aem_info aem_info;
	cmr_u32 max_fps = 0;

	memset(&ae_param, 0, sizeof(ae_param));

	switch (new_mode) {
	case ISP_MODE_ID_PRV_0:
	case ISP_MODE_ID_PRV_1:
	case ISP_MODE_ID_PRV_2:
	case ISP_MODE_ID_PRV_3:
		ae_mode = AE_WORK_MODE_COMMON;
		break;

	case ISP_MODE_ID_CAP_0:
	case ISP_MODE_ID_CAP_1:
	case ISP_MODE_ID_CAP_2:
	case ISP_MODE_ID_CAP_3:
		ae_mode = AE_WORK_MODE_CAPTURE;
		break;

	case ISP_MODE_ID_VIDEO_0:
	case ISP_MODE_ID_VIDEO_1:
	case ISP_MODE_ID_VIDEO_2:
	case ISP_MODE_ID_VIDEO_3:
		ae_mode = AE_WORK_MODE_VIDEO;
		break;

	default:
		break;
	}

	ae_param.mode = ae_mode;
	ae_param.fly_eb = fly_mode;
	ae_param.highflash_measure.highflash_flag = param_ptr->is_need_flash;
	ae_param.highflash_measure.capture_skip_num = param_ptr->capture_skip_num;
	ae_param.capture_skip_num = param_ptr->capture_skip_num;
	/* ae_param.zsl_flag = param_ptr->capture_mode; */
	ae_param.resolution_info.frame_size.w = cxt->commn_cxt.prv_size.w;
	ae_param.resolution_info.frame_size.h = cxt->commn_cxt.prv_size.h;
	ae_param.resolution_info.frame_line = cxt->commn_cxt.input_size_trim[cxt->commn_cxt.param_index].frame_line;
	ae_param.resolution_info.line_time = cxt->commn_cxt.input_size_trim[cxt->commn_cxt.param_index].line_time;
	ae_param.resolution_info.sensor_size_index = cxt->commn_cxt.param_index;
	ae_param.resolution_info.snr_setting_max_fps = param_ptr->sensor_fps.max_fps;
	ae_param.is_snapshot = param_ptr->is_snapshot;
	ae_param.dv_mode = param_ptr->dv_mode;

	ae_param.sensor_fps.mode = param_ptr->sensor_fps.mode;
	ae_param.sensor_fps.max_fps = param_ptr->sensor_fps.max_fps;
	ae_param.sensor_fps.min_fps = param_ptr->sensor_fps.min_fps;
	ae_param.sensor_fps.is_high_fps = param_ptr->sensor_fps.is_high_fps;
	ae_param.sensor_fps.high_fps_skip_num = param_ptr->sensor_fps.high_fps_skip_num;

	ret = ispalg_get_aem_param(cxt, &aem_info);
	cxt->ae_cxt.win_num.w = aem_info.blk_num.w;
	cxt->ae_cxt.win_num.h = aem_info.blk_num.h;

	ae_param.win_num.h = cxt->ae_cxt.win_num.h;
	ae_param.win_num.w = cxt->ae_cxt.win_num.w;
	ae_param.blk_num = ae_param.win_num;
	ae_param.cam_4in1_mode = cxt->is_4in1_sensor;
	ae_param.zsl_flag = cxt->zsl_flag;

	ae_param.win_size.w = ((ae_param.resolution_info.frame_size.w / ae_param.win_num.w) / 2) * 2;
	ae_param.win_size.h = ((ae_param.resolution_info.frame_size.h / ae_param.win_num.h) / 2) * 2;
	ae_param.shift = 0; /* no shift in AEM block of sharkl5/roc1*/
	cxt->ae_cxt.shift = ae_param.shift;
	memset(&ioctl_data, 0, sizeof(ioctl_data));
	BLOCK_PARAM_CFG(ioctl_input, ioctl_data,
			ISP_PM_BLK_ISP_SETTING,
			ISP_BLK_AE_ADAPT_PARAM, PNULL, 0);
	ret = isp_pm_ioctl(cxt->handle_pm,
			ISP_PM_CMD_GET_SINGLE_SETTING,
			(void *)&ioctl_input, (void *)&ioctl_output);
	if (ioctl_output.param_num == 1 && ioctl_output.param_data && ioctl_output.param_data->data_ptr){
		struct isp_ae_adapt_info *data;
		data  = ioctl_output.param_data->data_ptr;
		ae_param.binning_factor = data->binning_factor;
		ISP_LOGD("binning_factor = %d\n", ae_param.binning_factor);
	}

	if (cxt->ops.awb_ops.ioctrl) {
		ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle,
				AWB_CTRL_CMD_GET_CT_TABLE20, NULL, (void *)&ae_param.ct_table);
		ISP_TRACE_IF_FAIL(ret, ("fail to AWB_CTRL_CMD_GET_CT_TABLE20"));
	}

	if (cxt->ops.ae_ops.ioctrl) {
		ISP_LOGD("trigger ae start.");
		ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_VIDEO_START, &ae_param, NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to AE_VIDEO_START"));
	}

	max_fps = ae_param.sensor_fps.max_fps;
	if (cxt->ops.afl_ops.ioctrl) {
		ret = cxt->ops.afl_ops.ioctrl(cxt->afl_cxt.handle, AFL_SET_MAX_FPS, (void *)&max_fps, NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to AFL_MAX_FPS"));
	}

	return ret;
}

static cmr_int ispalg_update_alg_param(cmr_handle isp_alg_handle)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_u32 ct = 0;
	cmr_s32 bv = 0;
	cmr_s32 bv_gain = 0;
	cmr_u32 cfg_set_id;
	int i, num;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct smart_proc_input smart_proc_in;
	struct awb_gain result;
	struct isp_pm_ioctl_input ioctl_output = { PNULL, 0 };
	struct isp_pm_ioctl_input ioctl_input;
	struct isp_pm_param_data ioctl_data;
	struct isp_awbc_cfg awbc_cfg;

	if (cxt->takepicture_mode == CAMERA_ISP_SIMULATION_MODE) {
		awbc_cfg.r_gain = cxt->simu_param.awb_gain_r;
		awbc_cfg.g_gain = cxt->simu_param.awb_gain_g;
		awbc_cfg.b_gain = cxt->simu_param.awb_gain_b;
		awbc_cfg.r_offset = 0;
		awbc_cfg.g_offset = 0;
		awbc_cfg.b_offset = 0;
		ct = cxt->simu_param.smart_ct;
		bv = cxt->simu_param.smart_bv;
		bv_gain = cxt->simu_param.gain;
	} else {
		cxt->aem_is_update = 0;
		memset(&result, 0, sizeof(result));
		if (cxt->ops.awb_ops.ioctrl) {
			ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_READ_FILE, NULL, NULL);
			ISP_TRACE_IF_FAIL(ret, ("fail to read awb.file"));
			ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_GET_GAIN, (void *)&result, NULL);
			ISP_TRACE_IF_FAIL(ret, ("fail to AWB_CTRL_CMD_GET_GAIN"));
		}
		awbc_cfg.r_gain = result.r;
		awbc_cfg.g_gain = result.g;
		awbc_cfg.b_gain = result.b;
		awbc_cfg.r_offset = 0;
		awbc_cfg.g_offset = 0;
		awbc_cfg.b_offset = 0;

		if (cxt->ops.awb_ops.ioctrl) {
			ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_GET_CT, (void *)&ct, NULL);
			ISP_TRACE_IF_FAIL(ret, ("fail to AWB_CTRL_CMD_GET_CT"));
		}
		if (cxt->ops.ae_ops.ioctrl) {
			ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_GET_BV_BY_LUM_NEW, NULL, (void *)&bv);
			ISP_TRACE_IF_FAIL(ret, ("fail to AE_GET_BV_BY_LUM_NEW"));
			ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_GET_BV_BY_GAIN, NULL, (void *)&bv_gain);
			ISP_TRACE_IF_FAIL(ret, ("fail to AE_GET_BV_BY_GAIN"));
		}
	}

	BLOCK_PARAM_CFG(ioctl_input, ioctl_data,
			ISP_PM_BLK_AWBC, ISP_BLK_AWB_NEW,
			&awbc_cfg,  sizeof(awbc_cfg));
	ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_AWB, (void *)&ioctl_input, NULL);

	cxt->smart_cxt.tunning_gamma_cur[0] = NULL;
	cxt->smart_cxt.tunning_gamma_cur[1] = NULL;
	cxt->smart_cxt.tunning_gamma_cur[2] = NULL;

	num = (cxt->zsl_flag) ? 2 : 1;
	for (i = 0; i < num; i++) {
		cfg_set_id = (i == 0) ? PARAM_SET0 : PARAM_SET1;
		memset(&ioctl_data, 0, sizeof(ioctl_data));
		BLOCK_PARAM_CFG(ioctl_input, ioctl_data,
				ISP_PM_BLK_GAMMA_TAB,
				ISP_BLK_RGB_GAMC,
				&cfg_set_id, sizeof(cfg_set_id));
		ret = isp_pm_ioctl(cxt->handle_pm,
				ISP_PM_CMD_GET_SINGLE_SETTING,
				(void *)&ioctl_input, (void *)&ioctl_output);
		ISP_TRACE_IF_FAIL(ret, ("fail to get GAMMA TAB"));
		if (ioctl_output.param_num == 1 && ioctl_output.param_data_ptr) {
			cxt->smart_cxt.tunning_gamma_cur[i] = ioctl_output.param_data_ptr->data_ptr;
			ISP_LOGD("get gamma tab %p for set %d\n",cxt->smart_cxt.tunning_gamma_cur[i], i);
		} else {
			ISP_LOGE("fail to get gamma tab %p for set %d\n",cxt->smart_cxt.tunning_gamma_cur[i], i);
		}
	}

	if (cxt->is_simulator)
		return ret;

	memset(&smart_proc_in, 0, sizeof(smart_proc_in));
	if ((cxt->smart_cxt.sw_bypass == 0) && (0 != bv_gain) && (0 != ct)) {
		smart_proc_in.cal_para.bv = bv;
		smart_proc_in.cal_para.bv_gain = bv_gain;
		smart_proc_in.cal_para.ct = ct;
		smart_proc_in.alc_awb = cxt->awb_cxt.alc_awb;
		smart_proc_in.scene_flag = cxt->commn_cxt.nr_scene_flag;
		smart_proc_in.ai_scene_id = cxt->commn_cxt.ai_scene_id;
		isp_prepare_atm_param(isp_alg_handle, &smart_proc_in);
		for (i = 0; i < num; i++) {
			cxt->smart_cxt.cur_set_id = i;
			smart_proc_in.mode_flag = cxt->commn_cxt.isp_pm_mode[i];
			smart_proc_in.cal_para.gamma_tab = cxt->smart_cxt.tunning_gamma_cur[i];
			if (cxt->ops.smart_ops.ioctrl) {
				ret = cxt->ops.smart_ops.ioctrl(cxt->smart_cxt.handle,
							ISP_SMART_IOCTL_SET_WORK_MODE,
							(void *)&smart_proc_in.mode_flag, NULL);
				ISP_TRACE_IF_FAIL(ret, ("fail to ISP_SMART_IOCTL_SET_WORK_MODE"));
			}
			if (cxt->ops.smart_ops.calc)
				ret = cxt->ops.smart_ops.calc(cxt->smart_cxt.handle, &smart_proc_in);
		}
	}

	return ret;
}

static cmr_int ispalg_update_smart_param(cmr_handle isp_alg_handle)
{

	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct smart_proc_input smart_proc_in;
	struct isp_pm_ioctl_input ioctl_output = { PNULL, 0 };
	struct isp_pm_ioctl_input ioctl_input;
	struct isp_pm_param_data ioctl_data;
	struct isptool_scene_param scene_param;

	memset(&smart_proc_in, 0, sizeof(smart_proc_in));
	memset(&scene_param, 0, sizeof(scene_param));

	memset(&ioctl_data, 0, sizeof(ioctl_data));
	BLOCK_PARAM_CFG(ioctl_input, ioctl_data,
			ISP_PM_BLK_GAMMA_TAB,
			ISP_BLK_RGB_GAMC, PNULL, 0);
	ret = isp_pm_ioctl(cxt->handle_pm,
			ISP_PM_CMD_GET_SINGLE_SETTING,
			(void *)&ioctl_input, (void *)&ioctl_output);
	ISP_TRACE_IF_FAIL(ret, ("fail to get GAMMA TAB"));
	if (ioctl_output.param_num == 1 && ioctl_output.param_data_ptr)
		cxt->smart_cxt.tunning_gamma_cur[0] = ioctl_output.param_data_ptr->data_ptr;

	isp_sim_get_scene_parm(&scene_param);

	if ((cxt->smart_cxt.sw_bypass == 0) && (0 != scene_param.gain) && (0 != scene_param.smart_ct)) {
		smart_proc_in.cal_para.bv = scene_param.smart_bv;
		smart_proc_in.cal_para.abl_weight= scene_param.abl_weight;
		smart_proc_in.cal_para.bv_gain = scene_param.gain;
		smart_proc_in.cal_para.ct = scene_param.smart_ct;
		smart_proc_in.alc_awb = cxt->awb_cxt.alc_awb;
		smart_proc_in.scene_flag = cxt->commn_cxt.nr_scene_flag;
		smart_proc_in.cal_para.gamma_tab = cxt->smart_cxt.tunning_gamma_cur[0];
		isp_prepare_atm_param(isp_alg_handle, &smart_proc_in);
		cxt->smart_cxt.cur_set_id = 0;

		ISP_LOGI("bv=%d, abl_weight=%d, bv_gain=%d, ct=%d, alc_awb=%d, mode_flag=%d, nr_scene_flag=%d\n",
			smart_proc_in.cal_para.bv,
			smart_proc_in.cal_para.abl_weight,
			smart_proc_in.cal_para.bv_gain,
			smart_proc_in.cal_para.ct,
			smart_proc_in.alc_awb,
			smart_proc_in.mode_flag,
			smart_proc_in.scene_flag);
		smart_proc_in.mode_flag = cxt->commn_cxt.isp_pm_mode[0];
		if (cxt->ops.smart_ops.ioctrl) {
			ret = cxt->ops.smart_ops.ioctrl(cxt->smart_cxt.handle,
						ISP_SMART_IOCTL_SET_WORK_MODE,
						(void *)&smart_proc_in.mode_flag, NULL);
			ISP_TRACE_IF_FAIL(ret, ("fail to ISP_SMART_IOCTL_SET_WORK_MODE"));
		}
		if (cxt->ops.smart_ops.calc)
			ret = cxt->ops.smart_ops.calc(cxt->smart_cxt.handle, &smart_proc_in);
	}

	return ret;

}

static cmr_int ispalg_update_alsc_result(cmr_handle isp_alg_handle, cmr_handle out_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	enum isp_pm_cmd  cmd;
	cmr_s32 i =0;
	cmr_s32 lsc_result_size = 0;
	cmr_u16 *lsc_result_address_new = NULL;
	struct isp_size pic_size;
	struct isp_2d_lsc_param *lsc_tab_param_ptr;
	struct isp_lsc_info *lsc_info_new;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct alsc_fwstart_info *fwstart_info = (struct alsc_fwstart_info *)out_ptr;
	struct isp_pm_ioctl_input input = { PNULL, 0 };
	struct isp_pm_ioctl_input input2 = { PNULL, 0 };
	struct isp_pm_param_data pm_param;
	struct isp_pm_param_data param_data_grid;
	cmr_u32 adaptive_size_info[3] = {0, 0, 0};

	if (cxt->lsc_cxt.sw_bypass)
		return 0;

	pic_size = cxt->commn_cxt.prv_size;
	fwstart_info->img_width_new  = cxt->commn_cxt.prv_size.w;
	fwstart_info->img_height_new = cxt->commn_cxt.prv_size.h;

	memset(&input, 0, sizeof(struct isp_pm_ioctl_input));
	memset(&pm_param, 0, sizeof(pm_param));

	ret = ispalg_alsc_get_info(cxt);
	ISP_RETURN_IF_FAIL(ret, ("fail to do get lsc info"));
	lsc_tab_param_ptr = (struct isp_2d_lsc_param *)(cxt->lsc_cxt.lsc_tab_address);
	lsc_info_new = (struct isp_lsc_info *)cxt->lsc_cxt.lsc_info;

	ISP_LOGD("size0 (%d, %d), size1 (%d %d), lsc size (%d %d),  grid %d, gain_w %d gain_h %d\n",
		pic_size.w, pic_size.h, cxt->commn_cxt.src.w, cxt->commn_cxt.src.h,
		lsc_tab_param_ptr->resolution.w, lsc_tab_param_ptr->resolution.h,
		lsc_info_new->grid, lsc_info_new->gain_w, lsc_info_new->gain_h);

	for (i = 0; (i < (cxt->zsl_flag ? 2 : 1)) && (cxt->lsc_cxt.LSC_SPD_VERSION >= 6); i++) {

		if (i == 0) {
			pic_size = cxt->commn_cxt.prv_size;
			cmd = ISP_PM_CMD_SET_GRID0;
		} else {
			pic_size = cxt->commn_cxt.src;
			cmd = ISP_PM_CMD_SET_GRID1;
		}

		ISP_LOGI("[ALSC] lsc_pm_normalization, new_image_size[%d,%d], full_image_size[%d,%d]",
			pic_size.w, pic_size.h, cxt->lsc_cxt.full_size_width, cxt->lsc_cxt.full_size_height);

		if((cxt->lsc_cxt.full_size_width * pic_size.h) == (cxt->lsc_cxt.full_size_height * pic_size.w)){
			ISP_LOGI("[ALSC] lsc_pm_normalization case1, n binning");
			adaptive_size_info[0] = pic_size.w;
			adaptive_size_info[1] = pic_size.h;
			adaptive_size_info[2] = cxt->lsc_cxt.full_size_grid * pic_size.w / cxt->lsc_cxt.full_size_width;
			memset(&param_data_grid, 0, sizeof(param_data_grid));
			BLOCK_PARAM_CFG(input2, param_data_grid,
				ISP_PM_BLK_LSC_UPDATE_GRID,
				ISP_BLK_2D_LSC,
				&adaptive_size_info[0], 0);
			ret = isp_pm_ioctl(cxt->handle_pm, cmd, &input2, NULL);
		} else if ((pic_size.w == 1280) && (pic_size.h == 720)) {
			ISP_LOGI("[ALSC] lsc_pm_normalization case2. image size %d %d\n", pic_size.w, pic_size.h);
			adaptive_size_info[0] = cxt->commn_cxt.src.w;
			adaptive_size_info[1] = cxt->commn_cxt.src.h;
			adaptive_size_info[2] = 32;
			memset(&param_data_grid, 0, sizeof(param_data_grid));
			BLOCK_PARAM_CFG(input2, param_data_grid,
				ISP_PM_BLK_LSC_UPDATE_GRID,
				ISP_BLK_2D_LSC,
				&adaptive_size_info[0], 0);
			ret = isp_pm_ioctl(cxt->handle_pm, cmd, &input2, NULL);
		} else  if ((pic_size.w == 1920) && (pic_size.h == 1080)) {
			ISP_LOGI("[ALSC] lsc_pm_normalization case3. image size %d %d\n", pic_size.w, pic_size.h);
			adaptive_size_info[0] = cxt->commn_cxt.src.w;
			adaptive_size_info[1] = cxt->commn_cxt.src.h;
			adaptive_size_info[2] = 48;
			memset(&param_data_grid, 0, sizeof(param_data_grid));
			BLOCK_PARAM_CFG(input2, param_data_grid,
				ISP_PM_BLK_LSC_UPDATE_GRID,
				ISP_BLK_2D_LSC,
				&adaptive_size_info[0], 0);
			ret = isp_pm_ioctl(cxt->handle_pm, cmd, &input2, NULL);
		} else{
			ISP_LOGE("[ALSC] lsc_pm_normalization error");
		}
	}

	for (i = 0; i < 9; i++)
		fwstart_info->lsc_tab_address_new[i] = lsc_tab_param_ptr->map_tab[i].param_addr;
	fwstart_info->gain_width_new = lsc_info_new->gain_w;
	fwstart_info->gain_height_new = lsc_info_new->gain_h;
	fwstart_info->image_pattern_new = cxt->commn_cxt.image_pattern;
	fwstart_info->grid_new = lsc_info_new->grid;
	fwstart_info->camera_id = cxt->camera_id;

	lsc_result_size = lsc_info_new->gain_w * lsc_info_new->gain_h * 4 * sizeof(cmr_u16);
	lsc_result_address_new = (cmr_u16*)malloc(lsc_result_size);
	if (NULL == lsc_result_address_new) {
		ret = ISP_ALLOC_ERROR;
		ISP_LOGE("fail to alloc lsc_result_address_new");
		return ret;
	}
	fwstart_info->lsc_result_address_new = lsc_result_address_new;

	if (cxt->ops.lsc_ops.ioctrl) {
		ret = cxt->ops.lsc_ops.ioctrl(cxt->lsc_cxt.handle, ALSC_FW_START, (void *)fwstart_info, NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to ALSC_FW_START"));
		if(ret == ISP_SUCCESS){
			BLOCK_PARAM_CFG(input, pm_param,
					ISP_PM_BLK_LSC_MEM_ADDR,
					ISP_BLK_2D_LSC,
					lsc_result_address_new, lsc_result_size);
			ret = isp_pm_ioctl(cxt->handle_pm,
					ISP_PM_CMD_SET_OTHERS,
					&input, NULL);
		}
	}

	free(lsc_result_address_new);
	lsc_result_address_new = NULL;

	return ret;
}

cmr_int ispalg_calc_stats_size(cmr_handle isp_alg_handle)
{
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_stats_alloc_info *buf_info;
	cmr_u32 size0, size1;

	/* AEM max windows: 128 x 128, 3*16Bytes for each window */
	/* Alloc 128 more bytes(header) for pararmeters (aem win, zoom roi....) */
	buf_info = &cxt->stats_mem_info.buf_info[STATIS_AEM];
	buf_info->size = cxt->ae_cxt.win_num.w * cxt->ae_cxt.win_num.h * 16 * 3 + STATIS_AEM_HEADER_SIZE;
	buf_info->size += STATIS_TAIL_RESERVED_SIZE;
	buf_info->num = STATIS_AEM_BUF_NUM;

	/* for afm */
	buf_info = &cxt->stats_mem_info.buf_info[STATIS_AFM];
	buf_info->size = STATIS_AFM_BUF_SIZE;
	buf_info->size += STATIS_TAIL_RESERVED_SIZE;
	buf_info->num = STATIS_AFM_BUF_NUM;

	/* for lscm, size should be 0 if lscm_win is not set */
	buf_info = &cxt->stats_mem_info.buf_info[STATIS_LSCM];
	buf_info->size = cxt->lsc_cxt.lscm_win_num.w * cxt->lsc_cxt.lscm_win_num.h * 16;
	buf_info->size += STATIS_TAIL_RESERVED_SIZE;
	buf_info->num = STATIS_LSCM_BUF_NUM;

	/* for afl */
	buf_info = &cxt->stats_mem_info.buf_info[STATIS_AFL];
	buf_info->size = STATIS_AFL_SIZE;
	buf_info->size += STATIS_TAIL_RESERVED_SIZE;
	buf_info->num = STATIS_AFL_BUF_NUM;

	/* for bayer hist (sharkl3 and previous version have no bayer hist) */
	buf_info = &cxt->stats_mem_info.buf_info[STATIS_HIST];
	buf_info->size = STATIS_HIST_BUF_SIZE + STATIS_HIST_HEADER_SIZE;
	buf_info->size += STATIS_TAIL_RESERVED_SIZE;
	buf_info->num = STATIS_HIST_BUF_NUM;

	/* for isp yuv hist (only for SW write/read) */
	buf_info = &cxt->stats_mem_info.buf_info[STATIS_HIST2];
	buf_info->size = STATIS_ISP_HIST2_BUF_SIZE;
	buf_info->num = STATIS_ISP_HIST2_BUF_NUM;

	/* 3dnr: max size is (w/8 * 16 + h/8*16) bytes  */
	/* should save at least 5 mv buffer for capture */
	buf_info = &cxt->stats_mem_info.buf_info[STATIS_3DNR];
	buf_info->size = (cxt->commn_cxt.prv_size.w + 7) / 8 * 16;
	buf_info->size += (cxt->commn_cxt.prv_size.h + 7) / 8 * 16;
	buf_info->size += STATIS_TAIL_RESERVED_SIZE;
	buf_info->num = STATIS_3DNR_BUF_NUM;

	/* for embedline */
	/* TODO - ebd buffer size should be adapt to specific sensor type */
	buf_info = &cxt->stats_mem_info.buf_info[STATIS_EBD];
	if (cxt->ebd_cxt.ebd_support) {
		buf_info->size = 0x4000;
		buf_info->num = STATIS_EBD_BUF_NUM;
		ISP_LOGD("ebd buffer size %d\n", buf_info->size);
	}

	cxt->stats_mem_info.dbg_buf.size = 0;
	cxt->stats_mem_info.dbg_buf.num = 0;
	size0 = DCAM_PARAM_SIZE;
	size1 = ISP_PARAM_SIZE;
	if (cxt->save_data && (size0 > 0) && (size1 > 0)) {
		struct isp_pmdbg_alloc_info *buf_info;

		buf_info = &cxt->stats_mem_info.dbg_buf;
		buf_info->size = (size0 > size1) ? size0 : size1;
		buf_info->size += STATIS_TAIL_RESERVED_SIZE;
		buf_info->num = PARAM_BUF_NUM_MAX;
		ISP_LOGD("dbg buffer size %d, num %d\n", buf_info->size, buf_info->num);
	}

	return 0;
}

static cmr_int pdaf_type3_cfg(struct isp_alg_fw_context *cxt, struct sensor_pdaf_info *pdaf_info)
{
        cmr_int i = 0, ret = 0;
        struct pdaf_ppi_info ppi_info;
	memset(&ppi_info, 0, sizeof(struct pdaf_ppi_info));

	if (cxt->ops.pdaf_ops.ioctrl) {
		ISP_LOGI("open pdaf type 3");
		ret = cxt->ops.pdaf_ops.ioctrl(
				cxt->pdaf_cxt.handle,
				PDAF_CTRL_CMD_SET_PARAM,
				NULL, NULL);
		ISP_RETURN_IF_FAIL(ret, ("fail to cfg pdaf"));
	}

	ppi_info.block.start_x = pdaf_info->pd_offset_x;
	ppi_info.block.end_x = pdaf_info->pd_end_x;
	ppi_info.block.start_y = pdaf_info->pd_offset_y;
	ppi_info.block.end_y = pdaf_info->pd_end_y;
	ppi_info.block_size.height = pdaf_info->pd_block_h;
	ppi_info.block_size.width = pdaf_info->pd_block_w;
	ppi_info.pd_pos_size = pdaf_info->pd_pos_size;
	ppi_info.bypass = 0;

	for (i = 0; i < pdaf_info->pd_pos_size * 2; i++) {
		ppi_info.pattern_pixel_is_right[i] = pdaf_info->pd_is_right[i];
		ppi_info.pattern_pixel_row[i] = pdaf_info->pd_pos_row[i];
		ppi_info.pattern_pixel_col[i] = pdaf_info->pd_pos_col[i];
	}
	ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_PDAF_PPI_INFO, &ppi_info, NULL);
	ISP_RETURN_IF_FAIL(ret, ("fail to cfg ppi_info"));
        return ret;
}

cmr_int isp_alg_fw_start(cmr_handle isp_alg_handle, struct isp_video_start * in_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_int isp_pdaf_type = 0;
	cmr_u32 sn_mode = 0;
	cmr_u32 pdaf_bypass = 0;
	cmr_u32 flash_awb_en = 0;
	cmr_u8 mainflash_en = 1;
	char value[PROPERTY_VALUE_MAX] = { 0x00 };
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct alsc_fwstart_info fwstart_info = { NULL, {NULL}, 0, 0, 5, 0, 0, 0, 0};
	struct afctrl_fwstart_info af_start_info;
	struct dcam_dev_vc2_control vch2_info;
	struct sensor_pdaf_info *pdaf_info = NULL;
	struct pm_workmode_input  pm_input;
	struct pm_workmode_output pm_output;


	if (!isp_alg_handle || !in_ptr) {
		ISP_LOGE("fail to get valid ptr.");
		ret = ISP_PARAM_ERROR;
		return ret;
	}
	if (!cxt->alg_enable) {
		ISP_LOGD("alg bypass\n");
		return ret;
	}

	ISP_LOGI("cam%ld init status %d\n", cxt->camera_id, cxt->init_status);

	isp_alg_fw_init_async_done(cxt);

	if (cxt->init_status == FW_INIT_ERR) {
		ISP_LOGE("cam%ld fw init error\n", cxt->camera_id);
		return ISP_ERROR;
	}

	ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_RESET, NULL, NULL);

	/* free/reset statis buffers of previous session */
	ret = isp_dev_free_buf(cxt->dev_access_handle, &cxt->stats_mem_info);

    /* gtm_ltm_on != 1/0,means not set */
	cxt->gtm_ltm_on = (-1);

	cxt->first_frm = 1;
	cxt->aem_is_update = 0;
	cxt->smooth_ratio = 0;
	cxt->work_mode = in_ptr->work_mode;
	cxt->zsl_flag = in_ptr->zsl_flag;
	cxt->sensor_fps.mode = in_ptr->sensor_fps.mode;
	cxt->sensor_fps.max_fps = in_ptr->sensor_fps.max_fps;
	cxt->sensor_fps.min_fps = in_ptr->sensor_fps.min_fps;
	cxt->sensor_fps.is_high_fps = in_ptr->sensor_fps.is_high_fps;
	cxt->sensor_fps.high_fps_skip_num = in_ptr->sensor_fps.high_fps_skip_num;
	cxt->commn_cxt.src.w = in_ptr->size.w;
	cxt->commn_cxt.src.h = in_ptr->size.h;
	cxt->commn_cxt.prv_size = cxt->commn_cxt.src;
	cxt->remosaic_type = in_ptr->remosaic_type;
	cxt->is_high_res_mode = in_ptr->is_high_res_mode;
	cxt->is_ai_scene_pro = 0;
	cxt->app_mode = in_ptr->app_mode;
	cxt->ambient_highlight = 0;	/* default lowlight, zsl */
	if (cxt->remosaic_type && in_ptr->work_mode == 0) {
		cxt->commn_cxt.prv_size.w >>= 1;
		cxt->commn_cxt.prv_size.h >>= 1;
	}
	ISP_LOGD("work_mode %d,is_dv %d,zsl %d,size %d %d,prv_size %d %d, 4in1 %d, remosaic %d %d %d %d\n",
		in_ptr->work_mode, in_ptr->dv_mode, in_ptr->zsl_flag,
		in_ptr->size.w, in_ptr->size.h, cxt->commn_cxt.prv_size.w, cxt->commn_cxt.prv_size.h,
		cxt->is_4in1_sensor, cxt->remosaic_type, cxt->ambient_highlight,
		cxt->is_high_res_mode, cxt->fix_highlight);

	cxt->stats_mem_info.alloc_cb = in_ptr->alloc_cb;
	cxt->stats_mem_info.free_cb = in_ptr->free_cb;
	cxt->stats_mem_info.invalidate_cb = in_ptr->invalidate_cb;
	cxt->stats_mem_info.flush_cb = in_ptr->flush_cb;
	cxt->stats_mem_info.oem_handle = in_ptr->oem_handle;
	sn_mode = in_ptr->resolution_info.size_index;
	cxt->sn_mode = sn_mode;
	ISP_LOGV("is_master = %d, sn_mode = %d", cxt->is_master, sn_mode);
	if (!cxt->is_master) {
		isp_br_ioctrl(CAM_SENSOR_SLAVE0, SET_SLAVE_SENSOR_MODE, &sn_mode, NULL);
	}

	/* initialize isp pm */
	memset(&pm_input, 0, sizeof(struct pm_workmode_input));
	memset(&pm_output, 0, sizeof(struct pm_workmode_output));
	pm_input.pm_sets_num = 1;
	pm_input.remosaic_type = cxt->remosaic_type;

	switch (in_ptr->work_mode) {
	case 0:		/*preview / video */
	{
		if (in_ptr->dv_mode)
			pm_input.mode[0] = WORKMODE_VIDEO;
		else
			pm_input.mode[0] = WORKMODE_PREVIEW;
		pm_input.img_w[0] = cxt->commn_cxt.src.w;
		pm_input.img_h[0] = cxt->commn_cxt.src.h;
		if (cxt->remosaic_type && pm_input.mode[0] == WORKMODE_PREVIEW) {
			pm_input.define[0] = 1;
			pm_input.img_w[0] >>= 1;
			pm_input.img_h[0] >>= 1;
		}
		if (cxt->zsl_flag)  {
			pm_input.pm_sets_num++;
			pm_input.mode[1] = WORKMODE_CAPTURE;
			pm_input.img_w[1] = cxt->commn_cxt.src.w;
			pm_input.img_h[1] = cxt->commn_cxt.src.h;
		}
		break;
	}
	case 1:		/* capture only */
	{
		pm_input.mode[0] = WORKMODE_CAPTURE;
		pm_input.img_w[0] = cxt->commn_cxt.src.w;
		pm_input.img_h[0] = cxt->commn_cxt.src.h;
		break;
	}
	default:
		pm_input.mode[0] = WORKMODE_PREVIEW;
		pm_input.img_w[0] = cxt->commn_cxt.src.w;
		pm_input.img_h[0] = cxt->commn_cxt.src.h;
		break;
	}

	ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_MODE, &pm_input, &pm_output);
	ISP_RETURN_IF_FAIL(ret, ("fail to isp pm set mode"));

	cxt->commn_cxt.isp_pm_mode[0] = pm_output.mode_id[0];
	if (cxt->zsl_flag)
		cxt->commn_cxt.isp_pm_mode[1] = pm_output.mode_id[1];
	else
		cxt->commn_cxt.isp_pm_mode[1] = ISP_TUNE_MODE_INVALID;

	ISP_LOGD("cam %ld, pm mode 0x%x 0x%x,  gtm_ltm on %d\n", cxt->camera_id,
		cxt->commn_cxt.isp_pm_mode[0], cxt->commn_cxt.isp_pm_mode[1], cxt->gtm_ltm_on);

	cxt->commn_cxt.param_index = ispalg_get_param_index(cxt->commn_cxt.input_size_trim, &in_ptr->size);

	ret = ispalg_update_alg_param(cxt);
	ISP_RETURN_IF_FAIL(ret, ("fail to isp smart param calc"));

	ISP_LOGI("pdaf_support = %d, pdaf_enable = %d, is_multi_mode = %d",
			cxt->pdaf_cxt.pdaf_support, in_ptr->pdaf_enable, cxt->is_multi_mode);

	property_get("debug.camera.pdaf.type1.supprot", (char *)value, "0");
	sn_mode = in_ptr->resolution_info.size_index;
	pdaf_info = cxt->pdaf_info;
	cxt->fix_highlight = 0;
	ISP_LOGV("sn_mode:0x%x, type1.support:%d", sn_mode, atoi(value));
	/* high resolution: fix ambient_high light */
	property_get("persist.vendor.cam.4in1.light", (char *)value, "0");
	if (strcmp(value, "high") == 0) {
		cxt->fix_highlight = 1;
		ISP_LOGD("ambient_highlight fix to 1");
	} else if (strcmp(value, "low") == 0) {
		cxt->fix_highlight = 2;
		ISP_LOGD("ambient_highlight fix to 0");
	}

	/* start config PDAF information */
	if (in_ptr->pdaf_enable && pdaf_info) {
		isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_PDAF_BYPASS, &pdaf_bypass, NULL);
		switch (cxt->pdaf_cxt.pdaf_support){
		case SENSOR_PDAF_TYPE1_ENABLE:
			if (pdaf_info->sns_mode &&
					(pdaf_info->sns_mode[sn_mode] || (1 == atoi(value)))){
				ISP_LOGI("pdaf_info->sns_mode = %d",pdaf_info->sns_mode[sn_mode]);
				isp_pdaf_type = ISP_DEV_SET_PDAF_TYPE1_CFG;
			}
			cxt->stats_mem_info.buf_info[STATIS_PDAF].size = pdaf_info->pd_data_size;
			cxt->stats_mem_info.buf_info[STATIS_PDAF].num = STATIS_PDAF_BUF_NUM;
			break;
		case SENSOR_PDAF_TYPE2_ENABLE:
			isp_pdaf_type = ISP_DEV_SET_PDAF_TYPE2_CFG;
			cxt->stats_mem_info.buf_info[STATIS_PDAF].size = pdaf_info->pd_data_size;
			cxt->stats_mem_info.buf_info[STATIS_PDAF].num = STATIS_PDAF_BUF_NUM;
			break;
		case SENSOR_PDAF_TYPE3_ENABLE:
			isp_pdaf_type = ISP_DEV_SET_PDAF_TYPE3_CFG;
			/* config pdaf block: ROI, coordinate, window */
			ret = pdaf_type3_cfg(cxt, pdaf_info);
                        ISP_RETURN_IF_FAIL(ret, ("fail to cfg PDAF_TYPE3_CFG"));
			cxt->stats_mem_info.buf_info[STATIS_PDAF].size = pdaf_info->pd_data_size;
			cxt->stats_mem_info.buf_info[STATIS_PDAF].num = STATIS_PDAF_BUF_NUM;

			break;
		case SENSOR_DUAL_PDAF_ENABLE:
			isp_pdaf_type = ISP_DEV_SET_DUAL_PDAF_CFG;
			cxt->stats_mem_info.buf_info[STATIS_PDAF].size = pdaf_info->pd_data_size;
			cxt->stats_mem_info.buf_info[STATIS_PDAF].num = STATIS_PDAF_BUF_NUM;
			break;
		default:
			ISP_LOGI("PDAF param is invalid");
			break;
		}

		if (cxt->stats_mem_info.buf_info[STATIS_PDAF].size == 0) {
			ISP_LOGE("pdaf error:  cam%ld pdaf data size is 0, please set it to correct size\n", cxt->camera_id);
			cxt->stats_mem_info.buf_info[STATIS_PDAF].size = 256 * 1024;
		} else if (cxt->stats_mem_info.buf_info[STATIS_PDAF].size < 4096) {
			ISP_LOGD("pdaf size is smaller than one page\n");
			cxt->stats_mem_info.buf_info[STATIS_PDAF].size = 4096;
		} else {
			cxt->stats_mem_info.buf_info[STATIS_PDAF].size += STATIS_TAIL_RESERVED_SIZE;
		}

		vch2_info.bypass = pdaf_info->vch2_info.bypass;
		vch2_info.vch2_vc = pdaf_info->vch2_info.vch2_vc;
		vch2_info.vch2_data_type = pdaf_info->vch2_info.vch2_data_type;
		vch2_info.vch2_mode = pdaf_info->vch2_info.vch2_mode;
		ISP_LOGI("vch2_info.bypass = 0x%x, vc = 0x%x, data_type = 0x%x, data_size %d, mode = 0x%x",
				vch2_info.bypass, vch2_info.vch2_vc, vch2_info.vch2_data_type,
				cxt->stats_mem_info.buf_info[STATIS_PDAF].size, vch2_info.vch2_mode);
		if (isp_pdaf_type != 0)
			ret = isp_dev_access_ioctl(cxt->dev_access_handle, isp_pdaf_type, &vch2_info, 0);
	} /* end config pdaf */

	if (cxt->ebd_cxt.ebd_support) {
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_EBD_CFG, 0, 0);
		ISP_RETURN_IF_FAIL(ret, ("fail to cfg embedded line"));
	}

	ret = ispalg_update_alsc_result(cxt, (void *)&fwstart_info);
	ISP_RETURN_IF_FAIL(ret, ("fail to update alsc result"));

	ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_CFG_START, NULL, NULL);
	ISP_TRACE_IF_FAIL(ret, ("fail to do cfg start"));

	ret = ispalg_cfg_param(cxt, PARAM_CFG_START);
	ISP_RETURN_IF_FAIL(ret, ("fail to do ispalg_cfg_param"));

	if (cxt->afl_cxt.handle) {
		((struct isp_anti_flicker_cfg *)cxt->afl_cxt.handle)->width = cxt->commn_cxt.prv_size.w;
		((struct isp_anti_flicker_cfg *)cxt->afl_cxt.handle)->height = cxt->commn_cxt.prv_size.h;
	}

	cxt->afl_cxt.hw_bypass = 1;
	if(cxt->afl_cxt.version) {
		if (cxt->ops.afl_ops.config_new)
			ret = cxt->ops.afl_ops.config_new(cxt->afl_cxt.handle);
	}
	ISP_TRACE_IF_FAIL(ret, ("fail to do anti_flicker param update"));

	if (cxt->is_simulator == 0) {
		ret = ispalg_ae_set_work_mode(cxt, cxt->commn_cxt.isp_pm_mode[0], 1, in_ptr);
		ISP_RETURN_IF_FAIL(ret, ("fail to do ae cfg"));
	} else {
		ISP_LOGD("bypass AE for simulator\n");
	}

	if (cxt->ops.awb_ops.ioctrl) {
		ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle,
				AWB_CTRL_CMD_SET_WORK_MODE,
				&in_ptr->work_mode, NULL);
		ISP_RETURN_IF_FAIL(ret, ("fail to set_awb_work_mode"));
		ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle,
				AWB_CTRL_CMD_SET_AE_STAT_WIN_NUM,
				&cxt->ae_cxt.win_num, NULL);
		ISP_RETURN_IF_FAIL(ret, ("fail to set_awb_aem_stat_win"));

		ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle,
				AWB_CTRL_CMD_GET_PIX_CNT, &cxt->commn_cxt.prv_size, NULL);
		ISP_RETURN_IF_FAIL(ret, ("fail to get_awb_pix_cnt"));

		ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle,
				AWB_CTRL_CMD_GET_FLASH_AWB_ENABLE, NULL, &flash_awb_en);
		ISP_TRACE_IF_FAIL(ret, ("fail to get flash awb enable"));
	}

	memset(&af_start_info, 0, sizeof(struct afctrl_fwstart_info));
	af_start_info.sensor_fps.is_high_fps = in_ptr->sensor_fps.is_high_fps;
	af_start_info.sensor_fps.high_fps_skip_num = in_ptr->sensor_fps.high_fps_skip_num;

	if (cxt->af_cxt.handle && cxt->ops.af_ops.ioctrl) {
		ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle,
					AF_CMD_SET_AF_BYPASS,
					(void *)&cxt->af_cxt.sw_bypass, NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to set af_bypass"));

		if (ISP_VIDEO_MODE_CONTINUE == in_ptr->mode) {
			af_start_info.size = cxt->commn_cxt.prv_size;
			ISP_LOGD("trigger AF video start.  size %d %d\n", af_start_info.size.w, af_start_info.size.h);
			ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle,
					AF_CMD_SET_ISP_START_INFO,
					(void *)&af_start_info, NULL);
			ISP_TRACE_IF_FAIL(ret, ("fail to set af_start_info"));
		}
	}
	if (cxt->ops.lsc_ops.ioctrl) {
		ret = cxt->ops.lsc_ops.ioctrl(cxt->lsc_cxt.handle, ALSC_FW_START_END, (void *)&fwstart_info, NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to end alsc_fw_start"));
	}

	if (cxt->ops.ae_ops.ioctrl) {
		ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_FLASH_AWB_EN, (void *)&flash_awb_en, NULL);
		ISP_TRACE_IF_FAIL(ret, ("ae: fail to set flash awb en"));

		ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_GET_MAINFLASH_EN, NULL, (void *)&mainflash_en);
		ISP_TRACE_IF_FAIL(ret, ("ae: fail to get flash info"));

	}

	if (cxt->ops.awb_ops.ioctrl) {
		ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_SET_MAINFLASH_EN, (void*)&mainflash_en, NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to set mainflash_en"));
	}

	if (cxt->is_4in1_sensor && cxt->work_mode && cxt->is_high_res_mode) {
		cmr_u32 bypass = 1;
		isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AE_MONITOR_BYPASS, &bypass, NULL);
		isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_LSC_MONITOR_BYPASS, &bypass, NULL);
		isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AFM_BYPASS, &bypass, NULL);
		isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AFL_NEW_BYPASS, &bypass, NULL);
		isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_BAYERHIST_BYPASS, &bypass, NULL);
	}

	/* malloc statis/lsc and other buffers and mapping buffers to dev. */
	ispalg_calc_stats_size(cxt);
	ret = isp_dev_prepare_buf(cxt->dev_access_handle, &cxt->stats_mem_info);
	ISP_RETURN_IF_FAIL(ret, ("fail to prepare buf"));

	cxt->fw_started = 1;
exit:
	ISP_LOGD("done %ld", ret);
	return ret;
}


cmr_int isp_alg_fw_stop(cmr_handle isp_alg_handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;

	if (!cxt->alg_enable) {
		ISP_LOGD("alg bypass\n");
		return ret;
	}
	if (cxt->ops.ae_ops.ioctrl) {
		ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_VIDEO_STOP, NULL, NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to AE_VIDEO_STOP"));
	}
	if (cxt->ops.awb_ops.ioctrl) {
		ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_VIDEO_STOP_NOTIFY, NULL, NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to AWB_CTRL_CMD_VIDEO_STOP_NOTIFY"));
	}
	if (cxt->ops.af_ops.ioctrl) {
		ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle, AF_CMD_SET_ISP_STOP_INFO, NULL, NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to AF_CMD_SET_ISP_STOP_INFO"));
	}
	if (cxt->ops.lsc_ops.ioctrl) {
		ret = cxt->ops.lsc_ops.ioctrl(cxt->lsc_cxt.handle, ALSC_FW_STOP, NULL, NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to ALSC_FW_STOP"));
	}

	ISP_RETURN_IF_FAIL(ret, ("fail to stop isp alg fw"));

exit:
	if (cxt->dbg_cxt.fp_dat) {
		fclose(cxt->dbg_cxt.fp_dat);
		cxt->dbg_cxt.fp_dat = NULL;
	}
	if (cxt->dbg_cxt.fp_info) {
		fclose(cxt->dbg_cxt.fp_info);
		cxt->dbg_cxt.fp_info = NULL;
	}

	cxt->fdr_cxt.fdr_init = 0;
	cxt->fw_started = 0;
	ISP_LOGD("done %ld", ret);
	return ret;
}

static cmr_int ispalg_dump_alsc_info(struct alsc_do_simulation *alsc_param, cmr_u32 gain_w, cmr_u32 gain_h)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_u32 i = 0;
	FILE *fp = NULL;
	char file_name[260] = { 0 };
	char loop_cnt_str[10] = { 0 };

	if (alsc_param == NULL) {
		ISP_LOGE("fail to awb param pointer.");
		return ISP_ERROR;
	}
	ret = isp_sim_get_mipi_raw_file_name(file_name);
	if (strlen(file_name)) {
		sprintf(loop_cnt_str, ".%d", isp_video_get_simulation_loop_count());
		strcat(file_name, loop_cnt_str);
		strcat(file_name, ".alsc.log");
	} else {
		ISP_LOGE("fail to get mipi raw file name.");
		return ISP_ERROR;
	}
	CMR_LOGD("file name %s", file_name);
	fp = fopen(file_name, "wb");
	if (fp == NULL) {
		ISP_LOGE("fail to open file");
		return ISP_ERROR;
	}

	fprintf(fp, "1.input alsc param:\n");
	fprintf(fp, "bv:%d\n", alsc_param->bv);
	fprintf(fp, "ct:%d\n", alsc_param->ct);
	fprintf(fp, "bv_gain:%d\n", alsc_param->bv_gain);

	fprintf(fp, "2.output alsc table:\n");
	for (i=0; i < gain_w*gain_h*4; i++) {
		fprintf(fp, "%d \n", alsc_param->sim_output_table[i]);
	}
	fflush(fp);
	fclose(fp);
	return ret;
}



static cmr_int ispalg_alsc_update(cmr_handle isp_alg_handle)
{
	cmr_s32 ret = ISP_SUCCESS;
	enum isp_pm_cmd  cmd;
	cmr_u32 adaptive_size_info[3] = {0, 0, 0};
	struct isp_size pic_size;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	lsc_adv_handle_t lsc_adv_handle = cxt->lsc_cxt.handle;
	cmr_handle pm_handle = cxt->handle_pm;
	struct lsc_adv_calc_param calc_param;
	struct isp_pm_ioctl_input io_pm_input = { NULL, 0 };
	struct isp_pm_ioctl_output io_pm_output = { NULL, 0 };
	struct isp_pm_param_data pm_param;
	struct isp_pm_ioctl_input pm_tab_input = { NULL, 0 };
	struct isp_pm_ioctl_output pm_tab_output = { NULL, 0 };
	struct isp_pm_ioctl_input io_pm_input_alsc = { NULL, 0 };
	struct isp_pm_param_data param_data_alsc;
	struct alsc_do_simulation do_sim;
	cmr_u16 *sim_output_table = NULL;
	char value[PROPERTY_VALUE_MAX];

	memset(&calc_param, 0, sizeof(struct lsc_adv_calc_param));
	memset(&io_pm_input, 0, sizeof(struct isp_pm_ioctl_input));
	memset(&io_pm_output, 0, sizeof(struct isp_pm_ioctl_output));
	memset(&pm_param, 0, sizeof(struct isp_pm_param_data));
	memset(&pm_tab_input, 0, sizeof(struct isp_pm_ioctl_input));
	memset(&pm_tab_output, 0, sizeof(struct isp_pm_ioctl_output));
	memset(&io_pm_input_alsc, 0, sizeof(struct isp_pm_ioctl_input));
	memset(&param_data_alsc, 0, sizeof(struct isp_pm_param_data));
	memset(&do_sim, 0, sizeof(struct alsc_do_simulation));

	ISP_LOGI("enter\n");

	if (cxt->lsc_cxt.LSC_SPD_VERSION >= 6) {

		pic_size = cxt->commn_cxt.src;
		cmd = ISP_PM_CMD_SET_GRID0;

		ISP_LOGI("[ALSC] lsc_pm_normalization, new_image_size[%d,%d], full_image_size[%d,%d]",
			pic_size.w, pic_size.h, cxt->lsc_cxt.full_size_width, cxt->lsc_cxt.full_size_height);

		if((cxt->lsc_cxt.full_size_width * pic_size.h) == (cxt->lsc_cxt.full_size_height * pic_size.w)){
			ISP_LOGI("[ALSC] lsc_pm_normalization case1, n binning");
			adaptive_size_info[0] = pic_size.w;
			adaptive_size_info[1] = pic_size.h;
			adaptive_size_info[2] = cxt->lsc_cxt.full_size_grid * pic_size.w / cxt->lsc_cxt.full_size_width;
			memset(&pm_param, 0, sizeof(pm_param));
			BLOCK_PARAM_CFG(io_pm_input, pm_param,
				ISP_PM_BLK_LSC_UPDATE_GRID,
				ISP_BLK_2D_LSC,
				&adaptive_size_info[0], 0);
			ret = isp_pm_ioctl(cxt->handle_pm, cmd, &io_pm_input, NULL);
		} else if ((pic_size.w == 1280) && (pic_size.h == 720)) {
			ISP_LOGI("[ALSC] lsc_pm_normalization case2. image size %d %d\n", pic_size.w, pic_size.h);
			adaptive_size_info[0] = cxt->commn_cxt.src.w;
			adaptive_size_info[1] = cxt->commn_cxt.src.h;
			adaptive_size_info[2] = 32;
			memset(&pm_param, 0, sizeof(pm_param));
			BLOCK_PARAM_CFG(io_pm_input, pm_param,
				ISP_PM_BLK_LSC_UPDATE_GRID,
				ISP_BLK_2D_LSC,
				&adaptive_size_info[0], 0);
			ret = isp_pm_ioctl(cxt->handle_pm, cmd, &io_pm_input, NULL);
		} else {
			ISP_LOGE("[ALSC] lsc_pm_normalization error");
		}
	}

	if (cxt->lsc_cxt.LSC_SPD_VERSION >= 2) {
		cmr_u32 stat_w = 0;
		cmr_u32 stat_h = 0;
		struct isp_lsc_info *lsc_info;
		struct isp_awb_statistic_info awb_stat;
		struct isptool_scene_param scene_param;

		memset(&pm_param, 0, sizeof(pm_param));
		BLOCK_PARAM_CFG(io_pm_input, pm_param, ISP_PM_BLK_LSC_INFO, ISP_BLK_2D_LSC, PNULL, 0);
		ret = isp_pm_ioctl(pm_handle, ISP_PM_CMD_GET_SINGLE_SETTING, (void *)&io_pm_input, (void *)&io_pm_output);

		lsc_info = (struct isp_lsc_info *)io_pm_output.param_data->data_ptr;
		if (NULL == lsc_info || ISP_SUCCESS != ret) {
			ISP_LOGE("fail to get lsc info");
			return ISP_ERROR;
		}

		memset(&awb_stat, 0, sizeof(awb_stat));
		ret = isp_sim_get_no_lsc_ae_stats((void *)&awb_stat, &stat_w, &stat_h);
		if (ret) {
			ISP_LOGE("fail to get ae stats");
		}

		ret = isp_sim_get_scene_parm(&scene_param);
		if(ret) {
			ISP_LOGE("fail to get scene parameter");
		}

		sim_output_table = (cmr_u16*)malloc(lsc_info->gain_w * lsc_info->gain_h * 4 * sizeof(cmr_u16));
		do_sim.stat_r = awb_stat.r_info;
		do_sim.stat_g = awb_stat.g_info;
		do_sim.stat_b = awb_stat.b_info;
		do_sim.ct = scene_param.smart_ct;
		do_sim.bv = scene_param.smart_bv;
		do_sim.bv_gain = scene_param.gain;
		do_sim.sim_output_table = sim_output_table;
		if (cxt->ops.lsc_ops.ioctrl) {
			ret = cxt->ops.lsc_ops.ioctrl(lsc_adv_handle, ALSC_DO_SIMULATION, (void *)&do_sim, NULL);
			if (ISP_SUCCESS != ret)
				ISP_LOGE("fail to do ALSC_DO_SIMULATION!");
		}
		BLOCK_PARAM_CFG(io_pm_input_alsc, param_data_alsc,
				ISP_PM_BLK_LSC_MEM_ADDR, ISP_BLK_2D_LSC,
				do_sim.sim_output_table,
				lsc_info->gain_w * lsc_info->gain_h * 4 * sizeof(cmr_u16));
		ret = isp_pm_ioctl(pm_handle, ISP_PM_CMD_SET_OTHERS, &io_pm_input_alsc, NULL);
		property_get("persist.vendor.cam.debug.simulation", value, "false");
		if (!strcmp(value, "true")) {
			ispalg_dump_alsc_info(&do_sim, lsc_info->gain_w, lsc_info->gain_h);
		}
		free(sim_output_table);
		sim_output_table = NULL;
	}
	return ret;
}

cmr_int isp_alg_fw_proc_start(cmr_handle isp_alg_handle, struct ips_in_param *in_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_video_start param;
	struct isp_raw_proc_info raw_proc_in;
	struct pm_workmode_input  pm_input;
	struct pm_workmode_output pm_output;
	struct alsc_fwstart_info fwstart_info = { NULL, {NULL}, 0, 0, 5, 0, 0, 0, 0};

	if (!isp_alg_handle || !in_ptr) {
		ISP_LOGE("fail to get valid ptr.");
		ret = ISP_PARAM_ERROR;
		return ret;
	}
	if (!cxt->alg_enable) {
		ISP_LOGD("alg bypass\n");
		return ret;
	}

	ISP_LOGD("isp proc start\n");
	ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_RESET, NULL, NULL);
	ISP_TRACE_IF_FAIL(ret, ("fail to do isp_dev_reset"));

	/* free/reset statis buffers of previous session */
	ret = isp_dev_free_buf(cxt->dev_access_handle, &cxt->stats_mem_info);

	cxt->zsl_flag = 0;
	cxt->first_frm = 1;
	cxt->work_mode = PM_SCENE_CAP; /* 0 - preview, 1 - capture  */
	cxt->commn_cxt.src.w = in_ptr->src_frame.img_size.w;
	cxt->commn_cxt.src.h = in_ptr->src_frame.img_size.h;
	cxt->commn_cxt.prv_size = cxt->commn_cxt.src;

	cxt->stats_mem_info.alloc_cb = in_ptr->alloc_cb;
	cxt->stats_mem_info.free_cb = in_ptr->free_cb;
	cxt->stats_mem_info.invalidate_cb = in_ptr->invalidate_cb;
	cxt->stats_mem_info.flush_cb = in_ptr->flush_cb;
	cxt->stats_mem_info.oem_handle = in_ptr->oem_handle;

	ISP_LOGI("takepicture_mode[%d] camera_id[%d] cxt[%p]\n",
			cxt->takepicture_mode,
			(unsigned int)cxt->camera_id,
			cxt);

	memset(&raw_proc_in, 0, sizeof(raw_proc_in));
	raw_proc_in.src_format = IMG_PIX_FMT_GREY;
	raw_proc_in.src_pattern =  cxt->commn_cxt.image_pattern;
	raw_proc_in.src_size.width = in_ptr->src_frame.img_size.w;
	raw_proc_in.src_size.height = in_ptr->src_frame.img_size.h;
	raw_proc_in.dst_format = in_ptr->dst_frame.img_fmt;
	raw_proc_in.dst_size.width = in_ptr->dst_frame.img_size.w;
	raw_proc_in.dst_size.height = in_ptr->dst_frame.img_size.h;
	raw_proc_in.fd_src = in_ptr->src_frame.img_fd.y;
	raw_proc_in.src_offset = in_ptr->src_frame.img_addr_phy.chn0;
	/* dst1 is final dst image (YUV image after ISP) */
	raw_proc_in.fd_dst1 = in_ptr->dst_frame.img_fd.y;
	raw_proc_in.dst1_offset = in_ptr->dst_frame.img_addr_phy.chn0;

	/* dst0 is middle image (raw image after DCAM processing) */
	/* if HAL does not set this buffer (fd <= 0) kernel will allocate one */
	/* HAL may use same buffer as src buffer, but apply offset in it */
	raw_proc_in.fd_dst0 = in_ptr->dst2_frame.img_fd.y;
	raw_proc_in.dst0_offset = in_ptr->dst2_frame.img_addr_phy.chn0;

	/* RAW_PROC_PRE, kernel will init the whole channel for proc. */
	/* Then we can config parameters to right register or ISP context buffer */
	raw_proc_in.cmd = RAW_PROC_PRE;

	if (cxt->takepicture_mode == CAMERA_ISP_SIMULATION_MODE) {
		raw_proc_in.scene = RAW_PROC_SCENE_HWSIM;
	} else {
		raw_proc_in.scene = RAW_PROC_SCENE_RAWCAP;
	}

	ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_RAW_PROC, &raw_proc_in, NULL);
	ISP_RETURN_IF_FAIL(ret, ("fail to do isp_dev_raw_proc"));

	memset(&pm_input, 0, sizeof(struct pm_workmode_input));
	memset(&pm_output, 0, sizeof(struct pm_workmode_output));
	pm_input.pm_sets_num = 1;
	pm_input.mode[0] = WORKMODE_CAPTURE;
	pm_input.img_w[0] = cxt->commn_cxt.src.w;
	pm_input.img_h[0] = cxt->commn_cxt.src.h;

	pm_input.remosaic_type = in_ptr->remosaic_type;;
	ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_MODE, &pm_input, &pm_output);
	ISP_RETURN_IF_FAIL(ret, ("fail to do isp_pm_ioctl"));

	ISP_LOGV("mode %d %d\n", pm_output.mode_id[0],pm_output.mode_id[1]);
	cxt->commn_cxt.isp_pm_mode[0] = pm_output.mode_id[0];
	cxt->commn_cxt.isp_pm_mode[1] = ISP_TUNE_MODE_INVALID;

	cxt->commn_cxt.param_index =
		ispalg_get_param_index(cxt->commn_cxt.input_size_trim, &in_ptr->src_frame.img_size);

	if (cxt->takepicture_mode != CAMERA_ISP_SIMULATION_MODE) {
		ret = ispalg_update_alg_param(cxt);
		ISP_RETURN_IF_FAIL(ret, ("fail to update alg parm"));

		ret = ispalg_update_alsc_result(cxt, (void *)&fwstart_info);
		ISP_RETURN_IF_FAIL(ret, ("fail to update alsc result"));
	} else {
		ret = ispalg_update_smart_param(cxt);
		ISP_RETURN_IF_FAIL(ret, ("fail to update smart parm"));

		ispalg_alsc_update(isp_alg_handle);
	}

	ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_CFG_START, NULL, NULL);
	ISP_TRACE_IF_FAIL(ret, ("fail to do cfg start"));

	ret = ispalg_cfg_param(cxt, PARAM_CFG_START);
	ISP_RETURN_IF_FAIL(ret, ("fail to do isp cfg"));

	if (cxt->takepicture_mode == CAMERA_ISP_SIMULATION_MODE) {
		param.is_need_flash = 0;
		param.capture_skip_num = 0;
		param.is_snapshot = 1;
		param.dv_mode = 0;
		param.sensor_fps.mode = in_ptr->sensor_fps.mode;
		param.sensor_fps.max_fps = in_ptr->sensor_fps.max_fps;
		param.sensor_fps.min_fps = in_ptr->sensor_fps.min_fps;
		param.sensor_fps.is_high_fps = in_ptr->sensor_fps.is_high_fps;
		param.sensor_fps.high_fps_skip_num = in_ptr->sensor_fps.high_fps_skip_num;
		ret = ispalg_ae_set_work_mode(cxt, cxt->commn_cxt.isp_pm_mode[0], 0, &param);
		ISP_RETURN_IF_FAIL(ret, ("fail to set ae work mode"));
	}

	if (cxt->ops.ae_ops.ioctrl) {
		ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_RGB_GAIN, NULL, NULL);
		ISP_RETURN_IF_FAIL(ret, ("fail to set rgb gain"));
	}

	if (cxt->takepicture_mode != CAMERA_ISP_SIMULATION_MODE && cxt->ops.lsc_ops.ioctrl) {
		ISP_LOGD("ALSC_FW_START_END\n");
		ret = cxt->ops.lsc_ops.ioctrl(cxt->lsc_cxt.handle, ALSC_FW_START_END, (void *)&fwstart_info, NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to end alsc_fw_start_end"));
	}

	/* PDAF TYPE 3 PD info stored in sensor info, not in pm */
	if (SENSOR_PDAF_TYPE3_ENABLE == cxt->pdaf_cxt.pdaf_support) {
		if (cxt->ops.pdaf_ops.ioctrl) {
			ISP_LOGI("hwsim: pdaf3 set param\n");
			ret = cxt->ops.pdaf_ops.ioctrl(
					cxt->pdaf_cxt.handle,
					PDAF_CTRL_CMD_SET_PARAM,
					NULL, NULL);
			ISP_RETURN_IF_FAIL(ret, ("fail to cfg pdaf"));
		}
		cxt->stats_mem_info.buf_info[STATIS_PDAF].size = 4096;
		cxt->stats_mem_info.buf_info[STATIS_PDAF].num = 1;
	}

	/* malloc statis/lsc and other buffers and mapping buffers to dev. */
	ispalg_calc_stats_size(cxt);
	ret = isp_dev_prepare_buf(cxt->dev_access_handle, &cxt->stats_mem_info);
	ISP_RETURN_IF_FAIL(ret, ("fail to prepare buf"));

	/* RAW_PROC_POST, kernel will set all buffers and trigger raw image processing*/
	ISP_LOGV("start raw proc\n");
	raw_proc_in.cmd = RAW_PROC_POST;
	ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_RAW_PROC, &raw_proc_in, NULL);
	ISP_TRACE_IF_FAIL(ret, ("fail to do isp_dev_raw_proc"));

	/* restore to default mode, or else normal take picture will still be treated as simulation mode. */
	if (isp_video_get_simulation_flag()) {
		/* hwsim will do twice pipeline */
		ISP_LOGI("takepicture_mode[%d]\n", cxt->takepicture_mode);
	} else {
		cxt->takepicture_mode = CAMERA_NORMAL_MODE;
	}
exit:
	ISP_LOGV("done %ld", ret);
	return ret;
}


cmr_int isp_alg_fw_ioctl(cmr_handle isp_alg_handle, enum isp_ctrl_cmd io_cmd, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	enum isp_ctrl_cmd cmd = io_cmd & 0x7fffffff;
	isp_io_fun io_ctrl = NULL;
	cmr_u32 loop = 1000,  cnt = 0, time_out = 0;
	cmr_u32 is_sync_ai = 0, is_aysnc_ai = 0, is_ai_cmd = 0, *msg_data;
	cmr_u8 *wait_status;
	CMR_MSG_INIT(message);

	if (!cxt->alg_enable) {
		ISP_LOGD("alg bypass\n");
		return ret;
	}
	cxt->commn_cxt.isp_callback_bypass = io_cmd & 0x80000000;

	if ((cmd == ISP_CTRL_AI_PROCESS_START)
		|| (cmd == ISP_CTRL_AI_PROCESS_STOP))
		is_aysnc_ai = 1;
	if ((cmd == ISP_CTRL_AI_SET_IMG_PARAM)
		|| (cmd == ISP_CTRL_AI_GET_IMG_FLAG)
		|| (cmd == ISP_CTRL_AI_GET_STATUS)
		|| (cmd == ISP_CTRL_AI_SET_FD_STATUS))
		is_sync_ai = 1;

	is_ai_cmd = (is_aysnc_ai | is_sync_ai);

	if (is_aysnc_ai && (cxt->ai_init_status == FW_INIT_GOING)) {
		if (!cxt->thr_aihandle) {
			ISP_LOGE("cam%ld ai is not init\n", cxt->camera_id);
			return ret;
		}
		msg_data = (cmr_u32 *)malloc(sizeof(cmr_u32));
		if (msg_data == NULL) {
			ISP_LOGE("fail to malloc a int ptr\n");
			return ret;
		}
		*msg_data = *(cmr_u32 *)param_ptr;
		message.msg_type =
				(cmd == ISP_CTRL_AI_PROCESS_START) ?
				AI_CMD_START : AI_CMD_STOP;
		message.sync_flag = CMR_MSG_SYNC_NONE;
		message.sub_msg_type = cxt->commn_cxt.isp_callback_bypass;
		message.alloc_flag = 1;
		message.data = msg_data;

		ISP_LOGD("cam%ld send AI cmd %d, %d, data %p, val %d\n",
				cxt->camera_id, message.msg_type, cmd, msg_data, *msg_data);
		ret = cmr_thread_msg_send(cxt->thr_aihandle, &message);
		if (ret) {
			free(message.data);
			message.data = NULL;
		}
		return ret;
	}

	io_ctrl = isp_ioctl_get_fun(cmd);
	if (NULL != io_ctrl) {
		wait_status = (is_ai_cmd ? &cxt->ai_init_status :  &cxt->init_status);
		while (*wait_status== FW_INIT_GOING) {
			ISP_LOGD("cam%ld cmd %d wait init done %d\n", cxt->camera_id, cmd, cnt);
			usleep(1 * 1000);
			if (cnt++ >= loop) {
				time_out = 1;
				break;
			}
		}
		if (*wait_status == FW_INIT_ERR || time_out) {
			ISP_LOGE("cam%ld fw init error or time out %d\n", cxt->camera_id, time_out);
			return ISP_ERROR;
		}
		ret = io_ctrl(cxt, param_ptr);
	} else {
		ISP_LOGV("io_ctrl fun is null, cmd %d", cmd);
	}

	if (NULL != cxt->commn_cxt.callback) {
		cxt->commn_cxt.callback(cxt->commn_cxt.caller_id,
			ISP_CALLBACK_EVT | ISP_CTRL_CALLBACK | cmd, NULL, ISP_ZERO);
	}

	return ret;
}

cmr_int isp_alg_fw_capability(cmr_handle isp_alg_handle, enum isp_capbility_cmd cmd, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	cmr_u32 out_param = 0;
	cmr_u32 loop = 1000,  cnt = 0, time_out = 0;

	while (cxt->alg_enable && cxt->init_status == FW_INIT_GOING) {
		ISP_LOGD("cam%ld cmd %d wait init done %d\n", cxt->camera_id, cmd, cnt);
		usleep(1 * 1000);
		if (cnt++ >= loop) {
			time_out = 1;
			break;
		}
	}

	if (cxt->init_status == FW_INIT_ERR || time_out) {
		ISP_LOGE("cam%ld fw init error or time out %d\n", cxt->camera_id, time_out);
		return ISP_ERROR;
	}

	switch (cmd) {
	case ISP_LOW_LUX_EB:
		if (!cxt->alg_enable) {
			*((cmr_u32 *) param_ptr) = 0;
			break;
		}
		if (cxt->ops.ae_ops.ioctrl)
			ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_GET_FLASH_EB, NULL, &out_param);
		*((cmr_u32 *) param_ptr) = out_param;
		break;
	case ISP_CUR_ISO:
		if (!cxt->alg_enable) {
			*((cmr_u32 *) param_ptr) = 100;
			break;
		}
		if (cxt->ops.ae_ops.ioctrl)
			ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_GET_ISO, NULL, &out_param);
		*((cmr_u32 *) param_ptr) = out_param;
		break;
	case ISP_CTRL_GET_AE_LUM:
		if (!cxt->alg_enable) {
			*((cmr_u32 *) param_ptr) = 200;
			break;
		}
		if (cxt->ops.ae_ops.ioctrl)
			ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_GET_BV_BY_LUM_NEW, NULL, &out_param);
		*((cmr_u32 *) param_ptr) = out_param;
		break;
	default:
		break;
	}

	ISP_LOGV("done %ld", ret);
	return ret;
}


cmr_int isp_alg_fw_init(struct isp_alg_fw_init_in * input_ptr, cmr_handle * isp_alg_handle)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_u32 alg_bypass = 0;
	char value[PROPERTY_VALUE_MAX];
	cmr_s8 *sensor_name = NULL;
	pthread_attr_t attr;
	struct fw_init_local *init_cxt = NULL;
	struct isp_alg_fw_context *cxt = NULL;
	struct sensor_raw_info *sensor_raw_info_ptr = NULL;

	if (!input_ptr || !isp_alg_handle) {
		ISP_LOGE("fail to check input param, 0x%lx", (cmr_uint) input_ptr);
		ret = ISP_PARAM_NULL;
		return ret;
	}
	*isp_alg_handle = NULL;

	init_cxt = (struct fw_init_local *)malloc(sizeof(struct fw_init_local));
	cxt = (struct isp_alg_fw_context *)malloc(sizeof(struct isp_alg_fw_context));
	if (!cxt || !init_cxt) {
		ISP_LOGE("fail to malloc");
		ret = ISP_ALLOC_ERROR;
		goto exit;
	}
	memset(init_cxt, 0, sizeof(struct fw_init_local));
	memset(cxt, 0, sizeof(struct isp_alg_fw_context));
	init_cxt->cxt = cxt;
	cxt->init_cxt = (void *)init_cxt;

	sensor_raw_info_ptr =
		(struct sensor_raw_info *)input_ptr->init_param->setting_param_ptr;

	if (sensor_raw_info_ptr && sensor_raw_info_ptr->version_info)
		sensor_name = &sensor_raw_info_ptr->version_info->sensor_ver_name.sensor_name[0];

	property_get("persist.vendor.cam.bringup.sensor", value, "null");
	if (!strcmp(value, "all")) {
		alg_bypass = 1;
		ISP_LOGI("ISP 3A/pm will bypass for all sensor\n");
	} else if (sensor_raw_info_ptr == NULL) {
		alg_bypass = 1;
		ISP_LOGI("ISP 3A/pm will bypass for NULL sensor_raw_info\n");
	} else if (!strcmp(value, "auto")) {
		cmr_u32 i, valid_pm = 0;
		for (i = 0; i < MAX_MODE_NUM; i++) {
			if (sensor_raw_info_ptr->mode_ptr[i].addr && sensor_raw_info_ptr->mode_ptr[i].len) {
				valid_pm = 1;
				break;
			}
		}
		alg_bypass = !valid_pm;
		if (alg_bypass)
			ISP_LOGI("ISP 3A/pm will bypass because no tuning param\n");
	} else if (sensor_name && (!strcmp(value, (const char *)sensor_name))) {
		alg_bypass = 1;
		ISP_LOGI("ISP 3A/pm will bypass for this sensor: %s\n", sensor_name);
	}

	if (sensor_name)
		strcpy((char *)cxt->dbg_cxt.sensor_name, (const char *)sensor_name);

	if (alg_bypass) {
		cxt->alg_enable = 0;
		*isp_alg_handle = (cmr_handle)cxt;
		free((void *)init_cxt);
		return 0;
	}

	property_get("ro.debuggable", value, "0");
	s_dbg_ver = !!atoi(value);

	cxt->save_data = 0;
	property_get("persist.vendor.cam.all_data.mode", value, "0");
	if (!strcmp(value, "cap_all"))
		cxt->save_data = 1;
	if (0 && input_ptr->init_param->is_simulator)
		cxt->save_data = 0;

	cxt->dev_access_handle = input_ptr->dev_access_handle;
	cxt->lib_use_info = sensor_raw_info_ptr->libuse_info;
	cxt->otp_data = input_ptr->init_param->otp_data;
	cxt->camera_id = input_ptr->init_param->camera_id;
	cxt->is_master = input_ptr->init_param->is_master;
	cxt->is_multi_mode = input_ptr->init_param->multi_mode;
	cxt->af_cxt.tof_support = input_ptr->init_param->ex_info.tof_support;
	cxt->pdaf_cxt.pdaf_support = input_ptr->init_param->ex_info.pdaf_supported;
	cxt->ebd_cxt.ebd_support = input_ptr->init_param->ex_info.ebd_supported;
	cxt->awb_cxt.color_support = input_ptr->init_param->ex_info.color_support;
	cxt->long_expose_support = input_ptr->init_param->ex_info.long_expose_supported;
	cxt->is_4in1_sensor = input_ptr->init_param->is_4in1_sensor;
	cxt->is_simulator = input_ptr->init_param->is_simulator;
	cxt->commn_cxt.is_faceId_unlock =input_ptr->init_param->is_faceId_unlock;
	cxt->commn_cxt.sensor_max_size = input_ptr->init_param->sensor_max_size;
	cxt->commn_cxt.src.w = input_ptr->init_param->size.w;
	cxt->commn_cxt.src.h = input_ptr->init_param->size.h;
	cxt->commn_cxt.prv_size.w = input_ptr->init_param->size.w;
	cxt->commn_cxt.prv_size.h = input_ptr->init_param->size.h;
	cxt->commn_cxt.callback = input_ptr->init_param->ctrl_callback;
	cxt->commn_cxt.caller_id = input_ptr->init_param->oem_handle;
	cxt->commn_cxt.ops = input_ptr->init_param->ops;

	ISP_LOGI("camera_id = %ld, master %d, is_4in1_sensor %d\n", cxt->camera_id,
		cxt->is_master, cxt->is_4in1_sensor);

	if (input_ptr->init_param->pdaf_info) {
		cxt->pdaf_info = (struct sensor_pdaf_info *)malloc(sizeof(struct sensor_pdaf_info));
		if (!cxt->pdaf_info) {
			ISP_LOGE("fail to malloc pdaf_info buf");
			ret = ISP_ALLOC_ERROR;
			goto exit;
		}
		memcpy(cxt->pdaf_info, input_ptr->init_param->pdaf_info, sizeof(struct sensor_pdaf_info));
	}

	ret = ispalg_pm_init_sync(init_cxt, input_ptr->init_param);

	ISP_LOGD("cam%ld create fw init thread, cxt %p %p\n", cxt->camera_id, init_cxt, cxt);

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	ret = pthread_create(&init_cxt->init_thread_handle, &attr,
				isp_alg_fw_init_async,
				(void *)init_cxt);
	pthread_setname_np(init_cxt->init_thread_handle, "fw_init_async");
	pthread_attr_destroy(&attr);
	if (ret) {
		ISP_LOGE("cam%ld fail to create init thread %ld\n", cxt->camera_id, ret);
		goto exit;
	}

	cxt->alg_enable = 1;
	*isp_alg_handle = (cmr_handle)cxt;

	ISP_LOGI("cam%ld done, alg enable %d, init status %d\n", cxt->camera_id, cxt->alg_enable, cxt->init_status);
	return 0;

exit:
	if (cxt && cxt->pdaf_info) {
		free((void *)cxt->pdaf_info);
		cxt->pdaf_info = NULL;
	}
	if (init_cxt)
		free((void *)init_cxt);
	if (cxt)
		free(cxt);

	ISP_LOGE("done: %ld", ret);
	return ret;
}

cmr_int isp_alg_fw_deinit(cmr_handle isp_alg_handle)
{
	cmr_s32 ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	CMR_MSG_INIT(message);

	if (!cxt) {
		ISP_LOGE("fail to get cxt pointer");
		ret = ISP_PARAM_NULL;
		return ret;
	}
	if (!cxt->alg_enable) {
		free((void *)cxt);
		ISP_LOGI("done %d\n", ret);
		return ret;
	}

	/* sync with all done if init async is triggered  */
	/* Just for abnormal case deinit called */
	isp_alg_fw_init_async_done(cxt);
	if (cxt->thr_aihandle) {
		message.msg_type = AI_CMD_WAIT;
		message.sync_flag = CMR_MSG_SYNC_PROCESSED;
		ISP_LOGI("cam%ld send AI sync wait\n", cxt->camera_id);
		ret = cmr_thread_msg_send(cxt->thr_aihandle, &message);
	}

	ispalg_destroy_thread_proc((cmr_handle) cxt);
	ispalg_deinit((cmr_handle) cxt);
	isp_br_deinit(cxt->camera_id);

	pthread_mutex_destroy(&cxt->pm_getting_lock);
	isp_pm_deinit(cxt->handle_pm);

	if (cxt->ispalg_lib_handle) {
#if 0
		dlclose(cxt->ispalg_lib_handle);
#else
        put_lib_handle(cxt->ispalg_lib_handle);
#endif
		cxt->ispalg_lib_handle = NULL;
	}
	isp_dev_free_buf(cxt->dev_access_handle, &cxt->stats_mem_info);

	debuginfo_q_free(&cxt->ae_queue);
	debuginfo_q_free(&cxt->alsc_queue);
	debuginfo_q_free(&cxt->ai_queue);
	debuginfo_q_free(&cxt->awb_queue);
	debuginfo_q_free(&cxt->af_queue);
	debuginfo_q_free(&cxt->aft_queue);
	debuginfo_q_free(&cxt->smart_queue);
	pthread_mutex_destroy(&cxt->debuginfo_queue_lock);

	if (cxt->ae_cxt.log_alc) {
		free(cxt->ae_cxt.log_alc);
		cxt->ae_cxt.log_alc = NULL;
	}

	if (cxt->commn_cxt.log_isp) {
		free(cxt->commn_cxt.log_isp);
		cxt->commn_cxt.log_isp = NULL;
	}

	if (cxt->fdr_cxt.log_fdr) {
		free(cxt->fdr_cxt.log_fdr);
		cxt->fdr_cxt.log_fdr= NULL;
	}

	if (cxt->dbg_cxt.fp_dat) {
		fclose(cxt->dbg_cxt.fp_dat);
		cxt->dbg_cxt.fp_dat = NULL;
	}
	if (cxt->dbg_cxt.fp_info) {
		fclose(cxt->dbg_cxt.fp_info);
		cxt->dbg_cxt.fp_info = NULL;
	}

	if (cxt->pdaf_info) {
		free((void *)cxt->pdaf_info);
		cxt->pdaf_info= NULL;
	}
	free((void *)cxt);

	ISP_LOGI("done %d", ret);
	return ret;
}
