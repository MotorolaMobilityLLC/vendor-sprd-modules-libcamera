
isp_use2.0=1

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/vsp/inc \
	$(LOCAL_PATH)/vsp/src \
	$(LOCAL_PATH)/sensor \
	$(LOCAL_PATH)/jpeg/inc \
	$(LOCAL_PATH)/jpeg/src \
	$(LOCAL_PATH)/common/inc \
	$(LOCAL_PATH)/hal1.0/inc \
	$(LOCAL_PATH)/tool/auto_test/inc \
	$(LOCAL_PATH)/tool/mtrace \
	external/skia/include/images \
	external/skia/include/core\
	external/jhead \
	external/sqlite/dist \
	system/media/camera/include \
	$(TOP)vendor/sprd/modules/libmemion \
	$(TOP)/vendor/sprd/external/kernel-headers \
	$(TOP)/vendor/sprd/modules/libmemion \
	$(TARGET_OUT_INTERMEDIATES)/KERNEL/source/include/video \
	$(TARGET_OUT_INTERMEDIATES)/KERNEL/source/include/uapi


ifeq ($(strip $(TARGET_GPU_PLATFORM)),midgard)
LOCAL_C_INCLUDES += vendor/sprd/proprietories-source/libgpu/gralloc/midgard
else ifeq ($(strip $(TARGET_GPU_PLATFORM)),utgard)
LOCAL_C_INCLUDES += vendor/sprd/proprietories-source/libgpu/gralloc/utgard
else
LOCAL_C_INCLUDES += hardware/libhardware/modules/gralloc/
endif

include $(shell find $(LOCAL_PATH) -name 'Sprdroid.mk')

ifeq ($(strip $(TARGET_BOARD_CAMERA_MEMORY_OPTIMIZATION)),true)
LOCAL_CFLAGS += -DCONFIG_MEM_OPTIMIZATION
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_DIR)),3)
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/isp3.0/dummy \
	$(LOCAL_PATH)/isp3.0/common/inc \
	$(LOCAL_PATH)/isp3.0/middleware/inc
endif

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/oem2v0/inc \
    $(LOCAL_PATH)/oem2v0/isp_calibration/inc

LOCAL_SRC_FILES+= \
	hal1.0/src/SprdCameraParameters.cpp \
	common/src/cmr_msg.c \
	vsp/src/jpg_drv_sc8830.c \
	jpeg/src/jpegcodec_bufmgr.c \
	jpeg/src/jpegcodec_global.c \
	jpeg/src/jpegcodec_table.c \
	jpeg/src/jpegenc_bitstream.c \
	jpeg/src/jpegenc_frame.c \
	jpeg/src/jpegenc_header.c \
	jpeg/src/jpegenc_init.c \
	jpeg/src/jpegenc_interface.c \
	jpeg/src/jpegenc_malloc.c \
	jpeg/src/jpegenc_api.c \
	jpeg/src/jpegdec_bitstream.c \
	jpeg/src/jpegdec_frame.c \
	jpeg/src/jpegdec_init.c \
	jpeg/src/jpegdec_interface.c \
	jpeg/src/jpegdec_malloc.c \
	jpeg/src/jpegdec_dequant.c	\
	jpeg/src/jpegdec_out.c \
	jpeg/src/jpegdec_parse.c \
	jpeg/src/jpegdec_pvld.c \
	jpeg/src/jpegdec_vld.c \
	jpeg/src/jpegdec_api.c  \
	jpeg/src/exif_writer.c  \
	jpeg/src/jpeg_stream.c \
	tool/mtrace/mtrace.c


LOCAL_SRC_FILES+= \
	oem2v0/src/SprdOEMCamera.c \
	oem2v0/src/cmr_common.c \
	oem2v0/src/cmr_oem.c \
	oem2v0/src/cmr_setting.c \
	oem2v0/src/cmr_mem.c \
	oem2v0/src/cmr_scale.c \
	oem2v0/src/cmr_rotate.c \
	oem2v0/src/cmr_grab.c \
	oem2v0/src/jpeg_codec.c \
	oem2v0/src/cmr_exif.c \
	oem2v0/src/sensor_cfg.c \
	oem2v0/src/cmr_preview.c \
	oem2v0/src/cmr_snapshot.c \
	oem2v0/src/cmr_sensor.c \
	oem2v0/src/cmr_ipm.c \
	oem2v0/src/cmr_focus.c \
	oem2v0/src/sensor_drv_u.c \
	oem2v0/isp_calibration/src/isp_calibration.c \
	oem2v0/isp_calibration/src/isp_cali_interface.c

