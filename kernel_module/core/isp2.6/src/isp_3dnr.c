/*
 * Copyright (C) 2017 Spreadtrum Communications Inc.
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
#include <linux/uaccess.h>
#include <sprd_mm.h>
#include <sprd_isp_r8p1.h>

#include "isp_reg.h"
#include "cam_types.h"
#include "cam_block.h"

#include "isp_core.h"
#include "isp_3dnr.h"


#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt) "3DNR logic: %d %d %s : "\
	fmt, current->pid, __LINE__, __func__

static int isp_3dnr_update_memctrl_base_on_mv(struct isp_3dnr_ctx_desc *ctx)
{
	struct isp_3dnr_mem_ctrl *mem_ctrl = mem_ctrl = &ctx->mem_ctrl;

	if (ctx->mv.mv_x < 0) {
		if (ctx->mv.mv_x & 0x1) {
			mem_ctrl->ft_y_width =
				ctx->width + ctx->mv.mv_x + 1;
			mem_ctrl->ft_uv_width =
				ctx->width + ctx->mv.mv_x - 1;
			mem_ctrl->ft_luma_addr = mem_ctrl->ft_luma_addr;
			mem_ctrl->ft_chroma_addr = mem_ctrl->ft_chroma_addr + 2;
		} else {
			mem_ctrl->ft_y_width =
				ctx->width + ctx->mv.mv_x;
			mem_ctrl->ft_uv_width =
				ctx->width + ctx->mv.mv_x;
			mem_ctrl->ft_luma_addr = mem_ctrl->ft_luma_addr;
			mem_ctrl->ft_chroma_addr = mem_ctrl->ft_chroma_addr;
		}
	} else if (ctx->mv.mv_x > 0) {
		if (ctx->mv.mv_x  & 0x1) {
			mem_ctrl->ft_y_width =
				ctx->width - ctx->mv.mv_x+1;
			mem_ctrl->ft_uv_width =
				ctx->width - ctx->mv.mv_x+1;
			mem_ctrl->ft_luma_addr =
				mem_ctrl->ft_luma_addr + ctx->mv.mv_x;
			mem_ctrl->ft_chroma_addr =
				mem_ctrl->ft_chroma_addr + ctx->mv.mv_x - 1;
		} else {
			mem_ctrl->ft_y_width =
				ctx->width - ctx->mv.mv_x;
			mem_ctrl->ft_uv_width =
				ctx->width - ctx->mv.mv_x;
			mem_ctrl->ft_luma_addr =
				mem_ctrl->ft_luma_addr + ctx->mv.mv_x;
			mem_ctrl->ft_chroma_addr =
				mem_ctrl->ft_chroma_addr + ctx->mv.mv_x;
		}
	}
	if (ctx->mv.mv_y < 0) {
		if (ctx->mv.mv_y & 0x1) {
			mem_ctrl->last_line_mode = 0;
			mem_ctrl->ft_uv_height =
				ctx->height/2 + ctx->mv.mv_y/2;
		} else {
			mem_ctrl->last_line_mode = 1;
			mem_ctrl->ft_uv_height =
				ctx->height/2 + ctx->mv.mv_y/2+1;
		}
		mem_ctrl->first_line_mode = 0;
		mem_ctrl->ft_y_height =
			ctx->height + ctx->mv.mv_y;
		mem_ctrl->ft_luma_addr = mem_ctrl->ft_luma_addr;
		mem_ctrl->ft_chroma_addr = mem_ctrl->ft_chroma_addr;
	} else if (ctx->mv.mv_y > 0) {
		if ((ctx->mv.mv_y)  & 0x1) {
			/*temp modify first_line_mode =0*/
			mem_ctrl->first_line_mode = 0;
			mem_ctrl->last_line_mode = 0;
			mem_ctrl->ft_y_height =
				ctx->height - ctx->mv.mv_y;
			mem_ctrl->ft_uv_height =
				ctx->height / 2 - (ctx->mv.mv_y / 2);

			mem_ctrl->ft_luma_addr =
				mem_ctrl->ft_luma_addr
				+ mem_ctrl->ft_pitch * ctx->mv.mv_y;
			mem_ctrl->ft_chroma_addr =
				mem_ctrl->ft_chroma_addr
				+ mem_ctrl->ft_pitch * (ctx->mv.mv_y / 2);
		} else {
			mem_ctrl->ft_y_height =
				ctx->height - ctx->mv.mv_y;
			mem_ctrl->ft_uv_height =
				ctx->height / 2 - (ctx->mv.mv_y / 2);
			mem_ctrl->ft_luma_addr =
				mem_ctrl->ft_luma_addr
				+ mem_ctrl->ft_pitch * ctx->mv.mv_y;
			mem_ctrl->ft_chroma_addr =
				mem_ctrl->ft_chroma_addr
				+ mem_ctrl->ft_pitch * (ctx->mv.mv_y / 2);
		}
	}
	pr_debug("3DNR ft_luma=0x%lx,ft_chroma=0x%lx, mv_x=%d,mv_y=%d\n",
		mem_ctrl->ft_luma_addr,
		mem_ctrl->ft_chroma_addr,
		ctx->mv.mv_x,
		ctx->mv.mv_y);
	pr_debug("3DNR ft_y_h=%d, ft_uv_h=%d, ft_y_w=%d, ft_uv_w=%d\n",
		mem_ctrl->ft_y_height,
		mem_ctrl->ft_uv_height,
		mem_ctrl->ft_y_width,
		mem_ctrl->ft_uv_width);

	return 0;
}


