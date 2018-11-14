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

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/mfd/syscon.h>
/* #include <linux/mfd/syscon/sprd-glb.h> */
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <sprd_mm.h>

#include "cam_pw_domain.h"
#include "mm_ahb.h"

#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt) "camera_pw: %d %d %s : "\
	fmt, current->pid, __LINE__, __func__


struct cam_pw_domain_info {
	atomic_t users_pw;
	atomic_t users_clk;
	unsigned int chip_id0;
	unsigned int chip_id1;
	struct mutex client_lock;
	struct regmap *cam_ahb_gpr;
	struct regmap *aon_apb_gpr;
	struct regmap *pmu_apb_gpr;
	struct clk *cam_clk_cphy_cfg_gate_eb;
	struct clk *cam_mm_eb;
	struct clk *cam_ahb_clk;
	struct clk *cam_ahb_clk_default;
	struct clk *cam_ahb_clk_parent;
	struct clk *cam_emc_clk;
	struct clk *cam_emc_clk_default;
	struct clk *cam_emc_clk_parent;
};

static struct cam_pw_domain_info *cam_pw;

/* test on haps */
#if	(defined(TEST_ON_HAPS))
unsigned int read_hwaddress(unsigned int addr)
{
	void __iomem *io_tmp = NULL;
	unsigned int val;

	io_tmp = ioremap_nocache(addr, 0x4);
	val = __raw_readl(io_tmp);
	iounmap(io_tmp);

	return val;
}
void write_hwaddress(unsigned int addr, unsigned int val)
{
	void __iomem *io_tmp = NULL;

	io_tmp = ioremap_nocache(addr, 0x4);
	__raw_writel(val, io_tmp);
	mb(); /* asm/barrier.h */
	val = __raw_readl(io_tmp);
	iounmap(io_tmp);
}
void write_hwaddress_mask(unsigned int addr, unsigned int mask,
				unsigned int val)
{
	void __iomem *io_tmp = NULL;
	unsigned int tmp;

	io_tmp = ioremap_nocache(addr, 0x4);
	tmp = __raw_readl(io_tmp);
	val = (val & mask) | (tmp & (~mask));
	__raw_writel(val, io_tmp);
	val = __raw_readl(io_tmp);
	iounmap(io_tmp);
}
#endif

