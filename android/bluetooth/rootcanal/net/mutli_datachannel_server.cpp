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

#include <mutex>          // for mutex, unique_lock
#include <type_traits>    // for remove_extent_t
#include <unordered_set>  // for unordered_set
#include <vector>         // for vector

#include "aemu/base/logging/CLog.h"
#include "net/async_data_channel_server.h"  // for AsyncDataChannelServer (p...
#include "net/multi_datachannel_server.h"   // for MultiDataChannelServer

// #define DEBUG 1
/* set  for very verbose debugging */
#ifndef DEBUG
#define DD(...) (void)0
#else
#define DD(...) dinfo(__VA_ARGS__)
#endif

namespace android {
namespace net {

MultiDataChannelServer::MultiDataChannelServer(
        const DataChannelServerList& servers) {
    for (auto& channel : servers) {
        mServers.insert(channel);
        channel->SetOnConnectCallback([&](auto channel, auto server) {
            if (callback_) {
                callback_(channel, this);
            }
        });
    }
}

MultiDataChannelServer::MultiDataChannelServer(
        std::shared_ptr<AsyncDataChannelServer> oneServer)
    : MultiDataChannelServer(
              std::vector<std::shared_ptr<AsyncDataChannelServer>>(
                      {oneServer})) {}

bool MultiDataChannelServer::StartListening() {
    std::unique_lock<std::mutex> guard(mServerAccess);
    bool listening = true;
    for (auto& channel : mServers) {
        listening = channel->StartListening() && listening;
    }
    mIsListening = listening;
    return listening;
}

void MultiDataChannelServer::StopListening() {
    std::unique_lock<std::mutex> guard(mServerAccess);
    for (auto& channel : mServers) {
        channel->StopListening();
    }
    mIsListening = false;
}

void MultiDataChannelServer::Close() {
    std::unique_lock<std::mutex> guard(mServerAccess);
    for (auto& channel : mServers) {
        channel->Close();
    }
}

bool MultiDataChannelServer::Connected() {
    std::unique_lock<std::mutex> guard(mServerAccess);
    for (auto& channel : mServers) {
        if (!channel->Connected()) {
            return false;
        }
    }
    return true;
}

void MultiDataChannelServer::add(
        std::shared_ptr<AsyncDataChannelServer> server) {
    DD("Registering %p on %p, --> %s", server.get(), this,
       (callback_ ? "has callback" : "no callback"));
    server->SetOnConnectCallback([&](auto channel, auto server) {
        DD("Forwarding callback from %p -> %p ", server, this);
        if (callback_) {
            callback_(channel, this);
        }
    });
    std::unique_lock<std::mutex> guard(mServerAccess);
    mServers.insert(server);
    if (mIsListening) {
        server->StartListening();
    }
}

void MultiDataChannelServer::remove(
        std::shared_ptr<AsyncDataChannelServer> server) {
    std::unique_lock<std::mutex> guard(mServerAccess);
    mServers.erase(server);
}
}  // namespace net
}  // namespace android