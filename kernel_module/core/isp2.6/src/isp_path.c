/*
 * Copyright (C) 2017-2018 Spreadtrum Communications Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */


#include <linux/types.h>
#include <linux/kernel.h>
#include <sprd_mm.h>
#include <sprd_isp_r8p1.h>
#include "sprd_img.h"

#include "cam_types.h"
#include "cam_queue.h"
#include "cam_scaler.h"

#include "isp_reg.h"

#include "isp_interface.h"
#include "isp_core.h"
#include "isp_path.h"

#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt) "ISP_PATH: %d %d %s : "\
	fmt, current->pid, __LINE__, __func__


#define SHRINK_Y_UP_TH 235
#define SHRINK_Y_DN_TH 16
#define SHRINK_UV_UP_TH 240
#define SHRINK_UV_DN_TH 16
#define SHRINK_Y_OFFSET 16
#define SHRINK_Y_RANGE 2
#define SHRINK_C_OFFSET 16
#define SHRINK_C_RANGE 6

#define ISP_PATH_DECI_FAC_MAX       4
#define ISP_SC_COEFF_UP_MAX         4
#define ISP_SC_COEFF_DOWN_MAX       4

unsigned long coff_buf_addr[2][3][4] = {
	{
		{
			ISP_SCALER_PRE_LUMA_HCOEFF_BUF0,
			ISP_SCALER_PRE_CHROMA_HCOEFF_BUF0,
			ISP_SCALER_PRE_LUMA_VCOEFF_BUF0,
			ISP_SCALER_PRE_CHROMA_VCOEFF_BUF0,
		},
		{
			ISP_SCALER_VID_LUMA_HCOEFF_BUF0,
			ISP_SCALER_VID_CHROMA_HCOEFF_BUF0,
			ISP_SCALER_VID_LUMA_VCOEFF_BUF0,
			ISP_SCALER_VID_CHROMA_VCOEFF_BUF0,
		}
	},
	{
		{
			ISP_SCALER_PRE_LUMA_HCOEFF_BUF1,
			ISP_SCALER_PRE_CHROMA_HCOEFF_BUF1,
			ISP_SCALER_PRE_LUMA_VCOEFF_BUF1,
			ISP_SCALER_PRE_CHROMA_VCOEFF_BUF1,
		},
		{
			ISP_SCALER_VID_LUMA_HCOEFF_BUF1,
			ISP_SCALER_VID_CHROMA_HCOEFF_BUF1,
			ISP_SCALER_VID_LUMA_VCOEFF_BUF1,
			ISP_SCALER_VID_CHROMA_VCOEFF_BUF1,
		}
	},
};

static unsigned long store_base[ISP_SPATH_NUM] = {
	ISP_STORE_PRE_CAP_BASE,
	ISP_STORE_VID_BASE,
	ISP_STORE_THUMB_BASE,
};

static unsigned long scaler_base[ISP_SPATH_NUM] = {
	ISP_SCALER_PRE_CAP_BASE,
	ISP_SCALER_VID_BASE,
	ISP_SCALER_THUMB_BASE,
};

static uint32_t get_path_deci_factor(
				uint32_t src_size, uint32_t dst_size)
{
	uint32_t factor = 0;

	if (0 == src_size || 0 == dst_size)
		return factor;

	/* factor: 0 - 1/2, 1 - 1/4, 2 - 1/8, 3 - 1/16 */
	for (factor = 0; factor < ISP_PATH_DECI_FAC_MAX; factor++) {
		if (src_size < (uint32_t) (dst_size * (1 << (factor + 1))))
			break;
	}

	return factor;
}

static enum isp_store_format get_store_format(uint32_t forcc)
{
	enum isp_store_format format = ISP_STORE_FORMAT_MAX;

	switch (forcc) {
	case IMG_PIX_FMT_UYVY:
		format = ISP_STORE_UYVY;
		break;
	case IMG_PIX_FMT_YUV422P:
		format = ISP_STORE_YUV422_3FRAME;
		break;
	case IMG_PIX_FMT_NV12:
		format = ISP_STORE_YUV420_2FRAME;
		break;
	case IMG_PIX_FMT_NV21:
		format = ISP_STORE_YVU420_2FRAME;
		break;
	case IMG_PIX_FMT_YUV420:
		format = ISP_STORE_YUV420_3FRAME;
		break;
	default:
		format = ISP_STORE_FORMAT_MAX;
		pr_err("error, format 0x%x not support\n", forcc);
		break;
	}
	return format;
}

static enum isp_fetch_format get_fetch_format(uint32_t forcc)
{
	enum isp_fetch_format format = ISP_FETCH_FORMAT_MAX;

	switch (forcc) {
	case IMG_PIX_FMT_GREY:
		format = ISP_FETCH_CSI2_RAW10;
		break;
	case IMG_PIX_FMT_UYVY:
		format = ISP_FETCH_UYVY;
		break;
	case IMG_PIX_FMT_YUV422P:
		format = ISP_FETCH_YUV422_3FRAME;
		break;
	case IMG_PIX_FMT_NV12:
		format = ISP_FETCH_YUV420_2FRAME;
		break;
	case IMG_PIX_FMT_NV21:
		format = ISP_FETCH_YVU420_2FRAME;
		break;
	default:
		format = ISP_STORE_FORMAT_MAX;
		pr_err("error, format 0x%x not support\n", forcc);
		break;
	}
	return format;
}

static int calc_scaler_param(struct img_trim *in_trim,
					struct img_size *out_size,
					struct isp_scaler_info *scaler,
					struct img_deci_info *deci)
{
	int ret = 0;
	unsigned int tmp_dstsize = 0;
	unsigned int align_size = 0;
	unsigned int d_max = ISP_SC_COEFF_DOWN_MAX;
	unsigned int u_max = ISP_SC_COEFF_UP_MAX;
	unsigned int f_max = ISP_PATH_DECI_FAC_MAX;

	pr_debug("in_trim_size_x:%d, in_trim_size_y:%d, out_size_w:%d,out_size_h:%d\n",
		in_trim->size_x, in_trim->size_y, out_size->w, out_size->h);

