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

#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <sprd_mm.h>

#include "cam_hw.h"
#include "cam_types.h"
#include "cam_queue.h"
#include "cam_buf.h"

#include "isp_interface.h"
#include "isp_reg.h"
/* To include IOMMU relared registers which are common in both DCAM & ISP */
#include "dcam_reg.h"
#include "isp_int.h"
#include "isp_core.h"

#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt) "ISP_INT: %d %d %s : "\
	fmt, current->pid, __LINE__, __func__



typedef void(*isp_isr)(enum isp_context_id idx, void *param);


static const uint32_t isp_irq_process[] = {
	ISP_INT_ISP_ALL_DONE,
	ISP_INT_SHADOW_DONE,
	ISP_INT_DISPATCH_DONE,
	ISP_INT_STORE_DONE_PRE,
	ISP_INT_STORE_DONE_VID,
	ISP_INT_NR3_ALL_DONE,
	ISP_INT_NR3_SHADOW_DONE,
	ISP_INT_STORE_DONE_THUMBNAIL,
	ISP_INT_LTMHISTS_DONE,
	ISP_INT_FMCU_LOAD_DONE,
	ISP_INT_FMCU_SHADOW_DONE,
	ISP_INT_FMCU_STORE_DONE,
	ISP_INT_HIST_CAL_DONE,
};

//#define ISP_INT_RECORD 1
#ifdef ISP_INT_RECORD
#define INT_RCD_SIZE 0x10000
static uint32_t isp_int_recorder[ISP_CONTEXT_NUM][32][INT_RCD_SIZE];
static uint32_t int_index[ISP_CONTEXT_NUM][32];
#endif

static uint32_t irq_done[ISP_CONTEXT_NUM][32];
static char *isp_dev_name[] = {"isp0",
				"isp1"
				};

static inline void record_isp_int(enum isp_context_id c_id, uint32_t irq_line)
{
	uint32_t k;

	for (k = 0; k < 32; k++) {
		if (irq_line & (1 << k))
			irq_done[c_id][k]++;
	}

#ifdef ISP_INT_RECORD
	{
		uint32_t cnt, time, int_no;
		struct timespec cur_ts;

		ktime_get_ts(&cur_ts);
		time = (uint32_t)(cur_ts.tv_sec & 0xffff);
		time <<= 16;
		time |= (uint32_t)((cur_ts.tv_nsec / (NSEC_PER_USEC * 100)) & 0xffff);
		for (int_no = 0; int_no < 32; int_no++) {
			if (irq_line & BIT(int_no)) {
				cnt = int_index[c_id][int_no];
				isp_int_recorder[c_id][int_no][cnt] = time;
				cnt++;
				int_index[c_id][int_no] = (cnt & (INT_RCD_SIZE - 1));
			}
		}
	}
#endif
}


