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
#  include <stdio.h>
#  define  D(...)    do { printf("%s:%d: ", __FUNCTION__, __LINE__); printf(__VA_ARGS__); \
    fflush(stdout); } while (0)
#else
#  define  D(...)   ((void)0)
#endif

namespace android {

AndroidMessagePipe::AndroidMessagePipe(void* hwPipe, Service* service, DecodeAndExecuteFunction cb,
        base::Stream* stream) : AndroidPipe(hwPipe, service), m_cb(cb) {
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
        case GuestWaitingState::WaitingForGuestSendMesgLength:
        case GuestWaitingState::WaitingForGuestSendMesgPayload:
            return PIPE_POLL_OUT;
        case GuestWaitingState::WaitingForGuestRecvMesgLength:
        case GuestWaitingState::WaitingForGuestRecvMesgPayload:
            return PIPE_POLL_IN;
    }
    return 0;
}

bool AndroidMessagePipe::readyForGuestToReceive() const {
    return (mGuestWaitingState == GuestWaitingState::WaitingForGuestRecvMesgLength ||
            mGuestWaitingState == GuestWaitingState::WaitingForGuestRecvMesgPayload) &&
            (m_expected > 0);
}

int AndroidMessagePipe::onGuestRecv(AndroidPipeBuffer* buffers, int numBuffers) {
    if (!readyForGuestToReceive()) {
        return PIPE_ERROR_AGAIN;
    }
    const bool fromBufdata = false;
    int bytesRecv = 0;
    uint8_t *bufdata = nullptr;
    size_t bufsize = 0;
    while (numBuffers > 0) {
        bufdata = buffers->data;
        bufsize = buffers->size;
        buffers ++;
        numBuffers --;
        while (bufsize > 0) {
            bytesRecv += copyData(&bufdata, &bufsize, fromBufdata /* false */);
            if (m_expected == 0) {
                switchGuestWaitingState();
                if (readyForGuestToSend()) {
                    D("message pipe received %d bytes from host\n", bytesRecv);
                    break;
                }
            }
        }//bufsize > 0
    } // numBuffers > 0
    D("message pipe received %d bytes from host\n", bytesRecv);
    return bytesRecv;
}

bool AndroidMessagePipe::readyForGuestToSend() const {
    return (mGuestWaitingState == GuestWaitingState::WaitingForGuestSendMesgLength ||
            mGuestWaitingState == GuestWaitingState::WaitingForGuestSendMesgPayload) &&
            (m_expected > 0);
}

size_t AndroidMessagePipe::copyData(uint8_t** bufdatap, size_t* bufsizep, bool fromBufdata) {
    uint8_t * & bufdata = *bufdatap;
    size_t  & bufsize = *bufsizep;
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
                        int numBuffers) {
    if (!readyForGuestToSend()) {
        return PIPE_ERROR_AGAIN;
    }
    const bool fromBufdata = true;
    int bytesSent = 0;
    uint8_t *bufdata = nullptr;
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
        buffers ++;
        numBuffers --;
    }
    D("message pipe sent %d bytes to host\n", bytesSent);
    return bytesSent;
}

void AndroidMessagePipe::resetToInitState() {
    mRecvPayloadBuffer.clear();
    mSendPayloadBuffer.clear();
    mSendLengthBuffer = 0;
    m_curPos = reinterpret_cast<uint8_t*>(&mSendLengthBuffer);
    static_assert(kMessageHeaderNumBytes <= 4, "kMessageHeaderNumBytes should be <= 4 bytes");
    m_expected = kMessageHeaderNumBytes;
    mGuestWaitingState = GuestWaitingState::WaitingForGuestSendMesgLength;
}

