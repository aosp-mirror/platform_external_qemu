// Copyright 2016 The Android Open Source Project
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

#include "android/base/containers/TailQueueList.h"
#include "android/base/containers/ScopedPointerSet.h"
#include "android/base/Log.h"
#include "android/base/sockets/SocketUtils.h"
#include "android/base/synchronization/Lock.h"

namespace android {
namespace base {

namespace {

class GenEventHub : public Looper::EventHub {
public:
    // Constructor. |looper| is an android::base::Looper.
    explicit GenEventHub(Looper* looper) : mLooper(looper) {}

    // Destructor.
    virtual ~GenEventHub() {
        delete mSignalWatch;
        socketClose(mSignalPipe[1]);
        socketClose(mSignalPipe[0]);
    }

    // Initialize the hub. Return true on success, false on failure.
    // In case of failure, the only possible thing to do is to delete
    // the instance immediately.
    bool init() {
        if (socketCreatePair(&mSignalPipe[0], &mSignalPipe[1]) < 0) {
            return false;
        }
        mSignalWatch = mLooper->createFdWatch(mSignalPipe[1],
                                              onSignalPipeEvent,
                                              this);
        mSignalWatch->wantRead();
        return true;
    }

    virtual Looper::Event* createEvent(Looper::Event::Callback callback,
                                       void* opaque) override {
        return new GenEventHub::Event(this, callback, opaque);
    }

private:
    // Looper::Event implementation from this class.
    class Event : public Looper::Event {
    public:
        // Constructor. |hub| is the parent EventHub instance that will own
        // the object. |callback| and |opaque| are the event callback and its
        // first parameter value.
        Event(GenEventHub* hub,
              Looper::Event::Callback callback,
              void* opaque) : mHub(hub), mCallback(callback), mOpaque(opaque) {
            mHub->addEvent(this);
        }

        // Destructor.
        virtual ~Event() {
            mHub->deleteEvent(this);
        }

        virtual void disable() override {
            mHub->enableEvent(this, false);
        }

        virtual void enable() override {
            mHub->enableEvent(this, true);
        }

        virtual void signal() override {
            mHub->signalEvent(this);
        }

        virtual int countForTesting() const { return mCount; }

        TAIL_QUEUE_LIST_TRAITS(Traits, Event, mPendingLink);

        GenEventHub* mHub;
        Callback mCallback;
        void* mOpaque;
        int mCount = 0;
        bool mPending = false;
        TailQueueLink<Event> mPendingLink;
        bool mEnabled = true;
    };

    using EventList = TailQueueList<Event>;
    using EventSet = ScopedPointerSet<Event>;
    using EventSetIterator = EventSet::Iterator;

    // Called when at least one byte has been written to the signal pipe.
    // This always runs in the Looper's thread.
    static void onSignalPipeEvent(void* opaque, int fd, unsigned events) {
        auto hub = reinterpret_cast<GenEventHub*>(opaque);
        hub->mLock.lock();

        // Drain the signal pipe first.
        for (;;) {
            char c = '\0';
            int ret = socketRecv(fd, &c, 1);
            if (ret == 1) {
                continue;
            }
            break;
        }

        DCHECK(hub->mPendingEvents.empty()) << "Pending event list is not empty??";

        // Find all events that have a count > 0 and add them to the pending
        // list.
        EventSetIterator iter(&hub->mEvents);
        while (iter.hasNext()) {
            Event* event = iter.next();
            DCHECK(!event->mPending);
            if (event->mEnabled && event->mCount > 0) {
                event->mPending = true;
                hub->mPendingEvents.pushBack(event);
            }
        }

        // Now trigger all events after releasing the lock.
        bool lockHeld = true;
        for (;;) {
            if (!lockHeld) {
                hub->mLock.lock();
            }
            Event* event = hub->mPendingEvents.popFront();
            if (!event) {
                break;
            }
            event->mPending = false;
            int count = event->mCount;
            event->mCount = 0;
            hub->mLock.unlock();
            lockHeld = false;
            event->mCallback(event->mOpaque, count);
        }
        if (lockHeld) {
            hub->mLock.unlock();
        }
    }

    void enableEvent(Event* event, bool enabled) {
        AutoLock lock(mLock);
        bool oldEnabled = event->mEnabled;
        event->mEnabled = enabled;
        if (enabled && !oldEnabled && event->mCount > 0) {
            // Automatically signal that one event is available.
            signalLocked();
        }
    }

    void signalEvent(Event* event) {
        AutoLock lock(mLock);
        event->mCount += 1;
        if (event->mEnabled) {
            signalLocked();
        }
    }

    void addEvent(Event* event) {
        AutoLock lock(mLock);
        mEvents.add(event);
    }

    void deleteEvent(Event* event) {
        AutoLock lock(mLock);
        if (event->mPending) {
            event->mPending = false;
            mPendingEvents.remove(event);
        }
        mEvents.pick(event);
    }

    void signalLocked() {
        char c = '\001';
        int ret = socketSend(mSignalPipe[0], &c, 1);
        DCHECK(ret == 1) << "Could not send byte to EventHub signal pipe";
    }

    Looper* mLooper;
    Lock mLock;
    EventSet mEvents;
    EventList mPendingEvents;
    Looper::FdWatch* mSignalWatch = nullptr;
    int mSignalPipe[2] = { -1, -1 };
};

}  // namespace

// static
Looper::EventHub* Looper::EventHub::create(Looper* looper) {
    return new GenEventHub(looper);
}

}  // namespace base
}  // android
