# this file is included by various Makefiles and defines the set of sources used by our version of LibPng
#
OLD_LOCAL_PATH := $(LOCAL_PATH)
LOCAL_PATH := $(call my-dir)

 $(call start-cmake-project,emulator-libjpeg)

 PRODUCED_STATIC_LIBS=emulator-libjpeg

 $(call end-cmake-project)

LIBJPEG_INCLUDES := $(LOCAL_PATH)
LOCAL_PATH := $(OLD_LOCAL_PATH)
