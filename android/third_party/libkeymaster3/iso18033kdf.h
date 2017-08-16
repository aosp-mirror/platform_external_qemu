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

#ifndef SYSTEM_KEYMASTER_ISO18033KDF_H_
#define SYSTEM_KEYMASTER_ISO18033KDF_H_

#include "kdf.h"

#include <hardware/keymaster_defs.h>

#include <keymaster/serializable.h>

#include <UniquePtr.h>

namespace keymaster {

/**
 * A light implementation of ISO18033KDF as defined by ISO-18033-2 (www.shoup.net/iso/std6.pdf) and
 * its slightly different variant ANSI-X9-42.
 */
class Iso18033Kdf : public Kdf {
  public:
    ~Iso18033Kdf() {}

    bool Init(keymaster_digest_t digest_type, const uint8_t* secret, size_t secret_len) {
        return Kdf::Init(digest_type, secret, secret_len, nullptr /* salt */, 0 /* salt_len */);
    }

    /**
     * Generates ISO18033's derived key, as defined in ISO-18033-2 and ANSI-X9-42. In ISO 18033-2,
     * KDF takes a secret and outputs:
     *
     * hash(secret || start_counter) || hash(secret|| start_counter + 1) || ...
     *
     * In ANSI-X9-42, KDF takes a secret and additional info, and outputs:
     *
     * hash(secret || start_counter || info) || hash(secret || start_counter + 1 || info) || ...
     *
     * Note that the KDFs are the same if the info field is considered optional.  In both cases the
     * length of the output is specified by the caller, and the counter is encoded as a 4-element
     * byte array.
     */
    bool GenerateKey(const uint8_t* info, size_t info_len, uint8_t* output,
                     size_t output_len) override;

  protected:
    explicit Iso18033Kdf(uint32_t start_counter) : start_counter_(start_counter) {}

  private:
    uint32_t start_counter_;
};

}  // namespace keymaster

#endif  // SYSTEM_KEYMASTER_ISO18033KDF_H_
