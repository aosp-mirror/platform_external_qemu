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

#include "android/metrics/PeriodicReporter.h"

#include "android/base/memory/LazyInstance.h"

#include <iterator>
#include <utility>

using android::base::AutoLock;
using android::base::LazyInstance;
using android::base::Looper;
using android::base::System;

namespace android {
namespace metrics {

// An instance holder object: it manages the creation of the default reporter
// instance.
struct PeriodicReporter::InstanceHolder {
    PeriodicReporter::Ptr reporter;

    InstanceHolder() : reporter(new PeriodicReporter(nullptr, nullptr)) {}
};

static LazyInstance<PeriodicReporter::InstanceHolder> sReporter = {};

void PeriodicReporter::start(MetricsReporter* metricsReporter, Looper* looper) {
    sReporter->reporter.reset(new PeriodicReporter(metricsReporter, looper));
}

void PeriodicReporter::stop() {
    sReporter->reporter.reset();
}

PeriodicReporter& PeriodicReporter::get() {
    assert(sReporter->reporter.get());
    return *sReporter->reporter;
}

PeriodicReporter::PeriodicReporter(MetricsReporter* metricsReporter,
                                   Looper* looper)
    : mMetricsReporter(metricsReporter), mLooper(looper) {}

PeriodicReporter::~PeriodicReporter() {
    AutoLock lock(mLock);
    mPeriodDataByPeriod.clear();
}

void PeriodicReporter::addTask(System::Duration periodMs, Callback callback) {
    if (!mMetricsReporter || !mMetricsReporter->isReportingEnabled()) {
        return;
    }

    AutoLock lock(mLock);
    addTaskInternalNoLock(periodMs, std::move(callback));
}

PeriodicReporter::TaskToken PeriodicReporter::addCancelableTask(
        System::Duration periodMs,
        PeriodicReporter::Callback callback) {
    if (!mMetricsReporter || !mMetricsReporter->isReportingEnabled()) {
        return {};
    }

    // Capture a weak pointer to |this| into the task guard to make sure it can
    // outlive |this| and won't try to call into a dangling pointer.
    const auto weakThis = WeakPtr(shared_from_this());

    AutoLock lock(mLock);
    const auto iter = addTaskInternalNoLock(periodMs, std::move(callback));
    lock.unlock();

    return std::make_shared<TaskGuard>([periodMs, iter, weakThis]() {
        if (const auto sharedThis = weakThis.lock()) {
            sharedThis->removeTask(periodMs, iter);
        }
    });
}

PeriodicReporter::CallbackList::iterator
PeriodicReporter::addTaskInternalNoLock(System::Duration periodMs,
                                        PeriodicReporter::Callback callback) {
    PerPeriodData& data = mPeriodDataByPeriod[periodMs];
    data.callbacks.push_back(std::move(callback));
    if (!data.task) {
        // gcc 4.6 used to have a bug where capturing a reference by value
        // resulted in silent bad code generation; workaround it by capturing a
        // pointer explicitly.
        PerPeriodData* const dataPtr = &data;
        data.task.emplace(
                mLooper,
                [this, dataPtr]() {
                    mMetricsReporter->reportConditional(
                        [this, dataPtr](
                                android_studio::AndroidStudioEvent* event) {
                            AutoLock lock(mLock);

                            bool result = false;
                            // Callback may erase itself from the list, deal
                            // with that by using raw iterators in the loop.
                            auto itCallback = dataPtr->callbacks.begin();
                            while (itCallback != dataPtr->callbacks.end()) {
                                const auto itCurrentCallback = itCallback++;
                                // As task may try removing itself, unlock the
                                // object temporarily here.
                                lock.unlock();
                                result |= (*itCurrentCallback)(event);
                                lock.lock();
                            }
                            return result;
                        });
                    return true;
                },
                periodMs);
        data.task->start();
    }

    assert(data.task->inFlight());

    const auto myIt = std::prev(data.callbacks.end());
    return myIt;
}

void PeriodicReporter::removeTask(System::Duration periodMs,
                                  CallbackList::iterator iter) {
    AutoLock lock(mLock);

    PerPeriodData& data = mPeriodDataByPeriod[periodMs];
    data.callbacks.erase(iter);
    if (data.callbacks.empty()) {
        data.task.clear();
    }
}

}  // namespace metrics
}  // namespace android
