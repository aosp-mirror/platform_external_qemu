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
#include <cstdint>
#include <fstream>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

// This files need to be included before EmulatedBluetoothService.h
// because of windows winnt.h header issues.
#include "model/hci/hci_sniffer.h"

#include <grpcpp/grpcpp.h>
#include "android/emulation/bluetooth/EmulatedBluetoothService.h"
#include "aemu/base/files/PathUtils.h"
#include "aemu/base/files/ScopedFd.h"
#include "android/base/system/System.h"
#include "net/multi_datachannel_server.h"
#include "android/emulation/ConfigDirs.h"
#include "android/emulation/bluetooth/GrpcLinkChannelServer.h"
#include "android/utils/debug.h"
#include "android/utils/path.h"
#include "android/utils/debug.h"
#include "emulated_bluetooth.pb.h"
#include "emulated_bluetooth_device.pb.h"
#include "emulated_bluetooth_packets.pb.h"
#include "model/hci/hci_transport.h"
#include "os/log.h"
#include "root_canal_qemu.h"

#ifdef DISABLE_ASYNC_GRPC
#include "android/grpc/utils/SyncToAsyncAdapter.h"
#else
#include "android/grpc/utils/SimpleAsyncGrpc.h"
#endif

using android::base::System;
using android::bluetooth::Rootcanal;

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
class HCIForwarder : public SimpleServerBidiStream<HCIPacket, HCIPacket>,
                     public rootcanal::HciTransport {
public:
    ~HCIForwarder() {}

    static HCIForwarder* Create(android::bluetooth::Rootcanal* rootcanal,
                                grpc::ServerContextBase* context) {
        auto pointer = std::shared_ptr<HCIForwarder>(new HCIForwarder());
        pointer->mSelf = pointer;
        std::shared_ptr<rootcanal::HciTransport> transport = pointer;

        if (VERBOSE_CHECK(bluetooth)) {
            auto stream =
                    android::bluetooth::getLogstream(context->peer() + ".pcap");
            transport = rootcanal::HciSniffer::Create(transport, stream);
        }
        rootcanal->addHciConnection(transport);

        // It's safe to let the smart pointer be destructed here as we hold
        // another reference inside the class
        return pointer.get();
    }

    void SendEvent(const std::vector<uint8_t>& data) override {
        HCIPacket packet;
        packet.set_type(HCIPacket::PACKET_TYPE_EVENT);
        DD("Send dump");
        DD_BUF(data.begin(), data.size());
        packet.set_packet(std::string(data.begin(), data.end()));
        DD_BUF(packet.packet().data(), data.size());
        DD("Sending %s -> ", packet.ShortDebugString().c_str());
        this->Write(packet);
    }

    void SendAcl(const std::vector<uint8_t>& data) override {
        HCIPacket packet;
        packet.set_type(HCIPacket::PACKET_TYPE_ACL);
        packet.set_packet(std::string(data.begin(), data.end()));
        DD("Sending %s -> ", packet.ShortDebugString().c_str());
        this->Write(packet);
    }

    void SendSco(const std::vector<uint8_t>& data) override {
        HCIPacket packet;
        packet.set_type(HCIPacket::PACKET_TYPE_SCO);
        packet.set_packet(std::string(data.begin(), data.end()));
        DD("Sending %s -> ", packet.ShortDebugString().c_str());
        this->Write(packet);
    }

    void SendIso(const std::vector<uint8_t>& data) override {
        HCIPacket packet;
        packet.set_type(HCIPacket::PACKET_TYPE_ISO);
        packet.set_packet(std::string(data.begin(), data.end()));
        DD("Sending %s -> ", packet.ShortDebugString().c_str());
        this->Write(packet);
    }

    void RegisterCallbacks(rootcanal::PacketCallback commandCallback,
                           rootcanal::PacketCallback aclCallback,
                           rootcanal::PacketCallback scoCallback,
                           rootcanal::PacketCallback isoCallback,
                           rootcanal::CloseCallback closeCallback) override {
        mCommandCallback = commandCallback;
        mAclCallback = aclCallback;
        mScoCallback = scoCallback;
        mIsoCallback = isoCallback;
        mCloseCallback = closeCallback;
    }

    void Tick() override {}

    void Close() override {
        const std::lock_guard<std::recursive_mutex> lock(mClosingMutex);
        if (!mClosed) {
            Finish(grpc::Status::CANCELLED);
        }
    }

    void Read(const HCIPacket* packet) override {
        DD("Recv: %s", packet->ShortDebugString().c_str());

        auto data = std::make_shared<std::vector<uint8_t>>(
                packet->packet().begin(), packet->packet().end());

        switch (packet->type()) {
            case HCIPacket::PACKET_TYPE_HCI_COMMAND:
                mCommandCallback(data);
                break;
            case HCIPacket::PACKET_TYPE_ACL:
                mAclCallback(data);
                break;
            case HCIPacket::PACKET_TYPE_SCO:
                mScoCallback(data);
                break;
            case HCIPacket::PACKET_TYPE_ISO:
                mIsoCallback(data);
                break;
            case HCIPacket::PACKET_TYPE_EVENT:
                DD("Ignoring event packet.");
                break;
            default:
                DD("Ignoring unknown packet.");
                break;
        }
    }

    void OnDone() override {
        const std::lock_guard<std::recursive_mutex> lock(mClosingMutex);
        mClosed = true;
        if (mCloseCallback) {
            mCloseCallback();
            mCloseCallback = nullptr;
        }

        // Decrease smart pointer references count
        mSelf.reset();
    }

private:
    HCIForwarder() {}
    rootcanal::PacketCallback mAclCallback;

    std::shared_ptr<HCIForwarder> mSelf;
    rootcanal::PacketCallback mCommandCallback;
    rootcanal::PacketCallback mScoCallback;
    rootcanal::PacketCallback mIsoCallback;
    rootcanal::CloseCallback mCloseCallback;
    std::recursive_mutex mClosingMutex;
    bool mClosed;
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
    std::recursive_mutex mClosingMutex;
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
        return BidiRunner<HCIForwarder>(
                       stream, HCIForwarder::Create(mRootcanal, context))
                .status();
    }
};
#else
class AsyncEmulatedBluetoothServiceImpl
    : public EmulatedBluetoothService::WithCallbackMethod_registerBlePhy<
              EmulatedBluetoothService::WithCallbackMethod_registerClassicPhy<
                      EmulatedBluetoothService::
                              WithCallbackMethod_registerHCIDevice<
                                      EmulatedBluetoothService::Service>>>,
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
            ::grpc::CallbackServerContext* context) {
        return HCIForwarder::Create(mRootcanal, context);
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

        path_mkdir_if_needed(tmp_dir.c_str(), 0700);
        // Get temporary file path
        std::string temp_filename_pattern = "nimble_device_XXXXXX";
        std::string temp_file_path =
                android::base::PathUtils::join(tmp_dir, temp_filename_pattern);

        auto tmpfd =
                android::base::ScopedFd(mkstemp((char*)temp_file_path.data()));
        if (!tmpfd.valid()) {
            return ::grpc::Status(
                    ::grpc::StatusCode::INTERNAL,
                    "Unable to create temporary file: " + temp_file_path +
                            " with device description",
                    "");
        }

        std::ofstream out(
                android::base::PathUtils::asUnicodePath(temp_file_path.data()).c_str(),
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
