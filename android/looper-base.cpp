// Copyright 2014 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// Include this first to avoid compiler warning.
#include "android/base/Limits.h"

#include "android/looper-base.h"

#include "android/base/async/Looper.h"
#include "android/base/sockets/SocketUtils.h"

#include "android/qemu/base/async/Looper.h"

// An implementation of the C Looper type based on the C++
// android::base::Looper class.

namespace android {
namespace internal {

namespace {

typedef ::Duration CDuration;
typedef ::android::base::Looper::Duration BaseDuration;
typedef ::android::base::Looper::Timer BaseTimer;
typedef ::android::base::Looper::FdWatch BaseFdWatch;

// Forward declarations.
GLooper* asGLooper(CLooper* looper);
BaseLooper* asBaseLooper(CLooper* looper);

/**********************************************************************
 **********************************************************************
 *****
 *****  T I M E R S
 *****
 **********************************************************************
 **********************************************************************/

BaseTimer* asBaseTimer(void* impl) {
    return reinterpret_cast<BaseTimer*>(impl);
}

void glooptimer_stop(void* impl) {
    asBaseTimer(impl)->stop();
}

void glooptimer_startAbsolute(void* impl, Duration deadline_ms) {
    asBaseTimer(impl)->startAbsolute(deadline_ms);
}

void glooptimer_startRelative(void* impl, Duration  timeout_ms) {
    asBaseTimer(impl)->startRelative(timeout_ms);
}

int glooptimer_isActive(void* impl) {
    return asBaseTimer(impl)->isActive();
}

void glooptimer_free(void* impl) {
    delete asBaseTimer(impl);
}

const LoopTimerClass  glooptimer_class = {
    glooptimer_startRelative,
    glooptimer_startAbsolute,
    glooptimer_stop,
    glooptimer_isActive,
    glooptimer_free
};

void glooper_timer_init(CLooper*      looper,
                        LoopTimer*    timer,
                        LoopTimerFunc callback,
                        void*         opaque) {
    timer->impl  = static_cast<void*>(
            asBaseLooper(looper)->createTimer(callback, opaque));

    timer->clazz = (LoopTimerClass*) &glooptimer_class;
}

/**********************************************************************
 **********************************************************************
 *****
 *****  I / O
 *****
 **********************************************************************
 **********************************************************************/

BaseFdWatch* asBaseFdWatch(void* impl) {
    return reinterpret_cast<BaseFdWatch*>(impl);
}

void gloopio_wantRead(void* impl) {
    asBaseFdWatch(impl)->wantRead();
}

void gloopio_wantWrite(void* impl) {
    asBaseFdWatch(impl)->wantWrite();
}

void gloopio_dontWantRead(void* impl) {
    asBaseFdWatch(impl)->dontWantRead();
}

void gloopio_dontWantWrite(void* impl) {
    asBaseFdWatch(impl)->dontWantWrite();
}

static unsigned gloopio_poll(void* impl) {
    return asBaseFdWatch(impl)->poll();
};

void gloopio_free(void* impl) {
    delete asBaseFdWatch(impl);
}

LoopIoClass gloopio_class = {
    gloopio_wantRead,
    gloopio_wantWrite,
    gloopio_dontWantRead,
    gloopio_dontWantWrite,
    gloopio_poll,
    gloopio_free
};

void glooper_io_init(Looper* looper,
                     LoopIo* user,
                     int fd,
                     LoopIoFunc callback,
                     void* opaque) {
    android::base::socketSetNonBlocking(fd);

    user->impl  = asBaseLooper(looper)->createFdWatch(fd, callback, opaque);
    user->clazz = (LoopIoClass*) &gloopio_class;
}

/**********************************************************************
 **********************************************************************
 *****
 *****  L O O P E R
 *****
 **********************************************************************
 **********************************************************************/

GLooper* asGLooper(CLooper* ll) {
    return reinterpret_cast<GLooper*>(ll);
}

BaseLooper* asBaseLooper(CLooper* ll) {
    return asGLooper(ll)->baseLooper;
}

CDuration glooper_now(CLooper* ll) {
    return asBaseLooper(ll)->nowMs();
}

void glooper_forceQuit(CLooper* ll) {
    asBaseLooper(ll)->forceQuit();
}

int glooper_run(CLooper* ll, Duration loop_deadline_ms) {
    return asBaseLooper(ll)->runWithDeadlineMs(loop_deadline_ms);
}

void glooper_free(Looper* ll) {
    delete asGLooper(ll);
}

}  // namespace

GLooper::GLooper(BaseLooper* baseLooper_) :
        looper(),
        baseLooper(baseLooper_) {
    looper.now        = glooper_now;
    looper.timer_init = glooper_timer_init;
    looper.io_init    = glooper_io_init;
    looper.run        = glooper_run;
    looper.forceQuit  = glooper_forceQuit;
    looper.destroy    = glooper_free;
}

GLooper::~GLooper() {
    delete baseLooper;
}

BaseLooper* toBaseLooper(CLooper* looper) {
    return asBaseLooper(looper);
}

}  // namespace internal
}  // namespace android
