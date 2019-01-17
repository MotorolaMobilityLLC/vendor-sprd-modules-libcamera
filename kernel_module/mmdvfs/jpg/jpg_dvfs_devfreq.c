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

#include "jpg_dvfs.h"

static int jpg_dvfs_target(struct device *dev, unsigned long *freq,
	u32 flags);
static int jpg_dvfs_get_dev_status(struct device *dev,
	struct devfreq_dev_status *stat);
static int jpg_dvfs_get_cur_freq(struct device *dev, unsigned long *freq);
static int jpg_dvfs_probe(struct platform_device *pdev);
static int jpg_dvfs_remove(struct platform_device *pdev);

static const struct of_device_id jpg_dvfs_of_match[] = {
	{ .compatible = "sprd,sharkl5-hwdvfs-jpg" },
	{ },
	};
MODULE_DEVICE_TABLE(of, jpg_dvfs_of_match);


static struct platform_driver jpg_dvfs_driver = {
	.probe    = jpg_dvfs_probe,
	.remove     = jpg_dvfs_remove,
	.driver = {
	.name = "jpg-dvfs",
	.of_match_table = jpg_dvfs_of_match,
},
};

static struct devfreq_dev_profile jpg_dvfs_profile = {
	.polling_ms         = 200,
	.target             = jpg_dvfs_target,
	.get_dev_status     = jpg_dvfs_get_dev_status,
	.get_cur_freq       = jpg_dvfs_get_cur_freq,
};

static int jpg_dvfs_target(struct device *dev, unsigned long *freq,
	u32 flags)
{
	struct jpg_dvfs *jpg = dev_get_drvdata(dev);
	unsigned long target_freq = *freq, target_volt = 0;
	int err = 0;

	pr_info("devfreq_dev_profile-->target,freq=%lu\n", *freq);

	if (jpg->freq == *freq)
		return 0;

	mutex_lock(&jpg->lock);
	jpg->dvfs_ops->updata_target_freq(jpg->devfreq, target_volt,
		target_freq, FALSE);

	if (err) {
		dev_err(dev, "Cannot to set freq:%lu to jpg, err: %d\n",
		target_freq, err);
		goto out;
	}

	jpg->freq = target_freq;
	jpg->volt = target_volt;

out:
	mutex_unlock(&jpg->lock);
	return err;
}

int jpg_dvfs_get_dev_status(struct device *dev,
	struct devfreq_dev_status *stat)
{
	struct jpg_dvfs *jpg = dev_get_drvdata(dev);
	struct devfreq_event_data edata;
	int ret = 0;

	pr_info("devfreq_dev_profile-->get_dev_status\n");

	ret = devfreq_event_get_event(jpg->edev, &edata);
	if (ret < 0)
		return ret;

	stat->current_frequency = jpg->freq;
	stat->busy_time = edata.load_count;
	stat->total_time = edata.total_count;

	return ret;
}

static int jpg_dvfs_get_cur_freq(struct device *dev, unsigned long *freq)
{
	struct jpg_dvfs *jpg = dev_get_drvdata(dev);

	*freq = jpg->freq;
	pr_info("devfreq_dev_profile-->get_cur_freq,*freq=%lu\n", *freq);
	return 0;
}


static int jpg_dvfs_notify_callback(struct notifier_block *nb,
	unsigned long type, void *data)
{
	struct jpg_dvfs *jpg = container_of(nb, struct jpg_dvfs, pw_nb);

	switch (type) {

		case MMSYS_POWER_ON:
			if (jpg->dvfs_ops != NULL && jpg->dvfs_ops->power_on_nb != NULL)
				jpg->dvfs_ops->power_on_nb(jpg->devfreq);
			break;
		case MMSYS_POWER_OFF:
			if (jpg->dvfs_ops  !=  NULL &&
				jpg->dvfs_ops->power_off_nb != NULL)
				jpg->dvfs_ops->power_off_nb(jpg->devfreq);
			break;

		default:
			return -EINVAL;
	}

	return NOTIFY_OK;
}

static int jpg_dvfs_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;
	struct jpg_dvfs *jpg;
	int ret;

	pr_info("jpg-dvfs initialized\n");

	jpg = devm_kzalloc(dev, sizeof(*jpg), GFP_KERNEL);
	if (!jpg)
		return -ENOMEM;

	mutex_init(&jpg->lock);

#ifdef SUPPORT_SWDVFS

	jpg->clk_jpg_core = devm_clk_get(dev, "clk_jpg_core");
	if (IS_ERR(jpg->clk_jpg_core))
		dev_err(dev, "Cannot get the clk_jpg_core clk\n");

	of_property_read_u32(np, "sprd,dvfs-wait-window",
	&jpg->dvfs_wait_window);
