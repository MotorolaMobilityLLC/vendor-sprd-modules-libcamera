LOCAL_SHARED_LIBRARIES := libutils libmemion libcamera_client libcutils libhardware libcamera_metadata
LOCAL_SHARED_LIBRARIES += libui libbinder libdl libcamsensor libcamoem


ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_DIR)),3)
LOCAL_SHARED_LIBRARIES += libcamcommon libcamisp
LOCAL_CFLAGS += -DCONFIG_ISP_3
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_FACE_DETECT)),true)
ifeq ($(strip $(TARGET_BOARD_CAMERA_FD_LIB)),omron)
LOCAL_STATIC_LIBRARIES += libeUdnDt libeUdnCo
else
LOCAL_SHARED_LIBRARIES += libface_finder
endif
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_EIS)),true)
LOCAL_SHARED_LIBRARIES += libgyrostab
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_GYRO)),true)
LOCAL_SHARED_LIBRARIES +=libgui libandroid
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

ifeq ($(strip $(TARGET_BOARD_CAMERA_FACE_BEAUTY)),false)
else
LOCAL_SHARED_LIBRARIES += libts_face_beautify_hal
endif
