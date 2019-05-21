/* #include "mm_dvfs_reg.h" */
#include "mmsys_dvfs_comm.h"
#include "mm_dvfs.h"
#include "dcam_dvfs.h"
#include "sharkl5pro_mm_dvfs_coffe.h"


struct ip_dvfs_map_cfg  dcam_dvfs_config_table[] = {
	{0, REG_MM_DVFS_AHB_DCAM_IF_INDEX0_MAP,
		VOLT70, DCAM_CLK1920, DCAM_CLK_INDEX_1920,
		3, 0, DCAMAXI_CLK_INDEX_2560, 0},
	{1, REG_MM_DVFS_AHB_DCAM_IF_INDEX1_MAP,
		VOLT70, DCAM_CLK1920, DCAM_CLK_INDEX_1920,
		0, 0, DCAMAXI_CLK_INDEX_2560, 0},
	{2, REG_MM_DVFS_AHB_DCAM_IF_INDEX2_MAP,
		VOLT70, DCAM_CLK2560, DCAM_CLK_INDEX_2560,
		3, 0, DCAMAXI_CLK_INDEX_2560, 0},
	{3, REG_MM_DVFS_AHB_DCAM_IF_INDEX3_MAP,
		VOLT70, DCAM_CLK2560, DCAM_CLK_INDEX_2560,
		0, 0, DCAMAXI_CLK_INDEX_3072, 0},
	{4, REG_MM_DVFS_AHB_DCAM_IF_INDEX4_MAP,
		VOLT70, DCAM_CLK3072, DCAM_CLK_INDEX_3072,
		3, 0, DCAMAXI_CLK_INDEX_3072, 0},
	{5, REG_MM_DVFS_AHB_DCAM_IF_INDEX5_MAP,
		VOLT70, DCAM_CLK3072, DCAM_CLK_INDEX_3072,
		0, 0, DCAMAXI_CLK_INDEX_3072, 0},
	{6, REG_MM_DVFS_AHB_DCAM_IF_INDEX6_MAP,
		VOLT75, DCAM_CLK3840, DCAM_CLK_INDEX_3840,
		0, 0, DCAMAXI_CLK_INDEX_3840, 0},
	{7, REG_MM_DVFS_AHB_DCAM_IF_INDEX7_MAP,
		VOLT80, DCAM_CLK4680, DCAM_CLK_INDEX_3840,
		0, 0, DCAMAXI_CLK_INDEX_4680, 0},
};

/* userspace  interface*/
static int  ip_hw_dvfs_en(struct devfreq *devfreq, unsigned int dvfs_eb)
{
	u32 dfs_en_reg;

	mutex_lock(&mmsys_glob_reg_lock);
	dfs_en_reg = DVFS_REG_RD(REG_MM_DVFS_AHB_MM_DFS_EN_CTRL);
	if (dvfs_eb)
		DVFS_REG_WR(REG_MM_DVFS_AHB_MM_DFS_EN_CTRL,
			dfs_en_reg | BIT_DCAM_IF_DFS_EN);
	else
		DVFS_REG_WR(REG_MM_DVFS_AHB_MM_DFS_EN_CTRL,
			dfs_en_reg & (~BIT_DCAM_IF_DFS_EN));
	mutex_unlock(&mmsys_glob_reg_lock);
	DVFS_TRACE("dvfs ops: %s, dvfs_eb=%d\n", __func__, dvfs_eb);
	return 1;
}

static int ip_auto_tune_en(struct devfreq *devfreq, unsigned long dvfs_eb)
{
	DVFS_TRACE("dvfs ops: %s\n", __func__);
	/* dcam has no this funcation */
	return 1;
}

/*work-idle dvfs map  ops*/
static int  get_ip_dvfs_table(struct devfreq *devfreq,
		struct ip_dvfs_map_cfg *dvfs_table)
{
	int i = 0;

	DVFS_TRACE("dvfs ops: %s\n", __func__);
	for (i = 0; i < MAX_DVFS_INDEX; i++) {
		dvfs_table[i].map_index = dcam_dvfs_config_table[i].map_index;
		dvfs_table[i].reg_add = dcam_dvfs_config_table[i].reg_add;
		dvfs_table[i].volt = dcam_dvfs_config_table[i].volt;
		dvfs_table[i].clk = dcam_dvfs_config_table[i].clk;
		dvfs_table[i].fdiv_denom = dcam_dvfs_config_table[i].fdiv_denom;
		dvfs_table[i].fdiv_num = dcam_dvfs_config_table[i].fdiv_num;
		dvfs_table[i].axi_index = dcam_dvfs_config_table[i].axi_index;
		dvfs_table[i].mtx_index = dcam_dvfs_config_table[i].mtx_index;
	}

