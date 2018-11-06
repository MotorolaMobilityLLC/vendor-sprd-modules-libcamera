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

#include "isp_ltm.h"


#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt) "LTM logic: %d %d %s : "\
        fmt, current->pid, __LINE__, __func__

/*
 * 1. The static of preview frame N can be applied to another preview frame N+1
 * 2. The static of preview frame N can be applied to capture frame N
 *
 * 3. histogram statics support 128 bins (size 16 bits), Max statics is 65535 ?
 * 4. ROI crop, max support is 240 x 180
 * 5. Horizen (8, 6, 4, 2), no limitation in vertical
 * 6. Min tile is 128 x 20
 * 7. Max tile is 65536 (tile_height x tile_width)
 * */
#define BIN_NUM_BIT 7
#define TILE_NUM_MIN 4
#define TILE_NUM_MAX 8
#define TILE_MAX_SIZE (1<<16)
#define TILE_WIDTH_MIN 160

/*
 * row : 2, 4, 8
 * col : 2, 4, 8
 * */
#define MAX_TILE 64

#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#define MAX(a,b) (((a) > (b)) ? (a) : (b))


/*
 * Input:
 *	frame size: x, y
 *
 * Output:
 *	frame size after binning: x, y
 *	binning factor
 *
 * Notes:
 *	Sharkl5 ONLY suppout 1/2 binning
 *
 * */
static int ltm_calc_binning_factor(ltm_param_t *histo)
{
	int ret = 0;
	int min_tile_num = 0;
	int set_tile_num = 0;
	int frame_size = 0;
	int binning_factor = 0;
	int pow_factor = 0;

	frame_size = histo->frame_width * histo->frame_height;
	/*
	 * min_tile_num = (uint8)ceil ((float)(frame_width*frame_height)/TILE_MAX_SIZE);
	 */
	min_tile_num = (frame_size + TILE_MAX_SIZE - 1 ) / TILE_MAX_SIZE;
	set_tile_num = histo->tile_num_x * histo->tile_num_y;

	/*
	 * binning_factor = (uint8)ceil(MAX(log(min_tile_num/64.0)/log(4.0),0));
	 */
	if (min_tile_num <= set_tile_num) {
		binning_factor = 0;
		pow_factor = 1; /* pow(2.0, binning_factor) */
	} else if (min_tile_num <= set_tile_num *4) {
		binning_factor = 1;
		pow_factor = 2; /* pow(2.0, binning_factor) */
	} else {
		BUG_ON(0);
	}

	/*
	 * frame_width  = frame_width /(2*(uint16)pow(2.0, binning_factor)) *2;
	 * frame_height = frame_height/(2*(uint16)pow(2.0, binning_factor)) *2;
	 */
	pr_debug("B binning_factor[%d], pow_factor[%d], frame_width[%d], frame_height[%d]\n",
		binning_factor, pow_factor, histo->frame_width, histo->frame_height);

	histo->frame_width  = histo->frame_width  / (2 * pow_factor) * 2;
	histo->frame_height = histo->frame_height / (2 * pow_factor) * 2;

	histo->binning_en = binning_factor;
	pr_debug("A binning_factor[%d], pow_factor[%d], frame_width[%d], frame_height[%d]\n",
		binning_factor, pow_factor, histo->frame_width, histo->frame_height);

	return ret;
}

