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

#define LOG_TAG "cmr_common"
#define ATRACE_TAG (ATRACE_TAG_CAMERA | ATRACE_TAG_HAL)

#include <cutils/trace.h>
#include <stdlib.h>
#include "cmr_common.h"
#include <cutils/properties.h>

#define CAMERA_ZOOM_LEVEL_MAX 8
#define ZOOM_STEP(x) (((x) - (x) / CMR_ZOOM_FACTOR) / CAMERA_ZOOM_LEVEL_MAX)

struct CAMERA_TAKEPIC_STAT cap_stp[CMR_STEP_MAX] = {
    {"takepicture", 0, 0, 0},    {"capture start", 0, 0, 0},
    {"capture end", 0, 0, 0},    {"rotate start", 0, 0, 0},
    {"rotate end", 0, 0, 0},     {"isp pp start", 0, 0, 0},
    {"isp pp end", 0, 0, 0},     {"jpeg dec start", 0, 0, 0},
    {"jpeg dec end", 0, 0, 0},   {"scaling start", 0, 0, 0},
    {"scaling end", 0, 0, 0},    {"uv denoise start", 0, 0, 0},
    {"uv denoise end", 0, 0, 0}, {"y denoise start", 0, 0, 0},
    {"y denoise end", 0, 0, 0},  {"refocus start", 0, 0, 0},
    {"refocus end", 0, 0, 0},    {"jpeg enc start", 0, 0, 0},
    {"jpeg enc end", 0, 0, 0},   {"cvt thumb start", 0, 0, 0},
    {"cvt thumb end", 0, 0, 0},  {"thumb enc start", 0, 0, 0},
    {"thumb enc end", 0, 0, 0},  {"write exif start", 0, 0, 0},
    {"write exif end", 0, 0, 0}, {"call back", 0, 0, 0},
};

struct CAMERA_LAUNCH_TIME cmr_launch_time[CMR_LAUNCH_MAX_T] = {
    {"hal_init", 0, 0, 0},
    {"sensor_init", 0, 0, 0},
    {"grab_init", 0, 0, 0},
    {"isp_init", 0, 0, 0},
    {"res_init", 0, 0, 0},
    {"oem_others_init", 0, 0, 0},
    {"prev_start", 0, 0, 0},
    {"receive_frist_frame_preview", 0, 0, 0},
    {"receive_frist_frame_video", 0, 0, 0},
    {"send_first_frame_hal", 0, 0, 0},
    {"open_total", 0, 0, 0},
    {"flush", 0, 0, 0},
    {"close", 0, 0, 0},
    {"prev_stop", 0, 0, 0},
    {"isp_deinit", 0, 0, 0},
    {"close_total", 0, 0, 0},
    {"recording_start", 0, 0, 0},

    {"capture:path_start", 0, 0, 0},
    {"grab_cap_start", 0, 0, 0},
    {"capture:receive_frame", 0, 0, 0},
    {"arithmetic:hdr_do", 0, 0, 0},
    {"arithmetic:3dnr_do", 0, 0, 0},
    {"arithmetic:filter_do", 0, 0, 0},
    {"capture:hal_cb", 0, 0, 0},
    {"capture:total", 0, 0, 0},
    {"capture:pre_flash", 0, 0, 0},
};

