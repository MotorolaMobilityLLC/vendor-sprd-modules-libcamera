
/*
 * Copyright (c) 2016 - 2017 Cadence Design Systems Inc.
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

//#include <assert_1.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <utils/Timers.h>
#include <time.h>
#ifdef HAVE_NANOSLEEP
#include <time.h>
#endif
#include "xrp_interface.h"
#include "vdsp_interface_internal.h"
#include "vdsp_interface.h"
#include "example_namespace.h"
#ifdef HAVE_THREADS_XOS
#include <xtensa/xos.h>
#endif
#ifdef HAVE_XTENSA_HAL_H
#include <xtensa/hal.h>
#endif

#ifdef USE_SPRD_MODE
#include <pthread.h>
#include <utils/Log.h>
#endif
#define assert_1(x)   if((x)==0) \
			fprintf(stderr , "[%s][line:%d]fatal error!!\n",__func__,__LINE__); \
		      else \
			 fprintf(stderr , "[%s][line:%d]status ok!!\n",__func__,__LINE__)

/* Test data transfer from and to in/out buffers */
static void f1(int devid)
{
	enum xrp_status status = -1;
	struct xrp_device *device;
	struct xrp_queue *queue;
	char in_buf[32];
	char out_buf[32];
	int i, j;
	fprintf(stderr , "yzl add f1 before sprd_xrp_open_device\n");
	device = sprd_xrp_open_device(devid, &status);
	fprintf(stderr , "yzl add f1 after sprd_xrp_open_device device:%p\n" ,device);
	assert_1(status == XRP_STATUS_SUCCESS);
	status = -1;
	queue = sprd_xrp_create_ns_queue(device, XRP_EXAMPLE_V1_NSID, &status);
	assert_1(status == XRP_STATUS_SUCCESS);
	status = -1;

	for (i = 0; i <= 32; ++i) {
		int mismatch = 0;
		memset(in_buf, i, sizeof(in_buf));
		memset(out_buf, 0, sizeof(out_buf));
		fprintf(stderr , "yzl add f1 before sprd_xrp_run_command_sync i:%d\n" , i);
#ifdef USE_SPRD_MODE
		sprd_xrp_run_command_sync(queue , in_buf , -1 , i , out_buf , -1 , i , NULL , &status);
#else
		sprd_xrp_run_command_sync(queue,
				     in_buf, i,
				     out_buf, i,
				     NULL, &status);
#endif
		fprintf(stderr , "yzl add f1 after sprd_xrp_run_command_sync i:%d\n" , i);
		assert_1(status == XRP_STATUS_SUCCESS);
		status = -1;

		for (j = 0; j < 32; ++j)
			mismatch += (out_buf[j] != (j < i ? i + j : 0));

		if (!mismatch)
			continue;

		for (j = 0; j < 32; ++j) {
			int ne = (out_buf[j] != (j < i ? i + j : 0));
			fprintf(stderr,
				"out_buf[%d] (%p) == 0x%02x %c= expected: 0x%02x\n",
				j, out_buf + j, (uint8_t)out_buf[j],
				ne ? '!' : '=',
				(j < i ? i + j : 0));
		}
		assert_1(mismatch == 0);
	}

	sprd_xrp_release_queue(queue);
	sprd_xrp_release_device(device);
}

/* Test asynchronous API */
static void f2(int devid)
{
	enum xrp_status status = -1;
	struct xrp_device *device;
	struct xrp_queue *queue;
	struct xrp_buffer_group *group;
	struct xrp_buffer *buf;
	void *data;
	struct xrp_event *event[2];
	void *inviraddr;
	void *outviraddr;
	uint32_t inputsize = 10;
	int32_t infd;
	int32_t outfd;
	uint32_t outputsize = 20;
	void *inputhandle;
	void *outputhandle;
	inputhandle = sprd_alloc_ionmem(inputsize , 0 , &infd, &inviraddr);
	outputhandle = sprd_alloc_ionmem(outputsize , 0 , &outfd , &outviraddr);
	printf("---------test inputhandle:%p , outputhandle:%p\n" , inputhandle , outputhandle);
	if(inputhandle)
		memset(inviraddr , 0xbb , inputsize);
	if(outputhandle)
		memset(outviraddr , 0xdd, outputsize);
	device = sprd_xrp_open_device(devid, &status);
	assert_1(status == XRP_STATUS_SUCCESS);
	status = -1;

	queue = sprd_xrp_create_ns_queue(device, XRP_EXAMPLE_V1_NSID, &status);
	assert_1(status == XRP_STATUS_SUCCESS);
	status = -1;

	group = sprd_xrp_create_buffer_group(&status);
	assert_1(status == XRP_STATUS_SUCCESS);
	status = -1;
#ifdef USE_SPRD_MODE
	buf = sprd_xrp_create_buffer(device, 1024, NULL, -1 , &status);
#else
	buf = sprd_xrp_create_buffer(device, 1024, NULL, &status);
#endif
	assert_1(status == XRP_STATUS_SUCCESS);
	status = -1;

	data = sprd_xrp_map_buffer(buf, 0, 1024, XRP_READ_WRITE, &status);
	assert_1(status == XRP_STATUS_SUCCESS);
	status = -1;

	memset(data, 0xab , 1024);

	sprd_xrp_unmap_buffer(buf, data, &status);
	assert_1(status == XRP_STATUS_SUCCESS);
	status = -1;

	sprd_xrp_add_buffer_to_group(group, buf, XRP_READ_WRITE, &status);
	assert_1(status == XRP_STATUS_SUCCESS);
	status = -1;
#ifdef USE_SPRD_MODE
	sprd_xrp_enqueue_command(queue,inviraddr , infd , inputsize, outviraddr, outfd , outputsize, group, NULL, &status);
#else
	sprd_xrp_enqueue_command(queue, NULL, 0, NULL, 0, group, NULL, &status);
#endif
	assert_1(status == XRP_STATUS_SUCCESS);
#ifdef USE_SPRD_MODE
	sprd_xrp_enqueue_command(queue, NULL, -1, 0, NULL, -1 , 0, group, event + 0, &status);
#else
	sprd_xrp_enqueue_command(queue, NULL, 0, NULL, 0, group, event + 0, &status);
#endif
	assert_1(status == XRP_STATUS_SUCCESS);
	status = -1;
#ifdef USE_SPRD_MODE
	sprd_xrp_enqueue_command(queue, NULL, -1, 0, NULL, -1 , 0, group, event + 1, &status);
#else
	sprd_xrp_enqueue_command(queue, NULL, 0, NULL, 0, group, event + 1, &status);
#endif

	assert_1(status == XRP_STATUS_SUCCESS);
	status = -1;
	sprd_xrp_release_buffer_group(group);
	sprd_xrp_release_buffer(buf);
	sprd_xrp_release_queue(queue);
	sprd_xrp_wait(event[1], &status);
	assert_1(status == XRP_STATUS_SUCCESS);
	status = -1;
	sprd_xrp_event_status(event[0], &status);
	assert_1(status != XRP_STATUS_PENDING);
	status = -1;
	sprd_xrp_wait(event[0], &status);
	assert_1(status == XRP_STATUS_SUCCESS);
	status = -1;
	sprd_xrp_release_event(event[0]);
	sprd_xrp_release_event(event[1]);
	sprd_xrp_release_device(device);
	if(inputhandle)
		sprd_free_ionmem(inputhandle);
	if(outputhandle)
		sprd_free_ionmem(outputhandle);
}

