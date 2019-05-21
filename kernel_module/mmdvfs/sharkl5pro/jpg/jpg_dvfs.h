/*
 *Copyright (C) 2019 Unisoc Communications Inc.
 *
 *This software is licensed under the terms of the GNU General Public
 *License version 2, as published by the Free Software Foundation, and
 *may be copied, distributed, and modified under those terms.
 *
 *This program is distributed in the hope that it will be useful,
 *but WITHOUT ANY WARRANTY; without even the implied warranty of
 *MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *GNU General Public License for more details.
 */


#ifndef _JPG_DVFS_H
#define _JPG_DVFS_H

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/devfreq.h>
#include <linux/devfreq-event.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/platform_device.h>

#include "governor.h"
#include "mmsys_dvfs_comm.h"


struct jpg_dvfs {
	unsigned long dvfs_enable;
	uint32_t jpg_opp_hz;
	uint32_t jpg_opp_microvolt;
	uint32_t jpg_opp_microamp;
	uint32_t jpg_clock_latency;
	unsigned long freq;
	struct clk *clk_jpg_core;
	struct devfreq *devfreq;
	struct devfreq_event_dev *edev;
	struct ip_dvfs_ops  *dvfs_ops;
	struct ip_dvfs_para jpg_dvfs_para;
	struct mutex lock;
	struct notifier_block pw_nb;
};

extern struct ip_dvfs_ops  jpg_dvfs_ops;
extern struct devfreq_governor jpg_dvfs_gov;


#endif
