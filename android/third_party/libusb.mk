# Build rules for the static libusb prebuilt library.
LIBUSB_OLD_LOCAL_PATH := $(LOCAL_PATH)

LOCAL_PATH := $(call my-dir)

LIBUSB_TOP_DIR := $(LIBUSB_PREBUILTS_DIR)/$(BUILD_TARGET_TAG)

$(call define-emulator-prebuilt-library,\
    emulator-libusb,\
    $(LIBUSB_TOP_DIR)/lib/libusb-1.0$(BUILD_TARGET_STATIC_LIBEXT))

LIBUSB_INCLUDES := $(LIBUSB_TOP_DIR)/include/libusb-1.0
LIBUSB_STATIC_LIBRARIES := \
    emulator-libusb \

LOCAL_PATH := $(LIBUSB_OLD_LOCAL_PATH)
