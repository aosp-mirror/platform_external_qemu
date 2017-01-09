# Build rules for the static ffmpeg prebuilt libraries.
FFMPEG_OLD_LOCAL_PATH := $(LOCAL_PATH)

LOCAL_PATH := $(call my-dir)

FFMPEG_TOP_DIR := $(FFMPEG_PREBUILTS_DIR)/$(BUILD_TARGET_TAG)

$(call define-emulator-prebuilt-library,\
    emulator-avcodec,\
    $(FFMPEG_TOP_DIR)/lib/libavcodec.a)

$(call define-emulator-prebuilt-library,\
    emulator-libavfilter,\
    $(FFMPEG_TOP_DIR)/lib/libavfilter.a)

$(call define-emulator-prebuilt-library,\
    emulator-libavformat,\
    $(FFMPEG_TOP_DIR)/lib/libavformat.a)

$(call define-emulator-prebuilt-library,\
    emulator-libavresample,\
    $(FFMPEG_TOP_DIR)/lib/libavresample.a)

$(call define-emulator-prebuilt-library,\
    emulator-libavutil,\
    $(FFMPEG_TOP_DIR)/lib/libavutil.a)

$(call define-emulator-prebuilt-library,\
    emulator-libpostproc,\
    $(FFMPEG_TOP_DIR)/lib/libpostproc.a)

$(call define-emulator-prebuilt-library,\
    emulator-libswscale,\
    $(FFMPEG_TOP_DIR)/lib/libswscale.a)

$(call define-emulator-prebuilt-library,\
    emulator-libswresample,\
    $(FFMPEG_TOP_DIR)/lib/libswresample.a)

FFMPEG_INCLUDES := $(FFMPEG_TOP_DIR)/include
FFMPEG_STATIC_LIBRARIES := \
    emulator-avformat \
    emulator-avfilter \
    emulator-avcodec \
    emulator-swresample \
    emulator-swscale \
    emulator-avutil \

LOCAL_PATH := $(FFMPEG_OLD_LOCAL_PATH)
