
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
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

// It appears the win32 reactory implementation of gRPC  is not working properly
// While we wait to get to the root of this issue we introduce this
// wrapper that allows you to easily use the Async Implementation from the Sync
// one.

template <typename R, typename W>
class SyncToAsyncBidiAdapter {
public:

    using read_type = R;
    using write_type = W;

    SyncToAsyncBidiAdapter() {}

    void start(grpc::ServerReaderWriter<R, W>* stream) {
        mStream = stream;
        mReader = std::move(std::thread([&] { startReader(); }));
        mWriter = std::move(std::thread([&] { startWriter(); }));
    }

    virtual void Read(const R* msg) = 0;
    virtual void OnDone() = 0;
    virtual void OnCancel() {  Finish(::grpc::Status::CANCELLED); }

    void Write(const W& msg) {
        std::unique_lock<std::mutex> lock(mQueueMutex);
        mWriteQueue.push(msg);
        mQueueCv.notify_one();
    }

    void startReader() {
        R incoming;
        while (mOpen && mStream->Read(&incoming)) {
            Read(&incoming);
        }
        OnCancel();
    }

    void startWriter() {
        while (mOpen) {
            W msg;
            {
                std::unique_lock<std::mutex> lock(mQueueMutex);
                mQueueCv.wait(lock,
                              [&] { return !mOpen || !mWriteQueue.empty(); });
                if (!mOpen) {
                    return;
                }
                msg = mWriteQueue.front();
                mWriteQueue.pop();
            }
            mOpen = mStream->Write(msg);
        };
    }

    void Finish(::grpc::Status state) {
        std::unique_lock<std::mutex> lock(mQueueMutex);
        mOpen = false;
        mStatus = state;
        mQueueCv.notify_one();
    }

    grpc::Status status() { return mStatus; }

    void await() {
        mWriter.join();
        mReader.join();
    }

private:
    grpc::ServerContext* mContext;
    grpc::ServerReaderWriter<R, W>* mStream;

    // Writer
    std::mutex mQueueMutex;
    std::condition_variable mQueueCv;
    std::queue<W> mWriteQueue;
    std::thread mWriter;

    // Reader
    R* mWhereTo{nullptr};
    std::thread mReader;

    // General
    grpc::Status mStatus{grpc::Status::OK};
    bool mOpen{true};
};



template <class T>
class BidiRunner {
public:
    using R = typename T::read_type;
    using W = typename T::write_type;
    using is_adapter = std::is_base_of<SyncToAsyncBidiAdapter<R,W>, T>;

    BidiRunner(::grpc::ServerReaderWriter<R, W>* stream, T* t) {
        static_assert(is_adapter::value);
        handler = t;
        handler->start(stream);
        handler->await();
    }

    template <typename... Args>
    BidiRunner(::grpc::ServerReaderWriter<R, W>* stream, Args&&... args)
        : BidiRunner(stream, new T(std::forward<Args>(args)...)) {}

    auto status() { return handler->status(); }

    ~BidiRunner() { handler->OnDone(); }

private:
    T* handler;
};

template<typename R, typename W>
using SimpleServerBidiStream = SyncToAsyncBidiAdapter<R, W>;
