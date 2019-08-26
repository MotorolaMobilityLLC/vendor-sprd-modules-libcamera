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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <cutils/log.h>
#include <semaphore.h>
#include <linux/ion.h>
#include "sprd_ion.h"
#include <linux/types.h>
#include <asm/ioctl.h>
#include "MemIon.h"
#include "sprd_hdr_api.h"
#include <cutils/properties.h>
#include "cmr_ipm.h"
#include "cmr_types.h"

using namespace android;

#define ERR(x...) fprintf(stderr, x)
#define INFO(x...) fprintf(stdout, x)
#define HDR_NEED_FRAME_NUM 2
#define IMAGE_FORMAT "YVU420_SEMIPLANAR"
static char src1_yuv_420_file[] = "/vendor/pic/src0.yuv";
static char src2_yuv_420_file[] = "/vendor/pic/src1.yuv";
static char src3_yuv_420_file[] = "/vendor/pic/src1_yuv_420.raw";
static char dst_y_420_file[] = "/vendor/pic/dst_y_420.raw";
static char dst_uv_420_file[] = "/vendor/pic/dst_uv_420.raw";


typedef struct vdsp_memory {
    MemIon *ion_heap;
    unsigned long phys_addr;
    size_t phys_size;
    cmr_s32 fd;
    cmr_s32 dev_fd;
    void *handle;
    void *data;
    bool busy_flag;
} vdsp_memory_t;

struct lib_context {
    hdr_inst_t lib_handle;
    float ev[HDR_CAP_NUM];
    struct ipm_version version;
};

struct utest_hdr_cxt {
	cmr_uint width;
	cmr_uint height;
	cmr_u8 *alloc_addr[HDR_CAP_NUM];
	cmr_uint fd[HDR_CAP_NUM];
	vdsp_memory_t *in_p1_mem;
	vdsp_memory_t *in_p2_mem;
	vdsp_memory_t *in_p3_mem;
	vdsp_memory_t *out_y_mem;
	vdsp_memory_t *out_uv_mem;
	struct lib_context lib_cxt;
};

static vdsp_memory_t *allocMem(
	int buf_size, int num_bufs, uint32_t is_cache)
{
	size_t mem_size = 0;
	MemIon *pHeapIon = NULL;
	unsigned long paddr = 0;
	UNUSED(is_cache);

	vdsp_memory_t *memory =
	    (vdsp_memory_t *)malloc(sizeof(vdsp_memory_t));
	if (NULL == memory) {
		goto getpmem_fail;
	}
	memset(memory, 0, sizeof(vdsp_memory_t));
	memory->busy_flag = false;

	mem_size = buf_size * num_bufs;
	mem_size = (mem_size + 4095U) & (~4095U);

	pHeapIon = new MemIon("/dev/ion", mem_size, MemIon::NO_CACHING, ION_HEAP_ID_MASK_MM);
	if (pHeapIon == NULL || pHeapIon->getHeapID() < 0) {
		ERR("vdsp_trace failed to alloc ion pmem buffer1.\n");
		goto getpmem_fail;
	}
    	pHeapIon->get_phy_addr_from_ion(&paddr, &mem_size);

	if (NULL == pHeapIon->getBase() || MAP_FAILED == pHeapIon->getBase()) {
		ERR("vdsp_trace failed to alloc ion pmem buffer2.\n");
		goto getpmem_fail;
	}

	memory->ion_heap = pHeapIon;
	memory->fd = pHeapIon->getHeapID();
	memory->dev_fd = pHeapIon->getIonDeviceFd();
	memory->phys_addr = paddr;
	memory->phys_size = mem_size;
	memory->data = pHeapIon->getBase();

	INFO("vdsp_trace alloc success fd=0x%x, phys_addr=0x%lx, virt_addr=%p, size=0x%lx, heap=%p\n",
              memory->fd, memory->phys_addr,
              memory->data, memory->phys_size, pHeapIon);

	return memory;

getpmem_fail:
	if (pHeapIon)
		delete pHeapIon;

	if (memory != NULL) {
		free(memory);
		memory = NULL;
	}
	return NULL;
}

