// Copyright 2020 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/network/WifiForwardPeer.h"

#include "android/base/misc/IpcPipe.h"
#include "android/base/sockets/ScopedSocket.h"
#include "android/base/sockets/SocketUtils.h"
#include "android/base/testing/TestLooper.h"

#include <gtest/gtest.h>

using namespace android::base;
using namespace android::network;

static constexpr Looper::Duration kTimeoutMs = 1000;  // 1 seconds.

// Provide a stub impl for start() function.
class WifiForwardPeerForTest : public WifiForwardPeer {
public:
    WifiForwardPeerForTest(OnDataAvailableCallback onDataAvailable)
        : WifiForwardPeer(onDataAvailable) {}

protected:
    void start() override {}
};

static void block(TestLooper& looper,
                  bool& onDataIsCalled,
                  std::function<void()> callback) {
    Looper::Duration deadline = looper.nowMs() + kTimeoutMs;
    while (looper.nowMs() < deadline) {
        looper.runOneIterationWithDeadlineMs(looper.nowMs() + 10);
        if (onDataIsCalled) {
            callback();
            onDataIsCalled = false;
            break;
        }
    }
}

TEST(WifiForwardPeerTest, onDataAvailable) {
    // Always consume one byte less than provided.
    bool onDataIsCalled = false;
    auto onData = [&onDataIsCalled](const uint8_t* data, size_t size) {
        onDataIsCalled = true;
        return (size > 0) ? (size - 1) : 0;
    };
    TestLooper looper;
    auto forwarder = std::make_unique<WifiForwardPeerForTest>(onData);
    int fds[2];
    EXPECT_EQ(socketCreatePair(&fds[0], &fds[1]), 0);

    ASSERT_TRUE(forwarder->initForTesting(&looper, fds[0]));
    const char kData[] = "Hello World!";
    const size_t kDataLen = sizeof(kData) - 1U;
    EXPECT_EQ(forwarder->size(), 0);
    EXPECT_TRUE(socketSendAll(fds[1], kData, kDataLen));

    auto cb1 = [&forwarder, kData, kDataLen]() {
        EXPECT_EQ(forwarder->size(), 1);
        const char firstRemaining =
                *static_cast<const char*>(forwarder->data());
        EXPECT_EQ(firstRemaining, kData[kDataLen - 1]);
    };

    block(looper, onDataIsCalled, cb1);

    const char kMoreData[] = "Again";
    const size_t kMoreDataLen = sizeof(kMoreData) - 1U;
    EXPECT_TRUE(android::base::socketSendAll(fds[1], kMoreData, kMoreDataLen));

    auto cb2 = [&forwarder, kMoreData, kMoreDataLen]() {
        EXPECT_EQ(forwarder->size(), 1);
        // Test if the remain char is 'n'
        const char secondRemaining =
                *static_cast<const char*>(forwarder->data());
        EXPECT_EQ(secondRemaining, kMoreData[kMoreDataLen - 1]);
    };

    block(looper, onDataIsCalled, cb2);
}
