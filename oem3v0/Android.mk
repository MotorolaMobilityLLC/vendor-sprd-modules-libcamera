LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS += -fno-strict-aliasing -Wno-unused-parameter -Werror

TARGET_BOARD_CAMERA_READOTP_METHOD?=0

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_DIR)),3)

LOCAL_C_INCLUDES := \
	$(TARGET_OUT_INTERMEDIATES)/KERNEL/usr/include/video \
	$(LOCAL_PATH)/inc \
	$(LOCAL_PATH)/isp_calibration/inc \
	$(LOCAL_PATH)/../common/inc \
	$(LOCAL_PATH)/../jpeg/inc \
	$(LOCAL_PATH)/../vsp/inc \
	$(LOCAL_PATH)/../tool/mtrace \
	$(LOCAL_PATH)/../arithmetic/facebeauty/inc \
	$(LOCAL_PATH)/../sensor/dummy \
	$(LOCAL_PATH)/../sensor/af_drv \
	$(LOCAL_PATH)/../sensor/otp_drv

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../isp3.0/dummy \
	$(LOCAL_PATH)/../isp3.0/common/inc \
	$(LOCAL_PATH)/../isp3.0/middleware/inc

LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_OUT_INTERMEDIATES)/KERNEL/usr

LOCAL_SRC_FILES+= \
	../vsp/src/jpg_drv_sc8830.c \
	../jpeg/src/jpegcodec_bufmgr.c \
	../jpeg/src/jpegcodec_global.c \
	../jpeg/src/jpegcodec_table.c \
	../jpeg/src/jpegenc_bitstream.c \
	../jpeg/src/jpegenc_frame.c \
	../jpeg/src/jpegenc_header.c \
	../jpeg/src/jpegenc_init.c \
	../jpeg/src/jpegenc_interface.c \
	../jpeg/src/jpegenc_malloc.c \
	../jpeg/src/jpegenc_api.c \
	../jpeg/src/jpegdec_bitstream.c \
	../jpeg/src/jpegdec_frame.c \
	../jpeg/src/jpegdec_init.c \
	../jpeg/src/jpegdec_interface.c \
	../jpeg/src/jpegdec_malloc.c \
	../jpeg/src/jpegdec_dequant.c	\
	../jpeg/src/jpegdec_out.c \
	../jpeg/src/jpegdec_parse.c \
	../jpeg/src/jpegdec_pvld.c \
	../jpeg/src/jpegdec_vld.c \
	../jpeg/src/jpegdec_api.c \
	../jpeg/src/exif_writer.c \
	../jpeg/src/jpeg_stream.c


LOCAL_SRC_FILES+= \
	src/SprdOEMCamera.c \
	src/cmr_common.c \
	src/cmr_oem.c \
	src/cmr_setting.c \
	src/cmr_sensor.c \
	src/cmr_mem.c \
	src/cmr_scale.c \
	src/cmr_rotate.c \
	src/cmr_grab.c \
	src/jpeg_codec.c \
	src/cmr_exif.c \
	src/cmr_preview.c \
	src/cmr_snapshot.c \
	src/cmr_ipm.c \
	src/cmr_focus.c

ifeq ($(strip $(TARGET_BOARD_CAMERA_FACE_DETECT)),true)
	LOCAL_C_INCLUDES += \
		$(LOCAL_PATH)/../arithmetic/sprdface/inc
	LOCAL_SRC_FILES+= src/cmr_fd_sprd.c

endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_EIS)),true)
	LOCAL_C_INCLUDES += \
			$(LOCAL_PATH)/../arithmetic/eis/inc
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_Y_DENOISE)),true)
	LOCAL_C_INCLUDES += $(LOCAL_PATH)/inc/ydenoise_paten
	LOCAL_SRC_FILES+= src/cmr_ydenoise.c
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_HDR_CAPTURE)),true)
	LOCAL_SRC_FILES+= src/cmr_hdr.c
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_UV_DENOISE)),true)
	LOCAL_SRC_FILES+= src/cmr_uvdenoise.c
endif

ifeq ($(strip $(TARGET_BOARD_CONFIG_CAMERA_RT_REFOCUS)),true)
	LOCAL_C_INCLUDES += $(LOCAL_PATH)/sensor/al3200
	LOCAL_SRC_FILES+= src/cmr_refocus.c
endif


LOCAL_CFLAGS += -D_VSP_LINUX_ -D_VSP_

include $(LOCAL_PATH)/../SprdCtrl.mk

LOCAL_MODULE := libcamoem

LOCAL_MODULE_TAGS := optional

LOCAL_SHARED_LIBRARIES += libutils libcutils libcamsensor libcamcommon
LOCAL_SHARED_LIBRARIES += libcamisp

ifeq ($(strip $(TARGET_BOARD_CAMERA_FACE_BEAUTY)),true)
       LOCAL_SHARED_LIBRARIES += libcamfb
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_FACE_DETECT)),true)
	LOCAL_STATIC_LIBRARIES += libsprdfd libsprdfa libsprdfar
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_EIS)),true)
	LOCAL_SHARED_LIBRARIES += libgyrostab
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_GYRO)),true)
	LOCAL_SHARED_LIBRARIES +=libgui
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_HDR_CAPTURE)),true)
	LOCAL_CFLAGS += -DCONFIG_SPRD_HDR_LIB
	LOCAL_SHARED_LIBRARIES += libsprd_easy_hdr
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_UV_DENOISE)),true)
	LOCAL_SHARED_LIBRARIES += libuvdenoise
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_Y_DENOISE)),true)
	LOCAL_SHARED_LIBRARIES += libynoise
endif


ifeq ($(strip $(TARGET_BOARD_CONFIG_CAMERA_RT_REFOCUS)),true)
	LOCAL_SHARED_LIBRARIES += libalRnBLV
endif

include $(BUILD_SHARED_LIBRARY)

endif
