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

#define LOG_TAG "cpp_u_dev"
#include <stdio.h>
#include "cpp_u_dev.h"
#include "sprd_cpp.h"
#include "slice_drv.h"
#include "cpp_u_slice.h"
#include <string.h>
#include <time.h>
#include <cutils/properties.h>

static char cpp_dev_name[50] = "/dev/sprd_cpp";
#define CPP_DUMP_PATH "/root/data/like/pic/"

#define ERR(x...) fprintf(stderr, x)
#define INFO(x...) fprintf(stdout, x)

static int cpp_save_cpp_to_file(struct cpp_scale_param *scale_param);

static void cpp_trace_slice_param(struct sprd_cpp_scale_cfg_parm* param)
{
	unsigned int i = 0;
	struct sprd_cpp_hw_slice_parm *slice_ptr;
	INFO("Trace Slice parameters:\n");
	INFO("Slice count:%d\n",param->slice_param_1.output.slice_count);
	for(i = 0; i < param->slice_param_1.output.slice_count;i++){
		INFO("Slice %d:\n",i);
		slice_ptr = &param->slice_param_1.output.hw_slice_param[i];
		INFO("Input : [%d %d %d] [%d %d]\n",
			slice_ptr->path0_src_width, slice_ptr->path0_src_height,
			slice_ptr->path0_src_pitch,
			slice_ptr->path0_src_offset_x,slice_ptr->path0_src_offset_y);
		INFO("Slice [full in:%d %d] [full out:%d %d] \
			[slice in:%d %d ] [slice out:%d %d]\n",
			slice_ptr->sc_full_in_height,slice_ptr->sc_full_in_width,
			slice_ptr->sc_full_out_height,slice_ptr->sc_full_out_width,
			slice_ptr->sc_slice_in_height,slice_ptr->sc_slice_in_width,
			slice_ptr->sc_slice_out_height,slice_ptr->sc_slice_out_width);
		INFO("SC out:[%d %d %d] [%d %d]\n",
			slice_ptr->path0_sc_des_width, slice_ptr->path0_sc_des_height,
			slice_ptr->path0_sc_des_pitch,
			slice_ptr->path0_sc_des_offset_x,slice_ptr->path0_sc_des_offset_y);
	}
}

cmr_int cpp_rot_open(cmr_handle *handle)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;
	struct cpp_rot_file *file = NULL;
	cmr_int fd = -1;
	cmr_u32 val = 1;

	file = malloc(sizeof(struct cpp_rot_file));
	if (!file) {
		ret = -CMR_CAMERA_FAIL;
		goto rot_o_out;
	}

	fd = open(cpp_dev_name, O_RDWR, 0);
	if (fd < 0) {
		CMR_LOGE("Fail to open rotation device.\n");
		goto rot_o_free;
	}

	ret = ioctl(fd, SPRD_CPP_IO_OPEN_ROT, &val);
	if (ret) {
		CMR_LOGE("Fail to send SPRD_CPP_IO_OPEN_ROT.\n");
		goto rot_o_free;
	}

	file->fd = fd;
	*handle = (cmr_handle)file;
	goto rot_o_out;

rot_o_free:
	if (fd >= 0) {
		close(fd);
		fd = -1;
	}
	if (file)
		free(file);
	file = NULL;
rot_o_out:

	return ret;
}

cmr_int cpp_rot_close(cmr_handle handle)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;
	struct cpp_rot_file *file = (struct cpp_rot_file *)(handle);

	if (!file)
		goto rot_c_out;

	if (file->fd == -1) {
		CMR_LOGE("Invalid fd\n");
		ret = -CMR_CAMERA_FAIL;
		goto rot_c_free;
	}

	close(file->fd);

rot_c_free:
	free(file);

rot_c_out:
	CMR_LOGD("ret=%ld\n", ret);
	return ret;
}

cmr_int cpp_rot_start(struct cpp_rot_param *rot_param)
{
	struct sprd_cpp_rot_cfg_parm *rot_cfg = NULL;
	cmr_int ret = CMR_CAMERA_SUCCESS;
	cmr_int fd = -1;
	struct cpp_rot_file *file = NULL;

	if (!rot_param) {
		CMR_LOGE("Invalid Param!\n");
		ret = -CMR_CAMERA_FAIL;
		goto rot_s_exit;
	}

	rot_cfg = rot_param->rot_cfg_param;
	if (rot_param->host_fd > 0)
		fd = rot_param->host_fd;
	else {
		file = (struct cpp_rot_file *)rot_param->handle;
		if (!file) {
			CMR_LOGE("Invalid Param rot_file !\n");
			ret = -CMR_CAMERA_FAIL;
			goto rot_s_exit;
		}

		fd = file->fd;
		if (fd < 0) {
			CMR_LOGE("Invalid Param handle!\n");
			ret = -CMR_CAMERA_FAIL;
			goto rot_s_exit;
		}
	}

	ret = ioctl(fd, SPRD_CPP_IO_START_ROT, rot_cfg);
	if (ret) {
		CMR_LOGE("start rot fail. ret = %ld\n", ret);
		ret = -CMR_CAMERA_FAIL;
		goto rot_s_exit;
	}

rot_s_exit:

	CMR_LOGD("rot X ret=%ld", ret);
	return ret;
}

