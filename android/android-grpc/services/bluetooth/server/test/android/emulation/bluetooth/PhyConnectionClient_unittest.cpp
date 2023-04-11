// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#include "android/emulation/bluetooth/PhyConnectionClient.h"
#include <grpcpp/grpcpp.h>
#include <gtest/gtest.h>
#include <stdint.h>
#include <sys/types.h>
#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <type_traits>
#include "aemu/base/logging/Log.h"
#include "aemu/base/logging/LogFormatter.h"

#include "aemu/base/async/Looper.h"
#include "aemu/base/files/PathUtils.h"
#include "android/base/system/System.h"
#include "android/base/testing/TestEvent.h"
#include "android/base/testing/TestLooper.h"
#include "android/base/testing/TestSystem.h"
#include "android/base/testing/TestTempDir.h"
#include "root_canal_qemu.h"
#include "android/emulation/bluetooth/EmulatedBluetoothService.h"
#include "android/emulation/control/AndroidAgentFactory.h"
#include "android/emulation/control/GrpcServices.h"
#include "android/emulation/testing/MockAndroidAgentFactory.h"
#include "android/utils/debug.h"
#include "emulated_bluetooth.grpc.pb.h"
#include "emulated_bluetooth.pb.h"
#include "host-common/multi_display_agent.h"
#include "host-common/vm_operations.h"
#include "host-common/window_agent.h"
#include "android/emulation/testing/MockAndroidAgentFactory.h"
#include "android/utils/debug.h"
#include "emulated_bluetooth.grpc.pb.h"
#include "emulated_bluetooth.pb.h"
#include "gtest/gtest_pred_impl.h"
#include "host-common/multi_display_agent.h"
#include "host-common/vm_operations.h"
#include "host-common/window_agent.h"
#include "net/async_data_channel.h"
#include "net/multi_datachannel_server.h"
#define DEBUG 2
/* set  for very verbose debugging */
#if DEBUG <= 1
#define DD(...) (void)0
#else
#define DD(...) dinfo(__VA_ARGS__)
#endif
namespace android {
namespace net {
class AsyncDataChannelServer;
}  // namespace net

namespace emulation {
namespace bluetooth {

using android::base::PathUtils;
using android::base::System;
using android::base::TestLooper;
using android::base::TestSystem;
using android::base::TestTempDir;

using android::base::pj;
using android::net::AsyncDataChannelServer;

using grpc::Service;
using net::AsyncDataChannel;

class FakeRootCanal : public ::android::bluetooth::Rootcanal {
public:
    using Servers = std::vector<std::shared_ptr<AsyncDataChannelServer>>;
    FakeRootCanal()
        : mLinkBleServer(new net::MultiDataChannelServer(Servers())),
          mLinkClassicServer(new net::MultiDataChannelServer(Servers())) {}
    // Starts the root canal service.
    bool start() override { return true; };

    // Closes the root canal service
    void close() override {}

    bool connectQemu() override { return true; }

    bool disconnectQemu() override { return true; }

    void addHciConnection(
            std::shared_ptr<rootcanal::HciTransport> transport) override {}

    net::MultiDataChannelServer* linkBleServer() override {
        return mLinkBleServer.get();
    }

    net::MultiDataChannelServer* linkClassicServer() override {
        return mLinkClassicServer.get();
    }

private:
    std::shared_ptr<net::MultiDataChannelServer> mLinkBleServer;
    std::shared_ptr<net::MultiDataChannelServer> mLinkClassicServer;
};

android::base::VerboseLogFormatter s_log;

class PhyConnectionClientTest : public ::testing::Test {
protected:
    PhyConnectionClientTest() {}

    void SetUp() override {
        setLogFormatter(&s_log);
        VERBOSE_ENABLE(grpc);
        VERBOSE_ENABLE(bluetooth);
        mLooper = std::unique_ptr<base::TestLooper>(new base::TestLooper());
        RegisterGrpcService();
    }

