#pragma once

#include <GLES2/gl2.h>

#include "android/base/synchronization/MessageChannel.h"

#include "emugl/common/thread.h"

enum SyncThreadOpCode {
    SYNC_THREAD_INIT = 0,
    SYNC_THREAD_TRIGGER_WAIT = 1
};

struct SyncThreadCmd {
    SyncThreadOpCode opCode;
    uint32_t timelineHandle;
    int fenceFd;
    uint32_t timeArg;
    void* glsync;
};

class SyncThread : public emugl::Thread {
public:
    SyncThread();
    virtual intptr_t main();
private:
    // executes a particular sync command.
    android::base::MessageChannel<SyncThreadCmd, 4U> mInput;
    // holds result of cmds
    // (This will either be 0 or
    // the result of executing glClientWaitSync)
    android::base::MessageChannel<GLint, 4U> mOutput;
};

SyncThread* createSyncThread();
