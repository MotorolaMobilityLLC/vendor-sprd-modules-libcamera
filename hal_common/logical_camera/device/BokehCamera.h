#pragma once

#include <CameraDevice_3_5.h>
#include <ICameraBase.h>
#include <ICameraCreator.h>

namespace sprdcamera {

class BokehCamera : public CameraDevice_3_5, public ICameraBase {
  public:
    virtual ~BokehCamera();

    static std::shared_ptr<ICameraCreator> getCreator();

    int openCamera(hw_device_t **dev) override;
    int getCameraInfo(camera_info_t *info) override;

  private:
    BokehCamera();

    int close();
    int initialize(const camera3_callback_ops_t *callback_ops) override;
    int configure_streams(camera3_stream_configuration_t *stream_list) override;
    const camera_metadata_t *
    construct_default_request_settings(int type) override;
    int process_capture_request(camera3_capture_request_t *request) override;
    void dump(int fd) override;
    int flush() override;

    class BokehCreator : public ICameraCreator {
        std::shared_ptr<ICameraBase>
        createInstance(std::shared_ptr<Configurator> cfg) override;
    };

    friend class BokehCreator;
};
}
