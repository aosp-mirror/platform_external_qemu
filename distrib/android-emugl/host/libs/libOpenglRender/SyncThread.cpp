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

// SyncThread///////////////////////////////////////////////////////////////////
// The purpose of SyncThread is to track sync device timelines and give out +
// signal FD's that correspond to the completion of host-side GL fence commands.
// The interface has the following elements:
// - constructor: start up the sync thread given a parent context
// - triggerWait: async wait on the given FenceSync object. Assuming the
// object is not signaled already, SyncThread will employ a host-side
// eglClientWaitSyncKHR to wait on the sync object. After wait is over or if
// the sync object is already signaled, SyncThread will increment the given
// timeline, which should signal the guest-side fence FD.
// - cleanup: for use with destructors and other cleanup functions.
// it destroys the sync context and exits the sync thread.

static const uint32_t kTimelineInterval = 1;
static const uint64_t kDefaultTimeoutNsecs = 5ULL * 1000ULL * 1000ULL * 1000ULL;

SyncThread::SyncThread(EGLContext parentContext) :
    mDisplay(EGL_NO_DISPLAY),
    mParentContext(parentContext),
    mSyncContext(EGL_NO_CONTEXT) {

    if (parentContext == EGL_NO_CONTEXT) {
        emugl_crash_reporter(
                "ERROR: attempted to start a SyncThread "
                "with EGL_NO_CONTEXT as parent context!");
    }

    FrameBuffer *fb = FrameBuffer::getFB();
    mDisplay = fb->getDisplay();

    mTimelineInfo.timeline_handle = 0;
    mTimelineInfo.current_time = 0;

    this->start();

    createSyncContext();
}

void SyncThread::triggerWait(FenceSync* fenceSync,
                             uint64_t timeline) {
    DPRINT("fenceSync=0x%llx timeline=0x%lx ...",
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

void SyncThread::createSyncContext() {
    DPRINT("enter");
    SyncThreadCmd to_send;
    to_send.opCode = SYNC_THREAD_INIT;
    sendAsync(to_send);
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
    DPRINT("send with opcode=%u fenceSync=0x%llx",
           cmd.opCode, cmd.fenceSync);
    cmd.needReply = false;
    mInput.send(cmd);
}

void SyncThread::doSyncContextCreation() {
    DPRINT("enter");
    EGLint err = EGL_SUCCESS;

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
    err = s_egl.eglGetError();
    if (config_ret == EGL_FALSE ||
            err != EGL_SUCCESS) {
        DPRINT("error choosing config! egl err=0x%x", err);
    }

    static const EGLint kContextAttribList[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };
    mSyncContext =
        s_egl.eglCreateContext(mDisplay, trivial_config,
                mParentContext, kContextAttribList);

    err = s_egl.eglGetError();
    if (mSyncContext == EGL_NO_CONTEXT ||
            err != EGL_SUCCESS) {
        DPRINT("error creating context! egl err=0x%x", err);
    }

    static const EGLint pbuf_attribs[] = {
        EGL_WIDTH, 1, EGL_HEIGHT, 1,
        EGL_NONE
    };

    EGLSurface surf =
        s_egl.eglCreatePbufferSurface(mDisplay, trivial_config, pbuf_attribs);

    err = s_egl.eglGetError();
    if (err != EGL_SUCCESS) {
        DPRINT("error creating surface! egl err=0x%x", err);
    }

    EGLBoolean makecurrent_ret =
        s_egl.eglMakeCurrent(mDisplay, surf, surf, mSyncContext);

    err = s_egl.eglGetError();
    if (makecurrent_ret == EGL_FALSE ||
            err != EGL_SUCCESS) {
        DPRINT("error setting context! egl err=0x%x", __FUNCTION__, err);
    }
    DPRINT("exit");
}

void SyncThread::doSyncWait(SyncThreadCmd* cmd) {
    DPRINT("enter");

    EGLint wait_result = 0x0;
    if (cmd->fenceSync) {
        DPRINT("wait on sync obj: 0x%llx", cmd->fenceSync->sync);
        wait_result = s_egl.eglClientWaitSyncKHR
            (mDisplay,
             (EGLSyncKHR)cmd->fenceSync->sync,
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
    emugl_sync_timeline_inc(cmd->timeline, kTimelineInterval);
    DPRINT("done timeline increment");

    DPRINT("exit");
}

void SyncThread::doExit() {
    s_egl.eglDestroyContext(mDisplay, mSyncContext);
    mSyncContext = EGL_NO_CONTEXT;
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

// All timeline operations are assumed externally synchronized.

uint64_t SyncThread::timelineHandle() {
    if (mTimelineInfo.timeline_handle) {
        return mTimelineInfo.timeline_handle;
    }

    mTimelineInfo.timeline_handle = emugl_sync_create_timeline();
    mTimelineInfo.current_time = 0;
    return mTimelineInfo.timeline_handle;
}

void SyncThread::incTimeline() {
    emugl_sync_timeline_inc(mTimelineInfo.timeline_handle,
                            kTimelineInterval);
    mTimelineInfo.current_time += kTimelineInterval;
}

uint32_t SyncThread::currTimelinePt() const {
    return mTimelineInfo.current_time;
}

uint32_t SyncThread::nextTimelinePt() const {
    return mTimelineInfo.current_time + kTimelineInterval;
}

// Interface for libOpenglRender TLS

SyncThread* SyncThread::getSyncThread() {
    RenderThreadInfo* tInfo = RenderThreadInfo::get();

    if (!tInfo->syncThread.get()) {
        DPRINT("starting a sync thread for render thread info=%p", tInfo);
        tInfo->syncThread.reset(
                new SyncThread(s_egl.eglGetCurrentContext()));
    }

    return tInfo->syncThread.get();
}

void SyncThread::destroySyncThread() {
    RenderThreadInfo* tInfo = RenderThreadInfo::get();

    DPRINT("exiting a sync thread for render thread info=%p.", tInfo);

    if (!tInfo->syncThread) return;

    tInfo->syncThread->cleanup();
    intptr_t exitStatus;
    tInfo->syncThread->wait(&exitStatus);

    tInfo->syncThread.reset(nullptr);
}
