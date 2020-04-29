#if defined(CONFIG_FACE_BEAUTY)

#define LOG_TAG "facebeauty_adapter"

#include <cutils/properties.h>
#include <stdlib.h>
#include <utils/Log.h>
#include "sprd_facebeauty_adapter.h"
#include <time.h>
#define NUM_LEVELS 11
#define NUM_TYPES 3
#define CLIP(x, lo, hi) (((x) == (lo)) ? (lo) : ((x) > (hi)) ? (hi) : (x))
int dumpFrameCount = 0;
int faceLevelMap = 0;
struct facebeauty_param_t fbParam;
unsigned char lightPortraitType = 0;
fb_beauty_mask_t *fbMaskT = NULL;

void face_beauty_init(fb_beauty_param_t *faceBeauty, int workMode, int threadNum, fb_chipinfo chipinfo)
{
    if (!faceBeauty) {
        ALOGE("init_fb_handle faceBeauty is null ");
        return;
    }

    if (faceBeauty->runType != SPRD_CAMALG_RUN_TYPE_CPU &&
        faceBeauty->runType != SPRD_CAMALG_RUN_TYPE_VDSP) {
        ALOGE("init_fb_handle input type is ERROR! type:%d", faceBeauty->runType);
        faceBeauty->runType = SPRD_CAMALG_RUN_TYPE_CPU;
    }

    char strRunType[256];
    property_get("ro.boot.lwfq.type", strRunType , "-1");
    if (faceBeauty->runType == SPRD_CAMALG_RUN_TYPE_VDSP && strcmp("0", strRunType))
        faceBeauty->runType = SPRD_CAMALG_RUN_TYPE_CPU;

    property_get("persist.vendor.cam.fb.run_type", strRunType , "");
    if (!(strcmp("cpu", strRunType)))
        faceBeauty->runType = SPRD_CAMALG_RUN_TYPE_CPU;
    else if (!(strcmp("vdsp", strRunType)) && (workMode == 1))//Only movie mode run on VDSP.
        faceBeauty->runType = SPRD_CAMALG_RUN_TYPE_VDSP;
    ALOGD("init_fb_handle to CreateBeautyHandle. runType:%d", faceBeauty->runType);

    property_get("persist.vendor.cam.facebeauty.corp", faceBeauty->sprdAlgorithm, "1");
    if (!strcmp(faceBeauty->sprdAlgorithm, "2")) {
        if (faceBeauty->hSprdFB == 0) {
            faceBeauty->firstFrm = 1;
            if (faceBeauty->runType == SPRD_CAMALG_RUN_TYPE_CPU) {
                ALOGD("FB_CreateBeautyHandle");
                if (FB_OK != FB_CreateBeautyHandle(&(faceBeauty->hSprdFB), workMode,
                                                   threadNum)) {
                    ALOGE("FB_CreateBeautyHandle() Error");
                    return;
                }
            } else if (faceBeauty->runType == SPRD_CAMALG_RUN_TYPE_VDSP) {
                if (FB_OK != FB_CreateBeautyHandle_VDSP(&(faceBeauty->hSprdFB), workMode)) {
                    ALOGE("FB_CreateBeautyHandle()_VDSP Error");
                    return;
                }
            }
#ifdef CONFIG_CAMERA_BEAUTY_FOR_SHARKL5PRO
            faceBeauty->fb_option.fbVersion = 1;
#else
            faceBeauty->fb_option.fbVersion = 0;
#endif
            ALOGD("fb_option.fbVersion = %d",faceBeauty->fb_option.fbVersion);
        }
        if (workMode == 1) {
            faceBeauty->fb_mode = 1;
        } else {
            faceBeauty->fb_mode = 0;
        }
    }
}

void face_beauty_deinit(fb_beauty_param_t *faceBeauty)
{
    ALOGD("face_beauty_deinit");
    if (!faceBeauty) {
        ALOGE("deinit_fb_handle faceBeauty is null");
        return;
    }
    property_get("persist.vendor.cam.facebeauty.corp", faceBeauty->sprdAlgorithm, "1");
    if (!strcmp(faceBeauty->sprdAlgorithm, "2")) {
        if (faceBeauty->hSprdFB != 0) {
            ALOGD("face_beauty_deinit begin!");
            if (faceBeauty->runType == SPRD_CAMALG_RUN_TYPE_CPU) {
                FB_DeleteBeautyHandle(&(faceBeauty->hSprdFB));
                faceBeauty->hSprdFB = NULL;
            } else if (faceBeauty->runType == SPRD_CAMALG_RUN_TYPE_VDSP) {
                FB_DeleteBeautyHandle_VDSP(&(faceBeauty->hSprdFB));
                faceBeauty->hSprdFB = NULL;
            }
        }
        faceBeauty->noFaceFrmCnt = 0;
        ALOGD("face_beauty_deinit end!");
    }
   faceLevelMap = 0;
}

