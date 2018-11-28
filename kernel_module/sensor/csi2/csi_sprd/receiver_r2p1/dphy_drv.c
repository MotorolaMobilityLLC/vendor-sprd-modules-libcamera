/*
 * Copyright (C) 2018 Spreadtrum Communications Inc.
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
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/regmap.h>
#include <linux/semaphore.h>
#include <sprd_mm.h>

#include "csi_api.h"
#include "csi_driver.h"
#include "sprd_sensor_core.h"
#include "dphy_drv.h"

#include "mm_ahb.h"

#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt) "dphy_drv: %d %d %s : "\
	fmt, current->pid, __LINE__, __func__


#define PHY_TESTCLR			BIT_0
#define PHY_TESTCLK			BIT_1
#define PHY_TESTDIN			0xFF
#define PHY_TESTDOUT		0xFF00
#define PHY_TESTEN			BIT_16

#define RX0_MSK  0x3F
#define RX1_MSK  0xFC0
#define RX2_MSK  0x7000

static char *phy_name[] = {
	"mipi-csi-phy0", /* 4Lane */
	"mipi-csi-phy1", /* 2p2l */
	"mipi-csi-phy2", /* 2lane */
	"mipi-csi-phy1-m", /*2p2l_m*/
	"mipi-csi-phy1-s", /*2p2l_s*/
};

static struct dphy_info *g_phy_info[4];

static struct dphy_info *get_phy_info(int phyid)
{
	return g_phy_info[phyid];
}

/* phy testclear used to reset phy to right default state */
static void dphy_cfg_clr(int32_t idx)
{
	CSI_REG_MWR(idx, PHY_TEST_CRTL0, PHY_TESTCLR, 1);
	udelay(1);
	CSI_REG_MWR(idx, PHY_TEST_CRTL0, PHY_TESTCLR, 0);
	udelay(1);
}

static void csi_dphy_2p2_testclr_init(struct dphy_info *phy)
{
	unsigned int mask = 0;
	pr_info("entry");
#if 0
	regmap_update_bits(phy->anlg_phy, phy->testclr_s_en,
		phy->testclr_s_en_msk, phy->testclr_s_en_msk);

	/* config dbg_if_sel_m,s,db */
	regmap_update_bits(phy->anlg_phy, phy->cfg_0,
		phy->cfg_0_msk, phy->cfg_0_msk);

	/* set dsi_if_sel_db */
	regmap_update_bits(phy->anlg_phy, phy->dsi_if_sel_db,
		phy->dsi_if_sel_db_msk, ~(phy->dsi_if_sel_db_msk));

	/* set csi_if_sel_m */
	regmap_update_bits(phy->anlg_phy, phy->csi_if_sel_m,
		phy->csi_if_sel_m_msk, ~(phy->csi_if_sel_m_msk));

	/* set csi_if_sel_s */
	regmap_update_bits(phy->anlg_phy, phy->csi_if_sel_s,
		phy->csi_if_sel_s_msk, ~(phy->csi_if_sel_s_msk));

	/* set testclr_m/s_sel */
	regmap_update_bits(phy->anlg_phy, phy->testclr_m_sel,
		phy->testclr_m_sel_msk, phy->testclr_m_sel_msk);
	regmap_update_bits(phy->anlg_phy, phy->testclr_s_sel,
		phy->testclr_s_sel_msk, phy->testclr_s_sel_msk);
#else
	mask = MASK_ANLG_PHY_G10_ANALOG_MIPI_CSI_4LANE_CSI_2P2L_TESTCLR_S_EN;
	regmap_update_bits(phy->anlg_phy,
		REG_ANLG_PHY_G10_ANALOG_MIPI_CSI_4LANE_CSI_4L_BIST_TEST,
		mask, mask);

	mask = MASK_ANLG_PHY_G10_DBG_SEL_ANALOG_MIPI_CSI_2P2LANE_CSI_IF_SEL_S
		| MASK_ANLG_PHY_G10_DBG_SEL_ANALOG_MIPI_CSI_2P2LANE_CSI_IF_SEL_M
		| MASK_ANLG_PHY_G10_DBG_SEL_ANALOG_MIPI_CSI_2P2LANE_DSI_IF_SEL_DB;
	regmap_update_bits(phy->anlg_phy,
		REG_ANLG_PHY_G10_ANALOG_MIPI_CSI_2P2LANE_REG_SEL_CFG_0,
		mask, mask);

	mask = MASK_ANLG_PHY_G10_ANALOG_MIPI_CSI_2P2LANE_DSI_IF_SEL_DB;
	regmap_update_bits(phy->anlg_phy,
		REG_ANLG_PHY_G10_ANALOG_MIPI_CSI_2P2LANE_CSI_2P2L_CTRL_DB,
		mask, ~mask);

	mask = MASK_ANLG_PHY_G10_ANALOG_MIPI_CSI_2P2LANE_CSI_IF_SEL_S;
	regmap_update_bits(phy->anlg_phy,
		REG_ANLG_PHY_G10_ANALOG_MIPI_CSI_2P2LANE_CSI_2P2L_CTRL_S,
		mask, ~mask);

	mask = MASK_ANLG_PHY_G10_ANALOG_MIPI_CSI_2P2LANE_CSI_IF_SEL_M;
	regmap_update_bits(phy->anlg_phy,
		REG_ANLG_PHY_G10_ANALOG_MIPI_CSI_2P2LANE_CSI_2P2L_CTRL_M,
		mask, ~mask);

	mask = MASK_ANLG_PHY_G10_ANALOG_MIPI_CSI_4LANE_CSI_2P2L_TESTCLR_M_SEL
		| MASK_ANLG_PHY_G10_ANALOG_MIPI_CSI_4LANE_CSI_2P2L_TESTCLR_S_SEL;
	regmap_update_bits(phy->anlg_phy,
		REG_ANLG_PHY_G10_ANALOG_MIPI_CSI_4LANE_CSI_4L_BIST_TEST,
		mask, mask);
#endif
}

