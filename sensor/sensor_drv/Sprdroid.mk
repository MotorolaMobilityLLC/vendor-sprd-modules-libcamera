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
CUR_DIR := sensor_drv
LOCAL_SRC_DIR := $(LOCAL_PATH)/$(CUR_DIR)
SUB_DIR := classic

LOCAL_C_INCLUDES += $(shell find $(LOCAL_SRC_DIR)/$(SUB_DIR) -maxdepth 2 -type d)

#LOCAL_SRC_FILES += $(shell find $(LOCAL_SRC_DIR)/$(SUB_DIR) -maxdepth 3 -iregex ".*\.\(c\)" | sed s:^$(LOCAL_PATH)/::g )

LOCAL_SRC_FILES += sensor_drv/sensor_ic_drv.c

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_DIR)),3)
LOCAL_SRC_FILES += \
	sensor_drv/classic/Samsung/s5k3p8sm/sensor_s5k3p8sm_mipi_raw.c \
	sensor_drv/classic/Samsung/s5k4h8yx/sensor_s5k4h8yx_mipi_raw.c \
	sensor_drv/classic/Samsung/s5k3l8xxm3/sensor_s5k3l8xxm3_mipi_raw.c\
	sensor_drv/classic/Samsung/s5k5e2ya/sensor_s5k5e2ya_mipi_raw.c\
	sensor_drv/classic/Sony/imx230/sensor_imx230_mipi_raw.c \
	sensor_drv/classic/Sony/imx258/sensor_imx258_mipi_raw.c \
	sensor_drv/classic/OmniVision/ov2680/sensor_ov2680_mipi_raw.c \
	sensor_drv/classic/OmniVision/ov8856/sensor_ov8856_mipi_raw.c 
endif

ifeq ($(strip $(TARGET_BOARD_COVERED_SENSOR_SUPPORT)),true)
	LOCAL_SRC_FILES += \
	sensor_drv/classic/Galaxycore/gc0310/sensor_gc0310_mipi.c \
	sensor_drv/classic/Cista/c2580/sensor_c2580_mipi_raw.c
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_DIR)),2.1)
	LOCAL_SRC_FILES += \
	sensor_drv/classic/Sony/imx258/sensor_imx258_mipi_raw.c \
	sensor_drv/classic/OmniVision/ov13855/sensor_ov13855_mipi_raw.c \
	sensor_drv/classic/OmniVision/ov5675/sensor_ov5675_mipi_raw.c \
	sensor_drv/classic/Galaxycore/gc8024/sensor_gc8024_mipi_raw.c \
	sensor_drv/classic/Galaxycore/gc2375/sensor_gc2375_mipi_raw.c \
	sensor_drv/classic/Galaxycore/gc5005/sensor_gc5005_mipi_raw.c \
	sensor_drv/classic/Superpix/sp8407/sensor_sp8407_mipi_raw.c \
	sensor_drv/classic/Cista/c2390/sensor_c2390_mipi_raw.c \
	sensor_drv/classic/Cista/c2580/sensor_c2580_mipi_raw.c
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_DIR)),2.2)
	LOCAL_SRC_FILES += \
	sensor_drv/classic/Sony/imx258/sensor_imx258_mipi_raw.c \
	sensor_drv/classic/OmniVision/ov13855/sensor_ov13855_mipi_raw.c \
	sensor_drv/classic/OmniVision/ov5675/sensor_ov5675_mipi_raw.c \
	sensor_drv/classic/OmniVision/ov5675_dual/sensor_ov5675_mipi_raw.c \
	sensor_drv/classic/Galaxycore/gc8024/sensor_gc8024_mipi_raw.c \
	sensor_drv/classic/Galaxycore/gc2375/sensor_gc2375_mipi_raw.c \
	sensor_drv/classic/Galaxycore/gc5005/sensor_gc5005_mipi_raw.c \
	sensor_drv/classic/Superpix/sp8407/sensor_sp8407_mipi_raw.c \
	sensor_drv/classic/Cista/c2390/sensor_c2390_mipi_raw.c \
	sensor_drv/classic/Cista/c2580/sensor_c2580_mipi_raw.c
endif

#Second optimization
ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_DIR)),2.3)
	LOCAL_SRC_FILES += \
	sensor_drv/classic/Sony/imx258/sensor_imx258_mipi_raw.c \
	sensor_drv/classic/OmniVision/ov13855/sensor_ov13855_mipi_raw.c \
	sensor_drv/classic/OmniVision/ov5675/sensor_ov5675_mipi_raw.c \
	sensor_drv/classic/Galaxycore/gc8024/sensor_gc8024_mipi_raw.c \
	sensor_drv/classic/Galaxycore/gc2375/sensor_gc2375_mipi_raw.c \
	sensor_drv/classic/Galaxycore/gc5005/sensor_gc5005_mipi_raw.c \
	sensor_drv/classic/Superpix/sp8407/sensor_sp8407_mipi_raw.c \
	sensor_drv/classic/Cista/c2390/sensor_c2390_mipi_raw.c \
	sensor_drv/classic/Cista/c2580/sensor_c2580_mipi_raw.c
endif
