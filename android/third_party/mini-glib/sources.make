# Included from top-level Makefile.common

OLD_LOCAL_PATH := $(LOCAL_PATH)
LOCAL_PATH := $(call my-dir)

MINIGLIB_DIR := $(LOCAL_PATH)
MINIGLIB_INCLUDES := $(MINIGLIB_DIR)/include
MINIGLIB_STATIC_LIBRARIES := emulator-miniglib


# Qemu1, let's not convert to cmake..
$(call start-emulator-library,emulator-miniglib)

LOCAL_C_INCLUDES := $(MINIGLIB_INCLUDES)

LOCAL_SRC_FILES := src/glib-mini.c

ifeq ($(BUILD_TARGET_OS),windows)
LOCAL_SRC_FILES += src/glib-mini-win32.c
endif

$(call end-emulator-library)

LOCAL_PATH := $(OLD_LOCAL_PATH)