static int ltm_calc_histo_param(ltm_param_t *param_histo)
{
#if 0
	uint8 min_tile_num, binning_factor, max_tile_col, min_tile_row, tile_num_x, tile_num_y;
	uint8 cropRows, cropCols, cropUp, cropLeft, cropRight;
	uint16 min_tile_width, max_tile_height, tile_width, tile_height;
	uint16 clipLimit_min, clipLimit;
	uint8 strength = param_histo->strength;
#else
	uint32_t max_tile_col, min_tile_row;
	uint32_t tile_num_x, tile_num_y;
	uint32_t cropRows, cropCols, cropUp, cropLeft, cropRight, cropDown;
	uint32_t min_tile_width, max_tile_height, tile_width, tile_height;
	uint32_t clipLimit_min, clipLimit;
	uint32_t strength = param_histo->strength;
	uint32_t frame_width  = param_histo->frame_width;
	uint32_t frame_height = param_histo->frame_height;
#endif
	ltm_calc_binning_factor(param_histo);

	frame_width  = param_histo->frame_width;
	frame_height = param_histo->frame_height;

	if (param_histo->tile_num_auto)
	{
		int v_ceil = 0;
		int tmp = 0;

		max_tile_col = MAX(MIN(frame_width /(TILE_WIDTH_MIN*2) *2,
				       TILE_NUM_MAX), TILE_NUM_MIN);
		min_tile_width = frame_width /(max_tile_col*2) *2;
		max_tile_height = TILE_MAX_SIZE /(min_tile_width*2) *2;
		/*
		min_tile_row = (uint8)MAX(MIN(ceil((float)frame_height / max_tile_height), TILE_NUM_MAX), TILE_NUM_MIN);
		*/
		v_ceil = (frame_height + max_tile_height - 1) / max_tile_height;
		min_tile_row = MAX(MIN(v_ceil, TILE_NUM_MAX),
					  TILE_NUM_MIN);

		tile_num_y = (min_tile_row / 2) * 2;
		tile_num_x = MIN(MAX(((tile_num_y * frame_width / frame_height) / 2 ) * 2,
				     TILE_NUM_MIN), max_tile_col);

		tile_width  = frame_width  / (2 * tile_num_x) * 2;
		tile_height = frame_height / (2 * tile_num_y) * 2;

		while(tile_width*tile_height >= TILE_MAX_SIZE)
		{
			tile_num_y = MIN(MAX(tile_num_y + 2, TILE_NUM_MIN),
					 TILE_NUM_MAX);
			tmp = ((tile_num_y * frame_width / frame_height) / 2) * 2;
			tile_num_x = MIN(MAX(tmp, TILE_NUM_MIN), max_tile_col);

			tile_width  = frame_width  / (2 * tile_num_x) * 2;
			tile_height = frame_height / (2 * tile_num_y) * 2;
		}
	}
	else
	{
		tile_num_x = param_histo->tile_num_x;
		tile_num_y = param_histo->tile_num_y;
		tile_width = frame_width / (2 * tile_num_x) * 2;
		tile_height = frame_height / (2 * tile_num_y) * 2;
	}

	cropRows = frame_height - tile_height * tile_num_y;
	cropCols = frame_width - tile_width * tile_num_x;
	cropUp = cropRows >> 1;
	cropDown = cropRows >> 1;
	cropLeft = cropCols >> 1;
	cropRight = cropCols >> 1;

	clipLimit_min = tile_width * tile_height >> BIN_NUM_BIT;
	clipLimit = clipLimit_min * strength >> 2;

	//update patameters
	param_histo->cropUp   = cropUp;
	param_histo->cropDown = cropDown;
	param_histo->cropLeft = cropLeft;
	param_histo->cropRight = cropRight;
	param_histo->cropRows  = cropRows;
	param_histo->cropCols  = cropCols;
	param_histo->tile_width   = tile_width;
	param_histo->tile_height  = tile_height;
	param_histo->frame_width  = frame_width;
	param_histo->frame_height = frame_height;
	param_histo->clipLimit     = clipLimit;
	param_histo->clipLimit_min = clipLimit_min;
//	param_histo->binning_en = binning_factor;
	param_histo->tile_num_x = tile_num_x;
	param_histo->tile_num_y = tile_num_y;
	param_histo->tile_size  = tile_width * tile_height;

	return 0;
}

