PRODUCT_PACKAGES += sprd_flash_drv.ko \
    flash_ic_sc2703.ko



PRODUCT_COPY_FILES += vendor/sprd/modules/libcamera/kernel_module/flash/init.sprd_flash.rc:vendor/etc/init/init.sprd_flash.rc
