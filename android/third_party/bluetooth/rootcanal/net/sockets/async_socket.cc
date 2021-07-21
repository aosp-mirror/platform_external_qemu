// Copyright (C) 2021 The Android Open Source Project
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
#include "net/sockets/async_socket.h"

#include <functional>                          // for __base

#include "android/base/sockets/SocketUtils.h"  // for socketClose, socketRecv
#include "model/setup/async_manager.h"         // for AsyncManager


/* set  for very verbose debugging */
#ifndef DEBUG
#define DD(...) (void)0
#define DD_BUF(fd, buf, len) (void)0
#else
#define DD(...) DD(__VA_ARGS__)
#define DD_BUF(fd, buf, len)                            \
    do {                                                \
        printf("PosixSocket %s (%d):", __func__, fd);   \
        for (int x = 0; x < len; x++) {                 \
            if (isprint((int)buf[x]))                   \
                printf("%c", buf[x]);                   \
            else                                        \
                printf("[0x%02x]", 0xff & (int)buf[x]); \
        }                                               \
        printf("\n");                                   \
    } while (0)

#endif

namespace android {
namespace net {

AsyncSocket::AsyncSocket(int fd, AsyncManager* am)
    : fd_(fd), am_(am), watching_(false) {
    base::socketSetNonBlocking(fd);
}

AsyncSocket::AsyncSocket(AsyncSocket&& other) {
    fd_ = other.fd_;
    watching_ = other.watching_.load();
    am_ = other.am_;

    other.fd_ = -1;
    other.watching_ = false;
}
AsyncSocket::~AsyncSocket() {
    Close();
}

ssize_t AsyncSocket::Recv(uint8_t* buffer, uint64_t bufferSize) {
    ssize_t res = base::socketRecv(fd_, buffer, bufferSize);
    if (res < 0) {
        DD("Recv < 0: %s (%d)", strerror(errno), fd_);
    }

    DD_BUF(fd_, buffer, res);
    DD("%zd bytes (%d)", res, fd_);
    return res;
};

ssize_t AsyncSocket::Send(const uint8_t* buffer, uint64_t bufferSize) {
    ssize_t res = base::socketSend(fd_, buffer, bufferSize);

    DD_BUF(fd_, buffer, res);
    DD("%zd bytes (%d)", res, fd_);
    return res;
}

bool AsyncSocket::Connected() {
    if (fd_ == -1) {
        return false;
    }

    // We saw a byte in the queue, we are likely connected.
    return true;
}

void AsyncSocket::Close() {
    if (fd_ == -1) {
        return;
    }

    StopWatching();

    base::socketClose(fd_);
    DD("(%d)", fd_);
    fd_ = -1;
}

bool AsyncSocket::WatchForNonBlockingRead(
        const ReadCallback& on_read_ready_callback) {
    bool expected = false;
    if (watching_.compare_exchange_strong(expected, true)) {
        return am_->WatchFdForNonBlockingReads(
                       fd_, [on_read_ready_callback, this](int fd) {
                           on_read_ready_callback(this);
                       }) == 0;
    }
    return false;
}

void AsyncSocket::StopWatching() {
    bool expected = true;
    if (watching_.compare_exchange_strong(expected, false)) {
        am_->StopWatchingFileDescriptor(fd_);
    }
}
}  // namespace net
}  // namespace android
