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

#include <chrono>         // for millise...
#include <functional>     // for __base
#include <memory>         // for make_un...
#include <mutex>          // for mutex...
#include <thread>         // for sleep_for
#include <unordered_set>  // for unorder...

#include "android/emulation/control/async/AsyncGrpcStream.h"  // for AsyncGr...
#include "android/emulation/control/test/TestEchoService.h"   // for AsyncHe...
#include "android/emulation/control/utils/EventWaiter.h"      // for EventWa...
#include "grpcpp/impl/codegen/async_stream.h"                 // for ServerA...
#include "grpcpp/impl/codegen/completion_queue.h"             // for ServerC...
#include "grpcpp/impl/codegen/server_context.h"               // for ServerC...
#include "grpcpp/impl/codegen/status.h"                       // for Status
#include "grpcpp/impl/codegen/sync_stream.h"                  // for ServerR...
#include "test_echo_service.pb.h"                             // for Msg

using grpc::ServerContext;
using grpc::Status;

namespace android {
namespace emulation {
namespace control {

class EventReceiver {
public:
    virtual ~EventReceiver() = default;
    virtual void eventArrived() = 0;
};

// A global heart beat...
// It basically beats every 500ms, and fires an event.
class Beat {
public:
    Beat() : mRunner([this]() { run(); }) {}

    ~Beat() {
        mRun = false;
        mRunner.join();
    }

    void stop() { mRun = false; }

    void addListener(EventReceiver* waiter) {
        const std::lock_guard<std::mutex> lock(mListenerLock);
        mListeners.insert(waiter);
    }

    void removeListener(EventReceiver* waiter) {
        const std::lock_guard<std::mutex> lock(mListenerLock);
        mListeners.erase(waiter);
    }

    int counter() { return mCounter; }

private:
    void run() {
        while (mRun) {
            std::this_thread::sleep_for(mTimeout);
            mCounter++;
            {
                const std::lock_guard<std::mutex> lock(mListenerLock);

                for (auto waiter : mListeners) {
                    waiter->eventArrived();
                }
            }
        }
    }

    int mCounter{0};
    bool mRun{true};
    std::thread mRunner;
    std::chrono::milliseconds mTimeout{5};
    std::mutex mListenerLock;
    std::unordered_set<EventReceiver*> mListeners;
};

std::unique_ptr<Beat> s_global_beat = std::make_unique<Beat>();

// A bridge from event receiver --> EventWatiter
class SyncHeartbeatReceiver : public EventWaiter, public EventReceiver {
public:
    void eventArrived() override { newEvent(); };
};

// The synchronous version of or our Heartbeat service.
::grpc::Status HeartbeatService::streamEcho(
        ::grpc::ServerContext* context,
        ::grpc::ServerReaderWriter<::android::emulation::control::Msg,
                                   ::android::emulation::control::Msg>*
                stream) {
    bool clientAvailable = !context->IsCancelled();
    auto frameEvent = std::make_unique<SyncHeartbeatReceiver>();
    s_global_beat->addListener(frameEvent.get());
    while (clientAvailable) {
        const auto kTimeToWaitForFrame = std::chrono::milliseconds(125);
        auto arrived = frameEvent->next(kTimeToWaitForFrame);
        if (arrived > 0 && !context->IsCancelled()) {
            Msg reply;
            reply.set_counter(s_global_beat->counter());
            clientAvailable = stream->Write(reply);
        }

        clientAvailable = !context->IsCancelled() && clientAvailable;
    }

    s_global_beat->removeListener(frameEvent.get());
    return Status::OK;
}

// The async version.
template <class Connection>
class AsyncHeartbeatReceiver {
public:
    AsyncHeartbeatReceiver() : mRunner([this]() { deliveryLoop(); }) {
        s_global_beat->addListener(&mReceiver);
    }

    ~AsyncHeartbeatReceiver() {
        s_global_beat->removeListener(&mReceiver);
        mRun = false;
        mRunner.join();
    }

    void deliveryLoop() {
        while (mRun) {
            const auto kTimeToWaitForFrame = std::chrono::milliseconds(125);

            if (mReceiver.next(kTimeToWaitForFrame) > 0) {
                Msg next;
                next.set_counter(s_global_beat->counter());
                {
                    const std::lock_guard<std::mutex> lock(mListenerLock);
                    for (auto waiter : mListeners) {
                        waiter->write(next);
                    }
                }
            }
        }
    }

    void addListener(Connection waiter) {
        const std::lock_guard<std::mutex> lock(mListenerLock);
        mListeners.insert(waiter);
    }

    void removeListener(Connection waiter) {
        const std::lock_guard<std::mutex> lock(mListenerLock);
        mListeners.erase(waiter);
    }

private:
    bool mRun{true};
    std::thread mRunner;

    int mCounter{0};
    std::mutex mListenerLock;

    SyncHeartbeatReceiver mReceiver;
    std::unordered_set<Connection> mListeners;
};

void registerAsyncHeartBeat(AsyncGrpcHandler* handler,
                            AsyncHeartbeatService* testService) {
    handler->registerConnectionHandler(
                   testService, &AsyncHeartbeatService::RequeststreamEcho)
            .withCallback(

                    [testService](auto connection) {
                        static AsyncHeartbeatReceiver<decltype(connection)>
                                s_global_handler;
                        s_global_handler.addListener(connection);
                        connection->setCloseCallback(
                                [testService](auto connection) {
                                    s_global_handler.removeListener(connection);
                                });
                    });
}

}  // namespace control
}  // namespace emulation
}  // namespace android
