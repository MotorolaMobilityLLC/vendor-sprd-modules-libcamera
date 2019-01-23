#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <log/log.h>
#include "sprd_camalg_assist_interface.h"
#include "sprd_camalg_assist_log.h"
#include "sprd_ion.h"
#include "MemIon.h"
#include "sprd_vdsp_cmd.h"
#include <cutils/properties.h>
#include <linux/ioctl.h>

#define LOG_TAG "sprd_camalg_assist"

#define VDSP_IO_MAGIC               'R'
#define SPRD_VDSP_IO_VERSION     	    _IO(VDSP_IO_MAGIC, 1)
#define SPRD_VDSP_IO_LOAD_FW             _IO(VDSP_IO_MAGIC,2)
#define SPRD_VDSP_IO_SET_MSG \
	_IOW(VDSP_IO_MAGIC, 3, struct sprd_dsp_cmd)
#define SPRD_VDSP_IO_FAST_STOP \
	_IOW(VDSP_IO_MAGIC, 4, struct sprd_dsp_cmd)

#define VDSP_DRIVER_PATH "/dev/sprd_vdsp"

#define VDSP_CMD_SAVE_FILE_PATH    "/data/misc/cameraserver/"
using namespace android;
#if 0
static int save_vdsp_cmd_tofile(void* pcmd)
{
	struct sprd_dsp_cmd* cmd = (struct sprd_dsp_cmd*)pcmd;
	static int index = 0;
	char filename[256];
	FILE *fp;
	int i;
	sprintf(filename , "/data/misc/cameraserver/vdspcmd_parameter_index%d.txt" , index);
	fp = fopen(filename , "wt");
	if(fp)
	{
		fprintf(fp , "flag:%d,in_data_size:%d,out_data_size:%d,buffer_size:%d, priority:%d , nsid:%s\n" , cmd->flag,
				cmd->in_data_size , cmd->out_data_size , cmd->buffer_size , cmd->priority , cmd->nsid);
		fclose(fp);
	}
	else
		VDSP_CMD_LOGE("open filename:%s failed" , filename);
	sprintf(filename , "/data/misc/cameraserver/vdspcmd_paramdata_inddata_index%d" , index);
	fp = fopen(filename , "wb");
        if(fp)
        {
			fwrite(cmd->in_data_addr , cmd->in_data_size , 1 , fp);
                fclose(fp);
        }
	else
		VDSP_CMD_LOGE("open filename:%s failed" , filename);
	sprintf(filename , "/data/misc/cameraserver/vdspcmd_paramdata_outddata_index%d" , index);
        fp = fopen(filename , "wb");
        if(fp)
        {
                fwrite(cmd->out_data_addr , cmd->out_data_size , 1 , fp);
                fclose(fp);
        }
	else
		VDSP_CMD_LOGE("open filename:%s failed" , filename);
	for(i = 0; i < (cmd->buffer_size/sizeof(struct sprd_dsp_buffer)); i ++)
	{
		sprintf(filename , "/data/misc/cameraserver/vdspcmd_paramdata_buffer%d_index%d" , i , index);
        	fp = fopen(filename , "wb");
        	if(fp)
        	{
			struct sprd_dsp_buffer* dsp_buffer = (struct sprd_dsp_buffer*)cmd->buffer_addr;
                	if(NULL != dsp_buffer)
                        	fwrite(dsp_buffer[i].addr , dsp_buffer[i].size , 1 , fp);
                	else
                        	fwrite(cmd->buffer_data[i].addr , cmd->buffer_data[i].size , 1 , fp);
                	fclose(fp);
        	}
		else
			VDSP_CMD_LOGE("open filename:%s failed" , filename);
	}
	index++;
	VDSP_CMD_LOGE("save file end");
	return 0;
}
#endif
__attribute__ ((visibility("default"))) void* sprd_vdsp_alloc_cmd(const char* appid , int32_t inputfd , uint32_t inputsize , int32_t outputfd, uint32_t outputsize , uint32_t buffernum , uint32_t priority)
{
	void* pcmd = calloc(1 , sizeof(struct sprd_dsp_cmd));
	if(pcmd != NULL)
	{
		struct sprd_dsp_cmd* cmd = (struct sprd_dsp_cmd*)pcmd;
		if(strlen(appid) < SPRD_DSP_CMD_NAMESPACE_ID_SIZE)
        	{
                	strcpy(cmd->nsid , appid);
        	}
        	else
		{
			VDSP_CMD_LOGE("appid is too long");
			goto _ERR_EXIT;
		}
		cmd->in_data_fd = inputfd;
                cmd->out_data_fd = outputfd;
		if(buffernum > SPRD_DSP_CMD_INLINE_BUFFER_COUNT)
        	{
                	if(cmd->buffer_addr == 0)
                	{
				void* buffer;
				unsigned long temp;
				buffer = malloc(buffernum * sizeof(struct sprd_dsp_buffer));
				temp = (unsigned long) buffer;
				cmd->buffer_addr = (uint32_t)temp;
                        	if(cmd->buffer_addr == 0)
				{
					VDSP_CMD_LOGE("buffer_addr malloc failed");
					goto _ERR_EXIT;
				}
                	}
        	}
	        cmd->buffer_size = buffernum*sizeof(struct sprd_dsp_buffer);
		cmd->in_data_size = inputsize;
		cmd->out_data_size = outputsize;
		cmd->priority = priority;
	}
	VDSP_CMD_LOGI("input size:%d , outputsize:%d , buffer num:%d" , inputsize , outputsize , buffernum);
	return pcmd;
_ERR_EXIT:
	free(pcmd);
	return NULL;
}
__attribute__ ((visibility("default"))) int sprd_vdsp_free_cmd(void* pcmd)
{
	struct sprd_dsp_cmd* cmd = (struct sprd_dsp_cmd*)pcmd;
	unsigned long temp = (unsigned long)(cmd->buffer_addr);
	void* pcmd_buffer = (void*)temp;
	if(pcmd_buffer != NULL)
		free(pcmd_buffer);
	free(pcmd);
	return SPRD_VDSP_CMD_SUCCESS;
}
__attribute__ ((visibility("default"))) int sprd_vdsp_cmd_addbuffer(void* pcmd , int32_t buffer_fd  , uint32_t size , uint32_t index)
{
	struct sprd_dsp_buffer* dsp_buffer;
        struct sprd_dsp_cmd* cmd = (struct sprd_dsp_cmd*)pcmd;
	VDSP_CMD_LOGI("add buffer buffer_fd:%d , size:%d , index:%d" , buffer_fd , size , index);
	if(1)//cmd->buffer_addr == 0)
	{
		cmd->buffer_data[index].size = size;
		cmd->buffer_data[index].fd = buffer_fd;
	}
	else
	{
		unsigned long temp = (unsigned long)(cmd->buffer_addr);
		dsp_buffer = (struct sprd_dsp_buffer*)temp;
		dsp_buffer[index].size = size;
		dsp_buffer[index].fd = buffer_fd;
	}
        return SPRD_VDSP_CMD_SUCCESS;
}

