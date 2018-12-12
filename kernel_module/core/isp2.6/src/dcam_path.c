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


#include <linux/delay.h>
#include <linux/err.h>
#include <linux/interrupt.h>

#include <sprd_mm.h>

#include "cam_scaler.h"
#include "cam_hw.h"
#include "cam_types.h"
#include "cam_queue.h"
#include "cam_buf.h"
#include "cam_block.h"

#include "dcam_interface.h"
#include "dcam_reg.h"
#include "dcam_int.h"
#include "dcam_core.h"
#include "dcam_path.h"


/* Macro Definitions */
#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt) "DCAM_PATH: %d %d %s : "\
	fmt, current->pid, __LINE__, __func__


/*
 * path name for debug output
 */
static const char *_DCAM_PATH_NAMES[DCAM_PATH_MAX] = {
	[DCAM_PATH_FULL] = "FULL",
	[DCAM_PATH_BIN] = "BIN",
	[DCAM_PATH_PDAF] = "PDAF",
	[DCAM_PATH_VCH2] = "VCH2",
	[DCAM_PATH_VCH3] = "VCH3",
	[DCAM_PATH_AEM] = "AEM",
	[DCAM_PATH_AFM] = "AFM",
	[DCAM_PATH_AFL] = "AFL",
	[DCAM_PATH_HIST] = "HIST",
	[DCAM_PATH_3DNR] = "3DNR",
	[DCAM_PATH_BPC] = "BPC",
};

/*
 * convert @path_id to path name
 */
const char *to_path_name(enum dcam_path_id path_id) {
	return is_path_id(path_id) ? _DCAM_PATH_NAMES[path_id] : "(null)";
}


unsigned long dcam_store_addr[DCAM_PATH_MAX] = {
	DCAM_FULL_BASE_WADDR,
	DCAM_BIN_BASE_WADDR0,
	DCAM_PDAF_BASE_WADDR,
	DCAM_VCH2_BASE_WADDR,
	DCAM_VCH3_BASE_WADDR,
	DCAM_AEM_BASE_WADDR,
	ISP_AFM_BASE_WADDR,
	ISP_AFL_GLB_WADDR,
	DCAM_HIST_BASE_WADDR,
	ISP_NR3_WADDR,
	ISP_BPC_OUT_ADDR,
};
unsigned long dcam2_store_addr[DCAM_PATH_MAX] = {
	DCAM2_PATH0_BASE_WADDR,
	DCAM2_PATH1_BASE_WADDR,
	/* below:not cover usefull register */
	DCAM_PDAF_BASE_WADDR,
	DCAM_VCH2_BASE_WADDR,
	DCAM_VCH3_BASE_WADDR,
	DCAM_AEM_BASE_WADDR,
	ISP_AFM_BASE_WADDR,
	ISP_AFL_GLB_WADDR,
	DCAM_HIST_BASE_WADDR,
	ISP_NR3_WADDR,
	ISP_BPC_OUT_ADDR,
};


int dcam_cfg_path_base(void *dcam_handle,
				struct dcam_path_desc *path,
				void *param)
{
	int ret = 0;
	uint32_t idx;
	struct dcam_pipe_dev *dev = NULL;
	struct dcam_path_cfg_param *ch_desc;

	if (!dcam_handle || !path || !param) {
		pr_err("error input ptr.\n");
		return -EFAULT;
	}
	dev = (struct dcam_pipe_dev *)dcam_handle;
	ch_desc = (struct dcam_path_cfg_param *)param;
	idx = dev->idx;

	switch (path->path_id) {
	case  DCAM_PATH_FULL:
		path->src_sel = ch_desc->is_raw ? 0 : 1;
	case  DCAM_PATH_BIN:
		path->frm_deci = ch_desc->frm_deci;
		path->frm_skip = ch_desc->frm_skip;

		path->is_loose = ch_desc->is_loose;
		path->endian = ch_desc->endian;
		/* TODO: should unbind slow motion code to some path */
		dev->enable_slowmotion = ch_desc->enable_slowmotion;
		dev->slowmotion_count = ch_desc->slowmotion_count;
		/* TODO: should unbind 3dnr code to some path */
		dev->is_3dnr = ch_desc->enable_3dnr;
		break;

	case DCAM_PATH_VCH2:
		path->endian = ch_desc->endian;
		path->src_sel = ch_desc->is_raw ? 1 : 0;
		break;

	default:
		pr_err("unknown path %d\n", path->path_id);
		ret = -EFAULT;
		break;
	}
	return ret;
}

