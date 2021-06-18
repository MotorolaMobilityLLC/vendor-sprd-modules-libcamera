/*
 * Copyright (C) 2021 The Android Open Source Project
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

#define LOG_TAG "ips_core"

#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>
#include <pthread.h>
#include <cutils/list.h>
#include <cutils/trace.h>
#include <cutils/properties.h>

#include "libloader.h"
#include "cmr_types.h"
#include "cmr_msg.h"
#include "cmr_log.h"
#include "cmr_cvt.h"
#include "cmr_jpeg.h"
#include "cmr_watermark.h"
#include "isp_debug.h"
#include "swa_ipm_api.h"
#include "swa_param.h"
#include "ips_interface.h"

enum {
	IPS_EVT_START,
	IPS_EVT_PROC,
	IPS_EVT_PROC_DONE,
	IPS_EVT_SYNC_WAIT,
};

enum ips_thd_status {
	IPS_THREAD_IDLE,
	IPS_THREAD_BUSY,
};

enum ips_req_status {
	IPS_REQ_NEW,
	IPS_REQ_INIT,
	IPS_REQ_PROC_START,
	IPS_REQ_PROC_DONE,
	IPS_REQ_ALL_DONE,
};

#define IPS_THREAD_NUM 32
#define IPS_MSG_QUEUE_SIZE 32
#define EXIF_RESERVED_SIZE (64 * 1024)

struct ipmpro_type {
	uint32_t enable;
	uint32_t multi;
	uint32_t out_is_in; /* output buffer is same as input buffer for single frame process */
	const char *keywd;
};

static struct ipmpro_type ipmproc_list[IPS_TYPE_MAX] = {
	[IPS_TYPE_UWARP] =       { 1, 0, 0, "uwarp", },
	[IPS_TYPE_HDR] =         { 1, 0, 0, "hdr", },
	[IPS_TYPE_MFNR] =        { 1, 0, 0, "mfnr", },
	[IPS_TYPE_MFSR] =        { 1, 0, 0, "mfsr", },
	[IPS_TYPE_CNR] =         { 1, 0, 1, "cnr", },
	[IPS_TYPE_SHARP] =       { 1, 0, 1, "sharp", },
	[IPS_TYPE_DREPRO] =      { 0, 0, 1, "drepro", },
	[IPS_TYPE_FILTER] =      { 1, 0, 1, "filter", },
	[IPS_TYPE_FB] =          { 1, 0, 1, "fb", },
	[IPS_TYPE_WATERMARK] =   { 1, 1, 1, "wm", },
	[IPS_TYPE_JPEG] =        { 1, 0, 0, "jpeg", },
};

struct ipmpro_handle_base {
	cmr_u32 inited;
	cmr_u32 version;
	cmr_s32 get_flag;
	cmr_s32 swa_handle_size;
	cmr_u32 multi_support;
	cmr_u32 need_outbuf;

	cmr_handle lib_handle;
	pthread_mutex_t glock;

	int (*swa_open)(void *ipmpro_hanlde,
			void *open_param,
			uint32_t log_level);
	int (*swa_process)(void * ipmpro_hanlde,
			void *frms_in,
			void *frms_out,
			void * data);
	int (*swa_close)(void *ipmpro_hanlde,
			void * close_param);
};

struct ipmpro_node {
	struct listnode list;
	cmr_u32 type;
	cmr_u32 log_level;
	cmr_u32 is_async;
	cmr_handle swa_handle;
	cmr_handle param_ptr;
	struct ipmpro_handle_base *ipm_base;
};

struct ips_debug_info {
	void *data;
	cmr_u32 data_size;
};

struct ips_req_node {
	struct listnode list;
	struct ips_request_t req_in;
	ips_req_cb cb;
	cmr_handle client_data;
	struct ips_thread_context *thrd;

	struct swa_init_data init_param;
	cmr_u32 frame_cnt;
	cmr_u32 frame_total;
	struct img_frm frm_in[10];
	struct img_frm frm_out[10];
	struct img_frm frm_middle[10];
	struct img_frm frm_jpeg;
	struct img_frm frm_thumb_jpeg;

	struct ips_debug_info dbg_info[IPS_TYPE_MAX];

	uint32_t should_exit; /* force pre-exit if camera closed */
	enum ips_req_status status;
	struct ipmpro_node *cur_proc;
	struct listnode ipm_head;
};

struct ips_thread_context {
	cmr_u32 idx;
	cmr_u32 init;
	enum ips_thd_status  status;
	pthread_mutex_t lock;
	cmr_handle thr_handle;
	cmr_handle ips_handle;
	cmr_handle req_handle;
};

struct ips_context {
	cmr_handle client_data;

	pthread_mutex_t listlock;
	struct listnode req_head;
	cmr_u32 req_count;

	struct ips_handle_t other_handles;

	pthread_mutex_t thrd_lock;
	cmr_u32 thread_cnt;
	struct ips_thread_context ips_threads[IPS_THREAD_NUM];

	struct ips_req_node *req_in_jpeg;

	/* for sw process base hanlde */
	struct ipmpro_handle_base ipmpro_base[IPS_TYPE_MAX];
};

static int dump_pic;
static int s_dbg_ver;

static const char *MFSR_START = "ISP_MFSR__";
static const char *MFSR_END = "ISP_MFSR__";
static const char *HDR_START = "ISP_HDR_";
static const char *HDR_END = "ISP_HDR_";

#define COPY_MAGIC(m) \
{cmr_u32 len; len = strlen(m); memcpy(dst + off, m, len); off += len;}

static void copy_dbg_info(char *dst, const void *log, cmr_u32 size,
		const char *begin_magic, const char *end_magic, cmr_u32 *offset)
{
	cmr_u32 off = 0;

	CMR_LOGD("%p %p  magic %s, %s, off %p, size %d\n",
		dst, log, begin_magic, end_magic, offset, size);
	if (!log || !size)
		return;

	COPY_MAGIC(begin_magic);
	memcpy(dst + off, log, size);
	off += size;
	COPY_MAGIC(end_magic);
	*offset = off;
}

static cmr_int init_ipmpro_base(struct ipmpro_type *in, struct ipmpro_handle_base *cur)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;
	void *sw_handle = NULL;
	ipmpro_get_handle_size swa_get_size;
	char sym_get[128], sym_open[128], sym_close[128], sym_process[128];

	CMR_LOGD("E. %s enable %d\n", in->keywd, in->enable);
	if (in->enable == 0)
		goto exit;

	sw_handle = NULL;
	sw_handle = get_lib_handle("libcam_ipmpro.so");
	if (sw_handle)
		cur->get_flag = 1;
	else
		sw_handle = dlopen("libcam_ipmpro.so", RTLD_NOW);

	if (sw_handle == NULL) {
		CMR_LOGE("fail to open libcam_ipmpro.so\n");
		goto exit;
	}

	snprintf(sym_get, 128, "swa_%s_get_handle_size", in->keywd);
	snprintf(sym_open, 128, "swa_%s_open", in->keywd);
	snprintf(sym_close, 128, "swa_%s_close", in->keywd);
	snprintf(sym_process, 128, "swa_%s_process", in->keywd);
	CMR_LOGD("symbols %s %s %s %s\n", sym_get, sym_open, sym_close, sym_process);

	swa_get_size = dlsym(sw_handle, sym_get);
	if (swa_get_size)
		cur->swa_handle_size = swa_get_size();
	if (swa_get_size == NULL || cur->swa_handle_size <= 0) {
		CMR_LOGW("warning: %s get swa_get_handle_size %p, %d",
			in->keywd, swa_get_size, cur->swa_handle_size);
	}

	cur->swa_open = dlsym(sw_handle, sym_open);
	cur->swa_process = dlsym(sw_handle, sym_process);
	cur->swa_close = dlsym(sw_handle, sym_close);
	if (!cur->swa_open || !cur->swa_process || !cur->swa_close) {
		CMR_LOGW("warning: %s get NULL api %p %p %p\n",
			in->keywd, cur->swa_open, cur->swa_process, cur->swa_close);
	}

	cur->lib_handle = sw_handle;
	cur->multi_support = in->multi;
	cur->need_outbuf = in->out_is_in ? 0 : 1;
	cur->version = 0;
	cur->inited = 1;
	pthread_mutex_init(&cur->glock, NULL);

	CMR_LOGD("Done. ipm %s, lib_handle %p, multi %d, api %p %p %p, data size %d\n",
		in->keywd, sw_handle, cur->multi_support,
		cur->swa_open, cur->swa_process, cur->swa_close, cur->swa_handle_size);
	return 0;

exit:
	cur->inited = 0;
	cur->swa_open = NULL;
	cur->swa_process = NULL;
	cur->swa_close = NULL;
	CMR_LOGD("X, %s\n", in->keywd);
	return ret;
}

static cmr_int deinit_ipmpro_base(struct ipmpro_handle_base *cur)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;

	if (cur->inited == 0)
		goto exit;

	pthread_mutex_destroy(&cur->glock);
	if (cur->lib_handle) {
		if (cur->get_flag)
			put_lib_handle(cur->lib_handle);
		else
			dlclose(cur->lib_handle);
	}
	memset(cur, 0, sizeof(struct ipmpro_handle_base));

exit:
	CMR_LOGD("Done\n");
	return ret;
}

static cmr_int init_jpeg_base(struct ips_context *ips_ctx)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;
	void *sw_handle = NULL;
	struct ipmpro_handle_base *cur = &ips_ctx->ipmpro_base[IPS_TYPE_JPEG];

	cur->inited = 1;
	cur->version = 0;
	cur->swa_handle_size = sizeof(cmr_handle);
	cur->multi_support = 0;
	cur->lib_handle = NULL;
	pthread_mutex_init(&cur->glock, NULL);

	CMR_LOGD("Done. lib_handle %p, multi %d, api %p %p %p, data size %d\n",
		sw_handle, cur->multi_support,
		cur->swa_open, cur->swa_process, cur->swa_close, cur->swa_handle_size);

	return ret;
}

static cmr_int deinit_jpeg_base(struct ips_context *ips_ctx)
{
    	cmr_int ret = CMR_CAMERA_SUCCESS;
	struct ipmpro_handle_base *cur = &ips_ctx->ipmpro_base[IPS_TYPE_JPEG];

	pthread_mutex_destroy(&cur->glock);
	memset(cur, 0, sizeof(struct ipmpro_handle_base));

	CMR_LOGD("Done\n");
	return ret;
}

