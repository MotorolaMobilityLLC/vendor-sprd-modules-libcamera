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

#include "fd_dvfs.h"

static int fd_dvfs_target(struct device *dev, unsigned long *freq,
	u32 flags);
static int fd_dvfs_get_dev_status(struct device *dev,
	struct devfreq_dev_status *stat);
static int fd_dvfs_get_cur_freq(struct device *dev, unsigned long *freq);
static int fd_dvfs_probe(struct platform_device *pdev);
static int fd_dvfs_remove(struct platform_device *pdev);


static const struct of_device_id fd_dvfs_of_match[] = {
	{ .compatible = "sprd,hwdvfs-fd-sharkl5" },
	{ },
	};
MODULE_DEVICE_TABLE(of, fd_dvfs_of_match);



static struct platform_driver fd_dvfs_driver = {
	.probe    = fd_dvfs_probe,
	.remove     = fd_dvfs_remove,
	.driver = {
	.name = "fd-dvfs",
	.of_match_table = fd_dvfs_of_match,

},
};

static struct devfreq_dev_profile fd_dvfs_profile = {
	.polling_ms         = 200,
	.target             = fd_dvfs_target,
	.get_dev_status     = fd_dvfs_get_dev_status,
	.get_cur_freq       = fd_dvfs_get_cur_freq,
};

static int fd_dvfs_target(struct device *dev, unsigned long *freq,
	u32 flags)
{
	struct fd_dvfs *fd = dev_get_drvdata(dev);
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
	if (fd->freq == target_freq)
		return 0;

	mutex_lock(&fd->lock);
	fd->dvfs_ops->updata_target_freq(fd->devfreq, target_volt,
		target_freq, FALSE);

	if (err) {
		dev_err(dev, "Cannot to set freq:%lu to fd, err: %d\n",
		target_freq, err);
		goto out;
	}

	fd->freq = target_freq;
	fd->volt = target_volt;

out:
	mutex_unlock(&fd->lock);
	return err;
}

int fd_dvfs_get_dev_status(struct device *dev,
	struct devfreq_dev_status *stat)
{
	struct fd_dvfs *fd = dev_get_drvdata(dev);
	struct devfreq_event_data edata;
	int ret = 0;

	pr_info("devfreq_dev_profile-->get_dev_status\n");

	ret = devfreq_event_get_event(fd->edev, &edata);
	if (ret < 0)
		return ret;

	stat->current_frequency = fd->freq;
	stat->busy_time = edata.load_count;
	stat->total_time = edata.total_count;

	return ret;
}

static int fd_dvfs_get_cur_freq(struct device *dev, unsigned long *freq)
{
	struct fd_dvfs *fd = dev_get_drvdata(dev);

	*freq = fd->freq;
	pr_info("devfreq_dev_profile-->get_cur_freq,*freq=%lu\n", *freq);
	return 0;
}


static int fd_dvfs_notify_callback(struct notifier_block *nb,
	unsigned long type, void *data)
{
	struct fd_dvfs *fd = container_of(nb, struct fd_dvfs, pw_nb);

	switch (type) {

		case MMSYS_POWER_ON:
			if (fd->dvfs_ops != NULL && fd->dvfs_ops->power_on_nb != NULL)
				fd->dvfs_ops->power_on_nb(fd->devfreq);
			break;
		case MMSYS_POWER_OFF:
			if (fd->dvfs_ops  !=  NULL &&
				fd->dvfs_ops->power_off_nb != NULL)
				fd->dvfs_ops->power_off_nb(fd->devfreq);
			break;

		default:
			return -EINVAL;
	}

	return NOTIFY_OK;
}

static int fd_dvfs_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;
	struct fd_dvfs *fd;
	int ret;

	pr_info("fd-dvfs initialized\n");

	fd = devm_kzalloc(dev, sizeof(*fd), GFP_KERNEL);
	if (!fd)
		return -ENOMEM;

	mutex_init(&fd->lock);

#ifdef SUPPORT_SWDVFS

	fd->clk_fd_core = devm_clk_get(dev, "clk_fd_core");
	if (IS_ERR(fd->clk_fd_core))
		dev_err(dev, "Cannot get the clk_fd_core clk\n");

	of_property_read_u32(np, "sprd,dvfs-wait-window",
	&fd->dvfs_wait_window);
