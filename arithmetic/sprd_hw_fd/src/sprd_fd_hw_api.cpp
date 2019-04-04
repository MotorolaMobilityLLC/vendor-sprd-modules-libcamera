#define LOG_TAG "HW_FD"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <log/log.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include "MemIon.h"
#include <sprd_ion.h>
#include <sys/mman.h>


#include "sprd_fd_hw_api.h"
#include "sprd_fd.h"


#define FD_IP_STATUS_MASK 0x00010000
#define FD_IP_ERR_FLG_MASK 0x00000001
#define FD_IP_ERR_CODE_MASK 0x000000FE



enum FD_IP_ERR_FLG
{
	FD_IP_ERR_FLG_CORRECT = 0,
	FD_IP_ERR_FLG_ERROR = 1,
};
enum FD_IP_ERR_CODE
{
	FD_IP_ERR_SRC_OUT_RANGE = 1,
	FD_IP_ERR_CROP_OUT_RANGE = 2,
	FD_IP_ERR_CROP_SIZE_ERROR = 3,
};
struct fd_hw_handle {
    fd_s32 fd;
	fd_s32 ip_status;
	fd_s32 ip_err_flg;
	fd_s32 ip_err_code;
};
typedef struct sprd_camera_fd_memory {
    android::MemIon *ion_heap;
    fd_uint phys_addr;
    fd_uint phys_size;
    fd_s32 fd;
    fd_s32 dev_fd;
    void *handle;
    void *data;
    bool busy_flag;
} sprd_camera_fd_memory_t;

sprd_camera_fd_memory_t* dim;
sprd_camera_fd_memory_t* out;
int g_iommu_status = SPRD_FD_IOMMU_ENABLED;/*Default is enable*/



sprd_camera_fd_memory_t *AllocFDMem(int buf_size,int num_bufs,uint32_t is_cache)
{
    size_t mem_size = 0;
    android::MemIon *pHeapIon = NULL;

    sprd_camera_fd_memory_t *memory = (sprd_camera_fd_memory_t *)malloc(sizeof(sprd_camera_fd_memory_t));
    if (NULL == memory) {
        ALOGE("fatal error! memory pointer is null.\n");
        goto getpmem_fail;
    }
    memset(memory, 0, sizeof(sprd_camera_fd_memory_t));
    memory->busy_flag = false;

    mem_size = buf_size * num_bufs;
    // to make it page size aligned
    mem_size = (mem_size + 4095U) & (~4095U);
    if (mem_size == 0) {
        goto getpmem_fail;
    }
    if (SPRD_FD_IOMMU_ENABLED == SPRD_FD_IOMMU_DISABLED) {
        if (is_cache) {
            pHeapIon = new android::MemIon("/dev/ion", mem_size, 0,
                                  (1 << 31) | ION_HEAP_ID_MASK_MM);
        } else {
            pHeapIon = new android::MemIon("/dev/ion", mem_size, android::MemIon::NO_CACHING,
                                  ION_HEAP_ID_MASK_MM);
        }
    } else {
        if (is_cache) {
            pHeapIon = new android::MemIon("/dev/ion", mem_size, 0,
                                  (1 << 31) | ION_HEAP_ID_MASK_SYSTEM);
        } else {
            pHeapIon = new android::MemIon("/dev/ion", mem_size, android::MemIon::NO_CACHING,
                                  ION_HEAP_ID_MASK_SYSTEM);
        }
    }

    if (pHeapIon == NULL || pHeapIon->getHeapID() < 0) {
        ALOGE("pHeapIon is null or getHeapID failed\n");
        goto getpmem_fail;
    }

    if (NULL == pHeapIon->getBase() || MAP_FAILED == pHeapIon->getBase()) {
        ALOGE("error getBase is null.\n");
        goto getpmem_fail;
    }

    memory->ion_heap = pHeapIon;
    memory->fd = pHeapIon->getHeapID();
    memory->dev_fd = pHeapIon->getIonDeviceFd();
    // memory->phys_addr is offset from memory->fd, always set 0 for yaddr
    memory->phys_addr = 0;
    memory->phys_size = mem_size;
    memory->data = pHeapIon->getBase();

    //ALOGD("fd=0x%x, phys_addr=0x%lx, virt_addr=%p, size=0x%lx, heap=%p\n",
    //         memory->fd, memory->phys_addr, memory->data, memory->phys_size,pHeapIon);

    return memory;

getpmem_fail:
    if (memory != NULL) {
        free(memory);
        memory = NULL;
    }
    return NULL;
}
void FreeFDMem(sprd_camera_fd_memory_t *memory)
{
    if (memory)
	{
        if (memory->ion_heap)
		{
            //ALOGD("fd=0x%x, phys_addr=0x%lx, virt_addr=%p, size=0x%lx, heap=%p\n",
            //    memory->fd, memory->phys_addr, memory->data, memory->phys_size,memory->ion_heap);
            delete memory->ion_heap;
            memory->ion_heap = NULL;
        } else {
            ALOGD("memory->ion_heap is null:fd=0x%x, phys_addr=0x%lx, "
                     "virt_addr=%p, size=0x%lx,heap=%p \n",
                     memory->fd, memory->phys_addr, memory->data,
                     memory->phys_size,memory->ion_heap);
        }
        free(memory);
        memory = NULL;
    }
}
sprd_camera_fd_memory_t * CopyToIon(void *data,fd_u32 size)
{
	sprd_camera_fd_memory_t *memory = NULL;

	memory = AllocFDMem(size, 1, false);
	if (NULL == memory)
	{
	    ALOGE("error memory is null.");
	    goto mem_fail;
    }

	memcpy(memory->data,data,size);
	return memory;

mem_fail:
	FreeFDMem(memory);
	return NULL;
}

