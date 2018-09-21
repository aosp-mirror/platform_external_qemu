DARWINN_PREBUILT_OLD_LOCAL_PATH := $(LOCAL_PATH)
LOCAL_PATH := $(call my-dir)

DARWINN_PREBUILT_TOP_DIR := $(LOCAL_PATH)/$(BUILD_TARGET_TAG)

ifeq ($(BUILD_TARGET_OS),linux)
    $(call define-emulator-shared-prebuilt-library,\
        emulator-libdarwinn_compiler,\
        $(LOCAL_PATH)/libdarwinn_compiler.so)

    $(call define-emulator-shared-prebuilt-library,\
        emulator-libdarwinn-driver-none,\
        $(LOCAL_PATH)/libreference-driver-none.so)

    LIBDARWINN_PREBUILT_SHARED_LIBRARIES := \
        emulator-libdarwinn_compiler \
        emulator-libdarwinn-driver-none \

    LIBDARWINN_INCLUDES := \
        $(LOCAL_PATH)/include \

    DARWINN_CFLAGS := -DDARWINN_PORT_ANDROID_EMULATOR=1 -DDARWINN_CHIP_TYPE=USB

    LOCAL_PATH := $(DARWINN_PREBUILT_OLD_LOCAL_PATH)

endif