static int isp_3dnr_gen_memctrl_config(struct isp_3dnr_ctx_desc *ctx)
{
	int ret = 0;
	struct isp_3dnr_mem_ctrl *mem_ctrl = NULL;

	if (!ctx) {
		pr_err("invalid parameter, mem ctrl\n");
		return -EINVAL;
	}

	mem_ctrl = &ctx->mem_ctrl;

	mem_ctrl->bypass = 0;

	/* configuration param0 */
	if (!ctx->blending_cnt) {
		mem_ctrl->ref_pic_flag = 0;
		/* ctx->blending_cnt = 0; */
	} else {
		pr_debug("3DNR ref_pic_flag nonzero\n");
		mem_ctrl->ref_pic_flag = 1;
	}

	mem_ctrl->ft_max_len_sel = 1;
	mem_ctrl->retain_num = 0;
	mem_ctrl->roi_mode = 0;
	mem_ctrl->data_toyuv_en = 1;
	mem_ctrl->chk_sum_clr_en = 1;
	mem_ctrl->back_toddr_en = 1;
	mem_ctrl->nr3_done_mode = 0;
	mem_ctrl->start_col = 0;
	mem_ctrl->start_row = 0;

	/* configuration param2 */
	mem_ctrl->global_img_width = ctx->width;
	mem_ctrl->global_img_height = ctx->height;

	/* configuration param3 */
	mem_ctrl->img_width = ctx->width;
	mem_ctrl->img_height = ctx->height;
	pr_debug("3DNR img_width=%d, img_height=%d\n",
		mem_ctrl->img_width,
		mem_ctrl->img_height);

	/* configuration param4/5 */
	mem_ctrl->ft_y_width = ctx->width;
	mem_ctrl->ft_y_height = ctx->height;
	mem_ctrl->ft_uv_width = ctx->width;
	mem_ctrl->ft_uv_height = ctx->height / 2;

	/* configuration ref frame pitch */
	mem_ctrl->ft_pitch = ctx->width;

	mem_ctrl->mv_x = ctx->mv.mv_x;
	mem_ctrl->mv_y = ctx->mv.mv_y;

	if (ctx->blending_cnt % 2 == 1) {
		mem_ctrl->ft_luma_addr   = ctx->buf_info[0]->iova[0];
		mem_ctrl->ft_chroma_addr = ctx->buf_info[0]->iova[0] +
						(ctx->width * ctx->height);
	} else {
		mem_ctrl->ft_luma_addr   = ctx->buf_info[1]->iova[0];
		mem_ctrl->ft_chroma_addr = ctx->buf_info[1]->iova[0] +
						(ctx->width * ctx->height);
	}

	mem_ctrl->first_line_mode = 0;
	mem_ctrl->last_line_mode = 0;

	/*configuration param 8~11*/
	mem_ctrl->blend_y_en_start_row = 0;
	mem_ctrl->blend_y_en_start_col = 0;
	mem_ctrl->blend_y_en_end_row = ctx->height - 1;
	mem_ctrl->blend_y_en_end_col = ctx->width - 1;
	mem_ctrl->blend_uv_en_start_row = 0;
	mem_ctrl->blend_uv_en_start_col = 0;
	mem_ctrl->blend_uv_en_end_row = ctx->height / 2 - 1;
	mem_ctrl->blend_uv_en_end_col = ctx->width - 1;

	/*configuration param 12*/
	mem_ctrl->ft_hblank_num = 32;
	mem_ctrl->pipe_hblank_num = 60;
	mem_ctrl->pipe_flush_line_num = 17;

	/*configuration param 13*/
	mem_ctrl->pipe_nfull_num = 100;
	mem_ctrl->ft_fifo_nfull_num = 2648;

	if (ctx->type == NR3_FUNC_PRE)
		isp_3dnr_update_memctrl_base_on_mv(ctx);


	return ret;
}

