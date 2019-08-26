#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vdsp_interface_internal.h"
#include "xrp_api.h"
#include "example_namespace.h"
#include <android/log.h>
__attribute__ ((visibility("default"))) void *sprd_vdsp_open_device(int idx , enum sprd_vdsp_worktype type)
{
	enum xrp_status in_status = -1;
	void *device = NULL;
	enum xrp_open_type opentype;
	switch(type)
	{
	case SPRD_VDSP_WORK_NORMAL:
		opentype = XRP_OPEN_NORMAL_TYPE;
		break;
	case SPRD_VDSP_WORK_FACEID:
		opentype = XRP_OPEN_FACEID_TYPE;
		break;
	default:
		return NULL;
	}
	device = xrp_open_device_newmode(idx , opentype , &in_status);

	return device;
}
__attribute__ ((visibility("default"))) void sprd_vdsp_release_device(void *device)
{
	return xrp_release_device_newmode(device);
}
#ifdef USE_SPRD_MODE
__attribute__ ((visibility("default"))) int sprd_vdsp_send_command(void *device , const char *nsid ,
									struct sprd_vdsp_inout *input, struct sprd_vdsp_inout *output,
									struct sprd_vdsp_inout *buffer ,  uint32_t buf_num,
									enum sprd_xrp_queue_priority priority)
{
	enum xrp_status status;
	struct xrp_queue *queue;
	struct xrp_buffer_group *group;
        struct xrp_buffer **buf;
	unsigned int i;
	void *input_vir = NULL;
	void *output_vir = NULL;
	int32_t inputfd = -1;
	int32_t outputfd = -1;;
	uint32_t inputsize = 0;
	uint32_t outputsize = 0;
	enum sprd_vdsp_status ret = SPRD_XRP_STATUS_SUCCESS;
	if(input != NULL)
	{
		input_vir = input->vir_addr;
		inputfd = input->fd;
		inputsize = (uint32_t)input->size;
	}
	if(output != NULL)
	{
		output_vir = output->vir_addr;
		outputfd = output->fd;
		outputsize = (uint32_t) output->size;
	}
	buf = calloc(buf_num , sizeof(struct xrp_buffer *));
	if(buf == NULL)
	{
		ret = SPRD_XRP_STATUS_FAILURE;
		return ret;
        }
	group = xrp_create_buffer_group(&status);
	if(XRP_STATUS_SUCCESS != status)
	{
		free(buf);
		return SPRD_XRP_STATUS_FAILURE;
	}
	status = -1;
	queue = xrp_create_nsp_queue(device , nsid , priority , &status);
	if(XRP_STATUS_SUCCESS != status)
	{
		free(buf);
		xrp_release_buffer_group(group);
		return SPRD_XRP_STATUS_FAILURE;
	}
	status = -1;
	for(i = 0; i < buf_num; i++)
	{
		buf[i] = xrp_create_buffer(device, buffer[i].size , buffer[i].vir_addr, buffer[i].fd , &status);
		if(XRP_STATUS_SUCCESS != status)
		{
			xrp_release_buffer_group(group);
			for(unsigned int j = 0; j < i; j++)
			{
				xrp_release_buffer(buf[j]);
			}
			xrp_release_queue(queue);
			free(buf);
			return SPRD_XRP_STATUS_FAILURE;
		}
		status = -1;
		xrp_add_buffer_to_group(group, buf[i], XRP_READ_WRITE, &status);
		if(XRP_STATUS_SUCCESS != status)
		{
			xrp_release_buffer_group(group);
			for(unsigned int j = 0; j < i; j++)
			{
				xrp_release_buffer(buf[j]);
			}
			xrp_release_queue(queue);
			free(buf);
			return SPRD_XRP_STATUS_FAILURE;
		}
		status = -1;
	}
	xrp_run_command_sync(queue , input_vir , inputfd , inputsize ,
					output_vir , outputfd , outputsize,
					group , &status);
	if(XRP_STATUS_SUCCESS != status)
	{
		ret = SPRD_XRP_STATUS_FAILURE;
	}
	xrp_release_buffer_group(group);
	for(i = 0; i < buf_num; i++)
	{
		xrp_release_buffer(buf[i]);
	}
	xrp_release_queue(queue);
	free(buf);
	return ret;
}
static enum xrp_access_flags translate_access_flag(enum sprd_vdsp_bufflag inflag)
{
	switch(inflag)
	{
	case SPRD_VDSP_XRP_READ:
		return XRP_READ;
	case SPRD_VDSP_XRP_WRITE:
		return XRP_WRITE;
	case SPRD_VDSP_XRP_READ_WRITE:
		return XRP_READ_WRITE;
	default:
		return XRP_READ;	
	}
}
__attribute__ ((visibility("default"))) int sprd_vdsp_run_faceid_command_directly(void *device,unsigned long in_data, unsigned int in_height,
								unsigned int in_width,unsigned long *out_data)
{
	if (XRP_STATUS_SUCCESS != xrp_run_faceid_command(device,in_data, in_height, in_width, out_data))
		return SPRD_XRP_STATUS_FAILURE;

	return SPRD_XRP_STATUS_SUCCESS;
}
__attribute__ ((visibility("default"))) int sprd_vdsp_send_command_directly(void *device , const char *nsid ,
                                                                        struct sprd_vdsp_inout *input, struct sprd_vdsp_inout *output,
                                                                        struct sprd_vdsp_inout *buffer ,  uint32_t buf_num,
                                                                        enum sprd_xrp_queue_priority priority)
{
        enum xrp_status status;
         struct xrp_buffer_group *group;
        struct xrp_buffer **buf;
        unsigned int i;
	void *input_vir = NULL;
	void *output_vir = NULL;
	int32_t inputfd = -1;
	int32_t outputfd = -1;;
	uint32_t inputsize = 0;
	uint32_t outputsize = 0;
	char ns_id[XRP_NAMESPACE_ID_SIZE];
        enum sprd_vdsp_status ret = SPRD_XRP_STATUS_SUCCESS;
	memset(ns_id , 0 , XRP_NAMESPACE_ID_SIZE);
	strncpy(ns_id , nsid , strlen(nsid));
	if(input != NULL)
        {
                input_vir = input->vir_addr;
                inputfd = input->fd;
                inputsize = (uint32_t)input->size;
        }
        if(output != NULL)
        {
                output_vir = output->vir_addr;
                outputfd = output->fd;
                outputsize = (uint32_t) output->size;
        }
        buf = calloc(buf_num , sizeof(struct xrp_buffer *));
        if(buf == NULL)
        {
                ret = SPRD_XRP_STATUS_FAILURE;
		__android_log_print(ANDROID_LOG_ERROR , "vdsp_interface_interna" , "func:%s line:%d , buf is NULL, error\n" , __func__ , __LINE__);
                return ret;
        }
        group = xrp_create_buffer_group(&status);
        if(XRP_STATUS_SUCCESS != status)
        {
                free(buf);
		__android_log_print(ANDROID_LOG_ERROR , "vdsp_interface_interna" , "func:%s line:%d , xrp_create_buffer_group , error\n" , __func__ , __LINE__);
                return SPRD_XRP_STATUS_FAILURE;
        }
        status = -1;
	for(i = 0; i < buf_num; i++)
        {
                buf[i] = xrp_create_buffer(device, buffer[i].size , buffer[i].vir_addr, buffer[i].fd , &status);
                if(XRP_STATUS_SUCCESS != status)
                {
                        xrp_release_buffer_group(group);
                        for(unsigned int j = 0; j < i; j++)
                        {
                                xrp_release_buffer(buf[j]);
                        }
                        free(buf);
			__android_log_print(ANDROID_LOG_ERROR , "vdsp_interface_interna" , "func:%s line:%d , xrp_create_buffer i:%d, error\n" , __func__ , __LINE__ , i);
                        return SPRD_XRP_STATUS_FAILURE;
                }
                status = -1;
                xrp_add_buffer_to_group(group, buf[i], translate_access_flag(buffer[i].flag), &status);
                if(XRP_STATUS_SUCCESS != status)
                {
                        xrp_release_buffer_group(group);
                        for(unsigned int j = 0; j < i; j++)
                        {
                                xrp_release_buffer(buf[j]);
                        }
                        free(buf);
			__android_log_print(ANDROID_LOG_ERROR ,"vdsp_interface_interna" , "func:%s line:%d , xrp_add_buffer_to_group i:%d, error\n" , __func__ , __LINE__ , i);
                        return SPRD_XRP_STATUS_FAILURE;
                }
                status = -1;
        }