// for hal1.0 calculate crop
cmr_int camera_get_trim_rect(struct img_rect *src_trim_rect,
                             cmr_uint zoom_level, struct img_size *dst_size) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_uint trim_width = 0, trim_height = 0;
    cmr_uint zoom_step_w = 0, zoom_step_h = 0;

    if (!src_trim_rect || !dst_size) {
        CMR_LOGE("0x%lx 0x%lx", (cmr_uint)src_trim_rect, (cmr_uint)dst_size);
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    trim_width = src_trim_rect->width;
    trim_height = src_trim_rect->height;

    if (0 == dst_size->width || 0 == dst_size->height) {
        CMR_LOGE("0x%x 0x%x", dst_size->width, dst_size->height);
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    if (dst_size->width * src_trim_rect->height <
        dst_size->height * src_trim_rect->width) {
        trim_width = dst_size->width * src_trim_rect->height / dst_size->height;
    } else {
        trim_height = dst_size->height * src_trim_rect->width / dst_size->width;
    }

    zoom_step_w = ZOOM_STEP(trim_width);
    zoom_step_w &= ~1;
    zoom_step_w *= zoom_level;
    zoom_step_h = ZOOM_STEP(trim_height);
    zoom_step_h &= ~1;
    zoom_step_h *= zoom_level;
    trim_width = trim_width - zoom_step_w;
    trim_height = trim_height - zoom_step_h;

    src_trim_rect->start_x += (src_trim_rect->width - trim_width) >> 1;
    src_trim_rect->start_y += (src_trim_rect->height - trim_height) >> 1;
    src_trim_rect->start_x = CAMERA_WIDTH(src_trim_rect->start_x);
    src_trim_rect->start_y = CAMERA_HEIGHT(src_trim_rect->start_y);
    src_trim_rect->width = CAMERA_WIDTH(trim_width);
    src_trim_rect->height = CAMERA_HEIGHT(trim_height);
    CMR_LOGD("zoom_level %ld trim rect %d %d %d %d", zoom_level,
             src_trim_rect->start_x, src_trim_rect->start_y,
             src_trim_rect->width, src_trim_rect->height);
exit:
    return ret;
}

// for hal3.2 calculate crop
cmr_int camera_get_trim_rect2(struct img_rect *src_trim_rect, float zoom_ratio,
                              float dst_aspect_ratio, cmr_u32 sensor_w,
                              cmr_u32 sensor_h, cmr_u8 rot) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_u32 trim_width;
    cmr_u32 trim_height;
    float output_aspect_ratio;
    float zoom_width, zoom_height, sensor_aspect_ratio;

    if (NULL == src_trim_rect || dst_aspect_ratio < 0.0001f ||
        zoom_ratio < 0.0001f) {
        CMR_LOGE("pointer of src_trim_rect = %p, dst ratio=%f, zoom ratio=%f",
                 src_trim_rect, dst_aspect_ratio, zoom_ratio);
        return -CMR_CAMERA_INVALID_PARAM;
    }

    if (src_trim_rect->width == 0 || src_trim_rect->height == 0) {
        CMR_LOGE("w %d h %d", src_trim_rect->width, src_trim_rect->height);
        return -CMR_CAMERA_INVALID_PARAM;
    }

    output_aspect_ratio = dst_aspect_ratio;
    sensor_aspect_ratio = (float)sensor_w / (float)sensor_h;

    CMR_LOGD("src_trim_rect %d %d %d %d, sn w/h %d %d, zoom_ratio %f, "
             "dst_aspect_ratio %f,sensor_aspect_ratio =%f",
             src_trim_rect->start_x, src_trim_rect->start_y,
             src_trim_rect->width, src_trim_rect->height, sensor_w, sensor_h,
             zoom_ratio, dst_aspect_ratio, sensor_aspect_ratio);

    sensor_w = CAMERA_ALIGNED_16(sensor_w);
    sensor_h = CAMERA_ALIGNED_16(sensor_h);

    CMR_LOGD("align sensor w,h  %d %d", sensor_w, sensor_h);

    if (rot != IMG_ANGLE_0 && rot != IMG_ANGLE_180) {
        output_aspect_ratio = 1 / output_aspect_ratio;
    }
    if (output_aspect_ratio > sensor_aspect_ratio) {
        zoom_width = (float)sensor_w / zoom_ratio;
        zoom_height = zoom_width / output_aspect_ratio;
    } else {
        zoom_height = (float)sensor_h / zoom_ratio;
        zoom_width = zoom_height * output_aspect_ratio;
    }

    trim_width = (cmr_u32)zoom_width;
    trim_height = (cmr_u32)zoom_height;

    if (trim_width > src_trim_rect->width)
        trim_width = src_trim_rect->width;

    if (trim_height > src_trim_rect->height)
        trim_height = src_trim_rect->height;

    src_trim_rect->start_x += (src_trim_rect->width - trim_width) >> 1;
    src_trim_rect->start_y += (src_trim_rect->height - trim_height) >> 1;
    src_trim_rect->start_x = CAMERA_START(src_trim_rect->start_x);
    src_trim_rect->start_y = CAMERA_START(src_trim_rect->start_y);
    src_trim_rect->width = CAMERA_START(trim_width);
    src_trim_rect->height = CAMERA_START(trim_height);

    CMR_LOGD("output trim rect %d %d %d %d", src_trim_rect->start_x,
             src_trim_rect->start_y, src_trim_rect->width,
             src_trim_rect->height);
    ATRACE_END();
    return ret;
}

