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

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/mfd/syscon.h>
//#include <linux/mm_ahb.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/regmap.h>
#include <linux/semaphore.h>
#include <linux/spinlock.h>
#include <sprd_mm.h>

#include "csi_api.h"
#include "csi_driver.h"
#include "sprd_sensor_core.h"

#include "mm_ahb.h"

#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt) "csi_driver: %d %d %s : "\
	fmt, current->pid, __LINE__, __func__

#define CSI_MASK0			0x1FFFFFF
#define CSI_MASK1			0xFFFFFF
#define PHY_TESTCLR			BIT_0
#define PHY_TESTCLK			BIT_1
#define PHY_TESTDIN			0xFF
#define PHY_TESTDOUT			0xFF00
#define PHY_TESTEN			BIT_16
#define IPG_IMAGE_H_MASK		(0x1ff << 21)
#define IPG_COLOR_BAR_W_MASK		(0xff << 13)
#define IPG_IMAGE_W_MASK		(0x1FF << 4)
#define IPG_HSYNC_EN_MASK		BIT_3
#define IPG_COLOR_BAR_MODE_MASK	BIT_2
#define IPG_IMAGE_MODE_MASK		BIT_1
#define IPG_ENABLE_MASK			BIT_0
#define IPG_IMAGE_W			1280
#define IPG_IMAGE_H			960
#define IPG_IMAGE_H_REG			(((IPG_IMAGE_H)/8) << 21)
#define IPG_COLOR_BAR_W			(((IPG_IMAGE_W)/24) << 13)
#define IPG_IMAGE_W_REG			(((IPG_IMAGE_W)/16) << 4)
#define IPG_HSYNC_EN			(0 << 3)
#define IPG_COLOR_BAR_MODE		(0 << 2)
#define IPG_IMAGE_MODE			(1 << 1)   /*0: YUV 1:RAW*/
#define IPG_ENABLE				(1 << 0)

#define IPG_BAYER_PATTERN_MASK		0x3
#define IPG_BAYER_PATTERN_BGGR		0
#define IPG_BAYER_PATTERN_RGGB		1
#define IPG_BAYER_PATTERN_GBRG		2
#define IPG_BAYER_PATTERN_GRBG		3

#define IPG_BAYER_B_MASK		(0x3FF << 20)
#define IPG_BAYER_G_MASK		(0x3FF << 10)
#define IPG_BAYER_R_MASK		(0x3FF << 0)
#define IPG_RAW10_CFG0_B		(0 << 20)
#define IPG_RAW10_CFG0_G		(0 << 10)
#define IPG_RAW10_CFG0_R		0x3ff

#define IPG_RAW10_CFG1_B		(0 << 20)
#define IPG_RAW10_CFG1_G		(0x3FF << 10)
#define IPG_RAW10_CFG1_R		0

#define IPG_RAW10_CFG2_B		(0x3ff << 20)
#define IPG_RAW10_CFG2_G		(0 << 10)
#define IPG_RAW10_CFG2_R		0

#define IPG_YUV_CFG0_B		(0x51 << 16)
#define IPG_YUV_CFG0_G		(0x5f << 8)
#define IPG_YUV_CFG0_R		0xf0

#define IPG_YUV_CFG1_B		(0x91 << 16)
#define IPG_YUV_CFG1_G		(0x36 << 8)
#define IPG_YUV_CFG1_R		0x22

#define IPG_YUV_CFG2_B		(0xd2 << 16)
#define IPG_YUV_CFG2_G		(0x10 << 8)
#define IPG_YUV_CFG2_R		0x92
#define IPG_V_BLANK_MASK		(0xFFF << 13)
#define IPG_H_BLANK_MASK		0x1FFF
#define IPG_V_BLANK			(0x400 << 13)
#define IPG_H_BLANK			(0x500)
static unsigned long s_csi_regbase[SPRD_SENSOR_ID_MAX];
static unsigned long csi_dump_regbase[CSI_MAX_COUNT];

int csi_reg_base_save(struct csi_dt_node_info *dt_info, int32_t idx)
{
	if (!dt_info) {
		pr_err("fail to get valid dt_info ptr\n");
		return -EINVAL;
	}

	s_csi_regbase[idx] = dt_info->reg_base;
	csi_dump_regbase[dt_info->controller_id] = dt_info->reg_base;
	return 0;
}

