/*
 * Copyright 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef SYSTEM_KEYMASTER_ANDROID_KEYMASTER_TEST_UTILS_H_
#define SYSTEM_KEYMASTER_ANDROID_KEYMASTER_TEST_UTILS_H_

/*
 * Utilities used to help with testing.  Not used in production code.
 */

#include <stdarg.h>

#include <algorithm>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <hardware/keymaster0.h>
#include <hardware/keymaster1.h>
#include <hardware/keymaster2.h>
#include <hardware/keymaster_defs.h>

#include <keymaster/android_keymaster_utils.h>
#include <keymaster/authorization_set.h>
#include <keymaster/keymaster_context.h>
#include <keymaster/logger.h>

std::ostream& operator<<(std::ostream& os, const keymaster_key_param_t& param);
bool operator==(const keymaster_key_param_t& a, const keymaster_key_param_t& b);
std::string hex2str(std::string);

namespace keymaster {

bool operator==(const AuthorizationSet& a, const AuthorizationSet& b);
bool operator!=(const AuthorizationSet& a, const AuthorizationSet& b);

std::ostream& operator<<(std::ostream& os, const AuthorizationSet& set);

namespace test {

template <keymaster_tag_t Tag, typename KeymasterEnum>
bool contains(const AuthorizationSet& set, TypedEnumTag<KM_ENUM, Tag, KeymasterEnum> tag,
              KeymasterEnum val) {
    int pos = set.find(tag);
    return pos != -1 && static_cast<KeymasterEnum>(set[pos].enumerated) == val;
}

template <keymaster_tag_t Tag, typename KeymasterEnum>
bool contains(const AuthorizationSet& set, TypedEnumTag<KM_ENUM_REP, Tag, KeymasterEnum> tag,
              KeymasterEnum val) {
    int pos = -1;
    while ((pos = set.find(tag, pos)) != -1)
        if (static_cast<KeymasterEnum>(set[pos].enumerated) == val)
            return true;
    return false;
}

template <keymaster_tag_t Tag>
bool contains(const AuthorizationSet& set, TypedTag<KM_UINT, Tag> tag, uint32_t val) {
    int pos = set.find(tag);
    return pos != -1 && set[pos].integer == val;
}

template <keymaster_tag_t Tag>
bool contains(const AuthorizationSet& set, TypedTag<KM_UINT_REP, Tag> tag, uint32_t val) {
    int pos = -1;
    while ((pos = set.find(tag, pos)) != -1)
        if (set[pos].integer == val)
            return true;
    return false;
}

template <keymaster_tag_t Tag>
bool contains(const AuthorizationSet& set, TypedTag<KM_ULONG, Tag> tag, uint64_t val) {
    int pos = set.find(tag);
    return pos != -1 && set[pos].long_integer == val;
}

template <keymaster_tag_t Tag>
bool contains(const AuthorizationSet& set, TypedTag<KM_BYTES, Tag> tag, const std::string& val) {
    int pos = set.find(tag);
    return pos != -1 &&
           std::string(reinterpret_cast<const char*>(set[pos].blob.data),
                       set[pos].blob.data_length) == val;
}

template <keymaster_tag_t Tag>
bool contains(const AuthorizationSet& set, TypedTag<KM_BIGNUM, Tag> tag, const std::string& val) {
    int pos = set.find(tag);
    return pos != -1 &&
           std::string(reinterpret_cast<const char*>(set[pos].blob.data),
                       set[pos].blob.data_length) == val;
}

inline bool contains(const AuthorizationSet& set, keymaster_tag_t tag) {
    return set.find(tag) != -1;
}

class StdoutLogger : public Logger {
  public:
    StdoutLogger() { set_instance(this); }

    int log_msg(LogLevel level, const char* fmt, va_list args) const {
        int output_len = 0;
        switch (level) {
        case DEBUG_LVL:
            output_len = printf("DEBUG: ");
            break;
        case INFO_LVL:
            output_len = printf("INFO: ");
            break;
        case WARNING_LVL:
            output_len = printf("WARNING: ");
            break;
        case ERROR_LVL:
            output_len = printf("ERROR: ");
            break;
        case SEVERE_LVL:
            output_len = printf("SEVERE: ");
            break;
        }

        output_len += vprintf(fmt, args);
        output_len += printf("\n");
        return output_len;
    }
};

inline std::string make_string(const uint8_t* data, size_t length) {
    return std::string(reinterpret_cast<const char*>(data), length);
}

template <size_t N> std::string make_string(const uint8_t (&a)[N]) {
    return make_string(a, N);
}

/**
 * Keymaster2TestInstance is used to parameterize Keymaster2Tests.  Its main function is to create a
 * keymaster2_device_t to which test calls can be directed.  It also provides a place to specify
 * various bits of alternative behavior, in cases where different devices are expected to behave
 * differently (any such cases are a potential bug, but sometimes they may make sense).
 */
class Keymaster2TestInstanceCreator {
  public:
    virtual ~Keymaster2TestInstanceCreator(){};
    virtual keymaster2_device_t* CreateDevice() const = 0;

