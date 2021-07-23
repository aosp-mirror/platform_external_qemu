
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
#include "net/sockets/loopback_async_socket_server.h"

#include <functional>                          // for __base, function
#include <type_traits>                         // for remove_extent_t

#include "android/base/sockets/SocketUtils.h"  // for socketAcceptAny, socke...
#include "net/sockets/async_socket.h"          // for AsyncSocket, AsyncManager


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
class AsyncDataChannel;

LoopbackAsyncSocketServer::LoopbackAsyncSocketServer(int port, AsyncManager* am)
    : port_(port), am_(am) {
  int listen_fd = base::socketTcp4LoopbackServer(port);
  if (listen_fd < 0) {
    listen_fd = base::socketTcp6LoopbackServer(port);
  }
  server_socket_ = std::make_shared<AsyncSocket>(listen_fd, am_);
}

bool LoopbackAsyncSocketServer::StartListening() {
  if (!server_socket_ || !callback_) {
    return false;
  }

  server_socket_->WatchForNonBlockingRead(
      [this](AsyncDataChannel* socket) { AcceptSocket(); });
  return true;
}

void LoopbackAsyncSocketServer::Close() {
  if (server_socket_) {
    server_socket_->Close();
  }
}

bool LoopbackAsyncSocketServer::Connected() {
  return server_socket_ && server_socket_->Connected();
}

void LoopbackAsyncSocketServer::AcceptSocket() {
  int accept_fd = base::socketAcceptAny(server_socket_->fd());

  if (accept_fd < 0) {
    DD("Error accepting test channel connection errno=%d (%s).", errno,
             strerror(errno));
    return;
  }

  DD("accept_fd = %d.", accept_fd);
  StopListening();
  callback_(std::make_shared<AsyncSocket>(accept_fd, am_), this);
}

void LoopbackAsyncSocketServer::StopListening() { server_socket_->StopWatching(); }
}  // namespace net
}  // namespace android