void construct_fb_face(fb_beauty_param_t *faceBeauty, fbBeautyFacetT faceinfo) {
    if (!faceBeauty) {
        ALOGE("construct_fb_face faceBeauty is null");
        return;
    }

    property_get("persist.vendor.cam.facebeauty.corp", faceBeauty->sprdAlgorithm,
                 "1");
    if (!strcmp(faceBeauty->sprdAlgorithm, "2")) {
        faceBeauty->fb_face[faceinfo.idx].x = faceinfo.startX;
        faceBeauty->fb_face[faceinfo.idx].y = faceinfo.startY;
        faceBeauty->fb_face[faceinfo.idx].width = faceinfo.endX- faceinfo.startX;
        faceBeauty->fb_face[faceinfo.idx].height = faceinfo.endY- faceinfo.startY;
        faceBeauty->fb_face[faceinfo.idx].rollAngle = faceinfo.angle;
        faceBeauty->fb_face[faceinfo.idx].yawAngle = faceinfo.pose;
        faceBeauty->fb_face[faceinfo.idx].score = faceinfo.score;
        faceBeauty->fb_face[faceinfo.idx].faceAttriRace = faceinfo.faceAttriRace;
        faceBeauty->fb_face[faceinfo.idx].faceAttriGender= faceinfo.faceAttriGender;
        faceBeauty->fb_face[faceinfo.idx].faceAttriAge= faceinfo.faceAttriAge;
        ALOGD("sprdfb,  fb_face[%d] x:%d, y:%d, w:%d, h:%d , angle:%d, pose%d.",
              faceinfo.idx, faceBeauty->fb_face[faceinfo.idx].x, faceBeauty->fb_face[faceinfo.idx].y,
              faceBeauty->fb_face[faceinfo.idx].width, faceBeauty->fb_face[faceinfo.idx].height,
              faceinfo.angle, faceinfo.pose);
    }
}


void construct_fb_image(fb_beauty_param_t *faceBeauty, int picWidth,
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
                                      ? YUV420_FORMAT_CBCR_VDSP
                                      : YUV420_FORMAT_CRCB_VDSP; // NV12 : NV21
    }
}

void construct_fb_map(facebeauty_param_info_t *facemap){
    ALOGI("construct_fb_map facemap");
    faceLevelMap++;
    fbParam = facemap->cur.fb_param[FB_SKIN_DEFAULT];
}

void construct_fb_mask(fb_beauty_param_t *faceBeauty, fb_beauty_mask_t mFbMaskT) {
    if (!faceBeauty) {
        ALOGE("construct_fb_mask faceBeauty is null");
        return;
    }
    fbMaskT = &mFbMaskT;
    char debug_value[PROPERTY_VALUE_MAX];
    property_get("ro.debuggable", debug_value, "0");
    if (!strcmp(debug_value, "1")) {
        ALOGD("construct_fb_mask: width:%d, height:%d", fbMaskT->fb_mask.width,fbMaskT->fb_mask.height);
    }
}

void construct_fb_level(fb_beauty_param_t *faceBeauty,
                        faceBeautyLevelsT beautyLevels) {
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
    }

    if (!strcmp(isDebug, "1")) {
        faceBeauty->fb_option.debugMode = 1;
    } else {
        faceBeauty->fb_option.debugMode = 0;
    }

}

/*void construct_fb_portraitType(int portraitType){
    lightPortraitType = portraitType;
}*/

