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

#include "android/base/async/DefaultLooper.h"

#include "android/base/Log.h"
#include "android/base/system/System.h"
#include "android/base/sockets/SocketErrors.h"

namespace android {
namespace base {

DefaultLooper::DefaultLooper() : mWaiter(SocketWaiter::create()) {}

DefaultLooper::~DefaultLooper() = default;

Looper::Duration DefaultLooper::nowMs(Looper::ClockType clockType) {
    DCHECK(clockType == ClockType::kHost);

    const DurationNs nsTime = nowNs(clockType);
    return nsTime == static_cast<DurationNs>(-1)
                   ? -1
                   : static_cast<Duration>(nsTime / 1000000);
}

Looper::DurationNs DefaultLooper::nowNs(Looper::ClockType clockType) {
    DCHECK(clockType == ClockType::kHost);

    return System::get()->getUnixTimeUs() * 1000ull;
}

void DefaultLooper::forceQuit() {
    mForcedExit = true;
}

void DefaultLooper::addFdWatch(DefaultLooper::FdWatch* watch) {
    mFdWatches.add(watch);
}

void DefaultLooper::delFdWatch(DefaultLooper::FdWatch* watch) {
    mFdWatches.pick(watch);
}

void DefaultLooper::addPendingFdWatch(DefaultLooper::FdWatch* watch) {
    mPendingFdWatches.insertTail(watch);
}

void DefaultLooper::delPendingFdWatch(DefaultLooper::FdWatch* watch) {
    mPendingFdWatches.remove(watch);
}

void DefaultLooper::updateFdWatch(int fd, unsigned wantedEvents) {
    mWaiter->update(fd, wantedEvents);
}

Looper::FdWatch* DefaultLooper::createFdWatch(int fd,
                                          Looper::FdWatch::Callback callback,
                                          void* opaque) {
    return new FdWatch(this, fd, callback, opaque);
}

void DefaultLooper::addTimer(DefaultLooper::Timer* timer) {
    mTimers.add(timer);
}

void DefaultLooper::delTimer(DefaultLooper::Timer* timer) {
    mTimers.pick(timer);
}

void DefaultLooper::enableTimer(DefaultLooper::Timer* timer) {
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

void DefaultLooper::disableTimer(DefaultLooper::Timer* timer) {
    // Simply remove from the list.
    mActiveTimers.remove(timer);
}

void DefaultLooper::addPendingTimer(DefaultLooper::Timer* timer) {
    mPendingTimers.insertTail(timer);
}

void DefaultLooper::delPendingTimer(DefaultLooper::Timer* timer) {
    mPendingTimers.remove(timer);
}

Looper::Timer* DefaultLooper::createTimer(Looper::Timer::Callback callback,
                                      void* opaque,
                                      Looper::ClockType clock) {
    return new DefaultLooper::Timer(this, callback, opaque, clock);
}

int DefaultLooper::runWithDeadlineMs(Looper::Duration deadlineMs) {
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

        if (!runOneIterationWithDeadlineMs(deadlineMs)) {
            break;
        }
    }

    return 0;
}

bool DefaultLooper::runOneIterationWithDeadlineMs(Looper::Duration deadlineMs) {
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
            return false;
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

    return true;
}

DefaultLooper::FdWatch::FdWatch(DefaultLooper* looper,
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

DefaultLooper* DefaultLooper::FdWatch::defaultLooper() const {
    return reinterpret_cast<DefaultLooper*>(mLooper);
}

DefaultLooper::FdWatch::~FdWatch() {
    dontWantWrite();
    dontWantRead();
    clearPending();
    defaultLooper()->delFdWatch(this);
}

void DefaultLooper::FdWatch::addEvents(unsigned events) {
    events &= kEventMask;
    unsigned newEvents = mWantedEvents | events;
    if (newEvents != mWantedEvents) {
        mWantedEvents = newEvents;
        defaultLooper()->updateFdWatch(mFd, newEvents);
    }
}

void DefaultLooper::FdWatch::removeEvents(unsigned events) {
    events &= kEventMask;
    unsigned newEvents = mWantedEvents & ~events;
    if (newEvents != mWantedEvents) {
        mWantedEvents = newEvents;
        defaultLooper()->updateFdWatch(mFd, newEvents);
        if (!newEvents) {
            clearPending();
        }
    }
    // These events are no longer desired.
    mLastEvents &= ~events;
}

unsigned DefaultLooper::FdWatch::poll() const {
    return mLastEvents;
}

bool DefaultLooper::FdWatch::isPending() const {
    return mPending;
}

void DefaultLooper::FdWatch::setPending(unsigned events) {
    DCHECK(!mPending);
    mPending = true;
    mLastEvents = events & kEventMask;
    defaultLooper()->addPendingFdWatch(this);
}

void DefaultLooper::FdWatch::clearPending() {
    if (mPending) {
        defaultLooper()->delPendingFdWatch(this);
        mPending = false;
    }
}

void DefaultLooper::FdWatch::fire() {
    unsigned events = mLastEvents;
    mLastEvents = 0;
    mCallback(mOpaque, mFd, events);
}

DefaultLooper::Timer::Timer(DefaultLooper* looper,
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

DefaultLooper* DefaultLooper::Timer::defaultLooper() const {
    return reinterpret_cast<DefaultLooper*>(mLooper);
}

DefaultLooper::Timer::~Timer() {
    clearPending();
    defaultLooper()->delTimer(this);
}

DefaultLooper::Timer* DefaultLooper::Timer::next() const {
    return mPendingLink.next();
}

Looper::Duration DefaultLooper::Timer::deadline() const {
    return mDeadline;
}

void DefaultLooper::Timer::startRelative(Looper::Duration deadlineMs) {
    if (deadlineMs != kDurationInfinite) {
        deadlineMs += mLooper->nowMs(mClockType);
    }
    startAbsolute(deadlineMs);
}

void DefaultLooper::Timer::startAbsolute(Looper::Duration deadlineMs) {
    if (mDeadline != kDurationInfinite) {
        defaultLooper()->disableTimer(this);
    }
    mDeadline = deadlineMs;
    if (mDeadline != kDurationInfinite) {
        defaultLooper()->enableTimer(this);
    }
}

void DefaultLooper::Timer::stop() {
    startAbsolute(kDurationInfinite);
}

bool DefaultLooper::Timer::isActive() const {
    return mDeadline != kDurationInfinite;
}

void DefaultLooper::Timer::setPending() {
    mPending = true;
    defaultLooper()->addPendingTimer(this);
}

void DefaultLooper::Timer::clearPending() {
    if (mPending) {
        defaultLooper()->delPendingTimer(this);
        mPending = false;
        // resets mDeadline so it can be added to active timers list again later
        // without assertion failure inside startAbsolute
        // (defaultLooper->disableTimer)
        mDeadline = kDurationInfinite;
    }
}

void DefaultLooper::Timer::fire() {
    mCallback(mOpaque, this);
}

void DefaultLooper::Timer::save(Stream* stream) const {
    stream->putBe64(static_cast<uint64_t>(mDeadline));
}

void DefaultLooper::Timer::load(Stream* stream) {
    const Duration deadline = static_cast<Duration>(stream->getBe64());
    startAbsolute(deadline);
}

}  // namespace base
}  // namespace android
