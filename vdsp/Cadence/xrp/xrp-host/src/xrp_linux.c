/*
 * Copyright (c) 2016 - 2018 Cadence Design Systems Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <log/log_main.h>
#ifndef _UAPI_ASM_GENERIC_INT_LL64_H
typedef uint32_t __u32;
typedef uint64_t __u64;
#endif

#include "xrp_host_common.h"
#include "xrp_host_impl.h"
#include "xrp_kernel_defs.h"
#ifdef USE_SPRD_MODE
#include "vdsp_interface.h"
#include <android/log.h>
#endif

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "XrpServer"

struct xrp_request {
	struct xrp_queue_item q;

	void *in_data;
	void *out_data;
	int in_data_fd;
	int out_data_fd;
	size_t in_data_size;
	size_t out_data_size;
	struct xrp_buffer_group *buffer_group;
	struct xrp_event *event;
};

/* Device API. */
#ifdef USE_SPRD_MODE
enum sprd_vdsp_worktype translate_opentype(enum xrp_open_type type)
{
	switch(type) {
	case XRP_OPEN_NORMAL_TYPE:
		return SPRD_VDSP_WORK_NORMAL;
        case XRP_OPEN_FACEID_TYPE:
		return SPRD_VDSP_WORK_FACEID;
	default:
		return SPRD_VDSP_WORK_MAXTYPE;
	}
}
#endif
#ifdef USE_SPRD_MODE
struct xrp_device *xrp_open_device(__unused int idx, enum xrp_open_type type , enum xrp_status *status)
#else
struct xrp_device *xrp_open_device(int idx, enum xrp_open_type type , enum xrp_status *status)
#endif
{
#ifdef USE_SPRD_MODE
	struct xrp_device *device;
	struct vdsp_handle handle;
	int ret = 0;
	/*use open flag to distinguish normal boot or faceid boot*/
	ret = sprd_cavdsp_open_device(translate_opentype(type) , &handle);
	if (ret != 0) {
		set_status(status, XRP_STATUS_FAILURE);
		return NULL;
	}
	device = alloc_refcounted(sizeof(*device));
	if (!device) {
		set_status(status, XRP_STATUS_FAILURE);
		return NULL;
	}
	device->impl.handle.fd = handle.fd;
	device->impl.handle.generation = handle.generation;
	set_status(status, XRP_STATUS_SUCCESS);
	return device;
#else
	struct xrp_device *device;
        char name[sizeof("/dev/vdsp") + sizeof(int) * 4];
        int fd = -1;

        sprintf(name, "/dev/vdsp%u", idx);
        /*use open flag to distinguish normal boot or faceid boot*/
        if(type == XRP_OPEN_NORMAL_TYPE) {
                fd = open(name, O_RDONLY);
        }
        else if(type == XRP_OPEN_FACEID_TYPE) {
                fd = open(name, O_RDWR);
        }
        if (fd == -1) {
                set_status(status, XRP_STATUS_FAILURE);
                return NULL;
        }
        device = alloc_refcounted(sizeof(*device));
        if (!device) {
                set_status(status, XRP_STATUS_FAILURE);
                return NULL;
        }
        device->impl.fd = fd;
        set_status(status, XRP_STATUS_SUCCESS);
        return device;
#endif
}
struct xrp_device *xrp_open_device_newmode(int idx, enum xrp_open_type type , enum xrp_status *status)
{
        struct xrp_device *device;
        char name[sizeof("/dev/vdsp") + sizeof(int) * 4];
        int fd = -1;

        sprintf(name, "/dev/vdsp%u", idx);
        /*use open flag to distinguish normal boot or faceid boot*/
        if(type == XRP_OPEN_NORMAL_TYPE) {
                fd = open(name, O_RDONLY);
        }
        else if(type == XRP_OPEN_FACEID_TYPE) {
                fd = open(name, O_RDWR);
        }
	ALOGD("xrp_open_device_newmode name:%s , type:%d , fd:%d\n" , name , type , fd);
        if (fd == -1) {
                set_status(status, XRP_STATUS_FAILURE);
                return NULL;
        }
        device = alloc_refcounted(sizeof(*device));
        if (!device) {
                set_status(status, XRP_STATUS_FAILURE);
                return NULL;
        }
        device->impl.fd = fd;
        set_status(status, XRP_STATUS_SUCCESS);
        return device;
}
void xrp_impl_release_device(struct xrp_device *device)
{
#ifdef USE_SPRD_MODE
	sprd_cavdsp_close_device(&device->impl.handle);
#else
	close(device->impl.fd);
#endif
}