	return 1;
}

static int  set_ip_dvfs_table(struct devfreq *devfreq,
		struct ip_dvfs_map_cfg *dvfs_table)
{
	int i = 0;

	for (i = 0; i < MAX_DVFS_INDEX; i++) {
		if (dcam_dvfs_config_table[i].reg_add ==
				dvfs_table->reg_add) {
			memcpy(&dcam_dvfs_config_table[i],
				dvfs_table, sizeof(struct ip_dvfs_map_cfg));
			break;
		}
	}
	if (i == 8)
		pr_err("fail to set dvfs table.");
	DVFS_TRACE("dvfs ops: %s\n", __func__);
	return 1;
}

static void get_ip_index_from_table(struct ip_dvfs_map_cfg *dvfs_cfg,
	unsigned long work_freq, unsigned int  *index)
{
	unsigned long  set_clk = 0;
	u32 i;

	*index = 0;
	for (i = 0; i < MAX_DVFS_INDEX; i++) {
		set_clk = dcam_dvfs_config_table[i].clk_freq;
		if (work_freq == set_clk || work_freq < set_clk) {
			*index = i;
			break;
		}
		if (i == 7)
			*index = 7;
	}
	DVFS_TRACE("dvfs ops: %s\n", __func__);

}

static int  set_work_freq(struct devfreq *devfreq, unsigned long work_freq)
{
	u32 index_cfg_reg = 0;
	u32 index = 0;

	get_ip_index_from_table(dcam_dvfs_config_table,  work_freq, &index);
	index_cfg_reg = DVFS_REG_RD(REG_MM_DVFS_AHB_DCAM_IF_DVFS_INDEX_CFG);
	index_cfg_reg = (index_cfg_reg & (~0x7)) | index;
	DVFS_REG_WR(REG_MM_DVFS_AHB_DCAM_IF_DVFS_INDEX_CFG, index_cfg_reg);
	DVFS_TRACE("dvfs ops: %s, work_freq=%lu, index=%d,\n",
		__func__, work_freq, index);
	return 1;
}

static int set_idle_freq(struct devfreq *devfreq, unsigned long idle_freq)
{
	u32 index_cfg_reg = 0;
	u32 index = 0;

	get_ip_index_from_table(dcam_dvfs_config_table, idle_freq, &index);
	index_cfg_reg = DVFS_REG_RD(REG_MM_DVFS_AHB_DCAM_IF_DVFS_INDEX_IDLE_CFG);
	index_cfg_reg = (index_cfg_reg & (~0x7)) | index;
	DVFS_REG_WR(REG_MM_DVFS_AHB_DCAM_IF_DVFS_INDEX_IDLE_CFG, index_cfg_reg);
	DVFS_TRACE("dvfs ops: %s, idle_freq=%lu, index=%d,\n",
			__func__, idle_freq, index);

	return 1;
}

/* get ip current volt ,clk & map index*/
static int get_ip_status(struct devfreq *devfreq,
	struct ip_dvfs_status *ip_status)
{
	u32 volt_reg;
	u32 clk_reg;

	volt_reg = DVFS_REG_RD(REG_MM_DVFS_AHB_MM_DVFS_VOLTAGE_DBG);
	clk_reg = DVFS_REG_RD(REG_MM_DVFS_AHB_MM_DVFS_CGM_CFG_DBG);
	ip_status->current_ip_clk = (clk_reg >>
		SHFT_BITS_CGM_DCAM_IF_SEL_DVFS) &
		MASK_BITS_CGM_DCAM_IF_SEL_DVFS;
	ip_status->current_sys_volt = ((volt_reg >>
		SHFT_BITS_DCAM_IF_VOLTAGE) & MASK_BITS_DCAM_IF_VOLTAGE);
	DVFS_TRACE("dvfs ops: %s\n", __func__);

	return 1;
}

/*coffe setting ops*/
static void  set_ip_gfree_wait_delay(unsigned int  wind_para)
{
	DVFS_REG_WR(REG_MM_DVFS_AHB_MM_GFREE_WAIT_DELAY_CFG0,
	BITS_DCAM_IF_GFREE_WAIT_DELAY(wind_para));
	DVFS_TRACE("dvfs ops: %s\n", __func__);
}

