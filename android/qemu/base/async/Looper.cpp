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

#include "android/qemu/base/async/Looper.h"

#include "android/base/Log.h"
#include "android/base/containers/TailQueueList.h"
#include "android/base/containers/ScopedPointerSet.h"
#include "android/base/sockets/SocketUtils.h"

extern "C" {
#include "qemu-common.h"
#include "qemu/timer.h"
#include "sysemu/char.h"
}  // extern "C"

namespace android {
namespace qemu {

namespace {

extern "C" void qemu_system_shutdown_request(void);

typedef ::android::base::Looper BaseLooper;
typedef ::android::base::Looper::Timer BaseTimer;
typedef ::android::base::Looper::FdWatch BaseFdWatch;

// An implementation of android::base::Looper on top of the QEMU main
// event loop. There are few important things here:
//
//  1/ There is a single global QEMU event loop, so all instances returned
//     by createLooper() will really use the same state!
//
//     In other words, don't call it more than once!
//
//  2/ It is not possible to call the runWithDeadlineMs() method, since
//     the event loop is started in the application's main thread by
//     the executable.
//
// The implementation uses a bottom-half handler to process pending
// FdWatch instances, see the comment in the declaration of FdWatch
// below to understand why.
//
class QemuLooper : public BaseLooper {
public:
    QemuLooper() :
            Looper(),
            mQemuBh(NULL),
            mFdWatches(),
            mPendingFdWatches(),
            mTimers() {
        mQemuBh = qemu_bh_new(handleBottomHalf, this);
    }

    virtual ~QemuLooper() {
        qemu_bh_delete(mQemuBh);
    }

    //
    // F D   W A T C H E S
    //

    // Implementation is slightly more complicated because QEMU uses
    // distinct callback functions for read/write events, so use a pending
    // list to collect all FdWatch instances that have received any of such
    // call, then later process it.

    class FdWatch : public BaseFdWatch {
    public:
        FdWatch(QemuLooper* looper,
                int fd,
                BaseFdWatch::Callback callback,
                void* opaque) :
                        BaseFdWatch(looper, fd, callback, opaque),
                        mWantedEvents(0U),
                        mPendingEvents(0U),
                        mLink() {
            looper->addFdWatch(this);
        }

        virtual ~FdWatch() {
            clearPending();
            qemu_set_fd_handler(mFd, NULL, NULL, NULL);
            asQemuLooper(mLooper)->delFdWatch(this);
        }

        virtual void addEvents(unsigned events) {
            events &= kEventMask;
            updateEvents(mWantedEvents | events);
        }

        virtual void removeEvents(unsigned events) {
            events &= kEventMask;
            updateEvents(mWantedEvents & ~events);
        }

        virtual unsigned poll() const {
            return mPendingEvents;
        }

        bool isPending() const {
            return mPendingEvents != 0U;
        }

        void fire() {
            DCHECK(mPendingEvents);
            unsigned events = mPendingEvents;
            mPendingEvents = 0U;
            mCallback(mOpaque, mFd, events);
        }

        TAIL_QUEUE_LIST_TRAITS(Traits, FdWatch, mLink);

    private:
        void updateEvents(unsigned events) {
            IOHandler* cbRead = (events & kEventRead) ? handleRead : NULL;
            IOHandler* cbWrite = (events & kEventWrite) ? handleWrite : NULL;
            qemu_set_fd_handler(mFd, cbRead, cbWrite, this);
            mWantedEvents = events;
        }

        void setPending(unsigned event) {
            if (!mPendingEvents) {
                asQemuLooper(mLooper)->addPendingFdWatch(this);
            }
            mPendingEvents |= event;
        }

        void clearPending() {
            if (mPendingEvents) {
                asQemuLooper(mLooper)->delPendingFdWatch(this);
                mPendingEvents = 0;
            }
        }

        // Called by QEMU on a read i/o event. |opaque| is a FdWatch handle.
        static void handleRead(void* opaque) {
            FdWatch* watch = static_cast<FdWatch*>(opaque);
            watch->setPending(kEventRead);
        }

        // Called by QEMU on a write i/o event. |opaque| is a FdWatch handle.
        static void handleWrite(void* opaque) {
            FdWatch* watch = static_cast<FdWatch*>(opaque);
            watch->setPending(kEventWrite);
        }

