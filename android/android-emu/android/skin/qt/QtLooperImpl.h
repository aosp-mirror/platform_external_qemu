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

#pragma once

#include "android/base/Compiler.h"
#include "android/base/async/Looper.h"
#include "android/skin/qt/QtThreading.h"

#include <QThread>
#include <QTimer>
#include <QtGlobal>

#include <utility>

namespace android {
namespace qt {
namespace internal {

typedef ::android::base::Looper BaseLooper;
typedef ::android::base::Looper::Timer BaseTimer;

class TimerImpl : public QTimer {
    DISALLOW_COPY_AND_ASSIGN(TimerImpl);
    Q_OBJECT

public:
    TimerImpl(BaseTimer::Callback callback, void* opaque, BaseTimer* timer)
        : mCallback(callback), mOpaque(opaque), mTimer(timer) {
        moveToMainThread(this);

        connect(this, &QTimer::timeout, this, &TimerImpl::slot_timeout);
        connect(this, &TimerImpl::signalStart, this,
                static_cast<void (QTimer::*)(int)>(&QTimer::start));
        // This connection is dangerous - one must check if it's running in a
        // different thread than the QObject's thread or the signal deadlocks.
        connect(this, &TimerImpl::signalStop, this, &QTimer::stop,
                Qt::BlockingQueuedConnection);
    }

    ~TimerImpl() { stop(); }

    void start(int timeoutMs = 0) { emit signalStart(timeoutMs); }
    void stop() {
        if (thread() == QThread::currentThread()) {
            QTimer::stop();
        } else {
            emit signalStop();
        }
    }

signals:
    void signalStart(int timeoutMs);
    void signalStop();

public slots:
    void slot_timeout() { mCallback(mOpaque, mTimer); }

private:
    BaseTimer::Callback mCallback;
    void* mOpaque;
    BaseTimer* mTimer;
};

class TaskImpl : public QObject, public BaseLooper::Task {
    DISALLOW_COPY_AND_ASSIGN(TaskImpl);
    Q_OBJECT

public:
    TaskImpl(BaseLooper* looper,
             BaseLooper::TaskCallback&& callback,
             bool selfDeleting = false)
        : BaseLooper::Task(looper, std::move(callback)),
          mSelfDeleting(selfDeleting) {
        moveToMainThread(this);

        // Queued connections schedule themselves to run on the target thread's
        // event loop, so this is exactly what we need for the Task
        // implementation.
        connect(this, SIGNAL(runCallback()), this, SLOT(onRunCallback()),
                Qt::QueuedConnection);
    }

    ~TaskImpl() { disconnect(); }

    void schedule() override {
        mCanceled = false;
        emit runCallback();
    }

    void cancel() override { mCanceled = true; }

signals:
    void runCallback();

private slots:
    void onRunCallback() {
        if (!mCanceled) {
            mCallback();
        }
        if (mSelfDeleting) {
            QObject::deleteLater();
        }
    }

private:
    const bool mSelfDeleting;
    bool mCanceled = false;
};

}  // namespace internal
}  // namespace qt
}  // namespace android
