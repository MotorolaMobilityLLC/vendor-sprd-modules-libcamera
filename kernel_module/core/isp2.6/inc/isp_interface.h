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

#ifndef _ISP_INTERFACE_H_
#define _ISP_INTERFACE_H_

#include <linux/of.h>
#include <linux/platform_device.h>

#include "cam_hw.h"
#include "cam_types.h"


#define ISP_MAX_LINE_WIDTH		2560

#define ISP_NR3_BUF_NUM 2
#define ISP_LTM_BUF_NUM 10

enum isp_context_id {
	ISP_CONTEXT_P0,
	ISP_CONTEXT_C0,
	ISP_CONTEXT_P1,
	ISP_CONTEXT_C1,
	ISP_CONTEXT_NUM
};

enum isp_sub_path_id {
	ISP_SPATH_CP = 0,
	ISP_SPATH_VID,
	ISP_SPATH_FD,
	ISP_SPATH_NUM,
};

enum isp_path_cfg_cmd {
	ISP_PATH_CFG_CTX_BASE,
	ISP_PATH_CFG_CTX_SIZE,
	ISP_PATH_CFG_PATH_BASE,
	ISP_PATH_CFG_PATH_SIZE,
	ISP_PATH_CFG_OUTPUT_BUF,
	ISP_PATH_CFG_OUTPUT_RESERVED_BUF,
	ISP_PATH_CFG_3DNR_BUF,
	ISP_PATH_CFG_LTM_BUF,
};

enum isp_3dnr_mode {
	MODE_3DNR_OFF,
	MODE_3DNR_PRE,
	MODE_3DNR_CAP,
	MODE_3DNR_MAX,
};

enum isp_ltm_mode {
	MODE_LTM_OFF,
	MODE_LTM_PRE,
	MODE_LTM_CAP,
	MODE_LTM_MAX
};

enum isp_offline_param_valid {
	ISP_SRC_SIZE = (1 << 0),
	ISP_PATH0_TRIM = (1 << 1),
	ISP_PATH1_TRIM = (1 << 2),
	ISP_PATH2_TRIM = (1 << 3),
};


struct isp_ctx_base_desc {
	uint32_t mode_3dnr;
	uint32_t mode_ltm;
	uint32_t in_fmt;
	uint32_t bayer_pattern;
	uint32_t fetch_fbd;
};

struct isp_ctx_size_desc {
	struct img_size src;
	struct img_trim crop;
};

struct isp_path_base_desc {
	uint32_t out_fmt;
	struct img_endian endian;
	struct img_size output_size;
};

struct isp_offline_param {
	uint32_t valid;
	struct img_scaler_info src_info;
	struct img_trim trim_path[ISP_SPATH_NUM];
	void *prev;
};

struct isp_pipe_ops {
	int (*open)(void *isp_handle, void *arg);
	int (*close)(void *isp_handle);
	int (*reset)(void *isp_handle, void *arg);

	int (*get_context)(void *isp_handle, void *param);
	int (*put_context)(void *isp_handle, int ctx_id);

	int (*get_path)(void *isp_handle, int ctx_id, int path_id);
	int (*put_path)(void *isp_handle, int ctx_id, int path_id);
	int (*cfg_path)(void *isp_handle, enum isp_path_cfg_cmd cfg_cmd,
				int ctx_id, int path_id, void *param);
	int (*cfg_blk_param)(void *isp_handle, int ctx_id, void *param);
	int (*proc_frame)(void *isp_handle, void *param, int ctx_id);
	int (*set_callback)(void *isp_handle, int ctx_id,
					isp_dev_callback cb, void *priv_data);
	int (*update_clk)(void *isp_handle, void *arg);
};

struct isp_pipe_ops *get_isp_ops(void);

void *get_isp_pipe_dev(void);
int put_isp_pipe_dev(void *isp_handle);

int sprd_isp_debugfs_init(void);

int sprd_isp_parse_dt(struct device_node *dn,
		struct sprd_cam_hw_info **isp_hw,
		uint32_t *isp_count);
#endif
