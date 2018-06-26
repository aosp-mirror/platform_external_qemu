LOCAL_PATH := $(call my-dir)

host_common_SRC_FILES :=   \
     CoreProfileEngine.cpp \
     GLEScmImp.cpp         \
     GLEScmUtils.cpp       \
     GLEScmContext.cpp     \
     GLEScmValidate.cpp


### GLES_CM host implementation (On top of OpenGL) ########################
$(call emugl-begin-shared-library,lib$(BUILD_TARGET_SUFFIX)GLES_CM_translator)
$(call emugl-import,libGLcommon)
LOCAL_SRC_FILES := $(host_common_SRC_FILES)
LOCAL_STATIC_LIBRARIES += android-emu-base emulator-astc-codec
$(call emugl-end-module)
