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


#define LOG_TAG "cmr_mfsr"


#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>
#include <pthread.h>
#include <cutils/list.h>
#include <cutils/trace.h>
#include <cutils/properties.h>

#include "cmr_msg.h"
#include "cmr_ipm.h"
#include "cmr_common.h"
#include "cmr_sensor.h"
#include "cmr_oem.h"
#include "sprd_camalg_adapter.h"
#include "mfsr_adapter.h"


struct mfsr_context_t {
	struct ipm_common common;
	struct ipm_context_t *ipm_cxt;
	struct img_frm src_frame[10];
	struct img_frm dst_frame;
	struct img_frm detail_frame;
	struct img_size frame_size;
	struct img_rect frame_crop;
	ipm_callback reg_cb;
	sem_t sem;
	uint32_t is_inited;
	uint32_t frame_num;
	uint32_t frame_cnt;
	uint32_t pic_w;
	uint32_t pic_h;
	mfsr_exif_t exif_info;
	void * mfsr_handle;
};

struct mfsr_post_context_t {
	struct ipm_common common;
	struct ipm_context_t *ipm_cxt;
	sem_t sem;
	uint32_t is_inited;
	void * sharp_handle;
};


static cmr_int mfsr_open(cmr_handle ipm_handle,
					struct ipm_open_in *in,
					struct ipm_open_out *out,
					cmr_handle *class_handle);
static cmr_int mfsr_close(cmr_handle class_handle);
static cmr_int mfsr_transfer_frame(cmr_handle class_handle,
					struct ipm_frame_in *in,
					struct ipm_frame_out *out);

static cmr_int mfsr_post_open(cmr_handle ipm_handle,
					struct ipm_open_in *in,
					struct ipm_open_out *out,
					cmr_handle *class_handle);
static cmr_int mfsr_post_close(cmr_handle class_handle);
static cmr_int mfsr_post_transfer_frame(cmr_handle class_handle,
					struct ipm_frame_in *in,
					struct ipm_frame_out *out);


static struct class_ops mfsr_ops = {
	mfsr_open, mfsr_close, mfsr_transfer_frame, NULL, NULL,
};

struct class_tab_t mfsr_tab_info = {
	&mfsr_ops,
};

static struct class_ops mfsr_post_ops = {
	mfsr_post_open, mfsr_post_close, mfsr_post_transfer_frame, NULL, NULL,
};

struct class_tab_t mfsr_post_tab_info = {
	&mfsr_post_ops,
};

static int dump_pic;
static int s_dbg_ver;


static cmr_int mfsr_open(cmr_handle ipm_handle,
					struct ipm_open_in *in,
					struct ipm_open_out *out,
					cmr_handle *class_handle)
{
	int ret = 0;
	struct mfsr_context_t *cxt = NULL;
	MFSR_CMD_OPEN_PARAM_T cmd_param;
	mfsr_open_param mfsr_param;
	MFSR_CMD_GET_VERSION_PARAM_T cmd_ver;
	mfsr_lib_version lib_version;

	char value[PROPERTY_VALUE_MAX];
	property_get("debug.camera.save.snpfile", value, "0");
	dump_pic = atoi(value);
	property_get("ro.debuggable", value, "0");
	s_dbg_ver = !!atoi(value);

	if (!in || !ipm_handle || !class_handle) {
		CMR_LOGE("Invalid Param!");
		return CMR_CAMERA_INVALID_PARAM;
	}
	*class_handle = (cmr_handle)NULL;

	cxt = (struct mfsr_context_t *)malloc(sizeof(struct mfsr_context_t));
	if (cxt == NULL) {
		CMR_LOGE("Fail to malloc cxt for mfsr\n");
		return CMR_CAMERA_NO_MEM;
	}
	memset(cxt, 0, sizeof(struct mfsr_context_t));
	cxt->ipm_cxt = (struct ipm_context_t *)ipm_handle;

	cmd_ver.lib_version = &lib_version;
	ret = sprd_mfsr_adapter_ctrl(MFSR_CMD_GET_VERSION, &cmd_ver);
	if (ret) {
		CMR_LOGE("fail to get mfsr version\n");
		return -1;
	}
	CMR_LOGD("mfsr version %d.%d.%d.%d, build date %s, time %s rev %s\n",
		lib_version.major, lib_version.minor, lib_version.micro, lib_version.nano,
		lib_version.built_date, lib_version.built_time, lib_version.built_rev);

	cxt->pic_w = in->frame_size.width;
	cxt->pic_h = in->frame_size.height;
	cxt->frame_num = (uint32_t)in->frame_cnt;
	cxt->reg_cb = in->reg_cb;

	memset(&mfsr_param, 0, sizeof(mfsr_open_param));
	mfsr_param.max_width = in->frame_size.width;
	mfsr_param.max_height = in->frame_size.height;
	mfsr_param.tuning_param = in->otp_data.otp_ptr;
	mfsr_param.tuning_param_size = (int) in->otp_data.otp_size;
	mfsr_param.sr_mode = 0;
	cmd_param.ctx = &cxt->mfsr_handle;
	cmd_param.param = &mfsr_param;

	ret = sprd_mfsr_adapter_ctrl(MFSR_CMD_OPEN, &cmd_param);
	if (ret || (*cmd_param.ctx) == NULL) {
		CMR_LOGE("fail to init mfsr\n");
		return -1;
	}
	CMR_LOGD("done. handle %p\n", cxt->mfsr_handle);


	cxt->common.ipm_cxt = (struct ipm_context_t *)ipm_handle;
	cxt->common.class_type = IPM_TYPE_MFSR;
	cxt->common.receive_frame_count = 0;
	cxt->common.save_frame_count = 0;
	cxt->common.ops = &mfsr_ops;

	sem_init(&cxt->sem, 0, 1);
	cxt->is_inited = 1;

	*class_handle = (cmr_handle)cxt;
	return 0;
}

