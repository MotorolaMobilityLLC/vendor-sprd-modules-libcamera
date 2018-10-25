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
#include <sprd_isp_r8p1.h>
#include <video/sprd_img.h>
#include <sprd_mm.h>

#include "cam_hw.h"
#include "cam_types.h"
#include "cam_queue.h"
#include "cam_buf.h"

#include "dcam_interface.h"
#include "dcam_reg.h"
#include "dcam_int.h"
#include "dcam_core.h"
#include "dcam_path.h"


/* Macro Definitions */
#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt) "DCAM_INT: %d %d %s : "\
	fmt, current->pid, __LINE__, __func__





static void dcam_auto_copy(enum dcam_id idx)
{
	uint32_t mask = BIT_1 | BIT_5 | BIT_7 | BIT_9 |
					BIT_11 | BIT_13 | BIT_15 | BIT_17;

	pr_debug("DCAM%d: auto copy 0x%0x:\n", idx, mask);

	/* auto copy all*/
	DCAM_REG_MWR(idx, DCAM_CONTROL, mask, mask);
}

/*
 * Dequeue a frame from result queue.
 */
static struct camera_frame *dcam_prepare_frame(struct dcam_pipe_dev *dev,
					       enum dcam_path_id path_id)
{
	struct dcam_path_desc *path = NULL;
	struct camera_frame *frame = NULL;
	struct dcam_frame_synchronizer *sync = NULL;
	uint64_t sync_index = 0;

	if (unlikely(!dev || !is_path_id(path_id)))
		return NULL;

	path = &dev->path[path_id];
	if (atomic_read(&path->set_frm_cnt) <= 1) {
		pr_warn("DCAM%u %s should not output, deci %u\n",
			dev->idx, to_path_name(path_id), path->frm_deci);
		return NULL;
	}

	frame = camera_dequeue(&path->result_queue);
	if (!frame) {
		pr_err("DCAM%u %s output buffer unavailable\n",
			dev->idx, to_path_name(path_id));
		return NULL;
	}

	atomic_dec(&path->set_frm_cnt);
	if (unlikely(frame->is_reserved)) {
		pr_warn("DCAM%u %s use reserved buffer\n",
			dev->idx, to_path_name(path_id));
		camera_enqueue(&path->reserved_buf_queue, frame);
		return NULL;
	}

	if (frame->sync_data) {
		sync = (struct dcam_frame_synchronizer *)frame->sync_data;
		sync_index = sync->index;
		sync->valid |= BIT(path_id);
	}

	/* frame->fid = path->frm_cnt; */
	pr_info("DCAM%u %s: TX DONE, sync_id %llu, fid %u, sync 0x%p\n",
		dev->idx, to_path_name(path_id), sync_index,
		frame->fid, frame->sync_data);

	return frame;
}

/*
 * Add timestamp and dispatch frame.
 */
static void dcam_dispatch_frame(struct dcam_pipe_dev *dev,
				enum dcam_path_id path_id,
				struct camera_frame *frame,
				enum dcam_cb_type type)
{
	struct dcam_frame_synchronizer *sync = NULL;
	struct timespec cur_ts;

	if (unlikely(!dev || !frame || !is_path_id(path_id)))
		return;

	ktime_get_ts(&cur_ts);
	frame->time.tv_sec = cur_ts.tv_sec;
	frame->time.tv_usec = cur_ts.tv_nsec / NSEC_PER_USEC;
	frame->boot_time = ktime_get_boottime();
	pr_debug("DCAM%u path %d: time %06d.%06d\n", dev->idx, path_id,
			(int)frame->time.tv_sec, (int)frame->time.tv_usec);

	/* TODO: unmap operation should not be related to path_id */
	if (path_id == DCAM_PATH_FULL || path_id == DCAM_PATH_BIN)
		cambuf_iommu_unmap(&frame->buf);

	dev->dcam_cb_func(type, frame, dev->cb_priv_data);

	/* TODO: release this in downstream module */
	sync = (struct dcam_frame_synchronizer *)frame->sync_data;

