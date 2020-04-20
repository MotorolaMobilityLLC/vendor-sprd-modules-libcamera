/* Copyright (c) 2012-2016, The Linux Foundataion. All rights reserved.
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

#define LOG_TAG "Cam3Factory"
//#define LOG_NDEBUG 0

#include <map>
#include <stdlib.h>
#include <utils/Log.h>
#include <utils/Errors.h>
#include <hardware/camera3.h>

#include "SprdCamera3Factory.h"
#include "SprdCamera3HWI.h"
#include "SprdCamera3Flash.h"
#include "hal_common/multiCamera/SprdCamera3Wrapper.h"
#include <SingleCameraWrapper.h>
#include <Configurator.h>
#ifdef CONFIG_BOKEH_SUPPORT
#include <BokehCamera.h>
#endif

#ifdef CONFIG_MULTICAMERA_SUPPORT
#include "SprdCamera3MultiCamera.h"
#endif

using namespace android;
using namespace std;

extern camera_module_t HAL_MODULE_INFO_SYM;

namespace sprdcamera {

static SprdCamera3Factory gSprdCamera3Factory;

SprdCamera3Factory::SprdCamera3Factory()
    : mUseCameraId(PrivateId), mNumberOfCameras(0), mNumOfCameras(0),
      mWrapper(NULL) {
    HAL_LOGD("E");

    char boot_mode[PROPERTY_VALUE_MAX] = {'\0'};

    property_get("ro.bootmode", boot_mode, "0");
    HAL_LOGI("boot_mode = %s", boot_mode);

    if (strcmp(boot_mode, "autotest") && strcmp(boot_mode, "cali")) {
        mNumberOfCameras = mNumOfCameras =
            SprdCamera3Setting::getNumberOfCameras();
    }

    SprdCamera3Wrapper::getCameraWrapper(&mWrapper);
    if (!mWrapper)
        HAL_LOGE("fail to get SprdCamera3Wrapper");

    /* register cameras */
    registerCameraCreators();
}

SprdCamera3Factory::~SprdCamera3Factory() {
    HAL_LOGD("E");

    if (mWrapper) {
        delete mWrapper;
        mWrapper = NULL;
    }

    mCameras.clear();
    mCreators.clear();
}

int SprdCamera3Factory::get_number_of_cameras() {
    HAL_LOGV("E");
    return gSprdCamera3Factory.getNumberOfCameras();
}

int SprdCamera3Factory::get_camera_info(int camera_id,
                                        struct camera_info *info) {
    HAL_LOGV("E");
    return gSprdCamera3Factory.getCameraInfo(camera_id, info);
}

int SprdCamera3Factory::set_callbacks(
    const camera_module_callbacks_t *callbacks) {
    HAL_LOGV("E");
    return SprdCamera3Flash::registerCallbacks(callbacks);
}

void SprdCamera3Factory::get_vendor_tag_ops(vendor_tag_ops_t *ops) {
    HAL_LOGV("E");

    ops->get_tag_count = SprdCamera3Setting::get_tag_count;
    ops->get_all_tags = SprdCamera3Setting::get_all_tags;
    ops->get_section_name = SprdCamera3Setting::get_section_name;
    ops->get_tag_name = SprdCamera3Setting::get_tag_name;
    ops->get_tag_type = SprdCamera3Setting::get_tag_type;
}

int SprdCamera3Factory::open_legacy(const struct hw_module_t *module,
                                    const char *id, uint32_t halVersion,
                                    struct hw_device_t **device) {
    HAL_LOGE("unsupported method");
    return -ENOSYS;
}

int SprdCamera3Factory::set_torch_mode(const char *camera_id, bool enabled) {
    HAL_LOGV("E");

    int retval = 0;

    // TODO

    retval = SprdCamera3Flash::setTorchMode(camera_id, enabled);
    HAL_LOGV("camera_id %s,enabled %d ret %d", camera_id, enabled, retval);

    return retval;
}

