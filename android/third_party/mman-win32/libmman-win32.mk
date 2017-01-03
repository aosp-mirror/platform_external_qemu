# Build file for libwebp

ifeq ($(BUILD_TARGET_OS),windows)

# Update LOCAL_PATH after saving old value.
LIBMMAN_WIN32_OLD_LOCAL_PATH := $(LOCAL_PATH)
LOCAL_PATH := $(call my-dir)

LIBMMAN_WIN32_INCLUDES := $(LOCAL_PATH)/includes
LIBMMAN_WIN32_STATIC_LIBRARIES := emulator-libmman-win32

$(call start-emulator-library,emulator-libmman-win32)
    LOCAL_SRC_FILES := mman.c
    LOCAL_C_INCLUDES := $(LIBMMAN_WIN32_INCLUDES) $(LOCAL_PATH)
$(call end-emulator-library)

# Reset LOCAL_PATH before exiting this build file.
LOCAL_PATH := $(LIBMMAN_WIN32_OLD_LOCAL_PATH)

endif