	pr_debug("DCAM%u path %d: sync=%p, valid=%d\n",
		 dev->idx, path_id, sync, sync ? sync->nr3_me.valid : -1);

	dcam_if_release_sync(sync, frame);
}

static void dcam_cap_sof(void *param)
{
	int i;
	struct timespec cur_ts;
	struct camera_frame *pframe;
	struct dcam_pipe_dev *dev = (struct dcam_pipe_dev *)param;
	struct dcam_path_desc *path;
	struct dcam_sync_helper *helper = NULL;

	pr_debug("DCAM%u cap_sof\n", dev->idx);

	dev->frame_index++;
	pr_debug("hw frame=%d, frame number to set: %llu\n",
		 DCAM_REG_RD(dev->idx, DCAM_CAP_FRM_CLR) & 0x3f,
		 dev->frame_index);

	/* when in slow motion, only 0, 4, 8, 12, ..., frame will output */
	if (dev->enable_slowmotion &&
	    dev->frame_index % dev->slowmotion_count) {
		dcam_path_set_slowmotion_frame(dev);
		return;
	}

	for (i  = 0; i < DCAM_PATH_MAX; i++) {
		path = &dev->path[i];
		if (atomic_read(&path->user_cnt) < 1)
			continue;

		path->frm_cnt++;
		if (path->frm_cnt <= path->frm_skip)
			continue;

		path->frm_deci_cnt++;
		if (path->frm_deci_cnt >= path->frm_deci) {
			path->frm_deci_cnt = 0;
			if (!helper)
				helper = dcam_get_sync_helper(dev);
			dcam_path_set_store_frm(dev, path, helper);
		}
	}

	/* TODO: refine code to reduce spinlock use */
	if (helper) {
		if (helper->enabled)
			helper->sync.index = dev->frame_index;
		else
			dcam_put_sync_helper(dev, helper);
	}

	/* TODO: do not copy if no register is modified */
	dcam_auto_copy(dev->idx);

	pframe = get_empty_frame();
	if (pframe) {
		ktime_get_ts(&cur_ts);
		pframe->boot_time = ktime_get_boottime();
		pframe->time.tv_sec = cur_ts.tv_sec;
		pframe->time.tv_usec = cur_ts.tv_nsec / NSEC_PER_USEC;
		pframe->evt = IMG_TX_DONE;
		pframe->irq_type = CAMERA_IRQ_DONE;
		pframe->irq_property = IRQ_DCAM_SOF;
		pframe->fid = dev->frame_index;
		dev->dcam_cb_func(DCAM_CB_IRQ_EVENT,
					pframe, dev->cb_priv_data);
	}
}

/* TODO: */
static void dcam_preview_sof(void *param)
{
}

/* for Flash */
static void dcam_sensor_eof(void *param)
{
	struct camera_frame *pframe;
	struct dcam_pipe_dev *dev = (struct dcam_pipe_dev *)param;

	pr_debug("DCAM%d sn_eof\n", dev->idx);

	pframe = get_empty_frame();
	if (pframe) {
		pframe->evt = IMG_TX_DONE;
		pframe->irq_type = CAMERA_IRQ_DONE;
		pframe->irq_property = IRQ_DCAM_SN_EOF;
		dev->dcam_cb_func(DCAM_CB_IRQ_EVENT, pframe, dev->cb_priv_data);
	}
}

/*
 * cycling frames through FULL path
 */
static void dcam_full_path_done(void *param)
{
	struct dcam_pipe_dev *dev = (struct dcam_pipe_dev *)param;
	struct camera_frame *frame = NULL;

	if ((frame = dcam_prepare_frame(dev, DCAM_PATH_FULL))) {
		if (dev->is_4in1) {
			/* 4in1,send to hal for remosaic */
			frame->irq_type = CAMERA_IRQ_4IN1_DONE;
			/* TODO: stop dcam0 full path */
		}
		dcam_dispatch_frame(dev, DCAM_PATH_FULL, frame,
				    DCAM_CB_DATA_DONE);
	}
}

