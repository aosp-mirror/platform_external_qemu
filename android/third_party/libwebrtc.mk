# Build rules for the static ffmpeg prebuilt libraries.
WEBRTC_OLD_LOCAL_PATH := $(LOCAL_PATH)

LOCAL_PATH := $(call my-dir)

WEBRTC_TOP_DIR := $(LOCAL_PATH)/webrtc/$(BUILD_TARGET_TAG)

$(call define-emulator-prebuilt-library,\
    emulator-libwebrtc,\
    $(WEBRTC_TOP_DIR)/lib/libwebrtc.a)

$(call define-emulator-prebuilt-library,\
    emulator-libwebrtc-common,\
    $(WEBRTC_TOP_DIR)/lib/libwebrtc_common.a)

WEBRTC_INCLUDES := $(LOCAL_PATH)/webrtc/include
WEBRTC_STATIC_LIBRARIES := \
    emulator-libwebrtc \
    emulator-libwebrtc-common \

LOCAL_PATH := $(WEBRTC_OLD_LOCAL_PATH)
