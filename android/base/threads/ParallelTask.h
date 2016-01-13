// Copyright (C) 2015 The Android Open Source Project
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

#include "android/base/Compiler.h"
#include "android/base/async/Looper.h"
#include "android/base/threads/internal/ParallelTaskBase.h"
#include "android/base/threads/Types.h"

#include <memory>
#include <utility>

namespace android {
namespace base {

// A ParallelTask<Result> is an object that allows you to launch a thread from a
// main thread that has an associated looper, and then take follow up action on
// the looper once the thread finishes. Additionally, the |onJoined| function is
// called with a result of type |Result| returned from the launched thread.
//
//
// An example of a thread returning the typical int exitStatus:
//
//   class MyThread : public android::base::ParallelTask<int> {
//      intptr_t main(int* exitStatus) overrride {
//          *exitStatus = 42;
//      }
//
//      void onJoined(const int& exitStatus) override {
//          cout << "Our special thread finished with " << exitStatus;
//      }
//   };
//
//   int main() {
//      unique_ptr<android::base::ParallelTask> myThread(new MyThread(looper));
//
//      // |threadObj| will execute |main| on a separate thread. Then, it will
//      join thread on a function called from the event loop, which will call
//      |onJoined|.
//      myThread->start();
//
//      // Or, additionally, when |onJoined| finishes, |threadObj| can be
//      automatically deleted.
//      ParallelTask::fireAndForget(std::move(myThread));
//   }
//
template <class ResultType>
class ParallelTask : public internal::ParallelTaskBase {
public:
    // Args:
    //     looper: A running looper for the current thread.
    //     checkTimeoutMs: The time in milliseconds between consecutive checks
    //             for thread termination. A bigger value possibly delayes the
    //             call to |onJoined|, but leads to fewer checks.
    //             Default: 1 second.
    //     flags: See android::base::Thread.
    ParallelTask(android::base::Looper* looper,
                 android::base::Looper::Duration checkTimeoutMs = 1 * 1000,
                 ThreadFlags flags = ThreadFlags::MaskSignals)
        : ParallelTaskBase(looper, checkTimeoutMs, flags) {}

    // Start the thread instance. Returns true on success, false otherwise.
    // (e.g. if the thread was already started or terminated).
    bool start() { return ParallelTaskBase::start(); }

    // Returns true if the thread has been |start|'ed, but the call to
    // |onJoined| hasn't finished yet.
    // This function should be called from the same thread that called |start|.
    // It is an error to call this function on a |fireAndForget|ed thread.
    bool inFlight() const { return ParallelTaskBase::inFlight(); }

    // |start| a thread, and request that |this| be garbage collected whenever
    // the thread has finished. This is useful if you don't want to remember to
    // delete the object after the thread finishes |onJoined|.
    //
    // NOTE: A call to |fireAndForget| **always** invalidates |*thread|. It's an
    // error to use |this| object from the caller after a call to
    // |fireAndForget|. (We now just take ownership to make it explicit).
    static bool fireAndForget(std::unique_ptr<ParallelTask> thread) {
        return ParallelTaskBase::fireAndForget(std::move(thread));
    }

protected:
    // Override this method in your own sub-classes. This will
    // be called when |fireAndForget| is invoked on the Thread instance.
    virtual void main(ResultType* outResult) = 0;

    // Override this method in your own sub-classes. This will be dispatched on
    // |looper| after |main| finishes on the |fireAndForget|ed thread.
    // The exit status from the thread is provided as argument.
    virtual void onJoined(const ResultType& result){};

protected:
    void mainImpl() override final { main(&mResultBuffer); }

    void onJoinedImpl() override final { onJoined(mResultBuffer); }

private:
    ResultType mResultBuffer;

    DISALLOW_COPY_AND_ASSIGN(ParallelTask);
};

}  // namespace base
}  // namespace android
