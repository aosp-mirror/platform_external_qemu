LOCAL_PATH := $(call my-dir)

host_common_SRC_FILES := \
     GLESv2Imp.cpp       \
     GLESv2Context.cpp   \
     GLESv2Validate.cpp  \
     ShaderParser.cpp    \
     ShaderValidator.cpp \
     ProgramData.cpp


### GLES_V2 host implementation (On top of OpenGL) ########################
$(call emugl-begin-shared-library,lib$(BUILD_TARGET_SUFFIX)GLES_V2_translator)
$(call emugl-import, libGLcommon)

include $(EMUGL_PATH)/../libglslang.mk
LOCAL_C_INCLUDES += $(LIBGLSLANG_INCLUDES)

LOCAL_STATIC_LIBRARIES += \
    emulator-libglslang \
    emulator-libHLSL \
    emulator-libOGLCompiler \
    emulator-libOSDependent \
    emulator-libSPIRV \

LOCAL_SRC_FILES := $(host_common_SRC_FILES)

$(call emugl-end-module)