int dcam_cfg_path_size(void *dcam_handle,
				struct dcam_path_desc *path,
				void *param)
{
	int ret = 0;
	uint32_t idx;
	uint32_t invalid;
	struct img_size crop_size, dst_size;
	struct dcam_pipe_dev *dev = NULL;
	struct dcam_path_cfg_param *ch_desc;

	if (!dcam_handle || !path || !param) {
		pr_err("error input ptr.\n");
		return -EFAULT;
	}
	dev = (struct dcam_pipe_dev *)dcam_handle;
	ch_desc = (struct dcam_path_cfg_param *)param;
	idx = dev->idx;

	switch (path->path_id) {
	case  DCAM_PATH_FULL:
		spin_lock(&path->size_lock);
		if (path->size_update) {
			spin_unlock(&path->size_lock);
			return -EFAULT;
		}
		path->in_size = ch_desc->input_size;
		path->in_trim = ch_desc->input_trim;

		invalid = 0;
		invalid |= ((path->in_size.w == 0) || (path->in_size.h == 0));
		invalid |= (path->in_size.w > DCAM_PATH_WMAX);
		invalid |= (path->in_size.h > DCAM_PATH_WMAX);
		invalid |= ((path->in_trim.start_x +
				path->in_trim.size_x) > path->in_size.w);
		invalid |= ((path->in_trim.start_y +
				path->in_trim.size_y) > path->in_size.h);
		if (invalid) {
			spin_unlock(&path->size_lock);
			pr_err("error size:%d %d, trim %d %d %d %d\n",
				path->in_size.w, path->in_size.h,
				path->in_trim.start_x, path->in_trim.start_y,
				path->in_trim.size_x,
				path->in_trim.size_y);
			return -EINVAL;
		}
		if ((path->in_size.w > path->in_trim.size_x) ||
			(path->in_size.h > path->in_trim.size_y)) {
			path->out_size.w = path->in_trim.size_x;
			path->out_size.h = path->in_trim.size_y;
		} else {
			path->out_size.w = path->in_size.w;
			path->out_size.h = path->in_size.h;
		}

		path->priv_size_data = ch_desc->priv_size_data;
		path->size_update = 1;
		spin_unlock(&path->size_lock);

		pr_info("cfg full path done. size %d %d %d %d\n",
			path->in_size.w, path->in_size.h,
			path->out_size.w, path->out_size.h);
		pr_info("sel %d. trim %d %d %d %d\n", path->src_sel,
			path->in_trim.start_x, path->in_trim.start_y,
			path->in_trim.size_x, path->in_trim.size_y);
		break;

	case  DCAM_PATH_BIN:
		/* lock here to keep all size parameters updating is atomic.
		 * because the rds coeff caculation may be time-consuming,
		 * we should not disable irq here, or else may cause irq missed.
		 * Just trylock set_next_frame in irq handling to avoid deadlock
		 * If last updating has not been applied yet, will return error.
		 * error may happen if too frequent zoom ratio updateing,
		 * but should not happen for first time cfg before stream on,
		 * if error return, caller can discard updating
		 * or try cfg_size again after while.
		 */
		spin_lock(&path->size_lock);
		if (path->size_update) {
			if(atomic_read(&dev->state) != STATE_RUNNING)
				pr_info("Overwrite dcam path size before dcam start if any\n");
			else {
				spin_unlock(&path->size_lock);
				pr_info("Previous path updating pending\n");
				return -EFAULT;
			}
		}

		path->in_size = ch_desc->input_size;
		path->in_trim = ch_desc->input_trim;
		path->out_size = ch_desc->output_size;

		invalid = 0;
		invalid |= ((path->in_size.w == 0) || (path->in_size.h == 0));
		invalid |= (path->in_size.w > DCAM_PATH_WMAX);
		invalid |= (path->in_size.h > DCAM_PATH_WMAX);

		/* trim should not be out range of source */
		invalid |= ((path->in_trim.start_x +
				path->in_trim.size_x) > path->in_size.w);
		invalid |= ((path->in_trim.start_y +
				path->in_trim.size_y) > path->in_size.h);

		/* output size should not be larger than trim ROI */
		invalid |= path->in_trim.size_x < path->out_size.w;
		invalid |= path->in_trim.size_y < path->out_size.h;

		/* Down scaling should not be smaller then 1/4*/
		invalid |= path->in_trim.size_x >
				(path->out_size.w * DCAM_SCALE_DOWN_MAX);
		invalid |= path->in_trim.size_y >
				(path->out_size.h * DCAM_SCALE_DOWN_MAX);

		if (invalid) {
			spin_unlock(&path->size_lock);
			pr_err("error size:%d %d, trim %d %d %d %d, dst %d %d\n",
				path->in_size.w, path->in_size.h,
				path->in_trim.start_x, path->in_trim.start_y,
				path->in_trim.size_x, path->in_trim.size_y,
				path->out_size.w, path->out_size.h);
			return -EINVAL;
		}
		crop_size.w = path->in_trim.size_x;
		crop_size.h = path->in_trim.size_y;
		dst_size = path->out_size;

		if ((crop_size.w == dst_size.w) &&
			(crop_size.h == dst_size.h))
			path->scaler_sel = DCAM_SCALER_BYPASS;
		else if ((dst_size.w * 2 == crop_size.w) &&
			(dst_size.h * 2 == crop_size.h) &&
			(ch_desc->force_rds == 0)) {
			/* only if not force_rds and 1/2 bining matched.  */
			pr_debug("1/2 binning used. src %d %d, dst %d %d\n",
				crop_size.w, crop_size.h, dst_size.w,
				dst_size.h);
			path->scaler_sel = DCAM_SCALER_BINNING;
			path->bin_ratio = 0;
		} else {
			pr_debug("RDS used. in %d %d, out %d %d\n",
				crop_size.w, crop_size.h, dst_size.w,
				dst_size.h);
			path->scaler_sel = DCAM_SCALER_RAW_DOWNSISER;
			dcam_gen_rds_coeff((uint16_t)crop_size.w,
				(uint16_t)crop_size.h,
				(uint16_t)dst_size.w,
				(uint16_t)dst_size.h,
				(uint32_t *)path->rds_coeff_buf);
		}
		path->priv_size_data = ch_desc->priv_size_data;
		path->size_update = 1;
		spin_unlock(&path->size_lock);

		pr_info("cfg bin path done. size %d %d  dst %d %d\n",
			path->in_size.w, path->in_size.h,
			path->out_size.w, path->out_size.h);
		pr_info("scaler %d. trim %d %d %d %d\n", path->scaler_sel,
			path->in_trim.start_x, path->in_trim.start_y,
			path->in_trim.size_x, path->in_trim.size_y);
		break;

	default:
		pr_err("unknown path %d\n", path->path_id);
		ret = -EFAULT;
		break;
	}
	return ret;
}


