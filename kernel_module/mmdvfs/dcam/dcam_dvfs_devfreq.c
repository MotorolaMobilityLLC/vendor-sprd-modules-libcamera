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

#include "dcam_dvfs.h"

static int dcam_dvfs_target(struct device *dev, unsigned long *freq,
	u32 flags);
static int dcam_dvfs_get_dev_status(struct device *dev,
	struct devfreq_dev_status *stat);
static int dcam_dvfs_get_cur_freq(struct device *dev, unsigned long *freq);
static int dcam_dvfs_probe(struct platform_device *pdev);
static int dcam_dvfs_remove(struct platform_device *pdev);


static const struct of_device_id dcam_dvfs_of_match[] = {
	{ .compatible = "sprd,hwdvfs-dcam_if-sharkl5" },
	{ },
	};
MODULE_DEVICE_TABLE(of, dcam_dvfs_of_match);



static struct platform_driver dcam_dvfs_driver = {
	.probe    = dcam_dvfs_probe,
	.remove     = dcam_dvfs_remove,
	.driver = {
	.name = "dcam_if-dvfs",

	.of_match_table = dcam_dvfs_of_match,

},
};

static struct devfreq_dev_profile dcam_dvfs_profile = {
	.polling_ms         = 200,
	.target             = dcam_dvfs_target,
	.get_dev_status     = dcam_dvfs_get_dev_status,
	.get_cur_freq       = dcam_dvfs_get_cur_freq,
};

static int dcam_dvfs_target(struct device *dev, unsigned long *freq,
	u32 flags)
{
	struct dcam_dvfs *dcam = dev_get_drvdata(dev);
	struct dev_pm_opp *opp;
	unsigned long target_freq, target_volt;
	int err = 0;

	pr_info("devfreq_dev_profile-->target,freq=%lu\n", *freq);

	opp = devfreq_recommended_opp(dev, freq, flags);
	if (IS_ERR(opp)) {
		dev_err(dev, "Failed to find opp for %lu KHz\n", *freq);
		return PTR_ERR(opp);
	}
	target_freq = dev_pm_opp_get_freq(opp);
	target_volt = dev_pm_opp_get_voltage(opp);
	/* dev_pm_opp_put(opp); */
	if (dcam->freq == target_freq)
		return 0;

	mutex_lock(&dcam->lock);
	dcam->dvfs_ops->updata_target_freq(dcam->devfreq, target_volt,
		target_freq, FALSE);

	if (err) {
		dev_err(dev, "Cannot to set freq:%lu to dcam, err: %d\n",
		target_freq, err);
		goto out;
	}

	dcam->freq = target_freq;
	dcam->volt = target_volt;

out:
	mutex_unlock(&dcam->lock);
	return err;
}

int dcam_dvfs_get_dev_status(struct device *dev,
	struct devfreq_dev_status *stat)
{
	struct dcam_dvfs *dcam = dev_get_drvdata(dev);
	struct devfreq_event_data edata;
	int ret = 0;

	pr_info("devfreq_dev_profile-->get_dev_status\n");

	ret = devfreq_event_get_event(dcam->edev, &edata);
	if (ret < 0)
		return ret;

	stat->current_frequency = dcam->freq;
	stat->busy_time = edata.load_count;
	stat->total_time = edata.total_count;

	return ret;
}

static int dcam_dvfs_get_cur_freq(struct device *dev, unsigned long *freq)
{
	struct dcam_dvfs *dcam = dev_get_drvdata(dev);

	*freq = dcam->freq;
	pr_info("devfreq_dev_profile-->get_cur_freq,*freq=%lu\n", *freq);
	return 0;
}


static int dcam_dvfs_notify_callback(struct notifier_block *nb,
	unsigned long type, void *data)
{
	struct dcam_dvfs *dcam = container_of(nb, struct dcam_dvfs, pw_nb);

	switch (type) {

		case MMSYS_POWER_ON:
			if (dcam->dvfs_ops != NULL &&
				dcam->dvfs_ops->power_on_nb != NULL)
				dcam->dvfs_ops->power_on_nb(dcam->devfreq);
			break;
		case MMSYS_POWER_OFF:
			if (dcam->dvfs_ops  !=  NULL &&
				dcam->dvfs_ops->power_off_nb != NULL)
				dcam->dvfs_ops->power_off_nb(dcam->devfreq);
			break;

		default:
			return -EINVAL;
	}

	return NOTIFY_OK;
}

static int dcam_dvfs_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;
	struct dcam_dvfs *dcam;
	int ret;

	pr_info("dcam-dvfs initialized\n");

	dcam = devm_kzalloc(dev, sizeof(*dcam), GFP_KERNEL);
	if (!dcam)
		return -ENOMEM;

	mutex_init(&dcam->lock);

#ifdef SUPPORT_SWDVFS

	dcam->clk_dcam_core = devm_clk_get(dev, "clk_dcam_core");
	if (IS_ERR(dcam->clk_dcam_core))
		dev_err(dev, "Cannot get the clk_dcam_core clk\n");

	of_property_read_u32(np, "sprd,dvfs-wait-window",
	&dcam->dvfs_wait_window);
