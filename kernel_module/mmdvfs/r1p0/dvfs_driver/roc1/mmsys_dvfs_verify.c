#include "mmsys_dvfs_comm.h"
#include "roc1_top_dvfs_reg.h"
#include "roc1_mm_dvfs_reg.h"
#include "mm_dvfs.h"

#define DEBUG_BUF_COUNT 600
#define DEBUG_VERIFY_COUNT 6

static unsigned int  reg_value[DEBUG_BUF_COUNT]={0};
static unsigned int  reg_buf_count=0;

static u8	verify_idx[8]={0}; 
static u32  ip_eb[8]={0}; 
static u32  work_idx[8]={0}; 
static u32  idle_idx[8]={0}; 
static u32  ip_status[8]={0}; 
static u32  ip_dvfs_en[8]={0}; 
static u32  ip_used_idx[8]={0}; 

struct ip_dvfs_map_cfg_v  {
	uint32_t  map_index;
	unsigned long  reg_add;
	uint32_t  volt;
	uint32_t  clk;
	uint32_t  fdiv_denom;
	uint32_t  fdiv_num;
	uint32_t  axi_index;
	uint32_t  mtx_index;
};

static struct ip_dvfs_map_cfg_v  mtx_dvfs_config_table[] =
{
	{	0, REG_MM_DVFS_AHB_MM_MTX_INDEX0_MAP,	VOLT70,   MTX_CLK_INDEX_768,	0, 0, 0,	MTX_CLK_INDEX_768	},
	{	1, REG_MM_DVFS_AHB_MM_MTX_INDEX1_MAP,	VOLT70,   MTX_CLK_INDEX_1280,	0, 0, 0,	MTX_CLK_INDEX_1280	},
	{	2, REG_MM_DVFS_AHB_MM_MTX_INDEX2_MAP,	VOLT70,   MTX_CLK_INDEX_2560,	0, 0, 0,	MTX_CLK_INDEX_2560	},
	{	3, REG_MM_DVFS_AHB_MM_MTX_INDEX3_MAP,	VOLT70,   MTX_CLK_INDEX_3072,	0, 0, 0,	MTX_CLK_INDEX_3072	},
	{	4, REG_MM_DVFS_AHB_MM_MTX_INDEX4_MAP,	VOLT75,   MTX_CLK_INDEX_3840,	0, 0, 0,	MTX_CLK_INDEX_3840	},
	{	5, REG_MM_DVFS_AHB_MM_MTX_INDEX5_MAP,	VOLT75,   MTX_CLK_INDEX_4680,	0, 0, 0,	MTX_CLK_INDEX_4680	},
	{	6, REG_MM_DVFS_AHB_MM_MTX_INDEX6_MAP,	VOLT80,   MTX_CLK_INDEX_5120,	0, 0, 0,	MTX_CLK_INDEX_4680	},
	{	7, REG_MM_DVFS_AHB_MM_MTX_INDEX7_MAP,	VOLT80,   MTX_CLK_INDEX_5120,	0, 0, 0,	MTX_CLK_INDEX_4680	},
};

static struct ip_dvfs_map_cfg_v  jpg_dvfs_config_table[] =
{
	{   0, REG_MM_DVFS_AHB_JPG_INDEX0_MAP,  VOLT70,   JPG_CLK768,   0, 0, 0,    MTX_CLK_INDEX_768    },
	{   1, REG_MM_DVFS_AHB_JPG_INDEX1_MAP,  VOLT70,   JPG_CLK1280,  0, 0, 0,    MTX_CLK_INDEX_1280   },
	{   2, REG_MM_DVFS_AHB_JPG_INDEX2_MAP,  VOLT70,   JPG_CLK2560,  0, 0, 0,    MTX_CLK_INDEX_2560   },
	{   3, REG_MM_DVFS_AHB_JPG_INDEX3_MAP,  VOLT75,   JPG_CLK3840,  0, 0, 0,    MTX_CLK_INDEX_3840   },
	{   4, REG_MM_DVFS_AHB_JPG_INDEX4_MAP,  VOLT75,   JPG_CLK3840,  0, 0, 0,    MTX_CLK_INDEX_4680   },
	{   5, REG_MM_DVFS_AHB_JPG_INDEX5_MAP,  VOLT70,   JPG_CLK3840,  0, 0, 0,    MTX_CLK_INDEX_5120  },
	{   6, REG_MM_DVFS_AHB_JPG_INDEX6_MAP,  VOLT80,   JPG_CLK3840,  0, 0, 0,    MTX_CLK_INDEX_5120  },
	{   7, REG_MM_DVFS_AHB_JPG_INDEX7_MAP,  VOLT80,   JPG_CLK3840,  0, 0, 0,    MTX_CLK_INDEX_5120   },

};