    virtual bool algorithm_in_km0_hardware(keymaster_algorithm_t algorithm) const = 0;
    virtual int keymaster0_calls() const = 0;
    virtual int minimal_digest_set() const { return false; }
    virtual bool is_keymaster1_hw() const = 0;
    virtual KeymasterContext* keymaster_context() const = 0;
};

// Use a shared_ptr because it's copyable.
typedef std::shared_ptr<Keymaster2TestInstanceCreator> InstanceCreatorPtr;

const uint64_t OP_HANDLE_SENTINEL = 0xFFFFFFFFFFFFFFFF;
class Keymaster2Test : public testing::TestWithParam<InstanceCreatorPtr> {
  protected:
    Keymaster2Test();
    ~Keymaster2Test();

    keymaster2_device_t* device();

    keymaster_error_t GenerateKey(const AuthorizationSetBuilder& builder);

    keymaster_error_t DeleteKey();

    keymaster_error_t ImportKey(const AuthorizationSetBuilder& builder,
                                keymaster_key_format_t format, const std::string& key_material);

    keymaster_error_t ExportKey(keymaster_key_format_t format, std::string* export_data);

    keymaster_error_t GetCharacteristics();

    keymaster_error_t BeginOperation(keymaster_purpose_t purpose);
    keymaster_error_t BeginOperation(keymaster_purpose_t purpose, const AuthorizationSet& input_set,
                                     AuthorizationSet* output_set = NULL);

    keymaster_error_t UpdateOperation(const std::string& message, std::string* output,
                                      size_t* input_consumed);
    keymaster_error_t UpdateOperation(const AuthorizationSet& additional_params,
                                      const std::string& message, AuthorizationSet* output_params,
                                      std::string* output, size_t* input_consumed);

    keymaster_error_t FinishOperation(std::string* output);
    keymaster_error_t FinishOperation(const std::string& signature, std::string* output);
    keymaster_error_t FinishOperation(const AuthorizationSet& additional_params,
                                      const std::string& signature, std::string* output) {
        return FinishOperation(additional_params, signature, nullptr /* output_params */, output);
    }
    keymaster_error_t FinishOperation(const AuthorizationSet& additional_params,
                                      const std::string& signature, AuthorizationSet* output_params,
                                      std::string* output);

    keymaster_error_t AbortOperation();

    keymaster_error_t AttestKey(const std::string& attest_challenge, keymaster_cert_chain_t* chain);

    keymaster_error_t UpgradeKey(const AuthorizationSet& upgrade_params);

    keymaster_error_t GetVersion(uint8_t* major, uint8_t* minor, uint8_t* subminor);

    std::string ProcessMessage(keymaster_purpose_t purpose, const std::string& message);
    std::string ProcessMessage(keymaster_purpose_t purpose, const std::string& message,
                               const AuthorizationSet& begin_params,
                               const AuthorizationSet& update_params,
                               AuthorizationSet* output_params = NULL);
    std::string ProcessMessage(keymaster_purpose_t purpose, const std::string& message,
                               const std::string& signature, const AuthorizationSet& begin_params,
                               const AuthorizationSet& update_params,
                               AuthorizationSet* output_params = NULL);
    std::string ProcessMessage(keymaster_purpose_t purpose, const std::string& message,
                               const std::string& signature);

    void SignMessage(const std::string& message, std::string* signature, keymaster_digest_t digest);
    void SignMessage(const std::string& message, std::string* signature, keymaster_digest_t digest,
                     keymaster_padding_t padding);
    void MacMessage(const std::string& message, std::string* signature, size_t mac_length);

    void VerifyMessage(const std::string& message, const std::string& signature,
                       keymaster_digest_t digest);
    void VerifyMessage(const std::string& message, const std::string& signature,
                       keymaster_digest_t digest, keymaster_padding_t padding);
    void VerifyMac(const std::string& message, const std::string& signature);