cmr_int cpp_scale_open(cmr_handle *handle)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;
	cmr_int fd = -1;
	struct sc_file *file = NULL;
	cmr_u32 val = 1;

	file = malloc(sizeof(struct sc_file));
	if (!file) {
		CMR_LOGE("scale error: no memory for file\n");
		ret = CMR_CAMERA_NO_MEM;
		goto sc_o_out;
	}

	fd = open(cpp_dev_name, O_RDWR, 0);
	if (fd < 0) {
		CMR_LOGE("Fail to open scale device.\n");
		goto sc_o_free;
	}

	ret = ioctl(fd, SPRD_CPP_IO_OPEN_SCALE, &val);
	if (ret)
		goto sc_o_free;

	file->fd = fd;
	*handle = (cmr_handle)file;

	goto sc_o_out;

sc_o_free:
	if (fd >= 0) {
		close(fd);
		fd = -1;
	}
	if (file)
		free(file);
	file = NULL;
sc_o_out:

	return ret;
}

cmr_int cpp_scale_close(cmr_handle handle)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;
	struct sc_file *file = (struct sc_file *)(handle);

	if (!file) {
		CMR_LOGE("scale fail: file hand is null\n");
		ret = CMR_CAMERA_INVALID_PARAM;
		goto sc_c_out;
	}

	if (-1 == file->fd) {
		CMR_LOGE("Invalid fd\n");
		ret = -CMR_CAMERA_FAIL;
		goto sc_c_free;
	}

	close(file->fd);

sc_c_free:
	free(file);

sc_c_out:
	CMR_LOGI("scale close device exit\n");

	return ret;
}

cmr_int cpp_scale_start(struct cpp_scale_param *scale_param)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;
	cmr_int fd = -1;
	struct sprd_cpp_scale_cfg_parm *sc_cfg = NULL;
	struct sc_file *file = NULL;
	struct sprd_cpp_scale_slice_parm *slice_parm = NULL;

	if (!scale_param || !scale_param->scale_cfg_param) {
		CMR_LOGE("Invalid Param!\n");
		ret = -CMR_CAMERA_FAIL;
		goto sc_exit;
	}

	sc_cfg = scale_param->scale_cfg_param;
	slice_parm = &scale_param->scale_cfg_param->slice_param_1;
	if (scale_param->host_fd > 0)
		fd = scale_param->host_fd;
	else {
		file = (struct sc_file *)scale_param->handle;
		if (!file) {
			CMR_LOGE("Invalid Param sc_file !\n");
			ret = -CMR_CAMERA_FAIL;
			goto sc_exit;
		}
		fd = file->fd;
		if (fd < 0) {
			CMR_LOGE("Invalid Param handle!\n");
			ret = -CMR_CAMERA_FAIL;
			goto sc_exit;
		}
	}
	ret = cpp_u_input_param_check(sc_cfg);
	if (ret) {
		CMR_LOGE("Invalid input Param from cmr_scale!!\n");
		ret = -CMR_CAMERA_FAIL;
		goto sc_exit;
	}
	convert_param_to_calc(sc_cfg, slice_parm);
	slice_drv_param_calc(slice_parm);
	cpp_trace_slice_param(sc_cfg);
	ret = ioctl(fd, SPRD_CPP_IO_START_SCALE, sc_cfg);
	if (ret) {
		CMR_LOGE("scale done error\n");
		goto sc_exit;
	}
	cpp_save_cpp_to_file(scale_param);

sc_exit:
	CMR_LOGI("scale X ret=%ld\n", ret);
	return ret;
}

