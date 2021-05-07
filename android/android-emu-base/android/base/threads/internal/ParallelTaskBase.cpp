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

#include "android/base/threads/internal/ParallelTaskBase.h"

namespace android {
namespace base {
namespace internal {

ParallelTaskBase::ParallelTaskBase(Looper* looper,
                                   Looper::Duration checkTimeoutMs,
                                   ThreadFlags flags)
    : mLooper(looper),
      mManagedThread(this, flags) {}

bool ParallelTaskBase::start() {
    fprintf(stderr, "%s:%s:%d this=%p\n", "ParallelTaskBase", __func__, __LINE__, this);
    isRunning = true;
    if (!mManagedThread.start()) {
        fprintf(stderr, "%s:%s:%d this=%p\n", "ParallelTaskBase", __func__, __LINE__, this);
        isRunning = false;
        return false;
    } else {
        fprintf(stderr, "%s:%s:%d this=%p\n", "ParallelTaskBase", __func__, __LINE__, this);
        return true;
    }
}

bool ParallelTaskBase::inFlight() const {
    fprintf(stderr, "%s:%s:%d this=%p isRunning=%d\n", "ParallelTaskBase", __func__, __LINE__, this, isRunning.load());
    return isRunning;
}

}  // namespace internal
}  // namespace base
}  // namespace android