int SprdCamera3Factory::init() {
    HAL_LOGV("E");
    return gSprdCamera3Factory.init_();
}

int SprdCamera3Factory::get_physical_camera_info(
    int physical_camera_id, camera_metadata_t **static_metadata) {
    HAL_LOGV("E");
    return gSprdCamera3Factory.getPhysicalCameraInfo(physical_camera_id,
                                                     static_metadata);
}

int SprdCamera3Factory::is_stream_combination_supported(
    int camera_id, const camera_stream_combination_t *streams) {
    HAL_LOGV("E");

    // TODO return not supported for now
    return -EINVAL;
}

void SprdCamera3Factory::notify_device_state_change(uint64_t deviceState) {
    HAL_LOGV("E");

    // TODO
}

int SprdCamera3Factory::open(const struct hw_module_t *module, const char *id,
                             struct hw_device_t **hw_device) {
    HAL_LOGV("E");
    return gSprdCamera3Factory.open_(module, id, hw_device);
}

void SprdCamera3Factory::registerCreator(string name,
                                         shared_ptr<ICameraCreator> creator) {
    gSprdCamera3Factory.registerOneCreator(name, creator);
}

int SprdCamera3Factory::getNumberOfCameras() {
    /* Since mNumberOfCameras is corrected in init(), we can safely use it. */
    return mNumberOfCameras;
}

int SprdCamera3Factory::getCameraInfo(int camera_id, struct camera_info *info) {
    if (!info) {
        HAL_LOGE("invalid param info");
        return -EINVAL;
    }

    if (camera_id < 0) {
        HAL_LOGE("invalid param camera_id: %d", camera_id);
        return -EINVAL;
    }

    int id = overrideCameraIdIfNeeded(camera_id);
    HAL_LOGD("get_camera_info: %d", id);

    /* try dynamic ID */
    if (mUseCameraId == DynamicId && id < mNumberOfCameras)
        return mCameras[id]->getCameraInfo(info);

#ifdef CONFIG_MULTICAMERA_SUPPORT
    /* try private multi-camera */
    if (id == SPRD_MULTI_CAMERA_ID || id == SPRD_FOV_FUSION_ID ||
        id == SPRD_DUAL_VIEW_VIDEO_ID)
        return SprdCamera3MultiCamera::get_camera_info(id, info);
#endif

    /* try high resolution id */
    if (id == SPRD_BACK_HIGH_RESOLUTION_ID || id == SPRD_FRONT_HIGH_RES)
        return getHighResolutionSize(multi_id_to_phyid(id), info);

    /* try other multi-camera */
    if (mWrapper && is_single_expose(id))
        return mWrapper->getCameraInfo(id, info);

    /* get single camera id */
    int phyId = multi_id_to_phyid(id);
    if (phyId < 0)
        return -ENODEV;

    /* then try single camera */
    return getSingleCameraInfoChecked(phyId, info);
}

int SprdCamera3Factory::init_() {
    if (!mNumOfCameras)
        mNumOfCameras = SprdCamera3Setting::getNumberOfCameras();

    if (tryParseCameraConfig())
        mUseCameraId = DynamicId;

    /*
     * 1. 'autotest' or 'cali' mode seems not to obey Android call sequence.
     *    In this case mNumOfCameras is initialized in ctor.
     * 2. Normal Android case, init() will always be called. We change
     *    @mNumberOfCameras here if PrivateId mode for other API.
     */
    if (mUseCameraId == PrivateId)
        mNumberOfCameras = mNumOfCameras;

    return 0;
}

