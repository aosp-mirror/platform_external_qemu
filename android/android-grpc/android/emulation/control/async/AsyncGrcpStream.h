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
#include <grpcpp/alarm.h>
#include <grpcpp/grpcpp.h>  // for ServerCompletionQueue, ServerContext
#include <grpcpp/support/async_stream.h>
#include <stdint.h>       // for intptr_t, uint64_t
#include <atomic>         // for atomic_uint64_t, __atomic_base
#include <cassert>        // for assert
#include <functional>     // for function, bind
#include <memory>         // for shared_ptr, make_shared
#include <mutex>          // for mutex, lock_guard
#include <queue>          // for queue
#include <thread>         // for thread
#include <unordered_map>  // for unordered_map, unordered_map<>::map...

#include "android/utils/debug.h"  // for VERBOSE_INFO, VERBOSE_PRINT

#define DEBUG 0

#if DEBUG >= 1
// This is *very* noisy.
#define DD(...) VERBOSE_PRINT(grpc, __VA_ARGS__)
#else
#define DD(...) (void)0
#endif

namespace android {
namespace emulation {
namespace control {

using grpc::Alarm;
using grpc::CompletionQueue;
using grpc::ServerAsyncReaderWriter;
using grpc::ServerCompletionQueue;
using grpc::ServerContext;
using grpc::Status;

template <class R, class W>
class AsyncGrpcHandler;

// A Bidirectional Asynchronous gRPC connection.
//
// This can be used to make the writing of "partial" async services easier.
//
// An example of this can be found in the TestEchoService.cpp class.
//
// The basic idea is that you:
//
// Provide the following set of callbacks:
//
// A callback for receiving reads
// A callback that is used to accept incoming connections, this
// usually does nothing more than calling:
//   myService->RequestXXXX(context, stream, cq, cq, tag);
// where XXXX is the method of interest.
// A callback for close notifications.
//
template <class R, class W>
class BidiGrpcConnection {
    friend AsyncGrpcHandler<R, W>;

public:
    // Callback that will be invoked when an object has arrived at the
    // connection.
    using ReadCallback =
            std::function<void(BidiGrpcConnection<R, W>*, const R& received)>;

    // Callback that will be called when this connection has closed. No more
    // events will be delivered, and writes will no longer be possible.
    // Upon return of this callback the object will be deleted.
    using CloseCallback = std::function<void(BidiGrpcConnection<R, W>*)>;

    // Callback that will be called to get the AsyncRequest.
    // usually this calls the generated Requestxxxx method on the generated
    // server class.
    //
    // TODO(jansene): There must be a magic template way to use std::bind in the
    // constructor.
    using RequestMethodFn = std::function<void(ServerContext*,
                                               ServerAsyncReaderWriter<R, W>*,
                                               CompletionQueue*,
                                               ServerCompletionQueue*,
                                               void*)>;

    BidiGrpcConnection(ServerCompletionQueue* cq, RequestMethodFn requestFn)
        : mCq(cq), mStream(&mContext), mRequestMethodFn(requestFn) {}

    ~BidiGrpcConnection() {
        DD("Completed interaction with (%p) %s", this, mContext.peer().c_str());
    }

    // Places this object in the write queue, and schedules it for writing.
    void write(const W& toWrite) {
        const std::lock_guard<std::mutex> lock(mWriteQueueAccess);
        mWriteQueue.push(toWrite);
        if (mWriteQueue.size() == 1) {
            mWriteAlarm.Set(mCq, gpr_now(gpr_clock_type::GPR_CLOCK_REALTIME),
                            new CompletionQueueTask(this, QueueState::WRITE));
        }
    }

    // Number of elements that are scheduled to be written.
    size_t writeQueueSize() {
        const std::lock_guard<std::mutex> lock(mWriteQueueAccess);
        return mWriteQueue.size();
    }

    // Closes this connection, this will write the given status code to the
    // receiving endpoint.
    void close(Status status) {
        mStream.Finish(status,
                       new CompletionQueueTask(this, QueueState::FINISHED));
    }

    void setReadCallback(ReadCallback onRead) { mOnReadCallback = onRead; }
    void setCloseCallback(CloseCallback onClose) { mOnCloseCallback = onClose; }

    std::string peer() { return mContext.peer(); }

private:
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

