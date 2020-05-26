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

#define LOG_TAG "camera_lightportrait"

#include <cutils/properties.h>
#include <stdlib.h>
#include <utils/Log.h>
#include "camera_light_portrait.h"
#include <time.h>

char value[PROPERTY_VALUE_MAX];
char debug_value[PROPERTY_VALUE_MAX];
int dumpLptFrameCount = 0;

void init_lpt_options (struct class_lpt *lpt) {
    if (!lpt) {
        LPT_InitPortraitOption(&(lpt->lpt_option));
    }
}

void create_lpt_handle(struct class_lpt *lpt, int workMode, int threadNum) {//workMode: 0-> capture, 1->preview/video

    if (lpt->hSprdLPT == 0) {
        ALOGD("init_lpt_handle to LPT_CreatePortraitHandle");
        if (LPT_OK != LPT_CreatePortraitHandle(&(lpt->hSprdLPT), workMode,
                                           threadNum)) {
            ALOGE("LPT_CreatePortraitHandle() Error");
            return;
        }
    }
    if (workMode == 1) {//preview or video
        lpt->lpt_mode = LPT_WORKMODE_MOVIE;
    } else {
        lpt->lpt_mode = LPT_WORKMODE_STILL;//capture
    }
}

void deinit_lpt_handle(struct class_lpt *lpt) {
    dumpLptFrameCount = 0;
    if (lpt->hSprdLPT != 0) {
        ALOGD("deinit_lpt_handle to LPT_DeletePortraitHandle");
        LPT_DeletePortraitHandle(&(lpt->hSprdLPT));
        lpt->hSprdLPT = NULL;
    }
    lpt->noFaceFrmCnt = 0 ;
}

void construct_lpt_options(struct class_lpt *lpt, struct lpt_options lptOptions) {
    if (!lpt) {
        ALOGD("construct_lpt_options lightPortrait is NULL");
        return;
    }

    /* set the lpt options*/
    lpt->lpt_option.lightPortraitType = lptOptions.lightPortraitType;
    lpt->lpt_option.lightCursor = lptOptions.lightCursor;
    lpt->lpt_option.lightWeight = lptOptions.lightWeight;
    lpt->lpt_option.lightSplitMode = lptOptions.lightSplitMode;
    lpt->lpt_option.debugMode = lptOptions.debugMode;
    lpt->lpt_option.cameraWork = lptOptions.cameraWork;
    lpt->lpt_option.cameraBV = lptOptions.cameraBV;
    lpt->lpt_option.cameraISO = lptOptions.cameraISO;
    lpt->lpt_option.cameraCT = lptOptions.cameraCT;
    char lpt_prop[PROPERTY_VALUE_MAX];
    property_get("persist.vendor.cam.lpt.level.enable", lpt_prop, "0");
    if (!(strcmp("1", lpt_prop))){
        lpt->lpt_option.platformInfo = 1;
    }else {
        lpt->lpt_option.platformInfo = 0;
    }
    property_get("ro.debuggable", debug_value, "0");
    if (!strcmp(debug_value, "1")) {
        ALOGD("sprdlpt,  construct_lpt_options cameraWork:%d, cameraBV:%d, cameraISO:%d, cameraCT:%d",
            lpt->lpt_option.cameraWork, lpt->lpt_option.cameraBV,lpt->lpt_option.cameraISO,lpt->lpt_option.cameraCT);
    }

}

void construct_lpt_mask(struct class_lpt *lpt, int pWidth, int pHeight, unsigned char *pData) {
    if (!lpt) {
       ALOGD("construct_lpt_options lightPortrait is null");
       return;
    }

    lpt->lpt_mask.width = pWidth;
    lpt->lpt_mask.height = pHeight;
    lpt->lpt_mask.data = pData;

    property_get("ro.debuggable", debug_value, "0");
    if (!strcmp(debug_value, "1")) {
        ALOGD("sprdlpt,  lpt_mask width:%d, height:%d. ",
              lpt->lpt_mask.width, lpt->lpt_mask.height);
    }

}

