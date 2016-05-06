# Build rules for the static ANGLEtranslator prebuilt library.
ANGLE_TRANSLATOR_OLD_LOCAL_PATH := $(LOCAL_PATH)

LOCAL_PATH := $(call my-dir)

ANGLE_TRANSLATOR_TOP_DIR := $(ANGLE_TRANSLATOR_PREBUILTS_DIR)/$(BUILD_TARGET_TAG)

$(call define-emulator-prebuilt-library,\
    emulator-libpreprocessor,\
    $(ANGLE_TRANSLATOR_TOP_DIR)/lib/libpreprocessor.a)

$(call define-emulator-prebuilt-library,\
    emulator-libtranslator_lib,\
    $(ANGLE_TRANSLATOR_TOP_DIR)/lib/libtranslator_lib.a)

$(call define-emulator-prebuilt-library,\
    emulator-libangle_common,\
    $(ANGLE_TRANSLATOR_TOP_DIR)/lib/libangle_common.a)

$(call define-emulator-prebuilt-library,\
    emulator-libtranslator_static,\
    $(ANGLE_TRANSLATOR_TOP_DIR)/lib/libtranslator_static.a)

ANGLE_TRANSLATOR_INCLUDES := $(ANGLE_TRANSLATOR_TOP_DIR)/include
ANGLE_TRANSLATOR_STATIC_LIBRARIES := \
    emulator-libtranslator_static \
    emulator-libtranslator_lib \
    emulator-libpreprocessor \
    emulator-libangle_common \

LOCAL_PATH := $(ANGLE_TRANSLATOR_OLD_LOCAL_PATH)
