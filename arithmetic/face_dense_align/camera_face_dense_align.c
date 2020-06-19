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

#if defined(CONFIG_PORTRAIT_SUPPORT) || defined(CONFIG_PORTRAIT_SINGLE_SUPPORT)

#define LOG_TAG "camera_dfa"

#include <cutils/properties.h>
#include <stdlib.h>
#include <utils/Log.h>
#include "camera_face_dense_align.h"
#include <time.h>
#define NUM_LEVELS 11
#define NUM_TYPES 3
int dumpFrameCount = 0;
int dfa_mode = 1;
char value[PROPERTY_VALUE_MAX];

void create_dfa_handle(struct class_dfa *dfa, int workMode){
    DFA_InitOption(&(dfa->dfa_option));
    dfa_mode = workMode;//0 for capture,1 for preview
    ALOGD("create_dfa_handle dfa_mode: %d",dfa_mode);
    if (dfa->hSprdDfa == 0) {
        ALOGD("create_dfa_handle to DFA_CreateHandle");
        if (DFA_OK != DFA_CreateHandle(&(dfa->hSprdDfa), &(dfa->dfa_option))){
            ALOGE("DFA_CreateHandle() Error");
            return;
        }
    }
}
void deinit_dfa_handle(struct class_dfa *dfa){
    if (dfa->hSprdDfa != 0 ) {
        ALOGD("deinit_dfa_handle to DFA_DeleteHandle begin");
        DFA_DeleteHandle(&(dfa->hSprdDfa));
        dfa->hSprdDfa = NULL;
    }
    dfa_mode = 1;
}

void construct_dfa_yuv420sp(struct class_dfa *dfa, int picWidth,
                            int picHeight, unsigned char *addrY,
                            unsigned char *addrUV, int format){
    if (!dfa){
        ALOGE("construct_dfa_yuv420sp dfa is null");
        return;
    }
    dfa->dfa_image_sp.width = picWidth;
    dfa->dfa_image_sp.height = picHeight;
    dfa->dfa_image_sp.yData = addrY;
    dfa->dfa_image_sp.uvData = addrUV;
    dfa->dfa_image_sp.format = format;//NV21-> 1, NV12-> 0
    char debug_value[PROPERTY_VALUE_MAX];
    property_get("ro.debuggable", debug_value, "0");
    if (!strcmp(debug_value, "1")) {
        ALOGD("construct_dfa_yuv420sp image info width: %d, height: %d",picWidth,picHeight);
    }

}

void construct_dfa_yuv420(struct class_dfa *dfa, int picWidth,
                          int picHeight, int picYRowStride, int picUVRowStride,
                          int picUVPixelStride,unsigned char *addrY,
                          unsigned char *addrU, unsigned char *addrV){
    if (!dfa){
        ALOGE("construct_dfa_yuv420 dfa is null");
        return;
    }
    dfa->dfa_image.width = picWidth;
    dfa->dfa_image.height = picHeight;
    dfa->dfa_image.yData = addrY;
    dfa->dfa_image.uData = addrU;
    dfa->dfa_image.vData = addrV;
    dfa->dfa_image.yRowStride = picYRowStride;
    dfa->dfa_image.uvRowStride = picUVRowStride;
    dfa->dfa_image.uvPixelStride = picUVPixelStride;
    char debug_value[PROPERTY_VALUE_MAX];
    property_get("ro.debuggable", debug_value, "0");
    if (!strcmp(debug_value, "1")) {
        ALOGD("construct_dfa_yuv420 image width:%d, height:%d, picYRowStride:%d, picUVRowStride:%d, picUVPixelStride:%d",
        dfa->dfa_image.width,dfa->dfa_image.height, dfa->dfa_image.yRowStride,
        dfa->dfa_image.uvRowStride, dfa->dfa_image.uvPixelStride);
    }


}

void construct_dfa_bgr(struct class_dfa *dfa, int picWidth,
                          int picHeight,unsigned char *pBdata,
                          unsigned char *pGdata, unsigned char *pRdata){
    if (!dfa){
        ALOGE("construct_dfa_bgr dfa is null");
        return;
    }
    dfa->dfa_bgr.width = picWidth;
    dfa->dfa_bgr.height = picHeight;
    dfa->dfa_bgr.bData = pBdata;
    dfa->dfa_bgr.gData = pGdata;
    dfa->dfa_bgr.rData = pRdata;
}

void construct_dfa_face(struct class_dfa *dfa, int i, int rX, int rY, int rWidth,
                        int rHeight,int rRollAngle, unsigned char rType){
    if (!dfa) {
        ALOGE("construct_dfa_face dfa is null");
        return;
    }
    if (rType == 0) {//type:0->FD
        dfa->dfa_face[i].roi_type = rType;
        dfa->dfa_face[i].roi_box[0] = rX;
        dfa->dfa_face[i].roi_box[1] = rY;
        dfa->dfa_face[i].roi_box[2] = rWidth;
        dfa->dfa_face[i].roi_box[3] = rHeight;
        dfa->dfa_face[i].rollAngle = rRollAngle;
        char debug_value[PROPERTY_VALUE_MAX];
        property_get("ro.debuggable", debug_value, "0");
        if (!strcmp(debug_value, "1")) {
            ALOGD("construct_dfa_face face[%d] get FD info x: %d, y: %d, width: %d, height: %d, rollangle: %d ",
            i, dfa->dfa_face[i].roi_box[0],dfa->dfa_face[i].roi_box[1], dfa->dfa_face[i].roi_box[2],
            dfa->dfa_face[i].roi_box[3],dfa->dfa_face[i].rollAngle);
        }
    }
}

