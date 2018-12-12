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

#ifndef _DCAM_INTERFACE_H_
#define _DCAM_INTERFACE_H_

#include <linux/platform_device.h>

#include "cam_hw.h"
#include "cam_types.h"


#define DCAM_SCALE_DOWN_MAX		4

/*
 * Supported dcam_if index. Number 0&1 for dcam_if and 2 for dcam_if_lite.
 */
enum dcam_id {
	DCAM_ID_0 = 0,
	DCAM_ID_1,
	DCAM_ID_2,
	DCAM_ID_MAX,
};

/*
 * Quick function to check is @idx valid.
 */
#define is_dcam_id(idx) ((idx) >= DCAM_ID_0 && (idx) < DCAM_ID_MAX)

/*
 * Enumerating output paths in dcam_if device.
 *
 * @DCAM_PATH_FULL: with biggest line buffer, full path is often used as capture
 *                  path. Crop function available on this path.
 * @DCAM_PATH_BIN:  bin path is used as preview path. Crop and scale function
 *                  available on this path.
 * @DCAM_PATH_PDAF: this path is used to receive PDAF data
 * @DCAM_PATH_VCH2: can receive data according to data_type or
 *                  virtual_channel_id in a MIPI packet
 * @DCAM_PATH_VCH3: receive all data left
 * @DCAM_PATH_AEM:  output exposure by blocks
 * @DCAM_PATH_AFM:  output focus related data
 * @DCAM_PATH_AFL:  output anti-flicker data, including global data and region
 *                  data
 * @DCAM_PATH_HIST: output bayer histogram data in RGB channel
 * @DCAM_PATH_3DNR: output noise reduction data
 * @DCAM_PATH_BPC:  output bad pixel data
 */
enum dcam_path_id {
	DCAM_PATH_FULL = 0,
	DCAM_PATH_BIN,
	DCAM_PATH_PDAF,
	DCAM_PATH_VCH2,
	DCAM_PATH_VCH3,
	DCAM_PATH_AEM,
	DCAM_PATH_AFM,
	DCAM_PATH_AFL,
	DCAM_PATH_HIST,
	DCAM_PATH_3DNR,
	DCAM_PATH_BPC,
	DCAM_PATH_MAX,
};

/*
 * Quick function to check is @id valid.
 */
#define is_path_id(id) ((id) >= DCAM_PATH_FULL && (id) < DCAM_PATH_MAX)

enum dcam_path_cfg_cmd {
	DCAM_PATH_CFG_BASE = 0,
	DCAM_PATH_CFG_OUTPUT_BUF,
	DCAM_PATH_CFG_OUTPUT_RESERVED_BUF,
	DCAM_PATH_CFG_SIZE,
};


enum dcam_ioctrl_cmd {
	DCAM_IOCTL_CFG_CAP,
	DCAM_IOCTL_CFG_STATIS_BUF,
	DCAM_IOCTL_CFG_START,
	DCAM_IOCTL_INIT_STATIS_Q,
	DCAM_IOCTL_DEINIT_STATIS_Q,
	DCAM_IOCTL_CFG_PDAF,
};


