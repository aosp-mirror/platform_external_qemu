OLD_LOCAL_PATH := $(LOCAL_PATH)
LOCAL_PATH := $(call my-dir)

LIBDTB_UTILS_INCLUDES := $(LOCAL_PATH)/include

LIBDTB_UTILS_CFLAGS := -DHOST
LIBDTB_UTILS_CFLAGS += -Wno-error

ifeq ($(BUILD_TARGET_OS),windows)
    LIBDTB_UTILS_CFLAGS += -DUSE_MINGW=1
endif

$(call start-emulator-library,emulator-libdtb)

LOCAL_SRC_FILES := src/libdtb.c

LOCAL_C_INCLUDES := \
    $(LIBDTB_UTILS_INCLUDES) \
    $(LIBSPARSE_INCLUDES) \
    $(LIBSELINUX_INCLUDES)
LOCAL_CFLAGS := $(LIBDTB_UTILS_CFLAGS)
$(call end-emulator-library)

LOCAL_PATH := $(OLD_LOCAL_PATH)

