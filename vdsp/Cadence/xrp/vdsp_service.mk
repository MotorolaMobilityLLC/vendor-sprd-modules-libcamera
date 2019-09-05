PRODUCT_PACKAGES += libxrp-common \
		libvdspservice \
		xrptest \
		vdspserver \
		xrpclient

PRODUCT_COPY_FILES += \
    vendor/sprd/modules/libcamera/vdsp/Cadence/xrp/vdsp-service/service.vdspservice.rc:vendor/etc/init/service.vdspservice.rc \
	vendor/sprd/modules/libcamera/vdsp/Cadence/xrp/xrp-firmware/vdsp_firmware.bin:vendor/firmware/vdsp_firmware.bin \
	vendor/sprd/modules/libcamera/vdsp/Cadence/xrp/xrp-firmware/hdr_cadence.bin:vendor/firmware/hdr_cadence.bin \
	vendor/sprd/modules/libcamera/vdsp/Cadence/xrp/xrp-firmware/warp_cadence.bin:vendor/firmware/warp_cadence.bin \
	vendor/sprd/modules/libcamera/vdsp/Cadence/xrp/xrp-firmware/test_lib.o:/vendor/lib/test_lib.o \
	vendor/sprd/modules/libcamera/vdsp/Cadence/xrp/xrp-firmware/test_lib.bin:vendor/firmware/test_lib.bin \
	vendor/sprd/modules/libcamera/vdsp/Cadence/xrp/xrp-firmware/faceid_fw.bin:vendor/firmware/faceid_fw.bin \
	vendor/sprd/modules/libcamera/vdsp/Cadence/xrp/xrp-firmware/network_coeff_fd_o.bin:vendor/firmware/network_coeff_fd_o.bin \
	vendor/sprd/modules/libcamera/vdsp/Cadence/xrp/xrp-firmware/network_coeff_fd_p.bin:vendor/firmware/network_coeff_fd_p.bin \
	vendor/sprd/modules/libcamera/vdsp/Cadence/xrp/xrp-firmware/network_coeff_fd_r.bin:vendor/firmware/network_coeff_fd_r.bin \
	vendor/sprd/modules/libcamera/vdsp/Cadence/xrp/xrp-firmware/network_coeff_flv.bin:vendor/firmware/network_coeff_flv.bin \
	vendor/sprd/modules/libcamera/vdsp/Cadence/xrp/xrp-firmware/network_coeff_fv.bin:vendor/firmware/network_coeff_fv.bin \
	vendor/sprd/modules/libcamera/vdsp/Cadence/xrp/xrp-firmware/network_coeff_fp.bin:vendor/firmware/network_coeff_fp.bin
