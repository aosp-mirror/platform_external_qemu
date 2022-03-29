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
#include "android/emulation/bluetooth/EmulatedBluetoothService.h"

#include <grpcpp/grpcpp.h>
#include <stdint.h>
#include <string>
#include <type_traits>
#include <utility>

#include "android/bluetooth/rootcanal/net/multi_datachannel_server.h"
#include "android/bluetooth/rootcanal/root_canal_qemu.h"
#include "android/emulation/bluetooth/BufferedAsyncDataChannelAdapter.h"
#include "android/emulation/bluetooth/GrpcLinkChannelServer.h"
#include "android/emulation/bluetooth/HciAsyncDataChannelAdapter.h"
#include "android/utils/debug.h"
#include "emulated_bluetooth.pb.h"


#ifdef DISABLE_ASYNC_GRPC
#include "android/emulation/control/utils/SyncToAsyncAdapter.h"
#else
#include "android/emulation/control/utils/SimpleAsyncGrpc.h"
#endif

using android::bluetooth::Rootcanal;
using rootcanal::H4DataChannelPacketizer;
using rootcanal::H4Parser;

#define DEBUG 0
/* set  >1 for very verbose debugging */
#if DEBUG <= 1
#define DD(...) (void)0
#define DD_BUF(buf, len) (void)0
#else
#define DD(...) dinfo(__VA_ARGS__)
#define DD_BUF(buf, len)                                \
    do {                                                \
        printf("Channel %s (%p):", __func__, this);     \
        for (int x = 0; x < len; x++) {                 \
            if (isprint((int)buf[x]))                   \
                printf("%c", buf[x]);                   \
            else                                        \
                printf("[0x%02x]", 0xff & (int)buf[x]); \
        }                                               \
        printf("\n");                                   \
    } while (0)

#endif

namespace android {
namespace emulation {
namespace bluetooth {

class PhyForwarder : public SimpleServerBidiStream<RawData, RawData> {
public:
    PhyForwarder(GrpcLinkChannelServer* server) : mChannelServer(server) {
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
                DD("Refusing to send empty packet..");
                return;
            }
            packetBuffer.resize(toForward);

            dataChannel->ReadFromSendBuffer((uint8_t*)packetBuffer.data(),
                                            toForward);
            packet.set_packet(std::move(packetBuffer));
            DD("Send: %s", packet.ShortDebugString().c_str());
            this->Write(packet);
        });

        mRootcanalBridge->WatchForClose([this](auto theRootCanalBridge) {
            Finish(grpc::Status::CANCELLED);
        });
    }

    ~PhyForwarder() {
        mRootcanalBridge->WatchForNonBlockingRead(nullptr);
        mRootcanalBridge->WatchForNonBlockingSend(nullptr);
        mRootcanalBridge->WatchForClose(nullptr);
    }

    void Read(const RawData* received) override {
        DD("Recv: %s", received->ShortDebugString().c_str());
        mRootcanalBridge->WriteToRecvBuffer((uint8_t*)received->packet().data(),
                                            received->packet().size());
    }

    void OnDone() override {
        DD("Completed");
        delete this;
    }

private:
    std::shared_ptr<BufferedAsyncDataChannel> mRootcanalBridge;
    GrpcLinkChannelServer* mChannelServer;
};

class EmulatedBluetoothServiceBase {
public:
    EmulatedBluetoothServiceBase(android::bluetooth::Rootcanal* rootcanal,
                                 Looper* looper)
        : mRootcanal(rootcanal) {
        mClassicServer = std::make_shared<GrpcLinkChannelServer>(looper);
        mBleServer = std::make_shared<GrpcLinkChannelServer>(looper);

        mRootcanal->linkClassicServer()->add(mClassicServer);
        mRootcanal->linkBleServer()->add(mBleServer);
    }

    ~EmulatedBluetoothServiceBase() {
        DD("Closing down servers.");
        mRootcanal->linkClassicServer()->remove(mClassicServer);
        mRootcanal->linkBleServer()->remove(mBleServer);
    }

protected:
    std::shared_ptr<GrpcLinkChannelServer> mClassicServer;
    std::shared_ptr<GrpcLinkChannelServer> mBleServer;
    android::bluetooth::Rootcanal* mRootcanal;
};

#ifdef DISABLE_ASYNC_GRPC
class AsyncEmulatedBluetoothServiceImpl
    : public EmulatedBluetoothService::Service,
      public EmulatedBluetoothServiceBase {
public:
    AsyncEmulatedBluetoothServiceImpl(android::bluetooth::Rootcanal* rootcanal,
                                      Looper* looper)
        : EmulatedBluetoothServiceBase(rootcanal, looper) {}

    ::grpc::Status registerClassicPhy(
            ::grpc::ServerContext* context,
            ::grpc::ServerReaderWriter<RawData, RawData>* stream) override {
        return BidiRunner<PhyForwarder>(stream, mClassicServer.get())
                .status();
    }
    ::grpc::Status registerBlePhy(
            ::grpc::ServerContext* context,
            ::grpc::ServerReaderWriter<RawData, RawData>* stream) override {
        return BidiRunner<PhyForwarder>(stream, mBleServer.get())
                .status();
    }
};
#else
class AsyncEmulatedBluetoothServiceImpl
    : public EmulatedBluetoothService::CallbackService,
      public EmulatedBluetoothServiceBase {
public:
    AsyncEmulatedBluetoothServiceImpl(android::bluetooth::Rootcanal* rootcanal,
                                      Looper* looper)
        : EmulatedBluetoothServiceBase(rootcanal, looper) {}

    virtual ::grpc::ServerBidiReactor<RawData, RawData>*
    registerClassicPhy(::grpc::CallbackServerContext* /*context*/) {
        return new PhyForwarder(mClassicServer.get());
    }

    virtual ::grpc::ServerBidiReactor<RawData, RawData>* registerBlePhy(
            ::grpc::CallbackServerContext* /*context*/) {
        return new PhyForwarder(mBleServer.get());
    }

};
#endif

EmulatedBluetoothService::Service* getEmulatedBluetoothService(
        android::bluetooth::Rootcanal* rootcanal,
        base::Looper* looper) {
    return new AsyncEmulatedBluetoothServiceImpl(rootcanal, looper);
}
}  // namespace bluetooth
}  // namespace emulation
}  // namespace android