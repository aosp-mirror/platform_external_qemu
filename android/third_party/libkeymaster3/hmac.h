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

#ifndef SYSTEM_KEYMASTER_HMAC_H_
#define SYSTEM_KEYMASTER_HMAC_H_

#include <keymaster/serializable.h>

namespace keymaster {

// Only HMAC-SHA256 is supported.
class HmacSha256 {
  public:
    HmacSha256(){};

    // DigestLength returns the length, in bytes, of the resulting digest.
    size_t DigestLength() const;

    // Initializes this instance using |key|. Call Init only once. It returns
    // false on the second of later calls.
    bool Init(const uint8_t* key, size_t key_length);
    bool Init(const Buffer& key);

    // Sign calculates the HMAC of |data| with the key supplied to the Init
    // method. At most |digest_len| bytes of the resulting digest are written
    // to |digest|.
    bool Sign(const Buffer& data, uint8_t* digest, size_t digest_len) const;
    bool Sign(const uint8_t* data, size_t data_len, uint8_t* digest, size_t digest_len) const;

    // Verify returns true if |digest| is a valid HMAC of |data| using the key
    // supplied to Init. |digest| must be exactly |DigestLength()| bytes long.
    // Use of this method is strongly recommended over using Sign() with a manual
    // comparison (such as memcmp), as such comparisons may result in
    // side-channel disclosures, such as timing, that undermine the cryptographic
    // integrity.
    bool Verify(const Buffer& data, const Buffer& digest) const;
    bool Verify(const uint8_t* data, size_t data_len, const uint8_t* digest,
                size_t digest_len) const;

  private:
    UniquePtr<uint8_t[]> key_;
    size_t key_len_;
};

}  // namespace keymaster

#endif  // SYSTEM_KEYMASTER_HMAC_H_