cmr_int camera_scale_down_software(struct img_frm *src, struct img_frm *dst) {
    cmr_u8 *dst_y_buf;
    cmr_u8 *dst_uv_buf;
    cmr_u8 *src_y_buf;
    cmr_u8 *src_uv_buf;
    cmr_u32 src_w;
    cmr_u32 src_h;
    cmr_u32 dst_w;
    cmr_u32 dst_h;
    cmr_u32 dst_uv_w;
    cmr_u32 dst_uv_h;
    cmr_u32 cur_w = 0;
    cmr_u32 cur_h = 0;
    cmr_u32 cur_size = 0;
    cmr_u32 cur_byte = 0;
    cmr_u32 ratio_w;
    cmr_u32 ratio_h;
    uint16_t i, j;
    if (NULL == dst || NULL == src) {
        return -1;
    }
    dst_y_buf = (cmr_u8 *)dst->addr_vir.addr_y;
    dst_uv_buf = (cmr_u8 *)dst->addr_vir.addr_u;
    src_y_buf = (cmr_u8 *)src->addr_vir.addr_y;
    src_uv_buf = (cmr_u8 *)src->addr_vir.addr_u;
    src_w = src->size.width;
    src_h = src->size.height;
    dst_w = dst->size.width;
    dst_h = dst->size.height;
    dst_uv_w = dst_w / 2;
    dst_uv_h = dst_h / 2;
    ratio_w = (src_w << 10) / dst_w;
    ratio_h = (src_h << 10) / dst_h;
    for (i = 0; i < dst_h; i++) {
        cur_h = (ratio_h * i) >> 10;
        cur_size = cur_h * src_w;
        for (j = 0; j < dst_w; j++) {
            cur_w = (ratio_w * j) >> 10;
            *dst_y_buf++ = src_y_buf[cur_size + cur_w];
        }
    }
    for (i = 0; i < dst_uv_h; i++) {
        cur_h = (ratio_h * i) >> 10;
        cur_size = cur_h * (src_w / 2) * 2;
        for (j = 0; j < dst_uv_w; j++) {
            cur_w = (ratio_w * j) >> 10;
            cur_byte = cur_size + cur_w * 2;
            *dst_uv_buf++ = src_uv_buf[cur_byte];     // u
            *dst_uv_buf++ = src_uv_buf[cur_byte + 1]; // v
        }
    }
    CMR_LOGD("done");
    return 0;
}

cmr_int camera_save_y_to_file(cmr_u32 index, cmr_u32 img_fmt, cmr_u32 width,
                              cmr_u32 height, void *addr) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    char file_name[40];
    char tmp_str[10];
    FILE *fp = NULL;

    CMR_LOGD("index %d format %d width %d heght %d", index, img_fmt, width,
             height);

    cmr_bzero(file_name, 40);
    strcpy(file_name, CAMERA_DUMP_PATH);
    sprintf(tmp_str, "%d", width);
    strcat(file_name, tmp_str);
    strcat(file_name, "X");
    sprintf(tmp_str, "%d", height);
    strcat(file_name, tmp_str);

    if (IMG_DATA_TYPE_YUV420 == img_fmt || IMG_DATA_TYPE_YUV422 == img_fmt) {
        strcat(file_name, "_y_");
        sprintf(tmp_str, "%d", index);
        strcat(file_name, tmp_str);
        strcat(file_name, ".raw");
        CMR_LOGD("file name %s", file_name);
        fp = fopen(file_name, "wb");

        if (NULL == fp) {
            CMR_LOGD("can not open file: %s \n", file_name);
            return 0;
        }

        fwrite((void *)addr, 1, width * height, fp);
        fclose(fp);
    }
    return 0;
}

