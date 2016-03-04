/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "isp_u_dev"

#include <pthread.h>
#include <semaphore.h>
#include "isp_drv.h"
#include "isp_common_types.h"

#define isp_fw_size            0x200000

static char isp_dev_name[50] = "/dev/sprd_isp";
static char isp_fw_name[50] = "/system/lib/firmware/TBM_G2v1DDR.bin";

struct isp_fw_mem {
	cmr_uint virt_addr;
	cmr_uint phy_addr;
	cmr_int mfd;
	cmr_u32 size;
	cmr_u32 num;
};

struct isp_file {
	int                        fd;
	int                       camera_id;
	cmr_handle                 evt_3a_handle;
	pthread_mutex_t            cb_mutex;
	pthread_t                  thread_handle;
	isp_evt_cb                 isp_event_cb;  //isp event callback
	struct isp_dev_init_info   init_param;
	sem_t                      close_sem;
	struct isp_fw_mem          fw_mem;
	cmr_int                                isp_is_inited;
};

static cmr_int isp_dev_create_thread(isp_handle handle);
static cmr_int isp_dev_kill_thread(isp_handle handle);
static void* isp_dev_thread_proc(void* data);

cmr_int isp_dev_init(struct isp_dev_init_info *init_param_ptr, isp_handle *handle)
{
	cmr_int                       ret = 0;
	cmr_int                       fd = -1;
	struct isp_dev_init_param     init_param;
	struct isp_file               *file = NULL;

	file = malloc(sizeof(struct isp_file));
	if (!file) {
		ret = -1;
		CMR_LOGE("alloc memory error.");
		return ret;
	}
	if (NULL == init_param_ptr) {
		ret = -1;
		CMR_LOGE("init param null error.");
		goto isp_free;
	}

	/*file->init_param = *init_param_ptr;*/
	memcpy((void *)&file->init_param, init_param_ptr, sizeof(struct isp_dev_init_info));
	fd = open(isp_dev_name, O_RDWR, 0);
	if (fd < 0) {
		ret = -1;
		CMR_LOGE("isp_dev_open error.");
		goto isp_free;
	}
	file->fd = fd;

	CMR_LOGE("isp_dev fd 0x%x", file->fd);
	init_param.camera_id = init_param_ptr->camera_id;
	init_param.width = init_param_ptr->width;
	init_param.height = init_param_ptr->height;
	ret= ioctl(file->fd, ISP_IO_SET_INIT_PARAM, &init_param);
	if (ret) {
		CMR_LOGE("isp set initial param error.");
		goto isp_free;
	}

	sem_init(&file->close_sem, 0, 0);

	ret = pthread_mutex_init(&file->cb_mutex, NULL);
	if (ret) {
		CMR_LOGE("Failed to init mutex : %d", errno);
		goto isp_free;
	}

	file->camera_id = init_param_ptr->camera_id;
	file->isp_is_inited = 0;
	*handle = (isp_handle)file;

	/*create isp dev thread*/
	ret = isp_dev_create_thread((isp_handle)file);
	if (ret) {
		CMR_LOGE("isp dev create thread failed.");
	} else {
		CMR_LOGI("isp device init ok.");
	}
	return ret;

isp_free:
	if (file)
		free(file);
	file = NULL;

	return ret;
}

