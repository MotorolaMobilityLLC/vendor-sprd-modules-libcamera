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
CUR_DIR := tuning_param
LOCAL_SRC_DIR := $(LOCAL_PATH)/$(CUR_DIR)
#SUB_DIR := classic

#tuning param makefile config
SENSOR_FILE_COMPILER := $(TUNING_PARAM_LIST)

SENSOR_FILE_COMPILER := $(shell echo $(SENSOR_FILE_COMPILER))
$(warning $(SENSOR_FILE_COMPILER))

sensor_comma:=,
sensor_empty:=
sensor_space:=$(sensor_empty)
ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_VERSION)),2.1)
LOCAL_CFLAGS += -DCONFIG_ISP_2_1
else ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_VERSION)),2.2)
LOCAL_CFLAGS += -DCONFIG_ISP_2_2
else ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_VERSION)),2.3)
LOCAL_CFLAGS += -DCONFIG_ISP_2_3
else ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_VERSION)),2.4)
LOCAL_CFLAGS += -DCONFIG_ISP_2_4
else ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_VERSION)),2.5)
LOCAL_CFLAGS += -DCONFIG_ISP_2_5
else ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_VERSION)),2.6)
LOCAL_CFLAGS += -DCONFIG_ISP_2_6
else ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_VERSION)),2.7)
LOCAL_CFLAGS += -DCONFIG_ISP_2_7
endif
#$(warning $(LOCAL_PATH))

split_tuning_param:=$(sort $(subst $(sensor_comma),$(sensor_space) ,$(shell echo $(SENSOR_FILE_COMPILER))))
$(warning $(split_tuning_param))

#tuning param makefile config
#ifeq ($(strip $(TARGET_BOARD_TUNING_PARAM_LIB_ENABLED)),false)
#LOCAL_C_INCLUDES += $(shell find $(LOCAL_SRC_DIR)/$(SUB_DIR) -maxdepth 2 -type d)
#endif

#LOCAL_C_INCLUDES += sensor_ic_drv.h

#LOCAL_SRC_FILES += \
#    sensor_drv/sensor_ic_drv.c


#$(foreach item,$(split_sensor),$(eval $(call sensor-c-file-search,$(shell echo $(item) | tr A-Z a-z))))
#ifeq ($(strip $(TARGET_BOARD_TUNING_PARAM_LIB_ENABLED)),false)
#$(foreach item,$(split_sensor),$(eval $(call sensor-c-file-search,$(shell echo $(item)))))