/* Test data transfer from and to device and user buffers */
static void f3(int devid)
{
	enum xrp_status status = -1;
	struct xrp_device *device;
	struct xrp_queue *queue;
	uint32_t sz;
	unsigned i;


	device = sprd_xrp_open_device(devid, &status);
	assert_1(status == XRP_STATUS_SUCCESS);
	status = -1;
	queue = sprd_xrp_create_ns_queue(device, XRP_EXAMPLE_V1_NSID, &status);
	assert_1(status == XRP_STATUS_SUCCESS);
	status = -1;

	for (sz = 2048; sz < 16384; sz <<= 1) {
		fprintf(stderr, "%s: sz = %zd\n", __func__, (size_t)sz);
		for (i = 0; i < 4; ++i) {
			void *p1 = (i & 1) ? malloc(sz) : NULL;
			void *p2 = (i & 2) ? malloc(sz) : NULL;
			struct xrp_buffer_group *group;
			struct xrp_buffer *buf1;
			struct xrp_buffer *buf2;
			void *data1;
			void *data2;

			group = sprd_xrp_create_buffer_group(&status);
			assert_1(status == XRP_STATUS_SUCCESS);
			status = -1;
#ifndef USE_SPRD_MODE
			buf1 = sprd_xrp_create_buffer(device, sz, p1, &status);
			assert_1(status == XRP_STATUS_SUCCESS);
			status = -1;
			buf2 = sprd_xrp_create_buffer(device, sz, p2, &status);
#else
			buf1 = sprd_xrp_create_buffer(device, sz, p1, -1,&status);
			assert_1(status == XRP_STATUS_SUCCESS);
			status = -1;
			buf2 = sprd_xrp_create_buffer(device, sz, p2, -1 , &status);
#endif
			assert_1(status == XRP_STATUS_SUCCESS);
			status = -1;

			data1 = sprd_xrp_map_buffer(buf1, 0, sz, XRP_READ_WRITE, &status);
			assert_1(status == XRP_STATUS_SUCCESS);
			status = -1;

			memset(data1, i + 3 + sz / 512, sz);
			sprd_xrp_unmap_buffer(buf1, data1, &status);
			assert_1(status == XRP_STATUS_SUCCESS);
			status = -1;

			sprd_xrp_add_buffer_to_group(group, buf1, XRP_READ, &status);
			assert_1(status == XRP_STATUS_SUCCESS);
			status = -1;
			sprd_xrp_add_buffer_to_group(group, buf2, XRP_WRITE, &status);
			assert_1(status == XRP_STATUS_SUCCESS);
			status = -1;
#ifdef USE_SPRD_MODE
			sprd_xrp_run_command_sync(queue , &sz , -1 , sizeof(sz) , NULL , -1 , 0 , group , &status);
#else
			sprd_xrp_run_command_sync(queue, &sz, sizeof(sz), NULL, 0, group, &status);
#endif
			assert_1(status == XRP_STATUS_SUCCESS);
			status = -1;
			sprd_xrp_release_buffer_group(group);

			data1 = sprd_xrp_map_buffer(buf1, 0, sz, XRP_READ_WRITE, &status);
			assert_1(status == XRP_STATUS_SUCCESS);
			status = -1;
			data2 = sprd_xrp_map_buffer(buf2, 0, sz, XRP_READ_WRITE, &status);
			assert_1(status == XRP_STATUS_SUCCESS);
			status = -1;
			assert_1(data1);
			assert_1(data2);
			fprintf(stderr, "comparing %p vs %p\n", data1, data2);
			if (memcmp(data1, data2, sz)) {
				for (i = 0; i < sz; ++i) {
					uint8_t v1 = ((uint8_t *)data1)[i];
					uint8_t v2 = ((uint8_t *)data2)[i];
					if (v1 != v2) {
						fprintf(stderr,
							"data1[%d] (%p) (== 0x%02x) != data2[%d] (%p) (== 0x%02x)\n",
							i, (uint8_t*)data1 + i, v1, i, (uint8_t*)data2 + i, v2);

					}
				}
				assert_1(0);
			}
			sprd_xrp_unmap_buffer(buf1, data1, &status);
			assert_1(status == XRP_STATUS_SUCCESS);
			status = -1;
			sprd_xrp_unmap_buffer(buf2, data2, &status);
			assert_1(status == XRP_STATUS_SUCCESS);
			status = -1;
			sprd_xrp_release_buffer(buf1);
			sprd_xrp_release_buffer(buf2);
			free(p1);
			free(p2);
		}
	}
	sprd_xrp_release_queue(queue);
	sprd_xrp_release_device(device);
}

/* Test xrp_set_buffer_in_group */
static void f4(int devid)
{
	enum xrp_status status = -1;
	struct xrp_device *device;
	struct xrp_queue *queue;
	struct xrp_buffer_group *group;
	struct xrp_buffer *buf1;
	struct xrp_buffer *buf2;
	struct xrp_buffer *buf3;
	size_t i;

	device = sprd_xrp_open_device(devid, &status);
	assert_1(status == XRP_STATUS_SUCCESS);
	status = -1;
	queue = sprd_xrp_create_ns_queue(device, XRP_EXAMPLE_V1_NSID, &status);
	assert_1(status == XRP_STATUS_SUCCESS);
	status = -1;
	group = sprd_xrp_create_buffer_group(&status);
	assert_1(status == XRP_STATUS_SUCCESS);
	status = -1;
#ifndef USE_SPRD_MODE
	buf1 = sprd_xrp_create_buffer(device, 1, NULL, &status);
	assert_1(status == XRP_STATUS_SUCCESS);
	status = -1;
	buf2 = sprd_xrp_create_buffer(device, 1, NULL, &status);
#else
	buf1 = sprd_xrp_create_buffer(device, 1, NULL,-1, &status);
	assert_1(status == XRP_STATUS_SUCCESS);
	status = -1;
	buf2 = sprd_xrp_create_buffer(device, 1, NULL, -1, &status);
#endif
	assert_1(status == XRP_STATUS_SUCCESS);
	status = -1;
	i = sprd_xrp_add_buffer_to_group(group, buf1, XRP_READ, &status);
	assert_1(status == XRP_STATUS_SUCCESS);
	status = -1;
	sprd_xrp_set_buffer_in_group(group, i, buf2, XRP_READ, &status);
	assert_1(status == XRP_STATUS_SUCCESS);
	status = -1;
	sprd_xrp_set_buffer_in_group(group, i + 1, buf2, XRP_READ, &status);
	assert_1(status == XRP_STATUS_FAILURE);
	status = -1;
	buf3 = sprd_xrp_get_buffer_from_group(group, i, &status);
	assert_1(status == XRP_STATUS_SUCCESS);
	status = -1;
	assert_1(buf3 == buf2);
	sprd_xrp_release_buffer(buf1);
	sprd_xrp_release_buffer(buf2);
	sprd_xrp_release_buffer(buf3);
	sprd_xrp_release_buffer_group(group);
	sprd_xrp_release_queue(queue);
	sprd_xrp_release_device(device);
}