static int isp_3dnr_gen_store_config(struct isp_3dnr_ctx_desc *ctx)
{
	int ret = 0;
	struct isp_3dnr_store *store = NULL;

	if (!ctx) {
		pr_err("invalid parameter, store\n");
		return -EINVAL;
	}

	store = &ctx->nr3_store;

	store->st_bypass  = 0;
	store->img_width  = ctx->width;
	store->img_height = ctx->height;
	store->st_pitch   = ctx->width;

	store->chk_sum_clr_en = 1;
	store->shadow_clr_sel = 1;
	store->st_max_len_sel = 1;
	store->shadow_clr = 1;

	if (ctx->blending_cnt % 2 != 1) {
		store->st_luma_addr   = ctx->buf_info[0]->iova[0];
		store->st_chroma_addr = ctx->buf_info[0]->iova[0] +
					(ctx->width * ctx->height);
	} else {
		store->st_luma_addr   = ctx->buf_info[1]->iova[0];
		store->st_chroma_addr = ctx->buf_info[1]->iova[0] +
					(ctx->width * ctx->height);
	}

	pr_debug("3DNR nr3store st_luma=0x%lx, st_chroma=0x%lx\n",
		store->st_luma_addr, store->st_chroma_addr);
	pr_debug("3DNR nr3store w=%d,h=%d,frame_w=%d,frame_h=%d\n",
		store->img_width,
		store->img_height,
		ctx->width,
		ctx->height);

	return ret;
}

static int isp_3dnr_gen_crop_config(struct isp_3dnr_ctx_desc *ctx)
{
	int ret = 0;

	if (!ctx) {
		pr_err("invalid parameter, crop\n");
		return -EINVAL;
	}

	ctx->crop.crop_bypass = 1;
	ctx->crop.src_width = ctx->width;
	ctx->crop.src_height = ctx->height;
	ctx->crop.dst_width = ctx->width;
	ctx->crop.dst_height = ctx->height;
	ctx->crop.start_x = 0;
	ctx->crop.start_y = 0;

	return ret;
}


int isp_3dnr_gen_config(struct isp_3dnr_ctx_desc *ctx)
{
	int ret = 0;

	if (!ctx) {
		pr_err("invalid parameter, fail to 3ndr context\n");
		return -EINVAL;
	}

	ret = isp_3dnr_gen_memctrl_config(ctx);
	if (ret) {
		pr_err("fail to generate mem ctrl configuration\n");
		return ret;
	}

	ret = isp_3dnr_gen_store_config(ctx);
	if (ret) {
		pr_err("fail to generate store configuration\n");
		return ret;
	}

	ret = isp_3dnr_gen_crop_config(ctx);
	if (ret) {
		pr_err("fail to generate crop configuration\n");
		return ret;
	}

	ctx->blending_cnt++;

	return ret;
}