#include $(LOCAL_PATH)/isp2.0/isp2_0.mk
LOCAL_SRC_FILES+= \
	sensor/ov5640/sensor_ov5640_mipi.c \
	sensor/ov5640/sensor_ov5640_mipi_raw.c \
	sensor/s5k3p3sm/sensor_s5k3p3sm_mipi_raw.c \
	sensor/imx230/sensor_imx230_mipi_raw.c \
	sensor/ov2680/sensor_ov2680_mipi_raw.c \
	sensor/af_bu64297gwz.c \
	sensor/s5k4h8yx/sensor_s5k4h8yx_mipi_raw.c

ifeq ($(strip $(TARGET_CAMERA_OIS_FUNC)),true)
	LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/sensor/ois/

	LOCAL_SRC_FILES+= \
	sensor/ois/OIS_func.c \
	sensor/ois/OIS_user.c \
	sensor/ois/OIS_main.c
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_FACE_DETECT)),true)
ifeq ($(strip $(TARGET_BOARD_CAMERA_FD_LIB)),omron)
	LOCAL_C_INCLUDES += \
			    $(LOCAL_PATH)/arithmetic/omron/inc
	LOCAL_SRC_FILES+= oem2v0/src/cmr_fd_omron.c
else
	LOCAL_SRC_FILES+= oem2v0/src/cmr_fd.c
endif
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_HDR_CAPTURE)),true)
LOCAL_SRC_FILES+= oem2v0/src/cmr_hdr.c
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_UV_DENOISE)),true)
LOCAL_SRC_FILES+= oem2v0/src/cmr_uvdenoise.c
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_Y_DENOISE)),true)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/oem2v0/inc/ydenoise_paten
LOCAL_SRC_FILES+= oem2v0/src/cmr_ydenoise.c
endif

ifeq ($(TARGET_BOARD_CAMERA_HAL_VERSION), $(filter $(TARGET_BOARD_CAMERA_HAL_VERSION), HAL1.0 hal1.0 1.0))
LOCAL_SRC_FILES += hal1.0/src/SprdCameraHardwareInterface.cpp
else
LOCAL_SRC_FILES+= \
        hal3/SprdCamera3Factory.cpp \
        hal3/SprdCamera3Hal.cpp \
        hal3/SprdCamera3HWI.cpp \
        hal3/SprdCamera3Channel.cpp \
	hal3/SprdCamera3Mem.cpp \
	hal3/SprdCamera3OEMIf.cpp \
	hal3/SprdCamera3Setting.cpp \
	hal3/SprdCamera3Stream.cpp
endif


LOCAL_CFLAGS += -fno-strict-aliasing -D_VSP_ -DJPEG_ENC -D_VSP_LINUX_ -DCHIP_ENDIAN_LITTLE -Wno-unused-parameter

ifeq ($(strip $(TARGET_BOARD_IS_SC_FPGA)),true)
LOCAL_CFLAGS += -DSC_FPGA=1
else
LOCAL_CFLAGS += -DSC_FPGA=0
endif

ifeq ($(strip $(TARGET_BOARD_SC_IOMMU_PF)),1)
LOCAL_CFLAGS += -DSC_IOMMU_PF=1
endif

ifeq ($(strip $(TARGET_CAMERA_OIS_FUNC)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_OIS_FUNC
endif

ifeq ($(TARGET_BOARD_CAMERA_HAL_VERSION), $(filter $(TARGET_BOARD_CAMERA_HAL_VERSION), hal1.0 1.0))
LOCAL_CFLAGS += -DCONFIG_CAMERA_HAL_VERSION_1
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_SOFTWARE_VERSION)),0)
LOCAL_CFLAGS += -DCONFIG_SP7731GEA_BOARD
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_SOFTWARE_VERSION)),1)
LOCAL_CFLAGS += -DCONFIG_SP9630EA_BOARD
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_SOFTWARE_VERSION)),2)
LOCAL_CFLAGS += -DCONFIG_CAMERA_ISP_VERSION_V3
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_SOFTWARE_VERSION)),3)
LOCAL_CFLAGS += -DCONFIG_CAMERA_ISP_VERSION_V4
endif