	if (in_trim->size_x > (out_size->w * d_max * (1 << f_max)) ||
		in_trim->size_y > (out_size->h * d_max * (1 << f_max)) ||
		in_trim->size_x * u_max < out_size->w ||
		in_trim->size_y * u_max < out_size->h) {
		pr_err("in_trim %d %d. out _size %d %d, fmax %d, u_max %d\n",
				in_trim->size_x, in_trim->size_y,
				out_size->w, out_size->h, f_max, d_max);
		ret = -EINVAL;
	} else {
		scaler->scaler_factor_in = in_trim->size_x;
		scaler->scaler_ver_factor_in = in_trim->size_y;
		if (in_trim->size_x > out_size->w * d_max) {
			tmp_dstsize = out_size->w * d_max;
			deci->deci_x =
				get_path_deci_factor(in_trim->size_x,
								tmp_dstsize);
			deci->deci_x_eb = 1;
			align_size = (1 << (deci->deci_x + 1)) *
				ISP_PIXEL_ALIGN_WIDTH;
			in_trim->size_x = (in_trim->size_x)
				& ~(align_size - 1);
			in_trim->start_x = (in_trim->start_x)
				& ~(align_size - 1);
			scaler->scaler_factor_in =
				in_trim->size_x >> (deci->deci_x + 1);
		} else {
			deci->deci_x = 1;
			deci->deci_x_eb = 0;
		}

		if (in_trim->size_y > out_size->h * d_max) {
			tmp_dstsize = out_size->h * d_max;
			deci->deci_y =
				get_path_deci_factor(in_trim->size_y,
								tmp_dstsize);
			deci->deci_y_eb = 1;
			align_size = (1 << (deci->deci_y + 1)) *
				ISP_PIXEL_ALIGN_HEIGHT;
			in_trim->size_y = (in_trim->size_y)
				& ~(align_size - 1);
			in_trim->start_y = (in_trim->start_y)
				& ~(align_size - 1);
			scaler->scaler_ver_factor_in =
				in_trim->size_y >> (deci->deci_y + 1);
		} else {
			deci->deci_y = 1;
			deci->deci_y_eb = 0;
		}

		scaler->scaler_factor_out = out_size->w;
		scaler->scaler_ver_factor_out = out_size->h;
		scaler->scaler_out_width = out_size->w;
		scaler->scaler_out_height = out_size->h;
	}

	return ret;
}

static int calc_scaler_coeff(
					struct isp_scaler_info *scaler,
					uint32_t scale2yuv420)
{
	uint32_t *tmp_buf = NULL;
	uint32_t *h_coeff = NULL;
	uint32_t *v_coeff = NULL;
	uint32_t *v_chroma_coeff = NULL;
	uint8_t y_tap = 0;
	uint8_t uv_tap = 0;

	tmp_buf = scaler->coeff_buf;
	h_coeff = tmp_buf;
	v_coeff = tmp_buf + (ISP_SC_COEFF_COEF_SIZE / 4);
	v_chroma_coeff = v_coeff + (ISP_SC_COEFF_COEF_SIZE / 4);

	if (!(isp_gen_scale_coeff((short)scaler->scaler_factor_in,
				(short)scaler->scaler_ver_factor_in,
				(short)scaler->scaler_factor_out,
				(short)scaler->scaler_ver_factor_out,
				h_coeff,
				v_coeff,
				v_chroma_coeff,
				scale2yuv420,
				&y_tap,
				&uv_tap,
				tmp_buf + (ISP_SC_COEFF_COEF_SIZE * 3 / 4),
				ISP_SC_COEFF_TMP_SIZE))) {
		pr_err("fail to call dcam_gen_scale_coeff\n");
		return -EINVAL;
	}

	scaler->scaler_y_ver_tap = y_tap;
	scaler->scaler_uv_ver_tap = uv_tap;

	return 0;
}

int cfg_path_scaler(struct isp_path_desc *path)
{
	int ret = 0;
	uint32_t scale2yuv420 = 0;

	ret = calc_scaler_param(&path->in_trim, &path->dst,
						&path->scaler, &path->deci);
	if (ret) {
		pr_err("error to set scaler.");
		return ret;
	}

	if ((path->scaler.scaler_factor_in == path->scaler.scaler_factor_out) &&
		(path->scaler.scaler_ver_factor_in ==
		path->scaler.scaler_ver_factor_out) &&
		((path->out_fmt == IMG_PIX_FMT_YUV422P) ||
		(path->out_fmt == IMG_PIX_FMT_UYVY)))  {

		path->scaler.scaler_bypass = 1;
	} else {
		path->scaler.scaler_bypass = 0;
		if ((path->out_fmt == IMG_PIX_FMT_NV12) ||
			(path->out_fmt == IMG_PIX_FMT_NV21) ||
			(path->out_fmt == IMG_PIX_FMT_YUV420))
			scale2yuv420 = 1;
		ret = calc_scaler_coeff(&path->scaler, scale2yuv420);
	}

	if ((path->out_fmt == IMG_PIX_FMT_YUV422P) ||
			(path->out_fmt == IMG_PIX_FMT_UYVY))
		path->scaler.odata_mode = 0x00;
	else
		path->scaler.odata_mode = 0x01;

	return ret;
}

static uint32_t cal_deci(uint32_t src, uint32_t dst)
{
	uint32_t deci = 1;

	if (src <= dst * 4)
		deci = 1;
	else if (src <= dst * 8)
		deci = 2;
	else if (src <= dst * 16)
		deci = 4;
	else if (src <= dst * 32)
		deci = 8;
	else if (src <= dst * 64)
		deci = 16;
	else
		deci = 0;
	return deci;
}

static int cal_trim_deci_info(
		uint32_t src, uint32_t dst,
		uint32_t *trim, uint32_t *deci)
{
	uint32_t tmp;

	tmp = cal_deci(src, dst);
	if (tmp == 0)
		return -EINVAL;

	if ((src % (2 * tmp)) == 0) {
		*trim = src;
		*deci = tmp;
	} else {
		*trim = (src / (2 * tmp) * (2 * tmp));
		*deci = cal_deci(*trim, dst);
	}
	return 0;
}