cmr_int camera_save_yuv_to_file_scene(cmr_u32 index, cmr_u32 img_fmt,
                                      cmr_u32 width, cmr_u32 height,
                                      struct img_addr *addr, char *scene_type) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    char file_name[0x40];
    char tmp_str[20];
    FILE *fp = NULL;

    CMR_LOGD("index 0x%x format %d width %d height %d, addr 0x%lx 0x%lx", index,
             img_fmt, width, height, addr->addr_y, addr->addr_u);

    cmr_bzero(file_name, 0x40);
    strcpy(file_name, CAMERA_DUMP_PATH);

    sprintf(tmp_str, "%s_", scene_type);
    strcat(file_name, tmp_str);
    sprintf(tmp_str, "%08x_", index);
    strcat(file_name, tmp_str);
    sprintf(tmp_str, "%d", width);
    strcat(file_name, tmp_str);
    strcat(file_name, "X");
    sprintf(tmp_str, "%d", height);
    strcat(file_name, tmp_str);
    if (IMG_DATA_TYPE_YUV420 == img_fmt || IMG_DATA_TYPE_YUV422 == img_fmt) {

        strcat(file_name, "_y");
        strcat(file_name, ".raw");
        CMR_LOGD("file name %s", file_name);
        fp = fopen(file_name, "wb");
        if (NULL == fp) {
            CMR_LOGD("can not open file: %s \n", file_name);
            return 0;
        }
        fwrite((void *)addr->addr_y, 1, width * height, fp);
        fclose(fp);

        bzero(file_name, 40);
        strcpy(file_name, CAMERA_DUMP_PATH);
        sprintf(tmp_str, "%s_", scene_type);
        strcat(file_name, tmp_str);
        sprintf(tmp_str, "%08x_", index);
        strcat(file_name, tmp_str);
        sprintf(tmp_str, "%d", width);
        strcat(file_name, tmp_str);
        strcat(file_name, "X");
        sprintf(tmp_str, "%d", height);
        strcat(file_name, tmp_str);
        strcat(file_name, "_uv");
        strcat(file_name, ".raw");
        CMR_LOGD("file name %s", file_name);
        fp = fopen(file_name, "wb");
        if (NULL == fp) {
            CMR_LOGD("can not open file: %s \n", file_name);
            return 0;
        }

        if (IMG_DATA_TYPE_YUV420 == img_fmt) {
            fwrite((void *)addr->addr_u, 1, width * height / 2, fp);
        } else {
            fwrite((void *)addr->addr_u, 1, width * height, fp);
        }
        fclose(fp);
    } else if (IMG_DATA_TYPE_RAW == img_fmt) {
        strcat(file_name, "_mipi.raw");
        ISP_LOGI("file name %s\n", file_name);
        width = (width * 5 / 4);
        ISP_LOGI("new width %d\n", width);

        fp = fopen(file_name, "wb");
        if (NULL == fp) {
            ISP_LOGI("can not open file: %s", file_name);
            return 0;
        }

        fwrite((void *)addr->addr_y, 1, (uint32_t)((width * 5 / 4) * height),
               fp);
        fclose(fp);
    } else if (IMG_DATA_TYPE_RAW2 == img_fmt) {
        strcat(file_name, "_mipi2.raw");
        ISP_LOGI("file name %s", file_name);
        width = (width * 4 / 3 + 7) & (~7);
        ISP_LOGI("new width %d\n", width);

        fp = fopen(file_name, "wb");
        if (NULL == fp) {
            ISP_LOGI("can not open file: %s", file_name);
            return 0;
        }

        fwrite((void *)addr->addr_y, 1, (uint32_t)width * height, fp);
        fclose(fp);
    }

    return 0;
}

cmr_int camera_save_yuv_to_file(cmr_u32 index, cmr_u32 img_fmt, cmr_u32 width,
                                cmr_u32 height, struct img_addr *addr) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    char file_name[0x40];
    char tmp_str[10];
    FILE *fp = NULL;

    CMR_LOGD("index 0x%x format %d width %d height %d, addr 0x%lx 0x%lx", index,
             img_fmt, width, height, addr->addr_y, addr->addr_u);

    cmr_bzero(file_name, 0x40);
    strcpy(file_name, CAMERA_DUMP_PATH);
    sprintf(tmp_str, "%08x_", index);
    strcat(file_name, tmp_str);

    sprintf(tmp_str, "%d", width);
    strcat(file_name, tmp_str);
    strcat(file_name, "X");
    sprintf(tmp_str, "%d", height);
    strcat(file_name, tmp_str);
    if (IMG_DATA_TYPE_YUV420 == img_fmt || IMG_DATA_TYPE_YUV422 == img_fmt) {

        strcat(file_name, "_y");
        strcat(file_name, ".raw");
        CMR_LOGD("file name %s", file_name);
        fp = fopen(file_name, "wb");
        if (NULL == fp) {
            CMR_LOGD("can not open file: %s \n", file_name);
            return 0;
        }
        fwrite((void *)addr->addr_y, 1, width * height, fp);
        fclose(fp);

        bzero(file_name, 40);
        strcpy(file_name, CAMERA_DUMP_PATH);
        sprintf(tmp_str, "%08x_", index);
        strcat(file_name, tmp_str);
        sprintf(tmp_str, "%d", width);
        strcat(file_name, tmp_str);
        strcat(file_name, "X");
        sprintf(tmp_str, "%d", height);
        strcat(file_name, tmp_str);
        strcat(file_name, "_uv");
        strcat(file_name, ".raw");
        CMR_LOGD("file name %s", file_name);
        fp = fopen(file_name, "wb");
        if (NULL == fp) {
            CMR_LOGD("can not open file: %s \n", file_name);
            return 0;
        }

        if (IMG_DATA_TYPE_YUV420 == img_fmt) {
            fwrite((void *)addr->addr_u, 1, width * height / 2, fp);
        } else {
            fwrite((void *)addr->addr_u, 1, width * height, fp);
        }
        fclose(fp);
    } else if (IMG_DATA_TYPE_RAW == img_fmt) {
        strcat(file_name, "_mipi.raw");
        ISP_LOGI("file name %s\n", file_name);
        width = (width * 5 / 4);
        ISP_LOGI("new width %d\n", width);

        fp = fopen(file_name, "wb");
        if (NULL == fp) {
            ISP_LOGI("can not open file: %s", file_name);
            return 0;
        }

        fwrite((void *)addr->addr_y, 1, (uint32_t)((width * 5 / 4) * height),
               fp);
        fclose(fp);
    } else if (IMG_DATA_TYPE_RAW2 == img_fmt) {
        strcat(file_name, "_mipi2.raw");
        ISP_LOGI("file name %s", file_name);
        width = (width * 4 / 3 + 7) & (~7);
        ISP_LOGI("new width %d\n", width);

        fp = fopen(file_name, "wb");
        if (NULL == fp) {
            ISP_LOGI("can not open file: %s", file_name);
            return 0;
        }

        fwrite((void *)addr->addr_y, 1, (uint32_t)width * height, fp);
        fclose(fp);
    }

    return 0;
}