static cmr_int mfsr_close(cmr_handle class_handle)
{
	int ret = 0;
	struct mfsr_context_t *cxt = (struct mfsr_context_t *)class_handle;
	MFSR_CMD_CLOSE_PARAM_T cmd_param;

	if (class_handle == NULL)
		return CMR_CAMERA_INVALID_PARAM;

	if (cxt->is_inited == 0)
		goto exit;

	sem_wait(&cxt->sem);

	CMR_LOGD("mfsr handle %p\n", cxt->mfsr_handle);
	if (cxt->mfsr_handle) {
		cmd_param.ctx = &cxt->mfsr_handle;
		ret = sprd_mfsr_adapter_ctrl(MFSR_CMD_CLOSE, &cmd_param);
		CMR_LOGD("Done. ret %d, handle %p\n", ret, cxt->mfsr_handle);
	}
	cxt->is_inited = 0;
	sem_post(&cxt->sem);
	sem_destroy(&cxt->sem);

exit:
	free(cxt);
	CMR_LOGD("done");
	return 0;
}

static cmr_int mfsr_transfer_frame(cmr_handle class_handle,
							struct ipm_frame_in *in,
							struct ipm_frame_out *out)
{
	int ret = 0;
	struct mfsr_context_t *cxt = NULL;
	struct swa_frame_param *frm_param;
	struct img_frm *frm_in, *frm_out, *frm_detail;
	MFSR_CMD_PROCESS_PARAM_T cmd_param;
	mfsr_process_param proc_param;
	sprd_camalg_image_t image_in;
	sprd_camalg_image_t image_out, image_detail;
	MFSR_CMD_GET_EXIF_PARAM_T cmd_exif;

	if (class_handle == NULL || in == NULL || out == NULL) {
		CMR_LOGE("fail to get input %p %p %p\n", class_handle, in, out);
		return CMR_CAMERA_FAIL;
	}

	cxt = (struct mfsr_context_t *)class_handle;
	if (cxt->is_inited == 0)
		return CMR_CAMERA_FAIL;

	sem_wait(&cxt->sem);

	frm_param = (struct swa_frame_param *)in->src_frame.reserved;

	cxt->src_frame[cxt->frame_cnt] = in->src_frame;
	if (cxt->frame_cnt == 0) {
		cxt->dst_frame = in->dst_frame;
		cxt->detail_frame = out->dst_frame;
	}

	frm_in = &cxt->src_frame[cxt->frame_cnt];
	frm_out = &cxt->dst_frame;
	frm_detail = &cxt->detail_frame;

	proc_param.gain = 1.0 * frm_param->common_param.again;
	proc_param.exposure_time = 1.0 * frm_param->common_param.exp_time;
	proc_param.crop_area.x = frm_param->mfsr_param.frame_crop.start_x;
	proc_param.crop_area.y = frm_param->mfsr_param.frame_crop.start_y;
	proc_param.crop_area.w = frm_param->mfsr_param.frame_crop.width;
	proc_param.crop_area.h = frm_param->mfsr_param.frame_crop.height;

