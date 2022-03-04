//
// Copyright 2017 The Android Open Source Project
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
//
#include <stdio.h>  // for sscanf
#include <future>   // for promise
#include <memory>   // for shared_ptr
#include <string>   // for string
#include <utility>  // for move
#include <vector>   // for vector

// Include first! Windows will be unlhappy!
#include "root_canal_qemu.h"                              // for Rootcanal::...
#include "test_environment.h"                             // for TestEnviron...


#include "android/base/async/ThreadLooper.h"              // for ThreadLooper
#include "model/setup/async_manager.h"                    // for AsyncManager
#include "net/hci_datachannel_server.h"                   // for HciDataChan...
#include "net/multi_datachannel_server.h"                 // for MultiDataCh...
#include "net/null_datachannel_server.h"                  // for NullDataCha...
#include "net/sockets/loopback_async_socket_connector.h"  // for LoopbackAsy...
#include "net/sockets/loopback_async_socket_server.h"     // for LoopbackAsy...

namespace android {
namespace net {
class AsyncDataChannelServer;
}  // namespace net

namespace bluetooth {

using root_canal::TestEnvironment;
using RootcanalBuilder = Rootcanal::Builder;

class RootcanalImpl : public Rootcanal {
public:
    RootcanalImpl(std::unique_ptr<TestEnvironment> rootcanal,
                  std::unique_ptr<AsyncManager> am)
        : mRootcanal(std::move(rootcanal)), mAsyncManager(std::move(am)) {}
    ~RootcanalImpl() {}

    // Starts the root canal service.
    bool start() override {
        std::promise<void> barrier;
        mRootcanal->initialize(std::move(barrier));
        return true;
    };

    // Closes the root canal service
    void close() override {
        mAsyncManager.reset(nullptr);
        // the following has memory double free problem
        // and often crashes upon exit mostly on M1; so
        // just let the
        // object fall out of scope and be cleaned up
        // b/203795141
        // mRootcanal->close();
        mRootcanal.reset(nullptr);
    }

private:
    std::unique_ptr<TestEnvironment> mRootcanal;
    std::unique_ptr<AsyncManager> mAsyncManager;
};

RootcanalBuilder& RootcanalBuilder::withHciPort(int port) {
    mHci = port;
    return *this;
}

static int getPortNumber(const char* value) {
    int port = -1;
    if (value && sscanf(value, "%d", &port) == 1) {
        return port;
    }
    return -1;
}

RootcanalBuilder& RootcanalBuilder::withHciPort(const char* portStr) {
    return withHciPort(getPortNumber(portStr));
}

RootcanalBuilder& RootcanalBuilder::withTestPort(int port) {
    mTest = port;
    return *this;
}

RootcanalBuilder& RootcanalBuilder::withTestPort(const char* portStr) {
    return withTestPort(getPortNumber(portStr));
}

RootcanalBuilder& RootcanalBuilder::withLinkPort(int port) {
    mLink = port;
    return *this;
}

RootcanalBuilder& RootcanalBuilder::withLinkPort(const char* portStr) {
    return withLinkPort(getPortNumber(portStr));
}

RootcanalBuilder& RootcanalBuilder::withLinkBlePort(int port) {
    mLinkBle = port;
    return *this;
}

RootcanalBuilder& RootcanalBuilder::withLinkBlePort(const char* portStr) {
    return withLinkBlePort(getPortNumber(portStr));
}


RootcanalBuilder& RootcanalBuilder::withControllerProperties(
        const char* props) {
    if (props)
        mDefaultControllerProperties = props;
    return *this;
}

RootcanalBuilder& RootcanalBuilder::withCommandFile(const char* cmdFile) {
    if (cmdFile)
        mCmdFile = cmdFile;
    return *this;
}

static std::shared_ptr<net::AsyncDataChannelServer> getChannelServer(
        int port,
        AsyncManager* am) {
    if (port > 0) {
        return std::make_shared<net::LoopbackAsyncSocketServer>(port, am);
    }
    return std::make_shared<net::NullDataChannelServer>();
}

std::unique_ptr<Rootcanal> RootcanalBuilder::build() {
    auto asyncManager = std::make_unique<AsyncManager>();

    std::vector<std::shared_ptr<net::AsyncDataChannelServer>> hciServers{
            std::make_shared<net::HciDataChannelServer>(
                    base::ThreadLooper::get())};
    if (mHci > 0) {
        hciServers.push_back(std::make_shared<net::LoopbackAsyncSocketServer>(
                mHci, asyncManager.get()));
    }

    auto hciServer = std::make_shared<net::MultiDataChannelServer>(hciServers);
    auto testServer = getChannelServer(mTest, asyncManager.get());
    auto linkServer = getChannelServer(mLink, asyncManager.get());
    auto linkBleServer = getChannelServer(mLinkBle, asyncManager.get());
    auto localConnector = std::make_shared<net::LoopbackAsyncSocketConnector>(
            asyncManager.get());

    auto testEnv = std::make_unique<TestEnvironment>(
            testServer, hciServer, linkServer, linkBleServer, localConnector,
            mDefaultControllerProperties, mCmdFile);

    return std::make_unique<RootcanalImpl>(std::move(testEnv),
                                           std::move(asyncManager));
}
}  // namespace bluetooth
}  // namespace android
