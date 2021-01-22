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
#include "android/base/threads/Async.h"
#include "android/emulation/AdbVsockPipe.h"
#include "android/crashreport/crash-handler.h"

#include <chrono>
#include <future>

using FdWatch = android::base::Looper::FdWatch;

uint64_t virtio_vsock_device_stream_open(uint32_t guest_port,
                                         uint32_t host_port_offset,
                                         IVsockHostCallbacks *) {
    fprintf(stderr, "rkir555 %s:%d guest_port=%u host_port_offset=%u\n",
            __func__, __LINE__, guest_port, host_port_offset);
    return 0;
}

size_t virtio_vsock_device_stream_send(uint64_t key, const void *data, size_t size) {
    fprintf(stderr, "rkir555 %s:%d key=%llu size=%zu\n",
            __func__, __LINE__, (unsigned long long)key, size);
    return 0;
}

bool virtio_vsock_device_stream_close(uint64_t key) {
    fprintf(stderr, "rkir555 %s:%d key=%llu\n",
            __func__, __LINE__, (unsigned long long)key);
    return false;
}

const virtio_vsock_device_ops_t empty_ops = {
    .open = &virtio_vsock_device_stream_open,
    .send = &virtio_vsock_device_stream_send,
    .close = &virtio_vsock_device_stream_close,
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
    , mGuestAdbdPollingThread([this](){ pollGuestAdbd(); })
{}

AdbVsockPipe::Service::~Service() {
    mGuestAdbdPollingThreadRunning = false;
    mGuestAdbdPollingThread.join();
}

void AdbVsockPipe::Service::onHostConnection(android::base::ScopedSocket&& socket,
                                             AdbPortType portType) {
    auto pipe = AdbVsockPipe::create(std::move(socket), portType);
    if (pipe) {
        mPipes.insert(std::move(pipe));
    }
}

void AdbVsockPipe::Service::pollGuestAdbd() {
    bool listening = false;

    while (mGuestAdbdPollingThreadRunning) {
        const bool alive = checkIfGuestAdbdAlive();
        if (listening != alive) {
            if (alive) {
                fprintf(stderr, "rkir555 %s:%s:%d startListening\n",
                        "AdbVsockPipe::Service", __func__, __LINE__);
                mHostAgent->startListening();
                mHostAgent->notifyServer();
            } else {
                mHostAgent->stopListening();
                fprintf(stderr, "rkir555 %s:%s:%d stopListening\n",
                        "AdbVsockPipe::Service", __func__, __LINE__);
            }
            listening = alive;
        }

        using namespace std::chrono_literals;
        std::this_thread::sleep_for(500ms);
    }

    if (listening) {
        mHostAgent->stopListening();
    }
}

bool AdbVsockPipe::Service::checkIfGuestAdbdAlive() {
    struct VsockHostCallbacks : public IVsockHostCallbacks {
        ~VsockHostCallbacks() override {
            if (streamKey) {
                (*g_vsock_ops->close)(streamKey);
            }
        }

        void setConnected() override {
            isConnected.set_value();
        }

        void receive(const void *data, size_t size) override {}
        void sent() override {}

        bool waitConnected(const std::chrono::milliseconds timeout) {
            return isConnected.get_future().wait_for(timeout) == std::future_status::ready;
        }

        uint64_t streamKey = 0;
        std::promise<void> isConnected;
    };

    VsockHostCallbacks callbacks;
    callbacks.streamKey = (*g_vsock_ops->open)(kGuestAdbdPort, 0, &callbacks);

    using namespace std::chrono_literals;
    return callbacks.streamKey && callbacks.waitConnected(100ms);
}

void AdbVsockPipe::Service::resetActiveGuestPipeConnection() {
    fprintf(stderr, "rkir555 %s:%s:%d this=%p\n",
            "AdbVsockPipe::Service", __func__, __LINE__, this);

    mPipes.clear();
}

AdbVsockPipe::AdbVsockPipe(android::base::ScopedSocket socket,
                           AdbPortType portType)
        : mSocket(std::move(socket)), mPortType(portType), mVsockCallbacks(this) {
    mVsockCallbacks.streamKey = (*g_vsock_ops->open)(kGuestAdbdPort, 100000, &mVsockCallbacks);
    if (!mVsockCallbacks.streamKey) {
        ::crashhandler_die("%s:%d: streamKey==0", __func__, __LINE__);
    }

    mSocketWatcher.reset(android::base::ThreadLooper::get()->createFdWatch(
        mSocket.fd(),
        [](void* opaque, int fd, unsigned events) {
            static_cast<AdbVsockPipe*>(opaque)->onHostSocketEvent(events);
        },
        this));

    android::base::async([this](){
        using namespace std::chrono_literals;

        fprintf(stderr, "rkir555 %s:%s:%d before waitConnected\n",
                "AdbVsockPipe", __func__, __LINE__);

        if (!mVsockCallbacks.waitConnected(1000ms)) {
            ::crashhandler_die("%s:%d: waitConnected failed", __func__, __LINE__);
        }

        fprintf(stderr, "rkir555 %s:%s:%d after waitConnected\n",
                "AdbVsockPipe", __func__, __LINE__);

        mSocketWatcher->wantRead();

        fprintf(stderr, "rkir555 %s:%s:%d done\n",
                "AdbVsockPipe", __func__, __LINE__);
    });
}

std::unique_ptr<AdbVsockPipe> AdbVsockPipe::create(android::base::ScopedSocket socket,
                                                   AdbPortType portType) {
    android::base::socketSetNonBlocking(socket.get());
    android::base::socketSetNoDelay(socket.get());

    return std::make_unique<AdbVsockPipe>(std::move(socket), portType);
}

void AdbVsockPipe::onHostSocketEvent(unsigned events) {
    if (events & FdWatch::kEventRead) {
        const bool wasEmpty = mHostToGuest.isEmpty();

        while (true) {
            char data[256];
            const size_t received = android::base::socketRecv(mSocket.fd(),
                                                              data, sizeof(data));

            if (!received || (received > sizeof(data))) {
                break;
            }

            fprintf(stderr, "rkir555 %s:%s:%d kEventRead received=%zu\n",
                    "AdbVsockPipe", __func__, __LINE__, received);

            mHostToGuest.append(data, received);
        }

        if (wasEmpty) {
            onGuestReceived();
        } else {
            fprintf(stderr, "rkir555 %s:%s:%d not empty\n",
                    "AdbVsockPipe", __func__, __LINE__);
        }
    }

    if (events & FdWatch::kEventWrite) {
        while (true) {
            const auto buf = mGuestToHost.peek();
            if (!buf.second) {
                mSocketWatcher->dontWantWrite();
                break;
            }

            const size_t sent = android::base::socketSend(
                mSocket.fd(), buf.first, buf.second);

            if (!sent || (sent > buf.second)) {
                break;
            }

            fprintf(stderr, "rkir555 %s:%s:%d kEventWrite sent=%zu\n",
                    "AdbVsockPipe", __func__, __LINE__, sent);
            mGuestToHost.consume(sent);
        }
    }
}

void AdbVsockPipe::onGuestSend(const void *data, size_t size) {
    mGuestToHost.append(data, size);
    mSocketWatcher->wantWrite();
}

void AdbVsockPipe::onGuestReceived() {
    const auto buf = mHostToGuest.peek();
    if (buf.second) {
        const size_t sent = (*g_vsock_ops->send)(mVsockCallbacks.streamKey,
                                                 buf.first, buf.second);
        mHostToGuest.consume(sent);

        fprintf(stderr, "rkir555 %s:%s:%d sz=%zu sent=%zu\n",
                "AdbVsockPipe", __func__, __LINE__, buf.second, sent);
    }
}

AdbVsockPipe::VsockCallbacks::~VsockCallbacks() {
    if (streamKey) {
        (*g_vsock_ops->close)(streamKey);
    }
}

/*void AdbVsockPipe::onGuestClose(PipeCloseReason reason) {
    fprintf(stderr, "rkir555 %s:%s:%d this=%p reason=%d\n",
            "AdbVsockPipe", __func__, __LINE__, this, reason);
}

unsigned AdbVsockPipe::onGuestPoll() const {
    fprintf(stderr, "rkir555 %s:%s:%d this=%p\n",
            "AdbVsockPipe", __func__, __LINE__, this);
    return 0;
}

int AdbVsockPipe::onGuestRecv(AndroidPipeBuffer* buffers, int count) {
    fprintf(stderr, "rkir555 %s:%s:%d this=%p buffers=%p count=%d\n",
            "AdbVsockPipe", __func__, __LINE__, this, buffers, count);
    return 0;
}

int AdbVsockPipe::onGuestSend(const AndroidPipeBuffer* buffers,
                              int count, void** newPipePtr) {
    fprintf(stderr, "rkir555 %s:%s:%d this=%p buffers=%p count=%d\n",
            "AdbVsockPipe", __func__, __LINE__, this, buffers, count);
    return 0;
}

void AdbVsockPipe::onGuestWantWakeOn(int flags) {
    fprintf(stderr, "rkir555 %s:%s:%d this=%p\n",
            "AdbVsockPipe", __func__, __LINE__, this);
}

void AdbVsockPipe::onSave(android::base::Stream* stream) {
    fprintf(stderr, "rkir555 %s:%s:%d this=%p\n",
            "AdbVsockPipe", __func__, __LINE__, this);
}*/

}  // namespace emulation
}  // namespace android
