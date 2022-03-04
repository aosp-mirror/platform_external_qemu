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
#include <grpcpp/alarm.h>                 // for Alarm
#include <grpcpp/grpcpp.h>                // for ServerCompletionQueue, Serv...
#include <grpcpp/support/async_stream.h>  // for async streams
#include <functional>                     // for function, _1, _2, _3, _4, _5
#include <memory>                         // for unique_ptr
#include <mutex>                          // for mutex, lock_guard
#include <queue>                          // for queue
#include <string>                         // for string
#include <thread>                         // for thread
#include <type_traits>                    // for is_invocable, conditional
#include <vector>                         // for vector

namespace android {
namespace emulation {
namespace control {

using grpc::Alarm;
using grpc::CompletionQueue;
using grpc::ServerAsyncReaderWriter;
using grpc::ServerAsyncWriter;
using grpc::ServerCompletionQueue;
using grpc::ServerContext;
using grpc::Status;

class AsyncGrpcHandler;

// An asynchronous gRPC connection.
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

    // Closes this connection, this will write the given status code to the
    // receiving endpoint.
    virtual void close(Status status) = 0;

protected:
    enum class WriteState { BEGIN, COMPLETE };

    BaseAsyncGrpcConnection(ServerCompletionQueue* cq);

    // Subclasses implement these to actually handle the various states.
    // states might be handled differently pending the type of connection.

    // Initialize should call the reqeuestXXXXstream function to initialize the
    // async callbacks, it should also queue the next workitem to handle
    // new incoming connections.
    virtual void initialize() = 0;

    // Call out to listeners that an object has been read.
    virtual void notifyRead() = 0;

    // A new connection has arrived, you probably want to place new items
    // on the work queue.
    virtual void notifyConnect() = 0;

    // You might want to inform your listener(s) that you are no longer alive.
    virtual void notifyClose() = 0;

    // Prepare a next read, if needed.
    virtual void scheduleRead() = 0;

    // Schedule a write operation..
    virtual void scheduleWrite(WriteState state) = 0;

    // Provide a new connection that can be used.
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

    // Work items on the completion queue that execute the state machine.
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

protected:
    // Give the children access to context and queues.
    ServerContext* serverContext() { return &mContext; }
    ServerCompletionQueue* serverCompletionQueue() { return mCq; }

    Alarm mWriteAlarm;
    bool mOpen{false};

private:
    ServerContext mContext;
    ServerCompletionQueue* mCq;

    Alarm mConnectAlarm;

    bool mCanWrite{true};  // Debug only..
    bool mWriteRequested{false};

    int mRefcount{-1};  // Number of times we are in the queue.
};

// Some template magic to extract type signatures. This is needed
// by the subclasses of AsyncGrpcConnection<R,W> to extract the read & write
// type from the requestXXXXX methods. This is the case where the client writes
// many objects, and the server writes many objects.
template <typename S>
struct grpc_async_signature;

// This extracts the read and write type out of the "requestXXXX" method on
// gRPC async methods. This type is a BiDi interface.
template <class S, class R, class W>
struct grpc_async_signature<void (S::*)(ServerContext*,
                                        ServerAsyncReaderWriter<R, W>*,
                                        CompletionQueue*,
                                        ServerCompletionQueue*,
                                        void*)> {
    using read_type = R;
    using write_type = W;
};

// This extracts the read and write type out of the "requestXXXX" method on
// gRPC async methods.
// This type is ServerSide streaming interface, where the client writes one
// object and the server writes many,
template <class S, class R, class W>
struct grpc_async_signature<void (S::*)(ServerContext*,
                                        R*,
                                        ServerAsyncWriter<W>*,
                                        CompletionQueue*,
                                        ServerCompletionQueue*,
                                        void*)> {
    using read_type = R;
    using write_type = W;
};

// TODO(jansene): Add client side streaming when we need it.

// This is the general connection object that consumers will use
// to interact with an async connection.
// You can basically listen for:
// - Close, when the connection disappears.
// - OnRead, invoked when an object was read
// - Write, call this to place an object on the write queue.
//  (Note that this is async! It merely schedules the write to be performed.)
//
// It also handles the general incoming object(s), outgoing object(s)
// and notification of listeners.
template <class R, class W>
class AsyncGrpcConnection : public BaseAsyncGrpcConnection {
public:
    // Callback that will be called when this connection has closed. No more
    // events will be delivered, and writes will no longer be possible.
    // Upon return of this callback the object will be deleted.
    //
    // TODO(jansene): Do we care about the Status object that might be there?
    using CloseCallback = std::function<void(AsyncGrpcConnection<R, W>*)>;