void save_yuv_data(int num, int width, int height, unsigned char *addr_y,
                   char flag[10]) {
    char file_name[80];
    char tmp_str[20];
    FILE *fp = NULL;
    memset(file_name, '\0', 80);
    strcpy(file_name, "/data/vendor/cameraserver/");
    sprintf(tmp_str, "%s-%d-%d", flag, num, width);
    strcat(file_name, tmp_str);
    strcat(file_name, "x");
    sprintf(tmp_str, "%d.yuv", height);
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

void do_face_beauty(fb_beauty_param_t *faceBeauty, int faceCount) {

    int retVal = 0;
    struct timespec start_time, end_time;
    char value[PROPERTY_VALUE_MAX];
    unsigned int duration;
    if (!faceBeauty) {
        ALOGE("do_face_beauty faceBeauty is null");
        return;
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
        property_get("debug.camera.dump.frame", value, "null");
        if (!strcmp(value, "fb")) {
            save_yuv_data(dumpFrameCount, faceBeauty->fb_image.width,
                          faceBeauty->fb_image.height,
                          faceBeauty->fb_image.yData, "be");
        }

        if (faceBeauty->runType == SPRD_CAMALG_RUN_TYPE_CPU) {
            FB_IMAGE_YUV420SP inputImage;
            inputImage.width = faceBeauty->fb_image.width;
            inputImage.height = faceBeauty->fb_image.height;
            inputImage.format = (FB_YUV420_FORMAT)(faceBeauty->fb_image.format);
            inputImage.yData = faceBeauty->fb_image.yData;
            inputImage.uvData = faceBeauty->fb_image.uvData;

            FB_BEAUTY_OPTION option;
            memcpy(&option, &(faceBeauty->fb_option), sizeof(FB_BEAUTY_OPTION));
            FB_FACEINFO faceInfo[10];
            memcpy(faceInfo, faceBeauty->fb_face, faceCount*sizeof(FB_FACEINFO));

            if (lightPortraitType >= 5 && faceBeauty->fb_mode == FB_WORKMODE_STILL && faceBeauty->fb_option.slimFaceLevel > 0) {
                retVal =
                    FB_FaceBeauty_YUV420SP(faceBeauty->hSprdFB, &inputImage,
                                          &option, faceInfo, faceCount,&(fbMaskT->fb_mask));

            }else {
                ALOGV("do_face_beauty mask null");
                retVal =
                    FB_FaceBeauty_YUV420SP(faceBeauty->hSprdFB, &inputImage,
                                           &option, faceInfo, faceCount,NULL);
            }
        } else if (faceBeauty->runType == SPRD_CAMALG_RUN_TYPE_VDSP) {
            retVal = FB_FaceBeauty_YUV420SP_VDSP(faceBeauty->hSprdFB, &(faceBeauty->fb_image),
                                            &(faceBeauty->fb_option),faceBeauty->fb_face, faceCount);
        }
        if (!strcmp(value, "fb")) {
            save_yuv_data(dumpFrameCount, faceBeauty->fb_image.width,
                          faceBeauty->fb_image.height,
                          faceBeauty->fb_image.yData, "af");
            dumpFrameCount++;
        }

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
    }
    if (retVal != 0) {
        ALOGE("FACE_BEAUTY ERROR!, ret is %d", retVal);
    }
}

int face_beauty_ctrl(fb_beauty_param_t *faceBeauty, fb_beauty_cmd_t cmd, void *param)
{
    int ret = 0;
    if((NULL == faceBeauty) || ((NULL == param) && (FB_BEAUTY_FAST_STOP_CMD != cmd))) {
        ALOGE("face_beauty_ctrl input ERROR! faceBeauty:%p param:%p", faceBeauty, param);
        return -1;
    }

    switch (cmd) {
    case FB_BEAUTY_GET_VERSION_CMD: {
        fb_beauty_version_t *fbVersion = (fb_beauty_version_t *)param;
        if (faceBeauty->runType == SPRD_CAMALG_RUN_TYPE_CPU) {
            fbVersion->pVersion = FB_GetVersion();
        } else if (faceBeauty->runType == SPRD_CAMALG_RUN_TYPE_VDSP) {
            fbVersion->pVersion = FB_GetVersion_VDSP();
        }
        break;
    }
    case FB_BEAUTY_CONSTRUCT_FACE_CMD: {
        fbBeautyFacetT pFace;
        memset((void *)&pFace, 0, sizeof(fbBeautyFacetT));
        memcpy((void *)&pFace, param, sizeof(fbBeautyFacetT));
        construct_fb_face(faceBeauty, pFace);
        break;
    }
    case FB_BEAUTY_CONSTRUCT_FACEMAP_CMD:{
        facebeauty_param_info_t *pFaceMap = (facebeauty_param_info_t*)param;
        construct_fb_map(pFaceMap);
        break;
    }
    case FB_BEAUTY_CONSTRUCT_MASK_CMD: {
        fb_beauty_mask_t *fbMask = (fb_beauty_mask_t *)param;
        construct_fb_mask(faceBeauty, *fbMask);
        break;
    }
    case FB_BEAUTY_CONSTRUCT_IMAGE_CMD: {
        int picWidth, picHeight, format;
        unsigned char *addrY, *addrU;
        fb_beauty_image_t * pImage = (fb_beauty_image_t*)param;
        picWidth = pImage->inputImage.width;
        picHeight = pImage->inputImage.height;
        addrY = (unsigned char *)pImage->inputImage.addr[0];
        addrU = addrY + (picWidth * picHeight);
        format = (pImage->inputImage.format == SPRD_CAMALG_IMG_NV12)? 1 : 0;
        construct_fb_image(faceBeauty, picWidth, picHeight, addrY, addrU, format);
        faceBeauty->fb_image.imageFd = pImage->inputImage.ion_fd;
        break;
    }
    case FB_BEAUTY_CONSTRUCT_LEVEL_CMD: {
        faceBeautyLevelsT beautyLevels;
        faceBeautyLevelsT *pLevels = (faceBeautyLevelsT *)param;

        beautyLevels.blemishLevel = pLevels->blemishLevel;
        beautyLevels.smoothLevel = pLevels->smoothLevel;
        beautyLevels.skinColor = pLevels->skinColor;
        beautyLevels.skinLevel = pLevels->skinLevel;

        beautyLevels.brightLevel = pLevels->brightLevel;
        beautyLevels.lipColor = pLevels->lipColor;
        beautyLevels.lipLevel = pLevels->lipLevel;
        beautyLevels.slimLevel = pLevels->slimLevel;
        beautyLevels.largeLevel = pLevels->largeLevel;
        beautyLevels.cameraWork = pLevels->cameraWork;
        beautyLevels.cameraBV = pLevels->cameraBV;
        beautyLevels.cameraISO = pLevels->cameraISO;
        beautyLevels.cameraCT = pLevels->cameraCT;
        construct_fb_level(faceBeauty, beautyLevels);
        break;
    }
    case FB_BEAUTY_PROCESS_CMD: {
        //int faceCount = *(int *)param;
        //lightPortraitType = *((char *)param + sizeof(int));
        fb_beauty_lptparam_t *lptParam = (fb_beauty_lptparam_t *)param;
        int faceCount = lptParam->faceCount;
        lightPortraitType = lptParam->lightPortraitType;
        do_face_beauty(faceBeauty, faceCount);
        break;
    }
    case FB_BEAUTY_FAST_STOP_CMD: {
        if (faceBeauty->runType == SPRD_CAMALG_RUN_TYPE_VDSP) {
            FB_FaceBeautyFastStop_VDSP(faceBeauty->hSprdFB);
        }
        break;
    }
    default:
        ret = -1;
        ALOGE("face_beauty_ctrl unknown cmd: %d\n", cmd);
        break;
    }
    return ret;
}

int face_beauty_get_devicetype(fb_beauty_param_t *faceBeauty, enum camalg_run_type *type)
{
    if ((NULL == faceBeauty) || !type) {
        ALOGE("face_beauty_get_devicetype input ERROR! faceBeauty:%p type:%p", faceBeauty, type);
        return -1;
    }
    *type = faceBeauty->runType;
    return 0;
}

int face_beauty_set_devicetype(fb_beauty_param_t *faceBeauty, enum camalg_run_type type)
{
    if (NULL == faceBeauty) {
        ALOGE("face_beauty_set_devicetype input faceBeauty is NULL!");
        return -1;
    }
    if (type != SPRD_CAMALG_RUN_TYPE_CPU && type != SPRD_CAMALG_RUN_TYPE_VDSP) {
        ALOGE("face_beauty_set_devicetype input type is ERROR! type:%d", type);
        faceBeauty->runType = SPRD_CAMALG_RUN_TYPE_CPU;
        return -1;
    }
    faceBeauty->runType = type;
    return 0;
}

#endif
