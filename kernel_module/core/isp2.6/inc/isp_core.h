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

#ifndef _ISP_CORE_H_
#define _ISP_CORE_H_

#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/sprd_ion.h>
#include "sprd_img.h"

#include "cam_types.h"
#include "cam_queue.h"
#include "cam_block.h"
#include "isp_interface.h"
#include "isp_3dnr.h"
#include "isp_ltm.h"

#define ISP_LINE_BUFFER_W		2560

#define ISP_IN_Q_LEN			12
#define ISP_PROC_Q_LEN			12
#define ISP_RESULT_Q_LEN		12
#define ISP_OUT_BUF_Q_LEN		32
#define ISP_RESERVE_BUF_Q_LEN		12


#define ODATA_YUV420			1
#define ODATA_YUV422			0

#define ISP_SC_COEFF_BUF_SIZE		(24 << 10)
#define ISP_SC_COEFF_COEF_SIZE		(1 << 10)
#define ISP_SC_COEFF_TMP_SIZE		(21 << 10)

#define ISP_SC_H_COEF_SIZE		0xC0
#define ISP_SC_V_COEF_SIZE		0x210
#define ISP_SC_V_CHROM_COEF_SIZE	0x210

#define ISP_SC_COEFF_H_NUM		(ISP_SC_H_COEF_SIZE / 6)
#define ISP_SC_COEFF_H_CHROMA_NUM	(ISP_SC_H_COEF_SIZE / 12)
#define ISP_SC_COEFF_V_NUM		(ISP_SC_V_COEF_SIZE / 4)
#define ISP_SC_COEFF_V_CHROMA_NUM	(ISP_SC_V_CHROM_COEF_SIZE / 4)

#define ISP_PIXEL_ALIGN_WIDTH		4
#define ISP_PIXEL_ALIGN_HEIGHT		2


enum isp_work_mode {
	ISP_CFG_MODE,
	ISP_AP_MODE,
	ISP_WM_MAX
};

enum isp_fetch_format {
	ISP_FETCH_YUV422_3FRAME = 0,
	ISP_FETCH_YUYV,
	ISP_FETCH_UYVY,
	ISP_FETCH_YVYU,
	ISP_FETCH_VYUY,
	ISP_FETCH_YUV422_2FRAME,
	ISP_FETCH_YVU422_2FRAME,
	ISP_FETCH_RAW10,
	ISP_FETCH_CSI2_RAW10, /* MIPI RAW10*/
	ISP_FETCH_FULL_RGB10,
	ISP_FETCH_YUV420_2FRAME,
	ISP_FETCH_YVU420_2FRAME,
	ISP_FETCH_FORMAT_MAX
};

enum isp_store_format {
	ISP_STORE_UYVY = 0x00,
	ISP_STORE_YUV422_2FRAME,
	ISP_STORE_YVU422_2FRAME,
	ISP_STORE_YUV422_3FRAME,
	ISP_STORE_YUV420_2FRAME,
	ISP_STORE_YVU420_2FRAME,
	ISP_STORE_YUV420_3FRAME,
	ISP_STORE_FORMAT_MAX
};


enum isp_path_wk_status {
	PATH_STATUS_IDLE,
	PATH_STATUS_READY,
	PATH_STATUS_RUNNING,
};

struct isp_pipe_dev;
struct isp_pipe_context;

struct isp_fetch_info {
	enum isp_fetch_format fetch_fmt;
	struct img_size src;/* source buffer size */
	struct img_trim in_trim;/* support fetch trim */
	struct img_addr addr;
	struct img_addr trim_off;
	struct img_pitch pitch;
	uint32_t mipi_byte_rel_pos;
	uint32_t mipi_word_num;
};


struct isp_regular_info {
	uint32_t regular_mode;
	uint32_t shrink_uv_dn_th;
	uint32_t shrink_uv_up_th;
	uint32_t shrink_y_dn_th;
	uint32_t shrink_y_up_th;
	uint32_t effect_v_th;
	uint32_t effect_u_th;
	uint32_t effect_y_th;
	uint32_t shrink_c_range;
	uint32_t shrink_c_offset;
	uint32_t shrink_y_range;
	uint32_t shrink_y_offset;
};

struct isp_scaler_info {
	uint32_t scaler_bypass;
	uint32_t odata_mode;
	uint32_t scaler_y_ver_tap;
	uint32_t scaler_uv_ver_tap;
	uint32_t scaler_ip_int;
	uint32_t scaler_ip_rmd;
	uint32_t scaler_cip_int;
	uint32_t scaler_cip_rmd;
	uint32_t scaler_factor_in;
	uint32_t scaler_factor_out;
	uint32_t scaler_ver_ip_int;
	uint32_t scaler_ver_ip_rmd;
	uint32_t scaler_ver_cip_int;
	uint32_t scaler_ver_cip_rmd;
	uint32_t scaler_ver_factor_in;
	uint32_t scaler_ver_factor_out;
	uint32_t scaler_out_width;
	uint32_t scaler_out_height;
	uint32_t coeff_buf[ISP_SC_COEFF_BUF_SIZE];
};

struct isp_thumbscaler_info {
	uint32_t scaler_bypass;
	uint32_t odata_mode;

	struct img_deci_info y_deci;
	struct img_deci_info uv_deci;

