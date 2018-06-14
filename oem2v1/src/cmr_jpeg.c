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

#include "exif_writer.h"
#include "cmr_jpeg.h"
#include <dlfcn.h>

#ifndef LOG_TAG
#define LOG_TAG "cmr_jpeg"
#endif

cmr_int cmr_jpeg_init(cmr_handle oem_handle, cmr_handle *jpeg_handle,
                      jpg_evt_cb_ptr adp_event_cb) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct jpeg_codec_caller_handle *codec_handle = NULL;
    struct jpeg_lib_cxt *jcxt = NULL;
    const char libName[] = "libjpeg_hw_sprd.so";

    if (!jpeg_handle) {
        CMR_LOGE("Invalid Param!");
        return CMR_CAMERA_INVALID_PARAM;
    }
    *jpeg_handle = 0;

    jcxt = (struct jpeg_lib_cxt *)malloc(sizeof(struct jpeg_lib_cxt));
    if(!jcxt) {
        CMR_LOGE("No mem!\n");
        return CMR_CAMERA_NO_MEM;
    }
    codec_handle = (struct jpeg_codec_caller_handle *)malloc(
        sizeof(struct jpeg_codec_caller_handle));
    if(!codec_handle) {
        free(jcxt);
        jcxt = NULL;
        CMR_LOGE("No mem!\n");
        return CMR_CAMERA_NO_MEM;
    }
    codec_handle->reserved = oem_handle;
    jcxt->codec_handle = codec_handle;
    jcxt->mLibHandle = dlopen(libName, RTLD_NOW);

    if (jcxt->mLibHandle == NULL) {
        CMR_LOGE("can't open lib: %s", libName);
        ret = -CMR_CAMERA_NO_SUPPORT;
        goto exit_error_libhandle;
    } else
        CMR_LOGI(" open lib: %s \n", libName);

    jcxt->ops.jpeg_init = dlsym(jcxt->mLibHandle, "sprd_jpeg_init");
    if (jcxt->ops.jpeg_init == NULL) {
        CMR_LOGE("Can't find jpeg_init in %s", libName);
        ret = -CMR_CAMERA_NO_SUPPORT;
        goto exit_error;
    }

    jcxt->ops.jpeg_encode = dlsym(jcxt->mLibHandle, "sprd_jpg_encode");
    if (jcxt->ops.jpeg_encode == NULL) {
        CMR_LOGE("Can't find jpeg_enc_start in %s", libName);
        ret = -CMR_CAMERA_NO_SUPPORT;
        goto exit_error;
    }

    jcxt->ops.jpeg_decode = dlsym(jcxt->mLibHandle, "sprd_jpg_decode");
    if (jcxt->ops.jpeg_decode == NULL) {
        CMR_LOGE("Can't find jpeg_dec_start in %s", libName);
        ret = -CMR_CAMERA_NO_SUPPORT;
        goto exit_error;
    }

    jcxt->ops.jpeg_stop = dlsym(jcxt->mLibHandle, "sprd_stop_codec");
    if (jcxt->ops.jpeg_stop == NULL) {
        CMR_LOGE("Can't find jpeg_stop in %s", libName);
        ret = -CMR_CAMERA_NO_SUPPORT;
        goto exit_error;
    }

    jcxt->ops.jpeg_deinit = dlsym(jcxt->mLibHandle, "sprd_jpeg_deinit");
    if (jcxt->ops.jpeg_deinit == NULL) {
        CMR_LOGE("Can't find jpeg_deinit in %s", libName);
        ret = -CMR_CAMERA_NO_SUPPORT;
        goto exit_error;
    }

    jcxt->ops.jpeg_get_iommu_status =
        dlsym(jcxt->mLibHandle, "sprd_jpg_get_Iommu_status");
    if (jcxt->ops.jpeg_get_iommu_status == NULL) {
        CMR_LOGE("Can't find jpeg_get_Iommu_status in %s", libName);
        ret = -CMR_CAMERA_NO_SUPPORT;
        goto exit_error;
    }

    jcxt->ops.jpeg_dec_get_resolution =
        dlsym(jcxt->mLibHandle, "sprd_jpg_dec_get_resolution");
    if (jcxt->ops.jpeg_dec_get_resolution == NULL) {
        CMR_LOGE("Can't find jpeg_dec_get_resolution in %s", libName);
        ret = -CMR_CAMERA_NO_SUPPORT;
        goto exit_error;
    }

    jcxt->ops.jpeg_set_resolution =
        dlsym(jcxt->mLibHandle, "sprd_jpg_set_resolution");
    if (jcxt->ops.jpeg_set_resolution == NULL) {
        CMR_LOGE("Can't find sprd_jpg_set_resolution in %s", libName);
        ret = -CMR_CAMERA_NO_SUPPORT;
        goto exit_error;
    }

    ret = jcxt->ops.jpeg_init(codec_handle, adp_event_cb);
    if (ret) {
        CMR_LOGE("jpeg init fail");
        ret = CMR_CAMERA_FAIL;
    } else {
        goto pass;
    }
