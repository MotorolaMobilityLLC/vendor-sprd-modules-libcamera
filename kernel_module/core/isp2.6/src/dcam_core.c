/*
 * Copyright (C) 2017-2018 Spreadtrum Communications Inc.
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

#include <linux/fs.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/regmap.h>
#include <linux/spinlock.h>
#include <linux/kthread.h>
#include <linux/sprd_ion.h>

#include <sprd_isp_r8p1.h>
#include "sprd_img.h"
#include <video/sprd_mmsys_pw_domain.h>
#include <sprd_mm.h>


#include "cam_hw.h"
#include "cam_types.h"
#include "cam_queue.h"
#include "cam_buf.h"
#include "cam_block.h"
#include "cam_debugger.h"

#include "dcam_reg.h"
#include "dcam_int.h"

#include "dcam_interface.h"
#include "dcam_core.h"
#include "dcam_path.h"

#include "mm_ahb.h"

#include <linux/sprd_iommu.h>
#include <linux/sprd_ion.h>


#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt) "DCAM_CORE: %d %d %s : "\
	fmt, current->pid, __LINE__, __func__

#define DCAMX_STOP_TIMEOUT 2000
#define DCAM_AXI_STOP_TIMEOUT 2000
#define DCAM_AXIM_AQOS_MASK (0x30FFFF)

/* VCH2 maybe used for raw picture output
 * If yes, PDAF should not output data though VCH2
 * todo: avoid conflict between raw/pdaf type3
 */
struct statis_path_buf_info s_statis_path_info_all[] = {
	{DCAM_PATH_PDAF,    STATIS_PDAF_BUF_SIZE,  STATIS_PDAF_BUF_NUM, STATIS_PDAF},
	{DCAM_PATH_VCH2,    STATIS_EBD_BUF_SIZE,   STATIS_EBD_BUF_NUM, STATIS_EBD},
	{DCAM_PATH_AEM,     STATIS_AEM_BUF_SIZE,   STATIS_AEM_BUF_NUM, STATIS_AEM},
	{DCAM_PATH_AFM,     STATIS_AFM_BUF_SIZE,   STATIS_AFM_BUF_NUM, STATIS_AFM},
	{DCAM_PATH_AFL,     STATIS_AFL_BUF_SIZE,   STATIS_AFL_BUF_NUM, STATIS_AFL},
	{DCAM_PATH_HIST,    STATIS_HIST_BUF_SIZE,  STATIS_HIST_BUF_NUM, STATIS_HIST},
	{DCAM_PATH_3DNR,    STATIS_3DNR_BUF_SIZE,  STATIS_3DNR_BUF_NUM, STATIS_3DNR},

	/* DCAM_PATH_MAX used for ISP blocks,must put after DCAM blocks */
	{DCAM_PATH_MAX,    STATIS_ISP_HIST2_BUF_SIZE,  STATIS_ISP_HIST2_BUF_NUM, STATIS_HIST2},
};

static atomic_t s_dcam_working;
static atomic_t s_dcam_axi_opened;

/* dcam debugfs start */
#define DCAM_DEBUG
uint32_t g_dcam_bypass[DCAM_ID_MAX] = { 0, 0, 0 };
static atomic_t s_dcam_opened[DCAM_ID_MAX];
uint32_t g_dbg_zoom_mode = 1;
uint32_t g_dbg_rds_limit = 30;

struct cam_dbg_dump g_dbg_dump;

#ifdef DCAM_DEBUG
static struct dentry *s_p_dentry;
struct bypass_tag {
	char *p; /* abbreviation */
	uint32_t addr;
	uint32_t bpos; /* bit position */
};
static const struct bypass_tag tb_bypass[] = {
	[_E_4IN1] = {"4in1", DCAM_MIPI_CAP_CFG, 12}, /* 0x100.b12 */
	[_E_PDAF] = {"pdaf", DCAM_PPE_FRM_CTRL0, 1}, /* 0x120.b1 */
	[_E_LSC]  = {"lsc", DCAM_LENS_LOAD_ENABLE, 0}, /* 0x138.b0 */
	[_E_AEM]  = {"aem",  DCAM_AEM_FRM_CTRL0, 0}, /* 0x150.b0 */
	[_E_HIST] = {"hist", DCAM_HIST_FRM_CTRL0, 0}, /* 0x160.b0 */
	[_E_AFL]  = {"afl",  ISP_AFL_FRM_CTRL0, 0}, /* 0x170.b0 */
	[_E_AFM]  = {"afm",  ISP_AFM_FRM_CTRL, 0}, /* 0x1A0.b0 */
	[_E_BPC]  = {"bpc",  ISP_BPC_PARAM, 0}, /* 0x200.b0 */
	[_E_BLC]  = {"blc",  DCAM_BLC_PARA_R_B, 31}, /* 0x268.b31 */
	[_E_RGB]  = {"rgb",  ISP_RGBG_YRANDOM_PARAMETER0, 0}, /* 0x278.b0 rgb gain */
	[_E_RAND] = {"rand", ISP_RGBG_YRANDOM_PARAMETER0, 1}, /* 0x278.b1 */
	[_E_PPI]  = {"ppi",  ISP_PPI_PARAM, 0}, /* 0x284.b0 */
	[_E_AWBC] = {"awbc", ISP_AWBC_GAIN0, 31}, /* 0x380.b31 */
	[_E_NR3]  = {"nr3",  NR3_FAST_ME_PARAM, 0}, /* 0x3F0.b0 */
};
/* dcam sub block bypass
 * How: echo 4in1:1 > bypass_dcam0
 */
static ssize_t bypass_write(struct file *filp,
		const char __user *buffer, size_t count, loff_t *ppos)
{
	struct seq_file *p = (struct seq_file *)filp->private_data;
	uint32_t idx = *((uint32_t *)p->private);
	char buf[256];
	uint32_t val = 1; /* default bypass */
	struct bypass_tag dat;
	int i;
	char name[16];
	int bypass_all = 0;

	if (atomic_read(&s_dcam_opened[idx]) <= 0) {
		pr_info("dcam%d Hardware not enable\n", idx);
		return count;
	}
	memset(buf, 0x00, sizeof(buf));
	i = count;
	if (i >= sizeof(buf))
		i = sizeof(buf) - 1; /* last one for \0 */
	if (copy_from_user(buf, buffer, i)) {
		pr_err("fail to get user info\n");
		return -EFAULT;
	}
	buf[i] = '\0';
	/* get name */
	for (i = 0; i < sizeof(name) - 1; i++) {
		if (' ' == buf[i] || ':' == buf[i] ||
			',' == buf[i])
			break;
		if (buf[i] >= 'A' && buf[i] <= 'Z')
			buf[i] += ('a' - 'A');
		name[i] = buf[i];
	}
	name[i] = '\0';
	/* get val */
	for (; i < sizeof(buf); i++) {
		if (buf[i] >= '0' && buf[i] <= '9') {
			val = buf[i] - '0';
			break;
		}
	}
	val = val != 0 ? 1 : 0;
	/* find */
	if (strcmp(name, "all") == 0)
		bypass_all = 1;

	for (i = 0; i < sizeof(tb_bypass) / sizeof(struct bypass_tag); i++) {
		if (strcmp(tb_bypass[i].p, name) == 0 || bypass_all) {
			dat = tb_bypass[i];
			pr_debug("set dcam%d addr 0x%x, bit %d val %d\n",
				idx, dat.addr, dat.bpos, val);
			g_dcam_bypass[idx] &= (~(1 << i));
			g_dcam_bypass[idx] |= (val << i);
			msleep(20); /* If PM writing,wait little time */
			DCAM_REG_MWR(idx, dat.addr, 1 << dat.bpos,
				val << dat.bpos);
			/* afl need rgb2y work */
			if (strcmp(name, "afl") == 0)
				DCAM_REG_MWR(idx, ISP_AFL_PARAM0,
					BIT_0, val);
			if (!bypass_all)
				break;
		}
	}
	/* not opreate */
	if ((!bypass_all) && i >= sizeof(tb_bypass) / sizeof(struct bypass_tag))
		pr_info("Not operate, dcam%d,name:%s val:%d\n",
			idx, name, val);


	return count;
}

static int bypass_read(struct seq_file *s, void *unused)
{
	uint32_t idx = *((uint32_t *)s->private);
	uint32_t addr, val;
	struct bypass_tag dat;
	int i = 0;

	seq_printf(s, "-----dcam%d-----\n", idx);
	if (atomic_read(&s_dcam_opened[idx]) <= 0) {
		seq_puts(s, "Hardware not enable\n");
		if (unlikely(i))
			pr_warn("copy to user fail\n");
	} else {
		for (i = 0; i < sizeof(tb_bypass) /
			sizeof(struct bypass_tag); i++) {
			dat = tb_bypass[i];
			addr = dat.addr;
			val = DCAM_REG_RD(idx, addr) & (1 << dat.bpos);
			if (val)
				seq_printf(s, "%s:bit%d=1 bypass\n",
					dat.p, dat.bpos);
			else
				seq_printf(s, "%s:bit%d=0  work\n",
					dat.p, dat.bpos);
		}
		seq_puts(s, "\nall:1 #to bypass all\n");
	}
	return 0;
}

static int bypass_open(struct inode *inode, struct file *file)
{
	return single_open(file, bypass_read, inode->i_private);
}
static const struct file_operations bypass_ops = {
	.owner = THIS_MODULE,
	.open = bypass_open,
	.read = seq_read,
	.write = bypass_write,
	.llseek = seq_lseek,
	.release = single_release,
};

/* read dcamx register once */
static int dcam_reg_show(struct seq_file *s, void *unused)
{
	uint32_t idx = *(uint32_t *)s->private;
	int addr;
	const uint32_t addr_end[] = {0x400, 0x400, 0x110, 0x44};


	if (idx == 3) {
		seq_puts(s, "-----dcam axi and fetch----\n");
		if (atomic_read(&s_dcam_axi_opened) <= 0) {
			seq_puts(s, "Hardware not enable\n");

			return 0;
		}

		for (addr = 0; addr < addr_end[idx]; addr += 4)
			seq_printf(s, "0x%04x: 0x%08x\n",
				addr,  DCAM_AXIM_RD(addr));

		seq_puts(s, "--------------------\n");
	} else {
		seq_printf(s, "-----dcam%d----------\n", idx);
		if (atomic_read(&s_dcam_opened[idx]) <= 0) {
			seq_puts(s, "Hardware not enable\n");

			return 0;
		}
		for (addr = 0; addr < addr_end[idx]; addr += 4)
			seq_printf(s, "0x%04x: 0x%08x\n",
				addr,  DCAM_REG_RD(idx, addr));

		seq_puts(s, "--------------------\n");
	}

	return 0;
}

static int dcam_reg_open(struct inode *inode, struct file *file)
{
	return single_open(file, dcam_reg_show, inode->i_private);
}

