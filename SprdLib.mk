LOCAL_SHARED_LIBRARIES := libutils libmemion libcamera_client libcutils libhardware libcamera_metadata
LOCAL_SHARED_LIBRARIES += libui libbinder libdl


ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_DIR)),3)
LOCAL_SHARED_LIBRARIES += libcamcommon libcamisp
LOCAL_CFLAGS += -DCONFIG_ISP_3
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_FACE_DETECT)),true)
	ifeq ($(strip $(TARGET_BOARD_CAMERA_FD_LIB)),omron)
		LOCAL_STATIC_LIBRARIES +=libeUdnDt libeUdnCo
	else
		LOCAL_SHARED_LIBRARIES += libface_finder
	endif
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_EIS)),true)
LOCAL_SHARED_LIBRARIES += libgyrostab libgui libandroid
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


ifeq ($(strip $(TARGET_BOARD_CAMERA_FACE_BEAUTY)),false)
else
LOCAL_SHARED_LIBRARIES += libts_face_beautify_hal
endif

ifeq ($(strip $(isp_use2.0)),1)
ifeq ($(strip $(TARGET_ARCH)),arm)
LOCAL_SHARED_LIBRARIES += libAF libsft_af_ctrl libdeflicker
endif
else
LOCAL_SHARED_LIBRARIES += libawb libaf liblsc
endif

#ALC_S
ifeq ($(strip $(isp_use2.0)),1)
ifeq ($(strip $(TARGET_ARCH)),arm)
ifeq ($(strip $(TARGET_BOARD_USE_ALC_AWB)),true)
LOCAL_CFLAGS += -DCONFIG_USE_ALC_AWB
LOCAL_SHARED_LIBRARIES += libAl_Awb libAl_Awb_Sp
endif
endif
endif
#ALC_E

