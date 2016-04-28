LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

isp_use2.0:=0
TARGET_BOARD_CAMERA_READOTP_METHOD?=0


TARGET_BOARD_SC_IOMMU_PF:=1

include $(LOCAL_PATH)/SprdCtrl.mk

include $(LOCAL_PATH)/SprdLib.mk

LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_MODULE := camera.$(TARGET_BOARD_PLATFORM)

LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

#ALC_S
ifeq ($(strip $(TARGET_BOARD_USE_ALC_AWB)),true)

ifeq ($(strip $(TARGET_ARCH)),arm)
include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := isp2.0/awb/libAl_Awb.so
LOCAL_MODULE_TAGS := optional
include $(BUILD_MULTI_PREBUILT)
endif

ifeq ($(strip $(TARGET_ARCH)),arm64)
include $(CLEAR_VARS)
LOCAL_MODULE := libAl_Awb_Sp
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := libAl_Awb_Sp.so
LOCAL_MODULE_STEM_64 := libAl_Awb_Sp.so
LOCAL_SRC_FILES_32 :=  isp2.0/awb/libAl_Awb_Sp.so
LOCAL_SRC_FILES_64 :=  isp2.0/awb/libAl_Awb_Sp.so
include $(BUILD_PREBUILT)
else

include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := isp2.0/awb/libAl_Awb_Sp.so
LOCAL_MODULE_TAGS := optional
include $(BUILD_MULTI_PREBUILT)
endif

endif
#ALC_E


ifeq ($(strip $(TARGET_ARCH)),arm64)
include $(CLEAR_VARS)
LOCAL_MODULE := libisp
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := libisp.so
LOCAL_MODULE_STEM_64 := libisp.so
ifeq ($(strip $(isp_use2.0)),1)
    LOCAL_SRC_FILES_32 :=  oem2v0/isp/libisp.so
    LOCAL_SRC_FILES_64 :=  oem2v0/isp/libisp_64.so
else
    LOCAL_SRC_FILES_32 :=  oem/isp/libisp.so
    LOCAL_SRC_FILES_64 :=  oem/isp/libisp_64.so
endif
include $(BUILD_PREBUILT)
else
include $(CLEAR_VARS)
ifeq ($(strip $(isp_use2.0)),1)
    LOCAL_PREBUILT_LIBS := oem2v0/isp/libisp.so
else
    LOCAL_PREBUILT_LIBS := oem/isp1.0/libawb.so \
        oem/isp1.0/libaf.so \
        oem/isp1.0/liblsc.so
endif
LOCAL_MODULE_TAGS := optional
include $(BUILD_MULTI_PREBUILT)
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_UV_DENOISE)),true)
include $(CLEAR_VARS)
ifeq ($(strip $(isp_use2.0)),1)
    LOCAL_PREBUILT_LIBS := oem2v0/isp/libuvdenoise.so
else
    LOCAL_PREBUILT_LIBS := oem/isp/libuvdenoise.so
endif
LOCAL_MODULE_TAGS := optional
include $(BUILD_MULTI_PREBUILT)
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_Y_DENOISE)),true)
include $(CLEAR_VARS)
ifeq ($(strip $(isp_use2.0)),1)
    LOCAL_PREBUILT_LIBS := oem2v0/isp/libynoise.so
else
    LOCAL_PREBUILT_LIBS := oem/isp/libynoise.so
endif
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

ifeq ($(strip $(TARGET_BOARD_CAMERA_EIS)),true)
ifeq ($(strip $(TARGET_ARCH)),arm64)
include $(CLEAR_VARS)
LOCAL_MODULE := libgyrostab
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := libgyrostab.so
LOCAL_MODULE_STEM_64 := libgyrostab.so
LOCAL_SRC_FILES_32 :=  arithmetic/eis/lib32/libgyrostab.so
LOCAL_SRC_FILES_64 :=  arithmetic/eis/lib64/libgyrostab.so
include $(BUILD_PREBUILT)
else
include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := arithmetic/eis/lib32/libgyrostab.so
LOCAL_MODULE_TAGS := optional
include $(BUILD_MULTI_PREBUILT)
endif
endif

