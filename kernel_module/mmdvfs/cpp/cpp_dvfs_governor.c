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

static ssize_t get_dvfs_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct devfreq *devfreq = to_devfreq(dev);
	struct cpp_dvfs *cpp;
	int err = 0;

	cpp = dev_get_drvdata(devfreq->dev.parent);
	mutex_lock(&devfreq->lock);
	if (cpp != NULL)
		err = sprintf(buf, "%d\n", cpp->cpp_dvfs_para.u_dvfs_en);
	else
		err = sprintf(buf, "undefined\n");
	mutex_unlock(&devfreq->lock);

	return err;
}

static ssize_t set_dvfs_enable_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct devfreq *devfreq = to_devfreq(dev);
	struct cpp_dvfs *cpp;
	unsigned long  dvfs_en;
	int err;

	cpp = dev_get_drvdata(devfreq->dev.parent);

	mutex_lock(&devfreq->lock);

	err = sscanf(buf, "%lu\n", &dvfs_en);
	if (err != 1) {
		mutex_unlock(&devfreq->lock);
		return -EINVAL;
	}
	pr_info("%s: err=%d, dvfs_en=%lu\n", __func__, err, dvfs_en);

	cpp->cpp_dvfs_para.u_dvfs_en = dvfs_en;
	cpp->dvfs_enable = dvfs_en;
	if (cpp->dvfs_ops != NULL && cpp->dvfs_ops->ip_hw_dvfs_en != NULL && (
		err != 0)) {

		cpp->dvfs_ops->ip_hw_dvfs_en(devfreq,
		cpp->cpp_dvfs_para.u_dvfs_en);
	} else
		pr_info("%s: ip  ops null\n", __func__);


	err = count;
	mutex_unlock(&devfreq->lock);

	return err;
}

static ssize_t get_auto_tune_en_show(struct device *dev,
					struct device_attribute *attr, char *
						buf)
{
	struct devfreq *devfreq = to_devfreq(dev);
	struct cpp_dvfs *cpp;
	int err = 0;

	cpp = dev_get_drvdata(devfreq->dev.parent);
	mutex_lock(&devfreq->lock);
	if (cpp != NULL)
		err = sprintf(buf, "%d\n", cpp->cpp_dvfs_para.u_auto_tune_en);
	else
		err = sprintf(buf, "undefined\n");
	mutex_unlock(&devfreq->lock);

	return err;

}

static ssize_t set_auto_tune_en_store(struct device *dev,
	struct device_attribute *attr,  const char *buf, size_t count)
{
	struct devfreq *devfreq = to_devfreq(dev);
	struct cpp_dvfs *cpp;
	unsigned long  auto_tune_en;
	int err;

	cpp = dev_get_drvdata(devfreq->dev.parent);

	mutex_lock(&devfreq->lock);

	err = sscanf(buf, "%lu\n", &auto_tune_en);
	if (err != 1) {
		mutex_unlock(&devfreq->lock);
		return -EINVAL;
	}
	cpp->cpp_dvfs_para.u_auto_tune_en = auto_tune_en;

	if (cpp->dvfs_ops  !=  NULL &&     cpp->dvfs_ops->ip_auto_tune_en !=
		NULL) {

		cpp->dvfs_ops->ip_auto_tune_en(devfreq,
		cpp->cpp_dvfs_para.u_auto_tune_en);
	} else
		pr_info("%s: ip  ops null\n", __func__);


	err = count;

	mutex_unlock(&devfreq->lock);

	return err;
}

static ssize_t get_work_freq_show(struct device *dev, struct device_attribute *
	attr,
		char *buf)
{
	struct devfreq *devfreq = to_devfreq(dev);
	struct cpp_dvfs *cpp;
	int err = 0;

	cpp = dev_get_drvdata(devfreq->dev.parent);
	mutex_lock(&devfreq->lock);

	if (cpp != NULL)
		err = sprintf(buf, "%d\n", cpp->cpp_dvfs_para.u_work_freq);
	else
		err = sprintf(buf, "undefined\n");

	mutex_unlock(&devfreq->lock);

	return err;
}

