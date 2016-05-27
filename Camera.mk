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


ifeq ($(strip $(TARGET_GPU_PLATFORM)),midgard)
LOCAL_C_INCLUDES += vendor/sprd/proprietories-source/libgpu/gralloc/midgard
else ifeq ($(strip $(TARGET_GPU_PLATFORM)),utgard)
LOCAL_C_INCLUDES += vendor/sprd/proprietories-source/libgpu/gralloc/utgard
else
LOCAL_C_INCLUDES += hardware/libhardware/modules/gralloc/
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_DIR)),3)
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/isp3.0/dummy \
	$(LOCAL_PATH)/isp3.0/common/inc \
	$(LOCAL_PATH)/isp3.0/middleware/inc
endif

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/oem2v0/inc \
    $(LOCAL_PATH)/oem2v0/isp_calibration/inc

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
                $(LOCAL_PATH)/arithmetic/facebeauty/inc

ifeq ($(strip $(TARGET_BOARD_CAMERA_FACE_DETECT)),true)
ifeq ($(strip $(TARGET_BOARD_CAMERA_FD_LIB)),omron)
	LOCAL_C_INCLUDES += \
			    $(LOCAL_PATH)/arithmetic/omron/inc
endif
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_EIS)),true)
	LOCAL_C_INCLUDES += \
			$(LOCAL_PATH)/arithmetic/eis/inc
endif

ifeq ($(strip $(TARGET_BOARD_CONFIG_CAMERA_RE_FOCUS)),true)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/sensor/al3200
endif


ifeq ($(TARGET_BOARD_CAMERA_HAL_VERSION), $(filter $(TARGET_BOARD_CAMERA_HAL_VERSION), HAL1.0 hal1.0 1.0))
LOCAL_SRC_FILES += hal1.0/src/SprdCameraHardwareInterface.cpp
else
LOCAL_SRC_FILES+= \
	hal3/SprdCamera3Factory.cpp \
	hal3/SprdCamera3Hal.cpp \
	hal3/SprdCamera3HWI.cpp \
	hal3/SprdCamera3Channel.cpp \
	hal3/SprdCamera3Mem.cpp \
	hal3/SprdCamera3OEMIf.cpp \
	hal3/SprdCamera3Setting.cpp \
	hal3/SprdCamera3Stream.cpp \
#	hal1.0/src/SprdCameraHardwareInterface.cpp
endif

LOCAL_CFLAGS += -fno-strict-aliasing -D_VSP_ -DJPEG_ENC -D_VSP_LINUX_ -DCHIP_ENDIAN_LITTLE -Wno-unused-parameter

include $(LOCAL_PATH)/SprdCtrl.mk

include $(LOCAL_PATH)/SprdLib.mk

LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_MODULE := camera.$(TARGET_BOARD_PLATFORM)

LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)
