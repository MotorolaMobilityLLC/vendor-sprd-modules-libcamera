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
#define LOG_TAG "Cam3SysPer"
#include "SprdCameraSystemPerformance.h"

using namespace android;

namespace sprdcamera {

#define CAM_LOW_STR "camlow"
#define CAM_NORMAL_STR "camhigh"
#define CAM_VERYHIGH_STR "camveryhigh"

SprdCameraSystemPerformance *gCamSysPer = NULL;
int SprdCameraSystemPerformance::mCameraSessionActive = 0;
Mutex SprdCameraSystemPerformance::sLock;
// Error Check Macros
#define CHECK_SYSTEMPERFORMACE()                                               \
    if (!gCamSysPer) {                                                         \
        HAL_LOGE("Error getting CamSysPer ");                                  \
        return;                                                                \
    }

SprdCameraSystemPerformance::SprdCameraSystemPerformance() {

    HAL_LOGI("E");
    mCurrentPowerHint = CAM_POWER_NORMAL;
    mCameraDfsPolicyCur = CAM_EXIT;
    mPowermanageInited = false;

#if (CONFIG_HAS_CAMERA_HINTS_VERSION == ANDROID_VERSION_P)
    mPowerManager = NULL;
    mSceneLowPower = NULL;
    mScenePerformance = NULL;
#endif

    initPowerHint();
    HAL_LOGI("X");
}

SprdCameraSystemPerformance::~SprdCameraSystemPerformance() {
    HAL_LOGI("E");
    changeDfsPolicy(CAM_EXIT);
    setPowerHint(CAM_POWER_NORMAL);

    deinitPowerHint();
    HAL_LOGI("X");
}

void SprdCameraSystemPerformance::getSysPerformance(
    SprdCameraSystemPerformance **pgCamSysPer) {

    Mutex::Autolock l(&sLock);

    *pgCamSysPer = NULL;

    if (!gCamSysPer) {
        gCamSysPer = new SprdCameraSystemPerformance();
    }
    CHECK_SYSTEMPERFORMACE();
    *pgCamSysPer = gCamSysPer;
    mCameraSessionActive++;
    HAL_LOGD("gCamSysPer: %p, mCameraSessionActive = %d", gCamSysPer,
             mCameraSessionActive);

    return;
}

void SprdCameraSystemPerformance::freeSysPerformance(
    SprdCameraSystemPerformance **pgCamSysPer) {

    Mutex::Autolock l(&sLock);

    if (gCamSysPer && mCameraSessionActive == 1) {
        delete gCamSysPer;
        gCamSysPer = NULL;
        *pgCamSysPer = NULL;
    }

    mCameraSessionActive--;
    HAL_LOGD("mCameraSessionActive = %d", mCameraSessionActive);

    return;
}

void SprdCameraSystemPerformance::setCamPreformaceScene(
    sys_performance_camera_scene camera_scene) {

#ifndef CONFIG_CAMERA_DFS_FIXED_MAXLEVEL
    switch (camera_scene) {
    case CAM_PERFORMANCE_LEVEL_6:
        setPowerHint(CAM_POWER_PERFORMACE_ON);
        changeDfsPolicy(CAM_VERYHIGH);
        break;
    case CAM_PERFORMANCE_LEVEL_5:
    case CAM_PERFORMANCE_LEVEL_4:
        setPowerHint(CAM_POWER_NORMAL);
        changeDfsPolicy(CAM_NORMAL);
        break;
    case CAM_PERFORMANCE_LEVEL_3:
    case CAM_PERFORMANCE_LEVEL_2:
        setPowerHint(CAM_POWER_LOWPOWER_ON);
        changeDfsPolicy(CAM_NORMAL);
        break;
    case CAM_PERFORMANCE_LEVEL_1:
        setPowerHint(CAM_POWER_LOWPOWER_ON);
        changeDfsPolicy(CAM_LOW);
        break;
    default:
        HAL_LOGI("camera scene not support");
    }
#else
#if (CONFIG_CAMERA_DFS_FIXED_MAXLEVEL == 3)
    changeDfsPolicy(CAM_VERYHIGH);
#elif(CONFIG_CAMERA_DFS_FIXED_MAXLEVEL == 2)
    changeDfsPolicy(CAM_NORMAL);
#endif

    switch (camera_scene) {
    case CAM_PERFORMANCE_LEVEL_6:
        setPowerHint(CAM_POWER_PERFORMACE_ON);
        break;
    case CAM_PERFORMANCE_LEVEL_5:
    case CAM_PERFORMANCE_LEVEL_4:
        setPowerHint(CAM_POWER_NORMAL);
        break;
    case CAM_PERFORMANCE_LEVEL_3:
    case CAM_PERFORMANCE_LEVEL_2:
    case CAM_PERFORMANCE_LEVEL_1:
        setPowerHint(CAM_POWER_LOWPOWER_ON);
        break;
    default:
        HAL_LOGI("camera scene not support");
    }
#endif

    HAL_LOGD("x camera scene:%d", camera_scene);
}

void SprdCameraSystemPerformance::initPowerHint() {

    if (!mPowermanageInited) {
#if (CONFIG_HAS_CAMERA_HINTS_VERSION == ANDROID_VERSION_P)
        mPowerManager = new ::android::PowerHALManager();
        if (mPowerManager) {
            mPowerManager->init();
            mSceneLowPower = mPowerManager->createPowerHintScene(
                "camera", 0, "camera_lowpower");
            mScenePerformance =
                mPowerManager->createPowerHintScene("camera", 0, "camera_perf");

            mPowermanageInited = true;
            mCurrentPowerHint = CAM_POWER_NORMAL;

            HAL_LOGD("PowerHint init done");
        }
#endif
    }
}

void SprdCameraSystemPerformance::deinitPowerHint() {

    if (mPowermanageInited) {
#if (CONFIG_HAS_CAMERA_HINTS_VERSION == ANDROID_VERSION_P)
        if (mPowerManager != NULL)
            mPowerManager.clear();
        if (mSceneLowPower != NULL)
            mSceneLowPower.clear();
        if (mScenePerformance != NULL)
            mScenePerformance.clear();
#endif
        mPowermanageInited = false;

        HAL_LOGD("deinit.CurrentPowerHint status=%d", mCurrentPowerHint);
    }
}

void SprdCameraSystemPerformance::setPowerHint(
    power_hint_state_type_t powerhint_id) {

    Mutex::Autolock l(&mLock);
    if (!mPowermanageInited) {
        HAL_LOGI("need init");
        return;
    }
    HAL_LOGI("IN, mCurrentPowerHint=%d", mCurrentPowerHint);

#if (CONFIG_HAS_CAMERA_HINTS_VERSION == ANDROID_VERSION_P)
    switch (mCurrentPowerHint) {
    case CAM_POWER_NORMAL:
        if (powerhint_id == CAM_POWER_PERFORMACE_ON) {
            // thermalEnabled(false);
            acquirePowerHint(mScenePerformance);
            mCurrentPowerHint = CAM_POWER_PERFORMACE_ON;
        } else if (powerhint_id == CAM_POWER_LOWPOWER_ON) {
            acquirePowerHint(mSceneLowPower);
            mCurrentPowerHint = CAM_POWER_LOWPOWER_ON;
        } else if (powerhint_id == CAM_POWER_NORMAL) {
            HAL_LOGD("current power state is already CAM_POWER_NORMAL,"
                     "state are both 0, just return");
            goto exit;
        }
        break;
    case CAM_POWER_PERFORMACE_ON:
        if (powerhint_id == CAM_POWER_PERFORMACE_ON) {
            HAL_LOGD("current power state is already CAM_POWER_PERFORMACE_ON,"
                     "state are both 1, just return");
            goto exit;
        } else if (powerhint_id == CAM_POWER_LOWPOWER_ON) {
            releasePowerHint(mScenePerformance);
            acquirePowerHint(mSceneLowPower);
            // thermalEnabled(true);
            mCurrentPowerHint = CAM_POWER_LOWPOWER_ON;
        } else if (powerhint_id == CAM_POWER_NORMAL) {
            releasePowerHint(mScenePerformance);
            // thermalEnabled(true);
            mCurrentPowerHint = CAM_POWER_NORMAL;
        }
        break;
    case CAM_POWER_LOWPOWER_ON:
        if (powerhint_id == CAM_POWER_PERFORMACE_ON) {
            // thermalEnabled(false);
            releasePowerHint(mSceneLowPower);
            acquirePowerHint(mScenePerformance);
            mCurrentPowerHint = CAM_POWER_PERFORMACE_ON;
        } else if (powerhint_id == CAM_POWER_LOWPOWER_ON) {
            HAL_LOGD("current power state is already CAM_POWER_LOWPOWER_ON,"
                     "state are both 0, just return");
            goto exit;
        } else if (powerhint_id == CAM_POWER_NORMAL) {
            releasePowerHint(mSceneLowPower);
            mCurrentPowerHint = CAM_POWER_NORMAL;
        }
        break;
    default:
        HAL_LOGE("should not be here");
        goto exit;
    }
#endif
exit:
    HAL_LOGI("out, mCurrentPowerHint=%d", mCurrentPowerHint);
    return;
}

int SprdCameraSystemPerformance::changeDfsPolicy(dfs_policy_t dfs_policy) {

    Mutex::Autolock l(&mLock);
    switch (dfs_policy) {
    case CAM_EXIT:
        if (CAM_LOW == mCameraDfsPolicyCur) {
            releaseDfsPolicy(CAM_LOW);
        } else if (CAM_NORMAL == mCameraDfsPolicyCur) {
            releaseDfsPolicy(CAM_NORMAL);
        } else if (CAM_VERYHIGH == mCameraDfsPolicyCur)
            releaseDfsPolicy(CAM_VERYHIGH);
        mCameraDfsPolicyCur = CAM_EXIT;
        break;
    case CAM_LOW:
        if (CAM_EXIT == mCameraDfsPolicyCur) {
            setDfsPolicy(CAM_LOW);
        } else if (CAM_NORMAL == mCameraDfsPolicyCur) {
            setDfsPolicy(CAM_LOW);
            releaseDfsPolicy(CAM_NORMAL);
        } else if (CAM_VERYHIGH == mCameraDfsPolicyCur) {
            setDfsPolicy(CAM_LOW);
            releaseDfsPolicy(CAM_VERYHIGH);
        }
        mCameraDfsPolicyCur = CAM_LOW;
        break;
    case CAM_NORMAL:
        if (CAM_EXIT == mCameraDfsPolicyCur) {
            setDfsPolicy(CAM_NORMAL);
        } else if (CAM_LOW == mCameraDfsPolicyCur) {
            setDfsPolicy(CAM_NORMAL);
            releaseDfsPolicy(CAM_LOW);
        } else if (CAM_VERYHIGH == mCameraDfsPolicyCur) {
            setDfsPolicy(CAM_NORMAL);
            releaseDfsPolicy(CAM_VERYHIGH);
        }
        mCameraDfsPolicyCur = CAM_NORMAL;
        break;
    case CAM_VERYHIGH:
        if (CAM_EXIT == mCameraDfsPolicyCur) {
            setDfsPolicy(CAM_VERYHIGH);
        } else if (CAM_LOW == mCameraDfsPolicyCur) {
            setDfsPolicy(CAM_VERYHIGH);
            releaseDfsPolicy(CAM_LOW);
        } else if (CAM_NORMAL == mCameraDfsPolicyCur) {
            setDfsPolicy(CAM_VERYHIGH);
            releaseDfsPolicy(CAM_NORMAL);
        }
        mCameraDfsPolicyCur = CAM_VERYHIGH;
        break;
    default:
        HAL_LOGW("unrecognize dfs policy");
        break;
    }
    HAL_LOGI("mCameraDfsPolicyCur: %d", mCameraDfsPolicyCur);

    return NO_ERROR;
}

int SprdCameraSystemPerformance::setDfsPolicy(int dfs_policy) {

    const char *dfs_scene = NULL;
    const char *const scenario_dfs =
        "/sys/class/devfreq/scene-frequency/sprd_governor/scenario_dfs";
    FILE *fp = fopen(scenario_dfs, "wb");
    if (NULL == fp) {
        HAL_LOGW("failed to open %s X", scenario_dfs);
        return BAD_VALUE;
    }
    switch (dfs_policy) {
    case CAM_LOW:
        dfs_scene = CAM_LOW_STR;
        break;
    case CAM_NORMAL:
        dfs_scene = CAM_NORMAL_STR;
        break;
    case CAM_VERYHIGH:
        dfs_scene = CAM_VERYHIGH_STR;
        break;
    default:
        HAL_LOGW("unrecognize dfs policy");
        break;
    }
    HAL_LOGD("dfs_scene: %s", dfs_scene);
    // echo dfs_scene > scenario_dfs
    fprintf(fp, "%s", dfs_scene);
    fclose(fp);
    fp = NULL;

    return NO_ERROR;
}

int SprdCameraSystemPerformance::releaseDfsPolicy(int dfs_policy) {

    const char *dfs_scene = NULL;
    const char *const scenario_dfs =
        "/sys/class/devfreq/scene-frequency/sprd_governor/exit_scene";
    FILE *fp = fopen(scenario_dfs, "wb");
    if (NULL == fp) {
        HAL_LOGW("failed to open %s X", scenario_dfs);
        return BAD_VALUE;
    }

    switch (dfs_policy) {
    case CAM_LOW:
        dfs_scene = CAM_LOW_STR;
        break;
    case CAM_NORMAL:
        dfs_scene = CAM_NORMAL_STR;
        break;
    case CAM_VERYHIGH:
        dfs_scene = CAM_VERYHIGH_STR;
        break;
    default:
        HAL_LOGW("unrecognize dfs policy");
        break;
    }
    HAL_LOGD("release dfs_scene: %s", dfs_scene);
    // echo dfs_scene > scenario_dfs
    fprintf(fp, "%s", dfs_scene);
    fclose(fp);
    fp = NULL;
    return NO_ERROR;
}

#if (CONFIG_HAS_CAMERA_HINTS_VERSION == ANDROID_VERSION_P)
void SprdCameraSystemPerformance::acquirePowerHint(
    ::android::sp<::android::PowerHintScene> mScene) {

    if (mScene != NULL) {
        mScene->acquire();
    }
}

void SprdCameraSystemPerformance::releasePowerHint(
    ::android::sp<::android::PowerHintScene> mScene) {

    if (mScene != NULL) {
        mScene->release();
    }
}
#endif
};
