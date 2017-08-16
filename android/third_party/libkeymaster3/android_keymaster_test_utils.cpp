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

#include "android_keymaster_test_utils.h"

#include <algorithm>

#include <openssl/rand.h>

#include <keymaster/android_keymaster_messages.h>
#include <keymaster/android_keymaster_utils.h>

using std::copy_if;
using std::find_if;
using std::is_permutation;
using std::ostream;
using std::string;
using std::vector;

#ifndef KEYMASTER_NAME_TAGS
#error Keymaster test code requires that KEYMASTER_NAME_TAGS is defined
#endif

std::ostream& operator<<(std::ostream& os, const keymaster_key_param_t& param) {
    os << "Tag: " << keymaster::StringifyTag(param.tag);
    switch (keymaster_tag_get_type(param.tag)) {
    case KM_INVALID:
        os << " Invalid";
        break;
    case KM_UINT_REP:
        os << " (Rep)";
    /* Falls through */
    case KM_UINT:
        os << " Int: " << param.integer;
        break;
    case KM_ENUM_REP:
        os << " (Rep)";
    /* Falls through */
    case KM_ENUM:
        os << " Enum: " << param.enumerated;
        break;
    case KM_ULONG_REP:
        os << " (Rep)";
    /* Falls through */
    case KM_ULONG:
        os << " Long: " << param.long_integer;
        break;
    case KM_DATE:
        os << " Date: " << param.date_time;
        break;
    case KM_BOOL:
        os << " Bool: " << param.boolean;
        break;
    case KM_BIGNUM:
        os << " Bignum: ";
        if (!param.blob.data)
            os << "(null)";
        else
            for (size_t i = 0; i < param.blob.data_length; ++i)
                os << std::hex << std::setw(2) << static_cast<int>(param.blob.data[i]) << std::dec;
        break;
    case KM_BYTES:
        os << " Bytes: ";
        if (!param.blob.data)
            os << "(null)";
        else
            for (size_t i = 0; i < param.blob.data_length; ++i)
                os << std::hex << std::setw(2) << static_cast<int>(param.blob.data[i]) << std::dec;
        break;
    }
    return os;
}

bool operator==(const keymaster_key_param_t& a, const keymaster_key_param_t& b) {
    if (a.tag != b.tag) {
        return false;
    }

    switch (keymaster_tag_get_type(a.tag)) {
    case KM_INVALID:
        return true;
    case KM_UINT_REP:
    case KM_UINT:
        return a.integer == b.integer;
    case KM_ENUM_REP:
    case KM_ENUM:
        return a.enumerated == b.enumerated;
    case KM_ULONG:
    case KM_ULONG_REP:
        return a.long_integer == b.long_integer;
    case KM_DATE:
        return a.date_time == b.date_time;
    case KM_BOOL:
        return a.boolean == b.boolean;
    case KM_BIGNUM:
    case KM_BYTES:
        if ((a.blob.data == NULL || b.blob.data == NULL) && a.blob.data != b.blob.data)
            return false;
        return a.blob.data_length == b.blob.data_length &&
               (memcmp(a.blob.data, b.blob.data, a.blob.data_length) == 0);
    }

    return false;
}

static char hex_value[256] = {
    0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  0,  0,
    0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  0,  0,
    0, 1,  2,  3,  4,  5,  6,  7, 8, 9, 0, 0, 0, 0, 0, 0,  // '0'..'9'
    0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 'A'..'F'
    0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10, 11, 12, 13, 14, 15, 0,
    0, 0,  0,  0,  0,  0,  0,  0,  // 'a'..'f'
    0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  0,  0,
    0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  0,  0,
    0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  0,  0,
    0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  0,  0,
    0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  0,  0,
    0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  0,  0};

string hex2str(string a) {
    string b;
    size_t num = a.size() / 2;
    b.resize(num);
    for (size_t i = 0; i < num; i++) {
        b[i] = (hex_value[a[i * 2] & 0xFF] << 4) + (hex_value[a[i * 2 + 1] & 0xFF]);
    }
    return b;
}

