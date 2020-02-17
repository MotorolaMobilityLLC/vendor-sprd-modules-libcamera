LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

ANDROID_MAJOR_VER := $(word 1, $(subst ., , $(PLATFORM_VERSION)))

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_VERSION)),2.3)
HAL_DIR := hal3_2v6
OEM_DIR := oem2v6
ISPALG_DIR := ispalg/isp2.x
ISPDRV_DIR := camdrv/isp2.3
else ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_VERSION)),2.4)
HAL_DIR := hal3_2v4
OEM_DIR := oem2v4
ISPALG_DIR := ispalg/isp2.x
ISPDRV_DIR := camdrv/isp2.4
else ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_VERSION)),2.5)
HAL_DIR := hal3_2v6
OEM_DIR := oem2v6
ISPALG_DIR := ispalg/isp2.x
ISPDRV_DIR := camdrv/isp2.6
else ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_VERSION)),2.6)
HAL_DIR := hal3_2v6
OEM_DIR := oem2v6
ISPALG_DIR := ispalg/isp2.x
ISPDRV_DIR := camdrv/isp2.6
else ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_VERSION)),2.7)
HAL_DIR := hal3_2v6
OEM_DIR := oem2v6
ISPALG_DIR := ispalg/isp3.x
ISPDRV_DIR := camdrv/isp2.6
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_CPP_MODULAR_KERNEL)),lite_r5p0)
CPP_DIR:=cpp/lite_r5p0
else ifeq ($(strip $(TARGET_BOARD_CAMERA_CPP_MODULAR_KERNEL)),lite_r6p0)
CPP_DIR:=cpp/lite_r6p0
else
$(warning "Old CPP version")
endif

LOCAL_CFLAGS += -fno-strict-aliasing -D_VSP_ -DJPEG_ENC -D_VSP_LINUX_ -DCHIP_ENDIAN_LITTLE -Wno-unused-parameter -Werror -Wno-error=format -DXMP_UNIXBuild -DUNIX_ENV -fexceptions

ifeq (1, $(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
LOCAL_CFLAGS += -DCONFIG_SPRD_ANDROID_8
endif

include $(LOCAL_PATH)/SprdCtrl.mk

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/vsp/inc \
    $(LOCAL_PATH)/vsp/src \
    $(LOCAL_PATH)/sensor/inc \
    $(LOCAL_PATH)/sensor \
    $(LOCAL_PATH)/jpeg \
    $(LOCAL_PATH)/common/inc \
    $(LOCAL_PATH)/hal1.0/inc \
    $(LOCAL_PATH)/$(HAL_DIR)/inc \
    $(LOCAL_PATH)/$(HAL_DIR)/ \
    $(LOCAL_PATH)/tool/mtrace \
    $(LOCAL_PATH) \
    $(LOCAL_PATH)/hal_common/multiCamera \
    $(TOP)/external/skia/include/images \
    $(TOP)/external/skia/include/core\
    $(TOP)/external/jhead \
    $(TOP)/external/sqlite/dist \
    $(TOP)/external/libyuv/files/include \
    $(TOP)/external/libyuv/files/include/libyuv \
    $(TOP)/system/media/camera/include \
    $(TOP)/vendor/sprd/external/kernel-headers \
    $(TOP)/vendor/sprd/external/drivers/gpu \
    $(TOP)/vendor/sprd/modules/libmemion \
    $(TOP)/frameworks/native/libs/sensor/include \
    $(TOP)/hardware/interfaces/camera/common/1.0/default/include \
    $(TOP)/system/memory/libion/kernel-headers \
    $(TARGET_BSP_UAPI_PATH)/kernel/usr/include/video \
    $(LOCAL_PATH)/kernel_module/interface

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/$(ISPDRV_DIR)/isp_tune \
    $(LOCAL_PATH)/$(ISPALG_DIR)/common/inc \
    $(LOCAL_PATH)/$(ISPDRV_DIR)/middleware/inc \
    $(LOCAL_PATH)/$(ISPDRV_DIR)/driver/inc

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/$(OEM_DIR)/inc \
    $(LOCAL_PATH)/oemcommon/inc

ifeq ($(strip $(CONFIG_CAMERA_MM_DVFS_SUPPORT)),true)
LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/oemcommon/mm_dvfs
endif

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/arithmetic/inc \
    $(LOCAL_PATH)/arithmetic/facebeauty/inc \
    $(LOCAL_PATH)/arithmetic/sprdface/inc \
    $(LOCAL_PATH)/arithmetic/depth/inc \
    $(LOCAL_PATH)/arithmetic/bokeh/inc \
    $(LOCAL_PATH)/arithmetic/depth_bokeh/inc\
    $(LOCAL_PATH)/arithmetic/sprd_yuvprocess/inc\
    $(LOCAL_PATH)/arithmetic/sprd_scale/inc\
    $(LOCAL_PATH)/arithmetic/sprd_warp/inc \
    $(LOCAL_PATH)/arithmetic/libxmp/inc \
    $(LOCAL_PATH)/arithmetic/libxmp/inc/client-glue

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/arithmetic/OpticsZoom/inc

# for bbat
LOCAL_C_INCLUDES += \
   $(TOP)/vendor/sprd/proprietories-source/engpc/sprd_fts_inc \
   $(TOP)/vendor/sprd/proprietories-source/autotest/interface/include

ifeq ($(strip $(TARGET_CAMERA_OIS_FUNC)),true)
LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/sensor/ois
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_EIS)),true)
LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/arithmetic/eis/inc
endif

