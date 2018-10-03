# Build rules for the static ANGLE prebuilt library.
ANGLE_TRANSLATION_OLD_LOCAL_PATH := $(LOCAL_PATH)

LOCAL_PATH := $(call my-dir)

ANGLE_TRANSLATION_TOP_DIR := $(ANGLE_TRANSLATION_PREBUILTS_DIR)/$(BUILD_TARGET_TAG)

$(call define-emulator-prebuilt-library,\
    emulator-libpreprocessor,\
    $(ANGLE_TRANSLATION_TOP_DIR)/lib/libpreprocessor$(BUILD_TARGET_STATIC_LIBEXT))

$(call define-emulator-prebuilt-library,\
    emulator-libtranslator_lib,\
    $(ANGLE_TRANSLATION_TOP_DIR)/lib/libtranslator_lib$(BUILD_TARGET_STATIC_LIBEXT))

$(call define-emulator-prebuilt-library,\
    emulator-libangle_common,\
    $(ANGLE_TRANSLATION_TOP_DIR)/lib/libangle_common$(BUILD_TARGET_STATIC_LIBEXT))

$(call define-emulator-prebuilt-library,\
    emulator-libtranslator_static,\
    $(ANGLE_TRANSLATION_TOP_DIR)/lib/libtranslator_static$(BUILD_TARGET_STATIC_LIBEXT))

ANGLE_TRANSLATION_INCLUDES := $(ANGLE_TRANSLATION_TOP_DIR)/include
ANGLE_TRANSLATION_STATIC_LIBRARIES := \
    emulator-libtranslator_static \
    emulator-libtranslator_lib \
    emulator-libpreprocessor \
    emulator-libangle_common \

LOCAL_PATH := $(ANGLE_TRANSLATION_OLD_LOCAL_PATH)
