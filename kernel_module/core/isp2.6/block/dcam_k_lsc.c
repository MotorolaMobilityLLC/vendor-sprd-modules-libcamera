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
#include <linux/uaccess.h>
#include <linux/vmalloc.h>
#include <sprd_mm.h>
#include <sprd_isp_r8p1.h>

#include "dcam_reg.h"
#include "dcam_interface.h"
#include "cam_types.h"
#include "cam_buf.h"
#include "cam_block.h"

#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt) "LSC: %d %d %s : "\
	fmt, current->pid, __LINE__, __func__

#define LENS_LOAD_TIMEOUT 1000
enum {
	_UPDATE_INFO = BIT(0),
	_UPDATE_BUF = BIT(1),
};
int dcam_k_lsc_block(struct dcam_dev_param *param)
{
	int ret = 0;
	uint32_t idx = param->idx;
	uint32_t i = 0;
	uint32_t dst_w_num = 0;
	uint32_t val, lens_load_flag;
	uint32_t buf_sel;
	uint16_t *w_buff = NULL;
	struct dcam_dev_lsc_info *p;
	struct dcam_k_block *dcam_k_param = NULL;

	if (param == NULL)
		return -1;
	if (!(param->lsc.update & _UPDATE_INFO))
		return 0;
	param->lsc.update &= (~(_UPDATE_INFO));
	dcam_k_param = &(param->lsc.blk_handle);
	p = &(param->lsc.lens_info);
	if (s_dbg_bypass[idx] & (1 << _E_RAND))
		p->bypass = 1;
	/*lens_bypass*/
	if (p->bypass) {
		pr_debug("bypass.\n");
		DCAM_REG_MWR(idx, DCAM_LENS_LOAD_ENABLE, BIT_0, 1);
		return 0;
	}

	/* step0 */
	/* If it is not first configuration before stream on.
	 * we should check lens_load_flag to make sure
	 * last load is already done.
	 */
	if (!dcam_k_param->lsc_init) {
		val = DCAM_REG_RD(idx, DCAM_LENS_LOAD_ENABLE);
		lens_load_flag = (val >> 2) & 1;
		if (lens_load_flag == 0) {
			pr_debug("last lens load is not done. skip\n");
			return 0;
		}
		/* clear lens_load_flag */
		DCAM_REG_MWR(idx, DCAM_LENS_LOAD_CLR, BIT_1, 1 << 1);
	}

	/* step1:  load weight tab */
	if (dcam_k_param->weight_tab_size < p->weight_num) {
		if (dcam_k_param->lsc_weight_tab) {
			vfree(dcam_k_param->lsc_weight_tab);
			dcam_k_param->lsc_weight_tab = NULL;
			dcam_k_param->weight_tab_size = 0;
		}
		w_buff = vzalloc(p->weight_num);
		if (w_buff == NULL) {
			ret = -ENOMEM;
			goto exit;
		}
		dcam_k_param->lsc_weight_tab = w_buff;
		dcam_k_param->weight_tab_size = p->weight_num;
	}
	w_buff = (uint16_t *)dcam_k_param->lsc_weight_tab;

	ret = copy_from_user((void *)w_buff,
		p->weight_tab,
		p->weight_num);

	if (ret != 0) {
		pr_err("fail to copy from user, ret = %d\n", ret);
		ret = -EPERM;
		goto exit;
	}

	dst_w_num = (p->grid_width >> 1) + 1;
	for (i = 0; i < dst_w_num; i++) {
		val = (((uint32_t)w_buff[i * 3 + 0]) & 0xFFFF) |
			((((uint32_t)w_buff[i * 3 + 1]) & 0xFFFF) << 16);
		DCAM_REG_WR(idx, i * 8, val);
		val = (((uint32_t)w_buff[i * 3 + 2]) & 0xFFFF);
		DCAM_REG_WR(idx, i * 8 + 4, val);
	}

	/* set DCAM_APB_SRAM_CTRL in dcam_start() when stream on */
	/* DCAM_REG_MWR(idx, DCAM_APB_SRAM_CTRL, 0x1, 0x1); */

	/* step2: load grid table */
	DCAM_REG_WR(idx, DCAM_LENS_BASE_RADDR,
			(uint32_t)dcam_k_param->lsc_gridtab_haddr);
	DCAM_REG_WR(idx, DCAM_LENS_GRID_NUMBER,
			p->grid_num_t);
	/* trigger load */
	DCAM_REG_MWR(idx, DCAM_LENS_LOAD_CLR, BIT_0, 1);

	/* step3: configure lens enable and grid param...*/
	DCAM_REG_MWR(idx, DCAM_LENS_LOAD_ENABLE, BIT_0, 0);

	val = ((p->grid_width & 0x1ff) << 16) |
			((p->grid_y_num & 0xff) << 8) |
			(p->grid_x_num & 0xff);
	DCAM_REG_WR(idx, DCAM_LENS_GRID_SIZE, val);

	/* lens_load_buf_sel */
	val = DCAM_REG_RD(idx, DCAM_LENS_LOAD_ENABLE);
	buf_sel = ~(val & BIT_1) & BIT_1;
	DCAM_REG_MWR(idx, DCAM_LENS_LOAD_ENABLE, BIT_1, buf_sel);

	/* step 4: if initialized config, polling load done. */
	if (dcam_k_param->lsc_init) {
		i = 0;
		while (i++ < LENS_LOAD_TIMEOUT) {
			val = DCAM_REG_RD(idx, DCAM_LENS_LOAD_ENABLE);
			lens_load_flag = (val >> 2) & 1;
			if (lens_load_flag == 1)
				break;
			udelay(100);
		}
		if (i >= LENS_LOAD_TIMEOUT) {
			pr_err("lens grid table load timeout.\n");
			ret = -EPERM;
			goto exit;
		}
		dcam_k_param->lsc_init = 0;
	}

	pr_debug("done\n");
	return 0;

exit:
	/* bypass lsc if there is exception */
	DCAM_REG_MWR(idx, DCAM_LENS_LOAD_ENABLE, BIT_0, 1);
	return ret;
}