void xrp_impl_release_device_newmode(struct xrp_device *device)
{
	close(device->impl.fd);
}

/* Buffer API. */

void xrp_impl_create_device_buffer(struct xrp_device *device,
				   struct xrp_buffer *buffer,
				   size_t size,
				   enum xrp_status *status)
{
#ifdef USE_SPRD_MODE
	int fd;
	void *viraddr = NULL;
	void *ionhandle = NULL;
#else
	struct xrp_ioctl_alloc ioctl_alloc = {
		.size = size,
	};
	int ret
#endif

	xrp_retain_device(device);
	buffer->device = device;
#ifdef USE_SPRD_MODE
	ionhandle = sprd_alloc_ionmem(size, 1 , &fd, &viraddr);
	if(ionhandle == NULL) {
		xrp_release_device(buffer->device);
		set_status(status, XRP_STATUS_FAILURE);
		return;
	}
	buffer->ptr = viraddr;
	buffer->ionhandle = ionhandle;
	buffer->ion_fd = fd;
#else
	ret = ioctl(buffer->device->impl.fd, XRP_IOCTL_ALLOC, &ioctl_alloc);
	if (ret < 0) {
		xrp_release_device(buffer->device);
		set_status(status, XRP_STATUS_FAILURE);
		return;
	}
	buffer->ptr = (void *)(uintptr_t)ioctl_alloc.addr;
#endif
	buffer->size = size;
	set_status(status, XRP_STATUS_SUCCESS);
}

void xrp_impl_release_device_buffer(struct xrp_buffer *buffer)
{
#ifdef USE_SPRD_MODE
	sprd_free_ionmem(buffer->ionhandle);
#else
	struct xrp_ioctl_alloc ioctl_alloc = {
		.addr = (uintptr_t)buffer->ptr,
	};
	ioctl(buffer->device->impl.fd,
	      XRP_IOCTL_FREE, &ioctl_alloc);
#endif
	xrp_release_device(buffer->device);
}

