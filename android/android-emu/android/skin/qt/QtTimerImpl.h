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

#include <QTimer>
#include <QtGlobal>

namespace android {
namespace qt {
namespace internal {

typedef ::android::base::Looper BaseLooper;
typedef ::android::base::Looper::Timer BaseTimer;

class TimerImpl : public QTimer {
    Q_OBJECT

public:
    TimerImpl(BaseTimer::Callback callback, void* opaque, BaseTimer* timer)
        : QTimer(), mCallback(callback), mOpaque(opaque), mTimer(timer) {
        QObject::connect(this, &TimerImpl::timeout, this,
                         &TimerImpl::slot_timeout);
    }

public slots:
    void slot_timeout() { mCallback(mOpaque, mTimer); }

private:
    BaseTimer::Callback mCallback;
    void* mOpaque;
    BaseTimer* mTimer;

    DISALLOW_COPY_AND_ASSIGN(TimerImpl);
};

}  // namespace internal
}  // namespace qt
}  // namespace android
