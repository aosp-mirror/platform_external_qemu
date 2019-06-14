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

#include "android/emulation/control/waterfall/WaterfallService.h"
#include "android/base/system/System.h"
#include <gtest/gtest.h>

namespace android {
namespace emulation {
namespace control {

TEST(WaterfallService, socket_pair_loops) {
    int guestFd = 0;
    int hostFd = 0;
    base::Looper* looper = android::base::ThreadLooper::get();
    EXPECT_FALSE(base::socketCreatePair(&guestFd, &hostFd));
    std::string received;
    std::string msg = "hello";
    emulator::net::LambdaReader<std::string> msgReader([&received](SocketTransport* from, std::string object){
        received += object;
     });
    SocketTransport fakeHost(nullptr, new AsyncSocketFd(looper, hostFd));
    SocketTransport fakeDevice(&msgReader, new AsyncSocketFd(looper, guestFd));
    fakeHost.send("hello");
    looper->runWithTimeoutMs(1000);
    EXPECT_EQ(received.size(), 5);
    EXPECT_STREQ(received.c_str(), "hello");
}

TEST(WaterfallService, socket_pair_with_forwarder) {
    // device (a) <-> b  --- read forward [len|string] to string  -->  c <-> (d) client
    // device (a) <-> b  <-- write forward string to [len|string] ---  c <-> (d) client
    int a, b, c, d;
    base::Looper* looper = android::base::ThreadLooper::get();
    EXPECT_FALSE(base::socketCreatePair(&a, &b));
    EXPECT_FALSE(base::socketCreatePair(&c, &d));

    std::unique_ptr<AsyncSocketFd> sA = std::make_unique<AsyncSocketFd>(looper, a);
    std::unique_ptr<AsyncSocketFd> sB = std::make_unique<AsyncSocketFd>(looper, b);
    std::unique_ptr<AsyncSocketFd> sC = std::make_unique<AsyncSocketFd>(looper, c);
    std::unique_ptr<AsyncSocketFd> sD = std::make_unique<AsyncSocketFd>(looper, d);
    std::string received;
    std::string msg = "hello";
    emulator::net::LambdaReader<std::string> msgReader([&received](SocketTransport* from, std::string object){
        received += object;
     });
    WaterfallEncoder wwf(looper, sC.get(), sB.get());
    WaterfallDecoder wrf(looper, sB.get(), sC.get());
    SocketTransport fakeHost(nullptr, sD.get());
    SocketTransport fakeDevice(&msgReader, sA.get());
    fakeHost.send("hello");
    looper->runWithTimeoutMs(1000);
    EXPECT_EQ(received.size(), 9);
    EXPECT_STREQ(received.substr(4).c_str(), "hello");
}
}  // namespace control
}  // namespace emulation
}  // namespace android