static int dcam_k_lsc_tranaddr(struct isp_io_param *param,
				struct dcam_dev_param *pm)
{
	int ret = 0;
	struct dcam_dev_lsc_buf *p;
	struct camera_buf *ion_buf = NULL;
	struct dcam_k_block *dcam_k_param = NULL;

	if (param == NULL || pm == NULL)
		return -EPERM;
	/* update ? */
	if (!(pm->lsc.update & _UPDATE_BUF))
		return 0;
	pm->lsc.update &= (~(_UPDATE_BUF));
	dcam_k_param = &(pm->lsc.blk_handle);
	p = &(pm->lsc.buf_info);
	ion_buf = &dcam_k_param->lsc_buf;
	ion_buf->type = CAM_BUF_USER;
	ion_buf->mfd[0] = p->mfd;
	ret = cambuf_get_ionbuf(ion_buf);
	if (ret)
		goto exit;

	/* todo: update map dev to DCAM dev */
	ret = cambuf_iommu_map(ion_buf, CAM_IOMMUDEV_DCAM);
	if (ret) {
		pr_err("fail to map lsc buffer\n");
		goto map_err;
	}
	dcam_k_param->lsc_gridtab_haddr = ion_buf->iova[0];
	p->haddr = (uint32_t)ion_buf->iova[0];

	ret = copy_to_user(param->property_param,
				p, sizeof(*p));
	if (ret) {
		pr_err("fail to copy to user. ret %d\n", ret);
		goto map_err;
	}

	return 0;

map_err:
	cambuf_put_ionbuf(ion_buf);
exit:
	dcam_k_param->lsc_gridtab_haddr = 0UL;
	ion_buf->mfd[0] = 0;
	return ret;
}

int dcam_k_cfg_lsc(struct isp_io_param *param, struct dcam_dev_param *p)
{
	int ret = 0;
	void *pcpy;
	int size;
	uint32_t bit_update;

	switch (param->property) {
	case DCAM_PRO_LSC_BLOCK:
		pcpy = (void *)&(p->lsc.lens_info);
		size = sizeof(p->lsc.lens_info);
		bit_update = _UPDATE_INFO;
		break;
	case DCAM_PRO_LSC_TRANSADDR:
		pcpy = (void *)&(p->lsc.buf_info);
		size = sizeof(p->lsc.buf_info);
		bit_update = _UPDATE_BUF;
		break;
	default:
		pr_err("fail to support property %d\n",
			param->property);

		return -EINVAL;
	}
	if (DCAM_ONLINE_MODE) {
		ret = copy_from_user(pcpy, param->property_param, size);
		if (ret) {
			pr_err("fail to copy from user. ret %d\n", ret);
			return ret;
		}
		p->lsc.update |= bit_update;
	} else {
		mutex_lock(&p->param_lock);
		ret = copy_from_user(pcpy, param->property_param, size);
		if (ret) {
			mutex_unlock(&p->param_lock);
			pr_err("fail to copy from user. ret %d\n", ret);
			return ret;
		}
		p->lsc.update |= bit_update;
		mutex_unlock(&p->param_lock);
	}
	switch (param->property) {
	case DCAM_PRO_LSC_BLOCK:
		if (DCAM_ONLINE_MODE)
			ret = dcam_k_lsc_block(p);
		break;
	case DCAM_PRO_LSC_TRANSADDR:
		/* TBC, need return data to user, how to do when offline */
		ret = dcam_k_lsc_tranaddr(param, p);
		break;
	}

	return ret;
}
