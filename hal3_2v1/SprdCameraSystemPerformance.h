/* Copyright (c) 2017, The Linux Foundataion. All rights reserved.
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
#ifndef SPRDCAMERA3SYSTEMPERFORMACEH_HEADER
#define SPRDCAMERA3SYSTEMPERFORMACEH_HEADER

#include <stdlib.h>
#include <dlfcn.h>
#include <utils/Log.h>
#include <utils/Errors.h>
#include <utils/List.h>
#include <utils/Mutex.h>
#include <cutils/properties.h>
#include <sys/mman.h>
#include <cutils/sockets.h>
#include <sys/socket.h>
#include "SprdCamera3HALHeader.h"

#define ANDROID_VERSION_P (901)

#if (CONFIG_HAS_CAMERA_HINTS_VERSION == ANDROID_VERSION_P)
#include <power_hal_cli.h>
#endif

namespace sprdcamera {

typedef enum CURRENT_POWER_HINT {
    CAM_POWER_NORMAL,
    CAM_POWER_PERFORMACE_ON,
    CAM_POWER_LOWPOWER_ON
} power_hint_state_type_t;

typedef enum DFS_POLICY {
    CAM_EXIT,
    CAM_LOW,
    CAM_NORMAL,
    CAM_VERYHIGH,
} dfs_policy_t;

typedef enum CAMERA_PERFORMACE_SCENE {
    CAM_PERFORMANCE_LEVEL_1 = 1,
    CAM_PERFORMANCE_LEVEL_2,
    CAM_PERFORMANCE_LEVEL_3,
    CAM_PERFORMANCE_LEVEL_4,
    CAM_PERFORMANCE_LEVEL_5,
    CAM_PERFORMANCE_LEVEL_6,
} sys_performance_camera_scene;

class SprdCameraSystemPerformance {
  public:
    static void getSysPerformance(SprdCameraSystemPerformance **pmCamSysPer);
    static void freeSysPerformance(SprdCameraSystemPerformance **pgCamSysPer);
    void setCamPreformaceScene(sys_performance_camera_scene camera_scene);

  private:
    SprdCameraSystemPerformance();
    ~SprdCameraSystemPerformance();
    void setPowerHint(power_hint_state_type_t powerhint_id);
    int changeDfsPolicy(dfs_policy_t dfs_policy);
    void initPowerHint();
    void deinitPowerHint();

    int setDfsPolicy(int dfs_policy);
    int releaseDfsPolicy(int dfs_policy);
    static int mCameraSessionActive;
    int mCameraDfsPolicyCur;
    int mCurrentPowerHint;
    bool mPowermanageInited;

#if (CONFIG_HAS_CAMERA_HINTS_VERSION == ANDROID_VERSION_P)
    ::android::sp<::android::PowerHALManager> mPowerManager;
    ::android::sp<::android::PowerHintScene> mSceneLowPower;
    ::android::sp<::android::PowerHintScene> mScenePerformance;

    void acquirePowerHint(::android::sp<::android::PowerHintScene> mScene);
    void releasePowerHint(::android::sp<::android::PowerHintScene> mScene);
#endif

    Mutex mLock;
    static Mutex sLock;
};
}
#endif /* SPRDCAMERAMU*/
