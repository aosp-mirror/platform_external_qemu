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

bool empty_virtio_vsock_device_stream_ping(uint64_t key) {
    return false;
}

bool empty_virtio_vsock_device_stream_close(uint64_t key) {
    return false;
}

const virtio_vsock_device_ops_t empty_ops = {
    .open = &empty_virtio_vsock_device_stream_open,
    .send = &empty_virtio_vsock_device_stream_send,
    .ping = empty_virtio_vsock_device_stream_ping,
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
                    result |= EventBits::HostClosed;
                    break;
                }

                if (received > sizeof(buf)) {
                    break; // retry later
                }

                const size_t sent = (*g_vsock_ops->send)(guestKey, buf, received);
                if (sent != received) {
                    result |= EventBits::GuestClosed;
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
            result = EventBits::HostClosed;
        });

        if (result != EventBits::None) {
            return result;
        }

        uint8_t buf[2048];
        AndroidPipeBuffer abuf;
        abuf.data = buf;
        abuf.size = sizeof(buf);

        while (true) {
            const int sz = mAdbHub.onGuestRecvData(&abuf, 1);
            if (sz > 0) {
                if ((*g_vsock_ops->send)(guestKey, buf, sz) != sz) {
                    result |= EventBits::GuestClosed;  // guestKey is closed
                    break;
                }
            } else {
                break;
            }
        }

        if (result != EventBits::None) {
            return result;
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
    , mGuestAdbdPollingThread(&AdbVsockPipe::Service::pollGuestAdbdThreadLoop, this)
    , mDestroyPipesThread(&AdbVsockPipe::Service::destroyPipesThreadLoop, this) {
    g_service = this;
}

AdbVsockPipe::Service::~Service() {
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
    g_service = nullptr;
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);

    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
    mGuestAdbdPollingThreadRunning = false;
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
    mGuestAdbdPollingThread.join();
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);

    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
    destroyPipe(nullptr);
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
    mDestroyPipesThread.join();
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
}

void AdbVsockPipe::Service::onHostConnection(android::base::ScopedSocket&& socket,
                                             const AdbPortType portType) {
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
    // A workaround.
    // `BaseSocketServer::onAccept` calls `stopListening` for some reasons.
    // Probably we want to fix `BaseSocketServer::onAccept` instead.
    mHostAgent->startListening();
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);

    {
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
        std::lock_guard<std::mutex> guard(mPipesMtx);

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
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
        std::lock_guard<std::mutex> guard(mPipesMtx);
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
        mPipes.push_back(std::move(pipe));
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
    }
}

void AdbVsockPipe::Service::destroyPipesThreadLoop() {
    while (true) {
        AdbVsockPipe *toDestroy;
        {
            fprintf(stderr, "%s: destroy pipes threa dloop begin\n", __func__);
            std::unique_lock<std::mutex> guard(mPipesToDestroyMtx);
            mPipesToDestroyCv.wait(guard, [this](){ return !mPipesToDestroy.empty(); });
            toDestroy = mPipesToDestroy.front();
            fprintf(stderr, "%s: destroying a pipe\n", __func__);
            mPipesToDestroy.pop_front();
            fprintf(stderr, "%s: destroyed a pipe\n", __func__);
        }
        if (!toDestroy) {
            break;
        }

        std::unique_ptr<AdbVsockPipe> deleter;  // ~AdbVsockPipe will not be called under mPipesMtx

        std::lock_guard<std::mutex> guard(mPipesMtx);
            fprintf(stderr, "%s: looking for pipes\n", __func__);
        const auto i = std::find_if(
            mPipes.begin(), mPipes.end(),
            [toDestroy](const std::unique_ptr<AdbVsockPipe> &pipe){
                return toDestroy == pipe.get();
            });
            fprintf(stderr, "%s: looking for pipes (done)\n", __func__);

        if (i != mPipes.end()) {
            deleter = std::move(*i);
            fprintf(stderr, "%s: destroying a pipe (2)\n", __func__);
            mPipes.erase(i);
            fprintf(stderr, "%s: destroying a pipe (2) (done)\n", __func__);
        }
    }
}

void AdbVsockPipe::Service::destroyPipe(AdbVsockPipe *p) {
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
    std::lock_guard<std::mutex> guard(mPipesToDestroyMtx);
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
    mPipesToDestroy.push_back(p);
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
    mPipesToDestroyCv.notify_one();
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
}