static int dcam_update_path_size(
	struct dcam_pipe_dev *dev, struct dcam_path_desc *path)
{
	int ret = 0;
	uint32_t idx;
	uint32_t path_id;
	uint32_t reg_val;

	pr_debug("enter.");

	idx = dev->idx;
	path_id = path->path_id;

	switch (path_id) {
	case  DCAM_PATH_FULL:
		if ((path->in_size.w > path->in_trim.size_x) ||
			(path->in_size.h > path->in_trim.size_y)) {

			DCAM_REG_MWR(idx, DCAM_FULL_CFG, BIT_1, 1 << 1);

			reg_val = (path->in_trim.start_y << 16) |
						path->in_trim.start_x;
			DCAM_REG_WR(idx, DCAM_FULL_CROP_START, reg_val);

			reg_val = (path->in_trim.size_y << 16) |
						path->in_trim.size_x;
			DCAM_REG_WR(idx, DCAM_FULL_CROP_SIZE, reg_val);

		} else {
			DCAM_REG_MWR(idx, DCAM_FULL_CFG, BIT_1, 0 << 1);
		}
		break;

	case  DCAM_PATH_BIN:

		DCAM_REG_MWR(idx, DCAM_CAM_BIN_CFG,
					BIT_1, path->bin_ratio << 1);
		DCAM_REG_MWR(idx, DCAM_CAM_BIN_CFG,
					BIT_3 | BIT_2,
					(path->scaler_sel & 3) << 2);

		if ((path->in_size.w > path->in_trim.size_x) ||
			(path->in_size.h > path->in_trim.size_y)) {

			reg_val = (path->in_trim.start_y << 16) |
						path->in_trim.start_x;
			reg_val |= (1 << 31);
			DCAM_REG_WR(idx, DCAM_BIN_CROP_START, reg_val);

			reg_val = (path->in_trim.size_y << 16) |
						path->in_trim.size_x;
			DCAM_REG_WR(idx, DCAM_BIN_CROP_SIZE, reg_val);
		} else {
			/* bypass trim */
			DCAM_REG_MWR(idx, DCAM_BIN_CROP_START, BIT_31, 0 << 31);
		}

		if (path->scaler_sel == DCAM_SCALER_RAW_DOWNSISER) {
			uint32_t cnt;
			uint32_t *ptr = (uint32_t *)path->rds_coeff_buf;
			unsigned long addr = RDS_COEF_TABLE_START;

			for (cnt = 0; cnt < path->rds_coeff_size;
				cnt += 4, addr += 4)
				DCAM_REG_WR(idx, addr, *ptr++);

			reg_val = ((path->out_size.h & 0xfff) << 16) |
						(path->out_size.w & 0x1fff);
			DCAM_REG_WR(idx, DCAM_RDS_DES_SIZE, reg_val);
		}
		break;
	default:
		break;
	}

