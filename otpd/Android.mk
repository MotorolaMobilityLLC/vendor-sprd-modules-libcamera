LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    otpd.c      \
    otpd_listen.c\
    otp_buf.c	\
    otp_fs.c	\
    otp_os.c

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libdl \
    liblog \

LOCAL_MODULE := otpd
LOCAL_INIT_RC := otpd.rc
LOCAL_MODULE_TAGS := optional
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_EXECUTABLE)

include $(call all-makefiles-under,$(LOCAL_PATH))
