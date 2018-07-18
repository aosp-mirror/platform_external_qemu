# Build file for libgetopt and associated executables.
# Use for win32.

# Update LOCAL_PATH after saving old value.
LIBGETOPT_OLD_LOCAL_PATH := $(LOCAL_PATH)
LOCAL_PATH := $(call my-dir)

LIBGETOPT_INCLUDES := $(LOCAL_PATH)/include

$(call start-emulator-library,emulator-libgetopt)

LOCAL_SRC_FILES := \
    src/getopt.c \

LOCAL_C_INCLUDES := $(LIBGETOPT_INCLUDES) $(LOCAL_PATH)/src

$(call end-emulator-library)

# Reset LOCAL_PATH before exiting this build file.
LOCAL_PATH := $(LIBGETOPT_OLD_LOCAL_PATH)