exit_error:
    dlclose(jcxt->mLibHandle);
    jcxt->mLibHandle = NULL;
exit_error_libhandle:
    free(codec_handle);
    codec_handle = NULL;
    free(jcxt);
    jcxt = NULL;
    return ret;
pass:
    *jpeg_handle = (cmr_handle)jcxt;

    CMR_LOGD("ret %ld", ret);
    return ret;
}

cmr_int cmr_jpeg_encode(cmr_handle jpeg_handle, struct img_frm *src,
                        struct img_frm *dst, struct jpg_op_mean *mean, struct jpeg_enc_cb_param *enc_cb_param) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct jpeg_codec_caller_handle *codec_handle = NULL;
    struct jpeg_lib_cxt *jcxt = NULL;

    if (!jpeg_handle) {
        CMR_LOGE("Invalid Param!");
        return CMR_CAMERA_INVALID_PARAM;
    }
    jcxt = (struct jpeg_lib_cxt *)jpeg_handle;

    ret = jcxt->ops.jpeg_encode(jcxt->codec_handle, (struct yuvbuf_frm *)src,
                                (struct yuvbuf_frm *)dst, mean, enc_cb_param);
    if (ret) {
        CMR_LOGE("jpeg encode error");
        return CMR_CAMERA_FAIL;
    }
    CMR_LOGD("ret %ld", ret);
    return ret;
}

cmr_int cmr_jpeg_decode(cmr_handle jpeg_handle, struct img_frm *src,
                        struct img_frm *dst, struct jpg_op_mean *mean) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct jpeg_codec_caller_handle *codec_handle = NULL;
    struct jpeg_lib_cxt *jcxt = NULL;

    if (!jpeg_handle) {
        CMR_LOGE("Invalid Param!");
        return CMR_CAMERA_INVALID_PARAM;
    }

    jcxt = (struct jpeg_lib_cxt *)jpeg_handle;

    ret = jcxt->ops.jpeg_decode(jcxt->codec_handle, (struct yuvbuf_frm *)src,
                                (struct yuvbuf_frm *)dst, mean);
    if (ret) {
        CMR_LOGE("jpeg encode error");
        return CMR_CAMERA_FAIL;
    }
    CMR_LOGD("ret %ld", ret);
    return ret;
}

