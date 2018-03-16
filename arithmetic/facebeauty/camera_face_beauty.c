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
#include "arcsoft_beautyshot_wrapper.h"

#define NUM_LEVELS 11
#define NUM_TYPES 3
#define CLIP(x, lo, hi) (((x) < (lo)) ? (lo) : ((x) > (hi)) ? (hi) : (x))
void init_fb_handle(struct class_fb *faceBeauty, int workMode, int threadNum) {
    property_get("persist.sys.cam.facebeauty.corp", faceBeauty->sprdAlgorithm,
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
    } else {
        MRESULT retint;
        if (workMode == 1) {
            faceBeauty->fb_mode = 1;
            if (faceBeauty->hArcSoftFB == 0) {
                ALOGD("init_fb_handle to arcsoft_bsv_create");
                faceBeauty->firstFrm = 1;
                if (arcsoft_bsv_create(&(faceBeauty->hArcSoftFB)) != MOK ||
                    faceBeauty->hArcSoftFB == NULL) {
                    ALOGE("arcsoft_bsv_create  fail");
                    return;
                }
                /* Get arcsoft algorithm version
                const MPBASE_Version* info =
                arcsoft_bsv_get_version(faceBeauty->hArcSoftFB);
                MChar *version = info->Version;
                ALOGD("arcsoft_bsv_get_version : %s",version); */
                retint = arcsoft_bsv_init(faceBeauty->hArcSoftFB);
                if (MOK != retint) {
                    ALOGE("arcsoft_bsv_init() new Error");
                    return;
                }
            }
        } else {
            faceBeauty->fb_mode = 0;
            if (faceBeauty->hArcSoftFB == 0) {
                ALOGD("init_fb_handle to arcsoft_bsi_create");
                if (arcsoft_bsi_create(&(faceBeauty->hArcSoftFB)) != MOK ||
                    faceBeauty->hArcSoftFB == NULL) {
                    ALOGE("arcsoft_bsi_create  fail");
                    return;
                }
                if (MOK != arcsoft_bsi_init(faceBeauty->hArcSoftFB)) {
                    ALOGE("arcsoft_bsi_init() Error");
                    return;
                }
            }
        }
    }
}

void deinit_fb_handle(struct class_fb *faceBeauty) {
    property_get("persist.sys.cam.facebeauty.corp", faceBeauty->sprdAlgorithm,
                 "1");
    if (!strcmp(faceBeauty->sprdAlgorithm, "2")) {
        if (faceBeauty->hSprdFB != 0) {
            FB_DeleteBeautyHandle(&(faceBeauty->hSprdFB));
            faceBeauty->hSprdFB = NULL;
        }
    } else {
        if (faceBeauty->fb_mode == 1) {
            if (faceBeauty->hArcSoftFB != 0) {
                ALOGD("deinit_fb_handle arcsoft_bsv_fini");
                if (MOK != arcsoft_bsv_fini(faceBeauty->hArcSoftFB)) {
                    ALOGE("arcsoft_bsv_fini() Error");
                    return;
                }
                if (MOK != arcsoft_bsv_destroy(faceBeauty->hArcSoftFB)) {
                    ALOGE("arcsoft_bsv_destroy() Error");
                    return;
                }
                faceBeauty->hArcSoftFB = NULL;
            }
        } else {
            if (faceBeauty->hArcSoftFB != 0) {
                ALOGD("deinit_fb_handle arcsoft_bsi_fini");
                if (MOK != arcsoft_bsi_fini(faceBeauty->hArcSoftFB)) {
                    ALOGE("arcsoft_bsi_fini() Error");
                    return;
                }
                if (MOK != arcsoft_bsi_destroy(faceBeauty->hArcSoftFB)) {
                    ALOGE("arcsoft_bsi_destroy() Error");
                    return;
                }
                faceBeauty->hArcSoftFB = NULL;
            }
        }
    }
}

