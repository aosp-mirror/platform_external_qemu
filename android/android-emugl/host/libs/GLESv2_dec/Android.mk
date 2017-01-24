LOCAL_PATH := $(call my-dir)

### host library ##########################################
$(call emugl-begin-static-library,libGLESv2_dec)
$(call emugl-import, libOpenglCodecCommon)
$(call emugl-gen-decoder,$(LOCAL_PATH),gles2)

# For gl2_types.h !
$(call emugl-export,C_INCLUDES,$(LOCAL_PATH))
$(call emugl-export,C_INCLUDES,$(LOCAL_PATH)/../../include/OpenGLESDispatch)
$(call emugl-export,C_INCLUDES,$(LOCAL_PATH)/../libGLSnapshot)

$(call emugl-export,CFLAGS,$(EMUGL_USER_CFLAGS))

LOCAL_STATIC_LIBRARIES += libGLSnapshot
LOCAL_SRC_FILES := GLESv2Decoder.cpp

$(call emugl-end-module)
