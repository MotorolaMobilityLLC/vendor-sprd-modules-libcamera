
ALTEK_LIB := libalAWBLib \
	libalFlickerLib \
	libalAELib \
	libalAFLib \
	libspcaftrigger

ALTEK_FW := TBM_G2v1DDR.bin \
	TBM_Qmerge_Sensor1.bin \
	TBM_Qmerge_Sensor2.bin \
	TBM_Qmerge_Sensor3.bin

PRODUCT_PACKAGES += $(ALTEK_LIB)
PRODUCT_PACKAGES += $(ALTEK_FW)
PRODUCT_PACKAGES += tuning.bin
