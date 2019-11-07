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
#define UTEST_ROTATION_COUNTER 1

struct utest_rot_cxt {
    MemIon *input_y_pmem_hp;
    size_t input_y_pmem_size;
    unsigned long input_y_phy_addr;
    unsigned char *input_y_vir_addr;

    MemIon *input_uv_pmem_hp;
    size_t input_uv_pmem_size;
    unsigned long input_uv_phy_addr;
    unsigned char *input_uv_vir_addr;

    MemIon *output_y_pmem_hp;
    size_t output_y_pmem_size;
    unsigned long output_y_phy_addr;
    unsigned char *output_y_vir_addr;

    MemIon *output_uv_pmem_hp;
    size_t output_uv_pmem_size;
    unsigned long output_uv_phy_addr;
    unsigned char *output_uv_vir_addr;

    struct sprd_cpp_rot_cfg_parm rot_cfg;
};

static char utest_rot_src_y_422_file[] = "/data/vendor/cameraserver/data/like/pic/src_y_422.raw";
static char utest_rot_src_uv_422_file[] = "/data/vendor/cameraserver/data/like/pic/src_uv_422.raw";
static char utest_rot_src_y_420_file[] = "/data/vendor/cameraserver/data/like/pic/src_y_420.raw";
static char utest_rot_src_uv_420_file[] = "/data/vendor/cameraserver/data/like/pic/src_uv_420.raw";
static char utest_rot_dst_y_file[] =
    "/data/vendor/cameraserver/data/like/pic/dst_y_%dx%d-angle%d-format%d_%d.raw";
static char utest_rot_dst_uv_file[] =
    "/data/vendor/cameraserver/data/like/pic/dst_uv_%dx%d-angle%d-format%d_%d.raw";

static void usage(void) {
    INFO("Usage:\n");
    INFO("utest_rot -if format(yuv422) -iw width -ih height -ia angle(0(90) 1(180) 2(270) 3(-1))\n");
}

static unsigned int utest_rot_angle_cvt(int angle) {
    unsigned int tmp_angle = 0;

	if (angle == 90) {
		tmp_angle = ROT_90;
    } else if (angle == 270) {
        tmp_angle = ROT_270;
    } else if (angle == 180) {
        tmp_angle = ROT_180;
    } else if (angle == -1) {
		tmp_angle = ROT_MIRROR;
    } else {
		ERR("utest rotate  error  . Line:%d ", __LINE__);
    }
    INFO("utest_rotation_angle_cvt %d \n", tmp_angle);
    return tmp_angle;
}