static void freeMem(vdsp_memory_t *memory)
{
	if (!memory)
		return;

	INFO("vdsp_trace free memory %p,  fd %d, ptr %p\n",
			memory, memory->fd, memory->data);
	if (memory->ion_heap) {
		delete memory->ion_heap;
		memory->ion_heap = NULL;
	}
	free(memory);
}
static int utest_hdr_alloc_mem(struct utest_hdr_cxt *hdr_cxt)
{
	ERR("vdsp_trace alloc mem.\n");
	/* alloc input pic1 buffer */
	hdr_cxt->in_p1_mem =
	allocMem(hdr_cxt->width * hdr_cxt->height*3/2, 1, false);
    INFO("vdsp_trace LIKE:src pic1 phy addr :0x%lx, virtual addr:%p\n",
         hdr_cxt->in_p1_mem->phys_addr,
         hdr_cxt->in_p1_mem->data);
    memset(hdr_cxt->in_p1_mem->data, 0x80,
           hdr_cxt->in_p1_mem->phys_size);
	hdr_cxt->alloc_addr[0] =
		(cmr_u8 *)hdr_cxt->in_p1_mem->data;
	hdr_cxt->fd[0] = hdr_cxt->in_p1_mem->fd;

	/* alloc input pic2 buffer */
	hdr_cxt->in_p2_mem =
	allocMem(hdr_cxt->width * hdr_cxt->height*3/2, 1, false);
    INFO("vdsp_trace LIKE:src pic2 phy addr :0x%lx, virtual addr:%p\n",
         hdr_cxt->in_p2_mem->phys_addr,
         hdr_cxt->in_p2_mem->data);
    memset(hdr_cxt->in_p2_mem->data, 0x80,
           hdr_cxt->in_p2_mem->phys_size);
	hdr_cxt->alloc_addr[1] =
		(cmr_u8 *)hdr_cxt->in_p2_mem->data;
	hdr_cxt->fd[1] = hdr_cxt->in_p2_mem->fd;

	/* alloc input pic3 buffer */
	hdr_cxt->in_p3_mem =
	allocMem(hdr_cxt->width * hdr_cxt->height*3/2, 1, false);
    INFO("vdsp_trace LIKE:src pic3 phy addr :0x%lx, virtual addr:%p\n",
         hdr_cxt->in_p3_mem->phys_addr,
         hdr_cxt->in_p3_mem->data);
    memset(hdr_cxt->in_p3_mem->data, 0x80,
           hdr_cxt->in_p3_mem->phys_size);
	hdr_cxt->alloc_addr[2] =
		(cmr_u8 *)hdr_cxt->in_p3_mem->data;
	hdr_cxt->fd[2] = hdr_cxt->in_p3_mem->fd;

	/* alloc output pic buffer */
	hdr_cxt->out_y_mem =
	allocMem(hdr_cxt->width * hdr_cxt->height, 1, false);
    INFO("vdsp_trace LIKE:out pic y phy addr :0x%lx, virtual addr:%p\n",
         hdr_cxt->out_y_mem->phys_addr,
         hdr_cxt->out_y_mem->data);
    memset(hdr_cxt->out_y_mem->data, 0x80,
           hdr_cxt->out_y_mem->phys_size);
	//out uv
	hdr_cxt->out_uv_mem =
	allocMem(hdr_cxt->width * hdr_cxt->height/2, 1, false);
    INFO("vdsp_trace LIKE:out pic uv phy addr :0x%lx, virtual addr:%p\n",
         hdr_cxt->out_uv_mem->phys_addr,
         hdr_cxt->out_uv_mem->data);
    memset(hdr_cxt->out_uv_mem->data, 0x80,
           hdr_cxt->out_uv_mem->phys_size);
	ERR("vdsp_trace alloc mem successed.\n");
	return 0;
}
static void utest_hdr_free_mem(struct utest_hdr_cxt *hdr_cxt)
{
	ERR("vdsp_trace free mem.\n");
	freeMem(hdr_cxt->in_p1_mem);
	freeMem(hdr_cxt->in_p2_mem);
	freeMem(hdr_cxt->in_p3_mem);
	freeMem(hdr_cxt->out_y_mem);
	freeMem(hdr_cxt->out_uv_mem);
}
static int utest_hdr_set_src_data(struct utest_hdr_cxt *hdr_cxt) 
{
  FILE *fp = 0;

    ERR("vdsp_trace set src data start.\n");
	//src1
    fp = fopen(src1_yuv_420_file, "r");
    if (fp != NULL) {
        fread((void *)hdr_cxt->in_p1_mem->data, 1,hdr_cxt->width *hdr_cxt->height*3/2, fp);
        fclose(fp);
    } else {
        ERR("vdsp_trace load src1 file failed.\n");
        return -1;
    }
	//src2
    fp = fopen(src2_yuv_420_file, "r");
    if (fp != NULL) {
        fread((void *)hdr_cxt->in_p2_mem->data, 1,hdr_cxt->width *hdr_cxt->height*3/2, fp);
        fclose(fp);
    } else {
        ERR("vdsp_trace load src2 file failed.\n");
        return -1;
    }
	//src3
    fp = fopen(src3_yuv_420_file, "r");
    if (fp != NULL) {
        fread((void *)hdr_cxt->in_p3_mem->data, 1,hdr_cxt->width *hdr_cxt->height*3/2, fp);
        fclose(fp);
    } else {
        ERR("vdsp_trace load src3 file failed.\n");
        return -1;
    }
    ERR("vdsp_trace set src data successed.\n");
    return 0;
}

