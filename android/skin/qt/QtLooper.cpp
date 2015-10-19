// Copyright 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/skin/qt/QtLooper.h"
#include "android/skin/qt/QtLooperImpl.h"

#include "android/base/containers/TailQueueList.h"
#include "android/base/containers/ScopedPointerSet.h"
#include "android/base/sockets/SocketUtils.h"

#include <QTime>

namespace android {
namespace qt {
namespace internal {

typedef ::android::base::Looper BaseLooper;
typedef ::android::base::Looper::Timer BaseTimer;
typedef ::android::base::Looper::FdWatch BaseFdWatch;

// An implementation of android::base::Looper on top of the Qt main
// event loop. There are few important things here:
//
//  1/ There is a single global Qt event loop per thread. So, all instances
//     returned by createLooper() will really use the same state on a thread!
//
//     In other words, don't call it more than once per thread!
//
//  2/ It is not possible to call the runWithDeadlineMs() method, since
//     the event loop is started in the application's main thread by
//     the executable.
//
//  3/ Clock implemenatation is limited -- non nanosecond presision; only
//     ClockType::kHost is supported.

class QtLooper : public BaseLooper {
public:
    QtLooper() : Looper() {}

    //
    // F D   W A T C H E S
    //
    class FdWatch : public BaseFdWatch {
    public:
        virtual void addEvents(unsigned events) {
            if (events & BaseFdWatch::kEventRead) {
                mReadNotifier.setEnabled(true);
            }
            if (events & BaseFdWatch::kEventWrite) {
                mWriteNotifier.setEnabled(true);
            }
        }

        virtual void removeEvents(unsigned events) {
            if (events & BaseFdWatch::kEventRead) {
                mReadNotifier.setEnabled(false);
            }
            if (events & BaseFdWatch::kEventWrite) {
                mWriteNotifier.setEnabled(false);
            }
        }

        virtual unsigned poll() const {
            // We never cache events.
            return 0;
        }

    protected:
        // |QtLooper| provides method(s) to construct an object of this kind.
        friend class QtLooper;

        FdWatch(QtLooper* looper,
                int fd,
                BaseFdWatch::Callback callback,
                void* opaque)
            : BaseFdWatch(looper, fd, callback, opaque),
              mReadNotifier(fd, QSocketNotifier::Read, callback, opaque),
              mWriteNotifier(fd, QSocketNotifier::Write, callback, opaque) {
            mReadNotifier.setEnabled(false);
            mWriteNotifier.setEnabled(false);
        }

    private:
        SocketNotifierImpl mReadNotifier;
        SocketNotifierImpl mWriteNotifier;

        DISALLOW_COPY_AND_ASSIGN(FdWatch);
    };

    virtual BaseFdWatch* createFdWatch(int fd,
                                       BaseFdWatch::Callback callback,
                                       void* opaque) {
        ::android::base::socketSetNonBlocking(fd);
        return new QtLooper::FdWatch(this, fd, callback, opaque);
    }

    //
    // T I M E R S
    //
    // The underlying timer is of Qt::TimerType Qt::CoarseTimer. This gives us
    // almost millisecond accuracy (5% error margin), but is more effecient.
    class Timer : public BaseTimer {
    public:
        virtual void startRelative(Duration timeout_ms) {
            if (timeout_ms == kDurationInfinite) {
                mTimer.stop();
            } else {
                mTimer.start(timeout_ms);
            }
        }

        virtual void startAbsolute(Duration deadline_ms) {
            Q_ASSERT_X(false, "QLooper::Timer",
                       "startAbsolute is not supported.");
        }

        virtual void stop() { mTimer.stop(); }

        virtual bool isActive() const { return mTimer.isActive(); }

        virtual void save(android::base::Stream* stream) const {
            qDebug("QLooper::Timer does not support serialization. Skipped.");
        }

        virtual void load(android::base::Stream* stream) {
            qDebug("QLooper::Timer does not support deserialization. Skipped.");
        }

    protected:
        // |QtLooper| provides method(s) to construct an object of this kind.
        friend class QtLooper;

        Timer(QtLooper* looper,
              BaseTimer::Callback callback,
              void* opaque,
              ClockType clock)
            : BaseTimer(looper, callback, opaque, clock),
              mTimer(callback, opaque, this) {
            mTimer.setSingleShot(true);
        }

    private:
        TimerImpl mTimer;

        DISALLOW_COPY_AND_ASSIGN(Timer);
    };

    virtual BaseTimer* createTimer(BaseTimer::Callback callback,
                                   void* opaque,
                                   ClockType clock) {
        clock = validateClockType(clock);
        return new QtLooper::Timer(this, callback, opaque, clock);
    }

    //
    //  L O O P E R
    //
    virtual Duration nowMs(ClockType clock) {
        clock = validateClockType(clock);
        auto time = QTime::currentTime();
        return time.msec() +
               1000UL * (time.second() +
                         60UL * (time.minute() + 60UL * time.hour()));
    }

    virtual DurationNs nowNs(ClockType clockType) {
        qWarning("QLooper::nowNs is not supported. Defaulting to nowMs.");
        return nowMs(clockType);
    }

    virtual int runWithDeadlineMs(Duration deadline_ms) {
        qWarning("User cannot call |run*| on a QtLooper event loop");
        errno = ENOSYS;
        return -1;
    }

    virtual void forceQuit() {
        qWarning("User cannot call |forceQuit| on a QtLooper event loop");
    }

protected:
    ClockType validateClockType(ClockType clock) {
        if (clock != ClockType::kHost) {
            qWarning(
                    "QLooper::Timer does not support %s."
                    "Defaulting to  ClockType::kHost.",
                    Looper::clockTypeToString(clock));
        }
        return ClockType::kHost;
    }

private:
    DISALLOW_COPY_AND_ASSIGN(QtLooper);
};

}  // namespace internal

android::base::Looper* createLooper() {
    return new internal::QtLooper();
}

}  // namespace qt
}  // namespace android