cmr_int camera_save_jpg_to_file(cmr_u32 index, cmr_u32 img_fmt, cmr_u32 width,
                                cmr_u32 height, cmr_u32 stream_size,
                                struct img_addr *addr) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    char file_name[80];
    char tmp_str[10];
    FILE *fp = NULL;

    CMR_LOGD("index 0x%x format %d width %d height %d, addr 0x%lx 0x%lx", index,
             img_fmt, width, height, addr->addr_y, addr->addr_u);

    cmr_bzero(file_name, 40);
    strcpy(file_name, CAMERA_DUMP_PATH);
    sprintf(tmp_str, "%08x", index);
    strcat(file_name, tmp_str);
    strcat(file_name, "_");
    sprintf(tmp_str, "%d", width);
    strcat(file_name, tmp_str);
    strcat(file_name, "X");
    sprintf(tmp_str, "%d", height);
    strcat(file_name, tmp_str);
    strcat(file_name, ".jpg");
    CMR_LOGD("file name %s", file_name);

    fp = fopen(file_name, "wb");
    if (NULL == fp) {
        CMR_LOGD("can not open file: %s \n", file_name);
        return 0;
    }

    fwrite((void *)addr->addr_y, 1, stream_size, fp);
    fclose(fp);

    return 0;
}

cmr_int read_file(const char *file_name, void *data_buf, uint32_t buf_size) {
    FILE *pf = NULL;
    uint32_t file_len = 0;

    if (NULL == data_buf)
        return 0;

    char tmp_name[128];
    strcpy(tmp_name, CAMERA_DUMP_PATH);
    strcat(tmp_name, file_name);
    pf = fopen(tmp_name, "rb");
    if (NULL == pf)
        return 0;

    fseek(pf, 0, SEEK_END);
    file_len = ftell(pf);
    fseek(pf, 0, SEEK_SET);

    if (buf_size >= file_len && file_len > 0)
        file_len = fread(data_buf, 1, file_len, pf);
    else
        file_len = 0;

    fclose(pf);

    return file_len;
}

cmr_int save_file(const char *file_name, void *data, uint32_t data_size) {
    char tmp_name[128];
    strcpy(tmp_name, CAMERA_DUMP_PATH);
    strcat(tmp_name, file_name);
    FILE *pf = fopen(tmp_name, "wb");
    uint32_t write_bytes = 0;

    if (NULL == pf)
        return 0;

    write_bytes = fwrite(data, 1, data_size, pf);
    fclose(pf);

    return write_bytes;
}