#endif
#if 0
	of_property_read_u32(np, "sprd,dvfs-gfree-wait-delay",
	&jpg->jpg_dvfs_para.ip_coffe.gfree_wait_delay);
	of_property_read_u32(np, "sprd,dvfs-freq-upd-hdsk-en",
	&jpg->jpg_dvfs_para.ip_coffe.freq_upd_hdsk_en);
	of_property_read_u32(np, "sprd,dvfs-freq-upd-delay-en",
	&jpg->jpg_dvfs_para.ip_coffe.freq_upd_delay_en);
	of_property_read_u32(np, "sprd,dvfs-freq-upd-en-byp",
	&jpg->jpg_dvfs_para.ip_coffe.freq_upd_en_byp);
	of_property_read_u32(np, "sprd,dvfs-sw-trig-en",
	&jpg->jpg_dvfs_para.ip_coffe.sw_trig_en);
	of_property_read_u32(np, "sprd,dvfs-auto-tune",
	&jpg->jpg_dvfs_para.ip_coffe.auto_tune);
	of_property_read_u32(np, "sprd,dvfs-idle-index-def",
	&jpg->jpg_dvfs_para.ip_coffe.idle_index_def);
#endif
	of_property_read_u32(np, "sprd,dvfs-work-index-def",
	&jpg->jpg_dvfs_para.ip_coffe.work_index_def);
	platform_set_drvdata(pdev, jpg);
	jpg->devfreq = devm_devfreq_add_device(dev,
	&jpg_dvfs_profile,
	"jpg_dvfs",
	NULL);
	if (IS_ERR(jpg->devfreq)) {
		dev_err(dev,
		"failed to add devfreq dev with jpg-dvfs governor\n");
		ret = PTR_ERR(jpg->devfreq);
		goto err;
	}

	jpg->pw_nb.priority = 0;
	jpg->pw_nb.notifier_call =  jpg_dvfs_notify_callback;
	ret = mmsys_register_notifier(&jpg->pw_nb);

	return 0;

err:
	return ret;
}

static int jpg_dvfs_remove(struct platform_device *pdev)
{
	pr_err("%s:\n", __func__);

	return 0;
}

#ifndef KO_DEFINE

int jpg_dvfs_init(void)
{
	int ret = 0;

	ret = register_ip_dvfs_ops(&jpg_dvfs_ops);
	if (ret) {
		pr_err("%s: failed to add ops: %d\n", __func__, ret);
		return ret;
	}

	ret = devfreq_add_governor(&jpg_dvfs_gov);
	if (ret) {
		pr_err("%s: failed to add governor: %d\n", __func__, ret);
		return ret;
	}

	ret = platform_driver_register(&jpg_dvfs_driver);

	if (ret)
		devfreq_remove_governor(&jpg_dvfs_gov);

	return ret;
}

void jpg_dvfs_exit(void)
{
	int ret = 0;

	platform_driver_unregister(&jpg_dvfs_driver);

	ret = devfreq_remove_governor(&jpg_dvfs_gov);
	if (ret)
		pr_err("%s: failed to remove governor: %d\n", __func__, ret);

	ret = unregister_ip_dvfs_ops(&jpg_dvfs_ops);
	if (ret)
		pr_err("%s: failed to remove ops: %d\n", __func__, ret);
}

#else
int __init jpg_dvfs_init(void)
{
	int ret = 0;

	ret = register_ip_dvfs_ops(&jpg_dvfs_ops);
	if (ret) {
		pr_err("%s: failed to add ops: %d\n", __func__, ret);
		return ret;
	}

	ret = devfreq_add_governor(&jpg_dvfs_gov);
	if (ret) {
		pr_err("%s: failed to add governor: %d\n", __func__, ret);
		return ret;
	}

	ret = platform_driver_register(&jpg_dvfs_driver);

	if (ret)
		devfreq_remove_governor(&jpg_dvfs_gov);

	return ret;
}

void __exit jpg_dvfs_exit(void)
{
	int ret = 0;

	platform_driver_unregister(&jpg_dvfs_driver);

	ret = devfreq_remove_governor(&jpg_dvfs_gov);
	if (ret)
		pr_err("%s: failed to remove governor: %d\n", __func__, ret);

	ret = unregister_ip_dvfs_ops(&jpg_dvfs_ops);
	if (ret)
		pr_err("%s: failed to remove ops: %d\n", __func__, ret);
}



module_init(jpg_dvfs_init);
module_exit(jpg_dvfs_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Sprd jpg devfreq driver");
MODULE_AUTHOR("Multimedia_Camera@Spreadtrum");

#endif