/* Queue API. */
#ifdef USE_SPRD_MODE
static enum sprd_vdsp_bufflag translate_access_flag(enum xrp_access_flags flag)
{
	switch(flag) {
	case XRP_READ:
		return SPRD_VDSP_XRP_READ;
	case XRP_WRITE:
		return SPRD_VDSP_XRP_WRITE;
	case XRP_READ_WRITE:
		return SPRD_VDSP_XRP_READ_WRITE;
	default:
		return SPRD_VDSP_XRP_ACCESS_MAX;
	}
}
static void _xrp_run_command(struct xrp_queue *queue,
				const void *in_data, int in_data_fd , size_t in_data_size,
				void *out_data, int out_data_fd , size_t out_data_size,
				struct xrp_buffer_group *buffer_group,
				enum xrp_status *status)
{
	int ret;
	struct sprd_vdsp_client_inout input , output;
	struct sprd_vdsp_client_inout *buffer = NULL;
	void *pinput = NULL;
	void *poutput = NULL;
	fprintf(stderr , "yzl add %s enter queue:%p\n" , __func__ , queue);
	if (buffer_group)
		xrp_mutex_lock(&buffer_group->mutex);
	{
		size_t n_buffers = buffer_group ? buffer_group->n_buffers : 0;
		struct xrp_ioctl_buffer ioctl_buffer[n_buffers];/* TODO */
		struct xrp_ioctl_queue ioctl_queue = {
                        .flags = (queue->use_nsid ? XRP_QUEUE_FLAG_NSID : 0) |
                                ((queue->priority << XRP_QUEUE_FLAG_PRIO_SHIFT) &
                                 XRP_QUEUE_FLAG_PRIO),
                        .in_data_size = in_data_size,
			.in_data_fd = in_data_fd,
                        .out_data_size = out_data_size,
			.out_data_fd = out_data_fd,
                        .buffer_size = n_buffers *
                                sizeof(struct xrp_ioctl_buffer),
                        .in_data_addr = (uintptr_t)in_data,
                        .out_data_addr = (uintptr_t)out_data,
                        .buffer_addr = (uintptr_t)ioctl_buffer,
                        .nsid_addr = (uintptr_t)queue->nsid,
                };
                size_t i;
		if(n_buffers > 0) {
			buffer = (struct sprd_vdsp_client_inout*) calloc(n_buffers , sizeof(struct sprd_vdsp_client_inout));
			if(buffer == NULL) {
				fprintf(stderr , "yzl add %s calloc n_buffers:%zu is NULL\n" , __func__ , n_buffers);
				if (buffer_group)
					xrp_mutex_unlock(&buffer_group->mutex);
				ret = -1;
				goto __exit;
			}
		}
                for (i = 0; i < n_buffers; ++i) {
			buffer[i].fd = buffer_group->buffer[i].buffer->ion_fd,
			buffer[i].viraddr = buffer_group->buffer[i].buffer->ptr;
			buffer[i].size = buffer_group->buffer[i].buffer->size;
			buffer[i].flag = translate_access_flag(buffer_group->buffer[i].access_flags);
                }
		if (buffer_group)
                        xrp_mutex_unlock(&buffer_group->mutex);
                fprintf(stderr , "yzl add %s , flag:%x\n" , __func__ , ioctl_queue.flags);
		/*set input */
		printf("yzl add %s in_data:%p, indatasize:%zu, out_data:%p,outdatasize:%zu\n" , __func__ , in_data, in_data_size , out_data , out_data_size);
		if((in_data == NULL) || (in_data_size == 0)) {
			pinput = NULL;
		} else {
			pinput = &input;
			input.fd = in_data_fd;
			input.viraddr = (void*)in_data;
			input.size = in_data_size;
			input.flag = 0;
		}
		/*set output*/
		if((out_data == NULL) || (0 == out_data_size)) {
			poutput = NULL;
		} else {
			poutput = &output;
			output.fd = out_data_fd;
			output.viraddr = out_data;
			output.size = out_data_size;
			output.flag = 0;
		}
		ret = sprd_cavdsp_send_cmd(&queue->device->impl.handle , queue->nsid , pinput , poutput ,
                                                buffer , n_buffers , queue->priority);
		if(buffer != NULL) {
			free(buffer);
		}
        }
__exit:
        if (ret < 0)
                set_status(status, XRP_STATUS_FAILURE);
        else
                set_status(status, XRP_STATUS_SUCCESS);
}
void xrp_run_command_directly(struct xrp_device *device ,const char * nsid , unsigned int priority,
                                const void *in_data, int in_data_fd , size_t in_data_size,
                                void *out_data, int out_data_fd , size_t out_data_size,
                                struct xrp_buffer_group *buffer_group,
                                enum xrp_status *status)
{
        int ret;
        {
                size_t n_buffers = buffer_group ? buffer_group->n_buffers : 0;
                struct xrp_ioctl_buffer ioctl_buffer[n_buffers];/* TODO */
                struct xrp_ioctl_queue ioctl_queue = {
                        .flags = (1 ? XRP_QUEUE_FLAG_NSID : 0) |
                                ((priority << XRP_QUEUE_FLAG_PRIO_SHIFT) &
                                 XRP_QUEUE_FLAG_PRIO),
                        .in_data_size = in_data_size,
                        .in_data_fd = in_data_fd,
                        .out_data_size = out_data_size,
                        .out_data_fd = out_data_fd,
                        .buffer_size = n_buffers *
                                sizeof(struct xrp_ioctl_buffer),
                        .in_data_addr = (uintptr_t)in_data,
                        .out_data_addr = (uintptr_t)out_data,
                        .buffer_addr = (uintptr_t)ioctl_buffer,
                        .nsid_addr = (uintptr_t)nsid,
                };
                size_t i;

                for (i = 0; i < n_buffers; ++i) {
                        ioctl_buffer[i] = (struct xrp_ioctl_buffer){
                                .flags = buffer_group->buffer[i].access_flags,
                                .size = buffer_group->buffer[i].buffer->size,
                                .addr = (uintptr_t)buffer_group->buffer[i].buffer->ptr,
                                .fd = buffer_group->buffer[i].buffer->ion_fd,
                        };
                }
                fprintf(stderr , "yzl add %s , flag:%x\n" , __func__ , ioctl_queue.flags);
                ret = ioctl(device->impl.fd,
                            XRP_IOCTL_QUEUE, &ioctl_queue);
        }
	if (ret < 0)
                set_status(status, XRP_STATUS_FAILURE);
        else
                set_status(status, XRP_STATUS_SUCCESS);
}

