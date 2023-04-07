// Copyright (C) 2022 The Android Open Source Project
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
// limitations under the License
#include "android/emulation/bluetooth/HciAsyncDataChannelAdapter.h"

#include <stddef.h>                         // for size_t
#include <stdint.h>                         // for uint8_t
#include <string>                           // for string
#include <type_traits>                      // for remove_extent_t

#include "emulated_bluetooth_packets.pb.h"  // for HCIPacket, HCIPacket::PAC...
#include "model/hci/h4_parser.h"            // for PacketType, ClientDisconn...
#include "model/hci/hci_protocol.h"         // for PacketReadCallback

// #define DEBUG 1
/* set  for very verbose debugging */
#ifndef DEBUG
#define DD(...) (void)0
#else
#define DD(...) dinfo(__VA_ARGS__)
#endif
namespace android {
namespace net {
class AsyncDataChannel;
}  // namespace net

namespace emulation {
namespace bluetooth {
HciAsyncDataChannelAdapter::HciAsyncDataChannelAdapter(
        std::shared_ptr<AsyncDataChannel> inner,
        ReadCallback packetReadyCallback,
        CloseCallback closeCallback)
    : mPacketReadyCallback(packetReadyCallback),
      mCloseCallback(closeCallback),
      mDataChannel(inner),
      mPacketizer(
              inner,
              [this](auto data) {
                  HCIPacket packet;
                  packet.set_type(HCIPacket::PACKET_TYPE_HCI_COMMAND);
                  packet.set_packet(std::string(data.begin(), data.end()));
                  DD("Sending %s -> ", packet.ShortDebugString().c_str());
                  mPacketReadyCallback(packet);
              },
              [this](auto data) {
                  HCIPacket packet;
                  packet.set_type(HCIPacket::PACKET_TYPE_EVENT);
                  packet.set_packet(std::string(data.begin(), data.end()));
                  DD("Sending %s -> ", packet.ShortDebugString().c_str());
                  mPacketReadyCallback(packet);
              },
              [this](auto data) {
                  HCIPacket packet;
                  packet.set_type(HCIPacket::PACKET_TYPE_ACL);
                  packet.set_packet(std::string(data.begin(), data.end()));
                  DD("Sending %s -> ", packet.ShortDebugString().c_str());
                  mPacketReadyCallback(packet);
              },
              [this](auto data) {
                  HCIPacket packet;
                  packet.set_type(HCIPacket::PACKET_TYPE_SCO);
                  packet.set_packet(std::string(data.begin(), data.end()));
                  DD("Sending %s -> ", packet.ShortDebugString().c_str());
                  mPacketReadyCallback(packet);
              },
              [this](auto data) {
                  HCIPacket packet;
                  packet.set_type(HCIPacket::PACKET_TYPE_ISO);
                  packet.set_packet(std::string(data.begin(), data.end()));
                  DD("Sending %s -> ", packet.ShortDebugString().c_str());
                  mPacketReadyCallback(packet);
              },
              [this] { mCloseCallback(this); }) {
    inner->WatchForNonBlockingRead(
            [&](auto unused) { mPacketizer.OnDataReady(mDataChannel); });
}

bool HciAsyncDataChannelAdapter::send(const HCIPacket& packet) {
    using rootcanal::PacketType;
    PacketType type;
    switch (packet.type()) {
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

    DD("Sending %s -> qemu", packet.ShortDebugString().c_str());
    size_t packetLen = packet.packet().size();
    auto bytesSend = mPacketizer.Send(
            static_cast<uint8_t>(type),
            reinterpret_cast<const uint8_t*>(packet.packet().data()),
            packetLen);
    return bytesSend == (sizeof(int) + packetLen);
}

}  // namespace bluetooth
}  // namespace emulation
}  // namespace android
