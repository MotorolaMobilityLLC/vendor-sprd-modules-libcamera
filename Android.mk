LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

ifeq ($(strip $(TARGET_BOARD_CAMERA_FUNCTION_DUMMY)), true)
include $(LOCAL_PATH)/hal3dummy/Android.mk
else

include $(LOCAL_PATH)/Camera.mk
# include $(call all-subdir-makefiles)

ifeq ($(strip $(TARGET_BOARD_DEL_CPP)),)
include $(call first-makefiles-under,$(LOCAL_PATH))
else

define first-makefiles-under-cus
$(shell build/make/tools/findleaves.py $(FIND_LEAVES_EXCLUDES) \
        --mindepth=2 --prune=ispalg $(addprefix --dir=,$(1)) Android.mk)
endef
include $(call first-makefiles-under-cus,$(LOCAL_PATH))
endif

endif