static void csi_dphy_2p2_testclr_set(struct dphy_info *phy)
{
	unsigned int mask = 0;
	pr_info("entry");
#if 0
	/* set testclr_db/m/s */
	regmap_update_bits(phy->anlg_phy, phy->dsi_testclr_db,
		phy->dsi_testclr_db_msk, phy->dsi_testclr_db_msk);
	udelay(1);
	regmap_update_bits(phy->anlg_phy, phy->testclr_m,
		phy->testclr_m_msk, phy->testclr_m_msk);
	regmap_update_bits(phy->anlg_phy, phy->testclr_s,
		phy->testclr_s_msk, phy->testclr_s_msk);
#else
	mask = MASK_ANLG_PHY_G10_ANALOG_MIPI_CSI_2P2LANE_DSI_TESTCLR_DB;
	regmap_update_bits(phy->anlg_phy,
		REG_ANLG_PHY_G10_ANALOG_MIPI_CSI_2P2LANE_CSI_2P2L_TEST_DB,
		mask, mask);
	udelay(1);
	mask = MASK_ANLG_PHY_G10_ANALOG_MIPI_CSI_4LANE_CSI_2P2L_TESTCLR_M
		| MASK_ANLG_PHY_G10_ANALOG_MIPI_CSI_4LANE_CSI_2P2L_TESTCLR_S;
	regmap_update_bits(phy->anlg_phy,
		REG_ANLG_PHY_G10_ANALOG_MIPI_CSI_4LANE_CSI_4L_BIST_TEST,
		mask, mask);
#endif
}

static void csi_dphy_2p2_testclr_clear(struct dphy_info *phy)
{
	unsigned int mask = 0;
	pr_info("entry");

#if 0
	/* clear testclr_db/m/s */
	regmap_update_bits(phy->anlg_phy, phy->dsi_testclr_db,
			phy->dsi_testclr_db_msk, ~(phy->dsi_testclr_db_msk));
	regmap_update_bits(phy->anlg_phy, phy->testclr_m,
			phy->testclr_m_msk, ~(phy->testclr_m_msk));
		regmap_update_bits(phy->anlg_phy, phy->testclr_s,
			phy->testclr_s_msk, ~(phy->testclr_s_msk));
#else
	mask = MASK_ANLG_PHY_G10_ANALOG_MIPI_CSI_2P2LANE_DSI_TESTCLR_DB;
	regmap_update_bits(phy->anlg_phy,
		REG_ANLG_PHY_G10_ANALOG_MIPI_CSI_2P2LANE_CSI_2P2L_TEST_DB,
		mask, ~mask);
	mask = MASK_ANLG_PHY_G10_ANALOG_MIPI_CSI_4LANE_CSI_2P2L_TESTCLR_M
		| MASK_ANLG_PHY_G10_ANALOG_MIPI_CSI_4LANE_CSI_2P2L_TESTCLR_S;
	regmap_update_bits(phy->anlg_phy,
		REG_ANLG_PHY_G10_ANALOG_MIPI_CSI_4LANE_CSI_4L_BIST_TEST,
		mask, ~mask);
#endif
}

