/*
 * Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef _CSI_API_H_
#define _CSI_API_H_

#include <linux/mfd/syscon.h>
#include <linux/regmap.h>
#include <linux/spinlock.h>

struct glb_syscon {
	struct regmap *mm_ahb;
	u32 dphy_sel;
	u32 dphy_msk;
	u32 csi_eb;
	u32 csi_eb_msk;
	u32 csi_rst;
	u32 csi_rst_msk;
	u32 ckg_eb;
	u32 ckg_eb_msk;
	u32 cphy_ckg_eb;
	u32 cphy_ckg_eb_msk;
};
struct csi_dt_node_info {
	unsigned int controller_id;
	unsigned long reg_base;
	unsigned int phy_id;
	struct clk *ckg_eb;
	struct clk *mipi_csi_gate_eb;
	struct clk *csi_eb;
	struct clk *csi_src_eb;

	//struct csi_phy_info phy;
	struct glb_syscon syscon;
};

int csi_api_mipi_phy_cfg(void);
int csi_api_mipi_phy_cfg_init(struct device_node *phy_node, int sensor_id);
int csi_api_dt_node_init(struct device *dev, struct device_node *dn,
				int csi_id, unsigned int phy_id);
int csi_api_open(int bps_per_lane, int phy_id, int lane_num, int sensor_id);
int csi_api_close(uint32_t phy_id, int csi_id);
int csi_api_switch(int sensor_id);
#endif

