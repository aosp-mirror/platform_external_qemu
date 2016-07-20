
#pragma once

#include "android/base/async/Looper.h"
#include "android/base/async/RecurrentTask.h"
#include "android/base/async/ThreadLooper.h"
#include "android/base/threads/Thread.h"
#include "android/emulation/android_pipe_common.h"
#include "android/emulation/AndroidPipe.h"

class WakeThread : public android::base::Thread {
public:
    WakeThread();
    WakeThread(android::AndroidPipe* pipe);
    bool wakePipe();
    virtual intptr_t main() override;
    android::AndroidPipe* mPipe;
    android::base::Looper* mLooper;
    android::base::RecurrentTask* mTask;
    bool exiting;
};