static void csi_dphy_2p2_reset(struct dphy_info *phy, int csiId)
{
	unsigned int cphy_sel_mask;
	unsigned int cphy_sel_val;
	unsigned int mask;
	pr_info("entry");

	if (!phy) {
		pr_err("fail to get valid phy ptr\n");
		return;
	}
#if 0
	switch (csiId) {
	case CSI_RX0:
	case CSI_RX1:
		regmap_update_bits(phy->anlg_phy, phy->testclr_m,
			phy->testclr_m_msk, phy->testclr_m_msk);
		regmap_update_bits(phy->anlg_phy, phy->testclr_m,
			phy->testclr_m_msk, ~(phy->testclr_m_msk));

		regmap_update_bits(phy->anlg_phy, phy->testclr_s,
			phy->testclr_s_msk, phy->testclr_s_msk);
		regmap_update_bits(phy->anlg_phy, phy->testclr_s,
			phy->testclr_s_msk, ~(phy->testclr_s_msk));
		break;
	case CSI_RX2:
		regmap_update_bits(phy->anlg_phy, phy->testclr_s,
			phy->testclr_s_msk, phy->testclr_s_msk);
		regmap_update_bits(phy->anlg_phy, phy->testclr_s,
			phy->testclr_s_msk, ~(phy->testclr_s_msk));

		regmap_update_bits(phy->anlg_phy, phy->testclr_m,
			phy->testclr_m_msk, phy->testclr_m_msk);
		regmap_update_bits(phy->anlg_phy, phy->testclr_m,
			phy->testclr_m_msk, ~(phy->testclr_m_msk));
		break;
	default:
		pr_err("fail to get valid csi_rx id\n");
	}
#else
	switch (csiId) {
	case CSI_RX0:
	case CSI_RX1:
		cphy_sel_mask = 7 << (csiId * 6);
		cphy_sel_val  = 3 << (csiId * 6);

		regmap_update_bits(phy->cam_ahb,
				REG_MM_AHB_MIPI_CSI_SEL_CTRL,
				cphy_sel_mask, cphy_sel_val);
		mask = MASK_ANLG_PHY_G10_ANALOG_MIPI_CSI_4LANE_CSI_2P2L_TESTCLR_M
		| MASK_ANLG_PHY_G10_ANALOG_MIPI_CSI_4LANE_CSI_2P2L_TESTCLR_S;
		regmap_update_bits(phy->anlg_phy,
		REG_ANLG_PHY_G10_ANALOG_MIPI_CSI_4LANE_CSI_4L_BIST_TEST,
		mask, mask);
		regmap_update_bits(phy->anlg_phy,
		REG_ANLG_PHY_G10_ANALOG_MIPI_CSI_4LANE_CSI_4L_BIST_TEST,
		mask, ~mask);
		regmap_update_bits(phy->cam_ahb,
				REG_MM_AHB_MIPI_CSI_SEL_CTRL,
				cphy_sel_mask, 0x0);
		break;
	case CSI_RX2:
		cphy_sel_mask = 7 << (csiId * 6);
		cphy_sel_val  = 1 << (csiId * 6);

		regmap_update_bits(phy->cam_ahb,
				REG_MM_AHB_MIPI_CSI_SEL_CTRL,
				cphy_sel_mask, cphy_sel_val);

		mask = MASK_ANLG_PHY_G10_ANALOG_MIPI_CSI_4LANE_CSI_2P2L_TESTCLR_S;
		regmap_update_bits(phy->anlg_phy,
		REG_ANLG_PHY_G10_ANALOG_MIPI_CSI_4LANE_CSI_4L_BIST_TEST,
		mask, mask);
		regmap_update_bits(phy->anlg_phy,
		REG_ANLG_PHY_G10_ANALOG_MIPI_CSI_4LANE_CSI_4L_BIST_TEST,
		mask, ~mask);
		regmap_update_bits(phy->cam_ahb,
				REG_MM_AHB_MIPI_CSI_SEL_CTRL,
				cphy_sel_mask, 0x0);
		cphy_sel_val  = 2 << (csiId * 6);
		regmap_update_bits(phy->cam_ahb,
				REG_MM_AHB_MIPI_CSI_SEL_CTRL,
				cphy_sel_mask, cphy_sel_val);
		mask = MASK_ANLG_PHY_G10_ANALOG_MIPI_CSI_4LANE_CSI_2P2L_TESTCLR_M;
		regmap_update_bits(phy->anlg_phy,
		REG_ANLG_PHY_G10_ANALOG_MIPI_CSI_4LANE_CSI_4L_BIST_TEST,
		mask, mask);
		regmap_update_bits(phy->anlg_phy,
		REG_ANLG_PHY_G10_ANALOG_MIPI_CSI_4LANE_CSI_4L_BIST_TEST,
		mask, ~mask);
		regmap_update_bits(phy->cam_ahb,
				REG_MM_AHB_MIPI_CSI_SEL_CTRL,
				cphy_sel_mask, 0x0);
		break;
	default:
		pr_err("fail to get valid csi_rx id\n");
	}
#endif
}