int sprd_cam_pw_domain_init(struct platform_device *pdev)
{
#if	(defined(TEST_ON_HAPS))
	cam_pw = devm_kzalloc(&pdev->dev, sizeof(*cam_pw), GFP_KERNEL);
	if (!cam_pw)
		return -ENOMEM;
	pr_debug(", E\n");

	return 0;
#else /* normal */
	struct regmap *cam_ahb_gpr;
	struct regmap *pmu_apb_gpr;
	struct regmap *aon_apb_gpr;

	cam_pw = devm_kzalloc(&pdev->dev, sizeof(*cam_pw), GFP_KERNEL);
	if (!cam_pw)
		return -ENOMEM;
	pr_debug(", E\n");
	cam_pw->cam_clk_cphy_cfg_gate_eb =
		devm_clk_get(&pdev->dev, "clk_cphy_cfg_gate_eb");
	if (IS_ERR(cam_pw->cam_clk_cphy_cfg_gate_eb))
		return PTR_ERR(cam_pw->cam_clk_cphy_cfg_gate_eb);
/*
	cam_pw->cam_ckg_eb = devm_clk_get(&pdev->dev, "clk_gate_eb");
	if (IS_ERR(cam_pw->cam_ckg_eb))
		return PTR_ERR(cam_pw->cam_ckg_eb);
*/
	cam_pw->cam_mm_eb = devm_clk_get(&pdev->dev, "clk_mm_eb");
	if (IS_ERR(cam_pw->cam_mm_eb))
		return PTR_ERR(cam_pw->cam_mm_eb);

	cam_pw->cam_ahb_clk = devm_clk_get(&pdev->dev, "clk_mm_ahb");
	if (IS_ERR(cam_pw->cam_ahb_clk))
		return PTR_ERR(cam_pw->cam_ahb_clk);

	cam_pw->cam_ahb_clk_parent =
		devm_clk_get(&pdev->dev, "clk_mm_ahb_parent");
	if (IS_ERR(cam_pw->cam_ahb_clk_parent))
		return PTR_ERR(cam_pw->cam_ahb_clk_parent);

	cam_pw->cam_ahb_clk_default = clk_get_parent(cam_pw->cam_ahb_clk);
	if (IS_ERR(cam_pw->cam_ahb_clk_default))
		return PTR_ERR(cam_pw->cam_ahb_clk_default);

	cam_ahb_gpr = syscon_regmap_lookup_by_phandle(pdev->dev.of_node,
							"sprd,cam-ahb-syscon");
	if (IS_ERR(cam_ahb_gpr))
		return PTR_ERR(cam_ahb_gpr);
	cam_pw->cam_ahb_gpr = cam_ahb_gpr;

	pmu_apb_gpr = syscon_regmap_lookup_by_phandle(pdev->dev.of_node,
							"sprd,syscon-pmu-apb");
	if (IS_ERR(pmu_apb_gpr))
		return PTR_ERR(pmu_apb_gpr);
	cam_pw->pmu_apb_gpr = pmu_apb_gpr;

	aon_apb_gpr = syscon_regmap_lookup_by_phandle(pdev->dev.of_node,
							"sprd,aon-apb-syscon");
	if (IS_ERR(aon_apb_gpr)) {
		pr_err("%s: fail to find aon apb node\n", __func__);
		return PTR_ERR(aon_apb_gpr);
	}
	cam_pw->aon_apb_gpr = aon_apb_gpr;

	mutex_init(&cam_pw->client_lock);

	pr_info("pw_domain_init ok\n");

	return 0;
#endif
}
EXPORT_SYMBOL(sprd_cam_pw_domain_init);

int sprd_cam_pw_domain_deinit(void)
{
	cam_pw = NULL;

	return 0;
}
EXPORT_SYMBOL(sprd_cam_pw_domain_deinit);

int sprd_cam_pw_off(void)
{
#if	(defined(TEST_ON_HAPS))
	if (atomic_dec_return(&cam_pw->users_pw) == 0)
		pr_info("\n");

	return 0;
#else /* normal */
	int ret = 0;
	unsigned int power_state1;
	unsigned int power_state2;
	unsigned int power_state3;
	unsigned int read_count = 0;
	unsigned int val = 0;

	pr_info("user count:%d\n", atomic_read(&cam_pw->users_pw));

	mutex_lock(&cam_pw->client_lock);

	if (atomic_dec_return(&cam_pw->users_pw) == 0) {
		regmap_update_bits(cam_pw->pmu_apb_gpr,
			REG_PMU_APB_PD_MM_SYS_CFG,
			BIT_PMU_APB_PD_MM_SYS_AUTO_SHUTDOWN_EN,
			~(unsigned int)BIT_PMU_APB_PD_MM_SYS_AUTO_SHUTDOWN_EN);
		regmap_update_bits(cam_pw->pmu_apb_gpr,
			REG_PMU_APB_PD_MM_SYS_CFG,
			BIT_PMU_APB_PD_MM_SYS_FORCE_SHUTDOWN,
			BIT_PMU_APB_PD_MM_SYS_FORCE_SHUTDOWN);

		do {
			cpu_relax();
			usleep_range(300, 350);
			read_count++;

			ret = regmap_read(cam_pw->pmu_apb_gpr,
						REG_PMU_APB_PD_STATE, &val);
			if (ret)
				goto err_pw_off;
			power_state1 = val &  BIT_PMU_APB_PD_MM_SYS_STATE;
			pr_debug("power_state1=0x%x\n", power_state1);
			ret = regmap_read(cam_pw->pmu_apb_gpr,
						REG_PMU_APB_PD_STATE, &val);
			if (ret)
				goto err_pw_off;
			power_state2 = val &  BIT_PMU_APB_PD_MM_SYS_STATE;

			ret = regmap_read(cam_pw->pmu_apb_gpr,
						REG_PMU_APB_PD_STATE, &val);
			if (ret)
				goto err_pw_off;
			power_state3 = val &  BIT_PMU_APB_PD_MM_SYS_STATE;
		} while (((power_state1 != PD_MM_DOWN_FLAG) &&
			(read_count < 10)) ||
				(power_state1 != power_state2) ||
				(power_state2 != power_state3));

		if (power_state1 != PD_MM_DOWN_FLAG) {
			pr_err("power off failed 0x%x\n", power_state1);
			ret = -1;
			goto err_pw_off;
		}
	} else {
		pr_info("other camera module is working\n");
	}
	mutex_unlock(&cam_pw->client_lock);

	return 0;

err_pw_off:
	pr_err("power off failed, ret: %d, count: %d!\n", ret, read_count);
	mutex_unlock(&cam_pw->client_lock);

	return ret;
#endif
}
EXPORT_SYMBOL(sprd_cam_pw_off);

