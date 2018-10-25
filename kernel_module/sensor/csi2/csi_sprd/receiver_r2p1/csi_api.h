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

struct csi_phy_info {
	struct regmap *cam_ahb_syscon;
	struct regmap *aon_apb_syscon;
	struct regmap *anlg_phy_g1_syscon; /* anlg_phy control,0x4035 */
	unsigned int phy_id;
};

struct csi_dt_node_info {
	unsigned int controller_id;
	unsigned long reg_base;
	struct clk *ckg_eb;
	struct clk *mipi_csi_gate_eb;
	struct clk *csi_eb;
	struct clk *csi_src_eb;
	struct csi_phy_info phy;
};


struct bayer_value {
    unsigned int r;
    unsigned int g;
    unsigned int b;
};
struct csi_ipg_info {
    unsigned int bayer_mode;
    unsigned int frame_blank_size;
    unsigned int line_blank_size;
    unsigned int image_height;
    unsigned int image_width;
    unsigned int block_size;
    unsigned int color_bar_mode;
    unsigned int ipg_mode;
    unsigned int ipg_en;
    unsigned int hsync_en;
    struct bayer_value block0;
    struct bayer_value block1;
    struct bayer_value block2;
};
#if 1
int csi_api_mipi_phy_cfg(void);
int csi_api_mipi_phy_cfg_init(struct device_node *phy_node, int sensor_id);
int csi_api_dt_node_init(struct device *dev, struct device_node *dn,
				int csi_id, unsigned int phy_id);
int csi_api_open(int bps_per_lane, int phy_id, int lane_num, int csi_id);
int csi_api_close(uint32_t phy_id, int csi_id);
int csi_api_switch(int sensor_id);
#endif
#endif

