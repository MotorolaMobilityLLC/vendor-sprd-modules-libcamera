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
	fp = NULL;
	return ret;
}

static int camt_write_file(char *file_name, unsigned int chipid, size_t width, size_t height, struct img_store_addr addr)
{
	int ret = 0;
	FILE *fp = NULL;
	char temp[20];
	char filename1[256];

	if (chipid == CAMT_CHIP_ISP0 || chipid == CAMT_CHIP_ISP1) {
		IT_LOGD("file_name:%s size %lu vir_addr %ld_isp", file_name, width * height, addr.addr_ch0);
		strncat(file_name, "_yuv", 4);
		sprintf(temp, "_%lu_%lu", width, height);
		strncat(file_name, temp, 20);
		strncpy(filename1, file_name, 256);
		strncat(file_name, ".y", 2);
		fp = fopen(file_name, "wb");
		if (fp == NULL) {
			IT_LOGD("can not open file: %s \n", file_name);
			return -1;
		}
		ret = fwrite((void *)addr.addr_ch0, 1, width * height , fp);
		if (ret != 1) {
			IT_LOGD("Y Write Error!\n");
			fclose(fp);
			fp = NULL;
			return -1;
		}
		fclose(fp);
		fp = NULL;
		strncat(filename1, ".uv", 3);
		fp = fopen(filename1, "wb");
		if (fp == NULL) {
			IT_LOGD("can not open file: %s \n", file_name);
			return -1;
		}
		ret = fwrite((void *)addr.addr_ch1, 1, width * height / 2, fp);
		if (ret != 1) {
			IT_LOGD("UV Write Error!\n");
			fclose(fp);
			fp = NULL;
			return -1;
		}
		fclose(fp);
		fp = NULL;
	} else if (CAMT_CHIP_DCAM0 <= chipid && chipid <= CAMT_CHIP_DCAM_LITE1) {
		IT_LOGD("file_name:%s size %lu vir_addr %ld_dcam", file_name, width * height, addr.addr_ch0);
		sprintf(temp, "_%lu_%lu", width, height);
		strncat(file_name, temp, 4);
		strncat(file_name, ".mipi_raw", 9);
		fp = fopen(file_name, "wb");
		if (fp == NULL) {
			IT_LOGD("can not open file: %s \n", file_name);
			return -1;
		}
		ret = fwrite((void *)addr.addr_ch0, 1, width * height, fp);
		if (ret != 1) {
			IT_LOGD("Mipi_Raw Write Error!\n");
			fclose(fp);
			fp = NULL;
			return -1;
		}
		fclose(fp);
		fp = NULL;
	}
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
	int i;
	struct camt_drv_context cxt;
	DrvCaseComm *_json2=(DrvCaseComm *)Json2;
	ISP_PARAM_T *param = NULL;
	/*struct test_camera_mem_type_stat type;*/
	unsigned int size, sum_in, sum_out;
	int mfd_in, fd;;
	unsigned long phy_addr_in, phys_addr;
	unsigned long vir_addr_in;
	int mfd_out[DRV_PATH_NUM];
	unsigned long phy_addr_out[DRV_PATH_NUM];
	unsigned long vir_addr_out[DRV_PATH_NUM];
	char output_file[DRV_PATH_NUM][256];
	char tmp_str[10];
	struct img_store_addr store_addr_out[DRV_PATH_NUM] = {0};
	ion_info_t ion_in;
	ion_info_t ion_out;
	vector<ion_info_t> ion_config_in;
	vector<ion_info_t> ion_config_out;
	int tag_in;
	int tag_out;
	bufferData buf_data_in;
	bufferData buf_data_out[DRV_PATH_NUM];

	memset(output_file, '\0', sizeof(output_file));
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

	for(i = 0; i < _json2->m_pathID.size(); i++) {
		strncpy(output_file[i], CAMT_OUT_PATH, 36);
		strncat(output_file[i], "output_", 7);
		sprintf(tmp_str, "%d_%d", _json2->m_caseID,_json2->m_pathID[i]);
		strncat(output_file[i], tmp_str, 10);
	}

	param = &_json2->g_host_info->isp_param;
	cxt.info.chip = (enum camt_chip)_json2->m_chipID;
	for(i = 0; i < _json2->m_pathID.size(); i++)
	cxt.info.path_id[i] = _json2->m_pathID[i];
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

	/* malloc buffer, get frame */
	/* alloc buf size should sync with format
	* now just *2 for current use
	*/
	size = cxt.info.input_size.w * cxt.info.input_size.h * 2;
	IT_LOGD("size: %d", size);
	sum_in = sizeof(mfd_in) / sizeof(int);
	ion_in.buf_size = size;
	ion_in.num_bufs = sum_in;
	ion_in.is_cache = false;
	ion_in.tag = tag_in;
	ion_config_in.push_back(ion_in);

	sum_out = sizeof(mfd_out) / sizeof(int);
	ion_out.buf_size = size;
	ion_out.num_bufs = sum_out;
	ion_out.is_cache = false;
	ion_out.tag = tag_out;
	ion_config_out.push_back(ion_out);

	TestMemPool *pMemAlloc_in = new TestMemPool(ion_config_in);
	TestMemPool *pMemAlloc_out = new TestMemPool(ion_config_out);

	buf_data_in = pMemAlloc_in->popBufferList(tag_in);
	phy_addr_in = buf_data_in.ion_buffer->phys_addr;
	vir_addr_in = (unsigned long)(buf_data_in.ion_buffer->virs_addr);
	mfd_in = buf_data_in.ion_buffer->fd;

	IT_LOGD("mfd_in: %d", mfd_in);
	for(i = 0; i < DRV_PATH_NUM; i++){
		buf_data_out[i] = pMemAlloc_out->popBufferList(tag_out);
		phy_addr_out[i] = buf_data_out[i].ion_buffer->phys_addr;
		vir_addr_out[i] = (unsigned long)(buf_data_out[i].ion_buffer->virs_addr);
		mfd_out[i] = buf_data_out[i].ion_buffer->fd;
		IT_LOGD("mfd_out[%d]: %d", i, mfd_out[i]);
	}

	/* camt init */
	cxt.info.cmd = CAMT_CMD_INIT;
	ret = ioctl(cxt.fd, SPRD_IMG_IO_CAM_TEST, &cxt.info);
	if (ret < 0) {
		IT_LOGE("failed to init cam drv test");
		goto exit;
	}

	ret = camt_read_file(_json2->m_imageName.data(), size, vir_addr_in);
	if (ret < 0) {
		IT_LOGE("failed to read input file");
		goto start_fail;
	}

	/* start camt */
	cxt.info.cmd = CAMT_CMD_START;
	cxt.info.inbuf_fd = mfd_in;
	for(i = 0; i < DRV_PATH_NUM; i++)
		cxt.info.outbuf_fd[i] = mfd_out[i];
	ret = ioctl(cxt.fd, SPRD_IMG_IO_CAM_TEST, &cxt.info);
	if (ret < 0) {
		IT_LOGE("failed to start cam drv test");
		goto start_fail;
	}

	for(i = 0; i < DRV_PATH_NUM; i++) {
		if (cxt.info.chip == CAMT_CHIP_ISP0 || cxt.info.chip == CAMT_CHIP_ISP1) {
			store_addr_out[i].addr_ch0 = vir_addr_out[i];
			store_addr_out[i].addr_ch1 = vir_addr_out[i] + size / 2;
			strncat(output_file[i], "_isp", 4);
			IT_LOGD("isp_module");
		} else if (CAMT_CHIP_DCAM0 <= cxt.info.chip && cxt.info.chip <= CAMT_CHIP_DCAM_LITE1) {
			store_addr_out[i].addr_ch0 = vir_addr_out[i];
			strncat(output_file[i], "_dcam", 5);
			IT_LOGD("dcam_module");
		} else {
			IT_LOGD("error set ChipID,Please Check it!!!");
			return IT_ERR;
		}
		ret = camt_write_file(output_file[i], cxt.info.chip, cxt.info.input_size.w, cxt.info.input_size.h, store_addr_out[i]);
	}

	if (ret < 0) {
		IT_LOGE("failed to write output file");
		goto start_fail;
	}

	cxt.info.cmd = CAMT_CMD_STOP;
	ret = ioctl(cxt.fd, SPRD_IMG_IO_CAM_TEST, &cxt.info);
	if (ret < 0)
		IT_LOGE("failed to stop cam drv test");

start_fail:
	cxt.info.cmd = CAMT_CMD_DEINIT;
	ioctl(cxt.fd, SPRD_IMG_IO_CAM_TEST, &cxt.info);

exit:
	delete pMemAlloc_in;
	delete pMemAlloc_out;
	ret = close(cxt.fd);
	IT_LOGD("end %d", ret);
	return ret;
}

int ModuleWrapperDRV::InitOps()
{
    int ret = IT_OK;
    IT_LOGD("");
    return ret;
}
