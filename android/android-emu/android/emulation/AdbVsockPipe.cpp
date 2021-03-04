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
#include "android/emulation/AdbHub.h"
#include "android/emulation/AdbVsockPipe.h"
#include "android/crashreport/crash-handler.h"

#include <algorithm>
#include <chrono>
#include <future>
#include <numeric>

namespace {
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

android::emulation::AdbVsockPipe::Service *g_service = nullptr;
}  // namespace

const virtio_vsock_device_ops_t *
virtio_vsock_device_set_ops(const virtio_vsock_device_ops_t *ops) {
    const virtio_vsock_device_ops_t *old = g_vsock_ops;
    g_vsock_ops = ops ? ops : &empty_ops;
    return old;
}

namespace android {
namespace emulation {
namespace {

struct VsockAdbProxy : public AdbVsockPipe::Proxy {
    AdbPortType portType() const override { return AdbPortType::RegularAdb; }

    EventBits onHostSocketEvent(const int hostFd,
                                const uint64_t guestKey,
                                const unsigned events) override {
        char buf[2048];

        EventBits result = EventBits::None;

        if (events & FdWatch::kEventRead) {
            while (true) {
                const size_t received = android::base::socketRecv(hostFd, buf, sizeof(buf));
                if (!received) {
                    result |= EventBits::CloseSocket;  // hostFd is closed
                    break;
                }

                if (received > sizeof(buf)) {
                    break; // retry later
                }

                const size_t sent = (*g_vsock_ops->send)(guestKey, buf, received);
                if (sent != received) {
                    result |= EventBits::CloseSocket;  // guestKey is closed
                    break;
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
                        result |= EventBits::DontWantWrite;
                        break;
                    }
                }

                const size_t sent = android::base::socketSend(hostFd, buf, sz);
                if (!sent || (sent > sz)) {
                    break;
                }

                std::lock_guard<std::mutex> guard(mGuestToHostMutex);
                mGuestToHost.consume(sent);
            }
        }

        return result;
    }

    EventBits onGuestSend(const void *data, size_t size) override {
        std::lock_guard<std::mutex> guard(mGuestToHostMutex);
        mGuestToHost.append(data, size);
        return EventBits::WantWrite;
    }

    static std::unique_ptr<VsockAdbProxy> load(base::Stream* stream) {
        return nullptr;  // not supported
    }

    SocketBuffer mGuestToHost;
    mutable std::mutex mGuestToHostMutex;
};

struct VsockJdwpProxy : public AdbVsockPipe::Proxy {
    AdbPortType portType() const override { return AdbPortType::Jdwp; }

    EventBits onHostSocketEvent(const int hostFd,
                                const uint64_t guestKey,
                                const unsigned events) override {
        EventBits result = EventBits::None;

        mAdbHub.onHostSocketEvent(hostFd, events, [this, &result]() {
            result = EventBits::CloseSocket;
        });

        if (result == EventBits::None) {
            uint8_t buf[2048];
            AndroidPipeBuffer abuf;
            abuf.data = buf;
            abuf.size = sizeof(buf);

            while (true) {
                const int sz = mAdbHub.onGuestRecvData(&abuf, 1);
                if (sz > 0) {
                    if ((*g_vsock_ops->send)(guestKey, buf, sz) != sz) {
                        result |= EventBits::CloseSocket;  // guestKey is closed
                        break;
                    }
                } else {
                    break;
                }
            }

            if (mAdbHub.socketWantRead()) {
                result |= EventBits::WantRead;
            } else {
                result |= EventBits::DontWantRead;
            }

            if (mAdbHub.socketWantWrite()) {
                result |= EventBits::WantWrite;
            } else {
                result |= EventBits::DontWantWrite;
            }
        }

        return result;
    }

    EventBits onGuestSend(const void *data, size_t size) override {
        AndroidPipeBuffer abuf;
        abuf.data = static_cast<uint8_t *>(const_cast<void *>(data));
        abuf.size = size;

        mAdbHub.onGuestSendData(&abuf, 1);

        return mAdbHub.socketWantWrite()
            ? EventBits::WantWrite : EventBits::DontWantWrite;
    }

    bool canSave() const override { return true; }

    void onSave(base::Stream* stream) const override {
        mAdbHub.onSave(stream);
    }

    static std::unique_ptr<VsockJdwpProxy> load(base::Stream* stream) {
        auto proxy = std::make_unique<VsockJdwpProxy>();
        proxy->mAdbHub.onLoad(stream);
        return proxy;
    }

