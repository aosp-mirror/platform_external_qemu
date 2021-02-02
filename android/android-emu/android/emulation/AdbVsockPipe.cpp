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

#include "android/base/async/ThreadLooper.h"
#include "android/emulation/AdbVsockPipe.h"
#include "android/crashreport/crash-handler.h"

#include <chrono>
#include <future>

using FdWatch = android::base::Looper::FdWatch;

uint64_t empty_virtio_vsock_device_stream_open(uint32_t guest_port,
                                               IVsockHostCallbacks *) {
    return 0;
}

size_t empty_virtio_vsock_device_stream_send(uint64_t key,
                                             const void *data, size_t size) {
    return 0;
}

bool empty_virtio_vsock_device_stream_close(uint64_t key) {
    return false;
}

const virtio_vsock_device_ops_t empty_ops = {
    .open = &empty_virtio_vsock_device_stream_open,
    .send = &empty_virtio_vsock_device_stream_send,
    .close = &empty_virtio_vsock_device_stream_close,
};

const virtio_vsock_device_ops_t *g_vsock_ops = &empty_ops;

const virtio_vsock_device_ops_t *
virtio_vsock_device_set_ops(const virtio_vsock_device_ops_t *ops) {
    const virtio_vsock_device_ops_t *old = g_vsock_ops;
    g_vsock_ops = ops ? ops : &empty_ops;
    return old;
}

