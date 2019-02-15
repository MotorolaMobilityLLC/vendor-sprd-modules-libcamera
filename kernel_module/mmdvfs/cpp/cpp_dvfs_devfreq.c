/*
 * Copyright (C) 2018 Spreadtrum Communications Inc.
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

#include "cpp_dvfs.h"

static int cpp_dvfs_target(struct device *dev, unsigned long *freq,
	u32 flags);
static int cpp_dvfs_get_dev_status(struct device *dev,
	struct devfreq_dev_status *stat);
static int cpp_dvfs_get_cur_freq(struct device *dev, unsigned long *freq);

static struct devfreq_dev_profile cpp_dvfs_profile = {
	.polling_ms         = 200,
	.target             = cpp_dvfs_target,
	.get_dev_status     = cpp_dvfs_get_dev_status,
	.get_cur_freq       = cpp_dvfs_get_cur_freq,
};

static int cpp_dvfs_target(struct device *dev, unsigned long *freq,
	u32 flags)
{
	struct cpp_dvfs *cpp = dev_get_drvdata(dev);
	unsigned long target_freq = *freq, target_volt = 0;
	int err = 0;

	pr_info("devfreq_dev_profile-->target,freq=%lu\n", *freq);

	if (cpp->freq == *freq)
		return 0;

	mutex_lock(&cpp->lock);
	cpp->dvfs_ops->updata_target_freq(cpp->devfreq, target_volt,
		target_freq, FALSE);

	if (err) {
		dev_err(dev, "Cannot to set freq:%lu to cpp, err: %d\n",
		target_freq, err);
		goto out;
	}

	cpp->freq = target_freq;
	cpp->volt = target_volt;

out:
	mutex_unlock(&cpp->lock);
	return err;
}

int cpp_dvfs_get_dev_status(struct device *dev,
	struct devfreq_dev_status *stat)
{
	struct cpp_dvfs *cpp = dev_get_drvdata(dev);
	struct devfreq_event_data edata;
	int ret = 0;

	pr_info("devfreq_dev_profile-->get_dev_status\n");

	ret = devfreq_event_get_event(cpp->edev, &edata);
	if (ret < 0)
		return ret;

	stat->current_frequency = cpp->freq;
	stat->busy_time = edata.load_count;
	stat->total_time = edata.total_count;

	return ret;
}

static int cpp_dvfs_get_cur_freq(struct device *dev, unsigned long *freq)
{
	struct cpp_dvfs *cpp = dev_get_drvdata(dev);

	*freq = cpp->freq;
	pr_info("devfreq_dev_profile-->get_cur_freq,*freq=%lu\n", *freq);
	return 0;
}


static int cpp_dvfs_notify_callback(struct notifier_block *nb,
	unsigned long type, void *data)
{
	struct cpp_dvfs *cpp = container_of(nb, struct cpp_dvfs, pw_nb);

	switch (type) {

		case MMSYS_POWER_ON:
			if (cpp->dvfs_ops != NULL && cpp->dvfs_ops->power_on_nb != NULL)
				cpp->dvfs_ops->power_on_nb(cpp->devfreq);
			break;
		case MMSYS_POWER_OFF:
			if (cpp->dvfs_ops  !=  NULL &&
				cpp->dvfs_ops->power_off_nb != NULL)
				cpp->dvfs_ops->power_off_nb(cpp->devfreq);
			break;

		default:
			return -EINVAL;
	}

	return NOTIFY_OK;
}

static int cpp_dvfs_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;
	struct cpp_dvfs *cpp;
	int ret;

	pr_info("cpp-dvfs initialized\n");

	cpp = devm_kzalloc(dev, sizeof(*cpp), GFP_KERNEL);
	if (!cpp)
		return -ENOMEM;

	mutex_init(&cpp->lock);

#ifdef SUPPORT_SWDVFS

	cpp->clk_cpp_core = devm_clk_get(dev, "clk_cpp_core");
	if (IS_ERR(cpp->clk_cpp_core))
		dev_err(dev, "Cannot get the clk_cpp_core clk\n");

	of_property_read_u32(np, "sprd,dvfs-wait-window",
	&cpp->dvfs_wait_window);
#endif
#if 0
	of_property_read_u32(np, "sprd,dvfs-gfree-wait-delay",
	&cpp->cpp_dvfs_para.ip_coffe.gfree_wait_delay);
	of_property_read_u32(np, "sprd,dvfs-freq-upd-hdsk-en",
	&cpp->cpp_dvfs_para.ip_coffe.freq_upd_hdsk_en);
	of_property_read_u32(np, "sprd,dvfs-freq-upd-delay-en",
	&cpp->cpp_dvfs_para.ip_coffe.freq_upd_delay_en);
	of_property_read_u32(np, "sprd,dvfs-freq-upd-en-byp",
	&cpp->cpp_dvfs_para.ip_coffe.freq_upd_en_byp);
	of_property_read_u32(np, "sprd,dvfs-sw-trig-en",
	&cpp->cpp_dvfs_para.ip_coffe.sw_trig_en);
	of_property_read_u32(np, "sprd,dvfs-auto-tune",
	&cpp->cpp_dvfs_para.ip_coffe.auto_tune);
	of_property_read_u32(np, "sprd,dvfs-idle-index-def",
	&cpp->cpp_dvfs_para.ip_coffe.idle_index_def);
#endif
	of_property_read_u32(np, "sprd,dvfs-work-index-def",
	&cpp->cpp_dvfs_para.ip_coffe.work_index_def);
	platform_set_drvdata(pdev, cpp);
	cpp->devfreq = devm_devfreq_add_device(dev,
			&cpp_dvfs_profile,
			"cpp_dvfs",
			NULL);

	if (IS_ERR(cpp->devfreq)) {
		dev_err(dev,
		"failed to add devfreq dev with cpp-dvfs governor\n");
		ret = PTR_ERR(cpp->devfreq);
		pr_err("cpp-dvfs initialized err\n");
		goto err;
	}

	cpp->pw_nb.priority = 0;
	cpp->pw_nb.notifier_call =  cpp_dvfs_notify_callback;
	ret = mmsys_register_notifier(&cpp->pw_nb);

	return 0;

err:
	pr_err("cpp-dvfs probe err\n");
	return ret;
}

static int cpp_dvfs_remove(struct platform_device *pdev)
{
	pr_err("%s:\n", __func__);

	return 0;
}

static const struct of_device_id cpp_dvfs_of_match[] = {
	{ .compatible = "sprd,hwdvfs-cpp" },
	{ },
};

static struct platform_driver cpp_dvfs_driver = {
	.probe    = cpp_dvfs_probe,
	.remove     = cpp_dvfs_remove,
	.driver = {
		.name = "cpp-dvfs",
		.of_match_table = of_match_ptr(cpp_dvfs_of_match),
	},
};

#ifndef KO_DEFINE

int cpp_dvfs_init(void)
{
	int ret = 0;

	ret = register_ip_dvfs_ops(&cpp_dvfs_ops);
	if (ret) {
		pr_err("%s: failed to add ops: %d\n", __func__, ret);
		return ret;
	}

	ret = devfreq_add_governor(&cpp_dvfs_gov);
	if (ret) {
		pr_err("%s: failed to add governor: %d\n", __func__, ret);
		return ret;
	}

	pr_err("enter platform_driver_register ret=%d\n",ret);
	ret = platform_driver_register(&cpp_dvfs_driver);
  	pr_err("exit platform_driver_register ret=%d\n",ret);

	if (ret)
		devfreq_remove_governor(&cpp_dvfs_gov);

	return ret;
}

void cpp_dvfs_exit(void)
{
	int ret = 0;

	platform_driver_unregister(&cpp_dvfs_driver);

	ret = devfreq_remove_governor(&cpp_dvfs_gov);
	if (ret)
		pr_err("%s: failed to remove governor: %d\n", __func__, ret);

	ret = unregister_ip_dvfs_ops(&cpp_dvfs_ops);
	if (ret)
		pr_err("%s: failed to remove ops: %d\n", __func__, ret);
}

#else
int __init cpp_dvfs_init(void)
{
	int ret = 0;

	ret = register_ip_dvfs_ops(&cpp_dvfs_ops);
	if (ret) {
		pr_err("%s: failed to add ops: %d\n", __func__, ret);
		return ret;
	}

	ret = devfreq_add_governor(&cpp_dvfs_gov);
	if (ret) {
		pr_err("%s: failed to add governor: %d\n", __func__, ret);
		return ret;
	}

	ret = platform_driver_register(&cpp_dvfs_driver);

	if (ret)
		devfreq_remove_governor(&cpp_dvfs_gov);

	return ret;
}

void __exit cpp_dvfs_exit(void)
{
	int ret = 0;

	platform_driver_unregister(&cpp_dvfs_driver);

	ret = devfreq_remove_governor(&cpp_dvfs_gov);
	if (ret)
		pr_err("%s: failed to remove governor: %d\n", __func__, ret);

	ret = unregister_ip_dvfs_ops(&cpp_dvfs_ops);
	if (ret)
		pr_err("%s: failed to remove ops: %d\n", __func__, ret);
}

module_init(cpp_dvfs_init);
module_exit(cpp_dvfs_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Sprd cpp devfreq driver");
MODULE_AUTHOR("Evan Liu <evan.liu@unisoc.com>");

#endif