static void ltm_rgb_map_dump_data_rtl(ltm_param_t *param_map,
			       uint32_t *img_info,
			       ltm_map_rtl_t *param_map_rtl)
{
	int temp;
	int tile_index_xs, tile_index_xe;
	int tile_index_ys, tile_index_ye;
	int tile_1st_xs, tile_1st_ys;
	uint8_t tile_x_num_out, tile_y_num_out;
	uint8_t tile_left_flag=0, tile_right_flag=0;
	uint16_t tile0_start_x, tile0_start_y, tileX_start_x;
	uint16_t tile1_start_x, tile1_start_y;
	int img_tile1_xs_offset, img_tile1_ys_offset;

	// slice infomation
	int img_start_x = img_info[0];
	int img_start_y = img_info[1];
	int img_end_x	= img_info[2];
	int img_end_y	= img_info[3];

	// frame infomation
	uint8_t cropUp    = param_map->cropUp;
//	uint8_t cropDown  = param_map->cropDown;
	uint8_t cropLeft  = param_map->cropLeft;
//	uint8_t cropRight = param_map->cropRight;
	uint8_t tile_num_x = param_map->tile_num_x;
	uint8_t tile_num_y = param_map->tile_num_y;
	uint16_t tile_width  = param_map->tile_width;
	uint16_t tile_height = param_map->tile_height;

	tile_index_xs = (img_start_x + tile_width/2 - cropLeft)/tile_width - 1;
	if(tile_index_xs < 0) tile_index_xs = 0;
	tile_index_xe = (img_end_x + tile_width/2 - cropLeft)/tile_width;
	if(tile_index_xe > tile_num_x-1) tile_index_xe = tile_num_x-1;
	tile_x_num_out = tile_index_xe - tile_index_xs + 1;

	tile_index_ys = (img_start_y + tile_height/2 - cropUp)/tile_height - 1;
	if(tile_index_ys < 0) tile_index_ys = 0;
	tile_index_ye = (img_end_y + tile_height/2 - cropUp)/tile_height;
	if(tile_index_ye > tile_num_y-1) tile_index_ye = tile_num_y-1;
	tile_y_num_out = tile_index_ye - tile_index_ys + 1;

	tile_1st_xs = (img_start_x-cropLeft)/tile_width;
	if(tile_1st_xs < 0) tile_1st_xs = 0;
	if(tile_1st_xs > tile_num_x-1) tile_1st_xs = tile_num_x-1;
	pr_debug("img_start_x[%d], cropLeft[%d], tile_width[%d], tile_1st_xs[%d]\n",
		img_start_x, cropLeft, tile_width, tile_1st_xs);

	tile_1st_ys = (img_start_y-cropUp)/tile_height;
	if(tile_1st_ys < 0) tile_1st_ys = 0;
	if(tile_1st_ys > tile_num_y-1) tile_1st_ys = tile_num_y-1;
	pr_debug("img_start_y[%d], cropUp[%d], tile_height[%d], tile_1st_ys[%d]\n",
		img_start_y, cropUp, tile_height, tile_1st_ys);

	tile1_start_x = tile_1st_xs * tile_width + cropLeft;
	tile1_start_y = tile_1st_ys * tile_height + cropUp;
	img_tile1_xs_offset = img_start_x - tile1_start_x;
	img_tile1_ys_offset = img_start_y - tile1_start_y;
	pr_debug("tile_1st_xs[%d], cropLeft[%d], tile_width[%d], img_start_x[%d], tile1_start_x[%d], img_tile1_xs_offset[%d]\n",
		tile_1st_xs, cropLeft, tile_width, img_start_x, tile1_start_x, img_tile1_xs_offset);

	tile0_start_x = tile_index_xs * tile_width + cropLeft;
	tile0_start_y = tile_index_ys * tile_height + cropUp;

	tileX_start_x = tile_index_xe * tile_width + cropLeft;
	temp = img_start_x - (int)tile0_start_x;
	if ((temp >= tile_width)&&(temp < tile_width*3/2)) tile_left_flag = 1;
	temp = (int)tileX_start_x - img_end_x;
	if ((temp > 0)&&(temp <= tile_width/2)) tile_right_flag = 1;

	//output parameters for rtl
	param_map_rtl->tile_index_xs = tile_index_xs;
	param_map_rtl->tile_index_ys = tile_index_ys;
	param_map_rtl->tile_index_xe = tile_index_xe;
	param_map_rtl->tile_index_ye = tile_index_ye;
	param_map_rtl->tile_x_num_rtl = tile_x_num_out - 1;
	param_map_rtl->tile_y_num_rtl = tile_y_num_out - 1;
	param_map_rtl->tile_width_rtl = tile_width;
	param_map_rtl->tile_height_rtl = tile_height;
	param_map_rtl->tile_size_pro_rtl = tile_width * tile_height;
	param_map_rtl->tile_start_x_rtl  = img_tile1_xs_offset;
	param_map_rtl->tile_start_y_rtl  = img_tile1_ys_offset;
	param_map_rtl->tile_left_flag_rtl  = tile_left_flag;
	param_map_rtl->tile_right_flag_rtl = tile_right_flag;

	return;
}


