OLD_LOCAL_PATH := $(LOCAL_PATH)
LOCAL_PATH := $(call my-dir)

LIBEXT4_UTILS_SOURCES := \
    src/allocate.c \
    src/contents.c \
    src/crc16.c \
    src/ext4_sb.c \
    src/ext4_utils.c \
    src/extent.c \
    src/indirect.c \
    src/make_ext4fs.c \
    src/sha1.c \
    src/uuid.c \
    src/wipe.c \

LIBEXT4_UTILS_INCLUDES := $(LOCAL_PATH)/include

LIBEXT4_UTILS_CFLAGS := -DHOST
ifeq ($(BUILD_TARGET_OS),windows)
    LIBEXT4_UTILS_CFLAGS += -DUSE_MINGW=1
endif

$(call start-emulator-library,emulator-libext4_utils)
LOCAL_SRC_FILES := $(LIBEXT4_UTILS_SOURCES)
LOCAL_C_INCLUDES := \
    $(LIBEXT4_UTILS_INCLUDES) \
    $(LIBSPARSE_INCLUDES) \
    $(LIBSELINUX_INCLUDES)
LOCAL_CFLAGS := $(LIBEXT4_UTILS_CFLAGS)
$(call end-emulator-library)

$(call start-emulator-program,emulator$(HOST_SUFFIX)_make_ext4fs)
LOCAL_SRC_FILES := src/make_ext4fs_main.c
LOCAL_C_INCLUDES := \
    $(LIBEXT4_UTILS_INCLUDES) \
    $(LIBSELINUX_INCLUDES)
LOCAL_CFLAGS := $(LIBEXT4_UTILS_CFLAGS)
LOCAL_STATIC_LIBRARIES := \
    emulator-libext4_utils \
    emulator-libsparse \
    emulator-libselinux \
    emulator-zlib
$(call end-emulator-program)

LOCAL_PATH := $(OLD_LOCAL_PATH)
