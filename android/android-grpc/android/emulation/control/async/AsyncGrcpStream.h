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
#include <google/protobuf/message.h>
#include <grpcpp/alarm.h>
#include <grpcpp/grpcpp.h>  // for ServerCompletionQueue, ServerContext
#include <grpcpp/support/async_stream.h>
#include <functional>  // for function, bind
#include <mutex>       // for mutex, lock_guard
#include <queue>       // for queue
#include <thread>      // for thread

namespace android {
namespace emulation {
namespace control {

using grpc::Alarm;
using grpc::CompletionQueue;
using grpc::ServerAsyncReaderWriter;
using grpc::ServerCompletionQueue;
using grpc::ServerContext;
using grpc::Status;

class AsyncGrpcHandler;

// A Bidirectional Asynchronous gRPC connection.
//
// This can be used to make the writing of "partial" async services easier, this
// is the base class that handles all the refcounting, queue management and
// state transitions.
//
class BaseAsyncGrpcConnection {
    friend AsyncGrpcHandler;

public:
    virtual ~BaseAsyncGrpcConnection();
    std::string peer();

protected:
    enum class WriteState { BEGIN, COMPLETE };

    BaseAsyncGrpcConnection(ServerCompletionQueue* cq);
    virtual void initialize() = 0;
    virtual void notifyRead() = 0;
    virtual void notifyConnect() = 0;
    virtual void notifyClose() = 0;
    virtual void scheduleRead() = 0;
    virtual void scheduleWrite(WriteState state) = 0;
    virtual BaseAsyncGrpcConnection* clone() = 0;

    // The state in which this connection can be.
    // The connection has an independent read state and write state.
    enum class QueueState {
        CREATED,
        INCOMING_CONNECTION,
        READ,
        WRITE,
        WRITE_COMPLETE,
        CANCELLED,
        FINISHED
    };

    class CompletionQueueTask {
    public:
        CompletionQueueTask(BaseAsyncGrpcConnection* con, QueueState state);
        ~CompletionQueueTask();
        void execute();
        QueueState getState();
        const char* getStateStr();
        BaseAsyncGrpcConnection* connection();
        static const char* queueStateStr(QueueState state);

    private:
        BaseAsyncGrpcConnection* mConnection;
        QueueState mState;
    };

    void execute(QueueState state);

    ServerContext mContext;
    ServerCompletionQueue* mCq;

    bool mWriteRequested{false};
    Alarm mConnectAlarm;
    Alarm mWriteAlarm;
    bool mOpen{false};

private:
    bool mCanWrite{true};
    int mRefcount{-1};  // Number of times we are in the queue.
};

// Some template magic to extract type signatures.
template <typename S>
struct grpc_bidi_signature;

// This extracts the read and write type out of the "requestXXXX" method on
// gRPC async methods.
template <class S, class R, class W>
struct grpc_bidi_signature<void (S::*)(::grpc::ServerContext*,
                                       ::grpc::ServerAsyncReaderWriter<R, W>*,
                                       ::grpc::CompletionQueue*,
                                       ::grpc::ServerCompletionQueue*,
                                       void*)> {
    using read_type = R;
    using write_type = W;
};

// The BidiAsyncGrpcConnection object is what you will interact with.
template <class Service, class RequestStreamFunction>
class BidiAsyncGrpcConnection : public BaseAsyncGrpcConnection {
    friend AsyncGrpcHandler;

public:
    using R = typename grpc_bidi_signature<RequestStreamFunction>::read_type;
    using W = typename grpc_bidi_signature<RequestStreamFunction>::write_type;
    using OnConnect = std::function<void(
            BidiAsyncGrpcConnection<Service, RequestStreamFunction>*)>;

    // Callback that will be called when this connection has closed. No more
    // events will be delivered, and writes will no longer be possible.
    // Upon return of this callback the object will be deleted.
    using CloseCallback = std::function<void(
            BidiAsyncGrpcConnection<Service, RequestStreamFunction>*)>;

    // Callback that will be invoked when an object has arrived at the
    // connection.
    using ReadCallback = std::function<void(
            BidiAsyncGrpcConnection<Service, RequestStreamFunction>*,
            const R& received)>;

    void write(const W& toWrite) {
        const std::lock_guard<std::mutex> lock(mWriteQueueAccess);
        mWriteQueue.push(toWrite);
        if (mWriteQueue.size() == 1) {
            mWriteAlarm.Set(mCq, gpr_now(gpr_clock_type::GPR_CLOCK_REALTIME),
                            new CompletionQueueTask(this, QueueState::WRITE));
        }
    }

    // Closes this connection, this will write the given status code to the
    // receiving endpoint.
    void close(Status status) {
        mStream.Finish(status,
                       new CompletionQueueTask(this, QueueState::FINISHED));
    }

    void setReadCallback(ReadCallback onRead) { mOnReadCallback = onRead; }
    void setCloseCallback(CloseCallback onClose) { mOnCloseCallback = onClose; }

protected:
    BidiAsyncGrpcConnection(
            ServerCompletionQueue* cq,
            Service* s,               // The actual Async Service you are using
            RequestStreamFunction f,  // Function pointer to the
                                      // Requestxxxx function.
            OnConnect connect)
        : BaseAsyncGrpcConnection(cq),
          mService(s),
          mInitRequest(f),
          mStream(&mContext),
          mOnConnectCallback(connect) {}

    BaseAsyncGrpcConnection* clone() override {
        return new BidiAsyncGrpcConnection(mCq, mService, mInitRequest,
                                           mOnConnectCallback);
    }