cmr_int cpp_dma_open(cmr_handle *handle)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;
	struct dma_file *file = NULL;
	cmr_int fd = -1;
	cmr_u32 val = 1;

	file = malloc(sizeof(struct dma_file));
	if (!file) {
		CMR_LOGE("Fail to malloc dma file.\n");
		ret = -CMR_CAMERA_FAIL;
		goto dma_o_out;
	}

	fd = open(cpp_dev_name, O_RDWR, 0);
	if (fd < 0) {
		CMR_LOGE("Fail to open dma device.\n");
		goto dma_o_free;
	}

	ret = ioctl(fd, SPRD_CPP_IO_OPEN_DMA, &val);
	if (ret) {
		CMR_LOGE("Fail to send SPRD_CPP_IO_OPEN_DMA.\n");
		goto dma_o_free;
	}

	file->fd = fd;
	*handle = (cmr_handle)file;
	goto dma_o_out;

dma_o_free:
	if (fd >= 0) {
		close(fd);
		fd = -1;
	}
	if (file)
		free(file);
	
	file = NULL;
dma_o_out:
	
	return ret;
}

cmr_int cpp_dma_close(cmr_handle handle)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;
	struct dma_file *file = (struct dma_file *)(handle);

	if (!file) {
		CMR_LOGI("dma fail: file hand is null\n");
		ret = CMR_CAMERA_INVALID_PARAM;
		goto dma_c_out;
	}

	if (-1 == file->fd) {
		CMR_LOGE("Invalid fd\n");
		ret = -CMR_CAMERA_FAIL;
		goto dma_c_free;
	}

	close(file->fd);

dma_c_free:
	free(file);

dma_c_out:
	CMR_LOGI("dma close device exit\n");

	return ret;
}

cmr_int cpp_dma_start(struct cpp_dma_param *dma_param)
{
	struct sprd_cpp_dma_cfg_parm *dma_cfg = NULL;
	cmr_int ret = CMR_CAMERA_SUCCESS;
	cmr_int fd = -1;
	struct dma_file *file = NULL;

	if (!dma_param) {
		CMR_LOGE("Invalid Param!\n");
		ret = -CMR_CAMERA_FAIL;
		goto dma_s_exit;
	}

	dma_cfg = dma_param->dma_cfg_param;
	if (dma_param->host_fd > 0)
		fd = dma_param->host_fd;
	else {
		file = (struct dma_file *)dma_param->handle;
		if (!file) {
			CMR_LOGE("Invalid Param dma_file !\n");
			ret = -CMR_CAMERA_FAIL;
			goto dma_s_exit;
		}

		fd = file->fd;
		if (fd < 0) {
			CMR_LOGE("Invalid Param handle!\n");
			ret = -CMR_CAMERA_FAIL;
			goto dma_s_exit;
		}
	}

	ret = ioctl(fd, SPRD_CPP_IO_START_DMA, dma_cfg);
	if (ret) {
		CMR_LOGE("start dma fail. ret = %ld\n", ret);
		ret = -CMR_CAMERA_FAIL;
		goto dma_s_exit;
	}

dma_s_exit:

	CMR_LOGI("dma X ret=%ld", ret);
	return ret;
}

