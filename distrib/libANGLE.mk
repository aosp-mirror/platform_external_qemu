# Build rules for the static ANGLE prebuilt library.
ANGLE_OLD_LOCAL_PATH := $(LOCAL_PATH)

LOCAL_PATH := $(call my-dir)

ANGLE_TOP_DIR := $(ANGLE_PREBUILTS_DIR)/$(BUILD_TARGET_TAG)

$(call define-emulator-prebuilt-library,\
    emulator-libpreprocessor,\
    $(ANGLE_TOP_DIR)/lib/libpreprocessor.a)

$(call define-emulator-prebuilt-library,\
    emulator-libtranslator_lib,\
    $(ANGLE_TOP_DIR)/lib/libtranslator_lib.a)

$(call define-emulator-prebuilt-library,\
    emulator-libangle_common,\
    $(ANGLE_TOP_DIR)/lib/libangle_common.a)

$(call define-emulator-prebuilt-library,\
    emulator-libtranslator_static,\
    $(ANGLE_TOP_DIR)/lib/libtranslator_static.a)

ANGLE_INCLUDES := $(ANGLE_TOP_DIR)/include
ANGLE_STATIC_LIBRARIES := \
    emulator-libtranslator_static \
    emulator-libtranslator_lib \
    emulator-libpreprocessor \
    emulator-libangle_common \

LOCAL_PATH := $(ANGLE_OLD_LOCAL_PATH)
