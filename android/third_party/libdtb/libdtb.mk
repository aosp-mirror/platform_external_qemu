OLD_LOCAL_PATH := $(LOCAL_PATH)
LOCAL_PATH := $(call my-dir)

LIBDTB_UTILS_INCLUDES := $(LOCAL_PATH)/include

ifeq ($(BUILD_TARGET_OS),windows)
    LIBDTB_UTILS_CFLAGS += -DUSE_MINGW=1
endif

$(call start-cmake-project,emulator-libdtb)
PRODUCED_STATIC_LIBS := emulator-libdtb
$(call end-cmake-project)

LOCAL_PATH := $(OLD_LOCAL_PATH)

