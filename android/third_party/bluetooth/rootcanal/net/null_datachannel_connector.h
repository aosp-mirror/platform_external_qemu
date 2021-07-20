
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

#include <chrono>
#include <string>
#include "net/async_data_channel_connector.h"

using namespace std::chrono_literals;

namespace android {
namespace net {

// A data channel connector that does nothing.
class NullDataChannelConnector : public AsyncDataChannelConnector {
public:
    NullDataChannelConnector() = default;
    virtual ~NullDataChannelConnector() = default;

    std::shared_ptr<AsyncDataChannel> ConnectToRemoteServer(
            const std::string& server,
            int port,
            const std::chrono::milliseconds timeout = 5000ms) override {
        return nullptr;
    };
};
}  // namespace net
}  // namespace android