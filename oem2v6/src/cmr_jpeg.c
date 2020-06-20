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

#define LOG_TAG "cmr_jpeg"

#include "exif_writer.h"
#include "cmr_jpeg.h"
#include <dlfcn.h>
#include <libloader.h>

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
    if (!jcxt) {
        CMR_LOGE("No mem!\n");
        return CMR_CAMERA_NO_MEM;
    }
    codec_handle = (struct jpeg_codec_caller_handle *)malloc(
        sizeof(struct jpeg_codec_caller_handle));
    if (!codec_handle) {
        free(jcxt);
        jcxt = NULL;
        CMR_LOGE("No mem!\n");
        return CMR_CAMERA_NO_MEM;
    }
    codec_handle->reserved = oem_handle;
    jcxt->codec_handle = codec_handle;
#if USE_LEGACY_DLFCN
    jcxt->mLibHandle = dlopen(libName, RTLD_NOW);
#else
    jcxt->mLibHandle = get_lib_handle(libName);
#endif

    if (jcxt->mLibHandle == NULL) {
        CMR_LOGE("can't open lib: %s", libName);
        ret = -CMR_CAMERA_NO_SUPPORT;
        goto exit;
    }

    CMR_LOGI(" open lib: %s", libName);

    jcxt->ops.jpeg_init = dlsym(jcxt->mLibHandle, "sprd_jpeg_init");
    if (jcxt->ops.jpeg_init == NULL) {
        CMR_LOGE("Can't find jpeg_init in %s", libName);
#if USE_LEGACY_DLFCN
        dlclose(jcxt->mLibHandle);
#else
        put_lib_handle(jcxt->mLibHandle);
#endif
        jcxt->mLibHandle = NULL;
        ret = -CMR_CAMERA_NO_SUPPORT;
        goto exit;
    }

    jcxt->ops.jpeg_encode = dlsym(jcxt->mLibHandle, "sprd_jpg_encode");
    if (jcxt->ops.jpeg_encode == NULL) {
        CMR_LOGE("Can't find jpeg_enc_start in %s", libName);
#if USE_LEGACY_DLFCN
        dlclose(jcxt->mLibHandle);
#else
        put_lib_handle(jcxt->mLibHandle);
#endif
        jcxt->mLibHandle = NULL;
        ret = -CMR_CAMERA_NO_SUPPORT;
        goto exit;
    }

    jcxt->ops.jpeg_decode = dlsym(jcxt->mLibHandle, "sprd_jpg_decode");
    if (jcxt->ops.jpeg_decode == NULL) {
        CMR_LOGE("Can't find jpeg_dec_start in %s", libName);
#if USE_LEGACY_DLFCN
        dlclose(jcxt->mLibHandle);
#else
        put_lib_handle(jcxt->mLibHandle);
#endif
        jcxt->mLibHandle = NULL;
        ret = -CMR_CAMERA_NO_SUPPORT;
        goto exit;
    }

    jcxt->ops.jpeg_stop = dlsym(jcxt->mLibHandle, "sprd_stop_codec");
    if (jcxt->ops.jpeg_stop == NULL) {
        CMR_LOGE("Can't find jpeg_stop in %s", libName);
#if USE_LEGACY_DLFCN
        dlclose(jcxt->mLibHandle);
#else
        put_lib_handle(jcxt->mLibHandle);
#endif
        jcxt->mLibHandle = NULL;
        ret = -CMR_CAMERA_NO_SUPPORT;
        goto exit;
    }

    jcxt->ops.jpeg_deinit = dlsym(jcxt->mLibHandle, "sprd_jpeg_deinit");
    if (jcxt->ops.jpeg_deinit == NULL) {
        CMR_LOGE("Can't find jpeg_deinit in %s", libName);
#if USE_LEGACY_DLFCN
        dlclose(jcxt->mLibHandle);
#else
        put_lib_handle(jcxt->mLibHandle);
#endif
        jcxt->mLibHandle = NULL;
        ret = -CMR_CAMERA_NO_SUPPORT;
        goto exit;
    }

    jcxt->ops.jpeg_get_iommu_status =
        dlsym(jcxt->mLibHandle, "sprd_jpg_get_Iommu_status");
    if (jcxt->ops.jpeg_get_iommu_status == NULL) {
        CMR_LOGE("Can't find jpeg_get_Iommu_status in %s", libName);
#if USE_LEGACY_DLFCN
        dlclose(jcxt->mLibHandle);
#else
        put_lib_handle(jcxt->mLibHandle);
#endif
        jcxt->mLibHandle = NULL;
        ret = -CMR_CAMERA_NO_SUPPORT;
        goto exit;
    }

    jcxt->ops.jpeg_dec_get_resolution =
        dlsym(jcxt->mLibHandle, "sprd_jpg_dec_get_resolution");
    if (jcxt->ops.jpeg_dec_get_resolution == NULL) {
        CMR_LOGE("Can't find jpeg_dec_get_resolution in %s", libName);
#if USE_LEGACY_DLFCN
        dlclose(jcxt->mLibHandle);
#else
        put_lib_handle(jcxt->mLibHandle);
#endif
        jcxt->mLibHandle = NULL;
        ret = -CMR_CAMERA_NO_SUPPORT;
        goto exit;
    }

    jcxt->ops.jpeg_set_resolution =
        dlsym(jcxt->mLibHandle, "sprd_jpg_set_resolution");
    if (jcxt->ops.jpeg_set_resolution == NULL) {
        CMR_LOGE("Can't find sprd_jpg_set_resolution in %s", libName);
#if USE_LEGACY_DLFCN
        dlclose(jcxt->mLibHandle);
#else
        put_lib_handle(jcxt->mLibHandle);
#endif
        jcxt->mLibHandle = NULL;
        ret = -CMR_CAMERA_NO_SUPPORT;
        goto exit;
    }

    ret = jcxt->ops.jpeg_init(codec_handle, adp_event_cb);
    if (ret) {
        CMR_LOGE("jpeg init fail");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }
    *jpeg_handle = (cmr_handle)jcxt;

