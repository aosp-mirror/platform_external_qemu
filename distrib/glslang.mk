# Build rules for the static glslang prebuilt library.
GLSLANG_OLD_LOCAL_PATH := $(LOCAL_PATH)

LOCAL_PATH := $(call my-dir)

GLSLANG_TOP_DIR := $(GLSLANG_PREBUILTS_DIR)/$(BUILD_TARGET_TAG)

$(call define-emulator-prebuilt-library,\
    emulator-libglslang,\
    $(GLSLANG_TOP_DIR)/lib/libglslang.a)

$(call define-emulator-prebuilt-library,\
    emulator-libHLSL,\
    $(GLSLANG_TOP_DIR)/lib/libHLSL.a)

$(call define-emulator-prebuilt-library,\
    emulator-libOGLCompiler,\
    $(GLSLANG_TOP_DIR)/lib/libOGLCompiler.a)

$(call define-emulator-prebuilt-library,\
    emulator-libOSDependent,\
    $(GLSLANG_TOP_DIR)/lib/libOSDependent.a)

$(call define-emulator-prebuilt-library,\
    emulator-libSPIRV,\
    $(GLSLANG_TOP_DIR)/lib/libSPIRV.a)

GLSLANG_INCLUDES := $(GLSLANG_TOP_DIR)/include
GLSLANG_STATIC_LIBRARIES := \
    emulator-libglslang \
    emulator-libHLSL \
    emulator-libOGLCompiler \
    emulator-libOSDependent \
    emulator-libSPIRV \

LOCAL_PATH := $(GLSLANG_OLD_LOCAL_PATH)
