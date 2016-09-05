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

ifeq ($(TARGET_BOARD_CAMERA_HAL_VERSION), $(filter $(TARGET_BOARD_CAMERA_HAL_VERSION),HAL1.0 hal1.0 1.0))
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

ifeq ($(strip $(FRONT_CAMERA_SUPPORT_SIZE)),8M)
LOCAL_CFLAGS += -DCONFIG_FRONT_CAMERA_SUPPORT_8M
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

ifeq ($(strip $(BACK_EXT_CAMERA_SUPPORT_SIZE)),2M)
LOCAL_CFLAGS += -DCONFIG_BACK_EXT_CAMERA_SUPPORT_SIZE_2M
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

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_DIR)),2)
LOCAL_CFLAGS += -DCONFIG_CAMERA_ISP_DIR_2
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

ifeq ($(strip $(TARGET_BOARD_CAMERA_EIS)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_EIS
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_GYRO)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_GYRO
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

ifeq ($(strip $(TARGET_BOARD_FRONT_CAMERA_FLASH_DEV)),true)
LOCAL_CFLAGS += -DCONFIG_FRONT_FLASH_SUPPORT
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

ifeq ($(strip $(TARGET_BOARD_CAMERA_SENSOR_MULTI_INSTANCE_SUPPORT)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_SENSOR_MULTI_INSTANCE_SUPPORT
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
ifdef CAMERA_SENSOR_TYPE_BACK_EXT
LOCAL_CFLAGS += -DCAMERA_SENSOR_TYPE_BACK_EXT=\"$(CAMERA_SENSOR_TYPE_BACK_EXT)\"
else
LOCAL_CFLAGS += -DCAMERA_SENSOR_TYPE_BACK_EXT=\"\\0\"
endif
ifdef CAMERA_SENSOR_TYPE_FRONT_EXT
LOCAL_CFLAGS += -DCAMERA_SENSOR_TYPE_FRONT_EXT=\"$(CAMERA_SENSOR_TYPE_FRONT_EXT)\"
else
LOCAL_CFLAGS += -DCAMERA_SENSOR_TYPE_FRONT_EXT=\"\\0\"
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
ifdef CAMERA_EIS_BOARD_PARAM
LOCAL_CFLAGS += -DCAMERA_EIS_BOARD_PARAM=\"$(CAMERA_EIS_BOARD_PARAM)\"
else
LOCAL_CFLAGS += -DCAMERA_EIS_BOARD_PARAM=\"\\0\"
endif
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_FLASH_LED_0)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_FLASH_LED_0
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_FLASH_LED_1)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_FLASH_LED_1
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_FLASH_LED_SWITCH)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_FLASH_LED_SWITCH
endif

ifeq ($(strip $(TARGET_BOARD_CONFIG_CAMERA_RT_REFOCUS)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_RT_REFOCUS
LOCAL_CFLAGS += -DCONFIG_AE_SYNC_INFO_MAPPING
endif

ifeq ($(strip $(TARGET_BOARD_CONFIG_CAMERA_DUAL_SYNC)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_DUAL_SYNC
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_PIPVIV_SUPPORT)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_PIPVIV_SUPPORT
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_HIGHISO_SUPPORT)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_HIGHISO_SUPPORT
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_SUPPORT_4K2K)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_SUPPORT_4K2K
endif

ifeq ($(strip $(TARGET_BOARD_DUAL_CAMERA_HORIZONTAL)),true)
LOCAL_CFLAGS += -DCONFIG_DUAL_CAMERA_HORIZONTAL
endif

ifeq ($(strip $(LOWPOWER_DISPLAY_30FPS)),true)
LOCAL_CFLAGS += -DLOWPOWER_DISPLAY_30FPS
endif

LOCAL_CFLAGS += -DCHIP_ENDIAN_LITTLE -DJPEG_ENC
