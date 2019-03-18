/*
 * Copyright (C) 2015 Spreadtrum Communications Inc.
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

#include <linux/clk.h>
#include <linux/err.h>
#include <linux/mfd/syscon.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/regmap.h>
#include <video/sprd_mmsys_pw_domain.h>
#include "cam_hw.h"
#include "dcam_int.h"
#include "dcam_path.h"
#include "dcam_interface.h"
#include "dcam_reg.h"


#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt) "DCAM_DRV: %d %d %s : "\
	fmt, current->pid, __LINE__, __func__

static int dcam_enable_clk(struct sprd_cam_hw_info *hw, void *arg)
{
	int ret = 0;

	pr_debug(", E\n");
#ifndef TEST_ON_HAPS
	if (!hw) {
		pr_err("param erro\n");
		return -EINVAL;
	}
	ret = clk_set_parent(hw->clk, hw->clk_parent);
	if (ret) {
		pr_err("dcam%d, set clk parent fail\n", hw->idx);
		clk_set_parent(hw->clk, hw->clk_default);
		return ret;
	}
	ret = clk_prepare_enable(hw->clk);
	if (ret) {
		pr_err("dcam%d, clk enable fail\n", hw->idx);
		clk_set_parent(hw->clk, hw->clk_default);
		return ret;
	}
	ret = clk_set_parent(hw->axi_clk, hw->clk_axi_parent);
	if (ret) {
		pr_err("dcam%d, set axi clk parent fail\n", hw->idx);
		clk_set_parent(hw->axi_clk, hw->clk_axi_default);
		return ret;
	}
	ret = clk_prepare_enable(hw->axi_clk);
	if (ret) {
		pr_err("dcam%d, axi_clk enable fail\n", hw->idx);
		clk_set_parent(hw->axi_clk, hw->clk_axi_default);
		return ret;
	}
	ret = clk_prepare_enable(hw->core_eb);
	if (ret) {
		pr_err("dcam%d, set eb fail\n", hw->idx);
		clk_disable_unprepare(hw->clk);
		return ret;
	}
	ret = clk_prepare_enable(hw->axi_eb);
	if (ret) {
		pr_err("dcam%d, set dcam axi clk fail\n", hw->idx);
		clk_disable_unprepare(hw->clk);
		clk_disable_unprepare(hw->core_eb);
	}

#endif /* TEST_ON_HAPS */

	return ret;
}

static int dcam_disable_clk(struct sprd_cam_hw_info *hw, void *arg)
{
	int ret = 0;

	pr_debug(", E\n");
#ifndef TEST_ON_HAPS
	if (!hw) {
		pr_err("param erro\n");
		return -EINVAL;
	}
	clk_set_parent(hw->clk, hw->clk_default);
	clk_set_parent(hw->axi_clk, hw->clk_axi_default);
	clk_disable_unprepare(hw->clk);
	clk_disable_unprepare(hw->axi_clk);
	clk_disable_unprepare(hw->axi_eb);
	clk_disable_unprepare(hw->core_eb);
#endif /* TEST_ON_HAPS */

	return ret;
}

static int dcam_update_clk(struct sprd_cam_hw_info *hw, void *arg)
{
	int ret = 0;

	pr_debug(", E\n");
#ifndef TEST_ON_HAPS
	pr_warn("Not support and no use, now\n");
#endif /* TEST_ON_HAPS */

	return ret;
}

/*
 * Initialize dcam_if hardware, power/clk/int should be prepared after this call
 * returns. It also brings the dcam_pipe_dev from INIT state to IDLE state.
 */
static int dcam_hw_init(struct sprd_cam_hw_info *hw, void *arg)
{
	int ret = 0;

	if (unlikely(!hw)) {
		pr_err("invalid param hw\n");
		return -EINVAL;
	}

	ret = sprd_cam_pw_on();
	ret = sprd_cam_domain_eb();
	/* prepare clk */
	dcam_enable_clk(hw, arg);
	ret = dcam_irq_request(&hw->pdev->dev, hw->irq_no, arg);

	return ret;
}

/*
 * De-initialize dcam_if hardware thus power/clk/int resource can be released.
 * Registers will be inaccessible and dcam_pipe_dev will enter INIT state from
 * IDLE state.
 */
static int dcam_hw_deinit(struct sprd_cam_hw_info *hw, void *arg)
{
	int ret = 0;

	if (unlikely(!hw)) {
		pr_err("invalid param hw\n");
		return -EINVAL;
	}

	dcam_irq_free(&hw->pdev->dev, arg);
	/* unprepare clk and other resource */
	dcam_disable_clk(hw, arg);
	ret = sprd_cam_domain_disable();
	ret = sprd_cam_pw_off();

	return ret;
}