static void  set_ip_freq_upd_en_byp(unsigned int on)
{
	u32 reg_temp = 0;

	mutex_lock(&mmsys_glob_reg_lock);
	reg_temp = DVFS_REG_RD(REG_MM_DVFS_AHB_MM_FREQ_UPDATE_BYPASS);
	if (on)
		DVFS_REG_WR(REG_MM_DVFS_AHB_MM_FREQ_UPDATE_BYPASS,
			reg_temp |  BIT_REG_DCAM_IF_FREQ_UPD_EN_BYP);
	else
		DVFS_REG_WR(REG_MM_DVFS_AHB_MM_FREQ_UPDATE_BYPASS,
			reg_temp & (~BIT_REG_DCAM_IF_FREQ_UPD_EN_BYP));
	mutex_unlock(&mmsys_glob_reg_lock);
	DVFS_TRACE("dvfs ops: %s\n", __func__);
}

static void  set_ip_freq_upd_delay_en(unsigned int on)
{

	u32 reg_temp = 0;

	mutex_lock(&mmsys_glob_reg_lock);
	reg_temp = DVFS_REG_RD(REG_MM_DVFS_AHB_MM_FREQ_UPD_TYPE_CFG);
	if (on)
		DVFS_REG_WR(REG_MM_DVFS_AHB_MM_FREQ_UPD_TYPE_CFG,
			reg_temp | BIT_DCAM_IF_FREQ_UPD_DELAY_EN);
	else
		DVFS_REG_WR(REG_MM_DVFS_AHB_MM_FREQ_UPD_TYPE_CFG,
			reg_temp & (~(BIT_DCAM_IF_FREQ_UPD_DELAY_EN)));
	mutex_unlock(&mmsys_glob_reg_lock);
	DVFS_TRACE("dvfs ops: %s\n", __func__);
}

static void  set_ip_freq_upd_hdsk_en(unsigned int on)
{

	u32 reg_temp = 0;

	mutex_lock(&mmsys_glob_reg_lock);
	reg_temp = DVFS_REG_RD(REG_MM_DVFS_AHB_MM_FREQ_UPD_TYPE_CFG);
	if (on)
		DVFS_REG_WR(REG_MM_DVFS_AHB_MM_FREQ_UPD_TYPE_CFG,
			reg_temp | BIT_DCAM_IF_FREQ_UPD_HDSK_EN);
	else
		DVFS_REG_WR(REG_MM_DVFS_AHB_MM_FREQ_UPD_TYPE_CFG,
			reg_temp & (~(BIT_DCAM_IF_FREQ_UPD_HDSK_EN)));
	mutex_unlock(&mmsys_glob_reg_lock);
	DVFS_TRACE("dvfs ops: %s\n", __func__);

}

static void  set_ip_dvfs_swtrig_en(unsigned int en)
{
	u32 dfs_swtrig_en;

	mutex_lock(&mmsys_glob_reg_lock);
	dfs_swtrig_en = DVFS_REG_RD(REG_MM_DVFS_AHB_MM_DFS_SW_TRIG_CFG);
	if (en)
		DVFS_REG_WR(REG_MM_DVFS_AHB_MM_DFS_SW_TRIG_CFG,
			dfs_swtrig_en | BIT_DCAM_DFS_SW_TRIG);
	else
		DVFS_REG_WR(REG_MM_DVFS_AHB_MM_DFS_SW_TRIG_CFG,
			dfs_swtrig_en & (~BIT_DCAM_DFS_SW_TRIG));
	mutex_unlock(&mmsys_glob_reg_lock);
	DVFS_TRACE("dvfs ops: %s\n", __func__);
}

static void get_ip_dvfs_swtrig_en(unsigned int *en)
{
	u32 reg_temp = 0;

	mutex_lock(&mmsys_glob_reg_lock);
	reg_temp = DVFS_REG_RD(REG_MM_DVFS_AHB_MM_DFS_SW_TRIG_CFG);
	*en = (reg_temp >> SHIFT_BIT_DCAM_DFS_SW_TRIG) && MASK_DVFS_ONE_BIT;
	mutex_unlock(&mmsys_glob_reg_lock);
	DVFS_TRACE("dvfs ops: %s\n", __func__);
}

