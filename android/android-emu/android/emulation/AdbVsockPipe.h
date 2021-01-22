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

#include "android/base/async/Looper.h"
#include "android/emulation/AdbTypes.h"
#include "android/emulation/AdbHub.h"
#include "android/emulation/CrossSessionSocket.h"
//#include "android/emulation/AndroidPipe.h"
//#include "hw/virtio/virtio-vsock.h"

#include <atomic>
#include <memory>
#include <thread>
#include <unordered_set>

namespace android {
namespace emulation {

class AdbVsockPipe {
public:
    class Service : public AdbGuestAgent {
    public:
        Service(AdbHostAgent* hostAgent);
        ~Service();

        void onHostConnection(android::base::ScopedSocket&& socket,
                              AdbPortType portType) override;

        void resetActiveGuestPipeConnection() override;

    private:
        void pollGuestAdbd();
        bool checkIfGuestAdbdAlive();

        AdbHostAgent* mHostAgent;
        std::thread mGuestAdbdPollingThread;
        std::atomic<bool> mGuestAdbdPollingThreadRunning = true;
        std::unordered_set<std::unique_ptr<AdbVsockPipe>> mPipes;
        std::unique_ptr<android::base::Looper::FdWatch> mFdWatcher;
    };

    static std::unique_ptr<AdbVsockPipe> create(
        android::base::ScopedSocket socket,
        AdbPortType portType);

//    void onGuestClose(PipeCloseReason reason) override;
//    unsigned onGuestPoll() const override;
//    int onGuestRecv(AndroidPipeBuffer* buffers, int count) override;
//    int onGuestSend(const AndroidPipeBuffer* buffers,
//                    int count, void** newPipePtr) override;
//    void onGuestWantWakeOn(int flags) override;
//    void onSave(android::base::Stream* stream) override;

    AdbVsockPipe(const void* guest,
                 android::base::ScopedSocket socket,
                 AdbPortType portType);

private:
    friend Service;

    static const void* connectToGuestAdbd();
    void onHostSocketEvent(unsigned events);

    CrossSessionSocket mSocket;
    std::unique_ptr<android::base::Looper::FdWatch> mSocketWatcher;
    std::unique_ptr<AdbHub> mAdbHub;
    AdbPortType mPortType;
    bool mReuseFromSnapshot = false;
};

}  // namespace emulation
}  // namespace android