	/*ioctl */
	xrp_run_command_directly(device,ns_id , priority, input_vir , inputfd ,inputsize,output_vir , outputfd , outputsize, group, &status);
        if(XRP_STATUS_SUCCESS != status)
        {
		__android_log_print(ANDROID_LOG_ERROR ,"vdsp_interface_interna" , "func:%s xrp_run_command_directly status:%d\n" , __func__ , status);
                ret = SPRD_XRP_STATUS_FAILURE;
        }
        xrp_release_buffer_group(group);
	for(i = 0; i < buf_num; i++)
        {
                xrp_release_buffer(buf[i]);
        }
        free(buf);
	__android_log_print(ANDROID_LOG_DEBUG,"vdsp_interface_interna" , "func:%s ret:%d\n" , __func__ , ret);
        return ret;
}
#endif
__attribute__ ((visibility("default"))) int sprd_vdsp_load_library(void *device , struct sprd_vdsp_inout *buffer , const char *libname , enum sprd_xrp_queue_priority priority)
{
	struct sprd_vdsp_inout input;
	int ret;
	int32_t fd = -1;
	void *inputhandle = NULL;
	void * inputaddr = NULL;
	inputhandle = sprd_alloc_ionmem(USER_LIBRARY_CMD_LOAD_UNLOAD_INPUTSIZE , 0 , &fd , &inputaddr);
        if(inputhandle == NULL)
        {
		__android_log_print(ANDROID_LOG_ERROR ,"vdsp_interface_interna" , "func:%s sprd_alloc_ionmem failed\n" , __func__);
                return SPRD_XRP_STATUS_FAILURE;
        }
        input.fd = fd;
        input.vir_addr = inputaddr;
        input.size = USER_LIBRARY_CMD_LOAD_UNLOAD_INPUTSIZE;
        input.vir_addr[0] = 1; /*load flag*/
        sprintf(&input.vir_addr[1] , "%s" , libname);

	ret = sprd_vdsp_send_command_directly(device , (const char *)XRP_EXAMPLE_V3_NSID , &input , NULL , buffer , 1 , priority);
	sprd_free_ionmem(inputhandle);
	return ret;
}
__attribute__ ((visibility("default"))) int sprd_vdsp_unload_library(void *device , const char *libname , enum sprd_xrp_queue_priority priority)
{
	struct sprd_vdsp_inout input;
	int ret = SPRD_XRP_STATUS_FAILURE;
	int32_t fd = -1;
	void *inputhandle = NULL;
	void * inputaddr = NULL;
	inputhandle = sprd_alloc_ionmem(USER_LIBRARY_CMD_LOAD_UNLOAD_INPUTSIZE , 0 , &fd , &inputaddr);
	if(inputhandle == NULL)
	{
		__android_log_print(ANDROID_LOG_ERROR ,"vdsp_interface_interna" , "func:%s sprd_alloc_ionmem failed\n" , __func__);
		return ret;
	}
	input.fd = fd;
	input.vir_addr = inputaddr;
	input.size = USER_LIBRARY_CMD_LOAD_UNLOAD_INPUTSIZE;
	input.vir_addr[0] = 2; /*unload flag*/
	sprintf(&input.vir_addr[1] , "%s" , libname);
	ret = sprd_vdsp_send_command_directly(device , (const char *)XRP_EXAMPLE_V3_NSID , &input , NULL , NULL , 0 , priority);
	sprd_free_ionmem(inputhandle);
	return ret;
}

