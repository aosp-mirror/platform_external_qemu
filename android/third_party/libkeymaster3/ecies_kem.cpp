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

#include "ecies_kem.h"

#include "nist_curve_key_exchange.h"
#include "openssl_err.h"

namespace keymaster {

EciesKem::EciesKem(const AuthorizationSet& kem_description, keymaster_error_t* error) {
    const AuthorizationSet& authorizations(kem_description);

    if (!authorizations.GetTagValue(TAG_EC_CURVE, &curve_)) {
        LOG_E("%s", "EciesKem: no curve specified");
        *error = KM_ERROR_INVALID_ARGUMENT;
        return;
    }

    switch (curve_) {
    case KM_EC_CURVE_P_224:
    case KM_EC_CURVE_P_256:
    case KM_EC_CURVE_P_384:
    case KM_EC_CURVE_P_521:
        break;
    default:
        LOG_E("EciesKem: curve %d is unsupported", curve_);
        *error = KM_ERROR_UNSUPPORTED_EC_CURVE;
        return;
    }

    keymaster_kdf_t kdf;
    if (!authorizations.GetTagValue(TAG_KDF, &kdf)) {
        LOG_E("EciesKem: No KDF specified", 0);
        *error = KM_ERROR_UNSUPPORTED_KDF;
        return;
    }
    switch (kdf) {
    case KM_KDF_RFC5869_SHA256:
        kdf_.reset(new Rfc5869Sha256Kdf());
        break;
    default:
        LOG_E("Kdf %d is unsupported", kdf);
        *error = KM_ERROR_UNSUPPORTED_KDF;
        return;
    }
    if (!kdf_.get()) {
        *error = KM_ERROR_MEMORY_ALLOCATION_FAILED;
        return;
    }

    if (!authorizations.GetTagValue(TAG_KEY_SIZE, &key_bytes_to_generate_)) {
        LOG_E("%s", "EciesKem: no key length specified");
        *error = KM_ERROR_UNSUPPORTED_KEY_SIZE;
        return;
    }

    single_hash_mode_ = authorizations.GetTagValue(TAG_ECIES_SINGLE_HASH_MODE);
    *error = KM_ERROR_OK;
}

bool EciesKem::Encrypt(const Buffer& peer_public_value, Buffer* output_clear_key,
                       Buffer* output_encrypted_key) {
    return Encrypt(peer_public_value.peek_read(), peer_public_value.available_read(),
                   output_clear_key, output_encrypted_key);
}

// http://www.shoup.net/iso/std6.pdf, section 10.2.3.
bool EciesKem::Encrypt(const uint8_t* peer_public_value, size_t peer_public_value_len,
                       Buffer* output_clear_key, Buffer* output_encrypted_key) {

    key_exchange_.reset(NistCurveKeyExchange::GenerateKeyExchange(curve_));
    if (!key_exchange_.get()) {
        return false;
    }

    Buffer shared_secret;
    if (!key_exchange_->CalculateSharedKey(peer_public_value, peer_public_value_len,
                                           &shared_secret)) {
        LOG_E("EciesKem: ECDH failed, can't obtain shared secret", 0);
        return false;
    }
    if (!key_exchange_->public_value(output_encrypted_key)) {
        LOG_E("EciesKem: Can't obtain public value", 0);
        return false;
    }

    Buffer z;
    if (single_hash_mode_) {
        // z is empty.
    } else {
        // z = C0
        z.Reinitialize(output_encrypted_key->peek_read(), output_encrypted_key->available_read());
    }

    Buffer actual_secret(z.available_read() + shared_secret.available_read());
    actual_secret.write(z.peek_read(), z.available_read());
    actual_secret.write(shared_secret.peek_read(), shared_secret.available_read());

    if (!kdf_->Init(actual_secret.peek_read(), actual_secret.available_read(), nullptr /* salt */,
                    0 /* salt_len */)) {
        LOG_E("EciesKem: KDF failed, can't derived keys", 0);
        return false;
    }
    output_clear_key->Reinitialize(key_bytes_to_generate_);
    if (!kdf_->GenerateKey(nullptr /* info */, 0 /* info length */, output_clear_key->peek_write(),
                           key_bytes_to_generate_)) {
        LOG_E("EciesKem: KDF failed, can't derived keys", 0);
        return false;
    }
    output_clear_key->advance_write(key_bytes_to_generate_);

    return true;
}

bool EciesKem::Decrypt(EC_KEY* private_key, const Buffer& encrypted_key, Buffer* output_key) {
    return Decrypt(private_key, encrypted_key.peek_read(), encrypted_key.available_read(),
                   output_key);
}

// http://www.shoup.net/iso/std6.pdf, section 10.2.4.
bool EciesKem::Decrypt(EC_KEY* private_key, const uint8_t* encrypted_key, size_t encrypted_key_len,
                       Buffer* output_key) {

    keymaster_error_t error;
    key_exchange_.reset(new NistCurveKeyExchange(private_key, &error));
    if (!key_exchange_.get() || error != KM_ERROR_OK) {
        return false;
    }

    Buffer shared_secret;
    if (!key_exchange_->CalculateSharedKey(encrypted_key, encrypted_key_len, &shared_secret)) {
        LOG_E("EciesKem: ECDH failed, can't obtain shared secret", 0);
        return false;
    }

    Buffer public_value;
    if (!key_exchange_->public_value(&public_value)) {
        LOG_E("%s", "EciesKem: Can't obtain public value");
        return false;
    }

    Buffer z;
    if (single_hash_mode_) {
        // z is empty.
    } else {
        // z = C0
        z.Reinitialize(public_value.peek_read(), public_value.available_read());
    }

    Buffer actual_secret(z.available_read() + shared_secret.available_read());
    actual_secret.write(z.peek_read(), z.available_read());
    actual_secret.write(shared_secret.peek_read(), shared_secret.available_read());

    if (!kdf_->Init(actual_secret.peek_read(), actual_secret.available_read(), nullptr /* salt */,
                    0 /* salt_len */)) {
        LOG_E("%s", "EciesKem: KDF failed, can't derived keys");
        return false;
    }

    output_key->Reinitialize(key_bytes_to_generate_);
    if (!kdf_->GenerateKey(nullptr /* info */, 0 /* info_len */, output_key->peek_write(),
                           key_bytes_to_generate_)) {
        LOG_E("%s", "EciesKem: KDF failed, can't derived keys");
        return false;
    }
    output_key->advance_write(key_bytes_to_generate_);

    return true;
}

}  // namespace keymaster