void get_fd_iommu_status(int fd)
{
	int ret = 0;
	unsigned int iommu_status;

	ret = ioctl(fd,SPRD_FD_IO_GET_IOMMU_STATUS,&iommu_status);
	if(ret < 0)
	{
		ALOGE("Get iommu status fail.");
		g_iommu_status = SPRD_FD_IOMMU_ENABLED;
	}
	else
	{
		ALOGI("Get iommu status %d",iommu_status);
		g_iommu_status = iommu_status;
	}
	return;
}
int  hwfd_open(HWFD_DETECTOR_HANDLE *hDT)
{
	int ret = HWFD_OK;
	struct fd_hw_handle *file = NULL;
	fd_s32 fd = -1;
	char dev[32] ={0};
	char fd_set_freq[128];

	ALOGI("HW Face Detect API V0.91");

	file = (fd_hw_handle*)malloc(sizeof(struct fd_hw_handle));
	memset(file,0,sizeof(struct fd_hw_handle));
	if (!file) {
	  ret = HWFD_ERROR;
	}
	/*set dvfs*/
	fd_int work_clock = SPRD_FD_DVFS_INDEX2;
	sprintf(fd_set_freq, "/sys/class/devfreq/fd-dvfs/fd_governor/set_work_index");
	FILE *fp = fopen(fd_set_freq, "wb");
	if (fp == NULL) {
		ALOGE("fail to open file for fd dvfs: %d ", errno);
	}
	else
	{
		fprintf(fp, "%ld", work_clock);
		fclose(fp);
		fp = NULL;
	}

	sprintf(dev , "/dev/%s" ,SPRD_FD_DEVICE_NAME);

	fd = open(dev, O_RDWR, 0);
	if (fd > 0)
	{
		file->fd = fd;
		*hDT = (HWFD_DETECTOR_HANDLE)file;
		struct sprd_fd_cfg_param cmd_para;
		/*Get FD IP version*/
		cmd_para.reg_param = SPRD_FD_REG_PARAM_IP_REV;
		cmd_para.reg_val = 0x0;
		ret = ioctl(fd,SPRD_FD_IO_READ,&cmd_para);
		if(ret < 0)
		{
			ALOGE("Get FD IP version fail.");
		}
		else
		{
			ALOGI("Get FD IP version %02X ",cmd_para.reg_val);
		}
		get_fd_iommu_status(fd);
		dim = AllocFDMem(DIM_BUFFER_SIZE, 1, false);
		out = AllocFDMem(OUT_BUFFER_SIZE, 1, false);

		if((NULL == dim) || (NULL == out))
			goto open_fail;

		return HWFD_OK;
	}
open_fail:
	{
	  ALOGE("Fail to open FD device %s.",dev);
	  if (fd >= 0) {
		  close(fd);
		  fd = -1;
	  }
	  if (file)
		  free(file);
	  file = NULL;
	}
	return HWFD_ERROR;
}


