OLD_LOCAL_PATH := $(LOCAL_PATH)
LOCAL_PATH := $(call my-dir)

LIBEXT4_UTILS_INCLUDES := $(LOCAL_PATH)/include
LIBEXT4_UTILS_CFLAGS := -DHOST
LIBEXT4_UTILS_CFLAGS += -Wno-error

ifeq ($(BUILD_TARGET_OS),windows)
    LIBEXT4_UTILS_CFLAGS += -DUSE_MINGW=1
endif

$(call start-cmake-project,emulator-libext4_utils)
PRODUCED_STATIC_LIBS := emulator-libext4_utils

LOCAL_C_INCLUDES := \
    $(LIBSPARSE_INCLUDES) \
    $(LIBSELINUX_INCLUDES) \
    $(LOCAL_PATH)/../../../android/android-emu
$(call end-cmake-project)



$(call start-emulator-program,emulator$(BUILD_TARGET_SUFFIX)_make_ext4fs)
LOCAL_SRC_FILES := src/make_ext4fs_main.c
LOCAL_C_INCLUDES := \
    $(LIBEXT4_UTILS_INCLUDES) \
    $(LIBSELINUX_INCLUDES)
LOCAL_CFLAGS := $(LIBEXT4_UTILS_CFLAGS)
LOCAL_STATIC_LIBRARIES := \
    emulator-libext4_utils \
    emulator-libsparse \
    emulator-libselinux \
    emulator-zlib \
    $(LIBMMAN_WIN32_STATIC_LIBRARIES) \
    android-emu-base

$(call local-link-static-c++lib)
$(call end-emulator-program)

LOCAL_PATH := $(OLD_LOCAL_PATH)