namespace keymaster {

bool operator==(const AuthorizationSet& a, const AuthorizationSet& b) {
    if (a.size() != b.size())
        return false;

    for (size_t i = 0; i < a.size(); ++i)
        if (!(a[i] == b[i]))
            return false;
    return true;
}

bool operator!=(const AuthorizationSet& a, const AuthorizationSet& b) {
    return !(a == b);
}

std::ostream& operator<<(std::ostream& os, const AuthorizationSet& set) {
    if (set.size() == 0)
        os << "(Empty)" << std::endl;
    else {
        os << "\n";
        for (size_t i = 0; i < set.size(); ++i)
            os << set[i] << std::endl;
    }
    return os;
}

namespace test {

std::ostream& operator<<(std::ostream& os, const InstanceCreatorPtr& instance_creator) {
    return os << instance_creator->name();
}

Keymaster2Test::Keymaster2Test() : op_handle_(OP_HANDLE_SENTINEL) {
    memset(&characteristics_, 0, sizeof(characteristics_));
    blob_.key_material = nullptr;
    RAND_seed("foobar", 6);
    blob_.key_material = 0;
    device_ = GetParam()->CreateDevice();
}

Keymaster2Test::~Keymaster2Test() {
    FreeCharacteristics();
    FreeKeyBlob();
    device_->common.close(reinterpret_cast<hw_device_t*>(device_));
}

keymaster2_device_t* Keymaster2Test::device() {
    return device_;
}

keymaster_error_t Keymaster2Test::GenerateKey(const AuthorizationSetBuilder& builder) {
    AuthorizationSet params(builder.build());
    params.push_back(UserAuthParams());
    params.push_back(ClientParams());

    FreeKeyBlob();
    FreeCharacteristics();
    return device()->generate_key(device(), &params, &blob_, &characteristics_);
}

keymaster_error_t Keymaster2Test::DeleteKey() {
    return device()->delete_key(device(), &blob_);
}

keymaster_error_t Keymaster2Test::ImportKey(const AuthorizationSetBuilder& builder,
                                            keymaster_key_format_t format,
                                            const string& key_material) {
    AuthorizationSet params(builder.build());
    params.push_back(UserAuthParams());
    params.push_back(ClientParams());

    FreeKeyBlob();
    FreeCharacteristics();
    keymaster_blob_t key = {reinterpret_cast<const uint8_t*>(key_material.c_str()),
                            key_material.length()};
    return device()->import_key(device(), &params, format, &key, &blob_, &characteristics_);
}

AuthorizationSet Keymaster2Test::UserAuthParams() {
    AuthorizationSet set;
    set.push_back(TAG_USER_ID, 7);
    set.push_back(TAG_USER_AUTH_TYPE, HW_AUTH_PASSWORD);
    set.push_back(TAG_AUTH_TIMEOUT, 300);
    return set;
}

AuthorizationSet Keymaster2Test::ClientParams() {
    AuthorizationSet set;
    set.push_back(TAG_APPLICATION_ID, "app_id", 6);
    return set;
}

keymaster_error_t Keymaster2Test::BeginOperation(keymaster_purpose_t purpose) {
    AuthorizationSet in_params(client_params());
    keymaster_key_param_set_t out_params;
    keymaster_error_t error =
        device()->begin(device(), purpose, &blob_, &in_params, &out_params, &op_handle_);
    EXPECT_EQ(0U, out_params.length);
    EXPECT_TRUE(out_params.params == nullptr);
    return error;
}

keymaster_error_t Keymaster2Test::BeginOperation(keymaster_purpose_t purpose,
                                                 const AuthorizationSet& input_set,
                                                 AuthorizationSet* output_set) {
    keymaster_key_param_set_t out_params;
    keymaster_error_t error =
        device()->begin(device(), purpose, &blob_, &input_set, &out_params, &op_handle_);
    if (error == KM_ERROR_OK) {
        if (output_set) {
            output_set->Reinitialize(out_params);
        } else {
            EXPECT_EQ(0U, out_params.length);
            EXPECT_TRUE(out_params.params == nullptr);
        }
        keymaster_free_param_set(&out_params);
    }
    return error;
}

keymaster_error_t Keymaster2Test::UpdateOperation(const string& message, string* output,
                                                  size_t* input_consumed) {
    EXPECT_NE(op_handle_, OP_HANDLE_SENTINEL);
    keymaster_blob_t input = {reinterpret_cast<const uint8_t*>(message.c_str()), message.length()};
    keymaster_blob_t out_tmp;
    keymaster_key_param_set_t out_params;
    keymaster_error_t error = device()->update(device(), op_handle_, nullptr /* params */, &input,
                                               input_consumed, &out_params, &out_tmp);
    if (error == KM_ERROR_OK && out_tmp.data)
        output->append(reinterpret_cast<const char*>(out_tmp.data), out_tmp.data_length);
    free(const_cast<uint8_t*>(out_tmp.data));
    return error;
}

keymaster_error_t Keymaster2Test::UpdateOperation(const AuthorizationSet& additional_params,
                                                  const string& message,
                                                  AuthorizationSet* output_params, string* output,
                                                  size_t* input_consumed) {
    EXPECT_NE(op_handle_, OP_HANDLE_SENTINEL);
    keymaster_blob_t input = {reinterpret_cast<const uint8_t*>(message.c_str()), message.length()};
    keymaster_blob_t out_tmp;
    keymaster_key_param_set_t out_params;
    keymaster_error_t error = device()->update(device(), op_handle_, &additional_params, &input,
                                               input_consumed, &out_params, &out_tmp);
    if (error == KM_ERROR_OK && out_tmp.data)
        output->append(reinterpret_cast<const char*>(out_tmp.data), out_tmp.data_length);
    free((void*)out_tmp.data);
    if (output_params)
        output_params->Reinitialize(out_params);
    keymaster_free_param_set(&out_params);
    return error;
}

keymaster_error_t Keymaster2Test::FinishOperation(string* output) {
    return FinishOperation("", "", output);
}

keymaster_error_t Keymaster2Test::FinishOperation(const string& input, const string& signature,
                                                  string* output) {
    AuthorizationSet additional_params;
    AuthorizationSet output_params;
    return FinishOperation(additional_params, input, signature, &output_params, output);
}

keymaster_error_t Keymaster2Test::FinishOperation(const AuthorizationSet& additional_params,
                                                  const string& input, const string& signature,
                                                  AuthorizationSet* output_params, string* output) {
    keymaster_blob_t inp = {reinterpret_cast<const uint8_t*>(input.c_str()), input.length()};
    keymaster_blob_t sig = {reinterpret_cast<const uint8_t*>(signature.c_str()),
                            signature.length()};
    keymaster_blob_t out_tmp;
    keymaster_key_param_set_t out_params;
    keymaster_error_t error = device()->finish(device(), op_handle_, &additional_params, &inp, &sig,
                                               &out_params, &out_tmp);
    if (error != KM_ERROR_OK) {
        EXPECT_TRUE(out_tmp.data == nullptr);
        EXPECT_TRUE(out_params.params == nullptr);
        return error;
    }

    if (out_tmp.data)
        output->append(reinterpret_cast<const char*>(out_tmp.data), out_tmp.data_length);
    free((void*)out_tmp.data);
    if (output_params)
        output_params->Reinitialize(out_params);
    keymaster_free_param_set(&out_params);
    return error;
}

keymaster_error_t Keymaster2Test::AbortOperation() {
    return device()->abort(device(), op_handle_);
}

keymaster_error_t Keymaster2Test::AttestKey(const string& attest_challenge,
                                            keymaster_cert_chain_t* cert_chain) {
    AuthorizationSet attest_params;
    attest_params.push_back(UserAuthParams());
    attest_params.push_back(ClientParams());
    attest_params.push_back(TAG_ATTESTATION_CHALLENGE, attest_challenge.data(),
                            attest_challenge.length());
    return device()->attest_key(device(), &blob_, &attest_params, cert_chain);
}

keymaster_error_t Keymaster2Test::UpgradeKey(const AuthorizationSet& upgrade_params) {
    keymaster_key_blob_t upgraded_blob;
    keymaster_error_t error =
        device()->upgrade_key(device(), &blob_, &upgrade_params, &upgraded_blob);
    if (error == KM_ERROR_OK) {
        FreeKeyBlob();
        blob_ = upgraded_blob;
    }
    return error;
}

string Keymaster2Test::ProcessMessage(keymaster_purpose_t purpose, const string& message) {
    EXPECT_EQ(KM_ERROR_OK, BeginOperation(purpose, client_params(), NULL /* output_params */));

    string result;
    EXPECT_EQ(KM_ERROR_OK, FinishOperation(message, "" /* signature */, &result));
    return result;
}

string Keymaster2Test::ProcessMessage(keymaster_purpose_t purpose, const string& message,
                                      const AuthorizationSet& begin_params,
                                      const AuthorizationSet& update_params,
                                      AuthorizationSet* begin_out_params) {
    EXPECT_EQ(KM_ERROR_OK, BeginOperation(purpose, begin_params, begin_out_params));

    string result;
    EXPECT_EQ(KM_ERROR_OK, FinishOperation(update_params, message, "" /* signature */, &result));
    return result;
}

string Keymaster2Test::ProcessMessage(keymaster_purpose_t purpose, const string& message,
                                      const string& signature, const AuthorizationSet& begin_params,
                                      const AuthorizationSet& update_params,
                                      AuthorizationSet* output_params) {
    EXPECT_EQ(KM_ERROR_OK, BeginOperation(purpose, begin_params, output_params));

    string result;
    EXPECT_EQ(KM_ERROR_OK, FinishOperation(update_params, message, signature, &result));
    return result;
}

string Keymaster2Test::ProcessMessage(keymaster_purpose_t purpose, const string& message,
                                      const string& signature) {
    EXPECT_EQ(KM_ERROR_OK, BeginOperation(purpose, client_params(), NULL /* output_params */));

    string result;
    EXPECT_EQ(KM_ERROR_OK, FinishOperation(message, signature, &result));
    return result;
}

void Keymaster2Test::SignMessage(const string& message, string* signature,
                                 keymaster_digest_t digest) {
    SCOPED_TRACE("SignMessage");
    AuthorizationSet input_params(AuthorizationSet(client_params_, array_length(client_params_)));
    input_params.push_back(TAG_DIGEST, digest);
    AuthorizationSet update_params;
    AuthorizationSet output_params;
    *signature =
        ProcessMessage(KM_PURPOSE_SIGN, message, input_params, update_params, &output_params);
    EXPECT_GT(signature->size(), 0U);
}

void Keymaster2Test::SignMessage(const string& message, string* signature,
                                 keymaster_digest_t digest, keymaster_padding_t padding) {
    SCOPED_TRACE("SignMessage");
    AuthorizationSet input_params(AuthorizationSet(client_params_, array_length(client_params_)));
    input_params.push_back(TAG_DIGEST, digest);
    input_params.push_back(TAG_PADDING, padding);
    AuthorizationSet update_params;
    AuthorizationSet output_params;
    *signature =
        ProcessMessage(KM_PURPOSE_SIGN, message, input_params, update_params, &output_params);
    EXPECT_GT(signature->size(), 0U);
}

void Keymaster2Test::MacMessage(const string& message, string* signature, size_t mac_length) {
    SCOPED_TRACE("SignMessage");
    AuthorizationSet input_params(AuthorizationSet(client_params_, array_length(client_params_)));
    input_params.push_back(TAG_MAC_LENGTH, mac_length);
    AuthorizationSet update_params;
    AuthorizationSet output_params;
    *signature =
        ProcessMessage(KM_PURPOSE_SIGN, message, input_params, update_params, &output_params);
    EXPECT_GT(signature->size(), 0U);
}

void Keymaster2Test::VerifyMessage(const string& message, const string& signature,
                                   keymaster_digest_t digest) {
    SCOPED_TRACE("VerifyMessage");
    AuthorizationSet input_params(client_params());
    input_params.push_back(TAG_DIGEST, digest);
    AuthorizationSet update_params;
    AuthorizationSet output_params;
    ProcessMessage(KM_PURPOSE_VERIFY, message, signature, input_params, update_params,
                   &output_params);
}

void Keymaster2Test::VerifyMessage(const string& message, const string& signature,
                                   keymaster_digest_t digest, keymaster_padding_t padding) {
    SCOPED_TRACE("VerifyMessage");
    AuthorizationSet input_params(client_params());
    input_params.push_back(TAG_DIGEST, digest);
    input_params.push_back(TAG_PADDING, padding);
    AuthorizationSet update_params;
    AuthorizationSet output_params;
    ProcessMessage(KM_PURPOSE_VERIFY, message, signature, input_params, update_params,
                   &output_params);
}

void Keymaster2Test::VerifyMac(const string& message, const string& signature) {
    SCOPED_TRACE("VerifyMac");
    ProcessMessage(KM_PURPOSE_VERIFY, message, signature);
}

string Keymaster2Test::EncryptMessage(const string& message, keymaster_padding_t padding,
                                      string* generated_nonce) {
    SCOPED_TRACE("EncryptMessage");
    AuthorizationSet begin_params(client_params()), output_params;
    begin_params.push_back(TAG_PADDING, padding);
    AuthorizationSet update_params;
    string ciphertext =
        ProcessMessage(KM_PURPOSE_ENCRYPT, message, begin_params, update_params, &output_params);
    if (generated_nonce) {
        keymaster_blob_t nonce_blob;
        EXPECT_TRUE(output_params.GetTagValue(TAG_NONCE, &nonce_blob));
        *generated_nonce = make_string(nonce_blob.data, nonce_blob.data_length);
    } else {
        EXPECT_EQ(-1, output_params.find(TAG_NONCE));
    }
    return ciphertext;
}

string Keymaster2Test::EncryptMessage(const string& message, keymaster_digest_t digest,
                                      keymaster_padding_t padding, string* generated_nonce) {
    AuthorizationSet update_params;
    return EncryptMessage(update_params, message, digest, padding, generated_nonce);
}

string Keymaster2Test::EncryptMessage(const string& message, keymaster_block_mode_t block_mode,
                                      keymaster_padding_t padding, string* generated_nonce) {
    AuthorizationSet update_params;
    return EncryptMessage(update_params, message, block_mode, padding, generated_nonce);
}

string Keymaster2Test::EncryptMessage(const AuthorizationSet& update_params, const string& message,
                                      keymaster_digest_t digest, keymaster_padding_t padding,
                                      string* generated_nonce) {
    SCOPED_TRACE("EncryptMessage");
    AuthorizationSet begin_params(client_params()), output_params;
    begin_params.push_back(TAG_PADDING, padding);
    begin_params.push_back(TAG_DIGEST, digest);
    string ciphertext =
        ProcessMessage(KM_PURPOSE_ENCRYPT, message, begin_params, update_params, &output_params);
    if (generated_nonce) {
        keymaster_blob_t nonce_blob;
        EXPECT_TRUE(output_params.GetTagValue(TAG_NONCE, &nonce_blob));
        *generated_nonce = make_string(nonce_blob.data, nonce_blob.data_length);
    } else {
        EXPECT_EQ(-1, output_params.find(TAG_NONCE));
    }
    return ciphertext;
}

string Keymaster2Test::EncryptMessage(const AuthorizationSet& update_params, const string& message,
                                      keymaster_block_mode_t block_mode,
                                      keymaster_padding_t padding, string* generated_nonce) {
    SCOPED_TRACE("EncryptMessage");
    AuthorizationSet begin_params(client_params()), output_params;
    begin_params.push_back(TAG_PADDING, padding);
    begin_params.push_back(TAG_BLOCK_MODE, block_mode);
    string ciphertext =
        ProcessMessage(KM_PURPOSE_ENCRYPT, message, begin_params, update_params, &output_params);
    if (generated_nonce) {
        keymaster_blob_t nonce_blob;
        EXPECT_TRUE(output_params.GetTagValue(TAG_NONCE, &nonce_blob));
        *generated_nonce = make_string(nonce_blob.data, nonce_blob.data_length);
    } else {
        EXPECT_EQ(-1, output_params.find(TAG_NONCE));
    }
    return ciphertext;
}

string Keymaster2Test::EncryptMessageWithParams(const string& message,
                                                const AuthorizationSet& begin_params,
                                                const AuthorizationSet& update_params,
                                                AuthorizationSet* output_params) {
    SCOPED_TRACE("EncryptMessageWithParams");
    return ProcessMessage(KM_PURPOSE_ENCRYPT, message, begin_params, update_params, output_params);
}

string Keymaster2Test::DecryptMessage(const string& ciphertext, keymaster_padding_t padding) {
    SCOPED_TRACE("DecryptMessage");
    AuthorizationSet begin_params(client_params());
    begin_params.push_back(TAG_PADDING, padding);
    AuthorizationSet update_params;
    return ProcessMessage(KM_PURPOSE_DECRYPT, ciphertext, begin_params, update_params);
}

string Keymaster2Test::DecryptMessage(const string& ciphertext, keymaster_digest_t digest,
                                      keymaster_padding_t padding) {
    SCOPED_TRACE("DecryptMessage");
    AuthorizationSet begin_params(client_params());
    begin_params.push_back(TAG_PADDING, padding);
    begin_params.push_back(TAG_DIGEST, digest);
    AuthorizationSet update_params;
    return ProcessMessage(KM_PURPOSE_DECRYPT, ciphertext, begin_params, update_params);
}

string Keymaster2Test::DecryptMessage(const string& ciphertext, keymaster_block_mode_t block_mode,
                                      keymaster_padding_t padding) {
    SCOPED_TRACE("DecryptMessage");
    AuthorizationSet begin_params(client_params());
    begin_params.push_back(TAG_PADDING, padding);
    begin_params.push_back(TAG_BLOCK_MODE, block_mode);
    AuthorizationSet update_params;
    return ProcessMessage(KM_PURPOSE_DECRYPT, ciphertext, begin_params, update_params);
}

string Keymaster2Test::DecryptMessage(const string& ciphertext, keymaster_digest_t digest,
                                      keymaster_padding_t padding, const string& nonce) {
    SCOPED_TRACE("DecryptMessage");
    AuthorizationSet begin_params(client_params());
    begin_params.push_back(TAG_PADDING, padding);
    begin_params.push_back(TAG_DIGEST, digest);
    begin_params.push_back(TAG_NONCE, nonce.data(), nonce.size());
    AuthorizationSet update_params;
    return ProcessMessage(KM_PURPOSE_DECRYPT, ciphertext, begin_params, update_params);
}

string Keymaster2Test::DecryptMessage(const string& ciphertext, keymaster_block_mode_t block_mode,
                                      keymaster_padding_t padding, const string& nonce) {
    SCOPED_TRACE("DecryptMessage");
    AuthorizationSet begin_params(client_params());
    begin_params.push_back(TAG_PADDING, padding);
    begin_params.push_back(TAG_BLOCK_MODE, block_mode);
    begin_params.push_back(TAG_NONCE, nonce.data(), nonce.size());
    AuthorizationSet update_params;
    return ProcessMessage(KM_PURPOSE_DECRYPT, ciphertext, begin_params, update_params);
}

string Keymaster2Test::DecryptMessage(const AuthorizationSet& update_params,
                                      const string& ciphertext, keymaster_digest_t digest,
                                      keymaster_padding_t padding, const string& nonce) {
    SCOPED_TRACE("DecryptMessage");
    AuthorizationSet begin_params(client_params());
    begin_params.push_back(TAG_PADDING, padding);
    begin_params.push_back(TAG_DIGEST, digest);
    begin_params.push_back(TAG_NONCE, nonce.data(), nonce.size());
    return ProcessMessage(KM_PURPOSE_DECRYPT, ciphertext, begin_params, update_params);
}

keymaster_error_t Keymaster2Test::GetCharacteristics() {
    FreeCharacteristics();
    return device()->get_key_characteristics(device(), &blob_, &client_id_, NULL /* app_data */,
                                             &characteristics_);
}

keymaster_error_t Keymaster2Test::ExportKey(keymaster_key_format_t format, string* export_data) {
    keymaster_blob_t export_tmp;
    keymaster_error_t error = device()->export_key(device(), format, &blob_, &client_id_,
                                                   NULL /* app_data */, &export_tmp);

    if (error != KM_ERROR_OK)
        return error;

    *export_data = string(reinterpret_cast<const char*>(export_tmp.data), export_tmp.data_length);
    free((void*)export_tmp.data);
    return error;
}

void Keymaster2Test::CheckHmacTestVector(const string& key, const string& message,
                                         keymaster_digest_t digest, string expected_mac) {
    ASSERT_EQ(KM_ERROR_OK, ImportKey(AuthorizationSetBuilder()
                                         .HmacKey(key.size() * 8)
                                         .Authorization(TAG_MIN_MAC_LENGTH, expected_mac.size() * 8)
                                         .Digest(digest),
                                     KM_KEY_FORMAT_RAW, key));
    string signature;
    MacMessage(message, &signature, expected_mac.size() * 8);
    EXPECT_EQ(expected_mac, signature) << "Test vector didn't match for digest " << (int)digest;
}

void Keymaster2Test::CheckAesCtrTestVector(const string& key, const string& nonce,
                                           const string& message,
                                           const string& expected_ciphertext) {
    ASSERT_EQ(KM_ERROR_OK, ImportKey(AuthorizationSetBuilder()
                                         .AesEncryptionKey(key.size() * 8)
                                         .Authorization(TAG_BLOCK_MODE, KM_MODE_CTR)
                                         .Authorization(TAG_CALLER_NONCE)
                                         .Padding(KM_PAD_NONE),
                                     KM_KEY_FORMAT_RAW, key));

    AuthorizationSet begin_params(client_params()), update_params, output_params;
    begin_params.push_back(TAG_NONCE, nonce.data(), nonce.size());
    begin_params.push_back(TAG_BLOCK_MODE, KM_MODE_CTR);
    begin_params.push_back(TAG_PADDING, KM_PAD_NONE);
    string ciphertext =
        EncryptMessageWithParams(message, begin_params, update_params, &output_params);
    EXPECT_EQ(expected_ciphertext, ciphertext);
}

AuthorizationSet Keymaster2Test::hw_enforced() {
    return AuthorizationSet(characteristics_.hw_enforced);
}

AuthorizationSet Keymaster2Test::sw_enforced() {
    return AuthorizationSet(characteristics_.sw_enforced);
}

void Keymaster2Test::FreeCharacteristics() {
    keymaster_free_characteristics(&characteristics_);
}

void Keymaster2Test::FreeKeyBlob() {
    free(const_cast<uint8_t*>(blob_.key_material));
    blob_.key_material = NULL;
}

void Keymaster2Test::corrupt_key_blob() {
    assert(blob_.key_material);
    uint8_t* tmp = const_cast<uint8_t*>(blob_.key_material);
    ++tmp[blob_.key_material_size / 2];
}

class Sha256OnlyWrapper {
  public:
    explicit Sha256OnlyWrapper(const keymaster1_device_t* wrapped_device)
        : wrapped_device_(wrapped_device) {

        new_module = *wrapped_device_->common.module;
        new_module_name = std::string("SHA 256-only ") + wrapped_device_->common.module->name;
        new_module.name = new_module_name.c_str();

        memset(&device_, 0, sizeof(device_));
        device_.common.module = &new_module;

        device_.common.close = close_device;
        device_.get_supported_algorithms = get_supported_algorithms;
        device_.get_supported_block_modes = get_supported_block_modes;
        device_.get_supported_padding_modes = get_supported_padding_modes;
        device_.get_supported_digests = get_supported_digests;
        device_.get_supported_import_formats = get_supported_import_formats;
        device_.get_supported_export_formats = get_supported_export_formats;
        device_.add_rng_entropy = add_rng_entropy;
        device_.generate_key = generate_key;
        device_.get_key_characteristics = get_key_characteristics;
        device_.import_key = import_key;
        device_.export_key = export_key;
        device_.begin = begin;
        device_.update = update;
        device_.finish = finish;
        device_.abort = abort;
    }

