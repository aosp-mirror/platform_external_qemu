
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
#include <cstdint>

namespace emulator {
namespace net {

class AsyncSocketAdapter;
class AsyncSocketEventListener {
public:
    virtual ~AsyncSocketEventListener() = default;
    // Called when bytes can be read from the socket.
    virtual void OnRead(AsyncSocketAdapter* socket) = 0;

    // Called when this socket is closed.
    virtual void OnClose(AsyncSocketAdapter* socket, int err) = 0;
};

// A connected asynchronous socket.
// The videobridge will provide an implementation, as well as the android-webrtc module.
class AsyncSocketAdapter {
public:
    virtual ~AsyncSocketAdapter() = default;
    virtual void AddSocketEventListener(AsyncSocketEventListener* listener) = 0;
    virtual void RemoveSocketEventListener(
            AsyncSocketEventListener* listener) = 0;
    virtual void Close() = 0;
    virtual uint64_t Recv(char* buffer, uint64_t bufferSize) = 0;
    virtual uint64_t Send(const char* buffer, uint64_t bufferSize) = 0;
};
}  // namespace net
}  // namespace emulator