        unsigned mWantedEvents;
        unsigned mPendingEvents;
        ::android::base::TailQueueLink<FdWatch> mLink;
    };

    virtual BaseFdWatch* createFdWatch(int fd,
                                       BaseFdWatch::Callback callback,
                                       void* opaque) {
        ::android::base::socketSetNonBlocking(fd);
        return new FdWatch(this, fd, callback, opaque);
    }

    //
    // T I M E R S
    //
    class Timer : public BaseTimer {
    public:
        Timer(QemuLooper* looper,
              BaseTimer::Callback callback,
              void* opaque) :
                    BaseTimer(looper, callback, opaque),
                    mTimer(NULL) {
            mTimer = ::timer_new(QEMU_CLOCK_HOST,
                                 SCALE_MS,
                                 callback,
                                 opaque);
        }

        ~Timer() {
            ::timer_free(mTimer);
        }

        virtual void startRelative(Duration timeout_ms) {
            if (timeout_ms == kDurationInfinite) {
                timer_del(mTimer);
            } else {
                timeout_ms += qemu_clock_get_ms(QEMU_CLOCK_HOST);
                timer_mod(mTimer, timeout_ms);
            }
        }

        virtual void startAbsolute(Duration deadline_ms) {
            if (deadline_ms == kDurationInfinite) {
                timer_del(mTimer);
            } else {
                timer_mod(mTimer, deadline_ms);
            }
        }

        virtual void stop() {
            ::timer_del(mTimer);
        }

        virtual bool isActive() const {
            return timer_pending(mTimer);
        }

    private:
        QEMUTimer* mTimer;
    };

    virtual BaseTimer* createTimer(BaseTimer::Callback callback,
                                   void* opaque) {
        return new QemuLooper::Timer(this, callback, opaque);
    }

    //
    //  L O O P E R
    //

    virtual Duration nowMs() {
        return qemu_clock_get_ms(QEMU_CLOCK_HOST);
    }

    virtual int runWithDeadlineMs(Duration deadline_ms) {
        CHECK(false) << "User cannot call looper_run on a QEMU event loop";
        errno = ENOSYS;
        return -1;
    }

    virtual void forceQuit() {
        qemu_system_shutdown_request();
    }

private:

    typedef ::android::base::TailQueueList<QemuLooper::FdWatch> FdWatchList;
    typedef ::android::base::ScopedPointerSet<FdWatch> FdWatchSet;
    typedef ::android::base::ScopedPointerSet<Timer> TimerSet;

    static inline QemuLooper* asQemuLooper(BaseLooper* looper) {
        return reinterpret_cast<QemuLooper*>(looper);
    }

    void addFdWatch(FdWatch* watch) {
        mFdWatches.add(watch);
    }

    void delFdWatch(FdWatch* watch) {
        mFdWatches.pick(watch);
    }

    void addTimer(Timer* timer) {
        mTimers.add(timer);
    }

    void delTimer(Timer* timer) {
        mTimers.remove(timer);
    }

    void addPendingFdWatch(FdWatch* watch) {
        DCHECK(watch);
        DCHECK(!watch->isPending());

        if (mPendingFdWatches.empty()) {
            // Ensure the bottom-half is triggered to act on pending
            // watches as soon as possible.
            qemu_bh_schedule(mQemuBh);
        }

        mPendingFdWatches.insertTail(watch);
    }

    void delPendingFdWatch(FdWatch* watch) {
        DCHECK(watch);
        DCHECK(watch->isPending());
        mPendingFdWatches.remove(watch);
    }

    // Called by QEMU as soon as the main loop has finished processed
    // I/O events. Used to look at pending watches and fire them.
    static void handleBottomHalf(void* opaque) {
        QemuLooper* looper = reinterpret_cast<QemuLooper*>(opaque);
        for (;;) {
            FdWatch* watch = looper->mPendingFdWatches.front();
            if (!watch) {
                break;
            }
            looper->delPendingFdWatch(watch);
            watch->fire();
        }
    }

    QEMUBH* mQemuBh;

    FdWatchSet mFdWatches;
    FdWatchList mPendingFdWatches;
    TimerSet mTimers;
};

}  // namespace

BaseLooper* createLooper() {
    return new QemuLooper();
}

}  // namespace qemu
}  // namespace android