    keymaster1_device_t* keymaster_device() { return &device_; }

    static bool is_supported(keymaster_digest_t digest) {
        return digest == KM_DIGEST_NONE || digest == KM_DIGEST_SHA_2_256;
    }

    static bool all_digests_supported(const keymaster_key_param_set_t* params) {
        for (size_t i = 0; i < params->length; ++i)
            if (params->params[i].tag == TAG_DIGEST)
                if (!is_supported(static_cast<keymaster_digest_t>(params->params[i].enumerated)))
                    return false;
        return true;
    }

    static const keymaster_key_param_t*
    get_algorithm_param(const keymaster_key_param_set_t* params) {
        keymaster_key_param_t* end = params->params + params->length;
        auto alg_ptr = std::find_if(params->params, end, [](keymaster_key_param_t& p) {
            return p.tag == KM_TAG_ALGORITHM;
        });
        if (alg_ptr == end)
            return nullptr;
        return alg_ptr;
    }

    static int close_device(hw_device_t* dev) {
        Sha256OnlyWrapper* wrapper = reinterpret_cast<Sha256OnlyWrapper*>(dev);
        const keymaster1_device_t* wrapped_device = wrapper->wrapped_device_;
        delete wrapper;
        return wrapped_device->common.close(const_cast<hw_device_t*>(&wrapped_device->common));
    }