static ssize_t set_work_freq_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct devfreq *devfreq = to_devfreq(dev);
	struct cpp_dvfs *cpp;
	unsigned long user_freq;

	unsigned long pid;
	int err;
	Dvfs_Queue DvfsQ;
	int active;

	cpp = dev_get_drvdata(devfreq->dev.parent);
	mutex_lock(&devfreq->lock);
	err = sscanf(buf, "%lu:%lu:%d\n", &user_freq, &pid, &active);
	pr_info("%s:err=%d,count=%d", __func__, err, (int)count);
	if (err != 3) {
		mutex_unlock(&devfreq->lock);
		return -EINVAL;
	}
	pr_info("%s: dvfs freq %lu,pid %lu active %d",
	__func__, user_freq, pid, active);
	cpp->cpp_dvfs_para.u_work_freq = user_freq;
	DvfsQ.active = active;
	DvfsQ.freq = user_freq;
	DvfsQ.pid = pid;
	MatchPid_RemoveInvalidNode(&cpp->head, DvfsQ);
	err = update_devfreq(devfreq);
	if (err == 0)
		err = count;

	mutex_unlock(&devfreq->lock);

	return err;
}

static ssize_t get_idle_freq_show(struct device *dev, struct device_attribute *
	attr,
			char *buf)
{
	struct devfreq *devfreq = to_devfreq(dev);
	struct cpp_dvfs *cpp;
	int err = 0;

	cpp = dev_get_drvdata(devfreq->dev.parent);
	mutex_lock(&devfreq->lock);
	if (cpp != NULL)
		err = sprintf(buf, "%d\n", cpp->cpp_dvfs_para.u_idle_freq);
	else
		err = sprintf(buf, "undefined\n");
	mutex_unlock(&devfreq->lock);

	return err;

}

static ssize_t set_idle_freq_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct devfreq *devfreq = to_devfreq(dev);
	struct cpp_dvfs *cpp;
	unsigned long idle_freq;
	int err;

	cpp = dev_get_drvdata(devfreq->dev.parent);
	mutex_lock(&devfreq->lock);
	err = sscanf(buf, "%lu\n", &idle_freq);
	if (err == 0) {
		mutex_unlock(&devfreq->lock);
		return -EINVAL;
	}

	cpp->cpp_dvfs_para.u_idle_freq = idle_freq;

	err = update_devfreq(devfreq);
	if (err == 0)
		err = count;

	mutex_unlock(&devfreq->lock);

	return err;

}

static ssize_t get_work_index_show(struct device *dev,
		struct device_attribute *attr,  char *buf)
{
	struct devfreq *devfreq = to_devfreq(dev);
	struct cpp_dvfs *cpp;
	int err = 0;

	cpp = dev_get_drvdata(devfreq->dev.parent);
	mutex_lock(&devfreq->lock);
	if (cpp != NULL)
		err = sprintf(buf, "%d\n", cpp->cpp_dvfs_para.u_work_index);
	else
		err = sprintf(buf, "undefined\n");
	mutex_unlock(&devfreq->lock);

	return err;
}

static ssize_t set_work_index_store(struct device *dev,
					struct device_attribute *attr, const
						char *buf, size_t count)
{
	struct devfreq *devfreq = to_devfreq(dev);
	struct cpp_dvfs *cpp;
	unsigned long  work_index;
	int err;

	cpp = dev_get_drvdata(devfreq->dev.parent);

	mutex_lock(&devfreq->lock);

	err = sscanf(buf, "%lu\n", &work_index);

	if (err != 1) {
		mutex_unlock(&devfreq->lock);
		return -EINVAL;
	}
	pr_info("%s: count=%d\n", __func__, (int)count);
	pr_info("%s: ip  ops null,work_index= %lu\n", __func__, work_index);


	if (cpp->dvfs_ops  !=  NULL &&
		cpp->dvfs_ops->set_ip_dvfs_work_index != NULL && (err != 0)) {

		cpp->cpp_dvfs_para.u_work_index = work_index;
		cpp->dvfs_ops->set_ip_dvfs_work_index(devfreq,
		cpp->cpp_dvfs_para.u_work_index);
	} else
		pr_info("%s: ip  ops null\n", __func__);


	err = count;
	mutex_unlock(&devfreq->lock);


	return err;
}


static ssize_t get_idle_index_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct devfreq *devfreq = to_devfreq(dev);
	struct cpp_dvfs *cpp;
	int err = 0;

	cpp = dev_get_drvdata(devfreq->dev.parent);
	mutex_lock(&devfreq->lock);
	if (cpp != NULL)
		err = sprintf(buf, "%d\n", cpp->cpp_dvfs_para.u_idle_index);
	else
		err = sprintf(buf, "undefined\n");
	mutex_unlock(&devfreq->lock);

	return err;
}