cmr_int isp_dev_deinit(isp_handle handle)
{
	cmr_int                       ret = 0;
	struct isp_file               *file = (struct isp_file*)handle;

	if (!file) {
		ret = -1;
		CMR_LOGE("isp_handle is null error.");
		return ret;
	}

	ret = isp_dev_kill_thread(handle);
	if (ret) {
		CMR_LOGE("Failed to kill the thread. errno : %d", errno);
		return ret;
	}

	if (file->isp_is_inited) {
		if (-1 != file->fd) {
			sem_wait(&file->close_sem);
			if (-1 == close(file->fd)) {
				CMR_LOGE("close error.");
			}
		} else {
			CMR_LOGE("isp_dev_close error.");
		}

		file->init_param.free_cb(CAMERA_ISP_BINGING4AWB, file->init_param.mem_cb_handle,
			(cmr_uint*)file->fw_mem.phy_addr, (cmr_uint*)file->fw_mem.virt_addr, file->fw_mem.num);
	}

	pthread_mutex_lock(&file->cb_mutex);
	file->isp_event_cb = NULL;
	pthread_mutex_unlock(&file->cb_mutex);

	pthread_mutex_destroy(&file->cb_mutex);
	sem_destroy(&file->close_sem);

	file->isp_is_inited = 0;
	free(file);

	return ret;
}

cmr_int isp_dev_start(isp_handle handle)
{
	cmr_int                       ret = 0;
	struct isp_file          *file = (struct isp_file*)handle;
	struct isp_init_mem_param    load_input;
	cmr_int                                         fw_size = 0;
	cmr_u32                                       fw_buf_num = 1;
	cmr_u32                                       kaddr[2];
	cmr_u64                                       kaddr_temp;

	if (file->isp_is_inited) {
		CMR_LOGE("isp firmware no need load again ");
		goto exit;
	}

	ret = isp_dev_capability_fw_size((isp_handle)file, &fw_size);
	if (ret) {
		CMR_LOGE("failed to get fw size %ld", ret);
		goto exit;
	}

	CMR_LOGE("test 1");
	memset(&load_input, 0x00, sizeof(load_input));
	file->init_param.alloc_cb(CAMERA_ISP_BINGING4AWB, file->init_param.mem_cb_handle,
									 (cmr_u32*)&fw_size, &fw_buf_num, (cmr_uint*)kaddr,
									 &load_input.fw_buf_vir_addr, &load_input.fw_buf_mfd);
	if (ret) {
		CMR_LOGE("faield to alloc fw buffer %ld", ret);
		goto exit;
	}

	kaddr_temp = kaddr[1];
	load_input.fw_buf_phy_addr = (kaddr[0] | kaddr_temp << 32);
	CMR_LOGE("fw_buf_phy_addr 0x%llx,", load_input.fw_buf_phy_addr);
	CMR_LOGE("kaddr0 0x%x kaddr1 0x%x", kaddr[0], kaddr[1]);
	load_input.fw_buf_size = fw_size;
	file->fw_mem.virt_addr = load_input.fw_buf_vir_addr;
	file->fw_mem.phy_addr = load_input.fw_buf_phy_addr;
	file->fw_mem.mfd = load_input.fw_buf_mfd;
	file->fw_mem.size = load_input.fw_buf_size;
	file->fw_mem.num = fw_buf_num;

	ret = isp_dev_load_firmware((isp_handle)file, &load_input);
	if ((0 != ret) ||(0 == load_input.fw_buf_phy_addr)) {
		CMR_LOGE("failed to load firmware %ld", ret);
		goto exit;
	}

	file->isp_is_inited = 1;
exit:
	return ret;
}

void isp_dev_evt_reg(isp_handle handle, isp_evt_cb isp_event_cb, void *privdata)
{
	struct isp_file          *file = (struct isp_file*)handle;

	if (!file) {
		CMR_LOGE("isp_handle is null error.");
		return;
	}

	pthread_mutex_lock(&file->cb_mutex);
	file->isp_event_cb = isp_event_cb;
	file->evt_3a_handle = privdata;
	pthread_mutex_unlock(&file->cb_mutex);
	return;
}

static cmr_int isp_dev_create_thread(isp_handle handle)
{
	cmr_int                  ret = 0;
	pthread_attr_t           attr;
	struct isp_file          *file;

	file = (struct isp_file*)handle;
	if (!file) {
		ret = -1;
		CMR_LOGE("file hand is null error.");
		return ret;
	}

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	ret = pthread_create(&file->thread_handle, &attr, isp_dev_thread_proc, (void*)handle);
	pthread_attr_destroy(&attr);

	return ret;
}