static int isp_ltm_gen_histo_config(struct isp_ltm_ctx_desc *ctx,
			     struct isp_dev_ltm_stat_info *tuning)
{
	int ret = 0;
	int idx = 0;
	struct isp_ltm_hist_param hist_param;
	struct isp_ltm_hist_param *param = &hist_param;
	struct isp_ltm_hists *hists = &ctx->hists;

	param->bypass           = tuning->bypass;
	if (param->bypass)
	{
		hists->bypass = 1;
		return 0;
	}

	param->strength         = tuning->strength;
	param->region_est_en    = tuning->region_est_en;
	param->text_point_thres = tuning->text_point_thres;
	param->text_proportion  = tuning->text_proportion;
	param->tile_num_auto    = tuning->tile_num_auto;
	param->tile_num_x       = tuning->tile_num.tile_num_x;
	param->tile_num_y       = tuning->tile_num.tile_num_y;
	param->frame_height	= ctx->frame_height;
	param->frame_width	= ctx->frame_width;

	ltm_calc_histo_param(param);

	idx = ctx->frame_idx % ISP_LTM_BUF_NUM;

	hists->bypass		= param->bypass;
	hists->binning_en	= param->binning_en;
	hists->region_est_en	= param->region_est_en;
	hists->buf_full_mode	= 0;
	hists->roi_start_x	= param->cropLeft;
	hists->roi_start_y	= param->cropUp;
	hists->tile_width	= param->tile_width;
	hists->tile_num_x_minus = param->tile_num_x - 1;
	hists->tile_height	= param->tile_height;
	hists->tile_num_y_minus = param->tile_num_y - 1;
	hists->clip_limit	= param->clipLimit;
	hists->clip_limit_min	= param->clipLimit_min;
	hists->texture_proportion = param->text_proportion;
	hists->text_point_thres = param->text_point_thres;
	hists->addr		= ctx->pbuf[idx]->iova[0];
	hists->pitch		= param->tile_num_x - 1;
	hists->wr_num		= param->tile_num_x * 32;

	ctx->frame_width_stat	= param->frame_width;
	ctx->frame_height_stat	= param->frame_height;

	pr_debug("binning_en[%d], tile_num_x_minus[%d], tile_num_y_minus[%d]\n",
		hists->binning_en,
		hists->tile_num_x_minus,
		hists->tile_num_y_minus);
	pr_debug("tile_height[%d], tile_width[%d], clip_limit_min[%d], clip_limit[%d]\n",
		hists->tile_height, hists->tile_width,
		hists->clip_limit_min, hists->clip_limit);
	pr_debug("idx[%d], roi_start_x[%d], roi_start_y[%d], addr[0x%lx]\n",
		ctx->frame_idx, hists->roi_start_x,
		hists->roi_start_y, hists->addr);