int sprd_cam_pw_on(void)
{
#if	(defined(TEST_ON_HAPS))
	unsigned int state1, state2, state3;
	unsigned int read_count = 0;

	if (atomic_inc_return(&cam_pw->users_pw) != 1)
		return 0;
	/* pmu */
	write_hwaddress_mask(0x327E0024, BIT_24 | BIT_25, 0);
	/* wait state */
	do {
		cpu_relax();
		usleep_range(300, 350);
		state1 = read_hwaddress(
			0x327E010C) & BIT_PMU_APB_PD_MM_SYS_STATE;
		cpu_relax();
		state2 = read_hwaddress(
			0x327E010C) & BIT_PMU_APB_PD_MM_SYS_STATE;
		cpu_relax();
		state3 = read_hwaddress(
			0x327E010C) & BIT_PMU_APB_PD_MM_SYS_STATE;
		read_count++;
	} while (state1 != state2 || state2 != state3 ||
		(state1 && read_count < 100));
	/* aon */
	write_hwaddress_mask(0x327d0000, BIT_9, BIT_9);
	pr_info("power on. count = %d\n", read_count);

	return 0;
#else /* normal */
	int ret = 0;
	unsigned int power_state1;
	unsigned int power_state2;
	unsigned int power_state3;
	unsigned int read_count = 0;
	unsigned int val = 0;

	pr_info("power user count:%d\n", atomic_read(&cam_pw->users_pw));

	mutex_lock(&cam_pw->client_lock);

	if (atomic_inc_return(&cam_pw->users_pw) == 1) {
		/* cam domain power on */
		regmap_update_bits(cam_pw->pmu_apb_gpr,
			REG_PMU_APB_PD_MM_SYS_CFG,
			BIT_PMU_APB_PD_MM_SYS_AUTO_SHUTDOWN_EN,
			~(unsigned int)BIT_PMU_APB_PD_MM_SYS_AUTO_SHUTDOWN_EN);
		regmap_update_bits(cam_pw->pmu_apb_gpr,
			REG_PMU_APB_PD_MM_SYS_CFG,
			BIT_PMU_APB_PD_MM_SYS_FORCE_SHUTDOWN,
			~(unsigned int)BIT_PMU_APB_PD_MM_SYS_FORCE_SHUTDOWN);

		do {
			cpu_relax();
			usleep_range(300, 350);
			read_count++;

			ret = regmap_read(cam_pw->pmu_apb_gpr,
						REG_PMU_APB_PD_STATE, &val);
			if (ret)
				goto err_pw_on;
			power_state1 = val &  BIT_PMU_APB_PD_MM_SYS_STATE;
			pr_debug("power_state1=0x%x\n", power_state1);
			ret = regmap_read(cam_pw->pmu_apb_gpr,
						REG_PMU_APB_PD_STATE, &val);
			if (ret)
				goto err_pw_on;
			power_state2 = val &  BIT_PMU_APB_PD_MM_SYS_STATE;

			ret = regmap_read(cam_pw->pmu_apb_gpr,
						REG_PMU_APB_PD_STATE, &val);
			if (ret)
				goto err_pw_on;
			power_state3 = val &  BIT_PMU_APB_PD_MM_SYS_STATE;

		} while ((power_state1 && read_count < 10) ||
				(power_state1 != power_state2) ||
				(power_state2 != power_state3));

		if (power_state1) {
			pr_err("cam domain pw on failed 0x%x\n", power_state1);
			ret = -1;
			goto err_pw_on;
		}
	} else {
		pr_info("already power on\n");
	}
	mutex_unlock(&cam_pw->client_lock);

	return 0;

err_pw_on:
	atomic_dec_return(&cam_pw->users_pw);
	pr_info("cam domain, failed to power on\n");
	mutex_unlock(&cam_pw->client_lock);

	return ret;
#endif
}
EXPORT_SYMBOL(sprd_cam_pw_on);

