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

ifdef CAMERA_SENSOR_TYPE_DEV_2
LOCAL_CFLAGS += -DCAMERA_SENSOR_TYPE_DEV_2=\"$(CAMERA_SENSOR_TYPE_DEV_2)\"
else
LOCAL_CFLAGS += -DCAMERA_SENSOR_TYPE_DEV_2=\"\\0\"
endif

ifdef AT_CAMERA_SENSOR_TYPE_DEV_2
LOCAL_CFLAGS += -DAT_CAMERA_SENSOR_TYPE_DEV_2=\"$(AT_CAMERA_SENSOR_TYPE_DEV_2)\"
else
LOCAL_CFLAGS += -DAT_CAMERA_SENSOR_TYPE_DEV_2=\"\\0\"
endif


ifeq ($(TARGET_BOARD_CAMERA_HAL_VERSION), $(filter $(TARGET_BOARD_CAMERA_HAL_VERSION),HAL1.0 hal1.0 1.0))
LOCAL_CFLAGS += -DCONFIG_CAMERA_HAL_VERSION_1
endif





ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_AE_VERSION)),1)
LOCAL_CFLAGS += -DCONFIG_CAMERA_ISP_AE_VERSION_V1
else
LOCAL_CFLAGS += -DCONFIG_CAMERA_ISP_AE_VERSION_V0
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_NO_DCAM_DATA_PATH)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_NO_DCAM_DATA_PATH
endif

ifeq ($(strip $(TARGET_BOARD_PLATFORM)),scx15)
LOCAL_CFLAGS += -DCONFIG_CAMERA_SMALL_PREVSIZE
endif

ifeq ($(strip $(TARGET_BOARD_SHARKLE_BRINGUP)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_SHARKLE_BRINGUP
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_FLASH_CTRL)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_FLASH_CTRL
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_SENSOR_BACK_I2C_SWITCH)),true)
LOCAL_CFLAGS += -DCAMERA_SENSOR_BACK_I2C_SWITCH
endif

ifeq ($(strip $(CAMERA_DUAL)),true)
LOCAL_CFLAGS += -DCONFIG_DUAL_CAMERA
endif

ifeq ($(strip $(CAMERA_SUPPORT_SIZE)),21M)
LOCAL_CFLAGS += -DCONFIG_CAMERA_SUPPORT_21M
endif

ifeq ($(strip $(CAMERA_SUPPORT_SIZE)),20M)
LOCAL_CFLAGS += -DCONFIG_CAMERA_SUPPORT_20M
endif

ifeq ($(strip $(CAMERA_SUPPORT_SIZE)),16M)
LOCAL_CFLAGS += -DCONFIG_CAMERA_SUPPORT_16M
endif

ifeq ($(strip $(CAMERA_SUPPORT_SIZE)),13M)
LOCAL_CFLAGS += -DCONFIG_CAMERA_SUPPORT_13M
endif

ifeq ($(strip $(CAMERA_SUPPORT_SIZE)),12M)
LOCAL_CFLAGS += -DCONFIG_CAMERA_SUPPORT_12M
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

ifeq ($(strip $(CAMERA_SUPPORT_SIZE)),2M_1080P)
LOCAL_CFLAGS += -DCONFIG_CAMERA_SUPPORT_2M_1080P
endif

ifeq ($(strip $(CAMERA_SUPPORT_SIZE)),2M)
LOCAL_CFLAGS += -DCONFIG_CAMERA_SUPPORT_2M
endif

ifeq ($(strip $(CAMERA_SUPPORT_SIZE)),0M3)
LOCAL_CFLAGS += -DCONFIG_CAMERA_SUPPORT_0M3
endif

ifeq ($(strip $(FRONT_CAMERA_SUPPORT_SIZE)),21M)
LOCAL_CFLAGS += -DCONFIG_FRONT_CAMERA_SUPPORT_21M
endif

ifeq ($(strip $(FRONT_CAMERA_SUPPORT_SIZE)),20M)
LOCAL_CFLAGS += -DCONFIG_FRONT_CAMERA_SUPPORT_20M
endif

ifeq ($(strip $(FRONT_CAMERA_SUPPORT_SIZE)),16M)
LOCAL_CFLAGS += -DCONFIG_FRONT_CAMERA_SUPPORT_16M
endif