static struct ip_dvfs_map_cfg_v  cpp_dvfs_config_table[] =
{
	{	0, REG_MM_DVFS_AHB_CPP_INDEX0_MAP,	VOLT70,   CPP_CLK768,	0, 0, 0,	MTX_CLK_INDEX_768	},
	{	1, REG_MM_DVFS_AHB_CPP_INDEX1_MAP,	VOLT70,   CPP_CLK1280,	0, 0, 0,	MTX_CLK_INDEX_1280	},
	{	2, REG_MM_DVFS_AHB_CPP_INDEX2_MAP,	VOLT70,   CPP_CLK2560,	0, 0, 0,	MTX_CLK_INDEX_2560	},
	{	3, REG_MM_DVFS_AHB_CPP_INDEX3_MAP,	VOLT75,   CPP_CLK3840,	0, 0, 0,	MTX_CLK_INDEX_3840	},
	{	4, REG_MM_DVFS_AHB_CPP_INDEX4_MAP,	VOLT75,   CPP_CLK3840,	0, 0, 0,	MTX_CLK_INDEX_4680	},
	{	5, REG_MM_DVFS_AHB_CPP_INDEX5_MAP,	VOLT80,   CPP_CLK3840,	0, 0, 0,	MTX_CLK_INDEX_5120	},
	{	6, REG_MM_DVFS_AHB_CPP_INDEX6_MAP,	VOLT80,   CPP_CLK3840,	0, 0, 0,	MTX_CLK_INDEX_5120	},
	{	7, REG_MM_DVFS_AHB_CPP_INDEX7_MAP,	VOLT80,   CPP_CLK3840,	0, 0, 0,	MTX_CLK_INDEX_5120	},
};

static struct ip_dvfs_map_cfg_v  fd_dvfs_config_table[] =
{
	{0, REG_MM_DVFS_AHB_FD_INDEX0_MAP,	VOLT70,   FD_CLK768,	0, 0, 0,	MTX_CLK_INDEX_768	},
	{1, REG_MM_DVFS_AHB_FD_INDEX1_MAP,	VOLT70,   FD_CLK1920,	0, 0, 0,	MTX_CLK_INDEX_1280	},
	{2, REG_MM_DVFS_AHB_FD_INDEX2_MAP,	VOLT70,   FD_CLK3072,	0, 0, 0,	MTX_CLK_INDEX_2560	},
	{3, REG_MM_DVFS_AHB_FD_INDEX3_MAP,	VOLT75,   FD_CLK3840,	0, 0, 0,	MTX_CLK_INDEX_3840	},
	{4, REG_MM_DVFS_AHB_FD_INDEX4_MAP,	VOLT75,   FD_CLK3840,	0, 0, 0,	MTX_CLK_INDEX_4680	},
	{5, REG_MM_DVFS_AHB_FD_INDEX5_MAP,	VOLT80,   FD_CLK3840,	0, 0, 0,	MTX_CLK_INDEX_5120	},
	{6, REG_MM_DVFS_AHB_FD_INDEX6_MAP,	VOLT80,   FD_CLK3840,	0, 0, 0,	MTX_CLK_INDEX_5120	},
	{7, REG_MM_DVFS_AHB_FD_INDEX7_MAP,	VOLT80,   FD_CLK3840,	0, 0, 0,	MTX_CLK_INDEX_5120	},
};

static struct ip_dvfs_map_cfg_v  isp_dvfs_config_table[] =
{
	{0, REG_MM_DVFS_AHB_ISP_INDEX0_MAP,	VOLT70,   ISP_CLK2560,  0, 0, 0,	MTX_CLK_INDEX_2560},
	{1, REG_MM_DVFS_AHB_ISP_INDEX1_MAP,	VOLT70,   ISP_CLK3072,  0, 0, 0,	MTX_CLK_INDEX_3072},
	{2, REG_MM_DVFS_AHB_ISP_INDEX2_MAP,	VOLT75,   ISP_CLK3840,  0, 0, 0,	MTX_CLK_INDEX_3840},
	{3, REG_MM_DVFS_AHB_ISP_INDEX3_MAP,	VOLT75,   ISP_CLK4680,  0, 0, 0,	MTX_CLK_INDEX_4680},
	{4, REG_MM_DVFS_AHB_ISP_INDEX4_MAP,	VOLT80,   ISP_CLK5120,  0, 0, 0,	MTX_CLK_INDEX_5120},
	{5, REG_MM_DVFS_AHB_ISP_INDEX5_MAP,	VOLT80,   ISP_CLK5120,  0, 0, 0,	MTX_CLK_INDEX_5120},
	{6, REG_MM_DVFS_AHB_ISP_INDEX6_MAP,	VOLT80,   ISP_CLK5120,  0, 0, 0,	MTX_CLK_INDEX_5120},
	{7, REG_MM_DVFS_AHB_ISP_INDEX7_MAP,	VOLT80,   ISP_CLK5120,  0, 0, 0,	MTX_CLK_INDEX_5120},
};

