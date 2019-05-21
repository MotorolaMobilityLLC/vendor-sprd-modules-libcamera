/*
 * Copyright (C) 2019 Unisoc Communications Inc.
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

#include "mmsys_dvfs.h"
#include "sharkl5pro_top_dvfs_reg.h"
#include "mmsys_dvfs_comm.h"


static int mmsys_dvfs_target(struct device *dev, unsigned long *freq,
			u32 flags);
static int mmsys_dvfs_get_dev_status(struct device *dev,
			struct devfreq_dev_status *stat);
static int mmsys_dvfs_get_cur_freq(struct device *dev, unsigned long *freq);
static int mmsys_dvfs_probe(struct platform_device *pdev);
static int mmsys_dvfs_remove(struct platform_device *pdev);

static const struct of_device_id mmsys_dvfs_of_match[] = {
	{ .compatible = "sprd,hwdvfs-mmsys" },
	{ },
};
MODULE_DEVICE_TABLE(of, mmsys_dvfs_of_match);

static struct platform_driver mmsys_dvfs_driver = {
	.probe = mmsys_dvfs_probe,
	.remove = mmsys_dvfs_remove,
	.driver = {
		.name = "mmsys-dvfs",
		.of_match_table = mmsys_dvfs_of_match,
	},
};

static struct devfreq_dev_profile mmsys_dvfs_profile = {
	.polling_ms = 200,
	.target = mmsys_dvfs_target,
	.get_dev_status = mmsys_dvfs_get_dev_status,
	.get_cur_freq = mmsys_dvfs_get_cur_freq,
	};

static int mmsys_dvfs_target(struct device *dev, unsigned long *freq,
		u32 flags)
{
	DVFS_TRACE("%s:\n", __func__);
	/* TODO for sw dvfs  & sys dvfs */
	return 0;
}

int mmsys_dvfs_get_dev_status(struct device *dev,
			struct devfreq_dev_status *stat)
{
	/* TODO for sw dvfs  & sys dvfs */
	DVFS_TRACE("devfreq_dev_profile-->get_dev_status\n");
	return 0;
}

static int mmsys_dvfs_get_cur_freq(struct device *dev, unsigned long *freq)
{
	/* TODO for sw dvfs & sys dvfs */
	DVFS_TRACE("devfreq_dev_profile-->get_cur_freq\n");
	return 0;
}

static int mmsys_dvfs_notify_callback(struct notifier_block *nb,
						unsigned long type, void *data)
{
	struct mmsys_dvfs *mmsys = container_of(nb, struct mmsys_dvfs, pw_nb);

	DVFS_TRACE("%s: %lu\n", __FILE__, type);
	switch (type) {
	case MMSYS_POWER_ON:
		if (mmsys->dvfs_ops  !=  NULL &&
			mmsys->dvfs_ops->power_on_nb != NULL) {
			mmsys->dvfs_ops->power_on_nb(mmsys->devfreq);
		}
		break;
	case MMSYS_POWER_OFF:
		if (mmsys->dvfs_ops  !=  NULL &&
			mmsys->dvfs_ops->power_off_nb != NULL) {
			mmsys->dvfs_ops->power_off_nb(mmsys->devfreq);
		}
		break;
	default:
		return -EINVAL;
	}

	return NOTIFY_OK;
	}

static int mmsys_dvfs_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;
	struct mmsys_dvfs *mmsys;
	void __iomem *reg_base = NULL;
	struct resource reg_res = {0};
	int ret = 0;

	DVFS_TRACE("mmsys-dvfs initialized\n");

	mmsys = devm_kzalloc(dev, sizeof(*mmsys), GFP_KERNEL);
	if (!mmsys)
		return -ENOMEM;

	mutex_init(&mmsys->lock);

#ifdef SUPPORT_SWDVFS
	mmsys->clk_mmsys_core = devm_clk_get(dev, "clk_mmsys_core");
	if (IS_ERR(mmsys->clk_mmsys_core)) {
		dev_err(dev, "Cannot get the clk_mmsys_core clk\n");
	};