static cmr_int _jpeg_enc_wexif(struct jpeg_enc_exif_param *param_ptr,
                               struct jpeg_wexif_cb_param *out_ptr) {
    cmr_int ret = JPEG_CODEC_SUCCESS;
    JINF_WEXIF_IN_PARAM_T input_param;
    JINF_WEXIF_OUT_PARAM_T output_param;

    input_param.exif_info_ptr = param_ptr->exif_ptr;
    input_param.src_jpeg_buf_ptr = (cmr_u8 *)param_ptr->src_jpeg_addr_virt;
    input_param.src_jpeg_size = param_ptr->src_jpeg_size;
    input_param.thumbnail_buf_ptr = (cmr_u8 *)param_ptr->thumbnail_addr_virt;
    input_param.thumbnail_buf_size = param_ptr->thumbnail_size;
    input_param.target_buf_ptr = (cmr_u8 *)param_ptr->target_addr_virt;
    input_param.target_buf_size = param_ptr->target_size;
    input_param.temp_buf_size =
        param_ptr->thumbnail_size + JPEG_WEXIF_TEMP_MARGIN;
    input_param.temp_buf_ptr = (cmr_u8 *)malloc(input_param.temp_buf_size);
    input_param.exif_isp_info = param_ptr->exif_isp_info;
    input_param.exif_isp_debug_info = param_ptr->exif_isp_debug_info;
    if (PNULL == input_param.temp_buf_ptr) {
        CMR_LOGE("jpeg:malloc temp buf for wexif fail.");
        return JPEG_CODEC_NO_MEM;
    }
    input_param.temp_exif_isp_buf_size = 4 * 1024;
    input_param.temp_exif_isp_buf_ptr =
        (uint8_t *)malloc(input_param.temp_exif_isp_buf_size);
    input_param.temp_exif_isp_dbg_buf_size = 4 * 1024;
    input_param.temp_exif_isp_dbg_buf_ptr =
        (uint8_t *)malloc(input_param.temp_exif_isp_dbg_buf_size);
    input_param.wrtie_file_func = NULL;
    if (PNULL == input_param.temp_exif_isp_buf_ptr) {
        free(input_param.temp_buf_ptr);
        return JPEG_CODEC_NO_MEM;
    }

    CMR_LOGI("jpeg:src jpeg addr 0x%p, size %d thumbnail addr 0x%p, size %d "
             "target addr 0x%p,size %d",
             input_param.src_jpeg_buf_ptr, input_param.src_jpeg_size,
             input_param.thumbnail_buf_ptr, input_param.thumbnail_buf_size,
             input_param.target_buf_ptr, input_param.target_buf_size);

    ret = IMGJPEG_WriteExif(&input_param, &output_param);

    out_ptr->output_buf_virt_addr = (cmr_uint)output_param.output_buf_ptr;
    out_ptr->output_buf_size = output_param.output_size;
    free(input_param.temp_buf_ptr);
    free(input_param.temp_exif_isp_buf_ptr);
    free(input_param.temp_exif_isp_dbg_buf_ptr);
    input_param.temp_buf_ptr = PNULL;
    CMR_LOGI("jpeg:output: addr 0x%lx,size %d", out_ptr->output_buf_virt_addr,
             (cmr_u32)out_ptr->output_buf_size);

    return ret;
}

cmr_int cmr_jpeg_enc_add_eixf(cmr_handle jpeg_handle,
                              struct jpeg_enc_exif_param *param_ptr,
                              struct jpeg_wexif_cb_param *output_ptr) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    if (!jpeg_handle || !param_ptr || !output_ptr) {
        CMR_LOGE("Invalid Param!");
        return CMR_CAMERA_INVALID_PARAM;
    }

    CMR_LOGI("jpeg:enc add exit start");
    ret = _jpeg_enc_wexif(param_ptr, output_ptr);
    if (ret) {
        CMR_LOGE("jpeg encode add eixf error");
        return CMR_CAMERA_FAIL;
    }

    CMR_LOGI("jpeg:output addr 0x%x,size %d",
             (cmr_u32)output_ptr->output_buf_virt_addr,
             (cmr_u32)output_ptr->output_buf_size);
    return ret;
}

cmr_int cmr_stop_codec(cmr_handle jpeg_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct jpeg_lib_cxt *jcxt = NULL;

    if (!jpeg_handle) {
        CMR_LOGE("Invalid Param!");
        return CMR_CAMERA_INVALID_PARAM;
    }

    jcxt = (struct jpeg_lib_cxt *)jpeg_handle;
    ret = jcxt->ops.jpeg_stop(jcxt->codec_handle);
    if (ret) {
        CMR_LOGE("stop codec error");
        return CMR_CAMERA_FAIL;
    }
    CMR_LOGD("ret %ld", ret);
    return ret;
}

cmr_int cmr_jpeg_deinit(cmr_handle jpeg_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct jpeg_lib_cxt *jcxt = NULL;

    if (!jpeg_handle) {
        CMR_LOGE("Invalid Param!");
        return CMR_CAMERA_INVALID_PARAM;
    }

    jcxt = (struct jpeg_lib_cxt *)jpeg_handle;
    ret = jcxt->ops.jpeg_deinit(jcxt->codec_handle);
    if (ret) {
        CMR_LOGE("jpeg deinit error");
        return CMR_CAMERA_FAIL;
    }
    if (jcxt->codec_handle) {
        free(jcxt->codec_handle);
        jcxt->codec_handle = NULL;
    }
    if (jcxt) {
        dlclose(jcxt->mLibHandle);
        jcxt->mLibHandle = NULL;
        free(jcxt);
        jcxt = NULL;
    }
    CMR_LOGD("ret %ld", ret);
    return ret;
}