	memset(&image_in, 0, sizeof(sprd_camalg_image_t));
	image_in.format = SPRD_CAMALG_IMG_NV21;
	image_in.ion_fd = frm_in->fd;
	image_in.addr[0] = (void *)frm_in->addr_vir.addr_y;
	image_in.addr[1] = (void *)frm_in->addr_vir.addr_u;
	image_in.width = frm_in->size.width;
	image_in.height = frm_in->size.height;
	image_in.stride = frm_in->size.width;
	image_in.size = image_in.width * image_in.height * 3 / 2;

	memcpy(&image_out, &image_in, sizeof(sprd_camalg_image_t));
	image_out.ion_fd = frm_out->fd;
	image_out.addr[0] = (void *)frm_out->addr_vir.addr_y;
	image_out.addr[1] = (void *)frm_out->addr_vir.addr_u;

	memcpy(&image_detail, &image_in, sizeof(sprd_camalg_image_t));
	image_detail.ion_fd = frm_detail->fd;
	image_detail.addr[0] = (void *)frm_detail->addr_vir.addr_y;
	image_detail.addr[1] = (void *)frm_detail->addr_vir.addr_u;

	cmd_param.ctx = &cxt->mfsr_handle;
	cmd_param.image_input = &image_in;
	cmd_param.image_output = &image_out;
	cmd_param.detail_mask = &image_detail;
	cmd_param.denoise_mask = NULL;
	cmd_param.process_param = &proc_param;

	CMR_LOGD("in fd 0x%x, addr %p, %p, out fd 0x%x, addr %p %p, detail fd 0x%x, addr %p %p\n",
		image_in.ion_fd, image_in.addr[0], image_in.addr[1],
		image_out.ion_fd, image_out.addr[0], image_out.addr[1],
		image_detail.ion_fd, image_detail.addr[0], image_detail.addr[1]);
	CMR_LOGD("param gain %f, exp time %f, size %d %d, crop %d %d %d %d\n",
		proc_param.gain, proc_param.exposure_time, image_in.width, image_in.height,
		proc_param.crop_area.x, proc_param.crop_area.y,
		proc_param.crop_area.w, proc_param.crop_area.h);

	if (dump_pic & 1) {
		FILE *fp;
		char fname[256];
		snprintf(fname, 256, "%sframe%03d_mfsr_in%d_%dx%d.yuv", CAMERA_DUMP_PATH,
			frm_in->frame_number, cxt->frame_cnt, frm_in->size.width, frm_in->size.height);
		fp = fopen(fname, "wb");
		if (fp) {
			CMR_LOGD("dump file %s\n", fname);
			fwrite((void *)frm_in->addr_vir.addr_y, 1, frm_in->size.width * frm_in->size.height * 3 / 2, fp);
			fclose(fp);
		}
	}

	ret = sprd_mfsr_adapter_ctrl(MFSR_CMD_PROCESS, &cmd_param);
	if (ret )
		CMR_LOGE("Fail to call mfsr process capture, ret =%d", ret);

	cxt->frame_cnt++;
	if (cxt->frame_cnt < cxt->frame_num)
		goto exit;

	if (dump_pic & 1) {
		FILE *fp;
		char fname[256];

		snprintf(fname, 256, "%sframe%03d_mfsr_out_%dx%d.yuv", CAMERA_DUMP_PATH,
			frm_in->frame_number, frm_in->size.width, frm_in->size.height);
		fp = fopen(fname, "wb");
		if (fp) {
			CMR_LOGD("dump file %s\n", fname);
			fwrite((void *)frm_out->addr_vir.addr_y, 1, frm_in->size.width * frm_in->size.height * 3 / 2, fp);
			fclose(fp);
		}

		snprintf(fname, 256, "%sframe%03d_mfsr_detail_%dx%d.y", CAMERA_DUMP_PATH,
			frm_in->frame_number, proc_param.crop_area.w, proc_param.crop_area.h);
		fp = fopen(fname, "wb");
		if (fp) {
			CMR_LOGD("dump file %s\n", fname);
			fwrite((void *)frm_in->addr_vir.addr_y, 1, proc_param.crop_area.w * proc_param.crop_area.h, fp);
			fclose(fp);
		}
	}

	cmd_exif.ctx = &cxt->mfsr_handle;
	cmd_exif.exif_info = &cxt->exif_info;

	ret = sprd_mfsr_adapter_ctrl(MFSR_CMD_GET_EXIF, &cmd_exif);
	if (ret )
		CMR_LOGE("Fail to call mfsr get exif, ret =%d",ret);

