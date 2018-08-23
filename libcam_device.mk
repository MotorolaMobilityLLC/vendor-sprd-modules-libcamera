SPRD_LIB := libcamoem
SPRD_LIB += libispalg
SPRD_LIB += libcam_otp_parser
SPRD_LIB += libspcaftrigger
SPRD_LIB += libalRnBLV

#ifeq ($(strip $(TARGET_BOARD_SENSOR_OV4C)),true)
SPRD_LIB += libfcell
SPRD_LIB += libsprd_fcell
#endif
PRODUCT_PACKAGES += $(SPRD_LIB)