void xrp_run_faceid_command_directly(struct xrp_device *device,
								int in_fd , int out_fd,
                                enum xrp_status *status)
{
        int ret;
        {
			struct xrp_faceid_ctrl ioctrl_faceid = {
			    .in_fd = in_fd,
			    .out_fd = out_fd,
			};

			ret = ioctl(device->impl.fd,XRP_IOCTL_FACEID_CMD, &ioctrl_faceid);
        }
		if (ret < 0){
                set_status(status, XRP_STATUS_FAILURE);
        }else{
				set_status(status, XRP_STATUS_SUCCESS);
        	}
}
#else

static void _xrp_run_command(struct xrp_queue *queue,
			     const void *in_data, size_t in_data_size,
			     void *out_data, size_t out_data_size,
			     struct xrp_buffer_group *buffer_group,
			     enum xrp_status *status)
{
	int ret;

	if (buffer_group)
		xrp_mutex_lock(&buffer_group->mutex);
	{
		size_t n_buffers = buffer_group ? buffer_group->n_buffers : 0;
		struct xrp_ioctl_buffer ioctl_buffer[n_buffers];/* TODO */
		struct xrp_ioctl_queue ioctl_queue = {
			.flags = (queue->use_nsid ? XRP_QUEUE_FLAG_NSID : 0) |
				((queue->priority << XRP_QUEUE_FLAG_PRIO_SHIFT) &
				 XRP_QUEUE_FLAG_PRIO),
			.in_data_size = in_data_size,
			.out_data_size = out_data_size,
			.buffer_size = n_buffers *
				sizeof(struct xrp_ioctl_buffer),
			.in_data_addr = (uintptr_t)in_data,
			.out_data_addr = (uintptr_t)out_data,
			.buffer_addr = (uintptr_t)ioctl_buffer,
			.nsid_addr = (uintptr_t)queue->nsid,
		};
		size_t i;

		for (i = 0; i < n_buffers; ++i) {
			ioctl_buffer[i] = (struct xrp_ioctl_buffer){
				.flags = buffer_group->buffer[i].access_flags,
				.size = buffer_group->buffer[i].buffer->size,
				.addr = (uintptr_t)buffer_group->buffer[i].buffer->ptr,
			};
		}
		if (buffer_group)
			xrp_mutex_unlock(&buffer_group->mutex);
		fprintf(stderr , "yzl add %s , flag:%x\n" , __func__ , ioctl_queue.flags);
		ret = ioctl(queue->device->impl.fd,
			    XRP_IOCTL_QUEUE, &ioctl_queue);
	}

	if (ret < 0)
		set_status(status, XRP_STATUS_FAILURE);
	else
		set_status(status, XRP_STATUS_SUCCESS);
}
#endif

static void xrp_request_process(struct xrp_queue_item *q,
				void *context)
{
	enum xrp_status status;
	struct xrp_request *rq = (struct xrp_request *)q;
#ifdef USE_SPRD_MODE
	fprintf(stderr , "yzl add %s before _xrp_run_command\n" , __func__);
	_xrp_run_command(context,rq->in_data, rq->in_data_fd , rq->in_data_size,
				rq->out_data , rq->out_data_fd , rq->out_data_size,
				rq->buffer_group,
				&status);
	fprintf(stderr , "yzl add %s after _xrp_run_command\n" , __func__);
#else
	_xrp_run_command(context,
			 rq->in_data, rq->in_data_size,
			 rq->out_data, rq->out_data_size,
			 rq->buffer_group,
			 &status);
#endif
	if (rq->buffer_group)
		xrp_release_buffer_group(rq->buffer_group);

	if (rq->event) {
		struct xrp_event *event = rq->event;
		xrp_cond_lock(&event->impl.cond);
		event->status = status;
		xrp_cond_broadcast(&event->impl.cond);
		xrp_cond_unlock(&event->impl.cond);
		xrp_release_event(event);
	}
	free(rq->in_data);
	free(rq);
}