/* Test xrp_buffer[_group]_get_info */
static void f5(int devid)
{
	enum xrp_status status = -1;
	struct xrp_device *device;
	struct xrp_queue *queue;
	struct xrp_buffer_group *group;
	struct xrp_buffer *buf1;
	struct xrp_buffer *buf2;
	size_t i;
	size_t sz;
	void *ptr;
	enum xrp_access_flags flags;

	device = sprd_xrp_open_device(devid, &status);
	assert_1(status == XRP_STATUS_SUCCESS);
	status = -1;
	queue = sprd_xrp_create_ns_queue(device, XRP_EXAMPLE_V1_NSID, &status);
	assert_1(status == XRP_STATUS_SUCCESS);
	status = -1;
	group = sprd_xrp_create_buffer_group(&status);
	assert_1(status == XRP_STATUS_SUCCESS);
	status = -1;
#ifndef USE_SPRD_MODE
	buf1 = sprd_xrp_create_buffer(device, 1, NULL, &status);
	assert_1(status == XRP_STATUS_SUCCESS);
	status = -1;
	buf2 = sprd_xrp_create_buffer(device, sizeof(i), &i, &status);
#else
	buf1 = sprd_xrp_create_buffer(device, 1, NULL, -1 , &status);
	assert_1(status == XRP_STATUS_SUCCESS);
	status = -1;
	buf2 = sprd_xrp_create_buffer(device, sizeof(i), &i, -1 , &status);
#endif
	assert_1(status == XRP_STATUS_SUCCESS);
	status = -1;
	i = sprd_xrp_add_buffer_to_group(group, buf1, XRP_READ, &status);
	assert_1(status == XRP_STATUS_SUCCESS);
	status = -1;

	sprd_xrp_buffer_get_info(buf1, XRP_BUFFER_SIZE_SIZE_T,
			    &sz, sizeof(sz), &status);
	assert_1(status == XRP_STATUS_SUCCESS);
	status = -1;
	assert_1(sz == 1);
	sprd_xrp_buffer_get_info(buf1, XRP_BUFFER_HOST_POINTER_PTR,
			    &ptr, sizeof(ptr), &status);
	assert_1(status == XRP_STATUS_SUCCESS);
	status = -1;
	assert_1(ptr == NULL);
	sprd_xrp_buffer_get_info(buf2, XRP_BUFFER_HOST_POINTER_PTR,
			    &ptr, sizeof(ptr), &status);
	assert_1(status == XRP_STATUS_SUCCESS);
	status = -1;
	assert_1(ptr == &i);

	sprd_xrp_buffer_group_get_info(group, XRP_BUFFER_GROUP_BUFFER_FLAGS_ENUM, i,
				  &flags, sizeof(flags), &status);
	assert_1(status == XRP_STATUS_SUCCESS);
	status = -1;
	assert_1(flags == XRP_READ);
	sprd_xrp_buffer_group_get_info(group, XRP_BUFFER_GROUP_SIZE_SIZE_T, 0,
				  &sz, sizeof(sz), &status);
	assert_1(status == XRP_STATUS_SUCCESS);
	status = -1;
	assert_1(sz == 1);

	sprd_xrp_release_buffer(buf1);
	sprd_xrp_release_buffer(buf2);
	sprd_xrp_release_buffer_group(group);
	sprd_xrp_release_queue(queue);
	sprd_xrp_release_device(device);
}

/* Test default namespace */
static void f6(int devid)
{
	enum xrp_status status = -1;
	struct xrp_device *device;
	struct xrp_queue *queue;

	device = sprd_xrp_open_device(devid, &status);
	assert_1(status == XRP_STATUS_SUCCESS);
	status = -1;
	queue = sprd_xrp_create_queue(device, &status);
	assert_1(status == XRP_STATUS_SUCCESS);
	status = -1;
#ifdef USE_SPRD_MODE
	sprd_xrp_run_command_sync(queue, NULL, -1 , 0 , NULL , -1 , 0 , NULL , &status);
#else

	sprd_xrp_run_command_sync(queue,
			     NULL, 0,
			     NULL, 0,
			     NULL, &status);
#endif
	assert_1(status == XRP_STATUS_SUCCESS);
	status = -1;

	sprd_xrp_release_queue(queue);
	sprd_xrp_release_device(device);
}

/* Test command errors */
static void f7(int devid)
{
	enum xrp_status status = -1;
	struct xrp_device *device;
	struct xrp_queue *queue;
	struct example_v2_cmd cmd = {
		.cmd = EXAMPLE_V2_CMD_OK,
	};

	device = sprd_xrp_open_device(devid, &status);
	assert_1(status == XRP_STATUS_SUCCESS);
	status = -1;
	queue = sprd_xrp_create_ns_queue(device, XRP_EXAMPLE_V2_NSID, &status);
	assert_1(status == XRP_STATUS_SUCCESS);
	status = -1;
#ifdef USE_SPRD_MODE
	sprd_xrp_run_command_sync(queue , &cmd , -1 , sizeof(cmd) , NULL , -1 , 0 , NULL , &status);
#else
	sprd_xrp_run_command_sync(queue,
			     &cmd, sizeof(cmd),
			     NULL, 0,
			     NULL, &status);
#endif
	assert_1(status == XRP_STATUS_SUCCESS);
	status = -1;

	cmd.cmd = EXAMPLE_V2_CMD_FAIL;
#ifdef USE_SPRD_MODE
	sprd_xrp_run_command_sync(queue , &cmd , -1 , sizeof(cmd) , NULL , -1 , 0 , NULL , &status);
#else
	sprd_xrp_run_command_sync(queue,
			     &cmd, sizeof(cmd),
			     NULL, 0,
			     NULL, &status);
#endif
	assert_1(status == XRP_STATUS_FAILURE);
	status = -1;

	sprd_xrp_release_queue(queue);
	sprd_xrp_release_device(device);
}

/* Test priority queues */
static void f8(int devid)
{
	enum xrp_status status = -1;
	struct xrp_device *device;
	struct xrp_queue *queue0, *queue1;
	struct example_v2_cmd cmd = {
		.cmd = EXAMPLE_V2_CMD_LONG,
	};
	struct example_v2_rsp rsp;
	struct xrp_event *event;

	device = sprd_xrp_open_device(devid, &status);

	assert_1(status == XRP_STATUS_SUCCESS);
	status = -1;
	queue0 = sprd_xrp_create_nsp_queue(device, XRP_EXAMPLE_V2_NSID, 1, &status);
	assert_1(status == XRP_STATUS_SUCCESS);
	status = -1;
	queue1 = sprd_xrp_create_nsp_queue(device, XRP_EXAMPLE_V2_NSID, 2, &status);
	assert_1(status == XRP_STATUS_SUCCESS);
	status = -1;
#ifdef USE_SPRD_MODE
	sprd_xrp_enqueue_command(queue0,
                            &cmd, -1 , sizeof(cmd),
                            NULL, -1 , 0,
                            NULL, &event, &status);
#else
	sprd_xrp_enqueue_command(queue0,
			    &cmd, sizeof(cmd),
			    NULL, 0,
			    NULL, &event, &status);
#endif
	assert_1(status == XRP_STATUS_SUCCESS);
	status = -1;

	cmd.cmd = EXAMPLE_V2_CMD_SHORT;

	do {
#ifdef HAVE_NANOSLEEP
		struct timespec req;

		req.tv_sec = 0;
		req.tv_nsec = 100000000;
		/*
		 * This delay is here for the case of standalone host
		 * that runs much faster than the simulated DSP. It
		 * can send high priority requests so fast that it takes
		 * the low priority thread a very long time to get to a
		 * point where it's recognized by the high priority test
		 * function.
		 */
		nanosleep(&req, NULL);
#endif

#ifdef USE_SPRD_MODE
		sprd_xrp_run_command_sync(queue1,
                                     &cmd, -1 , sizeof(cmd),
                                     &rsp, -1 , sizeof(rsp),
                                     NULL, &status);
#else
		sprd_xrp_run_command_sync(queue1,
				     &cmd, sizeof(cmd),
				     &rsp, sizeof(rsp),
				     NULL, &status);
#endif
		assert_1(status == XRP_STATUS_SUCCESS);
		status = -1;
	} while (rsp.v != 1);

	sprd_xrp_wait(event, &status);
	assert_1(status == XRP_STATUS_SUCCESS);
	status = -1;

	sprd_xrp_release_event(event);
	sprd_xrp_release_queue(queue0);
	sprd_xrp_release_queue(queue1);
	sprd_xrp_release_device(device);
}

