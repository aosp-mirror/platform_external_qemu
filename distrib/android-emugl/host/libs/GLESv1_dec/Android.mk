LOCAL_PATH := $(call my-dir)

host_common_debug_CFLAGS :=

#For gl debbuging
#host_common_debug_CFLAGS += -DCHECK_GL_ERROR
#host_common_debug_CFLAGS += -DDEBUG_PRINTOUT


### host library #########################################
$(call emugl-begin-host-static-library,libGLESv1_dec)

$(call emugl-import, libOpenglCodecCommon)
$(call emugl-export,C_INCLUDES,$(LOCAL_PATH))

$(call emugl-gen-decoder,$(LOCAL_PATH),gles1)

LOCAL_SRC_FILES := GLESv1Decoder.cpp

$(call emugl-export,CFLAGS,$(host_common_debug_CFLAGS))
$(call emugl-export,LDLIBS,-lstdc++)

$(call emugl-end-module)


### host library, 64-bit ####################################
$(call emugl-begin-host64-static-library,lib64GLESv1_dec)

$(call emugl-import, lib64OpenglCodecCommon)
$(call emugl-export,C_INCLUDES,$(LOCAL_PATH))

$(call emugl-gen-decoder,$(LOCAL_PATH),gles1)

LOCAL_SRC_FILES := GLESv1Decoder.cpp

$(call emugl-export,CFLAGS,$(host_common_debug_CFLAGS))
$(call emugl-export,LDLIBS,-lstdc++)

$(call emugl-end-module)
