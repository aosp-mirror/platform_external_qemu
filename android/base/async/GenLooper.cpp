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

#include "android/base/async/GenLooper.h"

#include "android/base/Log.h"
#include "android/base/system/System.h"
#include "android/base/sockets/SocketErrors.h"

namespace android {
namespace base {

GenLooper::GenLooper() : mWaiter(SocketWaiter::create()) {}

GenLooper::~GenLooper() = default;

Looper::Duration GenLooper::nowMs(Looper::ClockType clockType) {
    DCHECK(clockType == ClockType::kHost);

    const DurationNs nsTime = nowNs(clockType);
    return nsTime == static_cast<DurationNs>(-1)
                   ? -1
                   : static_cast<Duration>(nsTime / 1000000);
}

Looper::DurationNs GenLooper::nowNs(Looper::ClockType clockType) {
    DCHECK(clockType == ClockType::kHost);

    return System::get()->getUnixTimeUs() * 1000ull;
}

void GenLooper::forceQuit() {
    mForcedExit = true;
}

void GenLooper::addFdWatch(GenLooper::FdWatch* watch) {
    mFdWatches.add(watch);
}

void GenLooper::delFdWatch(GenLooper::FdWatch* watch) {
    mFdWatches.pick(watch);
}

void GenLooper::addPendingFdWatch(GenLooper::FdWatch* watch) {
    mPendingFdWatches.insertTail(watch);
}

void GenLooper::delPendingFdWatch(GenLooper::FdWatch* watch) {
    mPendingFdWatches.remove(watch);
}

void GenLooper::updateFdWatch(int fd, unsigned wantedEvents) {
    mWaiter->update(fd, wantedEvents);
}

Looper::FdWatch* GenLooper::createFdWatch(int fd,
                                          Looper::FdWatch::Callback callback,
                                          void* opaque) {
    return new FdWatch(this, fd, callback, opaque);
}

void GenLooper::addTimer(GenLooper::Timer* timer) {
    mTimers.add(timer);
}

void GenLooper::delTimer(GenLooper::Timer* timer) {
    mTimers.pick(timer);
}

void GenLooper::enableTimer(GenLooper::Timer* timer) {
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

void GenLooper::disableTimer(GenLooper::Timer* timer) {
    // Simply remove from the list.
    mActiveTimers.remove(timer);
}

void GenLooper::addPendingTimer(GenLooper::Timer* timer) {
    mPendingTimers.insertTail(timer);
}

void GenLooper::delPendingTimer(GenLooper::Timer* timer) {
    mPendingTimers.remove(timer);
}

Looper::Timer* GenLooper::createTimer(Looper::Timer::Callback callback,
                                      void* opaque,
                                      Looper::ClockType clock) {
    return new GenLooper::Timer(this, callback, opaque, clock);
}

int GenLooper::runWithDeadlineMs(Looper::Duration deadlineMs) {
    mForcedExit = false;

    while (!mForcedExit) {
        // Return immediately with EWOULDBLOCK if there are no
        // more timers or watches registered.
        if (mFdWatches.empty() && mActiveTimers.empty()) {
            return EWOULDBLOCK;
        }

        // Return immediately with ETIMEDOUT if we've overrun the deadline.
        // This can happen in two ways:
        //     - We waited till timeout on some FDs.
        //     - We have no FDs to watch, but some timers overran the looper
        //       deadline.
        if (nowMs() > deadlineMs) {
            return ETIMEDOUT;
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
            if (timeOut < 0)
                timeOut = 0;
        }

        if (!mFdWatches.empty()) {
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
        } else {
            // We don't have any FDs to watch, so just sleep till the next
            // timer expires.
            System::get()->sleepMs(timeOut);
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
    }

    return 0;
}

GenLooper::FdWatch::FdWatch(GenLooper* looper,
                            int fd,
                            Looper::FdWatch::Callback callback,
                            void* opaque)
    : Looper::FdWatch(looper, fd, callback, opaque),
      mWantedEvents(0U),
      mLastEvents(0U),
      mPending(false),
      mPendingLink() {
    looper->addFdWatch(this);
}

GenLooper* GenLooper::FdWatch::genLooper() const {
    return reinterpret_cast<GenLooper*>(mLooper);
}

GenLooper::FdWatch::~FdWatch() {
    dontWantWrite();
    dontWantRead();
    clearPending();
    genLooper()->delFdWatch(this);
}

void GenLooper::FdWatch::addEvents(unsigned events) {
    events &= kEventMask;
    unsigned newEvents = mWantedEvents | events;
    if (newEvents != mWantedEvents) {
        mWantedEvents = newEvents;
        genLooper()->updateFdWatch(mFd, newEvents);
    }
}

void GenLooper::FdWatch::removeEvents(unsigned events) {
    events &= kEventMask;
    unsigned newEvents = mWantedEvents & ~events;
    if (newEvents != mWantedEvents) {
        mWantedEvents = newEvents;
        genLooper()->updateFdWatch(mFd, newEvents);
        if (!newEvents) {
            clearPending();
        }
    }
    // These events are no longer desired.
    mLastEvents &= ~events;
}

unsigned GenLooper::FdWatch::poll() const {
    return mLastEvents;
}

bool GenLooper::FdWatch::isPending() const {
    return mPending;
}

void GenLooper::FdWatch::setPending(unsigned events) {
    DCHECK(!mPending);
    mPending = true;
    mLastEvents = events & kEventMask;
    genLooper()->addPendingFdWatch(this);
}

void GenLooper::FdWatch::clearPending() {
    if (mPending) {
        genLooper()->delPendingFdWatch(this);
        mPending = false;
    }
}

void GenLooper::FdWatch::fire() {
    unsigned events = mLastEvents;
    mLastEvents = 0;
    mCallback(mOpaque, mFd, events);
}

GenLooper::Timer::Timer(GenLooper* looper,
                        Looper::Timer::Callback callback,
                        void* opaque,
                        Looper::ClockType clock)
    : Looper::Timer(looper, callback, opaque, clock),
      mDeadline(kDurationInfinite),
      mPending(false),
      mPendingLink() {
    // this implementation only supports a host clock
    DCHECK(clock == ClockType::kHost);
    DCHECK(mCallback);
    looper->addTimer(this);
}

GenLooper* GenLooper::Timer::genLooper() const {
    return reinterpret_cast<GenLooper*>(mLooper);
}

GenLooper::Timer::~Timer() {
    clearPending();
    genLooper()->delTimer(this);
}

GenLooper::Timer* GenLooper::Timer::next() const {
    return mPendingLink.next();
}

Looper::Duration GenLooper::Timer::deadline() const {
    return mDeadline;
}

void GenLooper::Timer::startRelative(Looper::Duration deadlineMs) {
    if (deadlineMs != kDurationInfinite) {
        deadlineMs += mLooper->nowMs(mClockType);
    }
    startAbsolute(deadlineMs);
}

void GenLooper::Timer::startAbsolute(Looper::Duration deadlineMs) {
    if (mDeadline != kDurationInfinite) {
        genLooper()->disableTimer(this);
    }
    mDeadline = deadlineMs;
    if (mDeadline != kDurationInfinite) {
        genLooper()->enableTimer(this);
    }
}

void GenLooper::Timer::stop() {
    startAbsolute(kDurationInfinite);
}

bool GenLooper::Timer::isActive() const {
    return mDeadline != kDurationInfinite;
}

void GenLooper::Timer::setPending() {
    mPending = true;
    genLooper()->addPendingTimer(this);
}

void GenLooper::Timer::clearPending() {
    if (mPending) {
        genLooper()->delPendingTimer(this);
        mPending = false;
        // resets mDeadline so it can be added to active timers list again later
        // without assertion failure inside startAbsolute
        // (genLooper->disableTimer)
        mDeadline = kDurationInfinite;
    }
}

void GenLooper::Timer::fire() {
    mCallback(mOpaque, this);
}

void GenLooper::Timer::save(Stream* stream) const {
    stream->putBe64(static_cast<uint64_t>(mDeadline));
}

void GenLooper::Timer::load(Stream* stream) {
    const Duration deadline = static_cast<Duration>(stream->getBe64());
    startAbsolute(deadline);
}

}  // namespace base
}  // namespace android