static cmr_int isp_dev_kill_thread(isp_handle handle)
{
	cmr_int                    ret = 0;
	struct isp_img_write_op    write_op;
	struct isp_file            *file;
	void                       *dummy;

	file = (struct isp_file*)handle;
	if (!file) {
		ret = -1;
		CMR_LOGE("file hand is null error.");
		return ret;
	}

	CMR_LOGI("kill isp proc thread.");
	memset(&write_op, 0, sizeof(struct isp_img_write_op));
	write_op.cmd = ISP_IMG_STOP_ISP;
	ret = write(file->fd, &write_op, sizeof(struct isp_img_write_op));
	if (ret > 0) {
		CMR_LOGI("write OK!");
		ret = pthread_join(file->thread_handle, &dummy);
		file->thread_handle = 0;
	}

	return ret;
}

static void* isp_dev_thread_proc(void *data)
{
	struct isp_file                  *file = NULL;
	struct isp_statis_frame_output    statis_frame;
	struct isp_statis_frame           statis_frame_buf;
	struct isp_irq                    irq_node;
	struct isp_irq_info               irq_info;

	file = (struct isp_file*)data;
	CMR_LOGI("isp dev thread file %p ", file);
	if(!file) {
		return NULL;
	}

	if(-1 == file->fd) {
		return NULL;
	}

	CMR_LOGI("isp dev thread in.");
	while (1) {
		/*Get irq*/
		if (-1 == ioctl(file->fd, ISP_IO_IRQ, &irq_info)) {
			CMR_LOGI("ISP_IO_IRQ error.");
			break;
		} else {
			if(ISP_IMG_TX_STOP == irq_info.irq_flag) {
				CMR_LOGE("isp_dev_thread_proc exit.");
				break;
			} else if (ISP_IMG_SYS_BUSY == irq_info.irq_flag) {
				cmr_usleep(100);
				CMR_LOGI("continue.");
				continue;
			} else if (ISP_IMG_NO_MEM == irq_info.irq_flag){
				CMR_LOGE("statistics no mem");
				continue;
			} else {
				if(ISP_IMG_TX_DONE == irq_info.irq_flag) {
					if (irq_info.irq_type == ISP_IRQ_STATIS) {
						CMR_LOGI("got one frame statis vaddr 0x%lx paddr 0x%lx buf_size 0x%lx", irq_info.yaddr_vir, irq_info.yaddr, irq_info.length);
						statis_frame.format = irq_info.format;
						statis_frame.buf_size = irq_info.length;
						statis_frame.phy_addr = irq_info.yaddr;
						statis_frame.vir_addr = irq_info.yaddr_vir;
						statis_frame.time_stamp.sec = irq_info.time_stamp.sec;
						statis_frame.time_stamp.usec = irq_info.time_stamp.usec;
						pthread_mutex_lock(&file->cb_mutex);
						if(file->isp_event_cb) {
							(*file->isp_event_cb)(ISP_DRV_STATISTICE, &statis_frame, (void *)file->evt_3a_handle);
						}
						pthread_mutex_unlock(&file->cb_mutex);
					} else if (irq_info.irq_type == ISP_IRQ_3A_SOF) {
						CMR_LOGI("got one sof");
						irq_node.irq_val0 = irq_info.irq_id;
						irq_node.reserved = 0;
						irq_node.ret_val = 0;
						irq_node.time_stamp.sec = irq_info.time_stamp.sec;
						irq_node.time_stamp.usec = irq_info.time_stamp.usec;
						pthread_mutex_lock(&file->cb_mutex);
						if(file->isp_event_cb) {
							(*file->isp_event_cb)(ISP_DRV_SENSOR_SOF, &irq_node, (void *)file->evt_3a_handle);
						}
						pthread_mutex_unlock(&file->cb_mutex);
					}
				}
			}
		}
	}

	sem_post(&file->close_sem);
	CMR_LOGI("isp dev thread out.");

	return NULL;
}
cmr_int isp_dev_open(isp_handle *handle, struct isp_dev_init_param *param)
{
	cmr_int ret = 0;
	cmr_int fd = -1;
	struct isp_file *file = NULL;
	struct isp_dev_init_param *init_param = NULL;

	file = malloc(sizeof(struct isp_file));
	if (!file) {
		ret = -1;
		CMR_LOGE("alloc memory error.");
		return ret;
	}
	if (NULL == param) {
		ret = -1;
		CMR_LOGE("init param null error.");
		goto isp_free;
	}

	init_param = (struct isp_dev_init_param *)param;

	fd = open(isp_dev_name, O_RDWR, 0);
	if (fd < 0) {
		ret = -1;
		CMR_LOGE("isp_dev_open error.");
		goto isp_free;
	}

	file->fd = fd;
	file->camera_id = 0;
	*handle = (isp_handle)file;

	ret= ioctl(file->fd, ISP_IO_SET_INIT_PARAM, init_param);
	if (ret) {
		CMR_LOGE("isp initial param error.");
	}

	return ret;

isp_free:
	if (file)
		free(file);
	file = NULL;

	return ret;
}

