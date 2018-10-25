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
//#include <linux/mm_ahb.h>
//#include <linux/sprd_otp.h>
#include <sprd_mm.h>

#include "sprd_sensor_core.h"
#include "csi_api.h"
#include "csi_driver.h"

#include "mm_ahb.h"

#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt) "csi_api: %d %d %s : "\
	fmt, current->pid, __LINE__, __func__

#define CSI_PATTERN_ENABLE		1

#if  0//sysfs debug
struct kobject *csi_kobj;
EXPORT_SYMBOL_GPL(csi_kobj);

static int g_csi_enabled = 0;

ssize_t csi_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	pr_info("entry %s\n", __func__);
	csi_api_open(0, 0, 0, 0);
	  return sprintf(buf, "csi_show %d\n", g_csi_enabled);
}

ssize_t csi_store(struct kobject *kobj, struct kobj_attribute *attr,
		  const char *buf, size_t n)
{
	return n;
}

static struct kobj_attribute csi_attr = {
	  .attr = {
		.name = "terminal",
		.mode = 0777,
	  },
	  .show = csi_show,
	  .store = csi_store,
};

//__ATTR("terminal", 0777, csi_show, csi_store);

static struct attribute * g[] = {
	  &csi_attr.attr,
	    NULL,
};

static struct attribute_group attr_group = {
	.attrs = g,
};

static int fund(void)
{
	int error = 0;

	csi_kobj = kobject_create_and_add("csi", NULL);
	if (!csi_kobj)
		     return -ENOMEM;

	 error = sysfs_create_group(csi_kobj, &attr_group);
	 if (error)
	     return error;

	 return error;
}
#endif /* sysfs debug end*/




static struct csi_dt_node_info *s_csi_dt_info_p[3];

static struct csi_dt_node_info *csi_get_dt_node_data(int sensor_id)
{
	return s_csi_dt_info_p[sensor_id];
}
static int csi_mipi_clk_enable(int sensor_id)
{
	struct csi_dt_node_info *dt_info = csi_get_dt_node_data(sensor_id);
	int ret = 0;

	if (!dt_info) {
		pr_err("fail to get valid dt_info ptr\n");
		return -EINVAL;
	}


	if (!dt_info->ckg_eb || !dt_info->mipi_csi_gate_eb
		|| !dt_info->csi_eb) {
		pr_err("fail to csi mipi clk enable\n");
		return -EINVAL;
	}

	ret = clk_prepare_enable(dt_info->ckg_eb);
	if (ret) {
		pr_err("fail to csi eb clk\n");
		return ret;
	}

	ret = clk_prepare_enable(dt_info->csi_eb);
	if (ret) {
		pr_err("fail to csi eb clk\n");
		return ret;
	}

	ret = clk_prepare_enable(dt_info->mipi_csi_gate_eb);
	if (ret) {
		pr_err("fail to mipi csi mipi gate clk\n");
		return ret;
	}


	/*ret = csi_ipg_set_clk(sensor_id);
	if (ret) {
		pr_err("fail to set ipg clk\n");
		return ret;
	}*/
	return ret;
}

