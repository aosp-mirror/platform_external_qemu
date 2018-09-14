# Build file for libwebp

# Update LOCAL_PATH after saving old value.
LIBWEBP_OLD_LOCAL_PATH := $(LOCAL_PATH)
LOCAL_PATH := $(call my-dir)

LIBWEBP_INCLUDES := $(LOCAL_PATH)/include

$(call start-cmake-project,emulator-libwebp)

PRODUCED_STATIC_LIBS := emulator-libwebp

$(call end-cmake-project)

# Reset LOCAL_PATH before exiting this build file.
LOCAL_PATH := $(LIBWEBP_OLD_LOCAL_PATH)
