// Copyright 2019 The Android Open Source Project
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

#include <functional>
#include <memory>

#include "android/base/files/Stream.h"
#include "android/base/threads/Thread.h"
#include "android/emulation/AdbProxy.h"
#include "android/emulation/android_pipe_common.h"
#include "android/emulation/apacket_utils.h"

namespace android {
namespace jdwp {
class JdwpProxy : public emulation::AdbProxy {
public:
    JdwpProxy(int hostId, int guestId, int guestPid);
    JdwpProxy(android::base::Stream* stream);
    void onGuestRecvData(const emulation::amessage* mesg,
                         const uint8_t* data,
                         bool* shouldForwardRecv,
                         std::queue<emulation::apacket>* toSends) override;
    void onGuestSendData(const emulation::amessage* mesg,
                         const uint8_t* data) override;
    bool shouldClose() const override;
    int32_t guestId() const override;
    int32_t originHostId() const override;
    int32_t currentHostId() const override;
    enum State {
        Uninitialized,
        Connect,
        ConnectOk,
        HandshakeSent,
        HandshakeSentOk,
        HandshakeReplied,
        HandshakeRepliedOk,
        Proxying = HandshakeRepliedOk,
    };
    enum IsJdwp {
        Unknown,
        Yes,
        No,
    };
    static State nextState(State state);
    State mProxyState = State::Uninitialized;
    State mClientState = State::Uninitialized;
    int mHostId = 0;
    int mCurrentHostId = 0;
    int mGuestId = 0;
    int mGuestPid = 0;
    void onSave(android::base::Stream* stream);
    void onGuestSendData(const AndroidPipeBuffer* buffers,
                         int numBuffers,
                         size_t actualSentBytes);
    void onGuestRecvData(const AndroidPipeBuffer* buffers,
                         int numBuffers,
                         size_t actualRecvBytes);

private:
    emulation::amessage buildReplyMesg();
    bool mShouldClose = false;
};
}  // namespace jdwp
}  // namespace android