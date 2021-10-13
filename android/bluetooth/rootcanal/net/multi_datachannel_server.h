
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

#include "net/async_data_channel_server.h"  // for AsyncDataChannelServer
#include "android/base/Log.h"
#include <vector>
#include <memory>
#include <mutex>

namespace android {
namespace net {

// A DataChannelServer that multiplexes several servers.
class MultiDataChannelServer : public AsyncDataChannelServer {
public:
    MultiDataChannelServer(std::vector<std::shared_ptr<AsyncDataChannelServer>> servers);
    virtual ~MultiDataChannelServer() = default;

    bool StartListening() override;

    void StopListening() override;

    void Close() override;

    bool Connected() override;


private:
    std::vector<std::shared_ptr<AsyncDataChannelServer>> mServers;
};



}  // namespace net
}  // namespace android
