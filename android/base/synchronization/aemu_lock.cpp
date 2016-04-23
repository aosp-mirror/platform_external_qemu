#include "android/base/synchronization/aemu_lock.h"

#include "android/base/synchronization/ConditionVariable.h"
#include "android/base/synchronization/Lock.h"

#include <stdio.h>

using android::base::Lock;
using android::base::ConditionVariable;

int aemu_mutex_init(void** lock_addr) {
    if (*lock_addr) {
        fprintf(stderr, "ERROR: trying to initialize non-null lock\n");
        return -1;
    }

    Lock* l = new Lock();
    *lock_addr = (void*)l;
    fprintf(stderr, "initialized mutex\n");
    return 0;
}

int aemu_mutex_lock(void* lock) {
    Lock* l = (Lock*)lock;
    l->lock();
    return 0;
}

int aemu_mutex_unlock(void* lock) {
    Lock* l = (Lock*)lock;
    l->unlock();
    return 0;
}

int aemu_cond_init(void** cv_addr) {
    if (*cv_addr) {
        fprintf(stderr, "ERROR: trying to initialize non-null cv\n");
        return -1;
    }

    ConditionVariable* cv = new ConditionVariable();
    *cv_addr = (void*)cv;
    fprintf(stderr, "initialized cv\n");
    return 0;
}

int aemu_cond_wait(void* _cv, void* _lock) {
    ConditionVariable* cv = (ConditionVariable*)_cv;
    Lock* lock = (Lock*)_lock;
    cv->wait(lock);
    return 0;
}

int aemu_cond_signal(void* _cv) {
    ConditionVariable* cv = (ConditionVariable*)_cv;
    cv->signal();
    return 0;
}