/*
 * Interface to manipulate dcam_if hardware resource including power domain, clk
 * resource and interrupt status. Service related operation should be
 * implemented in a dcam_pipe_dev.
 */
static struct sprd_cam_hw_ops s_dcam_ops = {
	.init = dcam_hw_init,
	.deinit = dcam_hw_deinit,
	.enable_clk = dcam_enable_clk,
	.disable_clk = dcam_disable_clk,
	.update_clk = dcam_update_clk,
};

/*
 * This variable saves hardware resources for dcam_if. All <DCAM_ID_MAX> dcam_if
 * shares same clk and AXI resource on top level.
 *
 * Useful member for a dcam_if:
 * @idx:         index
 * @irq_no:      irq number for the dcam_if
 * @pdev:        related platform device object
 * @cam_ahb_gpr: register mapping of AHB bus
 * @phy_base:    physical base address for each dcam_if
 * @reg_base:    virtual register base address for each dcam_if
 */
static struct sprd_cam_hw_info s_dcam_hw[DCAM_ID_MAX];

/*
 * extract dcam hardware resource from dt
 */
int dcam_if_parse_dt(struct platform_device *pdev,
			struct sprd_cam_hw_info **dcam_hw,
			uint32_t *dcam_count)
{
	struct sprd_cam_hw_info *hw = NULL;
	struct device_node *dn = NULL;
	struct device_node *qos_node = NULL;
	struct device_node *iommu_node = NULL;
	struct regmap *ahb_map = NULL;
	void __iomem *reg_base = NULL;
	struct resource reg_res = {0}, irq_res = {0};
	uint32_t count = 0, prj_id = 0;
	uint32_t dcam_max_w = 0, dcam_max_h = 0;
	int i = 0, irq = 0;

	pr_info("start dcam dts parse\n");

	if (unlikely(!pdev)) {
		pr_err("invalid platform device\n");
		return -EINVAL;
	}

	dn = pdev->dev.of_node;
	if (unlikely(!dn)) {
		pr_err("invalid device node\n");
		return -EINVAL;
	}

	ahb_map = syscon_regmap_lookup_by_phandle(dn, "sprd,cam-ahb-syscon");
	if (IS_ERR_OR_NULL(ahb_map)) {
		pr_err("fail to get sprd,cam-ahb-syscon\n");
		return PTR_ERR(ahb_map);
	}

	/* TODO: clk parse */
	pr_info("skip parse clock tree on haps.\n");

	if (of_property_read_u32(dn, "sprd,dcam-count", &count)) {
		pr_err("fail to parse the property of sprd,dcam-count\n");
		return -EINVAL;
	}

	if (of_property_read_u32(dn, "sprd,project-id", &prj_id)) {
		pr_info("fail to parse the property of sprd,projectj-id\n");
	}

	dcam_max_w = DCAM_PATH_WMAX;
	dcam_max_h = DCAM_PATH_HMAX;

	if (prj_id == ROC1) {
		dcam_max_w = DCAM_PATH_WMAX_ROC1;
		dcam_max_h = DCAM_PATH_HMAX_ROC1;
	}

	if (count > DCAM_ID_MAX) {
		pr_err("unsupported dcam count: %u\n", count);
		return -EINVAL;
	}

	pr_info("dev: %s, full name: %s, cam_ahb_gpr: %p, count: %u\n",
		pdev->name, dn->full_name, ahb_map, count);

	pr_info("DCAM dcam_max_w = %u dcam_max_h = %u\n",dcam_max_w, dcam_max_h);

	iommu_node = of_parse_phandle(dn, "iommus", 0);
	if (iommu_node) {
		if (of_address_to_resource(iommu_node, 1, &reg_res))
			pr_err("fail to get DCAM IOMMU  addr\n");
		else {
			reg_base = ioremap(reg_res.start,
				reg_res.end - reg_res.start + 1);
			if (!reg_base)
				pr_err("fail to map DCAM IOMMU base\n");
			else
				g_dcam_mmubase = (unsigned long)reg_base;
		}
	}
	pr_info("DCAM IOMMU Base  0x%lx \n", g_dcam_mmubase);

	for (i = 0; i < count; i++) {
		hw = &s_dcam_hw[i];

		/* DCAM index */
		hw->idx = i;

		/* Assign project ID, DCAM Max Height & Width Info */
		hw->prj_id =(enum sprd_cam_prj_id) prj_id;
		hw->path_max_width = dcam_max_w;
		hw->path_max_height = dcam_max_h;

		/* bounded kernel device node */
		hw->pdev = pdev;

		/* AHB bus register mapping */
		hw->cam_ahb_gpr = ahb_map;

		/* resource related hw ops */
		hw->ops = &s_dcam_ops;

		/* irq */
		irq = of_irq_to_resource(dn, i, &irq_res);
		if (irq <= 0) {
			pr_err("fail to get DCAM%d irq, error: %d\n", i, irq);
			goto err_iounmap;
		}
		hw->irq_no = (uint32_t) irq;

		/* DCAM register mapping */
		if (of_address_to_resource(dn, i, &reg_res)) {
			pr_err("fail to get DCAM%d phy addr\n", i);
			goto err_iounmap;
		}
		hw->phy_base = (unsigned long) reg_res.start;

		reg_base = ioremap(reg_res.start,
					reg_res.end - reg_res.start + 1);
		if (!reg_base) {
			pr_err("fail to map DCAM%d reg base\n", i);
			goto err_iounmap;
		}
		hw->reg_base = (unsigned long) reg_base;
		g_dcam_regbase[i] = (unsigned long)reg_base; /* TODO */

		pr_info("DCAM%d reg: %s 0x%lx %lx, irq: %s %u\n", i,
			reg_res.name, hw->phy_base, hw->reg_base,
			irq_res.name, hw->irq_no);

		/* qos dt parse */
		qos_node = of_parse_phandle(dn, "dcam_qos", 0);
		if (qos_node) {
			uint8_t val;

			if (of_property_read_u8(qos_node, "awqos-high", &val)) {
				pr_warn("isp awqos-high reading fail.\n");
				val = 0xD;
			}
			hw->awqos_high = (uint32_t)val;

			if (of_property_read_u8(qos_node, "awqos-low", &val)) {
				pr_warn("isp awqos-low reading fail.\n");
				val = 0xA;
			}
			hw->awqos_low = (uint32_t)val;

			if (of_property_read_u8(qos_node, "arqos", &val)) {
				pr_warn("isp arqos-high reading fail.\n");
				val = 0xA;
			}
			hw->arqos_high = val;
			hw->arqos_low = val;

			pr_info("get dcam qos node. r: %d %d w: %d %d\n",
				hw->arqos_high, hw->arqos_low,
				hw->awqos_high, hw->awqos_low);
		} else {
			hw->awqos_high = 0xD;
			hw->awqos_low = 0xA;
			hw->arqos_high = 0xA;
			hw->arqos_low = 0xA;
		}

#ifndef TEST_ON_HAPS
		/* read dcam clk */
		hw->core_eb = of_clk_get_by_name(dn, "dcam_eb");
		if (IS_ERR_OR_NULL(hw->core_eb)) {
			pr_err("read clk fail, dcam_eb\n");
			goto err_iounmap;
		}
		hw->axi_eb = of_clk_get_by_name(dn, "dcam_axi_eb");
		if (IS_ERR_OR_NULL(hw->axi_eb)) {
			pr_err("read clk fail, dcam_axi_eb\n");
			goto err_iounmap;
		}
		hw->clk = of_clk_get_by_name(dn, "dcam_clk");
		if (IS_ERR_OR_NULL(hw->clk)) {
			pr_err("read clk fail, dcam_clk\n");
			goto err_iounmap;
		}
		hw->clk_parent = of_clk_get_by_name(dn, "dcam_clk_parent");
		if (IS_ERR_OR_NULL(hw->clk_parent)) {
			pr_err("read clk fail, dcam_clk_parent\n");
			goto err_iounmap;
		}
		hw->clk_default = clk_get_parent(hw->clk);
		hw->axi_clk = of_clk_get_by_name(dn, "dcam_axi_clk");
		if (IS_ERR_OR_NULL(hw->clk)) {
			pr_err("read clk fail, axi_clk\n");
			goto err_iounmap;
		}
		hw->clk_axi_parent = of_clk_get_by_name(dn, "dcam_axi_clk_parent");
		if (IS_ERR_OR_NULL(hw->clk_parent)) {
			pr_err("read clk fail, dcam_clk_parent\n");
			goto err_iounmap;
		}
		hw->clk_axi_default = clk_get_parent(hw->axi_clk);
#endif /* TEST_ON_HAPS */

		dcam_hw[i] = hw;
	}

	if (of_address_to_resource(dn, i, &reg_res)) {
		pr_err("fail to get AXIM phy addr\n");
		goto err_iounmap;
	}

	reg_base = ioremap(reg_res.start, reg_res.end - reg_res.start + 1);
	if (!reg_base) {
		pr_err("fail to map AXIM reg base\n");
		goto err_iounmap;
	}
	g_dcam_aximbase = (unsigned long)reg_base; /* TODO */

	pr_info("DCAM AXIM reg: %s %lx\n", reg_res.name, g_dcam_aximbase);

	*dcam_count = count;

	return 0;

err_iounmap:
	for (i = i - 1; i >= 0; i--)
		iounmap((void __iomem *)(dcam_hw[0]->reg_base));
	iounmap((void __iomem *)g_dcam_mmubase);
	g_dcam_mmubase = 0;

	return -ENXIO;
}
