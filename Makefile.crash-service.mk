##############################################################################
##############################################################################
###
###  emulator-crash-service: Service for emulator crash dumps
###

# NOTE: Build as 32-bit or 64-bit executable, depending on the value of
#       EMULATOR_PROGRAM_BITNESS.
#

$(call start-emulator-program, emulator$(HOST_SUFFIX)-crash-service)
LOCAL_SRC_FILES := \
    android/crashreport/main-crash-service.cpp \
    android/crashreport/CrashSystem.cpp \
    android/crashreport/CrashService_common.cpp \
    android/crashreport/CrashService_$(HOST_OS).cpp \
    android/crashreport/ui/ConfirmDialog.cpp \
    android/curl-support.c \
    android/emulation/ConfigDirs.cpp \
    android/resource.c \
    android/skin/resource.c \

LOCAL_STATIC_LIBRARIES := \
    android-emu-base \
    emulator-libui \
    emulator-common \
    $(EMULATOR_LIBUI_STATIC_LIBRARIES) \
    $(LIBCURL_STATIC_LIBRARIES) \
    $(BREAKPAD_CLIENT_STATIC_LIBRARIES) \
    $(BREAKPAD_STATIC_LIBRARIES) \

LOCAL_QT_MOC_SRC_FILES := \
    android/crashreport/ui/ConfirmDialog.h \

LOCAL_LDFLAGS :=
ifeq ($(HOST_OS),windows)
LOCAL_LDFLAGS += -L$(QT_TOP_DIR)/bin
else
LOCAL_LDFLAGS += $(EMULATOR_LIBUI_LDFLAGS)
endif

LOCAL_LDLIBS := \
    $(EMULATOR_LIBUI_LDLIBS) \
    $(LIBCURL_LDLIBS) \
    $(BREAKPAD_CLIENT_LDLIBS) \
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
    $(EMULATOR_LIBUI_INCLUDES) \
    $(LIBCURL_INCLUDES) \
    $(BREAKPAD_INCLUDES) \
    $(BREAKPAD_CLIENT_INCLUDES) \

ifeq ($(HOST_OS),windows)
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

$(call start-emulator-program, emulator$(HOST_SUFFIX)_test_crasher)
LOCAL_C_INCLUDES += \
    $(EMULATOR_GTEST_INCLUDES) \
    $(LOCAL_PATH)/include \
    $(BREAKPAD_INCLUDES) \

LOCAL_CFLAGS += -O0

LOCAL_SRC_FILES += \
    android/crashreport/testing/main-test-crasher.cpp \
    android/emulation/ConfigDirs.cpp \

LOCAL_STATIC_LIBRARIES += \
    emulator-common \
    $(ANDROID_EMU_STATIC_LIBRARIES_QEMU1) \
    $(BREAKPAD_CLIENT_STATIC_LIBRARIES) \

# Link against static libstdc++ on Linux and Windows since the unit-tests
# cannot pick up our custom versions of the library from $(OBJS_DIR)/lib[64]/
$(call local-link-static-c++lib)

$(call end-emulator-program)