// parse the filename like
// 4208X3120_gain_123_awbgain_r_1659_g_1024_b_1757_ct_4901_bv_64.mipi_raw
// /%dX%d_gain_%d_ispdgain_%d_awbgain_r_%d_g_%d_b_%d_ct_%d_bv_%d.mipi_raw
cmr_int camera_parse_raw_filename(char *file_name,
                                  struct isptool_scene_param *scene_param) {
    char *ptr0, *ptr1, tmp_str[40], c = 'X';
    cmr_int ret = CMR_CAMERA_SUCCESS;

    if (!file_name || !scene_param) {
        return CMR_CAMERA_FAIL;
    }

    ptr0 = strchr(file_name, c);
    if (!ptr0) {
        CMR_LOGE("parse width failed");
        return CMR_CAMERA_FAIL;
    }
    bzero(tmp_str, sizeof(tmp_str));
    strncpy(tmp_str, file_name, ptr0 - file_name);
    scene_param->width = atoi(tmp_str);
    ptr1 = strstr(file_name, "_gain_");
    if (!ptr1) {
        CMR_LOGE("parse height failed");
        return CMR_CAMERA_FAIL;
    }

    bzero(tmp_str, sizeof(tmp_str));
    strncpy(tmp_str, ptr0 + 1, ptr1 - (ptr0 + 1));
    scene_param->height = atoi(tmp_str);

    ptr0 = strstr(file_name, "_ispdgain_");
    if (!ptr0) {
        CMR_LOGE("parse gain failed");
        return CMR_CAMERA_FAIL;
    }
    bzero(tmp_str, sizeof(tmp_str));
    strncpy(tmp_str, ptr1 + 6, ptr0 - (ptr1 + 6));
    scene_param->gain = atoi(tmp_str);

    ptr1 = strstr(file_name, "_awbgain_r_");
    if (!ptr1) {
        CMR_LOGE("parse ispdgain failed");
        return CMR_CAMERA_FAIL;
    }
    bzero(tmp_str, sizeof(tmp_str));
    strncpy(tmp_str, ptr0 + 10, ptr1 - (ptr0 + 10));
    scene_param->global_gain = atoi(tmp_str);

    ptr0 = strstr(file_name, "_g_");
    if (!ptr0) {
        CMR_LOGE("parse awb_gain_r failed");
        return CMR_CAMERA_FAIL;
    }

    bzero(tmp_str, sizeof(tmp_str));
    strncpy(tmp_str, ptr1 + 11, ptr0 - (ptr1 + 11));
    scene_param->awb_gain_r = atoi(tmp_str);

    ptr1 = strstr(file_name, "_b_");
    if (!ptr1) {
        CMR_LOGE("parse awb_gain_g failed");
        return CMR_CAMERA_FAIL;
    }

    bzero(tmp_str, sizeof(tmp_str));
    strncpy(tmp_str, ptr0 + 3, ptr1 - (ptr0 + 3));
    scene_param->awb_gain_g = atoi(tmp_str);

    ptr0 = strstr(file_name, "_ct_");
    if (!ptr0) {
        CMR_LOGE("parse awb_gain_b failed");
        return CMR_CAMERA_FAIL;
    }

    bzero(tmp_str, sizeof(tmp_str));
    strncpy(tmp_str, ptr1 + 3, ptr0 - (ptr1 + 3));
    scene_param->awb_gain_b = atoi(tmp_str);

    ptr1 = strstr(file_name, "_bv_");
    if (!ptr1) {
        CMR_LOGE("parse smart_ct failed");
        return CMR_CAMERA_FAIL;
    }

    bzero(tmp_str, sizeof(tmp_str));
    strncpy(tmp_str, ptr0 + 4, ptr1 - (ptr0 + 4));
    scene_param->smart_ct = atoi(tmp_str);

    ptr0 = strstr(file_name, ".mipi_raw");
    if (!ptr0) {
        CMR_LOGE("parse smart_bv failed");
        return CMR_CAMERA_FAIL;
    }
    bzero(tmp_str, sizeof(tmp_str));
    strncpy(tmp_str, ptr1 + 4, ptr0 - (ptr1 + 4));
    scene_param->smart_bv = atoi(tmp_str);

    CMR_LOGD(
        "w/h %d/%d, gain %d awb_r %d, awb_g %d awb_b %d ct %d bv %d glb %d",
        scene_param->width, scene_param->height, scene_param->gain,
        scene_param->awb_gain_r, scene_param->awb_gain_g,
        scene_param->awb_gain_b, scene_param->smart_ct, scene_param->smart_bv,
        scene_param->global_gain);

    return ret;
}