ifeq ($(strip $(TARGET_BOARD_PLATFORM)),scx15)
LOCAL_CFLAGS += -DCONFIG_CAMERA_SMALL_PREVSIZE
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_FLASH_CTRL)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_FLASH_CTRL
endif

ifeq ($(strip $(CAMERA_SUPPORT_SIZE)),21M)
LOCAL_CFLAGS += -DCONFIG_CAMERA_SUPPORT_21M
endif

ifeq ($(strip $(CAMERA_SUPPORT_SIZE)),16M)
LOCAL_CFLAGS += -DCONFIG_CAMERA_SUPPORT_16M
endif

ifeq ($(strip $(CAMERA_SUPPORT_SIZE)),13M)
LOCAL_CFLAGS += -DCONFIG_CAMERA_SUPPORT_13M
endif

ifeq ($(strip $(CAMERA_SUPPORT_SIZE)),8M)
LOCAL_CFLAGS += -DCONFIG_CAMERA_SUPPORT_8M
endif

ifeq ($(strip $(CAMERA_SUPPORT_SIZE)),5M)
LOCAL_CFLAGS += -DCONFIG_CAMERA_SUPPORT_5M
endif

ifeq ($(strip $(CAMERA_SUPPORT_SIZE)),3M)
LOCAL_CFLAGS += -DCONFIG_CAMERA_SUPPORT_3M
endif

ifeq ($(strip $(CAMERA_SUPPORT_SIZE)),2M)
LOCAL_CFLAGS += -DCONFIG_CAMERA_SUPPORT_2M
endif

ifeq ($(strip $(FRONT_CAMERA_SUPPORT_SIZE)),5M)
LOCAL_CFLAGS += -DCONFIG_FRONT_CAMERA_SUPPORT_5M
endif

ifeq ($(strip $(FRONT_CAMERA_SUPPORT_SIZE)),3M)
LOCAL_CFLAGS += -DCONFIG_FRONT_CAMERA_SUPPORT_3M
endif

ifeq ($(strip $(FRONT_CAMERA_SUPPORT_SIZE)),2M)
LOCAL_CFLAGS += -DCONFIG_FRONT_CAMERA_SUPPORT_2M
endif

ifeq ($(strip $(TARGET_BOARD_NO_FRONT_SENSOR)),true)
LOCAL_CFLAGS += -DCONFIG_DCAM_SENSOR_NO_FRONT_SUPPORT
endif

ifeq ($(strip $(TARGET_BOARD_SENSOR2_SUPPORT)),true)
LOCAL_CFLAGS += -DCONFIG_DCAM_SENSOR2_SUPPORT
endif

ifeq ($(strip $(TARGET_BOARD_SENSOR3_SUPPORT)),true)
LOCAL_CFLAGS += -DCONFIG_DCAM_SENSOR3_SUPPORT
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_NO_USE_ISP)),true)
else
LOCAL_CFLAGS += -DCONFIG_CAMERA_ISP
endif

LOCAL_CFLAGS += -DCONFIG_CAMERA_PREVIEW_YV12

ifeq ($(strip $(TARGET_BOARD_CAMERA_CAPTURE_MODE)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_ZSL_CAPTURE
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_BIG_PREVIEW_AND_RECORD_SIZE)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_BIG_PREVIEW_RECORD_SIZE
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_FORCE_ZSL_MODE)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_FORCE_ZSL_CAPTURE
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ANDROID_ZSL_MODE)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_ANDROID_ZSL_CAPTURE
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ANTI_FLICKER)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_AFL_AUTO_DETECTION
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_CAF)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_CAF
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_AF_ALG_SPRD)),true)
LOCAL_CFLAGS += -DCONFIG_NEW_AF
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ROTATION_CAPTURE)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_ROTATION_CAPTURE
endif

ifeq ($(strip $(TARGET_BOARD_FRONT_CAMERA_ROTATION)),true)
LOCAL_CFLAGS += -DCONFIG_FRONT_CAMERA_ROTATION
endif