static struct ip_dvfs_map_cfg_v  dcam_dvfs_config_table[] =
{
	{0, REG_MM_DVFS_AHB_DCAM_IF_INDEX0_MAP,	VOLT70,   DCAM_CLK1920,	 0, 0,	DCAMAXI_CLK_INDEX_2560, 0	},
	{1, REG_MM_DVFS_AHB_DCAM_IF_INDEX1_MAP,	VOLT70,   DCAM_CLK1920,	 0, 0, 	DCAMAXI_CLK_INDEX_2560, 0	},
	{2, REG_MM_DVFS_AHB_DCAM_IF_INDEX2_MAP,	VOLT70,   DCAM_CLK2560,	 0, 0, 	DCAMAXI_CLK_INDEX_3072, 0	},
	{3, REG_MM_DVFS_AHB_DCAM_IF_INDEX3_MAP,	VOLT70,   DCAM_CLK2560,	 0, 0,  DCAMAXI_CLK_INDEX_3072, 0	},
	{4, REG_MM_DVFS_AHB_DCAM_IF_INDEX4_MAP,	VOLT70,   DCAM_CLK3072,	 0, 0, 	DCAMAXI_CLK_INDEX_3072, 0	},
	{5, REG_MM_DVFS_AHB_DCAM_IF_INDEX5_MAP,	VOLT70,   DCAM_CLK3072,	 0, 0, 	DCAMAXI_CLK_INDEX_3072, 0	},
	{6, REG_MM_DVFS_AHB_DCAM_IF_INDEX6_MAP,	VOLT75,   DCAM_CLK3840,	 0, 0,	DCAMAXI_CLK_INDEX_3840, 0	},
	{7, REG_MM_DVFS_AHB_DCAM_IF_INDEX7_MAP,	VOLT80,   DCAM_CLK3840,	 0, 0, 	DCAMAXI_CLK_INDEX_4680, 0	},
};


static struct ip_dvfs_map_cfg_v  dcamaxi_dvfs_config_table[] =
{
	{	0, REG_MM_DVFS_AHB_DCAM_AXI_INDEX0_MAP,	VOLT70,   DCAMAXI_CLK2560,	 0, 0, 0,	0	},
	{	1, REG_MM_DVFS_AHB_DCAM_AXI_INDEX1_MAP,	VOLT70,   DCAMAXI_CLK3072,	 0, 0, 0,	0	},
	{	2, REG_MM_DVFS_AHB_DCAM_AXI_INDEX2_MAP,	VOLT75,   DCAMAXI_CLK3072,	 0, 0, 0,	0	},
	{	3, REG_MM_DVFS_AHB_DCAM_AXI_INDEX3_MAP,	VOLT80,   DCAMAXI_CLK3840,	 0, 0, 0,	0	},
	{	4, REG_MM_DVFS_AHB_DCAM_AXI_INDEX4_MAP,	VOLT80,   DCAMAXI_CLK4680,	 0, 0, 0,	0	},
	{	5, REG_MM_DVFS_AHB_DCAM_AXI_INDEX5_MAP,	VOLT80,   DCAMAXI_CLK4680,	 0, 0, 0,	0	},
	{	6, REG_MM_DVFS_AHB_DCAM_AXI_INDEX6_MAP,	VOLT80,   DCAMAXI_CLK4680,	 0, 0, 0,	0	},
	{	7, REG_MM_DVFS_AHB_DCAM_AXI_INDEX7_MAP,	VOLT80,   DCAMAXI_CLK4680,	 0, 0, 0,	0	},
};

void  get_cfg_work_idx(void)
{
	pr_info("dvfs ops: %s, get work index\n", __func__);

	work_idx[DVFS_DCAM] = DVFS_REG_RD(REG_MM_DVFS_AHB_DCAM_IF_DVFS_INDEX_CFG);
	pr_info("dvfs : dcam work index=%d\n",  work_idx[DVFS_DCAM]  );
	
	work_idx[DVFS_CPP] = DVFS_REG_RD(REG_MM_DVFS_AHB_CPP_DVFS_INDEX_CFG);
	pr_info("dvfs : cpp work index=%d\n",  work_idx[DVFS_CPP]  );

	work_idx[DVFS_ISP] = DVFS_REG_RD(REG_MM_DVFS_AHB_ISP_DVFS_INDEX_CFG);
	pr_info("dvfs : isp work index=%d\n",  work_idx[DVFS_ISP]  );

	work_idx[DVFS_FD] = DVFS_REG_RD(REG_MM_DVFS_AHB_FD_DVFS_INDEX_CFG);
	pr_info("dvfs : fd work index=%d\n",  work_idx[DVFS_FD]  );

	work_idx[DVFS_JPEG] = DVFS_REG_RD(REG_MM_DVFS_AHB_JPG_DVFS_INDEX_CFG);
	pr_info("dvfs : jpg work index=%d\n",  work_idx[DVFS_JPEG]  );

	work_idx[DVFS_DCAMAXI] = DVFS_REG_RD(REG_MM_DVFS_AHB_DCAM_AXI_DVFS_INDEX_CFG);
	pr_info("dvfs : axi work index=%d\n",  work_idx[DVFS_DCAM]  );

	work_idx[DVFS_MTX] = DVFS_REG_RD(REG_MM_DVFS_AHB_MM_MTX_DVFS_INDEX_CFG);
	pr_info("dvfs : mtx work index=%d\n",  work_idx[DVFS_MTX ] );

}


