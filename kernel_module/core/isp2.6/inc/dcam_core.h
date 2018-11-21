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

#ifndef _DCAM_CORE_H_
#define _DCAM_CORE_H_

#include "sprd_img.h"
#include <linux/sprd_ion.h>

#include "cam_types.h"
#include "cam_queue.h"
#include "dcam_interface.h"

#define DCAM_IN_Q_LEN  1
#define DCAM_PROC_Q_LEN  2

/* TODO: extend these for slow motion dev */
#define DCAM_RESULT_Q_LEN 10
#define DCAM_OUT_BUF_Q_LEN 16
#define DCAM_RESERVE_BUF_Q_LEN 6

#define DCAM_INTERNAL_RES_BUF_SIZE  0x40000


// TODO: how many helpers there should be?
#define DCAM_SYNC_HELPER_COUNT 10

/* enable 3dnr here for debug */
//#define NR3_DEV

struct dcam_pipe_dev;



enum dcam_scaler_type {
	DCAM_SCALER_BINNING = 0,
	DCAM_SCALER_RAW_DOWNSISER,
	DCAM_SCALER_BYPASS,
	DCAM_SCALER_MAX,
};


struct statis_path_buf_info {
	enum dcam_path_id path_id;
	size_t buf_size;
	size_t buf_cnt;
};

struct dcam_mipi_info {
	uint32_t sensor_if;  /* MIPI CSI-2 */
	uint32_t format;  /* input color format */
	uint32_t mode;   /* single or multi mode. */
	uint32_t data_bits;
	uint32_t pattern; /* bayer mode for rgb, yuv pattern for yuv */
	uint32_t href;
	uint32_t frm_deci;
	uint32_t frm_skip;
	uint32_t x_factor;
	uint32_t y_factor;
	uint32_t is_4in1;
	struct img_trim cap_size;
};

struct dcam_fetch_info {
	uint32_t is_loose;
	uint32_t endian;
	uint32_t pattern;
	struct img_size size;
	struct img_trim trim;
	struct img_addr addr;
};

struct dcam_path_desc {
	atomic_t user_cnt;
	enum dcam_path_id path_id;

	spinlock_t size_lock;
	uint32_t size_update;
	void *priv_size_data;

	struct img_endian endian;
	struct img_size in_size;
	struct img_trim in_trim;
	struct img_size out_size;

	uint32_t out_fmt;
	uint32_t is_loose;

	/* full path source sel */
	uint32_t src_sel;

	/* for bin path */
	uint32_t is_slw;
	uint32_t slw_frm_num;
	uint32_t bin_ratio;
	uint32_t scaler_sel; /* 0: bining, 1: RDS, 2&3: bypass */
	void *rds_coeff_buf;
	uint32_t rds_coeff_size;

	uint32_t frm_deci;
	uint32_t frm_deci_cnt;

	uint32_t frm_skip;
	uint32_t frm_cnt;

	atomic_t set_frm_cnt;
	struct camera_queue reserved_buf_queue;
	struct camera_queue out_buf_queue;
	struct camera_queue result_queue;
};

/*
 * state machine for DCAM
 *
 * @INIT:             initial state of this dcam_pipe_dev
 * @IDLE:             after hardware initialized, power/clk/int should be
 *                    prepared, which indicates registers are ready for
 *                    accessing
 * @RUNNING:          this dcam_pipe_dev is receiving data from CSI or DDR, and
 *                    writing data from one or more paths
 * @ERROR:            something is wrong, we should reset hardware right now
 */
enum dcam_state {
	STATE_INIT = 0,
	STATE_IDLE = 1,
	STATE_RUNNING = 2,
	STATE_ERROR = 3,
};

/*
 * Wrapper for dcam_frame_synchronizer. This helper uses kernel list to maintain
 * frame synchronizer data. Any operation on list should be protected by
 * helper_lock.
 *
 * @list:    support list operation
 * @sync:    underlaying synchronizer data
 * @enabled: enabled path for this exact frame, when it becomes zero, we should
 *           recycle this sync data
 * @dev:     pointer to the dcam_pipe_dev
 */
struct dcam_sync_helper {
	struct list_head list;
	struct dcam_frame_synchronizer sync;
	uint32_t enabled;
	struct dcam_pipe_dev *dev;
};

/*
 * A dcam_pipe_dev is a digital IP including one input for raw RGB or YUV
 * data, several outputs which usually be called paths. Input data can be
 * frames received from a CSI controller, or pure image data fetched from DDR.
 * Output paths contains a full path, a binning path and other data paths.
 * There may be raw domain processors in IP, each may or may NOT have data
 * output. Those who outputs data to some address in DDR behaves just same
 * as full or binning path. They can be treated as a special type of path.
 *
 * Each IP should implement a dcam_pipe_dev according to features it has. The
 * IP version may change, but basic function of DCAM IP is similar.
 *
 * @idx:              index of this device
 * @irq:              interrupt number for this device
 * @state:            working state of this device
 *
 * @frame_index:      frame index, tracked by CAP_SOF
 * @helper_enabled:   sync enabled path IDs
 * @helper_lock:      this lock protects synchronizer helper related data
 * @helper_list:      a list of sync helpers
 * @helpers:          memory for helpers
 */
struct dcam_pipe_dev {
	uint32_t idx;
	uint32_t irq;
	atomic_t state;// TODO: use mutex to protect

	uint64_t frame_index;
	uint32_t enable_slowmotion;
	uint32_t slowmotion_count;
	uint32_t helper_enabled;
	spinlock_t helper_lock;
	struct list_head helper_list;
	struct dcam_sync_helper helpers[DCAM_SYNC_HELPER_COUNT];

	struct sprd_cam_hw_info *hw;

	uint32_t err_status;// TODO: change to use state

	uint32_t is_4in1;
	uint32_t is_3dnr;
	uint32_t is_pdaf;
	uint32_t pdaf_type;
	uint32_t offline; /* flag: set 1 for 4in1 go through dcam1 bin */

	uint32_t iommu_enable;
	struct dcam_mipi_info cap_info;

	struct camera_buf *statis_buf;
	void *internal_reserved_buf; /* for statis path output */

	dcam_dev_callback dcam_cb_func;
	void *cb_priv_data;

	/* void *blk_lsc_pm_handle;  for lsc */
	struct dcam_dev_param *blk_dcam_pm; /* for dcam context */
	struct dcam_path_desc path[DCAM_PATH_MAX];

	struct dcam_fetch_info fetch;
	struct camera_queue in_queue;
	struct camera_queue proc_queue;
	struct cam_thread_info thread;
};

/*
 * Get an empty dcam_sync_helper. Returns NULL if no empty helper remains.
 */
struct dcam_sync_helper *dcam_get_sync_helper(struct dcam_pipe_dev *dev);
/*
 * Put an empty dcam_sync_helper.
 *
 * In CAP_SOF, when the requested empty dcam_sync_helper finally not being used,
 * it should be returned to circle. This only happens when no buffer is
 * available and all paths are using reserved memory.
 */
void dcam_put_sync_helper(struct dcam_pipe_dev *dev,
			  struct dcam_sync_helper *helper);

#endif
