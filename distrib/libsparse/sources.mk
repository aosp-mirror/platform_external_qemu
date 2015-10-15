# Build file for libsparse and associated executables.

# Update LOCAL_PATH after saving old value.
LIBSPARSE_OLD_LOCAL_PATH := $(LOCAL_PATH)
LOCAL_PATH := $(call my-dir)

LIBSPARSE_SOURCES := \
    src/backed_block.c \
    src/output_file.c \
    src/sparse.c \
    src/sparse_crc32.c \
    src/sparse_err.c \
    src/sparse_read.c \

LIBSPARSE_INCLUDES := $(LOCAL_PATH)/include $(ZLIB_INCLUDES)

ifeq (windows,$(HOST_OS))
LIBSPARSE_CFLAGS := -DUSE_MINGW=1
endif

$(call start-emulator-library,emulator-libsparse)
LOCAL_SRC_FILES := $(LIBSPARSE_SOURCES)
LOCAL_C_INCLUDES := $(LIBSPARSE_INCLUDES) $(LOCAL_PATH)/src
LOCAL_CFLAGS := $(LIBSPARSE_CFLAGS)
$(call end-emulator-library)

$(call start-emulator-program,emulator$(HOST_SUFFIX)_img2simg)
LOCAL_SRC_FILES := src/img2simg.c
LOCAL_C_INCLUDES := $(LIBSPARSE_INCLUDES)
LOCAL_STATIC_LIBRARIES := emulator-libsparse emulator-zlib
$(call end-emulator-program)

$(call start-emulator-program,emulator$(HOST_SUFFIX)_simg2img)
LOCAL_SRC_FILES := src/simg2img.c
LOCAL_C_INCLUDES := $(LIBSPARSE_INCLUDES)
LOCAL_STATIC_LIBRARIES := emulator-libsparse emulator-zlib
$(call end-emulator-program)

# Reset LOCAL_PATH before exiting this build file.
LOCAL_PATH := $(LIBSPARSE_OLD_LOCAL_PATH)