static void swa_jpeg_dbginfo(struct ips_req_node *req, struct ips_jpeg_param_t *pm)
{
	cmr_u32 i, total_size, offset, end_tag;
	char *pstart = (char *)(req->frm_jpeg.addr_vir.addr_y + req->frm_jpeg.buf_size);
	struct sprd_isp_debug_info hdr;
	struct _isp_log_info log_info;

	char *ptemp = (char *)(req->frm_jpeg.addr_vir.addr_y);

	CMR_LOGD("start addr 0x%lx, data %02x, %02x, size %d, info ptr %p\n",
		req->frm_jpeg.addr_vir.addr_y,
		ptemp[0], ptemp[1], req->frm_jpeg.buf_size, pstart);
	CMR_LOGD("dbg info %p %d\n", pm->isp_debug_info, pm->isp_debug_info_size);

	memcpy(pstart, pm->isp_debug_info, pm->isp_debug_info_size);

	/* re-pack debug info for post-processing */
	CMR_LOGD("sprd_isp_debug_info size %d", sizeof(struct sprd_isp_debug_info));
	memcpy(&hdr, pstart, sizeof(struct sprd_isp_debug_info));
	memcpy(&log_info, pstart + sizeof(struct sprd_isp_debug_info), sizeof(struct sprd_isp_debug_info));
	total_size = hdr.debug_len - 4;
	pstart += total_size;
	CMR_LOGD("new ptr %p, debug info size %d\n", pstart, hdr.debug_len);

	for (i = 0; i < IPS_TYPE_MAX; i++) {
		if (req->dbg_info[i].data == NULL || req->dbg_info[i].data_size == 0)
			continue;
		if (i != IPS_TYPE_MFSR && i != IPS_TYPE_HDR)
			continue;
		offset = 0;
		CMR_LOGD("cpy start, data  %p, size %d\n",
			req->dbg_info[i].data, req->dbg_info[i].data_size);
		if (i == IPS_TYPE_MFSR) {
			copy_dbg_info((void *)pstart, req->dbg_info[i].data, req->dbg_info[i].data_size,
					MFSR_START, MFSR_END, &offset);
			if (offset) {
				log_info.mfsr_off = total_size;
				total_size += offset;
				log_info.mfsr_len = offset;
			} else {
				log_info.mfsr_off = 0;
			}
			pstart += offset;
			CMR_LOGD("isp_type %d size %d, off %d, ptr %p\n", i, total_size, offset, pstart);
		}
		if (i == IPS_TYPE_HDR) {
			copy_dbg_info((void *)pstart, req->dbg_info[i].data, req->dbg_info[i].data_size,
					HDR_START, HDR_END, &offset);
			if (offset) {
				log_info.hdr_off = total_size;
				total_size += offset;
				log_info.hdr_len = offset;
			} else {
				log_info.hdr_off = 0;
			}
			pstart += offset;
			CMR_LOGD("isp_type %d size %d, off %d, ptr %p\n", i, total_size, offset, pstart);
		}
	}
	end_tag = SPRD_DEBUG_END_FLAG;
	memcpy(pstart, &end_tag, sizeof(cmr_u32));

	total_size += sizeof(cmr_u32);
	hdr.debug_len = total_size;

	pstart = (char *)(req->frm_jpeg.addr_vir.addr_y + req->frm_jpeg.buf_size);
	memcpy(pstart, &hdr, sizeof(struct sprd_isp_debug_info));
	pstart += sizeof(struct sprd_isp_debug_info);
	memcpy(pstart, &log_info, sizeof(struct _isp_log_info));

	req->frm_jpeg.buf_size += total_size;
	CMR_LOGD("debug info size %d, jpeg size %d\n", total_size, req->frm_jpeg.buf_size);
}

static cmr_int swa_exif_process(struct ips_context *ips_ctx,
	struct ips_req_node *req, struct ipmpro_node *cur_proc)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;
	struct img_frm src, src1, dst;
	struct ips_jpeg_param_t *pm = (struct ips_jpeg_param_t *)cur_proc->param_ptr;
	struct jpeg_enc_exif_param enc_exif_param;
	struct jpeg_wexif_cb_param out_param;

	CMR_LOGD("req_id %d exif start\n", req->req_in.request_id);

	src = req->frm_jpeg;
	src1 = req->frm_thumb_jpeg;
	dst = req->frm_jpeg;
	src.addr_vir.addr_y += EXIF_RESERVED_SIZE;

	if (dump_pic & 2) {
		FILE *fp;
		char fname[256];
		int size;
		snprintf(fname, 256, "%sframe%03d_ccc00.jpeg", CAMERA_DUMP_PATH, src.frame_number);
		fp = fopen(fname, "wb");
		size = src.buf_size;
		if (fp) {
			CMR_LOGD("jpeg raw fd 0x%x, addr %p,  size %d (%d %d) , write to file %s\n", src.fd,
				(void *)src.addr_vir.addr_y, size, src.size.width, src.size.height, fname);
			fwrite((void *)src.addr_vir.addr_y, 1, size, fp);
			fclose(fp);
		} else {
			CMR_LOGE("fail to open file %s\n", fname);
		}
	}

	memset(&enc_exif_param, 0, sizeof(struct jpeg_enc_exif_param));
	memset(&out_param, 0, sizeof(struct jpeg_wexif_cb_param));
	enc_exif_param.jpeg_handle = ips_ctx->other_handles.jpeg_handle;
	enc_exif_param.src_jpeg_addr_virt = src.addr_vir.addr_y;
	enc_exif_param.thumbnail_addr_virt = src1.addr_vir.addr_y;
	enc_exif_param.target_addr_virt = dst.addr_vir.addr_y;
	enc_exif_param.src_jpeg_size = src.buf_size;
	enc_exif_param.thumbnail_size = src1.buf_size;
	enc_exif_param.target_size = dst.buf_size + EXIF_RESERVED_SIZE;
	enc_exif_param.exif_ptr = &pm->exif_data.exif_info;
	ret = cmr_jpeg_enc_add_eixf(ips_ctx->other_handles.jpeg_handle,
			&enc_exif_param, &out_param);

	req->frm_jpeg.addr_vir.addr_y = out_param.output_buf_virt_addr;
	req->frm_jpeg.buf_size = out_param.output_buf_size;

	if (s_dbg_ver && (pm->isp_debug_info != NULL))
		swa_jpeg_dbginfo(req, pm);

	src = req->frm_jpeg;
	if (dump_pic & 2) {
		FILE *fp;
		char fname[256];
		int size;
		snprintf(fname, 256, "%sframe%03d_ccc11.jpeg", CAMERA_DUMP_PATH, src.frame_number);
		fp = fopen(fname, "wb");
		size = src.buf_size;
		if (fp) {
			CMR_LOGD("jpeg final fd 0x%x, addr %p,  size %d (%d %d) , write to file %s\n", src.fd,
				(void *)src.addr_vir.addr_y, size, src.size.width, src.size.height, fname);
			fwrite((void *)src.addr_vir.addr_y, 1, size, fp);
			fclose(fp);
		} else {
			CMR_LOGE("fail to open file %s\n", fname);
		}
	}
	CMR_LOGD("exif done\n");
	return ret;
}

static cmr_int swa_thumbnail_procss(struct ips_context *ips_ctx,
	struct ips_req_node *req, struct ipmpro_node *cur_proc)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;
	cmr_u32 offset;
	int retry_cnt = 100;
	struct img_frm src, dst, buf;
	struct cmr_op_mean mean;
	struct jpeg_enc_cb_param enc_cb_param;
	struct ips_jpeg_param_t *pm = (struct ips_jpeg_param_t *)cur_proc->param_ptr;

	src = req->frm_out[0];

	CMR_LOGI("thumb yuv src fd 0x%x, vaddr (%p %p %p), p (0x%lx 0x%lx 0x%lx), size %d %d, rect %d %d %d %d\n",
		src.fd, (void *)src.addr_vir.addr_y, (void *)src.addr_vir.addr_u, (void *)src.addr_vir.addr_v,
		src.addr_phy.addr_y, src.addr_phy.addr_u, src.addr_phy.addr_v,
		src.size.width, src.size.height,
		src.rect.start_x, src.rect.start_y, src.rect.width, src.rect.height);
retry:
	memset(&buf, 0, sizeof(struct img_frm));
	buf.buf_size = pm->thumb_size.width * pm->thumb_size.height * 3 / 2;
	ret = req->cb(req->client_data, &req->req_in, IPS_CB_GET_BUF, &buf);
	if (ret || buf.fd <= 0) {
		if ((retry_cnt-- > 0) && !req->should_exit) {
			usleep(20 * 1000);
			goto retry;
		}
		CMR_LOGE("fail to get thumbnail buffer\n");
		return ret;
	}
	dst = src;
	dst.fd = buf.fd;
	dst.buf_size = buf.buf_size;
	dst.size.width = pm->thumb_size.width;
	dst.size.height = pm->thumb_size.height;
	dst.rect.start_x = 0;
	dst.rect.start_y = 0;
	dst.rect.width = dst.size.width;
	dst.rect.height = dst.size.height;
	offset = dst.size.width * dst.size.height;
	dst.addr_vir.addr_y = buf.addr_vir.addr_y;
	dst.addr_vir.addr_u = dst.addr_vir.addr_y + offset;
	dst.addr_phy.addr_y = 0;
	dst.addr_phy.addr_u = offset;

	ret = cmr_scale_start(ips_ctx->other_handles.scale_handle,
			&src, &dst, (cmr_evt_cb)NULL, NULL);
	if (ret) {
		CMR_LOGE("fail to scale thumbnail");
		return ret;
	}

	src = dst;
	CMR_LOGI("thumb yuv dst fd 0x%x, vaddr (%p %p %p), p (0x%lx 0x%lx 0x%lx), size %d %d\n",
		src.fd, (void *)src.addr_vir.addr_y, (void *)src.addr_vir.addr_u, (void *)src.addr_vir.addr_v,
		src.addr_phy.addr_y, src.addr_phy.addr_u, src.addr_phy.addr_v,
		src.size.width, src.size.height);

	if (dump_pic & 4) {
		FILE *fp;
		char fname[256];
		int size;

		snprintf(fname, 256, "%sframe%03d_bbthumb2_scale.yuv", CAMERA_DUMP_PATH, src.frame_number);
		fp = fopen(fname, "wb");
		size = src.size.width * src.size.height * 3 / 2;
		if (fp) {
			CMR_LOGD("thumb yuv fd 0x%x, addr %p,  size %d (%d %d) , write to file %s\n", src.fd,
				(void *)src.addr_vir.addr_y, size, src.size.width, src.size.height, fname);
			fwrite((void *)src.addr_vir.addr_y, 1, size, fp);
			fclose(fp);
		} else {
			CMR_LOGE("fail to open file %s\n", fname);
		}
	}

	dst = src;
	dst.fd = 0;
	ret = req->cb(req->client_data, &req->req_in, IPS_CB_GET_BUF, &dst);
	if (ret || dst.fd <= 0) {
		if ((retry_cnt-- > 0) && !req->should_exit) {
			usleep(20 * 1000);
			goto retry;
		}
		CMR_LOGE("fail to get thumbnail jpeg buffer\n");
		req->cb(req->client_data, &req->req_in, IPS_CB_RETURN_BUF, (void *)&src);
		return ret;
	}
	dst.fmt = CAM_IMG_FMT_JPEG;
	dst.addr_vir.addr_u = dst.addr_vir.addr_y + offset;
	dst.addr_phy.addr_u = offset;

	memcpy(&mean, &pm->mean, sizeof(struct cmr_op_mean));
	mean.is_sync = 1;
	mean.is_thumb = 1;
	mean.slice_height = dst.size.height;
	if (mean.rot) {
		cmr_u32 temp;
		temp = dst.size.width;
		dst.size.width = dst.size.height;
		dst.size.height = temp;
		mean.slice_height = dst.size.height;
	}

	if (1) {
		struct img_frm temp;
		temp = src;
		src = dst;
		CMR_LOGI("thumb jpeg fd 0x%x, vaddr (%p %p %p), p (0x%lx 0x%lx 0x%lx), size %d %d\n",
			src.fd, (void *)src.addr_vir.addr_y, (void *)src.addr_vir.addr_u, (void *)src.addr_vir.addr_v,
			src.addr_phy.addr_y, src.addr_phy.addr_u, src.addr_phy.addr_v,
			src.size.width, src.size.height);
		src = temp;
	}

	memset(&enc_cb_param, 0, sizeof(struct jpeg_enc_cb_param));
	ret = cmr_jpeg_encode(cur_proc->swa_handle,
			&src, &dst, &mean, &enc_cb_param);
	if (ret) {
		CMR_LOGE("fail to jpeg encode\n");
	}
	req->frm_thumb_jpeg = dst;
	req->frm_thumb_jpeg.buf_size = enc_cb_param.stream_size;

	CMR_LOGD("dst addr %p,  cb addr %p, size %d\n", (void *)dst.addr_vir.addr_y,
		(void *)enc_cb_param.stream_buf_vir, enc_cb_param.stream_size);

	/* return thumbnail yuv buffer */
	req->cb(req->client_data, &req->req_in, IPS_CB_RETURN_BUF, (void *)&src);

	src = dst;
	if (src.addr_vir.addr_y && (dump_pic & 2)) {
		FILE *fp;
		char fname[256];
		int size;
		snprintf(fname, 256, "%sframe%03d_dddthumb.jpeg", CAMERA_DUMP_PATH, src.frame_number);
		fp = fopen(fname, "wb");
		size = src.size.width * src.size.height * 3 / 2;
		if (fp) {
			CMR_LOGD("jpeg fd 0x%x, addr %p,  size %d (%d %d) , write to file %s\n", src.fd,
				(void *)src.addr_vir.addr_y, size, src.size.width, src.size.height, fname);
			fwrite((void *)src.addr_vir.addr_y, 1, size, fp);
			fclose(fp);
		} else {
			CMR_LOGE("fail to open file %s\n", fname);
		}
	}

	if (enc_cb_param.stream_buf_vir && (dump_pic & 2)) {
		FILE *fp;
		char fname[256];
		int size;
		snprintf(fname, 256, "%sframe%03d_eeethumb.jpeg", CAMERA_DUMP_PATH, src.frame_number);
		fp = fopen(fname, "wb");
		size = enc_cb_param.stream_size;
		if (fp) {
			CMR_LOGD("jpeg fd 0x%x, addr %p,  size %d (%d %d) , write to file %s\n", src.fd,
				(void *)enc_cb_param.stream_buf_vir, size, src.size.width, src.size.height, fname);
			fwrite((void *)enc_cb_param.stream_buf_vir, 1, size, fp);
			fclose(fp);
		} else {
			CMR_LOGE("fail to open file %s\n", fname);
		}
	}

	CMR_LOGD("X");
	return ret;
}

