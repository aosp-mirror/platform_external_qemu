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
                const void* keyParams = (void*)data;
                data += keyParamsLen;
                // output parameters
                uint8_t* keyBlob;
                uint32_t keyBlobLength;
                int ret = Keymaster::generateKeypair(keyType,
                        keyParams, &keyBlob, &keyBlobLength);
                qemud_client_send(client, (uint8_t*)&keyBlobLength,
                        sizeof(keyBlobLength));
                qemud_client_send(client, (uint8_t*)keyBlob, keyBlobLength);
                qemud_client_send(client, (uint8_t*)&ret, sizeof(ret));
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
                uint8_t* keyBlob;
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
                // output parameters
                uint8_t* x509Data;
                uint32_t x509DataLen;
                int ret = Keymaster::getKeypairPublic(keyBlob,
                        keyBlobLen, &x509Data, &x509DataLen);
                qemud_client_send(client, (uint8_t*)&x509DataLen,
                        sizeof(x509DataLen));
                qemud_client_send(client, (uint8_t*)x509Data, x509DataLen);
                qemud_client_send(client, (uint8_t*)&ret, sizeof(ret));
                delete [] x509Data;
            }
            break;
        case SignData:
            {
                D("%s: signing data\n", __FUNCTION__);
                // input parameters
                uint32_t paramsLen = unpack<uint32_t>(&data);
                const void* params = (void*)data;
                data += paramsLen;
                uint32_t keyBlobLen = unpack<uint32_t>(&data);
                const uint8_t* keyBlob = (uint8_t*)data;
                data += keyBlobLen;
                uint32_t rawDataLen = unpack<uint32_t>(&data);
                const uint8_t* rawData = (uint8_t*)data;
                data += rawDataLen;
                // output parameters
                uint8_t* signedData;
                uint32_t signedDataLen;
                int ret = Keymaster::signData(params,
                        keyBlob, keyBlobLen,
                        rawData, rawDataLen,
                        &signedData, &signedDataLen);
                qemud_client_send(client, (uint8_t*)&signedDataLen,
                        sizeof(signedDataLen));
                qemud_client_send(client, (uint8_t*)signedData, signedDataLen);
                qemud_client_send(client, (uint8_t*)&ret, sizeof(ret));
                delete [] signedData;
            }
            break;
        case VerifyData:
            {
                D("%s: verifying data\n", __FUNCTION__);
                // input parameters
                uint32_t paramsLen = unpack<uint32_t>(&data);
                const void* params = (void*)data;
                data += paramsLen;
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
                int ret = Keymaster::verifyData(params,
                        keyBlob, keyBlobLen,
                        signedData, signedDataLen,
                        signature, signatureLen);
                qemud_client_send(client, (uint8_t*)&ret, sizeof(ret));
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