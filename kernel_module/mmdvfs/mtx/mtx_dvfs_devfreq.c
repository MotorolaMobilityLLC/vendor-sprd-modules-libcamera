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

#include "mtx_dvfs.h"

static int mtx_dvfs_target(struct device *dev, unsigned long *freq,
	u32 flags);
static int mtx_dvfs_get_dev_status(struct device *dev,
	struct devfreq_dev_status *stat);
static int mtx_dvfs_get_cur_freq(struct device *dev, unsigned long *freq);
static int mtx_dvfs_probe(struct platform_device *pdev);
static int mtx_dvfs_remove(struct platform_device *pdev);

static const struct of_device_id mtx_dvfs_of_match[] = {
	{ .compatible = "sprd,hwdvfs-mtx" },
	{ },
	};
MODULE_DEVICE_TABLE(of, mtx_dvfs_of_match);


static struct platform_driver mtx_dvfs_driver = {
	.probe    = mtx_dvfs_probe,
	.remove     = mtx_dvfs_remove,
	.driver = {
	.name = "mtx-dvfs",
	.of_match_table = mtx_dvfs_of_match,
},
};

static struct devfreq_dev_profile mtx_dvfs_profile = {
	.polling_ms         = 200,
	.target             = mtx_dvfs_target,
	.get_dev_status     = mtx_dvfs_get_dev_status,
	.get_cur_freq       = mtx_dvfs_get_cur_freq,
};

static int mtx_dvfs_target(struct device *dev, unsigned long *freq,
	u32 flags)
{
	struct mtx_dvfs *mtx = dev_get_drvdata(dev);
	unsigned long target_freq = *freq, target_volt = 0;
	int err = 0;

	pr_info("devfreq_dev_profile-->target,freq=%lu\n", *freq);

	if (mtx->freq == *freq)
		return 0;

	mutex_lock(&mtx->lock);
	mtx->dvfs_ops->updata_target_freq(mtx->devfreq, target_volt,
		target_freq, FALSE);

	if (err) {
		dev_err(dev, "Cannot to set freq:%lu to mtx, err: %d\n",
		target_freq, err);
		goto out;
	}

	mtx->freq = target_freq;
	mtx->volt = target_volt;

out:
	mutex_unlock(&mtx->lock);
	return err;
}

int mtx_dvfs_get_dev_status(struct device *dev,
	struct devfreq_dev_status *stat)
{
	struct mtx_dvfs *mtx = dev_get_drvdata(dev);
	struct devfreq_event_data edata;
	int ret = 0;

	pr_info("devfreq_dev_profile-->get_dev_status\n");

	ret = devfreq_event_get_event(mtx->edev, &edata);
	if (ret < 0)
		return ret;

	stat->current_frequency = mtx->freq;
	stat->busy_time = edata.load_count;
	stat->total_time = edata.total_count;

	return ret;
}

static int mtx_dvfs_get_cur_freq(struct device *dev, unsigned long *freq)
{
	struct mtx_dvfs *mtx = dev_get_drvdata(dev);

	*freq = mtx->freq;
	pr_info("devfreq_dev_profile-->get_cur_freq,*freq=%lu\n", *freq);
	return 0;
}


static int mtx_dvfs_notify_callback(struct notifier_block *nb,
	unsigned long type, void *data)
{
	struct mtx_dvfs *mtx = container_of(nb, struct mtx_dvfs, pw_nb);

	switch (type) {

	case MMSYS_POWER_ON:
		if (mtx->dvfs_ops != NULL && mtx->dvfs_ops->power_on_nb != NULL)
			mtx->dvfs_ops->power_on_nb(mtx->devfreq);
		break;

	case MMSYS_POWER_OFF:
		if (mtx->dvfs_ops  !=  NULL &&
			mtx->dvfs_ops->power_off_nb != NULL)
			mtx->dvfs_ops->power_off_nb(mtx->devfreq);
		break;

	default:
		return -EINVAL;
	}

	return NOTIFY_OK;
}

static int mtx_dvfs_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;
	struct mtx_dvfs *mtx;
	int ret;

	pr_info("mtx-dvfs initialized\n");

	mtx = devm_kzalloc(dev, sizeof(*mtx), GFP_KERNEL);
	if (!mtx)
		return -ENOMEM;

	mutex_init(&mtx->lock);

#ifdef SUPPORT_SWDVFS

	mtx->clk_mtx_core = devm_clk_get(dev, "clk_mtx_core");
	if (IS_ERR(mtx->clk_mtx_core))
		dev_err(dev, "Cannot get the clk_mtx_core clk\n");

	of_property_read_u32(np, "sprd,dvfs-wait-window",
	&mtx->dvfs_wait_window);
