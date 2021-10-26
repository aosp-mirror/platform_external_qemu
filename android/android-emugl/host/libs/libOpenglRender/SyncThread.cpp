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

#include "SyncThread.h"

#include "android/base/memory/LazyInstance.h"
#include "android/base/system/System.h"
#include "android/base/threads/Thread.h"
#include "android/utils/debug.h"
#include "emugl/common/OpenGLDispatchLoader.h"
#include "emugl/common/crash_reporter.h"
#include "emugl/common/sync_device.h"

#ifndef _MSC_VER
#include <sys/time.h>
#endif
#include <memory>

#define DEBUG 0

#if DEBUG

static uint64_t curr_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_usec / 1000 + tv.tv_sec * 1000;
}

#define DPRINT(fmt, ...) do { \
    if (!VERBOSE_CHECK(syncthreads)) VERBOSE_ENABLE(syncthreads); \
    VERBOSE_TID_FUNCTION_DPRINT(syncthreads, "@ time=%llu: " fmt, curr_ms(), ##__VA_ARGS__); \
} while(0)

#else

#define DPRINT(...)

#endif

using android::base::LazyInstance;

// The single global sync thread instance.
class GlobalSyncThread {
public:
    GlobalSyncThread() = default;

    SyncThread* syncThreadPtr() { return &mSyncThread; }

private:
    SyncThread mSyncThread;
};

static LazyInstance<GlobalSyncThread> sGlobalSyncThread = LAZY_INSTANCE_INIT;

static const uint32_t kTimelineInterval = 1;
static const uint64_t kDefaultTimeoutNsecs = 5ULL * 1000ULL * 1000ULL * 1000ULL;

SyncThread::SyncThread()
    : emugl::Thread(android::base::ThreadFlags::MaskSignals, 512 * 1024),
      mWorkerThreadPool(kNumWorkerThreads, [this](SyncThreadCmd&& cmd) {
          doSyncThreadCmd(&cmd);
      }) {
    this->start();
    mWorkerThreadPool.start();
}

SyncThread::~SyncThread() {
    cleanup();
}

void SyncThread::triggerWait(FenceSync* fenceSync,
                             uint64_t timeline) {
    DPRINT("fenceSyncInfo=0x%llx timeline=0x%lx ...",
            fenceSync, timeline);
    SyncThreadCmd to_send;
    to_send.opCode = SYNC_THREAD_WAIT;
    to_send.fenceSync = fenceSync;
    to_send.timeline = timeline;
    DPRINT("opcode=%u", to_send.opCode);
    sendAsync(to_send);
    DPRINT("exit");
}

void SyncThread::triggerWaitVk(VkFence vkFence, uint64_t timeline) {
    DPRINT("fenceSyncInfo=0x%llx timeline=0x%lx ...", fenceSync, timeline);
    SyncThreadCmd to_send;
    to_send.opCode = SYNC_THREAD_WAIT_VK;
    to_send.vkFence = vkFence;
    to_send.timeline = timeline;
    DPRINT("opcode=%u", to_send.opCode);
    sendAsync(to_send);
    DPRINT("exit");
}

void SyncThread::triggerBlockedWaitNoTimeline(FenceSync* fenceSync) {
    DPRINT("fenceSyncInfo=0x%llx ...", fenceSync);
    SyncThreadCmd to_send;
    to_send.opCode = SYNC_THREAD_BLOCKED_WAIT_NO_TIMELINE;
    to_send.fenceSync = fenceSync;
    DPRINT("opcode=%u", to_send.opCode);
    sendAndWaitForResult(to_send);
    DPRINT("exit");
}

void SyncThread::cleanup() {
    DPRINT("enter");
    SyncThreadCmd to_send;
    to_send.opCode = SYNC_THREAD_EXIT;
    sendAndWaitForResult(to_send);
    DPRINT("signal");
    mLock.lock();
    mExiting = true;
    mCv.signalAndUnlock(&mLock);
    DPRINT("exit");
}

// Private methods below////////////////////////////////////////////////////////

intptr_t SyncThread::main() {
    DPRINT("in sync thread");
    mLock.lock();
    mCv.wait(&mLock, [this] { return mExiting; });

    mWorkerThreadPool.done();
    mWorkerThreadPool.join();
    DPRINT("exited sync thread");
    return 0;
}