void do_dfa_image_yuv420sp(struct class_dfa *dfa, int faceCount,DFA_RESULT *dfa_result){
    struct timespec start_time, end_time;
    unsigned int duration;
    char debug_value[PROPERTY_VALUE_MAX];
    int retVal = 0;
    if (!dfa) {
        ALOGE("do_dfa_image_yuv420sp dfa is null");
        return;
    }
    char dfa_prop[PROPERTY_VALUE_MAX];
    property_get("persist.vendor.cam.lpt.level.enable", dfa_prop, "0");
    clock_gettime(CLOCK_BOOTTIME, &start_time);
    if ((!(strcmp("1", dfa_prop)) && (dfa_mode == 0)) || (!(strcmp("0", dfa_prop)))){
        if (faceCount > 0) {
            retVal = DFA_Run_YUV420SP(dfa->hSprdDfa,
                                   &(dfa->dfa_image_sp),
                                   dfa->dfa_face, dfa_result);
        }

    }
    clock_gettime(CLOCK_BOOTTIME, &end_time);
    duration = (end_time.tv_sec - start_time.tv_sec) * 1000 +
               (end_time.tv_nsec - start_time.tv_nsec) / 1000000;
    ALOGV("DFA_Run_YUV420SP duration is %d ms", duration);

    property_get("ro.debuggable", debug_value, "0");
    if (!strcmp(debug_value, "1")) {
        ALOGD("SPRD_DFA: DFA_Run_YUV420SP duration is %d ms", duration);
    }
    if (retVal != 0) {
        return;
        ALOGE("do_dfa_image_yuv420sp ERROR!, ret is %d", retVal);
    }
}

void do_dfa_image_yuv420(struct class_dfa *dfa, int faceCount, DFA_RESULT *dfa_result){
    struct timespec start_time, end_time;
    char debug_value[PROPERTY_VALUE_MAX];
    unsigned int duration;
    int retVal = 0;

    if (!dfa) {
        ALOGE("do_dfa_image_yuv420 dfa is null");
        return;
    }
    char dfa_prop[PROPERTY_VALUE_MAX];
    property_get("persist.vendor.cam.lpt.level.enable", dfa_prop, "0");
    clock_gettime(CLOCK_BOOTTIME, &start_time);
    if ((!(strcmp("1", dfa_prop)) && (dfa_mode == 0)) || (!(strcmp("0", dfa_prop)))){
        if (faceCount > 0) {
            retVal = DFA_Run_YUV420(dfa->hSprdDfa,
                                 &(dfa->dfa_image),
                                 dfa->dfa_face, dfa_result);
        }
    }
    clock_gettime(CLOCK_BOOTTIME, &end_time);
    duration = (end_time.tv_sec - start_time.tv_sec) * 1000 +
               (end_time.tv_nsec - start_time.tv_nsec) / 1000000;
    ALOGV("DFA_Run_YUV420 duration is %d ms", duration);

    property_get("ro.debuggable", debug_value, "0");
    if (!strcmp(debug_value, "1")) {
        ALOGD("SPRD_DFA: DFA_Run_YUV420 duration is %d ms", duration);
     }
    if (retVal != 0) {
        return ;
        ALOGE("do_dfa_image_yuv420 ERROR!, ret is %d", retVal);
    }
}

void do_dfa_image_bgr(struct class_dfa *dfa, int faceCount, DFA_RESULT *dfa_result){
    struct timespec start_time, end_time;
    char debug_value[PROPERTY_VALUE_MAX];
    unsigned int duration;
    int retVal = 0;

    if (!dfa) {
        ALOGE("do_dfa_image_yuv420 dfa is null");
        return ;
    }
    char dfa_prop[PROPERTY_VALUE_MAX];
    property_get("persist.vendor.cam.lpt.level.enable", dfa_prop, "0");
    clock_gettime(CLOCK_BOOTTIME, &start_time);
    if ((!(strcmp("1", dfa_prop)) && (dfa_mode == 0)) || (!(strcmp("0", dfa_prop)))){
        if (faceCount > 0) {
            retVal = DFA_Run_BGR(dfa->hSprdDfa,
                         &(dfa->dfa_bgr),
                         dfa->dfa_face, dfa_result);
        }
    }
    clock_gettime(CLOCK_BOOTTIME, &end_time);
    duration = (end_time.tv_sec - start_time.tv_sec) * 1000 +
               (end_time.tv_nsec - start_time.tv_nsec) / 1000000;
    ALOGV("DFA_Run_BGR duration is %d ms", duration);

    property_get("ro.debuggable", debug_value, "0");
    if (!strcmp(debug_value, "1")) {
        ALOGD("SPRD_DFA: DFA_Run_BGR duration is %d ms", duration);
     }
    if (retVal != 0) {
        return ;
        ALOGE("do_dfa_image_bgr ERROR!, ret is %d", retVal);
    }
}
#endif