    AdbHub mAdbHub;
};

}  // namespace


AdbVsockPipe::Service::Service(AdbHostAgent* hostAgent)
    : mHostAgent(hostAgent)
    , mDestroyPipesThread(&AdbVsockPipe::Service::destroyPipesThreadLoop, this) {
    startPollGuestAdbdThread();
    g_service = this;
}

AdbVsockPipe::Service::~Service() {
    g_service = nullptr;

    stopPollGuestAdbdThread(kAdbdPollingThreadDisabled);

    mPipesToDestroy.stop();
    mDestroyPipesThread.join();
}

void AdbVsockPipe::Service::onHostConnection(android::base::ScopedSocket&& socket,
                                             const AdbPortType portType) {
    {
        std::unique_lock<std::mutex> guard(mPipesMtx);

        const auto i = std::find_if(mPipes.begin(), mPipes.end(),
                                    [](const std::unique_ptr<AdbVsockPipe>& p){
            return !p->mSocket.valid();
        });

        if (i != mPipes.end()) {
            (*i)->setSocket(std::move(socket));
            return;
        }
    }

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
            if (mPipes.empty()) {
                stopPollGuestAdbdThread(kAdbdPollingThreadIdle);
                startPollGuestAdbdThread();
            }
        } else {
            break;
        }
    }
}

void AdbVsockPipe::Service::destroyPipe(AdbVsockPipe *p) {
    mPipesToDestroy.send(p);
}

