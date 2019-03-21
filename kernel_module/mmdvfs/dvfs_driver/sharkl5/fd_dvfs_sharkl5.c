/* #include "mm_dvfs_reg.h" */
#include "mmsys_dvfs_comm.h"
#include "mm_dvfs.h"
#include "fd_dvfs.h"
#include "sharkl5_mm_dvfs_coffe.h"



struct ip_dvfs_map_cfg  fd_dvfs_config_table[] =
{
	{0, REG_MM_DVFS_AHB_FD_INDEX0_MAP,	VOLT70,   FD_CLK768,	FD_CLK_INDEX_768,		0, 0, 0,	MTX_CLK_INDEX_768	},
	{1, REG_MM_DVFS_AHB_FD_INDEX1_MAP,	VOLT70,   FD_CLK1920,	FD_CLK_INDEX_1920,		0, 0, 0,	MTX_CLK_INDEX_1280	},
	{2, REG_MM_DVFS_AHB_FD_INDEX2_MAP,	VOLT70,   FD_CLK3072,	FD_CLK_INDEX_3072,		0, 0, 0,	MTX_CLK_INDEX_3072	},
	{3, REG_MM_DVFS_AHB_FD_INDEX3_MAP,	VOLT75,   FD_CLK3840,	FD_CLK_INDEX_3840,		0, 0, 0,	MTX_CLK_INDEX_3840	},
	{4, REG_MM_DVFS_AHB_FD_INDEX4_MAP,	VOLT75,   FD_CLK3840,	FD_CLK_INDEX_3840,		0, 0, 0,	MTX_CLK_INDEX_4680	},
	{5, REG_MM_DVFS_AHB_FD_INDEX5_MAP,	VOLT80,   FD_CLK3840,	FD_CLK_INDEX_3840,		0, 0, 0,	MTX_CLK_INDEX_4680	},
	{6, REG_MM_DVFS_AHB_FD_INDEX6_MAP,	VOLT80,   FD_CLK3840,	FD_CLK_INDEX_3840,		0, 0, 0,	MTX_CLK_INDEX_4680	},
	{7, REG_MM_DVFS_AHB_FD_INDEX7_MAP,	VOLT80,   FD_CLK3840,	FD_CLK_INDEX_3840,		0, 0, 0,	MTX_CLK_INDEX_4680	},

};

/* userspace  interface*/
static int  ip_hw_dvfs_en(struct devfreq *devfreq, unsigned int dvfs_eb)
{

	u32 dfs_en_reg;

	mutex_lock(&mmsys_glob_reg_lock);
	dfs_en_reg = DVFS_REG_RD(REG_MM_DVFS_AHB_MM_DFS_EN_CTRL);

	if (dvfs_eb)
		DVFS_REG_WR(REG_MM_DVFS_AHB_MM_DFS_EN_CTRL,
			dfs_en_reg | BIT_FD_DFS_EN);
	else
		DVFS_REG_WR(REG_MM_DVFS_AHB_MM_DFS_EN_CTRL,
			dfs_en_reg & (~BIT_FD_DFS_EN));
	mutex_unlock(&mmsys_glob_reg_lock);

	pr_info("dvfs ops: %s, dvfs_eb=%d\n", __func__, dvfs_eb);
	return 1;
}

static int ip_auto_tune_en(struct devfreq *devfreq, unsigned long dvfs_eb)
{
	pr_info("dvfs ops: %s\n", __func__);
	/* fd has no this funcation */
	return 1;
}

/*work-idle dvfs map  ops*/
static int  get_ip_dvfs_table(struct devfreq *devfreq,
		struct ip_dvfs_map_cfg *dvfs_table)
{
	int i = 0;

	pr_info("dvfs ops: %s\n", __func__);
	for(i = 0; i < 8; i++) {

		dvfs_table[i].map_index = fd_dvfs_config_table[i].map_index;
		dvfs_table[i].reg_add = fd_dvfs_config_table[i].reg_add;
		dvfs_table[i].volt = fd_dvfs_config_table[i].volt;
		dvfs_table[i].clk = fd_dvfs_config_table[i].clk;
		dvfs_table[i].fdiv_denom = fd_dvfs_config_table[i].fdiv_denom;
		dvfs_table[i].fdiv_num = fd_dvfs_config_table[i].fdiv_num;
		dvfs_table[i].axi_index = fd_dvfs_config_table[i].axi_index;
		dvfs_table[i].mtx_index = fd_dvfs_config_table[i].mtx_index;
	}

	return 1;
}

static int  set_ip_dvfs_table(struct devfreq *devfreq,
		struct ip_dvfs_map_cfg *dvfs_table)
{
	memcpy(fd_dvfs_config_table, dvfs_table, sizeof(struct
		ip_dvfs_map_cfg));
	pr_info("dvfs ops: %s\n", __func__);
	return 1;
}

