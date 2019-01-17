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

#include "dcamaxi_dvfs.h"

static int dcamaxi_dvfs_target(struct device *dev, unsigned long *freq,
	u32 flags);
static int dcamaxi_dvfs_get_dev_status(struct device *dev,
	struct devfreq_dev_status *stat);
static int dcamaxi_dvfs_get_cur_freq(struct device *dev, unsigned long *freq);
static int dcamaxi_dvfs_probe(struct platform_device *pdev);
static int dcamaxi_dvfs_remove(struct platform_device *pdev);

static const struct of_device_id dcamaxi_dvfs_of_match[] = {
	{ .compatible = "sprd,sharkl5-hwdvfs-dcam-axi" },
	{ },
	};
MODULE_DEVICE_TABLE(of, dcamaxi_dvfs_of_match);


static struct platform_driver dcamaxi_dvfs_driver = {
	.probe    = dcamaxi_dvfs_probe,
	.remove     = dcamaxi_dvfs_remove,
	.driver = {
	.name = "dcam_axi-dvfs",
	.of_match_table = dcamaxi_dvfs_of_match,
},
};

static struct devfreq_dev_profile dcamaxi_dvfs_profile = {
	.polling_ms         = 200,
	.target             = dcamaxi_dvfs_target,
	.get_dev_status     = dcamaxi_dvfs_get_dev_status,
	.get_cur_freq       = dcamaxi_dvfs_get_cur_freq,
};

static int dcamaxi_dvfs_target(struct device *dev, unsigned long *freq,
	u32 flags)
{
	struct dcamaxi_dvfs *dcamaxi = dev_get_drvdata(dev);
	unsigned long target_freq = *freq, target_volt = 0;
	int err = 0;

	pr_info("devfreq_dev_profile-->target,freq=%lu\n", *freq);

	if (dcamaxi->freq == *freq)
		return 0;

	mutex_lock(&dcamaxi->lock);
	dcamaxi->dvfs_ops->updata_target_freq(dcamaxi->devfreq, target_volt,
		target_freq, FALSE);

	if (err) {
		dev_err(dev, "Cannot to set freq:%lu to dcamaxi, err: %d\n",
		target_freq, err);
		goto out;
	}

	dcamaxi->freq = target_freq;
	dcamaxi->volt = target_volt;

out:
	mutex_unlock(&dcamaxi->lock);
	return err;
}

int dcamaxi_dvfs_get_dev_status(struct device *dev,
	struct devfreq_dev_status *stat)
{
	struct dcamaxi_dvfs *dcamaxi = dev_get_drvdata(dev);
	struct devfreq_event_data edata;
	int ret = 0;

	pr_info("devfreq_dev_profile-->get_dev_status\n");

	ret = devfreq_event_get_event(dcamaxi->edev, &edata);
	if (ret < 0)
		return ret;

	stat->current_frequency = dcamaxi->freq;
	stat->busy_time = edata.load_count;
	stat->total_time = edata.total_count;

	return ret;
}

static int dcamaxi_dvfs_get_cur_freq(struct device *dev, unsigned long *freq)
{
	struct dcamaxi_dvfs *dcamaxi = dev_get_drvdata(dev);

	*freq = dcamaxi->freq;
	pr_info("devfreq_dev_profile-->get_cur_freq,*freq=%lu\n", *freq);
	return 0;
}


static int dcamaxi_dvfs_notify_callback(struct notifier_block *nb,
	unsigned long type, void *data)
{
	struct dcamaxi_dvfs *dcamaxi = container_of(nb, struct dcamaxi_dvfs,
		pw_nb);

	switch (type) {

		case MMSYS_POWER_ON:
			if (dcamaxi->dvfs_ops != NULL &&
				dcamaxi->dvfs_ops->power_on_nb != NULL)
				dcamaxi->dvfs_ops->power_on_nb(dcamaxi->devfreq);
			break;
		case MMSYS_POWER_OFF:
			if (dcamaxi->dvfs_ops  !=  NULL &&
				dcamaxi->dvfs_ops->power_off_nb != NULL)
				dcamaxi->dvfs_ops->power_off_nb(dcamaxi->devfreq);
			break;

		default:
			return -EINVAL;
	}

	return NOTIFY_OK;
}

static int dcamaxi_dvfs_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;
	struct dcamaxi_dvfs *dcamaxi;
	int ret;

	pr_info("dcamaxi-dvfs initialized\n");

	dcamaxi = devm_kzalloc(dev, sizeof(*dcamaxi), GFP_KERNEL);
	if (!dcamaxi)
		return -ENOMEM;

	mutex_init(&dcamaxi->lock);

#ifdef SUPPORT_SWDVFS

	dcamaxi->clk_dcamaxi_core = devm_clk_get(dev, "clk_dcamaxi_core");
	if (IS_ERR(dcamaxi->clk_dcamaxi_core))
		dev_err(dev, "Cannot get the clk_dcamaxi_core clk\n");

	of_property_read_u32(np, "sprd,dvfs-wait-window",
	&dcamaxi->dvfs_wait_window);