    static const char* queueStateStr(QueueState state) {
        static const char* queueStateStr[] = {
                "CREATED",        "INCOMING_CONNECTION", "READ",    "WRITE",
                "WRITE_COMPLETE", "CANCELLED",           "FINISHED"};
        return queueStateStr[(int)state];
    }

    // These are tasks that are placed on the ServerCompletionQueue
    // the task encapsulates the connection, and which state we should
    // execute.
    //
    // We have the following set of invariants on the ordering in the
    // CompletionQueue of a given connection:
    //
    // incoming_connection < write
    // incoming_connection < read
    // write < write_complete
    // created < incoming_connection
    // created < cancelled
    //
    // Note in theory  writes can preempt reads and placed in the front of the
    // queue. We do our best to make sure this never happens.
    class CompletionQueueTask {
        using Connection = BidiGrpcConnection<R, W>;

    public:
        CompletionQueueTask(Connection* con, QueueState state)
            : mConnection(con), mState(state) {
            mConnection->mRefcount++;
            DD("Registered %p (%p,%d:%s)", this, mConnection,
               mConnection->mRefcount, queueStateStr(mState));
        }

        ~CompletionQueueTask() {
            int count = mConnection->mRefcount--;
            DD("Completed %p (%p,%d:%s)", this, mConnection, count,
               queueStateStr(mState));
            if (count == 0) {
                // We can now safely  delete the connection.
                delete mConnection;
            }
        }

        void execute() { mConnection->execute(mState); }

        QueueState getState() { return mState; }
        const char* getStateStr() { return queueStateStr(mState); }
        Connection* connection() { return mConnection; }

    private:
        Connection* mConnection;
        QueueState mState;
    };

    void start() {}

    // This function drives the actual state machine, and makes the appropriate
    // callbacks to a listener.
    //
    // We have the following states:
    //
    // created -> first_read -> read -------------->  closed
    //     |                       ^   |        ^
    //     |                       |---|        |
    //     |----> write --> write_complete -----|
    //               ^                     |
    //               |---------------------|
    //
    //
    void execute(QueueState state) {
        switch (state) {
            case QueueState::CREATED: {
                // We now need to actually register the proper stream endpoint.
                // The callback is used to setup the actual call.
                //
                // TODO(jansene): figure out how to use std::bind, so users
                // don't have to implement this.
                mRequestMethodFn(
                        &mContext, &mStream, mCq, mCq,
                        new CompletionQueueTask(
                                this, QueueState::INCOMING_CONNECTION));
                mContext.AsyncNotifyWhenDone(
                        new CompletionQueueTask(this, QueueState::CANCELLED));
                mOpen = true;
                break;
            }
            case QueueState::INCOMING_CONNECTION:
                if (mOpen) {
                    // Create another handler for a connection that can become
                    // active. This is the first time we are actually getting
                    // data from a connection, which means it actually becomes
                    // "alive".
                    //
                    // Ownership is transferred to the queue.
                    auto nxt =
                            new BidiGrpcConnection<R, W>(mCq, mRequestMethodFn);
                    mConnectAlarm.Set(
                            mCq, gpr_now(gpr_clock_type::GPR_CLOCK_REALTIME),
                            new CompletionQueueTask(nxt, QueueState::CREATED));

                    mStream.Read(&mIncoming, new CompletionQueueTask(
                                                     this, QueueState::READ));
                }
                break;
            case QueueState::READ: {
                // The actual message has arrived, notify the listener of the
                // incoming message and get ready to listen for the next
                // message.
                if (mOpen) {
                    if (mOnReadCallback) {
                        mOnReadCallback(this, mIncoming);
                    }

                    mStream.Read(&mIncoming, new CompletionQueueTask(
                                                     this, QueueState::READ));
                }
                break;
            }
            case QueueState::WRITE: {
                // Note: Writes are scheduled at the head, so this is going to
                // be the first task that will be picked up.
                const std::lock_guard<std::mutex> lock(mWriteQueueAccess);
                if (mOpen && !mWriteQueue.empty()) {
                    // Write out the actual object.
                    assert(mCanWrite);
                    mCanWrite = false;
                    mStream.Write(std::move(mWriteQueue.front()),
                                  new CompletionQueueTask(
                                          this, QueueState::WRITE_COMPLETE));
                    // Element will be popped once the write has completed..
                }
                break;
            }

            case QueueState::WRITE_COMPLETE: {
                // If we have more things to write we will schedule another
                // write at the end of our processing queue. This will make sure
                // writes do not starve reads.
                const std::lock_guard<std::mutex> lock(mWriteQueueAccess);
                mCanWrite = true;
                mWriteQueue.pop();
                if (mOpen && !mWriteQueue.empty()) {
                    mWriteAlarm.Set(
                            mCq, gpr_now(gpr_clock_type::GPR_CLOCK_REALTIME),
                            new CompletionQueueTask(this, QueueState::WRITE));
                }
                break;
            }

            case QueueState::CANCELLED: [[fallthrough]];
            case QueueState::FINISHED: {
                // Notify listeners of closing. TODO(jansene): Do we care about
                // finsihed/cancelled in bidi?
                mOpen = false;
                if (mOnCloseCallback) {
                    mOnCloseCallback(this);
                }
                break;
            }
        }
    }

private:
    ServerContext mContext;
    ServerCompletionQueue* mCq;

