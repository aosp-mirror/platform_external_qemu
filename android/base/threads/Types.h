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

#include <functional>
#include <stdint.h>

namespace android {
namespace base {

// a functor which can run in a separate thread
using ThreadFunctor = std::function<intptr_t()>;

enum class ThreadFlags {
    None = 0,
    MaskSignals = 1
};

// enum class doesn't allow bit operations unless there's an
// operator
inline ThreadFlags operator|(ThreadFlags l, ThreadFlags r) {
    return static_cast<ThreadFlags>(int(l) | int(r));
}

inline ThreadFlags operator&(ThreadFlags l, ThreadFlags r) {
    return static_cast<ThreadFlags>(int(l) & int(r));
}

}  // namespace base
}  // namespace android