	struct img_size y_factor_in;
	struct img_size y_factor_out;
	struct img_size uv_factor_in;
	struct img_size uv_factor_out;

	struct img_size src0; /* input image/slice size */

	struct img_trim y_trim;
	struct img_size y_src_after_deci;
	struct img_size y_dst_after_scaler;
	struct img_size y_init_phase;

	struct img_trim uv_trim;
	struct img_size uv_src_after_deci;
	struct img_size uv_dst_after_scaler;
	struct img_size uv_init_phase;
};

struct isp_store_info {
	uint32_t bypass;
	uint32_t endian;
	uint32_t speed_2x;
	uint32_t mirror_en;
	uint32_t max_len_sel;
	uint32_t shadow_clr;
	uint32_t store_res;
	uint32_t rd_ctrl;
	uint32_t shadow_clr_sel;
	uint32_t total_size;
	enum isp_store_format color_fmt; /* output color format */
	struct img_size size;
	struct img_addr addr;
	struct img_pitch pitch;
};


struct slice_cfg_input {
	struct img_size frame_in_size;
	struct img_size *frame_out_size[ISP_SPATH_NUM];
	struct isp_fetch_info *frame_fetch;
	struct isp_store_info *frame_store[ISP_SPATH_NUM];
	struct isp_scaler_info *frame_scaler[ISP_SPATH_NUM];
	struct img_deci_info *frame_deci[ISP_SPATH_NUM];
	struct img_trim *frame_trim0[ISP_SPATH_NUM];
	struct img_trim *frame_trim1[ISP_SPATH_NUM];
	uint32_t nlm_center_x;
	uint32_t nlm_center_y;
	uint32_t ynr_center_x;
	uint32_t ynr_center_y;
	struct isp_3dnr_ctx_desc *nr3_ctx;
	struct isp_ltm_ctx_desc *ltm_ctx;
};

struct isp_path_desc {
	atomic_t user_cnt;
	atomic_t store_cnt;
	enum isp_sub_path_id spath_id;
	uint32_t base_frm_id;
	uint32_t frm_cnt;
	uint32_t skip_pipeline;
	uint32_t uv_sync_v;
	uint32_t frm_deci;
	uint32_t out_fmt; /* forcc */
	uint32_t bind_type;
	uint32_t slave_path_id;
	struct isp_pipe_context *attach_ctx;

	struct isp_regular_info regular_info;
	struct isp_store_info store;
	union {
		struct isp_scaler_info scaler;
		struct isp_thumbscaler_info thumbscaler;
	};
	struct img_deci_info deci;
	struct img_trim in_trim;
	struct img_trim out_trim;
	struct img_size src;
	struct img_size dst;
	struct img_endian data_endian;

	int q_init;
	struct camera_queue reserved_buf_queue;
	/*struct camera_frame *reserved_out_buf;*/
	struct camera_queue out_buf_queue;
	struct camera_queue result_queue;
};


struct isp_pipe_context {
	atomic_t user_cnt;
	uint32_t ctx_id;
	uint32_t in_fmt; /* forcc */
	enum camera_id attach_cam_id;

	uint32_t updated;
	uint32_t mode_3dnr;
	uint32_t mode_ltm;
	uint32_t slw_state;
	uint32_t enable_slowmotion;
	uint32_t slowmotion_count;
	uint32_t dispatch_color;
	uint32_t dispatch_bayer_mode; /* RAWRGB_GR, RAWRGB_Gb, RAWRGB_R... */
	uint32_t fetch_path_sel; /* fbd or normal */
	/* lock ctx/path param(size) updated from zoom */
	struct mutex param_mutex;

	/* original: source image (from dcam/sensor) scaler info.
	 * it will be used to re-caclulate parameters for some blocks,
	 * because user alwas set parameters for sensor size image
	 */
	struct img_scaler_info original;
	struct img_size input_size;
	struct img_trim input_trim;
	struct isp_fetch_info fetch;
	struct isp_path_desc isp_path[ISP_SPATH_NUM];
	struct isp_pipe_dev *dev;
	struct isp_k_block isp_k_param;
	void *slice_ctx;
	void *fmcu_handle;
	struct camera_queue in_queue;
	struct camera_queue proc_queue;

	struct camera_frame *nr3_buf[ISP_NR3_BUF_NUM];
	struct camera_frame *ltm_buf[ISP_LTM_BUF_NUM];
	struct camera_queue ltm_avail_queue;
	struct camera_queue ltm_wr_queue;
	struct cam_thread_info thread;
	struct completion frm_done;

	struct isp_3dnr_ctx_desc nr3_ctx;
	struct isp_ltm_ctx_desc ltm_ctx;

	isp_dev_callback isp_cb_func;
	void *cb_priv_data;
};


struct isp_pipe_dev {
	uint32_t irq_no[2];
	atomic_t user_cnt;
	atomic_t enable;
	struct mutex path_mutex; /* lock ctx/path resource management */
	enum isp_work_mode wmode;
	void *cfg_handle;
	struct isp_ltm_share_ctx_desc *ltm_handle;
	struct isp_pipe_context ctx[ISP_CONTEXT_NUM];
	struct sprd_cam_hw_info *isp_hw;
};
#endif