namespace android {
namespace emulation {

AdbVsockPipe::Service::Service(AdbHostAgent* hostAgent)
    : mHostAgent(hostAgent)
    , mGuestAdbdPollingThread(&AdbVsockPipe::Service::pollGuestAdbdThreadLoop, this)
    , mDestroyPipesThread(&AdbVsockPipe::Service::pollGuestAdbdThreadLoop, this)
{}

AdbVsockPipe::Service::~Service() {
    mGuestAdbdPollingThreadRunning = false;
    mPipesToDestroy.stop();
    mGuestAdbdPollingThread.join();
    mDestroyPipesThread.join();
    mHostAgent->stopListening();
}

void AdbVsockPipe::Service::onHostConnection(android::base::ScopedSocket&& socket,
                                             AdbPortType portType) {
    auto pipe = AdbVsockPipe::create(this, std::move(socket), portType);
    if (pipe) {
        std::unique_lock<std::mutex> guard(mPipesMtx);
        mPipes.push_back(std::move(pipe));
    }
}

void AdbVsockPipe::Service::destroyPipesThreadLoop() {
    while (true) {
        AdbVsockPipe *toDestroy;
        if (mPipesToDestroy.receive(&toDestroy)) {
            std::unique_lock<std::mutex> guard(mPipesMtx);

            const auto end = std::remove_if(
                mPipes.begin(), mPipes.end(),
                [toDestroy](const std::unique_ptr<AdbVsockPipe> &pipe){
                    return toDestroy == pipe.get();
                });

            mPipes.erase(end, mPipes.end());
        } else {
            break;
        }
    }
}

void AdbVsockPipe::Service::destroyPipe(AdbVsockPipe *p) {
    mPipesToDestroy.send(p);
}

void AdbVsockPipe::Service::pollGuestAdbdThreadLoop() {
    while (mGuestAdbdPollingThreadRunning) {
        if (checkIfGuestAdbdAlive()) {
            mHostAgent->startListening();
            mHostAgent->notifyServer();
            break;
        } else {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(500ms);
        }
    }
}

bool AdbVsockPipe::Service::checkIfGuestAdbdAlive() {
    struct PollVsockHostCallbacks : public IVsockHostCallbacks {
        ~PollVsockHostCallbacks() override {
            if (streamKey) {
                (*g_vsock_ops->close)(streamKey);
            }
        }

        void onConnect() override {
            isConnected.set_value();
        }

        void onReceive(const void *data, size_t size) override {}
        void onClose() override {}

        bool waitConnected(const std::chrono::milliseconds timeout) {
            return isConnected.get_future().wait_for(timeout) == std::future_status::ready;
        }

        uint64_t streamKey = 0;
        std::promise<void> isConnected;
    };

    PollVsockHostCallbacks callbacks;
    callbacks.streamKey = (*g_vsock_ops->open)(kGuestAdbdPort, &callbacks);

    using namespace std::chrono_literals;
    return callbacks.streamKey && callbacks.waitConnected(100ms);
}

void AdbVsockPipe::Service::resetActiveGuestPipeConnection() {
    std::unique_lock<std::mutex> guard(mPipesMtx);
    mPipes.clear();
}

AdbVsockPipe::AdbVsockPipe(AdbVsockPipe::Service *service,
                           android::base::ScopedSocket socket,
                           AdbPortType portType)
        : mService(service)
        , mPortType(portType)
        , mSocket(std::move(socket))
        , mVsockCallbacks(this) {
    mVsockCallbacks.streamKey = (*g_vsock_ops->open)(kGuestAdbdPort, &mVsockCallbacks);
    if (!mVsockCallbacks.streamKey) {
        ::crashhandler_die("%s:%d: streamKey==0", __func__, __LINE__);
    }

    mSocketWatcher.reset(android::base::ThreadLooper::get()->createFdWatch(
        mSocket.fd(),
        [](void* opaque, int fd, unsigned events) {
            static_cast<AdbVsockPipe*>(opaque)->onHostSocketEvent(events);
        },
        this));

    mConnectThread = std::thread([this](){
        using namespace std::chrono_literals;

        // you cannot run blocking functions in the calling thread
        if (mVsockCallbacks.waitConnected(1000ms)) {
            mSocketWatcher->wantRead();
        } else {
            mService->destroyPipe(this);
        }
    });
}

AdbVsockPipe::~AdbVsockPipe() {
    mConnectThread.join();
}

std::unique_ptr<AdbVsockPipe> AdbVsockPipe::create(AdbVsockPipe::Service *service,
                                                   android::base::ScopedSocket socket,
                                                   AdbPortType portType) {
    android::base::socketSetNonBlocking(socket.get());
    android::base::socketSetNoDelay(socket.get());

    return std::make_unique<AdbVsockPipe>(service, std::move(socket), portType);
}

void AdbVsockPipe::onHostSocketEvent(unsigned events) {
    char buf[2048];

    if (events & FdWatch::kEventRead) {
        while (true) {
            const size_t received = android::base::socketRecv(mSocket.fd(),
                                                              buf, sizeof(buf));
            if (!received) {
                // mSocket is diconnected
                mService->destroyPipe(this);
                return;
            }

            if (received > sizeof(buf)) {
                break; // retry later
            }

            if ((*g_vsock_ops->send)(mVsockCallbacks.streamKey, buf, received) != received) {
                break;  // streamKey is closed
            }
        }
    }

    if (events & FdWatch::kEventWrite) {
        while (true) {
            size_t sz;

            {
                // we can't run android::base::socketSend under this lock
                std::lock_guard<std::mutex> guard(mGuestToHostMutex);
                const auto chunk = mGuestToHost.peek();
                if (chunk.second) {
                    sz = std::min(sizeof(buf), chunk.second);
                    memcpy(buf, chunk.first, sz);
                } else {
                    mSocketWatcher->dontWantWrite();
                    break;
                }
            }

            const size_t sent = android::base::socketSend(mSocket.fd(), buf, sz);
            if (!sent || (sent > sz)) {
                break;
            }

            std::lock_guard<std::mutex> guard(mGuestToHostMutex);
            mGuestToHost.consume(sent);
        }
    }
}

void AdbVsockPipe::onGuestSend(const void *data, size_t size) {
    std::lock_guard<std::mutex> guard(mGuestToHostMutex);

    mGuestToHost.append(data, size);
    mSocketWatcher->wantWrite();
}

void AdbVsockPipe::onGuestClose() {
    mVsockCallbacks.streamKey = 0;  // to prevent recursion in dctor
    mService->destroyPipe(this);
}

AdbVsockPipe::DataVsockCallbacks::~DataVsockCallbacks() {
    if (streamKey) {
        (*g_vsock_ops->close)(streamKey);
    }
}

}  // namespace emulation
}  // namespace android