void xrp_impl_create_queue(struct xrp_queue *queue,
			   enum xrp_status *status)
{
	xrp_queue_init(&queue->impl.queue, queue->priority , queue, xrp_request_process);
	set_status(status, XRP_STATUS_SUCCESS);
}

void xrp_impl_release_queue(struct xrp_queue *queue)
{
	xrp_queue_destroy(&queue->impl.queue);
}

/* Communication API */
#ifdef USE_SPRD_MODE
void xrp_enqueue_command(struct xrp_queue *queue,
                         const void *in_data, int in_data_fd , size_t in_data_size,
                         void *out_data, int out_data_fd , size_t out_data_size,
                         struct xrp_buffer_group *buffer_group,
                         struct xrp_event **evt,
                         enum xrp_status *status)
{
	struct xrp_request *rq;
        void *in_data_copy;
	fprintf(stderr , "yzl add %s enter queue:%p , indata:%p , infd:%d,insize:%d,outdata:%p,\
					outdatafd:%d,outdatasize:%d,buffer_group:%p\n" , __func__,
					queue , in_data, in_data_fd , (int)in_data_size , out_data, out_data_fd , (int)out_data_size,buffer_group);
        rq = malloc(sizeof(*rq));
        in_data_copy = malloc(in_data_size);

        if (!rq || (in_data_size && !in_data_copy)) {
		fprintf(stderr , "yzl add %s exit 0\n" , __func__);
                free(in_data_copy);
                free(rq);
                set_status(status, XRP_STATUS_FAILURE);
                return;
        }
	fprintf(stderr , "yzl add %s enter step 1\n" , __func__);
        memcpy(in_data_copy, in_data, in_data_size);
        rq->in_data = in_data_copy;
        rq->in_data_size = in_data_size;
        rq->out_data = out_data;
	rq->out_data_size = out_data_size;
	rq->in_data_fd = in_data_fd;
	rq->out_data_fd = out_data_fd;
	fprintf(stderr , "yzl add %s enter step2\n" , __func__);
        if (evt) {
                struct xrp_event *event = xrp_event_create();

                if (!event) {
                        free(rq->in_data);
                        free(rq);
                        set_status(status, XRP_STATUS_FAILURE);
                        return;
                }
                xrp_retain_queue(queue);
		event->queue = queue;
                *evt = event;
                xrp_retain_event(event);
                rq->event = event;
        } else {
                rq->event = NULL;
        }

        if (buffer_group)
                xrp_retain_buffer_group(buffer_group);
        rq->buffer_group = buffer_group;

        set_status(status, XRP_STATUS_SUCCESS);
        xrp_queue_push(&queue->impl.queue, &rq->q);
	fprintf(stderr , "yzl add %s exit\n" , __func__);
}
#else
void xrp_enqueue_command(struct xrp_queue *queue,
			 const void *in_data, size_t in_data_size,
			 void *out_data, size_t out_data_size,
			 struct xrp_buffer_group *buffer_group,
			 struct xrp_event **evt,
			 enum xrp_status *status)
{
	struct xrp_request *rq;
	void *in_data_copy;

	rq = malloc(sizeof(*rq));
	in_data_copy = malloc(in_data_size);

	if (!rq || (in_data_size && !in_data_copy)) {
		free(in_data_copy);
		free(rq);
		set_status(status, XRP_STATUS_FAILURE);
		return;
	}

	memcpy(in_data_copy, in_data, in_data_size);
	rq->in_data = in_data_copy;
	rq->in_data_size = in_data_size;
	rq->out_data = out_data;
	rq->out_data_size = out_data_size;

	if (evt) {
		struct xrp_event *event = xrp_event_create();

		if (!event) {
			free(rq->in_data);
			free(rq);
			set_status(status, XRP_STATUS_FAILURE);
			return;
		}
		xrp_retain_queue(queue);
		event->queue = queue;
		*evt = event;
		xrp_retain_event(event);
		rq->event = event;
	} else {
		rq->event = NULL;
	}

	if (buffer_group)
		xrp_retain_buffer_group(buffer_group);
	rq->buffer_group = buffer_group;

	set_status(status, XRP_STATUS_SUCCESS);
	xrp_queue_push(&queue->impl.queue, &rq->q);
}
#endif
