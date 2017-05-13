OLD_LOCAL_PATH := $(LOCAL_PATH)
LOCAL_PATH := $(call my-dir)

$(call start-emulator-library,emulator-websockets)
$(call gen-hw-config-defs)

ifeq (darwin,$(BUILD_TARGET_OS))
LOCAL_CFLAGS += -DWEBRTC_MAC
endif

LOCAL_SRC_FILES := \
		webrtc.cpp  \
		websocketserver.cpp \
		wsconsole.cpp \ 

LOCAL_C_INCLUDES := \
	$(EMULATOR_COMMON_INCLUDES) \
	$(WEBSOCKETS_INCLUDES) \
	$(WEBRTC_INCLUDES) \
	$(ANDROID_EMU_INCLUDES) \
	 
LOCAL_STATIC_LIBRARIES += emulator-libwebsockets


$(call end-emulator-library)

LOCAL_PATH := $(OLD_LOCAL_PATH)

