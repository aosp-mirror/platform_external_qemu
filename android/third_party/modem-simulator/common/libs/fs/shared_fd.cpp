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

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

#else
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
#include <errno.h>
#endif

#ifndef TEMP_FAILURE_RETRY
#define TEMP_FAILURE_RETRY(expression) \
  ({ ssize_t __res; \
     do \
       __res = expression; \
     while (__res == -1 && errno == EINTR); \
     __res; })
#endif

namespace cvd {

void FileInstance::Close() {
  if (fd_ == -1) {
    errno_ = EBADF;
  } else {
#ifdef _WIN32
      int ret =::closesocket(fd_);
#else
      int ret =::close(fd_);
#endif
      if (ret == -1) errno_ = errno;
  }
  fd_ = -1;
}

int FileInstance::Read(void* buf, size_t count) {
    errno = 0;
    ssize_t rval = TEMP_FAILURE_RETRY(::recv(fd_, (char*)buf, count,0));
    errno_ = errno;
    return rval;
}


int FileInstance::Write(const void* buf, size_t count) {
    errno = 0;
    ssize_t rval = TEMP_FAILURE_RETRY(::send(fd_, (const char*)buf, count,0));
    errno_ = errno;
    return rval;
}

bool FileInstance::IsSet(fd_set* in) const {
  if (IsOpen() && FD_ISSET(fd_, in)) {
    return true;
  }
  return false;
}


using cvd::SharedFDSet;

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

int Select(SharedFDSet* read_set, SharedFDSet* write_set,
           SharedFDSet* error_set, struct timeval* timeout) {
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
      select(max_index, &readfds, &writefds, &errorfds, timeout));
  CheckMarked(&readfds, read_set);
  CheckMarked(&writefds, write_set);
  CheckMarked(&errorfds, error_set);
  return rval;
}

FileInstance* FileInstance::Accept(struct sockaddr* addr, socklen_t* addrlen) const {
    int fd = TEMP_FAILURE_RETRY(accept(fd_, addr, addrlen));
    if (fd == -1) {
      return new FileInstance(fd, errno);
    } else {
      return new FileInstance(fd, 0);
    }
  }


SharedFD SharedFD::Accept(const FileInstance& listener, struct sockaddr* addr,
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

int FileInstance::Connect(const struct sockaddr* addr, socklen_t addrlen) {
    errno = 0;
    int rval = connect(fd_, addr, addrlen);
    errno_ = errno;
    return rval;
  }


bool SharedFD::Pipe(SharedFD* fd0, SharedFD* fd1) {
  int fds[2];
#ifdef _WIN32
  //TODO
  int rval = -1;
#else
  int rval = socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
#endif
  if (rval != -1) {
    (*fd0) = std::shared_ptr<FileInstance>(new FileInstance(fds[0], errno));
    (*fd1) = std::shared_ptr<FileInstance>(new FileInstance(fds[1], errno));
    return true;
  }

  return false;
}

SharedFD SharedFD::SocketLocalClient(const std::string& name, bool abstract,
                                     int in_type) {
    // TODO
    return SharedFD();
}

std::shared_ptr<FileInstance> FileInstance::ClosedInstance() {
    return std::shared_ptr<FileInstance>(new FileInstance(-1, EBADF));
}



SharedFD::SharedFD() : value_(FileInstance::ClosedInstance()) {}


}  // namespace cvd