struct dcam_cap_cfg {
	uint32_t sensor_if; /* MIPI CSI-2 */
	uint32_t format; /* input color format */
	uint32_t mode; /* single or multi mode. */
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


struct dcam_path_cfg_param {
	uint32_t enable_slowmotion;
	uint32_t slowmotion_count;
	uint32_t enable_3dnr;
	uint32_t is_raw;
	uint32_t is_loose;
	uint32_t frm_deci;
	uint32_t frm_skip;
	uint32_t force_rds;
	void *priv_size_data;
	struct img_endian endian;
	struct img_size input_size;
	struct img_trim input_trim;
	struct img_size output_size;
};

/*
 * supported operations for dcam_if device
 *
 * @open:         initialize software and hardware resource for dcam_if
 * @close:        uninitialize resource for dcam_if
 * @start:        configure MIPI parameters and enable capture, parameters
 *                must be updated by ioctl() before this call
 * @stop:         // TODO: fill this
 * @get_path:
 * @put_path:
 * @cfg_path:
 * @ioctl:
 * @proc_frame:
 * @set_callback:
 * @update_clk:
 *
 */
struct dcam_pipe_ops {
	int (*open)(void *handle);
	int (*close)(void *handle);
	int (*reset)(void *handle);
	int (*start)(void *handle);
	int (*stop)(void *handle);
	int (*get_path)(void *handle, int path_id);
	int (*put_path)(void *handle, int path_id);
	int (*cfg_path)(void *dcam_handle,
				enum dcam_path_cfg_cmd cfg_cmd,
				int path_id, void *param);
	int (*ioctl)(void *handle, enum dcam_ioctrl_cmd cmd, void *arg);
	int (*cfg_blk_param)(void *handle, void *param);
	int (*proc_frame)(void *handle, void *param);
	int (*set_callback)(void *handle,
			dcam_dev_callback cb, void *priv_data);
	int (*update_clk)(void *handle, void *arg);
};

/*
 * A nr3_me_data object carries motion vector and settings. Downstream module
 * who peforms noice reduction operation uses these information to calculate
 * correct motion vector for target image size.
 *
 * *****Note: target image should not be cropped*****
 *
 * @valid:         valid bit
 * @sub_me_bypass: sub_me_bypass bit, this has sth to do with mv calculation
 * @project_mode:  project_mode bit, 0 for step, 1 for interlace
 * @mv_x:          motion vector in x direction
 * @mv_y:          motion vector in y direction
 * @src_width:     source image width
 * @src_height:    source image height
 */
struct nr3_me_data {
	uint32_t valid:1;
	uint32_t sub_me_bypass:1;
	uint32_t project_mode:1;
	s8 mv_x;
	s8 mv_y;
	uint32_t src_width;
	uint32_t src_height;
};

/*
 * Use dcam_frame_synchronizer to synchronize frames between different paths.
 * Normally we set WADDR for all paths in CAP_SOF interrupt, which is the best
 * opportunity that we bind buffers together and set frame number for them.
 * Modules such as 3DNR or LTM need to know the precise relationship between
 * bin/full buffer and statis buffer.
 * Some data which can be read directly from register is also included in this
 * object.
 * After data in image or statis buffer consumed, consumer modules must call
 * dcam_if_release_sync() to notify dcam_if.
 *
 * @index:             frame index tracked by dcam_if
 * @valid:             camera_frame valid bit for each path
 * @frames:            pointers to frames from each path
 *
 * @nr3_me:            nr3 data
 */
struct dcam_frame_synchronizer {
	uint32_t index;
	uint32_t valid;
	struct camera_frame *frames[DCAM_PATH_MAX];

	struct nr3_me_data nr3_me;
};

/*
 * Enables/Disables frame sync for path_id. Should be called before streaming.
 */
int dcam_if_set_sync_enable(void *handle, int path_id, int enable);
/*
 * Release frame sync reference for @frame thus dcam_frame_synchronizer data
 * can be recycled for next use.
 */
int dcam_if_release_sync(struct dcam_frame_synchronizer *sync,
			 struct camera_frame *frame);
/*
 * Test if frame data is valid for path_id. Data will be valid only after
 * TX_DONE interrupt.
 */
#define dcam_if_is_sync_valid(sync, path_id) (sync->valid & BIT(path_id))
/*
 * Retrieve dcam_if supported operations.
 */
struct dcam_pipe_ops *dcam_if_get_ops(void);
/*
 * Retrieve a dcam_if device for the hardware. A dcam_if device is a wrapper
 * with supported operations defined in dcam_pipe_ops.
 */
void *dcam_if_get_dev(uint32_t idx, struct sprd_cam_hw_info *hw);
/*
 * Release a dcam_if device after capture finished.
 */
int dcam_if_put_dev(void *dcam_handle);
/* set lbuf share mode for dcam0,dcam1, set before stream on */
int dcam_lbuf_share_mode(enum dcam_id idx, uint32_t width);
/*
 * Retrieve hardware info from dt.
 */
int dcam_if_parse_dt(struct platform_device *pdev,
			struct sprd_cam_hw_info **dcam_hw,
			uint32_t *dcam_count);
int sprd_dcam_debugfs_init(void);
int sprd_dcam_debugfs_deinit(void);

#endif /* _DCAM_INTERFACE_H_ */
