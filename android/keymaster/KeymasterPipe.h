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

#include "android/emulation/AndroidPipe.h"

#include "android/keymaster/Keymaster.h"

#include <memory>
#include <vector>

namespace android {

class KeymasterPipe : public AndroidPipe {
public:
    //////////////////////////////////////////////////////////////////////////
    // The pipe service class for this implementation.
    class Service : public AndroidPipe::Service {
    public:
        Service() : AndroidPipe::Service("KeymasterPipe") {}

        virtual AndroidPipe* create(void* mHwPipe, const char* args) override {
            return new KeymasterPipe(mHwPipe, this);
        }
    };

    KeymasterPipe(void* hwPipe, Service* service) :
                            AndroidPipe(hwPipe, service) { }

    void onGuestClose() override {}
    unsigned onGuestPoll() const override;
    int onGuestRecv(AndroidPipeBuffer* buffers, int numBuffers) override;
    int onGuestSend(const AndroidPipeBuffer* buffers,
                            int numBuffers) override;
    void onGuestWantWakeOn(int flags) override {}

private:
    enum class State {
        WaitingForGuestSend,
        WaitingForGuestSendPartial, // the previous command is incomplete
        WaitingForGuestRecv,
        WaitingForGuestRecvPartial, // the previous command is incomplete
    };
    State mState = State::WaitingForGuestRecv;

    size_t mCommandLength = 0;
    std::vector<uint8_t> mCommandBuffer;
    size_t mCommandBufferOffset = 0;

    std::vector<uint8_t> mGuestRecvBuffer;
    size_t mGuestRecvBufferHead = 0;
    size_t mGuestRecvBufferTail = 0;
    void addToRecvBuffer(const uint8_t* data, uint32_t len);

    void decodeAndExecute();
    Keymaster m_keymaster;
    template <class _T>
    static _T unpack(uint8_t** data) {
        _T ret = *(_T*)(*data);
        *data = *data + sizeof(_T);
        return ret;
    }
};

// registerKeymasterPipeService() registers a "KeymasterPipe" pipe service that
// is used to simulate hardware keystore.
void registerKeymasterPipeService();

}  // namespace android