static cmr_int swa_jpeg_process(struct ips_context *ips_ctx,
	struct ips_req_node *req, struct ipmpro_node *cur_proc)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;
	cmr_u32 offset;
	int retry_cnt = 150;
	struct img_frm *src, *dst, buf;
	struct ips_jpeg_param_t *pm = (struct ips_jpeg_param_t *)cur_proc->param_ptr;

	CMR_LOGD("req_id %d jpeg start\n", req->req_in.request_id);

	src = &req->frm_out[0];
	dst = &req->frm_jpeg;

	if (dst->fd == 0) {
retry:
		memset(&buf, 0, sizeof(struct img_frm));
		buf.buf_size = src->size.width * src->size.height * 3 / 2;
		ret = req->cb(req->client_data, &req->req_in, IPS_CB_GET_BUF, &buf);
		if (ret || buf.fd <= 0) {
			if ((retry_cnt-- > 0) && !req->should_exit) {
				usleep(20 * 1000);
				goto retry;
			}
			CMR_LOGE("fail to get jpeg buffer\n");
			return ret;
		}
		offset = src->size.width * src->size.height;
		dst->fd = buf.fd;
		dst->addr_vir.addr_y = buf.addr_vir.addr_y;
		dst->addr_vir.addr_u = buf.addr_vir.addr_y + offset;
		dst->addr_phy.addr_y = 0;
		dst->addr_phy.addr_u = offset;
	}
	dst->rect.start_x = 0;
	dst->rect.start_y = 0;
	dst->rect.width = src->size.width;
	dst->rect.height = src->size.height;
	dst->size = src->size;
	dst->sec = src->sec;
	dst->usec = src->usec;
	dst->fmt = CAM_IMG_FMT_JPEG;
	dst->monoboottime = src->monoboottime;
	dst->frame_number = src->frame_number;
	if (pm->mean.rot) {
		cmr_u32 temp;
		temp = dst->size.width;
		dst->size.width = dst->size.height;
		dst->size.height = temp;
	}
	dst->addr_vir.addr_y += EXIF_RESERVED_SIZE;
	dst->addr_phy.addr_y = EXIF_RESERVED_SIZE;

	CMR_LOGD("jpeg size %d %d, fd 0x%x, fid %d, vaddr 0x%x, 0x%x, paddr 0x%x, 0x%x\n",
		dst->size.width, dst->size.height, dst->fd, dst->frame_number,
		dst->addr_vir.addr_y, dst->addr_vir.addr_u,
		dst->addr_phy.addr_y, dst->addr_phy.addr_u);

	if (cur_proc->ipm_base->multi_support == 0)
		pthread_mutex_lock(&cur_proc->ipm_base->glock);

	if (req->should_exit) {
		CMR_LOGD("req_id %d should exit\n", req->req_in.request_id);
		if (cur_proc->ipm_base->multi_support == 0)
			pthread_mutex_unlock(&cur_proc->ipm_base->glock);
		return ret;
	}

	ips_ctx->req_in_jpeg = req;

	CMR_LOGD("src fd 0x%x, %p, dst fd 0x%x, %p\n",
		src->fd, (void *)src->addr_vir.addr_y, dst->fd, (void *)dst->addr_vir.addr_y);
	CMR_LOGD("mirror=%d, flip=%d, rot=%d, is_sync=%d, quality %d\n",
		pm->mean.mirror, pm->mean.flip, pm->mean.rot,
		pm->mean.is_sync, pm->mean.quality_level);

	CMR_LOGD("handle %p %p %p\n", cur_proc->swa_handle,
		ips_ctx->other_handles.jpeg_handle, ips_ctx->other_handles.scale_handle);

	ret = cmr_jpeg_encode(cur_proc->swa_handle,
			src, dst, &pm->mean, NULL);
	if (ret) {
		CMR_LOGE("fail to jpeg encode\n");
	}

	dst->addr_vir.addr_y -= EXIF_RESERVED_SIZE;
	dst->addr_phy.addr_y = 0;

	ret = swa_thumbnail_procss(ips_ctx, req, cur_proc);
	if (ret) {
		CMR_LOGE("fail to swa_thumbnail_procss\n");
	}
	return ret;
}


static int check_skip(struct ipmpro_node *cur_proc,
	struct swa_frame_param *frm_param)
{
	switch (cur_proc->type) {

	case IPS_TYPE_FB:
		CMR_LOGD("fb bypass %d, face_num %d\n", frm_param->fb_info.bypass,
			frm_param->face_param.face_num);
		if (frm_param->fb_info.bypass || frm_param->face_param.face_num == 0)
			return 1;
		else
			return 0;

	case IPS_TYPE_CNR:
		CMR_LOGD("cnr2_bypass %d cnr3 %d %d, ynnrs %d %d",
			frm_param->cnr2_info.bypass,
			frm_param->cnr3_info.bypass, frm_param->cnr3_info.baseRadius,
			frm_param->ynrs_info.bypass, frm_param->ynrs_info.radius_base);
		if (frm_param->ynrs_info.bypass &&
			frm_param->cnr2_info.bypass &&
			frm_param->cnr3_info.bypass)
			return 1;
		else
			return 0;

	default:
		return 0;
	}
	return 0;
}

static cmr_int ipmpro_common(struct ips_context *ips_ctx,
	struct ips_req_node *req, struct img_frm *frame)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;
	int iret = 0, i, err = 0, retry_cnt = 100;
	struct img_frm *dst, *src, buf;
	struct ipmpro_node *cur_proc = req->cur_proc;
	struct ipmpro_handle_base *ipm_base = cur_proc->ipm_base;
	struct swa_frame_param *frm_param;
	struct swa_hdr_param *hdr_param;
	struct swa_frames_inout in, out;

	frm_param = (struct swa_frame_param *)frame->reserved;

	req->frm_in[req->frame_cnt] = *frame;
	src = &req->frm_in[req->frame_cnt];
	dst = &req->frm_out[req->frame_cnt];
	*dst = *src;
	CMR_LOGD("req_id %d, param %p, input frame %d,  fd 0x%x, addr 0x%lx, ghand %p\n",
		req->req_in.request_id, frm_param, req->frame_cnt,
		dst->fd, dst->addr_vir.addr_y, dst->gpu_handle);

	if (check_skip(cur_proc, frm_param)) {
		CMR_LOGD("proc type %d, skip\n", cur_proc->type);
		goto exit;
	}

	if (ipm_base->multi_support == 0)
		pthread_mutex_lock(&ipm_base->glock);

	if (ipm_base->swa_open) {
		iret = ipm_base->swa_open(cur_proc->swa_handle, (void *)&req->init_param, 4);
		if (iret) {
			CMR_LOGE("fail to open for proc %d\n", cur_proc->type);
			if (ipm_base->multi_support == 0)
				pthread_mutex_unlock(&ipm_base->glock);
			return CMR_CAMERA_FAIL;
		}
	}

	if (ipm_base->swa_process == NULL) {
		CMR_LOGD("type %d process is NULL\n", cur_proc->type);
		if (cur_proc->type ==  IPS_TYPE_WATERMARK) {
			sizeParam_t sizeparam;
			if (frm_param == NULL) {
				CMR_LOGE("fail to get param for watermarker\n");
				goto proc_done;
			}
			memset(&sizeparam, 0, sizeof(sizeParam_t));
			sizeparam.imgW = src->size.width;
			sizeparam.imgH = src->size.height;
			sizeparam.angle = frm_param->common_param.angle;
			sizeparam.isMirror = frm_param->common_param.flip_on;
			CMR_LOGD("add watermark, type %d, time %s, size %d %d, angle %d, mirror %d\n",
				frm_param->wm_param.flag, frm_param->wm_param.time_text,
				sizeparam.imgW, sizeparam.imgH, sizeparam.angle, sizeparam.isMirror);

			watermark_add_yuv(frm_param->wm_param.flag,
					(cmr_u8 *)dst->addr_vir.addr_y, frm_param->wm_param.time_text,
					&sizeparam);
		}
		goto proc_done;
	}

	if (cur_proc->ipm_base->need_outbuf) {
		uint32_t offset;
		offset = src->size.width * src->size.height;
		buf.buf_size = src->size.width * src->size.height * 3 / 2;
retry:
		buf.fd = 0;
		ret = req->cb(req->client_data, &req->req_in, IPS_CB_GET_BUF, &buf);
		if (ret || buf.fd <= 0) {
			if ((retry_cnt-- > 0) && !req->should_exit) {
				usleep(20 * 1000);
				goto retry;
			}
			CMR_LOGD("fail to get free buffer\n");
			goto unlock;
		}
		dst = &req->frm_out[req->frame_cnt];
		dst->fd = buf.fd;
		dst->gpu_handle = buf.gpu_handle;
		dst->addr_vir.addr_y = buf.addr_vir.addr_y;
		dst->addr_vir.addr_u = buf.addr_vir.addr_y + offset;
		dst->addr_phy.addr_y = 0;
		dst->addr_phy.addr_u = offset;
	}

	memset(&in, 0, sizeof(struct swa_frames_inout));
	memset(&out, 0, sizeof(struct swa_frames_inout));

	src = &req->frm_in[req->frame_cnt];
	in.frame_num = 1;
	in.frms[0].fd = src->fd;
	in.frms[0].gpu_handle = src->gpu_handle;
	in.frms[0].addr_vir[0] = src->addr_vir.addr_y;
	in.frms[0].addr_vir[1] = src->addr_vir.addr_u;
	in.frms[0].addr_vir[2] = src->addr_vir.addr_v;
	in.frms[0].size.width = src->size.width;
	in.frms[0].size.height = src->size.height;
	CMR_LOGD("in fd=0x%x, ghand %p, vaddr 0x%lx size %d %d\n", src->fd,
		src->gpu_handle, src->addr_vir.addr_y, src->size.width, src->size.height);

	if (cur_proc->type == IPS_TYPE_SHARP) {
		src = &req->frm_middle[0];
		in.frame_num = 2;
		in.frms[1].fd = src->fd;
		in.frms[1].gpu_handle = src->gpu_handle;
		in.frms[1].addr_vir[0] = src->addr_vir.addr_y;
		in.frms[1].addr_vir[1] = src->addr_vir.addr_u;
		in.frms[1].addr_vir[2] = src->addr_vir.addr_v;
		CMR_LOGD("detail fd=0x%x, ghand %p, vaddr 0x%lx size %d %d\n", src->fd,
		src->gpu_handle, src->addr_vir.addr_y, src->size.width, src->size.height);
	}

	src = &req->frm_out[req->frame_cnt];
	out.frame_num = 1;
	out.frms[0].fd = src->fd;
	out.frms[0].gpu_handle = src->gpu_handle;
	out.frms[0].addr_vir[0] = src->addr_vir.addr_y;
	out.frms[0].addr_vir[1] = src->addr_vir.addr_u;
	out.frms[0].addr_vir[2] = src->addr_vir.addr_v;
	out.frms[0].size.width = src->size.width;
	out.frms[0].size.height = src->size.height;
	CMR_LOGD("out fd=0x%x, ghand %p, vaddr 0x%lx size %d %d\n", src->fd,
		src->gpu_handle, src->addr_vir.addr_y, src->size.width, src->size.height);


	iret = ipm_base->swa_process(cur_proc->swa_handle, &in, &out, frame->reserved);

