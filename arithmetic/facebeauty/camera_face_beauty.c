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
#define CLIP(x, lo, hi) (((x) < (lo)) ? (lo) : ((x) > (hi)) ? (hi) : (x))
int dumpFrameCount = 0;
char value[PROPERTY_VALUE_MAX];

void init_fb_handle(struct class_fb *faceBeauty, int workMode, int threadNum) {
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
    }
}

void deinit_fb_handle(struct class_fb *faceBeauty) {
    dumpFrameCount = 0;
    property_get("persist.vendor.cam.facebeauty.corp", faceBeauty->sprdAlgorithm,
                 "1");
    if (!strcmp(faceBeauty->sprdAlgorithm, "2")) {
        if (faceBeauty->hSprdFB != 0) {
            FB_DeleteBeautyHandle(&(faceBeauty->hSprdFB));
            faceBeauty->hSprdFB = NULL;
        }
        faceBeauty->noFaceFrmCnt = 0;
    }
}

void construct_fb_face(struct class_fb *faceBeauty, int j, int sx, int sy,
                       int ex, int ey, int angle, int pose) {
    char debug_value[PROPERTY_VALUE_MAX];
    if (!faceBeauty) {
        ALOGE("construct_fb_face faceBeauty is null");
        return;
    }

    property_get("persist.vendor.cam.facebeauty.corp", faceBeauty->sprdAlgorithm,
                 "1");
    if (!strcmp(faceBeauty->sprdAlgorithm, "2")) {
        faceBeauty->fb_face[j].x = sx;
        faceBeauty->fb_face[j].y = sy;
        faceBeauty->fb_face[j].width = ex - sx;
        faceBeauty->fb_face[j].height = ey - sy;
        faceBeauty->fb_face[j].rollAngle = angle;
        faceBeauty->fb_face[j].yawAngle = pose;
        property_get("ro.debuggable", debug_value, "0");
        if (!strcmp(debug_value, "1")) {
            ALOGD("sprdfb,  fb_face[%d] x:%d, y:%d, w:%d, h:%d , angle:%d, pose%d.",
                    j, faceBeauty->fb_face[j].x, faceBeauty->fb_face[j].y,
                    faceBeauty->fb_face[j].width, faceBeauty->fb_face[j].height,
                    angle, pose);
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

void construct_fb_level(struct class_fb *faceBeauty,
                        struct face_beauty_levels beautyLevels) {
    if (!faceBeauty) {
        ALOGE("construct_fb_level faceBeauty is null");
        return;
    }

    // convert the skin_level set by APP to skinLevel & smoothLevel according to
    // the table saved.
    {
        beautyLevels.smoothLevel = CLIP(beautyLevels.smoothLevel, 0, 10);
        beautyLevels.brightLevel = CLIP(beautyLevels.brightLevel, 0, 10);
        beautyLevels.slimLevel = CLIP(beautyLevels.slimLevel, 0, 10);
        beautyLevels.largeLevel = CLIP(beautyLevels.largeLevel, 0, 10);
        beautyLevels.lipLevel = CLIP(beautyLevels.lipLevel, 0, 10);
        beautyLevels.skinLevel = CLIP(beautyLevels.skinLevel, 0, 10);
    }

    char isDebug[PROPERTY_VALUE_MAX];
    property_get("persist.vendor.cam.beauty.debug", isDebug, "0");
    property_get("persist.vendor.cam.facebeauty.corp", faceBeauty->sprdAlgorithm,
                 "1");
    if (!strcmp(faceBeauty->sprdAlgorithm, "2")) {
        unsigned char map_pictureSkinSmoothLevel[NUM_LEVELS] = {
            0, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7};
        unsigned char map_previewSkinSmoothLevel[NUM_LEVELS] = {
            0, 1, 1, 2, 2, 2, 2, 3, 3, 4, 4};
        unsigned char map_skinBrightLevel[NUM_LEVELS] = {0, 1, 2, 3, 3, 4,
                                                         4, 5, 5, 6, 6};
        unsigned char map_slimFaceLevel[NUM_LEVELS] = {0, 1, 1, 2, 2, 3,
                                                       3, 4, 4, 5, 5};
        unsigned char map_largeEyeLevel[NUM_LEVELS] = {0, 1, 1, 2, 2, 3,
                                                       4, 5, 5, 6, 7};
        unsigned char map_skinSmoothRadiusCoeff[NUM_LEVELS] = {
            55, 55, 55, 50, 50, 40, 40, 30, 30, 20, 20};
        unsigned char map_pictureSkinTextureHiFreqLevel[NUM_LEVELS] = {
            0, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10};
        unsigned char map_pictureSkinTextureLoFreqLevel[NUM_LEVELS] = {
            4, 4, 4, 3, 3, 2, 2, 1, 1, 0, 0};
        unsigned char map_lipColorLevel[NUM_LEVELS] = {0, 1, 1, 2, 2, 3,
                                                       3, 4, 4, 5, 5};
        unsigned char map_skinColorLevel[NUM_LEVELS] = {0, 1, 1, 2, 2, 3,
                                                        3, 4, 4, 5, 5};

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

        /* We don't use the following options */
        faceBeauty->fb_option.removeBlemishFlag = 0;
        faceBeauty->fb_option.removeBlemishFlag = beautyLevels.blemishLevel;
        faceBeauty->fb_option.lipColorType = beautyLevels.lipColor;
        faceBeauty->fb_option.lipColorLevel = beautyLevels.lipLevel;
        faceBeauty->fb_option.skinColorType = beautyLevels.skinColor;
        faceBeauty->fb_option.skinColorLevel = beautyLevels.skinLevel;
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
        faceBeauty->fb_option.bokehON = 0;

        if (!strcmp(isDebug, "1")) {
            faceBeauty->fb_option.debugMode = 1;
            faceBeauty->fb_option.removeBlemishFlag = 1;
            faceBeauty->fb_option.skinColorType = 1;
            faceBeauty->fb_option.skinColorLevel = 7;
            faceBeauty->fb_option.lipColorType = 2;
            faceBeauty->fb_option.lipColorLevel = 5;
            faceBeauty->fb_option.slimFaceLevel = 7;
            faceBeauty->fb_option.largeEyeLevel = 7;
        } else {
            faceBeauty->fb_option.debugMode = 0;
        }
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
        return;
    }
    property_get("persist.vendor.cam.facebeauty.corp", faceBeauty->sprdAlgorithm,
                 "1");
    if (!strcmp(faceBeauty->sprdAlgorithm, "2")) {
        clock_gettime(CLOCK_BOOTTIME, &start_time);
        BOKEH_DATA *bokehData = 0;

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
        retVal =
            FB_FaceBeauty_YUV420SP(faceBeauty->hSprdFB, &(faceBeauty->fb_image),
                                   &(faceBeauty->fb_option),
                                   faceBeauty->fb_face, faceCount, bokehData);
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