    std::string EncryptMessage(const std::string& message, keymaster_padding_t padding,
                               std::string* generated_nonce = NULL);
    std::string EncryptMessage(const std::string& message, keymaster_digest_t digest,
                               keymaster_padding_t padding, std::string* generated_nonce = NULL);
    std::string EncryptMessage(const std::string& message, keymaster_block_mode_t block_mode,
                               keymaster_padding_t padding, std::string* generated_nonce = NULL);
    std::string EncryptMessage(const AuthorizationSet& update_params, const std::string& message,
                               keymaster_digest_t digest, keymaster_padding_t padding,
                               std::string* generated_nonce = NULL);
    std::string EncryptMessage(const AuthorizationSet& update_params, const std::string& message,
                               keymaster_block_mode_t block_mode, keymaster_padding_t padding,
                               std::string* generated_nonce = NULL);
    std::string EncryptMessageWithParams(const std::string& message,
                                         const AuthorizationSet& begin_params,
                                         const AuthorizationSet& update_params,
                                         AuthorizationSet* output_params);

    std::string DecryptMessage(const std::string& ciphertext, keymaster_padding_t padding);
    std::string DecryptMessage(const std::string& ciphertext, keymaster_digest_t digest,
                               keymaster_padding_t padding);
    std::string DecryptMessage(const std::string& ciphertext, keymaster_block_mode_t block_mode,
                               keymaster_padding_t padding);
    std::string DecryptMessage(const std::string& ciphertext, keymaster_digest_t digest,
                               keymaster_padding_t padding, const std::string& nonce);
    std::string DecryptMessage(const std::string& ciphertext, keymaster_block_mode_t block_mode,
                               keymaster_padding_t padding, const std::string& nonce);
    std::string DecryptMessage(const AuthorizationSet& update_params, const std::string& ciphertext,
                               keymaster_digest_t digest, keymaster_padding_t padding,
                               const std::string& nonce);
    std::string DecryptMessage(const AuthorizationSet& update_params, const std::string& ciphertext,
                               keymaster_block_mode_t block_mode, keymaster_padding_t padding,
                               const std::string& nonce);

    void CheckHmacTestVector(const std::string& key, const std::string& message, keymaster_digest_t digest,
                             std::string expected_mac);
    void CheckAesOcbTestVector(const std::string& key, const std::string& nonce,
                               const std::string& associated_data, const std::string& message,
                               const std::string& expected_ciphertext);
    void CheckAesCtrTestVector(const std::string& key, const std::string& nonce,
                               const std::string& message, const std::string& expected_ciphertext);
    AuthorizationSet UserAuthParams();
    AuthorizationSet ClientParams();

    template <typename T>
    bool ResponseContains(const std::vector<T>& expected, const T* values, size_t len) {
        return expected.size() == len &&
               std::is_permutation(values, values + len, expected.begin());
    }

    template <typename T> bool ResponseContains(T expected, const T* values, size_t len) {
        return (len == 1 && *values == expected);
    }

    AuthorizationSet hw_enforced();
    AuthorizationSet sw_enforced();

    void FreeCharacteristics();
    void FreeKeyBlob();

    void corrupt_key_blob();

    void set_key_blob(const uint8_t* key, size_t key_length) {
        FreeKeyBlob();
        blob_.key_material = key;
        blob_.key_material_size = key_length;
    }

    AuthorizationSet client_params() {
        return AuthorizationSet(client_params_, sizeof(client_params_) / sizeof(client_params_[0]));
    }

  private:
    keymaster2_device_t* device_;
    keymaster_blob_t client_id_ = {.data = reinterpret_cast<const uint8_t*>("app_id"),
                                   .data_length = 6};
    keymaster_key_param_t client_params_[1] = {
        Authorization(TAG_APPLICATION_ID, client_id_.data, client_id_.data_length)};

    uint64_t op_handle_;

    keymaster_key_blob_t blob_;
    keymaster_key_characteristics_t characteristics_;
};

struct Keymaster0CountingWrapper : public keymaster0_device_t {
    explicit Keymaster0CountingWrapper(keymaster0_device_t* device) : device_(device), counter_(0) {
        common = device_->common;
        common.close = counting_close_device;
        client_version = device_->client_version;
        flags = device_->flags;
        context = this;

        generate_keypair = counting_generate_keypair;
        import_keypair = counting_import_keypair;
        get_keypair_public = counting_get_keypair_public;
        delete_keypair = counting_delete_keypair;
        delete_all = counting_delete_all;
        sign_data = counting_sign_data;
        verify_data = counting_verify_data;
    }

    int count() { return counter_; }

    // The blobs generated by the underlying softkeymaster start with "PK#8".  Tweak the prefix so
    // they don't get identified as softkeymaster blobs.
    static void munge_blob(uint8_t* blob, size_t blob_length) {
        if (blob && blob_length > 0 && *blob == 'P')
            *blob = 'Q';  // Mind your Ps and Qs!
    }

    // Copy and un-modfy the blob.  The caller must clean up the return value.
    static uint8_t* unmunge_blob(const uint8_t* blob, size_t blob_length) {
        uint8_t* dup_blob = dup_buffer(blob, blob_length);
        if (dup_blob && blob_length > 0 && *dup_blob == 'Q')
            *dup_blob = 'P';
        return dup_blob;
    }

