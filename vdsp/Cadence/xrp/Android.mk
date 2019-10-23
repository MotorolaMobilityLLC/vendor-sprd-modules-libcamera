#
# Copyright (C) 2008 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
ifeq ($(strip $(TARGET_BOARD_VDSP_MODULAR_KERNEL)), cadence)
LOCAL_PATH:= $(call my-dir)
BUILD_ANDROID = 1

ifeq ($(BUILD_ANDROID), 1)
# ************************************************
# libxrp-common
# ************************************************
include $(CLEAR_VARS)
LOCAL_CFLAGS += -fno-strict-aliasing -Wunused-variable -Werror -fvisibility=hidden

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/xrp-host/inc \
	$(LOCAL_PATH)/xrp-common/inc

LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_BSP_UAPI_PATH)/kernel/usr

LOCAL_SRC_FILES := \
			xrp-common/src/xrp_ns.c \
			xrp-common/src/xrp_rb_file.c

LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE := libxrp-common
LOCAL_MODULE_TAGS := optional
#LOCAL_MULTILIB := both

include $(BUILD_SHARED_LIBRARY)

# ************************************************
# libxrp-host
# ************************************************
include $(CLEAR_VARS)
LOCAL_CFLAGS += -fno-strict-aliasing -Wunused-variable -Werror -fvisibility=hidden

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/xrp-host/inc \
	$(LOCAL_PATH)/vdsp-interface \
	$(LOCAL_PATH)/xrp-common/inc \
	$(TOP)/vendor/sprd/modules/libmemion \
	$(LOCAL_PATH)/vdsp-service/interface \
	$(LOCAL_PATH)/xrp-example/inc \
	$(TOP)/vendor/sprd/external/kernel-headers

LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_BSP_UAPI_PATH)/kernel/usr

LOCAL_SRC_FILES := \
			xrp-host/src/xrp_host_common.c  \
			xrp-host/src/xrp_threaded_queue.c \
			xrp-host/src/xrp_linux.c \
			vdsp-interface/xrp_interface.c \
			vdsp-interface/vdsp_interface_internal.c \
			vdsp-interface/sprd_vdsp_ion.cpp

LOCAL_SHARED_LIBRARIES := libmemion libcutils libutils

LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE := libxrp-host-hosted
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := both

#include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_CFLAGS += -fno-strict-aliasing -Wunused-variable -Werror -fvisibility=hidden

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/xrp-host/inc \
	$(LOCAL_PATH)/vdsp-interface \
	$(LOCAL_PATH)/xrp-common/inc \
	$(TOP)/vendor/sprd/modules/libmemion \
	$(TOP)/vendor/sprd/external/kernel-headers \
	$(LOCAL_PATH)/vdsp-service \
	$(LOCAL_PATH)/vdsp-service/interface \
	$(TOP)/system/core/base/include \
	$(LOCAL_PATH)/vdsp-interface \
	$(LOCAL_PATH)/xrp-example/inc \
	$(LOCAL_PATH)/vdsp-service/dvfs

LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_BSP_UAPI_PATH)/kernel/usr
LOCAL_LDLIBS    := -lm -llog
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES := \
			vdsp-service/IVdspService.cpp \
			vdsp-interface/vdsp_interface.cpp \
			vdsp-interface/sprd_vdsp_ion.cpp \
			vdsp-service/dvfs/vdsp_dvfs.cpp \
			xrp-host/src/xrp_host_common.c  \
			xrp-host/src/xrp_threaded_queue.c \
			vdsp-interface/xrp_interface.c \
			vdsp-interface/vdsp_interface_internal.c \
			xrp-host/src/xrp_linux.c

LOCAL_SHARED_LIBRARIES := libcutils libbinder libutils libion libmemion

LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE := libvdspservice
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := both

include $(BUILD_SHARED_LIBRARY)
# ************************************************
# EXAMPLE xrp-host-hosted
# ************************************************
include $(CLEAR_VARS)
LOCAL_CFLAGS += -fno-strict-aliasing -Wunused-variable -Werror -fvisibility=hidden

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/xrp-host/inc \
	$(LOCAL_PATH)/xrp-common/inc \
	$(LOCAL_PATH)/xrp-example/inc \
	$(LOCAL_PATH)/vdsp-interface \
	$(LOCAL_PATH)/vdsp-service/interface

LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_BSP_UAPI_PATH)/kernel/usr