static void csi_mipi_clk_disable(int sensor_id)
{
	struct csi_dt_node_info *dt_info = csi_get_dt_node_data(sensor_id);

	if (!dt_info) {
		pr_err("fail to get valid dt_info ptr\n");
		return;
	}


	if (!dt_info->mipi_csi_gate_eb || !dt_info->csi_eb) {
		pr_err("fail to csi mipi clk disable\n");
		return;
	}
	clk_disable_unprepare(dt_info->mipi_csi_gate_eb);
	clk_disable_unprepare(dt_info->csi_eb);
	clk_disable_unprepare(dt_info->ckg_eb);

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
					int csi_id, unsigned int phy_id)
{
	struct csi_dt_node_info *csi_info = NULL;
	struct resource res;
	void __iomem *reg_base = NULL;
	struct regmap *regmap_syscon = NULL;

	pr_info("entry %s\n", __func__);
	//csi_info = devm_kzalloc(dev, sizeof(*csi_info), GFP_KERNEL);
	csi_info = kzalloc(sizeof(*csi_info), GFP_KERNEL);
	if (!csi_info)
		return -EINVAL;

	/* read address */
	if (of_address_to_resource(dn, 0, &res)) {
		pr_err("csi2_init:fail to get address info\n");
		return -EINVAL;
	}

	reg_base = ioremap_nocache(res.start, resource_size(&res));
	//reg_base = ioremap_nocache(0x62300000, 0x100);
	if (IS_ERR_OR_NULL(reg_base)) {
		pr_err("csi_dt_init:fail to get csi regbase\n");
		return PTR_ERR(reg_base);
	}
	csi_info->reg_base = (unsigned long)reg_base;

	csi_reg_base_save(csi_info, csi_id);
	s_csi_dt_info_p[csi_id] = csi_info;
	return 0;

	csi_info->phy.phy_id = phy_id;
	pr_info("csi node name %s\n", dn->name);

	if (of_property_read_u32_index(dn, "sprd,csi-id", 0,
		&csi_info->controller_id)) {
		pr_err("csi2_init:fail to get csi-id\n");
		return -EINVAL;
	}

	/*pr_info("csi %d ,phy addr:0x%"PRIx64" ,size:0x%"PRIx64" , reg %lx\n",
			csi_info->controller_id, res.start,
			resource_size(&res),
			csi_info->reg_base);*/

	/* read clocks */
	csi_info->ckg_eb = of_clk_get_by_name(dn,
					"clk_ckg_eb");

	csi_info->mipi_csi_gate_eb = of_clk_get_by_name(dn,
					"clk_mipi_csi_gate_eb");

	csi_info->csi_eb = of_clk_get_by_name(dn, "clk_csi_eb");

	regmap_syscon = syscon_regmap_lookup_by_phandle(dn,
					"sprd,cam-ahb-syscon");
	if (IS_ERR_OR_NULL(regmap_syscon)) {
		pr_err("csi_dt_init:fail to get cam-ahb-syscon\n");
		return PTR_ERR(regmap_syscon);
	}
	csi_info->phy.cam_ahb_syscon = regmap_syscon;

	regmap_syscon = syscon_regmap_lookup_by_phandle(dn,
					"sprd,anlg_phy_g10_controller");
	if (IS_ERR_OR_NULL(regmap_syscon)) {
		pr_err("csi_dt_init:fail to get anlg_phy_g10_controller\n");
		return PTR_ERR(regmap_syscon);
	}
	csi_info->phy.anlg_phy_g1_syscon = regmap_syscon;

	csi_reg_base_save(csi_info, csi_id);
	csi_phy_power_down(csi_info, csi_id, 1);
	pr_info("csi dt info:sensor_id :%d, csi_info:0x%p\n",
				csi_id, csi_info);

	return 0;
}

static int csi_init(unsigned int bps_per_lane,
				     unsigned int phy_id, int sensor_id)
{
	struct csi_dt_node_info *dt_info = csi_get_dt_node_data(sensor_id);

	csi_phy_power_down(dt_info, sensor_id, 0);
	csi_controller_enable(dt_info, sensor_id);
	dphy_init(&dt_info->phy, sensor_id);

	return 0;
}

static void csi_start(int sensor_id)
{
	csi_set_on_lanes(1, sensor_id);
	csi_shut_down_phy(0, sensor_id);
	csi_reset_phy(sensor_id);
	csi_reset_controller(sensor_id);
	csi_event_enable(sensor_id);
}

int csi_api_mipi_phy_cfg(void)
{
	int ret = 0;
	return ret;
}