void hwfd_close(HWFD_DETECTOR_HANDLE *hDT)
{
    struct fd_hw_handle *file = (struct fd_hw_handle *)(*hDT);

    ALOGI("close FD device.");
	FreeFDMem(out);
	FreeFDMem(dim);

	if(file)
	{
		if (-1 == file->fd)
		{
		   ALOGE("Invalid fd");
		}
		else
		{
			close(file->fd);
		}
		free(file);
		*hDT = NULL;
	}
    return ;

}
int32_t  hwfd_is_busy(HWFD_DETECTOR_HANDLE hDT)
{
	struct sprd_fd_cfg_param cmd_para;
    cmd_para.reg_param = SPRD_FD_REG_PARAM_INFO;
	cmd_para.reg_val = 0;
    fd_int fd = -1;
	int ret; 
	struct fd_hw_handle *file = NULL;

	file = (struct fd_hw_handle *)hDT;
	if (!file) 
	{
 	   ALOGE(" GetFDInfo Invalid Param fd_hw_handle !");
 	   return HWFD_ERROR;
	}
	else
	{
		fd = file->fd;
		if(fd < 0)
		{
			ALOGE("GetFDInfo Invalid Param fd !");
			return HWFD_ERROR;
		}
		ret = ioctl(fd, SPRD_FD_IO_READ, &cmd_para);
		if(ret < 0)
		{
			ALOGE("GetFDInfo Fd ioctl fail !");
			return HWFD_ERROR;
		}
		else
		{
			ALOGI("Get FD info is 0x%02X .",cmd_para.reg_val);
		}
		if(FD_IP_STATUS_MASK == (FD_IP_STATUS_MASK & cmd_para.reg_val))
		{
			return FD_IP_BUSY;
		}
		else
		{
			return FD_IP_IDLE;
		}
	}
}

