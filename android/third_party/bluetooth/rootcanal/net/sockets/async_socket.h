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
#pragma once
#include <stdint.h>     // for uint64_t, uint8_t
#include <sys/types.h>  // for ssize_t

#include <atomic>  // for atomic_bool

#include "net/async_data_channel.h"  // for AsyncDataChannel, ReadCallback

namespace test_vendor_lib {
class AsyncManager;
}  // namespace test_vendor_lib

namespace android {
namespace net {

using test_vendor_lib::AsyncManager;

// A socket based implementation of the AsyncDataChannel interface.
//
// The sockets are usually only locally available.
class AsyncSocket : public AsyncDataChannel {
 public:
  // The AsyncManager must support the following:
  //
  // - If a callback happens on thread t, and
  //   am->StopWatchingFileDescriptor(fd)
  //   is called from t then am will never fire an event for fd upon return.
  AsyncSocket(int fd, AsyncManager* am);
  AsyncSocket(const AsyncSocket& other) = delete;
  AsyncSocket(AsyncSocket&& other);

  // Make sure to close the socket before hand.
  ~AsyncSocket();

  // Receive data in the given buffer. Returns the number of bytes read,
  // or a negative number in case of failure. Check the errno variable to
  // learn why the call failed.
  ssize_t Recv(uint8_t* buffer, uint64_t bufferSize) override;

  // Send data in the given buffer. Returns the number of bytes read,
  // or a negative number in case of failure. Check the errno variable to
  // learn why the call failed. Note: This can be EAGAIN if we cannot
  // write to the socket at this time as it would block.
  ssize_t Send(const uint8_t* buffer, uint64_t bufferSize) override;

  // True if this socket is connected
  bool Connected() override;

  // Closes this socket. You must call this before deleting the socket.
  //
  // - On Linux, the socket is always closed when this function returns
  // - On OS X, whether the socket the underlying fd becomes available seems
  //   pretty much random! (You might want to drain the socket first.)
  void Close() override;

  // Registers the given callback to be invoked when a recv call can be made
  // to read data from this socket.
  // Only one callback can be registered per socket.
  bool WatchForNonBlockingRead(
      const ReadCallback& on_read_ready_callback) override;

  void StopWatching() override;

  int fd() { return fd_; }

 private:
  void OnReadCallback();

  int fd_;
  AsyncManager* am_;
  std::atomic_bool watching_;
};
}  // namespace net
}  // namespace android
