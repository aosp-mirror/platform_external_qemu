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

#include "android/base/async/Looper.h"

#include "android/base/containers/ScopedPointerSet.h"
#include "android/base/containers/TailQueueList.h"
#include "android/base/Log.h"
#include "android/base/memory/ScopedPtr.h"
#include "android/base/sockets/SocketErrors.h"
#include "android/base/sockets/SocketWaiter.h"

#include <sys/time.h>

#include <stdio.h>

namespace android {
namespace base {

namespace {

// Generic looper implementation based on select().
class GenLooper : public Looper {
public:
    GenLooper() :
            Looper(),
            mWaiter(SocketWaiter::create()),
            mFdWatches(),
            mPendingFdWatches(),
            mTimers(),
            mActiveTimers(),
            mPendingTimers(),
            mForcedExit(false) {}

    virtual ~GenLooper() {}

    virtual Duration nowMs() {
        struct timeval time_now;
        return gettimeofday(&time_now, NULL)
                ? -1
                : (Duration)time_now.tv_sec * 1000LL +
                        time_now.tv_usec / 1000;
    }

    virtual void forceQuit() {
        mForcedExit = true;
    }

    //
    //  F D   W A T C H E S
    //

    class FdWatch : public Looper::FdWatch {
    public:
        FdWatch(GenLooper* looper,
                int fd,
                Callback callback,
                void* opaque) :
                        Looper::FdWatch(looper, fd, callback, opaque),
                        mWantedEvents(0U),
                        mLastEvents(0U),
                        mPending(false),
                        mPendingLink() {
            looper->addFdWatch(this);
        }

        GenLooper* genLooper() const {
            return reinterpret_cast<GenLooper*>(mLooper);
        }

        virtual ~FdWatch() {
            dontWantWrite();
            dontWantRead();
            clearPending();
            genLooper()->delFdWatch(this);
        }

        virtual void addEvents(unsigned events) {
            events &= kEventMask;
            unsigned newEvents = mWantedEvents | events;
            if (newEvents != mWantedEvents) {
                mWantedEvents = newEvents;
                genLooper()->updateFdWatch(mFd, newEvents);
            }
        }

        virtual void removeEvents(unsigned events) {
            events &= kEventMask;
            unsigned newEvents = mWantedEvents & ~events;
            if (newEvents != mWantedEvents) {
                mWantedEvents = newEvents;
                genLooper()->updateFdWatch(mFd, newEvents);
            }
            // These events are no longer desired.
            mLastEvents &= ~events;
        }

        virtual unsigned poll() const {
            return mLastEvents;
        }

        // Return true iff this FdWatch is pending execution.
        bool isPending() const {
            return mPending;
        }

        // Add this FdWatch to a pending queue.
        void setPending(unsigned events) {
            DCHECK(!mPending);
            mPending = true;
            mLastEvents = events & kEventMask;
            genLooper()->addPendingFdWatch(this);
        }

        // Remove this FdWatch from a pending queue.
        void clearPending() {
            if (mPending) {
                genLooper()->delPendingFdWatch(this);
                mPending = false;
            }
        }

        // Fire the watch, i.e. invoke the callback with the right
        // parameters.
        void fire() {
            unsigned events = mLastEvents;
            mLastEvents = 0;
            mCallback(mOpaque, mFd, events);
        }

        TAIL_QUEUE_LIST_TRAITS(Traits, FdWatch, mPendingLink);

    private:
        unsigned mWantedEvents;
        unsigned mLastEvents;
        bool mPending;
        TailQueueLink<FdWatch> mPendingLink;
    };

    void addFdWatch(FdWatch* watch) {
        mFdWatches.add(watch);
    }

    void delFdWatch(FdWatch* watch) {
        mFdWatches.pick(watch);
    }

    void addPendingFdWatch(FdWatch* watch) {
        mPendingFdWatches.insertTail(watch);
    }

    void delPendingFdWatch(FdWatch* watch) {
        mPendingFdWatches.remove(watch);
    }

    void updateFdWatch(int fd, unsigned wantedEvents) {
        mWaiter->update(fd, wantedEvents);
    }

    virtual Looper::FdWatch* createFdWatch(
            int fd,
            Looper::FdWatch::Callback callback,
            void* opaque) {
        return new FdWatch(this, fd, callback, opaque);
    }

    //
    //  T I M E R S
    //

    class Timer : public Looper::Timer {
    public:
        Timer(GenLooper* looper, Callback callback, void* opaque) :
                Looper::Timer(looper, callback, opaque),
                mDeadline(kDurationInfinite),
                mPending(false),
                mPendingLink() {
            DCHECK(mCallback);
            looper->addTimer(this);
        }

        GenLooper* genLooper() const {
            return reinterpret_cast<GenLooper*>(mLooper);
        }

        virtual ~Timer() {
            clearPending();
            genLooper()->delTimer(this);
        }

        Timer* next() const { return mPendingLink.next(); }

        Duration deadline() const { return mDeadline; }

        virtual void startRelative(Duration deadlineMs) {
            if (deadlineMs != kDurationInfinite) {
                deadlineMs += mLooper->nowMs();
            }
            startAbsolute(deadlineMs);
        }

        virtual void startAbsolute(Duration deadlineMs) {
            if (mDeadline != kDurationInfinite) {
                genLooper()->disableTimer(this);
            }
            mDeadline = deadlineMs;
            if (mDeadline != kDurationInfinite) {
                genLooper()->enableTimer(this);
            }
        }