ifeq ($(strip $(FRONT_CAMERA_SUPPORT_SIZE)),13M)
LOCAL_CFLAGS += -DCONFIG_FRONT_CAMERA_SUPPORT_13M
endif

ifeq ($(strip $(FRONT_CAMERA_SUPPORT_SIZE)),12M)
LOCAL_CFLAGS += -DCONFIG_FRONT_CAMERA_SUPPORT_12M
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

ifeq ($(strip $(FRONT_CAMERA_SUPPORT_SIZE)),2M_1080P)
LOCAL_CFLAGS += -DCONFIG_FRONT_CAMERA_SUPPORT_2M_1080P
endif

ifeq ($(strip $(FRONT_CAMERA_SUPPORT_SIZE)),2M)
LOCAL_CFLAGS += -DCONFIG_FRONT_CAMERA_SUPPORT_2M
endif

ifeq ($(strip $(FRONT_CAMERA_SUPPORT_SIZE)),0M3)
LOCAL_CFLAGS += -DCONFIG_FRONT_CAMERA_SUPPORT_0M3
endif


ifeq ($(strip $(BACK_EXT_CAMERA_SUPPORT_SIZE)),5M)
LOCAL_CFLAGS += -DCONFIG_BACK_EXT_CAMERA_SUPPORT_SIZE_5M
endif

ifeq ($(strip $(BACK_EXT_CAMERA_SUPPORT_SIZE)),2M_1080P)
LOCAL_CFLAGS += -DCONFIG_BACK_EXT_CAMERA_SUPPORT_SIZE_2M_1080P
endif

ifeq ($(strip $(BACK_EXT_CAMERA_SUPPORT_SIZE)),2M)
LOCAL_CFLAGS += -DCONFIG_BACK_EXT_CAMERA_SUPPORT_SIZE_2M
endif

ifeq ($(strip $(BACK_EXT_CAMERA_SUPPORT_SIZE)),0M3)
LOCAL_CFLAGS += -DCONFIG_BACK_EXT_CAMERA_SUPPORT_SIZE_0M3
endif

max_sensor_num := 0
ifneq ($(strip $(CAMERA_SENSOR_TYPE_BACK)),)
LOCAL_CFLAGS += -DBACK_CAMERA_SENSOR_SUPPORT=1
max_sensor_num := $(shell expr $(max_sensor_num) + 1)
else
LOCAL_CFLAGS += -DBACK_CAMERA_SENSOR_SUPPORT=0
endif

ifneq ($(strip $(CAMERA_SENSOR_TYPE_FRONT)),)
LOCAL_CFLAGS += -DFRONT_CAMERA_SENSOR_SUPPORT=1
max_sensor_num := $(shell expr $(max_sensor_num) + 1)
else
LOCAL_CFLAGS += -DFRONT_CAMERA_SENSOR_SUPPORT=0
endif

ifneq ($(strip $(CAMERA_SENSOR_TYPE_BACK_EXT)),)
LOCAL_CFLAGS += -DBACK_EXT_CAMERA_SENSOR_SUPPORT=1
max_sensor_num := $(shell expr $(max_sensor_num) + 1)
else
LOCAL_CFLAGS += -DBACK_EXT_CAMERA_SENSOR_SUPPORT=0
endif

ifneq ($(strip $(CAMERA_SENSOR_TYPE_FRONT_EXT)),)
LOCAL_CFLAGS += -DFRONT_EXT_CAMERA_SENSOR_SUPPORT=1
max_sensor_num := $(shell expr $(max_sensor_num) + 1)
else
LOCAL_CFLAGS += -DFRONT_EXT_CAMERA_SENSOR_SUPPORT=0
endif
LOCAL_CFLAGS += -DCAMERA_SENSOR_NUM=$(max_sensor_num)