#endif

	of_property_read_u32(np, "sprd,dvfs-gfree-wait-delay",
	&fd->fd_dvfs_para.ip_coffe.gfree_wait_delay);
	of_property_read_u32(np, "sprd,dvfs-freq-upd-hdsk-en",
	&fd->fd_dvfs_para.ip_coffe.freq_upd_hdsk_en);
	of_property_read_u32(np, "sprd,dvfs-freq-upd-delay-en",
	&fd->fd_dvfs_para.ip_coffe.freq_upd_delay_en);
	of_property_read_u32(np, "sprd,dvfs-freq-upd-en-byp",
	&fd->fd_dvfs_para.ip_coffe.freq_upd_en_byp);
	of_property_read_u32(np, "sprd,dvfs-sw-trig-en",
	&fd->fd_dvfs_para.ip_coffe.sw_trig_en);
	of_property_read_u32(np, "sprd,dvfs-auto-tune",
	&fd->fd_dvfs_para.ip_coffe.auto_tune);
	of_property_read_u32(np, "sprd,dvfs-work-index-def",
	&fd->fd_dvfs_para.ip_coffe.work_index_def);
	of_property_read_u32(np, "sprd,dvfs-idle-index-def",
	&fd->fd_dvfs_para.ip_coffe.idle_index_def);

	if (dev_pm_opp_of_add_table(dev)) {
		dev_err(dev, "Invalid operating-points in device tree.\n");
		return -EINVAL;
	}

	platform_set_drvdata(pdev, fd);
	fd->devfreq = devm_devfreq_add_device(dev,
	&fd_dvfs_profile,
	"fd_dvfs",
	NULL);
	
	if (IS_ERR(fd->devfreq)) {
		dev_err(dev,
		"failed to add devfreq dev with fd-dvfs governor\n");
		ret = PTR_ERR(fd->devfreq);
		goto err;
	}

	fd->pw_nb.priority = 0;
	fd->pw_nb.notifier_call =  fd_dvfs_notify_callback;
	ret = mmsys_register_notifier(&fd->pw_nb);

	return 0;

err:
	dev_pm_opp_of_remove_table(dev);

	return ret;
}

static int fd_dvfs_remove(struct platform_device *pdev)
{
	pr_err("%s:\n", __func__);

	return 0;
}

#ifndef KO_DEFINE

int fd_dvfs_init(void)
{
	int ret = 0;

	ret = register_ip_dvfs_ops(&fd_dvfs_ops);
	if (ret) {
		pr_err("%s: failed to add ops: %d\n", __func__, ret);
		return ret;
	}

	ret = devfreq_add_governor(&fd_dvfs_gov);
	if (ret) {
		pr_err("%s: failed to add governor: %d\n", __func__, ret);
		return ret;
	}

	ret = platform_driver_register(&fd_dvfs_driver);

	if (ret)
		devfreq_remove_governor(&fd_dvfs_gov);

	return ret;
}

void fd_dvfs_exit(void)
{
	int ret = 0;

	platform_driver_unregister(&fd_dvfs_driver);

	ret = devfreq_remove_governor(&fd_dvfs_gov);
	if (ret)
		pr_err("%s: failed to remove governor: %d\n", __func__, ret);

	ret = unregister_ip_dvfs_ops(&fd_dvfs_ops);
	if (ret)
		pr_err("%s: failed to remove ops: %d\n", __func__, ret);
}

#else
int __init fd_dvfs_init(void)
{
	int ret = 0;

	ret = register_ip_dvfs_ops(&fd_dvfs_ops);
	if (ret) {
		pr_err("%s: failed to add ops: %d\n", __func__, ret);
		return ret;
	}

	ret = devfreq_add_governor(&fd_dvfs_gov);
	if (ret) {
		pr_err("%s: failed to add governor: %d\n", __func__, ret);
		return ret;
	}

	ret = platform_driver_register(&fd_dvfs_driver);

	if (ret)
		devfreq_remove_governor(&fd_dvfs_gov);

	return ret;
}

void __exit fd_dvfs_exit(void)
{
	int ret = 0;

	platform_driver_unregister(&fd_dvfs_driver);

	ret = devfreq_remove_governor(&fd_dvfs_gov);
	if (ret)
		pr_err("%s: failed to remove governor: %d\n", __func__, ret);

	ret = unregister_ip_dvfs_ops(&fd_dvfs_ops);
	if (ret)
		pr_err("%s: failed to remove ops: %d\n", __func__, ret);
}



module_init(fd_dvfs_init);
module_exit(fd_dvfs_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Sprd fd devfreq driver");
MODULE_AUTHOR("Multimedia_Camera@Spreadtrum");

#endif