static int utest_hdr_save_dst_data(struct utest_hdr_cxt *hdr_cxt) 
{
 FILE *fp = 0;

    ERR("vdsp_trace save dst data entry.\n");
    fp = fopen(dst_y_420_file, "wb");
    if (fp != NULL) {
        fwrite((void *)hdr_cxt->out_y_mem->data, 1,hdr_cxt->width *hdr_cxt->height, fp);
        fclose(fp);
    } else {
        ERR("vdsp_trace save dst y file failed.\n");
        return -1;
    }

    fp = fopen(dst_uv_420_file, "wb");
    if (fp != NULL) {
        fwrite((void *)hdr_cxt->out_uv_mem->data, 1,hdr_cxt->width *hdr_cxt->height/2, fp);
        fclose(fp);
    } else {
        ERR("vdsp_trace save dst uv file failed.\n");
        return -1;
    }

	ERR("vdsp_trace save dst data successed.\n");
	return 0;
}

static int utest_hdr_paser_param(struct utest_hdr_cxt *hdr_cxt, int argc, char **argv)
{
	int i = 0;
	    memset(hdr_cxt, 0, sizeof(struct utest_hdr_cxt));

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-iw") == 0 && (i < argc - 1)) {
            hdr_cxt->width = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-ih") == 0 && (i < argc - 1)) {
			hdr_cxt->height = atoi(argv[++i]);
        } else {
			ERR("vdsp_trace failed to paser param\n");
			return -1;
        }
    }
	return 0;
}
static int utest_hdr_open(struct utest_hdr_cxt *hdr_cxt){
	cmr_int ret = CMR_CAMERA_SUCCESS;
	hdr_version_t lib_version;
	hdr_config_t cfg;
	ERR("vdsp_trace hdr open entry.\n");
	memset(&lib_version, 0, sizeof(hdr_version_t));
	memset(&cfg, 0, sizeof(hdr_config_t));
	 if (!sprd_hdr_version(&lib_version)) {
        hdr_cxt->lib_cxt.version.major = lib_version.major;
        hdr_cxt->lib_cxt.version.minor = lib_version.minor;
        hdr_cxt->lib_cxt.version.micro = lib_version.micro;
        hdr_cxt->lib_cxt.version.nano = lib_version.nano;
        INFO("vdsp_trace major:%d minor:%d micro:%d nano:%d \n", lib_version.major,
                 lib_version.minor, lib_version.micro, lib_version.nano);
        INFO("vdsp_trace build date:%s build time:%s build rev:%s \n",
                 lib_version.built_date, lib_version.built_time,
                 lib_version.built_rev);
	ERR("vdsp_trace interface sprd_hdr_version successed====.\n");
    } else {
        ERR("vdsp_trace failed to get verion!\n");
    }
	ret = sprd_hdr_config_default(&cfg);
	if (!ret) {
		ERR("vdsp_trace interface SPRD_HDR_CONFIG_DEFAULT successed !\n");
	        cfg.img_width = hdr_cxt->width;
	        cfg.img_height = hdr_cxt->height;
	        cfg.img_stride = hdr_cxt->width;
	        cfg.img_num = HDR_NEED_FRAME_NUM;
	        cfg.max_width = hdr_cxt->width;
	        cfg.max_height = hdr_cxt->height;
	        cfg.core_str = "_4_0_1_2_3";
	        ret = sprd_hdr_vdsp_open(&hdr_cxt->lib_cxt.lib_handle, &cfg);
        if (ret) {
            ERR("vdsp_trace failed to open lib!\n");
            goto exit;
        } else {
        	ERR("vdsp_trace interface SPRD_HDR_OPEN successed \n");
        }
    } else {
        ERR("vdsp_trace failed to get cfg!\n");
        goto exit;
    }
exit:
    INFO("vdsp_trace done %ld\n", ret);
    return ret;	
	
}

