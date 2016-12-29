# Build file for libsparse and associated executables.

# Update LOCAL_PATH after saving old value.
LIBSPARSE_OLD_LOCAL_PATH := $(LOCAL_PATH)
LOCAL_PATH := $(call my-dir)

LIBSPARSE_INCLUDES := $(LOCAL_PATH)/include $(ZLIB_INCLUDES)

$(call start-emulator-library,emulator-libsparse)

LOCAL_SRC_FILES := \
    src/backed_block.c \
    src/output_file.c \
    src/sparse.c \
    src/sparse_crc32.c \
    src/sparse_err.c \
    src/sparse_read.c \

LOCAL_C_INCLUDES := $(LIBSPARSE_INCLUDES) $(LOCAL_PATH)/src $(LIBMMAN_WIN32_INCLUDES)

ifeq (windows,$(BUILD_TARGET_OS))
LOCAL_CFLAGS := -DUSE_MINGW=1
endif
# This third-party library creates warnings at compile time.
LOCAL_CFLAGS += -Wno-error

$(call end-emulator-library)

$(call start-emulator-program,emulator$(BUILD_TARGET_SUFFIX)_img2simg)
LOCAL_SRC_FILES := src/img2simg.c
LOCAL_C_INCLUDES := $(LIBSPARSE_INCLUDES)
LOCAL_STATIC_LIBRARIES := emulator-libsparse emulator-zlib $(LIBMMAN_WIN32_STATIC_LIBRARIES)
$(call end-emulator-program)

$(call start-emulator-program,emulator$(BUILD_TARGET_SUFFIX)_simg2img)
LOCAL_SRC_FILES := src/simg2img.c
LOCAL_C_INCLUDES := $(LIBSPARSE_INCLUDES)
LOCAL_STATIC_LIBRARIES := emulator-libsparse emulator-zlib $(LIBMMAN_WIN32_STATIC_LIBRARIES)
$(call end-emulator-program)

# Reset LOCAL_PATH before exiting this build file.
LOCAL_PATH := $(LIBSPARSE_OLD_LOCAL_PATH)