int cfg_path_thumbscaler(struct isp_path_desc *path)
{
	int ret = 0;
	uint32_t deci_w, deci_h, trim_w, trim_h;
	uint32_t offset, shift;
	struct img_size src, dst;
	struct isp_thumbscaler_info *scalerInfo;

	scalerInfo = &path->thumbscaler;
	scalerInfo->scaler_bypass = 0;

	/* y factor & deci */
	src.w = path->in_trim.size_x;
	src.h = path->in_trim.size_y;
	dst = path->dst;
	ret = cal_trim_deci_info(src.w, dst.w, &trim_w, &deci_w);
	ret |= cal_trim_deci_info(src.h, dst.h, &trim_h, &deci_h);
	if (ret) {
		pr_err("fail to set thumbscaler ydeci. src %d %d, dst %d %d\n",
					src.w, src.h, dst.w, dst.h);
		return ret;
	}

	scalerInfo->y_deci.deci_x = deci_w;
	scalerInfo->y_deci.deci_y = deci_h;
	if (deci_w > 1)
		scalerInfo->y_deci.deci_x_eb = 1;
	else
		scalerInfo->y_deci.deci_x_eb = 0;
	if (deci_h > 1)
		scalerInfo->y_deci.deci_y_eb = 1;
	else
		scalerInfo->y_deci.deci_y_eb = 0;
	scalerInfo->y_factor_in.w = trim_w / deci_w;
	scalerInfo->y_factor_in.h = trim_h / deci_h;
	scalerInfo->y_factor_out = path->dst;

	/* uv factor & deci, input: yuv422 */
	if ((path->store.color_fmt == ISP_STORE_YUV420_2FRAME) ||
		(path->store.color_fmt == ISP_STORE_YVU420_2FRAME) ||
		(path->store.color_fmt == ISP_STORE_YUV420_3FRAME))
		shift = 1;
	else
		shift = 0;

	scalerInfo->uv_deci.deci_x = deci_w;
	scalerInfo->uv_deci.deci_y = deci_h;
	if (deci_w > 1)
		scalerInfo->uv_deci.deci_x_eb = 1;
	else
		scalerInfo->uv_deci.deci_x_eb = 0;
	if (deci_h > 1)
		scalerInfo->uv_deci.deci_y_eb = 1;
	else
		scalerInfo->uv_deci.deci_y_eb = 0;
	trim_w >>= 1;
	scalerInfo->uv_factor_in.w = trim_w / deci_w;
	scalerInfo->uv_factor_in.h = trim_h / deci_h;
	scalerInfo->uv_factor_out.w = dst.w / 2;
	scalerInfo->uv_factor_out.h = dst.h >> shift;

	scalerInfo->src0.w = path->in_trim.size_x;
	scalerInfo->src0.h = path->in_trim.size_y;

	/* y trim */
	trim_w = scalerInfo->y_factor_in.w * scalerInfo->y_deci.deci_x;
	offset = (path->in_trim.size_x - trim_w) / 2;
	scalerInfo->y_trim.start_x = path->in_trim.start_x + offset;
	scalerInfo->y_trim.size_x = trim_w;

	trim_h = scalerInfo->y_factor_in.h * scalerInfo->y_deci.deci_y;
	offset = (path->in_trim.size_y - trim_h) / 2;
	scalerInfo->y_trim.start_y = path->in_trim.start_y + offset;
	scalerInfo->y_trim.size_y = trim_h;

	scalerInfo->y_src_after_deci = scalerInfo->y_factor_in;
	scalerInfo->y_dst_after_scaler = scalerInfo->y_factor_out;

	/* uv trim */
	trim_w = scalerInfo->uv_factor_in.w * scalerInfo->uv_deci.deci_x;
	offset = (path->in_trim.size_x / 2 - trim_w) / 2;
	scalerInfo->uv_trim.start_x = path->in_trim.start_x / 2 + offset;
	scalerInfo->uv_trim.size_x = trim_w;

	trim_h = scalerInfo->uv_factor_in.h * scalerInfo->uv_deci.deci_y;
	offset = (path->in_trim.size_y - trim_h) / 2;
	scalerInfo->uv_trim.start_y = path->in_trim.start_y + offset;
	scalerInfo->uv_trim.size_y = trim_h;

	scalerInfo->uv_src_after_deci = scalerInfo->uv_factor_in;
	scalerInfo->uv_dst_after_scaler = scalerInfo->uv_factor_out;

	if ((path->out_fmt == IMG_PIX_FMT_YUV422P) ||
			(path->out_fmt == IMG_PIX_FMT_UYVY))
		scalerInfo->odata_mode = 0x00;
	else
		scalerInfo->odata_mode = 0x01;

	pr_info("deciY %d %d, Yfactor (%d %d) => (%d %d) ytrim (%d %d %d %d)\n",
		scalerInfo->y_deci.deci_x, scalerInfo->y_deci.deci_y,
		scalerInfo->y_factor_in.w, scalerInfo->y_factor_in.h,
		scalerInfo->y_factor_out.w, scalerInfo->y_factor_out.h,
		scalerInfo->y_trim.start_x, scalerInfo->y_trim.start_y,
		scalerInfo->y_trim.size_x, scalerInfo->y_trim.size_y);
	pr_info("deciU %d %d, Ufactor (%d %d) => (%d %d), Utrim (%d %d %d %d)\n",
		scalerInfo->uv_deci.deci_x, scalerInfo->uv_deci.deci_y,
		scalerInfo->uv_factor_in.w, scalerInfo->uv_factor_in.h,
		scalerInfo->uv_factor_out.w, scalerInfo->uv_factor_out.h,
		scalerInfo->uv_trim.start_x, scalerInfo->uv_trim.start_y,
		scalerInfo->uv_trim.size_x, scalerInfo->uv_trim.size_y);

	pr_info("my frameY: %d %d %d %d\n",
		scalerInfo->y_src_after_deci.w, scalerInfo->y_src_after_deci.h,
		scalerInfo->y_dst_after_scaler.w,
			scalerInfo->y_dst_after_scaler.h);
	pr_info("my frameU: %d %d %d %d\n",
		scalerInfo->uv_src_after_deci.w,
		scalerInfo->uv_src_after_deci.h,
		scalerInfo->uv_dst_after_scaler.w,
		scalerInfo->uv_dst_after_scaler.h);

	return ret;
}


int isp_cfg_ctx_base(struct isp_pipe_context *pctx, void *param)
{
	int ret = 0;
	struct isp_ctx_base_desc *cfg_in;

	if (!pctx || !param) {
		pr_err("error input ptr: null\n");
		return -EFAULT;
	}

	cfg_in = (struct isp_ctx_base_desc *)param;
	pctx->mode_3dnr = cfg_in->mode_3dnr;
	pctx->mode_ltm = cfg_in->mode_ltm;
	pctx->in_fmt = cfg_in->in_fmt;
	pctx->fetch_path_sel = cfg_in->fetch_fbd;
	pctx->dispatch_bayer_mode = cfg_in->bayer_pattern;
	pctx->dev->ltm_handle->ops->set_status(1, pctx->ctx_id,
					       pctx->mode_ltm);
	if (pctx->in_fmt == IMG_PIX_FMT_GREY)
		pctx->dispatch_color = 0; /* raw rgb */
	else
		pctx->dispatch_color = 2; /* yuv */

	pr_info("ctx%d, in_fmt 0x%x, %d %d %d %d %d\n",
		pctx->ctx_id, pctx->in_fmt, pctx->mode_3dnr,
		pctx->mode_ltm, pctx->fetch_path_sel,
		pctx->dispatch_bayer_mode, pctx->dispatch_color);
	return ret;
}