	return ret;
}


static int isp_ltm_gen_map_config(struct isp_ltm_ctx_desc *ctx,
				  struct isp_dev_ltm_map_info *tuning)
{
	int idx = 0;

	struct isp_ltm_hist_param map_param;
	struct isp_ltm_hist_param *param = &map_param;
	struct isp_ltm_rtl_param  rtl_param;
	struct isp_ltm_rtl_param  *prtl = &rtl_param;

	struct isp_ltm_hists *hists = &ctx->hists;
	struct isp_ltm_map   *map   = &ctx->map;

	struct isp_ltm_tile_num_minus1 mnum;
	struct isp_ltm_tile_size ts;
	struct isp_ltm_tile_size tm;

	uint32_t frame_width_stat, frame_height_stat;
	uint32_t frame_width_map,  frame_height_map;

	uint32_t ratio = 0;
	uint32_t crop_cols_curr, crop_rows_curr;
	uint32_t crop_up_curr,   crop_down_curr;
	uint32_t crop_left_curr, crop_right_curr;

	uint32_t slice_info[4];

	mnum.tile_num_x = hists->tile_num_x_minus + 1;
	mnum.tile_num_y = hists->tile_num_y_minus + 1;
	ts.tile_width	   = hists->tile_width;
	ts.tile_height     = hists->tile_height;
	frame_width_stat   = ctx->frame_width_stat;
	frame_height_stat  = ctx->frame_height_stat;
	frame_width_map    = ctx->frame_width;
	frame_height_map   = ctx->frame_height;

	if (ctx->type == MODE_LTM_CONTINUE) {
		pr_debug("tile_num_x[%d], tile_num_y[%d], tile_width[%d], tile_height[%d],\
			frame_width_stat[%d], frame_height_stat[%d],\
			frame_width_map[%d], frame_height_map[%d]\n",
			mnum.tile_num_x, mnum.tile_num_y,
			ts.tile_width, ts.tile_height,
			frame_width_stat, frame_height_stat,
			frame_width_map, frame_height_map);
	}

	/*
	 * frame_width_map/frame_width_stat should be
	 * equal to frame_height_map/frame_height_stat
	 * */
	ratio = (frame_width_map << 7) / frame_width_stat;

	tm.tile_width  = (ratio * ts.tile_width  + 128) >> 8 << 1;
	tm.tile_height = (ratio * ts.tile_height + 128) >> 8 << 1;

	crop_cols_curr = frame_width_map  - tm.tile_width  * mnum.tile_num_x;
	crop_rows_curr = frame_height_map - tm.tile_height * mnum.tile_num_y;
	crop_up_curr   = crop_rows_curr >> 1;
	crop_down_curr = crop_rows_curr >> 1;
	crop_left_curr = crop_cols_curr >> 1;
	crop_right_curr = crop_cols_curr >> 1;

	/* update parameters */
	param->cropUp	 = crop_up_curr;
	param->cropDown  = crop_down_curr;
	param->cropLeft  = crop_left_curr;
	param->cropRight = crop_right_curr;
	param->cropCols  = crop_cols_curr;
	param->cropRows  = crop_rows_curr;

	param->tile_num_x   = mnum.tile_num_x;
	param->tile_num_y   = mnum.tile_num_y;
	param->tile_width   = tm.tile_width;
	param->tile_height  = tm.tile_height;
	param->frame_width  = frame_width_map;
	param->frame_height = frame_height_map;
	param->tile_size    = tm.tile_width * tm.tile_height;

	slice_info[0] = 0;
	slice_info[1] = 0;
	slice_info[2] = frame_width_map - 1;
	slice_info[3] = frame_height_map - 1;

