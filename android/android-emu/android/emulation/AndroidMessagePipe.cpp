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
#  define  D(...)    do { printf("%s:%d: ", __FUNCTION__, __LINE__); printf(__VA_ARGS__); fflush(stdout); } while (0)
#else
#  define  D(...)   ((void)0)
#endif

namespace android {

AndroidMessagePipe::AndroidMessagePipe(void* hwPipe, Service* service, DecodeAndExecute cb) :
                            AndroidPipe(hwPipe, service), mState(State::WaitingForGuestSendMesgLength),
                            m_expected(MESSAGE_LENGTH_BUFFER_SIZE), m_cb(cb) {
    mSendLengthBuffer.resize(MESSAGE_LENGTH_BUFFER_SIZE);
    mRecvLengthBuffer.resize(MESSAGE_LENGTH_BUFFER_SIZE);
    m_curPos = &(mSendLengthBuffer[0]);
    D("register new message pipe %s\n", service->name());
}

unsigned AndroidMessagePipe::onGuestPoll() const {
    switch (mState) {
        case State::WaitingForGuestSendMesgLength:
        case State::WaitingForGuestSendMesgPayload:
            return PIPE_POLL_OUT;
        case State::WaitingForGuestRecvMesgLenth:
        case State::WaitingForGuestRecvMesgPayload:
            return PIPE_POLL_IN;
    }
    return 0;
}

bool AndroidMessagePipe::readyForGuestToReceive() {
    return (mState == State::WaitingForGuestRecvMesgLenth ||
            mState == State::WaitingForGuestRecvMesgPayload) &&
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
            bytesRecv += copyData(bufdata, bufsize, fromBufdata /* false */);
            if (m_expected == 0) {
                switchState();
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

//
bool AndroidMessagePipe::readyForGuestToSend() {
    return (mState == State::WaitingForGuestSendMesgLength ||
            mState == State::WaitingForGuestSendMesgPayload) &&
            (m_expected > 0);
}

size_t AndroidMessagePipe::copyData(uint8_t* & bufdata, size_t& bufsize, bool fromBufdata) {
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
            bytesSent += copyData(bufdata, bufsize, fromBufdata);
            if (m_expected == 0) {
                switchState();
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

void AndroidMessagePipe::switchState() {
    assert(m_expected == 0);
    switch (mState) {
        case State::WaitingForGuestSendMesgLength:
            m_expected = getSendPayloadSize();
            mSendPayloadBuffer.resize(m_expected);
            m_curPos = &mSendPayloadBuffer[0];
            mState = State::WaitingForGuestSendMesgPayload;
            break;
        case State::WaitingForGuestSendMesgPayload:
            {
                m_cb(mSendPayloadBuffer, &mRecvPayloadBuffer);
                uint64_t length = mRecvPayloadBuffer.size();
                m_curPos = &mRecvLengthBuffer[0];
                m_expected = MESSAGE_LENGTH_BUFFER_SIZE;
                memcpy(m_curPos, &length, m_expected);
                mState = State::WaitingForGuestRecvMesgLenth;
            }
            break;
        case State::WaitingForGuestRecvMesgLenth:
            m_curPos = &mRecvPayloadBuffer[0];
            m_expected = mRecvPayloadBuffer.size();
            mState = State::WaitingForGuestRecvMesgPayload;
            break;
        case State::WaitingForGuestRecvMesgPayload:
            m_expected = MESSAGE_LENGTH_BUFFER_SIZE;
            m_curPos = &mSendLengthBuffer[0];
            mState = State::WaitingForGuestSendMesgLength;
            break;
        default:
            break;
    }
}

void registerAndroidMessagePipeService(const char* serviceName, android::AndroidMessagePipe::DecodeAndExecute cb) {
    android::AndroidPipe::Service::add(new AndroidMessagePipe::Service(serviceName, cb));
}

} // namespace android
