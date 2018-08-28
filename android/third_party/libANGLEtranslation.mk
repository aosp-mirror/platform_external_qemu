# Build rules for the static ANGLE prebuilt library.
ANGLE_TRANSLATION_OLD_LOCAL_PATH := $(LOCAL_PATH)

LOCAL_PATH := $(call my-dir)

ANGLE_TRANSLATION_TOP_DIR := $(ANGLE_TRANSLATION_PREBUILTS_DIR)/$(BUILD_TARGET_TAG)

ifeq ($(BUILD_TARGET_OS),windows_msvc)
    $(call define-emulator-prebuilt-library,\
        emulator-libpreprocessor,\
        $(ANGLE_TRANSLATION_TOP_DIR)/lib/preprocessor.lib)
    
    $(call define-emulator-prebuilt-library,\
        emulator-libtranslator_lib,\
        $(ANGLE_TRANSLATION_TOP_DIR)/lib/translator.lib)
    
    $(call define-emulator-prebuilt-library,\
        emulator-libangle_common,\
        $(ANGLE_TRANSLATION_TOP_DIR)/lib/angle_common.lib)

    ANGLE_TRANSLATION_STATIC_LIBRARIES := \
        emulator-libtranslator_lib \
        emulator-libpreprocessor \
        emulator-libangle_common
else
    $(call define-emulator-prebuilt-library,\
        emulator-libpreprocessor,\
        $(ANGLE_TRANSLATION_TOP_DIR)/lib/libpreprocessor.a)
    
    $(call define-emulator-prebuilt-library,\
        emulator-libtranslator_lib,\
        $(ANGLE_TRANSLATION_TOP_DIR)/lib/libtranslator_lib.a)
    
    $(call define-emulator-prebuilt-library,\
        emulator-libangle_common,\
        $(ANGLE_TRANSLATION_TOP_DIR)/lib/libangle_common.a)
    
    $(call define-emulator-prebuilt-library,\
        emulator-libtranslator_static,\
        $(ANGLE_TRANSLATION_TOP_DIR)/lib/libtranslator_static.a)

    ANGLE_TRANSLATION_STATIC_LIBRARIES := \
        emulator-libtranslator_static \
        emulator-libtranslator_lib \
        emulator-libpreprocessor \
        emulator-libangle_common
endif

ANGLE_TRANSLATION_INCLUDES := $(ANGLE_TRANSLATION_TOP_DIR)/include

LOCAL_PATH := $(ANGLE_TRANSLATION_OLD_LOCAL_PATH)
