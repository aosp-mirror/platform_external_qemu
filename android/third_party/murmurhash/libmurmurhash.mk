# Build rules for the murmurhash library
OLD_LOCAL_PATH := $(LOCAL_PATH)
LOCAL_PATH := $(call my-dir)

MURMURHASH_DIR := $(LOCAL_PATH)
MURMURHASH_INCLUDES := $(MURMURHASH_DIR)
MURMURHASH_STATIC_LIBRARIES := emulator-murmurhash

$(call start-cmake-project,emulator-murmurhash)
PRODUCED_STATIC_LIBS := emulator-murmurhash
$(call end-cmake-project)

LOCAL_PATH := $(OLD_LOCAL_PATH)
