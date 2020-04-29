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

#if defined(CONFIG_FACE_BEAUTY)

#define LOG_TAG "camera_fb"

#include <cutils/properties.h>
#include <stdlib.h>
#include <utils/Log.h>
#include "camera_face_beauty.h"
#include <time.h>
#define NUM_LEVELS 11
#define NUM_TYPES 3
int dumpFrameCount = 0;
char value[PROPERTY_VALUE_MAX];
int faceLevelMap = 0;
struct facebeauty_param_t fbParam;
int lightPortraitType = 0;
//fb_beauty_mask *fbMask = NULL;

void init_fb_handle(struct class_fb *faceBeauty, int workMode, int threadNum,fb_chipinfo chipinfo) {
    property_get("persist.vendor.cam.facebeauty.corp", faceBeauty->sprdAlgorithm,
                 "1");
    if (!strcmp(faceBeauty->sprdAlgorithm, "2")) {
        if (faceBeauty->hSprdFB == 0) {
            ALOGD("init_fb_handle to FB_CreateBeautyHandle");
            faceBeauty->firstFrm = 1;
            if (FB_OK != FB_CreateBeautyHandle(&(faceBeauty->hSprdFB), workMode,
                                               threadNum)) {
                ALOGE("FB_CreateBeautyHandle() Error");
                return;
            }
        }
        if (workMode == 1) {
            faceBeauty->fb_mode = 1;
        } else {
            faceBeauty->fb_mode = 0;
        }
#ifdef CONFIG_CAMERA_BEAUTY_FOR_SHARKL5PRO
        faceBeauty->fb_option.fbVersion = 1;
#else
        faceBeauty->fb_option.fbVersion = 0;
#endif
        ALOGD("fb_option.fbVersion = %d",faceBeauty->fb_option.fbVersion);
    }
}

void deinit_fb_handle(struct class_fb *faceBeauty) {
    dumpFrameCount = 0;
    property_get("persist.vendor.cam.facebeauty.corp", faceBeauty->sprdAlgorithm,
                 "1");
    if (!strcmp(faceBeauty->sprdAlgorithm, "2")) {
        if (faceBeauty->hSprdFB != 0) {
            ALOGD("deinit_fb_handle to FB_DeleteBeautyHandle begin");
            FB_DeleteBeautyHandle(&(faceBeauty->hSprdFB));
            faceBeauty->hSprdFB = NULL;
        }
        faceBeauty->noFaceFrmCnt = 0;
    }
    faceLevelMap = 0;
}

void construct_fb_face(struct class_fb *faceBeauty, struct fb_beauty_face_t faceinfo) {
    char debug_value[PROPERTY_VALUE_MAX];
    if (!faceBeauty) {
        ALOGE("construct_fb_face faceBeauty is null");
        return;
    }

    property_get("persist.vendor.cam.facebeauty.corp", faceBeauty->sprdAlgorithm,
                 "1");
    if (!strcmp(faceBeauty->sprdAlgorithm, "2")) {
        faceBeauty->fb_face[faceinfo.idx].x = faceinfo.startX;
        faceBeauty->fb_face[faceinfo.idx].y = faceinfo.startY;
        faceBeauty->fb_face[faceinfo.idx].width = faceinfo.endX - faceinfo.startX;
        faceBeauty->fb_face[faceinfo.idx].height = faceinfo.endY - faceinfo.startY;
        faceBeauty->fb_face[faceinfo.idx].rollAngle = faceinfo.angle;
        faceBeauty->fb_face[faceinfo.idx].yawAngle = faceinfo.pose;
        faceBeauty->fb_face[faceinfo.idx].score = faceinfo.score;
        faceBeauty->fb_face[faceinfo.idx].faceAttriRace = faceinfo.faceAttriRace;
        faceBeauty->fb_face[faceinfo.idx].faceAttriGender= faceinfo.faceAttriGender;
        faceBeauty->fb_face[faceinfo.idx].faceAttriAge= faceinfo.faceAttriAge;
        property_get("ro.debuggable", debug_value, "0");
        if (!strcmp(debug_value, "1")) {
            ALOGD("sprdfb, fb_face[%d] x:%d, y:%d, w:%d, h:%d , angle:%d, pose%d.",
                    faceinfo.idx, faceBeauty->fb_face[faceinfo.idx].x, faceBeauty->fb_face[faceinfo.idx].y,
                    faceBeauty->fb_face[faceinfo.idx].width, faceBeauty->fb_face[faceinfo.idx].height,
                    faceinfo.angle, faceinfo.pose);
        }
    }
}