static int utest_rot_mem_alloc(struct utest_rot_cxt *rot_cxt_ptr) {
//intput y addr-------------------------------
	rot_cxt_ptr->input_y_pmem_hp = new MemIon("/dev/ion",
		rot_cxt_ptr->rot_cfg.size.w * rot_cxt_ptr->rot_cfg.size.h,
		MemIon::NO_CACHING, ION_HEAP_ID_MASK_SYSTEM);

	if (rot_cxt_ptr->input_y_pmem_hp->getHeapID() < 0) {
		ERR("failed to alloc input_y pmem buffer.\n");
		return -1;
	}

	rot_cxt_ptr->input_y_pmem_hp->get_phy_addr_from_ion(
		&rot_cxt_ptr->input_y_phy_addr,
		&rot_cxt_ptr->input_y_pmem_size);

	rot_cxt_ptr->input_y_vir_addr =
		(unsigned char *)rot_cxt_ptr->input_y_pmem_hp->getBase();

    ERR("ROT:src y :phy addr:0x%lx,vir addr:%p\n",
        rot_cxt_ptr->input_y_phy_addr,
        rot_cxt_ptr->input_y_vir_addr);

    if (!rot_cxt_ptr->input_y_phy_addr) {
        ERR("failed to alloc input_y pmem buffer:addr is null.\n");
        return -1;
    }
    memset(rot_cxt_ptr->input_y_vir_addr, 0x80,
		rot_cxt_ptr->rot_cfg.size.w * rot_cxt_ptr->rot_cfg.size.h);

    //rot_cxt_ptr->rot_cfg.src_addr.y = rot_cxt_ptr->input_y_phy_addr;
    rot_cxt_ptr->rot_cfg.src_addr.mfd[0] = rot_cxt_ptr->input_y_pmem_hp->getHeapID();

//intput uv addr-------------------------------
    rot_cxt_ptr->input_uv_pmem_hp = new MemIon(
        "/dev/ion",
        rot_cxt_ptr->rot_cfg.size.w * rot_cxt_ptr->rot_cfg.size.h,
        MemIon::NO_CACHING, ION_HEAP_ID_MASK_SYSTEM);

    if (rot_cxt_ptr->input_uv_pmem_hp->getHeapID() < 0) {
        ERR("failed to alloc input_uv pmem buffer.\n");
        return -1;
    }

    rot_cxt_ptr->input_uv_pmem_hp->get_phy_addr_from_ion(
        &rot_cxt_ptr->input_uv_phy_addr,
        &rot_cxt_ptr->input_uv_pmem_size);

    rot_cxt_ptr->input_uv_vir_addr =
        (unsigned char *)rot_cxt_ptr->input_uv_pmem_hp->getBase();

    ERR("ROT:src uv :phy addr:0x%lx,vir addr:%p\n",
        rot_cxt_ptr->input_uv_phy_addr,
        rot_cxt_ptr->input_uv_vir_addr);

    if (!rot_cxt_ptr->input_uv_phy_addr) {
        ERR("failed to alloc input_uv pmem buffer:addr is null.\n");
        return -1;
    }
    memset(rot_cxt_ptr->input_uv_vir_addr, 0x80,
		rot_cxt_ptr->rot_cfg.size.w * rot_cxt_ptr->rot_cfg.size.h);

	//rot_cxt_ptr->rot_cfg.src_addr.u = rot_cxt_ptr->input_uv_phy_addr;
	//rot_cxt_ptr->rot_cfg.src_addr.v = rot_cxt_ptr->input_uv_phy_addr;
	rot_cxt_ptr->rot_cfg.src_addr.mfd[1] = rot_cxt_ptr->input_uv_pmem_hp->getHeapID();

//output y addr-------------------------------
	rot_cxt_ptr->output_y_pmem_hp = new MemIon("/dev/ion",
		rot_cxt_ptr->rot_cfg.size.w * rot_cxt_ptr->rot_cfg.size.h,
		MemIon::NO_CACHING, ION_HEAP_ID_MASK_SYSTEM);

    if (rot_cxt_ptr->output_y_pmem_hp->getHeapID() < 0) {
        ERR("failed to alloc output_y pmem buffer.\n");
        return -1;
    }

    rot_cxt_ptr->output_y_pmem_hp->get_phy_addr_from_ion(
        &rot_cxt_ptr->output_y_phy_addr,
        &rot_cxt_ptr->output_y_pmem_size);

    rot_cxt_ptr->output_y_vir_addr =
        (unsigned char *)rot_cxt_ptr->output_y_pmem_hp->getBase();

    ERR("ROT:dst y :phy addr:0x%lx,vir addr:%p\n",
        rot_cxt_ptr->output_y_phy_addr,
        rot_cxt_ptr->output_y_vir_addr);
    if (!rot_cxt_ptr->output_y_phy_addr) {
        ERR("failed to alloc output_y pmem buffer:addr is null.\n");
        return -1;
    }

    //rot_cxt_ptr->rot_cfg.dst_addr.y = rot_cxt_ptr->output_y_phy_addr;
    rot_cxt_ptr->rot_cfg.dst_addr.mfd[0] = rot_cxt_ptr->output_y_pmem_hp->getHeapID();
//output uv addr-------------------------------
    rot_cxt_ptr->output_uv_pmem_hp = new MemIon(
        "/dev/ion",
        rot_cxt_ptr->rot_cfg.size.w * rot_cxt_ptr->rot_cfg.size.h,
        MemIon::NO_CACHING, ION_HEAP_ID_MASK_SYSTEM);

    if (rot_cxt_ptr->output_uv_pmem_hp->getHeapID() < 0) {
        ERR("failed to alloc output_uv pmem buffer.\n");
        return -1;
    }

    rot_cxt_ptr->output_uv_pmem_hp->get_phy_addr_from_ion(
        &rot_cxt_ptr->output_uv_phy_addr,
        &rot_cxt_ptr->output_uv_pmem_size);

    rot_cxt_ptr->output_uv_vir_addr =
        (unsigned char *)rot_cxt_ptr->output_uv_pmem_hp->getBase();

    ERR("ROT:dst uv :phy addr:0x%lx,vir addr:%p\n",
        rot_cxt_ptr->output_uv_phy_addr,
        rot_cxt_ptr->output_uv_vir_addr);
    if (!rot_cxt_ptr->output_uv_phy_addr) {
        ERR("failed to alloc output_uv pmem buffer:addr is null.\n");
        return -1;
    }

    //rot_cxt_ptr->rot_cfg.dst_addr.u = rot_cxt_ptr->output_uv_phy_addr;
    //rot_cxt_ptr->rot_cfg.dst_addr.v = rot_cxt_ptr->output_uv_phy_addr;
    rot_cxt_ptr->rot_cfg.dst_addr.mfd[1] = rot_cxt_ptr->output_uv_pmem_hp->getHeapID();

    return 0;
}

