# Build rules for the static CURL prebuilt library.

LIBCURL_OLD_LOCAL_PATH := $(LOCAL_PATH)

LOCAL_PATH := $(call my-dir)

LIBCURL_TOP_DIR := $(LIBCURL_PREBUILTS_DIR)/$(BUILD_TARGET_TAG)

$(call define-emulator-prebuilt-library,\
    emulator-libcurl,\
    $(LIBCURL_TOP_DIR)/lib/libcurl.a)

$(call define-emulator-prebuilt-library,\
    emulator-libssl,\
    $(LIBCURL_TOP_DIR)/lib/libssl.a)

$(call define-emulator-prebuilt-library,\
    emulator-libcrypto,\
    $(LIBCURL_TOP_DIR)/lib/libcrypto.a)

LIBCURL_INCLUDES := $(LIBCURL_TOP_DIR)/include
LIBCURL_CFLAGS += -DCURL_STATICLIB
LIBCURL_STATIC_LIBRARIES := \
    emulator-libcurl \
    emulator-libssl \
    emulator-libcrypto \
    emulator-zlib

ifneq ($(BUILD_TARGET_OS),windows)
    LIBCURL_LDLIBS += -ldl
endif

LOCAL_PATH := $(LIBCURL_OLD_LOCAL_PATH)
