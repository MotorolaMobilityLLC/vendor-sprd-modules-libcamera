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
# ---------------------------------------------------------------------------
#                      Make the shared library
# ---------------------------------------------------------------------------

$(warning "CHIP_NAME" $(CHIP_NAME))
ifeq ($(strip $(CHIP_NAME)),pike2)
LOCAL_PATH:= $(call my-dir)

# ---------------------------------------------------------------------------
#                      Camera ID 0
# ---------------------------------------------------------------------------
include $(CLEAR_VARS)
include $(LOCAL_PATH)/../../../../../SprdCtrl.mk
LOCAL_C_INCLUDES := $(TARGET_BSP_UAPI_PATH)/kernel/usr/include/video \
        $(LOCAL_PATH)/../../../../../kernel_module/interface \
		$(LOCAL_PATH)/../../../../../$(ISPDRV_DIR)/driver/inc \
		$(LOCAL_PATH)/../../../../../$(ISPDRV_DIR)/middleware/inc \
		$(LOCAL_PATH)/../../../../../$(ISPDRV_DIR)/isp_tune \
		$(LOCAL_PATH)/../../../../../common/inc \
        $(LOCAL_PATH)/../../../../../$(ISPALG_DIR)/common/inc \
		$(LOCAL_PATH)/../../../../../$(ISPALG_DIR)/ae/sprd/ae/inc \
		$(LOCAL_PATH)/../../../../inc/ \
		$(LOCAL_PATH)/../../../../../$(OEM_DIR)/inc

LOCAL_SRC_FILES := default_param_process.c
LOCAL_SHARED_LIBRARIES := libcutils libutils liblog libdl libxml2
ifeq (1, $(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
LOCAL_PROPRIETARY_MODULE := true
endif
LOCAL_MODULE           := libparam_default_id_0
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)

# ---------------------------------------------------------------------------
#                      Camera ID 1
# ---------------------------------------------------------------------------
include $(CLEAR_VARS)
include $(LOCAL_PATH)/../../../../../SprdCtrl.mk
LOCAL_C_INCLUDES := $(TARGET_BSP_UAPI_PATH)/kernel/usr/include/video \
        $(LOCAL_PATH)/../../../../../kernel_module/interface \
		$(LOCAL_PATH)/../../../../../$(ISPDRV_DIR)/driver/inc \
		$(LOCAL_PATH)/../../../../../$(ISPDRV_DIR)/middleware/inc \
		$(LOCAL_PATH)/../../../../../$(ISPDRV_DIR)/isp_tune \
		$(LOCAL_PATH)/../../../../../common/inc \
        $(LOCAL_PATH)/../../../../../$(ISPALG_DIR)/common/inc \
		$(LOCAL_PATH)/../../../../../$(ISPALG_DIR)/ae/sprd/ae/inc \
		$(LOCAL_PATH)/../../../../inc/ \
		$(LOCAL_PATH)/../../../../../$(OEM_DIR)/inc

LOCAL_SRC_FILES := default_param_process.c
LOCAL_SHARED_LIBRARIES := libcutils libutils liblog libdl libxml2
ifeq (1, $(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
LOCAL_PROPRIETARY_MODULE := true
endif
LOCAL_MODULE           := libparam_default_id_1
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)

# ---------------------------------------------------------------------------
#                      Camera ID 2
# ---------------------------------------------------------------------------
include $(CLEAR_VARS)
include $(LOCAL_PATH)/../../../../../SprdCtrl.mk
LOCAL_C_INCLUDES := $(TARGET_BSP_UAPI_PATH)/kernel/usr/include/video \
        $(LOCAL_PATH)/../../../../../kernel_module/interface \
		$(LOCAL_PATH)/../../../../../$(ISPDRV_DIR)/driver/inc \
		$(LOCAL_PATH)/../../../../../$(ISPDRV_DIR)/middleware/inc \
		$(LOCAL_PATH)/../../../../../$(ISPDRV_DIR)/isp_tune \
		$(LOCAL_PATH)/../../../../../common/inc \
        $(LOCAL_PATH)/../../../../../$(ISPALG_DIR)/common/inc \
		$(LOCAL_PATH)/../../../../../$(ISPALG_DIR)/ae/sprd/ae/inc \
		$(LOCAL_PATH)/../../../../inc/ \
		$(LOCAL_PATH)/../../../../../$(OEM_DIR)/inc

LOCAL_SRC_FILES := default_param_process.c
LOCAL_SHARED_LIBRARIES := libcutils libutils liblog libdl libxml2
ifeq (1, $(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
LOCAL_PROPRIETARY_MODULE := true
endif
LOCAL_MODULE           := libparam_default_id_2
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)

# ---------------------------------------------------------------------------
#                      Camera ID 3
# ---------------------------------------------------------------------------
include $(CLEAR_VARS)
include $(LOCAL_PATH)/../../../../../SprdCtrl.mk
LOCAL_C_INCLUDES := $(TARGET_BSP_UAPI_PATH)/kernel/usr/include/video \
        $(LOCAL_PATH)/../../../../../kernel_module/interface \
		$(LOCAL_PATH)/../../../../../$(ISPDRV_DIR)/driver/inc \
		$(LOCAL_PATH)/../../../../../$(ISPDRV_DIR)/middleware/inc \
		$(LOCAL_PATH)/../../../../../$(ISPDRV_DIR)/isp_tune \
		$(LOCAL_PATH)/../../../../../common/inc \
        $(LOCAL_PATH)/../../../../../$(ISPALG_DIR)/common/inc \
		$(LOCAL_PATH)/../../../../../$(ISPALG_DIR)/ae/sprd/ae/inc \
		$(LOCAL_PATH)/../../../../inc/ \
		$(LOCAL_PATH)/../../../../../$(OEM_DIR)/inc

LOCAL_SRC_FILES := default_param_process.c
LOCAL_SHARED_LIBRARIES := libcutils libutils liblog libdl libxml2
ifeq (1, $(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
LOCAL_PROPRIETARY_MODULE := true
endif
LOCAL_MODULE           := libparam_default_id_3
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)
endif