proc_done:
	if (iret) {
		CMR_LOGE("fail to process type %d\n", cur_proc->type);
		err |= 1;
	}

	if (ipm_base->swa_close) {
		iret = ipm_base->swa_close(cur_proc->swa_handle, NULL);
		if (iret) {
			CMR_LOGE("fail to close for proc %d\n", cur_proc->type);
			err |= 1;
		}
	}

	if (cur_proc->ipm_base->need_outbuf)
		req->cb(req->client_data, &req->req_in, IPS_CB_RETURN_BUF, (void *)&req->frm_in[req->frame_cnt]);

	if (cur_proc->type == IPS_TYPE_SHARP) {
		req->cb(req->client_data, &req->req_in, IPS_CB_RETURN_BUF, (void *)&req->frm_middle[0]);
		memset(&req->frm_middle[0], 0, sizeof(req->frm_middle[0]));
	}

	if (!err  &&  (dump_pic & 1)) {
		FILE *fp;
		char fname[256];
		int size;
		struct img_frm src = req->frm_out[req->frame_cnt];
		snprintf(fname, 256, "%sframe%03d_type%d.yuv", CAMERA_DUMP_PATH, src.frame_number, cur_proc->type);
		fp = fopen(fname, "wb");
		size = src.size.width * src.size.height * 3 / 2;
		if (fp) {
			CMR_LOGD("req_id %d proc %d, fd 0x%x, addr %p,  size %d (%d %d) , write to file %s\n",
				req->req_in.request_id, cur_proc->type, src.fd,
				(void *)src.addr_vir.addr_y, size, src.size.width, src.size.height, fname);
			fwrite((void *)src.addr_vir.addr_y, 1, size, fp);
			fclose(fp);
		} else {
			CMR_LOGE("fail to open file %s\n", fname);
		}
	}

unlock:
	if (ipm_base->multi_support == 0)
		pthread_mutex_unlock(&ipm_base->glock);

exit:
	req->frame_cnt++;
	if (req->frame_cnt >= req->frame_total) {
		CMR_LOGD("req id %d type %d process all frame %d done\n",
			req->req_in.request_id,  cur_proc->type, req->frame_total);
		req->frame_cnt = 0;
		req->status = IPS_REQ_PROC_DONE;
	}

	return ret;
}

static cmr_int ipmpro_hdr(struct ips_context *ips_ctx,
	struct ips_req_node *req, struct img_frm *frame)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;
	int iret, i;
	struct img_frm *dst, *src;
	struct ipmpro_node *ipm_hdl = req->cur_proc;
	struct ipmpro_handle_base *hdr2_base = ipm_hdl->ipm_base;
	struct swa_frame_param *frm_param;
	struct swa_hdr_param *hdr_param, temp;
	struct swa_frames_inout in, out;

	dst = &req->frm_in[req->frame_cnt];
	*dst = *frame;

	CMR_LOGD("req_id %d, input frame %d, fd 0x%x, addr 0x%lx\n",
		req->req_in.request_id, req->frame_cnt, dst->fd, dst->addr_vir.addr_y);

	req->frame_cnt++;
	if (req->frame_cnt < req->frame_total)
		return ret;

	if (hdr2_base->multi_support == 0)
		pthread_mutex_lock(&hdr2_base->glock);

	frm_param = (struct swa_frame_param *)frame->reserved;
	if (frm_param) {
		hdr_param = &frm_param->hdr_param;
	} else {
		hdr_param = &temp;
		hdr_param->ev[0] = 0 - 1.0f;
		hdr_param->ev[1] = 0.0f;
		hdr_param->ev[2] = 1.0f;
	}

	frm_param->hdr_param.pic_w = frame->size.width;
	frm_param->hdr_param.pic_h = frame->size.height;
	frm_param->hdr_param.fmt = req->frame_cnt;
	frm_param->hdr_param.heap_mem_malloc = req->init_param.heap_mem_malloc;
	frm_param->hdr_param.heap_mem_free = req->init_param.heap_mem_free;
	CMR_LOGD("hdr open w %d, h %d, param %p, ev %f %f %f\n",
		hdr_param->pic_w, hdr_param->pic_h, frm_param,
		hdr_param->ev[0],  hdr_param->ev[1], hdr_param->ev[2]);

	iret = hdr2_base->swa_open(ipm_hdl->swa_handle, (void *)&frm_param->hdr_param, 4);
	if (iret) {
		CMR_LOGE("fail to open hdr2\n");
		return CMR_CAMERA_FAIL;
	}

	memset(&in, 0, sizeof(struct swa_frames_inout));
	memset(&out, 0, sizeof(struct swa_frames_inout));
	in.frame_num = req->frame_cnt;
	for (i = 0; i < in.frame_num; i++) {
		struct swa_frame *dst = &in.frms[i];
		src = &req->frm_in[i];
		dst->fd = src->fd;
		dst->addr_vir[0] = src->addr_vir.addr_y;
		dst->addr_vir[1] = src->addr_vir.addr_u;
		dst->addr_vir[2] = src->addr_vir.addr_v;
	}
	req->frm_out[0] = req->frm_in[req->frame_cnt - 1];
	src = &req->frm_out[0];
	out.frame_num = 1;
	out.frms[0].fd = src->fd;
	out.frms[0].addr_vir[0] = src->addr_vir.addr_y;
	out.frms[0].addr_vir[1] = src->addr_vir.addr_u;
	out.frms[0].addr_vir[2] = src->addr_vir.addr_v;

	iret = hdr2_base->swa_process(ipm_hdl->swa_handle, &in, &out, (void *)frm_param);
	if (iret) {
		CMR_LOGE("fail to process hdr2\n");
		return CMR_CAMERA_FAIL;
	}

	iret = hdr2_base->swa_close(ipm_hdl->swa_handle, NULL);
	if (iret) {
		CMR_LOGE("fail to close hdr2\n");
		return CMR_CAMERA_FAIL;
	}

	if (hdr2_base->multi_support == 0)
		pthread_mutex_unlock(&hdr2_base->glock);

	for (i = 0; i < req->frame_cnt - 1; i++) {
		free(req->frm_in[i].reserved);
		req->cb(req->client_data, &req->req_in, IPS_CB_RETURN_BUF, (void *)&req->frm_in[i]);
		memset(&req->frm_in[i], 0, sizeof(struct img_frm));
	}

	if (dump_pic & 1) {
		FILE *fp;
		char fname[256];
		int size;
		struct img_frm src = req->frm_out[0];
		snprintf(fname, 256, "%sframe%03d_type%d.yuv", CAMERA_DUMP_PATH, src.frame_number, ipm_hdl->type);
		fp = fopen(fname, "wb");
		size = src.size.width * src.size.height * 3 / 2;
		if (fp) {
			CMR_LOGD("req_id %d proc %d, fd 0x%x, addr %p,  size %d (%d %d) , write to file %s\n",
				req->req_in.request_id, ipm_hdl->type, src.fd,
				(void *)src.addr_vir.addr_y, size, src.size.width, src.size.height, fname);
			fwrite((void *)src.addr_vir.addr_y, 1, size, fp);
			fclose(fp);
		} else {
			CMR_LOGE("fail to open file %s\n", fname);
		}
	}

	req->status = IPS_REQ_PROC_DONE;
	req->frame_total = 1;
	CMR_LOGD("Done");

	if (s_dbg_ver && frm_param->hdr_param.out_exif_ptr && frm_param->hdr_param.out_exif_size) {
		req->dbg_info[IPS_TYPE_HDR].data = malloc(frm_param->hdr_param.out_exif_size);
		if (req->dbg_info[IPS_TYPE_HDR].data) {
			memcpy(req->dbg_info[IPS_TYPE_HDR].data,
					frm_param->hdr_param.out_exif_ptr,
					frm_param->hdr_param.out_exif_size);
			req->dbg_info[IPS_TYPE_HDR].data_size = frm_param->hdr_param.out_exif_size;
		}
	}

	return ret;
}

