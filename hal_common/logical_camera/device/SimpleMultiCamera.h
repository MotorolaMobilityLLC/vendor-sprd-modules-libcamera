#pragma once

#include <vector>
#include <CameraDevice_3_5.h>
#include <ICameraBase.h>
#include <ICameraCreator.h>
#include <utils/List.h>
#include <utils/Mutex.h>

typedef struct {
    uint32_t frame_number;
    CameraMetadata meta;
} metadata_t;

typedef struct {
    int32_t x;
    int32_t y;
    int32_t w;
    int32_t h;
} img_region;

namespace sprdcamera {

class SimpleMultiCamera : public CameraDevice_3_5 {
  public:
    static std::shared_ptr<ICameraCreator> getCreator();

    int openCameraDevice() override;
    int getCameraInfo(camera_info_t *info) override;
    int isStreamCombinationSupported(
        const camera_stream_combination_t *streams) override;

    int initialize(const camera3_callback_ops_t *callback_ops) override;
    int configure_streams(camera3_stream_configuration_t *stream_list) override;
    const camera_metadata_t *
    construct_default_request_settings(int type) override;
    int process_capture_request(camera3_capture_request_t *request) override;
    void dump(int fd) override;
    int flush() override;
    int closeCameraDevice() override;

    virtual ~SimpleMultiCamera();
    const camera3_callback_ops_t *mCallbackOps;
    static camera3_callback_ops callback_ops_main;
    static void process_capture_result(const struct camera3_callback_ops *ops,
                                       const camera3_capture_result_t *result);
    static void notify(const struct camera3_callback_ops *ops,
                       const camera3_notify_msg_t *msg);

  private:
    SimpleMultiCamera(int cameraId, const std::vector<int> &sensorIds,
                      const std::vector<int> &physicalIds);
    void constructCharacteristics();
    void destructCharacteristics();
    void constructConflictDevices();
    void destructConflictDevices();
    void resetRequest(camera3_capture_request_t *request);
    img_region transferCropRegion(img_region src, float ratio);
    img_region getImgRegion(img_region src, float ratio);
    void setResultMetadata(CameraMetadata &curMetadata, CameraMetadata meta);
    img_region transfer3ARegions(img_region src, img_region cropRegion,
                                 float ratio);
    img_region get3ARegions(img_region src, img_region cropRegion, float ratio);

    const std::vector<int> mSensorIds;
    const std::vector<int> mPhysicalIds;
    const int mMasterId;
    camera3_device_t *mDevice;
    camera_metadata_t *mStaticCameraCharacteristics;
    size_t mConflictingDevicesLength;
    char **mConflictingDevices;
    std::map<int, camera_metadata_t *> mSettingMap;
    int32_t mActiveArrayWidth;
    int32_t mActiveArrayHeight;
    int32_t mMaxRegions[3];
    Mutex mMetaDataLock;
    List<metadata_t> mReqMetadataList;

    class SimpleMultiCameraCreator : public ICameraCreator {
        std::shared_ptr<ICameraBase>
        createInstance(std::shared_ptr<Configurator> cfg) override;
    };

    friend class SimpleMultiCameraCreator;
};
}