    void initialize() override {
        using namespace std::placeholders;
        // Bind the function to the active service function and invoke it.
        // The end result is that we can schedule a listener.
        auto initFunc = std::bind(mInitRequest, mService, _1, _2, _3, _4, _5);
        initFunc(
                &mContext, &mStream, mCq, mCq,
                new CompletionQueueTask(this, QueueState::INCOMING_CONNECTION));
    }

    void notifyRead() override {
        if (mOnReadCallback) {
            mOnReadCallback(this, mIncomingReadObject);
        }
    }

    void notifyConnect() override {
        if (mOnConnectCallback) {
            mOnConnectCallback(this);
        }
    }

    void notifyClose() override {
        if (mOnCloseCallback) {
            mOnCloseCallback(this);
        }
    }

    void scheduleRead() override {
        mStream.Read(&mIncomingReadObject,
                     new CompletionQueueTask(this, QueueState::READ));
    }

    void scheduleWrite(WriteState state) override {
        const std::lock_guard<std::mutex> lock(mWriteQueueAccess);

        if (!mOpen || mWriteQueue.empty()) {
            // Nothing to do!
            return;
        }

        // Write out the actual object.
        switch (state) {
            case WriteState::BEGIN:
                mStream.Write(mWriteQueue.front(),
                              new CompletionQueueTask(
                                      this, QueueState::WRITE_COMPLETE));

                break;
            case WriteState::COMPLETE:
                mWriteQueue.pop();
                mWriteAlarm.Set(
                        mCq, gpr_now(gpr_clock_type::GPR_CLOCK_REALTIME),
                        new CompletionQueueTask(this, QueueState::WRITE));
                break;
        }
    }

private:
    std::mutex mWriteQueueAccess;
    RequestStreamFunction mInitRequest;
    OnConnect mOnConnectCallback;
    R mIncomingReadObject;
    std::queue<W> mWriteQueue;
    Service* mService;
    ReadCallback mOnReadCallback;
    CloseCallback mOnCloseCallback;
    ServerAsyncReaderWriter<R, W> mStream;
};

// template <class Service, class RequestStreamFunction>
// class CallbackBuilder;

// A handler for async grpc connections.
//
// Note, you want to keep this handler alive for as long you have your
// service up, otherwise you will not be processing events for the endpoint.
//
//
// A Handler will spin up one thread for each completion queue that it is
// supporting.
// Keep in mind that you will likely have to play with the number of
// queues. Incoming connections for example can only be processed by the
// INCOMING_STATE, of which there is usually only one per queue.
//
// IMPORTANT: This object needs to be alive destroyed AFTER the service has
// shutdown
//
//
// You usually want to do something like this:
//
// AsyncGrpcHandler handler{vector_of_queues};
// handler.registerConnectionHandler(
//   myAsyncService,
//   &MyClassDefintion::Requestxxxxx
//  ).withCallback([](auto connection) {
//    std::cout << "Received a connection from: " << connection->peer() <<
//    std::endl; connection->setReadCallback([](auto from, auto received) {
//       std::cout << "Received: " << received.DebugString();
//       from->write(received);
//     });
//     connection->setCloseCallback([testService](auto connection) {
//           std::cout << "Closed: " << connection->peer();
//     });
//  })
//
// // Process events on vector_of_queues.size() threads.
//  handler.start()
//
// TODO(jansene): Apply magic to generalize to unary connections.
class AsyncGrpcHandler {
public:
    // The number of completion queues you wish to use. Each queue is handled
    // by a single thread.
    AsyncGrpcHandler(
            std::vector<std::unique_ptr<ServerCompletionQueue>> queues);
    ~AsyncGrpcHandler();

    // Inner builder class to facilitate deriving of the callback method.
    template <class Service, class RequestStreamFunction>
    class CallbackBuilder {
    public:
        CallbackBuilder(AsyncGrpcHandler* handler,
                        Service* s,
                        RequestStreamFunction f)
            : mHandler(handler), mService(s), mFunction(f){};
        using callback = std::function<void(
                BidiAsyncGrpcConnection<Service, RequestStreamFunction>*)>;

        void withCallback(callback cb) {
            // Register and schedule the callbacks.
            for (const auto& queue : mHandler->mCompletionQueues) {
                auto initialHandler =
                        new BidiAsyncGrpcConnection<Service,
                                                    RequestStreamFunction>(
                                queue.get(), mService, mFunction, cb);
                initialHandler->execute(
                        BaseAsyncGrpcConnection::QueueState::CREATED);
            }
        }

    private:
        Service* mService;
        RequestStreamFunction mFunction;
        AsyncGrpcHandler* mHandler;
    };

    // Registers a connection handler, we use the builder pattern
    // otherwise typing is becoming pretty outrageous fast.
    //
    // The builder pattern allows us to use C++ template deduction rules.
    // This is mainly to facilate lambda functions that get auto deduced.
    template <class Service, class RequestStreamFunction>
    auto registerConnectionHandler(Service* s, RequestStreamFunction f)
            -> CallbackBuilder<Service, RequestStreamFunction> {
        return CallbackBuilder<Service, RequestStreamFunction>(this, s, f);
    }

private:
    void start();
    void runQueue(ServerCompletionQueue* queue);

    const std::vector<std::unique_ptr<ServerCompletionQueue>> mCompletionQueues;
    std::vector<std::thread> mRunners;
};

}  // namespace control
}  // namespace emulation
}  // namespace android
