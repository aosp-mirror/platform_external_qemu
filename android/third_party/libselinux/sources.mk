OLD_LOCAL_PATH := $(LOCAL_PATH)
LOCAL_PATH := $(call my-dir)

LIBSELINUX_INCLUDES := $(LOCAL_PATH)/include
LIBSELINUX_CFLAGS := -DHOST
ifeq (darwin,$(BUILD_TARGET_OS))
    LIBSELINUX_CFLAGS += -DDARWIN
endif

$(call start-cmake-project,emulator-libselinux)
 PRODUCED_STATIC_LIBS=emulator-libselinux
$(call end-cmake-project)

LOCAL_PATH := $(OLD_LOCAL_PATH)
