
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
#include "net/sockets/loopback_async_socket_connector.h"

#include "android/base/sockets/SocketUtils.h"  // for socketTcp4LoopbackClient
#include "net/sockets/async_socket.h"          // for AsyncSocket

namespace android {
namespace net {

LoopbackAsyncSocketConnector::LoopbackAsyncSocketConnector(AsyncManager* am)
    : am_(am) {}

std::shared_ptr<AsyncDataChannel>
LoopbackAsyncSocketConnector::ConnectToRemoteServer(
        const std::string& server,
        int port,
        const std::chrono::milliseconds timeout) {
    int socket_fd = android::base::socketTcp4LoopbackClient(port);
    if (socket_fd < 0) {
        socket_fd = android::base::socketTcp6LoopbackClient(port);
    }
    return std::make_shared<AsyncSocket>(socket_fd, am_);
}

}  // namespace net
}  // namespace android
