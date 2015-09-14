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

#include "android/utils/thread.h"

#include "android/base/Compiler.h"
#include "android/base/threads/Thread.h"

namespace {

class CThreadImpl : public android::base::Thread {
public:
    CThreadImpl(CThreadMainFunc* mainFunc, CThreadOnExitFunc* onExitFunc = NULL)
        : mMainFunc(mainFunc), mOnExitFunc(onExitFunc) {}

    virtual intptr_t main() { return (*mMainFunc)(); }

    virtual void onExit() {
        if (mOnExitFunc != NULL) {
            (*mOnExitFunc)();
        }
    }

private:
    CThreadMainFunc* mMainFunc;
    CThreadOnExitFunc* mOnExitFunc;

    DISALLOW_COPY_AND_ASSIGN(CThreadImpl);
};

}  // namespace

static CThreadImpl* asCThreadImpl(CThread* thread) {
    return reinterpret_cast<CThreadImpl*>(thread);
}

static CThread* asCThread(CThreadImpl* thread) {
    return reinterpret_cast<CThread*>(thread);
}

CThread* thread_new(CThreadMainFunc* mainFunc, CThreadOnExitFunc* onExitFunc) {
    return asCThread(new CThreadImpl(mainFunc, onExitFunc));
}

void thread_free(CThread* thread) {
    delete asCThreadImpl(thread);
}

bool thread_start(CThread* thread) {
    return asCThreadImpl(thread)->start();
}

bool thread_wait(CThread* thread, intptr_t* exitStatus) {
    return asCThreadImpl(thread)->wait(exitStatus);
}

bool thread_tryWait(CThread* thread, intptr_t* exitStatus) {
    return asCThreadImpl(thread)->tryWait(exitStatus);
}