void construct_lpt_dfaInfo(struct class_lpt *lpt, float picPitch, float picYaw,
                          float picRoll, float pitT3d[],int tLen, float picScale, float picR[][3],int rLen,float picAlpha_shp[],int sLen, float picAlpha_exp[],int eLen) {
    if (!lpt) {
       ALOGD("construct_lpt_dfaInfo dfa is null");
       return;
    }

    lpt->lpt_dfa.pitch = picPitch;
    lpt->lpt_dfa.yaw = picYaw;
    lpt->lpt_dfa.roll = picRoll;
    for (int i=0; i <tLen; i++){
      lpt->lpt_dfa.t3d[i] = pitT3d[i];
    }
    
    lpt->lpt_dfa.scale = picScale;
    for(int j=0; j< rLen; j++){
      for(int k=0; k<3; k++){
        lpt->lpt_dfa.R[j][k] = picR[j][k];
      }
    }
    for(int m=0; m<sLen; m++ ){
      lpt->lpt_dfa.alpha_shp[m] = picAlpha_shp[m];
    }
    for(int n=0; n<eLen; n++ ){
      lpt->lpt_dfa.alpha_exp[n] = picAlpha_exp[n];
    }
}

void construct_lpt_face(struct class_lpt *lpt, int j, int sx, int sy,
                       int ex, int ey, int yaw, int roll, int fScore,
                       unsigned char attriRace, unsigned char attriGender, unsigned char attriAge) {
    if (!lpt) {
        ALOGE("construct_lpt_face class_lpt is null");
        return;
    }

    lpt->lpt_face[j].x = sx;
    lpt->lpt_face[j].y = sy;
    lpt->lpt_face[j].width = ex - sx;
    lpt->lpt_face[j].height = ey - sy;
    lpt->lpt_face[j].yawAngle = yaw;
    lpt->lpt_face[j].rollAngle = roll;
    lpt->lpt_face[j].score = fScore;
    lpt->lpt_face[j].faceAttriRace = attriRace;
    lpt->lpt_face[j].faceAttriGender= attriGender;
    lpt->lpt_face[j].faceAttriAge= attriAge;
    property_get("ro.debuggable", debug_value, "0");
    if (!strcmp(debug_value, "1")) {
        ALOGD("sprdlpt,  lpt_face[%d] x:%d, y:%d, w:%d, h:%d , roll:%d, yaw:%d, scoll:%d.",
                j, lpt->lpt_face[j].x, lpt->lpt_face[j].y,
                lpt->lpt_face[j].width, lpt->lpt_face[j].height,
                lpt->lpt_face[j].rollAngle,lpt->lpt_face[j].yawAngle,
                lpt->lpt_face[j].score);
    }
}

void construct_lpt_image(struct class_lpt *lpt, int picWidth,
                        int picHeight, unsigned char *addrY,
                        unsigned char *addrU, int format) {
    if (!lpt) {
        ALOGE("construct_lpt_image is null");
        return;
    }

    lpt->lpt_image.width = picWidth;
    lpt->lpt_image.height = picHeight;
    lpt->lpt_image.yData = addrY;
    lpt->lpt_image.uvData = addrU;
    lpt->lpt_image.format = (format == 0)
                                ? LPT_YUV420_FORMAT_CBCR
                                : LPT_YUV420_FORMAT_CRCB; // NV12 : NV21
    property_get("ro.debuggable", debug_value, "0");
    if (!strcmp(debug_value, "1")) {
        ALOGD("sprdlpt,  construct_lpt_image width:%d, height:%d",picWidth,picHeight);
    }

}

