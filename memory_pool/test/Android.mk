LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := BufferPoolTest.cpp

#LOCAL_SRC_FILES +=	/../BufferPool.cpp

LOCAL_SRC_FILES +=	/../../oem2v6/src/cmr_oem.c

#LOCAL_CFLAGS += -fno-strict-aliasing -Wno-unused-parameter -Werror -Wno-error=format
#LOCAL_LDFLAGS += -ldl

LOCAL_C_INCLUDES := $(TOP)/system/core/libutils/include/

LOCAL_C_INCLUDES := $(TOP)/system/core/libutils/

LOCAL_C_INCLUDES += $(TOP)/system/core/libsystem/include/

LOCAL_C_INCLUDES += $(TOP)/external/libxml2/include/

LOCAL_C_INCLUDES += $(TOP)/hardware/libhardware/include/

LOCAL_PROPRIETARY_MODULE := true

LOCAL_32_BIT_ONLY := true

LOCAL_C_INCLUDES +=	$(LOCAL_PATH)/../../oem2v6/inc

LOCAL_C_INCLUDES +=	$(LOCAL_PATH)/../../oem2v6/src

LOCAL_C_INCLUDES +=	$(LOCAL_PATH)/../ \
	$(LOCAL_PATH)/../../common/inc/

LOCAL_C_INCLUDES += \
    $(TARGET_BSP_UAPI_PATH)/kernel/usr/include/video \
    $(TOP)/vendor/sprd/modules/enhance/include \
    $(LOCAL_PATH)/inc \
    $(LOCAL_PATH)/../../common/inc \
    $(LOCAL_PATH)/../../jpeg \
    $(LOCAL_PATH)/../../vsp/inc \
    $(LOCAL_PATH)/../../tool/mtrace \
    $(LOCAL_PATH)/../../arithmetic/facebeauty/inc \
    $(LOCAL_PATH)/../../sensor/inc \
    $(LOCAL_PATH)/../../sensor/dummy \
    $(LOCAL_PATH)/../../sensor/af_drv \
    $(LOCAL_PATH)/../../sensor/otp_drv \
    $(LOCAL_PATH)/../../arithmetic/inc \
    $(LOCAL_PATH)/../../cpp/lite_r6p0/driver/inc \
    $(LOCAL_PATH)/../../cpp/lite_r6p0/algo/inc \
    $(LOCAL_PATH)/../../arithmetic/sprd_easy_hdr/inc \
    $(LOCAL_PATH)/../../kernel_module/interface

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/../../ispalg/common/inc \
    $(LOCAL_PATH)/../../camdrv/isp2.6/isp_tune \
    $(LOCAL_PATH)/../../camdrv/isp2.6/middleware/inc \
    $(LOCAL_PATH)/../../camdrv/isp2.6/driver/inc

LOCAL_SHARED_LIBRARIES := liblog libcamoem libutils

LOCAL_MODULE := mempoolTest

include $(BUILD_EXECUTABLE)
