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
#include <stdio.h>     // for sscanf
#include <functional>  // for bind, placeholders
#include <future>      // for promise
#include <memory>      // for shared_ptr
#include <string>      // for string
#include <utility>     // for move
#include <vector>      // for vector

// These must be included first due to some windows header issues.
#include "model/setup/test_model.h"  // for TestModel
#include "root_canal_qemu.h"         // for Rootcanal::...
#include "hci/hci_packets.h"         // for bluetooth::hci::...

#include "model/devices/beacon.h"
#include "model/devices/beacon_swarm.h"
#include "model/devices/sniffer.h"
#include "model/hci/hci_sniffer.h"
#include "model/setup/device_boutique.h"

#include "aemu/base/async/ThreadLooper.h"  // for ThreadLooper
#include "aemu/base/files/PathUtils.h"
#include "android/base/system/System.h"
#include "android/utils/debug.h"  // for derror
#include "android/utils/path.h"
#include "model/devices/link_layer_socket_device.h"  // for LinkLayerSo...
#include "model/hci/hci_socket_transport.h"          // for HciSocketTr...
#include "model/setup/async_manager.h"               // for AsyncManager
#include "model/setup/test_command_handler.h"        // for TestCommand...

#include "android/console.h"  // for getConsoleAgents, AndroidCons...
#include "net/multi_datachannel_server.h"  // for MultiDataCh...
#include "net/null_datachannel_server.h"   // for NullDataCha...
#include "net/qemu_datachannel.h"          // for QemuDataChannel, Looper
#include "net/sockets/loopback_async_socket_connector.h"  // for LoopbackAsy...
#include "net/sockets/loopback_async_socket_server.h"     // for LoopbackAsy...

namespace android {

namespace bluetooth {

using rootcanal::DeviceBoutique;
using rootcanal::HciDevice;
using rootcanal::HciSocketTransport;
using rootcanal::LinkLayerSocketDevice;
using rootcanal::TestCommandHandler;
using rootcanal::TestModel;
using RootcanalBuilder = Rootcanal::Builder;
using android::base::System;
using namespace std::placeholders;

class RootcanalImpl : public Rootcanal {
public:
    RootcanalImpl(
            base::Looper* looper,
            std::string controllerProperties,
            std::string cmdFile,
            std::shared_ptr<net::MultiDataChannelServer> linkBleServer,
            std::shared_ptr<net::MultiDataChannelServer> linkClassicServer)
        : mLooper(looper),
          mControllerProperties(controllerProperties),
          mCmdFile(cmdFile),
          mLinkBleServer(linkBleServer),
          mLinkClassicServer(linkClassicServer) {}
    ~RootcanalImpl() {}

    // Starts the root canal service.
    bool start() override {
        auto testCommands = TestCommandHandler(mRootcanal);

        testCommands.RegisterSendResponse([](const std::string&) {});

        testCommands.AddPhy({"BR_EDR"});
        testCommands.AddPhy({"LOW_ENERGY"});
        testCommands.SetTimerPeriod({"5"});
        testCommands.StartTimer({});
        testCommands.FromFile(mCmdFile);

        mLinkBleServer->SetOnConnectCallback(
                [this](std::shared_ptr<net::AsyncDataChannel> socket,
                       net::AsyncDataChannelServer* srv) {
                    auto phy_type = rootcanal::Phy::Type::LOW_ENERGY;
                    mRootcanal.AddLinkLayerConnection(
                            LinkLayerSocketDevice::Create(socket, phy_type),
                            phy_type);
                    srv->StartListening();
                });
        mLinkBleServer->StartListening();

        mLinkClassicServer->SetOnConnectCallback(
                [this](std::shared_ptr<net::AsyncDataChannel> socket,
                       net::AsyncDataChannelServer* srv) {
                    auto phy_type = rootcanal::Phy::Type::BR_EDR;
                    mRootcanal.AddLinkLayerConnection(
                            LinkLayerSocketDevice::Create(socket, phy_type),
                            phy_type);
                    srv->StartListening();
                });
        mLinkClassicServer->StartListening();

        return connectQemu();
    };

    // Closes the root canal service
    void close() override { mRootcanal.Reset(); }

    bool connectQemu() override {
        if (!mQemuHciDevice) {
            auto channel = std::make_shared<net::QemuDataChannel>(
                    getConsoleAgents()->rootcanal, mLooper);

            auto transport = HciSocketTransport::Create(channel);
            if (VERBOSE_CHECK(bluetooth)) {
                auto stream = android::bluetooth::getLogstream("qemu.pcap");
                transport = rootcanal::HciSniffer::Create(transport, stream);
            }

            mQemuHciDevice =
                    HciDevice::Create(transport, mControllerProperties);
            mRootcanal.AddHciConnection(mQemuHciDevice);
            return true;
        } else {
            return false;
        }
    }

    bool disconnectQemu() override {
        if (mQemuHciDevice) {
            mQemuHciDevice->Close();
            mQemuHciDevice.reset();
            return true;
        } else {
            return false;
        }
    }

    void addHciConnection(
            std::shared_ptr<rootcanal::HciTransport> transport) override {
        auto device = HciDevice::Create(transport, mControllerProperties);
        mRootcanal.AddHciConnection(device);
    }

    net::MultiDataChannelServer* linkBleServer() override {
        return mLinkBleServer.get();
    }

    net::MultiDataChannelServer* linkClassicServer() override {
        return mLinkClassicServer.get();
    }

private:
    AsyncManager mAsyncManager;

    TestModel mRootcanal{
            std::bind(&AsyncManager::GetNextUserId, &mAsyncManager),
            std::bind(&AsyncManager::ExecAsync, &mAsyncManager, _1, _2, _3),
            std::bind(&AsyncManager::ExecAsyncPeriodically,
                      &mAsyncManager,
                      _1,
                      _2,
                      _3,
                      _4),
            std::bind(&AsyncManager::CancelAsyncTasksFromUser,
                      &mAsyncManager,
                      _1),
            std::bind(&AsyncManager::CancelAsyncTask, &mAsyncManager, _1),
            [this](const std::string& /* server */,
                   int /* port */,
                   rootcanal::Phy::Type /* phy_type */) { return nullptr; }};

    base::Looper* mLooper;
    std::string mControllerProperties;
    std::string mCmdFile;
    std::shared_ptr<rootcanal::HciDevice> mQemuHciDevice;
    std::shared_ptr<net::MultiDataChannelServer> mLinkBleServer;
    std::shared_ptr<net::MultiDataChannelServer> mLinkClassicServer;
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
    // Register devices.
    DeviceBoutique::Register("beacon", &rootcanal::Beacon::Create);
    DeviceBoutique::Register("beacon_swarm", &rootcanal::BeaconSwarm::Create);
    DeviceBoutique::Register("sniffer", &rootcanal::Sniffer::Create);

    auto nullChannel = std::make_shared<net::NullDataChannelServer>();
    auto linkServer =
            std::make_shared<net::MultiDataChannelServer>(nullChannel);
    auto linkBleServer =
            std::make_shared<net::MultiDataChannelServer>(nullChannel);

    sRootcanalImpl = std::make_shared<RootcanalImpl>(
            mLooper, mDefaultControllerProperties, mCmdFile,
            std::move(linkBleServer), std::move(linkServer));
}
}  // namespace bluetooth
}  // namespace android