int isp_3dnr_update_memctrl_slice_info(struct nr3_slice *in,
	struct nr3_slice_for_blending *out)
{
	uint32_t end_row = 0, end_col = 0, ft_pitch = 0;
	int mv_x = 0, mv_y = 0;
	uint32_t global_img_width = 0, global_img_height = 0;

	if (!in || !out) {
		pr_err("fail to get valid input ptr\n");
		return -EFAULT;
	}

	end_row = in->end_row;
	end_col = in->end_col;
	ft_pitch = in->cur_frame_width;
	mv_x = in->mv_x;
	mv_y = in->mv_y;
	global_img_width = in->cur_frame_width;
	global_img_height = in->cur_frame_height;

	if (in->slice_num == 1) {
		if (mv_x < 0) {
			if ((mv_x) & 0x1) {
				out->ft_y_width = global_img_width + mv_x + 1;
				out->ft_uv_width = global_img_width + mv_x - 1;
				out->src_chr_addr += 2;
			} else {
				out->ft_y_width = global_img_width + mv_x;
				out->ft_uv_width = global_img_width + mv_x;
			}
		} else if (mv_x > 0) {
			if ((mv_x) & 0x1) {
				out->ft_y_width = global_img_width - mv_x + 1;
				out->ft_uv_width = global_img_width - mv_x + 1;
				out->src_lum_addr += mv_x;
				out->src_chr_addr += mv_x - 1;
			} else {
				out->ft_y_width = global_img_width - mv_x;
				out->ft_uv_width = global_img_width - mv_x;
				out->src_lum_addr += mv_x;
				out->src_chr_addr += mv_x;
			}
		}

	} else { /* slice > 1 */
		if (out->start_col == 0) {
			if (mv_x < 0) {
				if ((mv_x) & 0x1) {
					out->src_lum_addr = out->src_lum_addr;
					out->src_chr_addr = out->src_chr_addr +
						2;
				} else {
					out->src_lum_addr = out->src_lum_addr;
					out->src_chr_addr = out->src_chr_addr;
				}
			} else if (mv_x > 0) {
				if ((mv_x) & 0x1) {
					out->src_lum_addr = out->src_lum_addr +
						mv_x;
					out->src_chr_addr = out->src_chr_addr +
						mv_x-1;
				} else {
					out->src_lum_addr = out->src_lum_addr +
						mv_x;
					out->src_chr_addr = out->src_chr_addr +
						mv_x;
				}
			}
		} else {
			if ((mv_x < 0) && ((mv_x) & 0x1)) {
				out->src_lum_addr = out->src_lum_addr + mv_x;
				out->src_chr_addr = out->src_chr_addr + (
					mv_x / 2) * 2;
			} else if ((mv_x > 0) && ((mv_x) & 0x1)) {
				out->src_lum_addr = out->src_lum_addr + mv_x;
				out->src_chr_addr = out->src_chr_addr + mv_x -
					1;
			} else {
				out->src_lum_addr = out->src_lum_addr + mv_x;
				out->src_chr_addr = out->src_chr_addr + mv_x;
			}
		}
		if (out->start_col == 0) {
			if (mv_x < 0) {
				if ((mv_x) & 0x1) {
					out->ft_y_width = out->ft_y_width + mv_x
						+1;
					out->ft_uv_width = out->ft_uv_width +
						mv_x-1;
				} else {
					out->ft_y_width = out->ft_y_width +
						mv_x;
					out->ft_uv_width = out->ft_uv_width +
						mv_x;
				}
			}
		}
		if ((global_img_width - 1) == end_col) {
			if (mv_x > 0) {
				if ((mv_x) & 0x1) {
					out->ft_y_width = out->ft_y_width -
							mv_x + 1;
					out->ft_uv_width = out->ft_uv_width -
						mv_x + 1;
				} else {
					out->ft_y_width = out->ft_y_width -
						mv_x;
					out->ft_uv_width = out->ft_uv_width -
						mv_x;
				}
			}
		}
	} /* slice_num > 1 */