    // Callback that will be invoked when an object has arrived at the
    // connection.
    using ReadCallback =
            std::function<void(AsyncGrpcConnection<R, W>*, const R& received)>;

    using OnConnect = std::function<void(AsyncGrpcConnection<R, W>*)>;

    // Write the given object to the connection. The object will be scheduled
    // for writing on the writer queue.
    //
    // TODO(jansene): Do we need a callback to notify completion?
    void write(const W& toWrite) {
        const std::lock_guard<std::mutex> lock(mWriteQueueAccess);
        mWriteQueue.push(toWrite);
        if (mWriteQueue.size() == 1) {
            mWriteAlarm.Set(serverCompletionQueue(),
                            gpr_now(gpr_clock_type::GPR_CLOCK_REALTIME),
                            new CompletionQueueTask(this, QueueState::WRITE));
        }
    }

    // Sets the callback that should be invoked whenever a new object was
    // received.
    void setReadCallback(ReadCallback onRead) { mOnReadCallback = onRead; }

    // Set the callback that will be invoked when this connection was closed.
    void setCloseCallback(CloseCallback onClose) { mOnCloseCallback = onClose; }

protected:
    AsyncGrpcConnection(ServerCompletionQueue* cq, OnConnect connect)
        : BaseAsyncGrpcConnection(cq), mOnConnectCallback(connect) {}

    virtual ~AsyncGrpcConnection() = default;

    // Common notification methods.
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

    void setOnConnect(OnConnect connect) { mOnConnectCallback = connect; }

    R* incoming() { return &mIncomingReadObject; }