static int utest_hdr_close(struct utest_hdr_cxt *hdr_cxt){
	cmr_int ret = CMR_CAMERA_SUCCESS;
	cmr_int i;

	ERR("vdsp_trace close entry\n");
	ret = sprd_hdr_fast_stop(hdr_cxt->lib_cxt.lib_handle);
	if (ret) {
		ERR("vdsp_trace HDR fast stop failed.\n");
	}
	ERR("vdsp_trace interface SPRD_HDR_FAST_STOP successed\n");
	ret = sprd_hdr_vdsp_close(hdr_cxt->lib_cxt.lib_handle);
	if (ret) {
		ERR("vdsp_trace HDR close failed.\n");
	}
	ERR("vdsp_trace interface SPRD_HDR_CLOSE successed\n");
	INFO("vdsp_trace X\n");
    return ret;
}

static int utest_hdr_arithmetic(struct utest_hdr_cxt *hdr_cxt){
	cmr_int ret = CMR_CAMERA_SUCCESS;
	cmr_u32 size = hdr_cxt->width * hdr_cxt->height;
	cmr_u8 *temp_addr0 = NULL;
	cmr_u8 *temp_addr1 = NULL;
	cmr_u8 *temp_addr2 = NULL;
	ldr_image_vdsp_t input_img[HDR_NEED_FRAME_NUM];
	ldr_image_vdsp_t out_img;
	const char *p_format = IMAGE_FORMAT;

	temp_addr0 = hdr_cxt->alloc_addr[0];
	temp_addr1 = hdr_cxt->alloc_addr[1];
	temp_addr2 = hdr_cxt->alloc_addr[2];
	ERR("vdsp_trace utest_hdr_arithmetic entry \n");
	if ((NULL != temp_addr0) && (NULL != temp_addr1) && (NULL != temp_addr2)) {
		input_img[0].image.data = hdr_cxt->alloc_addr[0];
		input_img[0].fd = hdr_cxt->fd[0];
		input_img[0].image.ev = -1;
		input_img[0].image.width = hdr_cxt->width;
		input_img[0].image.height = hdr_cxt->height;
		input_img[0].image.stride = hdr_cxt->width;
		input_img[1].image.data = hdr_cxt->alloc_addr[1];
		input_img[1].fd = hdr_cxt->fd[1];
		input_img[1].image.ev = 1;
		input_img[1].image.width = hdr_cxt->width;
		input_img[1].image.height = hdr_cxt->height;
		input_img[1].image.stride = hdr_cxt->width;
		out_img.image.data = hdr_cxt->alloc_addr[2];
		out_img.fd = hdr_cxt->fd[2];
		out_img.image.width = hdr_cxt->width;
		out_img.image.height = hdr_cxt->height;
		out_img.image.stride = hdr_cxt->width;
		INFO("addr: 0x%lx, 0x%lx, ev: %f, %f \n", input_img[0].image.data,
		input_img[1].image.data, input_img[0].image.ev, input_img[1].image.ev);
		ret = sprd_hdr_vdsp_process(hdr_cxt->lib_cxt.lib_handle, &input_img[0],
		   &out_img);
		if (ret) {
		ERR("vdsp_trace faild to hdr process\n");
		}
		ERR("vdsp_trace interface SPRD_HDR_PROCESS successed\n");
	} else {
		ERR("vdsp_trace can't handle hdr. \n");
		ret = CMR_CAMERA_FAIL;
	}
	if (NULL != temp_addr2) {
		memcpy((void *)hdr_cxt->out_y_mem->data, (void *)temp_addr2, size);
		memcpy((void *)hdr_cxt->out_uv_mem->data, (void *)(temp_addr2 + size), size / 2);
	}
	if (CMR_CAMERA_SUCCESS == ret) {
	INFO("vdsp_trace hdr done. \n");
	}

	return ret;
}

