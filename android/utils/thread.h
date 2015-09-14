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

ANDROID_BEGIN_HEADER

typedef struct CThread CThread;
typedef intptr_t(CThreadMainFunc)(void);
typedef void(CThreadOnExitFunc)(void);

CThread* thread_new(CThreadMainFunc* mainFunc, CThreadOnExitFunc* onExitFunc);
void thread_free(CThread* thread);

bool thread_start(CThread* thread);
bool thread_wait(CThread* thread, intptr_t* exitStatus);
bool thread_tryWait(CThread* thread, intptr_t* exitStatus);

ANDROID_END_HEADER

#endif  // ANDROID_UTILS_THREAD_H
