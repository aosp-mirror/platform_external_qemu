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
#include "Keymaster.h"

#include <assert.h>

namespace android {

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
        switch (mState) {
            case State::WaitingForGuestSend:
                assert(buffers->size >= 4);
                dataSize = buffers->size - sizeof(mCommandLength);
                mCommandLength = (size_t)*(uint32_t*)buffers->data;
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
    int cmd = *(int*)data;
    data += sizeof(cmd);
    switch (cmd) {
        case GenerateKeypair:
            {
                // input parameters
                Keymaster::keymaster_keypair_t keyType =
                        unpack<Keymaster::keymaster_keypair_t>(&data);
                uint32_t keyParamsLen = unpack<uint32_t>(&data);
                const void* keyParams = (void*)data;
                data += keyParamsLen;
                // output parameters
                uint8_t* keyBlob;
                uint32_t keyBlobLength;
                int ret = m_keymaster.generateKeypair(keyType,
                        keyParams, &keyBlob, &keyBlobLength);
                addToRecvBuffer((uint8_t*)&keyBlobLength, sizeof(keyBlobLength));
                addToRecvBuffer((uint8_t*)keyBlob, keyBlobLength);
                addToRecvBuffer((uint8_t*)&ret, sizeof(ret));
                delete [] keyBlob;
            }
            break;
        case ImportKeypair:
            {
                // input parameters
                uint32_t keyLen = unpack<uint32_t>(&data);
                const uint8_t* key = (uint8_t*)data;
                data += keyLen;
                // output parameters
                uint8_t* keyBlob;
                uint32_t keyBlobLength;
                int ret = m_keymaster.importKeypair(key, keyLen,
                        &keyBlob, &keyBlobLength);
                addToRecvBuffer((uint8_t*)&keyBlobLength, sizeof(keyBlobLength));
                addToRecvBuffer((uint8_t*)keyBlob, keyBlobLength);
                addToRecvBuffer((uint8_t*)&ret, sizeof(ret));
                delete [] keyBlob;
            }
            break;
        case GetKeypairPublic:
            {
                // input parameters
                uint32_t keyBlobLen = unpack<uint32_t>(&data);
                const uint8_t* keyBlob = (uint8_t*)data;
                data += keyBlobLen;
                // output parameters
                uint8_t* x509Data;
                uint32_t x509DataLen;
                int ret = m_keymaster.getKeypairPublic(keyBlob,
                        keyBlobLen, &x509Data, &x509DataLen);
                addToRecvBuffer((uint8_t*)&x509DataLen, sizeof(x509DataLen));
                addToRecvBuffer((uint8_t*)x509Data, x509DataLen);
                addToRecvBuffer((uint8_t*)&ret, sizeof(ret));
                delete [] x509Data;
            }
            break;
        case SignData:
            {
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
                int ret = m_keymaster.signData(params,
                        keyBlob, keyBlobLen,
                        rawData, rawDataLen,
                        &signedData, &signedDataLen);
                addToRecvBuffer((uint8_t*)&signedDataLen, sizeof(signedDataLen));
                addToRecvBuffer((uint8_t*)signedData, signedDataLen);
                addToRecvBuffer((uint8_t*)&ret, sizeof(ret));
                delete [] signedData;
            }
            break;
        case VerifyData:
            {
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
                int ret = m_keymaster.verifyData(params,
                        keyBlob, keyBlobLen,
                        signedData, signedDataLen,
                        signature, signatureLen);
                addToRecvBuffer((uint8_t*)&ret, sizeof(ret));
            }
            break;
        default:
            // No this does not happen.
            assert(0);
    }
    // Tell the guest we are ready to send the return value
    mState = State::WaitingForGuestRecv;
}

void registerKeymasterPipeService() {
    android::AndroidPipe::Service::add(new KeymasterPipe::Service());
}
}