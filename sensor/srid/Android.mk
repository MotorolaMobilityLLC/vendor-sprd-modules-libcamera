# Copyright 2013 The Android Open Source Project

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= srid.c

LOCAL_SHARED_LIBRARIES:= libcutils libdl

LOCAL_MODULE:= srid
LOCAL_MULTILIB := 32
#LOCAL_CFLAGS += -fno-strict-aliasing -Werror
LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)
