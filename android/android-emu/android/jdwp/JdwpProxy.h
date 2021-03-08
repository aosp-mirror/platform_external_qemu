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
#include <unordered_set>

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
                         const uint8_t* data,
                         bool* shouldForwardSend,
                         std::queue<emulation::apacket>* extraSends) override;
    bool shouldClose() const override;
    int32_t guestId() const override;
    int32_t originHostId() const override;
    int32_t currentHostId() const override;
    void setCurrentHostId(int32_t currentHostId);
    int32_t guestPid() const;
    void onSave(android::base::Stream* stream);
private:
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
    static State nextState(State state);
    emulation::amessage buildReplyMesg();
    State mProxyState = State::Uninitialized;
    State mClientState = State::Uninitialized;
    int mHostId = 0;
    int mCurrentHostId = 0;
    int mGuestId = 0;
    int mGuestPid = 0;

    bool mShouldClose = false;
    bool mShouldSendCachePacket = false;
    bool mDebuggerActivated = false;
    std::unique_ptr<emulation::apacket> mCachedPacket = nullptr;
    std::unique_ptr<emulation::apacket> mloadedCachedPacket = nullptr;
    std::unordered_set<uint32_t> mPendingGuestReplyCommandIds;
    int mBreakpointRequestId = 0;
    int mBreakpointEventId = 0;
    int64_t mLastSendMs = 0;
};
}  // namespace jdwp
}  // namespace android