ifeq ($(strip $(isp_use2.0)),1)

ifeq ($(strip $(TARGET_ARCH)),arm64)
include $(CLEAR_VARS)
LOCAL_MODULE := libae
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := libae.so
LOCAL_MODULE_STEM_64 := libae.so
LOCAL_SRC_FILES_32 :=  oem2v0/isp/libae.so
LOCAL_SRC_FILES_64 :=  oem2v0/isp/libae_64.so
include $(BUILD_PREBUILT)
else
include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := oem2v0/isp/libae.so
LOCAL_MODULE_TAGS := optional
include $(BUILD_MULTI_PREBUILT)
endif

ifeq ($(strip $(TARGET_ARCH)),arm64)
include $(CLEAR_VARS)
LOCAL_MODULE := libawb
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := libawb.so
LOCAL_MODULE_STEM_64 := libawb.so
LOCAL_SRC_FILES_32 :=  oem2v0/isp/libawb.so
LOCAL_SRC_FILES_64 :=  oem2v0/isp/libawb_64.so
include $(BUILD_PREBUILT)
else
include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := oem2v0/isp/libawb.so
LOCAL_MODULE_TAGS := optional
include $(BUILD_MULTI_PREBUILT)
endif

ifeq ($(strip $(TARGET_ARCH)),arm64)
include $(CLEAR_VARS)
LOCAL_MODULE := libcalibration
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := libcalibration.so
LOCAL_MODULE_STEM_64 := libcalibration.so
LOCAL_SRC_FILES_32 :=  oem2v0/isp/libcalibration.so
LOCAL_SRC_FILES_64 :=  oem2v0/isp/libcalibration_64.so
include $(BUILD_PREBUILT)
else
include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := oem2v0/isp/libcalibration.so
LOCAL_MODULE_TAGS := optional
include $(BUILD_MULTI_PREBUILT)
endif

ifeq ($(strip $(TARGET_ARCH)),arm)
include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := isp2.0/sft_af/lib/libAF.so
LOCAL_MODULE_TAGS := optional
include $(BUILD_MULTI_PREBUILT)

include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := isp2.0/sft_af/lib/libsft_af_ctrl.so
LOCAL_MODULE_TAGS := optional
include $(BUILD_MULTI_PREBUILT)

include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := oem2v0/isp/libdeflicker.so
LOCAL_MODULE_TAGS := optional
include $(BUILD_MULTI_PREBUILT)
endif

ifeq ($(strip $(TARGET_ARCH)),arm64)
include $(CLEAR_VARS)
LOCAL_MODULE := libspaf
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := libspaf.so
LOCAL_MODULE_STEM_64 := libspaf.so
LOCAL_SRC_FILES_32 :=  oem2v0/isp/libspaf.so
LOCAL_SRC_FILES_64 :=  oem2v0/isp/libspaf_64.so
include $(BUILD_PREBUILT)
else
include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := oem2v0/isp/libspaf.so
LOCAL_MODULE_TAGS := optional
include $(BUILD_MULTI_PREBUILT)
endif

ifeq ($(strip $(TARGET_ARCH)),arm64)
include $(CLEAR_VARS)
LOCAL_MODULE := liblsc
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := liblsc.so
LOCAL_MODULE_STEM_64 := liblsc.so
LOCAL_SRC_FILES_32 :=  oem2v0/isp/liblsc.so
LOCAL_SRC_FILES_64 :=  oem2v0/isp/liblsc_64.so
include $(BUILD_PREBUILT)
else
include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := oem2v0/isp/liblsc.so
LOCAL_MODULE_TAGS := optional
include $(BUILD_MULTI_PREBUILT)
endif

endif

