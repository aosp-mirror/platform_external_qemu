# Build file for tinyepoxy library.

OLD_LOCAL_PATH := $(LOCAL_PATH)
LOCAL_PATH := $(call my-dir)

$(call start-cmake-project, emulator-tinyepoxy)
PRODUCED_STATIC_LIBS := emulator-tinyepoxy
$(call end-emulator-library)

LOCAL_PATH = $(OLD_LOCAL_PATH)