static ssize_t set_idle_index_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct devfreq *devfreq = to_devfreq(dev);
	struct cpp_dvfs *cpp;
	unsigned long  idle_index;
	int err;

	cpp = dev_get_drvdata(devfreq->dev.parent);

	mutex_lock(&devfreq->lock);

	err = sscanf(buf, "%lu\n", &idle_index);
	if (err != 1) {
		mutex_unlock(&devfreq->lock);
		return -EINVAL;
	}
	cpp->cpp_dvfs_para.u_idle_index = idle_index;

	if (cpp->dvfs_ops  !=  NULL &&
		cpp->dvfs_ops->set_ip_dvfs_idle_index  != NULL && (err != 0)) {

		cpp->dvfs_ops->set_ip_dvfs_idle_index(devfreq,
		cpp->cpp_dvfs_para.u_idle_index);
	} else
		pr_info("%s: ip  ops null\n", __func__);


	err = count;
	mutex_unlock(&devfreq->lock);

	return err;
}


static ssize_t get_fix_dvfs_show(struct device *dev, struct device_attribute *
	attr,
		char *buf)
{
	struct devfreq *devfreq = to_devfreq(dev);
	struct cpp_dvfs *cpp;
	int err = 0;

	cpp = dev_get_drvdata(devfreq->dev.parent);
	mutex_lock(&devfreq->lock);
	if (cpp != NULL)
		err = sprintf(buf, "%d\n", cpp->cpp_dvfs_para.u_fix_volt);
	else
		err = sprintf(buf, "undefined\n");
	mutex_unlock(&devfreq->lock);

	return err;
}

static ssize_t set_fix_dvfs_store(struct device *dev, struct device_attribute *
	attr,
					const char *buf, size_t count)
{
	struct devfreq *devfreq = to_devfreq(dev);
	struct cpp_dvfs *cpp;
	unsigned long  fix_volt;
	int err;

	cpp = dev_get_drvdata(devfreq->dev.parent);

	mutex_lock(&devfreq->lock);

	err = sscanf(buf, "%lu\n", &fix_volt);
	if (err != 1) {
		mutex_unlock(&devfreq->lock);
		return -EINVAL;
	}
	cpp->cpp_dvfs_para.u_fix_volt = fix_volt;

	if (cpp->dvfs_ops  !=  NULL &&   cpp->dvfs_ops->set_fix_dvfs_value !=
		NULL) {

		cpp->dvfs_ops->set_fix_dvfs_value(devfreq,
		cpp->cpp_dvfs_para.u_fix_volt);
	} else
		pr_info("%s: ip  ops null\n", __func__);


	err = count;
	mutex_unlock(&devfreq->lock);

	return err;
}


static ssize_t get_dvfs_coffe_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct devfreq *devfreq = to_devfreq(dev);
	struct cpp_dvfs *cpp;
	int len = 0;

	cpp = dev_get_drvdata(devfreq->dev.parent);
	mutex_lock(&devfreq->lock);

	len = sprintf(buf, "IP_dvfs_coffe_show\n");

	len += sprintf(buf+len, "%d\n",
		cpp->cpp_dvfs_para.ip_coffe.auto_tune);
	len += sprintf(buf+len, "%d\n",
		cpp->cpp_dvfs_para.ip_coffe.freq_upd_delay_en);
	len += sprintf(buf+len, "%d\n",
		cpp->cpp_dvfs_para.ip_coffe.freq_upd_en_byp);
	len += sprintf(buf+len, "%d\n",
		cpp->cpp_dvfs_para.ip_coffe.freq_upd_hdsk_en);
	len += sprintf(buf+len, "%d\n",
		cpp->cpp_dvfs_para.ip_coffe.gfree_wait_delay);
	len += sprintf(buf+len, "%d\n",
		cpp->cpp_dvfs_para.ip_coffe.idle_index_def);
	len += sprintf(buf+len, "%d\n",
		cpp->cpp_dvfs_para.ip_coffe.idle_index_def);
	len += sprintf(buf+len, "%d\n",
		cpp->cpp_dvfs_para.ip_coffe.work_index_def);
	len += sprintf(buf+len, "%d\n",
		cpp->cpp_dvfs_para.ip_coffe.sw_trig_en);

	mutex_unlock(&devfreq->lock);

	return len;

}