int SprdCamera3Factory::overrideCameraIdIfNeeded(int cameraId) {
    int id = cameraId;

    char value[PROPERTY_VALUE_MAX];
    property_get("persist.vendor.cam.id", value, "null");
    if (value[0] >= '0' && value[0] <= '9') {
        id = value[0] - '0';
        HAL_LOGD("fallback to %d for debug", id);
        return id;
    }

#ifdef CONFIG_MULTICAMERA_SUPPORT
    /* fallback to '0' if no multi-camera module info found */
    if ((cameraId == SPRD_MULTI_CAMERA_ID || cameraId == SPRD_FOV_FUSION_ID ||
         cameraId == SPRD_DUAL_VIEW_VIDEO_ID) &&
        !sensorGetLogicaInfo4MulitCameraId(SPRD_MULTI_CAMERA_ID)) {
        HAL_LOGW("fallback to 0 since no module info found");
        return 0;
    }
#endif

    return id;
}

int SprdCamera3Factory::getHighResolutionSize(int camera_id,
                                              struct camera_info *info) {
    camera_metadata_t *staticMetadata;
    int rc;

    HAL_LOGI("E, camera_id = %d", camera_id);

    if (!mNumOfCameras || camera_id >= mNumOfCameras || !info ||
        (camera_id < 0)) {
        return -ENODEV;
    }

    SprdCamera3Setting::getSensorStaticInfo(camera_id);

    SprdCamera3Setting::initDefaultParameters(camera_id);

    rc = SprdCamera3Setting::getStaticMetadata(camera_id, &staticMetadata);
    if (rc < 0) {
        return rc;
    }

    // TODO no free
    CameraMetadata metadata = clone_camera_metadata(staticMetadata);

    const struct img_size *stream_info;
    int32_t scaler_formats[] = {
        HAL_PIXEL_FORMAT_YCbCr_420_888, HAL_PIXEL_FORMAT_BLOB,
        HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED, HAL_PIXEL_FORMAT_RAW16};
    size_t scaler_formats_count = sizeof(scaler_formats) / sizeof(int32_t);
    int array_size = 0;
    int32_t avail_stream_config[CAMERA_SETTINGS_CONFIG_ARRAYSIZE * 4];

    memset(avail_stream_config, 0, CAMERA_SETTINGS_CONFIG_ARRAYSIZE * 4);

    SprdCamera3Setting::getHighResCapSize(camera_id, &stream_info);

    for (size_t j = 0; j < scaler_formats_count; j++) {
        for (size_t i = 0; stream_info; i++) {
            /* last img_size must be {0, 0} */
            if ((stream_info[i].width == 0) || (stream_info[i].height == 0))
                break;
            avail_stream_config[array_size++] = scaler_formats[j];
            avail_stream_config[array_size++] = stream_info[i].width;
            avail_stream_config[array_size++] = stream_info[i].height;
            avail_stream_config[array_size++] =
                ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT;
        }
    }

    metadata.update(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS,
                    avail_stream_config, array_size);

    staticMetadata = metadata.release();
    SprdCamera3Setting::getCameraInfo(camera_id, info);

    info->device_version = CAMERA_DEVICE_API_VERSION_3_2;
    info->static_camera_characteristics = staticMetadata;

    HAL_LOGV("X");
    return rc;
}

/*
 * Camera ID Expose To Camera Apk On MultiCameraMode
 * camera apk use one camera id to open camera on MODE_3D_VIDEO,
 * MODE_RANGE_FINDER, MODE_3D_CAPTURE
 * camera apk use two camera id MODE_REFOCUS and 2 to open Camera
 * ValidationTools apk use two camera id MODE_3D_CALIBRATION and 3 to open
 * camera
 */
bool SprdCamera3Factory::is_single_expose(int cameraId) {
    if ((SprdCamera3Setting::mPhysicalSensorNum > cameraId) ||
        (cameraId > SPRD_MULTI_CAMERA_MAX_ID)) {
        return false;
    }

    if ((SPRD_REFOCUS_ID == cameraId) || (SPRD_3D_CALIBRATION_ID == cameraId) ||
        (SPRD_ULTRA_WIDE_ID == cameraId) ||
        (SPRD_BACK_HIGH_RESOLUTION_ID == cameraId) ||
        (SPRD_FRONT_HIGH_RES == cameraId) ||
        (SPRD_OPTICSZOOM_W_ID == cameraId) ||
        (SPRD_OPTICSZOOM_T_ID == cameraId)) {
        return false;
    }

    return true;
}