#endif

	of_property_read_u32(np, "sprd,dvfs-gfree-wait-delay",
	&dcam->dcam_dvfs_para.ip_coffe.gfree_wait_delay);
	of_property_read_u32(np, "sprd,dvfs-freq-upd-hdsk-en",
	&dcam->dcam_dvfs_para.ip_coffe.freq_upd_hdsk_en);
	of_property_read_u32(np, "sprd,dvfs-freq-upd-delay-en",
	&dcam->dcam_dvfs_para.ip_coffe.freq_upd_delay_en);
	of_property_read_u32(np, "sprd,dvfs-freq-upd-en-byp",
	&dcam->dcam_dvfs_para.ip_coffe.freq_upd_en_byp);
	of_property_read_u32(np, "sprd,dvfs-sw-trig-en",
	&dcam->dcam_dvfs_para.ip_coffe.sw_trig_en);
	of_property_read_u32(np, "sprd,dvfs-auto-tune",
	&dcam->dcam_dvfs_para.ip_coffe.auto_tune);
	of_property_read_u32(np, "sprd,dvfs-work-index-def",
	&dcam->dcam_dvfs_para.ip_coffe.work_index_def);
	of_property_read_u32(np, "sprd,dvfs-idle-index-def",
	&dcam->dcam_dvfs_para.ip_coffe.idle_index_def);

	if (dev_pm_opp_of_add_table(dev)) {
		dev_err(dev, "Invalid operating-points in device tree.\n");
		return -EINVAL;
	}

	platform_set_drvdata(pdev, dcam);
	dcam->devfreq = devm_devfreq_add_device(dev,
	&dcam_dvfs_profile,
	"dcam_if_dvfs",
	NULL);
	if (IS_ERR(dcam->devfreq)) {
		dev_err(dev,
		"failed to add devfreq dev with dcam-dvfs governor\n");
		ret = PTR_ERR(dcam->devfreq);
		goto err;
	}

	dcam->pw_nb.priority = 0;
	dcam->pw_nb.notifier_call =  dcam_dvfs_notify_callback;
	ret = mmsys_register_notifier(&dcam->pw_nb);

	return 0;

err:
	dev_pm_opp_of_remove_table(dev);

	return ret;
}

static int dcam_dvfs_remove(struct platform_device *pdev)
{
	pr_err("%s:\n", __func__);

	return 0;
}

#ifndef KO_DEFINE

int dcam_dvfs_init(void)
{
	int ret = 0;

	ret = register_ip_dvfs_ops(&dcam_dvfs_ops);
	if (ret) {
		pr_err("%s: failed to add ops: %d\n", __func__, ret);
		return ret;
	}

	ret = devfreq_add_governor(&dcam_dvfs_gov);
	if (ret) {
		pr_err("%s: failed to add governor: %d\n", __func__, ret);
		return ret;
	}

	ret = platform_driver_register(&dcam_dvfs_driver);

	if (ret)
		devfreq_remove_governor(&dcam_dvfs_gov);

	return ret;
}

void dcam_dvfs_exit(void)
{
	int ret = 0;

	platform_driver_unregister(&dcam_dvfs_driver);

	ret = devfreq_remove_governor(&dcam_dvfs_gov);
	if (ret)
		pr_err("%s: failed to remove governor: %d\n", __func__, ret);

	ret = unregister_ip_dvfs_ops(&dcam_dvfs_ops);
	if (ret)
		pr_err("%s: failed to remove ops: %d\n", __func__, ret);
}

#else
int __init dcam_dvfs_init(void)
{
	int ret = 0;

	ret = register_ip_dvfs_ops(&dcam_dvfs_ops);
	if (ret) {
		pr_err("%s: failed to add ops: %d\n", __func__, ret);
		return ret;
	}

	ret = devfreq_add_governor(&dcam_dvfs_gov);
	if (ret) {
		pr_err("%s: failed to add governor: %d\n", __func__, ret);
		return ret;
	}

	ret = platform_driver_register(&dcam_dvfs_driver);

	if (ret)
		devfreq_remove_governor(&dcam_dvfs_gov);

	return ret;
}

void __exit dcam_dvfs_exit(void)
{
	int ret = 0;

	platform_driver_unregister(&dcam_dvfs_driver);

	ret = devfreq_remove_governor(&dcam_dvfs_gov);
	if (ret)
		pr_err("%s: failed to remove governor: %d\n", __func__, ret);

	ret = unregister_ip_dvfs_ops(&dcam_dvfs_ops);
	if (ret)
		pr_err("%s: failed to remove ops: %d\n", __func__, ret);
}



module_init(dcam_dvfs_init);
module_exit(dcam_dvfs_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Sprd dcam devfreq driver");
MODULE_AUTHOR("Multimedia_Camera@Spreadtrum");

#endif

