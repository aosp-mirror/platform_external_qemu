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

#ifndef SYSTEM_KEYMASTER_NIST_CURVE_KEY_EXCHANGE_H_
#define SYSTEM_KEYMASTER_NIST_CURVE_KEY_EXCHANGE_H_

#include "key_exchange.h"

#include <keymaster/authorization_set.h>
#include <hardware/keymaster_defs.h>

#include <UniquePtr.h>

#include "openssl_utils.h"

namespace keymaster {

/**
 * NistCurveKeyExchange implements a KeyExchange using elliptic-curve
 * Diffie-Hellman on NIST curves: P-224, P-256, P-384 and P-521.
 */
class NistCurveKeyExchange : public KeyExchange {
  public:
    ~NistCurveKeyExchange() override {}

    /**
     * NistCurveKeyExchange takes ownership of \p private_key.
     */
    NistCurveKeyExchange(EC_KEY* private_key, keymaster_error_t* error);

    /**
     * GenerateKeyExchange generates a new public/private key pair on a NIST curve and returns
     * a new key exchange object.
     */
    static NistCurveKeyExchange* GenerateKeyExchange(keymaster_ec_curve_t curve);

    /**
     * KeyExchange interface.
     */
    bool CalculateSharedKey(const uint8_t* peer_public_value, size_t peer_public_value_len,
                            Buffer* shared_key) const override;
    bool CalculateSharedKey(const Buffer& peer_public_value, Buffer* shared_key) const override;
    bool public_value(Buffer* public_value) const override;

    /* Caller takes ownership of \p private_key. */
    EC_KEY* private_key() { return private_key_.release(); }

  private:
    keymaster_error_t ExtractPublicKey();

    UniquePtr<EC_KEY, EC_KEY_Delete> private_key_;
    UniquePtr<uint8_t[]> public_key_;
    size_t public_key_len_;
    size_t shared_secret_len_;
};

}  // namespace keymaster

#endif  // SYSTEM_KEYMASTER_NIST_CURVE_KEY_EXCHANGE_H_