cmr_int isp_dev_close(isp_handle handle)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;

	if (!handle) {
		CMR_LOGE("file hand is null error.");
		ret = -1;
		return ret;
	}

	file = (struct isp_file *)handle;

	if (-1 != file->fd) {
		if (-1 == close(file->fd)) {
			CMR_LOGE("close error.");
		}
	} else {
		CMR_LOGE("isp_dev_close error.");
	}

	free(file);

	return ret;
}

cmr_int isp_dev_stop(isp_handle handle)
{
	cmr_int ret = 0;
	cmr_int camera_id = 0;
	struct isp_file *file = NULL;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	camera_id = file->camera_id;

	ret = ioctl(file->fd, ISP_IO_STOP, &camera_id);
	if (ret) {
		CMR_LOGE("isp_dev_stop error.");
	}

	return ret;
}

cmr_int isp_dev_stream_on(isp_handle handle)
{
	cmr_int ret = 0;
	cmr_int camera_id = 0;
	struct isp_file *file = NULL;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	camera_id = file->camera_id;

	ret = ioctl(file->fd, ISP_IO_STREAM_ON, &camera_id);
	if (ret) {
		CMR_LOGE("isp_dev_stream_on error.");
	}

	return ret;
}

cmr_int isp_dev_stream_off(isp_handle handle)
{
	cmr_int ret = 0;
	cmr_int camera_id = 0;
	struct isp_file *file = NULL;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	camera_id = file->camera_id;

	ret = ioctl(file->fd, ISP_IO_STREAM_OFF, &camera_id);
	if (ret) {
		CMR_LOGE("isp_dev_stream_off error.");
	}

	return ret;
}

cmr_int isp_dev_load_firmware(isp_handle handle, struct isp_init_mem_param *param)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}
	if (!param) {
		CMR_LOGE("Param is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	ret = ioctl(file->fd, ISP_IO_LOAD_FW, param);
	if (ret) {
		CMR_LOGE("isp_dev_load_firmware error.");
	}

	return ret;
}

cmr_int isp_dev_set_statis_buf(isp_handle handle, struct isp_statis_buf *param)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}
	if (!param) {
		CMR_LOGE("Param is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_SET_STATIS_BUF, param);
	if (ret) {
		CMR_LOGE("isp_dev_set_statis_buf error. 0x%lx", ret);
	}

	return ret;
}

cmr_int isp_dev_get_statis_buf(isp_handle handle, struct isp_img_read_op *param)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;
	struct isp_img_read_op *op = NULL;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}
	if (!param) {
		CMR_LOGE("Param is null error.");
	}

	op = param;
	op->cmd = ISP_IMG_GET_STATISTICS_FRAME;

	file = (struct isp_file *)(handle);

	ret = read(file->fd, op, sizeof(struct isp_img_read_op));
	if (ret) {
		CMR_LOGE("isp_dev_get_statis_buf error.");
	}

	return ret;
}