/*work-idle dvfs index ops*/
static void set_ip_dvfs_work_index(struct devfreq *devfreq,
	unsigned int index)
{
	u32 index_cfg_reg = 0;

	DVFS_TRACE("dvfs ops: %s, work index=%d\n", __func__, index);
	index_cfg_reg = DVFS_REG_RD(REG_MM_DVFS_AHB_DCAM_IF_DVFS_INDEX_CFG);
	index_cfg_reg = (index_cfg_reg & (~0x7)) | index;
	DVFS_REG_WR(REG_MM_DVFS_AHB_DCAM_IF_DVFS_INDEX_CFG, index_cfg_reg);
}
static void  set_ip_dvfs_idle_index(struct devfreq *devfreq, unsigned int index)
{
	u32 index_cfg_reg = 0;

	DVFS_TRACE("dvfs ops: %s, work index=%d\n", __func__, index);
	index_cfg_reg = DVFS_REG_RD(
		REG_MM_DVFS_AHB_DCAM_IF_DVFS_INDEX_IDLE_CFG);
	index_cfg_reg = (index_cfg_reg & (~0x7)) | index;
	DVFS_REG_WR(REG_MM_DVFS_AHB_DCAM_IF_DVFS_INDEX_IDLE_CFG, index_cfg_reg);
}
/*work-idle dvfs index ops*/
static void  get_ip_dvfs_work_index(struct devfreq *devfreq,
	unsigned int *index)
{
	*index = DVFS_REG_RD(REG_MM_DVFS_AHB_DCAM_IF_DVFS_INDEX_CFG);
	DVFS_TRACE("dvfs ops: %s, work_index=%d\n", __func__, *index);
}

static void  get_ip_dvfs_idle_index(struct devfreq *devfreq,
	unsigned int *index)
{
	*index = DVFS_REG_RD(REG_MM_DVFS_AHB_DCAM_IF_DVFS_INDEX_IDLE_CFG);
	DVFS_TRACE("dvfs ops: %s, work_index=%d\n", __func__, *index);
}

static void  set_ip_dfs_idle_disable(unsigned int on)
{
	u32 reg_temp = 0;

	mutex_lock(&mmsys_glob_reg_lock);
	reg_temp = DVFS_REG_RD(REG_MM_DVFS_AHB_MM_DFS_IDLE_DISABLE_CFG);
	if (on)
		DVFS_REG_WR(REG_MM_DVFS_AHB_MM_DFS_IDLE_DISABLE_CFG,
			reg_temp | BIT_DCAM_IF_DFS_IDLE_DISABLE);
	else
		DVFS_REG_WR(REG_MM_DVFS_AHB_MM_DFS_IDLE_DISABLE_CFG,
			reg_temp & (~(BIT_DCAM_IF_DFS_IDLE_DISABLE)));
	mutex_unlock(&mmsys_glob_reg_lock);
	DVFS_TRACE("dvfs ops: %s\n", __func__);
}

static void get_ip_dfs_idle_disable(unsigned int *on)
{
	u32 reg_value = 0;

	reg_value = DVFS_REG_RD(REG_MM_DVFS_AHB_MM_DFS_IDLE_DISABLE_CFG);
	*on = (reg_value >> 0) & BIT_DCAM_IF_DFS_IDLE_DISABLE;
}

static void dcam_dvfs_map_cfg(void)
{
	u32 map_cfg_reg = 0;
	int i = 0;

	for (i = 0; i < MAX_DVFS_INDEX; i++) {
		map_cfg_reg = DVFS_REG_RD(dcam_dvfs_config_table[i].reg_add);
		map_cfg_reg = (map_cfg_reg & (~0x1ffff)) |
		BITS_DCAM_IF_VOTE_AXI_INDEX0(
			dcam_dvfs_config_table[i].axi_index) |
		BITS_DCAM_IF_VOL_INDEX0(dcam_dvfs_config_table[i].volt) |
		BITS_CGM_DCAM_IF_FDIV_DENOM_INDEX0(
			dcam_dvfs_config_table[i].fdiv_denom) |
		BITS_CGM_DCAM_IF_FDIV_NUM_INDEX0(
			dcam_dvfs_config_table[i].fdiv_num) |
		BITS_CGM_DCAM_IF_SEL_INDEX0(dcam_dvfs_config_table[i].clk);
		DVFS_REG_WR(dcam_dvfs_config_table[i].reg_add, map_cfg_reg);
	}
}