    static const keymaster1_device_t* unwrap(const keymaster1_device_t* dev) {
        return reinterpret_cast<const Sha256OnlyWrapper*>(dev)->wrapped_device_;
    }

    static keymaster_error_t get_supported_algorithms(const struct keymaster1_device* dev,
                                                      keymaster_algorithm_t** algorithms,
                                                      size_t* algorithms_length) {
        return unwrap(dev)->get_supported_algorithms(unwrap(dev), algorithms, algorithms_length);
    }
    static keymaster_error_t get_supported_block_modes(const struct keymaster1_device* dev,
                                                       keymaster_algorithm_t algorithm,
                                                       keymaster_purpose_t purpose,
                                                       keymaster_block_mode_t** modes,
                                                       size_t* modes_length) {
        return unwrap(dev)->get_supported_block_modes(unwrap(dev), algorithm, purpose, modes,
                                                      modes_length);
    }
    static keymaster_error_t get_supported_padding_modes(const struct keymaster1_device* dev,
                                                         keymaster_algorithm_t algorithm,
                                                         keymaster_purpose_t purpose,
                                                         keymaster_padding_t** modes,
                                                         size_t* modes_length) {
        return unwrap(dev)->get_supported_padding_modes(unwrap(dev), algorithm, purpose, modes,
                                                        modes_length);
    }

