/*
 * Copyright (C) 2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "shared_fd.h"
#include "shared_select.h"

#include "android/base/sockets/SocketUtils.h"

#ifdef _WIN32
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#else
#include <errno.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#ifndef TEMP_FAILURE_RETRY
#define TEMP_FAILURE_RETRY(expression)         \
    ({                                         \
        ssize_t __res;                         \
        do                                     \
            __res = expression;                \
        while (__res == -1 && errno == EINTR); \
        __res;                                 \
    })
#endif

namespace cuttlefish {

void FileInstance::Close() {
    if (fd_ == -1) {
        errno_ = EBADF;
    } else {
        android::base::socketClose(fd_);
    }
    fd_ = -1;
}

int FileInstance::Read(void* buf, size_t count) {
    errno = 0;
    ssize_t rval = android::base::socketRecv(fd_, buf, count);
    errno_ = errno;
    return rval;
}

int FileInstance::Write(const void* buf, size_t count) {
    errno = 0;
    ssize_t rval = android::base::socketSend(fd_, buf, count);
    errno_ = errno;
    return rval;
}

FileInstance::FileInstance(int fd, int in_errno) : fd_(fd), errno_(in_errno) {}

bool FileInstance::IsSet(fd_set* in) const {
    if (IsOpen() && FD_ISSET(fd_, in)) {
        return true;
    }
    return false;
}

using cuttlefish::SharedFDSet;

void MarkAll(const SharedFDSet& input, fd_set* dest, int* max_index) {
    for (SharedFDSet::const_iterator it = input.begin(); it != input.end();
         ++it) {
        (*it)->Set(dest, max_index);
    }
}

void CheckMarked(fd_set* in_out_mask, SharedFDSet* in_out_set) {
    if (!in_out_set) {
        return;
    }
    SharedFDSet save;
    save.swap(in_out_set);
    for (SharedFDSet::iterator it = save.begin(); it != save.end(); ++it) {
        if ((*it)->IsSet(in_out_mask)) {
            in_out_set->Set(*it);
        }
    }
}

int ReadExact(SharedFD fd, std::string* buffer) {
    return fd.ReadExact(*buffer);
}

int SharedFD::ReadExact(std::string& buffer) {
    bool success = android::base::socketRecvAll(value_->fd_, buffer.data(),
                                                buffer.size());
    if (success) {
        return buffer.size();
    } else {
        return 0;
    }
}

int WriteAll(SharedFD fd, const std::string& buffer) {
    return fd.WriteAll(buffer);
}

int SharedFD::WriteAll(const std::string& buffer) {
    bool success = android::base::socketSendAll(value_->fd_, buffer.data(),
                                                buffer.size());
    if (success) {
        return buffer.size();
    } else {
        return 0;
    }
}

int Select(SharedFDSet* read_set,
           SharedFDSet* write_set,
           SharedFDSet* error_set,
           struct timeval* timeout) {
    int max_index = 0;
    fd_set readfds;
    FD_ZERO(&readfds);
    if (read_set) {
        MarkAll(*read_set, &readfds, &max_index);
    }
    fd_set writefds;
    FD_ZERO(&writefds);
    if (write_set) {
        MarkAll(*write_set, &writefds, &max_index);
    }
    fd_set errorfds;
    FD_ZERO(&errorfds);
    if (error_set) {
        MarkAll(*error_set, &errorfds, &max_index);
    }

    int rval = TEMP_FAILURE_RETRY(
            ::select(max_index, &readfds, &writefds, &errorfds, timeout));
    CheckMarked(&readfds, read_set);
    CheckMarked(&writefds, write_set);
    CheckMarked(&errorfds, error_set);
    return rval;
}

FileInstance* FileInstance::Accept(struct sockaddr* addr,
                                   socklen_t* addrlen) const {
    int fd = android::base::socketAcceptAny(fd_);
    android::base::socketSetNoDelay(fd);
    if (fd == -1) {
        return new FileInstance(fd, errno);
    } else {
        return new FileInstance(fd, 0);
    }
}

SharedFD SharedFD::Accept(const FileInstance& listener,
                          struct sockaddr* addr,
                          socklen_t* addrlen) {
    return SharedFD(
            std::shared_ptr<FileInstance>(listener.Accept(addr, addrlen)));
}

SharedFD SharedFD::Accept(const FileInstance& listener) {
    return SharedFD::Accept(listener, NULL, NULL);
}

void FileInstance::Set(fd_set* dest, int* max_index) const {
    if (!IsOpen()) {
        return;
    }
    if (fd_ >= *max_index) {
        *max_index = fd_ + 1;
    }
    FD_SET(fd_, dest);
}

bool SharedFD::Pipe(SharedFD* fd0, SharedFD* fd1) {
    int fds[2];
    if (android::base::socketCreatePair(&fds[0], &fds[1])) {
        return false;
    }
    android::base::socketSetNoDelay(fds[0]);
    android::base::socketSetNoDelay(fds[1]);
    (*fd0) = std::shared_ptr<FileInstance>(new FileInstance(fds[0], 0));
    (*fd1) = std::shared_ptr<FileInstance>(new FileInstance(fds[1], 0));
    return true;
}

SharedFD SharedFD::SocketLocalClient(int port) {
    int fd = android::base::socketTcp6LoopbackClient(port);
    if (fd == -1) {
        fd = android::base::socketTcp4LoopbackClient(port);
    }
    if (fd == -1) {
        return SharedFD();
    }
    android::base::socketSetNoDelay(fd);
    return SharedFD(std::shared_ptr<FileInstance>(new FileInstance(fd, 0)));
}

SharedFD SharedFD::SocketLocalClient(const std::string& name,
                                     bool abstract,
                                     int in_type) {
    // the modem simulator tries to send sms to something like "1-650-555-6789";
    // but since it is just a made up phone number, the simulator just send
    // to "modem_simulator6789" unix pipe; but this is a bit tricky, as on
    // windows, the unix pipe does not work
    // so we assume the last 4 digit to be port number
    const std::string prefix = "modem_simulator";
    if (name.size() != prefix.size() + 4 || name.find(prefix) != 0 ||
        in_type != SOCK_STREAM || !abstract) {
        return SharedFD();
    }
    int port = std::stoi(name.substr(prefix.size()));
    int fd = android::base::socketTcp6LoopbackClient(port);
    if (fd == -1) {
        fd = android::base::socketTcp4LoopbackClient(port);
    }
    if (fd == -1) {
        return SharedFD();
    }
    android::base::socketSetNoDelay(fd);
    return SharedFD(std::shared_ptr<FileInstance>(new FileInstance(fd, 0)));
}

SharedFD SharedFD::SocketLocalServer(int port) {
    int fd = android::base::socketTcp6LoopbackServer(port);
    if (fd == -1) {
        fd = android::base::socketTcp4LoopbackServer(port);
    }
    if (fd == -1) {
        return SharedFD();
    }
    android::base::socketSetNoDelay(fd);
    return SharedFD(std::shared_ptr<FileInstance>(new FileInstance(fd, 0)));
}

std::shared_ptr<FileInstance> FileInstance::ClosedInstance() {
    return std::shared_ptr<FileInstance>(new FileInstance(-1, EBADF));
}

SharedFD::SharedFD() : value_(FileInstance::ClosedInstance()) {}
SharedFD::SharedFD(const std::shared_ptr<FileInstance>& in) : value_(in) {}

}  // namespace cuttlefish