struct csi_ipg_info csi_ipg_cxt[2] = {
	//for raw
	{IPG_BAYER_PATTERN_BGGR, 0x400, 0x500, 0x280, 0x1e0, 0,0,1,1,1,
		{0x3ff,0,0}, {0,0x3ff,0}, {0,0,0x3ff}},
	//for yuv
	{IPG_BAYER_PATTERN_BGGR, 0x400, 0x500, 0x280, 0x1e0, 0,0,0,1,1,
		{0x51,0x5a,0xf0}, {0x91,0x36,0x22}, {0x29,0xf0,0x6e}}
};

void csi_ipg_mode_cfg(uint32_t idx, int enable, int dt, int w, int h)
{
	struct csi_ipg_info *info = &csi_ipg_cxt[dt];

	pr_info("color mode:%s, ipg mode:%s, size:%dx%d\n", 
			info->color_bar_mode==0?"vertical":"horizontal",
			info->ipg_mode==0?"yuv422":"raw10", w, h);

	if (enable) {
		CSI_REG_MWR(idx, MODE_CFG, IPG_IMAGE_H_MASK, (h/8)<<21);
		//CSI_REG_MWR(idx, MODE_CFG, IPG_COLOR_BAR_W_MASK, (w/16)<<4);
		//CSI_REG_MWR(idx, MODE_CFG, IPG_IMAGE_W_MASK, (w/24)<<13); //block size
		
		CSI_REG_MWR(idx, MODE_CFG, IPG_IMAGE_W_MASK, (w/16)<<4);
		CSI_REG_MWR(idx, MODE_CFG, IPG_COLOR_BAR_W_MASK, (w/24)<<13); //block size

		CSI_REG_MWR(idx, MODE_CFG, IPG_HSYNC_EN_MASK, info->hsync_en << 3);
		CSI_REG_MWR(idx, MODE_CFG, IPG_COLOR_BAR_MODE_MASK, info->color_bar_mode << 2);
		CSI_REG_MWR(idx, MODE_CFG, IPG_IMAGE_MODE_MASK, info->ipg_mode << 1);

		if (info->ipg_mode == 1) {//raw10
			CSI_REG_MWR(idx, IPG_RAW10_CFG0, IPG_BAYER_B_MASK, info->block0.b<<20);
			CSI_REG_MWR(idx, IPG_RAW10_CFG0, IPG_BAYER_G_MASK, info->block0.g<<10);
			CSI_REG_MWR(idx, IPG_RAW10_CFG0, IPG_BAYER_R_MASK, info->block0.r);

			CSI_REG_MWR(idx, IPG_RAW10_CFG1, IPG_BAYER_B_MASK, info->block1.b<<20);
			CSI_REG_MWR(idx, IPG_RAW10_CFG1, IPG_BAYER_G_MASK, info->block1.g<<10);
			CSI_REG_MWR(idx, IPG_RAW10_CFG1, IPG_BAYER_R_MASK, info->block1.r);

			CSI_REG_MWR(idx, IPG_RAW10_CFG2, IPG_BAYER_B_MASK, info->block2.b<<20);
			CSI_REG_MWR(idx, IPG_RAW10_CFG2, IPG_BAYER_G_MASK, info->block2.g<<10);
			CSI_REG_MWR(idx, IPG_RAW10_CFG2, IPG_BAYER_R_MASK, info->block2.r);

			CSI_REG_MWR(idx, IPG_RAW10_CFG3, IPG_BAYER_PATTERN_MASK, IPG_BAYER_PATTERN_BGGR);
		}else if (info->ipg_mode == 0) { //yuv
			CSI_REG_MWR(idx, IPG_YUV422_8_CFG0, 0x00FF0000, info->block0.b<<16);
			CSI_REG_MWR(idx, IPG_YUV422_8_CFG0, 0x0000FF00, info->block0.g<<8);
			CSI_REG_MWR(idx, IPG_YUV422_8_CFG0, 0x000000FF, info->block0.r);

			CSI_REG_MWR(idx, IPG_YUV422_8_CFG1, 0x00FF0000, info->block1.b << 16);
			CSI_REG_MWR(idx, IPG_YUV422_8_CFG1, 0x0000FF00, info->block1.g << 8);
			CSI_REG_MWR(idx, IPG_YUV422_8_CFG1, 0x000000FF, info->block1.r);

			CSI_REG_MWR(idx, IPG_YUV422_8_CFG2, 0x00FF0000, info->block2.b<<16);
			CSI_REG_MWR(idx, IPG_YUV422_8_CFG2, 0x0000FF00, info->block2.g<<8);
			CSI_REG_MWR(idx, IPG_YUV422_8_CFG2, 0x000000FF, info->block2.r);
		}

		CSI_REG_MWR(idx, IPG_OTHER_CFG0, IPG_V_BLANK_MASK, info->frame_blank_size << 13);
		CSI_REG_MWR(idx, IPG_OTHER_CFG0, IPG_H_BLANK_MASK, info->line_blank_size);

		CSI_REG_MWR(idx, MODE_CFG, IPG_ENABLE_MASK, info->ipg_en);
	} else
		CSI_REG_MWR(idx, MODE_CFG, IPG_ENABLE_MASK, 0);

	pr_info("CSI IPG enable %d\n", enable);
}
/* phy testclear used to reset phy to right default state */
static void dphy_cfg_clr(int32_t idx)
{
	CSI_REG_MWR(idx, PHY_TEST_CRTL0, PHY_TESTCLR, 1);
	udelay(1);
	CSI_REG_MWR(idx, PHY_TEST_CRTL0, PHY_TESTCLR, 0);
	udelay(1);
}
void csi_phy_power_down(struct csi_dt_node_info *csi_info,
			unsigned int sensor_id, int is_eb)
{

	uint32_t ps_pd_l = 0;
	uint32_t ps_pd_s = 0;
	uint32_t iso_sw = 0;
	//uint32_t shutdownz = 0;
	//uint32_t reg = 0;
	//uint32_t dphy_eb = 0;
	uint32_t iso_reg = 0;
	uint32_t ctr_reg = 0;
	struct csi_phy_info *phy = &csi_info->phy;

	if (!phy || !csi_info) {
		pr_err("fail to get valid phy ptr\n");
		return;
	}

	return;
#if 0
	switch (phy->phy_id) {
		case PHY_2P2:
			/* 2p2lane phy as a 4lane phy  */
			ps_pd_l = BIT_ANLG_PHY_G10_DBG_SEL_ANALOG_MIPI_CSI_2P2LANE_CSI_PS_PD_L;
			ps_pd_s = BIT_ANLG_PHY_G10_DBG_SEL_ANALOG_MIPI_CSI_2P2LANE_CSI_PS_PD_S;
			iso_sw = BIT_ANLG_PHY_G10_ANALOG_MIPI_CSI_2P2LANE_CSI_ISO_SW_EN;
			iso_reg = REG_ANLG_PHY_G10_ANALOG_MIPI_CSI_2P2LANE_CSI_2L_ISO_SW;
			ctr_reg = REG_ANLG_PHY_G10_ANALOG_MIPI_CSI_2P2LANE_CTRL_CSI_2P2L;
			/* 0:2L+2L, 1:4Lane */
			regmap_update_bits(phy->anlg_phy_g1_syscon,
				REG_ANLG_PHY_G10_ANALOG_MIPI_CSI_2P2LANE_CTRL_CSI_2P2L,
				BIT_ANLG_PHY_G10_ANALOG_MIPI_CSI_2P2LANE_CSI_MODE_SEL,
				BIT_ANLG_PHY_G10_ANALOG_MIPI_CSI_2P2LANE_CSI_MODE_SEL);
			break;
		case PHY_4LANE:
			/* phy: 4lane phy */
			ps_pd_l = BIT_ANLG_PHY_G10_ANALOG_MIPI_CSI_4LANE_CSI_PS_PD_L;
			ps_pd_s = BIT_ANLG_PHY_G10_ANALOG_MIPI_CSI_4LANE_CSI_PS_PD_S;
			iso_sw = BIT_ANLG_PHY_G10_ANALOG_MIPI_CSI_4LANE_CSI_ISO_SW_EN;
			iso_reg = REG_ANLG_PHY_G10_ANALOG_MIPI_CSI_4LANE_CSI_4L_ISO_SW;
			ctr_reg = REG_ANLG_PHY_G10_ANALOG_MIPI_CSI_4LANE_CTRL_CSI_4L;
			break;
		case PHY_2LANE:
			/* phy: 2lane phy */
			ps_pd_l = BIT_ANLG_PHY_G10_ANALOG_MIPI_CSI_2LANE_CSI_PS_PD_L;
			ps_pd_s = BIT_ANLG_PHY_G10_ANALOG_MIPI_CSI_2LANE_CSI_PS_PD_S;
			iso_sw = BIT_ANLG_PHY_G10_ANALOG_MIPI_CSI_2LANE_CSI_ISO_SW_EN;
			iso_reg = REG_ANLG_PHY_G10_ANALOG_MIPI_CSI_2LANE_CSI_2L_ISO_SW;
			ctr_reg = REG_ANLG_PHY_G10_ANALOG_MIPI_CSI_2LANE_CTRL_CSI_2L;
			break;
		#if 1
		case PHY_2P2_M:
		case PHY_2P2_S:
			/* 2p2lane phy as a 2lane phy  */
			ps_pd_l = BIT_AON_APB_MIPI_CSI_2P2LANE_PS_PD_L;
			ps_pd_s = BIT_AON_APB_MIPI_CSI_2P2LANE_PS_PD_S;
			iso_sw = BIT_AON_APB_MIPI_CSI_2P2LANE_ISO_SW_EN;

			/* 0:2L+2L, 1:4Lane */
			regmap_update_bits(phy->anlg_phy_g1_syscon,
				REG_ANLG_PHY_G10_ANALOG_MIPI_CSI_2P2LANE_CTRL_CSI_2P2L,
				BIT_ANLG_PHY_G10_ANALOG_MIPI_CSI_2P2LANE_CSI_MODE_SEL,
				BIT_ANLG_PHY_G10_ANALOG_MIPI_CSI_2P2LANE_CSI_MODE_SEL);
			break;
		#endif
		default:
			pr_err("fail to get valid phy id %d\n", phy->phy_id);
			return;
	}
#endif

	if (is_eb) { //power down
		regmap_update_bits(phy->anlg_phy_g1_syscon, ctr_reg, ps_pd_s, ~ps_pd_s);
		udelay(100);
		regmap_update_bits(phy->anlg_phy_g1_syscon, ctr_reg, ps_pd_l, ~ps_pd_l);
		udelay(100);
		regmap_update_bits(phy->anlg_phy_g1_syscon, iso_reg, iso_sw, ~iso_sw);
	} else {//power up
		/* According to the time sequence of CSI-DPHY INIT,
		 * need pull down POWER, DPHY-reset and CSI-2 controller reset
		*/
		csi_shut_down_phy(1, sensor_id);
		csi_reset_shut_down(1, sensor_id);
		
		regmap_update_bits(phy->anlg_phy_g1_syscon, iso_reg, iso_sw, iso_sw);
		udelay(100);
		regmap_update_bits(phy->anlg_phy_g1_syscon, ctr_reg, ps_pd_s, ps_pd_s);
		udelay(100);
		regmap_update_bits(phy->anlg_phy_g1_syscon, ctr_reg, ps_pd_l, ps_pd_l);
		
		/* According to the time sequence of CSI-DPHY INIT,
		 * need pull up POWER, DPHY-reset and CSI-2 controller reset
		*/
		csi_shut_down_phy(0, sensor_id);
		csi_reset_shut_down(0, sensor_id);
	}
}

