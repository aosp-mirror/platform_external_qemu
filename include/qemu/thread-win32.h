#ifndef QEMU_THREAD_WIN32_H
#define QEMU_THREAD_WIN32_H

#include <windows.h>

struct QemuMutex {
    SRWLOCK lock;
    bool initialized;
};

struct QemuCond {
    CONDITION_VARIABLE cv;
};

typedef struct QemuRecMutex QemuRecMutex;
struct QemuRecMutex {
    CRITICAL_SECTION lock;
    bool initialized;
};

void qemu_rec_mutex_destroy(QemuRecMutex *mutex);
void qemu_rec_mutex_lock(QemuRecMutex *mutex);
int qemu_rec_mutex_trylock(QemuRecMutex *mutex);
void qemu_rec_mutex_unlock(QemuRecMutex *mutex);

<<<<<<< HEAD   (b314f6 Merge "Allow emulator with WindowsHypervisorPlatform to run )
=======
struct QemuCond {
    CONDITION_VARIABLE var;
    bool initialized;
};

>>>>>>> BRANCH (ba8716 Update version for 2.10.2 release)
struct QemuSemaphore {
    HANDLE sema;
    bool initialized;
};

struct QemuEvent {
    int value;
    HANDLE event;
    bool initialized;
};

typedef struct QemuThreadData QemuThreadData;
struct QemuThread {
    QemuThreadData *data;
    unsigned tid;
};

/* Only valid for joinable threads.  */
HANDLE qemu_thread_get_handle(QemuThread *thread);

#endif
