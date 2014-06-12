###########################################################
###########################################################
###
###  GTest libraries.
###
###  GoogleTest is used to build the emulator's Android-specific
###  unit tests. The sources are located under
###  $ANDROID/extern/gtest but because we need to build both
###  32-bit and 64-bit host libraries, don't reuse the
###  Android.mk there, define a module here instead.

EMULATOR_GTEST_SOURCES_DIR ?= $(LOCAL_PATH)/../gtest
EMULATOR_GTEST_SOURCES_DIR := $(EMULATOR_GTEST_SOURCES_DIR)
ifeq (,$(strip $(wildcard $(EMULATOR_GTEST_SOURCES_DIR))))
    $(error Cannot find GoogleTest sources directory: $(EMULATOR_GTEST_SOURCES_DIR))
endif

EMULATOR_GTEST_INCLUDES := $(EMULATOR_GTEST_SOURCES_DIR)/include
EMULATOR_GTEST_SOURCES := src/gtest-all.cc src/gtest_main.cc
EMULATOR_GTEST_LDLIBS := -lstdc++

old_LOCAL_PATH := $(LOCAL_PATH)

LOCAL_PATH := $(EMULATOR_GTEST_SOURCES_DIR)

$(call start-emulator-library, emulator-libgtest)
LOCAL_C_INCLUDES += $(EMULATOR_GTEST_INCLUDES)
LOCAL_CPP_EXTENSION := .cc
LOCAL_CFLAGS += -O0
LOCAL_SRC_FILES := $(EMULATOR_GTEST_SOURCES)
$(call end-emulator-library)

$(call start-emulator64-library, emulator64-libgtest)
LOCAL_C_INCLUDES += $(EMULATOR_GTEST_INCLUDES)
LOCAL_CPP_EXTENSION := .cc
LOCAL_CFLAGS += -O0
LOCAL_SRC_FILES := $(EMULATOR_GTEST_SOURCES)
$(call end-emulator-library)

LOCAL_PATH := $(old_LOCAL_PATH)