enum {
	CMD_TEST,

	CMD_N,
};
#ifdef USE_SPRD_MODE
static void f9(int devid , uint32_t inputsize , uint32_t outputsize , uint32_t buffnum)
{
	void *device = NULL;
	void *ionhandle = NULL;
	void *ionviraddr = NULL;
	void *inputhandle = NULL;
	void *outputhandle = NULL;
	void *bufferhandle = NULL;
	struct sprd_vdsp_inout buffer[10];
	struct sprd_vdsp_inout input, output;
	uint32_t buffersize = 1024;
	int32_t fd;
	int ret;
	device = sprd_vdsp_open_device(devid , SPRD_VDSP_WORK_NORMAL);
	if(device == NULL)
	{
		fprintf(stderr , "yzl add %s , sprd_vdsp_open_device NULL device\n" , __func__);
		return;
	}
	inputhandle = sprd_alloc_ionmem(inputsize , 1 , &fd , &ionviraddr);
	if(NULL == inputhandle) {
		fprintf(stderr , "yzl add %s , sprd_alloc_ionmem inputsize:%d NULL\n" , __func__ , inputsize);
		sprd_vdsp_release_device(device);
		return;
	}
	input.fd = fd;
	input.vir_addr = ionviraddr;
	input.size = inputsize;
	memset(ionviraddr , 0x11 , inputsize);
	sprd_flush_ionmem(inputhandle , ionviraddr, NULL , inputsize);
	outputhandle = sprd_alloc_ionmem(outputsize , 1 , &fd , &ionviraddr);
	if(NULL == outputhandle) {
		fprintf(stderr , "yzl add %s , sprd_alloc_ionmem outputsize:%d , NULL\n" , __func__ , outputsize);
		sprd_vdsp_release_device(device);
		sprd_free_ionmem(inputhandle);
		return;
	}
	output.fd = fd;
	output.vir_addr = ionviraddr;
	output.size = outputsize;
	memset(ionviraddr , 0x22 , outputsize);
	sprd_flush_ionmem(outputhandle , ionviraddr , NULL , outputsize);
	ionhandle = sprd_alloc_ionmem(buffersize, 1 , &fd , &ionviraddr);
	if(ionhandle == NULL)
	{
		fprintf(stderr , "yzl add %s , sprd_alloc_ionmem NULL\n" , __func__);
		sprd_vdsp_release_device(device);
		sprd_free_ionmem(inputhandle);
		sprd_free_ionmem(outputhandle);
		return;
	}
	memset(ionviraddr , 0x88 , buffersize);
	sprd_flush_ionmem(ionhandle , ionviraddr , NULL , buffersize);
	buffer[0].fd = fd;
	buffer[0].vir_addr = ionviraddr;
	buffer[0].size = buffersize;
	bufferhandle = ionhandle;
	ionhandle = sprd_alloc_ionmem(buffersize, 1 , &fd , &ionviraddr);
	if(ionhandle == NULL)
	{
		fprintf(stderr , "yzl add %s , sprd_alloc_ionmem  buffer 1 NULL\n" , __func__);
		sprd_vdsp_release_device(device);
		sprd_free_ionmem(inputhandle);
		sprd_free_ionmem(outputhandle);
		sprd_free_ionmem(bufferhandle);
	}
	memset(ionviraddr , 0x88 , buffersize);
	sprd_flush_ionmem(ionhandle , ionviraddr , NULL , buffersize);
	buffer[1].fd = fd;
	buffer[1].vir_addr = ionviraddr;
	buffer[1].size = buffersize;

#if 0
	sprd_flush_ionmem(inputhandle, input.vir_addr, NULL , input.size);
	sprd_flush_ionmem(outputhandle, output.vir_addr , NULL , output.size);
	sprd_flush_ionmem(ionhandle , buffer[0].vir_addr , NULL , buffer[0].size);
#endif
	fprintf(stderr , "yzl add %s , buffnum:%d , before vdsp processed input result:%x,%x,%x, output result:%x,%x,%x, buffer result:%x,%x,%x,%x,%x,%x,%x,%x,%x,%x , buffersize:%d, last buffele:%x\n" ,
                        __func__,
			buffnum,
			input.vir_addr[0],
			input.vir_addr[1],
			input.vir_addr[2],
			output.vir_addr[0],
                        output.vir_addr[1],
			output.vir_addr[2],
			buffer[0].vir_addr[0],
			buffer[0].vir_addr[1],
			buffer[0].vir_addr[2],
			buffer[0].vir_addr[3],
			buffer[0].vir_addr[4],
			buffer[0].vir_addr[5],
			buffer[0].vir_addr[6],
			buffer[0].vir_addr[7],
			buffer[0].vir_addr[8],
			buffer[0].vir_addr[9],
			buffersize,
			buffer[0].vir_addr[buffersize-1]);

	ret = sprd_vdsp_send_command(device,
				     (const char*)XRP_EXAMPLE_V1_NSID,
				     &input,
				     &output,
				     buffer,
				     buffnum,
				     0);
	fprintf(stderr, "yzl add %s , sprd_vdsp_send_command ret:%d\n",
		__func__, ret);
	fprintf(stderr , "yzl add %s , input result:%x,%x,%x, output result:%x,%x,%x, buffer result:%x,%x,%x,%x,%x,%x,%x,%x,%x,%x , buffersize:%d, last buffele:%x\n" ,
			__func__ ,
			input.vir_addr[0],
			input.vir_addr[1],
			input.vir_addr[2],
			output.vir_addr[0],
			output.vir_addr[1],
			output.vir_addr[2],
			buffer[0].vir_addr[0],
			buffer[0].vir_addr[1],
			buffer[0].vir_addr[2],
			buffer[0].vir_addr[3],
			buffer[0].vir_addr[4],
			buffer[0].vir_addr[5],
			buffer[0].vir_addr[6],
			buffer[0].vir_addr[7],
			buffer[0].vir_addr[8],
			buffer[0].vir_addr[9],
			buffersize,
			buffer[0].vir_addr[buffersize-1]);
#if 1
	{
		sprd_invalid_ionmem(ionhandle);
		sprd_invalid_ionmem(bufferhandle);
		sprd_invalid_ionmem(outputhandle);
		sprd_invalid_ionmem(inputhandle);
		//check output
		unsigned int i;
		for(i = 0; i < outputsize; i++)
		{
			if((output.vir_addr[i]) != 0x44)
			{
				fprintf(stderr , "yzl add %s , check output eror value:%x, golden:%x\n" , __func__ , (output.vir_addr[i]) , 0x44);
				break;
			}
		}
		if(i == outputsize)
		{
			fprintf(stderr , "yzl add %s , check output ok, size:%d\n" , __func__ , outputsize);
		}
		//check buffer
		for(i = 0; i < buffnum ; i++)
		{
			unsigned int j = 0;
			for(j=0; j < buffersize; j++)
			{
				if((buffer[i].vir_addr[j]) != 0x55)
				{
					fprintf(stderr , "yzl add %s , check buffer index:%d eror value:%x, golden:%x\n" , __func__ , i , (buffer[i].vir_addr[j]) , 0x55);
					break;
				}
			}
			if(j == buffersize)
			{
				fprintf(stderr , "yzl add %s , check buffer index:%d ok\n" , __func__ , i);
			}
		}
	}
#endif
	sprd_free_ionmem(ionhandle);
	sprd_free_ionmem(bufferhandle);
	sprd_free_ionmem(inputhandle);
	sprd_free_ionmem(outputhandle);
        sprd_vdsp_release_device(device);
}

