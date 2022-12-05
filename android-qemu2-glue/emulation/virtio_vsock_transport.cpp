// Copyright 2021 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <mutex>
#include <vector>
#include <android-qemu2-glue/emulation/virtio_vsock_transport.h>
#include <host-common/crash-handler.h>
#include <android/emulation/SocketBuffer.h>
#include <android/emulation/CrossSessionSocket.h>
#include <android/utils/looper.h>
#include <android/utils/sockets.h>

size_t virtio_vsock_host_to_guest_send(uint64_t handle, const void *data, size_t size);
bool virtio_vsock_host_to_guest_close(uint64_t handle);
bool android_unix_pipe_check_path(const char* path);

namespace android {
namespace emulation {
namespace {

Looper* g_looper;

struct BrokenTransport : public IVsockNewTransport {
    int writeGuestToHost(const void *, size_t size, std::shared_ptr<IVsockNewTransport> *) override {
        return -1;
    }
};

#ifndef _WIN32
struct VsockUnixSocketTransport : public IVsockNewTransport {
    int writeGuestToHost(const void *data, size_t size,
                            std::shared_ptr<IVsockNewTransport> *) override {
        std::lock_guard<std::mutex> guard(mGuestToHostMutex);
        if (mClosed) {
            loopIo_dontWantWrite(mIO);
            return -1;
        }
        mGuestToHostBuf.append(data, size);

        loopIo_wantWrite(mIO);
        return size;
    }

    static std::shared_ptr<IVsockNewTransport> create(const uint64_t key, const char *path) {
        if (!android_unix_pipe_check_path(path)) {
            return nullptr;
        }

        int fd;

        SockAddress address;
        sock_address_init_unix(&address, path);
        fd = socket_create(sock_address_get_family(&address), SOCKET_STREAM);
        if (fd < 0) {
            sock_address_done(&address);
            return nullptr;
        }
        if (socket_connect(fd, &address) < 0) {
            sock_address_done(&address);
            socket_close(fd);
            return nullptr;
        }
        sock_address_done(&address);

        return std::make_shared<VsockUnixSocketTransport>(key, fd);
    }

    void ioFunc(const int fd, const unsigned events) {
        char buf[1024];

        if (events & LOOP_IO_READ) {
            const int n = socket_recv(fd, buf, sizeof(buf));

            if (n > 0) {
                virtio_vsock_host_to_guest_send(mKey, buf, n);
            } else if (n == 0) {
                loopIo_dontWantRead(mIO);
                loopIo_dontWantWrite(mIO);
//                virtio_vsock_host_to_guest_close(mKey);  // loopIo_free(mIO) will be called from this thread
                std::lock_guard<std::mutex> guard(mGuestToHostMutex);
                mClosed = true;
                return;
            }
        }

        if (events & LOOP_IO_WRITE) {
            size_t size;
            {
                std::lock_guard<std::mutex> guard(mGuestToHostMutex);
                const auto chunk = mGuestToHostBuf.peek();
                size = std::min(sizeof(buf), chunk.second);
                memcpy(buf, chunk.first, size);
            }

            if (size > 0) {
                const int sent = socket_send(fd, buf, size);
                if (sent > 0) {
                    std::lock_guard<std::mutex> guard(mGuestToHostMutex);
                    mGuestToHostBuf.consume(sent);
                }
            } else {
                std::lock_guard<std::mutex> guard(mGuestToHostMutex);
                if (mGuestToHostBuf.peek().second == 0) {
                    loopIo_dontWantWrite(mIO);
                }
            }
        }
    }

    static void ioFuncStatic(void* thatRaw, int fd, unsigned events) {
        const auto that = static_cast<VsockUnixSocketTransport *>(thatRaw);
        const auto pin = that->shared_from_this();
        if (pin.use_count() == 1) {
            ::crashhandler_die("%s:%s:%d the transport is already closed on the device side",
                               "VsockUnixSocketTransport", __func__, __LINE__);
        }
        that->ioFunc(fd, events);
    }

    VsockUnixSocketTransport(const uint64_t key, const int fd)
        : mKey(key)
        , mS(fd)
        , mIO(loopIo_new(g_looper, fd, &ioFuncStatic, this)) {
        loopIo_wantRead(mIO);
    }

    ~VsockUnixSocketTransport() {
        loopIo_dontWantRead(mIO);
        loopIo_dontWantWrite(mIO);
        loopIo_free(mIO);
    }

    const uint64_t mKey;
    android::emulation::CrossSessionSocket mS;
    LoopIo* mIO = nullptr;
    SocketBuffer mGuestToHostBuf;
    bool mClosed = false;
    mutable std::mutex mGuestToHostMutex;
};
#endif

struct NewTransportConnector : public IVsockNewTransport {
    NewTransportConnector(uint64_t key) : mKey(key) {}

    int writeGuestToHost(const void *data, size_t size,
                            std::shared_ptr<IVsockNewTransport> *pNextStage) override {
        size_t n = 0;
        bool complete = false;
        const char *i = static_cast<const char *>(data);
        while (size > 0) {
            const char c = *i;
            mBuf.push_back(c);
            ++i;
            ++n;
            --size;
            if (c == 0) {
                complete = true;
                break;
            }
        }

        if (complete) {
            auto nextStage = createNextStage(mBuf.data());
            if (nextStage) {
                *pNextStage = std::move(nextStage);
                return n;
            } else {
                *pNextStage = std::make_shared<BrokenTransport>();
                return -1;
            }
        } else {
            return n;
        }
    }

    std::shared_ptr<IVsockNewTransport> createNextStage(char *str) const {
        if (strncmp(str, "pipe:", 5)) {
            return nullptr;
        }
        str += 5;
        char *i = strchr(str, ':');
        if (i) {
            *i = 0;
            return createNextStageImpl(str, i + 1);
        } else {
            return createNextStageImpl(str, nullptr);
        }
    }

    std::shared_ptr<IVsockNewTransport> createNextStageImpl(const char *service,
                                                            const char *arg) const {
#ifndef _WIN32
        if (!strcmp(service, "unix")) {
            return VsockUnixSocketTransport::create(mKey, arg);
        }
#endif

        return nullptr;
    }

    const uint64_t mKey;
    std::vector<char> mBuf;
};
}  // namespace

std::shared_ptr<IVsockNewTransport> vsock_create_transport_connector(uint64_t key) {
    return std::make_shared<NewTransportConnector>(key);
}

std::shared_ptr<IVsockNewTransport> vsock_load_transport(android::base::Stream *stream) {
    return nullptr;
}

void virtio_vsock_new_transport_init() {
    g_looper = looper_getForThread();
}

}  // namespace emulation
}  // namespace android
