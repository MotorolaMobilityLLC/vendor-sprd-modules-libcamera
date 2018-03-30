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

//#define CAM_EXIT_STR          "exit"
#define CAM_LOW_STR "camlow"
#define CAM_NORMAL_STR "camhigh"
#define CAM_VERYHIGH_STR "camveryhigh"

SprdCameraSystemPerformance *gCamSysPer = NULL;
Mutex SprdCameraSystemPerformance::sLock;
// Error Check Macros
#define CHECK_SYSTEMPERFORMACE()                                               \
    if (!gCamSysPer) {                                                         \
        HAL_LOGE("Error getting CamSysPer ");                                  \
        return;                                                                \
    }

SprdCameraSystemPerformance::SprdCameraSystemPerformance() {
    HAL_LOGI(" E");

    mCurrentPowerHint = CAM_POWER_NORMAL;
    mCameraDfsPolicyCur = CAM_EXIT;
    mPowermanageInited = false;

#if (CONFIG_HAS_CAMERA_HINTS_VERSION == ANDROID_VERSION_O)
    mPowerManager = NULL;
    mPowerManagerLowPower = NULL;
    mPrfmLock = NULL;
    mPrfmLockLowPower = NULL;
    mThermalManager = NULL;
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
    HAL_LOGD("gCamSysPer: %p", gCamSysPer);

    return;
}

void SprdCameraSystemPerformance::freeSysPerformance(
    SprdCameraSystemPerformance **pgCamSysPer) {

    Mutex::Autolock l(&sLock);

    if (gCamSysPer) {
        delete gCamSysPer;
        gCamSysPer = NULL;
        *pgCamSysPer = NULL;
    }

    return;
}

void SprdCameraSystemPerformance::setCamPreformaceScene(
    sys_performance_camera_scene camera_scene) {

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

    HAL_LOGD("x camera scene:%d", camera_scene);
}

void SprdCameraSystemPerformance::initPowerHint() {

    if (!mPowermanageInited) {
#if (CONFIG_HAS_CAMERA_HINTS_VERSION == ANDROID_VERSION_O)
        mThermalManager = IExtThermal::getService();
        sp<IPower> mPowerManagerEnable = IPower::getService();
        if (mThermalManager == NULL) {
            HAL_LOGE("Thread cannot get the Thermal service");
        }

        if (mPowerManagerEnable == NULL) {
            HAL_LOGE("Thread cannot get the power service");
        } else {
            mPowerManager = mPowerManagerEnable;
            mPowerManagerLowPower = mPowerManagerEnable;
        }

        mPowermanageInited = true;
        mCurrentPowerHint = CAM_POWER_NORMAL;
#elif(CONFIG_HAS_CAMERA_HINTS_VERSION == ANDROID_VERSION_N)
        char value[PROPERTY_VALUE_MAX];
        sp<IPowerManager> mPowerManagerEnable = NULL;
        if (mPowerManagerEnable == NULL) {
            // use checkService() to avoid blocking if power service is not up
            // yet
            sp<IBinder> binder =
                defaultServiceManager()->checkService(String16("power"));
            if (binder == NULL) {
                HAL_LOGE("Thread cannot connect to the power manager service");
            } else {
                mPowerManagerEnable = interface_cast<IPowerManager>(binder);
            }
        }
        if (mPowerManagerEnable != NULL) {
            mPowerManager = mPowerManagerEnable;
#ifdef CONFIG_CAMERA_POWERHINT_LOWPOWER
            property_get("debug.camera.dis.lowpower", value, "0");
            if (!atoi(value)) {
                mPowerManagerLowPower = mPowerManagerEnable;
            }
#endif
        }
        mPowermanageInited = true;
        mCurrentPowerHint = CAM_POWER_NORMAL;
#endif
    }
}

void SprdCameraSystemPerformance::deinitPowerHint() {

    if (mPowermanageInited) {
#if (CONFIG_HAS_CAMERA_HINTS_VERSION == ANDROID_VERSION_N)
        mPowermanageInited = 0;
        if (mPowerManager != NULL)
            mPowerManager.clear();
        if (mPrfmLock != NULL)
            mPrfmLock.clear();
#ifdef CONFIG_CAMERA_POWERHINT_LOWPOWER
        if (mPowerManagerLowPower != NULL)
            mPowerManagerLowPower.clear();
        if (mPrfmLockLowPower != NULL)
            mPrfmLockLowPower.clear();
#endif
#endif

#if (CONFIG_HAS_CAMERA_HINTS_VERSION == ANDROID_VERSION_O)
        mPowermanageInited = 0;
        if (mPowerManager != NULL)
            mPowerManager.clear();
        if (mPrfmLock != NULL)
            mPrfmLock.clear();
        if (mPowerManagerLowPower != NULL)
            mPowerManagerLowPower.clear();
        if (mPrfmLockLowPower != NULL)
            mPrfmLockLowPower.clear();
        if (mThermalManager != NULL)
            mThermalManager.clear();
#endif
        mPowermanageInited = false;

        HAL_LOGD("deinit.CurrentPowerHint status=%d", mCurrentPowerHint);
    }
}

