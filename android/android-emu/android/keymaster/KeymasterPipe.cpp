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
#include "android/keymaster/keymaster_ipc.h"
#include "android/utils/debug.h"

#include <assert.h>

#ifdef DEBUG
#  include <stdio.h>
#  define  D(...)    do { printf("%s:%d: ", __FUNCTION__, __LINE__); printf(__VA_ARGS__); fflush(stdout); } while (0)
#else
#  define  D(...)   ((void)0)
#endif

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
    while (numBuffers > 0 && mGuestRecvBufferHead != mGuestRecvBuffer.size()) {
        size_t dataSize = mGuestRecvBuffer.size() - mGuestRecvBufferHead;
        if (dataSize > buffers->size) {
            dataSize = buffers->size;
        }
        memcpy(buffers->data, &mGuestRecvBuffer[mGuestRecvBufferHead],
                dataSize);
        bytesRecv += dataSize;
        mGuestRecvBufferHead += dataSize;
        buffers ++;
        numBuffers --;
    }
    if (mGuestRecvBufferHead == mGuestRecvBuffer.size()) {
        mGuestRecvBuffer.clear();
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
        const uint8_t* data = nullptr;
        size_t dataSize = 0;
        D("keymaster receive buffer size %lu\n", buffers->size);
        switch (mState) {
            case State::WaitingForGuestSend: {
                if (buffers->size < 8) {
                    derror("ERROR: KeymasterPipe: received incomplete command,"
                        " expect size 8, received size %lu\n",
                        buffers->size);
                    return PIPE_ERROR_INVAL;
                }
                uint64_t commandLength;
                memcpy(&commandLength, buffers->data, sizeof(commandLength));
                dataSize = buffers->size - sizeof(commandLength);
                D("keymaster new command, length %lu\n", commandLength);
                mCommandBufferOffset = 0;
                data = buffers->data + sizeof(uint64_t);
                mState = State::WaitingForGuestSendPartial;
                if (commandLength > mCommandBuffer.size()) {
                    mCommandBuffer.resize(commandLength);
                }
                break;
            }
            case State::WaitingForGuestSendPartial:
                dataSize = buffers->size;
                data = buffers->data;
                break;
            default: // this is not expected
                derror("ERROR: KeymasterPipe: received command"
                    "while pending for send\n");
                return PIPE_ERROR_INVAL;
        }
        bytesSent += buffers->size;
        // we are not expecting more than one command at a time
        assert(mCommandBufferOffset + dataSize <= mCommandBuffer.size());
        memcpy(&mCommandBuffer[0] + mCommandBufferOffset, data, dataSize);
        mCommandBufferOffset += dataSize;
        buffers++;
        numBuffers--;
        mState = State::WaitingForGuestSendPartial;
    }
    if (mCommandBufferOffset == mCommandBuffer.size()) {
        // decode and execute the command
        decodeAndExecute();
        mCommandBuffer.clear();
    }
    return bytesSent;
}

void KeymasterPipe::addToRecvBuffer(const uint8_t* data, uint32_t len) {
    mGuestRecvBuffer.insert(mGuestRecvBuffer.end(), data, data + len);
}

void KeymasterPipe::decodeAndExecute() {
    mGuestRecvBufferHead = 0;
    // Tell the guest we are ready to send the return value
    keymaster_ipc_call(mCommandBuffer, &mGuestRecvBuffer);
    mState = State::WaitingForGuestRecv;
}

void registerKeymasterPipeService() {
    android::AndroidPipe::Service::add(new KeymasterPipe::Service());
}
} // namespace android

extern "C" void android_init_keymaster_pipe(void) {
    android::registerKeymasterPipeService();
}

extern "C" void android_init_keymaster(void) {
    keymaster_ipc_init();
    android_init_keymaster_pipe();
}
