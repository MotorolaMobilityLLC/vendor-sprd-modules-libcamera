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
#define LOG_TAG "cmr_mm_dvfs"

#include "cmr_mm_dvfs.h"
#include "cmr_oem.h"
#include <stdio.h>

const uint CLK_48M = 48000000;
const uint CLK_64M = 64000000;
const uint CLK_76M = 76000000;
const uint CLK_128M = 128000000;
const uint CLK_192M = 192000000;
const uint CLK_256M = 256000000;
const uint CLK_307M = 307200000;
const uint CLK_384M = 384000000;
const uint CLK_468M = 468000000;
const uint CLK_512M = 512000000;

const uint CLK_DCAM_MAX = 512000000;
const uint CLK_ISP_MAX = 512000000;

const uint PIC_SIZE_48M = 48;
const uint PIC_SIZE_32M = 32;
const uint PIC_SIZE_24M = 24;
const uint PIC_SIZE_16M = 16;
const uint PIC_SIZE_13M = 13;
const uint PIC_SIZE_12M = 12;
const uint PIC_SIZE_8M = 8;
const uint PIC_SIZE_5M = 5;

const char *str_module[] = {"cpp",     "fd",       "jpg", "isp",
                            "dcam-if", "dcam-axi", "mtx"};

struct class_mm_dvfs *getInstance() {
    if (mm_dvfs_instance != NULL)
        return mm_dvfs_instance;
    else {
        mm_dvfs_instance =
            (struct class_mm_dvfs *)malloc(sizeof(struct class_mm_dvfs));
        if (!mm_dvfs_instance) {
            CMR_LOGE("No mem");
            return NULL;
        }
        memset(mm_dvfs_instance, 0, sizeof(struct class_mm_dvfs));
        pthread_mutex_init(&mm_dvfs_instance->mm_dvfs_mutex, NULL);
        return mm_dvfs_instance;
    }
}

cmr_int cmr_mm_dvfs_init(cmr_handle *mm_dvfs_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    *mm_dvfs_handle = (cmr_handle)getInstance();
    if (!mm_dvfs_handle)
        ret = -1;
    return ret;
}

cmr_int cmr_mm_dvfs_deinit(cmr_handle mm_dvfs_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    if (!mm_dvfs_handle) {
        CMR_LOGE("Invalid handle");
        return -1;
    }
    struct class_mm_dvfs *p_dvfs = (struct class_mm_dvfs *)mm_dvfs_handle;
    CMR_LOGI("mm_dvfs_instance %p mm_dvfs_handle %p", mm_dvfs_instance,
             mm_dvfs_handle);
    if (mm_dvfs_instance) {
        pthread_mutex_destroy(&mm_dvfs_instance->mm_dvfs_mutex);
        free((void *)mm_dvfs_instance);
        mm_dvfs_instance = NULL;
    }
    mm_dvfs_handle = NULL;
    CMR_LOGI("X");
    return ret;
}