void SprdCameraSystemPerformance::setPowerHint(
    power_hint_state_type_t powerhint_id) {

    Mutex::Autolock l(&mLock);
    if (!mPowermanageInited) {
        HAL_LOGE("need init.");
        return;
    }
    HAL_LOGI("IN, mCurrentPowerHint=%d", mCurrentPowerHint);

#if (CONFIG_HAS_CAMERA_HINTS_VERSION == ANDROID_VERSION_O)
    switch (mCurrentPowerHint) {
    case CAM_POWER_NORMAL:
        if (powerhint_id == CAM_POWER_PERFORMACE_ON) {
            thermalEnabled(false);
            acquirePowerHint(mPowerManager, PowerHint::VENDOR_CAMERA_PERFORMANCE);
            mCurrentPowerHint = CAM_POWER_PERFORMACE_ON;
        } else if (powerhint_id == CAM_POWER_LOWPOWER_ON) {
            acquirePowerHint(mPowerManagerLowPower, PowerHint::VENDOR_CAMERA_LOW_POWER);
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
            releasePowerHint(mPowerManager, PowerHint::VENDOR_CAMERA_PERFORMANCE);
            acquirePowerHint(mPowerManagerLowPower, PowerHint::VENDOR_CAMERA_LOW_POWER);
            thermalEnabled(true);
            mCurrentPowerHint = CAM_POWER_LOWPOWER_ON;
        } else if (powerhint_id == CAM_POWER_NORMAL) {
            releasePowerHint(mPowerManager, PowerHint::VENDOR_CAMERA_PERFORMANCE);
            thermalEnabled(true);
            mCurrentPowerHint = CAM_POWER_NORMAL;
        }
        break;
    case CAM_POWER_LOWPOWER_ON:
        if (powerhint_id == CAM_POWER_PERFORMACE_ON) {
            thermalEnabled(false);
            releasePowerHint(mPowerManagerLowPower, PowerHint::VENDOR_CAMERA_LOW_POWER);
            acquirePowerHint(mPowerManager, PowerHint::VENDOR_CAMERA_PERFORMANCE);
            mCurrentPowerHint = CAM_POWER_PERFORMACE_ON;
        } else if (powerhint_id == CAM_POWER_LOWPOWER_ON) {
            HAL_LOGD("current power state is already CAM_POWER_LOWPOWER_ON,"
                     "state are both 0, just return");
            goto exit;
        } else if (powerhint_id == CAM_POWER_NORMAL) {
            releasePowerHint(mPowerManagerLowPower, PowerHint::VENDOR_CAMERA_LOW_POWER);
            mCurrentPowerHint = CAM_POWER_NORMAL;
        }
        break;
    default:
        HAL_LOGE("should not be here");
        goto exit;
    }
#elif(CONFIG_HAS_CAMERA_HINTS_VERSION == ANDROID_VERSION_N)
    switch (mCurrentPowerHint) {
    case CAM_POWER_NORMAL:
        if (powerhint_id == CAM_POWER_PERFORMACE_ON) {
            enablePowerHintExt(mPowerManager, mPrfmLock,
                               POWER_HINT_VENDOR_CAMERA_PERFORMANCE);
            // disable thermal
            thermalEnabled(false);
            mCurrentPowerHint = CAM_POWER_PERFORMACE_ON;
        } else if (powerhint_id == CAM_POWER_LOWPOWER_ON) {
            enablePowerHintExt(mPowerManagerLowPower, mPrfmLockLowPower,
                               POWER_HINT_VENDOR_CAMERA_LOW_POWER);
            mCurrentPowerHint = CAM_POWER_LOWPOWER_ON;
        } else if (powerhint_id == CAM_POWER_NORMAL) {
            HAL_LOGD("current power state is already POWER_IDLE, just return");
            goto exit;
        }
        break;
    case CAM_POWER_PERFORMACE_ON:
        if (powerhint_id == CAM_POWER_PERFORMACE_ON) {
            HAL_LOGI("current power state is already POWER_PERFORMACE_ON, "
                     "just return");
            goto exit;
        } else if (powerhint_id == CAM_POWER_LOWPOWER_ON) {
            // first, disable CAMERA_POWER_HINT_PERFORMANCE
            disablePowerHintExt(mPowerManager, mPrfmLock);
            thermalEnabled(true);

            // second, enable CAMERA_POWER_HINT_LOWPOWER
            enablePowerHintExt(mPowerManagerLowPower, mPrfmLockLowPower,
                               POWER_HINT_VENDOR_CAMERA_LOW_POWER);
            mCurrentPowerHint = CAM_POWER_LOWPOWER_ON;
        } else if (powerhint_id == CAM_POWER_NORMAL) {
            disablePowerHintExt(mPowerManager, mPrfmLock);
            thermalEnabled(true);
            mCurrentPowerHint = CAM_POWER_NORMAL;
        }
        break;
    case CAM_POWER_LOWPOWER_ON:
        if (powerhint_id == CAM_POWER_PERFORMACE_ON) {
            // first, disable CAMERA_POWER_HINT_LOWPOWER
            disablePowerHintExt(mPowerManagerLowPower, mPrfmLockLowPower);

            // second, enable POWER_HINT_VENDOR_CAMERA_HDR
            enablePowerHintExt(mPowerManager, mPrfmLock,
                               POWER_HINT_VENDOR_CAMERA_PERFORMANCE);
            // disable thermal
            thermalEnabled(false);
            mCurrentPowerHint = CAM_POWER_PERFORMACE_ON;
        } else if (powerhint_id == CAM_POWER_LOWPOWER_ON) {
            HAL_LOGD("current power state is already POWER_LOWPOWER_ON,"
                     "just return");
            goto exit;
        } else if (powerhint_id == CAM_POWER_NORMAL) {
            // first, disable CAMERA_POWER_HINT_LOWPOWER
            disablePowerHintExt(mPowerManagerLowPower, mPrfmLockLowPower);
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

#if (CONFIG_HAS_CAMERA_HINTS_VERSION == ANDROID_VERSION_O)
void SprdCameraSystemPerformance::acquirePowerHint(
    sp<IPower> powermanager, PowerHint id) {

    if (powermanager != NULL) {
        sp<IBase> lock = new IBase();
        powermanager->acquirePowerHintBySceneId(lock, "Camera", (int32_t)id);
        HAL_LOGI("powerhint enabled,%x", id);

        if (id == PowerHint::VENDOR_CAMERA_PERFORMANCE) {
            mPrfmLock = lock;
        } else if (id == PowerHint::VENDOR_CAMERA_LOW_POWER) {
            mPrfmLockLowPower = lock;
        }
    }
}

void SprdCameraSystemPerformance::releasePowerHint(
    sp<IPower> powermanager, PowerHint id) {

    if (powermanager != NULL) {
        if(id == PowerHint::VENDOR_CAMERA_PERFORMANCE) {
            powermanager->releasePowerHintBySceneId(mPrfmLock, (int32_t)id);
            mPrfmLock.clear();
        } else if (id == PowerHint::VENDOR_CAMERA_LOW_POWER) {
            powermanager->releasePowerHintBySceneId(mPrfmLockLowPower, (int32_t)id);
            mPrfmLockLowPower.clear();
        }
        HAL_LOGI("powerhint disable,%x", id);
    }
}
void SprdCameraSystemPerformance::thermalEnabled(bool flag){
    if (mThermalManager != NULL) {
        if(flag == false){
           mThermalManager->setExtThermal(ExtThermalCmd::THMCMD_SET_PERF_EN);
        }else{
           mThermalManager->setExtThermal(ExtThermalCmd::THMCMD_SET_PERF_DIS);
        }
        HAL_LOGI("thermalEnabled done,flag is =%d", flag);
    }
}
#endif

#if (CONFIG_HAS_CAMERA_HINTS_VERSION == ANDROID_VERSION_N)
void SprdCameraSystemPerformance::thermalEnabled(bool flag) {
    int therm_fd = -1;
    int i = 0;
    char buf[20] = {0};
    char *p = NULL;

    therm_fd = socket_local_client(
        "thermald", ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
    if (therm_fd < 0)
        HAL_LOGI("%s open thermald  failed: %s\n", __func__, strerror(errno));
    else {
        p = buf;
        if (flag) {
            p += snprintf(p, 20, "%s", "SetPerfDis");
        } else {
            p += snprintf(p, 20, "%s", "SetPerfEn");
        }
        write(therm_fd, buf, strlen(buf));
        close(therm_fd);
        HAL_LOGI("%s, strlen of buf: %d, flag: %d", buf, strlen(buf), flag);
    }
}

void SprdCameraSystemPerformance::enablePowerHintExt(
    sp<IPowerManager> powermanager, sp<IBinder> prfmlock, int powerhint_id) {

    if (prfmlock != NULL) {
        if (powermanager != 0) {
            HAL_LOGI("releaseWakeLock_l() - Prfmlock ");
            powermanager->releasePrfmLock(prfmlock);
        }
        prfmlock.clear();
    }
    if (powermanager != NULL) {
        sp<IBinder> binder = new BBinder();
        powermanager->acquirePrfmLock(binder, String16("Camera"),
                                      String16("CameraServer"), powerhint_id);
        prfmlock = binder;
        HAL_LOGI("powerhint enabled,%d", powerhint_id);
    }

    if (powerhint_id == POWER_HINT_VENDOR_CAMERA_PERFORMANCE) {
        mPrfmLock = prfmlock;
    } else if (powerhint_id == POWER_HINT_VENDOR_CAMERA_LOW_POWER) {
        mPrfmLockLowPower = prfmlock;
    }
}

void SprdCameraSystemPerformance::disablePowerHintExt(
    sp<IPowerManager> powermanager, sp<IBinder> prfmlock) {

    if (prfmlock != 0) {
        if (powermanager != 0) {
            HAL_LOGI("releaseWakeLock_l() - Prfmlock ");
            powermanager->releasePrfmLock(prfmlock);
        }
        prfmlock.clear();
    }
}
#endif
};
