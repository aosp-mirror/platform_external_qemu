LOCAL_PATH := $(call my-dir)

### host library ##########################################
$(call emugl-begin-host-static-library,libEGLDispatch)

# use Translator's egl headers
LOCAL_C_INCLUDES += $(EMUGL_PATH)/host/libs/Translator/include
LOCAL_C_INCLUDES += $(EMUGL_PATH)/shared

LOCAL_STATIC_LIBRARIES += libemugl_common

LOCAL_SRC_FILES := EGLDispatch.cpp

$(call emugl-end-module)