int SyncThread::sendAndWaitForResult(SyncThreadCmd& cmd) {
    DPRINT("send with opcode=%d", cmd.opCode);
    android::base::Lock lock;
    android::base::ConditionVariable cond;
    android::base::Optional<int> result = android::base::kNullopt;
    cmd.lock = &lock;
    cmd.cond = &cond;
    cmd.result = &result;

    lock.lock();
    mWorkerThreadPool.enqueue(std::move(cmd));
    cond.wait(&lock, [&result] { return result.hasValue(); });

    DPRINT("result=%d", *result);
    return *result;
}

void SyncThread::sendAsync(SyncThreadCmd& cmd) {
    DPRINT("send with opcode=%u fenceSyncInfo=0x%llx",
           cmd.opCode, cmd.fenceSync);
    mWorkerThreadPool.enqueue(std::move(cmd));
}

void SyncThread::doSyncContextInit(SyncThreadCmd* cmd) {
    const EGLDispatch* egl = emugl::LazyLoadedEGLDispatch::get();

    mDisplay = egl->eglGetDisplay(EGL_DEFAULT_DISPLAY);
    int eglMaj, eglMin;
    egl->eglInitialize(mDisplay, &eglMaj , &eglMin);

    const EGLint configAttribs[] = {
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_NONE,
    };

    EGLint nConfigs;
    EGLConfig config;

    egl->eglChooseConfig(mDisplay, configAttribs, &config, 1, &nConfigs);

    const EGLint pbufferAttribs[] = {
        EGL_WIDTH, 1,
        EGL_HEIGHT, 1,
        EGL_NONE,
    };

    mSurface[cmd->workerId] =
        egl->eglCreatePbufferSurface(mDisplay, config, pbufferAttribs);

    const EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    mContext[cmd->workerId] = egl->eglCreateContext(mDisplay, config, EGL_NO_CONTEXT, contextAttribs);

    egl->eglMakeCurrent(mDisplay, mSurface[cmd->workerId], mSurface[cmd->workerId], mContext[cmd->workerId]);
}

void SyncThread::doSyncWait(SyncThreadCmd* cmd) {
    DPRINT("enter");

    FenceSync* fenceSync =
        FenceSync::getFromHandle((uint64_t)(uintptr_t)cmd->fenceSync);

    if (!fenceSync) {
        emugl::emugl_sync_timeline_inc(cmd->timeline, kTimelineInterval);
        return;
    }

    EGLint wait_result = 0x0;

    DPRINT("wait on sync obj: %p", cmd->fenceSync);
    wait_result = cmd->fenceSync->wait(kDefaultTimeoutNsecs);

    DPRINT("done waiting, with wait result=0x%x. "
           "increment timeline (and signal fence)",
           wait_result);

    if (wait_result != EGL_CONDITION_SATISFIED_KHR) {
        DPRINT("error: eglClientWaitSync abnormal exit 0x%x\n",
               wait_result);
    }

    DPRINT("issue timeline increment");

    // We always unconditionally increment timeline at this point, even
    // if the call to eglClientWaitSync returned abnormally.
    // There are three cases to consider:
    // - EGL_CONDITION_SATISFIED_KHR: either the sync object is already
    //   signaled and we need to increment this timeline immediately, or
    //   we have waited until the object is signaled, and then
    //   we increment the timeline.
    // - EGL_TIMEOUT_EXPIRED_KHR: the fence command we put in earlier
    //   in the OpenGL stream is not actually ever signaled, and we
    //   end up blocking in the above eglClientWaitSyncKHR call until
    //   our timeout runs out. In this case, provided we have waited
    //   for |kDefaultTimeoutNsecs|, the guest will have received all
    //   relevant error messages about fence fd's not being signaled
    //   in time, so we are properly emulating bad behavior even if
    //   we now increment the timeline.
    // - EGL_FALSE (error): chances are, the underlying EGL implementation
    //   on the host doesn't actually support fence objects. In this case,
    //   we should fail safe: 1) It must be only very old or faulty
    //   graphics drivers / GPU's that don't support fence objects.
    //   2) The consequences of signaling too early are generally, out of
    //   order frames and scrambled textures in some apps. But, not
    //   incrementing the timeline means that the app's rendering freezes.
    //   So, despite the faulty GPU driver, not incrementing is too heavyweight a response.

    emugl::emugl_sync_timeline_inc(cmd->timeline, kTimelineInterval);
    FenceSync::incrementTimelineAndDeleteOldFences();

    DPRINT("done timeline increment");

    DPRINT("exit");
}

