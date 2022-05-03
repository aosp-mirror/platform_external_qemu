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

// These must be included first due to some windows header issues.
#include "root_canal_qemu.h"   // for Rootcanal::...
#include "test_environment.h"  // for TestEnviron...

#include "android/base/async/ThreadLooper.h"              // for ThreadLooper
#include "android/utils/debug.h"                          // for derror
#include "model/setup/async_manager.h"                    // for AsyncManager
#include "net/hci_datachannel_server.h"                   // for HciDataChan...
#include "net/multi_datachannel_server.h"                 // for MultiDataCh...
#include "net/null_datachannel_server.h"                  // for NullDataCha...
#include "net/sockets/loopback_async_socket_connector.h"  // for LoopbackAsy...
#include "net/sockets/loopback_async_socket_server.h"     // for LoopbackAsy...

namespace android {

namespace bluetooth {

using root_canal::TestEnvironment;
using RootcanalBuilder = Rootcanal::Builder;

class RootcanalImpl : public Rootcanal {
public:
    RootcanalImpl(
            std::unique_ptr<TestEnvironment> rootcanal,
            std::unique_ptr<AsyncManager> am,
            std::shared_ptr<net::HciDataChannelServer> qemuHciServer,
            std::shared_ptr<net::MultiDataChannelServer> linkBleServer,
            std::shared_ptr<net::MultiDataChannelServer> linkClassicServer,
            std::shared_ptr<net::MultiDataChannelServer> hciServer)
        : mRootcanal(std::move(rootcanal)),
          mAsyncManager(std::move(am)),
          mQemuHciServer(std::move(qemuHciServer)),
          mLinkBleServer(linkBleServer),
          mLinkClassicServer(linkClassicServer),
          mHciServer(hciServer) {}
    ~RootcanalImpl() {}

    // Starts the root canal service.
    bool start() override {
        std::promise<void> barrier;
        mRootcanal->initialize(std::move(barrier));
        return true;
    };

    // Closes the root canal service
    void close() override { mRootcanal.reset(nullptr); }

    net::HciDataChannelServer* qemuHciServer() override {
        return mQemuHciServer.get();
    }

    net::MultiDataChannelServer* linkBleServer() override {
        return mLinkBleServer.get();
    }

    net::MultiDataChannelServer* linkClassicServer() override {
        return mLinkClassicServer.get();
    }

    net::MultiDataChannelServer* hciServer() override {
        return mHciServer.get();
    }

private:
    std::unique_ptr<AsyncManager> mAsyncManager;
    std::shared_ptr<net::HciDataChannelServer> mQemuHciServer;
    std::shared_ptr<net::MultiDataChannelServer> mLinkBleServer;
    std::shared_ptr<net::MultiDataChannelServer> mLinkClassicServer;
    std::shared_ptr<net::MultiDataChannelServer> mHciServer;
    std::unique_ptr<TestEnvironment> mRootcanal;
};

RootcanalBuilder::Builder() {
    mLooper = base::ThreadLooper::get();
}

static int getPortNumber(const char* value) {
    int port = -1;
    if (value && sscanf(value, "%d", &port) == 1) {
        return port;
    }
    return -1;
}

RootcanalBuilder& RootcanalBuilder::withControllerProperties(
        const char* props) {
    if (props)
        mDefaultControllerProperties = props;
    return *this;
}

RootcanalBuilder& RootcanalBuilder::withLooper(android::base::Looper* looper) {
    mLooper = looper;
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

std::shared_ptr<RootcanalImpl> sRootcanalImpl;

std::shared_ptr<Rootcanal> RootcanalBuilder::getInstance() {
    if (!sRootcanalImpl) {
        derror("Rootcanal has not yet been initialized..");
    }
    return sRootcanalImpl;
}

void RootcanalBuilder::buildSingleton() {
    auto asyncManager = std::make_unique<AsyncManager>();

    auto nullChannel = std::make_shared<net::NullDataChannelServer>();
    auto qemuHciServer = std::make_shared<net::HciDataChannelServer>(mLooper);
    net::DataChannelServerList hciServers;
    hciServers.push_back(qemuHciServer);
    auto hciServer = std::make_shared<net::MultiDataChannelServer>(hciServers);
    auto testServer = nullChannel;
    auto linkServer =
            std::make_shared<net::MultiDataChannelServer>(nullChannel);
    auto linkBleServer =
            std::make_shared<net::MultiDataChannelServer>(nullChannel);
    auto localConnector = std::make_shared<net::LoopbackAsyncSocketConnector>(
            asyncManager.get());

    auto testEnv = std::make_unique<TestEnvironment>(
            testServer, hciServer, linkServer, linkBleServer, localConnector,
            mDefaultControllerProperties, mCmdFile);

    sRootcanalImpl = std::make_shared<RootcanalImpl>(
            std::move(testEnv), std::move(asyncManager), qemuHciServer,
            std::move(linkBleServer), std::move(linkServer),
            std::move(hciServer));
}
}  // namespace bluetooth
}  // namespace android
