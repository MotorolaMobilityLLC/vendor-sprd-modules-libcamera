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
#include <linux/uaccess.h>
#include <sprd_mm.h>
#include <sprd_isp_r8p1.h>

#include "isp_reg.h"
#include "cam_types.h"
#include "cam_block.h"

#include "isp_3dnr.h"

#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt) "3DNR: %d %d %s : "\
	fmt, current->pid, __LINE__, __func__


static struct isp_3dnr_const_param g_3dnr_param_pre = {
	0, 1,
	5, 3, 3, 255, 255, 255,
	0, 255, 0, 255,
	20, 20, 20, 20, 20, 20, 20, 20, 20,
	15, 15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15,
	63, 63, 63, 63, 63, 63, 63, 63, 63,
	63, 63, 63, 63, 63, 63, 63, 63, 63,
	63, 63, 63, 63, 63, 63, 63, 63, 63,
	127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
	{180, 180, 180, 180}, {180, 180, 180, 180}, {180, 180, 180, 180},
	{31, 37, 48, 63}, {31, 37, 48, 63}, {1, 2, 2, 3}, {1, 2, 2, 3},
	814, 931, 1047
};

static struct isp_3dnr_const_param g_3dnr_param_cap = {
	0, 1,
	5, 3, 3, 255, 255, 255,
	0, 255, 0, 255,
	30, 30, 30, 30, 30, 30, 30, 30, 30,
	30, 30, 30, 30, 30, 30, 30, 30, 30,
	30, 30, 30, 30, 30, 30, 30, 30, 30,
	63, 63, 63, 63, 63, 63, 63, 63, 63,
	63, 63, 63, 63, 63, 63, 63, 63, 63,
	63, 63, 63, 63, 63, 63, 63, 63, 63,
	127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
	{180, 180, 180, 180}, {180, 180, 180, 180}, {180, 180, 180, 180},
	{31, 37, 48, 63}, {31, 37, 48, 63}, {1, 2, 2, 3}, {1, 2, 2, 3},
	814, 931, 1047
};

static unsigned int g_frame_param[4][3] = {
	{128, 128, 128},
	{154, 154, 154},
	{154, 180, 180},
	{180, 180, 180},
};

/*
 * static function
 */
#if 0
static void isp_3dnr_config_fast_me(enum isp_id idx,
				    struct isp_3dnr_fast_me *fast_me)
{
	uint32_t val = 0;

	if (fast_me == NULL) {
		pr_err("fail to 3ndr fast_me_reg param NULL\n");
		return;
	}

	val = ((fast_me->nr3_channel_sel & 0x3) << 2) |
	       (fast_me->nr3_project_mode & 0x3);

	DCAM_REG_MWR(idx, DCAM_NR3_PARA0, 0xF, val);
}
#endif

static void isp_3dnr_config_mem_ctrl(uint32_t idx,
				     struct isp_3dnr_mem_ctrl *mem_ctrl)
{
	unsigned int val;

	if (s_isp_bypass[idx] & (1 << _EISP_NR3))
		mem_ctrl->bypass = 1;
	ISP_REG_MWR(idx,
		    ISP_3DNR_MEM_CTRL_PARAM0,
		    BIT_0,
		    mem_ctrl->bypass);

	val = ((mem_ctrl->nr3_done_mode & 0x1) << 1)	|
	      ((mem_ctrl->nr3_ft_path_sel & 0x1) << 2)  |
	      ((mem_ctrl->back_toddr_en & 0x1) << 6)	|
	      ((mem_ctrl->chk_sum_clr_en & 0x1) << 9)	|
	      ((mem_ctrl->data_toyuv_en & 0x1) << 12)	|
	      ((mem_ctrl->roi_mode & 0x1) << 14)	|
	      ((mem_ctrl->retain_num & 0x7F) << 16)	|
	      ((mem_ctrl->ref_pic_flag & 0x1) << 23)	|
	      ((mem_ctrl->ft_max_len_sel & 0x1) << 28);
	ISP_REG_WR(idx, ISP_3DNR_MEM_CTRL_PARAM0, val);

