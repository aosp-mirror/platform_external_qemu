# This contains common definitions used to define a host module
# to link GoogleTest with the EmuGL test programs.
#
# This is used instead of including external/gtest/Android.mk to
# be able to build both the 32-bit and 64-bit binaries while
# building a 32-bit only SDK (sdk-eng, sdk_x86-eng, sdk_mips-eng).


LOCAL_PATH := $(EMULATOR_GTEST_SOURCES_DIR)

common_SRC_FILES := \
    googletest/src/gtest-all.cc \
    googlemock/src/gmock-all.cc \
    googlemock/src/gmock_main.cc

common_INCLUDES := \
    $(LOCAL_PATH)/googletest/include \
    $(LOCAL_PATH)/googlemock/include

common_CFLAGS := -O0 -Wno-unused-variable

common_LDLIBS += \
    $(call if-target-any-windows,,-lpthread)

$(call emugl-begin-static-library,libemugl_gtest)

LOCAL_SRC_FILES := $(common_SRC_FILES)
LOCAL_CFLAGS += $(common_CFLAGS)
LOCAL_C_INCLUDES := \
    $(common_INCLUDES) \
    $(LOCAL_PATH)/googletest \
    $(LOCAL_PATH)/googlemock

LOCAL_CPP_EXTENSION := .cc
$(call emugl-export,C_INCLUDES,$(LOCAL_C_INCLUDES))
$(call emugl-export,LDLIBS,$(common_LDLIBS))
$(call emugl-end-module)

ifdef FIRST_INCLUDE
    $(call emugl-begin-host-static-library,libemugl_gtest_host)
    LOCAL_SRC_FILES := $(common_SRC_FILES)
    LOCAL_CFLAGS += $(common_CFLAGS)
    LOCAL_C_INCLUDES := \
        $(common_INCLUDES) \
        $(LOCAL_PATH)/googletest \
        $(LOCAL_PATH)/googlemock
    LOCAL_CPP_EXTENSION := .cc
    $(call emugl-export,C_INCLUDES,$(LOCAL_C_INCLUDES))
    $(call emugl-export,LDLIBS,$(common_LDLIBS) -lpthread)
    $(call emugl-end-module)
endif
