SPRD_LIB := libcamoem
SPRD_LIB += libmempool
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

SPRD_LIB += libunnengine

PRODUCT_PACKAGES += $(SPRD_LIB)

PRODUCT_COPY_FILES += vendor/sprd/modules/libcamera/arithmetic/sprd_easy_hdr/param/sprd_hdr_tuning.param:vendor/etc/sprd_hdr_tuning.param

$(call inherit-product-if-exists, vendor/sprd/modules/libcamera/arithmetic/sprd_fdr/product.mk)
$(call inherit-product-if-exists, vendor/sprd/modules/libcamera/arithmetic/sprd_portrait_scene/product.mk)

ifneq ($(filter $(TARGET_BOARD_PLATFORM), ums512), )
PRODUCT_COPY_FILES += vendor/sprd/modules/libcamera/arithmetic/sprd_easy_hdr/firmware/hdr_cadence.bin:vendor/firmware/hdr_cadence.bin \
                      vendor/sprd/modules/libcamera/arithmetic/sprd_warp/firmware/warp_cadence.bin:vendor/firmware/warp_cadence.bin \
                      vendor/sprd/modules/libcamera/arithmetic/facebeauty/firmware/facebeauty_cadence.bin:vendor/firmware/facebeauty_cadence.bin \
                      vendor/sprd/modules/libcamera/arithmetic/libmfnr/firmware/mfnr_cadence.bin:vendor/firmware/mfnr_cadence.bin\
                      vendor/sprd/modules/libcamera/arithmetic/depth/firmware/PortraitSegHP_cadence.bin:vendor/firmware/PortraitSegHP_cadence.bin \
                      vendor/sprd/modules/libcamera/arithmetic/depth/firmware/PortraitSegHP_network.bin:vendor/firmware/PortraitSegHP_network.bin \
                      vendor/sprd/modules/libcamera/arithmetic/depth/firmware/PortraitSegLP_cadence.bin:vendor/firmware/PortraitSegLP_cadence.bin \
                      vendor/sprd/modules/libcamera/arithmetic/depth/firmware/PortraitSegLP_network.bin:vendor/firmware/PortraitSegLP_network.bin \
                      vendor/sprd/modules/libcamera/arithmetic/depth/firmware/PortraitSegMP_cadence.bin:vendor/firmware/PortraitSegMP_cadence.bin \
                      vendor/sprd/modules/libcamera/arithmetic/depth/firmware/PortraitSegMP_network.bin:vendor/firmware/PortraitSegMP_network.bin 
endif

TF_MODEL_PATH := vendor/sprd/modules/libcamera/arithmetic/tf_models
model_files := $(shell ls $(TF_MODEL_PATH))
PRODUCT_COPY_FILES += $(foreach file, $(model_files), \
         $(TF_MODEL_PATH)/$(file):vendor/etc/tf_models/$(file))
         
UNN_TF_MODEL_PATH := vendor/sprd/modules/libcamera/arithmetic/tf_models
unn_model_files := $(shell ls $(UNN_TF_MODEL_PATH))
PRODUCT_COPY_FILES += $(foreach file, $(unn_model_files), \
         $(UNN_TF_MODEL_PATH)/$(file):vendor/firmware/$(file))

