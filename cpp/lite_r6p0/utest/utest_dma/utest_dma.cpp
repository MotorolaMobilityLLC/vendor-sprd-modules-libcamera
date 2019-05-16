/*
 * Copyright (C) 2008 The Android Open Source Project
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
#include "sprd_ion.h"

#include "sprd_cpp.h"
#include "MemIon.h"
extern "C" {
	#include "cpp_u_dev.h"
}
#include "cmr_type.h"
#include "cmr_common.h"

using namespace android;

#define ERR(x...) fprintf(stderr, x)
#define INFO(x...) fprintf(stdout, x)
#define UTEST_DMA_COUNTER 1

const char* src_file_name = "/root/data/like/pic/src_y_422.raw";
const char* dma_output_name = "/root/data/like/pic/dma_out.dat";

struct utest_dma_cxt {
	MemIon *input_pmem_hp;
	size_t input_pmem_size;
	unsigned long input_phy_addr;
	unsigned char *input_vir_addr;

	MemIon *output_pmem_hp;
	size_t output_pmem_size;
	unsigned long output_phy_addr;
	unsigned char *output_vir_addr;

	struct sprd_cpp_dma_cfg_parm dma_cfg;
};

static void usage(void) 
{
	INFO("Usage:\n");
	INFO("utest_dma -inum 100(total num)\n");
}

static int utest_dma_param_set(struct utest_dma_cxt *dma_cxt_ptr, int argc,char **argv) 
{
	int i = 0;
	if (argc < 3) {
		usage();
		return -1;
	}
	memset(dma_cxt_ptr, 0, sizeof(struct utest_dma_cxt));

	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-inum") == 0 && (i < argc - 1)) {
			dma_cxt_ptr->dma_cfg.total_num = atoi(argv[i+1]);
			INFO("Dma Copy %d Bytes\n",dma_cxt_ptr->dma_cfg.total_num);
			i++;
		}else {
			usage();
			return -1;
		}
	}
	return 0;
}

static int utest_dma_mem_alloc(struct utest_dma_cxt *dma_cxt_ptr) 
{
	dma_cxt_ptr->input_pmem_hp = new MemIon("/dev/ion",
		dma_cxt_ptr->dma_cfg.total_num,
		MemIon::NO_CACHING, ION_HEAP_ID_MASK_MM);

	if (dma_cxt_ptr->input_pmem_hp->getHeapID() < 0) {
		ERR("failed to alloc input_pmem buffer.\n");
		return -1;
	}

	dma_cxt_ptr->input_pmem_hp->get_phy_addr_from_ion(
		&dma_cxt_ptr->input_phy_addr,&dma_cxt_ptr->input_pmem_size);

	dma_cxt_ptr->input_vir_addr =
		(unsigned char *)dma_cxt_ptr->input_pmem_hp->getBase();

	ERR("DMA:src: phy addr:0x%lx,vir addr:%p\n",
		dma_cxt_ptr->input_phy_addr,dma_cxt_ptr->input_vir_addr);

	if (!dma_cxt_ptr->input_phy_addr) {
		ERR("failed to alloc src pmem buffer:addr is null.\n");
		return -1;
	}
	memset(dma_cxt_ptr->input_vir_addr, 0x80,dma_cxt_ptr->dma_cfg.total_num);
	dma_cxt_ptr->dma_cfg.input_addr.y= dma_cxt_ptr->input_phy_addr;

	dma_cxt_ptr->output_pmem_hp = new MemIon("/dev/ion",
		dma_cxt_ptr->dma_cfg.total_num,
		MemIon::NO_CACHING, ION_HEAP_ID_MASK_MM);

	if (dma_cxt_ptr->output_pmem_hp->getHeapID() < 0) {
		ERR("failed to alloc output pmem buffer.\n");
		return -1;
	}

	dma_cxt_ptr->output_pmem_hp->get_phy_addr_from_ion(
		&dma_cxt_ptr->output_phy_addr,&dma_cxt_ptr->output_pmem_size);

	dma_cxt_ptr->output_vir_addr =
		(unsigned char *)dma_cxt_ptr->output_pmem_hp->getBase();

	ERR("DMA:dst  :phy addr:0x%lx,vir addr:%p\n",
		dma_cxt_ptr->output_phy_addr,dma_cxt_ptr->output_vir_addr);

	if (!dma_cxt_ptr->output_phy_addr) {
		ERR("failed to alloc output pmem buffer:addr is null.\n");
		return -1;
	}
	
	dma_cxt_ptr->dma_cfg.output_addr.y = dma_cxt_ptr->output_phy_addr;
	return 0;
}

static int utest_dma_set_src_data(struct utest_dma_cxt *dma_cxt_ptr)
{
	FILE *fp = 0;
	fp = fopen(src_file_name, "r");
	if (fp != NULL) {
		fread((void *)dma_cxt_ptr->input_vir_addr, 1,
			dma_cxt_ptr->dma_cfg.total_num,fp);
		fclose(fp);
	} else {
		ERR("dma set data fail : no input source file.\n");
		return -1;
	}
	return 0;
}

static int utest_dma_set_des_data(struct utest_dma_cxt *dma_cxt_ptr,
	int count)
{
	FILE *fp = 0;
	char file_name[128] = "";
	sprintf(file_name, "%s_%d",dma_output_name,count);
	fp = fopen(file_name, "wb");
	if (fp != NULL) {
		fwrite((void *)dma_cxt_ptr->output_vir_addr, 1,
			dma_cxt_ptr->dma_cfg.total_num, fp);
		fclose(fp);
	} else {
		ERR("Dma: failed to open save_file.\n");
		return -1;
	}
	return 0;
}

static int utest_dma_mem_release(struct utest_dma_cxt *dma_cxt_ptr)
{
	ERR("utest_dma_mem_release.\n");
	delete dma_cxt_ptr->input_pmem_hp;
	delete dma_cxt_ptr->output_pmem_hp;
	return 0;
}

int main(int argc, char **argv) {
	int i = 0, ret = -1;
	int64_t time_start = 0, time_end = 0;
	static struct utest_dma_cxt utest_dma_cxt;
	struct utest_dma_cxt *dma_cxt_ptr = &utest_dma_cxt;
	struct cpp_dma_param dma_param;
	cmr_handle dma_handle;

	dma_param.host_fd = -1;
	dma_param.handle = NULL;
	dma_param.dma_cfg_param = &dma_cxt_ptr->dma_cfg;

	if (utest_dma_param_set(dma_cxt_ptr, argc, argv)) {
		ERR("DMA error: invild cfg parameters \n");
		ret = CMR_CAMERA_INVALID_PARAM;
		return ret;
	}

	if (utest_dma_mem_alloc(dma_cxt_ptr)) {
		ERR("DMA error: no mem alloced for src des data \n");
		ret = CMR_CAMERA_NO_MEM;
		goto err;
	}

	if (utest_dma_set_src_data(dma_cxt_ptr)) {
		ERR("dma error: failed to set src data\n");
		ret = CMR_CAMERA_INVALID_PARAM;
		goto err;
	}

	ERR("1debug %d \n", *(int *)dma_param.handle);
	for (i = 0; i < UTEST_DMA_COUNTER; i++) {
		if (cpp_dma_open(&dma_handle) != 0) {
			ERR("failed to open dma drv.\n");
			goto err;
		}
		dma_param.handle = dma_handle;
		ERR("2debug %d \n", *(int *)dma_param.handle);
		time_start = systemTime();
		if (cpp_dma_start(&dma_param) != 0) {
			ERR("failed to start dma drv.\n");
			goto err;
		}
		time_end = systemTime();
		ERR("DMA testing  end cost time=%d\n",(unsigned int)((time_end - time_start) / 1000000L));
		usleep(30*1000);
		if (utest_dma_set_des_data(dma_cxt_ptr, i))
			goto err;
		if (cpp_dma_close(dma_param.handle) != 0) {
			ERR("failed to close dma drv.\n");
			goto err;
		}
	}

err:
	utest_dma_mem_release(dma_cxt_ptr);
	return ret;
}
