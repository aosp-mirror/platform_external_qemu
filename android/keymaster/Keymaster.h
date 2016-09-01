// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

#include "android/keymaster/keymaster_common.h"

class Keymaster {
public:
    int generateKeypair(
            const keymaster_keypair_t keyType, const void* keyParams,
            uint8_t** keyBlob, uint32_t* keyBlobLength);
    int importKeypair(const uint8_t* key, const uint32_t keyLength,
            uint8_t** keyBlob, uint32_t* keyBlobLength);
    int getKeypairPublic(const uint8_t* keyBlob, const uint32_t keyBlobLength,
            uint8_t** x509Data, uint32_t* x509DataLength);
    int signData(const void* params,
            const uint8_t* keyBlob, const uint32_t keyBlobLength,
            const uint8_t* data, const uint32_t dataLength, uint8_t** signedData,
            uint32_t* signedDataLength);
    int verifyData(const void* params,
            const uint8_t* keyBlob, const uint32_t keyBlobLength,
            const uint8_t* signedData, const uint32_t signedDataLength,
            const uint8_t* signature, const uint32_t signatureLength);
};

