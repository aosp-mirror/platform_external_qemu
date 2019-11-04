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

#include <functional>
#include <memory>

#include "android/base/files/Stream.h"
#include "android/base/threads/Thread.h"
#include "android/emulation/android_pipe_common.h"
#include "android/emulation/AdbHub.h"

namespace android {
namespace jdwp {
class JdwpProxy: public emulation::AdbProxy {
public:
    JdwpProxy(int hostId, int guestPid);
    bool needProxyRecvData(const android::emulation::AdbMessageSniffer::amessage* mesg) override;
    void onGuestRecvData(const char* data, size_t len) override;
    bool needProxySendData(const android::emulation::AdbMessageSniffer::amessage* mesg) override;
    void onGuestSendData(const char* data, size_t len) override;
    bool shouldClose() const override;
    //int32_t currentHostId() const override;
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
    // TODO: atomic?
    State mProxyState = State::Uninitialized;
    State mClientState = State::Uninitialized;
    int mHostId = 0;
    //int mCurrentHostId = 0; // TODO
    int mGuestId = 0;
    int mGuestPid = 0;
    void onSave(android::base::Stream* stream);
    void onLoad(android::base::Stream* stream);
    void onGuestSendData(const AndroidPipeBuffer* buffers,
                         int numBuffers,
                         size_t actualSentBytes);
    void onGuestRecvData(const AndroidPipeBuffer* buffers,
                         int numBuffers,
                         size_t actualRecvBytes);
    bool shouldBlockGuest() const;
    bool shouldRegister() const;
    bool shouldCheckRegistration() const;
    bool shouldUnregister() const;
    void setRegistered(bool registered);
    // Set up a fake server to talk to the client asynchronizely.
    // Hold a weak pointer to the socket. If it is closed in the middle
    // then just shut down the thread.
    void fakeServerAsync(int socket,
            std::function<void()> serverDoneCallback);
    // Wait and reset
    void resetServerThread();
private:
    bool mRegistered = false;
    std::unique_ptr<base::Thread> mFakeServerThread = {};
    bool mShouldClose = false;
};
}  // namespace jdwp
}  // namespace android