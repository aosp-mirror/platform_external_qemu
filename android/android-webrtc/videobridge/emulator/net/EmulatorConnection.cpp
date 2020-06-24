// Copyright (C) 2019 The Android Open Source Project
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
#include "emulator/net/EmulatorConnection.h"

#include <rtc_base/async_socket.h>  // for AsyncSocket
#include <rtc_base/logging.h>       // for RTC_LOG
#include <rtc_base/thread.h>        // for AutoSocketServerThread
#include <stdio.h>                  // for fprintf, stderr

#include <utility>  // for move

#ifndef _WIN32
#include <sys/socket.h>  // for AF_INET, SOCK_STREAM
#include <unistd.h>      // for fork, pid_t
#endif

#include "emulator/net/RtcAsyncSocketAdapter.h"  // for AsyncSocket, RtcAsyn...
#include "emulator/webrtc/Switchboard.h"         // for Switchboard
#include "rtc_base/physical_socket_server.h"     // for PhysicalSocketServer
#include "rtc_base/socket_address.h"             // for SocketAddress

namespace emulator {
namespace net {

EmulatorConnection::EmulatorConnection(int port,
                                       std::string handle,
                                       std::string turnConfig)
    : mPort(port), mHandle(handle), mTurnConfig(turnConfig) {}

EmulatorConnection::~EmulatorConnection() {}

#ifdef _WIN32
static bool initializeSockets() {
    WORD  wVersionRequested = MAKEWORD(2, 2);
    WSADATA wsaData;
    return WSAStartup(wVersionRequested, &wsaData);
}
#endif

bool EmulatorConnection::listen(bool should_fork) {
    RTC_LOG(INFO) << "Listening as "
                  << (should_fork ? " deamon (forked)"
                                  : " thread, not returning.");
#ifdef _WIN32
    initializeSockets();
#endif
    rtc::PhysicalSocketServer socket_server;

    // TODO(jansen): Use own thread that finalizes participants?
    rtc::AutoSocketServerThread thread(&socket_server);
    thread.SetName("Main Socket Server", &socket_server);
    auto socket = socket_server.CreateAsyncSocket(AF_INET, SOCK_STREAM);
    if (!socket) {
#ifdef _WIN32
        errno = WSAGetLastError();
#endif
        RTC_LOG(LERROR) << "Failed to create socket, errno: " << errno;
    }

    socket->SignalCloseEvent.connect(this, &EmulatorConnection::OnClose);
    socket->SignalConnectEvent.connect(this, &EmulatorConnection::OnConnect);
    socket->SignalReadEvent.connect(this, &EmulatorConnection::OnRead);

    rtc::SocketAddress address("127.0.0.1", mPort);
    if (socket->Bind(address) != 0) {
        RTC_LOG(LERROR) << "Failed to bind to " << mPort;
        return false;
    }

#ifndef _WIN32
    if (should_fork) {
        pid_t pid = ::fork();
        if (pid != 0) {
            RTC_LOG(INFO) << "Spawned a child under: " << pid;
            fprintf(stderr, "%d\n", pid);
            return true;
        }
    }
#endif

    if (socket->Listen(32) != 0) {
        RTC_LOG(LERROR) << "Failed to listen for incoming sockets.";
        return false;
    };
    RTC_LOG(INFO) << "Listening on: " << address.ToString();

    // Socket server is associated with this thread..
    thread.Run();

    RTC_LOG(INFO) << "Completed";
    return true;
}

void EmulatorConnection::disconnect(webrtc::Switchboard* disconnect) {
    RTC_LOG(INFO) << "Removing switchboard.";
}
void EmulatorConnection::OnRead(rtc::AsyncSocket* socket) {
    rtc::SocketAddress accept_addr;
    while (AsyncSocket* incoming = socket->Accept(&accept_addr)) {
        RtcAsyncSocketAdapter* adapter = new RtcAsyncSocketAdapter(incoming);
        std::unique_ptr<Switchboard> board(
                new webrtc::Switchboard(mHandle, mTurnConfig, adapter, this));
        mConnections.push_back(std::move(board));
        RTC_LOG(INFO) << "New switchboard registered for incoming emulator.";
    }
}
void EmulatorConnection::OnClose(rtc::AsyncSocket* socket, int err) {}
void EmulatorConnection::OnConnect(rtc::AsyncSocket* socket) {}
}  // namespace net
}  // namespace emulator
