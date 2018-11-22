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

#ifndef _CSI_DRIVER_H_
#define _CSI_DRIVER_H_

#include <linux/of.h>
#include <linux/spinlock.h>

#define CSI_MAX_COUNT				3

enum csi_phy_t {
	PHY_4LANE = 0,
	PHY_2LANE,
	PHY_2P2,
	PHY_2P2_M,
	PHY_2P2_S,
	PHY_MAX,
};

enum csi_controller_t {
	CSI_RX0 = 0,
	CSI_RX1,
	CSI_RX2,
	CSI_RX_MAX,
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

/* csi_register_t just for sharkl2 */
enum csi_registers_t {
	IP_REVISION = 0x00,
	LANE_NUMBER = 0x04,
	PHY_PD_N = 0x08,
	RST_DPHY_N = 0x0C,
	RST_CSI2_N = 0x10,
	MODE_CFG = 0x14,
	PHY_STATE = 0x18,
	ERR0 = 0x20,
	ERR1 = 0x24,
	MASK0 = 0x28,
	MASK1 = 0x2C,
	ERR0_CLR = 0x30,
	ERR1_CLR = 0x34,
	CAL_DONE = 0x38,
	CAL_FAILED = 0x3C,
	MSK_CAL_DONE = 0x40,
	MSK_CAL_FAILED = 0x44,
	PHY_TEST_CRTL0 = 0x48,
	PHY_TEST_CRTL1 = 0x4c,
	IPG_RAW10_CFG0 = 0x50,
	IPG_RAW10_CFG1 = 0x54,
	IPG_RAW10_CFG2 = 0x58,
	IPG_RAW10_CFG3 = 0x5C,
	IPG_YUV422_8_CFG0 = 0x60,
	IPG_YUV422_8_CFG1 = 0x64,
	IPG_YUV422_8_CFG2 = 0x68,
	IPG_OTHER_CFG0 = 0x70,
	IPG_SIZE_CFG0 = 0x74,
};

int csi_reg_base_save(struct csi_dt_node_info *dt_info, int32_t idx);
void csi_controller_enable(struct csi_dt_node_info *dt_info, int32_t phyid);
void csi_set_on_lanes(uint8_t lanes, int32_t idx);
void csi_reset_shut_down(uint8_t shutdown, int32_t idx);
void csi_shut_down_phy(uint8_t shutdown, int32_t idx);
void csi_start(int sensor_id);
void csi_close(int32_t idx);
void csi_reset_controller(int32_t idx);
void csi_reset_phy(int32_t idx);
void csi_event_enable(int32_t idx);
int csi_ahb_reset(struct csi_dt_node_info *csi,
			unsigned int csi_id);
void csi_ipg_mode_cfg(uint32_t idx, int enable, int dt, int w, int h);

extern unsigned long s_csi_regbase[];

#define CSI_BASE(idx)    (s_csi_regbase[idx])
#define CSI_REG_WR(idx, reg, val)  (REG_WR(CSI_BASE(idx) + reg, val))
#define CSI_REG_RD(idx, reg)  (REG_RD(CSI_BASE(idx) + reg))
#define CSI_REG_MWR(idx, reg, msk, val)  CSI_REG_WR(idx, reg, \
	((val) & (msk)) | (CSI_REG_RD(idx, reg) & (~(msk))))
#define IPG_REG_MWR(reg, msk, val)  REG_WR(reg, \
		((val) & (msk)) | (REG_RD(reg) & (~(msk))))
#endif