static void isp_frame_done(enum isp_context_id idx, struct isp_pipe_dev *dev)
{
	int i;
	struct isp_pipe_context *pctx;
	struct camera_frame *pframe;
	struct isp_path_desc *path;
	struct timespec cur_ts;
	ktime_t boot_time;

	pctx = &dev->ctx[idx];

	if (pctx->enable_slowmotion == 0)
		complete(&pctx->frm_done);

	boot_time = ktime_get_boottime();
	ktime_get_ts(&cur_ts);

	/* return buffer to cam channel shared buffer queue. */
	pframe = camera_dequeue(&pctx->proc_queue);
	if (pframe) {
		cambuf_iommu_unmap(&pframe->buf);
		pr_debug("ctx %d, ret src buf: %p,  priv %p\n",
				pctx->ctx_id, pframe,  pctx->cb_priv_data);
		pctx->isp_cb_func(ISP_CB_RET_SRC_BUF, pframe,
			pctx->cb_priv_data);
	} else {
		/* should not be here */
		pr_err("fail to get src frame.\n");
	}

	/* get output buffers for all path */
	for (i = 0; i < ISP_SPATH_NUM; i++) {
		path = &pctx->isp_path[i];
		if (atomic_read(&path->user_cnt) <= 0) {
			pr_debug("path %p not enable\n", path);
			continue;
		}
		if (path->bind_type == ISP_PATH_SLAVE) {
			pr_debug("slave path %d\n", path->spath_id);
			continue;
		}

		pframe = camera_dequeue(&path->result_queue);
		if (!pframe) {
			pr_err("error: no frame from queue. cxt:%d, path:%d\n",
						pctx->ctx_id, path->spath_id);
			continue;
		}
		atomic_dec(&path->store_cnt);
		pframe->boot_time = boot_time;
		pframe->time.tv_sec = cur_ts.tv_sec;
		pframe->time.tv_usec = cur_ts.tv_nsec / NSEC_PER_USEC;

		pr_debug("ctx %d path %d, fid %d, storen %d\n",
			pctx->ctx_id, path->spath_id, pframe->fid,
			atomic_read(&path->store_cnt));
		pr_debug("time_sensor %03d.%6d, time_isp %03d.%06d\n",
			(uint32_t)pframe->sensor_time.tv_sec,
			(uint32_t)pframe->sensor_time.tv_usec,
			(uint32_t)pframe->time.tv_sec,
			(uint32_t)pframe->time.tv_usec);

		if (unlikely(pframe->is_reserved)) {
			camera_enqueue(&path->reserved_buf_queue, pframe);
		} else {
			cambuf_iommu_unmap(&pframe->buf);
			pctx->isp_cb_func(ISP_CB_RET_DST_BUF,
						pframe, pctx->cb_priv_data);
		}
		path->frm_cnt++;
	}
	pr_debug("cxt_id:%d done.\n", idx);
}


static int isp_err_pre_proc(enum isp_context_id idx, void *isp_handle)
{
	struct isp_pipe_dev *dev = NULL;
	struct isp_pipe_context *pctx;

	pr_err("isp cxt_id:%d error happened\n", idx);
	dev = (struct isp_pipe_dev *)isp_handle;
	pctx = &dev->ctx[idx];
	/* todo: isp error handling */
	/*pctx->isp_cb_func(ISP_CB_DEV_ERR, dev, pctx->cb_priv_data);*/
	return 0;
}

static void isp_all_done(enum isp_context_id idx, void *isp_handle)
{
	struct isp_pipe_dev *dev = NULL;
	struct isp_pipe_context *pctx;

	dev = (struct isp_pipe_dev *)isp_handle;
	pctx = &dev->ctx[idx];
	if (pctx->fmcu_handle) {
		pr_debug("fmcu started. skip all done.\n ");
		return;
	}
	pr_debug("cxt_id:%d done.\n", idx);
	isp_frame_done(idx, dev);
}

static void isp_shadow_done(enum isp_context_id idx, void *isp_handle)
{
	pr_debug("cxt_id:%d shadow done.\n", idx);
}

static void isp_dispatch_done(enum isp_context_id idx, void *isp_handle)
{
	pr_debug("cxt_id:%d done.\n", idx);
}

static void isp_pre_store_done(enum isp_context_id idx, void *isp_handle)
{
	pr_debug("cxt_id:%d done.\n", idx);
}

static void isp_vid_store_done(enum isp_context_id idx, void *isp_handle)
{
	pr_debug("cxt_id:%d done.\n", idx);
}

static void isp_thumb_store_done(enum isp_context_id idx, void *isp_handle)
{
	pr_debug("cxt_id:%d done.\n", idx);
}

static void isp_fmcu_store_done(enum isp_context_id idx, void *isp_handle)
{
	struct isp_pipe_dev *dev = NULL;
	struct isp_pipe_context *pctx;
	int i;

	dev = (struct isp_pipe_dev *)isp_handle;
	pctx = &dev->ctx[idx];

	pr_info("fmcu done cxt_id:%d ch_id[%d]\n", idx, pctx->ch_id);
	isp_frame_done(idx, dev);

	if (pctx->enable_slowmotion == 1) {
		if (pctx->enable_slowmotion == 1)
			complete(&pctx->frm_done);
		for (i = 0; i < pctx->slowmotion_count - 1; i++)
			isp_frame_done(idx, dev);
	}
}

