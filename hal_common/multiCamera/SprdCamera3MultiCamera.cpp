#define LOG_TAG "MultiCameraWrapper"

#include <SprdCamera3Factory.h>
#include "SprdCamera3MultiCamera.h"

ANDROID_SINGLETON_STATIC_INSTANCE(sprdcamera::SprdCamera3MultiCamera);

namespace sprdcamera {

int SprdCamera3MultiCamera::get_camera_info(int camera_id,
                                            struct camera_info *info) {
    auto &inst = SprdCamera3MultiCamera::getInstance();

    if (inst.mAuthorized) {
        return inst.mFuncGetCameraInfo(camera_id, info);
    } else {
        ALOGW("Fallback to id \"0\"");
        return SprdCamera3Factory::get_camera_info(0, info);
    }
}

int SprdCamera3MultiCamera::camera_device_open(const struct hw_module_t *module,
                                               const char *id,
                                               struct hw_device_t **hw_device) {
    auto &inst = SprdCamera3MultiCamera::getInstance();

    if (inst.mAuthorized) {
        return inst.mFuncOpen(module, id, hw_device);
    } else {
        ALOGW("Fallback to id \"0\"");
        return SprdCamera3Factory::mModuleMethods.open(module, "0", hw_device);
    }
}

SprdCamera3MultiCamera::SprdCamera3MultiCamera()
    : mAuthorized(false), mHandle(NULL), mFuncGetCameraInfo(NULL),
      mFuncOpen(NULL) {
    mHandle = dlopen("/vendor/lib/libmulticam.so", RTLD_LAZY);
    if (!mHandle) {
        ALOGE("Fail to load library: %s", dlerror());
        goto exit;
    }

    mFuncGetCameraInfo =
        (FuncType_GetCameraInfo)dlsym(mHandle, "get_camera_info");
    if (!mFuncGetCameraInfo) {
        ALOGE("Fail to load symbol: %s", dlerror());
        dlclose(mHandle);
        goto exit;
    }

    mFuncOpen = (FuncType_Open)dlsym(mHandle, "camera_device_open");
    if (!mFuncOpen) {
        ALOGE("Fail to load symbol: %s", dlerror());
        dlclose(mHandle);
        goto exit;
    }

    mAuthorized = true;

exit:
    ALOGI("Instantiate wrapper %s!",
          mAuthorized ? "authorized" : "unauthorized");
}

SprdCamera3MultiCamera::~SprdCamera3MultiCamera() {
    if (mAuthorized)
        dlclose(mHandle);

    ALOGI("destroy wrapper");
}
}
