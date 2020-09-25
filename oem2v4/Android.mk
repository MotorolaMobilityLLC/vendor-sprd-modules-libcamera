LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS += -fno-strict-aliasing -Wno-unused-parameter -Werror -Wno-error=format

#sensor makefile config
SENSOR_FILE_COMPILER := $(CAMERA_SENSOR_TYPE_BACK)
SENSOR_FILE_COMPILER += $(CAMERA_SENSOR_TYPE_FRONT)
SENSOR_FILE_COMPILER += $(CAMERA_SENSOR_TYPE_BACK_EXT)
SENSOR_FILE_COMPILER += $(CAMERA_SENSOR_TYPE_FRONT_EXT)

SENSOR_FILE_COMPILER := $(shell echo $(SENSOR_FILE_COMPILER))
#$(warning $(SENSOR_FILE_COMPILER))

sensor_comma := ,
sensor_empty :=
sensor_space := $(sensor_empty)

split_sensor := $(sort $(subst $(sensor_comma),$(sensor_space) ,$(shell echo $(SENSOR_FILE_COMPILER))))
#$(warning $(split_sensor))

sensor_macro:=$(shell echo $(split_sensor) | tr a-z A-Z)
#$(warning $(sensor_macro))
$(foreach item,$(sensor_macro), $(eval LOCAL_CFLAGS += -D$(shell echo $(item))))

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_VERSION)),2.4)
ISPALG_DIR = ispalg
ISPDRV_DIR = camdrv/isp2.4
LOCAL_C_INCLUDES := \
    $(TARGET_BSP_UAPI_PATH)/kernel/usr/include/video \
    $(LOCAL_PATH)/inc \
    $(LOCAL_PATH)/isp_calibration/inc \
    $(LOCAL_PATH)/../common/inc \
    $(LOCAL_PATH)/../oemcommon/inc \
    $(LOCAL_PATH)/../jpeg \
    $(LOCAL_PATH)/../vsp/inc \
    $(LOCAL_PATH)/../tool/mtrace \
    $(LOCAL_PATH)/../arithmetic/facebeauty/inc \
    $(LOCAL_PATH)/../sensor/dummy \
    $(LOCAL_PATH)/../sensor/af_drv \
    $(LOCAL_PATH)/../sensor/otp_drv \
    $(LOCAL_PATH)/../sensor/inc \
    $(LOCAL_PATH)/../arithmetic/inc

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/../$(ISPALG_DIR)/common/inc \
    $(LOCAL_PATH)/../$(ISPDRV_DIR)/isp_tune \
    $(LOCAL_PATH)/../$(ISPDRV_DIR)/middleware/inc \
    $(LOCAL_PATH)/../$(ISPDRV_DIR)/driver/inc

ifeq ($(strip $(TARGET_BOARD_CAMERA_MODULAR)),true)
LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/../kernel_module/interface
endif

LOCAL_HEADER_LIBRARIES += jni_headers

#LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_BSP_UAPI_PATH)/kernel/usr

LOCAL_SRC_FILES+= \
        ../oemcommon/src/cmr_img_debug.c \
        ../oemcommon/src/exif_writer.c \
        ../oemcommon/src/jpeg_stream.c \
        ../oemcommon/src/cmr_jpeg.c \
        ../oemcommon/src/cmr_exif.c \
        ../oemcommon/src/cmr_sensor.c \
        ../oemcommon/src/cmr_ipm.c \
        ../oemcommon/src/cmr_filter.c

LOCAL_SRC_FILES+= \
    src/SprdOEMCamera.c \
    src/cmr_common.c \
    src/cmr_oem.c \
    src/cmr_setting.c \
    src/cmr_mem.c \
    src/cmr_scale.c \
    src/cmr_rotate.c \
    src/cmr_grab.c \
    src/cmr_preview.c \
    src/cmr_snapshot.c \
    src/cmr_focus.c \
    src/cmr_isptool.c

ifeq ($(strip $(TARGET_BOARD_CAMERA_SUPPORT_ULTRA_WIDE)),true)
    LOCAL_SRC_FILES += ../oemcommon/src/cmr_ultrawide.c
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_FACE_DETECT)),true)
    LOCAL_C_INCLUDES += \
        $(LOCAL_PATH)/../arithmetic/sprdface/inc
    LOCAL_SRC_FILES += ../oemcommon/src/cmr_fd_sprd.c
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_EIS)),true)
    LOCAL_C_INCLUDES += $(LOCAL_PATH)/../arithmetic/eis/inc
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_Y_DENOISE)),true)
    LOCAL_C_INCLUDES += $(LOCAL_PATH)/inc/ydenoise_paten
    LOCAL_SRC_FILES += ../oemcommon/src/cmr_ydenoise.c
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_HDR_CAPTURE)),true)
    LOCAL_SRC_FILES += ../oemcommon/src/cmr_hdr.c
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_UV_DENOISE)),true)
    LOCAL_SRC_FILES += ../oemcommon/src/cmr_uvdenoise.c