#endif
#if 0
	of_property_read_u32(np, "sprd,dvfs-gfree-wait-delay",
	&mtx->mtx_dvfs_para.ip_coffe.gfree_wait_delay);
	of_property_read_u32(np, "sprd,dvfs-freq-upd-hdsk-en",
	&mtx->mtx_dvfs_para.ip_coffe.freq_upd_hdsk_en);
	of_property_read_u32(np, "sprd,dvfs-freq-upd-delay-en",
	&mtx->mtx_dvfs_para.ip_coffe.freq_upd_delay_en);
	of_property_read_u32(np, "sprd,dvfs-freq-upd-en-byp",
	&mtx->mtx_dvfs_para.ip_coffe.freq_upd_en_byp);
	of_property_read_u32(np, "sprd,dvfs-sw-trig-en",
	&mtx->mtx_dvfs_para.ip_coffe.sw_trig_en);
	of_property_read_u32(np, "sprd,dvfs-idle-index-def",
	&mtx->mtx_dvfs_para.ip_coffe.idle_index_def);
#endif
	of_property_read_u32(np, "sprd,dvfs-auto-tune",
	&mtx->mtx_dvfs_para.ip_coffe.auto_tune);
	of_property_read_u32(np, "sprd,dvfs-work-index-def",
	&mtx->mtx_dvfs_para.ip_coffe.work_index_def);

	platform_set_drvdata(pdev, mtx);
		mtx->devfreq = devm_devfreq_add_device(dev,
		&mtx_dvfs_profile,
		"mtx_dvfs",
		NULL);

	if (IS_ERR(mtx->devfreq)) {
		dev_err(dev,
		"failed to add devfreq dev with mtx-dvfs governor\n");
		ret = PTR_ERR(mtx->devfreq);
		goto err;
	}

	mtx->pw_nb.priority = 0;
	mtx->pw_nb.notifier_call =  mtx_dvfs_notify_callback;
	ret = mmsys_register_notifier(&mtx->pw_nb);

	return 0;

err:
	return ret;
}

static int mtx_dvfs_remove(struct platform_device *pdev)
{
	pr_err("%s:\n", __func__);

	return 0;
}

#ifndef KO_DEFINE

int mtx_dvfs_init(void)
{
	int ret = 0;

	ret = register_ip_dvfs_ops(&mtx_dvfs_ops);
	if (ret) {
		pr_err("%s: failed to add ops: %d\n", __func__, ret);
		return ret;
	}

	ret = devfreq_add_governor(&mtx_dvfs_gov);
	if (ret) {
		pr_err("%s: failed to add governor: %d\n", __func__, ret);
		return ret;
	}

	ret = platform_driver_register(&mtx_dvfs_driver);

	if (ret)
		devfreq_remove_governor(&mtx_dvfs_gov);

	return ret;
}

void mtx_dvfs_exit(void)
{
	int ret = 0;

	platform_driver_unregister(&mtx_dvfs_driver);

	ret = devfreq_remove_governor(&mtx_dvfs_gov);
	if (ret)
		pr_err("%s: failed to remove governor: %d\n", __func__, ret);

	ret = unregister_ip_dvfs_ops(&mtx_dvfs_ops);
	if (ret)
		pr_err("%s: failed to remove ops: %d\n", __func__, ret);
}

#else
int __init mtx_dvfs_init(void)
{
	int ret = 0;

	ret = register_ip_dvfs_ops(&mtx_dvfs_ops);
	if (ret) {
		pr_err("%s: failed to add ops: %d\n", __func__, ret);
		return ret;
	}

	ret = devfreq_add_governor(&mtx_dvfs_gov);
	if (ret) {
		pr_err("%s: failed to add governor: %d\n", __func__, ret);
		return ret;
	}

	ret = platform_driver_register(&mtx_dvfs_driver);

	if (ret)
		devfreq_remove_governor(&mtx_dvfs_gov);

	return ret;
}

void __exit mtx_dvfs_exit(void)
{
	int ret = 0;

	platform_driver_unregister(&mtx_dvfs_driver);

	ret = devfreq_remove_governor(&mtx_dvfs_gov);
	if (ret)
		pr_err("%s: failed to remove governor: %d\n", __func__, ret);

	ret = unregister_ip_dvfs_ops(&mtx_dvfs_ops);
	if (ret)
		pr_err("%s: failed to remove ops: %d\n", __func__, ret);
}



module_init(mtx_dvfs_init);
module_exit(mtx_dvfs_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Sprd mtx devfreq driver");
MODULE_AUTHOR("Multimedia_Camera@Spreadtrum");

#endif