void  get_cfg_idle_idx(void)
{
	pr_info("dvfs ops: %s\n", __func__);

	idle_idx[DVFS_DCAM] = DVFS_REG_RD(REG_MM_DVFS_AHB_DCAM_IF_DVFS_INDEX_IDLE_CFG);
	pr_info("dvfs : dcam idle_idx=%d\n",  idle_idx[DVFS_DCAM]  );

	idle_idx[DVFS_CPP] = DVFS_REG_RD(REG_MM_DVFS_AHB_CPP_DVFS_INDEX_IDLE_CFG);
	pr_info("dvfs : cpp idle_idx=%d\n",  idle_idx[DVFS_CPP]  );

	idle_idx[DVFS_ISP] = DVFS_REG_RD(REG_MM_DVFS_AHB_ISP_DVFS_INDEX_IDLE_CFG);
	pr_info("dvfs : isp idle_idx=%d\n",  idle_idx[DVFS_ISP]  );

	idle_idx[DVFS_FD] = DVFS_REG_RD(REG_MM_DVFS_AHB_FD_DVFS_INDEX_IDLE_CFG);
	pr_info("dvfs : fd idle_idx=%d\n",  idle_idx[DVFS_FD]  );

	idle_idx[DVFS_JPEG] = DVFS_REG_RD(REG_MM_DVFS_AHB_JPG_DVFS_INDEX_IDLE_CFG);
	pr_info("dvfs : jpg idle_idx=%d\n",  idle_idx[DVFS_JPEG]  );
	
	idle_idx[DVFS_DCAMAXI] = DVFS_REG_RD(REG_MM_DVFS_AHB_DCAM_AXI_DVFS_INDEX_IDLE_CFG);
	pr_info("dvfs : axi idle_idx=%d\n",  idle_idx[DVFS_DCAM]  );

	idle_idx[DVFS_MTX] = DVFS_REG_RD(REG_MM_DVFS_AHB_MM_MTX_DVFS_INDEX_IDLE_CFG);
	pr_info("dvfs : mtx idle_idx=%d\n",  idle_idx[DVFS_MTX ] );

}


 void get_ip_status( u32 reg_value)
{
	printk("dvfs ops : get_ip_status 0x%x\n", reg_value);

	ip_status[DVFS_DCAM] =(reg_value & 0x8)  >> (3);
	printk("dvfs: get dcam status: 0x%x\n", ip_status[DVFS_DCAM]);

	ip_status[DVFS_CPP] = (reg_value & 0x1)  >> (0);
	printk("dvfs: get cpp status: 0x%x\n", ip_status[DVFS_CPP]);

	ip_status[DVFS_ISP] = (reg_value & 0x4)  >> (2);
	printk("dvfs: get isp status: 0x%x\n", ip_status[DVFS_ISP]);
	
	ip_status[DVFS_FD] = (reg_value & 0x10)  >> (4);
	printk("dvfs: get fd status: 0x%x\n", ip_status[DVFS_FD]);

	ip_status[DVFS_JPEG] =(reg_value & 0x2)  >> (1);
	printk("dvfs: get jpg status: 0x%x\n", ip_status[DVFS_JPEG]);

}

 void get_ip_eb( u32 reg_value)
{
	printk("dvfs ops: get_ip_eb 0x%x\n", reg_value);
	
	ip_eb[DVFS_DCAM] =(reg_value & 0x4)  >> (2);
	printk("dvfs:  dcam eb: 0x%x\n", ip_eb[DVFS_DCAM]);
	
	ip_eb[DVFS_CPP] = (reg_value & 0x1)  >> (0);
	printk("dvfs:  cpp eb: 0x%x\n", ip_eb[DVFS_CPP]);

	ip_eb[DVFS_ISP] = (reg_value & 0x8)  >> (3);
	printk("dvfs:  isp eb: 0x%x\n", ip_eb[DVFS_ISP]);

	ip_eb[DVFS_FD] = (reg_value & 0x40)  >> (10);
	printk("dvfs:  fd eb: 0x%x\n", ip_eb[DVFS_FD]);
	
	ip_eb[DVFS_JPEG] =(reg_value & 0x2)  >> (1);
	printk("dvfs:  jpg eb: 0x%x\n", ip_eb[DVFS_JPEG]);

}

void get_ip_dvfs_eb( u32 reg_value)
{
	printk("dvfs ops: get_ip_dvfs_eb 0x%x\n", reg_value);
	
	ip_dvfs_en[DVFS_DCAM] =(reg_value & 0x20)  >> (5);
	printk("dvfs:  dcam dvfs_eb: 0x%x\n", ip_dvfs_en[DVFS_DCAM]);
	
	ip_dvfs_en[DVFS_CPP] = (reg_value & 0x4)  >> (2);
	printk("dvfs:  cpp dvfs_eb: 0x%x\n", ip_dvfs_en[DVFS_CPP]);

	ip_dvfs_en[DVFS_ISP] = (reg_value & 0x2)  >> (1);
	printk("dvfs:  isp dvfs_eb: 0x%x\n", ip_dvfs_en[DVFS_ISP]);

	ip_dvfs_en[DVFS_FD] = (reg_value & 0x40)  >> (6);
	printk("dvfs:  fd dvfs_eb: 0x%x\n", ip_dvfs_en[DVFS_FD]);
	
	ip_dvfs_en[DVFS_JPEG] =(reg_value & 0x8)  >> (3);
	printk("dvfs:  jpg dvfs_eb: 0x%x\n", ip_dvfs_en[DVFS_JPEG]);

	ip_dvfs_en[DVFS_DCAMAXI] = (reg_value & 0x80)  >> (7);
	printk("dvfs:  axi dvfs_eb: 0x%x\n", ip_dvfs_en[DVFS_DCAMAXI]);
	
	ip_dvfs_en[DVFS_MTX] =(reg_value & 0x1)  >> (0);
	printk("dvfs:  mtx dvfs_eb: 0x%x\n", ip_dvfs_en[DVFS_MTX]);

}

