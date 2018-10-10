OLD_PATH:=$(LOCAL_PATH)
LOCAL_PATH:= $(call my-dir)
#OLD_CFLAGS=$(LOCAL_CFLAGS)

$(call start-cmake-project,android-emu)
LOCAL_CFLAGS:= $(BUILD_TARGET_CFLAGS) $(EMULATOR_COMMON_CFLAGS)
PRODUCED_STATIC_LIBS :=android-emu-base emulator-libui android-emu
PRODUCED_SHARED_LIBS :=android-emu-shared
PRODUCED_PROTO_LIBS :=metrics featurecontrol snapshot crashreport location emulation telephony verified-boot automation offworld
PRODUCED_TESTS :=android-emu_unittests=android_emu$(BUILD_TARGET_SUFFIX)_unittests \
     emulator-libui_unittests=emulator$(BUILD_TARGET_SUFFIX)_libui_unittests \
     android-emu-metrics_unittests=android_emu_metrics$(BUILD_TARGET_SUFFIX)_unittests

# Third party
PRODUCED_STATIC_LIBS += emulator-tinyobjloader \
    emulator-murmurhash \
    emulator-libselinux \
    emulator-libwebp \
    emulator-tinyepoxy \
    emulator-astc-codec \
    emulator-libyuv \
    emulator-libjpeg \
    emulator-libsparse \
    emulator-libkeymaster3 \
    emulator-libdtb \
	emulator-libext4_utils  \

ifeq ($(BUILD_TARGET_OS),windows)
    PRODUCED_STATIC_LIBS += emulator-libmman-win32
endif


PRODUCED_TESTS += emulator_astc_unittests=emulator$(BUILD_TARGET_SUFFIX)_astc_unittests \
                        emulator_img2simg=emulator$(BUILD_TARGET_SUFFIX)_img2simg \
                        emulator_simg2img=emulator$(BUILD_TARGET_SUFFIX)_simg2img

LOCAL_COPY_COMMON_PREBUILT_RESOURCES := \
    virtualscene/Toren1BD/Toren1BD.mtl \
    virtualscene/Toren1BD/Toren1BD.obj \
    virtualscene/Toren1BD/Toren1BD.posters \
    virtualscene/Toren1BD/Toren1BD_Decor.png \
    virtualscene/Toren1BD/Toren1BD_Main.png \
    virtualscene/Toren1BD/poster.png \

LOCAL_COPY_COMMON_TESTDATA := \
    snapshots/random-ram-100.bin \
    textureutils/gray_alpha_golden.bmp \
    textureutils/gray_alpha.png \
    textureutils/gray_golden.bmp \
    textureutils/gray.png \
    textureutils/indexed_alpha_golden.bmp \
    textureutils/indexed_alpha.png \
    textureutils/indexed_golden.bmp \
    textureutils/indexed.png \
    textureutils/interlaced_golden.bmp \
    textureutils/interlaced.png \
    textureutils/jpeg_gray_golden.bmp \
    textureutils/jpeg_gray.jpg \
    textureutils/jpeg_gray_progressive_golden.bmp \
    textureutils/jpeg_gray_progressive.jpg \
    textureutils/jpeg_rgb24_golden.bmp \
    textureutils/jpeg_rgb24.jpg \
    textureutils/jpeg_rgb24_progressive_golden.bmp \
    textureutils/jpeg_rgb24_progressive.jpg \
    textureutils/rgb24_31px_golden.bmp \
    textureutils/rgb24_31px.png \
    textureutils/rgba32_golden.bmp \
    textureutils/rgba32.png \

LOCAL_COPY_COMMON_TESTDATA_DIRS := \
    test-sdk \

$(call end-cmake-project)
LOCAL_COPY_COMMON_PREBUILT_RESOURCES :=
LOCAL_COPY_COMMON_TESTDATA :=
LOCAL_COPY_COMMON_TESTDATA_DIRS :=

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

ifeq ($(BUILD_TARGET_OS),windows)
    LIBMMAN_WIN32_INCLUDES := android/third_party/mman-win32/includes
    LIBMMAN_WIN32_STATIC_LIBRARIES := emulator-libmman-win32
endif

#LOCAL_CFLAGS:=$(OLD_CFLAGS)
# Definitions for others..
EMULATOR_LIBUI_LDFLAGS := $(QT_LDFLAGS)
EMULATOR_LIBUI_LDLIBS := $(QT_LDLIBS)
EMULATOR_LIBUI_STATIC_LIBRARIES :=  $(FFMPEG_STATIC_LIBRARIES) $(LIBX264_STATIC_LIBRARIES) $(LIBVPX_STATIC_LIBRARIES) emulator-zlib
EMULATOR_LIBUI_INCLUDES += $(QT_INCLUDES) $(PROTOBUF_INCLUDES)
ifeq (linux,$(BUILD_TARGET_OS))
EMULATOR_LIBUI_LDLIBS += -lX11
endif

