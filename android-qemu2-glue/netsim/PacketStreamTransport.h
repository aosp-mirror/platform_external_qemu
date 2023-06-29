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
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "PacketProtocol.h"
#include "android/grpc/utils/SimpleAsyncGrpc.h"
#include "grpcpp/grpcpp.h"
#include "netsim/packet_streamer.grpc.pb.h"
#include "netsim/packet_streamer.pb.h"

// clang-format off
// IWYU pragma: begin_keep
extern "C" {
// #include "qemu/typedefs.h"
#include "qemu/osdep.h"
#include "chardev/char.h"
}
// IWYU pragma: end_keep
// clang-format on

namespace android {
namespace qemu2 {

class PacketStreamerTransport;

typedef struct PacketStreamChardev {
    Chardev parent;
    std::weak_ptr<PacketStreamerTransport> transport;
} PacketStreamChardev;

using netsim::packet::PacketRequest;
using netsim::packet::PacketResponse;
using netsim::packet::PacketStreamer;

// A ChannelFactor can produce a new grpc channel to a PacketStreamer service
// It should:
//
//    - Return an open channel. It's okay if this channel fails sometimes (soft
//    failures)
//    - Return nullptr it will never be able to produce an open channel (hard
//    failure)
using ChannelFactory = std::function<std::shared_ptr<::grpc::Channel>()>;

// A PacketStreamerTransport manages the connection between a remote packet
// streamer and qemu. Note that the transport uses async i/o.
//
// It does the following:
// - Manage the connection with qemu (connect/disconnect)
// - Manage the connection with the packet streamer (connect/disconnect)
//
// It will use a PacketProtocol to translate between qemu <-> packet streamer
// packets.
class PacketStreamerTransport
    : public SimpleClientBidiStream<PacketResponse, PacketRequest>,
      public std::enable_shared_from_this<PacketStreamerTransport> {
public:
    virtual ~PacketStreamerTransport();

    // Callback async incoming packet
    void Read(const PacketResponse* received) override;

    // Callback gRPC async completion
    void OnDone(const grpc::Status& s) override;

    // Called when we should terminate the connection, this will result in
    // reconnect() returning false.
    void cancel();

    // Called when bytes are coming in from qemu.
    uint64_t fromQemu(const uint8_t* buffer, uint64_t bufferSize);

    // Called when qemu has become available, the guest has now become active
    // and is in a "blank" state.
    void qemuConnected();

    // Blocks and waits until this connection has completed,
    // returns true if we should reconnect.
    bool await();

    // Remote peer, becomes more detailed once initial packets
    // have gone over the wire.
    std::string peer() { return mContext.peer(); }

    // True if we should attempt to reconnect when the packet streamer fails.
    bool reconnect() { return mReconnect; }

    // Called when you are ready to start the stream, this will obtain a self
    // reference. It will start the protocol with the remote service.
    void start();

    /**
     * @brief Creates a PacketStreamerTransport object.
     *
     * @param qemuDevice The QEMU device. This is chardev device that
     *     is connected to the guest over a virtual socket.
     * @param protocol The packet protocol to use. This is the actual
     *     protocol used to translate between qemu bytes and the packet
     *     streamer service .
     * @param open_channel A GRPC channel to use to communicate with the
     *     PacketStreamer server. It should be open, if it is not a new
     *     channel will be created.
     * @param factory The factory to use to create a channel to the remote
     *     PacketStreamer service. The factory will be used in case the
     *     transport needs to be re-established. The factory should produce
     *     a valid connection, or a nullptr in case it will never be able to do
     * so.
     *
     * @return A pointer to a PacketStreamerTransport object.
     */
    static std::shared_ptr<PacketStreamerTransport> create(
            PacketStreamChardev* qemuDevice,
            std::unique_ptr<PacketProtocol> protocol,
            std::shared_ptr<grpc::Channel> open_channel,
            ChannelFactory factory);

private:
    PacketStreamerTransport(PacketStreamChardev* qemuDevice,
                            std::unique_ptr<PacketProtocol> serializer,
                            std::shared_ptr<grpc::Channel> channel,
                            ChannelFactory factory);

    void writePackets(const std::vector<PacketRequest>& requests);

    grpc::ClientContext mContext;
    PacketStreamChardev* mQemudevice;
    ChannelFactory mChannelFactory;

    std::unique_ptr<PacketStreamer::Stub> mStub;
    std::unique_ptr<PacketProtocol> mSerializer;

    // Self reference, erased when onDone is called.
    std::shared_ptr<PacketStreamerTransport> mMe;

    byte_writer mQemuDeviceWriter;

    // Finishing
    bool mReconnect{true};
    bool mDone{false};

    // Used to block and wait for connection.
    std::mutex mAwaitMutex;
    std::condition_variable mAwaitCondition;

    // Protect writer
    std::mutex mWriterMutex;
};

// Factory that can produce a packet serializer for the given device name.
std::unique_ptr<PacketProtocol> getPacketProtocol(std::string deviceType,
                                                  std::string deviceName);

}  // namespace qemu2
}  // namespace android