cmr_int isp_dev_set_img_buf(isp_handle handle, struct isp_cfg_img_buf *param)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}
	if (!param) {
		CMR_LOGE("Param is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_SET_IMG_BUF, param);
	if (ret) {
		CMR_LOGE("isp_dev_cfg_img_buf error.");
	}

	return ret;
}

cmr_int isp_dev_get_img_buf(isp_handle handle, struct isp_img_read_op *param)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;
	struct isp_img_read_op *op = NULL;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}
	if (!param) {
		CMR_LOGE("Param is null error.");
		return -1;
	}

	op = param;
	op->cmd = ISP_IMG_GET_FRAME;

	file = (struct isp_file *)(handle);

	ret = read(file->fd, op, sizeof(struct isp_img_read_op));
	if (ret) {
		CMR_LOGE("isp_dev_get_img_buf error.");
	}

	return ret;
}

cmr_int isp_dev_set_img_param(isp_handle handle, struct isp_cfg_img_param *param)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}
	if (!param) {
		CMR_LOGE("Param is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_SET_IMG_PARAM, param);
	if (ret) {
		CMR_LOGE("isp_dev_set_img_param error.");
	}

	return ret;
}

cmr_int isp_dev_get_timestamp(isp_handle handle, cmr_u32 *sec, cmr_u32 *usec)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;
	struct sprd_isp_time time;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_GET_TIME, &time);
	if (ret) {
		CMR_LOGE("isp_dev_get_timestamp error.");
	}

	*sec = time.sec;
	*usec = time.usec;
	CMR_LOGI("sec=%d, usec=%d \n", *sec, *usec);

	return ret;
}

cmr_int isp_dev_cfg_scenario_info(isp_handle handle, SCENARIO_INFO_AP *data)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}
	if (!data) {
		CMR_LOGE("Param is null error.");
	}

	param.sub_id = ISP_CFG_SET_SCENARIO_INFO;
	param.property_param = data;

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);
	if (ret) {
		CMR_LOGE("isp_dev_cfg_scenario_info error.");
	}

	return ret;
}

cmr_int isp_dev_cfg_iso_speed(isp_handle handle, cmr_u32 *data)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}
	if (!data) {
		CMR_LOGE("Param is null error.");
	}

	param.sub_id = ISP_CFG_SET_ISO_SPEED;
	param.property_param = data;

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);
	if (ret) {
		CMR_LOGE("isp_dev_cfg_iso_speed error.");
	}

	return ret;
}

cmr_int isp_dev_cfg_awb_gain(isp_handle handle, struct isp_awb_gain_info *data)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}
	if (!data) {
		CMR_LOGE("Param is null error.");
	}

	param.sub_id = ISP_CFG_SET_AWB_GAIN;
	param.property_param = data;

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);
	if (ret) {
		CMR_LOGE("isp_dev_cfg_awb_gain error.");
	}

	return ret;
}

cmr_int isp_dev_cfg_awb_gain_balanced(isp_handle handle, struct isp_awb_gain_info *data)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}
	if (!data) {
		CMR_LOGE("Param is null error.");
	}

	param.sub_id = ISP_CFG_SET_AWB_GAIN_BALANCED;
	param.property_param = data;

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);
	if (ret) {
		CMR_LOGE("isp_dev_cfg_awb_gain error.");
	}

	return ret;
}

