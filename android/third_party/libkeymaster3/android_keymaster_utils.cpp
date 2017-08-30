/*
 * Copyright 2014 The Android Open Source Project
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

#include <keymaster/android_keymaster_utils.h>

#include <new>

namespace keymaster {

// Keymaster never manages enormous buffers, so anything particularly large is bad data or the
// result of a bug.  We arbitrarily set a 16 MiB limit.
const size_t kMaxDupBufferSize = 16 * 1024 * 1024;

uint8_t* dup_buffer(const void* buf, size_t size) {
    if (size >= kMaxDupBufferSize)
        return nullptr;
    uint8_t* retval = new (std::nothrow) uint8_t[size];
    if (retval)
        memcpy(retval, buf, size);
    return retval;
}

int memcmp_s(const void* p1, const void* p2, size_t length) {
    const uint8_t* s1 = static_cast<const uint8_t*>(p1);
    const uint8_t* s2 = static_cast<const uint8_t*>(p2);
    uint8_t result = 0;
    while (length-- > 0)
        result |= *s1++ ^ *s2++;
    return result == 0 ? 0 : 1;
}

keymaster_error_t EcKeySizeToCurve(uint32_t key_size_bits, keymaster_ec_curve_t* curve) {
    switch (key_size_bits) {
    default:
        return KM_ERROR_UNSUPPORTED_KEY_SIZE;

    case 224:
        *curve = KM_EC_CURVE_P_224;
        break;

    case 256:
        *curve = KM_EC_CURVE_P_256;
        break;

    case 384:
        *curve = KM_EC_CURVE_P_384;
        break;

    case 521:
        *curve = KM_EC_CURVE_P_521;
        break;
    }

    return KM_ERROR_OK;
}

keymaster_error_t EcCurveToKeySize(keymaster_ec_curve_t curve, uint32_t* key_size_bits) {
    switch (curve) {
    default:
        return KM_ERROR_UNSUPPORTED_EC_CURVE;

    case KM_EC_CURVE_P_224:
        *key_size_bits = 224;
        break;

    case KM_EC_CURVE_P_256:
        *key_size_bits = 256;
        break;

    case KM_EC_CURVE_P_384:
        *key_size_bits = 384;
        break;

    case KM_EC_CURVE_P_521:
        *key_size_bits = 521;
        break;
    }

    return KM_ERROR_OK;
}

}  // namespace keymaster
