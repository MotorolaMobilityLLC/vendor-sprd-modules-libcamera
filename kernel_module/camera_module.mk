PRODUCT_PACKAGES += sprd_sensor.ko \
    sprd_camera.ko \
    sprd_fd.ko \
    sprd_cpp.ko \
    mmdvfs.ko
PRODUCT_COPY_FILES += \
    vendor/sprd/modules/libcamera/kernel_module/camera.rc:vendor/etc/init/camera.rc
