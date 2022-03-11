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
#pragma once
#include <memory>         // for shared_ptr
#include <mutex>          // for mutex
#include <unordered_set>  // for unordered_set

#include "android/emulation/bluetooth/BufferedAsyncDataChannelAdapter.h"
#include "net/async_data_channel_server.h"  // for AsyncDataChannelServer

namespace android {
namespace base {
class Looper;
}  // namespace base

namespace emulation {
namespace bluetooth {

// A Grpc ChannelServer is an AsyncDataChannelServer that will
// provide a BufferedAsyncDataChannel connection that will
// can be used to adapt the in and outgoing data in gRPC connections.
//
// You usually want to use this as follows:
//
// Create an Async Client or Server Reactor for gRPC, upon
// creation of your reactor you call createConnection on this object.
// When things are properly configured this should result in a connection
// event for Rootcanal, which will start using the AsyncChannel.
//
// Look at:
// android/android-grpc/android/emulation/bluetooth/EmulatedBluetoothService.cpp
// for a server side example that adapts Ble/Classic link layer connections
// Look at:
// android/android-grpc/android/emulation/bluetooth/PhyConnectionClient.cpp
// for a server clients example that adapts Ble/Classic link layer connections
class GrpcLinkChannelServer : public net::AsyncDataChannelServer {
public:
    using ChannelSet =
            std::unordered_set<std::shared_ptr<BufferedAsyncDataChannel>>;

    // Lets hope we are not sending more packets in one go than this..
    static const size_t DEFAULT_BUFFER_SIZE = 1024 * 16;

    GrpcLinkChannelServer(base::Looper* looper,
                          size_t buffer = DEFAULT_BUFFER_SIZE);
    virtual ~GrpcLinkChannelServer();

    bool StartListening() override;
    void StopListening() override;
    void Close() override;
    bool Connected() override;

    // Call this when you need a now connection object. Calling this
    // *should* fire events inside rootcanal indicating that a new
    // connection has arrived.
    std::shared_ptr<BufferedAsyncDataChannel> createConnection();

private:
    bool mListening{false};
    size_t mBufferSize;
    bool mOpen{true};
    base::Looper* mLooper;
};

}  // namespace bluetooth
}  // namespace emulation
}  // namespace android