void construct_fb_face(struct class_fb *faceBeauty, int j, int sx, int sy,
                       int ex, int ey, int angle, int pose) {
    if (!faceBeauty) {
        ALOGE("construct_fb_face faceBeauty is null");
        return;
    }

    property_get("persist.sys.cam.facebeauty.corp", faceBeauty->sprdAlgorithm,
                 "1");
    if (!strcmp(faceBeauty->sprdAlgorithm, "2")) {
        faceBeauty->fb_face[j].x = sx;
        faceBeauty->fb_face[j].y = sy;
        faceBeauty->fb_face[j].width = ex - sx;
        faceBeauty->fb_face[j].height = ey - sy;
        faceBeauty->fb_face[j].rollAngle = angle;
        faceBeauty->fb_face[j].yawAngle = pose;
        ALOGD("sprdfb,  fb_face[%d] x:%d, y:%d, w:%d, h:%d , angle:%d, pose%d.",
              j, faceBeauty->fb_face[j].x, faceBeauty->fb_face[j].y,
              faceBeauty->fb_face[j].width, faceBeauty->fb_face[j].height,
              angle, pose);
    } else {
        faceBeauty->arc_fb_face.prtFaces[j].left = sx;
        faceBeauty->arc_fb_face.prtFaces[j].top = sy;
        faceBeauty->arc_fb_face.prtFaces[j].right = ex;
        faceBeauty->arc_fb_face.prtFaces[j].bottom = ey;
        if (angle > 0) {
            angle = -angle;
            angle += 360;
        } else {
            angle = -angle;
        }
        faceBeauty->arc_fb_face.plFaceRolls[j] = angle;
        ALOGD("acr_fb_face[%d]  angle: %d. rect: %d %d   %d %d.", j,
              (int)faceBeauty->arc_fb_face.plFaceRolls[j], sx, sy, ex, ey);
    }
}

void construct_fb_image(struct class_fb *faceBeauty, int picWidth,
                        int picHeight, unsigned char *addrY,
                        unsigned char *addrU, int format) {
    if (!faceBeauty) {
        ALOGE("construct_fb_image faceBeauty is null");
        return;
    }
    property_get("persist.sys.cam.facebeauty.corp", faceBeauty->sprdAlgorithm,
                 "1");
    if (!strcmp(faceBeauty->sprdAlgorithm, "2")) {
        faceBeauty->fb_image.width = picWidth;
        faceBeauty->fb_image.height = picHeight;
        faceBeauty->fb_image.yData = addrY;
        faceBeauty->fb_image.uvData = addrU;
        faceBeauty->fb_image.format = (format == 1)
                                          ? YUV420_FORMAT_CBCR
                                          : YUV420_FORMAT_CRCB; // NV12 : NV21
    } else {
        faceBeauty->arc_fb_image.u32PixelArrayFormat = ASVL_PAF_NV21;
        if (format == 1) {
            faceBeauty->arc_fb_image.u32PixelArrayFormat = ASVL_PAF_NV12;
        }
        faceBeauty->arc_fb_image.i32Width = picWidth;
        faceBeauty->arc_fb_image.i32Height = picHeight;
        faceBeauty->arc_fb_image.pi32Pitch[0] = picWidth;
        faceBeauty->arc_fb_image.pi32Pitch[1] = picWidth;
        faceBeauty->arc_fb_image.ppu8Plane[0] = addrY;
        faceBeauty->arc_fb_image.ppu8Plane[1] = addrU;
    }
}

