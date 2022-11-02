// Copyright 2022 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#include "android/emulation/bluetooth/GrpcLinkChannelServer.h"

#include <gtest/gtest.h>  // for Message, TestPartResult
#include <string>         // for string, operator==, bas...
#include <thread>         // for thread

#include "aemu/base/async/Looper.h"        // for Looper, Looper::Duration
#include "android/base/testing/TestEvent.h"   // for TestEvent
#include "android/base/testing/TestLooper.h"  // for TestLooper

using android::base::TestLooper;

namespace android {
namespace emulation {
namespace bluetooth {

class GrpcLinklChannelServerTest : public ::testing::Test {
protected:
    void SetUp() override {
        mLooper = std::unique_ptr<base::TestLooper>(new base::TestLooper());
        mServer = std::make_unique<GrpcLinkChannelServer>(mLooper.get());
    }

    void TearDown() override {
        mServer.reset();
        mLooper.reset();
    }

    void pumpLooperUntilEvent(TestEvent& event) {
        constexpr base::Looper::Duration kTimeoutMs = 1000;
        constexpr base::Looper::Duration kStep = 50;

        base::Looper::Duration current = mLooper->nowMs();
        const base::Looper::Duration deadline = mLooper->nowMs() + kTimeoutMs;

        while (!event.isSignaled() && current + kStep < deadline) {
            mLooper->runOneIterationWithDeadlineMs(current + kStep);
            current = mLooper->nowMs();
        }

        event.wait();
    }

    std::unique_ptr<TestLooper> mLooper;
    std::unique_ptr<GrpcLinkChannelServer> mServer;
};

TEST_F(GrpcLinklChannelServerTest, can_create_connection) {
    EXPECT_TRUE(mServer->createConnection() != nullptr);
}

TEST_F(GrpcLinklChannelServerTest, create_raises_an_event_when_listening) {
    TestEvent receivedCallback;
    mServer->SetOnConnectCallback(
            [&](auto, auto) { receivedCallback.signal(); });
    mServer->StartListening();
    mServer->createConnection();
    pumpLooperUntilEvent(receivedCallback);
}
}  // namespace bluetooth
}  // namespace emulation
}  // namespace android