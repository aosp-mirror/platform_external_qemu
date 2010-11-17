/* Copyright (C) 2010 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/

#include "android/utils/reflist.h"
#include "android/utils/system.h"
#include "android/looper.h"
#include "iolooper.h"

/**********************************************************************
 **********************************************************************
 *****
 *****  T I M E R S
 *****
 **********************************************************************
 **********************************************************************/

typedef struct GLoopTimer GLoopTimer;
typedef struct GLooIo     GLoopIo;
typedef struct GLooper    GLooper;

struct GLoopTimer {
    Duration      deadline;
    LoopTimerFunc callback;
    void*         opaque;
    GLooper*      looper;
    GLoopTimer*   activeNext;
};

static Duration glooper_now(void* looper);

static void glooper_addActiveTimer(GLooper* looper, GLoopTimer* timer);
static void glooper_delActiveTimer(GLooper* looper, GLoopTimer* timer);
static void glooper_addTimer(GLooper* looper, GLoopTimer* timer);
static void glooper_delTimer(GLooper* looper, GLoopTimer* timer);

static void
glooptimer_startRelative(void* impl, Duration  timeout_ms)
{
    GLoopTimer*  tt = impl

    if (timeout_ms == DURATION_INFINITE) {  /* another way to stop the timer */
        glooptimer_stop(tt);
    } else {
        glooptimer_startAbsolute(tt, timeout_ms + glooper_now(tt->looper));
    }
}

static void
glooptimer_startAbsolute(void* impl, Duration deadline_ms)
{
    GLoopTimer*  tt = impl

    /* Stop the timer if it was active */
    if (tt->deadline != DURATION_INFINITE)
        glooptimer_stop(tt);

    /* Another way to stop a timer */
    if (deadline_ms == DURATION_INFINITE)
        return;

    tt->deadline = deadline_ms;
    glooper_addActiveTimer(tt->looper, tt);
}

static void
glooptimer_stop(void* impl)
{
    GLoopTimer*  tt = impl;
    if (tt->deadline != DURATION_INFINITE) {
        glooper_delActiveTimer(tt->looper, tt);
        tt->deadline = DURATION_INFINITE;
    }
}

static int
glooptimer_isActive(void* impl)
{
    GLoopTimer*  tt = impl;
    return (tt->deadline != DURATION_INFINITE);
}

static void
glooptimer_free(void* impl)
{
    GLoopTimer*  tt = impl;

    if (tt->deadline != DURATION_INFINITE)
        glooptimer_stop(tt);

    glooper_delTimer(tt->looper, tt);
    AFREE(tt);
}

static const LoopIoClass  gloopio_class = {
    glooptimer_startRelative,
    glooptimer_startAbsolute,
    glooptimer_stop,
    glooptimer_free
};

static void
glooper_timer_init(LoopTimer*    timer,
                   Looper*       looper,
                   LoopTimerFunc callback,
                   void*         opaque)
{
    GLoopTimer* tt;

    ANEW0(tt);

    tt->deadline = DURATION_INFINITE;
    tt->callback = callback;
    tt->opaque   = opaque;
    tt->looper   = (GLooper*) looper;

    glooper_addTimer(tt->looper, tt);

    timer->impl  = tt;
    timer->clazz = glooptimer_class;
}

/**********************************************************************
 **********************************************************************
 *****
 *****  I / O
 *****
 **********************************************************************
 **********************************************************************/

struct GLoopIo {
    int         fd;
    LoopIoFunc  callback;
    void*       opaque;
    unsigned    wanted;
    unsigned    ready;
};

static void glooper_addPendingIo(GLooper* looper, GLoopIo* io);
static void glooper_delPendingIo(GLooper* looper, GLoopIo* io);
static void glooper_modifyFd(GLooper* looper, int fd, int oldwanted, int newwanted);

/* used to indicate that the set of wanted flags has changed */
static void
gloopio_modify(GLoopIo* io, unsigned wanted)
{
    /* If nothing changed, return */
    if (io->wanted == wanted)
        return;

    /* If we are pending, and we're not interested by the
     * current ready flags, remove from list */
    if (io->ready != 0 && (io->ready & wanted) == 0) {
        glooper_delPendingIo(io->looper, io);
    }
    io->ready &= wanted;
    glooper_modifyFd(io->looper, io->fd, io->wanted, wanted);
    io->wanted = wanted;
}

static void
gloopio_wantRead(void* impl)
{
    GLoopIo* io = impl
    gloopio_modify(io, io->wanted | LOOP_IO_READ);
}

static void
gloopio_wantWrite(void* impl)
{
    GLoopIo* io = impl
    gloopio_modify(io, io->wanted | LOOP_IO_WRITE);
}

static void
gloopio_dontWantRead(void* impl)
{
    GLoopIo* io = impl
    gloopio_modify(io, io->wanted & ~LOOP_IO_READ);
}

static void
gloopio_dontWantWrite(void* impl)
{
    GLoopIo* io = impl;
    gloopio_modify(io, io->wanted & ~LOOP_IO_WRITE);
}