#endif
#if 0
	of_property_read_u32(np, "sprd,dvfs-gfree-wait-delay",
	&dcamaxi->dcamaxi_dvfs_para.ip_coffe.gfree_wait_delay);
	of_property_read_u32(np, "sprd,dvfs-freq-upd-hdsk-en",
	&dcamaxi->dcamaxi_dvfs_para.ip_coffe.freq_upd_hdsk_en);
	of_property_read_u32(np, "sprd,dvfs-freq-upd-delay-en",
	&dcamaxi->dcamaxi_dvfs_para.ip_coffe.freq_upd_delay_en);
	of_property_read_u32(np, "sprd,dvfs-freq-upd-en-byp",
	&dcamaxi->dcamaxi_dvfs_para.ip_coffe.freq_upd_en_byp);
	of_property_read_u32(np, "sprd,dvfs-sw-trig-en",
	&dcamaxi->dcamaxi_dvfs_para.ip_coffe.sw_trig_en);
	of_property_read_u32(np, "sprd,dvfs-idle-index-def",
	&dcamaxi->dcamaxi_dvfs_para.ip_coffe.idle_index_def);
#endif
	of_property_read_u32(np, "sprd,dvfs-auto-tune",
	&dcamaxi->dcamaxi_dvfs_para.ip_coffe.auto_tune);
	of_property_read_u32(np, "sprd,dvfs-work-index-def",
	&dcamaxi->dcamaxi_dvfs_para.ip_coffe.work_index_def);

	platform_set_drvdata(pdev, dcamaxi);
	dcamaxi->devfreq = devm_devfreq_add_device(dev,
	&dcamaxi_dvfs_profile,
	"dcam_axi_dvfs",
	NULL);
	if (IS_ERR(dcamaxi->devfreq)) {
		dev_err(dev,
		"failed to add devfreq dev with dcamaxi-dvfs governor\n");
		ret = PTR_ERR(dcamaxi->devfreq);
		goto err;
	}

	dcamaxi->pw_nb.priority = 0;
	dcamaxi->pw_nb.notifier_call =  dcamaxi_dvfs_notify_callback;
	ret = mmsys_register_notifier(&dcamaxi->pw_nb);

	return 0;

err:
	return ret;
}

static int dcamaxi_dvfs_remove(struct platform_device *pdev)
{
	pr_err("%s:\n", __func__);

	return 0;
}

#ifndef KO_DEFINE

int dcamaxi_dvfs_init(void)
{
	int ret = 0;

	ret = register_ip_dvfs_ops(&dcamaxi_dvfs_ops);
	if (ret) {
		pr_err("%s: failed to add ops: %d\n", __func__, ret);
		return ret;
	}

	ret = devfreq_add_governor(&dcamaxi_dvfs_gov);
	if (ret) {
		pr_err("%s: failed to add governor: %d\n", __func__, ret);
		return ret;
	}

	ret = platform_driver_register(&dcamaxi_dvfs_driver);

	if (ret)
		devfreq_remove_governor(&dcamaxi_dvfs_gov);

	return ret;
}

void dcamaxi_dvfs_exit(void)
{
	int ret = 0;

	platform_driver_unregister(&dcamaxi_dvfs_driver);

	ret = devfreq_remove_governor(&dcamaxi_dvfs_gov);
	if (ret)
		pr_err("%s: failed to remove governor: %d\n", __func__, ret);

	ret = unregister_ip_dvfs_ops(&dcamaxi_dvfs_ops);
	if (ret)
		pr_err("%s: failed to remove ops: %d\n", __func__, ret);
}

#else
int __init dcamaxi_dvfs_init(void)
{
	int ret = 0;

	ret = register_ip_dvfs_ops(&dcamaxi_dvfs_ops);
	if (ret) {
		pr_err("%s: failed to add ops: %d\n", __func__, ret);
		return ret;
	}

	ret = devfreq_add_governor(&dcamaxi_dvfs_gov);
	if (ret) {
		pr_err("%s: failed to add governor: %d\n", __func__, ret);
		return ret;
	}

	ret = platform_driver_register(&dcamaxi_dvfs_driver);

	if (ret)
		devfreq_remove_governor(&dcamaxi_dvfs_gov);

	return ret;
}

void __exit dcamaxi_dvfs_exit(void)
{
	int ret = 0;

	platform_driver_unregister(&dcamaxi_dvfs_driver);

	ret = devfreq_remove_governor(&dcamaxi_dvfs_gov);
	if (ret)
		pr_err("%s: failed to remove governor: %d\n", __func__, ret);

	ret = unregister_ip_dvfs_ops(&dcamaxi_dvfs_ops);
	if (ret)
		pr_err("%s: failed to remove ops: %d\n", __func__, ret);
}



module_init(dcamaxi_dvfs_init);
module_exit(dcamaxi_dvfs_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Sprd dcamaxi devfreq driver");
MODULE_AUTHOR("Multimedia_Camera@Spreadtrum");

#endif