static void   get_ip_index_from_table(struct ip_dvfs_map_cfg  *dvfs_cfg,
	unsigned long work_freq, unsigned int  *index)
{
	unsigned long  set_clk = 0;
	u32 i;



	*index = 0;
	pr_info("dvfs ops: %s,work_freq=%lu\n", __func__, work_freq);
	for (i = 0; i < 8; i++) {
		set_clk =  fd_dvfs_config_table[i].clk_freq;

		if (work_freq == set_clk||work_freq < set_clk) {
			*index = i;
			break;
		}
		if(i==7)
		*index = 7;
	}

	pr_info("dvfs ops: %s,index=%d\n", __func__, *index);

}

static int  set_work_freq(struct devfreq *devfreq, unsigned long work_freq)
{
	u32 index_cfg_reg = 0;
		u32 index = 0;

	get_ip_index_from_table(fd_dvfs_config_table,  work_freq, &index);

	index_cfg_reg = DVFS_REG_RD(REG_MM_DVFS_AHB_FD_DVFS_INDEX_CFG);
	index_cfg_reg = (index_cfg_reg & (~0x7)) | index;
	DVFS_REG_WR(REG_MM_DVFS_AHB_FD_DVFS_INDEX_CFG, index_cfg_reg);

	pr_info("dvfs ops: %s, work_freq=%lu, index=%d,\n",
		__func__, work_freq, index);
	return 1;
}

static int  set_idle_freq(struct devfreq *devfreq, unsigned long  idle_freq)
{
	u32 index_cfg_reg = 0;
	u32 index = 0;

		get_ip_index_from_table(fd_dvfs_config_table,  idle_freq, &
			index);

	index_cfg_reg = DVFS_REG_RD(REG_MM_DVFS_AHB_FD_DVFS_INDEX_CFG);
	index_cfg_reg = (index_cfg_reg & (~0x7)) | index;
	DVFS_REG_WR(REG_MM_DVFS_AHB_FD_DVFS_INDEX_CFG, index_cfg_reg);

	pr_info("dvfs ops: %s, work_freq=%lu, index=%d,\n",
		__func__, idle_freq, index);
	return 1;

}

/* get ip current volt ,clk & map index*/
static int  get_ip_status(struct devfreq *devfreq, struct ip_dvfs_status *
	ip_status)
{

	u32 volt_reg;
	u32 clk_reg;
	u32 i;
	volt_reg = DVFS_REG_RD(REG_MM_DVFS_AHB_MM_DVFS_VOLTAGE_DBG);
	clk_reg = DVFS_REG_RD(REG_MM_DVFS_AHB_MM_DVFS_CGM_CFG_DBG);

	ip_status->current_ip_clk = (clk_reg >>
		SHFT_BITS_CGM_FD_SEL_DVFS) &
		MASK_BITS_CGM_FD_SEL_DVFS;
	for (i = 0; i < 8; i++){
		if(ip_status->current_ip_clk==fd_dvfs_config_table[i].clk){
			ip_status->current_ip_clk=fd_dvfs_config_table[i].clk_freq;
		  break;
	  }
	}
	ip_status->current_sys_volt = ((volt_reg >>SHFT_BITS_FD_VOLTAGE)
				& MASK_BITS_FD_VOLTAGE);
	pr_info("dvfs ops: %s\n", __func__);
	return 1;
}


/*coffe setting ops*/
static void  set_ip_gfree_wait_delay(unsigned int  wind_para)
{

	DVFS_REG_WR(REG_MM_DVFS_AHB_MM_GFREE_WAIT_DELAY_CFG0,
	BITS_FD_GFREE_WAIT_DELAY(wind_para));

	pr_info("dvfs ops: %s\n", __func__);
}

static void  set_ip_freq_upd_en_byp(unsigned int on)
{

	u32 reg_temp = 0;

	mutex_lock(&mmsys_glob_reg_lock);
	reg_temp = DVFS_REG_RD(REG_MM_DVFS_AHB_MM_FREQ_UPDATE_BYPASS);

	if (on)
		DVFS_REG_WR(REG_MM_DVFS_AHB_MM_FREQ_UPDATE_BYPASS,
			reg_temp |  BIT_REG_FD_FREQ_UPD_EN_BYP);
	else
		DVFS_REG_WR(REG_MM_DVFS_AHB_MM_FREQ_UPDATE_BYPASS,
			reg_temp & (~BIT_REG_FD_FREQ_UPD_EN_BYP));
	mutex_unlock(&mmsys_glob_reg_lock);
	pr_info("dvfs ops: %s\n", __func__);
}

