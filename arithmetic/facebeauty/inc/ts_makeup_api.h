/**
 * Copyright (C) 2012 Thundersoft Corporation
 * All rights Reserved
 */
#ifndef __TS_MAKEUP_API_H__
#define __TS_MAKEUP_API_H__
#include "ts_makeup_data.h"
#include "ts_makeup_image.h"

#ifdef __cplusplus
extern "C" {
#endif

    void ts_printVersionInfo();
    void ts_SetImageOritation(int angle);//for algorithm face detection
    /*
     * PARAMETERS :
     *   @param[in] pInData : The TSMakeupData pointer.MUST not NULL.
     *   @param[out] pOutData : The TSMakeupData pointer.MUST not NULL.
     *   @param[in] skinCleanLevel : Skin clean level, value range [0,100].
     *   @param[in] skinWhitenLevel : Skin whiten level, value range [0,100].
     *   @param[in] pfaceRect: pointer to facerect, if NULL,use ts facedetect, else use system facedata.
     *   @param[in] IsneedFaceDetection:Whether it needs face beauty lib FD,1 for yes ,0 for no.
     *   @param[in] nImageFormat: Image format,default value 0 is for NV21,TSFB_FMT_NV12 for NV12.
     * RETURN    : TS_OK if success, otherwise failed.
     *
     */
    int ts_face_beautify(TSMakeupData *pInData, TSMakeupData *pOutData, int skinCleanLevel, int skinWhitenLevel,TSRect* pfaceRect, bool IsneedFaceDetection , int nImageFormat);

#ifdef __cplusplus
} /* extern "C" { */
#endif

#endif // __TS_MAKEUP_API_H__
