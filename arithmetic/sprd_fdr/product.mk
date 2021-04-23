ifneq ($(filter $(TARGET_BOARD_PLATFORM), ums512), )
PRODUCT_COPY_FILES += vendor/sprd/modules/libcamera/arithmetic/sprd_fdr/firmware/fdr_cadence.bin:vendor/firmware/fdr_cadence.bin
endif

ifneq ($(filter $(TARGET_BOARD_PLATFORM), ums9620), )
PRODUCT_COPY_FILES += vendor/sprd/modules/libcamera/arithmetic/sprd_fdr/firmware/fdr_cadence_vq7.bin:vendor/firmware/fdr_cadence.bin
endif