	pr_debug("done\n");
	return ret;
}

int dcam_start_path(void *dcam_handle, struct dcam_path_desc *path)
{
	int ret = 0;
	uint32_t idx;
	uint32_t path_id;
	struct dcam_pipe_dev *dev = NULL;

	pr_debug("enter.");

	if (!path || !dcam_handle) {
		pr_err("error input ptr.\n");
		return -EFAULT;
	}

	dev = (struct dcam_pipe_dev *)dcam_handle;
	idx = dev->idx;
	path_id = path->path_id;

	switch (path_id) {
	case  DCAM_PATH_FULL:

		DCAM_REG_MWR(idx, DCAM_PATH_ENDIAN,
			BIT_17 |  BIT_16, path->endian.y_endian << 16);

		DCAM_REG_MWR(idx, DCAM_FULL_CFG, BIT_0, path->is_loose);
		DCAM_REG_MWR(idx, DCAM_FULL_CFG, BIT_2, path->src_sel << 2);

		/* full_path_en */
		DCAM_REG_MWR(idx, DCAM_CFG, BIT_1, (1 << 1));
		break;


	case  DCAM_PATH_BIN:
		DCAM_REG_MWR(idx, DCAM_PATH_ENDIAN,
			BIT_19 |  BIT_18, path->endian.y_endian << 18);

		DCAM_REG_MWR(idx,
			DCAM_CAM_BIN_CFG, BIT_0, path->is_loose);
		DCAM_REG_MWR(idx, DCAM_CAM_BIN_CFG,
				BIT_16, dev->enable_slowmotion << 16);
		DCAM_REG_MWR(idx, DCAM_CAM_BIN_CFG,
				BIT_19 | BIT_18 | BIT_17,
				(dev->slowmotion_count & 7) << 17);

		/* bin_path_en */
		DCAM_REG_MWR(idx, DCAM_CFG, BIT_2, (1 << 2));
		break;
	case DCAM_PATH_PDAF:
		/* pdaf path en */
		if (dev->is_pdaf)
			DCAM_REG_MWR(idx, DCAM_CFG, BIT_3, (1 << 3));
		break;

	case DCAM_PATH_VCH2:
		/* data type for raw picture */
		if (path->src_sel)
			DCAM_REG_WR(idx, DCAM_VC2_CONTROL, 0x2b << 8 | 0x01);

		DCAM_REG_MWR(idx, DCAM_PATH_ENDIAN,
			BIT_23 |  BIT_22, path->endian.y_endian << 22);

		/*vch2 path en */
		DCAM_REG_MWR(idx, DCAM_CFG, BIT_4, (1 << 4));
		break;

	case DCAM_PATH_VCH3:
		DCAM_REG_MWR(idx, DCAM_PATH_ENDIAN,
			BIT_25 |  BIT_24, path->endian.y_endian << 24);
		/*vch3 path en */
		DCAM_REG_MWR(idx, DCAM_CFG, BIT_5, (1 << 5));
		break;
	case DCAM_PATH_3DNR:
		/*
		 * TODO remove this later after pm ready
		 * set default value for 3DNR
		 */
		DCAM_REG_WR(idx, NR3_FAST_ME_PARAM, 0x010);
		dcam_k_3dnr_set_roi(dev->cap_info.cap_size.size_x,
				    dev->cap_info.cap_size.size_y,
				    1/* project_mode=1 */, idx);
		break;
	default:
		break;
	}

	pr_debug("done\n");
	return ret;
}

