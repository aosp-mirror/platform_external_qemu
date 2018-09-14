# Build file for libkeymaster3

# Update LOCAL_PATH after saving old value.
LIBKEYMASTER3_OLD_LOCAL_PATH := $(LOCAL_PATH)
LOCAL_PATH := $(call my-dir)

LIBCURL_TOP_DIR := $(LIBCURL_PREBUILTS_DIR)/$(BUILD_TARGET_TAG)

LIBKEYMASTER3_INCLUDES := $(LOCAL_PATH)
LIBKEYMASTER3_STATIC_LIBRARIES := emulator-libkeymaster3 emulator-libcrypto android-emu

$(call start-cmake-project,emulator-libkeymaster3)

LOCAL_C_INCLUDES := $(LIBCURL_TOP_DIR)/include \

PRODUCED_STATIC_LIBS := emulator-libkeymaster3
$(call end-cmake-project)

# Reset LOCAL_PATH before exiting this build file.
LOCAL_PATH := $(LIBKEYMASTER3_OLD_LOCAL_PATH)