	val = ((mem_ctrl->last_line_mode & 0x1) << 1)	|
	      ((mem_ctrl->first_line_mode & 0x1));
	ISP_REG_WR(idx, ISP_3DNR_MEM_CTRL_LINE_MODE, val);

	val = ((mem_ctrl->start_col & 0x1FFF) << 16) |
	       (mem_ctrl->start_row & 0x1FFF);
	ISP_REG_WR(idx, ISP_3DNR_MEM_CTRL_PARAM1, val);

	val = ((mem_ctrl->global_img_height & 0x1FFF) << 16) |
	       (mem_ctrl->global_img_width & 0x1FFF);
	ISP_REG_WR(idx, ISP_3DNR_MEM_CTRL_PARAM2, val);

	val = ((mem_ctrl->img_height & 0xFFF) << 16)	|
	       (mem_ctrl->img_width & 0xFFF);
	ISP_REG_WR(idx, ISP_3DNR_MEM_CTRL_PARAM3, val);

	val = ((mem_ctrl->ft_y_height & 0xFFF) << 16)	|
	       (mem_ctrl->ft_y_width & 0xFFF);
	ISP_REG_WR(idx, ISP_3DNR_MEM_CTRL_PARAM4, val);

	val = (mem_ctrl->ft_uv_width & 0xFFF)
		| ((mem_ctrl->ft_uv_height & 0xFFF) << 16);
	ISP_REG_WR(idx, ISP_3DNR_MEM_CTRL_PARAM5, val);

	val = ((mem_ctrl->mv_x & 0xFF) << 8) |
	       (mem_ctrl->mv_y & 0xFF);
	ISP_REG_WR(idx, ISP_3DNR_MEM_CTRL_PARAM7, val);

	ISP_REG_WR(idx,
		   ISP_3DNR_MEM_CTRL_FT_CUR_LUMA_ADDR,
		   mem_ctrl->ft_luma_addr);

	ISP_REG_WR(idx,
		   ISP_3DNR_MEM_CTRL_FT_CUR_CHROMA_ADDR,
		   mem_ctrl->ft_chroma_addr);

	val = mem_ctrl->img_width & 0xFFFF;
	ISP_REG_WR(idx, ISP_3DNR_MEM_CTRL_FT_CTRL_PITCH, val);

	val = ((mem_ctrl->blend_y_en_start_col & 0xFFF) << 16)  |
	       (mem_ctrl->blend_y_en_start_row & 0xFFF);
	ISP_REG_WR(idx, ISP_3DNR_MEM_CTRL_PARAM8, val);

	/*
	 * following code in minicode ? How TODO
	 *
	 * val = (((mem_ctrl->img_height - 1) & 0xFFF) << 16)    |
	 *        ((mem_ctrl->img_width - 1) & 0xFFF);
	 */
	val = ((mem_ctrl->blend_y_en_end_col & 0xFFF) << 16)    |
	       (mem_ctrl->blend_y_en_end_row & 0xFFF);
	ISP_REG_WR(idx, ISP_3DNR_MEM_CTRL_PARAM9, val);

	val = ((mem_ctrl->blend_uv_en_start_col & 0xFFF) << 16)	|
	       (mem_ctrl->blend_uv_en_start_row & 0xFFF);
	ISP_REG_WR(idx, ISP_3DNR_MEM_CTRL_PARAM10, val);

	/*
	 * following code in minicode ? How TODO
	 *
	 * val = (((mem_ctrl->img_height/2 - 1) & 0xFFF) << 16)    |
	 *        ((mem_ctrl->img_width - 1) & 0xFFF);
	 */
	val = ((mem_ctrl->blend_uv_en_end_col & 0xFFF) << 16)	|
	       (mem_ctrl->blend_uv_en_end_row & 0xFFF);
	ISP_REG_WR(idx, ISP_3DNR_MEM_CTRL_PARAM11, val);

	val = ((mem_ctrl->ft_hblank_num & 0xFFFF) << 16)	|
	      ((mem_ctrl->pipe_hblank_num & 0xFF) << 8)		|
	       (mem_ctrl->pipe_flush_line_num & 0xFF);
	ISP_REG_WR(idx, ISP_3DNR_MEM_CTRL_PARAM12, val);