	CMR_LOGD("exif 0x%x, %d %d, (%d %d) (%d %d), gain %d ex time %d, ratio %d %d\n",
		cxt->exif_info.version,
		cxt->exif_info.merge_num, cxt->exif_info.sharpen_field,
		cxt->exif_info.im_input_w, cxt->exif_info.im_input_h,
		cxt->exif_info.im_output_w, cxt->exif_info.im_output_h,
		cxt->exif_info.gain, cxt->exif_info.exposure_time,
		cxt->exif_info.scale, cxt->exif_info.sharpen_level);


	if (s_dbg_ver && cxt->ipm_cxt->init_in.ipm_isp_ioctl) {
		struct isp_info dbg_info;
		struct common_isp_cmd_param isp_cmd;

		dbg_info.addr = &cxt->exif_info;
		dbg_info.size = sizeof(cxt->exif_info);
		dbg_info.frame_id = cxt->dst_frame.frame_number;
		isp_cmd.cmd_ptr = &dbg_info;
		ret = cxt->ipm_cxt->init_in.ipm_isp_ioctl(
					cxt->ipm_cxt->init_in.oem_handle,
					COM_ISP_SET_MFSR_LOG,
					&isp_cmd);
	}

exit:
	sem_post(&cxt->sem);

	CMR_LOGD("frame no.%d done\n", cxt->frame_cnt);
	if (ret)
		return CMR_CAMERA_FAIL;
	else
		return 0;
}

static cmr_int mfsr_post_open(cmr_handle ipm_handle,
					struct ipm_open_in *in,
					struct ipm_open_out *out,
					cmr_handle *class_handle)
{
	int ret = 0;
	struct mfsr_post_context_t *cxt = NULL;
	MFSR_CMD_OPEN_PARAM_T open_param;
	mfsr_open_param mfsr_param;

	if (!in || !ipm_handle || !class_handle) {
		CMR_LOGE("Invalid Param!");
		return CMR_CAMERA_INVALID_PARAM;
	}
	*class_handle = (cmr_handle)NULL;

	cxt = (struct mfsr_post_context_t *)malloc(sizeof(struct mfsr_post_context_t));
	if (cxt == NULL) {
		CMR_LOGE("Fail to malloc cxt for mfsr\n");
		return CMR_CAMERA_NO_MEM;
	}
	memset(cxt, 0, sizeof(struct mfsr_post_context_t));
	cxt->ipm_cxt = (struct ipm_context_t *)ipm_handle;

	memset(&mfsr_param, 0, sizeof(mfsr_open_param));
	mfsr_param.max_width = in->frame_size.width;
	mfsr_param.max_height = in->frame_size.height;
	mfsr_param.tuning_param = in->otp_data.otp_ptr;
	mfsr_param.tuning_param_size = in->otp_data.otp_size;
	mfsr_param.sr_mode = 0;
	open_param.ctx = &cxt->sharp_handle;
	open_param.param = &mfsr_param;

	ret = sprd_mfsr_adapter_ctrl(MFSR_CMD_POST_OPEN, &open_param);
	if (ret || (*open_param.ctx) == NULL) {
		CMR_LOGE("fail to init mfsr post\n");
		return CMR_CAMERA_FAIL;
	}
	CMR_LOGD("sharp_handle %p\n", cxt->sharp_handle);

	cxt->common.ipm_cxt = (struct ipm_context_t *)ipm_handle;
	cxt->common.class_type = IPM_TYPE_MFSR_POST;
	cxt->common.receive_frame_count = 0;
	cxt->common.save_frame_count = 0;
	cxt->common.ops = &mfsr_post_ops;

	sem_init(&cxt->sem, 0, 1);
	cxt->is_inited = 1;
	*class_handle = (cmr_handle)cxt;

	return 0;
}

static cmr_int mfsr_post_close(cmr_handle class_handle)
{
	int ret = 0;
	struct mfsr_post_context_t *cxt = NULL;
	MFSR_CMD_CLOSE_PARAM_T close_param;

	if (class_handle == NULL)
		return CMR_CAMERA_INVALID_PARAM;

	cxt = (struct mfsr_post_context_t *)class_handle;
	if (cxt->is_inited == 0)
		goto exit;

	sem_wait(&cxt->sem);

	CMR_LOGD("sharp handle %p\n", cxt->sharp_handle);
	if (cxt->sharp_handle) {
		close_param.ctx = &cxt->sharp_handle;
		ret = sprd_mfsr_adapter_ctrl(MFSR_CMD_POST_CLOSE, &close_param);
		CMR_LOGD("Close done. ret %d, handle %p\n", ret, cxt->sharp_handle);
	}

	cxt->is_inited = 0;
	sem_post(&cxt->sem);
	sem_destroy(&cxt->sem);

exit:
	free(cxt);
	CMR_LOGD("done");
	return 0;
}

