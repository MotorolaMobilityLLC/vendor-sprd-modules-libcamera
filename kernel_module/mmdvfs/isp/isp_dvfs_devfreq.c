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

#include "isp_dvfs.h"

static int isp_dvfs_target(struct device *dev, unsigned long *freq,
	u32 flags);
static int isp_dvfs_get_dev_status(struct device *dev,
	struct devfreq_dev_status *stat);
static int isp_dvfs_get_cur_freq(struct device *dev, unsigned long *freq);
static int isp_dvfs_probe(struct platform_device *pdev);
static int isp_dvfs_remove(struct platform_device *pdev);

static const struct of_device_id isp_dvfs_of_match[] = {
	{ .compatible = "sprd,hwdvfs-isp" },
	{ },
	};
MODULE_DEVICE_TABLE(of, isp_dvfs_of_match);


static struct platform_driver isp_dvfs_driver = {
	.probe    = isp_dvfs_probe,
	.remove     = isp_dvfs_remove,
	.driver = {
	.name = "isp-dvfs",
	.of_match_table = isp_dvfs_of_match,
},
};

static struct devfreq_dev_profile isp_dvfs_profile = {
	.polling_ms         = 200,
	.target             = isp_dvfs_target,
	.get_dev_status     = isp_dvfs_get_dev_status,
	.get_cur_freq       = isp_dvfs_get_cur_freq,
};

static int isp_dvfs_target(struct device *dev, unsigned long *freq,
	u32 flags)
{
	struct isp_dvfs *isp = dev_get_drvdata(dev);
	unsigned long target_freq = *freq, target_volt = 0;
	int err = 0;
	pr_info("devfreq_dev_profile-->target,freq=%lu isp->freq = %lu\n", *freq,isp->freq);

	if (isp->freq == *freq)
		return 0;

	mutex_lock(&isp->lock);
	isp->dvfs_ops->updata_target_freq(isp->devfreq, target_volt,
		target_freq, FALSE);

	if (err) {
		dev_err(dev, "Cannot to set freq:%lu to isp, err: %d\n",
		target_freq, err);
		goto out;
	}

	isp->freq = target_freq;
	isp->volt = target_volt;

out:
	mutex_unlock(&isp->lock);
	return err;
}

int isp_dvfs_get_dev_status(struct device *dev,
	struct devfreq_dev_status *stat)
{
	struct isp_dvfs *isp = dev_get_drvdata(dev);
	struct devfreq_event_data edata;
	int ret = 0;

	pr_info("devfreq_dev_profile-->get_dev_status\n");

	ret = devfreq_event_get_event(isp->edev, &edata);
	if (ret < 0)
		return ret;

	stat->current_frequency = isp->freq;
	stat->busy_time = edata.load_count;
	stat->total_time = edata.total_count;

	return ret;
}

static int isp_dvfs_get_cur_freq(struct device *dev, unsigned long *freq)
{
	struct isp_dvfs *isp = dev_get_drvdata(dev);

	*freq = isp->freq;
	pr_info("devfreq_dev_profile-->get_cur_freq,*freq=%lu\n", *freq);
	return 0;
}


static int isp_dvfs_notify_callback(struct notifier_block *nb,
	unsigned long type, void *data)
{
	struct isp_dvfs *isp = container_of(nb, struct isp_dvfs, pw_nb);


	switch (type) {

		case MMSYS_POWER_ON:
			if (isp->dvfs_ops != NULL && isp->dvfs_ops->power_on_nb != NULL)
				isp->dvfs_ops->power_on_nb(isp->devfreq);
			break;
		case MMSYS_POWER_OFF:
			if (isp->dvfs_ops  !=  NULL &&
				isp->dvfs_ops->power_off_nb != NULL)
				isp->dvfs_ops->power_off_nb(isp->devfreq);
			break;

		default:
			return -EINVAL;
	}

	return NOTIFY_OK;
}

static int isp_dvfs_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;
	struct isp_dvfs *isp;
	int ret;

	pr_info("isp-dvfs initialized\n");

	isp = devm_kzalloc(dev, sizeof(*isp), GFP_KERNEL);
	if (!isp)
		return -ENOMEM;

	mutex_init(&isp->lock);

#ifdef SUPPORT_SWDVFS

	isp->clk_isp_core = devm_clk_get(dev, "clk_isp_core");
	if (IS_ERR(isp->clk_isp_core))
		dev_err(dev, "Cannot get the clk_isp_core clk\n");

	of_property_read_u32(np, "sprd,dvfs-wait-window",
	&isp->dvfs_wait_window);
