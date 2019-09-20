$(warning "tuning TARGET_BOARD" $(TARGET_BOARD))
$(warning "tuning CHIP_NAME" $(CHIP_NAME))

#ifeq ($(strip $(CHIP_NAME)),sharkl5Pro)
#ums512_1h10
PRODUCT_PACKAGES += libparam_ov32a1q_back_main
PRODUCT_PACKAGES += libparam_ov16885_normal
PRODUCT_PACKAGES += libparam_imx351_back_main
PRODUCT_PACKAGES += libparam_imx363_back_main
PRODUCT_PACKAGES += libparam_imx258_back_main
PRODUCT_PACKAGES += libparam_ov13855_back_main
PRODUCT_PACKAGES += libparam_s5ks3p92_front_main
PRODUCT_PACKAGES += libparam_ov8856_shine
#endif

#ifeq ($(strip $(CHIP_NAME)),sharkl3)
#s9863a1h10
PRODUCT_PACKAGES += libparam_imx351
PRODUCT_PACKAGES += libparam_ov16885
PRODUCT_PACKAGES += libparam_imx362
PRODUCT_PACKAGES += libparam_s5k4h9yx
PRODUCT_PACKAGES += libparam_s5k3p9sx04
PRODUCT_PACKAGES += libparam_ov5675
PRODUCT_PACKAGES += libparam_ov7251
PRODUCT_PACKAGES += libparam_ov5675_dual
PRODUCT_PACKAGES += libparam_ov7251_dual
PRODUCT_PACKAGES += libparam_s5k5e9yu05
PRODUCT_PACKAGES += libparam_ov8856_shine_back
PRODUCT_PACKAGES += libparam_ov8856_shine_front
PRODUCT_PACKAGES += libparam_ov8856_shine_go_back
PRODUCT_PACKAGES += libparam_ov8856_shine_Tele
#s9863a3c10
PRODUCT_PACKAGES += libparam_s5k3l6xx03_back_main
PRODUCT_PACKAGES += libparam_s5k4h7_front_main
PRODUCT_PACKAGES += libparam_gc5035_back_main
#s9863a1h10_go
PRODUCT_PACKAGES += libparam_imx351
PRODUCT_PACKAGES += libparam_s5k3p9sx04
PRODUCT_PACKAGES += libparam_ov8856
PRODUCT_PACKAGES += libparam_ov8856_shine_front
PRODUCT_PACKAGES += libparam_ov5675
#endif

#ifeq ($(strip $(CHIP_NAME)),pike2)
#sp7731e_1h10
PRODUCT_PACKAGES += libparam_ov8856_shine_back
PRODUCT_PACKAGES += libparam_gc2375_front_main
#sp7731e_1h20
PRODUCT_PACKAGES += libparam_s5k5e8yx_back_main
PRODUCT_PACKAGES += libparam_gc2375_front_main
#endif

#ifeq ($(strip $(CHIP_NAME)),sharkle)
#sp9832e_1h10_go
PRODUCT_PACKAGES += libparam_ov13855_back_main
PRODUCT_PACKAGES += libparam_ov8856_shine_back
PRODUCT_PACKAGES += libparam_ov5675_front_main
#endif
