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
// limitations under the License.
#pragma once

#include <functional>               // for function
#include <memory>                   // for enable_shared_from_this, shared_ptr
#include <stddef.h>                 // for size_t
#include <sys/types.h>              // for ssize_t
#include <cstdint>                  // for uint8_t, uint64_t
#include <mutex>                    // for mutex, recursive_mutex
#include <vector>                   // for vector

#include "net/async_data_channel.h"  // for ReadCallback, AsyncDataChannel

namespace android {
namespace base {
class Looper;
}  // namespace base

namespace emulation {
namespace bluetooth {
using android::base::Looper;
using net::AsyncDataChannel;
using net::ReadCallback;

// An implementation of the AsyncDataChannel that is backed by a
// memory buffer. This can be used to adapt between various
// data channel protocols.
//
// This channel can deliver spurious receive events.
//
// Note: You must keep the looper alive for as long this channel exists.
class BufferedAsyncDataChannel
    : public std::enable_shared_from_this<BufferedAsyncDataChannel>,
      public AsyncDataChannel {
public:
    BufferedAsyncDataChannel(const BufferedAsyncDataChannel& other) = delete;

    using SendCallback = std::function<void(BufferedAsyncDataChannel*)>;
    using OnCloseCallback = std::function<void(BufferedAsyncDataChannel*)>;

    virtual ~BufferedAsyncDataChannel();

    // Receive data in the given buffer. Returns the number of bytes read,
    // or a negative number in case of failure. Check the errno variable to
    // learn why the call failed.
    ssize_t Recv(uint8_t* buffer, uint64_t bufferSize) override;

    // Send this data. Returns the number of bytes send,
    // or a negative number in case of failure. The data will end up
    // in the write queue, and a callback will be scheduled to notify
    // listeners of the available bytes.
    ssize_t Send(const uint8_t* buffer, uint64_t bufferSize) override;

    // This is always connected.
    bool Connected() override { return true; };

    // Mark this channel as closed. This will invoke the callback for close.
    void Close() override;

    // Registers the given callback to be invoked when a recv call can be made
    // to read data from this socket.
    // Only one callback can be registered per channel. Registration is thread
    // safe. Registration and invocation is protected by a recursive_mutex. i.e:
    //   You can set the callback to nullptr from within the callback.
    //   If a callback is active it will complete before you can set it.
    bool WatchForNonBlockingRead(
            const ReadCallback& on_read_ready_callback) override;

    void StopWatching() override;

    // Adaptation methods.

    // Register a callback that will be invoked when someone just called send
    // and placed bytes in the send buffer.
    //
    // Listeners now can pick the bytes in the queue and use them.
    //
    // Only one callback can be registered per channel. Registration is thread
    // safe. Registration and invocation is protected by a recursive_mutex. i.e:
    //  - You can set the callback to nullptr from within the callback.
    //  - If a callback is active it will complete before you can set it.
    bool WatchForNonBlockingSend(const SendCallback& on_send_ready_callback);

    // Register a callback that will be invoked when someone just called Close.
    // Only one callback can be registered per channel. Registration is thread
    // safe. Registration and invocation is protected by a recursive_mutex.
    // i.e: You can set the callback to nullptr from within the callback.
    // (Which would make no sense in close)
    bool WatchForClose(const OnCloseCallback& on_close_callback);

    // Writes to the receive buffer and generate a non blocking read event.
    ssize_t WriteToRecvBuffer(const uint8_t* buffer, uint64_t bufferSize);

    // Receive data in the given buffer. Returns the number of bytes read,
    // form the send buffer or a negative number in case of failure.
    ssize_t ReadFromSendBuffer(uint8_t* buffer, uint64_t bufferSize);

    // Information about the number of bytes that might be available.
    size_t availabeInReadBuffer();
    size_t availableInSendBuffer();
    size_t maxQueueSize();

    // True if this channel is open.
    bool isOpen() { return mOpen; }

    static std::shared_ptr<BufferedAsyncDataChannel> create(size_t maxSize,
                                                            Looper* looper);

protected:
    BufferedAsyncDataChannel(size_t maxSize, Looper* looper);

private:
    ssize_t writeToQueue(std::vector<uint8_t>& queue,
                         const uint8_t* buffer,
                         uint64_t bufferSize);
    ssize_t readFromQueue(std::vector<uint8_t>& queue,
                          uint8_t* buffer,
                          uint64_t bufferSize);

    ReadCallback mReadCallback;
    SendCallback mWriteCallback;
    OnCloseCallback mOnCloseCallback;

    std::vector<uint8_t> mReadQueue;
    std::vector<uint8_t> mWriteQueue;
    size_t mMaxQueueSize;

    Looper* mLooper;
    bool mOpen{true};

    std::mutex mReadMutex;
    std::mutex mWriteMutex;
    std::recursive_mutex mCallbackAccess;
};
}  // namespace bluetooth
}  // namespace emulation
}  // namespace android
