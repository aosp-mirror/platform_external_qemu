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

#ifndef ANDROID_UTILS_THREAD_H
#define ANDROID_UTILS_THREAD_H

#include "android/base/Limits.h"
#include "android/utils/compiler.h"

// This is a simple C wrapper for the portable Thread object implemented in
// android/base/threads/Thread.h
// The C++ implementation is extensible by inheritence. This wrapper has limited
// extensibility -- users provide their own main and onExit function
// implemenetaions.
//
//     intptr_t mainFuncImpl() {
//         // The main function for the thread.
//     }
//
//     void onExitFuncImpl() {
//         // onExit function for the thread.
//     }
//
//     // Basic usage:
//     CThread* thread = thread_new(&mainFuncImpl, &onExitFuncImpl);
//     if (!thread_start(thread)) {
//         // handle error in launching thread.
//     }
//     intptr_t exitStatus;
//     if (thread_wait(thread, &exitStatus)) {
//         // Thread finished, result in exitStatus.
//     }
//     thread_free(thread);
//
// See android/base/threads/Thread.h for more details.

ANDROID_BEGIN_HEADER

typedef struct CThread CThread;
typedef intptr_t(CThreadMainFunc)(void);
typedef void(CThreadOnExitFunc)(void);

// Create a new opaque |CThread| object. |onExitFunc| can take |NULL| to
// indicate no onExit function.
CThread* thread_new(CThreadMainFunc* mainFunc, CThreadOnExitFunc* onExitFunc);

void thread_free(CThread* thread);

// See android::base::thread::Thread::start(...).
bool thread_start(CThread* thread);

// See android::base::thread::Thread::wait(...).
bool thread_wait(CThread* thread, intptr_t* exitStatus);

// See android::base::thread::Thread::tryWait(...).
bool thread_tryWait(CThread* thread, intptr_t* exitStatus);

ANDROID_END_HEADER

#endif  // ANDROID_UTILS_THREAD_H
