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
#include "android/base/threads/internal/LooperThreadBase.h"
#include "android/base/threads/Types.h"

#include <memory>

namespace android {
namespace base {

// A LooperThread<Result> is an object that allows you to launch a thread from a
// main thread that has an associated looper, and then take follow up action on
// the looper once the thread finishes. Additionally, the |onJoined| function is
// called with a result of type |Result| returned from the launched thread.
//
//
// An example of a thread returning the typical int exitStatus:
//
//   class MyThread : public android::base::LooperThread<int> {
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
//      unique_ptr<android::base::LooperThread> myThread(new MyThread(looper));
//
//      // |threadObj| will execute |main| on a separate thread. Then, it will
//      join thread on a function called from the event loop, which will call
//      |onJoined|.
//      myThread->start();
//
//      // Or, additionally, when |onJoined| finishes, |threadObj| can be
//      automatically deleted.
//      LooperThread::fireAndForget(std::move(myThread));
//   }
//
template <class ResultType>
class LooperThread : public internal::LooperThreadBase {
public:
    // Args:
    //     looper: A running looper for the current thread.
    //     checkTimeoutMs: The time in milliseconds between consecutive checks
    //             for thread termination. A bigger value possibly delayes the
    //             call to |onJoined|, but leads to fewer checks.
    //             Default: 10 seconds.
    //     flags: See android::base::Thread.
    LooperThread(android::base::Looper* looper,
                 android::base::Looper::Duration checkTimeoutMs = 10 * 1000,
                 ThreadFlags flags = ThreadFlags::MaskSignals)
        : LooperThreadBase(looper, checkTimeoutMs, flags),
          mResultBuffer(new ResultType) {}

    // Start the thread instance. Returns true on success, false otherwise.
    // (e.g. if the thread was already started or terminated).
    bool start() { return LooperThreadBase::start(); }

    // Returns true if the thread has been |start|'ed, but the call to
    // |onJoined| hasn't finished yet.
    // This function should be called from the same thread that called |start|.
    // It is an error to call this function on a |fireAndForget|ed thread.
    bool inFlight() const { return LooperThreadBase::inFlight(); }

    // |start| a thread, and request that |this| be garbage collected whenever
    // the thread has finished. This is useful if you don't want to remember to
    // delete the object after the thread finishes |onJoined|.
    //
    // This comes at a cost: If the thread that calls |fireAndForget| dies
    // before |onJoined| has finished, |this| will be leaked.
    //
    // NOTE: A call to |fireAndForget| **always** invalidates |this|. It's an
    // error to use |this| object from the caller after a call to
    // |fireAndForget|. (We now just take ownership to make it explicit).
    static bool fireAndForget(std::unique_ptr<LooperThread> thread) {
        return LooperThreadBase::fireAndForget(std::move(thread));
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
    void mainImpl() override final { main(mResultBuffer.get()); }

    void onJoinedImpl() override final { onJoined(*mResultBuffer.get()); }

private:
    std::unique_ptr<ResultType> mResultBuffer;

    DISALLOW_COPY_AND_ASSIGN(LooperThread);
};

}  // namespace base
}  // namespace android
