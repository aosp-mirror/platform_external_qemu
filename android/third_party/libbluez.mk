# Build rules for the static BLUEZ prebuilt library.

LIBBLUEZ_OLD_LOCAL_PATH := $(LOCAL_PATH)

LOCAL_PATH := $(call my-dir)

LIBBLUEZ_TOP_DIR := $(LIBBLUEZ_PREBUILTS_DIR)/$(BUILD_TARGET_TAG)

$(call define-emulator-prebuilt-library,\
    emulator-libbluez,\
    $(LIBBLUEZ_TOP_DIR)/lib/libbluetooth$(BUILD_TARGET_STATIC_LIBEXT))

LIBBLUEZ_INCLUDES := $(LIBBLUEZ_TOP_DIR)/include
LIBBLUEZ_STATIC_LIBRARIES := \
    emulator-libbluez \

LOCAL_PATH := $(LIBBLUEZ_OLD_LOCAL_PATH)

