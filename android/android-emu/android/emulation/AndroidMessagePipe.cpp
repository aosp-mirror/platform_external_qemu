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
#include "android/utils/debug.h"

#include <assert.h>

#ifdef DEBUG
#  include <stdio.h>
#  define  D(...)    do { printf("%s:%d: ", __FUNCTION__, __LINE__); printf(__VA_ARGS__); \
    fflush(stdout); } while (0)
#else
#  define  D(...)   ((void)0)
#endif

namespace android {

AndroidMessagePipe::AndroidMessagePipe(void* hwPipe, Service* service, DecodeAndExecuteFunction cb,
        base::Stream* stream) : AndroidPipe(hwPipe, service),
    mGuestWaitingState(GuestWaitingState::WaitingForGuestSendMesgLength),
    m_expected(kMessageHeaderNumBytes), m_cb(cb) {
    mSendLengthBuffer.resize(kMessageHeaderNumBytes);
    m_curPos = &(mSendLengthBuffer[0]);
    if (stream) {
        loadFromStream(stream);
    }
    D("register new message pipe %s\n", service->name());
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
    size_t toCopy = bufsize <= m_expected ? bufsize : m_expected;
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

int AndroidMessagePipe::getSendPayloadSize() {
    uint64_t size;
    memcpy(&size, &(mSendLengthBuffer[0]), 8);
    return size;
}

void AndroidMessagePipe::switchGuestWaitingState() {
    assert(m_expected == 0);
    switch (mGuestWaitingState) {
        case GuestWaitingState::WaitingForGuestSendMesgLength:
            m_expected = getSendPayloadSize();
            mSendPayloadBuffer.resize(m_expected);
            m_curPos = &mSendPayloadBuffer[0];
            mSendLengthBuffer.clear();
            mGuestWaitingState = GuestWaitingState::WaitingForGuestSendMesgPayload;
            break;
        case GuestWaitingState::WaitingForGuestSendMesgPayload:
            {
                m_cb(mSendPayloadBuffer, &mRecvPayloadBuffer);
                uint64_t length = mRecvPayloadBuffer.size();
                assert(sizeof(length) == kMessageHeaderNumBytes);
                mRecvLengthBuffer.resize(kMessageHeaderNumBytes);
                m_curPos = &mRecvLengthBuffer[0];
                m_expected = kMessageHeaderNumBytes;
                memcpy(m_curPos, &length, m_expected);
                mSendPayloadBuffer.clear();
                mGuestWaitingState = GuestWaitingState::WaitingForGuestRecvMesgLength;
            }
            break;
        case GuestWaitingState::WaitingForGuestRecvMesgLength:
            m_curPos = &mRecvPayloadBuffer[0];
            m_expected = mRecvPayloadBuffer.size();
            mRecvLengthBuffer.clear();
            mGuestWaitingState = GuestWaitingState::WaitingForGuestRecvMesgPayload;
            break;
        case GuestWaitingState::WaitingForGuestRecvMesgPayload:
            m_expected = kMessageHeaderNumBytes;
            m_curPos = &mSendLengthBuffer[0];
            mRecvPayloadBuffer.clear();
            mGuestWaitingState = GuestWaitingState::WaitingForGuestSendMesgLength;
            break;
        default:
            break;
    }
}

uint64_t AndroidMessagePipe::getOffset() const {
    uint64_t myoffset = 0;
    switch(mGuestWaitingState) {
        case GuestWaitingState::WaitingForGuestSendMesgLength:
            myoffset = m_curPos - &mSendLengthBuffer[0];
            break;
        case GuestWaitingState::WaitingForGuestSendMesgPayload:
            myoffset = m_curPos - &mSendPayloadBuffer[0];
            break;
        case GuestWaitingState::WaitingForGuestRecvMesgLength:
            myoffset = m_curPos - &mRecvLengthBuffer[0];
            break;
        case GuestWaitingState::WaitingForGuestRecvMesgPayload:
            myoffset = m_curPos - &mRecvPayloadBuffer[0];
            break;
        default:
            break;
    }
    return myoffset;
}

void AndroidMessagePipe::setOffset(uint64_t myoffset) {
    switch(mGuestWaitingState) {
        case GuestWaitingState::WaitingForGuestSendMesgLength:
            m_curPos = &mSendLengthBuffer[0] + myoffset;
            break;
        case GuestWaitingState::WaitingForGuestSendMesgPayload:
            m_curPos = &mSendPayloadBuffer[0] + myoffset;
            break;
        case GuestWaitingState::WaitingForGuestRecvMesgLength:
            m_curPos = &mRecvLengthBuffer[0] + myoffset;
            break;
        case GuestWaitingState::WaitingForGuestRecvMesgPayload:
            m_curPos = &mRecvPayloadBuffer[0] + myoffset;
            break;
        default:
            break;
    }
}

void AndroidMessagePipe::onSave(base::Stream* stream) {
    saveVectorToStream(mSendLengthBuffer, stream);
    saveVectorToStream(mSendPayloadBuffer, stream);
    saveVectorToStream(mRecvLengthBuffer, stream);
    saveVectorToStream(mRecvPayloadBuffer, stream);
    stream->putBe64((uint64_t)mGuestWaitingState);
    stream->putBe64(m_expected);
    stream->putBe64(getOffset());
}

void AndroidMessagePipe::loadFromStream(base::Stream* stream) {
    loadVectorFromStream(stream, &mSendLengthBuffer);
    loadVectorFromStream(stream, &mSendPayloadBuffer);
    loadVectorFromStream(stream, &mRecvLengthBuffer);
    loadVectorFromStream(stream, &mRecvPayloadBuffer);
    mGuestWaitingState = (GuestWaitingState)(stream->getBe64());
    m_expected = stream->getBe64();
    setOffset(stream->getBe64());
}

void registerAndroidMessagePipeService(const char* serviceName,
        android::AndroidMessagePipe::DecodeAndExecuteFunction cb) {
    android::AndroidPipe::Service::add(new AndroidMessagePipe::Service(serviceName, cb));
}

} // namespace android