static cmr_int ipmpro_mfnr(struct ips_context *ips_ctx,
	struct ips_req_node *req, struct img_frm *frame)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;
	int iret, i, is_clear = 0;
	int s_w, s_h, offset, retry_cnt = 100;
	struct img_frm *dst, *small;
	struct ipmpro_node *ipm_hdl = req->cur_proc;
	struct ipmpro_handle_base *ipm_base = ipm_hdl->ipm_base;
	struct swa_frame_param *frm_param;
	struct swa_frames_inout in;
	struct swa_init_data init_param;

	if (frame == NULL) {
		CMR_LOGD("mfnr should quick stop\n");
		is_clear = 1;
		if (req->frame_cnt == 0)
			return ret;
		else
			goto clear;
	}

	dst = &req->frm_out[req->frame_cnt];
	*dst = *frame;
	frm_param = (struct swa_frame_param *)frame->reserved;

	CMR_LOGD("req_id %d, fno.%d, fd 0x%x, vaddr 0x%lx, gpu %p, fpm %p (%d %d) (%d %d)\n",
		req->req_in.request_id, req->frame_cnt, dst->fd, dst->addr_vir.addr_y, dst->gpu_handle,
		frm_param, dst->size.width, dst->size.height, dst->rect.width, dst->rect.height);

	if (req->frame_cnt > 0)
		goto proc_mfnr;

	if (ipm_base->multi_support == 0)
		pthread_mutex_lock(&ipm_base->glock);

	init_param.frame_size = frame->size;
	init_param.frame_crop = frame->rect;
	init_param.frm_total_num = req->frame_total;
	init_param.ae_again = frm_param->common_param.again;
	init_param.pri_data = (void *)&frm_param->mfnr_param;
	init_param.heap_mem_malloc = req->init_param.heap_mem_malloc;
	init_param.heap_mem_free = req->init_param.heap_mem_free;
	init_param.af_ctrl_roi.start_x = frm_param->af_ctrl_roi.start_x;
	init_param.af_ctrl_roi.start_y = frm_param->af_ctrl_roi.start_y;
	init_param.af_ctrl_roi.width = frm_param->af_ctrl_roi.width;
	init_param.af_ctrl_roi.height = frm_param->af_ctrl_roi.height;

	iret = ipm_base->swa_open(ipm_hdl->swa_handle, (void *)&init_param, 4);
	if (iret) {
		CMR_LOGE("fail to open mfnr\n");
		goto open_fail;
	}

	s_w = frame->rect.width;
	s_h = frame->rect.height;
	offset = s_w * s_h;
	for (i = 0; i < (int)req->frame_total; i++) {
		struct img_frm buf;
retry:
		memset(&buf, 0, sizeof(struct img_frm));
		buf.buf_size = s_w * s_h * 3 / 2;
		ret = req->cb(req->client_data, &req->req_in, IPS_CB_GET_BUF, &buf);
		if (ret || buf.fd <= 0) {
			if ((retry_cnt-- > 0) && !req->should_exit) {
				usleep(20 * 1000);
				goto retry;
			}
			CMR_LOGE("fail to get mfnr small buffer\n");
			goto buf_error;
		}
		buf.size.width = s_w;
		buf.size.height = s_h;
		buf.rect.width = s_w;
		buf.rect.height = s_h;
		buf.addr_vir.addr_u = buf.addr_vir.addr_y + (cmr_uint)offset;
		buf.addr_phy.addr_u = (cmr_uint)offset;
		buf.fmt = dst->fmt;
		buf.data_end = dst->data_end;
		req->frm_middle[i] = buf;
		CMR_LOGD("mfnr sbuf No.%d fd=0x%x, addr = 0x%lx 0x%lx, size %d %d",
			i, buf.fd, buf.addr_vir.addr_y, buf.addr_vir.addr_u, s_w, s_h);
	}

proc_mfnr:
	small = &req->frm_middle[req->frame_cnt];
	dst->rect.start_x = 0;
	dst->rect.start_y = 0;
	dst->rect.width = dst->size.width;
	dst->rect.height = dst->size.height;
	ret = cmr_scale_start(ips_ctx->other_handles.scale_handle,
			dst, small, (cmr_evt_cb)NULL, NULL);
	if (ret) {
		CMR_LOGE("fail to scale thumbnail");
		return ret;
	}

	memset(&in, 0, sizeof(struct swa_frames_inout));
	in.frame_num = 2;
	in.frms[0].fd = dst->fd;
	in.frms[0].gpu_handle = dst->gpu_handle;
	in.frms[0].addr_vir[0] = dst->addr_vir.addr_y;
	in.frms[0].addr_vir[1] = dst->addr_vir.addr_u;
	in.frms[1].fd = small->fd;
	in.frms[1].addr_vir[0] = small->addr_vir.addr_y;
	in.frms[1].addr_vir[1] = small->addr_vir.addr_u;
	iret = ipm_base->swa_process(ipm_hdl->swa_handle, &in, NULL, (void *)frm_param);
	if (iret)
		CMR_LOGE("fail to process mfnr\n");

	if (dump_pic & 1) {
		FILE *fp;
		char fname[256], fname1[256], fname2[256];
		int size;
		struct img_frm src = req->frm_out[req->frame_cnt];

		snprintf(fname, 256, "%sframe%03d_type%d_%d_in.yuv", CAMERA_DUMP_PATH,
			src.frame_number, ipm_hdl->type, req->frame_cnt);
		snprintf(fname1, 256, "%sframe%03d_type%d_%d_small.yuv", CAMERA_DUMP_PATH,
			src.frame_number, ipm_hdl->type, req->frame_cnt);
		snprintf(fname2, 256, "%sframe%03d_type%d_%d_out.yuv", CAMERA_DUMP_PATH,
			src.frame_number, ipm_hdl->type, req->frame_cnt);

		fp = fopen(fname, "wb");
		size = src.size.width * src.size.height * 3 / 2;
		if (fp) {
			CMR_LOGD("req_id %d proc %d, fd 0x%x, addr %p,  size %d (%d %d) , write to file %s\n",
				req->req_in.request_id, ipm_hdl->type, src.fd,
				(void *)src.addr_vir.addr_y, size, src.size.width, src.size.height, fname);
			fwrite((void *)src.addr_vir.addr_y, 1, size, fp);
			fclose(fp);
		} else {
			CMR_LOGE("fail to open file %s\n", fname);
		}

		src = req->frm_middle[req->frame_cnt];
		fp = fopen(fname1, "wb");
		size = src.size.width * src.size.height * 3 / 2;
		if (fp) {
			CMR_LOGD("req_id %d proc %d, fd 0x%x, addr %p,	size %d (%d %d) , write to file %s\n",
				req->req_in.request_id, ipm_hdl->type, src.fd,
				(void *)src.addr_vir.addr_y, size, src.size.width, src.size.height, fname1);
			fwrite((void *)src.addr_vir.addr_y, 1, size, fp);
			fclose(fp);
		} else {
			CMR_LOGE("fail to open file %s\n", fname1);
		}

		src = req->frm_out[0];
		fp = fopen(fname2, "wb");
		size = src.size.width * src.size.height * 3 / 2;
		if (fp) {
			CMR_LOGD("req_id %d proc %d, fd 0x%x, addr %p,	size %d (%d %d) , write to file %s\n",
				req->req_in.request_id, ipm_hdl->type, src.fd,
				(void *)src.addr_vir.addr_y, size, src.size.width, src.size.height, fname2);
			fwrite((void *)src.addr_vir.addr_y, 1, size, fp);
			fclose(fp);
		} else {
			CMR_LOGE("fail to open file %s\n", fname2);
		}
	}

	req->frame_cnt++;
	if (req->frame_cnt < req->frame_total)
		goto proc_done;

clear:
	iret = ipm_base->swa_close(ipm_hdl->swa_handle, NULL);
	if (iret)
		CMR_LOGE("fail to close mfnr\n");

	for (i = 0; i < req->frame_total; i++)
		req->cb(req->client_data, &req->req_in, IPS_CB_RETURN_BUF, (void *)&req->frm_middle[i]);
	memset(&req->frm_middle[0], 0, sizeof(req->frm_middle));

	if (ipm_base->multi_support == 0)
		pthread_mutex_unlock(&ipm_base->glock);

	if (is_clear)
		return 0;

	for (i = 1; i < req->frame_total; i++) {
		free(req->frm_out[i].reserved);
		req->cb(req->client_data, &req->req_in, IPS_CB_RETURN_BUF, (void *)&req->frm_out[i]);
	}
	memset(&req->frm_in[0], 0, sizeof(req->frm_in));
	memset(&req->frm_out[1], 0, sizeof(req->frm_out) - sizeof(req->frm_out[0]));
	req->status = IPS_REQ_PROC_DONE;
	req->frame_total = 1;

proc_done:
	CMR_LOGD("Done");
	return 0;

buf_error:
	for (i = 0; i < req->frame_total; i++) {
		if (req->frm_middle[i].fd <= 0)
			continue;
		req->cb(req->client_data, &req->req_in, IPS_CB_RETURN_BUF, (void *)&req->frm_middle[i]);
	}

	iret = ipm_base->swa_close(ipm_hdl->swa_handle, NULL);
	if (iret)
		CMR_LOGE("fail to close mfnr\n");

open_fail:
	if (ipm_base->multi_support == 0)
		pthread_mutex_unlock(&ipm_base->glock);
	return CMR_CAMERA_FAIL;
}

static cmr_int ipmpro_jpeg(struct ips_context *ips_ctx,
	struct ips_req_node *req, struct img_frm *frame)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;
	cmr_u32 offset;
	struct img_frm *dst;
	struct ipmpro_node *first_proc;

	first_proc = node_to_item(req->ipm_head.next, struct ipmpro_node, list);
	if (req->cur_proc != first_proc)
		req->cb(req->client_data, &req->req_in, IPS_CB_FLUSH_BUFCACHE, (void *)frame);

	CMR_LOGD("jpegsize %d %d, frame size %d %d\n", req->frm_jpeg.size.width,
		req->frm_jpeg.size.height, frame->size.width, frame->size.height);

	dst = &req->frm_out[0];
	*dst = *frame;

	CMR_LOGD("buf fd 0x%x,  param ptr %p\n", req->frm_out[0].fd,
		req->frm_out[0].reserved);

	CMR_LOGD("frame_id %d, fmt 0x%x, time %03d.%06d, 0x%llx\n",
		frame->frame_number, frame->fmt, frame->sec, frame->usec, frame->monoboottime);

	CMR_LOGD("fd 0x%x, size %d %d, vaddr 0x%x 0x%x 0x%x, paddr  0x%x 0x%x 0x%x\n",
		frame->fd, frame->size.width, frame->size.height,
		frame->addr_vir.addr_y, frame->addr_vir.addr_u, frame->addr_vir.addr_v,
		frame->addr_phy.addr_y, frame->addr_phy.addr_u, frame->addr_phy.addr_v);
#if 0
	offset = dst->size.width * dst->size.height;
	if (dst->addr_phy.addr_u == 0)
		dst->addr_phy.addr_u = dst->addr_phy.addr_y + offset;
	if (dst->addr_vir.addr_u == 0)
		dst->addr_vir.addr_u = dst->addr_vir.addr_y + offset;
#endif

	swa_jpeg_process(ips_ctx, req, req->cur_proc);

	return ret;
}

static cmr_int ipmpro_jpeg_done(struct ips_context *ips_ctx,
	struct ips_req_node *req, struct jpeg_enc_cb_param *data)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;
	struct ipmpro_node *ipm_hdl = req->cur_proc;
	struct ipmpro_handle_base *jpeg_base = ipm_hdl->ipm_base;

	ips_ctx->req_in_jpeg = NULL;
	if (jpeg_base->multi_support == 0)
		pthread_mutex_unlock(&jpeg_base->glock);

	if (req->should_exit) {
		CMR_LOGE("req id %d jpeg done should exit\n", req->req_in.request_id);
		ret = cmr_stop_codec(req->cur_proc->swa_handle);
		if (ret)
			CMR_LOGE("fail to stop jpeg codec\n");
		goto exit;
	}

	req->frm_jpeg.buf_size = data->stream_size;

	CMR_LOGD("req_id %d jpeg done, addr %p %p, size %d,  %d %d %d\n",
		req->req_in.request_id, (void *)data->stream_buf_phy, (void *)data->stream_buf_vir,
		data->stream_size, data->slice_height, data->total_height, data->is_thumbnail);

	swa_exif_process(ips_ctx, req, req->cur_proc);

exit:
	CMR_LOGD("buf fd 0x%x,  param ptr %p\n", req->frm_out[0].fd,
		req->frm_out[0].reserved);
	free(req->frm_out[0].reserved);

	CMR_LOGD("return out buffer exit %d\n", req->should_exit);
	req->cb(req->client_data, &req->req_in, IPS_CB_RETURN_BUF, (void *)&req->frm_out[0]);
	memset(&req->frm_out[0], 0, sizeof(struct img_frm));

	req->cb(req->client_data, &req->req_in, IPS_CB_RETURN_BUF, (void *)&req->frm_thumb_jpeg);
	memset(&req->frm_thumb_jpeg, 0, sizeof(struct img_frm));

	if (req->should_exit)
		req->cb(req->client_data, &req->req_in, IPS_CB_RETURN_BUF, &req->frm_jpeg);
	else
		req->cb(req->client_data, &req->req_in, IPS_CB_JPEG_DONE, &req->frm_jpeg);
	memset(&req->frm_jpeg, 0, sizeof(struct img_frm));

	CMR_LOGD("Done\n");
	return ret;
}

