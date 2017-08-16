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

#include "nist_curve_key_exchange.h"

#include <openssl/ec.h>
#include <openssl/ecdh.h>
#include <openssl/err.h>
#include <openssl/evp.h>

#include "openssl_err.h"

namespace keymaster {

NistCurveKeyExchange::NistCurveKeyExchange(EC_KEY* private_key, keymaster_error_t* error)
    : private_key_(private_key) {
    if (!private_key_.get() || !EC_KEY_check_key(private_key_.get())) {
        *error = KM_ERROR_INVALID_ARGUMENT;
        return;
    }
    *error = ExtractPublicKey();
}

/* static */
NistCurveKeyExchange* NistCurveKeyExchange::GenerateKeyExchange(keymaster_ec_curve_t curve) {
    int curve_name;
    switch (curve) {
    case KM_EC_CURVE_P_224:
        curve_name = NID_secp224r1;
        break;
    case KM_EC_CURVE_P_256:
        curve_name = NID_X9_62_prime256v1;
        break;
    case KM_EC_CURVE_P_384:
        curve_name = NID_secp384r1;
        break;
    case KM_EC_CURVE_P_521:
        curve_name = NID_secp521r1;
        break;
    default:
        LOG_E("Not a NIST curve: %d", curve);
        return nullptr;
    }

    UniquePtr<EC_KEY, EC_KEY_Delete> key(EC_KEY_new_by_curve_name(curve_name));
    if (!key.get() || !EC_KEY_generate_key(key.get())) {
        return nullptr;
    }
    keymaster_error_t error;
    NistCurveKeyExchange* key_exchange = new NistCurveKeyExchange(key.release(), &error);
    if (error != KM_ERROR_OK) {
        return nullptr;
    }
    return key_exchange;
}

keymaster_error_t NistCurveKeyExchange::ExtractPublicKey() {
    const EC_GROUP* group = EC_KEY_get0_group(private_key_.get());
    size_t field_len_bits;
    keymaster_error_t error = ec_get_group_size(group, &field_len_bits);
    if (error != KM_ERROR_OK)
        return error;

    shared_secret_len_ = (field_len_bits + 7) / 8;
    public_key_len_ = 1 + 2 * shared_secret_len_;
    public_key_.reset(new uint8_t[public_key_len_]);
    if (EC_POINT_point2oct(group, EC_KEY_get0_public_key(private_key_.get()),
                           POINT_CONVERSION_UNCOMPRESSED, public_key_.get(), public_key_len_,
                           nullptr /* ctx */) != public_key_len_) {
        return TranslateLastOpenSslError();
    }
    return KM_ERROR_OK;
}

bool NistCurveKeyExchange::CalculateSharedKey(const Buffer& peer_public_value,
                                              Buffer* out_result) const {

    return CalculateSharedKey(peer_public_value.peek_read(), peer_public_value.available_read(),
                              out_result);
}

bool NistCurveKeyExchange::CalculateSharedKey(const uint8_t* peer_public_value,
                                              size_t peer_public_value_len,
                                              Buffer* out_result) const {
    const EC_GROUP* group = EC_KEY_get0_group(private_key_.get());
    UniquePtr<EC_POINT, EC_POINT_Delete> point(EC_POINT_new(group));
    if (!point.get() ||
        !EC_POINT_oct2point(/* also test if point is on curve */
                            group, point.get(), peer_public_value, peer_public_value_len,
                            nullptr /* ctx */) ||
        !EC_POINT_is_on_curve(group, point.get(), nullptr /* ctx */)) {
        LOG_E("Can't convert peer public value to point: %d", TranslateLastOpenSslError());
        return false;
    }

    UniquePtr<uint8_t[]> result(new uint8_t[shared_secret_len_]);
    if (ECDH_compute_key(result.get(), shared_secret_len_, point.get(), private_key_.get(),
                         nullptr /* kdf */) != static_cast<int>(shared_secret_len_)) {
        LOG_E("Can't compute ECDH shared key: %d", TranslateLastOpenSslError());
        return false;
    }

    out_result->Reinitialize(result.get(), shared_secret_len_);
    return true;
}

bool NistCurveKeyExchange::public_value(Buffer* public_value) const {
    if (public_key_.get() != nullptr && public_key_len_ != 0) {
        return public_value->Reinitialize(public_key_.get(), public_key_len_);
    }
    return false;
}

}  // namespace keymaster