# gl-widget.cpp depends on libOpenGLESDispatch, which depends on
# libemugl_common. Using libOpenGLESDispatch ensures that the code
# will find and use the same host EGL/GLESv2 libraries as the ones
# used by EmuGL. Doing anything else is prone to really bad failure
# cases.
EMULATOR_LIBUI_STATIC_LIBRARIES += \
    libOpenGLESDispatch \
    libemugl_common \

# ffmpeg mac dependency
ifeq ($(BUILD_TARGET_OS),darwin)
    EMULATOR_LIBUI_LDLIBS += -lbz2
endif

_ANDROID_EMU_ROOT := $(LOCAL_PATH)/android-emu
# all includes are like 'android/...', so we need to count on that
ANDROID_EMU_BASE_INCLUDES := $(_ANDROID_EMU_ROOT)
ANDROID_EMU_INCLUDES := $(ANDROID_EMU_BASE_INCLUDES)

# Let others find the generated protobuf files.
ANDROID_EMU_INCLUDES += $(call local-cmake-path,android-emu)/android-emu

# List of static libraries that anything that depends on the base libraries
# should use.
ANDROID_EMU_BASE_STATIC_LIBRARIES := \
    android-emu-base \
    $(LIBUUID_STATIC_LIBRARIES) \
    emulator-lz4 \

ANDROID_EMU_BASE_LDLIBS := \
    $(LIBUUID_LDLIBS) \

ifeq ($(BUILD_TARGET_OS),linux)
    ANDROID_EMU_BASE_LDLIBS += -lrt
    ANDROID_EMU_BASE_LDLIBS += -lX11
    ANDROID_EMU_BASE_LDLIBS += -lGL
endif
ifeq ($(BUILD_TARGET_OS),windows)
    ANDROID_EMU_BASE_LDLIBS += -lpsapi -ld3d9
endif
ifeq ($(BUILD_TARGET_OS),darwin)
    ANDROID_EMU_BASE_LDLIBS += -Wl,-framework,AppKit
    ANDROID_EMU_BASE_LDLIBS += -Wl,-framework,Accelerate
    ANDROID_EMU_BASE_LDLIBS += -Wl,-framework,IOKit
    ANDROID_EMU_BASE_LDLIBS += -Wl,-weak_framework,Hypervisor
    ANDROID_EMU_BASE_LDLIBS += -Wl,-framework,OpenGL
endif

ANDROID_EMU_STATIC_LIBRARIES_DEPS := \
    emulator-libext4_utils \
    $(ANDROID_EMU_BASE_STATIC_LIBRARIES) \
    $(LIBCURL_STATIC_LIBRARIES) \
    $(LIBXML2_STATIC_LIBRARIES) \
    $(BREAKPAD_CLIENT_STATIC_LIBRARIES) \
    emulator-libsparse \
    emulator-libselinux \
    emulator-libjpeg \
    emulator-libpng \
    emulator-libyuv \
    emulator-libwebp \
    emulator-tinyobjloader \
    emulator-zlib \
    emulator-murmurhash \
    emulator-tinyepoxy \
    $(ALL_PROTOBUF_LIBS) \
    $(PROTOBUF_STATIC_LIBRARIES) \

ANDROID_EMU_STATIC_LIBRARIES := \
    android-emu $(ANDROID_EMU_STATIC_LIBRARIES_DEPS) \
    $(LIBKEYMASTER3_STATIC_LIBRARIES) \

ANDROID_EMU_LDLIBS := \
    $(ANDROID_EMU_BASE_LDLIBS) \
    $(LIBCURL_LDLIBS) \
    $(BREAKPAD_CLIENT_LDLIBS) \

ifeq ($(BUILD_TARGET_OS),windows)
# For CoTaskMemFree used in camera-capture-windows.cpp
ANDROID_EMU_LDLIBS += -lole32
# For GetPerformanceInfo in CrashService_windows.cpp
ANDROID_EMU_LDLIBS += -lpsapi
# Winsock functions
ANDROID_EMU_LDLIBS += -lws2_32
# GetNetworkParams() for android/utils/dns.c
ANDROID_EMU_LDLIBS += -liphlpapi
endif

#
LOCAL_PATH:=$(OLD_PATH)