void construct_fb_image(struct class_fb *faceBeauty, int picWidth,
                        int picHeight, unsigned char *addrY,
                        unsigned char *addrU, int format) {
    if (!faceBeauty) {
        ALOGE("construct_fb_image faceBeauty is null");
        return;
    }
    property_get("persist.vendor.cam.facebeauty.corp", faceBeauty->sprdAlgorithm,
                 "1");
    if (!strcmp(faceBeauty->sprdAlgorithm, "2")) {
        faceBeauty->fb_image.width = picWidth;
        faceBeauty->fb_image.height = picHeight;
        faceBeauty->fb_image.yData = addrY;
        faceBeauty->fb_image.uvData = addrU;
        faceBeauty->fb_image.format = (format == 1)
                                          ? YUV420_FORMAT_CBCR
                                          : YUV420_FORMAT_CRCB; // NV12 : NV21
    } 
}

void construct_fb_map(facebeauty_param_info_t *facemap){
    ALOGI("construct_fb_map");
    faceLevelMap++;
    fbParam = facemap->cur.fb_param[FB_SKIN_DEFAULT];
}

void construct_fb_portraitType(int portraitType){
    lightPortraitType = portraitType;
}
/*void construct_fb_mask(struct class_fb *faceBeauty, fb_beauty_mask mFbMask) {
    if (!faceBeauty) {
        ALOGE("construct_fb_mask faceBeauty is null");
        return;
    }
    fbMask = &mFbMask;
    char debug_value[PROPERTY_VALUE_MAX];
    property_get("ro.debuggable", debug_value, "0");
    if (!strcmp(debug_value, "1")) {
        ALOGD("construct_fb_mask: width:%d, height:%d", fbMask->fb_mask.width,fbMask->fb_mask.height);
    }
fa
}*/