ifeq ($(strip $(TARGET_BOARD_BACK_CAMERA_ROTATION)),true)
LOCAL_CFLAGS += -DCONFIG_BACK_CAMERA_ROTATION
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ROTATION)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_ROTATION
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ANTI_SHAKE)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_ANTI_SHAKE
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_DMA_COPY)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_DMA_COPY
endif

ifeq ($(strip $(TARGET_BOARD_BACK_CAMERA_INTERFACE)),mipi)
LOCAL_CFLAGS += -DCONFIG_BACK_CAMERA_MIPI
endif

ifeq ($(strip $(TARGET_BOARD_BACK_CAMERA_INTERFACE)),ccir)
LOCAL_CFLAGS += -DCONFIG_BACK_CAMERA_CCIR
endif

ifeq ($(strip $(TARGET_BOARD_FRONT_CAMERA_INTERFACE)),mipi)
LOCAL_CFLAGS += -DCONFIG_FRONT_CAMERA_MIPI
endif

ifeq ($(strip $(TARGET_BOARD_FRONT_CAMERA_INTERFACE)),ccir)
LOCAL_CFLAGS += -DCONFIG_FRONT_CAMERA_CCIR
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_SUPPORT_720P)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_SUPPORT_720P
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_SUPPORT_CIF)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_SUPPORT_CIF
endif

ifeq ($(strip $(CAMERA_DISP_ION)),true)
LOCAL_CFLAGS += -DUSE_ION_MEM
endif

ifeq ($(strip $(CAMERA_SENSOR_OUTPUT_ONLY)),true)
LOCAL_CFLAGS += -DCONFIG_SENSOR_OUTPUT_ONLY
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_FACE_DETECT)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_FACE_DETECT
ifeq ($(strip $(TARGET_BOARD_CAMERA_FD_LIB)),omron)
LOCAL_CFLAGS += -DCONFIG_CAMERA_FACE_DETECT_OMRON
else
LOCAL_CFLAGS += -DCONFIG_CAMERA_FACE_DETECT_FD
endif
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_HDR_CAPTURE)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_HDR_CAPTURE
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_UV_DENOISE)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_UV_DENOISE
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_Y_DENOISE)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_Y_DENOISE
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_NO_FLASH_DEV)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_FLASH_NOT_SUPPORT
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_NO_AUTOFOCUS_DEV)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_NO_720P_PREVIEW)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_NO_720P_PREVIEW
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ADAPTER_IMAGE)),180)
LOCAL_CFLAGS += -DCONFIG_CAMERA_IMAGE_180
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_PRE_ALLOC_CAPTURE_MEM)),true)
LOCAL_CFLAGS += -DCONFIG_PRE_ALLOC_CAPTURE_MEM
endif

ifeq ($(strip $(TARGET_BOARD_FRONT_CAMERA_MIPI)),phya)
LOCAL_CFLAGS += -DCONFIG_FRONT_CAMERA_MIPI_PHYA
endif

ifeq ($(strip $(TARGET_BOARD_FRONT_CAMERA_MIPI)),phyb)
LOCAL_CFLAGS += -DCONFIG_FRONT_CAMERA_MIPI_PHYB
endif

ifeq ($(strip $(TARGET_BOARD_FRONT_CAMERA_MIPI)),phyc)
LOCAL_CFLAGS += -DCONFIG_FRONT_CAMERA_MIPI_PHYC
endif

ifeq ($(strip $(TARGET_BOARD_FRONT_CAMERA_MIPI)),phyab)
LOCAL_CFLAGS += -DCONFIG_FRONT_CAMERA_MIPI_PHYAB
endif

ifeq ($(strip $(TARGET_BOARD_BACK_CAMERA_MIPI)),phya)
LOCAL_CFLAGS += -DCONFIG_BACK_CAMERA_MIPI_PHYA
endif

ifeq ($(strip $(TARGET_BOARD_BACK_CAMERA_MIPI)),phyb)
LOCAL_CFLAGS += -DCONFIG_BACK_CAMERA_MIPI_PHYB
endif

ifeq ($(strip $(TARGET_BOARD_BACK_CAMERA_MIPI)),phyab)
LOCAL_CFLAGS += -DCONFIG_BACK_CAMERA_MIPI_PHYAB
endif


