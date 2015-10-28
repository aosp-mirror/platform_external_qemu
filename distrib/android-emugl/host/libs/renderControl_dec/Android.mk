LOCAL_PATH := $(call my-dir)


### host library ############################################
$(call emugl-begin-host-static-library,lib_renderControl_dec)
$(call emugl-import,libOpenglCodecCommon)
$(call emugl-gen-decoder,$(LOCAL_PATH),renderControl)
# For renderControl_types.h
$(call emugl-export,C_INCLUDES,$(LOCAL_PATH))
LOCAL_CFLAGS += -DDEBUG_PRINTOUT
$(call emugl-end-module)