ifeq ($(strip $(TARGET_BOARD_CAMERA_POWER_OPTIMIZATION)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_POWER_OPTIMIZATION
endif

ifdef CONFIG_CAMERA_DFS_FIXED_MAXLEVEL
LOCAL_CFLAGS += -DCONFIG_CAMERA_DFS_FIXED_MAXLEVEL=$(CONFIG_CAMERA_DFS_FIXED_MAXLEVEL)
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_POWERHINT_LOWPOWER)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_POWERHINT_LOWPOWER
ifeq ($(strip $(TARGET_BOARD_CAMERA_POWERHINT_LOWPOWER_BINDCORE)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_POWERHINT_ACQUIRECORE
endif
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_POWERHINT_PERFORMANCE)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_POWERHINT_PERFORMANCE
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_NO_USE_ISP)),true)
else
LOCAL_CFLAGS += -DCONFIG_CAMERA_ISP
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_VERSION)),2.1)
LOCAL_CFLAGS += -DCONFIG_ISP_2_1
else ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_VERSION)),2.2)
LOCAL_CFLAGS += -DCONFIG_ISP_2_2
else ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_VERSION)),2.3)
LOCAL_CFLAGS += -DCONFIG_ISP_2_3
else ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_VERSION)),2.4)
LOCAL_CFLAGS += -DCONFIG_ISP_2_4
else ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_VERSION)),2.5)
LOCAL_CFLAGS += -DCONFIG_ISP_2_5
endif

LOCAL_CFLAGS += -DCONFIG_CAMERA_PREVIEW_YV12

ifeq ($(strip $(TARGET_BOARD_CAMERA_CAPTURE_MODE)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_ZSL_CAPTURE
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_CAPTURE_NOZSL)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_CAPTURE_NOZSL
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

ifeq ($(strip $(TARGET_BOARD_CAMERA_MIRROR_TYPE)),dcam)
LOCAL_CFLAGS += -DCONFIG_CAMERA_MIRROR_DCAM
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_MIRROR_TYPE)),jpg)
LOCAL_CFLAGS += -DCONFIG_CAMERA_MIRROR_JPG
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_MIRROR_TYPE)),cpp)
LOCAL_CFLAGS += -DCONFIG_CAMERA_MIRROR_CPP
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

ifeq ($(strip $(TARGET_BOARD_CAMERA_SUPPORT_720P)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_SUPPORT_720P
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_SUPPORT_CIF)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_SUPPORT_CIF
endif

ifeq ($(strip $(TARGET_BOARD_COVERED_SENSOR_SUPPORT)),true)
LOCAL_CFLAGS += -DCONFIG_COVERED_SENSOR
endif

ifeq ($(strip $(TARGET_BOARD_BLUR_MODE_SUPPORT)),true)
LOCAL_CFLAGS += -DCONFIG_BLUR_SUPPORT
endif

ifeq ($(strip $(TARGET_BOARD_BOKEH_MODE_SUPPORT)),sbs)
LOCAL_CFLAGS += -DCONFIG_SIDEBYSIDE_SUPPORT
endif

ifeq ($(strip $(TARGET_BOARD_SBS_MODE_SENSOR)),true)
LOCAL_CFLAGS += -DSBS_MODE_SENSOR
endif

ifeq ($(strip $(TARGET_BOARD_SBS_SENSOR_FRONT)),true)
LOCAL_CFLAGS += -DSBS_SENSOR_FRONT
endif

ifeq ($(strip $(TARGET_BOARD_FACE_UNLOCK_SUPPORT)),true)
LOCAL_CFLAGS += -DCONFIG_SINGLE_FACEID_SUPPORT
endif

ifeq ($(strip $(TARGET_BOARD_DUAL_FACE_UNLOCK_SUPPORT)),true)
LOCAL_CFLAGS += -DCONFIG_DUAL_FACEID_SUPPORT
endif

ifeq ($(strip $(TARGET_BOARD_BOKEH_MODE_SUPPORT)),true)
LOCAL_CFLAGS += -DCONFIG_BOKEH_SUPPORT
LOCAL_CFLAGS += -DCONFIG_SPRD_BOKEH_SUPPORT
else ifeq ($(strip $(TARGET_BOARD_ARCSOFT_BOKEH_MODE_SUPPORT)),true)
LOCAL_CFLAGS += -DCONFIG_BOKEH_SUPPORT
LOCAL_CFLAGS += -DCONFIG_ARCSOFT_BOKEH_SUPPORT
endif

ifeq ($(strip $(TARGET_BOARD_NEED_SR_ENABLE)),true)
LOCAL_CFLAGS += -DCONFIG_SUPERRESOLUTION_ENABLED
endif

ifeq ($(strip $(CAMERA_DISP_ION)),true)
LOCAL_CFLAGS += -DUSE_ION_MEM
endif

ifeq ($(strip $(CAMERA_SENSOR_OUTPUT_ONLY)),true)
LOCAL_CFLAGS += -DCONFIG_SENSOR_OUTPUT_ONLY
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_FACE_DETECT)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_FACE_DETECT
LOCAL_CFLAGS += -DCONFIG_CAMERA_FACE_DETECT_SPRD
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_EIS)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_EIS
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_GYRO)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_GYRO
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_4IN1)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_4IN1
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_HDR_CAPTURE)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_HDR_CAPTURE
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_SUPPORT_AUTO_HDR)),true)
LOCAL_CFLAGS += -DCONFIG_SUPPROT_AUTO_HDR
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_3DNR_CAPTURE)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_3DNR_CAPTURE
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_3DNR_CAPTURE_GPU)),true)
LOCAL_CFLAGS += -DCAMERA_3DNR_CAPTURE_GPU
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_SUPPORT_AI_SCENE)),true)
LOCAL_CFLAGS += -DCONFIG_SUPPROT_AI_SCENE
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

