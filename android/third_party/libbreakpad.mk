# Build rules for the static breakpad prebuilt libraries.

BREAKPAD_OLD_LOCAL_PATH := $(LOCAL_PATH)

LOCAL_PATH := $(call my-dir)

BREAKPAD_TOP_DIR := $(BREAKPAD_PREBUILTS_DIR)/$(BUILD_TARGET_TAG)

$(call define-emulator-prebuilt-library,\
    emulator-libbreakpad_client, \
    $(BREAKPAD_TOP_DIR)/lib/libbreakpad_client$(BUILD_TARGET_STATIC_LIBEXT))

BREAKPAD_CLIENT_STATIC_LIBRARIES := \
    emulator-libbreakpad_client
BREAKPAD_CLIENT_INCLUDES := $(BREAKPAD_TOP_DIR)/include/breakpad
BREAKPAD_CLIENT_LDLIBS :=

$(call define-emulator-prebuilt-library,\
    emulator-libbreakpad, \
    $(BREAKPAD_TOP_DIR)/lib/libbreakpad$(BUILD_TARGET_STATIC_LIBEXT))

$(call define-emulator-prebuilt-library,\
    emulator-libdisasm, \
    $(BREAKPAD_TOP_DIR)/lib/libdisasm$(BUILD_TARGET_STATIC_LIBEXT))

BREAKPAD_STATIC_LIBRARIES := \
    emulator-libbreakpad \
    emulator-libdisasm
BREAKPAD_INCLUDES := $(BREAKPAD_TOP_DIR)/include/breakpad
BREAKPAD_LDLIBS :=

LOCAL_PATH := $(BREAKPAD_OLD_LOCAL_PATH)