#endif

	of_property_read_u32(np, "sprd,dvfs-sys-sw-dvfs-en",
		&mmsys->mmsys_dvfs_para.sys_sw_dvfs_en);
	of_property_read_u32(np, "sprd,dvfs-sys-dvfs-hold-en",
		&mmsys->mmsys_dvfs_para.sys_dvfs_hold_en);
	of_property_read_u32(np, "sprd,dvfs-sys-dvfs-clk-gate-ctrl",
		&mmsys->mmsys_dvfs_para.sys_dvfs_clk_gate_ctrl);
	of_property_read_u32(np, "sprd,dvfs-sys-dvfs-wait_window",
		&mmsys->mmsys_dvfs_para.sys_dvfs_wait_window);
	of_property_read_u32(np, "sprd,dvfs-sys-dvfs-min_volt",
		&mmsys->mmsys_dvfs_para.sys_dvfs_min_volt);

	reg_res.start = REGS_MM_DVFS_AHB_START;
	reg_res.end = REGS_MM_DVFS_AHB_END;
	reg_base = ioremap(reg_res.start,
	reg_res.end - reg_res.start + 1);
	if (!reg_base) {
		pr_err("fail to map mmsys dvfs ahb reg base\n");
		goto err_iounmap;
	}
	g_mmreg_map.mmdvfs_ahb_regbase = (unsigned long) reg_base;


	reg_res.start = REGS_MM_TOP_DVFS_START;
	reg_res.end = REGS_MM_TOP_DVFS_END;
	reg_base = ioremap(reg_res.start,
			reg_res.end - reg_res.start + 1);
	if (!reg_base) {
		pr_err("fail to map mmsys dvfs top reg base\n");
		goto err_iounmap;
	}
	g_mmreg_map.mmdvfs_top_regbase = (unsigned long) reg_base;


	reg_res.start = REGS_MM_AHB_REG_START;
	reg_res.end = REGS_MM_AHB_REG_END;
	reg_base = ioremap(reg_res.start,
			reg_res.end - reg_res.start + 1);
	if (!reg_base) {
		pr_err("fail to map mm_ahb reg base\n");
		goto err_iounmap;
	}
	g_mmreg_map.mm_ahb_regbase = (unsigned long) reg_base;

	reg_res.start  = REGS_MM_POWER_ON_REG_START;
	reg_res.end   =  REGS_MM_POWER_ON_REG_END;
	reg_base = ioremap(reg_res.start,
			reg_res.end - reg_res.start + 1);
	if (!reg_base) {
		pr_err("fail to map mm_ahb reg base\n");
		goto err_iounmap;
	}
	g_mmreg_map.mm_power_on_regbase = (unsigned long) reg_base;

	reg_res.start  = REGS_MM_POWER_REG_START;
	reg_res.end   =  REGS_MM_POWER_REG_END;
	reg_base = ioremap(reg_res.start,
			reg_res.end - reg_res.start + 1);
	if (!reg_base) {
		pr_err("fail to map mm_ahb reg base\n");
		goto err_iounmap;
	}
	g_mmreg_map.mm_power_regbase = (unsigned long) reg_base;

	platform_set_drvdata(pdev, mmsys);
	mmsys->devfreq = devm_devfreq_add_device(dev,
		&mmsys_dvfs_profile, "mmsys_dvfs", NULL);
	if (IS_ERR(mmsys->devfreq)) {
		dev_err(dev,
		"failed to add devfreq dev with mmsys-dvfs governor\n");
		ret = PTR_ERR(mmsys->devfreq);
		goto err;
	}

	mmsys->pw_nb.priority = 0;
	mmsys->pw_nb.notifier_call =  mmsys_dvfs_notify_callback;
	ret = mmsys_register_notifier(&mmsys->pw_nb);

	return 0;

err_iounmap:
err:
	return ret;
}

static int mmsys_dvfs_power(struct notifier_block *self, unsigned long event,
		void *ptr)
{
	int ret = 0;

	DVFS_TRACE("%s:on=%ld\n", __func__, event);
/*
*	Need Add this event in PowerDomain
*	if (event == _E_PW_ON)
*		mmsys_notify_call_chain(MMSYS_POWER_ON);
*	else
*		mmsys_notify_call_chain(MMSYS_POWER_OFF);
*/

return ret;
}

