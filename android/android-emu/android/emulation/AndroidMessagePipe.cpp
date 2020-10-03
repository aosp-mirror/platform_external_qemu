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

#include "android/emulation/AndroidMessagePipe.h"
#include "android/base/files/StreamSerializing.h"
#include "android/utils/debug.h"

#include <assert.h>

// #define DEBUG 1

#ifdef DEBUG
#include <stdio.h>
#define D(...)                                     \
    do {                                           \
        printf("%s:%d: ", __FUNCTION__, __LINE__); \
        printf(__VA_ARGS__);                       \
        fflush(stdout);                            \
    } while (0)
#else
#define D(...) ((void)0)
#endif

namespace android {

AndroidMessagePipe::AndroidMessagePipe(void* hwPipe,
                                       android::AndroidPipe::Service* service,
                                       DecodeAndExecuteFunction cb,
                                       base::Stream* stream)
    : AndroidPipe(hwPipe, service), m_cb(cb) {
    D("register new message pipe %s\n", service->name().c_str());
    if (stream) {
        D("load AndroidMessagePipe state from snapshot\n");
        loadFromStream(stream);
    } else {
        D("create new AndroidMessagePipe state\n");
        resetToInitState();
    }
}

unsigned AndroidMessagePipe::onGuestPoll() const {
    switch (mGuestWaitingState) {
        case GuestWaitingState::SendMesgLength:
        case GuestWaitingState::SendMesgPayload:
            return PIPE_POLL_OUT;
        case GuestWaitingState::RecvMesgLength:
        case GuestWaitingState::RecvMesgPayload:
            return PIPE_POLL_IN;
    }
    return 0;
}

bool AndroidMessagePipe::readyForGuestToReceive() const {
    return (mGuestWaitingState == GuestWaitingState::RecvMesgLength ||
            mGuestWaitingState == GuestWaitingState::RecvMesgPayload) &&
           (m_expected > 0);
}

int AndroidMessagePipe::onGuestRecv(AndroidPipeBuffer* buffers,
                                    int numBuffers) {
    if (!readyForGuestToReceive()) {
        return PIPE_ERROR_AGAIN;
    }
    const bool fromBufdata = false;
    int bytesRecv = 0;
    uint8_t* bufdata = nullptr;
    size_t bufsize = 0;
    while (numBuffers > 0) {
        bufdata = buffers->data;
        bufsize = buffers->size;
        buffers++;
        numBuffers--;
        while (bufsize > 0) {
            bytesRecv += copyData(&bufdata, &bufsize, fromBufdata /* false */);
            if (m_expected == 0) {
                switchGuestWaitingState();
                if (readyForGuestToSend()) {
                    D("message pipe received %d bytes from host\n", bytesRecv);
                    break;
                }
            }
        }  // bufsize > 0
    }      // numBuffers > 0
    D("message pipe received %d bytes from host\n", bytesRecv);
    return bytesRecv;
}

bool AndroidMessagePipe::readyForGuestToSend() const {
    return (mGuestWaitingState == GuestWaitingState::SendMesgLength ||
            mGuestWaitingState == GuestWaitingState::SendMesgPayload) &&
           (m_expected > 0);
}

size_t AndroidMessagePipe::copyData(uint8_t** bufdatap,
                                    size_t* bufsizep,
                                    bool fromBufdata) {
    uint8_t*& bufdata = *bufdatap;
    size_t& bufsize = *bufsizep;
    assert(m_expected > 0);
    size_t toCopy = std::min(bufsize, (size_t)m_expected);
    if (fromBufdata) {
        memcpy(m_curPos, bufdata, toCopy);
    } else {
        memcpy(bufdata, m_curPos, toCopy);
    }
    m_expected -= toCopy;
    bufsize -= toCopy;
    m_curPos += toCopy;
    bufdata += toCopy;
    return toCopy;
}

int AndroidMessagePipe::onGuestSend(const AndroidPipeBuffer* buffers,
                                    int numBuffers,
                                    void** newPipePtr) {
    if (!readyForGuestToSend()) {
        return PIPE_ERROR_AGAIN;
    }
    const bool fromBufdata = true;
    int bytesSent = 0;
    uint8_t* bufdata = nullptr;
    size_t bufsize = 0;
    while (numBuffers > 0) {
        bufdata = buffers->data;
        bufsize = buffers->size;
        while (bufsize > 0) {
            bytesSent += copyData(&bufdata, &bufsize, fromBufdata /* true */);
            if (m_expected == 0) {
                switchGuestWaitingState();
                if (readyForGuestToReceive()) {
                    D("message pipe sent %d bytes to host\n", bytesSent);
                    return bytesSent;
                }
            }
        }
        buffers++;
        numBuffers--;
    }
    D("message pipe sent %d bytes to host\n", bytesSent);
    return bytesSent;
}

void AndroidMessagePipe::resetToInitState() {
    mRecvPayloadBuffer.clear();
    mSendPayloadBuffer.clear();
    mSendLengthBuffer = 0;
    m_curPos = reinterpret_cast<uint8_t*>(&mSendLengthBuffer);
    static_assert(kMessageHeaderNumBytes <= 4,
                  "kMessageHeaderNumBytes should be <= 4 bytes");
    m_expected = kMessageHeaderNumBytes;
    mGuestWaitingState = GuestWaitingState::SendMesgLength;
}

void AndroidMessagePipe::resetRecvPayload(DataBuffer&& buffer) {
    mRecvPayloadBuffer = std::move(buffer);
    m_expected = 0;
    switchGuestWaitingState();
}

void AndroidMessagePipe::switchGuestWaitingState() {
    assert(m_expected == 0);
    switch (mGuestWaitingState) {
        case GuestWaitingState::SendMesgLength:
            m_expected = mSendLengthBuffer;
            if (m_expected > 0) {
                mSendPayloadBuffer.resize(m_expected);
                m_curPos = &mSendPayloadBuffer[0];
                mGuestWaitingState = GuestWaitingState::SendMesgPayload;
            } else {
                // This is abnormal; just ignore it
                D("guest sent a message with payload %d, , reset state.\n",
                  m_expected);
                resetToInitState();
            }
            break;
        case GuestWaitingState::SendMesgPayload:
            m_cb(mSendPayloadBuffer, &mRecvPayloadBuffer);
            mSendPayloadBuffer.clear();
            mRecvLengthBuffer = mRecvPayloadBuffer.size();
            static_assert(sizeof(mRecvLengthBuffer) == kMessageHeaderNumBytes,
                          "size of mRecvLengthBuffer should equal to "
                          "kMessageHeaderNumBytes");
            m_curPos = reinterpret_cast<uint8_t*>(&mRecvLengthBuffer);
            m_expected = kMessageHeaderNumBytes;
            mGuestWaitingState = GuestWaitingState::RecvMesgLength;
            break;
        case GuestWaitingState::RecvMesgLength:
            m_expected = mRecvPayloadBuffer.size();
            if (m_expected > 0) {
                m_curPos = &mRecvPayloadBuffer[0];
                mGuestWaitingState = GuestWaitingState::RecvMesgPayload;
            } else {
                // This is legal; we have no payload to send
                D("no payload to guest, reset state.\n");
                resetToInitState();
            }
            break;
        case GuestWaitingState::RecvMesgPayload:
            resetToInitState();
            break;
        default:
            D("bad state %d\n", mGuestWaitingState);
            break;
    }
}

uint32_t AndroidMessagePipe::getOffset() const {
    uintptr_t myoffset = 0;
    switch (mGuestWaitingState) {
        case GuestWaitingState::SendMesgLength:
            myoffset = (uintptr_t)m_curPos -
                       (uintptr_t)&mSendLengthBuffer;
            break;
        case GuestWaitingState::SendMesgPayload:
            myoffset = (uintptr_t)m_curPos - (uintptr_t)&mSendPayloadBuffer[0];
            break;
        case GuestWaitingState::RecvMesgLength:
            myoffset = (uintptr_t)m_curPos - (uintptr_t)&mRecvLengthBuffer;
            break;
        case GuestWaitingState::RecvMesgPayload:
            myoffset = (uintptr_t)m_curPos - (uintptr_t)&mRecvPayloadBuffer[0];
            break;
        default:
            break;
    }
    return (uint32_t)myoffset;
}

void AndroidMessagePipe::setOffset(uint32_t myoffset) {
    switch (mGuestWaitingState) {
        case GuestWaitingState::SendMesgLength:
            m_curPos =
                    reinterpret_cast<uint8_t*>(&mSendLengthBuffer) + myoffset;
            break;
        case GuestWaitingState::SendMesgPayload:
            m_curPos = &mSendPayloadBuffer[0] + myoffset;
            break;
        case GuestWaitingState::RecvMesgLength:
            m_curPos =
                    reinterpret_cast<uint8_t*>(&mRecvLengthBuffer) + myoffset;
            break;
        case GuestWaitingState::RecvMesgPayload:
            m_curPos = &mRecvPayloadBuffer[0] + myoffset;
            break;
        default:
            break;
    }
}

void AndroidMessagePipe::onSave(base::Stream* stream) {
    stream->putBe32(mSendLengthBuffer);
    saveBuffer(stream, mSendPayloadBuffer);
    stream->putBe32(mRecvLengthBuffer);
    saveBuffer(stream, mRecvPayloadBuffer);
    stream->putByte((uint8_t)mGuestWaitingState);
    stream->putBe32(m_expected);
    stream->putBe32(getOffset());
}

void AndroidMessagePipe::loadFromStream(base::Stream* stream) {
    resetToInitState();

    mSendLengthBuffer = stream->getBe32();
    mSendPayloadBuffer.resize(mSendLengthBuffer);
    loadBuffer(stream, &mSendPayloadBuffer);

    mRecvLengthBuffer = stream->getBe32();
    mRecvPayloadBuffer.resize(mRecvLengthBuffer);
    loadBuffer(stream, &mRecvPayloadBuffer);

    mGuestWaitingState = (GuestWaitingState)(stream->getByte());
    m_expected = stream->getBe32();
    setOffset(stream->getBe32());
}

void registerAndroidMessagePipeService(
        const char* serviceName,
        android::AndroidMessagePipe::DecodeAndExecuteFunction cb) {
    AndroidPipe::Service::add(
        std::make_unique<AndroidMessagePipe::Service>(serviceName, cb));
}

}  // namespace android