static void f10(int devid)
{
        void *device = NULL;
        void *ionhandle = NULL;
        void *ionviraddr = NULL;
	void *ioninputhandle = NULL;
        struct sprd_vdsp_inout buffer[1];
	struct sprd_vdsp_inout input;
	struct stat statbuf;
        uint32_t buffersize = 8192;
        int32_t fd;
        int ret;
	FILE *fp = NULL;

	stat("/vendor/firmeware/test_lib.bin",&statbuf);
	//buffersize = statbuf.st_size;
	fprintf(stderr , "yzl add %s , file buffer size:%d\n",
		__func__ , buffersize);
        device = sprd_vdsp_open_device(devid , SPRD_VDSP_WORK_NORMAL);
        if(device == NULL)
        {
                fprintf(stderr , "yzl add %s , sprd_vdsp_open_device NULL device\n" , __func__);
                return;
        }
        ionhandle = sprd_alloc_ionmem(buffersize, 0 , &fd , &ionviraddr);
        if(ionhandle == NULL)
        {
                fprintf(stderr , "yzl add %s , sprd_alloc_ionmem NULL\n",
			__func__);
                sprd_vdsp_release_device(device);
                return;
        }
        buffer[0].fd = fd;
        buffer[0].vir_addr = ionviraddr;
        buffer[0].size = buffersize;
	fp = fopen("/vendor/firmware/test_lib.bin" , "rb");
	if(fp) {
		ret = fread(buffer[0].vir_addr , 1, buffersize , fp);
		fprintf(stderr , "yzl add %s , fwrite test_lib.bin buffersize:%d , size:%d\n" , __func__ ,buffersize , ret);
		fclose(fp);
	}
	else
	{
		fprintf(stderr , "yzl add %s , fopen test_lib.bin failed\n",
			__func__);
	}
	ioninputhandle = sprd_alloc_ionmem(100, 0 , &fd , &ionviraddr);
	if(ioninputhandle == NULL)
        {
                fprintf(stderr , "yzl add %s , sprd_alloc_ionmem ioninputhandle NULL\n" , __func__);
                sprd_vdsp_release_device(device);
		sprd_free_ionmem(ionhandle);
                return;
        }

	input.fd = fd;
	input.vir_addr = ionviraddr;
	input.size = 44;
	input.vir_addr[0] = 1; /*load flag*/
	sprintf(&input.vir_addr[1] , "%s" , "test_lib");
	ret = sprd_vdsp_send_command_directly(device,
					      (const char*)XRP_EXAMPLE_V3_NSID,
					      &input,
					      NULL,
					      buffer,
					      1,
					      0);
        fprintf(stderr , "yzl1234567890 add %s , sprd_vdsp_send_command_directly load test_lib 1 ret:%d\n" , __func__ , ret);

#if 0
	ret = sprd_vdsp_send_command_directly(device,
					      "test_lib",
					      &input,
					      NULL,
					      buffer,
					      1,
					      1);
#endif
	fprintf(stderr , "yzl1234567890 add %s , sprd_vdsp_send_command_directly to test_lib 4 ret:%d\n" , __func__ , ret);

	/*unload*/
	input.fd = fd;
	input.vir_addr = ionviraddr;
	input.size = 44;
	input.vir_addr[0] = 2; /*unload flag*/
	sprintf(&input.vir_addr[1] , "%s" , "test_lib");

	ret = sprd_vdsp_send_command_directly(device,
					      (const char*)XRP_EXAMPLE_V3_NSID,
					      &input,
					      NULL,
					      buffer,
					      1,
					      0);
	fprintf(stderr , "yzl1234567890 add %s , sprd_vdsp_send_command_directlyl to unload test_lib ret:%d\n" , __func__ , ret);

#if 0
        sprd_free_ionmem(ionhandle);

        sprd_vdsp_release_device(device);
#endif
}


static void test_load_unload(int devid , char *libname)
{
	void *device = NULL;
        void *ionhandle = NULL;
        void *ionviraddr = NULL;
        void *ioninputhandle = NULL;
        struct sprd_vdsp_inout buffer[1];
        struct sprd_vdsp_inout input;
        uint32_t buffersize = 8192;
        int32_t fd;
        int ret;
	char filename[128];
        FILE *fp = NULL;
        //buffersize = statbuf.st_size;
        fprintf(stderr , "yzl add %s , file buffer size:%d\n" , __func__ , buffersize);
        device = sprd_vdsp_open_device(devid , SPRD_VDSP_WORK_NORMAL);
        if(device == NULL)
        {
                fprintf(stderr , "yzl add %s , sprd_vdsp_open_device NULL device\n" , __func__);
                return;
        }
        ionhandle = sprd_alloc_ionmem(buffersize, 0 , &fd , &ionviraddr);
        if(ionhandle == NULL)
        {
                fprintf(stderr , "yzl add %s , sprd_alloc_ionmem NULL\n" , __func__);
                sprd_vdsp_release_device(device);
                return;
        }
        buffer[0].fd = fd;
        buffer[0].vir_addr = ionviraddr;
        buffer[0].size = buffersize;
	sprintf(filename , "/vendor/firmware/%s.bin" , libname);
        fp = fopen(filename , "rb");
        if(fp) {
                ret = fread(buffer[0].vir_addr , 1, buffersize , fp);
                fprintf(stderr , "yzl add %s , fwrite test_lib.bin buffersize:%d , size:%d\n" , __func__ ,buffersize , ret);
                fclose(fp);
        }
        else
        {
                fprintf(stderr , "yzl add %s , fopen test_lib.bin failed\n" , __func__);
        }
        ioninputhandle = sprd_alloc_ionmem(100, 0 , &fd , &ionviraddr);
        if(ioninputhandle == NULL)
        {
                fprintf(stderr , "yzl add %s , sprd_alloc_ionmem ioninputhandle NULL\n" , __func__);
                sprd_vdsp_release_device(device);
                sprd_free_ionmem(ionhandle);
                return;
        }
        input.fd = fd;
        input.vir_addr = ionviraddr;
        input.size = 44;
        input.vir_addr[0] = 1; /*load flag*/
        sprintf(&input.vir_addr[1] , "%s" , libname);
//        ret = sprd_vdsp_send_command(device , (const char*)XRP_EXAMPLE_V3_NSID , &input , NULL , buffer , 1 , 0);
        ret = sprd_vdsp_send_command_directly(device , (const char*)XRP_EXAMPLE_V3_NSID , &input, NULL,buffer, 1, 0);
        fprintf(stderr , "yzl add %s , sprd_vdsp_send_command_directly load test_lib ret:%d\n" , __func__ , ret);

        ret = sprd_vdsp_send_command_directly(device , libname , &input , NULL , buffer , 1 , 1);
        fprintf(stderr , "yzl add %s , sprd_vdsp_send_command_directly to test_lib ret:%d\n" , __func__ , ret);

        /*unload*/
        input.fd = fd;
        input.vir_addr = ionviraddr;
        input.size = 44;
        input.vir_addr[0] = 2; /*unload flag*/
        sprintf(&input.vir_addr[1] , "%s" , libname);

        ret = sprd_vdsp_send_command_directly(device , (const char*)XRP_EXAMPLE_V3_NSID , &input , NULL , buffer , 1 , 0);
        fprintf(stderr , "yzl add %s , sprd_vdsp_send_command_directlyl to unload test_lib ret:%d\n" , __func__ , ret);

        sprd_free_ionmem(ionhandle);
	sprd_free_ionmem(ioninputhandle);
        sprd_vdsp_release_device(device);
}
static void *thread_loadlibrary1(void *param)
{
	while(1) {
		//usleep(1000000);
		test_load_unload(0 , (char*)param);
	}
	return NULL;
}