void AndroidMessagePipe::switchGuestWaitingState() {
    assert(m_expected == 0);
    switch (mGuestWaitingState) {
        case GuestWaitingState::WaitingForGuestSendMesgLength:
            m_expected = mSendLengthBuffer;
            if (m_expected > 0) {
                mSendPayloadBuffer.resize(m_expected);
                m_curPos = &mSendPayloadBuffer[0];
                mGuestWaitingState = GuestWaitingState::WaitingForGuestSendMesgPayload;
            } else {
                // This is abnormal; just ignore it
                D("guest sent a message with payload %d, , reset state.\n", m_expected);
                resetToInitState();
            }
            break;
        case GuestWaitingState::WaitingForGuestSendMesgPayload:
            m_cb(mSendPayloadBuffer, &mRecvPayloadBuffer);
            mSendPayloadBuffer.clear();
            mRecvLengthBuffer = mRecvPayloadBuffer.size();
            static_assert(sizeof(mRecvLengthBuffer) == kMessageHeaderNumBytes,
                    "size of mRecvLengthBuffer should equal to kMessageHeaderNumBytes");
            m_curPos = reinterpret_cast<uint8_t*>(&mRecvLengthBuffer);
            m_expected = kMessageHeaderNumBytes;
            mGuestWaitingState = GuestWaitingState::WaitingForGuestRecvMesgLength;
            break;
        case GuestWaitingState::WaitingForGuestRecvMesgLength:
            m_expected = mRecvPayloadBuffer.size();
            if (m_expected > 0) {
                m_curPos = &mRecvPayloadBuffer[0];
                mGuestWaitingState = GuestWaitingState::WaitingForGuestRecvMesgPayload;
            } else {
                // This is legal; we have no payload to send
                D("no payload to guest, reset state.\n");
                resetToInitState();
            }
            break;
        case GuestWaitingState::WaitingForGuestRecvMesgPayload:
            resetToInitState();
            break;
        default:
            D("bad state %d\n", mGuestWaitingState);
            break;
    }
}

int AndroidMessagePipe::getOffset() const {
    int myoffset = 0;
    switch(mGuestWaitingState) {
        case GuestWaitingState::WaitingForGuestSendMesgLength:
            myoffset = m_curPos - reinterpret_cast<uint8_t*>(const_cast<int*>(&mSendLengthBuffer));
            break;
        case GuestWaitingState::WaitingForGuestSendMesgPayload:
            myoffset = m_curPos - &mSendPayloadBuffer[0];
            break;
        case GuestWaitingState::WaitingForGuestRecvMesgLength:
            myoffset = m_curPos - reinterpret_cast<uint8_t*>(const_cast<int*>(&mRecvLengthBuffer));
            break;
        case GuestWaitingState::WaitingForGuestRecvMesgPayload:
            myoffset = m_curPos - &mRecvPayloadBuffer[0];
            break;
        default:
            break;
    }
    return myoffset;
}

void AndroidMessagePipe::setOffset(int myoffset) {
    switch(mGuestWaitingState) {
        case GuestWaitingState::WaitingForGuestSendMesgLength:
            m_curPos = reinterpret_cast<uint8_t*>(&mSendLengthBuffer) + myoffset;
            break;
        case GuestWaitingState::WaitingForGuestSendMesgPayload:
            m_curPos = &mSendPayloadBuffer[0] + myoffset;
            break;
        case GuestWaitingState::WaitingForGuestRecvMesgLength:
            m_curPos = reinterpret_cast<uint8_t*>(&mRecvLengthBuffer) + myoffset;
            break;
        case GuestWaitingState::WaitingForGuestRecvMesgPayload:
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
    mSendLengthBuffer = stream->getBe32();
    loadBuffer(stream, &mSendPayloadBuffer);
    mRecvLengthBuffer = stream->getBe32();
    loadBuffer(stream, &mRecvPayloadBuffer);
    mGuestWaitingState = (GuestWaitingState)(stream->getByte());
    m_expected = stream->getBe32();
    setOffset(stream->getBe32());
}

void registerAndroidMessagePipeService(const char* serviceName,
        android::AndroidMessagePipe::DecodeAndExecuteFunction cb) {
    android::AndroidPipe::Service::add(new AndroidMessagePipe::Service(serviceName, cb));
}

} // namespace android