void get_ip_ver_idx( void)
{
	u8   idx=0;

	printk("dvfs ops: get_ip_ver_idx \n");

          if(ip_eb[DVFS_DCAM] == 0)
		ip_used_idx[DVFS_DCAM]=idle_idx[DVFS_DCAM];
	else if( ip_status[DVFS_DCAM] == 1  || ip_dvfs_en[DVFS_DCAM] == 0 )
		ip_used_idx[DVFS_DCAM]=work_idx[DVFS_DCAM];
	else 
		ip_used_idx[DVFS_DCAM]=idle_idx[DVFS_DCAM];

	 if(ip_eb[DVFS_DCAM] == 0)
		ip_used_idx[DVFS_DCAMAXI] =idle_idx[DVFS_DCAMAXI];
	else if( ip_dvfs_en[DVFS_DCAMAXI] == 0   )
		ip_used_idx[DVFS_DCAMAXI] =work_idx[DVFS_DCAMAXI];
	else
	{
		idx = ip_used_idx[DVFS_DCAM];
		ip_used_idx[DVFS_DCAMAXI]  = dcam_dvfs_config_table[idx].axi_index;	
	}
	printk("dvfs: dcamaxi ver index: 0x%x\n", ip_used_idx[DVFS_DCAMAXI]);

	if(ip_eb[DVFS_CPP] == 0)
		ip_used_idx[DVFS_CPP] =idle_idx[DVFS_CPP];
	else if( ip_status[DVFS_CPP] == 1  || ip_dvfs_en[DVFS_CPP] == 0 )
		ip_used_idx[DVFS_CPP]=work_idx[DVFS_CPP];
	else 
		ip_used_idx[DVFS_CPP]=idle_idx[DVFS_CPP];

	if(ip_eb[DVFS_ISP] == 0)
		ip_used_idx[DVFS_ISP] =idle_idx[DVFS_ISP];		
	else if( ip_status[DVFS_ISP] == 1  || ip_dvfs_en[DVFS_ISP] == 0 )
		ip_used_idx[DVFS_ISP]=work_idx[DVFS_ISP];
	else 
		ip_used_idx[DVFS_ISP]=idle_idx[DVFS_ISP];

	if(ip_eb[DVFS_JPEG] == 0)
		ip_used_idx[DVFS_JPEG] =idle_idx[DVFS_JPEG];	
	else if( ip_status[DVFS_JPEG] == 1  || ip_dvfs_en[DVFS_JPEG] == 0 )
		ip_used_idx[DVFS_JPEG]=work_idx[DVFS_JPEG];
	else 
		ip_used_idx[DVFS_JPEG]=idle_idx[DVFS_JPEG];
	
	if(ip_eb[DVFS_FD] == 0)
		ip_used_idx[DVFS_FD] =idle_idx[DVFS_FD];
	else if( ip_status[DVFS_FD] == 1  || ip_dvfs_en[DVFS_FD] == 0 )
		ip_used_idx[DVFS_FD] =work_idx[DVFS_FD];
	else 
		ip_used_idx[DVFS_FD] =idle_idx[DVFS_FD];
}

void read_regto_buffer(void)
{
	if(reg_buf_count  <= DEBUG_BUF_COUNT -6 )
	{
		reg_value[reg_buf_count++]= DVFS_REG_RD_AHB(REG_MM_IP_BUSY);
		reg_value[reg_buf_count++]= DVFS_REG_RD(REG_MM_DVFS_AHB_MM_DVFS_VOLTAGE_DBG);
		reg_value[reg_buf_count++]= DVFS_REG_RD(REG_MM_DVFS_AHB_MM_DVFS_CGM_CFG_DBG);
		reg_value[reg_buf_count++]= DVFS_REG_RD_ABS(REG_TOP_DVFS_APB_DCDC_MM_DVFS_STATE_DBG);
		reg_value[reg_buf_count++]= DVFS_REG_RD(REG_MM_DVFS_AHB_MM_DFS_EN_CTRL);
		reg_value[reg_buf_count++]= DVFS_REG_RD_AHB(REG_MM_AHB_AHB_EB);
	}
}

