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

#include "DispatchTables.h"
#include "FrameBuffer.h"
#include "RenderThreadInfo.h"

#include "OpenGLESDispatch/EGLDispatch.h"

#include "android/utils/debug.h"
#include "emugl/common/crash_reporter.h"
#include "emugl/common/sync_device.h"

#include <sys/time.h>

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

static const uint32_t kTimelineInterval = 1;
static const uint64_t kDefaultTimeoutNsecs = 5ULL * 1000ULL * 1000ULL * 1000ULL;

SyncThread::SyncThread(EGLContext parentContext) :
    mDisplay(EGL_NO_DISPLAY),
    mContext(parentContext),
    mSyncContext(EGL_NO_CONTEXT) {

    if (parentContext == EGL_NO_CONTEXT) {
        emugl_crash_reporter(
                "ERROR: attempted to start a SyncThread "
                "with EGL_NO_CONTEXT as parent context!");
    }

    FrameBuffer *fb = FrameBuffer::getFB();
    mDisplay = fb->getDisplay();

    this->start();

    createSyncContext();
}

void SyncThread::triggerWait(FenceSyncInfo* fenceSyncInfo,
                             FenceSync* fenceSync,
                             uint64_t timeline) {
    DPRINT("fenceSyncInfo=0x%llx timeline=0x%lx ...",
            fenceSyncInfo, timeline);
    SyncThreadCmd to_send;
    to_send.opCode = SYNC_THREAD_WAIT;
    to_send.fenceSyncInfo = fenceSyncInfo;
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

void SyncThread::createSyncContext() {
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
           cmd.opCode, cmd.fenceSyncInfo);
    cmd.needReply = false;
    mInput.send(cmd);
}

void SyncThread::doSyncContextCreation() {
    DPRINT("enter");
    FrameBuffer::getFB()->lockFramebuffer();

    EGLint err = s_egl.eglGetError();
    if (err != EGL_SUCCESS) {
        DPRINT("has existing EGL error 0x%x", err);
    }

    static const EGLint kTrivialAttribList[] = {
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_DEPTH_SIZE, 24,
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };

    EGLConfig trivial_config;
    EGLint numConfigs;
    EGLBoolean config_ret =
        s_egl.eglChooseConfig(mDisplay, kTrivialAttribList,
                &trivial_config, 1, &numConfigs);

    if (config_ret == EGL_FALSE) {
        err = s_egl.eglGetError();
        DPRINT("error choosing config! egl err=0x%x", err);
    }

    static const EGLint kContextAttribList[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    mSyncContext =
        s_egl.eglCreateContext(mDisplay, trivial_config,
                mContext, kContextAttribList);

    if (mSyncContext == EGL_NO_CONTEXT) {
        err = s_egl.eglGetError();
        DPRINT("error creating context! egl err=0x%x", err);
    }

    static const EGLint pbuf_attribs[] = {
        EGL_WIDTH, 1, EGL_HEIGHT, 1,
        EGL_NONE
    };

    EGLSurface surf =
        s_egl.eglCreatePbufferSurface(mDisplay, trivial_config, pbuf_attribs);

    if (surf == EGL_NO_SURFACE) {
        err = s_egl.eglGetError();
        DPRINT("error creating surface! egl err=0x%x", err);
    }

    EGLBoolean makecurrent_ret =
        s_egl.eglMakeCurrent(mDisplay, surf, surf, mSyncContext);

    if (makecurrent_ret == EGL_FALSE) {
        err = s_egl.eglGetError();
        DPRINT("error setting context! egl err=0x%x", __FUNCTION__, err);
    }

    FrameBuffer::getFB()->unlockFramebuffer();
    DPRINT("exit");
}

void SyncThread::doSyncWait(SyncThreadCmd* cmd) {
    DPRINT("enter");

    EGLint wait_result = 0x0;

    if (cmd->fenceSync) {
        DPRINT("wait on sync obj: 0x%llx", cmd->fenceSync->mGLSync);
        wait_result = s_egl.eglClientWaitSyncKHR
            (mDisplay,
             (EGLSyncKHR)cmd->fenceSync->mGLSync,
             EGL_SYNC_FLUSH_COMMANDS_BIT_KHR,
             kDefaultTimeoutNsecs);
    } else {
        DPRINT("WARNING: syncthread waiting on already signaled!\n");
    }

    DPRINT("done waiting, with wait result=0x%x. "
           "increment timeline (and signal fence)",
           wait_result);

    if (wait_result != EGL_CONDITION_SATISFIED_KHR) {
        DPRINT("error: eglClientWaitSync abnormal exit 0x%x",
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
    
    emugl_sync_timeline_inc(cmd->timeline, kTimelineInterval);
    // Also set this FenceSync object to signaled.
    if (cmd->fenceSync) {
        cmd->fenceSyncInfo->setSignaled(cmd->fenceSync->getHandle());
    }

    DPRINT("done timeline increment");

    DPRINT("exit");
}

void SyncThread::doExit() {
    FrameBuffer::getFB()->lockFramebuffer();

    s_egl.eglMakeCurrent(mDisplay,
                         EGL_NO_SURFACE,
                         EGL_NO_SURFACE,
                         EGL_NO_CONTEXT);
    s_egl.eglDestroyContext(mDisplay, mSyncContext);
    mSyncContext = EGL_NO_CONTEXT;

    FrameBuffer::getFB()->unlockFramebuffer();
}

int SyncThread::doSyncThreadCmd(SyncThreadCmd* cmd) {
    int result = 0;
    switch (cmd->opCode) {
    case SYNC_THREAD_INIT:
        DPRINT("exec SYNC_THREAD_INIT");
        doSyncContextCreation();
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

// Interface for libOpenglRender TLS

/* static */
SyncThread* SyncThread::getSyncThread() {
    RenderThreadInfo* tInfo = RenderThreadInfo::get();

    if (!tInfo->syncThread.get()) {
        DPRINT("starting a sync thread for render thread info=%p", tInfo);
        tInfo->syncThread.reset(
                new SyncThread(s_egl.eglGetCurrentContext()));
    }

    return tInfo->syncThread.get();
}

/* static */
void SyncThread::destroySyncThread() {
    RenderThreadInfo* tInfo = RenderThreadInfo::get();

    DPRINT("exiting a sync thread for render thread info=%p.", tInfo);

    if (!tInfo->syncThread) return;

    tInfo->syncThread->cleanup();
    intptr_t exitStatus;
    tInfo->syncThread->wait(&exitStatus);
    tInfo->syncThread.reset(nullptr);
}