int isp_cfg_ctx_size(struct isp_pipe_context *pctx, void *param)
{
	int ret = 0;
	int invalid = 0;
	unsigned long trim_offset[3] = { 0 };
	struct isp_ctx_size_desc *cfg_in;
	struct img_size *src;
	struct img_trim *intrim;
	struct isp_fetch_info *fetch = &pctx->fetch;

	if (!pctx || !param) {
		pr_err("error input ptr: null\n");
		return -EFAULT;
	}

	cfg_in = (struct isp_ctx_size_desc *)param;
	src = &cfg_in->src;
	intrim = &cfg_in->crop;
	if (((intrim->start_x + intrim->size_x) > src->w) ||
		((intrim->start_y + intrim->size_y) > src->h))
		invalid |= 1;
	invalid |= ((intrim->start_x & 1) | (intrim->start_y & 1));
	if (invalid) {
		pr_err("invalid ctx size. src %d %d, crop %d %d %d %d\n",
			src->w, src->h, intrim->start_x, intrim->size_y,
			intrim->size_x, intrim->size_y);
		return -EINVAL;
	}

	pctx->input_size = cfg_in->src;
	pctx->input_trim = cfg_in->crop;
	intrim = &pctx->input_trim;
	fetch->size.w = intrim->size_x;
	fetch->size.h = intrim->size_y;
	fetch->fetch_fmt = get_fetch_format(pctx->in_fmt);
	pr_info("ctx%d fetch fmt: %d  in %d %d, crop %d %d %d %d\n",
		pctx->ctx_id, fetch->fetch_fmt, src->w, src->h,
		intrim->start_x, intrim->start_y, intrim->size_x,
			intrim->size_y);

	switch (fetch->fetch_fmt) {
	case ISP_FETCH_YUV422_3FRAME:
		fetch->pitch.pitch_ch0 = pctx->input_size.w;
		fetch->pitch.pitch_ch1 = pctx->input_size.w / 2;
		fetch->pitch.pitch_ch2 = pctx->input_size.w / 2;
		trim_offset[0] = intrim->start_y *
			fetch->pitch.pitch_ch0 + intrim->start_x;
		trim_offset[1] = intrim->start_y *
			fetch->pitch.pitch_ch1 + intrim->start_x / 2;
		trim_offset[2] = intrim->start_y *
			fetch->pitch.pitch_ch2 + intrim->start_x / 2;
		break;

	case ISP_FETCH_YUYV:
	case ISP_FETCH_UYVY:
	case ISP_FETCH_YVYU:
	case ISP_FETCH_VYUY:
	case ISP_FETCH_RAW10:
		fetch->pitch.pitch_ch0 = pctx->input_size.w * 2;
		trim_offset[0] = intrim->start_y *
			fetch->pitch.pitch_ch0 + intrim->start_x * 2;
		break;

	case ISP_FETCH_YUV422_2FRAME:
	case ISP_FETCH_YVU422_2FRAME:
		fetch->pitch.pitch_ch0 = pctx->input_size.w;
		fetch->pitch.pitch_ch1 = pctx->input_size.w;
		trim_offset[0] = intrim->start_y *
			fetch->pitch.pitch_ch0 + intrim->start_x;
		trim_offset[1] = intrim->start_y *
			fetch->pitch.pitch_ch1 + intrim->start_x;
		break;

	case ISP_FETCH_YUV420_2FRAME:
	case ISP_FETCH_YVU420_2FRAME:
		fetch->pitch.pitch_ch0 = pctx->input_size.w;
		fetch->pitch.pitch_ch1 = pctx->input_size.w;
		trim_offset[0] = intrim->start_y *
			fetch->pitch.pitch_ch0 / 2 + intrim->start_x;
		trim_offset[1] = intrim->start_y *
			fetch->pitch.pitch_ch1 / 2 + intrim->start_x;
		break;

	case ISP_FETCH_FULL_RGB10:
		fetch->pitch.pitch_ch0 = pctx->input_size.w * 3;
		trim_offset[0] = intrim->start_y *
			fetch->pitch.pitch_ch0 + intrim->start_x * 3;
		break;

	case ISP_FETCH_CSI2_RAW10:
	{
		uint32_t mipi_byte_info = 0;
		uint32_t mipi_word_info = 0;
		uint32_t start_col = intrim->start_x;
		uint32_t end_col =  intrim->start_x + intrim->size_x - 1;
		uint32_t mipi_word_num_start[16] = {
			0, 1, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5};
		uint32_t mipi_word_num_end[16] = {
			0, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5};
		uint32_t mod16_pixel = pctx->input_size.w & 0xF;
		uint32_t mod16_bytes = (mod16_pixel + 3) / 4 * 5;
		uint32_t mod16_words = (mod16_bytes + 3) / 4;

		mipi_byte_info = start_col & 0xF;
		mipi_word_info =
			((end_col + 1) >> 4) * 5
			+ mipi_word_num_end[(end_col + 1) & 0xF]
			- ((start_col + 1) >> 4) * 5
			- mipi_word_num_start[(start_col + 1) & 0xF] + 1;
		fetch->mipi_byte_rel_pos = mipi_byte_info;
		fetch->mipi_word_num = mipi_word_info;
		fetch->pitch.pitch_ch0 =
			pctx->input_size.w / 16 * 20 + mod16_words * 4;
		trim_offset[0] = intrim->start_y *
			fetch->pitch.pitch_ch0 / 2 + intrim->start_x / 16 * 20;
		pr_info("fetch %d, %d, %d,  pitch:  %d\n",
				mod16_pixel, mod16_bytes, mod16_words,
				fetch->pitch.pitch_ch0);
		break;
	}
	default:
		pr_err("error fetch format: %d\n", fetch->fetch_fmt);
		break;
	}

	fetch->trim_off.addr_ch0 = trim_offset[0];
	fetch->trim_off.addr_ch1 = trim_offset[1];
	fetch->trim_off.addr_ch2 = trim_offset[2];

	return ret;
}

int isp_cfg_path_base(struct isp_path_desc *path, void *param)
{
	int ret = 0;
	struct isp_path_base_desc *cfg_in;
	struct isp_store_info *store = &path->store;

	if (!path || !param) {
		pr_err("error input ptr: null\n");
		return -EFAULT;
	}
	cfg_in = (struct isp_path_base_desc *)param;
	path->skip_pipeline = 0;
	path->frm_deci = 0;
	path->out_fmt = cfg_in->out_fmt;
	path->data_endian = cfg_in->endian;
	path->bind_type = cfg_in->slave_type;
	if (path->bind_type == ISP_PATH_MASTER)
		path->slave_path_id = cfg_in->slave_path_id;

	/* dst size should be configured once
	 * and only once.
	 * It should not be changed after stream_on.
	 * So, cfg it as base.
	 */
	path->dst = cfg_in->output_size;

	/* CFG output format */
	store->color_fmt = get_store_format(path->out_fmt);
	if (store->color_fmt == ISP_STORE_UYVY)
		path->uv_sync_v = 1;
	else
		path->uv_sync_v = 0;

	store->bypass = 0;
	store->endian = path->data_endian.uv_endian;
	store->speed_2x = 1;
	store->mirror_en = 0;
	store->max_len_sel = 0;
	store->shadow_clr_sel = 1;
	store->shadow_clr = 1;
	store->store_res = 1;
	store->rd_ctrl = 0;
	pr_info("path %d, fmt 0x%x, store %d, dst_size %d %d\n", path->spath_id,
		path->out_fmt, store->color_fmt, path->dst.w, path->dst.h);
	return ret;
}