/*
 * cycling frames through BIN path
 */
static void dcam_bin_path_done(void *param)
{
	struct dcam_pipe_dev *dev = (struct dcam_pipe_dev *)param;
	struct camera_frame *frame = NULL;

	if (unlikely(dev->idx == DCAM_ID_2))
		return;

	if ((frame = dcam_prepare_frame(dev, DCAM_PATH_BIN))) {
		dcam_dispatch_frame(dev, DCAM_PATH_BIN, frame,
				    DCAM_CB_DATA_DONE);
	}

	if (dev->enable_slowmotion) {
		int i = 1;
		while (i++ < dev->slowmotion_count)
			dcam_dispatch_frame(dev, DCAM_PATH_BIN,
				dcam_prepare_frame(dev, DCAM_PATH_BIN),
				DCAM_CB_DATA_DONE);
	}

	if (dev->offline) {
		/* there is source buffer for offline process */
		frame = camera_dequeue(&dev->proc_queue);
		if (frame) {
			cambuf_iommu_unmap(&frame->buf);
			dev->dcam_cb_func(DCAM_CB_RET_SRC_BUF, frame,
					  dev->cb_priv_data);
		}
	}
}

/*
 * cycling frames through AEM path
 */
static void dcam_aem_done(void *param)
{
	struct dcam_pipe_dev *dev = (struct dcam_pipe_dev *)param;
	struct camera_frame *frame = NULL;

	if (unlikely(dev->idx == DCAM_ID_2))
		return;

	if ((frame = dcam_prepare_frame(dev, DCAM_PATH_AEM))) {
		dcam_dispatch_frame(dev, DCAM_PATH_AEM, frame,
				    DCAM_CB_STATIS_DONE);
	}
}

/*
 * cycling frames through PDAF path
 */
static void dcam_pdaf_path_done(void *param)
{
	struct dcam_pipe_dev *dev = (struct dcam_pipe_dev *)param;
	struct camera_frame *frame = NULL;

	if (unlikely(dev->idx == DCAM_ID_2))
		return;

	if ((frame = dcam_prepare_frame(dev, DCAM_PATH_PDAF))) {
		dcam_dispatch_frame(dev, DCAM_PATH_PDAF, frame,
				    DCAM_CB_STATIS_DONE);
	}
}

/*
 * cycling frames through VCH2 path
 */
static void dcam_vch2_path_done(void *param)
{
	struct dcam_pipe_dev *dev = (struct dcam_pipe_dev *)param;
	struct dcam_path_desc *path = &dev->path[DCAM_PATH_VCH2];
	struct camera_frame *frame = NULL;
	enum dcam_cb_type type;

	if (unlikely(dev->idx == DCAM_ID_2))
		return;

	type = path->src_sel ? DCAM_CB_DATA_DONE : DCAM_CB_STATIS_DONE;
	if ((frame = dcam_prepare_frame(dev, DCAM_PATH_VCH2))) {
		dcam_dispatch_frame(dev, DCAM_PATH_VCH2, frame, type);
	}
}

/*
 * cycling frame through VCH3 path
 */
static void dcam_vch3_path_done(void *param)
{
	struct dcam_pipe_dev *dev = (struct dcam_pipe_dev *)param;
	struct camera_frame *frame = NULL;

	if (unlikely(dev->idx == DCAM_ID_2))
		return;

	if ((frame = dcam_prepare_frame(dev, DCAM_PATH_VCH3))) {
		dcam_dispatch_frame(dev, DCAM_PATH_AFM, frame,
				    DCAM_CB_STATIS_DONE);
	}
}

/*
 * cycling frames through AFM path
 */
static void dcam_afm_done(void *param)
{
	struct dcam_pipe_dev *dev = (struct dcam_pipe_dev *)param;
	struct camera_frame *frame = NULL;

	if (unlikely(dev->idx == DCAM_ID_2))
		return;

	if ((frame = dcam_prepare_frame(dev, DCAM_PATH_AFM))) {
		dcam_dispatch_frame(dev, DCAM_PATH_AFM, frame,
				    DCAM_CB_STATIS_DONE);
	}
}

