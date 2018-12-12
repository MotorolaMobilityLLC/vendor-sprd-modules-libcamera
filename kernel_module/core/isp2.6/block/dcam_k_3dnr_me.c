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

#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <sprd_mm.h>
#include <sprd_isp_r8p1.h>

#include "dcam_core.h"
#include "dcam_reg.h"
#include "dcam_interface.h"
#include "cam_types.h"
#include "cam_block.h"

#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt) "BPC: %d %d %s : "\
	fmt, current->pid, __LINE__, __func__

#define DCAM_3DNR_ROI_MAX_WIDTH 4672u
#define DCAM_3DNR_ROI_MAX_HEIGHT 3504u
#define DCAM_3DNR_ROI_SIZE_ALIGN 16u
#define DCAM_3DNR_ROI_LINE_CUT 32u


enum {
	_UPDATE_NR3 = BIT(0),
};

void dcam_k_3dnr_set_roi(uint32_t img_w, uint32_t img_h,
			 uint32_t project_mode, uint32_t idx)
{
	uint32_t roi_w_max, roi_h_max;
	uint32_t roi_w, roi_h, roi_x = 0, roi_y = 0;

	/* get max roi size
	 * max roi size should be half of normal value if project_mode is off
	 */
	roi_w_max = DCAM_3DNR_ROI_MAX_WIDTH >> !project_mode;
	roi_h_max = DCAM_3DNR_ROI_MAX_HEIGHT >> !project_mode;

	/* get roi and align to 16 pixels */
	roi_w = ALIGN_DOWN(min(roi_w_max, img_w), DCAM_3DNR_ROI_SIZE_ALIGN);
	roi_h = ALIGN_DOWN(min(roi_h_max, img_h), DCAM_3DNR_ROI_SIZE_ALIGN);

	/* get offset */
	roi_x = ALIGN_DOWN(img_w - roi_w, 2) >> 1;
	roi_y = ALIGN_DOWN(img_h - roi_h, 2) >> 1;

	/* leave 32 lines to make sure BIN DONE comes earlier than NR3 DONE */
	roi_h = max(roi_h, DCAM_3DNR_ROI_LINE_CUT) - DCAM_3DNR_ROI_LINE_CUT;

	/* almost done! */
	DCAM_REG_WR(idx, NR3_FAST_ME_ROI_PARAM0, roi_x << 16 | roi_y);
	DCAM_REG_WR(idx, NR3_FAST_ME_ROI_PARAM1, roi_w << 16 | roi_h);

	pr_info("DCAM%u 3DNR ROI %u %u %u %u\n",
		idx, roi_x, roi_y, roi_w, roi_h);
}

int dcam_k_3dnr_me(struct dcam_dev_param *param)
{
	int ret = 0;
	uint32_t idx = param->idx;
	struct dcam_pipe_dev *dev = param->dev;
	struct dcam_dev_3dnr_me *p = NULL; /* nr3_me; */

	if (param == NULL)
		return -EPERM;
	/* update ? */
	if (!(param->nr3.update & _UPDATE_NR3))
		return 0;
	param->nr3.update &= (~(_UPDATE_NR3));
	p = &param->nr3.nr3_me;
	/* debugfs blc bypass */
	if (s_dbg_bypass[idx] & (1 << _E_NR3))
		p->bypass = 1;
	DCAM_REG_MWR(idx, NR3_FAST_ME_PARAM,
			BIT(0), (p->bypass & 0x1));
	if (p->bypass)
		return 0;

	DCAM_REG_MWR(idx, NR3_FAST_ME_PARAM,
		0x30, (p->nr3_project_mode & 0x3) << 4);
	DCAM_REG_MWR(idx, NR3_FAST_ME_PARAM,
		0xC0, (p->nr3_channel_sel & 0x3) << 6);

	/* nr3_mv_bypass:  0 - calc by hardware, 1 - not calc  */
	DCAM_REG_MWR(idx, NR3_FAST_ME_PARAM, BIT(8), 0 << 8);

	/*  output_en = 0 : project value not output to ddr.  */
	DCAM_REG_MWR(idx, NR3_FAST_ME_PARAM, BIT(2), 0 << 2);

	/* update ROI according to project_mode */
	dcam_k_3dnr_set_roi(dev->cap_info.cap_size.size_x,
			    dev->cap_info.cap_size.size_y,
			    p->nr3_project_mode, idx);

	/*  sub_me_bypass.  */
	DCAM_REG_MWR(idx, NR3_FAST_ME_PARAM, BIT(3), 0 << 3);

	return ret;
}

int dcam_k_cfg_3dnr_me(struct isp_io_param *param, struct dcam_dev_param *p)
{
	int ret = 0;

	switch (param->property) {
	case DCAM_PRO_3DNR_ME:
		if (DCAM_ONLINE_MODE) {
			ret = copy_from_user((void *)&(p->nr3.nr3_me),
					param->property_param,
					sizeof(p->nr3.nr3_me));
			if (ret) {
				pr_err("blc_block: copy error, ret=0x%x\n",
					(unsigned int)ret);
				return -EPERM;
			}
			p->nr3.update |= _UPDATE_NR3;
			ret = dcam_k_3dnr_me(p);
		} else {
			mutex_lock(&p->param_lock);
			ret = copy_from_user((void *)&(p->nr3.nr3_me),
					param->property_param,
					sizeof(p->nr3.nr3_me));
			if (ret) {
				mutex_unlock(&p->param_lock);
				pr_err("blc_block: copy error, ret=0x%x\n",
					(unsigned int)ret);
				return -EPERM;
			}
			p->nr3.update |= _UPDATE_NR3;
			mutex_unlock(&p->param_lock);
		}

		break;
	default:
		pr_err("fail to support property %d\n",
			param->property);
		ret = -EINVAL;
		break;
	}

	return ret;
}
