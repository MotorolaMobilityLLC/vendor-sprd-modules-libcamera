/* Copyright (c) 2017, The Linux Foundataion. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include "module_wrapper_drv.h"
#include <sys/ioctl.h>
#include <fcntl.h>
#include "sprd_img.h"
//#include "sprd_cam_test.h"

#define LOG_TAG "IT-moduleDrv"
#define THIS_MODULE_NAME "drv"
#define CAMT_DEV_NAME "/dev/sprd_image"
extern map<string,vector<resultData_t>*> gMap_Result;

#if 0
struct camt_drv_context {
	int fd;
	struct camt_info info;
};
#endif

ModuleWrapperDRV::ModuleWrapperDRV()
{
    IT_LOGD("");
    pVec_Result=gMap_Result[THIS_MODULE_NAME];
}

ModuleWrapperDRV::~ModuleWrapperDRV()
{
    IT_LOGD("");
}

int ModuleWrapperDRV::SetUp()
{
    int ret = IT_OK;
    IT_LOGD("");
    return ret;
}

int ModuleWrapperDRV::TearDown()
{
    int ret = IT_OK;
    IT_LOGD("");
    return ret;
}

int ModuleWrapperDRV::Run(IParseJson *Json2)
{
	int ret = IT_OK;
	IT_LOGD("");
#if 0
	struct camt_drv_context cxt;

	struct test_camera_mem_type_stat type;
	int size, sum;
	Test_Mem_alloc *pMemAlloc = NULL;
	int index = 0;
	int fd[2] = {-1, -1};
#endif

#if 0 //temp
	memset(&cxt, 0, sizeof(cxt));
	/* open camera device */
	cxt.fd = open(CAMT_DEV_NAME, O_RDWR, 0);
	if (cxt.fd == -1) {
		IT_LOGE("failed to open dcam devic, errno: %d", errno);
		return -1;
	}

	/* camt init */
	cxt.info.cmd = CAMT_CMD_INIT;
	/* chip should com from json2 */
	cxt.info.chip = CAMT_CHIP_DCAM0;
	ret = ioctl(cxt.fd, SPRD_IMG_IO_CAM_TEST, &cxt.info);
	if (ret < 0)
		goto exit;

	/* malloc buffer, get frame */
	#if 0
	type.is_cache = false;
	type.mem_type = TEST_CAMERA_PREVIEW;
	/* 1164x876 */
	size = (1152 * 5 / 4 + 16) * 876;
	sum = 2;
	index = 0;

	pMemAlloc = new Test_Mem_alloc();
	pMemAlloc->Test_Callback_IonMalloc(mem_type, &size, &sum, NULL, NULL, NULL, NULL);

	if (type.mem_type == TEST_CAMERA_PREVIEW) {
		for (list<Test_MemIonQueue>::iterator i = cam_MemGpuQueue.begin(); i != cam_MemGpuQueue.end(); i++) {
			if ((type.mem_type == i->mem_type.mem_type) && (NULL != i->mIonHeap)) {
				fd[index] = i->mIonHeap->fd;
				index++;
				if (index >= 2)
					break;
			}
		}
	}
	#endif

	/* start camt */
	cxt.info.cmd = CAMT_CMD_START;
	cxt.info.dcam_id = CAMT_DCAM_0;
	cxt.info.dcam_path_id = CAMT_DCAM_PATH_BIN;
	cxt.info.online = 0;
	ret = ioctl(cxt.fd, SPRD_IMG_IO_CAM_TEST, &cxt.info);
	if (ret < 0) {
		IT_LOGE("failed to start cam drv test");
		goto start_fail;
	}

	cxt.info.cmd = CAMT_CMD_STOP;
	ret = ioctl(cxt.fd, SPRD_IMG_IO_CAM_TEST, &cxt.info);
	if (ret < 0)
		IT_LOGE("failed to stop cam drv test");

start_fail:
	cxt.info.cmd = CAMT_CMD_DEINIT;
	ioctl(cxt.fd, SPRD_IMG_IO_CAM_TEST, &cxt.info);
#endif
exit:
	return ret;
}

int ModuleWrapperDRV::InitOps()
{
    int ret = IT_OK;
    IT_LOGD("");
    return ret;
}