static const struct file_operations dcam_reg_ops = {
	.owner = THIS_MODULE,
	.open = dcam_reg_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static char zoom_mode_strings[2][8] = {"binning", "rds"};

static ssize_t zoom_mode_show(
		struct file *filp, char __user *buffer,
		size_t count, loff_t *ppos)
{
	char buf[16];

	snprintf(buf, sizeof(buf), "%d(%s)\n", g_dbg_zoom_mode,
		zoom_mode_strings[g_dbg_zoom_mode&1]);

	return simple_read_from_buffer(
			buffer, count, ppos,
			buf, strlen(buf));
}

static ssize_t zoom_mode_write(
		struct file *filp, const char __user *buffer,
		size_t count, loff_t *ppos)
{
	int ret = 0;
	char msg[8];
	char *last;
	int val;

	if (count > 2)
		return -EINVAL;

	ret = copy_from_user(msg, (void __user *)buffer, count);
	if (ret) {
		pr_err("fail to copy_from_user\n");
		return -EFAULT;
	}

	msg[1] = '\0';
	val = simple_strtol(msg, &last, 0);
	if (val == 0)
		g_dbg_zoom_mode = 0;
	else if (val == 1)
		g_dbg_zoom_mode = 1;
	else
		pr_err("error: invalid zoom mode: %d", val);

	pr_info("set zoom mode %d(%s)\n", g_dbg_zoom_mode,
		zoom_mode_strings[g_dbg_zoom_mode&1]);
	return count;
}

static const struct file_operations zoom_mode_ops = {
	.owner =	THIS_MODULE,
	.open = simple_open,
	.read = zoom_mode_show,
	.write = zoom_mode_write,
};

static ssize_t rds_limit_show(
		struct file *filp, char __user *buffer,
		size_t count, loff_t *ppos)
{
	char buf[16];

	snprintf(buf, sizeof(buf), "%d\n", g_dbg_rds_limit);

	return simple_read_from_buffer(
			buffer, count, ppos,
			buf, strlen(buf));
}

static ssize_t rds_limit_write(
		struct file *filp, const char __user *buffer,
		size_t count, loff_t *ppos)
{
	int ret = 0;
	char msg[8];
	char *last;
	int val;

	if (count > 3)
		return -EINVAL;

	ret = copy_from_user(msg, (void __user *)buffer, count);
	if (ret) {
		pr_err("fail to copy_from_user\n");
		return -EFAULT;
	}

	msg[2] = '\0';
	val = simple_strtol(msg, &last, 0);
	if (val > 10 && val <= 40)
		g_dbg_rds_limit = val;
	else
		pr_err("error: invalid rds limit: %d", val);

	pr_info("set rds limit %d\n", g_dbg_rds_limit);
	return count;
}

static const struct file_operations rds_limit_ops = {
	.owner =	THIS_MODULE,
	.open = simple_open,
	.read = rds_limit_show,
	.write = rds_limit_write,
};

static ssize_t dump_raw_show(
		struct file *filp, char __user *buffer,
		size_t count, loff_t *ppos)
{
	const char *desc = "0: disable, 1: both, 2: full, 3: bin";
	char buf[48];

	snprintf(buf, sizeof(buf), "%u\n\n%s\n", g_dbg_dump.dump_en, desc);

	return simple_read_from_buffer(
			buffer, count, ppos,
			buf, strlen(buf));
}

static ssize_t dump_raw_write(
		struct file *filp, const char __user *buffer,
		size_t count, loff_t *ppos)
{
	int ret = 0;
	char msg[8];
	uint32_t val;

	if (count > 2)
		return -EINVAL;

	ret = copy_from_user(msg, (void __user *)buffer, count);
	if (ret) {
		pr_err("fail to copy_from_user\n");
		return -EFAULT;
	}

	msg[count] = '\0';
	ret = kstrtouint(msg, 10, &val);
	if (ret < 0) {
		pr_err("fail to convert '%s', ret %d", msg, ret);
		return ret;
	}

	g_dbg_dump.dump_en = val;
	pr_info("set dump_raw_en %u\n", g_dbg_dump.dump_en);

	return count;
}

static const struct file_operations dump_raw_ops = {
	.owner =	THIS_MODULE,
	.open = simple_open,
	.read = dump_raw_show,
	.write = dump_raw_write,
};

static ssize_t dump_count_write(
		struct file *filp, const char __user *buffer,
		size_t count, loff_t *ppos)
{
	int ret = 0;
	char msg[8];
	char *last;
	int val;
	struct cam_dbg_dump *dbg = &g_dbg_dump;

	if ((dbg->dump_en == 0) || count > 3)
		return -EINVAL;

	ret = copy_from_user(msg, (void __user *)buffer, count);
	if (ret) {
		pr_err("fail to copy_from_user\n");
		return -EFAULT;
	}

	msg[3] = '\0';
	val = simple_strtol(msg, &last, 0);

	/* for preview dcam raw dump frame count. */
	/* dump thread will be trigged when this value is set. */
	/* valid value: 1 ~ 99 */
	/* if dump thread is ongoing, new setting will not be accepted. */
	/* capture raw dump will be triggered when catpure starts. */
	mutex_lock(&dbg->dump_lock);
	dbg->dump_count = 0;
	if (val >= 200 || val == 0) {
		pr_err("unsupported dump_raw_count %d\n", val);
	} else if (dbg->dump_ongoing == 0) {
		dbg->dump_count = val;
		if (dbg->dump_start[0])
			complete(dbg->dump_start[0]);
		if (dbg->dump_start[1])
			complete(dbg->dump_start[1]);
		pr_info("set dump_raw_count %d\n", dbg->dump_count);
	}
	mutex_unlock(&dbg->dump_lock);

	return count;
}

static const struct file_operations dump_count_ops = {
	.owner =	THIS_MODULE,
	.open = simple_open,
	.write = dump_count_write,
};

static int replace_image_read(struct seq_file *s, void *unused)
{
	struct dcam_image_replacer *replacer;
	char *str;
	int i, j;

	replacer = (struct dcam_image_replacer *)s->private;
	seq_printf(s, "\n");
	for (i = 0; i < DCAM_ID_MAX; i++) {
		seq_printf(s, "DCAM%d\n", i);
		for (j = 0; j < DCAM_IMAGE_REPLACER_PATH_MAX; j++) {
			str = "<disabled>";
			if (replacer[i].enabled[j])
				str = replacer[i].filename[j];
			seq_printf(s, "   %s: %s\n", to_path_name(j), str);
		}
	}

	seq_printf(s, "\nUsage:\n");
	seq_printf(s, "         echo DCAM_ID PATH_ID disable > replace_image\n");
	seq_printf(s, "         echo DCAM_ID PATH_ID filename > replace_image\n");
	seq_printf(s, "\nExample:\n");
	seq_printf(s, "         echo 0 0 disable > replace_image        // disable\n");
	seq_printf(s, "         echo 1 1 abc.mipi_raw > replace_image   // replace DCAM1 BIN image with 'abc.mipi_raw'\n");

	return 0;
}

static int replace_image_open(struct inode *inode, struct file *file)
{
	return single_open(file, replace_image_read, inode->i_private);
}

static ssize_t replace_image_write(struct file *filp, const char __user *buffer,
				   size_t count, loff_t *ppos)
{
	struct seq_file *p = (struct seq_file *)filp->private_data;
	struct dcam_image_replacer *replacer =
		(struct dcam_image_replacer *)p->private;
	char buf[DCAM_IMAGE_REPLACER_FILENAME_MAX] = { 0 };
	char *s, *c;
	uint32_t dcam, path;
	int ret;

	/* filename is less than this value, which is intentional */
	if (count > DCAM_IMAGE_REPLACER_FILENAME_MAX) {
		pr_err("command too long\n");
		return -EINVAL;
	}

	ret = copy_from_user(buf, (void __user *)buffer, count);
	if (ret) {
		pr_err("fail to copy_from_user\n");
		return -EINVAL;
	}
	buf[count - 1] = '\0';

	/* DCAM id */
	s = buf;
	c = strchr(s, ' ');
	if (!c) {
		pr_err("fail to get DCAM id\n");
		return -EINVAL;
	}
	*c = '\0';

	if (kstrtouint(s, 10, &dcam) < 0) {
		pr_err("fail to parse DCAM id '%s'\n", s);
		return -EINVAL;
	}

	if (dcam >= DCAM_ID_MAX) {
		pr_err("invalid DCAM id %u\n", dcam);
		return -EINVAL;
	}

	/* path */
	s = c + 1;
	c = strchr(s, ' ');
	if (!c) {
		pr_err("fail to get DCAM path\n");
		return -EINVAL;
	}
	*c = '\0';

	if (kstrtouint(s, 10, &path) < 0) {
		pr_err("fail to parse DCAM path '%s'\n", s);
		return -EINVAL;
	}

	/* filename */
	s = c + 1;
	if (path >= DCAM_IMAGE_REPLACER_PATH_MAX) {
		pr_err("unsupported DCAM path %u\n", path);
		return -EINVAL;
	}

	/* filename */
	replacer[dcam].enabled[path] = !!strncmp(s, "disable", 7);
	strcpy(replacer[dcam].filename[path], s);

	pr_info("DCAM%u %s set replace image: %u %s\n",
		dcam, to_path_name(path),
		replacer[dcam].enabled[path],
		replacer[dcam].filename[path]);

	return count;
}

static const struct file_operations replace_image_ops = {
	.owner = THIS_MODULE,
	.open = replace_image_open,
	.read = seq_read,
	.write = replace_image_write,
};

/* /sys/kernel/debug/sprd_dcam/
 * dcam0_reg, dcam1_reg, dcam2_reg, dcam_axi_reg
 * dcam0/1/2_reg,dcam_axi_reg: cat .....(no echo > )
 * reg_dcam: echo 0xxxx > reg_dcam, then cat reg_dcam
 */
int sprd_dcam_debugfs_init(struct camera_debugger *debugger)
{
	/* folder in /sys/kernel/debug/ */
	const char tb_folder[] = {"sprd_dcam"};
	struct dentry *pd, *entry;
	static int tb_dcam_id[] = {0, 1, 2, 3};
	int ret = 0;

	s_p_dentry = debugfs_create_dir(tb_folder, NULL);
	pd = s_p_dentry;
	if (pd == NULL)
		return -ENOMEM;
	/* sub block bypass */
	if (!debugfs_create_file("bypass_dcam0", 0660,
		pd, &tb_dcam_id[0], &bypass_ops))
		ret |= BIT(1);
	if (!debugfs_create_file("bypass_dcam1", 0660,
		pd, &tb_dcam_id[1], &bypass_ops))
		ret |= BIT(2);
	/* the same response Function, parameter differ */
	if (!debugfs_create_file("reg_dcam0", 0440,
		pd, &tb_dcam_id[0], &dcam_reg_ops))
		ret |= BIT(3);
	if (!debugfs_create_file("reg_dcam1", 0440,
		pd, &tb_dcam_id[1], &dcam_reg_ops))
		ret |= BIT(4);
	if (!debugfs_create_file("reg_dcam2", 0440,
		pd, &tb_dcam_id[2], &dcam_reg_ops))
		ret |= BIT(5);
	if (!debugfs_create_file("reg_fetch", 0440,
		pd, &tb_dcam_id[3], &dcam_reg_ops))
		ret |= BIT(6);
	if (!debugfs_create_file("zoom_mode", 0664,
		pd, NULL, &zoom_mode_ops))
		ret |= BIT(7);
	if (!debugfs_create_file("zoom_rds_limit", 0664,
		pd, NULL, &rds_limit_ops))
		ret |= BIT(8);

	if (!debugfs_create_file("dump_raw_en", 0664,
		pd, NULL, &dump_raw_ops))
		ret |= BIT(9);
	if (!debugfs_create_file("dump_count", 0664,
		pd, NULL, &dump_count_ops))
		ret |= BIT(10);
	mutex_init(&g_dbg_dump.dump_lock);

	entry = debugfs_create_file("replace_image", 0644, pd,
				    &debugger->replacer[0],
				    &replace_image_ops);
	if (IS_ERR_OR_NULL(entry))
		return -ENOMEM;

	if (ret)
		ret = -ENOMEM;
	pr_info("dcam debugfs init ok\n");

	return ret;
}

int sprd_dcam_debugfs_deinit(void)
{
	if (s_p_dentry)
		debugfs_remove_recursive(s_p_dentry);
	s_p_dentry = NULL;
	mutex_destroy(&g_dbg_dump.dump_lock);
	return 0;
}
#else
int sprd_dcam_debugfs_init(struct camera_debugger *debugger)
{
	memset(g_dcam_bypass, 0x00, sizeof(g_dcam_bypass));
	return 0;
}

int sprd_dcam_debugfs_deinit(void)
{
	return 0;
}
#endif
/* dcam debugfs end */


static uint32_t dcam_trace_regs[] = {
		DCAM_CFG,
		DCAM_APB_SRAM_CTRL,
		DCAM_IMAGE_CONTROL,
		DCAM_PDAF_CONTROL,
		DCAM_LENS_LOAD_ENABLE,
		ISP_BPC_PARAM,
		DCAM_AEM_FRM_CTRL0,
		ISP_AFM_FRM_CTRL,
		ISP_AFL_FRM_CTRL0,
		DCAM_HIST_FRM_CTRL0,
		NR3_FAST_ME_PARAM,
		DCAM_FULL_BASE_WADDR,
		DCAM_BIN_BASE_WADDR0,
		DCAM_PDAF_BASE_WADDR,
		DCAM_VCH2_BASE_WADDR,
		DCAM_VCH3_BASE_WADDR,
		DCAM_AEM_BASE_WADDR,
		DCAM_HIST_BASE_WADDR,
		DCAM_PPE_RIGHT_WADDR,
		ISP_AFL_GLB_WADDR,
		ISP_AFL_REGION_WADDR,
		ISP_BPC_OUT_ADDR,
		ISP_AFM_BASE_WADDR,
		ISP_NR3_WADDR,
};
static void dcam_debug_trace(struct dcam_pipe_dev *dev)
{
	uint32_t addr, val_mmu, val[8], i, j, n, cnt;

	val_mmu = DCAM_MMU_RD(MMU_EN);
	cnt = sizeof(dcam_trace_regs) / sizeof(dcam_trace_regs[0]);
	pr_info("dcam%d: 0x%08x, cnt %d\n", dev->idx, val_mmu, cnt);

	for (i = 0; i < cnt; i += 8) {
		memset(val, 0, sizeof(val));
		n = ((cnt - i) < 8) ? (cnt - i) : 8;
		for (j = 0; j < n; j++) {
			addr = dcam_trace_regs[i + j];
			val[j] = DCAM_REG_RD(dev->idx, addr);
		}
		pr_info("n=%d, %08x %08x %08x %08x %08x %08x %08x %08x\n", n,
			val[0], val[1], val[2], val[3], val[4], val[5], val[6], val[7]);
	}
	return;
}


/*
 * Do force copy to before capture enabled.
 * id: refer to enum dcam_ctrl_id in dcam_core.h
 * bit0 - cap, bit1 - coef, bit2 - rds coef, bit3 - full, ...
 */
void dcam_force_copy(struct dcam_pipe_dev *dev, uint32_t id)
{
	const uint32_t bitmap[] = {
		BIT_0, BIT_4, BIT_6, BIT_8, BIT_10, BIT_12, BIT_14, BIT_16
	};
	const uint32_t bitmap2 = BIT_4;
	uint32_t mask = 0, j;
	unsigned long flags = 0;

	if (unlikely(!dev)) {
		pr_warn("invalid param dev\n");
		return;
	}

	if (dev->idx < 2) {
		for (j = 0; j < 8; j++) {
			if (id & (1 << j))
				mask |= bitmap[j];
		}
	} else if (id && DCAM_CTRL_CAP) {
		mask = bitmap2;
	}
	pr_debug("DCAM%u: force copy 0x%0x, id 0x%x\n", dev->idx, mask, id);
	if (mask == 0)
		return;

	spin_lock_irqsave(&dev->glb_reg_lock, flags);
	DCAM_REG_MWR(dev->idx, DCAM_CONTROL, mask, mask);
	spin_unlock_irqrestore(&dev->glb_reg_lock, flags);
}

void dcam_auto_copy(struct dcam_pipe_dev *dev, uint32_t id)
{
	const uint32_t bitmap[] = {
		BIT_1, BIT_5, BIT_7, BIT_9, BIT_11, BIT_13, BIT_15, BIT_17
	};
	const uint32_t bitmap2 = BIT_5;
	uint32_t mask = 0, j;
	unsigned long flags = 0;

	if (unlikely(!dev)) {
		pr_warn("invalid param dev\n");
		return;
	}
	if (dev->idx < 2) {
		for (j = 0; j < 8; j++) {
			if (id & (1 << j))
				mask |= bitmap[j];
		}
	} else if (id && DCAM_CTRL_CAP) {
		mask = bitmap2;
	}

	pr_debug("DCAM%u: auto copy 0x%0x, id 0x%x\n", dev->idx, mask, id);
	if (mask == 0)
		return;

	spin_lock_irqsave(&dev->glb_reg_lock, flags);
	DCAM_REG_MWR(dev->idx, DCAM_CONTROL, mask, mask);
	spin_unlock_irqrestore(&dev->glb_reg_lock, flags);
}



/* TODO: check this */
/*
 * Some register is in RAM, so it's important to set initial value for them.
 */
static void dcam_init_default(struct dcam_pipe_dev *dev)
{
	uint32_t idx, bypass, eb;

	if (unlikely(!dev)) {
		pr_warn("invalid param dev\n");
		return;
	}
	idx = dev->idx;

	/* init registers(sram regs) to default value */
	dcam_reg_set_default_value(idx);


	/* disable internal logic access sram */
	DCAM_REG_MWR(idx, DCAM_APB_SRAM_CTRL, BIT_0, 0);

	DCAM_REG_WR(idx, DCAM_CFG, 0); /* disable all path */
	DCAM_REG_WR(idx, DCAM_IMAGE_CONTROL, 0x2b << 8 | 0x01);

	eb = 0;
	DCAM_REG_MWR(idx, DCAM_PDAF_CONTROL, BIT_1 | BIT_0, eb);
	DCAM_REG_MWR(idx, DCAM_CROP0_START, BIT_31, eb << 31);

	/* default bypass all blocks */
	bypass = 1;
	DCAM_REG_MWR(idx, DCAM_BLC_PARA_R_B, BIT_31, bypass << 31);
	DCAM_REG_MWR(idx, ISP_RGBG_YRANDOM_PARAMETER0, BIT_0, bypass);
	DCAM_REG_MWR(idx, ISP_RGBG_YRANDOM_PARAMETER0, BIT_1, bypass << 1);
	DCAM_REG_MWR(idx, DCAM_LENS_LOAD_ENABLE, BIT_0, bypass);
	DCAM_REG_MWR(idx, ISP_PPI_PARAM, BIT_0, bypass);
	DCAM_REG_MWR(idx, ISP_AWBC_GAIN0, BIT_31, bypass << 31);
	DCAM_REG_MWR(idx, ISP_BPC_PARAM, 0xF, 0xF); /*bpc bypass all */
	DCAM_REG_MWR(idx, ISP_AFL_PARAM0, BIT_1, bypass << 1); /*bayer2y*/

	/* 3A statistic */
	DCAM_REG_MWR(idx, DCAM_AEM_FRM_CTRL0, BIT_0, bypass);
	DCAM_REG_MWR(idx, ISP_AFM_FRM_CTRL, BIT_0, bypass);
	DCAM_REG_MWR(idx, ISP_AFL_FRM_CTRL0, BIT_0, bypass);
	DCAM_REG_MWR(idx, NR3_FAST_ME_PARAM, BIT_0, bypass);
	DCAM_REG_MWR(idx, DCAM_HIST_FRM_CTRL0, BIT_0, bypass);
}

/* TODO: check this */
static int dcam_reset(struct dcam_pipe_dev *dev)
{
	int ret = 0;
	enum dcam_id idx = dev->idx;
	struct sprd_cam_hw_info *hw = dev->hw;
	uint32_t time_out = 0, flag = 0;
	uint32_t reset_bit[DCAM_ID_MAX] = {
		BIT_MM_AHB_DCAM0_SOFT_RST,
		BIT_MM_AHB_DCAM1_SOFT_RST,
		BIT_MM_AHB_DCAM2_SOFT_RST
	};
	uint32_t sts_bit[DCAM_ID_MAX] = {
		BIT(12), BIT(13), BIT(14)
	};

	pr_info("DCAM%d: reset.\n", idx);

	/* then wait for AXIM cleared */
	while (++time_out < DCAM_AXI_STOP_TIMEOUT) {
		if (0 == (DCAM_AXIM_RD(AXIM_DBG_STS) & sts_bit[idx]))
			break;
		udelay(1000);
	}

	if (time_out >= DCAM_AXI_STOP_TIMEOUT) {
		pr_info("DCAM%d: reset timeout, axim status 0x%x\n", idx,
			DCAM_AXIM_RD(AXIM_DBG_STS));
	} else {
		flag = reset_bit[idx];
		regmap_update_bits(hw->cam_ahb_gpr,
			REG_MM_AHB_AHB_RST, flag, flag);
		udelay(10);
		regmap_update_bits(hw->cam_ahb_gpr,
			REG_MM_AHB_AHB_RST, flag, ~flag);
	}

	DCAM_REG_MWR(idx, DCAM_INT_CLR,
		DCAMINT_IRQ_LINE_MASK, DCAMINT_IRQ_LINE_MASK);
	DCAM_REG_MWR(idx, DCAM_INT_EN,
		DCAMINT_IRQ_LINE_MASK, DCAMINT_IRQ_LINE_MASK);

	dcam_init_default(dev);
	pr_info("DCAM%d: reset end\n", idx);

	return ret;
}


static void dcam_init_axim(struct sprd_cam_hw_info *hw)
{
	uint32_t reg_val;
	uint32_t time_out = 0;

	/* firstly, stop AXI writing. */
	DCAM_AXIM_MWR(AXIM_CTRL, BIT_24 | BIT_23, (0x3 << 23));

	/* then wait for AHB busy cleared */
	while (++time_out < DCAM_AXI_STOP_TIMEOUT) {
		if (0 == (DCAM_AXIM_RD(AXIM_DBG_STS) & 0x1F00F))
			break;
		udelay(1000);
	}

	if (time_out >= DCAM_AXI_STOP_TIMEOUT) {
		pr_info("dcam axim timeout status 0x%x\n",
			DCAM_AXIM_RD(AXIM_DBG_STS));
	} else {
		/* reset dcam all (0/1/2/bus) */
		regmap_update_bits(hw->cam_ahb_gpr,
			REG_MM_AHB_AHB_RST,
			BIT_MM_AHB_DCAM_ALL_SOFT_RST,
			BIT_MM_AHB_DCAM_ALL_SOFT_RST);
		udelay(10);
		regmap_update_bits(hw->cam_ahb_gpr,
			REG_MM_AHB_AHB_RST,
			BIT_MM_AHB_DCAM_ALL_SOFT_RST, 0);
	}

	/* AXIM shared by all dcam, should be init once only...*/
	dcam_aximreg_set_default_value();

	reg_val = (0x0 << 20) |
		((hw->arqos_low & 0xF) << 12) |
		(0x8 << 8) |
		((hw->awqos_high & 0xF) << 4) |
		(hw->awqos_low & 0xF);
	DCAM_AXIM_MWR(AXIM_CTRL,
			DCAM_AXIM_AQOS_MASK, reg_val);

	/* the end, enable AXI writing */
	DCAM_AXIM_MWR(AXIM_CTRL, BIT_24 | BIT_23, (0x0 << 23));
}

/*
 * Start capture by set cap_eb.
 */
static int dcam_start(struct dcam_pipe_dev *dev)
{
	int ret = 0;
	uint32_t idx = dev->idx;

	DCAM_REG_WR(idx, DCAM_INT_CLR, 0xFFFFFFFF);

	/* see DCAM_PREVIEW_SOF in dcam_int.h for details */
	DCAM_REG_WR(idx, DCAM_INT_EN, DCAMINT_IRQ_LINE_EN_NORMAL);

	/* enable internal logic access sram */
	DCAM_REG_MWR(idx, DCAM_APB_SRAM_CTRL, BIT_0, 1);

	/* trigger cap_en*/
	DCAM_REG_MWR(idx, DCAM_CFG, BIT_0, 1);

	return ret;
}

static int dcam_stop(struct dcam_pipe_dev *dev)
{
	int ret = 0;
	int time_out = DCAMX_STOP_TIMEOUT;
	uint32_t idx = dev->idx;

	/* reset  cap_en*/
	DCAM_REG_MWR(idx, DCAM_CFG, BIT_0, 0);
	DCAM_REG_WR(idx, DCAM_PATH_STOP, 0x2DFF);

	DCAM_REG_WR(idx, DCAM_INT_EN, 0);
	DCAM_REG_WR(idx, DCAM_INT_CLR, 0xFFFFFFFF);

	/* wait for AHB path busy cleared */
	while (time_out) {
		ret = DCAM_REG_RD(idx, DCAM_PATH_BUSY) & 0x2FFF;
		if (!ret)
			break;
		udelay(1000);
		time_out--;
	}

	if (time_out == 0)
		pr_err("DCAM%d: stop timeout for 2s\n", idx);

	pr_info("dcam%d stop\n", idx);
	return ret;
}

/*
 * set MIPI capture related register
 * range: 0x0100 ~ 0x010c
 *
 * TODO: support DCAM2, some registers only exist in DCAM0/1
 */
static int dcam_set_mipi_cap(struct dcam_pipe_dev *dev,
				struct dcam_mipi_info *cap_info)
{
	int ret = 0;
	uint32_t idx = dev->idx;
	uint32_t reg_val;

	/* set mipi interface  */
	if (cap_info->sensor_if != DCAM_CAP_IF_CSI2) {
		pr_err("error: unsupported sensor if : %d\n",
			cap_info->sensor_if);
		return -EINVAL;
	}

	/* data format */
	if (cap_info->format == DCAM_CAP_MODE_RAWRGB) {
		DCAM_REG_MWR(idx, DCAM_MIPI_CAP_CFG, BIT_1, 1 << 1);
		DCAM_REG_MWR(idx, DCAM_MIPI_CAP_CFG, BIT_17 | BIT_16,
				cap_info->pattern << 16);
	} else if (cap_info->format == DCAM_CAP_MODE_YUV) {
		if (unlikely(cap_info->data_bits != DCAM_CAP_8_BITS)) {
			pr_err("error: invalid %d bits for yuv format\n",
				cap_info->data_bits);
			return -EINVAL;
		}

		DCAM_REG_MWR(idx, DCAM_MIPI_CAP_CFG, BIT_1,  0 << 1);
		DCAM_REG_MWR(idx, DCAM_MIPI_CAP_FRM_CTRL,
				BIT_1 | BIT_0, cap_info->pattern);

		/* x & y deci */
		DCAM_REG_MWR(idx, DCAM_MIPI_CAP_FRM_CTRL,
				BIT_9 | BIT_8 | BIT_5 | BIT_4,
				(cap_info->y_factor << 8)
				| (cap_info->x_factor << 4));
	} else {
		pr_err("error: unsupported capture format: %d\n",
			cap_info->format);
		return -EINVAL;
	}

	/* data mode */
	DCAM_REG_MWR(idx, DCAM_MIPI_CAP_CFG, BIT_2, cap_info->mode << 2);

	/* href */
	DCAM_REG_MWR(idx, DCAM_MIPI_CAP_CFG, BIT_3, cap_info->href << 3);

	/* data bits */
	if (cap_info->data_bits == DCAM_CAP_12_BITS) {
		reg_val = 2;
	} else if (cap_info->data_bits == DCAM_CAP_10_BITS) {
		reg_val = 1;
	} else if (cap_info->data_bits == DCAM_CAP_8_BITS) {
		reg_val = 0;
	} else {
		pr_err("error: unsupported data bits: %d\n",
			cap_info->data_bits);
		return -EINVAL;
	}
	DCAM_REG_MWR(idx, DCAM_MIPI_CAP_CFG, BIT_5 | BIT_4, reg_val << 4);

	/* frame deci */
	DCAM_REG_MWR(idx, DCAM_MIPI_CAP_CFG, BIT_7 | BIT_6,
			cap_info->frm_deci << 6);

	/* MIPI capture start */
	reg_val = (cap_info->cap_size.start_y << 16);
	reg_val |= cap_info->cap_size.start_x;
	DCAM_REG_WR(idx, DCAM_MIPI_CAP_START, reg_val);

	/* MIPI capture end */
	reg_val = (cap_info->cap_size.start_y
			+ cap_info->cap_size.size_y - 1) << 16;
	reg_val |= (cap_info->cap_size.start_x
			+ cap_info->cap_size.size_x - 1);
	DCAM_REG_WR(idx, DCAM_MIPI_CAP_END, reg_val);

	/* frame skip before capture */
	DCAM_REG_MWR(idx, DCAM_MIPI_CAP_CFG,
			BIT_8 | BIT_9 | BIT_10 | BIT_11,
				cap_info->frm_skip << 8);

	/* bypass 4in1 */
	if (cap_info->is_4in1) /* 4in1 use sum, not avrg */
		DCAM_REG_MWR(idx, DCAM_MIPI_CAP_CFG, BIT_13,
				(!!cap_info->is_4in1) << 13);
	DCAM_REG_MWR(idx, DCAM_MIPI_CAP_CFG, BIT_12,
			(!cap_info->is_4in1) << 12);

	pr_info("cap size : %d %d %d %d\n",
		cap_info->cap_size.start_x, cap_info->cap_size.start_y,
		cap_info->cap_size.size_x, cap_info->cap_size.size_y);
	pr_info("cap: frm %d, mode %d, bits %d, pattern %d, href %d\n",
		cap_info->format, cap_info->mode, cap_info->data_bits,
		cap_info->pattern, cap_info->href);
	pr_info("cap: deci %d, skip %d, x %d, y %d, 4in1 %d\n",
		cap_info->frm_deci, cap_info->frm_skip, cap_info->x_factor,
		cap_info->y_factor, cap_info->is_4in1);

	return ret;
}

static int dcam_cfg_ebd(struct dcam_pipe_dev *dev, void *param)
{
	struct sprd_ebd_control *p = (struct sprd_ebd_control *)param;
	uint32_t idx = dev->idx;

	pr_info("mode:0x%x, vc:0x%x, dt:0x%x\n", p->mode,
			p->image_vc, p->image_dt);
	dev->is_ebd = 1;
	DCAM_REG_WR(idx, DCAM_VC2_CONTROL,
		((p->image_vc & 0x3) << 16) |
		((p->image_dt & 0x3F) << 8) |
		(p->mode & 0x3));

	return 0;
}

static int dcam_cfg_dcamsec(struct dcam_pipe_dev *dev, void *param)
{
	bool *sec_eb = (bool *)param;

	dev->dcamsec_eb =  *sec_eb;

	pr_info("camca : dcamsec_mode=%d\n", dev->dcamsec_eb);
	return 0;
}

static int dcam_cfg_rps(struct dcam_pipe_dev *dev, void *param)
{
	uint32_t *data = (uint32_t *)param;

	dev->rps =  *data;

	pr_info("hwsim : rps[%d]\n", dev->rps);
	return 0;
}


void dcam_ret_src_frame(void *param)
{
	struct camera_frame *frame;
	struct dcam_pipe_dev *dev;

	if (!param) {
		pr_err("error: null input ptr.\n");
		return;
	}

	frame = (struct camera_frame *)param;
	dev = (struct dcam_pipe_dev *)frame->priv_data;
	pr_debug("frame %p, ch_id %d, buf_fd %d\n",
		frame, frame->channel_id, frame->buf.mfd[0]);

	cambuf_iommu_unmap(&frame->buf);
	dev->dcam_cb_func(
		DCAM_CB_RET_SRC_BUF,
		frame, dev->cb_priv_data);
}

void dcam_ret_out_frame(void *param)
{
	struct camera_frame *frame;
	struct dcam_pipe_dev *dev;
	struct dcam_path_desc *path;

	if (!param) {
		pr_err("error: null input ptr.\n");
		return;
	}

	frame = (struct camera_frame *)param;

	if (frame->is_reserved) {
		path =  (struct dcam_path_desc *)frame->priv_data;
		camera_enqueue(&path->reserved_buf_queue, frame);
	} else {
		cambuf_iommu_unmap(&frame->buf);
		dev = (struct dcam_pipe_dev *)frame->priv_data;
		dev->dcam_cb_func(DCAM_CB_DATA_DONE, frame, dev->cb_priv_data);
	}
}


void dcam_destroy_reserved_buf(void *param)
{
	struct camera_frame *frame;

	if (!param) {
		pr_err("error: null input ptr.\n");
		return;
	}

	frame = (struct camera_frame *)param;

	if (unlikely(frame->is_reserved == 0)) {
		pr_err("error: frame has no reserved buffer.");
		return;
	}
	/* is_reserved:
	 *  1:  basic mapping reserved buffer;
	 *  2:  copy of reserved buffer.
	 */
	if (frame->is_reserved == 1) {
		cambuf_iommu_unmap(&frame->buf);
		cambuf_put_ionbuf(&frame->buf);
	}
	put_empty_frame(frame);
}

void dcam_destroy_statis_buf(void *param)
{
	struct camera_frame *frame;

	if (!param) {
		pr_err("error: null input ptr.\n");
		return;
	}

	frame = (struct camera_frame *)param;
	put_empty_frame(frame);
}


static struct camera_buf *get_reserved_buffer(struct dcam_pipe_dev *dev)
{
	int ret = 0;
	int iommu_enable = 0; /* todo: get from dev dts config value */
	size_t size;
	struct camera_buf *ion_buf = NULL;

	ion_buf = kzalloc(sizeof(*ion_buf), GFP_KERNEL);
	if (!ion_buf) {
		pr_err("fail to alloc buffer.\n");
		goto nomem;
	}

	if (get_iommu_status(CAM_IOMMUDEV_DCAM) == 0)
		iommu_enable = 1;

	size = DCAM_INTERNAL_RES_BUF_SIZE;
	ret = cambuf_alloc(ion_buf, size, 0, iommu_enable);
	if (ret) {
		pr_err("fail to get dcam reserverd buffer\n");
		goto ion_fail;
	}

	ret = cambuf_kmap(ion_buf);
	if (ret) {
		pr_err("fail to kmap dcame reserved buffer\n");
		goto kmap_fail;
	}

	ret = cambuf_iommu_map(ion_buf, CAM_IOMMUDEV_DCAM);
	if (ret) {
		pr_err("fail to map dcam reserved buffer to iommu\n");
		goto hwmap_fail;
	}
	pr_info("dcam%d done. ion %p\n", dev->idx, ion_buf);
	return ion_buf;

hwmap_fail:
	cambuf_kunmap(ion_buf);
kmap_fail:
	cambuf_free(ion_buf);
ion_fail:
	kfree(ion_buf);
nomem:
	return NULL;
}

static int put_reserved_buffer(struct dcam_pipe_dev *dev)
{
	struct camera_buf *ion_buf = NULL;

	ion_buf = (struct camera_buf *)dev->internal_reserved_buf;
	if (!ion_buf) {
		pr_info("no reserved buffer.\n");
		return 0;
	}
	pr_info("ionbuf %p\n", ion_buf);

	cambuf_iommu_unmap(ion_buf);
	cambuf_kunmap(ion_buf);
	cambuf_free(ion_buf);
	kfree(ion_buf);
	dev->internal_reserved_buf = NULL;

	return 0;
}
struct statis_path_buf_info *dcam_get_statis_distribution_array(void)
{
	return &s_statis_path_info_all[0];
}

int dcam_get_statis_distribution_size(void)
{
	return ARRAY_SIZE(s_statis_path_info_all);
}

static int statis_type_to_path_id(enum isp_statis_buf_type type)
{
	switch (type) {
	case STATIS_AEM:
		return DCAM_PATH_AEM;
	case STATIS_AFM:
		return DCAM_PATH_AFM;
	case STATIS_AFL:
		return DCAM_PATH_AFL;
	case STATIS_HIST:
		return DCAM_PATH_HIST;
	case STATIS_PDAF:
		return DCAM_PATH_PDAF;
	case STATIS_EBD:
		return DCAM_PATH_VCH2;
	case STATIS_3DNR:
		return DCAM_PATH_3DNR;
	default:
		return -1;
	}
}

static int path_id_to_statis_type(enum dcam_path_id path_id)
{
	switch (path_id) {
	case DCAM_PATH_AEM:
		return STATIS_AEM;
	case DCAM_PATH_AFM:
		return STATIS_AFM;
	case DCAM_PATH_AFL:
		return STATIS_AFL;
	case DCAM_PATH_HIST:
		return STATIS_HIST;
	case DCAM_PATH_PDAF:
		return STATIS_PDAF;
	case DCAM_PATH_VCH2:
		return STATIS_EBD;
	case DCAM_PATH_3DNR:
		return STATIS_3DNR;
	default:
		return -1;
	}
}

static void init_reserved_statis_bufferq(struct dcam_pipe_dev *dev)
{
	int i, j;
	struct camera_frame *newfrm;
	enum dcam_path_id path_id;
	struct camera_buf *ion_buf = NULL;
	struct dcam_path_desc *path;

	pr_debug("enter\n");

	if (dev->internal_reserved_buf == NULL) {
		ion_buf =  get_reserved_buffer(dev);
		if (IS_ERR_OR_NULL(ion_buf))
			return;

		dev->internal_reserved_buf = ion_buf;
	}

	for (i = 0; i < (int)ARRAY_SIZE(s_statis_path_info_all); i++) {
		path_id = s_statis_path_info_all[i].path_id;
		/* pdaf needs large buffer, no reserved for it */
		if (path_id == DCAM_PATH_PDAF)
			continue;
		/* DCAM_PATH_MAX for ISP statis */
		if (path_id == DCAM_PATH_MAX)
			continue;
		path = &dev->path[path_id];
		j  = 0;
		while (j < DCAM_RESERVE_BUF_Q_LEN) {
			newfrm = get_empty_frame();
			if (newfrm) {
				newfrm->is_reserved = 1;
				memcpy(&newfrm->buf, ion_buf, sizeof(struct camera_buf));
				camera_enqueue(&path->reserved_buf_queue, newfrm);
				j++;
			}
			pr_debug("path%d reserved buffer %d\n", path->path_id, j);
		}
	}

	pr_debug("init statis reserver bufq done: %p\n", ion_buf);
}

static int init_statis_bufferq(struct dcam_pipe_dev *dev)
{
	int ret = 0;
	int i;
	int j;
	int count = 0;
	size_t used_size = 0, total_size;
	size_t buf_size;
	unsigned long kaddr;
	unsigned long paddr;
	unsigned long uaddr;
	enum dcam_path_id path_id;
	enum isp_statis_buf_type stats_type;
	struct camera_buf *ion_buf;
	struct camera_frame *pframe;
	struct dcam_path_desc *path;

	pr_debug("enter\n");

	for (i = 0; i < ARRAY_SIZE(s_statis_path_info_all); i++) {
		path_id = s_statis_path_info_all[i].path_id;

		/* DCAM_PATH_MAX for ISP statis */
		if (path_id == DCAM_PATH_MAX)
			continue;

		path = &dev->path[path_id];
		camera_queue_init(&path->out_buf_queue,
				DCAM_OUT_BUF_Q_LEN, 0, dcam_destroy_statis_buf);
		camera_queue_init(&path->result_queue,
				DCAM_RESULT_Q_LEN, 0, dcam_destroy_statis_buf);
		camera_queue_init(&path->reserved_buf_queue,
				DCAM_RESERVE_BUF_Q_LEN, 0,
					dcam_destroy_statis_buf);
	}

	init_reserved_statis_bufferq(dev);

	ion_buf = dev->statis_buf;
	if (ion_buf == NULL) {
		pr_info("dcam%d no init statis buf.\n", dev->idx);
		return ret;
	}

	kaddr = ion_buf->addr_k[0];
	paddr = ion_buf->iova[0];
	uaddr = ion_buf->addr_vir[0];
	total_size = ion_buf->size[0];

	pr_debug("size %d  addr %p %p,  %08x\n", (int)total_size,
			(void *)kaddr, (void *)uaddr, (uint32_t)paddr);

	for (i = 0; i < ARRAY_SIZE(s_statis_path_info_all); i++) {

		path_id = s_statis_path_info_all[i].path_id;
		buf_size = s_statis_path_info_all[i].buf_size;

		/* DCAM_PATH_MAX for ISP statis */
		if (path_id == DCAM_PATH_MAX)
			continue;

		path = &dev->path[path_id];
		stats_type = path_id_to_statis_type(path_id);

		for (j = 0; j < s_statis_path_info_all[i].buf_cnt; j++) {
			used_size += buf_size;
			if (used_size >= total_size)
				break;

			pframe = get_empty_frame();
			if (pframe) {
				pframe->channel_id = path_id;
				pframe->irq_property = stats_type;
				pframe->buf.addr_vir[0] = uaddr;
				pframe->buf.addr_k[0] = kaddr;
				pframe->buf.iova[0] = paddr;
				pframe->buf.size[0] = buf_size;
				camera_enqueue(&path->out_buf_queue, pframe);
				uaddr += buf_size;
				kaddr += buf_size;
				paddr += buf_size;
			}

			pr_debug("dcam path[%d] i[%d] j[%d] uaddr[%lx] kaddr[%lx] paddr[%lx]\n",
				path_id,
				i,
				j,
				uaddr,
				kaddr,
				paddr);

		}
		count++;
	}

	pr_info("done.\n");
	return ret;
}

static int deinit_statis_bufferq(struct dcam_pipe_dev *dev)
{
	int ret = 0;
	int i;
	enum dcam_path_id path_id;
	struct dcam_path_desc *path;

	pr_debug("enter\n");

	for (i = 0; i < ARRAY_SIZE(s_statis_path_info_all); i++) {
		path_id = s_statis_path_info_all[i].path_id;
		path = &dev->path[path_id];

		pr_debug("path_id[%d] i[%d]\n", path_id, i);

		if (path_id == DCAM_PATH_MAX)
			continue;

		atomic_set(&path->user_cnt, 0);
		camera_queue_clear(&path->out_buf_queue);
		camera_queue_clear(&path->result_queue);
		camera_queue_clear(&path->reserved_buf_queue);
	}
	pr_info("done.\n");

	return ret;
}


static int unmap_statis_buffer(struct dcam_pipe_dev *dev)
{
	struct camera_buf *ion_buf = NULL;

	ion_buf = dev->statis_buf;
	if (!ion_buf) {
		pr_info("no statis buffer.\n");
		return 0;
	}
	pr_info("%p\n", ion_buf);

	cambuf_iommu_unmap(ion_buf);
	cambuf_put_ionbuf(ion_buf);
	kfree(ion_buf);
	dev->statis_buf = NULL;
	pr_info("done.\n");

	return 0;
}

static int dcam_cfg_statis_buffer(
		struct dcam_pipe_dev *dev,
		struct isp_statis_buf_input *input)
{
	int ret = 0;
	int path_id;
	struct camera_frame *pframe;
	struct camera_buf *ion_buf = NULL;

	if (input->type == STATIS_INIT) {
		ion_buf = kzalloc(sizeof(*ion_buf), GFP_KERNEL);

		if (IS_ERR_OR_NULL(ion_buf)) {
			pr_err("fail to alloc memory for dcam%d statis buf.\n",
					dev->idx);
			ret = -ENOMEM;
			goto exit;
		}

		ion_buf->mfd[0] = input->u.init_data.mfd;
		ion_buf->size[0] = input->u.init_data.buf_size;
		ion_buf->addr_vir[0] = (unsigned long)input->uaddr;
		ion_buf->addr_k[0] = (unsigned long)input->kaddr;
		ion_buf->type = CAM_BUF_USER;

		ret = cambuf_get_ionbuf(ion_buf);
		if (ret) {
			kfree(ion_buf);
			ret = -EINVAL;
			goto exit;
		}

		ret = cambuf_iommu_map(ion_buf, CAM_IOMMUDEV_DCAM);
		if (ret) {
			pr_err("fail to map dcam statis buffer to iommu\n");
			cambuf_put_ionbuf(ion_buf);
			kfree(ion_buf);
			ret = -EINVAL;
			goto exit;
		}
		dev->statis_buf = ion_buf;

		pr_debug("map dcam statis buffer. mfd: %d, size: 0x%x\n",
			ion_buf->mfd[0], (int)ion_buf->size[0]);
		pr_debug("uaddr: %p, kaddr: %p,  iova: 0x%x\n",
				(void *)ion_buf->addr_vir[0],
				(void *)ion_buf->addr_k[0],
				(uint32_t)ion_buf->iova[0]);
	} else if (atomic_read(&dev->state) == STATE_RUNNING) {
		path_id = statis_type_to_path_id(input->type);
		if (path_id < 0) {
			pr_err("invalid statis type: %d\n", input->type);
			ret = -EINVAL;
			goto exit;
		}
		pframe = get_empty_frame();
		pframe->irq_property = input->type;
		pframe->buf.addr_vir[0] = (unsigned long)input->uaddr;
		pframe->buf.addr_k[0] = (unsigned long)input->kaddr;
		pframe->buf.iova[0] = input->u.block_data.hw_addr;
		ret = camera_enqueue(&dev->path[path_id].out_buf_queue, pframe);
		pr_debug("statis type %d, iova 0x%08x,  uaddr 0x%lx\n",
			input->type, (uint32_t)pframe->buf.iova[0],
			pframe->buf.addr_vir[0]);
		if (ret)
			put_empty_frame(pframe);
	}
exit:
	return ret;
}

/* dcam_offline_cfg_param
 * after param prepare
 * Input: param
 * unused
 */
int dcam_offline_cfg_param(struct dcam_dev_param *pm)
{
	FUNC_DCAM_PARAM func = NULL;
	const FUNC_DCAM_PARAM all_sub[] = {
		dcam_k_blc_block, dcam_k_rgb_gain_block,
		dcam_k_rgb_dither_random_block,
		dcam_k_lsc_block, dcam_k_bayerhist_block, dcam_k_aem_bypass,
		dcam_k_aem_mode, dcam_k_aem_win, dcam_k_aem_skip_num,
		dcam_k_aem_rgb_thr, dcam_k_afl_block, dcam_k_afl_bypass,
		dcam_k_awbc_block, dcam_k_awbc_gain, dcam_k_awbc_block,
		dcam_k_bpc_block,
		dcam_k_bpc_ppe_param, dcam_k_3dnr_me, dcam_k_afm_block,
		dcam_k_afm_bypass, dcam_k_afm_win, dcam_k_afm_win_num,
		dcam_k_afm_mode, dcam_k_afm_skipnum, dcam_k_afm_crop_eb,
		dcam_k_afm_crop_size, dcam_k_afm_done_tilenum,
	};
	int i;
	int ret = 0, t = 0;

	if (pm == NULL) {
		pr_err("dcam param error(pm == NULL)\n");
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(all_sub); i++) {
		func = all_sub[i];
		if (func)
			t = func(pm);
		if (t)
			pr_warn("set param fail, sub block %d\n", i);

		ret |= t;
	}

	return ret;
}

/* TODO: need to be refined */
static int dcam_offline_start_frame(void *param)
{
	int ret = 0;
	int i, loop;
	uint32_t force_ids = DCAM_CTRL_ALL;
	struct dcam_pipe_dev *dev = NULL;
	struct camera_frame *pframe = NULL;
	struct dcam_path_desc *path;
	struct dcam_fetch_info *fetch;

	pr_debug("enter.\n");

	dev = (struct dcam_pipe_dev *)param;
	dev->offline = 1;
	fetch = &dev->fetch;

	pframe = camera_dequeue(&dev->in_queue);
	if (pframe == NULL) {
		pr_warn("no frame from in_q. dcam%d\n", dev->idx);
		return 0;
	}

	pr_debug("frame %p, ctx %d  ch_id %d.  buf_fd %d\n", pframe,
		dev->idx, pframe->channel_id, pframe->buf.mfd[0]);

	ret = cambuf_iommu_map(&pframe->buf, CAM_IOMMUDEV_DCAM);
	if (ret) {
		pr_err("fail to map buf to dcam%d iommu.\n", dev->idx);
		goto map_err;
	}

	loop = 0;
	do {
		ret = camera_enqueue(&dev->proc_queue, pframe);
		if (ret == 0)
			break;
		pr_info("wait for proc queue. loop %d\n", loop);

		/* wait for previous frame proccessed done. */
		mdelay(1);
	} while (loop++ < 500);

	if (ret) {
		pr_err("error: input frame queue tmeout.\n");
		ret = -EINVAL;
		goto inq_overflow;
	}

	/* todo: enable statis path from user config */
	pr_debug("hwsim:rps[%d]\n",dev->rps);

	if (dev->rps == 1) {
		pr_debug("hwsim:offline enable aem\n");
		atomic_set(&dev->path[DCAM_PATH_AEM].user_cnt, 1); /* hwsim first loop need aem statis */
	} else {
		atomic_set(&dev->path[DCAM_PATH_AEM].user_cnt, 0);
	}
	atomic_set(&dev->path[DCAM_PATH_AFM].user_cnt, 0);
	atomic_set(&dev->path[DCAM_PATH_AFL].user_cnt, 0);
	atomic_set(&dev->path[DCAM_PATH_HIST].user_cnt, 0);

	/* prepare frame info for tx done
	 * ASSERT: this dev has no cap_sof
	 */
	dev->index_to_set = pframe->fid;
	if (pframe->sensor_time.tv_sec || pframe->sensor_time.tv_usec) {
		dev->frame_ts[tsid(pframe->fid)].tv_sec =
			pframe->sensor_time.tv_sec;
		dev->frame_ts[tsid(pframe->fid)].tv_nsec =
			pframe->sensor_time.tv_usec * NSEC_PER_USEC;
		dev->frame_ts_boot[tsid(pframe->fid)] = pframe->boot_sensor_time;
		pr_info("frame[%d]\n", pframe->fid);
	}

	for (i  = 0; i < DCAM_PATH_MAX; i++) {
		path = &dev->path[i];
		if (atomic_read(&path->user_cnt) < 1)
			continue;
		ret = dcam_path_set_store_frm(dev, path, NULL); /* TODO: */
		if (ret == 0) {
			/* interrupt need > 1 */
			atomic_set(&path->set_frm_cnt, 1);
			atomic_inc(&path->set_frm_cnt);
			dcam_start_path(dev, path);
		}
	}

	/* todo - need to cfg fetch param from input or frame. */
	fetch->is_loose = 0;
	fetch->endian = pframe->irq_type;
	fetch->pattern = pframe->irq_property;
	fetch->size.w = pframe->width;
	fetch->size.h = pframe->height;
	fetch->trim.start_x = 0;
	fetch->trim.start_y = 0;
	fetch->trim.size_x = pframe->width;
	fetch->trim.size_y = pframe->height;
	fetch->addr.addr_ch0 = (uint32_t)pframe->buf.iova[0];

	ret = dcam_set_fetch(dev, fetch);

	dcam_init_lsc(dev, 0);

	/* DCAM_CTRL_COEF will always set in dcam_init_lsc() */
	force_ids &= ~DCAM_CTRL_COEF;
	dcam_force_copy(dev, force_ids);
	udelay(500);

	dev->iommu_status = (uint32_t)(-1);
	atomic_set(&dev->state, STATE_RUNNING);
	dcam_debug_trace(dev);

	if (dev->dcamsec_eb)
		pr_warn("camca : dcamsec_eb= %d, fetch disable\n", dev->dcamsec_eb);
	else
		dcam_start_fetch();

	return ret;

inq_overflow:
	cambuf_iommu_unmap(&pframe->buf);
map_err:
	/* return buffer to cam channel shared buffer queue. */
	dev->dcam_cb_func(DCAM_CB_RET_SRC_BUF, pframe, dev->cb_priv_data);
	return ret;
}

static int dcam_offline_thread_loop(void *arg)
{
	struct dcam_pipe_dev *dev = NULL;
	struct cam_thread_info *thrd;
	uint32_t idx = 0;

	if (!arg) {
		pr_err("fail to get valid input ptr\n");
		return -1;
	}

	thrd = (struct cam_thread_info *)arg;
	dev = (struct dcam_pipe_dev *)thrd->ctx_handle;
	idx = dev->idx;

	while (1) {
		if (wait_for_completion_interruptible(
			&thrd->thread_com) == 0) {
			if (atomic_cmpxchg(
					&thrd->thread_stop, 1, 0) == 1) {
				pr_info("dcam%d offline thread stop.\n", idx);
				break;
			}
			pr_debug("thread com done.\n");

			if (thrd->proc_func(dev)) {
				pr_err(
					"fail to start dcam pipe to proc. exit thread\n");
				dev->dcam_cb_func(
					DCAM_CB_DEV_ERR, dev,
					dev->cb_priv_data);
				break;
			}
		} else {
			pr_debug("offline thread exit!");
			break;
		}
	}

	return 0;
}

static int dcam_stop_offline_thread(void *param)
{
	int cnt = 0;
	struct cam_thread_info *thrd;

	thrd = (struct cam_thread_info *)param;

	if (thrd->thread_task) {
		atomic_set(&thrd->thread_stop, 1);
		complete(&thrd->thread_com);
		while (cnt < 1000) {
			cnt++;
			if (atomic_read(&thrd->thread_stop) == 0)
				break;
			udelay(1000);
		}
		thrd->thread_task = NULL;
		pr_info("offline thread stopped. wait %d ms\n", cnt);
	}

	return 0;
}

static int dcam_create_offline_thread(void *param)
{
	struct dcam_pipe_dev *dev;
	struct cam_thread_info *thrd;
	char thread_name[32] = { 0 };

	dev = (struct dcam_pipe_dev *)param;
	thrd = &dev->thread;
	thrd->ctx_handle = dev;
	thrd->proc_func = dcam_offline_start_frame;
	atomic_set(&thrd->thread_stop, 0);
	init_completion(&thrd->thread_com);

	sprintf(thread_name, "dcam%d_offline", dev->idx);
	thrd->thread_task = kthread_run(dcam_offline_thread_loop,
					thrd, thread_name);
	if (IS_ERR_OR_NULL(thrd->thread_task)) {
		pr_err("fail to start offline thread for dcam%d\n",
				dev->idx);
		return -EFAULT;
	}

	pr_info("dcam%d offline thread created.\n", dev->idx);
	return 0;
}

/*
 * Helper function to initialize dcam_sync_helper.
 */
static inline void
_init_sync_helper_locked(struct dcam_pipe_dev *dev,
			 struct dcam_sync_helper *helper)
{
	memset(&helper->sync, 0, sizeof(struct dcam_frame_synchronizer));
	helper->enabled = 0;
	helper->dev = dev;
}

/*
 * Initialize frame synchronizer helper.
 */
static inline int dcam_init_sync_helper(struct dcam_pipe_dev *dev)
{
	unsigned long flags = 0;
	int i = 0;

	INIT_LIST_HEAD(&dev->helper_list);

	spin_lock_irqsave(&dev->helper_lock, flags);
	list_empty(&dev->helper_list);
	for (i = 0; i < DCAM_SYNC_HELPER_COUNT; i++) {
		_init_sync_helper_locked(dev, &dev->helpers[i]);
		list_add_tail(&dev->helpers[i].list, &dev->helper_list);
	}
	spin_unlock_irqrestore(&dev->helper_lock, flags);

	return 0;
}

/*
 * Enables/Disables frame sync for path_id. Should be called before streaming.
 */
int dcam_if_set_sync_enable(void *handle, int path_id, int enable)
{
	struct dcam_pipe_dev *dev = NULL;
	int ret = 0;

	if (unlikely(!handle)) {
		pr_err("invalid param handle\n");
		return -EINVAL;
	}
	dev = (struct dcam_pipe_dev *)handle;

	if (unlikely(!is_path_id(path_id))) {
		pr_err("invalid param path_id: %d\n", path_id);
		return -EINVAL;
	}

	ret = atomic_read(&dev->state);
	if (unlikely(ret != STATE_INIT && ret != STATE_IDLE)) {
		pr_err("cannot enable DCAM%u frame sync in %d state\n",
			dev->idx, ret);
		return -EINVAL;
	}

	if (enable)
		dev->helper_enabled |= BIT(path_id);
	else
		dev->helper_enabled &= ~BIT(path_id);

	pr_info("DCAM%u %s %s frame sync\n", dev->idx,
		to_path_name(path_id), enable ? "enable" : "disable");

	return 0;
}

/*
 * Helper function to put dcam_sync_helper.
 */
static inline void
_put_sync_helper_locked(struct dcam_pipe_dev *dev,
			struct dcam_sync_helper *helper)
{
	_init_sync_helper_locked(dev, helper);
	list_add_tail(&helper->list, &dev->helper_list);
}

/*
 * Release frame sync reference for @frame thus dcam_frame_synchronizer data
 * can be recycled for next use.
 */
int dcam_if_release_sync(struct dcam_frame_synchronizer *sync,
			 struct camera_frame *frame)
{
	struct dcam_sync_helper *helper = NULL;
	struct dcam_pipe_dev *dev = NULL;
	unsigned long flags = 0;
	int ret = 0, path_id = 0;
	bool ignore = false;

	if (unlikely(!sync || !frame)) {
		pr_err("invalid param\n");
		return -EINVAL;
	}

	helper = container_of(sync, struct dcam_sync_helper, sync);
	dev = helper->dev;

	for (path_id = 0; path_id < DCAM_PATH_MAX; path_id++) {
		if (frame == sync->frames[path_id])
			break;
	}

	if (unlikely(!is_path_id(path_id))) {
		pr_err("DCAM%u can't find path id, fid %u, sync %u\n",
		       dev->idx, frame->fid, sync->index);
		return -EINVAL;
	}

	/* unbind each other */
	frame->sync_data = NULL;
	sync->frames[path_id] = NULL;

	pr_debug("DCAM%u %s release sync, id %u, data 0x%p\n",
		dev->idx, to_path_name(path_id), sync->index, sync);

	spin_lock_irqsave(&dev->helper_lock, flags);
	if (unlikely(!helper->enabled)) {
		ignore = true;
		goto exit;
	}
	helper->enabled &= ~BIT(path_id);
	if (!helper->enabled)
		_put_sync_helper_locked(dev, helper);

exit:
	spin_unlock_irqrestore(&dev->helper_lock, flags);

	if (ignore)
		pr_warn("ignore not enabled sync helper\n");

	return ret;
}

/*
 * Get an empty dcam_sync_helper. Returns NULL if no empty helper remains.
 */
struct dcam_sync_helper *dcam_get_sync_helper(struct dcam_pipe_dev *dev)
{
	struct dcam_sync_helper *helper = NULL;
	unsigned long flags = 0;
	bool running_low = false;

	if (unlikely(!dev)) {
		pr_err("invalid param dev\n");
		return NULL;
	}

	spin_lock_irqsave(&dev->helper_lock, flags);
	if (unlikely(list_empty(&dev->helper_list))) {
		running_low = true;
		goto exit;
	}

	helper = list_first_entry(&dev->helper_list,
				  struct dcam_sync_helper, list);
	list_del_init(&helper->list);

exit:
	spin_unlock_irqrestore(&dev->helper_lock, flags);

	if (running_low)
		pr_warn_ratelimited("DCAM%u helper is running low...\n",
				    dev->idx);

	return helper;
}

/*
 * Put an empty dcam_sync_helper.
 *
 * In CAP_SOF, when the requested empty dcam_sync_helper finally not being used,
 * it should be returned to FIFO. This only happens when no buffer is
 * available and all paths are using reserved memory.
 *
 * This is also an code defect in CAP_SOF that should be changed later...
 */
void dcam_put_sync_helper(struct dcam_pipe_dev *dev,
				struct dcam_sync_helper *helper)
{
	unsigned long flags = 0;

	if (unlikely(!dev)) {
		pr_err("invalid param dev\n");
		return;
	}

	spin_lock_irqsave(&dev->helper_lock, flags);
	_put_sync_helper_locked(dev, helper);
	spin_unlock_irqrestore(&dev->helper_lock, flags);
}

/* config fbc, see dcam_interface.h for @fbc_mode */
static int dcam_cfg_fbc(struct dcam_pipe_dev *dev, int fbc_mode)
{
	struct dcam_path_desc *path = NULL;
	struct camera_frame *frame = NULL;

	DCAM_REG_MWR(dev->idx, DCAM_PATH_ENDIAN, 0x3, fbc_mode);
	pr_info("fbc mode %d\n", fbc_mode);

	/* update compressed flag for reserved buffer */
	if (fbc_mode == DCAM_FBC_FULL)
		path = &dev->path[DCAM_PATH_FULL];
	else if (fbc_mode == DCAM_PATH_BIN)
		path = &dev->path[DCAM_PATH_BIN];

	if (!path)
		return 0;

	/* bad code, but don't have any other choice */
	list_for_each_entry(frame, &path->reserved_buf_queue.head, list) {
		frame->is_compressed = 1;
	}

	return 0;
}

static int sprd_dcam_get_path(
	void *dcam_handle, int path_id)
{
	struct dcam_pipe_dev *dev;
	struct dcam_path_desc *path = NULL;

	if (!dcam_handle) {
		pr_err("error input param: %p\n",
				dcam_handle);
		return -EFAULT;
	}
	if (path_id < DCAM_PATH_FULL || path_id >= DCAM_PATH_MAX) {
		pr_err("error dcam path id %d\n", path_id);
		return -EFAULT;
	}

	dev = (struct dcam_pipe_dev *)dcam_handle;
	path = &dev->path[path_id];
	if (atomic_inc_return(&path->user_cnt) > 1) {
		atomic_dec(&path->user_cnt);
		pr_err("error: dcam%d path %d in use.\n", dev->idx, path_id);
		return -EFAULT;
	}

	camera_queue_init(&path->result_queue, DCAM_RESULT_Q_LEN,
					0, dcam_ret_out_frame);
	camera_queue_init(&path->out_buf_queue, DCAM_OUT_BUF_Q_LEN,
					0, dcam_ret_out_frame);
	camera_queue_init(&path->reserved_buf_queue, DCAM_RESERVE_BUF_Q_LEN,
					0, dcam_destroy_reserved_buf);

	return 0;
}

static int sprd_dcam_put_path(
	void *dcam_handle, int path_id)
{
	int ret = 0;
	struct dcam_pipe_dev *dev;
	struct dcam_path_desc *path = NULL;

	if (!dcam_handle) {
		pr_err("error input param: %p\n",
				dcam_handle);
		return -EFAULT;
	}
	if (path_id < DCAM_PATH_FULL || path_id >= DCAM_PATH_MAX) {
		pr_err("error dcam path id %d\n", path_id);
		return -EFAULT;
	}

	dev = (struct dcam_pipe_dev *)dcam_handle;
	path = &dev->path[path_id];

	if (atomic_read(&path->user_cnt) == 0) {
		pr_err("dcam%d path %d is not in use.\n",
					dev->idx, path_id);
		return -EFAULT;
	}

	if (atomic_dec_return(&path->user_cnt) != 0) {
		pr_warn("dcam%d path %d has multi users.\n",
					dev->idx, path_id);
		atomic_set(&path->user_cnt, 0);
	}

	camera_queue_clear(&path->result_queue);
	camera_queue_clear(&path->out_buf_queue);
	camera_queue_clear(&path->reserved_buf_queue);

	pr_info("put dcam%d path %d done\n", dev->idx, path_id);
	return ret;
}

static int  dcam_cfg_path_full_source(void *dcam_handle,
				struct dcam_path_desc *path,
				void *param)
{
	struct dcam_pipe_dev *dev = (struct dcam_pipe_dev *)dcam_handle;
	uint32_t lowlux_4in1 = *(uint32_t *)param;
	static const char *tb_src[] = {"(4c)raw", "bin-sum"}; /* for log */

	if (lowlux_4in1) {
		dev->lowlux_4in1 = 1;
		DCAM_REG_MWR(dev->idx, DCAM_FULL_CFG, BIT(2), BIT(2));
		dev->skip_4in1 = 1; /* auto copy, so need skip 1 frame */
	} else {
		dev->lowlux_4in1 = 0;
		DCAM_REG_MWR(dev->idx, DCAM_FULL_CFG, BIT(2), 0);
		dev->skip_4in1 = 1;
	}
	pr_info("dev%d lowlux %d, skip_4in1 %d, full src: %s\n", dev->idx,
		dev->lowlux_4in1, dev->skip_4in1, tb_src[lowlux_4in1]);

	return 0;
}

static inline void sprd_dcam_show_frame_info(struct dcam_pipe_dev *dev,
					     struct dcam_path_desc *path,
					     struct camera_frame *frame)
{
	uint32_t size = 0;

	if (frame->is_compressed)
		size = dcam_if_cal_compressed_size(frame->width, frame->height);
	else
		size = cal_sprd_raw_pitch(frame->width) * frame->height;

	pr_info("DCAM%u %s frame %u %u size %u %u buf %08lx %08x\n",
		dev->idx, to_path_name(path->path_id),
		frame->is_reserved, frame->is_compressed,
		frame->width, frame->width,
		frame->buf.iova[0], size);
}

static int sprd_dcam_cfg_path(
	void *dcam_handle,
	enum dcam_path_cfg_cmd cfg_cmd,
	int path_id, void *param)
{
	int ret = 0;
	struct dcam_pipe_dev *dev;
	struct dcam_path_desc *path;
	struct camera_frame *pframe;

	if (!dcam_handle || !param) {
		pr_err("error input param: %p, %p\n",
				dcam_handle, param);
		return -EFAULT;
	}
	if (path_id < DCAM_PATH_FULL || path_id >= DCAM_PATH_MAX) {
		pr_err("error dcam path id %d\n", path_id);
		return -EFAULT;
	}

	dev = (struct dcam_pipe_dev *)dcam_handle;
	path = &dev->path[path_id];

	if (atomic_read(&path->user_cnt) == 0) {
		pr_err("dcam%d, path %d is not in use.\n",
			dev->idx, path_id);
		return -EFAULT;
	}

	switch (cfg_cmd) {
	case DCAM_PATH_CFG_OUTPUT_RESERVED_BUF:
	case DCAM_PATH_CFG_OUTPUT_BUF:
		pframe = (struct camera_frame *)param;
		ret = cambuf_iommu_map(&pframe->buf, CAM_IOMMUDEV_DCAM);
		if (ret)
			goto exit;

		if (atomic_read(&dev->state) != STATE_RUNNING)
			sprd_dcam_show_frame_info(dev, path, pframe);

		/* is_reserved:
		 *  1:  basic mapping reserved buffer;
		 *  2:  copy of reserved buffer.
		 */
		if (unlikely(cfg_cmd == DCAM_PATH_CFG_OUTPUT_RESERVED_BUF)) {
			int i = 1;
			struct camera_frame *newfrm;

			pframe->is_reserved = 1;
			pframe->priv_data = path;
			ret = camera_enqueue(&path->reserved_buf_queue, pframe);
			if (ret) {
				pr_err("dcam path %d reserve buffer en queue failed\n",
					path_id);
				cambuf_iommu_unmap(&pframe->buf);
				goto exit;
			}

			pr_info("config dcam output reserverd buffer.\n");

			while (i < DCAM_RESERVE_BUF_Q_LEN) {
				newfrm = get_empty_frame();
				if (newfrm) {
					newfrm->is_reserved = 2;
					newfrm->priv_data = path;
					memcpy(&newfrm->buf, &pframe->buf,
						sizeof(pframe->buf));
					ret = camera_enqueue(
						&path->reserved_buf_queue,
						newfrm);
					i++;
				}
			}
		} else {
			pframe->is_reserved = 0;
			pframe->priv_data = dev;
			ret = camera_enqueue(&path->out_buf_queue, pframe);
			if (ret) {
				pr_err("dcam path %d output buffer en queue failed\n",
					path_id);
				cambuf_iommu_unmap(&pframe->buf);
				goto exit;
			}
			pr_debug("config dcam output buffer.\n");
		}
		break;

	case DCAM_PATH_CFG_SIZE:
		ret = dcam_cfg_path_size(dev, path, param);
		break;

	case DCAM_PATH_CFG_BASE:
		ret = dcam_cfg_path_base(dev, path, param);
		break;
	case DCAM_PATH_CFG_FULL_SOURCE:
		ret = dcam_cfg_path_full_source(dev, path, param);
	default:
		pr_warn("unsupported command: %d\n", cfg_cmd);
		break;
	}
exit:
	return ret;
}

/* offline process frame */
static int sprd_dcam_proc_frame(
		void *dcam_handle,  void *param)
{
	int ret = 0;
	struct dcam_pipe_dev *dev = NULL;
	struct camera_frame *pframe;

	if (!dcam_handle || !param) {
		pr_err("fail to get valid input ptr\n");
		return -EFAULT;
	}
	dev = (struct dcam_pipe_dev *)dcam_handle;

	pr_debug("dcam%d offline proc frame!\n", dev->idx);
	/* if enable, 4in1 capture once more then dcam1 can't run
	 * if (atomic_read(&dev->state) == STATE_RUNNING) {
	 *	pr_err("DCAM%u started for online\n", dev->idx);
	 *	return -EFAULT;
	 * }
	 */
	pframe = (struct camera_frame *)param;
	pframe->priv_data = dev;
	ret = camera_enqueue(&dev->in_queue, pframe);
	if (ret == 0)
		complete(&dev->thread.thread_com);
	else
		pr_err("enqueue to dev->in_queue fail, ret = %d\n", ret);

	return ret;
}

static int sprd_dcam_ioctrl(void *dcam_handle,
	enum dcam_ioctrl_cmd cmd, void *param)
{
	int ret = 0;
	struct dcam_pipe_dev *dev = NULL;
	struct dcam_mipi_info *cap;
	int *fbc_mode;

	if (!dcam_handle) {
		pr_err("fail to get valid input ptr\n");
		return -EFAULT;
	}
	dev = (struct dcam_pipe_dev *)dcam_handle;

	if (unlikely(atomic_read(&dev->state) == STATE_INIT)) {
		pr_err("DCAM%d is not initialized\n", dev->idx);
		return -EINVAL;
	}

	switch (cmd) {
	case DCAM_IOCTL_CFG_CAP:
		cap = &dev->cap_info;
		memcpy(cap, param, sizeof(struct dcam_mipi_info));
		dev->is_4in1 = cap->is_4in1;
		break;
	case DCAM_IOCTL_CFG_STATIS_BUF:
		ret = dcam_cfg_statis_buffer(dev, param);
		break;
	case DCAM_IOCTL_CFG_START:
		/* sign of isp mw starting to config block param. */
		ret = cambuf_iommu_map(
				&dev->blk_dcam_pm->lsc.buf,
				CAM_IOMMUDEV_DCAM);
		if (ret)
			dev->blk_dcam_pm->lsc.buf.iova[0] = 0L;
		break;
	case DCAM_IOCTL_CFG_STOP:
		cambuf_iommu_unmap(&dev->blk_dcam_pm->lsc.buf);
		break;
	case DCAM_IOCTL_INIT_STATIS_Q:
		ret = init_statis_bufferq(dev);
		break;
	case DCAM_IOCTL_DEINIT_STATIS_Q:
		ret = deinit_statis_bufferq(dev);
		put_reserved_buffer(dev);
		unmap_statis_buffer(dev);
		break;
	case DCAM_IOCTL_CFG_EBD:
		ret = dcam_cfg_ebd(dev, param);
	case DCAM_IOCTL_CFG_SEC:
		ret = dcam_cfg_dcamsec(dev, param);
		break;
	case DCAM_IOCTL_CFG_FBC:
		fbc_mode = (int *)param;
		if (fbc_mode)
			ret = dcam_cfg_fbc(dev, *fbc_mode);
		break;
	case DCAM_IOCTL_CFG_RPS:
		ret = dcam_cfg_rps(dev, param);
		break;
	case DCAM_IOCTL_CFG_REPLACER:
		dev->replacer = (struct dcam_image_replacer *)param;
		break;
	default:
		pr_err("error: unknown cmd: %d\n", cmd);
		ret = -EFAULT;
		break;
	}

	return ret;
}

typedef int (*func_dcam_cfg_param)(struct isp_io_param *param,
				struct dcam_dev_param *p);

struct cfg_entry {
	uint32_t sub_block;
	func_dcam_cfg_param cfg_func;
};

/* todo: enable block config one by one */
/* because parameters from user may be illegal when bringup*/
static struct cfg_entry cfg_func_tab[DCAM_BLOCK_TOTAL] = {
[DCAM_BLOCK_BLC - DCAM_BLOCK_BASE]     = {DCAM_BLOCK_BLC,              dcam_k_cfg_blc},
[DCAM_BLOCK_RGBG - DCAM_BLOCK_BASE]    = {DCAM_BLOCK_RGBG,             dcam_k_cfg_rgb_gain},
[DCAM_BLOCK_RGBG_DITHER - DCAM_BLOCK_BASE] = {DCAM_BLOCK_RGBG_DITHER,  dcam_k_cfg_rgb_dither},
[DCAM_BLOCK_PDAF - DCAM_BLOCK_BASE]    = {DCAM_BLOCK_PDAF,             dcam_k_cfg_pdaf},
[DCAM_BLOCK_LSC - DCAM_BLOCK_BASE]     = {DCAM_BLOCK_LSC,              dcam_k_cfg_lsc},
[DCAM_BLOCK_BAYERHIST - DCAM_BLOCK_BASE] = {DCAM_BLOCK_BAYERHIST,      dcam_k_cfg_bayerhist},
[DCAM_BLOCK_AEM - DCAM_BLOCK_BASE]     = {DCAM_BLOCK_AEM,              dcam_k_cfg_aem},
[DCAM_BLOCK_AFL - DCAM_BLOCK_BASE]     = {DCAM_BLOCK_AFL,              dcam_k_cfg_afl},
[DCAM_BLOCK_AWBC - DCAM_BLOCK_BASE]    = {DCAM_BLOCK_AWBC,             dcam_k_cfg_awbc},
[DCAM_BLOCK_BPC - DCAM_BLOCK_BASE]     = {DCAM_BLOCK_BPC,              dcam_k_cfg_bpc},
[DCAM_BLOCK_3DNR_ME - DCAM_BLOCK_BASE] = {DCAM_BLOCK_3DNR_ME,          dcam_k_cfg_3dnr_me},
[DCAM_BLOCK_AFM - DCAM_BLOCK_BASE]     = {DCAM_BLOCK_AFM,              dcam_k_cfg_afm},
};

static int sprd_dcam_cfg_param(void *dcam_handle, void *param)
{
	int ret = 0;
	uint32_t i = 0;
	func_dcam_cfg_param cfg_fun_ptr = NULL;
	struct dcam_pipe_dev *dev = NULL;
	struct isp_io_param *io_param;
	struct dcam_dev_param *pm;

	if (!dcam_handle || !param) {
		pr_err("fail to get valid input ptr\n");
		return -EFAULT;
	}

	dev = (struct dcam_pipe_dev *)dcam_handle;
	pm = dev->blk_dcam_pm;
	pm->idx = dev->idx;
	pm->dev = dev;
	io_param = (struct isp_io_param *)param;

	i = io_param->sub_block - DCAM_BLOCK_BASE;
	if (cfg_func_tab[i].sub_block == io_param->sub_block) {
		cfg_fun_ptr = cfg_func_tab[i].cfg_func;
	} else { /* if not, some error */
		pr_err("sub_block = %d, error\n", io_param->sub_block);
	}
	if (cfg_fun_ptr == NULL) {
		pr_debug("block %d not supported.\n", io_param->sub_block);
		goto exit;
	}

	ret = cfg_fun_ptr(io_param, pm);

exit:
	return ret;
}


static int sprd_dcam_set_cb(void *dcam_handle,
		dcam_dev_callback cb, void *priv_data)
{
	int ret = 0;
	struct dcam_pipe_dev *dev = NULL;

	if (!dcam_handle || !cb || !priv_data) {
		pr_err("fail to get valid input ptr\n");
		return -EFAULT;
	}

	dev = (struct dcam_pipe_dev *)dcam_handle;
	dev->dcam_cb_func = cb;
	dev->cb_priv_data = priv_data;

	return ret;
}

static int sprd_dcam_dev_start(void *dcam_handle)
{
	int ret = 0;
	int i;
	uint32_t force_ids = DCAM_CTRL_ALL;
	struct dcam_pipe_dev *dev = NULL;
	struct dcam_sync_helper *helper = NULL;
	struct dcam_path_desc *path;

	if (!dcam_handle) {
		pr_err("invalid dcam_pipe_dev\n");
		return -EFAULT;
	}

	dev = (struct dcam_pipe_dev *)dcam_handle;
	dev->offline = 0;

	ret = atomic_read(&dev->state);
	if (unlikely(ret != STATE_IDLE)) {
		pr_err("starting DCAM%u in state %d\n", dev->idx, ret);
		return -EINVAL;
	}

	pr_info("DCAM%u start: %p\n", dev->idx, dev);

	ret = dcam_init_sync_helper(dev);
	if (ret < 0) {
		pr_err("DCAM%u fail to init sync helper, ret: %d\n",
			dev->idx, ret);
		return ret;
	}

	/* enable statistic paths  */
	if (dev->blk_dcam_pm->aem.bypass == 0)
		atomic_set(&dev->path[DCAM_PATH_AEM].user_cnt, 1);
	if (dev->blk_dcam_pm->afm.bypass == 0)
		atomic_set(&dev->path[DCAM_PATH_AFM].user_cnt, 1);
	if (dev->blk_dcam_pm->afl.bypass == 0)
		atomic_set(&dev->path[DCAM_PATH_AFL].user_cnt, 1);
	if (dev->blk_dcam_pm->hist.bayerHist_info.hist_bypass == 0)
		atomic_set(&dev->path[DCAM_PATH_HIST].user_cnt, 1);

	if (dev->is_pdaf)
		atomic_set(&dev->path[DCAM_PATH_PDAF].user_cnt, 1);
	if (dev->is_3dnr)
		atomic_set(&dev->path[DCAM_PATH_3DNR].user_cnt, 1);

	if (dev->is_ebd)
		atomic_set(&dev->path[DCAM_PATH_VCH2].user_cnt, 1);

	dev->frame_index = 0;
	dev->index_to_set = 0;
	dev->iommu_status = 0;
	memset(dev->frame_ts, 0,
	       sizeof(dev->frame_ts[0]) * DCAM_FRAME_TIMESTAMP_COUNT);
	memset(dev->frame_ts_boot, 0,
	       sizeof(dev->frame_ts_boot[0]) * DCAM_FRAME_TIMESTAMP_COUNT);

	dev->helper_enabled = 0;
	if (!dev->slowmotion_count) {
		/* enable frame sync for 3DNR in normal mode */
		dcam_if_set_sync_enable(dev, DCAM_PATH_FULL, 1);
		dcam_if_set_sync_enable(dev, DCAM_PATH_BIN, 1);
		dcam_if_set_sync_enable(dev, DCAM_PATH_3DNR, 1);

		helper = dcam_get_sync_helper(dev);
	}

	ret = dcam_set_mipi_cap(dev, &dev->cap_info);
	if (ret < 0) {
		pr_err("DCAM%u fail to set mipi cap\n", dev->idx);
		return ret;
	}

	for (i = 0; i < DCAM_PATH_MAX; i++) {
		path = &dev->path[i];
		atomic_set(&path->set_frm_cnt, 0);

		if (atomic_read(&path->user_cnt) < 1)
			continue;

		ret = dcam_path_set_store_frm(dev, path, helper);
		if (ret < 0) {
			pr_err("DCAM%u %s fail to set frame, ret %d\n",
			       dev->idx, to_path_name(path->path_id), ret);
			return ret;
		}

		if (atomic_read(&path->set_frm_cnt) > 0)
			dcam_start_path(dev, path);
	}

	dcam_init_lsc(dev, 1);

	/* DCAM_CTRL_COEF will always set in dcam_init_lsc() */
	force_ids &= ~DCAM_CTRL_COEF;
	dcam_force_copy(dev, force_ids);

	if (helper) {
		if (helper->enabled)
			helper->sync.index = dev->index_to_set;
		else
			dcam_put_sync_helper(dev, helper);
	}

	/* TODO: change AFL trigger */
	atomic_set(&dev->path[DCAM_PATH_AFL].user_cnt, 0);

	dcam_reset_int_tracker(dev->idx);
	dcam_start(dev);

	if (dev->idx < 2)
		atomic_inc(&s_dcam_working);
	atomic_set(&dev->state, STATE_RUNNING);
	dcam_debug_trace(dev);
	dev->auto_cpy_id = 0;
	pr_info("dcam%d done\n", dev->idx);
	return ret;
}

static int sprd_dcam_dev_stop(void *dcam_handle)
{
	int ret = 0;
	struct dcam_pipe_dev *dev = NULL;

	if (!dcam_handle) {
		pr_err("fail to get valid input ptr\n");
		return -EFAULT;
	}
	dev = (struct dcam_pipe_dev *)dcam_handle;

	ret = atomic_read(&dev->state);
	if (unlikely(ret == STATE_INIT) || unlikely(ret == STATE_IDLE)) {
		pr_warn("DCAM%d not started yet\n", dev->idx);
		return -EINVAL;
	}

	pr_info("stop dcam %d.\n", dev->idx);

	dcam_stop(dev);
	dcam_reset(dev);

	if (0) {
		int i;

		for (i = 0; i < DCAM_FRAME_TIMESTAMP_COUNT; i++)
			pr_info("DCAM%u i=%02d t=%lld\n",
				dev->idx, i, dev->frame_ts_boot[i]);
	}

	dcam_dump_int_tracker(dev->idx);
	dcam_reset_int_tracker(dev->idx);

	if (dev->idx < 2)
		atomic_dec(&s_dcam_working);
	atomic_set(&dev->state, STATE_IDLE);

	dev->blk_dcam_pm->aem.bypass = 1;
	dev->blk_dcam_pm->afm.bypass = 1;
	dev->blk_dcam_pm->afl.bypass = 1;
	dev->blk_dcam_pm->hist.bayerHist_info.hist_bypass = 1;
	dev->is_pdaf = dev->is_3dnr = dev->is_4in1 = 0;

	pr_info("stop dcam pipe dev[%d]!\n", dev->idx);
	return ret;
}

/*
 * Open dcam_pipe_dev and hardware dcam_if IP.
 */
static int sprd_dcam_dev_open(void *dcam_handle)
{
	int ret = 0;
	int i, iommu_enable = 0;
	struct dcam_pipe_dev *dev = NULL;
	struct dcam_path_desc *path;
	struct sprd_cam_hw_info *hw;

	if (!dcam_handle) {
		pr_err("fail to get valid input ptr\n");
		return -EFAULT;
	}
	dev = (struct dcam_pipe_dev *)dcam_handle;

	ret = atomic_read(&dev->state);
	if (unlikely(ret != STATE_INIT)) {
		pr_err("DCAM%u already initialized, state=%d\n",
			dev->idx, ret);
		return -EINVAL;
	}

	hw = dev->hw;
	if (hw->ops == NULL) {
		pr_err("error: no hw ops.\n");
		return -EFAULT;
	}

	memset(&dev->path[0], 0, sizeof(dev->path));
	for (i  = 0; i < DCAM_PATH_MAX; i++) {
		path = &dev->path[i];
		path->path_id = i;
		atomic_set(&path->user_cnt, 0);
		atomic_set(&path->set_frm_cnt, 0);
		spin_lock_init(&path->size_lock);

		if (path->path_id == DCAM_PATH_BIN) {
			path->rds_coeff_size = RDS_COEF_TABLE_SIZE;
			path->rds_coeff_buf = kzalloc(path->rds_coeff_size, GFP_KERNEL);
			if (path->rds_coeff_buf == NULL) {
				path->rds_coeff_size = 0;
				pr_err("alloc rds coeff buffer failed.\n");
				ret = -ENOMEM;
				goto exit;
			}
		}
	}

	dev->blk_dcam_pm =
		kzalloc(sizeof(struct dcam_dev_param), GFP_KERNEL);
	if (dev->blk_dcam_pm == NULL) {
		pr_err("alloc dcam blk param failed.\n");
		ret = -ENOMEM;
		goto exit;
	}
	pr_info("alloc buf for all dcam pm success, %p, len %d\n",
			dev->blk_dcam_pm, (int)sizeof(struct dcam_dev_param));
	dev->blk_dcam_pm->aem.bypass = 1;
	dev->blk_dcam_pm->afm.bypass = 1;
	dev->blk_dcam_pm->afl.bypass = 1;
	dev->blk_dcam_pm->hist.bayerHist_info.hist_bypass = 1;
	spin_lock_init(&dev->blk_dcam_pm->lsc.lock);

	if (get_iommu_status(CAM_IOMMUDEV_DCAM) == 0)
		iommu_enable = 1;
	ret = cambuf_alloc(&dev->blk_dcam_pm->lsc.buf,
			DCAM_LSC_BUF_SIZE, 0, iommu_enable);
	ret |= cambuf_kmap(&dev->blk_dcam_pm->lsc.buf);
	if (ret) {
		ret = -ENOMEM;
		goto exit;
	}

	ret = hw->ops->init(hw, dev);
	if (ret) {
		pr_err("fail to open DCAM%u, ret: %d\n",
			dev->idx, ret);
		goto exit;
	}

	if (atomic_inc_return(&s_dcam_axi_opened) == 1)
		dcam_init_axim(dev->hw);
	ret = dcam_reset(dev);
	if (ret)
		goto reset_fail;


	ret = dcam_create_offline_thread(dev);
	if (ret)
		goto reset_fail;

	camera_queue_init(&dev->in_queue, DCAM_IN_Q_LEN,
				0, dcam_ret_src_frame);
	camera_queue_init(&dev->proc_queue, DCAM_PROC_Q_LEN,
				0, dcam_ret_src_frame);

	atomic_set(&dev->state, STATE_IDLE);
	spin_lock_init(&dev->glb_reg_lock);

	/* for debugfs */
	atomic_inc(&s_dcam_opened[dev->idx]);

	pr_info("open dcam pipe dev[%d]!\n", dev->idx);

	return 0;

reset_fail:
	atomic_dec(&s_dcam_axi_opened);
	ret = hw->ops->deinit(hw, dev);
exit:
	if (dev->blk_dcam_pm) {
		cambuf_kunmap(&dev->blk_dcam_pm->lsc.buf);
		cambuf_free(&dev->blk_dcam_pm->lsc.buf);
		kfree(dev->blk_dcam_pm);
		dev->blk_dcam_pm = NULL;
	}
	if (dev->path[DCAM_PATH_BIN].rds_coeff_buf) {
		kfree(dev->path[DCAM_PATH_BIN].rds_coeff_buf);
		dev->path[DCAM_PATH_BIN].rds_coeff_buf = NULL;
		dev->path[DCAM_PATH_BIN].rds_coeff_size = 0;
	}
	pr_info("fail to open dcam pipe dev[%d]!\n", dev->idx);

	return ret;
}

/*
 * Close dcam_pipe_dev and hardware dcam_if IP.
 */
int sprd_dcam_dev_close(void *dcam_handle)
{
	int ret = 0;
	struct dcam_pipe_dev *dev = NULL;
	struct sprd_cam_hw_info *hw;

	if (!dcam_handle) {
		pr_err("fail to get valid input ptr\n");
		return -EINVAL;
	}

	dev = (struct dcam_pipe_dev *)dcam_handle;

	if (unlikely(atomic_read(&dev->state) == STATE_INIT)) {
		pr_err("DCAM%u already closed\n", dev->idx);
		return -EINVAL;
	}

	dcam_stop_offline_thread(&dev->thread);
	camera_queue_clear(&dev->in_queue);
	camera_queue_clear(&dev->proc_queue);

	if (dev->blk_dcam_pm) {
		cambuf_kunmap(&dev->blk_dcam_pm->lsc.buf);
		cambuf_free(&dev->blk_dcam_pm->lsc.buf);
		kfree(dev->blk_dcam_pm);
		dev->blk_dcam_pm = NULL;
	}
	if (dev->path[DCAM_PATH_BIN].rds_coeff_buf) {
		kfree(dev->path[DCAM_PATH_BIN].rds_coeff_buf);
		dev->path[DCAM_PATH_BIN].rds_coeff_buf = NULL;
		dev->path[DCAM_PATH_BIN].rds_coeff_size = 0;
	}

	hw = dev->hw;
	ret = hw->ops->deinit(hw, dev);

	atomic_set(&dev->state, STATE_INIT);
	/* for debugfs */
	atomic_dec(&s_dcam_opened[dev->idx]);
	atomic_dec(&s_dcam_axi_opened);

	pr_info("close dcam pipe dev[%d]!\n", dev->idx);

	return ret;
}

/*
 * Operations for this dcam_pipe_dev.
 */
static struct dcam_pipe_ops s_dcam_pipe_ops = {
	.open = sprd_dcam_dev_open,
	.close = sprd_dcam_dev_close,
	.start = sprd_dcam_dev_start,
	.stop = sprd_dcam_dev_stop,
	.get_path = sprd_dcam_get_path,
	.put_path = sprd_dcam_put_path,
	.cfg_path = sprd_dcam_cfg_path,
	.ioctl = sprd_dcam_ioctrl,
	.cfg_blk_param = sprd_dcam_cfg_param,
	.proc_frame = sprd_dcam_proc_frame,
	.set_callback = sprd_dcam_set_cb,
};

/*
 * Get supported operations for a dcam_pipe_dev.
 */
struct dcam_pipe_ops *dcam_if_get_ops(void)
{
	return &s_dcam_pipe_ops;
}

static DEFINE_MUTEX(s_dcam_dev_mutex);
static struct dcam_pipe_dev *s_dcam_dev[DCAM_ID_MAX];

/*
 * Create a dcam_pipe_dev for designated sprd_cam_hw_info.
 */
void *dcam_if_get_dev(uint32_t idx, struct sprd_cam_hw_info *hw)
{
	struct dcam_pipe_dev *dev = NULL;

	if (idx >= DCAM_ID_MAX) {
		pr_err("invalid DCAM index: %u\n", idx);
		return NULL;
	}

	if (unlikely(!hw)) {
		pr_err("invalid param hw\n");
		return NULL;
	}

	mutex_lock(&s_dcam_dev_mutex);
	if (s_dcam_dev[idx]) {
		pr_err("dcam %d already in use. pipe dev: %p\n",
			idx, s_dcam_dev[idx]);
		goto exit;
	}

	dev = vzalloc(sizeof(struct dcam_pipe_dev));
	if (!dev) {
		pr_err("no memory for DCAM%u\n", idx);
		goto exit;
	}

	dev->idx = idx;
	dev->hw = hw;
	dev->is_pdaf = 0;

	/* frame sync helper */
	spin_lock_init(&dev->helper_lock);

	atomic_set(&dev->state, STATE_INIT);

	s_dcam_dev[idx] = dev;

exit:
	mutex_unlock(&s_dcam_dev_mutex);

	if (dev == NULL)
		pr_err("fail to get DCAM%u pipe dev\n", idx);
	else
		pr_info("get DCAM%u pipe dev: %p\n", idx, dev);

	return dev;
}

/*
 * Release a dcam_pipe_dev.
 */
int dcam_if_put_dev(void *dcam_handle)
{
	uint32_t idx = 0;
	int ret = 0;
	struct dcam_pipe_dev *dev = NULL;

	if (!dcam_handle) {
		pr_err("fail to get valid input ptr\n");
		return -EFAULT;
	}

	dev = (struct dcam_pipe_dev *)dcam_handle;
	idx = dev->idx;
	if (idx >= DCAM_ID_MAX) {
		pr_err("error dcam index: %u\n", idx);
		return -EINVAL;
	}

	mutex_lock(&s_dcam_dev_mutex);
	if (dev != s_dcam_dev[idx]) {
		pr_err("error: mismatched dev: %p, %p\n",
			dev, s_dcam_dev[idx]);
		mutex_unlock(&s_dcam_dev_mutex);
		return -EFAULT;
	}

	ret = atomic_read(&dev->state);
	if (unlikely(ret != STATE_INIT)) {
		pr_warn("releasing DCAM%u in state %d may cause leak\n",
			dev->idx, ret);
	}
	pr_info("put DCAM%u pipe dev: %p\n", idx, dev);
	vfree(dev);
	s_dcam_dev[idx] = NULL;

	mutex_unlock(&s_dcam_dev_mutex);

	return ret;
}

/* set line buffer share mode
 * Attention: set before stream on
 * Input: dcam idx, image width(max)
 */
int dcam_lbuf_share_mode(enum dcam_id idx, uint32_t width)
{
	int i = 0;
	int ret = 0;
	uint32_t tb_w[] = {
	/*     dcam0, dcam1 */
		4672, 3648,
		4224, 4224,
		3648, 4672,
		3648, 4672,
	};
	if (atomic_read(&s_dcam_working) > 0) {
		pr_warn("dcam 0/1 already in working\n");
		return 0;
	}

	switch (idx) {
	case 0:
		for (i = 3; i >= 0; i--) {
			if (width <= tb_w[i * 2])
				break;
		}
		DCAM_AXIM_WR(DCAM_LBUF_SHARE_MODE, i);
		pr_info("alloc dcam linebuf %d %d\n", tb_w[i*2], tb_w[i*2 + 1]);
		break;
	case 1:
		for (i = 0; i < 3; i++) {
			if (width <= tb_w[i * 2 + 1])
				break;
		}
		DCAM_AXIM_WR(DCAM_LBUF_SHARE_MODE, i);
		pr_info("alloc dcam linebuf %d %d\n", tb_w[i*2], tb_w[i*2 + 1]);
		break;
	default:
		pr_info("dcam %d no this setting\n", idx);
		ret = 1;
	}

	return ret;
}

/* return the number of how many buf in the out_buf_queue */
uint32_t get_outbuf_queue_cnt(void *dev, int path_id)
{
	struct dcam_path_desc *path;

	path = &(((struct dcam_pipe_dev *)dev)->path[path_id]);

	return camera_queue_cnt(&path->out_buf_queue);
}