int SprdCamera3Factory::multi_id_to_phyid(int cameraId) {
    char prop[PROPERTY_VALUE_MAX];

    if (MIN_MULTI_CAMERA_FAKE_ID > cameraId) {
        return cameraId;
    } else if (SPRD_REFOCUS_ID == cameraId ||
               SPRD_BACK_HIGH_RESOLUTION_ID == cameraId) {
        return 0;
    } else if (SPRD_3D_CALIBRATION_ID == cameraId) {
        property_get("persist.vendor.cam.ba.blur.version", prop, "0");
        if (6 == atoi(prop)) {
            return 0;
        }
        return 1;
    } else if (SPRD_ULTRA_WIDE_ID == cameraId) {
        return sensorGetRole(MODULE_SPW_NONE_BACK);
    } else if (SPRD_FRONT_HIGH_RES == cameraId) {
        return 1;
    } else if (SPRD_OPTICSZOOM_W_ID == cameraId) {
        return sensorGetRole(MODULE_OPTICSZOOM_WIDE_BACK);
    } else if (SPRD_OPTICSZOOM_T_ID == cameraId) {
        return sensorGetRole(MODULE_OPTICSZOOM_TELE_BACK);
    }

    return 0xff;
}

int SprdCamera3Factory::getSingleCameraInfoChecked(int cameraId,
                                                   struct camera_info *info) {
    camera_metadata_t *staticMetadata;
    int rc;

    SprdCamera3Setting::getSensorStaticInfo(cameraId);
    SprdCamera3Setting::initDefaultParameters(cameraId);
    rc = SprdCamera3Setting::getStaticMetadata(cameraId, &staticMetadata);
    if (rc < 0) {
        HAL_LOGE("fail to get static metadata for %d", cameraId);
        return rc;
    }

    SprdCamera3Setting::getCameraInfo(cameraId, info);

    info->device_version = CAMERA_DEVICE_API_VERSION_3_2;
    info->static_camera_characteristics = staticMetadata;

    return rc;
}

int SprdCamera3Factory::open_(const struct hw_module_t *module, const char *id,
                              struct hw_device_t **hw_device) {
    if (module != &HAL_MODULE_INFO_SYM.common) {
        HAL_LOGE("invalid param module %p, expect %p", module,
                 &HAL_MODULE_INFO_SYM.common);
        return -EINVAL;
    }

    if (!id || !hw_device) {
        HAL_LOGE("invalid param");
        return -EINVAL;
    }

    int idInt = atoi(id);
    if (idInt < 0) {
        HAL_LOGE("invalid param camera_id: %s", id);
        return -EINVAL;
    }

#ifdef CONFIG_PORTRAIT_SCENE_SUPPORT
    /* only available when enabled by IP control */
    char value[PROPERTY_VALUE_MAX];
    property_get("persist.vendor.cam.ip.wechat.back.replace", value, "0");
    if (!strcmp(value, "1")) {
        HAL_LOGD("com.tencent.mm call camera, use camera pbrp");
        if (idInt == 0) {
            idInt = 53;
        } else if (idInt == 1) {
            idInt = 52;
        } else {
            HAL_LOGI("unsupport camera id for tencent camera!!");
        }
    }
#endif

    int cameraId = overrideCameraIdIfNeeded(idInt);
    HAL_LOGD("open: %d", cameraId);

    /* try dynamic ID */
    if (mUseCameraId == DynamicId && cameraId < mNumberOfCameras)
        return mCameras[cameraId]->openCamera(hw_device);

#ifdef CONFIG_MULTICAMERA_SUPPORT
    /* try private multi-camera */
    if (cameraId == SPRD_MULTI_CAMERA_ID || cameraId == SPRD_FOV_FUSION_ID ||
        cameraId == SPRD_DUAL_VIEW_VIDEO_ID)
        return SprdCamera3MultiCamera::camera_device_open(module, id,
                                                          hw_device);
#endif

    /* try other multi-camera */
    if (mWrapper && is_single_expose(cameraId))
        return mWrapper->cameraDeviceOpen(module, id, hw_device);

    /* then try single camera */
    return cameraDeviceOpen(cameraId, hw_device);
}

