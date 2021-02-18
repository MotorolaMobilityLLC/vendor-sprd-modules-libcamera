LOCAL_PATH:= $(call my-dir)

ifneq ($(filter $(TARGET_BOARD_PLATFORM), ud710), )
NPU_SUPPORT := true
else
NPU_SUPPORT := false
endif

ifneq ($(filter $(TARGET_BOARD_PLATFORM), ums512), )
VDSP_SUPPORT := true
else
VDSP_SUPPORT := false
endif

TFLITE_SUPPORT := true
MNN_SUPPORT := true


ifeq ($(TARGET_ARCH), $(filter $(TARGET_ARCH), arm arm64))
LIB_PATH := lib
endif


ifeq ($(strip $(MNN_SUPPORT)), true)
include $(CLEAR_VARS)
LOCAL_VENDOR_MODULE := true
LOCAL_MODULE := libMNN
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := libMNN.so
LOCAL_MODULE_STEM_64 := libMNN.so
LOCAL_SRC_FILES_32 := libmnn/armeabi-v7a/libMNN.so
LOCAL_SRC_FILES_64 := libmnn/arm64-v8a/libMNN.so
LOCAL_CHECK_ELF_FILES := false
include $(BUILD_PREBUILT)
endif

ifeq ($(strip $(TFLITE_SUPPORT)), true)
include $(CLEAR_VARS)
LOCAL_VENDOR_MODULE := true
LOCAL_MODULE := libtensorflowlite_c
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := libtensorflowlite_c.so
LOCAL_MODULE_STEM_64 := libtensorflowlite_c.so
LOCAL_SRC_FILES_32 := libtflite/armeabi-v7a/libtensorflowlite_c.so
LOCAL_SRC_FILES_64 := libtflite/arm64-v8a/libtensorflowlite_c.so
LOCAL_CHECK_ELF_FILES := false
include $(BUILD_PREBUILT)
endif

include $(CLEAR_VARS)
LOCAL_MODULE := libunnengine
LOCAL_SRC_FILES := unn_engine.cpp \
                   nna_wrapper.cpp \
                   tflite_wrapper.cpp \
                   mnn_wrapper.cpp \
                   sprd_nn_vdsp_cadence.cpp \
                   vdspnn_wrapper.cpp \
                   ion/unn_ion.cpp \
                   ion/IONObject.cpp

LOCAL_C_INCLUDES := nna_wrapper.h \
                    unn_engine.h \
                    mnn_wrapper.h \
                    tflite_wrapper.h \
                    vdspnn_wrapper.h \
                    sprd_nn_vdsp_cadence.h \
                    $(LOCAL_PATH)/ion \
                    $(LOCAL_PATH)/libmnn/include \
                    $(LOCAL_PATH)/libtflite/include \
                    vendor/sprd/modules/NPU/imgtec/runtime/libnna/include


LOCAL_C_INCLUDES += vendor/sprd/modules/vdsp/Cadence/xrp/vdsp-interface

LOCAL_VENDOR_MODULE := true
LOCAL_SHARED_LIBRARIES := liblog libutils libbinder libcutils
LOCAL_LDLIBS := -ldl -lm -lc -Wl,--gc-sections,--unresolved-symbols=ignore-all
LOCAL_CFLAGS := -O3 -fvisibility=hidden -Wno-tautological-compare

ifeq ($(strip $(MNN_SUPPORT)), true)
LOCAL_SHARED_LIBRARIES += libMNN
LOCAL_CFLAGS += -DENABLE_MNN
endif

ifeq ($(strip $(NPU_SUPPORT)), true)
LOCAL_SHARED_LIBRARIES += libnnaruntime
LOCAL_CFLAGS += -DENABLE_IMGNPU
endif

ifeq ($(strip $(TFLITE_SUPPORT)), true)
LOCAL_SHARED_LIBRARIES += libtensorflowlite_c
LOCAL_CFLAGS += -DENABLE_TFLITE
endif

ifeq ($(strip $(VDSP_SUPPORT)), true)
LOCAL_SHARED_LIBRARIES += libvdspservice
LOCAL_CFLAGS += -DENABLE_VDSPNN
endif

include $(BUILD_SHARED_LIBRARY)
# include $(BUILD_STATIC_LIBRARY)
