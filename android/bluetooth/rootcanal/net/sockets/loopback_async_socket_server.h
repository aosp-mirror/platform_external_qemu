
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

#include <memory>  // for shared_ptr

#include "net/async_data_channel_server.h"  // for AsyncDataChannelServer
#include "net/sockets/async_socket.h"   // for AsyncManager, PosixAsyncS...

namespace test_vendor_lib {
class AsyncManager;
}  // namespace test_vendor_lib

using test_vendor_lib::AsyncManager;

namespace android {
namespace net {

// An AsyncDataChannelServer that binds to all interfaces on the given port and
// listens for incoming socket connections.
//
// It uses the AsyncManager for watching the socket.
class LoopbackAsyncSocketServer : public AsyncDataChannelServer {
 public:
  // Binds to the given port on all interfaces.
  // Note: do not use port 0!
  LoopbackAsyncSocketServer(int port, AsyncManager* am);

  // Return the port that this server was initialized with.
  int port() const { return port_; };

  bool StartListening() override;

  void StopListening() override;

  void Close() override;

  bool Connected() override;

 private:
  void AcceptSocket();

  int port_;
  std::shared_ptr<AsyncSocket> server_socket_;
  AsyncManager* am_;
};

}  // namespace net
}  // namespace android
