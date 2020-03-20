#
# Copyright (C) 2018 The Android Open Source Project
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

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS += -fno-strict-aliasing -Wunused-variable -Wunused-function -Werror

# ************************************************
# external header file
# ************************************************
LOCAL_C_INCLUDES := \
	$(TARGET_BSP_UAPI_PATH)/kernel/usr/include/video \
	$(LOCAL_PATH)/../../../common/inc \
	$(LOCAL_PATH)/../../../kernel_module/interface

# ************************************************
# internal header file
# ************************************************
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../../../$(ISPALG_DIR)/smart \
	$(LOCAL_PATH)/../../../$(ISPALG_DIR)/awb/inc \
	$(LOCAL_PATH)/../../../$(ISPALG_DIR)/ae/inc \
	$(LOCAL_PATH)/../../../$(ISPALG_DIR)/ae/sprd/ae3.x/ae/inc \
	$(LOCAL_PATH)/../../../$(ISPALG_DIR)/ae/sprd/ae2.x/ae/inc \
	$(LOCAL_PATH)/../../../$(ISPALG_DIR)/af/inc \
	$(LOCAL_PATH)/../../../$(ISPALG_DIR)/common/inc \
	$(LOCAL_PATH)/../../../$(OEM_DIR)/inc \
	$(LOCAL_PATH)/../middleware/inc \
	$(LOCAL_PATH)/../param_manager \
	$(LOCAL_PATH)/../driver/inc


#LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_BSP_UAPI_PATH)/kernel/usr

LOCAL_SRC_FILES += $(call all-c-files-under, .)

include $(LOCAL_PATH)/../../../SprdCtrl.mk

LOCAL_MODULE := libcambr

LOCAL_MODULE_TAGS := optional

LOCAL_SHARED_LIBRARIES := libcutils libutils libdl liblog
LOCAL_SHARED_LIBRARIES += libcamcommon

ifeq (1, 1) #(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
LOCAL_PROPRIETARY_MODULE := true
endif

LOCAL_CFLAGS += -DTEST_ON_HAPS
include $(BUILD_SHARED_LIBRARY)

include $(call first-makefiles-under,$(LOCAL_PATH))
