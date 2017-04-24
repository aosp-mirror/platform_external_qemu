/* Copyright (C) 2017 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/

#include "android/emulation/android_qemud.h"
#include "android/keymaster/Keymaster.h"
#include "android/keymaster/keymaster_common.h"
#include "android/utils/debug.h"

#include <assert.h>
#include <memory>

#define DEBUG

#ifdef DEBUG
#  include <stdio.h>
#  define  D(...)   ( fprintf( stderr, __VA_ARGS__ ) )
#else
#  define  D(...)   ((void)0)
#endif

template <class _T>
static _T unpack(uint8_t** data) {
    _T ret = *(_T*)(*data);
    *data = *data + sizeof(_T);
    return ret;
}

static QemudService* sKeystoreService = nullptr;

// Read param from src.
// Return a pointer for the param
static void* s_readGenParam(uint8_t* src,
    const keymaster_keypair_t type) {
    switch (type) {
        case TYPE_DSA: {
            keymaster_dsa_keygen_params_t* dsaParams =
                    new keymaster_dsa_keygen_params_t();
            dsaParams->key_size = unpack<uint32_t>(&src);
            dsaParams->generator_len = unpack<uint32_t>(&src);
            dsaParams->prime_p_len = unpack<uint32_t>(&src);
            dsaParams->prime_q_len = unpack<uint32_t>(&src);
            dsaParams->generator = src;
            src += dsaParams->generator_len;
            dsaParams->prime_p = src;
            src += dsaParams->prime_p_len;
            dsaParams->prime_q = src;
            src += dsaParams->prime_q_len;
            return (void*)dsaParams;
            break;
        }
        case TYPE_EC: {
            keymaster_ec_keygen_params_t* ecParams =
                    new keymaster_ec_keygen_params_t();
            ecParams->field_size = unpack<uint32_t>(&src);
            D("EC key field size %d\n", ecParams->field_size);
            return (void*)ecParams;
            break;
        }
        case TYPE_RSA: {
            keymaster_rsa_keygen_params_t* rsaParams =
                new keymaster_rsa_keygen_params_t();
            rsaParams->modulus_size = unpack<uint32_t>(&src);
            rsaParams->public_exponent = unpack<uint64_t>(&src);
            return (void*)rsaParams;
            break;
        }
        default:
            D("Unsupported key type %d\n", type);
            return nullptr;
    }
}

static void s_releaseGenKeyParam(const keymaster_keypair_t type,
        void** param) {
    switch (type) {
        case TYPE_DSA: {
            keymaster_dsa_keygen_params_t* dsaParams =
                    (keymaster_dsa_keygen_params_t*)*param;
            delete [] dsaParams->generator;
            delete [] dsaParams->prime_p;
            delete [] dsaParams->prime_q;
            delete dsaParams;
            break;
        }
        case TYPE_EC: {
            keymaster_ec_keygen_params_t* ecParams =
                    (keymaster_ec_keygen_params_t*)*param;
            delete ecParams;
            break;
        }
        case TYPE_RSA: {
            keymaster_rsa_keygen_params_t* rsaParams =
                    (keymaster_rsa_keygen_params_t*)*param;
            delete rsaParams;
            break;
        }
        default:
            D("Unsupported key type %d\n", type);
    }
    *param = nullptr;
}

// Read param from src.
// Return a pointer for the param
static void* s_readSignParam(uint8_t* src,
    const keymaster_keypair_t type) {
    switch (type) {
        case TYPE_DSA: {
            keymaster_dsa_sign_params_t* dsaParams =
                new keymaster_dsa_sign_params_t();
            dsaParams->digest_type = unpack<keymaster_digest_algorithm_t>(&src);
            return (void*)dsaParams;
        }
        case TYPE_EC: {
            keymaster_ec_sign_params_t* ecParams =
                new keymaster_ec_sign_params_t();
            ecParams->digest_type = unpack<keymaster_digest_algorithm_t>(&src);
            return (void*)ecParams;
        }
        case TYPE_RSA: {
            keymaster_rsa_sign_params_t* rsaParams =
                new keymaster_rsa_sign_params_t();
            rsaParams->digest_type = unpack<keymaster_digest_algorithm_t>(&src);
            rsaParams->padding_type = unpack<keymaster_rsa_padding_t>(&src);
            return (void*)rsaParams;
        }
        default:
            D("Unsupported key type %d\n", type);
            return nullptr;
    }
}

static void s_releaseSignKeyParam(const keymaster_keypair_t type,
        void** param) {
    switch (type) {
        case TYPE_DSA: {
            keymaster_dsa_sign_params_t* dsaParams =
                    (keymaster_dsa_sign_params_t*)*param;
            delete dsaParams;
            break;
        }
        case TYPE_EC: {
            keymaster_ec_sign_params_t* ecParams =
                    (keymaster_ec_sign_params_t*)*param;
            delete ecParams;
            break;
        }
        case TYPE_RSA: {
            keymaster_rsa_sign_params_t* rsaParams =
                    (keymaster_rsa_sign_params_t*)*param;
            delete rsaParams;
            break;
        }
        default:
            D("Unsupported key type %d\n", type);
    }
    *param = nullptr;
}

static void s_keystoreRecv(void* opaque, uint8_t* msg, int msglen,
                      QemudClient*  client ) {
    enum {
        GenerateKeypair = 0,
        ImportKeypair,
        GetKeypairPublic,
        SignData,
        VerifyData,
    };
    uint8_t* data = msg;
    int32_t cmd = unpack<int32_t>(&data);
    switch (cmd) {
        case GenerateKeypair:
            {
                D("%s: generating key\n", __FUNCTION__);
                // input parameters
                keymaster_keypair_t keyType =
                        (keymaster_keypair_t)unpack<int32_t>(&data);
                uint32_t keyParamsLen = unpack<uint32_t>(&data);
                void* keyParams = s_readGenParam(data, keyType);
                data += keyParamsLen;
                assert(data == msg + msglen);
                // output parameters
                uint8_t* keyBlob = nullptr;
                uint32_t keyBlobLength = 0;
                int ret = Keymaster::generateKeypair(keyType,
                        keyParams, &keyBlob, &keyBlobLength);
                qemud_client_send(client, (uint8_t*)&keyBlobLength,
                        sizeof(keyBlobLength));
                if (keyBlobLength) {
                    qemud_client_send(client, (uint8_t*)keyBlob, keyBlobLength);
                }
                qemud_client_send(client, (uint8_t*)&ret, sizeof(ret));
                D("%s: send back done\n", __FUNCTION__);
                s_releaseGenKeyParam(keyType, &keyParams);
                delete [] keyBlob;
            }
            break;
        case ImportKeypair:
            {
                D("%s: importing key pair\n", __FUNCTION__);
                // input parameters
                uint32_t keyLen = unpack<uint32_t>(&data);
                const uint8_t* key = (uint8_t*)data;
                data += keyLen;
                // output parameters
                uint8_t* keyBlob = nullptr;
                uint32_t keyBlobLength;
                int ret = Keymaster::importKeypair(key, keyLen,
                        &keyBlob, &keyBlobLength);
                qemud_client_send(client, (uint8_t*)&keyBlobLength,
                        sizeof(keyBlobLength));
                qemud_client_send(client, (uint8_t*)keyBlob, keyBlobLength);
                qemud_client_send(client, (uint8_t*)&ret, sizeof(ret));
                delete [] keyBlob;
            }
            break;
        case GetKeypairPublic:
            {
                D("%s: getting key pair\n", __FUNCTION__);
                // input parameters
                uint32_t keyBlobLen = unpack<uint32_t>(&data);
                const uint8_t* keyBlob = (uint8_t*)data;
                data += keyBlobLen;
                assert(data == msg + msglen);
                // output parameters
                uint8_t* x509Data = nullptr;
                uint32_t x509DataLen;
                int32_t ret = Keymaster::getKeypairPublic(keyBlob,
                        keyBlobLen, &x509Data, &x509DataLen);
                qemud_client_send(client, (uint8_t*)&x509DataLen,
                        sizeof(x509DataLen));
                if (x509DataLen) {
                    qemud_client_send(client, (uint8_t*)x509Data, x509DataLen);
                }
                qemud_client_send(client, (uint8_t*)&ret, sizeof(ret));
                delete [] x509Data;
                D("%s: getting key pair done\n", __FUNCTION__);
            }
            break;
        case SignData:
            {
                D("%s: %d signing data\n", __FUNCTION__, __LINE__);
                // input parameters
                keymaster_keypair_t keyType =
                        (keymaster_keypair_t)unpack<int32_t>(&data);
                uint32_t keyParamsLen = unpack<uint32_t>(&data);
                void* keyParams = s_readSignParam(data, keyType);
                data += keyParamsLen;
                uint32_t keyBlobLen = unpack<uint32_t>(&data);
                const uint8_t* keyBlob = (uint8_t*)data;
                data += keyBlobLen;
                uint32_t rawDataLen = unpack<uint32_t>(&data);
                const uint8_t* rawData = (uint8_t*)data;
                data += rawDataLen;
                // output parameters
                uint8_t* signedData = nullptr;
                uint32_t signedDataLen;
                int ret = Keymaster::signData(keyParams,
                        keyBlob, keyBlobLen,
                        rawData, rawDataLen,
                        &signedData, &signedDataLen);
                qemud_client_send(client, (uint8_t*)&signedDataLen,
                        sizeof(signedDataLen));
                if (signedDataLen) {
                    qemud_client_send(client, (uint8_t*)signedData, signedDataLen);
                }
                qemud_client_send(client, (uint8_t*)&ret, sizeof(ret));
                D("%s: %d signing data\n", __FUNCTION__, __LINE__);
                delete [] signedData;
                s_releaseSignKeyParam(keyType, &keyParams);
                D("%s: %d signing data\n", __FUNCTION__, __LINE__);
            }
            break;
        case VerifyData:
            {
                D("%s: verifying data\n", __FUNCTION__);
                // input parameters
                keymaster_keypair_t keyType =
                        (keymaster_keypair_t)unpack<int32_t>(&data);
                uint32_t keyParamsLen = unpack<uint32_t>(&data);
                void* keyParams = s_readSignParam(data, keyType);
                data += keyParamsLen;
                uint32_t keyBlobLen = unpack<uint32_t>(&data);
                const uint8_t* keyBlob = (uint8_t*)data;
                data += keyBlobLen;
                uint32_t signedDataLen = unpack<uint32_t>(&data);
                const uint8_t* signedData = (uint8_t*)data;
                data += signedDataLen;
                uint32_t signatureLen = unpack<uint32_t>(&data);
                const uint8_t* signature = (uint8_t*)data;
                data += signatureLen;
                // output parameters
                int ret = Keymaster::verifyData(keyParams,
                        keyBlob, keyBlobLen,
                        signedData, signedDataLen,
                        signature, signatureLen);
                qemud_client_send(client, (uint8_t*)&ret, sizeof(ret));
                s_releaseSignKeyParam(keyType, &keyParams);
            }
            break;
        default:
            // No this does not happen.
            assert(0);
    }
}

static QemudClient* s_keystoreConnect(void*  opaque,
                    QemudService*  service,
                    int  channel,
                    const char* client_param) {
    QemudClient* client  = qemud_client_new(service, channel, client_param,
            nullptr, s_keystoreRecv, nullptr, nullptr, nullptr);
    qemud_client_set_framing(client, 1);
    D("%s: new keystore client registered\n", __FUNCTION__);
    return client;
}

extern "C" void android_hw_keystore_init() {
    if (!sKeystoreService) {
        sKeystoreService = qemud_service_register("KeymasterService", 0, nullptr,
                s_keystoreConnect, nullptr, nullptr);
        D("%s: keystore qemud listen service initialized\n", __FUNCTION__);
    }
}