exit:
    if (ret) { // ret = 0 means jpeg_init success, buffer should not free
        if (codec_handle) {
            free(codec_handle);
            codec_handle = NULL;
        }
        if (jcxt) {
            free(jcxt);
            jcxt = NULL;
        }
    }

    CMR_LOGD("ret %d", ret);
    return ret;
}

cmr_int cmr_jpeg_encode(cmr_handle jpeg_handle, struct img_frm *src,
                        struct img_frm *dst, struct jpg_op_mean *mean,
                        struct jpeg_enc_cb_param *enc_cb_param) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct jpeg_codec_caller_handle *codec_handle = NULL;
    struct jpeg_lib_cxt *jcxt = NULL;
    struct yuvbuf_frm jpeg_src;
    struct yuvbuf_frm jpeg_dst;
    if (!jpeg_handle) {
        CMR_LOGE("Invalid Param!");
        return CMR_CAMERA_INVALID_PARAM;
    }
    jcxt = (struct jpeg_lib_cxt *)jpeg_handle;

    jpeg_src.fmt = src->fmt;
    jpeg_src.buf_size = src->buf_size;
    jpeg_src.rect.start_x = src->rect.start_x;
    jpeg_src.rect.start_y = src->rect.start_y;
    jpeg_src.rect.width = src->rect.width;
    jpeg_src.rect.height = src->rect.height;
    jpeg_src.size.width = src->size.width;
    jpeg_src.size.height = src->size.height;
    jpeg_src.addr_phy.addr_y = src->addr_phy.addr_y;
    jpeg_src.addr_phy.addr_u = src->addr_phy.addr_u;
    jpeg_src.addr_phy.addr_v = src->addr_phy.addr_v;
    jpeg_src.addr_vir.addr_y = src->addr_vir.addr_y;
    jpeg_src.addr_vir.addr_u = src->addr_vir.addr_u;
    jpeg_src.addr_vir.addr_v = src->addr_vir.addr_v;
    jpeg_src.fd = src->fd;
    jpeg_src.data_end.y_endian = src->data_end.y_endian;
    jpeg_src.data_end.uv_endian = src->data_end.uv_endian;
    jpeg_src.data_end.reserved0 = src->data_end.reserved0;
    jpeg_src.data_end.reserved1 = src->data_end.reserved1;
    jpeg_src.format_pattern = src->format_pattern;
    jpeg_src.reserved = src->reserved;

    jpeg_dst.fmt = dst->fmt;
    jpeg_dst.buf_size = dst->buf_size;
    jpeg_dst.rect.start_x = dst->rect.start_x;
    jpeg_dst.rect.start_y = dst->rect.start_y;
    jpeg_dst.rect.width = dst->rect.width;
    jpeg_dst.rect.height = dst->rect.height;
    jpeg_dst.size.width = dst->size.width;
    jpeg_dst.size.height = dst->size.height;
    jpeg_dst.addr_phy.addr_y = dst->addr_phy.addr_y;
    jpeg_dst.addr_phy.addr_u = dst->addr_phy.addr_u;
    jpeg_dst.addr_phy.addr_v = dst->addr_phy.addr_v;
    jpeg_dst.addr_vir.addr_y = dst->addr_vir.addr_y;
    jpeg_dst.addr_vir.addr_u = dst->addr_vir.addr_u;
    jpeg_dst.addr_vir.addr_v = dst->addr_vir.addr_v;
    jpeg_dst.fd = dst->fd;
    jpeg_dst.data_end.y_endian = dst->data_end.y_endian;
    jpeg_dst.data_end.uv_endian = dst->data_end.uv_endian;
    jpeg_dst.data_end.reserved0 = dst->data_end.reserved0;
    jpeg_dst.data_end.reserved1 = dst->data_end.reserved1;
    jpeg_dst.format_pattern = dst->format_pattern;
    jpeg_dst.reserved = dst->reserved;

    ret = jcxt->ops.jpeg_encode(jcxt->codec_handle, &jpeg_src, &jpeg_dst, mean,
                                enc_cb_param);
    if (ret) {
        CMR_LOGE("jpeg encode error");
        return CMR_CAMERA_FAIL;
    }
    CMR_LOGD("ret %d", ret);
    return ret;
}

