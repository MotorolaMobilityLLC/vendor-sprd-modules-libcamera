
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
#include <stdlib.h>
#include <cutils/trace.h>
#include <fcntl.h>
#include <math.h>
#include "cmr_common.h"
#include "hal_yuvprocess.h"
int yuv_scale_nv21_hal(const struct img_frm *src,
                   struct img_frm *dst) {

    CMR_LOGI("yuv scale enter");
    int ret = 0;
    if (src == NULL) {
        CMR_LOGE("src is NULL");
        return CMR_CAMERA_INVALID_PARAM;
    }

    unsigned int i420_src_size = 0;
    unsigned int i420_src_w = 0;
    unsigned int i420_src_h = 0;
    uint8_t *i420_src = NULL;
    uint8_t *i420_src_u = NULL;
    uint8_t *i420_src_v = NULL;
    uint8_t *tmp = NULL;
    unsigned int tmp_w = 0;
    unsigned int tmp_h = 0;
    uint8_t *tmp_u = NULL;
    uint8_t *tmp_v = NULL;
    unsigned int dst_size = dst->size.width * dst->size.height;
    uint8_t *i420_dst = (uint8_t *)malloc(dst_size * 3 / 2);
    if (!i420_dst) {
        //HAL_LOGD("no memory for i420_dst!\n");
        return CMR_CAMERA_NO_MEM;
    }
    uint8_t *i420_dst_u = (uint8_t *)i420_dst + dst_size;
    uint8_t *i420_dst_v = (uint8_t *)i420_dst + dst_size * 5 / 4;

    i420_src_size = src->size.width * src->size.height;
    i420_src_w = src->size.width;
    i420_src_h = src->size.height;
    i420_src = (uint8_t *)malloc(i420_src_size * 3 / 2);
    if (!i420_src) {
        CMR_LOGE("no memory for i420_src!\n");
        ret = CMR_CAMERA_NO_MEM;
        goto exit;
    }
    i420_src_u = (uint8_t *)i420_src + i420_src_size;
    i420_src_v = (uint8_t *)i420_src + i420_src_size * 5 / 4;
    if (1) {
#if defined(CONFIG_LIBYUV_NEON)
        uint8x16_t v;
        uint32_t crop_size;
        uint32_t crop_x = 0;
        uint32_t crop_y = 0;
        uint8_t *src_y = (uint8_t *)(cmr_uint)(src->addr_vir.addr_y) +
                         src->size.width * crop_y + crop_x;
        uint8_t *src_u = (uint8_t *)(cmr_uint)(src->addr_vir.addr_u) +
                         ((crop_y / 2) * src->size.width) +
                         ((crop_x / 2) * 2);
        uint32_t crop_w = src->size.width;
        uint32_t crop_h = src->size.height;

        crop_w = ((crop_w >> 4) << 4);
        crop_h = ((crop_h >> 4) << 4);
        crop_size = crop_w * crop_h;
        tmp = i420_src;

        for (uint32_t i = 0; i < crop_size; i += 16) {
            if (i < crop_w) {
                v = vld1q_u8(src_y + i);
                vst1q_u8(tmp + i, v);
            } else {
                i = 0;
                crop_size -= crop_w;
                if (crop_size) {
                    src_y += src->size.width;
                    tmp += crop_w;
                }
            }
        }
        SplitUVPlane(src_u, src->size.width, i420_src_v, crop_w >> 1,
                     i420_src_u, crop_w >> 1, crop_w >> 1, crop_h >> 1);
        i420_src_w = crop_w;
        i420_src_h = crop_h;
#else
        tmp_w = (src->size.width >> 5) << 5;
        tmp_h = (src->size.height >> 1) << 1;
        i420_src_w = tmp_w;
        i420_src_h = tmp_h;
        ret = ConvertToI420((uint8_t *)(cmr_uint)src->addr_vir.addr_y,
                            src->size.width, i420_src, tmp_w, i420_src_u,
                            tmp_w / 2, i420_src_v, tmp_w / 2, 0,
                            0, src->size.width, src->size.height,
                            tmp_w, tmp_h, kRotate0, FOURCC_NV21);
#endif
    } else {
        ret = ConvertToI420((uint8_t *)(cmr_uint)src->addr_vir.addr_y,
                            src->size.width, i420_src, src->size.width,
                            i420_src_u, src->size.width / 2, i420_src_v,
                            src->size.width / 2, 0, 0,
                            src->size.width, src->size.height, src->size.width,
                            src->size.height, kRotate0, FOURCC_NV21);
    }

    CMR_LOGI("yuv convert done");
    if (ret) {
        CMR_LOGE("libyuv convert to i420 fail\n");
        goto exit;
    }

    tmp_w = (i420_src_w >> 6) << 6;
    tmp_h = (i420_src_h >> 2) << 2;
    uint8_t *tmp_swp = NULL;
    tmp = (uint8_t *)malloc(i420_src_size * 3 / 2);
    if (!tmp) {
        CMR_LOGE("no memory for tmp!\n");
        ret = CMR_CAMERA_NO_MEM;
        goto exit;
    }
    tmp_u = (uint8_t *)tmp + i420_src_size;
    tmp_v = (uint8_t *)tmp + i420_src_size * 5 / 4;
    tmp_w >>= 2;
    tmp_h >>= 2;
    if ((tmp_w > dst->size.width) && (tmp_h > dst->size.height)) {
        I420Scale(i420_src, i420_src_w, i420_src_u, (i420_src_w + 1) / 2,
                        i420_src_v, (i420_src_w + 1) / 2, tmp_w << 2,
                        tmp_h << 2, tmp, tmp_w, tmp_u, tmp_w / 2, tmp_v,
                        tmp_w / 2, tmp_w, tmp_h, kFilterBox); // kFilterLinear);
        tmp_swp = i420_src;
        i420_src = tmp;
        tmp = tmp_swp;

        tmp_swp = i420_src_u;
        i420_src_u = tmp_u;
        tmp_u = tmp_swp;

        tmp_swp = i420_src_v;
        i420_src_v = tmp_v;
        tmp_v = tmp_swp;

        i420_src_w = tmp_w;
        i420_src_h = tmp_h;
    }

    while (((i420_src_w >> 5) << 5 > (dst->size.width << 1)) &&
           ((i420_src_h >> 1) << 1 > (dst->size.height << 1))) {
        tmp_w = (i420_src_w >> 5) << 5;
        tmp_h = (i420_src_h >> 1) << 1;
        tmp_w >>= 1;
        tmp_h >>= 1;
        I420Scale(i420_src, i420_src_w, i420_src_u, (i420_src_w + 1) / 2,
                        i420_src_v, (i420_src_w + 1) / 2, tmp_w << 1,
                        tmp_h << 1, tmp, tmp_w, tmp_u, tmp_w / 2, tmp_v,
                        tmp_w / 2, tmp_w, tmp_h, kFilterBox); // kFilterLinear);
        tmp_swp = i420_src;
        i420_src = tmp;
        tmp = tmp_swp;

        tmp_swp = i420_src_u;
        i420_src_u = tmp_u;
        tmp_u = tmp_swp;

        tmp_swp = i420_src_v;
        i420_src_v = tmp_v;
        tmp_v = tmp_swp;

        i420_src_w = tmp_w;
        i420_src_h = tmp_h;
    }

    ret = I420Scale(i420_src, i420_src_w, i420_src_u, i420_src_w / 2,
                    i420_src_v, i420_src_w / 2, i420_src_w, i420_src_h,
                    i420_dst, dst->size.width, i420_dst_u, dst->size.width / 2,
                    i420_dst_v, dst->size.width / 2, dst->size.width,
                    dst->size.height, kFilterBox); // kFilterLinear);
    if (ret) {
        CMR_LOGE("libyuv::I420Scale failed");
        goto exit;
    }
    CMR_LOGI("yuv scale done");
    ret = I420ToNV21(i420_dst, dst->size.width, i420_dst_u, dst->size.width / 2,
                     i420_dst_v, dst->size.width / 2,
                     (uint8_t *)(cmr_uint)dst->addr_vir.addr_y, dst->size.width,
                     (uint8_t *)(cmr_uint)dst->addr_vir.addr_u, dst->size.width,
                     dst->size.width, dst->size.height);
    if (ret) {
        CMR_LOGE("libyuv::I420ToNV21 failed");
        goto exit;
    }
exit:
    if (i420_src)
        free(i420_src);
    if (tmp)
        free(tmp);
    if (i420_dst)
        free(i420_dst);

    CMR_LOGI("done");
    return ret;
}