	val = ((mem_ctrl->pipe_nfull_num & 0x7FF) << 16)	|
	       (mem_ctrl->ft_fifo_nfull_num & 0xFFF);
	ISP_REG_WR(idx, ISP_3DNR_MEM_CTRL_PARAM13, val);
}

static void isp_3dnr_config_blend(uint32_t idx,
				  struct isp_3dnr_const_param *blend)
{
	unsigned int val;

	if (blend == NULL) {
		pr_err("fail to 3ndr config_blend_reg param NULL\n");
		return;
	}

	val = ((blend->u_pixel_noise_threshold & 0xFF) << 24) |
	      ((blend->v_pixel_noise_threshold & 0xFF) << 16) |
	      ((blend->y_pixel_noise_weight & 0xFF) << 8)     |
	       (blend->u_pixel_noise_weight & 0xFF);
	ISP_REG_WR(idx, ISP_3DNR_BLEND_CFG2, val);

	val = ((blend->v_pixel_noise_weight & 0xFF) << 24) |
	      ((blend->threshold_radial_variation_u_range_min & 0xFF) << 16) |
	      ((blend->threshold_radial_variation_u_range_max & 0xFF) << 8)  |
	       (blend->threshold_radial_variation_v_range_min & 0xFF);
	ISP_REG_WR(idx, ISP_3DNR_BLEND_CFG3, val);

	val = ((blend->threshold_radial_variation_v_range_max & 0xFF) << 24) |
	      ((blend->y_threshold_polyline_0 & 0xFF) << 16)		     |
	      ((blend->y_threshold_polyline_1 & 0xFF) << 8)		     |
	       (blend->y_threshold_polyline_2 & 0xFF);
	ISP_REG_WR(idx, ISP_3DNR_BLEND_CFG4, val);

	val = ((blend->y_threshold_polyline_3 & 0xFF) << 24)	|
	      ((blend->y_threshold_polyline_4 & 0xFF) << 16)	|
	      ((blend->y_threshold_polyline_5 & 0xFF) << 8)	|
	       (blend->y_threshold_polyline_6 & 0xFF);
	ISP_REG_WR(idx, ISP_3DNR_BLEND_CFG5, val);

	val = ((blend->y_threshold_polyline_7 & 0xFF) << 24)	|
	      ((blend->y_threshold_polyline_8 & 0xFF) << 16)	|
	      ((blend->u_threshold_polyline_0 & 0xFF) << 8)	|
	       (blend->u_threshold_polyline_1 & 0xFF);
	ISP_REG_WR(idx, ISP_3DNR_BLEND_CFG6, val);

	val = ((blend->u_threshold_polyline_2 & 0xFF) << 24)	|
	      ((blend->u_threshold_polyline_3 & 0xFF) << 16)	|
	      ((blend->u_threshold_polyline_4 & 0xFF) << 8)	|
	       (blend->u_threshold_polyline_5 & 0xFF);
	ISP_REG_WR(idx, ISP_3DNR_BLEND_CFG7, val);

	val = ((blend->u_threshold_polyline_6 & 0xFF) << 24)	|
	      ((blend->u_threshold_polyline_7 & 0xFF) << 16)	|
	      ((blend->u_threshold_polyline_8 & 0xFF) << 8)	|
	       (blend->v_threshold_polyline_0 & 0xFF);
	ISP_REG_WR(idx, ISP_3DNR_BLEND_CFG8, val);

	val = ((blend->v_threshold_polyline_1 & 0xFF) << 24)	|
	      ((blend->v_threshold_polyline_2 & 0xFF) << 16)	|
	      ((blend->v_threshold_polyline_3 & 0xFF) << 8)	|
	       (blend->v_threshold_polyline_4 & 0xFF);
	ISP_REG_WR(idx, ISP_3DNR_BLEND_CFG9, val);

	val = ((blend->v_threshold_polyline_5 & 0xFF) << 24)	|
	      ((blend->v_threshold_polyline_6 & 0xFF) << 16)	|
	      ((blend->v_threshold_polyline_7 & 0xFF) << 8)	|
	       (blend->v_threshold_polyline_8 & 0xFF);
	ISP_REG_WR(idx, ISP_3DNR_BLEND_CFG10, val);

