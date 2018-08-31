# Build file for libkeymaster3

# Update LOCAL_PATH after saving old value.
LIBKEYMASTER3_OLD_LOCAL_PATH := $(LOCAL_PATH)
LOCAL_PATH := $(call my-dir)

LIBCURL_TOP_DIR := $(LIBCURL_PREBUILTS_DIR)/$(BUILD_TARGET_TAG)

LIBKEYMASTER3_INCLUDES := $(LOCAL_PATH)
LIBKEYMASTER3_STATIC_LIBRARIES := emulator-libkeymaster3 emulator-libcrypto android-emu

$(call start-emulator-library,emulator-libkeymaster3)

LOCAL_SRC_FILES := \
    aes_key.cpp \
    aes_operation.cpp \
    android_keymaster.cpp \
    android_keymaster_messages.cpp \
    android_keymaster_utils.cpp \
    asymmetric_key.cpp \
    asymmetric_key_factory.cpp \
    attestation_record.cpp \
    auth_encrypted_key_blob.cpp \
    authorization_set.cpp \
    ecdsa_keymaster1_operation.cpp \
    ecdsa_operation.cpp \
    ecies_kem.cpp \
    ec_key.cpp \
    ec_key_factory.cpp \
    ec_keymaster0_key.cpp \
    ec_keymaster1_key.cpp \
    hkdf.cpp \
    hmac.cpp \
    hmac_key.cpp \
    hmac_operation.cpp \
    integrity_assured_key_blob.cpp \
    iso18033kdf.cpp \
    kdf.cpp \
    key.cpp \
    keymaster0_engine.cpp \
    keymaster1_engine.cpp \
    keymaster_enforcement.cpp \
    keymaster_ipc.cpp \
    keymaster_tags.cpp \
    logger.cpp \
    nist_curve_key_exchange.cpp \
    ocb.c \
    ocb_utils.cpp \
    openssl_err.cpp \
    openssl_utils.cpp \
    operation.cpp \
    operation_table.cpp \
    rsa_key.cpp \
    rsa_key_factory.cpp \
    rsa_keymaster0_key.cpp \
    rsa_keymaster1_key.cpp \
    rsa_keymaster1_operation.cpp \
    rsa_operation.cpp \
    serializable.cpp \
    soft_keymaster_context.cpp \
    soft_keymaster_device.cpp \
    symmetric_key.cpp \
    trusty_keymaster_context.cpp \
    trusty_keymaster_enforcement.cpp \
    trusty_rng.c \


LOCAL_C_INCLUDES := $(LIBKEYMASTER3_INCLUDES) \
                    $(LOCAL_PATH)/src \
                    $(LOCAL_PATH)/include \
                    $(LIBCURL_TOP_DIR)/include \
                    $(LOCAL_PATH)/../../../android/android-emu

ifeq (windows,$(BUILD_TARGET_OS))
LOCAL_CFLAGS := -DUSE_MINGW=1
endif

ifeq (windows_msvc,$(BUILD_TARGET_OS))
    LOCAL_C_INCLUDES += $(MSVC_POSIX_COMPAT_INCLUDES)
    LOCAL_STATIC_LIBRARIES += msvc-posix-compat
    LOCAL_CFLAGS := -mssse3
endif

$(call end-emulator-library)

# Reset LOCAL_PATH before exiting this build file.
LOCAL_PATH := $(LIBKEYMASTER3_OLD_LOCAL_PATH)
