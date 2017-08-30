/*
 * Copyright 2015 The Android Open Source Project
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

#include "rsa_keymaster1_operation.h"

#include <memory>

#include <keymaster/android_keymaster_utils.h>

#include "openssl_err.h"
#include "openssl_utils.h"
#include "rsa_keymaster1_key.h"

using std::unique_ptr;

namespace keymaster {

keymaster_error_t RsaKeymaster1WrappedOperation::Begin(EVP_PKEY* rsa_key,
                                                       const AuthorizationSet& input_params) {
    Keymaster1Engine::KeyData* key_data = engine_->GetData(rsa_key);
    if (!key_data)
        return KM_ERROR_UNKNOWN_ERROR;

    // Copy the input params and substitute KM_DIGEST_NONE for whatever was specified.  Also change
    // KM_PAD_RSA_PSS and KM_PAD_OAEP to KM_PAD_NONE, if necessary. These are the params we'll pass
    // to the hardware module.  The regular Rsa*Operation classes will do software digesting and
    // padding where we've told the HW not to.
    //
    // The reason we don't change KM_PAD_RSA_PKCS1_1_5_SIGN or KM_PAD_RSA_PKCS1_1_5_ENCRYPT to
    // KM_PAD_NONE is because the hardware can perform those padding modes, since they don't involve
    // digesting.
    //
    // We also cache in the key the padding value that we expect to be passed to the engine crypto
    // operation.  This just allows us to double-check that the correct padding value is reaching
    // that layer.
    AuthorizationSet begin_params(input_params);
    int pos = begin_params.find(TAG_DIGEST);
    if (pos == -1)
        return KM_ERROR_UNSUPPORTED_DIGEST;
    begin_params[pos].enumerated = KM_DIGEST_NONE;

    pos = begin_params.find(TAG_PADDING);
    if (pos == -1)
        return KM_ERROR_UNSUPPORTED_PADDING_MODE;
    switch (begin_params[pos].enumerated) {

    case KM_PAD_RSA_PSS:
    case KM_PAD_RSA_OAEP:
        key_data->expected_openssl_padding = RSA_NO_PADDING;
        begin_params[pos].enumerated = KM_PAD_NONE;
        break;

    case KM_PAD_RSA_PKCS1_1_5_ENCRYPT:
    case KM_PAD_RSA_PKCS1_1_5_SIGN:
        key_data->expected_openssl_padding = RSA_PKCS1_PADDING;
        break;
    }

    return engine_->device()->begin(engine_->device(), purpose_, &key_data->key_material,
                                    &begin_params, nullptr /* out_params */, &operation_handle_);
}

keymaster_error_t
RsaKeymaster1WrappedOperation::PrepareFinish(EVP_PKEY* rsa_key,
                                             const AuthorizationSet& input_params) {
    Keymaster1Engine::KeyData* key_data = engine_->GetData(rsa_key);
    if (!key_data) {
        LOG_E("Could not get extended key data... not a Keymaster1Engine key?", 0);
        return KM_ERROR_UNKNOWN_ERROR;
    }
    key_data->op_handle = operation_handle_;
    key_data->finish_params.Reinitialize(input_params);

    return KM_ERROR_OK;
}

keymaster_error_t RsaKeymaster1WrappedOperation::Abort() {
    return engine_->device()->abort(engine_->device(), operation_handle_);
}

keymaster_error_t RsaKeymaster1WrappedOperation::GetError(EVP_PKEY* rsa_key) {
    Keymaster1Engine::KeyData* key_data = engine_->GetData(rsa_key);  // key_data is owned by rsa
    if (!key_data)
        return KM_ERROR_UNKNOWN_ERROR;
    return key_data->error;
}

static EVP_PKEY* GetEvpKey(const RsaKeymaster1Key& key, keymaster_error_t* error) {
    if (!key.key()) {
        *error = KM_ERROR_UNKNOWN_ERROR;
        return nullptr;
    }

    UniquePtr<EVP_PKEY, EVP_PKEY_Delete> pkey(EVP_PKEY_new());
    if (!key.InternalToEvp(pkey.get())) {
        *error = KM_ERROR_UNKNOWN_ERROR;
        return nullptr;
    }
    return pkey.release();
}

Operation* RsaKeymaster1OperationFactory::CreateOperation(const Key& key,
                                                          const AuthorizationSet& begin_params,
                                                          keymaster_error_t* error) {
    keymaster_digest_t digest;
    if (!GetAndValidateDigest(begin_params, key, &digest, error))
        return nullptr;

    keymaster_padding_t padding;
    if (!GetAndValidatePadding(begin_params, key, &padding, error))
        return nullptr;

    const RsaKeymaster1Key& rsa_km1_key(static_cast<const RsaKeymaster1Key&>(key));
    unique_ptr<EVP_PKEY, EVP_PKEY_Delete> rsa(GetEvpKey(rsa_km1_key, error));
    if (!rsa)
        return nullptr;

    switch (purpose_) {
    case KM_PURPOSE_SIGN:
        return new RsaKeymaster1Operation<RsaSignOperation>(digest, padding, rsa.release(),
                                                            engine_);
    case KM_PURPOSE_DECRYPT:
        return new RsaKeymaster1Operation<RsaDecryptOperation>(digest, padding, rsa.release(),
                                                               engine_);
    default:
        LOG_E("Bug: Pubkey operation requested.  Those should be handled by normal RSA operations.",
              0);
        *error = KM_ERROR_UNSUPPORTED_PURPOSE;
        return nullptr;
    }
}

static const keymaster_digest_t supported_digests[] = {
    KM_DIGEST_NONE,      KM_DIGEST_MD5,       KM_DIGEST_SHA1,     KM_DIGEST_SHA_2_224,
    KM_DIGEST_SHA_2_256, KM_DIGEST_SHA_2_384, KM_DIGEST_SHA_2_512};

const keymaster_digest_t*
RsaKeymaster1OperationFactory::SupportedDigests(size_t* digest_count) const {
    *digest_count = array_length(supported_digests);
    return supported_digests;
}

static const keymaster_padding_t supported_sig_padding[] = {
    KM_PAD_NONE, KM_PAD_RSA_PKCS1_1_5_SIGN, KM_PAD_RSA_PSS,
};
static const keymaster_padding_t supported_crypt_padding[] = {
    KM_PAD_NONE, KM_PAD_RSA_PKCS1_1_5_ENCRYPT, KM_PAD_RSA_OAEP,
};

const keymaster_padding_t*
RsaKeymaster1OperationFactory::SupportedPaddingModes(size_t* padding_mode_count) const {
    switch (purpose_) {
    case KM_PURPOSE_SIGN:
    case KM_PURPOSE_VERIFY:
        *padding_mode_count = array_length(supported_sig_padding);
        return supported_sig_padding;
    case KM_PURPOSE_ENCRYPT:
    case KM_PURPOSE_DECRYPT:
        *padding_mode_count = array_length(supported_crypt_padding);
        return supported_crypt_padding;
    default:
        *padding_mode_count = 0;
        return nullptr;
    }
}

}  // namespace keymaster