	val = ((blend->y_intensity_gain_polyline_0 & 0x7F) << 24) |
	      ((blend->y_intensity_gain_polyline_1 & 0x7F) << 16) |
	      ((blend->y_intensity_gain_polyline_2 & 0x7F) << 8)  |
	       (blend->y_intensity_gain_polyline_3 & 0x7F);
	ISP_REG_WR(idx, ISP_3DNR_BLEND_CFG11, val);

	val = ((blend->y_intensity_gain_polyline_4 & 0x7F) << 24) |
	      ((blend->y_intensity_gain_polyline_5 & 0x7F) << 16) |
	      ((blend->y_intensity_gain_polyline_6 & 0x7F) << 8)  |
	       (blend->y_intensity_gain_polyline_7 & 0x7F);
	ISP_REG_WR(idx, ISP_3DNR_BLEND_CFG12, val);

	val = ((blend->y_intensity_gain_polyline_8 & 0x7F) << 24) |
	      ((blend->u_intensity_gain_polyline_0 & 0x7F) << 16) |
	      ((blend->u_intensity_gain_polyline_1 & 0x7F) << 8)  |
	       (blend->u_intensity_gain_polyline_2 & 0x7F);
	ISP_REG_WR(idx, ISP_3DNR_BLEND_CFG13, val);

	val = ((blend->u_intensity_gain_polyline_3 & 0x7F) << 24) |
	      ((blend->u_intensity_gain_polyline_4 & 0x7F) << 16) |
	      ((blend->u_intensity_gain_polyline_5 & 0x7F) << 8)  |
	       (blend->u_intensity_gain_polyline_6 & 0x7F);
	ISP_REG_WR(idx, ISP_3DNR_BLEND_CFG14, val);

	val = ((blend->u_intensity_gain_polyline_7 & 0x7F) << 24) |
	      ((blend->u_intensity_gain_polyline_8 & 0x7F) << 16) |
	      ((blend->v_intensity_gain_polyline_0 & 0x7F) << 8)  |
	       (blend->v_intensity_gain_polyline_1 & 0x7F);
	ISP_REG_WR(idx, ISP_3DNR_BLEND_CFG15, val);

	val = ((blend->v_intensity_gain_polyline_2 & 0x7F) << 24) |
	      ((blend->v_intensity_gain_polyline_3 & 0x7F) << 16) |
	      ((blend->v_intensity_gain_polyline_4 & 0x7F) << 8)  |
	       (blend->v_intensity_gain_polyline_5 & 0x7F);
	ISP_REG_WR(idx, ISP_3DNR_BLEND_CFG16, val);

	val = ((blend->v_intensity_gain_polyline_6 & 0x7F) << 24) |
	      ((blend->v_intensity_gain_polyline_7 & 0x7F) << 16) |
	      ((blend->v_intensity_gain_polyline_8 & 0x7F) << 8)  |
	       (blend->gradient_weight_polyline_0 & 0x7F);
	ISP_REG_WR(idx, ISP_3DNR_BLEND_CFG17, val);

	val = ((blend->gradient_weight_polyline_1 & 0x7F) << 24) |
	      ((blend->gradient_weight_polyline_2 & 0x7F) << 16) |
	      ((blend->gradient_weight_polyline_3 & 0x7F) << 8)	 |
	       (blend->gradient_weight_polyline_4 & 0x7F);
	ISP_REG_WR(idx, ISP_3DNR_BLEND_CFG18, val);

	val = ((blend->gradient_weight_polyline_5 & 0x7F) << 24) |
	      ((blend->gradient_weight_polyline_6 & 0x7F) << 16) |
	      ((blend->gradient_weight_polyline_7 & 0x7F) << 8)  |
	       (blend->gradient_weight_polyline_8 & 0x7F);
	ISP_REG_WR(idx, ISP_3DNR_BLEND_CFG19, val);

	val = ((blend->gradient_weight_polyline_9 & 0x7F) << 24)  |
	      ((blend->gradient_weight_polyline_10 & 0x7F) << 16) |
	      ((blend->u_threshold_factor[0] & 0x7F) << 8)	  |
	       (blend->u_threshold_factor[1] & 0x7F);
	ISP_REG_WR(idx, ISP_3DNR_BLEND_CFG20, val);

