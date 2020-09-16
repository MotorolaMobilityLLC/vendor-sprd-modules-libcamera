LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

define all-cpp-files-under
$(patsubst ./%,%, \
  $(shell cd $(LOCAL_PATH) ; \
          find $(1) -name "*.cpp" -and -not -name ".*"))
endef

LOCAL_C_INCLUDES := \
	$(TOP)/system/media/camera/include \
	$(TOP)/hardware/interfaces/camera/common/1.0/default/include \
	$(TOP)/frameworks/av/media/libstagefright/include \
	$(TOP)/frameworks/native/libs/nativewindow/include \
	$(TOP)/frameworks/native/include \
	$(TOP)/vendor/sprd/modules/libmemion \
	$(TOP)/vendor/sprd/external/kernel-headers \
	$(TOP)/hardware/libhardware/modules/gralloc \
	$(TOP)/vendor/sprd/external/drivers/gpu/soft/include \
	$(TOP)/vendor/sprd/external/drivers/gpu \
	$(TOP)/frameworks/av/services/camera/libcameraservice \
	$(TOP)/system/media/private/camera/include \
	$(TOP)/vendor/sprd/modules/libcamera/hal3_2v6 \
	$(TOP)/vendor/sprd/modules/libcamera/common/inc \
	$(TOP)/vendor/sprd/modules/libcamera/oem2v6/inc \
	$(TOP)/vendor/sprd/modules/libcamera/include \
	$(TOP)/vendor/sprd/modules/libcamera/ispalg/common/inc \
	$(TOP)/vendor/sprd/modules/libcamera/camdrv/isp2.6/middleware/inc \
	$(TOP)/vendor/sprd/modules/libcamera/camdrv/isp2.6/driver/inc \
	$(TOP)/bsp/modules/camera/interface \
	$(TOP)/bsp/kernel/kernel4.14/include/uapi/video \
	$(TOP)/vendor/sprd/modules/libcamera/jpeg \
	$(TOP)/vendor/sprd/modules/libcamera/sensor/inc \
	$(TOP)/vendor/sprd/modules/libcamera/hal_common/camera_power_perf \
	$(TOP)/vendor/sprd/modules/libcamera \
	$(TOP)/vendor/sprd/modules/libcamera/test/suite \
	$(TOP)/vendor/sprd/modules/libcamera/arithmetic/facebeauty/inc \
	$(TOP)/vendor/sprd/modules/libcamera/arithmetic/inc \
	$(TOP)/vendor/sprd/modules/libcamera/test/framework/json/include \
    $(TOP)/vendor/sprd/modules/libcamera/ispalg/common/inc \
    $(TOP)/system/memory/libion/kernel-headers \
	$(TOP)/vendor/sprd/modules/libcamera/hal1.0/inc \
	$(TOP)/system/core/include \
	$(TOP)/bsp/modules/common/camera/interface/ \
	$(LOCAL_PATH)/module \
	$(LOCAL_PATH)/suite \
	$(LOCAL_PATH)/framework\
	$(LOCAL_PATH)/framework/json/include \
	$(LOCAL_PATH)

LOCAL_SHARED_LIBRARIES := libbinder libcamera_metadata libcutils libfmq libui libbase libcutils liblog libhidlbase libhidltransport libhwbinder libutils libhardware libdl libxml2
LOCAL_SHARED_LIBRARIES += libcamoem libcamcommon libmemion libcamsensor

LOCAL_CFLAGS:=-o0 -g
LOCAL_CFLAGS += -DTARGET_GPU_PLATFORM=$(TARGET_GPU_PLATFORM)

LOCAL_STATIC_LIBRARIES :=android.hardware.camera.common@1.0 android.hardware.camera.common@1.0-helper android.hardware.camera.device@1.0 android.hardware.camera.device@3.2
LOCAL_STATIC_LIBRARIES +=android.hardware.camera.device@3.3 android.hardware.camera.device@3.4 android.hardware.camera.provider@2.4 android.hardware.graphics.allocator@2.0
LOCAL_STATIC_LIBRARIES +=android.hardware.graphics.common@1.0 android.hardware.graphics.common@1.0 android.hardware.graphics.mapper@2.0 android.hidl.allocator@1.0 libgrallocusage libhidlmemory
LOCAL_STATIC_LIBRARIES += libjsoncpp

LOCAL_SRC_FILES := $(call all-cpp-files-under,.)

LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE := CameraIT


include $(BUILD_EXECUTABLE)

# Add Camera IT to UNISOC Test Suite (UNITS)
include $(CLEAR_VARS)
LOCAL_MODULE := CameraIT.config
LOCAL_MODULE_CLASS := DATA
LOCAL_MODULE_TAGS := optional
LOCAL_PREBUILT_MODULE_FILE := $(LOCAL_PATH)/CameraIT.xml
LOCAL_MODULE_PATH := $(TARGET_OUT_TESTCASES)/CameraIT
LOCAL_COMPATIBILITY_SUITE := units
include $(BUILD_PREBUILT)
