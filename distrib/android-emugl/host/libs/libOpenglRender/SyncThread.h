#pragma once

#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include "android/base/synchronization/MessageChannel.h"

#include "emugl/common/thread.h"

enum SyncThreadOpCode {
    SYNC_THREAD_INIT = 0,
    SYNC_THREAD_WAIT = 1,
    SYNC_THREAD_EXIT = 2
};

struct SyncThreadCmd {
    SyncThreadOpCode opCode;
    uint32_t timelineHandle;
    int fenceFd;
    uint32_t timeArg;
    void* glsync;
    bool needReply;
};

class SyncThread : public emugl::Thread {
public:
    SyncThread();
    void createSyncContext();
    void triggerWait(void* glsync,
                     uint32_t timeline);
    void triggerExit();
    virtual intptr_t main();
private:
    // executes a particular sync command.
    android::base::MessageChannel<SyncThreadCmd, 4U> mInput;
    // holds result of cmds
    // (This will either be 0 or
    // the result of executing glClientWaitSync)
    android::base::MessageChannel<GLint, 4U> mOutput;
    GLint sendAndWaitForResult(SyncThreadCmd& cmd);
    void sendAsync(SyncThreadCmd& cmd);

    // Implementation of the actual commands
    void doSyncContextCreation();
    GLint doSyncWait(void* glsync, uint32_t timeline_handle);
    GLint doSyncThreadCmd(SyncThreadCmd* cmd);

    // EGL objects specific to the sync thread.
    EGLDisplay mDisplay;
    EGLContext mParentContext;
    EGLContext mSyncContext;

};

SyncThread* createSyncThread();