	if (mv_y < 0) {
		if ((mv_y) & 0x1) {
			out->last_line_mode = 0;
			out->ft_uv_height = global_img_height / 2 + mv_y / 2;
		} else{
			out->last_line_mode = 1;
			out->ft_uv_height = global_img_height / 2 +
						mv_y / 2 + 1;
		}
		out->first_line_mode = 0;
		out->ft_y_height = global_img_height + mv_y;
	} else if (mv_y > 0) {
		if ((mv_y) & 0x1) {
			out->first_line_mode = 1;
			out->last_line_mode  = 0;
			out->ft_y_height  = global_img_height - mv_y;
			out->ft_uv_height = global_img_height / 2 - (mv_y / 2);
			out->src_lum_addr += ft_pitch * mv_y;
			out->src_chr_addr += ft_pitch * (mv_y / 2);
		} else {
			out->ft_y_height  = global_img_height - mv_y;
			out->ft_uv_height = global_img_height / 2 - (mv_y / 2);
			out->src_lum_addr += ft_pitch * mv_y;
			out->src_chr_addr += ft_pitch * (mv_y / 2);
		}
	}

	return 0;
}

/*
 * INPUT:
 *   mv_x		: input mv_x (3DNR GME output)
 *   mv_y		: input mv_y (3DNR GME output)
 *   mode_projection	: mode projection in 3DNR GME
 *			: 1 - interlaced mode
 *			: 0 - step mode
 *   sub_me_bypass	: 1 - sub pixel motion estimation bypass
 *			: 0 - do sub pixel motion estimation
 *   input_width	:  input image width for binning and bayer scaler
 *   output_width	: output image width for binning and bayer scaler
 *   input_height	:  input image width for binning and bayer scaler
 *   output_height	: output image width for binning and bayer scaler
 *
 * OUTPUT:
 *   out_mv_x
 *   out_mv_y
 */
#if 0
static int mv_conversion_base_on_resolution(
		int mv_x, int mv_y,
		uint32_t mode_projection,
		uint32_t sub_me_bypass,
		int input_width, int output_width,
		int input_height, int output_height,
		int *out_mv_x, int *out_mv_y)
{
	if (sub_me_bypass == 1) {
		if (mode_projection == 1) {
			if (mv_x > 0)
				*out_mv_x = (mv_x * 2 * output_width +
					(input_width >> 1)) / input_width;
			else
				*out_mv_x = (mv_x * 2 * output_width -
					(input_width >> 1)) / input_width;

			if (mv_y > 0)
				*out_mv_y = (mv_y * 2 * output_height +
					(input_height >> 1)) / input_height;
			else
				*out_mv_y = (mv_y * 2 * output_height -
					(input_height >> 1)) / input_height;
		} else {
			if (mv_x > 0)
				*out_mv_x = (mv_x * output_width +
					(input_width >> 1)) / input_width;
			else
				*out_mv_x = (mv_x * output_width -
					(input_width >> 1)) / input_width;

			if (mv_y > 0)
				*out_mv_y = (mv_y * output_height +
					(input_height >> 1)) / input_height;
			else
				*out_mv_y = (mv_y * output_height -
					(input_height >> 1)) / input_height;
		}
	} else {
		if (mode_projection == 1) {
			if (mv_x > 0)
				*out_mv_x = (mv_x * output_width +
					 (input_width >> 1)) / input_width;
			else
				*out_mv_x = (mv_x * output_width -
					(input_width >> 1)) / input_width;

			if (mv_y > 0)
				*out_mv_y = (mv_y * output_height +
					(input_height >> 1)) / input_height;
			else
				*out_mv_y = (mv_y * output_height -
					(input_height >> 1)) / input_height;
		} else {
			if (mv_x > 0)
				*out_mv_x = (mv_x * output_width +
					input_width) / (input_width * 2);
			else
				*out_mv_x = (mv_x * output_width -
					input_width) / (input_width * 2);

			if (mv_y > 0)
				*out_mv_y = (mv_y * output_height +
					input_height) / (input_height * 2);
			else
				*out_mv_y = (mv_y * output_height -
					input_height) / (input_height * 2);
		}
	}

	return 0;
}
#endif