cmr_int set_dcam_dvfs_policy(cmr_handle mm_dvfs_handle,
                             enum DVFS_MM_MODULE module,
                             enum CamProcessingState camera_state) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    uint mmSetFreq = 0;
    char mm_set_freq[128];
    uint freqValue = 0;
    struct class_mm_dvfs *p_dvfs = NULL;
    p_dvfs = (struct class_mm_dvfs *)(mm_dvfs_handle);
    if (!p_dvfs) {
        CMR_LOGE("invalid mm_dvfs_handle");
        return -1;
    }
    if (p_dvfs->dvfs_param.is_high_fps == 1 ||
        p_dvfs->dvfs_param.cam_mode == 1) {
        freqValue = CLK_DCAM_MAX;
        CMR_LOGD("dcam_if dvfs vaule is already CLK_DCAM_MAX  %d,do nothing",
                 freqValue);
    } else {
        switch (p_dvfs->dvfs_param.lane_num) {
        case 1:
        case 2:
            freqValue = p_dvfs->dvfs_param.bps_per_lane / 8;
            break;
        case 3:
            freqValue = p_dvfs->dvfs_param.bps_per_lane / 8 * 6 / 5;
            break;
        case 4:
            freqValue = p_dvfs->dvfs_param.bps_per_lane /
                        5; // camParam.bps_per_lane/8 * 8 / 5
            break;
        default:
            CMR_LOGE("invalid lane_numb params for isp mm dvfs");
            return ret;
        }
        if (freqValue * 1000000 <= CLK_192M) {
            freqValue = CLK_192M;
        } else if (freqValue * 1000000 <= CLK_256M) {
            freqValue = CLK_256M;
        } else if (freqValue * 1000000 <= CLK_307M) {
            freqValue = CLK_307M;
        } else if (freqValue * 1000000 <= CLK_384M) {
            freqValue = CLK_384M;
        } else if (freqValue * 1000000 <= CLK_468M) {
            freqValue = CLK_468M;
        } else {
            freqValue = CLK_DCAM_MAX;
        }
    }
    if (freqValue == p_dvfs->dvfs_status.dmca_if_cur_freq) {
        CMR_LOGD("dcam_if dvfs vaule is already %d,do nothing", freqValue);
        return ret;
    }
    sprintf(mm_set_freq, "/sys/class/devfreq/%s-dvfs/%s_governor/set_work_freq",
            str_module[module - 1], str_module[module - 1]);

    FILE *fp = fopen(mm_set_freq, "wb");
    if (fp == NULL) {
        CMR_LOGE("fail to open file for mm dvfs: %s ", mm_set_freq);
        return 0;
    }
    fprintf(fp, "%d", freqValue);
    fclose(fp);
    fp = NULL;
    p_dvfs->dvfs_status.dmca_if_cur_freq = freqValue;
    if (freqValue < CLK_DCAM_MAX) {
        CMR_LOGI("set dvfs-> %s done, freq is %d", str_module[module - 1],
                 freqValue);
    } else {
        CMR_LOGI("set dvfs-> %s done, freq is max", str_module[module - 1]);
    }

    return ret;
    // str_module[module - 1], CLK_DCAM_MAX != freqValue ? freqValue :"max");
    /*
     * clk_mipi is 1/8 of bps_per_lane, clk_dcam_if more than or equals to
     * clk_mipi when lane_num is 1 & 2, more than 1.2X when lane_num is 3,
     * more than 1.6X when lane_num is 4.
    clk_mipi = dcam_cxt->bps_per_lane / 8;
    pr_info("dcam_cxt->bps_per_lane is %d\n", dcam_cxt->bps_per_lane);
    switch (dcam_cxt->lane_num) {
    case DCAM_CSI_LANE_1:
        clk_dcam_if = clk_mipi;
        clk_isp_if = clk_mipi;
        break;
    case DCAM_CSI_LANE_2:
        clk_dcam_if = clk_mipi;
        clk_isp_if = clk_mipi * 8 / 5;
        break;
    case DCAM_CSI_LANE_3:
        clk_dcam_if = clk_mipi * 6 / 5;
        clk_isp_if = clk_mipi * 12 / 5;
        break;
    case DCAM_CSI_LANE_4:
        clk_dcam_if = clk_mipi * 8 / 5;
        clk_isp_if = clk_mipi * 16 / 5;
        break;
    */
}