u8   mmsys_case_verify_value(u8 case_idx, u8* index, mmsys_module m_type , uint32_t* reg_value)
{	
	u8   i, ret = 0 ,idx=0;
	u8   mtx_index;
	u8   set_volt[DVFS_MODULE_MAX]={0}, set_clk[DVFS_MODULE_MAX]={0};
	u8   request_volt[DVFS_MODULE_MAX]={0}; 
	u8   mm_volt_result = 0; 
	u8   top_volt_result=0;
	u8   result_clk[DVFS_MODULE_MAX] = {0};
	u8   mtx_cpp,mtx_isp,mtx_jpg, mtx_fd;
	uint32_t  verify_result=0;

   	//msleep(3);
	get_cfg_work_idx();
	get_cfg_idle_idx( );
	get_ip_status( reg_value[0]);
	get_ip_dvfs_eb( reg_value[4]);	 
	get_ip_eb( reg_value[5]);	 
	get_ip_ver_idx();
	
       /* request volt */
	request_volt[DVFS_DCAM]     =   (u8)(((reg_value[1]) >> SHFT_BITS_DCAM_IF_VOLTAGE) & MASK_BITS_DCAM_IF_VOLTAGE);
	request_volt[DVFS_CPP]        =   (u8)((reg_value[1] >> SHFT_BITS_CPP_VOLTAGE) & MASK_BITS_CPP_VOLTAGE);
	request_volt[DVFS_ISP]         =   (u8)((reg_value[1] >> SHFT_BITS_ISP_VOLTAGE) & MASK_BITS_ISP_VOLTAGE);
	request_volt[DVFS_FD]         =  (u8)((reg_value[1] >> SHFT_BITS_FD_VOLTAGE) & MASK_BITS_FD_VOLTAGE);
	request_volt[DVFS_MTX]        =   (u8)((reg_value[1] >> SHFT_BITS_MM_MTX_VOLTAGE) & MASK_BITS_MM_MTX_VOLTAGE);
	request_volt[DVFS_DCAMAXI] =   (u8)((reg_value[1] >> SHFT_BITS_DCAM_AXI_VOLTAGE) & MASK_BITS_DCAM_AXI_VOLTAGE);
	request_volt[DVFS_JPEG]        = (u8)((reg_value[1] >> SHFT_BITS_JPG_VOLTAGE) & MASK_BITS_JPG_VOLTAGE);

	/* result clk */
	result_clk[DVFS_DCAM]        =  (u8)((reg_value[2] >> SHFT_BITS_CGM_DCAM_IF_SEL_DVFS) & MASK_BITS_CGM_DCAM_IF_SEL_DVFS);
	result_clk[DVFS_CPP]        =  (u8)((reg_value[2] >> SHFT_BITS_CGM_CPP_SEL_DVFS) & MASK_BITS_CGM_CPP_SEL_DVFS);
	result_clk[DVFS_ISP]         =  (u8)((reg_value[2] >> SHFT_BITS_CGM_ISP_SEL_DVFS) & MASK_BITS_CGM_ISP_SEL_DVFS);          
	result_clk[DVFS_FD]         =   (u8)((reg_value[2] >> SHFT_BITS_CGM_FD_SEL_DVFS) & MASK_BITS_CGM_FD_SEL_DVFS);
	result_clk[DVFS_MTX]        =  (u8)((reg_value[2] >> SHFT_BITS_CGM_MM_MTX_SEL_DVFS) & MASK_BITS_CGM_MM_MTX_SEL_DVFS);
	result_clk[DVFS_DCAMAXI] =  (u8)((reg_value[2] >> SHFT_BITS_CGM_DCAM_AXI_SEL_DVFS) & MASK_BITS_CGM_DCAM_AXI_SEL_DVFS);
	result_clk[DVFS_JPEG]        = (u8)((reg_value[2] >> SHFT_BITS_CGM_JPG_SEL_DVFS) & MASK_BITS_CGM_JPG_SEL_DVFS);

	mm_volt_result = (u8)((reg_value[1] & 0x7000000) >> 24);
	top_volt_result=  (u8)((reg_value[3] & 0x700000) >> 20);

	 /* get set para */  	
	 idx = ip_used_idx[DVFS_DCAM];
	set_volt[DVFS_DCAM] =  dcam_dvfs_config_table[idx].volt;
	set_clk[DVFS_DCAM] =   dcam_dvfs_config_table[idx].clk;

	idx=	ip_used_idx[DVFS_DCAMAXI] ;
	set_volt[DVFS_DCAMAXI] =  dcamaxi_dvfs_config_table[idx].volt;
	set_clk[DVFS_DCAMAXI] =   dcamaxi_dvfs_config_table[idx].clk;

	 idx =ip_used_idx[DVFS_CPP];
	set_volt[DVFS_CPP] =  cpp_dvfs_config_table[idx].volt;
	set_clk[DVFS_CPP] =   cpp_dvfs_config_table[idx].clk;
	mtx_cpp=                  cpp_dvfs_config_table[idx].mtx_index;

	 idx =ip_used_idx[DVFS_ISP];
	set_volt[DVFS_ISP] =  isp_dvfs_config_table[idx].volt;
	set_clk[DVFS_ISP] =   isp_dvfs_config_table[idx].clk;
	mtx_isp=                  isp_dvfs_config_table[idx].mtx_index;

	idx = ip_used_idx[DVFS_JPEG]  ;
	set_volt[DVFS_JPEG] =  jpg_dvfs_config_table[idx].volt;
	set_clk[DVFS_JPEG] =   jpg_dvfs_config_table[idx].clk;
	mtx_jpg=                  jpg_dvfs_config_table[idx].mtx_index;

	idx = ip_used_idx[DVFS_FD]  ;
	set_volt[DVFS_FD] =  fd_dvfs_config_table[idx].volt;
	set_clk[DVFS_FD] =   fd_dvfs_config_table[idx].clk;
	mtx_fd=                  fd_dvfs_config_table[idx].mtx_index;

 //sel max mtx index
	if( ip_dvfs_en[DVFS_MTX] == 1 )
	{ 
	idx= mtx_cpp;
	if(mtx_isp >= mtx_cpp)
           idx= mtx_isp;

	if(idx < mtx_jpg)
           idx= mtx_jpg;

      if(idx < mtx_fd)
           idx= mtx_fd;
	} else {
		idx=work_idx[DVFS_MTX];
	}
			
	set_volt[DVFS_MTX] =  mtx_dvfs_config_table[idx].volt;
	set_clk[DVFS_MTX] =   mtx_dvfs_config_table[idx].clk;
	ip_used_idx[DVFS_MTX] = idx;
	mtx_index=idx;

	for(i=DVFS_DCAM; i<=DVFS_JPEG; i++)
       {
		idx=ip_used_idx[i];
			  
		if ( top_volt_result>=mm_volt_result && mm_volt_result >= set_volt[i] && result_clk[i] == set_clk[i]) {
			ret=1;
			verify_result++;
		} else {
			ret=0;  
		}
		
	       pr_err("[dvfs--%d-%d] result-%d= %d:  map_idx=%d,  top_volt_result=%d,  result_clk=%d,set_volt=%d, set_clk=%d,volt={mm=%d,gpu=%d,audio=%d}\n", 
				 	case_idx,
				 	m_type,
				 	i,
				 	ret,
				 	idx,
				 	top_volt_result,
					result_clk[i], 
				 	set_volt[i], 
				 	set_clk[i],
			             mm_volt_result,0 ,0
		   );
       }

       for(i=DVFS_DCAMAXI; i<=DVFS_MTX; i++)
       { 
		idx=ip_used_idx[i];
			
		if (mm_volt_result >= set_volt[i] && result_clk[i] == set_clk[i]) {
			ret=1;
			verify_result++;
		} else {
			ret=0;  
		}
		
		if(i==DVFS_MTX)
				pr_err("[dvfs--%d-%d] result-%d= %d:  map_idx=%d,  top_volt_result=%d,  result_clk=%d,set_volt=%d, set_clk=%d, mtx={cpp=%d,jpg=%d,isp=%d,fd=%d}\n", 
				 	case_idx,
				 	m_type,
				 	i,
				 	ret,
				 	idx,
				 	top_volt_result, 
					result_clk[i], 
				 	set_volt[i], 
				 	set_clk[i],
				 	mtx_cpp,
				 	mtx_jpg,
				 	mtx_isp,
				 	mtx_fd);
		else
			pr_err("[dvfs--%d-%d] result-%d= %d:  map_idx=%d,  top_volt_result=%d,  result_clk=%d,set_volt=%d, set_clk=%d, axi= %d\n", 
				 	case_idx,
				 	m_type,
				 	i,
				 	ret,
				 	idx,
				 	top_volt_result, 
					result_clk[i], 
				 	set_volt[i], 
				 	set_clk[i],
				 	idx);
       }	   

	if(verify_result==DVFS_MODULE_MAX)
       {
		pr_err("[dvfs---%d] ### Test PASS ### : result= %d\n", case_idx,verify_result );
	    ret=1;	   
       }   
	else
	{
	    ret=0;
		pr_err("[dvfs---%d] ### Test FAIL ### : result= %d\n", case_idx,verify_result );
	}
		
	return ret;

}

 void  mmsys_case_verify(u8 case_idx, u8* index, mmsys_module m_type)
 {
     int j =0 ;
 
     for(j=0; j<=reg_buf_count/DEBUG_VERIFY_COUNT; j++)
{
         mmsys_case_verify_value(1, verify_idx, m_type , &reg_value[j*DEBUG_VERIFY_COUNT]);  
     }
     reg_buf_count=0;    
}
 
 static void  set_isp_dvfs_work_index(unsigned int   index)
 {
     u32 index_cfg_reg = 0;
 
	pr_info("dvfs : %s, work index=%d\n", __func__, index);
 
     index_cfg_reg = DVFS_REG_RD(REG_MM_DVFS_AHB_ISP_DVFS_INDEX_CFG);
     index_cfg_reg = (index_cfg_reg & (~0x7)) | index;
     DVFS_REG_WR(REG_MM_DVFS_AHB_ISP_DVFS_INDEX_CFG, index_cfg_reg);
	pr_info("dvfs : %s, index_cfg=%d\n", __func__, index_cfg_reg);
 }

   
   static void  set_cpp_dvfs_work_index( unsigned int     index)
   {
       u32 index_cfg_reg = 0;
   
       pr_info("dvfs ops: %s, work index=%d\n", __func__, index);
   
       index_cfg_reg = DVFS_REG_RD(REG_MM_DVFS_AHB_CPP_DVFS_INDEX_CFG);
       index_cfg_reg = (index_cfg_reg & (~0x7)) | index;
       DVFS_REG_WR(REG_MM_DVFS_AHB_CPP_DVFS_INDEX_CFG, index_cfg_reg);
       pr_info("dvfs : %s, index_cfg=%d\n", __func__, index_cfg_reg);
   
   }
  
   /*work-idle dvfs index ops*/
   static void  set_fd_dvfs_work_index( unsigned int      index)
   {
       u32 index_cfg_reg = 0;
   
       pr_info("dvfs : %s, work index=%d\n", __func__, index);
        index_cfg_reg = DVFS_REG_RD(REG_MM_DVFS_AHB_FD_DVFS_INDEX_CFG);
       index_cfg_reg = (index_cfg_reg & (~0x7)) | index;
       DVFS_REG_WR(REG_MM_DVFS_AHB_FD_DVFS_INDEX_CFG, index_cfg_reg);
       pr_info("dvfs : %s, index_cfg=%d\n", __func__, index_cfg_reg);
   }

   /*work-idle dvfs index ops*/
   static void  set_dcam_dvfs_work_index( unsigned int index)
   {
       u32 index_cfg_reg = 0;
   
       pr_info("dvfs : %s, work index=%d\n", __func__, index);
   
       index_cfg_reg = DVFS_REG_RD(REG_MM_DVFS_AHB_DCAM_IF_DVFS_INDEX_CFG);
       index_cfg_reg = (index_cfg_reg & (~0x7)) | index;
       DVFS_REG_WR(REG_MM_DVFS_AHB_DCAM_IF_DVFS_INDEX_CFG, index_cfg_reg);
 }

 void mmsys_rf_index_cfg(u8 rf_index, mmsys_module m_type)
{
	switch((mmsys_module)(m_type))
	 {
		   case DVFS_DCAM:
			   set_dcam_dvfs_work_index(rf_index); 
			   break;
		   case DVFS_CPP:
			   set_cpp_dvfs_work_index(rf_index);
			    break;
		   case DVFS_ISP:
			   set_isp_dvfs_work_index(rf_index);
			    break;
		  case DVFS_FD:
			   set_fd_dvfs_work_index(rf_index);
			    break;		
		   case DVFS_JPEG:	    
		   default:
			   break;
	}

}


