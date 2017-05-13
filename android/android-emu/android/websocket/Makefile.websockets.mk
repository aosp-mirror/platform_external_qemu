OLD_LOCAL_PATH := $(LOCAL_PATH)
LOCAL_PATH := $(call my-dir)

$(call start-emulator-library,emulator-websockets)
$(call gen-hw-config-defs)

LOCAL_SRC_FILES := \
		webrtc.cpp  \
		websocketserver.cpp \
		wsconsole.cpp \
		pubsub.cpp \
		console-adapter.cpp \

LOCAL_C_INCLUDES := \
	$(EMULATOR_COMMON_INCLUDES) \
	$(WEBSOCKETS_INCLUDES) \
	$(WEBRTC_INCLUDES) \
	$(ANDROID_EMU_INCLUDES) \
	 
LOCAL_STATIC_LIBRARIES += emulator-libwebsockets


$(call end-emulator-library)

LOCAL_PATH := $(OLD_LOCAL_PATH)