int isp_cfg_path_size(struct isp_path_desc *path, void *param)
{
	int ret = 0;
	struct img_size src;
	struct img_trim *crop;
	struct isp_pipe_context *pctx = path->attach_ctx;
	struct isp_store_info *store = &path->store;

	if (!path || !param) {
		pr_err("error input ptr: null\n");
		return -EFAULT;
	}
	crop = (struct img_trim *)param;
	src.w = pctx->input_trim.size_x;
	src.h = pctx->input_trim.size_y;
	if (((crop->start_x + crop->size_x) > src.w) ||
		((crop->start_y + crop->size_y) > src.h)) {
		pr_err("error: path trim(%d %d %d %d) outside of src (%d %d)\n",
			crop->start_x, crop->start_y,
			crop->size_x, crop->size_y, src.w, src.h);
		return -EINVAL;
	}

	/* no trim in isp scaler. */
	path->src = src;
	path->in_trim = *crop;
	path->out_trim.start_x = 0;
	path->out_trim.start_y = 0;
	path->out_trim.size_x = path->dst.w;
	path->out_trim.size_y = path->dst.h;

	pr_info("path %d. src %d %d, trim %d %d %d %d. dst %d %d\n",
		path->spath_id, path->src.w, path->src.h,
		path->in_trim.start_x, path->in_trim.start_y,
		path->in_trim.size_x, path->in_trim.size_y,
		path->dst.w, path->dst.h);

	if (path->spath_id == ISP_SPATH_FD)
		ret = cfg_path_thumbscaler(path);
	else
		ret = cfg_path_scaler(path);

	store->size.w = path->dst.w;
	store->size.h = path->dst.h;
	switch (store->color_fmt) {
	case ISP_STORE_UYVY:
		store->pitch.pitch_ch0 = store->size.w * 2;
		store->total_size = store->size.w * store->size.h * 2;
		break;

	case ISP_STORE_YUV422_2FRAME:
	case ISP_STORE_YVU422_2FRAME:
	case ISP_STORE_YUV420_2FRAME:
	case ISP_STORE_YVU420_2FRAME:
		store->pitch.pitch_ch0 = store->size.w;
		store->pitch.pitch_ch1 = store->size.w;
		store->total_size = store->size.w * store->size.h * 2;
		break;

	case ISP_STORE_YUV422_3FRAME:
	case ISP_STORE_YUV420_3FRAME:
		store->pitch.pitch_ch0 = store->size.w;
		store->pitch.pitch_ch1 = store->size.w / 2;
		store->pitch.pitch_ch2 = store->size.w / 2;
		store->total_size = store->size.w * store->size.h * 3 / 2;
		break;
	default:
		pr_err("unsupported store fmt: %d\n", store->color_fmt);
		store->pitch.pitch_ch0 = 0;
		store->pitch.pitch_ch1 = 0;
		store->pitch.pitch_ch2 = 0;
		break;
	}
	return ret;
}

static int set_path_common(struct isp_path_desc *path)
{
	uint32_t idx = path->attach_ctx->ctx_id;
	struct img_deci_info *deciInfo = &path->deci;
	unsigned long addr;
	uint32_t path_mask[ISP_SPATH_NUM] = {
		BIT_1 | BIT_0,
		BIT_3 | BIT_2,
		BIT_5 | BIT_4
	};
	uint32_t path_off[ISP_SPATH_NUM] = {0, 2, 4};

	addr = scaler_base[path->spath_id];

	ISP_REG_MWR(idx, ISP_COMMON_SCL_PATH_SEL,
		path_mask[path->spath_id],
		(path->skip_pipeline << path_off[path->spath_id]));

	/* set path_eb*/
	ISP_REG_MWR(idx, addr + ISP_SCALER_CFG,
		BIT_31, 1 << 31); /* path enable */
	ISP_REG_MWR(idx, addr + ISP_SCALER_CFG,
		BIT_30, 0 << 30); /* CLK_SWITCH*/
	ISP_REG_MWR(idx, addr + ISP_SCALER_CFG,
		BIT_29, 0 << 29); /* sw_switch_en*/
	ISP_REG_MWR(idx, addr + ISP_SCALER_CFG,
		BIT_9, 0 << 9); /* bypass all scaler */
	ISP_REG_MWR(idx, addr + ISP_SCALER_CFG,
		BIT_8, 0 << 8); /* scaler path stop */
	ISP_REG_MWR(idx, addr + ISP_SCALER_CFG,
		BIT_10, path->uv_sync_v << 10);

	ISP_REG_MWR(idx, addr + ISP_SCALER_CFG, (BIT_23 | BIT_24),
			(path->frm_deci & 3) << 23);

	ISP_REG_MWR(idx, addr + ISP_SCALER_CFG, BIT_6 | BIT_7,
			path->scaler.odata_mode << 6);

	/*set X/Y deci */
	ISP_REG_MWR(idx, addr + ISP_SCALER_CFG, BIT_2,
		deciInfo->deci_x_eb << 2);
	ISP_REG_MWR(idx, addr + ISP_SCALER_CFG, (BIT_0 | BIT_1),
		deciInfo->deci_x);
	ISP_REG_MWR(idx, addr + ISP_SCALER_CFG, BIT_5,
		deciInfo->deci_y_eb << 5);
	ISP_REG_MWR(idx, addr + ISP_SCALER_CFG, (BIT_3 | BIT_4),
		deciInfo->deci_y << 3);

	/*src size*/
	ISP_REG_WR(idx, addr + ISP_SCALER_SRC_SIZE,
				((path->src.h & 0x3FFF) << 16) |
					(path->src.w & 0x3FFF));

	/* trim0 */
	ISP_REG_WR(idx, addr + ISP_SCALER_TRIM0_START,
				((path->in_trim.start_y & 0x3FFF) << 16) |
					(path->in_trim.start_x & 0x3FFF));
	ISP_REG_WR(idx, addr + ISP_SCALER_TRIM0_SIZE,
				((path->in_trim.size_y & 0x3FFF) << 16) |
					(path->in_trim.size_x & 0x3FFF));

	/* trim1 */
	ISP_REG_WR(idx, addr + ISP_SCALER_TRIM1_START,
				((path->out_trim.start_y & 0x3FFF) << 16) |
					(path->out_trim.start_x & 0x3FFF));
	ISP_REG_WR(idx, addr + ISP_SCALER_TRIM1_SIZE,
				((path->out_trim.size_y & 0x3FFF) << 16) |
					(path->out_trim.size_x & 0x3FFF));

	/* des size */
	ISP_REG_WR(idx, addr + ISP_SCALER_DES_SIZE,
				((path->dst.h & 0x3FFF) << 16) |
					(path->dst.w & 0x3FFF));

	return 0;
}


static void set_path_shrink_info(
			uint32_t idx, unsigned long  scaler_base,
			struct isp_regular_info *regular_info)
{
	unsigned long addr = 0;
	uint32_t reg_val = 0;

	pr_debug("regular_mode %d\n", regular_info->regular_mode);
	addr = ISP_SCALER_CFG + scaler_base;
	ISP_REG_MWR(idx, addr, (BIT_25 | BIT_26),
		regular_info->regular_mode << 25);