int SyncThread::doSyncWaitVk(SyncThreadCmd* cmd) {
    DPRINT("enter");

    auto decoder = goldfish_vk::VkDecoderGlobalState::get();
    auto result = decoder->waitForFence(cmd->vkFence, kDefaultTimeoutNsecs);
    if (result == VK_TIMEOUT) {
        fprintf(stderr, "SyncThread::%s: SYNC_WAIT_VK timeout: vkFence=%p\n",
                __func__, cmd->vkFence);
    } else if (result != VK_SUCCESS) {
        fprintf(stderr, "SyncThread::%s: SYNC_WAIT_VK error: %d vkFence=%p\n",
                __func__, result, cmd->vkFence);
    }

    DPRINT("issue timeline increment");

    // We always unconditionally increment timeline at this point, even
    // if the call to vkWaitForFences returned abnormally.
    // See comments in |doSyncWait| about the rationale.
    emugl::emugl_sync_timeline_inc(cmd->timeline, kTimelineInterval);

    DPRINT("done timeline increment");

    DPRINT("exit");
    return result;
}

void SyncThread::doSyncBlockedWaitNoTimeline(SyncThreadCmd* cmd) {
    DPRINT("enter");

    FenceSync* fenceSync =
        FenceSync::getFromHandle((uint64_t)(uintptr_t)cmd->fenceSync);

    if (!fenceSync) {
        return;
    }

    EGLint wait_result = 0x0;

    DPRINT("wait on sync obj: %p", cmd->fenceSync);
    wait_result = cmd->fenceSync->wait(kDefaultTimeoutNsecs);

    DPRINT("done waiting, with wait result=0x%x. "
           "increment timeline (and signal fence)",
           wait_result);

    if (wait_result != EGL_CONDITION_SATISFIED_KHR) {
        fprintf(stderr, "error: eglClientWaitSync abnormal exit 0x%x\n",
                wait_result);
    }
}

void SyncThread::doExit(SyncThreadCmd* cmd) {

    const EGLDispatch* egl = emugl::LazyLoadedEGLDispatch::get();

    egl->eglMakeCurrent(mDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

    egl->eglDestroyContext(mDisplay, mContext[cmd->workerId]);
    egl->eglDestroySurface(mDisplay, mSurface[cmd->workerId]);
    mContext[cmd->workerId] = EGL_NO_CONTEXT;
    mSurface[cmd->workerId] = EGL_NO_SURFACE;
}

int SyncThread::doSyncThreadCmd(SyncThreadCmd* cmd) {
#if DEBUG
    thread_local static auto threadId = android::base::getCurrentThreadId();
    thread_local static size_t numCommands = 0U;
    DPRINT("threadId = %lu numCommands = %lu cmd = %p", threadId, ++numCommands,
           cmd);
#endif  // DEBUG

    int result = 0;
    if (!mContext[cmd->workerId]) {
        doSyncContextInit(cmd);
    }
    switch (cmd->opCode) {
    case SYNC_THREAD_WAIT:
        DPRINT("exec SYNC_THREAD_WAIT");
        doSyncWait(cmd);
        break;
    case SYNC_THREAD_WAIT_VK:
        DPRINT("exec SYNC_THREAD_WAIT_VK");
        result = doSyncWaitVk(cmd);
        break;
    case SYNC_THREAD_EXIT:
        DPRINT("exec SYNC_THREAD_EXIT");
        doExit(cmd);
        break;
    case SYNC_THREAD_BLOCKED_WAIT_NO_TIMELINE:
        DPRINT("exec SYNC_THREAD_BLOCKED_WAIT_NO_TIMELINE");
        doSyncBlockedWaitNoTimeline(cmd);
        break;
    }

    bool need_reply = cmd->lock != nullptr && cmd->cond != nullptr;
    if (need_reply) {
        cmd->lock->lock();
        *cmd->result = android::base::makeOptional(result);
        cmd->cond->signalAndUnlock(cmd->lock);
    }
    return result;
}

/* static */
SyncThread* SyncThread::get() {
    return sGlobalSyncThread->syncThreadPtr();
}