cmr_int isp_dev_cfg_dld_seq(isp_handle handle, DldSequence *data)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}
	if (!data) {
		CMR_LOGE("Param is null error.");
	}

	param.sub_id = ISP_CFG_SET_DLD_SEQUENCE;
	param.property_param = data;

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);
	if (ret) {
		CMR_LOGE("isp_dev_cfg_dld_seq error.");
	}

	return ret;
}

cmr_int isp_dev_cfg_3a_param(isp_handle handle, Cfg3A_Info *data)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}
	if (!data) {
		CMR_LOGE("Param is null error.");
	}

	param.sub_id = ISP_CFG_SET_3A_CFG;
	param.property_param = data;

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);
	if (ret) {
		CMR_LOGE("isp_dev_cfg_3a_param error.");
	}

	return ret;
}

cmr_int isp_dev_cfg_ae_param(isp_handle handle, AE_CfgInfo *data)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}
	if (!data) {
		CMR_LOGE("Param is null error.");
	}

	param.sub_id = ISP_CFG_SET_AE_CFG;
	param.property_param = data;

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);
	if (ret) {
		CMR_LOGE("isp_dev_cfg_ae_param error.");
	}

	return ret;
}

cmr_int isp_dev_cfg_af_param(isp_handle handle, AF_CfgInfo *data)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}
	if (!data) {
		CMR_LOGE("Param is null error.");
	}

	param.sub_id = ISP_CFG_SET_AF_CFG;
	param.property_param = data;

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);
	if (ret) {
		CMR_LOGE("isp_dev_cfg_af_param error.");
	}

	return ret;
}

cmr_int isp_dev_cfg_awb_param(isp_handle handle, AWB_CfgInfo *data)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}
	if (!data) {
		CMR_LOGE("Param is null error.");
	}

	param.sub_id = ISP_CFG_SET_AWB_CFG;
	param.property_param = data;

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);
	if (ret) {
		CMR_LOGE("isp_dev_cfg_awb_param error.");
	}

	return ret;
}

cmr_int isp_dev_cfg_yhis_param(isp_handle handle, YHis_CfgInfo *data)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}
	if (!data) {
		CMR_LOGE("Param is null error.");
	}

	param.sub_id = ISP_CFG_SET_YHIS_CFG;
	param.property_param = data;

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);
	if (ret) {
		CMR_LOGE("isp_dev_cfg_yhis_param error.");
	}

	return ret;
}

cmr_int isp_dev_cfg_sub_sample(isp_handle handle, SubSample_CfgInfo *data)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}
	if (!data) {
		CMR_LOGE("Param is null error.");
	}

	param.sub_id = ISP_CFG_SET_SUB_SAMP_CFG;
	param.property_param = data;

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);
	if (ret) {
		CMR_LOGE("isp_dev_cfg_sub_sample error.");
	}

	return ret;
}

cmr_int isp_dev_cfg_anti_flicker(isp_handle handle, AntiFlicker_CfgInfo *data)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}
	if (!data) {
		CMR_LOGE("Param is null error.");
	}

	param.sub_id = ISP_CFG_SET_AFL_CFG;
	param.property_param = data;

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);
	if (ret) {
		CMR_LOGE("isp_dev_cfg_anti_flicker error.");
	}

	return ret;
}

cmr_int isp_dev_cfg_dld_seq_basic_prev(isp_handle handle, cmr_u32 size, cmr_u8*data)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}
	if (!data) {
		CMR_LOGE("Param is null error.");
	}

	param.sub_id = ISP_CFG_SET_DLD_SEQ_BASIC_PREV;
	param.property_param = data;
	param.reserved = size;

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);
	if (ret) {
		CMR_LOGE("isp_dev_cfg_dld_seq_basic_prev error.");
	}

	return ret;
}

cmr_int isp_dev_cfg_dld_seq_adv_prev(isp_handle handle, cmr_u32 size, cmr_u8*data)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}
	if (!data) {
		CMR_LOGE("Param is null error.");
	}

	param.sub_id = ISP_CFG_SET_DLD_SEQ_ADV_PREV;
	param.property_param = data;
	param.reserved = size;

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);
	if (ret) {
		CMR_LOGE("isp_dev_cfg_dld_seq_adv_prev error.");
	}

	return ret;
}

