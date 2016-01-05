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

#include "android/base/threads/Async.h"

#include "android/base/threads/FunctorThread.h"
#include "android/base/threads/ThreadStore.h"

#include <memory>

#include <inttypes.h>

namespace android {
namespace base {

namespace {

class SelfDeletingThread final : public FunctorThread {
public:
    using FunctorThread::FunctorThread;

    intptr_t main() override {
        // This ensures that this object is deleted automatically when this
        // object exits.
        myStore.set(this);
        return FunctorThread::main();
    }

private:
    ThreadStore<SelfDeletingThread> myStore;
};

}

bool async(const ThreadFunctor& func, ThreadFlags flags) {
    auto thread =
            std::unique_ptr<SelfDeletingThread>(
                new SelfDeletingThread(func, flags));
    if (thread->start()) {
        thread.release();
        return true;
    }
    return false;
}

}  // namespace base
}  // namespace android
