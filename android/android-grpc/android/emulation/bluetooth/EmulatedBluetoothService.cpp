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
#include <cstdint>
#include <fstream>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "android/base/files/PathUtils.h"
#include "android/base/files/ScopedFd.h"
#include "android/base/system/System.h"
#include "android/bluetooth/rootcanal/net/multi_datachannel_server.h"
#include "android/bluetooth/rootcanal/root_canal_qemu.h"
#include "android/emulation/ConfigDirs.h"
#include "android/emulation/bluetooth/BufferedAsyncDataChannelAdapter.h"
#include "android/emulation/bluetooth/GrpcLinkChannelServer.h"
#include "emulated_bluetooth.pb.h"
#include "emulated_bluetooth_device.pb.h"
#include "emulated_bluetooth_packets.pb.h"
#include "model/hci/h4_data_channel_packetizer.h"
#include "model/hci/h4_parser.h"

#ifdef DISABLE_ASYNC_GRPC
#include "android/emulation/control/utils/SyncToAsyncAdapter.h"
#else
#include "android/emulation/control/utils/SimpleAsyncGrpc.h"
#endif

using android::base::System;
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

// This marshalls HCIPackets <--> datachannel that is connected to rootcanal.
class HCIForwarder : public SimpleServerBidiStream<HCIPacket, HCIPacket> {
public:
    HCIForwarder(GrpcLinkChannelServer* server) : mChannelServer(server) {
        mRootcanalBridge = mChannelServer->createConnection();
        // todo(jansene): This might be simplified after aosp/2026506 goes in.
        mH4Parser = std::make_unique<H4Parser>(
                [this](auto data) {
                    HCIPacket packet;
                    packet.set_type(HCIPacket::PACKET_TYPE_HCI_COMMAND);
                    packet.set_packet(std::string(data.begin(), data.end()));
                    DD("Sending %s -> ", packet.ShortDebugString().c_str());
                    this->Write(packet);
                },
                [this](auto data) {
                    HCIPacket packet;
                    packet.set_type(HCIPacket::PACKET_TYPE_EVENT);
                    DD("Send dump");
                    DD_BUF(data.begin(), data.size());
                    packet.set_packet(std::string(data.begin(), data.end()));
                    DD_BUF(packet.packet().data(), data.size());
                    DD("Sending %s -> ", packet.ShortDebugString().c_str());
                    this->Write(packet);
                },
                [this](auto data) {
                    HCIPacket packet;
                    packet.set_type(HCIPacket::PACKET_TYPE_ACL);
                    packet.set_packet(std::string(data.begin(), data.end()));
                    DD("Sending %s -> ", packet.ShortDebugString().c_str());
                    this->Write(packet);
                },
                [this](auto data) {
                    HCIPacket packet;
                    packet.set_type(HCIPacket::PACKET_TYPE_SCO);
                    packet.set_packet(std::string(data.begin(), data.end()));
                    DD("Sending %s -> ", packet.ShortDebugString().c_str());
                    this->Write(packet);
                },
                [this](auto data) {
                    HCIPacket packet;
                    packet.set_type(HCIPacket::PACKET_TYPE_ISO);
                    packet.set_packet(std::string(data.begin(), data.end()));
                    DD("Sending %s -> ", packet.ShortDebugString().c_str());
                    this->Write(packet);
                });

        mRootcanalBridge->WatchForNonBlockingSend([this](auto channel) {
            do {
                ssize_t bytes_to_read = mH4Parser->BytesRequested();
                std::vector<uint8_t> buffer(bytes_to_read);
                ssize_t bytes_read = channel->ReadFromSendBuffer(buffer.data(),
                                                                 bytes_to_read);
                if (bytes_read > 0) {
                    mH4Parser->Consume(buffer.data(), bytes_read);
                }
                DD("Read %d/%d", bytes_read, bytes_to_read);
                DD_BUF(buffer.data(), bytes_read);
            } while (channel->availableInSendBuffer() > 0);
        });
    }

    ~HCIForwarder() {
        mRootcanalBridge->WatchForNonBlockingRead(nullptr);
        mRootcanalBridge->WatchForNonBlockingSend(nullptr);
        mRootcanalBridge->WatchForClose(nullptr);
    }

    void Read(const HCIPacket* packet) override {
        using rootcanal::PacketType;
        DD("Recv: %s", packet->ShortDebugString().c_str());
        PacketType type;
        switch (packet->type()) {
            case HCIPacket::PACKET_TYPE_HCI_COMMAND:
                type = PacketType::COMMAND;
                break;
            case HCIPacket::PACKET_TYPE_ACL:
                type = PacketType::ACL;
                break;
            case HCIPacket::PACKET_TYPE_SCO:
                type = PacketType::SCO;
                break;
            case HCIPacket::PACKET_TYPE_ISO:
                type = PacketType::ISO;
                break;
            case HCIPacket::PACKET_TYPE_EVENT:
                type = PacketType::EVENT;
            default:
                DD("Ignoring unknown packet.");
                break;
        }

        // todo(jansene): This might be simplified after aosp/2026506 goes in.
        mRootcanalBridge->WriteToRecvBuffer((uint8_t*)&type, 1);
        mRootcanalBridge->WriteToRecvBuffer((uint8_t*)packet->packet().data(),
                                            packet->packet().size());
    }

