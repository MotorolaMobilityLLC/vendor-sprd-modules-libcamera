/*
 * Copyright (c) 2016, The Linux Foundataion. All rights reserved.
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

#ifndef SPRDCAMERA3MULTICAMERA_H_HEADER
#define SPRDCAMERA3MULTICAMERA_H_HEADER

#include <hardware/camera3.h>
#include <utils/Singleton.h>

namespace sprdcamera {

/*
 * A wrapper to multi-camera class.
 */
class SprdCamera3MultiCamera
    : public android::Singleton<SprdCamera3MultiCamera> {
  public:
    static int get_camera_info(int camera_id, struct camera_info *info);
    static int camera_device_open(const struct hw_module_t *module,
                                  const char *id,
                                  struct hw_device_t **hw_device);

  private:
    SprdCamera3MultiCamera();
    ~SprdCamera3MultiCamera();

    typedef void *HandleType;
    typedef int (*FuncType_GetCameraInfo)(int, struct camera_info *);
    typedef int (*FuncType_Open)(const struct hw_module_t *, const char *,
                                 struct hw_device_t **);

    bool mAuthorized;
    HandleType mHandle;
    FuncType_GetCameraInfo mFuncGetCameraInfo;
    FuncType_Open mFuncOpen;

    friend class android::Singleton<SprdCamera3MultiCamera>;
};
};

#endif