static ssize_t set_dvfs_coffe_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct devfreq *devfreq = to_devfreq(dev);
	struct cpp_dvfs *cpp;
	struct ip_dvfs_coffe dvfs_coffe;
	int err = 0;

	cpp = dev_get_drvdata(devfreq->dev.parent);
	mutex_lock(&devfreq->lock);
	err = sscanf(buf, "%d,%d,%d,%d,%d,%d,%d,%d\n",
	&dvfs_coffe.gfree_wait_delay,
	&dvfs_coffe.freq_upd_hdsk_en, &dvfs_coffe.freq_upd_delay_en,
	&dvfs_coffe.freq_upd_en_byp, &dvfs_coffe.sw_trig_en,
	&dvfs_coffe.auto_tune, &dvfs_coffe.work_index_def,
	&dvfs_coffe.idle_index_def);
	cpp->cpp_dvfs_para.ip_coffe = dvfs_coffe;
	if (err != 8) {
		mutex_unlock(&devfreq->lock);
		return -EINVAL;
	}

	if (cpp->dvfs_ops  !=  NULL &&   cpp->dvfs_ops->set_ip_dvfs_coffe !=
		NULL) {
		err = cpp->dvfs_ops->set_ip_dvfs_coffe(devfreq,  &dvfs_coffe);
	} else
		pr_info("%s: ip  ops null\n", __func__);


	err = count;

	mutex_unlock(&devfreq->lock);

	return err;
}

static ssize_t get_ip_status_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct devfreq *devfreq = to_devfreq(dev);
	struct cpp_dvfs *cpp;
	struct ip_dvfs_status ip_status = {0};
	ssize_t len = 0;
	int ret = 0;
	unsigned int top_volt = 0, mm_volt = 0;


	cpp = dev_get_drvdata(devfreq->dev.parent);

	mutex_lock(&devfreq->lock);
	if (cpp->dvfs_ops  !=  NULL &&
		cpp->dvfs_ops->get_ip_status != NULL) {
		ret = cpp->dvfs_ops->get_ip_status(devfreq, &ip_status);
	} else
		pr_info("%s: dvfs_read_ops is null\n", __func__);

	if (cpp->dvfs_ops  !=  NULL &&
		cpp->dvfs_ops->top_current_volt != NULL) {
		ret = cpp->dvfs_ops->top_current_volt(devfreq, &top_volt);
	} else
		pr_info("%s: dvfs_read_top_volt is null\n", __func__);

	if (cpp->dvfs_ops  !=  NULL &&
		cpp->dvfs_ops->mm_current_volt != NULL) {
		ret = cpp->dvfs_ops->mm_current_volt(devfreq, &mm_volt);
	} else
		pr_info("%s: dvfs_read_top_volt is null\n", __func__);

	len = sprintf(buf, "cpp_dvfs_read_clk=%d\n", ip_status.current_ip_clk);
	len += sprintf(buf+len, "cpp_dvfs_read_volt=%d\n",
		ip_status.current_sys_volt);
	len += sprintf(buf+len, "cpp_dvfs_read_top_volt=%d\n", top_volt);
	len += sprintf(buf+len, "cpp_dvfs_read_mm_volt=%d\n", mm_volt);

	mutex_unlock(&devfreq->lock);

	return len;
}


static ssize_t get_ip_status_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	return 0;
}

static ssize_t get_dvfs_table_info_show(struct device *dev,
	struct device_attribute *attr,   char *buf)
{
	struct devfreq *devfreq = to_devfreq(dev);
	struct cpp_dvfs *cpp;
	struct ip_dvfs_map_cfg dvfs_table[8] = {{0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}};
	ssize_t len = 0;
	int err = 0, i = 0;

	cpp = dev_get_drvdata(devfreq->dev.parent);
	mutex_lock(&devfreq->lock);

	if (cpp->dvfs_ops  !=  NULL &&   cpp->dvfs_ops->get_ip_dvfs_table !=
	NULL) {

		err = cpp->dvfs_ops->get_ip_dvfs_table(devfreq, dvfs_table);

	} else
		pr_info("%s: ip ops null\n", __func__);
	for (i = 0; i < 8; i++) {
		len += sprintf(buf+len, "map_index=%d\n",
			dvfs_table[i].map_index);
		len += sprintf(buf+len, "reg_add=%lu\n", dvfs_table[i].reg_add);
		len += sprintf(buf+len, "volt=%d\n", dvfs_table[i].volt);
		len += sprintf(buf+len, "clk=%d\n", dvfs_table[i].clk);
		len += sprintf(buf+len, "fdiv_denom=%d\n",
			dvfs_table[i].fdiv_denom);
		len += sprintf(buf+len, "fdiv_num=%d\n",
			dvfs_table[i].fdiv_num);
		len += sprintf(buf+len, "axi_index=%d\n",
			dvfs_table[i].axi_index);
		len += sprintf(buf+len, "mtx_index=%d\n", dvfs_table[i].mtx_index);

	}


