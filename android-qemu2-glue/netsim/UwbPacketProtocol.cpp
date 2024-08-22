
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
#include "netsim.h"
#include "netsim/common.pb.h"
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
        printf("uwb %s:", __func__);              \
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

// The protocol for /dev/hvc2 <-> PacketStreamer
class UwbPacketProtocol : public PacketProtocol {
public:
    UwbPacketProtocol(std::shared_ptr<DeviceInfo> device_info)
        : mDeviceInfo(device_info) {}

    std::unique_ptr<ChipInfo> chip_info() override {
        auto info = std::make_unique<ChipInfo>();
        info->set_name(mDeviceInfo->name());
        info->mutable_chip()->set_kind(netsim::common::ChipKind::UWB);
        info->mutable_device_info()->CopyFrom(*mDeviceInfo);
        return info;
    }

    virtual void forwardToQemu(const PacketResponse* received,
                               byte_writer toStream) override {
        auto packet = received->packet();
        toStream((uint8_t*)packet.data(), packet.size());
    }

    virtual std::vector<PacketRequest> forwardToPacketStreamer(
            const uint8_t* buffer,
            size_t len) override {
        uint64_t left = len;

        while (left > 0) {
            uint64_t send = std::min<uint64_t>(left, mUwbParser.BytesRequested());
            mUwbParser.Consume(buffer, send);
            buffer += send;
            left -= send;
        }
        return mUwbParser.getPacketQueue();
    }

    virtual std::vector<PacketRequest> connectionStateChange(
            ConnectionStateChange state,
            byte_writer toQemu) override {
        std::vector<PacketRequest> response;

        if (state == ConnectionStateChange::REMOTE_CONNECTED) {
            // Registration packet.
            PacketRequest registration;
            registration.set_allocated_initial_info(chip_info().release());
            response.push_back(registration);
            return response;
        }
        return response;
    }

private:
    std::shared_ptr<DeviceInfo> mDeviceInfo;
    class UwbParser {
        public:
            UwbParser() {}
            std::vector<PacketRequest> getPacketQueue() {
                return std::move(mPacketQueue);
            }

            bool Consume(const uint8_t* buffer, int32_t bytes_read) {
                if (bytes_read <= 0) {
                    LOG(INFO) << "remote disconnected, or unhandled error?";
                    return false;
                }
                size_t bytes_to_read = BytesRequested();
                if ((uint32_t)bytes_read > bytes_to_read) {
                    LOG(FATAL) << "Uwb: More bytes read than expected";
                }

                mPacket.insert(mPacket.end(), buffer, buffer + bytes_read);
                mBytesWanted -= bytes_read;

                if (mBytesWanted == 0) {
                    switch (state_) {
                        case UCI_HEADER:
                            mBytesWanted = mPacket.at(UCI_PAYLOAD_LENGTH_FIELD);
                            state_ = UCI_PAYLOAD;
                            if (mBytesWanted > 0) {
                                break;
                            }
                            // Fall through if entire packet is just a header
                        case UCI_PAYLOAD:
                            PacketRequest request;
                            request.set_allocated_packet(new std::string(mPacket.begin(), mPacket.end()));
                            mPacketQueue.push_back(request);
                            mPacket.clear();
                            mBytesWanted = UCI_HEADER_SIZE;
                            state_ = UCI_HEADER;
                            break;
                    }
                }
                return true;
            }

            size_t BytesRequested() {
                return mBytesWanted;
            }

        private:
            const size_t UCI_HEADER_SIZE = 4;
            const size_t UCI_PAYLOAD_LENGTH_FIELD = 3;
            enum State { UCI_HEADER, UCI_PAYLOAD };

            State state_{UCI_HEADER};
            std::vector<uint8_t> mPacket;
            size_t mBytesWanted{UCI_HEADER_SIZE};
            std::vector<PacketRequest> mPacketQueue;
    };
    UwbParser mUwbParser;
};


std::unique_ptr<PacketProtocol> getUwbPacketProtocol(
        std::string deviceType,
        std::shared_ptr<DeviceInfo> deviceInfo) {
    return std::make_unique<UwbPacketProtocol>(deviceInfo);
}

}  // namespace qemu2
}  // namespace android
