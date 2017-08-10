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

#ifndef SYSTEM_KEYMASTER_KDF_H_
#define SYSTEM_KEYMASTER_KDF_H_

#include <hardware/keymaster_defs.h>
#include <keymaster/android_keymaster_utils.h>
#include <keymaster/serializable.h>

#include <UniquePtr.h>

namespace keymaster {

/**
 * A base class for wrapping different key derivation functions.
 */
class Kdf {
  public:
    virtual ~Kdf() { memset_s(secret_key_.get(), 0, secret_key_len_); };
    Kdf();
    bool Init(keymaster_digest_t digest_type, const uint8_t* secret, size_t secret_len,
              const uint8_t* salt, size_t salt_len);
    virtual bool GenerateKey(const uint8_t* info, size_t info_len, uint8_t* output,
                             size_t output_len) = 0;

  protected:
    bool Uint32ToBigEndianByteArray(uint32_t number, uint8_t* output);
    UniquePtr<uint8_t[]> secret_key_;
    size_t secret_key_len_;
    UniquePtr<uint8_t[]> salt_;
    size_t salt_len_;
    bool is_initialized_;
    keymaster_digest_t digest_type_;
    size_t digest_size_;
};

}  // namespace keymaster

#endif  // SYSTEM_KEYMASTER_KDF_H_
