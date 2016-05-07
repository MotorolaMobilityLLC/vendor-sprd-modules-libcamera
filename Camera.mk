LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

TARGET_BOARD_CAMERA_READOTP_METHOD?=0


TARGET_BOARD_SC_IOMMU_PF:=1

include $(LOCAL_PATH)/SprdCtrl.mk

include $(LOCAL_PATH)/SprdLib.mk

LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_MODULE := camera.$(TARGET_BOARD_PLATFORM)

LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)


ifeq ($(strip $(TARGET_BOARD_CAMERA_UV_DENOISE)),true)
include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := oem2v0/isp/libuvdenoise.so
LOCAL_MODULE_TAGS := optional
include $(BUILD_MULTI_PREBUILT)
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_Y_DENOISE)),true)
include $(CLEAR_VARS)
    LOCAL_PREBUILT_LIBS := oem2v0/isp/libynoise.so
LOCAL_MODULE_TAGS := optional
include $(BUILD_MULTI_PREBUILT)
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_FACE_BEAUTY)),false)
else
ifeq ($(strip $(TARGET_ARCH)),arm64)
include $(CLEAR_VARS)
LOCAL_MODULE := libts_face_beautify_hal
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := libts_face_beautify_hal.so
LOCAL_MODULE_STEM_64 := libts_face_beautify_hal.so
LOCAL_SRC_FILES_32 :=  arithmetic/facebeauty/libts_face_beautify_hal.so
LOCAL_SRC_FILES_64 :=  arithmetic/facebeauty/libts_face_beautify_hal_64.so
include $(BUILD_PREBUILT)
else
include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := arithmetic/facebeauty/libts_face_beautify_hal.so
LOCAL_MODULE_TAGS := optional
include $(BUILD_MULTI_PREBUILT)
endif
endif

ifeq ($(strip $(TARGET_BOARD_CONFIG_CAMERA_RE_FOCUS)),true)
ifeq ($(strip $(TARGET_ARCH)),arm64)
include $(CLEAR_VARS)
LOCAL_MODULE :=libalRnBLV
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := libalRnBLV.so
LOCAL_MODULE_STEM_64 := libalRnBLV.so
LOCAL_SRC_FILES_32 :=  arithmetic/libalRnBLV.so
LOCAL_SRC_FILES_64 :=  arithmetic/libalRnBLV_v8a.so
include $(BUILD_PREBUILT)
else
include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := arithmetic/libalRnBLV.so
LOCAL_MODULE_TAGS := optional
include $(BUILD_MULTI_PREBUILT)
endif
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_HDR_CAPTURE)),true)
	ifeq ($(strip $(TARGET_BOARD_CAMERA_HDR_SPRD_LIB)),true)
		ifeq ($(strip $(TARGET_ARCH)),arm64)
			include $(CLEAR_VARS)
			LOCAL_MODULE := libsprd_easy_hdr
			LOCAL_MODULE_CLASS := SHARED_LIBRARIES
			LOCAL_MODULE_TAGS := optional
			LOCAL_MULTILIB := both
			LOCAL_MODULE_STEM_32 := libsprd_easy_hdr.so
			LOCAL_MODULE_STEM_64 := libsprd_easy_hdr.so
			LOCAL_SRC_FILES_32 :=  arithmetic/libsprd_easy_hdr.so
			LOCAL_SRC_FILES_64 :=  arithmetic/lib64/libsprd_easy_hdr.so
			include $(BUILD_PREBUILT)
		else
			include $(CLEAR_VARS)
			LOCAL_PREBUILT_LIBS := arithmetic/libsprd_easy_hdr.so
			LOCAL_MODULE_TAGS := optional
			include $(BUILD_MULTI_PREBUILT)
		endif #end ARCH64
	endif #end SPRD_LIB
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_FACE_DETECT)),true)
	ifeq ($(strip $(TARGET_BOARD_CAMERA_FD_LIB)),omron)
	ifeq ($(strip $(TARGET_ARCH)),arm64)
		include $(CLEAR_VARS)
		LOCAL_MODULE := libeUdnDt
		LOCAL_MODULE_CLASS := STATIC_LIBRARIES
		LOCAL_MULTILIB := both
		LOCAL_MODULE_STEM_32 := libeUdnDt.a
		LOCAL_MODULE_STEM_64 := libeUdnDt.a
		LOCAL_SRC_FILES_32 := arithmetic/omron/lib32/libeUdnDt.a
		LOCAL_SRC_FILES_64 := arithmetic/omron/lib64/libeUdnDt.a
		LOCAL_MODULE_TAGS := optional
		include $(BUILD_PREBUILT)

		include $(CLEAR_VARS)
		LOCAL_MODULE := libeUdnCo
		LOCAL_MODULE_CLASS := STATIC_LIBRARIES
		LOCAL_MULTILIB := both
		LOCAL_MODULE_STEM_32 := libeUdnCo.a
		LOCAL_MODULE_STEM_64 := libeUdnCo.a
		LOCAL_SRC_FILES_32 := arithmetic/omron/lib32/libeUdnCo.a
		LOCAL_SRC_FILES_64 := arithmetic/omron/lib64/libeUdnCo.a
		LOCAL_MODULE_TAGS := optional
		include $(BUILD_PREBUILT)
	else
		include $(CLEAR_VARS)
		LOCAL_MODULE := libeUdnDt
		LOCAL_SRC_FILES := arithmetic/omron/lib32/libeUdnDt.a
		LOCAL_MODULE_TAGS := optional
		include $(PREBUILT_STATIC_LIBRARY)

		include $(CLEAR_VARS)
		LOCAL_MODULE := libeUdnCo
		LOCAL_SRC_FILES := arithmetic/omron/lib32/libeUdnCo.a
		LOCAL_MODULE_TAGS := optional
		include $(PREBUILT_STATIC_LIBRARY)

		include $(CLEAR_VARS)
		LOCAL_PREBUILT_LIBS := arithmetic/omron/lib32/libeUdnDt.a
		LOCAL_MODULE_TAGS := optional
		include $(BUILD_MULTI_PREBUILT)

		include $(CLEAR_VARS)
		LOCAL_PREBUILT_LIBS := arithmetic/omron/lib32/libeUdnCo.a
		LOCAL_MODULE_TAGS := optional
		include $(BUILD_MULTI_PREBUILT)
	endif # end TARGET_ARCH
	else
		include $(CLEAR_VARS)
		LOCAL_PREBUILT_LIBS := arithmetic/libface_finder.so
		LOCAL_MODULE_TAGS := optional
		include $(BUILD_MULTI_PREBUILT)
	endif # fd_lib
endif
