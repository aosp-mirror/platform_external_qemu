# Build files for the Android Wear agent library and test programs.

# First, the Wear agent library, which will be linked into the emulator
# and the standalone executable.

LIBWEAR_AGENT_SOURCES := \
    android/wear-agent/android_wear_agent.cpp \
    android/wear-agent/WearAgent.cpp \
    android/wear-agent/PairUpWearPhone.cpp \

LIBWEAR_AGENT_INCLUDES := $(LOCAL_PATH) $(LOCAL_PATH)/include

$(call start-emulator-library, libandroid-wear-agent)
LOCAL_SRC_FILES := $(LIBWEAR_AGENT_SOURCES)
LOCAL_C_INCLUDES := $(LIBWEAR_AGENT_INCLUDES)
$(call end-emulator-library)

# Then, the standalone executable / test programs

WEAR_AGENT_SOURCES := \
    android/wear-agent/main.cpp \
    android/wear-agent/PairUpWearPhone.cpp \
    android/wear-agent/WearAgent.cpp \

WEAR_AGENT_LIBS := emulator-common $(ANDROID_EMU_STATIC_LIBRARIES_QEMU1)

$(call start-emulator-program, wear-agent$(HOST_SUFFIX))
LOCAL_SRC_FILES := $(WEAR_AGENT_SOURCES)
LOCAL_C_INCLUDES := $(LIBWEAR_AGENT_INCLUDES)
LOCAL_STATIC_LIBRARIES := $(WEAR_AGENT_LIBS)
LOCAL_LDLIBS += $(CXX_STD_LIB)
$(call end-emulator-program)