static void isp_fmcu_shadow_done(enum isp_context_id idx, void *isp_handle)
{
	struct isp_pipe_dev *dev = NULL;
	struct isp_pipe_context *pctx;

	dev = (struct isp_pipe_dev *)isp_handle;
	pctx = &dev->ctx[idx];

	pr_debug("cxt_id:%d done.\n", idx);
}

static void isp_fmcu_load_done(enum isp_context_id idx, void *isp_handle)
{
	pr_debug("cxt_id:%d done.\n", idx);
}

static void isp_3dnr_all_done(enum isp_context_id idx, void *isp_handle)
{
	struct isp_pipe_context *pctx;
	struct isp_pipe_dev *dev;

	dev = (struct isp_pipe_dev *)isp_handle;
	pctx = &dev->ctx[idx];

	pr_debug("3dnr all done. cxt_id:%d\n", idx);

}

static void isp_3dnr_shadow_done(enum isp_context_id idx, void *isp_handle)
{
	struct isp_pipe_context *pctx;
	struct isp_pipe_dev *dev;

	dev = (struct isp_pipe_dev *)isp_handle;
	pctx = &dev->ctx[idx];

	pr_debug("3dnr shadow done. cxt_id:%d\n", idx);

}

static void isp_ltm_hists_done(enum isp_context_id idx, void *isp_handle)
{
	struct isp_pipe_context *pctx;
	struct isp_pipe_dev *dev;
	int completion = 0;

	dev = (struct isp_pipe_dev *)isp_handle;
	pctx = &dev->ctx[idx];

	dev->ltm_handle->ops->set_frmidx(pctx->ltm_ctx.fid);
	completion = dev->ltm_handle->ops->get_completion();
	pr_debug("ltm hists done. cxt_id:%d, %d, fid:[%d], completion[%d]\n",
		idx, pctx->ltm_ctx.isp_pipe_ctx_id,
		pctx->ltm_ctx.fid, completion);

	if (completion && (pctx->ltm_ctx.fid >= completion)) {
		completion = dev->ltm_handle->ops->complete_completion();
		pr_info("complete completion fid [%d], completion[%d]\n",
			pctx->ltm_ctx.fid, completion);
	}
}

static struct camera_frame *isp_hist2_frame_prepare(enum isp_context_id idx,
						void *isp_handle)
{
	int i = 0;
	int max_item = 256;
	unsigned long HIST_BUF = ISP_HIST2_BUF0_ADDR;
	struct camera_frame *frame = NULL;
	struct isp_pipe_dev *dev;
	struct isp_pipe_context *pctx;

	uint32_t *buf = NULL;

	dev = (struct isp_pipe_dev *)isp_handle;
	pctx = &dev->ctx[idx];

	frame = camera_dequeue(&pctx->hist2_result_queue);
	if (!frame) {
		pr_err("isp ctx_id[%d] hist2_result_queue unavailable\n", idx);
		return NULL;
	}

	buf = (uint32_t *)frame->buf.addr_k[0];

	if (!frame->buf.addr_k[0]) {
		pr_err("err: null ptr\n");
		if (camera_enqueue(&pctx->hist2_result_queue, frame) < 0)
			pr_err("fatal err\n");
		return NULL;
	}
	for (i = 0; i < max_item; i++)
		buf[i] = ISP_HREG_RD(HIST_BUF + i * 4);


	frame->width = pctx->fetch.in_trim.size_x;
	frame->height = pctx->fetch.in_trim.size_y;

	return frame;
}

static void isp_dispatch_frame(enum isp_context_id idx,
				void *isp_handle,
				struct camera_frame *frame,
				enum isp_cb_type type)
{
	struct timespec cur_ts;
	struct isp_pipe_dev *dev;
	struct isp_pipe_context *pctx;

	dev = (struct isp_pipe_dev *)isp_handle;
	pctx = &dev->ctx[idx];

