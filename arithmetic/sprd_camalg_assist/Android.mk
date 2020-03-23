LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := libsprdcamalgassist
LOCAL_SRC_FILES := sprd_camalg_assist_ionmem.cpp \
                   sprd_vdsp.cpp \
                   sprd_vdsp_ceva.cpp \
                   sprd_vdsp_cadence.cpp \
				   sprd_camalg_assist_binder.cpp

LOCAL_C_INCLUDES := vendor/sprd/modules/libmemion \
                    vendor/sprd/external/kernel-headers \
                    vendor/sprd/modules/vdsp/Cadence/xrp/vdsp-interface

LOCAL_SHARED_LIBRARIES := liblog libmemion libutils libbinder
LOCAL_CFLAGS := -fvisibility=hidden
LOCAL_PROPRIETARY_MODULE := true

ifneq ($(filter $(TARGET_BOARD_PLATFORM), ud710), )
LOCAL_CFLAGS += -DVDSP_CEVA
endif

ifneq ($(filter $(TARGET_BOARD_PLATFORM), ums512), )
LOCAL_CFLAGS += -DVDSP_CADENCE
LOCAL_SHARED_LIBRARIES += libvdspservice
endif

include $(BUILD_SHARED_LIBRARY)
