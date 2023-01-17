// Copyright 2021 The Android Open Source Project
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
#include <assert.h>
#include <algorithm>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

#include "aemu/base/logging/CLog.h"
#include "android-qemu2-glue/netsim/netsim.h"
#include "android/grpc/utils/EnumTranslate.h"
#include "android/grpc/utils/SimpleAsyncGrpc.h"
#include "android/utils/debug.h"
#include "backend/packet_streamer_client.h"
#include "grpcpp/grpcpp.h"
#include "hci_packet.pb.h"
#include "model/hci/h4_parser.h"
#include "model/hci/hci_protocol.h"
#include "packet_streamer.grpc.pb.h"
#include "startup.pb.h"

using namespace grpc;
using netsim::packet::HCIPacket;
using netsim::packet::PacketRequest;
using netsim::packet::PacketResponse;
using netsim::packet::PacketStreamer;

// clang-format off
extern "C" {
#include "qemu/osdep.h"
#include "chardev/char.h"
#include "chardev/char-fe.h"
#include "qapi/error.h"
#include "migration/vmstate.h"
#include "hw/qdev-core.h"
}
// clang-format on

#ifdef _MSC_VER
#undef send
#undef recv
#endif

// #define DEBUG 1
/* set  for very verbose debugging */
#ifndef DEBUG
#define DD(...) (void)0
#define DD_BUF(buf, len) (void)0
#else
#define DD(...) dinfo(__VA_ARGS__)
#define DD_BUF(buf, len)                                \
    do {                                                \
        printf("chardev-netsim %s:", __func__);         \
        for (int x = 0; x < len; x++) {                 \
            if (isprint((int)buf[x])) {                 \
                printf("%c", buf[x]);                   \
            } else {                                    \
                printf("[0x%02x]", 0xff & (int)buf[x]); \
            }                                           \
        }                                               \
    } while (0);                                        \
    printf("\n");
#endif

class HciTransport;

typedef struct NetsimChardev {
    Chardev parent;
    QemuMutex transportMutex;
    HciTransport* transport;
} NetsimChardev;

// Global netsim configuration.
static struct {
    std::string serial;
    std::string default_commands_file;
    std::string controller_properties_file;
} gNetsimConfiguration;

static void reset_android_bluetooth_stack(NetsimChardev* device);

#define TYPE_CHARDEV_NETSIM "chardev-netsim"
#define NETSIM_CHARDEV(obj) \
    OBJECT_CHECK(NetsimChardev, (obj), TYPE_CHARDEV_NETSIM)
#define NETSIM_DEVICE_GET_CLASS(obj) \
    OBJECT_GET_CLASS(NetsimChardev, obj, TYPE_CHARDEV_NETSIM)