static cmr_int ipmpro_mfsr_mfsr(struct ips_context *ips_ctx,
	struct ips_req_node *req, struct img_frm *frame)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;
	int iret, i, is_clear = 0;
	int offset, retry_cnt = 200;
	struct img_frm *src, *dst[2], buf[2];
	struct ipmpro_node *ipm_hdl = req->cur_proc;
	struct ipmpro_handle_base *ipm_base = ipm_hdl->ipm_base;
	struct swa_frame_param *frm_param;
	struct swa_frames_inout in, out;
	struct swa_init_data init_param;

	if (frame == NULL) {
		CMR_LOGD("mfsr should quick stop\n");
		is_clear = 1;
		if (req->frame_cnt == 0)
			return 0;
		else
			goto clear;
	}

	req->frm_in[req->frame_cnt] = *frame;
	dst[0] = &req->frm_out[0];
	dst[1] = &req->frm_middle[0];
	src = &req->frm_in[req->frame_cnt];
	frm_param = (struct swa_frame_param *)frame->reserved;

	CMR_LOGD("req_id %d, fno.%d, fd 0x%x, vaddr 0x%lx, gpu %p, fpm %p (%d %d) (%d %d)\n",
		req->req_in.request_id, req->frame_cnt, src->fd, src->addr_vir.addr_y, src->gpu_handle,
		frm_param, src->size.width, src->size.height, src->rect.width, src->rect.height);

	if (req->frame_cnt > 0)
		goto proc_mfsr;

	buf[0].fd = 0;
	buf[1].fd = 0;
	for (i = 0; i < 2; i++) {
retry:
		memcpy(dst[i], src, sizeof(struct img_frm));
		offset = src->size.width * src->size.height;
		buf[i].buf_size = src->size.width * src->size.height * 3 / 2;
		ret = req->cb(req->client_data, &req->req_in, IPS_CB_GET_BUF, &buf[i]);
		if (ret || buf[i].fd <= 0) {
			if ((retry_cnt-- > 0) && !req->should_exit) {
				usleep(20 * 1000);
				goto retry;
			}
			CMR_LOGD("fail to get free buffer\n");
			if (i == 1)
				req->cb(req->client_data, &req->req_in, IPS_CB_RETURN_BUF, (void *)&buf[0]);
			return CMR_CAMERA_FAIL;
		}
		dst[i]->fd = buf[i].fd;
		dst[i]->gpu_handle = buf[i].gpu_handle;
		dst[i]->addr_vir.addr_y = buf[i].addr_vir.addr_y;
		dst[i]->addr_vir.addr_u = buf[i].addr_vir.addr_y + offset;
		dst[i]->addr_phy.addr_y = 0;
		dst[i]->addr_phy.addr_u = offset;
	}
	dst[1]->reserved = NULL;

	if (ipm_base->multi_support == 0)
		pthread_mutex_lock(&ipm_base->glock);

	init_param.frame_size = frame->size;
	init_param.frm_total_num = req->frame_total;
	init_param.pri_data = frm_param->mfsr_param.data;
	init_param.pri_data_size = frm_param->mfsr_param.data_size;

	iret = ipm_base->swa_open(ipm_hdl->swa_handle, (void *)&init_param, 4);
	if (iret) {
		CMR_LOGE("fail to open mfsr\n");
		goto open_fail;
	}

proc_mfsr:
	memset(&in, 0, sizeof(struct swa_frames_inout));
	in.frame_num = 1;
	in.frms[0].fd = src->fd;
	in.frms[0].gpu_handle = src->gpu_handle;
	in.frms[0].addr_vir[0] = src->addr_vir.addr_y;
	in.frms[0].addr_vir[1] = src->addr_vir.addr_u;
	in.frms[0].rect.start_x = src->rect.start_x;
	in.frms[0].rect.start_y = src->rect.start_y;
	in.frms[0].rect.width = src->rect.width;
	in.frms[0].rect.height = src->rect.height;
	in.frms[0].size.width = src->size.width;
	in.frms[0].size.height = src->size.height;

	memset(&out, 0, sizeof(struct swa_frames_inout));
	out.frame_num = 2;
	out.frms[0].fd = dst[0]->fd;
	out.frms[0].gpu_handle = dst[0]->gpu_handle;
	out.frms[0].addr_vir[0] = dst[0]->addr_vir.addr_y;
	out.frms[0].addr_vir[1] = dst[0]->addr_vir.addr_u;

	out.frms[1].fd = dst[1]->fd;
	out.frms[1].gpu_handle = dst[1]->gpu_handle;
	out.frms[1].addr_vir[0] = dst[1]->addr_vir.addr_y;
	out.frms[1].addr_vir[1] = dst[1]->addr_vir.addr_u;

	iret = ipm_base->swa_process(ipm_hdl->swa_handle, &in, &out, (void *)frm_param);
	if (iret)
		CMR_LOGE("fail to process mfsr\n");

	if (dump_pic & 1) {
		FILE *fp;
		char fname[256], fname2[256], fname3[256];
		int size;
		struct img_frm src = req->frm_in[req->frame_cnt];

		snprintf(fname, 256, "%sframe%03d_type%d_%d_in.yuv", CAMERA_DUMP_PATH,
			src.frame_number, ipm_hdl->type, req->frame_cnt);
		snprintf(fname2, 256, "%sframe%03d_type%d_%d_out.yuv", CAMERA_DUMP_PATH,
			src.frame_number, ipm_hdl->type, req->frame_cnt);

		fp = fopen(fname, "wb");
		size = src.size.width * src.size.height * 3 / 2;
		if (fp) {
			CMR_LOGD("req_id %d proc %d, fd 0x%x, addr %p, size %d (%d %d) , write to file %s\n",
				req->req_in.request_id, ipm_hdl->type, src.fd,
				(void *)src.addr_vir.addr_y, size, src.size.width, src.size.height, fname);
			fwrite((void *)src.addr_vir.addr_y, 1, size, fp);
			fclose(fp);
		} else {
			CMR_LOGE("fail to open file %s\n", fname);
		}

		src = req->frm_out[0];
		fp = fopen(fname2, "wb");
		size = src.size.width * src.size.height * 3 / 2;
		if (fp) {
			CMR_LOGD("req_id %d proc %d, fd 0x%x, addr %p, size %d (%d %d) , write to file %s\n",
				req->req_in.request_id, ipm_hdl->type, src.fd,
				(void *)src.addr_vir.addr_y, size, src.size.width, src.size.height, fname2);
			fwrite((void *)src.addr_vir.addr_y, 1, size, fp);
			fclose(fp);
		} else {
			CMR_LOGE("fail to open file %s\n", fname2);
		}

		src = req->frm_middle[0];
		src.size.width = frm_param->mfsr_param.frame_crop.width;
		src.size.height = frm_param->mfsr_param.frame_crop.height;
		snprintf(fname3, 256, "%sframe%03d_type%d_%d_detail_%dx%d.y", CAMERA_DUMP_PATH,
			frame->frame_number, ipm_hdl->type, req->frame_cnt, src.size.width, src.size.height);
		fp = fopen(fname3, "wb");
		size = src.size.width * src.size.height;
		if (fp) {
			CMR_LOGD("req_id %d proc %d, fd 0x%x, addr %p, size %d (%d %d) , write to file %s\n",
				req->req_in.request_id, ipm_hdl->type, src.fd,
				(void *)src.addr_vir.addr_y, size, src.size.width, src.size.height, fname2);
			fwrite((void *)src.addr_vir.addr_y, 1, size, fp);
			fclose(fp);
		} else {
			CMR_LOGE("fail to open file %s\n", fname3);
		}

	}

	req->frame_cnt++;
	if (req->frame_cnt < req->frame_total)
		goto proc_done;

	if (s_dbg_ver && frm_param->mfsr_param.out_exif_ptr && frm_param->mfsr_param.out_exif_size) {
		req->dbg_info[IPS_TYPE_MFSR].data = malloc(frm_param->mfsr_param.out_exif_size);
		if (req->dbg_info[IPS_TYPE_MFSR].data) {
			memcpy(req->dbg_info[IPS_TYPE_MFSR].data,
					frm_param->mfsr_param.out_exif_ptr,
					frm_param->mfsr_param.out_exif_size);
			req->dbg_info[IPS_TYPE_MFSR].data_size = frm_param->mfsr_param.out_exif_size;
		}
	}

clear:
	iret = ipm_base->swa_close(ipm_hdl->swa_handle, NULL);
	if (iret)
		CMR_LOGE("fail to close mfsr\n");

	for (i = 0; i < req->frame_total; i++) {
		if (i > 0) /* input frame 0 param passed to output frame */
			free(req->frm_in[i].reserved);
		req->cb(req->client_data, &req->req_in, IPS_CB_RETURN_BUF, (void *)&req->frm_in[i]);
	}
	memset(&req->frm_in[0], 0, sizeof(req->frm_in));

	if (ipm_base->multi_support == 0)
		pthread_mutex_unlock(&ipm_base->glock);

	if (is_clear)
		return 0;

	req->status = IPS_REQ_PROC_DONE;
	req->frame_total = 1;

proc_done:
	CMR_LOGD("Done");
	return 0;

open_fail:
	for (i = 0; i < 2; i++)
		req->cb(req->client_data, &req->req_in, IPS_CB_RETURN_BUF, (void *)&buf[i]);

	if (ipm_base->multi_support == 0)
		pthread_mutex_unlock(&ipm_base->glock);
	return CMR_CAMERA_FAIL;
}