void save_yuv_data(int num, int width, int height, unsigned char *addr_y,
                   char flag[10]) {
    char file_name[256];
    char temp_time[80];
    char tmp_str[20];
    FILE *fp = NULL;
    struct tm *p;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int second = tv.tv_sec;
    int millisecond = tv.tv_usec / 1000;
    p = localtime((&tv.tv_sec));
    memset(file_name, '\0', 80);
    strcpy(file_name, "/data/vendor/cameraserver/");
    sprintf(temp_time , "%04d%02d%02d%02d%02d%02d_%03d_" ,(1900+p->tm_year),
                            (1+p->tm_mon) , p->tm_mday , p->tm_hour, p->tm_min , second,millisecond);
    strcat(file_name, temp_time);
    sprintf(tmp_str, "%03d-%s-%d", num, flag, width);
    strcat(file_name, tmp_str);
    strcat(file_name, "x");
    sprintf(tmp_str, "%d.nv21", height);
    strcat(file_name, tmp_str);

    ALOGD("file name %s", file_name);
    fp = fopen(file_name, "wb");

    if (NULL == fp) {
        ALOGE("cannot open file:%s \n", file_name);
        return;
    }

    fwrite(addr_y, 1, width * height * 3 / 2, fp);
    fclose(fp);
}

int do_image_lpt(struct class_lpt *lpt, int faceCount) {
    int retVal = 0;
    struct timespec start_time, end_time;
    char dump_value[PROPERTY_VALUE_MAX];
    char debug_value[PROPERTY_VALUE_MAX];
    unsigned int duration;

    if (!lpt) {
        ALOGE("do_image_lpt lpt is null");
        return -1;
    }
    clock_gettime(CLOCK_BOOTTIME, &start_time);
    /*wait first face frame*/
    if (0 == lpt->isFaceGot && faceCount > 0) {
            lpt->isFaceGot = 1;
    }
    if (lpt->isFaceGot == 1) {
        if (faceCount == 0) {
            if (lpt->noFaceFrmCnt < 100)
                lpt->noFaceFrmCnt++;
        } else
            lpt->noFaceFrmCnt = 0;

    }
    property_get("debug.camera.light.dump.frame", dump_value, "null");
    if (!strcmp(dump_value, "lpt")) {
         save_yuv_data(dumpLptFrameCount, lpt->lpt_image.width,
                      lpt->lpt_image.height,
                      lpt->lpt_image.yData, "be");
    }
    property_get("ro.debuggable", debug_value, "0");

    if (lpt->lpt_mode == 1) {
        if (!strcmp(debug_value, "1")){
            ALOGD("do_image_lpt preview transfer mask null to arithmetic");
        }
        retVal = LPT_lightPortrait_YUV420SP(lpt->hSprdLPT, &(lpt->lpt_image),
                      NULL,&(lpt->lpt_option),lpt->lpt_face,
                      faceCount,&(lpt->lpt_dfa));
    }else{
        retVal = LPT_lightPortrait_YUV420SP(lpt->hSprdLPT, &(lpt->lpt_image),
                      &(lpt->lpt_mask),&(lpt->lpt_option),lpt->lpt_face,
                      faceCount,&(lpt->lpt_dfa));
    }

    if (!strcmp(dump_value, "lpt")) {
        save_yuv_data(dumpLptFrameCount, lpt->lpt_image.width,
                      lpt->lpt_image.height,
                      lpt->lpt_image.yData, "af");
        dumpLptFrameCount++;
    }

    clock_gettime(CLOCK_BOOTTIME, &end_time);
    duration = (end_time.tv_sec - start_time.tv_sec) * 1000 +
               (end_time.tv_nsec - start_time.tv_nsec) / 1000000;
    ALOGV("LPT_lightPortrait_YUV420SP duration is %d ms", duration);

    if (!strcmp(debug_value, "1")) {
        ALOGD("SPRD_LPT: LPT_lightPortrait_YUV420SP duration is %d ms", duration);
    }

    if (retVal != 0) {
        ALOGE("FACE_LPT ERROR!, ret is %d", retVal);
    }
    return retVal;

}
#endif