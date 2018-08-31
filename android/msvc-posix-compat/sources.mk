# Build file for libsparse and associated executables.

# Update LOCAL_PATH after saving old value.
MSVC_POSIX_OLD_LOCAL_PATH := $(LOCAL_PATH)
LOCAL_PATH := $(call my-dir)

MSVC_POSIX_COMPAT_INCLUDES := $(LOCAL_PATH)/include

$(call start-emulator-library,msvc-posix-compat)

LOCAL_SRC_FILES := \
    src/msvc-posix.c \
    src/getopt.c \
    src/gettimeofday.c

LOCAL_C_INCLUDES := $(MSVC_POSIX_COMPAT_INCLUDES)

$(call end-emulator-library)

# Reset LOCAL_PATH before exiting this build file.
LOCAL_PATH := $(MSVC_POSIX_OLD_LOCAL_PATH)