    void OnDone() override {
        DD("Completed");
        delete this;
    }

private:
    std::shared_ptr<BufferedAsyncDataChannel> mRootcanalBridge;
    std::unique_ptr<rootcanal::H4DataChannelPacketizer> mPacketizer;
    GrpcLinkChannelServer* mChannelServer;
    std::unique_ptr<H4Parser> mH4Parser;
};

class EmulatedBluetoothServiceBase {
public:
    EmulatedBluetoothServiceBase(android::bluetooth::Rootcanal* rootcanal,
                                 Looper* looper)
        : mRootcanal(rootcanal) {
        mClassicServer = std::make_shared<GrpcLinkChannelServer>(looper);
        mBleServer = std::make_shared<GrpcLinkChannelServer>(looper);
        mHciServer = std::make_shared<GrpcLinkChannelServer>(looper);

        mRootcanal->linkClassicServer()->add(mClassicServer);
        mRootcanal->linkBleServer()->add(mBleServer);
        mRootcanal->hciServer()->add(mHciServer);
    }

    ~EmulatedBluetoothServiceBase() {
        DD("Closing down servers.");
        mRootcanal->linkClassicServer()->remove(mClassicServer);
        mRootcanal->linkBleServer()->remove(mBleServer);
        mRootcanal->hciServer()->remove(mHciServer);
    }

protected:
    std::shared_ptr<GrpcLinkChannelServer> mClassicServer;
    std::shared_ptr<GrpcLinkChannelServer> mBleServer;
    std::shared_ptr<GrpcLinkChannelServer> mHciServer;
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
        return BidiRunner<PhyForwarder>(stream, mClassicServer.get()).status();
    }
    ::grpc::Status registerBlePhy(
            ::grpc::ServerContext* context,
            ::grpc::ServerReaderWriter<RawData, RawData>* stream) override {
        return BidiRunner<PhyForwarder>(stream, mBleServer.get()).status();
    }

    ::grpc::Status registerHCIDevice(
            ::grpc::ServerContext* context,
            ::grpc::ServerReaderWriter<HCIPacket, HCIPacket>* stream) override {
        return BidiRunner<HCIForwarder>(stream, mHciServer.get()).status();
    }
};
#else
class AsyncEmulatedBluetoothServiceImpl
    : public EmulatedBluetoothService::WithCallbackMethod_registerBlePhy<
              EmulatedBluetoothService::WithCallbackMethod_registerClassicPhy<
                      EmulatedBluetoothService::
                              WithCallbackMethod_registerHCIDevice<
                                      EmulatedBluetoothService::Service> > >,
      public EmulatedBluetoothServiceBase {
public:
    AsyncEmulatedBluetoothServiceImpl(android::bluetooth::Rootcanal* rootcanal,
                                      Looper* looper)
        : EmulatedBluetoothServiceBase(rootcanal, looper) {}

    virtual ::grpc::ServerBidiReactor<RawData, RawData>* registerClassicPhy(
            ::grpc::CallbackServerContext* /*context*/) {
        return new PhyForwarder(mClassicServer.get());
    }

    virtual ::grpc::ServerBidiReactor<RawData, RawData>* registerBlePhy(
            ::grpc::CallbackServerContext* /*context*/) {
        return new PhyForwarder(mBleServer.get());
    }

    virtual ::grpc::ServerBidiReactor<HCIPacket, HCIPacket>* registerHCIDevice(
            ::grpc::CallbackServerContext* /*context*/) {
        return new HCIForwarder(mHciServer.get());
    }

    ::grpc::Status registerGattDevice(
            ::grpc::ServerContext* ctx,
            const ::android::emulation::bluetooth::GattDevice* request,
            ::android::emulation::bluetooth::RegistrationStatus* response)
            override {
        const std::string kNimbleBridgeExe = "nimble_bridge";
        std::string executable =
                System::get()->findBundledExecutable(kNimbleBridgeExe);

        // No nimble bridge?
        if (executable.empty()) {
            return ::grpc::Status(::grpc::StatusCode::INTERNAL,
                                  "Nimble bridge executable missing", "");
        }

        auto tmp_dir = android::base::PathUtils::join(
                android::ConfigDirs::getDiscoveryDirectory(),
                std::to_string(
                        android::base::System::get()->getCurrentProcessId()));

        // Get temporary file path
        std::string temp_filename_pattern = "nimble_device_XXXXXX";
        std::string temp_file_path =
                android::base::PathUtils::join(tmp_dir, temp_filename_pattern);

        auto tmpfd =
                android::base::ScopedFd(mkstemp((char*)temp_file_path.data()));
        if (!tmpfd.valid()) {
            return ::grpc::Status(
                    ::grpc::StatusCode::INTERNAL,
                    "Unalbe to create temporary file with device description",
                    "");
        }

        std::ofstream out(
                android::base::PathUtils::asUnicodePath(temp_file_path).c_str(),
                std::ios::out | std::ios::binary);
        request->SerializeToOstream(&out);
        std::vector<std::string> cmdArgs{executable, "--with_device_proto",
                                         temp_file_path};

        System::Pid bridgePid;
        if (!System::get()->runCommand(cmdArgs, base::RunOptions::DontWait,
                                       System::kInfinite, nullptr,
                                       &bridgePid)) {
            return ::grpc::Status(::grpc::StatusCode::INTERNAL,
                                  "Failed to launch nimble bridge", "");
        }

        response->mutable_callback_device_id()->set_identity(
                std::to_string(bridgePid));

        VERBOSE_INFO(bluetooth, "Launched nimble bridge with %s",
                     temp_file_path.c_str());
        return ::grpc::Status::OK;
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