int csi_ahb_reset(struct csi_phy_info *phy, unsigned int csi_id)
{
	unsigned int flag = 0;

	if (!phy) {
		pr_err("fail to get valid phy ptr\n");
		return -EINVAL;
	}
	pr_info("%s csi, id %d dphy %d\n", __func__, csi_id, phy->phy_id);

	csi_dump_regbase[0] = 0;
	csi_dump_regbase[1] = 0;
	csi_dump_regbase[2] = 0;

	switch (csi_id) {
	case CSI_RX0:
		flag = BIT_MM_AHB_MIPI_CSI0_SOFT_RST;
		break;
	case CSI_RX1:
		flag = BIT_MM_AHB_MIPI_CSI1_SOFT_RST;
		break;
	case CSI_RX2:
		flag = BIT_MM_AHB_MIPI_CSI2_SOFT_RST;
		break;
	default:
		pr_err("fail to get valid csi id %d\n", csi_id);
	}
	regmap_update_bits(phy->cam_ahb_syscon,
			   REG_MM_AHB_AHB_RST, flag, flag);
	udelay(1);
	regmap_update_bits(phy->cam_ahb_syscon,
			   REG_MM_AHB_AHB_RST, flag, ~flag);

	return 0;
}

void csi_controller_enable(struct csi_dt_node_info *dt_info, int32_t idx)
{
	struct csi_phy_info *phy = NULL;
	uint32_t cphy_sel_mask;
	uint32_t cphy_sel_val;
	uint32_t mask_eb = 0;
	uint32_t mask_rst = 0;

	if (!dt_info) {
		pr_err("fail to get valid dt_info ptr\n");
		return;
	}

	phy = &dt_info->phy;
	if (!phy) {
		pr_err("fail to get valid phy ptr\n");
		return;
	}

	pr_info("%s csi, id %d dphy %d\n", __func__, dt_info->controller_id,
		phy->phy_id);

	switch (dt_info->controller_id) {
	case CSI_RX0: {
		csi_dump_regbase[0] = dt_info->reg_base;
		mask_eb = BIT_MM_AHB_CSI0_EB;
		mask_rst = BIT_MM_AHB_MIPI_CSI0_SOFT_RST;
		break;
	}
	case CSI_RX1: {
		csi_dump_regbase[1] = dt_info->reg_base;
		mask_eb = BIT_MM_AHB_CSI1_EB;
		mask_rst = BIT_MM_AHB_MIPI_CSI1_SOFT_RST;
		break;
	}
	case CSI_RX2: {
		csi_dump_regbase[2] = dt_info->reg_base;
		mask_eb = BIT_MM_AHB_CSI2_EB;
		mask_rst = BIT_MM_AHB_MIPI_CSI2_SOFT_RST;
		break;
	}
	default:
		pr_err("fail to get valid csi id\n");
		break;
	}

	regmap_update_bits(phy->cam_ahb_syscon, REG_MM_AHB_AHB_EB,
			mask_eb, mask_eb);
	regmap_update_bits(phy->cam_ahb_syscon, REG_MM_AHB_AHB_RST,
			mask_rst, mask_rst);
	udelay(1);
	regmap_update_bits(phy->cam_ahb_syscon, REG_MM_AHB_AHB_RST,
			mask_rst, ~mask_rst);

	if  (dt_info->controller_id == CSI_RX0 ||
			dt_info->controller_id == CSI_RX1) {
		cphy_sel_mask = 7 << (dt_info->controller_id * 4);

		switch (phy->phy_id) {
		case PHY_2P2: {
			cphy_sel_val = 3;
			break;
		}
		case PHY_4LANE: {
			cphy_sel_val = 2;
			break;
		}
		case PHY_2LANE: {
			cphy_sel_val = 4;
			break;
		}
		case PHY_2P2_S: {
			cphy_sel_val = 0;
			break;
		}
		case PHY_2P2_M: {
			cphy_sel_val = 1;
			break;
		}
		default:
			pr_err("fail to get valid csi phy id\n");
			return;
		}
		cphy_sel_val  <<= dt_info->controller_id * 4;
	} else {
		cphy_sel_mask = 7 << 8;

		switch (phy->phy_id) {
		case PHY_2LANE: {
			cphy_sel_val = 0;
			break;
		}
		case PHY_2P2_S: {
			cphy_sel_val = 1;
			break;
		}
		case PHY_2P2_M: {
			cphy_sel_val = 2;
			break;
		}
		default:
			pr_err("fail to get valid csi phy id\n");
			return;
		}
		cphy_sel_val  <<= 8;
	}
	/*regmap_hwlock_update_bits(phy->cam_ahb_syscon,
			REG_MM_AHB_MIPI_CSI_SEL_CTRL,
			cphy_sel_mask,
			cphy_sel_val);*/
}