cmr_int cmr_jpeg_decode(cmr_handle jpeg_handle, struct img_frm *src,
                        struct img_frm *dst, struct jpg_op_mean *mean) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct jpeg_codec_caller_handle *codec_handle = NULL;
    struct jpeg_lib_cxt *jcxt = NULL;
    struct yuvbuf_frm jpeg_src;
    struct yuvbuf_frm jpeg_dst;

    if (!jpeg_handle) {
        CMR_LOGE("Invalid Param!");
        return CMR_CAMERA_INVALID_PARAM;
    }

    jcxt = (struct jpeg_lib_cxt *)jpeg_handle;

    jpeg_src.fmt = src->fmt;
    jpeg_src.buf_size = src->buf_size;
    jpeg_src.rect.start_x = src->rect.start_x;
    jpeg_src.rect.start_y = src->rect.start_y;
    jpeg_src.rect.width = src->rect.width;
    jpeg_src.rect.height = src->rect.height;
    jpeg_src.size.width = src->size.width;
    jpeg_src.size.height = src->size.height;
    jpeg_src.addr_phy.addr_y = src->addr_phy.addr_y;
    jpeg_src.addr_phy.addr_u = src->addr_phy.addr_u;
    jpeg_src.addr_phy.addr_v = src->addr_phy.addr_v;
    jpeg_src.addr_vir.addr_y = src->addr_vir.addr_y;
    jpeg_src.addr_vir.addr_u = src->addr_vir.addr_u;
    jpeg_src.addr_vir.addr_v = src->addr_vir.addr_v;
    jpeg_src.fd = src->fd;
    jpeg_src.data_end.y_endian = src->data_end.y_endian;
    jpeg_src.data_end.uv_endian = src->data_end.uv_endian;
    jpeg_src.data_end.reserved0 = src->data_end.reserved0;
    jpeg_src.data_end.reserved1 = src->data_end.reserved1;
    jpeg_src.format_pattern = src->format_pattern;
    jpeg_src.reserved = src->reserved;

    jpeg_dst.fmt = dst->fmt;
    jpeg_dst.buf_size = dst->buf_size;
    jpeg_dst.rect.start_x = dst->rect.start_x;
    jpeg_dst.rect.start_y = dst->rect.start_y;
    jpeg_dst.rect.width = dst->rect.width;
    jpeg_dst.rect.height = dst->rect.height;
    jpeg_dst.size.width = dst->size.width;
    jpeg_dst.size.height = dst->size.height;
    jpeg_dst.addr_phy.addr_y = dst->addr_phy.addr_y;
    jpeg_dst.addr_phy.addr_u = dst->addr_phy.addr_u;
    jpeg_dst.addr_phy.addr_v = dst->addr_phy.addr_v;
    jpeg_dst.addr_vir.addr_y = dst->addr_vir.addr_y;
    jpeg_dst.addr_vir.addr_u = dst->addr_vir.addr_u;
    jpeg_dst.addr_vir.addr_v = dst->addr_vir.addr_v;
    jpeg_dst.fd = dst->fd;
    jpeg_dst.data_end.y_endian = dst->data_end.y_endian;
    jpeg_dst.data_end.uv_endian = dst->data_end.uv_endian;
    jpeg_dst.data_end.reserved0 = dst->data_end.reserved0;
    jpeg_dst.data_end.reserved1 = dst->data_end.reserved1;
    jpeg_dst.format_pattern = dst->format_pattern;
    jpeg_dst.reserved = dst->reserved;

    ret = jcxt->ops.jpeg_decode(jcxt->codec_handle, &jpeg_src, &jpeg_dst, mean);
    if (ret) {
        CMR_LOGE("jpeg encode error");
        return CMR_CAMERA_FAIL;
    }
    CMR_LOGD("ret %d", ret);
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
        input_param.temp_buf_ptr = NULL;
        return JPEG_CODEC_NO_MEM;
    }

    CMR_LOGD("jpeg:src jpeg addr 0x%p, size %d thumbnail addr 0x%p, size %d "
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
    input_param.temp_exif_isp_buf_ptr = NULL;
    input_param.temp_exif_isp_dbg_buf_ptr = NULL;
    CMR_LOGD("jpeg:output: addr 0x%lx,size %d", out_ptr->output_buf_virt_addr,
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

    CMR_LOGD("jpeg:enc add exit start");
    ret = _jpeg_enc_wexif(param_ptr, output_ptr);
    if (ret) {
        CMR_LOGE("jpeg encode add eixf error");
        return CMR_CAMERA_FAIL;
    }

    CMR_LOGD("jpeg:output addr 0x%x,size %d",
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
    CMR_LOGI("cmr_stop_codec enter");
    ret = jcxt->ops.jpeg_stop(jcxt->codec_handle);
    if (ret) {
        CMR_LOGE("stop codec error");
        return CMR_CAMERA_FAIL;
    }
    CMR_LOGD("ret %d", ret);
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
#if USE_LEGACY_DLFCN
        dlclose(jcxt->mLibHandle);
#else
        put_lib_handle(jcxt->mLibHandle);
#endif
        jcxt->mLibHandle = NULL;
        free(jcxt);
        jcxt = NULL;
    }
    CMR_LOGD("ret %d", ret);
    return ret;
}
