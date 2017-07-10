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

#include "openssl_utils.h"

#include <keymaster/android_keymaster_utils.h>

#include "openssl_err.h"

namespace keymaster {

keymaster_error_t ec_get_group_size(const EC_GROUP* group, size_t* key_size_bits) {
    switch (EC_GROUP_get_curve_name(group)) {
    case NID_secp224r1:
        *key_size_bits = 224;
        break;
    case NID_X9_62_prime256v1:
        *key_size_bits = 256;
        break;
    case NID_secp384r1:
        *key_size_bits = 384;
        break;
    case NID_secp521r1:
        *key_size_bits = 521;
        break;
    default:
        return KM_ERROR_UNSUPPORTED_EC_FIELD;
    }
    return KM_ERROR_OK;
}

EC_GROUP* ec_get_group(keymaster_ec_curve_t curve) {
    switch (curve) {
    case KM_EC_CURVE_P_224:
        return EC_GROUP_new_by_curve_name(NID_secp224r1);
        break;
    case KM_EC_CURVE_P_256:
        return EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1);
        break;
    case KM_EC_CURVE_P_384:
        return EC_GROUP_new_by_curve_name(NID_secp384r1);
        break;
    case KM_EC_CURVE_P_521:
        return EC_GROUP_new_by_curve_name(NID_secp521r1);
        break;
    default:
        return nullptr;
        break;
    }
}

void convert_bn_to_blob(BIGNUM* bn, keymaster_blob_t* blob) {
    blob->data_length = BN_num_bytes(bn);
    blob->data = new uint8_t[blob->data_length];
    BN_bn2bin(bn, const_cast<uint8_t*>(blob->data));
}

static int convert_to_evp(keymaster_algorithm_t algorithm) {
    switch (algorithm) {
    case KM_ALGORITHM_RSA:
        return EVP_PKEY_RSA;
    case KM_ALGORITHM_EC:
        return EVP_PKEY_EC;
    default:
        return -1;
    };
}

keymaster_error_t convert_pkcs8_blob_to_evp(const uint8_t* key_data, size_t key_length,
                                            keymaster_algorithm_t expected_algorithm,
                                            UniquePtr<EVP_PKEY, EVP_PKEY_Delete>* pkey) {
    if (key_data == NULL || key_length <= 0)
        return KM_ERROR_INVALID_KEY_BLOB;

    UniquePtr<PKCS8_PRIV_KEY_INFO, PKCS8_PRIV_KEY_INFO_Delete> pkcs8(
        d2i_PKCS8_PRIV_KEY_INFO(NULL, &key_data, key_length));
    if (pkcs8.get() == NULL)
        return TranslateLastOpenSslError(true /* log_message */);

    pkey->reset(EVP_PKCS82PKEY(pkcs8.get()));
    if (!pkey->get())
        return TranslateLastOpenSslError(true /* log_message */);

    if (EVP_PKEY_type((*pkey)->type) != convert_to_evp(expected_algorithm)) {
        LOG_E("EVP key algorithm was %d, not the expected %d", EVP_PKEY_type((*pkey)->type),
              convert_to_evp(expected_algorithm));
        return KM_ERROR_INVALID_KEY_BLOB;
    }

    return KM_ERROR_OK;
}

keymaster_error_t KeyMaterialToEvpKey(keymaster_key_format_t key_format,
                                      const KeymasterKeyBlob& key_material,
                                      keymaster_algorithm_t expected_algorithm,
                                      UniquePtr<EVP_PKEY, EVP_PKEY_Delete>* pkey) {
    if (key_format != KM_KEY_FORMAT_PKCS8)
        return KM_ERROR_UNSUPPORTED_KEY_FORMAT;

    return convert_pkcs8_blob_to_evp(key_material.key_material, key_material.key_material_size,
                                     expected_algorithm, pkey);
}

keymaster_error_t EvpKeyToKeyMaterial(const EVP_PKEY* pkey, KeymasterKeyBlob* key_blob) {
    int key_data_size = i2d_PrivateKey(pkey, NULL /* key_data*/);
    if (key_data_size <= 0)
        return TranslateLastOpenSslError();

    if (!key_blob->Reset(key_data_size))
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;

    uint8_t* tmp = key_blob->writable_data();
    i2d_PrivateKey(pkey, &tmp);

    return KM_ERROR_OK;
}

size_t ec_group_size_bits(EC_KEY* ec_key) {
    const EC_GROUP* group = EC_KEY_get0_group(ec_key);
    UniquePtr<BN_CTX, BN_CTX_Delete> bn_ctx(BN_CTX_new());
    UniquePtr<BIGNUM, BIGNUM_Delete> order(BN_new());
    if (!EC_GROUP_get_order(group, order.get(), bn_ctx.get())) {
        LOG_E("Failed to get EC group order", 0);
        return 0;
    }
    return BN_num_bits(order.get());
}

}  // namespace keymaster
