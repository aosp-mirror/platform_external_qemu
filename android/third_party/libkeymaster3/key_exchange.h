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

#ifndef SYSTEM_KEYMASTER_KEY_EXCHANGE_H_
#define SYSTEM_KEYMASTER_KEY_EXCHANGE_H_

#include <openssl/ec.h>

#include <keymaster/serializable.h>

namespace keymaster {

/**
 * KeyExchange is an abstract class that provides an interface to a
 * key-exchange primitive.
 */
class KeyExchange {
  public:
    virtual ~KeyExchange() {}

    /**
     * CalculateSharedKey computes the shared key between the local private key
     * (which is implicitly known by a KeyExchange object) and a public value
     * from the peer.
     */
    virtual bool CalculateSharedKey(const Buffer& peer_public_value, Buffer* shared_key) const = 0;
    virtual bool CalculateSharedKey(const uint8_t* peer_public_value, size_t peer_public_value_len,
                                    Buffer* shared_key) const = 0;

    /**
     * public_value writes to |public_value| the local public key which can be
     * sent to a peer in order to complete a key exchange.
     */
    virtual bool public_value(Buffer* public_value) const = 0;
};

}  // namespace keymaster

#endif  // SYSTEM_KEYMASTER_KEY_EXCHANGE_H_
