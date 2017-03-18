## Copyright (C) 2016 The Android Open Source Project
##
## This software is licensed under the terms of the GNU General Public
## License version 2, as published by the Free Software Foundation, and
## may be copied, distributed, and modified under those terms.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
##############################################################################
##############################################################################
###
###  emulator-crash-service: Service for emulator crash dumps
###

# NOTE: Build as 32-bit or 64-bit executable, depending on the value of
#       EMULATOR_PROGRAM_BITNESS.
#

OLD_LOCAL_PATH := $(LOCAL_PATH)
LOCAL_PATH := $(call my-dir)

$(call start-emulator-program, emulator$(BUILD_TARGET_SUFFIX)-crash-service)
$(call gen-hw-config-defs)

LOCAL_SRC_FILES := \
    android/crashreport/main-crash-service.cpp \
    android/crashreport/CrashService_common.cpp \
    android/crashreport/CrashService_$(BUILD_TARGET_OS).cpp \
    android/crashreport/ui/ConfirmDialog.cpp \
    android/resource.c \
    android/skin/resource.c \

LOCAL_STATIC_LIBRARIES := \
    $(ANDROID_EMU_STATIC_LIBRARIES) \
    emulator-libui \
    $(EMULATOR_LIBUI_STATIC_LIBRARIES) \
    $(BREAKPAD_STATIC_LIBRARIES) \

LOCAL_QT_UI_SRC_FILES := \
    android/crashreport/ui/ConfirmDialog.ui \

LOCAL_QT_MOC_SRC_FILES := \
    android/crashreport/ui/ConfirmDialog.h \

LOCAL_LDFLAGS :=
ifeq ($(BUILD_TARGET_OS),windows)
LOCAL_LDFLAGS += -L$(QT_TOP_DIR)/bin
else
LOCAL_LDFLAGS += $(EMULATOR_LIBUI_LDFLAGS)
endif

LOCAL_LDLIBS := \
    $(EMULATOR_LIBUI_LDLIBS) \
    $(ANDROID_EMU_LDLIBS) \
    $(BREAKPAD_LDLIBS) \
    $(CXX_STD_LIB) \

LOCAL_CFLAGS := \
    -DCONFIG_QT \
    $(EMULATOR_VERSION_CFLAGS) \
    $(EMULATOR_LIBUI_CFLAGS) \
    $(LIBCURL_CFLAGS) \

ifdef EMULATOR_CRASHUPLOAD
LOCAL_CFLAGS += \
    -DCRASHUPLOAD=$(EMULATOR_CRASHUPLOAD)
endif

LOCAL_C_INCLUDES += \
    $(ANDROID_EMU_INCLUDES) \
    $(EMULATOR_LIBUI_INCLUDES) \
    $(LIBCURL_INCLUDES) \
    $(BREAKPAD_INCLUDES) \
    $(BREAKPAD_CLIENT_INCLUDES) \

ifeq ($(BUILD_TARGET_OS),windows)
$(eval $(call insert-windows-icon))
endif

$(call end-emulator-program)

##############################################################################
##############################################################################
###
###  emulator_test_crasher: Test exectuable that attaches to crash service
###

# NOTE: Build as 32-bit or 64-bit executable, depending on the value of
#       EMULATOR_PROGRAM_BITNESS.
#

$(call start-emulator-program, emulator$(BUILD_TARGET_SUFFIX)_test_crasher)

LOCAL_C_INCLUDES += \
    $(ANDROID_EMU_INCLUDES) \
    $(BREAKPAD_INCLUDES) \

LOCAL_CFLAGS += -O0

LOCAL_SRC_FILES += \
    android/crashreport/testing/main-test-crasher.cpp \

LOCAL_STATIC_LIBRARIES += \
    $(ANDROID_EMU_STATIC_LIBRARIES) \
    $(BREAKPAD_STATIC_LIBRARIES) \

LOCAL_LDLIBS += \
    $(ANDROID_EMU_LDLIBS) \

# Link against static libstdc++ on Linux and Windows since the unit-tests
# cannot pick up our custom versions of the library from
# $(BUILD_OBJS_DIR)/lib[64]/
$(call local-link-static-c++lib)

$(call end-emulator-program)

##############################################################################
##############################################################################
###
###  crash reporter unit tests

# NOTE: Build as 32-bit or 64-bit executable, depending on the value of
#       EMULATOR_PROGRAM_BITNESS.
#

$(call start-emulator-program, emulator$(BUILD_TARGET_SUFFIX)_crashreport_unittests)

LOCAL_C_INCLUDES += \
    $(ANDROID_EMU_INCLUDES) \
    $(EMULATOR_GTEST_INCLUDES) \
    $(LIBCURL_INCLUDES) \
    $(BREAKPAD_INCLUDES) \

LOCAL_CFLAGS += -O0 $(LIBCURL_CFLAGS)

LOCAL_SRC_FILES := \
    android/crashreport/CrashService_common.cpp \
    android/crashreport/CrashService_$(BUILD_TARGET_OS).cpp \
    android/crashreport/CrashService_unittest.cpp \
    android/crashreport/CrashSystem_unittest.cpp \

LOCAL_STATIC_LIBRARIES += \
    $(ANDROID_EMU_STATIC_LIBRARIES) \
    $(BREAKPAD_STATIC_LIBRARIES) \
    emulator-libgtest \

LOCAL_LDLIBS += \
    $(ANDROID_EMU_LDLIBS) \

# Link against static libstdc++ on Linux and Windows since the unit-tests
# cannot pick up our custom versions of the library from
# $(BUILD_OBJS_DIR)/lib[64]/
$(call local-link-static-c++lib)

$(call end-emulator-program)

LOCAL_PATH := $(OLD_LOCAL_PATH)
