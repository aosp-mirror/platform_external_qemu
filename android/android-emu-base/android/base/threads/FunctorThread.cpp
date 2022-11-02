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

#include "aemu/base/threads/FunctorThread.h"

#include <assert.h>

namespace android {
namespace base {

FunctorThread::FunctorThread(Functor&& func, ThreadFlags flags)
    : Thread(flags)
    , mThreadFunc(std::move(func)) {
    assert(mThreadFunc);
}

intptr_t FunctorThread::main() {
    return mThreadFunc();
}

}  // namespace base
}  // namespace android
