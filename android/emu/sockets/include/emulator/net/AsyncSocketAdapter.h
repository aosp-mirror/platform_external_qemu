
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
#include <limits>

namespace emulator {
namespace net {

class AsyncSocketAdapter;
class AsyncSocketEventListener {
public:
    virtual ~AsyncSocketEventListener() = default;
    // Called when bytes can be read from the socket.
    virtual void onRead(AsyncSocketAdapter* socket) = 0;

    // Called when this socket is closed.
    virtual void onClose(AsyncSocketAdapter* socket, int err) = 0;

    // Called when this socket (re) established a connection.
    virtual void onConnected(AsyncSocketAdapter* socket) = 0;
};

// A connected asynchronous socket.
// The videobridge will provide an implementation, as well as the android-webrtc
// module.
class AsyncSocketAdapter {
public:
    virtual ~AsyncSocketAdapter() = default;
    void setSocketEventListener(AsyncSocketEventListener* listener) {
        mListener = listener;
    }
    virtual uint64_t recv(char* buffer, uint64_t bufferSize) = 0;
    virtual uint64_t send(const char* buffer, uint64_t bufferSize) = 0;
    virtual void close() = 0;

    // True if this socket is connected
    virtual bool connected() = 0;

    // Re-connect the socket, return false if
    // reconnection will never succeed,
    virtual bool connect() = 0;

    // Connect synchronously, returning true if succeeded
    // false if we timed out. The onConnected callback will have been called
    // before this function returns. This means that if you lock on mutex x before
    // calling this you will not be able to lock mutex x in the onConnected callback.
    virtual bool connectSync(uint64_t timeoutms=std::numeric_limits<int>::max()) = 0;

    // Disposes this socket, after return the following should hold:
    // - No events will be delivered.
    // - No send/recv/connect/close calls will be made.
    // - The socket can be closed, and any ongoing connects should stop.
    virtual void dispose() = 0;

protected:
    AsyncSocketEventListener* mListener = nullptr;
};

}  // namespace net
}  // namespace emulator