int SprdCamera3Factory::cameraDeviceOpen(int camera_id,
                                         struct hw_device_t **hw_device) {
    int rc = NO_ERROR;
    int phyId = 0;
    int is_high_res_mode = 0;
    unsigned int on_off = 0;
    struct phySensorInfo *phyPtr = NULL;
    char prop[PROPERTY_VALUE_MAX];

    phyId = multi_id_to_phyid(camera_id);
    if (camera_id < 0 || phyId >= mNumOfCameras)
        return -ENODEV;

    if (phyId < 0)
        phyId = 0;
    phyPtr = sensorGetPhysicalSnsInfo(phyId);
    if (phyPtr->phyId != phyPtr->slotId)
        return -ENODEV;

    SprdCamera3HWI *hw = new SprdCamera3HWI(phyId);

    if (!hw) {
        HAL_LOGE("Allocation of hardware interface failed");
        return NO_MEMORY;
    }

    // Only 3d calibration use this
    // TBD: move 3d calibration to multi camera
    HAL_LOGD("SPRD Camera Hal, camera_id=%d", camera_id);
    if (SPRD_3D_CALIBRATION_ID == camera_id) {
        hw->setMultiCameraMode(MODE_3D_CALIBRATION);
        property_set("vendor.cam.dualmode", "high_mode");
    }
    if (SPRD_OPTICSZOOM_W_ID == camera_id) {
        hw->setMultiCameraMode(MODE_OPTICSZOOM_CALIBRATION);
    }
    if (SPRD_OPTICSZOOM_T_ID == camera_id) {
        hw->setMultiCameraMode(MODE_OPTICSZOOM_CALIBRATION);
    }
    rc = hw->openCamera(hw_device);
    if (rc != 0) {
        delete hw;
    } else {
        if (SPRD_FRONT_HIGH_RES == camera_id ||
            SPRD_BACK_HIGH_RESOLUTION_ID == camera_id) {
            is_high_res_mode = 1;
            hw->camera_ioctrl(CAMERA_TOCTRL_SET_HIGH_RES_MODE,
                              &is_high_res_mode, NULL);
        }

        property_get("persist.vendor.cam.ultrawide.enable", prop, "1");
        if (SPRD_ULTRA_WIDE_ID == camera_id)
            on_off = atoi(prop);
        hw->camera_ioctrl(CAMERA_IOCTRL_ULTRA_WIDE_MODE, &on_off, NULL);
    }

    return rc;
}

void SprdCamera3Factory::registerCameraCreators() {
    /* single camera wrapper */
    registerCreator(Configurator::kFeatureSingleCamera,
                    SingleCamera::getCreator());

#ifdef CONFIG_BOKEH_SUPPORT
    /* bokeh */
    registerCreator(Configurator::kFeatureBokehCamera,
                    BokehCamera::getCreator());
#endif

#ifdef CONFIG_MULTICAMERA_SUPPORT
    /* private cameras */
    SprdCamera3MultiCamera::getInstance();
#endif
}

void SprdCamera3Factory::registerOneCreator(
    string name, shared_ptr<ICameraCreator> creator) {
    if (mCreators.find(name) != mCreators.cend()) {
        HAL_LOGE("'%s' has already registered!", name.c_str());
        return;
    }

    mCreators[name] = creator;
    HAL_LOGD("register a '%s' camera", name.c_str());
}

