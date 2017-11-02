#ifndef QEMU_THREAD_WIN32_H
#define QEMU_THREAD_WIN32_H

#include <windows.h>

#ifdef CONFIG_WIN32_VISTA_SYNCHRONIZATION
struct QemuMutex {
    SRWLOCK lock;
};

struct QemuCond {
    CONDITION_VARIABLE cv;
};
#else
struct QemuMutex {
    SRWLOCK lock;
};

struct QemuCond {
    LONG waiters, target;
    HANDLE sema;
    HANDLE continue_event;
};
#endif

typedef struct QemuRecMutex QemuRecMutex;
struct QemuRecMutex {
    CRITICAL_SECTION lock;
};

void qemu_rec_mutex_destroy(QemuRecMutex *mutex);
void qemu_rec_mutex_lock(QemuRecMutex *mutex);
int qemu_rec_mutex_trylock(QemuRecMutex *mutex);
void qemu_rec_mutex_unlock(QemuRecMutex *mutex);

<<<<<<< HEAD   (1e9dc2 Merge "Bump snapshot version number." into emu-master-dev)
=======
struct QemuCond {
    CONDITION_VARIABLE var;
};
>>>>>>> BRANCH (359c41 Update version for v2.9.0 release)

struct QemuSemaphore {
    HANDLE sema;
};

struct QemuEvent {
    int value;
    HANDLE event;
};

typedef struct QemuThreadData QemuThreadData;
struct QemuThread {
    QemuThreadData *data;
    unsigned tid;
};

/* Only valid for joinable threads.  */
HANDLE qemu_thread_get_handle(QemuThread *thread);

#endif
