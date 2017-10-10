# Build rules for the murmurhash library
OLD_LOCAL_PATH := $(LOCAL_PATH)
LOCAL_PATH := $(call my-dir)

MURMURHASH_DIR := $(LOCAL_PATH)
MURMURHASH_INCLUDES := $(MURMURHASH_DIR)
MURMURHASH_STATIC_LIBRARIES := emulator-murmurhash

$(call start-emulator-library,emulator-murmurhash)
    LOCAL_C_INCLUDES := $(MURMURHASH_INCLUDES)
    LOCAL_SRC_FILES := MurmurHash3.cpp
$(call end-emulator-library)

LOCAL_PATH := $(OLD_LOCAL_PATH)