cmr_int camera_get_data_from_file(char *file_name, cmr_u32 img_fmt,
                                  cmr_u32 width, cmr_u32 height,
                                  struct img_addr *addr) {
    FILE *fp = NULL;
    cmr_int ret = CMR_CAMERA_SUCCESS;

    CMR_LOGD("file_name:%s format %d width %d heght %d", file_name, img_fmt,
             width, height);
    if (IMG_DATA_TYPE_YUV420 == img_fmt || IMG_DATA_TYPE_YUV422 == img_fmt) {
        return 0;
    } else if (IMG_DATA_TYPE_JPEG == img_fmt) {
        return 0;
    } else if (IMG_DATA_TYPE_RAW == img_fmt) {
        fp = fopen(file_name, "rb");
        if (NULL == fp) {
            CMR_LOGD("can not open file: %s \n", file_name);
            return 0;
        }

        ret = fread((void *)addr->addr_y, 1, (uint32_t)(width * height * 5 / 4),
                    fp);
        fclose(fp);
    }

    return ret;
}

void camera_snapshot_step_statisic(struct img_size *image_size) {
    cmr_int i = 0, time_delta = 0;

    CMR_LOGE("[Camera_HAL_Time]***************Take picture "
             "statistic*******Start****");

    for (i = 0; i < CMR_STEP_MAX; i++) {
        if (i == 0) {
            CMR_LOGE("[Camera_HAL_Time]%20s, %10d", cap_stp[i].step_name, 0);
            continue;
        }

        if (1 == cap_stp[i].valid) {
            time_delta = (int)((cap_stp[i].timestamp -
                                cap_stp[CMR_STEP_TAKE_PIC].timestamp) /
                               1000000);
            CMR_LOGE("[Camera_HAL_Time]%20s, %10ld", cap_stp[i].step_name,
                     time_delta);
            cap_stp[i].valid = 0;
        }
    }
    CMR_LOGE("[Camera_HAL_Time]***************Take picture "
             "statistic********End****");
}

void camera_take_snapshot_step(enum CAMERA_TAKEPIC_STEP step) {
    if (step > CMR_STEP_CALL_BACK) {
        CMR_LOGE("error %d", step);
        return;
    }
    cap_stp[step].timestamp = systemTime(CLOCK_MONOTONIC);
    cap_stp[step].valid = 1;
}
void LAUNCHLOGS(enum CAMERA_LAUNCH_STEP step) {
    char value[PROPERTY_VALUE_MAX];

    if (step > CMR_LAUNCH_MAX_T - 1) {
        CMR_LOGE("error %d", step);
        return;
    }

    property_get("persist.vendor.cam.hal.camera.launch.time", value, "false");
    if (!strcmp(value, "false")) {
        return;
    }

    nsecs_t timestamp_now = systemTime(CLOCK_MONOTONIC);
    // during open camera,first frame will process.
    if (cmr_launch_time[CMR_OPEN_TOTAL_T].valid != 1 &&
        (step == CMR_PREV_RECEIVE_FIRST_FRAME_T ||
         step == CMR_HAL_SEND_FIRST_FRAME_T)) {
        return;
    } else if (cmr_launch_time[CMR_PRE_FLASH_T].valid == 1 &&
               step == CMR_PRE_FLASH_T) {
        // pre_flash only once.
        return;
    } else {
        cmr_launch_time[step].timestamp_start = timestamp_now;
        cmr_launch_time[step].valid = 1;
    }

    if (step == CMR_HAL_INIT_T) {
        cmr_launch_time[CMR_OPEN_TOTAL_T].timestamp_start = timestamp_now;
        cmr_launch_time[CMR_OPEN_TOTAL_T].valid = 1;
    } else if (step == CMR_FLUSH_T /*&&
               cmr_launch_time[CMR_CLOSE_TOTAL_T].valid != 1*/) {
        cmr_launch_time[CMR_CLOSE_TOTAL_T].timestamp_start = timestamp_now;
        cmr_launch_time[CMR_CLOSE_TOTAL_T].valid = 1;
    } else if (step == CMR_CLOSE_T &&
               cmr_launch_time[CMR_CLOSE_TOTAL_T].valid != 1) {
        cmr_launch_time[CMR_CLOSE_TOTAL_T].timestamp_start = timestamp_now;
        cmr_launch_time[CMR_CLOSE_TOTAL_T].valid = 1;
    } else if (step == CMR_CAPTURE_PATH_START_T &&
               cmr_launch_time[CMR_CAPTURE_TOTAL_T].valid != 1) {
        cmr_launch_time[CMR_CAPTURE_TOTAL_T].timestamp_start = timestamp_now;
        cmr_launch_time[CMR_CAPTURE_TOTAL_T].valid = 1;
    } else if (step == CMR_PRE_FLASH_T &&
               cmr_launch_time[CMR_CAPTURE_TOTAL_T].valid != 1) {
        CMR_LOGE("flash open with auto focus.total time start form pre flash");
        cmr_launch_time[CMR_CAPTURE_TOTAL_T].timestamp_start = timestamp_now;
        cmr_launch_time[CMR_CAPTURE_TOTAL_T].valid = 1;
    }
}