static void f11()
{
	pthread_t a ,b;
        pthread_create(&a , NULL , thread_loadlibrary1 , "test_lib");
        pthread_create(&b , NULL , thread_loadlibrary1 , "test_lib");
	while(1)
		;//usleep(100000);
}
#endif
#ifdef HAVE_XTENSA_HAL_H
int _xt_atomic_compare_exchange_4(unsigned int *_ptr,
				  unsigned int _exp,
				  unsigned int _val)
{
	return xthal_compare_and_set((int32_t *)_ptr, _exp, _val);
}
#endif
static void f12()
{
	void *device = NULL;
        void *ionhandle = NULL;
	void *ionviraddr = NULL;
        void *ioninputhandle = NULL;
        struct sprd_vdsp_inout buffer[1];
        struct sprd_vdsp_inout input;
        uint32_t buffersize = 8192;
        int32_t fd;
        int ret;
        char filename[128];
        FILE *fp = NULL;
        //buffersize = statbuf.st_size;
        fprintf(stderr , "yzl add %s , file buffer size:%d\n" , __func__ , buffersize);
        device = sprd_vdsp_open_device(0 , SPRD_VDSP_WORK_NORMAL);
        if(device == NULL)
        {
                fprintf(stderr , "yzl add %s , sprd_vdsp_open_device NULL device\n" , __func__);
                return;
        }
        ionhandle = sprd_alloc_ionmem(buffersize, 0 , &fd , &ionviraddr);
        if(ionhandle == NULL)
        {
                fprintf(stderr , "yzl add %s , sprd_alloc_ionmem NULL\n" , __func__);
                sprd_vdsp_release_device(device);
                return;
        }
        buffer[0].fd = fd;
        buffer[0].vir_addr = ionviraddr;
        buffer[0].size = buffersize;
        sprintf(filename , "/vendor/firmware/%s.bin" , "test_lib");
        fp = fopen(filename , "rb");
        if(fp) {
                ret = fread(buffer[0].vir_addr , 1, buffersize , fp);
                fprintf(stderr , "yzl add %s , fwrite test_lib.bin buffersize:%d , size:%d\n" , __func__ ,buffersize , ret);
                fclose(fp);
        }
        else
        {
                fprintf(stderr , "yzl add %s , fopen test_lib.bin failed\n" , __func__);
        }
	ioninputhandle = sprd_alloc_ionmem(100, 0 , &fd , &ionviraddr);
        if(ioninputhandle == NULL)
        {
                fprintf(stderr , "yzl add %s , sprd_alloc_ionmem ioninputhandle NULL\n" , __func__);
                sprd_vdsp_release_device(device);
                sprd_free_ionmem(ionhandle);
                return;
        }
#if 0
        input.fd = fd;
        input.vir_addr = ionviraddr;
        input.size = 44;
        input.vir_addr[0] = 1; /*load flag*/
        sprintf(&input.vir_addr[1] , "%s" , "test_lib");
//        ret = sprd_vdsp_send_command(device , (const char*)XRP_EXAMPLE_V3_NSID , &input , NULL , buffer , 1 , 0);
	/*load test lib*/
        ret = sprd_vdsp_send_command_directly(device , (const char*)XRP_EXAMPLE_V3_NSID , &input, NULL,buffer, 1, 0);
        fprintf(stderr , "yzl add %s , sprd_vdsp_send_command_directly load test_lib ret:%d\n" , __func__ , ret);

	sprintf(filename , "/vendor/firmware/%s.bin" , "test_lib1");
	fp = fopen(filename , "rb");
	if(fp) {

		ret = fread(buffer[0].vir_addr , 1, buffersize , fp);
		fprintf(stderr , "yzl add %s , fwrite test_lib1.bin buffersize:%d , size:%d\n" , __func__ ,buffersize , ret);
		fclose(fp);
	}
	else {
		fprintf(stderr , "yzl add %s , fopen test_lib1.bin failed\n" , __func__);
	}
	input.fd = fd;
	input.vir_addr = ionviraddr;
	input.size = 44;
	input.vir_addr[0] = 1;
	sprintf(&input.vir_addr[1] , "%s" , "test_lib1");
	/*load test_lib1*/
	ret = sprd_vdsp_send_command_directly(device , (const char*)XRP_EXAMPLE_V3_NSID , &input, NULL,buffer, 1, 0);
	fprintf(stderr , "yzl add %s , sprd_vdsp_send_command_directly load test_lib	 ret:%d\n" , __func__ , ret);
#endif
	while(1) {
#if 0
	input.fd = fd;
        input.vir_addr = ionviraddr;
        input.size = 44;
        input.vir_addr[0] = 1; /*load flag*/
        sprintf(&input.vir_addr[1] , "%s" , "test_lib");
#endif
	sprintf(filename , "/vendor/firmware/%s.bin" , "test_lib");
        fp = fopen(filename , "rb");
        if(fp) {
                ret = fread(buffer[0].vir_addr , 1, buffersize , fp);
                fprintf(stderr , "yzl add %s , fwrite test_lib.bin buffersize:%d , size:%d\n" , __func__ ,buffersize , ret);
                fclose(fp);
        }
        else
        {
                fprintf(stderr , "yzl add %s , fopen test_lib.bin failed\n" , __func__);
        }	
//        ret = sprd_vdsp_send_command(device , (const char*)XRP_EXAMPLE_V3_NSID , &input , NULL , buffer , 1 , 0);
        /*load test lib*/
//        ret = sprd_vdsp_send_command_directly(device , (const char*)XRP_EXAMPLE_V3_NSID , &input, NULL,buffer, 1, 0);
	ret = sprd_vdsp_load_library(device , buffer , "test_lib" , 0);
	fprintf(stderr , "yzl add %s , sprd_vdsp_load_library load test_lib ret:%d\n" , __func__ , ret);
	input.fd = fd;
        input.vir_addr = ionviraddr;
        input.size = 44;
        input.vir_addr[0] = 1; /*load flag*/
        sprintf(&input.vir_addr[1] , "%s" , "test_lib1");

	sprintf(filename , "/vendor/firmware/%s.bin" , "test_lib1");
        fp = fopen(filename , "rb");
        if(fp) {

                ret = fread(buffer[0].vir_addr , 1, buffersize , fp);
                fprintf(stderr , "yzl add %s , fwrite test_lib1.bin buffersize:%d , size:%d\n" , __func__ ,buffersize , ret);
                fclose(fp);
        }
        else {
                fprintf(stderr , "yzl add %s , fopen test_lib1.bin failed\n" , __func__);
        }
//        ret = sprd_vdsp_send_command(device , (const char*)XRP_EXAMPLE_V3_NSID , &input , NULL , buffer , 1 , 0);
        /*load test lib*/
//        ret = sprd_vdsp_send_command_directly(device , (const char*)XRP_EXAMPLE_V3_NSID , &input, NULL,buffer, 1, 0);
	ret = sprd_vdsp_load_library(device , buffer , "test_lib1" , 0);
	fprintf(stderr , "yzl add %s , sprd_vdsp_load_library load test_lib1 ret:%d\n" , __func__ , ret);
	/*test send command*/
        ret = sprd_vdsp_send_command_directly(device , "test_lib" , &input , NULL , buffer , 1 , 1);
	fprintf(stderr , "yzl add %s , sprd_vdsp_send_command_directly to test_lib ret:%d\n" , __func__ , ret);
	ret = sprd_vdsp_send_command_directly(device , "test_lib1" , &input , NULL , buffer , 1 , 1);
        fprintf(stderr , "yzl add %s , sprd_vdsp_send_command_directly to test_lib1 ret:%d\n" , __func__ , ret);
	//usleep(1000*1000);
#if 0
	input.fd = fd;
        input.vir_addr = ionviraddr;
        input.size = 44;
        input.vir_addr[0] = 2; /*load flag*/
        sprintf(&input.vir_addr[1] , "%s" , "test_lib");
//        ret = sprd_vdsp_send_command(device , (const char*)XRP_EXAMPLE_V3_NSID , &input , NULL , buffer , 1 , 0);
        /*load test lib*/
#endif
//        ret = sprd_vdsp_send_command_directly(device , (const char*)XRP_EXAMPLE_V3_NSID , &input, NULL,buffer, 1, 0);
	ret = sprd_vdsp_unload_library(device , "test_lib" , 0);
	fprintf(stderr , "yzl add %s , sprd_vdsp_unload_library load test_lib ret:%d\n" , __func__ , ret);
#if 0
	input.fd = fd;
        input.vir_addr = ionviraddr;
        input.size = 44;
        input.vir_addr[0] = 2; /*load flag*/
        sprintf(&input.vir_addr[1] , "%s" , "test_lib1");
//        ret = sprd_vdsp_send_command(device , (const char*)XRP_EXAMPLE_V3_NSID , &input , NULL , buffer , 1 , 0);
#endif
        /*load test lib*/
        //ret = sprd_vdsp_send_command_directly(device , (const char*)XRP_EXAMPLE_V3_NSID , &input, NULL,buffer, 1, 0);
	ret = sprd_vdsp_unload_library(device , "test_lib1" , 0);
	fprintf(stderr , "yzl add %s , sprd_vdsp_unload_library load test_lib1 ret:%d\n" , __func__ , ret);
	}
	
#if 0
        /*unload*/
        input.fd = fd;
        input.vir_addr = ionviraddr;
        input.size = 44;
        input.vir_addr[0] = 2; /*unload flag*/
        sprintf(&input.vir_addr[1] , "%s" , libname);

        ret = sprd_vdsp_send_command_directly(device , (const char*)XRP_EXAMPLE_V3_NSID , &input , NULL , buffer , 1 , 0);
        fprintf(stderr , "yzl add %s , sprd_vdsp_send_command_directlyl to unload test_lib ret:%d\n" , __func__ , ret);
#endif
        sprd_free_ionmem(ionhandle);
        sprd_free_ionmem(ioninputhandle);
        sprd_vdsp_release_device(device);
}
static void f13(devid)
{
	void *device = NULL;
	void *ionhandle = NULL;//input image
	void *ionhandle2 = NULL;//out buffer
	void *ionviraddr = NULL;
	void *ionviraddr2 = NULL;
	unsigned long inphyaddr;
	FACEID_INFO *face_info;
	unsigned int out_result = 0;
	//void *ioninputhandle = NULL;
	struct sprd_vdsp_inout buffer[1];
	//struct sprd_vdsp_inout input;
	uint32_t buffersize = 960*720*3/2;
	int32_t fd,fd_out;
	int ret = 0;
	char filename[128];
	FILE *fp = NULL;
	//buffersize = statbuf.st_size;



	fprintf(stderr , "yzl add %s , file buffer size:%d\n" , __func__ , buffersize);
	device = sprd_vdsp_open_device(0 , SPRD_VDSP_WORK_FACEID);
	if(device == NULL)
	{
	        fprintf(stderr , "yzl add %s , sprd_vdsp_open_device NULL device\n" , __func__);
	        return;
	}

	ionhandle = sprd_alloc_ionmem2(buffersize, 0 , &fd , &ionviraddr, &inphyaddr);
	if(ionhandle == NULL)
	{
			fprintf(stderr , "yzl add %s , sprd_alloc_ionmem NULL\n" , __func__);
			sprd_vdsp_release_device(device);
			return;
	}
	buffer[0].fd = fd;
	buffer[0].vir_addr = ionviraddr;
	buffer[0].phy_addr = inphyaddr;
	buffer[0].size = buffersize;

	sprintf(filename , "/vendor/bin/test.yuv" );
    fp = fopen(filename , "rb");
    if(fp) {

            ret = fread(buffer[0].vir_addr , 1, buffersize , fp);
            fprintf(stderr , "yzl add %s , test.yuv buffersize:%d , size:%d\n" , __func__ ,buffersize , ret);
            fclose(fp);
    }
    else {
            fprintf(stderr , "yzl add %s , fopen test.yuv failed\n" , __func__);
    }

	ionhandle2 = sprd_alloc_ionmem(sizeof(FACEID_INFO), 0 , &fd_out , &ionviraddr2);
	if(ionhandle2 == NULL)
	{
			fprintf(stderr , "yzl add %s , sprd_alloc_ionmem NULL\n" , __func__);
			sprd_vdsp_release_device(device);
			sprd_free_ionmem(ionhandle2);
			return;
	}

	printf("out_fd %d\n",fd_out);
	int64_t start_time = systemTime(CLOCK_MONOTONIC);
	if (devid == 0)
		devid = 1;
	for(int i = 0;i<devid;i++)
	{
		//if (SPRD_XRP_STATUS_SUCCESS != sprd_vdsp_run_faceid_command_directly(device, inphyaddr, 720,960,1,&out_result,fd_out))
		//	fprintf(stderr , "xrp_run_faceid_command failed\n");
		//else
		{
			face_info = (FACEID_INFO *)ionviraddr2;
			fprintf(stderr ,"vdsp result %d,out addr %lX\n",out_result,face_info->facepoint_addr);
			fprintf(stderr ,"x %d y %d w %d h %d yaw %d pitch %d\n",face_info->x,face_info->y,face_info->width,face_info->height,face_info->yawAngle,face_info->pitchAngle);
		}
	}

	int64_t end_time = systemTime(CLOCK_MONOTONIC);
	int duration = (int)((end_time - start_time)/1000000);
	printf("run %d times take %d ms\n",devid,duration/devid);
	//fprintf(stderr , "wait\n");
	//sleep(30);

	sprd_vdsp_release_device(device);
	sprd_free_ionmem(ionhandle);
	sprd_free_ionmem(ionhandle2);
}