    RequestMethodFn mRequestMethodFn;

    // Generic..
    ServerAsyncReaderWriter<R, W> mStream;

    // The incoming object, this will be filled out in an async fashion.
    R mIncoming;

    // Set of objects to be written out.
    std::queue<W> mWriteQueue;
    CloseCallback mOnCloseCallback;
    ReadCallback mOnReadCallback;

    std::mutex mWriteQueueAccess;
    bool mWriteRequested{false};
    Alarm mWriteAlarm;
    Alarm mConnectAlarm;

    bool mOpen{false};
    bool mCanWrite{true};
    int mRefcount{-1};  // Number of times we are in the queue.
};

// A handler for async grpc connections.
//
// See android/android-grpc/android/emulation/control/test/TestEchoService.cpp
// for an example of how to implement an async streamEcho method
//
// Note, you want to keep this handler alive for as long you have your service
// up, otherwise you will not be processing events for the endpoint.
//
// Keep in mind that you will likely have to play with the number of threads.
// Incoming connections for example can only be processed by the INCOMING_STATE,
// of which there is usually only one per queue.
//
//
// TODO(jansene): Apply magic to generalize to unary, and non bidi connections.
template <class R, class W>
class AsyncGrpcHandler {
    using Connection = BidiGrpcConnection<R, W>;
    using QueueState = typename Connection::QueueState;
    using QueueTask = typename Connection::CompletionQueueTask;
    using OnConnectCallback = std::function<void(Connection*)>;
    using RequestFn = typename Connection::RequestMethodFn;

public:
    AsyncGrpcHandler(std::vector<ServerCompletionQueue*> queues,
                     RequestFn callback,
                     OnConnectCallback connectCb)
        : mCompletionQueues(queues),
          mOnConnect(connectCb),
          mRequestFn(callback) {}

    ~AsyncGrpcHandler() {
        for (auto& t : mRunners) {
            t.join();
        }
    }

    void start() {
        for (auto queue : mCompletionQueues) {
            mRunners.emplace_back(
                    std::thread([&, queue]() { runQueue(queue); }));
        }
    }

private:
    void runQueue(ServerCompletionQueue* queue) {
        // Ownership will be transferred to a queuetask..
        auto initialHandler = new Connection(queue, mRequestFn);
        initialHandler->execute(QueueState::CREATED);

        void* tag;  // uniquely identifies a request.
        bool ok;
        while (queue->Next(&tag, &ok)) {
            QueueTask* task = static_cast<QueueTask*>(tag);
            // 2 cases, it's ok to handle this request.
            // or we are done with it..
            if (ok) {
                DD("Ok for %p (%p:%s)", task, task->connection(),
                   task->getStateStr());
                // We now have an actual incoming connection, and we will start
                // scheduling incoming reads. We will notify listeners of the
                // new connection that we have.
                if (task->getState() == QueueState::INCOMING_CONNECTION) {
                    mOnConnect(task->connection());
                }
                task->execute();
            } else {
                // TODO(jansene): We could also immediately notify the connection
                // of a connection issues. Note: this happens very frequently, for example
                // when a client prematurely disconnects, it is too common to consider logging
                // it as an error.
                VERBOSE_PRINT(grpc, "Not ok (disconnected?) for %p (%p:%s)", task, task->connection(),
                   task->getStateStr());
            }
            delete task;
        }

        DD("Completed async queue.");
    }

    const std::vector<ServerCompletionQueue*> mCompletionQueues;
    OnConnectCallback mOnConnect;
    RequestFn mRequestFn;
    std::vector<std::thread> mRunners;
};

}  // namespace control
}  // namespace emulation
}  // namespace android