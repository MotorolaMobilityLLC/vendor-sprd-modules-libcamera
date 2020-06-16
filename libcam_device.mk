SPRD_LIB := libcamoem
SPRD_LIB += libispalg
SPRD_LIB += libcam_otp_parser
SPRD_LIB += libspcaftrigger
SPRD_LIB += libalRnBLV
#ifeq ($(strip $(TARGET_BOARD_SENSOR_SS4C)),true)
SPRD_LIB += libremosaiclib
SPRD_LIB += libremosaic_wrapper
SPRD_LIB += libsprd_fcell_ss
#endif

#ifeq ($(strip $(TARGET_BOARD_SENSOR_OV4C)),true)
SPRD_LIB += libfcell
SPRD_LIB += libsprd_fcell
SPRD_LIB += libcamcalitest
#endif
PRODUCT_PACKAGES += $(SPRD_LIB)

PRODUCT_COPY_FILES += vendor/sprd/modules/libcamera/arithmetic/sprd_easy_hdr/param/sprd_hdr_tuning.param:vendor/etc/sprd_hdr_tuning.param

ifneq ($(filter $(TARGET_BOARD_PLATFORM), ums512), )
PRODUCT_COPY_FILES += vendor/sprd/modules/libcamera/arithmetic/sprd_fdr/firmware/fdr_cadence.bin:vendor/firmware/fdr_cadence.bin \
                      vendor/sprd/modules/libcamera/arithmetic/sprd_easy_hdr/firmware/hdr_cadence.bin:vendor/firmware/hdr_cadence.bin \
                      vendor/sprd/modules/libcamera/arithmetic/sprd_warp/firmware/warp_cadence.bin:vendor/firmware/warp_cadence.bin \
                      vendor/sprd/modules/libcamera/arithmetic/facebeauty/firmware/facebeauty_cadence.bin:vendor/firmware/facebeauty_cadence.bin \
					  vendor/sprd/modules/libcamera/arithmetic/libmfnr/firmware/mfnr_cadence.bin:vendor/firmware/mfnr_cadence.bin\
		      vendor/sprd/modules/libcamera/arithmetic/sprd_portrait_scene/firmware/hourglass_cadence.bin:vendor/firmware/hourglass_cadence.bin \
		      vendor/sprd/modules/libcamera/arithmetic/sprd_portrait_scene/firmware/hourglass_network.bin:vendor/firmware/hourglass_network.bin

COM_IMG_PATH :=$(LOCAL_PATH)/arithmetic/sprd_portrait_scene/image/common
img_files := $(shell ls $(COM_IMG_PATH))
PRODUCT_COPY_FILES += $(foreach file, $(img_files), \
         $(COM_IMG_PATH)/$(file):vendor/etc/aiimg/common/$(file))

COM_IMG_PATH :=$(LOCAL_PATH)/arithmetic/sprd_portrait_scene/image/front
img_files := $(shell ls $(COM_IMG_PATH))
PRODUCT_COPY_FILES += $(foreach file, $(img_files), \
         $(COM_IMG_PATH)/$(file):vendor/etc/aiimg/front/$(file))
endif

TF_MODEL_PATH := vendor/sprd/modules/libcamera/arithmetic/tf_models
model_files := $(shell ls $(TF_MODEL_PATH))
PRODUCT_COPY_FILES += $(foreach file, $(model_files), \
         $(TF_MODEL_PATH)/$(file):vendor/etc/tf_models/$(file))

