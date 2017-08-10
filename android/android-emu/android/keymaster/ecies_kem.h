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

#ifndef SYSTEM_KEYMASTER_ECIES_KEM_H_
#define SYSTEM_KEYMASTER_ECIES_KEM_H_

#include "kem.h"

#include <UniquePtr.h>
#include <openssl/ec.h>

#include <keymaster/authorization_set.h>

#include "hkdf.h"
#include "key_exchange.h"

namespace keymaster {

/**
 * EciesKem is an implementation of the key encapsulation mechanism ECIES-KEM described in
 * ISO 18033-2 (http://www.shoup.net/iso/std6.pdf, http://www.shoup.net/papers/iso-2_1.pdf).
 */
class EciesKem : public Kem {
  public:
    virtual ~EciesKem() override {}
    EciesKem(const AuthorizationSet& kem_description, keymaster_error_t* error);

    /* Kem interface. */
    bool Encrypt(const Buffer& peer_public_value, Buffer* output_clear_key,
                 Buffer* output_encrypted_key) override;
    bool Encrypt(const uint8_t* peer_public_value, size_t peer_public_value_len,
                 Buffer* output_clear_key, Buffer* output_encrypted_key) override;

    bool Decrypt(EC_KEY* private_key, const Buffer& encrypted_key, Buffer* output_key) override;
    bool Decrypt(EC_KEY* private_key, const uint8_t* encrypted_key, size_t encrypted_key_len,
                 Buffer* output_key) override;

  private:
    UniquePtr<KeyExchange> key_exchange_;
    UniquePtr<Rfc5869Sha256Kdf> kdf_;
    bool single_hash_mode_;
    uint32_t key_bytes_to_generate_;
    keymaster_ec_curve_t curve_;
};

}  // namespace keymaster

#endif  // SYSTEM_KEYMASTER_ECIES_KEM_H_