    static keymaster_error_t get_supported_digests(const keymaster1_device_t* dev,
                                                   keymaster_algorithm_t algorithm,
                                                   keymaster_purpose_t purpose,
                                                   keymaster_digest_t** digests,
                                                   size_t* digests_length) {
        keymaster_error_t error = unwrap(dev)->get_supported_digests(
            unwrap(dev), algorithm, purpose, digests, digests_length);
        if (error != KM_ERROR_OK)
            return error;

        std::vector<keymaster_digest_t> filtered_digests;
        std::copy_if(*digests, *digests + *digests_length, std::back_inserter(filtered_digests),
                     [](keymaster_digest_t digest) { return is_supported(digest); });

        free(*digests);
        *digests_length = filtered_digests.size();
        *digests = reinterpret_cast<keymaster_digest_t*>(
            malloc(*digests_length * sizeof(keymaster_digest_t)));
        std::copy(filtered_digests.begin(), filtered_digests.end(), *digests);

        return KM_ERROR_OK;
    }

    static keymaster_error_t get_supported_import_formats(const struct keymaster1_device* dev,
                                                          keymaster_algorithm_t algorithm,
                                                          keymaster_key_format_t** formats,
                                                          size_t* formats_length) {
        return unwrap(dev)->get_supported_import_formats(unwrap(dev), algorithm, formats,
                                                         formats_length);
    }
    static keymaster_error_t get_supported_export_formats(const struct keymaster1_device* dev,
                                                          keymaster_algorithm_t algorithm,
                                                          keymaster_key_format_t** formats,
                                                          size_t* formats_length) {
        return unwrap(dev)->get_supported_export_formats(unwrap(dev), algorithm, formats,
                                                         formats_length);
    }
    static keymaster_error_t add_rng_entropy(const struct keymaster1_device* dev,
                                             const uint8_t* data, size_t data_length) {
        return unwrap(dev)->add_rng_entropy(unwrap(dev), data, data_length);
    }

