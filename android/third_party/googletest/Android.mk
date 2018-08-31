###########################################################
###########################################################
###
###  Googletest libraries.
###
###  Googletest is used to build the emulator's Android-specific
###  unit tests. The sources are located under
###  $ANDROID/extern/gtest but because we need to build both
###  32-bit and 64-bit host libraries, don't reuse the
###  Android.mk there, define a module here instead.

EMULATOR_GTEST_SOURCES_DIR ?= $(LOCAL_PATH)/../googletest
EMULATOR_GTEST_SOURCES_DIR := $(EMULATOR_GTEST_SOURCES_DIR)
ifeq (,$(strip $(wildcard $(EMULATOR_GTEST_SOURCES_DIR))))
    $(error Cannot find GoogleTest sources directory: $(EMULATOR_GTEST_SOURCES_DIR))
endif

EMULATOR_GTEST_INCLUDES := \
    $(EMULATOR_GTEST_SOURCES_DIR)/googletest/include \
    $(EMULATOR_GTEST_SOURCES_DIR)/googlemock/include

EMULATOR_GTEST_SOURCES := \
    googletest/src/gtest-all.cc \
    googlemock/src/gmock-all.cc \
    ../qemu/android/third_party/googletest/gtest_emu_main.cc

old_LOCAL_PATH := $(LOCAL_PATH)

LOCAL_PATH := $(EMULATOR_GTEST_SOURCES_DIR)

$(call start-emulator-library, emulator-libgtest)
LOCAL_C_INCLUDES += \
    $(EMULATOR_GTEST_SOURCES_DIR)/googletest \
    $(EMULATOR_GTEST_SOURCES_DIR)/googlemock \
    $(EMULATOR_GTEST_INCLUDES)
# ANDROID_EMU_BASE_INCLUDES is not available when this file is included, so hardcode it.
LOCAL_C_INCLUDES += $(EMULATOR_GTEST_SOURCES_DIR)/../qemu/android/android-emu
LOCAL_CPP_EXTENSION := .cc
LOCAL_CFLAGS += -O0 -Wno-unused-variable
# Don't include the custom TempDir function. It is for Android only.
LOCAL_CFLAGS += -DGTEST_INCLUDE_GTEST_INTERNAL_CUSTOM_GTEST_H_
LOCAL_SRC_FILES := $(EMULATOR_GTEST_SOURCES)
$(call end-emulator-library)

LOCAL_PATH := $(old_LOCAL_PATH)

EMULATOR_GTEST_STATIC_LIBRARIES := \
    emulator-libgtest \
    android-emu-base
