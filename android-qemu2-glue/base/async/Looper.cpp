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
#ifdef _MSC_VER
extern "C" {
#include "sysemu/os-win32-msvc.h"
}
#endif
#include "android-qemu2-glue/base/async/Looper.h"

#include "aemu/base/Log.h"
#include "aemu/base/sockets/SocketUtils.h"
#include "android/base/system/System.h"
#include "host-common/vm_operations.h"
#include "android-qemu2-glue/base/files/QemuFileStream.h"
#include "aemu/base/utils/stream.h"

extern "C" {
#include "qemu/osdep.h"
#include "qemu-common.h"
#include "qemu/timer.h"
#include "chardev/char.h"
}  // extern "C"

#include <memory>
#include <string_view>
#include <unordered_set>
#include <utility>

namespace android {
namespace qemu {

static bool sSkipTimerOps = false;

void skipTimerOps() {
    sSkipTimerOps = true;
}

namespace {

extern "C" void qemu_system_shutdown_request(QemuShutdownCause reason);

using android::base::System;

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
    QemuLooper() : mQemuBh(qemu_bh_new(handleBottomHalf, this)) {}

    virtual ~QemuLooper() {
        DCHECK(mPendingFdWatches.empty());
    }

    std::string_view name() const override { return "QEMU2 main loop"; }

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
            updateEvents(0);
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
            if (events == mWantedEvents) {
                return;
            }
            IOHandler* cbRead = (events & kEventRead) ? handleRead : NULL;
            IOHandler* cbWrite = (events & kEventWrite) ? handleWrite : NULL;
            qemu_set_fd_handler(mFd, cbRead, cbWrite,
                                (cbRead || cbWrite) ? this : nullptr);
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
                                       void* opaque) override {
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
                    BaseTimer(looper, callback, opaque, clock) {
            mTimer = ::timer_new(QemuLooper::toQemuClockType(mClockType),
                                 SCALE_MS,
                                 qemuTimerCallbackAdapter,
                                 this);
        }

        ~Timer() {
            if (sSkipTimerOps) return;
            ::timer_join(mTimer);
            ::timer_free(mTimer);
        }

        virtual void startRelative(Duration timeout_ms) override {
            if (sSkipTimerOps) {
                return;
            }

            if (timeout_ms == kDurationInfinite) {
                timer_del(mTimer);
            } else {
                timeout_ms += qemu_clock_get_ms(
                        QemuLooper::toQemuClockType(mClockType));
                timer_mod(mTimer, timeout_ms);
            }
        }

        virtual void startAbsolute(Duration deadline_ms) override {
            if (sSkipTimerOps) {
                return;
            }

            if (deadline_ms == kDurationInfinite) {
                timer_del(mTimer);
            } else {
                timer_mod(mTimer, deadline_ms);
            }
        }

        virtual void stop() override {
            if (sSkipTimerOps) return;
            ::timer_del(mTimer);
        }

        virtual bool isActive() const override {
            if (sSkipTimerOps) return false;
            return timer_pending(mTimer);
        }

        void save(android::base::Stream* stream) const override {
            stream->putBe64(timer_expire_time_ns(mTimer));
        }

        void load(android::base::Stream* stream) override {
            uint64_t deadline_ns = stream->getBe64();
            if (deadline_ns != -1) {
                timer_mod_ns(mTimer, deadline_ns);
            } else {
                timer_del(mTimer);
            }
        }

    private:
        static void qemuTimerCallbackAdapter(void* opaque) {
            Timer* timer = static_cast<Timer*>(opaque);
            timer->mCallback(timer->mOpaque, timer);
        }

        QEMUTimer* mTimer = nullptr;
    };

    virtual BaseTimer* createTimer(BaseTimer::Callback callback,
                                   void* opaque, ClockType clock) override {
        return new QemuLooper::Timer(this, callback, opaque, clock);
    }