	if (unlikely(!dev || !frame))
		return;

	ktime_get_ts(&cur_ts);
	frame->time.tv_sec = cur_ts.tv_sec;
	frame->time.tv_usec = cur_ts.tv_nsec / NSEC_PER_USEC;
	frame->boot_time = ktime_get_boottime();

	pr_debug("isp ctx[%d]: time %06d.%06d\n", idx,
		(int)frame->time.tv_sec, (int)frame->time.tv_usec);

	pctx->isp_cb_func(type, frame, pctx->cb_priv_data);
}

static void isp_hist_cal_done(enum isp_context_id idx, void *isp_handle)
{
	struct camera_frame *frame = NULL;
	struct isp_pipe_dev *dev = NULL;
	struct isp_pipe_context *pctx;

	dev = (struct isp_pipe_dev *)isp_handle;
	pctx = &dev->ctx[idx];

	/* only use isp hist in preview channel */
	if (pctx->ch_id != CAM_CH_PRE)
		return;

	if ((frame = isp_hist2_frame_prepare(idx, isp_handle)))
		isp_dispatch_frame(idx, isp_handle, frame, ISP_CB_STATIS_DONE);

}


static isp_isr isp_isr_handler[32] = {
	[ISP_INT_ISP_ALL_DONE] = isp_all_done,
	[ISP_INT_SHADOW_DONE] = isp_shadow_done,
	[ISP_INT_DISPATCH_DONE] = isp_dispatch_done,
	[ISP_INT_STORE_DONE_PRE] = isp_pre_store_done,
	[ISP_INT_STORE_DONE_VID] = isp_vid_store_done,
	[ISP_INT_NR3_ALL_DONE]	 = isp_3dnr_all_done,
	[ISP_INT_NR3_SHADOW_DONE] = isp_3dnr_shadow_done,
	[ISP_INT_STORE_DONE_THUMBNAIL] = isp_thumb_store_done,
	[ISP_INT_LTMHISTS_DONE]  = isp_ltm_hists_done,
	[ISP_INT_FMCU_LOAD_DONE] = isp_fmcu_load_done,
	[ISP_INT_FMCU_SHADOW_DONE] = isp_fmcu_shadow_done,
	[ISP_INT_FMCU_STORE_DONE] = isp_fmcu_store_done,
	[ISP_INT_HIST_CAL_DONE] = isp_hist_cal_done,
};

struct isp_int_ctx {
	unsigned long reg_offset;
	uint32_t err_mask;
	uint32_t irq_numbers;
	const uint32_t *irq_vect;
} isp_int_ctxs[4] = {
		{ /* P0 */
			ISP_P0_INT_BASE,
			ISP_INT_LINE_MASK_ERR | ISP_INT_LINE_MASK_MMU,
			(uint32_t)ARRAY_SIZE(isp_irq_process),
			isp_irq_process,
		},
		{ /* C0 */
			ISP_C0_INT_BASE,
			ISP_INT_LINE_MASK_ERR | ISP_INT_LINE_MASK_MMU,
			(uint32_t)ARRAY_SIZE(isp_irq_process),
			isp_irq_process,
		},
		{ /* P1 */
			ISP_P1_INT_BASE,
			ISP_INT_LINE_MASK_ERR | ISP_INT_LINE_MASK_MMU,
			(uint32_t)ARRAY_SIZE(isp_irq_process),
			isp_irq_process,
		},
		{ /* C1 */
			ISP_C1_INT_BASE,
			ISP_INT_LINE_MASK_ERR | ISP_INT_LINE_MASK_MMU,
			(uint32_t)ARRAY_SIZE(isp_irq_process),
			isp_irq_process,
		},
};

static void  isp_dump_iommu_regs(void)
{
	uint32_t reg = 0;
	uint32_t val[4];

	for (reg = 0; reg <= MMU_STS; reg += 16) {
		val[0] = ISP_MMU_RD(reg);
		val[1] = ISP_MMU_RD(reg + 4);
		val[2] = ISP_MMU_RD(reg + 8);
		val[3] = ISP_MMU_RD(reg + 12);
		pr_err_ratelimited("offset=0x%04x: %08x %08x %08x %08x\n",
			 reg, val[0], val[1], val[2], val[3]);
	}
}

