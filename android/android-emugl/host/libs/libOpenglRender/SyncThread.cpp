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
#include "android/utils/debug.h"
#include "emugl/common/crash_reporter.h"
#include "emugl/common/OpenGLDispatchLoader.h"
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

SyncThread::SyncThread() :
    emugl::Thread(android::base::ThreadFlags::MaskSignals, 512 * 1024) {
    this->start();
    initSyncContext();
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

void SyncThread::cleanup() {
    DPRINT("enter");
    SyncThreadCmd to_send;
    to_send.opCode = SYNC_THREAD_EXIT;
    sendAndWaitForResult(to_send);
    DPRINT("exit");
}

// Private methods below////////////////////////////////////////////////////////

void SyncThread::initSyncContext() {
    DPRINT("enter");
    SyncThreadCmd to_send;
    to_send.opCode = SYNC_THREAD_INIT;
    sendAndWaitForResult(to_send);
    DPRINT("exit");
}

intptr_t SyncThread::main() {
    DPRINT("in sync thread");

    bool exiting = false;
    uint32_t num_iter = 0;

    while (!exiting) {
        SyncThreadCmd cmd = {};

        DPRINT("waiting to receive command");
        mInput.receive(&cmd);
        num_iter++;

        DPRINT("sync thread @%p num iter: %u", this, num_iter);

        bool need_reply = cmd.needReply;
        int result = doSyncThreadCmd(&cmd);

        if (need_reply) {
            DPRINT("sending result");
            mOutput.send(result);
            DPRINT("sent result");
        }

        if (cmd.opCode == SYNC_THREAD_EXIT) exiting = true;
    }

    DPRINT("exited sync thread");
    return 0;
}

int SyncThread::sendAndWaitForResult(SyncThreadCmd& cmd) {
    DPRINT("send with opcode=%d", cmd.opCode);
    cmd.needReply = true;
    mInput.send(cmd);
    int result = -1;
    mOutput.receive(&result);
    DPRINT("result=%d", result);
    return result;
}

void SyncThread::sendAsync(SyncThreadCmd& cmd) {
    DPRINT("send with opcode=%u fenceSyncInfo=0x%llx",
           cmd.opCode, cmd.fenceSync);
    cmd.needReply = false;
    mInput.send(cmd);
}

void SyncThread::doSyncContextInit() {
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

    mSurface =
        egl->eglCreatePbufferSurface(mDisplay, config, pbufferAttribs);

    const EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    mContext = egl->eglCreateContext(mDisplay, config, EGL_NO_CONTEXT, contextAttribs);

    egl->eglMakeCurrent(mDisplay, mSurface, mSurface, mContext);
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

void SyncThread::doExit() {

    if (mContext == EGL_NO_CONTEXT) return;

    const EGLDispatch* egl = emugl::LazyLoadedEGLDispatch::get();

    egl->eglMakeCurrent(mDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    egl->eglDestroyContext(mDisplay, mContext);
    egl->eglDestroySurface(mDisplay, mContext);
    mContext = EGL_NO_CONTEXT;
    mSurface = EGL_NO_SURFACE;
}

int SyncThread::doSyncThreadCmd(SyncThreadCmd* cmd) {
    int result = 0;
    switch (cmd->opCode) {
    case SYNC_THREAD_INIT:
        DPRINT("exec SYNC_THREAD_INIT");
        doSyncContextInit();
        break;
    case SYNC_THREAD_WAIT:
        DPRINT("exec SYNC_THREAD_WAIT");
        doSyncWait(cmd);
        break;
    case SYNC_THREAD_EXIT:
        DPRINT("exec SYNC_THREAD_EXIT");
        doExit();
        break;
    }
    return result;
}

/* static */
SyncThread* SyncThread::get() {
    return sGlobalSyncThread->syncThreadPtr();
}