	val = ((blend->u_threshold_factor[2] & 0x7F) << 24)	|
	      ((blend->u_threshold_factor[3] & 0x7F) << 16)	|
	      ((blend->v_threshold_factor[0] & 0x7F) << 8)	|
	       (blend->v_threshold_factor[1] & 0x7F);
	ISP_REG_WR(idx, ISP_3DNR_BLEND_CFG21, val);

	val = ((blend->v_threshold_factor[2] & 0x7F) << 24)	|
	      ((blend->v_threshold_factor[3] & 0x7F) << 16)	|
	      ((blend->u_divisor_factor[0] & 0x7) << 12)	|
	      ((blend->u_divisor_factor[1] & 0x7) << 8)		|
	      ((blend->u_divisor_factor[2] & 0x7) << 4)		|
	       (blend->u_divisor_factor[3] & 0x7);
	ISP_REG_WR(idx, ISP_3DNR_BLEND_CFG22, val);

	val = ((blend->v_divisor_factor[0] & 0x7) << 28)	|
	      ((blend->v_divisor_factor[1] & 0x7) << 24)	|
	      ((blend->v_divisor_factor[2] & 0x7) << 20)	|
	      ((blend->v_divisor_factor[3] & 0x7) << 16)	|
	       (blend->r1_circle & 0xFFF);
	ISP_REG_WR(idx, ISP_3DNR_BLEND_CFG23, val);

	val = ((blend->r2_circle & 0xFFF) << 16) |
	       (blend->r3_circle & 0xFFF);
	ISP_REG_WR(idx, ISP_3DNR_BLEND_CFG24, val);
}

static void isp_3dnr_config_store(uint32_t idx,
				  struct isp_3dnr_store *nr3_store)
{
	unsigned int val;

	if (s_isp_bypass[idx] & (1 << _EISP_NR3))
		nr3_store->st_bypass = 1;
	val = ((nr3_store->chk_sum_clr_en & 0x1) << 4)	|
	      ((nr3_store->shadow_clr_sel & 0x1) << 3)	|
	      ((nr3_store->st_max_len_sel & 0x1) << 1)	|
	       (nr3_store->st_bypass & 0x1);
	ISP_REG_WR(idx, ISP_3DNR_STORE_PARAM, val);

	val = ((nr3_store->img_height & 0xFFFF) << 16)	|
	       (nr3_store->img_width & 0xFFFF);
	ISP_REG_WR(idx, ISP_3DNR_STORE_SIZE, val);

	val = nr3_store->st_luma_addr;
	ISP_REG_WR(idx, ISP_3DNR_STORE_LUMA_ADDR, val);

	val = nr3_store->st_chroma_addr;
	ISP_REG_WR(idx, ISP_3DNR_STORE_CHROMA_ADDR, val);

	val = nr3_store->st_pitch & 0xFFFF;
	ISP_REG_WR(idx, ISP_3DNR_STORE_PITCH, val);

	ISP_REG_MWR(idx,
		    ISP_3DNR_STORE_SHADOW_CLR,
		    BIT_0,
		    nr3_store->shadow_clr);
}

static void isp_3dnr_config_crop(uint32_t idx,
				 struct isp_3dnr_crop *crop)
{
	unsigned int val;

	if (s_isp_bypass[idx] & (1 << _EISP_NR3))
		crop->crop_bypass = 1;
	ISP_REG_MWR(idx,
		    ISP_3DNR_MEM_CTRL_PRE_PARAM0,
		    BIT_0,
		    crop->crop_bypass);

	val = ((crop->src_height & 0xFFFF) << 16) |
	       (crop->src_width & 0xFFFF);
	ISP_REG_WR(idx, ISP_3DNR_MEM_CTRL_PRE_PARAM1, val);

	val = ((crop->dst_height & 0xFFFF) << 16) |
	       (crop->dst_width & 0xFFFF);
	ISP_REG_WR(idx, ISP_3DNR_MEM_CTRL_PRE_PARAM2, val);

	val = ((crop->start_x & 0xFFFF) << 16)	  |
	       (crop->start_y & 0xFFFF);
	ISP_REG_WR(idx, ISP_3DNR_MEM_CTRL_PRE_PARAM3, val);
}

