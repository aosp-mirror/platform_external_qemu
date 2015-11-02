# this file is included by various Makefiles and defines the set of sources used by our version of LibPng
#
OLD_LOCAL_PATH := $(LOCAL_PATH)
LOCAL_PATH := $(call my-dir)

$(call start-emulator-library,emulator-libjpeg)

LOCAL_SRC_FILES := \
    jcapimin.c \
    jcapistd.c \
    jccoefct.c \
    jccolor.c \
    jcdctmgr.c \
    jchuff.c \
    jcinit.c \
    jcmainct.c \
    jcmarker.c \
    jcmaster.c \
    jcomapi.c \
    jcparam.c \
    jcphuff.c \
    jcprepct.c \
    jcsample.c \
    jctrans.c \
    jdapimin.c \
    jdapistd.c \
    jdatadst.c \
    jdatasrc.c \
    jdcoefct.c \
    jdcolor.c \
    jddctmgr.c \
    jdhuff.c \
    jdinput.c \
    jdmainct.c \
    jdmarker.c \
    jdmaster.c \
    jdmerge.c \
    jdphuff.c \
    jdpostct.c \
    jdsample.c \
    jdtrans.c \
    jerror.c \
    jfdctflt.c \
    jfdctfst.c \
    jfdctint.c \
    jidctflt.c \
    jidctfst.c \
    jidctint.c \
    jidctintelsse.c \
    jidctred.c \
    jmemmgr.c \
    jmem-android.c \
    jquant1.c \
    jquant2.c \
    jutils.c \

LOCAL_CFLAGS += \
    -DAVOID_TABLES \
    -O3 \
    -fstrict-aliasing \
    -DANDROID_INTELSSE2_IDCT \
    -msse2 \
    -DANDROID_TILE_BASED_DECODE \
    -Wno-all \

$(call end-emulator-library)

LIBJPEG_INCLUDES := $(LOCAL_PATH)
LIBJPEG_CFLAGS := -I$(LOCAL_PATH)

LOCAL_PATH := $(OLD_LOCAL_PATH)