ifeq ($(strip $(TARGET_BOARD_FRONT_CAMERA_CCIR_PCLK)),source0)
LOCAL_CFLAGS += -DCONFIG_FRONT_CAMERA_CCIR_PCLK_SOURCE0
endif

ifeq ($(strip $(TARGET_BOARD_FRONT_CAMERA_CCIR_PCLK)),source1)
LOCAL_CFLAGS += -DCONFIG_FRONT_CAMERA_CCIR_PCLK_SOURCE1
endif

ifeq ($(strip $(TARGET_BOARD_BACK_CAMERA_CCIR_PCLK)),source0)
LOCAL_CFLAGS += -DCONFIG_BACK_CAMERA_CCIR_PCLK_SOURCE0
endif

ifeq ($(strip $(TARGET_BOARD_BACK_CAMERA_CCIR_PCLK)),source1)
LOCAL_CFLAGS += -DCONFIG_BACK_CAMERA_CCIR_PCLK_SOURCE1
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_CAPTURE_DENOISE)),true)
LOCAL_CFLAGS += -DCONFIG_CAPTURE_DENOISE
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_NO_EXPOSURE_METERING)),true)
LOCAL_CFLAGS += -DCONFIG_EXPOSURE_METERING_NOT_SUPPORT
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISO_NOT_SUPPORT)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_ISO_NOT_SUPPORT
endif

ifeq ($(strip $(TARGET_BOARD_LOW_CAPTURE_MEM)),true)
LOCAL_CFLAGS += -DCONFIG_LOW_CAPTURE_MEM
endif

ifeq ($(strip $(TARGET_BOARD_MULTI_CAP_MEM)),true)
LOCAL_CFLAGS += -DCONFIG_MULTI_CAP_MEM
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_FULL_SCREEN_DISPLAY)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_FULL_SCREEN_DISPLAY
endif



LOCAL_CFLAGS += -DCONFIG_READOTP_METHOD=$(TARGET_BOARD_CAMERA_READOTP_METHOD)


ifdef CAMERA_SENSOR_TYPE_BACK
LOCAL_CFLAGS += -DCAMERA_SENSOR_TYPE_BACK=\"$(CAMERA_SENSOR_TYPE_BACK)\"
else
LOCAL_CFLAGS += -DCAMERA_SENSOR_TYPE_BACK=\"\\0\"
endif
ifdef CAMERA_SENSOR_TYPE_FRONT
LOCAL_CFLAGS += -DCAMERA_SENSOR_TYPE_FRONT=\"$(CAMERA_SENSOR_TYPE_FRONT)\"
else
LOCAL_CFLAGS += -DCAMERA_SENSOR_TYPE_FRONT=\"\\0\"
endif
ifdef AT_CAMERA_SENSOR_TYPE_BACK
LOCAL_CFLAGS += -DAT_CAMERA_SENSOR_TYPE_BACK=\"$(AT_CAMERA_SENSOR_TYPE_BACK)\"
else
LOCAL_CFLAGS += -DAT_CAMERA_SENSOR_TYPE_BACK=\"\\0\"
endif
ifdef AT_CAMERA_SENSOR_TYPE_FRONT
LOCAL_CFLAGS += -DAT_CAMERA_SENSOR_TYPE_FRONT=\"$(AT_CAMERA_SENSOR_TYPE_FRONT)\"
else
LOCAL_CFLAGS += -DAT_CAMERA_SENSOR_TYPE_FRONT=\"\\0\"
endif

ifeq ($(strip $(TARGET_BOOTLOADER_BOARD_NAME)),ss_sharklt8)
LOCAL_CFLAGS += -DCONFIG_NEW_AF
endif

ifeq ($(strip $(TARGET_GPU_PLATFORM)),midgard)
LOCAL_CFLAGS += -DCONFIG_GPU_MIDGARD
endif

ifeq ($(strip $(TARGET_VCM_BU64241GWZ)),true)
LOCAL_CFLAGS += -DCONFIG_VCM_BU64241GWZ
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_FACE_BEAUTY)),false)
else
LOCAL_CFLAGS += -DCONFIG_FACE_BEAUTY
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_EIS)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_EIS
endif
