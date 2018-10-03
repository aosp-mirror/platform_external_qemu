# Build rules for the static UUID prebuilt library (Linux only).
# For Windows we need to link to the Rpcrt4.lib for the set of Guid functions.
# Mac has an onw preinstalled version of libuud; using libuuid breaks somewhere
#  deep in Cocoa headers

ifeq ($(BUILD_TARGET_OS),linux)

LIBUUID_OLD_LOCAL_PATH := $(LOCAL_PATH)
LOCAL_PATH := $(call my-dir)

LIBUUID_TOP_DIR := $(LIBUUID_PREBUILTS_DIR)/$(BUILD_TARGET_TAG)

$(call define-emulator-prebuilt-library,\
    emulator-libuuid,\
    $(LIBUUID_TOP_DIR)/lib/libuuid$(BUILD_TARGET_STATIC_LIBEXT))

LIBUUID_INCLUDES := $(LIBUUID_TOP_DIR)/include
LIBUUID_STATIC_LIBRARIES := emulator-libuuid
LIBUUID_LDLIBS :=

LOCAL_PATH := $(LIBUUID_OLD_LOCAL_PATH)

endif

ifeq ($(BUILD_TARGET_OS),windows)

LIBUUID_LDLIBS := -lrpcrt4

endif