	ltm_rgb_map_dump_data_rtl(param, slice_info, prtl);
/*
	map->tile_left_flag = prtl->tile_left_flag_rtl;
	map->tile_right_flag = prtl->tile_right_flag_rtl;
	map->tile_x_num = prtl->tile_x_num_rtl;
	map->tile_y_num = prtl->tile_y_num_rtl;
	map->tile_start_x = prtl->tile_start_x_rtl;
	map->tile_start_y = prtl->tile_start_y_rtl;
*/
	if (ctx->frame_idx)
		map->bypass = 0;
	else
		map->bypass = 1;
	/*
	 * burst8_en : 0 ~ burst8; 1 ~ burst16
	 */
	map->burst8_en = 0;
	/*
	 * map auto bypass if hist error happen
	 */
	map->hist_error_en = 0;
	/*
	 * wait_en & wait_line
	 * fetch data raw, rgb: set 0
	 * fetch data yuv     : set 1
	 */
	map->fetch_wait_en   = 0;
	map->fetch_wait_line = 0;

	map->tile_width	     = tm.tile_width;
	map->tile_height     = tm.tile_height;
	map->tile_x_num	     = prtl->tile_x_num_rtl;
	map->tile_y_num      = prtl->tile_y_num_rtl;
	map->tile_size_pro   = tm.tile_width * tm.tile_height;
	map->tile_start_x    = prtl->tile_start_x_rtl;
	map->tile_left_flag  = prtl->tile_left_flag_rtl;
	map->tile_start_y    = prtl->tile_start_y_rtl;
	map->tile_right_flag = prtl->tile_right_flag_rtl;
	map->hist_pitch      = mnum.tile_num_x - 1 ;

	idx = ctx->frame_idx % ISP_LTM_BUF_NUM;

	if (tuning->ltm_map_video_mode) {
		if (idx == 0)
			idx = ISP_LTM_BUF_NUM;
		map->mem_init_addr = ctx->pbuf[idx - 1]->iova[0];
	} else {
		map->mem_init_addr = ctx->pbuf[idx]->iova[0];
	}

	pr_debug("tile_width[%d], tile_height[%d], tile_x_num[%d], tile_y_num[%d]\n",
		map->tile_width, map->tile_height,
		map->tile_x_num, map->tile_y_num);
	pr_debug("tile_size_pro[%d], tile_start_x[%d], tile_left_flag[%d]\n",
		map->tile_size_pro, map->tile_start_x, map->tile_left_flag);
	pr_debug("tile_start_y[%d], tile_right_flag[%d], hist_pitch[%d]\n",
		map->tile_start_y, map->tile_right_flag, map->hist_pitch);
	pr_debug("idx[%d], addr[0x%lx]\n",
		ctx->frame_idx, map->mem_init_addr);

	return 0;
}


/*
 * external function interface
 *
 */

