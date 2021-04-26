ifneq ($(filter $(TARGET_BOARD_PLATFORM), ums512 ums9620), )
COM_IMG_PATH := vendor/sprd/modules/libcamera/arithmetic/sprd_portrait_scene/image/common
img_files := $(shell ls $(COM_IMG_PATH))
PRODUCT_COPY_FILES += $(foreach file, $(img_files), \
         $(COM_IMG_PATH)/$(file):vendor/etc/aiimg/common/$(file))

COM_IMG_PATH := vendor/sprd/modules/libcamera/arithmetic/sprd_portrait_scene/image/front
img_files := $(shell ls $(COM_IMG_PATH))
PRODUCT_COPY_FILES += $(foreach file, $(img_files), \
         $(COM_IMG_PATH)/$(file):vendor/etc/aiimg/front/$(file))
endif

ifneq ($(filter $(TARGET_BOARD_PLATFORM), ums512), )
PRODUCT_COPY_FILES += vendor/sprd/modules/libcamera/arithmetic/sprd_portrait_scene/firmware/portraitseg_cadence.bin:vendor/firmware/portraitseg_cadence.bin \
                      vendor/sprd/modules/libcamera/arithmetic/sprd_portrait_scene/firmware/portraitseg_network.bin:vendor/firmware/portraitseg_network.bin 
endif

ifneq ($(filter $(TARGET_BOARD_PLATFORM), ums9620), )
PRODUCT_COPY_FILES += vendor/sprd/modules/libcamera/arithmetic/sprd_portrait_scene/firmware/portraitseg_cadence_vq7.bin:vendor/firmware/portraitseg_cadence.bin \
                      vendor/sprd/modules/libcamera/arithmetic/sprd_portrait_scene/firmware/portraitseg_network_vq7.bin:vendor/firmware/portraitseg_network.bin 
endif