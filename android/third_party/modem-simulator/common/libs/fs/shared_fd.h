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

#pragma once

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
#endif

#include <memory>
#include <string>

// goldfish implementation of the sharefd functions, the bare minimum
// to get it work with modem simulator, on linux/osx/win

namespace cvd {

class FileInstance;

class SharedFD {

public:

  SharedFD();

  SharedFD(const std::shared_ptr<FileInstance>& in);

    static SharedFD Accept(const FileInstance& listener, struct sockaddr* addr,
                          socklen_t* addrlen);

  static SharedFD Accept(const FileInstance& listener);

  static bool Pipe(SharedFD* fd0, SharedFD* fd1);

  static SharedFD SocketLocalClient(const std::string& name, bool is_abstract,
                                    int in_type);

  bool operator==(const SharedFD& rhs) const { return value_ == rhs.value_; }

  bool operator!=(const SharedFD& rhs) const { return value_ != rhs.value_; }

  bool operator<(const SharedFD& rhs) const { return value_ < rhs.value_; }

  bool operator<=(const SharedFD& rhs) const { return value_ <= rhs.value_; }

  bool operator>(const SharedFD& rhs) const { return value_ > rhs.value_; }

  bool operator>=(const SharedFD& rhs) const { return value_ >= rhs.value_; }

  std::shared_ptr<FileInstance> operator->() const { return value_; }

  const cvd::FileInstance& operator*() const { return *value_; }

  cvd::FileInstance& operator*() { return *value_; }

private:

  std::shared_ptr<FileInstance> value_;
};

class FileInstance {
  // Give SharedFD access to the aliasing constructor.
  friend class SharedFD;

 public:

  void Close();
 virtual ~FileInstance() { Close(); }

  bool IsOpen() const { return fd_ != -1; }

  bool IsSet(fd_set* in) const;

  void Set(fd_set* dest, int* max_index) const;

  static std::shared_ptr<FileInstance> ClosedInstance();

  std::string StrError() {return "error"; }

  int Connect(const struct sockaddr* addr, socklen_t addrlen);

  int Write(const void* buf, size_t count);

  int Read(void* buf, size_t count);

FileInstance* Accept(struct sockaddr* addr, socklen_t* addrlen) const;

 private:
  FileInstance(int fd, int in_errno);

  int fd_;
  int errno_;
  std::string identity_;
  char strerror_buf_[160];
};

}  // namespace cvd
