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
#include "sprd_cam_test.h"
#include "test_drv_isp_parameter.h"
#include "json2drv.h"
#include "test_mem_alloc.h"

#define LOG_TAG "IT-moduleDrv"
#define THIS_MODULE_NAME "drv"
#define CAMT_DEV_NAME "/dev/sprd_image"
#define CAMT_OUT_PATH "/data/vendor/cameraserver/testimage/"
extern map<string,vector<resultData_t>*> gMap_Result;

struct camt_drv_context {
	int fd;
	struct camt_info info;
};

static int camt_read_file(char *file_name, unsigned int size, unsigned long addr)
{
	int ret = 0;
	FILE *fp = NULL;

	IT_LOGD("file_name:%s size %d vir_addr %ld", file_name, size, addr);
	fp = fopen(file_name, "rb");
	if (NULL == fp) {
		IT_LOGD("can not open file: %s \n", file_name);
		return -1;
	}

	ret = fread((void *)addr, 1, size, fp);
	fclose(fp);
	return ret;
}

static int camt_write_file(char *file_name, unsigned int size, unsigned long addr)
{
	int ret = 0;
	FILE *fp = NULL;

	IT_LOGD("file_name:%s size %d vir_addr %ld", file_name, size, addr);
	fp = fopen(file_name, "wb");
	if (NULL == fp) {
		IT_LOGD("can not open file: %s \n", file_name);
		return -1;
	}

	ret = fwrite((void *)addr, 1, size, fp);
	fclose(fp);
	return ret;
}

static int camt_check_json_param(DrvCaseComm *_json2)
{
	int ret = 0;

	if (!_json2) {
		IT_LOGE("fail to get invalid jason ptr %p", _json2);
		return -1;
	}

	IT_LOGD("case id= %d",_json2->getID());
	if (_json2->m_chipID > CAMT_CHIP_MAX) {
		IT_LOGE("fail to get invalid chipid %d", _json2->m_chipID);
		return -1;
	}

	if (!_json2->g_host_info) {
		IT_LOGE("fail to get g_params");
		return -1;
	}

	return ret;
}

ModuleWrapperDRV::ModuleWrapperDRV()
{
    IT_LOGD("in ModuleWrapperDRV");
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
	struct camt_drv_context cxt;
	DrvCaseComm *_json2=(DrvCaseComm *)Json2;
	ISP_PARAM_T *param = NULL;
	struct test_camera_mem_type_stat type;
	unsigned int size, sum;
	Test_Mem_alloc *pMemAlloc = new Test_Mem_alloc();
	int mfd[2];
	unsigned long phy_addr[2];
	unsigned long vir_addr[2];
	char output_file[256];
	char tmp_str[10];

	memset(output_file, '\0', 256);
	memset(tmp_str, '\0', 10);
	memset(&cxt, 0, sizeof(cxt));
	/* open camera device */
	cxt.fd = open(CAMT_DEV_NAME, O_RDWR, 0);
	if (cxt.fd == -1) {
		IT_LOGE("fail to open dcam devic, errno: %d", errno);
		return -1;
	}

	/* check json param */
	ret = camt_check_json_param(_json2);
	if (ret) {
		IT_LOGE("fail to get invalid jason param");
		return -1;
	}

	strcpy(output_file, CAMT_OUT_PATH);
	strcat(output_file, "output_");
	sprintf(tmp_str, "%d", _json2->m_caseID);
	strcat(output_file, tmp_str);
	strcat(output_file, ".raw");
	param = &_json2->g_host_info->isp_param;
	cxt.info.chip = (enum camt_chip)_json2->m_chipID;
	cxt.info.path_id = _json2->m_pathID;
	cxt.info.test_mode = _json2->m_testMode;
	cxt.info.bayer_mode = param->general_info.general_image_bayer_mode;
	cxt.info.input_size.w = param->general_info.general_image_width;
	cxt.info.input_size.h = param->general_info.general_image_height;
	cxt.info.crop_rect.x = param->fetch_info.general_crop_start_x;
	cxt.info.crop_rect.y = param->fetch_info.general_crop_start_y;
	cxt.info.crop_rect.w = param->fetch_info.general_crop_width;
	cxt.info.crop_rect.h = param->fetch_info.general_crop_height;

	/* output size should cal by zoom ratio or from json file
	* now just keep the same with crop size. TBD
	*/
	cxt.info.output_size.w = cxt.info.crop_rect.w;
	cxt.info.output_size.h = cxt.info.crop_rect.h;

	IT_LOGD(" in %d %d crop %d %d %d %d out %d %d", cxt.info.input_size.w,
		cxt.info.input_size.h, cxt.info.crop_rect.x, cxt.info.crop_rect.y,
		cxt.info.crop_rect.w, cxt.info.crop_rect.h, cxt.info.output_size.w,
		cxt.info.output_size.h);

	/* camt init */
	cxt.info.cmd = CAMT_CMD_INIT;
	ret = ioctl(cxt.fd, SPRD_IMG_IO_CAM_TEST, &cxt.info);
	if (ret < 0) {
		IT_LOGE("failed to init cam drv test");
		goto exit;
	}

	/* malloc buffer, get frame */
	type.is_cache = false;
	type.mem_type = TEST_CAMERA_CAPTURE;
	/* alloc buf size should sync with format
	* now just *2 for current use
	*/
	size = cxt.info.input_size.w * cxt.info.input_size.h * 2;
	sum = 2;

	pMemAlloc->Test_Callback_IonMalloc(type, &size, &sum,
		&phy_addr[0], &vir_addr[0], &mfd[0], NULL);
	IT_LOGD("mfd %d %d", mfd[0], mfd[1]);

	ret = camt_read_file(_json2->m_imageName.data(), size, vir_addr[0]);
	if (ret < 0) {
		IT_LOGE("failed to read input file");
		goto exit;
	}

	/* start camt */
	cxt.info.cmd = CAMT_CMD_START;
	cxt.info.inbuf_fd = mfd[0];
	cxt.info.outbuf_fd = mfd[1];
	ret = ioctl(cxt.fd, SPRD_IMG_IO_CAM_TEST, &cxt.info);
	if (ret < 0) {
		IT_LOGE("failed to start cam drv test");
		goto start_fail;
	}

	ret = camt_write_file(output_file, size, vir_addr[1]);
	if (ret < 0) {
		IT_LOGE("failed to write output file");
		goto exit;
	}

	cxt.info.cmd = CAMT_CMD_STOP;
	ret = ioctl(cxt.fd, SPRD_IMG_IO_CAM_TEST, &cxt.info);
	if (ret < 0)
		IT_LOGE("failed to stop cam drv test");

start_fail:
	pMemAlloc->Test_Callback_Free(type, &phy_addr[0], &vir_addr[0], &mfd[0], sum, NULL);
	delete pMemAlloc;
	cxt.info.cmd = CAMT_CMD_DEINIT;
	ioctl(cxt.fd, SPRD_IMG_IO_CAM_TEST, &cxt.info);
	ret = close(cxt.fd);

exit:
	IT_LOGD("end %d", ret);
	return ret;
}

int ModuleWrapperDRV::InitOps()
{
    int ret = IT_OK;
    IT_LOGD("");
    return ret;
}
