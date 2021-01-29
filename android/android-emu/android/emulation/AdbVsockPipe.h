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
#include "android/emulation/virtio_vsock_device.h"

#include <atomic>
#include <chrono>
#include <future>
#include <memory>
#include <thread>
#include <unordered_set>

namespace android {
namespace emulation {

class AdbVsockPipe {
public:
    static constexpr int kGuestAdbdPort = 5555;

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

    AdbVsockPipe(android::base::ScopedSocket socket, AdbPortType portType);

private:
    struct DataVsockCallbacks : public IVsockHostCallbacks {
        DataVsockCallbacks(AdbVsockPipe *p) : pipe(p) {}
        ~DataVsockCallbacks() override;

        void setConnected() override {
            isConnected.set_value();
        }

        void receive(const void *data, size_t size) override {
            pipe->onGuestSend(data, size);
        }

        bool waitConnected(const std::chrono::milliseconds timeout) {
            return isConnected.get_future().wait_for(timeout) == std::future_status::ready;
        }

        AdbVsockPipe *const pipe;
        uint64_t streamKey = 0;
        std::promise<void> isConnected;
    };

    friend Service;
    friend DataVsockCallbacks;

    void onHostSocketEvent(unsigned events);
    void onGuestSend(const void *data, size_t size);
    void onGuestReceived();

    CrossSessionSocket mSocket;
    std::unique_ptr<android::base::Looper::FdWatch> mSocketWatcher;
    //std::unique_ptr<AdbHub> mAdbHub;
    AdbPortType mPortType;
    DataVsockCallbacks mVsockCallbacks;
    SocketBuffer mGuestToHost;
    bool mReuseFromSnapshot = false;
    mutable std::mutex mGuestToHostMutex;
};

}  // namespace emulation
}  // namespace android
