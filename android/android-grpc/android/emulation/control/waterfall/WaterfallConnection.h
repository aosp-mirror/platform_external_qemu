// Copyright (C) 2019 The Android Open Source Project
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

#include <functional>

namespace android {
namespace emulation {
namespace control {

// A connection to a waterfall endpoint.
class WaterfallConnection {
public:
    virtual ~WaterfallConnection() = default;

    // Called after construction, openConnection() can
    // be called after this.
    virtual bool initialize() = 0;

    // Returns a new FD to the waterfall server.
    // or -1 in case of error. The returned fd
    // is ready for usage, so handshaking has taken place.
    //
    // The returened fd will use the forwarding protocol.
    // where Bstr's are exchanged, and close is done by sending
    // an empty bstr.
    virtual int openConnection() = 0;
};

WaterfallConnection* getDefaultWaterfallConnection();
WaterfallConnection* getTestWaterfallConnection(
        std::function<int()>&& serverSockFn);
}  // namespace control
}  // namespace emulation
}  // namespace android