class HciTransport
    : public SimpleClientBidiStream<PacketResponse, PacketRequest> {
public:
    HciTransport(NetsimChardev* netsimDevice,
                 std::shared_ptr<grpc::Channel> channel)
        : mNetsimCharDev(netsimDevice),
          mStub(PacketStreamer::NewStub(channel)),
          mH4Parser([this](auto data) { sendPacket(HCIPacket::COMMAND, data); },
                    [this](auto data) { sendPacket(HCIPacket::EVENT, data); },
                    [this](auto data) { sendPacket(HCIPacket::ACL, data); },
                    [this](auto data) { sendPacket(HCIPacket::SCO, data); },
                    [this](auto data) { sendPacket(HCIPacket::ISO, data); }) {}

    virtual ~HciTransport() {}

    enum class PacketType : uint8_t {
        UNKNOWN = 0,
        COMMAND = 1,
        ACL = 2,
        SCO = 3,
        EVENT = 4,
        ISO = 5,
    };

    void Read(const PacketResponse* received) override {
        // We should only get hci packets.
        assert(received->has_hci_packet());

        DD("From: %s, Received: %s", mContext.peer().c_str(),
           received->ShortDebugString().c_str());

        auto packet = received->hci_packet();

        // Unpack and write out.
        uint8_t type = static_cast<uint8_t>(
                EnumTranslate::translate(mPacketType, packet.packet_type()));
        qemu_chr_be_write(&mNetsimCharDev->parent, &type, 1);
        qemu_chr_be_write(&mNetsimCharDev->parent,
                          (uint8_t*)packet.packet().data(),
                          packet.packet().size());
    }

    void OnDone(const grpc::Status& s) override {
        VERBOSE_INFO(bluetooth, "Disconnected bluetooth from %s, due to %s",
                     peer().c_str(), s.error_message().c_str());
        mDone = true;
        mAwaitCondition.notify_one();
    }

    void cancel() {
        mReconnect = false;
        mContext.TryCancel();
    }

    void startCall() {
        // Setup async call
        mStub->async()->StreamPackets(&mContext, this);
        StartCall();
        StartRead();

        // And register.
        PacketRequest registration;
        registration.mutable_initial_info()->set_serial(
                gNetsimConfiguration.serial);
        registration.mutable_initial_info()->mutable_chip()->set_kind(
                netsim::startup::Chip::BLUETOOTH);
        Write(registration);

        dinfo("Bluetooth registration: %s send to netsim at: %s",
              registration.ShortDebugString().c_str(), peer().c_str());
    };

    // Parse the incoming packets from android, triggering a forward if a
    // complete packet was received.
    uint64_t fromAndroid(const uint8_t* buffer, uint64_t bufferSize) {
        DD("Received %" PRIu64 " bytes, needed: %d", bufferSize,
           mH4Parser.BytesRequested());
        uint64_t left = bufferSize;
        while (left > 0) {
            uint64_t send =
                    std::min<uint64_t>(left, mH4Parser.BytesRequested());
            DD("Consuming %d bytes", send);
            mH4Parser.Consume(buffer, send);
            buffer += send;
            left -= send;
        }
        return bufferSize;
    }

    // Blocks and waits until this connection has completed,
    // returns true if we should reconnect.
    bool await() {
        std::unique_lock<std::mutex> l(mAwaitMutex);
        mAwaitCondition.wait(l, [this] { return mDone; });
        return mReconnect;
    }

    std::string peer() { return mContext.peer(); }

    bool reconnect() { return mReconnect; }

private:
    // Forwards the packet of the given type to netsim
    void sendPacket(HCIPacket::PacketType type,
                    const std::vector<uint8_t>& data) {
        PacketRequest request;
        auto packet = request.mutable_hci_packet();
        packet->set_packet_type(type);
        packet->set_packet(std::string(data.begin(), data.end()));
        Write(request);

        DD("Async write %s -> %s", packet->ShortDebugString().c_str(),
           peer().c_str());
    }

    // External <-> Internal mapping.
    static constexpr const std::tuple<HCIPacket::PacketType, PacketType>
            mPacketType[] = {
                    {HCIPacket::HCI_PACKET_UNSPECIFIED, PacketType::UNKNOWN},
                    {HCIPacket::COMMAND, PacketType::COMMAND},
                    {HCIPacket::ACL, PacketType::ACL},
                    {HCIPacket::SCO, PacketType::SCO},
                    {HCIPacket::EVENT, PacketType::EVENT},
                    {HCIPacket::ISO, PacketType::ISO},
            };

    grpc::ClientContext mContext;
    NetsimChardev* mNetsimCharDev;
    std::unique_ptr<PacketStreamer::Stub> mStub;
    rootcanal::H4Parser mH4Parser;

    // Finishing
    bool mReconnect{true};
    bool mDone{false};
    std::mutex mAwaitMutex;
    std::condition_variable mAwaitCondition;
};

