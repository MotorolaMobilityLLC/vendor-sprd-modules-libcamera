LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS += -fno-strict-aliasing -Wno-unused-parameter -Werror -Wno-error=format
LOCAL_LDFLAGS += -ldl

TARGET_BOARD_CAMERA_READOTP_METHOD?=0

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../sensor/inc

ifeq ($(filter $(strip $(TARGET_BOARD_PLATFORM)),$(strip $(PLATFORM_VERSION_FILTER))),)
ifeq ($(strip $(TARGET_BOARD_CAMERA_OEM_DIR)),2v1)
ISPALG_DIR = ispalg/isp2.x
ISPDRV_DIR = camdrv/isp$(TARGET_BOARD_CAMERA_ISP_DIR)
LOCAL_C_INCLUDES += \
	$(TARGET_OUT_INTERMEDIATES)/KERNEL/usr/include/video \
	$(LOCAL_PATH)/inc \
	$(LOCAL_PATH)/isp_calibration/inc \
	$(LOCAL_PATH)/../common/inc \
	$(LOCAL_PATH)/../jpeg \
	$(LOCAL_PATH)/../vsp/inc \
	$(LOCAL_PATH)/../tool/mtrace \
	$(LOCAL_PATH)/../arithmetic/facebeauty/inc \
	$(LOCAL_PATH)/../sensor/dummy \
	$(LOCAL_PATH)/../sensor/af_drv \
	$(LOCAL_PATH)/../sensor/otp_drv

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../$(ISPALG_DIR)/common/inc \
	$(LOCAL_PATH)/../$(ISPDRV_DIR)/isp_tune \
	$(LOCAL_PATH)/../$(ISPDRV_DIR)/middleware/inc \
	$(LOCAL_PATH)/../$(ISPDRV_DIR)/driver/inc


LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_OUT_INTERMEDIATES)/KERNEL/usr

LOCAL_SRC_FILES+= \
	src/SprdOEMCamera.c \
	src/cmr_common.c \
	src/cmr_oem.c \
	src/cmr_setting.c \
	src/cmr_sensor.c \
	src/cmr_mem.c \
	src/cmr_scale.c \
	src/cmr_rotate.c \
	src/cmr_grab.c \
	src/cmr_jpeg.c \
	src/cmr_exif.c \
	src/cmr_preview.c \
	src/cmr_snapshot.c \
	src/cmr_ipm.c \
	src/cmr_focus.c \
	src/cmr_img_debug.c \
	src/cmr_filter.c \
	src/exif_writer.c \
        src/jpeg_stream.c

ifeq ($(strip $(TARGET_BOARD_CAMERA_FACE_DETECT)),true)
	LOCAL_C_INCLUDES += \
		$(LOCAL_PATH)/../arithmetic/sprdface/inc
	LOCAL_SRC_FILES+= src/cmr_fd_sprd.c
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_EIS)),true)
	LOCAL_C_INCLUDES += \
			$(LOCAL_PATH)/../arithmetic/eis/inc
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_Y_DENOISE)),true)
	LOCAL_C_INCLUDES += $(LOCAL_PATH)/inc/ydenoise_paten
	LOCAL_SRC_FILES+= src/cmr_ydenoise.c
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_HDR_CAPTURE)),true)
	LOCAL_SRC_FILES+= src/cmr_hdr.c
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_UV_DENOISE)),true)
	LOCAL_SRC_FILES+= src/cmr_uvdenoise.c
endif

ifeq ($(strip $(TARGET_BOARD_CONFIG_CAMERA_RT_REFOCUS)),true)
	LOCAL_C_INCLUDES += $(LOCAL_PATH)/sensor/al3200
	LOCAL_SRC_FILES+= src/cmr_refocus.c
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_3DNR_CAPTURE)),true)
ifeq ($(strip $(TARGET_BOARD_PLATFORM)),sp9832e)
	LOCAL_SRC_FILES+= src/cmr_3dnr_sw.c
	LOCAL_C_INCLUDES += $(LOCAL_PATH)/../arithmetic/lib3dnr/blacksesame/inc
	LOCAL_SHARED_LIBRARIES += libtdnsTest libui libEGL libGLESv2
else
	LOCAL_SRC_FILES+= src/cmr_3dnr.c
	LOCAL_C_INCLUDES += $(LOCAL_PATH)/../arithmetic/lib3dnr/sprd/inc
	LOCAL_SHARED_LIBRARIES += libsprd3dnr
endif
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_FILTER_VERSION)),0)
LOCAL_CFLAGS += -DCONFIG_CAMERA_FILTER
LOCAL_CFLAGS += -DCONFIG_FILTER_VERSION=0
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../arithmetic/sprd_filter/inc
LOCAL_SRC_FILES+= src/sprd_filter.c
LOCAL_SHARED_LIBRARIES += libSprdImageFilter
else ifeq ($(strip $(TARGET_BOARD_CAMERA_FILTER_VERSION)),1)
LOCAL_CFLAGS += -DCONFIG_CAMERA_FILTER
LOCAL_CFLAGS += -DCONFIG_FILTER_VERSION=1
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../arithmetic/filter/inc
LOCAL_SRC_FILES+= src/arcsoft_filter.c
LOCAL_SHARED_LIBRARIES += libamipengine
else
LOCAL_CFLAGS += -DCONFIG_FILTER_VERSION=0xFF
endif

LOCAL_CFLAGS += -D_VSP_LINUX_ -D_VSP_

include $(LOCAL_PATH)/../SprdCtrl.mk

LOCAL_MODULE := libcamoem

LOCAL_MODULE_TAGS := optional

LOCAL_SHARED_LIBRARIES += libutils libcutils libcamsensor libcamcommon
LOCAL_SHARED_LIBRARIES += libcamdrv

LOCAL_SHARED_LIBRARIES += liblog

ifeq ($(strip $(TARGET_BOARD_CAMERA_FACE_BEAUTY)),true)
       LOCAL_SHARED_LIBRARIES += libcamfb
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_FACE_DETECT)),true)
	LOCAL_STATIC_LIBRARIES += libsprdfa libsprdfar
	LOCAL_SHARED_LIBRARIES += libsprdfd
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_EIS)),true)
	LOCAL_SHARED_LIBRARIES += libgyrostab
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_GYRO)),true)
	LOCAL_SHARED_LIBRARIES +=libgui
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_HDR_CAPTURE)),true)
	LOCAL_CFLAGS += -DCONFIG_SPRD_HDR_LIB
	LOCAL_SHARED_LIBRARIES += libsprd_easy_hdr
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

ifeq (1, $(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_SHARED_LIBRARY)
endif

endif