static int utest_rot_mem_release(struct utest_rot_cxt *rot_cxt_ptr)
{
	ERR("utest_rot_mem_release.\n");
	delete rot_cxt_ptr->input_y_pmem_hp;
	delete rot_cxt_ptr->input_uv_pmem_hp;
	delete rot_cxt_ptr->output_y_pmem_hp;
	delete rot_cxt_ptr->output_uv_pmem_hp;

	return 0;
}

static int utest_rot_set_src_data(struct utest_rot_cxt *rot_cxt_ptr)
{
	FILE *fp = 0;
//y
	if (rot_cxt_ptr->rot_cfg.format== 0)
		fp = fopen(utest_rot_src_y_422_file, "r");
	else if (rot_cxt_ptr->rot_cfg.format == 1)
		fp = fopen(utest_rot_src_y_420_file, "r");

    if (fp != NULL) {
        fread((void *)rot_cxt_ptr->input_y_vir_addr, 1,
		rot_cxt_ptr->rot_cfg.size.w * rot_cxt_ptr->rot_cfg.size.h,
              fp);
        fclose(fp);
    } else {
        ERR("utest_rotation_src_cfg fail : no input_y source file.\n");
        return -1;
    }
//uv
    if (rot_cxt_ptr->rot_cfg.format == 0)
        fp = fopen(utest_rot_src_uv_422_file, "r");
    else if (rot_cxt_ptr->rot_cfg.format == 1)
        fp = fopen(utest_rot_src_uv_420_file, "r");

    if (fp != NULL) {
        if (rot_cxt_ptr->rot_cfg.format == 0)
            fread((void *)rot_cxt_ptr->input_uv_vir_addr, 1,
                  rot_cxt_ptr->rot_cfg.size.w * rot_cxt_ptr->rot_cfg.size.h, fp);
        else if (rot_cxt_ptr->rot_cfg.format == 1)
            fread((void *)rot_cxt_ptr->input_uv_vir_addr, 1,
                  rot_cxt_ptr->rot_cfg.size.w * rot_cxt_ptr->rot_cfg.size.h / 2, fp);

        fclose(fp);
    } else {
        ERR("utest_rotation_src_cfg fail : no input_uv source file.\n");
        return -1;
    }

    return 0;
}

static int utest_rot_set_des_data(
    struct utest_rot_cxt *rot_cxt_ptr, int count) {

    FILE *fp = 0;
    char file_name[128] = "utest_rotation_output_temp.raw";

    sprintf(file_name, utest_rot_dst_y_file,
	    rot_cxt_ptr->rot_cfg.size.w,
            rot_cxt_ptr->rot_cfg.size.h,
            rot_cxt_ptr->rot_cfg.angle,
            rot_cxt_ptr->rot_cfg.format,
            count);
    fp = fopen(file_name, "wb");
    if (fp != NULL) {
        fwrite((void *)rot_cxt_ptr->output_y_vir_addr, 1,
               rot_cxt_ptr->rot_cfg.size.w * rot_cxt_ptr->rot_cfg.size.h, fp);
        fclose(fp);
    } else {
        ERR("utest_rotation_save_raw_data: failed to open save_file_y.\n");
        return -1;
    }

    sprintf(file_name, utest_rot_dst_uv_file,
	rot_cxt_ptr->rot_cfg.size.w,
        rot_cxt_ptr->rot_cfg.size.h,
        rot_cxt_ptr->rot_cfg.angle,
        rot_cxt_ptr->rot_cfg.format,
        count);
    fp = fopen(file_name, "wb");
    if (fp != NULL) {
        if (rot_cxt_ptr->rot_cfg.format == 0)
            fwrite((void *)rot_cxt_ptr->output_uv_vir_addr, 1,
                   rot_cxt_ptr->rot_cfg.size.w * rot_cxt_ptr->rot_cfg.size.h, fp);
        else if (rot_cxt_ptr->rot_cfg.format == 1)
            fwrite((void *)rot_cxt_ptr->output_uv_vir_addr, 1,
                   rot_cxt_ptr->rot_cfg.size.w * rot_cxt_ptr->rot_cfg.size.h / 2, fp);
        fclose(fp);
    } else {
        ERR("utest_rotation_save_raw_data: failed to open save_file_uv.\n");
        return -1;
    }

    return 0;
}