int dcam_stop_path(void *dcam_handle, struct dcam_path_desc *path)
{
	int ret = 0;
	uint32_t idx;
	uint32_t path_id;
	uint32_t reg_val;
	struct dcam_pipe_dev *dev = NULL;

	pr_debug("enter.");

	if (!path || !dcam_handle) {
		pr_err("error input ptr.\n");
		return -EFAULT;
	}

	dev = (struct dcam_pipe_dev *)dcam_handle;
	idx = dev->idx;
	path_id = path->path_id;

	switch (path_id) {
	case  DCAM_PATH_FULL:
		reg_val = 0;
		DCAM_REG_MWR(idx, DCAM_PATH_STOP, BIT_0, 1);
		DCAM_REG_MWR(idx, DCAM_CFG, BIT_1, (0 << 1));
		break;

	case  DCAM_PATH_BIN:
		DCAM_REG_MWR(idx, DCAM_PATH_STOP, BIT_1, 1 << 1);
		DCAM_REG_MWR(idx, DCAM_CFG, BIT_2, (0 << 2));
		break;
	case  DCAM_PATH_PDAF:
		DCAM_REG_MWR(idx, DCAM_PATH_STOP, BIT_2, 1 << 2);
		DCAM_REG_MWR(idx, DCAM_CFG, BIT_3, (0 << 3));
		break;
	case  DCAM_PATH_VCH2:
		DCAM_REG_MWR(idx, DCAM_PATH_STOP, BIT_3, 1 << 3);
		DCAM_REG_MWR(idx, DCAM_CFG, BIT_4, (0 << 4));
		break;

	case  DCAM_PATH_VCH3:
		DCAM_REG_MWR(idx, DCAM_PATH_STOP, BIT_4, 1 << 4);
		DCAM_REG_MWR(idx, DCAM_CFG, BIT_5, (0 << 5));
		break;

	default:
		break;
	}

	pr_debug("done\n");
	return ret;
}


