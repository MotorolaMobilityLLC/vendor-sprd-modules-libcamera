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

#ifndef _ISP_LTM_H_
#define _ISP_LTM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "cam_queue.h"
#include "dcam_interface.h"
#include "isp_interface.h"

/*
 * tuning struct, will be move to isp_r8p1.h
 */
struct isp_ltm_tile_num_minus1 {
	uint32_t tile_num_x;
	uint32_t tile_num_y;
};

struct isp_ltm_tile_size {
	uint32_t tile_width;
	uint32_t tile_height;
};

struct isp_ltm_clip_limit {
	uint32_t limit;
	uint32_t limit_min;
};

struct isp_dev_ltm_stat_info {
	uint32_t bypass; /* bypass */

	struct isp_ltm_tile_num_minus1 tile_num;
	struct isp_ltm_tile_size tile_size;
	struct isp_ltm_clip_limit tile_clip;

	uint32_t strength;
	uint32_t tile_num_auto;

	uint32_t text_point_thres; /* text_point_thres */
	uint32_t text_proportion; /* texture_proportion */
	uint32_t region_est_en; /* region_est_en */
	uint32_t binning_en;
};

struct isp_dev_ltm_map_info {
	uint32_t bypass; /* ltm map bypass */
	uint32_t ltm_map_video_mode;
};

struct isp_dev_ltm_info {
	struct isp_dev_ltm_stat_info ltm_stat;
	struct isp_dev_ltm_map_info ltm_map;
};
/*
 * tuning struct end
 */

typedef struct isp_ltm_hist_param
{
	/* match ltm stat info */
	uint32_t bypass;
	uint32_t region_est_en;
	uint32_t text_point_thres;
	uint32_t text_proportion;

	/* input */
	uint32_t strength;
	uint32_t tile_num_auto;

	/* output / input */
	uint32_t tile_num_x;
	uint32_t tile_num_y;

	/* output */
	uint32_t tile_width;
	uint32_t tile_height;
	uint32_t tile_size;

	uint32_t frame_width;
	uint32_t frame_height;
	uint32_t clipLimit;
	uint32_t clipLimit_min;
	uint32_t binning_en;

	uint32_t cropUp;
	uint32_t cropDown;
	uint32_t cropLeft;
	uint32_t cropRight;
	uint32_t cropRows;
	uint32_t cropCols;
} ltm_param_t;

typedef struct isp_ltm_map_param
{
	/* input, from ltm hists
	 * uint32_t tile_num_x;
	 * uint32_t tile_num_y;
	 * uint32_t tile_width_stat;
	 * uint32_t tile_height_stat;
	 * uint32_t frame_width_stat;
	 * uint32_t frame_height_stat;
	 */

	/* input & output, frame size */
	uint32_t frame_width;
	uint32_t frame_height;

	/* output
	 * uint32_t cropUp;
	 * uint32_t cropDown;
	 * uint32_t cropLeft;
	 * uint32_t cropRight;
	 * uint32_t cropCols;
	 * uint32_t cropRows;
	 */

	uint32_t tile_width;
	uint32_t tile_height;
	uint32_t tile_size_map;
} ltm_map_param_t;

typedef struct isp_ltm_rtl_param
{
	int tile_index_xs;
	int tile_index_ys;
	int tile_index_xe;
	int tile_index_ye;
	uint32_t tile_x_num_rtl;
	uint32_t tile_y_num_rtl;
	uint32_t tile_width_rtl;
	uint32_t tile_height_rtl;
	uint32_t tile_size_pro_rtl;
	uint32_t tile_start_x_rtl;
	uint32_t tile_start_y_rtl;
	uint32_t tile_left_flag_rtl;
	uint32_t tile_right_flag_rtl;
} ltm_map_rtl_t;

/*
 * DON'T CHANGE
 * LTM register, copy from SPEC.
 *
 */
enum ltm_func_type {
	LTM_FUNC_PRE,
	LTM_FUNC_VID,
	LTM_FUNC_CAP,
	LTM_FUNC_MAX
};

struct isp_ltm_hists {
	/* ISP_LTM_PARAMETERS 0x0010 */
	uint32_t bypass;
	uint32_t binning_en;
	uint32_t region_est_en;
	uint32_t buf_full_mode;

	/* ISP_LTM_ROI_START 0x0014 */
	uint32_t roi_start_x;
	uint32_t roi_start_y;

	/* ISP_LTM_TILE_RANGE 0x0018 */
	uint32_t tile_width;
	uint32_t tile_num_x_minus;
	uint32_t tile_height;
	uint32_t tile_num_y_minus;

	/* ISP_LTM_CLIP_LIMIT 0x0020 */
	uint32_t clip_limit;
	uint32_t clip_limit_min;

	/* ISP_LTM_THRES 0x0024 */
	uint32_t texture_proportion;
	uint32_t text_point_thres;

	/* ISP_LTM_ADDR 0x0028 */
	unsigned long addr;

	/* ISP_LTM_PITCH 0x002C */
	uint32_t pitch;
	uint32_t wr_num;
};

struct isp_ltm_map {
	/* ISP_LTM_MAP_PARAM0 0x0010 */
	uint32_t bypass;
	uint32_t burst8_en;
	uint32_t hist_error_en;
	uint32_t fetch_wait_en;
	uint32_t fetch_wait_line;

	/* ISP_LTM_MAP_PARAM1 0x0014 */
	uint32_t tile_width;
	uint32_t tile_height;
	uint32_t tile_x_num;
	uint32_t tile_y_num;

	/* ISP_LTM_MAP_PARAM2 0x0018 */
	uint32_t tile_size_pro; /* ltmap_tile_width x ltmap_tile_height */

	/* ISP_LTM_MAP_PARAM3 0x001c */
	uint32_t tile_start_x;
	uint32_t tile_left_flag;
	uint32_t tile_start_y;
	uint32_t tile_right_flag;

	/* ISP_LTM_MAP_PARAM4 0x0020 */
	unsigned long mem_init_addr;

	/* ISP_LTM_MAP_PARAM5 0x0024 */
	uint32_t hist_pitch;
};

struct isp_ltm_ctx_desc {
	uint32_t bypass;
	uint32_t type;
	uint32_t isp_pipe_ctx_id;
	uint32_t wait_completion;

	/*
	 * preview and capture frame has UNIFY and UNIQ frame ID
	 * match frame of preview and capture
	 */
	uint32_t frame_idx;
	/*
	 * Origion frame size
	 */
	uint32_t frame_width;
	uint32_t frame_height;
	/*
	 * Histo calc frame size, binning
	 */
	uint32_t frame_width_stat;
	uint32_t frame_height_stat;

	struct isp_ltm_hists hists;
	struct isp_ltm_map   map;

	struct camera_buf *pbuf[ISP_LTM_BUF_NUM];
};


/*
 * EXPORT function interface
 */
int isp_ltm_gen_frame_config(struct isp_ltm_ctx_desc *ctx);
int isp_ltm_gen_map_slice_config(struct isp_ltm_ctx_desc *ctx,
			struct isp_ltm_rtl_param  *prtl,
			uint32_t *slice_info);

struct isp_dev_ltm_info *isp_ltm_get_tuning_config(int type);
int isp_ltm_config_param(struct isp_ltm_ctx_desc *ctx);

#ifdef __cplusplus
}
#endif

#endif /* _ISP_LTM_H_ */
