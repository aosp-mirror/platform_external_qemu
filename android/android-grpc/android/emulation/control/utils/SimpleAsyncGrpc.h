
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
#include <grpcpp/grpcpp.h>
#include <mutex>
#include <queue>

// A simple reader reactor, use this if you want to read
// a stream of data that closes the connection when all messages
// have been received.
template <typename T, typename R>
class WithSimpleReader : public T {
public:
    WithSimpleReader() { T::StartRead(&mIncoming); }

    void OnReadDone(bool ok) override {
        if (ok) {
            Read(&mIncoming);
            T::StartRead(&mIncoming);
        } else {
            T::Finish(grpc::Status::OK);
        }
    }

    // Callback that will be invoked when a new object was read.
    virtual void Read(const R* read) = 0;

private:
    R mIncoming;
};

// A simple async writer where objects will be placed in a queue and written
// when it can. You will not be notified when the object is written.
template <typename T, typename W>
class WithSimpleQueueWriter : public T {
public:
    // TODO(jansene): Auto derive W
    // using W = typename grpc_write_signature<T>::write_type;

    void OnWriteDone(bool /*ok*/) override {
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
    bool mWriting;
};

template <typename R, typename W>
using SimpleBidiStream = WithSimpleQueueWriter<
        WithSimpleReader<grpc::ServerBidiReactor<R, W>, R>,
        W>;
template <typename R>
using SimpleReader = WithSimpleReader<grpc::ServerReadReactor<R>, R>;
template <typename W>
using SimpleWriter = WithSimpleQueueWriter<grpc::ServerWriteReactor<W>, W>;