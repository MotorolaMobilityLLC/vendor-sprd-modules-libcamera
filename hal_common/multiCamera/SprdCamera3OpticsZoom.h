/* Copyright (c) 2016, The Linux Foundataion. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#ifndef SPRDCAMERA3ZOOM_H_HEADER
#define SPRDCAMERA3ZOOM_H_HEADER

#include "SprdCamera3Multi.h"

namespace sprdcamera {

class SprdCamera3OpticsZoom : public SprdCamera3Multi {
  public:
    SprdCamera3OpticsZoom();
    virtual ~SprdCamera3OpticsZoom();
    static void getCamera3dZoom(SprdCamera3Multi **pMuxer);

    void reReqConfig(camera3_capture_request_t *request, CameraMetadata *meta);

    config_multi_camera *load_config_file(void);

    float setZoomInfo(CameraMetadata *WideSettings,
                      CameraMetadata *TeleSettings);
    void reConfigGetCameraInfo(CameraMetadata &metadata);

    camera_metadata_t *reConfigResultMeta(camera_metadata_t *meta);
    void coordinateTra(int inputWidth, int inputHeight, int outputWidth,
                       int outputHeight, float inputRatio, float outputRatio,
                       int *area);
    float mZoomValue;
    float mZoomValueTh;
    cmr_u16 mTeleMaxWidth;
    cmr_u16 mTeleMaxHeight;
    cmr_u16 mWideMaxWidth;
    cmr_u16 mWideMaxHeight;
};
};

#endif /* SPRDCAMERA3ZOOM_H_HEADER*/