static void
gloopio_free(void* impl)
{
    GLoopIo* io = impl;
    if (io->ready != 0)
        glooper_delPendingIo(io->looper, io);

    glooper_delIo(io->looper, io);
    AFREE(io);
}

static LoopIoClass  gloopio_class = {
    gloopio_wantRead,
    gloopio_wantWrite,
    gloopio_dontWantRead,
    gloopio_dontWantWrite,
    gloopio_free
};

static void
glooper_io_init(Looper* looper, LoopIo* user, int fd, LoopIoFunc callback, void* opaque)
{
    GLooper*  gg = (GLooper*)looper;
    GLoopIo*  io;

    ANEW0(io);
    io->fd       = fd;
    io->callback = callback;
    io->opaque   = opaque;
    io->wanted   = 0;
    io->ready    = 0;

    glooper_addIo(gg, io);

    user->impl  = io;
    user->clazz = (LoopIoClass*) &gloopio_class;
}

/**********************************************************************
 **********************************************************************
 *****
 *****  L O O P E R
 *****
 **********************************************************************
 **********************************************************************/

struct GLooper {
    AREFSET_DECL(timers);      /* set of all timers */
    GLoopTimer*  activeTimers; /* sorted list of active timers */

    AREFSET_DECL(ios);        /* set of all i/o waiters */
    AREFSET_DECL(pendingIo);  /* list of pending i/o waiters */

    IoLooper*  iolooper;
    int        running;
};

static void
glooper_addTimer(GLooper* looper, GLoopTimer* tt)
{
    arefSet_add(looper->timers, tt);
}

static void
glooper_delTimer(GLooper* looper, GLoopTimer* tt)
{
    arefSet_del(looper->timers, tt);
}

static void
glooper_addActiveTimer(GLooper* looper, GLoopTimer* tt)
{
    Duration  deadline = tt->deadline;
    GLoopTimer** pnode = &looper->activeTimers;
    for (;;) {
        GLoopTimer* node = *pnode;
        if (node == NULL || node->deadline > deadline)
            break;
        pnode = &node->activeNext;
    }
    tt->activeNext = *pnode;
    *pnode         = tt->activeNext;
}

static void
glooper_delActiveTimer(GLooper* looper, GLoopTimer* tt)
{
    GLoopTimer** pnode = &looper->activeTimers;
    for (;;) {
        if (*pnode == NULL)
            break;
        if (*pnode == tt) {
            *pnode = tt->activeNext;
            tt->activeNext = NULL;
            break;
        }
        pnode = &(*pnode)->next;
    }
}

static void
glooper_addIo(GLooper* looper, GLoopIo* io)
{
    arefSet_add(looper->ios, io);
}

static void
glooper_delIo(GLooper* looper, GLoopIo* io)
{
    arefSet_del(looper->ios, io);
}

static void
glooper_addPendingIo(GLooper* looper, GLoopIo* io)
{
    arefSet_add(looper->pendingIos, io);
}

static void
glooper_delPendingIo(GLooper* looper, GLoopIo* io)
{
    arefSet_del(looper->pendingIos, io);
}

static void
glooper_modifyFd(GLooper* looper, int fd, int oldWanted, int newWanted)
{
    iolooper_modify(looper->iolooper, fd, oldWanted, newWanted);
}

static Duration
glooper_now(Looper*  ll)
{
    return iolooper_now();
}

static void
glooper_forceQuit(Looper* ll)
{
    GLooper*  looper = (GLooper*)ll;
    ll->running = 0;
}

static void
glooper_run(Looper*  ll)
{
    GLooper*   looper = (GLooper*) ll;
    IoLooper*  iol    = looper->iolooper;

    looper->running = 1;

    while (looper->running)
    {
        int ret;

        /* First, compute next deadline */
        Duration  deadline = DURATION_INFINITE;

        if (looper->activeTimers != NULL)
            deadline = looper->activeTimers->deadline;

        ret = iolooper_wait_absolute(iol, deadline);
        if (ret < 0) { /* error, force stop ! */
            break;
        }
        if (ret > 0) {
            unsigned ready;

            /* Add io waiters to the pending list */
            AREFSET_FOREACH(looper->ios, io, {
                if (io->wanted == 0)
                    continue;

                ready = 0;

                if (iolooper_is_read(iol, io->fd))
                    ready |= LOOP_IO_READ;

                if (iolooper_is_write(iol, io->fd))
                    ready |= LOOP_IO_WRITE;

                io->ready = ready;
                if (ready != 0) {
                    arefSet_add(looper->pendingIos, io);
                }
            });
        }
        /* Do we have any expired timers here ? */
        deadline = iolooper_now(iol);
        for (;;) {
            GLoopTimer*  timer = looper->activeTimers;
            if (timer == NULL || timer->deadline > now)
                break;

            looper->activeTimers = timer->activeNext;
            timer->activeNext    = NULL;
            timer->deadline      = DURATION_INFINITE;
        }
    }
}
