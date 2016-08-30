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

class Keymaster {
public:
    typedef enum {
        TYPE_RSA = 1,
        TYPE_DSA = 2,
        TYPE_EC = 3,
    } keymaster_keypair_t;
    int openssl_generate_keypair(
            const keymaster_keypair_t key_type, const void* key_params,
            uint8_t** keyBlob, uint32_t* keyBlobLength) {return 0;}
    int openssl_import_keypair(const uint8_t* key,
            const uint32_t key_length, uint8_t** key_blob, uint32_t* key_blob_length) {return 0;}
    int openssl_get_keypair_public(
            const uint8_t* key_blob, const uint32_t key_blob_length,
            uint8_t** x509_data, uint32_t* x509_data_length) {return 0;}
    int openssl_sign_data(const void* params,
            const uint8_t* keyBlob, const uint32_t keyBlobLength,
            const uint8_t* data, const uint32_t dataLeggnth, uint8_t** signedData,
            uint32_t* signedDataLength) {return 0;}
    int openssl_verify_data(const void* params,
            const uint8_t* keyBlob, const uint32_t keyBlobLength,
            const uint8_t* signedData, const uint32_t signedDataLength,
            const uint8_t* signature, const uint32_t signatureLength) {return 0;}
};

