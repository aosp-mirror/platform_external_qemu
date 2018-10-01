# Build rules for the static CURL prebuilt library.

LIBCURL_OLD_LOCAL_PATH := $(LOCAL_PATH)

LOCAL_PATH := $(call my-dir)

LIBCURL_TOP_DIR := $(LIBCURL_PREBUILTS_DIR)/$(BUILD_TARGET_TAG)

ifeq ($(BUILD_TARGET_OS),windows_msvc)
    $(call define-emulator-prebuilt-library,\
        emulator-libcurl,\
        $(LIBCURL_TOP_DIR)/lib/libcurl.lib)
    # ssleay32.lib ==> libssl.a on Windows
    $(call define-emulator-prebuilt-library,\
        emulator-libssl,\
        $(LIBCURL_TOP_DIR)/lib/ssleay32.lib)
    # libeay32.lib ==> libcrypto.a on Windows
    $(call define-emulator-prebuilt-library,\
        emulator-libcrypto,\
        $(LIBCURL_TOP_DIR)/lib/libeay32.lib)
else
    $(call define-emulator-prebuilt-library,\
        emulator-libcurl,\
        $(LIBCURL_TOP_DIR)/lib/libcurl.a)

    $(call define-emulator-prebuilt-library,\
        emulator-libssl,\
        $(LIBCURL_TOP_DIR)/lib/libssl.a)

    $(call define-emulator-prebuilt-library,\
        emulator-libcrypto,\
        $(LIBCURL_TOP_DIR)/lib/libcrypto.a)
endif

LIBCURL_INCLUDES := $(LIBCURL_TOP_DIR)/include
LIBCURL_CFLAGS += -DCURL_STATICLIB
LIBCURL_STATIC_LIBRARIES := \
    emulator-libcurl \
    emulator-libssl \
    emulator-libcrypto \
    emulator-zlib

# Believe it or not: libcurl depends on gdi32.dll on Windows!
LIBCURL_LDLIBS += \
        $(call if-target-any-windows, \
        -lcrypt32 -lgdi32, \
        -ldl)

LOCAL_PATH := $(LIBCURL_OLD_LOCAL_PATH)
