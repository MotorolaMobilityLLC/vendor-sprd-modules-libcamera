/*
 * Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/semaphore.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/mfd/syscon.h>
#include <sprd_mm.h>
#include "sprd_sensor_core.h"
#include "csi_api.h"
#include "csi_driver.h"
#include "dphy_drv.h"

#include "mm_ahb.h"

#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt) "csi_api: %d %d %s : "\
	fmt, current->pid, __LINE__, __func__

#define CSI_PATTERN_ENABLE (1)

static struct csi_dt_node_info *s_csi_dt_info_p[3];

static struct csi_dt_node_info *csi_get_dt_node_data(int sensor_id)
{
	return s_csi_dt_info_p[sensor_id];
}

static int csi_mipi_clk_enable(int sensor_id)
{
	struct csi_dt_node_info *dt_info = NULL;
	void __iomem *reg_base = NULL;
	unsigned long reg = 0;

	dt_info = csi_get_dt_node_data(sensor_id);

	/* enable ckg clock */
	regmap_update_bits(dt_info->syscon.mm_ahb, dt_info->syscon.ckg_eb,
		dt_info->syscon.ckg_eb_msk, dt_info->syscon.ckg_eb_msk);

	/* enable csi clock */
	regmap_update_bits(dt_info->syscon.mm_ahb, dt_info->syscon.csi_eb,
		dt_info->syscon.csi_eb_msk, dt_info->syscon.csi_eb_msk);

	if (CSI_PATTERN_ENABLE) {
		reg = 0x6210004c + (dt_info->controller_id*0x4);
		reg_base = ioremap_nocache(reg, 0x04);
		if (reg_base == NULL) {
			pr_err("enable ipg clock failed\n");
			return -1;
		}
		/* set clock from soc */
		REG_MWR(reg_base, BIT_16, ~BIT_16);
		iounmap(reg_base);
		reg_base = NULL;
	}

	return 0;
}

static void csi_mipi_clk_disable(int sensor_id)
{
	struct csi_dt_node_info *dt_info = NULL;

	dt_info = csi_get_dt_node_data(sensor_id);

	/* enable ckg clock */
	regmap_update_bits(dt_info->syscon.mm_ahb, dt_info->syscon.ckg_eb,
		dt_info->syscon.ckg_eb_msk, ~(dt_info->syscon.ckg_eb_msk));

	/* enable csi clock */
	regmap_update_bits(dt_info->syscon.mm_ahb, dt_info->syscon.csi_eb,
		dt_info->syscon.csi_eb_msk, ~(dt_info->syscon.csi_eb_msk));
}

int csi_api_mipi_phy_cfg_init(struct device_node *phy_node, int sensor_id)
{
	unsigned int phy_id = 0;

	if (of_property_read_u32(phy_node, "sprd,phyid", &phy_id)) {
		pr_err("fail to get the sprd_phyid\n");
		return -1;
	}

	if (phy_id >= PHY_MAX) {
		pr_err("%s:fail to parse phyid : phyid:%u\n", __func__, phy_id);
		return -1;
	}

	pr_info("sensor_id %d mipi_phy:phy_id:%u\n", sensor_id, phy_id);

	return phy_id;
}