#endif
#if 0
	of_property_read_u32(np, "sprd,dvfs-gfree-wait-delay",
	&isp->isp_dvfs_para.ip_coffe.gfree_wait_delay);
	of_property_read_u32(np, "sprd,dvfs-freq-upd-hdsk-en",
	&isp->isp_dvfs_para.ip_coffe.freq_upd_hdsk_en);
	of_property_read_u32(np, "sprd,dvfs-freq-upd-delay-en",
	&isp->isp_dvfs_para.ip_coffe.freq_upd_delay_en);
	of_property_read_u32(np, "sprd,dvfs-freq-upd-en-byp",
	&isp->isp_dvfs_para.ip_coffe.freq_upd_en_byp);
	of_property_read_u32(np, "sprd,dvfs-sw-trig-en",
	&isp->isp_dvfs_para.ip_coffe.sw_trig_en);
	of_property_read_u32(np, "sprd,dvfs-auto-tune",
	&isp->isp_dvfs_para.ip_coffe.auto_tune);
	of_property_read_u32(np, "sprd,dvfs-idle-index-def",
	&isp->isp_dvfs_para.ip_coffe.idle_index_def);
#endif
	of_property_read_u32(np, "sprd,dvfs-work-index-def",
	&isp->isp_dvfs_para.ip_coffe.work_index_def);
	platform_set_drvdata(pdev, isp);
	isp->devfreq = devm_devfreq_add_device(dev,
	&isp_dvfs_profile,
	"isp_dvfs",
	NULL);
	if (IS_ERR(isp->devfreq)) {
		dev_err(dev,
		"failed to add devfreq dev with isp-dvfs governor\n");
		ret = PTR_ERR(isp->devfreq);
		goto err;
	}

	isp->pw_nb.priority = 0;
	isp->pw_nb.notifier_call =  isp_dvfs_notify_callback;
	ret = mmsys_register_notifier(&isp->pw_nb);

	return 0;

err:
	return ret;
}

static int isp_dvfs_remove(struct platform_device *pdev)
{
	pr_err("%s:\n", __func__);

	return 0;
}

#ifndef KO_DEFINE

int isp_dvfs_init(void)
{
	int ret = 0;

	ret = register_ip_dvfs_ops(&isp_dvfs_ops);
	if (ret) {
		pr_err("%s: failed to add ops: %d\n", __func__, ret);
		return ret;
	}

	ret = devfreq_add_governor(&isp_dvfs_gov);
	if (ret) {
		pr_err("%s: failed to add governor: %d\n", __func__, ret);
		return ret;
	}

	ret = platform_driver_register(&isp_dvfs_driver);

	if (ret)
		devfreq_remove_governor(&isp_dvfs_gov);

	return ret;
}

void isp_dvfs_exit(void)
{
	int ret = 0;

	platform_driver_unregister(&isp_dvfs_driver);

	ret = devfreq_remove_governor(&isp_dvfs_gov);
	if (ret)
		pr_err("%s: failed to remove governor: %d\n", __func__, ret);

	ret = unregister_ip_dvfs_ops(&isp_dvfs_ops);
	if (ret)
		pr_err("%s: failed to remove ops: %d\n", __func__, ret);
}

#else
int __init isp_dvfs_init(void)
{
	int ret = 0;

	ret = register_ip_dvfs_ops(&isp_dvfs_ops);
	if (ret) {
		pr_err("%s: failed to add ops: %d\n", __func__, ret);
		return ret;
	}

	ret = devfreq_add_governor(&isp_dvfs_gov);
	if (ret) {
		pr_err("%s: failed to add governor: %d\n", __func__, ret);
		return ret;
	}

	ret = platform_driver_register(&isp_dvfs_driver);

	if (ret)
		devfreq_remove_governor(&isp_dvfs_gov);

	return ret;
}

void __exit isp_dvfs_exit(void)
{
	int ret = 0;

	platform_driver_unregister(&isp_dvfs_driver);

	ret = devfreq_remove_governor(&isp_dvfs_gov);
	if (ret)
		pr_err("%s: failed to remove governor: %d\n", __func__, ret);

	ret = unregister_ip_dvfs_ops(&isp_dvfs_ops);
	if (ret)
		pr_err("%s: failed to remove ops: %d\n", __func__, ret);
}



module_init(isp_dvfs_init);
module_exit(isp_dvfs_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Sprd isp devfreq driver");
MODULE_AUTHOR("Multimedia_Camera@Spreadtrum");

#endif