int dcam_set_fetch(void *dcam_handle, struct dcam_fetch_info *fetch)
{
	int ret = 0;
	uint32_t fetch_pitch;
	struct dcam_pipe_dev *dev = NULL;

	pr_debug("enter.\n");

	if (!dcam_handle || !fetch) {
		pr_err("error input ptr.\n");
		return -EFAULT;
	}

	dev = (struct dcam_pipe_dev *)dcam_handle;
	/* !0 is loose */
	if (fetch->is_loose != 0) {
		fetch_pitch = fetch->size.w * 2;
	} else {
		/* to bytes */
		fetch_pitch = (fetch->size.w + 3) / 4 * 5;
		/* bytes align 32b */
		fetch_pitch = (fetch_pitch + 3) & (~0x3);
	}
	pr_info("size [%d %d], start %d, pitch %d, 0x%x\n",
		fetch->trim.size_x, fetch->trim.size_y,
		fetch->trim.start_x, fetch_pitch, fetch->addr.addr_ch0);
	/* (bitfile)unit 32b,(spec)64b */
	fetch_pitch /= 4;
	DCAM_REG_MWR(dev->idx,
		DCAM_MIPI_CAP_CFG, BIT_0, 1);
	DCAM_REG_MWR(dev->idx,
		DCAM_MIPI_CAP_CFG, (0xF << 16), fetch->pattern);

	DCAM_AXIM_MWR(IMG_FETCH_CTRL,
		BIT_1, fetch->is_loose << 1);
	DCAM_AXIM_MWR(IMG_FETCH_CTRL,
		BIT_3 | BIT_2, fetch->endian << 2);

	DCAM_AXIM_WR(IMG_FETCH_SIZE,
		(fetch->trim.size_y << 16) | (fetch->trim.size_x & 0xffff));
	DCAM_AXIM_WR(IMG_FETCH_X,
		(fetch_pitch << 16) | (fetch->trim.start_x & 0xffff));

	DCAM_AXIM_WR(IMG_FETCH_RADDR, fetch->addr.addr_ch0);

	pr_info("done.\n");

	return ret;
}

void dcam_start_fetch(void)
{
	DCAM_AXIM_WR(IMG_FETCH_START, 1);
}

static const unsigned long s_slowmotion_waddr[3][3] = {
	{
		DCAM_BIN_BASE_WADDR1,
		DCAM_BIN_BASE_WADDR2,
		DCAM_BIN_BASE_WADDR3
	},
	{
		DCAM_AEM_BASE_WADDR1,
		DCAM_AEM_BASE_WADDR2,
		DCAM_AEM_BASE_WADDR3
	},
	{
		DCAM_HIST_BASE_WADDR1,
		DCAM_HIST_BASE_WADDR2,
		DCAM_HIST_BASE_WADDR3
	}
};

int dcam_path_set_slowmotion_frame(struct dcam_pipe_dev *dev)
{
	struct dcam_path_desc *path = NULL;
	struct camera_frame *frame = NULL;
	struct timespec cur_ts;
	const int _bin = 0;
	int i = 0;

	path = &dev->path[DCAM_PATH_BIN];
	/* retrieve a frame */
	frame = camera_dequeue(&path->out_buf_queue);
	if (!frame) {
		frame = camera_dequeue(&path->reserved_buf_queue);
		pr_warn("DCAM%u bin path use reserved buffer\n", dev->idx);
		if (!frame) {
			pr_err("DCAM%u bin path no reserved memory\n",
				dev->idx);
			return -ENOMEM;
		}
	}

	/* set timestamp */
	ktime_get_ts(&cur_ts);
	frame->sensor_time.tv_sec = cur_ts.tv_sec;
	frame->sensor_time.tv_usec = cur_ts.tv_nsec / NSEC_PER_USEC;
	frame->fid = dev->frame_index;

	/* set store address */
	i = dev->frame_index % dev->slowmotion_count;
	DCAM_REG_WR(dev->idx, s_slowmotion_waddr[_bin][i - 1],
			frame->buf.iova[0]);

	/* save this frame */
	if (camera_enqueue(&path->result_queue, frame) < 0) {
		pr_err("DCAM%u bin path result queue full\n", dev->idx);
		return -EINVAL;
	}
	atomic_inc(&path->set_frm_cnt);

	pr_info("DCAM%u bin path set frame: index=%u, count=%d\n",
		dev->idx, frame->fid, atomic_read(&path->set_frm_cnt));

	return 0;
}

