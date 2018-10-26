# Build file for msvc-posix-compat.

# Update LOCAL_PATH after saving old value.
MSVC_POSIX_OLD_LOCAL_PATH := $(LOCAL_PATH)
LOCAL_PATH := $(call my-dir)

$(call start-cmake-project,msvc-posix-compat)

LOCAL_C_INCLUDES := $(MSVC_POSIX_COMPAT_INCLUDES)

PRODUCED_STATIC_LIBS := msvc-posix-compat

$(call end-cmake-project)

MSVC_POSIX_COMPAT_INCLUDES := $(LOCAL_PATH)/include

# Reset LOCAL_PATH before exiting this build file.
LOCAL_PATH := $(MSVC_POSIX_OLD_LOCAL_PATH)
