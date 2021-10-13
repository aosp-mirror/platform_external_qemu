
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
#include <chrono>  // for operator""ms, chrono_literals
#include <memory>  // for shared_ptr
#include <string>  // for string

#include "net/async_data_channel_connector.h"  // for AsyncDataChannelConnector

namespace android {
namespace net {
class AsyncDataChannel;
}  // namespace net
}  // namespace android

namespace test_vendor_lib {
class AsyncManager;
}  // namespace test_vendor_lib

using test_vendor_lib::AsyncManager;
namespace android {
namespace net {

using namespace std::chrono_literals;

// A Posix compliant implementation of the AsyncDataChannelConnector interface.
//
// Supports both Darwin (freebsd) and linux.
class LoopbackAsyncSocketConnector : public AsyncDataChannelConnector {
 public:
  LoopbackAsyncSocketConnector(AsyncManager* am);
  ~LoopbackAsyncSocketConnector() = default;

  // Blocks and waits until a connection to the remote server has been
  // established. Returns null in case of failure. In case of null the errno
  // variable will be set with the encountered error.
  //
  // Note: This does not mean that the socket is fully opened! A server
  // might not (yet?) have called accept on the socket.
  std::shared_ptr<AsyncDataChannel> ConnectToRemoteServer(
      const std::string& server, int port,
      const std::chrono::milliseconds timeout = 5000ms);

 private:
  AsyncManager* am_;
};
}  // namespace net
}  // namespace android