static int get_ip_work_or_idle(unsigned int *is_work)
{
	unsigned int temp_status = 0;
	unsigned int temp_dfs_en = 0;

	get_ip_dfs_idle_disable(&temp_status);
	if (temp_status == 1) {
		DVFS_TRACE("Idle disable ,return work.\n");
		*is_work = 1;
		goto out;
	}
	temp_status = (DVFS_REG_RD_AHB(REG_MM_AHB_AHB_EB)
		>> SHIFT_BIT_MM_DCAM_EB) & MASK_DVFS_ONE_BIT;
	if (temp_status == 0) {
		DVFS_TRACE("Module not enable, return idle\n");
		*is_work = 0;
		goto out;
	}
	temp_dfs_en = (DVFS_REG_RD(REG_MM_DVFS_AHB_MM_DFS_EN_CTRL)
		>> SHIFT_BIT_DCAM_IF_DFS_EN) & MASK_DVFS_ONE_BIT;
	if (temp_dfs_en == 1) {
		get_ip_dvfs_swtrig_en(&temp_status);
		if (temp_status == 1) {
			DVFS_TRACE("SW trigger true, return work\n");
			*is_work = 1;
			goto out;
		} else {
			DVFS_TRACE("SW trigger false , return idle\n");
			*is_work = 0;
			goto out;
		}
	} else {
		temp_status = (DVFS_REG_RD_AHB(REG_MM_IP_BUSY)
			>> SHIFT_BIT_MM_DCAM_BUSY) & (MASK_DVFS_ONE_BIT);
		if (temp_status == 1) {
			DVFS_TRACE("Ip is work,return work.\n");
			*is_work = 1;
		} else {
			DVFS_TRACE("Ip is idle,return idle.\n");
			*is_work = 0;
		}
	}
out:
	return 0;
}

static int ip_dvfs_init(struct devfreq *devfreq)
{
	struct dcam_dvfs *dcam;

	DVFS_TRACE("dvfs ops: dcam %s\n", __func__);
	dcam = dev_get_drvdata(devfreq->dev.parent);
	if (!dcam) {
		pr_err("undefined dcam_dvfs\n");
		return 1;
	}
	dcam_dvfs_map_cfg();
	devfreq->max_freq = dcam_dvfs_config_table[7].clk_freq;
	devfreq->min_freq = dcam_dvfs_config_table[0].clk_freq;
	set_ip_freq_upd_en_byp(DCAM_IF_FREQ_UPD_EN_BYP);
	dcam->dcam_dvfs_para.ip_coffe.freq_upd_en_byp = DCAM_IF_FREQ_UPD_EN_BYP;
	set_ip_freq_upd_delay_en(DCAM_IF_FREQ_UPD_DELAY_EN);
	dcam->dcam_dvfs_para.ip_coffe.freq_upd_delay_en =
		DCAM_IF_FREQ_UPD_DELAY_EN;
	set_ip_freq_upd_hdsk_en(DCAM_IF_FREQ_UPD_HDSK_EN);
	dcam->dcam_dvfs_para.ip_coffe.freq_upd_hdsk_en =
		DCAM_IF_FREQ_UPD_HDSK_EN;
	set_ip_gfree_wait_delay(DCAM_IF_GFREE_WAIT_DELAY);
	dcam->dcam_dvfs_para.ip_coffe.gfree_wait_delay =
		DCAM_IF_GFREE_WAIT_DELAY;
	set_ip_dvfs_swtrig_en(DCAM_IF_SW_TRIG_EN);
	set_ip_dvfs_work_index(devfreq, DCAM_IF_WORK_INDEX_DEF);
	dcam->dcam_dvfs_para.ip_coffe.work_index_def = DCAM_IF_WORK_INDEX_DEF;
	dcam->dcam_dvfs_para.u_work_index = DCAM_IF_WORK_INDEX_DEF;
	dcam->dcam_dvfs_para.u_work_freq =
		dcam_dvfs_config_table[DCAM_IF_WORK_INDEX_DEF].clk_freq;
	set_ip_dvfs_idle_index(devfreq, DCAM_IF_IDLE_INDEX_DEF);
	dcam->dcam_dvfs_para.ip_coffe.idle_index_def = DCAM_IF_IDLE_INDEX_DEF;
	dcam->dcam_dvfs_para.u_idle_index = DCAM_IF_IDLE_INDEX_DEF;
	dcam->dcam_dvfs_para.u_idle_freq =
		dcam_dvfs_config_table[DCAM_IF_IDLE_INDEX_DEF].clk_freq;
	ip_hw_dvfs_en(devfreq, DCAM_IF_AUTO_TUNE);
	dcam->freq = dcam_dvfs_config_table[DCAM_IF_IDLE_INDEX_DEF].clk_freq;
	dcam->dvfs_enable = DVFS_ON;
	DVFS_TRACE("DCAM FREQ:%lu\n", dcam->freq);
	DVFS_TRACE("Max:%lu Min:%lu\n", devfreq->max_freq, devfreq->min_freq);

	return 1;
}

