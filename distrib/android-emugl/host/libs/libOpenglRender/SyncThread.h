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

#include "FenceSyncInfo.h"

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "android/base/synchronization/Lock.h"
#include "android/base/synchronization/MessageChannel.h"

#include "emugl/common/thread.h"

// Provides:
// - thread-local timeline info (SyncTimelineInfo)
// - Running sync operations in separate threads (SyncThread)

struct SyncTimelineInfo {
    uint64_t timeline_handle = 0;
    uint32_t current_time = 0;
    android::base::Lock lock;
};

enum SyncThreadOpCode {
    SYNC_THREAD_INIT = 0,
    SYNC_THREAD_WAIT = 1,
    SYNC_THREAD_EXIT = 2
};

struct SyncThreadCmd {
    SyncThreadOpCode opCode = SYNC_THREAD_INIT;
    FenceSync* fenceSync = nullptr;
    uint64_t timeline = 0;
    bool needReply = false;
};

class SyncThread : public emugl::Thread {
public:
    SyncThread(EGLContext parentContext);
    void triggerWait(FenceSync* fenceSync, uint64_t timeline);
    void cleanup();

    static SyncThread* getSyncThread();
    static void destroySyncThread();

private:
    void createSyncContext();

    // Thread function executing all sync commands.
    virtual intptr_t main() override final;

    // Message channel: input: info about a particular sync operation
    static const size_t kSyncThreadChannelCapacity = 256;
    android::base::MessageChannel<SyncThreadCmd, kSyncThreadChannelCapacity> mInput;
    // holds result of cmds
    // (This will either be 0 or
    // the result of executing glClientWaitSync)
    android::base::MessageChannel<GLint, kSyncThreadChannelCapacity> mOutput;
    GLint sendAndWaitForResult(SyncThreadCmd& cmd);
    void sendAsync(SyncThreadCmd& cmd);

    // Implementation of the actual commands
    GLint doSyncThreadCmd(SyncThreadCmd* cmd);

    void doSyncContextCreation();
    void doSyncWait(SyncThreadCmd* cmd);
    void doExit();

    uint64_t timelineHandle();
    uint32_t currTimelinePt() const;
    uint32_t nextTimelinePt() const;
    void incTimeline();

    // EGL objects specific to the sync thread.
    EGLDisplay mDisplay;
    EGLContext mParentContext;
    EGLContext mSyncContext;

    // Tracking sync device parameters
    SyncTimelineInfo mTimelineInfo;
};