int phy_parse_dt(int phyid, struct device *dev)
{
	u32 args[2] = {0};
	int ret = 0;
	int out_value;
	const char *name;
	struct device_node *dn = NULL;
	struct dphy_info *phy = NULL;

	if (phyid > PHY_MAX) {
		pr_err("invalid phyid: %d\n", phyid);
		goto err;
	}

	phy = devm_kzalloc(dev, sizeof(*phy), GFP_KERNEL);
		if (!phy)
			return -EINVAL;

	dn = of_find_node_by_name(NULL, phy_name[phyid]);
	if (!dn) {
		pr_err("get phy node:%s failed\n", phy_name[phyid]);
		goto err;
	}

	of_property_read_string(dn, "sprd,phyname", &name);
	of_property_read_u32(dn, "sprd,phyid", &out_value);

	phy->phy_id = phyid;

	pr_info("name:%s, id:%d\n", name, out_value);

	phy->cam_ahb = syscon_regmap_lookup_by_phandle(dn, "sprd,cam-ahb-syscon");
	if (IS_ERR_OR_NULL(phy->cam_ahb)) {
		pr_err("get cam-ahb-syscon failed!\n");
		goto err;
	}

	/* parse anlg 10 */
	phy->anlg_phy = syscon_regmap_lookup_by_name(dn, "iso_sw_en");
	if (IS_ERR_OR_NULL(phy->anlg_phy)) {
		pr_err("get anlg phy failed\n");
		/* return PRT_ERR(csi_info->phy.anlg_phy); */
		goto err;
	}

	ret = syscon_get_args_by_name(dn, "iso_sw_en", 2, args);
	if (ret == 2) {
		phy->iso_sw_en = args[0];
		phy->iso_sw_en_msk = args[1];
	} else {
		pr_err("get csi anlg iso_sw_en failed\n");
		goto err;
	}

	ret = syscon_get_args_by_name(dn, "ps_pd_s", 2, args);
	if (ret == 2) {
		phy->ps_pd_s = args[0];
		phy->ps_pd_s_msk = args[1];
	} else {
		pr_err("get csi anlg ps_pd_s failed\n");
		goto err;
	}

	ret = syscon_get_args_by_name(dn, "ps_pd_l", 2, args);
	if (ret == 2) {
		phy->ps_pd_l = args[0];
		phy->ps_pd_l_msk = args[1];
	} else {
		pr_err("get csi anlg ps_pd_l failed\n");
		goto err;
	}

	/* csi mode for 2p2l */
	if (phy->phy_id == 1
		|| phy->phy_id == 3
		|| phy->phy_id == 4) {
		ret = syscon_get_args_by_name(dn, "csi_mode_sel", 2, args);
		if (ret == 2) {
			phy->csi_mode = args[0];
			phy->csi_mode_msk = args[1];
		} else {
			pr_err("get csi mode sel failed\n");
			goto err;
		}
#if 0
		ret = syscon_get_args_by_name(dn, "2p2l_cfg_0", 2, args);
		if (ret == 2) {
			phy->cfg_0 = args[0];
			phy->cfg_0_msk = args[1];
		} else {
			pr_err("get 2p2l_cfg_0 failed\n");
			goto err;
		}
		ret = syscon_get_args_by_name(dn, "dsi_if_sel_db", 2, args);
		if (ret == 2) {
			phy->dsi_if_sel_db = args[0];
			phy->dsi_if_sel_db_msk = args[1];
		} else {
			pr_err("get dsi_if_sel_db failed\n");
			goto err;
		}
		ret = syscon_get_args_by_name(dn, "dsi_testclr_db", 2, args);
		if (ret == 2) {
			phy->dsi_testclr_db = args[0];
			phy->dsi_testclr_db_msk = args[1];
		} else {
			pr_err("get dsi_testclr_db failed\n");
			goto err;
		}
		ret = syscon_get_args_by_name(dn, "csi_if_sel_s", 2, args);
		if (ret == 2) {
			phy->csi_if_sel_s = args[0];
			phy->csi_if_sel_s_msk = args[1];
		} else {
			pr_err("get csi_if_sel_s failed\n");
			goto err;
		}
		ret = syscon_get_args_by_name(dn, "csi_if_sel_m", 2, args);
		if (ret == 2) {
			phy->csi_if_sel_m = args[0];
			phy->csi_if_sel_m_msk = args[1];
		} else {
			pr_err("get csi_if_sel_m failed\n");
			goto err;
		}
		ret = syscon_get_args_by_name(dn, "testclr_m_sel", 2, args);
		if (ret == 2) {
			phy->testclr_m_sel = args[0];
			phy->testclr_m_sel_msk = args[1];
		} else {
			pr_err("testclr_m_sel failed\n");
			goto err;
		}
		ret = syscon_get_args_by_name(dn, "testclr_s_sel", 2, args);
		if (ret == 2) {
			phy->testclr_s_sel = args[0];
			phy->testclr_s_sel_msk = args[1];
		} else {
			pr_err("get testclr_s_sel failed\n");
			goto err;
		}
		ret = syscon_get_args_by_name(dn, "testclr_m", 2, args);
		if (ret == 2) {
			phy->testclr_m = args[0];
			phy->testclr_m_msk = args[1];
		} else {
			pr_err("get testclr_m failed\n");
			goto err;
		}
		ret = syscon_get_args_by_name(dn, "testclr_s", 2, args);
		if (ret == 2) {
			phy->testclr_s = args[0];
			phy->testclr_s_msk = args[1];
		} else {
			pr_err("get testclr_s failed\n");
			goto err;
		}
		ret = syscon_get_args_by_name(dn, "testclr_s_en", 2, args);
		if (ret == 2) {
			phy->testclr_s_en = args[0];
			phy->testclr_s_en_msk = args[1];
		} else {
			pr_err("get testclr_s_en failed\n");
			goto err;
		}
	#endif
	}

	g_phy_info[phyid] = phy;
	return 0;
err:
	pr_err("paser phy dts failed\n");
	return -1;
}

