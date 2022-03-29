
// Copyright (C) 2021 The Android Open Source Project
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

#include <memory>
#include <mutex>
#include <unordered_set>
#include <vector>

#include "net/async_data_channel_server.h"  // for AsyncDataChannelServer

namespace android {
namespace net {

using DataChannelServerList =
        std::vector<std::shared_ptr<AsyncDataChannelServer>>;

// A DataChannelServer that multiplexes several servers.
//
// The idea here is that you can provide multiple type of servers
// and if any of them gets a connection, it is handed off to rootcanal.
//
// For example you could have a Socket based server + Grpc based server
// on the same channel. That way the emulator can serve both grpc
// and socket connections.
//
// Rootcanal will just see a  new incoming connection, without really
// knowing if it is a grpc or socket channel.
class MultiDataChannelServer : public AsyncDataChannelServer {
public:
    MultiDataChannelServer(std::shared_ptr<AsyncDataChannelServer> oneServer);
    MultiDataChannelServer(const DataChannelServerList& servers);
    virtual ~MultiDataChannelServer() = default;

    bool StartListening() override;

    void StopListening() override;

    void Close() override;

    bool Connected() override;

    void add(std::shared_ptr<AsyncDataChannelServer> server);

    void remove(std::shared_ptr<AsyncDataChannelServer> server);

private:
    std::unordered_set<std::shared_ptr<AsyncDataChannelServer>> mServers;
    std::mutex mServerAccess;
    bool mIsListening;
};

}  // namespace net
}  // namespace android