ifeq ($(strip $(TARGET_BOARD_FRONT_CAMERA_FLASH_LED_0)),true)
LOCAL_CFLAGS += -DCONFIG_FRONT_CAMERA_FLASH_LED_0
endif

ifneq ($(strip $(TARGET_BOARD_CAMERA_AUTOFOCUS)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
endif

ifeq ($(strip $(TARGET_BOARD_FRONT_CAMERA_AUTOFOCUS)),true)
LOCAL_CFLAGS += -DCONFIG_FRONT_CAMERA_AUTOFOCUS
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

ifeq ($(strip $(TARGET_BOARD_AF_VCM_DW9800W)),true)
LOCAL_CFLAGS += -DCONFIG_AF_VCM_DW9800W
endif


ifeq ($(strip $(TARGET_BOARD_AF_VCM_9714)),true)
LOCAL_CFLAGS += -DCONFIG_AF_VCM_DW9714
endif

ifeq ($(strip $(TARGET_BOARD_PLATFORM)),sp9853i)
ifeq ($(CAMERA_SENSOR_TYPE_BACK), $(filter $(CAMERA_SENSOR_TYPE_BACK),"ov13855,ov13855a" "ov13855" "ov13855a"))
LOCAL_CFLAGS += -DCAMERA_SERNSOR_SUPPORT_4224
endif
endif

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

ifeq ($(strip $(TARGET_BOARD_CAMERA_FLASH_TYPE)),ocp8137)
LOCAL_CFLAGS += -DCONFIG_CAMERA_FLASH_OCP8137
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

ifeq ($(strip $(TARGET_BOARD_CAMERA_FACE_BEAUTY)),true)
LOCAL_CFLAGS += -DCONFIG_FACE_BEAUTY
endif

ifdef TARGET_BOARD_CAMERA_PDAF_TYPE
LOCAL_CFLAGS += -DCONFIG_CAMERA_PDAF_TYPE=$(TARGET_BOARD_CAMERA_PDAF_TYPE)
endif

ifdef TARGET_BOARD_CAMERA_DCAM_PDAF
LOCAL_CFLAGS += -DCONFIG_CAMERA_DCAM_PDAF
endif

ifeq ($(strip $(TARGET_BOARD_ISP_PDAF_EXTRACTOR)),true)
LOCAL_CFLAGS += -DCONFIG_ISP_PDAF_EXTRACTOR
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_3DPREVIEW_NO_FACE_BEAUTY)),true)
LOCAL_CFLAGS += -DCONFIG_3DPREVIEW_NO_FACE_BEAUTY
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

ifeq ($(strip $(TARGET_BOARD_CAMERA_SUPPORT_4K_RECORD)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_SUPPORT_4K_RECORD
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_SUPPORT_4K2K)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_SUPPORT_4K2K
endif

ifeq ($(strip $(TARGET_BOARD_DUAL_CAMERA_HORIZONTAL)),true)
LOCAL_CFLAGS += -DCONFIG_DUAL_CAMERA_HORIZONTAL
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_DUAL_SENSOR_MODULE)),true)
LOCAL_CFLAGS += -DCONFIG_DUAL_MODULE
endif

ifeq ($(strip $(TARGET_DISABLE_DUAL_CAMERA_FRAMESYNC)),true)
LOCAL_CFLAGS += -DCONFIG_DISABLE_DUAL_CAMERA_FRAMESYNC
endif

ifeq ($(strip $(TARGET_SENSOR_LOWPOWER_MODE)),true)
LOCAL_CFLAGS += -DCONFIG_SENSOR_LOWPOWER_MODE
endif

