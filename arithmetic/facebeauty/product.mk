ifneq ($(filter $(TARGET_BOARD_PLATFORM), ums512), )
PRODUCT_COPY_FILES += vendor/sprd/modules/libcamera/arithmetic/facebeauty/firmware/facebeauty_cadence.bin:vendor/firmware/facebeauty_cadence.bin
endif

ifneq ($(filter $(TARGET_BOARD_PLATFORM), ums9620), )
PRODUCT_COPY_FILES += vendor/sprd/modules/libcamera/arithmetic/facebeauty/firmware/facebeauty_cadence_vq7.bin:vendor/firmware/facebeauty_cadence.bin
endif