	/*TBD
	 * the value need to update.
	 */
	if (regular_info->regular_mode == DCAM_REGULAR_SHRINK) {
		regular_info->shrink_y_up_th = SHRINK_Y_UP_TH;
		regular_info->shrink_y_dn_th = SHRINK_Y_DN_TH;
		regular_info->shrink_uv_up_th = SHRINK_UV_UP_TH;
		regular_info->shrink_uv_dn_th = SHRINK_UV_DN_TH;
		addr = ISP_SCALER_SHRINK_CFG + scaler_base;
		reg_val = ((regular_info->shrink_uv_dn_th & 0xFF) << 24) |
			((regular_info->shrink_uv_up_th & 0xFF) << 16);
		reg_val |= ((regular_info->shrink_y_dn_th  & 0xFF) << 8) |
			((regular_info->shrink_y_up_th & 0xFF));
		ISP_REG_WR(idx, addr, reg_val);

		regular_info->shrink_y_offset = SHRINK_Y_OFFSET;
		regular_info->shrink_y_range = SHRINK_Y_RANGE;
		regular_info->shrink_c_offset = SHRINK_C_OFFSET;
		regular_info->shrink_c_range = SHRINK_C_RANGE;
		addr = ISP_SCALER_REGULAR_CFG + scaler_base;
		reg_val = ((regular_info->shrink_c_range & 0xF) << 24) |
			((regular_info->shrink_c_offset & 0x1F) << 16);
		reg_val |= ((regular_info->shrink_y_range & 0xF) << 8) |
			(regular_info->shrink_y_offset & 0x1F);
		ISP_REG_WR(idx, addr, reg_val);
	} else if (regular_info->regular_mode == DCAM_REGULAR_CUT) {
		addr = ISP_SCALER_SHRINK_CFG + scaler_base;
		reg_val = ((regular_info->shrink_uv_dn_th & 0xFF) << 24) |
			((regular_info->shrink_uv_up_th & 0xFF) << 16);
		reg_val |= ((regular_info->shrink_y_dn_th  & 0xFF) << 8) |
			((regular_info->shrink_y_up_th & 0xFF));
		ISP_REG_WR(idx, addr, reg_val);
	} else if (regular_info->regular_mode == DCAM_REGULAR_EFFECT) {
		addr = ISP_SCALER_EFFECT_CFG + scaler_base;
		reg_val = ((regular_info->effect_v_th & 0xFF) << 16) |
				((regular_info->effect_u_th & 0xFF) << 8);
		reg_val |= (regular_info->effect_y_th & 0xFF);
		ISP_REG_WR(idx, addr, reg_val);
	} else
		pr_debug("regular_mode %d\n", regular_info->regular_mode);
}

static int set_path_scaler_coeff(
			uint32_t idx, unsigned long  scaler_base,
			uint32_t *coeff_buf,
			struct isp_path_desc *path)
{
	int i = 0, rtn = 0;
	uint32_t h_coeff_addr = 0;
	uint32_t v_coeff_addr = 0;
	uint32_t h_chroma_coeff_addr = 0;
	uint32_t v_chroma_coeff_addr = 0;
	uint32_t *h_coeff = NULL;
	uint32_t *v_coeff = NULL;
	uint32_t *v_chroma_coeff = NULL;
	uint32_t buf_sel;

	h_coeff = coeff_buf;
	v_coeff = coeff_buf + (ISP_SC_COEFF_COEF_SIZE / 4);
	v_chroma_coeff = v_coeff + (ISP_SC_COEFF_COEF_SIZE / 4);

	/* ping pong buffer. */
	buf_sel = ISP_REG_RD(idx, scaler_base + ISP_SCALER_CFG);
	buf_sel = (~((buf_sel & BIT_30) >> 30)) & 1;

	/* temp set: config mode always select buf 0 */
	buf_sel = 0;

	h_coeff_addr = coff_buf_addr[buf_sel][path->spath_id][0];
	h_chroma_coeff_addr = coff_buf_addr[buf_sel][path->spath_id][1];
	v_coeff_addr = coff_buf_addr[buf_sel][path->spath_id][2];
	v_chroma_coeff_addr = coff_buf_addr[buf_sel][path->spath_id][3];

	for (i = 0; i < ISP_SC_COEFF_H_NUM; i++) {
		ISP_REG_WR(idx, h_coeff_addr, *h_coeff);
		h_coeff_addr += 4;
		h_coeff++;
	}

	for (i = 0; i < ISP_SC_COEFF_H_CHROMA_NUM; i++) {
		ISP_REG_WR(idx, h_chroma_coeff_addr, *h_coeff);
		h_chroma_coeff_addr += 4;
		h_coeff++;
	}

	for (i = 0; i < ISP_SC_COEFF_V_NUM; i++) {
		ISP_REG_WR(idx, v_coeff_addr, *v_coeff);
		v_coeff_addr += 4;
		v_coeff++;
	}

	for (i = 0; i < ISP_SC_COEFF_V_CHROMA_NUM; i++) {
		ISP_REG_WR(idx, v_chroma_coeff_addr, *v_chroma_coeff);
		v_chroma_coeff_addr += 4;
		v_chroma_coeff++;
	}

	ISP_REG_MWR(idx, scaler_base + ISP_SCALER_CFG,
			BIT_30, buf_sel << 30);

	pr_debug("end. buf_sel %d\n", buf_sel);
	return rtn;
}

static int set_path_scaler(struct isp_path_desc *path)
{
	uint32_t reg_val, idx;
	struct isp_scaler_info *scalerInfo = NULL;
	unsigned long addr_base;

	scalerInfo = &path->scaler;
	addr_base = scaler_base[path->spath_id];
	idx = path->attach_ctx->ctx_id;

	ISP_REG_MWR(idx, addr_base + ISP_SCALER_CFG, BIT_20,
			scalerInfo->scaler_bypass << 20);
	ISP_REG_MWR(idx, addr_base + ISP_SCALER_CFG, 0xF0000,
			scalerInfo->scaler_y_ver_tap << 16);
	ISP_REG_MWR(idx, addr_base + ISP_SCALER_CFG, 0xF800,
			scalerInfo->scaler_uv_ver_tap << 11);

	reg_val = ((scalerInfo->scaler_ip_int & 0xF) << 16) |
			(scalerInfo->scaler_ip_rmd & 0x3FFF);
	ISP_REG_WR(idx, addr_base + ISP_SCALER_IP, reg_val);
	reg_val = ((scalerInfo->scaler_cip_int & 0xF) << 16) |
			(scalerInfo->scaler_cip_rmd & 0x3FFF);
	ISP_REG_WR(idx, addr_base + ISP_SCALER_CIP, reg_val);
	reg_val = ((scalerInfo->scaler_factor_in & 0x3FFF) << 16) |
			(scalerInfo->scaler_factor_out & 0x3FFF);
	ISP_REG_WR(idx, addr_base + ISP_SCALER_FACTOR, reg_val);

	reg_val = ((scalerInfo->scaler_ver_ip_int & 0xF) << 16) |
				(scalerInfo->scaler_ver_ip_rmd & 0x3FFF);
	ISP_REG_WR(idx, addr_base + ISP_SCALER_VER_IP, reg_val);
	reg_val = ((scalerInfo->scaler_ver_cip_int & 0xF) << 16) |
				(scalerInfo->scaler_ver_cip_rmd & 0x3FFF);
	ISP_REG_WR(idx, addr_base + ISP_SCALER_VER_CIP, reg_val);
	reg_val = ((scalerInfo->scaler_ver_factor_in & 0x3FFF) << 16) |
				(scalerInfo->scaler_ver_factor_out & 0x3FFF);
	ISP_REG_WR(idx, addr_base + ISP_SCALER_VER_FACTOR, reg_val);

	pr_debug("set_scale_info in %d %d out %d %d\n",
		scalerInfo->scaler_factor_in,
		scalerInfo->scaler_ver_factor_in,
		scalerInfo->scaler_factor_out,
		scalerInfo->scaler_ver_factor_out);

	if (!scalerInfo->scaler_bypass)
		set_path_scaler_coeff(idx,
			addr_base, scalerInfo->coeff_buf, path);

	if (path->spath_id == ISP_SPATH_VID)
		set_path_shrink_info(idx, addr_base, &path->regular_info);

	return 0;
}

