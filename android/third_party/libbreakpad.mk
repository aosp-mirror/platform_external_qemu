# Build rules for the static breakpad prebuilt libraries.

BREAKPAD_OLD_LOCAL_PATH := $(LOCAL_PATH)

LOCAL_PATH := $(call my-dir)

BREAKPAD_TOP_DIR := $(BREAKPAD_PREBUILTS_DIR)/$(BUILD_TARGET_TAG)

ifeq ($(BUILD_TARGET_OS),windows_msvc)
    $(call define-emulator-prebuilt-library,\
        emulator-libbreakpad_client, \
        $(BREAKPAD_TOP_DIR)/lib/crash_generation_client.lib)
else
    $(call define-emulator-prebuilt-library,\
        emulator-libbreakpad_client, \
        $(BREAKPAD_TOP_DIR)/lib/libbreakpad_client.a)
endif

BREAKPAD_CLIENT_STATIC_LIBRARIES := \
    emulator-libbreakpad_client
BREAKPAD_CLIENT_INCLUDES := $(BREAKPAD_TOP_DIR)/include/breakpad
BREAKPAD_CLIENT_LDLIBS :=

ifeq ($(BUILD_TARGET_OS),windows_msvc)
    $(call define-emulator-prebuilt-library,\
        emulator-libbreakpad-common, \
        $(BREAKPAD_TOP_DIR)/lib/common.lib)
    
    $(call define-emulator-prebuilt-library,\
        emulator-libbreakpad-exception-handler, \
        $(BREAKPAD_TOP_DIR)/lib/exception_handler.lib)
    
    $(call define-emulator-prebuilt-library,\
        emulator-crash-report-sender, \
        $(BREAKPAD_TOP_DIR)/lib/crash_report_sender.lib)
    
    $(call define-emulator-prebuilt-library,\
        emulator-crash-generation-server, \
        $(BREAKPAD_TOP_DIR)/lib/crash_generation_server.lib)

    BREAKPAD_STATIC_LIBRARIES := \
        emulator-libbreakpad-common \
        emulator-libbreakpad-exception-handler \
	emulator-crash-report-handler \
	emulator-crash-generation-server

else
    $(call define-emulator-prebuilt-library,\
        emulator-libbreakpad, \
        $(BREAKPAD_TOP_DIR)/lib/libbreakpad.a)
    
    $(call define-emulator-prebuilt-library,\
        emulator-libdisasm, \
        $(BREAKPAD_TOP_DIR)/lib/libdisasm.a)
    
    BREAKPAD_STATIC_LIBRARIES := \
        emulator-libbreakpad \
        emulator-libdisasm
endif

BREAKPAD_INCLUDES := $(BREAKPAD_TOP_DIR)/include/breakpad
BREAKPAD_LDLIBS :=

LOCAL_PATH := $(BREAKPAD_OLD_LOCAL_PATH)
