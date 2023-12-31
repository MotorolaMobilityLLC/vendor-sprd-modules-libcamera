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

LOCAL_PATH := $(call my-dir)

#gtest cnr apt
include $(CLEAR_VARS)
LOCAL_SRC_FILES := main_test.cpp cnr_it_adapter_test.cpp
LOCAL_MODULE:= sprd_cnr_test_adpt

#LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := libutils libhidlbase libsprdcnradapter
LOCAL_C_INCLUDES := $(TOP)/external/googletest/googletest/include $(LOCAL_PATH)/../inc $(LOCAL_PATH)/../../inc

LOCAL_COMPATIBILITY_SUITE := general-tests units

LOCAL_PROPRIETARY_MODULE := true
include $(BUILD_NATIVE_TEST)