void AdbVsockPipe::Service::pollGuestAdbdThreadLoop() {
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
    struct AdbdAliveCallbacks : public IVsockHostCallbacks {
        ~AdbdAliveCallbacks() override {
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
            if (streamKey) {
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
                (*g_vsock_ops->close)(streamKey);
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
            }
        }

        void onConnect() override {
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
            isConnected.set_value();
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
        }

        void onReceive(const void *data, size_t size) override {}

        void onClose() override {
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
            streamKey = 0;
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
        }

        bool connect(const std::chrono::milliseconds timeout) {
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
            if (streamKey) {
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
                ::crashhandler_die("%s:%d: streamKey is not closed", __func__, __LINE__);
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
            }

    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
            isConnected = decltype(isConnected)();
            const uint64_t key = (*g_vsock_ops->open)(kGuestAdbdPort, this);
            if (key) {
                if (isConnected.get_future().wait_for(timeout) == std::future_status::ready) {
                    streamKey = key;
                    return true;
                } else {
                    (*g_vsock_ops->close)(key);
                    return false;
                }
            } else {
                return false;
            }
        }

        bool ping() {
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
            if (streamKey) {
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
                if ((*g_vsock_ops->ping)(streamKey)) {
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
                    return true;
                } else {
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
                    streamKey = 0;
                    return false;
                }
            } else {
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
                return false;
            }
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
        }

        std::atomic<uint64_t> streamKey = 0;
        std::promise<void> isConnected;
    };

    enum class State { NotAlive, Alive, Retrying };

    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
    AdbdAliveCallbacks adbdAliveCallbacks;
    State state = State::NotAlive;

    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
    while (mGuestAdbdPollingThreadRunning) {
        using namespace std::chrono_literals;

    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
        switch (state) {
        default:
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
            ::crashhandler_die("%s:%d: unexpected state", __func__, __LINE__);

        case State::NotAlive:
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
            if (adbdAliveCallbacks.connect(100ms)) {

    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
                mHostAgent->startListening();
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
                mHostAgent->notifyServer();
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
                state = State::Alive;
            }
            break;

        case State::Alive:
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
            if (adbdAliveCallbacks.ping()) {
                bool needNotifyAdbServer;

                {
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
                    std::lock_guard<std::mutex> guard(mPipesMtx);
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
                    needNotifyAdbServer = mPipes.empty();
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
                }

                if (needNotifyAdbServer) {
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
                    mHostAgent->notifyServer();
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
                }
            } else {
                state = State::Retrying;
            }
            break;

        case State::Retrying:
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
            if (adbdAliveCallbacks.connect(100ms)) {
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
                state = State::Alive;
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
            } else {
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
                mHostAgent->stopListening();
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
                state = State::NotAlive;
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
            }
            break;
        }

    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
        std::this_thread::sleep_for(500ms);
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
    }

    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
    if (state != State::NotAlive) {
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
        mHostAgent->stopListening();
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
    }
}

void AdbVsockPipe::Service::resetActiveGuestPipeConnection() {
    std::lock_guard<std::mutex> guard(mPipesMtx);
    for (const auto& pipe : mPipes) {
        fprintf(stderr, "%s: begin destroy pipe\n", __func__);
        // destroying a pipe requires the BQL which we can't aquire while holding mPipesMtx
        destroyPipe(pipe.get());
        fprintf(stderr, "%s: end destroy pipe\n", __func__);
    }
}

void AdbVsockPipe::Service::saveImpl(base::Stream* stream) const {
    std::lock_guard<std::mutex> guard(mPipesMtx);

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
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
    std::lock_guard<std::mutex> guard(mPipesMtx);
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);

    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
    mPipes.clear();
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
    for (uint32_t n = stream->getBe32(); n > 0; --n) {
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
        auto p = AdbVsockPipe::load(this, stream);
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
        if (p) {
            mPipes.push_back(std::move(p));
        }
    }
    return true;
}

IVsockHostCallbacks* AdbVsockPipe::Service::getHostCallbacksImpl(uint64_t key) const {
    std::lock_guard<std::mutex> guard(mPipesMtx);

    for (const auto& p : mPipes) {
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
        if (p->mVsockCallbacks.streamKey == key) {
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
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
    if (mSocketWatcher) {
        mSocketWatcher->dontWantWrite();
        mSocketWatcher->dontWantRead();
    }
}

std::unique_ptr<AdbVsockPipe> AdbVsockPipe::create(AdbVsockPipe::Service *service,
                                                   android::base::ScopedSocket socket,
                                                   AdbPortType portType) {
    android::base::socketSetNonBlocking(socket.get());
    android::base::socketSetNoDelay(socket.get());

    return std::make_unique<AdbVsockPipe>(service, std::move(socket), portType);
}

void AdbVsockPipe::onHostSocketEvent(int hostFd, unsigned events) {
    processProxyEventBits(mProxy->onHostSocketEvent(hostFd,
                                                    mVsockCallbacks.streamKey,
                                                    events));
}

void AdbVsockPipe::onGuestSend(const void *data, size_t size) {
    processProxyEventBits(mProxy->onGuestSend(data, size));
}

void AdbVsockPipe::onGuestClose() {
    processProxyEventBits(Proxy::EventBits::GuestClosed);
}

void AdbVsockPipe::processProxyEventBits(const Proxy::EventBits bits) {
    if (nonzero(bits & (Proxy::EventBits::HostClosed | Proxy::EventBits::GuestClosed))) {
        if (nonzero(bits & Proxy::EventBits::GuestClosed)) {
            fprintf(stderr, "%s: Call reset on mVsockCallbacks\n", __func__);
            mVsockCallbacks.reset();
            fprintf(stderr, "%s: Call reset on mVsockCallbacks (done)\n", __func__);
        }

    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
        if (nonzero(bits & Proxy::EventBits::HostClosed)) {
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
            mSocketWatcher->dontWantWrite();
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
            mSocketWatcher->dontWantRead();
        }
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);

        mService->destroyPipe(this);
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
    } else if (mSocketWatcher) {
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
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
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
    }
}

void AdbVsockPipe::setSocket(android::base::ScopedSocket socket) {
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
    mSocket = std::move(socket);

    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
    mSocketWatcher.reset(android::base::ThreadLooper::get()->createFdWatch(
        mSocket.get(),
        [](void* opaque, int fd, unsigned events) {
            static_cast<AdbVsockPipe*>(opaque)->onHostSocketEvent(fd, events);
        },
        this));

    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
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
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
        (*g_vsock_ops->close)(streamKey);
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
        reset();
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
    }
}

AdbVsockPipe::DataVsockCallbacks::~DataVsockCallbacks() {
    if (streamKey) {
        if (*g_vsock_ops->close) {
            (*g_vsock_ops->close)(streamKey);
        } else {
            fprintf(stderr, "%s: warning: vsock ops close is null\n", __func__);
        }
    }
}

}  // namespace emulation
}  // namespace android
