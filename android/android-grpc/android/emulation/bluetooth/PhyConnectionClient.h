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
#include <memory>                                                // for shar...
#include <string>                                                // for string
#include <vector>                                                // for vector

#include "android/emulation/control/utils/EmulatorGrcpClient.h"  // for Emul...
#include "emulated_bluetooth.grpc.pb.h"                          // for Emul...

namespace android {
namespace bluetooth {
class Rootcanal;
}  // namespace bluetooth

namespace base {
class Looper;
}  // namespace base

namespace emulation {
namespace bluetooth {
class GrpcLinkChannelServer;
class LinkForwarder;

using control::EmulatorGrpcClient;

// An PhyConnectionClient can establish a classic and ble
// linklayer connection over gRPC to another emulator.
//
// It will adapt the gRPC channel to the relevand DataChannels that will be
// registered within rootcanal.
class PhyConnectionClient {
public:
    using LinkLayerConnections =
            std::vector<std::unique_ptr<PhyConnectionClient>>;
    PhyConnectionClient(EmulatorGrpcClient client,
                                      android::bluetooth::Rootcanal* rootcanal,
                                      base::Looper* looper);
    ~PhyConnectionClient();

    void connectLinkLayerClassic();
    void connectLinkLayerBle();

    // The human readable address of the remote gRPC service.
    std::string peer();

    // This will discover all running emulators on the current system and
    // establish a link layer connection to the first one that is discovered.
    static LinkLayerConnections connectToAllEmulators(
            android::bluetooth::Rootcanal* rootcanal,
            base::Looper* looper);

private:
    std::shared_ptr<GrpcLinkChannelServer> mClassicServer;
    std::shared_ptr<GrpcLinkChannelServer> mBleServer;
    android::bluetooth::Rootcanal* mRootcanal;
    // Order is important, the forwarders depend on the channel server
    // and stub
    EmulatorGrpcClient mClient;
    std::unique_ptr<EmulatedBluetoothService::Stub> mStub;
    std::unique_ptr<LinkForwarder> mClassicHandler;
    std::unique_ptr<LinkForwarder> mBleHandler;
};

}  // namespace bluetooth
}  // namespace emulation
}  // namespace android