    static keymaster_error_t generate_key(const keymaster1_device_t* dev,
                                          const keymaster_key_param_set_t* params,
                                          keymaster_key_blob_t* key_blob,
                                          keymaster_key_characteristics_t** characteristics) {
        auto alg_ptr = get_algorithm_param(params);
        if (!alg_ptr)
            return KM_ERROR_UNSUPPORTED_ALGORITHM;
        if (alg_ptr->enumerated == KM_ALGORITHM_HMAC && !all_digests_supported(params))
            return KM_ERROR_UNSUPPORTED_DIGEST;

        return unwrap(dev)->generate_key(unwrap(dev), params, key_blob, characteristics);
    }

    static keymaster_error_t
    get_key_characteristics(const struct keymaster1_device* dev,
                            const keymaster_key_blob_t* key_blob, const keymaster_blob_t* client_id,
                            const keymaster_blob_t* app_data,
                            keymaster_key_characteristics_t** characteristics) {
        return unwrap(dev)->get_key_characteristics(unwrap(dev), key_blob, client_id, app_data,
                                                    characteristics);
    }

    static keymaster_error_t
    import_key(const keymaster1_device_t* dev, const keymaster_key_param_set_t* params,
               keymaster_key_format_t key_format, const keymaster_blob_t* key_data,
               keymaster_key_blob_t* key_blob, keymaster_key_characteristics_t** characteristics) {
        auto alg_ptr = get_algorithm_param(params);
        if (!alg_ptr)
            return KM_ERROR_UNSUPPORTED_ALGORITHM;
        if (alg_ptr->enumerated == KM_ALGORITHM_HMAC && !all_digests_supported(params))
            return KM_ERROR_UNSUPPORTED_DIGEST;

        return unwrap(dev)->import_key(unwrap(dev), params, key_format, key_data, key_blob,
                                       characteristics);
    }