static void dcam_afl_done(void *param)
{
	struct dcam_pipe_dev *dev = (struct dcam_pipe_dev *)param;
	struct camera_frame *frame = NULL;

	if (unlikely(dev->idx == DCAM_ID_2))
		return;

	dcam_path_set_store_frm(dev, &dev->path[DCAM_PATH_AFL], NULL);
	if ((frame = dcam_prepare_frame(dev, DCAM_PATH_AFL))) {
		dcam_dispatch_frame(dev, DCAM_PATH_AFL, frame,
				    DCAM_CB_STATIS_DONE);
	}
}

/*
 * cycling frames through HIST path
 */
static void dcam_hist_done(void *param)
{
	struct dcam_pipe_dev *dev = (struct dcam_pipe_dev *)param;
	struct camera_frame *frame = NULL;

	if (unlikely(dev->idx == DCAM_ID_2))
		return;

	if ((frame = dcam_prepare_frame(dev, DCAM_PATH_HIST))) {
		dcam_dispatch_frame(dev, DCAM_PATH_HIST, frame,
				    DCAM_CB_STATIS_DONE);
	}
}

/*
 * cycling frames through 3DNR path
 * DDR data is not used by now, while motion vector is used by ISP
 */
static void dcam_nr3_done(void *param)
{
	struct dcam_pipe_dev *dev = (struct dcam_pipe_dev *)param;
	struct camera_frame *frame = NULL;
	struct dcam_frame_synchronizer *sync = NULL;
	uint32_t p = 0, out0 = 0, out1 = 0;

	if (unlikely(dev->idx == DCAM_ID_2))
		return;

	p = DCAM_REG_RD(dev->idx, NR3_FAST_ME_PARAM);
	out0 = DCAM_REG_RD(dev->idx, NR3_FAST_ME_OUT0);
	out1 = DCAM_REG_RD(dev->idx, NR3_FAST_ME_OUT1);

	if ((frame = dcam_prepare_frame(dev, DCAM_PATH_3DNR))) {
		sync = (struct dcam_frame_synchronizer *)frame->sync_data;
		if (unlikely(!sync)) {
			pr_warn("sync not found\n");
		} else {
			sync->nr3_me.sub_me_bypass = (p >> 8) & 0x1;
			sync->nr3_me.project_mode = (p >> 4) & 0x1;
			/* currently ping-pong is disabled, mv will always be stored in ping */
			sync->nr3_me.mv_x = (out0 >> 8) & 0xff;
			sync->nr3_me.mv_y = out1 & 0xff;
			sync->nr3_me.src_width = dev->cap_info.cap_size.size_x;
			sync->nr3_me.src_height = dev->cap_info.cap_size.size_y;
			sync->nr3_me.valid = 1;
		}

		dcam_dispatch_frame(dev, DCAM_PATH_3DNR, frame,
				    DCAM_CB_STATIS_DONE);
	}
}

/*
 * track interrupt count for debug use
 */
static uint64_t dcam_int_tracker[DCAM_ID_MAX][DCAM_IRQ_NUMBER];

/*
 * reset tracker
 */
void dcam_reset_int_tracker(uint32_t idx)
{
	if (is_dcam_id(idx))
		memset(dcam_int_tracker[idx], 0, sizeof(dcam_int_tracker[idx]));
}

/*
 * print int count
 */
void dcam_dump_int_tracker(uint32_t idx)
{
	int i = 0;

	if (!is_dcam_id(idx))
		return;

	for (i = 0; i < DCAM_IRQ_NUMBER; i++) {
		pr_debug("DCAM%u i=%d, int=%llu\n", idx, i,
			 dcam_int_tracker[idx][i]);
	}
}

/*
 * registered sub interrupt service routine
 */
