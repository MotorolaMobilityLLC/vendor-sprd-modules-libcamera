#pragma once

#include <vector>
#include <CameraDevice_3_5.h>
#include <ICameraBase.h>
#include <ICameraCreator.h>

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

  private:
    SimpleMultiCamera(int cameraId, const std::vector<int> &sensorIds,
                      const std::vector<int> &physicalIds);
    void constructCharacteristics();
    void destructCharacteristics();
    void constructConflictDevices();
    void destructConflictDevices();

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

    class SimpleMultiCameraCreator : public ICameraCreator {
        std::shared_ptr<ICameraBase>
        createInstance(std::shared_ptr<Configurator> cfg) override;
    };

    friend class SimpleMultiCameraCreator;
};
}
