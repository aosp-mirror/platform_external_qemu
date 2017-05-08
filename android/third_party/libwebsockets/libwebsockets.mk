# Build rules for the static breakpad prebuilt libraries.

WEBSOCKETS_OLD_LOCAL_PATH := $(LOCAL_PATH)

LOCAL_PATH := $(call my-dir)

WEBSOCKETS_TOP_DIR := $(LOCAL_PATH)/build/

$(call define-emulator-prebuilt-library,\
    emulator-libwebsockets, \
    $(WEBSOCKETS_TOP_DIR)/lib/libwebsockets.a)

WEBSOCKETS_STATIC_LIBRARIES := \
    emulator-libwebsockets
WEBSOCKETS_INCLUDES := $(WEBSOCKETS_TOP_DIR)/include
WEBSOCKETS_LDLIBS :=

LOCAL_PATH := $(WEBSOCKETS_OLD_LOCAL_PATH)