void AdbVsockPipe::Service::pollGuestAdbdThreadLoop() {
    while (mGuestAdbdPollingThreadState.load() == kAdbdPollingThreadRunning) {
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

void AdbVsockPipe::Service::startPollGuestAdbdThread() {
    auto expectedState = kAdbdPollingThreadIdle;
    if (mGuestAdbdPollingThreadState.compare_exchange_strong(expectedState,
                                                             kAdbdPollingThreadRunning)) {
        mGuestAdbdPollingThread = std::thread(
            &AdbVsockPipe::Service::pollGuestAdbdThreadLoop, this);
    }
}

void AdbVsockPipe::Service::stopPollGuestAdbdThread(const int newState) {
    int expectedState = kAdbdPollingThreadRunning;

    while (expectedState != newState) {
        switch (expectedState) {
        case kAdbdPollingThreadRunning:
            if (mGuestAdbdPollingThreadState.compare_exchange_strong(expectedState, newState)) {
                mHostAgent->stopListening();
                mGuestAdbdPollingThread.join();
                return;
            }
            break;

        case kAdbdPollingThreadIdle:
            if (mGuestAdbdPollingThreadState.compare_exchange_strong(expectedState, newState)) {
                return;
            }
            break;

        case kAdbdPollingThreadDisabled:
            return;

        default:
            ::crashhandler_die("%s:%d: unexpected adbd polling thread state: %d",
                               __func__, __LINE__, expectedState);
            break;
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

void AdbVsockPipe::Service::saveImpl(base::Stream* stream) const {
    std::unique_lock<std::mutex> guard(mPipesMtx);

    const size_t n = std::accumulate(
        mPipes.begin(), mPipes.end(), 0,
        [](size_t n, const std::unique_ptr<AdbVsockPipe>& p){
            return n + (p->canSave() ? 1 : 0);
        });

    stream->putBe32(n);
    if (n > 0) {
        for (const auto& p : mPipes) {
            if (p->canSave()) {
                p->save(stream);
            }
        }
    }
}

bool AdbVsockPipe::Service::loadImpl(base::Stream* stream) {
    std::unique_lock<std::mutex> guard(mPipesMtx);
    mPipes.clear();
    for (uint32_t n = stream->getBe32(); n > 0; --n) {
        auto p = AdbVsockPipe::load(this, stream);
        if (p) {
            mPipes.push_back(std::move(p));
        }
    }
    return true;
}

IVsockHostCallbacks* AdbVsockPipe::Service::getHostCallbacksImpl(uint64_t key) const {
    std::unique_lock<std::mutex> guard(mPipesMtx);
    for (const auto& p : mPipes) {
        if (p->mVsockCallbacks.streamKey == key) {
            return &p->mVsockCallbacks;
        }
    }
    return nullptr;
}

void AdbVsockPipe::Service::save(base::Stream* stream) {
    g_service->saveImpl(stream);
}

bool AdbVsockPipe::Service::load(base::Stream* stream) {
    return g_service->loadImpl(stream);
}

IVsockHostCallbacks* AdbVsockPipe::Service::getHostCallbacks(uint64_t key) {
    return g_service->getHostCallbacksImpl(key);
}

AdbVsockPipe::AdbVsockPipe(Service *service)
        : mService(service)
        , mVsockCallbacks(this) {}

AdbVsockPipe::AdbVsockPipe(AdbVsockPipe::Service *service,
                           android::base::ScopedSocket socket,
                           const AdbPortType portType)
        : mService(service)
        , mVsockCallbacks(this) {
    mVsockCallbacks.streamKey = (*g_vsock_ops->open)(kGuestAdbdPort, &mVsockCallbacks);
    if (!mVsockCallbacks.streamKey) {
        ::crashhandler_die("%s:%d: streamKey==0", __func__, __LINE__);
    }

    switch (portType) {
    case AdbPortType::RegularAdb:
        mProxy = std::make_unique<VsockAdbProxy>();
        break;

    case AdbPortType::Jdwp:
        mProxy = std::make_unique<VsockJdwpProxy>();
        break;

    default:
        ::crashhandler_die("%s:%d: unexpected portType", __func__, __LINE__);
        break;
    }

    setSocket(std::move(socket));
}

AdbVsockPipe::~AdbVsockPipe() {
    mVsockCallbacks.close();
    processProxyEventBits(Proxy::EventBits::DontWantRead |
                          Proxy::EventBits::DontWantWrite);
}

std::unique_ptr<AdbVsockPipe> AdbVsockPipe::create(AdbVsockPipe::Service *service,
                                                   android::base::ScopedSocket socket,
                                                   AdbPortType portType) {
    android::base::socketSetNonBlocking(socket.get());
    android::base::socketSetNoDelay(socket.get());

    return std::make_unique<AdbVsockPipe>(service, std::move(socket), portType);
}

void AdbVsockPipe::onHostSocketEvent(unsigned events) {
    processProxyEventBits(mProxy->onHostSocketEvent(mSocket.get(),
                                                    mVsockCallbacks.streamKey,
                                                    events));
}

void AdbVsockPipe::onGuestSend(const void *data, size_t size) {
    processProxyEventBits(mProxy->onGuestSend(data, size));
}

void AdbVsockPipe::onGuestClose() {
    mVsockCallbacks.reset();
    mService->destroyPipe(this);
}

void AdbVsockPipe::processProxyEventBits(const Proxy::EventBits bits) {
    if (nonzero(bits & Proxy::EventBits::CloseSocket)) {
        mService->destroyPipe(this);
    } else if (mSocketWatcher) {
        if (nonzero(bits & Proxy::EventBits::WantWrite)) {
            mSocketWatcher->wantWrite();
        }
        if (nonzero(bits & Proxy::EventBits::DontWantWrite)) {
            mSocketWatcher->dontWantWrite();
        }
        if (nonzero(bits & Proxy::EventBits::WantRead)) {
            mSocketWatcher->wantRead();
        }
        if (nonzero(bits & Proxy::EventBits::DontWantRead)) {
            mSocketWatcher->dontWantRead();
        }
    }
}

void AdbVsockPipe::setSocket(android::base::ScopedSocket socket) {
    mSocket = std::move(socket);

    mSocketWatcher.reset(android::base::ThreadLooper::get()->createFdWatch(
        mSocket.get(),
        [](void* opaque, int fd, unsigned events) {
            static_cast<AdbVsockPipe*>(opaque)->onHostSocketEvent(events);
        },
        this));

    mSocketWatcher->wantRead();
}

bool AdbVsockPipe::canSave() const {
    return mProxy && mProxy->canSave() && mVsockCallbacks.isValid();
}

void AdbVsockPipe::save(base::Stream* stream) const {
    stream->putBe64(mVsockCallbacks.streamKey);
    stream->putByte(static_cast<uint8_t>(mProxy->portType()));
    mProxy->onSave(stream);
}

bool AdbVsockPipe::loadImpl(base::Stream* stream) {
    mVsockCallbacks.streamKey = stream->getBe64();

    switch (static_cast<AdbPortType>(stream->getByte())) {
    case AdbPortType::RegularAdb:
        mProxy = VsockAdbProxy::load(stream);
        break;

    case AdbPortType::Jdwp:
        mProxy = VsockJdwpProxy::load(stream);
        break;

    default:
        return false;
    }

    if (!mVsockCallbacks.isValid() || !mProxy) {
        return false;
    }

    return true;
}

std::unique_ptr<AdbVsockPipe> AdbVsockPipe::load(AdbVsockPipe::Service* service,
                                                 base::Stream* stream) {
    auto pipe = std::make_unique<AdbVsockPipe>(service);
    if (pipe->loadImpl(stream)) {
        return pipe;
    } else {
        return nullptr;
    }
}

void AdbVsockPipe::DataVsockCallbacks::close() {
    if (streamKey) {
        (*g_vsock_ops->close)(streamKey);
        reset();
    }
}

AdbVsockPipe::DataVsockCallbacks::~DataVsockCallbacks() {
    if (streamKey) {
        ::crashhandler_die("%s:%d: streamKey is not zero", __func__, __LINE__);
    }
}

}  // namespace emulation
}  // namespace android