static struct notifier_block dvfs_power_notifier = {
	.notifier_call = mmsys_dvfs_power,
};

static int mmsys_dvfs_remove(struct platform_device *pdev)
{
	DVFS_TRACE("%s:", __func__);
	return 0;
}

#ifndef KO_DEFINE
int  mmsys_dvfs_init(void)
{
	int ret = 0;

	ret = register_ip_dvfs_ops(&mmsys_dvfs_ops);
	if (ret) {
		pr_err("%s: failed to add ops: %d\n", __func__, ret);
	return ret;
	}

	ret = mmsys_register_notifier(&dvfs_power_notifier);
	if (ret) {
		pr_err("%s: failed to add dvfs_power: %d\n", __func__, ret);
	return ret;
	}

	ret = devfreq_add_governor(&mmsys_dvfs_gov);
	if (ret) {
		pr_err("%s: failed to add governor: %d\n", __func__, ret);
		return ret;
	}

	ret = platform_driver_register(&mmsys_dvfs_driver);
	if (ret)
		devfreq_remove_governor(&mmsys_dvfs_gov);

	return ret;
}

void  mmsys_dvfs_exit(void)
{
	int ret = 0;

	platform_driver_unregister(&mmsys_dvfs_driver);

	ret = devfreq_remove_governor(&mmsys_dvfs_gov);
	if (ret)
		pr_err("%s: failed to remove governor: %d\n", __func__, ret);

	ret = unregister_ip_dvfs_ops(&mmsys_dvfs_ops);
	if (ret)
		pr_err("%s: failed to remove ops: %d\n", __func__, ret);

	ret = mmsys_unregister_notifier((&dvfs_power_notifier));
	if (ret)
		pr_err("%s: failed to remove mm_dvfs_power: %d\n",
			__func__, ret);
}

#else


int __init mmsys_dvfs_init(void)
{
	int ret = 0;

	ret = register_ip_dvfs_ops(&mmsys_dvfs_ops);
	if (ret) {
		pr_err("%s: failed to add ops: %d\n", __func__, ret);
	return ret;
	}

	ret = devfreq_add_governor(&mmsys_dvfs_gov);
	if (ret) {
		pr_err("%s: failed to add governor: %d\n", __func__, ret);
		return ret;
	}

	ret = platform_driver_register(&mmsys_dvfs_driver);
	if (ret)
		devfreq_remove_governor(&mmsys_dvfs_gov);

	return ret;
}

void __exit mmsys_dvfs_exit(void)
{
	int ret = 0;

	platform_driver_unregister(&mmsys_dvfs_driver);
	ret = devfreq_remove_governor(&mmsys_dvfs_gov);
	if (ret)
		pr_err("%s: failed to remove governor: %d\n", __func__, ret);

	ret = unregister_ip_dvfs_ops(&mmsys_dvfs_ops);
	if (ret)
		pr_err("%s: failed to remove ops: %d\n", __func__, ret);

}

#endif


#ifndef KO_DEFINE

int __init mmsys_module_init(void)
{
	int ret = 0;

	DVFS_TRACE("enter mmsys_moudule_init\n");
	ret = mmsys_dvfs_init();
	ret = cpp_dvfs_init();
	ret = fd_dvfs_init();
	ret = jpg_dvfs_init();
	ret = isp_dvfs_init();
	ret = dcam_dvfs_init();
	ret = dcamaxi_dvfs_init();
	ret = mtx_dvfs_init();
	DVFS_TRACE("end mmsys_moudule_init\n");

	return ret;
}

void __exit mmsys_module_exit(void)
{
	DVFS_TRACE("enter mmsys_moudule_exit\n");
	mmsys_dvfs_exit();
	cpp_dvfs_exit();
	fd_dvfs_exit();
	isp_dvfs_exit();
	jpg_dvfs_exit();
	dcam_dvfs_exit();
	dcamaxi_dvfs_exit();
	mtx_dvfs_exit();
	DVFS_TRACE("end mmsys_moudule_exit\n");
}

module_init(mmsys_module_init);
module_exit(mmsys_module_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Sprd mmsys devfreq driver");
MODULE_AUTHOR("Multimedia_Camera@UNISOC");

#endif
