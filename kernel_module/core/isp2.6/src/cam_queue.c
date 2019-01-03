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

#include <linux/types.h>
#include <linux/list.h>
#include <linux/slab.h>

#include "cam_types.h"
#include "cam_buf.h"
#include "cam_queue.h"

#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt) "cam_queue: %d %d %s : "\
	fmt, current->pid, __LINE__, __func__


int camera_enqueue(struct camera_queue *q, struct camera_frame *pframe)
{
	int ret = 0;
	unsigned long flags;

	if (q == NULL || pframe == NULL) {
		pr_err("error: input ptr is NULL\n");
		return -EINVAL;
	}

	spin_lock_irqsave(&q->lock, flags);
	if (q->state == CAM_Q_CLEAR) {
		pr_err("error: q is not inited.\n");
		ret = -EPERM;
		goto unlock;
	}

	if (q->cnt >= q->max) {
		pr_debug("warn: queue full %d\n", q->cnt);
		ret = -EPERM;
		goto unlock;
	}
	q->cnt++;
	list_add_tail(&pframe->list, &q->head);

unlock:
	spin_unlock_irqrestore(&q->lock, flags);

	return ret;
}

struct camera_frame *camera_dequeue(struct camera_queue *q)
{
	int fatal_err;
	unsigned long flags;
	struct camera_frame *pframe = NULL;

	if (q == NULL) {
		pr_err("error: input ptr is NULL\n");
		return NULL;
	}

	spin_lock_irqsave(&q->lock, flags);
	if (q->state == CAM_Q_CLEAR) {
		pr_err("error: q is not inited.\n");
		goto unlock;
	}

	if (list_empty(&q->head) || q->cnt == 0) {
		pr_debug("queue empty %d, %d\n",
			list_empty(&q->head), q->cnt);
		fatal_err = (list_empty(&q->head) ^ (q->cnt == 0));
		if (fatal_err)
			pr_err("error:  empty %d, cnt %d\n",
					list_empty(&q->head), q->cnt);
		goto unlock;
	}

	pframe = list_first_entry(&q->head, struct camera_frame, list);
	list_del(&pframe->list);
	q->cnt--;
unlock:
	spin_unlock_irqrestore(&q->lock, flags);

	return pframe;
}

/* dequeue frame from tail of queue */
struct camera_frame *camera_dequeue_tail(struct camera_queue *q)
{
	int fatal_err;
	unsigned long flags;
	struct camera_frame *pframe = NULL;

	if (q == NULL) {
		pr_err("error: input ptr is NULL\n");
		return NULL;
	}

	spin_lock_irqsave(&q->lock, flags);
	if (q->state == CAM_Q_CLEAR) {
		pr_err("error: q is not inited.\n");
		goto unlock;
	}

	if (list_empty(&q->head) || q->cnt == 0) {
		pr_debug("queue empty %d, %d\n",
			list_empty(&q->head), q->cnt);
		fatal_err = (list_empty(&q->head) ^ (q->cnt == 0));
		if (fatal_err)
			pr_debug("error:  empty %d, cnt %d\n",
					list_empty(&q->head), q->cnt);
		goto unlock;
	}

	pframe = list_last_entry(&q->head, struct camera_frame, list);
	list_del(&pframe->list);
	q->cnt--;
unlock:
	spin_unlock_irqrestore(&q->lock, flags);

	return pframe;
}

int camera_queue_init(struct camera_queue *q,
			uint32_t max, uint32_t type,
			void (*cb_func)(void *))
{
	int ret = 0;

	if (q == NULL) {
		pr_err("error: input ptr is NULL\n");
		return -EINVAL;
	}
	q->cnt = 0;
	q->max = max;
	q->type = type;
	q->state = CAM_Q_INIT;
	q->destroy = cb_func;
	spin_lock_init(&q->lock);
	INIT_LIST_HEAD(&q->head);

	return ret;
}


int camera_queue_clear(struct camera_queue *q)
{
	int ret = 0;
	int fatal_err;
	unsigned long flags;
	struct camera_frame *pframe = NULL;

	if (q == NULL) {
		pr_err("error: input ptr is NULL\n");
		return -EINVAL;
	}
	spin_lock_irqsave(&q->lock, flags);

	do {
		if (list_empty(&q->head) || q->cnt == 0) {
			pr_debug("queue empty %d, %d\n",
				list_empty(&q->head), q->cnt);
			fatal_err = (list_empty(&q->head) ^ (q->cnt == 0));
			if (fatal_err)
				pr_debug("error:  empty %d, cnt %d\n",
						list_empty(&q->head), q->cnt);
			break;
		}
		pframe = list_first_entry(&q->head, struct camera_frame, list);
		list_del(&pframe->list);
		q->cnt--;
		if (pframe == NULL)
			break;

		if (q->destroy) {
			spin_unlock_irqrestore(&q->lock, flags);
			q->destroy(pframe);
			spin_lock_irqsave(&q->lock, flags);
		}
	} while (1);

	q->cnt = 0;
	q->max = 0;
	q->type = 0;
	q->state = CAM_Q_CLEAR;
	q->destroy = NULL;
	INIT_LIST_HEAD(&q->head);

	spin_unlock_irqrestore(&q->lock, flags);

	return ret;
}

