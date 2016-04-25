# Build rules for the static glslang prebuilt library.
LIBGLSLANG_OLD_LOCAL_PATH := $(LOCAL_PATH)

LOCAL_PATH := $(call my-dir)

LIBGLSLANG_TOP_DIR := $(LIBGLSLANG_PREBUILTS_DIR)/$(BUILD_TARGET_TAG)

$(call define-emulator-prebuilt-library,\
    emulator-libglslang,\
    $(LIBGLSLANG_TOP_DIR)/lib/libglslang.a)

$(call define-emulator-prebuilt-library,\
    emulator-libHLSL,\
    $(LIBGLSLANG_TOP_DIR)/lib/libHLSL.a)

$(call define-emulator-prebuilt-library,\
    emulator-libOGLCompiler,\
    $(LIBGLSLANG_TOP_DIR)/lib/libOGLCompiler.a)

$(call define-emulator-prebuilt-library,\
    emulator-libOSDependent,\
    $(LIBGLSLANG_TOP_DIR)/lib/libOSDependent.a)

$(call define-emulator-prebuilt-library,\
    emulator-libSPIRV,\
    $(LIBGLSLANG_TOP_DIR)/lib/libSPIRV.a)

LIBGLSLANG_INCLUDES := $(LIBGLSLANG_TOP_DIR)/include
LIBGLSLANG_STATIC_LIBRARIES := \
    emulator-libglslang \
    emulator-libHLSL \
    emulator-libOGLCompiler \
    emulator-libOSDependent \
    emulator-libSPIRV \

LOCAL_PATH := $(LIBGLSLANG_OLD_LOCAL_PATH)