int csi_api_open(int bps_per_lane, int phy_id, int lane_num, int csi_id)
{
	int ret = 0;
	struct csi_dt_node_info *dt_info = csi_get_dt_node_data(csi_id);
	void __iomem *reg_base = NULL;

		pr_info("entry -> %s\n", __func__);

if (1)
{
	{
		/* power on */
		pr_info("power up\n");
		reg_base = ioremap_nocache(0x327d0000, 0x4);
		if (!reg_base) {
			pr_info("0x327d0000: ioremap failed\n");
			return -EINVAL;
		}
		pr_info("11---, reg_base: 0x%p\n", reg_base);
		REG_MWR(reg_base, 0x00000200, BIT_9);
		udelay(100);
		iounmap(reg_base);
		reg_base = NULL;
		pr_info("power up end\n");

		reg_base = ioremap_nocache(0x327e0024, 0x4);
		if (!reg_base) {
			pr_info("0x327e0004: ioremap failed\n");
			return -EINVAL;
		}
		pr_info("11---, reg_base: 0x%p\n", reg_base);
		REG_MWR(reg_base, BIT_24|BIT_25, 0X00);
		udelay(100);
		iounmap(reg_base);
		reg_base = NULL;

	}

	{
		pr_info("enabel clock\n");
		reg_base = ioremap_nocache(0x62200000, 0x8);
		if (!reg_base) {
			pr_err("0x62200000: remap failed\n");
			return -EINVAL;
		}
		pr_info("start enable clock, addr: 0x%p\n", reg_base);
		//dump_stack();
		/* enable ckg_eb and csi0,1,2 */
		REG_MWR(reg_base, 0x000000f0, (BIT_7|BIT_6|BIT_5|BIT_4));
		/* enable mipi clock gate */
		REG_MWR((reg_base + 0x8), 0x0000038, (BIT_5|BIT_4|BIT_3));
		iounmap(reg_base);
		reg_base = NULL;
		pr_info("enabel clock end\n");
	}

	pr_info("start IPG mode\n");

	csi_start(csi_id);
	csi_set_on_lanes(lane_num, csi_id);
	
	csi_ipg_mode_cfg(csi_id, 1, 0, 4224, 3136);	
//	csi_ipg_mode_cfg(csi_id, 1, 0, 2112, 1568);

	return ret;
}
  
	if (!dt_info) {
		pr_err("fail to get valid phy ptr\n");
		return -EINVAL;
	}
	ret = csi_ahb_reset(&dt_info->phy, dt_info->controller_id);
	if (unlikely(ret))
		goto EXIT;
	ret = csi_mipi_clk_enable(csi_id);
	if (unlikely(ret)) {
		pr_err("fail to csi mipi clk enable\n");
		csi_mipi_clk_disable(csi_id);
		goto EXIT;
	}
	udelay(1);
	ret = csi_init(bps_per_lane, phy_id, csi_id);
	if (unlikely(ret))
		goto EXIT;
	csi_start(csi_id);
	csi_set_on_lanes(lane_num, csi_id);
	if (CSI_PATTERN_ENABLE)
		csi_ipg_mode_cfg(csi_id, 1, 1, 640, 480);

	return ret;
EXIT:
	pr_err("fail to open csi api %d\n", ret);
	csi_api_close(phy_id, csi_id);
	return ret;
}

int csi_api_close(uint32_t phy_id, int sensor_id)
{
	int ret = 0;
	struct csi_dt_node_info *dt_info = csi_get_dt_node_data(sensor_id);
	if (!dt_info) {
		pr_err("fail to get valid phy ptr\n");
		return -EINVAL;
	}

	if (CSI_PATTERN_ENABLE)
		csi_ipg_mode_cfg(sensor_id, 0, 1, 640, 480);
	if (0){
		csi_close(sensor_id);
		csi_phy_power_down(dt_info, sensor_id, 1);
		csi_mipi_clk_disable(sensor_id);
	}
	pr_info("csi api close ret: %d\n", ret);

	return ret;
}

int csi_api_switch(int sensor_id)
{
	return 0;
}

#if 0
static int __init sprd_csi_init(void)
{
	struct device_node *dn = NULL;
	int sensor_id = 0;
	int phy_id = 0;

	pr_info("entry %s\n", __func__);
	dn = of_find_node_by_name(NULL, "csi00");
	if (!dn)
		pr_err("%s :error getting csi_node\n", __func__);

	csi_api_dt_node_init(NULL, dn, sensor_id, phy_id);

	fund();
	pr_info("exit %s\n", __func__);

	return 0;
}

static void __exit sprd_csi_exit(void)
{
	pr_info("exit -> %s\n", __func__);
}

module_init(sprd_csi_init);
module_exit(sprd_csi_exit);
MODULE_DESCRIPTION("CSI");
MODULE_LICENSE("GPL");
#endif
