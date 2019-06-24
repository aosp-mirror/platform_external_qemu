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

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <iostream>
#include <istream>
#include <sstream>
#include <string>
#include <thread>
#include "android/base/sockets/SocketUtils.h"
#include "android/emulation/control/waterfall/SocketStreambuf.h"

#include "android/base/system/System.h"
#include "android/base/threads/FunctorThread.h"
#include "android/emulation/control/waterfall/WaterfallForwarder.h"
#include "android/emulation/control/waterfall/WaterfallServiceForwarder.h"

namespace android {
namespace emulation {
namespace control {

using namespace base;

class FakeWaterfallConnection : public WaterfallConnection {
public:
    FakeWaterfallConnection(int fd) : mFd(fd) {}
    bool initialize() override { return true; }
    int openConnection() { return mFd; }

    int mFd;
};

TEST(WaterfallServiceForwarder, bad_fd_is_unavailable) {
    ::grpc::ServerReaderWriter<::waterfall::Message, ::waterfall::Message>*
            stream;
    grpc::ClientContext ctx;

    WaterfallServiceForwarder wfall(new FakeWaterfallConnection(-1));
    auto stub = wfall.getWaterfallStub();
    auto status = ManyToMany<::waterfall::Message>(
            &wfall, stream,
            [](auto stub, auto ctx) { return stub->Echo(ctx); });
    EXPECT_FALSE(status.ok());
    EXPECT_EQ(status.error_code(), grpc::StatusCode::UNAVAILABLE);
}


}  // namespace control
}  // namespace emulation
}  // namespace android