	mutex_unlock(&devfreq->lock);

	return len;
}

static ssize_t set_dvfs_table_info_store(struct device *dev,
		struct device_attribute *attr,   const char *buf, size_t count)
{
	struct devfreq *devfreq = to_devfreq(dev);
	struct cpp_dvfs *cpp;
	struct ip_dvfs_map_cfg dvfs_table = {0};
	uint32_t  map_index;
	int err = 0;

	cpp = dev_get_drvdata(devfreq->dev.parent);
	mutex_lock(&devfreq->lock);
		/* To do */
	err = sscanf(buf, "%d,%lu,%d,%d,%d,%d,%d,%d\n",
	&dvfs_table.map_index, &dvfs_table.reg_add
	, &dvfs_table.volt, &dvfs_table.clk, &dvfs_table.fdiv_denom,
	&dvfs_table.fdiv_num, &dvfs_table.axi_index, &dvfs_table.mtx_index);
	if (err != 8) {
		mutex_unlock(&devfreq->lock);
		return -EINVAL;
	}

	map_index = dvfs_table.map_index;
	cpp->cpp_dvfs_para.ip_dvfs_map[map_index] = dvfs_table;

	if (cpp->dvfs_ops  !=  NULL &&   cpp->dvfs_ops->set_ip_dvfs_table !=
		NULL) {

		err = cpp->dvfs_ops->set_ip_dvfs_table(devfreq,  &dvfs_table);
	} else
		pr_info("%s: ip ops null\n", __func__);


	err = count;

	mutex_unlock(&devfreq->lock);

	return err;
}

static ssize_t get_gfree_wait_delay_show(struct device *dev,
			struct device_attribute *attr,  char *buf)
{
	struct devfreq *devfreq = to_devfreq(dev);
	struct cpp_dvfs *cpp;
	int err = 0;

	cpp = dev_get_drvdata(devfreq->dev.parent);
	mutex_lock(&devfreq->lock);
	if (cpp != NULL) {
		err = sprintf(buf, "%d\n",
		cpp->cpp_dvfs_para.ip_coffe.gfree_wait_delay);
		}
	else
		err = sprintf(buf, "undefined\n");
	mutex_unlock(&devfreq->lock);

	return err;

}

static ssize_t set_gfree_wait_delay_store(struct device *dev,
		struct device_attribute *attr,   const char *buf, size_t count)
{
	struct devfreq *devfreq = to_devfreq(dev);
	struct cpp_dvfs *cpp;
	unsigned long  gfree_wait_delay;
	int err;

	cpp = dev_get_drvdata(devfreq->dev.parent);

	mutex_lock(&devfreq->lock);

	err = sscanf(buf, " %lu\n", &gfree_wait_delay);
	pr_info("%s:err=%d,gfree_wait_delay=%lu,count=%d",
	__func__, err, gfree_wait_delay, (int)count);

	if (err != 1) {
		mutex_unlock(&devfreq->lock);
		return -EINVAL;
	}
	cpp->cpp_dvfs_para.ip_coffe.gfree_wait_delay = gfree_wait_delay;

	if (cpp->dvfs_ops  !=  NULL &&
		cpp->dvfs_ops->set_ip_gfree_wait_delay != NULL) {

		cpp->dvfs_ops->set_ip_gfree_wait_delay(
		cpp->cpp_dvfs_para.ip_coffe.gfree_wait_delay);
	} else
		pr_info("%s: ip ops null\n", __func__);

	err = count;
	mutex_unlock(&devfreq->lock);

	return err;
}

static ssize_t set_freq_upd_hdsk_en_store(struct device *dev,
		struct device_attribute *attr,    const char *buf, size_t count)
{
	struct devfreq *devfreq = to_devfreq(dev);
	struct cpp_dvfs *cpp;
	unsigned long  freq_upd_hdsk_en;
	int err;

	cpp = dev_get_drvdata(devfreq->dev.parent);

	mutex_lock(&devfreq->lock);

	err = sscanf(buf, "%lu\n", &freq_upd_hdsk_en);
	if (err != 1) {
		mutex_unlock(&devfreq->lock);
		return -EINVAL;
	}
	cpp->cpp_dvfs_para.ip_coffe.freq_upd_hdsk_en = freq_upd_hdsk_en;

