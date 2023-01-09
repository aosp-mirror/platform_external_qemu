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
#include <grpcpp/grpcpp.h>
#include <grpcpp/support/client_callback.h>
#include <mutex>
#include <queue>
#include <type_traits>

// A simple reader reactor, use this if you want to read
// a stream of data.
//
// For a server reader the channel will be closed with status::ok
// if a message is not ok. Do not use this in a serverif you want to write
// while the client reads are already finished (done/failed).
//
// T can be a Server or Client Reactor..
template <typename T, typename R>
class WithSimpleReader : public T {
public:
    using is_server = std::is_base_of<grpc::internal::ServerReactor, T>;

    WithSimpleReader() {
        if constexpr (is_server::value) {
            // Clients should not start immediately
            StartRead();
        }
    }

    void OnReadDone(bool ok) override {
        if (ok) {
            Read(&mIncoming);
            T::StartRead(&mIncoming);
        } else {
            if constexpr (is_server::value) {
                // Call finish if we are a server
                T::Finish(grpc::Status::OK);
            }
        }
    }

    void StartRead() { T::StartRead(&mIncoming); }

    // Callback that will be invoked when a new object was read.
    virtual void Read(const R* read) = 0;

private:
    R mIncoming;
};

// A simple async writer where objects will be placed in a queue and written
// when it can.  Some things to be aware of:
//
// - You will not be notified when the object is written.
// - The queue will also grow on forever.. Your write speed should not be
//   higher than what gRPC can actually push out on the wire.
template <typename T, typename W>
class WithSimpleQueueWriter : public T {
public:
    // TODO(jansene): Auto derive W
    // using W = typename grpc_write_signature<T>::write_type;

    void OnWriteDone(bool ok) override {
        {
            const std::lock_guard<std::mutex> lock(mWritelock);
            mWriteQueue.pop();
            mWriting = false;
        }
        NextWrite();
    }

    // Writes out the given object on the gRPC thread.
    void Write(const W& msg) {
        {
            const std::lock_guard<std::mutex> lock(mWritelock);
            mWriteQueue.push(msg);
        }
        NextWrite();
    }

private:
    void NextWrite() {
        {
            const std::lock_guard<std::mutex> lock(mWritelock);
            if (!mWriteQueue.empty() && !mWriting) {
                mWriting = true;
                T::StartWrite(&mWriteQueue.front());
            }
        }
    }

    std::queue<W> mWriteQueue;
    std::mutex mWritelock;
    bool mWriting{false};
};

// A bidi serverstream constructed from a simple reader and queuewriter.
template <typename R, typename W>
using SimpleServerBidiStream = WithSimpleQueueWriter<
        WithSimpleReader<grpc::ServerBidiReactor<R, W>, R>,
        W>;
// A simple server reader
template <typename R>
using SimpleServerReader = WithSimpleReader<grpc::ServerReadReactor<R>, R>;

// A simple server writer
template <typename W>
using SimpleServerWriter = WithSimpleQueueWriter<grpc::ServerWriteReactor<W>, W>;

// A client bidi stream
template <typename R, typename W>
using SimpleClientBidiStream = WithSimpleQueueWriter<
        WithSimpleReader<grpc::ClientBidiReactor<W, R>, R>,
        W>;
