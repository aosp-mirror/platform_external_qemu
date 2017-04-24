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

#include "android/keymaster/KeymasterPipe.h"
#include "android/keymaster/Keymaster.h"

#include <assert.h>

#define DEBUG

#ifdef DEBUG
#  include <stdio.h>
#  define  D(...)   ( fprintf( stderr, __VA_ARGS__ ) )
#else
#  define  D(...)   ((void)0)
#endif

template <class Decoded_t>
static Decoded_t unpack(uint8_t** data) {
    Decoded_t ret;
    memcpy(&ret, *data, sizeof(ret));
    *data = *data + sizeof(Decoded_t);
    return ret;
}

// Read param from src.
// Return a pointer for the param
static void* readGenParam(uint8_t* src,
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

static void releaseGenKeyParam(const keymaster_keypair_t type,
        void** param) {
    switch (type) {
        case TYPE_DSA: {
            keymaster_dsa_keygen_params_t* dsaParams =
                    (keymaster_dsa_keygen_params_t*)*param;
            // dsaParams does not have ownership for its pointer fields
            // do not delete them
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
static void* readSignParam(uint8_t* src,
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

static void releaseSignKeyParam(const keymaster_keypair_t type,
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

namespace android {

KeymasterPipe::KeymasterPipe(void* hwPipe, Service* service) :
                            AndroidPipe(hwPipe, service) {
    D("register new keymaster pipe\n");
}

unsigned KeymasterPipe::onGuestPoll() const {
    switch (mState) {
        case State::WaitingForGuestSend:
        case State::WaitingForGuestSendPartial:
            return PIPE_POLL_OUT;
        case State::WaitingForGuestRecv:
        case State::WaitingForGuestRecvPartial:
            return PIPE_POLL_IN;
    }
    return 0;
}

int KeymasterPipe::onGuestRecv(AndroidPipeBuffer* buffers, int numBuffers) {
    int bytesRecv = 0;
    while (numBuffers > 0 && mGuestRecvBufferHead != mGuestRecvBufferTail) {
        size_t dataSize = mGuestRecvBufferTail - mGuestRecvBufferHead;
        if (dataSize > buffers->size) {
            dataSize = buffers->size;
        }
        memcpy(buffers->data, &mGuestRecvBuffer[mGuestRecvBufferHead], dataSize);
        bytesRecv += dataSize;
        mGuestRecvBufferHead += dataSize;
        buffers ++;
        numBuffers --;
    }
    if (mGuestRecvBufferHead == mGuestRecvBufferTail) {
        mState = State::WaitingForGuestSend;
    } else {
        mState = State::WaitingForGuestRecvPartial;
    }
    return bytesRecv;
}

int KeymasterPipe::onGuestSend(const AndroidPipeBuffer* buffers,
                        int numBuffers) {
    int bytesSent = 0;
    while (numBuffers > 0) {
        const uint8_t* data;
        size_t dataSize;
        D("keymaster receive buffer size %lu\n", buffers->size);
        switch (mState) {
            case State::WaitingForGuestSend:
                assert(buffers->size >= 4);
                dataSize = buffers->size - sizeof(mCommandLength);
                mCommandLength = (size_t)*(uint32_t*)buffers->data;
                D("keymaster new command, length %lu\n", mCommandLength);
                mCommandBufferOffset = 0;
                data = buffers->data + sizeof(mCommandLength);
                mState = State::WaitingForGuestSendPartial;
                if (mCommandLength > mCommandBuffer.size()) {
                    mCommandBuffer.resize(mCommandLength);
                }
                break;
            case State::WaitingForGuestSendPartial:
                dataSize = buffers->size;
                data = buffers->data;
                break;
            default: // this is not expected
                assert(0);
        }
        bytesSent += buffers->size;
        // we are not expecting more than one command at a time
        assert(mCommandBufferOffset + dataSize <= mCommandLength);
        memcpy(&mCommandBuffer[0] + mCommandBufferOffset, data, dataSize);
        mCommandBufferOffset += dataSize;
        buffers++;
        numBuffers--;
        mState = State::WaitingForGuestSendPartial;
    }
    if (mCommandBufferOffset == mCommandLength) {
        // decode and execute the command
        decodeAndExecute();
    }
    return bytesSent;
}

void KeymasterPipe::addToRecvBuffer(const uint8_t* data, uint32_t len) {
    if (len + mGuestRecvBufferTail > mGuestRecvBuffer.size()) {
        mGuestRecvBuffer.resize(len + mGuestRecvBufferTail);
    }
    memcpy(&mGuestRecvBuffer[0] + mGuestRecvBufferTail, data, len);
    mGuestRecvBufferTail += len;
}

void KeymasterPipe::decodeAndExecute() {
    enum {
        GenerateKeypair,
        ImportKeypair,
        GetKeypairPublic,
        SignData,
        VerifyData,
    };
    mGuestRecvBufferHead = 0;
    mGuestRecvBufferTail = 0;
    uint8_t* data = &mCommandBuffer[0];
    int32_t cmd = unpack<int32_t>(&data);
    switch (cmd) {
        case GenerateKeypair:
            {
                D("%s: generating key\n", __FUNCTION__);
                // input parameters
                keymaster_keypair_t keyType =
                        (keymaster_keypair_t)unpack<int32_t>(&data);
                uint32_t keyParamsLen = unpack<uint32_t>(&data);
                void* keyParams = readGenParam(data, keyType);
                data += keyParamsLen;
                // output parameters
                std::vector<uint8_t> keyBlob;
                int ret = Keymaster::generateKeypair(keyType,
                        keyParams, keyBlob);
                uint32_t keyBlobLength = keyBlob.size();
                addToRecvBuffer((uint8_t*)&keyBlobLength,
                        sizeof(keyBlobLength));
                if (keyBlobLength) {
                    addToRecvBuffer(keyBlob.data(), keyBlobLength);
                }
                addToRecvBuffer((uint8_t*)&ret, sizeof(ret));
                releaseGenKeyParam(keyType, &keyParams);
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
                std::vector<uint8_t> keyBlob;
                int ret = Keymaster::importKeypair(key, keyLen,
                        keyBlob);
                uint32_t keyBlobLength = keyBlob.size();
                addToRecvBuffer((uint8_t*)&keyBlobLength,
                        sizeof(keyBlobLength));
                addToRecvBuffer(keyBlob.data(), keyBlobLength);
                addToRecvBuffer((uint8_t*)&ret, sizeof(ret));
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
                uint8_t* x509Data = nullptr;
                uint32_t x509DataLen;
                int32_t ret = Keymaster::getKeypairPublic(keyBlob,
                        keyBlobLen, &x509Data, &x509DataLen);
                addToRecvBuffer((uint8_t*)&x509DataLen,
                        sizeof(x509DataLen));
                if (x509DataLen) {
                    addToRecvBuffer((uint8_t*)x509Data, x509DataLen);
                }
                addToRecvBuffer((uint8_t*)&ret, sizeof(ret));
                delete [] x509Data;
            }
            break;
        case SignData:
            {
                D("%s: %d signing data\n", __FUNCTION__, __LINE__);
                // input parameters
                keymaster_keypair_t keyType =
                        (keymaster_keypair_t)unpack<int32_t>(&data);
                uint32_t keyParamsLen = unpack<uint32_t>(&data);
                void* keyParams = readSignParam(data, keyType);
                data += keyParamsLen;
                uint32_t keyBlobLen = unpack<uint32_t>(&data);
                const uint8_t* keyBlob = (uint8_t*)data;
                data += keyBlobLen;
                uint32_t rawDataLen = unpack<uint32_t>(&data);
                const uint8_t* rawData = (uint8_t*)data;
                data += rawDataLen;
                // output parameters
                std::vector<uint8_t> signedData;
                int ret = Keymaster::signData(keyParams,
                        keyBlob, keyBlobLen,
                        rawData, rawDataLen,
                        signedData);
                uint32_t signedDataLen = signedData.size();
                addToRecvBuffer((uint8_t*)&signedDataLen,
                        sizeof(signedDataLen));
                if (signedDataLen) {
                    addToRecvBuffer(signedData.data(), signedDataLen);
                }
                addToRecvBuffer((uint8_t*)&ret, sizeof(ret));
                releaseSignKeyParam(keyType, &keyParams);
            }
            break;
        case VerifyData:
            {
                D("%s: verifying data\n", __FUNCTION__);
                // input parameters
                keymaster_keypair_t keyType =
                        (keymaster_keypair_t)unpack<int32_t>(&data);
                uint32_t keyParamsLen = unpack<uint32_t>(&data);
                void* keyParams = readSignParam(data, keyType);
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
                addToRecvBuffer((uint8_t*)&ret, sizeof(ret));
                releaseSignKeyParam(keyType, &keyParams);
            }
            break;
        default:
            fprintf(stderr, "Warning: keymaster received bad command %d,"
                    "Shutting down keymaster pipe\n",
                    cmd);
            closeFromHost();
            return;
    }
    // Tell the guest we are ready to send the return value
    mState = State::WaitingForGuestRecv;
}

void registerKeymasterPipeService() {
    android::AndroidPipe::Service::add(new KeymasterPipe::Service());
}
}

extern "C" void android_init_keymaster_pipe(void) {
    android::registerKeymasterPipeService();
}