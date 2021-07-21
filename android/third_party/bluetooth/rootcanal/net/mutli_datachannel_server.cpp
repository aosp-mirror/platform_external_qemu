// Copyright 2021 The Android Open Source Project
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

#include "net/multi_datachannel_server.h"

#include <functional>  // for __base, function

namespace android {
namespace net {

MultiDataChannelServer::MultiDataChannelServer(
        std::vector<std::shared_ptr<AsyncDataChannelServer>> servers)
    : mServers(servers) {
    for (auto& channel : mServers) {
        channel->SetOnConnectCallback([&](auto channel, auto server) {
            if (callback_) {
                callback_(channel, this);
            }
        });
    }
}

bool MultiDataChannelServer::StartListening() {
    bool listening = false;
    for (auto& channel : mServers) {
        listening = channel->StartListening() || listening;
    }
    return listening;
}

void MultiDataChannelServer::StopListening() {
    for (auto& channel : mServers) {
        channel->StopListening();
    }
}

void MultiDataChannelServer::Close() {
    for (auto& channel : mServers) {
        channel->Close();
    }
}

bool MultiDataChannelServer::Connected() {
    for (auto& channel : mServers) {
        if (!channel->Connected()) {
            return false;
        }
    }
    return true;
}

}  // namespace net
}  // namespace android