void  ip_module_dvfs_work(mmsys_module module_type )
{		
	u8 case_idx=1;
	
	verify_idx[module_type]=6;

	if(module_type== DVFS_DCAM)
	{
		read_regto_buffer( );
		read_regto_buffer( );
		printk("[ip dvfs]: Enter ip_module_dvfs_work \n");
		read_regto_buffer( );
		read_regto_buffer( );
		read_regto_buffer( );
		read_regto_buffer( );
	}	
	else if(module_type <= DVFS_MTX)
	{
		printk("[ip dvfs]: Enter ip_module_dvfs_work \n");
		read_regto_buffer( );
		read_regto_buffer( );
		read_regto_buffer( );
		mmsys_case_verify(case_idx, verify_idx, module_type); 	
	}

}

void  ip_module_dvfs_idle(mmsys_module module_type )
{		
	u8 case_idx=0;
	u32 clk_reg=0;
	
	clk_reg = DVFS_REG_RD_AHB(REG_MM_IP_BUSY);
	printk("[ip dvfs]: Enter ip_module_dvfs_idle , get_ip_status 0x%x\n", clk_reg);

	
	verify_idx[module_type]=0;

	 if(module_type== DVFS_DCAM )
{
	int j =0 ;
		verify_idx[module_type]=6;

		read_regto_buffer( );
		read_regto_buffer( );

		 printk("[ip dvfs]: Enter ip_module_dvfs_work  reg_buf_count=%d \n", reg_buf_count);

	for(j=0; j<=reg_buf_count/4; j++)
		{
			mmsys_case_verify_value(1, verify_idx, module_type , &reg_value[j*4+1]);	
		}
	}
	else if(module_type <= DVFS_MTX)
	{		
		read_regto_buffer( );
		read_regto_buffer( );
	  	mmsys_case_verify(case_idx, verify_idx, module_type); 
	}	
}