static void  set_ip_freq_upd_delay_en(unsigned int on)
{

	u32 reg_temp = 0;

	mutex_lock(&mmsys_glob_reg_lock);
	reg_temp = DVFS_REG_RD(REG_MM_DVFS_AHB_MM_FREQ_UPD_TYPE_CFG);
	if (on)
		DVFS_REG_WR(REG_MM_DVFS_AHB_MM_FREQ_UPD_TYPE_CFG,
			reg_temp | BIT_FD_FREQ_UPD_DELAY_EN);
	else
		DVFS_REG_WR(REG_MM_DVFS_AHB_MM_FREQ_UPD_TYPE_CFG,
			reg_temp & (~(BIT_FD_FREQ_UPD_DELAY_EN)));
	mutex_unlock(&mmsys_glob_reg_lock);

	pr_info("dvfs ops: %s\n", __func__);
}

static void  set_ip_freq_upd_hdsk_en(unsigned int on)
{

	u32 reg_temp = 0;

	mutex_lock(&mmsys_glob_reg_lock);
	reg_temp = DVFS_REG_RD(REG_MM_DVFS_AHB_MM_FREQ_UPD_TYPE_CFG);

	if (on)
		DVFS_REG_WR(REG_MM_DVFS_AHB_MM_FREQ_UPD_TYPE_CFG,
			reg_temp | BIT_FD_FREQ_UPD_HDSK_EN);
	else
		DVFS_REG_WR(REG_MM_DVFS_AHB_MM_FREQ_UPD_TYPE_CFG,
			reg_temp & (~(BIT_FD_FREQ_UPD_HDSK_EN)));
	mutex_unlock(&mmsys_glob_reg_lock);

	pr_info("dvfs ops: %s\n", __func__);

}

static void  set_ip_dvfs_swtrig_en(unsigned int en)
{
	u32 dfs_swtrig_en;

	mutex_lock(&mmsys_glob_reg_lock);

	dfs_swtrig_en = DVFS_REG_RD(REG_MM_DVFS_AHB_MM_DFS_SW_TRIG_CFG);
	if (en)
		DVFS_REG_WR(REG_MM_DVFS_AHB_MM_DFS_SW_TRIG_CFG,
			dfs_swtrig_en | BIT_FD_DFS_SW_TRIG); /* bit2 */
	else
		DVFS_REG_WR(REG_MM_DVFS_AHB_MM_DFS_SW_TRIG_CFG,
			dfs_swtrig_en & (~BIT_FD_DFS_SW_TRIG));
	mutex_unlock(&mmsys_glob_reg_lock);

	pr_info("dvfs ops: %s\n", __func__);
}

/*work-idle dvfs index ops*/
static void  set_ip_dvfs_work_index(struct devfreq *devfreq, unsigned int
	index)
{
	u32 index_cfg_reg = 0;

	pr_info("dvfs ops: %s, work index=%d\n", __func__, index);

	index_cfg_reg = DVFS_REG_RD(REG_MM_DVFS_AHB_FD_DVFS_INDEX_CFG);
	index_cfg_reg = (index_cfg_reg & (~0x7)) | index;
	DVFS_REG_WR(REG_MM_DVFS_AHB_FD_DVFS_INDEX_CFG, index_cfg_reg);
	pr_info("dvfs ops: %s, index_cfg=%d\n", __func__, index_cfg_reg);
}

/*work-idle dvfs index ops*/
static void  get_ip_dvfs_work_index(struct devfreq *devfreq, unsigned int *
	index)
{


	*index = DVFS_REG_RD(REG_MM_DVFS_AHB_FD_DVFS_INDEX_CFG);

	pr_info("dvfs ops: %s, work_index=%d\n", __func__, *index);

}

static void  set_ip_dvfs_idle_index(struct devfreq *devfreq, unsigned int
	index)
{
	u32 index_cfg_reg = 0;

	pr_info("dvfs ops: %s, work index=%d\n", __func__, index);

	index_cfg_reg = DVFS_REG_RD(REG_MM_DVFS_AHB_FD_DVFS_INDEX_IDLE_CFG);
	index_cfg_reg = (index_cfg_reg & (~0x7)) | index;
	DVFS_REG_WR(REG_MM_DVFS_AHB_FD_DVFS_INDEX_IDLE_CFG, index_cfg_reg);

}


	static void fd_dvfs_map_cfg(void)
{
	u32 map_cfg_reg = 0;
	u32 i = 0;

	for (i = 0; i < 8; i++) {
		map_cfg_reg = 0x0;
		map_cfg_reg = (map_cfg_reg & (~0x1ff)) |
		BITS_FD_VOL_INDEX0(fd_dvfs_config_table[i].volt) |
		BITS_FD_VOTE_MTX_INDEX0(fd_dvfs_config_table[i].mtx_index) |
		BITS_CGM_FD_SEL_INDEX0(fd_dvfs_config_table[i].clk);

		/* DVFS_REG_WR(REG_MM_DVFS_AHB_FD_INDEX0_MAP, map_cfg_reg); */
		DVFS_REG_WR((fd_dvfs_config_table[i].reg_add), map_cfg_reg);
	}
}


