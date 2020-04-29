/* Copyright (c) 2012-2013, The Linux Foundataion. All rights reserved.
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

#ifndef __SPRDCAMERA3FACTORY_H__
#define __SPRDCAMERA3FACTORY_H__

#include <map>
#include <memory>
#include <sstream>
#include <vector>
#include <hardware/camera3.h>

#include <ICameraCreator.h>

namespace sprdcamera {

class SprdCamera3Wrapper;

class SprdCamera3Factory {
  public:
    SprdCamera3Factory();
    virtual ~SprdCamera3Factory();

    /* camera3 module methods */
    static int get_number_of_cameras();
    static int get_camera_info(int camera_id, struct camera_info *info);
    static int set_callbacks(const camera_module_callbacks_t *callbacks);
    static void get_vendor_tag_ops(vendor_tag_ops_t *ops);
    static int open_legacy(const struct hw_module_t *module, const char *id,
                           uint32_t halVersion, struct hw_device_t **device);
    static int set_torch_mode(const char *camera_id, bool enabled);
    static int init();
    static int get_physical_camera_info(int physical_camera_id,
                                        camera_metadata_t **static_metadata);
    static int
    is_stream_combination_supported(int camera_id,
                                    const camera_stream_combination_t *streams);
    static void notify_device_state_change(uint64_t deviceState);

    /* hw module methods */
    static int open(const struct hw_module_t *module, const char *id,
                    struct hw_device_t **device);

  public:
    static void registerCreator(std::string name,
                                std::shared_ptr<ICameraCreator> creator);

  private:
    int getNumberOfCameras();
    int getCameraInfo(int camera_id, struct camera_info *info);
    int init_();
    int overrideCameraIdIfNeeded(int cameraId);
    int getHighResolutionSize(int camera_id, struct camera_info *info);
    static bool is_single_expose(int cameraId);
    static int multi_id_to_phyid(int cameraId);
    int getSingleCameraInfoChecked(int cameraId, struct camera_info *info);
    int open_(const struct hw_module_t *module, const char *id,
              struct hw_device_t **device);
    int cameraDeviceOpen(int camera_id, struct hw_device_t **hw_device);

  private:
    enum UseCameraId {
        PrivateId,
        DynamicId,
    };

    UseCameraId mUseCameraId;
    std::map<std::string, std::shared_ptr<ICameraCreator>> mCreators;
    std::vector<std::shared_ptr<ICameraBase>> mCameras;
    int mNumberOfCameras;
    int mNumOfCameras;
    SprdCamera3Wrapper *mWrapper;
    const camera_module_callbacks_t * mCameraCallbacks;

    void registerCameraCreators();
    void registerOneCreator(std::string name,
                            std::shared_ptr<ICameraCreator> creator);
    bool tryParseCameraConfig();
    bool validateSensorId(int id);
    int getPhysicalCameraInfo(int id, camera_metadata_t **static_metadata);
};

}; /*namespace sprdcamera*/

#endif
