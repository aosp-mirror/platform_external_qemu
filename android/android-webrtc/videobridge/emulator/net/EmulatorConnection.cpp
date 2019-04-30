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

#include "emulator/net/RtcAsyncSocketAdapter.h"
#include "rtc_base/socketaddress.h"

namespace emulator {
namespace net {

EmulatorConnection::EmulatorConnection(int port, std::string handle)
    : mPort(port), mHandle(handle) {}

EmulatorConnection::~EmulatorConnection() {}

bool EmulatorConnection::listen_and_fork() {
    if (!listen()) {
        return false;
    }
    pid_t pid = fork();
    if (pid == 0) {
        //I'm the child..
        return run();
    }
    // I'm the parent.
    mSocket->Close();
    fprintf(stderr, "%d\n", pid);
    return true;
}

bool EmulatorConnection::listen() {
    mThread.reset(new rtc::AutoSocketServerThread(&mSocketServer));
    mThread->SetName("Main Socket Server", &mSocketServer);
    mSocket.reset(mSocketServer.CreateAsyncSocket(AF_INET, SOCK_STREAM));

    mSocket->SignalCloseEvent.connect(this, &EmulatorConnection::OnClose);
    mSocket->SignalConnectEvent.connect(this, &EmulatorConnection::OnConnect);
    mSocket->SignalReadEvent.connect(this, &EmulatorConnection::OnRead);

    rtc::SocketAddress address("127.0.0.1", mPort);
    if (mSocket->Bind(address) != 0) {
        RTC_LOG(LERROR) << "Failed to bind to " << mPort;
        return false;
    }
    rtc::SocketAddress accept_addr;
    if (mSocket->Listen(32) != 0) {
        RTC_LOG(LERROR) << "Failed to listen for incoming sockets.";
        return false;
    };
    RTC_LOG(INFO) << "Listening on: " << address.ToString();

    // Socket server is associated with this thread..
    return true;
}

bool EmulatorConnection::run() {
    mThread->Run();
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
                new webrtc::Switchboard(mHandle, adapter, this));
        mConnections.push_back(std::move(board));
        RTC_LOG(INFO) << "New switchboard registered for incoming emulator.";
    }
}
void EmulatorConnection::OnClose(rtc::AsyncSocket* socket, int err) {}
void EmulatorConnection::OnConnect(rtc::AsyncSocket* socket) {}
}  // namespace net
}  // namespace emulator