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
#include "android/emulation/control/async/AsyncGrpcStream.h"

#include <grpcpp/grpcpp.h>  // for ServerCompletionQueue, Serv...
#include <algorithm>        // for max
#include <string>           // for string
#include <thread>           // for thread
#include <utility>          // for move
#include <vector>           // for vector

#include "android/utils/debug.h"          // for VERBOSE_PRINT, VERBOSE_grpc
#include "grpc/impl/codegen/gpr_types.h"  // for gpr_clock_type
#include "grpc/support/time.h"            // for gpr_now
#include "grpcpp/alarm.h"                 // for Alarm

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

BaseAsyncGrpcConnection::BaseAsyncGrpcConnection(ServerCompletionQueue* cq)
    : mCq(cq) {}

BaseAsyncGrpcConnection::~BaseAsyncGrpcConnection() {
    DD("Completed interaction with (%p) %s", this, mContext.peer().c_str());
}

// This function drives the actual state machine, and makes the
// appropriate callbacks to a listener.
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
void BaseAsyncGrpcConnection::execute(QueueState state) {
    switch (state) {
        case QueueState::CREATED: {
            // We now need to actually register the proper stream
            // endpoint. The callback is used to setup the actual call.
            initialize();
            mContext.AsyncNotifyWhenDone(
                    new BaseAsyncGrpcConnection::CompletionQueueTask(
                            this, QueueState::CANCELLED));
            mOpen = true;
            break;
        }
        case QueueState::INCOMING_CONNECTION:
            if (mOpen) {
                notifyConnect();
                // Create another handler for a connection that can
                // become active. This is the first time we are actually
                // getting data from a connection, which means it
                // actually becomes "alive".
                //
                // Ownership is transferred to the queue.
                auto nxt = clone();
                mConnectAlarm.Set(
                        mCq, gpr_now(gpr_clock_type::GPR_CLOCK_REALTIME),
                        new BaseAsyncGrpcConnection::CompletionQueueTask(
                                nxt, QueueState::CREATED));

                scheduleRead();
            }
            break;
        case QueueState::READ: {
            // The actual message has arrived, notify the listener of
            // the incoming message and get ready to listen for the next
            // message.
            if (mOpen) {
                notifyRead();
                scheduleRead();
            }
            break;
        }
        case QueueState::WRITE: {
            // Note: Writes are scheduled at the head, so this is going
            // to be the first task that will be picked up.
            scheduleWrite(WriteState::BEGIN);
            break;
        }

        case QueueState::WRITE_COMPLETE: {
            // If we have more things to write we will schedule another
            // write at the end of our processing queue. This will make
            // sure writes do not starve reads.
            scheduleWrite(WriteState::COMPLETE);
            break;
        }

        case QueueState::CANCELLED:
            [[fallthrough]];
        case QueueState::FINISHED: {
            // Notify listeners of closing. TODO(jansene): Do we care
            // about finsihed/cancelled in bidi?
            mOpen = false;
            notifyClose();
            break;
        }
    }
}

std::string BaseAsyncGrpcConnection::peer() {
    return mContext.peer();
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
// Note in theory  writes can preempt reads and placed in the front of
// the queue. We do our best to make sure this never happens.
BaseAsyncGrpcConnection::CompletionQueueTask::CompletionQueueTask(
        BaseAsyncGrpcConnection* con,
        QueueState state)
    : mConnection(con), mState(state) {
    mConnection->mRefcount++;
    DD("Registered %p (%p,%d:%s)", this, mConnection, mConnection->mRefcount,
       queueStateStr(mState));
}

const char* BaseAsyncGrpcConnection::CompletionQueueTask::queueStateStr(
        QueueState state) {
    static const char* queueStateStr[] = {
            "CREATED",        "INCOMING_CONNECTION", "READ",    "WRITE",
            "WRITE_COMPLETE", "CANCELLED",           "FINISHED"};
    return queueStateStr[(int)state];
}

BaseAsyncGrpcConnection::CompletionQueueTask::~CompletionQueueTask() {
    int count = mConnection->mRefcount--;
    DD("Completed %p (%p,%d:%s)", this, mConnection, count,
       queueStateStr(mState));
    if (count == 0) {
        // We can now safely  delete the connection.
        delete mConnection;
    }
}

void BaseAsyncGrpcConnection::CompletionQueueTask::execute() {
    mConnection->execute(mState);
}

BaseAsyncGrpcConnection::QueueState
BaseAsyncGrpcConnection::CompletionQueueTask::getState() {
    return mState;
}
const char* BaseAsyncGrpcConnection::CompletionQueueTask::getStateStr() {
    return queueStateStr(mState);
}

BaseAsyncGrpcConnection*
BaseAsyncGrpcConnection::CompletionQueueTask::connection() {
    return mConnection;
}

void AsyncGrpcHandler::runQueue(ServerCompletionQueue* queue) {
    // Ownership will be transferred to a queuetask..

    void* tag;  // uniquely identifies a request.
    bool ok;
    DD("Running queue: %p", queue);
    while (queue->Next(&tag, &ok)) {
        BaseAsyncGrpcConnection::CompletionQueueTask* task =
                static_cast<BaseAsyncGrpcConnection::CompletionQueueTask*>(tag);
        // 2 cases, it's ok to handle this request.
        // or we are done with it..
        if (ok) {
            DD("Ok for %p (%p:%s)", task, task->connection(),
               task->getStateStr());
            task->execute();
        } else {
            // TODO(jansene): We could also immediately notify the
            // connection of a connection issues. Note: this happens
            // very frequently, for example when a client prematurely
            // disconnects, it is too common to consider logging it as
            // an error.
            VERBOSE_PRINT(grpc, "Not ok (disconnected?) for %p (%p:%s)", task,
                          task->connection(), task->getStateStr());
        }
        delete task;
    }

    DD("Completed async queue.");
}

AsyncGrpcHandler::AsyncGrpcHandler(
        std::vector<std::unique_ptr<ServerCompletionQueue>> queues)
    : mCompletionQueues(std::move(queues)) {
    start();
}

AsyncGrpcHandler::~AsyncGrpcHandler() {
    VERBOSE_PRINT(grpc, "Taking down server queues");
    // Shutdown the completion queues.
    void* ignored_tag;
    bool ignored_ok;
    for (const auto& queue : mCompletionQueues) {
        queue->Shutdown();
    }

    // And wait for the threads to complete.
    for (auto& t : mRunners) {
        t.join();
    }
}

void AsyncGrpcHandler::start() {
    for (const auto& queue_ptr : mCompletionQueues) {
        ServerCompletionQueue* queue = queue_ptr.get();
        mRunners.emplace_back(std::thread([&, queue]() { runQueue(queue); }));
    }
}

}  // namespace control
}  // namespace emulation
}  // namespace android