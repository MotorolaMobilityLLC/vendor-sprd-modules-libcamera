LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS += -fno-strict-aliasing -Wno-unused-parameter -Werror -Wno-error=format
LOCAL_LDFLAGS += -ldl

#sensor makefile config
SENSOR_FILE_COMPILER := $(CAMERA_SENSOR_TYPE_BACK)
SENSOR_FILE_COMPILER += $(CAMERA_SENSOR_TYPE_FRONT)
SENSOR_FILE_COMPILER += $(CAMERA_SENSOR_TYPE_BACK_EXT)
SENSOR_FILE_COMPILER += $(CAMERA_SENSOR_TYPE_FRONT_EXT)

SENSOR_FILE_COMPILER := $(shell echo $(SENSOR_FILE_COMPILER))
#$(warning $(SENSOR_FILE_COMPILER))

sensor_comma:=,
sensor_empty:=
sensor_space:=$(sensor_empty)

split_sensor:=$(sort $(subst $(sensor_comma),$(sensor_space) ,$(shell echo $(SENSOR_FILE_COMPILER))))
#$(warning $(split_sensor))

sensor_macro:=$(shell echo $(split_sensor) | tr a-z A-Z)
#$(warning $(sensor_macro))
$(foreach item,$(sensor_macro), $(eval LOCAL_CFLAGS += -D$(shell echo $(item))))

ifeq ($(strip $(OEM_DIR)),oem2v6)
LOCAL_C_INCLUDES += \
    $(TARGET_BSP_UAPI_PATH)/kernel/usr/include/video \
    $(TOP)/vendor/sprd/modules/enhance/include \
    $(LOCAL_PATH)/inc \
    $(LOCAL_PATH)/../performance \
    $(LOCAL_PATH)/../common/inc \
    $(LOCAL_PATH)/../oemcommon/mm_dvfs \
    $(LOCAL_PATH)/../jpeg \
    $(LOCAL_PATH)/../vsp/inc \
    $(LOCAL_PATH)/../tool/mtrace \
    $(LOCAL_PATH)/../arithmetic/facebeauty/inc \
    $(LOCAL_PATH)/../sensor/inc \
    $(LOCAL_PATH)/../sensor/dummy \
    $(LOCAL_PATH)/../sensor/af_drv \
    $(LOCAL_PATH)/../sensor/otp_drv \
    $(LOCAL_PATH)/../arithmetic/inc \
    $(LOCAL_PATH)/../$(CPP_DIR)/driver/inc \
    $(LOCAL_PATH)/../$(CPP_DIR)/algo/inc \
    $(LOCAL_PATH)/../arithmetic/sprd_easy_hdr/inc \
    $(LOCAL_PATH)/../arithmetic/sprd_warp/inc \
    $(LOCAL_PATH)/../arithmetic/sprdface/inc \
    $(LOCAL_PATH)/../arithmetic/4dtracking/inc \
    $(LOCAL_PATH)/../arithmetic/sprd_yuvprocess/inc \
    $(LOCAL_PATH)/../arithmetic/sprd_filter/inc \
    $(LOCAL_PATH)/../arithmetic/eis/inc \
    $(LOCAL_PATH)/../arithmetic/sprd_fdr/inc \
    $(LOCAL_PATH)/../arithmetic/libmfnr/blacksesame/inc \
    $(LOCAL_PATH)/../arithmetic/libdre/inc \
    $(LOCAL_PATH)/../arithmetic/libcnr/inc \
    $(LOCAL_PATH)/../arithmetic/libdrepro/inc \
    $(LOCAL_PATH)/../kernel_module/interface

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/../$(ISPALG_DIR)/common/inc \
    $(LOCAL_PATH)/../$(ISPDRV_DIR)/isp_tune \
    $(LOCAL_PATH)/../$(ISPDRV_DIR)/middleware/inc \
    $(LOCAL_PATH)/../$(ISPDRV_DIR)/driver/inc

LOCAL_SRC_FILES += \
    src/SprdOEMCamera.c \
    src/cmr_common.c \
    src/cmr_oem.c \
    src/cmr_setting.c \
    src/cmr_isptool.c \
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
    src/cmr_filter.c \
    src/cmr_img_debug.c \
    src/cmr_watermark.c \
    src/cmr_ai_scene.c \
    src/ips_core.c \
    src/exif_writer.c \
    src/jpeg_stream.c \
    src/cmr_4in1.c