void csi_phy_power_down(unsigned int phyid, int csiId, int is_eb)
{
	unsigned int shutdownz = 0;
	unsigned int reg = 0;
	struct dphy_info *phy = get_phy_info(phyid);

	if (!phy) {
		pr_err("fail to get valid phy ptr\n");
		return;
	}

	pr_info("phyid: %d, csi_id:%d, is_eb:%d\n", phyid, csiId, is_eb);

	switch (csiId) {
	case CSI_RX0:
		shutdownz =
		MASK_ANLG_PHY_G10_ANALOG_MIPI_CSI_4LANE_FORCE_CSI_PHY_SHUTDOWNZ;
		reg = REG_ANLG_PHY_G10_ANALOG_MIPI_CSI_4LANE_CSI_4L_BIST_TEST;
		break;
	case CSI_RX1:
		shutdownz =
		MASK_ANLG_PHY_G10_ANALOG_MIPI_CSI_4LANE_FORCE_CSI_S_PHY_SHUTDOWNZ;
		reg = REG_ANLG_PHY_G10_ANALOG_MIPI_CSI_4LANE_CSI_4L_BIST_TEST;
		break;
	case CSI_RX2:
		shutdownz =
		MASK_ANLG_PHY_G10_ANALOG_MIPI_CSI_2LANE_FORCE_CSI_PHY_SHUTDOWNZ;
		reg = REG_ANLG_PHY_G10_ANALOG_MIPI_CSI_2LANE_MIPI_PHY_BIST_TEST;
		break;
	default:
		pr_err("fail to get valid csi_rx id\n");
	}

	switch (phy->phy_id) {
	case PHY_4LANE:
	case PHY_2LANE:
		break;
	case PHY_2P2:
		/* 2p2lane phy as a 4lane phy
		 * 0:2L+2L, 1:4Lane
		 */
		regmap_update_bits(phy->anlg_phy, phy->csi_mode,
			phy->csi_mode_msk, phy->csi_mode_msk);
		break;
	case PHY_2P2_M:
	case PHY_2P2_S:
		/* 2p2lane phy as a 2lane phy
		 * 0:2L+2L, 1:4Lane
		 */
		regmap_update_bits(phy->anlg_phy, phy->csi_mode,
			phy->csi_mode_msk, ~(phy->csi_mode_msk));
		break;
	default:
		pr_err("fail to get valid phy id %d\n", phy->phy_id);
		return;
	}

	if (is_eb) {/* power down */
		regmap_update_bits(phy->anlg_phy, phy->iso_sw_en,
				phy->iso_sw_en_msk, phy->iso_sw_en_msk);
		udelay(100);
		regmap_update_bits(phy->anlg_phy, phy->ps_pd_l,
				phy->ps_pd_l_msk, phy->ps_pd_l_msk);
		udelay(100);
		regmap_update_bits(phy->anlg_phy, phy->ps_pd_s,
				phy->ps_pd_l_msk, phy->ps_pd_l_msk);
		udelay(100);
		/* set phy force shutdown */
		regmap_update_bits(phy->anlg_phy, reg,
				shutdownz, shutdownz);
	} else { /* power on */
		/* According to the time sequence of CSI-DPHY INIT,
		 * need pull down POWER, DPHY-reset and CSI-2 controller reset
		 */
		csi_shut_down_phy(1, csiId);
		csi_reset_shut_down(1, csiId);

		if (phy->phy_id == PHY_2P2
			|| phy->phy_id == PHY_2P2_M
			|| phy->phy_id == PHY_2P2_S) {
			csi_dphy_2p2_testclr_init(phy);
			csi_dphy_2p2_testclr_set(phy);
		} else if (phy->phy_id == PHY_4LANE
			|| phy->phy_id == PHY_2LANE){
			CSI_REG_MWR(csiId, PHY_TEST_CRTL0, PHY_TESTCLR, 1);
		}

		regmap_update_bits(phy->anlg_phy, phy->ps_pd_s,
				phy->ps_pd_s_msk, ~(phy->ps_pd_s_msk));
		udelay(100);
		regmap_update_bits(phy->anlg_phy, phy->ps_pd_l,
				phy->ps_pd_l_msk, ~(phy->ps_pd_l_msk));
		udelay(100);
		regmap_update_bits(phy->anlg_phy, phy->iso_sw_en,
				phy->iso_sw_en_msk, ~(phy->iso_sw_en_msk));

		/* set phy force shutdown */
		regmap_update_bits(phy->anlg_phy, reg,
				shutdownz, shutdownz);

		/* According to the time sequence of CSI-DPHY INIT,
		 * need pull up POWER, DPHY-reset and CSI-2 controller reset
		 */

		if (phy->phy_id == PHY_2P2
			|| phy->phy_id == PHY_2P2_M
			|| phy->phy_id == PHY_2P2_S) {
			csi_dphy_2p2_testclr_clear(phy);
			csi_dphy_2p2_reset(phy, csiId);
		} else if (phy->phy_id == PHY_4LANE
			|| phy->phy_id == PHY_2LANE){
			CSI_REG_MWR(csiId, PHY_TEST_CRTL0, PHY_TESTCLR, 0);
		}

		csi_shut_down_phy(0, csiId);
		csi_reset_shut_down(0, csiId);
	}
}

