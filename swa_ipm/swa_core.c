/*-------------------------------------------------------------------*/
/*  Copyright(C) 2020 by Unisoc                                  */
/*  All Rights Reserved.                                             */
/*-------------------------------------------------------------------*/


#define LOG_TAG "swa_core"


#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <stdbool.h>

#include "sprd_camalg_adapter.h"
#include "sprd_hdr_adapter.h"
#include "sprd_img_warp.h"
#include "mfsr_adapter.h"
#include "mfnr_adapt_interface.h"
#include "sprd_yuv_denoise_adapter.h"
#include "sprd_dre_adapter_pro.h"
#include "sprd_facebeauty_adapter.h"
#include "imagefilter_interface.h"

#include "swa_param.h"
#include "swa_ipm_api.h"


static long s_swa_log_level = 4;

#ifdef ANDROID_LOG

#include <cutils/log.h>

#define DEBUG_STR "%d, %s: "
#define ERROR_STR "%d, %s: hal_err "
#define DEBUG_ARGS __LINE__, __FUNCTION__

#define SWA_LOGE(format, ...) ALOGE(DEBUG_STR format, DEBUG_ARGS, ##__VA_ARGS__)

#define SWA_LOGW(format, ...)                                                  \
    ALOGW_IF(s_swa_log_level >= SWA_LOGLEVEL_W, DEBUG_STR format, DEBUG_ARGS, \
             ##__VA_ARGS__)

#define SWA_LOGI(format, ...)                                                  \
    ALOGI_IF(s_swa_log_level >= SWA_LOGLEVEL_I, DEBUG_STR format, DEBUG_ARGS, \
             ##__VA_ARGS__)
#define SWA_LOGD(format, ...)                                                  \
    ALOGD_IF(s_swa_log_level >= SWA_LOGLEVEL_D, DEBUG_STR format, DEBUG_ARGS, \
             ##__VA_ARGS__)
/* ISP_LOGV uses ALOGD_IF */
#define SWA_LOGV(format, ...)                                                  \
    ALOGD_IF(s_swa_log_level >= SWA_LOGLEVEL_V, DEBUG_STR format, DEBUG_ARGS, \
             ##__VA_ARGS__)
#else
#define SWA_LOGE printf
#define SWA_LOGW printf
#define SWA_LOGI printf
#define SWA_LOGD printf
#define SWA_LOGV printf
#endif

#ifndef MAX
#define MAX(a, b) ((a > b) ?  a : b)
#endif

#ifndef ALIGN
#define ALIGN(val, align) ((val + align - 1) & (~(align - 1)))
#endif

#if defined(CONFIG_SPRD_HDR_LIB_VERSION_3)
#define CONFIG_CAMERA_HDR_CAPTURE_IPS
#elif defined(CONFIG_SPRD_HDR_LIB_VERSION_2)
#define CONFIG_CAMERA_HDR_CAPTURE_IPS
#endif

#ifdef CONFIG_CAMERA_HDR_CAPTURE
#ifdef CONFIG_CAMERA_HDR_CAPTURE_IPS

struct hdr_context_t {
	uint32_t frame_num;
	uint32_t pic_w;
	uint32_t pic_h;
	float ev[3];

	void * hdr_handle;  // for hdr algo api internal handle
	sprd_hdr_version_t version;
};


int swa_hdr_get_handle_size()
{
	return sizeof(struct hdr_context_t);
}

int swa_hdr_open(void *ipmpro_hanlde,
			void * open_param, enum swa_log_level log_level)
{
	int ret = 0;
	struct hdr_context_t *cxt = (struct hdr_context_t *)ipmpro_hanlde;
	struct swa_hdr_param *init_param;
	sprd_hdr_init_param_t hdr_param;

	s_swa_log_level = log_level;

	if ((ipmpro_hanlde == NULL) || (open_param == NULL)) {
		SWA_LOGE("fail to get hdr handle %p %p\n", ipmpro_hanlde, open_param);
		return -1;
	}

	init_param = (struct swa_hdr_param *)open_param;
	memset(cxt, 0, sizeof(struct hdr_context_t));

	cxt->frame_num = init_param->fmt;
	cxt->pic_w = init_param->pic_w;
	cxt->pic_h = init_param->pic_h;
	hdr_param.malloc = init_param->heap_mem_malloc;
	hdr_param.free = init_param->heap_mem_free;
	hdr_param.tuning_param_size = init_param->tuning_param_size;
	hdr_param.tuning_param = init_param->tuning_param_ptr;
	SWA_LOGD("frame num %d, pic size %d %d, hdr_param.tuning_param_size %d ,%p\n",
		cxt->frame_num, cxt->pic_w, cxt->pic_h, hdr_param.tuning_param_size, hdr_param.tuning_param);

	cxt->hdr_handle = sprd_hdr_adpt_init(cxt->pic_w, cxt->pic_h, (void *)&hdr_param);
	if (cxt->hdr_handle == NULL) {
		SWA_LOGE("fail to init hdr\n");
		return -1;
	}

	SWA_LOGD("hdr_handle %p\n", cxt->hdr_handle);
	return 0;
}

int swa_hdr_process(void * ipmpro_hanlde,
			struct swa_frames_inout *in,
			struct swa_frames_inout *out,
			void * param)
{
	int ret = 0;
	int i;
	struct hdr_context_t *cxt = NULL;
	struct swa_frame *dst_frm, *src_frm;
	struct swa_frame_param *frm_param;
	struct swa_hdr_param *hdr_param;
	sprd_hdr_param_t alg_param;

	if (ipmpro_hanlde == NULL || in == NULL || out == NULL || param == NULL) {
		SWA_LOGE("fail to get input %p %p %p %p\n", ipmpro_hanlde, in, out, param);
		return -1;
	}

	cxt = (struct hdr_context_t *)ipmpro_hanlde;
	frm_param = (struct swa_frame_param *)param;
	hdr_param = &frm_param->hdr_param;
	memset(&alg_param, 0, sizeof(sprd_hdr_param_t));

	if (hdr_param->hdr_version == 3) {
		if (in->frame_num < 2 || in->frame_num > HDR3_CAP_MAX) {
			SWA_LOGE("fail to get valid input hdr frame num %d\n", in->frame_num);
			return -1;
		}
	} else if (hdr_param->hdr_version == 2) {
		if (in->frame_num < 2 || in->frame_num > HDR_CAP_MAX) {
			SWA_LOGE("fail to get valid input hdr frame num %d\n", in->frame_num);
			return -1;
		}
	}

	for (i = 0; i < in->frame_num; i++) {
		src_frm = &in->frms[i];
		alg_param.input[i].format = SPRD_CAMALG_IMG_NV21;
		alg_param.input[i].addr[0] = (void *)src_frm->addr_vir[0];
		alg_param.input[i].addr[1] = (void *)src_frm->addr_vir[1];
		alg_param.input[i].addr[2] = (void *)src_frm->addr_vir[2];
		alg_param.input[i].ion_fd = src_frm->fd;
		alg_param.input[i].offset[0] = 0;
		alg_param.input[i].offset[1] = cxt->pic_w * cxt->pic_h;
		alg_param.input[i].width = cxt->pic_w;
		alg_param.input[i].height = cxt->pic_h;
		alg_param.input[i].stride = cxt->pic_w;
		alg_param.input[i].size = cxt->pic_w * cxt->pic_h * 3 / 2;
		alg_param.ev[i] = hdr_param->ev[i];
		SWA_LOGD("input%d, size %d %d, ev %f, fd 0x%x, addr %p %p %p\n",
			i, cxt->pic_w, cxt->pic_h,  alg_param.ev[i], alg_param.input[i].ion_fd,
			alg_param.input[i].addr[0], alg_param.input[i].addr[1], alg_param.input[i].addr[2]);
	}

	dst_frm = &out->frms[0];
	alg_param.output.format = SPRD_CAMALG_IMG_NV21;
	alg_param.output.addr[0] = (void *)dst_frm->addr_vir[0];
	alg_param.output.addr[1] = (void *)dst_frm->addr_vir[1];
	alg_param.output.addr[2] = (void *)dst_frm->addr_vir[2];
	alg_param.output.ion_fd = dst_frm->fd;
	alg_param.output.offset[0] = 0;
	alg_param.output.offset[1] = cxt->pic_w * cxt->pic_h;
	alg_param.output.width = cxt->pic_w;
	alg_param.output.height = cxt->pic_h;
	alg_param.output.stride = cxt->pic_w;
	alg_param.output.size = cxt->pic_w * cxt->pic_h * 3 / 2;
	alg_param.callback = hdr_param->hdr_callback;
	alg_param.detect_out = hdr_param->hdr_callback;
	alg_param.sensor_ae = hdr_param->ae_exp_gain_info;
	alg_param.input_frame_num = in->frame_num;

	SWA_LOGD("output fd 0x%x, addr %p %p %p, alg_param.callback %p %p\n", alg_param.output.ion_fd,
		alg_param.output.addr[0], alg_param.output.addr[1], alg_param.output.addr[2],
		alg_param.callback,alg_param.sensor_ae);

	ret = sprd_hdr_adpt_ctrl(cxt->hdr_handle, SPRD_HDR_PROCESS_CMD, &alg_param);
	if (ret)
		SWA_LOGE("fail to process HDR\n");
	else
		SWA_LOGD("Done success\n");

	return ret;
}

int swa_hdr_close(void * ipmpro_hanlde,
			void * close_param)

{
	int ret = 0;
	struct hdr_context_t *cxt = NULL;

	if (ipmpro_hanlde == NULL)
		return -1;

	cxt = (struct hdr_context_t *)ipmpro_hanlde;

	ret = sprd_hdr_adpt_ctrl(cxt->hdr_handle, SPRD_HDR_FAST_STOP_CMD, NULL);
	if (ret) {
		SWA_LOGE("fail to fast stop HDR\n");
	}

	ret = sprd_hdr_adpt_deinit(cxt->hdr_handle);
	if (ret) {
		SWA_LOGE("fail to deinit HDR\n");
	}

	SWA_LOGD("Done");
	return ret;
}
#endif
#endif


#ifdef CONFIG_WIDE_ULTRAWIDE_SUPPORT

struct ultrawide_context_t {
	bool is_isp_zoom;
	INST_TAG tag;
	img_warp_param_t warp_param;
	img_warp_inst_t warp_inst;  // for uw algo api internal handle
};

int swa_uwarp_get_handle_size()
{
	return sizeof(struct ultrawide_context_t);
}

int swa_uwarp_process(void * ipmpro_hanlde,
			struct swa_frames_inout *in,
			struct swa_frames_inout *out,
			void * param)
{
	int ret = 0;
	int x, y, w, h;
	struct ultrawide_context_t *cxt = NULL;
	struct swa_frame *frm_in, *frm_out;
	struct swa_frame_param *frm_param;
	struct isp_warp_info *warp_info;
	img_warp_buffer_t input;
	img_warp_buffer_t output;
	img_warp_undistort_param_t proc_param, *p;

	if (ipmpro_hanlde == NULL || in == NULL || out == NULL || param == NULL) {
		SWA_LOGE("fail to get input %p %p %p %p\n", ipmpro_hanlde, in, out, param);
		return -1;
	}

	cxt = (struct ultrawide_context_t *)ipmpro_hanlde;
	frm_param = (struct swa_frame_param *)param;
	warp_info = &frm_param->warp_info;

	img_warp_grid_config_default(&cxt->warp_param);

	cxt->tag = warp_info->cap_tag ? WARP_CAPTURE : WARP_PREVIEW;
	cxt->warp_param.otp_buf = warp_info->otp_data;
	cxt->warp_param.otp_size = warp_info->otp_size;
	cxt->warp_param.input_info.binning_mode = (warp_info->binning_factor == 2) ? 1 : 0;
	cxt->warp_param.input_info.fullsize_width = warp_info->src_size.width;
	cxt->warp_param.input_info.fullsize_height = warp_info->src_size.height;
	cxt->warp_param.input_info.crop_x = warp_info->src_crop.start_x;
	cxt->warp_param.input_info.crop_y = warp_info->src_crop.start_y;
	cxt->warp_param.input_info.crop_width = warp_info->src_crop.width;
	cxt->warp_param.input_info.crop_height = warp_info->src_crop.height;
	cxt->warp_param.input_info.input_width = warp_info->in_size.width;
	cxt->warp_param.input_info.input_height = warp_info->in_size.height;
	cxt->warp_param.dst_width = warp_info->out_size.width;
	cxt->warp_param.dst_height = warp_info->out_size.height;

	SWA_LOGD("otp %p %d, bin %d, sn (%d %d), crop (%d %d %d %d), (%d %d) => (%d %d)\n",
		cxt->warp_param.otp_buf, cxt->warp_param.otp_size,
		cxt->warp_param.input_info.binning_mode,
		cxt->warp_param.input_info.fullsize_width, cxt->warp_param.input_info.fullsize_height,
		cxt->warp_param.input_info.crop_x, cxt->warp_param.input_info.crop_y,
		cxt->warp_param.input_info.crop_width, cxt->warp_param.input_info.crop_height,
		cxt->warp_param.input_info.input_width, cxt->warp_param.input_info.input_height,
		cxt->warp_param.dst_width, cxt->warp_param.dst_height);

	ret = sprd_warp_adapter_open(&cxt->warp_inst,
				&cxt->is_isp_zoom, &cxt->warp_param, cxt->tag);
	if (ret) {
		SWA_LOGE("fail to open warp adapter\n");
		goto exit;
	}

	frm_in = &in->frms[0];
	frm_out = &out->frms[0];
	if ((in->frame_num == 0) || (frm_in->fd <= 0) ||
			(out->frame_num == 0) || (frm_out->fd <= 0) ) {
		SWA_LOGE("fail to get in/out frame\n");
		goto close;
	}
	if (warp_info->dst_crop.width < 2 || warp_info->dst_crop.height < 2) {
		SWA_LOGE("fail to get valid crop size %d %d\n",
			warp_info->dst_crop.width, warp_info->dst_crop.height);
		goto close;
	}

	p = &proc_param;
	p->input_info.fullsize_width = warp_info->src_size.width;
	p->input_info.fullsize_height = warp_info->src_size.height;
	p->input_info.input_width = warp_info->in_size.width;
	p->input_info.input_height = warp_info->in_size.height;
	p->input_info.crop_x = ALIGN(warp_info->dst_crop.start_x, 2);
	p->input_info.crop_y = ALIGN(warp_info->dst_crop.start_y, 2);
	warp_info->dst_crop.width += (warp_info->dst_crop.start_x & 1);
	warp_info->dst_crop.height += (warp_info->dst_crop.start_y & 1);
	p->input_info.crop_width = ALIGN(warp_info->dst_crop.width, 2);
	p->input_info.crop_height = ALIGN(warp_info->dst_crop.height, 2);

	x = p->input_info.crop_x + p->input_info.crop_width / 2;
	y = p->input_info.crop_y + p->input_info.crop_height / 2;
	w = warp_info->src_size.width;
	h = warp_info->src_size.height;
	x -= (w / 2);
	y -= (h / 2);
	p->zoomRatio = (float)w / (float)p->input_info.crop_width;
	p->zoomCenterOffsetX = (float)x / ((float)w / 2.0);
	p->zoomCenterOffsetY = (float)y / ((float)h / 2.0);

	SWA_LOGD("param ratio %f, cent_off (%f %f), (%d %d), (%d %d), (%d %d %d %d)\n",
		p->zoomRatio, p->zoomCenterOffsetX, p->zoomCenterOffsetY,
		p->input_info.fullsize_width, p->input_info.fullsize_height,
		p->input_info.input_width, p->input_info.input_height,
		p->input_info.crop_x, p->input_info.crop_y,
		p->input_info.crop_width, p->input_info.crop_height);

	input.width = frm_in->size.width;
	input.height = frm_in->size.height;
	input.stride = frm_in->size.width;
	input.graphic_handle = frm_in->gpu_handle;
	input.ion_fd = frm_in->fd;
	input.addr[0] = (void *)frm_in->addr_vir[0];

	output.width = frm_out->size.width;
	output.height = frm_out->size.height;
	output.stride = frm_out->size.width;
	output.graphic_handle = frm_out->gpu_handle;
	output.ion_fd = frm_out->fd;
	output.addr[0] = (void *)frm_out->addr_vir[0];
	SWA_LOGD("in fd=0x%x, addr %p, gpu_ptr %p, (%d %d), out fd=0x%x, addr %p, gpu_ptr %p, (%d %d)\n",
		input.ion_fd, input.addr[0], input.graphic_handle, input.width, input.height,
		output.ion_fd, output.addr[0], output.graphic_handle, output.width, output.height);

	sprd_warp_adapter_run(cxt->warp_inst, &input, &output, (void *)p, cxt->tag);

close:
	sprd_warp_adapter_close(&cxt->warp_inst, cxt->tag);

exit:
	return ret;
}
#endif



#ifdef CONFIG_CAMERA_3DNR_CAPTURE

struct mfnr_context_t {
	uint32_t frame_num;
	uint32_t pic_w;
	uint32_t pic_h;
	uint32_t small_w;
	uint32_t small_h;
	sprd_camalg_device_type run_type;
	void * mfnr_handle;
};

struct mfnr_fixed_params {
	cmr_s32 threshold[4][6];
	cmr_s32 slope[4][6];
	cmr_s32 gain_thr[6];
};

static struct mfnr_fixed_params default_mfnr_param = {
	{{3, 4, 6, 9, 9, 9}, {3, 5, 6, 9, 9, 9}, {3, 5, 6, 9, 9, 9}, {2, 6, 7, 9, 9, 9}}, /* threshold[4][6]; */
	{{5, 6, 7, 9, 9, 9}, {5, 6, 7, 9, 9, 9}, {5, 6, 7, 9, 9, 9}, {5, 6, 6, 9, 9, 9}}, /* slope[4][6]; */
	{6, 12, 18, 24, 30, 36}, /* gain_thr[6]; */
};
static int slope_tmp[6] = { 4, 4, 4, 4, 4, 4 };
static int sigma_tmp[6] = { 4, 4, 4, 4, 4, 4 };

static struct isp_mfnr_info default_mfnr_tuning_param = {
	{ 0, 0, 0, 0 },
	{ 0, 0, 0, 0 },
	11, 11, /* searchWindow_x, searchWindow_y, */
	-1, /*  recur_str */
	0, 12, /* match_ratio_sad, match_ratio_pro */
	100, /* feat_thr */
	6, /* zone_size */
	665, 410, /* luma_ratio_high, luma_ratio_low */
	{ 0 }
};


int swa_mfnr_get_handle_size()
{
	return sizeof(struct mfnr_context_t);
}

int swa_mfnr_open(void *ipmpro_hanlde,
			void * open_param, enum swa_log_level log_level)
{
	int ret = 0;
	uint32_t valid_param = 0;
	struct mfnr_context_t *cxt = (struct mfnr_context_t *)ipmpro_hanlde;
	struct swa_init_data *init_param = (struct swa_init_data *)open_param;
	struct isp_mfnr_info *mfnr_param = NULL;
	struct __mfnr_memory_ops bufferpool_ops;
	mfnr_param_info_t param;
	mfnr_cmd_proc_t proc_data;
	mfnr_setparam_cmd_param_t *dst;
	struct mfnr_tuning_param_info cap_3dnr_tuning_param;

	if (!cxt || !init_param) {
		SWA_LOGE("fail to get input ptr %p %p\n", cxt, init_param);
		return -1;
	}

	cxt->pic_w = init_param->frame_size.width;
	cxt->pic_h = init_param->frame_size.height;
	cxt->small_w = init_param->frame_crop.width;
	cxt->small_h = init_param->frame_crop.height;
	cxt->frame_num = init_param->frm_total_num;

	memset(&param, 0, sizeof(mfnr_param_info_t));
	param.orig_width = cxt->pic_w;
	param.orig_height = cxt->pic_h;
	param.small_width = cxt->small_w;
	param.small_height = cxt->small_h;
	param.total_frame_num = cxt->frame_num;
	param.poutimg = NULL;
	param.gain = init_param->ae_again;
	param.low_thr = 100;
	param.ratio = 2;
	param.sigma_tmp = sigma_tmp;
	param.slope_tmp = slope_tmp;
	param.yuv_mode = 1;   // NV21
	param.control_en = 0x0;
	param.thread_num_acc = 4;
	param.thread_num = 4;
	param.preview_cpyBuf = 1;
	param.AF_ROI_start_x = init_param->af_ctrl_roi.start_x;
	param.AF_ROI_start_y = init_param->af_ctrl_roi.start_y;
	param.AF_ROI_width = init_param->af_ctrl_roi.width;
	param.AF_ROI_height = init_param->af_ctrl_roi.height;
	SWA_LOGD("af_roi: x=%d, y=%d, w=%d, h=%d", param.AF_ROI_start_x,param.AF_ROI_start_y,
		param.AF_ROI_width,param.AF_ROI_height);
	cap_3dnr_tuning_param.threthold = default_mfnr_param.threshold;
	cap_3dnr_tuning_param.slope = default_mfnr_param.slope;
	memcpy(cap_3dnr_tuning_param.gain_thr, default_mfnr_param.gain_thr, sizeof(cap_3dnr_tuning_param.gain_thr));
	param.productInfo = PLATFORM_ID;
	param.pMemoryOps = NULL;

	mfnr_param = (struct isp_mfnr_info *)init_param->pri_data;
	valid_param = mfnr_param->searchWindow_x | mfnr_param->searchWindow_x;
	valid_param |= mfnr_param->match_ratio_pro | mfnr_param->match_ratio_sad;
	valid_param |= mfnr_param->recur_str | mfnr_param->feat_thr | mfnr_param->zone_size;
	valid_param |= mfnr_param->luma_ratio_high | mfnr_param->luma_ratio_low;
	if (!valid_param) {
		SWA_LOGI("warning: no valid mfnr tuning param. will set default\n");
		memcpy(mfnr_param, &default_mfnr_tuning_param, sizeof(struct isp_mfnr_info));
	}

	cap_3dnr_tuning_param.SearchWindow_x = mfnr_param->searchWindow_x;
	cap_3dnr_tuning_param.SearchWindow_y = mfnr_param->searchWindow_y;
	cap_3dnr_tuning_param.recur_str = mfnr_param->recur_str;
	cap_3dnr_tuning_param.match_ratio_sad = mfnr_param->match_ratio_sad;
	cap_3dnr_tuning_param.match_ratio_pro = mfnr_param->match_ratio_pro;
	cap_3dnr_tuning_param.feat_thr = mfnr_param->feat_thr;
	cap_3dnr_tuning_param.luma_ratio_high = mfnr_param->luma_ratio_high;
	cap_3dnr_tuning_param.luma_ratio_low = mfnr_param->luma_ratio_low;
	cap_3dnr_tuning_param.zone_size = mfnr_param->zone_size;
	memcpy(cap_3dnr_tuning_param.reserved, mfnr_param->reserverd, sizeof(cap_3dnr_tuning_param.reserved));
	bufferpool_ops.malloc = init_param->heap_mem_malloc;
	bufferpool_ops.free = init_param->heap_mem_free;
	param.pMemoryOps = &bufferpool_ops;

	ret = sprd_mfnr_adpt_init(&cxt->mfnr_handle,  &param, (void *)(&cap_3dnr_tuning_param));
	if (ret) {
		SWA_LOGE("fail to init mfnr\n");
		return -1;
	}

	ret = sprd_mfnr_get_devicetype(&cxt->run_type);
	SWA_LOGD("mfnr handle %p, run_type %d\n", cxt->mfnr_handle, cxt->run_type);

	dst = &proc_data.proc_param.setpara_param;
	memcpy(dst->thr, mfnr_param->threshold, sizeof(dst->thr));
	memcpy(dst->slp, mfnr_param->slope, sizeof(dst->slp));
	ret = sprd_mfnr_adpt_ctrl(cxt->mfnr_handle,
				SPRD_MFNR_PROC_SET_PARAMS_CMD,
				(void *)&proc_data);
	if(ret) {
		SWA_LOGE("failed to set 3dnr init params. ret = %d", ret);
		ret |= sprd_mfnr_adpt_deinit(&cxt->mfnr_handle);
	}

	return ret;
}

int swa_mfnr_process(void * ipmpro_hanlde,
			struct swa_frames_inout *in,
			struct swa_frames_inout *out,
			void * param)
{
	int ret = 0, offset, i;
	struct mfnr_context_t *cxt = NULL;
	struct swa_frame *frm_in, *frm_out;
	mfnr_buffer_t small_image, big_image;
	mfnr_cap_gpu_buffer_t orig_image;
	mfnr_cmd_proc_t process_param;

	if (ipmpro_hanlde == NULL || in == NULL) {
		SWA_LOGE("fail to get input %p %p\n", ipmpro_hanlde, in);
		return -1;
	}

	cxt = (struct mfnr_context_t *)ipmpro_hanlde;
	frm_in = &in->frms[1];
	small_image.cpu_buffer.bufferY = (uint8_t *)frm_in->addr_vir[0];
	small_image.cpu_buffer.bufferU = (uint8_t *)frm_in->addr_vir[1];
	small_image.cpu_buffer.bufferV = small_image.cpu_buffer.bufferU;
	small_image.cpu_buffer.fd = (int32_t)frm_in->fd;
	frm_in = &in->frms[0];
	SWA_LOGD("buf fd=0x%x, vaddr_y 0x%lx, sbuf fd=0x%x, addr %p %p %p\n",
		frm_in->fd, frm_in->addr_vir[0],
		small_image.cpu_buffer.fd, small_image.cpu_buffer.bufferY,
		small_image.cpu_buffer.bufferU, small_image.cpu_buffer.bufferV);
	if(cxt->run_type != SPRD_CAMALG_RUN_TYPE_VDSP) {
		orig_image.gpuHandle = frm_in->gpu_handle;
		orig_image.bufferY = (uint8_t *)frm_in->addr_vir[0];
		orig_image.bufferU = (uint8_t *)frm_in->addr_vir[1];
		orig_image.bufferV = orig_image.bufferU;
		process_param.proc_param.cap_new_param.small_image = &small_image;
		process_param.proc_param.cap_new_param.orig_image = &orig_image;
		process_param.callWay = 1;
	} else {
		big_image.cpu_buffer.bufferY = (uint8_t *)frm_in->addr_vir[0];
		big_image.cpu_buffer.bufferU = (uint8_t *)frm_in->addr_vir[1];
		big_image.cpu_buffer.bufferV = big_image.cpu_buffer.bufferU;
		big_image.cpu_buffer.fd = (int32_t)frm_in->fd;
		process_param.proc_param.cap_param.small_image = &small_image;
		process_param.proc_param.cap_param.orig_image = &big_image;
		process_param.callWay = 0;
	}

	ret = sprd_mfnr_adpt_ctrl(cxt->mfnr_handle,
				SPRD_MFNR_PROC_CAPTURE_CMD,
				(void *)&process_param);
	if (ret )
		SWA_LOGE("Fail to call mfnr process capture, ret =%d",ret);

	return ret;
}


int swa_mfnr_close(void * ipmpro_hanlde,
			void * close_param)
{
	int ret = 0;
	struct mfnr_context_t *cxt = (struct mfnr_context_t *)ipmpro_hanlde;

	if (ipmpro_hanlde == NULL)
		return -1;

	SWA_LOGD("mfnr handle %p\n", cxt->mfnr_handle);
	if (cxt->mfnr_handle == NULL)
		return ret;

	ret = sprd_mfnr_adpt_deinit(&cxt->mfnr_handle);
	SWA_LOGD("Done. ret %d, handle %p\n", ret, cxt->mfnr_handle);
	cxt->mfnr_handle = NULL;
	return ret;
}
#endif



#ifdef CONFIG_CAMERA_MFSR_CAPTURE

struct mfsr_context_t {
	uint32_t frame_num;
	uint32_t frame_cnt;
	uint32_t pic_w;
	uint32_t pic_h;
	mfsr_exif_t exif_info;
	void * mfsr_handle;
};

int swa_mfsr_get_handle_size()
{
	return sizeof(struct mfsr_context_t);
}

int swa_mfsr_open(void *ipmpro_hanlde,
			void * open_param, enum swa_log_level log_level)
{
	int ret = 0;
	struct mfsr_context_t *cxt = (struct mfsr_context_t *)ipmpro_hanlde;
	struct swa_init_data *init_param = (struct swa_init_data *)open_param;
	MFSR_CMD_OPEN_PARAM_T cmd_param;
	mfsr_open_param mfsr_param;
	MFSR_CMD_GET_VERSION_PARAM_T cmd_ver;
	mfsr_lib_version lib_version;

	if (!cxt || !init_param) {
		SWA_LOGE("fail to get input ptr %p %p\n", cxt, init_param);
		return -1;
	}

	cmd_ver.lib_version = &lib_version;
	ret = sprd_mfsr_adapter_ctrl(MFSR_CMD_GET_VERSION, &cmd_ver);
	if (ret) {
		SWA_LOGE("fail to get mfsr version\n");
		return -1;
	}
	SWA_LOGD("mfsr version %d.%d.%d.%d, build date %s, time %s rev %s\n",
		lib_version.major, lib_version.minor, lib_version.micro, lib_version.nano,
		lib_version.built_date, lib_version.built_time, lib_version.built_rev);

	cxt->pic_w = init_param->frame_size.width;
	cxt->pic_h = init_param->frame_size.height;
	cxt->frame_num = init_param->frm_total_num;

	memset(&mfsr_param, 0, sizeof(mfsr_open_param));
	mfsr_param.max_width = init_param->frame_size.width;
	mfsr_param.max_height = init_param->frame_size.height;
	mfsr_param.tuning_param = init_param->pri_data;
	mfsr_param.tuning_param_size = init_param->pri_data_size;
	mfsr_param.sr_mode = 0;
	cmd_param.ctx = &cxt->mfsr_handle;
	cmd_param.param = &mfsr_param;

	ret = sprd_mfsr_adapter_ctrl(MFSR_CMD_OPEN, &cmd_param);
	if (ret || (*cmd_param.ctx) == NULL) {
		SWA_LOGE("fail to init mfsr\n");
		return -1;
	}
	SWA_LOGD("done. handle %p\n", cxt->mfsr_handle);

	return 0;
}

int swa_mfsr_process(void * ipmpro_hanlde,
			struct swa_frames_inout *in,
			struct swa_frames_inout *out,
			void * param)
{
	int ret = 0;
	struct mfsr_context_t *cxt = NULL;
	struct swa_frame_param *frm_param;
	struct swa_frame *frm_in, *frm_out, *frm_detail;
	MFSR_CMD_PROCESS_PARAM_T cmd_param;
	mfsr_process_param proc_param;
	sprd_camalg_image_t image_in;
	sprd_camalg_image_t image_out, image_detail;
	MFSR_CMD_GET_EXIF_PARAM_T cmd_exif;

	if (ipmpro_hanlde == NULL || in == NULL || out == NULL) {
		SWA_LOGE("fail to get input %p %p %p\n", ipmpro_hanlde, in, out);
		return -1;
	}

	cxt = (struct mfsr_context_t *)ipmpro_hanlde;
	frm_param = (struct swa_frame_param *)param;

	frm_in = &in->frms[0];
	frm_out = &out->frms[0];
	frm_detail = &out->frms[1];

	proc_param.gain = 1.0 * frm_param->common_param.again;
	proc_param.exposure_time = 1.0 * frm_param->common_param.exp_time;
	proc_param.crop_area.x = frm_param->mfsr_param.frame_crop.start_x;
	proc_param.crop_area.y = frm_param->mfsr_param.frame_crop.start_y;
	proc_param.crop_area.w = frm_param->mfsr_param.frame_crop.width;
	proc_param.crop_area.h = frm_param->mfsr_param.frame_crop.height;

	memset(&image_in, 0, sizeof(sprd_camalg_image_t));
	image_in.format = SPRD_CAMALG_IMG_NV21;
	image_in.ion_fd = frm_in->fd;
	image_in.addr[0] = (void *)frm_in->addr_vir[0];
	image_in.addr[1] = (void *)frm_in->addr_vir[1];
	image_in.width = frm_in->size.width;
	image_in.height = frm_in->size.height;
	image_in.stride = frm_in->size.width;
	image_in.size = image_in.width * image_in.height * 3 / 2;

	memcpy(&image_out, &image_in, sizeof(sprd_camalg_image_t));
	image_out.ion_fd = frm_out->fd;
	image_out.addr[0] = (void *)frm_out->addr_vir[0];
	image_out.addr[1] = (void *)frm_out->addr_vir[1];

	memcpy(&image_detail, &image_in, sizeof(sprd_camalg_image_t));
	image_detail.ion_fd = frm_detail->fd;
	image_detail.addr[0] = (void *)frm_detail->addr_vir[0];
	image_detail.addr[1] = (void *)frm_detail->addr_vir[1];

	cmd_param.ctx = &cxt->mfsr_handle;
	cmd_param.image_input = &image_in;
	cmd_param.image_output = &image_out;
	cmd_param.detail_mask = &image_detail;
	cmd_param.denoise_mask = NULL;
	cmd_param.process_param = &proc_param;

	SWA_LOGD("in fd 0x%x, addr %p, %p, out fd 0x%x, addr %p %p, detail fd 0x%x, addr %p %p\n",
		image_in.ion_fd, image_in.addr[0], image_in.addr[1],
		image_out.ion_fd, image_out.addr[0], image_out.addr[1],
		image_detail.ion_fd, image_detail.addr[0], image_detail.addr[1]);
	SWA_LOGD("param gain %f, exp time %f, size %d %d, crop %d %d %d %d\n",
		proc_param.gain, proc_param.exposure_time, image_in.width, image_in.height,
		proc_param.crop_area.x, proc_param.crop_area.y,
		proc_param.crop_area.w, proc_param.crop_area.h);

	ret = sprd_mfsr_adapter_ctrl(MFSR_CMD_PROCESS, &cmd_param);
	if (ret )
		SWA_LOGE("Fail to call mfsr process capture, ret =%d", ret);

	cxt->frame_cnt++;
	if (cxt->frame_cnt < cxt->frame_num)
		goto exit;

	cmd_exif.ctx = &cxt->mfsr_handle;
	cmd_exif.exif_info = &cxt->exif_info;

	ret = sprd_mfsr_adapter_ctrl(MFSR_CMD_GET_EXIF, &cmd_exif);
	if (ret )
		SWA_LOGE("Fail to call mfsr get exif, ret =%d",ret);

	SWA_LOGD("exif 0x%x, %d %d, (%d %d) (%d %d), gain %d ex time %d, ratio %d %d\n",
		cxt->exif_info.version,
		cxt->exif_info.merge_num, cxt->exif_info.sharpen_field,
		cxt->exif_info.im_input_w, cxt->exif_info.im_input_h,
		cxt->exif_info.im_output_w, cxt->exif_info.im_output_h,
		cxt->exif_info.gain, cxt->exif_info.exposure_time,
		cxt->exif_info.scale, cxt->exif_info.sharpen_level);
	frm_param->mfsr_param.out_exif_ptr = &cxt->exif_info;
	frm_param->mfsr_param.out_exif_size = sizeof(mfsr_exif_t);

exit:
	SWA_LOGD("frame no.%d done\n", cxt->frame_cnt);
	return ret;
}


int swa_mfsr_close(void * ipmpro_hanlde,
			void * close_param)
{
	int ret = 0;
	struct mfsr_context_t *cxt = (struct mfsr_context_t *)ipmpro_hanlde;
	MFSR_CMD_CLOSE_PARAM_T cmd_param;

	if (ipmpro_hanlde == NULL)
		return -1;

	SWA_LOGD("mfsr handle %p\n", cxt->mfsr_handle);
	if (cxt->mfsr_handle == NULL)
		return ret;

	cmd_param.ctx = &cxt->mfsr_handle;
	ret = sprd_mfsr_adapter_ctrl(MFSR_CMD_CLOSE, &cmd_param);
	SWA_LOGD("Done. ret %d, handle %p\n", ret, cxt->mfsr_handle);
	cxt->mfsr_handle = NULL;
	return ret;
}



struct sharp_context_t {
	void * sharp_handle;
};

int swa_sharp_get_handle_size()
{
	return sizeof(struct sharp_context_t);
}

int swa_sharp_process(void * ipmpro_hanlde,
			struct swa_frames_inout *in,
			struct swa_frames_inout *out,
			void * param)
{
	int ret = 0;
	struct sharp_context_t *cxt = NULL;
	struct swa_frame_param *frm_param;
	struct swa_frame *frm_in, *frm_detail;
	MFSR_CMD_OPEN_PARAM_T open_param;
	mfsr_open_param mfsr_param;
	MFSR_CMD_CLOSE_PARAM_T close_param;
	MFSR_CMD_POSTPROCESS_PARAM_T cmd_param;
	mfsr_process_param proc_param;
	sprd_camalg_image_t image_in;
	sprd_camalg_image_t image_detail;

	if (ipmpro_hanlde == NULL || in == NULL) {
		SWA_LOGE("fail to get input %p %p\n", ipmpro_hanlde, in);
		return -1;
	}

	cxt = (struct sharp_context_t *)ipmpro_hanlde;
	frm_param = (struct swa_frame_param *)param;

	memset(&mfsr_param, 0, sizeof(mfsr_open_param));
	mfsr_param.max_width = frm_param->mfsr_param.frame_size.width;
	mfsr_param.max_height = frm_param->mfsr_param.frame_size.height;
	mfsr_param.tuning_param = frm_param->mfsr_param.data;
	mfsr_param.tuning_param_size = frm_param->mfsr_param.data_size;
	mfsr_param.sr_mode = 0;
	open_param.ctx = &cxt->sharp_handle;
	open_param.param = &mfsr_param;
	SWA_LOGD("size %d %d, tuning pm %p, %d\n", mfsr_param.max_width,
		mfsr_param.max_height, mfsr_param.tuning_param, mfsr_param.tuning_param_size);

	ret = sprd_mfsr_adapter_ctrl(MFSR_CMD_POST_OPEN, &open_param);
	if (ret || (*open_param.ctx) == NULL) {
		SWA_LOGE("fail to init mfsr post\n");
		return -1;
	}
	SWA_LOGD("sharp_handle %p\n", cxt->sharp_handle);

	frm_in = &in->frms[0];
	frm_detail = &in->frms[1];

	proc_param.gain = 1.0 * frm_param->common_param.again;
	proc_param.exposure_time = 1.0 * frm_param->common_param.exp_time;
	proc_param.crop_area.x = 0;
	proc_param.crop_area.y = 0;
	proc_param.crop_area.w = frm_in->size.width;
	proc_param.crop_area.h = frm_in->size.height;

	proc_param.gain = 1.0 * frm_param->common_param.again;
	proc_param.exposure_time = 1.0 * frm_param->common_param.exp_time;
	proc_param.crop_area.x = frm_param->mfsr_param.frame_crop.start_x;
	proc_param.crop_area.y = frm_param->mfsr_param.frame_crop.start_y;
	proc_param.crop_area.w = frm_param->mfsr_param.frame_crop.width;
	proc_param.crop_area.h = frm_param->mfsr_param.frame_crop.height;

	memset(&image_in, 0, sizeof(sprd_camalg_image_t));
	image_in.format = SPRD_CAMALG_IMG_NV21;
	image_in.ion_fd = frm_in->fd;
	image_in.addr[0] = (void *)frm_in->addr_vir[0];
	image_in.addr[1] = (void *)frm_in->addr_vir[1];
	image_in.width = frm_in->size.width;
	image_in.height = frm_in->size.height;
	image_in.stride = frm_in->size.width;
	image_in.size = image_in.width * image_in.height * 3 / 2;

	memcpy(&image_detail, &image_in, sizeof(sprd_camalg_image_t));
	image_detail.ion_fd = frm_detail->fd;
	image_detail.addr[0] = (void *)frm_detail->addr_vir[0];
	image_detail.addr[1] = (void *)frm_detail->addr_vir[1];

	cmd_param.ctx = &cxt->sharp_handle;
	cmd_param.image = &image_in;
	cmd_param.detail_mask = &image_detail;
	cmd_param.denoise_mask = NULL;
	cmd_param.process_param = &proc_param;

	SWA_LOGD("in fd 0x%x, addr %p, %p, detail fd 0x%x, addr %p %p\n",
		image_in.ion_fd, image_in.addr[0], image_in.addr[1],
		image_detail.ion_fd, image_detail.addr[0], image_detail.addr[1]);

	SWA_LOGD("param gain %f, exp time %f, size %d %d, crop %d %d %d %d\n",
		proc_param.gain, proc_param.exposure_time, image_in.width, image_in.height,
		proc_param.crop_area.x, proc_param.crop_area.y,
		proc_param.crop_area.w, proc_param.crop_area.h);

	ret = sprd_mfsr_adapter_ctrl(MFSR_CMD_POST_PROCESS, &cmd_param);
	if (ret )
		SWA_LOGE("Fail to call mfsr post process, ret =%d",ret);

	close_param.ctx = &cxt->sharp_handle;
	ret = sprd_mfsr_adapter_ctrl(MFSR_CMD_POST_CLOSE, &close_param);
	SWA_LOGD("Close done. ret %d, handle %p\n", ret, cxt->sharp_handle);
	cxt->sharp_handle = NULL;

	return ret;
}
#endif


#ifdef CONFIG_CAMERA_CNR

struct cnr_context_t {
	uint32_t frame_num;
	uint32_t pic_w;
	uint32_t pic_h;

	void * cnr_handle;  // for cnr/ynrs algo api internal handle
};

int swa_cnr_get_handle_size()
{
	return sizeof(struct cnr_context_t);
}

int swa_cnr_open(void *ipmpro_hanlde,
			void * open_param, enum swa_log_level log_level)
{
	int ret = 0;
	struct cnr_context_t *cxt = (struct cnr_context_t *)ipmpro_hanlde;
	struct swa_init_data *init_param = (struct swa_init_data *)open_param;
	sprd_yuv_denoise_init_t param;

	if (!cxt || !init_param) {
		SWA_LOGE("fail to get input ptr %p %p\n", cxt, init_param);
		return -1;
	}

	param.width = init_param->frame_size.width;
	param.height = init_param->frame_size.height;
	param.runversion = 1;

	cxt->pic_w = init_param->frame_size.width;
	cxt->pic_h = init_param->frame_size.height;
	param.memory_ops.malloc = init_param->heap_mem_malloc;
	param.memory_ops.free = init_param->heap_mem_free;
	cxt->cnr_handle = sprd_yuv_denoise_adpt_init((void *)&param);
	if (cxt->cnr_handle == NULL) {
		SWA_LOGE("fail to init yuv_denoise\n");
		return -1;
	}

	SWA_LOGD("cnr handle %p\n", cxt->cnr_handle);
	return ret;
}

int swa_cnr_process(void * ipmpro_hanlde,
			struct swa_frames_inout *in,
			struct swa_frames_inout *out,
			void * param)
{
	int ret = 0, i;
	struct cnr_context_t *cxt = NULL;
	struct swa_frame *frm_in, *frm_out;
	struct swa_frame_param *frm_param;
	YNR_Param ynr_param;
	CNR_Parameter cnr2_param;
	cnr_param_t cnr3_param;
	cmr_u32 mode = 0;
	sprd_yuv_denoise_param_t denoise_param;

	if (ipmpro_hanlde == NULL || in == NULL || out == NULL || param == NULL) {
		SWA_LOGE("fail to get input %p %p %p %p\n", ipmpro_hanlde, in, out, param);
		return -1;
	}

	cxt = (struct cnr_context_t *)ipmpro_hanlde;
	frm_param = (struct swa_frame_param *)param;

	memset(&denoise_param, 0, sizeof(sprd_yuv_denoise_param_t));

	if (!frm_param->ynrs_info.bypass) {
		mode |= 1;
		memcpy(&ynr_param, &frm_param->ynrs_info.ynrs_param, sizeof(YNR_Param));
		denoise_param.ynr_ration_base = frm_param->ynrs_info.radius_base;
		denoise_param.ynrParam = &ynr_param;
	}
	if (!frm_param->cnr3_info.bypass) {
		mode |= 4;
		cnr3_param.bypass = 0;
		memcpy(&cnr3_param.paramLayer[0],
			&frm_param->cnr3_info.param_layer[0],
			sizeof(cnr3_param.paramLayer));
		denoise_param.cnr_ration_base = frm_param->cnr3_info.baseRadius;
		denoise_param.cnr3Param = &cnr3_param;
	}
	if (!frm_param->cnr2_info.bypass) {
		mode |= 2;
		memcpy(&cnr2_param, &frm_param->cnr2_info.param, sizeof(CNR_Parameter));
		denoise_param.cnr2Param = &cnr2_param;
	}

	SWA_LOGD("cnr handle %p, mode %d\n", cxt->cnr_handle, mode);
	if (cxt->cnr_handle == NULL || mode == 0)
		return ret;

	i = 0;
next:
	frm_in = &in->frms[i];
	frm_out = &out->frms[i];
	SWA_LOGD("frm No.%d, %p %p, fd 0x%x 0x%x, addr %p %p\n", i, frm_in, frm_out,
		frm_in->fd, frm_out->fd,(void *)frm_in->addr_vir[0], (void *)frm_in->addr_vir[1]);

	denoise_param.bufferY.addr[0] = (void *)frm_in->addr_vir[0];
	denoise_param.bufferUV.addr[0] = (void *)frm_in->addr_vir[1];
	denoise_param.bufferY.ion_fd = frm_in->fd;
	denoise_param.bufferUV.ion_fd = frm_in->fd;
	denoise_param.width = frm_in->size.width;
	denoise_param.height = frm_in->size.height;
	denoise_param.zoom_ratio = frm_param->common_param.zoom_ratio;

	ret = sprd_yuv_denoise_adpt_ctrl(cxt->cnr_handle, (sprd_yuv_denoise_cmd_t)mode, (void *)&denoise_param);
	if (ret)
		SWA_LOGE("fail to process Y/CNR\n");
	else
		SWA_LOGD("Done success\n");
	i++;
	if (i < in->frame_num)
		goto next;

	return ret;
}


int swa_cnr_close(void * ipmpro_hanlde,
			void * close_param)

{
	int ret = 0;
	struct cnr_context_t *cxt = (struct cnr_context_t *)ipmpro_hanlde;

	if (ipmpro_hanlde == NULL)
		return -1;

	SWA_LOGD("cnr handle %p\n", cxt->cnr_handle);
	if (cxt->cnr_handle == NULL)
		return ret;

	ret = sprd_yuv_denoise_adpt_deinit(cxt->cnr_handle);
	if (ret)
		SWA_LOGE("fail to deinit cnr process\n");
	cxt->cnr_handle = NULL;

	SWA_LOGD("Done");
	return ret;
}
#endif



#ifdef CONFIG_CAMERA_DRE_PRO

struct dre_context_t {
	void * dre_handle;
};

int swa_drepro_get_handle_size()
{
	return sizeof(struct dre_context_t);
}

int swa_drepro_open(void *ipmpro_hanlde,
			void * open_param, enum swa_log_level log_level)
{
	int ret = 0;
	struct dre_context_t *cxt = NULL, local_cxt;
	struct swa_init_data *init_param = (struct swa_init_data *)open_param;
	sprd_dre_pro_memory memOps;

	if (ipmpro_hanlde)
		cxt = (struct dre_context_t *)ipmpro_hanlde;
	else
		cxt = &local_cxt;

	memOps.malloc = init_param->heap_mem_malloc;
	memOps.free = init_param->heap_mem_free;
	ret = sprd_dre_pro_adpt_init(&cxt->dre_handle,
				init_param->frame_size.width, init_param->frame_size.height, &memOps);
	if (ret) {
		SWA_LOGE("fail to init dre ret %d\n", ret);
		return -1;
	}
	SWA_LOGD("dre handle %p\n", cxt->dre_handle);
	return ret;
}

int swa_drepro_process(void * ipmpro_hanlde,
			struct swa_frames_inout *in,
			struct swa_frames_inout *out,
			void * param)
{
	int ret = 0;
	struct dre_context_t *cxt = NULL, local_cxt;
	struct swa_frame *frm_in;
	struct swa_frame_param *frm_param;
	struct sprd_camalg_image image_in;

	if (in == NULL || param == NULL) {
		SWA_LOGE("fail to get input %p %p\n", in, param);
		return -1;
	}

	if (in->frame_num == 0) {
		SWA_LOGE("fail to get in/out frame num %d %d\n", in->frame_num, out->frame_num);
		return -1;
	}
	if (ipmpro_hanlde)
		cxt = (struct dre_context_t *)ipmpro_hanlde;
	else
		cxt = &local_cxt;
	frm_param = (struct swa_frame_param *)param;
	frm_in = &in->frms[0];
	
	memset(&image_in, 0, sizeof(struct sprd_camalg_image));
	image_in.addr[0] = (void *)frm_in->addr_vir[0];
	image_in.addr[1] = (void *)frm_in->addr_vir[1];
	image_in.width = frm_in->size.width;
	image_in.height = frm_in->size.height;
	image_in.format = SPRD_CAMALG_IMG_NV21;
	ret = sprd_dre_pro_adpt_ctrl(cxt->dre_handle, SPRD_DRE_PROCESS_CMD,
				&image_in, &frm_param->dre_pro_param);
	if (ret) {
		SWA_LOGD("fail to do DRE	process ret %d\n", ret);
	}

	return ret;
}

int swa_drepro_close(void * ipmpro_hanlde,
			void * close_param)
{
	int ret = 0;
	struct dre_context_t *cxt = (struct dre_context_t *)ipmpro_hanlde;

	if (ipmpro_hanlde == NULL)
		return -1;
	SWA_LOGD("dre handle %p\n", cxt->dre_handle);
	if (cxt->dre_handle == NULL)
		return ret;

	ret = sprd_dre_pro_adpt_ctrl(cxt->dre_handle, SPRD_DRE_FAST_STOP_CMD, NULL, NULL);
	ret = sprd_dre_pro_adpt_deinit(cxt->dre_handle);
	if (ret) {
		SWA_LOGE("failed to deinit");
	}
	cxt->dre_handle = NULL;
	SWA_LOGD("Done");
	return ret;
}

#endif


#ifdef CONFIG_FACE_BEAUTY

struct fb_context_t {
	uint32_t frame_num;
	uint32_t pic_w;
	uint32_t pic_h;

	fb_beauty_param_t fb_handle;
};

int swa_fb_get_handle_size()
{
	return sizeof(struct fb_context_t);
}

int swa_fb_open(void *ipmpro_hanlde,
			void * open_param, enum swa_log_level log_level)
{
	int ret = 0;
	struct fb_context_t *cxt = (struct fb_context_t *)ipmpro_hanlde;
	struct swa_init_data *init_param = (struct swa_init_data *)open_param;

	if (!cxt || !init_param) {
		SWA_LOGE("fail to get input ptr %p %p\n", cxt, init_param);
		return -1;
	}

	face_beauty_set_devicetype(&cxt->fb_handle, SPRD_CAMALG_RUN_TYPE_CPU);
	face_beauty_init(&cxt->fb_handle, FB_WORKMODE_STILL, 2, 0);

	SWA_LOGD("fb handle %p, frame size %d %d\n", &cxt->fb_handle,
		init_param->frame_size.width, init_param->frame_size.height);
	return ret;
}

int swa_fb_process(void * ipmpro_hanlde,
			struct swa_frames_inout *in,
			struct swa_frames_inout *out,
			void * param)
{
	int ret = 0, i, face_cnt;
	int w, h;
	float ratio;
	struct fb_context_t *cxt = NULL;
	struct swa_frame_param *frm_param;
	struct isp_fb_info *fb_info;
	struct isp_face_area *src;
	struct swa_frame *frm_in, *frm_out;
	fbBeautyFacetT beauty_face;
	fb_beauty_image_t beauty_image;
	faceBeautyLevelsT beautyLevels;

	if (ipmpro_hanlde == NULL || in == NULL || out == NULL || param == NULL) {
		SWA_LOGE("fail to get input %p %p %p %p\n", ipmpro_hanlde, in, out, param);
		return -1;
	}

	cxt = (struct fb_context_t *)ipmpro_hanlde;
	frm_param = (struct swa_frame_param *)param;
	fb_info = &frm_param->fb_info;
	src = &frm_param->face_param;
	frm_in = &in->frms[0];

	if (fb_info->bypass || (src->face_num == 0)) {
		SWA_LOGD("fb bypass %d, face_num %d\n", fb_info->bypass, src->face_num);
		return 0;
	}

	if (fb_info->param_valid) {
		facebeauty_param_info_t fb_param_map;
		memcpy(&fb_param_map, &fb_info->param, sizeof(facebeauty_param_info_t));
		ret = face_beauty_ctrl(&cxt->fb_handle,
				FB_BEAUTY_CONSTRUCT_FACEMAP_CMD,
				(void *)&fb_param_map);
	}

	for (i = 0; i < src->face_num; i++) {
		beauty_face.idx = i;
		w = src->face_info[i].ex - src->face_info[i].sx;
		h = src->face_info[i].ey - src->face_info[i].sy;
		ratio = (float)frm_in->size.width / (float)(src->frame_width);
		beauty_face.startX = (int)(src->face_info[i].sx * ratio);
		beauty_face.startY = (int)(src->face_info[i].sy * ratio);
		beauty_face.endX = (int)(beauty_face.startX + w * ratio);
		beauty_face.endY = (int)(beauty_face.startY + h * ratio);
		beauty_face.angle = src->face_info[i].angle;
		beauty_face.pose = src->face_info[i].pose;
		SWA_LOGD("src face_info(idx[%d]:sx,ex(%d, %d) sy,ey(%d, %d)", i,
			src->face_info[i].sx, src->face_info[i].ex,
			src->face_info[i].sy, src->face_info[i].ey);
		SWA_LOGD("dst face_info(idx[%d]:w,h(%d, %d) sx,ex(%d, %d) sy,ey(%d, %d)", i,
			beauty_face.endX - beauty_face.startX,
			beauty_face.endY - beauty_face.startY,
			beauty_face.startX, beauty_face.endX,
			beauty_face.startY, beauty_face.endY);
		ret = face_beauty_ctrl(&cxt->fb_handle,
				FB_BEAUTY_CONSTRUCT_FACE_CMD,
				(void *)&beauty_face);
	}

	beauty_image.inputImage.format = SPRD_CAMALG_IMG_NV21;
	beauty_image.inputImage.addr[0] = (void *)frm_in->addr_vir[0];
	beauty_image.inputImage.addr[1] = (void *)frm_in->addr_vir[1];
	beauty_image.inputImage.addr[2] = (void *)frm_in->addr_vir[2];
	beauty_image.inputImage.ion_fd = frm_in->fd;
	beauty_image.inputImage.offset[0] = 0;
	beauty_image.inputImage.offset[1] = frm_in->size.width * frm_in->size.height;
	beauty_image.inputImage.width = frm_in->size.width;
	beauty_image.inputImage.height = frm_in->size.height;
	beauty_image.inputImage.stride = frm_in->size.width;
	beauty_image.inputImage.size = frm_in->size.width * frm_in->size.height * 3 / 2;
	SWA_LOGD("image fmt %d, fd 0x%x, addr %p %p, off %d %d, (%d x %d), stride %d, size %d\n",
		beauty_image.inputImage.format, beauty_image.inputImage.ion_fd,
		beauty_image.inputImage.addr[0], beauty_image.inputImage.addr[1],
		beauty_image.inputImage.offset[0], beauty_image.inputImage.offset[1],
		beauty_image.inputImage.width, beauty_image.inputImage.height,
		beauty_image.inputImage.stride, beauty_image.inputImage.size);
	ret = face_beauty_ctrl(&cxt->fb_handle,
				FB_BEAUTY_CONSTRUCT_IMAGE_CMD,
	                     (void *)&beauty_image);

	beautyLevels.blemishLevel = fb_info->levels.blemishLevel;
	beautyLevels.smoothLevel = fb_info->levels.smoothLevel;
	beautyLevels.skinColor = fb_info->levels.skinColor;
	beautyLevels.skinLevel = fb_info->levels.skinLevel;
	beautyLevels.brightLevel = fb_info->levels.brightLevel;
	beautyLevels.lipColor = fb_info->levels.lipColor;
	beautyLevels.lipLevel = fb_info->levels.lipLevel;
	beautyLevels.slimLevel = fb_info->levels.slimLevel;
	beautyLevels.largeLevel = fb_info->levels.largeLevel;
	beautyLevels.cameraBV = frm_param->common_param.bv;
	beautyLevels.cameraCT = frm_param->common_param.ct;
	beautyLevels.cameraISO = frm_param->common_param.iso;
	beautyLevels.cameraWork = frm_param->common_param.cam_id;

	struct isp_beauty_levels *fb_level = &fb_info->levels;
	SWA_LOGD("parm addr %p, %p, ---fb_level %d %d %d %d,  %d %d %d %d %d\n",
		frm_param, fb_level,
		fb_level->blemishLevel,
		fb_level->smoothLevel,
		fb_level->skinColor,
		fb_level->skinLevel, ///
		fb_level->brightLevel,
		fb_level->lipColor,
		fb_level->lipLevel,
		fb_level->slimLevel,
		fb_level->largeLevel);

	SWA_LOGD("%d %d %d %d,  %d %d %d %d %d, BV: %d %d %d %d\n",
		beautyLevels.blemishLevel,
		beautyLevels.smoothLevel,
		beautyLevels.skinColor,
		beautyLevels.skinLevel, //
		beautyLevels.brightLevel,
		beautyLevels.lipColor,
		beautyLevels.lipLevel,
		beautyLevels.slimLevel,
		beautyLevels.largeLevel, //
		beautyLevels.cameraBV,
		beautyLevels.cameraCT,
		beautyLevels.cameraISO,
		beautyLevels.cameraWork);
	ret = face_beauty_ctrl(&cxt->fb_handle,
				FB_BEAUTY_CONSTRUCT_LEVEL_CMD,
				(void *)&beautyLevels);

	face_cnt = src->face_num;
	SWA_LOGD("face num %d\n", face_cnt);
	ret = face_beauty_ctrl(&cxt->fb_handle,
				FB_BEAUTY_PROCESS_CMD,
				(void *)&face_cnt);

	SWA_LOGD("Done");
	return ret;
}

int swa_fb_close(void * ipmpro_hanlde,
			void * close_param)

{
	int ret = 0;
	struct fb_context_t *cxt = (struct fb_context_t *)ipmpro_hanlde;

	if (ipmpro_hanlde == NULL)
		return -1;

	SWA_LOGD("fb handle %p\n", &cxt->fb_handle);

	face_beauty_deinit(&(cxt->fb_handle));

	SWA_LOGD("Done");
	return ret;
}
#endif



#ifdef CONFIG_CAMERA_FILTER

struct filter_context_t {
	void *filter_handle;
};

int swa_filter_get_handle_size()
{
	return sizeof(struct filter_context_t);
}

int swa_filter_process(void * ipmpro_hanlde,
			struct swa_frames_inout *in,
			struct swa_frames_inout *out,
			void * param)
{
	int ret = 0;
	struct filter_context_t *cxt = NULL;
	struct swa_frame_param *frm_param;
	struct swa_frame *frm_in, *frm_out;
	IFInitParam initPara;
	IFImageData inputData, outputData;
	FilterParam_t filter_param;

	if (ipmpro_hanlde == NULL || in == NULL || out == NULL || param == NULL) {
		SWA_LOGE("fail to get input %p %p %p %p\n", ipmpro_hanlde, in, out, param);
		return -1;
	}

	cxt = (struct filter_context_t *)ipmpro_hanlde;
	frm_param = (struct swa_frame_param *)param;
	frm_in = &in->frms[0];
	frm_out = &out->frms[0];

	memset(&initPara, 0, sizeof(IFInitParam));
	initPara.imageType = NV21;
	initPara.imageWidth = frm_in->size.width;
	initPara.imageHeight = frm_in->size.height;
	initPara.paramPath = NULL;
	cxt->filter_handle = ImageFilterCreateEngine(&initPara);

	memset(&inputData, 0, sizeof(IFImageData));
	memset(&outputData, 0, sizeof(IFImageData));
	inputData.c1 = (void *)frm_in->addr_vir[0];
	inputData.c2 = (void *)frm_in->addr_vir[1];
	inputData.c3 = NULL;
	outputData.c1 = (void *)frm_out->addr_vir[0];
	outputData.c2 = (void *)frm_out->addr_vir[1];
	outputData.c3 = NULL;
	filter_param.orientation = frm_param->common_param.sensor_orientation;
	filter_param.flip_on = frm_param->common_param.flip_on;
	filter_param.is_front = frm_param->common_param.is_front;
	filter_param.filter_version = 2;//frm_param->filter_param.version;
	SWA_LOGD("orientation=%d, flip_on=%d, FilterType=%d, version=%d, is_front=%d",
			filter_param.orientation, filter_param.flip_on, frm_param->filter_param.filter_type,
			filter_param.filter_version, filter_param.is_front);

	ret = ImageFilterRun(cxt->filter_handle, &inputData, &outputData,
			(IFFilterType)frm_param->filter_param.filter_type, &filter_param, NULL);
	if (ret) {
		SWA_LOGE("failed to ImageFilterRun %d", ret);
	}

	ret = ImageFilterDestroyEngine(cxt->filter_handle);

	SWA_LOGD("Done");
	return ret;
}
#endif
