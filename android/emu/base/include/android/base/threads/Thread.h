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
#include "android/base/threads/Types.h"
#include "android/base/synchronization/Lock.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

#include <inttypes.h>
#include <optional>
#include <string>

namespace android {
namespace base {

// Wrapper class for platform-specific threads.
// To create your own thread, define a sub-class of emugl::Thread
// and override its main() method.
//
// For example:
//
//    class MyThread : public emugl::Thread {
//    public:
//        MyThread() : Thread() {}
//
//        virtual intptr_t main() {
//            ... main thread loop implementation
//            return 0;
//        }
//    };
//
//    ...
//
//    // Create new instance, but does not start it.
//    MyThread* thread = new MyThread();
//
//    // Start the thread.
//    thread->start();
//
//    // Wait for thread completion, and gets result into |exitStatus|.
//    int exitStatus;
//    thread->wait(&exitStatus);
//
class Thread {
    DISALLOW_COPY_ASSIGN_AND_MOVE(Thread);

public:
    // Public constructor.
    Thread(ThreadFlags flags = ThreadFlags::MaskSignals, int stackSize = 0,
           std::optional<std::string> name = std::nullopt);

    // Virtual destructor.
    virtual ~Thread();

    // Override this method in your own thread sub-classes. This will
    // be called when start() is invoked on the Thread instance.
    virtual intptr_t main() = 0;

    // Override this if you need to execute some code after thread has
    // exited main() and is guaranteed not to access any of its members.
    // E.g. if you need to delete a thread object from the same thread
    // it has created
    virtual void onExit() {}

    // Start a thread instance. Return true on success, false otherwise
    // (e.g. if the thread was already started or terminated).
    bool start();

    // Wait for thread termination and retrieve exist status into
    // |*exitStatus|. Return true on success, false otherwise.
    // NOTE: |exitStatus| can be NULL.
    virtual bool  wait(intptr_t *exitStatus = nullptr);

    // Check whether a thread has terminated. On success, return true
    // and sets |*exitStatus|. On failure, return false.
    // NOTE: |exitStatus| can be NULL.
    bool tryWait(intptr_t *exitStatus);

    // Mask all signals for the current thread
    // This is needed for the qemu guest to run properly
    // NB: noop for Win32
    static void maskAllSignals();

    // Sleep for |n| milliseconds
    static void sleepMs(unsigned n);

    // Sleep for |n| microseconds
    static void sleepUs(unsigned n);

    // Yield the remaining part of current thread's CPU time slice to another
    // thread that's ready to run.
    static void yield();

private:
#ifdef _WIN32
    static DWORD WINAPI thread_main(void* arg);

    HANDLE mThread = nullptr;
#else // !WIN32
    static void* thread_main(void* arg);

    pthread_t mThread;
#endif
    Lock mLock;
    // Access guarded by |mLock|.
    intptr_t mExitStatus = 0;
    int mStackSize;
    const ThreadFlags mFlags;
    bool mStarted = false;
    // Access guarded by |mLock|.
    bool mFinished = false;
    std::optional<std::string> mNameOpt = std::nullopt;
#ifndef _WIN32
    // Posix-only, remember if we've joined our non-detached thread already.
    bool mJoined = false;
#endif
};

// Helper function to obtain a printable id for the current thread.
unsigned long getCurrentThreadId();

// Helper function to set/get/ui thread status.
void setUiThreadId(unsigned long id);
bool isRunningInUiThread();

// Subclass of Thread covering interruptible threads.
class InterruptibleThread : public Thread {
    DISALLOW_COPY_ASSIGN_AND_MOVE(InterruptibleThread);
public:
    // Public constructor.
    InterruptibleThread(ThreadFlags flags = ThreadFlags::MaskSignals, int stackSize = 0) :
        Thread(flags, stackSize) { }

    virtual void interrupt() = 0;
};

}  // namespace base
}  // namespace android