int main(int argc, char **argv) {
	int i = 0, ret = -1;
    int64_t time_start = 0, time_end = 0;
   
    struct utest_hdr_cxt utest_hdr_cxt;
    struct utest_hdr_cxt *hdr_cxt_ptr = &utest_hdr_cxt;

	ERR("vdsp_trace start==============.\n");
	if (utest_hdr_paser_param(hdr_cxt_ptr, argc, argv)) {
	ERR("get invild hdr param.\n");
	return ret;
	}
	ERR("vdsp_trace paser param finished.\n");
	if (utest_hdr_alloc_mem(hdr_cxt_ptr)) {
	ERR("failed to alloc hdr memory.\n");
	goto err;
	}
	ERR("vdsp_trace hdr alloc mem successed.\n");
	if (utest_hdr_set_src_data(hdr_cxt_ptr)) {
	ERR("failed to set src data.\n");
	goto err;
	}
	ERR("vdsp_trace set src data successed.\n");
	if (utest_hdr_open(hdr_cxt_ptr)) {
	ERR("failed to open hdr \n");
	goto err;
	}
	ERR("vdsp_trace hdr open successed.\n");
	if (utest_hdr_arithmetic(hdr_cxt_ptr)) {
		ERR("vdsp_trace failed to process hdr\n");
		goto err;
	}
	ERR("vdsp_trace hdr process successed.\n");
	//if (utest_hdr_close(hdr_cxt_ptr)) {
	//	ERR("vdsp_trace failed to close hdr");
	//	goto err;
	//}
  	ERR("vdsp_trace hdr close successed.\n");
	if (utest_hdr_save_dst_data(hdr_cxt_ptr)) {
		ERR("vdsp_trace to save dst data\n");
		goto err;
	}
	ERR("vdsp_trace save dst data successed\n");

	
err:
	utest_hdr_free_mem(hdr_cxt_ptr);
	ERR("vdsp_trace vdsp test finished\n");
    return 0;
}

