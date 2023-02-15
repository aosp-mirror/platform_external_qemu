
// Copyright 2023 The Android Open Source Project
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
#include "PacketStreamTransport.h"

#include <stddef.h>
#include <type_traits>
#include <utility>

#include "aemu/base/logging/CLog.h"
#include "android-qemu2-glue/netsim/PacketProtocol.h"
#include "startup.pb.h"

namespace android {
namespace qemu2 {

// #define DEBUG 2
/* set  for very verbose debugging */
#ifndef DEBUG
#define DD(...) (void)0
#define DD_BUF(buf, len) (void)0
#else
#define DD(...) dinfo(__VA_ARGS__)
#define DD_BUF(buf, len)                                \
    do {                                                \
        printf("packet-stream %s:", __func__);          \
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

PacketStreamerTransport::PacketStreamerTransport(
        PacketStreamChardev* qemuDevice,
        std::unique_ptr<PacketProtocol> serializer,
        std::shared_ptr<grpc::Channel> channel,
        ChannelFactory factory)
    : mQemudevice(qemuDevice),
      mStub(PacketStreamer::NewStub(channel)),
      mSerializer(std::move(serializer)),
      mChannelFactory(factory),
      mQemuDeviceWriter([this](uint8_t* bytes, size_t size) {
          DD("Writing %d bytest to qemu", size);
          qemu_chr_be_write(&mQemudevice->parent, bytes, size);
      }) {}

PacketStreamerTransport::~PacketStreamerTransport() {}

void PacketStreamerTransport::Read(const PacketResponse* received) {
    mSerializer->forwardToQemu(received, mQemuDeviceWriter);
}

void PacketStreamerTransport::OnDone(const grpc::Status& s) {
    mSerializer->connectionStateChange(
            PacketProtocol::ConnectionStateChange::REMOTE_DISCONNECTED,
            mQemuDeviceWriter);
    mDone = true;
    mAwaitCondition.notify_one();

    if (mReconnect) {
        auto channel = mChannelFactory();

        if (!channel) {
            auto info = mSerializer->chip_info();
            dwarning("Unable to reconnect to packet streamer for device: %s",
                     info->name().c_str());
        } else {
            // replace the transport..
            DD("Replacing transport");
            auto transport = PacketStreamerTransport::create(
                    mQemudevice, std::move(mSerializer), channel,
                    mChannelFactory);
            mQemudevice->transport = transport;
            transport->start();
        }
    }

    // No longer active..
    DD("Goodbye kids! It was fun");
    mMe.reset();
}

void PacketStreamerTransport::cancel() {
    mReconnect = false;

    // This will result in the scheduling of an OnDone callback.
    // At that point we will notify the packet protocol of a state change.
    mContext.TryCancel();
}

uint64_t PacketStreamerTransport::fromQemu(const uint8_t* buffer,
                                           uint64_t bufferSize) {
    auto packets = mSerializer->forwardToPacketStreamer(buffer, bufferSize);
    writePackets(packets);
    return bufferSize;
}

void PacketStreamerTransport::qemuConnected() {
    auto packets = mSerializer->connectionStateChange(
            PacketProtocol::ConnectionStateChange::QEMU_CONNECTED,
            mQemuDeviceWriter);
    writePackets(packets);
}

bool PacketStreamerTransport::await() {
    std::unique_lock<std::mutex> l(mAwaitMutex);
    mAwaitCondition.wait(l, [this] { return mDone; });
    return mReconnect;
}

void PacketStreamerTransport::start() {
    // I'm active! Update the self reference.
    DD("Starting stream.");
    mMe = shared_from_this();

    // Setup async call
    mStub->async()->StreamPackets(&mContext, this);
    StartCall();
    StartRead();

    // Notify that we are connected, and send out the handshake packets
    // (if any)
    auto packets = mSerializer->connectionStateChange(
            PacketProtocol::ConnectionStateChange::REMOTE_CONNECTED,
            mQemuDeviceWriter);
    writePackets(packets);
    DD("Stream scheduled.");
}

void PacketStreamerTransport::writePackets(
        const std::vector<PacketRequest>& packets) {
    std::lock_guard<std::mutex> guard(mWriterMutex);
    DD("Writing packets..");
    for (const auto& packet : packets) {
        Write(packet);
    }
    DD("Completed writing packets.");
}

std::shared_ptr<PacketStreamerTransport> PacketStreamerTransport::create(
        PacketStreamChardev* qemuDevice,
        std::unique_ptr<PacketProtocol> serializer,
        std::shared_ptr<grpc::Channel> channel,
        ChannelFactory factory) {
    return std::shared_ptr<PacketStreamerTransport>(new PacketStreamerTransport(
            qemuDevice, std::move(serializer), std::move(channel),
            std::move(factory)));
}

}  // namespace qemu2
}  // namespace android