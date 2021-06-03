LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LIBCAMERA_DIR := $(TOP)/vendor/sprd/modules/libcamera

include $(LOCAL_PATH)/../SprdCtrl.mk
LOCAL_CFLAGS += -DANDROID_LOG
LOCAL_CFLAGS += -DCAMERA_CNR3_ENABLE

LOCAL_C_INCLUDES += $(LOCAL_PATH)/ \
    $(TOP)/system/core/include/ \
    $(TOP)/system/core/include/cutils/ \
    $(LIBCAMERA_DIR)/common/inc \
    $(LIBCAMERA_DIR)/arithmetic/inc \
    $(LIBCAMERA_DIR)/arithmetic/libdre/inc \
    $(LIBCAMERA_DIR)/arithmetic/libdrepro/inc \
    $(LIBCAMERA_DIR)/arithmetic/libcnr/inc \
    $(LIBCAMERA_DIR)/arithmetic/libmfnr/blacksesame/inc \
    $(LIBCAMERA_DIR)/arithmetic/sprdface/inc \
    $(LIBCAMERA_DIR)/arithmetic/facebeauty/inc \
    $(LIBCAMERA_DIR)/arithmetic/sprd_yuvprocess/inc \
    $(LIBCAMERA_DIR)/arithmetic/sprd_warp/inc \
    $(LIBCAMERA_DIR)/arithmetic/sprd_filter/inc \
    $(LIBCAMERA_DIR)/arithmetic/sprd_easy_hdr/inc \
    $(LIBCAMERA_DIR)/arithmetic/sprd_fdr/inc \
    $(LIBCAMERA_DIR)/arithmetic/sprd_ee/inc

LOCAL_HEADER_LIBRARIES += liblog_headers
LOCAL_HEADER_LIBRARIES += jni_headers

LOCAL_SRC_FILES := swa_core.c

LOCAL_SHARED_LIBRARIES += liblog

ifeq ($(strip $(TARGET_BOARD_CAMERA_FILTER_VERSION)),0)
LOCAL_SHARED_LIBRARIES += libSprdImageFilter
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_FACE_BEAUTY)),true)
LOCAL_SHARED_LIBRARIES += libcamfb libcamfacebeauty
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_HDR_CAPTURE)),true)
ifeq ($(strip $(TARGET_BOARD_SPRD_HDR_VERSION)),2)
LOCAL_SHARED_LIBRARIES += libsprdhdr libsprdhdradapter
endif
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_HDR_CAPTURE)),true)
ifeq ($(strip $(TARGET_BOARD_SPRD_HDR_VERSION)),3)
LOCAL_SHARED_LIBRARIES += libsprdhdr3 libsprdhdradapter
endif
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_CNR_CAPTURE)),true)
LOCAL_SHARED_LIBRARIES += libsprdcnr libsprdcnradapter
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_DRE_CAPTURE)),true)
LOCAL_SHARED_LIBRARIES += libsprddre libsprddreadapter
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_DRE_PRO_CAPTURE)),true)
LOCAL_SHARED_LIBRARIES += libsprddrepro libsprddreproadapter
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_3DNR_CAPTURE)),true)
LOCAL_SHARED_LIBRARIES += libSprdMfnrAdapter libmfnr libui libEGL libGLESv2
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_SUPPORT_ULTRA_WIDE)),true)
LOCAL_SHARED_LIBRARIES += libsprdwarp libsprdwarpadapter
endif

LOCAL_MODULE := libcam_ipmpro
LOCAL_MODULE_TAGS := optional
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_SHARED_LIBRARY)