typedef void (*dcam_isr_type)(void *param);
static const dcam_isr_type _DCAM_ISRS[] = {
	[DCAM_SENSOR_EOF] = dcam_sensor_eof,
	[DCAM_CAP_SOF] = dcam_cap_sof,
	[DCAM_PREVIEW_SOF] = dcam_preview_sof,
	[DCAM_FULL_PATH_TX_DONE] = dcam_full_path_done,
	[DCAM_PREV_PATH_TX_DONE] = dcam_bin_path_done,
	[DCAM_PDAF_PATH_TX_DONE] = dcam_pdaf_path_done,
	[DCAM_VCH2_PATH_TX_DONE] = dcam_vch2_path_done,
	[DCAM_VCH3_PATH_TX_DONE] = dcam_vch3_path_done,
	[DCAM_AEM_TX_DONE] = dcam_aem_done,
	[DCAM_HIST_TX_DONE] = dcam_hist_done,
	[DCAM_AFL_TX_DONE] = dcam_afl_done,
	[DCAM_AFM_INTREQ1] = dcam_afm_done,
	[DCAM_NR3_TX_DONE] = dcam_nr3_done,
};

/*
 * interested interrupt bits in DCAM0
 */
static const int _DCAM0_SEQUENCE[] = {
	DCAM_CAP_SOF,/* must */
	DCAM_SENSOR_EOF,/* TODO: why for flash */
	DCAM_NR3_TX_DONE,/* for 3dnr, before data path */
	DCAM_PREV_PATH_TX_DONE,/* for bin path */
	DCAM_FULL_PATH_TX_DONE,/* for full path */
	DCAM_AEM_TX_DONE,/* for aem statis */
	DCAM_HIST_TX_DONE,/* for hist statis */
	DCAM_AFM_INTREQ0,/* for afm statis, not sure 0 or 1 */
	DCAM_AFM_INTREQ1,/* TODO: which afm interrupt to use */
	DCAM_AFL_TX_DONE,/* for afl statis */
	DCAM_PDAF_PATH_TX_DONE,/* for pdaf data */
	DCAM_VCH2_PATH_TX_DONE,/* for vch2 data */
	DCAM_VCH3_PATH_TX_DONE,/* for vch3 data */
};

/*
 * interested interrupt bits in DCAM1
 */
static const int _DCAM1_SEQUENCE[] = {
	DCAM_CAP_SOF,/* must */
	DCAM_SENSOR_EOF,/* TODO: why for flash */
	DCAM_NR3_TX_DONE,/* for 3dnr, before data path */
	DCAM_PREV_PATH_TX_DONE,/* for bin path */
	DCAM_FULL_PATH_TX_DONE,/* for full path */
	DCAM_AEM_TX_DONE,/* for aem statis */
	DCAM_HIST_TX_DONE,/* for hist statis */
	DCAM_AFM_INTREQ0,/* for afm statis, not sure 0 or 1 */
	DCAM_AFM_INTREQ1,/* TODO: which afm interrupt to use */
	DCAM_AFL_TX_DONE,/* for afl statis */
	DCAM_PDAF_PATH_TX_DONE,/* for pdaf data */
	DCAM_VCH2_PATH_TX_DONE,/* for vch2 data */
	DCAM_VCH3_PATH_TX_DONE,/* for vch3 data */
};

/*
 * interested interrupt bits in DCAM2
 */
static const int _DCAM2_SEQUENCE[] = {
	DCAM2_SENSOR_EOF,/* TODO: why for flash */
	DCAM2_FULL_PATH_TX_DONE,/* for path data */
};

/*
 * interested interrupt bits
 */
static const struct {
	size_t count;
	const int *bits;
} DCAM_SEQUENCES[DCAM_ID_MAX] = {
	{
		ARRAY_SIZE(_DCAM0_SEQUENCE),
		_DCAM0_SEQUENCE,
	},
	{
		ARRAY_SIZE(_DCAM1_SEQUENCE),
		_DCAM2_SEQUENCE,
	},
	{
		ARRAY_SIZE(_DCAM2_SEQUENCE),
		_DCAM2_SEQUENCE,
	},
};