void dphy_init(struct csi_phy_info *phy, int32_t idx)
{
	if (!phy) {
		pr_err("fail to get valid phy ptr\n");
		return;
	}

	if (phy->phy_id == PHY_4LANE || phy->phy_id == PHY_2LANE)
		dphy_cfg_clr(idx);
}

void csi_set_on_lanes(uint8_t lanes, int32_t idx)
{
	CSI_REG_MWR(idx, LANE_NUMBER, 0x7, (lanes - 1));
}

void csi_reset_shut_down(uint8_t shutdown, int32_t idx)
{
	/* DPHY reset output, active low */
	CSI_REG_MWR(idx, RST_DPHY_N, BIT_0, shutdown ? 0 : 1);
	/* CSI-2 controller reset output, active low */
	CSI_REG_MWR(idx, RST_CSI2_N, BIT_0, shutdown ? 0 : 1);
}

/* PHY power down input, active low */
void csi_shut_down_phy(uint8_t shutdown, int32_t idx)
{
	CSI_REG_MWR(idx, PHY_PD_N, BIT_0, shutdown ? 0 : 1);
}

void csi_reset_controller(int32_t idx)
{
	CSI_REG_MWR(idx, RST_CSI2_N, BIT_0, 0);
	CSI_REG_MWR(idx, RST_CSI2_N, BIT_0, 1);
}

void csi_reset_phy(int32_t idx)
{
	CSI_REG_MWR(idx, RST_DPHY_N, BIT_0, 0);
	CSI_REG_MWR(idx, RST_DPHY_N, BIT_0, 1);
}

void csi_event_enable(int32_t idx)
{
	CSI_REG_WR(idx, MASK0, CSI_MASK0);
	CSI_REG_WR(idx, MASK1, CSI_MASK1);
}

void csi_close(int32_t idx)
{
	csi_shut_down_phy(1, idx);
	csi_reset_controller(idx);
	csi_reset_phy(idx);
}
