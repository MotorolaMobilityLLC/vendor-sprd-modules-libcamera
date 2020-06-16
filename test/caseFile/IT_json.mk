LOCAL_PATH := $(call my-dir)
JSON_PATH :=$(LOCAL_PATH)/stage1
json_files := $(shell ls $(JSON_PATH))
PRODUCT_COPY_FILES += $(foreach file, $(json_files), \
         $(JSON_PATH)/$(file):vendor/bin/$(file))

JSON_PATH :=$(LOCAL_PATH)/stage2
json_files := $(shell ls $(JSON_PATH))
PRODUCT_COPY_FILES += $(foreach file, $(json_files), \
         $(JSON_PATH)/$(file):vendor/bin/$(file))