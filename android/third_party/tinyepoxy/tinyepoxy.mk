# Build file for tinyepoxy library.

$(call start-emulator-library, emulator-tinyepoxy)
LOCAL_C_INCLUDES := $(LOCAL_PATH)/android/android-emugl/host/include
LOCAL_SRC_FILES := android/third_party/tinyepoxy/tinyepoxy.cpp
$(call end-emulator-library)
