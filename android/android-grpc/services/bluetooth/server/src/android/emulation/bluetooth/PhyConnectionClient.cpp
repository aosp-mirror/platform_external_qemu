// Copyright (C) 2022 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use connection file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "android/emulation/bluetooth/PhyConnectionClient.h"

#include <grpcpp/grpcpp.h>     // for Clien...
#include <stdint.h>            // for uint8_t
#include <condition_variable>  // for condi...
#include <mutex>               // for condi...
#include <string>              // for basic...
#include <type_traits>         // for remov...
#include <utility>             // for move

#include "android/emulation/bluetooth/GrpcLinkChannelServer.h"  // for GrpcL...
#include "android/emulation/control/EmulatorAdvertisement.h"    // for Emula...
#include "android/grpc/utils/SimpleAsyncGrpc.h"                 // for Simpl...
#include "android/utils/debug.h"                                // for dinfo
#include "emulated_bluetooth.grpc.pb.h"                         // for Emula...
#include "emulated_bluetooth.pb.h"                              // for RawData
#include "net/multi_datachannel_server.h"                       // for Multi...
#include "root_canal_qemu.h"                                    // for Rootc...

// #define DEBUG 1
/* set >1 for very verbose debugging */
#if DEBUG <= 1
#define DD(...) (void)0
#else
#define DD(...) dinfo(__VA_ARGS__)
#endif

