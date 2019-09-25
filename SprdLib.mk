LOCAL_SHARED_LIBRARIES := libutils libmemion libcutils libhardware
LOCAL_SHARED_LIBRARIES += libcamera_metadata
#LOCAL_SHARED_LIBRARIES += libpowermanager
LOCAL_SHARED_LIBRARIES += libui libbinder libdl libcamsensor libcamoem
LOCAL_STATIC_LIBRARIES += android.hardware.camera.common@1.0-helper

LOCAL_SHARED_LIBRARIES += libcamcommon libcamdrv

ifeq ($(strip $(TARGET_BOARD_CAMERA_CPP_USER_DRIVER)),true)
LOCAL_SHARED_LIBRARIES += libcppdrv
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_FACE_BEAUTY)),true)
LOCAL_SHARED_LIBRARIES += libcamfb
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_FACE_DETECT)),true)
LOCAL_SHARED_LIBRARIES += libsprdfa libsprdfd
LOCAL_SHARED_LIBRARIES += libsprdfd_hw
ifeq ($(strip $(TARGET_BOARD_SPRD_FD_VERSION)),1)
LOCAL_CFLAGS += -DCONFIG_SPRD_FD_LIB_VERSION_2
LOCAL_SHARED_LIBRARIES += libsprdfarcnn
else
LOCAL_SHARED_LIBRARIES += libsprdfar
LOCAL_CFLAGS += -DCONFIG_SPRD_FD_LIB_VERSION_1
endif
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_EIS)),true)
LOCAL_SHARED_LIBRARIES += libgyrostab
endif

ifeq ($(strip $(TARGET_BOARD_BOKEH_MODE_SUPPORT)),sbs)
LOCAL_SHARED_LIBRARIES += libsprddepth libsprdbokeh
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_HDR_CAPTURE)),true)
ifeq ($(strip $(TARGET_BOARD_SPRD_HDR_VERSION)),2)
LOCAL_CFLAGS += -DCONFIG_SPRD_HDR_LIB_VERSION_2
LOCAL_SHARED_LIBRARIES += libsprdhdr
else
LOCAL_CFLAGS += -DCONFIG_SPRD_HDR_LIB
LOCAL_SHARED_LIBRARIES += libsprd_easy_hdr
endif
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

ifeq ($(strip $(TARGET_BOARD_BLUR_MODE_SUPPORT)),true)
LOCAL_SHARED_LIBRARIES += libbokeh_gaussian libbokeh_gaussian_cap libSegLite libBokeh2Frames
endif

LOCAL_SHARED_LIBRARIES += libXMPCore libXMPFiles

ifeq ($(strip $(TARGET_BOARD_BOKEH_MODE_SUPPORT)),true)
LOCAL_SHARED_LIBRARIES += libsprdbokeh libsprddepth libbokeh_depth libSegLite
#else ifeq ($(strip $(TARGET_BOARD_SPRD_RANGEFINDER_SUPPORT)),true)
#LOCAL_SHARED_LIBRARIES += libsprddepth libalParseOTP
endif

ifeq ($(strip $(TARGET_BOARD_PORTRAIT_SUPPORT)),true)
LOCAL_SHARED_LIBRARIES += libsprd_portrait_cap libsprddepth libbokeh_depth libSegLite
#else ifeq ($(strip $(TARGET_BOARD_SPRD_RANGEFINDER_SUPPORT)),true)
#LOCAL_SHARED_LIBRARIES += libsprddepth libalParseOTP
endif

ifeq ($(strip $(TARGET_BOARD_STEREOVIDEO_SUPPORT)),true)
LOCAL_SHARED_LIBRARIES += libimagestitcher
else ifeq ($(strip $(TARGET_BOARD_STEREOPREVIEW_SUPPORT)),true)
LOCAL_SHARED_LIBRARIES += libimagestitcher
else ifeq ($(strip $(TARGET_BOARD_STEREOCAPTURE_SUPPORT)),true)
LOCAL_SHARED_LIBRARIES += libimagestitcher
endif

ifdef CONFIG_HAS_CAMERA_HINTS_VERSION
LOCAL_CFLAGS += -DCONFIG_HAS_CAMERA_HINTS_VERSION=$(CONFIG_HAS_CAMERA_HINTS_VERSION)
else
LOCAL_CFLAGS += -DCONFIG_HAS_CAMERA_HINTS_VERSION=0
endif

ifeq ($(strip $(TARGET_BOARD_OPTICSZOOM_SUPPORT)),true)
LOCAL_SHARED_LIBRARIES += libWT
endif

LOCAL_SHARED_LIBRARIES += libyuv
LOCAL_SHARED_LIBRARIES += libyuv420_scaler
LOCAL_SHARED_LIBRARIES += libsprdcamalgassist