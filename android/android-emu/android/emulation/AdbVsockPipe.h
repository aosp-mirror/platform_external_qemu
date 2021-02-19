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
#include "android/base/synchronization/MessageChannel.h"
#include "android/emulation/AdbTypes.h"
#include "android/emulation/AdbHub.h"
#include "android/emulation/CrossSessionSocket.h"
#include "android/emulation/SocketBuffer.h"
#include "android/emulation/virtio_vsock_device.h"

#include <atomic>
#include <chrono>
#include <future>
#include <memory>
#include <thread>

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
        friend AdbVsockPipe;

        void destroyPipesThreadLoop();
        void destroyPipe(AdbVsockPipe *);

        void pollGuestAdbdThreadLoop();
        bool checkIfGuestAdbdAlive();

        AdbHostAgent* mHostAgent;
        std::atomic<bool> mGuestAdbdPollingThreadRunning = true;
        std::vector<std::unique_ptr<AdbVsockPipe>> mPipes;
        base::MessageChannel<AdbVsockPipe *, 4> mPipesToDestroy;
        std::mutex mPipesMtx;
        std::thread mGuestAdbdPollingThread;
        std::thread mDestroyPipesThread;
    };

    static std::unique_ptr<AdbVsockPipe> create(
        Service *service,
        android::base::ScopedSocket socket,
        AdbPortType portType);

    AdbVsockPipe(Service *service,
                 android::base::ScopedSocket socket,
                 AdbPortType portType);

    ~AdbVsockPipe();

private:
    struct DataVsockCallbacks : public IVsockHostCallbacks {
        DataVsockCallbacks(AdbVsockPipe *p) : pipe(p) {}
        ~DataVsockCallbacks() override;

        void onConnect() override {
            isConnected.set_value();
        }

        void onReceive(const void *data, size_t size) override {
            pipe->onGuestSend(data, size);
        }

        void onClose() override {
            pipe->onGuestClose();
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
    void onHostSocketEventSimple(unsigned events);
    void onHostSocketEventTranslated(unsigned events);
    void onGuestSend(const void *data, size_t size);
    void onGuestClose();

    Service *const mService;
    const AdbPortType mPortType;
    CrossSessionSocket mSocket;
    SocketBuffer mGuestToHost;
    DataVsockCallbacks mVsockCallbacks;
    std::unique_ptr<android::base::Looper::FdWatch> mSocketWatcher;
    std::unique_ptr<AdbHub> mAdbHub;
    std::thread mConnectThread;
    mutable std::mutex mGuestToHostMutex;
};

}  // namespace emulation
}  // namespace android