#ifndef FD_DEBUG
int  hwfd_start_fd(HWFD_DETECTOR_HANDLE hDT,void* i_request,void* o_response)
{
	struct sprd_fd_cfg_param cmd_para;

    fd_s32 fd = -1;
	int ret = HWFD_OK;
	struct fd_hw_handle *file = NULL;

	HWFD_REQUEST* request = (HWFD_REQUEST*)i_request;
	HWFD_RESPONSE* response = (HWFD_RESPONSE*)o_response;


	file = (struct fd_hw_handle *)hDT;
    if (!file)
	{
 	   ALOGE("Invalid Param fd_hw_handle !");
 	   ret = HWFD_ERROR;
    }
	else
	{
		fd = file->fd;
		if(fd < 0)
		{
			ALOGE("Invalid Param fd %d !",fd);
			return HWFD_ERROR;
		}

		/*Set mode cfg*/
		cmd_para.reg_param = SPRD_FD_REG_PARAM_MOD_CFG;
		cmd_para.reg_val = 0x00110000;  //default value
		ret = ioctl(fd,SPRD_FD_IO_WRITE,&cmd_para);
		if(ret < 0)
		{
			ALOGE("SPRD_FD_REG_PARAM_MOD_CFG ret %d",ret);
			return HWFD_ERROR;
		}

		/*Set cmd num*/
		cmd_para.reg_param = SPRD_FD_REG_PARAM_CMD_NUM;
		cmd_para.reg_val = request->fd_cmd_num;
		ret = ioctl(fd,SPRD_FD_IO_WRITE,&cmd_para);
		if(ret < 0)
		{
			ALOGE("SPRD_FD_REG_PARAM_CMD_NUM ret %d",ret);
			return HWFD_ERROR;
		}

		/*Input image*/
		cmd_para.reg_param = SPRD_FD_REG_PARAM_IMAGE_BADDR;
		cmd_para.reg_val = request->fd_image_addr;
		ret = ioctl(fd,SPRD_FD_IO_WRITE,&cmd_para);
		if(ret < 0)
		{
			ALOGE("SPRD_FD_REG_PARAM_IMAGE_BADDR ret %d",ret);
			return HWFD_ERROR;
		}

		/*Input image line step*/
		cmd_para.reg_param = SPRD_FD_REG_PARAM_IMAGE_LINENUM;
		cmd_para.reg_val = request->fd_image_linestep;
		ret = ioctl(fd,SPRD_FD_IO_WRITE,&cmd_para);
		if(ret < 0)
		{
			ALOGE("SPRD_FD_REG_PARAM_IMAGE_LINENUM ret %d",ret);
			return HWFD_ERROR;
		}

		/*Face search num*/
		cmd_para.reg_param = SPRD_FD_REG_PARAM_FACE_NUM;
		cmd_para.reg_val = 0x00001FFF;
		ret = ioctl(fd,SPRD_FD_IO_WRITE,&cmd_para);
		if(ret < 0)
		{
			ALOGE("SPRD_FD_REG_PARAM_FACE_NUM ret %d",ret);
			return HWFD_ERROR;
		}

		/*Set out addr*/
		cmd_para.reg_param = SPRD_FD_REG_PARAM_OUT_BUF_ADDR;
		cmd_para.reg_val = out->fd;
		ret = ioctl(fd,SPRD_FD_IO_WRITE,&cmd_para);
		if(ret < 0)
		{
			ALOGE("SPRD_FD_REG_PARAM_OUT_BUF_ADDR ret %d",ret);
			return HWFD_ERROR;
		}

		/*Set dim addr*/
		cmd_para.reg_param = SPRD_FD_REG_PARAM_DIM_BUF_ADDR;
		cmd_para.reg_val = dim->fd;
		ret = ioctl(fd,SPRD_FD_IO_WRITE,&cmd_para);
		if(ret < 0)
		{
			ALOGE("SPRD_FD_REG_PARAM_DIM_BUF_ADDR ret %d",ret);
			return HWFD_ERROR;
		}

		/*Set cmd queue addr*/
		cmd_para.reg_param = SPRD_FD_REG_PARAM_CMD_BADDR;
		sprd_camera_fd_memory_t* cmd_queue = CopyToIon(request->fd_cmd_addr,request->fd_cmd_size);
		if(NULL == cmd_queue)
		{
			return HWFD_ERROR;
		}
		cmd_para.reg_val = cmd_queue->fd;
		ret = ioctl(fd,SPRD_FD_IO_WRITE,&cmd_para);
		if(ret < 0)
		{
			ALOGE("SPRD_FD_REG_PARAM_CMD_BADDR ret %d",ret);
			FreeFDMem(cmd_queue);
			return HWFD_ERROR;
		}

		/*Start run */
		cmd_para.reg_param = SPRD_FD_REG_PARAM_RUN;
		cmd_para.reg_val = 0x01;
		ret = ioctl(fd, SPRD_FD_IO_WRITE, &cmd_para);
		if(ret < 0)
		{
			ALOGE("SPRD_FD_REG_PARAM_RUN ret %d",ret);
			FreeFDMem(cmd_queue);
			return HWFD_ERROR;
		}

		/*Get face num*/
		cmd_para.reg_param = SPRD_FD_REG_PARAM_FACE_NUM;
		cmd_para.reg_val = 0;
		ret = ioctl(fd, SPRD_FD_IO_READ, &cmd_para);
		if(ret < 0)
		{
			ALOGE("SPRD_FD_REG_PARAM_FACE_NUM ret %d",ret >> 16);
			FreeFDMem(cmd_queue);
			return HWFD_ERROR;
		}
#ifdef LOG_DEBUG
		ALOGI("Get face num %02X",cmd_para.reg_val);
#endif
		response->face_count = cmd_para.reg_val >> 16;
		response->data = out->data;
		FreeFDMem(cmd_queue);
	}
	return ret;
}
#else/*for debug*/
int  hwfd_start_fd(HWFD_DETECTOR_HANDLE hDT,void* i_request,void* o_response)
{
	struct sprd_fd_cfg_param cmd_para;
	
    fd_s32 fd = -1;
	int ret = HWFD_OK;
	struct fd_hw_handle *file = NULL;

	HWFD_REQUEST* request = (HWFD_REQUEST*)i_request;
	HWFD_RESPONSE* response = (HWFD_RESPONSE*)o_response;
	ALOGI("%d, %d",request->fd_image_addr_debug,request->fd_image_size_debug);

	file = (struct fd_hw_handle *)hDT;
    if (!file) 
	{
 	   ALOGE("Invalid Param fd_hw_handle !");
 	   ret = HWFD_ERROR;
    }
	else
	{
		fd = file->fd;
		if(fd < 0)
		{
			ALOGE("Invalid Param fd %d !",fd);
			return HWFD_ERROR;
		}
		
		/*Set idle clock*/
		fd_int idle_clock = 0; 
		ret = ioctl(fd,SPRD_FD_IO_IDLE_CLOCK_SEL,&idle_clock);
		if(ret < 0)
		{
			ALOGI("SPRD_FD_IO_IDLE_CLOCK_SEL ret %d",ret);
			return HWFD_ERROR;
		}
		
		/*Set mode cfg*/
		cmd_para.reg_param = SPRD_FD_REG_PARAM_MOD_CFG;
		cmd_para.reg_val = 0x00110000;  //default value
		ret = ioctl(fd,SPRD_FD_IO_WRITE,&cmd_para);
		if(ret < 0)
		{
			ALOGI("SPRD_FD_REG_PARAM_MOD_CFG ret %d",ret);
			return HWFD_ERROR;
		}

		/*Set cmd queue addr*/
		cmd_para.reg_param = SPRD_FD_REG_PARAM_CMD_BADDR;
		sprd_camera_fd_memory_t* cmd_queue = CopyToIon(request->fd_cmd_addr,request->fd_cmd_size);
		if(NULL == cmd_queue)
			return HWFD_ERROR;

		cmd_para.reg_val = cmd_queue->fd;
		ret = ioctl(fd,SPRD_FD_IO_WRITE,&cmd_para);
		if(ret < 0)
		{
			ALOGI("SPRD_FD_REG_PARAM_CMD_BADDR ret %d",ret);
			FreeFDMem(cmd_queue);
			return HWFD_ERROR;
		}

		/*Set cmd num*/
		cmd_para.reg_param = SPRD_FD_REG_PARAM_CMD_NUM;
		cmd_para.reg_val = request->fd_cmd_num;
		ret = ioctl(fd,SPRD_FD_IO_WRITE,&cmd_para);
		if(ret < 0)
		{
			ALOGI("SPRD_FD_REG_PARAM_CMD_NUM ret %d",ret);
			FreeFDMem(cmd_queue);
			return HWFD_ERROR;
		}

		/*Input image*/
		cmd_para.reg_param = SPRD_FD_REG_PARAM_IMAGE_BADDR;
		sprd_camera_fd_memory_t* y = CopyToIon(request->fd_image_addr_debug,request->fd_image_size_debug);
		if(NULL == y)
		{
			FreeFDMem(cmd_queue);
			return HWFD_ERROR;
		}
		cmd_para.reg_val = y->fd;
		ret = ioctl(fd,SPRD_FD_IO_WRITE,&cmd_para);
		if(ret < 0)
		{
			ALOGI("SPRD_FD_REG_PARAM_IMAGE_BADDR ret %d",ret);
			FreeFDMem(cmd_queue);
			FreeFDMem(y);
			return HWFD_ERROR;
		}

		/*Input image line step*/
		cmd_para.reg_param = SPRD_FD_REG_PARAM_IMAGE_LINENUM;
		cmd_para.reg_val = request->fd_image_linestep;
		ret = ioctl(fd,SPRD_FD_IO_WRITE,&cmd_para);
		if(ret < 0)
		{
			ALOGI("SPRD_FD_REG_PARAM_IMAGE_LINENUM ret %d",ret);
			FreeFDMem(cmd_queue);
			FreeFDMem(y);
			return HWFD_ERROR;
		}


		/*Face search num*/
		cmd_para.reg_param = SPRD_FD_REG_PARAM_FACE_NUM;
		cmd_para.reg_val = 0x00001FFF;
		ret = ioctl(fd,SPRD_FD_IO_WRITE,&cmd_para);
		if(ret < 0)
		{
			ALOGI("SPRD_FD_REG_PARAM_FACE_NUM ret %d",ret);
			FreeFDMem(cmd_queue);
			FreeFDMem(y);
			return HWFD_ERROR;
		}

		/*Set out addr*/
		cmd_para.reg_param = SPRD_FD_REG_PARAM_OUT_BUF_ADDR;
		cmd_para.reg_val = out->fd;
		ALOGI("out fd is %d",out->fd);
		ret = ioctl(fd,SPRD_FD_IO_WRITE,&cmd_para);
		if(ret < 0)
		{
			ALOGI("SPRD_FD_REG_PARAM_OUT_BUF_ADDR ret %d",ret);
			FreeFDMem(cmd_queue);
			FreeFDMem(y);
			return HWFD_ERROR;
		}

		/*Set dim addr*/
		cmd_para.reg_param = SPRD_FD_REG_PARAM_DIM_BUF_ADDR;
		cmd_para.reg_val = dim->fd;
		ret = ioctl(fd,SPRD_FD_IO_WRITE,&cmd_para);
		if(ret < 0)
		{
			ALOGI("SPRD_FD_REG_PARAM_DIM_BUF_ADDR ret %d",ret);
			FreeFDMem(cmd_queue);
			FreeFDMem(y);
			return HWFD_ERROR;
		}

		/*Set work clock*/
		fd_int work_clock = SPRD_FD_DVFS_INDEX7;
		ret = ioctl(fd,SPRD_FD_IO_WORK_CLOCK_SEL,&work_clock);
		if(ret < 0)
		{
			ALOGI("SPRD_FD_DVFS_INDEX7 ret %d",ret);
			FreeFDMem(cmd_queue);
			FreeFDMem(y);
			return HWFD_ERROR;
		}

		/*Start run */
		cmd_para.reg_param = SPRD_FD_REG_PARAM_RUN;
		cmd_para.reg_val = 0x01;
		ret = ioctl(fd, SPRD_FD_IO_WRITE, &cmd_para);
		if(ret < 0)
		{
			ALOGI("SPRD_FD_REG_PARAM_RUN ret %d",ret);
			FreeFDMem(cmd_queue);
			FreeFDMem(y);
			return HWFD_ERROR;
		}

		/*Get face num*/
		cmd_para.reg_param = SPRD_FD_REG_PARAM_FACE_NUM;
		cmd_para.reg_val = 0;
		ret = ioctl(fd, SPRD_FD_IO_READ, &cmd_para);
		if(ret < 0)
		{
			ALOGI("SPRD_FD_REG_PARAM_FACE_NUM ret %d",ret);
			FreeFDMem(cmd_queue);
			FreeFDMem(y);
			return HWFD_ERROR;
		}
		ALOGI("Get face num %02X",cmd_para.reg_val);
		response->face_count = cmd_para.reg_val >> 16;
		response->data = out->data;
		FreeFDMem(cmd_queue);
		FreeFDMem(y);
	}
	return ret;
}
#endif









