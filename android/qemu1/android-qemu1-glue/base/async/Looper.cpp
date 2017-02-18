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

#include "android-qemu1-glue/base/async/Looper.h"

#include "android/base/Log.h"
#include "android/base/sockets/SocketUtils.h"
#include "android-qemu1-glue/base/files/QemuFileStream.h"
#include "android/utils/stream.h"

extern "C" {
#include "qemu-common.h"
#include "qemu/timer.h"
#include "sysemu/char.h"
}  // extern "C"

#include <list>
#include <unordered_map>

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
    QemuLooper() {
        mQemuBh = qemu_bh_new(handleBottomHalf, this);
    }

    virtual ~QemuLooper() {
        qemu_bh_delete(mQemuBh);
        DCHECK(mPendingFdWatches.empty());
    }

    static QEMUClockType toQemuClockType(ClockType clock) {
        static_assert((int) QEMU_CLOCK_HOST == (int) BaseLooper::ClockType::kHost &&
                      (int) QEMU_CLOCK_VIRTUAL == (int) BaseLooper::ClockType::kVirtual &&
                      (int) QEMU_CLOCK_REALTIME == (int) BaseLooper::ClockType::kRealtime,
                      "Values in the Looper::ClockType enumeration are out of sync with "
                              "QEMUClockType");

        return static_cast<QEMUClockType>(clock);
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
                void* opaque) : BaseFdWatch(looper, fd, callback, opaque) {}

        virtual ~FdWatch() {
            clearPending();
            qemu_set_fd_handler(mFd, NULL, NULL, NULL);
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

        unsigned mWantedEvents = 0;
        unsigned mPendingEvents = 0;
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
              void* opaque, ClockType clock) :
                    BaseTimer(looper, callback, opaque, clock),
                    mTimer(NULL) {
            mTimer = ::timer_new(QemuLooper::toQemuClockType(mClockType),
                                 SCALE_MS,
                                 qemuTimerCallbackAdapter,
                                 this);
        }

        ~Timer() {
            ::timer_free(mTimer);
        }

        virtual void startRelative(Duration timeout_ms) {
            if (timeout_ms == kDurationInfinite) {
                timer_del(mTimer);
            } else {
                timeout_ms += qemu_clock_get_ms(
                        QemuLooper::toQemuClockType(mClockType));
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

        void save(android::base::Stream* stream) const {
            timer_put(
                reinterpret_cast<android::qemu::QemuFileStream*>(stream)->file(),
                mTimer);
        }

        void load(android::base::Stream* stream) {
            timer_get(
                reinterpret_cast<android::qemu::QemuFileStream*>(stream)->file(),
                mTimer);
        }

    private:
        static void qemuTimerCallbackAdapter(void* opaque) {
            Timer* timer = static_cast<Timer*>(opaque);
            timer->mCallback(timer->mOpaque, timer);
        }

        QEMUTimer* mTimer;
    };

    virtual BaseTimer* createTimer(BaseTimer::Callback callback,
                                   void* opaque, ClockType clock) {
        return new QemuLooper::Timer(this, callback, opaque, clock);
    }

    //
    //  L O O P E R
    //

    virtual Duration nowMs(ClockType clockType) {
        return qemu_clock_get_ms(toQemuClockType(clockType));
    }

    virtual DurationNs nowNs(ClockType clockType) {
        return qemu_clock_get_ns(toQemuClockType(clockType));
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
    typedef std::list<FdWatch*> FdWatchList;
    typedef std::unordered_map<FdWatch*, FdWatchList::iterator> FdWatchSet;

    static inline QemuLooper* asQemuLooper(BaseLooper* looper) {
        return reinterpret_cast<QemuLooper*>(looper);
    }

    void addPendingFdWatch(FdWatch* watch) {
        DCHECK(watch);
        DCHECK(!watch->isPending());

        if (mPendingFdWatches.empty()) {
            // Ensure the bottom-half is triggered to act on pending
            // watches as soon as possible.
            qemu_bh_schedule(mQemuBh);
        }

        mPendingFdWatches.push_back(watch);
        mFdWatchIterMap[watch] = std::prev(mPendingFdWatches.end());
    }

    void delPendingFdWatch(FdWatch* watch) {
        DCHECK(watch);
        DCHECK(watch->isPending());
        const auto it = mFdWatchIterMap.find(watch);
        DCHECK(it != mFdWatchIterMap.end());
        mPendingFdWatches.erase(it->second);
        mFdWatchIterMap.erase(it);
    }

    // Called by QEMU as soon as the main loop has finished processed
    // I/O events. Used to look at pending watches and fire them.
    static void handleBottomHalf(void* opaque) {
        QemuLooper* looper = reinterpret_cast<QemuLooper*>(opaque);
        while (!looper->mPendingFdWatches.empty()) {
            FdWatch* watch = looper->mPendingFdWatches.front();
            looper->delPendingFdWatch(watch);
            watch->fire();
        }
    }

    QEMUBH* mQemuBh = nullptr;
    FdWatchSet mFdWatchIterMap;
    FdWatchList mPendingFdWatches;
};

}  // namespace

BaseLooper* createLooper() {
    return new QemuLooper();
}

}  // namespace qemu
}  // namespace android