/*
 * report error back to adaptive layer
 */
static irqreturn_t dcam_error_handler(struct dcam_pipe_dev *dev,
				      uint32_t status)
{
	pr_err("DCAM%u error 0x%x\n", dev->idx, status);

	if (atomic_read(&dev->state) == STATE_ERROR)
		return IRQ_HANDLED;
	atomic_set(&dev->state, STATE_ERROR);

	dev->dcam_cb_func(DCAM_CB_DEV_ERR, dev, dev->cb_priv_data);

	return IRQ_HANDLED;
}

/*
 * interrupt handler
 */
static irqreturn_t dcam_isr_root(int irq, void *priv)
{
	struct dcam_pipe_dev *dev = (struct dcam_pipe_dev *)priv;
	uint32_t status = 0;
	int i = 0;

	if (unlikely(irq != dev->irq)) {
		pr_err("DCAM%u irq %d mismatch %d\n", dev->idx, irq, dev->irq);
		return IRQ_NONE;
	}

	if (atomic_read(&dev->state) != STATE_RUNNING) {
		/* clear int */
		DCAM_REG_WR(dev->idx, DCAM_INT_CLR, 0xFFFFFFFF);
		pr_warn("DCAM%u ignore irq in NON-running state\n", dev->idx);
		return IRQ_NONE;
	}

	status = DCAM_REG_RD(dev->idx, DCAM_INT_MASK);
	status &= DCAMINT_IRQ_LINE_MASK;

	if (unlikely(!status))
		return IRQ_NONE;

	DCAM_REG_WR(dev->idx, DCAM_INT_CLR, status);

	for (i = 0; i < DCAM_IRQ_NUMBER; i++) {
		if (status & BIT(i))
			dcam_int_tracker[dev->idx][i]++;
	}

	if (unlikely(DCAMINT_ALL_ERROR & status))
		return dcam_error_handler(dev, status);

	pr_debug("DCAM%u status=0x%x\n", dev->idx, status);

	for (i = 0; i < DCAM_SEQUENCES[dev->idx].count; i++) {
		int cur_int = DCAM_SEQUENCES[dev->idx].bits[i];
		if (status & BIT(cur_int)) {
			if (_DCAM_ISRS[cur_int]) {
				_DCAM_ISRS[cur_int](dev);
			} else {
				pr_warn("DCAM%u missing handler for int %d\n",
					dev->idx, cur_int);
			}
			status &= ~BIT(cur_int);
			if (!status)
				break;
		}
	}

	if (unlikely(status))
		pr_warn("DCAM%u unhandled int 0x%x\n", dev->idx, status);

	return IRQ_HANDLED;
}

/*
 * request irq each time we open a camera
 */
int dcam_irq_request(struct device *pdev, int irq, void *param)
{
	struct dcam_pipe_dev *dev = NULL;
	char dev_name[32];
	int ret = 0;

	if (unlikely(!pdev || !param || irq < 0)) {
		pr_err("invalid param\n");
		return -EINVAL;
	}

	dev = (struct dcam_pipe_dev *)param;
	dev->irq = irq;
	sprintf(dev_name, "DCAM%u\n", dev->idx);

	ret = devm_request_irq(pdev, dev->irq, dcam_isr_root,
			       IRQF_SHARED, dev_name, dev);
	if (ret < 0) {
		pr_err("DCAM%u fail to install irq %d\n", dev->idx, dev->irq);
		return -EFAULT;
	}

	dcam_reset_int_tracker(dev->idx);

	return ret;
}

/*
 * free irq each time we close a camera
 */
void dcam_irq_free(struct device *pdev, void *param)
{
	struct dcam_pipe_dev *dev = NULL;

	if (unlikely(!pdev || !param)) {
		pr_err("invalid param\n");
		return;
	}

	dev = (struct dcam_pipe_dev *)param;
	devm_free_irq(pdev, dev->irq, dev);
}