namespace android {
namespace base {
class Looper;
}  // namespace base

namespace emulation {
namespace bluetooth {
class BufferedAsyncDataChannel;

using ::android::bluetooth::Rootcanal;
using android::emulation::control::EmulatorAdvertisement;
using grpc::ClientContext;

class LinkForwarder : public SimpleClientBidiStream<RawData, RawData>,
                      public std::enable_shared_from_this<LinkForwarder> {
public:
    LinkForwarder(GrpcLinkChannelServer* server,
                  std::unique_ptr<ClientContext>&& ctx)
        : mChannelServer(server), mContext(std::move(ctx)) {
        mRootcanalBridge = mChannelServer->createConnection();
        // We are going to intercept sends from rootcanal and package
        // them into RawData objects that will be send over the
        // connection.
        mRootcanalBridge->WatchForNonBlockingSend([this](auto dataChannel) {
            RawData packet;
            std::string packetBuffer;

            auto toForward = dataChannel->availableInSendBuffer();
            // Let's not send empty packets..
            if (toForward == 0) {
                return;
            }

            packetBuffer.resize(toForward);
            dataChannel->ReadFromSendBuffer((uint8_t*)packetBuffer.data(),
                                            toForward);
            packet.set_packet(std::move(packetBuffer));

            DD("Forwarding %s <- %s", mContext->peer().c_str(),
               packet.ShortDebugString().c_str());
            this->Write(packet);
        });

        mRootcanalBridge->WatchForClose([this](auto theRootCanalBridge) {
            // Bye bye
            DD("Cancelling %s due to close.", mContext->peer().c_str());
            mContext->TryCancel();
        });
    }

    virtual ~LinkForwarder() {}

    void Read(const RawData* received) override {
        DD("Forwarding %s -> %s", mContext->peer().c_str(),
           received->ShortDebugString().c_str());
        mRootcanalBridge->WriteToRecvBuffer((uint8_t*)received->packet().data(),
                                            received->packet().size());
    }

    void OnDone(const grpc::Status& s) override {
        DD("OnDone %s", mContext->peer().c_str());

        // Close out rootcanal connection, note that this
        // will be called either due to emulator exit,
        // or connection issues with the remote instance
        mRootcanalBridge->WatchForNonBlockingRead(nullptr);
        mRootcanalBridge->WatchForClose(nullptr);
        mRootcanalBridge->Close();

        std::unique_lock<std::mutex> l(mAwaitMutex);
        mStatus = s;
        mDone = true;
        mAwaitCondition.notify_one();
    }

    // Blocks and waits until this connection has completed.
    grpc::Status await() {
        std::unique_lock<std::mutex> l(mAwaitMutex);
        mAwaitCondition.wait(l, [this] { return mDone; });
        return std::move(mStatus);
    }

    void cancel() { mContext->TryCancel(); }

    virtual void startCall(EmulatedBluetoothService::Stub* stub){};

protected:
    std::unique_ptr<grpc::ClientContext> mContext;

private:
    std::mutex mAwaitMutex;
    std::condition_variable mAwaitCondition;
    grpc::Status mStatus;
    bool mDone = false;

    GrpcLinkChannelServer* mChannelServer;
    std::shared_ptr<BufferedAsyncDataChannel> mRootcanalBridge;
};

class BleLinkForwarder : public LinkForwarder {
public:
    BleLinkForwarder(GrpcLinkChannelServer* server,
                     std::unique_ptr<ClientContext>&& ctx)
        : LinkForwarder(server, std::move(ctx)) {}
    void startCall(EmulatedBluetoothService::Stub* stub) override {
        stub->async()->registerBlePhy(mContext.get(), this);
        StartCall();
        StartRead();
    }
};

class ClassicLinkForwarder : public LinkForwarder {
public:
    ClassicLinkForwarder(GrpcLinkChannelServer* server,
                         std::unique_ptr<ClientContext>&& ctx)
        : LinkForwarder(server, std::move(ctx)) {}
    void startCall(EmulatedBluetoothService::Stub* stub) override {
        stub->async()->registerClassicPhy(mContext.get(), this);
        StartRead();
        StartCall();
    }
};

PhyConnectionClient::PhyConnectionClient(
        EmulatorGrpcClient client,
        android::bluetooth::Rootcanal* rootcanal,
        base::Looper* looper)
    : mClient(client), mRootcanal(rootcanal) {
    mClassicServer = std::make_shared<GrpcLinkChannelServer>(looper);
    mBleServer = std::make_shared<GrpcLinkChannelServer>(looper);

    mRootcanal->linkClassicServer()->add(mClassicServer);
    mRootcanal->linkBleServer()->add(mBleServer);
    mStub = mClient.stub<
            android::emulation::bluetooth::EmulatedBluetoothService>();
}

PhyConnectionClient::~PhyConnectionClient() {
    DD("Closing down servers.");
    if (mClassicHandler) {
        mClassicHandler->cancel();
        mClassicHandler->await();
    }
    if (mBleHandler) {
        mBleHandler->cancel();
        mBleHandler->await();
    }

    mClassicServer->Close();
    mBleServer->Close();
    mRootcanal->linkClassicServer()->remove(mClassicServer);
    mRootcanal->linkBleServer()->remove(mBleServer);
}

void PhyConnectionClient::connectLinkLayerClassic() {
    mClassicHandler = std::make_unique<ClassicLinkForwarder>(
            mClassicServer.get(), mClient.newContext());
    mClassicHandler->startCall(mStub.get());
}

void PhyConnectionClient::connectLinkLayerBle() {
    auto stub = mClient.stub<
            android::emulation::bluetooth::EmulatedBluetoothService>();
    mBleHandler = std::make_unique<BleLinkForwarder>(mBleServer.get(),
                                                     mClient.newContext());
    mBleHandler->startCall(mStub.get());
}

std::string PhyConnectionClient::peer() {
    return mClient.address();
}

PhyConnectionClient::LinkLayerConnections
PhyConnectionClient::connectToAllEmulators(Rootcanal* rootcanal,
                                           base::Looper* looper) {
    std::vector<std::unique_ptr<PhyConnectionClient>> clients;
    auto emulators = EmulatorAdvertisement({}).discoverRunningEmulators();
    for (const auto& discovered : emulators) {
        auto client = std::make_unique<PhyConnectionClient>(
                EmulatorGrpcClient(discovered), rootcanal, looper);
        dinfo("Connecting to bluetooth mesh: %s", client->peer().c_str());
        client->connectLinkLayerBle();
        client->connectLinkLayerClassic();
        clients.push_back(std::move(client));

        // todo: Once b/226154705 is resolved connect to all emulators again.
        break;
    }
    return clients;
}

}  // namespace bluetooth
}  // namespace emulation
}  // namespace android