cmr_int ips_thread_proc(struct cmr_msg *message, void *p_data)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;
	int i;
	struct ips_thread_context *ips_thread = (struct ips_thread_context *)p_data;
	struct ips_context *ips_ctx = (struct ips_context *)ips_thread->ips_handle;
	struct ips_req_node *req = (struct ips_req_node *)ips_thread->req_handle;
	struct ipmpro_node *cur_proc = req->cur_proc;
	struct ipmpro_node *next_proc;
	struct img_frm *frame;

	CMR_LOGD("ips thread %d req_id %d, msg 0x%x, cur_proc %d\n", ips_thread->idx,
			req->req_in.request_id, message->msg_type, cur_proc->type);

	switch (message->msg_type) {
	case IPS_EVT_PROC:
		frame = (struct img_frm *)message->data;
		if (req->should_exit) {
			CMR_LOGD("req id %d should exit, return buf fd 0x%x\n",
				req->req_in.request_id, frame->fd);
			free(frame->reserved);
			req->cb(req->client_data, &req->req_in, IPS_CB_RETURN_BUF, (void *)frame);
			break;
		}
		req->status = IPS_REQ_PROC_START;

		CMR_LOGD("frame id %d, fd 0x%x, param %p\n", frame->frame_number, frame->fd, frame->reserved);

		struct swa_frame_param *frm_param = (struct swa_frame_param *)frame->reserved;
		if (frm_param) {
			struct swa_common_info *com_info = &frm_param->common_param;

			CMR_LOGD("iso %d, bv %d, gain %d, ext %d,  ang %d, sn_ori %d, flip %d, zoom %f, hdr_ev %f %f %f\n",
				com_info->iso, com_info->bv, com_info->again, com_info->exp_time,
				com_info->angle, com_info->sensor_orientation, com_info->flip_on, com_info->zoom_ratio,
				frm_param->hdr_param.ev[0], frm_param->hdr_param.ev[1],
				frm_param->hdr_param.ev[2]);
		}

		switch (cur_proc->type) {
		case IPS_TYPE_JPEG:
			ipmpro_jpeg(ips_ctx, req, frame);
			break;
		case IPS_TYPE_HDR:
			ipmpro_hdr(ips_ctx, req, frame);
			break;
		case IPS_TYPE_MFNR:
			ipmpro_mfnr(ips_ctx, req, frame);
			break;
		case IPS_TYPE_MFSR:
			ipmpro_mfsr_mfsr(ips_ctx, req, frame);
			break;
		case IPS_TYPE_UWARP:
		case IPS_TYPE_SHARP:
		case IPS_TYPE_CNR:
		case IPS_TYPE_DREPRO:
		case IPS_TYPE_FILTER:
		case IPS_TYPE_FB:
		case IPS_TYPE_WATERMARK:
			ipmpro_common(ips_ctx, req, frame);
			break;
		default:
			CMR_LOGE("fail to do type %d\n", cur_proc->type);
			break;
		}
		break;

	case IPS_EVT_PROC_DONE:
		if (cur_proc->type == IPS_TYPE_JPEG) {
			ipmpro_jpeg_done(ips_ctx, req, (struct jpeg_enc_cb_param *)message->data);
		}
		req->status = IPS_REQ_PROC_DONE;
		break;

	case IPS_EVT_SYNC_WAIT:
		CMR_LOGD("wait done here, current status %d\n", req->status);
		req->status = IPS_REQ_ALL_DONE;
		break;

	default:
		CMR_LOGI("Unknown msg 0x%x\n", message->msg_type);
		req->status = IPS_REQ_ALL_DONE;
		break;
	}

	if (req->should_exit && req->frame_total > 0) {
		CMR_LOGD("req id %d should exit\n", req->req_in.request_id);
		for (i = 0; i < req->frame_total; i++) {
			CMR_LOGD("req id %d return frame No.%d, fd 0x%x\n",
				req->req_in.request_id, i, req->frm_out[i].fd);
			if (req->frm_out[i].fd == 0)
				continue;
			if (req->req_in.request_id != IPS_THUMB_REQID)
				free(req->frm_out[i].reserved);
			req->cb(req->client_data, &req->req_in, IPS_CB_RETURN_BUF, (void *)&req->frm_out[i]);
			memset(&req->frm_out[i], 0, sizeof(struct img_frm));
		}
		for (i = 0; i < req->frame_total; i++) {
			if (req->frm_middle[i].fd == 0)
				continue;
			req->cb(req->client_data, &req->req_in, IPS_CB_RETURN_BUF, (void *)&req->frm_middle[i]);
			memset(&req->frm_middle[i], 0, sizeof(struct img_frm));
		}

		if (cur_proc->type == IPS_TYPE_MFNR)
			ipmpro_mfnr(ips_ctx, req, NULL);
		if (cur_proc->type == IPS_TYPE_MFSR)
			ipmpro_mfsr_mfsr(ips_ctx, req, NULL);

		req->frame_total = 0;

		if (req->frm_jpeg.fd) {
			CMR_LOGD("req id %d return jpeg buf fd 0x%x\n", req->req_in.request_id, req->frm_jpeg.fd);
			req->cb(req->client_data, &req->req_in, IPS_CB_RETURN_BUF, (void *)&req->frm_jpeg);
			memset(&req->frm_jpeg, 0, sizeof(struct img_frm));
		}
		return ret;
	}

	if (req->status == IPS_REQ_PROC_DONE) {
		if (cur_proc->list.next == &req->ipm_head) {
			CMR_LOGD("request %d proc type %d is last step", req->req_in.request_id, cur_proc->type);
			req->cb(req->client_data, &req->req_in, IPS_CB_ALL_DONE, NULL);
			req->status = IPS_REQ_ALL_DONE;
		} else {
			next_proc = node_to_item(cur_proc->list.next, struct ipmpro_node, list);
			req->cur_proc = next_proc;
			req->frame_cnt = 0;
			CMR_LOGD("req id %d next proc type %d", req->req_in.request_id, next_proc->type);
			for (i = 0; i < req->frame_total; i++) {
				CMR_LOGD("req id %d post frame No.%d, fd 0x%x\n",
					req->req_in.request_id, i, req->frm_out[i].fd);
				cmr_ips_post(ips_ctx, &req->req_in, &req->frm_out[i]);
				memset(&req->frm_out[i], 0, sizeof(struct img_frm));
			}
		}
	}
	CMR_LOGD("Done. req id %d, req %p, cur_proc %p status %d\n",
		req->req_in.request_id, req, req->cur_proc, req->status);
	return ret;
}

static cmr_int init_ips_thread(struct ips_thread_context *ips_thread, cmr_handle ips_handle)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;

	ret = cmr_thread_create(&ips_thread->thr_handle, IPS_MSG_QUEUE_SIZE,
			ips_thread_proc, (void *)ips_thread, "ips_service");
	if (ret) {
		CMR_LOGE("fail to create ips_service thread\n");
		return ret;
	}
	ips_thread->init = 1;
	ips_thread->status = IPS_THREAD_IDLE;
	ips_thread->ips_handle = ips_handle;
	pthread_mutex_init(&ips_thread->lock, NULL);
	CMR_LOGI("new thread %p, idx %d\n", ips_thread, ips_thread->idx);
	return ret;
}

static cmr_int deinit_ips_thread(struct ips_thread_context *ips_thread)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;

	/* we should release all request and unbind their threads before this called */
	if (ips_thread->status != IPS_THREAD_IDLE || ips_thread->req_handle) {
		CMR_LOGE("Error status %d, handle %p, should be idle here\n",
				ips_thread->status, ips_thread->req_handle);
	}

	CMR_LOGD("thread %d start\n", ips_thread->idx);

	if (ips_thread->thr_handle) {
		ret = cmr_thread_destroy(ips_thread->thr_handle);
		if (ret)
			CMR_LOGE("fail to destroy thread");
	}
	pthread_mutex_destroy(&ips_thread->lock);

	memset(ips_thread, 0, sizeof(struct ips_thread_context));

	return CMR_CAMERA_SUCCESS;
}


static cmr_int get_ips_thread(struct ips_context *ips_ctx, struct ips_thread_context **dst)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;
	cmr_u32 i, found = 0;
	struct ips_thread_context *cur = NULL;

search:
	for (i = 0; i < ips_ctx->thread_cnt; i++) {
		cur = &ips_ctx->ips_threads[i];
		if (!cur->init)
			continue;

		pthread_mutex_lock(&cur->lock);
		if (cur->status == IPS_THREAD_IDLE) {
			found = 1;
			*dst = cur;
			cur->status = IPS_THREAD_BUSY;
			pthread_mutex_unlock(&cur->lock);
			break;
		}
		pthread_mutex_unlock(&cur->lock);
	}

	if (found)
		goto exit;

	/* create a new thread */
	i = ips_ctx->thread_cnt;
	if (ips_ctx->thread_cnt < IPS_THREAD_NUM) {
		ret = init_ips_thread(&ips_ctx->ips_threads[i], ips_ctx);
		if (ret) {
			CMR_LOGE("fail to init ips thread %d\n", i);
			goto exit;
		}
		ips_ctx->ips_threads[i].idx = i;
		ips_ctx->thread_cnt++;
		goto search;
	}

exit:
	CMR_LOGD("X. ret %ld, thread %p\n", ret, *dst);
	return ret;
}

static cmr_int put_ips_thread(struct ips_thread_context *cur)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;

	pthread_mutex_lock(&cur->lock);
	if (cur->status == IPS_THREAD_IDLE) {
		CMR_LOGE("thread No.%d error status. req %p", cur->idx, cur->req_handle);
	}
	if (cur->req_handle) {
		struct ips_req_node *req = (struct ips_req_node *)cur->req_handle;
		req->thrd = NULL;
	}
	cur->status = IPS_THREAD_IDLE;
	cur->req_handle = (cmr_handle)NULL;
	pthread_mutex_unlock(&cur->lock);

	CMR_LOGD("Done. thread No.%d, ptr %p\n", cur->idx, cur);
	return ret;
}

cmr_int cmr_ips_set_jpeg_param(cmr_handle ips_handle,
	struct ips_request_t *dst, void *param)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;
	struct ips_req_node *req;
	struct ips_jpeg_param_t *pm;
	struct ipmpro_node *proc = NULL;
	struct listnode *node;

	req = container_of(dst, struct ips_req_node, req_in);
	list_for_each(node,  &req->ipm_head) {
		proc = node_to_item(node, struct ipmpro_node , list);
		if (proc->type == IPS_TYPE_JPEG)
			break;
	}

	if (proc && proc->type == IPS_TYPE_JPEG) {
		memcpy(proc->param_ptr, param, sizeof(struct ips_jpeg_param_t));
		pm = (struct ips_jpeg_param_t *)proc->param_ptr;
		CMR_LOGD("jpeg param: quality %d, flip %d rot %d mirror %d\n",
			pm->mean.quality_level,
			pm->mean.flip, pm->mean.rot, pm->mean.mirror);
		CMR_LOGD("exif w %d h %d, exp_idx %d %d exp_tm %d %d, fnum %d %d\n",
			pm->exif_data.exif_info.primary.basic.ImageWidth,
			pm->exif_data.exif_info.primary.basic.ImageLength,
			pm->exif_data.spec_pic.ExposureIndex.numerator,
			pm->exif_data.spec_pic.ExposureIndex.denominator,
			pm->exif_data.spec_pic.ExposureTime.numerator,
			pm->exif_data.spec_pic.ExposureTime.denominator,
			pm->exif_data.spec_pic.FNumber.numerator,
			pm->exif_data.spec_pic.FNumber.denominator);
	} else {
		CMR_LOGE("fail to get JPEG proc node\n");
	}

	return ret;
}

cmr_int cmr_ips_jpeg_done(cmr_handle ips_handle,
	struct jpeg_enc_cb_param *cb_data)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;
	int found = 0;
	struct ips_context *ips_ctx = (struct ips_context *)ips_handle;
	struct listnode *node = NULL;
	struct ips_req_node *req = ips_ctx->req_in_jpeg;
	CMR_MSG_INIT(message);

	if (req)
		found = 1;

	if (found == 0) {
		CMR_LOGD("No request waiting jpeg\n");
		return ret;
	}

	message.data = malloc(sizeof(struct jpeg_enc_cb_param));
	if (message.data == NULL) {
		CMR_LOGE("fail to malloc\n");
		return CMR_CAMERA_NO_MEM;
	}
	memcpy(message.data, cb_data, sizeof(struct jpeg_enc_cb_param));
	message.msg_type = IPS_EVT_PROC_DONE;
	message.sync_flag = CMR_MSG_SYNC_NONE;
	message.alloc_flag = 1;
	ret = cmr_thread_msg_send(req->thrd->thr_handle, &message);
	if (ret) {
		CMR_LOGE("failed to send proc message to  ips thread %ld", ret);
		free(message.data);
		ret = CMR_CAMERA_FAIL;
	}

	CMR_LOGD("req_id %d jpeg done\n", req->req_in.request_id);
	return ret;
}

