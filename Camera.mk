LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

TARGET_BOARD_CAMERA_READOTP_METHOD?=0

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/vsp/inc \
	$(LOCAL_PATH)/vsp/src \
	$(LOCAL_PATH)/sensor \
	$(LOCAL_PATH)/jpeg/inc \
	$(LOCAL_PATH)/jpeg/src \
	$(LOCAL_PATH)/common/inc \
	$(LOCAL_PATH)/hal1.0/inc \
	$(LOCAL_PATH)/tool/auto_test/inc \
	$(LOCAL_PATH)/tool/mtrace \
	external/skia/include/images \
	external/skia/include/core\
	external/jhead \
	external/sqlite/dist \
	system/media/camera/include \
	$(TOP)vendor/sprd/modules/libmemion \
	$(TOP)/vendor/sprd/external/kernel-headers \
	$(TOP)/vendor/sprd/modules/libmemion \
	$(TARGET_OUT_INTERMEDIATES)/KERNEL/usr/include/video

LOCAL_C_INCLUDES += $(GPU_GRALLOC_INCLUDES)
LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_OUT_INTERMEDIATES)/KERNEL/usr


ifeq ($(strip $(TARGET_GPU_PLATFORM)),midgard)
LOCAL_C_INCLUDES += vendor/sprd/proprietories-source/libgpu/gralloc/midgard
else ifeq ($(strip $(TARGET_GPU_PLATFORM)),utgard)
LOCAL_C_INCLUDES += vendor/sprd/proprietories-source/libgpu/gralloc/utgard
else ifeq ($(strip $(TARGET_GPU_PLATFORM)),rogue)
LOCAL_C_INCLUDES += vendor/sprd/proprietories-source/libgpu/gralloc/
LOCAL_C_INCLUDES += hardware/libhardware/modules/gralloc/
else
LOCAL_C_INCLUDES += hardware/libhardware/modules/gralloc/
endif

ISP_HW_VER = 3v0

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_DIR)),2.1)
ISP_HW_VER = 2v1
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/isp2.1/isp_tune \
	$(LOCAL_PATH)/isp2.1/common/inc \
	$(LOCAL_PATH)/isp2.1/middleware/inc
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_DIR)),2.2)
ISP_HW_VER = 2v1
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/isp2.2/isp_tune \
	$(LOCAL_PATH)/isp2.2/common/inc \
	$(LOCAL_PATH)/isp2.2/middleware/inc
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_DIR)),3)
ISP_HW_VER = 3v0
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/isp3.0/dummy \
	$(LOCAL_PATH)/isp3.0/common/inc \
	$(LOCAL_PATH)/isp3.0/middleware/inc
endif

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/oem$(ISP_HW_VER)/inc \
    $(LOCAL_PATH)/oem$(ISP_HW_VER)/isp_calibration/inc

LOCAL_SRC_FILES+= \
	hal1.0/src/SprdCameraParameters.cpp \
	common/src/cmr_msg.c \
	tool/mtrace/mtrace.c \
	tool/auto_test/src/SprdCamera_autest_Interface.cpp

LOCAL_SRC_FILES += test.cpp

ifeq ($(strip $(TARGET_CAMERA_OIS_FUNC)),true)
	LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/sensor/ois
endif

LOCAL_C_INCLUDES += \
                $(LOCAL_PATH)/arithmetic/inc \
                $(LOCAL_PATH)/arithmetic/facebeauty/inc \
                $(LOCAL_PATH)/arithmetic/sprdface/inc

ifeq ($(strip $(TARGET_BOARD_CAMERA_EIS)),true)
	LOCAL_C_INCLUDES += \
			$(LOCAL_PATH)/arithmetic/eis/inc
endif

ifeq ($(strip $(TARGET_BOARD_CONFIG_CAMERA_RT_REFOCUS)),true)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/sensor/al3200
endif


ifeq ($(TARGET_BOARD_CAMERA_HAL_VERSION), $(filter $(TARGET_BOARD_CAMERA_HAL_VERSION), HAL1.0 hal1.0 1.0))
LOCAL_SRC_FILES += hal1.0/src/SprdCameraHardwareInterface.cpp
LOCAL_SRC_FILES += hal1.0/src/SprdCameraFlash.cpp
else
LOCAL_SRC_FILES+= \
	hal3_$(ISP_HW_VER)/SprdCamera3Factory.cpp \
	hal3_$(ISP_HW_VER)/SprdCamera3Hal.cpp \
	hal3_$(ISP_HW_VER)/SprdCamera3HWI.cpp \
	hal3_$(ISP_HW_VER)/SprdCamera3Channel.cpp \
	hal3_$(ISP_HW_VER)/SprdCamera3Mem.cpp \
	hal3_$(ISP_HW_VER)/SprdCamera3OEMIf.cpp \
	hal3_$(ISP_HW_VER)/SprdCamera3Setting.cpp \
	hal3_$(ISP_HW_VER)/SprdCamera3Stream.cpp \
	hal3_$(ISP_HW_VER)/SprdCamera3Flash.cpp \
	hal3_$(ISP_HW_VER)/multiCamera/SprdCamera3Wrapper.cpp \
	hal3_$(ISP_HW_VER)/multiCamera/SprdCamera3MultiBase.cpp

ifeq ($(strip $(TARGET_BOARD_RANGEFINDER_SUPPORT)),true)
LOCAL_SRC_FILES+= \
	hal3_$(ISP_HW_VER)/multiCamera/SprdCamera3RangeFinder.cpp
endif

ifeq ($(strip $(TARGET_BOARD_STEREOVIDEO_SUPPORT)),true)
LOCAL_SRC_FILES+= \
	hal3_$(ISP_HW_VER)/multiCamera/SprdCamera3StereoVideo.cpp
endif

ifeq ($(strip $(TARGET_BOARD_STEREOPREVIEW_SUPPORT)),true)
LOCAL_SRC_FILES+= \
	hal3_$(ISP_HW_VER)/multiCamera/SprdCamera3StereoPreview.cpp
endif

ifeq ($(strip $(TARGET_BOARD_STEREOCAPTURE_SUPPORT)),true)
LOCAL_SRC_FILES+= \
	hal3_$(ISP_HW_VER)/multiCamera/SprdCamera3Capture.cpp
endif
#	hal1.0/src/SprdCameraHardwareInterface.cpp \
#	hal1.0/src/SprdCameraFlash.cpp

ifeq ($(strip $(TARGET_BOARD_BLUR_MODE_SUPPORT)),true)
LOCAL_SRC_FILES+= \
    hal3_$(ISP_HW_VER)/multiCamera/SprdCamera3Blur.cpp
endif

ifeq ($(strip $(TARGET_BOARD_COVERED_SENSOR_SUPPORT)),true)
LOCAL_SRC_FILES+= \
    hal3_$(ISP_HW_VER)/multiCamera/SprdCamera3PageTurn.cpp  \
    hal3_$(ISP_HW_VER)/multiCamera/SprdCamera3SelfShot.cpp
endif

endif

LOCAL_CFLAGS += -fno-strict-aliasing -D_VSP_ -DJPEG_ENC -D_VSP_LINUX_ -DCHIP_ENDIAN_LITTLE -Wno-unused-parameter -Werror -Wno-error=format

include $(LOCAL_PATH)/SprdCtrl.mk

include $(LOCAL_PATH)/SprdLib.mk

LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_MODULE := camera.$(TARGET_BOARD_PLATFORM)

LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)