    static keymaster0_device_t* device(const keymaster0_device_t* dev) {
        Keymaster0CountingWrapper* wrapper =
            reinterpret_cast<Keymaster0CountingWrapper*>(dev->context);
        return wrapper->device_;
    }

    static void increment(const keymaster0_device_t* dev) {
        Keymaster0CountingWrapper* wrapper =
            reinterpret_cast<Keymaster0CountingWrapper*>(dev->context);
        wrapper->counter_++;
    }

    static int counting_close_device(hw_device_t* dev) {
        keymaster0_device_t* k0_dev = reinterpret_cast<keymaster0_device_t*>(dev);
        increment(k0_dev);
        Keymaster0CountingWrapper* wrapper =
            reinterpret_cast<Keymaster0CountingWrapper*>(k0_dev->context);
        int retval =
            wrapper->device_->common.close(reinterpret_cast<hw_device_t*>(wrapper->device_));
        delete wrapper;
        return retval;
    }

    static int counting_generate_keypair(const struct keymaster0_device* dev,
                                         const keymaster_keypair_t key_type, const void* key_params,
                                         uint8_t** key_blob, size_t* key_blob_length) {
        increment(dev);
        int result = device(dev)->generate_keypair(device(dev), key_type, key_params, key_blob,
                                                   key_blob_length);
        if (result == 0)
            munge_blob(*key_blob, *key_blob_length);
        return result;
    }

    static int counting_import_keypair(const struct keymaster0_device* dev, const uint8_t* key,
                                       const size_t key_length, uint8_t** key_blob,
                                       size_t* key_blob_length) {
        increment(dev);
        int result =
            device(dev)->import_keypair(device(dev), key, key_length, key_blob, key_blob_length);
        if (result == 0)
            munge_blob(*key_blob, *key_blob_length);
        return result;
    }

    static int counting_get_keypair_public(const struct keymaster0_device* dev,
                                           const uint8_t* key_blob, const size_t key_blob_length,
                                           uint8_t** x509_data, size_t* x509_data_length) {
        increment(dev);
        std::unique_ptr<uint8_t[]> dup_blob(unmunge_blob(key_blob, key_blob_length));
        return device(dev)->get_keypair_public(device(dev), dup_blob.get(), key_blob_length,
                                               x509_data, x509_data_length);
    }

    static int counting_delete_keypair(const struct keymaster0_device* dev, const uint8_t* key_blob,
                                       const size_t key_blob_length) {
        increment(dev);
        if (key_blob && key_blob_length > 0)
            EXPECT_EQ('Q', *key_blob);
        if (device(dev)->delete_keypair) {
            std::unique_ptr<uint8_t[]> dup_blob(unmunge_blob(key_blob, key_blob_length));
            return device(dev)->delete_keypair(device(dev), dup_blob.get(), key_blob_length);
        }
        return 0;
    }

    static int counting_delete_all(const struct keymaster0_device* dev) {
        increment(dev);
        if (device(dev)->delete_all)
            return device(dev)->delete_all(device(dev));
        return 0;
    }

    static int counting_sign_data(const struct keymaster0_device* dev, const void* signing_params,
                                  const uint8_t* key_blob, const size_t key_blob_length,
                                  const uint8_t* data, const size_t data_length,
                                  uint8_t** signed_data, size_t* signed_data_length) {
        increment(dev);
        std::unique_ptr<uint8_t[]> dup_blob(unmunge_blob(key_blob, key_blob_length));
        return device(dev)->sign_data(device(dev), signing_params, dup_blob.get(), key_blob_length,
                                      data, data_length, signed_data, signed_data_length);
    }

    static int counting_verify_data(const struct keymaster0_device* dev, const void* signing_params,
                                    const uint8_t* key_blob, const size_t key_blob_length,
                                    const uint8_t* signed_data, const size_t signed_data_length,
                                    const uint8_t* signature, const size_t signature_length) {
        increment(dev);
        std::unique_ptr<uint8_t[]> dup_blob(unmunge_blob(key_blob, key_blob_length));
        return device(dev)->verify_data(device(dev), signing_params, dup_blob.get(),
                                        key_blob_length, signed_data, signed_data_length, signature,
                                        signature_length);
    }

  private:
    keymaster0_device_t* device_;
    int counter_;
};

/**
 * This function takes a keymaster1_device_t and wraps it in an adapter that supports only
 * KM_DIGEST_SHA_2_256.
 */
keymaster1_device_t* make_device_sha256_only(keymaster1_device_t* device);

}  // namespace test
}  // namespace keymaster

#endif  // SYSTEM_KEYMASTER_ANDROID_KEYMASTER_TEST_UTILS_H_