LOCAL_C_INCLUDES += $(GPU_GRALLOC_INCLUDES)
ifeq ($(strip $(TARGET_GPU_PLATFORM)),soft)
LOCAL_C_INCLUDES += $(GPU_GRALLOC_INCLUDES)/soft/include
endif

#LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_BSP_UAPI_PATH)/kernel/usr

LOCAL_HEADER_LIBRARIES += media_plugin_headers
LOCAL_HEADER_LIBRARIES += libutils_headers
LOCAL_HEADER_LIBRARIES += jni_headers

LOCAL_SHARED_LIBRARIES := liblog libxml2

ifeq (1, $(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
LOCAL_SHARED_LIBRARIES += libsensorndkbridge
endif

ifeq ($(strip $(CONFIG_HAS_CAMERA_HINTS_VERSION)),901)
LOCAL_SHARED_LIBRARIES += libpowerhal_cli
endif

include $(LOCAL_PATH)/SprdLib.mk

CAMERA_CFLAGS := $(LOCAL_CFLAGS)
CAMERA_C_INCLUDES := $(LOCAL_C_INCLUDES)
CAMERA_HEADER_LIBRARIES := $(LOCAL_HEADER_LIBRARIES)
CAMERA_STATIC_LIBRARIES := $(LOCAL_STATIC_LIBRARIES)
CAMERA_SHARED_LIBRARIES := $(LOCAL_SHARED_LIBRARIES)









# camera so
include $(CLEAR_VARS)

LOCAL_CFLAGS := $(CAMERA_CFLAGS)

LOCAL_C_INCLUDES := $(CAMERA_C_INCLUDES)

LOCAL_HEADER_LIBRARIES := $(CAMERA_HEADER_LIBRARIES)

LOCAL_STATIC_LIBRARIES := $(CAMERA_STATIC_LIBRARIES)

LOCAL_SHARED_LIBRARIES := $(CAMERA_SHARED_LIBRARIES)

# TBD: will remove hal1.0/src/SprdCameraParameters.cpp for hal3
ifeq ($(TARGET_BOARD_CAMERA_HAL_VERSION), $(filter $(TARGET_BOARD_CAMERA_HAL_VERSION), HAL1.0 hal1.0 1.0))
LOCAL_SRC_FILES := \
    hal1.0/src/SprdCameraHardwareInterface.cpp \
    hal1.0/src/SprdCameraFlash.cpp \
    hal1.0/src/SprdCameraParameters.cpp
else
LOCAL_SRC_FILES := \
    $(HAL_DIR)/SprdCamera3HWI.cpp \
    $(HAL_DIR)/SprdCamera3Channel.cpp \
    $(HAL_DIR)/SprdCamera3Mem.cpp \
    $(HAL_DIR)/SprdCamera3OEMIf.cpp \
    $(HAL_DIR)/SprdCamera3Setting.cpp \
    $(HAL_DIR)/SprdCamera3Stream.cpp \
    $(HAL_DIR)/SprdCamera3Flash.cpp  \
    hal_common/multiCamera/SprdCamera3Wrapper.cpp \
    hal_common/multiCamera/SprdCamera3MultiBase.cpp \
    hal_common/camera_power_perf/SprdCameraPowerPerformance.cpp \
    hal1.0/src/SprdCameraParameters.cpp

# for multi-camera
ifeq ($(strip $(TARGET_BOARD_STEREOVIDEO_SUPPORT)),true)
LOCAL_SRC_FILES+= \
    hal_common/multiCamera/SprdCamera3StereoVideo.cpp
endif
ifeq ($(strip $(TARGET_BOARD_STEREOPREVIEW_SUPPORT)),true)
LOCAL_SRC_FILES+= \
    hal_common/multiCamera/SprdCamera3StereoPreview.cpp
endif
ifeq ($(strip $(TARGET_BOARD_STEREOCAPTURE_SUPPORT)),true)
LOCAL_SRC_FILES+= \
    hal_common/multiCamera/SprdCamera3Capture.cpp
endif
ifeq ($(strip $(TARGET_BOARD_BLUR_MODE_SUPPORT)),true)
LOCAL_SRC_FILES+= \
    hal_common/multiCamera/SprdCamera3Blur.cpp
endif
ifeq ($(strip $(TARGET_BOARD_COVERED_SENSOR_SUPPORT)),true)
LOCAL_SRC_FILES+= \
    hal_common/multiCamera/SprdCamera3PageTurn.cpp  \
    hal_common/multiCamera/SprdCamera3SelfShot.cpp
endif
ifeq ($(strip $(TARGET_BOARD_BOKEH_MODE_SUPPORT)),true)
LOCAL_SRC_FILES+= \
    hal_common/multiCamera/SprdCamera3RealBokeh.cpp \
    hal_common/multiCamera/SprdBokehAlgo.cpp
endif

ifeq ($(strip $(TARGET_BOARD_BOKEH_MODE_SUPPORT)),sbs)
LOCAL_SRC_FILES+= \
    hal_common/multiCamera/SprdCamera3SidebySideCamera.cpp
endif

ifeq ($(strip $(TARGET_BOARD_FACE_UNLOCK_SUPPORT)),true)
LOCAL_SRC_FILES+= \
    hal_common/multiCamera/SprdCamera3SingleFaceIdRegister.cpp \
    hal_common/multiCamera/SprdCamera3SingleFaceIdUnlock.cpp
endif
ifeq ($(strip $(TARGET_BOARD_DUAL_FACE_UNLOCK_SUPPORT)),true)
LOCAL_SRC_FILES+= \
    hal_common/multiCamera/SprdCamera3DualFaceId.cpp
endif
ifeq ($(strip $(TARGET_BOARD_PORTRAIT_SUPPORT)),true)
LOCAL_SRC_FILES+= \
    hal_common/multiCamera/SprdCamera3Portrait.cpp \
    hal_common/multiCamera/SprdPortraitAlgo.cpp
endif
ifeq ($(strip $(TARGET_BOARD_PORTRAIT_SINGLE_SUPPORT)),true)
LOCAL_SRC_FILES+= \
    hal_common/multiCamera/SprdCamera3SinglePortrait.cpp
endif

ifeq ($(strip $(TARGET_BOARD_3D_FACE_UNLOCK_SUPPORT)),true)
LOCAL_SRC_FILES+= \
    hal_common/multiCamera/SprdCamera33DFaceId.cpp
endif

# TBD: just for hal3_2v1 now, will add this for all chips later
ifeq ($(HAL_DIR), $(filter $(HAL_DIR), hal3_2v1 hal3_2v4 hal3_2v6))
LOCAL_SRC_FILES+= \
    hal_common/multiCamera/SprdDualCamera3Tuning.cpp
endif
LOCAL_SRC_FILES+= \
    hal_common/multiCamera/SprdCamera3Multi.cpp
ifeq ($(strip $(TARGET_BOARD_OPTICSZOOM_SUPPORT)),true)
LOCAL_SRC_FILES+= \
    hal_common/multiCamera/SprdCamera3OpticsZoomV1.cpp
endif
ifeq ($(strip $(TARGET_BOARD_3DFACE_SUPPORT)),true)
LOCAL_SRC_FILES+= \
    hal_common/multiCamera/SprdCamera33dFace.cpp
endif
ifeq ($(strip $(TARGET_BOARD_MULTICAMERA_SUPPORT)),true)
LOCAL_SRC_FILES+= \
    hal_common/multiCamera/SprdCamera3MultiCamera.cpp
endif

LOCAL_SRC_FILES += \
    arithmetic/sprd_yuvprocess/src/hal_yuvprocess.c

ifeq ($(strip $(TARGET_BOARD_DEL_CPP)),)
LOCAL_SRC_FILES += \
    test.cpp
endif

LOCAL_SRC_FILES += \
    $(HAL_DIR)/SprdCamera3Factory.cpp \
    $(HAL_DIR)/SprdCamera3Hal.cpp

endif

LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_MODULE := camera.$(TARGET_BOARD_PLATFORM)
LOCAL_MODULE_TAGS := optional

ifeq (1, $(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
LOCAL_PROPRIETARY_MODULE := true
endif
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

#LOCAL_SRC_FILES = tool/baseband_autotester_camera/test_camera.cpp
LOCAL_C_INCLUDES := \
    $(TOP)/system/core/libutils/include/ \
    $(TOP)/vendor/sprd/proprietories-source/engpc/sprd_fts_inc \
    $(TOP)/vendor/sprd/proprietories-source/autotest/interface/include \
    $(TOP)/frameworks/native/headers/media_plugin \
    $(TOP)/vendor/sprd/modules/libcamera/common/inc/ \
    $(LOCAL_PATH)/vsp/inc \
    $(LOCAL_PATH)/vsp/src \
    $(LOCAL_PATH)/sensor/inc \
    $(LOCAL_PATH)/sensor \
    $(LOCAL_PATH)/jpeg \
    $(LOCAL_PATH)/common/inc \
    $(LOCAL_PATH)/hal1.0/inc \
    $(LOCAL_PATH)/$(HAL_DIR)/inc \
    $(LOCAL_PATH)/$(HAL_DIR)/ \
    $(LOCAL_PATH)/tool/mtrace \
    $(TOP)/external/skia/include/images \
    $(TOP)/external/skia/include/core\
    $(TOP)/external/jhead \
    $(TOP)/external/sqlite/dist \
    $(TOP)/external/libyuv/files/include \
    $(TOP)/external/libyuv/files/include/libyuv \
    $(TOP)/system/media/camera/include \
    $(TOP)/vendor/sprd/external/kernel-headers \
    $(TOP)/vendor/sprd/external/drivers/gpu \
    $(TOP)/vendor/sprd/modules/libmemion \
    $(TOP)/frameworks/native/libs/sensor/include \
    $(TOP)/hardware/interfaces/camera/common/1.0/default/include \
    $(TOP)/system/memory/libion/kernel-headers \
    $(TARGET_BSP_UAPI_PATH)/kernel/usr/include/video \
    $(LOCAL_PATH)/kernel_module/interface

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/$(OEM_DIR)/inc \
    $(LOCAL_PATH)/oemcommon/inc

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/$(ISPDRV_DIR)/isp_tune \
    $(LOCAL_PATH)/$(ISPALG_DIR)/common/inc \
    $(LOCAL_PATH)/$(ISPDRV_DIR)/middleware/inc \
    $(LOCAL_PATH)/$(ISPDRV_DIR)/driver/inc

#$(error ${LOCAL_C_INCLUDES})

LOCAL_MODULE := libcamcalitest
LOCAL_MODULE_TAGS := optional
ifeq (1, $(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
LOCAL_PROPRIETARY_MODULE := true
endif

LOCAL_SHARED_LIBRARIES:= \
    liblog \
    libcutils  \
    libhardware \
    libutils libmemion libcamcommon

# for bbat test
CAMERA_NPI_FILE := /vendor/lib/libcamcalitest.so
SYMLINK := $(TARGET_OUT_VENDOR)/lib/npidevice/libcamcalitest.so
LOCAL_POST_INSTALL_CMD := $(hide) \
	mkdir -p $(TARGET_OUT_VENDOR)/lib/npidevice; \
	rm -rf $(SYMLINK) ;\
	ln -sf $(CAMERA_NPI_FILE) $(SYMLINK);

include $(BUILD_SHARED_LIBRARY)

