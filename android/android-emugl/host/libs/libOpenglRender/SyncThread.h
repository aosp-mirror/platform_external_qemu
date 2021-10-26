/*
* Copyright (C) 2016 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#pragma once

#include "FenceSync.h"

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "android/base/Optional.h"
#include "android/base/synchronization/ConditionVariable.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/synchronization/MessageChannel.h"

#include "android/base/threads/ThreadPool.h"
#include "emugl/common/thread.h"
#include "vulkan/VkDecoderGlobalState.h"

// SyncThread///////////////////////////////////////////////////////////////////
// The purpose of SyncThread is to track sync device timelines and give out +
// signal FD's that correspond to the completion of host-side GL fence commands.

// We communicate with the sync thread in 3 ways:
enum SyncThreadOpCode {
    // Nonblocking command to wait on a given FenceSync object
    // and timeline handle.
    // A fence FD object in the guest is signaled.
    SYNC_THREAD_WAIT = 0,
    // Blocking command to clean up and exit the sync thread.
    SYNC_THREAD_EXIT = 1,
    // Blocking command to wait on a given FenceSync object.
    // No timeline handling is done.
    SYNC_THREAD_BLOCKED_WAIT_NO_TIMELINE = 2,
    // Nonblocking command to wait on a given VkFence
    // and timeline handle.
    // A fence FD object / Zircon eventpair in the guest is signaled.
    SYNC_THREAD_WAIT_VK = 3,
};

struct SyncThreadCmd {
    // For use with initialization in multiple thread pools.
    int workerId = 0;
    // For use with ThreadPool::broadcastIndexed
    void setIndex(int id) { workerId = id; }

    SyncThreadOpCode opCode = SYNC_THREAD_WAIT;
    union {
        FenceSync* fenceSync = nullptr;
        VkFence vkFence;
    };
    uint64_t timeline = 0;

    android::base::Lock* lock = nullptr;
    android::base::ConditionVariable* cond = nullptr;
    android::base::Optional<int>* result = nullptr;
};

struct RenderThreadInfo;
class SyncThread : public emugl::Thread {
public:
    // - constructor: start up the sync worker threads for a given context.
    // The initialization of the sync threads is nonblocking.
    // - Triggers a |SyncThreadCmd| with op code |SYNC_THREAD_INIT|
    SyncThread();
    ~SyncThread();

    // |triggerWait|: async wait with a given FenceSync object.
    // We use the wait() method to do a eglClientWaitSyncKHR.
    // After wait is over, the timeline will be incremented,
    // which should signal the guest-side fence FD.
    // This method is how the goldfish sync virtual device
    // knows when to increment timelines / signal native fence FD's.
    void triggerWait(FenceSync* fenceSync,
                     uint64_t timeline);

    // |triggerWaitVk|: async wait with a given VkFence object.
    // The |vkFence| argument is a *boxed* host Vulkan handle of the fence.
    //
    // We call vkWaitForFences() on host Vulkan device to wait for the fence.
    // After wait is over, the timeline will be incremented,
    // which should signal the guest-side fence FD / Zircon eventpair.
    // This method is how the goldfish sync virtual device
    // knows when to increment timelines / signal native fence FD's.
    void triggerWaitVk(VkFence vkFence, uint64_t timeline);

    // for use with the virtio-gpu path; is meant to have a current context
    // while waiting.
    void triggerBlockedWaitNoTimeline(FenceSync* fenceSync);

    // |cleanup|: for use with destructors and other cleanup functions.
    // it destroys the sync context and exits the sync thread.
    // This is blocking; after this function returns, we're sure
    // the sync thread is gone.
    // - Triggers a |SyncThreadCmd| with op code |SYNC_THREAD_EXIT|
    void cleanup();

    // Obtains the global sync thread.
    static SyncThread* get();

    // Destroys and recreates the sync thread, for use on snapshot load.
    static void destroy();
    static void recreate();

private:

    // Thread function.
    // It keeps the workers runner until |mExiting| is set.
    virtual intptr_t main() override final;

    // These two functions are used to communicate with the sync thread
    // from another thread:
    // - |sendAndWaitForResult| issues |cmd| to the sync thread,
    //   and blocks until it receives the result of the command.
    // - |sendAsync| issues |cmd| to the sync thread and does not
    //   wait for the result, returning immediately after.
    int sendAndWaitForResult(SyncThreadCmd& cmd);
    void sendAsync(SyncThreadCmd& cmd);

    // |doSyncThreadCmd| and related functions below
    // execute the actual commands. These run on the sync thread.
    int doSyncThreadCmd(SyncThreadCmd* cmd);
    void doSyncContextInit(SyncThreadCmd* cmd);
    void doSyncWait(SyncThreadCmd* cmd);
    int doSyncWaitVk(SyncThreadCmd* cmd);
    void doSyncBlockedWaitNoTimeline(SyncThreadCmd* cmd);
    void doExit(SyncThreadCmd* cmd);

    // EGL objects / object handles specific to
    // a sync thread.
    static const uint32_t kNumWorkerThreads = 4u;

    EGLDisplay mDisplay = EGL_NO_DISPLAY;
    EGLSurface mSurface[kNumWorkerThreads];
    EGLContext mContext[kNumWorkerThreads];

    bool mExiting = false;
    android::base::Lock mLock;
    android::base::ConditionVariable mCv;
    android::base::ThreadPool<SyncThreadCmd> mWorkerThreadPool;
};