cmr_int isp_dev_cfg_dld_seq_fast_converge(isp_handle handle, cmr_u32 size, cmr_u8*data)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}
	if (!data) {
		CMR_LOGE("Param is null error.");
	}

	param.sub_id = ISP_CFG_SET_DLD_SEQ_BASIC_FAST_CONV;
	param.property_param = data;
	param.reserved = size;

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);
	if (ret) {
		CMR_LOGE("isp_dev_cfg_dld_seq_fast_converge error.");
	}

	return ret;
}

cmr_int isp_dev_cfg_sharpness(isp_handle handle, cmr_u32 mode)
{
	cmr_int ret = 0;
	cmr_u32 temp_mode = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}

	temp_mode = mode;
	param.sub_id = ISP_CFG_SET_SHARPNESS;
	param.property_param = &temp_mode;

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);
	if (ret) {
		CMR_LOGE("isp_dev_cfg_sharpness error.");
	}

	return ret;
}

cmr_int isp_dev_cfg_saturation(isp_handle handle, cmr_u32 mode)
{
	cmr_int ret = 0;
	cmr_u32 temp_mode = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}

	temp_mode = mode;
	param.sub_id = ISP_CFG_SET_SATURATION;
	param.property_param = &temp_mode;

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);
	if (ret) {
		CMR_LOGE("isp_dev_cfg_saturation error.");
	}

	return ret;
}

cmr_int isp_dev_cfg_contrast(isp_handle handle, cmr_u32 mode)
{
	cmr_int ret = 0;
	cmr_u32 temp_mode = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}

	temp_mode = mode;
	param.sub_id = ISP_CFG_SET_CONTRAST;
	param.property_param = &temp_mode;

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);
	if (ret) {
		CMR_LOGE("isp_dev_cfg_contrast error.");
	}

	return ret;
}

cmr_int isp_dev_cfg_special_effect(isp_handle handle, cmr_u32 mode)
{
	cmr_int ret = 0;
	cmr_u32 temp_mode = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}

	temp_mode = mode;
	param.sub_id = ISP_CFG_SET_SPECIAL_EFFECT;
	param.property_param = &temp_mode;

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);
	if (ret) {
		CMR_LOGE("isp_dev_cfg_special_effect error.");
	}

	return ret;
}

cmr_int isp_dev_capability_fw_size(isp_handle handle, cmr_int *size)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;
	struct isp_capability param;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}
	if (!size) {
		CMR_LOGE("Param is null error.");
	}

	param.index = ISP_GET_FW_BUF_SIZE;
	param.property_param = (void *)size;

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_CAPABILITY, &param);
	if (ret) {
		CMR_LOGE("isp_dev_capability_fw_size error.");
	}

	return ret;
}

cmr_int isp_dev_capability_statis_buf_size(isp_handle handle, cmr_int *size)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;
	struct isp_capability param;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}
	if (!size) {
		CMR_LOGE("Param is null error.");
	}

	param.index = ISP_GET_STATIS_BUF_SIZE;
	param.property_param = (void *)size;

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_CAPABILITY, &param);
	if (ret) {
		CMR_LOGE("isp_dev_capability_statis_buf_size error.");
	}

	return ret;
}

cmr_int isp_dev_capability_dram_buf_size(isp_handle handle, cmr_int *size)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;
	struct isp_capability param;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}
	if (!size) {
		CMR_LOGE("Param is null error.");
	}

	param.index = ISP_GET_DRAM_BUF_SIZE;
	param.property_param = (void *)size;

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_CAPABILITY, &param);
	if (ret) {
		CMR_LOGE("isp_dev_capability_dram_buf_size error.");
	}

	return ret;
}

