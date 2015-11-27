# Build files for the Android Wear test programs.

$(call start-emulator-program, wear-agent$(HOST_SUFFIX))

LOCAL_SRC_FILES := \
    android/wear-agent/main.cpp \
    android/wear-agent/PairUpWearPhone.cpp \
    android/wear-agent/WearAgent.cpp \

LOCAL_C_INCLUDES := $(LOCAL_PATH)

LOCAL_STATIC_LIBRARIES := $(ANDROID_EMU_STATIC_LIBRARIES)

LOCAL_LDLIBS := $(CXX_STD_LIB)

$(call end-emulator-program)