	if (cpp->dvfs_ops  !=  NULL &&
		cpp->dvfs_ops->set_ip_freq_upd_hdsk_en != NULL) {

		cpp->dvfs_ops->set_ip_freq_upd_hdsk_en(
		cpp->cpp_dvfs_para.ip_coffe.freq_upd_hdsk_en);
	} else
		pr_info("%s: ip ops null\n", __func__);


	err = count;
	mutex_unlock(&devfreq->lock);

	return err;
}


static ssize_t get_freq_upd_hdsk_en_show(struct device *dev,
	struct device_attribute *attr,  char *buf)
{
	struct devfreq *devfreq = to_devfreq(dev);
	struct cpp_dvfs *cpp;
	int err = 0;

	cpp = dev_get_drvdata(devfreq->dev.parent);
	mutex_lock(&devfreq->lock);
	if (cpp != NULL) {
		err = sprintf(buf, "%d\n",
		cpp->cpp_dvfs_para.ip_coffe.freq_upd_hdsk_en);
		}
	else
		err = sprintf(buf, "undefined\n");
	mutex_unlock(&devfreq->lock);

	return err;

}

static ssize_t set_freq_upd_delay_en_store(struct device *dev,
		struct device_attribute *attr,   const char *buf, size_t count)
{
	struct devfreq *devfreq = to_devfreq(dev);
	struct cpp_dvfs *cpp;
	unsigned long  freq_upd_delay_en;
	int err;

	cpp = dev_get_drvdata(devfreq->dev.parent);

	mutex_lock(&devfreq->lock);

	err = sscanf(buf, " %lu\n", &freq_upd_delay_en);
	if (err != 1) {
		mutex_unlock(&devfreq->lock);
		return -EINVAL;
	}
	cpp->cpp_dvfs_para.ip_coffe.freq_upd_delay_en = freq_upd_delay_en;

	if (cpp->dvfs_ops  !=  NULL
		&&  cpp->dvfs_ops->set_ip_freq_upd_delay_en != NULL) {

		cpp->dvfs_ops->set_ip_freq_upd_delay_en(
		cpp->cpp_dvfs_para.ip_coffe.freq_upd_delay_en);
	} else
		pr_info("%s: ip ops null\n", __func__);


	err = count;
	mutex_unlock(&devfreq->lock);

	return err;
}


static ssize_t get_freq_upd_delay_en_show(struct device *dev,
		struct device_attribute *attr,    char *buf)

{
	struct devfreq *devfreq = to_devfreq(dev);
	struct cpp_dvfs *cpp;
	int err = 0;

	cpp = dev_get_drvdata(devfreq->dev.parent);
	mutex_lock(&devfreq->lock);
	if (cpp != NULL) {
		err = sprintf(buf, "%d\n",
		cpp->cpp_dvfs_para.ip_coffe.freq_upd_delay_en);
		}
	else
		err = sprintf(buf, "undefined\n");
	mutex_unlock(&devfreq->lock);

	return err;

}

static ssize_t set_freq_upd_en_byp_store(struct device *dev,
		struct device_attribute *attr,  const char *buf, size_t count)
{
	struct devfreq *devfreq = to_devfreq(dev);
	struct cpp_dvfs *cpp;
	unsigned long  freq_upd_en_byp;
	int err;

	cpp = dev_get_drvdata(devfreq->dev.parent);

	mutex_lock(&devfreq->lock);

	err = sscanf(buf, " %lu\n", &freq_upd_en_byp);
	if (err != 1) {
		mutex_unlock(&devfreq->lock);
		return -EINVAL;
	}
	cpp->cpp_dvfs_para.ip_coffe.freq_upd_en_byp = freq_upd_en_byp;

	if (cpp->dvfs_ops  !=  NULL &&
		cpp->dvfs_ops->set_ip_freq_upd_en_byp != NULL) {

		cpp->dvfs_ops->set_ip_freq_upd_en_byp(
		cpp->cpp_dvfs_para.ip_coffe.freq_upd_en_byp);
	} else
		pr_info("%s: ip ops null\n", __func__);


	err = count;
	mutex_unlock(&devfreq->lock);

	return err;
}


static ssize_t get_freq_upd_en_byp_show(struct device *dev,
		struct device_attribute *attr,  char *buf)
{
	struct devfreq *devfreq = to_devfreq(dev);
	struct cpp_dvfs *cpp;
	int err = 0;