void LAUNCHLOGE(enum CAMERA_LAUNCH_STEP step) {
    char value[PROPERTY_VALUE_MAX];

    if (step > CMR_LAUNCH_MAX_T - 1) {
        CMR_LOGE("error %d", step);
        return;
    }
    property_get("persist.vendor.cam.hal.camera.launch.time", value, "false");
    if (!strcmp(value, "false")) {
        return;
    }
    nsecs_t timestamp_now = systemTime(CLOCK_MONOTONIC);

    if (cmr_launch_time[step].valid == 1) {
        cmr_launch_time[step].timestamp_result =
            ns2ms(timestamp_now - cmr_launch_time[step].timestamp_start);
        LAUNCHPLOG(step);
        cmr_launch_time[step].valid = 0;
        if (step == CMR_HAL_SEND_FIRST_FRAME_T &&
            cmr_launch_time[CMR_OPEN_TOTAL_T].valid == 1) {
            cmr_launch_time[CMR_OPEN_TOTAL_T].timestamp_result =
                ns2ms(timestamp_now -
                      cmr_launch_time[CMR_OPEN_TOTAL_T].timestamp_start);
            LAUNCHPLOG(CMR_OPEN_TOTAL_T);
            cmr_launch_time[CMR_OPEN_TOTAL_T].valid = 0;
        } else if (step == CMR_CLOSE_T &&
                   cmr_launch_time[CMR_CLOSE_TOTAL_T].valid == 1) {
            cmr_launch_time[CMR_CLOSE_TOTAL_T].timestamp_result =
                ns2ms(timestamp_now -
                      cmr_launch_time[CMR_CLOSE_TOTAL_T].timestamp_start);
            LAUNCHPLOG(CMR_CLOSE_TOTAL_T);
            cmr_launch_time[CMR_CLOSE_TOTAL_T].valid = 0;
        } else if (step == CMR_CAPTURE_HAL_CB_T &&
                   cmr_launch_time[CMR_CAPTURE_TOTAL_T].valid == 1) {
            cmr_launch_time[CMR_CAPTURE_TOTAL_T].timestamp_result =
                ns2ms(timestamp_now -
                      cmr_launch_time[CMR_CAPTURE_TOTAL_T].timestamp_start);
            LAUNCHPLOG(CMR_CAPTURE_TOTAL_T);
            cmr_launch_time[CMR_CAPTURE_TOTAL_T].valid = 0;
            camera_snapshot_step_statisic(NULL);
        }
    }
}

void LAUNCHPLOG(enum CAMERA_LAUNCH_STEP step) {
    CMR_LOGE("[Camera_HAL_Time]******[%28s]    cost time:    [%4d ms]",
             cmr_launch_time[step].step_name,
             cmr_launch_time[step].timestamp_result);
}

cmr_int camera_get_snap_postproc_time() {
    cmr_int postproc_time = 0;
    postproc_time = (int)((cap_stp[CMR_STEP_CALL_BACK].timestamp -
                           cap_stp[CMR_STEP_CAP_E].timestamp) /
                          1000000);
    return postproc_time;
}

void camera_get_picture_size(multiCameraMode mode, int *width, int *height) {
    if ((MODE_BOKEH == mode) || (MODE_TUNING == mode)) {
        char value[PROPERTY_VALUE_MAX];
        *width = 2592;
        *height = 1944;
        property_get("persist.vendor.cam.res.bokeh", value, "RES_5M");
        if (!strncmp(value, "RES_0_3M", 12)) {
            *width = 640;
            *height = 480;
        } else if (!strncmp(value, "RES_2M", 12)) {
            *width = 1600;
            *height = 1200;
        } else if (!strncmp(value, "RES_1080P", 12)) {
            *width = 1920;
            *height = 1080;
        } else if (!strncmp(value, "RES_5M", 12)) {
            *width = 2592;
            *height = 1944;
        } else if (!strncmp(value, "RES_8M", 12)) {
            *width = 3264;
            *height = 2440;
        } else if (!strncmp(value, "RES_13M", 12)) {
            *width = 4160;
            *height = 3120;
        }
    } else {
        *width = 2592;
        *height = 1944;
    }
}