static void f14(devid)
{
	void *device = NULL;
	void *ionhandle = NULL;//input image
	void *ionhandle2 = NULL;//out buffer
	void *ionviraddr = NULL;
	void *ionviraddr2 = NULL;
	unsigned long inphyaddr;
	FACEID_INFO *face_info;
	unsigned int out_result = 0;
	//void *ioninputhandle = NULL;
	struct sprd_vdsp_inout output;
	struct sprd_vdsp_inout input;
	int w = 960, h = 720,liveness = 1;
	uint32_t buffersize = (w*h*3/2) + 12;//add width height liveness
	int32_t fd,fd_out;
	int ret = 0;
	char filename[128];
	FILE *fp = NULL;


	fprintf(stderr , "yzl add %s , file buffer size:%d\n" , __func__ , buffersize);
	device = sprd_vdsp_open_device(0 , SPRD_VDSP_WORK_FACEID);
	if(device == NULL)
	{
	        fprintf(stderr , "yzl add %s , sprd_vdsp_open_device NULL device\n" , __func__);
	        return;
	}

	ionhandle = sprd_alloc_ionmem2(buffersize, 0 , &fd , &ionviraddr, &inphyaddr);
	if(ionhandle == NULL)
	{
			fprintf(stderr , "yzl add %s , sprd_alloc_ionmem NULL\n" , __func__);
			sprd_vdsp_release_device(device);
			return;
	}
	input.fd = fd;
	input.vir_addr = ionviraddr;
	input.phy_addr = inphyaddr;
	input.size = buffersize;

	sprintf(filename , "/vendor/bin/test.yuv" );
    fp = fopen(filename , "rb");
    if(fp) {

            ret = fread(input.vir_addr , 1, w*h*3/2 , fp);
            fprintf(stderr , "yzl add %s , test.yuv , size:%d\n" , __func__ , ret);
            fclose(fp);
    }
    else {
            fprintf(stderr , "yzl add %s , fopen test.yuv failed\n" , __func__);
    }

	memcpy(input.vir_addr + w*h*3/2,&w,sizeof(int));
	memcpy(input.vir_addr + w*h*3/2 + 4,&h,sizeof(int));
	memcpy(input.vir_addr + w*h*3/2 + 8,&liveness,sizeof(int));


	ionhandle2 = sprd_alloc_ionmem(sizeof(FACEID_INFO), 0 , &fd_out , &ionviraddr2);
	if(ionhandle2 == NULL)
	{
			fprintf(stderr , "yzl add %s , sprd_alloc_ionmem NULL\n" , __func__);
			sprd_vdsp_release_device(device);
			sprd_free_ionmem(ionhandle2);
			return;
	}

	printf("out_fd %d\n",fd_out);
	output.fd = fd_out;
	output.vir_addr = ionviraddr2;
	output.phy_addr = 0;
	output.size = sizeof(FACEID_INFO);

	int64_t start_time = systemTime(CLOCK_MONOTONIC);
	if (devid == 0)
		devid = 1;
	for(int i = 0;i<devid;i++)
	{
		if (SPRD_XRP_STATUS_SUCCESS != sprd_vdsp_send_command_directly(device , FACEID_NSID , &input , &output , NULL , 1 , 1))
			fprintf(stderr , "xrp_run_faceid_command failed\n");
		else
		{
			face_info = (FACEID_INFO *)ionviraddr2;
			fprintf(stderr ,"vdsp result %d,out addr %lX\n",out_result,face_info->facepoint_addr);
			fprintf(stderr ,"x %d y %d w %d h %d yaw %d pitch %d\n",face_info->x,face_info->y,face_info->width,face_info->height,face_info->yawAngle,face_info->pitchAngle);
		}
	}

	int64_t end_time = systemTime(CLOCK_MONOTONIC);
	int duration = (int)((end_time - start_time)/1000000);
	printf("run %d times take %d ms\n",devid,duration/devid);
	//fprintf(stderr , "wait\n");
	//sleep(30);

	sprd_vdsp_release_device(device);
	sprd_free_ionmem(ionhandle);
	sprd_free_ionmem(ionhandle2);

}