/* get the number of how many buf in the queue */
uint32_t camera_queue_cnt(struct camera_queue *q)
{
	unsigned long flags;
	uint32_t tmp;

	spin_lock_irqsave(&q->lock, flags);
	tmp = q->cnt;
	spin_unlock_irqrestore(&q->lock, flags);

	return tmp;
}

/* Get the same time two frame from two queue
 * Input: q0, q1, two queue
 *        *pf0, *pf1, return the two frame
 *        t, no use now
 */
int camera_queue_same_frame(struct camera_queue *q0, struct camera_queue *q1,
			struct camera_frame **pf0, struct camera_frame **pf1,
			int64_t t)
{
	int ret = 0;
	unsigned long flags0, flags1;
	struct camera_frame *tmpf0, *tmpf1;
	int64_t t0, t1, mint;

	spin_lock_irqsave(&q0->lock, flags0);
	spin_lock_irqsave(&q1->lock, flags1);
	if (list_empty(&q0->head) || list_empty(&q1->head)) {
		pr_err("some buffer is empty\n");
		ret = -EFAULT;
		goto _EXT;
	}
	/* set mint a large value */
	mint = (((uint64_t)1 << 63) - 1);
	/* get least delta time two frame */
	list_for_each_entry(tmpf0, &q0->head, list) {
		t0 = tmpf0->sensor_time.tv_sec * 1000000ll;
		t0 += tmpf0->sensor_time.tv_usec;
		list_for_each_entry(tmpf1, &q1->head, list) {
			t1 = tmpf1->sensor_time.tv_sec * 1000000ll;
			t1 += tmpf1->sensor_time.tv_usec;
			t1 -= t0;
			if (t1 < 0)
				t1 = -t1;
			if (t1 < mint) {
				*pf0 = tmpf0;
				*pf1 = tmpf1;
				mint = t1;
			}
		}
		/* loop 9times --> 3times, 400us */
		if (mint < 400)
			break;
	}
	pr_info("mint:%lld\n", mint);
	if (mint > 50 * 1000) { /* delta > 50ms fail */
		ret = -EFAULT;
		goto _EXT;
	}
	list_del(&((*pf0)->list));
	q0->cnt--;
	list_del(&((*pf1)->list));
	q1->cnt--;

	ret = 0;
_EXT:
	spin_unlock_irqrestore(&q0->lock, flags0);
	spin_unlock_irqrestore(&q1->lock, flags1);

	return ret;
}

struct camera_frame *get_empty_frame(void)
{
	int ret = 0;
	uint32_t i;
	struct camera_queue *q = g_empty_frm_q;
	struct camera_frame *pframe = NULL;

	pr_debug("Enter.\n");
	do {
		pframe = camera_dequeue(q);
		if (pframe == NULL) {
			for (i = 0; i < CAM_EMP_Q_LEN_INC; i++) {
				pframe = kzalloc(sizeof(*pframe), GFP_KERNEL);
				if (pframe == NULL) {
					pr_err("error: no memory.\n");
					break;
				}
				pr_debug("alloc frame %p\n", pframe);
				ret = camera_enqueue(q, pframe);
				if (ret) {
					pr_err("fail to input empty q, %p\n",
						pframe);
					kfree(pframe);
					pframe = NULL;
					break;
				}
				pframe = NULL;
				atomic_inc(&g_mem_dbg->empty_frm_cnt);
			}
			pr_info("alloc %d empty frames, cnt %d\n",
				i, atomic_read(&g_mem_dbg->empty_frm_cnt));
		}
	} while (pframe == NULL);

	pr_debug("Done. get frame %p\n", pframe);
	return pframe;
}


int put_empty_frame(struct camera_frame *pframe)
{
	int ret = 0;

	if (pframe == NULL) {
		pr_err("error: null input.\n");
		return -EINVAL;
	}

	memset(pframe, 0, sizeof(struct camera_frame));
	ret = camera_enqueue(g_empty_frm_q, pframe);
	if (ret) {
		pr_err("fail to input frame to empty queue.\n");
		ret = -EINVAL;
	}
	pr_debug("put frame %p\n", pframe);

	return ret;
}

void free_empty_frame(void *param)
{
	struct camera_frame *pframe;

	pframe = (struct camera_frame *)param;
	atomic_dec(&g_mem_dbg->empty_frm_cnt);
	pr_debug("free frame %p, cnt %d\n", pframe,
		atomic_read(&g_mem_dbg->empty_frm_cnt));
	kfree(pframe);
}

