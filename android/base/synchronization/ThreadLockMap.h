// Copyright (C) 2014 The Android Open Source Project
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
#include "android/base/threads/ThreadStore.h"

#include <unordered_map>

// android::base::LockMap provides a thread-local
// std::unordered_map from void* to int.
// void* represents the lock object.
// This is to keep track of each thread's
// lock objects, useful for accounting for
// recursive locking.

namespace android {
namespace base {

typedef std::unordered_map<void*, int> ThreadLockMap;
typedef android::base::ThreadStore<ThreadLockMap> ThreadLockMapStore;

ThreadLockMap& getLockMap();
void addLockMapEntry(void*);
void removeLockMapEntry(void*);

} // namespace base
} // namespace android