ifeq ($(strip $(LOWPOWER_DISPLAY_30FPS)),true)
LOCAL_CFLAGS += -DLOWPOWER_DISPLAY_30FPS
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_DCAM_SUPPORT_FORMAT)),nv12)
LOCAL_CFLAGS += -DCONFIG_CAMERA_DCAM_SUPPORT_FORMAT_NV12
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_DCAM_SUPPORT_FORMAT)),nv21)
LOCAL_CFLAGS += -DCONFIG_CAMERA_DCAM_SUPPORT_FORMAT_NV21
endif

ifeq ($(strip $(TARGET_VSP_PLATFORM)),iwhale2)
LOCAL_CFLAGS += -DJPG_IWHALE2
endif

LOCAL_CFLAGS += -DCHIP_ENDIAN_LITTLE -DJPEG_ENC

LOCAL_CFLAGS += -DTARGET_GPU_PLATFORM=$(TARGET_GPU_PLATFORM)
ifeq ($(strip $(TARGET_GPU_PLATFORM)),rogue)
LOCAL_CFLAGS += -DCONFIG_GPU_PLATFORM_ROGUE
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_AUTO_DETECT_SENSOR)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_AUTO_DETECT_SENSOR
endif

ifeq ($(strip $(TARGET_BOARD_SPRD_SLOWMOTION_OPTIMIZE)),true)
LOCAL_CFLAGS += -DSPRD_SLOWMOTION_OPTIMIZE
endif

##ifeq ($(strip $(TARGET_BOARD_CAMERA_UHD_VIDEOSNAPSHOT_SUPPORT)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_UHD_VIDEOSNAPSHOT_SUPPORT
##endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_VIDEOSNAPSHOT_USE_VIDEOBUFFER)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_VIDEOSNAPSHOT_USE_VIDEOBUFFER
endif

LOCAL_CFLAGS += -DCAMERA_POWER_DEBUG_ENABLE

ifeq ($(strip $(TARGET_BOARD_SUPPORT_AF_STATS)),true)
LOCAL_CFLAGS += -DCONFIG_ISP_SUPPORT_AF_STATS
endif

ifeq ($(strip $(TARGET_BOARD_RANGEFINDER_SUPPORT)),true)
LOCAL_CFLAGS += -DCONFIG_RANGEFINDER_SUPPORT
else ifeq ($(strip $(TARGET_BOARD_SPRD_RANGEFINDER_SUPPORT)),true)
LOCAL_CFLAGS += -DCONFIG_RANGEFINDER_SUPPORT
endif

ifeq ($(strip $(TARGET_BOARD_STEREOVIDEO_SUPPORT)),true)
LOCAL_CFLAGS += -DCONFIG_STEREOVIDEO_SUPPORT
endif

ifeq ($(strip $(TARGET_BOARD_STEREOPREVIEW_SUPPORT)),true)
LOCAL_CFLAGS += -DCONFIG_STEREOPREVIEW_SUPPORT
endif

ifeq ($(strip $(TARGET_BOARD_STEREOCAPTURE_SUPPORT)),true)
LOCAL_CFLAGS += -DCONFIG_STEREOCAPUTRE_SUPPORT
endif

ifeq ($(strip $(TARGET_BOARD_CONFIG_CAMRECORDER_DYNAMIC_FPS)),true)
LOCAL_CFLAGS += -DCONFIG_CAMRECORDER_DYNAMIC_FPS
ifdef CONFIG_MIN_CAMRECORDER_FPS
LOCAL_CFLAGS += -DCONFIG_MIN_CAMRECORDER_FPS=$(CONFIG_MIN_CAMRECORDER_FPS)
else
LOCAL_CFLAGS += -DCONFIG_MIN_CAMRECORDER_FPS=15
endif
ifdef CONFIG_MAX_CAMRECORDER_FPS
LOCAL_CFLAGS += -DCONFIG_MAX_CAMRECORDER_FPS=$(CONFIG_MAX_CAMRECORDER_FPS)
endif
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_FLASH_LEVEL)),true)
ifdef CONFIG_AVAILABLE_MAX_FLASH_LEVEL
LOCAL_CFLAGS += -DCONFIG_AVAILABLE_FLASH_LEVEL=$(CONFIG_AVAILABLE_MAX_FLASH_LEVEL)
else
LOCAL_CFLAGS += -DCONFIG_AVAILABLE_FLASH_LEVEL=0
endif
else
LOCAL_CFLAGS += -DCONFIG_AVAILABLE_FLASH_LEVEL=0
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ZOOM_FACTOR_SUPPORT)),4)
LOCAL_CFLAGS += -DCONFIG_CAMERA_ZOOM_FACTOR_SUPPORT_4X
endif