    void TearDown() override { emptyPump(); }

    void RegisterGrpcService() {
        auto bluetooth =
                getEmulatedBluetoothService(&mRootcanal, mLooper.get());
        mGrpcService =
                mBuilder.withService(bluetooth).withPortRange(0, 1).build();
        EXPECT_TRUE(mGrpcService.get());
        mClient = std::make_unique<EmulatorGrpcClient>(address(), "", "", "");
    }

    std::string address() {
        return !mGrpcService
                       ? ""
                       : "localhost:" + std::to_string(mGrpcService->port());
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

    void emptyPump() {
        // Empty out the pump;
        mLooper->runOneIterationWithDeadlineMs(mLooper->nowMs() + 100);
    }

    std::unique_ptr<TestLooper> mLooper;
    FakeRootCanal mRootcanal;
    control::EmulatorControllerService::Builder mBuilder;

    std::unique_ptr<control::EmulatorControllerService> mGrpcService;
    std::unique_ptr<EmulatorGrpcClient> mClient;
};

TEST_F(PhyConnectionClientTest, simple_connect) {
    // Mainly here to check for leaks using asan
    EXPECT_TRUE(mClient->hasOpenChannel());
    auto stub = mClient->stub<
            android::emulation::bluetooth::EmulatedBluetoothService>();
    auto ctx = mClient->newContext();

    // Classic connection.
    auto stream = stub->registerClassicPhy(ctx.get());
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ctx->TryCancel();
    DD("Finished.");
}

TEST_F(PhyConnectionClientTest,
       incoming_grpc_classic_connection_observed_by_rootcanal) {
    TestEvent receivedCallback;
    bool connectionWasMade{false};
    net::MultiDataChannelServer* classic = mRootcanal.linkClassicServer();

    // Rootcanal is listening on this callback, so if this gets fired than
    // rootcanal should have noticed the incoming client connection.
    classic->SetOnConnectCallback([&](auto _unused_, auto _unused2_) {
        DD("Incoming connection, notify listener.");
        connectionWasMade = true;
        receivedCallback.signal();
    });
    classic->StartListening();

    EXPECT_TRUE(mClient->hasOpenChannel());
    auto stub = mClient->stub<
            android::emulation::bluetooth::EmulatedBluetoothService>();
    auto ctx = mClient->newContext();
    ctx->set_deadline(std::chrono::system_clock::now() +
                      std::chrono::seconds(5));
    // Classic connection.
    auto stream = stub->registerClassicPhy(ctx.get());
    pumpLooperUntilEvent(receivedCallback);
    ctx->TryCancel();
    emptyPump();
    DD("Finished.");
}

TEST_F(PhyConnectionClientTest,
       incoming_grpc_ble_connection_observed_by_rootcanal) {
    TestEvent receivedCallback;
    net::MultiDataChannelServer* ble = mRootcanal.linkBleServer();

    // Rootcanal is listening on this callback, so if this gets fired than
    // rootcanal should have noticed the incoming client connection.
    ble->SetOnConnectCallback(
            [&](auto _unused_, auto _unused2_) { receivedCallback.signal(); });
    ble->StartListening();
    auto stub = mClient->stub<
            android::emulation::bluetooth::EmulatedBluetoothService>();
    auto ctx = mClient->newContext();

    // Ble connection.
    auto stream = stub->registerBlePhy(ctx.get());
    pumpLooperUntilEvent(receivedCallback);
}

TEST_F(PhyConnectionClientTest, bytes_from_channel_are_received) {
    // This tests validates that rootcanal would actually receive the raw bytes
    // that come in on the gRPC endpoint.
    // This is done by validating that the gRPC data is being marshalled to an
    // async data channel.

    TestEvent receivedData;
    std::string sampleData = "Hello World";
    net::MultiDataChannelServer* ble = mRootcanal.linkBleServer();
    std::string received;

    ble->SetOnConnectCallback([&](std::shared_ptr<AsyncDataChannel> datachannel,
                                  auto _unused2_) {
        // Rootcanal would receive the bytes from this datachannel.
        datachannel->WatchForNonBlockingRead([&](AsyncDataChannel* from) {
            ssize_t read = 1;
            while (read > 0) {
                uint8_t buf[512];
                read = from->Recv(buf, sizeof(buf));
                if (read > 0)
                    received.append(std::string((char*)buf, read));
            }
            EXPECT_EQ(received, sampleData);
            receivedData.signal();
        });
    });
    ble->StartListening();

    EXPECT_TRUE(mClient->hasOpenChannel());
    auto stub = mClient->stub<
            android::emulation::bluetooth::EmulatedBluetoothService>();
    auto ctx = mClient->newContext();

    // Ble connection.
    auto stream = stub->registerBlePhy(ctx.get());
    RawData data;
    data.set_packet(sampleData);
    EXPECT_TRUE(stream->Write(data));
    pumpLooperUntilEvent(receivedData);
}

TEST_F(PhyConnectionClientTest, bytes_to_channel_are_received) {
    // This tests validates that if rootcanal sends bytes to a datachannel
    // That they will actually end up in the grpc client.
    // This tests validates that rootcanal would actually receive the raw bytes
    // that come in on the gRPC endpoint.
    // This is done by validating that the gRPC data is being marshalled to an
    // async data channel.

    TestEvent done;
    std::string sampleData = "Hello World";
    net::MultiDataChannelServer* ble = mRootcanal.linkBleServer();
    std::shared_ptr<AsyncDataChannel> activeChannel;

    ble->SetOnConnectCallback(
            [&](std::shared_ptr<AsyncDataChannel> datachannel, auto _unused2_) {
                // Make sure the channel doesn't go out of scope before we
                // actually finished writing. activeChannel = datachannel;
                auto written = datachannel->Send((uint8_t*)sampleData.data(),
                                                 sampleData.size());
                EXPECT_EQ(written, sampleData.size());
                DD("Server wrote %s", sampleData.c_str());
            });
    ble->StartListening();

    EXPECT_TRUE(mClient->hasOpenChannel());
    auto stub = mClient->stub<
            android::emulation::bluetooth::EmulatedBluetoothService>();
    auto ctx = mClient->newContext();

    ctx->set_deadline(std::chrono::system_clock::now() +
                      std::chrono::seconds(5));

    // Ble connection.
    auto stream = stub->registerBlePhy(ctx.get());

    // We have to manually pump the message loop, this is not needed in qemu.
    std::thread pump([&]() { pumpLooperUntilEvent(done); });

    std::string receiving;
    RawData data;

    while (receiving != sampleData && stream->Read(&data)) {
        DD("Got some stuff: %s!", data.packet().c_str());
        receiving.append(data.packet());
    }

    EXPECT_EQ(sampleData, receiving);
    done.signal();

    pump.join();
}

TEST_F(PhyConnectionClientTest,
       emulated_link_layer_connection_client_connects) {
    TestEvent receivedCallback;
    net::MultiDataChannelServer* ble = mRootcanal.linkBleServer();

    // Rootcanal is listening on this callback, so if this gets fired than
    // rootcanal should have noticed the incoming client connection.
    ble->SetOnConnectCallback(
            [&](auto _unused_, auto _unused2_) { receivedCallback.signal(); });
    ble->StartListening();

    FakeRootCanal canal2;

    PhyConnectionClient llc(*mClient.get(), &canal2, mLooper.get());
    llc.connectLinkLayerBle();

    pumpLooperUntilEvent(receivedCallback);
    emptyPump();
}
TEST_F(PhyConnectionClientTest,
       emulated_link_layer_connection_client_receives_from_server) {
    // The Fake Rootcanal on the server side will send Hello World back to the
    // client. So we should see this: Rootcanal <--server adapter--> grpc
    // <--client adapter--> Rootcanal 2
    TestEvent receivedData;
    std::string sampleData = "Hello World";
    net::MultiDataChannelServer* ble = mRootcanal.linkBleServer();

    ble->SetOnConnectCallback(
            [&](std::shared_ptr<AsyncDataChannel> datachannel, auto _unused2_) {
                DD("Server is connected.");
                // Make sure the channel doesn't go out of scope before we
                // actually finished writing. activeChannel = datachannel;
                auto written = datachannel->Send((uint8_t*)sampleData.data(),
                                                 sampleData.size());
                EXPECT_EQ(written, sampleData.size());
                DD("Server wrote %s", sampleData.c_str());
            });
    ble->StartListening();

    FakeRootCanal canal2;
    net::MultiDataChannelServer* ble2 = canal2.linkBleServer();
    std::string received;
    DD("Registering on %p", ble2);
    ble2->SetOnConnectCallback(
            [&](std::shared_ptr<AsyncDataChannel> datachannel, auto _unused2_) {
                // Rootcanal would receive the bytes from this datachannel.
                // In this case we are just waiting to receive the sample data,
                // Which means we are adapting properly over gRPC.
                DD("Client is connected.");
                datachannel->WatchForNonBlockingRead(
                        [&](AsyncDataChannel* from) {
                            ssize_t read = 1;
                            while (read > 0) {
                                uint8_t buf[512];
                                read = from->Recv(buf, sizeof(buf));
                                if (read > 0)
                                    received.append(
                                            std::string((char*)buf, read));
                            }
                            EXPECT_EQ(received, sampleData);
                            receivedData.signal();
                        });
            });
    ble2->StartListening();

    PhyConnectionClient llc(*mClient.get(), &canal2, mLooper.get());
    llc.connectLinkLayerBle();

    pumpLooperUntilEvent(receivedData);
    emptyPump();
}

class FakeConsoleFactory : public android::emulation::AndroidConsoleFactory {
    const QAndroidHciAgent* const android_get_QAndroidHciAgent()
            const override {
        static QAndroidHciAgent mAgent{
                .recv = [](uint8_t* b, uint64_t size) { return (ssize_t)0; },
                .send = [](const uint8_t* b,
                           uint64_t size) { return (ssize_t)0; },
                .available = []() { return (size_t)0; },
                .registerDataAvailableCallback = [](auto o, auto availabe) {}};
        return &mAgent;
    }
};

TEST_F(PhyConnectionClientTest, real_rootcanal_interacts) {
    // The Fake Rootcanal on the server side will send Hello World back to the
    // client. So we should see this: Rootcanal <--server adapter--> grpc
    // <--client adapter--> Rootcanal 2
    android::emulation::injectConsoleAgents(FakeConsoleFactory());
    TestEvent receivedData;
    std::string sampleData = "DIE PLEASE!";
    android::bluetooth::Rootcanal::Builder builder;
    builder.withLooper(mLooper.get()).buildSingleton();
    auto rootcanal = android::bluetooth::Rootcanal::Builder::getInstance();
    rootcanal->start();

    auto bluetooth =
            getEmulatedBluetoothService(rootcanal.get(), mLooper.get());
    auto rpcService =
            mBuilder.withService(bluetooth).withPortRange(0, 1).build();

    auto client = std::make_unique<EmulatorGrpcClient>(
            "localhost:" + std::to_string(rpcService->port()), "", "", "");
    client->hasOpenChannel();

    EXPECT_TRUE(mClient->hasOpenChannel());
    auto stub = client->stub<
            android::emulation::bluetooth::EmulatedBluetoothService>();
    auto ctx = client->newContext();

    // Classic connection.
    auto stream = stub->registerBlePhy(ctx.get());
    RawData packet;
    packet.set_packet("HELLO HERE'S SOME NONSENSE FOR YOU.");
    stream->Write(packet);
    emptyPump();
    ctx->TryCancel();
    DD("Finished.");
    emptyPump();
}
}  // namespace bluetooth
}  // namespace emulation
}  // namespace android
