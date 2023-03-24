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
#pragma once
#include <functional>  // for function
#include <memory>      // for shared_ptr

#include "emulated_bluetooth_packets.pb.h"         // for HCIPacket
#include "model/hci/h4_data_channel_packetizer.h"  // for H4DataChannelP...
#include "net/async_data_channel.h"

namespace android {
namespace emulation {
namespace bluetooth {

using android::net::AsyncDataChannel;

// An adapter that will listen to a datachannel and transforms the bytes
// into HCIPacket, invoking a callback when a new packet is available.
//
// Usually the datachannel is connected to /dev/vhci on qemu.
class HciAsyncDataChannelAdapter {
public:
    using ReadCallback = std::function<void(const HCIPacket&)>;
    using CloseCallback = std::function<void(HciAsyncDataChannelAdapter*)>;

    // Registers the callback on the given datachannel. The callback will
    // be invoked when a new HCIPacket is made available.
    HciAsyncDataChannelAdapter(std::shared_ptr<AsyncDataChannel> inner,
                              ReadCallback packetReadyCallback,
                              CloseCallback closeCallback);
    ~HciAsyncDataChannelAdapter() = default;

    // Send the given HCIPacket over this datachannel (to /dev/vhci)
    bool send(const HCIPacket& toSend);

private:
    ReadCallback mPacketReadyCallback;
    CloseCallback mCloseCallback;
    rootcanal::H4DataChannelPacketizer mPacketizer;
    std::shared_ptr<AsyncDataChannel> mDataChannel;
};
}  // namespace bluetooth
}  // namespace emulation
}  // namespace android