	cpp = dev_get_drvdata(devfreq->dev.parent);
	mutex_lock(&devfreq->lock);
	if (cpp != NULL) {
		err = sprintf(buf, "%d\n",
		cpp->cpp_dvfs_para.ip_coffe.freq_upd_en_byp);
		}
	else
		err = sprintf(buf, "undefined\n");
	mutex_unlock(&devfreq->lock);

	return err;

}

static ssize_t set_sw_trig_en_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct devfreq *devfreq = to_devfreq(dev);
	struct cpp_dvfs *cpp;
	unsigned long  sw_trig_en;
	int err;

	cpp = dev_get_drvdata(devfreq->dev.parent);

	mutex_lock(&devfreq->lock);

	err = sscanf(buf, "%lu\n", &sw_trig_en);
	if (err != 1) {
		mutex_unlock(&devfreq->lock);
		return -EINVAL;
	}
	cpp->cpp_dvfs_para.ip_coffe.freq_upd_en_byp = sw_trig_en;

	if (cpp->dvfs_ops  !=  NULL && cpp->dvfs_ops->set_ip_dvfs_swtrig_en !=
		NULL)
		cpp->dvfs_ops->set_ip_dvfs_swtrig_en(sw_trig_en);
	else
		pr_info("%s: ip ops null\n", __func__);


	err = count;
	mutex_unlock(&devfreq->lock);

	return err;
}


static ssize_t get_sw_trig_en_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct devfreq *devfreq = to_devfreq(dev);
	struct cpp_dvfs *cpp;
	int err = 0;

	cpp = dev_get_drvdata(devfreq->dev.parent);
	mutex_lock(&devfreq->lock);
	if (cpp != NULL) {
		err = sprintf(buf, "%d\n",
		cpp->cpp_dvfs_para.ip_coffe.sw_trig_en);
		}
	else
		err = sprintf(buf, "undefined\n");
	mutex_unlock(&devfreq->lock);

	return err;

}

/*sys for gov_entries*/
static DEVICE_ATTR(set_hw_dvfs_en, 0644, get_dvfs_enable_show,
			set_dvfs_enable_store);
static DEVICE_ATTR(set_auto_tune_en, 0644, get_auto_tune_en_show,
			set_auto_tune_en_store);
static DEVICE_ATTR(set_dvfs_coffe, 0644, get_dvfs_coffe_show,
	set_dvfs_coffe_store);
static DEVICE_ATTR(get_ip_status, 0644, get_ip_status_show,
	get_ip_status_store);
static DEVICE_ATTR(set_work_freq, 0644, get_work_freq_show,
	set_work_freq_store);
static DEVICE_ATTR(set_idle_freq, 0644, get_idle_freq_show,
	set_idle_freq_store);
static DEVICE_ATTR(set_work_index, 0644, get_work_index_show,
			set_work_index_store);
static DEVICE_ATTR(set_idle_index, 0644, get_idle_index_show,
			set_idle_index_store);
static DEVICE_ATTR(set_fix_dvfs_value, 0644, get_fix_dvfs_show,
	set_fix_dvfs_store);
static DEVICE_ATTR(get_dvfs_table_info, 0644, get_dvfs_table_info_show,
			set_dvfs_table_info_store);
/*sys for coeff_entries*/
static DEVICE_ATTR(set_gfree_wait_delay, 0644, get_gfree_wait_delay_show,
			set_gfree_wait_delay_store);
static DEVICE_ATTR(set_freq_upd_hdsk_en, 0644, get_freq_upd_hdsk_en_show,
			set_freq_upd_hdsk_en_store);
static DEVICE_ATTR(set_freq_upd_delay_en, 0644, get_freq_upd_delay_en_show,
			set_freq_upd_delay_en_store);
static DEVICE_ATTR(set_freq_upd_en_byp, 0644, get_freq_upd_en_byp_show,
			set_freq_upd_en_byp_store);
static DEVICE_ATTR(set_sw_trig_en, 0644, get_sw_trig_en_show,
			set_sw_trig_en_store);
static DEVICE_ATTR(set_idle_def_index, 0644, get_idle_index_show,
			set_idle_index_store);
static DEVICE_ATTR(set_work_def_index, 0644, get_work_index_show,
			NULL);


static struct attribute *dev_entries[] = {

