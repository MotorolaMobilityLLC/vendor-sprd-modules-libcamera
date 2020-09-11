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
#include <set>
#include <sstream>
#include <vector>
#include <hardware/camera3.h>

#include <ICameraCreator.h>

namespace sprdcamera {

class SprdCamera3Wrapper;

class SprdCamera3Factory : public ICameraBase::CameraClosedListener {
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

    /* camera module callbacks */
    static void
    camera_device_status_change(const camera_module_callbacks_t *callbacks,
                                int camera_id, int new_status);
    static void
    torch_mode_status_change(const camera_module_callbacks_t *callbacks,
                             const char *camera_id, int new_status);

  public:
    static void registerCreator(std::string name,
                                std::shared_ptr<ICameraCreator> creator);
    void onCameraClosed(int camera_id);

  private:
    int getNumberOfCameras();
    int getDynamicCameraIdInfo(int camera_id, struct camera_info *info);
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
    int setCallbacks(const camera_module_callbacks_t *callbacks);
    void torchModeStatusChange(const char *camera_id, int new_status) const;
    int setTorchMode(const char *camera_id, bool enabled);
    void initializeTorchHelper(const camera_module_callbacks_t *callbacks);

  private:
    enum UseCameraId {
        PrivateId,
        DynamicId,
    };

    UseCameraId mUseCameraId;
    std::map<std::string, std::shared_ptr<ICameraCreator>> mCreators;
    std::vector<std::shared_ptr<ICameraBase>> mCameras;
    std::map<int, int> mHiddenCameraParents;
    std::map<int, std::set<int>> mConflictingCameraIds;
    std::map<int, char **> mConflictingDevices;
    std::map<int, size_t> mConflictingDevicesCount;
    int mNumberOfCameras;
    int mNumOfCameras;
    SprdCamera3Wrapper *mWrapper;
    const camera_module_callbacks_t *mCameraCallbacks;

    /*
     * TODO refine flash module
     *
     * In our SprdCamera3Flash design, there's only two kinds of torch: rear
     * and front. Even the front torch is implemented by LCD light and is a
     * private API. So we collect all cameras facing BACK here and make them
     * mutually exclusive for torch function. This is a temporary design that
     * should be optimized later - when we have more powerful flash module.
     */
    class TorchHelper {
      public:
        static const int REAR_TORCH = 0;
        static const int FRONT_TORCH = 1;

        void addTorchUser(int id, bool isFront = false);
        int setTorchMode(int id, bool enabled);
        /* camera @id locks flash unit */
        void lockTorch(int id);
        /* camera @id unlocks flash unit */
        void unlockTorch(int id);
        size_t totalSize() const {
            return mFrontTorchStatus.size() + mRearTorchStatus.size();
        }
        void setCallbacks(const camera_module_callbacks_t *callbacks) {
            mCallbacks = callbacks;
        }

      private:
        int set(int id, bool enabled, int direction,
                std::map<int, int> &statusMap, std::vector<int> &locker);
        void lock(int id, std::map<int, int> &statusMap,
                  std::vector<int> &locker);
        void unlock(int id, std::map<int, int> &statusMap,
                    std::vector<int> &locker);
        void callback(const char *id, int status);

        const camera_module_callbacks_t *mCallbacks;
        std::map<int, int> mRearTorchStatus;
        std::vector<int> mRearTorchLocker;
        std::map<int, int> mFrontTorchStatus;
        std::vector<int> mFrontTorchLocker;
        std::mutex mMutex;
    };
    std::shared_ptr<TorchHelper> mTorchHelper;

    /*
     * callbacks for flash module
     *
     * don't pass callbacks to flash module since we will handle camera ID
     * conversion int SprdCamera3Factory
     */
    struct flash_callback_t : public camera_module_callbacks_t {
        SprdCamera3Factory const *parent;

        flash_callback_t(SprdCamera3Factory *parent)
            : camera_module_callbacks_t(
                  {SprdCamera3Factory::camera_device_status_change,
                   SprdCamera3Factory::torch_mode_status_change}),
              parent(parent) {}
    };
    const flash_callback_t mFlashCallback;

    void registerCameraCreators();
    void registerOneCreator(std::string name,
                            std::shared_ptr<ICameraCreator> creator);
    bool tryParseCameraConfig();
    bool validateSensorId(int id);
    int getPhysicalCameraInfo(int id, camera_metadata_t **static_metadata);
    int
    isStreamCombinationSupported(int id,
                                 const camera_stream_combination_t *streams);
    char **allocateConflictingDevices(const std::vector<int> &devices);
    void freeConflictingDevices(char **devices, size_t size);
};

}; /*namespace sprdcamera*/

#endif
