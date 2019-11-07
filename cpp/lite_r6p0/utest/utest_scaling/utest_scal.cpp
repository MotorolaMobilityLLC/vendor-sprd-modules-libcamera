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
//#include "cmr_common.h"
#include <semaphore.h>
#include <ion.h>
#include "sprd_ion.h"
#include <linux/types.h>
#include <asm/ioctl.h>
#include "sprd_cpp.h"
#include "MemIon.h"

extern "C" {
	#include "cpp_u_dev.h"
}

using namespace android;

#define UTEST_SCALING_COUNTER 1
#define ERR(x...) fprintf(stderr, x)
#define INFO(x...) fprintf(stdout, x)

#define TEST_ON_HAPS

typedef struct cpp_memory {
	MemIon *ion_heap;
	unsigned long phys_addr;
	size_t phys_size;
	cmr_s32 fd;
	cmr_s32 dev_fd;
	void *handle;
	void *data;
	bool busy_flag;
} cpp_memory_t;

struct utest_scal_cxt {
	cpp_memory_t *input_y_mem;
	cpp_memory_t *input_uv_mem;
	cpp_memory_t *output_sc_y_mem;
	cpp_memory_t *output_sc_uv_mem;
	cpp_memory_t *output_bp_y_mem;
	cpp_memory_t *output_bp_uv_mem;

	sprd_cpp_scale_cfg_parm scal_cfg;
};

static char u_test_cfg_file[] = "/data/vendor/cameraserver/data/like/cfg_file.txt";
static char utest_scal_src_y_422_file[] = "/data/vendor/cameraserver/data/like/pic/src_y_422.raw";
static char utest_scal_src_uv_422_file[] = "/data/vendor/cameraserver/data/like/pic/src_uv_422.raw";
static char utest_scal_src_y_420_file[] = "/data/vendor/cameraserver/data/like/pic/src_y_420.raw";
static char utest_scal_src_uv_420_file[] = "/data/vendor/cameraserver/data/like/pic/src_uv_420.raw";

static char utest_scal_sc_dst_y_file[] =
    "/data/vendor/cameraserver/data/like/pic/utest_scal_sc_dst_y_%dx%d_format_%d_%d_%d.raw";
static char utest_scal_sc_dst_uv_file[] =
    "/data/vendor/cameraserver/data/like/pic/utest_scal_sc_dst_uv_%dx%d_format_%d_%d_%d.raw";
static char utest_scal_bp_dst_y_file[] =
    "/data/vendor/cameraserver/data/like/pic/utest_scal_bp_dst_y_%dx%d_format_%d_%d_%d.raw";
static char utest_scal_bp_dst_uv_file[] =
    "/data/vendor/cameraserver/data/like/pic/utest_scal_bp_dst_uv_%dx%d_format_%d_%d_%d.raw";

const char *cfg_string[] = {
	"input_size_w",
	"input_size_h",
	"input_rect_x",
	"input_rect_y",
	"input_rect_w",
	"input_rect_h",
	"input_format",
	"intput_endian_y",
	"input_endian_uv",
	"sc_trim_x",
	"sc_trim_y",
	"sc_trim_w",
	"sc_trim_h",
	"output_size_w",
	"output_size_h",
	"out_format",
	"output_endian_y",
	"output_endian_uv",
	"scale_mode",
	"scale_deci_hor",
	"scale_deci_ver",
	"bp_trim_x",
	"bp_trim_y",
	"bp_trim_w",
	"bp_trim_h"
};
static void usage(void) {
	INFO("Usage: 25 args\n");
	INFO("utest_scaling -if 2 -iw 640 -ih 480 -rx 0 -ry 0 -rw 0 -rh 0 -iey 0 -ieu 0"
	"-sctx 0 -scty 0 -sctw 0 -scth 0 -ow 1280 -oh 960 -op 1296 -of 2"
	"-oey 0 -oeu 0 -sm 2 -decih 0 -deciv 0 -box 0 -boy 0 -bow 320 -boh 240 -bop 336\n");
}