	&dev_attr_set_hw_dvfs_en.attr,
	&dev_attr_set_auto_tune_en.attr,
	&dev_attr_set_dvfs_coffe.attr,
	&dev_attr_get_ip_status.attr,
	&dev_attr_set_work_freq.attr,
	&dev_attr_set_idle_freq.attr,
	&dev_attr_set_work_index.attr,
	&dev_attr_set_idle_index.attr,
	&dev_attr_set_fix_dvfs_value.attr,
	&dev_attr_get_dvfs_table_info.attr,
	NULL,
};

static struct attribute_group dev_attr_group = {
	.name    = "cpp_governor",
	.attrs    = dev_entries,
};

static struct attribute *coeff_entries[] = {

	&dev_attr_set_gfree_wait_delay.attr,
	&dev_attr_set_freq_upd_hdsk_en.attr,
	&dev_attr_set_freq_upd_delay_en.attr,
	&dev_attr_set_freq_upd_en_byp.attr,

	&dev_attr_set_sw_trig_en.attr,
	&dev_attr_set_work_def_index.attr,
	&dev_attr_set_idle_def_index.attr,
	&dev_attr_set_auto_tune_en.attr,
	NULL,
};

static struct attribute_group coeff_attr_group = {
	.name    = "cpp_coeff",
	.attrs    = coeff_entries,
};

static void userspace_exit(struct devfreq *devfreq)
{
	/*
	 * Remove the sysfs entry, unless this is being called after
	 * device_del(), which should have done this already via kobject_del().
	 */
	if (devfreq->dev.kobj.sd) {
		sysfs_remove_group(&devfreq->dev.kobj, &dev_attr_group);
		sysfs_remove_group(&devfreq->dev.kobj, &coeff_attr_group);
	}
}

static int userspace_init(struct devfreq *devfreq)
{
	int err = 0;

	struct cpp_dvfs *cpp = dev_get_drvdata(devfreq->dev.parent);

	pr_info("dvfs ops:  cpp %s\n", __func__);


	cpp->dvfs_ops = get_ip_dvfs_ops("CPP_DVFS_OPS");

	err = sysfs_create_group(&devfreq->dev.kobj, &dev_attr_group);
	err = sysfs_create_group(&devfreq->dev.kobj, &coeff_attr_group);

	return err;
}

static int cpp_dvfs_gov_get_target(struct devfreq *devfreq,
			unsigned long *freq)
{
	struct cpp_dvfs *cpp = dev_get_drvdata(devfreq->dev.parent);

	pr_info("devfreq_governor-->get_target_freq\n");

	if (cpp->dvfs_enable) {
		unsigned long adjusted_freq = cpp->user_freq;

		ShowQueue(&cpp->head);
		if (!IsEmpty(&cpp->head)) {
			adjusted_freq = GetFrontNodeFreq(&cpp->head);
			cpp->user_freq = adjusted_freq; /*  */
			pr_info("dvfs GetFrontNodeFreq %lu", adjusted_freq);
		} else {
			cpp->user_freq = devfreq->min_freq; /* TODO */
			adjusted_freq = devfreq->min_freq;
		}
	pr_info("dvfs cpp->user_freq = cpp->head->next.DvfsQ.freq %lu\n",
	cpp->user_freq);

		if (cpp->user_freq_type == DVFS_WORK)
			adjusted_freq = cpp->cpp_dvfs_para.u_work_freq;
		else
			adjusted_freq = cpp->cpp_dvfs_para.u_idle_freq;

		if (devfreq->max_freq && adjusted_freq > devfreq->max_freq)
			adjusted_freq = devfreq->max_freq;

		if (devfreq->min_freq && adjusted_freq < devfreq->min_freq)
			adjusted_freq = devfreq->min_freq;
	*freq = adjusted_freq;
	} else
		*freq = devfreq->previous_freq; /* No user freq specified yet */
	pr_info("dvfs *freq %lu", *freq);
	return 0;
}

	static int cpp_dvfs_gov_event_handler(struct devfreq *devfreq,
			unsigned int event, void *data)
{
	int ret = 0;

	pr_info("devfreq_governor-->event_handler(%d)\n", event);
	switch (event) {
	case DEVFREQ_GOV_START:
		ret = userspace_init(devfreq);
		break;
	case DEVFREQ_GOV_STOP:
		userspace_exit(devfreq);
		break;
	default:
		break;
	}

	return ret;
}

struct devfreq_governor cpp_dvfs_gov = {
	.name = "cpp_dvfs",
	.get_target_freq = cpp_dvfs_gov_get_target,
	.event_handler = cpp_dvfs_gov_event_handler,
};

