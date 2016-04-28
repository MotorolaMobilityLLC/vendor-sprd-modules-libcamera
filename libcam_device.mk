TUNING_BIN := s5k3l2xx_mipi_raw_3a.bin
TUNING_BIN += s5k3l2xx_mipi_raw_shading.bin
TUNING_BIN += s5k5e3yx_mipi_raw_3a.bin
TUNING_BIN += s5k5e3yx_mipi_raw_shading.bin
TUNING_BIN += imx132_mipi_raw_3a.bin
TUNING_BIN += imx132_mipi_raw_shading.bin
TUNING_BIN += s5k3p3sm_mipi_raw_3a.bin
TUNING_BIN += s5k3p3sm_mipi_raw_shading.bin
TUNING_BIN += s5k4h8yx_mipi_raw_3a.bin
TUNING_BIN += s5k4h8yx_mipi_raw_shading.bin
TUNING_BIN += imx230_mipi_raw_3a.bin
TUNING_BIN += imx230_mipi_raw_shading.bin
TUNING_BIN += ov13870_mipi_raw_3a.bin
TUNING_BIN += ov13870_mipi_raw_shading.bin

ALTEK_LIB := libalAWBLib \
	libalAELib \
	libalAFLib \
	libalFlickerLib \
	libspcaftrigger

ALTEK_FW := TBM_G2v1DDR.bin \
	TBM_Qmerge_Sensor1.bin \
	TBM_Qmerge_Sensor2.bin \
	TBM_Qmerge_Sensor3.bin \
	TBM_Shading_0.bin \
	TBM_Shading_1.bin \
	TBM_Shading_2.bin

PRODUCT_PACKAGES += $(ALTEK_LIB)
PRODUCT_PACKAGES += $(ALTEK_FW)
PRODUCT_PACKAGES += $(TUNING_BIN)
PRODUCT_PACKAGES += tuning.bin

### face beautify lib
PRODUCT_PACKAGES += libts_face_beautify_hal