    static keymaster_error_t export_key(const struct keymaster1_device* dev,  //
                                        keymaster_key_format_t export_format,
                                        const keymaster_key_blob_t* key_to_export,
                                        const keymaster_blob_t* client_id,
                                        const keymaster_blob_t* app_data,
                                        keymaster_blob_t* export_data) {
        return unwrap(dev)->export_key(unwrap(dev), export_format, key_to_export, client_id,
                                       app_data, export_data);
    }

    static keymaster_error_t begin(const keymaster1_device_t* dev,  //
                                   keymaster_purpose_t purpose, const keymaster_key_blob_t* key,
                                   const keymaster_key_param_set_t* in_params,
                                   keymaster_key_param_set_t* out_params,
                                   keymaster_operation_handle_t* operation_handle) {
        if (!all_digests_supported(in_params))
            return KM_ERROR_UNSUPPORTED_DIGEST;
        return unwrap(dev)->begin(unwrap(dev), purpose, key, in_params, out_params,
                                  operation_handle);
    }

    static keymaster_error_t update(const keymaster1_device_t* dev,
                                    keymaster_operation_handle_t operation_handle,
                                    const keymaster_key_param_set_t* in_params,
                                    const keymaster_blob_t* input, size_t* input_consumed,
                                    keymaster_key_param_set_t* out_params,
                                    keymaster_blob_t* output) {
        return unwrap(dev)->update(unwrap(dev), operation_handle, in_params, input, input_consumed,
                                   out_params, output);
    }

    static keymaster_error_t finish(const struct keymaster1_device* dev,  //
                                    keymaster_operation_handle_t operation_handle,
                                    const keymaster_key_param_set_t* in_params,
                                    const keymaster_blob_t* signature,
                                    keymaster_key_param_set_t* out_params,
                                    keymaster_blob_t* output) {
        return unwrap(dev)->finish(unwrap(dev), operation_handle, in_params, signature, out_params,
                                   output);
    }

    static keymaster_error_t abort(const struct keymaster1_device* dev,
                                   keymaster_operation_handle_t operation_handle) {
        return unwrap(dev)->abort(unwrap(dev), operation_handle);
    }

  private:
    keymaster1_device_t device_;
    const keymaster1_device_t* wrapped_device_;
    hw_module_t new_module;
    string new_module_name;
};

keymaster1_device_t* make_device_sha256_only(keymaster1_device_t* device) {
    return (new Sha256OnlyWrapper(device))->keymaster_device();
}

}  // namespace test
}  // namespace keymaster