static int update_target_freq(struct devfreq *devfreq, unsigned long volt,
	unsigned long w_freq, unsigned long i_freq)
{
	set_work_freq(devfreq, w_freq);
	set_idle_freq(devfreq, i_freq);
	DVFS_TRACE("dvfs ops: %s\n", __func__);

	return 1;
}

/*debug interface */
static int set_fix_dvfs_value(struct devfreq *devfreq, unsigned long fix_volt)
{
	mmsys_set_fix_dvfs_value(fix_volt);
	DVFS_TRACE("dvfs ops: %s\n", __func__);
	return 1;
}

static void power_on_nb(struct devfreq *devfreq)
{
	ip_dvfs_init(devfreq);
	DVFS_TRACE("dvfs ops:dcam %s\n", __func__);
}

static void power_off_nb(struct devfreq *devfreq)
{
	struct dcam_dvfs *dcam;

	DVFS_TRACE("dvfs ops: isp %s\n", __func__);
	dcam = dev_get_drvdata(devfreq->dev.parent);
	if (!dcam) {
		pr_err("undefined isp_dvfs\n");
		return;
	}
	dcam->dvfs_enable = DVFS_OFF;
}

static int top_current_volt(struct devfreq *devfreq, unsigned int *top_volt)
{
	unsigned int ret;

	ret = top_mm_dvfs_current_volt(devfreq);
	*top_volt = ret;
	DVFS_TRACE("dvfs ops: %s\n", __func__);
	return 1;
}

static int mm_current_volt(struct devfreq *devfreq, unsigned int *mm_volt)
{
	unsigned int ret;

	ret = mm_dvfs_read_current_volt(devfreq);
	*mm_volt = ret;
	DVFS_TRACE("dvfs ops: %s\n", __func__);
	return 1;
}

struct ip_dvfs_ops  dcam_dvfs_ops  =  {

	.name = "DCAM_DVFS_OPS",
	.available = 1,

	.ip_dvfs_init = ip_dvfs_init,
	.ip_hw_dvfs_en = ip_hw_dvfs_en,
	.ip_auto_tune_en = ip_auto_tune_en,
	.set_work_freq = set_work_freq,
	.set_idle_freq = set_idle_freq,
	.get_ip_dvfs_table = get_ip_dvfs_table,
	.set_ip_dvfs_table = set_ip_dvfs_table,
	.get_ip_status = get_ip_status,
	.set_ip_dvfs_work_index = set_ip_dvfs_work_index,
	.get_ip_dvfs_work_index = get_ip_dvfs_work_index,
	.set_ip_dvfs_idle_index = set_ip_dvfs_idle_index,
	.get_ip_dvfs_idle_index = get_ip_dvfs_idle_index,

	.set_ip_gfree_wait_delay = set_ip_gfree_wait_delay,
	.set_ip_freq_upd_en_byp = set_ip_freq_upd_en_byp,
	.set_ip_freq_upd_delay_en = set_ip_freq_upd_delay_en,
	.set_ip_freq_upd_hdsk_en = set_ip_freq_upd_hdsk_en,
	.set_ip_dvfs_swtrig_en = set_ip_dvfs_swtrig_en,
	.set_ip_dfs_idle_disable = set_ip_dfs_idle_disable,
	.get_ip_work_or_idle = get_ip_work_or_idle,

	.set_fix_dvfs_value = set_fix_dvfs_value,
	.update_target_freq = update_target_freq,
	.power_on_nb = power_on_nb,
	.power_off_nb = power_off_nb,
	.top_current_volt = top_current_volt,
	.mm_current_volt = mm_current_volt,
	.event_handler = NULL,
};