int dcam_path_init_slowmotion_frame(struct dcam_pipe_dev *dev)
{
	struct camera_frame *frame = NULL, *frame2 = NULL;
	struct dcam_path_desc *path = NULL;
	struct dcam_sync_helper *helper = NULL;
	struct timespec cur_ts;
	const int _bin = 0, _aem = 1, _hist = 2;
	int i = 1;

	if (unlikely(dev->slowmotion_count < 2)) {
		pr_warn("DCAM%u: not slow motion %u\n", dev->idx,
			dev->slowmotion_count);
		return -EINVAL;
	}

	/* bin path runs at full speed */
	path = &dev->path[DCAM_PATH_BIN];
	while (i < dev->slowmotion_count) {
		frame = camera_dequeue(&path->out_buf_queue);
		if (!frame) {
			pr_warn("DCAM%u bin path use reserved buffer\n",
				dev->idx);

			frame = camera_dequeue(&path->reserved_buf_queue);
			if (!frame) {
				pr_err("DCAM%u bin path doesn't even own a reserved buffer\n",
					dev->idx);
				return -ENOMEM;
			}

			/* since we only have reserved buffer... */
			while (i < dev->slowmotion_count) {
				DCAM_REG_WR(dev->idx,
						s_slowmotion_waddr[_bin][i - 1],
						frame->buf.iova[0]);
				if (camera_enqueue(&path->result_queue,
					frame) < 0) {
					pr_err("DCAM%u bin path result queue full\n",
						dev->idx);
					return -EINVAL;
				}
				atomic_inc(&path->set_frm_cnt);
			}

			break;
		}

		ktime_get_ts(&cur_ts);
		frame->sensor_time.tv_sec = cur_ts.tv_sec;
		frame->sensor_time.tv_usec = cur_ts.tv_nsec / NSEC_PER_USEC;
		frame->fid = i;

		helper = dcam_get_sync_helper(dev);
		if (helper) {
			helper->enabled |= BIT(DCAM_PATH_BIN);
			helper->sync.frames[DCAM_PATH_BIN] = frame;
			frame->sync_data = &helper->sync;
		}

		/* set store address */
		DCAM_REG_WR(dev->idx, s_slowmotion_waddr[_bin][i - 1],
				frame->buf.iova[0]);
		if (camera_enqueue(&path->result_queue, frame) < 0) {
			pr_err("DCAM%u bin path result queue full\n", dev->idx);
			return -EINVAL;
		}
		atomic_inc(&path->set_frm_cnt);

		i++;
	}

	dev->frame_index = dev->slowmotion_count;

	/* aem and hist path runs at normal speed tho */
	path = &dev->path[DCAM_PATH_AEM];
	frame = camera_dequeue(&path->reserved_buf_queue);
	if (!frame) {
		pr_err("DCAM%u aem path no reserved buffer\n", dev->idx);
		return -ENOMEM;
	}

	path = &dev->path[DCAM_PATH_HIST];
	frame2 = camera_dequeue(&path->reserved_buf_queue);
	if (!frame) {
		pr_err("DCAM%u hist path no reserved buffer\n", dev->idx);
		return -ENOMEM;
	}

	i = 1;
	while (i < dev->slowmotion_count) {
		DCAM_REG_WR(dev->idx, s_slowmotion_waddr[_aem][i - 1],
				frame->buf.iova[0]);
		DCAM_REG_WR(dev->idx, s_slowmotion_waddr[_hist][i - 1],
				frame2->buf.iova[0]);
		i++;
	}

	return 0;
}