int isp_ltm_gen_map_slice_config(struct isp_ltm_ctx_desc *ctx,
				 struct isp_ltm_rtl_param  *prtl,
				 uint32_t *slice_info)
{
	struct isp_ltm_hist_param map_param;
	struct isp_ltm_hist_param *param = &map_param;
/*
	struct isp_ltm_rtl_param  rtl_param;
	struct isp_ltm_rtl_param  *prtl = &rtl_param;
*/
	struct isp_ltm_hists *hists = &ctx->hists;

	struct isp_ltm_tile_num_minus1 mnum;
	struct isp_ltm_tile_size ts;
	struct isp_ltm_tile_size tm;

	uint32_t frame_width_stat, frame_height_stat;
	uint32_t frame_width_map,  frame_height_map;

	uint32_t ratio = 0;
	uint32_t crop_cols_curr, crop_rows_curr;
	uint32_t crop_up_curr,   crop_down_curr;
	uint32_t crop_left_curr, crop_right_curr;

	mnum.tile_num_x = hists->tile_num_x_minus + 1;
	mnum.tile_num_y = hists->tile_num_y_minus + 1;
	ts.tile_width	   = hists->tile_width;
	ts.tile_height     = hists->tile_height;
	frame_width_stat   = ctx->frame_width_stat;
	frame_height_stat  = ctx->frame_height_stat;
	frame_width_map    = ctx->frame_width;
	frame_height_map   = ctx->frame_height;

	if (ctx->type == MODE_LTM_CONTINUE) {
		pr_debug("tile_num_x[%d], tile_num_y[%d], tile_width[%d], tile_height[%d],\
			frame_width_stat[%d], frame_height_stat[%d],\
			frame_width_map[%d], frame_height_map[%d]\n",
			mnum.tile_num_x, mnum.tile_num_y,
			ts.tile_width, ts.tile_height,
			frame_width_stat, frame_height_stat,
			frame_width_map, frame_height_map);
	}

	/*
	 * frame_width_map/frame_width_stat should be
	 * equal to frame_height_map/frame_height_stat
	 */
	ratio = (frame_width_map << 7) / frame_width_stat;

	tm.tile_width  = (ratio * ts.tile_width  + 128) >> 8 << 1;
	tm.tile_height = (ratio * ts.tile_height + 128) >> 8 << 1;

	crop_cols_curr = frame_width_map  - tm.tile_width  * mnum.tile_num_x;
	crop_rows_curr = frame_height_map - tm.tile_height * mnum.tile_num_y;
	crop_up_curr   = crop_rows_curr >> 1;
	crop_down_curr = crop_rows_curr >> 1;
	crop_left_curr = crop_cols_curr >> 1;
	crop_right_curr = crop_cols_curr >> 1;

	/* update parameters */
	param->cropUp	 = crop_up_curr;
	param->cropDown  = crop_down_curr;
	param->cropLeft  = crop_left_curr;
	param->cropRight = crop_right_curr;
	param->cropCols  = crop_cols_curr;
	param->cropRows  = crop_rows_curr;

	param->tile_num_x   = mnum.tile_num_x;
	param->tile_num_y   = mnum.tile_num_y;
	param->tile_width   = tm.tile_width;
	param->tile_height  = tm.tile_height;
	param->frame_width  = frame_width_map;
	param->frame_height = frame_height_map;
	param->tile_size    = tm.tile_width * tm.tile_height;

	ltm_rgb_map_dump_data_rtl(param, slice_info, prtl);

	return 0;
}


int isp_ltm_gen_frame_config(struct isp_ltm_ctx_desc *ctx)
{
	int ret = 0;
	int i = 0;
	struct isp_dev_ltm_info *ltm_info = NULL;

	pr_debug("type[%d], frame_idx[%d], frame_width[%d], frame_height[%d]\n",
		ctx->type, ctx->frame_idx, ctx->frame_width, ctx->frame_height);

	for (i = 0; i < ISP_LTM_BUF_NUM; i++) {
		pr_debug("ctx->pbuf[%d] =  0x%p, 0x%lx\n",
			 i, ctx->pbuf[i], ctx->pbuf[i]->iova[0]);
	}

	switch (ctx->type) {
	case MODE_LTM_CONTINUE_OUT:
		ltm_info = isp_ltm_get_tuning_config(ISP_PRO_LTM_PRE_PARAM);
		isp_ltm_gen_histo_config(ctx, &ltm_info->ltm_stat);
		isp_ltm_gen_map_config(ctx, &ltm_info->ltm_map);
		isp_ltm_config_param(ctx);
		break;
	case MODE_LTM_CONTINUE:
		ltm_info = isp_ltm_get_tuning_config(ISP_PRO_LTM_CAP_PARAM);
		isp_ltm_gen_histo_config(ctx, &ltm_info->ltm_stat);
		isp_ltm_gen_map_config(ctx, &ltm_info->ltm_map);
		isp_ltm_config_param(ctx);
		break;
	case MODE_LTM_OFF:
		ctx->bypass = 1;
		isp_ltm_config_param(ctx);
		break;
	default:
		ctx->bypass = 1;
		isp_ltm_config_param(ctx);
		break;
	}

	return ret;
}