void construct_fb_level(struct class_fb *faceBeauty,
                        struct face_beauty_levels beautyLevels) {
    if (!faceBeauty) {
        ALOGE("construct_fb_level faceBeauty is null");
        return;
    }
    // init the static parameters table. save the value until the process is
    // restart or the device is restart.
    unsigned char preview_whitenLevel[NUM_LEVELS] = {0,  15, 20, 30, 40, 45,
                                                     50, 55, 60, 65, 70};
    unsigned char preview_cleanLevel[NUM_LEVELS] = {0,  15, 20, 30, 40, 45,
                                                    50, 55, 60, 65, 70};
    unsigned char picture_whitenLevel[NUM_LEVELS] = {0,  10, 15, 20, 25, 30,
                                                     35, 40, 45, 50, 55};
    unsigned char picture_cleanLevel[NUM_LEVELS] = {0,  10, 15, 20, 25, 30,
                                                    35, 40, 45, 50, 55};
    unsigned char face_slimLevel[NUM_LEVELS] = {0,  6,  8,  10, 15, 18,
                                                25, 30, 35, 40, 45};
    unsigned char eye_largeLevel[NUM_LEVELS] = {0,  8,  10, 15, 20, 25,
                                                30, 40, 45, 50, 55};
    unsigned char tab_skinWhitenLevel[NUM_LEVELS];
    unsigned char tab_skinCleanLevel[NUM_LEVELS];
    unsigned char tab_faceSlimLevel[NUM_LEVELS] = {0};
    unsigned char tab_eyeLargeLevel[NUM_LEVELS] = {0};

    if (!strcmp(faceBeauty->sprdAlgorithm, "2")) {
        memcpy(tab_skinWhitenLevel, preview_whitenLevel,
               sizeof(unsigned char) * NUM_LEVELS);
        memcpy(tab_skinCleanLevel, preview_cleanLevel,
               sizeof(unsigned char) * NUM_LEVELS);
    } else {
        if (faceBeauty->fb_mode == 1) {
            memcpy(tab_skinWhitenLevel, preview_whitenLevel,
                   sizeof(unsigned char) * NUM_LEVELS);
            memcpy(tab_skinCleanLevel, preview_cleanLevel,
                   sizeof(unsigned char) * NUM_LEVELS);
        } else {
            memcpy(tab_skinWhitenLevel, picture_whitenLevel,
                   sizeof(unsigned char) * NUM_LEVELS);
            memcpy(tab_skinCleanLevel, picture_cleanLevel,
                   sizeof(unsigned char) * NUM_LEVELS);
        }
        memcpy(tab_faceSlimLevel, face_slimLevel,
               sizeof(unsigned char) * NUM_LEVELS);
        memcpy(tab_eyeLargeLevel, eye_largeLevel,
               sizeof(unsigned char) * NUM_LEVELS);
    }
    // get the property level and parameters value and save in the parameters
    // table.
    // one time only adjust one level's parameters. In order to adjust all the
    // values, six time should be done.
    {
        char str_adb_level[PROPERTY_VALUE_MAX];
        char str_adb_white[PROPERTY_VALUE_MAX];
        char str_adb_clean[PROPERTY_VALUE_MAX];
        char str_adb_faceSlim[PROPERTY_VALUE_MAX];
        char str_adb_eyeLarge[PROPERTY_VALUE_MAX];
        int adb_level_val = 0;
        int adb_white_val = 0;
        int adb_clean_val = 0;
        int adb_faceSlim_val = 0;
        int adb_eyeLarge_val = 0;
        if ((property_get("persist.sys.camera.beauty.level", str_adb_level,
                          "0")) &&
            (property_get("persist.sys.camera.beauty.white", str_adb_white,
                          "0")) &&
            (property_get("persist.sys.camera.beauty.clean", str_adb_clean,
                          "0")) &&
            (property_get("persist.sys.camera.beauty.slim", str_adb_faceSlim,
                          "0")) &&
            (property_get("persist.sys.camera.beauty.large", str_adb_eyeLarge,
                          "0"))) {
            adb_level_val = atoi(str_adb_level);
            adb_level_val = CLIP(adb_level_val, 0, 10);
            adb_white_val = atoi(str_adb_white);
            adb_white_val = CLIP(adb_white_val, 0, 100);
            adb_clean_val = atoi(str_adb_clean);
            adb_clean_val = CLIP(adb_clean_val, 0, 100);
            adb_faceSlim_val = atoi(str_adb_faceSlim);
            adb_faceSlim_val = CLIP(adb_faceSlim_val, 0, 100);
            adb_eyeLarge_val = atoi(str_adb_eyeLarge);
            adb_eyeLarge_val = CLIP(adb_eyeLarge_val, 0, 100);

            // replace the static value.
            tab_skinWhitenLevel[adb_level_val] = adb_white_val;
            tab_skinCleanLevel[adb_level_val] = adb_clean_val;
            tab_faceSlimLevel[adb_level_val] = adb_faceSlim_val;
            tab_eyeLargeLevel[adb_level_val] = adb_eyeLarge_val;
        }
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
    property_get("persist.sys.camera.beauty.debug", isDebug, "0");
    property_get("persist.sys.cam.facebeauty.corp", faceBeauty->sprdAlgorithm,
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
    } else {
        // convert the skin_level set by APP to skinLevel & smoothLevel
        // according to
        // the table saved.
        beautyLevels.smoothLevel = tab_skinCleanLevel[beautyLevels.smoothLevel];
        beautyLevels.brightLevel =
            tab_skinWhitenLevel[beautyLevels.brightLevel];
        beautyLevels.slimLevel = tab_faceSlimLevel[beautyLevels.slimLevel];
        beautyLevels.largeLevel = tab_eyeLargeLevel[beautyLevels.largeLevel];

        faceBeauty->faceSoften = (MInt32)beautyLevels.smoothLevel;
        faceBeauty->faceWhiten = (MInt32)beautyLevels.brightLevel;
        faceBeauty->eyeEnlargement = (MInt32)beautyLevels.largeLevel;
        faceBeauty->faceSlender = (MInt32)beautyLevels.slimLevel;
        if (!strcmp(isDebug, "1")) {
            faceBeauty->eyeEnlargement = 50;
            faceBeauty->faceSlender = 50;
        }
        ALOGD("arc_levels: %d %d %d %d.", faceBeauty->faceSoften,
              faceBeauty->faceWhiten, faceBeauty->eyeEnlargement,
              faceBeauty->faceSlender);
    }
}

void save_yuv_data(int num, int width, int height, unsigned char *addr_y) {
    char file_name[80];
    char tmp_str[20];
    FILE *fp = NULL;
    memset(file_name, '\0', 80);
    strcpy(file_name, "/data/misc/cameraserver/");
    sprintf(tmp_str, "%d-%d", num, width);
    strcat(file_name, tmp_str);
    strcat(file_name, "x");
    sprintf(tmp_str, "%d,nv21", height);
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
    char value[PROPERTY_VALUE_MAX];
    unsigned int duration;
    if (!faceBeauty) {
        ALOGE("do_face_beauty faceBeauty is null");
        return;
    }
    property_get("persist.sys.cam.facebeauty.corp", faceBeauty->sprdAlgorithm,
                 "1");
    if (!strcmp(faceBeauty->sprdAlgorithm, "2")) {
        clock_gettime(CLOCK_BOOTTIME, &start_time);
        BOKEH_DATA *bokehData = 0;

        /*check first face frame*/
        if (faceBeauty->firstFrm && faceCount > 0) {
            faceBeauty->firstFrm = 0;
        }
        if (faceBeauty->firstFrm == 0) {
            if (faceCount == 0) {
                if (faceBeauty->noFaceFrmCnt < 100)
                    faceBeauty->noFaceFrmCnt++;
            } else
                faceBeauty->noFaceFrmCnt = 0;

            /*from face to no face.remain 10 frames to do face beauty*/
            if (faceBeauty->noFaceFrmCnt < 10)
                faceCount = faceCount > 0 ? faceCount : 1;
        }

        retVal =
            FB_FaceBeauty_YUV420SP(faceBeauty->hSprdFB, &(faceBeauty->fb_image),
                                   &(faceBeauty->fb_option),
                                   faceBeauty->fb_face, faceCount, bokehData);
        clock_gettime(CLOCK_BOOTTIME, &end_time);
        duration = (end_time.tv_sec - start_time.tv_sec) * 1000 +
                   (end_time.tv_nsec - start_time.tv_nsec) / 1000000;
        ALOGV("FB_FaceBeauty_YUV420SP duration is %d ms", duration);
        ALOGD("SPRD_FB: FB_FaceBeauty_YUV420SP duration is %d ms", duration);
        if (faceBeauty->fb_mode == 0) { // this work mode is useless.
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
    } else {
        faceBeauty->arc_fb_face.lFaceNum = faceCount;
        if (faceBeauty->fb_mode == 1) {
            arcsoft_bsv_set_feature_level(faceBeauty->hArcSoftFB,
                                          FEATURE_FACE_SOFTEN_KEY,
                                          faceBeauty->faceSoften);
            arcsoft_bsv_set_feature_level(faceBeauty->hArcSoftFB,
                                          FEATURE_FACE_WHITEN_KEY,
                                          faceBeauty->faceWhiten);
            arcsoft_bsv_set_feature_level(faceBeauty->hArcSoftFB,
                                          FEATURE_EYE_ENLARGEMENT_KEY,
                                          faceBeauty->eyeEnlargement);
            arcsoft_bsv_set_feature_level(faceBeauty->hArcSoftFB,
                                          FEATURE_FACE_SLENDER_KEY,
                                          faceBeauty->faceSlender);
            clock_gettime(CLOCK_BOOTTIME, &start_time);
            ALOGD("format : %d,w %d,h %d , white %d ,soft %d",
                  faceBeauty->arc_fb_image.u32PixelArrayFormat,
                  faceBeauty->arc_fb_image.i32Width,
                  faceBeauty->arc_fb_image.i32Height, faceBeauty->faceSoften,
                  faceBeauty->faceWhiten);
            property_get("persist.sys.cam.beauty.dump", value, "off");
            if (!strcmp(value, "on")) {
                save_yuv_data(11, faceBeauty->arc_fb_image.i32Width,
                              faceBeauty->arc_fb_image.i32Height,
                              faceBeauty->arc_fb_image.ppu8Plane[0]);
            }

            retVal = arcsoft_bsv_process(
                faceBeauty->hArcSoftFB, &(faceBeauty->arc_fb_image),
                &(faceBeauty->arc_fb_image), &(faceBeauty->arc_fb_face), MNull);

            property_get("persist.sys.cam.beauty.dump", value, "off");
            if (!strcmp(value, "on")) {
                save_yuv_data(22, faceBeauty->arc_fb_image.i32Width,
                              faceBeauty->arc_fb_image.i32Height,
                              faceBeauty->arc_fb_image.ppu8Plane[0]);
            }
            /* Arcsoft issue, first frame is no effective, so process two
             * times*/
            if (1 == faceBeauty->firstFrm) {
                ALOGD("do_facebeauty in the oem first frame!");
                retVal = arcsoft_bsv_process(faceBeauty->hArcSoftFB,
                                             &(faceBeauty->arc_fb_image),
                                             &(faceBeauty->arc_fb_image),
                                             &(faceBeauty->arc_fb_face), MNull);
                faceBeauty->firstFrm = 0;
            }
            clock_gettime(CLOCK_BOOTTIME, &end_time);
            duration = (end_time.tv_sec - start_time.tv_sec) * 1000 +
                       (end_time.tv_nsec - start_time.tv_nsec) / 1000000;
            // for test
            ALOGD("arcsoft_bsv_process duration is %d ms", duration);
            ALOGV("arcsoft_bsv_process duration is %d ms", duration);
        } else {
            arcsoft_bsi_set_feature_level(faceBeauty->hArcSoftFB,
                                          FEATURE_FACE_SOFTEN_KEY,
                                          faceBeauty->faceSoften);
            arcsoft_bsi_set_feature_level(faceBeauty->hArcSoftFB,
                                          FEATURE_FACE_WHITEN_KEY,
                                          faceBeauty->faceWhiten);
            arcsoft_bsi_set_feature_level(faceBeauty->hArcSoftFB,
                                          FEATURE_EYE_ENLARGEMENT_KEY,
                                          faceBeauty->eyeEnlargement);
            arcsoft_bsi_set_feature_level(faceBeauty->hArcSoftFB,
                                          FEATURE_FACE_SLENDER_KEY,
                                          faceBeauty->faceSlender);
            clock_gettime(CLOCK_BOOTTIME, &start_time);
            ALOGD("format : %d,w %d,h %d , white %d ,soft %d",
                  faceBeauty->arc_fb_image.u32PixelArrayFormat,
                  faceBeauty->arc_fb_image.i32Width,
                  faceBeauty->arc_fb_image.i32Height, faceBeauty->faceSoften,
                  faceBeauty->faceWhiten);
            property_get("persist.sys.cam.beauty.dump", value, "off");
            if (!strcmp(value, "on")) {
                save_yuv_data(11, faceBeauty->arc_fb_image.i32Width,
                              faceBeauty->arc_fb_image.i32Height,
                              faceBeauty->arc_fb_image.ppu8Plane[0]);
            }
            retVal = arcsoft_bsi_process(
                faceBeauty->hArcSoftFB, &(faceBeauty->arc_fb_image),
                &(faceBeauty->arc_fb_image), &(faceBeauty->arc_fb_face), MNull);
            property_get("persist.sys.cam.beauty.dump", value, "off");
            if (!strcmp(value, "on")) {
                save_yuv_data(22, faceBeauty->arc_fb_image.i32Width,
                              faceBeauty->arc_fb_image.i32Height,
                              faceBeauty->arc_fb_image.ppu8Plane[0]);
            }
            clock_gettime(CLOCK_BOOTTIME, &end_time);
            duration = (end_time.tv_sec - start_time.tv_sec) * 1000 +
                       (end_time.tv_nsec - start_time.tv_nsec) / 1000000;
            ALOGD("arcsoft_bsi_process duration is %d ms", duration);
        }
    }
    if (retVal != 0) {
        ALOGE("FACE_BEAUTY ERROR!, ret is %d", retVal);
    }
}

#endif
