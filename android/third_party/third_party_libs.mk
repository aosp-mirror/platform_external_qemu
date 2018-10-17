OLD_LOCAL_PATH := $(LOCAL_PATH)
LOCAL_PATH := $(call my-dir)

$(call start-cmake-project,third_party)

#(TODO: Jansene) Once we have prebuilt support we should remove
# These compiler includes, so they can become project specific.
LOCAL_C_INCLUDES := $(ZLIB_INCLUDES)
LOCAL_C_INCLUDES += $(LIBCURL_TOP_DIR)/include \

PRODUCED_STATIC_LIBS := emulator-tinyobjloader \
    emulator-murmurhash \
    emulator-libselinux \
    emulator-libwebp \
    emulator-tinyepoxy \
    emulator-astc-codec \
    emulator-libyuv \
    emulator-libjpeg \
    emulator-libsparse \
    emulator-libkeymaster3 \
    emulator-libdtb

PRODUCED_EXECUTABLES := emulator_astc_unittests=emulator$(BUILD_TARGET_SUFFIX)_astc_unittests \
                        emulator_img2simg=emulator$(BUILD_TARGET_SUFFIX)_img2simg \
                        emulator_simg2img=emulator$(BUILD_TARGET_SUFFIX)_simg2img)

# Since we are using a cross build system, we need to declare the static libs that
# are consumed
CONSUMED_STATIC_LIBS := emulator-libsparse

ifeq ($(BUILD_TARGET_OS_FLAVOR),windows)
    PRODUCED_STATIC_LIBS += emulator-libmman-win32
endif

ifeq ($(BUILD_TARGET_OS),windows_msvc)
    PRODUCED_STATIC_LIBS += dirent-win32
    CONSUMED_STATIC_LIBS += msvc-posix-compat
endif

$(call end-cmake-project)

# Set of defines needed for the rest of the build system
LIBDTB_UTILS_INCLUDES := android/third_party/libdtb/include
LIBSELINUX_INCLUDES := android/third_party/libselinux/include
LIBJPEG_INCLUDES := android/third_party/jpeg-6b
EMULATOR_LIBYUV_INCLUDES := ../libyuv/files/include
MURMURHASH_INCLUDES := android/third_party/murmurhash
TINYOBJLOADER_INCLUDES :=  ../tinyobjloader
LIBKEYMASTER3_INCLUDES := android/third_party/libkeymaster3
LIBKEYMASTER3_STATIC_LIBRARIES := emulator-libkeymaster3 emulator-libcrypto android-emu
MURMURHASH_STATIC_LIBRARIES := emulator-murmurhash

ifeq ($(BUILD_TARGET_OS_FLAVOR),windows)
    LIBMMAN_WIN32_INCLUDES := android/third_party/mman-win32/includes
    LIBMMAN_WIN32_STATIC_LIBRARIES := emulator-libmman-win32
endif

ifeq ($(BUILD_TARGET_OS),windows_msvc)
    DIRENT_WIN32_INCLUDES := android/third_party/dirent-win32/include
endif

# Reset LOCAL_PATH before exiting this build file.
LOCAL_PATH := $(OLD_LOCAL_PATH)
