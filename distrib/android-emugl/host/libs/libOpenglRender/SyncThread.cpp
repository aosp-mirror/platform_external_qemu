
#include "SyncThread.h"

#include "android/utils/debug.h"

#define SYNC_THREAD_DEBUG 1

#if SYNC_THREAD_DEBUG
#define DPRINT(...) do { \
    if (!VERBOSE_CHECK(syncthreads)) VERBOSE_ENABLE(syncthreads); \
    VERBOSE_TID_FUNCTION_DPRINT(syncthreads, __VA_ARGS__); \
} while(0)
#else
#define DPRINT(...)
#endif

SyncThread::SyncThread() {
    DPRINT("ctor");
    this->start();
}

intptr_t SyncThread::main() {
    DPRINT("in sync thread");

    while (true) {
        SyncThreadCmd cmd = {};
        mInput.receive(&cmd);
        // intptr_t result = doSyncThreadCmd(&cmd);
        // mOutput.send
    }

    return 0;
}
SyncThread* createSyncThread() {
    return new SyncThread();
}

