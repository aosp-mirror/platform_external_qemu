
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
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "PacketStreamTransport.h"
#include "aemu/base/logging/Log.h"
#include "android-qemu2-glue/netsim/PacketProtocol.h"
#include "android/avd/info.h"
#include "android/console.h"
#include "android/grpc/utils/EnumTranslate.h"
#include "android/utils/debug.h"
#include "h4_parser.h"
#include "netsim.h"
#include "netsim/common.pb.h"
#include "netsim/hci_packet.pb.h"
#include "netsim/packet_streamer.pb.h"
#include "netsim/startup.pb.h"

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
        printf("bluetooth %s:", __func__);              \
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

using netsim::packet::HCIPacket;

static void apply_le_workaround_if_needed(ChipInfo* info) {
    // We have some older Google Play store images, that we cannot
    // update, that rely on particular bluetooth hardware behavior.
    // See also b/293901745, b/288717403
    auto build =
            avdInfo_getBuildProperties(getConsoleAgents()->settings->avdInfo());
    std::string props((char*)build->data, build->size);
    auto idx = props.find("ro.build.version.sdk=33");
    if (idx != std::string::npos) {
        VERBOSE_INFO(bluetooth,
                     "Explicitly disabling le_connected_isochronous stream "
                     "feature!");
        info->mutable_chip()
                ->mutable_bt_properties()
                ->mutable_features()
                ->set_le_connected_isochronous_stream(false);
    } else {
        VERBOSE_INFO(bluetooth, "Not applying mutable features.");
    }
}

// The protocol for /dev/vhci <-> PacketStreamer
class BluetoothPacketProtocol : public PacketProtocol {
public:
    BluetoothPacketProtocol(std::shared_ptr<DeviceInfo> deviceInfo)
        : mH4Parser([this](auto data) { enqueue(HCIPacket::COMMAND, data); },
                    [this](auto data) { enqueue(HCIPacket::EVENT, data); },
                    [this](auto data) { enqueue(HCIPacket::ACL, data); },
                    [this](auto data) { enqueue(HCIPacket::SCO, data); },
                    [this](auto data) { enqueue(HCIPacket::ISO, data); }),
          mDeviceInfo(deviceInfo) {}

    std::unique_ptr<ChipInfo> chip_info() override {
        auto info = std::make_unique<ChipInfo>();
        info->set_name(mDeviceInfo->name());
        info->mutable_chip()->set_kind(netsim::common::ChipKind::BLUETOOTH);
        info->mutable_device_info()->CopyFrom(*mDeviceInfo);
        apply_le_workaround_if_needed(info.get());
        return info;
    }

    virtual void forwardToQemu(const PacketResponse* received,
                               byte_writer toStream) override {
        assert(received->has_hci_packet());
        auto packet = received->hci_packet();
        // Unpack and write out.
        uint8_t type = static_cast<uint8_t>(
                EnumTranslate::translate(mPacketType, packet.packet_type()));
        toStream(&type, 1);
        toStream((uint8_t*)packet.packet().data(), packet.packet().size());
    }

    virtual std::vector<PacketRequest> forwardToPacketStreamer(
            const uint8_t* buffer,
            size_t len) override {
        uint64_t left = len;

        // This will fill up our mPacketQueue queue through the mH4Parser..
        while (left > 0) {
            uint64_t send =
                    std::min<uint64_t>(left, mH4Parser.BytesRequested());
            DD("Consuming %d bytes", send);
            mH4Parser.Consume(buffer, send);
            buffer += send;
            left -= send;
        }

        // Return all the packets, cleaning out our internal storage.
        return std::move(mPacketQueue);
    }

    virtual std::vector<PacketRequest> connectionStateChange(
            ConnectionStateChange state,
            byte_writer toQemu) override {
        std::vector<PacketRequest> response;

        switch (state) {
            case ConnectionStateChange::QEMU_CONNECTED:
                // Reset android stack
                toQemu((uint8_t*)reset_sequence, sizeof(reset_sequence));
                break;
            case ConnectionStateChange::REMOTE_CONNECTED: {
                // Reset android stack.
                toQemu((uint8_t*)reset_sequence, sizeof(reset_sequence));

                // Registration packet.
                PacketRequest registration;
                registration.set_allocated_initial_info(chip_info().release());
                response.push_back(registration);
                return response;
            }
            case ConnectionStateChange::QEMU_DISCONNECTED:
            case ConnectionStateChange::REMOTE_DISCONNECTED:
                // Clean out our parser.
                mH4Parser.Reset();
                break;
        }

        return response;
    }

private:
    // Internal representation.
    enum class PacketType : uint8_t {
        UNKNOWN = 0,
        COMMAND = 1,
        ACL = 2,
        SCO = 3,
        EVENT = 4,
        ISO = 5,
    };

    // Adds this packet to the queue.
    void enqueue(HCIPacket::PacketType type, const std::vector<uint8_t>& data) {
        PacketRequest request;
        auto packet = request.mutable_hci_packet();
        packet->set_packet_type(type);
        packet->set_packet(std::string(data.begin(), data.end()));
        mPacketQueue.push_back(request);
    }

    // This is a HCI Hardware Error Event, which indicates a serious problem
    // with the Bluetooth hardware. As a result, the Bluetooth stack should
    // crash and restart.
    static const uint8_t constexpr reset_sequence[] = {0x04, 0x10, 0x01, 0x42};

    // External <-> Internal mapping. This makes sure the external gRPC
    // representation is independent of what is used internally.
    static constexpr const std::tuple<HCIPacket::PacketType, PacketType>
            mPacketType[] = {
                    {HCIPacket::HCI_PACKET_UNSPECIFIED, PacketType::UNKNOWN},
                    {HCIPacket::COMMAND, PacketType::COMMAND},
                    {HCIPacket::ACL, PacketType::ACL},
                    {HCIPacket::SCO, PacketType::SCO},
                    {HCIPacket::EVENT, PacketType::EVENT},
                    {HCIPacket::ISO, PacketType::ISO},
            };

    std::vector<PacketRequest> mPacketQueue;
    std::shared_ptr<DeviceInfo> mDeviceInfo;
    rootcanal::H4Parser mH4Parser;
};

std::unique_ptr<PacketProtocol> getBluetoothPacketProtocol(
        std::string deviceType,
        std::shared_ptr<DeviceInfo> deviceInfo) {
    return std::make_unique<BluetoothPacketProtocol>(deviceInfo);
}

}  // namespace qemu2
}  // namespace android
