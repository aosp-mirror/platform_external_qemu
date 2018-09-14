OLD_LOCAL_PATH := $(LOCAL_PATH)
LOCAL_PATH := $(call my-dir)

$(call start-cmake-project,third_party)
PRODUCED_STATIC_LIBS := emulator-tinyobjloader \
	emulator-murmurhash \
	emulator-libselinux \
	emulator-libwebp \
	emulator-tinyepoxy \
	emulator-astc-codec \
	emulator-libyuv \
	emulator-libjpeg \
	emulator-libsparse \
	emulator-libdtb

PRODUCED_EXECUTABLES := emulator_astc_unittests=emulator$(BUILD_TARGET_SUFFIX)_astc_unittests

$(call end-cmake-project)

LIBSELINUX_INCLUDES := $(LOCAL_PATH)/libselinux/include
EMULATOR_LIBYUV_INCLUDES := ../libyuv/files/include

# Reset LOCAL_PATH before exiting this build file.
LOCAL_PATH := $(OLD_LOCAL_PATH)