static uint32_t cal_deci_par(uint32_t deci)
{
	/* 0: 1/2, 1: 1/4, 2: 1/8, 3: 1/16*/
	if (deci == 16)
		return 3;
	else if (deci == 8)
		return 2;
	else if (deci == 4)
		return 1;
	else
		return 0;
}

static int set_path_thumbscaler(struct isp_path_desc *path)
{
	uint32_t val, idx;
	uint32_t y_deci_w_par, y_deci_h_par;
	uint32_t uv_deci_w_par, uv_deci_h_par;
	struct isp_thumbscaler_info *scalerInfo = NULL;

	scalerInfo = &path->thumbscaler;
	idx = path->attach_ctx->ctx_id;

	ISP_REG_MWR(idx, ISP_COMMON_SCL_PATH_SEL,
		BIT_5 | BIT_4, (path->skip_pipeline << 4));

	ISP_REG_MWR(idx, ISP_THMB_CFG, BIT_0, scalerInfo->scaler_bypass & 0x1);

	y_deci_w_par = cal_deci_par(scalerInfo->y_deci.deci_x);
	y_deci_h_par = cal_deci_par(scalerInfo->y_deci.deci_y);
	uv_deci_w_par = cal_deci_par(scalerInfo->uv_deci.deci_x);
	uv_deci_h_par = cal_deci_par(scalerInfo->uv_deci.deci_y);
	val = ((path->frm_deci & 0x3) << 2) |
		((scalerInfo->odata_mode & 0x3) << 4) |
		((y_deci_w_par & 0x3) << 16) |
		((scalerInfo->y_deci.deci_x_eb & 0x1) << 19) |
		((y_deci_h_par & 0x3) << 20) |
		((scalerInfo->y_deci.deci_y_eb & 0x1) << 23) |
		((uv_deci_w_par & 0x3) << 24) |
		((scalerInfo->uv_deci.deci_x_eb & 0x1) << 27) |
		((uv_deci_h_par & 0x3) << 28) |
		((scalerInfo->uv_deci.deci_y_eb & 0x1) << 31);
	ISP_REG_MWR(idx, ISP_THMB_CFG, 0xBBBB003C, val);

	val = ((scalerInfo->y_factor_in.w & 0x1FFF) << 16) |
		(scalerInfo->y_factor_out.w & 0x3FF);
	ISP_REG_WR(idx, ISP_THMB_Y_FACTOR_HOR, val);

	val = ((scalerInfo->y_factor_in.h & 0x1FFF) << 16) |
		(scalerInfo->y_factor_out.h & 0x3FF);
	ISP_REG_WR(idx, ISP_THMB_Y_FACTOR_VER, val);

	val = ((scalerInfo->uv_factor_in.w & 0x1FFF) << 16) |
		(scalerInfo->uv_factor_out.w & 0x3FF);
	ISP_REG_WR(idx, ISP_THMB_UV_FACTOR_HOR, val);

	val = ((scalerInfo->uv_factor_in.h & 0x1FFF) << 16) |
		(scalerInfo->uv_factor_out.h & 0x3FF);
	ISP_REG_WR(idx, ISP_THMB_UV_FACTOR_VER, val);

	val = ((scalerInfo->src0.h & 0x1FFF) << 16) |
		(scalerInfo->src0.w & 0x1FFF);
	ISP_REG_WR(idx, ISP_THMB_BEFORE_TRIM_SIZE, val);

	val = ((scalerInfo->y_src_after_deci.h & 0x1FFF) << 16) |
		(scalerInfo->y_src_after_deci.w & 0x1FFF);
	ISP_REG_WR(idx, ISP_THMB_Y_SLICE_SRC_SIZE, val);

	val = ((scalerInfo->y_dst_after_scaler.h & 0x3FF) << 16) |
		(scalerInfo->y_dst_after_scaler.w & 0x3FF);
	ISP_REG_WR(idx, ISP_THMB_Y_DES_SIZE, val);

	val = ((scalerInfo->y_trim.start_y & 0x1FFF) << 16) |
		(scalerInfo->y_trim.start_x & 0x1FFF);
	ISP_REG_WR(idx, ISP_THMB_Y_TRIM0_START, val);

	val = ((scalerInfo->y_trim.size_y & 0x1FFF) << 16) |
		(scalerInfo->y_trim.size_x & 0x1FFF);
	ISP_REG_WR(idx, ISP_THMB_Y_TRIM0_SIZE, val);

	val = ((scalerInfo->y_init_phase.h & 0x3FF) << 16) |
		(scalerInfo->y_init_phase.w & 0x3FF);
	ISP_REG_WR(idx, ISP_THMB_Y_INIT_PHASE, val);

	val = ((scalerInfo->uv_src_after_deci.h & 0x1FFF) << 16) |
		(scalerInfo->uv_src_after_deci.w & 0x1FFF);
	ISP_REG_WR(idx, ISP_THMB_UV_SLICE_SRC_SIZE, val);

	val = ((scalerInfo->uv_dst_after_scaler.h & 0x3FF) << 16) |
		(scalerInfo->uv_dst_after_scaler.w & 0x3FF);
	ISP_REG_WR(idx, ISP_THMB_UV_DES_SIZE, val);

	val = ((scalerInfo->uv_trim.start_y & 0x1FFF) << 16) |
		(scalerInfo->uv_trim.start_x & 0x1FFF);
	ISP_REG_WR(idx, ISP_THMB_UV_TRIM0_START, val);

	val = ((scalerInfo->uv_trim.size_y & 0x1FFF) << 16) |
		(scalerInfo->uv_trim.size_x & 0x1FFF);
	ISP_REG_WR(idx, ISP_THMB_UV_TRIM0_SIZE, val);

	val = ((scalerInfo->uv_init_phase.h & 0x3FF) << 16) |
		(scalerInfo->uv_init_phase.w & 0x3FF);
	ISP_REG_WR(idx, ISP_THMB_UV_INIT_PHASE, val);

	/* bypass regular. */
	ISP_REG_WR(idx, ISP_THMB_EFFECT_CFG, 0);

	return 0;
}

