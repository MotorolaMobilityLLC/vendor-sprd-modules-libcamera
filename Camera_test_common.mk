include $(LOCAL_PATH)/SprdCtrl.mk

ISP_HW_VER = 3v0

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_DIR)),2.1)
ISP_HW_VER = 2v1
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_DIR)),2.2)
ISP_HW_VER = 2v1
endif


ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_DIR)),3)
ISP_HW_VER = 3v0
endif


ifeq ($(strip $(isp_use2.0)),1)
LOCAL_SRC_FILES+= \
	oem$(ISP_HW_VER)/isp_calibration/src/port_isp2.0.c
else
LOCAL_SRC_FILES+= \
	oem/isp_calibration/src/port_isp1.0.c
endif

ifneq ($(findstring HAL3,$(strip $(TARGET_BOARD_CAMERA_HAL_VERSION))),)
	ifeq ($(strip $(isp_use2.0)),1)
		LOCAL_SRC_FILES += oem$(ISP_HW_VER)/isp_calibration/src/testdcam3.cpp
        else
		LOCAL_SRC_FILES += oem/isp_calibration/src/testdcam3.cpp
        endif
	LOCAL_CFLAGS +=-DCONFIG_CAMERA_HAL_VERSION=3
else
	ifeq ($(strip $(isp_use2.0)),1)
		LOCAL_SRC_FILES += oem$(ISP_HW_VER)/isp_calibration/src/testdcam1.cpp
        else
		LOCAL_SRC_FILES += oem/isp_calibration/src/testdcam1.cpp
        endif
	LOCAL_CFLAGS +=-DCONFIG_CAMERA_HAL_VERSION=1
endif

MINICAMERA:=2

ifeq ($(strip $(MINICAMERA)),2)
LOCAL_CFLAGS += -DMINICAMERA=2
else
LOCAL_CFLAGS += -DMINICAMERA=1
endif

include $(LOCAL_PATH)/SprdLib.mk

#include $(shell find $(LOCAL_PATH) -name 'Sprdroid.mk')
