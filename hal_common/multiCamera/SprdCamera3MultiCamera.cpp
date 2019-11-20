#define LOG_TAG "MultiCameraWrapper"

#include <SprdCamera3Factory.h>
#include "SprdCamera3MultiCamera.h"

namespace sprdcamera {

int SprdCamera3MultiCamera::get_camera_info(int camera_id,
        struct camera_info *info)
{
    if (GetInstance()->mAuthorized) {
        return GetInstance()->mFunc1(camera_id, info);
    } else {
        ALOGW("Fallback to id \"0\"");
        return SprdCamera3Factory::get_camera_info(0, info);
    }
}

int SprdCamera3MultiCamera::camera_device_open(const struct hw_module_t *module,
        const char *id, struct hw_device_t **hw_device)
{
    if (GetInstance()->mAuthorized) {
        return GetInstance()->mFunc2(module, id, hw_device);
    } else {
        ALOGW("Fallback to id \"0\"");
        return SprdCamera3Factory::mModuleMethods.open(module, "0", hw_device);
    }
}

SprdCamera3MultiCamera *SprdCamera3MultiCamera::sInstance = NULL;

SprdCamera3MultiCamera *SprdCamera3MultiCamera::GetInstance() {
    if (!sInstance) {
        void *lib = dlopen("/vendor/lib/libmulticam.so", RTLD_LAZY);
        if (!lib) {
            ALOGE("Fail to load library: %s", dlerror());
            sInstance = new SprdCamera3MultiCamera();
            return sInstance;
        }

        FuncType1 f1 = (FuncType1)dlsym(lib, "get_camera_info");
        if (!f1) {
            ALOGE("Fail to load symbol: %s", dlerror());
            sInstance = new SprdCamera3MultiCamera();
            return sInstance;
        }

        FuncType2 f2 = (FuncType2)dlsym(lib, "camera_device_open");
        if (!f2) {
            ALOGE("Fail to load symbol: %s", dlerror());
            sInstance = new SprdCamera3MultiCamera();
            return sInstance;
        }

        sInstance = new SprdCamera3MultiCamera(lib, f1, f2);
    }

ret:
    return sInstance;
}

SprdCamera3MultiCamera::SprdCamera3MultiCamera()
    : mAuthorized(false), mHandle(NULL), mFunc1(NULL), mFunc2(NULL)
{
    ALOGI("Instantiate wrapper unauthorized!");
}

SprdCamera3MultiCamera::SprdCamera3MultiCamera(HandleType h, FuncType1 f1, FuncType2 f2)
    : mAuthorized(true), mHandle(h), mFunc1(f1), mFunc2(f2)
{
    ALOGI("Instantiate wrapper");
}

SprdCamera3MultiCamera::~SprdCamera3MultiCamera() {
    dlclose(mHandle);
}

}

