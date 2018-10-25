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
#include "cam_hw.h"
#include "cam_pw_domain.h"
#include "dcam_int.h"
#include "dcam_interface.h"
#include "dcam_reg.h"


#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt) "DCAM_DRV: %d %d %s : "\
	fmt, current->pid, __LINE__, __func__

/* TODO: refine clk */
#if 0
#define DCAM_CLK_NUM 4

static DEFINE_MUTEX(clk_domain_mutex);
static int clk_domain_user;

struct dcam_if_clk_tag {
	char *clock;
	char *clk_name;
};
static const struct dcam_if_clk_tag dcam_if_clk_tab[DCAM_CLK_NUM] = {
	{"128", "dcam_clk_128m"},
	{"256", "dcam_clk_256m"},
	{"307", "dcam_clk_307m2"},
	{"384", "dcam_clk_384m"},
};

static int dcam_enable_clk(struct sprd_cam_hw_info *hw, void *arg)
{
	int ret = 0;

	/* dcam_if share same clk domain */
	mutex_lock(&clk_domain_mutex);
	clk_domain_user++;
	if (clk_domain_user > 1) {
		mutex_unlock(&clk_domain_mutex);
		return 0;
	}

#ifdef TEST_ON_HAPS
	pr_info("skip on haps.\n");
#else
	pr_info("todo here.\n");
#endif

	clk_domain_user--;
	mutex_unlock(&clk_domain_mutex);

	return ret;
}

static int dcam_disable_clk(struct sprd_cam_hw_info *hw, void *arg)
{
	mutex_lock(&clk_domain_mutex);
	clk_domain_user--;
	if (clk_domain_user > 0)
		goto exit;

#ifdef TEST_ON_HAPS
	pr_info("skip on haps.\n");
#else
	pr_info("todo here.\n");
#endif

exit:
	pr_info("done.\n");
	mutex_unlock(&clk_domain_mutex);
	return 0;
}

static int dcam_update_clk(struct sprd_cam_hw_info *hw, void *arg)
{
	int ret = 0;

	/* todo: update dcam clock here */
#ifdef TEST_ON_HAPS
	pr_info("skip on haps.\n");
#else
	pr_info("todo here.\n");
#endif
	return ret;
}
#endif

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
	/* TODO: prepare clk */
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
	/* TODO: unprepare clk and other resource */
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
	struct regmap *ahb_map = NULL;
	void __iomem *reg_base = NULL;
	struct resource reg_res = {0}, irq_res = {0};
	uint32_t count = 0;
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

	if (count > DCAM_ID_MAX) {
		pr_err("unsupported dcam count: %u\n", count);
		return -EINVAL;
	}

	pr_info("dev: %s, full name: %s, cam_ahb_gpr: %p, count: %u\n",
		pdev->name, dn->full_name, ahb_map, count);

	for (i = 0; i < count; i++) {
		hw = &s_dcam_hw[i];

		/* DCAM index */
		hw->idx = i;

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

	return -ENXIO;
}