int dcam_path_set_store_frm(void *dcam_handle,
			    struct dcam_path_desc *path,
			    struct dcam_sync_helper *helper)
{
	int ret = 0;
	uint32_t idx;
	uint32_t path_id;
	uint32_t lock = 0;
	unsigned long flags = 0;
	struct dcam_pipe_dev *dev = NULL;
	struct camera_frame *frame;
	unsigned long reg_addr;
	struct timespec cur_ts;

	pr_debug("enter. path %d\n", path->path_id);

	if (!path || !dcam_handle) {
		pr_err("error input ptr.\n");
		return -EFAULT;
	}

	dev = (struct dcam_pipe_dev *)dcam_handle;
	idx = dev->idx;
	path_id = path->path_id;

	frame = camera_dequeue(&path->out_buf_queue);
	if (frame == NULL)
		frame = camera_dequeue(
				&path->reserved_buf_queue);

	if (frame == NULL) {
		pr_err("fail to get available output buffer.\n");
		ret = -EINVAL;
		goto no_buf;
	}

	/* TODO: consider using same time between paths here */
	ktime_get_ts(&cur_ts);
	frame->sensor_time.tv_sec = cur_ts.tv_sec;
	frame->sensor_time.tv_usec = cur_ts.tv_nsec / NSEC_PER_USEC;

	frame->fid = dev->frame_index;
	/* frame->fid = path->frm_cnt; */
	ret = camera_enqueue(&path->result_queue, frame);
	if (ret) {
		pr_err("dcam%d path %d output queue overflow.\n",
				idx, path_id);
		ret = -EINVAL;
		goto overflow;
	}

	/* bind frame sync data if it is not reserved buffer */
	if (!frame->is_reserved && is_sync_enabled(dev, path_id) && helper) {
		helper->enabled |= BIT(path_id);
		helper->sync.frames[path_id] = frame;
		frame->sync_data = &helper->sync;
	}

	if ((path_id ==  DCAM_PATH_FULL) || (path_id ==  DCAM_PATH_BIN)) {
		/* use trylock here to avoid waiting if cfg_path_size
		 * is already lock
		 * because this function maybe called from irq handling.
		 * disable irq to make sure less time gap between size updating
		 * to register and frame buffer address updating to register
		 * make sure that corresponding param
		 * will applied for same frame.
		 * or else size may mismatch with frame.
		 */
		if (spin_trylock_irqsave(&path->size_lock, flags)) {
			if (path->size_update) {
				lock = 1;
				dcam_update_path_size(dev, path);
				frame->param_data = path->priv_size_data;
				path->size_update = 0;
				path->priv_size_data = NULL;
			} else {
				spin_unlock_irqrestore(&path->size_lock, flags);
			}
		}
	}

	if (idx == 2) /* dcam2 path0 ~ full path */
		reg_addr = dcam2_store_addr[path->path_id];
	else
		reg_addr = dcam_store_addr[path->path_id];
	DCAM_REG_WR(idx, reg_addr, frame->buf.iova[0]);

	if (lock)
		spin_unlock_irqrestore(&path->size_lock, flags);

	/* PDAF type3, when need write vch2_base_waddr */
	if ((path->path_id == DCAM_PATH_PDAF) && dev->is_pdaf &&
		dev->pdaf_type == 3) {
		reg_addr = dcam_store_addr[DCAM_PATH_VCH2];
		/* PDAF type3, half buffer for right PD, TBD */
		DCAM_REG_WR(idx, reg_addr, frame->buf.iova[0] +
			frame->buf.size[0] / 2);
	}
	pr_debug("done. reg %08x,  addr %08x\n",
			(uint32_t)reg_addr, (uint32_t)frame->buf.iova[0]);

	atomic_inc(&path->set_frm_cnt);
	pr_debug("DCAM%u %s set frame: index=%u, count=%d\n",
		dev->idx, to_path_name(path_id), frame->fid,
		atomic_read(&path->set_frm_cnt));

	return 0;

overflow:
	if (frame->is_reserved)
		camera_enqueue(&path->reserved_buf_queue, frame);
	else
		camera_enqueue(&path->out_buf_queue, frame);
no_buf:
	return ret;
}

