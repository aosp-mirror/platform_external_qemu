# Build rules for the static ffmpeg prebuilt libraries.
WEBRTC_OLD_LOCAL_PATH := $(LOCAL_PATH)

LOCAL_PATH := $(call my-dir)

WEBRTC_TOP_DIR := $(LOCAL_PATH)/webrtc/$(BUILD_TARGET_TAG)

$(call define-emulator-library, emulator-libwebrtc-all)
  
# List of static libs we depend on to link in webrtc thingies.
WEBRTC_STATICS := webrtc/libwebrtc.a

		    
# Gets the library name from a full path..
_lib_name = $(lastword $(subst /,  ,$1))

# Declare all the libraries as static libraries
_add_libs =  $(eval __libs :=) \
             $(foreach _lib,$(patsubst %.a,%,$1), \
	        $(eval __lib_name := $(call _lib_name, $(_lib))) \
                $(call define-emulator-prebuilt-library, $(__lib_name), $(WEBRTC_TOP_DIR)/lib/$(_lib).a ) \
	        $(eval __libs += $(__lib_name)) \
	     ) \
	     $(__libs)


$(eval WEBRTC_STATIC_LIBRARIES := $(call _add_libs, $(WEBRTC_STATICS)))

$(call end-emulator-library)

WEBRTC_INCLUDES := $(WEBRTC_TOP_DIR)/include

LOCAL_PATH := $(WEBRTC_OLD_LOCAL_PATH)
