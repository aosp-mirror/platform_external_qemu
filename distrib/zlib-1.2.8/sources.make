OLD_LOCAL_PATH := $(LOCAL_PATH)
LOCAL_PATH := $(call my-dir)

ZLIB_SOURCES := \
    adler32.c \
    compress.c \
    crc32.c \
    deflate.c \
    gzclose.c \
    gzlib.c \
    gzread.c \
    gzwrite.c \
    infback.c \
    inffast.c \
    inflate.c \
    inftrees.c \
    trees.c \
    uncompr.c \
    zutil.c

ZLIB_INCLUDES := $(LOCAL_PATH)

$(call start-emulator-library,emulator-zlib)
LOCAL_SRC_FILES := $(ZLIB_SOURCES)
LOCAL_C_INCLUDES := $(ZLIB_INCLUDES)
$(call end-emulator-library)

$(call start-emulator64-library,emulator64-zlib)
LOCAL_SRC_FILES := $(ZLIB_SOURCES)
LOCAL_C_INCLUDES := $(ZLIB_INCLUDES)
$(call end-emulator-library)

LOCAL_PATH := $(OLD_LOCAL_PATH)