void construct_fb_level(struct class_fb *faceBeauty,
                        struct face_beauty_levels beautyLevels) {
    if (!faceBeauty) {
        ALOGE("construct_fb_level faceBeauty is null");
        return;
    }

    // convert the skin_level set by APP to skinLevel & smoothLevel according to
    // the table saved.
    {
        beautyLevels.smoothLevel = beautyLevels.smoothLevel > 10 ? 10 : beautyLevels.smoothLevel;
        beautyLevels.brightLevel = beautyLevels.brightLevel > 10 ? 10 : beautyLevels.brightLevel;
        beautyLevels.slimLevel = beautyLevels.slimLevel > 10 ? 10 : beautyLevels.slimLevel;
        beautyLevels.largeLevel = beautyLevels.largeLevel > 10 ? 10 : beautyLevels.largeLevel;
        beautyLevels.lipLevel = beautyLevels.lipLevel > 10 ? 10 : beautyLevels.lipLevel;
        beautyLevels.skinLevel = beautyLevels.skinLevel > 10 ? 10 : beautyLevels.skinLevel;
    }

    char isDebug[PROPERTY_VALUE_MAX];
    property_get("persist.vendor.cam.beauty.debug", isDebug, "0");
    property_get("persist.vendor.cam.facebeauty.corp", faceBeauty->sprdAlgorithm,
                 "1");
    if (!strcmp(faceBeauty->sprdAlgorithm, "2")) {
        if (!faceLevelMap) {
            unsigned char map_pictureSkinSmoothLevel[NUM_LEVELS] = {
                0, 6, 6, 8, 8, 10, 10, 12, 12, 14, 14};
            unsigned char map_previewSkinSmoothLevel[NUM_LEVELS] = {
                0, 2, 2, 3, 3, 4, 4, 6, 6, 8, 8};
            unsigned char map_skinBrightLevel[NUM_LEVELS] = {0, 2, 4, 6, 6, 8,
                                                         8, 10, 10, 12, 12};
            unsigned char map_slimFaceLevel[NUM_LEVELS] = {0, 1, 2, 3, 4, 5,
                                                       6, 7, 8, 9, 10};
            unsigned char map_largeEyeLevel[NUM_LEVELS] = {0, 1, 2, 3, 4, 5,
                                                       6, 7, 8, 9, 10};
            unsigned char map_skinSmoothRadiusCoeff[NUM_LEVELS] = {
                55, 55, 55, 50, 50, 40, 40, 30, 30, 20, 20};
            unsigned char map_pictureSkinTextureHiFreqLevel[NUM_LEVELS] = {
                0, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10};
            unsigned char map_pictureSkinTextureLoFreqLevel[NUM_LEVELS] = {
                4, 4, 4, 3, 3, 2, 2, 1, 1, 0, 0};
            unsigned char map_lipColorLevel[NUM_LEVELS] = {0, 2, 3, 5, 6, 8, 9, 10, 11, 12, 12};
            unsigned char map_skinColorLevel[NUM_LEVELS] = {0, 2, 3, 5, 6, 8, 9, 10, 11, 12, 12};

            faceBeauty->fb_option.skinBrightLevel =
                map_skinBrightLevel[beautyLevels.brightLevel];
            faceBeauty->fb_option.slimFaceLevel =
                map_slimFaceLevel[beautyLevels.slimLevel];
            faceBeauty->fb_option.largeEyeLevel =
                map_largeEyeLevel[beautyLevels.largeLevel];
            faceBeauty->fb_option.lipColorLevel =
                map_lipColorLevel[beautyLevels.lipLevel];
            faceBeauty->fb_option.skinColorLevel =
                map_skinColorLevel[beautyLevels.skinLevel];

           // We don't use the following options
            faceBeauty->fb_option.removeBlemishFlag = 0;
            faceBeauty->fb_option.removeBlemishFlag = beautyLevels.blemishLevel;
            faceBeauty->fb_option.lipColorType = beautyLevels.lipColor;
            faceBeauty->fb_option.skinColorType = beautyLevels.skinColor;
            faceBeauty->fb_option.blemishSizeThrCoeff = 14;

            faceBeauty->fb_option.cameraWork = FB_CAMERA_REAR;
            faceBeauty->fb_option.cameraBV = beautyLevels.cameraBV;
            faceBeauty->fb_option.cameraISO = 0;
            faceBeauty->fb_option.cameraCT = 0;

            if (faceBeauty->fb_mode == 1) {
                faceBeauty->fb_option.skinSmoothRadiusCoeff =
                    map_skinSmoothRadiusCoeff[beautyLevels.smoothLevel];
                faceBeauty->fb_option.skinSmoothLevel =
                    map_previewSkinSmoothLevel[beautyLevels.smoothLevel];
                faceBeauty->fb_option.skinTextureHiFreqLevel = 0;
                faceBeauty->fb_option.skinTextureLoFreqLevel = 0;
            } else {
                faceBeauty->fb_option.skinSmoothRadiusCoeff = 55;
                faceBeauty->fb_option.skinSmoothLevel =
                    map_pictureSkinSmoothLevel[beautyLevels.smoothLevel];
                faceBeauty->fb_option.skinTextureHiFreqLevel =
                    map_pictureSkinTextureHiFreqLevel[beautyLevels.smoothLevel];
                faceBeauty->fb_option.skinTextureLoFreqLevel =
                    map_pictureSkinTextureLoFreqLevel[beautyLevels.smoothLevel];
            }
        } else {

            /*set default value*/
            faceBeauty->fb_option.skinBrightLevel = fbParam.fb_layer.skinBrightDefaultLevel;
            faceBeauty->fb_option.slimFaceLevel = fbParam.fb_layer.slimFaceDefaultLevel;
            faceBeauty->fb_option.largeEyeLevel = fbParam.fb_layer.largeEyeDefaultLevel;
            faceBeauty->fb_option.lipColorLevel = fbParam.fb_layer.lipColorDefaultLevel;
            faceBeauty->fb_option.skinColorLevel = fbParam.fb_layer.skinColorDefaultLevel;
            faceBeauty->fb_option.removeBlemishFlag = fbParam.removeBlemishFlag;
            faceBeauty->fb_option.lipColorType = fbParam.lipColorType;
            faceBeauty->fb_option.skinColorType = fbParam.skinColorType;
            faceBeauty->fb_option.blemishSizeThrCoeff = fbParam.blemishSizeThrCoeff;

            faceBeauty->fb_option.skinSmoothRadiusCoeff = fbParam.fb_layer.skinSmoothRadiusDefaultLevel;
            faceBeauty->fb_option.skinSmoothLevel = fbParam.fb_layer.skinSmoothDefaultLevel;
            faceBeauty->fb_option.skinTextureHiFreqLevel = fbParam.fb_layer.skinTextureHiFreqDefaultLevel;
            faceBeauty->fb_option.skinTextureLoFreqLevel = fbParam.fb_layer.skinTextureLoFreqDefaultLevel;

            /*set value*/
            faceBeauty->fb_option.skinBrightLevel = fbParam.fb_layer.skinBrightLevel[beautyLevels.brightLevel/2];
            faceBeauty->fb_option.slimFaceLevel = fbParam.fb_layer.slimFaceLevel[beautyLevels.slimLevel/2];
            faceBeauty->fb_option.largeEyeLevel = fbParam.fb_layer.largeEyeLevel[beautyLevels.largeLevel/2];
            faceBeauty->fb_option.lipColorLevel = fbParam.fb_layer.lipColorLevel[beautyLevels.lipLevel/2];
            faceBeauty->fb_option.skinColorLevel = fbParam.fb_layer.skinColorLevel[beautyLevels.skinLevel/2];

            faceBeauty->fb_option.removeBlemishFlag = beautyLevels.blemishLevel;
            faceBeauty->fb_option.lipColorType = beautyLevels.lipColor;
            faceBeauty->fb_option.skinColorType = beautyLevels.skinColor;

            faceBeauty->fb_option.skinSmoothRadiusCoeff = fbParam.fb_layer.skinSmoothRadiusCoeff[beautyLevels.smoothLevel/2];
            faceBeauty->fb_option.skinSmoothLevel = fbParam.fb_layer.skinSmoothLevel[beautyLevels.smoothLevel/2];
            faceBeauty->fb_option.skinTextureHiFreqLevel = fbParam.fb_layer.skinTextureHiFreqLevel[beautyLevels.smoothLevel/2];
            faceBeauty->fb_option.skinTextureLoFreqLevel = fbParam.fb_layer.skinTextureLoFreqLevel[beautyLevels.smoothLevel/2];

            faceBeauty->fb_option.cameraWork = beautyLevels.cameraWork;
            faceBeauty->fb_option.cameraBV = beautyLevels.cameraBV;
            faceBeauty->fb_option.cameraISO = beautyLevels.cameraISO;
            faceBeauty->fb_option.cameraCT = beautyLevels.cameraCT;
        }
        char isLevelDebug[PROPERTY_VALUE_MAX];
        property_get("persist.vendor.cam.beauty.level.debug", isLevelDebug, "0");
        if(!strcmp(isLevelDebug, "1")){
           ALOGD("SPRD_FB: skinBrightLevel: %d, slimFaceLevel: %d, "
                          "largeEyeLevel: %d \n",
                          faceBeauty->fb_option.skinBrightLevel,
                          faceBeauty->fb_option.slimFaceLevel,
                          faceBeauty->fb_option.largeEyeLevel);
           ALOGD("SPRD_FB: skinSmoothLevel: %d, removeBlemishFlag: %d, "
                         "skinColorLevel: %d \n",
                         faceBeauty->fb_option.skinSmoothLevel,
                         faceBeauty->fb_option.removeBlemishFlag,
                         faceBeauty->fb_option.skinColorLevel);
           ALOGD("SPRD_FB: lipColorLevel: %d \n",
                         faceBeauty->fb_option.lipColorLevel);
        }
    }

    if (!strcmp(isDebug, "1")) {
        faceBeauty->fb_option.debugMode = 1;
            /*faceBeauty->fb_option.removeBlemishFlag = 1;
            faceBeauty->fb_option.skinColorType = 1;
            faceBeauty->fb_option.skinColorLevel = 7;
            faceBeauty->fb_option.lipColorType = 2;
            faceBeauty->fb_option.lipColorLevel = 5;
            faceBeauty->fb_option.slimFaceLevel = 7;
            faceBeauty->fb_option.largeEyeLevel = 7;*/
    } else {
        faceBeauty->fb_option.debugMode = 0;
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

void do_face_beauty(struct class_fb *faceBeauty, int faceCount) {

    int retVal = 0;
    struct timespec start_time, end_time;
    char dump_value[PROPERTY_VALUE_MAX];
    char debug_value[PROPERTY_VALUE_MAX];
    char faceInfo[PROPERTY_VALUE_MAX];
    unsigned int duration;
    if (!faceBeauty) {
        ALOGE("do_face_beauty faceBeauty is null");
        return ;
    }
    property_get("persist.vendor.cam.facebeauty.corp", faceBeauty->sprdAlgorithm,
                 "1");
    if (!strcmp(faceBeauty->sprdAlgorithm, "2")) {
        clock_gettime(CLOCK_BOOTTIME, &start_time);

        /*wait first face frame*/
        if (0 == faceBeauty->isFaceGot && faceCount > 0) {
            faceBeauty->isFaceGot = 1;
        }
        if (faceBeauty->isFaceGot == 1) {
            if (faceCount == 0) {
                if (faceBeauty->noFaceFrmCnt < 100)
                    faceBeauty->noFaceFrmCnt++;
            } else
                faceBeauty->noFaceFrmCnt = 0;

            /*from face to no face.remain 10 frames to do face beauty*/
            if (faceBeauty->noFaceFrmCnt < 10)
                faceCount = faceCount > 0 ? faceCount : 1;
        }
        property_get("debug.camera.dump.frame", dump_value, "null");
        if (!strcmp(dump_value, "fb")) {
            save_yuv_data(dumpFrameCount, faceBeauty->fb_image.width,
                          faceBeauty->fb_image.height,
                          faceBeauty->fb_image.yData, "be");
        }
        /*if (lightPortraitType >=5 && faceBeauty->fb_mode == FB_WORKMODE_STILL && faceBeauty->fb_option.slimFaceLevel > 0) {
            retVal =
                FB_FaceBeauty_YUV420SP(faceBeauty->hSprdFB, &(faceBeauty->fb_image),
                                       &(faceBeauty->fb_option),
                                       faceBeauty->fb_face, faceCount, &(fbMask->fb_mask));
        }else {
            ALOGV("do_face_beauty mask null");
            retVal =
                FB_FaceBeauty_YUV420SP(faceBeauty->hSprdFB, &(faceBeauty->fb_image),
                                       &(faceBeauty->fb_option),
                                       faceBeauty->fb_face, faceCount, NULL);
        }*/
        FB_FaceBeauty_YUV420SP(faceBeauty->hSprdFB, &(faceBeauty->fb_image),
                                &(faceBeauty->fb_option),
                                faceBeauty->fb_face, faceCount, NULL);
        if (!strcmp(dump_value, "fb")) {
            save_yuv_data(dumpFrameCount, faceBeauty->fb_image.width,
                          faceBeauty->fb_image.height,
                          faceBeauty->fb_image.yData, "af");
            dumpFrameCount++;
        }

        clock_gettime(CLOCK_BOOTTIME, &end_time);
        duration = (end_time.tv_sec - start_time.tv_sec) * 1000 +
                   (end_time.tv_nsec - start_time.tv_nsec) / 1000000;
        ALOGV("FB_FaceBeauty_YUV420SP duration is %d ms", duration);

        property_get("ro.debuggable", debug_value, "0");
        if (!strcmp(debug_value, "1")) {
            ALOGD("SPRD_FB: FB_FaceBeauty_YUV420SP duration is %d ms", duration);
        }

        property_get("debug.camera.dump.frame.fd", faceInfo, "null");
        if (faceBeauty->fb_mode == 0 || (!strcmp(faceInfo, "fd"))) { //mode 0 is capture,mode 1 is preview.
            int i = 0;
            for (i = 0; i < faceCount; i++) {
                ALOGD("SPRD_FB: fb_face[%d] x:%d, y:%d, w:%d, h:%d , angle:%d, "
                      "pose%d.\n",
                      i, faceBeauty->fb_face[i].x, faceBeauty->fb_face[i].y,
                      faceBeauty->fb_face[i].width,
                      faceBeauty->fb_face[i].height,
                      faceBeauty->fb_face[i].rollAngle,
                      faceBeauty->fb_face[i].yawAngle);
            }
            ALOGD("SPRD_FB: skinSmoothLevel: %d, skinTextureHiFreqLevel: %d, "
                  "skinTextureLoFreqLevel: %d \n",
                  faceBeauty->fb_option.skinSmoothLevel,
                  faceBeauty->fb_option.skinTextureHiFreqLevel,
                  faceBeauty->fb_option.skinTextureLoFreqLevel);
            ALOGD("SPRD_FB: skinBrightLevel: %d, slimFaceLevel: %d, "
                  "largeEyeLevel: %d \n",
                  faceBeauty->fb_option.skinBrightLevel,
                  faceBeauty->fb_option.slimFaceLevel,
                  faceBeauty->fb_option.largeEyeLevel);
        }
    } 
    if (retVal != 0) {
        ALOGE("FACE_BEAUTY ERROR!, ret is %d", retVal);
    }
}

#endif
