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
#endif
PRODUCT_PACKAGES += $(SPRD_LIB)

PRODUCT_COPY_FILES += vendor/sprd/modules/libcamera/arithmetic/sprd_easy_hdr/param/sprd_hdr_tuning.param:vendor/etc/sprd_hdr_tuning.param

ifneq ($(filter $(TARGET_BOARD_PLATFORM), ums512), )
PRODUCT_COPY_FILES += vendor/sprd/modules/libcamera/arithmetic/sprd_easy_hdr/firmware/hdr_cadence.bin:vendor/firmware/hdr_cadence.bin \
                      vendor/sprd/modules/libcamera/arithmetic/sprd_warp/firmware/warp_cadence.bin:vendor/firmware/warp_cadence.bin \
                      vendor/sprd/modules/libcamera/arithmetic/facebeauty/firmware/facebeauty_cadence.bin:vendor/firmware/facebeauty_cadence.bin
endif