static void connect_netsim(NetsimChardev* netsim) {
    std::unique_ptr<HciTransport> transport;

    do {
        VERBOSE_INFO(bluetooth,
                     "Connecting to netsim for bluetooth emulation, "
                     "default_commands_file: %s, controller_properties_file: "
                     "%s as: %s",
                     gNetsimConfiguration.default_commands_file.c_str(),
                     gNetsimConfiguration.controller_properties_file.c_str(),
                     gNetsimConfiguration.serial.c_str());

        // TODO(jansene): Add support for connect to custom endpoint vs.
        // discovery.
        auto channel = netsim::packet::CreateChannel(
                gNetsimConfiguration.default_commands_file,
                gNetsimConfiguration.controller_properties_file);
        transport = std::make_unique<HciTransport>(netsim, channel);

        qemu_mutex_lock(&netsim->transportMutex);
        netsim->transport = transport.get();
        qemu_mutex_unlock(&netsim->transportMutex);
        transport->startCall();

        // We just established a new connection to netsim, so we have a
        // bluetooth chip in a clean state. Reset the internal android stack to
        // handle this.
        reset_android_bluetooth_stack(netsim);

        DD("Waiting for gRPC completion");
        transport->await();
    } while (transport->reconnect());

    VERBOSE_INFO(bluetooth, "Finished bluetooth emulation.");
}

static int netsim_chr_write(Chardev* chr, const uint8_t* buf, int len) {
    DD("Received %d bytes from /dev/vhci", len);
    NetsimChardev* netsim = NETSIM_CHARDEV(chr);
    int consumed = 0;

    // Make sure that we are not re-connecting right now due to netsim
    // disappearing..
    // TODO(jansene): This could be a potential concern if  netsim is not coming
    // back in a timely fashion as this will block qemu.
    qemu_mutex_lock(&netsim->transportMutex);
    consumed = netsim->transport->fromAndroid(buf, len);
    qemu_mutex_unlock(&netsim->transportMutex);

    DD("Consumed: %d", consumed);
    return consumed;
}

static void reset_android_bluetooth_stack(NetsimChardev* device) {
    VERBOSE_INFO(bluetooth, "sending reset to android bluetooth stack.");
    static uint8_t reset_sequence[] = {0x01, 0x03, 0x0c, 0x00};
    qemu_chr_be_write(&device->parent, reset_sequence, sizeof(reset_sequence));
}

static void netsim_chr_open(Chardev* chr,
                            ChardevBackend* backend,
                            bool* be_opened,
                            Error** errp) {
    std::thread netsim_thread(connect_netsim, NETSIM_CHARDEV(chr));
    netsim_thread.detach();

    *be_opened = true;
}

static void netsim_chr_cleanup(Object* o) {
    DD("Canceling bluetooth transport.");
    NetsimChardev* netsim = NETSIM_CHARDEV(o);
    qemu_mutex_lock(&netsim->transportMutex);
    netsim->transport->cancel();
    qemu_mutex_unlock(&netsim->transportMutex);
}

static int netsim_chr_machine_done_hook(Chardev* chr) {
    NetsimChardev* netsim = NETSIM_CHARDEV(chr);
    reset_android_bluetooth_stack(netsim);
    return 0;
}

static void netsim_chr_init(Object* obj) {
    NetsimChardev* netsim = NETSIM_CHARDEV(obj);
    qemu_mutex_init(&netsim->transportMutex);
    netsim->transport = nullptr;
}

static void char_netsim_class_init(ObjectClass* oc, void* data) {
    ChardevClass* cc = CHARDEV_CLASS(oc);
    cc->chr_write = netsim_chr_write;
    cc->open = netsim_chr_open;
    cc->chr_machine_done = netsim_chr_machine_done_hook;
}

static const TypeInfo char_netsim_type_info = {
        .name = TYPE_CHARDEV_NETSIM,
        .parent = TYPE_CHARDEV,
        .instance_size = sizeof(NetsimChardev),
        .instance_init = netsim_chr_init,
        .instance_finalize = netsim_chr_cleanup,
        .class_init = char_netsim_class_init,
};

void register_netsim(const std::string address,
                     const std::string rootcanal_default_commands_file,
                     const std::string rootcanal_controller_properties_file,
                     const std::string host_id) {
    if (!address.empty()) {
        dwarning(
                "Unable to connect to %s, (b/265451720) reverting to auto "
                "discovery.",
                address.c_str());
    }
    gNetsimConfiguration.serial = host_id;
    gNetsimConfiguration.default_commands_file =
            rootcanal_default_commands_file;
    gNetsimConfiguration.controller_properties_file =
            rootcanal_controller_properties_file;
}

static void register_types(void) {
    type_register_static(&char_netsim_type_info);
}

type_init(register_types);
