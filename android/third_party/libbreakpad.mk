# Build rules for the static breakpad prebuilt libraries.

BREAKPAD_OLD_LOCAL_PATH := $(LOCAL_PATH)

LOCAL_PATH := $(call my-dir)

BREAKPAD_TOP_DIR := $(BREAKPAD_PREBUILTS_DIR)/$(BUILD_TARGET_TAG)

ifeq ($(BUILD_TARGET_OS),windows_msvc)
    # Windows build generates a bunch of different libraries, and it doesn't
    # look like there's a one-to-one mapping from windows to linux/mac.
    # It's possible that it's built incorrectly as well, since there had to be
    # modifications to built breakpad on Windows without mingw.
    $(call define-emulator-prebuilt-library,\
        emulator-libbreakpad-common, \
        $(BREAKPAD_TOP_DIR)/lib/common$(BUILD_TARGET_STATIC_LIBEXT))

    $(call define-emulator-prebuilt-library,\
        emulator-libbreakpad-exception-handler, \
        $(BREAKPAD_TOP_DIR)/lib/exception_handler$(BUILD_TARGET_STATIC_LIBEXT))

    $(call define-emulator-prebuilt-library,\
        emulator-crash-report-sender, \
        $(BREAKPAD_TOP_DIR)/lib/crash_report_sender$(BUILD_TARGET_STATIC_LIBEXT))

    $(call define-emulator-prebuilt-library,\
        emulator-crash-generation-server, \
        $(BREAKPAD_TOP_DIR)/lib/crash_generation_server$(BUILD_TARGET_STATIC_LIBEXT))

    $(call define-emulator-prebuilt-library,\
        emulator-libbreakpad_client, \
        $(BREAKPAD_TOP_DIR)/lib/crash_generation_client$(BUILD_TARGET_STATIC_LIBEXT))

    $(call define-emulator-prebuilt-library,\
        emulator-libbreakpad-processor, \
        $(BREAKPAD_TOP_DIR)/lib/processor$(BUILD_TARGET_STATIC_LIBEXT))

    $(call define-emulator-prebuilt-library,\
        emulator-libdisasm, \
        $(BREAKPAD_TOP_DIR)/lib/libdisasm$(BUILD_TARGET_STATIC_LIBEXT))

    BREAKPAD_CLIENT_STATIC_LIBRARIES := \
        emulator-libbreakpad_client \
        emulator-libbreakpad-exception-handler \
        emulator-libbreakpad-common \
        emulator-libbreakpad-processor \
        emulator-libdisasm
else
    $(call define-emulator-prebuilt-library,\
        emulator-libbreakpad_client, \
        $(BREAKPAD_TOP_DIR)/lib/libbreakpad_client$(BUILD_TARGET_STATIC_LIBEXT))

    BREAKPAD_CLIENT_STATIC_LIBRARIES := \
        emulator-libbreakpad_client
endif

BREAKPAD_CLIENT_INCLUDES := $(BREAKPAD_TOP_DIR)/include/breakpad
BREAKPAD_CLIENT_LDLIBS :=

ifeq ($(BUILD_TARGET_OS),windows_msvc)
    BREAKPAD_STATIC_LIBRARIES := \
        emulator-libbreakpad-common \
        emulator-libbreakpad-exception-handler \
        emulator-crash-report-sender \
        emulator-crash-generation-server \
        emulator-libbreakpad-processor
else
    $(call define-emulator-prebuilt-library,\
        emulator-libbreakpad, \
        $(BREAKPAD_TOP_DIR)/lib/libbreakpad$(BUILD_TARGET_STATIC_LIBEXT))

    $(call define-emulator-prebuilt-library,\
        emulator-libdisasm, \
        $(BREAKPAD_TOP_DIR)/lib/libdisasm$(BUILD_TARGET_STATIC_LIBEXT))

    BREAKPAD_STATIC_LIBRARIES := \
        emulator-libbreakpad \
        emulator-libdisasm
endif

BREAKPAD_INCLUDES := $(BREAKPAD_TOP_DIR)/include/breakpad
BREAKPAD_LDLIBS :=

LOCAL_PATH := $(BREAKPAD_OLD_LOCAL_PATH)