__attribute__ ((visibility("default"))) int sprd_vdsp_open_drv()
{
	int fd = open(VDSP_DRIVER_PATH , O_RDWR | O_CLOEXEC , 0);
	if(fd < 0)
	{
		VDSP_CMD_LOGE("open fd:%s failed" , VDSP_DRIVER_PATH);
	}
	return fd;
}

__attribute__ ((visibility("default"))) int sprd_vdsp_close_drv(int fd)
{
	if(fd < 0)
	{
		VDSP_CMD_LOGE("input fd:%d is error" , fd);
		return -1;
	}
	close(fd);
	return 0;
}

__attribute__ ((visibility("default"))) int sprd_vdsp_send_cmd(int fd , void* pcmd)
{
	int ret = 0;
	struct sprd_dsp_cmd* cmd = (struct sprd_dsp_cmd*)pcmd;
	//char value[PROPERTY_VALUE_MAX];
	VDSP_CMD_LOGD("send cmd flag:%d, indata_size:%d, outdatasize:%d, buffer_size:%d , in_data_fd:%d ,\
			out_data_fd:%d , buffer_addr:%x ,priority:%d , nsid:%s" , cmd->flag , cmd->in_data_size , cmd->out_data_size, cmd->buffer_size,
			cmd->in_data_fd , cmd->out_data_fd , cmd->buffer_addr , cmd->priority , cmd->nsid);
	//property_get("vdsp_debug_cmd_enable" , value , "0");
#if 0
	if(!strcmp(value , "1"))
		save_vdsp_cmd_tofile(pcmd);
#endif
#if 1
	if(cmd->priority == 0)
	{
		VDSP_CMD_LOGI("yzl add SPRD_VDSP_IO_SET_MSG:%x ,VDSP_IO_MAGIC:%x , sizeof struct sprd_dsp_cmd:%d" , SPRD_VDSP_IO_SET_MSG , 
					VDSP_IO_MAGIC , sizeof(struct sprd_dsp_cmd));
		ret = ioctl(fd, SPRD_VDSP_IO_SET_MSG , cmd);
		if (ret) {
			VDSP_CMD_LOGE("Fail to send SPRD_VDSP_IO_START_ALGORITHM.\n");
			return -1;
		}
	}
	else if(cmd->priority == 1)
	{
		VDSP_CMD_LOGI("yzl add SPRD_VDSP_IO_FAST_STOP:%x" , SPRD_VDSP_IO_FAST_STOP);
		//send stop cmd
		ret = ioctl(fd, SPRD_VDSP_IO_FAST_STOP , cmd);
                if (ret) {
                        VDSP_CMD_LOGE("Fail to send SPRD_VDSP_IO_START_ALGORITHM.\n");
                        return -1;
                }
	}
#endif
	return 0;
}
__attribute__ ((visibility("default"))) void* sprd_alloc_ionmem(uint32_t size , uint8_t iscache , int32_t* fd , void** viraddr)
{
	MemIon *pHeapIon = NULL;
	*fd = -1;
	*viraddr = NULL;
	if(iscache != 0)
	{
		//USE ION_HEAP_ID_MASK_MM temp , may be modify to SYSTEM
		pHeapIon = new MemIon("/dev/ion" , size , 0 ,  (1 << 31) | ION_HEAP_ID_MASK_MM | ION_FLAG_NO_CLEAR);
	}
	else
	{
		//USE ION_HEAP_ID_MASK_MM temp , may be modify to SYSTEM
		pHeapIon =  new MemIon("/dev/ion", size, MemIon::NO_CACHING , ION_HEAP_ID_MASK_MM | ION_FLAG_NO_CLEAR);
	}
	if(pHeapIon != NULL)
	{
		*viraddr = pHeapIon->getBase();
		*fd = pHeapIon->getHeapID();
	}
	VDSP_CMD_LOGI("alloc ionmem viraddr:%p , fd:%d" , *viraddr , *fd);
	return pHeapIon;
}
__attribute__ ((visibility("default"))) int sprd_flush_ionmem(void* handle , void* vaddr, void* paddr, uint32_t size)
{
	MemIon* pHeapIon = (MemIon*) handle;
	if(pHeapIon != NULL)
	{
		int32_t fd = pHeapIon->getHeapID();
		int32_t ret = pHeapIon->Flush_ion_buffer(fd , vaddr , paddr, size);
		if(0 == ret)
		{
			return SPRD_VDSP_CMD_SUCCESS;
		}
	}
	return SPRD_VDSP_CMD_FAIL;
}
__attribute__ ((visibility("default"))) int sprd_invalid_ionmem(void* handle)
{
        MemIon* pHeapIon = (MemIon*) handle;
        if(pHeapIon != NULL)
        {
                int32_t fd = pHeapIon->getHeapID();
		int32_t ret = pHeapIon->Invalid_ion_buffer(fd);
		if(ret == 0)
			return SPRD_VDSP_CMD_SUCCESS;
        }
        return SPRD_VDSP_CMD_FAIL;
}
__attribute__ ((visibility("default"))) int sprd_free_ionmem(void* handle)
{
	int errno;
	MemIon* pHeapIon = (MemIon*) handle;
	errno = SPRD_VDSP_CMD_SUCCESS;
	if(pHeapIon != NULL)
	{
		delete pHeapIon;
	}
	else
	{
		VDSP_CMD_LOGE("input handle is NULL");
		errno = SPRD_VDSP_CMD_FAIL;
	}
	return errno;
}