include $(CLEAR_VARS)
LOCAL_CFLAGS += -fno-strict-aliasing -Wunused-variable -Werror -fvisibility=hidden

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/xrp-host/inc \
	$(LOCAL_PATH)/xrp-common/inc \
	$(LOCAL_PATH)/xrp-example/inc \
	$(LOCAL_PATH)/vdsp-service/interface \
	$(LOCAL_PATH)/vdsp-service/include \
	$(TOP)/external/icu/android_utils/include \
	$(LOCAL_PATH)/vdsp-interface

LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_BSP_UAPI_PATH)/kernel/usr


LOCAL_SRC_FILES := \
			vdsp-service/VdspServer.cpp
LOCAL_LDLIBS    := -lm -llog
LOCAL_SHARED_LIBRARIES := libutils libcutils libvdspservice libbinder #libicuandroid_utils

LOCAL_MODULE := vdspserver
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/bin

#LOCAL_INIT_RC := vdsp-service/service.vdspservice.rc

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_CFLAGS += -fno-strict-aliasing -Wunused-variable -Werror -fvisibility=hidden

LOCAL_C_INCLUDES := \
        $(LOCAL_PATH)/xrp-host/inc \
        $(LOCAL_PATH)/xrp-common/inc \
        $(LOCAL_PATH)/xrp-example/inc \
        $(LOCAL_PATH)/vdsp-service \
        $(TOP)/vendor/sprd/modules/libmemion \
        $(TOP)/vendor/sprd/external/kernel-headers \
        $(TOP)/system/core/base/include \
	$(LOCAL_PATH)/vdsp-interface


LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_BSP_UAPI_PATH)/kernel/usr

LOCAL_LDLIBS    := -lm -llog

LOCAL_SRC_FILES := \
                        vdsp-example/TestClient.cpp

LOCAL_SHARED_LIBRARIES := libutils libcutils libbinder libvdspservice libion

LOCAL_MODULE := xrpclient
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/bin

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_CFLAGS += -fno-strict-aliasing -fvisibility=hidden

LOCAL_C_INCLUDES := \
        $(LOCAL_PATH)/xrp-host/inc \
        $(LOCAL_PATH)/xrp-common/inc \
        $(LOCAL_PATH)/xrp-example/inc \
        $(LOCAL_PATH)/vdsp-service/interface \
        $(TOP)/vendor/sprd/modules/libmemion \
        $(TOP)/vendor/sprd/external/kernel-headers \
        $(TOP)/system/core/base/include


LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_BSP_UAPI_PATH)/kernel/usr

LOCAL_LDLIBS    := -lm -llog

LOCAL_SRC_FILES := \
                        vdsp-example/TestClient2.cpp

LOCAL_SHARED_LIBRARIES := libutils libcutils libbinder libvdspservice libion

LOCAL_MODULE := xrpclient2
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/bin

#include $(BUILD_EXECUTABLE)
# ************************************************used static lib **********************************************************
else

include $(CLEAR_VARS)
LOCAL_MODULE := libxrp-common
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := $(LOCAL_MODULE).a
LOCAL_MODULE_STEM_64 := $(LOCAL_MODULE).a
LOCAL_SRC_FILES_32 := vdsp-interface/lib/32/$(LOCAL_MODULE).a
LOCAL_SRC_FILES_64 := vdsp-interface/lib/64/$(LOCAL_MODULE).a

LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := libxrp-host-hosted
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := $(LOCAL_MODULE).a
LOCAL_MODULE_STEM_64 := $(LOCAL_MODULE).a
LOCAL_SRC_FILES_32 := vdsp-interface/lib/32/$(LOCAL_MODULE).a
LOCAL_SRC_FILES_64 := vdsp-interface/lib/64/$(LOCAL_MODULE).a

LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_PREBUILT)




include $(CLEAR_VARS)

LOCAL_CFLAGS += -fno-strict-aliasing -Wunused-variable -Werror -fvisibility=hidden

LOCAL_C_INCLUDES := \
        $(LOCAL_PATH)/xrp-host/inc \
        $(LOCAL_PATH)/xrp-common/inc \
        $(LOCAL_PATH)/vdsp-interface

LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_BSP_UAPI_PATH)/kernel/usr

# don't modify this code
LOCAL_SRC_FILES := xrp-example/host_main.c

LOCAL_MODULE := xrptest

LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES = libutils libcutils libvdsp_xrp
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/bin/
#LOCAL_STATIC_LIBRARIES = libxrp-common libxrp-host-hosted

include $(BUILD_EXECUTABLE)

endif

include $(call all-makefiles-under,$(LOCAL_PATH))
endif