int dphy_csi_path_cfg(struct csi_dt_node_info *dt_info)
{
	/* uint32_t cphy_sel_mask; */
	uint32_t cphy_sel_val;

	if (!dt_info) {
		pr_err("input param is invalid\n");
		return -EINVAL;
	}

	switch (dt_info->phy_id) {
	case PHY_2P2: {
		cphy_sel_val = 3;
		break;
	}
	case PHY_4LANE: {
		cphy_sel_val = 0x12;
		break;
	}
	case PHY_2LANE: {
		cphy_sel_val = 0x24;
		break;
	}
	case PHY_2P2_S: {
		cphy_sel_val = 0x0;
		break;
	}
	case PHY_2P2_M: {
		cphy_sel_val = 0x1;
		break;
	}
	default:
		pr_err("fail to get valid csi phy id\n");
		return -1;
	}

	cphy_sel_val = cphy_sel_val << (dt_info->controller_id*6);
	pr_info("phyid:%d, csi_id:%d, select value:0x%x\n",
		dt_info->phy_id, dt_info->controller_id, cphy_sel_val);
	regmap_update_bits(dt_info->syscon.mm_ahb,
		dt_info->syscon.dphy_sel, dt_info->syscon.dphy_msk,
		cphy_sel_val);

	return 0;
}