    //
    // Tasks
    //

    class Task : public BaseLooper::Task {
    public:
        Task(Looper* looper, BaseLooper::Task::Callback&& callback)
            : BaseLooper::Task(looper, std::move(callback))
            , mBottomHalf(qemu_bh_new(&Task::handleBottomHalf, this)) {}

        ~Task() {
            cancel();
            qemu_bh_delete(mBottomHalf);
        }

        void schedule() override {
            qemu_bh_schedule(mBottomHalf);
        }

        void cancel() override {
            qemu_bh_cancel(mBottomHalf);
        }

    protected:
        static void handleBottomHalf(void* opaque) {
            static_cast<Task*>(opaque)->run();
        }

        virtual void run() {
            mCallback();
        }

        QEMUBH* const mBottomHalf;
    };

    class SelfDeletingTask : public Task {
    public:
        SelfDeletingTask(Looper* looper, BaseLooper::Task::Callback&& callback)
            : Task(looper, std::move(callback)) {}

        void run() override {
            Task::run();
            delete this;
        }
    };

    BaseLooper::TaskPtr createTask(TaskCallback&& callback) override {
        return BaseLooper::TaskPtr(new Task(this, std::move(callback)));
    }

    void scheduleCallback(BaseLooper::TaskCallback&& callback) override {
        // Create a task and forget it - it will take care of itself.
        (new SelfDeletingTask(this, std::move(callback)))->schedule();
    }

    //
    //  L O O P E R
    //

    virtual Duration nowMs(ClockType clockType) override {
        return qemu_clock_get_ms(toQemuClockType(clockType));
    }

    virtual DurationNs nowNs(ClockType clockType) override {
        return qemu_clock_get_ns(toQemuClockType(clockType));
    }

    virtual int runWithDeadlineMs(Duration deadline_ms) override {
        CHECK(false) << "User cannot call looper_run on a QEMU event loop";
        errno = ENOSYS;
        return -1;
    }

    virtual void forceQuit() override {
        qemu_system_shutdown_request(QEMU_SHUTDOWN_CAUSE_HOST_SIGNAL);
    }

private:
    typedef std::unordered_set<FdWatch*> FdWatchSet;

    struct QEMUBHDeleter {
        void operator()(QEMUBH* x) const {
            qemu_bh_delete(x);
        }
    };

    static inline QemuLooper* asQemuLooper(BaseLooper* looper) {
        return reinterpret_cast<QemuLooper*>(looper);
    }

    void addPendingFdWatch(FdWatch* watch) {
        DCHECK(watch);
        DCHECK(!watch->isPending());

        const bool firstFdWatch = mPendingFdWatches.empty();
        CHECK(mPendingFdWatches.insert(watch).second) << "duplicate FdWatch";
        if (firstFdWatch) {
            qemu_bh_schedule(mQemuBh.get());
        }
    }

    void delPendingFdWatch(FdWatch* watch) {
        DCHECK(watch);
        DCHECK(watch->isPending());
        CHECK(mPendingFdWatches.erase(watch) > 0);
    }

    // Called by QEMU as soon as the main loop has finished processed
    // I/O events. Used to look at pending watches and fire them.
    static void handleBottomHalf(void* opaque) {
        QemuLooper* looper = reinterpret_cast<QemuLooper*>(opaque);
        FdWatchSet& pendingFdWatches = looper->mPendingFdWatches;
        const auto end = pendingFdWatches.end();
        auto i = pendingFdWatches.begin();
        while (i != end) {
            FdWatch* watch = *i;
            i = pendingFdWatches.erase(i);
            watch->fire();
        }
    }

    const std::unique_ptr<QEMUBH, QEMUBHDeleter> mQemuBh;
    FdWatchSet mPendingFdWatches;
};

}  // namespace

BaseLooper* createLooper() {
    return new QemuLooper();
}

}  // namespace qemu
}  // namespace android