static int set_path_store(struct isp_path_desc *path)
{
	int ret = 0;
	uint32_t val = 0;
	uint32_t idx = path->attach_ctx->ctx_id;
	struct isp_store_info *store_info = &path->store;
	unsigned long addr = store_base[path->spath_id];

	pr_info("isp set store in.  bypass %d, path_id:%d, w:%d,h:%d\n",
			store_info->bypass, path->spath_id,
			store_info->size.w, store_info->size.h);

	ISP_REG_MWR(idx, addr + ISP_STORE_PARAM,
		BIT_0, store_info->bypass);
	if (store_info->bypass)
		return 0;

	ISP_REG_MWR(idx, addr + ISP_STORE_PARAM,
		BIT_1, (store_info->max_len_sel << 1));

	ISP_REG_MWR(idx, addr + ISP_STORE_PARAM,
		BIT_2, (store_info->speed_2x << 2));

	ISP_REG_MWR(idx, addr + ISP_STORE_PARAM,
		BIT_3, (store_info->mirror_en << 3));

	ISP_REG_MWR(idx, addr + ISP_STORE_PARAM,
		0xF0, (store_info->color_fmt << 4));

	ISP_REG_MWR(idx, addr + ISP_STORE_PARAM,
		0x300, (store_info->endian << 8));

	val = ((store_info->size.h & 0xFFFF) << 16) |
		(store_info->size.w & 0xFFFF);
	ISP_REG_WR(idx, addr + ISP_STORE_SLICE_SIZE, val);

	ISP_REG_WR(idx, addr + ISP_STORE_BORDER, 0);
	ISP_REG_WR(idx, addr + ISP_STORE_Y_PITCH, store_info->pitch.pitch_ch0);
	ISP_REG_WR(idx, addr + ISP_STORE_U_PITCH, store_info->pitch.pitch_ch1);
	ISP_REG_WR(idx, addr + ISP_STORE_V_PITCH, store_info->pitch.pitch_ch2);

	pr_debug("set_store size %d %d\n",
		store_info->size.w, store_info->size.h);

	ISP_REG_MWR(idx, addr + ISP_STORE_READ_CTRL,
		0x3, store_info->rd_ctrl);
	ISP_REG_MWR(idx, addr + ISP_STORE_READ_CTRL,
		0xFFFFFFFC, store_info->store_res << 2);

	ISP_REG_MWR(idx, addr + ISP_STORE_SHADOW_CLR_SEL,
		BIT_1, store_info->shadow_clr_sel << 1);
	ISP_REG_MWR(idx, addr + ISP_STORE_SHADOW_CLR,
		BIT_0, store_info->shadow_clr);

	return ret;
}


/* config path common register */
int isp_set_path(struct isp_path_desc *path)
{
	int ret = 0;

	if (!path) {
		pr_err("error input ptr: null\n");
		return -EINVAL;
	}
	pr_info("enter.\n");
	if (path->spath_id == ISP_SPATH_FD) {
		set_path_thumbscaler(path);
	} else {
		set_path_common(path);
		set_path_scaler(path);
	}
	set_path_store(path);
	pr_info("done.\n");
	return ret;
}


int isp_path_set_store_frm(
		struct isp_path_desc *path,
		struct camera_frame *frame)
{
	int ret = 0;
	int idx;
	int planes;
	unsigned long offset_u, offset_v, yuv_addr[3] = {0};
	struct isp_pipe_context *pctx;
	struct isp_store_info *store;
	unsigned long addr;

	if (!path || !frame) {
		pr_err("error input ptr: null\n");
		return -EINVAL;
	}
	pr_debug("enter.\n");
	pctx = path->attach_ctx;
	store = &path->store;
	idx = pctx->ctx_id;
	addr = store_base[path->spath_id];

	if (store->color_fmt == ISP_STORE_UYVY)
		planes = 1;
	else if ((store->color_fmt == ISP_STORE_YUV422_3FRAME)
			|| (store->color_fmt == ISP_STORE_YUV420_3FRAME))
		planes = 3;
	else
		planes = 2;

	yuv_addr[0] = frame->buf.iova[0];
	yuv_addr[1] = frame->buf.iova[1];
	yuv_addr[2] = frame->buf.iova[2];

	pr_debug("planes %d addr %lx %lx %lx\n", planes,
			yuv_addr[0], yuv_addr[1], yuv_addr[2]);

	if ((planes > 1) && yuv_addr[1] == 0) {
		offset_u = store->pitch.pitch_ch0 * store->size.h;
		yuv_addr[1] = yuv_addr[0] + offset_u;
	}

	if ((planes > 2) && yuv_addr[2] == 0) {
		offset_v = store->pitch.pitch_ch1 * store->size.h;
		if (path->store.color_fmt == ISP_STORE_YUV420_3FRAME)
			offset_v >>= 1;
		yuv_addr[2] = yuv_addr[1] + offset_v;
	}

	pr_debug("path %d planes %d addr %lx %lx %lx\n",
		path->spath_id, planes,
		yuv_addr[0], yuv_addr[1], yuv_addr[2]);

	ISP_REG_WR(idx, addr + ISP_STORE_SLICE_Y_ADDR, yuv_addr[0]);
	ISP_REG_WR(idx, addr + ISP_STORE_SLICE_U_ADDR, yuv_addr[1]);
	ISP_REG_WR(idx, addr + ISP_STORE_SLICE_V_ADDR, yuv_addr[2]);

	path->store.addr.addr_ch0 = yuv_addr[0];
	path->store.addr.addr_ch1 = yuv_addr[1];
	path->store.addr.addr_ch2 = yuv_addr[2];
	pr_debug("done %x %x %x\n", path->store.addr.addr_ch0,
		path->store.addr.addr_ch1,
		path->store.addr.addr_ch2);

	return ret;
}


int isp_path_set_fetch_frm(
		struct isp_pipe_context *pctx,
		struct camera_frame *frame,
		struct img_addr *fetch_addr)
{
	int ret = 0;
	int idx;
	int planes;
	unsigned long offset_u, offset_v, yuv_addr[3] = {0};
	struct isp_fetch_info *fetch = &pctx->fetch;

	if (!pctx || !frame || !fetch_addr) {
		pr_err("error input ptr: null\n");
		return -EINVAL;
	}
	pr_debug("enter.\n");

	if (fetch->fetch_fmt == ISP_FETCH_YUV422_3FRAME)
		planes = 3;
	else if ((fetch->fetch_fmt == ISP_FETCH_YUV422_2FRAME)
			|| (fetch->fetch_fmt == ISP_FETCH_YVU422_2FRAME)
			|| (fetch->fetch_fmt == ISP_FETCH_YUV420_2FRAME)
			|| (fetch->fetch_fmt == ISP_FETCH_YVU420_2FRAME))
		planes = 2;
	else
		planes = 1;

	yuv_addr[0] = frame->buf.iova[0];
	yuv_addr[1] = frame->buf.iova[1];
	yuv_addr[2] = frame->buf.iova[2];

	if ((planes > 1) && yuv_addr[1] == 0) {
		offset_u = fetch->pitch.pitch_ch0 * fetch->size.h;
		yuv_addr[1] = yuv_addr[0] + offset_u;
	}

	if ((planes > 2) && yuv_addr[2] == 0) {
		/* ISP_FETCH_YUV422_3FRAME */
		offset_v = fetch->pitch.pitch_ch1 * fetch->size.h;
		yuv_addr[2] = yuv_addr[1] + offset_v;
	}

	yuv_addr[0] += fetch->trim_off.addr_ch0;
	yuv_addr[1] += fetch->trim_off.addr_ch1;
	yuv_addr[2] += fetch->trim_off.addr_ch2;

	idx = pctx->ctx_id;
	ISP_REG_WR(idx, ISP_FETCH_SLICE_Y_ADDR, yuv_addr[0]);
	ISP_REG_WR(idx, ISP_FETCH_SLICE_U_ADDR, yuv_addr[1]);
	ISP_REG_WR(idx, ISP_FETCH_SLICE_V_ADDR, yuv_addr[2]);

	fetch_addr->addr_ch0 = yuv_addr[0];
	fetch_addr->addr_ch1 = yuv_addr[1];
	fetch_addr->addr_ch2 = yuv_addr[2];

	pr_debug("done %x %x %x\n", fetch_addr->addr_ch0,
		fetch_addr->addr_ch1,
		fetch_addr->addr_ch2);
	return ret;
}