static int isp_k_3dnr_block(struct isp_io_param *param, uint32_t idx)
{
	int ret = 0;
	struct isp_3dnr_tunning_param tunning_param;
	struct isp_3dnr_fast_me fast_me = {0};
	struct isp_3dnr_const_param *blend_para = NULL;

	ret = copy_from_user((void *)&tunning_param,
		param->property_param, sizeof(struct isp_3dnr_tunning_param));
	if (ret != 0) {
		pr_err("fail to 3dnr copy from user, ret = %d\n", ret);
		return -EPERM;
	}

	if (param->property == ISP_PRO_3DNR_UPDATE_PRE_PARAM)
		blend_para = &g_3dnr_param_pre;
	else
		blend_para = &g_3dnr_param_cap;

	memcpy(blend_para, &tunning_param.blend_param,
		sizeof(struct isp_3dnr_const_param));
	memcpy(&fast_me, &tunning_param.fast_me,
		sizeof(struct isp_3dnr_fast_me));

#if 0
	isp_3dnr_config_fast_me(idx, &fast_me);
#endif

	isp_3dnr_config_blend(idx, blend_para);

	return ret;
}

/*
 * global function
 */
void isp_3dnr_bypass_config(uint32_t idx)
{
	ISP_REG_MWR(idx, ISP_3DNR_MEM_CTRL_PARAM0, BIT_0, 1);
	ISP_REG_MWR(idx, ISP_3DNR_BLEND_CONTROL0, BIT_0, 1);
	ISP_REG_MWR(idx, ISP_3DNR_STORE_PARAM, BIT_0, 1);
	ISP_REG_MWR(idx, ISP_3DNR_MEM_CTRL_PRE_PARAM0, BIT_0, 1);
	ISP_REG_MWR(idx, ISP_COMMON_SCL_PATH_SEL, BIT_8, 0 << 8);
}

#ifdef _NR3_DATA_TO_YUV_
static unsigned long irq_base[ISP_CONTEXT_MAX] = {
	ISP_P0_INT_BASE,
	ISP_C0_INT_BASE,
	ISP_P1_INT_BASE,
	ISP_C1_INT_BASE
};
#endif /* _NR3_DATA_TO_YUV_ */

void isp_3dnr_config_param(struct isp_3dnr_ctx_desc *ctx,
			   uint32_t idx,
			   enum nr3_func_type type_id)
{
	uint32_t val = 0;
	uint32_t blend_cnt = 0;
	struct isp_3dnr_mem_ctrl *mem_ctrl = NULL;
	struct isp_3dnr_store *nr3_store = NULL;
	struct isp_3dnr_crop *crop = NULL;
	struct isp_3dnr_const_param *blend_ptr = NULL;

	if (!ctx) {
		pr_err("fail to 3dnr_config_reg parm NULL\n");
		return;
	}

	/*config memctl*/
	mem_ctrl = &ctx->mem_ctrl;
	isp_3dnr_config_mem_ctrl(idx, mem_ctrl);

	/*config store*/
	nr3_store = &ctx->nr3_store;
	isp_3dnr_config_store(idx, nr3_store);

	/*config crop*/
	crop = &ctx->crop;
	isp_3dnr_config_crop(idx, crop);

