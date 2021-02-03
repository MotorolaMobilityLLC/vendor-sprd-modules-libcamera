LOCAL_SHARED_LIBRARIES += libutils libmemion libcutils libhardware
LOCAL_SHARED_LIBRARIES += libcamera_metadata
#LOCAL_SHARED_LIBRARIES += libpowermanager
LOCAL_SHARED_LIBRARIES += libui libbinder libdl libcamsensor libcamoem libxml2
LOCAL_STATIC_LIBRARIES += android.hardware.camera.common@1.0-helper
LOCAL_SHARED_LIBRARIES += libinterface libinfo libverify libkey

LOCAL_SHARED_LIBRARIES += libcamcommon libcamdrv

ifeq ($(strip $(TARGET_BOARD_CAMERA_CPP_USER_DRIVER)),true)
LOCAL_SHARED_LIBRARIES += libcppdrv
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_FACE_BEAUTY)),true)
LOCAL_SHARED_LIBRARIES += libcamfb libcamfacebeauty
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_FACE_DETECT)),true)
LOCAL_SHARED_LIBRARIES += libsprdfa libsprdfd
LOCAL_SHARED_LIBRARIES += libsprdfd_hw
ifeq ($(strip $(TARGET_BOARD_SPRD_FD_VERSION)),1)
LOCAL_SHARED_LIBRARIES += libsprdfarcnn
else
LOCAL_SHARED_LIBRARIES += libsprdfar
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
LOCAL_SHARED_LIBRARIES += libsprdhdr
else
LOCAL_SHARED_LIBRARIES += libsprd_easy_hdr
endif
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_DRE_CAPTURE)),true)
LOCAL_SHARED_LIBRARIES += libsprddre
endif

ifeq ($(strip $(TARGET_BOARD_BLUR_MODE_SUPPORT)),true)
LOCAL_SHARED_LIBRARIES += libbokeh_gaussian libbokeh_gaussian_cap libTfliteWrapper libSegLite libBokeh2Frames
endif

LOCAL_SHARED_LIBRARIES += libXMPCore libXMPFiles

ifeq ($(strip $(TARGET_BOARD_BOKEH_MODE_SUPPORT)),true)
LOCAL_SHARED_LIBRARIES += libsprdbokeh libsprddepth libbokeh_depth libTfliteWrapper libSegLite libbokeh_depth_adapter libsprdbokeh_adapter libsprddepthadapter
#else ifeq ($(strip $(TARGET_BOARD_SPRD_RANGEFINDER_SUPPORT)),true)
#LOCAL_SHARED_LIBRARIES += libsprddepth libalParseOTP
endif

ifeq ($(strip $(TARGET_BOARD_PORTRAIT_SUPPORT)),true)
LOCAL_SHARED_LIBRARIES += libsprd_portrait_cap libsprdbokeh libsprddepth libbokeh_depth libSegLiteMNN libcamlpt liblightportrait libcamdfa libdfa libsprd_portrait_cap_adapter
#else ifeq ($(strip $(TARGET_BOARD_SPRD_RANGEFINDER_SUPPORT)),true)
#LOCAL_SHARED_LIBRARIES += libsprddepth libalParseOTP
endif

ifeq ($(strip $(TARGET_BOARD_PORTRAIT_SINGLE_SUPPORT)),true)
LOCAL_SHARED_LIBRARIES += libsprdbokeh libbokeh_gaussian libbokeh_gaussian_cap libSegLiteMNN libBokeh2Frames libsprd_portrait_cap libcamlpt liblightportrait libcamdfa libdfa libbokeh_gaussian_adapter libsprd_portrait_cap_adapter
endif

ifeq ($(strip $(TARGET_BOARD_PORTRAIT_SCENE_SUPPORT)),true)
LOCAL_SHARED_LIBRARIES += libsprdportraitsceneadapter libportrait_scene_prev libportrait_scene_cap libSegLiteXNNC libTfliteWrapper libSegLite
endif

ifeq ($(strip $(TARGET_BOARD_STEREOVIDEO_SUPPORT)),true)
LOCAL_SHARED_LIBRARIES += libimagestitcher
else ifeq ($(strip $(TARGET_BOARD_STEREOPREVIEW_SUPPORT)),true)
LOCAL_SHARED_LIBRARIES += libimagestitcher
else ifeq ($(strip $(TARGET_BOARD_STEREOCAPTURE_SUPPORT)),true)
LOCAL_SHARED_LIBRARIES += libimagestitcher
endif

ifeq ($(strip $(TARGET_BOARD_OPTICSZOOM_SUPPORT)),true)
LOCAL_SHARED_LIBRARIES += libWT libsprdwtadapter
endif

LOCAL_SHARED_LIBRARIES += libyuv
LOCAL_SHARED_LIBRARIES += libyuv420_scaler
LOCAL_SHARED_LIBRARIES += libsprdcamalgassist
LOCAL_SHARED_LIBRARIES += libcam_ipmpro