static cmr_int mfsr_post_transfer_frame(cmr_handle class_handle,
							struct ipm_frame_in *in,
							struct ipm_frame_out *out)
{
	int ret = 0;
	struct mfsr_post_context_t *cxt = NULL;
	struct swa_frame_param *frm_param;
	struct img_frm *frm_in, *frm_detail;
	MFSR_CMD_POSTPROCESS_PARAM_T cmd_param;
	mfsr_process_param proc_param;
	sprd_camalg_image_t image_in;
	sprd_camalg_image_t image_detail;

	if (class_handle == NULL || in == NULL) {
		CMR_LOGE("fail to get input %p %p\n", class_handle, in);
		return -1;
	}

	cxt = (struct mfsr_post_context_t *)class_handle;
	if (cxt->is_inited == 0)
		return CMR_CAMERA_FAIL;

	sem_wait(&cxt->sem);

	frm_param = (struct swa_frame_param *)in->src_frame.reserved;

	frm_in = &in->src_frame;
	frm_detail = &in->dst_frame;

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
	image_in.addr[0] = (void *)frm_in->addr_vir.addr_y;
	image_in.addr[1] = (void *)frm_in->addr_vir.addr_u;
	image_in.width = frm_in->size.width;
	image_in.height = frm_in->size.height;
	image_in.stride = frm_in->size.width;
	image_in.size = image_in.width * image_in.height * 3 / 2;

	memcpy(&image_detail, &image_in, sizeof(sprd_camalg_image_t));
	image_detail.ion_fd = frm_detail->fd;
	image_detail.addr[0] = (void *)frm_detail->addr_vir.addr_y;
	image_detail.addr[1] = (void *)frm_detail->addr_vir.addr_u;

	cmd_param.ctx = &cxt->sharp_handle;
	cmd_param.image = &image_in;
	cmd_param.detail_mask = &image_detail;
	cmd_param.denoise_mask = NULL;
	cmd_param.process_param = &proc_param;

	CMR_LOGD("in fd 0x%x, addr %p, %p, detail fd 0x%x, addr %p %p\n",
		image_in.ion_fd, image_in.addr[0], image_in.addr[1],
		image_detail.ion_fd, image_detail.addr[0], image_detail.addr[1]);

	CMR_LOGD("param gain %f, exp time %f, size %d %d, crop %d %d %d %d\n",
		proc_param.gain, proc_param.exposure_time, image_in.width, image_in.height,
		proc_param.crop_area.x, proc_param.crop_area.y,
		proc_param.crop_area.w, proc_param.crop_area.h);

	if (dump_pic & 1) {
		FILE *fp;
		char fname[256];
		snprintf(fname, 256, "%sframe%03d_mfsr_post_in_%dx%d.yuv", CAMERA_DUMP_PATH,
			frm_in->frame_number, frm_in->size.width, frm_in->size.height);
		fp = fopen(fname, "wb");
		if (fp) {
			CMR_LOGD("dump file %s\n", fname);
			fwrite((void *)frm_in->addr_vir.addr_y, 1, frm_in->size.width * frm_in->size.height * 3 / 2, fp);
			fclose(fp);
		}
	}

	ret = sprd_mfsr_adapter_ctrl(MFSR_CMD_POST_PROCESS, &cmd_param);
	if (ret ) {
		CMR_LOGE("Fail to call mfsr post process, ret =%d", ret);
		return CMR_CAMERA_FAIL;
	}

	if (dump_pic & 1) {
		FILE *fp;
		char fname[256];
		snprintf(fname, 256, "%sframe%03d_mfsr_post_out_%dx%d.yuv", CAMERA_DUMP_PATH,
			frm_in->frame_number, frm_in->size.width, frm_in->size.height);
		fp = fopen(fname, "wb");
		if (fp) {
			CMR_LOGD("dump file %s\n", fname);
			fwrite((void *)frm_in->addr_vir.addr_y, 1, frm_in->size.width * frm_in->size.height * 3 / 2, fp);
			fclose(fp);
		}
	}

	sem_post(&cxt->sem);
	return 0;
}
