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
#pragma once
#include <vector>
#include "backend/packet_streamer_client.h"
#include "netsim/packet_streamer.grpc.pb.h"
#include "netsim/startup.pb.h"

using netsim::packet::PacketRequest;
using netsim::packet::PacketResponse;
using netsim::startup::ChipInfo;

namespace android {
namespace qemu2 {

using byte_writer = std::function<void(uint8_t*, size_t)>;

// A PacketProtocol is responsible for translating between bytes
// that come from qemu and concrete objects that are coming from the
// PacketStreamerService
//
// You basically are repsonsible for executing the streaming protocol that
// exists between the packet service and qemu.
//
// You:
//   - Must properly respond to connection state changes.
//   - Translate bytes from qemu into packets.
//   - Translate packets into bytes that can be written to qemu.
//
class PacketProtocol {
public:
    enum class ConnectionStateChange {
        QEMU_CONNECTED,       // The device just became active (snapshot, machine start)
        QEMU_DISCONNECTED,    // The device is no longer in use (shutdown)
        REMOTE_CONNECTED,     // The remote (grpc) endpoint just connected (endpoint active)
        REMOTE_DISCONNECTED,  // The remote (grpc) endpoint is gone (shutdown, disconnect)
    };

    virtual ~PacketProtocol(){};

    // The chip info that is used to identify this device with
    // the packet streamer.
    virtual std::unique_ptr<ChipInfo> chip_info() = 0;

    // Deserialize the packet response from the packet streamer.
    // In this method you need to forward the bytes by writing them to the qemu
    // stream.
    virtual void forwardToQemu(const PacketResponse* response,
                               byte_writer toQemu) = 0;

    // Serialize the incoming data from qemu. You should populate
    // a vector with requests that can be forwarded to the streamer.
    virtual std::vector<PacketRequest> forwardToPacketStreamer(
            const uint8_t* data,
            size_t len) = 0;

    // Callen when either the remote or local connection changes state.
    //
    // Make sure to return the handshake packets when you receive
    // REMOTE_CONNECTED state change.
    virtual std::vector<PacketRequest> connectionStateChange(
            ConnectionStateChange state,
            byte_writer toQemu) = 0;
};
}  // namespace qemu2
}  // namespace android
