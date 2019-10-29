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


LOCAL_PATH:= $(call my-dir)


include $(CLEAR_VARS)
LOCAL_MODULE := libsprddepth
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MULTILIB := both
#LOCAL_MODULE_STEM := libsprddepth.so
LOCAL_SRC_FILES_32 = arm_lib/32/libsprddepth.so
LOCAL_SRC_FILES_64 = arm_lib/64/libsprddepth.so
LOCAL_PROPRIETARY_MODULE := true
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := libbokeh_depth
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MULTILIB := both
#LOCAL_MODULE_STEM := libbokeh_depth.so
LOCAL_SRC_FILES_32 = arm_lib/32/libbokeh_depth.so
LOCAL_SRC_FILES_64 = arm_lib/64/libbokeh_depth.so
LOCAL_PROPRIETARY_MODULE := true
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := libsprdhdr
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MULTILIB := both
#LOCAL_MODULE_STEM := libbokeh_depth.so
LOCAL_SRC_FILES_32 = arm_lib/32/libsprdhdr.so
LOCAL_SRC_FILES_64 = arm_lib/64/libsprdhdr.so

LOCAL_PROPRIETARY_MODULE := true
include $(BUILD_PREBUILT)


include $(CLEAR_VARS)


LOCAL_32_BIT_ONLY:=true

LOCAL_SRC_FILES := main.cpp \
		   assist.cpp \
		   depth_bokeh/ImageFormat_Conversion.cpp \
		   depth_bokeh/bmp_io.cpp \
		   hdr/bmp.cpp \
		   hdr/hdr_test.cpp \
		   hdr/util.cpp

LOCAL_MODULE :=test_vdsp
LOCAL_SHARED_LIBRARIES := libsprddepth libbokeh_depth libvdspservice libsprdhdr libbinder libutils
LOCAL_ARM_MODE := arm
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS :=  -O3 -fno-strict-aliasing -fPIC -fvisibility=hidden -DNEON_OPT -DMULTI_THREAD -DBOKEH_OPT -DBOKEH_TEST 
LOCAL_CFLAGS += -Wno-error=unknown-pragmas -Wno-error=unknown-attributes -Wno-error=deprecated-declarations \
		-Wno-error=macro-redefined -Wno-error=uninitialized -Wno-error=unused-parameter \
		-Wno-error=constant-conversion -Wno-error=format -Wno-error=date-time -Wno-error=unused-value -Wno-error=unused-variable -Wno-error=sign-compare -Wno-error=writable-strings
LOCAL_STATIC_LIBRARIES :=
LOCAL_C_INCLUDES := \
        $(LOCAL_PATH)/depth_bokeh \
	$(LOCAL_PATH)/hdr \
	$(TOP)/system/core/base/include

LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_EXECUTABLE)