/*
 * testen:1 + clk:falling => addr write operation
 * testen:0 + clk:rising  => data write operation
 * testout: (RO) read out
 * testin: phy internal register programme
 *
 * testclr ___/---\_____
 *
 * testen _________/------\______
 *
 * testclk ____________/--\____/--\____/--\___
 *
 * testin __________________/----\______/---\___
 *
 * testout ____________________/-----\_/-----\___
 *
 */
/* static void phy_testclr(int idx)
{
	CSI_REG_MWR(idx, PHY_TEST_CRTL0, PHY_TESTCLR, 1);
	udelay(1);
	CSI_REG_MWR(idx, PHY_TEST_CRTL0, PHY_TESTCLR, 0);
	udelay(1);
} */

static void phy_testen(int idx, int value)
{
	CSI_REG_MWR(idx, PHY_TEST_CRTL0, PHY_TESTCLR, value);
}

static void phy_testclk(int idx, int value)
{
	CSI_REG_MWR(idx, PHY_TEST_CRTL0, PHY_TESTCLK, value);
}

static void phy_write(int idx, int addr, int data)
{
	int temp;

	pr_info("idx:%d, addr:0x%x, data:0x%x\n", idx, addr, data);
	/* for addr */
	phy_testen(idx, 1);
	udelay(2);

	/* set addr */
	phy_testclk(idx, 1);
	udelay(2);
	CSI_REG_MWR(idx, PHY_TEST_CRTL1, PHY_TESTDIN, addr);
	phy_testclk(idx, 0);
	udelay(2);

	/* readout value */
	temp = CSI_REG_RD(idx, PHY_TEST_CRTL1) & 0x0000ff00;
	temp |= data;
	pr_info("temp: 0x%x\n", temp);

	/* for data */
	phy_testen(idx, 0);
	udelay(2);
	CSI_REG_MWR(idx, PHY_TEST_CRTL1, PHY_TESTDIN, temp);
	phy_testclk(idx, 1);
	udelay(1);
}

void dphy_init_state(unsigned int phyid, int csi_id, int sensor_id)
{
	struct dphy_info *phy = get_phy_info(phyid);
	if (!phy) {
		pr_err("fail to get valid phy ptr\n");
		return;
	}

	if (phy->phy_id == PHY_4LANE || phy->phy_id == PHY_2LANE)
		dphy_cfg_clr(phy->phy_id);

	if (phy->phy_id == PHY_2P2
		|| phy->phy_id == PHY_2P2_M
		|| phy->phy_id == PHY_2P2_S) {
		regmap_update_bits(phy->cam_ahb, REG_MM_AHB_MIPI_CSI_SEL_CTRL,
			0x7 << (csi_id*6), 0x0 << (csi_id*6));

		/* To init 2p2l_s on SharkL5 */
		phy_write(sensor_id, 0x4d, 0x10);
		phy_write(sensor_id, 0x5d, 0x11);
	}
}
