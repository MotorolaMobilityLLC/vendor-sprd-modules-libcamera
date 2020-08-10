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
ifeq ($(strip $(TARGET_BOARD_CAMERA_FACE_DETECT)),true)
LOCAL_PATH := $(call my-dir)

ifeq ($(TARGET_ARCH), $(filter $(TARGET_ARCH), arm arm64))
LIB_PATH := lib/lib
else ifeq ($(TARGET_ARCH), $(filter $(TARGET_ARCH), x86 x86_64))
LIB_PATH := lib/x86_lib
endif

include $(CLEAR_VARS)
LOCAL_MODULE := libumnn
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := libumnn.so
LOCAL_MODULE_STEM_64 := libumnn.so
LOCAL_SRC_FILES_32 := $(LIB_PATH)/libumnn.so
LOCAL_SRC_FILES_64 := $(LIB_PATH)64/libumnn.so
LOCAL_SHARED_LIBRARIES := libc libdl liblog libm
LOCAL_MODULE_TAGS := optional
ifeq (1, 1) #(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
LOCAL_PROPRIETARY_MODULE := true
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := libFaceDetectV3
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := libFaceDetectV3.so
LOCAL_MODULE_STEM_64 := libFaceDetectV3.so
LOCAL_SRC_FILES_32 := $(LIB_PATH)/libFaceDetectV3.so
LOCAL_SRC_FILES_64 := $(LIB_PATH)64/libFaceDetectV3.so
LOCAL_SHARED_LIBRARIES := libc libdl liblog libm libumnn
LOCAL_MODULE_TAGS := optional
ifeq (1, 1) #(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
LOCAL_PROPRIETARY_MODULE := true
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := libFaceChecker
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := libFaceChecker.so
LOCAL_MODULE_STEM_64 := libFaceChecker.so
LOCAL_SRC_FILES_32 := $(LIB_PATH)/libFaceChecker.so
LOCAL_SRC_FILES_64 := $(LIB_PATH)64/libFaceChecker.so
LOCAL_SHARED_LIBRARIES := libumnn libc libdl liblog libm
LOCAL_MODULE_TAGS := optional
ifeq (1, 1) #(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
LOCAL_PROPRIETARY_MODULE := true
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := libFaceTrackerFA
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := libFaceTrackerFA.so
LOCAL_MODULE_STEM_64 := libFaceTrackerFA.so
LOCAL_SRC_FILES_32 := $(LIB_PATH)/libFaceTrackerFA.so
LOCAL_SRC_FILES_64 := $(LIB_PATH)64/libFaceTrackerFA.so
LOCAL_SHARED_LIBRARIES := libc libdl liblog libm
LOCAL_MODULE_TAGS := optional
ifeq (1, 1) #(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
LOCAL_PROPRIETARY_MODULE := true
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := libFaceDetect
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := libFaceDetect.so
LOCAL_MODULE_STEM_64 := libFaceDetect.so
LOCAL_SRC_FILES_32 := $(LIB_PATH)/libFaceDetect.so
LOCAL_SRC_FILES_64 := $(LIB_PATH)64/libFaceDetect.so
LOCAL_SHARED_LIBRARIES := libFaceDetectV3 libFaceChecker libFaceTrackerFA libc libdl liblog libm
LOCAL_MODULE_TAGS := optional
ifeq (1, 1) #(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
LOCAL_PROPRIETARY_MODULE := true
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := libFaceDetectHW
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := libFaceDetectHW.so
LOCAL_MODULE_STEM_64 := libFaceDetectHW.so
LOCAL_SRC_FILES_32 := $(LIB_PATH)/libFaceDetectHW.so
LOCAL_SRC_FILES_64 := $(LIB_PATH)64/libFaceDetectHW.so
LOCAL_SHARED_LIBRARIES := libc libdl liblog libm libFaceChecker libFaceTrackerFA libsprdfd_hw
LOCAL_MODULE_TAGS := optional
ifeq (1, 1) #(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
LOCAL_PROPRIETARY_MODULE := true
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := libFaceDetectL
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := libFaceDetectL.so
LOCAL_MODULE_STEM_64 := libFaceDetectL.so
LOCAL_SRC_FILES_32 := $(LIB_PATH)/libFaceDetectL.so
LOCAL_SRC_FILES_64 := $(LIB_PATH)64/libFaceDetectL.so
LOCAL_SHARED_LIBRARIES := libc libdl liblog libm libFaceTrackerFA
LOCAL_MODULE_TAGS := optional
ifeq (1, 1) #(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
LOCAL_PROPRIETARY_MODULE := true
endif
include $(BUILD_PREBUILT)



#SPRD face detect
include $(CLEAR_VARS)
LOCAL_MODULE := libsprdfd
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := libsprdfd.so
LOCAL_MODULE_STEM_64 := libsprdfd.so
LOCAL_SRC_FILES_32 := $(LIB_PATH)/libsprdfd.so
LOCAL_SRC_FILES_64 := $(LIB_PATH)64/libsprdfd.so
LOCAL_SHARED_LIBRARIES := libFaceDetect libFaceDetectHW libFaceDetectL libFaceDetectV3 libc libdl liblog libm libsprdfd_hw
LOCAL_MODULE_TAGS := optional
ifeq (1, 1) #(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
LOCAL_PROPRIETARY_MODULE := true
endif
include $(BUILD_PREBUILT)


# SPRD face alignment library
include $(CLEAR_VARS)
LOCAL_MODULE := libsprdfa
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := libsprdfa.so
LOCAL_MODULE_STEM_64 := libsprdfa.so
LOCAL_SRC_FILES_32 := $(LIB_PATH)/libsprdfa.so
LOCAL_SRC_FILES_64 := $(LIB_PATH)64/libsprdfa.so
LOCAL_SHARED_LIBRARIES := libc libdl liblog libm
LOCAL_MODULE_TAGS := optional
ifeq (1, 1) #(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
LOCAL_PROPRIETARY_MODULE := true
endif
include $(BUILD_PREBUILT)

ifeq ($(strip $(TARGET_BOARD_SPRD_FD_VERSION)),1)
# SPRD face attribute recognition (smile detection) library
include $(CLEAR_VARS)
LOCAL_MODULE := libsprdfarcnn
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := libsprdfarcnn.so
LOCAL_MODULE_STEM_64 := libsprdfarcnn.so
LOCAL_SRC_FILES_32 := $(LIB_PATH)/libsprdfarcnn.so
LOCAL_SRC_FILES_64 := $(LIB_PATH)64/libsprdfarcnn.so
LOCAL_SHARED_LIBRARIES := libc libdl liblog libm
LOCAL_MODULE_TAGS := optional
ifeq (1, 1) #(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
LOCAL_PROPRIETARY_MODULE := true
endif
include $(BUILD_PREBUILT)
else

include $(CLEAR_VARS)
LOCAL_MODULE := libsprdfar
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := libsprdfar.so
LOCAL_MODULE_STEM_64 := libsprdfar.so
LOCAL_SRC_FILES_32 := $(LIB_PATH)/libsprdfar.so
LOCAL_SRC_FILES_64 := $(LIB_PATH)64/libsprdfar.so
LOCAL_SHARED_LIBRARIES := libc libdl liblog libm
LOCAL_MODULE_TAGS := optional
ifeq (1, 1) #(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
LOCAL_PROPRIETARY_MODULE := true
endif
include $(BUILD_PREBUILT)
endif
endif
