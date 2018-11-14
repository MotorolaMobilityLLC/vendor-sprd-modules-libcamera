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

#include "isp_ltm.h"

#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt) "LTM MAP: %d %d %s : "\
	fmt, current->pid, __LINE__, __func__

static struct isp_dev_ltm_info g_ltm_info_pre = {
	.ltm_stat.bypass  = 0,
	.ltm_stat.tile_num.tile_num_x = 8,
	.ltm_stat.tile_num.tile_num_y = 8,
	.ltm_stat.strength = 4,
	.ltm_stat.region_est_en = 1,
	.ltm_stat.text_point_thres = 7,
	.ltm_stat.text_proportion  = 19,
	.ltm_stat.tile_num_auto    = 0,

	.ltm_map.bypass = 0,
	.ltm_map.ltm_map_video_mode = 1,
};

static struct isp_dev_ltm_info g_ltm_info_cap = {
	.ltm_stat.bypass  = 1,
	.ltm_stat.tile_num.tile_num_x = 8,
	.ltm_stat.tile_num.tile_num_y = 8,
	.ltm_stat.strength = 4,
	.ltm_stat.region_est_en = 1,
	.ltm_stat.text_point_thres = 7,
	.ltm_stat.text_proportion  = 19,
	.ltm_stat.tile_num_auto    = 0,

	.ltm_map.bypass = 0,
	.ltm_map.ltm_map_video_mode = 0,
};


static void isp_ltm_config_hists(uint32_t idx, struct isp_ltm_hists *hists)
{
	unsigned int val;

	if (s_isp_bypass[idx] & (1 << _EISP_LTM))
		hists->bypass = 1;
	ISP_REG_MWR(idx, ISP_LTM_HIST_PARAM, BIT_0, hists->bypass);

	if (hists->bypass)
		return;

	val = ((hists->buf_full_mode & 0x1) << 3) |
		((hists->region_est_en & 0x1) << 2) |
		((hists->binning_en    & 0x1) << 1) |
		(hists->bypass        & 0x1);
	ISP_REG_WR(idx, ISP_LTM_HIST_PARAM, val);

	val = ((hists->roi_start_y & 0x1FFF) << 16) |
		(hists->roi_start_x & 0x1FFF);
	ISP_REG_WR(idx, ISP_LTM_ROI_START, val); /* slice */

	/* tile_num_y tile_num_x HOW TODO */
	val = ((hists->tile_num_y_minus & 0x7)   << 28) |
		((hists->tile_height      & 0x1FF) << 16) |
		((hists->tile_num_x_minus & 0x7)   << 12) |
		(hists->tile_width       & 0x1FF);
	ISP_REG_WR(idx, ISP_LTM_TILE_RANGE, val); /* slice */

	val = ((hists->clip_limit_min & 0xFFFF) << 16) |
		(hists->clip_limit     & 0xFFFF);
	ISP_REG_WR(idx, ISP_LTM_CLIP_LIMIT, val);

	val = ((hists->text_point_thres   & 0x3F) << 16) |
		(hists->texture_proportion & 0x1F);
	ISP_REG_WR(idx, ISP_LTM_THRES, val);

	val = hists->addr;
	ISP_REG_WR(idx, ISP_LTM_ADDR, val); /* slice */

	val = ((hists->wr_num & 0x1FF) << 16) |
		(hists->pitch & 0xFFFF);
	ISP_REG_WR(idx, ISP_LTM_PITCH, val); /* slice */
}


static void isp_ltm_config_map(uint32_t idx, struct isp_ltm_map *map)
{
	unsigned int val;

	if (s_isp_bypass[idx] & (1 << _EISP_LTM))
		map->bypass = 1;
	ISP_REG_MWR(idx, ISP_LTM_MAP_PARAM0, BIT_0, map->bypass);
	if (map->bypass)
		return;

	val = ((map->fetch_wait_line & 0x1) << 4) |
		((map->fetch_wait_en   & 0x1) << 3) |
		((map->hist_error_en   & 0x1) << 2) |
		((map->burst8_en       & 0x1) << 1) |
		(map->bypass          & 0x1);
	ISP_REG_WR(idx, ISP_LTM_MAP_PARAM0, val);

	val = ((map->tile_y_num  & 0x7)   << 28) |
		((map->tile_x_num  & 0x7)   << 24) |
		((map->tile_height & 0x3FF) << 12) |
		(map->tile_width  & 0x3FF);
	ISP_REG_WR(idx, ISP_LTM_MAP_PARAM1, val); /* slice */

	val = map->tile_size_pro & 0x3FFFF;
	ISP_REG_WR(idx, ISP_LTM_MAP_PARAM2, val);

	val = ((map->tile_right_flag & 0x1)   << 23) |
		((map->tile_start_y    & 0x7FF) << 12) |
		((map->tile_left_flag  & 0x1)   << 11) |
		(map->tile_start_x    & 0x7FF);
	ISP_REG_WR(idx, ISP_LTM_MAP_PARAM3, val); /* slice */

	val = map->mem_init_addr;
	ISP_REG_WR(idx, ISP_LTM_MAP_PARAM4, val); /* slice */

	val = (map->hist_pitch & 0x7) << 24;
	ISP_REG_WR(idx, ISP_LTM_MAP_PARAM5, val);
}


int isp_ltm_config_param(struct isp_ltm_ctx_desc *ctx)
{
	uint32_t idx = ctx->isp_pipe_ctx_id;
	struct isp_ltm_hists *hists = &ctx->hists;
	struct isp_ltm_map *map     = &ctx->map;

	if (ctx->bypass) {
		hists->bypass = 1;
		map->bypass   = 1;
	}

	isp_ltm_config_hists(idx, hists);
	isp_ltm_config_map(idx, map);

	return 0;
}

struct isp_dev_ltm_info *isp_ltm_get_tuning_config(int type)
{
	struct isp_dev_ltm_info *info;

	if (type == ISP_PRO_LTM_PRE_PARAM)
		info = &g_ltm_info_pre;
	else
		info = &g_ltm_info_cap;

	return info;
}

int isp_k_ltm_block(struct isp_io_param *param, uint32_t idx)
{
	int ret = 0;
	struct isp_dev_ltm_info ltm_info;
	struct isp_dev_ltm_info *info;

	ret = copy_from_user((void *)&ltm_info,
			     param->property_param,
			     sizeof(struct isp_dev_ltm_info));
	if (ret != 0) {
		pr_err("fail to get ltm from user, ret = %d\n", ret);
		return -EPERM;
	}

	if (param->property == ISP_PRO_LTM_PRE_PARAM)
		info = &g_ltm_info_pre;
	else
		info = &g_ltm_info_cap;

//	isp_ltm_config_hists(idx, &hists);
	return ret;
}
