ifeq ($(strip $(TARGET_BOARD_CAMERA_SENSOR_MODULAR_KERNEL)),yes)

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

#flash makefile config
FLASH_FILE_COMPILER := $(shell echo $(CAMERA_FLASH_IC_TYPE_LIST))
#$(warning $(FLASH_FILE_COMPILER))

flash_comma:=,
flash_empty:=
flash_space:=$(flash_empty) $(flash_empty)

split_flash:=$(sort $(subst $(flash_comma),$(flash_space),$(shell echo $(FLASH_FILE_COMPILER))))
#$(warning $(split_flash))

export split_flash

#$(warning $(LOCAL_PATH))

#include $(wildcard $(LOCAL_PATH)/*/Android.mk)
include $(call first-makefiles-under,$(LOCAL_PATH))

endif