int csi_api_dt_node_init(struct device *dev, struct device_node *dn,
					int sensor_id, unsigned int phy_id)
{
	struct csi_dt_node_info *csi_info = NULL;
	struct resource res;
	void __iomem *reg_base = NULL;
	u32 args[2] = {0};
	int ret = 0;

	pr_info("Entry, phyid:%d, sensor_id:%d\n", phy_id, sensor_id);
	csi_info = devm_kzalloc(dev, sizeof(*csi_info), GFP_KERNEL);
	if (!csi_info)
		return -EINVAL;

	/* read address */
	if (of_address_to_resource(dn, 0, &res)) {
		pr_err("csi2_init:fail to get address info\n");
		return -EINVAL;
	}

	reg_base = ioremap_nocache(res.start, resource_size(&res));
	if (IS_ERR_OR_NULL(reg_base)) {
		pr_err("csi_dt_init:fail to get csi regbase\n");
		return PTR_ERR(reg_base);
	}
	csi_info->reg_base = (unsigned long)reg_base;

	csi_info->phy_id = phy_id;

	if (of_property_read_u32_index(dn, "sprd,csi-id", 0,
				&csi_info->controller_id)) {
		pr_err("get csi id failed\n");
		return -EINVAL;
	}

	/* get mm ahb register handle */
	csi_info->syscon.mm_ahb = syscon_regmap_lookup_by_name(dn, "csi_eb");
	if (IS_ERR_OR_NULL(csi_info->syscon.mm_ahb)) {
		pr_err("get mm ahb syscon failed\n");
		return -EINVAL; /* PRT_ERR(csi_info->syscon.mm_ahb); */
	}

	ret = syscon_get_args_by_name(dn, "csi_dhpy_c_sel", 2, args);
	if (ret == 2) {
		csi_info->syscon.dphy_sel = args[0];
		csi_info->syscon.dphy_msk = args[1];
	} else {
		pr_err("get dphy sel failed\n");
		goto err;
	}

	ret = syscon_get_args_by_name(dn, "csi_eb", 2, args);
	if (ret == 2) {
		csi_info->syscon.csi_eb = args[0];
		csi_info->syscon.csi_eb_msk = args[1];
	} else {
		pr_err("get csi eb failed\n");
		goto err;
	}

	ret = syscon_get_args_by_name(dn, "csi_rst", 2, args);
	if (ret == 2) {
		csi_info->syscon.csi_rst = args[0];
		csi_info->syscon.csi_rst_msk = args[1];
	} else {
		pr_err("get csi rest failed\n");
		goto err;
	}

	ret = syscon_get_args_by_name(dn, "csi_ckg_en", 2, args);
	if (ret == 2) {
		csi_info->syscon.ckg_eb = args[0];
		csi_info->syscon.ckg_eb_msk = args[1];
	} else {
		pr_err("get csi ckg en failed\n");
		goto err;
	}

	ret = syscon_get_args_by_name(dn, "cphy_ckg_en", 2, args);
	if (ret == 2) {
		csi_info->syscon.cphy_ckg_eb = args[0];
		csi_info->syscon.cphy_ckg_eb_msk = args[1];
	} else {
		pr_err("get csi cphy ckg failed\n");
		goto err;
	}

	ret = phy_parse_dt(phy_id, dev);
	if (ret != 0) {
		pr_err("parse phy dts efailed\n");
		goto err;
	}

	csi_reg_base_save(csi_info, sensor_id);
	csi_phy_power_down(phy_id, sensor_id, 1);
	s_csi_dt_info_p[sensor_id] = csi_info;
	pr_info("csi dt info:sensor_id :%d, phy_id:%d, csi_id:%d, csi_info:0x%p\n",
		sensor_id, csi_info->phy_id, csi_info->controller_id,
		csi_info);

	return 0;
err:
	pr_err("%s failed, ret = %d\n", __func__, ret);
	return -EINVAL;
}

static int dphy_init(unsigned int bps_per_lane,
					unsigned int phy_id, int sensor_id)
{
	int ret = 0;
	struct csi_dt_node_info *dt_info = NULL;

	dt_info = csi_get_dt_node_data(sensor_id);
	csi_phy_power_down(phy_id, sensor_id, 0);
	/* csi_controller_enable(dt_info, phy_id); */
	dphy_init_state(phy_id, dt_info->controller_id);
	ret = dphy_csi_path_cfg(dt_info);

	return ret;
}

int csi_api_mipi_phy_cfg(void)
{
	int ret = 0;
	return ret;
}

int csi_api_open(int bps_per_lane, int phy_id, int lane_num, int sensor_id)
{
	int ret = 0;
	struct csi_dt_node_info *dt_info = csi_get_dt_node_data(sensor_id);

	pr_info("entry, phyid:%d, lane_number:%d, csi_id:%d\n",
			phy_id, lane_num, sensor_id);

	if (!dt_info) {
		pr_err("fail to get valid phy ptr\n");
		return -EINVAL;
	}
	ret = csi_ahb_reset(dt_info, dt_info->controller_id);
	if (unlikely(ret))
		goto EXIT;

	ret = csi_mipi_clk_enable(sensor_id);
	if (unlikely(ret < 0))
		goto EXIT;

	udelay(1);

	ret = dphy_init(bps_per_lane, phy_id, sensor_id);
	if (unlikely(ret))
		goto EXIT;

	csi_start(sensor_id);
	csi_set_on_lanes(lane_num, sensor_id);
	if (CSI_PATTERN_ENABLE)
		csi_ipg_mode_cfg(sensor_id, 1, 0, 4224, 3136);

	pr_info("exit success\n");

	return ret;
EXIT:
	pr_err("fail to open csi %d\n", ret);
	csi_api_close(phy_id, sensor_id);
	return ret;
}

int csi_api_close(uint32_t phy_id, int sensor_id)
{
	int ret = 0;
	struct csi_dt_node_info *dt_info = NULL;

	dt_info = csi_get_dt_node_data(sensor_id);
	if (!dt_info) {
		pr_err("fail to get valid phy ptr\n");
		return -EINVAL;
	}

	if (CSI_PATTERN_ENABLE)
		csi_ipg_mode_cfg(sensor_id, 0, 0, 4224, 3136);
	csi_close(sensor_id);
	csi_phy_power_down(phy_id, sensor_id, 1);
	csi_mipi_clk_disable(sensor_id);
	pr_info("csi api close ret: %d\n", ret);

	return ret;
}

int csi_api_switch(int sensor_id)
{
	return 0;
}