	/* open nr3 path in common config */
	ISP_REG_MWR(idx, ISP_COMMON_SCL_PATH_SEL, BIT_8, 0x1 << 8);
#ifdef _NR3_DATA_TO_YUV_
	if (mem_ctrl->data_toyuv_en) {
		uint32_t val = 0;

		val = ISP_HREG_RD(ISP_P0_INT_BASE + ISP_INT_STATUS);
		pr_info("vall 0x%x\n", val);
		val |= (BIT_1);
		pr_info("val after 0x%x\n", val);
		ISP_HREG_MWR(irq_base[idx] + ISP_INT_ALL_DONE_CTRL, 0x1F, val);

		ISP_REG_MWR(idx, ISP_STORE_PRE_CAP_BASE + ISP_STORE_PARAM,
			BIT_0, 0);
		ISP_REG_MWR(idx, ISP_STORE_VID_BASE + ISP_STORE_PARAM,
			BIT_0, 1);
		ISP_REG_MWR(idx, ISP_STORE_THUMB_BASE + ISP_STORE_PARAM,
			BIT_0, 1);

	} else {
		uint32_t val = 0;

		val = ISP_HREG_RD(ISP_P0_INT_BASE+ISP_INT_STATUS);
		pr_info("val 0x%x\n", val);
		val &= (~BIT_1);
		pr_info("val after 0x%x\n", val);
		ISP_HREG_MWR(irq_base[idx] + ISP_INT_ALL_DONE_CTRL, 0x1F, val);

		ISP_REG_MWR(idx, ISP_STORE_PRE_CAP_BASE + ISP_STORE_PARAM,
			BIT_0, 1);
		ISP_REG_MWR(idx, ISP_STORE_VID_BASE + ISP_STORE_PARAM,
			BIT_0, 1);
		ISP_REG_MWR(idx, ISP_STORE_THUMB_BASE + ISP_STORE_PARAM,
			BIT_0, 1);
	}
#endif /* _NR3_DATA_TO_YUV_ */

	/*config variational blending*/
	ISP_REG_MWR(idx, ISP_3DNR_BLEND_CONTROL0, BIT_0, 0);
	ISP_REG_MWR(idx, ISP_3DNR_BLEND_CONTROL0, BIT_1, 0 << 1);
	ISP_REG_MWR(idx, ISP_3DNR_BLEND_CONTROL0, BIT_2, 1 << 2);

	blend_cnt = ctx->blending_cnt;
	if (blend_cnt > 3)
		blend_cnt = 3;

	if (type_id != NR3_FUNC_CAP) {
		blend_ptr = &g_3dnr_param_pre;
		val = (blend_ptr->y_pixel_noise_threshold & 0xFF)	  |
		((blend_ptr->v_pixel_src_weight[blend_cnt] & 0xFF) << 8)  |
		((blend_ptr->u_pixel_src_weight[blend_cnt] & 0xFF) << 16) |
		((blend_ptr->y_pixel_src_weight[blend_cnt] & 0xFF) << 24);
	} else {
		blend_ptr = &g_3dnr_param_cap;
		val = (blend_ptr->y_pixel_noise_threshold & 0xFF)
			| ((g_frame_param[blend_cnt][2] & 0xFF) << 8)
			| ((g_frame_param[blend_cnt][1] & 0xFF) << 16)
			| ((g_frame_param[blend_cnt][0] & 0xFF) << 24);
	}
	ISP_REG_WR(idx, ISP_3DNR_BLEND_CFG1, val);

	blend_ptr->r1_circle = (unsigned int)(7 * mem_ctrl->img_width / 20);
	blend_ptr->r2_circle = (unsigned int)(2 * mem_ctrl->img_width / 5);
	blend_ptr->r3_circle = (unsigned int)(9 * mem_ctrl->img_width / 20);

	val = ((blend_ptr->v_divisor_factor[0] & 0x7) << 28)
		| ((blend_ptr->v_divisor_factor[1] & 0x7) << 24)
		| ((blend_ptr->v_divisor_factor[2] & 0x7) << 20)
		| ((blend_ptr->v_divisor_factor[3] & 0x7) << 16)
		| (blend_ptr->r1_circle & 0xFFF);
	ISP_REG_WR(idx, ISP_3DNR_BLEND_CFG23, val);
	val = ((blend_ptr->r2_circle & 0xFFF) << 16)
		| (blend_ptr->r3_circle & 0xFFF);
	ISP_REG_WR(idx, ISP_3DNR_BLEND_CFG24, val);
}

int isp_k_cfg_3dnr(struct isp_io_param *param,
	struct isp_k_block *isp_k_param, uint32_t idx)
{
	int ret = 0;

	if (!param || !param->property_param) {
		pr_err("fail, param error\n");
		return -1;
	}

	switch (param->property) {
	case ISP_PRO_3DNR_UPDATE_CAP_PARAM:
	case ISP_PRO_3DNR_UPDATE_PRE_PARAM:
		ret = isp_k_3dnr_block(param, idx);
		break;
	default:
		pr_err("fail to 3dnr cmd id = %d\n", param->property);
		break;
	}

	return ret;
}