static int cpp_save_cpp_to_file(struct cpp_scale_param *scale_param)
{

	cmr_int ret = CMR_CAMERA_FAIL;
	char value[PROPERTY_VALUE_MAX];
	struct sprd_cpp_scale_cfg_parm *scale_cfg_param = NULL;
	char file_name[0x40];
	char tmp_str[10];
	FILE *fp = NULL;

        scale_cfg_param = scale_param->scale_cfg_param;
            
        property_get("debug.cpp.dump.frame", value, "null");
        CMR_LOGD("value %s format %d width %d height %d, addr 0x%x 0x%x",
            value,
             scale_cfg_param->input_format,
             scale_cfg_param->input_size.w,
             scale_cfg_param->input_size.h,
             scale_cfg_param->input_addr.y,
             scale_cfg_param->input_addr.u);
            
        if (!strcmp(value, "src")) {
            //Y
                if (IMG_DATA_TYPE_YUV420 == scale_cfg_param->input_format ||
                    IMG_DATA_TYPE_YUV422 == scale_cfg_param->input_format||
                    IMG_DATA_TYPE_YVU420 == scale_cfg_param->input_format) {
                    cmr_bzero(file_name, 0x40);
                    strcpy(file_name, CPP_DUMP_PATH);
                    sprintf(tmp_str, "%s_", value);
                    strcat(file_name, tmp_str);
                    sprintf(tmp_str, "%d", scale_cfg_param->input_size.w);
                    strcat(file_name, tmp_str);
                    strcat(file_name, "X");
                    sprintf(tmp_str, "%d", scale_cfg_param->input_size.h);
                    strcat(file_name, tmp_str);
                    strcat(file_name, "_y");
                    strcat(file_name, ".raw");
                    CMR_LOGD("file name %s", file_name);
                    fp = fopen(file_name, "wb");
                    if (NULL == fp) {
                        CMR_LOGE("can not open file: %s \n", file_name);
                        return ret;
                    }
                    fwrite((void *)(long)scale_cfg_param->input_addr_vir.y, 1,
                        scale_cfg_param->input_size.w * scale_cfg_param->input_size.h, fp);
                    fclose(fp);
            //UV
                    bzero(file_name, 40);
                    strcpy(file_name, CPP_DUMP_PATH);
                     sprintf(tmp_str, "%s_", value);
                    strcat(file_name, tmp_str);
                    sprintf(tmp_str, "%d", scale_cfg_param->input_size.w);
                    strcat(file_name, tmp_str);
                    strcat(file_name, "X");
                    sprintf(tmp_str, "%d", scale_cfg_param->input_size.h);
                    strcat(file_name, tmp_str);
                    strcat(file_name, "_uv");
                    strcat(file_name, ".raw");
                    CMR_LOGD("file name %s", file_name);
                    fp = fopen(file_name, "wb");
                    if (NULL == fp) {
                        CMR_LOGE("can not open file: %s \n", file_name);
                        return ret;
                    }

                    if (IMG_DATA_TYPE_YUV420 == scale_cfg_param->input_format ||
                        IMG_DATA_TYPE_YVU420 == scale_cfg_param->input_format ) {
                        fwrite((void *)(long)scale_cfg_param->input_addr_vir.u, 1,
                            scale_cfg_param->input_size.w * scale_cfg_param->input_size.h / 2, fp);
                    } else {
                        fwrite((void *)(long)scale_cfg_param->input_addr_vir.u, 1,
                            scale_cfg_param->input_size.w * scale_cfg_param->input_size.h, fp);
                    }
                    fclose(fp);
                } 
            }else if (!strcmp(value, "dst")) {
                        //Y

                if (IMG_DATA_TYPE_YUV420 == scale_cfg_param->output_format ||
                    IMG_DATA_TYPE_YUV422 == scale_cfg_param->output_format||
                    IMG_DATA_TYPE_YVU420 == scale_cfg_param->output_format) {
                    cmr_bzero(file_name, 0x40);
                    strcpy(file_name, CPP_DUMP_PATH);
                    sprintf(tmp_str, "%s_", value);
                    strcat(file_name, tmp_str);
                    sprintf(tmp_str, "%d", scale_cfg_param->output_size.w);
                    strcat(file_name, tmp_str);
                    strcat(file_name, "X");
                    sprintf(tmp_str, "%d", scale_cfg_param->output_size.h);
                    strcat(file_name, tmp_str);
                    strcat(file_name, "_y");
                    strcat(file_name, ".raw");
                    CMR_LOGD("file name %s", file_name);
                    fp = fopen(file_name, "wb");
                    if (NULL == fp) {
                        CMR_LOGE("can not open file: %s \n", file_name);
                        return ret;
                    }
                    fwrite((void *)(long)scale_cfg_param->output_addr_vir.y, 1,
                        scale_cfg_param->output_size.w * scale_cfg_param->output_size.h, fp);
                    fclose(fp);
            //UV
                    bzero(file_name, 40);
                    strcpy(file_name, CPP_DUMP_PATH);
                     sprintf(tmp_str, "%s_", value);
                    strcat(file_name, tmp_str);
                    sprintf(tmp_str, "%d", scale_cfg_param->output_size.w);
                    strcat(file_name, tmp_str);
                    strcat(file_name, "X");
                    sprintf(tmp_str, "%d", scale_cfg_param->output_size.h);
                    strcat(file_name, tmp_str);
                    strcat(file_name, "_uv");
                    strcat(file_name, ".raw");
                    CMR_LOGD("file name %s", file_name);
                    fp = fopen(file_name, "wb");
                    if (NULL == fp) {
                        CMR_LOGE("can not open file: %s \n", file_name);
                        return ret;
                    }

                    if (IMG_DATA_TYPE_YUV420 == scale_cfg_param->output_format ||
                        IMG_DATA_TYPE_YVU420 == scale_cfg_param->output_format ) {
                        fwrite((void *)(long)scale_cfg_param->output_addr_vir.u, 1,
                            scale_cfg_param->output_size.w * scale_cfg_param->output_size.h / 2, fp);
                    } else {
                        fwrite((void *)(long)scale_cfg_param->output_addr_vir.u, 1,
                            scale_cfg_param->output_size.w * scale_cfg_param->output_size.h, fp);
                    }
                    fclose(fp);
                } 
                } else {
                    CMR_LOGD("need not dump cpp frame \n");
                }
                return 0;
}