static int utest_rot_param_set(
    struct utest_rot_cxt *rot_cxt_ptr, int argc,
    char **argv) {
    int i = 0;

    if (argc < 7) {
        usage();
        return -1;
    }

    memset(rot_cxt_ptr, 0, sizeof(struct utest_rot_cxt));

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-if") == 0 && (i < argc - 1)) {
            if (strcmp(argv[i + 1], "yuv422") == 0)
                rot_cxt_ptr->rot_cfg.format = 0;
            else if (strcmp(argv[i + 1], "yuv420") == 0)
                rot_cxt_ptr->rot_cfg.format = 1;
            else
                ERR("error:picture format is error\n");

            i++;
        } else if (strcmp(argv[i], "-iw") == 0 && (i < argc - 1)) {
            rot_cxt_ptr->rot_cfg.size.w = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-ih") == 0 && (i < argc - 1)) {
            rot_cxt_ptr->rot_cfg.size.h = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-ia") == 0 && (i < argc - 1)) {
           rot_cxt_ptr->rot_cfg.angle= atoi(argv[++i]);
        } else {
            usage();
            return -1;
        }
    }

    return 0;
}

int main(int argc, char **argv) {
    int i = 0, ret = -1;
    int64_t time_start = 0, time_end = 0;
    static struct utest_rot_cxt utest_rot_cxt;
    struct utest_rot_cxt *rot_cxt_ptr = &utest_rot_cxt;
    struct cpp_rot_param rot_param;
    cmr_handle rot_handle;

    //rot_param = (struct cpp_rot_param *)malloc(sizeof(struct cpp_rot_param));
    rot_param.host_fd = -1;
    rot_param.handle = NULL;
    rot_param.rot_cfg_param = &rot_cxt_ptr->rot_cfg;

    if (utest_rot_param_set(rot_cxt_ptr, argc, argv)) {
	ERR("rot error: invild cfg parameters \n");
	ret = CMR_CAMERA_INVALID_PARAM;
	return ret;
    }

    if (utest_rot_mem_alloc(rot_cxt_ptr)) {
	ERR("rot error: no mem alloced for src des data \n");
	ret = CMR_CAMERA_NO_MEM;
	goto err;
    }

    if (utest_rot_set_src_data(rot_cxt_ptr)) {
	ERR("rot error: failed to set srd data\n");
	ret = CMR_CAMERA_INVALID_PARAM;
        goto err;
    }

    usleep(30*1000);
    //ERR("1debug %d \n", *(int *)rot_param.handle);
    for (i = 0; i < UTEST_ROTATION_COUNTER; i++) {
	if (cpp_rot_open(&rot_handle) != 0) {
		ERR("failed to open rot drv.\n");
		goto err;
	}
	rot_param.handle = rot_handle;
	ERR("2debug %d \n", *(int *)rot_param.handle);
	time_start = systemTime();
	if (cpp_rot_start(&rot_param) != 0) {
		ERR("failed to start rot drv.\n");
		goto err;
	}
	time_end = systemTime();
	ERR("utest_rotation testing  end cost time=%d\n",(unsigned int)((time_end - time_start) / 1000000L));

	usleep(30*1000);
	if (utest_rot_set_des_data(rot_cxt_ptr, i)){
		ERR("failed to utest_rot_set_des_data.\n");
		goto err;
	}
	if (cpp_rot_close(rot_param.handle) != 0) {
	ERR("failed to close rot drv.\n");
	goto err;
	}
    }
	while(1);
	//return 0;
err:
	utest_rot_mem_release(rot_cxt_ptr);

	return ret;
}