cmr_int set_isp_dvfs_policy(cmr_handle mm_dvfs_handle,
                            enum DVFS_MM_MODULE module,
                            enum CamProcessingState camera_state) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    uint mmSetFreq = 0;
    char mm_set_freq[128];
    struct class_mm_dvfs *p_dvfs = NULL;
    uint freqValue = 0;
    p_dvfs = (struct class_mm_dvfs *)(mm_dvfs_handle);
    if (!p_dvfs) {
        CMR_LOGE("invalid mm_dvfs_handle");
        return -1;
    }
    uint picSize = (uint)(
        ((p_dvfs->dvfs_param.sn_max_h * p_dvfs->dvfs_param.sn_max_w + 500000) /
         1000000));
    switch (camera_state) {
    case IS_PREVIEW_BEGIN:
    case IS_CAP_END:
        if (p_dvfs->dvfs_param.channel_x_enble || p_dvfs->dvfs_param.cam_mode) {
            freqValue = CLK_ISP_MAX;
        } else if (picSize <= PIC_SIZE_16M) {
            freqValue = CLK_307M;
        } else {
            freqValue = CLK_468M;
        }
        break;
    case IS_CAP_BEGIN:
        if (p_dvfs->dvfs_param.channel_x_enble || p_dvfs->dvfs_param.cam_mode ||
            picSize >= PIC_SIZE_32M) {
            freqValue = CLK_ISP_MAX;
        } else {
            freqValue = CLK_468M;
        }
        break;
    case IS_CAM_EXIT:
        freqValue = CLK_256M;
        break;
    /*case isRecording:
     {
         //TBD
     }
    case isRecording_Capture:
     {
         //TBD
     }*/
    default:
        CMR_LOGW("unrecognize dvfs cam processing State ");
        break;
    }
    if (p_dvfs->dvfs_status.dmca_if_cur_freq > freqValue) {
        freqValue = p_dvfs->dvfs_status.dmca_if_cur_freq;
        CMR_LOGD("DCAM_IF clock must higehr than ISP clock,DCAM_IF clock is %d",
                 p_dvfs->dvfs_status.dmca_if_cur_freq);
    }

    if (freqValue == p_dvfs->dvfs_status.isp_cur_freq) {
        CMR_LOGD("isp dvfs vaule is already %d,do nothing", freqValue);
        return ret;
    }
    sprintf(mm_set_freq, "/sys/class/devfreq/%s-dvfs/%s_governor/set_work_freq",
            str_module[module - 1], str_module[module - 1]);

    FILE *fp = fopen(mm_set_freq, "wb");
    if (fp == NULL) {
        CMR_LOGE("fail to open file for mm dvfs: %s ", mm_set_freq);
        return 0;
    }
    fprintf(fp, "%d", freqValue);
    fclose(fp);
    fp = NULL;
    p_dvfs->dvfs_status.isp_cur_freq = freqValue;
    if (freqValue < CLK_ISP_MAX) {
        CMR_LOGI("set dvfs-> %s done, freq is %d", str_module[module - 1],
                 freqValue);
    } else {
        CMR_LOGI("set dvfs-> %s done, freq is max", str_module[module - 1]);
    }

    return ret;
}

cmr_int do_debug_dvfs_policy(cmr_handle mm_dvfs_handle,
                             enum DVFS_MM_MODULE module,
                             enum CamProcessingState camera_state) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    char HWDvfsEnable[PROPERTY_VALUE_MAX];
    char IspDebugIndex[PROPERTY_VALUE_MAX];
    char DcamDebugIndex[PROPERTY_VALUE_MAX];

    int isp_hw_dvfs_en = 1;

    property_get("persist.vendor.cam.hw.dvfs.enable", HWDvfsEnable, "true");
    property_get("persist.vendor.cam.isp.dvfs.work.index", IspDebugIndex, "0");
    property_get("persist.vendor.cam.dcam.dvfs.work.index", DcamDebugIndex,
                 "0");

    int get_isp_index = atoi(IspDebugIndex);
    int get_dcam_index = atoi(DcamDebugIndex);

    if (!strcmp(HWDvfsEnable, "false")) {
        isp_hw_dvfs_en = 0;
    }

    if (isp_hw_dvfs_en < 0 || get_isp_index < 0 || get_dcam_index < 0) {
        CMR_LOGI("Invalid param isp_hw_dvfs_en= %d get_isp_index = %d "
                 "get_dcam_index =%d",
                 isp_hw_dvfs_en, get_isp_index, get_dcam_index);
        return -1;
    }

    // isp hw dvfs debug enalbe or disable
    FILE *fp =
        fopen("sys/class/devfreq/isp-dvfs/isp_governor/set_hw_dvfs_en", "wb");
    if (fp == NULL) {
        CMR_LOGE("isp dvfs set_hw_dvfs_en fail to open file for mm dvfs");
    } else {
        fprintf(fp, "%d", isp_hw_dvfs_en);
        CMR_LOGI("mmdvfs_debug hw disable Isp hwen dvfs  set_hw_dvfs_en "
                 "set success");
    }
    if (fp != NULL) {
        fclose(fp);
        fp = NULL;
    }

    // isp dvfs debug by index
    fp = fopen("sys/class/devfreq/isp-dvfs/isp_governor/set_work_index", "wb");
    if (fp == NULL) {
        CMR_LOGE("isp dvfs fail to open file for mm set_work_index");
    } else {
        fprintf(fp, "%d", get_isp_index);
        CMR_LOGI("mmdvfs_debug index = %d Isp set_work_index  set success",
                 get_isp_index);
    }
    if (fp != NULL) {
        fclose(fp);
        fp = NULL;
    }

    // dcam_if dvfs debug by index
    fp = fopen("sys/class/devfreq/dcam-if-dvfs/dcam-if_governor/set_work_index",
               "wb");
    if (fp == NULL) {
        CMR_LOGE("dcam dvfs fail to open file for mm set_work_index");
    } else {
        fprintf(fp, "%d", get_dcam_index);
        CMR_LOGI("mmdvfs_debug index = %d dcam set_work_index set success",
                 get_dcam_index);
    }
    if (fp != NULL) {
        fclose(fp);
        fp = NULL;
    }
    return ret;
}