        virtual void stop() {
            startAbsolute(kDurationInfinite);
        }

        virtual bool isActive() const {
            return mDeadline != kDurationInfinite;
        }

        void setPending() {
            mPending = true;
            genLooper()->addPendingTimer(this);
        }

        void clearPending() {
            if (mPending) {
                genLooper()->delPendingTimer(this);
                mPending = false;
                // resets mDeadline so it can be added to active timers list again later
                // without assertion failure inside startAbsolute (genLooper->disableTimer)
                mDeadline = kDurationInfinite;
            }
        }

        void fire() {
            mCallback(mOpaque);
        }

        TAIL_QUEUE_LIST_TRAITS(Traits, Timer, mPendingLink);

    private:
        Duration mDeadline;
        bool mPending;
        TailQueueLink<Timer> mPendingLink;
    };

    void addTimer(Timer* timer) {
        mTimers.add(timer);
    }

    void delTimer(Timer* timer) {
        mTimers.pick(timer);
    }

    void enableTimer(Timer* timer) {
        // Find the first timer that expires after this one.
        Timer* item = mActiveTimers.front();
        while (item && item->deadline() < timer->deadline()) {
            item = item->next();
        }
        if (item) {
            mActiveTimers.insertBefore(item, timer);
        } else {
            mActiveTimers.insertTail(timer);
        }
    }

    void disableTimer(Timer* timer) {
        // Simply remove from the list.
        mActiveTimers.remove(timer);
    }

    void addPendingTimer(Timer* timer) {
        mPendingTimers.insertTail(timer);
    }

    void delPendingTimer(Timer* timer) {
        mPendingTimers.remove(timer);
    }

    virtual Looper::Timer* createTimer(Looper::Timer::Callback callback,
                                       void* opaque) {
        return new GenLooper::Timer(this, callback, opaque);
    }

    //
    //  M A I N   L O O P
    //

    virtual int runWithDeadlineMs(Duration deadlineMs) {
        mForcedExit = false;

        while (!mForcedExit) {
            // Return immediately with EWOULDBLOCK if there are no
            // more timers or watches registered.
            if (mFdWatches.empty() && mActiveTimers.empty()) {
                return EWOULDBLOCK;
            }

            // Compute next deadline from timers.
            Duration nextDeadline = kDurationInfinite;

            Timer* firstTimer = mActiveTimers.front();
            if (firstTimer) {
                nextDeadline = firstTimer->deadline();
            }

            if (nextDeadline > deadlineMs) {
                nextDeadline = deadlineMs;
            }

            // Wait for fd events until next deadline.
            Duration timeOut = INT64_MAX;
            if (nextDeadline < kDurationInfinite) {
                timeOut = nextDeadline - nowMs();
                if (timeOut < 0) timeOut = 0;
            }

            int ret = mWaiter->wait(timeOut);
            if (ret < 0) {
                // Error, force stop.
                break;
            }
            DCHECK(mPendingFdWatches.empty());

            if (ret > 0) {
                // Queue pending FdWatch instances.
                for (;;) {
                    unsigned events;
                    int fd = mWaiter->nextPendingFd(&events);
                    if (fd < 0) {
                        break;
                    }

                    // Find the FdWatch for this file descriptor.
                    // TODO(digit): Improve efficiency with a map?
                    FdWatchSet::Iterator iter(&mFdWatches);
                    while (iter.hasNext()) {
                        FdWatch* watch = iter.next();
                        if (watch->fd() == fd) {
                            watch->setPending(events);
                            break;
                        }
                    }
                }
            }

            // Queue pending expired timers.
            DCHECK(mPendingTimers.empty());

            const Duration kNow = nowMs();
            Timer* timer = mActiveTimers.front();
            while (timer) {
                if (timer->deadline() > kNow) {
                    break;
                }

                // Remove from active list, add to pending list.
                mActiveTimers.remove(timer);
                timer->setPending();
                timer = timer->next();
            }

            // Fire the pending timers, this is done in a separate step
            // because the callback could modify the active timers list.
            for (;;) {
                Timer* timer = mPendingTimers.front();
                if (!timer) {
                    break;
                }
                timer->clearPending();
                timer->fire();
            }

            // Fire the pending fd watches. Also done in a separate step
            // since the callbacks could modify
            for (;;) {
                FdWatch* watch = mPendingFdWatches.front();
                if (!watch) {
                    break;
                }
                watch->clearPending();
                watch->fire();
            }

            if (ret == 0) {
                return ETIMEDOUT;
            }
        }

        return 0;
    }

    typedef TailQueueList<Timer> TimerList;
    typedef ScopedPointerSet<Timer> TimerSet;

    typedef TailQueueList<FdWatch> FdWatchList;
    typedef ScopedPointerSet<FdWatch> FdWatchSet;

private:
    ScopedPtr<SocketWaiter> mWaiter;
    FdWatchSet mFdWatches;         // Set of all fd watches.
    FdWatchList mPendingFdWatches;  // Queue of pending fd watches.

    TimerSet  mTimers;        // Set of all timers.
    TimerList mActiveTimers;  // Sorted list of active timers.
    TimerList mPendingTimers; // Sorted list of pending timers.

    bool mForcedExit;
};

}  // namespace

// static
Looper* Looper::create() {
    return new GenLooper();
}

}  // namespace base
}  // namespace android