cmr_int cmr_ips_post(cmr_handle ips_handle,
	struct ips_request_t *src, struct img_frm *frame)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;
	struct ips_req_node *req;
	struct ips_context *ips_ctx = (struct ips_context *)ips_handle;
	CMR_MSG_INIT(message);

	req = container_of(src, struct ips_req_node, req_in);
	CMR_LOGD("req id %d, req %p %p, cur_proc %p, frame %p\n",
		req->req_in.request_id, src, req, req->cur_proc, frame);

	if (req->thrd == NULL) {
		int loop = 0;
		while (loop++ < 30) {
			ret = get_ips_thread(ips_ctx, &req->thrd);
			if (ret || req->thrd == NULL) {
				usleep(30 * 1000);
				continue;
			}
			break;
		}
		if (!req->thrd) {
			CMR_LOGE("fail to get available thread\n");
			return CMR_CAMERA_FAIL;
		}
		CMR_LOGD("get thread No.%d for req_id %d\n", req->thrd->idx, src->request_id);
		req->thrd->req_handle = (cmr_handle)req;
	}

	message.data = malloc(sizeof(struct img_frm));
	if (message.data == NULL) {
		CMR_LOGE("fail to malloc\n");
		return CMR_CAMERA_NO_MEM;
	}
	memcpy(message.data, frame, sizeof(struct img_frm));
	message.msg_type = IPS_EVT_PROC;
	message.sync_flag = CMR_MSG_SYNC_NONE;
	message.alloc_flag = 1;
	ret = cmr_thread_msg_send(req->thrd->thr_handle, &message);
	if (ret) {
		CMR_LOGE("failed to send proc message to  ips thread %ld", ret);
		free(message.data);
		ret = CMR_CAMERA_FAIL;
	}

	return ret;
}

cmr_int cmr_ips_create_req(cmr_handle ips_handle,
	struct ips_request_t **dst, cmr_handle client_data)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;
	struct ips_req_node *new_req;
	struct ips_context *ips_ctx = (struct ips_context *)ips_handle;

	if (!ips_handle || dst == NULL) {
		CMR_LOGE("fail to get input ptr\n");
		return CMR_CAMERA_FAIL;
	}
	*dst = NULL;

	new_req = (struct ips_req_node *)malloc(sizeof(struct ips_req_node));
	if (new_req == NULL) {
		CMR_LOGE("fail to alloc new ips request\n");
		return CMR_CAMERA_NO_MEM;
	}
	memset(new_req, 0, sizeof(struct ips_req_node));
	new_req->client_data = client_data;
	new_req->status = IPS_REQ_NEW;
	new_req->should_exit = 0;
	list_init(&new_req->ipm_head);

	pthread_mutex_lock(&ips_ctx->listlock);
	list_add_tail(&ips_ctx->req_head, &new_req->list);
	ips_ctx->req_count++;
	pthread_mutex_unlock(&ips_ctx->listlock);

	*dst = &new_req->req_in;
	CMR_LOGD("done req %p %p, total req count %d\n", new_req, *dst, ips_ctx->req_count);
	return ret;
}

cmr_int cmr_ips_quickstop_req(cmr_handle ips_handle,
	struct ips_request_t *dst)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;
	struct ips_req_node *req;
	struct ips_context *ips_ctx = (struct ips_context *)ips_handle;

	if (!ips_handle || !dst) {
		CMR_LOGE("fail to get input ptr\n");
		return CMR_CAMERA_FAIL;
	}
	req = container_of(dst, struct ips_req_node, req_in);
	CMR_LOGD("E. req_id %d\n", req->req_in.request_id);

	req->should_exit = 1;
	return ret;
}

cmr_int cmr_ips_destroy_req(cmr_handle ips_handle,
	struct ips_request_t **dst)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;
	cmr_u32 i;
	struct ips_req_node *req;
	struct ips_context *ips_ctx = (struct ips_context *)ips_handle;
	struct ips_thread_context *ips_thread;
	struct ipmpro_node *ipm_hdl;
	struct listnode *node, *next;
	CMR_MSG_INIT(message);

	if (!ips_handle || !dst || (*dst == NULL)) {
		CMR_LOGE("fail to get input ptr\n");
		return CMR_CAMERA_FAIL;
	}
	req = container_of(*dst, struct ips_req_node, req_in);
	CMR_LOGD("E. req_id %d\n", req->req_in.request_id);

	req->should_exit = 1;
	ips_thread = req->thrd;
	if (ips_thread) {
		/* wait thread done */
		message.msg_type = IPS_EVT_SYNC_WAIT;
		message.sync_flag = CMR_MSG_SYNC_PROCESSED;
		message.alloc_flag = 0;
		ret = cmr_thread_msg_send(ips_thread->thr_handle, &message);
		if (ret) {
			CMR_LOGE("failed to send proc message to  ips thread %ld", ret);
			ret = CMR_CAMERA_FAIL;
		}
		/* release thread */
		put_ips_thread(ips_thread);
	}

	list_for_each_safe(node, next, &req->ipm_head) {
		ipm_hdl = node_to_item(node, struct ipmpro_node , list);
		if (ipm_hdl->swa_handle && (ipm_hdl->type != IPS_TYPE_JPEG)) {
			free(ipm_hdl->swa_handle);
			ipm_hdl->swa_handle = NULL;
		}
		if (ipm_hdl->param_ptr) {
			free(ipm_hdl->param_ptr);
			ipm_hdl->param_ptr = NULL;
		}
		list_remove(&ipm_hdl->list);
		free(ipm_hdl);
	}

	for (i = 0; i < IPS_TYPE_MAX; i++) {
		if (req->dbg_info[i].data)
			free(req->dbg_info[i].data);
	}

	CMR_LOGD("request id %d freed\n", req->req_in.request_id);

	pthread_mutex_lock(&ips_ctx->listlock);
	list_remove(&req->list);
	ips_ctx->req_count--;
	pthread_mutex_unlock(&ips_ctx->listlock);

	free(req);
	*dst = NULL;
	return ret;
}

cmr_int cmr_ips_init_req(cmr_handle ips_handle,
	struct ips_request_t *dst, void *data, ips_req_cb req_cb)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;
	int i, j;
	cmr_u32 type;
	struct ips_req_node *req;
	struct ips_context *ips_ctx = (struct ips_context *)ips_handle;
	struct ipmpro_node *ipm_hdl;
	struct ipmpro_handle_base *ipm_base;

	if (!ips_handle || dst == NULL) {
		CMR_LOGE("fail to get input ptr\n");
		return CMR_CAMERA_FAIL;
	}
	req = container_of(dst, struct ips_req_node, req_in);
	req->cb = req_cb;
	req->status = IPS_REQ_INIT;
	req->frame_total = dst->frame_total;
	if (data)
		memcpy(&req->init_param, data, sizeof(struct swa_init_data));
	CMR_LOGD("req_id  %d, data %p,  init size %d %d\n", dst->request_id, data,
		req->init_param.frame_size.width, req->init_param.frame_size.height);

	for (i = 0, j = 0; i < IPS_STEP_MAX; i++) {
		type = dst->proc_steps[i].type;
		if (type >= IPS_TYPE_MAX)
			break;

		ipm_base = &ips_ctx->ipmpro_base[type];
		if (ipm_base->inited == 0) {
			CMR_LOGE("ips type %d is not inited\n", type);
			continue;
		}

		ipm_hdl = malloc(sizeof(struct ipmpro_node));
		if (ipm_hdl == NULL) {
			CMR_LOGE("fail to malloc handle for type %d\n", type);
			continue;
		}

		CMR_LOGD("No.%d, type %d\n", i, type);
		memset(ipm_hdl, 0, sizeof(struct ipmpro_node));
		ipm_hdl->log_level = 4;
		ipm_hdl->type = type;
		ipm_hdl->ipm_base = ipm_base;
		if (type == IPS_TYPE_JPEG) {
			ipm_hdl->swa_handle = dst->proc_steps[i].handle; // JPEG handle from oem open
			ipm_hdl->param_ptr = malloc(sizeof(struct ips_jpeg_param_t));
			ipm_hdl->is_async = 1;
		} else if (ipm_base->swa_handle_size) {
			ipm_hdl->swa_handle = malloc(ipm_base->swa_handle_size);
			if(ipm_hdl->swa_handle)
				memset(ipm_hdl->swa_handle, 0, ipm_base->swa_handle_size);
		}
		list_add_tail(&req->ipm_head, &ipm_hdl->list);
		j++;
	}

	if (j == 0) {
		CMR_LOGE("fail to init any proc handle\n");
		return CMR_CAMERA_FAIL;
	}

	/* set to first proc node */
	req->cur_proc = node_to_item(req->ipm_head.next, struct ipmpro_node, list);
	return ret;
}

cmr_int cmr_ips_set_handles(cmr_handle ips_handle, struct ips_handle_t *data)
{
	struct ips_context *ips_ctx = (struct ips_context *)ips_handle;

	if (!ips_handle || !data) {
		CMR_LOGE("fail to get valid ptr\n");
		return CMR_CAMERA_FAIL;
	}

	memcpy(&ips_ctx->other_handles, data, sizeof(struct ips_handle_t));
	CMR_LOGD("handle %p %p\n",
		ips_ctx->other_handles.jpeg_handle,
		ips_ctx->other_handles.scale_handle);
	return 0;
}

cmr_int cmr_ips_init(cmr_handle *handle, cmr_handle client_data)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;
	cmr_u32 i;
	struct ips_context *ips_ctx;

	char value[PROPERTY_VALUE_MAX];
	property_get("debug.camera.save.snpfile", value, "0");
	dump_pic = atoi(value);
	property_get("ro.debuggable", value, "0");
	s_dbg_ver = !!atoi(value);

	ips_ctx = (struct ips_context *)malloc(sizeof(struct ips_context));
	if (ips_ctx == NULL) {
		CMR_LOGE("fail to malloc ips handle\n");
		return CMR_CAMERA_NO_MEM;
	}
	memset(ips_ctx, 0, sizeof(struct ips_context));

	pthread_mutex_init(&ips_ctx->listlock, NULL);
	list_init(&ips_ctx->req_head);

	pthread_mutex_init(&ips_ctx->thrd_lock, NULL);
	for (i = 0; i < IPS_THREAD_NUM; i++) {
		ips_ctx->ips_threads[i].idx = i;
	}

	for (i = 0; i < IPS_TYPE_MAX; i++) {
		if (i == IPS_TYPE_JPEG) {
			init_jpeg_base(ips_ctx);
			continue;
		}
		init_ipmpro_base(&ipmproc_list[i], &ips_ctx->ipmpro_base[i]);
	}

	ips_ctx->client_data = client_data;
	*handle = (cmr_handle)ips_ctx;

	return ret;
}

cmr_int cmr_ips_deinit(cmr_handle handle)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;
	cmr_u32 i;
	struct ips_context *ips_ctx = (struct ips_context *)handle;

	if (ips_ctx == NULL) {
		CMR_LOGE("fail to get ips handle\n");
		return CMR_CAMERA_INVALID_PARAM;
	}

	/* TODO - release all request and unbind their threads */

	pthread_mutex_destroy(&ips_ctx->listlock);

	/* deinit ips threads */
	for (i = 0; i < IPS_THREAD_NUM; i++) {
		if (ips_ctx->ips_threads[i].init == 0)
			continue;
		deinit_ips_thread(&ips_ctx->ips_threads[i]);
	}
	pthread_mutex_destroy(&ips_ctx->thrd_lock);

	/* deinit sw process lib base */
	for (i = 0; i < IPS_TYPE_MAX; i++) {
		if (i == IPS_TYPE_JPEG) {
			deinit_jpeg_base(ips_ctx);
			continue;
		}
		deinit_ipmpro_base(&ips_ctx->ipmpro_base[i]);
	}

	free(ips_ctx);
	CMR_LOGD("Done\n");
	return ret;
}