endif

ifeq ($(strip $(TARGET_BOARD_CONFIG_CAMERA_RT_REFOCUS)),true)
    LOCAL_C_INCLUDES += $(LOCAL_PATH)/sensor/al3200
    LOCAL_SRC_FILES += ../oemcommon/src/cmr_refocus.c
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_3DNR_CAPTURE)),true)
ifeq ($(strip $(TARGET_BOARD_PLATFORM)),sp7731e)
    LOCAL_SRC_FILES += src/cmr_3dnr_sw.c
    LOCAL_C_INCLUDES += $(LOCAL_PATH)/../arithmetic/libmfnr/blacksesame/inc
    LOCAL_SHARED_LIBRARIES += libmfnr libui libEGL libGLESv2
    LOCAL_SHARED_LIBRARIES += libSprdMfnrAdapter
endif
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_CNR_CAPTURE)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_CNR
LOCAL_SRC_FILES+= src/cmr_cnr.c
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../arithmetic/libcnr/inc
LOCAL_SHARED_LIBRARIES += libsprdcnr
LOCAL_SHARED_LIBRARIES += libsprdcnradapter
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_FILTER_VERSION)),0)
    LOCAL_CFLAGS += -DCONFIG_CAMERA_FILTER
    LOCAL_CFLAGS += -DCONFIG_FILTER_VERSION=0
    LOCAL_C_INCLUDES += $(LOCAL_PATH)/../arithmetic/sprd_filter/inc
    LOCAL_SRC_FILES += ../oemcommon/src/sprd_filter.c
    LOCAL_SHARED_LIBRARIES += libSprdImageFilter
else
    LOCAL_CFLAGS += -DCONFIG_FILTER_VERSION=0xFF
endif

ifneq ($(filter $(strip $(PLATFORM_VERSION)),O 8.0.0 8.1.0 P 9),)
    LOCAL_C_INCLUDES += $(LOCAL_PATH)/../arithmetic/sprd_yuvprocess/inc
    LOCAL_SRC_FILES += ../arithmetic/sprd_yuvprocess/src/cmr_yuvprocess.c
    LOCAL_SHARED_LIBRARIES += libyuv
endif

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../performance
LOCAL_SHARED_LIBRARIES += libcamperf

LOCAL_CFLAGS += -D_VSP_LINUX_ -D_VSP_

include $(LOCAL_PATH)/../SprdCtrl.mk

LOCAL_MODULE := libcamoem
LOCAL_MODULE_TAGS := optional
ifeq ($(strip $(TARGET_BOARD_CAMERA_ASAN_MEM_DETECT)),true)
LOCAL_SANITIZE := address
endif

LOCAL_SHARED_LIBRARIES += libutils libcutils libcamsensor libcamcommon libhardware libxml2
LOCAL_SHARED_LIBRARIES += libcamdrv

LOCAL_SHARED_LIBRARIES += liblog

ifeq ($(strip $(TARGET_BOARD_CAMERA_FACE_BEAUTY)),true)
    LOCAL_SHARED_LIBRARIES += libcamfb
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_FACE_DETECT)),true)
    LOCAL_SHARED_LIBRARIES += libsprdfa libsprdfar
    LOCAL_SHARED_LIBRARIES += libsprdfd
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_EIS)),true)
    LOCAL_SHARED_LIBRARIES += libgyrostab
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_HDR_CAPTURE)),true)
        ifeq ($(strip $(TARGET_BOARD_SPRD_HDR_VERSION)),2)
        LOCAL_CFLAGS += -DCONFIG_SPRD_HDR_LIB_VERSION_2
        LOCAL_SHARED_LIBRARIES += libsprdhdr
else
        LOCAL_CFLAGS += -DCONFIG_SPRD_HDR_LIB
        LOCAL_SHARED_LIBRARIES += libsprd_easy_hdr
endif
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_UV_DENOISE)),true)
    LOCAL_SHARED_LIBRARIES += libuvdenoise
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_Y_DENOISE)),true)
    LOCAL_SHARED_LIBRARIES += libynoise
endif

ifeq ($(strip $(TARGET_BOARD_CONFIG_CAMERA_RT_REFOCUS)),true)
    LOCAL_SHARED_LIBRARIES += libalRnBLV
endif

ifeq (1, 1) #(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
    LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_SHARED_LIBRARY)

endif