static int mv_conversion_base_on_resolution(
		int mv_x, int mv_y,
		uint32_t mode_projection,
		uint32_t sub_me_bypass,
		int iw, int ow,
		int ih, int oh,
		int *o_mv_x, int *o_mv_y)
{
	if (sub_me_bypass == 1) {
		if (mode_projection == 1) {
			if (mv_x > 0)
				*o_mv_x = (mv_x * 2 * ow + (iw >> 1)) / iw;
			else
				*o_mv_x = (mv_x * 2 * ow - (iw >> 1)) / iw;

			if (mv_y > 0)
				*o_mv_y = (mv_y * 2 * oh + (ih >> 1)) / ih;
			else
				*o_mv_y = (mv_y * 2 * oh - (ih >> 1)) / ih;
		} else {
			if (mv_x > 0)
				*o_mv_x = (mv_x * ow + (iw >> 1)) / iw;
			else
				*o_mv_x = (mv_x * ow - (iw >> 1)) / iw;

			if (mv_y > 0)
				*o_mv_y = (mv_y * oh + (ih >> 1)) / ih;
			else
				*o_mv_y = (mv_y * oh - (ih >> 1)) / ih;
		}
	} else {
		if (mode_projection == 1) {
			if (mv_x > 0)
				*o_mv_x = (mv_x * ow + (iw >> 1)) / iw;
			else
				*o_mv_x = (mv_x * ow - (iw >> 1)) / iw;

			if (mv_y > 0)
				*o_mv_y = (mv_y * oh + (ih >> 1)) / ih;
			else
				*o_mv_y = (mv_y * oh - (ih >> 1)) / ih;
		} else {
			if (mv_x > 0)
				*o_mv_x = (mv_x * ow + iw) / (iw * 2);
			else
				*o_mv_x = (mv_x * ow - iw) / (iw * 2);

			if (mv_y > 0)
				*o_mv_y = (mv_y * oh + ih) / (ih * 2);
			else
				*o_mv_y = (mv_y * oh - ih) / (ih * 2);
		}
	}

	return 0;
}


int isp_3dnr_conversion_mv(struct isp_3dnr_ctx_desc *nr3_ctx)
{
	int ret = 0;
	int output_x = 0;
	int output_y = 0;
	int input_width = 0;
	int input_height = 0;
	int output_width = 0;
	int output_height = 0;

	if (!nr3_ctx) {
		pr_err("invalid parameter\n");
		return -EINVAL;
	}

	input_width = nr3_ctx->mvinfo->src_width;
	input_height = nr3_ctx->mvinfo->src_height;
	output_width = nr3_ctx->width;
	output_height = nr3_ctx->height;
	ret = mv_conversion_base_on_resolution(
			nr3_ctx->mvinfo->mv_x,
			nr3_ctx->mvinfo->mv_y,
			nr3_ctx->mvinfo->project_mode,
			nr3_ctx->mvinfo->sub_me_bypass,
			input_width,
			output_width,
			input_height,
			output_height,
			&output_x,
			&output_y);

	nr3_ctx->mv.mv_x = output_x;
	nr3_ctx->mv.mv_y = output_y;

	pr_debug("3DNR conv_mv in_x =%d, in_y =%d, out_x=%d, out_y=%d\n",
			nr3_ctx->mvinfo->mv_x, nr3_ctx->mvinfo->mv_y,
		nr3_ctx->mv.mv_x, nr3_ctx->mv.mv_y);

	return ret;
}