    std::mutex mWriteQueueAccess;
    OnConnect mOnConnectCallback;
    R mIncomingReadObject;
    std::queue<W> mWriteQueue;
    ReadCallback mOnReadCallback;
    CloseCallback mOnCloseCallback;
};

// The BidiAsyncGrpcConnection object is what you will interact with when
// using bi directional streaming.
template <class Service, class RequestStreamFunction>
class BidiAsyncGrpcConnection
    : public AsyncGrpcConnection<
              typename grpc_async_signature<RequestStreamFunction>::read_type,
              typename grpc_async_signature<
                      RequestStreamFunction>::write_type> {
    friend AsyncGrpcHandler;

public:
    using R = typename grpc_async_signature<RequestStreamFunction>::read_type;
    using W = typename grpc_async_signature<RequestStreamFunction>::write_type;

    // True if you can actually construct this class given the request stream
    // type. This basically garantees that we can std::bind to the
    // RequestStreamFunction. that was generated by the gRPC compiler.
    using can_construct = std::is_invocable<RequestStreamFunction,
                                            Service*,
                                            ServerContext*,
                                            ServerAsyncReaderWriter<R, W>*,
                                            CompletionQueue*,
                                            ServerCompletionQueue*,
                                            void*>;

    // Allright, so the compiler is not really able to derive that we are a
    // subclass of Parent, so we need give all our references a little nudge.
    // References to a protected member of the parent class will fail to resolve
    // otherwise.
    using Parent = AsyncGrpcConnection<R, W>;
    using CompletionQueueTask = typename Parent::CompletionQueueTask;
    using WriteState = typename Parent::WriteState;
    using QueueState = typename Parent::QueueState;

    void close(Status status) override {
        mStream.Finish(status,
                       new CompletionQueueTask(this, QueueState::FINISHED));
    }

private:
    BidiAsyncGrpcConnection(
            ServerCompletionQueue* cq,
            Service* s,               // The actual Async Service you are using
            RequestStreamFunction f,  // Function pointer to the
                                      // Requestxxxx function.
            typename Parent::OnConnect connect)
        : Parent::AsyncGrpcConnection(cq, connect),
          mService(s),
          mInitRequest(f),
          mStream(Parent::serverContext()) {}

    BaseAsyncGrpcConnection* clone() override {
        return new BidiAsyncGrpcConnection(Parent::serverCompletionQueue(),
                                           mService, mInitRequest,
                                           Parent::mOnConnectCallback);
    }

    void initialize() override {
        using namespace std::placeholders;
        // Bind the function to the active service function and invoke it.
        // The end result is that we can schedule a listener.
        auto initFunc = std::bind(mInitRequest, mService, _1, _2, _3, _4, _5);
        initFunc(Parent::serverContext(), &mStream,
                 Parent::serverCompletionQueue(),
                 Parent::serverCompletionQueue(),
                 new CompletionQueueTask(
                         this, Parent::QueueState::INCOMING_CONNECTION));
    }

    void scheduleRead() override {
        mStream.Read(Parent::incoming(),
                     new CompletionQueueTask(this, Parent::QueueState::READ));
    }

    void scheduleWrite(WriteState state) override {
        const std::lock_guard<std::mutex> lock(Parent::mWriteQueueAccess);

        if (!Parent::mOpen || Parent::mWriteQueue.empty()) {
            // Nothing to do!
            return;
        }

        // Write out the actual object.
        switch (state) {
            case WriteState::BEGIN:
                mStream.Write(Parent::mWriteQueue.front(),
                              new CompletionQueueTask(
                                      this, QueueState::WRITE_COMPLETE));

                break;
            case WriteState::COMPLETE:
                Parent::mWriteQueue.pop();
                Parent::mWriteAlarm.Set(
                        Parent::serverCompletionQueue(),
                        gpr_now(gpr_clock_type::GPR_CLOCK_REALTIME),
                        new CompletionQueueTask(this, QueueState::WRITE));
                break;
        }
    }

private:
    Service* mService;
    RequestStreamFunction mInitRequest;
    ServerAsyncReaderWriter<R, W> mStream;
};

// The ServerStreamingAsyncGrpcConnection object is what you will interact with
// when using service side only streaming.
template <class Service, class RequestStreamFunction>
class ServerStreamingAsyncGrpcConnection
    : public AsyncGrpcConnection<
              typename grpc_async_signature<RequestStreamFunction>::read_type,
              typename grpc_async_signature<
                      RequestStreamFunction>::write_type> {
    friend AsyncGrpcHandler;

public:
    using R = typename grpc_async_signature<RequestStreamFunction>::read_type;
    using W = typename grpc_async_signature<RequestStreamFunction>::write_type;
    using can_construct = std::is_invocable<RequestStreamFunction,
                                            Service*,
                                            ServerContext*,
                                            R*,
                                            ServerAsyncWriter<W>*,
                                            CompletionQueue*,
                                            ServerCompletionQueue*,
                                            void*>;

    // Allright, so the compiler is not really able to derive that we are a
    // subclass of AsyncGrpcConnection<R, W>, so we need give all our references
    // a little nudge. References to a protected member of the parent class will
    // fail to resolve otherwise.
    using Parent = AsyncGrpcConnection<R, W>;
    using CompletionQueueTask = typename Parent::CompletionQueueTask;
    using WriteState = typename Parent::WriteState;
    using QueueState = typename Parent::QueueState;

    void close(Status status) override {
        mStream.Finish(status,
                       new CompletionQueueTask(this, QueueState::FINISHED));
    }

protected:
    ServerStreamingAsyncGrpcConnection(
            ServerCompletionQueue* cq,
            Service* s,               // The actual Async Service you are using
            RequestStreamFunction f,  // Function pointer to the
                                      // Requestxxxx function.
            typename Parent::OnConnect connect)
        : Parent::AsyncGrpcConnection(cq, connect),
          mService(s),
          mInitRequest(f),
          mStream(Parent::serverContext()) {}

    BaseAsyncGrpcConnection* clone() override {
        return new ServerStreamingAsyncGrpcConnection(
                Parent::serverCompletionQueue(), mService, mInitRequest,
                Parent::mOnConnectCallback);
    }

    void initialize() override {
        // Bind the function to the active service function and invoke it.
        // The end result is that we can schedule a listener.
        // initFunc basically binds to a stricter type call to
        // ::grpc::Service::RequestAsyncServerStreaming

        using namespace std::placeholders;
        auto initFunc =
                std::bind(mInitRequest, mService, _1, _2, _3, _4, _5, _6);
        initFunc(Parent::serverContext(), Parent::incoming(), &mStream,
                 Parent::serverCompletionQueue(),
                 Parent::serverCompletionQueue(),
                 new CompletionQueueTask(
                         this, Parent::QueueState::INCOMING_CONNECTION));
    }

    void scheduleRead() override {
        // We are not scheduling reads, as we get the first read upon
        // registration. When this method is called, incoming() should already
        // have been filled out so we can notify the listener.
        Parent::notifyRead();
    }

    void scheduleWrite(WriteState state) override {
        const std::lock_guard<std::mutex> lock(Parent::mWriteQueueAccess);

        // std::cout << "Scheduling write";
        if (!Parent::mOpen || Parent::mWriteQueue.empty()) {
            // Nothing to do!
            return;
        }

        // Write out the actual object.
        switch (state) {
            case WriteState::BEGIN:
                // std::cout << "Write begin: " <<
                // Parent::mWriteQueue.front().DebugString();
                mStream.Write(Parent::mWriteQueue.front(),
                              new CompletionQueueTask(
                                      this, QueueState::WRITE_COMPLETE));

                break;
            case WriteState::COMPLETE:
                // std::cout << "Write complete: " <<
                // Parent::mWriteQueue.front().DebugString();
                Parent::mWriteQueue.pop();
                Parent::mWriteAlarm.Set(
                        Parent::serverCompletionQueue(),
                        gpr_now(gpr_clock_type::GPR_CLOCK_REALTIME),
                        new CompletionQueueTask(this, QueueState::WRITE));
                break;
        }
    }

private:
    Service* mService;
    RequestStreamFunction mInitRequest;
    ServerAsyncWriter<W> mStream;
};

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

        using bidi = BidiAsyncGrpcConnection<Service, RequestStreamFunction>;

        using server_stream =
                ServerStreamingAsyncGrpcConnection<Service,
                                                   RequestStreamFunction>;

        // All streaming types are of AsyncGrpcConnection<R,W>, so lets extract
        // the type, extraction does not really invoke methods so this will work
        // for all subtypes of AsyncGrpcConnection.
        using callback = typename BidiAsyncGrpcConnection<
                Service,
                RequestStreamFunction>::Parent::OnConnect;

        // Note that incoming connections will be of the type:
        // AsyncGrpcConnection<R, W>* where R = read type, W = the write type.
        void withCallback(callback cb) {
            // Register and schedule the callbacks on all completion queues.
            for (const auto& queue : mHandler->mCompletionQueues) {
                // Select the proper type based on which class can serve
                // the RequestStreamFunction that was provided.
                //
                // TODO(jansene): Extend with client side streaming when needed.
                using async_grpc_type =
                        typename std::conditional<bidi::can_construct::value,
                                                  bidi, server_stream>::type;
                auto initialHandler = new async_grpc_type(queue.get(), mService,
                                                          mFunction, cb);
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