static irqreturn_t isp_isr_root(int irq, void *priv)
{
	unsigned long irq_offset;
	uint32_t iid;
	enum isp_context_id c_id;
	uint32_t sid, k;
	uint32_t err_mask;
	uint32_t irq_line = 0;
	uint32_t irq_numbers = 0;
	const uint32_t *irq_vect = NULL;
	struct isp_pipe_dev *isp_handle = (struct isp_pipe_dev *)priv;

	if (!isp_handle) {
		pr_err("error: null dev\n");
		return IRQ_HANDLED;
	}
	pr_debug("isp irq %d, %p\n", irq, priv);

	if (irq == isp_handle->irq_no[0]) {
		iid = 0;
	} else if (irq == isp_handle->irq_no[1]) {
		iid = 1;
	} else {
		pr_err("error irq %d mismatched\n", irq);
		return IRQ_NONE;
	}

	for (sid = 0; sid < 2; sid++) {
		c_id = (iid << 1) | sid;
		isp_handle->ctx[c_id].in_irq_handler = 1;
		irq_offset = isp_int_ctxs[c_id].reg_offset;
		err_mask = isp_int_ctxs[c_id].err_mask;
		irq_numbers = isp_int_ctxs[c_id].irq_numbers;
		irq_vect = isp_int_ctxs[c_id].irq_vect;

		pr_debug("offset %lx,  num %d, vect: %p\n",
				irq_offset, irq_numbers,  irq_vect);
		irq_line = ISP_HREG_RD(irq_offset + ISP_INT_INT0);
		pr_debug("cid: %d,  irq_line: %08x\n", c_id,  irq_line);
		if (unlikely(irq_line == 0)) {
			isp_handle->ctx[c_id].in_irq_handler = 0;
			continue;
		}

		record_isp_int(c_id, irq_line);

		/*clear the interrupt*/
		ISP_HREG_WR(irq_offset + ISP_INT_CLR0, irq_line);

		pr_debug("isp ctx %d irqno %d, INT: 0x%x\n",
						c_id, irq, irq_line);

		if (atomic_read(&isp_handle->ctx[c_id].user_cnt) < 1) {
			pr_info("contex %d is stopped\n", c_id);
			isp_handle->ctx[c_id].in_irq_handler = 0;
			return IRQ_HANDLED;
		}

		if (unlikely(err_mask & irq_line)) {
			pr_err("error irq: isp ctx %d, INT:0x%x\n",
					c_id, irq_line);
			if (irq_line & ISP_INT_LINE_MASK_MMU) {
				pr_err("IOMMU  Error, IOMMU Registers Dump\n");
				isp_dump_iommu_regs();
			}

			/*handle the error here*/
			if (isp_err_pre_proc(c_id, isp_handle)) {
				isp_handle->ctx[c_id].in_irq_handler = 0;
				return IRQ_HANDLED;
			}
		}

		for (k = 0; k < irq_numbers; k++) {
			uint32_t irq_id = irq_vect[k];

			if (irq_line & (1 << irq_id)) {
				if (isp_isr_handler[irq_id]) {
					isp_isr_handler[irq_id](
						c_id, isp_handle);
				}
			}
			irq_line  &= ~(1 << irq_id);
			if (!irq_line)
				break;
		}
		isp_handle->ctx[c_id].in_irq_handler = 0;
	}

	return IRQ_HANDLED;
}