void dvfs_test_case(u32 tc_id)
{
	int j =0 ;

	while(1)
	{
		if(reg_buf_count  == DEBUG_BUF_COUNT -DEBUG_VERIFY_COUNT )
		{
			for(j=0; j<=reg_buf_count/DEBUG_VERIFY_COUNT; j++)
			mmsys_case_verify_value(tc_id, verify_idx, 0 , &reg_value[j*DEBUG_VERIFY_COUNT]);   

			reg_buf_count=0;    

		} else {
			read_regto_buffer( );
			msleep(2);
			printk("**** dvfs  reg_buf_count = %d*********\n", reg_buf_count);
		}
}
}


void dvfs_test_case0(u32 tc_id)
{
	u8 i=0, mmtype= 0;
	u8 case_idx=tc_id;

	for(mmtype=DVFS_ISP; mmtype<=DVFS_ISP; mmtype++)
	{   
		for(i = 0; i < 8; i++)
		{
			printk("dvfs mmtype:%d i=%d*************\n", mmtype,i);
			mmsys_rf_index_cfg(i, mmtype);
			msleep(20);
			verify_idx[mmtype]=i;
			mmsys_case_verify(case_idx, verify_idx,mmtype); 
		}

		i=2;
		mmsys_rf_index_cfg(i, mmtype);
		verify_idx[mmtype]=i;
		mmsys_case_verify(case_idx, verify_idx,mmtype); 
	}
}


