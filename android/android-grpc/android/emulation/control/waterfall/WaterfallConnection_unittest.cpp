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

#include "android/emulation/control/waterfall/WaterfallConnection.h"

#include <gtest/gtest.h>

#include <iostream>
#include <istream>
#include <sstream>
#include <string>
#include <thread>

#include "android/base/Log.h"
#include "android/base/async/DefaultLooper.h"
#include "android/base/async/ThreadLooper.h"
#include "android/base/sockets/ScopedSocket.h"
#include "android/base/sockets/SocketUtils.h"
#include "android/emulation/control/waterfall/SocketStreambuf.h"
#include "android/utils/sockets.h"


namespace android {
namespace emulation {
namespace control {

using namespace base;

ScopedSocket getServerSocket() {
    // Find a free TCP IPv4 port and bind to it.
    ScopedSocket serverSocket;
    int port;
    for (port = 1024; port < 65536; ++port) {
        serverSocket.reset(socketTcp4LoopbackServer(port));
        if (serverSocket.valid()) {
            break;
        }
    }
    EXPECT_TRUE(serverSocket.valid()) << "Could not find free TCP port";
    EXPECT_EQ(port, socketGetPort(serverSocket.get()));

    return serverSocket;
}

class WaterfallConnectionTest : public ::testing::Test {
public:
    WaterfallConnectionTest() { mLooper = android::base::ThreadLooper::get(); }

    void SetUp() override {
        mServerSocket = getServerSocket();
        mPort = socketGetPort(mServerSocket.get());
        int fd = mServerSocket.get();
        mConnection.reset(getTestWaterfallConnection([fd]() { return fd; }));
        ASSERT_TRUE(mConnection->initialize());
        mWaterfallSocket = socketTcp4LoopbackClient(mPort);
        ASSERT_TRUE(mWaterfallSocket.valid())
                << "cannot connect to " << mPort << " due to :" << errno
                << " : " << errno_str;
        mLooper->runWithTimeoutMs(100);
    }

    void completeConnection() {
          // However it should have received an invitation.
        char buf[256] = {0};
        socketRecv(mWaterfallSocket.get(), buf, sizeof(buf));
        EXPECT_STREQ("pipe:unix:sockets/h2o", buf);
        LOG(INFO) << "Connecting back";
        ScopedSocket callback = socketTcp4LoopbackClient(mPort);
        ASSERT_TRUE(callback.valid())
                << "cannot connect to " << mPort << " due to :" << errno
                << " : " << errno_str;
        socket_read_timeout(callback.get(), 1000 * 500);
        char rdy[256] = {0};
        LOG(INFO) << "Waiting for message";
        socketRecv(callback.get(), rdy, sizeof(rdy));
        EXPECT_STREQ("rdy", rdy);
        EXPECT_TRUE(socketSendAll(callback.get(), "rdy", 3));
    }

    void TearDown() override {}

protected:
    int mPort;
    ScopedSocket mServerSocket;
    ScopedSocket mWaterfallSocket;
    std::unique_ptr<WaterfallConnection> mConnection;
    Looper* mLooper;
};

TEST(WaterfallConnection, waterfall_not_responding) {
    ScopedSocket serverSocket = getServerSocket();
    int port = socketGetPort(serverSocket.get());
    auto serverConnector = [&serverSocket]() { return serverSocket.get(); };
    std::unique_ptr<WaterfallConnection> con(
            getTestWaterfallConnection(std::move(serverConnector)));
    EXPECT_TRUE(con->initialize());
    EXPECT_EQ(-1, con->openConnection());
}

TEST_F(WaterfallConnectionTest, waterfall_receive_dial_in) {
    // We expect to timeout as waterfall is not responding.
    EXPECT_EQ(-1, mConnection->openConnection());
    mLooper->runWithTimeoutMs(100);

    // However it should have received an invitation.
    char buf[256] = {0};
    socketRecv(mWaterfallSocket.get(), buf, sizeof(buf));
    EXPECT_STREQ("pipe:unix:sockets/h2o", buf);
}

TEST_F(WaterfallConnectionTest, waterfall_callback_on_dial_in_rdy) {
    std::thread callback([&] {
        // However it should have received an invitation.
        char buf[256] = {0};
        socketRecv(mWaterfallSocket.get(), buf, sizeof(buf));
        EXPECT_STREQ("pipe:unix:sockets/h2o", buf);
        LOG(INFO) << "Connecting back";
        ScopedSocket callback = socketTcp4LoopbackClient(mPort);
        ASSERT_TRUE(callback.valid())
                << "cannot connect to " << mPort << " due to :" << errno
                << " : " << errno_str;
        socket_read_timeout(callback.get(), 1000 * 500);
        char rdy[256] = {0};
        LOG(INFO) << "Waiting for message";
        socketRecv(callback.get(), rdy, sizeof(rdy));
        EXPECT_STREQ("rdy", rdy);
    });
    std::thread looper([&] { mLooper->runWithTimeoutMs(100); });
    // We expect to timeout as waterfall is not responding.
    EXPECT_EQ(-1, mConnection->openConnection());
    callback.join();
    looper.join();
}

TEST_F(WaterfallConnectionTest, waterfall_callback_complete) {
    std::thread callback([&] {
        completeConnection();
    });
    std::thread looper([&] { mLooper->runWithTimeoutMs(100); });
    // We expect to timeout as waterfall is not responding.
    EXPECT_NE(-1, mConnection->openConnection());
    callback.join();
    looper.join();
}


TEST_F(WaterfallConnectionTest, waterfall_two_connections) {
    std::thread callback([&] {
        completeConnection();
        completeConnection();
    });
    std::thread looper([&] { mLooper->runWithTimeoutMs(500); });
    // We expect to timeout as waterfall is not responding.
    EXPECT_NE(-1, mConnection->openConnection());
    EXPECT_NE(-1, mConnection->openConnection());
    callback.join();
    looper.join();
}

}  // namespace control
}  // namespace emulation
}  // namespace android