static cpp_memory_t *allocMem(int buf_size, int num_bufs)
{
	size_t mem_size = 0;
	int heap_type;
	int iommu_en = false;
	MemIon *pHeapIon = NULL;
	unsigned long paddr = 0;

#ifdef TEST_ON_HAPS
	iommu_en = false;
#else
	iommu_en = true;
#endif
	//for test....
	iommu_en = true;
	heap_type = iommu_en ? ION_HEAP_ID_MASK_SYSTEM : ION_HEAP_ID_MASK_MM;

	cpp_memory_t *memory = (cpp_memory_t *)malloc(sizeof(cpp_memory_t));
	if (NULL == memory) {
		goto getpmem_fail;
	}
	memset(memory, 0, sizeof(cpp_memory_t));
	memory->busy_flag = false;

	mem_size = buf_size * num_bufs;
	mem_size = (mem_size + 4095U) & (~4095U); //Why need 4096 Align

	pHeapIon = new MemIon("/dev/ion", mem_size, MemIon::NO_CACHING, heap_type);

	if (pHeapIon == NULL || pHeapIon->getHeapID() < 0) {
		ERR("failed to alloc ion pmem buffer1.\n");
		goto getpmem_fail;
	}
	pHeapIon->get_phy_addr_from_ion(&paddr, &mem_size);

	if (NULL == pHeapIon->getBase() || MAP_FAILED == pHeapIon->getBase()) {
		ERR("failed to alloc ion pmem buffer2.\n");
		goto getpmem_fail;
	}

	memory->ion_heap = pHeapIon;
	memory->fd = pHeapIon->getHeapID();
	memory->dev_fd = pHeapIon->getIonDeviceFd();
	memory->phys_addr = paddr;
	memory->phys_size = mem_size;
	memory->data = pHeapIon->getBase();

	INFO("alloc success fd=0x%x, phys_addr=0x%lx, virt_addr=%p, size=0x%lx, heap=%p\n",
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

static void freeMem(cpp_memory_t *memory)
{
	if (!memory)
		return;

	ERR("free memory %p,  fd %d, ptr %p\n",memory, memory->fd, memory->data);
	if (memory->ion_heap) {
		delete memory->ion_heap;
		memory->ion_heap = NULL;
	}
	free(memory);
}

static int utest_scal_param_set(struct utest_scal_cxt *scal_cxt_ptr,
	int argc, char **argv)
{
	FILE *fp = NULL;
	char param[20];
	int value = 0;
	int i;

	memset(scal_cxt_ptr, 0, sizeof(struct utest_scal_cxt));
	if (fp = fopen(u_test_cfg_file, "r")){
		//Read Parameter from File.
		fscanf(fp, "%s%d", param, &scal_cxt_ptr->scal_cfg.input_size.w);
		ERR("param get from txt is param:%s, value:%d .\n",
			param, scal_cxt_ptr->scal_cfg.input_size.w);
		fscanf(fp, "%s%d", param, &scal_cxt_ptr->scal_cfg.input_size.h);
		ERR("param get from txt is param:%s, value:%d .\n",
			param, scal_cxt_ptr->scal_cfg.input_size.h);
		fscanf(fp, "%s%d", param, &scal_cxt_ptr->scal_cfg.input_rect.x);
		ERR("param get from txt is param:%s, value:%d .\n",
			param, scal_cxt_ptr->scal_cfg.input_rect.x);
		fscanf(fp, "%s%d", param, &scal_cxt_ptr->scal_cfg.input_rect.y);
		ERR("param get from txt is param:%s, value:%d .\n",
			param, scal_cxt_ptr->scal_cfg.input_rect.y);
		fscanf(fp, "%s%d", param, &scal_cxt_ptr->scal_cfg.input_rect.w);
		ERR("param get from txt is param:%s, value:%d .\n",
			param, scal_cxt_ptr->scal_cfg.input_rect.w);
		fscanf(fp, "%s%d", param, &scal_cxt_ptr->scal_cfg.input_rect.h);
		ERR("param get from txt is param:%s, value:%d .\n",
			param, scal_cxt_ptr->scal_cfg.input_rect.h);
		fscanf(fp, "%s%d", param, &scal_cxt_ptr->scal_cfg.input_format);
		ERR("param get from txt is param:%s, value:%d .\n",
			param, scal_cxt_ptr->scal_cfg.input_format);
		fscanf(fp, "%s%d", param, &scal_cxt_ptr->scal_cfg.input_endian.y_endian);
		ERR("param get from txt is param:%s, value:%d .\n",
			param, scal_cxt_ptr->scal_cfg.input_endian.y_endian);
		fscanf(fp, "%s%d", param, &scal_cxt_ptr->scal_cfg.input_endian.uv_endian);
		ERR("param get from txt is param:%s, value:%d .\n",
			param, scal_cxt_ptr->scal_cfg.input_endian.uv_endian);
		fscanf(fp, "%s%d", param, &scal_cxt_ptr->scal_cfg.sc_trim.x);
		ERR("param get from txt is param:%s, value:%d .\n",
			param, scal_cxt_ptr->scal_cfg.sc_trim.x);
		fscanf(fp, "%s%d", param, &scal_cxt_ptr->scal_cfg.sc_trim.y);
		ERR("param get from txt is param:%s, value:%d .\n",
			param, scal_cxt_ptr->scal_cfg.sc_trim.y);
		fscanf(fp, "%s%d", param, &scal_cxt_ptr->scal_cfg.sc_trim.w);
		ERR("param get from txt is param:%s, value:%d .\n",
			param, scal_cxt_ptr->scal_cfg.sc_trim.w);
		fscanf(fp, "%s%d", param, &scal_cxt_ptr->scal_cfg.sc_trim.h);
		ERR("param get from txt is param:%s, value:%d .\n",
			param, scal_cxt_ptr->scal_cfg.sc_trim.h);
		fscanf(fp, "%s%d", param, &scal_cxt_ptr->scal_cfg.output_size.w);
		ERR("param get from txt is param:%s, value:%d .\n",
			param, scal_cxt_ptr->scal_cfg.output_size.w);
		fscanf(fp, "%s%d", param, &scal_cxt_ptr->scal_cfg.output_size.h);
		ERR("param get from txt is param:%s, value:%d .\n",
			param, scal_cxt_ptr->scal_cfg.output_size.h);
		fscanf(fp, "%s%d", param, &scal_cxt_ptr->scal_cfg.output_format);
		ERR("param get from txt is param:%s, value:%d .\n",
			param, scal_cxt_ptr->scal_cfg.output_format);
		fscanf(fp, "%s%d", param, &scal_cxt_ptr->scal_cfg.output_endian.y_endian);
		ERR("param get from txt is param:%s, value:%d .\n",
			param, scal_cxt_ptr->scal_cfg.output_endian.y_endian);
		fscanf(fp, "%s%d", param, &scal_cxt_ptr->scal_cfg.output_endian.uv_endian);
		ERR("param get from txt is param:%s, value:%d .\n",
			param, scal_cxt_ptr->scal_cfg.output_endian.uv_endian);
		fscanf(fp, "%s%d", param, &scal_cxt_ptr->scal_cfg.scale_mode);
		ERR("param get from txt is param:%s, value:%d .\n",
			param, scal_cxt_ptr->scal_cfg.scale_mode);
		fscanf(fp, "%s%d", param, &scal_cxt_ptr->scal_cfg.scale_deci.hor);
		ERR("param get from txt is param:%s, value:%d .\n",
			param, scal_cxt_ptr->scal_cfg.scale_deci.hor);
		fscanf(fp, "%s%d", param, &scal_cxt_ptr->scal_cfg.scale_deci.ver);
		ERR("param get from txt is param:%s, value:%d .\n",
			param, scal_cxt_ptr->scal_cfg.scale_deci.ver);
		fscanf(fp, "%s%d", param, &scal_cxt_ptr->scal_cfg.bp_trim.x);
		ERR("param get from txt is param:%s, value:%d .\n",
			param, scal_cxt_ptr->scal_cfg.bp_trim.x);
		fscanf(fp, "%s%d", param, &scal_cxt_ptr->scal_cfg.bp_trim.y);
		ERR("param get from txt is param:%s, value:%d .\n",
			param, scal_cxt_ptr->scal_cfg.bp_trim.y);
		fscanf(fp, "%s%d", param, &scal_cxt_ptr->scal_cfg.bp_trim.w);
		ERR("param get from txt is param:%s, value:%d .\n",
			param, scal_cxt_ptr->scal_cfg.bp_trim.w);
		fscanf(fp, "%s%d", param, &scal_cxt_ptr->scal_cfg.bp_trim.h);
		ERR("param get from txt is param:%s, bp_trim.h value:%d .\n",
			param, scal_cxt_ptr->scal_cfg.bp_trim.h);
		scal_cxt_ptr->scal_cfg.output_pitch = scal_cxt_ptr->scal_cfg.output_size.w;
		scal_cxt_ptr->scal_cfg.bpout_pitch = scal_cxt_ptr->scal_cfg.bp_trim.w;
		fclose(fp);
	}else{
		//Read parameter from command line parameter.
		if (argc < 25) {
			usage();
			return -1;
		}
		for (i = 1; i < argc; i++) {
			if (strcmp(argv[i], "-if") == 0 && (i < argc - 1)) {
				scal_cxt_ptr->scal_cfg.input_format = atoi(argv[++i]);
			} else if (strcmp(argv[i], "-iw") == 0 && (i < argc - 1)) {
				scal_cxt_ptr->scal_cfg.input_size.w = atoi(argv[++i]);
			} else if (strcmp(argv[i], "-ih") == 0 && (i < argc - 1)) {
				scal_cxt_ptr->scal_cfg.input_size.h = atoi(argv[++i]);
			} else if (strcmp(argv[i], "-rx") == 0 && (i < argc - 1)) {
				scal_cxt_ptr->scal_cfg.input_rect.x = atoi(argv[++i]);
			} else if (strcmp(argv[i], "-ry") == 0 && (i < argc - 1)) {
				scal_cxt_ptr->scal_cfg.input_rect.y = atoi(argv[++i]);
			} else if (strcmp(argv[i], "-rw") == 0 && (i < argc - 1)) {
				scal_cxt_ptr->scal_cfg.input_rect.w = atoi(argv[++i]);
			} else if (strcmp(argv[i], "-rh") == 0 && (i < argc - 1)) {
				scal_cxt_ptr->scal_cfg.input_rect.h = atoi(argv[++i]);
			} else if (strcmp(argv[i], "-iey") == 0 && (i < argc - 1)) {
				scal_cxt_ptr->scal_cfg.input_endian.y_endian = atoi(argv[++i]);
			} else if (strcmp(argv[i], "-ieu") == 0 && (i < argc - 1)) {
				scal_cxt_ptr->scal_cfg.input_endian.uv_endian = atoi(argv[++i]);
			} else if (strcmp(argv[i], "-sctx") == 0 && (i < argc - 1)) {
				scal_cxt_ptr->scal_cfg.sc_trim.x = atoi(argv[++i]);
			} else if (strcmp(argv[i], "-scty") == 0 && (i < argc - 1)) {
				scal_cxt_ptr->scal_cfg.sc_trim.y = atoi(argv[++i]);
			} else if (strcmp(argv[i], "-sctw") == 0 && (i < argc - 1)) {
				scal_cxt_ptr->scal_cfg.sc_trim.w = atoi(argv[++i]);
			} else if (strcmp(argv[i], "-scth") == 0 && (i < argc - 1)) {
			scal_cxt_ptr->scal_cfg.sc_trim.h = atoi(argv[++i]);
			} else if (strcmp(argv[i], "-ow") == 0 && (i < argc - 1)) {
				scal_cxt_ptr->scal_cfg.output_size.w = atoi(argv[++i]);
			} else if (strcmp(argv[i], "-oh") == 0 && (i < argc - 1)) {
				scal_cxt_ptr->scal_cfg.output_size.h = atoi(argv[++i]);
			}else if (strcmp(argv[i], "-op") == 0 && (i < argc - 1)) {
				scal_cxt_ptr->scal_cfg.output_pitch = atoi(argv[++i]);
			} else if (strcmp(argv[i], "-of") == 0 && (i < argc - 1)) {
				scal_cxt_ptr->scal_cfg.output_format = atoi(argv[++i]);
			} else if (strcmp(argv[i], "-oey") == 0 && (i < argc - 1)) {
				scal_cxt_ptr->scal_cfg.output_endian.y_endian = atoi(argv[++i]);
			} else if (strcmp(argv[i], "-oeu") == 0 && (i < argc - 1)) {
				scal_cxt_ptr->scal_cfg.output_endian.uv_endian = atoi(argv[++i]);
			} else if (strcmp(argv[i], "-sm") == 0 && (i < argc - 1)) {
				scal_cxt_ptr->scal_cfg.scale_mode = atoi(argv[++i]);
			} else if (strcmp(argv[i], "-decih") == 0 && (i < argc - 1)) {
				scal_cxt_ptr->scal_cfg.scale_deci.hor = atoi(argv[++i]);
			} else if (strcmp(argv[i], "-deciv") == 0 && (i < argc - 1)) {
				scal_cxt_ptr->scal_cfg.scale_deci.ver = atoi(argv[++i]);
			} else if (strcmp(argv[i], "-box") == 0 && (i < argc - 1)) {
				scal_cxt_ptr->scal_cfg.bp_trim.x = atoi(argv[++i]);
			} else if (strcmp(argv[i], "-boy") == 0 && (i < argc - 1)) {
				scal_cxt_ptr->scal_cfg.bp_trim.y = atoi(argv[++i]);
			} else if (strcmp(argv[i], "-bow") == 0 && (i < argc - 1)) {
				scal_cxt_ptr->scal_cfg.bp_trim.w = atoi(argv[++i]);
			} else if (strcmp(argv[i], "-boh") == 0 && (i < argc - 1)) {
				scal_cxt_ptr->scal_cfg.bp_trim.h = atoi(argv[++i]);
			}else if (strcmp(argv[i], "-bop") == 0 && (i < argc - 1)) {
				scal_cxt_ptr->scal_cfg.bpout_pitch = atoi(argv[++i]);
			} else {
				usage();
				return -1;
			}
		}
	}
	return 0;
}

static int utest_scal_mem_alloc(struct utest_scal_cxt *scal_cxt_ptr)
{
	/* alloc input y buffer */
	scal_cxt_ptr->input_y_mem =
	allocMem(scal_cxt_ptr->scal_cfg.input_size.w *
		scal_cxt_ptr->scal_cfg.input_size.h, 1);
	INFO("LIKE:src y phy addr :0x%lx, virtual addr:%p\n",
		scal_cxt_ptr->input_y_mem->phys_addr,
		scal_cxt_ptr->input_y_mem->data);
	memset(scal_cxt_ptr->input_y_mem->data, 0x80,
		scal_cxt_ptr->input_y_mem->phys_size);
#ifdef TEST_ON_HAPS
	//scal_cxt_ptr->scal_cfg.input_addr.y = scal_cxt_ptr->input_y_mem->phys_addr;
#endif
	scal_cxt_ptr->scal_cfg.input_addr_vir.y =
		(unsigned int)scal_cxt_ptr->input_y_mem->data;
	scal_cxt_ptr->scal_cfg.input_addr.mfd[0] =
		scal_cxt_ptr->input_y_mem->fd;
	/* alloc input uv buffer */
	scal_cxt_ptr->input_uv_mem =
	allocMem(scal_cxt_ptr->scal_cfg.input_size.w *
		scal_cxt_ptr->scal_cfg.input_size.h, 1);
	INFO("LIKE:src uv phy addr :0x%lx, virtual addr:%p\n",
		scal_cxt_ptr->input_uv_mem->phys_addr,
		scal_cxt_ptr->input_uv_mem->data);
	memset(scal_cxt_ptr->input_uv_mem->data, 0x80,
		scal_cxt_ptr->input_uv_mem->phys_size);
#ifdef TEST_ON_HAPS
	//scal_cxt_ptr->scal_cfg.input_addr.u = scal_cxt_ptr->input_uv_mem->phys_addr;
	//scal_cxt_ptr->scal_cfg.input_addr.v = scal_cxt_ptr->input_uv_mem->phys_addr;
#endif
	scal_cxt_ptr->scal_cfg.input_addr_vir.u =
		(unsigned int)scal_cxt_ptr->input_uv_mem->data;
	scal_cxt_ptr->scal_cfg.input_addr.mfd[1] =
		scal_cxt_ptr->input_uv_mem->fd;

	/* alloc sc outout y buffer */
	scal_cxt_ptr->output_sc_y_mem =
	allocMem(scal_cxt_ptr->scal_cfg.output_pitch *
		scal_cxt_ptr->scal_cfg.output_size.h, 1);
	INFO("LIKE:sc out y phy addr :0x%lx, virtual addr:%p\n",
		scal_cxt_ptr->output_sc_y_mem->phys_addr,
		scal_cxt_ptr->output_sc_y_mem->data);
	memset(scal_cxt_ptr->output_sc_y_mem->data, 0x80,
		scal_cxt_ptr->output_sc_y_mem->phys_size);
#ifdef TEST_ON_HAPS
	//scal_cxt_ptr->scal_cfg.output_addr.y = scal_cxt_ptr->output_sc_y_mem->phys_addr;
#endif
	scal_cxt_ptr->scal_cfg.output_addr_vir.y =
		(unsigned int)scal_cxt_ptr->output_sc_y_mem->data;
	scal_cxt_ptr->scal_cfg.output_addr.mfd[0] =
		scal_cxt_ptr->output_sc_y_mem->fd;
	/* alloc sc outout uv buffer */
	scal_cxt_ptr->output_sc_uv_mem =
	allocMem(scal_cxt_ptr->scal_cfg.output_pitch *
		scal_cxt_ptr->scal_cfg.output_size.h, 1);
	INFO("LIKE:sc out uv phy addr :0x%lx, virtual addr:%p\n",
		scal_cxt_ptr->output_sc_uv_mem->phys_addr,
		scal_cxt_ptr->output_sc_uv_mem->data);
	memset(scal_cxt_ptr->output_sc_uv_mem->data, 0x80,
		scal_cxt_ptr->output_sc_uv_mem->phys_size);
#ifdef TEST_ON_HAPS
	//scal_cxt_ptr->scal_cfg.output_addr.u = scal_cxt_ptr->output_sc_uv_mem->phys_addr;
	//scal_cxt_ptr->scal_cfg.output_addr.v = scal_cxt_ptr->output_sc_uv_mem->phys_addr;
#endif
	scal_cxt_ptr->scal_cfg.output_addr_vir.u =
		(unsigned int)scal_cxt_ptr->output_sc_uv_mem->data;
	scal_cxt_ptr->scal_cfg.output_addr.mfd[1] =
		scal_cxt_ptr->output_sc_uv_mem->fd;
	if (scal_cxt_ptr->scal_cfg.scale_mode == 2) {
		/* alloc bp outout y buffer */
		scal_cxt_ptr->output_bp_y_mem =
		allocMem(scal_cxt_ptr->scal_cfg.bpout_pitch *
			scal_cxt_ptr->scal_cfg.bp_trim.h, 1);
		INFO("LIKE:bp out y phy addr :0x%lx, virtual addr:%p\n",
			scal_cxt_ptr->output_bp_y_mem->phys_addr,
			scal_cxt_ptr->output_bp_y_mem->data);
		memset(scal_cxt_ptr->output_bp_y_mem->data, 0x80,
			scal_cxt_ptr->output_bp_y_mem->phys_size);
#ifdef TEST_ON_HAPS
		//scal_cxt_ptr->scal_cfg.bp_output_addr.y = scal_cxt_ptr->output_bp_y_mem->phys_addr;
#endif
		scal_cxt_ptr->scal_cfg.bp_output_addr_vir.y =
			(unsigned int)scal_cxt_ptr->output_bp_y_mem->data;
		scal_cxt_ptr->scal_cfg.bp_output_addr.mfd[0] =
			scal_cxt_ptr->output_bp_y_mem->fd;
	    /* alloc bp outout uv buffer */
		scal_cxt_ptr->output_bp_uv_mem =
		allocMem(scal_cxt_ptr->scal_cfg.bpout_pitch *
			scal_cxt_ptr->scal_cfg.bp_trim.h, 1);
		INFO("LIKE:bp out uv phy addr :0x%lx, virtual addr:%p\n",
			scal_cxt_ptr->output_bp_uv_mem->phys_addr,
			scal_cxt_ptr->output_bp_uv_mem->data);
		memset(scal_cxt_ptr->output_bp_uv_mem->data, 0x80,
			scal_cxt_ptr->output_bp_uv_mem->phys_size);
#ifdef TEST_ON_HAPS
		//scal_cxt_ptr->scal_cfg.bp_output_addr.u = scal_cxt_ptr->output_bp_uv_mem->phys_addr;
		//scal_cxt_ptr->scal_cfg.bp_output_addr.v = scal_cxt_ptr->output_bp_uv_mem->phys_addr;
#endif
		scal_cxt_ptr->scal_cfg.bp_output_addr_vir.u =
			(unsigned int)scal_cxt_ptr->output_bp_uv_mem->data;
		scal_cxt_ptr->scal_cfg.bp_output_addr.mfd[1] =
			scal_cxt_ptr->output_bp_uv_mem->fd;
	}else {
		scal_cxt_ptr->scal_cfg.bp_output_addr.y = 0;
		scal_cxt_ptr->scal_cfg.bp_output_addr.u = 0;
		scal_cxt_ptr->scal_cfg.bp_output_addr.v = 0;
	}
	//For Write Test
	INFO("utest_scaling: alloc memory OK\n");

	return 0;
}

static int utest_scal_mem_release(struct utest_scal_cxt *scal_cxt_ptr)
{
	freeMem(scal_cxt_ptr->input_y_mem);
	freeMem(scal_cxt_ptr->input_uv_mem);
	freeMem(scal_cxt_ptr->output_sc_y_mem);
	freeMem(scal_cxt_ptr->output_sc_uv_mem);
	freeMem(scal_cxt_ptr->output_bp_y_mem);
	freeMem(scal_cxt_ptr->output_bp_uv_mem);

	return 0;
}

static int
utest_scal_set_src_data(struct utest_scal_cxt *scal_cxt_ptr)
{
	FILE *fp = 0;
	INFO("utest scaling read src image start\n");
	/* get input_y src */ //0 YUV4202P 2YUV4222P
	if (scal_cxt_ptr->scal_cfg.input_format == 2)
		fp = fopen(utest_scal_src_y_422_file, "r");
	else if (scal_cxt_ptr->scal_cfg.input_format == 0)
		fp = fopen(utest_scal_src_y_420_file, "r");
	if (fp != NULL) {
		fread((void *)scal_cxt_ptr->input_y_mem->data, 1,
			scal_cxt_ptr->scal_cfg.input_size.w *
			scal_cxt_ptr->scal_cfg.input_size.h,
			fp);
		fclose(fp);
	} else {
		ERR("utest_scaling_src_cfg fail : no input_y source file.\n");
		return -1;
	}

	/* get input_uv src */
	if (scal_cxt_ptr->scal_cfg.input_format == 2) {
		fp = fopen(utest_scal_src_uv_422_file, "r");
		if (fp != NULL) {
			fread((void *)scal_cxt_ptr->input_uv_mem->data, 1,
				scal_cxt_ptr->scal_cfg.input_size.w *
				scal_cxt_ptr->scal_cfg.input_size.h,
				fp);
			fclose(fp);
		} else {
			ERR("utest_scaling_src_cfg fail : no input_uv source file.\n");
			return -1;
		}
	} else if (scal_cxt_ptr->scal_cfg.input_format == 0) {
		fp = fopen(utest_scal_src_uv_420_file, "r");
		if (fp != NULL) {
			fread((void *)scal_cxt_ptr->input_uv_mem->data, 1,
				scal_cxt_ptr->scal_cfg.input_size.w *
				scal_cxt_ptr->scal_cfg.input_size.h / 2,
				fp);
			fclose(fp);
		} else {
			ERR("utest_scaling_src_cfg fail : no input_uv source file.\n");
			return -1;
		}
	}
	INFO("utest scaling read src image OK\n");
	
	return 0;
}
#if 0
static int crop_raw(uint8 *vect, unsigned int w, unsigned int h, unsigned int pitch) {
	uint8 *temp = NULL;
	uint8 *golden = NULL;
	int i,j;

	if (pitch != w)
	{
		temp = (uint8 *)malloc(sizeof(uint8)*w*h);
		memset(temp, 0, sizeof(uint8)*w*h);
		for (i=0; i<h;i++) {
			for(j=0;j<w;j++)
				temp[i*w+j] = vect[i*pitch+j];
		}
		memcpy(vect, temp, w*h);
		free(temp);
	}
}
#endif
static int utest_scal_set_des_data(
	struct utest_scal_cxt *scal_cxt_ptr, int count) {

	FILE *fp = 0;
	char sc_file_name[128] = "utest_scaling_output_temp.raw";
	char bp_file_name[128] = "utest_scaling_output_temp.raw";

//scale path des data save
	sprintf(sc_file_name, utest_scal_sc_dst_y_file,
		scal_cxt_ptr->scal_cfg.output_pitch,
		scal_cxt_ptr->scal_cfg.output_size.h,
		scal_cxt_ptr->scal_cfg.input_format,
		scal_cxt_ptr->scal_cfg.output_format,
		count);
	fp = fopen(sc_file_name, "wb");
	if (fp != NULL) {
		fwrite((void *)scal_cxt_ptr->output_sc_y_mem->data, 1,
			scal_cxt_ptr->scal_cfg.output_pitch *
			scal_cxt_ptr->scal_cfg.output_size.h,
			fp);
		fclose(fp);
	} else {
		ERR("utest_scaling_save_raw_data: failed to open save_file_y.\n");
		return -1;
	}

	sprintf(sc_file_name, utest_scal_sc_dst_uv_file,
		scal_cxt_ptr->scal_cfg.output_pitch,
		scal_cxt_ptr->scal_cfg.output_size.h,
		scal_cxt_ptr->scal_cfg.input_format,
		scal_cxt_ptr->scal_cfg.output_format,
		count);
	fp = fopen(sc_file_name, "wb");
	if (fp != NULL) {
		if (scal_cxt_ptr->scal_cfg.output_format == 2)
		fwrite((void *)scal_cxt_ptr->output_sc_uv_mem->data, 1,
			scal_cxt_ptr->scal_cfg.output_pitch *
			scal_cxt_ptr->scal_cfg.output_size.h,
			fp);
		else if (scal_cxt_ptr->scal_cfg.output_format == 0)
		fwrite((void *)scal_cxt_ptr->output_sc_uv_mem->data, 1,
			scal_cxt_ptr->scal_cfg.output_pitch *
			scal_cxt_ptr->scal_cfg.output_size.h /2,
			fp);

		fclose(fp);
	} else {
		ERR("utest_scaling_save_raw_data: failed to open save_file_uv.\n");
		return -1;
	}

	if (scal_cxt_ptr->scal_cfg.scale_mode == 2) {
	// bp path des data save
		sprintf(bp_file_name, utest_scal_bp_dst_y_file,
			scal_cxt_ptr->scal_cfg.bpout_pitch,
			scal_cxt_ptr->scal_cfg.bp_trim.h,
			scal_cxt_ptr->scal_cfg.input_format,
			scal_cxt_ptr->scal_cfg.input_format,
			count);
	fp = fopen(bp_file_name, "wb");
	if (fp != NULL) {
		fwrite((void *)scal_cxt_ptr->output_bp_y_mem->data, 1,
			scal_cxt_ptr->scal_cfg.bpout_pitch *
			scal_cxt_ptr->scal_cfg.bp_trim.h,
			fp);
		fclose(fp);
	} else {
		ERR("utest_scaling_save_raw_data: failed to open save_bp_file_y.\n");
		return -1;
	}
		sprintf(bp_file_name, utest_scal_bp_dst_uv_file,
			scal_cxt_ptr->scal_cfg.bpout_pitch,scal_cxt_ptr->scal_cfg.bp_trim.h,
			scal_cxt_ptr->scal_cfg.input_format,scal_cxt_ptr->scal_cfg.input_format,
			count);
		fp = fopen(bp_file_name, "wb");
		if (fp != NULL) {
			if (scal_cxt_ptr->scal_cfg.input_format == 2)
				fwrite((void *)scal_cxt_ptr->output_bp_uv_mem->data, 1,
					scal_cxt_ptr->scal_cfg.bpout_pitch *
					scal_cxt_ptr->scal_cfg.bp_trim.h,
					fp);
			else if (scal_cxt_ptr->scal_cfg.input_format == 0)
				fwrite((void *)scal_cxt_ptr->output_bp_uv_mem->data, 1,
					scal_cxt_ptr->scal_cfg.bpout_pitch *
					scal_cxt_ptr->scal_cfg.bp_trim.h /2,
					fp);

			fclose(fp);
		} else {
			ERR("utest_scaling_save_raw_data: failed to open save_file_uv.\n");
			return -1;
		}
	}
    return 0;
}

int main(int argc, char **argv) {
	int i = 0, ret = -1;
	int64_t time_start = 0, time_end = 0;
	cmr_handle scal_handle;
	struct utest_scal_cxt utest_scal_cxt;
	struct utest_scal_cxt *scal_cxt_ptr = &utest_scal_cxt;
	struct cpp_scale_param scal_param;

	scal_param.host_fd = -1;
	scal_param.scale_cfg_param = &scal_cxt_ptr->scal_cfg;

	if (utest_scal_param_set(scal_cxt_ptr, argc, argv)) {
		ERR("get invild scal param.\n");
		return ret;
	}
	if (utest_scal_mem_alloc(scal_cxt_ptr)) {
		ERR("failed to alloc scal memory.\n");
		goto err;
	}

	if (utest_scal_set_src_data(scal_cxt_ptr)) {
		ERR("failed to set src data.\n");
		goto err;
	}

	ERR("JUN1 inaddry%d, infmt%d, inw%d\n", scal_param.scale_cfg_param->input_addr.y,
		scal_param.scale_cfg_param->input_format,
		scal_param.scale_cfg_param->input_size.w);
	usleep(30*1000);
	for (i = 0; i < UTEST_SCALING_COUNTER; i++) {
		if (cpp_scale_open(&scal_handle) != 0) {
			ERR("failed to open scal drv.\n");
			goto err;
		}
	scal_param.handle = scal_handle;
	time_start = systemTime();
//open DVFS test here.
	if (cpp_scale_start(&scal_param) != 0) {
		ERR("failed to start scal drv.\n");
		goto err;
	}
//close DVFS here
	time_end = systemTime();
	INFO("utest_scal testing  end cost time=%d\n",
	     (unsigned int)((time_end - time_start) / 1000000L));

	usleep(30*1000);
	if (utest_scal_set_des_data(scal_cxt_ptr, i)) {
		ERR("failed to save scal des data.\n");
		goto err;
	}

	if (cpp_scale_close(scal_param.handle) != 0) {
		ERR("failed to close scal drv.\n");
		goto err;
	}
	}

err:
	INFO("Before While(1)\n");
	while(1);
	utest_scal_mem_release(scal_cxt_ptr);
	return 0;
}