LOCAL_SRC_FILES += ../arithmetic/sprd_yuvprocess/src/cmr_yuvprocess.c

LOCAL_SHARED_LIBRARIES += libcamperf
LOCAL_SHARED_LIBRARIES += libutils libcutils libcamsensor libcamcommon libhardware libxml2
LOCAL_SHARED_LIBRARIES += libcamdrv
LOCAL_SHARED_LIBRARIES += liblog
LOCAL_SHARED_LIBRARIES += libcppdrv
LOCAL_SHARED_LIBRARIES += libyuv
LOCAL_HEADER_LIBRARIES += liblog_headers
LOCAL_HEADER_LIBRARIES += jni_headers

ifeq ($(strip $(CONFIG_CAMERA_MM_DVFS_SUPPORT)),true)
LOCAL_SRC_FILES += ../oemcommon/mm_dvfs/cmr_mm_dvfs.c
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_AUTO_TRACKING)),true)
LOCAL_SRC_FILES += src/cmr_auto_tracking.c
LOCAL_SHARED_LIBRARIES += libSprdOTAlgo
endif

#include fdr files
ifeq ($(strip $(TARGET_BOARD_CAMERA_FDR_CAPTURE)),true)
LOCAL_SHARED_LIBRARIES += libsprdfdr
LOCAL_SHARED_LIBRARIES += libsprdfdradapter
endif


ifeq ($(strip $(TARGET_BOARD_CAMERA_3DNR_CAPTURE)),true)
LOCAL_SRC_FILES += src/cmr_3dnr_sw.c
LOCAL_SHARED_LIBRARIES += libmfnr libui libEGL libGLESv2
LOCAL_SHARED_LIBRARIES += libSprdMfnrAdapter
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_CNR_CAPTURE)),true)
LOCAL_SRC_FILES+= src/cmr_cnr.c
LOCAL_SHARED_LIBRARIES += libsprdcnr
LOCAL_SHARED_LIBRARIES += libsprdcnradapter
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_DRE_CAPTURE)),true)
LOCAL_SRC_FILES+= src/cmr_dre.c
LOCAL_SHARED_LIBRARIES += libsprddre
LOCAL_SHARED_LIBRARIES += libsprddreadapter
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_DRE_PRO_CAPTURE)),true)
LOCAL_SRC_FILES+= src/cmr_dre_pro.c
LOCAL_SHARED_LIBRARIES += libsprddrepro
LOCAL_SHARED_LIBRARIES += libsprddreproadapter
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_FILTER_VERSION)),0)
LOCAL_SRC_FILES+= src/sprd_filter.c
LOCAL_SHARED_LIBRARIES += libSprdImageFilter
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_FACE_BEAUTY)),true)
LOCAL_SHARED_LIBRARIES += libcamfb libcamfacebeauty
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_SUPPORT_ULTRA_WIDE)),true)
LOCAL_SRC_FILES += src/cmr_ultrawide.c
LOCAL_SHARED_LIBRARIES += libsprdwarp
LOCAL_SHARED_LIBRARIES += libsprdwarpadapter
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_FACE_DETECT)),true)
LOCAL_SRC_FILES += src/cmr_fd_sprd.c
LOCAL_SHARED_LIBRARIES += libsprdfa libsprdfd libsprdfd_hw
ifeq ($(strip $(TARGET_BOARD_SPRD_FD_VERSION)),1)
LOCAL_SHARED_LIBRARIES += libsprdfarcnn
else
LOCAL_SHARED_LIBRARIES += libsprdfar
endif
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_EIS)),true)
LOCAL_SHARED_LIBRARIES += libgyrostab
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_HDR_CAPTURE)),true)
LOCAL_SRC_FILES += src/cmr_hdr.c
ifeq ($(strip $(TARGET_BOARD_SPRD_HDR_VERSION)),2)
LOCAL_SHARED_LIBRARIES += libsprdhdradapter
else
LOCAL_SHARED_LIBRARIES += libsprd_easy_hdr
endif
endif

include $(LOCAL_PATH)/../SprdCtrl.mk
LOCAL_CFLAGS += -DCONFIG_LIBYUV
LOCAL_CFLAGS += -D_VSP_LINUX_ -D_VSP_

ifeq ($(strip $(TARGET_BOARD_CAMERA_ASAN_MEM_DETECT)),true)
LOCAL_SANITIZE := address
endif

LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE := libcamoem
LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)
endif
