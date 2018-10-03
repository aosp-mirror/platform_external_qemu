# Build rules for the static ffmpeg prebuilt libraries.
FFMPEG_OLD_LOCAL_PATH := $(LOCAL_PATH)

LOCAL_PATH := $(call my-dir)

FFMPEG_TOP_DIR := $(FFMPEG_PREBUILTS_DIR)/$(BUILD_TARGET_TAG)

$(call define-emulator-prebuilt-library,\
    emulator-libavcodec,\
    $(FFMPEG_TOP_DIR)/lib/libavcodec$(BUILD_TARGET_STATIC_LIBEXT))

$(call define-emulator-prebuilt-library,\
    emulator-libavfilter,\
    $(FFMPEG_TOP_DIR)/lib/libavfilter$(BUILD_TARGET_STATIC_LIBEXT))

$(call define-emulator-prebuilt-library,\
    emulator-libavformat,\
    $(FFMPEG_TOP_DIR)/lib/libavformat$(BUILD_TARGET_STATIC_LIBEXT))

$(call define-emulator-prebuilt-library,\
    emulator-libavresample,\
    $(FFMPEG_TOP_DIR)/lib/libavresample$(BUILD_TARGET_STATIC_LIBEXT))

$(call define-emulator-prebuilt-library,\
    emulator-libavutil,\
    $(FFMPEG_TOP_DIR)/lib/libavutil$(BUILD_TARGET_STATIC_LIBEXT))

$(call define-emulator-prebuilt-library,\
    emulator-libpostproc,\
    $(FFMPEG_TOP_DIR)/lib/libpostproc$(BUILD_TARGET_STATIC_LIBEXT))

$(call define-emulator-prebuilt-library,\
    emulator-libswscale,\
    $(FFMPEG_TOP_DIR)/lib/libswscale$(BUILD_TARGET_STATIC_LIBEXT))

$(call define-emulator-prebuilt-library,\
    emulator-libswresample,\
    $(FFMPEG_TOP_DIR)/lib/libswresample$(BUILD_TARGET_STATIC_LIBEXT))

FFMPEG_INCLUDES := $(FFMPEG_TOP_DIR)/include
FFMPEG_STATIC_LIBRARIES := \
    emulator-libavformat \
    emulator-libavfilter \
    emulator-libavcodec \
    emulator-libswresample \
    emulator-libswscale \
    emulator-libavutil \

LOCAL_PATH := $(FFMPEG_OLD_LOCAL_PATH)