int sprd_cam_domain_eb(void)
{
#if	(defined(TEST_ON_HAPS))
	if (atomic_inc_return(&cam_pw->users_clk) != 1)
		return 0;

	write_hwaddress_mask(0x62200000, 0x3FC, 0x3FC); /* D2:D9 */
	write_hwaddress_mask(0x62200008, 0xF8, 0xF8); /* D3:D7 */
	pr_info("enable\n");

	return 0;
#else /* normal */
#if 0
	unsigned int rst_bit = 0;

	pr_debug("clk users count:%d\n", atomic_read(&cam_pw->users_clk));

	mutex_lock(&cam_pw->client_lock);

	if (atomic_inc_return(&cam_pw->users_clk) == 1) {
		/* mm bus enable */
		clk_prepare_enable(cam_pw->cam_mm_eb);

		/* cam CKG enable */
		clk_prepare_enable(cam_pw->cam_ckg_eb);

		clk_prepare_enable(cam_pw->cam_clk_cphy_cfg_gate_eb);

		/* config cam ahb clk */
		clk_set_parent(cam_pw->cam_ahb_clk, cam_pw->cam_ahb_clk_parent);
		clk_prepare_enable(cam_pw->cam_ahb_clk);

		/* clock for anlg_phy_g7_controller */
		regmap_update_bits(cam_pw->aon_apb_gpr,
			REG_AON_APB_APB_EB2,
			BIT_AON_APB_ANLG_APB_EB,
			BIT_AON_APB_ANLG_APB_EB);

		rst_bit = (0xD << 4) | 0xD;
		regmap_update_bits(cam_pw->cam_ahb_gpr,
			QOS_THREHOLD_MM,
			QOS_THREHOLD_MM_MASK,
			rst_bit);
	}
	mutex_unlock(&cam_pw->client_lock);
#endif
	return 0;
#endif
}
EXPORT_SYMBOL(sprd_cam_domain_eb);

int sprd_cam_domain_disable(void)
{
#if	(defined(TEST_ON_HAPS))
	if (atomic_dec_return(&cam_pw->users_clk) == 0)
		pr_info("\n");

	return 0;
#else /* normal */
	pr_debug("clk users count: %d\n", atomic_read(&cam_pw->users_clk));

	mutex_lock(&cam_pw->client_lock);

	if (atomic_dec_return(&cam_pw->users_clk) == 0) {
		clk_set_parent(cam_pw->cam_ahb_clk,
				cam_pw->cam_ahb_clk_default);
		clk_disable_unprepare(cam_pw->cam_ahb_clk);

		//clk_disable_unprepare(cam_pw->cam_ckg_eb);

		clk_disable_unprepare(cam_pw->cam_clk_cphy_cfg_gate_eb);

		clk_disable_unprepare(cam_pw->cam_mm_eb);
	}
	mutex_unlock(&cam_pw->client_lock);

	return 0;
#endif
}
EXPORT_SYMBOL(sprd_cam_domain_disable);