cmr_int cmr_set_mm_dvfs_policy(cmr_handle oem_handle,
                               enum DVFS_MM_MODULE module,
                               enum CamProcessingState camera_state) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_mm_dvfs *p_dvfs = NULL;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    p_dvfs = (struct class_mm_dvfs *)(cxt->mm_dvfs_cxt.mm_dvfs_handle);
    if (!p_dvfs) {
        CMR_LOGE("invalid mm_dvfs_handle");
        return -1;
    }
    if (p_dvfs->dvfs_param.bps_per_lane < 0) {
        CMR_LOGE("error: invalid bps_per_lane for DcamIF dvfs");
        return ret;
    }
    char Debug_MM_DVfS[PROPERTY_VALUE_MAX];
    property_get("persist.vendor.cam.mm.dvfs.debug", Debug_MM_DVfS, "off");
    if (!strcmp(Debug_MM_DVfS, "on")) {
        module = DVFS_DEBUG;
    }
    switch (module) {
    case DVFS_ISP:
        ret = set_isp_dvfs_policy(cxt->mm_dvfs_cxt.mm_dvfs_handle, module,
                                  camera_state);
        break;
    case DVFS_DCAM_IF:
        ret = set_dcam_dvfs_policy(cxt->mm_dvfs_cxt.mm_dvfs_handle, module,
                                   camera_state);
        break;
    // case DVFS_CPP:
    //    res = setCppDvfsPolicy(module, camParam, camState);
    //    break;
    // case DVFS_JPG:
    //    res = setJpgDvfsPolicy(module, camParam, camState);
    //   break;
    // case DVFS_FD:        //bypass
    //    res = setFdDvfsPolicy(module, camParam, camState);
    //    break;
    // case DVFS_MTX:       //auto tune
    //   break;
    // case DVFS_DCAM_AXI:  //auto tune
    //    break;
    case DVFS_DEBUG:
        ret = do_debug_dvfs_policy(cxt->mm_dvfs_cxt.mm_dvfs_handle, module,
                                   camera_state);
        break;
    default:
        CMR_LOGW("unrecognize mm dvfs module");
    }

    return ret;
}

cmr_int cmr_set_mm_dvfs_param(cmr_handle oem_handle,
                              struct prev_sn_param_dvfs_type dvfs_param) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct class_mm_dvfs *p_dvfs = NULL;
    p_dvfs = (struct class_mm_dvfs *)(cxt->mm_dvfs_cxt.mm_dvfs_handle);
    if (!p_dvfs && (mm_dvfs_instance != p_dvfs)) {
        CMR_LOGE("invalid mm_dvfs_handle");
        return -1;
    }
    pthread_mutex_lock(&mm_dvfs_instance->mm_dvfs_mutex);
    if (dvfs_param.bps_per_lane < 0 || dvfs_param.sn_max_w < 0) {
        CMR_LOGE("Invalid param");
        pthread_mutex_unlock(&p_dvfs->mm_dvfs_mutex);
        return -1;
    } else if ((dvfs_param.sn_max_w < p_dvfs->dvfs_param.sn_max_w ||
                dvfs_param.sn_max_h < p_dvfs->dvfs_param.sn_max_h) &&
               dvfs_param.is_high_fps == p_dvfs->dvfs_param.is_high_fps &&
               p_dvfs->dvfs_param.sn_max_w != 0) {
        CMR_LOGD("last sensor param is bigger or no changed, do nothing ,just "
                 "return");
        pthread_mutex_unlock(&p_dvfs->mm_dvfs_mutex);
        return ret;
    }
    CMR_LOGD("bps_per_lane  %d lane_num %d sn_max_w %d sn_max_h %d is_high_fps "
             "%d cam_mode %d",
             dvfs_param.bps_per_lane, dvfs_param.lane_num, dvfs_param.sn_max_w,
             dvfs_param.sn_max_h, dvfs_param.is_high_fps, dvfs_param.cam_mode);
    p_dvfs->dvfs_param = dvfs_param;
    pthread_mutex_unlock(&mm_dvfs_instance->mm_dvfs_mutex);
    return ret;
}