static int  ip_dvfs_init(struct devfreq *devfreq)
{

	struct fd_dvfs *fd;

	pr_info("dvfs ops: fd %s\n", __func__);

	fd = dev_get_drvdata(devfreq->dev.parent);

	if (!fd) {
		pr_info("undefined fd_dvfs\n");
		return 1;
	}


	pr_info("dvfs : fd %d\n", FD_FREQ_UPD_EN_BYP);
	pr_info("dvfs : fd %d\n", FD_FREQ_UPD_DELAY_EN);
	pr_info("dvfs : fd %d\n", FD_FREQ_UPD_HDSK_EN);
	pr_info("dvfs : fd %d\n", FD_GFREE_WAIT_DELAY);
	pr_info("dvfs : fd %d\n", FD_SW_TRIG_EN);
	pr_info("dvfs : fd %d\n", FD_WORK_INDEX_DEF);
	pr_info("dvfs : fd %d\n", FD_IDLE_INDEX_DEF);
	pr_info("dvfs : fd %d\n", FD_AUTO_TUNE);
	fd_dvfs_map_cfg();

	devfreq->max_freq = fd_dvfs_config_table[7].clk_freq;
	devfreq->min_freq = fd_dvfs_config_table[0].clk_freq;

	set_ip_freq_upd_en_byp(FD_FREQ_UPD_EN_BYP);
	set_ip_freq_upd_delay_en(FD_FREQ_UPD_DELAY_EN);
	set_ip_freq_upd_hdsk_en(FD_FREQ_UPD_HDSK_EN);
	set_ip_gfree_wait_delay(FD_GFREE_WAIT_DELAY);
	set_ip_dvfs_swtrig_en(FD_SW_TRIG_EN);
	set_ip_dvfs_work_index(devfreq,FD_WORK_INDEX_DEF);
	set_ip_dvfs_idle_index(devfreq,FD_IDLE_INDEX_DEF);
	ip_hw_dvfs_en(devfreq, FD_AUTO_TUNE);
	fd->dvfs_enable = FD_AUTO_TUNE;
    fd->freq = fd_dvfs_config_table[FD_WORK_INDEX_DEF].clk_freq;

	return 1;
}

static int updata_target_freq(struct devfreq *devfreq, unsigned long  volt,
	unsigned long freq, unsigned int eb_sysgov)
{

	pr_info("dvfs ops: %s\n", __func__);

	if (eb_sysgov == TRUE)
		mmsys_adjust_target_freq(DVFS_FD,  volt,     freq,  eb_sysgov);
	else
		set_work_freq(devfreq, freq);


	pr_info("dvfs ops: %s\n", __func__);
	return 1;
}

	/*debug interface */
static  int set_fix_dvfs_value(struct devfreq *devfreq, unsigned long  fix_volt)
{

	mmsys_set_fix_dvfs_value(fix_volt);

	pr_info("dvfs ops: %s\n,fix_volt=%lu", __func__, fix_volt);
	return 1;
}

static void power_on_nb(struct devfreq *devfreq)
{
	ip_dvfs_init(devfreq);

	pr_info("dvfs ops: fd %s\n", __func__);
}

static int top_current_volt(struct devfreq *devfreq, unsigned int *top_volt)
{
	unsigned int ret;

	ret = top_mm_dvfs_current_volt(devfreq);
	*top_volt = ret;
	pr_info("dvfs ops: %s\n", __func__);
	return 1;
}

static int mm_current_volt(struct devfreq *devfreq, unsigned int *mm_volt)
{
	unsigned int ret;

	ret = mm_dvfs_read_current_volt(devfreq);
	*mm_volt = ret;
	pr_info("dvfs ops: %s\n", __func__);
	return 1;
}

struct ip_dvfs_ops  fd_dvfs_ops  =  {

	.name = "FD_DVFS_OPS",
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
	.get_ip_work_index_from_table = get_ip_index_from_table,
	.get_ip_idle_index_from_table = get_ip_index_from_table,

	.set_ip_gfree_wait_delay = set_ip_gfree_wait_delay,
	.set_ip_freq_upd_en_byp = set_ip_freq_upd_en_byp,
	.set_ip_freq_upd_delay_en = set_ip_freq_upd_delay_en,
	.set_ip_freq_upd_hdsk_en = set_ip_freq_upd_hdsk_en,
	.set_ip_dvfs_swtrig_en = set_ip_dvfs_swtrig_en,

	.set_fix_dvfs_value = set_fix_dvfs_value,
	.updata_target_freq = updata_target_freq,
	.power_on_nb = power_on_nb,
	.top_current_volt = top_current_volt,
	.mm_current_volt = mm_current_volt,
	.event_handler = NULL,
};

