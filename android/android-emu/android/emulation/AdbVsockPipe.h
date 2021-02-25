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
#include "android/base/files/Stream.h"
#include "android/base/sockets/ScopedSocket.h"
#include "android/base/synchronization/MessageChannel.h"
#include "android/emulation/AdbTypes.h"
#include "android/emulation/SocketBuffer.h"
#include "android/emulation/virtio_vsock_device.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

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

        static void save(base::Stream* stream);
        static bool load(base::Stream* stream);
        static IVsockHostCallbacks* getHostCallbacks(uint64_t key);

    private:
        friend AdbVsockPipe;

        void destroyPipesThreadLoop();
        void destroyPipe(AdbVsockPipe *);

        void pollGuestAdbdThreadLoop();
        bool checkIfGuestAdbdAlive();

        void reset();
        void saveImpl(base::Stream* stream) const;
        bool loadImpl(base::Stream* stream);
        IVsockHostCallbacks* getHostCallbacksImpl(uint64_t key) const;

        AdbHostAgent* mHostAgent;
        std::atomic<bool> mGuestAdbdPollingThreadRunning = true;
        std::vector<std::unique_ptr<AdbVsockPipe>> mPipes;
        base::MessageChannel<AdbVsockPipe *, 4> mPipesToDestroy;
        std::thread mGuestAdbdPollingThread;
        std::thread mDestroyPipesThread;
        mutable std::mutex mPipesMtx;
    };

    static std::unique_ptr<AdbVsockPipe> create(
        Service *service,
        android::base::ScopedSocket socket,
        AdbPortType portType);

    AdbVsockPipe(Service *service);

    AdbVsockPipe(Service *service,
                 android::base::ScopedSocket socket,
                 AdbPortType portType);

    ~AdbVsockPipe();

    struct Proxy {
        enum class EventBits {
            None = 0,
            CloseSocket = 1u << 0,
            WantWrite = 1u << 1,
            DontWantWrite = 1u << 2,
            WantRead = 1u << 3,
            DontWantRead = 1u << 4,
        };

        virtual ~Proxy() {}
        virtual AdbPortType portType() const = 0;
        virtual EventBits onHostSocketEvent(int fd, uint64_t guestKey, unsigned events) = 0;
        virtual EventBits onGuestSend(const void *data, size_t size) = 0;
        virtual bool canSave() const { return false; }
        virtual void onSave(base::Stream* stream) const {}
    };

private:
    struct DataVsockCallbacks : public IVsockHostCallbacks {
        explicit DataVsockCallbacks(AdbVsockPipe *p) : pipe(p) {}
        ~DataVsockCallbacks() override;

        void onConnect() override {}

        void onReceive(const void *data, size_t size) override {
            pipe->onGuestSend(data, size);
        }

        void onClose() override {
            pipe->onGuestClose();
        }

        bool canSave() const override {
            return pipe->canSave();
        }

        bool isValid() const {
            return streamKey;
        }

        void reset() {
            streamKey = 0;
        }

        void close();

        AdbVsockPipe *const pipe;
        uint64_t streamKey = 0;
    };

    friend Service;
    friend DataVsockCallbacks;

    void onHostSocketEvent(unsigned events);
    void onHostSocketEventSimple(unsigned events);
    void onHostSocketEventTranslated(unsigned events);
    void onGuestSend(const void *data, size_t size);
    void onGuestClose();
    void processProxyEventBits(Proxy::EventBits bits);
    void setSocket(android::base::ScopedSocket socket);

    bool canSave() const;
    void save(base::Stream* stream) const;
    bool loadImpl(base::Stream* stream);
    static std::unique_ptr<AdbVsockPipe> load(Service* service, base::Stream* stream);

    Service *const mService;
    android::base::ScopedSocket mSocket;
    std::unique_ptr<Proxy> mProxy;
    std::unique_ptr<android::base::Looper::FdWatch> mSocketWatcher;
    DataVsockCallbacks mVsockCallbacks;
};

}  // namespace emulation
}  // namespace android