bool SprdCamera3Factory::tryParseCameraConfig() {
    auto rawConfigs = Configurator::getConfigurators();
    if (!rawConfigs.size())
        return false;

    vector<shared_ptr<Configurator>> configs;
    map<int, int> singleCameras;

    /* filter out invalid raw configs */
    for (auto cfg : rawConfigs) {
        /* check if creator of this type exists */
        string type = cfg->getType();
        if (mCreators.find(type) == mCreators.cend()) {
            ALOGW("unrecognized camera '%s', ignore", type.c_str());
            continue;
        }

        /* check sensor id count */
        auto sensorIds = cfg->getSensorIds();
        if (!sensorIds.size() || (type == Configurator::kFeatureSingleCamera &&
                                  sensorIds.size() > 1)) {
            ALOGW("invalid sensor count %zu as '%s', ignore", sensorIds.size(),
                  type.c_str());
            continue;
        }

        /* check if sensor id is valid */
        bool valid = true;
        int invalidId = -1;
        for (auto id : sensorIds) {
            if (!validateSensorId(id)) {
                invalidId = id;
                valid = false;
                break;
            }
        }
        if (!valid) {
            ALOGW("invalid sensor id %d, ignore", invalidId);
            continue;
        }

        /* assign camera ID */
        auto c = cfg->clone();
        c->setCameraId(configs.size());
        if (type == Configurator::kFeatureSingleCamera &&
            singleCameras.find(sensorIds[0]) == singleCameras.cend()) {
            /* record first single camera ID */
            singleCameras[sensorIds[0]] = c->getCameraId();
        }
        configs.push_back(c);
    }

    /* assign physical camera IDs */
    size_t count = configs.size();
    for (size_t id = 0; id < count; id++) {
        if (configs[id]->getType() == Configurator::kFeatureSingleCamera)
            continue;

        vector<int> physicalIds;
        for (auto sensorId : configs[id]->getSensorIds()) {
            if (singleCameras.find(sensorId) == singleCameras.cend()) {
                auto c = Configurator::createInstance(configs.size(), sensorId);
                singleCameras[sensorId] = c->getCameraId();
                configs.push_back(c);
            }
            physicalIds.push_back(singleCameras[sensorId]);
        }

        configs[id]->setPhysicalIds(physicalIds);
    }

    /* create camera instance */
    for (auto cfg : configs) {
        auto camera = mCreators[cfg->getType()]->createInstance(cfg);
        if (!camera) {
            ALOGW("fail to create camera %d as '%s', ignore",
                  cfg->getCameraId(), cfg->getType().c_str());
            count--;
            continue;
        }
        ALOGI("create camera: %s", cfg->getBrief().c_str());
        mCameras.push_back(camera);
    }

    /* create hidden cameras */
    mNumberOfCameras = count;
    ALOGI("dynamic camera id is ready, %d visible, %zu hidden",
          mNumberOfCameras, mCameras.size() - mNumberOfCameras);

    return true;
}

bool SprdCamera3Factory::validateSensorId(int id) {
    /* for now, @mNumOfCameras is sensor num used as camera id */
    return id < mNumOfCameras;
}

int SprdCamera3Factory::getPhysicalCameraInfo(
    int id, camera_metadata_t **static_metadata) {
    if (!static_metadata) {
        HAL_LOGE("invalid parameter");
        return -EINVAL;
    }

    *static_metadata = NULL;

    if (id < mNumberOfCameras || id >= mCameras.size()) {
        HAL_LOGE("invalid physical camera id %d", id);
        return -ENODEV;
    }

    struct camera_info info;
    if (mCameras[id]->getCameraInfo(&info) < 0) {
        HAL_LOGE("fail to get camera info for %d", id);
        return -ENODEV;
    }

    *static_metadata =
        const_cast<camera_metadata_t *>(info.static_camera_characteristics);
    return 0;
}

}; // namespace sprdcamera
