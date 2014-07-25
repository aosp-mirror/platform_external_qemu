OLD_LOCAL_PATH := $(LOCAL_PATH)
LOCAL_PATH := $(call my-dir)

LIBSELINUX_SOURCES := \
    src/callbacks.c \
    src/check_context.c \
    src/freecon.c \
    src/init.c \
    src/label.c \
    src/label_file.c \
    src/label_android_property.c

ifeq ($(HOST_OS),windows)
    # This code doesn't not build on Windows, so create empty
    # libraries on this platform, this simplifies the build
    # configuration.
    LIBSELINUX_SOURCES :=
endif

LIBSELINUX_INCLUDES := $(LOCAL_PATH)/include

LIBSELINUX_CFLAGS := -DHOST
ifeq (darwin,$(HOST_OS))
    LIBSELINUX_CFLAGS += -DDARWIN
endif

$(call start-emulator-library,emulator-libselinux)
LOCAL_SRC_FILES := $(LIBSELINUX_SOURCES)
LOCAL_C_INCLUDES := $(LIBSELINUX_INCLUDES)
LOCAL_CFLAGS := $(LIBSELINUX_CFLAGS)
$(call end-emulator-library)

$(call start-emulator64-library,emulator64-libselinux)
LOCAL_SRC_FILES := $(LIBSELINUX_SOURCES)
LOCAL_C_INCLUDES := $(LIBSELINUX_INCLUDES)
LOCAL_CFLAGS := $(LIBSELINUX_CFLAGS)
$(call end-emulator-library)

LOCAL_PATH := $(OLD_LOCAL_PATH)