int isp_irq_request(struct device *p_dev,
		uint32_t *irq_no, void *isp_handle)
{
	int ret = 0;
	uint32_t  id;
	struct isp_pipe_dev *ispdev;

	if (!p_dev || !isp_handle || !irq_no) {
		pr_err("Input ptr is NULL\n");
		return -EFAULT;
	}
	ispdev = (struct isp_pipe_dev *)isp_handle;

	for (id = 0; id < ISP_LOGICAL_COUNT; id++) {
		ispdev->irq_no[id] = irq_no[id];
		ret = devm_request_irq(p_dev,
				ispdev->irq_no[id], isp_isr_root,
				IRQF_SHARED, isp_dev_name[id], (void *)ispdev);
		if (ret) {
			pr_err("fail to install isp%d irq_no %d\n",
					id, ispdev->irq_no[id]);
			if (id == 1)
				free_irq(ispdev->irq_no[0], (void *)ispdev);
			return -EFAULT;
		}
		pr_info("install isp%d irq_no %d\n", id, ispdev->irq_no[id]);
	}

	memset(irq_done, 0, sizeof(irq_done));

	return ret;
}

int reset_isp_irq_cnt(int ctx_id)
{
	if (ctx_id < ISP_CONTEXT_NUM)
		memset(irq_done[ctx_id], 0, sizeof(irq_done[ctx_id]));

#ifdef ISP_INT_RECORD
	if (ctx_id < ISP_CONTEXT_NUM) {
		memset(isp_int_recorder[ctx_id][0], 0, sizeof(isp_int_recorder) / ISP_CONTEXT_NUM);
		memset(int_index[ctx_id], 0, sizeof(int_index) / ISP_CONTEXT_NUM);
	}
#endif
	return 0;
}

int trace_isp_irq_cnt(int ctx_id)
{
	int i;

	for (i = 0; i < 32; i++)
		pr_debug("done %d %d :   %d\n", ctx_id, i, irq_done[ctx_id][i]);

#ifdef ISP_INT_RECORD
	{
		uint32_t cnt, j;
		int idx = ctx_id;
		for (cnt = 0; cnt < (uint32_t)irq_done[idx][ISP_INT_SHADOW_DONE]; cnt += 4) {
			j = (cnt & (INT_RCD_SIZE - 1)); //rolling
			pr_info("isp%u j=%d, %03d.%04d, %03d.%04d, %03d.%04d, %03d.%04d, %03d.%04d, %03d.%04d\n",
			idx, j, (uint32_t)isp_int_recorder[idx][ISP_INT_ISP_ALL_DONE][j] >> 16,
			 (uint32_t)isp_int_recorder[idx][ISP_INT_ISP_ALL_DONE][j] & 0xffff,
			 (uint32_t)isp_int_recorder[idx][ISP_INT_SHADOW_DONE][j] >> 16,
			 (uint32_t)isp_int_recorder[idx][ISP_INT_SHADOW_DONE][j] & 0xffff,
			 (uint32_t)isp_int_recorder[idx][ISP_INT_DISPATCH_DONE][j] >> 16,
			 (uint32_t)isp_int_recorder[idx][ISP_INT_DISPATCH_DONE][j] & 0xffff,
			 (uint32_t)isp_int_recorder[idx][ISP_INT_STORE_DONE_PRE][j] >> 16,
			 (uint32_t)isp_int_recorder[idx][ISP_INT_STORE_DONE_PRE][j] & 0xffff,
			 (uint32_t)isp_int_recorder[idx][ISP_INT_STORE_DONE_VID][j] >> 16,
			 (uint32_t)isp_int_recorder[idx][ISP_INT_STORE_DONE_VID][j] & 0xffff,
			 (uint32_t)isp_int_recorder[idx][ISP_INT_FMCU_CONFIG_DONE][j] >> 16,
			 (uint32_t)isp_int_recorder[idx][ISP_INT_FMCU_CONFIG_DONE][j] & 0xffff);
		}
	}
#endif
	return 0;
}

int isp_irq_free(struct device *p_dev, void *isp_handle)
{
	struct isp_pipe_dev *ispdev;

	ispdev = (struct isp_pipe_dev *)isp_handle;
	if (!ispdev) {
		pr_err("fail to get valid input ptr\n");
		return -EFAULT;
	}

	devm_free_irq(p_dev, ispdev->irq_no[0], (void *)ispdev);
	devm_free_irq(p_dev, ispdev->irq_no[1], (void *)ispdev);

	return 0;
}