int main(int argc, char **argv)
{
	int devid = 0;
	static const char * const cmd[CMD_N] = {
		[CMD_TEST] = "test",
	};
	int i = 0;

#ifdef HAVE_THREADS_XOS
	xos_set_clock_freq(XOS_CLOCK_FREQ);
	xos_start_main("main", 0, 0);
#endif
	if (argc > 1)
		sscanf(argv[1], "%i", &devid);
	if (argc > 2) {
		for (i = 0; i < CMD_N; ++i)
			if (strcmp(argv[2], cmd[i]) == 0)
				break;
		if (i == CMD_N) {
			fprintf(stderr, "%s: unrecognized command: %s\n", argv[0], argv[2]);
			return 1;
		}
	}
	switch(i) {
	case CMD_TEST:
		{
			unsigned long tests = -1;

			if (argc > 3)
				sscanf(argv[3], "%li", &tests);

			if (tests & 1) {
				f1(devid);
				printf("=======================================================\n");
			}
			if (tests & 2) {
				f2(devid);
				printf("=======================================================\n");
			}
			if (tests & 4) {
				f3(devid);
				printf("=======================================================\n");
			}
			if (tests & 8) {
				f4(devid);
				printf("=======================================================\n");
			}
			if (tests & 0x10) {
				f5(devid);
				printf("=======================================================\n");
			}
			if (tests & 0x20) {
				f6(devid);
				printf("=======================================================\n");
			}
			if (tests & 0x40) {
				f7(devid);
				printf("=======================================================\n");
			}
			if (tests & 0x80) {
				f8(devid);
				printf("=======================================================\n");
			}
#ifdef USE_SPRD_MODE
			if (tests & 0x100) {
				uint32_t insize = 0;
				uint32_t outsize = 0;
				uint32_t buffnum = 0;
				if(argc > 5) {
					insize = atoi(argv[4]);
					outsize = atoi(argv[5]);
					buffnum = atoi(argv[6]);
				}
				f9(devid , insize , outsize , buffnum);
				printf("=======================================================\n");
			}
			if(tests & 0x200) {
				f10(devid);
			}
			if(tests & 0x400) {
				f11();
			}
			if(tests & 0x800) {
				f14(devid);
			}
			if(tests & 0x2000) {
				f13(devid);
			}
			if(tests & 0x1000) {
				void *device;
				device = sprd_vdsp_open_device(0 , SPRD_VDSP_WORK_NORMAL);
				while(1)
					;//usleep(1000000);
			}
#endif
		}
		break;
	}
	return 0;
}
