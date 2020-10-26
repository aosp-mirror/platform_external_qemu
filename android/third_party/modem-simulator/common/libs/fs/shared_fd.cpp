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
#include "android-base/logging.h"

#ifdef _WIN32
#include "android/base/sockets/Winsock.h"
#else
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include "glib.h"

#include <map>
#include <vector>

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

namespace {
int UnixSelect(SharedFDSet* read_set,
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

#ifndef _WIN32
int UnixPollFDs(SharedFDSet& read_set, struct timeval* timeout) {
    std::vector<pollfd> myfds;
    std::map<int, SharedFD> myreadmap;
    for (const auto& iter : read_set) {
        myfds.emplace_back(pollfd{iter->getFd(), POLLIN, 0});
        myreadmap[iter->getFd()] = iter;
    }

    int mytimeout = -1;
    if (timeout) {
        mytimeout = timeout->tv_sec * 1000 + timeout->tv_usec / 1000;
    }
    read_set.Zero();
    int ready = ::poll(myfds.data(), myfds.size(), mytimeout);
    if (ready > 0) {
        for (const auto& iter : myfds) {
            switch (iter.revents) {
                case POLLIN:
                    read_set.Set(myreadmap[iter.fd]);
                    break;
                default:
                    // something wrong
                    break;
            }
        }
    }
    return ready;
}
#endif

#ifdef _WIN32
int WindowsPollFDs(SharedFDSet& read_set,
           struct timeval* timeout) {
    std::map<int, SharedFD> myreadmap;
    std::map<HANDLE, int> myhandlemap;
    std::vector<HANDLE> myevents;
    for (const auto& iter : read_set) {
        int fd = iter->getFd();
        HANDLE event = CreateEvent(NULL, TRUE, FALSE, NULL);
        WSAEventSelect(fd, event, FD_READ | FD_ACCEPT | FD_CLOSE | FD_CONNECT);
        myevents.push_back(event);
        myhandlemap[event] = fd;
        myreadmap[fd] = iter;
    }

    int mytimeout = INFINITE;
    if (timeout) {
        mytimeout = timeout->tv_sec * 1000 + timeout->tv_usec / 1000;
    }

    int ret = WaitForMultipleObjects(myevents.size(), myevents.data(), FALSE,
                                     mytimeout);

    int numFdReady = 0;
    read_set.Zero();
    if (ret >= WAIT_OBJECT_0 && ret < WAIT_OBJECT_0 + myevents.size()) {
        // setup the fd sets
        int index = ret - WAIT_OBJECT_0;
        int fd = myhandlemap[myevents[index]];
        read_set.Set(myreadmap[fd]);
        ++numFdReady;
        // check for other fd
        LOG(DEBUG)<<"fd fired " << fd;
        for (int i = index + 1; i < myevents.size(); ++ i) {
            ret = WaitForMultipleObjects(1, &(myevents[i]), TRUE, 0);
            if (ret == WAIT_OBJECT_0) {
                fd = myhandlemap[myevents[i]];
                read_set.Set(myreadmap[fd]);
                ++numFdReady;
                LOG(DEBUG)<<"additional fd fired " << fd << " total fired fd " << numFdReady;
            }
        }
    }

    for (auto& iter : myevents) {
        int fd = myhandlemap[iter];
        WSAEventSelect(fd, 0, 0);
        CloseHandle(iter);
    }

    return numFdReady;
}
#endif
}  // anonymous namespace

int Select(SharedFDSet* read_set,
           SharedFDSet* write_set,
           SharedFDSet* error_set,
           struct timeval* timeout) {
    if (write_set || error_set) {
        // just ignore them
        fprintf(stdout, "WARNING: %s %d ignore write and error\n", __func__,
                __LINE__);
    }

    if (!read_set) {
        return -1;
    }

#ifdef _WIN32
    return WindowsPollFDs(*read_set, timeout);
    //return UnixSelect(read_set, nullptr, nullptr, timeout);
#else
    return UnixPollFDs(*read_set, timeout);
#endif
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
