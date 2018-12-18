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


#include <linux/of.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/kthread.h>

#include <sprd_isp_r8p1.h>
#include "sprd_img.h"
#include <video/sprd_mmsys_pw_domain.h>
#include <sprd_mm.h>
#include <linux/sprd_ion.h>

#include "cam_hw.h"
#include "cam_types.h"
#include "cam_queue.h"
#include "cam_buf.h"
#include "cam_block.h"

#include "dcam_interface.h"

#include "isp_int.h"
#include "isp_reg.h"

#include "isp_interface.h"
#include "isp_core.h"
#include "isp_path.h"
#include "isp_slice.h"

#include "isp_cfg.h"
#include "isp_fmcu.h"

#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt) "ISP_CORE: %d %d %s : "\
	fmt, current->pid, __LINE__, __func__


uint32_t line_buffer_len;
unsigned long *isp_cfg_poll_addr[ISP_CONTEXT_MAX];

static DEFINE_MUTEX(isp_pipe_dev_mutex);
static struct isp_pipe_dev *s_isp_dev;


static int sprd_isp_put_context(
	void *isp_handle, int ctx_id);

static int sprd_isp_put_path(
	void *isp_handle, int ctx_id, int path_id);

static int isp_slice_ctx_init(struct isp_pipe_context *pctx);


/* isp debug fs starts */
#define DBG_REGISTER
#define WORK_MODE_SLEN  2
#define LBUF_LEN_SLEN  8

static struct dentry *debugfs_base;
static uint32_t s_dbg_linebuf_len = ISP_LINE_BUFFER_W;
static int s_dbg_work_mode = ISP_CFG_MODE;
uint32_t s_isp_bypass[ISP_CONTEXT_NUM] = { 0, 0, 0, 0 };
static uint32_t debug_ctx_id[4] = {0, 1, 2, 3};
int g_dbg_iommu_mode = IOMMU_AUTO;
int g_dbg_set_iommu_mode = IOMMU_AUTO;;

struct bypass_isptag {
	char *p; /* abbreviation */
	uint32_t addr;
	uint32_t bpos; /* bit position */
	uint32_t all; /* 1: all bypass except preview path */
};
static const struct bypass_isptag tb_bypass[] = {
[_EISP_GC] =       {"gc",        0x1B10, 0, 1}, /* GrGb correction */
[_EISP_VST] =      {"vst",       0x2010, 0, 1},
[_EISP_NLM] =      {"nlm",       0x2110, 0, 1},
[_EISP_IVST] =     {"ivst",      0x1E10, 0, 1},
[_EISP_CMC] =      {"cmc",       0x3110, 0, 1},
[_EISP_GAMC] =     {"gamma-c",   0x3210, 0, 1}, /* Gamma correction */
[_EISP_HSV] =      {"hsv",       0x3310, 0, 1},
[_EISP_HIST2] =    {"hist2",     0x3710, 0, 1},
[_EISP_PSTRZ] =    {"pstrz",     0x3410, 0, 1},
[_EISP_PRECDN] =   {"precdn",    0x5010, 0, 1},
[_EISP_YNR] =      {"ynr",       0x5110, 0, 1},
[_EISP_EE] =       {"ee",        0x5710, 0, 1},
[_EISP_GAMY] =     {"ygamma",    0x5B10, 0, 1},
[_EISP_CDN] =      {"cdn",       0x5610, 0, 1},
[_EISP_POSTCDN] =  {"postcdn",   0x5A10, 0, 1},
[_EISP_UVD] =      {"uvd",       0x3610, 0, 1},
[_EISP_IIRCNR] =   {"iircnr",    0x5D10, 0, 1},
[_EISP_YRAND] =    {"yrandom",   0x5E10, 0, 1},
[_EISP_BCHS] =     {"bchs",      0x5910, 0, 1},
[_EISP_YUVNF] =    {"yuvnf",     0xC210, 0, 1},

	{"ydelay",    0x5C10, 0, 1},
	{"fetch-fbd", 0x0C10, 0, 1},
	{"fbc-pre",   0xC310, 0, 1},
	{"scale-vid", 0xD010, 9, 1},
	{"store-vid", 0xD110, 0, 1},
	{"fbc-vid",   0xD310, 0, 1},
	{"scale-thb", 0xE010, 0, 1},
	{"store-thb", 0xE110, 0, 1},
	{"hist",      0x5410, 0, 1},
	/* ltm */
	{"ltm-map",   0x5F10, 0, 1},
	{"ltm-hist",  0x5510, 0, 1},
	/* 3dnr/nr3 */
	{"nr3-fbc",   0x9410, 0, 1},
	{"nr3-bld",   0x9110, 0, 1},
	{"nr3-crop",  0x9310, 0, 1},
	{"nr3-store", 0x9210, 0, 1},
	{"nr3-mem",   0x9010, 0, 1},
	/* can't bypass when prev */
	{"fetch",     0x0110, 0, 0},
	{"cfa",       0x3010, 0, 0},
	{"cce",       0x3510, 0, 0}, /* color conversion enhance */
	{"cfg",       0x8110, 0, 0},
	{"scale-pre", 0xC010, 9, 0},
	{"store-pre", 0xC110, 0, 0},

};

#ifdef DBG_REGISTER
static int reg_buf_show(struct seq_file *s, void *unused)
{
	debug_show_ctx_reg_buf((void *)s);
	return 0;
}

static int reg_buf_open(struct inode *inode, struct file *file)
{
	return single_open(file, reg_buf_show, inode->i_private);
}

