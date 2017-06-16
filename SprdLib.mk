LOCAL_SHARED_LIBRARIES := libutils libmemion libcamera_client libcutils libhardware libcamera_metadata
LOCAL_SHARED_LIBRARIES += libui libbinder libdl libcamsensor libcamoem libpowermanager

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_DIR)),2.1)
LOCAL_SHARED_LIBRARIES += libcamcommon libcamdrv
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_DIR)),2.2)
LOCAL_SHARED_LIBRARIES += libcamcommon libcamdrv
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_DIR)),2.3)
LOCAL_SHARED_LIBRARIES += libcamcommon libcamdrv
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_DIR)),3)
LOCAL_SHARED_LIBRARIES += libcamcommon libcamisp
LOCAL_CFLAGS += -DCONFIG_ISP_3
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

ifeq ($(strip $(TARGET_BOARD_CAMERA_FACE_BEAUTY)),true)
ifeq ($(TARGET_ARCH), $(filter $(TARGET_ARCH), arm arm64))
LOCAL_SHARED_LIBRARIES += libts_face_beautify_hal
else ifeq ($(TARGET_ARCH), $(filter $(TARGET_ARCH), x86 x86_64))
ifeq ($(strip $(TARGET_BOARD_CAMERA_DCAM_SUPPORT_FORMAT)),nv12)
LOCAL_SHARED_LIBRARIES += libts_face_beautify_hal_nv12
else ifeq ($(strip $(TARGET_BOARD_CAMERA_DCAM_SUPPORT_FORMAT)),nv21)
LOCAL_SHARED_LIBRARIES += libts_face_beautify_hal
endif
endif
endif

ifeq ($(strip $(TARGET_BOARD_BLUR_MODE_SUPPORT)),true)
LOCAL_SHARED_LIBRARIES += libbokeh_gaussian libbokeh_gaussian_cap libBokeh2Frames
endif
ifeq ($(strip $(TARGET_BOARD_BOKEH_MODE_SUPPORT)),true)
LOCAL_SHARED_LIBRARIES += libsprdbokeh libsprddepth libbokeh_depth
endif
ifeq ($(strip $(TARGET_BOARD_STEREOVIDEO_SUPPORT)),true)
LOCAL_SHARED_LIBRARIES += libimagestitcher
else ifeq ($(strip $(TARGET_BOARD_STEREOPREVIEW_SUPPORT)),true)
LOCAL_SHARED_LIBRARIES += libimagestitcher
else ifeq ($(strip $(TARGET_BOARD_STEREOCAPTURE_SUPPORT)),true)
LOCAL_SHARED_LIBRARIES += libimagestitcher
endif