ifeq ($(strip $(TARGET_BOARD_ARCSOFT_ZTE_MODE_SUPPORT)),true)
LOCAL_CFLAGS += -DCONFIG_ALTEK_ZTE_CALI
endif

# for 8.0 bringup temp
ifneq ($(filter $(strip $(PLATFORM_VERSION)),O 8.0.0 8.1.0),)
LOCAL_CFLAGS += -DANDROID_VERSION_O_BRINGUP
endif

ifeq ($(strip $(CAMERA_SIZE_LIMIT_FOR_ANDROIDGO)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_SIZE_LIMIT_FOR_ANDROIDGO
endif

ifeq ($(strip $(CAMERA_SIZE_LIMIT_FOR_DDR_1G)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_SIZE_LIMIT_FOR_DDR_1G
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_OFFLINE)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_OFFLINE
endif

ifneq ($(strip $(CAMERA_TORCH_LIGHT_LEVEL)),)
LOCAL_CFLAGS += -DCAMERA_TORCH_LIGHT_LEVEL=$(CAMERA_TORCH_LIGHT_LEVEL)
endif

ifneq ($(filter $(strip $(TARGET_BOARD_CAMERA_ISP_VERSION)),2.2 2.3),)
LOCAL_CFLAGS += -DCONFIG_CAMERA_FRAME_MANUAL
endif

# for 4.4 mocor5 bringup
ifeq ($(PLATFORM_VERSION),4.4.4)
LOCAL_CFLAGS += -DANDROID_VERSION_KK
endif

ifneq ($(filter $(strip $(TARGET_BOARD_PLATFORM)),sp9850ka sc9850kh sp9832e sp9850e sp7731e sp9853i),)
LOCAL_CFLAGS += -DCONFIG_CAMERA_MEET_JPG_ALIGNMENT
endif

ifneq ($(filter $(strip $(TARGET_BOARD_PLATFORM)),sp9850e sp9853i),)
LOCAL_CFLAGS += -DCONFIG_CAMERA_VIDEO_1920_1080
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_MMITEST_PREVIEWSIZE)),320X240)
LOCAL_CFLAGS += -DCONFIG_CAMERA_MMITEST_PREVIEWSIZE_320X240
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_MMITEST_PREVIEWSIZE)),640X480)
LOCAL_CFLAGS += -DCONFIG_CAMERA_MMITEST_PREVIEWSIZE_640X480
endif

ifeq (1, $(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 7)))
LOCAL_CFLAGS += -DCONFIG_USE_CAMERASERVER_PROC
endif

# just for hal3_2v1 and oem2v1
ifeq ($(OEM_DIR),oem2v1)
LOCAL_CFLAGS += -DCAMERA_SUPPORT_ROLLING_SHUTTER_SKEW
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_VIDEO_SNAPSHOT_NOT_SUPPORT)),true)
LOCAL_CFLAGS += -DCONFIG_VIDEO_SNAPSHOT_NOT_SUPPORT
endif

ifeq ($(strip $(TARGET_BOARD_ADJUST_FPS_IN_RANGE)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_ADJUST_FPS_IN_RANGE
endif

ifeq ($(strip $(TARGET_BOARD_ECONOMIZE_MEMORY)),true)
LOCAL_CFLAGS += -DECONOMIZE_MEMORY
endif

ifeq (1, $(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 7)))
LOCAL_CFLAGS += -DCAMERA_DATA_FILE=\"/data/vendor/cameraserver\"
else
LOCAL_CFLAGS += -DCAMERA_DATA_FILE=\"/data/misc/media\"
endif

ifeq ($(strip $(TARGET_BOARD_PLATFORM)), $(filter $(TARGET_BOARD_PLATFORM), sp9863a sp9853i))
LOCAL_CFLAGS += -DCONFIG_CAMERA_MAX_PREVSIZE_1080P
endif
ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_VERSION)),2.2)
ifeq ($(strip $(TARGET_BOARD_CAMERA_PER_FRAME_CONTROL)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_PER_FRAME_CONTROL
endif
endif
