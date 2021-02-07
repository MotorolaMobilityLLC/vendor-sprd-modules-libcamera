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

#include "sprd_camalg_adapter.h"
#include "sprd_hdr_adapter.h"
#include "sprd_yuv_denoise_adapter.h"
#include "sprd_dre_adapter.h"
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


#ifdef CONFIG_CAMERA_HDR_CAPTURE

struct hdr_context_t {
	uint32_t frame_num;
	uint32_t pic_w;
	uint32_t pic_h;
	float ev[3];

	void * hdr_handle;  // for hdr algo api internal handle
	sprd_hdr_version_t version;
};

#ifdef CONFIG_SPRD_HDR_LIB_VERSION_2

int swa_hdr_get_handle_size()
{
	return sizeof(struct hdr_context_t);
}

int swa_hdr_open(void *ipmpro_hanlde,
			void * open_param, enum swa_log_level log_level)
{
	int ret = 0;
	struct hdr_context_t *cxt = (struct hdr_context_t *)ipmpro_hanlde;
	struct swa_init_data *init_param;

	s_swa_log_level = log_level;

	if ((ipmpro_hanlde == NULL) || (open_param == NULL)) {
		SWA_LOGE("fail to get hdr handle %p %p\n", ipmpro_hanlde, open_param);
		return -1;
	}

	init_param = (struct swa_init_data *)open_param;
	memset(cxt, 0, sizeof(struct hdr_context_t));

	cxt->frame_num = init_param->frm_total_num;
	cxt->pic_w = init_param->frame_size.width;
	cxt->pic_h = init_param->frame_size.height;
	SWA_LOGI("frame num %d, pic size %d %d\n",
		cxt->frame_num, cxt->pic_w, cxt->pic_h);

	cxt->hdr_handle = sprd_hdr_adpt_init(cxt->pic_w, cxt->pic_h, NULL);
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

	if (in->frame_num < 2 || in->frame_num > HDR_CAP_MAX) {
		SWA_LOGE("fail to get valid input hdr frame num %d\n", in->frame_num);
		return -1;
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

	SWA_LOGD("output fd 0x%x, addr %p %p %p\n", alg_param.output.ion_fd,
		alg_param.output.addr[0], alg_param.output.addr[1], alg_param.output.addr[2]);

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
	int ret = 0, offset, i;
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
	SWA_LOGD("frm No.%d, %p %p, fd 0x%x 0x%x\n", i, frm_in, frm_out, frm_in->fd, frm_out->fd);

	offset = frm_out->size.width * frm_out->size.height;
	denoise_param.bufferY.addr[0] = (void *)frm_in->addr_vir[0];
	denoise_param.bufferUV.addr[0] = (void *)(frm_in->addr_vir[0] + offset);
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



#ifdef CONFIG_CAMERA_DRE

struct dre_context_t {
	void * dre_handle;
};

int swa_dre_get_handle_size()
{
	return sizeof(struct dre_context_t);
}

int swa_dre_process(void * ipmpro_hanlde,
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

	ret = sprd_dre_adpt_init(&cxt->dre_handle,
				frm_in->size.width, frm_in->size.height, NULL);
	if (ret) {
		SWA_LOGE("fail to init dre ret %d\n", ret);
		return -1;
	}

	memset(&image_in, 0, sizeof(struct sprd_camalg_image));
	image_in.addr[0] = (void *)frm_in->addr_vir[0];
	image_in.addr[1] = (void *)frm_in->addr_vir[1];
	image_in.width = frm_in->size.width;
	image_in.height = frm_in->size.height;
	image_in.format = SPRD_CAMALG_IMG_NV21;
	ret = sprd_dre_adpt_ctrl(cxt->dre_handle, SPRD_DRE_PROCESS_CMD,
				&image_in, &frm_param->dre_param);
	if (ret) {
		SWA_LOGD("fail to do DRE	process ret %d\n", ret);
	}

	ret = sprd_dre_adpt_ctrl(cxt->dre_handle, SPRD_DRE_FAST_STOP_CMD, NULL, NULL);
	ret = sprd_dre_adpt_deinit(cxt->dre_handle);

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
	filter_param.orientation = frm_param->common_param.rotation;
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
