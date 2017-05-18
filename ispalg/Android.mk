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

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS += -fno-strict-aliasing -Werror

#AE_WORK_MOD_V0: Old ae algorithm + slow converge
#AE_WORK_MOD_V1: new ae algorithm + slow converge
#AE_WORK_MOD_V2: new ae algorithm + fast converge
LOCAL_CFLAGS += -DAE_WORK_MOD_V0

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_3AMOD)),1)
include $(call first-makefiles-under,$(LOCAL_PATH))
endif