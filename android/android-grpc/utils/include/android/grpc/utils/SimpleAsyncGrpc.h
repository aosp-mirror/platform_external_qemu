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
#include <functional>
#include <mutex>
#include <queue>
#include <type_traits>
#include <utility>

// Selects a type from a pack of types based on the provided index
template <std::size_t Index, typename... Args>
struct Select {
    // Calculates the index of the last available type in the pack
    static constexpr std::size_t LastIndex = sizeof...(Args) - 1;

    // Selects the type at the specified index from the pack of types
    // If the index is out of range, the last available type is selected
    using type =
            typename std::tuple_element<(Index < sizeof...(Args) ? Index
                                                                 : LastIndex),
                                        std::tuple<Args...>>::type;
};

// Specialization for template types with a pack of template arguments
template <std::size_t Index,
          template <typename...>
          class Template,
          typename... Args>
struct Select<Index, Template<Args...>> {
    // Recursively selects the type at the specified index from the pack of
    // template arguments
    using type = typename Select<Index, Args...>::type;
};

// Convenient type alias for the selected type
template <std::size_t Index, typename T>
using Select_t = typename Select<Index, T>::type;

// A simple reader reactor, use this if you want to read
// a stream of data.
//
// For a server reader the channel will be closed with status::ok
// if a message cannot be read (i.e. OnReadDone is not ok)
//
// T can be a Server or Client Reactor..
//
// Note for a client you must explicitly call StartRead once
// you create the reader object.
template <typename T>
class WithSimpleReader : public T {
public:
    using is_server = std::is_base_of<grpc::internal::ServerReactor, T>;

    // We select index 0, or index 1 in case of Bi-directional reactors.
    // I.e. T<X,Y,...> --> Y
    // T<X> --> X
    using R = Select_t<1, T>;

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

// A simple reader reactor, use this if you want to read
// a stream of data. You provide read and done std::function.
// This class will auto delete itself.
//
// The channel will be closed with status::ok
// if a message cannot be read (i.e. OnReadDone is not ok)
template <typename R>
class SimpleServerLambdaReader
    : public WithSimpleReader<grpc::ServerReadReactor<R>> {
    using ReadCallback = std::function<void(const R*)>;
    using OnDoneCallback = std::function<void()>;

public:
    SimpleServerLambdaReader(
            ReadCallback readFn,
            OnDoneCallback doneFn = []() {})
        : mReadFn(readFn), mDoneFn(doneFn) {}

    virtual void Read(const R* read) override { mReadFn(read); }

    virtual void OnDone() override {
        mDoneFn();
        delete this;
    }

private:
    ReadCallback mReadFn;
    OnDoneCallback mDoneFn;
};

// A simple client reader reactor, use this if you want to read
// a stream of data in an async fashion. You provide read and done callback
// functions This class will auto delete itself.
//
// For example:
//
// grpc::ClientContext* context = mClient->newContext().release();
// static google::protobuf::Empty empty;
// auto read = new SimpleClientLambdaReader<PhoneEvent>(
//         [](auto event) {
//            std::cout << "Received event: " << event.ShortDebugString();
//         }
//         ,
//         [context](auto status) {
//             std::cout << "Finished: " << status.error_message());
//             delete context;
//         });
// mService->async()->receivePhoneEvents(context, &empty, read);
// read->StartRead();
// read->StartCall();
template <typename R>
class SimpleClientLambdaReader
    : public WithSimpleReader<grpc::ClientReadReactor<R>> {
    using ReadCallback = std::function<void(const R*)>;
    using OnDoneCallback = std::function<void(::grpc::Status)>;

public:
    SimpleClientLambdaReader(
            std::shared_ptr<grpc::ClientContext> context,
            ReadCallback readFn,
            OnDoneCallback doneFn = [](auto s) {})
        : mReadFn(readFn), mContext(std::move(context)), mDoneFn(doneFn) {}

    virtual void Read(const R* read) override { mReadFn(read); }

    virtual void OnDone(const grpc::Status& status) override {
        mDoneFn(status);
        delete this;
    }

    virtual void TryCancel() { mContext->TryCancel(); }

private:
    ReadCallback mReadFn;
    OnDoneCallback mDoneFn;
    std::shared_ptr<grpc::ClientContext> mContext;
};

// A simple async writer where objects will be placed in a queue and written
// when it can.  Some things to be aware of:
//
// - You will not be notified when the object is written.
// - The queue will also grow on forever.. Your write speed should not be
//   higher than what gRPC can actually push out on the wire.
template <typename T>
class WithSimpleQueueWriter : public T {
public:
    // We always select index 0 of the T<X,...>
    using W = Select_t<0, T>;

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

template <typename W>
class SimpleClientWriter
    : public WithSimpleQueueWriter<grpc::ClientWriteReactor<W>> {
public:
    SimpleClientWriter(std::shared_ptr<::grpc::ClientContext> context)
        : mContext(std::move(context)) {}

    ::grpc::ClientContext* context() { return mContext.get(); }

private:
    std::shared_ptr<::grpc::ClientContext> mContext;
};
// A bi directional serverstream constructed from a simple reader and
// queuewriter.
template <typename R, typename W>
using SimpleServerBidiStream =
        WithSimpleQueueWriter<WithSimpleReader<grpc::ServerBidiReactor<R, W>>>;

// A simple server reader
template <typename R>
using SimpleServerReader = WithSimpleReader<grpc::ServerReadReactor<R>>;

// A simple server writer
template <typename W>
using SimpleServerWriter = WithSimpleQueueWriter<grpc::ServerWriteReactor<W>>;

// A client bidi stream
template <typename R, typename W>
using SimpleClientBidiStream =
        WithSimpleQueueWriter<WithSimpleReader<grpc::ClientBidiReactor<W, R>>>;