cmr_int isp_dev_capability_highiso_buf_size(isp_handle handle, cmr_int *size)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;
	struct isp_capability param;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}
	if (!size) {
		CMR_LOGE("Param is null error.");
	}

	param.index = ISP_GET_HIGH_ISO_BUF_SIZE;
	param.property_param = (void *)size;

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_CAPABILITY, &param);
	if (ret) {
		CMR_LOGE("isp_dev_capability_highiso_buf_size error.");
	}

	return ret;
}

cmr_int isp_dev_capability_video_size(isp_handle handle, struct isp_img_size *size)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;
	struct isp_capability param;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}
	if (!size) {
		CMR_LOGE("Param is null error.");
	}

	param.index = ISP_GET_CONTINUE_SIZE;
	param.property_param = (void *)size;

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_CAPABILITY, &param);
	if (ret) {
		CMR_LOGE("error");
	}

	return ret;
}

cmr_int isp_dev_capability_single_size(isp_handle handle, struct isp_img_size *size)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;
	struct isp_capability param;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}
	if (!size) {
		CMR_LOGE("Param is null error.");
	}

	param.index = ISP_GET_SINGLE_SIZE;
	param.property_param = (void *)size;

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_CAPABILITY, &param);
	if (ret) {
		CMR_LOGE("error");
	}

	return ret;
}
#if 0
cmr_int isp_dev_get_irq(isp_handle handle, cmr_int *evt_ptr)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;
	struct isp_irq *ptr = (struct isp_irq *)evt_ptr;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);

	while (1) {
		ret = ioctl(file->fd, ISP_IO_IRQ, ptr);
		if (0 == ret) {
			break;
		} else {
			if (-EINTR == ptr->ret_val) {
				cmr_usleep(5000);
				CMR_LOGE("continue.");
				continue;
			}
			CMR_LOGE("ret_val=%d", ptr->ret_val);
			break;
		}
	}

	return ret;
}
#endif

cmr_int isp_dev_set_dcam_id(isp_handle handle, cmr_u32 dcam_id)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}
	CMR_LOGI("dcam_id %d", dcam_id);
	file = (struct isp_file *)(handle);
	ret = ioctl(file->fd, ISP_IO_SET_DCAM_ID, &dcam_id);
	if (ret) {
		CMR_LOGE("isp_dev_stream_on error.");
	}

	return ret;
}

cmr_int isp_dev_get_iq_param(isp_handle handle, struct debug_info1 *info1, struct debug_info2 *info2)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;
	struct altek_iq_info param;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_GET_IQ_PARAM, &param);
	if (ret) {
		CMR_LOGE("ioctl error.");
	}

	if (info1) {
		memcpy((void *)&info1->raw_debug_info1, &param.iq_info_1.tRawInfo, sizeof(struct raw_img_debug));
		memcpy((void *)&info1->shading_debug_info1, &param.iq_info_1.tShadingInfo, sizeof(struct shading_debug));
		memcpy((void *)&info1->irp_tuning_para_debug_info1, &param.iq_info_1.tIrpInfo, sizeof(struct irp_tuning_debug));
		memcpy((void *)&info1->sw_debug1, &param.iq_info_1.tSWInfo, sizeof(struct isp_sw_debug));
	}
	if (info2) {
		memcpy((void *)&info2->irp_tuning_para_debug_info2, &param.iq_info_2.tGammaTone, sizeof(struct irp_gamma_tone));
	}

	CMR_LOGI("%d %d %d %d %d %d\n",
		param.iq_info_1.tRawInfo.uwWidth,
		param.iq_info_1.tRawInfo.uwHeight,
		param.iq_info_1.tRawInfo.ucFormat,
		param.iq_info_1.tRawInfo.ucBitNumber,
		param.iq_info_1.tRawInfo.ucMirrorFlip,
		param.iq_info_1.tRawInfo.nColorOrder);

	return ret;
}