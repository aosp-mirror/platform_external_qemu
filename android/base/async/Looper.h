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

#ifndef ANDROID_BASE_ASYNC_LOOPER_H
#define ANDROID_BASE_ASYNC_LOOPER_H

#include <android/base/Compiler.h>
#include <android/base/Limits.h>

#include <stddef.h>
#include <stdint.h>

namespace android {
namespace base {

// A Looper is an abstraction for an event loop that can wait for either
// I/O events on file descriptors, or timers.
//
// One can call Looper::create() to create a new instance, create
// Looper::Timer and Looper::FdWatch instances, then call Looper::run()
// to run the loop until there is nothing more to do.
//
// It's possible to stop a Looper from running by calling
// Looper::forceQuit() from within the event loop.
//
class Looper {
public:
    typedef int64_t Duration;

    enum {
        kDurationInfinite = INT64_MAX,
    };

    // Create a new generic Looper instance.
    static Looper* create();

    virtual ~Looper() {}

    // Return the current time as seen by this looper instance in
    // milliseconds.
    virtual Duration nowMs() = 0;

    // Run the event loop until forceQuit() is called or there is no
    // more registered watchers or timers in the looper.
    void run() {
        runWithDeadlineMs(kDurationInfinite);
    }

    // A variant of run() that allows to run the event loop only until
    // a fixed deadline has passed. |deadlineMs| is a deadline in
    // milliseconds relative to the current clock used by nowMs().
    // If can be kDurationInfinite to indicate no deadline.
    virtual int runWithDeadlineMs(Duration deadlineMs) = 0;

    // A variant of run() that allows to run the event loop only until
    // a certain timeout is milliseconds has passed. Return the reason
    // why the looper stopped:
    //    0           -> normal exit through forceQuit()
    //    EWOULDBLOCK -> no more watchers and timers registered.
    //    ETIMEOUT    -> timeout reached.
    int runWithTimeoutMs(Duration timeoutMs) {
        if (timeoutMs != kDurationInfinite) {
            timeoutMs += nowMs();
        }
        return runWithDeadlineMs(timeoutMs);
    }

    // Call this function from within the event loop to force it to quit
    // as soon as possible. runWithDeadlineMS() and runWithTimeoutMs() will
    // return 0.
    virtual void forceQuit() = 0;

    // Interface class for timers implemented by a Looper instance.
    // Use createTimer() to create these.
    class Timer {
    public:
        // Type of callback function called when the timer expires.
        typedef void (*Callback)(void* opaque);

        virtual ~Timer() {}

        // Start, or restart the timer to expire after |timeout_ms|
        // milliseconds.
        virtual void startRelative(Duration timeout_ms) = 0;

        // Start, or restart the timer to expire at |deadline_ms|
        // milliseconds.
        virtual void startAbsolute(Duration deadline_ms) = 0;

        // Stop the timer.
        virtual void stop() = 0;

        // Returns true iff this timer is active.
        virtual bool isActive() const = 0;

    protected:
        Timer(Looper* looper, Callback callback, void* opaque) :
            mLooper(looper), mCallback(callback), mOpaque(opaque) {}

        Looper* mLooper;
        Callback mCallback;
        void* mOpaque;
    };

    // Create a new timer for this Looper instance.
    virtual Timer* createTimer(Timer::Callback callback, void* opaque) = 0;

    // Interface class for I/O event watchers on a given file descriptor
    // implemented by this Looper instance.
    class FdWatch {
    public:
        enum {
            kEventRead = (1 << 0),
            kEventWrite = (1 << 1),

            kEventMask = (kEventRead | kEventWrite),
        };

        // Type of function called when an I/O event occurs.
        // |opaque| is the opaque pointer passed at creation.
        // |fd| is the file descriptor.
        // |events| is an event bitmask.
        typedef void (*Callback)(void* opaque, int fd, unsigned events);

        virtual ~FdWatch() {}

        virtual void addEvents(unsigned events) = 0;

        virtual void removeEvents(unsigned events) = 0;

        inline void wantRead() {
            addEvents(FdWatch::kEventRead);
        }

        inline void wantWrite() {
            addEvents(FdWatch::kEventWrite);
        }

        inline void dontWantRead() {
            removeEvents(FdWatch::kEventRead);
        }

        inline void dontWantWrite() {
            removeEvents(FdWatch::kEventWrite);
        }

        virtual unsigned poll() const = 0;

        int fd() const { return mFd; }

    protected:
        FdWatch(Looper* looper, int fd, Callback callback, void* opaque) :
            mLooper(looper),
            mFd(fd),
            mCallback(callback),
            mOpaque(opaque) {}

        Looper* mLooper;
        int mFd;
        Callback mCallback;
        void* mOpaque;
    };

    // Create a new FdWatch instance from this looper.
    virtual FdWatch* createFdWatch(int fd,
                                   FdWatch::Callback callback,
                                   void* opaque) = 0;

protected:
    // Default constructor is protected. Use create() method to create
    // new generic Looper instances.
    Looper() {}

private:
    DISALLOW_COPY_AND_ASSIGN(Looper);
};

}  // namespace base
}  // namespace android

#endif  // ANDROID_BASE_ASYNC_LOOPER_H
