LOCAL_PATH := $(call my-dir)


### host library ############################################
$(call emugl-begin-static-library,lib_renderControl_dec)
$(call emugl-import,libOpenglCodecCommon)
$(call emugl-gen-decoder,$(LOCAL_PATH),renderControl)
# For renderControl_types.h
$(call emugl-export,C_INCLUDES,$(LOCAL_PATH))

$(call emugl-export,CFLAGS,$(EMUGL_USER_CFLAGS))
$(call emugl-end-module)