static const struct file_operations reg_buf_ops = {
	.owner =	THIS_MODULE,
	.open = reg_buf_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif

static int bypass_read(struct seq_file *s, void *unused)
{

	uint32_t addr, val;
	uint32_t idx = *((uint32_t *)s->private);
	struct bypass_isptag dat;
	int i;

	seq_printf(s, "===========isp context %d=============\n", idx);
	for (i = 0; i < sizeof(tb_bypass) /
		sizeof(struct bypass_isptag); i++) {
		dat = tb_bypass[i];
		addr = dat.addr;
		val = ISP_REG_RD(idx, addr) & (1 << dat.bpos);
		if (val)
			seq_printf(s, "%s:%d  bypass\n", dat.p, val);
		else
			seq_printf(s, "%s:%d  work\n", dat.p, val);
	}
	seq_puts(s, "\nall:1 //bypass all except preview path\n");
	seq_puts(s, "\nltm:1 //(ltm-hist,ltm-map)\n");
	seq_puts(s, "\nr3:1 //or 3dnr:1(all of 3dnr block)\n");

	return 0;
}

static ssize_t bypass_write(struct file *filp,
		const char __user *buffer, size_t count, loff_t *ppos)
{
	struct seq_file *p = (struct seq_file *)filp->private_data;
	uint32_t idx = *((uint32_t *)p->private);
	char buf[256];
	uint32_t val = 2;
	struct bypass_isptag dat;
	int i;
	char name[16 + 1];
	uint32_t bypass_all = 0;

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
		/* to lowwer */
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
	if (strcmp(name, "all") == 0) {
		bypass_all = 1;
	} else { /* check special: ltm, nr3 */
		if (strcmp(name, "ltm") == 0) {
			ISP_HREG_MWR(ISP_LTM_HIST_PARAM, BIT_0, val);
			ISP_HREG_MWR(ISP_LTM_MAP_PARAM0, BIT_0, val);
			s_isp_bypass[idx] &= (~(1 << _EISP_LTM));
			if (val)
				s_isp_bypass[idx] |= (1 << _EISP_LTM);
			return count;
		} else if (strcmp(name, "nr3") == 0 ||
			strcmp(name, "3dnr") == 0) {
			s_isp_bypass[idx] &= (~(1 << _EISP_NR3));
			if (val)
				s_isp_bypass[idx] |= (1 << _EISP_NR3);
			ISP_HREG_MWR(ISP_3DNR_MEM_CTRL_PARAM0, BIT_0, val);
			ISP_HREG_MWR(ISP_3DNR_BLEND_CONTROL0, BIT_0, val);
			ISP_HREG_MWR(ISP_3DNR_STORE_PARAM, BIT_0, val);
			ISP_HREG_MWR(ISP_3DNR_MEM_CTRL_PRE_PARAM0, BIT_0, val);
			ISP_HREG_MWR(ISP_FBC_3DNR_STORE_PARAM, BIT_0, val);
			return count;
		}
	}
	for (i = 0; i < sizeof(tb_bypass) / sizeof(struct bypass_isptag); i++) {
		if (strcmp(tb_bypass[i].p, name) == 0 || bypass_all) {
			dat = tb_bypass[i];
			pr_debug("set isp addr 0x%x, bit %d val %d\n",
				dat.addr, dat.bpos, val);
			if (i < _EISP_TOTAL) {
				s_isp_bypass[idx] &= (~(1 << i));
				s_isp_bypass[idx] |= (val << i);
			}
			if (bypass_all && (dat.all == 0))
				continue;
			ISP_REG_MWR(idx, dat.addr, 1 << dat.bpos,
				val << dat.bpos);

			if (!bypass_all)
				break;
		}
	}

	return count;
}

static int bypass_open(struct inode *inode, struct file *file)
{
	return single_open(file, bypass_read, inode->i_private);
}

static const struct file_operations isp_bypass_ops = {
	.owner = THIS_MODULE,
	.open = bypass_open,
	.read = seq_read,
	.write = bypass_write,
};


static uint8_t work_mode_string[2][16] = {
	"ISP_CFG_MODE", "ISP_AP_MODE"
};

static ssize_t work_mode_show(
		struct file *filp, char __user *buffer,
		size_t count, loff_t *ppos)
{
	char buf[16];

	snprintf(buf, sizeof(buf), "%d(%s)\n", s_dbg_work_mode,
		work_mode_string[s_dbg_work_mode&1]);

	return simple_read_from_buffer(
			buffer, count, ppos,
			buf, strlen(buf));
}

static ssize_t work_mode_write(
		struct file *filp, const char __user *buffer,
		size_t count, loff_t *ppos)
{
	int ret = 0;
	char msg[8];
	char *last;
	int val;

	if (count > WORK_MODE_SLEN)
		return -EINVAL;

	ret = copy_from_user(msg, (void __user *)buffer, count);
	if (ret) {
		pr_err("fail to copy_from_user\n");
		return -EFAULT;
	}

	msg[WORK_MODE_SLEN-1] = '\0';
	val = simple_strtol(msg, &last, 0);
	if (val == 0)
		s_dbg_work_mode = ISP_CFG_MODE;
	else if (val == 1)
		s_dbg_work_mode = ISP_AP_MODE;
	else
		pr_err("error: invalid work mode: %d", val);

	return count;
}

static const struct file_operations work_mode_ops = {
	.owner =	THIS_MODULE,
	.open = simple_open,
	.read = work_mode_show,
	.write = work_mode_write,
};


static uint8_t iommu_mode_string[4][32] = {
	"IOMMU_AUTO",
	"IOMMU_OFF",
	"IOMMU_ON_RESERVED",
	"IOMMU_ON"
};
static ssize_t iommu_mode_show(
		struct file *filp, char __user *buffer,
		size_t count, loff_t *ppos)
{
	char buf[64];

	snprintf(buf, sizeof(buf), "cur: %d(%s), next: %d(%s)\n",
		g_dbg_iommu_mode,
		iommu_mode_string[g_dbg_iommu_mode&3],
		g_dbg_set_iommu_mode,
		iommu_mode_string[g_dbg_set_iommu_mode&3]);

	return simple_read_from_buffer(
			buffer, count, ppos,
			buf, strlen(buf));
}

static ssize_t iommu_mode_write(
		struct file *filp, const char __user *buffer,
		size_t count, loff_t *ppos)
{
	int ret = 0;
	char msg[8];
	char *last;
	int val;

	if (count > WORK_MODE_SLEN)
		return -EINVAL;

	ret = copy_from_user(msg, (void __user *)buffer, count);
	if (ret) {
		pr_err("fail to copy_from_user\n");
		return -EFAULT;
	}

	msg[WORK_MODE_SLEN-1] = '\0';
	val = simple_strtol(msg, &last, 0);
	if (val == 0)
		g_dbg_set_iommu_mode = IOMMU_AUTO;
	else if (val == 1)
		g_dbg_set_iommu_mode = IOMMU_OFF;
	else if (val == 2)
		g_dbg_set_iommu_mode = IOMMU_ON_RESERVED;
	else if (val == 3)
		g_dbg_set_iommu_mode = IOMMU_ON;
	else
		pr_err("error: invalid work mode: %d", val);

	pr_info("set_iommu_mode : %d(%s)\n",
		g_dbg_set_iommu_mode,
		iommu_mode_string[g_dbg_set_iommu_mode&3]);
	return count;
}

static const struct file_operations iommu_mode_ops = {
	.owner =	THIS_MODULE,
	.open = simple_open,
	.read = iommu_mode_show,
	.write = iommu_mode_write,
};

static ssize_t lbuf_len_show(
			struct file *filp, char __user *buffer,
			size_t count, loff_t *ppos)
{
	char buf[16];

	snprintf(buf, sizeof(buf), "%d\n", s_dbg_linebuf_len);

	return simple_read_from_buffer(
			buffer, count, ppos,
			buf, strlen(buf));
}

static ssize_t lbuf_len_write(struct file *filp,
			const char __user *buffer,
			size_t count, loff_t *ppos)
{
	int ret = 0;
	char msg[8];
	int val;

	if (count > LBUF_LEN_SLEN)
		return -EINVAL;

	ret = copy_from_user(msg, (void __user *)buffer, count);
	if (ret) {
		pr_err("fail to copy_from_user\n");
		return -EFAULT;
	}

	msg[LBUF_LEN_SLEN - 1] = '\0';
	val = simple_strtol(msg, NULL, 0);
	s_dbg_linebuf_len = val;
	pr_info("set line buf len %d.  %s\n", val, msg);

	return count;
}

static const struct file_operations lbuf_len_ops = {
	.owner =	THIS_MODULE,
	.open = simple_open,
	.read = lbuf_len_show,
	.write = lbuf_len_write,
};

int sprd_isp_debugfs_init(void)
{
	char dirname[32] = {0};

	snprintf(dirname, sizeof(dirname), "sprd_isp");
	debugfs_base = debugfs_create_dir(dirname, NULL);
	if (!debugfs_base) {
		pr_err("fail to create debugfs dir\n");
		return -ENOMEM;
	}
	memset(s_isp_bypass, 0x00, sizeof(s_isp_bypass));
	if (!debugfs_create_file("work_mode", 0644,
			debugfs_base, NULL, &work_mode_ops))
		return -ENOMEM;

	if (!debugfs_create_file("iommu_mode", 0644,
			debugfs_base, NULL, &iommu_mode_ops))
		return -ENOMEM;

	if (!debugfs_create_file("line_buf_len", 0644,
			debugfs_base, NULL, &lbuf_len_ops))
		return -ENOMEM;

#ifdef DBG_REGISTER
	if (!debugfs_create_file("pre0_buf", 0444,
			debugfs_base, &debug_ctx_id[0], &reg_buf_ops))
		return -ENOMEM;
	if (!debugfs_create_file("cap0_buf", 0444,
			debugfs_base, &debug_ctx_id[1], &reg_buf_ops))
		return -ENOMEM;
	if (!debugfs_create_file("pre1_buf", 0444,
			debugfs_base, &debug_ctx_id[2], &reg_buf_ops))
		return -ENOMEM;
	if (!debugfs_create_file("cap1_buf", 0444,
			debugfs_base, &debug_ctx_id[3], &reg_buf_ops))
		return -ENOMEM;
#endif

	if (!debugfs_create_file("pre0_bypass", 0660,
			debugfs_base, &debug_ctx_id[0], &isp_bypass_ops))
		return -ENOMEM;
	if (!debugfs_create_file("cap0_bypass", 0660,
			debugfs_base, &debug_ctx_id[1], &isp_bypass_ops))
		return -ENOMEM;
	if (!debugfs_create_file("pre1_bypass", 0660,
			debugfs_base, &debug_ctx_id[2], &isp_bypass_ops))
		return -ENOMEM;
	if (!debugfs_create_file("cap1_bypass", 0660,
			debugfs_base, &debug_ctx_id[3], &isp_bypass_ops))
		return -ENOMEM;

	return 0;
}

int sprd_isp_debugfs_deinit(void)
{
	if (debugfs_base)
		debugfs_remove_recursive(debugfs_base);
	debugfs_base = NULL;
	return 0;
}
/* isp debug fs end */

static void free_offline_pararm(void *param)
{
	struct isp_offline_param *cur, *prev;

	cur = (struct isp_offline_param *)param;
	while (cur) {
		prev = (struct isp_offline_param *)cur->prev;
		kfree(cur);
		pr_info("free %p\n", cur);
		cur = prev;
	}
}

void isp_unmap_frame(void *param)
{
	struct camera_frame *frame;

	if (!param) {
		pr_err("error: null input ptr.\n");
		return;
	}
	frame = (struct camera_frame *)param;
	cambuf_iommu_unmap(&frame->buf);
}

void isp_ret_out_frame(void *param)
{
	struct camera_frame *frame;
	struct isp_pipe_context *pctx;
	struct isp_path_desc *path;

	if (!param) {
		pr_err("error: null input ptr.\n");
		return;
	}

	frame = (struct camera_frame *)param;
	path = (struct isp_path_desc *)frame->priv_data;

	pr_debug("frame %p, ch_id %d, buf_fd %d\n",
		frame, frame->channel_id, frame->buf.mfd[0]);

	if (frame->is_reserved)
		camera_enqueue(
			&path->reserved_buf_queue, frame);
	else {
		pctx = path->attach_ctx;
		cambuf_iommu_unmap(&frame->buf);
		pctx->isp_cb_func(
			ISP_CB_RET_DST_BUF,
			frame, pctx->cb_priv_data);
	}
}

void isp_ret_src_frame(void *param)
{
	struct camera_frame *frame;
	struct isp_pipe_context *pctx;

	if (!param) {
		pr_err("error: null input ptr.\n");
		return;
	}

	frame = (struct camera_frame *)param;
	pctx = (struct isp_pipe_context *)frame->priv_data;
	pr_debug("frame %p, ch_id %d, buf_fd %d\n",
		frame, frame->channel_id, frame->buf.mfd[0]);
	free_offline_pararm(frame->param_data);
	frame->param_data = NULL;
	cambuf_iommu_unmap(&frame->buf);
	pctx->isp_cb_func(
		ISP_CB_RET_SRC_BUF,
		frame, pctx->cb_priv_data);
}

void isp_destroy_reserved_buf(void *param)
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

void isp_set_ctx_common(struct isp_pipe_context *pctx)
{
	uint32_t idx = pctx->ctx_id;
	uint32_t bypass = 0;
	uint32_t en_3dnr;
	struct isp_fetch_info *fetch = &pctx->fetch;

	pr_info("enter %s: fmt:%d, w:%d, h:%d\n", __func__, fetch->fetch_fmt,
			fetch->in_trim.size_x, fetch->in_trim.size_y);

	en_3dnr = 0;/* (pctx->mode_3dnr == MODE_3DNR_OFF) ? 0 : 1; */
	ISP_REG_MWR(idx, ISP_COMMON_SPACE_SEL,
			BIT_1 | BIT_0, pctx->dispatch_color);
	/* 11b: close store_dbg module */
	ISP_REG_MWR(idx, ISP_COMMON_SPACE_SEL,
			BIT_3 | BIT_2, 3 << 2);
	ISP_REG_MWR(idx, ISP_COMMON_SPACE_SEL, BIT_4, 0 << 4);

	ISP_REG_MWR(idx, ISP_COMMON_SCL_PATH_SEL,
			BIT_10, pctx->fetch_path_sel  << 10);
	ISP_REG_MWR(idx, ISP_COMMON_SCL_PATH_SEL,
			BIT_8, en_3dnr << 8); /* 3dnr off */
	ISP_REG_MWR(idx, ISP_COMMON_SCL_PATH_SEL,
			BIT_5 | BIT_4, 3 << 4);  /* thumb path off */
	ISP_REG_MWR(idx, ISP_COMMON_SCL_PATH_SEL,
			BIT_3 | BIT_2, 3 << 2); /* vid path off */
	ISP_REG_MWR(idx, ISP_COMMON_SCL_PATH_SEL,
			BIT_1 | BIT_0, 3 << 0);  /* pre/cap path off */
	if (pctx->fmcu_handle) {
		unsigned long reg_offset;
		struct isp_fmcu_ctx_desc *fmcu;
		uint32_t reg_bits[ISP_CONTEXT_NUM] = { 0x00, 0x02, 0x01, 0x03};

		fmcu = (struct isp_fmcu_ctx_desc *)pctx->fmcu_handle;
		reg_offset = (fmcu->fid == 0) ?
					ISP_COMMON_FMCU0_PATH_SEL :
					ISP_COMMON_FMCU1_PATH_SEL;
		ISP_HREG_MWR(reg_offset, BIT_1 | BIT_0, reg_bits[pctx->ctx_id]);
	}

	ISP_REG_MWR(idx, ISP_FETCH_PARAM, BIT_0, bypass);
	ISP_REG_MWR(idx, ISP_FETCH_PARAM,
			(0xF << 4), fetch->fetch_fmt << 4);
	ISP_REG_WR(idx, ISP_FETCH_MEM_SLICE_SIZE,
			fetch->in_trim.size_x | (fetch->in_trim.size_y << 16));

	ISP_REG_WR(idx, ISP_FETCH_SLICE_Y_PITCH, fetch->pitch.pitch_ch0);
	ISP_REG_WR(idx, ISP_FETCH_SLICE_U_PITCH, fetch->pitch.pitch_ch1);
	ISP_REG_WR(idx, ISP_FETCH_SLICE_V_PITCH, fetch->pitch.pitch_ch2);
	ISP_REG_WR(idx, ISP_FETCH_LINE_DLY_CTRL, 0x200);
	ISP_REG_WR(idx, ISP_FETCH_MIPI_INFO,
		fetch->mipi_word_num | (fetch->mipi_byte_rel_pos << 16));

	ISP_REG_WR(idx, ISP_DISPATCH_DLY,  0x253C);
	ISP_REG_WR(idx, ISP_DISPATCH_LINE_DLY1,  0x280001C);
	ISP_REG_WR(idx, ISP_DISPATCH_PIPE_BUF_CTRL_CH0,  0x64043C);
	ISP_REG_WR(idx, ISP_DISPATCH_CH0_SIZE,
		fetch->in_trim.size_x | (fetch->in_trim.size_y << 16));
	ISP_REG_WR(idx, ISP_DISPATCH_CH0_BAYER, pctx->dispatch_bayer_mode);

	/*CFA*/
	ISP_REG_MWR(idx, ISP_CFAE_NEW_CFG0, BIT_0, 0);
	ISP_REG_WR(idx, ISP_CFAE_INTP_CFG0, 0x1F4 | 0x1F4 << 16);
	ISP_REG_WR(idx, ISP_CFAE_INTP_CFG1,
		(0x1 << 31) | (0x14 << 12) | (0x7F << 4) | 0x4);
	ISP_REG_WR(idx, ISP_CFAE_INTP_CFG2, 0x8 | (0x0 << 8));
	ISP_REG_WR(idx, ISP_CFAE_INTP_CFG3,
		(0x8 << 20) | (0x8 << 12) | 0x118);
	ISP_REG_WR(idx, ISP_CFAE_INTP_CFG4, 0x64 | (0x64 << 16));
	ISP_REG_WR(idx, ISP_CFAE_INTP_CFG5, 0xC8 | (0xC8 << 16));

	ISP_REG_WR(idx, ISP_CFAE_CSS_CFG0, 0x64 | (0x96 << 16));
	ISP_REG_WR(idx, ISP_CFAE_CSS_CFG1, 0x14 | (0x5 << 16));
	ISP_REG_WR(idx, ISP_CFAE_CSS_CFG2,
		(0x28 << 20) | (0x1E << 10) | 0xF);
	ISP_REG_WR(idx, ISP_CFAE_CSS_CFG3, 0xC8);
	ISP_REG_WR(idx, ISP_CFAE_CSS_CFG4, (0x5 << 10));
	ISP_REG_WR(idx, ISP_CFAE_CSS_CFG5, (0x50 << 9) | 0x78);
	ISP_REG_WR(idx, ISP_CFAE_CSS_CFG6,
		(0x32 << 18) | (0x32 << 9));
	ISP_REG_WR(idx, ISP_CFAE_CSS_CFG7, (0x64 << 18));
	ISP_REG_WR(idx, ISP_CFAE_CSS_CFG8, 0x3C | (0x8 << 17));
	ISP_REG_WR(idx, ISP_CFAE_CSS_CFG9,
		(0x1FC << 20) | (0x134 << 10) | 0x27C);
	ISP_REG_WR(idx, ISP_CFAE_CSS_CFG10,
		(0x214 << 20) | (0x1FF << 10) | 0x1CD);
	ISP_REG_WR(idx, ISP_CFAE_CSS_CFG11, 0x22D << 10 | 0x205);

	/*CCE*/
	ISP_REG_MWR(idx, ISP_CCE_PARAM, BIT_0, 0);
	ISP_REG_WR(idx, ISP_CCE_MATRIX0, (150 << 11) | 77);
	ISP_REG_WR(idx, ISP_CCE_MATRIX1, ((-43) << 11) | 29);
	ISP_REG_WR(idx, ISP_CCE_MATRIX2, 0x407AB);
	ISP_REG_WR(idx, ISP_CCE_MATRIX3, ((-107) << 11) | 128);
	ISP_REG_WR(idx, ISP_CCE_MATRIX4, (-21));
	ISP_REG_WR(idx, ISP_CCE_SHIFT, 0);

	ISP_REG_WR(idx, ISP_YDELAY_STEP, 0x144);
	ISP_REG_WR(idx, ISP_SCALER_PRE_CAP_BASE
		+ ISP_SCALER_HBLANK, 0x4040);
	ISP_REG_WR(idx, ISP_SCALER_PRE_CAP_BASE + ISP_SCALER_RES, 0xFF);
	ISP_REG_WR(idx, ISP_SCALER_PRE_CAP_BASE + ISP_SCALER_DEBUG, 1);

	pr_info("end\n");
}

void isp_set_ctx_default(struct isp_pipe_context *pctx)
{
	uint32_t idx = pctx->ctx_id;
	uint32_t bypass = 1;

	ISP_REG_MWR(idx, ISP_STORE_DEBUG_BASE + ISP_STORE_PARAM, BIT_0, bypass);

	/* bypass all path scaler & store */
	ISP_REG_MWR(idx, ISP_SCALER_PRE_CAP_BASE + ISP_SCALER_CFG,
		BIT_31, 0 << 31);
	ISP_REG_MWR(idx, ISP_SCALER_VID_BASE + ISP_SCALER_CFG,
		BIT_31, 0 << 31);

	ISP_REG_MWR(idx, ISP_STORE_PRE_CAP_BASE + ISP_STORE_PARAM,
		BIT_0, 1);
	ISP_REG_MWR(idx, ISP_STORE_VID_BASE + ISP_STORE_PARAM,
		BIT_0, 1);
	ISP_REG_MWR(idx, ISP_STORE_THUMB_BASE + ISP_STORE_PARAM,
		BIT_0, 1);

	/* default bypass all blocks */
	ISP_REG_MWR(idx, ISP_NLM_PARA, BIT_0, bypass);
	ISP_REG_MWR(idx, ISP_VST_PARA, BIT_0, bypass);
	ISP_REG_MWR(idx, ISP_IVST_PARA, BIT_0, bypass);

	ISP_REG_MWR(idx, ISP_CMC10_PARAM, BIT_0, bypass);
	ISP_REG_MWR(idx, ISP_GAMMA_PARAM, BIT_0, bypass);
	ISP_REG_MWR(idx, ISP_HSV_PARAM, BIT_0, bypass);
	ISP_REG_MWR(idx, ISP_PSTRZ_PARAM, BIT_0, bypass);
	ISP_REG_MWR(idx, ISP_CCE_PARAM, BIT_0, bypass);

	ISP_REG_MWR(idx, ISP_UVD_PARAM, BIT_0, 1);
	ISP_REG_MWR(idx, ISP_PRECDN_PARAM, BIT_0, 1);
	ISP_REG_MWR(idx, ISP_YNR_CONTRL0, BIT_0|BIT_1, 0x3);
	ISP_REG_MWR(idx, ISP_HIST_PARAM, BIT_0, 1);
	ISP_REG_MWR(idx, ISP_HIST_CFG_READY, BIT_0, 1);
	ISP_REG_MWR(idx, ISP_LTM_HIST_PARAM, BIT_0, 1);
	ISP_REG_MWR(idx, ISP_LTM_MAP_PARAM0, BIT_0, 1);
	ISP_REG_MWR(idx, ISP_CDN_PARAM, BIT_0, 1);
	ISP_REG_MWR(idx, ISP_EE_PARAM, BIT_0, 1);
	ISP_REG_MWR(idx, ISP_BCHS_PARAM, BIT_0, 1);
	ISP_REG_MWR(idx, ISP_POSTCDN_COMMON_CTRL, BIT_0|BIT_1, 0x3);
	ISP_REG_MWR(idx, ISP_YGAMMA_PARAM, BIT_0, 1);
	ISP_REG_MWR(idx, ISP_IIRCNR_PARAM, BIT_0, 1);
	ISP_REG_MWR(idx, ISP_YRANDOM_PARAM1, BIT_0, 1);


	/* 3DNR bypass */
	ISP_REG_MWR(idx, ISP_3DNR_MEM_CTRL_PARAM0, BIT_0, 1);
	ISP_REG_MWR(idx, ISP_3DNR_BLEND_CONTROL0, BIT_0, 1);
	ISP_REG_MWR(idx, ISP_3DNR_STORE_PARAM, BIT_0, 1);
	ISP_REG_MWR(idx, ISP_3DNR_MEM_CTRL_PRE_PARAM0, BIT_0, 1);
	pr_info("end\n");
}

static int isp_adapt_blkparam(struct isp_pipe_context *pctx)
{
	uint32_t new_width, old_width;
	uint32_t new_height, old_height;
	uint32_t crop_start_x, crop_start_y;
	uint32_t crop_end_x, crop_end_y;
	struct img_trim *src_trim;
	struct img_size *dst = &pctx->original.dst_size;

	if (pctx->original.src_size.w > 0) {
		/* for input scaled image */
		src_trim = &pctx->original.src_trim;
		new_width = dst->w;
		new_height = dst->h;
		old_width = src_trim->size_x;
		old_height = src_trim->size_y;
	} else {
		src_trim = &pctx->input_trim;
		old_width = src_trim->size_x;
		old_height = src_trim->size_y;
		new_width = old_width;
		new_height = old_height;
	}
	crop_start_x = src_trim->start_x;
	crop_start_y = src_trim->start_y;
	crop_end_x = src_trim->start_x + src_trim->size_x - 1;
	crop_end_y = src_trim->start_y + src_trim->size_y - 1;

	pr_debug("crop %d %d %d %d, size(%d %d) => (%d %d)\n",
		crop_start_x, crop_start_y, crop_end_x, crop_end_y,
		old_width, old_height, new_width, new_height);

	isp_k_update_nlm(pctx->ctx_id, &pctx->isp_k_param,
		new_width, old_width, new_height, old_height,
		crop_start_x, crop_start_y, crop_end_x, crop_end_y);
	isp_k_update_ynr(pctx->ctx_id, &pctx->isp_k_param,
		new_width, old_width, new_height, old_height,
		crop_start_x, crop_start_y, crop_end_x, crop_end_y);

	return 0;
}

static int isp_3dnr_process_frame_previous(struct isp_pipe_context *pctx,
					   struct camera_frame *pframe)
{
	if (!pctx || !pframe) {
		pr_err("invalid parameter\n");
		return -EINVAL;
	}

	if (pctx->mode_3dnr == MODE_3DNR_OFF)
		return 0;

	/*  Check Zoom or not */
	if ((pctx->input_trim.size_x != pctx->nr3_ctx.width) ||
	    (pctx->input_trim.size_y != pctx->nr3_ctx.height)) {
		pr_info("frame size changed, reset 3dnr blending\n");

		/*
		 * 1. reset blending count, so
		 *	cnt = 0, DONOT fetch ref
		 * 2. MUST before get output buffer, because
		 *      USING blend cnt to choose reserved buf or HAL buf
		 */
		pctx->nr3_ctx.blending_cnt = 0;
	}

	return 0;
}

static int isp_3dnr_process_frame(struct isp_pipe_context *pctx,
				  struct camera_frame *pframe)
{
	int loop = 0;
	struct isp_3dnr_ctx_desc *nr3_ctx;

	struct dcam_frame_synchronizer *fsync = NULL;

	fsync = (struct dcam_frame_synchronizer *)pframe->sync_data;

	if (fsync) {
		pr_debug("id %u, valid %d, x %d, y %d, w %u, h %u\n",
			 fsync->index, fsync->nr3_me.valid,
			 fsync->nr3_me.mv_x, fsync->nr3_me.mv_y,
			 fsync->nr3_me.src_width, fsync->nr3_me.src_height);
	}

	nr3_ctx = &pctx->nr3_ctx;

	nr3_ctx->width  = pctx->input_trim.size_x;
	nr3_ctx->height = pctx->input_trim.size_y;

	pr_debug("input.w[%d], input.h[%d], trim.w[%d], trim.h[%d]\n",
		pctx->input_size.w, pctx->input_size.h,
		pctx->input_trim.size_x, pctx->input_trim.size_y);

	switch (pctx->mode_3dnr) {
	case MODE_3DNR_PRE:
		nr3_ctx->type = NR3_FUNC_PRE;

		if (fsync && fsync->nr3_me.valid) {
			nr3_ctx->mv.mv_x = fsync->nr3_me.mv_x;
			nr3_ctx->mv.mv_y = fsync->nr3_me.mv_y;
			nr3_ctx->mvinfo = &fsync->nr3_me;

			isp_3dnr_conversion_mv(nr3_ctx);
		} else {
			pr_err("binning path mv not ready, set default 0\n");
			nr3_ctx->mv.mv_x = 0;
			nr3_ctx->mv.mv_y = 0;
			nr3_ctx->mvinfo = NULL;
		}

		for (loop = 0; loop < ISP_NR3_BUF_NUM; loop++) {
			nr3_ctx->buf_info[loop] =
				&pctx->nr3_buf[loop]->buf;
		}

		isp_3dnr_gen_config(nr3_ctx);
		isp_3dnr_config_param(nr3_ctx,
				      pctx->ctx_id,
				      NR3_FUNC_PRE);

		pr_debug("MODE_3DNR_PRE frame type: %d from dcam binning path mv[%d, %d]!\n.",
			 pframe->channel_id,
			 nr3_ctx->mv.mv_x,
			 nr3_ctx->mv.mv_y);
		break;

	case MODE_3DNR_CAP:
		nr3_ctx->type = NR3_FUNC_CAP;

		if (fsync && fsync->nr3_me.valid) {
			nr3_ctx->mv.mv_x = fsync->nr3_me.mv_x;
			nr3_ctx->mv.mv_y = fsync->nr3_me.mv_y;
		} else {
			pr_err("full path mv not ready, set default 0\n");
			nr3_ctx->mv.mv_x = 0;
			nr3_ctx->mv.mv_y = 0;
		}

		for (loop = 0; loop < ISP_NR3_BUF_NUM; loop++) {
			nr3_ctx->buf_info[loop] =
				&pctx->nr3_buf[loop]->buf;
		}

		isp_3dnr_gen_config(nr3_ctx);
		isp_3dnr_config_param(nr3_ctx,
				      pctx->ctx_id,
				      NR3_FUNC_CAP);

		pr_debug("MODE_3DNR_CAP frame type: %d from dcam binning path mv[%d, %d]!\n.",
			 pframe->channel_id,
			 nr3_ctx->mv.mv_x,
			 nr3_ctx->mv.mv_y);
		break;

	case MODE_3DNR_OFF:
		/* default: bypass 3dnr */
		nr3_ctx->type = NR3_FUNC_OFF;
		isp_3dnr_bypass_config(pctx->ctx_id);
		pr_debug("isp_offline_start_frame MODE_3DNR_OFF\n");
		break;
	default:
		/* default: bypass 3dnr */
		nr3_ctx->type = NR3_FUNC_OFF;
		isp_3dnr_bypass_config(pctx->ctx_id);
		pr_debug("isp_offline_start_frame default\n");
		break;
	}

	if (fsync)
		dcam_if_release_sync(fsync, pframe);

	return 0;
}

static int isp_ltm_process_frame_previous(struct isp_pipe_context *pctx,
					  struct camera_frame *pframe)
{
	if (!pctx || !pframe) {
		pr_err("invalid parameter\n");
		return -EINVAL;
	}

	/*
	 * Only preview path care of frame size changed
	 * Because capture path, USING hist from preview path
	 */
	if (pctx->mode_ltm != MODE_LTM_PRE)
		return 0;

	/*  Check Zoom or not */
	if ((pctx->input_trim.size_x != pctx->ltm_ctx.frame_width) ||
	    (pctx->input_trim.size_y != pctx->ltm_ctx.frame_height)) {
		pr_info("frame size changed, bypass ltm map\n");

		/* 1. hists from preview path always on
		 * 2. map will be off one time in preview case
		 */
		pctx->ltm_ctx.map.bypass = 1;
	} else {
		pctx->ltm_ctx.map.bypass = 0;
	}

	return 0;
}

static int isp_ltm_process_frame(struct isp_pipe_context *pctx,
				 struct camera_frame *pframe)
{
	int ret = 0;

	/* pre & cap */
	pctx->ltm_ctx.type = pctx->mode_ltm;
	pctx->ltm_ctx.fid	   = pframe->fid;
	pctx->ltm_ctx.frame_width  = pctx->input_trim.size_x;
	pctx->ltm_ctx.frame_height = pctx->input_trim.size_y;
	pctx->ltm_ctx.isp_pipe_ctx_id = pctx->ctx_id;

	/* pre & cap */
	ret = isp_ltm_gen_frame_config(&pctx->ltm_ctx);
	if (ret == -1) {
		pctx->mode_ltm = MODE_LTM_OFF;
		pr_err("LTM cfg frame err, DISABLE\n");
	}
#if 0
	pr_info("type[%d], fid[%d], frame_width[%d], frame_height[%d], isp_pipe_ctx_id[%d]\n",
		pctx->ltm_ctx.type,
		pctx->ltm_ctx.fid,
		pctx->ltm_ctx.frame_width,
		pctx->ltm_ctx.frame_height,
		pctx->ltm_ctx.isp_pipe_ctx_id);

	pr_info("frame_height_stat[%d], frame_width_stat[%d],\
		tile_num_x_minus[%d], tile_num_y_minus[%d],\
		tile_width[%d], tile_height[%d]\n",
		pctx->ltm_ctx.frame_height_stat,
		pctx->ltm_ctx.frame_width_stat,
		pctx->ltm_ctx.hists.tile_num_x_minus,
		pctx->ltm_ctx.hists.tile_num_y_minus,
		pctx->ltm_ctx.hists.tile_width,
		pctx->ltm_ctx.hists.tile_height);
#endif
	return ret;
}

static int isp_update_offline_param(
	struct isp_pipe_context *pctx,
	struct isp_offline_param *in_param)
{
	int ret = 0;
	int i;
	struct img_size *src_new = NULL;
	struct img_trim path_trim;
	struct isp_path_desc *path;
	struct isp_ctx_size_desc cfg;
	uint32_t update[ISP_SPATH_NUM] =
		{ISP_PATH0_TRIM, ISP_PATH1_TRIM, ISP_PATH2_TRIM};

	if (in_param->valid & ISP_SRC_SIZE) {
		memcpy(&pctx->original, &in_param->src_info,
			sizeof(pctx->original));
		cfg.src = in_param->src_info.dst_size;
		cfg.crop.start_x = 0;
		cfg.crop.start_y = 0;
		cfg.crop.size_x = cfg.src.w;
		cfg.crop.size_y = cfg.src.h;
		ret = isp_cfg_ctx_size(pctx, &cfg);
		pr_info("isp ctx %d update size: %d %d\n",
			pctx->ctx_id, cfg.src.w, cfg.src.h);
		src_new = &cfg.src;
	}

	/* update all path scaler trim0  */
	for (i = 0; i < ISP_SPATH_NUM; i++) {
		path = &pctx->isp_path[i];
		if (atomic_read(&path->user_cnt) < 1)
			continue;

		if (in_param->valid & update[i]) {
			path_trim = in_param->trim_path[i];
		} else if (src_new) {
			path_trim.start_x = path_trim.start_y = 0;
			path_trim.size_x = src_new->w;
			path_trim.size_y = src_new->h;
		} else {
			continue;
		}
		ret = isp_cfg_path_size(path, &path_trim);
		pr_info("update isp path%d trim %d %d %d %d\n",
			i, path_trim.start_x, path_trim.start_y,
			path_trim.size_x, path_trim.size_y);
	}
	pctx->updated = 1;
	return ret;
}

static int isp_offline_start_frame(void *ctx)
{
	int ret = 0;
	int i, loop, kick_fmcu = 0;
	uint32_t frame_id;
	struct isp_pipe_dev *dev = NULL;
	struct camera_frame *pframe = NULL;
	struct camera_frame *out_frame = NULL;
	struct isp_pipe_context *pctx;
	struct isp_path_desc *path, *slave_path;
	struct isp_cfg_ctx_desc *cfg_desc;
	struct isp_fmcu_ctx_desc *fmcu;
	struct isp_offline_param *in_param;

	pr_debug("enter.\n");

	pctx = (struct isp_pipe_context *)ctx;
	if (atomic_read(&pctx->user_cnt) < 1) {
		pr_err("isp cxt %d is not inited.\n", pctx->ctx_id);
		return -EINVAL;
	}

	dev = pctx->dev;
	cfg_desc = (struct isp_cfg_ctx_desc *)dev->cfg_handle;
	fmcu = (struct isp_fmcu_ctx_desc *)pctx->fmcu_handle;

	pframe = camera_dequeue(&pctx->in_queue);
	if (pframe == NULL) {
		pr_warn("no frame from input queue. cxt:%d\n",
				pctx->ctx_id);
		return 0;
	}

	if ((pframe->fid & 0xf) == 0)
		pr_info("frame %d, ctx %d  ch_id %d.  buf_fd %d\n",
			pframe->fid, pctx->ctx_id, pframe->channel_id, pframe->buf.mfd[0]);

	ret = cambuf_iommu_map(&pframe->buf, CAM_IOMMUDEV_ISP);
	if (ret) {
		pr_err("fail to map buf to ISP iommu. cxt %d\n", pctx->ctx_id);
		ret = -EINVAL;
		goto map_err;
	}

	frame_id = pframe->fid;
	loop = 0;
	do {
		ret = camera_enqueue(&pctx->proc_queue, pframe);
		if (ret == 0)
			break;
		printk_ratelimited(KERN_INFO "wait for proc queue. loop %d\n", loop);
		/* wait for previous frame proccessed done.*/
		mdelay(1);
	} while (loop++ < 500);

	if (ret) {
		pr_err("error: input frame queue tmeout.\n");
		ret = -EINVAL;
		goto inq_overflow;
	}

	/*
	 * param_mutex to avoid ctx/all paths param
	 * updated when set to register.
	 */
	mutex_lock(&pctx->param_mutex);

	in_param = (struct isp_offline_param *)pframe->param_data;
	if (in_param) {
		isp_update_offline_param(pctx, in_param);
		free_offline_pararm(in_param);
		pframe->param_data = NULL;
	}
	pframe->width = pctx->input_size.w;
	pframe->height = pctx->input_size.h;

	/*update NR param for crop/scaling image */
	isp_adapt_blkparam(pctx);

	/* the context/path maybe init/updated after dev start. */
	if (pctx->updated) {
		ret = isp_slice_ctx_init(pctx);
		isp_set_ctx_common(pctx);
	}

	/* config fetch address */
	isp_path_set_fetch_frm(pctx, pframe, &pctx->fetch.addr);

	/* Reset blending count if frame size change */
	isp_3dnr_process_frame_previous(pctx, pframe);

	/* config all paths output */
	for (i = 0; i < ISP_SPATH_NUM; i++) {
		path = &pctx->isp_path[i];
		if (atomic_read(&path->user_cnt) < 1)
			continue;

		if (pctx->updated)
			ret = isp_set_path(path);

		/* slave path output buffer binding to master buffer*/
		if (path->bind_type == ISP_PATH_SLAVE)
			continue;

		if ((pctx->mode_3dnr == MODE_3DNR_CAP) &&
		    (pctx->nr3_ctx.blending_cnt % 5 != 4)) {
			out_frame = camera_dequeue(&path->reserved_buf_queue);
			pr_debug("3dnr frame [0 ~ 3], discard cnt[%d][%d]\n",
				 pctx->nr3_ctx.blending_cnt,
				 pctx->nr3_ctx.blending_cnt % 5);
		} else {
			out_frame = camera_dequeue(&path->out_buf_queue);
			if (out_frame == NULL)
				out_frame =
				camera_dequeue(&path->reserved_buf_queue);
		}

		if (out_frame == NULL) {
			pr_debug("fail to get available output buffer.\n");
			ret = 0;
			goto unlock;
		}
		out_frame->fid = frame_id;
		out_frame->sensor_time = pframe->sensor_time;

		/* config store buffer */
		pr_debug("isp output buf, iova 0x%x, phy: 0x%x\n",
			 (uint32_t)out_frame->buf.iova[0],
			 (uint32_t)out_frame->buf.addr_k[0]);
		isp_path_set_store_frm(path, out_frame);

		if (path->bind_type == ISP_PATH_MASTER) {
			struct camera_frame temp;
			/* fixed buffer offset here. HAL should use same offset calculation method */
			temp.buf.iova[0] = out_frame->buf.iova[0] + path->store.total_size;
			temp.buf.iova[1] = temp.buf.iova[2] = 0;
			slave_path = &pctx->isp_path[path->slave_path_id];
			isp_path_set_store_frm(slave_path, &temp);
		}
		/*
		 * context proc_queue frame number
		 * should be equal to path result queue.
		 * if ctx->proc_queue enqueue OK,
		 * path result_queue enqueue should be OK.
		 */
		loop = 0;
		do {
			ret = camera_enqueue(&path->result_queue, out_frame);
			if (ret == 0)
				break;
			printk_ratelimited(KERN_INFO "wait for output queue. loop %d\n", loop);
			/* wait for previous frame output queue done */
			mdelay(1);
		} while (loop++ < 500);

		if (ret) {
			trace_isp_irq_cnt(pctx->ctx_id);
			pr_err("fatal: should not be here. path %d, store %d\n",
					path->spath_id,
					atomic_read(&path->store_cnt));
			/* ret frame to original queue */
			if (out_frame->is_reserved)
				camera_enqueue(
					&path->reserved_buf_queue, out_frame);
			else
				camera_enqueue(
					&path->out_buf_queue, out_frame);
			ret = -EINVAL;
			goto unlock;
		}
		atomic_inc(&path->store_cnt);
	}

	isp_3dnr_process_frame(pctx, pframe);
	isp_ltm_process_frame_previous(pctx, pframe);
	isp_ltm_process_frame(pctx, pframe);

	ret = wait_for_completion_interruptible_timeout(
					&pctx->frm_done,
					ISP_CONTEXT_TIMEOUT);
	if (ret == ERESTARTSYS) {
		pr_err("interrupt when isp wait\n");
		ret = -EFAULT;
		goto unlock;
	} else if (ret == 0) {
		pr_err("error: isp context %d timeout.\n", pctx->ctx_id);
		ret = -EFAULT;
		goto unlock;
	}

	if (fmcu)
		fmcu->ops->ctx_reset(fmcu);

	if (pctx->slice_ctx) {
		struct slice_cfg_input slc_cfg;

		memset(&slc_cfg, 0, sizeof(slc_cfg));
		for (i = 0; i < ISP_SPATH_NUM; i++) {
			path = &pctx->isp_path[i];
			if (atomic_read(&path->user_cnt) < 1)
				continue;
			slc_cfg.frame_store[i] = &path->store;
		}
		slc_cfg.frame_fetch = &pctx->fetch;
		isp_cfg_slice_fetch_info(&slc_cfg, pctx->slice_ctx);
		isp_cfg_slice_store_info(&slc_cfg, pctx->slice_ctx);

		/* 3DNR Capture case: Using CP Config */
		slc_cfg.frame_in_size.w = pctx->input_trim.size_x;
		slc_cfg.frame_in_size.h = pctx->input_trim.size_y;
		slc_cfg.nr3_ctx = &pctx->nr3_ctx;
		isp_cfg_slice_3dnr_info(&slc_cfg, pctx->slice_ctx);

		slc_cfg.ltm_ctx = &pctx->ltm_ctx;
		isp_cfg_slice_ltm_info(&slc_cfg, pctx->slice_ctx);

		if (!fmcu) {
			/* should not be here */
			pr_err("error: no fmcu to support slices.\n");
		} else {
			ret = isp_set_slices_fmcu_cmds((void *)fmcu,
				pctx->slice_ctx,
				pctx->ctx_id,
				(uint32_t)pctx->dev->wmode);
			if (ret == 0)
				kick_fmcu = 1;
		}
	}

	pctx->updated = 0;
	mutex_unlock(&pctx->param_mutex);

	/* start to prepare/kickoff cfg buffer. */
	if (likely(dev->wmode == ISP_CFG_MODE)) {
		pr_debug("cfg enter.");
		ret = cfg_desc->ops->hw_cfg(
				cfg_desc, pctx->ctx_id, kick_fmcu);

		if (kick_fmcu) {
			pr_info("fmcu start.");
			ret = fmcu->ops->hw_start(fmcu);
		} else {
			pr_debug("cfg start. fid %d\n", frame_id);
			ret = cfg_desc->ops->hw_start(
					cfg_desc, pctx->ctx_id);
		}
	} else {
		if (kick_fmcu) {
			pr_info("fmcu start.");
			ret = fmcu->ops->hw_start(fmcu);
		} else {
			pr_debug("fetch start.");
			ISP_HREG_WR(ISP_FETCH_START, 1);
		}
	}

	pr_debug("done.\n");
	return 0;

unlock:
	mutex_unlock(&pctx->param_mutex);
	for (i = i - 1; i >= 0; i--) {
		path = &pctx->isp_path[i];
		if (atomic_read(&path->user_cnt) < 1)
			continue;
		pframe = camera_dequeue_tail(&path->result_queue);
		/* ret frame to original queue */
		if (out_frame->is_reserved)
			camera_enqueue(
				&path->reserved_buf_queue, out_frame);
		else
			camera_enqueue(
				&path->out_buf_queue, out_frame);
		atomic_dec(&path->store_cnt);
	}

	pframe = camera_dequeue_tail(&pctx->proc_queue);
inq_overflow:
	cambuf_iommu_unmap(&pframe->buf);
map_err:
	free_offline_pararm(pframe->param_data);
	pframe->param_data = NULL;
	/* return buffer to cam channel shared buffer queue. */
	pctx->isp_cb_func(ISP_CB_RET_SRC_BUF, pframe, pctx->cb_priv_data);
	return ret;
}


static int isp_offline_thread_loop(void *arg)
{
	struct isp_pipe_dev *dev = NULL;
	struct isp_pipe_context *pctx;
	struct cam_thread_info *thrd;

	if (!arg) {
		pr_err("fail to get valid input ptr\n");
		return -1;
	}

	thrd = (struct cam_thread_info *)arg;
	pctx = (struct isp_pipe_context *)thrd->ctx_handle;
	dev = pctx->dev;

	while (1) {
		if (wait_for_completion_interruptible(
			&thrd->thread_com) == 0) {
			if (atomic_cmpxchg(
					&thrd->thread_stop, 1, 0) == 1) {
				pr_info("isp context %d thread stop.\n",
						pctx->ctx_id);
				break;
			}
			pr_debug("thread com done.\n");

			if (thrd->proc_func(pctx)) {
				pr_err("fail to start isp pipe to proc. exit thread\n");
				pctx->isp_cb_func(
					ISP_CB_DEV_ERR, dev,
					pctx->cb_priv_data);
				break;
			}
		} else {
			pr_debug("offline thread exit!");
			break;
		}
	}

	return 0;
}


static int isp_stop_offline_thread(void *param)
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


static int isp_create_offline_thread(void *param)
{
	struct isp_pipe_context *pctx;
	struct cam_thread_info *thrd;
	char thread_name[32] = { 0 };

	pctx = (struct isp_pipe_context *)param;
	thrd = &pctx->thread;
	thrd->ctx_handle = pctx;
	thrd->proc_func = isp_offline_start_frame;
	atomic_set(&thrd->thread_stop, 0);
	init_completion(&thrd->thread_com);

	sprintf(thread_name, "isp_ctx%d_offline", pctx->ctx_id);
	thrd->thread_task = kthread_run(
						isp_offline_thread_loop,
					      thrd, thread_name);
	if (IS_ERR_OR_NULL(thrd->thread_task)) {
		pr_err("fail to start offline thread for isp ctx%d\n",
				pctx->ctx_id);
		return -EFAULT;
	}

	pr_info("isp ctx %d offline thread created.\n", pctx->ctx_id);
	return 0;
}


static int isp_context_init(struct isp_pipe_dev *dev)
{
	int ret = 0;
	int i;
	struct isp_cfg_ctx_desc *cfg_desc = NULL;
	struct isp_pipe_context *pctx;
	enum isp_context_id cid[4] = {
		ISP_CONTEXT_P0,
		ISP_CONTEXT_C0,
		ISP_CONTEXT_P1,
		ISP_CONTEXT_C1
	};

	pr_info("isp contexts init start!\n");
	memset(&dev->ctx[0], 0, sizeof(dev->ctx));
	for (i = 0; i < ISP_CONTEXT_NUM; i++) {
		pctx  = &dev->ctx[i];
		pctx->ctx_id = cid[i];
		pctx->dev = dev;
		pctx->attach_cam_id = CAM_ID_MAX;
		atomic_set(&pctx->user_cnt, 0);
		pr_debug("isp context %d init done!\n", cid[i]);
	}

	/* CFG module init */
	if (dev->wmode == ISP_AP_MODE) {

		pr_info("isp ap mode.\n");
		for (i = 0; i < ISP_CONTEXT_NUM; i++)
			isp_cfg_poll_addr[i] = &s_isp_regbase[0];

	} else {
		cfg_desc = get_isp_cfg_ctx_desc();
		if (!cfg_desc || !cfg_desc->ops) {
			pr_err("error: get isp cfg ctx failed.\n");
			ret = -EINVAL;
			goto cfg_null;
		}
		pr_info("cfg_init start.\n");

		ret = cfg_desc->ops->ctx_init(cfg_desc);
		if (ret) {
			pr_err("error: cfg ctx init failed.\n");
			goto ctx_fail;
		}
		pr_info("cfg_init done.\n");

		ret = cfg_desc->ops->hw_init(cfg_desc);
		if (ret)
			goto hw_fail;
	}
	dev->cfg_handle = cfg_desc;

	dev->ltm_handle = isp_get_ltm_share_ctx_desc();

	pr_info("done!\n");
	return 0;

hw_fail:
	cfg_desc->ops->ctx_deinit(cfg_desc);
ctx_fail:
	put_isp_cfg_ctx_desc(cfg_desc);
cfg_null:
	pr_err("failed!\n");
	return ret;
}

static int isp_context_deinit(struct isp_pipe_dev *dev)
{
	int ret = 0;
	int i, j;
	uint32_t path_id;
	struct isp_cfg_ctx_desc *cfg_desc = NULL;
	struct isp_pipe_context *pctx;
	struct isp_path_desc *path;

	pr_info("enter.\n");

	for (i = 0; i < ISP_CONTEXT_NUM; i++) {
		pctx  = &dev->ctx[i];

		/* free all used path here if user did not call put_path  */
		for (j = 0; j < ISP_SPATH_NUM; j++) {
			path = &pctx->isp_path[j];
			path_id = path->spath_id;
			if (atomic_read(&path->user_cnt) > 0)
				sprd_isp_put_path(dev, pctx->ctx_id, path_id);
		}
		sprd_isp_put_context(dev, pctx->ctx_id);

		atomic_set(&pctx->user_cnt, 0);
		mutex_destroy(&pctx->param_mutex);
	}
	pr_info("isp contexts deinit done!\n");

	cfg_desc = (struct isp_cfg_ctx_desc *)dev->cfg_handle;
	if (cfg_desc) {
		cfg_desc->ops->ctx_deinit(cfg_desc);
		put_isp_cfg_ctx_desc(cfg_desc);
	}
	dev->cfg_handle = NULL;

	isp_put_ltm_share_ctx_desc(dev->ltm_handle);
	dev->ltm_handle = NULL;

	pr_info("done.\n");
	return ret;
}

static int isp_slice_ctx_init(struct isp_pipe_context *pctx)
{
	int ret = 0;
	int j;
	uint32_t val;
	struct isp_path_desc *path;
	struct slice_cfg_input slc_cfg_in;

	if (pctx->fmcu_handle == NULL) {
		pr_debug("no need slices.\n");
		return ret;
	}

	if (pctx->slice_ctx == NULL) {
		pctx->slice_ctx = get_isp_slice_ctx();
		if (IS_ERR_OR_NULL(pctx->slice_ctx)) {
			pr_err("fail to get memory for slice_ctx.\n");
			pctx->slice_ctx = NULL;
			ret = -ENOMEM;
			goto exit;
		}
	}

	memset(&slc_cfg_in, 0, sizeof(struct slice_cfg_input));
	slc_cfg_in.frame_in_size.w = pctx->input_trim.size_x;
	slc_cfg_in.frame_in_size.h = pctx->input_trim.size_y;
	slc_cfg_in.frame_fetch = &pctx->fetch;
	for (j = 0; j < ISP_SPATH_NUM; j++) {
		path = &pctx->isp_path[j];
		if (atomic_read(&path->user_cnt) <= 0)
			continue;
		slc_cfg_in.frame_out_size[j] = &path->dst;
		slc_cfg_in.frame_store[j] = &path->store;
		slc_cfg_in.frame_scaler[j] = &path->scaler;
		slc_cfg_in.frame_deci[j] = &path->deci;
		slc_cfg_in.frame_trim0[j] = &path->in_trim;
		slc_cfg_in.frame_trim1[j] = &path->out_trim;
	}
	val = ISP_REG_RD(pctx->ctx_id, ISP_NLM_RADIAL_1D_DIST);
	slc_cfg_in.nlm_center_x = val & 0x3FFF;
	slc_cfg_in.nlm_center_y = (val >> 16) & 0x3FFF;

	val = ISP_REG_RD(pctx->ctx_id, ISP_YNR_CFG31);
	slc_cfg_in.ynr_center_x = val & 0xFFFF;
	slc_cfg_in.ynr_center_y = (val >> 16) & 0xfFFF;

	isp_cfg_slices(&slc_cfg_in, pctx->slice_ctx);

exit:
	return ret;
}

/* offline process frame */
static int sprd_isp_proc_frame(void *isp_handle,
			void *param, int ctx_id)
{
	int ret = 0;
	struct camera_frame *pframe;
	struct isp_pipe_context *pctx;
	struct isp_pipe_dev *dev;

	if (!isp_handle || !param) {
		pr_err("fail to get valid input ptr\n");
		return -EFAULT;
	}
	if (ctx_id < 0 || ctx_id >= ISP_CONTEXT_NUM) {
		pr_err("Illegal. ctx_id %d\n", ctx_id);
		return -EFAULT;
	}

	dev = (struct isp_pipe_dev *)isp_handle;
	pctx = &dev->ctx[ctx_id];
	pframe = (struct camera_frame *)param;
	pframe->priv_data = pctx;

	pr_debug("frame %p, ctx %d  path %d, ch_id %d.  buf_fd %d\n", pframe,
		ctx_id, ctx_id, pframe->channel_id, pframe->buf.mfd[0]);
	ret = camera_enqueue(&pctx->in_queue, pframe);
	if (ret == 0)
		complete(&pctx->thread.thread_com);

	return ret;
}

/*
 * Get a free context and initialize it.
 * Input param is possible max_size of image.
 * Return valid context id, or -1 for failure.
 */
static int sprd_isp_get_context(void *isp_handle, void *param)
{
	int ret = 0;
	int i;
	int sel_ctx_id = -1;
	enum  isp_context_id  ctx_id;
	struct isp_pipe_context *pctx;
	struct isp_pipe_dev *dev;
	struct isp_path_desc *path;
	struct isp_cfg_ctx_desc *cfg_desc;
	struct isp_fmcu_ctx_desc *fmcu = NULL;
	struct sprd_cam_hw_info *hw;
	struct img_size *max_size;

	if (!isp_handle || !param) {
		pr_err("fail to get valid input ptr\n");
		return -EFAULT;
	}
	pr_debug("start.\n");

	dev = (struct isp_pipe_dev *)isp_handle;
	max_size = (struct img_size *)param;

	mutex_lock(&dev->path_mutex);

	if (unlikely(dev->wmode == ISP_AP_MODE)) {
		ctx_id = ISP_CONTEXT_P0;
		pctx = &dev->ctx[ctx_id];
		if (atomic_inc_return(&pctx->user_cnt) == 1) {
			sel_ctx_id = ctx_id;
			pr_info("AP mode. get new ctx: %d\n", ctx_id);
			goto new_ctx;
		} else {
			atomic_dec(&pctx->user_cnt);
			pr_err("AP mode only context0 can be used. Already in use.\n");
			goto exit;
		}
	}

	/* ISP cfg mode, get free context */
	for (i = 0; i < ISP_CONTEXT_NUM; i++) {
		ctx_id = i;
		pctx = &dev->ctx[ctx_id];
		if (atomic_inc_return(&pctx->user_cnt) == 1) {
			sel_ctx_id = ctx_id;
			break;
		}
		atomic_dec(&pctx->user_cnt);
	}
	if (sel_ctx_id == -1)
		goto exit;

new_ctx:
	if (dev->wmode == ISP_CFG_MODE) {
		cfg_desc = (struct isp_cfg_ctx_desc *)dev->cfg_handle;
		cfg_desc->ops->ctx_reset(cfg_desc, ctx_id);
	}

	for (i = 0; i < ISP_SPATH_NUM; i++) {
		path = &pctx->isp_path[i];
		path->spath_id = i;
		path->attach_ctx = pctx;
		path->q_init = 0;
		atomic_set(&path->user_cnt, 0);
	}

	mutex_init(&pctx->param_mutex);
	init_completion(&pctx->frm_done);
	/* complete for first frame config */
	complete(&pctx->frm_done);
	pctx->isp_k_param.nlm_info.bypass = 1;
	pctx->isp_k_param.ynr_info.bypass = 1;

	ret = isp_create_offline_thread(pctx);
	if (unlikely(ret != 0)) {
		pr_err("fail to create offline thread for isp cxt:%d\n",
				pctx->ctx_id);
		goto thrd_err;
	}

	if (max_size->w > line_buffer_len) {
		fmcu = get_isp_fmcu_ctx_desc();
		pr_info("ctx get fmcu %p\n", fmcu);
		if (fmcu == NULL) {
			pr_err("error: no fmcu for size %d, %d\n",
				max_size->w, max_size->h);
			goto no_fmcu;
		} else if (fmcu->ops) {
			ret = fmcu->ops->ctx_init(fmcu);
			if (ret) {
				pr_err("error: fmcu ctx init failed.\n");
				goto fmcu_err;
			}
		} else {
			pr_err("error: no fmcu ops.\n");
			goto fmcu_err;
		}
		pctx->fmcu_handle = fmcu;
	}

	camera_queue_init(&pctx->in_queue, ISP_IN_Q_LEN,
						0, isp_ret_src_frame);
	camera_queue_init(&pctx->proc_queue, ISP_PROC_Q_LEN,
						0, isp_ret_src_frame);
	camera_queue_init(&pctx->ltm_avail_queue, ISP_LTM_BUF_NUM,
						0, isp_unmap_frame);
	camera_queue_init(&pctx->ltm_wr_queue, ISP_LTM_BUF_NUM,
						0, isp_unmap_frame);
	isp_set_ctx_default(pctx);
	reset_isp_irq_cnt(pctx->ctx_id);

	hw = dev->isp_hw;
	ret = hw->ops->enable_irq(hw, &pctx->ctx_id);

	goto exit;

fmcu_err:
	if (fmcu)
		put_isp_fmcu_ctx_desc(fmcu);

no_fmcu:
thrd_err:
	atomic_dec(&pctx->user_cnt); /* free context */
	sel_ctx_id = -1;
exit:
	mutex_unlock(&dev->path_mutex);
	pr_info("done, ret context: %d\n", sel_ctx_id);
	return sel_ctx_id;
}


/*
 * Free a context and deinitialize it.
 * All paths of this context should be put before this.
 *
 * TODO:
 * we do not stop or reset ISP here because four contexts share it.
 * How to make sure current context process in ISP is done
 * before we clear buffer Q?
 * If ISP hw doesn't done buffer reading/writting,
 * we free buffer here may cause memory over-writting and perhaps system crash.
 * Delay buffer Q clear is a just solution to reduce this risk
 */
static int sprd_isp_put_context(void *isp_handle, int ctx_id)
{
	int ret = 0;
	int i;
	struct isp_pipe_dev *dev;
	struct isp_pipe_context *pctx;
	struct isp_path_desc *path;
	struct isp_fmcu_ctx_desc *fmcu;
	struct sprd_cam_hw_info *hw;

	if (!isp_handle) {
		pr_err("fail to get valid input ptr\n");
		return -EFAULT;
	}
	if (ctx_id < 0 || ctx_id >= ISP_CONTEXT_NUM) {
		pr_err("Illegal. ctx_id %d\n", ctx_id);
		return -EFAULT;
	}

	dev = (struct isp_pipe_dev *)isp_handle;
	pctx = &dev->ctx[ctx_id];

	mutex_lock(&dev->path_mutex);
	if (atomic_dec_return(&pctx->user_cnt) == 0) {
		pr_info("free context %d without users.\n", pctx->ctx_id);

		isp_stop_offline_thread(&pctx->thread);
		hw = dev->isp_hw;
		ret = hw->ops->disable_irq(hw, &pctx->ctx_id);
		ret = hw->ops->irq_clear(hw, &pctx->ctx_id);

		fmcu = (struct isp_fmcu_ctx_desc *)pctx->fmcu_handle;
		if (fmcu) {
			fmcu->ops->ctx_deinit(fmcu);
			put_isp_fmcu_ctx_desc(fmcu);
			pctx->fmcu_handle = NULL;
		}
		if (pctx->slice_ctx)
			put_isp_slice_ctx(&pctx->slice_ctx);

		camera_queue_clear(&pctx->in_queue);
		camera_queue_clear(&pctx->proc_queue);
#ifdef USING_LTM_Q
		camera_queue_clear(&pctx->ltm_avail_queue);
		camera_queue_clear(&pctx->ltm_wr_queue);
#else
		dev->ltm_handle->ops->set_status(0, ctx_id, pctx->mode_ltm);
		if (pctx->mode_ltm == MODE_LTM_PRE) {
			dev->ltm_handle->ops->complete_completion();
			for (i = 0; i < ISP_LTM_BUF_NUM; i++) {
				if (pctx->ltm_buf[i])
					isp_unmap_frame(pctx->ltm_buf[i]);
			}
		}
#endif /* USING_LTM_Q */

		for (i = 0; i < 2; i++) {
			if (pctx->nr3_buf[i])
				isp_unmap_frame(pctx->nr3_buf[i]);
		}

		/* clear path queue. */
		for (i = 0; i < ISP_SPATH_NUM; i++) {
			path = &pctx->isp_path[i];
			if (path->q_init == 0)
				continue;

			/* reserved buffer queue should be cleared at last. */
			camera_queue_clear(&path->result_queue);
			camera_queue_clear(&path->out_buf_queue);
			camera_queue_clear(&path->reserved_buf_queue);
		}

		trace_isp_irq_cnt(pctx->ctx_id);
	} else {
		pr_err("ctx %d is already release.\n", ctx_id);
		atomic_set(&pctx->user_cnt, 0);
	}
	mutex_unlock(&dev->path_mutex);
	pr_info("done, put ctx_id: %d\n", ctx_id);
	return ret;
}

/*
 * Enable path of sepicific path_id for specific context.
 * The context must be enable before get_path.
 */
static int sprd_isp_get_path(void *isp_handle, int ctx_id, int path_id)
{
	struct isp_pipe_context *pctx;
	struct isp_pipe_dev *dev;
	struct isp_path_desc *path;

	if (!isp_handle) {
		pr_err("fail to get valid input ptr\n");
		return -EFAULT;
	}

	pr_debug("start.\n");
	if (ctx_id < 0 || ctx_id >= ISP_CONTEXT_NUM ||
		path_id < 0 || path_id >= ISP_SPATH_NUM) {
		pr_err("Illegal. ctx_id %d, path_id %d\n", ctx_id, path_id);
		return -EFAULT;
	}

	dev = (struct isp_pipe_dev *)isp_handle;
	pctx = &dev->ctx[ctx_id];

	mutex_lock(&dev->path_mutex);

	if (atomic_read(&pctx->user_cnt) < 1) {
		mutex_unlock(&dev->path_mutex);
		pr_err("fail to get path from free context.\n");
		return -EFAULT;
	}

	path = &pctx->isp_path[path_id];
	if (atomic_inc_return(&path->user_cnt) > 1) {
		mutex_unlock(&dev->path_mutex);
		atomic_dec(&path->user_cnt);
		pr_err("path %d of cxt %d is already in use.\n",
			path_id, ctx_id);
		return -EFAULT;
	}

	mutex_unlock(&dev->path_mutex);

	path->frm_cnt = 0;
	atomic_set(&path->store_cnt, 0);

	if (path->q_init == 0) {
		camera_queue_init(&path->result_queue, ISP_RESULT_Q_LEN,
			0, isp_ret_out_frame);
		camera_queue_init(&path->out_buf_queue, ISP_OUT_BUF_Q_LEN,
			0, isp_ret_out_frame);
		camera_queue_init(&path->reserved_buf_queue,
			ISP_RESERVE_BUF_Q_LEN, 0, isp_destroy_reserved_buf);
		path->q_init = 1;
	}

	pr_info("get path %d done.", path_id);
	return 0;
}

/*
 * Disable path of sepicific path_id for specific context.
 * The path and context should be enable before this.
 */
static int sprd_isp_put_path(void *isp_handle, int ctx_id, int path_id)
{
	int ret = 0;
	struct isp_pipe_context *pctx;
	struct isp_pipe_dev *dev;
	struct isp_path_desc *path;

	if (!isp_handle) {
		pr_err("fail to get valid input ptr\n");
		return -EFAULT;
	}

	pr_debug("start.\n");
	if (ctx_id < 0 || ctx_id >= ISP_CONTEXT_NUM ||
		path_id < 0 || path_id >= ISP_SPATH_NUM) {
		pr_err("Illegal. ctx_id %d, path_id %d\n", ctx_id, path_id);
		return -EFAULT;
	}

	dev = (struct isp_pipe_dev *)isp_handle;
	pctx = &dev->ctx[ctx_id];

	mutex_lock(&dev->path_mutex);

	if (atomic_read(&pctx->user_cnt) < 1) {
		mutex_unlock(&dev->path_mutex);
		pr_err("fail to free path for free context.\n");
		return -EFAULT;
	}

	path = &pctx->isp_path[path_id];

	if (atomic_read(&path->user_cnt) == 0) {
		mutex_unlock(&dev->path_mutex);
		pr_err("isp cxt %d, path %d is not in use.\n",
					ctx_id, path_id);
		return -EFAULT;
	}

	if (atomic_dec_return(&path->user_cnt) != 0) {
		pr_warn("isp cxt %d, path %d has multi users.\n",
					ctx_id, path_id);
		atomic_set(&path->user_cnt, 0);
	}

	mutex_unlock(&dev->path_mutex);
	pr_info("done, put path_id: %d for ctx %d\n", path_id, ctx_id);
	return ret;
}

static int sprd_isp_cfg_path(void *isp_handle,
			enum isp_path_cfg_cmd cfg_cmd,
			int ctx_id, int path_id,
			void *param)
{
	int ret = 0;
	int i;
	struct isp_pipe_context *pctx;
	struct isp_pipe_dev *dev;
	struct isp_path_desc *path = NULL;
	struct isp_path_desc *slave_path;
	struct camera_frame *pframe;

	if (!isp_handle || !param) {
		pr_err("fail to get valid input ptr\n");
		return -EFAULT;
	}

	if (ctx_id >= ISP_CONTEXT_NUM) {
		pr_err("error id. ctx %d\n", ctx_id);
		return -EFAULT;
	}
	dev = (struct isp_pipe_dev *)isp_handle;
	pctx = &dev->ctx[ctx_id];

	if ((cfg_cmd != ISP_PATH_CFG_CTX_BASE) &&
		(cfg_cmd != ISP_PATH_CFG_CTX_SIZE)) {
		if (path_id >= ISP_SPATH_NUM) {
			pr_err("error id. path %d\n", path_id);
			return -EFAULT;
		}
		path = &pctx->isp_path[path_id];
		if (atomic_read(&path->user_cnt) == 0) {
			pr_err("isp cxt %d, path %d is not in use.\n",
					ctx_id, path_id);
			return -EFAULT;
		}
	}

	switch (cfg_cmd) {
	case ISP_PATH_CFG_OUTPUT_RESERVED_BUF:
	case ISP_PATH_CFG_OUTPUT_BUF:
		pr_debug("cfg buf path %d, %p\n",
			path->spath_id, pframe);
		pframe = (struct camera_frame *)param;
		ret = cambuf_iommu_map(
				&pframe->buf, CAM_IOMMUDEV_ISP);
		if (ret) {
			pr_err("isp buf iommu map failed.\n");
			ret = -EINVAL;
			goto exit;
		}

		/* is_reserved:
		 *  1:  basic mapping reserved buffer;
		 *  2:  copy of reserved buffer.
		 */
		if (unlikely(cfg_cmd == ISP_PATH_CFG_OUTPUT_RESERVED_BUF)) {
			struct camera_frame *newfrm;

			pframe->is_reserved = 1;
			pframe->priv_data = path;
			pr_info("reserved buf\n");
			ret = camera_enqueue(
					&path->reserved_buf_queue, pframe);
			i = 1;
			while (i < ISP_RESERVE_BUF_Q_LEN) {
				newfrm = get_empty_frame();
				if (newfrm) {
					newfrm->is_reserved = 2;
					newfrm->priv_data = path;
					memcpy(&newfrm->buf,
						&pframe->buf,
						sizeof(pframe->buf));
					camera_enqueue(
						&path->reserved_buf_queue,
						newfrm);
					i++;
				}
			}
		} else {
			pr_debug("output buf\n");
			pframe->is_reserved = 0;
			pframe->priv_data = path;
			ret = camera_enqueue(
					&path->out_buf_queue, pframe);
			if (ret) {
				pr_err("isp path %d output buffer en queue failed.\n",
						path_id);
				cambuf_iommu_unmap(&pframe->buf);
				goto exit;
			}
		}
		break;

	case ISP_PATH_CFG_3DNR_BUF:
		pframe = (struct camera_frame *)param;
		ret = cambuf_iommu_map(
				&pframe->buf, CAM_IOMMUDEV_ISP);
		if (ret) {
			pr_err("isp buf iommu map failed.\n");
			ret = -EINVAL;
			goto exit;
		}
		for (i = 0; i < ISP_NR3_BUF_NUM; i++) {
			if (pctx->nr3_buf[i] == NULL) {
				pctx->nr3_buf[i] = pframe;
				break;
			}
		}
		if (i == 2) {
			pr_err("isp ctx %d nr3 buffers already set.\n", ctx_id);
			cambuf_iommu_unmap(&pframe->buf);
			goto exit;
		}
		break;

	case ISP_PATH_CFG_LTM_BUF:
		pframe = (struct camera_frame *)param;
		if (pctx->mode_ltm == MODE_LTM_PRE) {
			ret = cambuf_iommu_map(
				 &pframe->buf, CAM_IOMMUDEV_ISP);
			if (ret) {
				pr_err("isp buf iommu map failed.\n");
				ret = -EINVAL;
				goto exit;
			}
		}
		for (i = 0; i < ISP_LTM_BUF_NUM; i++) {
			if (pctx->ltm_buf[i] == NULL) {
				pctx->ltm_buf[i] = pframe;
				pctx->ltm_ctx.pbuf[i] = &pframe->buf;
				break;
			}
		}
		pr_debug("isp ctx [%d], ltm buf idx [%d], buf addr [0x%p]\n",
			pctx->ctx_id, i, pframe);
#ifdef USING_LTM_Q
		ret = camera_enqueue(&pctx->ltm_avail_queue, pframe);
		if (ret) {
			cambuf_iommu_unmap(&pframe->buf);
			pr_err("isp ctx %d ltm mode %d cfg buf failed.\n",
					pctx->ctx_id, pctx->mode_ltm);
		}
#endif /* USING_LTM_Q */
		break;

	case ISP_PATH_CFG_CTX_BASE:
		mutex_lock(&pctx->param_mutex);
		ret = isp_cfg_ctx_base(pctx, param);
		pctx->updated = 1;
		mutex_unlock(&pctx->param_mutex);
		break;

	case ISP_PATH_CFG_CTX_SIZE:
		mutex_lock(&pctx->param_mutex);
		ret = isp_cfg_ctx_size(pctx, param);
		pctx->updated = 1;
		mutex_unlock(&pctx->param_mutex);
		break;

	case ISP_PATH_CFG_PATH_BASE:
		mutex_lock(&pctx->param_mutex);
		ret = isp_cfg_path_base(path, param);
		pctx->updated = 1;
		mutex_unlock(&pctx->param_mutex);
		break;

	case ISP_PATH_CFG_PATH_SIZE:
		mutex_lock(&pctx->param_mutex);
		ret = isp_cfg_path_size(path, param);
		if (path->bind_type == ISP_PATH_MASTER) {
			slave_path = &pctx->isp_path[path->slave_path_id];
			ret = isp_cfg_path_size(slave_path, param);
		}
		pctx->updated = 1;
		mutex_unlock(&pctx->param_mutex);
		break;

	default:
		pr_warn("unsupported cmd: %d\n", cfg_cmd);
		break;
	}
exit:
	pr_debug("cfg path %d done. ret %d\n", path->spath_id, ret);
	return ret;
}


typedef int (*func_isp_cfg_param)(
	struct isp_io_param *param, uint32_t idx);

struct isp_cfg_entry {
	uint32_t sub_block;
	func_isp_cfg_param cfg_func;
};

static struct isp_cfg_entry cfg_func_tab[ISP_BLOCK_TOTAL - ISP_BLOCK_BASE] = {
[ISP_BLOCK_BCHS - ISP_BLOCK_BASE]	= {ISP_BLOCK_BCHS, isp_k_cfg_bchs},
[ISP_BLOCK_CCE - ISP_BLOCK_BASE]	= {ISP_BLOCK_CCE, isp_k_cfg_cce},
[ISP_BLOCK_CDN - ISP_BLOCK_BASE]	= {ISP_BLOCK_CDN, isp_k_cfg_cdn},
[ISP_BLOCK_CFA - ISP_BLOCK_BASE]	= {ISP_BLOCK_CFA, isp_k_cfg_cfa},
[ISP_BLOCK_CMC - ISP_BLOCK_BASE]	= {ISP_BLOCK_CMC, isp_k_cfg_cmc10},
[ISP_BLOCK_EDGE - ISP_BLOCK_BASE]	= {ISP_BLOCK_EDGE, isp_k_cfg_edge},
[ISP_BLOCK_GAMMA - ISP_BLOCK_BASE]	= {ISP_BLOCK_GAMMA, isp_k_cfg_gamma},
[ISP_BLOCK_GRGB - ISP_BLOCK_BASE]	= {ISP_BLOCK_GRGB, isp_k_cfg_grgb},
[ISP_BLOCK_HIST2 - ISP_BLOCK_BASE]	= {ISP_BLOCK_HIST2, isp_k_cfg_hist2},
[ISP_BLOCK_HSV - ISP_BLOCK_BASE]	= {ISP_BLOCK_HSV, isp_k_cfg_hsv},
[ISP_BLOCK_IIRCNR - ISP_BLOCK_BASE]	= {ISP_BLOCK_IIRCNR, isp_k_cfg_iircnr},
[ISP_BLOCK_LTM - ISP_BLOCK_BASE]  = {ISP_BLOCK_LTM, isp_k_cfg_ltm},
[ISP_BLOCK_POST_CDN - ISP_BLOCK_BASE]	= {ISP_BLOCK_POST_CDN, isp_k_cfg_post_cdn},
[ISP_BLOCK_PRE_CDN - ISP_BLOCK_BASE]	= {ISP_BLOCK_PRE_CDN, isp_k_cfg_pre_cdn},
[ISP_BLOCK_PSTRZ - ISP_BLOCK_BASE]	= {ISP_BLOCK_PSTRZ, isp_k_cfg_pstrz},
[ISP_BLOCK_UVD - ISP_BLOCK_BASE]	= {ISP_BLOCK_UVD, isp_k_cfg_uvd},
[ISP_BLOCK_YGAMMA - ISP_BLOCK_BASE]	= {ISP_BLOCK_YGAMMA, isp_k_cfg_ygamma},
[ISP_BLOCK_YRANDOM - ISP_BLOCK_BASE]	= {ISP_BLOCK_YRANDOM, isp_k_cfg_yrandom},
[ISP_BLOCK_NOISEFILTER - ISP_BLOCK_BASE] = {ISP_BLOCK_NOISEFILTER, isp_k_cfg_yuv_noisefilter},
};

static int sprd_isp_cfg_blkparam(
	void *isp_handle, int ctx_id, void *param)
{
	int ret = 0;
	int i;
	struct isp_pipe_context *pctx;
	struct isp_pipe_dev *dev;
	struct isp_cfg_entry *cfg_entry = NULL;
	struct isp_io_param *io_param;

	if (!isp_handle || !param) {
		pr_err("fail to get valid input ptr\n");
		return -EFAULT;
	}
	if (ctx_id < 0 || ctx_id >= ISP_CONTEXT_NUM) {
		pr_err("Illegal. ctx_id %d\n", ctx_id);
		return -EINVAL;
	}

	dev = (struct isp_pipe_dev *)isp_handle;
	pctx = &dev->ctx[ctx_id];
	io_param = (struct isp_io_param *)param;
	if (atomic_read(&pctx->user_cnt) < 1) {
		pr_err("isp ctx %d not enable.\n", ctx_id);
		return -EINVAL;
	}

	/* todo: enable block config one by one */
	/* because parameters from user may be illegal when bringup*/
	/* temp disable during bringup */
	return 0;

	if (io_param->sub_block == ISP_BLOCK_NLM)
		ret = isp_k_cfg_nlm(param, &pctx->isp_k_param, ctx_id);
	else if (io_param->sub_block == ISP_BLOCK_3DNR)
		ret = isp_k_cfg_3dnr(param, &pctx->isp_k_param, ctx_id);
	else if (io_param->sub_block == ISP_BLOCK_YNR) {
		mutex_lock(&pctx->param_mutex);
		pctx->isp_k_param.src.w = pctx->input_trim.size_x;
		pctx->isp_k_param.src.h = pctx->input_trim.size_y;
		mutex_unlock(&pctx->param_mutex);
		ret = isp_k_cfg_ynr(param, &pctx->isp_k_param, ctx_id);
	} else {
		i = io_param->sub_block - ISP_BLOCK_BASE;
		if (cfg_func_tab[i].sub_block == io_param->sub_block)
			cfg_entry = &cfg_func_tab[i];
		else
			pr_err("sub_block %d error\n", io_param->sub_block);
		if (cfg_entry && cfg_entry->cfg_func)
			ret = cfg_entry->cfg_func(io_param, ctx_id);
		else
			pr_info("isp block 0x%x is not supported.\n",
				io_param->sub_block);
	}

	return ret;
}


static int sprd_isp_set_sb(void *isp_handle, int ctx_id,
		isp_dev_callback cb, void *priv_data)
{
	int ret = 0;
	struct isp_pipe_dev *dev = NULL;
	struct isp_pipe_context *pctx;

	if (!isp_handle || !cb || !priv_data) {
		pr_err("fail to get valid input ptr\n");
		return -EFAULT;
	}
	if (ctx_id < 0 || ctx_id >= ISP_CONTEXT_NUM) {
		pr_err("Illegal. ctx_id %d\n", ctx_id);
		return -EFAULT;
	}

	dev = (struct isp_pipe_dev *)isp_handle;
	pctx = &dev->ctx[ctx_id];
	if (pctx->isp_cb_func == NULL) {
		pctx->isp_cb_func = cb;
		pctx->cb_priv_data = priv_data;
		pr_info("ctx: %d, cb %p, %p\n",
			ctx_id, cb, priv_data);
	}

	return ret;
}


static int sprd_isp_dev_open(void *isp_handle, void *param)
{
	int ret = 0;
	struct isp_pipe_dev *dev = NULL;
	struct sprd_cam_hw_info *hw;

	pr_info("enter.\n");
	if (!isp_handle || !param) {
		pr_err("fail to get valid input ptr\n");
		return -EFAULT;
	}
	dev = (struct isp_pipe_dev *)isp_handle;
	hw = (struct sprd_cam_hw_info *)param;
	if (hw->ops == NULL) {
		pr_err("error: no hw ops.\n");
		return -EFAULT;
	}

	if (atomic_inc_return(&dev->enable) == 1) {

		pr_info("isp dev init start.\n");

		/* line_buffer_len for debug */
		if (s_dbg_linebuf_len > 0 &&
			s_dbg_linebuf_len <= ISP_LINE_BUFFER_W)
			line_buffer_len = s_dbg_linebuf_len;
		else
			line_buffer_len = ISP_LINE_BUFFER_W;

		pr_info("work mode: %d,  line_buf_len: %d\n",
			s_dbg_work_mode, line_buffer_len);

		dev->wmode = s_dbg_work_mode;
		dev->isp_hw = param;
		mutex_init(&dev->path_mutex);

		ret = sprd_cam_pw_on();
		ret = sprd_cam_domain_eb();

		ret = hw->ops->enable_clk(hw, NULL);
		if (ret)
			goto clk_fail;

		ret = hw->ops->reset(hw, NULL);
		if (ret)
			goto reset_fail;

		ret = hw->ops->init(hw, dev);
		if (ret)
			goto reset_fail;

		ret = hw->ops->start(dev->isp_hw, dev);

		ret = isp_context_init(dev);
		if (ret) {
			pr_err("error: isp context init failed.\n");
			ret = -EFAULT;
			goto err_init;
		}
	}

	pr_info("open isp pipe dev done!\n");
	return 0;

err_init:
	hw->ops->stop(hw, dev);
	hw->ops->deinit(hw, dev);
reset_fail:
	hw->ops->disable_clk(hw, NULL);
clk_fail:
	sprd_cam_domain_disable();
	sprd_cam_pw_off();

	atomic_dec(&dev->enable);
	pr_err("fail to open isp dev!\n");
	return ret;
}



int sprd_isp_dev_close(void *isp_handle)
{
	int ret = 0;
	struct isp_pipe_dev *dev = NULL;
	struct sprd_cam_hw_info *hw;

	if (!isp_handle) {
		pr_err("fail to get valid input ptr\n");
		return -EINVAL;
	}

	dev = (struct isp_pipe_dev *)isp_handle;
	hw = dev->isp_hw;
	if (atomic_dec_return(&dev->enable) == 0) {

		ret = hw->ops->stop(hw, dev);

		ret = isp_context_deinit(dev);
		mutex_destroy(&dev->path_mutex);

		ret = hw->ops->reset(hw, NULL);
		ret = hw->ops->deinit(hw, dev);
		ret = hw->ops->disable_clk(hw, NULL);

		sprd_cam_domain_disable();
		sprd_cam_pw_off();
	}

	pr_info("isp dev disable done\n");
	return ret;

}


static int sprd_isp_dev_reset(void *isp_handle, void *param)
{
	int ret = 0;

	if (!isp_handle || !param) {
		pr_err("fail to get valid input ptr\n");
		return -EFAULT;
	}
	return ret;
}

static struct isp_pipe_ops isp_ops = {
	.open = sprd_isp_dev_open,
	.close = sprd_isp_dev_close,
	.reset = sprd_isp_dev_reset,
	.get_context = sprd_isp_get_context,
	.put_context = sprd_isp_put_context,
	.get_path = sprd_isp_get_path,
	.put_path = sprd_isp_put_path,
	.cfg_path = sprd_isp_cfg_path,
	.cfg_blk_param = sprd_isp_cfg_blkparam,
	.proc_frame = sprd_isp_proc_frame,
	.set_callback = sprd_isp_set_sb,
};

struct isp_pipe_ops *get_isp_ops(void)
{
	return &isp_ops;
}

void *get_isp_pipe_dev(void)
{
	struct isp_pipe_dev *dev = NULL;

	mutex_lock(&isp_pipe_dev_mutex);

	if (s_isp_dev) {
		atomic_inc(&s_isp_dev->user_cnt);
		dev = s_isp_dev;
		goto exit;
	}

	dev = vzalloc(sizeof(struct isp_pipe_dev));
	if (!dev)
		goto exit;

	atomic_set(&dev->user_cnt, 1);
	atomic_set(&dev->enable, 0);
	s_isp_dev = dev;

exit:
	mutex_unlock(&isp_pipe_dev_mutex);

	if (dev)
		pr_info("get isp pipe dev: %p, users %d\n",
			dev, atomic_read(&dev->user_cnt));
	else
		pr_err("error: no memory for isp pipe dev.\n");

	return dev;
}


int put_isp_pipe_dev(void *isp_handle)
{
	int ret = 0;
	int user_cnt, en_cnt;
	struct isp_pipe_dev *dev = NULL;

	if (!isp_handle) {
		pr_err("fail to get valid input ptr\n");
		return -EFAULT;
	}

	dev = (struct isp_pipe_dev *)isp_handle;

	pr_info("put isp pipe dev: %p, %p, users: %d\n",
		dev, s_isp_dev, atomic_read(&dev->user_cnt));

	mutex_lock(&isp_pipe_dev_mutex);

	if (dev != s_isp_dev) {
		mutex_unlock(&isp_pipe_dev_mutex);
		pr_err("error: mismatched dev: %p, %p\n",
					dev, s_isp_dev);
		return -EINVAL;
	}

	user_cnt = atomic_read(&dev->user_cnt);
	en_cnt = atomic_read(&dev->enable);
	if (user_cnt != (en_cnt + 1))
		pr_err("error: mismatched %d %d\n",
				user_cnt, en_cnt);

	if (atomic_dec_return(&dev->user_cnt) == 0) {
		vfree(dev);
		s_isp_dev = NULL;
		pr_info("free isp pipe dev %p\n", dev);
	}
	mutex_unlock(&isp_pipe_dev_mutex);

	pr_info("put isp pipe dev: %p\n", dev);

	return ret;
}
